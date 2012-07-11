#include "mouse.h"
#include "syscall.h"
#include "errno.h"
#include "string.h"
#include "display.h"
#include "environm.h"
#include "message.h"

static unsigned short mou_queid=0;
static unsigned short mou_clientq=0;
static unsigned char  mou_requested=0;
static char s[10];

int mouse_init(void)
{
  int r;
  int retry=100;

  if(mou_queid != 0)
    return 0;

  for(;;) {
    r = syscall_que_lookup(MOU_QNM_MOUSE);
    if(r==ERRNO_NOTEXIST) {
      syscall_wait(10);
      if(retry--)
        continue;
      return -1;
    }
    if(r<0) {
      mou_queid = 0;
      display_puts("Cmou_init srvq=");
      long2hex(-r,s);
      display_puts(s);
      display_puts("\n");
      return r;
    }
    mou_queid = r;
    break;
  }
  mou_clientq=environment_getqueid();
/*
  display_puts("Ckbdqids q=");
  int2dec(mou_queid,s);
  display_puts(s);
  display_puts(",");
  int2dec(mou_clientq,s);
  display_puts(s);
  display_puts("\n");
*/
  return 0;
}

int mouse_request_code(union mou_msg *msg)
{
  int r;

  if(mou_queid==0) {
    r=mouse_init();
    if(r<0)
      return r;
  }

  msg->req.h.size=sizeof(union mou_msg);
  msg->req.h.service=MOU_SRV_MOUSE;
  msg->req.h.command=MOU_CMD_GETCODE;
  msg->req.queid=mou_clientq;
/*
int2dec(msg.req.queid,s);
display_puts(s);
*/
  r=message_send(mou_queid, msg);
  if(r<0) {	
    display_puts("getcode sndcmd=");
    long2hex(-r,s);
    display_puts(s);
    display_puts("\n");
    return r;
  }

  return 0;
}

int mouse_decode_code(union mou_msg *msg, int *button, int *dx, int *dy)
{
  *button=msg->res.h.arg;
  *dx=msg->res.dx;
  *dy=msg->res.dy;
  return 0;
}

int mouse_getcode(int *button, int *dx, int *dy)
{
  union mou_msg msg;
  int r;

  if(!mou_requested) {
    r=mouse_request_code(&msg);
    if(r<0)
      return r;
    mou_requested=1;
  }
  msg.req.h.size=sizeof(msg);
  r=message_receive(0,MOU_SRV_MOUSE, MOU_CMD_GETCODE, &msg);
  mou_requested=0;
  if(r<0) {
    display_puts("getcode getresp=");
    int2dec(-r,s);
    display_puts(s);
    display_puts("\n");
    return r;
  }

  *button=msg.res.h.arg;
  *dx=msg.res.dx;
  *dy=msg.res.dy;
  return 0;
}

int mouse_poll(void)
{
  union mou_msg msg;
  int r;

  if(!mou_requested) {
    r=mouse_request_code(&msg);
    if(r<0)
      return r;
    mou_requested=1;
  }
  msg.req.h.size=sizeof(msg);
  r=message_poll(MOU_SRV_MOUSE, MOU_CMD_GETCODE, &msg);
  if(r<0) {
    display_puts("mousepoll getresp=");
    int2dec(-r,s);
    display_puts(s);
    display_puts("\n");
    return r;
  }

  return r;
}
