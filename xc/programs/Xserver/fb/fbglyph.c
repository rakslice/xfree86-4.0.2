/*
 * Id: fbglyph.c,v 1.1 1999/11/02 03:54:45 keithp Exp $
 *
 * Copyright � 1998 Keith Packard
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Keith Packard not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Keith Packard makes no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * KEITH PACKARD DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL KEITH PACKARD BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
/* $XFree86: xc/programs/Xserver/fb/fbglyph.c,v 1.9 2000/09/27 00:14:08 keithp Exp $ */

#include "fb.h"
#include	"fontstruct.h"
#include	"dixfontstr.h"
      
Bool
fbGlyphIn (RegionPtr	pRegion,
	   int		x,
	   int		y,
	   int		width,
	   int		height)
{
    BoxRec  box;

    if (x + width < 0) return FALSE;
    if (y + height < 0) return FALSE;
    box.x1 = x;
    box.x2 = x + width;
    box.y1 = y;
    box.y2 = y + height;
    return RECT_IN_REGION (0, pRegion, &box) == rgnIN;
}

#ifdef FB_24BIT
#ifndef FBNOPIXADDR

#define WRITE1(d,n,fg)	((d)[n] = (CARD8) fg)
#define WRITE2(d,n,fg)	(*(CARD16 *) &(d[n]) = (CARD16) fg)
#define WRITE4(d,n,fg)	(*(CARD32 *) &(d[n]) = (CARD32) fg)
#if FB_UNIT == 6 && IMAGE_BYTE_ORDER == LSBFirst
#define WRITE8(d)	(*(FbBits *) &(d[0]) = fg)
#else
#define WRITE8(d)	WRITE4(d,0,_ABCA), WRITE4(d,4,_BCAB)
#endif
			 
/*
 * This is a bit tricky, but it's brief.  Write 12 bytes worth
 * of dest, which is four pixels, at a time.  This gives constant
 * code for each pattern as they're always aligned the same
 *
 *  a b c d  a b c d  a b c d	bytes
 *  A B C A  B C A B  C A B C	pixels
 * 
 *    f0        f1       f2
 *  A B C A  B C A B  C A B C	pixels LSB
 *  C A B C  A B C A  B C A B	pixels MSB
 *
 *		LSB	MSB
 *  A		f0	f1
 *  B		f1	f2
 *  C		f2	f0
 *  A B		f0	f2
 *  B C		f1	f0
 *  C A		f2	f1
 *  A B C A	f0	f1
 *  B C A B	f1    	f2
 *  C A B C	f2	f0
 */

#if IMAGE_BYTE_ORDER == MSBFirst
#define	_A	f1
#define _B	f2
#define _C	f0
#define _AB	f2
#define _BC	f0
#define _CA	f1
#define _ABCA	f1
#define _BCAB	f2
#define _CABC	f0
#define CASE(a,b,c,d)	((a << 3) | (b << 2) | (c << 1) | d)
#else
#define	_A	f0
#define _B	f1
#define _C	f2
#define _AB	f0
#define _BC	f1
#define _CA	f2
#define _ABCA	f0
#define _BCAB	f1
#define _CABC	f2
#define CASE(a,b,c,d)	(a | (b << 1) | (c << 2) | (d << 3))
#endif

