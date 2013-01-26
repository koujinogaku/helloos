#include "ata.h"
#include "display.h"
#include "string.h"
#include "block.h"
#include "list.h"
#include "errno.h"
#include "memory.h"
#include "../server/iso9660.h"
#include "keyboard.h"
#include "stdlib.h"

static char s[64];

#define CDFS_SECTOR_SIZE  2048

#define ATA_SHMNAME 10001

struct cdfs_directory {
  struct cdfs_directory *next;
  struct cdfs_directory *prev;
  struct cdfs_directory *parent;
  struct cdfs_directory *children;
  struct cdfs_directory *brother;
  struct iso_path_table *path;
  struct iso_directory_record *dir_record;
  unsigned long size;
};

struct cdfs_device {
  int devid;
  unsigned int shmname;
  unsigned long offset;
  void *buf;
  struct iso_primary_descriptor *pvd;
  struct cdfs_directory cdfs_root_directory;
};

struct cdfs_descriptor {
  char status;
  unsigned long current;
  struct cdfs_device *device;
  struct cdfs_directory *dir;
  struct iso_directory_record description;
};

#define CDFS_DESC_TABLE_SIZE 32
static struct cdfs_descriptor cdfs_desc_table[CDFS_DESC_TABLE_SIZE];
static char *cdfs_directory_cache=NULL;
static unsigned long cdfs_directory_cache_lba=0;
static unsigned long cdfs_block_desc=0;
static unsigned long cdfs_block_offset=0;
static char *cdfs_block_buf=0;

void dump(char *addr)
{
  int i,j;
  for(j=0;j<2;j++) {
    long2hex((unsigned long)addr,s);
    display_puts(s);
    for(i=0;i<16;i++,addr++) {
      byte2hex(*addr,s);
      display_puts(" ");
      display_puts(s);
    }
    display_puts("\n");
    syscall_wait(10);
  }
}

int cdfs_read_sectors(struct cdfs_device *device,unsigned long lba, void *buf, unsigned long size)
{
  unsigned long n;
  int seq=0;
  char *p;
  int rc;
  int resultcode;
  int count=1;

  for(n=0,p=buf;n<size;n+=CDFS_SECTOR_SIZE,p+=CDFS_SECTOR_SIZE,lba++) {
    unsigned long len;
    len=size-n;
    if(len>CDFS_SECTOR_SIZE)
      len=CDFS_SECTOR_SIZE;
    rc=ata_read(seq,device->devid,lba,count,device->shmname,device->offset,CDFS_SECTOR_SIZE,&resultcode);
    if(rc<0) {
      display_puts("(read error1)");
      return rc;
    }
    if(resultcode<0) {
      display_puts("(read error2)");
      return resultcode;
    }

    memcpy(p,device->buf,len);
  }
  return 0;
}

void directory_disp_entry(struct iso_directory_record *record)
{
  syscall_wait(10);
  display_puts("(len=");
  int2dec(record->length,s);
  display_puts(s);
  display_puts(")");
  display_puts("(attrlen=");
  int2dec(record->ext_attr_length,s);
  display_puts(s);
  display_puts(")");
  display_puts("(extent=");
  int2dec(record->extent_le,s);
  display_puts(s);
  display_puts(")");
  display_puts("(size=");
  int2dec(record->size_le,s);
  display_puts(s);
  display_puts(")");

  syscall_wait(10);
  display_puts("(flags=");
  int2dec(record->flags,s);
  display_puts(s);
  display_puts(")");
  display_puts("(unitsz=");
  int2dec(record->file_unit_size,s);
  display_puts(s);
  display_puts(")");
  display_puts("(int=");
  int2dec(record->interleave,s);
  display_puts(s);
  display_puts(")");
  display_puts("(seq=");
  int2dec(record->volume_sequence_number_le,s);
  display_puts(s);
  display_puts(")");

  syscall_wait(10);
  display_puts("(namelen=");
  int2dec(record->name_len,s);
  display_puts(s);
  display_puts(")");
  display_puts("(name=");
  memcpy(s,record->name,record->name_len);
  s[record->name_len]=0;
  display_puts(s);
  display_puts(")");

  display_puts("\n");
}

