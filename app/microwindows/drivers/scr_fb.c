/*
 * Copyright (c) 1999, 2000, 2001, 2002 Greg Haerr <greg@censoft.com>
 * Portions Copyright (c) 2002 Koninklijke Philips Electronics
 *
 * Microwindows Screen Driver for Linux kernel framebuffers
 *
 * Portions used from Ben Pfaff's BOGL <pfaffben@debian.org>
 * 
 * Note: modify select_fb_driver() to add new framebuffer subdrivers
 */
/*** HELLO OS ***/
#include "config.h"
#include "syscall.h"
#include "string.h"
#include "cpu.h"
#include "portunixstd.h"

/****************/

#include "device.h"
#include "genfont.h"
#include "genmem.h"
#include "fb.h"


#ifndef FB_TYPE_VGA_PLANES
#define FB_TYPE_VGA_PLANES 4
#endif

#define SCR_FB_VMODE_TEXT  1
#define SCR_FB_COLOR_PALLET    1
#define SCR_FB_COLOR_TRUECOLOR 0

#define	VGA_DAC_READ_INDEX	0x3C7
#define	VGA_DAC_WRITE_INDEX	0x3C8
#define	VGA_DAC_DATA		0x3C9

static PSD  fb_open(PSD psd);
static void fb_close(PSD psd);
static void fb_setportrait(PSD psd, int portraitmode);
static void fb_setpalette(PSD psd,int first, int count, MWPALENTRY *palette);
static void gen_getscreeninfo(PSD psd,PMWSCREENINFO psi);


SCREENDEVICE	scrdev = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL,
	fb_open,
	fb_close,
	gen_getscreeninfo,
	fb_setpalette,
	NULL,			/* DrawPixel subdriver*/
	NULL,			/* ReadPixel subdriver*/
	NULL,			/* DrawHorzLine subdriver*/
	NULL,			/* DrawVertLine subdriver*/
	NULL,			/* FillRect subdriver*/
	gen_fonts,
	NULL,			/* Blit subdriver*/
	NULL,			/* PreSelect*/
	NULL,			/* DrawArea subdriver*/
	NULL,			/* SetIOPermissions*/
	gen_allocatememgc,
	fb_mapmemgc,
	gen_freememgc,
	NULL,			/* StretchBlit subdriver*/
	fb_setportrait		/* SetPortrait*/
};

/* static variables*/
static int status=0;		/* 0=never inited, 1=once inited, 2=inited. */
static short saved_red[16];	/* original hw palette*/
static short saved_green[16];
static short saved_blue[16];

extern SUBDRIVER fbportrait_left, fbportrait_right, fbportrait_down;
static PSUBDRIVER pdrivers[4] = { /* portrait mode subdrivers*/
	NULL, &fbportrait_left, &fbportrait_right, &fbportrait_down
};

/* local functions*/
static void	set_directcolor_palette(PSD psd);

