/*
** iplinfo.h --- fdd information
*/
#ifndef IPLINFO_H
#define IPLINFO_H


enum {
  FDD_BOOTDRV = 0,   /* default is A-drive */
  FDD_SECSIZE = 512, /* 1sector is 512 bytes */
  FDD_SECPTRK = 18,  /* 1track is 18 sectors */
  FDD_TRKSIZE = FDD_SECPTRK*FDD_SECSIZE, /* bytes of 1 track */
  FDD_SECPCLS = 1,   // change from 4
  FDD_FAT1HD  = 1,
  FDD_FAT2HD  = 10,
  FDD_FATCNT  = 3072,
  FDD_ROOTHD  = 19,
  FDD_DIRCNT  = 224, // change from 110
  FDD_DATAHD  = 33,
  FDD_SETUPHD = 33,  /* second boot start sector */
  FDD_SECCNT  = 2880,
};

struct fdd_directory {
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

#define FDD_ATTR_READ_ONLY	0x01
#define FDD_ATTR_HIDDEN		0x02
#define FDD_ATTR_SYSTEM		0x04
#define FDD_ATTR_VOLUME_ID	0x08
#define FDD_ATTR_DIRECTORY	0x10
#define FDD_ATTR_ARCHIVE	0x20
#define FDD_ATTR_BOOTFILE	(FDD_ATTR_READ_ONLY+FDD_ATTR_HIDDEN+FDD_ATTR_SYSTEM)

struct ipl_info {
  /* FDD IPL information */
  unsigned long setup_head;
  unsigned long setup_count;
  unsigned long kernel_head;
  unsigned long kernel_count;
  unsigned long kernel_size;
};


#endif /* IPLINFO_H */