void directory_disp_path(struct iso_path_table *path)
{
  display_puts("(namelen=");
  int2dec(path->name_len,s);
  display_puts(s);
  display_puts(")");
  display_puts("(attrlen=");
  byte2hex(path->ext_attr_len,s);
  display_puts(s);
  display_puts(")");
  display_puts("(extent=");
  int2dec(path->extent,s);
  display_puts(s);
  display_puts(")");
  display_puts("(parent=");
  word2hex(path->parent,s);
  display_puts(s);
  display_puts(")");
  display_puts("(name=");
  memcpy(s,path->name,path->name_len);
  s[path->name_len]=0;
  display_puts(s);
  display_puts(")");

  display_puts("\n");
}


static int directory_add_path(struct cdfs_directory *tree, struct iso_path_table *path, unsigned long path_table_size)
{
  unsigned long l,n=0;
  struct cdfs_directory *record[256];
  int path_id=1;
  int parent_id;
  int count=0;

  while(n<path_table_size) {
    struct cdfs_directory *entry = malloc(sizeof(struct cdfs_directory));
    memset(entry,0,sizeof(struct cdfs_directory));
    record[path_id-1] = entry;

    entry->path = path;
    parent_id = path->parent;
    if(path_id>0 && parent_id>0 && parent_id<256)
      entry->parent = record[parent_id-1];

    if(entry->parent != NULL) {
      if(entry->parent->children == NULL)
        entry->parent->children = entry;
      else {
        struct cdfs_directory *children=entry->parent->children;
        for(;;) {
          if(children->brother==NULL) {
            children->brother=entry;
            break;
          }
          children = children->brother;
        }
      }
    }

//    directory_disp_path(path);

    l = sizeof(struct iso_path_table) + path->name_len + (path->name_len % 2);
    path = (void*)((unsigned long)path + l);
    n += l;
    list_add_tail(tree,entry);
    path_id++;
    count++;
//    if(count%10==9)
//      keyboard_getcode();
  }

  display_puts("(path count=");
  int2dec(count,s);
  display_puts(s);
  display_puts(")\n");
  return 0;
}

int cdfs_iso_getVolDesc(struct cdfs_device *device)
{
  unsigned long lba;
  int rc;
  struct iso_volume_descriptor  bvd;
  unsigned long buflen;
  
  // --------- Get primary volume descriptor --------
  for(lba=16;lba<30;lba++) {
    rc=cdfs_read_sectors(device,lba,&bvd,sizeof(bvd));
    if(rc<0) {
      display_puts("(read error:primary volume descriptor)");
      return rc;
    }

    display_puts("(type=");
    int2dec(bvd.type,s);
    display_puts(s);
    strncpy(s,bvd.id,sizeof(bvd.id));
    s[sizeof(bvd.id)]=0;
    display_puts(",id=");
    display_puts(s);
    display_puts(")");

    if(bvd.type!=ISO_VD_TYPE_PRIMARY)
      break;
    if(strncmp(bvd.id,ISO_STANDARD_ID,sizeof(bvd.id))!=0)
      break;

    if(device->pvd==NULL)
      device->pvd=malloc(CDFS_SECTOR_SIZE);
    memcpy(device->pvd,&bvd,sizeof(bvd));

  }
  if(device->pvd==NULL) {
    display_puts("(not found:primary volume descriptor)");
    return ERRNO_NOTEXIST;
  }

    display_puts("(pathtblsize=");
    int2dec(device->pvd->path_table_size_le,s);
    display_puts(s);
    display_puts(",typepathtbl=");
    int2dec(device->pvd->type_l_path_table_le,s);
    display_puts(s);
    display_puts(")");

  // ------- Get path table --------
  syscall_wait(10);
  lba    = device->pvd->type_l_path_table_le;
  buflen = device->pvd->path_table_size_le;
  struct iso_path_table *path_table=malloc(buflen);

  rc=cdfs_read_sectors(device,lba,path_table,buflen);
  if(rc<0) {
    display_puts("(read error1:path table)");
    return rc;
  }

  syscall_wait(10);
  display_puts("\n");

  list_init(&(device->cdfs_root_directory));
  directory_add_path(&(device->cdfs_root_directory),path_table,buflen);


  // ------- Get root directory --------
  struct cdfs_directory *root = device->cdfs_root_directory.next;
  root->dir_record = (struct iso_directory_record *)device->pvd->root_directory_record;
  root->size = root->dir_record->size_le;
  //syscall_wait(10);
  //directory_disp_entry(root->dir_record);

  return 0;
}

