#ifndef TIMER_H
#define TIMER_H

/* timer.c */
typedef unsigned long TIMER_SYSTIME_TYPE;

void timer_init(void);
void timer_enable(void);
void timer_get_systime(TIMER_SYSTIME_TYPE *systime);


#endif
