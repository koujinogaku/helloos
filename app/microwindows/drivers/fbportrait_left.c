/*
 * Copyright (c) 2000, 2001, 2010 Greg Haerr <greg@censoft.com>
 *
 * Left portrait mode subdriver for Microwindows
 *
 * Left rotation:
 * X -> Y
 * Y -> maxx - X - w
 * W -> H
 * H -> W
 */
/*** HELLO OS ***/
#include "portunixstd.h"
#include "string.h"
#include "memory.h"
/****************/

#include "device.h"
#include "fb.h"

static void
fbportrait_drawpixel(PSD psd,MWCOORD x, MWCOORD y, MWPIXELVAL c)
{
	psd->orgsubdriver->DrawPixel(psd, y, psd->xvirtres-x-1, c);
}

static MWPIXELVAL
fbportrait_readpixel(PSD psd,MWCOORD x, MWCOORD y)
{
	return psd->orgsubdriver->ReadPixel(psd, y, psd->xvirtres-x-1);
}

static void
fbportrait_drawhorzline(PSD psd,MWCOORD x1, MWCOORD x2, MWCOORD y, MWPIXELVAL c)
{
	/*x2 = psd->xvirtres-x2-1;
	while (x2 <= (psd->xvirtres-x1-1))
		fbportrait_drawpixel(psd, y, x2++, c);*/

	psd->orgsubdriver->DrawVertLine(psd, y, psd->xvirtres-x2-1, psd->xvirtres-x1-1, c);
}

static void
fbportrait_drawvertline(PSD psd,MWCOORD x, MWCOORD y1, MWCOORD y2, MWPIXELVAL c)
{
	/*while (y1 <= y2)
		fbportrait_drawpixel(psd, y1++, psd->xvirtres-x-1, c);*/

	psd->orgsubdriver->DrawHorzLine(psd, y1, y2, psd->xvirtres-x-1, c);
}

static void
fbportrait_fillrect(PSD psd,MWCOORD x1, MWCOORD y1, MWCOORD x2, MWCOORD y2, MWPIXELVAL c)
{
#if 0
	register ADDR16 addr;
        
#if DEBUG
	assert (x2 >= 0 && x2 < psd->yres);
	assert (x1 >= 0 && x1 < psd->yres);
	assert (x1 >= x2);
	assert (y1 >= 0 && y1 < psd->xres);
	assert (y2 >= 0 && y2 < psd->xres);
	assert (y2 >= y1);
	assert (c < psd->ncolors);
#endif
 	x1 = psd->xvirtres-x1-1;
	x2 = psd->xvirtres-x2-1;
	
	DRAWON;        
	if(gr_mode == MWROP_COPY) {                  
		// bytes per RECT-line
		int len = (y2-y1+1) << 1;
		ADDR16 addr0;

		// first line
		addr = ((ADDR16)psd->addr) + x2 * psd->linelen + y1;
		addr0 = addr;
		while (y1++ <= y2)
			*addr++ = c;

		// copy first line to all  other lines...
		addr = addr0;
		while (++x2 <= x1) {
			addr += psd->linelen;
			memcpy(addr, addr0, len);
		}
	} else {
		while(x2 <= x1) {
			addr = ((ADDR16)psd->addr) + y1 + x2++ * psd->linelen;
			while(y1++ <= y2) {
				applyOp(gr_mode, c, addr, ADDR16);
				++addr;
			}
		}
	}        
	DRAWOFF;
#else
	x1 = psd->xvirtres-x1-1;
	x2 = psd->xvirtres-x2-1;
	while(x2 <= x1)
		psd->orgsubdriver->DrawHorzLine(psd, y1, y2, x2++, c);
#endif
}

static void
fbportrait_blit(PSD dstpsd,MWCOORD destx,MWCOORD desty,MWCOORD w,MWCOORD h,
	PSD srcpsd, MWCOORD srcx,MWCOORD srcy,int op)
{
	dstpsd->orgsubdriver->Blit(dstpsd, desty, dstpsd->xvirtres-destx-w,
		h, w, srcpsd, srcy, srcpsd->xvirtres-srcx-w, op);
}

#if 0
static void
fbportrait_stretchblit(PSD dstpsd, MWCOORD destx, MWCOORD desty, MWCOORD dstw,
	MWCOORD dsth, PSD srcpsd, MWCOORD srcx, MWCOORD srcy, MWCOORD srcw,
	MWCOORD srch, int op)
{
	dstpsd->orgsubdriver->StretchBlit(dstpsd, desty, dstpsd->xvirtres-destx-dstw,
		dsth, dstw, srcpsd, srcy, srcpsd->xvirtres-srcx-srcw, srch, srcw, op);
}
#endif

