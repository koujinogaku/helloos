#include "config.h"
#include "list.h"
#include "kmemory.h"
#include "string.h"
#include "console.h"
#include "floppy.h"
#include "fat.h"
#include "errno.h"

struct fat_boot_sector {
	char jumpcode[3];
	char systemname[8];
	byte byte_per_sector[2];
	byte sector_per_cluster[1];
	byte reserve_sector[2];
	byte fat_copy[1];
	byte directory_entry_count[2];
	byte total_sector[2];
	byte media_descripter[1];
	byte sector_per_fat[2];
	byte sector_per_track[2];
	byte head_count[2];
	byte hidden_sectors[4];
	byte total_sectors32MB[4];
	byte drive_number[1];
	byte reserve[1];
	byte extend_boot_signeture[1];
	byte volume_serial[4];
	char volume_label[11];
	char fat_type[8];
};

struct fat_directory {
  unsigned char filename[8];
  unsigned char fileext[3];
  unsigned char attr;
  unsigned char reserve1;
  unsigned char timeten;
  unsigned short time;
  unsigned short date;
  unsigned short accdate;
  unsigned short clusterHi;
  unsigned short updatetime;
  unsigned short updatedate;
  unsigned short clusterLo;
  unsigned long filesize;
};

#define FAT_ATTR_READ_ONLY	0x01
#define FAT_ATTR_HIDDEN		0x02
#define FAT_ATTR_SYSTEM		0x04
#define FAT_ATTR_VOLUME_ID	0x08
#define FAT_ATTR_DIRECTORY	0x10
#define FAT_ATTR_ARCHIVE	0x20
#define FAT_ATTR_BOOTFILE	(FAT_ATTR_READ_ONLY|FAT_ATTR_HIDDEN|FAT_ATTR_SYSTEM)

#define FAT_STAT_NOTUSE  0
#define FAT_STAT_INUSE   1

struct fat_filedesc
{
  struct fat_filedesc *next;
  struct fat_filedesc *prev;
  unsigned int status;
  unsigned int mode;
  unsigned int dirpnt;
  unsigned int start_cluster;
  unsigned int current_pointer;
  unsigned int current_cluster;
  unsigned int filesize;
  byte drive;
};
struct fat_dirdesc
{
  char status;
  byte drive;
  unsigned short dirpnt;
};

#define FAT_FILEDESCCNT 64
struct fat_filedesc  *fat_filedesclist=NULL;

struct fat_volume_info
{
  unsigned long byte_per_sector;
  unsigned long sector_per_cluster;
  unsigned long sector_per_track;
  unsigned long total_sector;
  unsigned long head_count;
  unsigned long fat1sec;
  unsigned long fat1sec_cnt;
  unsigned long fat2sec;
  unsigned long fat2sec_cnt;
  unsigned long rootdirsec;
  unsigned long rootdirsec_cnt;
  unsigned long datasec;
  word16 *fat16;
  unsigned long fat16_cnt;
  byte *fat12;
  unsigned long fat12_cnt;
  struct fat_directory *rootdir;
  unsigned long rootdir_cnt;
  byte volume_serial;
  byte drive;
  byte status;
  char volume_label[11];
  char fat_type[8];
  char *tmpbuf;
  int tmpbufclstno;
};

static struct fat_volume_info *fat_volumelist=NULL;
#define FAT_VOLUMELISTCNT 4 // ‚Æ‚è‚ ‚¦‚¸4‚Â

//static char s[10];

//static char tmpbuf[1024];
// static char s[64];

static byte fat_current_drive=0;

