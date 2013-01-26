#include "cdfs.h"
#include "syscall.h"
#include "errno.h"
#include "string.h"
#include "display.h"
#include "environm.h"
#include "message.h"

static unsigned short cdfs_queid=0;
static unsigned short cdfs_seq=0;
static unsigned short cdfs_devid=0;
static char s[10];

int cdfs_init(void)
{
  int rc;

  if(cdfs_queid != 0)
    return 0;

  for(;;) {
    rc = syscall_que_lookup(CDFS_QNM_CDFS);
    if(rc==ERRNO_NOTEXIST) {
      syscall_wait(10);
      continue;
    }
    if(rc<0) {
      cdfs_queid = 0;
      display_puts("cdfs_init srvq=");
      long2hex(-rc,s);
      display_puts(s);
      display_puts("\n");
      return rc;
    }
    cdfs_queid = rc;
    break;
  }


//display_puts("cdfs_init done\n");

  return 0;
}

int cdfs_open_request(unsigned short seq,unsigned short devid,char *fullpath)
{
  int rc;
  union cdfs_msg msg;

  if(cdfs_queid==0) {
    rc=cdfs_init();
    if(rc<0)
      return rc;
  }

  msg.h.service=CDFS_SRV_CDFS;
  msg.h.command=CDFS_CMD_OPEN;
  msg.h.arg=environment_getqueid();
  msg.open.req.seq=seq;
  msg.open.req.devid=devid;
  strncpy(msg.open.req.fullpath,fullpath,CDFS_FULLPATH_MAXLENGTH);
  msg.open.req.fullpath[CDFS_FULLPATH_MAXLENGTH-1]=0;

  msg.h.size=(unsigned long)(&msg.open.req.fullpath[0])-(unsigned long)(&msg)+strlen(msg.open.req.fullpath)+1;

/*
int2dec(msg.req.queid,s);
display_puts(s);
*/
  rc=message_send(cdfs_queid, &msg);
  if(rc<0) {	
    display_puts("cdfs open sndcmd=");
    int2dec(-rc,s);
    display_puts(s);
    display_puts("\n");
    return rc;
  }

  return 0;
}

int cdfs_open_response(unsigned short *seq, int *handle)
{
  union cdfs_msg msg;
  int rc;

  if(cdfs_queid==0) {
    rc=cdfs_init();
    if(rc<0)
      return rc;
  }

  msg.h.size=sizeof(msg);
  rc=message_receive(MESSAGE_MODE_WAIT,CDFS_SRV_CDFS, CDFS_CMD_OPEN, &msg);
  if(rc<0) {
    display_puts("cdfs open=");
    int2dec(-rc,s);
    display_puts(s);
    display_puts("\n");
    return rc;
  }
  *seq = msg.open.res.seq;
  *handle = msg.open.res.handle;

  return rc;
}

int cdfs_open(char *fullpath)
{
  int rc;
  unsigned short res_seq;
  int handle;

  cdfs_seq++;

  rc=cdfs_open_request(cdfs_seq,cdfs_devid,fullpath);
  if(rc<0)
    return rc;
  rc=cdfs_open_response(&res_seq, &handle);
  if(rc<0)
    return rc;
  if(cdfs_seq!=res_seq)
    return ERRNO_CTRLBLOCK;
  return handle;
}


int cdfs_close_request(unsigned short seq,int handle)
{
  int rc;
  union cdfs_msg msg;

  if(cdfs_queid==0) {
    rc=cdfs_init();
    if(rc<0)
      return rc;
  }

  msg.h.size=sizeof(struct cdfs_req_close);
  msg.h.service=CDFS_SRV_CDFS;
  msg.h.command=CDFS_CMD_CLOSE;
  msg.h.arg=environment_getqueid();
  msg.close.req.seq=seq;
  msg.close.req.handle=handle;

/*
int2dec(msg.req.queid,s);
display_puts(s);
*/
  rc=message_send(cdfs_queid, &msg);
  if(rc<0) {	
    display_puts("cdfs open sndcmd=");
    int2dec(-rc,s);
    display_puts(s);
    display_puts("\n");
    return rc;
  }

  return rc;
}

int cdfs_close_response(unsigned short *seq, int *resultcode)
{
  union cdfs_msg msg;
  int rc;

  if(cdfs_queid==0) {
    rc=cdfs_init();
    if(rc<0)
      return rc;
  }

  msg.h.size=sizeof(msg);
  rc=message_receive(MESSAGE_MODE_WAIT,CDFS_SRV_CDFS, CDFS_CMD_CLOSE, &msg);
  if(rc<0) {
    display_puts("cdfs open=");
    int2dec(-rc,s);
    display_puts(s);
    display_puts("\n");
    return rc;
  }
  *seq = msg.open.res.seq;
  *resultcode = msg.close.res.resultcode;

  return rc;
}