static void
fbportrait_stretchblitex(PSD dstpsd, PSD srcpsd, MWCOORD dest_x_start, int dest_y_start,
	MWCOORD width, int height, int x_denominator, int y_denominator,
	int src_x_fraction, int src_y_fraction,
	int x_step_fraction, int y_step_fraction, int op)
{ 
	// X -> Y
 	// Y -> maxx - X - w
	dstpsd->orgsubdriver->StretchBlitEx(dstpsd, srcpsd,
		dest_y_start, dstpsd->xvirtres - dest_x_start - width,
		height, width, y_denominator, x_denominator,
		src_y_fraction, srcpsd->xvirtres - src_x_fraction - width,
		y_step_fraction, x_step_fraction,
		op);
}

#if MW_FEATURE_PSDOP_ALPHACOL
static void
fbportrait_drawarea_alphacol(PSD dstpsd, driver_gc_t * gc)
{
	ADDR8 alpha_in, alpha_out;
	MWCOORD	in_x, in_y, in_w, in_h;
	MWCOORD	out_x, out_y, out_h; //out_w, 
	driver_gc_t	l_gc;

	/* create new gc with rotated coords*/
	l_gc.width = gc->height;
	l_gc.height = gc->width;
	l_gc.dstx = gc->dsty;
	l_gc.dsty = dstpsd->xvirtres - gc->dstx - gc->width;
	l_gc.srcx = 0;	//FIXME rotate may fail with srcx/srcy not zero
	l_gc.srcy = 0;
	l_gc.src_linelen = l_gc.width;	/* 1 byte alpha channel*/

	/* copy the rest*/
	l_gc.op = gc->op;
	l_gc.fg_color = gc->fg_color;
	l_gc.bg_color = gc->bg_color;
	l_gc.usebg = gc->usebg;
	l_gc.dst_linelen = gc->dst_linelen;

	if (!(l_gc.data = ALLOCA(l_gc.width * l_gc.height)))
		return;

	alpha_in = ((ADDR8)gc->data) + gc->src_linelen * gc->srcy + gc->srcx;
	in_w = gc->width;
	in_h = gc->height;

	alpha_out = l_gc.data;
	//out_w = l_gc.width;
	out_h = l_gc.height;

	/* rotate_left_8bpp*/
	for (in_y = 0; in_y < in_h; in_y++) {
		for (in_x = 0; in_x < in_w; in_x++) {
			out_y = (out_h - 1) - in_x;
			out_x = in_y;

			//alpha_out[(out_y * out_w) + out_x] = alpha_in[(in_y * in_w) + in_x];
			alpha_out[(out_y * l_gc.src_linelen) + out_x] = alpha_in[(in_y * gc->src_linelen) + in_x];
		}
	}

	dstpsd->orgsubdriver->DrawArea(dstpsd, &l_gc);

	FREEA(l_gc.data);
}
#endif

#if MW_FEATURE_PSDOP_BITMAP_BYTES_MSB_FIRST
static void
fbportrait_drawarea_bitmap_bytes_msb_first(PSD psd, driver_gc_t * gc)
{
	ADDR8 pixel_in, pixel_out;
	MWCOORD	in_x, in_y, in_w, in_h;
	MWCOORD	out_x, out_y, out_h;//out_w, 
	driver_gc_t	l_gc;

	/* create new gc with rotated coords*/
	l_gc.width = gc->height;
	l_gc.height = gc->width;
	l_gc.dstx = gc->dsty;
	l_gc.dsty = psd->xvirtres - gc->dstx - gc->width;
	l_gc.srcx = 0;	//FIXME rotate may fail with srcx/srcy not zero
	l_gc.srcy = 0;
	l_gc.src_linelen = (l_gc.width + 7) / 8;

	/* copy the rest*/
	l_gc.op = gc->op;
	l_gc.fg_color = gc->fg_color;
	l_gc.bg_color = gc->bg_color;
	l_gc.usebg = gc->usebg;
	l_gc.dst_linelen = gc->dst_linelen;

	if (!(l_gc.data = ALLOCA(l_gc.height * l_gc.src_linelen)))
		return;
	memset(l_gc.data, 0, l_gc.height * l_gc.src_linelen);

	pixel_in = ((ADDR8)gc->data) + gc->src_linelen * gc->srcy + gc->srcx;
	in_w = gc->width;
	in_h = gc->height;

	pixel_out = l_gc.data;
	//out_w = l_gc.width;
	out_h = l_gc.height;

	/* rotate_left_1bpp*/
	for (in_y = 0; in_y < in_h; in_y++) {
		for (in_x = 0; in_x < in_w; in_x++) {
			out_y = (out_h - 1) - in_x;
			out_x = in_y;

			//pixel_out[(out_y * out_w) + out_x] = pixel_in[(in_y * in_w) + in_x];
			if (pixel_in[in_y*gc->src_linelen + (in_x >> 3)] & (0x80 >> (in_x&7)))
				pixel_out[out_y*l_gc.src_linelen + (out_x >> 3)] |= (0x80 >> (out_x&7));
		}
	}

	psd->orgsubdriver->DrawArea(psd, &l_gc);

	FREEA(l_gc.data);
}
#endif /* MW_FEATURE_PSDOP_BITMAP_BYTES_MSB_FIRST */

