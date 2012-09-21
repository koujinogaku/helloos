#include "config.h"
#include "cpu.h"
#include "kmemory.h"
#include "desc.h"
#include "task.h"
#include "list.h"
#include "string.h"
#include "console.h"
#include "page.h"
#include "errno.h"
#include "kernel.h"
#include "queue.h"

#define TASK_IOENABLE_SIZE 33
#define TASK_IOBASE	(sizeof(struct TSS)-TASK_IOENABLE_SIZE)

#define TASK_STACKSZ MEM_PAGESIZE

struct TSS {
  word16 back,	r0;
  word32 esp0;
  word16 ss0,	r1;
  word32 esp1;
  word16 ss1,	r2;
  word32 esp2;
  word16 ss2,	r3;
  word32 cr3;
  word32 eip;
  word32 eflags;
  word32 eax;
  word32 ecx;
  word32 edx;
  word32 ebx;
  word32 esp;
  word32 ebp;
  word32 esi;
  word32 edi;
  word16 es,	r4;
  word16 cs,	r5;
  word16 ss,	r6;
  word16 ds,	r7;
  word16 fs,	r8;
  word16 gs,	r9;
  word16 ldtr,	rA;
  word16 trap,	iobase;
  byte ioenable[TASK_IOENABLE_SIZE];
};

struct TASK {
  struct TASK *next;
  struct TASK *prev;
  struct TSS tss;
  struct desc_seg gdt;
  void *stack;
  void *fpu;
  unsigned short id;
  unsigned short status;
  unsigned short exitcode;
  unsigned short lastfunc;
  unsigned long tick;
};

static int task_dispatch_on=0;
static struct TASK *tasktbl=0;
static unsigned short task_real_taskid=0;
//static char s[64];

//#pragma GCC push_options
//#pragma GCC optimize("O0")

void task_idle_process(void)
{
/*
  struct TASK *task_cur;
  task_cur = tasktbl->next;
  task_cur->status = TASK_STAT_IDLE;
*/

  for(;;) {
//    console_puts("idle ");
//    console_putc('I');

    Asm("nop");
    cpu_halt();
/*
{
  static int i=0;
  int pos;
  char s[4];
  byte2hex(i,s);
  i++;
  pos = console_getpos();
  console_locatepos(77,0);
  console_puts(s);
  console_putpos(pos);
}
*/
  }
}
//#pragma GCC pop_options

int task_init(void)
{
  struct desc_seg *gdt = (struct desc_seg *) CFG_MEM_GDTHEAD;
  struct TASK *task_new;
  int idletask;

  tasktbl = (struct TASK*)mem_alloc(TASK_TBLSZ * sizeof(struct TASK));
  if(tasktbl==0)
    return ERRNO_RESOURCE;

/*
{char s[10];
console_puts("tasktbl size=");
int2dec(TASK_TBLSZ * sizeof(struct TASK),s);
console_puts(s);
console_puts("\n");
}
*/
  memset(tasktbl,0,(TASK_TBLSZ * sizeof(struct TASK)));

  list_init(tasktbl);

  task_new = &tasktbl[1];
  task_new->id=1;
  task_new->tss.cr3 = (unsigned int)page_get_system_pgd();
  task_new->tss.es = DESC_SELECTOR(DESC_KERNEL_DATA);
  task_new->tss.cs = DESC_SELECTOR(DESC_KERNEL_CODE);
  task_new->tss.ss = DESC_SELECTOR(DESC_KERNEL_STACK);
  task_new->tss.ds = DESC_SELECTOR(DESC_KERNEL_DATA);
  task_new->tss.fs = DESC_SELECTOR(DESC_KERNEL_DATA);
  task_new->tss.gs = DESC_SELECTOR(DESC_KERNEL_DATA);
  task_new->tss.ldtr = 0;
  task_new->tss.iobase = TASK_IOBASE;
  memset( &(task_new->tss.ioenable), 0xff, TASK_IOENABLE_SIZE );

  task_new->fpu = 0;
  task_new->status = TASK_STAT_RUN;
  task_new->tick = 0;
  list_add_tail(tasktbl, task_new);
  task_real_taskid = 1;

  desc_set_tss32(&gdt[DESC_KERNEL_TASK], (int)&(task_new->tss), sizeof(task_new->tss)-1, DESC_DPL_SYSTEM);
  desc_set_tss32(&task_new->gdt, (int)&(task_new->tss), sizeof(task_new->tss)-1, DESC_DPL_SYSTEM);


  Asm("ltr %%ax"::"a"(DESC_SELECTOR(DESC_KERNEL_TASK)));

  idletask = task_create(task_idle_process);
  if(idletask<0)
    return idletask;
  task_start(idletask);
  tasktbl[idletask].status = TASK_STAT_IDLE;

  return 0;
}


