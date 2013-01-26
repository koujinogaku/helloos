#include "ata.h"
#include "syscall.h"
#include "errno.h"
#include "string.h"
#include "display.h"
#include "environm.h"
#include "message.h"
#include "config.h"

static unsigned short ata_queid=0;
static char s[10];

static char *atapi_sense_key_message[16] = {
  "No Sense.",         // 0h
  "Recovered Error.",  // 1h
  "Not Ready.",        // 2h
  "Medium Error.",     // 3h
  "Hardware Error.",   // 4h
  "Illegal Request.",  // 5h
  "Unit Attention.",   // 6h
  "Data Protect.",     // 7h
  "Blank Check.",      // 8h
  "Vendor Specific.",  // 9h
  "Copy Aborted.",     // Ah
  "Aborted Command.",  // Bh
  "Equal.",            // Ch
  "Volume Overflow.",  // Dh
  "Miscompare.",       // Eh
  "Reserved.",         // Fh
};

int ata_is_sensekey(int errno)
{
  int sensekey;
  sensekey = (-errno) + ERRNO_SCSI_NoSense;
display_puts("(calc=");
sint2dec(sensekey,s);
display_puts(s);
display_puts(")");
  if(sensekey<0 || sensekey>15)
    return 0;
  return 1;
}

int ata_errno2sensekey(int errno)
{
  int sensekey;
  sensekey = (-errno) + ERRNO_SCSI_NoSense;
  if(sensekey<0 || sensekey>15)
    return 0;
  return sensekey;
}

char *ata_sense_key_message(int num)
{
  if(num<0 || num>=16)
    return NULL;
  return atapi_sense_key_message[num];
}

int ata_init(void)
{
  int r;

  if(ata_queid != 0)
    return 0;

  for(;;) {
    r = syscall_que_lookup(ATA_QNM_ATA);
    if(r==ERRNO_NOTEXIST) {
      syscall_wait(10);
      continue;
    }
    if(r<0) {
      ata_queid = 0;
      display_puts("Cata_init srvq=");
      long2hex(-r,s);
      display_puts(s);
      display_puts("\n");
      return r;
    }
    ata_queid = r;
    break;
  }
/*
  display_puts("Ckbdqids q=");
  int2dec(kbd_queid,s);
  display_puts(s);
  display_puts(",");
  int2dec(kbd_clientq,s);
  display_puts(s);
  display_puts("\n");
*/
  return 0;
}

int ata_getinfo_request(unsigned short seq,unsigned short devid)
{
  int rc;
  union ata_msg msg;

  if(ata_queid==0) {
    rc=ata_init();
    if(rc<0)
      return rc;
  }

  msg.h.size=sizeof(union ata_msg_getinfo);
  msg.h.service=ATA_SRV_ATA;
  msg.h.command=ATA_CMD_GETINFO;
  msg.h.arg=environment_getqueid();
  msg.getinfo.req.seq=seq;
  msg.getinfo.req.devid=devid;
/*
int2dec(msg.req.queid,s);
display_puts(s);
*/
  rc=message_send(ata_queid, &msg);
  if(rc<0) {	
    display_puts("getcode sndcmd=");
    int2dec(-rc,s);
    display_puts(s);
    display_puts("\n");
    return rc;
  }

  return 0;
}

int ata_getinfo_response(unsigned short *seq, int *typecode)
{
  union ata_msg msg;
  int rc;

  msg.h.size=sizeof(msg);
  rc=message_receive(0,ATA_SRV_ATA, ATA_CMD_GETINFO, &msg);
  if(rc<0) {
    display_puts("getcode getresp=");
    int2dec(-rc,s);
    display_puts(s);
    display_puts("\n");
    return rc;
  }
  *seq = msg.getinfo.res.seq;
  *typecode = msg.getinfo.res.type;

  return rc;
}

int ata_getinfo(unsigned short seq, unsigned short devid, int *typecode)
{
  int rc;
  unsigned short res_seq;

  rc=ata_getinfo_request(seq,devid);
  if(rc<0)
    return rc;
  rc=ata_getinfo_response(&res_seq, typecode);
  if(rc<0)
    return rc;
  if(seq!=res_seq)
    return ERRNO_CTRLBLOCK;
  return rc;
}

