/* $TOG: panoramiXprocs.c /main/9 1998/03/17 06:51:10 kaleb $ */
/****************************************************************
*                                                               *
*    Copyright (c) Digital Equipment Corporation, 1991, 1997    *
*                                                               *
*   All Rights Reserved.  Unpublished rights  reserved  under   *
*   the copyright laws of the United States.                    *
*                                                               *
*   The software contained on this media  is  proprietary  to   *
*   and  embodies  the  confidential  technology  of  Digital   *
*   Equipment Corporation.  Possession, use,  duplication  or   *
*   dissemination of the software and media is authorized only  *
*   pursuant to a valid written license from Digital Equipment  *
*   Corporation.                                                *
*                                                               *
*   RESTRICTED RIGHTS LEGEND   Use, duplication, or disclosure  *
*   by the U.S. Government is subject to restrictions  as  set  *
*   forth in Subparagraph (c)(1)(ii)  of  DFARS  252.227-7013,  *
*   or  in  FAR 52.227-19, as applicable.                       *
*                                                               *
*****************************************************************/

/* Massively rewritten by Mark Vojkovich <markv@valinux.com> */

/* $XFree86: xc/programs/Xserver/Xext/panoramiXprocs.c,v 3.27 2000/04/08 19:18:54 mvojkovi Exp $ */

#include <stdio.h>
#include "X.h"
#define NEED_REPLIES
#define NEED_EVENTS
#include "Xproto.h"
#include "windowstr.h"
#include "dixfontstr.h"
#include "gcstruct.h"
#include "colormapst.h"
#include "scrnintstr.h"
#include "opaque.h"
#include "inputstr.h"
#include "migc.h"
#include "misc.h"
#include "dixstruct.h"
#include "panoramiX.h"
#include "panoramiXsrv.h"
#include "resource.h"

#define XINERAMA_IMAGE_BUFSIZE (256*1024)
#define INPUTONLY_LEGAL_MASK (CWWinGravity | CWEventMask | \
                              CWDontPropagate | CWOverrideRedirect | CWCursor )

extern ScreenInfo *GlobalScrInfo;
extern WindowPtr *WindowTable;
extern char *ConnectionInfo;
extern int connBlockScreenStart;

extern WindowPtr *WindowTable;
extern void Swap32Write();

extern int (* InitialVector[3]) ();
extern int (* ProcVector[256]) ();
extern int (* SwappedProcVector[256]) ();
extern int (* SavedProcVector[256]) ();
extern void (* EventSwapVector[128]) ();
extern void (* ReplySwapVector[256]) ();
extern void Swap32Write(), SLHostsExtend(), SQColorsExtend(), 
WriteSConnectionInfo();
extern void WriteSConnSetupPrefix();
extern char *ClientAuthorized();
extern Bool InsertFakeRequest();
extern void ProcessWorkQueue();

extern char isItTimeToYield;

/* Various of the DIX function interfaces were not designed to allow
 * the client->errorValue to be set on BadValue and other errors.
 * Rather than changing interfaces and breaking untold code we introduce
 * a new global that dispatch can use.
 */
extern XID clientErrorValue;   /* XXX this is a kludge */



extern int Ones();


int PanoramiXCreateWindow(ClientPtr client)
{
    PanoramiXRes *parent, *newWin;
    PanoramiXRes *backPix = NULL;
    PanoramiXRes *bordPix = NULL;
    PanoramiXRes *cmap    = NULL;
    REQUEST(xCreateWindowReq);
    int pback_offset, pbord_offset, cmap_offset;
    int result, len, j;
    int orig_x, orig_y;
    XID orig_visual, tmp;
    Bool parentIsRoot;

    REQUEST_AT_LEAST_SIZE(xCreateWindowReq);
    
    len = client->req_len - (sizeof(xCreateWindowReq) >> 2);
    if (Ones(stuff->mask) != len)
        return BadLength;

    if (!(parent = (PanoramiXRes *)SecurityLookupIDByType(
		client, stuff->parent, XRT_WINDOW, SecurityWriteAccess)))
        return BadWindow;

    if(stuff->class == CopyFromParent)
	stuff->class = parent->u.win.class;

    if((stuff->class == InputOnly) && (stuff->mask & (~INPUTONLY_LEGAL_MASK)))
        return BadMatch;

    if ((Mask)stuff->mask & CWBackPixmap) {
	pback_offset = Ones((Mask)stuff->mask & (CWBackPixmap - 1));
	tmp = *((CARD32 *) &stuff[1] + pback_offset);
	if ((tmp != None) && (tmp != ParentRelative)) {
	   if(!(backPix = (PanoramiXRes*) SecurityLookupIDByType(
		client, tmp, XRT_PIXMAP, SecurityReadAccess)))
	      return BadPixmap;
	}
    }
    if ((Mask)stuff->mask & CWBorderPixmap) {
	pbord_offset = Ones((Mask)stuff->mask & (CWBorderPixmap - 1));
	tmp = *((CARD32 *) &stuff[1] + pbord_offset);
	if (tmp != CopyFromParent) {
	   if(!(bordPix = (PanoramiXRes*) SecurityLookupIDByType(
		client, tmp, XRT_PIXMAP, SecurityReadAccess)))
	      return BadPixmap;
	}
    }
    if ((Mask)stuff->mask & CWColormap) {
	cmap_offset = Ones((Mask)stuff->mask & (CWColormap - 1));
	tmp = *((CARD32 *) &stuff[1] + cmap_offset);
	if ((tmp != CopyFromParent) && (tmp != None)) {
	   if(!(cmap = (PanoramiXRes*) SecurityLookupIDByType(
		client, tmp, XRT_COLORMAP, SecurityReadAccess)))
	      return BadColor;
	}
    }

    if(!(newWin = (PanoramiXRes *) xalloc(sizeof(PanoramiXRes))))
        return BadAlloc;

    newWin->type = XRT_WINDOW;
    newWin->u.win.visibility = VisibilityNotViewable;
    newWin->u.win.class = stuff->class;
    newWin->info[0].id = stuff->wid;
    for(j = 1; j < PanoramiXNumScreens; j++)
        newWin->info[j].id = FakeClientID(client->index);

    if (stuff->class == InputOnly)
	stuff->visual = CopyFromParent;
    orig_visual = stuff->visual;
    orig_x = stuff->x;
    orig_y = stuff->y;
    parentIsRoot = (stuff->parent == WindowTable[0]->drawable.id);
    FOR_NSCREENS_BACKWARD(j) {
        stuff->wid = newWin->info[j].id;
        stuff->parent = parent->info[j].id;
	if (parentIsRoot) {
	    stuff->x = orig_x - panoramiXdataPtr[j].x;
	    stuff->y = orig_y - panoramiXdataPtr[j].y;
	}
	if (backPix)
	    *((CARD32 *) &stuff[1] + pback_offset) = backPix->info[j].id;
	if (bordPix)
	    *((CARD32 *) &stuff[1] + pbord_offset) = bordPix->info[j].id;
	if (cmap)
	    *((CARD32 *) &stuff[1] + cmap_offset) = cmap->info[j].id;
	if ( orig_visual != CopyFromParent ) 
	    stuff->visual = PanoramiXVisualTable[orig_visual][j];
        result = (*SavedProcVector[X_CreateWindow])(client);
        if(result != Success) break;
    }

    if (result == Success)
        AddResource(newWin->info[0].id, XRT_WINDOW, newWin);
    else 
        xfree(newWin);

    return (result);
}


int PanoramiXChangeWindowAttributes(ClientPtr client)
{
    PanoramiXRes *win;
    PanoramiXRes *backPix = NULL;
    PanoramiXRes *bordPix = NULL;
    PanoramiXRes *cmap    = NULL;
    REQUEST(xChangeWindowAttributesReq);
    int pback_offset, pbord_offset, cmap_offset;
    int result, len, j;
    XID tmp;

    REQUEST_AT_LEAST_SIZE(xChangeWindowAttributesReq);
    
    len = client->req_len - (sizeof(xChangeWindowAttributesReq) >> 2);
    if (Ones(stuff->valueMask) != len)
        return BadLength;

    if (!(win = (PanoramiXRes *)SecurityLookupIDByType(
		client, stuff->window, XRT_WINDOW, SecurityWriteAccess)))
        return BadWindow;

    if((win->u.win.class == InputOnly) && 
       (stuff->valueMask & (~INPUTONLY_LEGAL_MASK)))
        return BadMatch;

    if ((Mask)stuff->valueMask & CWBackPixmap) {
	pback_offset = Ones((Mask)stuff->valueMask & (CWBackPixmap - 1));
	tmp = *((CARD32 *) &stuff[1] + pback_offset);
	if ((tmp != None) && (tmp != ParentRelative)) {
	   if(!(backPix = (PanoramiXRes*) SecurityLookupIDByType(
		client, tmp, XRT_PIXMAP, SecurityReadAccess)))
	      return BadPixmap;
	}
    }
    if ((Mask)stuff->valueMask & CWBorderPixmap) {
	pbord_offset = Ones((Mask)stuff->valueMask & (CWBorderPixmap - 1));
	tmp = *((CARD32 *) &stuff[1] + pbord_offset);
	if (tmp != CopyFromParent) {
	   if(!(bordPix = (PanoramiXRes*) SecurityLookupIDByType(
		client, tmp, XRT_PIXMAP, SecurityReadAccess)))
	      return BadPixmap;
	}
    }
    if ((Mask)stuff->valueMask & CWColormap) {
	cmap_offset = Ones((Mask)stuff->valueMask & (CWColormap - 1));
	tmp = *((CARD32 *) &stuff[1] + cmap_offset);
	if ((tmp != CopyFromParent) && (tmp != None)) {
	   if(!(cmap = (PanoramiXRes*) SecurityLookupIDByType(
		client, tmp, XRT_COLORMAP, SecurityReadAccess)))
	      return BadColor;
	}
    }

    FOR_NSCREENS_BACKWARD(j) {
        stuff->window = win->info[j].id;
	if (backPix)
	    *((CARD32 *) &stuff[1] + pback_offset) = backPix->info[j].id;
	if (bordPix)
	    *((CARD32 *) &stuff[1] + pbord_offset) = bordPix->info[j].id;
	if (cmap)
	    *((CARD32 *) &stuff[1] + cmap_offset) = cmap->info[j].id;
        result = (*SavedProcVector[X_ChangeWindowAttributes])(client);
        if(result != Success) break;
    }

    return (result);
}


