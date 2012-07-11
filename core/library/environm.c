#include "syscall.h"
#include "string.h"
#include "errno.h"
#include "environm.h"
#include "config.h"

static unsigned short environment_queid = 0;
static int environment_argc=0;
static char *environment_argv[32];

static char s[10];

void env_setargs(void)
{
  char *args=(char*)CFG_MEM_USERARGUMENT;
  int i;
  int f=0;

  for(i=0;i<CFG_MEM_USERARGUMENTSZ;i++,args++) {
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

  env_setargs();

  return 0;
}

int environment_free(void)
{
  if(environment_queid!=0)
    syscall_que_delete(environment_queid);
  return 0;
}
