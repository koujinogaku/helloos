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
#include "environm.h"
#include "kmessage.h"


#define BUCKET_MAXDSC 128
#define BUCKET_MAXBUF 8192

#define BUCKET_MODE_OPEN     0
#define BUCKET_MODE_ACCEPT   1
#define BUCKET_MODE_DATA     2 // send=enable,  recv=enable
#define BUCKET_MODE_SHUTDOWN 3 // send=disable, recv=enable  (sent close packet)
#define BUCKET_MODE_SHUTPEER 4 // send=enable,  recv=disable (received close packet)
#define BUCKET_MODE_CLOSED   5 // send=disable, recv=disable (send & received close packet)

#define BUCKET_ARG_CONNECT 1 // not used
#define BUCKET_ARG_DATA    2
#define BUCKET_ARG_CLOSE   3

#define min(a, b)       (a) < (b) ? a : b

union bucket_msg {
  struct msg_head h;
  struct bucket_msg_n {
    struct msg_head h;
    void *block;
    unsigned int size;
  } n;
};

struct bucket_dsc {
  struct bucket_dsc *next;
  struct bucket_dsc *prev;
  int dsc;
  int src_qid;
  int dst_qid;
  int service;
  union bucket_msg last_msg;
  int pos;
  int have_block;
  int mode;
};

typedef struct bucket_dsc BUCKET;


struct BUCKET_TBL {
  struct BUCKET_TBL *next;
  struct BUCKET_TBL *prev;
  unsigned int id;
  int vmem;
  unsigned int size;
};

static struct msg_head selected_msg;

static struct BUCKET_TBL *buckettbl=0;
static BUCKET **dsctbl=0;

static int  *smtx=0;
static char *hctl=0;
static int alarm_command=BUCKET_ALARM_COMMAND;


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

  dsctbl=malloc(sizeof(BUCKET*)*BUCKET_MAXDSC);
  if(dsctbl==0)
    return ERRNO_RESOURCE;
  memset(dsctbl,0,sizeof(BUCKET*)*BUCKET_MAXDSC);
  dsctbl[0]=malloc(sizeof(BUCKET));
  list_init(dsctbl[0]);

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


int bucket_open(void)
{
  int rc,i;

  rc=bucket_init();
  if(rc<0)
    return rc;

  for(i=1;i<BUCKET_MAXDSC;i++)
    if(dsctbl[i]==0)
      break;
  if(i>=BUCKET_MAXDSC)
    return ERRNO_RESOURCE;

  dsctbl[i]=malloc(sizeof(BUCKET));
  memset(dsctbl[i],0,sizeof(BUCKET));
  dsctbl[i]->dsc=i;
  list_add_tail(dsctbl[0],dsctbl[i]);

  dsctbl[i]->mode=BUCKET_MODE_OPEN;
  return i;
}

int bucket_bind(int dsc, int que_name, int service)
{
  int rc;

  rc=bucket_init();
  if(rc<0)
    return rc;

  if(dsc<=0 || dsc>=BUCKET_MAXDSC)
    return ERRNO_NOTEXIST;
  if(dsctbl[dsc]==0)
    return ERRNO_NOTEXIST;
  if(dsctbl[dsc]->mode!=BUCKET_MODE_OPEN)
    return ERRNO_MODE;

  dsctbl[dsc]->src_qid = environment_getqueid();
  dsctbl[dsc]->dst_qid = dsctbl[dsc]->src_qid;    // command dst(as client) = connect 
  dsctbl[dsc]->service = service;
  rc = syscall_que_setname(dsctbl[dsc]->src_qid, que_name);
  if(rc<0)
    return rc;

  dsctbl[dsc]->mode=BUCKET_MODE_ACCEPT;

  return 0;
}

int bucket_connect(int dsc, int que_name, int service)
{
  struct msg_head msg;
  int dst_qid;
  int rc;

  rc=bucket_init();
  if(rc<0)
    return rc;

  if(dsc<=0 || dsc>=BUCKET_MAXDSC)
    return ERRNO_NOTEXIST;
  if(dsctbl[dsc]==0)
    return ERRNO_NOTEXIST;
  if(dsctbl[dsc]->mode!=BUCKET_MODE_OPEN)
    return ERRNO_MODE;

  dst_qid = syscall_que_lookup(que_name);
  if(dst_qid<0)
    return dst_qid;

  dsctbl[dsc]->src_qid = environment_getqueid();
  dsctbl[dsc]->dst_qid = dst_qid;
  dsctbl[dsc]->service = service;

  msg.size=sizeof(struct msg_head);
  msg.service=dsctbl[dsc]->service;
  msg.command=dsctbl[dsc]->dst_qid; // command dst = connect
  msg.arg=dsctbl[dsc]->src_qid;

  rc=message_send(dsctbl[dsc]->dst_qid, &msg);
  if(rc<0)
    return rc;

  dsctbl[dsc]->mode=BUCKET_MODE_DATA;
  return 0;
}