int task_create(void *task_func)
{
  int taskid;
  void *task_stack;
  struct TASK *task_new;
  int hold_locked;

  cpu_hold_lock(hold_locked);

  for(taskid=1;taskid<TASK_TBLSZ;++taskid)
  {
    if(tasktbl[taskid].status == TASK_STAT_NOTUSE)
    break;
  }
  if(taskid>=TASK_TBLSZ) {
    cpu_hold_unlock(hold_locked);
    return ERRNO_RESOURCE;
  }

  task_new = &(tasktbl[taskid]);
  task_new->id=taskid;

//mem_dumpfree();
  task_stack = mem_alloc(TASK_STACKSZ);
  if(task_stack==NULL) {
    cpu_hold_unlock(hold_locked);
    return ERRNO_RESOURCE;
  }
//mem_dumpfree();

  task_new->stack  = task_stack;
  task_new->exitcode = 0;

  task_new->tss.esp0 = 0;
  task_new->tss.ss0  = 0;
  task_new->tss.cr3 = (unsigned int)page_get_system_pgd();
  task_new->tss.eip = (int)task_func;
  task_new->tss.eflags = 0x00000202; /* IF = 1; */
  task_new->tss.eax = 0;
  task_new->tss.ecx = 0;
  task_new->tss.edx = 0;
  task_new->tss.ebx = 0;
  task_new->tss.esp = (unsigned int)task_stack + TASK_STACKSZ;
  task_new->tss.ebp = task_new->tss.esp;
  task_new->tss.esi = 0;
  task_new->tss.edi = 0;
  task_new->tss.es = DESC_SELECTOR(DESC_KERNEL_DATA);
  task_new->tss.cs = DESC_SELECTOR(DESC_KERNEL_CODE);
  task_new->tss.ss = DESC_SELECTOR(DESC_KERNEL_STACK);
  task_new->tss.ds = DESC_SELECTOR(DESC_KERNEL_DATA);
  task_new->tss.fs = DESC_SELECTOR(DESC_KERNEL_DATA);
  task_new->tss.gs = DESC_SELECTOR(DESC_KERNEL_DATA);
  task_new->tss.ldtr = 0;
  task_new->tss.iobase = TASK_IOBASE;
  memset( &(task_new->tss.ioenable), 0xff, TASK_IOENABLE_SIZE );

  desc_set_tss32(&task_new->gdt, (int)&(task_new->tss), sizeof(task_new->tss)-1, DESC_DPL_SYSTEM);

  task_new->fpu = 0;
  task_new->status = TASK_STAT_STOP;
  task_new->tick = 0;

  cpu_hold_unlock(hold_locked);
  return taskid;
}

int task_create_process(void *pgd, void *procaddr, unsigned int esp)
{
  struct TASK *task_new;
  void *pageaddr;
  int taskid;

  pageaddr=page_frame_addr(procaddr);

  taskid = task_create(pageaddr);
  if(taskid<0) {
    return taskid;
  }

  task_new = &(tasktbl[taskid]);

  task_new->tss.esp= esp;
  task_new->tss.ebp= task_new->tss.esp;
  task_new->tss.es = DESC_SELECTOR(DESC_USER_DATA) | DESC_DPL_USER;
  task_new->tss.cs = DESC_SELECTOR(DESC_USER_CODE) | DESC_DPL_USER;
  task_new->tss.ss = DESC_SELECTOR(DESC_USER_STACK)| DESC_DPL_USER;
  task_new->tss.ds = DESC_SELECTOR(DESC_USER_DATA) | DESC_DPL_USER;
  task_new->tss.fs = DESC_SELECTOR(DESC_USER_DATA) | DESC_DPL_USER;
  task_new->tss.gs = DESC_SELECTOR(DESC_USER_DATA) | DESC_DPL_USER;
  task_new->tss.esp0 = (unsigned int)task_new->stack+TASK_STACKSZ;
  task_new->tss.ss0  = DESC_SELECTOR(DESC_KERNEL_DATA);
  task_new->tss.cr3 = (unsigned int)pgd;

  return taskid;
}

int task_start(int taskid)
{
  struct TASK *task_cur;
  int hold_locked;

  cpu_hold_lock(hold_locked);

  task_cur = &(tasktbl[taskid]);
  task_cur->status = TASK_STAT_RUN;
  list_add_tail(tasktbl, task_cur);

  cpu_hold_unlock(hold_locked);
  return 0;
}
int task_get_currentid(void)
{
  struct TASK *task_cur;
  task_cur = tasktbl->next;
  return task_cur->id;
}
int task_get_status(int taskid)
{
  struct TASK *task_cur;
  task_cur = &(tasktbl[taskid]);
  return task_cur->status;
}
void *task_get_pgd(int taskid)
{
  struct TASK *task_cur;
  task_cur = &(tasktbl[taskid]);
  return (void*)task_cur->tss.cr3;
}

