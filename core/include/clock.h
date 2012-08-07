#ifndef CLOCK_H
#define CLOCK_H

struct clock_datetime_set {
  int tm_sec;
  int tm_min;
  int tm_hour;
  int tm_mday;
  int tm_mon;
  int tm_year;
  int tm_wday;  // Don't use !!
  int tm_yday;  // Don't use !!
  int tm_isdst; // Don't use !!
};

int clock_get_date(struct clock_datetime_set* datetime_set);

#endif