int PanoramiXDestroyWindow(ClientPtr client)
{
    PanoramiXRes *win;
    int         result, j;
    REQUEST(xResourceReq);

    REQUEST_SIZE_MATCH(xResourceReq);

    if(!(win = (PanoramiXRes *)SecurityLookupIDByType(
		client, stuff->id, XRT_WINDOW, SecurityDestroyAccess)))
	return BadWindow;

    FOR_NSCREENS_BACKWARD(j) {
	stuff->id = win->info[j].id;
	result = (*SavedProcVector[X_DestroyWindow])(client);
        if(result != Success) break;
    }

    /* Since ProcDestroyWindow is using FreeResource, it will free
	our resource for us on the last pass through the loop above */
 
    return (result);
}


int PanoramiXDestroySubwindows(ClientPtr client)
{
    PanoramiXRes *win;
    int         result, j;
    REQUEST(xResourceReq);

    REQUEST_SIZE_MATCH(xResourceReq);

    if(!(win = (PanoramiXRes *)SecurityLookupIDByType(
		client, stuff->id, XRT_WINDOW, SecurityDestroyAccess)))
	return BadWindow;

    FOR_NSCREENS_BACKWARD(j) {
	stuff->id = win->info[j].id;
	result = (*SavedProcVector[X_DestroySubwindows])(client);
        if(result != Success) break;
    }

    /* DestroySubwindows is using FreeResource which will free
	our resources for us on the last pass through the loop above */

    return (result);
}


int PanoramiXChangeSaveSet(ClientPtr client)
{
    PanoramiXRes *win;
    int         result, j;
    REQUEST(xChangeSaveSetReq);

    REQUEST_SIZE_MATCH(xChangeSaveSetReq);

    if(!(win = (PanoramiXRes *)SecurityLookupIDByType(
		client, stuff->window, XRT_WINDOW, SecurityReadAccess)))
	return BadWindow;

    FOR_NSCREENS_BACKWARD(j) {
	stuff->window = win->info[j].id;
	result = (*SavedProcVector[X_ChangeSaveSet])(client);
        if(result != Success) break;
    }

    return (result);
}


int PanoramiXReparentWindow(ClientPtr client)
{
    PanoramiXRes *win, *parent;
    int         result, j;
    int		x, y;
    Bool	parentIsRoot;
    REQUEST(xReparentWindowReq);

    REQUEST_SIZE_MATCH(xReparentWindowReq);

    if(!(win = (PanoramiXRes *)SecurityLookupIDByType(
		client, stuff->window, XRT_WINDOW, SecurityWriteAccess)))
	return BadWindow;

    if(!(parent = (PanoramiXRes *)SecurityLookupIDByType(
		client, stuff->parent, XRT_WINDOW, SecurityWriteAccess)))
	return BadWindow;

    x = stuff->x;
    y = stuff->y;
    parentIsRoot = (stuff->parent == WindowTable[0]->drawable.id);
    FOR_NSCREENS_BACKWARD(j) {
	stuff->window = win->info[j].id;
	stuff->parent = parent->info[j].id;
	if(parentIsRoot) {
	    stuff->x = x - panoramiXdataPtr[j].x;
	    stuff->y = y - panoramiXdataPtr[j].y;
	}
	result = (*SavedProcVector[X_ReparentWindow])(client);
        if(result != Success) break;
    }

    return (result);
}


int PanoramiXMapWindow(ClientPtr client)
{
    PanoramiXRes *win;
    int         result, j;
    REQUEST(xResourceReq);

    REQUEST_SIZE_MATCH(xResourceReq);

    if(!(win = (PanoramiXRes *)SecurityLookupIDByType(
		client, stuff->id, XRT_WINDOW, SecurityReadAccess)))
	return BadWindow;

    FOR_NSCREENS_FORWARD(j) {
	stuff->id = win->info[j].id;
	result = (*SavedProcVector[X_MapWindow])(client);
        if(result != Success) break;
    }

    return (result);
}


int PanoramiXMapSubwindows(ClientPtr client)
{
    PanoramiXRes *win;
    int         result, j;
    REQUEST(xResourceReq);

    REQUEST_SIZE_MATCH(xResourceReq);

    if(!(win = (PanoramiXRes *)SecurityLookupIDByType(
		client, stuff->id, XRT_WINDOW, SecurityReadAccess)))
	return BadWindow;

    FOR_NSCREENS_FORWARD(j) {
	stuff->id = win->info[j].id;
	result = (*SavedProcVector[X_MapSubwindows])(client);
        if(result != Success) break;
    }

    return (result);
}


int PanoramiXUnmapWindow(ClientPtr client)
{
    PanoramiXRes *win;
    int         result, j;
    REQUEST(xResourceReq);

    REQUEST_SIZE_MATCH(xResourceReq);

    if(!(win = (PanoramiXRes *)SecurityLookupIDByType(
		client, stuff->id, XRT_WINDOW, SecurityReadAccess)))
	return BadWindow;

    FOR_NSCREENS_FORWARD(j) {
	stuff->id = win->info[j].id;
	result = (*SavedProcVector[X_UnmapWindow])(client);
        if(result != Success) break;
    }

    return (result);
}


int PanoramiXUnmapSubwindows(ClientPtr client)
{
    PanoramiXRes *win;
    int         result, j;
    REQUEST(xResourceReq);

    REQUEST_SIZE_MATCH(xResourceReq);

    if(!(win = (PanoramiXRes *)SecurityLookupIDByType(
		client, stuff->id, XRT_WINDOW, SecurityReadAccess)))
	return BadWindow;

    FOR_NSCREENS_FORWARD(j) {
	stuff->id = win->info[j].id;
	result = (*SavedProcVector[X_UnmapSubwindows])(client);
        if(result != Success) break;
    }

    return (result);
}


int PanoramiXConfigureWindow(ClientPtr client)
{
    PanoramiXRes *win;
    PanoramiXRes *sib = NULL;
    WindowPtr   pWin;
    int         result, j, len, sib_offset, x, y;
    int		x_offset = -1;
    int		y_offset = -1;
    REQUEST(xConfigureWindowReq);

    REQUEST_AT_LEAST_SIZE(xConfigureWindowReq);

    len = client->req_len - (sizeof(xConfigureWindowReq) >> 2);
    if (Ones(stuff->mask) != len)
        return BadLength;

    /* because we need the parent */
    if (!(pWin = (WindowPtr)SecurityLookupIDByType(
		client, stuff->window, RT_WINDOW, SecurityWriteAccess)))
        return BadWindow;

    if (!(win = (PanoramiXRes *)SecurityLookupIDByType(
		client, stuff->window, XRT_WINDOW, SecurityWriteAccess)))
        return BadWindow;

    if ((Mask)stuff->mask & CWSibling) {
	XID tmp;
	sib_offset = Ones((Mask)stuff->mask & (CWSibling - 1));
	if ((tmp = *((CARD32 *) &stuff[1] + sib_offset))) {
	   if(!(sib = (PanoramiXRes*) SecurityLookupIDByType(
		client, tmp, XRT_WINDOW, SecurityReadAccess)))
	      return BadWindow;
	}
    }

    if(pWin->parent && (pWin->parent == WindowTable[0])) {
	if ((Mask)stuff->mask & CWX) {
	    x_offset = 0;
	    x = *((CARD32 *)&stuff[1]);
	}
	if ((Mask)stuff->mask & CWY) {
	    y_offset = (x_offset == -1) ? 0 : 1;
	    y = *((CARD32 *) &stuff[1] + y_offset);
	}
    }

    /* have to go forward or you get expose events before 
	ConfigureNotify events */
    FOR_NSCREENS_FORWARD(j) {
	stuff->window = win->info[j].id;
	if(sib)
	    *((CARD32 *) &stuff[1] + sib_offset) = sib->info[j].id;
	if(x_offset >= 0)
	    *((CARD32 *) &stuff[1] + x_offset) = x - panoramiXdataPtr[j].x;
	if(y_offset >= 0)
	    *((CARD32 *) &stuff[1] + y_offset) = y - panoramiXdataPtr[j].y;
	result = (*SavedProcVector[X_ConfigureWindow])(client);
        if(result != Success) break;
    }

    return (result);
}


int PanoramiXCirculateWindow(ClientPtr client)
{
    PanoramiXRes *win;
    int         result, j;
    REQUEST(xCirculateWindowReq);

    REQUEST_SIZE_MATCH(xCirculateWindowReq);

    if(!(win = (PanoramiXRes *)SecurityLookupIDByType(
		client, stuff->window, XRT_WINDOW, SecurityWriteAccess)))
	return BadWindow;

    FOR_NSCREENS_FORWARD(j) {
	stuff->window = win->info[j].id;
	result = (*SavedProcVector[X_CirculateWindow])(client);
        if(result != Success) break;
    }

    return (result);
}


int PanoramiXGetGeometry(ClientPtr client)
{
    xGetGeometryReply 	 rep;
    DrawablePtr pDraw;
    REQUEST(xResourceReq);

    REQUEST_SIZE_MATCH(xResourceReq);
    VERIFY_GEOMETRABLE (pDraw, stuff->id, client);
    rep.type = X_Reply;
    rep.length = 0;
    rep.sequenceNumber = client->sequence;
    rep.root = WindowTable[0]->drawable.id;
    rep.depth = pDraw->depth;
    rep.width = pDraw->width;
    rep.height = pDraw->height;
    rep.x = rep.y = rep.borderWidth = 0;

    if (stuff->id == rep.root) {
	xWindowRoot *root  = (xWindowRoot *)
				    (ConnectionInfo + connBlockScreenStart);

	rep.width = root->pixWidth;
	rep.height = root->pixHeight;
    } else 
    if ((pDraw->type == UNDRAWABLE_WINDOW) || (pDraw->type == DRAWABLE_WINDOW))
    {
        WindowPtr pWin = (WindowPtr)pDraw;
	rep.x = pWin->origin.x - wBorderWidth (pWin);
	rep.y = pWin->origin.y - wBorderWidth (pWin);
	if(pWin->parent == WindowTable[0]) {
	   rep.x += panoramiXdataPtr[0].x;
	   rep.y += panoramiXdataPtr[0].y;
	}
	rep.borderWidth = pWin->borderWidth;
    }

    WriteReplyToClient(client, sizeof(xGetGeometryReply), &rep);
    return (client->noClientException);
}

