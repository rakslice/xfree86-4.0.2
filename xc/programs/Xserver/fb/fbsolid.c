/*
 * Id: fbsolid.c,v 1.1 1999/11/02 03:54:45 keithp Exp $
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
/* $XFree86: xc/programs/Xserver/fb/fbsolid.c,v 1.7 2000/09/22 05:58:01 keithp Exp $ */

#define FbSelectPart(xor,o,t)    xor

#include "fb.h"

void
fbSolid (FbBits	    *dst,
	 FbStride   dstStride,
	 int	    dstX,
	 int	    bpp,

	 int	    width,
	 int	    height,

	 FbBits	    and,
	 FbBits	    xor)
{
    FbBits  startmask, endmask;
    int	    n, nmiddle;
    int	    startbyte, endbyte;

#ifdef FB_24BIT
    if (bpp == 24 && (!FbCheck24Pix(and) || !FbCheck24Pix(xor)))
    {
	fbSolid24 (dst, dstStride, dstX, width, height, and, xor);
	return;
    }
#endif
	
    dst += dstX >> FB_SHIFT;
    dstX &= FB_MASK;
    FbMaskBitsBytes(dstX, width, and == 0, startmask, startbyte, 
		    nmiddle, endmask, endbyte);
    if (startmask)
	dstStride--;
    dstStride -= nmiddle;
    while (height--)
    {
	if (startmask)
	{
	    FbDoLeftMaskByteRRop(dst,startbyte,startmask,and,xor);
	    dst++;
	}
	n = nmiddle;
	if (!and)
	    while (n--)
		*dst++ = xor;
	else
	    while (n--)
	    {
		*dst = FbDoRRop (*dst, and, xor);
		dst++;
	    }
	if (endmask)
	    FbDoRightMaskByteRRop(dst,endbyte,endmask,and,xor);
	dst += dstStride;
    }
}

#ifdef FB_24BIT
void
fbSolid24 (FbBits   *dst,
	   FbStride dstStride,
	   int	    dstX,

	   int	    width,
	   int	    height,

	   FbBits   and,
	   FbBits   xor)
{
    FbBits  startmask, endmask;
    FbBits  xor0, xor1, xor2;
    FbBits  and0, and1, and2;
    FbBits  xorS, andS, xorE, andE;
    int	    n, nmiddle;
    int	    rotS, rot;

    dst += dstX >> FB_SHIFT;
    dstX &= FB_MASK;
    /*
     * Rotate pixel values this far across the word to align on
     * screen pixel boundaries
     */
    rot = FbFirst24Rot (dstX);
    FbMaskBits (dstX, width, startmask, nmiddle, endmask);
    if (startmask)
	dstStride--;
    dstStride -= nmiddle;
    
    /*
     * Precompute rotated versions of the rasterop values
     */
    rotS = rot;
    xor = FbRot24(xor,rotS);
    and = FbRot24(and,rotS);
    if (startmask)
    {
	xorS = xor;
	andS = and;
	xor = FbNext24Pix(xor);
	and = FbNext24Pix(and);
    }
    
    if (nmiddle)
    {
	xor0 = xor;
	and0 = and;
	xor1 = FbNext24Pix(xor0);
	and1 = FbNext24Pix(and0);
	xor2 = FbNext24Pix(xor1);
	and2 = FbNext24Pix(and1);
    }
    
    if (endmask)
    {
	switch (nmiddle % 3) {
	case 0:
	    xorE = xor;
	    andE = and;
	    break;
	case 1:
	    xorE = xor1;
	    andE = and1;
	    break;
	case 2:
	    xorE = xor2;
	    andE = and2;
	    break;
	}
    }
    
    while (height--)
    {
	if (startmask)
	{
	    *dst = FbDoMaskRRop(*dst, andS, xorS, startmask);
	    dst++;
	}
	n = nmiddle;
	if (!and0)
	{
	    while (n >= 3)
	    {
		*dst++ = xor0;
		*dst++ = xor1;
		*dst++ = xor2;
		n -= 3;
	    }
	    if (n)
	    {
		*dst++ = xor0;
		n--;
		if (n)
		{
		    *dst++ = xor1;
		}
	    }
	}
	else
	{
	    while (n >= 3)
	    {
		*dst = FbDoRRop (*dst, and0, xor0);
		dst++;
		*dst = FbDoRRop (*dst, and1, xor1);
		dst++;
		*dst = FbDoRRop (*dst, and2, xor2);
		dst++;
		n -= 3;
	    }
	    if (n)
	    {
		*dst = FbDoRRop (*dst, and0, xor0);
		dst++;
		n--;
		if (n)
		{
		    *dst = FbDoRRop (*dst, and1, xor1);
		    dst++;
		}
	    }
	}
	if (endmask)
	    *dst = FbDoMaskRRop (*dst, andE, xorE, endmask);
	dst += dstStride;
    }
}
#endif
