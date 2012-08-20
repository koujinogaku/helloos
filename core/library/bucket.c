#include "config.h"
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
#include "stdlib.h"

#define BUCKET_MAXDSC 128
#define BUCKET_MAXBUF 8192

#define BUCKET_MODE_OPEN     0
#define BUCKET_MODE_ACCEPT   1
#define BUCKET_MODE_DATA     2 // send=enable,  recv=enable
#define BUCKET_MODE_SHUTDOWN 3 // send=disable, recv=enable  (sent close packet)
#define BUCKET_MODE_SHUTPEER 4 // send=enable,  recv=disable (received close packet)
#define BUCKET_MODE_CLOSED   5 // send=disable, recv=disable (send & received close packet)

#define BUCKET_ARG_CONNECT 1
#define BUCKET_ARG_DATA    2
#define BUCKET_ARG_CLOSE   3

#ifndef min
#define min(a, b)       (a) < (b) ? a : b
#endif

#define BUCKET_QUEMAX 128

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

struct bucket_quetbl {
  int dsc;
  int service;
};

typedef struct bucket_dsc BUCKET;

static struct msg_head selected_msg;

static BUCKET **dsctbl=0;

static int  *smtx=0;
static char *hctl=0;
static int alarm_command=BUCKET_ALARM_COMMAND;
static struct bucket_quetbl *quetbl=0;

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

  dsctbl=malloc(sizeof(BUCKET)*BUCKET_MAXDSC);
  if(dsctbl==0)
    return ERRNO_RESOURCE;
  memset(dsctbl,0,sizeof(BUCKET)*BUCKET_MAXDSC);

  quetbl=malloc(sizeof(struct bucket_quetbl)*BUCKET_QUEMAX);
  if(quetbl==0)
    return ERRNO_RESOURCE;
  memset(quetbl,0,sizeof(struct bucket_quetbl)*BUCKET_QUEMAX);
  for(rc=0;rc<BUCKET_QUEMAX;rc++)
    quetbl[rc].dsc = -1;

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

  for(i=0;i<BUCKET_MAXDSC;i++)
    if(dsctbl[i]==0)
      break;
  if(i>=BUCKET_MAXDSC)
    return ERRNO_RESOURCE;

  dsctbl[i]=malloc(sizeof(BUCKET));
  memset(dsctbl[i],0,sizeof(BUCKET));
  dsctbl[i]->dsc=i;

  dsctbl[i]->mode=BUCKET_MODE_OPEN;

  return i;
}

