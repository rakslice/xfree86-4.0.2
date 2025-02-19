/*
 * $XFree86: xc/programs/Xserver/render/render.c,v 1.8 2000/12/05 03:13:34 keithp Exp $
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

#define NEED_REPLIES
#define NEED_EVENTS
#include "X.h"
#include "Xproto.h"
#include "misc.h"
#include "os.h"
#include "dixstruct.h"
#include "resource.h"
#include "scrnintstr.h"
#include "windowstr.h"
#include "pixmapstr.h"
#include "colormapst.h"
#include "extnsionst.h"
#include "servermd.h"
#include "render.h"
#include "renderproto.h"
#include "picturestr.h"
#include "glyphstr.h"
#include "Xfuncproto.h"
#ifdef EXTMODULE
#include "xf86_ansic.h"
#endif

static int ProcRenderQueryVersion (ClientPtr pClient);
static int ProcRenderQueryPictFormats (ClientPtr pClient);
static int ProcRenderQueryPictIndexValues (ClientPtr pClient);
static int ProcRenderQueryDithers (ClientPtr pClient);
static int ProcRenderCreatePicture (ClientPtr pClient);
static int ProcRenderChangePicture (ClientPtr pClient);
static int ProcRenderSetPictureClipRectangles (ClientPtr pClient);
static int ProcRenderFreePicture (ClientPtr pClient);
static int ProcRenderComposite (ClientPtr pClient);
static int ProcRenderScale (ClientPtr pClient);
static int ProcRenderTrapezoids (ClientPtr pClient);
static int ProcRenderTriangles (ClientPtr pClient);
static int ProcRenderTriStrip (ClientPtr pClient);
static int ProcRenderTriFan (ClientPtr pClient);
static int ProcRenderColorTrapezoids (ClientPtr pClient);
static int ProcRenderColorTriangles (ClientPtr pClient);
static int ProcRenderTransform (ClientPtr pClient);
static int ProcRenderCreateGlyphSet (ClientPtr pClient);
static int ProcRenderReferenceGlyphSet (ClientPtr pClient);
static int ProcRenderFreeGlyphSet (ClientPtr pClient);
static int ProcRenderAddGlyphs (ClientPtr pClient);
static int ProcRenderAddGlyphsFromPicture (ClientPtr pClient);
static int ProcRenderFreeGlyphs (ClientPtr pClient);
static int ProcRenderCompositeGlyphs (ClientPtr pClient, int size);
static int ProcRenderDispatch (ClientPtr pClient);
static int SProcRenderQueryVersion (ClientPtr pClient);
static int SProcRenderQueryPictFormats (ClientPtr pClient);
static int SProcRenderQueryPictIndexValues (ClientPtr pClient);
static int SProcRenderQueryDithers (ClientPtr pClient);
static int SProcRenderCreatePicture (ClientPtr pClient);
static int SProcRenderChangePicture (ClientPtr pClient);
static int SProcRenderSetPictureClipRectangles (ClientPtr pClient);
static int SProcRenderFreePicture (ClientPtr pClient);
static int SProcRenderComposite (ClientPtr pClient);
static int SProcRenderScale (ClientPtr pClient);
static int SProcRenderTrapezoids (ClientPtr pClient);
static int SProcRenderTriangles (ClientPtr pClient);
static int SProcRenderTriStrip (ClientPtr pClient);
static int SProcRenderTriFan (ClientPtr pClient);
static int SProcRenderColorTrapezoids (ClientPtr pClient);
static int SProcRenderColorTriangles (ClientPtr pClient);
static int SProcRenderTransform (ClientPtr pClient);
static int SProcRenderCreateGlyphSet (ClientPtr pClient);
static int SProcRenderReferenceGlyphSet (ClientPtr pClient);
static int SProcRenderFreeGlyphSet (ClientPtr pClient);
static int SProcRenderAddGlyphs (ClientPtr pClient);
static int SProcRenderAddGlyphsFromPicture (ClientPtr pClient);
static int SProcRenderFreeGlyphs (ClientPtr pClient);
static int SProcRenderDispatch (ClientPtr pClient);

static void
RenderResetProc (ExtensionEntry *extEntry);
    
static CARD8	RenderReqCode;
static int	RenderErrBase;

void
RenderExtensionInit (void)
{
    ExtensionEntry *extEntry;

    if (!PictureType)
	return;
    extEntry = AddExtension (RENDER_NAME, 0, RenderNumberErrors,
			     ProcRenderDispatch, SProcRenderDispatch,
			     RenderResetProc, StandardMinorOpcode);
    if (!extEntry)
	return;
    RenderReqCode = (CARD8) extEntry->base;
    RenderErrBase = extEntry->errorBase;							
}

static void
RenderResetProc (ExtensionEntry *extEntry)
{
}

static int
ProcRenderQueryVersion (ClientPtr client)
{
    xRenderQueryVersionReply rep;
    register int n;
/*    REQUEST(xRenderQueryVersionReq); */

    REQUEST_SIZE_MATCH(xRenderQueryVersionReq);
    rep.type = X_Reply;
    rep.length = 0;
    rep.sequenceNumber = client->sequence;
    rep.majorVersion = RENDER_MAJOR;
    rep.minorVersion = RENDER_MINOR;
    if (client->swapped) {
    	swaps(&rep.sequenceNumber, n);
    	swapl(&rep.length, n);
	swapl(&rep.majorVersion, n);
	swapl(&rep.minorVersion, n);
    }
    WriteToClient(client, sizeof(xRenderQueryVersionReply), (char *)&rep);
    return (client->noClientException);
}

