#include "config.h"
#include "cpu.h"
#include "queue.h"
#include "list.h"
#include "kmemory.h"
#include "string.h"
#include "task.h"
#include "queue.h"
#include "errno.h"
#include "console.h"

#define QUEUE_TBLSZ 128
#define QUEUE_SZ    MEM_PAGESIZE

#define QUEUE_STAT_NOTUSE 0
#define QUEUE_STAT_ACTIVE 1

struct QUE {
  short in;
  short out;
  char status;
  char dmy;
  unsigned short name;
  struct list_head waitlist;
  unsigned char *buf;
};

static struct QUE *queuetbl=0;
//static char s[10];

int queue_init(void)
{
  queuetbl = (struct QUE *)mem_alloc(QUEUE_TBLSZ * sizeof(struct QUE));
  if(queuetbl==0)
    return ERRNO_RESOURCE;
  memset(queuetbl,0,QUEUE_TBLSZ * sizeof(struct QUE));
/*
{char s[10];
console_puts("queuetbl size=");
int2dec(QUEUE_TBLSZ * sizeof(struct QUE),s);
console_puts(s);
console_puts("\n");
}
*/
  return 0;
}

int queue_create(unsigned int quename)
{
  int queid;
  struct list_head *head;
  int hold_locked;
  unsigned char *buf;

  cpu_hold_lock(hold_locked);

  for(queid=1;queid<QUEUE_TBLSZ;queid++) {
    if(queuetbl[queid].status==QUEUE_STAT_NOTUSE)
      break;
  }
  if(queid>=QUEUE_TBLSZ){
    cpu_hold_unlock(hold_locked);
    return ERRNO_RESOURCE;
  }
  buf=mem_alloc(QUEUE_SZ);
  if(buf==NULL){
    cpu_hold_unlock(hold_locked);
    return ERRNO_RESOURCE;
  }

  queuetbl[queid].name = quename;
  queuetbl[queid].in  = 0;
  queuetbl[queid].out = 0;
  queuetbl[queid].status = QUEUE_STAT_ACTIVE;
  queuetbl[queid].buf = buf;
  head = &queuetbl[queid].waitlist;
  list_init(head);

  cpu_hold_unlock(hold_locked);

  return queid;
}

int queue_setname(int queid, unsigned int quename)
{
  int hold_locked;

  cpu_hold_lock(hold_locked);

  if(queid<1 || queid>=QUEUE_TBLSZ) {
    cpu_hold_unlock(hold_locked);
    return ERRNO_NOTEXIST;
  }
  if(queuetbl[queid].status == QUEUE_STAT_NOTUSE) {
    cpu_hold_unlock(hold_locked);
    return ERRNO_NOTEXIST;
  }

  queuetbl[queid].name = quename;

  cpu_hold_unlock(hold_locked);

  return queid;
}

int queue_destroy(int queid)
{
  struct list_head *head;
  int hold_locked;

  cpu_hold_lock(hold_locked);

  if(queid<1 || queid>=QUEUE_TBLSZ) {
    cpu_hold_unlock(hold_locked);
    return ERRNO_NOTEXIST;
  }
  if(queuetbl[queid].status == QUEUE_STAT_NOTUSE) {
    cpu_hold_unlock(hold_locked);
    return ERRNO_NOTEXIST;
  }

  queuetbl[queid].status=QUEUE_STAT_NOTUSE;
  queuetbl[queid].in  = 0;
  queuetbl[queid].out = 0;
  mem_free(queuetbl[queid].buf,QUEUE_SZ);
  head = (struct list_head*)&queuetbl[queid].waitlist;
  list_init(head);

  cpu_hold_unlock(hold_locked);
  return 0;
}

static inline int queue_get_freesize(struct QUE *que)
{
  if(que->in == que->out)
    return QUEUE_SZ-1;
  if(que->out < que->in)
    return QUEUE_SZ-1 - que->in + que->out ;
  return que->out - 1 - que->in  ;
}

