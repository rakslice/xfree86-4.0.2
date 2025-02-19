/* $XFree86: xc/lib/GL/mesa/src/drv/r128/r128_dd.c,v 1.6 2000/12/15 22:48:38 dawes Exp $ */
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
#include "r128_state.h"
#include "r128_vb.h"
#include "r128_pipeline.h"
#include "r128_dd.h"

#include "extensions.h"
#if defined(USE_X86_ASM) || defined(USE_3DNOW_ASM) || defined(USE_KATMAI_ASM)
#include "X86/common_x86_asm.h"
#endif

#define R128_DATE	"20001215"

/* Return the current color buffer size */
static void r128DDGetBufferSize( GLcontext *ctx,
				 GLuint *width, GLuint *height )
{
   r128ContextPtr r128ctx = R128_CONTEXT( ctx );

   *width  = r128ctx->driDrawable->w;
   *height = r128ctx->driDrawable->h;
}

/* Return various strings for glGetString() */
static const GLubyte *r128DDGetString( GLcontext *ctx, GLenum name )
{
   r128ContextPtr r128ctx = R128_CONTEXT( ctx );
   static GLubyte buffer[128];

   switch ( name ) {
   case GL_VENDOR:
      return (const GLubyte *)"VA Linux Systems, Inc.";

   case GL_RENDERER:
      sprintf((void *)buffer, "Mesa DRI Rage128 " R128_DATE );

      /* Append any chipset-specific information.
       */
      if ( R128_IS_PRO( r128ctx ) ) {
	 strncat( buffer, " Pro", 4 );
      }
      if ( R128_IS_MOBILITY( r128ctx ) ) {
	 strncat( buffer, " M3", 3 );
      }

      /* Append any AGP-specific information.
       */
      switch ( r128ctx->r128Screen->AGPMode ) {
      case 1:
	 strncat( buffer, " AGP 1x", 7 );
	 break;
      case 2:
	 strncat( buffer, " AGP 2x", 7 );
	 break;
      case 4:
	 strncat( buffer, " AGP 4x", 7 );
	 break;
      }

      /* Append any CPU-specific information.
       */
#ifdef USE_X86_ASM
      if ( gl_x86_cpu_features ) {
	 strncat( buffer, " x86", 4 );
      }
#endif
#ifdef USE_3DNOW_ASM
      if ( cpu_has_3dnow ) {
	 strncat( buffer, "/3DNow!", 7 );
      }
#endif
#ifdef USE_KATMAI_ASM
      if ( cpu_has_xmm ) {
	 strncat( buffer, "/SSE", 4 );
      }
#endif
      return buffer;

   default:
      return NULL;
   }
}

/* Send all commands to the hardware.  If vertex buffers or indirect
 * buffers are in use, then we need to make sure they are sent to the
 * hardware.  All commands that are normally sent to the ring are
 * already considered `flushed'.
 */
static void r128DDFlush( GLcontext *ctx )
{
   r128ContextPtr r128ctx = R128_CONTEXT( ctx );

   FLUSH_BATCH( r128ctx );

#if ENABLE_PERF_BOXES
   if ( r128ctx->boxes ) {
      LOCK_HARDWARE( r128ctx );
      r128PerformanceBoxesLocked( r128ctx );
      UNLOCK_HARDWARE( r128ctx );
   }

   /* Log the performance counters if necessary */
   r128PerformanceCounters( r128ctx );
#endif
}

/* Make sure all commands have been sent to the hardware and have
 * completed processing.
 */
static void r128DDFinish( GLcontext *ctx )
{
   r128ContextPtr r128ctx = R128_CONTEXT( ctx );

#if ENABLE_PERF_BOXES
   /* Bump the performance counter */
   r128ctx->c_drawWaits++;
#endif

   r128DDFlush( ctx );
   r128WaitForIdle( r128ctx );
}

/* Return various parameters requested by Mesa (this is deprecated) */
static GLint r128DDGetParameteri( const GLcontext *ctx, GLint param )
{
   switch (param) {
   case DD_HAVE_HARDWARE_FOG:
      return 1;
   default:
      return 0;
   }
}

/* Initialize the extensions supported by this driver */
void r128DDInitExtensions( GLcontext *ctx )
{
   /* FIXME: Are there other extensions to enable/disable??? */
   gl_extensions_disable( ctx, "GL_EXT_shared_texture_palette" );
   gl_extensions_disable( ctx, "GL_EXT_paletted_texture" );
   gl_extensions_disable( ctx, "GL_EXT_point_parameters" );
   gl_extensions_disable( ctx, "ARB_imaging" );
   gl_extensions_disable( ctx, "GL_EXT_blend_minmax" );
   gl_extensions_disable( ctx, "GL_EXT_blend_logic_op" );
   gl_extensions_disable( ctx, "GL_EXT_blend_subtract" );
   gl_extensions_disable( ctx, "GL_INGR_blend_func_separate" );

   if ( getenv( "LIBGL_NO_MULTITEXTURE" ) )
      gl_extensions_disable( ctx, "GL_ARB_multitexture" );

   gl_extensions_disable( ctx, "GL_SGI_color_matrix" );
   gl_extensions_disable( ctx, "GL_SGI_color_table" );
   gl_extensions_disable( ctx, "GL_SGIX_pixel_texture" );
   gl_extensions_disable( ctx, "GL_ARB_texture_cube_map" );
   gl_extensions_disable( ctx, "GL_ARB_texture_compression" );
   gl_extensions_disable( ctx, "GL_EXT_convolution" );
}

/* Initialize the driver's misc functions */
void r128DDInitDriverFuncs( GLcontext *ctx )
{
    ctx->Driver.GetBufferSize		= r128DDGetBufferSize;
    ctx->Driver.GetString		= r128DDGetString;
    ctx->Driver.Finish			= r128DDFinish;
    ctx->Driver.Flush			= r128DDFlush;

    ctx->Driver.Error			= NULL;
    ctx->Driver.GetParameteri		= r128DDGetParameteri;

    ctx->Driver.DrawPixels		= NULL;
    ctx->Driver.Bitmap			= NULL;

    ctx->Driver.RegisterVB		= r128DDRegisterVB;
    ctx->Driver.UnregisterVB		= r128DDUnregisterVB;
    ctx->Driver.BuildPrecalcPipeline	= r128DDBuildPrecalcPipeline;
}
