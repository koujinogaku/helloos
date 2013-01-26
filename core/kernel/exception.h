/*
** exeception.h --- exception handling
*/
#ifndef EXCEPTION_H
#define EXCEPTION_H

#include <kmem.h>

typedef struct kmem_except_reg Excinfo;

#define EXCP_NO_DIV  0 // "Divide Error"
#define EXCP_NO_DEB  1 // "Debug"
#define EXCP_NO_NMI  2 // "NMI Interrupt"
#define EXCP_NO_BRK  3 // "Breakpoint"
#define EXCP_NO_OVF  4 // "Overflow"
#define EXCP_NO_RAN  5 // "BOUND Range Exceeded"
#define EXCP_NO_OPC  6 // "Undefined Opcode"
#define EXCP_NO_FPU  7 // "No Math Coprocessor"
#define EXCP_NO_DBL  8 // "Double Fault"
#define EXCP_NO_CSO  9 // "Coprocessor Segment Overrun"
#define EXCP_NO_TSS 10 // "Invalid TSS"
#define EXCP_NO_SEG 11 // "Segment Not Present"
#define EXCP_NO_STK 12 // "Stack-Segment Fault"
#define EXCP_NO_PRO 13 // "General Protection"
#define EXCP_NO_PGF 14 // "Page Fault"
#define EXCP_NO_RES 15 // "Reserved"
#define EXCP_NO_MTH 16 // "Math Fault"
#define EXCP_NO_AGC 17 // "Alignment Check"
#define EXCP_NO_MAC 18 // "Machine Check"
#define EXCP_NO_SIM 19 // "Streaming SIMD Extensions"

#define EXCP_EXCEPTION( func )\
extern void func##_excp(void);\
asm(\
".text				\n"\
#func"_excp:                    \n"\
"	pushal			\n"\
"	push	%ss		\n"\
"	push	%ds		\n"\
"	push	%es		\n"\
"	push	%fs		\n"\
"	push	%gs		\n"\
"       push $0x08              \n"\
"       pop %es                 \n"\
"       push $0x08              \n"\
"       pop %ds                 \n"\
"	call	"#func"         \n"\
"	pop	%gs		\n"\
"	pop	%fs		\n"\
"	pop	%es		\n"\
"	pop	%ds		\n"\
"	pop	%ss		\n"\
"	popal			\n"\
"	iretl			\n")
//        ::"i"(DESC_SELECTOR(DESC_KERNEL_DATA))
//        ,"i"(DESC_SELECTOR(DESC_KERNEL_DATA))
//        )

#define EXCP_EXCEPTION_EC( func )\
extern void func##_excp_ec(void);\
asm(\
".text				\n"\
#func"_excp_ec:                 \n"\
"	pushal			\n"\
"	push	%ss		\n"\
"	push	%ds		\n"\
"	push	%es		\n"\
"	push	%fs		\n"\
"	push	%gs		\n"\
"       push $0x08              \n"\
"       pop %es                 \n"\
"       push $0x08              \n"\
"       pop %ds                 \n"\
"	call	"#func"         \n"\
"	pop	%gs		\n"\
"	pop	%fs		\n"\
"	pop	%es		\n"\
"	pop	%ds		\n"\
"	pop	%ss		\n"\
"	popal			\n"\
"	addl	$0x04, %esp	\n"\
"	iret			\n")
//        ::"i"(DESC_SELECTOR(DESC_KERNEL_DATA))
//        ,"i"(DESC_SELECTOR(DESC_KERNEL_DATA))
//        )

#define EXCP_ENTRY( func )      ((unsigned int)func##_excp)
#define EXCP_ENTRY_EC( func )   ((unsigned int)func##_excp_ec)

void exception_init(void);
void excp_abort(Excinfo *i, int excpno);

#endif /* EXCEPTION_H */
