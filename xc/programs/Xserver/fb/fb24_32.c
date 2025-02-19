/*
 * $XFree86: xc/programs/Xserver/fb/fb24_32.c,v 1.4 2000/08/09 17:50:51 keithp Exp $
 *
 * Copyright � 2000 SuSE, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of SuSE not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  SuSE makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * SuSE DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL SuSE
 * BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN 
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author:  Keith Packard, SuSE, Inc.
 */

#ifdef XFree86LOADER
#include "xf86.h"
#include "xf86_ansic.h"
#endif

#include "fb.h"

/* X apps don't like 24bpp images, this code exposes 32bpp images */

/*
 * These two functions do a full CopyArea while reformatting
 * the data between 24 and 32bpp.  They try to go a bit faster
 * by reading/writing aligned CARD32s where it's easy
 */

#define Get8(a)	((CARD32) *(a))

#if BITMAP_BIT_ORDER == MSBFirst
#define Get24(a)    ((Get8(a) << 16) | (Get8((a)+1) << 8) | Get8((a)+2))
#define Put24(a,p)  (((a)[0] = (CARD8) ((p) >> 16)), \
		     ((a)[1] = (CARD8) ((p) >> 8)), \
		     ((a)[2] = (CARD8) (p)))
#else
#define Get24(a)    (Get8(a) | (Get8((a)+1) << 8) | (Get8((a)+2)<<16))
#define Put24(a,p)  (((a)[0] = (CARD8) (p)), \
		     ((a)[1] = (CARD8) ((p) >> 8)), \
		     ((a)[2] = (CARD8) ((p) >> 16)))
#endif

typedef void (*fb24_32BltFunc) (CARD8	    *srcLine,
				FbStride    srcStride,
				int	    srcX,

				CARD8	    *dstLine,
				FbStride    dstStride,
				int	    dstX,

				int	    width, 
				int	    height,

				int	    alu,
				FbBits	    pm);

static void
fb24_32BltDown (CARD8	    *srcLine,
		FbStride    srcStride,
		int	    srcX,

		CARD8	    *dstLine,
		FbStride    dstStride,
		int	    dstX,

		int	    width, 
		int	    height,

		int	    alu,
		FbBits	    pm)
{
    CARD32  *src;
    CARD8   *dst;
    int	    w;
    Bool    destInvarient;
    CARD32  pixel, dpixel;
    FbDeclareMergeRop ();
    
    srcLine += srcX * 4;
    dstLine += dstX * 3;

    FbInitializeMergeRop(alu, (pm | ~(FbBits) 0xffffff));
    destInvarient = FbDestInvarientMergeRop();

    while (height--)
    {
	src = (CARD32 *) srcLine;
	dst = dstLine;
	srcLine += srcStride;
	dstLine += dstStride;
	w = width;
	if (destInvarient)
	{
	    while (((long) dst & 3) && w)
	    {
		w--;
		pixel = *src++;
		pixel = FbDoDestInvarientMergeRop(pixel);
		Put24 (dst, pixel);
		dst += 3;
	    }
	    /* Do four aligned pixels at a time */
	    while (w >= 4)
	    {
		CARD32  s0, s1;
		s0 = *src++;
		s0 = FbDoDestInvarientMergeRop(s0);
		s1 = *src++;
		s1 = FbDoDestInvarientMergeRop(s1);
#if BITMAP_BIT_ORDER == LSBFirst
		*(CARD32 *)(dst) = (s0 & 0xffffff) | (s1 << 24);
#else
		*(CARD32 *)(dst) = (s0 << 8) | ((s1 & 0xffffff) >> 16);
#endif
		s0 = *src++;
		s0 = FbDoDestInvarientMergeRop(s0);
#if BITMAP_BIT_ORDER == LSBFirst
		*(CARD32 *)(dst+4) = ((s1 & 0xffffff) >> 8) | (s0 << 16);
#else
		*(CARD32 *)(dst+4) = (s1 << 16) | ((s0 & 0xffffff) >> 8);
#endif
		s1 = *src++;
		s1 = FbDoDestInvarientMergeRop(s1);
#if BITMAP_BIT_ORDER == LSBFirst
		*(CARD32 *)(dst+8) = ((s0 & 0xffffff) >> 16) | (s1 << 8);
#else
		*(CARD32 *)(dst+8) = (s0 << 24) | (s1 & 0xffffff);
#endif
		dst += 12;
		w -= 4;
	    }
	    while (w--)
	    {
		pixel = *src++;
		pixel = FbDoDestInvarientMergeRop(pixel);
		Put24 (dst, pixel);
		dst += 3;
	    }
	}
	else
	{
	    while (w--)
	    {
		pixel = *src++;
		dpixel = Get24 (dst);
		pixel = FbDoMergeRop(pixel, dpixel);
		Put24 (dst, pixel);
		dst += 3;
	    }
	}
    }
}

