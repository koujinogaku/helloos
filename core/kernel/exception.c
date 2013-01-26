/*
** exception.c --- interrupt handling
*/

#include "config.h"
#include "console.h"
#include "intr.h"
#include "string.h"
#include "exception.h"
#include "cpu.h"
#include "task.h"
#include "syscall.h"
#include "page.h"
#include "excptdmp.h"
#include "kmem.h"

/*
static void
excp_abort( Excinfo *i, const char *msg )
{
  printk("\n*** Exception: %s ***\n"
	 "Info    eflags: %08x, errcode: %08x\n"
	 "Inst.   cs:  %08x, eip: %08x\n"
	 "Stack   ss:  %08x, esp: %08x, ebp: %08x\n"
	 "Data    ds:  %08x, es:  %08x, fs:  %08x, gs:  %08x\n"
	 "        eax: %08x, ebx: %08x, ecx: %08x, edx: %08x\n"
	 "        esi: %08x, edi: %08x\n",
	 msg,
	 i->eflags, i->errcode,
	  i->cs, i->eip,
	 i->ss, i->esp +16, i->ebp,
	  i->ds, i->es, i->fs, i->gs,
	 i->eax, i->ebx, i->ecx, i->edx,
	 i->esi, i->edi );

  printk("INSTRUCTION DUMP");
  dump((void*)(i->eip), 16);

  printk("STACK DUMP");
  stack_dump((void*)(i->esp +16), 4);

  panic("*** System Abort ***");
}
*/

/*
static int excp_checkmem(void *vaddr)
{
  if(vaddr==0)
    return 1;

  if(page_get_ppage(page_get_current_pgd(),vaddr)==0)
    return 0;
  return 1;
}
*/

/*
static void excp_dump(int dmpadr,int start,int end)
{
  int adr;
  char s[10];

  for(adr=start;adr<end;adr++) {
    if((adr-start)%16 == 0) {
      console_puts("\n");
      long2hex(dmpadr+adr,s); console_puts(s);
      console_puts(":");
    }
    if(excp_checkmem((void*)(dmpadr+adr)))
      byte2hex(*((char*)(dmpadr+adr)),s);
    else
      memcpy(s,"xx",3);
    if(adr==0)
      console_puts("[");
    else if(adr==1)
      console_puts("]");
    else
      console_puts(" ");
    console_puts(s);
  }
  console_puts("\n");
}
*/

void
excp_abort( Excinfo *i, int excpno)
{
  //int esp;
  struct kmem_except_info infobuf;
  struct kmem_except_info *info;
  unsigned long system_pgd =(unsigned long)page_get_system_pgd();
  unsigned long current_pgd =(unsigned long)page_get_current_pgd();

//  if((i->cs&0x07)==0)
//  if(task_dispatch_status()==0 || task_get_currentid()==1)
  if(system_pgd==0 || system_pgd==current_pgd)
  {/* kernel task */
    info=&infobuf;
  }
  else
  { /* user code */
    info=(void*)CFG_MEM_USERARGUMENT;
  }

  info->excpno = excpno;
  info->cr0 = (unsigned long)page_get_mode();
  info->cr2 = (unsigned long)page_get_faultvaddr();
  info->cr3 = current_pgd;
  info->taskid = task_get_currentid();
  info->lastfunc = task_get_lastfunc();
  memcpy(&(info->regs),i,sizeof(struct kmem_except_reg));

//  if((i->cs&0x07)==0)
//  if(task_dispatch_status()==0 || task_get_currentid()==1)
  if(system_pgd==0 || system_pgd==current_pgd)
  {/* kernel task */
     exception_dump(info, console_puts);
     console_puts("*** System Abort ***\n");
     for(;;) cpu_halt();
  }

  /* user code */
  cpu_unlock();
  task_exit(excpno | 0x8000); // no return;
  for(;;) ;
}

#define EXCEPTION_ABORT(func, no)\
  EXCP_EXCEPTION(func); void func( Excinfo info ){ excp_abort(&info, no); }

#define EXCEPTION_ABORT_EC(func, no)\
  EXCP_EXCEPTION_EC(func); void func( Excinfo info ){ excp_abort(&info, no); }

