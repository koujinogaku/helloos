#include "cpu.h"
#include "pic.h"
#include "display.h"
#include "mouse.h"
#include "syscall.h"
#include "string.h"
#include "errno.h"
#include "environm.h"
#include "message.h"

#define MOUSE_PORT_DATA			0x60
#define MOUSE_PORT_STAT			0x64
#define MOUSE_PORT_CMD			0x64

#define MOUSE_PORTCMD_WRITE_REG		0x60
#define MOUSE_PORTCMD_READ_REG		0x20
#define MOUSE_PORTCMD_WRITE_MOUSEREG	0xd4
#define MOUSE_PORTDAT_MOU_ENABLE	0xf4

#define MOUSE_STAT_READ_READY		0x01
#define MOUSE_STAT_WRITE_NOTREADY	0x02
#define MOUSE_STAT_READ_OVERFLOW	0x20


static int mou_intrstat=0;
#define MOUSE_STAT_INTR_FIRST		1
#define MOUSE_STAT_INTR_SECOND		2
#define MOUSE_STAT_INTR_THIRD		3
static int mou_port_overflow=0;


static int mou_queid=0;
static int mou_has_request=0;
static struct msg_list *mou_bufferq=NULL;
static struct msg_list *mou_requestq=NULL;
static char s[16];

static int mouse_wait_writeReady(void)
{
  int timeout,stat;
  for(timeout=0;timeout<5000;++timeout)
  {
    stat=cpu_in8(MOUSE_PORT_STAT);
    if(!(stat & MOUSE_STAT_WRITE_NOTREADY))
      return 0;
  }
  return ERRNO_TIMEOUT;
}
static int mouse_wait_readReady(void)
{
  int timeout,stat;

  if(mou_port_overflow)
    return ERRNO_OVER;

  for(timeout=0;timeout<5000;++timeout)
  {
    stat=cpu_in8(MOUSE_PORT_STAT);
    if(stat & MOUSE_STAT_READ_READY) {
      if(stat & MOUSE_STAT_READ_OVERFLOW)
        mou_port_overflow=0;//debug
      return 0;
    }
  }
  return ERRNO_TIMEOUT;
}

static int mouse_send_portcmd(int cmd, int witharg, int arg)
{
  int r;
  int data;

  r=mouse_wait_writeReady();
  if(r<0) {
//    display_puts("mou_snd_port waitwrite1=");
//    int2dec(-r,s);
//    display_puts(s);
//    display_puts("\n");
    return ERRNO_WRITE;
  }
//  display_puts("cmd\n");
  cpu_out8(MOUSE_PORT_CMD, cmd);

  if(witharg) {
    r=mouse_wait_writeReady();
    if(r<0) {
//      display_puts("mou_snd_port waitwrite2=");
//      int2dec(-r,s);
//      display_puts(s);
//      display_puts("\n");
      return ERRNO_WRITE;
    }
//    display_puts("data\n");
    cpu_out8(MOUSE_PORT_DATA, arg);
  }

  r=mouse_wait_readReady();
  if(r<0) {
//    display_puts("mou_snd_port waitread1=");
//    int2dec(-r,s);
//    display_puts(s);
//    display_puts("\n");
    return ERRNO_READ;
  }
//  display_puts("read\n");
  data=(int)cpu_in8(MOUSE_PORT_DATA);

  for(;;) {
    r=mouse_wait_readReady();
    if(r==ERRNO_TIMEOUT)
      break;

    if(r<0) {
//      display_puts("mou_snd_port waitread2=");
//      int2dec(-r,s);
//      display_puts(s);
//      display_puts("\n");
      return r;
    }

    r=(int)cpu_in8(MOUSE_PORT_DATA);
//    display_puts("mou_snd_port read2=");
//    int2dec(r,s);
//    display_puts(s);
//    display_puts("\n");
  }

  return data;
}