int queue_put(int queid, struct msg_head *msg)
{
  struct QUE *que;
  int hold_locked;
  unsigned char *chr;
  short sz;

  cpu_hold_lock(hold_locked);

  if(queid<1 || queid>=QUEUE_TBLSZ) {
    cpu_hold_unlock(hold_locked);
    return ERRNO_NOTEXIST;
  }

  que=&(queuetbl[queid]);
  if(que->status==QUEUE_STAT_NOTUSE) {
    cpu_hold_unlock(hold_locked);
    return ERRNO_NOTEXIST;
  }

  if(queue_get_freesize(que) < msg->size) {
    cpu_hold_unlock(hold_locked);
    return ERRNO_OVER;
  }

  chr = (unsigned char*)msg;
  sz = msg_size(msg->size); // round up to 4 byte
//  sz = msg->size & 0xfffc; // truncation to 4 byte

  // Copy it for size that after edited. The "in+2" exists in the absolute becouse 4 byte based
  que->buf[que->in] = sz & 0x00ff;
  que->buf[que->in+1] = sz/256;
  (que->in)+=2;
  sz-=2;
  chr+=2;

  for(; sz>0 ; --sz) {
    que->buf[que->in] = *chr;
    ++chr;
    ++(que->in);
    if(que->in == QUEUE_SZ)
      que->in = 0;
  }

/*
if(queid==5) {
  char s[10];
  console_puts("p(");
  int2dec(que->in,s);
  console_puts(s);
  console_puts(",");
  int2dec(que->out,s);
  console_puts(s);
  console_puts(")");
}
*/
  cpu_hold_unlock(hold_locked);
/*
if(queid==5) {
char s[10];
console_puts("p(");int2dec(que->in,s);console_puts(s);
console_puts(",");int2dec(que->out,s);console_puts(s);
console_puts(")");
console_puts("[");
//task_dbg_task(&(que->waitlist));
//console_puts(",");
}
*/
  task_wake_ipc(&(que->waitlist));

/*
if(queid==5) {
;
task_dbg_task(&(que->waitlist));
console_puts("]");
}
*/
  return 0;

}

int queue_tryget(int queid, struct msg_head *msg)
{
  struct QUE *que;
  int hold_locked;
  short maxsz,sz;
  unsigned char *chr;

  cpu_hold_lock(hold_locked);

  if(queid<1 || queid>=QUEUE_TBLSZ) {
    cpu_hold_unlock(hold_locked);
    return ERRNO_NOTEXIST;
  }
  que=&(queuetbl[queid]);
  if(que->status==QUEUE_STAT_NOTUSE) {
    cpu_hold_unlock(hold_locked);
    return ERRNO_NOTEXIST;
  }
  if(que->in == que->out) {
    cpu_hold_unlock(hold_locked);
    return ERRNO_OVER;
  }

  maxsz = msg->size;
  chr = (unsigned char*)msg;
  sz  = que->buf[que->out];
  sz += que->buf[que->out+1]*256; // It exists in the absolute becouse 4byte based
/*
if(sz==0) {
console_puts("msg sz=0");
}
*/
  for(;sz>0;--sz) {
    if(maxsz>0) {
      *chr = que->buf[que->out];
      --maxsz;
    }
    ++chr;
    ++(que->out);
    if(que->out == QUEUE_SZ)
      que->out = 0;
  }

  cpu_hold_unlock(hold_locked);
  return 0;
}