int bucket_send(int dsc, void *buffer, int size)
{
  union bucket_msg msg;
  char *block;
  int rc;

  rc=bucket_init();
  if(rc<0)
    return rc;

  if(dsc<=0 || dsc>=BUCKET_MAXDSC)
    return ERRNO_NOTEXIST;
  if(dsctbl[dsc]==0)
    return ERRNO_NOTEXIST;
  if(dsctbl[dsc]->mode!=BUCKET_MODE_DATA && dsctbl[dsc]->mode!=BUCKET_MODE_SHUTPEER)
    return ERRNO_MODE;

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
  msg.h.service=dsctbl[dsc]->service;
  msg.h.command=dsctbl[dsc]->src_qid; // command src = send
  msg.h.arg=BUCKET_ARG_DATA;
  msg.n.block=block;
  msg.n.size=size;

  rc=message_send(dsctbl[dsc]->dst_qid, &msg);

  if(rc<0) {
    bucket_mfree(block);
  }
  else {
    rc=size;
  }

  return rc;
}

int bucket_select(unsigned long timeout)
{
  struct bucket_dsc *dscp;
  union  bucket_msg bucketmsg;
  int selected=0;
  int rc;
  int timeout_alarm=0;

  rc=bucket_init();
  if(rc<0)
    return rc;

  list_for_each(dsctbl[0],dscp) {
    if(dscp->have_block==0) {
      bucketmsg.h.size=sizeof(union bucket_msg);
      rc=message_receive(MESSAGE_MODE_TRY, dscp->service, dscp->dst_qid, &bucketmsg);
      if(rc==ERRNO_OVER) {
      }
      else if(rc<0)
        return rc;
      else {
        memcpy(&(dscp->last_msg),&bucketmsg, min(sizeof(union bucket_msg),bucketmsg.h.size));
        dscp->pos=0;
        dscp->have_block=1;
        selected=1;
      }
    }
    else {
      selected=1;
    }
  }

  selected_msg.size=sizeof(struct msg_head);
  rc=message_poll(MESSAGE_MODE_TRY, 0, 0, &selected_msg);
  if(rc<0)
    return rc;

  if(selected  && rc==0)
      return BUCKET_SELECT_DATA;

  if(rc==0) {
    if(timeout!=0) {
      for(;;) {
        selected_msg.size=sizeof(struct msg_head);
        rc=message_receive(MESSAGE_MODE_TRY, MSG_SRV_ALARM, alarm_command, &selected_msg);
        if(rc<0)
          break;
      }
      timeout_alarm = syscall_alarm_set(timeout/10,environment_getqueid(), alarm_command<<16);
    }
    selected_msg.size=sizeof(struct msg_head);
    rc=message_poll(MESSAGE_MODE_WAIT, 0, 0, &selected_msg);
    if(rc<0)
      return rc;
    if(timeout!=0) {
      if(selected_msg.service==MSG_SRV_ALARM && selected_msg.command==alarm_command) {
        selected_msg.size=sizeof(struct msg_head);
        rc=message_receive(MESSAGE_MODE_TRY, MSG_SRV_ALARM, alarm_command, &selected_msg);
      }
      else {
        syscall_alarm_unset(timeout_alarm,environment_getqueid());
      }
    }
  }

  list_for_each(dsctbl[0],dscp) {
    if(dscp->have_block==0 && dscp->service==selected_msg.service && dscp->dst_qid==selected_msg.command) {
      bucketmsg.h.size=sizeof(union bucket_msg);
      rc=message_receive(MESSAGE_MODE_TRY, dscp->service, dscp->dst_qid, &bucketmsg);
      if(rc<0)
        return rc;
      memcpy(&(dscp->last_msg),&bucketmsg,sizeof(union bucket_msg));
      dscp->pos=0;
      dscp->have_block=1;
      return BUCKET_SELECT_DATA;
    }
  }

  if(selected)
    return BUCKET_SELECT_DATA;
  else {
    if(selected_msg.service==MSG_SRV_ALARM && selected_msg.command==alarm_command)
      return 0; // timeout
    else
      return BUCKET_SELECT_MSG;
  }
}

void *bucket_selected_msg(void)
{
  return &selected_msg;
}

int bucket_isset(int dsc)
{
  if(dsc<=0 || dsc>=BUCKET_MAXDSC)
    return 0;
  if(dsctbl[dsc]==0)
    return 0;

  return dsctbl[dsc]->have_block;
}