#if 0
static int
VisualDepth (ScreenPtr pScreen, VisualPtr pVisual)
{
    DepthPtr    pDepth;
    int		d, v;

    for (d = 0; d < pScreen->numDepths; d++)
    {
	pDepth = pScreen->allowedDepths + d;
	for (v = 0; v < pDepth->numVids; v++)
	{
	    if (pDepth->vids[v] == pVisual->vid)
		return pDepth->depth;
	}
    }
    return 0;
}
#endif

static VisualPtr
findVisual (ScreenPtr pScreen, VisualID vid)
{
    VisualPtr	pVisual;
    int		v;

    for (v = 0; v < pScreen->numVisuals; v++)
    {
	pVisual = pScreen->visuals + v;
	if (pVisual->vid == vid)
	    return pVisual;
    }
    return 0;
}

static int
ProcRenderQueryPictFormats (ClientPtr client)
{
    xRenderQueryPictFormatsReply    *reply;
    xPictScreen			    *pictScreen;
    xPictDepth			    *pictDepth;
    xPictVisual			    *pictVisual;
    xPictFormInfo		    *pictForm;
    ScreenPtr			    pScreen;
    VisualPtr			    pVisual;
    DepthPtr			    pDepth;
    int				    v, d;
    PictureScreenPtr		    ps;
    PictFormatPtr		    pFormat;
    int				    nformat;
    int				    ndepth;
    int				    nvisual;
    int				    rlength;
    int				    s;
    int				    n;
/*    REQUEST(xRenderQueryPictFormatsReq); */

    REQUEST_SIZE_MATCH(xRenderQueryPictFormatsReq);

    ndepth = nformat = nvisual = 0;
    for (s = 0; s < screenInfo.numScreens; s++)
    {
	pScreen = screenInfo.screens[s];
	for (d = 0; d < pScreen->numDepths; d++)
	{
	    pDepth = pScreen->allowedDepths + d;
	    ++ndepth;

	    for (v = 0; v < pDepth->numVids; v++)
	    {
		pVisual = findVisual (pScreen, pDepth->vids[v]);
		if (pVisual && PictureMatchVisual (pScreen, pDepth->depth, pVisual))
		    ++nvisual;
	    }
	}
	ps = GetPictureScreenIfSet(pScreen);
	if (ps)
	    nformat += ps->nformats;
    }
    rlength = (sizeof (xRenderQueryPictFormatsReply) +
	       nformat * sizeof (xPictFormInfo) +
	       screenInfo.numScreens * sizeof (xPictScreen) +
	       ndepth * sizeof (xPictDepth) +
	       nvisual * sizeof (xPictVisual));
    reply = (xRenderQueryPictFormatsReply *) xalloc (rlength);
    if (!reply)
	return BadAlloc;
    reply->type = X_Reply;
    reply->sequenceNumber = client->sequence;
    reply->length = (rlength - sizeof(xGenericReply)) >> 2;
    reply->numFormats = nformat;
    reply->numScreens = screenInfo.numScreens;
    reply->numDepths = ndepth;
    reply->numVisuals = nvisual;
    
    pictForm = (xPictFormInfo *) (reply + 1);
    
    for (s = 0; s < screenInfo.numScreens; s++)
    {
	pScreen = screenInfo.screens[s];
	ps = GetPictureScreenIfSet(pScreen);
	if (ps)
	{
	    for (nformat = 0, pFormat = ps->formats; 
		 nformat < ps->nformats;
		 nformat++, pFormat++)
	    {
		pictForm->id = pFormat->id;
		pictForm->type = pFormat->type;
		pictForm->depth = pFormat->depth;
		pictForm->direct.red = pFormat->direct.red;
		pictForm->direct.redMask = pFormat->direct.redMask;
		pictForm->direct.green = pFormat->direct.green;
		pictForm->direct.greenMask = pFormat->direct.greenMask;
		pictForm->direct.blue = pFormat->direct.blue;
		pictForm->direct.blueMask = pFormat->direct.blueMask;
		pictForm->direct.alpha = pFormat->direct.alpha;
		pictForm->direct.alphaMask = pFormat->direct.alphaMask;
		if (pFormat->pColormap)
		    pictForm->colormap = pFormat->pColormap->mid;
		else
		    pictForm->colormap = None;
		if (client->swapped)
		{
		    swapl (&pictForm->id, n);
		    swaps (&pictForm->direct.red, n);
		    swaps (&pictForm->direct.redMask, n);
		    swaps (&pictForm->direct.green, n);
		    swaps (&pictForm->direct.greenMask, n);
		    swaps (&pictForm->direct.blue, n);
		    swaps (&pictForm->direct.blueMask, n);
		    swaps (&pictForm->direct.alpha, n);
		    swaps (&pictForm->direct.alphaMask, n);
		    swapl (&pictForm->colormap, n);
		}
		pictForm++;
	    }
	}
    }
    
    pictScreen = (xPictScreen *) pictForm;
    for (s = 0; s < screenInfo.numScreens; s++)
    {
	pScreen = screenInfo.screens[s];
	pictDepth = (xPictDepth *) (pictScreen + 1);
	ndepth = 0;
	for (d = 0; d < pScreen->numDepths; d++)
	{
	    pictVisual = (xPictVisual *) (pictDepth + 1);
	    pDepth = pScreen->allowedDepths + d;

	    nvisual = 0;
	    for (v = 0; v < pDepth->numVids; v++)
	    {
		pVisual = findVisual (pScreen, pDepth->vids[v]);
		if (pVisual && (pFormat = PictureMatchVisual (pScreen, 
							      pDepth->depth, 
							      pVisual)))
		{
		    pictVisual->visual = pVisual->vid;
		    pictVisual->format = pFormat->id;
		    if (client->swapped)
		    {
			swapl (&pictVisual->visual, n);
			swapl (&pictVisual->format, n);
		    }
		    pictVisual++;
		    nvisual++;
		}
	    }
	    pictDepth->depth = pDepth->depth;
	    pictDepth->nPictVisuals = nvisual;
	    if (client->swapped)
	    {
		swaps (&pictDepth->nPictVisuals, n);
	    }
	    ndepth++;
	    pictDepth = (xPictDepth *) pictVisual;
	}
	pictScreen->nDepth = ndepth;
	ps = GetPictureScreenIfSet(pScreen);
	if (ps)
	    pictScreen->fallback = ps->fallback->id;
	else
	    pictScreen->fallback = 0;
	if (client->swapped)
	{
	    swapl (&pictScreen->nDepth, n);
	    swapl (&pictScreen->fallback, n);
	}
	pictScreen = (xPictScreen *) pictDepth;
    }
    if (client->swapped)
    {
	swaps (&reply->sequenceNumber, n);
	swapl (&reply->length, n);
	swapl (&reply->numFormats, n);
	swapl (&reply->numScreens, n);
	swapl (&reply->numDepths, n);
	swapl (&reply->numVisuals, n);
    }
    WriteToClient(client, rlength, (char *) reply);
    xfree (reply);
    return client->noClientException;
}

