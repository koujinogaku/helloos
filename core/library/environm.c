#include "syscall.h"
#include "string.h"
#include "errno.h"
#include "environm.h"
#include "config.h"
#include "memory.h"
#include "kmessage.h"
#include "message.h"

struct environment_process_arg {
  short  display_queid;
  short  keyboard_queid;
  char   arguments[CFG_MEM_USERARGUMENTSZ-sizeof(short)*2];
};

static unsigned short environment_queid = 0;
static int environment_argc=0;
static char *environment_argv[32];

static char s[10];

static void env_setargs(void)
{
  struct environment_process_arg *process_arg=(void*)CFG_MEM_USERARGUMENT;
  char *args=&(process_arg->arguments[0]);
  int i;
  int f=0;

  for(i=0;i<sizeof(process_arg->arguments);i++,args++) {
    if(*args!=0) {
      if(f==0) {
        f=1;
        environment_argv[environment_argc++]=args;
        if(environment_argc>=32)
          break;
      }
    }
    else {
      f=0;
    }
  }
}

int environment_get_session_size(void)
{
  return CFG_MEM_USERARGUMENTSZ;
}

void *environment_copy_session(void)
{
  struct environment_process_arg *process_arg=malloc(CFG_MEM_USERARGUMENTSZ);
  struct environment_process_arg *process_arg_org=(void*)CFG_MEM_USERARGUMENT;

  memset(process_arg,0,CFG_MEM_USERARGUMENTSZ);
  process_arg->display_queid = process_arg_org->display_queid;
  process_arg->keyboard_queid = process_arg_org->keyboard_queid;

  return process_arg;
}

void environment_make_args(void *process_arg_v, int argc, char *argv[])
{
  struct environment_process_arg *process_arg=process_arg_v;
  int i,n,len;

  n=0;
  for(i=0;i<argc;i++) {
    if(argv[i]==0)
      break;
    len=strlen(argv[i]);
    strncpy(&process_arg->arguments[n],argv[i],sizeof(process_arg->arguments)-1-n);
    n=n+len+1;
    if(n>=sizeof(process_arg->arguments)-1)
      break;
  }
}

void environment_make_display(void *process_arg_v, int display_queid)
{
  struct environment_process_arg *process_arg=process_arg_v;

  process_arg->display_queid = display_queid;
}

void environment_make_keyboard(void *process_arg_v, int keyboard_queid)
{
  struct environment_process_arg *process_arg=process_arg_v;

  process_arg->keyboard_queid = keyboard_queid;
}

int environment_get_display(void *process_arg_v)
{
  struct environment_process_arg *process_arg=process_arg_v;

  return process_arg->display_queid;
}

int environment_get_keyboard(void *process_arg_v)
{
  struct environment_process_arg *process_arg=process_arg_v;

  return process_arg->keyboard_queid;
}

int environment_exec(char *filename, void *session)
{
  int taskid,r,n;
  int queid;

  taskid=r=syscall_pgm_load(filename, SYSCALL_PGM_TYPE_VGA|SYSCALL_PGM_TYPE_IO);
  if(r<0)
    return r;

  if(session!=0) {
    n = environment_get_session_size();
    r = syscall_pgm_setargs(taskid, session, n);
    if(r<0)
      return r;
  }

  queid = environment_getqueid();

  r=syscall_pgm_start(taskid, queid);
  if(r<0)
    return r;

  return taskid;
}

int environment_allocimage(char *programname, unsigned long size)
{
  return syscall_pgm_allocate(programname,SYSCALL_PGM_TYPE_IO|SYSCALL_PGM_TYPE_VGA,size);
}

int environment_loadimage(int taskid, void *image, unsigned long size)
{
  return syscall_pgm_loadimage(taskid, image, size);
}

int environment_execimage(int taskid, void *session)
{
  int r,n;
  int queid;

  if(session!=0) {
    n = environment_get_session_size();
    r = syscall_pgm_setargs(taskid, session, n);
    if(r<0)
      return r;
  }

  queid = environment_getqueid();

  r=syscall_pgm_start(taskid, queid);
  if(r<0)
    return r;

  return taskid;
}

int environment_getqueid(void)
{
  return environment_queid;
}
int environment_getargc(void)
{
  return environment_argc;
}
char **environment_getargv(void)
{
  return environment_argv;
}

int environment_init(void)
{
  int r;
//syscall_putc('*');
  r = syscall_que_create(0);
  if(r<0) {
    syscall_puts("program que create error=");
    int2dec(-r,s);
    syscall_puts(s);
    syscall_puts("\n");
    return r;
  }
  environment_queid = r;
  r = syscall_pgm_settaskq(environment_queid);
  if(r<0) {
    syscall_puts("program que set error=");
    int2dec(-r,s);
    syscall_puts(s);
    syscall_puts("\n");
    return r;
  }

  env_setargs();

  return 0;
}

int environment_free(void)
{
  if(environment_queid!=0)
    syscall_que_delete(environment_queid);
  return 0;
}

void environment_exit(int exitcode)
{
  environment_free();
  syscall_exit(exitcode); // no return
  for(;;) ;
}

int environment_kill(int taskid)
{
  int taskque;
  struct msg_head msg;

  taskque=syscall_pgm_gettaskq(taskid);
  msg.size=sizeof(msg);
  msg.service=MSG_SRV_KERNEL;
  msg.command=MSG_CMD_KRN_SIGTERM;
  msg.arg=0;

  return message_send(taskque,&msg);
}

int environment_wait(int *exitcode, int tryflg, void *userargs, int usersize)
{
  struct msg_head msg;
  int taskid;
  int r,ec;

  msg.size=sizeof(msg);
  r=message_receive(tryflg, MSG_SRV_KERNEL, MSG_CMD_KRN_EXIT, &msg);
  if(r<0) {
    return r;
  }

  taskid=msg.arg;

  ec = syscall_pgm_getexitcode(taskid);
  if(ec<0) {
    return ec;
  }
  if(ec & 0x8000) {
    if(userargs!=0 && usersize!=0) {
      r = syscall_pgm_getargs(taskid, userargs, usersize);
      if(r<0) {
        syscall_puts("core dump failed.\n");
      }
    }
  }

  r = syscall_pgm_delete(taskid);
  if(r<0) {
    return r;
  }

  *exitcode=ec;

  return taskid;
}

