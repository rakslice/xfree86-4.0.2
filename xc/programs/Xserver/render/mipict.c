/*
 * $XFree86: xc/programs/Xserver/render/mipict.c,v 1.5 2000/12/07 06:11:30 keithp Exp $
 *
 * Copyright � 1999 Keith Packard
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

#include "scrnintstr.h"
#include "gcstruct.h"
#include "pixmapstr.h"
#include "windowstr.h"
#include "mi.h"
#include "picturestr.h"
#include "mipict.h"

int
miCreatePicture (PicturePtr pPicture)
{
    return Success;
}

void
miDestroyPicture (PicturePtr pPicture)
{
    if (pPicture->freeCompClip)
	REGION_DESTROY(pPicture->pDrawable->pScreen, pPicture->pCompositeClip);
}

void
miDestroyPictureClip (PicturePtr pPicture)
{
    switch (pPicture->clientClipType) {
    case CT_NONE:
	return;
    case CT_PIXMAP:
	(*pPicture->pDrawable->pScreen->DestroyPixmap) ((PixmapPtr) (pPicture->clientClip));
	break;
    default:
	/*
	 * we know we'll never have a list of rectangles, since ChangeClip
	 * immediately turns them into a region
	 */
	REGION_DESTROY(pPicture->pDrawable->pScreen, pPicture->clientClip);
	break;
    }
    pPicture->clientClip = NULL;
    pPicture->clientClipType = CT_NONE;
}    

int
miChangePictureClip (PicturePtr    pPicture,
		     int	   type,
		     pointer	   value,
		     int	   n)
{
    ScreenPtr		pScreen = pPicture->pDrawable->pScreen;
    PictureScreenPtr    ps = GetPictureScreen(pScreen);
    pointer		clientClip;
    int			clientClipType;
    
    switch (type) {
    case CT_PIXMAP:
	/* convert the pixmap to a region */
	clientClip = (pointer) BITMAP_TO_REGION(pScreen, (PixmapPtr) value);
	if (!clientClip)
	    return BadAlloc;
	clientClipType = CT_REGION;
	(*pScreen->DestroyPixmap) ((PixmapPtr) value);
	break;
    case CT_REGION:
	clientClip = value;
	clientClipType = CT_REGION;
	break;
    case CT_NONE:
	clientClip = 0;
	clientClipType = CT_NONE;
	break;
    default:
	clientClip = (pointer) RECTS_TO_REGION(pScreen, n,
					       (xRectangle *) value,
					       type);
	if (!clientClip)
	    return BadAlloc;
	clientClipType = CT_REGION;
	xfree(value);
	break;
    }
    (*ps->DestroyPictureClip) (pPicture);
    pPicture->clientClip = clientClip;
    pPicture->clientClipType = clientClipType;
    pPicture->stateChanges |= CPClipMask;
    return Success;
}

void
miChangePicture (PicturePtr pPicture,
		 Mask       mask)
{
    return;
}