void
fbGlyph24 (FbBits   *dstBits,
	   FbStride dstStride,
	   int	    dstBpp,
	   FbStip   *stipple,
	   FbBits   fg,
	   int	    x,
	   int	    height)
{
    int	    lshift;
    FbStip  bits;
    CARD8   *dstLine;
    CARD8   *dst;
    FbStip  f0, f1, f2;
    int	    n;
    int	    shift;

    f0 = fg;
    f1 = FbRot24(f0,16);
    f2 = FbRot24(f0,8);
    
    dstLine = (CARD8 *) dstBits;
    dstLine += (x & ~3) * 3;
    dstStride *= (sizeof (FbBits) / sizeof (CARD8));
    shift = x & 3;
    lshift = 4 - shift;
    while (height--)
    {
	bits = *stipple++;
	n = lshift;
	dst = dstLine;
	while (bits)
	{
	    switch (FbStipMoveLsb (FbLeftStipBits (bits, n), 4, n)) {
	    case CASE(0,0,0,0):
		break;
	    case CASE(1,0,0,0):
		WRITE2(dst,0,_AB);
		WRITE1(dst,2,_C);
		break;
	    case CASE(0,1,0,0):
		WRITE1(dst,3,_A);
		WRITE2(dst,4,_BC);
		break;
	    case CASE(1,1,0,0):
		WRITE4(dst,0,_ABCA);
		WRITE2(dst,4,_BC);
		break;
	    case CASE(0,0,1,0):
		WRITE2(dst,6,_AB);
		WRITE1(dst,8,_C);
		break;
	    case CASE(1,0,1,0):
		WRITE2(dst,0,_AB);
		WRITE1(dst,2,_C);
		
		WRITE2(dst,6,_AB);
		WRITE1(dst,8,_C);
		break;
	    case CASE(0,1,1,0):
		WRITE1(dst,3,_A);
		WRITE4(dst,4,_BCAB);
		WRITE1(dst,8,_C);
		break;
	    case CASE(1,1,1,0):
		WRITE8(dst);
		WRITE1(dst,8,_C);
		break;
	    case CASE(0,0,0,1):
		WRITE1(dst,9,_A);
		WRITE2(dst,10,_BC);
		break;
	    case CASE(1,0,0,1):
		WRITE2(dst,0,_AB);
		WRITE1(dst,2,_C);
		
		WRITE1(dst,9,_A);
		WRITE2(dst,10,_BC);
		break;
	    case CASE(0,1,0,1):
		WRITE1(dst,3,_A);
		WRITE2(dst,4,_BC);
		
		WRITE1(dst,9,_A);
		WRITE2(dst,10,_BC);
		break;
	    case CASE(1,1,0,1):
		WRITE4(dst,0,_ABCA);
		WRITE2(dst,4,_BC);
		
		WRITE1(dst,9,_A);
		WRITE2(dst,10,_BC);
		break;
	    case CASE(0,0,1,1):
		WRITE2(dst,6,_AB);
		WRITE4(dst,8,_CABC);
		break;
	    case CASE(1,0,1,1):
		WRITE2(dst,0,_AB);
		WRITE1(dst,2,_C);
		
		WRITE2(dst,6,_AB);
		WRITE4(dst,8,_CABC);
		break;
	    case CASE(0,1,1,1):
		WRITE1(dst,3,_A);
		WRITE4(dst,4,_BCAB);
		WRITE4(dst,8,_CABC);
		break;
	    case CASE(1,1,1,1):
		WRITE8(dst);
		WRITE4(dst,8,_CABC);
		break;
	    }
	    bits = FbStipLeft (bits, n);
	    n = 4;
	    dst += 12;
	}
	dstLine += dstStride;
    }
}
#endif
#endif

void
fbPolyGlyphBlt (DrawablePtr	pDrawable,
		GCPtr		pGC,
		int		x, 
		int		y,
		unsigned int	nglyph,
		CharInfoPtr	*ppci,
		pointer		pglyphBase)
{
    FbGCPrivPtr	    pPriv = fbGetGCPrivate (pGC);
    CharInfoPtr	    pci;
    unsigned char   *pglyph;		/* pointer bits in glyph */
    int		    gx, gy;
    int		    gWidth, gHeight;	/* width and height of glyph */
    FbStride	    gStride;		/* stride of glyph */
#ifndef FBNOPIXADDR
    void	    (*glyph) (FbBits *,
			      FbStride,
			      int,
			      FbStip *,
			      FbBits,
			      int,
			      int);
    FbBits	    *dst;
    FbStride	    dstStride;
    int		    dstBpp;
    
    glyph = 0;
    if (pGC->fillStyle == FillSolid && pPriv->and == 0)
    {
	fbGetDrawable (pDrawable, dst, dstStride, dstBpp);
	switch (dstBpp) {
	case 8:	    glyph = fbGlyph8; break;
	case 16:    glyph = fbGlyph16; break;
#ifdef FB_24BIT
	case 24:    glyph = fbGlyph24; break;
#endif
	case 32:    glyph = fbGlyph32; break;
	}
    }
#endif
    x += pDrawable->x;
    y += pDrawable->y;

    while (nglyph--)
    {
	pci = *ppci++;
	pglyph = FONTGLYPHBITS(pglyphBase, pci);
	gWidth = GLYPHWIDTHPIXELS(pci);
	gHeight = GLYPHHEIGHTPIXELS(pci);
	if (gWidth && gHeight)
	{
	    gx = x + pci->metrics.leftSideBearing;
	    gy = y - pci->metrics.ascent; 
#ifndef FBNOPIXADDR
	    if (glyph && gWidth <= sizeof (FbStip) * 8 &&
		fbGlyphIn (fbGetCompositeClip(pGC), gx, gy, gWidth, gHeight))
	    {
		(*glyph) (dst + gy * dstStride,
			  dstStride,
			  dstBpp,
			  (FbStip *) pglyph,
			  pPriv->xor,
			  gx,
			  gHeight);
	    }
	    else
#endif
	    {
		gStride = GLYPHWIDTHBYTESPADDED(pci) / sizeof (FbStip);
		fbPushImage (pDrawable,
			     pGC,
    
			     (FbStip *) pglyph,
			     gStride,
			     0,
    
			     gx,
			     gy,
			     gWidth, gHeight);
	    }
	}
	x += pci->metrics.characterWidth;
    }
}


