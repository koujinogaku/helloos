/*
 * Copyright (c) 2000 Greg Haerr <greg@censoft.com>
 *
 * Screen Driver Utilities
 * 
 * Microwindows memory device routines header file
 */

/* entry points*/

/* genmem.c*/
PSD 	gen_allocatememgc(PSD psd);
void	gen_freememgc(PSD mempsd);
void	initmemgc(PSD mempsd,MWCOORD w,MWCOORD h,int planes,int bpp,
		int linelen,int size,void *addr);

void	gen_fillrect(PSD psd,MWCOORD x1, MWCOORD y1, MWCOORD x2, MWCOORD y2,
		MWPIXELVAL c);
MWBOOL	set_subdriver(PSD psd, PSUBDRIVER subdriver, MWBOOL init);

/* fb.c*/
MWBOOL	fb_mapmemgc(PSD mempsd,MWCOORD w,MWCOORD h,int planes,int bpp,
		int linelen,int size,void *addr);
