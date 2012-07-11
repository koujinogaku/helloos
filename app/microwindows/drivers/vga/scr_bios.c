/*
 * Copyright (c) 1999, 2000 Greg Haerr <greg@censoft.com>
 *
 * EGA/VGA 16 color 4 planes Screen Driver, direct hardware or bios
 * 	If HWINIT 1 is #defined, the file vgainit.c is
 * 	used to provide non-bios direct hw initialization of the VGA
 * 	chipset.  Otherwise, this driver uses int10 bios to 
 * 	set/reset graphics/text modes.
 *
 * 	If ROMFONT is #defined, the file romfont.c is used to
 * 	draw all fonts, afte the bios is called to
 * 	get the address of the ROM character font which
 * 	is used for the character bitmaps.  Otherwise, the
 * 	file genfont.c is used to draw linked in fonts, without
 * 	consulting the bios.
 *
 * 	All other access to the hardware is controlled through this driver.
 *
 * 	Blitting enabled with #define HAVEBLIT in vgaplan4.h
 *
 * 	This driver links with one of two other files, vgaplan4.c,
 * 	the portable VGA 4 planes 16 color driver, or asmplan4.s, which
 * 	is 8086 assembly language for speed.  This file itself
 * 	doesn't know about any planar or packed arrangement, relying soley
 * 	on the following external routines for all graphics drawing:
 * 		ega_init, ega_drawpixel, ega_readpixel,
 * 		ega_drawhorzline, ega_drawvertline
 *
 * 	All text/font drawing code is based on the above routines and
 * 	the included entry points for getting the ROM bitmap data.
 *
 * 	If the environment variable EGAMODE is set, the driver implements
 *	the EGA 640x350 (mode 10h) resolution, otherwise 640x480 (mode 12h)
 *	graphics mode is set.
 *
 *	The environment variable CHARHEIGHT if set will set the assumed rom
 *	font character height, which defaults to 14.
 *
 */

/*** HELLO OS ***/
#include "memory.h"

/****************/


#include "device.h"
#include "vgaplan4.h"
#if ROMFONT
#include "romfont.h"
#else
#include "genfont.h"
#endif


#include "genmem.h"


/* VGA driver entry points*/
static PSD  VGA_open(PSD psd);
static void VGA_close(PSD psd);
static void VGA_getscreeninfo(PSD psd,PMWSCREENINFO psi);
static void VGA_setpalette(PSD psd,int first,int count,MWPALENTRY *pal);
static MWBOOL VGA_mapmemgc(PSD mempsd,MWCOORD w,MWCOORD h,int planes,int bpp,
		int linelen,int size,void *addr);
static void NULL_blit(PSD dstpsd, MWCOORD dstx, MWCOORD dsty, MWCOORD w,
		MWCOORD h, PSD srcpsd, MWCOORD srcx, MWCOORD srcy, long op) {}
static PSD  NULL_allocatememgc(PSD psd) { return NULL; }
static MWBOOL NULL_mapmemgc(PSD mempsd,MWCOORD w,MWCOORD h,int planes,int bpp,
			int linelen,int size,void *addr) { return 0; }
static void NULL_freememgc(PSD mempsd) {}

SCREENDEVICE	scrdev = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL,
	VGA_open,
	VGA_close,
	VGA_getscreeninfo,
	VGA_setpalette,
	ega_drawpixel,
	ega_readpixel,
	ega_drawhorzline,
	ega_drawvertline,
	gen_fillrect,
#if ROMFONT
	pcrom_fonts,
#else
	gen_fonts,
#endif
#if HAVEBLIT
	ega_blit,		/* Blit*/
#else
	NULL_blit,		/* Blit*/
#endif
	NULL,			/* PreSelect*/
	NULL,			/* DrawArea*/
	NULL,			/* SetIOPermissions*/
	gen_allocatememgc,
#if HAVEBLIT
	VGA_mapmemgc,
#else
	NULL_mapmemgc,
#endif
	gen_freememgc
};

/* operating mode*/
static MWBOOL VGAMODE = TRUE;	/* ega or vga screen rows*/


static PSD
VGA_open(PSD psd)
{
	VGAMODE = TRUE;

	/* enter graphics mode*/
	ega_hwinit();

	/* init driver variables depending on ega/vga mode*/
	psd->xres = psd->xvirtres = 640;
	psd->yres = psd->yvirtres = VGAMODE? 480: 350;
	psd->planes = 4;
	psd->bpp = 4;
	psd->ncolors = 16;
	psd->pixtype = MWPF_PALETTE;
#if HAVEBLIT
	psd->flags = PSF_SCREEN | PSF_HAVEBLIT;
#else
	psd->flags = PSF_SCREEN;
#endif

	/* init planes driver (sets psd->addr and psd->linelen)*/
	ega_init(psd);

#if ROMFONT
	/* init pc rom font routines*/
	pcrom_init(psd);
#endif
#if 0
	ROM_CHAR_HEIGHT = VGAMODE? 16: 14;
#endif
	/* FIXME: add palette code*/
	return psd;
}

static void
VGA_close(PSD psd)
{
	ega_hwterm();
}

static void
VGA_getscreeninfo(PSD psd,PMWSCREENINFO psi)
{
	psi->rows = psd->yvirtres;
	psi->cols = psd->xvirtres;
	psi->planes = psd->planes;
	psi->bpp = psd->bpp;
	psi->ncolors = psd->ncolors;
	psi->pixtype = psd->pixtype;
	psi->fonts = NUMBER_FONTS;

	if(VGAMODE) {
		/* VGA 640x480*/
		psi->xdpcm = 27;	/* assumes screen width of 24 cm*/
		psi->ydpcm = 27;	/* assumes screen height of 18 cm*/
	} else {
		/* EGA 640x350*/
		psi->xdpcm = 27;	/* assumes screen width of 24 cm*/
		psi->ydpcm = 19;	/* assumes screen height of 18 cm*/
	}

#if ETA4000
	/* SVGA 800x600*/
	psi->xdpcm = 33;		/* assumes screen width of 24 cm*/
	psi->ydpcm = 33;		/* assumes screen height of 18 cm*/
#endif
}

static void
VGA_setpalette(PSD psd,int first,int count,MWPALENTRY *pal)
{
	/* not yet implemented, std 16 color palette assumed*/
}

#if HAVEBLIT
/* initialize memory device with passed parms, and select suitable fb driver*/
static MWBOOL
VGA_mapmemgc(PSD mempsd,MWCOORD w,MWCOORD h,int planes,int bpp,int linelen,
	int size,void *addr)
{
	extern SUBDRIVER memplan4;

	/* initialize mem screen driver*/
	initmemgc(mempsd, w, h, planes, bpp, linelen, size, addr);

	/* set and initialize subdriver into mem screen driver*/
	if (!set_subdriver(mempsd, &memplan4, TRUE))
		return 0;

	return 1;
}
#endif
