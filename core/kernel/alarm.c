/* timer */

#include "config.h"
#include "cpu.h"
#include "string.h"
#include "kmemory.h"
#include "console.h"
#include "task.h"
#include "queue.h"
#include "timer.h"
#include "alarm.h"
#include "errno.h"
#include "mutex.h"

#define ALARM_TBLSZ  16
#define ALARM_STAT_NOTUSE 0
#define ALARM_STAT_DMY    1
#define ALARM_STAT_RUN    2


/* alarm list structure */
struct alarm
{
  struct alarm *next;
  struct alarm *prev;
  unsigned short status;
  unsigned short queid;
  unsigned int arg;
  TIMER_SYSTIME_TYPE time;
};

static struct alarm *alarmtbl=0;
static int alarm_tbl_mutex=0;

int alarm_init(void)
{
  /* Create alarm control table */
  alarmtbl = (struct alarm*)mem_alloc(ALARM_TBLSZ * sizeof(struct alarm));
  if(alarmtbl==0)
    return ERRNO_RESOURCE;
/*
{char s[10];
console_puts("almtbl size=");
int2dec(ALARM_TBLSZ * sizeof(struct alarm),s);
console_puts(s);
console_puts("\n");
}
*/
  memset(alarmtbl,0,(ALARM_TBLSZ * sizeof(struct alarm)));

  list_init(alarmtbl);

  return 0;
}

int alarm_set(unsigned int alarmtime, int queid, int cmd_arg)
{
  int i;
  struct alarm *alarm_new, *list;
  TIMER_SYSTIME_TYPE systime;

  mutex_lock(&alarm_tbl_mutex);

  timer_get_systime(&systime);

  /* find a free space */
  for(i=1;i<ALARM_TBLSZ;i++)
  {
    if(alarmtbl[i].status==ALARM_STAT_NOTUSE)
      break;
  }
  if(i>=ALARM_TBLSZ) {
    mutex_unlock(&alarm_tbl_mutex);
    return ERRNO_RESOURCE;  /* Alarm table is full */
  }

  /* Use new alarm as a list */
  alarm_new = &(alarmtbl[i]);
  timer_add(&systime,alarmtime);
  alarm_new->time=systime;
  alarm_new->queid=queid;
  alarm_new->arg=cmd_arg;
  alarm_new->status=ALARM_STAT_RUN;

  /* Insert to alarm list by chronological order */
  list_for_each(alarmtbl,list) {
    if(timer_compare(&(alarm_new->time), &(list->time) )<0) {
      list_insert_prev(list,alarm_new);
      break;
    }
  }
  if(alarmtbl==list) {  // not found
    list_add_tail(alarmtbl,alarm_new); // Add To last becouse it is biggest
  }

  mutex_unlock(&alarm_tbl_mutex);
  return i;  /* alarmid */
}
int alarm_unset(int alarmid, int queid)
{
  struct alarm *alm;

  mutex_lock(&alarm_tbl_mutex);

  alm = &(alarmtbl[alarmid]);
  if(alm->status==ALARM_STAT_NOTUSE || alm->queid != queid) {
    mutex_unlock(&alarm_tbl_mutex);
    return ERRNO_NOTEXIST;
  }

  list_del(alm);
  alm->status = ALARM_STAT_NOTUSE;

  mutex_unlock(&alarm_tbl_mutex);
  return 0;
}

int alarm_wait(unsigned int alarmtime)
{
//  char s[8];
  int r;
  unsigned int q;
//  unsigned char c;
  struct msg_head msg;
  q = queue_create(0);
  if(q<0)
    return ERRNO_RESOURCE;
  r = alarm_set(alarmtime,q,0);
  if(r<0) {
    queue_destroy(q);
    return r;
  }
/*
console_puts("WAIT(q=");
int2str(q,s);
console_puts(s);
console_puts(",a=");
int2str(r,s);
console_puts(s);
console_puts(") ");
*/
  msg.size=sizeof(msg);
  r = queue_get(q, &msg);
  if(r<0) {
    queue_destroy(q);
    return ERRNO_RESOURCE;
  }
  queue_destroy(q);
  return 0;
}

void alarm_check(void)
{
  struct alarm *list;
  TIMER_SYSTIME_TYPE systime;
  int queid;
  unsigned int cmd_arg;
  struct msg_head msg;

  if(alarmtbl==0)
    return;

  if(mutex_trylock(&alarm_tbl_mutex)<0)
    return;

  timer_get_systime(&systime);

  list_for_each(alarmtbl,list) {
    if(timer_compare(&systime ,&(list->time))>=0) {
      queid=list->queid;
      cmd_arg=list->arg;
      list_del(list);
      list->status = ALARM_STAT_NOTUSE;
      msg.size=sizeof(msg);
      msg.service=MSG_SRV_ALARM;
      msg.command=(cmd_arg>>16);
      msg.arg=(cmd_arg&0x0000ffff);
      queue_put(queid,&msg);
      mutex_unlock(&alarm_tbl_mutex);
      break;
    }
  }
  if(alarmtbl==list)
    mutex_unlock(&alarm_tbl_mutex);

}

