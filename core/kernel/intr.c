/*
** intr.c --- interrupt handling
*/

#include "config.h"
#include "cpu.h"
#include "desc.h"
#include "intr.h"
#include "queue.h"
#include "string.h"
#include "pic.h"
#include "task.h"
#include "errno.h"
#include "console.h"

int intr_receiver_queid[16];

void
intr_init(void)
{
  memset(intr_receiver_queid,0,sizeof(intr_receiver_queid));
}

void
intr_define( unsigned int n, unsigned int addr, int dpl )
{
  BEGIN_CPULOCK();

  struct desc_gate *idt = (void*)CFG_MEM_IDTHEAD;
  desc_set_gate( &idt[n], DESC_SELECTOR(DESC_KERNEL_CODE), addr, DESC_GATE_INTR32, dpl );

  END_CPULOCK();
}


#define INTR_SEND_MESSAGE(irq)\
  INTR_INTERRUPT(intr_snd_msg##irq);\
  void intr_snd_msg##irq(void) {\
    struct msg_head intrmsg;\
    if(intr_receiver_queid[irq]!=0) {\
      intrmsg.size=sizeof(intrmsg);\
      intrmsg.service=MSG_SRV_KERNEL;\
      intrmsg.command=MSG_CMD_KRN_INTERRUPT;\
      intrmsg.arg=irq;\
      queue_put(intr_receiver_queid[irq], &intrmsg);\
    }\
    pic_eoi(irq);\
    task_dispatch();\
  }

INTR_SEND_MESSAGE(0);
INTR_SEND_MESSAGE(1);
INTR_SEND_MESSAGE(2);
INTR_SEND_MESSAGE(3);
INTR_SEND_MESSAGE(4);
INTR_SEND_MESSAGE(5);
INTR_SEND_MESSAGE(6);
INTR_SEND_MESSAGE(7);
INTR_SEND_MESSAGE(8);
INTR_SEND_MESSAGE(9);
INTR_SEND_MESSAGE(10);
INTR_SEND_MESSAGE(11);
INTR_SEND_MESSAGE(12);
INTR_SEND_MESSAGE(13);
INTR_SEND_MESSAGE(14);
INTR_SEND_MESSAGE(15);

/*
#define PORT_KEYDAT		0x0060

  INTR_INTERRUPT(intr_snd_key);
  void intr_snd_key(void) {
    int irq=1;
    struct msg_head intrmsg;
//     char data;
//     data = cpu_in8(PORT_KEYDAT);

    if(intr_receiver_queid[irq]!=0) {
      intrmsg.size=sizeof(intrmsg);
      intrmsg.service=MSG_SRV_KERNEL;
      intrmsg.command=MSG_CMD_KRN_INTERRUPT;
      intrmsg.arg=irq;
//  console_putc('0'+intr_receiver_queid[irq]);
      queue_put(intr_receiver_queid[irq], &intrmsg);
//      queue_put(intr_receiver_queid[irq], data);
    }

    pic_eoi(irq);
*/
/*
{
  int pos;
  static char i=0;
  char s[4];


  byte2hex(i,s);
  i++;
  pos = console_getpos();
  console_locatepos(77,1);
  console_puts(s);
  console_putpos(pos);
}
 //console_puts("key!");
*/
/*
    task_dispatch();


}
*/
/*
  INTR_INTERRUPT(intr_snd_mou);

  void intr_snd_mou(void) {
    struct msg_head intrmsg;
    int irq=PIC_IRQ_MOUSE;
    if(intr_receiver_queid[irq]!=0) {
      intrmsg.size=sizeof(intrmsg);
      intrmsg.service=MSG_SRV_KERNEL;
      intrmsg.command=MSG_CMD_KRN_INTERRUPT;
      intrmsg.arg=irq;
      queue_put(intr_receiver_queid[irq], &intrmsg);
    }
    pic_eoi(irq);
{
  int pos;
  static char i=0;
  char s[4];


  byte2hex(i,s);
  i++;
  pos = console_getpos();
  console_locatepos(77,1);
  console_puts(s);
  console_putpos(pos);
}
    task_dispatch();
  }

*/

int
intr_regist_receiver(int irq, int queid)
{
  if(irq<1 || irq>15)
    return ERRNO_NOTEXIST;
  intr_receiver_queid[irq]=queid;

  switch(irq) {
  case  0: intr_define(0x20+irq,INTR_INTR_ENTRY(intr_snd_msg0),INTR_DPL_SYSTEM); break;
  case  1: intr_define(0x20+irq,INTR_INTR_ENTRY(intr_snd_msg1),INTR_DPL_SYSTEM); break;
//  case  1: intr_define(0x20+irq,INTR_INTR_ENTRY(intr_snd_key),INTR_DPL_SYSTEM); break;
  case  2: intr_define(0x20+irq,INTR_INTR_ENTRY(intr_snd_msg2),INTR_DPL_SYSTEM); break;
  case  3: intr_define(0x20+irq,INTR_INTR_ENTRY(intr_snd_msg3),INTR_DPL_SYSTEM); break;
  case  4: intr_define(0x20+irq,INTR_INTR_ENTRY(intr_snd_msg4),INTR_DPL_SYSTEM); break;
  case  5: intr_define(0x20+irq,INTR_INTR_ENTRY(intr_snd_msg5),INTR_DPL_SYSTEM); break;
  case  6: intr_define(0x20+irq,INTR_INTR_ENTRY(intr_snd_msg6),INTR_DPL_SYSTEM); break;
  case  7: intr_define(0x20+irq,INTR_INTR_ENTRY(intr_snd_msg7),INTR_DPL_SYSTEM); break;
  case  8: intr_define(0x20+irq,INTR_INTR_ENTRY(intr_snd_msg8),INTR_DPL_SYSTEM); break;
  case  9: intr_define(0x20+irq,INTR_INTR_ENTRY(intr_snd_msg9),INTR_DPL_SYSTEM); break;
  case 10: intr_define(0x20+irq,INTR_INTR_ENTRY(intr_snd_msg10),INTR_DPL_SYSTEM); break;
  case 11: intr_define(0x20+irq,INTR_INTR_ENTRY(intr_snd_msg11),INTR_DPL_SYSTEM); break;
  case 12: intr_define(0x20+irq,INTR_INTR_ENTRY(intr_snd_msg12),INTR_DPL_SYSTEM); break;
//  case 12: intr_define(0x20+irq,INTR_INTR_ENTRY(intr_snd_mou),INTR_DPL_SYSTEM); break;
  case 13: intr_define(0x20+irq,INTR_INTR_ENTRY(intr_snd_msg13),INTR_DPL_SYSTEM); break;
  case 14: intr_define(0x20+irq,INTR_INTR_ENTRY(intr_snd_msg14),INTR_DPL_SYSTEM); break;
  case 15: intr_define(0x20+irq,INTR_INTR_ENTRY(intr_snd_msg15),INTR_DPL_SYSTEM); break;
  }
  return 0;
}