static int
ProcRenderQueryPictIndexValues (ClientPtr client)
{
    return BadImplementation;
}

static int
ProcRenderQueryDithers (ClientPtr client)
{
    return BadImplementation;
}

static int
ProcRenderCreatePicture (ClientPtr client)
{
    PicturePtr	    pPicture;
    DrawablePtr	    pDrawable;
    PictFormatPtr   pFormat;
    int		    len;
    int		    error;
    REQUEST(xRenderCreatePictureReq);

    REQUEST_AT_LEAST_SIZE(xRenderCreatePictureReq);

    LEGAL_NEW_RESOURCE(stuff->pid, client);
    SECURITY_VERIFY_DRAWABLE(pDrawable, stuff->drawable, client,
			     SecurityWriteAccess);
    pFormat = (PictFormatPtr) SecurityLookupIDByType (client, 
						      stuff->format,
						      PictFormatType,
						      SecurityReadAccess);
    if (!pFormat)
    {
	client->errorValue = stuff->format;
	return RenderErrBase + BadPictFormat;
    }
    if (pFormat->depth != pDrawable->depth)
	return BadMatch;
    len = client->req_len - (sizeof(xRenderCreatePictureReq) >> 2);
    if (Ones(stuff->mask) != len)
	return BadLength;
    
    pPicture = CreatePicture (stuff->pid,
			      pDrawable,
			      pFormat,
			      stuff->mask,
			      (XID *) (stuff + 1),
			      client,
			      &error);
    if (!pPicture)
	return error;
    if (!AddResource (stuff->pid, PictureType, (pointer)pPicture))
	return BadAlloc;
    return Success;
}

static int
ProcRenderChangePicture (ClientPtr client)
{
    PicturePtr	    pPicture;
    REQUEST(xRenderChangePictureReq);
    int len;

    REQUEST_AT_LEAST_SIZE(xRenderChangePictureReq);
    VERIFY_PICTURE (pPicture, stuff->picture, client, SecurityWriteAccess,
		    RenderErrBase + BadPicture);
    len = client->req_len - (sizeof(xRenderChangePictureReq) >> 2);
    if (Ones(stuff->mask) != len)
	return BadLength;
    
    return ChangePicture (pPicture, stuff->mask, (XID *) (stuff + 1),
			  (DevUnion *) 0, client);
}

static int
ProcRenderSetPictureClipRectangles (ClientPtr client)
{
    REQUEST(xRenderSetPictureClipRectanglesReq);
    PicturePtr	    pPicture;
    int		    nr;
    int		    result;

    REQUEST_AT_LEAST_SIZE(xRenderSetPictureClipRectanglesReq);
    VERIFY_PICTURE (pPicture, stuff->picture, client, SecurityWriteAccess,
		    RenderErrBase + BadPicture);
    nr = (client->req_len << 2) - sizeof(xRenderChangePictureReq);
    if (nr & 4)
	return BadLength;
    nr >>= 3;
    result = SetPictureClipRects (pPicture, 
				  stuff->xOrigin, stuff->yOrigin,
				  nr, (xRectangle *) &stuff[1]);
    if (client->noClientException != Success)
        return(client->noClientException);
    else
        return(result);
}

static int
ProcRenderFreePicture (ClientPtr client)
{
    PicturePtr	pPicture;
    REQUEST(xRenderFreePictureReq);

    REQUEST_SIZE_MATCH(xRenderFreePictureReq);

    VERIFY_PICTURE (pPicture, stuff->picture, client, SecurityDestroyAccess,
		    RenderErrBase + BadPicture);
    FreeResource (stuff->picture, RT_NONE);
    return(client->noClientException);
}

