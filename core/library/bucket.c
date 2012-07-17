#include "config.h"
#include "list.h"
#include "syscall.h"
#include "memory.h"
#include "errno.h"
#include "shm.h"
#include "string.h"
#include "bucket.h"
#include "message.h"
#include "display.h"

#define BUCKET_MAXDSC 128
#define BUCKET_MAXBUF 8192


struct BUCKET_TBL {
  struct BUCKET_TBL *next;
  struct BUCKET_TBL *prev;
  unsigned int id;
  int vmem;
  unsigned int size;
};


static struct BUCKET_TBL *buckettbl=0;
static int  *smtx=0;
static char *hctl=0;

void *bucket_malloc(unsigned long size)
{
  int *mem;

  syscall_mtx_lock(smtx);
  mem=memory_alloc(size,hctl);
  syscall_mtx_unlock(smtx);
  return mem;
}
void bucket_mfree(void* mem)
{
  syscall_mtx_lock(smtx);
  memory_free(mem,hctl);
  syscall_mtx_unlock(smtx);
}

static int bucket_init_in(void)
{
  int shmrc,rc;

  shmrc=shm_create(CFG_MEM_BUCKETHEAP,CFG_MEM_BUCKETHEAPSZ); // same name to map address
  if(shmrc<0 && shmrc!=ERRNO_INUSE)
    return shmrc;

  rc=shm_map(CFG_MEM_BUCKETHEAP,(void*)CFG_MEM_BUCKETHEAP);
  if(rc<0)
    return rc;

  smtx = (void*)CFG_MEM_BUCKETHEAP;
  hctl = (void*)((unsigned long)CFG_MEM_BUCKETHEAP+sizeof(int));
  if(shmrc!=ERRNO_INUSE) {
    *smtx=1;
    memory_init(hctl,CFG_MEM_BUCKETHEAPSZ-sizeof(int));
    syscall_mtx_unlock(smtx);
    buckettbl = bucket_malloc(sizeof(struct BUCKET_TBL));
    memset(buckettbl,0,sizeof(struct BUCKET_TBL));
    list_init(buckettbl);
  }

  return 0;
}

inline static int bucket_init(void)
{
  if(hctl)
    return 0;
  
  return bucket_init_in();
}


int bucket_open(BUCKET **dsc)
{
  int rc;

  rc=bucket_init();
  if(rc<0)
    return rc;

  *dsc=malloc(sizeof(BUCKET));
  memset(*dsc,0,sizeof(BUCKET));

  return 0;
}

int bucket_bind(BUCKET *dsc, int src_qid)
{
  dsc->src_qid=src_qid;

  return 0;
}

int bucket_connect(BUCKET *dsc, int dst_qid,int service,int command)
{
  dsc->dst_qid=dst_qid;
  dsc->dst_svc=service;
  dsc->dst_cmd=command;

  return 0;
}

int bucket_send(BUCKET *dsc, void *buffer, int size)
{
  union bucket_msg msg;
  char *block;
  int r;

  if(size<=0) {
    return 0;
  }
  if(size>BUCKET_MAXBUF) {
    size = BUCKET_MAXBUF;
  }
  block = bucket_malloc(size);
  if(block==0) {
    return 0;
  }

  memcpy(block,buffer,size);

  msg.h.size=sizeof(union bucket_msg);
  msg.h.service=dsc->dst_svc;
  msg.h.command=dsc->dst_cmd;
  msg.h.arg=dsc->src_qid;
  msg.n.block=block;
  msg.n.size=size;

  r=message_send(dsc->dst_qid, &msg);

  if(r<0) {
    bucket_mfree(block);
  }
  else {
    r=size;
  }

  return r;
}

int bucket_setmsg(BUCKET *dsc, void *msg_v)
{

  if(dsc->have_block) {
    bucket_mfree(dsc->last_msg.n.block);
  }

  memcpy(&(dsc->last_msg),msg_v,sizeof(union bucket_msg));
  dsc->pos=0;
  dsc->have_block=1;

  return 0;
}

int bucket_recv(BUCKET *dsc, void *buffer, int size)
{
  if(!(dsc->have_block))
    return 0;

  if(size > dsc->last_msg.n.size - dsc->pos)
    size = dsc->last_msg.n.size - dsc->pos;

  memcpy(buffer, ((char*)(dsc->last_msg.n.block))+dsc->pos, size);
  if(dsc->pos+size == dsc->last_msg.n.size) {
    bucket_mfree(dsc->last_msg.n.block);
    dsc->have_block=0;
  }
  else {
    dsc->have_block=1;
    dsc->pos += size;
  }
  return size;
}

