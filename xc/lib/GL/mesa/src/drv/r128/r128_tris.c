/* $XFree86: xc/lib/GL/mesa/src/drv/r128/r128_tris.c,v 1.3 2000/12/04 19:21:47 dawes Exp $ */ /* -*- c-basic-offset: 3 -*- */
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

#include "r128_context.h"
#include "r128_ioctl.h"
#include "r128_vb.h"
#include "r128_tris.h"
#include "r128_state.h"

#include "pipeline.h"
#include "vbindirect.h"

static struct {
   points_func		points;
   line_func		line;
   triangle_func	triangle;
   quad_func		quad;
} rast_tab[R128_MAX_TRIFUNC];

#define R128_COLOR(to, from)                                                \
do {                                                                        \
   (to)[0] = (from)[2];                                                    \
   (to)[1] = (from)[1];                                                    \
   (to)[2] = (from)[0];                                                    \
   (to)[3] = (from)[3];                                                    \
} while (0)


static void r128_null_quad( GLcontext *ctx, GLuint v0,
			    GLuint v1, GLuint v2, GLuint v3, GLuint pv ) {
}
static void r128_null_triangle( GLcontext *ctx, GLuint v0,
				GLuint v1, GLuint v2, GLuint pv ) {
}
static void r128_null_line( GLcontext *ctx, GLuint v1, GLuint v2, GLuint pv ) {
}

static void r128_null_points( GLcontext *ctx, GLuint first, GLuint last ) {
}

static void r128PrintRenderState( const char *msg, GLuint state )
{
   fprintf( stderr, "%s: (0x%x) %s%s%s%s%s\n",
	    msg, state,
	    (state & R128_FLAT_BIT)       ? "flat, "       : "",
	    (state & R128_OFFSET_BIT)     ? "offset, "     : "",
	    (state & R128_TWOSIDE_BIT)    ? "twoside, "    : "",
	    (state & R128_NODRAW_BIT)     ? "no-draw, "    : "",
	    (state & R128_FALLBACK_BIT)   ? "fallback"     : "" );
}

#define IND (0)
#define TAG(x) x
#include "r128_tritmp.h"

#define IND (R128_FLAT_BIT)
#define TAG(x) x##_flat
#include "r128_tritmp.h"

#define IND (R128_OFFSET_BIT)
#define TAG(x) x##_offset
#include "r128_tritmp.h"

#define IND (R128_OFFSET_BIT | R128_FLAT_BIT)
#define TAG(x) x##_offset_flat
#include "r128_tritmp.h"

#define IND (R128_TWOSIDE_BIT)
#define TAG(x) x##_twoside
#include "r128_tritmp.h"

#define IND (R128_TWOSIDE_BIT | R128_FLAT_BIT)
#define TAG(x) x##_twoside_flat
#include "r128_tritmp.h"

#define IND (R128_TWOSIDE_BIT | R128_OFFSET_BIT)
#define TAG(x) x##_twoside_offset
#include "r128_tritmp.h"

#define IND (R128_TWOSIDE_BIT | R128_OFFSET_BIT | R128_FLAT_BIT)
#define TAG(x) x##_twoside_offset_flat
#include "r128_tritmp.h"


/* Initialize the table of points, line and triangle drawing functions */
void r128DDTriangleFuncsInit( void )
{
   int i;

   init();
   init_flat();
   init_offset();
   init_offset_flat();
   init_twoside();
   init_twoside_flat();
   init_twoside_offset();
   init_twoside_offset_flat();

   for ( i = 0 ; i < 0x20 ; i++ ) {
      if ( (i & (R128_NODRAW_BIT | R128_FALLBACK_BIT)) == R128_NODRAW_BIT ) {
	 rast_tab[i].points   = r128_null_points;
	 rast_tab[i].line     = r128_null_line;
	 rast_tab[i].triangle = r128_null_triangle;
	 rast_tab[i].quad     = r128_null_quad;
      }
   }
}


/* FIXME: Only enable software fallback for stencil in 16 bpp mode after
 * we have hardware stencil support.
 */
#define ALL_FALLBACK	(DD_MULTIDRAW | DD_SELECT | DD_FEEDBACK | DD_STENCIL)
#define POINT_FALLBACK	(ALL_FALLBACK | DD_POINT_SMOOTH | DD_POINT_ATTEN)
#define LINE_FALLBACK	(ALL_FALLBACK | DD_LINE_SMOOTH | DD_LINE_STIPPLE)
#define TRI_FALLBACK	(ALL_FALLBACK | DD_TRI_SMOOTH | DD_TRI_STIPPLE | DD_TRI_UNFILLED)
#define ANY_FALLBACK	(POINT_FALLBACK | LINE_FALLBACK | TRI_FALLBACK)
#define ANY_RASTER_FLAGS (/*DD_FLATSHADE |*/ DD_TRI_LIGHT_TWOSIDE | DD_TRI_OFFSET | DD_Z_NEVER)

/* Setup the Point, Line, Triangle and Quad functions based on the
 * current rendering state.  Wherever possible, use the hardware to
 * render the primitive.  Otherwise, fallback to software rendering.
 */
void r128DDChooseRenderState( GLcontext *ctx )
{
   r128ContextPtr r128ctx = R128_CONTEXT( ctx );
   GLuint flags = ctx->TriangleCaps;
   CARD32 index = 0;

   if ( r128ctx->Fallback ) {
      r128ctx->RenderIndex = R128_FALLBACK_BIT;
      return;
   }

   if ( flags & ANY_RASTER_FLAGS ) {
      if ( flags & DD_FLATSHADE )		index |= R128_FLAT_BIT;
      if ( flags & DD_TRI_LIGHT_TWOSIDE )	index |= R128_TWOSIDE_BIT;
      if ( flags & DD_TRI_OFFSET )		index |= R128_OFFSET_BIT;
      if ( flags & DD_Z_NEVER )			index |= R128_NODRAW_BIT;
   }

   r128ctx->PointsFunc = rast_tab[index].points;
   r128ctx->LineFunc = rast_tab[index].line;
   r128ctx->TriangleFunc = rast_tab[index].triangle;
   r128ctx->QuadFunc = rast_tab[index].quad;

   r128ctx->RenderIndex = index;
   r128ctx->IndirectTriangles = 0;

   if ( flags & ANY_FALLBACK ) {
      r128ctx->RenderIndex |= R128_FALLBACK_BIT;

      if ( flags & POINT_FALLBACK ) {
	 r128ctx->PointsFunc = 0;
	 r128ctx->IndirectTriangles |= DD_POINT_SW_RASTERIZE;
      }

      if ( flags & LINE_FALLBACK ) {
	 r128ctx->LineFunc = 0;
	 r128ctx->IndirectTriangles |= DD_LINE_SW_RASTERIZE;
      }

      if ( flags & TRI_FALLBACK ) {
	 r128ctx->TriangleFunc = 0;
	 r128ctx->QuadFunc = 0;
	 r128ctx->IndirectTriangles |= (DD_TRI_SW_RASTERIZE |
					DD_QUAD_SW_RASTERIZE);
      }
   }

   if ( 0 ) {
      gl_print_tri_caps( "tricaps", ctx->TriangleCaps );
      r128PrintRenderState( "r128 render state", r128ctx->RenderIndex );
   }
}