static int
ProcRenderComposite (ClientPtr client)
{
    PicturePtr	pSrc, pMask, pDst;
    REQUEST(xRenderCompositeReq);

    if (stuff->op > PictOpMaximum)
    {
	client->errorValue = stuff->op;
	return BadValue;
    }
    VERIFY_PICTURE (pSrc, stuff->src, client, SecurityReadAccess, 
		    RenderErrBase + BadPicture);
    VERIFY_ALPHA (pMask, stuff->mask, client, SecurityReadAccess, 
		  RenderErrBase + BadPicture);
    VERIFY_PICTURE (pDst, stuff->dst, client, SecurityWriteAccess, 
		    RenderErrBase + BadPicture);
    if (pSrc->pDrawable->pScreen != pDst->pDrawable->pScreen ||
	(pMask && pSrc->pDrawable->pScreen != pMask->pDrawable->pScreen))
	return BadMatch;
    CompositePicture (stuff->op,
		      pSrc,
		      pMask,
		      pDst,
		      stuff->xSrc,
		      stuff->ySrc,
		      stuff->xMask,
		      stuff->yMask,
		      stuff->xDst,
		      stuff->yDst,
		      stuff->width,
		      stuff->height);
    return Success;
}

static int
ProcRenderScale (ClientPtr client)
{
    return BadImplementation;
}

static int
ProcRenderTrapezoids (ClientPtr client)
{
    return BadImplementation;
}

static int
ProcRenderTriangles (ClientPtr client)
{
    return BadImplementation;
}

static int
ProcRenderTriStrip (ClientPtr client)
{
    return BadImplementation;
}

static int
ProcRenderTriFan (ClientPtr client)
{
    return BadImplementation;
}

static int
ProcRenderColorTrapezoids (ClientPtr client)
{
    return BadImplementation;
}

static int
ProcRenderColorTriangles (ClientPtr client)
{
    return BadImplementation;
}

static int
ProcRenderTransform (ClientPtr client)
{
    return BadImplementation;
}

static int
ProcRenderCreateGlyphSet (ClientPtr client)
{
    GlyphSetPtr	    glyphSet;
    PictFormatPtr   format;
    int		    f;
    REQUEST(xRenderCreateGlyphSetReq);

    REQUEST_SIZE_MATCH(xRenderCreateGlyphSetReq);

    LEGAL_NEW_RESOURCE(stuff->gsid, client);
    format = (PictFormatPtr) SecurityLookupIDByType (client,
						     stuff->format,
						     PictFormatType,
						     SecurityReadAccess);
    if (!format)
    {
	client->errorValue = stuff->format;
	return RenderErrBase + BadPictFormat;
    }
    switch (format->depth) {
    case 1:
	f = GlyphFormat1;
	break;
    case 4:
	f = GlyphFormat4;
	break;
    case 8:
	f = GlyphFormat8;
	break;
    case 16:
	f = GlyphFormat16;
	break;
    case 32:
	f = GlyphFormat32;
	break;
    default:
	return BadMatch;
    }
    if (format->type != PictTypeDirect)
	return BadMatch;
    glyphSet = AllocateGlyphSet (f, format);
    if (!glyphSet)
	return BadAlloc;
    if (!AddResource (stuff->gsid, GlyphSetType, (pointer)glyphSet))
	return BadAlloc;
    return Success;
}

static int
ProcRenderReferenceGlyphSet (ClientPtr client)
{
    GlyphSetPtr     glyphSet;
    REQUEST(xRenderReferenceGlyphSetReq);

    REQUEST_SIZE_MATCH(xRenderReferenceGlyphSetReq);

    LEGAL_NEW_RESOURCE(stuff->gsid, client);

    glyphSet = (GlyphSetPtr) SecurityLookupIDByType (client,
						     stuff->existing,
						     GlyphSetType,
						     SecurityWriteAccess);
    if (!glyphSet)
    {
	client->errorValue = stuff->existing;
	return RenderErrBase + BadGlyphSet;
    }
    glyphSet->refcnt++;
    if (!AddResource (stuff->gsid, GlyphSetType, (pointer)glyphSet))
	return BadAlloc;
    return client->noClientException;
}

#define NLOCALDELTA	64
#define NLOCALGLYPH	256

static int
ProcRenderFreeGlyphSet (ClientPtr client)
{
    GlyphSetPtr     glyphSet;
    REQUEST(xRenderFreeGlyphSetReq);

    REQUEST_SIZE_MATCH(xRenderFreeGlyphSetReq);
    glyphSet = (GlyphSetPtr) SecurityLookupIDByType (client,
						     stuff->glyphset,
						     GlyphSetType,
						     SecurityDestroyAccess);
    if (!glyphSet)
    {
	client->errorValue = stuff->glyphset;
	return RenderErrBase + BadGlyphSet;
    }
    FreeResource (stuff->glyphset, RT_NONE);
    return client->noClientException;
}

typedef struct _GlyphNew {
    Glyph	id;
    GlyphPtr    glyph;
} GlyphNewRec, *GlyphNewPtr;

