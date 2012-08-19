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

#define TIMER_PIT_ADDR_CNT0 0x40
#define TIMER_PIT_ADDR_CNT1 0x41
#define TIMER_PIT_ADDR_CNT2 0x42
#define TIMER_PIT_ADDR_CTRL 0x43

// value mode
#define TIMER_PIT_CNT_BIN 0x00
#define TIMER_PIT_CNT_BCD 0x01
// counter mode
#define TIMER_PIT_MODE_0 0x00 // terminal count
#define TIMER_PIT_MODE_1 0x02 // programable one shot
#define TIMER_PIT_MODE_2 0x04 // rate generator
#define TIMER_PIT_MODE_3 0x06 // square generator
#define TIMER_PIT_MODE_4 0x08 // software triggerstrobe
#define TIMER_PIT_MODE_5 0x0A // hardware triggerstrobe
// counter access mode
#define TIMER_PIT_ACC_16  0x30 // 16 bit read load
#define TIMER_PIT_ACC_8H  0x20 // 8 high bit read load
#define TIMER_PIT_ACC_8L  0x10 // 8 low  bit read load
#define TIMER_PIT_ACC_LAT 0x00 // 8 low  bit read load
// counter channel
#define TIMER_PIT_CNL_CNT0 0x00 // counter 0
#define TIMER_PIT_CNL_CNT1 0x40 // counter 1
#define TIMER_PIT_CNL_CNT2 0x80 // counter 2
#define TIMER_PIT_CNL_RBC  0xC0 // read back command

#define TIMER_SYSTIME_LOW_RESO 100 // 10msec=1/100sec

static TIMER_SYSTIME_TYPE timer_systime_counter;

//#pragma interrupt
INTR_INTERRUPT(timer_intr);
void timer_intr(void)
{
  timer_systime_counter.low++;
  if(timer_systime_counter.low >= TIMER_SYSTIME_LOW_RESO) {
    timer_systime_counter.high++;
    timer_systime_counter.low = 0;
  }

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

  task_tick();
  task_dispatch();

  return;
}

// Set timer counter
//   counter -> 0xffff = 54.9msec
//   counter -> 0x0001 =  0.000838msec = 0.838micro-sec
int timer_set_counter(int counter)
{
  cpu_out8(TIMER_PIT_ADDR_CTRL, TIMER_PIT_CNL_CNT0|TIMER_PIT_ACC_16|TIMER_PIT_MODE_2|TIMER_PIT_CNT_BIN );  /*     0x34     */
  cpu_out8(TIMER_PIT_ADDR_CNT0, (counter & 0xff) );
  cpu_out8(TIMER_PIT_ADDR_CNT0, (counter >> 8)   );

  return 0;
}

void timer_init(void)
{
  // 1193180 / 1.19318MHz =   1 sec  --> overflow for 16bit
  // 119318  / 1.19318MHz = 100msec  --> overflow for 16bit
  // 59659   / 1.19318MHz =  50msec      = 1/20sec
  // 11932   / 1.19318MHz =  10msec      = 1/100sec
  // 1193    / 1.19318MHz =   1msec      = 1/1000sec
  // 119     / 1.19318MHz = 100micro-sec = 1/10000sec
  int counter = 11932; 

  intr_define(0x20+PIC_IRQ_TIMER, INTR_INTR_ENTRY(timer_intr),INTR_DPL_SYSTEM);

  timer_set_counter(counter);

  memset(&timer_systime_counter,0,sizeof(timer_systime_counter));
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

void timer_add(TIMER_SYSTIME_TYPE *systime, unsigned int delta)
{
  systime->low += delta;
  if(systime->low >= TIMER_SYSTIME_LOW_RESO) {
    systime->high += (systime->low / TIMER_SYSTIME_LOW_RESO);
    systime->low  %= TIMER_SYSTIME_LOW_RESO;
  }
}

int timer_compare(TIMER_SYSTIME_TYPE *time1, TIMER_SYSTIME_TYPE *time2)
{
  if(time1->high > time2->high)
    return 1;
  if(time1->high < time2->high)
    return -1;
  if(time1->low > time2->low)
    return 1;
  if(time1->low < time2->low)
    return -1;
  return 0;
}