void
fbImageGlyphBlt (DrawablePtr	pDrawable,
		 GCPtr		pGC,
		 int		x, 
		 int		y,
		 unsigned int	nglyph,
		 CharInfoPtr	*ppciInit,
		 pointer	pglyphBase)
{
    FbGCPrivPtr	    pPriv = fbGetGCPrivate(pGC);
    CharInfoPtr	    *ppci;
    CharInfoPtr	    pci;
    unsigned char   *pglyph;		/* pointer bits in glyph */
    int		    gWidth, gHeight;	/* width and height of glyph */
    FbStride	    gStride;		/* stride of glyph */
    Bool	    opaque;
    int		    n;
    int		    gx, gy;
#ifndef FBNOPIXADDR
    void	    (*glyph) (FbBits *,
			      FbStride,
			      int,
			      FbStip *,
			      FbBits,
			      int,
			      int);
    FbBits	    *dst;
    FbStride	    dstStride;
    int		    dstBpp;
    
    glyph = 0;
    if (pPriv->and == 0)
    {
	fbGetDrawable (pDrawable, dst, dstStride, dstBpp);
	switch (dstBpp) {
	case 8:	    glyph = fbGlyph8; break;
	case 16:    glyph = fbGlyph16; break;
#ifdef FB_24BIT
	case 24:    glyph = fbGlyph24; break;
#endif
	case 32:    glyph = fbGlyph32; break;
	}
    }
#endif
    
    x += pDrawable->x;
    y += pDrawable->y;

    if (TERMINALFONT (pGC->font)
#ifndef FBNOPIXADDR
	&& !glyph
#endif
	)
    {
	opaque = TRUE;
    }
    else
    {
	int		xBack, widthBack;
	int		yBack, heightBack;
	
	ppci = ppciInit;
	n = nglyph;
	widthBack = 0;
	while (n--)
	    widthBack += (*ppci++)->metrics.characterWidth;
	
        xBack = x;
	if (widthBack < 0)
	{
	    xBack += widthBack;
	    widthBack = -widthBack;
	}
	yBack = y - FONTASCENT(pGC->font);
	heightBack = FONTASCENT(pGC->font) + FONTDESCENT(pGC->font);
	fbSolidBoxClipped (pDrawable,
			   fbGetCompositeClip(pGC),
			   xBack,
			   yBack,
			   xBack + widthBack,
			   yBack + heightBack,
			   fbAnd(GXcopy,pPriv->bg,pPriv->pm),
			   fbXor(GXcopy,pPriv->bg,pPriv->pm));
	opaque = FALSE;
    }

    ppci = ppciInit;
    while (nglyph--)
    {
	pci = *ppci++;
	pglyph = FONTGLYPHBITS(pglyphBase, pci);
	gWidth = GLYPHWIDTHPIXELS(pci);
	gHeight = GLYPHHEIGHTPIXELS(pci);
	if (gWidth && gHeight)
	{
	    gx = x + pci->metrics.leftSideBearing;
	    gy = y - pci->metrics.ascent; 
#ifndef FBNOPIXADDR
	    if (glyph && gWidth <= sizeof (FbStip) * 8 &&
		fbGlyphIn (fbGetCompositeClip(pGC), gx, gy, gWidth, gHeight))
	    {
		(*glyph) (dst + gy * dstStride,
			  dstStride,
			  dstBpp,
			  (FbStip *) pglyph,
			  pPriv->fg,
			  gx,
			  gHeight);
	    }
	    else
#endif
	    {
		gStride = GLYPHWIDTHBYTESPADDED(pci) / sizeof (FbStip);
		fbPutXYImage (pDrawable,
			      fbGetCompositeClip(pGC),
			      pPriv->fg,
			      pPriv->bg,
			      pPriv->pm,
			      GXcopy,
			      opaque,
    
			      gx,
			      gy,
			      gWidth, gHeight,
    
			      (FbStip *) pglyph,
			      gStride,
			      0);
	    }
	}
	x += pci->metrics.characterWidth;
    }
}
