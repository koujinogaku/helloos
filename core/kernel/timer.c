/* Timer */

#include "config.h"
#include "cpu.h"
#include "pic.h"
#include "task.h"
#include "alarm.h"
#include "intr.h"
#include "timer.h"
#include "console.h"
#include "string.h"

#define TIMER_PIT_CTRL	0x0043
#define TIMER_PIT_CNT0	0x0040

static TIMER_SYSTIME_TYPE timer_systime_counter=0;


//#pragma interrupt
INTR_INTERRUPT(timer_intr);
void timer_intr(void)
{
  timer_systime_counter++;

  pic_eoi(PIC_IRQ_TIMER);	/* notify "Reception completion" to PIC */
/*
{
  static char i=0;
  int pos;
  char s[4];
  byte2hex(i,s);
  i++;
  pos = console_getpos();
  console_locatepos(77,0);
  console_puts(s);
  console_putpos(pos);
}
*/
  alarm_check();

  task_dispatch();

  return;
}

void timer_init(void)
{
  intr_define(0x20+PIC_IRQ_TIMER, INTR_INTR_ENTRY(timer_intr),INTR_DPL_SYSTEM);

  cpu_out8(TIMER_PIT_CTRL, 0x34);  /*          */
  cpu_out8(TIMER_PIT_CNT0, 0x9c);  /* 10msec   */
  cpu_out8(TIMER_PIT_CNT0, 0x2e);  /*          */

  timer_systime_counter=0;
  return;
}

void timer_enable(void)
{
	pic_enable(PIC_IRQ_TIMER);	/* Allow PIT(TIMER) */
}

void timer_get_systime(TIMER_SYSTIME_TYPE *systime)
{
	*systime = timer_systime_counter;
}


