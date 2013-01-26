#ifndef TASK_H
#define TASK_H
#include "list.h"

#define TASK_TBLSZ 128
#define TASK_FPUSZ 64


#define TASK_STAT_NOTUSE 0
#define TASK_STAT_RUN    1
#define TASK_STAT_ORDER  2
#define TASK_STAT_READY  3
#define TASK_STAT_WAIT   4
#define TASK_STAT_PREPARATION 5
#define TASK_STAT_IDLE   6
#define TASK_STAT_ZONBIE 7

#define TASK_TYPE_KERNEL  0
#define TASK_TYPE_USER    3

#define TASK_PRI_MID 5

//extern int task_timer_counter;

int task_init(void);
int task_create(void* task_func);
int task_create_process(void *pgd, void *procaddr, unsigned int esp);
int task_start(int taskid);
int task_get_currentid(void);
int task_get_status(int taskid);
int task_get_exitcode(int taskid);
void task_set_lastfunc( int lastfunc );
int task_get_lastfunc(void);
void* task_get_pgd(int taskid);
int task_delete(int taskid);
void task_exit(int exitcode);
int task_enable_io(int taskid);
void task_wait_ipc(void* waitlist);
void task_wake_ipc(void* vwaitlist);
void task_dispatch_start(void);
void task_dispatch_stop(void);
int task_dispatch_status(void);
void task_dispatch(void);
void task_idle_process(void);
void *task_get_fpu(int taskid);
void task_set_fpu(int taskid,void *fpu);
void task_tick(void);
unsigned long task_get_tick(int taskid);

void task_dbg_task(void *s);

#endif