static int
ProcRenderAddGlyphs (ClientPtr client)
{
    GlyphSetPtr     glyphSet;
    REQUEST(xRenderAddGlyphsReq);
    GlyphNewRec	    glyphsLocal[NLOCALGLYPH];
    GlyphNewPtr	    glyphsBase, glyphs;
    GlyphPtr	    glyph;
    int		    remain, nglyphs;
    CARD32	    *gids;
    xGlyphInfo	    *gi;
    CARD8	    *bits;
    int		    size;
    int		    err = BadAlloc;

    REQUEST_AT_LEAST_SIZE(xRenderAddGlyphsReq);
    glyphSet = (GlyphSetPtr) SecurityLookupIDByType (client,
						     stuff->glyphset,
						     GlyphSetType,
						     SecurityWriteAccess);
    if (!glyphSet)
    {
	client->errorValue = stuff->glyphset;
	return RenderErrBase + BadGlyphSet;
    }

    nglyphs = stuff->nglyphs;
    if (nglyphs <= NLOCALGLYPH)
	glyphsBase = glyphsLocal;
    else
    {
	glyphsBase = (GlyphNewPtr) ALLOCATE_LOCAL (nglyphs * sizeof (GlyphNewRec));
	if (!glyphsBase)
	    return BadAlloc;
    }

    remain = (stuff->length << 2) - sizeof (xRenderAddGlyphsReq);

    glyphs = glyphsBase;

    gids = (CARD32 *) (stuff + 1);
    gi = (xGlyphInfo *) (gids + nglyphs);
    bits = (CARD8 *) (gi + nglyphs);
    remain -= (sizeof (CARD32) + sizeof (xGlyphInfo)) * nglyphs;
    while (remain >= 0 && nglyphs)
    {
	glyph = AllocateGlyph (gi, glyphSet->fdepth);
	if (!glyph)
	{
	    err = BadAlloc;
	    goto bail;
	}
	
	glyphs->glyph = glyph;
	glyphs->id = *gids;	
	
	size = glyph->size - sizeof (xGlyphInfo);
	if (remain < size)
	    break;
	memcpy ((CARD8 *) (glyph + 1), bits, size);
	
	if (size & 3)
	    size += 4 - (size & 3);
	bits += size;
	remain -= size;
	gi++;
	gids++;
	glyphs++;
	nglyphs--;
    }
    if (nglyphs || remain)
    {
	err = BadLength;
	goto bail;
    }
    nglyphs = stuff->nglyphs;
    if (!ResizeGlyphSet (glyphSet, nglyphs))
    {
	err = BadAlloc;
	goto bail;
    }
    glyphs = glyphsBase;
    while (nglyphs--)
	AddGlyph (glyphSet, glyphs->glyph, glyphs->id);

    if (glyphsBase != glyphsLocal)
	DEALLOCATE_LOCAL (glyphsBase);
    return client->noClientException;
bail:
    while (glyphs != glyphsBase)
    {
	--glyphs;
	xfree (glyphs->glyph);
    }
    if (glyphsBase != glyphsLocal)
	DEALLOCATE_LOCAL (glyphsBase);
    return err;
}

static int
ProcRenderAddGlyphsFromPicture (ClientPtr client)
{
    return BadImplementation;
}

static int
ProcRenderFreeGlyphs (ClientPtr client)
{
    return BadImplementation;
}

static int
ProcRenderCompositeGlyphs (ClientPtr client, int size)
{
    GlyphSetPtr     glyphSet;
    GlyphSet	    gs;
    PicturePtr      pSrc, pDst;
    PictFormatPtr   pFormat;
    GlyphListRec    listsLocal[NLOCALDELTA];
    GlyphListPtr    lists, listsBase;
    GlyphPtr	    glyphsLocal[NLOCALGLYPH];
    Glyph	    glyph;
    GlyphPtr	    *glyphs, *glyphsBase;
    xGlyphElt	    *elt;
    CARD8	    *buffer, *end;
    int		    nglyph;
    int		    nlist;
    int		    space;
    int		    n;
    
    REQUEST(xRenderCompositeGlyphsReq);

    REQUEST_AT_LEAST_SIZE(xRenderCompositeGlyphsReq);

    VERIFY_PICTURE (pSrc, stuff->src, client, SecurityReadAccess,
		    RenderErrBase + BadPicture);
    VERIFY_PICTURE (pDst, stuff->dst, client, SecurityWriteAccess,
		    RenderErrBase + BadPicture);
    if (pSrc->pDrawable->pScreen != pDst->pDrawable->pScreen)
	return BadMatch;
    if (stuff->maskFormat)
    {
	pFormat = (PictFormatPtr) SecurityLookupIDByType (client,
							  stuff->maskFormat,
							  PictFormatType,
							  SecurityReadAccess);
	if (!pFormat)
	{
	    client->errorValue = stuff->maskFormat;
	    return RenderErrBase + BadPictFormat;
	}
    }
    else
	pFormat = 0;

    glyphSet = (GlyphSetPtr) SecurityLookupIDByType (client,
						     stuff->glyphset,
						     GlyphSetType,
						     SecurityReadAccess);
    if (!glyphSet)
    {
	client->errorValue = stuff->glyphset;
	return RenderErrBase + BadGlyphSet;
    }

    buffer = (CARD8 *) (stuff + 1);
    end = (CARD8 *) stuff + (stuff->length << 2);
    nglyph = 0;
    nlist = 0;
    while (buffer + sizeof (xGlyphElt) < end)
    {
	elt = (xGlyphElt *) buffer;
	buffer += sizeof (xGlyphElt);
	
	if (elt->len == 0xff)
	{
	    buffer += 4;
	}
	else
	{
	    nlist++;
	    nglyph += elt->len;
	    space = size * elt->len;
	    if (space & 3)
		space += 4 - (space & 3);
	    buffer += space;
	}
    }
    if (nglyph <= NLOCALGLYPH)
	glyphsBase = glyphsLocal;
    else
    {
	glyphsBase = (GlyphPtr *) ALLOCATE_LOCAL (nglyph * sizeof (GlyphPtr));
	if (!glyphsBase)
	    return BadAlloc;
    }
    if (nlist <= NLOCALDELTA)
	listsBase = listsLocal;
    else
    {
	listsBase = (GlyphListPtr) ALLOCATE_LOCAL (nlist * sizeof (GlyphListRec));
	if (!listsBase)
	    return BadAlloc;
    }
    buffer = (CARD8 *) (stuff + 1);
    glyphs = glyphsBase;
    lists = listsBase;
    while (buffer + sizeof (xGlyphElt) < end)
    {
	elt = (xGlyphElt *) buffer;
	buffer += sizeof (xGlyphElt);
	
	if (elt->len == 0xff)
	{
	    if (buffer + sizeof (GlyphSet) < end)
	    {
		gs = *(GlyphSet *) buffer;
		glyphSet = (GlyphSetPtr) SecurityLookupIDByType (client,
								 gs,
								 GlyphSetType,
								 SecurityReadAccess);
		if (!glyphSet)
		{
		    client->errorValue = gs;
		    if (glyphsBase != glyphsLocal)
			DEALLOCATE_LOCAL (glyphsBase);
		    if (listsBase != listsLocal)
			DEALLOCATE_LOCAL (listsBase);
		    return RenderErrBase + BadGlyphSet;
		}
	    }
	    buffer += 4;
	}
	else
	{
	    lists->xOff = elt->deltax;
	    lists->yOff = elt->deltay;
	    lists->format = glyphSet->format;
	    lists->len = 0;
	    n = elt->len;
	    while (n--)
	    {
		if (buffer + size <= end)
		{
		    switch (size) {
		    case 1:
			glyph = *((CARD8 *)buffer); break;
		    case 2:
			glyph = *((CARD16 *)buffer); break;
		    case 4:
		    default:
			glyph = *((CARD32 *)buffer); break;
		    }
		    if ((*glyphs = FindGlyph (glyphSet, glyph)))
		    {
			lists->len++;
			glyphs++;
		    }
		}
		buffer += size;
	    }
	    space = size * elt->len;
	    if (space & 3)
		buffer += 4 - (space & 3);
	    lists++;
	}
    }
    if (buffer > end)
	return BadLength;

    CompositeGlyphs (stuff->op,
		     pSrc,
		     pDst,
		     pFormat,
		     stuff->xSrc,
		     stuff->ySrc,
		     nlist,
		     listsBase,
		     glyphsBase);

    if (glyphsBase != glyphsLocal)
	DEALLOCATE_LOCAL (glyphsBase);
    if (listsBase != listsLocal)
	DEALLOCATE_LOCAL (listsBase);
    
    return client->noClientException;
}

