#include "cdfs.h"
#include "ata.h"
#include "display.h"
#include "string.h"
#include "block.h"
#include "list.h"
#include "errno.h"
#include "memory.h"
#include "iso9660.h"
#include "keyboard.h"
#include "stdlib.h"
#include "message.h"

static char s[64];

struct cdfs_device {
  int devid;
  unsigned int shmname;
  unsigned long offset;
  void *buf;
  int pvd_available;
  struct iso_primary_descriptor *pvd;
  struct iso_path_table *path_table;
  unsigned long path_table_size;
  char         *directory_cache;
  unsigned long directory_cache_size;
  unsigned long directory_cache_lba;
};

#define CDFS_DESC_OPEN    1
#define CDFS_DESC_DIROPEN 2
#define CDFS_DESC_EJECT   3

struct cdfs_descriptor {
  char status;
  char opentype;
  struct cdfs_device *device;
  char *cache;
  unsigned long cur_pos;
  struct iso_directory_record description;
};

#define CDFS_DESC_TABLE_SIZE 32
static struct cdfs_descriptor cdfs_desc_table[CDFS_DESC_TABLE_SIZE];
static unsigned long cdfs_block_desc=0;
static unsigned long cdfs_block_offset=0;
static char         *cdfs_block_buf=0;
static struct cdfs_device cdfs_device_table;
static int cdfs_seqnum=0;

void cdfs_disable_pvdcache(struct cdfs_device *device);
void cdfs_set_alldescriptor(int opentype);

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

int cdfs_read_sectors(struct cdfs_device *device,unsigned long lba, void *buf, unsigned long size)
{
  unsigned long n;
  char *p;
  int rc;
  int resultcode;
  int count=1;

  for(n=0,p=buf;n<size;n+=CDFS_SECTOR_SIZE,p+=CDFS_SECTOR_SIZE,lba++) {
    unsigned long len;
    len=size-n;
    if(len>CDFS_SECTOR_SIZE)
      len=CDFS_SECTOR_SIZE;
    rc=ata_read(cdfs_seqnum++,device->devid,lba,count,device->shmname,device->offset,CDFS_SECTOR_SIZE,&resultcode);
    if(rc<0) {
      display_puts("(read error1)");
      return rc;
    }
    if(resultcode<0) {
//display_puts("(read error2=");
//sint2dec(resultcode,s);
//display_puts(s);
//if(ata_is_sensekey(resultcode)) {
//  display_puts(":");
//  display_puts(ata_sense_key_message(ata_errno2sensekey(resultcode)));
//}
//display_puts(")");
      if(resultcode==ERRNO_SCSI_UnitAttention)
        cdfs_disable_pvdcache(device);
      return resultcode;
    }

    memcpy(p,device->buf,len);
  }
  return 0;
}

int cdfs_iso_get_volume(struct cdfs_device *device)
{
  unsigned long lba;
  int rc;
  struct iso_volume_descriptor  bvd;
  
//display_puts("(reading pvd)");
  // --------- Get primary volume descriptor --------
  for(lba=16;lba<30;lba++) {
    rc=cdfs_read_sectors(device,lba,&bvd,sizeof(bvd));
    if(rc<0) {
//      display_puts("(read error:primary volume descriptor)");
      return rc;
    }

//    display_puts("(type=");
//    int2dec(bvd.type,s);
//    display_puts(s);
//    strncpy(s,bvd.id,sizeof(bvd.id));
//    s[sizeof(bvd.id)]=0;
//    display_puts(",id=");
//    display_puts(s);
//    display_puts(")");

    if(bvd.type!=ISO_VD_TYPE_PRIMARY)
      break;
    if(strncmp(bvd.id,ISO_STANDARD_ID,sizeof(bvd.id))!=0)
      break;

    if(device->pvd==NULL)
      device->pvd=malloc(CDFS_SECTOR_SIZE);
    if(device->pvd==NULL)
      return ERRNO_RESOURCE;
    memcpy(device->pvd,&bvd,sizeof(bvd));

  }
  if(device->pvd==NULL) {
    display_puts("(not found:primary volume descriptor)");
    return ERRNO_NOTEXIST;
  }

//display_puts("(done pvd)");

//    display_puts("(pathtblsize=");
//    int2dec(device->pvd->path_table_size_le,s);
//    display_puts(s);
//    display_puts(",typepathtbl=");
//    int2dec(device->pvd->type_l_path_table_le,s);
//    display_puts(s);
//    display_puts(")");

  // ------- Get path table --------
//syscall_wait(10);
//display_puts("(reading path table)");
  lba    = device->pvd->type_l_path_table_le;

  if(device->path_table!=NULL)
    mfree(device->path_table);
  device->path_table_size = device->pvd->path_table_size_le;
  device->path_table = malloc(device->path_table_size);
  if(device->path_table==NULL)
    return ERRNO_RESOURCE;

  rc=cdfs_read_sectors(device,lba,device->path_table,device->path_table_size);
  if(rc<0) {
    display_puts("(read error1:path table)");
    return rc;
  }

//display_puts("(done path table)");
//  syscall_wait(10);
//  display_puts("\n");



  // ------- Get root directory --------
  //root->dir_record = (struct iso_directory_record *)device->pvd->root_directory_record;
  //root->size = root->dir_record->size_le;
  //syscall_wait(10);
  //directory_disp_entry(root->dir_record);

  return 0;
}