int bucket_accept(int dsc)
{
  int newdsc;
  int rc;

  rc=bucket_init();
  if(rc<0)
    return rc;

  if(dsc<=0 || dsc>=BUCKET_MAXDSC)
    return ERRNO_NOTEXIST;
  if(dsctbl[dsc]==0)
    return ERRNO_NOTEXIST;
  if(dsctbl[dsc]->mode!=BUCKET_MODE_ACCEPT)
    return ERRNO_MODE;

  if(!dsctbl[dsc]->have_block)
    return ERRNO_CTRLBLOCK;

  if(dsctbl[dsc]->last_msg.h.command!=dsctbl[dsc]->src_qid) // not accept command
    return ERRNO_CTRLBLOCK;

  newdsc = bucket_open();
  dsctbl[newdsc]->dst_qid = dsctbl[dsc]->last_msg.h.arg;
  dsctbl[newdsc]->src_qid = dsctbl[dsc]->src_qid;
  dsctbl[newdsc]->service = dsctbl[dsc]->service;
  dsctbl[newdsc]->mode=BUCKET_MODE_DATA;
  dsctbl[dsc]->have_block=0;

  return newdsc;
}

int bucket_recv(int dsc, void *buffer, int size)
{
  union bucket_msg bucketmsg;
  int rc;

  rc=bucket_init();
  if(rc<0)
    return rc;

  if(dsc<=0 || dsc>=BUCKET_MAXDSC)
    return ERRNO_NOTEXIST;
  if(dsctbl[dsc]==0)
    return ERRNO_NOTEXIST;
  if(dsctbl[dsc]->mode==BUCKET_MODE_CLOSED || dsctbl[dsc]->mode==BUCKET_MODE_SHUTPEER) {
    return 0;
  }
  if(dsctbl[dsc]->mode!=BUCKET_MODE_DATA && dsctbl[dsc]->mode!=BUCKET_MODE_SHUTDOWN) {
    return ERRNO_MODE;
  }

  if(!(dsctbl[dsc]->have_block)) {
    bucketmsg.h.size=sizeof(union bucket_msg);
    rc=message_receive(MESSAGE_MODE_WAIT, dsctbl[dsc]->service, dsctbl[dsc]->dst_qid, &bucketmsg);
    //if(rc==ERRNO_OVER) {
    //  return 0;
    //}
    if(rc<0)
      return rc;
    memcpy(&(dsctbl[dsc]->last_msg),&bucketmsg,sizeof(union bucket_msg));
    dsctbl[dsc]->pos=0;
    dsctbl[dsc]->have_block=1;
  }

  if(dsctbl[dsc]->last_msg.h.arg==BUCKET_ARG_CLOSE) {
    dsctbl[dsc]->have_block=0;
    if(dsctbl[dsc]->mode==BUCKET_MODE_SHUTDOWN)
      dsctbl[dsc]->mode=BUCKET_MODE_CLOSED;
    else
      dsctbl[dsc]->mode=BUCKET_MODE_SHUTPEER;
    return 0;
  }

  if(size > dsctbl[dsc]->last_msg.n.size - dsctbl[dsc]->pos)
    size = dsctbl[dsc]->last_msg.n.size - dsctbl[dsc]->pos;

  memcpy(buffer, ((char*)(dsctbl[dsc]->last_msg.n.block))+dsctbl[dsc]->pos, size);
  if(dsctbl[dsc]->pos+size == dsctbl[dsc]->last_msg.n.size) {
    bucket_mfree(dsctbl[dsc]->last_msg.n.block);
    dsctbl[dsc]->have_block=0;
  }
  else {
    dsctbl[dsc]->have_block=1;
    dsctbl[dsc]->pos += size;
  }
  return size;
}

int bucket_shutdown(int dsc)
{
  struct msg_head msg;
  int rc;

  if(dsc<=0 || dsc>=BUCKET_MAXDSC)
    return ERRNO_NOTEXIST;
  if(dsctbl[dsc]==0)
    return ERRNO_NOTEXIST;
  if(dsctbl[dsc]->mode != BUCKET_MODE_DATA &&
     dsctbl[dsc]->mode != BUCKET_MODE_SHUTPEER )
    return 0;

  msg.size=sizeof(struct msg_head);
  msg.service=dsctbl[dsc]->service;
  msg.command=dsctbl[dsc]->src_qid;
  msg.arg=BUCKET_ARG_CLOSE;

  rc=message_send(dsctbl[dsc]->dst_qid, &msg);
  if(rc<0)
    return rc;

  if(dsctbl[dsc]->mode==BUCKET_MODE_SHUTPEER) {
    dsctbl[dsc]->mode=BUCKET_MODE_CLOSED;
  }
  else {
    dsctbl[dsc]->mode=BUCKET_MODE_SHUTDOWN;
  }

  return 0;
}

int bucket_close(int dsc)
{
  int rc;

  if(dsc<=0 || dsc>=BUCKET_MAXDSC)
    return ERRNO_NOTEXIST;
  if(dsctbl[dsc]==0)
    return ERRNO_NOTEXIST;

  if(dsctbl[dsc]->have_block) {
    bucket_mfree(dsctbl[dsc]->last_msg.n.block);
  }

  rc=bucket_shutdown(dsc);
  if(rc<0)
    return rc;

  list_del(dsctbl[dsc]);
  mfree(dsctbl[dsc]);
  dsctbl[dsc]=0;

  return 0;
}