static int
ProcRenderFillRectangles (ClientPtr client)
{
    PicturePtr	    pDst;
    int             things;
    REQUEST(xRenderFillRectanglesReq);
    
    if (stuff->op > PictOpMaximum)
    {
	client->errorValue = stuff->op;
	return BadValue;
    }
    VERIFY_PICTURE (pDst, stuff->dst, client, SecurityWriteAccess, 
		    RenderErrBase + BadPicture);
    
    things = (client->req_len << 2) - sizeof(xRenderFillRectanglesReq);
    if (things & 4)
	return(BadLength);
    things >>= 3;
    
    CompositeRects (stuff->op,
		    pDst,
		    &stuff->color,
		    things,
		    (xRectangle *) &stuff[1]);
    
    return client->noClientException;
}

static int
ProcRenderDispatch (ClientPtr client)
{
    REQUEST(xReq);
    switch (stuff->data)
    {
    case X_RenderQueryVersion:
	return ProcRenderQueryVersion(client);
    case X_RenderQueryPictFormats:
	return ProcRenderQueryPictFormats(client);
    case X_RenderQueryPictIndexValues:
	return ProcRenderQueryPictIndexValues(client);
    case X_RenderQueryDithers:
	return ProcRenderQueryDithers(client);
    case X_RenderCreatePicture:
	return ProcRenderCreatePicture(client);
    case X_RenderChangePicture:
	return ProcRenderChangePicture(client);
    case X_RenderSetPictureClipRectangles:
	return ProcRenderSetPictureClipRectangles(client);
    case X_RenderFreePicture:
	return ProcRenderFreePicture(client);
    case X_RenderComposite:
	return ProcRenderComposite(client);
    case X_RenderScale:
	return ProcRenderScale(client);
    case X_RenderTrapezoids:
	return ProcRenderTrapezoids(client);
    case X_RenderTriangles:
	return ProcRenderTriangles(client);
    case X_RenderTriStrip:
	return ProcRenderTriStrip(client);
    case X_RenderTriFan:
	return ProcRenderTriFan(client);
    case X_RenderColorTrapezoids:
	return ProcRenderColorTrapezoids(client);
    case X_RenderColorTriangles:
	return ProcRenderColorTriangles(client);
    case X_RenderTransform:
	return ProcRenderTransform(client);
    case X_RenderCreateGlyphSet:
	return ProcRenderCreateGlyphSet(client);
    case X_RenderReferenceGlyphSet:
	return ProcRenderReferenceGlyphSet(client);
    case X_RenderFreeGlyphSet:
	return ProcRenderFreeGlyphSet(client);
    case X_RenderAddGlyphs:
	return ProcRenderAddGlyphs(client);
    case X_RenderAddGlyphsFromPicture:
	return ProcRenderAddGlyphsFromPicture(client);
    case X_RenderFreeGlyphs:
	return ProcRenderFreeGlyphs(client);
    case X_RenderCompositeGlyphs8:
	return ProcRenderCompositeGlyphs(client, 1);
    case X_RenderCompositeGlyphs16:
	return ProcRenderCompositeGlyphs(client, 2);
    case X_RenderCompositeGlyphs32:
	return ProcRenderCompositeGlyphs(client, 4);
    case X_RenderFillRectangles:
	return ProcRenderFillRectangles(client);
    default:
	return BadRequest;
    }
}

