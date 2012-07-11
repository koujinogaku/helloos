/*
** desc.h --- descriptor tables
*/
#ifndef DESC_H
#define DESC_H

/* code/data descriptor types */
enum {
  DESC_TYPE_DATA	= 0x00,
  DESC_TYPE_WRITABLE	= 0x02,
  DESC_TYPE_EXPAND	= 0x04,

  DESC_TYPE_CODE	= 0x08,
  DESC_TYPE_READABLE	= 0x02,
  DESC_TYPE_CONFORM	= 0x04,

  DESC_TYPE_ACCESSED	= 0x01,
};

/* system/gate descriptor types */
enum {
  DESC_GATE_TSS16	= 1,
  DESC_GATE_LDT		= 2,
  DESC_GATE_TSS16_BUSY	= 3,
  DESC_GATE_CALL16	= 4,
  DESC_GATE_INTR16	= 6,
  DESC_GATE_TRAP16	= 7,
  DESC_GATE_TSS32	= 9,
  DESC_GATE_TSS32_BUSY	= 11,
  DESC_GATE_CALL32	= 12,
  DESC_GATE_INTR32	= 14,
  DESC_GATE_TRAP32	= 15,
};

/* descriptor privilege level */
enum {
  DESC_DPL_SYSTEM	= 0,
  DESC_DPL_USER		= 3,
};

/* descriptors for kernel */
enum {
  DESC_KERNEL_DATA	= 1,
  DESC_KERNEL_CODE	= 2,
  DESC_KERNEL_STACK	= 3,
  DESC_KERNEL_TASK	= 4,
  DESC_KERNEL_TASKBAK	= 5,
  DESC_USER_DATA	= 6,
  DESC_USER_CODE	= 7,
  DESC_USER_STACK	= 8,
};

#define DESC_SELECTOR(x) ((x)*8)

/* code/data segment descriptor table */
struct desc_seg {
  unsigned limitL16:16;	/* limit 15:0 */
  unsigned baseL24:24 __attribute__((packed)) ;	/* base 23:0 */
  unsigned type:4;	/* type */
  unsigned s:1;		/* descriptor type (1 for code/data segment) */
  unsigned dpl:2;	/* descriptor privilege level */
  unsigned p:1;		/* present bit */
  unsigned limitH4:4;	/* limit 19:16 */
  unsigned avl:1;	/* available for use by system software */
  unsigned x:1;		/* reserved zero */
  unsigned d:1;		/* default operation size 16/32 bits segment */
  unsigned g:1;		/* granularity (limit scale) bytes/4KB */
  unsigned baseH8:8;	/* base 31:24 */
};

/* system segment or gate descriptor table */
struct desc_gate {
  unsigned offsetL16:16;/* offset 15:0 */
  unsigned selector:16;	/* selector */
  unsigned count:8;	/* count for stack switing */
  unsigned type:4;	/* type */
  unsigned s:1;		/* descriptor type (0 for system segment) */
  unsigned dpl:2;	/* descriptor privilege level */
  unsigned p:1;		/* present bit */
  unsigned offsetH16:16;/* offset 31:16 */
};

/* descriptor table pointer */
struct desc_tblptr {
  unsigned limit:16;
  unsigned base:32 __attribute__((packed)) ;
};

void desc_set_seg(   struct desc_seg  *desc, word32 base, word32 limit,  unsigned int type, unsigned int dpl );
void desc_set_tss32( struct desc_seg  *desc, word32 base, word32 limit,  unsigned int dpl );
void desc_set_gate(  struct desc_gate *desc, word16 sel,  word32 offset, unsigned int type, unsigned int dpl );
void desc_init_gdtidt(void);

#endif /* DESC_H */