static byte
fat_gen_byte(unsigned char *c)
{
  return *c;
}
static word16
fat_gen_word16(unsigned char *c)
{
  return c[1]*256 + c[0];
}
static word32
fat_gen_word32(unsigned char *c)
{
  return c[3]*16777216 + c[2]*65536 + c[1]*256 + c[0];
}
/*
static void fat_16to12(unsigned int fat16_cnt,word16 fat16[], byte fat12[])
{
  int i=0,j=0;
  for(;i<fat16_cnt;i+=2,j+=3) {
    fat12[j]   = fat16[i] & 0xff;
    fat12[j+1] = (fat16[i] & 0xf00)/256 + (fat16[i+1] & 0xf)*16;
    fat12[j+2] = (fat16[i+1] & 0xff0)/16;
  }
}
*/
static void fat_12to16(unsigned int fat16_cnt,byte fat12[], word16 fat16[])
{
  int i=0,j=0;
  for(;i<fat16_cnt;i+=2,j+=3) {
    fat16[i]   = fat12[j] | (fat12[j+1]&0xf)*256;
    fat16[i+1] = (fat12[j+1]&0xf0)/16 | fat12[j+2]*16;
  }
}


int
fat_init(void)
{
  byte *infobuf;

  infobuf = mem_alloc(sizeof(struct fat_volume_info)*FAT_VOLUMELISTCNT +
                      sizeof(struct fat_filedesc)*FAT_FILEDESCCNT       );
  if(infobuf==0)
    return ERRNO_RESOURCE;
/*
{char s[10];
console_puts("fattbls size=");
int2dec(sizeof(struct fat_volume_info)*FAT_VOLUMELISTCNT +
        sizeof(struct fat_filedesc)*FAT_FILEDESCCNT        ,s);
console_puts(s);
console_puts("\n");
}
*/
  memset(infobuf,0,sizeof(struct fat_volume_info)*FAT_VOLUMELISTCNT +
                   sizeof(struct fat_filedesc)*FAT_FILEDESCCNT        );

  fat_volumelist = (void*)infobuf;
  infobuf += sizeof(struct fat_volume_info)*FAT_VOLUMELISTCNT;
  fat_filedesclist = (void*)infobuf;

  list_init(fat_filedesclist);

  return 0;
}

