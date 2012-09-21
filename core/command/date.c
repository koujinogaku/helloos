#include "clock.h"
#include "print.h"
#include "string.h"
#include "time.h"
#include "syscall.h"

int start(int argc, char *argv[])
{
  struct clock_datetime_set datetime;
  char *wday[]  = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
  char *month[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
//  unsigned long unixtime;

#if 0
  time_t systime;
  struct tm *tm;
  struct timeval tmval;
#endif

  clock_get_date(&datetime);
  time_trans_unixtime((void *)&datetime);

  print_format("%s %s %02d %02d:%02d:%02d %d\n", wday[datetime.tm_wday], month[datetime.tm_mon], datetime.tm_mday, datetime.tm_hour, datetime.tm_min, datetime.tm_sec, datetime.tm_year);

#if 0
  time(&systime);
  tm = localtime(&systime);
  print_format("%d %s %s %02d %02d:%02d:%02d %d\n", systime, wday[tm->tm_wday], month[tm->tm_mon], tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec, tm->tm_year+1900);
#endif

  return 0;
}