int mouse_initsrv(void)
{
  int r;
  int data;

  mou_queid = environment_getqueid();
  if(mou_queid==0) {
    syscall_puts("mou_init que get error");
    syscall_puts("\n");
    return ERRNO_NOTINIT;
  }
  r = syscall_que_setname(mou_queid, MOU_QNM_MOUSE);
  if(r<0) {
    mou_queid = 0;
    display_puts("mou_init msgq=");
    int2dec(-r,s);
    display_puts(s);
    display_puts("\n");
    return r;
  }
  mou_bufferq = message_create_userqueue();
  if(mou_bufferq==NULL) {
    display_puts("mou_init bufq\n");
    return ERRNO_RESOURCE;
  }
  mou_requestq = message_create_userqueue();
  if(mou_requestq==NULL) {
    display_puts("mou_init reqq\n");
    return ERRNO_RESOURCE;
  }

  r=syscall_intr_regist(PIC_IRQ_MOUSE, mou_queid);
  if(r<0) {
    mou_queid = 0;
    display_puts("mou_init intrreg=");
    int2dec(-r,s);
    display_puts(s);
    display_puts("\n");
    return r;
  }

  mou_port_overflow=0;

  data=r=mouse_send_portcmd(MOUSE_PORTCMD_READ_REG,0,0);
  if(r<0) {
    mou_queid = 0;
    display_puts("mou_init sndcmd readreg=");
    int2dec(-r,s);
    display_puts(s);
    display_puts("\n");
    return r;
  }

//  display_puts("mou_init read reg=");
//  byte2hex((char)data,s);
//  display_puts(s);
//  display_puts("\n");

  data = (data & 0xcf) | 0x03;

  r=mouse_send_portcmd(MOUSE_PORTCMD_WRITE_REG,1,data);
  if(r<0 && r!=ERRNO_READ) {
    mou_queid = 0;
    display_puts("mou_init sndcmd writereg=");
    int2dec(-r,s);
    display_puts(s);
    display_puts("\n");
    return r;
  }
/*
  display_puts("mou_init write reg=");
  byte2hex((char)data,s);
  display_puts(s);
  display_puts("\n");
*/

  data=r=mouse_send_portcmd(MOUSE_PORTCMD_WRITE_MOUSEREG,1,MOUSE_PORTDAT_MOU_ENABLE);
  if(r<0) {
    mou_queid = 0;
    display_puts("mou_init sndcmd mouse_enable=");
    int2dec(-r,s);
    display_puts(s);
    display_puts("\n");
    return r;
  }

  if(data!=0xfa) {
    display_puts("mou_init read nack=");
    int2dec(data,s);
    display_puts(s);
    display_puts("\n");
    return -1;
  }

  mou_intrstat=0;

  // flush data
  for(r=0;r<16;r++) {
    data=(int)cpu_in8(MOUSE_PORT_DATA);
  }
  
  pic_enable(PIC_IRQ_MOUSE);	/* enable interrupt from mouse */
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


//static int mouse_shift=0;


int mouse_intr(union mou_msg *msg)
{
  unsigned char data;
  int r;
  union mou_msg code;

  static char button=0;
  static char dx=0;
  static char dy=0;

  if(msg->req.h.arg!=PIC_IRQ_MOUSE) {
    display_puts("intr badirq=");
    long2hex(msg->req.h.command,s);
    display_puts(s);
    display_puts(":");
    long2hex(msg->req.h.arg,s);
    display_puts(s);
    display_puts("\n");
    return -1;
  }

  data = cpu_in8(MOUSE_PORT_DATA);

//data=irq;
//display_putc('.');

  switch(mou_intrstat)
  {
    case 0:
      if(data==0x1c || data==0xfa) {
        mou_intrstat=MOUSE_STAT_INTR_FIRST;
        return 0;
      }
    case MOUSE_STAT_INTR_FIRST:
      button=data;
      mou_intrstat=MOUSE_STAT_INTR_SECOND;
      return 0;

    case MOUSE_STAT_INTR_SECOND:
      dx=data;
      mou_intrstat=MOUSE_STAT_INTR_THIRD;
      return 0;

    case MOUSE_STAT_INTR_THIRD:
      dy= -(char)data;

      mou_intrstat=MOUSE_STAT_INTR_FIRST;
      break;

    default:
      return ERRNO_CTRLBLOCK;

  }


  if(mou_has_request) {
    code.res.h.size=sizeof(code);
    code.res.h.service=MOU_SRV_MOUSE;
    code.res.h.command=MOU_CMD_GETCODE;
    code.res.h.arg = button;
    code.res.dx = dx;
    code.res.dy = dy;
    r=message_send(mou_has_request, &code);
    if(r<0 && r!=ERRNO_OVER) {
      display_puts("mou_intr=");
      int2dec(-r,s);
      display_puts(s);
      display_puts("\n");
      return r;
    }
  }

  return 0;
}

int mouse_cmd_getcode(union mou_msg *msg)
{
  mou_has_request=msg->req.queid;

  return 0;
}


int mouse_krn(union mou_msg *cmd)
{
  int r;

  switch(cmd->req.h.command)
  {
  case MSG_CMD_KRN_INTERRUPT:
//  display_puts("intr!");
    r=mouse_intr(cmd);
    break;
  default:
    r=ERRNO_CTRLBLOCK;
  }
  return r;
}
int mouse_cmd(union mou_msg *cmd)
{
  int r;
  switch(cmd->req.h.command)
  {
    case MOU_CMD_GETCODE:
//    display_puts("get!");
      r=mouse_cmd_getcode(cmd);
      break;
    default:
      r=ERRNO_CTRLBLOCK;
  }
  return r;
}

int start(void)
{
  int r;
  union mou_msg cmd;

  pic_disable(PIC_IRQ_KEYBRD);	/* キーボードを禁止 */
  pic_disable(PIC_IRQ_MOUSE);	/* マウス割り込みを禁止 */
  r=mouse_initsrv();
  pic_enable(PIC_IRQ_KEYBRD);	/* キーボードを許可 */
  if(r<0) {
    return 255;
  }

  for(;;)
  {
    cmd.req.h.size=sizeof(cmd);
    r=syscall_que_get(mou_queid,&cmd);
    if(r<0) {
      display_puts("cmd receive error=");
      int2dec(-r,s);
      display_puts(s);
      display_puts("\n");
      return 255;
    }

//display_puts("srv.cmd=");
//word2hex(cmd.req.h.service,s);
//display_puts(s);
//display_puts(".");
//word2hex(cmd.req.h.command,s);
//display_puts(s);

    switch(cmd.req.h.service) {
    case MSG_SRV_KERNEL:
//    display_puts("intr!");
      r=mouse_krn(&cmd);
      break;
    case MOU_SRV_MOUSE:
//    display_puts("get!");
      r=mouse_cmd(&cmd);
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
      r=ERRNO_CTRLBLOCK;
      break;
    }

    if(r<0) {
      display_puts("*** keyboard terminate ***\n");
      return 255;
    }
  }
  return 0;
}