int
fat_open(byte drive)
{
  byte *infobuf;
  struct fat_boot_sector *bootsec;
  struct fat_volume_info *info;
  int r;
  char bootbuf[FLOPPY_BOOTSECSZ];
//  int i;

  if(fat_volumelist == NULL)
    return ERRNO_NOTINIT;
  if(drive >= FLOPPY_NUM_DRIVE)
    return ERRNO_NOTEXIST;
  if(fat_volumelist[drive].status == FAT_STAT_INUSE)
    return ERRNO_INUSE;

  if((r=floppy_open(drive))<0)
    return r;

  if((r=floppy_read_sector(drive, bootbuf, 0, 1))<0)
    return r;
/*
  console_puts("boot sector=");
  for(i=0;i<16;i++) {
    byte2hex(tmpbuf[i],s);
    console_puts(s);
    console_putc(' ');
  }
  console_puts("\n");
*/

  bootsec = (void*)bootbuf;
  info = &fat_volumelist[drive];

  info->status = FAT_STAT_INUSE;
  info->drive = drive;
  info->byte_per_sector    =fat_gen_word16(bootsec->byte_per_sector);
  info->sector_per_cluster =fat_gen_byte(bootsec->sector_per_cluster);
  info->sector_per_track   =fat_gen_word16(bootsec->sector_per_track);
  info->total_sector       =fat_gen_word16(bootsec->total_sector);
  info->head_count         =fat_gen_word16(bootsec->head_count);
  info->fat1sec            =fat_gen_word16(bootsec->reserve_sector);
  info->fat1sec_cnt        =fat_gen_word16(bootsec->sector_per_fat);
  info->fat2sec            =info->fat1sec + info->fat1sec_cnt;
  info->fat2sec_cnt        =info->fat1sec_cnt;

  info->fat12_cnt   = info->fat1sec_cnt * info->byte_per_sector;
  info->fat16_cnt   = info->fat12_cnt * 2 / 3; // FAT12
  info->rootdir_cnt = fat_gen_word16(bootsec->directory_entry_count);

  info->rootdirsec         =info->fat2sec + info->fat2sec_cnt;
  info->rootdirsec_cnt     =info->rootdir_cnt * 32 / 512 ; // FAT12
  info->volume_serial      =fat_gen_word32(bootsec->volume_serial);
  info->datasec            =info->rootdirsec + info->rootdirsec_cnt;
  memcpy(info->volume_label,bootsec->volume_label, 11);
  memcpy(info->fat_type,    bootsec->fat_type, 8);

/*
  console_puts("byte per sec=");
  int2str(info->byte_per_sector,s);
  console_puts(s);
  console_puts("\n");

  console_puts("sector_per_cluster=");
  int2str(info->sector_per_cluster,s);
  console_puts(s);
  console_puts("\n");

  console_puts("sector_per_track=");
  int2str(info->sector_per_track,s);
  console_puts(s);
  console_puts("\n");

  console_puts("total_sector=");
  int2str(info->total_sector,s);
  console_puts(s);
  console_puts("\n");

  console_puts("head_count=");
  int2str(info->head_count,s);
  console_puts(s);
  console_puts("\n");

  console_puts("fat1sec=");
  int2str(info->fat1sec,s);
  console_puts(s);
  console_puts("\n");

  console_puts("fat1sec_cnt=");
  int2str(info->fat1sec_cnt,s);
  console_puts(s);
  console_puts("\n");

  console_puts("fat12_cnt=");
  int2str(info->fat12_cnt,s);
  console_puts(s);
  console_puts("\n");

  console_puts("fat16_cnt=");
  int2str(info->fat16_cnt,s);
  console_puts(s);
  console_puts("\n");

  console_puts("rootdir_cnt=");
  int2str(info->rootdir_cnt,s);
  console_puts(s);
  console_puts("\n");

  console_puts("rootdirsec=");
  int2str(info->rootdirsec,s);
  console_puts(s);
  console_puts("\n");

  console_puts("rootdirsec_cnt=");
  int2str(info->rootdirsec_cnt,s);
  console_puts(s);
  console_puts("\n");

  console_puts("volume_serial=");
  int2str(info->volume_serial,s);
  console_puts(s);
  console_puts("\n");

  console_puts("volume_label=");
  memcpy(s, info->volume_label,11);
  s[11]=0;
  console_puts(s);
  console_puts("\n");

  console_puts("fat_type=");
  memcpy(s, info->fat_type,8);
  s[8]=0;
  console_puts(s);
  console_puts("\n");
*/


  infobuf=mem_alloc(sizeof(word16)*(info->fat16_cnt) +
                    sizeof(byte)*(info->fat12_cnt) +
                    sizeof(struct fat_directory)*(info->rootdir_cnt) +
                    (info->byte_per_sector)*(info->sector_per_cluster)
                   );
  if(infobuf==0)
    return ERRNO_RESOURCE;
/*
{char s[10];
console_puts("drvinfotbls size=");
int2dec(sizeof(word16)*(info->fat16_cnt) +
        sizeof(byte)*(info->fat12_cnt) +
        sizeof(struct fat_directory)*(info->rootdir_cnt) ,s);
console_puts(s);
console_puts("\n");
}
*/
  memset(infobuf,0, sizeof(word16)*(info->fat16_cnt) + 
                    sizeof(byte)*(info->fat12_cnt) + 
                    sizeof(struct fat_directory)*(info->rootdir_cnt) );

  info->fat16=(void*)infobuf;
  infobuf += sizeof(word16)*(info->fat16_cnt);
  info->fat12=(void*)infobuf;
  infobuf += sizeof(byte)*(info->fat12_cnt);
  info->rootdir=(void*)infobuf;
  infobuf += sizeof(struct fat_directory)*(info->rootdir_cnt);
  info->tmpbuf=(void*)infobuf;
  info->tmpbufclstno=0;

  if((r=floppy_read_sector(info->drive, info->fat12, info->fat1sec, info->fat1sec_cnt))<0)
    return r;

  fat_12to16(info->fat16_cnt, info->fat12, info->fat16);
/*
  console_puts("fat=");
  for(i=0;i<32;i++) {
    word2hex(info->fat16[i],s);
    console_puts(s);
    console_putc(' ');
  }
  console_puts("\n");
*/

  if((r=floppy_read_sector(info->drive, info->rootdir, info->rootdirsec, info->rootdirsec_cnt))<0)
    return r;
/*
  for(i=0;i<8;i++) {
    if(info->rootdir[i].filename[0]==0)
      break;
    memcpy(s, info->rootdir[i].filename,11);
    s[11]=0;
    console_puts(s);
    console_puts(" sz=");
    int2str(info->rootdir[i].filesize,s);
    console_puts(s);
    console_puts(" cl=");
    word2hex(info->rootdir[i].clusterLo,s);
    console_puts(s);
    console_puts("\n");
  }
*/
  return 0;
}

