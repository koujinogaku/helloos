#ifndef CDFS_H
#define CDFS_H
#include "syscall.h"
#include "kmessage.h"
#include "msgquenames.h"
#include "file.h"

#define CDFS_QNM_CDFS   MSGQUENAMES_CDFS
#define CDFS_SRV_CDFS   CDFS_QNM_CDFS
#define CDFS_CMD_GETINFO    0x0001
#define CDFS_CMD_OPEN       0x0002
#define CDFS_CMD_READ       0x0003
#define CDFS_CMD_WRITE      0x0004
#define CDFS_CMD_CLOSE      0x0005
#define CDFS_CMD_READDIR    0x0006
#define CDFS_CMD_STAT       0x0007

#define CDFS_FULLPATH_MAXLENGTH  256
#define CDFS_SECTOR_SIZE  2048

#define CDFS_ATTR_HIDDEN      0x01
#define CDFS_ATTR_DIRECTORY   0x02
#define CDFS_ATTR_ASSOCIATE   0x04
#define CDFS_ATTR_RECORD      0x08
#define CDFS_ATTR_PROTECTED   0x10
#define CDFS_ATTR_MULTIEXTENT 0x80


union cdfs_msg {
  struct msg_head h;
  union cdfs_msg_getinfo {
    struct cdfs_req_getinfo {
      struct msg_head h;
      unsigned short seq;
      unsigned short devid;
    } req;
    struct cdfs_res_getinfo {
      struct msg_head h;
      unsigned short seq;
      int type;
    } res;
  } getinfo;
  union cdfs_msg_open {
    struct cdfs_req_open {
      struct msg_head h;
      unsigned short seq;
      unsigned short devid;
      char fullpath[CDFS_FULLPATH_MAXLENGTH];
    } req;
    struct cdfs_res_open {
      struct msg_head h;
      unsigned short seq;
      short handle;
    } res;
  } open;
  union cdfs_msg_close {
    struct cdfs_req_close {
      struct msg_head h;
      unsigned short seq;
      short handle;
    } req;
    struct cdfs_res_close {
      struct msg_head h;
      unsigned short seq;
      short resultcode;
    } res;
  } close;
  union cdfs_msg_read {
    struct cdfs_req_read {
      struct msg_head h;
      unsigned short seq;
      short handle;
      unsigned long pos;
      unsigned int shmname;
      unsigned long offset;
      unsigned long bufsize;
    } req;
    struct cdfs_res_read {
      struct msg_head h;
      unsigned short seq;
      short readbyte;
    } res;
  } read;
  union cdfs_msg_readdir {
    struct cdfs_req_readdir {
      struct msg_head h;
      unsigned short seq;
      short handle;
    } req;
    struct cdfs_res_readdir {
      struct msg_head h;
      unsigned short seq;
      struct file_info info;
    } res;
  } readdir;
  union cdfs_msg_stat {
    struct cdfs_req_stat {
      struct msg_head h;
      unsigned short seq;
      short handle;
    } req;
    struct cdfs_res_stat {
      struct msg_head h;
      unsigned short seq;
      struct file_info info;
    } res;
  } stat;
};


int cdfs_init(void);
int cdfs_getinfo_request(unsigned short seq,unsigned short devid);
int cdfs_getinfo_response(unsigned short *seq, int *typecode);
int cdfs_getinfo(unsigned short seq, unsigned short devid, int *typecode);
int cdfs_open_request(unsigned short seq,unsigned short devid,char *fullpath);
int cdfs_open_response(unsigned short *seq, int *handle);
int cdfs_open(char *fullpath);
int cdfs_close_request(unsigned short seq,int handle);
int cdfs_close_response(unsigned short *seq, int *resultcode);
int cdfs_close(int handle);
int cdfs_read_request(unsigned short seq,int handle,unsigned long pos,unsigned int shmname,unsigned long offset,unsigned long bufsize);
int cdfs_read_response(unsigned short *seq, int *readbyte);
int cdfs_read(int handle,unsigned long pos,unsigned int shmname,unsigned long offset,unsigned long bufsize);
int cdfs_readdir_request(unsigned short seq,int handle);
int cdfs_readdir_response(unsigned short *seq, struct file_info *info, unsigned long bufsize);
int cdfs_readdir(int handle,struct file_info *info,unsigned long bufsize);
int cdfs_stat_request(unsigned short seq,int handle);
int cdfs_stat_response(unsigned short *seq, struct file_info *info, unsigned long bufsize);
int cdfs_stat(int handle,struct file_info *info,unsigned long bufsize);

#endif