void cdfs_disable_pvdcache(struct cdfs_device *device)
{
  device->pvd_available=0;
}

int cdfs_check_pvdcache(struct cdfs_device *device)
{
  int rc;
  if(device->pvd_available)
    return 0;

//display_puts("(checking pvd)");
  int retry;
  for(retry=0;retry<3;retry++) {
    rc = cdfs_iso_get_volume(device);
    if(rc!=ERRNO_SCSI_UnitAttention)
      break;
    cdfs_set_alldescriptor(CDFS_DESC_EJECT);
  }
  if(rc==0)
    device->pvd_available=1;

  return rc;
}

char *cdfs_chop_path(char *path, char *name)
{
  int count=CDFS_FULLPATH_MAXLENGTH;
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

struct iso_path_table *cdfs_search_path(struct cdfs_device *device, char *fullpath, char **filename)
{
  char name[CDFS_FULLPATH_MAXLENGTH];
  char *pathname=fullpath;
  struct iso_path_table *path = device->path_table;
  int path_id=1;
  int perent_id=1;
  int in_dir=0;

  struct iso_path_table *target = path;

  if(pathname[0]=='/' && pathname[1]==0) {
    *filename=&pathname[1];
    return target;
  }
  if(*pathname=='/')
    pathname++;

  for(;;) { // pathname
    *filename=pathname;
    pathname = cdfs_chop_path(pathname,name);
//display_puts("\n(name=");
//display_puts(name);
//display_puts(",pathname=");
//display_puts(pathname);
//display_puts(")");
    if(*pathname==0 && *name==0)
      break;
    int namelen = strlen(name);
    if(namelen==0)
      namelen=1;
    for(;;) { // path
      if((unsigned long)path - (unsigned long)device->path_table >= device->path_table_size) {
        return target;
      }
      path_id++;
      unsigned long l = sizeof(struct iso_path_table) + path->name_len + (path->name_len % 2);
      path = (void*)((unsigned long)path + l);
      if(path->parent == perent_id) {
        // in target directory
        if(namelen==path->name_len    && 
           strncmp(name,path->name,path->name_len)==0) {
          perent_id = path_id;
          target = path;
          in_dir = 0;
          break;
        }
        in_dir = 1;
      }
      else {
        // out of target directory
        if(in_dir)
          return target;
      }
    }//end of for(path)
  }//end of for(pathname)

  return target;
}

int cdfs_cache_directory(struct cdfs_device *device, unsigned long lba, char** bufp, unsigned long *buflenp)
{
  int rc;
  int resultcode;
  int count=1;

  rc=ata_read(cdfs_seqnum++,device->devid,lba,count,device->shmname,device->offset,CDFS_SECTOR_SIZE,&resultcode);
  if(rc<0) {
//    display_puts("(read error1)");
    return rc;
  }
  if(resultcode<0) {
//    display_puts("(read error2)");
    if(resultcode==ERRNO_SCSI_UnitAttention)
      cdfs_disable_pvdcache(device);
    return resultcode;
  }
  struct iso_directory_record *dirrec=device->buf;
  if(dirrec->name_len!=1 || dirrec->name[0]!=0) {
//    display_puts("(directory record error)");
    return ERRNO_CTRLBLOCK;
  }
  unsigned long buflen=dirrec->size_le;
  char *buf=malloc(buflen);
  if(buf==NULL)
    return ERRNO_RESOURCE;
  memcpy(buf,device->buf,min(buflen,CDFS_SECTOR_SIZE));
  if(buflen>CDFS_SECTOR_SIZE) {
    rc=cdfs_read_sectors(device,lba+1, buf+CDFS_SECTOR_SIZE, buflen-CDFS_SECTOR_SIZE);
    if(rc<0) {
//      display_puts("(read error1)");
      mfree(buf);

      return rc;
    }
  }
  *bufp = buf;
  *buflenp = buflen;
  return 0;
}

struct iso_directory_record *cdfs_nextentry(void *bufv,unsigned long buflen, unsigned long *pos)
{
  char *buf = bufv;
  struct iso_directory_record *entry;
  for(;;) {
    if(*pos >= buflen) {
      return NULL;
    }

//display_puts("(pos=");
//int2dec(*pos,s);
//display_puts(s);
//display_puts(")");
    entry=(struct iso_directory_record*)&(buf[*pos]);
//directory_disp_entry(entry);
//syscall_wait(500);
    if(entry->length != 0) {
      break;
    }
//display_puts("length=0\n");
//syscall_wait(1000);
    *pos = (*pos - (*pos % CDFS_SECTOR_SIZE)) + CDFS_SECTOR_SIZE;
  }
  *pos = *pos + entry->length;

  return entry;
}
/*
int cdfs_compare_filename(char *filename, int flen, char *entryname, int elen)
{
  for(;;) {
    if(flen<=0) {
      if(elen>0) {
        if(*entryname==0||*entryname==';')
          return 0;
        else
          return -1;
      }
      return 0;
    }
    if(elen<=0) {
      if(flen>0) {
        if(*filename==0)
          return 0;
        else
          return -1;
      }
      return 0;
    }

    if(*filename != *entryname)
      return -1;
    if(*filename==0 || *entryname==0 || *entryname==';')
      break;

    filename++;
    entryname++
    flen--;
    elen--;
  }

  if(*filename==0) {
    if(*entryname==0 || *entryname==';')
      return 0;
    else
      return -1;
  }
  return -1;
}
*/
unsigned int cdfs_filename_length(char *filename, unsigned int len)
{
  unsigned int n;
  for(n=0;n<len;n++) {
    if(*filename==0 || *filename==';')
      break;
    filename++;
  }
  return n;
}

int cdfs_search_entry(char *buf, unsigned long buflen, char *filename, struct iso_directory_record **record)
{
  int search_dot=0;
  int len=strlen(filename);
  if(strncmp(filename,".",2)==0)
    search_dot=1;
  unsigned long pos=0;
  for(;;) {
    struct iso_directory_record *dirrec=cdfs_nextentry(buf, buflen, &pos);
    if(dirrec==NULL)
      return ERRNO_NOTEXIST;
    int name_len = cdfs_filename_length(dirrec->name,dirrec->name_len);
//display_puts("(name=");
//display_puts(dirrec->name);
//display_puts(",len=");
//int2dec(name_len,s);
//display_puts(s);
//display_puts(")");
//syscall_wait(10);
    if(len==name_len) {
      char entry_name[CDFS_FULLPATH_MAXLENGTH];
      int i;
      for(i=0;i<name_len;i++) {
        entry_name[i]=toupper(dirrec->name[i]);
      }
      if(strncmp(filename,entry_name,len)==0) {
        *record = dirrec;
        return 0;
      }
    }
    else if(search_dot && dirrec->name_len==1 && dirrec->name[0]==0) {
      *record = dirrec;
      return 0;
    }
  }
  return ERRNO_NOTEXIST;
}

void cdfs_set_alldescriptor(int opentype)
{
  int desc_num;
  for(desc_num=1; desc_num<CDFS_DESC_TABLE_SIZE; desc_num++) {
    if(cdfs_desc_table[desc_num].status) {
      cdfs_desc_table[desc_num].opentype = opentype;
    }
  }
}

int cdfs_get_descriptor(int desc_num,struct cdfs_descriptor **desc)
{
  if(desc_num<1 || desc_num>=CDFS_DESC_TABLE_SIZE) {
    return ERRNO_NOTEXIST;
  }
  *desc = &cdfs_desc_table[desc_num];
  if((*desc)->status==0) {
    return ERRNO_NOTEXIST;
  }
  if((*desc)->opentype==CDFS_DESC_EJECT) {
    return ERRNO_MODE;
  }
  return 0;
}

int cdfs_open_descriptor(struct cdfs_device *device, char *fullpath)
{
  int opentype;
  //struct cdfs_directory *dir;
  char *filename;
  int rc;
  struct iso_path_table *path;
  int desc_num;
  int resultcode;

  rc=cdfs_check_pvdcache(device);
  if(rc<0) {
    desc_num = rc;
    rc = 0;
    return desc_num;
  }

  path=cdfs_search_path(device,fullpath,&filename);

//  directory_disp_path(path);
//  display_puts("(filename=");
//  display_puts(filename);
//  display_puts(")\n");

  if(*filename==0) {
//    display_puts("(opentype=dir)");
    opentype=CDFS_DESC_DIROPEN;
  }
  else {
//    display_puts("(opentype=file)");
    opentype=CDFS_DESC_OPEN;
  }

  if(device->directory_cache_lba==path->extent) {
    rc = ata_checkmedia(cdfs_seqnum++,device->devid,&resultcode);
    if(rc<0) {
      desc_num = rc;
      return desc_num;
    }
    if(resultcode<0) {
      cdfs_disable_pvdcache(device);
      return resultcode;
    }
  }
  else {
//display_puts("(cache_directory)");
    if(device->directory_cache!=NULL) {
      mfree(device->directory_cache);
      device->directory_cache=NULL;
    }
    rc = cdfs_cache_directory(device, path->extent, &device->directory_cache, &device->directory_cache_size);
    if(rc<0) {
      desc_num = rc;
      return desc_num;
    }
    device->directory_cache_lba = path->extent;
  }

//display_puts("(cachesize=");
//int2dec(device->directory_cache_size,s);
//display_puts(s);
//display_puts(")");

  struct iso_directory_record *entry;
  char *dircache=NULL;
  if(opentype==CDFS_DESC_OPEN) {
    rc = cdfs_search_entry(device->directory_cache, device->directory_cache_size, filename, &entry);
    if(rc<0) {
//display_puts("(error search_entry)");
      desc_num = rc;
      if(rc==ERRNO_NOTEXIST) {
//display_puts("(not found)");
        rc = 0;
      }
      return desc_num;
    }
  }
  else {
    dircache=malloc(device->directory_cache_size);
    if(dircache==NULL) {
      rc = ERRNO_RESOURCE;
      desc_num = rc;
      return desc_num;
    }
    memcpy(dircache,device->directory_cache,device->directory_cache_size);
    entry = (void*)dircache;
  }

//syscall_wait(10);
//directory_disp_entry(entry);

  list_alloc(cdfs_desc_table,CDFS_DESC_TABLE_SIZE,desc_num);
  if(desc_num==0) {
    if(dircache!=NULL)
      mfree(dircache);
    desc_num = ERRNO_RESOURCE;
    return desc_num;
  }

  cdfs_desc_table[desc_num].device=device;
  cdfs_desc_table[desc_num].opentype=opentype;
  cdfs_desc_table[desc_num].cache=dircache;
  cdfs_desc_table[desc_num].cur_pos=0;
  memcpy(&(cdfs_desc_table[desc_num].description),entry,sizeof(struct iso_directory_record));

  return desc_num;
}

int cdfs_cmd_open(union cdfs_msg *cmd)
{
  union cdfs_msg res;
  int desc_num;
  int retry;
  int len;
  struct cdfs_device *device=&cdfs_device_table;

  len = (unsigned long)(cmd->open.req.h.size) - (((unsigned long)&(cmd->open.req.fullpath[0]))-((unsigned long)cmd));
  char *fn = cmd->open.req.fullpath;
  for(;len>0;len--,fn++)
    *fn=toupper(*fn);

  for(retry=0;retry<5;retry++) {
    desc_num = cdfs_open_descriptor(device,cmd->open.req.fullpath);
    // This is for the VirtualBox bug. VirtualBox reports not UnitAttention but also NotReady when Media Changing.
    // And VirtualBox reports errors two times.
    if(desc_num!=ERRNO_SCSI_UnitAttention && desc_num!=ERRNO_SCSI_NotReady)
      break;
  }

  res.open.res.h.size = sizeof(struct cdfs_res_open);
  res.open.res.h.service = cmd->open.req.h.service;
  res.open.res.h.command = cmd->open.req.h.command;
  res.open.res.seq = cmd->open.req.seq;
  res.open.res.handle = desc_num;
  message_send(cmd->open.req.h.arg, &res);

  return 0;
}

int cdfs_cmd_close(union cdfs_msg *cmd)
{
  int desc_num = cmd->close.req.handle;
  int rc;
  union cdfs_msg res;
  struct cdfs_descriptor *desc;
  int resultcode;

  rc = cdfs_get_descriptor(desc_num,&desc);
  if(rc<0 && rc!=ERRNO_MODE) {
    resultcode = ERRNO_NOTEXIST;
    rc = 0;
    goto LEAVE;
  }

 if(desc->cache!=0)
   mfree(cdfs_desc_table[desc_num].cache);

  desc->opentype=0;
  desc->cache=NULL;
  desc->cur_pos=0;
  desc->status=0;

  rc = 0;
  resultcode = 0;

LEAVE:
  res.close.res.h.size = sizeof(struct cdfs_res_close);
  res.close.res.h.service = cmd->close.req.h.service;
  res.close.res.h.command = cmd->close.req.h.command;
  res.close.res.seq = cmd->close.req.seq;
  res.close.res.resultcode = resultcode;
  message_send(cmd->close.req.h.arg, &res);

  return rc;
}

int cdfs_cmd_read(union cdfs_msg *cmd)
{
  int desc_num = cmd->read.req.handle;
  union cdfs_msg res;
  int rc;
  int readbyte;

  struct cdfs_descriptor *desc;
  rc = cdfs_get_descriptor(desc_num,&desc);
  if(rc<0) {
    if(rc==ERRNO_MODE)
      rc = ERRNO_READ;
    readbyte = rc;
    rc = 0;
    goto LEAVE;
  }
  if(desc->opentype!=CDFS_DESC_OPEN) {
    readbyte = ERRNO_MODE;
    rc = 0;
    goto LEAVE;
  }

  if(cmd->read.req.pos >= desc->description.size_le) {
    readbyte = ERRNO_SEEK;
    rc = 0;
    goto LEAVE;
  }

  unsigned long lba = desc->description.extent_le + (cmd->read.req.pos/CDFS_SECTOR_SIZE);
  int count = 1;
  int resultcode;

  rc = ata_read(
          cdfs_seqnum++,
          desc->device->devid,
          lba,
          count,
          cmd->read.req.shmname,
          cmd->read.req.offset,
          cmd->read.req.bufsize,
          &resultcode);
  if(rc<0) {
    readbyte = rc;
    goto LEAVE;
  }

  if(resultcode<0) {
    if(resultcode==ERRNO_SCSI_UnitAttention)
      cdfs_disable_pvdcache(desc->device);
    readbyte = resultcode;
    rc = 0;
    goto LEAVE;
  }

  unsigned long leftsize = desc->description.size_le - ((cmd->read.req.pos)-(cmd->read.req.pos%CDFS_SECTOR_SIZE));
  if(leftsize < CDFS_SECTOR_SIZE)
    readbyte = leftsize;
  else
    readbyte = CDFS_SECTOR_SIZE;

  rc = 0;

LEAVE:
  res.read.res.h.size = sizeof(struct cdfs_res_read);
  res.read.res.h.service = cmd->read.req.h.service;
  res.read.res.h.command = cmd->read.req.h.command;
  res.read.res.seq = cmd->read.req.seq;
  res.read.res.readbyte = readbyte;
  message_send(cmd->read.req.h.arg, &res);

  return rc;
}

int cdfs_cmd_readdir(union cdfs_msg *cmd)
{
  int desc_num = cmd->readdir.req.handle;
  union cdfs_msg res;
  int rc;
  unsigned long size;

  struct cdfs_descriptor *desc;
  rc = cdfs_get_descriptor(desc_num,&desc);
  if(rc<0) {
    if(rc==ERRNO_MODE)
      rc = ERRNO_READ;
    size = sizeof(struct cdfs_res_readdir);
    res.readdir.res.info.size = (unsigned long)-1;
    res.readdir.res.info.blksize = (unsigned long)rc;
    rc = 0;
    goto LEAVE;
  }
  if(desc->opentype!=CDFS_DESC_DIROPEN) {
    size = sizeof(struct cdfs_res_readdir);
    res.readdir.res.info.size = (unsigned long)-1;
    res.readdir.res.info.blksize = (unsigned long)ERRNO_MODE;
    rc = 0;
    goto LEAVE;
  }

  struct iso_directory_record *entry;
  entry = cdfs_nextentry(desc->cache, desc->description.size_le, &(desc->cur_pos));
  if(entry==NULL) {
    size = sizeof(struct cdfs_res_readdir);
    res.readdir.res.info.size = (unsigned long)-1;
    res.readdir.res.info.blksize = (unsigned long)ERRNO_OVER;
    rc = 0;
    goto LEAVE;
  }

//display_puts("(size=");
//int2dec(desc->description.size_le,s);
//display_puts(s);
//display_puts(")");
//display_puts("(pos=");
//int2dec(desc->cur_pos,s);
//display_puts(s);
//display_puts(")");
//directory_disp_entry(entry);

  res.readdir.res.info.size = entry->size_le;
  res.readdir.res.info.blksize = CDFS_SECTOR_SIZE;
  res.readdir.res.info.attr = entry->flags;
  memcpy(res.readdir.res.info.atime,entry->date,sizeof(res.readdir.res.info.atime));
  memcpy(res.readdir.res.info.mtime,entry->date,sizeof(res.readdir.res.info.mtime));
  if(entry->name[0]==0 && entry->name_len==1) {
    res.readdir.res.info.namelen = 2;
    memcpy(res.readdir.res.info.name, ".", 2);
    size = sizeof(struct cdfs_res_readdir) + 2;
  }
  else if(entry->name[0]==1 && entry->name_len==1) {
    res.readdir.res.info.namelen = 3;
    memcpy(res.readdir.res.info.name, "..", 3);
    size = sizeof(struct cdfs_res_readdir) + 3;
  }
  else {
    res.readdir.res.info.namelen = cdfs_filename_length(entry->name,entry->name_len);
    memcpy(res.readdir.res.info.name,entry->name,res.readdir.res.info.namelen);
    res.readdir.res.info.name[res.readdir.res.info.namelen] = 0;
    size = sizeof(struct cdfs_res_readdir) + res.readdir.res.info.namelen + 1;
  }

  rc = 0;

LEAVE:
  res.h.size = size;
  res.h.service = cmd->h.service;
  res.h.command = cmd->h.command;
  res.readdir.res.seq = cmd->readdir.req.seq;
  message_send(cmd->h.arg, &res);

  return rc;
}

int cdfs_cmd_stat(union cdfs_msg *cmd)
{
  int desc_num = cmd->stat.req.handle;
  union cdfs_msg res;
  int rc;
  unsigned long size;

  struct cdfs_descriptor *desc;
  rc = cdfs_get_descriptor(desc_num,&desc);
  if(rc<0) {
    if(rc==ERRNO_MODE)
      rc = ERRNO_READ;
    size = sizeof(struct cdfs_res_readdir);
    res.readdir.res.info.size = (unsigned long)-1;
    res.readdir.res.info.blksize = (unsigned long)rc;
    rc = 0;
    goto LEAVE;
  }

  res.readdir.res.info.size = desc->description.size_le;
  res.readdir.res.info.blksize = CDFS_SECTOR_SIZE;
  res.readdir.res.info.attr = desc->description.flags;
  memcpy(res.readdir.res.info.atime, desc->description.date, sizeof(res.readdir.res.info.atime));
  memcpy(res.readdir.res.info.mtime, desc->description.date, sizeof(res.readdir.res.info.mtime));
  res.readdir.res.info.namelen = 0;
  size = sizeof(struct cdfs_res_readdir);

  rc = 0;

LEAVE:
  res.h.size = size;
  res.h.service = cmd->h.service;
  res.h.command = cmd->h.command;
  res.stat.res.seq = cmd->stat.req.seq;
  message_send(cmd->h.arg, &res);

  return rc;
}

int cdfs_cmd_handler(union cdfs_msg *cmd)
{
  int rc;
  switch(cmd->h.command) {
  case CDFS_CMD_OPEN:
    rc=cdfs_cmd_open(cmd);
    break;
  case CDFS_CMD_CLOSE:
    rc=cdfs_cmd_close(cmd);
    break;
  case CDFS_CMD_READ:
    rc=cdfs_cmd_read(cmd);
    break;
  case CDFS_CMD_READDIR:
    rc=cdfs_cmd_readdir(cmd);
    break;
  case CDFS_CMD_STAT:
    rc=cdfs_cmd_stat(cmd);
    break;
  default:
    display_puts("unknown cmd error=");
    int2dec(cmd->h.command,s);
    display_puts(s);
    display_puts("\n");
    return ERRNO_CTRLBLOCK;
  }
  return rc;
}

int cdfs_init(void)
{
  unsigned int shmname=BLOCK_SHM_ID;
  int rc;

  cdfs_block_desc = block_init(shmname);
  if(cdfs_block_desc==0)
    return ERRNO_NOTINIT;

  cdfs_block_offset = block_alloc(cdfs_block_desc);
  if(cdfs_block_offset==0)
    return ERRNO_NOTINIT;

  cdfs_block_buf = block_addr(cdfs_block_desc,cdfs_block_offset);

  rc = message_set_quename(CDFS_QNM_CDFS);
  if(rc<0) {
    display_puts("set_quename error=");
    sint2dec(rc,s);
    display_puts(s);
    display_puts("\n");
    return rc;
  }

  return 0;
}

int cdfs_getdevice(struct cdfs_device *device)
{
  int rc;
  unsigned short devid=0;
  int typecode;

  for(devid=0;devid<4;devid++) {
    rc=ata_getinfo(cdfs_seqnum++,devid,&typecode);
    if(rc<0) {
      display_puts("rc=");
      sint2dec(rc,s);
      display_puts(s);
      break;
    }

    if(typecode==ATA_DEVTYPE_CD) {
      device->devid=devid;
      device->shmname=BLOCK_SHM_ID;
      device->offset=cdfs_block_offset;
      device->buf=cdfs_block_buf;
      display_puts("cdfsd:Found a CD-ROM Drive on the device ");
      int2dec(devid,s);
      display_puts("\n");
      return 0;
    }
  }
  return ERRNO_NOTEXIST;
}

int start(int ac, char *av[])
{
  int rc;
  union cdfs_msg cmd;

  memset(&cdfs_device_table,0,sizeof(cdfs_device_table));
  memset(&cdfs_desc_table,0,sizeof(cdfs_desc_table));

  struct cdfs_device *device=&cdfs_device_table;

  rc=cdfs_init();
  if(rc<0) {
    return 255;
  }

  if(cdfs_getdevice(device)<0) {
    display_puts("cdfsd:Device Not Found\n");
    return 0;
  }

//  rc=cdfs_iso_get_volume(device);
//  if(rc) {
//    display_puts("cdfsd:Bad Format\n");
//    return 0;
//  }

  for(;;) {
    cmd.h.size=sizeof(cmd);
    rc=message_receive(MESSAGE_MODE_WAIT, CDFS_SRV_CDFS, 0, &cmd);
    if(rc<0) {
      display_puts("cmd receive error=");
      int2dec(-rc,s);
      display_puts(s);
      display_puts("\n");
      return 255;
    }
//display_puts("(rcvcmd)");

    rc=cdfs_cmd_handler(&cmd);

    if(rc<0) {
      display_puts("*** cdfs driver terminate ***\n");
      return -rc;
    }

  }

  return 0;
}
