/* $XConsortium: Display.h,v 1.2 94/02/06 17:51:43 rws Exp $ */
/*

Copyright 1993 by Davor Matic

Permission to use, copy, modify, distribute, and sell this software
and its documentation for any purpose is hereby granted without fee,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation.  Davor Matic makes no representations about
the suitability of this software for any purpose.  It is provided "as
is" without express or implied warranty.

*/
/* $XFree86: xc/programs/Xserver/hw/xnest/Display.h,v 1.5 2000/08/01 20:05:43 dawes Exp $ */

#ifndef XNESTCOMMON_H
#define XNESTCOMMON_H

#define UNDEFINED -1

#define MAXDEPTH 32
#define MAXVISUALSPERDEPTH 256

extern Display *xnestDisplay;
extern XVisualInfo *xnestVisuals;       
extern int xnestNumVisuals;
extern int xnestDefaultVisualIndex;
extern Colormap *xnestDefaultColormaps;
extern int xnestNumDefaultClormaps;
extern int *xnestDepths;
extern int xnestNumDepths;
extern XPixmapFormatValues *xnestPixmapFormats;
extern int xnestNumPixmapFormats;
extern Pixel xnestBlackPixel;             
extern Pixel xnestWhitePixel;             
extern Drawable xnestDefaultDrawables[MAXDEPTH + 1];
extern Pixmap xnestIconBitmap;
extern Pixmap xnestScreenSaverPixmap;
extern XlibGC xnestBitmapGC;
extern Window xnestConfineWindow;
extern unsigned long xnestEventMask;

void xnestOpenDisplay();
void xnestCloseDisplay();

#endif /* XNESTCOMMON_H */