int PanoramiXTranslateCoords(ClientPtr client)
{
    INT16 x, y;
    REQUEST(xTranslateCoordsReq);

    register WindowPtr pWin, pDst;
    xTranslateCoordsReply rep;

    REQUEST_SIZE_MATCH(xTranslateCoordsReq);
    pWin = (WindowPtr)SecurityLookupWindow(stuff->srcWid, client,
					   SecurityReadAccess);
    if (!pWin)
        return(BadWindow);
    pDst = (WindowPtr)SecurityLookupWindow(stuff->dstWid, client,
					   SecurityReadAccess);
    if (!pDst)
        return(BadWindow);
    rep.type = X_Reply;
    rep.length = 0;
    rep.sequenceNumber = client->sequence;
    rep.sameScreen = xTrue;
    rep.child = None;

    if(pWin == WindowTable[0]) {
	x = stuff->srcX - panoramiXdataPtr[0].x;
	y = stuff->srcY - panoramiXdataPtr[0].y;
    } else {
	x = pWin->drawable.x + stuff->srcX;
	y = pWin->drawable.y + stuff->srcY;
    }
    pWin = pDst->firstChild;
    while (pWin) {
#ifdef SHAPE
	    BoxRec  box;
#endif
	    if ((pWin->mapped) &&
		(x >= pWin->drawable.x - wBorderWidth (pWin)) &&
		(x < pWin->drawable.x + (int)pWin->drawable.width +
		 wBorderWidth (pWin)) &&
		(y >= pWin->drawable.y - wBorderWidth (pWin)) &&
		(y < pWin->drawable.y + (int)pWin->drawable.height +
		 wBorderWidth (pWin))
#ifdef SHAPE
		/* When a window is shaped, a further check
		 * is made to see if the point is inside
		 * borderSize
		 */
		&& (!wBoundingShape(pWin) ||
		    POINT_IN_REGION(pWin->drawable.pScreen, 
					&pWin->borderSize, x, y, &box))
#endif
		)
            {
		rep.child = pWin->drawable.id;
		pWin = (WindowPtr) NULL;
	    }
	    else
		pWin = pWin->nextSib;
    }
    rep.dstX = x - pDst->drawable.x;
    rep.dstY = y - pDst->drawable.y;
    if(pDst == WindowTable[0]) {
	rep.dstX += panoramiXdataPtr[0].x;
	rep.dstY += panoramiXdataPtr[0].y;
    }

    WriteReplyToClient(client, sizeof(xTranslateCoordsReply), &rep);
    return(client->noClientException);
}

int PanoramiXCreatePixmap(ClientPtr client)
{
    PanoramiXRes *refDraw, *newPix;
    int result, j;
    REQUEST(xCreatePixmapReq);

    REQUEST_SIZE_MATCH(xCreatePixmapReq);
    client->errorValue = stuff->pid;

    if(!(refDraw = (PanoramiXRes *)SecurityLookupIDByClass(
		client, stuff->drawable, XRC_DRAWABLE, SecurityReadAccess)))
	return BadDrawable;

    if(!(newPix = (PanoramiXRes *) xalloc(sizeof(PanoramiXRes))))
	return BadAlloc;

    newPix->type = XRT_PIXMAP;
    newPix->u.pix.shared = FALSE;
    newPix->info[0].id = stuff->pid;
    for(j = 1; j < PanoramiXNumScreens; j++)
	newPix->info[j].id = FakeClientID(client->index);
   
    FOR_NSCREENS_BACKWARD(j) {
	stuff->pid = newPix->info[j].id;
	stuff->drawable = refDraw->info[j].id;
	result = (*SavedProcVector[X_CreatePixmap])(client);
	if(result != Success) break;
    }

    if (result == Success)
	AddResource(newPix->info[0].id, XRT_PIXMAP, newPix);
    else 
	xfree(newPix);

    return (result);
}


int PanoramiXFreePixmap(ClientPtr client)
{
    PanoramiXRes *pix;
    int         result, j;
    REQUEST(xResourceReq);

    REQUEST_SIZE_MATCH(xResourceReq);

    client->errorValue = stuff->id;

    if(!(pix = (PanoramiXRes *)SecurityLookupIDByType(
		client, stuff->id, XRT_PIXMAP, SecurityDestroyAccess)))
	return BadPixmap;

    FOR_NSCREENS_BACKWARD(j) {
	stuff->id = pix->info[j].id;
	result = (*SavedProcVector[X_FreePixmap])(client);
	if(result != Success) break;
    }

    /* Since ProcFreePixmap is using FreeResource, it will free
	our resource for us on the last pass through the loop above */
 
    return (result);
}


int PanoramiXCreateGC(ClientPtr client)
{
    PanoramiXRes *refDraw;
    PanoramiXRes *newGC;
    PanoramiXRes *stip = NULL;
    PanoramiXRes *tile = NULL;
    PanoramiXRes *clip = NULL;
    REQUEST(xCreateGCReq);
    int tile_offset, stip_offset, clip_offset;
    int result, len, j;
    XID tmp;

    REQUEST_AT_LEAST_SIZE(xCreateGCReq);
    
    client->errorValue = stuff->gc;
    len = client->req_len - (sizeof(xCreateGCReq) >> 2);
    if (Ones(stuff->mask) != len)
        return BadLength;

    if (!(refDraw = (PanoramiXRes *)SecurityLookupIDByClass(
		client, stuff->drawable, XRC_DRAWABLE, SecurityReadAccess)))
        return BadDrawable;

    if ((Mask)stuff->mask & GCTile) {
	tile_offset = Ones((Mask)stuff->mask & (GCTile - 1));
	if ((tmp = *((CARD32 *) &stuff[1] + tile_offset))) {
	   if(!(tile = (PanoramiXRes*) SecurityLookupIDByType(
		client, tmp, XRT_PIXMAP, SecurityReadAccess)))
	      return BadPixmap;
	}
    }
    if ((Mask)stuff->mask & GCStipple) {
	stip_offset = Ones((Mask)stuff->mask & (GCStipple - 1));
	if ((tmp = *((CARD32 *) &stuff[1] + stip_offset))) {
	   if(!(stip = (PanoramiXRes*) SecurityLookupIDByType(
		client, tmp, XRT_PIXMAP, SecurityReadAccess)))
	      return BadPixmap;
	}
    }
    if ((Mask)stuff->mask & GCClipMask) {
	clip_offset = Ones((Mask)stuff->mask & (GCClipMask - 1));
	if ((tmp = *((CARD32 *) &stuff[1] + clip_offset))) {
	   if(!(clip = (PanoramiXRes*) SecurityLookupIDByType(
		client, tmp, XRT_PIXMAP, SecurityReadAccess)))
	      return BadPixmap;
	}
    }

    if(!(newGC = (PanoramiXRes *) xalloc(sizeof(PanoramiXRes))))
        return BadAlloc;

    newGC->type = XRT_GC;
    newGC->info[0].id = stuff->gc;
    for(j = 1; j < PanoramiXNumScreens; j++)
        newGC->info[j].id = FakeClientID(client->index);

    FOR_NSCREENS_BACKWARD(j) {
        stuff->gc = newGC->info[j].id;
        stuff->drawable = refDraw->info[j].id;
	if (tile)
	    *((CARD32 *) &stuff[1] + tile_offset) = tile->info[j].id;
	if (stip)
	    *((CARD32 *) &stuff[1] + stip_offset) = stip->info[j].id;
	if (clip)
	    *((CARD32 *) &stuff[1] + clip_offset) = clip->info[j].id;
        result = (*SavedProcVector[X_CreateGC])(client);
        if(result != Success) break;
    }

    if (result == Success)
        AddResource(newGC->info[0].id, XRT_GC, newGC);
    else 
        xfree(newGC);

    return (result);
}

int PanoramiXChangeGC(ClientPtr client)
{
    PanoramiXRes *gc;
    PanoramiXRes *stip = NULL;
    PanoramiXRes *tile = NULL;
    PanoramiXRes *clip = NULL;
    REQUEST(xChangeGCReq);
    int tile_offset, stip_offset, clip_offset;
    int result, len, j;
    XID tmp;

    REQUEST_AT_LEAST_SIZE(xChangeGCReq);
    
    len = client->req_len - (sizeof(xChangeGCReq) >> 2);
    if (Ones(stuff->mask) != len)
        return BadLength;

    if (!(gc = (PanoramiXRes *)SecurityLookupIDByType(
		client, stuff->gc, XRT_GC, SecurityReadAccess)))
        return BadGC;

    if ((Mask)stuff->mask & GCTile) {
	tile_offset = Ones((Mask)stuff->mask & (GCTile - 1));
	if ((tmp = *((CARD32 *) &stuff[1] + tile_offset))) {
	   if(!(tile = (PanoramiXRes*) SecurityLookupIDByType(
		client, tmp, XRT_PIXMAP, SecurityReadAccess)))
	      return BadPixmap;
	}
    }
    if ((Mask)stuff->mask & GCStipple) {
	stip_offset = Ones((Mask)stuff->mask & (GCStipple - 1));
	if ((tmp = *((CARD32 *) &stuff[1] + stip_offset))) {
	   if(!(stip = (PanoramiXRes*) SecurityLookupIDByType(
		client, tmp, XRT_PIXMAP, SecurityReadAccess)))
	      return BadPixmap;
	}
    }
    if ((Mask)stuff->mask & GCClipMask) {
	clip_offset = Ones((Mask)stuff->mask & (GCClipMask - 1));
	if ((tmp = *((CARD32 *) &stuff[1] + clip_offset))) {
	   if(!(clip = (PanoramiXRes*) SecurityLookupIDByType(
		client, tmp, XRT_PIXMAP, SecurityReadAccess)))
	      return BadPixmap;
	}
    }


    FOR_NSCREENS_BACKWARD(j) {
        stuff->gc = gc->info[j].id;
	if (tile)
	    *((CARD32 *) &stuff[1] + tile_offset) = tile->info[j].id;
	if (stip)
	    *((CARD32 *) &stuff[1] + stip_offset) = stip->info[j].id;
	if (clip)
	    *((CARD32 *) &stuff[1] + clip_offset) = clip->info[j].id;
        result = (*SavedProcVector[X_ChangeGC])(client);
        if(result != Success) break;
    }

    return (result);
}


int PanoramiXCopyGC(ClientPtr client)
{
    PanoramiXRes *srcGC, *dstGC;
    int         result, j;
    REQUEST(xCopyGCReq);

    REQUEST_SIZE_MATCH(xCopyGCReq);

    if(!(srcGC = (PanoramiXRes *)SecurityLookupIDByType(
		client, stuff->srcGC, XRT_GC, SecurityReadAccess)))
	return BadGC;

    if(!(dstGC = (PanoramiXRes *)SecurityLookupIDByType(
		client, stuff->dstGC, XRT_GC, SecurityWriteAccess)))
	return BadGC;

    FOR_NSCREENS(j) {
	stuff->srcGC = srcGC->info[j].id;
	stuff->dstGC = dstGC->info[j].id;
	result = (*SavedProcVector[X_CopyGC])(client);
        if(result != Success) break;
    }

    return (result);
}