int fat_conv_name2dirent(char *filename, byte *drive, char *dirent)
{
  char c;
  char *np=filename;
  int i,ext;

  c=uppercase(filename[0]);
  if(c>='A' && c<='Z' && filename[1]==':') {
    *drive=c-'A';
    *np += 2;
  }
  else {
    *drive=fat_current_drive;
  }
  ext=0;
  i=0;
  while(i<11 && *np!=0) {
    c=uppercase(*np);
    if(c==':'||c=='\\') {
      return ERRNO_NOTEXIST;
    }
    else if(c=='.') {
      if(ext) {
        return ERRNO_NOTEXIST;
      }
      else {
        if(i==0) { return ERRNO_NOTEXIST; }
        else     { ext=1; np++; }
      }
    }
    else {
      if(ext) {
        if(i<8) { dirent[i]=' '; i++; }
        else    { dirent[i]=uppercase(*np); i++; np++; }
      }
      else {
        if(i<8) { dirent[i]=uppercase(*np); i++; np++; }
        else    { np++; }
      }
    }
  }
  while(i<11) {
    dirent[i]=' '; i++;
  }
  dirent[i]=0;

  return 0;
}

int fat_conv_dirent2name(char *dirent,char *filename)
{
  char *np=filename;
  int i;
  for(i=0;i<11;i++) {
    if(dirent[i]<' ')
      break;
    if(i==8) {
      *np='.';
      np++;
    }
    if(dirent[i]!=' ') {
      *np=dirent[i];
      np++;
    }
  }
  *np=0;
  return 0;
}

int
fat_open_file(char *filename, int mode)
{
  struct fat_volume_info *info;
  int fp;
  int dirpnt;
  byte drive;
  char dirent[12];
  int r;

  if(fat_volumelist == NULL)
    return ERRNO_NOTINIT;
  if(fat_filedesclist == NULL)
    return ERRNO_NOTINIT;
  if(mode!=FAT_O_RDONLY && mode!=FAT_O_RDWR)
    return ERRNO_MODE;

  r=fat_conv_name2dirent(filename, &drive, dirent);
  if(r<0) {
    return r;
  }

  if(drive >= FLOPPY_NUM_DRIVE)
    return ERRNO_NOTEXIST;
  info=&fat_volumelist[drive];
  if(info->status==FAT_STAT_NOTUSE) {
    r=fat_open(drive);
    if(r<0)
      return r;
  }

  for(dirpnt=0;dirpnt<info->rootdir_cnt;dirpnt++)
  {
    if(info->rootdir[dirpnt].filename[0]==0)
      continue;
    if(memcmp(info->rootdir[dirpnt].filename,dirent,11)==0)
      break;
  }
  if(dirpnt >= info->rootdir_cnt)
    return ERRNO_NOTEXIST;

  list_alloc(fat_filedesclist,FAT_FILEDESCCNT,fp);
  if(fp==0)
    return ERRNO_RESOURCE;
  memset(&fat_filedesclist[fp],0,sizeof(struct fat_filedesc));
  fat_filedesclist[fp].status=FAT_STAT_INUSE;
  fat_filedesclist[fp].mode=mode;
  fat_filedesclist[fp].dirpnt=dirpnt;
  fat_filedesclist[fp].start_cluster=info->rootdir[dirpnt].clusterLo + info->rootdir[dirpnt].clusterHi*65536;
  fat_filedesclist[fp].current_pointer=0;
  fat_filedesclist[fp].current_cluster=fat_filedesclist[fp].start_cluster;
  fat_filedesclist[fp].filesize=info->rootdir[dirpnt].filesize;
  fat_filedesclist[fp].drive=drive;

  return fp;
}