/* init framebuffer*/
static PSD
fb_open(PSD psd)
{
	int	visual;
	PSUBDRIVER subdriver;
	struct bios_info info;

	assert(status < 2);

	/* locate and open framebuffer, get info*/
	if(syscall_kernel_get_bios_info((char *)&info) < 0) {
		EPRINTF("Error reading screen info: %m\n");
			goto fail;
	}
	/* setup screen device from framebuffer info*/
	visual = SCR_FB_COLOR_PALLET;

	psd->portrait = MWPORTRAIT_NONE;
	psd->xres = psd->xvirtres = info.scrnx;
	psd->yres = psd->yvirtres = info.scrny;

	/* set planes from fb type*/
	if (info.vmode == SCR_FB_VMODE_TEXT)
		psd->planes = 0; /* force error later*/
	else
		psd->planes = 1;

	psd->bpp = info.depth;
	psd->ncolors = (psd->bpp >= 24)? (1 << 24): (1 << psd->bpp);

	/* set linelen to byte length, possibly converted later*/
	psd->linelen = info.scrnx * (info.depth / 8);
	psd->size = 0;		/* force subdriver init of size*/

	psd->flags = PSF_SCREEN | PSF_HAVEBLIT;
	if (psd->bpp == 16)
		psd->flags |= PSF_HAVEOP_COPY;

	/* set pixel format*/
	if(visual == SCR_FB_COLOR_TRUECOLOR) {
		switch(psd->bpp) {
		case 8:
			psd->pixtype = MWPF_TRUECOLOR332;
			break;
		case 16:
			if (info.depth != 16)
				psd->pixtype = MWPF_TRUECOLOR555;
			else
				psd->pixtype = MWPF_TRUECOLOR565;
			break;
		case 24:
			psd->pixtype = MWPF_TRUECOLOR888;
			break;
		case 32:
			psd->pixtype = MWPF_TRUECOLOR0888;
			/* Check if we have alpha */
			if (info.depth != 32)
				psd->pixtype = MWPF_TRUECOLOR8888;
			break;
		default:
			EPRINTF(
			"Unsupported %ld color (%d bpp) truecolor framebuffer\n",
				psd->ncolors, psd->bpp);
			goto fail;
		}
	} else 
		psd->pixtype = MWPF_PALETTE;

	/*DPRINTF("%dx%dx%d linelen %d type %d visual %d bpp %d\n", psd->xres,
	 	psd->yres, psd->ncolors, psd->linelen, type, visual,
		psd->bpp);*/

	/* select a framebuffer subdriver based on planes and bpp*/
	subdriver = select_fb_subdriver(psd);
	if (!subdriver) {
		EPRINTF("No driver for screen type %d visual %d bpp %d\n",
			info.vmode, visual, psd->bpp);
		goto fail;
	}

	/*
	 * set and initialize subdriver into screen driver
	 * psd->size is calculated by subdriver init
	 */
	if(!set_subdriver(psd, subdriver, TRUE)) {
		EPRINTF("Driver initialize failed type %d visual %d bpp %d\n",
			info.vmode, visual, psd->bpp);
		goto fail;
	}

	/* remember original subdriver for portrait mode switching*/
	pdrivers[0] = psd->orgsubdriver = subdriver;

	/* mmap framebuffer into this address space*/
	if(info.vmode < 0x100 ) {  // VGA MODE(320x200x8bit)
		psd->addr = (unsigned char *)info.vram; // Physical Address
	}
	else {                     // VESA MODE
		psd->addr = (void*)CFG_MEM_VESAWINDOWSTART;    // Virtual  Address
	}

	if(psd->addr == NULL || psd->addr == (unsigned char *)-1) {
		EPRINTF("Error vram addr\n");
		goto fail;
	}

	/* save original palette*/
	ioctl_getpalette(0, 16, saved_red, saved_green, saved_blue);

	/* setup direct color palette if required (ATI cards)*/
	if(visual == SCR_FB_COLOR_TRUECOLOR)
		set_directcolor_palette(psd);

	status = 2;
	return psd;	/* success*/

 fail:
	return NULL;
}

/* close framebuffer*/
static void
fb_close(PSD psd)
{

	/* if not opened, return*/
	if(status != 2)
		return;
	status = 1;

  	/* reset hw palette*/
	ioctl_setpalette(0, 16, saved_red, saved_green, saved_blue);
  
}

static void
fb_setportrait(PSD psd, int portraitmode)
{
	psd->portrait = portraitmode;

	/* swap x and y in left or right portrait modes*/
	if (portraitmode & (MWPORTRAIT_LEFT|MWPORTRAIT_RIGHT)) {
		/* swap x, y*/
		psd->xvirtres = psd->yres;
		psd->yvirtres = psd->xres;
	} else {
		/* normal x, y*/
		psd->xvirtres = psd->xres;
		psd->yvirtres = psd->yres;
	}
	/* assign portrait subdriver which calls original subdriver*/
	if (portraitmode == MWPORTRAIT_DOWN)
		portraitmode = 3;	/* change bitpos to index*/
	set_subdriver(psd, pdrivers[portraitmode], FALSE);
}

/* setup directcolor palette - required for ATI cards*/
static void
set_directcolor_palette(PSD psd)
{
	int i;
	short r[256];

	/* 16bpp uses 32 palette entries*/
	if(psd->bpp == 16) {
		for(i=0; i<32; ++i)
			r[i] = i<<11;
		ioctl_setpalette(0, 32, r, r, r);
	} else {
		/* 32bpp uses 256 entries*/
		for(i=0; i<256; ++i)
			r[i] = i<<8;
		ioctl_setpalette(0, 256, r, r, r);
	}
}

static int fade = 100;

/* convert Microwindows palette to framebuffer format and set it*/
static void
fb_setpalette(PSD psd,int first, int count, MWPALENTRY *palette)
{
	int 	i;
	unsigned short 	red[256];
	unsigned short 	green[256];
	unsigned short 	blue[256];

	if (count > 256)
		count = 256;

	/* convert palette to framebuffer format*/
	for(i=0; i < count; i++) {
		MWPALENTRY *p = &palette[i];

		/* grayscale computation:
		 * red[i] = green[i] = blue[i] =
		 *	(p->r * 77 + p->g * 151 + p->b * 28);
		 */
		red[i] = (p->r * fade / 100) << 8;
		green[i] = (p->g * fade / 100) << 8;
		blue[i] = (p->b * fade / 100) << 8;
	}
	ioctl_setpalette(first, count, (short *)red, (short *)green, (short *)blue);
}