int PanoramiXSetDashes(ClientPtr client)
{
    PanoramiXRes *gc;
    int         result, j;
    REQUEST(xSetDashesReq);

    REQUEST_FIXED_SIZE(xSetDashesReq, stuff->nDashes);

    if(!(gc = (PanoramiXRes *)SecurityLookupIDByType(
		client, stuff->gc, XRT_GC, SecurityWriteAccess)))
	return BadGC;

    FOR_NSCREENS_BACKWARD(j) {
	stuff->gc = gc->info[j].id;
	result = (*SavedProcVector[X_SetDashes])(client);
        if(result != Success) break;
    }

    return (result);
}


int PanoramiXSetClipRectangles(ClientPtr client)
{
    PanoramiXRes *gc;
    int         result, j;
    REQUEST(xSetClipRectanglesReq);

    REQUEST_AT_LEAST_SIZE(xSetClipRectanglesReq);

    if(!(gc = (PanoramiXRes *)SecurityLookupIDByType(
		client, stuff->gc, XRT_GC, SecurityWriteAccess)))
	return BadGC;

    FOR_NSCREENS_BACKWARD(j) {
	stuff->gc = gc->info[j].id;
	result = (*SavedProcVector[X_SetClipRectangles])(client);
        if(result != Success) break;
    }

    return (result);
}


int PanoramiXFreeGC(ClientPtr client)
{
    PanoramiXRes *gc;
    int         result, j;
    REQUEST(xResourceReq);

    REQUEST_SIZE_MATCH(xResourceReq);

    if(!(gc = (PanoramiXRes *)SecurityLookupIDByType(
		client, stuff->id, XRT_GC, SecurityDestroyAccess)))
	return BadGC;

    FOR_NSCREENS_BACKWARD(j) {
	stuff->id = gc->info[j].id;
	result = (*SavedProcVector[X_FreeGC])(client);
	if(result != Success) break;
    }

    /* Since ProcFreeGC is using FreeResource, it will free
	our resource for us on the last pass through the loop above */
 
    return (result);
}


int PanoramiXClearToBackground(ClientPtr client)
{
    PanoramiXRes *win;
    int         result, j, x, y;
    Bool	isRoot;
    REQUEST(xClearAreaReq);

    REQUEST_SIZE_MATCH(xClearAreaReq);

    if(!(win = (PanoramiXRes *)SecurityLookupIDByType(
		client, stuff->window, XRT_WINDOW, SecurityWriteAccess)))
	return BadWindow;

    x = stuff->x;
    y = stuff->y;
    isRoot = (stuff->window == WindowTable[0]->drawable.id);
    FOR_NSCREENS_BACKWARD(j) {
	stuff->window = win->info[j].id;
	if(isRoot) {
	    stuff->x = x - panoramiXdataPtr[j].x;
	    stuff->y = y - panoramiXdataPtr[j].y;
	}
	result = (*SavedProcVector[X_ClearArea])(client);
	if(result != Success) break;
    }
 
    return (result);
}


/* 
    For Window to Pixmap copies you're screwed since each screen's
    pixmap will look like what it sees on its screen.  Unless the
    screens overlap and the window lies on each, the two copies
    will be out of sync.  To remedy this we do a GetImage and PutImage
    in place of the copy.  Doing this as a single Image isn't quite
    correct since it will include the obscured areas but we will
    have to fix this later. (MArk).
*/

int PanoramiXCopyArea(ClientPtr client)
{
    int			j, result, srcx, srcy, dstx, dsty;
    PanoramiXRes	*gc, *src, *dst;
    Bool		srcIsRoot = FALSE;
    Bool		dstIsRoot = FALSE;
    Bool		srcShared, dstShared;
    REQUEST(xCopyAreaReq);

    REQUEST_SIZE_MATCH(xCopyAreaReq);

    if(!(src = (PanoramiXRes *)SecurityLookupIDByClass(
		client, stuff->srcDrawable, XRC_DRAWABLE, SecurityReadAccess)))
	return BadDrawable;

    srcShared = IS_SHARED_PIXMAP(src);

    if(!(dst = (PanoramiXRes *)SecurityLookupIDByClass(
		client, stuff->dstDrawable, XRC_DRAWABLE, SecurityWriteAccess)))
	return BadDrawable;

    dstShared = IS_SHARED_PIXMAP(dst);

    if(dstShared && srcShared)
	return (* SavedProcVector[X_CopyArea])(client);

    if(!(gc = (PanoramiXRes *)SecurityLookupIDByType(
		client, stuff->gc, XRT_GC, SecurityReadAccess)))
	return BadGC;

    if((dst->type == XRT_WINDOW) &&
       (stuff->dstDrawable == WindowTable[0]->drawable.id))
	dstIsRoot = TRUE;
    if((src->type == XRT_WINDOW) &&
       (stuff->srcDrawable == WindowTable[0]->drawable.id))
	srcIsRoot = TRUE;

    srcx = stuff->srcX; srcy = stuff->srcY;
    dstx = stuff->dstX; dsty = stuff->dstY;
    if((dst->type == XRT_PIXMAP) && (src->type == XRT_WINDOW)) {
	DrawablePtr drawables[MAXSCREENS];
	DrawablePtr pDst;
	GCPtr pGC;
        char *data;
	int pitch;

	FOR_NSCREENS(j)
	    VERIFY_DRAWABLE(drawables[j], src->info[j].id, client);

	pitch = PixmapBytePad(stuff->width, drawables[0]->depth); 
	if(!(data = xcalloc(1, stuff->height * pitch)))
	    return BadAlloc;

	XineramaGetImageData(drawables, srcx, srcy, 
		stuff->width, stuff->height, ZPixmap, ~0, data, pitch, 
		srcIsRoot);

	FOR_NSCREENS_BACKWARD(j) {
	    stuff->gc = gc->info[j].id;
	    VALIDATE_DRAWABLE_AND_GC(dst->info[j].id, pDst, pGC, client);

	    if(drawables[0]->depth != pDst->depth) {
		client->errorValue = stuff->dstDrawable;
		xfree(data);
		return (BadMatch);
	    }

	    (*pGC->ops->PutImage) (pDst, pGC, pDst->depth, dstx, dsty, 
				   stuff->width, stuff->height, 
				   0, ZPixmap, data);

	    if(dstShared) break;
	}

	xfree(data);

	result = Success;
    } else {
	DrawablePtr pDst, pSrc;
	GCPtr pGC;
	RegionPtr pRgn[MAXSCREENS];

	FOR_NSCREENS_BACKWARD(j) {
	    stuff->dstDrawable = dst->info[j].id;
	    stuff->srcDrawable = src->info[j].id;
	    stuff->gc          = gc->info[j].id;
 	    if (srcIsRoot) {	
		stuff->srcX = srcx - panoramiXdataPtr[j].x;
		stuff->srcY = srcy - panoramiXdataPtr[j].y;
	    }
 	    if (dstIsRoot) {	
		stuff->dstX = dstx - panoramiXdataPtr[j].x;
		stuff->dstY = dsty - panoramiXdataPtr[j].y;
	    }

	    VALIDATE_DRAWABLE_AND_GC(stuff->dstDrawable, pDst, pGC, client); 
	    if (stuff->dstDrawable != stuff->srcDrawable) {
		SECURITY_VERIFY_DRAWABLE(pSrc, stuff->srcDrawable, client,
                                 SecurityReadAccess);
		if ((pDst->pScreen != pSrc->pScreen) || 
		    (pDst->depth != pSrc->depth)) {
			client->errorValue = stuff->dstDrawable;
			return (BadMatch);
   		}
 	    } else
		pSrc = pDst;

	    pRgn[j] = (*pGC->ops->CopyArea)(pSrc, pDst, pGC, 
				stuff->srcX, stuff->srcY,
				stuff->width, stuff->height, 
				stuff->dstX, stuff->dstY);

	    if(dstShared) {
		while(j--) pRgn[j] = NULL;
		break;
	    }
	}

	if(pGC->graphicsExposures) {
	    ScreenPtr pScreen = pDst->pScreen;
	    RegionRec totalReg;
	    Bool overlap;

	    REGION_INIT(pScreen, &totalReg, NullBox, 1);
	    FOR_NSCREENS_BACKWARD(j) {
		if(pRgn[j]) {
		   if(srcIsRoot) {
		       REGION_TRANSLATE(pScreen, pRgn[j], 
				panoramiXdataPtr[j].x, panoramiXdataPtr[j].y);
		   }
		   REGION_APPEND(pScreen, &totalReg, pRgn[j]);
		   REGION_DESTROY(pScreen, pRgn[j]);
		}
	    }
	    REGION_VALIDATE(pScreen, &totalReg, &overlap);
	    (*pScreen->SendGraphicsExpose)(
		client, &totalReg, stuff->dstDrawable, X_CopyArea, 0);
	    REGION_UNINIT(pScreen, &totalReg);
	}
	
	result = client->noClientException;
    }

    return (result);
}


