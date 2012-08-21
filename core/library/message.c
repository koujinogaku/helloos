/* message management */

#include "config.h"
#include "memory.h"
#include "string.h"
#include "list.h"
#include "syscall.h"
#include "message.h"
#include "errno.h"
#include "environm.h"
#include "display.h"
#include "stdlib.h"

#ifndef min
#define min(a, b)       (a) < (b) ? a : b
#endif

static struct msg_list msg_listhead;
//static char msg_status=0;
static int msg_myqueid=0;

//static char s[16];

#if 0
void *message_alloc(unsigned int srv, unsigned int cmd, unsigned int arg, void* data, unsigned int size)
{
  struct msg_head *msg;
  if(sizeof(struct msg_head)+size >=1024)
    return NULL;
  msg=malloc(sizeof(struct msg_head)+size);
  if(msg==NULL)
    return NULL;
  msg->size=sizeof(struct msg_head)+size;
  msg->service=srv;
  msg->command=cmd;
  msg->arg=arg;
  if(data!=NULL)
    memcpy((void*)((unsigned int)msg + sizeof(struct msg_head)),data,size-sizeof(struct msg_head));
  return msg;
}

void *message_set(void *vmsg, unsigned int srv, unsigned int cmd, unsigned int arg, void* data, unsigned int size)
{
  struct msg_head *msg=vmsg;
  if(sizeof(struct msg_head)+size >=1024)
    return NULL;
  msg->size=sizeof(struct msg_head)+size;
  msg->service=srv;
  msg->command=cmd;
  msg->arg=arg;
  if(data!=NULL)
    memcpy((void*)((unsigned int)msg + sizeof(struct msg_head)),data,size-sizeof(struct msg_head));
  return msg;
}

void message_free(void *msg)
{
  mfree(msg);
}
#endif

int message_send(int queid, void *msg)
{
  int r;

  r=syscall_que_put(queid, msg);
  if(r<0) {
/*
    display_puts("msg_snd que_put=");
    int2dec(-r,s);
    display_puts(s);
    display_puts(" cmd=");
    int2dec(((struct msg_head *)msg)->command,s);
    display_puts(s);
    display_puts(" size=");
    int2dec(((struct msg_head *)msg)->size,s);
    display_puts(s);
    display_puts("\n");
*/
    return r;
  }
  return r;
}

static inline int message_match(unsigned short srv, unsigned short cmd, struct msg_head *msg)
{
  if(srv==0)
    return 1;
  if(srv!=msg->service)
    return 0;
  if(cmd==0)
    return 1;
  if(cmd!=msg->command)
    return 0;
  return 1;
}

static void message_checksigterm(struct msg_head *msg)
{
  if(msg->service==MSG_SRV_KERNEL && msg->command==MSG_CMD_KRN_SIGTERM)
    exit(0);
}

int message_poll(int tryflg, int srv, int cmd, void *vmsgret)
{
  struct msg_list *msgctl;
  struct msg_head *msg;
  int msgsize;
  int r;

  if(msg_myqueid==0) {
    r=message_init();
    if(r<0)
      return r;
  }

  list_for_each(&msg_listhead, msgctl) {
    if(message_match(srv,cmd,msgctl->msg)) {
      msg=vmsgret;
      memcpy(msg,msgctl->msg,min(msgctl->msg->size,msg->size));
      return 1;
    }
  }

  for(;;) {
    if(tryflg==MESSAGE_MODE_WAIT)
      msgsize=syscall_que_peeksize(msg_myqueid);
    else
      msgsize=syscall_que_trypeeksize(msg_myqueid);

    if(msgsize==ERRNO_OVER) {
      return 0;
    }
    if(msgsize<0) {
      return msgsize;
    }
    msg=malloc(msgsize);
    if(msg==NULL)
      return ERRNO_RESOURCE;
    msg->size=msgsize;
    r=syscall_que_get(msg_myqueid, msg);
    if(r<0) {
      mfree(msg);
      return r;
    }
    message_checksigterm(msg);
    msgctl=malloc(sizeof(struct msg_list));
    if(msgctl==NULL) {
      return ERRNO_RESOURCE;
    }
    msgctl->msg=msg;
    list_add_tail(&msg_listhead,msgctl);
    if(message_match(srv,cmd,msg)) {
      msg=vmsgret;
      memcpy(msg,msgctl->msg,min(msgctl->msg->size,msg->size));
      return 1;
    }
  }
}