static int
SProcRenderQueryVersion (ClientPtr client)
{
    register int n;
    REQUEST(xRenderQueryVersionReq);

    swaps(&stuff->length, n);
    swapl(&stuff->majorVersion, n);
    swapl(&stuff->minorVersion, n);
    return ProcRenderQueryVersion(client);
}

static int
SProcRenderQueryPictFormats (ClientPtr client)
{
    register int n;
    REQUEST(xRenderQueryPictFormatsReq);
    swaps(&stuff->length, n);
    return ProcRenderQueryPictFormats (client);
}

static int
SProcRenderQueryPictIndexValues (ClientPtr client)
{
    return BadImplementation;
}

static int
SProcRenderQueryDithers (ClientPtr client)
{
    return BadImplementation;
}

static int
SProcRenderCreatePicture (ClientPtr client)
{
    register int n;
    REQUEST(xRenderCreatePictureReq);
    swaps(&stuff->length, n);
    swapl(&stuff->pid, n);
    swapl(&stuff->drawable, n);
    swapl(&stuff->format, n);
    swapl(&stuff->mask, n);
    SwapRestL(stuff);
    return ProcRenderCreatePicture (client);
}

static int
SProcRenderChangePicture (ClientPtr client)
{
    register int n;
    REQUEST(xRenderChangePictureReq);
    swaps(&stuff->length, n);
    swapl(&stuff->picture, n);
    swapl(&stuff->mask, n);
    SwapRestL(stuff);
    return ProcRenderChangePicture (client);
}

static int
SProcRenderSetPictureClipRectangles (ClientPtr client)
{
    register int n;
    REQUEST(xRenderSetPictureClipRectanglesReq);
    swaps(&stuff->length, n);
    swapl(&stuff->picture, n);
    SwapRestS(stuff);
    return ProcRenderSetPictureClipRectangles (client);
}

static int
SProcRenderFreePicture (ClientPtr client)
{
    register int n;
    REQUEST(xRenderFreePictureReq);
    swaps(&stuff->length, n);
    swapl(&stuff->picture, n);
    return ProcRenderFreePicture (client);
}

static int
SProcRenderComposite (ClientPtr client)
{
    register int n;
    REQUEST(xRenderCompositeReq);
    swaps(&stuff->length, n);
    swapl(&stuff->src, n);
    swapl(&stuff->mask, n);
    swapl(&stuff->dst, n);
    swaps(&stuff->xSrc, n);
    swaps(&stuff->ySrc, n);
    swaps(&stuff->xMask, n);
    swaps(&stuff->yMask, n);
    swaps(&stuff->xDst, n);
    swaps(&stuff->yDst, n);
    swaps(&stuff->width, n);
    swaps(&stuff->height, n);
    return ProcRenderComposite (client);
}

static int
SProcRenderScale (ClientPtr client)
{
    register int n;
    REQUEST(xRenderScaleReq);
    swaps(&stuff->length, n);
    swapl(&stuff->src, n);
    swapl(&stuff->dst, n);
    swapl(&stuff->colorScale, n);
    swapl(&stuff->alphaScale, n);
    swaps(&stuff->xSrc, n);
    swaps(&stuff->ySrc, n);
    swaps(&stuff->xDst, n);
    swaps(&stuff->yDst, n);
    swaps(&stuff->width, n);
    swaps(&stuff->height, n);
    return ProcRenderScale (client);
}

static int
SProcRenderTrapezoids (ClientPtr client)
{
    return BadImplementation;
}

static int
SProcRenderTriangles (ClientPtr client)
{
    return BadImplementation;
}

static int
SProcRenderTriStrip (ClientPtr client)
{
    return BadImplementation;
}

static int
SProcRenderTriFan (ClientPtr client)
{
    return BadImplementation;
}

static int
SProcRenderColorTrapezoids (ClientPtr client)
{
    return BadImplementation;
}

static int
SProcRenderColorTriangles (ClientPtr client)
{
    return BadImplementation;
}

static int
SProcRenderTransform (ClientPtr client)
{
    return BadImplementation;
}

static int
SProcRenderCreateGlyphSet (ClientPtr client)
{
    register int n;
    REQUEST(xRenderCreateGlyphSetReq);
    swaps(&stuff->length, n);
    swapl(&stuff->gsid, n);
    swapl(&stuff->format, n);
    return ProcRenderCreateGlyphSet (client);
}

static int
SProcRenderReferenceGlyphSet (ClientPtr client)
{
    register int n;
    REQUEST(xRenderReferenceGlyphSetReq);
    swaps(&stuff->length, n);
    swapl(&stuff->gsid, n);
    swapl(&stuff->existing, n);
    return ProcRenderReferenceGlyphSet  (client);
}

static int
SProcRenderFreeGlyphSet (ClientPtr client)
{
    register int n;
    REQUEST(xRenderFreeGlyphSetReq);
    swaps(&stuff->length, n);
    swapl(&stuff->glyphset, n);
    return ProcRenderFreeGlyphSet (client);
}

