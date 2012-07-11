/*
** setup.c --- kernel loader
*/

/* startup code */
asm(
".text				\n"
".code16gcc			\n"
"	cli			\n"
"	mov	%cs, %ax	\n"
"	mov	%ax, %ds	\n"
"	mov	%ax, %es	\n"
"	mov	%ax, %ss	\n"
"	mov 	$0x0f000, %sp	\n" //(CFG_MEM_KERNELSTACK));
"	sti			\n"
"	call	setup_main	\n"
"dead:	jmp	dead		\n"
);

#include "config.h"
#include "iplinfo.h"
#include "desc.h"
#include "vesa.h"

#define PIC0_IMR		0x0021
#define PIC1_IMR		0x00a1

#define IPL_SETUPBUFFSZ 256
static unsigned char setup_buff[IPL_SETUPBUFFSZ+1];

/* cpu contorols */
/* these are copied from "cpu.h" */
inline byte
cpu_in8( unsigned int port )
{
  byte ret;
  asm volatile("inb %%dx, %%al": "=a"(ret): "d"(port));
  return ret;
}
inline void
cpu_out8( unsigned int port, byte data )
{
  asm volatile("outb %%al, %%dx":: "a"(data), "d"(port));
}
inline void
cpu_lock(void)
{
  asm volatile("cli");
}
inline void
cpu_disable_nmi(void)
{
  cpu_out8(0x70, 0x80);
}

/* coping memory blocks */
void *
memcpy( void *dest, const void *src, unsigned int n )
{
  char *d = (char*)dest;
  const char *s = (const char*)src;

  while( n-- )
    *d++ = *s++;

  return dest;
}

void *
memset( void *s, int c, unsigned int n )
{
  char *p = (char*)s;

  while( n-- )
    *p++ = (char)c;

  return s;
}

/* set descriptor to GDT */
/* this is copyed from "desc.c" */
void
desc_set_seg( struct desc_seg *desc, word32 base, word32 limit, unsigned int type, unsigned int dpl )
{
  desc->baseL24	= base & 0xffffff;
  desc->baseH8	= (base >> 24) & 0xff;
  desc->limitL16= limit & 0xffff;
  desc->limitH4	= (limit >> 16) & 0xf;
  desc->type	= type; /* type */
  desc->dpl	= dpl;  /* descriptor privilege level */
  desc->s	= 1;	/* 1=code/data segment */
  desc->p	= 1;	/* 1=present */
  desc->avl	= 0;	/* 0=available for use by system */
  desc->x	= 0;	/* reserved zero */
  desc->d	= 1;	/* 1=use 32 bits segment */
  desc->g	= 1;	/* 1=4KB limit scale */
}

/* setup GDT for temporary */
void
setup_set_tmpgdt(void)
{
  struct desc_seg *gdt = (struct desc_seg*)CFG_MEM_TMPGDT;
  struct desc_tblptr gdtr;

  /* clear GDT */
  memset( gdt, 0, sizeof(struct desc_seg)*CFG_MEM_TMPGDTNUM );

  // 0: empty
  desc_set_seg( &gdt[0], 0, 0, 0, 0 );
  // 1: kernel data segment
  desc_set_seg( &gdt[DESC_KERNEL_DATA], 0, 0xfffff, DESC_TYPE_DATA | DESC_TYPE_WRITABLE, DESC_DPL_SYSTEM );
  // 2: kernel code segment
  desc_set_seg( &gdt[DESC_KERNEL_CODE], 0, 0xfffff, DESC_TYPE_CODE | DESC_TYPE_READABLE, DESC_DPL_SYSTEM );
  // 3: kernel stack segment
  desc_set_seg( &gdt[DESC_KERNEL_STACK],0, 0xfffff, DESC_TYPE_DATA | DESC_TYPE_WRITABLE, DESC_DPL_SYSTEM );

  /* load GDTR */
  gdtr.limit = sizeof(struct desc_seg)*CFG_MEM_TMPGDTNUM -1;
  gdtr.base = CFG_MEM_TMPGDT;
  asm volatile("lgdt %0"::"m"(gdtr));
}

/* display a string using the BIOS */
static void
bios_puts(const char *str)
{
  char c;
  while((c = *str++))
  asm volatile("int $0x10":: "a"(0x0e00 | (0xff & c)), "b"(7));
}
#if 0
/* get 64KB memory size */
inline word16
bios_get_memsize(void)
{
  word16 ksize,size;
  struct bios_info *biosinfo;
  biosinfo = (void*)CFG_MEM_BIOSINFO;

  asm volatile("int $0x15": "=d"(size),"=c"(ksize):"c"(0),"d"(0),"a"(0xe801));
  if(size!=0 || ksize!=0) {
    if(size!=0) {
      size += (256+4);   // Correction 16MB
  biosinfo->xmemsize = size*64;
    }
    else {
      ksize += (1024+256); // Correction 1MB
      size = ksize / 64;   // translate 64KB
    }
  }
  else {
    asm volatile("int $0x15": "=a"(ksize): "a"(0x8800));
    ksize += 1024;      // Correction 1MB
    size = ksize / 64;  // translate 64KB
  }
  
  return size;        // 64KB
}
#endif

