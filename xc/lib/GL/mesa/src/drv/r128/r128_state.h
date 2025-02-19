/* $XFree86: xc/lib/GL/mesa/src/drv/r128/r128_state.h,v 1.2 2000/12/04 19:21:47 dawes Exp $ */
/**************************************************************************

Copyright 1999, 2000 ATI Technologies Inc. and Precision Insight, Inc.,
                                               Cedar Park, Texas.
All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
on the rights to use, copy, modify, merge, publish, distribute, sub
license, and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice (including the next
paragraph) shall be included in all copies or substantial portions of the
Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
ATI, PRECISION INSIGHT AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/*
 * Authors:
 *   Kevin E. Martin <martin@valinux.com>
 *   Gareth Hughes <gareth@valinux.com>
 *
 */

#ifndef _R128_STATE_H_
#define _R128_STATE_H_

#ifdef GLX_DIRECT_RENDERING

#include "r128_context.h"

extern void r128DDInitState( r128ContextPtr r128ctx );
extern void r128DDInitStateFuncs( GLcontext *ctx );

extern void r128DDUpdateState( GLcontext *ctx );
extern void r128DDUpdateHWState( GLcontext *ctx );

extern void r128UpdateWindow( GLcontext *ctx );
extern void r128SetClipRects( r128ContextPtr r128ctx,
			      XF86DRIClipRectPtr pc, int nc );

extern void r128EmitHwStateLocked( r128ContextPtr r128ctx );

#endif
#endif /* _R128_STATE_H_ */
