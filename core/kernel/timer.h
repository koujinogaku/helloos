#ifndef TIMER_H
#define TIMER_H
#include "kmem.h"

/* timer.c */
typedef struct kmem_systime TIMER_SYSTIME_TYPE;

void timer_init(void);
void timer_enable(void);
void timer_get_systime(TIMER_SYSTIME_TYPE *systime);
void timer_add(TIMER_SYSTIME_TYPE *systime, unsigned int delta);
int timer_compare(TIMER_SYSTIME_TYPE *time1, TIMER_SYSTIME_TYPE *time2);


#endif
