#include "keyboard.h"
#include "syscall.h"
#include "errno.h"
#include "string.h"
#include "display.h"
#include "environm.h"
#include "message.h"

static unsigned short kbd_queid=0;
static char s[10];
static unsigned char  keyboard_requested=0;
static unsigned char  keyboard_received=0;
static union kbd_msg  keyboard_received_message;

int keyboard_init(void)
{
  int r;

  if(kbd_queid != 0)
    return 0;

  for(;;) {
    r = syscall_que_lookup(KBD_QNM_KEYBOARD);
    if(r==ERRNO_NOTEXIST) {
      syscall_wait(10);
      continue;
    }
    if(r<0) {
      kbd_queid = 0;
      display_puts("Ckbd_init srvq=");
      long2hex(-r,s);
      display_puts(s);
      display_puts("\n");
      return r;
    }
    kbd_queid = r;
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

int keyboard_request_key()
{
  int r;
  union kbd_msg msg;

  if(kbd_queid==0) {
    r=keyboard_init();
    if(r<0)
      return r;
  }

  if(keyboard_requested)
    return 0;

  msg.req.h.size=sizeof(union kbd_msg);
  msg.req.h.service=KBD_SRV_KEYBOARD;
  msg.req.h.command=KBD_CMD_GETCODE;
  msg.req.queid=environment_getqueid();
/*
int2dec(msg.req.queid,s);
display_puts(s);
*/
  r=message_send(kbd_queid, &msg);
  if(r<0) {	
    display_puts("getcode sndcmd=");
    int2dec(-r,s);
    display_puts(s);
    display_puts("\n");
    return r;
  }

  keyboard_requested=1;
  return 0;
}

int keyboard_decode_key(void *msg_v)
{
  union kbd_msg *msg=msg_v;

  keyboard_requested=0;
  return msg->res.key;
}

int keyboard_setmsg(void *msg_v)
{
  keyboard_received=1;
  memcpy(&keyboard_received_message, msg_v, sizeof(keyboard_received_message));
  return 0;
}

int keyboard_getcode(void)
{
  union kbd_msg msg;
  int r;


  if(keyboard_received) {
    keyboard_received=0;
    return msg.res.key;
  }

  r=keyboard_request_key();
  if(r<0)
    return r;

  msg.req.h.size=sizeof(msg);
  r=message_receive(0,KBD_SRV_KEYBOARD, KBD_CMD_GETCODE, &msg);
  keyboard_requested=0;
  if(r<0) {
    display_puts("getcode getresp=");
    int2dec(-r,s);
    display_puts(s);
    display_puts("\n");
    return r;
  }

  return msg.res.key;
}

int keyboard_poll(void)
{
  union kbd_msg msg;
  int r;

  r=keyboard_request_key();
  if(r<0)
    return r;

  msg.req.h.size=sizeof(msg);
  r=message_poll(MESSAGE_MODE_TRY,KBD_SRV_KEYBOARD, KBD_CMD_GETCODE, &msg);
  if(r<0) {
    display_puts("keypoll getresp=");
    int2dec(-r,s);
    display_puts(s);
    display_puts("\n");
    return r;
  }

  return r;
}