int bucket_bind(int dsc, int que_name, int service)
{
  int rc;

  rc=bucket_init();
  if(rc<0)
    return rc;

  if(dsc<0 || dsc>=BUCKET_MAXDSC)
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

  if(dsctbl[dsc]->dst_qid < BUCKET_QUEMAX) {
    quetbl[dsctbl[dsc]->dst_qid].dsc = dsc;
    quetbl[dsctbl[dsc]->dst_qid].service = service;
  }

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

  if(dsc<0 || dsc>=BUCKET_MAXDSC)
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

  rc=message_receive(MESSAGE_MODE_WAIT, dsctbl[dsc]->service, dsctbl[dsc]->dst_qid, &msg);
  if(rc<0)
    return rc;
  if(msg.arg!=BUCKET_ARG_CONNECT)
    return ERRNO_CTRLBLOCK;

  dsctbl[dsc]->mode=BUCKET_MODE_DATA;

  if(dsctbl[dsc]->dst_qid < BUCKET_QUEMAX) {
    quetbl[dsctbl[dsc]->dst_qid].dsc = dsc;
    quetbl[dsctbl[dsc]->dst_qid].service = service;
  }

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

  if(dsc<0 || dsc>=BUCKET_MAXDSC)
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

int bucket_find_dsc(int service, int queid)
{
  if(queid >= BUCKET_QUEMAX) {
    return ERRNO_NOTEXIST;
  }

  if(quetbl[queid].dsc == -1) {
    return ERRNO_NOTEXIST;
  }
  if(quetbl[queid].service != service) {
    return ERRNO_NOTEXIST;
  }

  return quetbl[queid].dsc;
}

int bucket_has_block(int dsc)
{
  if(dsc<0 || dsc>=BUCKET_MAXDSC)
    return ERRNO_NOTEXIST;
  if(dsctbl[dsc]==0)
    return ERRNO_NOTEXIST;

  return dsctbl[dsc]->have_block;
}

int bucket_select(int fdsetsize, fd_set *rfds, unsigned long timeout)
{
  struct bucket_dsc *dscp;
  union  bucket_msg bucketmsg;
  int selected=0;
  int rc;
  int fd;
  int timeout_alarm=0;
  fd_set result;

  rc=bucket_init();
  if(rc<0)
    return rc;

  FD_ZERO(&result);
  for(fd=0; fd<fdsetsize && fd<BUCKET_MAXDSC; fd++) {
    if(dsctbl[fd]==0 || !FD_ISSET(fd,rfds))
      continue;
    dscp = dsctbl[fd];
    if(dscp->have_block==0) {
      bucketmsg.h.size=sizeof(union bucket_msg);
      rc=message_poll(MESSAGE_MODE_TRY, dscp->service, dscp->dst_qid, &bucketmsg);
      if(rc==0) {
      }
      else if(rc<0)
        return rc;
      else {
        FD_SET(fd,&result);
        memcpy(&selected_msg,&bucketmsg,sizeof(selected_msg));
        selected=1;
      }
    }
    else {
      FD_SET(fd,&result);
      memcpy(&selected_msg,(&dscp->last_msg),sizeof(selected_msg));
      selected=1;
    }
  }

  if(selected) {
    memcpy(rfds,&result,sizeof(fd_set));
    return BUCKET_SELECT_DATA;
  }

  selected_msg.size=sizeof(struct msg_head);
  rc=message_poll(MESSAGE_MODE_TRY, 0, 0, &selected_msg);
  if(rc<0)
    return rc;

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

  for(fd=0; fd<fdsetsize && fd<BUCKET_MAXDSC; fd++) {
    if(dsctbl[fd]==0 || !FD_ISSET(fd,rfds))
      continue;
    dscp = dsctbl[fd];
    if(dscp->service==selected_msg.service && dscp->dst_qid==selected_msg.command) {
      FD_SET(fd,&result);
      memcpy(rfds,&result,sizeof(fd_set));
      return BUCKET_SELECT_DATA;
    }
  }

  FD_ZERO(rfds);
  if(selected_msg.service==MSG_SRV_ALARM && selected_msg.command==alarm_command)
    return 0; // timeout
  else
    return BUCKET_SELECT_MSG;
}

void *bucket_selected_msg(void)
{
  return &selected_msg;
}

int bucket_accept(int dsc)
{
  int newdsc;
  int rc;
  union bucket_msg bucketmsg;
  unsigned int client;

  rc=bucket_init();
  if(rc<0)
    return rc;

  if(dsc<0 || dsc>=BUCKET_MAXDSC)
    return ERRNO_NOTEXIST;
  if(dsctbl[dsc]==0)
    return ERRNO_NOTEXIST;
  if(dsctbl[dsc]->mode!=BUCKET_MODE_ACCEPT)
    return ERRNO_MODE;

  if(!(dsctbl[dsc]->have_block)) {
    bucketmsg.h.size=sizeof(union bucket_msg);
    rc=message_receive(MESSAGE_MODE_WAIT, dsctbl[dsc]->service, dsctbl[dsc]->dst_qid, &bucketmsg);
    if(rc<0)
      return rc;
    memcpy(&(dsctbl[dsc]->last_msg),&bucketmsg,sizeof(union bucket_msg));
    dsctbl[dsc]->pos=0;
    dsctbl[dsc]->have_block=1;
  }
  if(dsctbl[dsc]->last_msg.h.command!=dsctbl[dsc]->src_qid) // not accept command
    return ERRNO_CTRLBLOCK;

  client = dsctbl[dsc]->last_msg.h.arg;

  bucketmsg.h.arg=BUCKET_ARG_CONNECT;
  rc=message_send(client, &bucketmsg);
  if(rc<0)
    return rc;

  newdsc = bucket_open();
  dsctbl[newdsc]->dst_qid = client;
  dsctbl[newdsc]->src_qid = dsctbl[dsc]->src_qid;
  dsctbl[newdsc]->service = dsctbl[dsc]->service;
  dsctbl[newdsc]->mode=BUCKET_MODE_DATA;
  dsctbl[dsc]->have_block=0;

  if(dsctbl[newdsc]->dst_qid < BUCKET_QUEMAX) {
    quetbl[dsctbl[newdsc]->dst_qid].dsc = newdsc;
    quetbl[dsctbl[newdsc]->dst_qid].service = dsctbl[newdsc]->service;
  }

  return newdsc;
}

int bucket_recv(int dsc, void *buffer, int size)
{
  union bucket_msg bucketmsg;
  int rc;

  rc=bucket_init();
  if(rc<0)
    return rc;

  if(dsc<0 || dsc>=BUCKET_MAXDSC)
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

  if(dsc<0 || dsc>=BUCKET_MAXDSC)
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
  //int rc;

  if(dsc<0 || dsc>=BUCKET_MAXDSC)
    return ERRNO_NOTEXIST;
  if(dsctbl[dsc]==0)
    return ERRNO_NOTEXIST;

  if(dsctbl[dsc]->have_block) {
    bucket_mfree(dsctbl[dsc]->last_msg.n.block);
  }

  //rc=bucket_shutdown(dsc);
  //if(rc<0)
  //  return rc;

  if(dsctbl[dsc]->dst_qid < BUCKET_QUEMAX) {
    quetbl[dsctbl[dsc]->dst_qid].dsc = -1;
    quetbl[dsctbl[dsc]->dst_qid].service = 0;
  }

  mfree(dsctbl[dsc]);
  dsctbl[dsc]=0;

  return 0;
}
