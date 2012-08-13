#include "cpu.h"
#include "string.h"
#include "clock.h"
#include "config.h"

#define CLOCK_RTC_PORT_ADDR		0x70
#define CLOCK_RTC_PORT_DATA		0x71

#define CLOCK_RTC_CMD_SEC		0x00
#define CLOCK_RTC_CMD_SEC_ALARM		0x01
#define CLOCK_RTC_CMD_MINUTE		0x02
#define CLOCK_RTC_CMD_MINUTE_ALARM	0x03
#define CLOCK_RTC_CMD_HOUR		0x04
#define CLOCK_RTC_CMD_HOUR_ALARM	0x05
#define CLOCK_RTC_CMD_WEEK		0x06
#define CLOCK_RTC_CMD_DAY		0x07
#define CLOCK_RTC_CMD_MONTH		0x08
#define CLOCK_RTC_CMD_YEAR		0x09

#if 0
static void clock_write(int reg_no, int val)
{
  BEGIN_CPULOCK();
//  bool hold_locked;
//  hold_locked=cpu_hold_lock(hold_locked);

  cpu_out8(CLOCK_RTC_PORT_ADDR, reg_no);
  cpu_out8(CLOCK_RTC_PORT_DATA, val);

//  cpu_hold_unlock(hold_locked);

  END_CPULOCK();
}
#endif

static int clock_read(int reg_no)
{
  int val;
  BEGIN_CPULOCK();

  //bool hold_locked;
  //hold_locked=cpu_hold_lock(hold_locked);

  cpu_out8(CLOCK_RTC_PORT_ADDR, reg_no);
  val=cpu_in8(CLOCK_RTC_PORT_DATA);

  //cpu_hold_unlock(hold_locked);
  END_CPULOCK();

  return val;
}

static int bcd2bin(int bcd)
{
  return (bcd >> 4) * 10 + ( bcd & 0xf);
}

int clock_get_date(struct clock_datetime_set* datetime_set)
{
  int first=1;
  int sel=0;
  struct clock_datetime_set buff[2];

  memset(&buff,0,sizeof(buff));

  for(;;) {
    buff[sel].tm_sec  = clock_read(CLOCK_RTC_CMD_SEC);
    buff[sel].tm_min  = clock_read(CLOCK_RTC_CMD_MINUTE);
    buff[sel].tm_hour = clock_read(CLOCK_RTC_CMD_HOUR);
    //buff[sel].tm_wday = clock_read(CLOCK_RTC_CMD_WEEK); // Dirty data
    buff[sel].tm_mday = clock_read(CLOCK_RTC_CMD_DAY);
    buff[sel].tm_mon  = clock_read(CLOCK_RTC_CMD_MONTH);
    buff[sel].tm_year = clock_read(CLOCK_RTC_CMD_YEAR);

    if(sel)
      sel=0;
    else
      sel=1;

    if(first) {
      first=0;
      continue;
    }
    if(memcmp(&buff[0],&buff[1],sizeof(struct clock_datetime_set)) == 0)
      break;
  }

  buff[0].tm_year   = bcd2bin(buff[0].tm_year) + 2000;
  buff[0].tm_mon    = bcd2bin(buff[0].tm_mon) - 1;
  buff[0].tm_mday   = bcd2bin(buff[0].tm_mday);
  buff[0].tm_wday   = bcd2bin(buff[0].tm_wday) - 1;  // Dirty data
  if(buff[0].tm_hour & 0x80)
    buff[0].tm_hour = bcd2bin(buff[0].tm_hour & 0x7f) + 12;
  else
    buff[0].tm_hour = bcd2bin(buff[0].tm_hour);
  buff[0].tm_min    = bcd2bin(buff[0].tm_min);
  buff[0].tm_sec    = bcd2bin(buff[0].tm_sec);

  memcpy(datetime_set, &buff[0], sizeof(struct clock_datetime_set));

  return 0;
}

