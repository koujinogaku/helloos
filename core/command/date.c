#include "clock.h"
#include "print.h"

int start(int argc, char *argv[])
{
  struct clock_datetime_set datetime;
  //char *wday[]  = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
  char *month[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

  clock_get_date(&datetime);

  print_format("%s %02d %02d:%02d:%02d %d\n", month[datetime.tm_mon], datetime.tm_mday, datetime.tm_hour, datetime.tm_min, datetime.tm_sec, datetime.tm_year);

  return 0;
}