int queue_get(int queid, struct msg_head *msg)
{
  struct QUE *que;
  int hold_locked;
  short maxsz,sz;
  unsigned char *chr;

  cpu_hold_lock(hold_locked);

  if(queid<1 || queid>=QUEUE_TBLSZ) {
    cpu_hold_unlock(hold_locked);
    return ERRNO_NOTEXIST;
  }
  que=&(queuetbl[queid]);
  if(que->status==QUEUE_STAT_NOTUSE) {
    cpu_hold_unlock(hold_locked);
    return ERRNO_NOTEXIST;
  }

//if(queid==6) {console_puts("g!");}

  while(que->in == que->out) {

//if(queid==5) {console_puts("g[");task_dbg_task(&(que->waitlist));console_puts(",");}
//if(queid==7) {console_puts("g7[");task_dbg_task(&(que->waitlist));console_puts("]");}

    task_wait_ipc(&(que->waitlist));

//if(queid==5) {task_dbg_task(&(que->waitlist));console_puts("]");}

    task_dispatch();
  }

  maxsz = msg->size;
  chr = (unsigned char*)msg;
  sz  = que->buf[que->out];
  sz += que->buf[que->out+1]*256; // It exists in the absolute becouse 4byte based
/*
if(queid==5) {
char s[10];
console_puts("msg id=");
int2dec(queid,s);
console_puts(s);
console_puts("o=");
int2dec(que->out,s);
console_puts(s);
console_puts("i=");
int2dec(que->in,s);
console_puts(s);
console_puts("o[+0]=");
int2dec(que->buf[que->out],s);
console_puts(s);
console_puts("o[+1]=");
int2dec(que->buf[que->out+1],s);
console_puts(s);
console_puts("o[+2]=");
int2dec(que->buf[que->out+2],s);
console_puts(s);
console_puts("o[+3]=");
int2dec(que->buf[que->out+3],s);
console_puts(s);
}
*/
  for(;sz>0;--sz) {
    if(maxsz>0) {
      *chr = que->buf[que->out];
      --maxsz;
    }
    ++chr;
    ++(que->out);
    if(que->out == QUEUE_SZ)
      que->out = 0;
  }
/*
if(((struct msg_head*)msg)->size==0) {
char s[10];
console_puts("msg sz=0 id=");
int2dec(queid,s);
console_puts(s);
console_puts("o=");
int2dec(que->out,s);
console_puts(s);
console_puts("i=");
int2dec(que->in,s);
console_puts(s);
}
*/

/*
if(queid==5) {
  char s[10];
  console_puts("g(");
  int2dec(que->in,s);
  console_puts(s);
  console_puts(",");
  int2dec(que->out,s);
  console_puts(s);
  console_puts(")");
}
*/
  cpu_hold_unlock(hold_locked);
  return 0;
}

int queue_lookup(unsigned int quename)
{
  int queid;

  for(queid=1;queid<QUEUE_TBLSZ;queid++) {
    if(queuetbl[queid].status==QUEUE_STAT_ACTIVE &&
       queuetbl[queid].name==quename )
      return queid;
  }
  return ERRNO_NOTEXIST;
}

int queue_peek_nextsize(int queid)
{
  struct QUE *que;
  int hold_locked;
  short sz;

  cpu_hold_lock(hold_locked);

  if(queid<1 || queid>=QUEUE_TBLSZ) {
    cpu_hold_unlock(hold_locked);
    return ERRNO_NOTEXIST;
  }
  que=&(queuetbl[queid]);
  if(que->status==QUEUE_STAT_NOTUSE) {
    cpu_hold_unlock(hold_locked);
    return ERRNO_NOTEXIST;
  }

  while(que->in == que->out) {
    task_wait_ipc(&(que->waitlist));
    task_dispatch();
  }

  sz  = que->buf[que->out];
  sz += que->buf[que->out+1]*256; // It exists in the absolute becouse 4byte based

  cpu_hold_unlock(hold_locked);
  return sz;
}

int queue_trypeek_nextsize(int queid)
{
  struct QUE *que;
  int hold_locked;
  short sz;

  cpu_hold_lock(hold_locked);

  if(queid<1 || queid>=QUEUE_TBLSZ) {
    cpu_hold_unlock(hold_locked);
    return ERRNO_NOTEXIST;
  }
  que=&(queuetbl[queid]);
  if(que->status==QUEUE_STAT_NOTUSE) {
    cpu_hold_unlock(hold_locked);
    return ERRNO_NOTEXIST;
  }

  if(que->in == que->out) {
    cpu_hold_unlock(hold_locked);
    return ERRNO_OVER;
  }

  sz  = que->buf[que->out];
  sz += que->buf[que->out+1]*256; // It exists in the absolute becouse 4byte based

  cpu_hold_unlock(hold_locked);
  return sz;
}
#if 0
void queue_make_msg(struct msg_head *msg, int size, int srv, int cmd, int arg, void *data)
{
  unsigned char *msgdata=(void *)msg;
  if(size<sizeof(struct msg_head) || size>1024)
    return;
  msg->size = size;
  msg->service = srv;
  msg->command = cmd;
  msg->arg = arg;
  memcpy(&msgdata[sizeof(struct msg_head)],data,size-sizeof(struct msg_head));
}
#endif

