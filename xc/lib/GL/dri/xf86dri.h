/* $XFree86: xc/lib/GL/dri/xf86dri.h,v 1.7 2000/12/07 20:26:02 dawes Exp $ */
/**************************************************************************

Copyright 1998-1999 Precision Insight, Inc., Cedar Park, Texas.
Copyright 2000 VA Linux Systems, Inc.
All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sub license, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice (including the
next paragraph) shall be included in all copies or substantial portions
of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
IN NO EVENT SHALL PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR
ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/*
 * Authors:
 *   Kevin E. Martin <martin@valinux.com>
 *   Jens Owen <jens@valinux.com>
 *   Rickard E. (Rik) Faith <faith@valinux.com>
 *
 */

#ifndef _XF86DRI_H_
#define _XF86DRI_H_

#include <X11/Xfuncproto.h>
#include <xf86drm.h>

#define X_XF86DRIQueryVersion			0
#define X_XF86DRIQueryDirectRenderingCapable	1
#define X_XF86DRIOpenConnection			2
#define X_XF86DRICloseConnection		3
#define X_XF86DRIGetClientDriverName		4
#define X_XF86DRICreateContext			5
#define X_XF86DRIDestroyContext			6
#define X_XF86DRICreateDrawable			7
#define X_XF86DRIDestroyDrawable		8
#define X_XF86DRIGetDrawableInfo		9
#define X_XF86DRIGetDeviceInfo			10
#define X_XF86DRIAuthConnection                 11
#define X_XF86DRIOpenFullScreen                 12
#define X_XF86DRICloseFullScreen                13

#define XF86DRINumberEvents		0

#define XF86DRIClientNotLocal		0
#define XF86DRIOperationNotSupported	1
#define XF86DRINumberErrors		(XF86DRIOperationNotSupported + 1)

/* Warning : Do not change XF86DRIClipRect without changing the kernel 
 * structure! */
typedef struct _XF86DRIClipRect {
    unsigned short	x1; /* Upper left: inclusive */
    unsigned short	y1;
    unsigned short	x2; /* Lower right: exclusive */
    unsigned short	y2;
} XF86DRIClipRectRec, *XF86DRIClipRectPtr;

#ifndef _XF86DRI_SERVER_

_XFUNCPROTOBEGIN

Bool XF86DRIQueryExtension(
#if NeedFunctionPrototypes
    Display*		/* dpy */,
    int*		/* event_base */,
    int*		/* error_base */
#endif
);

Bool XF86DRIQueryVersion(
#if NeedFunctionPrototypes
    Display*		/* dpy */,
    int*		/* majorVersion */,
    int*		/* minorVersion */,
    int*		/* patchVersion */
#endif
);

Bool XF86DRIQueryDirectRenderingCapable(
#if NeedFunctionPrototypes
    Display*		/* dpy */,
    int			/* screen */,
    Bool*		/* isCapable */
#endif
);

Bool XF86DRIOpenConnection(
#if NeedFunctionPrototypes
    Display*		/* dpy */,
    int			/* screen */,
    drmHandlePtr	/* hSAREA */,
    char**		/* busIDString */
#endif
);

Bool XF86DRIAuthConnection(
#if NeedFunctionPrototypes
    Display*		/* dpy */,
    int			/* screen */,
    drmMagic            /* magic */
#endif
);

Bool XF86DRICloseConnection(
#if NeedFunctionPrototypes
    Display*		/* dpy */,
    int			/* screen */
#endif
);

Bool XF86DRIGetClientDriverName(
#if NeedFunctionPrototypes
    Display*		/* dpy */,
    int			/* screen */,
    int*		/* ddxDriverMajorVersion */,
    int*		/* ddxDriverMinorVersion */,
    int*		/* ddxDriverPatchVersion */,
    char**		/* clientDriverName */
#endif
);

Bool XF86DRICreateContext(
#if NeedFunctionPrototypes
    Display*		/* dpy */,
    int			/* screen */,
    Visual*		/* visual */,
    XID*		/* ptr to returned context id */,
    drmContextPtr	/* hHWContext */
#endif
);

Bool XF86DRIDestroyContext(
#if NeedFunctionPrototypes
    Display*		/* dpy */,
    int			/* screen */,
    XID 	        /* context id */
#endif
);

Bool XF86DRICreateDrawable(
#if NeedFunctionPrototypes
    Display*		/* dpy */,
    int			/* screen */,
    Drawable		/* drawable */,
    drmDrawablePtr 	/* hHWDrawable */
#endif
);

Bool XF86DRIDestroyDrawable(
#if NeedFunctionPrototypes
    Display*		/* dpy */,
    int			/* screen */,
    Drawable 		/* drawable */
#endif
);

Bool XF86DRIGetDrawableInfo(
#if NeedFunctionPrototypes
    Display*		/* dpy */,
    int			/* screen */,
    Drawable 		/* drawable */,
    unsigned int*	/* index */,
    unsigned int*	/* stamp */,
    int*		/* X */,
    int*		/* Y */,
    int*		/* W */,
    int*		/* H */,
    int*		/* numClipRects */,
    XF86DRIClipRectPtr*,/* pClipRects */
    int*		/* backX */,
    int*		/* backY */,
    int*		/* numBackClipRects */,
    XF86DRIClipRectPtr*	/* pBackClipRects */    
#endif
);

Bool XF86DRIGetDeviceInfo(
#if NeedFunctionPrototypes
    Display*		/* dpy */,
    int			/* screen */,
    drmHandlePtr	/* hFrameBuffer */,
    int*		/* fbOrigin */,
    int*		/* fbSize */,
    int*		/* fbStride */,
    int*		/* devPrivateSize */,
    void**		/* pDevPrivate */
#endif
);

Bool XF86DRIOpenFullScreen(
#if NeedFunctionPrototypes
    Display*		/* dpy */,
    int                 /* screen */,
    Drawable 		/* drawable */
#endif
);

Bool XF86DRICloseFullScreen(
#if NeedFunctionPrototypes
    Display*		/* dpy */,
    int                 /* screen */,
    Drawable 		/* drawable */
#endif
);

_XFUNCPROTOEND

#endif /* _XF86DRI_SERVER_ */

#endif /* _XF86DRI_H_ */