int cdfs_close(int handle)
{
  int rc;
  unsigned short res_seq;
  int resultcode;

  cdfs_seq++;

  rc=cdfs_close_request(cdfs_seq,handle);
  if(rc<0)
    return rc;
  rc=cdfs_close_response(&res_seq, &resultcode);
  if(rc<0)
    return rc;
  if(cdfs_seq!=res_seq)
    return ERRNO_CTRLBLOCK;
  return handle;
}


int cdfs_read_request(unsigned short seq,int handle,unsigned long pos,unsigned int shmname,unsigned long offset,unsigned long bufsize)
{
  int rc;
  union cdfs_msg msg;

  if(cdfs_queid==0) {
    rc=cdfs_init();
    if(rc<0)
      return rc;
  }

  msg.h.size=sizeof(struct cdfs_req_read);
  msg.h.service=CDFS_SRV_CDFS;
  msg.h.command=CDFS_CMD_READ;
  msg.h.arg=environment_getqueid();
  msg.read.req.seq=seq;
  msg.read.req.handle=handle;
  msg.read.req.pos=pos;
  msg.read.req.shmname=shmname;
  msg.read.req.offset=offset;
  msg.read.req.bufsize=bufsize;
/*
int2dec(msg.req.queid,s);
display_puts(s);
*/
  rc=message_send(cdfs_queid, &msg);
  if(rc<0) {	
    display_puts("cdfs_read sndcmd=");
    int2dec(-rc,s);
    display_puts(s);
    display_puts("\n");
    return rc;
  }

  return 0;
}

int cdfs_read_response(unsigned short *seq, int *readbyte)
{
  union cdfs_msg msg;
  int rc;

  if(cdfs_queid==0) {
    rc=cdfs_init();
    if(rc<0)
      return rc;
  }

  msg.h.size=sizeof(msg);
  rc=message_receive(MESSAGE_MODE_WAIT,CDFS_SRV_CDFS, CDFS_CMD_READ, &msg);
  if(rc<0) {
    display_puts("getcode getresp=");
    int2dec(-rc,s);
    display_puts(s);
    display_puts("\n");
    return rc;
  }
  *seq = msg.read.res.seq;
  *readbyte = msg.read.res.readbyte;

  return rc;
}

int cdfs_read(int handle,unsigned long pos,unsigned int shmname,unsigned long offset,unsigned long bufsize)
{
  int rc;
  unsigned short res_seq;
  int readbyte;

  cdfs_seq++;

  rc=cdfs_read_request(cdfs_seq,handle,pos,shmname,offset,bufsize);
  if(rc<0)
    return rc;
  rc=cdfs_read_response(&res_seq, &readbyte);
  if(rc<0)
    return rc;
  if(cdfs_seq!=res_seq)
    return ERRNO_CTRLBLOCK;

  return readbyte;
}

int cdfs_readdir_request(unsigned short seq,int handle)
{
  int rc;
  union cdfs_msg msg;

  if(cdfs_queid==0) {
    rc=cdfs_init();
    if(rc<0)
      return rc;
  }

  msg.h.size=sizeof(struct cdfs_req_readdir);
  msg.h.service=CDFS_SRV_CDFS;
  msg.h.command=CDFS_CMD_READDIR;
  msg.h.arg=environment_getqueid();
  msg.readdir.req.seq=seq;
  msg.readdir.req.handle=handle;
/*
int2dec(msg.req.queid,s);
display_puts(s);
*/
  rc=message_send(cdfs_queid, &msg);
  if(rc<0) {	
    display_puts("cdfs_read sndcmd=");
    int2dec(-rc,s);
    display_puts(s);
    display_puts("\n");
    return rc;
  }

  return 0;
}

int cdfs_readdir_response(unsigned short *seq, struct file_info *info, unsigned long bufsize)
{
  union cdfs_msg msg;
  int rc;

  if(cdfs_queid==0) {
    rc=cdfs_init();
    if(rc<0)
      return rc;
  }

  msg.h.size=sizeof(msg);
  rc=message_receive(MESSAGE_MODE_WAIT,CDFS_SRV_CDFS, CDFS_CMD_READDIR, &msg);
  if(rc<0) {
    display_puts("getcode getresp=");
    int2dec(-rc,s);
    display_puts(s);
    display_puts("\n");
    return rc;
  }
  *seq = msg.readdir.res.seq;
  unsigned long len = sizeof(struct file_info);
  if(len>bufsize)
    len = bufsize;
  memcpy(info, &msg.readdir.res.info, len);
  if((int)(msg.readdir.res.info.size) == -1)
    return 0;
  if(bufsize < sizeof(struct file_info))
    return 0;
  len = msg.readdir.res.info.namelen+1;
  if(len>bufsize-sizeof(struct file_info))
    len = bufsize-sizeof(struct file_info);
  memcpy(info->name, &msg.readdir.res.info.name, len);

  return 0;
}