int PanoramiXCopyPlane(ClientPtr client)
{
    int			j, srcx, srcy, dstx, dsty;
    PanoramiXRes	*gc, *src, *dst;
    Bool		srcIsRoot = FALSE;
    Bool		dstIsRoot = FALSE;
    Bool		srcShared, dstShared;
    DrawablePtr 	psrcDraw, pdstDraw;
    GCPtr 		pGC;
    RegionPtr 		pRgn[MAXSCREENS];
    REQUEST(xCopyPlaneReq);

    REQUEST_SIZE_MATCH(xCopyPlaneReq);

    if(!(src = (PanoramiXRes *)SecurityLookupIDByClass(
		client, stuff->srcDrawable, XRC_DRAWABLE, SecurityReadAccess)))
	return BadDrawable;    

    srcShared = IS_SHARED_PIXMAP(src);

    if(!(dst = (PanoramiXRes *)SecurityLookupIDByClass(
		client, stuff->dstDrawable, XRC_DRAWABLE, SecurityWriteAccess)))
	return BadDrawable;

    dstShared = IS_SHARED_PIXMAP(dst);

    if(dstShared && srcShared)
	return (* SavedProcVector[X_CopyPlane])(client);

    if(!(gc = (PanoramiXRes *)SecurityLookupIDByType(
		client, stuff->gc, XRT_GC, SecurityReadAccess)))
	return BadGC;

    if((dst->type == XRT_WINDOW) &&
       (stuff->dstDrawable == WindowTable[0]->drawable.id))
	dstIsRoot = TRUE;
    if((src->type == XRT_WINDOW) &&
       (stuff->srcDrawable == WindowTable[0]->drawable.id))
	srcIsRoot = TRUE;

    srcx = stuff->srcX; srcy = stuff->srcY;
    dstx = stuff->dstX; dsty = stuff->dstY;
 
    FOR_NSCREENS_BACKWARD(j) {
	stuff->dstDrawable = dst->info[j].id;
	stuff->srcDrawable = src->info[j].id;
	stuff->gc          = gc->info[j].id;
	if (srcIsRoot) {	
	    stuff->srcX = srcx - panoramiXdataPtr[j].x;
	    stuff->srcY = srcy - panoramiXdataPtr[j].y;
	}
	if (dstIsRoot) {	
	    stuff->dstX = dstx - panoramiXdataPtr[j].x;
	    stuff->dstY = dsty - panoramiXdataPtr[j].y;
	}

	VALIDATE_DRAWABLE_AND_GC(stuff->dstDrawable, pdstDraw, pGC, client);
	if (stuff->dstDrawable != stuff->srcDrawable) {
	    SECURITY_VERIFY_DRAWABLE(psrcDraw, stuff->srcDrawable, client,
                                 SecurityReadAccess);
            if (pdstDraw->pScreen != psrcDraw->pScreen) {
		client->errorValue = stuff->dstDrawable;
		return (BadMatch);
	    }
	} else
	    psrcDraw = pdstDraw;

	if(stuff->bitPlane == 0 || (stuff->bitPlane & (stuff->bitPlane - 1)) ||
		(stuff->bitPlane > (1L << (psrcDraw->depth - 1)))) {
	    client->errorValue = stuff->bitPlane;
	    return(BadValue);
	}

	pRgn[j] = (*pGC->ops->CopyPlane)(psrcDraw, pdstDraw, pGC, 
				stuff->srcX, stuff->srcY,
				stuff->width, stuff->height, 
				stuff->dstX, stuff->dstY, stuff->bitPlane);

	if(dstShared) {
	    while(j--) pRgn[j] = NULL;
	    break;
	}
    }

    if(pGC->graphicsExposures) {
	ScreenPtr pScreen = pdstDraw->pScreen;
	RegionRec totalReg;
	Bool overlap;

	REGION_INIT(pScreen, &totalReg, NullBox, 1);
	FOR_NSCREENS_BACKWARD(j) {
	    if(pRgn[j]) {
		REGION_APPEND(pScreen, &totalReg, pRgn[j]);
		REGION_DESTROY(pScreen, pRgn[j]);
	    }
	}
	REGION_VALIDATE(pScreen, &totalReg, &overlap);
	(*pScreen->SendGraphicsExpose)(
		client, &totalReg, stuff->dstDrawable, X_CopyPlane, 0);
	REGION_UNINIT(pScreen, &totalReg);
    }

    return (client->noClientException);
}


int PanoramiXPolyPoint(ClientPtr client)
{
    PanoramiXRes *gc, *draw;
    int 	  result, npoint, j;
    xPoint 	  *origPts;
    Bool	  isRoot;
    REQUEST(xPolyPointReq);

    REQUEST_AT_LEAST_SIZE(xPolyPointReq);

    if(!(draw = (PanoramiXRes *)SecurityLookupIDByClass(
		client, stuff->drawable, XRC_DRAWABLE, SecurityWriteAccess)))
	return BadDrawable;

    if(IS_SHARED_PIXMAP(draw))
	return (*SavedProcVector[X_PolyPoint])(client);

    if(!(gc = (PanoramiXRes *)SecurityLookupIDByType(
		client, stuff->gc, XRT_GC, SecurityReadAccess)))
	return BadGC;    

    isRoot = (draw->type == XRT_WINDOW) &&
		(stuff->drawable == WindowTable[0]->drawable.id);
    npoint = ((client->req_len << 2) - sizeof(xPolyPointReq)) >> 2;
    if (npoint > 0) {
        origPts = (xPoint *) ALLOCATE_LOCAL(npoint * sizeof(xPoint));
        memcpy((char *) origPts, (char *) &stuff[1], npoint * sizeof(xPoint));
        FOR_NSCREENS_FORWARD(j){

            if(j) memcpy(&stuff[1], origPts, npoint * sizeof(xPoint));

            if (isRoot) {
                int x_off = panoramiXdataPtr[j].x;
                int y_off = panoramiXdataPtr[j].y;

		if(x_off || y_off) {
                    xPoint *pnts = (xPoint*)&stuff[1];
		    int i = (stuff->coordMode==CoordModePrevious) ? 1 : npoint;

		    while(i--) {
			pnts->x -= x_off;
			pnts->y -= y_off;
			pnts++;
                    }
		}
            }

	    stuff->drawable = draw->info[j].id;
	    stuff->gc = gc->info[j].id;
	    result = (* SavedProcVector[X_PolyPoint])(client);
	    if(result != Success) break;
        }
        DEALLOCATE_LOCAL(origPts);
        return (result);
    } else
	return (client->noClientException);
}


int PanoramiXPolyLine(ClientPtr client)
{
    PanoramiXRes *gc, *draw;
    int 	  result, npoint, j;
    xPoint 	  *origPts;
    Bool	  isRoot;
    REQUEST(xPolyLineReq);

    REQUEST_AT_LEAST_SIZE(xPolyLineReq);

    if(!(draw = (PanoramiXRes *)SecurityLookupIDByClass(
		client, stuff->drawable, XRC_DRAWABLE, SecurityWriteAccess)))
	return BadDrawable;    

    if(IS_SHARED_PIXMAP(draw))
	return (*SavedProcVector[X_PolyLine])(client);

    if(!(gc = (PanoramiXRes *)SecurityLookupIDByType(
		client, stuff->gc, XRT_GC, SecurityReadAccess)))
	return BadGC;    

    isRoot = (draw->type == XRT_WINDOW) &&
		(stuff->drawable == WindowTable[0]->drawable.id);
    npoint = ((client->req_len << 2) - sizeof(xPolyLineReq)) >> 2;
    if (npoint > 0){
        origPts = (xPoint *) ALLOCATE_LOCAL(npoint * sizeof(xPoint));
        memcpy((char *) origPts, (char *) &stuff[1], npoint * sizeof(xPoint));
        FOR_NSCREENS_FORWARD(j){

            if(j) memcpy(&stuff[1], origPts, npoint * sizeof(xPoint));

            if (isRoot) {
                int x_off = panoramiXdataPtr[j].x;
                int y_off = panoramiXdataPtr[j].y;

		if(x_off || y_off) {
		    xPoint *pnts = (xPoint*)&stuff[1];
		    int i = (stuff->coordMode==CoordModePrevious) ? 1 : npoint;

		    while(i--) {
			pnts->x -= x_off;
			pnts->y -= y_off;
			pnts++;
		    }
		}
            }

	    stuff->drawable = draw->info[j].id;
	    stuff->gc = gc->info[j].id;
	    result = (* SavedProcVector[X_PolyLine])(client);
	    if(result != Success) break;
        }
        DEALLOCATE_LOCAL(origPts);
        return (result);
   } else
	return (client->noClientException);
}


int PanoramiXPolySegment(ClientPtr client)
{
    int		  result, nsegs, i, j;
    PanoramiXRes *gc, *draw;
    xSegment 	  *origSegs;
    Bool	  isRoot;
    REQUEST(xPolySegmentReq);

    REQUEST_AT_LEAST_SIZE(xPolySegmentReq);

    if(!(draw = (PanoramiXRes *)SecurityLookupIDByClass(
		client, stuff->drawable, XRC_DRAWABLE, SecurityWriteAccess)))
	return BadDrawable;    

    if(IS_SHARED_PIXMAP(draw))
	return (*SavedProcVector[X_PolySegment])(client);

    if(!(gc = (PanoramiXRes *)SecurityLookupIDByType(
		client, stuff->gc, XRT_GC, SecurityReadAccess)))
	return BadGC;    

    isRoot = (draw->type == XRT_WINDOW) &&
		(stuff->drawable == WindowTable[0]->drawable.id);

    nsegs = (client->req_len << 2) - sizeof(xPolySegmentReq);
    if(nsegs & 4) return BadLength;
    nsegs >>= 3;
    if (nsegs > 0) {
	origSegs = (xSegment *) ALLOCATE_LOCAL(nsegs * sizeof(xSegment));
        memcpy((char *) origSegs, (char *) &stuff[1], nsegs * sizeof(xSegment));
        FOR_NSCREENS_FORWARD(j){

            if(j) memcpy(&stuff[1], origSegs, nsegs * sizeof(xSegment));

            if (isRoot) {
                int x_off = panoramiXdataPtr[j].x;
                int y_off = panoramiXdataPtr[j].y;

		if(x_off || y_off) {
		    xSegment *segs = (xSegment*)&stuff[1];

		    for (i = nsegs; i--; segs++) {
			segs->x1 -= x_off;
			segs->x2 -= x_off;
			segs->y1 -= y_off;
			segs->y2 -= y_off;
		    }
		}
            }

	    stuff->drawable = draw->info[j].id;
	    stuff->gc = gc->info[j].id;
	    result = (* SavedProcVector[X_PolySegment])(client);
	    if(result != Success) break;
    	}
	DEALLOCATE_LOCAL(origSegs);
	return (result);
    } else
	  return (client->noClientException);
}


int PanoramiXPolyRectangle(ClientPtr client)
{
    int 	  result, nrects, i, j;
    PanoramiXRes *gc, *draw;
    Bool	  isRoot;
    xRectangle 	  *origRecs;
    REQUEST(xPolyRectangleReq);

    REQUEST_AT_LEAST_SIZE(xPolyRectangleReq);


    if(!(draw = (PanoramiXRes *)SecurityLookupIDByClass(
		client, stuff->drawable, XRC_DRAWABLE, SecurityWriteAccess)))
	return BadDrawable;

    if(IS_SHARED_PIXMAP(draw))
	return (*SavedProcVector[X_PolyRectangle])(client);

    if(!(gc = (PanoramiXRes *)SecurityLookupIDByType(
		client, stuff->gc, XRT_GC, SecurityReadAccess)))
	return BadGC;    

    isRoot = (draw->type == XRT_WINDOW) &&
		(stuff->drawable == WindowTable[0]->drawable.id);

    nrects = (client->req_len << 2) - sizeof(xPolyRectangleReq);
    if(nrects & 4) return BadLength;
    nrects >>= 3;
    if (nrects > 0){
	origRecs = (xRectangle *) ALLOCATE_LOCAL(nrects * sizeof(xRectangle));
	memcpy((char *)origRecs,(char *)&stuff[1],nrects * sizeof(xRectangle));
        FOR_NSCREENS_FORWARD(j){

            if(j) memcpy(&stuff[1], origRecs, nrects * sizeof(xRectangle));

	    if (isRoot) {
		int x_off = panoramiXdataPtr[j].x;
		int y_off = panoramiXdataPtr[j].y;


		if(x_off || y_off) {
	    	    xRectangle *rects = (xRectangle *) &stuff[1];

		    for (i = nrects; i--; rects++) {
			rects->x -= x_off;
			rects->y -= y_off;
		    }
		}
	    } 

	    stuff->drawable = draw->info[j].id;
	    stuff->gc = gc->info[j].id;
	    result = (* SavedProcVector[X_PolyRectangle])(client);
	    if(result != Success) break;
	}
	DEALLOCATE_LOCAL(origRecs);
	return (result);
    } else
       return (client->noClientException);
}


