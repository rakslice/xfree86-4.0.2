/*
 * $XFree86: xc/lib/Xft/xftextent.c,v 1.4 2000/12/15 17:12:52 keithp Exp $
 *
 * Copyright � 2000 Keith Packard, member of The XFree86 Project, Inc.
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

#include "xftint.h"

void
XftTextExtents8 (Display	*dpy,
		 XftFont	*font,
		 XftChar8	*string, 
		 int		len,
		 XGlyphInfo	*extents)
{
    if (font->core)
    {
	XftCoreExtents8 (dpy, font->u.core.font, string, len, extents);
    }
#ifdef FREETYPE2
    else
    {
	XftRenderExtents8 (dpy, font->u.ft.font, string, len, extents);
    }
#endif
}

void
XftTextExtents16 (Display	    *dpy,
		  XftFont	    *font,
		  XftChar16	    *string, 
		  int		    len,
		  XGlyphInfo	    *extents)
{
    if (font->core)
    {
	XftCoreExtents16 (dpy, font->u.core.font, string, len, extents);
    }
#ifdef FREETYPE2
    else
    {
	XftRenderExtents16 (dpy, font->u.ft.font, string, len, extents);
    }
#endif
}

void
XftTextExtents32 (Display	*dpy,
		  XftFont	*font,
		  XftChar32	*string, 
		  int		len,
		  XGlyphInfo	*extents)
{
    if (font->core)
    {
	XftCoreExtents32 (dpy, font->u.core.font, string, len, extents);
    }
#ifdef FREETYPE2
    else
    {
	XftRenderExtents32 (dpy, font->u.ft.font, string, len, extents);
    }
#endif
}
