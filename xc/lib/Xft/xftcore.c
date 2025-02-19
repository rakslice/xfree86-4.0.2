/*
 * $XFree86: xc/lib/Xft/xftcore.c,v 1.4 2000/12/08 07:51:27 keithp Exp $
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

#include <stdlib.h>
#include "xftint.h"

XChar2b *
XftCoreConvert16 (XftChar16	*string,
		  int		len,
		  XChar2b	xcloc[XFT_CORE_N16LOCAL])
{
    XChar2b *xc;
    int	    i;
    
    if (len < XFT_CORE_N16LOCAL)
	xc = xcloc;
    else
	xc = (XChar2b *) malloc (len * sizeof (XChar2b));
    for (i = 0; i < len; i++)
    {
	xc[i].byte1 = string[i] & 0xff;
	xc[i].byte2 = (string[i] >> 8) & 0xff;
    }
    return xc;
}

XChar2b *
XftCoreConvert32 (XftChar32	    *string,
		  int		    len,
		  XChar2b	    xcloc[XFT_CORE_N16LOCAL])
{
    XChar2b *xc;
    int	    i;
    
    if (len < XFT_CORE_N16LOCAL)
	xc = xcloc;
    else
	xc = (XChar2b *) malloc (len * sizeof (XChar2b));
    for (i = 0; i < len; i++)
    {
	xc[i].byte1 = string[i] & 0xff;
	xc[i].byte2 = (string[i] >> 8) & 0xff;
    }
    return xc;
}

void
XftCoreExtents8 (Display	*dpy,
		 XFontStruct	*fs,
		 XftChar8	*string, 
		 int		len,
		 XGlyphInfo	*extents)
{
    int		direction;
    int		ascent, descent;
    XCharStruct overall;

    XTextExtents (fs, (char *) string, len, &direction,
		  &ascent, &descent, &overall);
    if (overall.lbearing < overall.rbearing)
    {
	extents->x = overall.lbearing;
	extents->width = overall.rbearing - overall.lbearing;
    }
    else
    {
	extents->x = overall.rbearing;
	extents->width = overall.lbearing - overall.rbearing;
    }
    extents->y = -overall.ascent;
    extents->height = overall.ascent + overall.descent;
    extents->xOff = overall.width;
    extents->yOff = 0;
}

void
XftCoreExtents16 (Display	    *dpy,
		  XFontStruct	    *fs,
		  XftChar16	    *string, 
		  int		    len,
		  XGlyphInfo	    *extents)
{
    int		direction;
    int		ascent, descent;
    XCharStruct overall;
    XChar2b	*xc, xcloc[XFT_CORE_N16LOCAL];

    xc = XftCoreConvert16 (string, len, xcloc);
    XTextExtents16 (fs, xc, len, &direction,
		    &ascent, &descent, &overall);
    if (overall.lbearing < overall.rbearing)
    {
	extents->x = overall.lbearing;
	extents->width = overall.rbearing - overall.lbearing;
    }
    else
    {
	extents->x = overall.rbearing;
	extents->width = overall.lbearing - overall.rbearing;
    }
    extents->y = -overall.ascent;
    extents->height = overall.ascent + overall.descent;
    extents->xOff = overall.width;
    extents->yOff = 0;
}

void
XftCoreExtents32 (Display	    *dpy,
		  XFontStruct	    *fs,
		  XftChar32	    *string, 
		  int		    len,
		  XGlyphInfo	    *extents)
{
    int		direction;
    int		ascent, descent;
    XCharStruct overall;
    XChar2b	*xc, xcloc[XFT_CORE_N16LOCAL];

    xc = XftCoreConvert32 (string, len, xcloc);
    XTextExtents16 (fs, xc, len, &direction,
		    &ascent, &descent, &overall);
    if (overall.lbearing < overall.rbearing)
    {
	extents->x = overall.lbearing;
	extents->width = overall.rbearing - overall.lbearing;
    }
    else
    {
	extents->x = overall.rbearing;
	extents->width = overall.lbearing - overall.rbearing;
    }
    extents->y = -overall.ascent;
    extents->height = overall.ascent + overall.descent;
    extents->xOff = overall.width;
    extents->yOff = 0;
}

Bool
XftCoreGlyphExists (Display	    *dpy,
		    XFontStruct	    *fs,
		    XftChar32	    glyph)
{
    int		direction;
    int		ascent, descent;
    XCharStruct overall;
    XChar2b	xc;

    XftCoreConvert32 (&glyph, 1, &xc);
    XTextExtents16 (fs, &xc, 1, &direction,
		    &ascent, &descent, &overall);
    return (overall.lbearing != 0 ||
	    overall.rbearing != 0 ||
	    overall.width != 0 ||
	    overall.ascent != 0 ||
	    overall.descent != 0);
}

