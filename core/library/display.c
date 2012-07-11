#include "syscall.h"
#include "errno.h"
#include "string.h"
#include "display.h"
#include "environm.h"
#include "message.h"

static unsigned short display_queid=0;
static char s[10];

int display_init(void)
{
  int r;

  if(display_queid != 0)
    return 0;

  for(;;) {
    r = syscall_que_lookup(DISPLAY_QNM_DISPLAY);
    if(r==ERRNO_NOTEXIST) {
      syscall_wait(10);
      continue;
    }
    if(r<0) {
      syscall_puts("dsp_init srvq err=");
      long2hex(-r,s);
      syscall_puts(s);
      syscall_puts("\n");
      return r;
    }
    display_queid = r;
    break;
  }
/*
  syscall_puts("dspqids q=");
  int2dec(display_queid,s);
  syscall_puts(s);
  syscall_puts(",");
  int2dec(display_clientq,s);
  syscall_puts(s);
  syscall_puts("\n");
*/
  return 0;
}

int display_putc(int chr)
{
  struct msg_head msg;
  int r;

  if(display_queid==0) {
    r=display_init();
    if(r<0)
      return r;
  }

  msg.size=sizeof(struct msg_head);
  msg.service=DISPLAY_SRV_DISPLAY;
  msg.command=DISPLAY_CMD_PUTC;
  msg.arg=chr;
  for(;;) {
    r=message_send(display_queid, &msg);
    if(r!=ERRNO_OVER)
      break;
    syscall_wait(10);
  }
  if(r<0) {
    syscall_puts("putc sndcmd=");
    long2hex(-r,s);
    syscall_puts(s);
    syscall_puts("\n");
    return r;
  }

  return 0;
}

int display_puts(char *chr)
{
  union display_msg msg;
  short len;
  int r;

  if(display_queid==0) {
    r=display_init();
    if(r<0)
      return r;
  }
/*
if(display_queid<100) {
  syscall_puts(" qid=");
  int2dec(display_queid,s);
  syscall_puts(s);
  syscall_puts("\n");
 }
*/
  len=strlen(chr);
  if(len>=DISPLAY_STRLEN-1)
    len=DISPLAY_STRLEN-1;

//  msg.h.size=msg_size(sizeof(struct msg_head)+len+1);
  msg.h.size=sizeof(struct msg_head)+len+1;
  msg.h.service=DISPLAY_SRV_DISPLAY;
  msg.h.command=DISPLAY_CMD_PUTS;
  msg.h.arg=len;
  strncpy(msg.s.s, chr, DISPLAY_STRLEN);
/*
{
char *ms=(char *)&msg;
syscall_puts("sz=");
int2dec(msg.h.size,s);
syscall_puts(s);
syscall_puts("arg=");
int2dec(msg.h.arg,s);
syscall_puts(s);
syscall_puts("str=");
byte2hex(ms[8],&s[0]);
byte2hex(ms[9],&s[2]);
syscall_puts(s);
}
*/
  for(;;) {
    r=message_send(display_queid, &msg);
    if(r!=ERRNO_OVER)
      break;
    syscall_wait(10);
  }
  if(r<0) {
    syscall_puts("puts sndcmd=");
    int2dec(-r,s);
    syscall_puts(s);
    syscall_puts(" qid=");
    int2dec(display_queid,s);
    syscall_puts(s);
    syscall_puts("\n");
    return r;
  }

  return 0;
}

int display_setattr(int attr)
{
  struct msg_head msg;
  int r;

  if(display_queid==0) {
    r=display_init();
    if(r<0)
      return r;
  }

  msg.size=sizeof(struct msg_head);
  msg.service=DISPLAY_SRV_DISPLAY;
  msg.command=DISPLAY_CMD_SETATTR;
  msg.arg=attr;
  for(;;) {
    r=message_send(display_queid, &msg);
    if(r!=ERRNO_OVER)
      break;
    syscall_wait(10);
  }
  if(r<0) {
    syscall_puts("setattr sndcmd=");
    long2hex(-r,s);
    syscall_puts(s);
    syscall_puts("\n");
    return r;
  }

  return 0;
}
