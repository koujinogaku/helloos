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
excp_abort( Excinfo *i, int excpno, const char *msg )
{

  char s[10];
  //int esp;

  console_puts("\n*** Exception: ");
  console_puts((char*)msg);
  console_puts(" *** \n");
  console_puts("Task  taskid=");
  long2hex(task_get_currentid(),s);
  console_puts(s);
  console_puts(" lastfunc=");
  long2hex(task_get_lastfunc(),s);
  console_puts(s);
  console_puts("\nInfo  eflags=");
  long2hex(i->eflags,s); console_puts(s);
  console_puts(" errcode =");
  long2hex(i->errcode,s); console_puts(s);
  if(excpno==EXCP_NO_PGF) {
    console_puts(" cr2=");
    long2hex((int)page_get_faultvaddr(),s); console_puts(s);
    console_puts(" cr3=");
    long2hex((int)page_get_current_pgd(),s); console_puts(s);
  }
  console_puts("\nInst. cs =");
  long2hex(i->cs,s); console_puts(s);
  console_puts(" eip=");
  long2hex(i->eip,s); console_puts(s);
  console_puts("\nStack ss =");
  long2hex(i->ss,s); console_puts(s);
  console_puts(" esp=");
  long2hex(i->esp,s); console_puts(s);
  console_puts(" ebp=");
  long2hex(i->ebp,s); console_puts(s);
  if((i->cs&0x07)!=0) {/* user code */
    console_puts("\nAppl  ss =");
    long2hex(i->app_ss,s); console_puts(s);
    console_puts(" esp=");
    long2hex(i->app_esp,s); console_puts(s);
  }
  console_puts("\nData  ds =");
  long2hex(i->ds,s); console_puts(s);
  console_puts(" es =");
  long2hex(i->es,s); console_puts(s);
  console_puts(" fs =");
  long2hex(i->fs,s); console_puts(s);
  console_puts(" gs =");
  long2hex(i->gs,s); console_puts(s);
  console_puts("\n      eax=");
  long2hex(i->eax,s); console_puts(s);
  console_puts(" ebx=");
  long2hex(i->ebx,s); console_puts(s);
  console_puts(" ecx=");
  long2hex(i->ecx,s); console_puts(s);
  console_puts(" edx=");
  long2hex(i->edx,s); console_puts(s);
  console_puts("\n      esi=");
  long2hex(i->esi,s); console_puts(s);
  console_puts(" edi=");
  long2hex(i->edi,s); console_puts(s);
  console_puts("\n");

/*
  console_puts("CODE  ");
  excp_dump(i->eip,-8,8);

  console_puts("STACK ");
  if((i->cs&0x07)!=0)
    esp=i->app_esp;
  else
    esp=i->esp;
  excp_dump(esp,-40,24);
*/

//  if(task_get_currentid()==1)
  if((i->cs&0x07)==0)
  {/* kernel code */
     console_puts("*** System Abort ***\n");
     for(;;) cpu_halt();
  }
  else
  { /* user code */
     console_puts("*** Exit User Process ***\n");
     cpu_unlock();
     task_exit(i->errcode);
     for(;;) ;
  }
}

#define EXCEPTION_ABORT(func, no, msg)\
  EXCP_EXCEPTION(func); void func( Excinfo info ){ excp_abort(&info, no, msg); }

#define EXCEPTION_ABORT_EC(func, no, msg)\
  EXCP_EXCEPTION_EC(func); void func( Excinfo info ){ excp_abort(&info, no, msg); }

EXCEPTION_ABORT(excp_div, EXCP_NO_DIV, "Divide Error" );
EXCEPTION_ABORT(excp_deb, EXCP_NO_DEB, "Debug" );
EXCEPTION_ABORT(excp_nmi, EXCP_NO_NMI, "NMI Interrupt" );
EXCEPTION_ABORT(excp_brk, EXCP_NO_BRK, "Breakpoint" );
EXCEPTION_ABORT(excp_ovf, EXCP_NO_OVF, "Overflow" );
EXCEPTION_ABORT(excp_ran, EXCP_NO_RAN, "BOUND Range Exceeded" );
EXCEPTION_ABORT(excp_opc, EXCP_NO_OPC, "Undefined Opcode" );
EXCEPTION_ABORT(excp_fpu, EXCP_NO_FPU, "No Math Coprocessor" );
EXCEPTION_ABORT_EC( excp_dbl, EXCP_NO_DBL, "Double Fault" );
EXCEPTION_ABORT(excp_cso, EXCP_NO_CSO, "Coprocessor Segment Overrun" );
EXCEPTION_ABORT_EC( excp_tss, EXCP_NO_TSS, "Invalid TSS" );
EXCEPTION_ABORT_EC( excp_seg, EXCP_NO_SEG, "Segment Not Present" );
EXCEPTION_ABORT_EC( excp_stk, EXCP_NO_STK, "Stack-Segment Fault" );
EXCEPTION_ABORT_EC( excp_pro, EXCP_NO_PRO, "General Protection" );
EXCEPTION_ABORT_EC( excp_pgf, EXCP_NO_PGF, "Page Fault" );
EXCEPTION_ABORT(excp_res, EXCP_NO_RES, "Reserved" );
EXCEPTION_ABORT(excp_mth, EXCP_NO_MTH, "Math Fault" );
EXCEPTION_ABORT(excp_agc, EXCP_NO_AGC, "Alignment Check" );
EXCEPTION_ABORT(excp_mac, EXCP_NO_MAC, "Machine Check" );
EXCEPTION_ABORT(excp_sim, EXCP_NO_SIM, "Streaming SIMD Extensions" );

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