static void
fb24_32BltUp (CARD8	    *srcLine,
	      FbStride	    srcStride,
	      int	    srcX,

	      CARD8	    *dstLine,
	      FbStride	    dstStride,
	      int	    dstX,

	      int	    width, 
	      int	    height,

	      int	    alu,
	      FbBits	    pm)
{
    CARD8   *src;
    CARD32  *dst;
    int	    w;
    Bool    destInvarient;
    CARD32  pixel;
    FbDeclareMergeRop ();
    
    FbInitializeMergeRop(alu, (pm | (~(FbBits) 0xffffff)));
    destInvarient = FbDestInvarientMergeRop();

    srcLine += srcX * 3;
    dstLine += dstX * 4;

    while (height--)
    {
	w = width;
	src = srcLine;
	dst = (CARD32 *) dstLine;
	srcLine += srcStride;
	dstLine += dstStride;
	if (destInvarient)
	{
	    while (((long) src & 3) && w)
	    {
		w--;
		pixel = Get24(src);
		src += 3;
		*dst++ = FbDoDestInvarientMergeRop(pixel);
	    }
	    /* Do four aligned pixels at a time */
	    while (w >= 4)
	    {
		CARD32  s0, s1;

		s0 = *(CARD32 *)(src);
#if BITMAP_BIT_ORDER == LSBFirst
		pixel = s0 & 0xffffff;
#else
		pixel = s0 >> 8;
#endif
		*dst++ = FbDoDestInvarientMergeRop(pixel);
		s1 = *(CARD32 *)(src+4);
#if BITMAP_BIT_ORDER == LSBFirst
		pixel = (s0 >> 24) | ((s1 << 8) & 0xffffff);
#else
		pixel = ((s0 << 16) & 0xffffff) | (s1 >> 16);
#endif
		*dst++ = FbDoDestInvarientMergeRop(pixel);
		s0 = *(CARD32 *)(src+8);
#if BITMAP_BIT_ORDER == LSBFirst
		pixel = (s1 >> 16) | ((s0 << 16) & 0xffffff);
#else
		pixel = ((s1 << 8) & 0xffffff) | (s0 >> 24);
#endif
		*dst++ = FbDoDestInvarientMergeRop(pixel);
#if BITMAP_BIT_ORDER == LSBFirst
		pixel = s0 >> 8;
#else
		pixel = s0 & 0xffffff;
#endif
		*dst++ = FbDoDestInvarientMergeRop(pixel);
		src += 12;
		w -= 4;
	    }
	    while (w)
	    {
		w--;
		pixel = Get24(src);
		src += 3;
		*dst++ = FbDoDestInvarientMergeRop(pixel);
	    }
	}
	else
	{
	    while (w--)
	    {
		pixel = Get24(src);
		src += 3;
		*dst = FbDoMergeRop(pixel, *dst);
		dst++;
	    }
	}
    }
}

/*
 * Spans functions; probably unused.
 */
void
fb24_32GetSpans(DrawablePtr	pDrawable, 
		int		wMax, 
		DDXPointPtr	ppt, 
		int		*pwidth, 
		int		nspans, 
		char		*pchardstStart)
{
    FbBits	    *srcBits;
    CARD8	    *src;
    FbStride	    srcStride;
    int		    srcBpp;
    CARD8	    *dst;
    
    fbGetDrawable (pDrawable, srcBits, srcStride, srcBpp);
    src = (CARD8 *) srcBits;
    srcStride *= sizeof (FbBits);
    
    while (nspans--)
    {
	dst = (CARD8 *) pchardstStart;
	fb24_32BltUp (src + ppt->y * srcStride, srcStride,
		      ppt->x,
	       
		      dst,
		      1,
		      0,

		      *pwidth,
		      1,

		      GXcopy,
		      FB_ALLONES);
	
	pchardstStart += PixmapBytePad(*pwidth, pDrawable->depth);
	ppt++;
	pwidth++;
    }
}