int
fat_close_file(int fp)
{
  if(fat_filedesclist==0)
    return ERRNO_NOTINIT;
  if(fp<0 || fp >= FAT_FILEDESCCNT)
    return ERRNO_NOTEXIST;

  if(fat_filedesclist[fp].status==FAT_STAT_NOTUSE)
    return ERRNO_NOTEXIST;
  memset(&fat_filedesclist[fp],0,sizeof(struct fat_filedesc));
  fat_filedesclist[fp].status=FAT_STAT_NOTUSE;
  return 0;
}

int
fat_get_filesize(int fp, unsigned int *size)
{
  if(fat_filedesclist==0)
    return ERRNO_NOTINIT;
  if(fp<0 || fp>=FAT_FILEDESCCNT)
    return ERRNO_NOTEXIST;

  if(fat_filedesclist[fp].status==FAT_STAT_NOTUSE)
    return ERRNO_NOTEXIST;

  *size = fat_filedesclist[fp].filesize;

  return 0;
}
static int fat_cluster2sector(struct fat_volume_info *info, int cluster, int offset)
{
  return ( info->sector_per_cluster * (cluster-2) + info->datasec ) +
         ( offset % info->byte_per_sector );
}

#if 0
int
fat_seek_file(int fp, unsigned int offset, int direction)
{
  unsigned int new_pointer,byte_per_cluster,cur_clst_ofs,current_cluster;
  struct fat_filedesc *fd;
  struct fat_volume_info *info;

  if(fat_filedesclist==0)
    return ERRNO_NOTINIT;
  if(fp<0 || fp>=FAT_FILEDESCCNT)
    return ERRNO_NOTEXIST;

  if(fat_filedesclist[fp].status==FAT_STAT_NOTUSE)
    return ERRNO_NOTEXIST;

  fd=&fat_filedesclist[fp];
  info=&fat_volumelist[fd->drive];

  if(direction<0)
    return ERRNO_MODE;

  if(direction==0) {
    fd->current_pointer = 0;
    fd->current_cluster = fd->start_cluster;
  }

  new_pointer = fd->current_pointer + offset;
  if(new_pointer > fd->filesize)
    return ERRNO_OVER;
  byte_per_cluster = info->byte_per_sector*info->sector_per_cluster;
  cur_clst_ofs = fd->current_pointer%byte_per_cluster;
  current_cluster = fd->current_cluster;
  for(;;) {
    if(cur_clst_ofs+offset < byte_per_cluster)
      break;
    current_cluster = info->fat16[current_cluster];
    if(current_cluster==0x0fff)
      return ERRNO_CTRLBLOCK;
    offset = offset - (byte_per_cluster - cur_clst_ofs);
    cur_clst_ofs = 0;
  }
  cur_clst_ofs = cur_clst_ofs + offset;
  fd->current_cluster = current_cluster;
  fd->current_pointer = new_pointer;
  return 0;
}
#endif
int
fat_read_file(int fp, void* buf, unsigned int bufsize)
{
  unsigned int byte_per_cluster,cur_clst_ofs,sectorcnt;
  struct fat_filedesc *fd;
  struct fat_volume_info *info;
  char *readbuf,* bufpnt;
  int r;
  unsigned int size;
  int sector;

  if(fat_filedesclist==0)
    return ERRNO_NOTINIT;
  if(fp<0 || fp>=FAT_FILEDESCCNT)
    return ERRNO_NOTEXIST;
  if(fat_filedesclist[fp].status==FAT_STAT_NOTUSE)
    return ERRNO_NOTEXIST;

  bufpnt=buf;

  fd=&fat_filedesclist[fp];
  info=&fat_volumelist[fd->drive];

  if(fd->current_pointer>=fd->filesize)
    return ERRNO_OVER;

  if((fd->filesize-fd->current_pointer) < bufsize)
    bufsize = fd->filesize-fd->current_pointer;
  size=bufsize;

  byte_per_cluster = info->byte_per_sector*info->sector_per_cluster;
  cur_clst_ofs = fd->current_pointer%byte_per_cluster;

  for(;size!=0;) {

    if(fd->current_cluster == 0xfff)
      return ERRNO_CTRLBLOCK;

    if(cur_clst_ofs==0 && size >= byte_per_cluster) {
      sector=fat_cluster2sector(info, fd->current_cluster, 0);
      sectorcnt=info->sector_per_cluster;
      if((r=floppy_read_sector(info->drive, bufpnt, sector, sectorcnt))<0)
        return r;
      bufpnt += byte_per_cluster;
      size -= byte_per_cluster;
      fd->current_pointer += byte_per_cluster;
      fd->current_cluster = info->fat16[fd->current_cluster];
    }
    else {
      sector=fat_cluster2sector(info, fd->current_cluster, 0);
      sectorcnt=info->sector_per_cluster;

      if(fd->current_cluster!=info->tmpbufclstno)
        if((r=floppy_read_sector(info->drive, info->tmpbuf, sector, sectorcnt))<0)
          return r;
      info->tmpbufclstno=fd->current_cluster;

      readbuf=(char*)((unsigned int)(info->tmpbuf)+(cur_clst_ofs%info->byte_per_sector));
      for(;size!=0;) {
        *bufpnt++ = *readbuf++;
        cur_clst_ofs++;
        size--;
        fd->current_pointer++;
        if(cur_clst_ofs>=byte_per_cluster) {
          cur_clst_ofs=0;
          fd->current_cluster = info->fat16[fd->current_cluster];
          break;
        }
      }
    }
  }

  return bufsize-size;
}