int PanoramiXPolyArc(ClientPtr client)
{
    int 	  result, narcs, i, j;
    PanoramiXRes *gc, *draw;
    Bool	  isRoot;
    xArc	  *origArcs;
    REQUEST(xPolyArcReq);

    REQUEST_AT_LEAST_SIZE(xPolyArcReq);

    if(!(draw = (PanoramiXRes *)SecurityLookupIDByClass(
		client, stuff->drawable, XRC_DRAWABLE, SecurityWriteAccess)))
	return BadDrawable;    

    if(IS_SHARED_PIXMAP(draw))
	return (*SavedProcVector[X_PolyArc])(client);

    if(!(gc = (PanoramiXRes *)SecurityLookupIDByType(
		client, stuff->gc, XRT_GC, SecurityReadAccess)))
	return BadGC;    

    isRoot = (draw->type == XRT_WINDOW) &&
		(stuff->drawable == WindowTable[0]->drawable.id);

    narcs = (client->req_len << 2) - sizeof(xPolyArcReq);
    if(narcs % sizeof(xArc)) return BadLength;
    narcs /= sizeof(xArc);
    if (narcs > 0){
	origArcs = (xArc *) ALLOCATE_LOCAL(narcs * sizeof(xArc));
	memcpy((char *) origArcs, (char *) &stuff[1], narcs * sizeof(xArc));
        FOR_NSCREENS_FORWARD(j){

            if(j) memcpy(&stuff[1], origArcs, narcs * sizeof(xArc));

	    if (isRoot) {
		int x_off = panoramiXdataPtr[j].x;
		int y_off = panoramiXdataPtr[j].y;
	
		if(x_off || y_off) {
		    xArc *arcs = (xArc *) &stuff[1];

		    for (i = narcs; i--; arcs++) {
			arcs->x -= x_off;
			arcs->y -= y_off;
		    }
		}
            }
	    stuff->drawable = draw->info[j].id;
	    stuff->gc = gc->info[j].id;
	    result = (* SavedProcVector[X_PolyArc])(client);
	    if(result != Success) break;
        }
	DEALLOCATE_LOCAL(origArcs);
	return (result);
    } else
       return (client->noClientException);
}


int PanoramiXFillPoly(ClientPtr client)
{
    int 	  result, count, j;
    PanoramiXRes *gc, *draw;
    Bool	  isRoot;
    DDXPointPtr	  locPts;
    REQUEST(xFillPolyReq);

    REQUEST_AT_LEAST_SIZE(xFillPolyReq);

    if(!(draw = (PanoramiXRes *)SecurityLookupIDByClass(
		client, stuff->drawable, XRC_DRAWABLE, SecurityWriteAccess)))
	return BadDrawable;    

    if(IS_SHARED_PIXMAP(draw))
	return (*SavedProcVector[X_FillPoly])(client);

    if(!(gc = (PanoramiXRes *)SecurityLookupIDByType(
		client, stuff->gc, XRT_GC, SecurityReadAccess)))
	return BadGC;    

    isRoot = (draw->type == XRT_WINDOW) &&
		(stuff->drawable == WindowTable[0]->drawable.id);

    count = ((client->req_len << 2) - sizeof(xFillPolyReq)) >> 2;
    if (count > 0){
	locPts = (DDXPointPtr) ALLOCATE_LOCAL(count * sizeof(DDXPointRec));
	memcpy((char *)locPts, (char *)&stuff[1], count * sizeof(DDXPointRec));
        FOR_NSCREENS_FORWARD(j){

	    if(j) memcpy(&stuff[1], locPts, count * sizeof(DDXPointRec));

	    if (isRoot) {
		int x_off = panoramiXdataPtr[j].x;
		int y_off = panoramiXdataPtr[j].y;

		if(x_off || y_off) {
		    DDXPointPtr pnts = (DDXPointPtr)&stuff[1];
		    int i = (stuff->coordMode==CoordModePrevious) ? 1 : count;

		    while(i--) {
			pnts->x -= x_off;
			pnts->y -= y_off;
			pnts++;
		    }
		}
	    }

	    stuff->drawable = draw->info[j].id;
	    stuff->gc = gc->info[j].id;
	    result = (* SavedProcVector[X_FillPoly])(client);
	    if(result != Success) break;
	}
	DEALLOCATE_LOCAL(locPts);
	return (result);
    } else
       return (client->noClientException);
}


int PanoramiXPolyFillRectangle(ClientPtr client)
{
    int 	  result, things, i, j;
    PanoramiXRes *gc, *draw;
    Bool	  isRoot;
    xRectangle	  *origRects;
    REQUEST(xPolyFillRectangleReq);

    REQUEST_AT_LEAST_SIZE(xPolyFillRectangleReq);

    if(!(draw = (PanoramiXRes *)SecurityLookupIDByClass(
		client, stuff->drawable, XRC_DRAWABLE, SecurityWriteAccess)))
	return BadDrawable;    

    if(IS_SHARED_PIXMAP(draw))
	return (*SavedProcVector[X_PolyFillRectangle])(client);

    if(!(gc = (PanoramiXRes *)SecurityLookupIDByType(
		client, stuff->gc, XRT_GC, SecurityReadAccess)))
	return BadGC;    

    isRoot = (draw->type == XRT_WINDOW) &&
		(stuff->drawable == WindowTable[0]->drawable.id);

    things = (client->req_len << 2) - sizeof(xPolyFillRectangleReq);
    if(things & 4) return BadLength;
    things >>= 3;
    if (things > 0){
	origRects = (xRectangle *) ALLOCATE_LOCAL(things * sizeof(xRectangle));
	memcpy((char*)origRects,(char*)&stuff[1], things * sizeof(xRectangle));
        FOR_NSCREENS_FORWARD(j){

	    if(j) memcpy(&stuff[1], origRects, things * sizeof(xRectangle));

	    if (isRoot) {
		int x_off = panoramiXdataPtr[j].x;
		int y_off = panoramiXdataPtr[j].y;

		if(x_off || y_off) {
		    xRectangle *rects = (xRectangle *) &stuff[1];

		    for (i = things; i--; rects++) {
			rects->x -= x_off;
			rects->y -= y_off;
		    }
		}
	    }

	    stuff->drawable = draw->info[j].id;
	    stuff->gc = gc->info[j].id;
	    result = (* SavedProcVector[X_PolyFillRectangle])(client);
	    if(result != Success) break;
	}
	DEALLOCATE_LOCAL(origRects);
	return (result);
    } else
       return (client->noClientException);
}


int PanoramiXPolyFillArc(ClientPtr client)
{
    PanoramiXRes *gc, *draw;
    Bool	  isRoot;
    int 	  result, narcs, i, j;
    xArc	  *origArcs;
    REQUEST(xPolyFillArcReq);

    REQUEST_AT_LEAST_SIZE(xPolyFillArcReq);

    if(!(draw = (PanoramiXRes *)SecurityLookupIDByClass(
		client, stuff->drawable, XRC_DRAWABLE, SecurityWriteAccess)))
	return BadDrawable;    

    if(IS_SHARED_PIXMAP(draw))
	return (*SavedProcVector[X_PolyFillArc])(client);

    if(!(gc = (PanoramiXRes *)SecurityLookupIDByType(
		client, stuff->gc, XRT_GC, SecurityReadAccess)))
	return BadGC;    

    isRoot = (draw->type == XRT_WINDOW) &&
		(stuff->drawable == WindowTable[0]->drawable.id);

    narcs = (client->req_len << 2) - sizeof(xPolyFillArcReq);
    IF_RETURN((narcs % sizeof(xArc)), BadLength);
    narcs /= sizeof(xArc);
    if (narcs > 0) {
	origArcs = (xArc *) ALLOCATE_LOCAL(narcs * sizeof(xArc));
	memcpy((char *) origArcs, (char *)&stuff[1], narcs * sizeof(xArc));
        FOR_NSCREENS_FORWARD(j){

	    if(j) memcpy(&stuff[1], origArcs, narcs * sizeof(xArc));

	    if (isRoot) {
		int x_off = panoramiXdataPtr[j].x;
		int y_off = panoramiXdataPtr[j].y;

		if(x_off || y_off) {
		    xArc *arcs = (xArc *) &stuff[1];

		    for (i = narcs; i--; arcs++) {
			arcs->x -= x_off;
			arcs->y -= y_off;
		    }
		}
	    }

	    stuff->drawable = draw->info[j].id;
	    stuff->gc = gc->info[j].id;
	    result = (* SavedProcVector[X_PolyFillArc])(client);
	    if(result != Success) break;
	}
	DEALLOCATE_LOCAL(origArcs);
	return (result);
    } else
       return (client->noClientException);
}


int PanoramiXPutImage(ClientPtr client)
{
    PanoramiXRes *gc, *draw;
    Bool	  isRoot;
    int		  j, result, orig_x, orig_y;
    REQUEST(xPutImageReq);

    REQUEST_AT_LEAST_SIZE(xPutImageReq);

    if(!(draw = (PanoramiXRes *)SecurityLookupIDByClass(
		client, stuff->drawable, XRC_DRAWABLE, SecurityWriteAccess)))
	return BadDrawable;    

    if(IS_SHARED_PIXMAP(draw))
	return (*SavedProcVector[X_PutImage])(client);

    if(!(gc = (PanoramiXRes *)SecurityLookupIDByType(
		client, stuff->gc, XRT_GC, SecurityReadAccess)))
	return BadGC;    

    isRoot = (draw->type == XRT_WINDOW) &&
		(stuff->drawable == WindowTable[0]->drawable.id);

    orig_x = stuff->dstX;
    orig_y = stuff->dstY;
    FOR_NSCREENS_BACKWARD(j){
	if (isRoot) {
    	  stuff->dstX = orig_x - panoramiXdataPtr[j].x;
	  stuff->dstY = orig_y - panoramiXdataPtr[j].y;
	}
	stuff->drawable = draw->info[j].id;
	stuff->gc = gc->info[j].id;
	result = (* SavedProcVector[X_PutImage])(client);
	if(result != Success) break;
    }
    return (result);
}


