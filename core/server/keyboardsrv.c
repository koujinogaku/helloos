#include "cpu.h"
#include "pic.h"
#include "display.h"
#include "keyboard.h"
#include "syscall.h"
#include "string.h"
#include "errno.h"
#include "environm.h"
#include "message.h"

#define KBD_PORT_DATA		0x0060

static int kbd_queid=0;

static struct msg_list *kbd_bufferq =NULL;
static struct msg_list *kbd_requestq=NULL;

static int kbd_has_request=0;

static char s[16];

int keyboard_initsrv(void)
{
  int r;

  kbd_queid = environment_getqueid();
  if(kbd_queid==0) {
    syscall_puts("kbd_init que get error");
    syscall_puts("\n");
    return ERRNO_NOTINIT;
  }

  r = syscall_que_setname(kbd_queid, KBD_QNM_KEYBOARD);
  if(r<0) {
    kbd_queid = 0;
    display_puts("kbd_init msgq=");
    int2dec(-r,s);
    display_puts(s);
    display_puts("\n");
    return r;
  }

  kbd_bufferq=message_create_userqueue();
  if(kbd_bufferq==NULL) {
    display_puts("kbd_init create userqueue\n");
    return ERRNO_RESOURCE;
  }
  kbd_requestq=message_create_userqueue();
  if(kbd_requestq==NULL) {
    display_puts("kbd_init create userqueue\n");
    return ERRNO_RESOURCE;
  }

  r=syscall_intr_regist(PIC_IRQ_KEYBRD, kbd_queid);
  if(r<0) {
    kbd_queid = 0;
    display_puts("kbd_init intrreg=");
    int2dec(-r,s);
    display_puts(s);
    display_puts("\n");
    return r;
  }

  pic_enable(PIC_IRQ_KEYBRD);	/* enable keyboard */
/*
  display_puts("queids=");
    int2dec(kbd_bufferq,s);
    display_puts(s);
    display_puts(",");
    int2dec(kbd_requestq,s);
    display_puts(s);
    display_puts(",");
    int2dec(kbd_queid,s);
    display_puts(s);
    display_puts("\n");
*/

  return 0;
}

static short keyboard_table[256] = {
//  0     1     2     3     4     5     6     7
     0, KESC,  '1',  '2',  '3',  '4',  '5',  '6',
   '7',  '8',  '9',  '0',  '-',  '=',  KBS, KTAB,   //0f
   'q',  'w',  'e',  'r',  't',  'y',  'u',  'i',
   'o',  'p',  '[',  ']', KRET, KCTL,  'a',  's',   //1f
   'd',  'f',  'g',  'h',  'j',  'k',  'l',  ';',
  '\'',  '`', KSFT, '\\',  'z',  'x',  'c',  'v',   //2f
   'b',  'n',  'm',  ',',  '.',  '/', KSFT,  '*',
  KALT,  ' ', KCAP,  KF1,  KF2,  KF3,  KF4,  KF5,   //3f
   KF6,  KF7,  KF8,  KF9, KF10, KNUM, KSCR, KHOM,
    KU, KPGU,  '-',   KL,    0,   KR,  '+', KEND,   //4f
    KD, KPGD, KINS, KDEL, KSRQ,    0,    0, KF11,
  KF12,    0,    0,    0,    0,    0,    0,    0,   //5f
     0,    0,    0,    0,    0,    0,    0,    0,
     0,    0,    0,    0,    0,    0,    0,    0,   //6f
     0,    0,    0,    0,    0,    0,    0,    0,
     0,    0,    0,    0,    0,    0,    0,    0,   //7f
//  0     1     2     3     4     5     6     7
     0, KESC,  '!',  '@',  '#',  '$',  '%',  '^',
   '&',  '*',  '(',  ')',  '_',  '+',  KBS, KTAB,   //8f
   'Q',  'W',  'E',  'R',  'T',  'Y',  'U',  'I',
   'O',  'P',  '{',  '}', KRET, KCTL,  'A',  'S',   //9f
   'D',  'F',  'G',  'H',  'J',  'K',  'L',  ':',
   '"',  '~', KSFT,  '|',  'Z',  'X',  'C',  'V',   //af
   'B',  'N',  'M',  '<',  '>',  '?', KSFT,  '*',
  KALT,  ' ', KCAP,  KF1,  KF2,  KF3,  KF4,  KF5,   //bf
   KF6,  KF7,  KF8,  KF9, KF10, KNUM, KSCR, KHOM,
    KU, KPGU,  '-',   KL,    0,   KR,  '+', KEND,   //cf
    KD, KPGD, KINS, KDEL, KSRQ,    0,    0, KF11,
  KF12,    0,    0,    0,    0,    0,    0,    0,   //df
     0,    0,    0,    0,    0,    0,    0,    0,
     0,    0,    0,    0,    0,    0,    0,    0,   //ef
     0,    0,    0,    0,    0,    0,    0,    0,
     0,    0,    0,    0,    0,    0,    0,    0,   //ff
};

static int keyboard_shift=0;