inline int
bios_set_vesamode(int mode)
{
    struct bios_info *binfo = (void *)CFG_MEM_TMPBIOSINFO;
    word32 ret;
    unsigned int wd,ht,depth;
    unsigned long vram;

    asm volatile( 
                 "mov %%ax,    %%di \n"
                 "mov $0x00,   %%ax \n"
                 "mov %%ax,    %%es \n"
                 "mov $0x4f00, %%ax \n"  /* VESA INFO */
                 "int $0x10 \n" 
                 :"=a"(ret):"a"(CFG_MEM_TMPVESAINFO)
                 );
    if(ret!=0x004f) {
      return ret;
    }
    if(((vbe_info_t *)CFG_MEM_TMPVESAINFO)->ver_major < 2) {
      return 1;
    }


    asm volatile(
                 "mov %%ax,    %%di \n"
                 "mov $0x00,   %%ax \n"
                 "mov %%ax,    %%es \n"
                 "mov $0x4f01, %%ax \n"  /* MODE INFO */
                 "int $0x10 \n"
                 :"=a"(ret):"a"(CFG_MEM_TMPVESAMODEINFO),
                 "c"(mode)
                 );
    if(ret!=0x004f) {
      return ret;
    }
    // Linear Frame Buffer(b7) & Graphics(b4) Supported(b0)
    if(((((mode_info_t *)CFG_MEM_TMPVESAMODEINFO)->mode_attrib)&0x89)!=0x89) {
      return ((mode_info_t *)CFG_MEM_TMPVESAMODEINFO)->mode_attrib;
    }
    // depth = 8bit
    if(((mode_info_t *)CFG_MEM_TMPVESAMODEINFO)->depth != 8) {
      return 3;
    }
    // memory_model = Packed pixel(4)
    if(((mode_info_t *)CFG_MEM_TMPVESAMODEINFO)->memory_model != 4) {
      return 4;
    }
    
    wd    = ((mode_info_t *)CFG_MEM_TMPVESAMODEINFO)->wd;
    ht    = ((mode_info_t *)CFG_MEM_TMPVESAMODEINFO)->ht;
    depth = ((mode_info_t *)CFG_MEM_TMPVESAMODEINFO)->depth;
    vram  = ((mode_info_t *)CFG_MEM_TMPVESAMODEINFO)->lfb_adr;
    binfo->vmode = mode;
    binfo->scrnx = wd;
    binfo->scrny = ht;
    binfo->depth = depth;
    binfo->vram  = vram;
    mode |= 0x4000;
    asm volatile(
                 "mov %%ax,    %%di \n"
                 "mov $0x00,   %%ax \n"
                 "mov %%ax,    %%es \n"
                 "mov $0x4f02, %%ax \n"  /* MODE INFO */
                 "int $0x10 \n"
                 :"=a"(ret):"a"(CFG_MEM_TMPVESAMODEINFO),
                 "b"(mode)
                 );
    if(ret!=0x004f) {
      return ret;
    }

    return 0;
}
/* set up video mode */
// display_vmode
// VGA_MODE_TEXT80x25         1
// VGA_MODE_GRAPH640x480x16   2
// VGA_MODE_GRAPH320x200x256  3
inline void
bios_set_videomode(int mode)
{
  int rc;
  struct bios_info *binfo = (void *)CFG_MEM_TMPBIOSINFO;
  if(mode==1) {
    /* TEXT MODE */
    asm volatile("int $0x10":: "a"(0x0003));
    binfo->vmode=1; //VGA_MODE_TEXT80x25
    binfo->depth=16;
    binfo->vram=CFG_MEM_VGATEXT;
    binfo->scrnx=80;
    binfo->scrny=25;
  }
  else if(mode==3) {
    // 320x200x8bit
    asm volatile("int $0x10":: "a"(0x0013));
    binfo->vmode=3;  //VGA_MODE_GRAPH320x200x256
    binfo->depth=8;
    binfo->vram=CFG_MEM_VGAGRAPHICS;
    binfo->scrnx=320;
    binfo->scrny=200;
  }
  else {
    rc=bios_set_vesamode(mode);
    if(rc) {
      /* if error then text mode */
      asm volatile("int $0x10":: "a"(0x0003));
      binfo->vmode=1; //VGA_MODE_TEXT80x25
      binfo->depth=rc;
      binfo->vram=CFG_MEM_VGATEXT;
      binfo->scrnx=80;
      binfo->scrny=25;
    }
  }
}
/* copy to extend memory area (16MB limits) */
inline int
bios_copy_emb( unsigned long dest, unsigned long src, unsigned long count )
{
  struct desc_seg desc[6];
  int i, ret;

  for( i = 0 ; i < sizeof(desc) ; i++ )
    ((byte*)desc)[i] = 0;

  desc_set_seg( &desc[2], src,  0xffff, DESC_TYPE_DATA | DESC_TYPE_WRITABLE, DESC_DPL_SYSTEM );
  desc_set_seg( &desc[3], dest, 0xffff, DESC_TYPE_DATA | DESC_TYPE_WRITABLE, DESC_DPL_SYSTEM );
  Asm("int $0x15":"=a"(ret):"a"(0x8700),"c"(count),"S"(desc));
  return (ret>>8);
}

