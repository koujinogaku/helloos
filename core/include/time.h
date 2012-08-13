#ifndef TIME_H
#define TIME_H

struct tm {
  int tm_sec;
  int tm_min;
  int tm_hour;
  int tm_mday;
  int tm_mon;
  int tm_year;
  int tm_wday;
  int tm_yday;
  int tm_isdst;
};

typedef unsigned long time_t;
typedef unsigned long suseconds_t;

struct timeval {
  time_t tv_sec;
  suseconds_t tv_usec;
};

int time_get_date(struct clock_datetime_set* datetime_set);
unsigned long time_trans_unixtime(struct clock_datetime_set* datetime_set);
int gettimeofday(struct timeval *systime);
time_t time(time_t *timer);
struct tm *gmtime(time_t *timer);
struct tm *localtime(time_t *timer);

#endif