/* get framebuffer palette*/
void
ioctl_getpalette(int start, int len, short *red, short *green, short *blue)
{
	int i;

	cpu_lock();
	cpu_out8(VGA_DAC_READ_INDEX, start);
	for (i = 0; i <len; i++) {
		*red  =cpu_in8(VGA_DAC_DATA) << (2+8);
		*green=cpu_in8(VGA_DAC_DATA) << (2+8);
		*blue =cpu_in8(VGA_DAC_DATA) << (2+8);
		red++;
		green++;
		blue++;
	}
	cpu_unlock();
}

/* set framebuffer palette*/
void
ioctl_setpalette(int start, int len, short *red, short *green, short *blue)
{
	int i;

	cpu_lock();
	cpu_out8(VGA_DAC_WRITE_INDEX, start);
	for (i = 0; i < len; i++) {
		cpu_out8(VGA_DAC_DATA, (*red   >> (2+8)) & 0x3f);
		cpu_out8(VGA_DAC_DATA, (*green >> (2+8)) & 0x3f);
		cpu_out8(VGA_DAC_DATA, (*blue  >> (2+8)) & 0x3f);

		red++;
		green++;
		blue++;
	}
	cpu_unlock();
}

/* experimental palette animation*/
void
setfadelevel(PSD psd, int f)
{
	int 		i;
	unsigned short 	r[256], g[256], b[256];
	extern MWPALENTRY gr_palette[256];

	if(psd->pixtype != MWPF_PALETTE)
		return;

	fade = f;
	if(fade > 100)
		fade = 100;
	for(i=0; i<256; ++i) {
		r[i] = (gr_palette[i].r * fade / 100) << 8;
		g[i] = (gr_palette[i].g * fade / 100) << 8;
		b[i] = (gr_palette[i].b * fade / 100) << 8;
	}
	ioctl_setpalette(0, 256, (short *)r, (short *)g, (short *)b);
}

static void
gen_getscreeninfo(PSD psd,PMWSCREENINFO psi)
{
	psi->rows = psd->yvirtres;
	psi->cols = psd->xvirtres;
	psi->planes = psd->planes;
	psi->bpp = psd->bpp;
	psi->ncolors = psd->ncolors;
	psi->fonts = NUMBER_FONTS;
	psi->portrait = psd->portrait;
	psi->fbdriver = TRUE;	/* running fb driver, can direct map*/

	psi->pixtype = psd->pixtype;
	switch (psd->pixtype) {
	case MWPF_TRUECOLOR8888:
	case MWPF_TRUECOLOR0888:
	case MWPF_TRUECOLOR888:
		psi->rmask 	= 0xff0000;
		psi->gmask 	= 0x00ff00;
		psi->bmask	= 0x0000ff;
		break;
	case MWPF_TRUECOLOR565:
		psi->rmask 	= 0xf800;
		psi->gmask 	= 0x07e0;
		psi->bmask	= 0x001f;
		break;
	case MWPF_TRUECOLOR555:
		psi->rmask 	= 0x7c00;
		psi->gmask 	= 0x03e0;
		psi->bmask	= 0x001f;
		break;
	case MWPF_TRUECOLOR332:
		psi->rmask 	= 0xe0;
		psi->gmask 	= 0x1c;
		psi->bmask	= 0x03;
		break;
	case MWPF_PALETTE:
	default:
		psi->rmask 	= 0xff;
		psi->gmask 	= 0xff;
		psi->bmask	= 0xff;
		break;
	}

	if(psd->yvirtres > 480) {
		/* SVGA 800x600*/
		psi->xdpcm = 33;	/* assumes screen width of 24 cm*/
		psi->ydpcm = 33;	/* assumes screen height of 18 cm*/
	} else if(psd->yvirtres > 350) {
		/* VGA 640x480*/
		psi->xdpcm = 27;	/* assumes screen width of 24 cm*/
		psi->ydpcm = 27;	/* assumes screen height of 18 cm*/
        } else if(psd->yvirtres <= 240) {
		/* half VGA 640x240 */
		psi->xdpcm = 14;        /* assumes screen width of 24 cm*/ 
		psi->ydpcm =  5;
	} else {
		/* EGA 640x350*/
		psi->xdpcm = 27;	/* assumes screen width of 24 cm*/
		psi->ydpcm = 19;	/* assumes screen height of 18 cm*/
	}
}