#if MW_FEATURE_PSDOP_BITMAP_BYTES_LSB_FIRST
static void
fbportrait_drawarea_bitmap_bytes_lsb_first(PSD psd, driver_gc_t * gc)
{
	ADDR8 pixel_in, pixel_out;
	MWCOORD	in_x, in_y, in_w, in_h;
	MWCOORD	out_x, out_y, out_h;// out_w,
	driver_gc_t	l_gc;

	/* create new gc with rotated coords*/
	l_gc.width = gc->height;
	l_gc.height = gc->width;
	l_gc.dstx = gc->dsty;
	l_gc.dsty = psd->xvirtres - gc->dstx - gc->width;
	l_gc.srcx = 0;	//FIXME rotate may fail with srcx/srcy not zero
	l_gc.srcy = 0;
	l_gc.src_linelen = (l_gc.width + 7) / 8;

	/* copy the rest*/
	l_gc.op = gc->op;
	l_gc.fg_color = gc->fg_color;
	l_gc.bg_color = gc->bg_color;
	l_gc.usebg = gc->usebg;
	l_gc.dst_linelen = gc->dst_linelen;

	if (!(l_gc.data = ALLOCA(l_gc.height * l_gc.src_linelen)))
		return;
	memset(l_gc.data, 0, l_gc.height * l_gc.src_linelen);

	pixel_in = ((ADDR8)gc->data) + gc->src_linelen * gc->srcy + gc->srcx;
	in_w = gc->width;
	in_h = gc->height;

	pixel_out = l_gc.data;
	//out_w = l_gc.width;
	out_h = l_gc.height;

	/* rotate_left_1bpp*/
	for (in_y = 0; in_y < in_h; in_y++) {
		for (in_x = 0; in_x < in_w; in_x++) {
			out_y = (out_h - 1) - in_x;
			out_x = in_y;

			//pixel_out[(out_y * out_w) + out_x] = pixel_in[(in_y * in_w) + in_x];
			if (pixel_in[in_y*gc->src_linelen + (in_x >> 3)] & (0x01 << (in_x&7)))
				pixel_out[out_y*l_gc.src_linelen + (out_x >> 3)] |= (0x01 << (out_x&7));
		}
	}

	psd->orgsubdriver->DrawArea(psd, &l_gc);

	FREEA(l_gc.data);
}
#endif /* MW_FEATURE_PSDOP_BITMAP_BYTES_LSB_FIRST */

static void
fbportrait_drawarea(PSD dstpsd, driver_gc_t * gc)
{
	if (!dstpsd->orgsubdriver->DrawArea)
		return;

	switch(gc->op) {
#if MW_FEATURE_PSDOP_ALPHACOL
	case PSDOP_ALPHACOL:
		fbportrait_drawarea_alphacol(dstpsd, gc);
		break;
#endif

#if MW_FEATURE_PSDOP_BITMAP_BYTES_MSB_FIRST
	case PSDOP_BITMAP_BYTES_MSB_FIRST:
		fbportrait_drawarea_bitmap_bytes_msb_first(dstpsd, gc);
		break;
#endif /* MW_FEATURE_PSDOP_BITMAP_BYTES_MSB_FIRST */

#if MW_FEATURE_PSDOP_BITMAP_BYTES_LSB_FIRST
	case PSDOP_BITMAP_BYTES_LSB_FIRST:
		fbportrait_drawarea_bitmap_bytes_lsb_first(dstpsd, gc);
		break;
#endif /* MW_FEATURE_PSDOP_BITMAP_BYTES_LSB_FIRST */
	}
}

SUBDRIVER fbportrait_left = {
	NULL,
	fbportrait_drawpixel,
	fbportrait_readpixel,
	fbportrait_drawhorzline,
	fbportrait_drawvertline,
	fbportrait_fillrect,
	fbportrait_blit,
	fbportrait_drawarea,
	fbportrait_stretchblitex
};
