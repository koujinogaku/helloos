#include "config.h"
#include "task.h"
#include "fpu.h"
#include "string.h"
#include "exception.h"
#include "kmemory.h"
#include "errno.h"
#include "intr.h"
#include "console.h"
#include "cpu.h"

struct FPUTASK {
  int status;
  char registers[108];
};

static struct FPUTASK *fputbl=0;
static struct FPUTASK *fpu_current=0;

void *fpu_alloc(void)
{
  int i;
  for(i=1;i<TASK_FPUSZ;++i)
  {
    if(fputbl[i].status == TASK_STAT_NOTUSE) {
      fputbl[i].status = TASK_STAT_RUN;
      memcpy(&(fputbl[i].registers),&(fputbl[0].registers),sizeof(fputbl[0].registers));
      return &(fputbl[i]);
    }
  }
  return 0;
}

void fpu_free(void *fpuv)
{
  struct FPUTASK *fpu=fpuv;

  if(fputbl==0)
    return;

  fpu->status=TASK_STAT_NOTUSE;
  if(fpu==fpu_current)
    fpu_current=0;
}

EXCP_EXCEPTION(fpu_handler); 
void fpu_handler( Excinfo info )
{
  int taskid;
  void *addr;
  struct FPUTASK *fpu_task;

  if(fputbl==0) {
    excp_abort(&info, EXCP_NO_FPU, "No Math Coprocessor");
    return;
  }
  // Disable TS flag
  asm volatile(
    "movl %%cr0, %%eax \n"
    "and  $0xfffffff7, %%eax \n"
    "movl %%eax, %%cr0"
    :::"eax"
  );
  taskid=task_get_currentid();
  fpu_task=task_get_fpu(taskid);
  if(fpu_current!=0 && fpu_current==fpu_task) {
    return;
  }

  if(fpu_current!=0) {
    addr=&(fpu_current->registers);
    asm volatile (
      "fsave (%0)\n"
      "fwait"
      ::"r"(addr)
    );
  }
  if(fpu_task==0) {
    fpu_current=fpu_alloc();
    task_set_fpu(taskid,fpu_current);
  }
  else {
    fpu_current=fpu_task;
  }
  addr=&(fpu_current->registers);
  asm volatile (
    "frstor (%0)\n"
    "fwait"
    ::"r"(addr)
  );
}

int fpu_init(void)
{
  void *fputmpl;
  int r;

  asm volatile(
    "mov %%cr0, %0"
    :"=r"(r):
  );
  if(!(r & 0x10))
    return 0;

  fputbl = (struct FPUTASK*)mem_alloc(TASK_FPUSZ * sizeof(struct FPUTASK));
  if(fputbl==0)
    return ERRNO_RESOURCE;
  memset(fputbl,0,(TASK_FPUSZ * sizeof(struct FPUTASK)));

  // Enable Math Present
  asm volatile(
    "mov %%cr0, %%eax\n"
    "or  $0x00000002, %%eax\n"
    "mov %%eax, %%cr0"
    :::"eax"
  );

  fputbl[0].status=TASK_STAT_RUN;
  fputmpl = &(fputbl[0].registers);
  asm volatile (
    "finit\n"
    "fsave (%0)\n"
    "fwait"
    ::"r"(fputmpl)
  );

  intr_define( EXCP_NO_FPU, EXCP_ENTRY(fpu_handler), INTR_DPL_USER );	// Page Fault

  return 0;
}