int ata_read_request(unsigned short seq,unsigned short devid,unsigned long lba,int count,unsigned int shmname,unsigned long offset,unsigned long bufsize)
{
  int rc;
  union ata_msg msg;

  if(ata_queid==0) {
    rc=ata_init();
    if(rc<0)
      return rc;
  }

  msg.h.size=sizeof(union ata_msg_read);
  msg.h.service=ATA_SRV_ATA;
  msg.h.command=ATA_CMD_READ;
  msg.h.arg=environment_getqueid();
  msg.read.req.seq=seq;
  msg.read.req.devid=devid;
  msg.read.req.lba=lba;
  msg.read.req.count=count;
  msg.read.req.shmname=shmname;
  msg.read.req.offset=offset;
  msg.read.req.bufsize=bufsize;
/*
int2dec(msg.req.queid,s);
display_puts(s);
*/
  rc=message_send(ata_queid, &msg);
  if(rc<0) {	
    display_puts("getcode sndcmd=");
    int2dec(-rc,s);
    display_puts(s);
    display_puts("\n");
    return rc;
  }

  return 0;
}

int ata_read_response(unsigned short *seq, int *resultcode)
{
  union ata_msg msg;
  int rc;

  msg.h.size=sizeof(msg);
  rc=message_receive(0,ATA_SRV_ATA, ATA_CMD_READ, &msg);
  if(rc<0) {
    display_puts("getcode getresp=");
    int2dec(-rc,s);
    display_puts(s);
    display_puts("\n");
    return rc;
  }
  *seq = msg.read.res.seq;
  *resultcode = msg.read.res.resultcode;

  return rc;
}

int ata_read(unsigned short seq,unsigned short devid,unsigned long lba,int count,unsigned int shmname,unsigned long offset,unsigned long bufsize,int *resultcode)
{
  int rc;
  unsigned short res_seq;

  rc=ata_read_request(seq,devid,lba,count,shmname,offset,bufsize);
  if(rc<0)
    return rc;
  rc=ata_read_response(&res_seq, resultcode);
  if(rc<0)
    return rc;
  if(seq!=res_seq)
    return ERRNO_CTRLBLOCK;
  return rc;
}



int ata_checkmedia_request(unsigned short seq,unsigned short devid)
{
  int rc;
  union ata_msg msg;

  if(ata_queid==0) {
    rc=ata_init();
    if(rc<0)
      return rc;
  }

  msg.h.size=sizeof(union ata_msg_read);
  msg.h.service=ATA_SRV_ATA;
  msg.h.command=ATA_CMD_CHECKMEDIA;
  msg.h.arg=environment_getqueid();
  msg.checkmedia.req.seq=seq;
  msg.checkmedia.req.devid=devid;
/*
int2dec(msg.req.queid,s);
display_puts(s);
*/
  rc=message_send(ata_queid, &msg);
  if(rc<0) {	
    display_puts("checkmedia sndcmd=");
    int2dec(-rc,s);
    display_puts(s);
    display_puts("\n");
    return rc;
  }

  return 0;
}

int ata_checkmedia_response(unsigned short *seq, int *resultcode)
{
  union ata_msg msg;
  int rc;

  msg.h.size=sizeof(msg);
  rc=message_receive(0,ATA_SRV_ATA, ATA_CMD_CHECKMEDIA, &msg);
  if(rc<0) {
    display_puts("checkmedia getresp=");
    int2dec(-rc,s);
    display_puts(s);
    display_puts("\n");
    return rc;
  }
  *seq = msg.checkmedia.res.seq;
  *resultcode = msg.checkmedia.res.resultcode;

  return rc;
}

int ata_checkmedia(unsigned short seq,unsigned short devid,int *resultcode)
{
  int rc;
  unsigned short res_seq;

  rc=ata_checkmedia_request(seq,devid);
  if(rc<0)
    return rc;
  rc=ata_checkmedia_response(&res_seq, resultcode);
  if(rc<0)
    return rc;
  if(seq!=res_seq)
    return ERRNO_CTRLBLOCK;
  return rc;
}