void
fb24_32SetSpans (DrawablePtr	    pDrawable,
		 GCPtr		    pGC,
		 char		    *src,
		 DDXPointPtr	    ppt,
		 int		    *pwidth,
		 int		    nspans,
		 int		    fSorted)
{
    FbGCPrivPtr	    pPriv = fbGetGCPrivate (pGC);
    RegionPtr	    pClip = fbGetCompositeClip(pGC);
    FbBits	    *dstBits;
    CARD8	    *dst, *d, *s;
    FbStride	    dstStride;
    int		    dstBpp;
    BoxPtr	    pbox;
    int		    n;
    int		    x1, x2;

    fbGetDrawable (pDrawable, dstBits, dstStride, dstBpp);
    dst = (CARD8 *) dstBits;
    dstStride *= sizeof (FbBits);
    while (nspans--)
    {
	d = dst + ppt->y * dstStride;
	s = (CARD8 *) src;
	n = REGION_NUM_RECTS(pClip);
	pbox = REGION_RECTS (pClip);
	while (n--)
	{
	    if (pbox->y1 > ppt->y)
		break;
	    if (pbox->y2 > ppt->y)
	    {
		x1 = ppt->x;
		x2 = x1 + *pwidth;
		if (pbox->x1 > x1)
		    x1 = pbox->x1;
		if (pbox->x2 < x2)
		    x2 = pbox->x2;
		if (x1 < x2)
		    fb24_32BltDown (s,
				    0,
				    (x1 - ppt->x),
				    d,
				    dstStride,
				    x1,

				    (x2 - x1),
				    1,
				    pGC->alu,
				    pPriv->pm);
	    }
	}
	src += PixmapBytePad (*pwidth, pDrawable->depth);
	ppt++;
	pwidth++;
    }
}

/*
 * Clip and put 32bpp Z-format images to a 24bpp drawable
 */
void
fb24_32PutZImage (DrawablePtr	pDrawable,
		  RegionPtr	pClip,
		  int		alu,
		  FbBits	pm,
		  int		x,
		  int		y,
		  int		width,
		  int		height,
		  CARD8		*src,
		  FbStride	srcStride)
{
    FbBits	*dstBits;
    CARD8	*dst;
    FbStride	dstStride;
    int		dstBpp;
    int		nbox;
    BoxPtr	pbox;
    int		x1, y1, x2, y2;

    fbGetDrawable (pDrawable, dstBits, dstStride, dstBpp);
    dstStride *= sizeof(FbBits);
    dst = (CARD8 *) dstBits;

    for (nbox = REGION_NUM_RECTS (pClip),
	 pbox = REGION_RECTS(pClip);
	 nbox--;
	 pbox++)
    {
	x1 = x;
	y1 = y;
	x2 = x + width;
	y2 = y + height;
	if (x1 < pbox->x1)
	    x1 = pbox->x1;
	if (y1 < pbox->y1)
	    y1 = pbox->y1;
	if (x2 > pbox->x2)
	    x2 = pbox->x2;
	if (y2 > pbox->y2)
	    y2 = pbox->y2;
	if (x1 >= x2 || y1 >= y2)
	    continue;
	fb24_32BltDown (src + (y1 - y) * srcStride,
			srcStride,
			(x1 - x),

			dst + y1 * dstStride,
			dstStride,
			x1,

			(x2 - x1),
			(y2 - y1),

			alu,
			pm);
    }
}

void
fb24_32GetImage (DrawablePtr     pDrawable,
		 int             x,
		 int             y,
		 int             w,
		 int             h,
		 unsigned int    format,
		 unsigned long   planeMask,
		 char            *d)
{
    FbBits	    *srcBits;
    CARD8	    *src;
    FbStride	    srcStride;
    int		    srcBpp;
    FbStride	    dstStride;
    FbBits	    pm;

    fbGetDrawable (pDrawable, srcBits, srcStride, srcBpp);
    src = (CARD8 *) srcBits;
    srcStride *= sizeof (FbBits);

    x += pDrawable->x;
    y += pDrawable->y;
    
    pm = fbReplicatePixel (planeMask, 32);
    dstStride = PixmapBytePad(w, pDrawable->depth);
    if (pm != FB_ALLONES)
	memset (d, 0, dstStride * h);
    fb24_32BltUp (src + y * srcStride, srcStride, x,
		  (CARD8 *) d, dstStride, 0,
		  w, h, GXcopy, pm);
}

