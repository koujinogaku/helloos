/*
 * Copyright (c) 1999, 2000 Greg Haerr <greg@censoft.com>
 *
 * 16 color 4 planes EGA/VGA Planar Video Driver for Microwindows
 * Portable C version
 *
 * Based on BOGL - Ben's Own Graphics Library.
 *   Written by Ben Pfaff <pfaffben@debian.org>.
 *	 BOGL is licensed under the terms of the GNU General Public License
 *
 * In this driver, psd->linelen is line byte length, not line pixel length
 *
 * This file is meant to compile under Linux, ELKS, and MSDOS
 * without changes.  Please try to keep it that way.
 * 
 */

#include "portunixstd.h"


#include "device.h"
#include "vgaplan4.h"
/*#include "fb.h"*/

#if 1
/* assumptions for speed: NOTE: psd is ignored in these routines*/
#define SCREENBASE 		EGA_BASE
#define BYTESPERLINE		80

#else
/* run on top of framebuffer*/
#define SCREENBASE 		((char *)psd->addr)
#define BYTESPERLINE		(psd->linelen)
#endif

/* Values for the data rotate register to implement drawing modes. */
static unsigned char mode_table[MWMODE_MAX + 1] = {
  0x00, 0x18, 0x10, 0x08,	/* COPY, XOR, AND, OR implemented*/
  0x00, 0x00, 0x00, 0x00,	/* no VGA HW for other modes*/
  0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00,
};

/* precalculated mask bits*/
static unsigned char mask[8] = {
	0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01
};


/* Init VGA controller, calc linelen and mmap size, return 0 on fail*/
int
ega_init(PSD psd)
{
#if 1
	/* fill in screendevice struct if not framebuffer driver*/
	psd->addr = (void *)SCREENBASE;		/* long ptr -> short on 16bit sys*/
	psd->linelen = BYTESPERLINE;
#endif
	/* framebuffer mmap size*/
	psd->size = 0x10000;
	/* Set up some default values for the VGA Graphics Registers. */
	set_enable_sr (0x0f);
	set_op (0);
	set_mode (0);
	return 1;
}

/* draw a pixel at x,y of color c*/
void
ega_drawpixel(PSD psd,MWCOORD x, MWCOORD y, MWPIXELVAL c)
{
	assert (x >= 0 && x < psd->xres);
	assert (y >= 0 && y < psd->yres);
	assert (c < psd->ncolors);
  
	DRAWON;
	set_op(mode_table[gr_mode]);
	set_color (c);
	select_and_set_mask (mask[x&7]);
	RMW_FP ((FARADDR)SCREENBASE + (x>>3) + y * BYTESPERLINE);
	DRAWOFF;
}

/* Return 4-bit pixel value at x,y*/
MWPIXELVAL
ega_readpixel(PSD psd,MWCOORD x,MWCOORD y)
{
	FARADDR		src;
	int		plane;
	MWPIXELVAL	c = 0;
	
	assert (x >= 0 && x < psd->xres);
	assert (y >= 0 && y < psd->yres);
  
	DRAWON;

	src = (FARADDR)(SCREENBASE + (x>>3) + y * BYTESPERLINE);

	for(plane=0; plane<4; ++plane) {
		set_read_plane(plane);
		if(GETBYTE_FP(src) & mask[x&7])
			c |= 1 << plane;
	}
	DRAWOFF;
	return c;
}

/* Draw horizontal line from x1,y to x2,y including final point*/
void
ega_drawhorzline(PSD psd, MWCOORD x1, MWCOORD x2, MWCOORD y,
	MWPIXELVAL c)
{
	FARADDR dst, last;

	assert (x1 >= 0 && x1 < psd->xres);
	assert (x2 >= 0 && x2 < psd->xres);
	assert (x2 >= x1);
	assert (y >= 0 && y < psd->yres);
	assert (c < psd->ncolors);

	DRAWON;
	set_color (c);
	set_op(mode_table[gr_mode]);
	/*
	* The following fast drawhline code is buggy for XOR drawing,
	* for some reason.  So, we use the equivalent slower drawpixel
	* method when not drawing MWMODE_COPY.
	*/
	if(gr_mode == MWMODE_COPY) {
		dst = (FARADDR)(SCREENBASE + (x1>>3) + y * BYTESPERLINE);
		if ((x1>>3) == (x2>>3)) {
			select_and_set_mask ((0xff >> (x1 & 7)) & (0xff << (7 - (x2 & 7))));
			RMW_FP (dst);
		} else {
			select_and_set_mask (0xff >> (x1 & 7));
			RMW_FP (dst++);

			set_mask (0xff);
			last = (FARADDR)(SCREENBASE) + (x2>>3) + y * BYTESPERLINE;
			while (dst < last)
				PUTBYTE_FP(dst++, 1);

			set_mask (0xff << (7 - (x2 & 7)));
			RMW_FP (dst);
		}
	} else {
		/* slower method, draw pixel by pixel*/
		select_mask ();
		while(x1 <= x2) {
			set_mask (mask[x1&7]);
			RMW_FP ((FARADDR)SCREENBASE + (x1++>>3) + y * BYTESPERLINE);
		}
	}
	DRAWOFF;
}

/* Draw a vertical line from x,y1 to x,y2 including final point*/
void
ega_drawvertline(PSD psd,MWCOORD x, MWCOORD y1, MWCOORD y2,
	MWPIXELVAL c)
{
	FARADDR dst, last;

	assert (x >= 0 && x < psd->xres);
	assert (y1 >= 0 && y1 < psd->yres);
	assert (y2 >= 0 && y2 < psd->yres);
	assert (y2 >= y1);
	assert (c < psd->ncolors);

	DRAWON;
	set_op(mode_table[gr_mode]);
	set_color (c);
	select_and_set_mask (mask[x&7]);
	dst = SCREENBASE + (x>>3) + y1 * BYTESPERLINE;
	last = SCREENBASE + (x>>3) + y2 * BYTESPERLINE;
	while (dst <= last) {
		RMW_FP (dst);
		dst += BYTESPERLINE;
	}
	DRAWOFF;
}

#if FBVGA
SUBDRIVER vgaplan4 = {
	(void *)ega_init,
	(void *)ega_drawpixel,
	(void *)ega_readpixel,
	(void *)ega_drawhorzline,
	(void *)ega_drawvertline,
	(void *)gen_fillrect,
	(void *)ega_blit
};
#endif /* FBVGA*/
