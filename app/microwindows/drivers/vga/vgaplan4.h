/*
 * Copyright (c) 1999 Greg Haerr <greg@censoft.com>
 *
 * Header file for EGA/VGA 16 color 4 planes screen driver
 * Added functions for Hercules access
 *
 */
#ifndef VGAPLANE4_H
#define VGAPLANE4_H
/*** HELLO OS ***/
#include "cpu.h"

/****************/


#define SLOWVGA		1	/* =1 for outb rather than outw instructions*/
#define HAVEBLIT	0	/* =0 to exclude blitting in vgaplan4 drivers*/
#define HWINIT		1	/* =1 for non-bios direct hardware init*/
#define ROMFONT		0	/* =0 no bios rom fonts available*/

#define EGA_BASE 	((unsigned char *)0xa0000)
#define FAR
typedef unsigned char *FARADDR;

/***** fb.h  ******/
extern int 	gr_mode;	/* temp kluge*/
#define DRAWON
#define DRAWOFF
/******************/

//#define	inportb(P)		cpu_in8(P)
//#define	outportb(P,V)		cpu_out8(P,V)
#define outb(val,port)	cpu_out8(port,val)
//#define outw(val,port)	outport(port,val)
#define	inb(port)		cpu_in8(port)

#if NOTUSED
#define HAVEFARPTR	1
#define FAR
#define HAVEIOPERM	1	/* has ioperm() system call*/
#define outb(val,port)	outportb(port,val)
#define outw(val,port)	outport(port,val)
#endif

/* get byte at address*/
#define GETBYTE_FP(addr)	(*(FARADDR)(addr))
/* put byte at address*/
#define PUTBYTE_FP(addr,val)	((*(FARADDR)(addr)) = (val))
/* read-modify-write at address*/
#define RMW_FP(addr)		((*(FARADDR)(addr)) |= 1)
/* or byte at address*/
#define ORBYTE_FP(addr,val)	((*(FARADDR)(addr)) |= (val))
/* and byte at address*/
#define ANDBYTE_FP(addr,val)	((*(FARADDR)(addr)) &= (val))


/* vgaplan4.c portable C, asmplan4.s asm, or ELKS asm elkplan4.c driver*/
int		ega_init(PSD psd);
/*
void 		ega_drawpixel(PSD psd,unsigned int x,unsigned int y,
			MWPIXELVAL c);
MWPIXELVAL 	ega_readpixel(PSD psd,unsigned int x,unsigned int y);
void		ega_drawhorzline(PSD psd,unsigned int x1,unsigned int x2,
			unsigned int y,MWPIXELVAL c);
void		ega_drawvertline(PSD psd,unsigned int x,unsigned int y1,
			unsigned int y2, MWPIXELVAL c);
*/
void 		ega_drawpixel(PSD psd,MWCOORD x,MWCOORD y,
			MWPIXELVAL c);
MWPIXELVAL 	ega_readpixel(PSD psd,MWCOORD x,MWCOORD y);
void		ega_drawhorzline(PSD psd,MWCOORD x1,MWCOORD x2,
			MWCOORD y,MWPIXELVAL c);
void		ega_drawvertline(PSD psd,MWCOORD x,MWCOORD y1,
			MWCOORD y2, MWPIXELVAL c);

#if HAVEBLIT
/* memplan4.c*/
void	 	ega_blit(PSD dstpsd, MWCOORD dstx, MWCOORD dsty, MWCOORD w,
			MWCOORD h,PSD srcpsd,MWCOORD srcx,MWCOORD srcy,long op);
#endif

/* vgainit.c - direct hw init*/
void		ega_hwinit(void);
void		ega_hwterm(void);

#if SLOWVGA
/* use outb rather than outw instructions for older, slower VGA's*/

/* Program the Set/Reset Register for drawing in color COLOR for write
   mode 0. */
#define set_color(c)		{ outb (0, 0x3ce); outb (c, 0x3cf); }

/* Set the Enable Set/Reset Register. */
#define set_enable_sr(mask) { outb (1, 0x3ce); outb (mask, 0x3cf); }

/* Select the Bit Mask Register on the Graphics Controller. */
#define select_mask() 		{ outb (8, 0x3ce); }

/* Program the Bit Mask Register to affect only the pixels selected in
   MASK.  The Bit Mask Register must already have been selected with
   select_mask (). */
#define set_mask(mask)		{ outb (mask, 0x3cf); }

#define select_and_set_mask(mask) { outb (8, 0x3ce); outb (mask, 0x3cf); }

/* Set the Data Rotate Register.  Bits 0-2 are rotate count, bits 3-4
   are logical operation (0=NOP, 1=AND, 2=OR, 3=XOR). */
#define set_op(op) 		{ outb (3, 0x3ce); outb (op, 0x3cf); }

/* Set the Memory Plane Write Enable register. */
#define set_write_planes(mask) { outb (2, 0x3c4); outb (mask, 0x3c5); }

/* Set the Read Map Select register. */
#define set_read_plane(plane)	{ outb (4, 0x3ce); outb (plane, 0x3cf); }

/* Set the Graphics Mode Register.  The write mode is in bits 0-1, the
   read mode is in bit 3. */
#define set_mode(mode) 		{ outb (5, 0x3ce); outb (mode, 0x3cf); }

#else /* !SLOWVGA*/
/* use outw rather than outb instructions for new VGAs*/

/* Program the Set/Reset Register for drawing in color COLOR for write
   mode 0. */
#define set_color(c)		{ outw ((c)<<8, 0x3ce); }

/* Set the Enable Set/Reset Register. */
#define set_enable_sr(mask) 	{ outw (1|((mask)<<8), 0x3ce); }

/* Select the Bit Mask Register on the Graphics Controller. */
#define select_mask() 		{ outb (8, 0x3ce); }

/* Program the Bit Mask Register to affect only the pixels selected in
   MASK.  The Bit Mask Register must already have been selected with
   select_mask (). */
#define set_mask(mask)		{ outb (mask, 0x3cf); }

#define select_and_set_mask(mask) { outw (8|((mask)<<8), 0x3ce); }

/* Set the Data Rotate Register.  Bits 0-2 are rotate count, bits 3-4
   are logical operation (0=NOP, 1=AND, 2=OR, 3=XOR). */
#define set_op(op) 		{ outw (3|((op)<<8), 0x3ce); }

/* Set the Memory Plane Write Enable register. */
#define set_write_planes(mask) { outw (2|((mask)<<8), 0x3c4); }

/* Set the Read Map Select register. */
#define set_read_plane(plane)	{ outw (4|((plane)<<8), 0x3ce); }

/* Set the Graphics Mode Register.  The write mode is in bits 0-1, the
   read mode is in bit 3. */
#define set_mode(mode) 		{ outw (5|((mode)<<8), 0x3ce); }

#endif /* SLOWVGA*/


#endif /* ifndef VGAPLANE4_H */