int message_receive(int tryflg, int srv, int cmd, void *vmsgret)
{
  struct msg_list *msgctl;
  struct msg_head *msg, *msgret=vmsgret;
  int msgsize;
  int r;

  if(msg_myqueid==0) {
    r=message_init();
    if(r<0)
      return r;
  }

  list_for_each(&msg_listhead, msgctl) {
    if(message_match(srv,cmd,msgctl->msg)) {
      msg = msgctl->msg;
      list_del(msgctl);
      mfree(msgctl);
/*
int2dec(msgret->size,s);
display_puts("[");
display_puts(s);
display_puts("]");
*/
      msgsize=msg->size;
      if(msgsize > msgret->size)
        msgsize = msgret->size;
      memcpy(msgret, msg, msgret->size);
      mfree(msg);
      return 0;
    }
  }
  for(;;) {
    if(tryflg==MESSAGE_MODE_WAIT)
      msgsize=syscall_que_peeksize(msg_myqueid);
    else
      msgsize=syscall_que_trypeeksize(msg_myqueid);

    if(msgsize<0) {
      return msgsize;
    }
    msg=malloc(msgsize);
    if(msg==NULL)
      return ERRNO_RESOURCE;
    msg->size=msgsize;
    r=syscall_que_get(msg_myqueid, msg);
    if(r<0) {
      mfree(msg);
      return r;
    }
    message_checksigterm(msg);
    if(message_match(srv,cmd,msg)) {
/*
int2dec(msgret->size,s);
display_puts("[");
display_puts(s);
display_puts("]");
*/
      if(msgsize > msgret->size)
        msgsize = msgret->size;
      memcpy(msgret, msg, msgsize);
      mfree(msg);
      return 0;
    }
    msgctl=malloc(sizeof(struct msg_list));
    if(msgctl==NULL) {
      return ERRNO_RESOURCE;
    }
    msgctl->msg=msg;
    list_add_tail(&msg_listhead,msgctl);
  }
}

struct msg_list *message_create_userqueue(void)
{
  struct msg_list *head;

  head=malloc(sizeof(struct list_head));
  if(head==NULL)
    return NULL;

  list_init(head);

  return head;
}

int message_put_userqueue(struct msg_list *head, void *vmsg)
{
  struct msg_list *msgctl;
  struct msg_head *msgdata, *msg=vmsg;

  msgdata=malloc(msg->size);
  if(msg==NULL)
    return ERRNO_RESOURCE;

  memcpy(msgdata,msg,msg->size);

  msgctl=malloc(sizeof(struct msg_list));
  if(msgctl==NULL) {
    mfree(msgdata);
    return ERRNO_RESOURCE;
  }
  msgctl->msg=msgdata;
  list_add_tail(head,msgctl);
  
  return 0;
}
int message_get_userqueue(struct msg_list *head, void *vmsg)
{
  struct msg_list *msgctl;
  struct msg_head *msg=vmsg;
  int msgsize;

  if(list_empty(head))
    return ERRNO_OVER;

  msgctl=(void*)(head->next);

  list_del(msgctl);

  msgsize = msgctl->msg->size;
  if(msgsize > msg->size)
    msgsize = msg->size;

  memcpy(msg, msgctl->msg, msgsize);
  mfree(msgctl->msg);
  mfree(msgctl);
  return 0;
}
int message_init(void)
{
//for(;;);
//mem_dumpfree();

  if(msg_myqueid!=0)
    return 0;

  list_init(&msg_listhead);

  msg_myqueid=environment_getqueid();
  if(msg_myqueid==0) {
    display_puts("msg_init cliq err");
    display_puts("\n");
    return ERRNO_NOTINIT;
  }
  return 0;
}