int PanoramiXGetImage(ClientPtr client)
{
    DrawablePtr 	drawables[MAXSCREENS];
    DrawablePtr 	pDraw;
    PanoramiXRes	*draw;
    xGetImageReply	xgi;
    Bool		isRoot;
    char		*pBuf;
    int         	i, x, y, w, h, format;
    Mask		plane, planemask;
    int			linesDone, nlines, linesPerBuf;
    long		widthBytesLine, length;

    REQUEST(xGetImageReq);

    REQUEST_SIZE_MATCH(xGetImageReq);

    if ((stuff->format != XYPixmap) && (stuff->format != ZPixmap)) {
	client->errorValue = stuff->format;
        return(BadValue);
    }

    if(!(draw = (PanoramiXRes *)SecurityLookupIDByClass(
		client, stuff->drawable, XRC_DRAWABLE, SecurityWriteAccess)))
	return BadDrawable;

    if(draw->type == XRT_PIXMAP)
	return (*SavedProcVector[X_GetImage])(client);

    VERIFY_DRAWABLE(pDraw, stuff->drawable, client);

    if(!((WindowPtr)pDraw)->realized)
	return(BadMatch);

    x = stuff->x;
    y = stuff->y;
    w = stuff->width;
    h = stuff->height;
    format = stuff->format;
    planemask = stuff->planeMask;

    isRoot = (draw->type == XRT_WINDOW) &&
		(stuff->drawable == WindowTable[0]->drawable.id);

    if(isRoot) {
      if( /* check for being onscreen */
	x < 0 || x + w > PanoramiXPixWidth ||
	y < 0 || y + h > PanoramiXPixHeight )
	    return(BadMatch);
    } else {
      if( /* check for being onscreen */
	panoramiXdataPtr[0].x + pDraw->x + x < 0 ||
	panoramiXdataPtr[0].x + pDraw->x + x + w > PanoramiXPixWidth ||
        panoramiXdataPtr[0].y + pDraw->y + y < 0 ||
	panoramiXdataPtr[0].y + pDraw->y + y + h > PanoramiXPixHeight ||
	 /* check for being inside of border */
       	x < - wBorderWidth((WindowPtr)pDraw) ||
	x + w > wBorderWidth((WindowPtr)pDraw) + (int)pDraw->width ||
	y < -wBorderWidth((WindowPtr)pDraw) ||
	y + h > wBorderWidth ((WindowPtr)pDraw) + (int)pDraw->height)
	    return(BadMatch);
    }

    drawables[0] = pDraw;
    for(i = 1; i < PanoramiXNumScreens; i++)
	VERIFY_DRAWABLE(drawables[i], draw->info[i].id, client);

    xgi.visual = wVisual (((WindowPtr) pDraw));
    xgi.type = X_Reply;
    xgi.sequenceNumber = client->sequence;
    xgi.depth = pDraw->depth;
    if(format == ZPixmap) {
	widthBytesLine = PixmapBytePad(w, pDraw->depth);
	length = widthBytesLine * h;


    } else {
	widthBytesLine = BitmapBytePad(w);
	plane = ((Mask)1) << (pDraw->depth - 1);
	/* only planes asked for */
	length = widthBytesLine * h *
		 Ones(planemask & (plane | (plane - 1)));

    }

    xgi.length = (length + 3) >> 2;

    if (widthBytesLine == 0 || h == 0)
	linesPerBuf = 0;
    else if (widthBytesLine >= XINERAMA_IMAGE_BUFSIZE)
	linesPerBuf = 1;
    else {
	linesPerBuf = XINERAMA_IMAGE_BUFSIZE / widthBytesLine;
	if (linesPerBuf > h)
	    linesPerBuf = h;
    }
    length = linesPerBuf * widthBytesLine;
    if(!(pBuf = xalloc(length)))
	return (BadAlloc);

    WriteReplyToClient(client, sizeof (xGetImageReply), &xgi);

    if (linesPerBuf == 0) {
	/* nothing to do */
    }
    else if (format == ZPixmap) {
        linesDone = 0;
        while (h - linesDone > 0) {
	    nlines = min(linesPerBuf, h - linesDone);

	    if(pDraw->depth == 1)
		bzero(pBuf, nlines * widthBytesLine);

	    XineramaGetImageData(drawables, x, y + linesDone, w, nlines,
			format, planemask, pBuf, widthBytesLine, isRoot);

		(void)WriteToClient(client,
				    (int)(nlines * widthBytesLine),
				    pBuf);
	    linesDone += nlines;
        }
    } else { /* XYPixmap */
        for (; plane; plane >>= 1) {
	    if (planemask & plane) {
	        linesDone = 0;
	        while (h - linesDone > 0) {
		    nlines = min(linesPerBuf, h - linesDone);

		    bzero(pBuf, nlines * widthBytesLine);

		    XineramaGetImageData(drawables, x, y + linesDone, w, 
					nlines, format, plane, pBuf,
					widthBytesLine, isRoot);

		    (void)WriteToClient(client,
				    (int)(nlines * widthBytesLine),
				    pBuf);

		    linesDone += nlines;
		}
            }
	}
    }
    xfree(pBuf);
    return (client->noClientException);
}


/* The text stuff should be rewritten so that duplication happens
   at the GlyphBlt level.  That is, loading the font and getting
   the glyphs should only happen once */

int 
PanoramiXPolyText8(ClientPtr client)
{
    PanoramiXRes *gc, *draw;
    Bool	  isRoot;
    int 	  result, j;
    int	 	  orig_x, orig_y;
    REQUEST(xPolyTextReq);

    REQUEST_AT_LEAST_SIZE(xPolyTextReq);

    if(!(draw = (PanoramiXRes *)SecurityLookupIDByClass(
		client, stuff->drawable, XRC_DRAWABLE, SecurityWriteAccess)))
	return BadDrawable;    

    if(IS_SHARED_PIXMAP(draw))
	return (*SavedProcVector[X_PolyText8])(client);

    if(!(gc = (PanoramiXRes *)SecurityLookupIDByType(
		client, stuff->gc, XRT_GC, SecurityReadAccess)))
	return BadGC;    

    isRoot = (draw->type == XRT_WINDOW) &&
		(stuff->drawable == WindowTable[0]->drawable.id);

    orig_x = stuff->x;
    orig_y = stuff->y;
    FOR_NSCREENS_BACKWARD(j){
	stuff->drawable = draw->info[j].id;
	stuff->gc = gc->info[j].id;
	if (isRoot) {
	    stuff->x = orig_x - panoramiXdataPtr[j].x;
	    stuff->y = orig_y - panoramiXdataPtr[j].y;
	}
	result = (*SavedProcVector[X_PolyText8])(client);
	if(result != Success) break;
    }
    return (result);
}

int 
PanoramiXPolyText16(ClientPtr client)
{
    PanoramiXRes *gc, *draw;
    Bool	  isRoot;
    int 	  result, j;
    int	 	  orig_x, orig_y;
    REQUEST(xPolyTextReq);

    REQUEST_AT_LEAST_SIZE(xPolyTextReq);

    if(!(draw = (PanoramiXRes *)SecurityLookupIDByClass(
		client, stuff->drawable, XRC_DRAWABLE, SecurityWriteAccess)))
	return BadDrawable;    

    if(IS_SHARED_PIXMAP(draw))
	return (*SavedProcVector[X_PolyText16])(client);

    if(!(gc = (PanoramiXRes *)SecurityLookupIDByType(
		client, stuff->gc, XRT_GC, SecurityReadAccess)))
	return BadGC;    

    isRoot = (draw->type == XRT_WINDOW) &&
		(stuff->drawable == WindowTable[0]->drawable.id);

    orig_x = stuff->x;
    orig_y = stuff->y;
    FOR_NSCREENS_BACKWARD(j){
	stuff->drawable = draw->info[j].id;
	stuff->gc = gc->info[j].id;
	if (isRoot) {
	    stuff->x = orig_x - panoramiXdataPtr[j].x;
	    stuff->y = orig_y - panoramiXdataPtr[j].y;
	}
	result = (*SavedProcVector[X_PolyText16])(client);
	if(result != Success) break;
    }
    return (result);
}


int PanoramiXImageText8(ClientPtr client)
{
    int 	  result, j;
    PanoramiXRes *gc, *draw;
    Bool	  isRoot;
    int		  orig_x, orig_y;
    REQUEST(xImageTextReq);

    REQUEST_FIXED_SIZE(xImageTextReq, stuff->nChars);

    if(!(draw = (PanoramiXRes *)SecurityLookupIDByClass(
		client, stuff->drawable, XRC_DRAWABLE, SecurityWriteAccess)))
	return BadDrawable;    

    if(IS_SHARED_PIXMAP(draw))
	return (*SavedProcVector[X_ImageText8])(client);

    if(!(gc = (PanoramiXRes *)SecurityLookupIDByType(
		client, stuff->gc, XRT_GC, SecurityReadAccess)))
	return BadGC;    

    isRoot = (draw->type == XRT_WINDOW) &&
		(stuff->drawable == WindowTable[0]->drawable.id);

    orig_x = stuff->x;
    orig_y = stuff->y;
    FOR_NSCREENS_BACKWARD(j){
	stuff->drawable = draw->info[j].id;
	stuff->gc = gc->info[j].id;
	if (isRoot) {
	    stuff->x = orig_x - panoramiXdataPtr[j].x;
	    stuff->y = orig_y - panoramiXdataPtr[j].y;
	}
	result = (*SavedProcVector[X_ImageText8])(client);
	if(result != Success) break;
    }
    return (result);
}