int fat_open_dir(unsigned long *vdirdesc, char *dirname)
{
  struct fat_dirdesc *dirdesc=(void*)vdirdesc;
  struct fat_volume_info *info;
  int r;
  byte drive;
  char dirent[12];

  r=fat_conv_name2dirent(dirname, &drive, dirent);
  if(r<0)
    return r;
  
  if(drive >= FLOPPY_NUM_DRIVE)
    return ERRNO_NOTEXIST;
  info=&fat_volumelist[drive];
  if(info->status==FAT_STAT_NOTUSE) {
    r=fat_open(drive);
    if(r<0)
      return r;
  }
  dirdesc->drive=drive;
  dirdesc->status=0;
  dirdesc->dirpnt=0;

  return 0;
}

int fat_read_dir(unsigned long *vdirdesc, char *dirent)
{
  struct fat_dirdesc *dirdesc=(void*)vdirdesc;
  struct fat_volume_info *info;
  byte drive;
  int dirpnt;

  drive=dirdesc->drive;

  if(drive >= FLOPPY_NUM_DRIVE)
    return ERRNO_NOTEXIST;
  info=&fat_volumelist[drive];
  if(info->status==FAT_STAT_NOTUSE) {
    return ERRNO_NOTEXIST;
  }
  if(dirdesc->dirpnt >= info->rootdir_cnt) {
    return ERRNO_OVER;
  }
  for(dirpnt=dirdesc->dirpnt;dirpnt<info->rootdir_cnt;dirpnt++)
  {
    if(info->rootdir[dirpnt].filename[0]==0)
      continue;
    fat_conv_dirent2name((char*)(info->rootdir[dirpnt].filename),dirent);
    dirpnt++;
    dirdesc->dirpnt=dirpnt;
    return 0;
  }
  return ERRNO_OVER;
}
int fat_close_dir(unsigned long *vdirdesc)
{
  return 0;
}