int cdfs_disp_directory(struct cdfs_device *device, unsigned long lba,unsigned long buflen)
{
  int rc;
  char *record = malloc(buflen);

  rc=cdfs_read_sectors(device,lba,record,buflen);
  if(rc<0) {
    display_puts("(read error1:path table)");
    return rc;
  }

  struct iso_directory_record *dirrec;
  char *p;
  unsigned long n;
  for(p=record,n=0; n<buflen; p+=dirrec->length,n+=dirrec->length) {
    dirrec=(void*)p;
    directory_disp_entry(dirrec);
  }

  return 0;
}

char *cdfs_chop_path(char *path, char *name)
{
  int count=256;
  while(*path!=0 && *path!='/') {
    if(count>0) {
      *name=*path;
      name++;
    }
    path++;
    count--;
  }
  *name=0;
  if(*path=='/')
    path++;
  return path;
}

struct cdfs_directory *cdfs_search_path(struct cdfs_device *device, char *fullpath, char **filename)
{
  struct cdfs_directory *dir = device->cdfs_root_directory.next;
  char *pathp;
  char name[256];
  struct cdfs_directory *target;

  if(*fullpath=='/')
    fullpath++;
  pathp = fullpath;
  *filename=pathp;
  target=dir;

  while(dir!=NULL && *pathp!=0) {
    pathp = cdfs_chop_path(pathp,name);

/*
syscall_wait(10);
directory_disp_path(dir->path);
display_puts("(filename=");
display_puts(name);
display_puts(")\n");
display_puts("(child=");
long2hex((long)(dir->children),s);
display_puts(s);
display_puts(")\n");
*/

    dir=dir->children;
    while(dir!=NULL) {

//syscall_wait(10);
//directory_disp_path(dir->path);

      if(strlen(name)==dir->path->name_len && 
         strncmp(name,dir->path->name,dir->path->name_len)==0) {
        target=dir;
        *filename=pathp;
        break;
      }
      dir=dir->brother;
    }
  }

  return target;
}

int cdfs_cache_directory(struct cdfs_device *device, unsigned long lba, char** bufp)
{
  int seq=0;
  int rc;
  int resultcode;
  int count=1;

  rc=ata_read(seq,device->devid,lba,count,device->shmname,device->offset,CDFS_SECTOR_SIZE,&resultcode);
  if(rc<0) {
    display_puts("(read error1)");
    return rc;
  }
  if(resultcode<0) {
    display_puts("(read error2)");
    return resultcode;
  }
  struct iso_directory_record *dirrec=device->buf;
  if(dirrec->name_len!=1 || dirrec->name[0]!=0) {
    display_puts("(directory record error)");
    return ERRNO_CTRLBLOCK;
  }
  unsigned long buflen=dirrec->size_le;
  char *buf=malloc(buflen);
  memcpy(buf,device->buf,min(buflen,CDFS_SECTOR_SIZE));
  if(buflen>CDFS_SECTOR_SIZE) {
    rc=cdfs_read_sectors(device,lba+1, buf+=CDFS_SECTOR_SIZE, buflen-CDFS_SECTOR_SIZE);
    if(rc<0) {
      display_puts("(read error1)");
      mfree(buf);
      return rc;
    }
  }
  *bufp = buf;
  return 0;
}
int cdfs_search_entry(char *buf, char *filename, struct iso_directory_record **record)
{
  char *p;
  unsigned long n;
  struct iso_directory_record *dirrec=(struct iso_directory_record *)buf;
  unsigned long buflen=dirrec->size_le;
  int len=strlen(filename);
  for(p=buf,n=0; n<buflen; p+=dirrec->length,n+=dirrec->length) {
    dirrec=(void*)p;
    if(len==dirrec->name_len && strncmp(filename,dirrec->name,len)==0)
      break;
  }
  if(n>=buflen)
    return ERRNO_NOTEXIST;

  *record = dirrec;
  return 0;
}