void task_set_lastfunc( int lastfunc )
{
  tasktbl->next->lastfunc = lastfunc;
}
int task_get_lastfunc(void)
{
  return tasktbl->next->lastfunc;
}

void *task_get_fpu(int taskid)
{
  struct TASK *task_cur;
  if(taskid>=TASK_TBLSZ)
    return (void*)-1;
  task_cur = &(tasktbl[taskid]);
  return task_cur->fpu;
}

void task_set_fpu(int taskid,void *fpu)
{
  struct TASK *task_cur;
  if(taskid>=TASK_TBLSZ)
    return;
  task_cur = &(tasktbl[taskid]);
  task_cur->fpu=fpu;
}

void task_dbg_dumplist(void *lst)
{
  char s[10];
  struct TASK *tsk,*head=lst;
  list_for_each(head,tsk) {
    console_puts("T(");
    int2dec(tsk->id,s);
    console_puts(s);
    console_puts(",");
    int2dec(tsk->status,s);
    console_puts(s);
    console_puts(")");
  }
}
void task_dbg_task(void *lst)
{
  console_puts("Wt");
  task_dbg_dumplist(lst);
  console_puts("Ru");
  task_dbg_dumplist(tasktbl);
}

int task_delete(int taskid)
{
  struct TASK *task_cur;
  int exitcode;
  void *stack;
  int hold_locked;

  cpu_hold_lock(hold_locked);

  task_cur = &(tasktbl[taskid]);
  task_cur->status = TASK_STAT_NOTUSE;
  exitcode = task_cur->exitcode;
  stack = task_cur->stack;
  list_del(task_cur);

  if(stack != 0) {
    mem_free(stack,TASK_STACKSZ);
  }
  memset(task_cur,0,sizeof(struct TASK));

  cpu_hold_unlock(hold_locked);
  return exitcode;
}

void task_exit(int exitcode)
{
  struct TASK *task_cur;
  int kq;
  int hold_locked;
  struct msg_head msg;
  unsigned short taskid;

  cpu_hold_lock(hold_locked);

  exitcode = exitcode & 0x7fff;

  task_cur = tasktbl->next;
  taskid = task_cur->id;
  list_del(task_cur);
  task_cur->status = TASK_STAT_ZONBIE;
  task_cur->exitcode = exitcode;

  kq = queue_lookup(MSG_QNM_KERNEL);
  if(kq>=0) {
    msg.size=sizeof(msg);
    msg.service=MSG_SRV_KERNEL;
    msg.command=MSG_CMD_KRN_EXIT;
    msg.arg=taskid;
    queue_put(kq,&msg);
  }

  cpu_hold_unlock(hold_locked);

  task_dispatch();
  
  // for debug
  for(;;) {
    console_puts("[ZONBIE]");
    cpu_halt();
  }
}

void task_wait_ipc(void* vwaitlist)
{
  struct TASK *task_cur, *waitlist=vwaitlist;
  int hold_locked;

  if(tasktbl==NULL) {
    return;
  }
  if(list_empty(tasktbl)) {
    return;
  }

  cpu_hold_lock(hold_locked);

  task_cur = tasktbl->next;

//if(task_cur->id==4 && task_cur->status==3) { console_puts("*"); }

  task_cur->status = TASK_STAT_WAIT;
  list_del(task_cur);
  list_add_tail(waitlist,task_cur);

//if(task_cur->id==4) { console_puts("+"); }

  cpu_hold_unlock(hold_locked);
}

void task_wake_ipc(void* vwaitlist)
{
//  char s[5];
  struct TASK *task_wake, *task_idx, *waitlist=vwaitlist;
  int hold_locked;

  cpu_hold_lock(hold_locked);

  if(list_empty(waitlist)) {
    cpu_hold_unlock(hold_locked);
    return;
  }
 
  task_wake = waitlist->next;
  task_wake->status = TASK_STAT_ORDER;
  list_del(task_wake);
  list_for_each(tasktbl,task_idx) {
    if(task_idx->status!=TASK_STAT_RUN && task_idx->status!=TASK_STAT_ORDER) {
      break;
    }
  }
  list_insert_prev(task_idx,task_wake);

/*
  console_puts("T");
  list_for_each(tasktbl,task_idx) {
    int2str(task_idx->status,s);
    console_puts(s);
  }
*/

  cpu_hold_unlock(hold_locked);
}

