/*
 * Id: fbsetsp.c,v 1.1 1999/11/02 03:54:45 keithp Exp $
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
/* $XFree86: xc/programs/Xserver/fb/fbsetsp.c,v 1.4 2000/05/06 21:09:34 keithp Exp $ */

#include "fb.h"

void
fbSetSpans (DrawablePtr	    pDrawable,
	    GCPtr	    pGC,
	    char	    *src,
	    DDXPointPtr	    ppt,
	    int		    *pwidth,
	    int		    nspans,
	    int		    fSorted)
{
    FbGCPrivPtr	    pPriv = fbGetGCPrivate (pGC);
    RegionPtr	    pClip = fbGetCompositeClip(pGC);
    FbBits	    *dst, *d, *s;
    FbStride	    dstStride;
    int		    dstBpp;
    BoxPtr	    pbox;
    int		    n;
    int		    xoff;
    int		    x1, x2;
    
#ifdef FB_24_32BIT
    if (pDrawable->bitsPerPixel != BitsPerPixel(pDrawable->depth))
    {
	fb24_32SetSpans (pDrawable, pGC, src, ppt, pwidth, nspans, fSorted);
	return;
    }
#endif
    fbGetDrawable (pDrawable, dst, dstStride, dstBpp);
    while (nspans--)
    {
	d = dst + ppt->y * dstStride;
	xoff = (int) (((long) src) & (FB_MASK >> 3));
	s = (FbBits *) (src - xoff);
	xoff <<= 3;
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
		    fbBlt ((FbBits *) s,
			   0,
			   (x1 - ppt->x) * dstBpp + xoff,
			   d,
			   dstStride,
			   x1 * dstBpp,

			   (x2 - x1) * dstBpp,
			   1,
			   pGC->alu,
			   pPriv->pm,
			   dstBpp,
			   
			   FALSE,
			   FALSE);
	    }
	}
	src += PixmapBytePad (*pwidth, pDrawable->depth);
	ppt++;
	pwidth++;
    }
    fbValidateDrawable (pDrawable);
}
	    
