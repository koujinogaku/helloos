#include <string.h>
#include <kmem.h>
#include "../kernel/exception.h"

#define PAGE_ERR_PROTECT  0x01
#define PAGE_ERR_WRITE    0x02
#define PAGE_ERR_USER     0x04
#define PAGE_TYPE_PRESENT 0x01
#define PAGE_TYPE_RDONLY  0x00
#define PAGE_TYPE_RDWR    0x02
#define PAGE_TYPE_SUPER   0x00
#define PAGE_TYPE_USER    0x04

static char *exception_name[] = {
  "Divide Error",                // EXCP_NO_DIV  0
  "Debug",                       // EXCP_NO_DEB  1
  "NMI Interrupt",               // EXCP_NO_NMI  2
  "Breakpoint",                  // EXCP_NO_BRK  3
  "Overflow",                    // EXCP_NO_OVF  4
  "BOUND Range Exceeded",        // EXCP_NO_RAN  5
  "Undefined Opcode",            // EXCP_NO_OPC  6
  "No Math Coprocessor",         // EXCP_NO_FPU  7
  "Double Fault",                // EXCP_NO_DBL  8
  "Coprocessor Segment Overrun", // EXCP_NO_CSO  9
  "Invalid TSS",                 // EXCP_NO_TSS 10
  "Segment Not Present",         // EXCP_NO_SEG 11
  "Stack-Segment Fault",         // EXCP_NO_STK 12
  "General Protection",          // EXCP_NO_PRO 13
  "Page Fault",                  // EXCP_NO_PGF 14
  "Reserved",                    // EXCP_NO_RES 15
  "Math Fault",                  // EXCP_NO_MTH 16
  "Alignment Check",             // EXCP_NO_AGC 17
  "Machine Check",               // EXCP_NO_MAC 18
  "Streaming SIMD Extensions",   // EXCP_NO_SIM 19
};


void exception_dump(void *infov, int(*call_puts)(char *))
{
  struct kmem_except_info *info=infov;
  char s[16];

  call_puts("\n*** Exception: ");
  call_puts(exception_name[info->excpno]);
  call_puts(" *** \n");

  if(info->excpno==EXCP_NO_PGF && info->regs.errcode&PAGE_ERR_PROTECT) {
    call_puts("*** page is ");
    if(info->regs.errcode&PAGE_ERR_WRITE)
      call_puts("write");
    else
      call_puts("read");
    call_puts("-protected running on ");
    if(info->regs.errcode&PAGE_ERR_USER)
      call_puts("user");
    else
      call_puts("supervisor");
    call_puts("-mode ***\n");

    call_puts("** fault address is ");
    long2hex(info->cr2,s); call_puts(s);
    call_puts(" ***\n");
  }

  call_puts("Task  taskid=");
  long2hex(info->taskid,s);
  call_puts(s);
  call_puts(" lastfunc=");
  long2hex(info->lastfunc,s);
  call_puts(s);
  call_puts(" cr0=");
  long2hex(info->cr0,s);
  call_puts(s);

  call_puts("\nInfo  eflags=");
  long2hex(info->regs.eflags,s); call_puts(s);
  call_puts(" errcode =");
  long2hex(info->regs.errcode,s); call_puts(s);
  call_puts(" cr2=");
  long2hex(info->cr2,s); call_puts(s);
  call_puts(" cr3=");
  long2hex(info->cr3,s); call_puts(s);
  call_puts("\nInst. cs =");
  long2hex(info->regs.cs,s); call_puts(s);
  call_puts(" eip=");
  long2hex(info->regs.eip,s); call_puts(s);
  call_puts("\nStack ss =");
  long2hex(info->regs.ss,s); call_puts(s);
  call_puts(" esp=");
  long2hex(info->regs.esp,s); call_puts(s);
  call_puts(" ebp=");
  long2hex(info->regs.ebp,s); call_puts(s);
  if((info->regs.cs&0x07)!=0) {/* user code */
    call_puts("\nAppl  ss =");
    long2hex(info->regs.app_ss,s); call_puts(s);
    call_puts(" esp=");
    long2hex(info->regs.app_esp,s); call_puts(s);
  }
  call_puts("\nData  ds =");
  long2hex(info->regs.ds,s); call_puts(s);
  call_puts(" es =");
  long2hex(info->regs.es,s); call_puts(s);
  call_puts(" fs =");
  long2hex(info->regs.fs,s); call_puts(s);
  call_puts(" gs =");
  long2hex(info->regs.gs,s); call_puts(s);
  call_puts("\n      eax=");
  long2hex(info->regs.eax,s); call_puts(s);
  call_puts(" ebx=");
  long2hex(info->regs.ebx,s); call_puts(s);
  call_puts(" ecx=");
  long2hex(info->regs.ecx,s); call_puts(s);
  call_puts(" edx=");
  long2hex(info->regs.edx,s); call_puts(s);
  call_puts("\n      esi=");
  long2hex(info->regs.esi,s); call_puts(s);
  call_puts(" edi=");
  long2hex(info->regs.edi,s); call_puts(s);
  call_puts("\n");

/*
  call_puts("CODE  ");
  excp_dump(i->eip,-8,8);

  call_puts("STACK ");
  if((i->cs&0x07)!=0)
    esp=i->app_esp;
  else
    esp=i->esp;
  excp_dump(esp,-40,24);
*/

}