int cdfs_readdir(int handle,struct file_info *info,unsigned long bufsize)
{
  int rc;
  unsigned short res_seq;

  cdfs_seq++;

  rc=cdfs_readdir_request(cdfs_seq,handle);
  if(rc<0)
    return rc;
  rc=cdfs_readdir_response(&res_seq, info, bufsize);
  if(rc<0)
    return rc;
  if(cdfs_seq!=res_seq)
    return ERRNO_CTRLBLOCK;

  if((int)(info->size)==-1)
    return info->blksize;

  return 0;
}

int cdfs_stat_request(unsigned short seq,int handle)
{
  int rc;
  union cdfs_msg msg;

  if(cdfs_queid==0) {
    rc=cdfs_init();
    if(rc<0)
      return rc;
  }

  msg.h.size=sizeof(struct cdfs_req_readdir);
  msg.h.service=CDFS_SRV_CDFS;
  msg.h.command=CDFS_CMD_STAT;
  msg.h.arg=environment_getqueid();
  msg.stat.req.seq=seq;
  msg.stat.req.handle=handle;
/*
int2dec(msg.req.queid,s);
display_puts(s);
*/
  rc=message_send(cdfs_queid, &msg);
  if(rc<0) {	
    display_puts("cdfs_stat sndcmd=");
    int2dec(-rc,s);
    display_puts(s);
    display_puts("\n");
    return rc;
  }

  return 0;
}

int cdfs_stat_response(unsigned short *seq, struct file_info *info, unsigned long bufsize)
{
  union cdfs_msg msg;
  int rc;

  if(cdfs_queid==0) {
    rc=cdfs_init();
    if(rc<0)
      return rc;
  }

  msg.h.size=sizeof(msg);
  rc=message_receive(MESSAGE_MODE_WAIT,CDFS_SRV_CDFS, CDFS_CMD_STAT, &msg);
  if(rc<0) {
    display_puts("getcode getresp=");
    int2dec(-rc,s);
    display_puts(s);
    display_puts("\n");
    return rc;
  }
  *seq = msg.stat.res.seq;
  unsigned long len = sizeof(struct file_info);
  if(len>bufsize)
    len = bufsize;
  memcpy(info, &msg.stat.res.info, len);
  if((int)(msg.stat.res.info.size) == -1)
    return 0;
  if(bufsize < sizeof(struct file_info))
    return 0;

  return 0;
}

int cdfs_stat(int handle,struct file_info *info,unsigned long bufsize)
{
  int rc;
  unsigned short res_seq;

  cdfs_seq++;

  rc=cdfs_stat_request(cdfs_seq,handle);
  if(rc<0)
    return rc;
  rc=cdfs_stat_response(&res_seq, info, bufsize);
  if(rc<0)
    return rc;
  if(cdfs_seq!=res_seq)
    return ERRNO_CTRLBLOCK;

  if((int)(info->size)==-1)
    return info->blksize;

  return 0;
}

int cdfs_getinfo_request(unsigned short seq,unsigned short devid)
{
  int rc;
  union cdfs_msg msg;

  if(cdfs_queid==0) {
    rc=cdfs_init();
    if(rc<0)
      return rc;
  }

  msg.h.size=sizeof(struct cdfs_req_getinfo);
  msg.h.service=CDFS_SRV_CDFS;
  msg.h.command=CDFS_CMD_GETINFO;
  msg.h.arg=environment_getqueid();
  msg.getinfo.req.seq=seq;
  msg.getinfo.req.devid=devid;
/*
int2dec(msg.req.queid,s);
display_puts(s);
*/
  rc=message_send(cdfs_queid, &msg);
  if(rc<0) {	
    display_puts("getcode sndcmd=");
    int2dec(-rc,s);
    display_puts(s);
    display_puts("\n");
    return rc;
  }

  return 0;
}

int cdfs_getinfo_response(unsigned short *seq, int *typecode)
{
  union cdfs_msg msg;
  int rc;

  if(cdfs_queid==0) {
    rc=cdfs_init();
    if(rc<0)
      return rc;
  }

  msg.h.size=sizeof(msg);
  rc=message_receive(MESSAGE_MODE_WAIT,CDFS_SRV_CDFS, CDFS_CMD_GETINFO, &msg);
  if(rc<0) {
    display_puts("getcode getresp=");
    int2dec(-rc,s);
    display_puts(s);
    display_puts("\n");
    return rc;
  }
  *seq = msg.getinfo.res.seq;
  *typecode = msg.getinfo.res.type;

  return 0;
}

int cdfs_getinfo(unsigned short seq, unsigned short devid, int *typecode)
{
  int rc;
  unsigned short res_seq;

  cdfs_seq++;

  rc=cdfs_getinfo_request(cdfs_seq,devid);
  if(rc<0)
    return rc;
  rc=cdfs_getinfo_response(&res_seq, typecode);
  if(rc<0)
    return rc;
  if(cdfs_seq!=res_seq)
    return ERRNO_CTRLBLOCK;
  return rc;
}