static int
SProcRenderAddGlyphs (ClientPtr client)
{
    register int n;
    register int i;
    CARD32  *gids;
    void    *end;
    xGlyphInfo *gi;
    REQUEST(xRenderAddGlyphsReq);
    swaps(&stuff->length, n);
    swapl(&stuff->glyphset, n);
    swapl(&stuff->nglyphs, n);
    if (stuff->nglyphs & 0xe0000000)
	return BadLength;
    end = (CARD8 *) stuff + (stuff->length << 2);
    gids = (CARD32 *) (stuff + 1);
    gi = (xGlyphInfo *) (gids + stuff->nglyphs);
    if ((char *) end - (char *) (gids + stuff->nglyphs) < 0)
	return BadLength;
    if ((char *) end - (char *) (gi + stuff->nglyphs) < 0)
	return BadLength;
    for (i = 0; i < stuff->nglyphs; i++)
    {
	swapl (&gids[i], n);
	swaps (&gi[i].width, n);
	swaps (&gi[i].height, n);
	swaps (&gi[i].x, n);
	swaps (&gi[i].y, n);
	swaps (&gi[i].xOff, n);
	swaps (&gi[i].yOff, n);
    }
    return ProcRenderAddGlyphs (client);
}

static int
SProcRenderAddGlyphsFromPicture (ClientPtr client)
{
    return BadImplementation;
}

static int
SProcRenderFreeGlyphs (ClientPtr client)
{
    register int n;
    REQUEST(xRenderFreeGlyphsReq);
    swaps(&stuff->length, n);
    swapl(&stuff->glyphset, n);
    SwapRestL(stuff);
    return ProcRenderFreeGlyphs (client);
}

static int
SProcRenderCompositeGlyphs (ClientPtr client, int size)
{
    register int n;
    xGlyphElt	*elt;
    CARD8	*buffer;
    CARD8	*end;
    int		space;
    
    REQUEST(xRenderCompositeGlyphsReq);
    swaps(&stuff->length, n);
    swapl(&stuff->src, n);
    swapl(&stuff->dst, n);
    swapl(&stuff->maskFormat, n);
    swapl(&stuff->glyphset, n);
    swaps(&stuff->xSrc, n);
    swaps(&stuff->ySrc, n);
    buffer = (CARD8 *) (stuff + 1);
    end = (CARD8 *) stuff + (stuff->length << 2);
    while (buffer + sizeof (xGlyphElt) < end)
    {
	elt = (xGlyphElt *) buffer;
	buffer += sizeof (xGlyphElt);
	
	swaps (&elt->deltax, n);
	swaps (&elt->deltay, n);
	
	if (elt->len == 0xff)
	{
	    buffer += 4;
	}
	else
	{
	    space = size * elt->len;
	    if (space & 3)
		space += 4 - (space & 3);
	    buffer += space;
	}
    }
    return ProcRenderCompositeGlyphs (client, size);
}

static int
SProcRenderFillRectangles (ClientPtr client)
{
    register int n;
    
    REQUEST(xRenderFillRectanglesReq);
    swaps(&stuff->length, n);
    swapl(&stuff->dst, n);
    swaps(&stuff->color.red, n);
    swaps(&stuff->color.green, n);
    swaps(&stuff->color.blue, n);
    swaps(&stuff->color.alpha, n);
    SwapRestS(stuff);
    return ProcRenderFillRectangles (client);
}
    
static int
SProcRenderDispatch (ClientPtr client)
{
    REQUEST(xReq);
    switch (stuff->data)
    {
    case X_RenderQueryVersion:
	return SProcRenderQueryVersion(client);
    case X_RenderQueryPictFormats:
	return SProcRenderQueryPictFormats(client);
    case X_RenderQueryPictIndexValues:
	return SProcRenderQueryPictIndexValues(client);
    case X_RenderQueryDithers:
	return SProcRenderQueryDithers(client);
    case X_RenderCreatePicture:
	return SProcRenderCreatePicture(client);
    case X_RenderChangePicture:
	return SProcRenderChangePicture(client);
    case X_RenderSetPictureClipRectangles:
	return SProcRenderSetPictureClipRectangles(client);
    case X_RenderFreePicture:
	return SProcRenderFreePicture(client);
    case X_RenderComposite:
	return SProcRenderComposite(client);
    case X_RenderScale:
	return SProcRenderScale(client);
    case X_RenderTrapezoids:
	return SProcRenderTrapezoids(client);
    case X_RenderTriangles:
	return SProcRenderTriangles(client);
    case X_RenderTriStrip:
	return SProcRenderTriStrip(client);
    case X_RenderTriFan:
	return SProcRenderTriFan(client);
    case X_RenderColorTrapezoids:
	return SProcRenderColorTrapezoids(client);
    case X_RenderColorTriangles:
	return SProcRenderColorTriangles(client);
    case X_RenderTransform:
	return SProcRenderTransform(client);
    case X_RenderCreateGlyphSet:
	return SProcRenderCreateGlyphSet(client);
    case X_RenderReferenceGlyphSet:
	return SProcRenderReferenceGlyphSet(client);
    case X_RenderFreeGlyphSet:
	return SProcRenderFreeGlyphSet(client);
    case X_RenderAddGlyphs:
	return SProcRenderAddGlyphs(client);
    case X_RenderAddGlyphsFromPicture:
	return SProcRenderAddGlyphsFromPicture(client);
    case X_RenderFreeGlyphs:
	return SProcRenderFreeGlyphs(client);
    case X_RenderCompositeGlyphs8:
	return SProcRenderCompositeGlyphs(client, 1);
    case X_RenderCompositeGlyphs16:
	return SProcRenderCompositeGlyphs(client, 2);
    case X_RenderCompositeGlyphs32:
	return SProcRenderCompositeGlyphs(client, 4);
    case X_RenderFillRectangles:
	return SProcRenderFillRectangles(client);
    default:
	return BadRequest;
    }
}