void
miValidatePicture (PicturePtr pPicture,
		   Mask       mask)
{
    DrawablePtr	    pDrawable = pPicture->pDrawable;

    if ((mask & (CPClipXOrigin|CPClipYOrigin|CPClipMask|CPSubwindowMode)) ||
	(pDrawable->serialNumber != (pPicture->serialNumber & DRAWABLE_SERIAL_BITS)))
    {
	if (pDrawable->type == DRAWABLE_WINDOW)
	{
	    WindowPtr       pWin = (WindowPtr) pDrawable;
	    RegionPtr       pregWin;
	    Bool            freeTmpClip, freeCompClip;

	    if (pPicture->subWindowMode == IncludeInferiors)
	    {
		pregWin = NotClippedByChildren(pWin);
		freeTmpClip = TRUE;
	    }
	    else
	    {
		pregWin = &pWin->clipList;
		freeTmpClip = FALSE;
	    }
	    freeCompClip = pPicture->freeCompClip;

	    /*
	     * if there is no client clip, we can get by with just keeping the
	     * pointer we got, and remembering whether or not should destroy
	     * (or maybe re-use) it later.  this way, we avoid unnecessary
	     * copying of regions.  (this wins especially if many clients clip
	     * by children and have no client clip.)
	     */
	    if (pPicture->clientClipType == CT_NONE)
	    {
		if (freeCompClip)
		    REGION_DESTROY(pScreen, pPicture->pCompositeClip);
		pPicture->pCompositeClip = pregWin;
		pPicture->freeCompClip = freeTmpClip;
	    }
	    else
	    {
		/*
		 * we need one 'real' region to put into the composite clip. if
		 * pregWin the current composite clip are real, we can get rid of
		 * one. if pregWin is real and the current composite clip isn't,
		 * use pregWin for the composite clip. if the current composite
		 * clip is real and pregWin isn't, use the current composite
		 * clip. if neither is real, create a new region.
		 */

		REGION_TRANSLATE(pScreen, pPicture->clientClip,
				 pDrawable->x + pPicture->clipOrigin.x,
				 pDrawable->y + pPicture->clipOrigin.y);

		if (freeCompClip)
		{
		    REGION_INTERSECT(pPicture->pScreen, pPicture->pCompositeClip,
				     pregWin, pPicture->clientClip);
		    if (freeTmpClip)
			REGION_DESTROY(pScreen, pregWin);
		}
		else if (freeTmpClip)
		{
		    REGION_INTERSECT(pScreen, pregWin, pregWin, pPicture->clientClip);
		    pPicture->pCompositeClip = pregWin;
		}
		else
		{
		    pPicture->pCompositeClip = REGION_CREATE(pScreen, NullBox, 0);
		    REGION_INTERSECT(pScreen, pPicture->pCompositeClip,
				     pregWin, pPicture->clientClip);
		}
		pPicture->freeCompClip = TRUE;
		REGION_TRANSLATE(pScreen, pPicture->clientClip,
				 -(pDrawable->x + pPicture->clipOrigin.x),
				 -(pDrawable->y + pPicture->clipOrigin.y));
	    }
	}	/* end of composite clip for a window */
	else
	{
	    BoxRec          pixbounds;

	    /* XXX should we translate by drawable.x/y here ? */
	    /* If you want pixmaps in offscreen memory, yes */
	    pixbounds.x1 = pDrawable->x;
	    pixbounds.y1 = pDrawable->y;
	    pixbounds.x2 = pDrawable->x + pDrawable->width;
	    pixbounds.y2 = pDrawable->y + pDrawable->height;

	    if (pPicture->freeCompClip)
	    {
		REGION_RESET(pScreen, pPicture->pCompositeClip, &pixbounds);
	    }
	    else
	    {
		pPicture->freeCompClip = TRUE;
		pPicture->pCompositeClip = REGION_CREATE(pScreen, &pixbounds, 1);
	    }

	    if (pPicture->clientClipType == CT_REGION)
	    {
		if(pDrawable->x || pDrawable->y) {
		    REGION_TRANSLATE(pScreen, pPicture->clientClip,
				     pDrawable->x + pPicture->clipOrigin.x, 
				     pDrawable->y + pPicture->clipOrigin.y);
		    REGION_INTERSECT(pScreen, pPicture->pCompositeClip,
				     pPicture->pCompositeClip, pPicture->clientClip);
		    REGION_TRANSLATE(pScreen, pPicture->clientClip,
				     -(pDrawable->x + pPicture->clipOrigin.x), 
				     -(pDrawable->y + pPicture->clipOrigin.y));
		} else {
		    REGION_TRANSLATE(pScreen, pPicture->pCompositeClip,
				     -pPicture->clipOrigin.x, -pPicture->clipOrigin.y);
		    REGION_INTERSECT(pScreen, pPicture->pCompositeClip,
				     pPicture->pCompositeClip, pPicture->clientClip);
		    REGION_TRANSLATE(pScreen, pPicture->pCompositeClip,
				     pPicture->clipOrigin.x, pPicture->clipOrigin.y);
		}
	    }
	}	/* end of composite clip for pixmap */
    }
}

#define BOUND(v)	(INT16) ((v) < MINSHORT ? MINSHORT : (v) > MAXSHORT ? MAXSHORT : (v))

static void
miInitBox (BoxPtr   pBox,
	   INT16    x,
	   INT16    y,
	   CARD16   w, 
	   CARD16   h)
{
    int	    x1, y1, x2, y2;

    x1 = x;
    y1 = y;
    x2 = x1 + (int) w;
    y2 = y1 + (int) h;
    pBox->x1 = BOUND(x1);
    pBox->y1 = BOUND(y1);
    pBox->x2 = BOUND(x2);
    pBox->y2 = BOUND(y2);
}

