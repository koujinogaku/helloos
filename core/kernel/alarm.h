#ifndef ALARM_H
#define ALARM_H

int alarm_init(void);
int alarm_set(unsigned int alarmtime, int queid, int arg);
int alarm_unset(int alarmid, int queid);
int alarm_wait(unsigned int alarmtime);
void alarm_check(void);

#endif