int PanoramiXImageText16(ClientPtr client)
{
    int 	  result, j;
    PanoramiXRes *gc, *draw;
    Bool	  isRoot;
    int		  orig_x, orig_y;
    REQUEST(xImageTextReq);

    REQUEST_FIXED_SIZE(xImageTextReq, stuff->nChars << 1);

    if(!(draw = (PanoramiXRes *)SecurityLookupIDByClass(
		client, stuff->drawable, XRC_DRAWABLE, SecurityWriteAccess)))
	return BadDrawable;    

    if(IS_SHARED_PIXMAP(draw))
	return (*SavedProcVector[X_ImageText16])(client);

    if(!(gc = (PanoramiXRes *)SecurityLookupIDByType(
		client, stuff->gc, XRT_GC, SecurityReadAccess)))
	return BadGC;    

    isRoot = (draw->type == XRT_WINDOW) &&
		(stuff->drawable == WindowTable[0]->drawable.id);

    orig_x = stuff->x;
    orig_y = stuff->y;
    FOR_NSCREENS_BACKWARD(j){
	stuff->drawable = draw->info[j].id;
	stuff->gc = gc->info[j].id;
	if (isRoot) {
	    stuff->x = orig_x - panoramiXdataPtr[j].x;
	    stuff->y = orig_y - panoramiXdataPtr[j].y;
	}
	result = (*SavedProcVector[X_ImageText16])(client);
	if(result != Success) break;
    }
    return (result);
}



int PanoramiXCreateColormap(ClientPtr client)
{
    PanoramiXRes	*win, *newCmap;
    int 		result, j, orig_visual;
    REQUEST(xCreateColormapReq);

    REQUEST_SIZE_MATCH(xCreateColormapReq);

    if(!(win = (PanoramiXRes *)SecurityLookupIDByType(
		client, stuff->window, XRT_WINDOW, SecurityReadAccess)))
	return BadWindow;    

    if(!stuff->visual || (stuff->visual > 255)) 
	return BadValue;

    if(!(newCmap = (PanoramiXRes *) xalloc(sizeof(PanoramiXRes))))
        return BadAlloc;

    newCmap->type = XRT_COLORMAP;
    newCmap->info[0].id = stuff->mid;
    for(j = 1; j < PanoramiXNumScreens; j++)
        newCmap->info[j].id = FakeClientID(client->index);

    orig_visual = stuff->visual;
    FOR_NSCREENS_BACKWARD(j){
	stuff->mid = newCmap->info[j].id;
	stuff->window = win->info[j].id;
	stuff->visual = PanoramiXVisualTable[orig_visual][j];
	result = (* SavedProcVector[X_CreateColormap])(client);
	if(result != Success) break;
    }
 
    if (result == Success)
        AddResource(newCmap->info[0].id, XRT_COLORMAP, newCmap);
    else 
        xfree(newCmap);

    return (result);
}


int PanoramiXFreeColormap(ClientPtr client)
{
    PanoramiXRes *cmap;
    int          result, j;
    REQUEST(xResourceReq);

    REQUEST_SIZE_MATCH(xResourceReq);

    client->errorValue = stuff->id;

    if(!(cmap = (PanoramiXRes *)SecurityLookupIDByType(
                client, stuff->id, XRT_COLORMAP, SecurityDestroyAccess)))
        return BadColor;

    FOR_NSCREENS_BACKWARD(j) {
        stuff->id = cmap->info[j].id;
        result = (* SavedProcVector[X_FreeColormap])(client);
	if(result != Success) break;
    }

    /* Since ProcFreeColormap is using FreeResource, it will free
	our resource for us on the last pass through the loop above */

    return (result);
}


int
PanoramiXCopyColormapAndFree(ClientPtr client)
{
    PanoramiXRes *cmap, *newCmap;
    int          result, j;
    REQUEST(xCopyColormapAndFreeReq);

    REQUEST_SIZE_MATCH(xCopyColormapAndFreeReq);

    client->errorValue = stuff->srcCmap;

    if(!(cmap = (PanoramiXRes *)SecurityLookupIDByType(
                client, stuff->srcCmap, XRT_COLORMAP, 
		SecurityReadAccess | SecurityWriteAccess)))
        return BadColor;

    if(!(newCmap = (PanoramiXRes *) xalloc(sizeof(PanoramiXRes))))
        return BadAlloc;

    newCmap->type = XRT_COLORMAP;
    newCmap->info[0].id = stuff->mid;
    for(j = 1; j < PanoramiXNumScreens; j++)
        newCmap->info[j].id = FakeClientID(client->index);

    FOR_NSCREENS_BACKWARD(j){
        stuff->srcCmap = cmap->info[j].id;
	stuff->mid = newCmap->info[j].id;
        result = (* SavedProcVector[X_CopyColormapAndFree])(client);
	if(result != Success) break;
    }

    if (result == Success)
        AddResource(newCmap->info[0].id, XRT_COLORMAP, newCmap);
    else 
        xfree(newCmap);

    return (result);
}


int PanoramiXInstallColormap(ClientPtr client)
{
    REQUEST(xResourceReq);
    int 	result, j;
    PanoramiXRes *cmap;

    REQUEST_SIZE_MATCH(xResourceReq);

    client->errorValue = stuff->id;

    if(!(cmap = (PanoramiXRes *)SecurityLookupIDByType(
                client, stuff->id, XRT_COLORMAP, SecurityReadAccess)))
        return BadColor;

    FOR_NSCREENS_BACKWARD(j){
	stuff->id = cmap->info[j].id;
	result = (* SavedProcVector[X_InstallColormap])(client);
	if(result != Success) break;
    }
    return (result);
}


int PanoramiXUninstallColormap(ClientPtr client)
{
    REQUEST(xResourceReq);
    int 	result, j;
    PanoramiXRes *cmap;

    REQUEST_SIZE_MATCH(xResourceReq);
 
    client->errorValue = stuff->id;

    if(!(cmap = (PanoramiXRes *)SecurityLookupIDByType(
                client, stuff->id, XRT_COLORMAP, SecurityReadAccess)))
        return BadColor;

    FOR_NSCREENS_BACKWARD(j) {
	stuff->id = cmap->info[j].id;
	result = (* SavedProcVector[X_UninstallColormap])(client);
	if(result != Success) break;
    }
    return (result);
}


int PanoramiXAllocColor(ClientPtr client)
{
    int           result, j;
    PanoramiXRes *cmap;
    REQUEST(xAllocColorReq);

    REQUEST_SIZE_MATCH(xAllocColorReq);

    client->errorValue = stuff->cmap;

    if(!(cmap = (PanoramiXRes *)SecurityLookupIDByType(
                client, stuff->cmap, XRT_COLORMAP, SecurityWriteAccess)))
	return BadColor;

    FOR_NSCREENS_BACKWARD(j){
	stuff->cmap = cmap->info[j].id;
	result = (* SavedProcVector[X_AllocColor])(client);
	if(result != Success) break;
    }
    return (result);
}


int PanoramiXAllocNamedColor(ClientPtr client)
{
    int           result, j;
    PanoramiXRes  *cmap;
    REQUEST(xAllocNamedColorReq);

    REQUEST_FIXED_SIZE(xAllocNamedColorReq, stuff->nbytes);

    client->errorValue = stuff->cmap;

    if(!(cmap = (PanoramiXRes *)SecurityLookupIDByType(
                client, stuff->cmap, XRT_COLORMAP, SecurityWriteAccess)))
        return BadColor;

    FOR_NSCREENS_BACKWARD(j){
        stuff->cmap = cmap->info[j].id;
        result = (* SavedProcVector[X_AllocNamedColor])(client);
	if(result != Success) break;
    }
    return (result);
}


int PanoramiXAllocColorCells(ClientPtr client)
{
    int           result, j;
    PanoramiXRes  *cmap;
    REQUEST(xAllocColorCellsReq);

    REQUEST_SIZE_MATCH(xAllocColorCellsReq);

    client->errorValue = stuff->cmap;

    if(!(cmap = (PanoramiXRes *)SecurityLookupIDByType(
                client, stuff->cmap, XRT_COLORMAP, SecurityWriteAccess)))
	return BadColor;
	
    FOR_NSCREENS_BACKWARD(j){
	stuff->cmap = cmap->info[j].id;
	result = (* SavedProcVector[X_AllocColorCells])(client);
	if(result != Success) break;
    }
    return (result);
}


int PanoramiXAllocColorPlanes(ClientPtr client)
{
    int           result, j;
    PanoramiXRes  *cmap;
    REQUEST(xAllocColorPlanesReq);

    REQUEST_SIZE_MATCH(xAllocColorPlanesReq);

    client->errorValue = stuff->cmap;

    if(!(cmap = (PanoramiXRes *)SecurityLookupIDByType(
                client, stuff->cmap, XRT_COLORMAP, SecurityWriteAccess)))
	return BadColor;
	
    FOR_NSCREENS_BACKWARD(j){
	stuff->cmap = cmap->info[j].id;
	result = (* SavedProcVector[X_AllocColorPlanes])(client);
	if(result != Success) break;
    }
    return (result);
}



int PanoramiXFreeColors(ClientPtr client)
{
    int           result, j;
    PanoramiXRes  *cmap;
    REQUEST(xFreeColorsReq);

    REQUEST_AT_LEAST_SIZE(xFreeColorsReq);

    client->errorValue = stuff->cmap;

    if(!(cmap = (PanoramiXRes *)SecurityLookupIDByType(
                client, stuff->cmap, XRT_COLORMAP, SecurityWriteAccess)))
        return BadColor;

    FOR_NSCREENS_BACKWARD(j) {
        stuff->cmap = cmap->info[j].id;
        result = (* SavedProcVector[X_FreeColors])(client);
	if(result != Success) break;
    }
    return (result);
}


int PanoramiXStoreColors(ClientPtr client)
{
    int           result, j;
    PanoramiXRes  *cmap;
    REQUEST(xStoreColorsReq);

    REQUEST_AT_LEAST_SIZE(xStoreColorsReq);

    client->errorValue = stuff->cmap;

    if(!(cmap = (PanoramiXRes *)SecurityLookupIDByType(
                client, stuff->cmap, XRT_COLORMAP, SecurityWriteAccess)))
        return BadColor;

    FOR_NSCREENS_BACKWARD(j){
	stuff->cmap = cmap->info[j].id;
	result = (* SavedProcVector[X_StoreColors])(client);
	if(result != Success) break;
    }
    return (result);
}


int PanoramiXStoreNamedColor(ClientPtr client)
{
    int           result, j;
    PanoramiXRes  *cmap;
    REQUEST(xStoreNamedColorReq);

    REQUEST_FIXED_SIZE(xStoreNamedColorReq, stuff->nbytes);

    client->errorValue = stuff->cmap;

    if(!(cmap = (PanoramiXRes *)SecurityLookupIDByType(
                client, stuff->cmap, XRT_COLORMAP, SecurityWriteAccess)))
        return BadColor;

    FOR_NSCREENS_BACKWARD(j){
	stuff->cmap = cmap->info[j].id;
	result = (* SavedProcVector[X_StoreNamedColor])(client);
	if(result != Success) break;
    }
    return (result);
}
