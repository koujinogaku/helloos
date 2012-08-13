#include "syscall.h"
#include "string.h"
#include "clock.h"
#include "time.h"
#include "kmem.h"
#include "print.h"

#define TIME_SYSTIME_RESO 100
#define TIME_ZONE_ADJ 9*60*60 // (JST= +900)


static time_t init_unixtime;
static struct clock_datetime_set initial_datetime;
static struct kmem_systime init_systime;

static int days_in_month[]= {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

#define time_adjust_leapyear(year)  (((year)/4) - ((year)/100) + ((year)/400))

static int time_is_leapyear(int year)
{
  if((year%4==0 && year%100!=0) || year%400==0)
    return 1;
  else
    return 0;
}

unsigned long time_trans_unixtime(struct clock_datetime_set* datetime_set)
{
  unsigned int yday=0;
  unsigned int delta_year;
  unsigned long datetime;
  int i;

  if(datetime_set->tm_year<1970)
    return 0;

  delta_year=datetime_set->tm_year - 1970;

  for(i=0; i<datetime_set->tm_mon; i++)
    yday += days_in_month[i];
  if(time_is_leapyear(datetime_set->tm_year) && datetime_set->tm_mon >=2)
    yday++;

  yday += datetime_set->tm_mday;

  datetime_set->tm_yday=yday-1;

  yday = ( delta_year*365L + time_adjust_leapyear(delta_year+1) + yday-1 );
  datetime_set->tm_wday=(yday+4) % 7;

  datetime = yday * 24L*60L*60L;

  datetime += datetime_set->tm_hour * 60 * 60;
  datetime += datetime_set->tm_min  * 60;
  datetime += datetime_set->tm_sec;

  datetime -= TIME_ZONE_ADJ;  

  return datetime;
}

int time_init(void)
{
  static int init=0;
  int r;

  if(init)
    return 0;

  r=clock_get_date(&initial_datetime);
  if(r<0)
    return r;
  r=syscall_krn_get_systime(&init_systime);
  if(r<0)
    return r;

  init_unixtime = time_trans_unixtime(&initial_datetime);

  init=1;
  return 0;
}

int gettimeofday(struct timeval *systime, struct timezone *tz)
{
  struct kmem_systime now;
  int r;

  r=time_init();
  if(r<0)
    return r;

  r=syscall_krn_get_systime(&now);
  if(r<0)
    return r;

  if(now.low >= init_systime.low) {
    systime->tv_usec = (now.low - init_systime.low)*(1000/TIME_SYSTIME_RESO)*1000;
    systime->tv_sec  = now.high - init_systime.high + init_unixtime;
  }
  else {
    systime->tv_usec = (TIME_SYSTIME_RESO - ((init_systime.low - now.low)%TIME_SYSTIME_RESO))*(1000/TIME_SYSTIME_RESO)*1000;
    systime->tv_sec  = now.high - init_systime.high - ( 1+ ((init_systime.low - now.low)/TIME_SYSTIME_RESO)) + init_unixtime;
  }

  if(tz!=0) {
    tz->tz_minuteswest=-(TIME_ZONE_ADJ/60);
    tz->tz_dsttime=0;
  }

  return 0;
}

time_t time(time_t *timer)
{
  struct kmem_systime now;

  time_init();

  syscall_krn_get_systime(&now);
  if(timer!=0) {
    *timer = now.high - init_systime.high + init_unixtime;
    return *timer;
  }

  return now.high - init_systime.high + init_unixtime;
}

struct tm *gmtime(time_t *timer)
{
  static struct tm ltime;

  time_t unixtime = *timer;
  int tmp_year, tmp_yday, tmp_month, is_leapyear;

  ltime.tm_sec  = unixtime % 60;
  unixtime /= 60;
  ltime.tm_min  = unixtime % 60;
  unixtime /= 60;
  ltime.tm_hour = unixtime % 24;
  unixtime /= 24;
  ltime.tm_wday = (unixtime + 4) % 7;
  unixtime += time_adjust_leapyear(1970-1) + (1970-1)*365;
  tmp_year = unixtime / 365;
  tmp_yday = (int)( unixtime % 365 ) - (int)time_adjust_leapyear( tmp_year );
  while( tmp_yday < 0 ) {
    tmp_yday += time_is_leapyear( tmp_year ) ? 366 : 365;
    --tmp_year;
  }
  ++tmp_year;
  ltime.tm_year = tmp_year - 1900;
  ltime.tm_yday = tmp_yday;
  is_leapyear = time_is_leapyear(tmp_year);
  tmp_yday++;
  for( tmp_month = 0; tmp_month < 12; ++tmp_month ) {
    int d;
    d = days_in_month[tmp_month] + ((is_leapyear && tmp_month==1)? 1 : 0);
    if( tmp_yday <= d ){
      break;
    }
    tmp_yday -= d;
  }
  ltime.tm_mon = tmp_month;
  ltime.tm_mday = tmp_yday;
  return &ltime;
}

struct tm *localtime(time_t *timer)
{
  time_t local = *timer+TIME_ZONE_ADJ;
  return gmtime(&local);
}
