/*
* $TOG: Xtos.h /main/20 1998/02/06 13:28:11 kaleb $
*/

/***********************************************************

Copyright 1987, 1988, 1994, 1998  The Open Group

All Rights Reserved.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from The Open Group.


Copyright 1987, 1988 by Digital Equipment Corporation, Maynard, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the name of Digital not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.  

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/
/* $XFree86: xc/lib/Xt/Xtos.h,v 3.4 2000/08/23 22:10:03 tsi Exp $ */

#ifndef _Xtos_h
#define _Xtos_h

#define ALLOCATE_LOCAL_FALLBACK(_size) XtMalloc((unsigned long)(_size))
#define DEALLOCATE_LOCAL_FALLBACK(_ptr) XtFree((XtPointer)(_ptr))
#include <X11/Xalloca.h>

#ifdef CRAY
#define WORD64
#endif

#if defined(__alpha) || defined(__alpha__) || \
    defined(__ia64__) || defined(ia64)
#define LONG64
#endif

#ifdef __sgi
#if (_MIPS_SZLONG == 64)
#define LONG64
#endif
#endif

#endif /* _Xtos_h */
/* DON'T ADD STUFF AFTER THIS #endif */
