/* $XConsortium: omImText.c,v 1.2 94/01/20 18:08:09 rws Exp $ */
/*
 * Copyright 1992, 1993 by TOSHIBA Corp.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of TOSHIBA not be used in advertising
 * or publicity pertaining to distribution of the software without specific,
 * written prior permission. TOSHIBA make no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * TOSHIBA DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
 * ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
 * TOSHIBA BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
 * ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
 * ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 *
 * Author: Katsuhisa Yano	TOSHIBA Corp.
 *			   	mopi@osa.ilab.toshiba.co.jp
 */
/* $XFree86: xc/lib/X11/omImText.c,v 1.3 2000/11/29 17:40:26 dawes Exp $ */

#include "Xlibint.h"
#include "XomGeneric.h"

extern int _XomGenericTextExtents(), _XomGenericDrawString();

#define GET_VALUE_MASK	(GCFunction | GCForeground | GCBackground | GCFillStyle)
#define SET_VALUE_MASK	(GCFunction | GCForeground | GCFillStyle)

static void
_XomGenericDrawImageString(dpy, d, oc, gc, x, y, type, text, length)
    Display *dpy;
    Drawable d;
    XOC oc;
    GC gc;
    int x, y;
    XOMTextType type;
    XPointer text;
    int length;
{
    XGCValues values;
    XRectangle extent;

    XGetGCValues(dpy, gc, GET_VALUE_MASK, &values);

    XSetFunction(dpy, gc, GXcopy);
    XSetForeground(dpy, gc, values.background);
    XSetFillStyle(dpy, gc, FillSolid);

    _XomGenericTextExtents(oc, type, text, length, 0, &extent);
    XFillRectangle(dpy, d, gc, x + extent.x, y + extent.y, extent.width,
		   extent.height);

    XChangeGC(dpy, gc, SET_VALUE_MASK, &values);

    _XomGenericDrawString(dpy, d, oc, gc, x, y, type, text, length);
}

void
#if NeedFunctionPrototypes
_XmbGenericDrawImageString(Display *dpy, Drawable d, XOC oc, GC gc, int x,
			   int y, _Xconst char *text, int length)
#else
_XmbGenericDrawImageString(dpy, d, oc, gc, x, y, text, length)
    Display *dpy;
    Drawable d;
    XOC oc;
    GC gc;
    int x, y;
    _Xconst char *text;
    int length;
#endif
{
    _XomGenericDrawImageString(dpy, d, oc, gc, x, y, XOMMultiByte,
			       (XPointer) text, length);
}

void
#if NeedFunctionPrototypes
_XwcGenericDrawImageString(Display *dpy, Drawable d, XOC oc, GC gc, int x,
			   int y, _Xconst wchar_t *text, int length)
#else
_XwcGenericDrawImageString(dpy, d, oc, gc, x, y, text, length)
    Display *dpy;
    Drawable d;
    XOC oc;
    GC gc;
    int x, y;
    _Xconst wchar_t *text;
    int length;
#endif
{
    _XomGenericDrawImageString(dpy, d, oc, gc, x, y, XOMWideChar,
			       (XPointer) text, length);
}

void
#if NeedFunctionPrototypes
_Xutf8GenericDrawImageString(Display *dpy, Drawable d, XOC oc, GC gc, int x,
			     int y, _Xconst char *text, int length)
#else
_Xutf8GenericDrawImageString(dpy, d, oc, gc, x, y, text, length)
    Display *dpy;
    Drawable d;
    XOC oc;
    GC gc;
    int x, y;
    _Xconst char *text;
    int length;
#endif
{
    _XomGenericDrawImageString(dpy, d, oc, gc, x, y, XOMUtf8String,
			       (XPointer) text, length);
}