int task_enable_io(int taskid)
{
  struct TASK *task_cur;
  task_cur = &(tasktbl[taskid]);
  task_cur->tss.eflags |= 0x00003000;
  return 0;
}

void task_dispatch_start(void)
{
  task_dispatch_on=1;
}
void task_dispatch_stop(void)
{
  task_dispatch_on=0;
}

void task_tick(void)
{
  struct TASK *task_cur;
  task_cur = tasktbl->next;
  task_cur->tick++;
}

unsigned long task_get_tick(int taskid)
{
  struct TASK *task_cur;
  task_cur = &(tasktbl[taskid]);
  return task_cur->tick;
}

//#pragma GCC push_options
//#pragma GCC optimize("O0")

void task_dispatch(void)
{
  struct TASK *task_cur;//, *task_prev;
//  struct TASK *task_prev_n, *task_prev_p;
  struct desc_seg *gdt;
  int hold_locked;
//  static unsigned int dispatch_dbg_cnt=0;

  if(list_empty(tasktbl)) {
    return;
  }

  cpu_hold_lock(hold_locked);

  if(task_dispatch_on==0) {
    cpu_hold_unlock(hold_locked);
    return;
  }
  gdt = (struct desc_seg *) CFG_MEM_GDTHEAD;

/*
dispatch_dbg_cnt++;
if(dispatch_dbg_cnt==3) { 
console_puts("%"); 
task_dbg_task(tasktbl);
}
*/

  //task_prev = 
  task_cur = tasktbl->next;
//  task_prev_n = task_prev->next; task_prev_p = task_prev->prev; 
/*
{static int n=0;
if(task_cur->id==2) {
  console_puts("HELLO BEFORE!!\n");
  task_dbg_task(tasktbl);
  for(;;);
    n++;
  }
}
*/

  if(task_cur->status == TASK_STAT_RUN) {  // if "Running" status is ongoing then go the last
    task_cur->status = TASK_STAT_READY;
    list_del(task_cur);
    list_add_tail(tasktbl,task_cur);
  }
  if(tasktbl->next->status != TASK_STAT_IDLE) { // if first task is normal then execute
    tasktbl->next->status = TASK_STAT_RUN;
  }
  else {                                        // if first task is idle then
    if(tasktbl->next->next != tasktbl) {        // and if another task exists then skip for idle
      task_cur = tasktbl->next;
      list_del(task_cur);
      list_add_tail(tasktbl,task_cur);
      tasktbl->next->status = TASK_STAT_RUN;
//      console_puts("s");
    }
  }
  task_cur = tasktbl->next;
/*
if(task_cur->status != TASK_STAT_IDLE) {
  int pos;
  char s[10];
  pos = console_getpos();
  console_locatepos(0,task_cur->id);
  console_puts("CS=");
  long2hex(task_cur->tss.cs,s);
  console_puts(s);
  console_puts(" EIP=");
  long2hex(task_cur->tss.eip,s);
  console_puts(s);
  console_puts(" SS=");
  long2hex(task_cur->tss.ss,s);
  console_puts(s);
  console_puts(" ESP=");
  long2hex(task_cur->tss.esp,s);
  console_puts(s);
  console_puts(" SS0=");
  long2hex(task_cur->tss.ss0,s);
  console_puts(s);
  console_puts(" ESP0=");
  long2hex(task_cur->tss.esp0,s);
  console_puts(s);
  console_putpos(pos);
}
*/
/*
if(task_cur->status!=1&&task_cur->status!=6) { 
char s[10];
console_puts("$"); 
int2dec(dispatch_dbg_cnt,s);
console_puts(s);
task_dbg_task(tasktbl);
}
*/
/*
{static int n=0;
if(task_cur->id==2) {
  console_puts("HELLO IN!!\n");
  task_dbg_task(tasktbl);
  for(;;);
    n++;
  }
}
*/

  if(task_cur->id==task_real_taskid) {
    cpu_hold_unlock(hold_locked);
    return;
  }

  memcpy(&gdt[DESC_KERNEL_TASKBAK],&gdt[DESC_KERNEL_TASK],sizeof(struct desc_seg));
  gdt[DESC_KERNEL_TASKBAK].type = DESC_GATE_TSS32;
  Asm("ltr %%ax"::"a"(DESC_SELECTOR(DESC_KERNEL_TASKBAK)));

  memcpy(&gdt[DESC_KERNEL_TASK],&task_cur->gdt,sizeof(struct desc_seg));

  task_real_taskid=task_cur->id;
  cpu_hold_unlock(hold_locked);

  Asm("ljmpl %0,$0"::"i"(DESC_SELECTOR(DESC_KERNEL_TASK)));
/*
console_puts("HELLO OUT!!\n");
  for(;;);
*/
}
//#pragma GCC pop_options

