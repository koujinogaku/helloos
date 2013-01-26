#ifndef ATA_H
#define ATA_H
#include "syscall.h"
#include "kmessage.h"
#include "msgquenames.h"

#define ATA_QNM_ATA        MSGQUENAMES_ATA
#define ATA_SRV_ATA        ATA_QNM_ATA
#define ATA_CMD_GETINFO    0x0001
#define ATA_CMD_READ       0x0002
#define ATA_CMD_CHECKMEDIA 0x0003

#define ATA_DEVTYPE_TAPE     1
#define ATA_DEVTYPE_HDD      2
#define ATA_DEVTYPE_CD       5

union ata_msg {
  struct msg_head h;
  union ata_msg_getinfo {
    struct ata_req_getinfo {
      struct msg_head h;
      unsigned short seq;
      unsigned short devid;
    } req;
    struct ata_res_getinfo {
      struct msg_head h;
      unsigned short seq;
      int type;
    } res;
  } getinfo;
  union ata_msg_read {
    struct ata_req_read {
      struct msg_head h;
      unsigned short seq;
      unsigned short devid;
      unsigned long lba;
      int count;
      unsigned int shmname;
      unsigned long offset;
      unsigned long bufsize;
    } req;
    struct ata_res_read {
      struct msg_head h;
      unsigned short seq;
      short resultcode;
      int readbyte;
    } res;
  } read;
  union ata_msg_checkmedia {
    struct ata_req_checkmedia {
      struct msg_head h;
      unsigned short seq;
      unsigned short devid;
    } req;
    struct ata_res_checkmedia {
      struct msg_head h;
      unsigned short seq;
      short resultcode;
    } res;
  } checkmedia;
};


int ata_init(void);
char *ata_sense_key_message(int num);
int ata_errno2sensekey(int errno);
int ata_is_sensekey(int errno);
int ata_getinfo(unsigned short seq, unsigned short devid, int *typecode);
int ata_getinfo_response(unsigned short *seq, int *typecode);
int ata_getinfo_request(unsigned short seq,unsigned short devid);
int ata_read(unsigned short seq,unsigned short devid,unsigned long lba,int count,unsigned int shmname,unsigned long offset,unsigned long bufsize,int *resultcode);
int ata_read_request(unsigned short seq,unsigned short devid,unsigned long lba,int count,unsigned int shmname,unsigned long offset,unsigned long bufsize);
int ata_read_response(unsigned short *seq, int *resultcode);
int ata_checkmedia(unsigned short seq,unsigned short devid, int *resultcode);
int ata_checkmedia_request(unsigned short seq,unsigned short devid);
int ata_checkmedia_response(unsigned short *seq, int *resultcode);

#endif
