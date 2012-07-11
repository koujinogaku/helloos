/*
** intr.h --- interrupt handling
*/
#ifndef INTR_H
#define INTR_H

#include "desc.h"


/* entry definition of an interrupt call handler */
#define INTR_INTERRUPT( func )\
extern void func##_intr(void);\
asm (\
".text				\n"\
#func"_intr:                    \n"\
"       pushal                  \n"\
"       push %ds                \n"\
"       push %es                \n"\
"       push $0x08              \n"\
"       pop %es                 \n"\
"       push $0x08              \n"\
"       pop %ds                 \n"\
"       call    "#func"         \n"\
"       pop %es                 \n"\
"       pop %ds                 \n"\
"       popal                   \n"\
"       iretl                   \n")

//        ::"i"(DESC_SELECTOR(DESC_KERNEL_DATA))
//        ,"i"(DESC_SELECTOR(DESC_KERNEL_DATA))
//        )

/* get entry address of an interrupt call handler */
#define INTR_INTR_ENTRY( func )      ((unsigned int)func##_intr)

#define INTR_DPL_SYSTEM DESC_DPL_SYSTEM
#define INTR_DPL_USER   DESC_DPL_USER

void intr_init(void);
void intr_define( unsigned int n, unsigned int addr, int dpl );
int intr_regist_receiver(int irq, int queid);

#endif /* INTR_H */