Bool
miClipPicture (RegionPtr    pRegion,
	       PicturePtr   pPicture,
	       INT16	    xReg,
	       INT16	    yReg,
	       INT16	    xPict,
	       INT16	    yPict)
{
    if (pPicture->repeat)
    {
	if (pPicture->clientClipType != CT_NONE)
	{
	    REGION_TRANSLATE(pScreen, pRegion, 
			     xPict - xReg - pPicture->clipOrigin.x,
			     yPict - yReg - pPicture->clipOrigin.y);
	    if (!REGION_INTERSECT (pScreen, pRegion, pRegion, 
				   (RegionPtr) pPicture->clientClip))
		return FALSE;
	    REGION_TRANSLATE(pScreen, pRegion, 
			     - (xPict - xReg - pPicture->clipOrigin.x),
			     - (yPict - yReg - pPicture->clipOrigin.y));
	}
    }
    else
    {
	REGION_TRANSLATE(pScreen, pRegion, 
			 xPict - xReg,
			 yPict - yReg);
	if (!REGION_INTERSECT (pScreen, pRegion, pRegion,
			       pPicture->pCompositeClip))
	    return FALSE;
	REGION_TRANSLATE(pScreen, pRegion,
			 -(xPict - xReg),
			 -(yPict - yReg));
    }
    return TRUE;
}

Bool
miComputeCompositeRegion (RegionPtr	pRegion,
			  PicturePtr	pSrc,
			  PicturePtr	pMask,
			  PicturePtr	pDst,
			  INT16		xSrc,
			  INT16		ySrc,
			  INT16		xMask,
			  INT16		yMask,
			  INT16		xDst,
			  INT16		yDst,
			  CARD16	width,
			  CARD16	height)
{
    BoxRec	dstBox;
    RegionPtr	pDstClip = pDst->pCompositeClip;

    miInitBox (&dstBox, xDst, yDst, width, height);
    REGION_INIT (pScreen, pRegion, &dstBox, 1);
    if (!REGION_INTERSECT (pScreen, pRegion, pRegion, pDstClip))
    {
	REGION_UNINIT (pScreen, pRegion);
	return FALSE;
    }
    /* clip against src */
    if (!miClipPicture (pRegion, pSrc, xDst, yDst, xSrc, ySrc))
    {
	REGION_UNINIT (pScreen, pRegion);
	return FALSE;
    }
    /* clip against mask */
    if (pMask)
    {
	if (!miClipPicture (pRegion, pMask, xDst, yDst, xMask, yMask))
	{
	    REGION_UNINIT (pScreen, pRegion);
	    return FALSE;
	}	
    }
    return TRUE;
}

void
miRenderColorToPixel (PictFormatPtr format,
		      xRenderColor  *color,
		      CARD32	    *pixel)
{
    CARD32  r, g, b, a;
    
    switch (format->type) {
    case PictTypeDirect:
	r = color->red >> (16 - Ones (format->direct.redMask));
	g = color->green >> (16 - Ones (format->direct.greenMask));
	b = color->blue >> (16 - Ones (format->direct.blueMask));
	a = color->alpha >> (16 - Ones (format->direct.alphaMask));
	r = r << format->direct.red;
	g = g << format->direct.green;
	b = b << format->direct.blue;
	a = a << format->direct.alpha;
	*pixel = r|g|b|a;
	break;
    case PictTypeIndexed:
	*pixel = 0;
	break;
    }
}

static CARD16
miFillColor (CARD32 pixel, int bits)
{
    while (bits < 16)
    {
	pixel |= pixel << bits;
	bits <<= 1;
    }
    return (CARD16) pixel;
}

void
miRenderPixelToColor (PictFormatPtr format,
		      CARD32	    pixel,
		      xRenderColor  *color)
{
    CARD32  r, g, b, a;
    
    switch (format->type) {
    case PictTypeDirect:
	r = (pixel >> format->direct.red) & format->direct.redMask;
	g = (pixel >> format->direct.green) & format->direct.greenMask;
	b = (pixel >> format->direct.blue) & format->direct.blueMask;
	a = (pixel >> format->direct.alpha) & format->direct.alphaMask;
	color->red = miFillColor (r, Ones (format->direct.redMask));
	color->green = miFillColor (r, Ones (format->direct.greenMask));
	color->blue = miFillColor (r, Ones (format->direct.blueMask));
	color->alpha = miFillColor (r, Ones (format->direct.alphaMask));
	break;
    case PictTypeIndexed:
	color->red = 0;
	color->green = 0;
	color->blue = 0;
	color->alpha = 0;
	break;
    }
}

Bool
miPictureInit (ScreenPtr pScreen, PictFormatPtr formats, int nformats)
{
    PictureScreenPtr    ps;
    
    if (!PictureInit (pScreen, formats, nformats))
	return FALSE;
    ps = GetPictureScreen(pScreen);
    ps->CreatePicture = miCreatePicture;
    ps->DestroyPicture = miDestroyPicture;
    ps->ChangePictureClip = miChangePictureClip;
    ps->DestroyPictureClip = miDestroyPictureClip;
    ps->ChangePicture = miChangePicture;
    ps->ValidatePicture = miValidatePicture;
    return TRUE;
}
