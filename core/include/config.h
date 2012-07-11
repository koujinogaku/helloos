/*
** config.h --- system configuration
*/
#ifndef CONFIG_H
#define CONFIG_H

#define Asm	__asm__ __volatile__
typedef unsigned char	byte;
typedef unsigned short	word16;
typedef unsigned long	word32;
typedef int		bool;

/*#include "iplinfo.h"*/

#define CFG_MEM_SETUP		0x1000
#define CFG_MEM_PAGEWINDOW	0x1000
#define CFG_MEM_BIOSINFO	(CFG_MEM_SETUP-sizeof(struct bios_info))
#define CFG_MEM_TMPGDT		0x4000
#define CFG_MEM_TMPGDTNUM	4
#define CFG_MEM_TMPBIOSINFO	(CFG_MEM_TMPGDT+(CFG_MEM_TMPGDTNUM*8))
#define CFG_MEM_TMPVESAINFO	(CFG_MEM_TMPBIOSINFO+sizeof(struct bios_info))
#define CFG_MEM_TMPVESAMODEINFO	(CFG_MEM_TMPVESAINFO+16)
#define CFG_MEM_START		0x7c00
#define CFG_MEM_IPLINFO		480
#define CFG_MEM_KERNELSTACK	0x0000f000
#define CFG_MEM_IDTHEAD		0x0000f800 // 0x0000f800-0x0000ffff 2KB
#define CFG_MEM_IDTNUM		256
#define CFG_MEM_GDTHEAD 	0x00010000 // 0x00010000-0x0001ffff 64KB
#define CFG_MEM_GDTNUM		8192
#define CFG_MEM_KERNELHEAP2	0x00020000 // 0x00020000-0x0007ffff 512KB
#define CFG_MEM_KERNELHEAP2SZ	(CFG_MEM_VGAGRAPHICS-CFG_MEM_KERNELHEAP2)
#define CFG_MEM_VGAGRAPHICS	0x000a0000
#define CFG_MEM_VGATEXT		0x000b8000
#define CFG_MEM_VGASIZE		(0x00020000) // 64KB*2
#define CFG_MEM_KERNEL		0x00100000 // 0x00100000-0x001fffff 1MB(1M-2M)
#define CFG_MEM_KERNELHEAP	0x00200000 // 0x00200000-0x007fffff 6MB(2M-8M)
#define CFG_MEM_KERNELHEAPSZ	(CFG_MEM_KERNELMAX-CFG_MEM_KERNELHEAP) // 6MB(2M-8M)
#define CFG_MEM_KERNELMAX	0x00800000 // 0M-8M for kernel memory map
#define CFG_MEM_PAGEPOOL	(CFG_MEM_KERNELMAX) // 6MB(2M-8M)
#define CFG_MEM_USER            0x80000000 // 2G-4G
#define CFG_MEM_USERCODE        0x80000000 // 512MB

#define CFG_MEM_USERHEAP        0xA0000000 // 1GB(-2MB)
#define CFG_MEM_USERHEAPMAX     0xCE000000
#define CFG_MEM_USERHEAPSZ      (CFG_MEM_USERHEAPMAX-CFG_MEM_USERHEAP)
#define CFG_MEM_USERSTACKBOTTOM 0xCE000000 // 2MB
#define CFG_MEM_USERSTACKTOP    0xCFFFFF80
#define CFG_MEM_USERARGUMENT    0xCFFFFF80
#define CFG_MEM_USERARGUMENTSZ  (CFG_MEM_USERDATAMAX-CFG_MEM_USERARGUMENT) //128byte
#define CFG_MEM_USERDATAMAX     0xD0000000
#define CFG_MEM_VESAWINDOWSTART CFG_MEM_USERDATAMAX

/*
#define CFG_MEM_USERHEAP        0xA0000000
#define CFG_MEM_USERHEAPMAX     (CFG_MEM_USERHEAP+4096*2)
#define CFG_MEM_USERHEAPSZ      (CFG_MEM_USERHEAPMAX-CFG_MEM_USERHEAP)
#define CFG_MEM_USERSTACKBOTTOM (CFG_MEM_USERHEAPMAX)
#define CFG_MEM_USERSTACKTOP    (CFG_MEM_USERHEAPMAX+4096)
*/

struct bios_info {
  word16   vmode;     /* Video Mode */
  word16   depth;     /* num of bit in 1dot */
  word32   vram;      /* video frame physical address */
  word16   scrnx;     /* video X size */
  word16   scrny;     /* video Y size */
};

#endif /* CONFIG_H */