short keyboard_convert(unsigned char keycode)
{
  int keyup;
  int key;

  keyup = keycode & 0x80;
  keycode = keycode & 0x7f;
  key = keyboard_table[keycode+keyboard_shift];
  switch (key) {
  case KSFT:
    if(keyup) keyboard_shift = 0;
    else      keyboard_shift = 0x80;
    break;
  }
  if(keyup || key > 0x100)
    return 0;
  else
    return key;
}

int keyboard_intr(union kbd_msg *msg)
{
  unsigned char data;
  int r;
  union kbd_msg key;

  if(msg->req.h.arg!=PIC_IRQ_KEYBRD) {
    display_puts("intr budirq=");
    long2hex(msg->req.h.command,s);
    display_puts(s);
    display_puts(":");
    long2hex(msg->req.h.arg,s);
    display_puts(s);
    display_puts("\n");
    return ERRNO_CTRLBLOCK;
  }
  data = cpu_in8(KBD_PORT_DATA);

//data=irq;
//display_putc('.');

  key.res.key = keyboard_convert(data);

/*
display_puts("[");
word2hex(key.res.key,s);
display_puts(s);
display_puts("]");
*/
//display_putc(key);

  if(key.res.key>0) {
    key.res.h.size=sizeof(key);
    key.res.h.service=KBD_SRV_KEYBOARD;
    key.res.h.command=KBD_CMD_GETCODE;
    r=message_put_userqueue(kbd_bufferq, &key);
    if(r<0) {
      display_puts("kbd_intr=");
      int2dec(-r,s);
      display_puts(s);
      display_puts("\n");
      return r;
    }
  }
/*
{
  static char intcnt=0;
  byte2hex(intcnt,s);
  byte2hex(data,&s[2]);
  intcnt++;
  syscall_dbg_locputs(75,2,s);
}
*/
  return 0;
}

int keyboard_cmd_getcode(union kbd_msg *msg)
{
  int r;

  if(kbd_has_request==0) {
/*
display_puts("setfirst");
int2dec(msg->req.queid,s);
display_puts(s);
*/
    kbd_has_request=msg->req.queid;
  }
  else {
    r=message_put_userqueue(kbd_requestq, msg);
    if(r<0) {
      display_puts("getcode putreq=");
      int2dec(-r,s);
      display_puts(s);
      display_puts("\n");
      return r;
    }
  }
  return 0;
}


int keyboard_response(void)
{
  int r;
  union kbd_msg msg;

  if(kbd_has_request==0)
    return 0;
/*
display_puts("resin");
*/
  for(;;) {
    msg.res.h.size=sizeof(msg);
    r=message_get_userqueue(kbd_bufferq, &msg);
    if(r==ERRNO_OVER)
      return 0;
    if(r<0) {
      display_puts("resp getkey=");
      int2dec(-r,s);
      display_puts(s);
      display_puts("\n");
      return r;
    }
/*
display_puts("snd=");
word2hex(msg.res.h.cmd,s);
display_puts(s);
*/
    r=syscall_que_put(kbd_has_request, &msg);
    if(r<0) {
      display_puts("resp sndkey=");
      int2dec(-r,s);
      display_puts(s);
      display_puts(" qid=");
      int2dec(kbd_has_request,s);
      display_puts(s);
      display_puts("\n");
      return r;
    }
    msg.req.h.size=sizeof(msg);
    r=message_get_userqueue(kbd_requestq, &msg);
    if(r==ERRNO_OVER) {
      kbd_has_request=0;
      return 0;
    }
    if(r<0) {
      display_puts("resp getreq=");
      int2dec(-r,s);
      display_puts(s);
      display_puts("\n");
      return r;
    }
    kbd_has_request=msg.req.queid;
  }
  return 0;
}

int start(void)
{
  int r;
  union kbd_msg cmd;

  r=keyboard_initsrv();
  if(r<0) {
    return 255;
  }

  for(;;)
  {
    cmd.req.h.size=sizeof(cmd);
    r=syscall_que_get(kbd_queid,&cmd);
    if(r<0) {
      display_puts("cmd receive error=");
      int2dec(-r,s);
      display_puts(s);
      display_puts("\n");
      return 255;
    }
/*
display_puts("cmd=");
word2hex(cmd.req.h.cmd,s);
display_puts(s);
*/
    switch(cmd.req.h.service<<16 | cmd.req.h.command) {
    case MSG_SRV_KERNEL<<16 | MSG_CMD_KRN_INTERRUPT:
//    display_puts("intr!");
      r=keyboard_intr(&cmd);
      break;
    case KBD_SRV_KEYBOARD<<16 | KBD_CMD_GETCODE:
//    display_puts("get!");
      r=keyboard_cmd_getcode(&cmd);
      break;
    default:
      display_puts("cmd number error srv=");
      word2hex(cmd.req.h.service,s);
      display_puts(s);
      display_puts(" sz=");
      word2hex(cmd.req.h.size,s);
      display_puts(s);
      display_puts(" cmd=");
      long2hex(cmd.req.h.command,s);
      display_puts(s);
      display_puts(" arg=");
      long2hex(cmd.req.h.arg,s);
      display_puts(s);
      display_puts("\n");
      r=-1;
      break;
    }
//    display_puts("end!");    
    if(r>=0 && kbd_has_request!=0)
      r=keyboard_response();

    if(r<0) {
      display_puts("*** keyboard terminate ***\n");
      return 255;
    }
  }
  return 0;
}