int cdfs_open(struct cdfs_device *device, char *fullpath)
{
  struct cdfs_directory *dir;
  char *filename;
  int rc;

  dir=cdfs_search_path(device,fullpath,&filename);
  if(dir==NULL)
    return ERRNO_CTRLBLOCK;

  directory_disp_path(dir->path);
  display_puts("(filename=");
  display_puts(filename);
  display_puts(")\n");

  if(cdfs_directory_cache_lba!=dir->path->extent) {
    if(cdfs_directory_cache!=NULL) {
      mfree(cdfs_directory_cache);
      cdfs_directory_cache=NULL;
    }
    rc = cdfs_cache_directory(device, dir->path->extent, &cdfs_directory_cache);
    if(rc<0) {
      return rc;
    }
  }
  struct iso_directory_record *entry;
  rc = cdfs_search_entry(cdfs_directory_cache, filename, &entry);
  if(rc<0) {
display_puts("(not found)");
    return rc;
  }

syscall_wait(10);
directory_disp_entry(entry);

  int desc_num;
  list_alloc(cdfs_desc_table,CDFS_DESC_TABLE_SIZE,desc_num);
  if(desc_num==0)
    return ERRNO_RESOURCE;

  cdfs_desc_table[desc_num].current=0;
  cdfs_desc_table[desc_num].device=device;
  cdfs_desc_table[desc_num].dir=dir;
  memcpy(&(cdfs_desc_table[desc_num].description),entry,sizeof(struct iso_directory_record));

  return desc_num;
}

int cdfs_close(int desc_num)
{
  if(desc_num<1 || desc_num>=CDFS_DESC_TABLE_SIZE)
    return ERRNO_NOTEXIST;
  cdfs_desc_table[desc_num].status=0;
  return 0;
}

int cdfs_read(int desc_num,void *bufv, unsigned long size)
{
  return 0;
}

int init_shm(void)
{
  unsigned int shmname=ATA_SHMNAME;

  cdfs_block_desc = block_init(shmname);
  if(cdfs_block_desc==0)
    return ERRNO_NOTINIT;

  cdfs_block_offset = block_alloc(cdfs_block_desc);
  if(cdfs_block_offset==0)
    return ERRNO_NOTINIT;

  cdfs_block_buf = block_addr(cdfs_block_desc,cdfs_block_offset);

  return 0;
}

int getinfo(struct cdfs_device device[])
{
  int rc;
  unsigned short seq=1;
  unsigned short devid=0;
  int typecode;

  for(devid=0;devid<4;devid++) {
    rc=ata_getinfo(seq++,devid,&typecode);
    if(rc<0) {
      display_puts("rc=");
      sint2dec(rc,s);
      display_puts(s);
      break;
    }
    display_puts("\n=====");
    display_puts("devid=");
    int2dec(devid,s);
    display_puts(s);
    display_puts(",type=");
    sint2dec(typecode,s);
    display_puts(s);
    display_puts("=====\n");

    device[devid].devid=devid;
    device[devid].shmname=ATA_SHMNAME;
    device[devid].offset=cdfs_block_offset;
    device[devid].buf=cdfs_block_buf;
  }
  return 0;
}

int start(int ac, char *av[])
{

  struct cdfs_device device_table[4];
  struct cdfs_device *device;
  memset(&device,0,sizeof(device));

  memset(&cdfs_desc_table,0,sizeof(cdfs_desc_table));

  init_shm();
  char fullpath[256];

  getinfo(device_table);
  device=&device_table[2];

  cdfs_iso_getVolDesc(device);

  // --------------------------------------
// SYSTEM32
  unsigned long lba    = 181;
  unsigned long buflen = 152;
//  lba    = root->dir_record->extent_le;
//  buflen = root->size;
  cdfs_disp_directory(device, lba, buflen);

  //strncpy(fullpath,"/",256);
  //strncpy(fullpath,"/A",256);
  //strncpy(fullpath,"/I386/SYSTEM32",256);
  strncpy(fullpath,"/I386/SYSTEM32/NTDLL.DLL",256);
  //strncpy(fullpath,"/I386/SYSTEM31/NTDLL.DLL",256);
  cdfs_open(device, fullpath);

  return 0;
}