/* reset floppy drive */
static inline word32
bios_reset_drive( byte drive )
{
  int ret;
  asm volatile("int $0x13": "=a"(ret): "a"(0), "d"(drive));
  return ((ret >> 8) & 0xff);
}

/* read floppy drive */
static inline word32
bios_read_drive( byte drive, void* buf, word32 sec )
{
  word32 ret;
  asm volatile("int $0x13": "=a"(ret):
      "a"(0x201),
      "b"(buf),
      "c"( ( (sec/FDD_SECPTRK/2)  << 8) | (sec%FDD_SECPTRK+1) ),
      "d"( (((sec/FDD_SECPTRK)%2) << 8) | drive)			);
  return ((ret >> 8) & 0xff);
}

/*  load kernel */
int
setup_load_kernel(unsigned int kernel_head, unsigned int kernel_count)
{
  unsigned int i;

  for( i = 0 ; i < kernel_count ; i++)
  {
    bios_puts(".");

    if( bios_read_drive(FDD_BOOTDRV, setup_buff, kernel_head + i) ) {
      bios_puts("read error!");
      return -1;
    }
    
    if( bios_copy_emb( CFG_MEM_KERNEL + i * FDD_SECSIZE,
	      (unsigned long)setup_buff, FDD_SECSIZE ) ) {
      bios_puts("emb copy error!");
      return -1;
    }
  }
  return 0;
}

/* stop fdd motor */
inline void
setup_stop_floppy_motor(void)
{
  cpu_out8( 0x3f2, 0x0c );
  cpu_in8( 0x80 );
}

inline void
setup_enable_a20(void)
{
  while(cpu_in8(0x64) & 2);
  cpu_out8(0x64, 0xd1);
  while(cpu_in8(0x64) & 2);
  cpu_out8(0x60, 0xdf);
  while(cpu_in8(0x64) & 2);
  cpu_out8(0x64, 0xff);
  while(cpu_in8(0x64) & 2);
}

/* setup main */
word32
setup_main(void)
{
  static struct ipl_info iplinfo;

  memcpy(&iplinfo,(void*)(CFG_MEM_START+CFG_MEM_IPLINFO),sizeof(struct ipl_info));

  bios_puts("Loading:");

  /* load setup program */
  if( bios_reset_drive( FDD_BOOTDRV ) ){
    goto load_error;
  }

  if( setup_load_kernel(iplinfo.kernel_head,iplinfo.kernel_count) ) {
    goto load_error;
  }

  bios_puts("ok\n\r");

  setup_stop_floppy_motor();

  // text=1 , vga320x200x8=3, vesa640x480x8=0x101, vesa800x600x8=0x103, vesa1024x768x8=0x105
  bios_set_videomode(0x101);

  setup_enable_a20();

  /* disable all interrupts */
  cpu_out8(PIC0_IMR, 0xff);
  cpu_out8(PIC1_IMR, 0xff);
  cpu_disable_nmi();
  cpu_lock();

  setup_set_tmpgdt(); /* Make the Provisional GDT */

  /* turn on PE(Protection Enable) bit */
  asm("	mov	%cr0, %eax	\n"
      "	and	$0x7fffffff, %eax	\n" /* disable paging */
      "	or	$1, %eax	\n" /* protect mode */
      "	mov	%eax, %cr0	\n"
      /* flush out the pipeline */
      "	jmp	1f		\n"
      "1:			\n");
  asm volatile("mov %0,    %%eax \n"  /* mov $DESC_KERNEL_DATASEL, %eax */
               "mov %%eax, %%ds  \n"
               "mov %%eax, %%es  \n"
               "mov %%eax, %%fs  \n"
               "mov %%eax, %%gs  \n"
               "mov %1,    %%eax \n"  /* mov $DESC_KERNEL_STACKSEL, %eax */
               "mov %%eax, %%ss  \n"
               "mov %2,    %%esp \n" /* (CFG_MEM_KERNELSTACK) */
               ::"i"(DESC_SELECTOR(DESC_KERNEL_DATA)), 
                 "i"(DESC_SELECTOR(DESC_KERNEL_STACK)), 
                 "i"(CFG_MEM_KERNELSTACK)
               );

  /* activate kernel */
  asm volatile("ljmpl %0, %1"::"i"(DESC_SELECTOR(DESC_KERNEL_CODE)),"i"(CFG_MEM_KERNEL));

 load_error:
  bios_puts("error");
  return 0;
}