void
fb24_32CopyMtoN (DrawablePtr pSrcDrawable,
		 DrawablePtr pDstDrawable,
		 GCPtr       pGC,
		 BoxPtr      pbox,
		 int         nbox,
		 int         dx,
		 int         dy,
		 Bool        reverse,
		 Bool        upsidedown,
		 Pixel       bitplane,
		 void        *closure)
{
    FbGCPrivPtr	pPriv = fbGetGCPrivate(pGC);
    FbBits	*srcBits;
    CARD8	*src;
    FbStride	srcStride;
    int		srcBpp;
    FbBits	*dstBits;
    CARD8	*dst;
    FbStride	dstStride;
    int		dstBpp;
    fb24_32BltFunc  blt;
    
    fbGetDrawable (pSrcDrawable, srcBits, srcStride, srcBpp);
    src = (CARD8 *) srcBits;
    srcStride *= sizeof (FbBits);
    fbGetDrawable (pDstDrawable, dstBits, dstStride, dstBpp);
    dst = (CARD8 *) dstBits;
    dstStride *= sizeof (FbBits);
    if (srcBpp == 24)
	blt = fb24_32BltUp;
    else
	blt = fb24_32BltDown;
    
    while (nbox--)
    {
	(*blt) (src + (pbox->y1 + dy) * srcStride,
		srcStride,
		(pbox->x1 + dx),

		dst + (pbox->y1) * dstStride,
		dstStride,
		(pbox->x1),

		(pbox->x2 - pbox->x1),
		(pbox->y2 - pbox->y1),

		pGC->alu,
		pPriv->pm);
	pbox++;
    }
}

PixmapPtr
fb24_32ReformatTile(PixmapPtr pOldTile, int bitsPerPixel)
{
    ScreenPtr	pScreen = pOldTile->drawable.pScreen;
    PixmapPtr	pNewTile;
    FbBits	*old, *new;
    FbStride	oldStride, newStride;
    int		oldBpp, newBpp;
    fb24_32BltFunc  blt;

    pNewTile = fbCreatePixmapBpp (pScreen,
				  pOldTile->drawable.width,
				  pOldTile->drawable.height,
				  pOldTile->drawable.depth,
				  bitsPerPixel);
    if (!pNewTile)
	return 0;
    fbGetDrawable (&pOldTile->drawable, 
		   old, oldStride, oldBpp);
    fbGetDrawable (&pNewTile->drawable, 
		   new, newStride, newBpp);
    if (oldBpp == 24)
	blt = fb24_32BltUp;
    else
	blt = fb24_32BltDown;

    (*blt) ((CARD8 *) old,
	    oldStride * sizeof (FbBits),
	    0,

	    (CARD8 *) new,
	    newStride * sizeof (FbBits),
	    0,

	    pOldTile->drawable.width,
	    pOldTile->drawable.height,

	    GXcopy,
	    FB_ALLONES);

    return pNewTile;
}

typedef struct {
    pointer pbits; 
    int width;   
} miScreenInitParmsRec, *miScreenInitParmsPtr;

Bool
fb24_32CreateScreenResources(ScreenPtr pScreen)
{
    miScreenInitParmsPtr pScrInitParms;
    int pitch;
    Bool retval;

    /* get the pitch before mi destroys it */
    pScrInitParms = (miScreenInitParmsPtr)pScreen->devPrivate;
    pitch = BitmapBytePad(pScrInitParms->width * 24);

    if((retval = miCreateScreenResources(pScreen))) {
	/* fix the screen pixmap */
	PixmapPtr pPix = (PixmapPtr)pScreen->devPrivate;
	pPix->drawable.bitsPerPixel = 24;
	pPix->devKind = pitch;
    }

    return retval;
}

Bool
fb24_32ModifyPixmapHeader (PixmapPtr   pPixmap,
			   int         width,
			   int         height,
			   int         depth,
			   int         bitsPerPixel,
			   int         devKind,
			   pointer     pPixData)
{
    int	    bpp, w;

    if (!pPixmap)
	return FALSE;
    bpp = bitsPerPixel;
    if (bpp <= 0)
	bpp = pPixmap->drawable.bitsPerPixel;
    if (bpp == 24)
    {
	if (devKind < 0)
	{
	    w = width;
	    if (w <= 0)
		w = pPixmap->drawable.width;
	    devKind = BitmapBytePad(w * 24);
	}
    }
    return miModifyPixmapHeader(pPixmap, width, height, depth, bitsPerPixel,
				devKind, pPixData);	
}		