EXCEPTION_ABORT(excp_div, EXCP_NO_DIV);//, "Divide Error" );
EXCEPTION_ABORT(excp_deb, EXCP_NO_DEB);//, "Debug" );
EXCEPTION_ABORT(excp_nmi, EXCP_NO_NMI);//, "NMI Interrupt" );
EXCEPTION_ABORT(excp_brk, EXCP_NO_BRK);//, "Breakpoint" );
EXCEPTION_ABORT(excp_ovf, EXCP_NO_OVF);//, "Overflow" );
EXCEPTION_ABORT(excp_ran, EXCP_NO_RAN);//, "BOUND Range Exceeded" );
EXCEPTION_ABORT(excp_opc, EXCP_NO_OPC);//, "Undefined Opcode" );
EXCEPTION_ABORT(excp_fpu, EXCP_NO_FPU);//, "No Math Coprocessor" );
EXCEPTION_ABORT_EC( excp_dbl, EXCP_NO_DBL);//, "Double Fault" );
EXCEPTION_ABORT(excp_cso, EXCP_NO_CSO);//, "Coprocessor Segment Overrun" );
EXCEPTION_ABORT_EC( excp_tss, EXCP_NO_TSS);//, "Invalid TSS" );
EXCEPTION_ABORT_EC( excp_seg, EXCP_NO_SEG);//, "Segment Not Present" );
EXCEPTION_ABORT_EC( excp_stk, EXCP_NO_STK);//, "Stack-Segment Fault" );
EXCEPTION_ABORT_EC( excp_pro, EXCP_NO_PRO);//, "General Protection" );
EXCEPTION_ABORT_EC( excp_pgf, EXCP_NO_PGF);//, "Page Fault" );
EXCEPTION_ABORT(excp_res, EXCP_NO_RES);//, "Reserved" );
EXCEPTION_ABORT(excp_mth, EXCP_NO_MTH);//, "Math Fault" );
EXCEPTION_ABORT(excp_agc, EXCP_NO_AGC);//, "Alignment Check" );
EXCEPTION_ABORT(excp_mac, EXCP_NO_MAC);//, "Machine Check" );
EXCEPTION_ABORT(excp_sim, EXCP_NO_SIM);//, "Streaming SIMD Extensions" );

void
exception_init(void)
{
  /* define exception handlers */
  intr_define( EXCP_NO_DIV, EXCP_ENTRY(excp_div), INTR_DPL_SYSTEM );
  intr_define( EXCP_NO_DEB, EXCP_ENTRY(excp_deb), INTR_DPL_SYSTEM );
  intr_define( EXCP_NO_NMI, EXCP_ENTRY(excp_nmi), INTR_DPL_SYSTEM );
  intr_define( EXCP_NO_BRK, EXCP_ENTRY(excp_brk), INTR_DPL_SYSTEM );
  intr_define( EXCP_NO_OVF, EXCP_ENTRY(excp_ovf), INTR_DPL_SYSTEM );
  intr_define( EXCP_NO_RAN, EXCP_ENTRY(excp_ran), INTR_DPL_SYSTEM );
  intr_define( EXCP_NO_OPC, EXCP_ENTRY(excp_opc), INTR_DPL_SYSTEM );
  intr_define( EXCP_NO_FPU, EXCP_ENTRY(excp_fpu), INTR_DPL_SYSTEM );
  intr_define( EXCP_NO_DBL, EXCP_ENTRY_EC(excp_dbl), INTR_DPL_SYSTEM );
  intr_define( EXCP_NO_CSO, EXCP_ENTRY(excp_cso), INTR_DPL_SYSTEM );
  intr_define( EXCP_NO_TSS, EXCP_ENTRY_EC(excp_tss), INTR_DPL_SYSTEM );
  intr_define( EXCP_NO_SEG, EXCP_ENTRY_EC(excp_seg), INTR_DPL_SYSTEM );
  intr_define( EXCP_NO_STK, EXCP_ENTRY_EC(excp_stk), INTR_DPL_SYSTEM );
  intr_define( EXCP_NO_PRO, EXCP_ENTRY_EC(excp_pro), INTR_DPL_SYSTEM );
  intr_define( EXCP_NO_PGF, EXCP_ENTRY_EC(excp_pgf), INTR_DPL_SYSTEM );
  intr_define( EXCP_NO_RES, EXCP_ENTRY(excp_res), INTR_DPL_SYSTEM );
  intr_define( EXCP_NO_MTH, EXCP_ENTRY(excp_mth), INTR_DPL_SYSTEM );
  intr_define( EXCP_NO_AGC, EXCP_ENTRY(excp_agc), INTR_DPL_SYSTEM );
  intr_define( EXCP_NO_MAC, EXCP_ENTRY(excp_mac), INTR_DPL_SYSTEM );
  intr_define( EXCP_NO_SIM, EXCP_ENTRY(excp_sim), INTR_DPL_SYSTEM );
}

