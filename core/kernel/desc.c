/*
** desc.c --- descriptor tables
*/

#include "config.h"
#include "desc.h"

void
desc_set_seg( struct desc_seg *desc, word32 base, word32 limit, unsigned int type, unsigned int dpl )
{
  desc->limitL16= limit & 0xffff;
  desc->baseL24	= base & 0xffffff;
  desc->type	= type; /* type */
  desc->s	= 1;	/* 1=code/data segment */
  desc->dpl	= dpl;  /* descriptor privilege level */
  desc->p	= 1;	/* 1=present */
  desc->limitH4	= (limit >> 16) & 0xf;
  desc->avl	= 0;	/* 0=available for use by system */
  desc->x	= 0;	/* reserved zero */
  desc->d	= 1;	/* 1=use 32 bits segment */
  desc->g	= 1;	/* 1=4KB limit scale */
  desc->baseH8	= (base >> 24) & 0xff;
}

void
desc_set_tss32( struct desc_seg *desc, word32 base, word32 limit, unsigned int dpl )
{
  desc->baseL24	= base & 0xffffff;
  desc->baseH8	= (base >> 24) & 0xff;
  desc->limitL16= limit & 0xffff;
  desc->limitH4	= (limit >> 16) & 0xf;
  desc->type	= DESC_GATE_TSS32;
  desc->dpl	= dpl;
  desc->s	= 0;	/* 0=system segment */
  desc->p	= 1;	/* 1=present */
  desc->avl	= 0;	/* 0=available for use by system */
  desc->x	= 0;	/* reserved zero */
  desc->d	= 0;	/* 0=TSS descriptor */
  desc->g	= 1;	/* 0=1B limit scale */
}

void
desc_set_gate( struct desc_gate *desc, word16 sel, word32 offset, unsigned int type, unsigned int dpl )
{
  desc->selector  = sel;
  desc->offsetL16 = offset & 0xffff;
  desc->offsetH16 = (offset >> 16) & 0xffff;
  desc->count	= 0;
  desc->type	= type;
  desc->dpl	= dpl;
  desc->s	= 0;	/* system segment or gate */
  desc->p	= 1;	/* present */
}

void
desc_init_gdtidt(void)
{
  struct desc_seg *gdt = (struct desc_seg*)CFG_MEM_GDTHEAD;
  struct desc_tblptr gdtr;
  struct desc_gate *idt = (struct desc_gate*)CFG_MEM_IDTHEAD;
  struct desc_tblptr idtr;
  int i;

  /* clear GDT */
  for(i=0;i<CFG_MEM_GDTNUM;i++)
    desc_set_seg( &gdt[i], 0, 0, 0, 0 );

  // 0: empty
  desc_set_seg( &gdt[0], 0, 0, 0, 0 );
  // 1: kernel data segment
  desc_set_seg( &gdt[DESC_KERNEL_DATA], 0, 0xfffff, DESC_TYPE_DATA | DESC_TYPE_WRITABLE, DESC_DPL_SYSTEM );
  // 2: kernel code segment
  desc_set_seg( &gdt[DESC_KERNEL_CODE], 0, 0xfffff, DESC_TYPE_CODE | DESC_TYPE_READABLE, DESC_DPL_SYSTEM );
  // 3: kernel data segment
  desc_set_seg( &gdt[DESC_KERNEL_STACK],0, 0xfffff, DESC_TYPE_DATA | DESC_TYPE_WRITABLE, DESC_DPL_SYSTEM );

  desc_set_seg( &gdt[DESC_USER_DATA], 0, 0xfffff, DESC_TYPE_DATA | DESC_TYPE_WRITABLE, DESC_DPL_USER );
  desc_set_seg( &gdt[DESC_USER_CODE], 0, 0xfffff, DESC_TYPE_CODE | DESC_TYPE_READABLE, DESC_DPL_USER );
  desc_set_seg( &gdt[DESC_USER_STACK],0, 0xfffff, DESC_TYPE_DATA | DESC_TYPE_WRITABLE, DESC_DPL_USER );

  /* load GDTR */
  gdtr.limit = sizeof(struct desc_seg)*CFG_MEM_GDTNUM -1;
  gdtr.base = CFG_MEM_GDTHEAD;
  Asm("lgdt %0"::"m"(gdtr));

  /* clear IDT */
  for(i=0;i<CFG_MEM_IDTNUM;i++)
    desc_set_gate( &idt[i], 0, 0, 0, 0 );

  /* load IDTR */
  idtr.limit = sizeof(struct desc_gate)*CFG_MEM_IDTNUM -1;
  idtr.base = CFG_MEM_IDTHEAD;
  Asm("lidt %0"::"m"(idtr));

}

