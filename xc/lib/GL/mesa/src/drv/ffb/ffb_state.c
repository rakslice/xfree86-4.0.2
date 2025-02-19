/* $XFree86: xc/lib/GL/mesa/src/drv/ffb/ffb_state.c,v 1.1 2000/06/20 05:08:39 dawes Exp $
 *
 * GLX Hardware Device Driver for Sun Creator/Creator3D
 * Copyright (C) 2000 David S. Miller
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * DAVID MILLER, OR ANY OTHER CONTRIBUTORS BE LIABLE FOR ANY CLAIM, 
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR 
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE 
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *
 *    David S. Miller <davem@redhat.com>
 */

#include "types.h"
#include "vbrender.h"

#include <stdio.h>
#include <stdlib.h>

#include "mm.h"
#include "ffb_dd.h"
#include "ffb_span.h"
#include "ffb_depth.h"
#include "ffb_context.h"
#include "ffb_vb.h"
#include "ffb_tris.h"
#include "ffb_lines.h"
#include "ffb_points.h"
#include "ffb_state.h"
#include "ffb_lock.h"
#include "extensions.h"
#include "vb.h"
#include "dd.h"
#include "enums.h"
#include "pipeline.h"
#include "pb.h"

#undef STATE_TRACE

static unsigned int ffbComputeAlphaFunc(GLcontext *ctx)
{
	unsigned int xclip;

#ifdef STATE_TRACE
	fprintf(stderr, "ffbDDAlphaFunc: func(%s) ref(%02x)\n",
		gl_lookup_enum_by_nr(ctx->Color.AlphaFunc),
		ctx->Color.AlphaRef & 0xff);
#endif

	switch (ctx->Color.AlphaFunc) {
	case GL_NEVER: xclip = FFB_XCLIP_TEST_NEVER; break;
	case GL_LESS: xclip = FFB_XCLIP_TEST_LT; break;
	case GL_EQUAL: xclip = FFB_XCLIP_TEST_EQ; break;
	case GL_LEQUAL: xclip = FFB_XCLIP_TEST_LE; break;
	case GL_GREATER: xclip = FFB_XCLIP_TEST_GT; break;
	case GL_NOTEQUAL: xclip = FFB_XCLIP_TEST_NE; break;
	case GL_GEQUAL: xclip = FFB_XCLIP_TEST_GE; break;
	case GL_ALWAYS: xclip = FFB_XCLIP_TEST_ALWAYS; break;

	default:
		return FFB_XCLIP_TEST_ALWAYS | 0x00;
	}

	xclip |= (ctx->Color.AlphaRef & 0xff);

	return xclip;
}

static void ffbDDAlphaFunc(GLcontext *ctx, GLenum func, GLclampf ref)
{
	ffbContextPtr fmesa = FFB_CONTEXT(ctx);

	if (ctx->Color.AlphaEnabled) {
		unsigned int xclip = ffbComputeAlphaFunc(ctx);

		if (fmesa->xclip != xclip) {
			fmesa->xclip = xclip;
			fmesa->state_dirty |= FFB_STATE_XCLIP;
			fmesa->state_fifo_ents += 1;
		}
	}
}

static void ffbDDBlendEquation(GLcontext *ctx, GLenum mode)
{
	ffbContextPtr fmesa = FFB_CONTEXT(ctx);

#ifdef STATE_TRACE
	fprintf(stderr, "ffbDDBlendEquation: mode(%s)\n", gl_lookup_enum_by_nr(mode));
#endif
	if (mode != GL_FUNC_ADD_EXT)
		fmesa->bad_fragment_attrs |= FFB_BADATTR_BLENDEQN;
	else
		fmesa->bad_fragment_attrs &= ~FFB_BADATTR_BLENDEQN;
}

static void ffbDDBlendFunc(GLcontext *ctx, GLenum sfactor, GLenum dfactor)
{
	ffbContextPtr fmesa = FFB_CONTEXT(ctx);
	unsigned int blendc = 1 << 4;

#ifdef STATE_TRACE
	fprintf(stderr, "ffbDDBlendFunc: sfactor(%s) dfactor(%s)\n",
		gl_lookup_enum_by_nr(sfactor), gl_lookup_enum_by_nr(dfactor));
#endif
	switch (ctx->Color.BlendSrcRGB) {
	case GL_ZERO:
		blendc |= (0 << 0);
		break;

	case GL_ONE:
		blendc |= (1 << 0);
		break;

	case GL_ONE_MINUS_SRC_ALPHA:
		blendc |= (2 << 0);
		break;

	case GL_SRC_ALPHA:
		blendc |= (3 << 0);
		break;

	default:
		if (ctx->Color.BlendEnabled)
			fmesa->bad_fragment_attrs |= FFB_BADATTR_BLENDFUNC;
		return;
	};

	switch (ctx->Color.BlendDstRGB) {
	case GL_ZERO:
		blendc |= (0 << 2);
		break;

	case GL_ONE:
		blendc |= (1 << 2);
		break;

	case GL_ONE_MINUS_SRC_ALPHA:
		blendc |= (2 << 2);
		break;

	case GL_SRC_ALPHA:
		blendc |= (3 << 2);
		break;

	default:
		if (ctx->Color.BlendEnabled)
			fmesa->bad_fragment_attrs |= FFB_BADATTR_BLENDFUNC;
		return;
	};

	if (ctx->Color.BlendEnabled &&
	    ctx->Color.ColorLogicOpEnabled &&
	    ctx->Color.LogicOp != GL_COPY) {
		/* We could avoid this if sfactor is GL_ONE and
		 * dfactor is GL_ZERO.  I do not think that is even
		 * worthwhile to check because if someone is using
		 * blending they use more interesting settings and
		 * also it would add more state tracking to a lot
		 * of the code in this file.
		 */
		fmesa->bad_fragment_attrs |= FFB_BADATTR_BLENDROP;
		return;
	}

	fmesa->bad_fragment_attrs &= ~(FFB_BADATTR_BLENDFUNC |
				       FFB_BADATTR_BLENDROP);

	if (blendc != fmesa->blendc) {
		fmesa->blendc = blendc;
		fmesa->state_dirty |= FFB_STATE_BLEND;
		fmesa->state_fifo_ents += 1;
	}
}

static void ffbDDBlendFuncSeparate(GLcontext *ctx, GLenum sfactorRGB,
				   GLenum dfactorRGB, GLenum sfactorA,
				   GLenum dfactorA)
{
#ifdef STATE_TRACE
	fprintf(stderr, "ffbDDBlendFuncSeparate: sRGB(%s) dRGB(%s) sA(%s) dA(%s)\n",
		gl_lookup_enum_by_nr(sfactorRGB),
		gl_lookup_enum_by_nr(dfactorRGB),
		gl_lookup_enum_by_nr(sfactorA),
		gl_lookup_enum_by_nr(dfactorA));
#endif

	ffbDDBlendFunc(ctx, sfactorRGB, dfactorRGB);
}

static void ffbDDDepthFunc(GLcontext *ctx, GLenum func)
{
	ffbContextPtr fmesa = FFB_CONTEXT(ctx);
	GLuint cmp;

#ifdef STATE_TRACE
	fprintf(stderr, "ffbDDDepthFunc: func(%s)\n",
		gl_lookup_enum_by_nr(func));
#endif

	switch (func) {
	case GL_NEVER:
		cmp = FFB_CMP_MAGN_NEVER;
		break;
	case GL_ALWAYS:
		cmp = FFB_CMP_MAGN_ALWAYS;
		break;
	case GL_LESS:
		cmp = FFB_CMP_MAGN_LT;
		break;
	case GL_LEQUAL:
		cmp = FFB_CMP_MAGN_LE;
		break;
	case GL_EQUAL:
		cmp = FFB_CMP_MAGN_EQ;
		break;
	case GL_GREATER:
		cmp = FFB_CMP_MAGN_GT;
		break;
	case GL_GEQUAL:
		cmp = FFB_CMP_MAGN_GE;
		break;
	case GL_NOTEQUAL:
		cmp = FFB_CMP_MAGN_NE;
		break;
	default:
		return;
	};

	if (! ctx->Depth.Test)
		cmp = FFB_CMP_MAGN_ALWAYS;

	cmp <<= 16;
	cmp = (fmesa->cmp & ~(0xff<<16)) | cmp;
	if (cmp != fmesa->cmp) {
		fmesa->cmp = cmp;
		fmesa->state_dirty |= FFB_STATE_CMP;
		fmesa->state_fifo_ents += 1;
	}
}

static void ffbDDDepthMask(GLcontext *ctx, GLboolean flag)
{
	ffbContextPtr fmesa = FFB_CONTEXT(ctx);
	GLuint fbc = fmesa->fbc;
	GLboolean enabled_now;

#ifdef STATE_TRACE
	fprintf(stderr, "ffbDDDepthMask: flag(%d)\n", flag);
#endif

	if ((fbc & FFB_FBC_ZE_MASK) == FFB_FBC_ZE_OFF)
		enabled_now = GL_FALSE;
	else
		enabled_now = GL_TRUE;

	if (flag != enabled_now) {
		fbc &= ~FFB_FBC_ZE_MASK;
		if (flag) {
			fbc |= FFB_FBC_WB_C | FFB_FBC_ZE_ON;
		} else {
			fbc |= FFB_FBC_ZE_OFF;
			fbc &= ~FFB_FBC_WB_C;
		}
		fmesa->fbc = fbc;
		fmesa->state_dirty |= FFB_STATE_FBC;
		fmesa->state_fifo_ents += 1;
	}
}

static void ffbDDStencilFunc(GLcontext *ctx, GLenum func, GLint ref, GLuint mask)
{
	ffbContextPtr fmesa = FFB_CONTEXT(ctx);
	unsigned int stencil, stencilctl, consty;

	/* We will properly update sw/hw state when stenciling is
	 * enabled.
	 */
	if (! ctx->Stencil.Enabled)
		return;

	stencilctl = fmesa->stencilctl;
	stencilctl &= ~(7 << 16);

	switch (func) {
	case GL_ALWAYS:		stencilctl |= (0 << 16); break;
	case GL_GREATER:	stencilctl |= (1 << 16); break;
	case GL_EQUAL:		stencilctl |= (2 << 16); break;
	case GL_GEQUAL:		stencilctl |= (3 << 16); break;
	case GL_NEVER:		stencilctl |= (4 << 16); break;
	case GL_LEQUAL:		stencilctl |= (5 << 16); break;
	case GL_NOTEQUAL:	stencilctl |= (6 << 16); break;
	case GL_LESS:		stencilctl |= (7 << 16); break;

	default:
		return;
	};

	consty = ref & 0xf;

	stencil = fmesa->stencil;
	stencil &= ~(0xf << 20);
	stencil |= (mask & 0xf) << 20;

	if (fmesa->stencil != stencil ||
	    fmesa->stencilctl != stencilctl ||
	    fmesa->consty != consty) {
		fmesa->stencil = stencil;
		fmesa->stencilctl = stencilctl;
		fmesa->consty = consty;
		fmesa->state_dirty |= FFB_STATE_STENCIL;
		fmesa->state_fifo_ents += 6;
	}
}

static void ffbDDStencilMask(GLcontext *ctx, GLuint mask)
{
	ffbContextPtr fmesa = FFB_CONTEXT(ctx);

	mask &= 0xf;
	if (fmesa->ypmask != mask) {
		fmesa->ypmask = mask;
		fmesa->state_dirty |= FFB_STATE_YPMASK;
		fmesa->state_fifo_ents += 1;
	}
}

static void ffbDDStencilOp(GLcontext *ctx, GLenum fail, GLenum zfail, GLenum zpass)
{
	ffbContextPtr fmesa = FFB_CONTEXT(ctx);
	unsigned int stencilctl;

	/* We will properly update sw/hw state when stenciling is
	 * enabled.
	 */
	if (! ctx->Stencil.Enabled)
		return;

	stencilctl = fmesa->stencilctl;
	stencilctl &= ~(0xfff00000);

	switch (fail) {
	case GL_ZERO:		stencilctl |= (0 << 28); break;
	case GL_KEEP:		stencilctl |= (1 << 28); break;
	case GL_INVERT:		stencilctl |= (2 << 28); break;
	case GL_REPLACE:	stencilctl |= (3 << 28); break;
	case GL_INCR:		stencilctl |= (4 << 28); break;
	case GL_DECR:		stencilctl |= (5 << 28); break;

	default:
		return;
	};

	switch (zfail) {
	case GL_ZERO:		stencilctl |= (0 << 24); break;
	case GL_KEEP:		stencilctl |= (1 << 24); break;
	case GL_INVERT:		stencilctl |= (2 << 24); break;
	case GL_REPLACE:	stencilctl |= (3 << 24); break;
	case GL_INCR:		stencilctl |= (4 << 24); break;
	case GL_DECR:		stencilctl |= (5 << 24); break;

	default:
		return;
	};

	switch (zpass) {
	case GL_ZERO:		stencilctl |= (0 << 20); break;
	case GL_KEEP:		stencilctl |= (1 << 20); break;
	case GL_INVERT:		stencilctl |= (2 << 20); break;
	case GL_REPLACE:	stencilctl |= (3 << 20); break;
	case GL_INCR:		stencilctl |= (4 << 20); break;
	case GL_DECR:		stencilctl |= (5 << 20); break;

	default:
		return;
	};

	if (fmesa->stencilctl != stencilctl) {
		fmesa->stencilctl = stencilctl;
		fmesa->state_dirty |= FFB_STATE_STENCIL;
		fmesa->state_fifo_ents += 6;
	}
}

void ffbDDScissor(GLcontext *ctx, GLint cx, GLint cy,
		  GLsizei cw, GLsizei ch)
{
	ffbContextPtr fmesa = FFB_CONTEXT(ctx);
	__DRIdrawablePrivate *dPriv = fmesa->driDrawable;
	GLint x, y;
	unsigned int vcmin, vcmax;

#ifdef STATE_TRACE
	fprintf(stderr, "ffbDDScissor: x(%x) y(%x) w(%x) h(%x)\n",
		cx, cy, cw, ch);
#endif
	x = cx + dPriv->x;
	y = dPriv->y + (cy - ch);
	vcmin = ((y & 0xffff) << 16) | (x & 0xffff);
	vcmax = ((((y + ch) & 0xffff) << 16) |
		 (((x + cw) & 0xffff)));
	if (fmesa->vclipmin != vcmin ||
	    fmesa->vclipmax != vcmax) {
		fmesa->vclipmin = vcmin;
		fmesa->vclipmax = vcmax;
		fmesa->state_dirty |= FFB_STATE_CLIP;
		fmesa->state_fifo_ents += 4 + (4 * 2);
	}
}

static GLboolean ffbDDSetDrawBuffer(GLcontext *ctx, GLenum buffer)
{
	ffbContextPtr fmesa = FFB_CONTEXT(ctx);
	unsigned int fbc = fmesa->fbc;

#ifdef STATE_TRACE
	fprintf(stderr, "ffbDDSetDrawBuffer: mode(%s)\n",
		gl_lookup_enum_by_nr(buffer));
#endif
	fbc &= ~(FFB_FBC_WB_AB);
	switch (buffer) {
	case GL_FRONT_LEFT:
		if (fmesa->back_buffer == 0)
			fbc |= FFB_FBC_WB_B;
		else
			fbc |= FFB_FBC_WB_A;
		break;

	case GL_BACK_LEFT:
		if (fmesa->back_buffer == 0)
			fbc |= FFB_FBC_WB_A;
		else
			fbc |= FFB_FBC_WB_B;
		break;

	case GL_FRONT_AND_BACK:
		fbc |= FFB_FBC_WB_AB;
		break;

	default:
		return GL_FALSE;
	};

	if (fbc != fmesa->fbc) {
		fmesa->fbc = fbc;
		fmesa->state_dirty |= FFB_STATE_FBC;
		fmesa->state_fifo_ents += 1;
	}

	return GL_TRUE;
}

static void ffbDDSetReadBuffer(GLcontext *ctx, GLframebuffer *colorBuffer,
			       GLenum buffer)
{
	ffbContextPtr fmesa = FFB_CONTEXT(ctx);
	unsigned int fbc = fmesa->fbc;

#ifdef STATE_TRACE
	fprintf(stderr, "ffbDDSetReadBuffer: mode(%s)\n",
		gl_lookup_enum_by_nr(buffer));
#endif
	fbc &= ~(FFB_FBC_RB_MASK);
	switch (buffer) {
	case GL_FRONT_LEFT:
		if (fmesa->back_buffer == 0)
			fbc |= FFB_FBC_RB_B;
		else
			fbc |= FFB_FBC_RB_A;
		break;

	case GL_BACK_LEFT:
		if (fmesa->back_buffer == 0)
			fbc |= FFB_FBC_RB_A;
		else
			fbc |= FFB_FBC_RB_B;
		break;

	default:
		return;
	};

	if (fbc != fmesa->fbc) {
		fmesa->fbc = fbc;
		fmesa->state_dirty |= FFB_STATE_FBC;
		fmesa->state_fifo_ents += 1;
	}
}

static void ffbDDSetColor(GLcontext *ctx, GLubyte r, GLubyte g, GLubyte b, GLubyte a)
{
	ffbContextPtr fmesa = FFB_CONTEXT(ctx);

	fmesa->MonoColor = ((r <<  0) |
			    (g << 8) |
			    (b << 16));
}

static void ffbDDClearColor(GLcontext *ctx, GLubyte r, GLubyte g, GLubyte b, GLubyte a)
{
	ffbContextPtr fmesa = FFB_CONTEXT(ctx);

	fmesa->clear_pixel = ((r << 0) |
			      (g << 8) |
			      (b << 16));
}

static void ffbDDClearDepth(GLcontext *ctx, GLclampd depth)
{
	ffbContextPtr fmesa = FFB_CONTEXT(ctx);

	fmesa->clear_depth = Z_FROM_MESA(depth * 4294967295.0f);
}

static void ffbDDClearStencil(GLcontext *ctx, GLint stencil)
{
	ffbContextPtr fmesa = FFB_CONTEXT(ctx);

	fmesa->clear_stencil = stencil & 0xf;
}

static void ffbDDReducedPrimitiveChange(GLcontext *ctx, GLenum prim)
{
	ffbContextPtr fmesa = FFB_CONTEXT(ctx);
	GLuint drawop, fbc, ppc;
	int do_sw = 0;

	drawop = fmesa->drawop;
	fbc = fmesa->fbc;
	ppc = fmesa->ppc & ~(FFB_PPC_ZS_MASK | FFB_PPC_CS_MASK);

#ifdef STATE_TRACE
	fprintf(stderr,
		"ffbDDReducedPrimitiveChange: prim(%d) ", prim);
#endif
	switch(prim) {
	case GL_POINTS:
#ifdef STATE_TRACE
		fprintf(stderr, "GL_POINTS ");
#endif
		if (ctx->IndirectTriangles & DD_POINT_SW_RASTERIZE) {
			do_sw = 1;
			break;
		}

		if (ctx->TriangleCaps & DD_POINT_SIZE) {
			ppc |= FFB_PPC_ZS_CONST | FFB_PPC_CS_CONST;
			drawop = FFB_DRAWOP_TRIANGLE;
		} else {
			if (ctx->Point.SmoothFlag) {
				ppc |= (FFB_PPC_ZS_VAR | FFB_PPC_CS_CONST);
				drawop = FFB_DRAWOP_AADOT;
			} else {
				ppc |= (FFB_PPC_ZS_CONST | FFB_PPC_CS_CONST);
				drawop = FFB_DRAWOP_DOT;
			}
		}
		break;

	case GL_LINES:
#ifdef STATE_TRACE
		fprintf(stderr, "GL_LINES ");
#endif
		if (ctx->IndirectTriangles & DD_LINE_SW_RASTERIZE) {
			do_sw = 1;
			break;
		}

		if (ctx->TriangleCaps & DD_FLATSHADE) {
			ppc |= FFB_PPC_ZS_VAR | FFB_PPC_CS_CONST;
		} else {
			ppc |= FFB_PPC_ZS_VAR | FFB_PPC_CS_VAR;
		}
		if (ctx->TriangleCaps & DD_LINE_WIDTH) {
			drawop = FFB_DRAWOP_TRIANGLE;
		} else {
			if (ctx->Line.SmoothFlag)
				drawop = FFB_DRAWOP_AALINE;
			else
				drawop = FFB_DRAWOP_DDLINE;
		}
		break;

	case GL_POLYGON:
#ifdef STATE_TRACE
		fprintf(stderr, "GL_POLYGON ");
#endif
		if (ctx->IndirectTriangles & DD_TRI_SW_RASTERIZE) {
			do_sw = 1;
			break;
		}

		ppc &= ~FFB_PPC_APE_MASK;
		if (ctx->Polygon.StippleFlag)
			ppc |= FFB_PPC_APE_ENABLE;
		else
			ppc |= FFB_PPC_APE_DISABLE;

		if (ctx->TriangleCaps & DD_FLATSHADE) {
			ppc |= FFB_PPC_ZS_VAR | FFB_PPC_CS_CONST;
		} else {
			ppc |= FFB_PPC_ZS_VAR | FFB_PPC_CS_VAR;
		}
		drawop = FFB_DRAWOP_TRIANGLE;
		break;

	default:
#ifdef STATE_TRACE
		fprintf(stderr, "unknown %d!\n", prim);
#endif
		return;
	};

#ifdef STATE_TRACE
	fprintf(stderr, "do_sw(%d) ", do_sw);
#endif
	if (do_sw != 0) {
		fbc &= ~(FFB_FBC_WB_C);
		fbc &= ~(FFB_FBC_ZE_MASK | FFB_FBC_RGBE_MASK);
		fbc |=   FFB_FBC_ZE_OFF  | FFB_FBC_RGBE_MASK;
		ppc &= ~(FFB_PPC_XS_MASK | FFB_PPC_ABE_MASK |
			 FFB_PPC_DCE_MASK | FFB_PPC_APE_MASK);
		ppc |=  (FFB_PPC_ZS_VAR | FFB_PPC_CS_VAR | FFB_PPC_XS_WID |
			 FFB_PPC_ABE_DISABLE | FFB_PPC_DCE_DISABLE |
			 FFB_PPC_APE_DISABLE);
	} else {
		fbc |= FFB_FBC_WB_C;
		fbc &= ~(FFB_FBC_RGBE_MASK);
		fbc |=   FFB_FBC_RGBE_MASK;
		ppc &= ~(FFB_PPC_ABE_MASK | FFB_PPC_XS_MASK);
		if (ctx->Color.BlendEnabled) {
			ppc |= FFB_PPC_ABE_ENABLE | FFB_PPC_XS_VAR;
		} else {
			ppc |= FFB_PPC_ABE_DISABLE | FFB_PPC_XS_WID;
		}
	}
#ifdef STATE_TRACE
	fprintf(stderr, "fbc(%08x) ppc(%08x)\n", fbc, ppc);
#endif

	FFBFifo(fmesa, 4);
	if (fmesa->drawop != drawop)
		fmesa->regs->drawop = fmesa->drawop = drawop;
	if (fmesa->fbc != fbc)
		fmesa->regs->fbc = fmesa->fbc = fbc;
	if (fmesa->ppc != ppc)
		fmesa->regs->ppc = fmesa->ppc = ppc;
	if (do_sw != 0) {
		fmesa->regs->cmp =
			(fmesa->cmp & ~(0xff<<16)) | (0x80 << 16);
	} else
		fmesa->regs->cmp = fmesa->cmp;

	/* Flush the vertex cache. */
	fmesa->vtx_cache[0] = fmesa->vtx_cache[1] =
		fmesa->vtx_cache[2] = fmesa->vtx_cache[3] = NULL;
}

/* XXX Actually, should I be using FBC controls for this? -DaveM */
static GLboolean ffbDDColorMask(GLcontext *ctx,
				GLboolean r, GLboolean g,
				GLboolean b, GLboolean a)
{
	ffbContextPtr fmesa = FFB_CONTEXT(ctx);
	unsigned int new_pmask = 0x0;

#ifdef STATE_TRACE
	fprintf(stderr, "ffbDDColorMask: r(%d) g(%d) b(%d) a(%d)\n",
		r, g, b, a);
#endif
	if (r)
		new_pmask |= 0x000000ff;
	if (g)
		new_pmask |= 0x0000ff00;
	if (b)
		new_pmask |= 0x00ff0000;

	if (fmesa->pmask != new_pmask) {
		fmesa->pmask = new_pmask;
		fmesa->state_dirty |= FFB_STATE_PMASK;
		fmesa->state_fifo_ents += 1;
	}

	return GL_TRUE;
}

static GLboolean ffbDDLogicOp(GLcontext *ctx, GLenum op)
{
	ffbContextPtr fmesa = FFB_CONTEXT(ctx);
	unsigned int rop;

#ifdef STATE_TRACE
	fprintf(stderr, "ffbDDLogicOp: op(%s)\n",
		gl_lookup_enum_by_nr(op));
#endif
	switch (op) {
	case GL_CLEAR: rop = FFB_ROP_ZERO; break;
	case GL_SET: rop = FFB_ROP_ONES; break;
	case GL_COPY: rop = FFB_ROP_NEW; break;
	case GL_AND: rop = FFB_ROP_NEW_AND_OLD; break;
	case GL_NAND: rop = FFB_ROP_NEW_AND_NOLD; break;
	case GL_OR: rop = FFB_ROP_NEW_OR_OLD; break;
	case GL_NOR: rop = FFB_ROP_NEW_OR_NOLD; break;
	case GL_XOR: rop = FFB_ROP_NEW_XOR_OLD; break;
	case GL_NOOP: rop = FFB_ROP_OLD; break;
	case GL_COPY_INVERTED: rop = FFB_ROP_NNEW; break;
	case GL_INVERT: rop = FFB_ROP_NOLD; break;
	case GL_EQUIV: rop = FFB_ROP_NNEW_XOR_NOLD; break;
	case GL_AND_REVERSE: rop = FFB_ROP_NEW_AND_NOLD; break;
	case GL_AND_INVERTED: rop = FFB_ROP_NNEW_AND_OLD; break;
	case GL_OR_REVERSE: rop = FFB_ROP_NEW_OR_NOLD; break;
	case GL_OR_INVERTED: rop = FFB_ROP_NNEW_OR_OLD; break;

	default:
		return GL_FALSE;
	};

	rop |= fmesa->rop & ~0xff;
	if (rop != fmesa->rop) {
		fmesa->rop = rop;
		fmesa->state_dirty |= FFB_STATE_ROP;
		fmesa->state_fifo_ents += 1;

		if (op == GL_COPY)
			fmesa->bad_fragment_attrs &= ~FFB_BADATTR_BLENDROP;
	}

	return GL_TRUE;
}

#if 0
/* XXX Also need to track near/far just like 3dfx driver.
 * XXX
 * XXX Actually, that won't work, because the 3dfx chip works by
 * XXX having 1/w coordinates fed to it for each primitive, and
 * XXX it uses this to index it's 64 entry fog table.
 */
static void ffb_fog_linear(GLcontext *ctx, ffbContextPtr fmesa)
{
	GLfloat c = ctx->ProjectionMatrix.m[10];
	GLfloat d = ctx->ProjectionMatrix.m[14];
	GLfloat tz = ctx->Viewport.WindowMap.m[MAT_TZ];
	GLfloat szInv = 1.0F / ctx->Viewport.WindowMap.m[MAT_SZ];
	GLfloat fogEnd = ctx->Fog.End;
	GLfloat fogScale = 1.0F / (ctx->Fog.End - ctx->Fog.Start);
	GLfloat ndcz;
	GLfloat eyez;
	GLfloat Zzero, Zone;
	unsigned int zb, zf;

	/* Compute the Z at which f reaches 0.0, this is the full
	 * saturation point.
	 *
	 * Thus compute Z (as seen by the chip during rendering),
	 * such that:
	 *
	 *	0.0 = (fogEnd - eyez) * fogScale
	 *
	 * fogScale is usually not zero, thus we are looking for:
	 *
	 *	fogEnd = eyez
	 *
	 *	fogEnd = -d / (c + ((Z - tz) * szInv))
	 *	fogEnd * (c + ((Z - tz) * szInv)) = -d
	 *	(c + ((Z - tz) * szInv)) = -d / fogEnd
	 *	(Z - tz) * szInv = (-d / fogEnd) - c
	 *	(Z - tz) = ((-d / fogEnd) - c) / szInv
	 *	Z = (((-d / fogEnd) - c) / szInv) + tz
	 */
	Zzero = (((-d / fogEnd) - c) / szInv) + tz;

	/* Compute the Z at which f reaches 1.0, this is where
	 * the incoming frag's full intensity is shown.  This
	 * equation is:
	 *
	 *	1.0 = (fogEnd - eyez)
	 *
	 * We are looking for:
	 *
	 *	1.0 + eyez = fogEnd
	 *
	 *	1.0 + (-d / (c + ((Z - tz) * szInv))) = fogEnd
	 *	-d / (c + ((Z - tz) * szInv)) = fogEnd - 1.0
	 *	-d / (FogEnd - 1.0) = (c + ((Z - tz) * szInv))
	 *	(-d / (fogEnd - 1.0)) - c = ((Z - tz) * szInv)
	 *	((-d / (fogEnd - 1.0)) - c) / szInv = (Z - tz)
	 *	(((-d / (fogEnd - 1.0)) - c) / szInv) + tz = Z
	 */
	Zone = (((-d / (fogEnd - 1.0)) - c) / szInv) + tz;

	/* FFB's Zfront must be less than Zback, thus we may have
	 * to invert Sf/Sb to satisfy this constraint.
	 */
	if (Zzero < Zone) {
		sf = 0.0;
		sb = 1.0;
		zf = Z_FROM_MESA(Zzero);
		zb = Z_FROM_MESA(Zone);
	} else {
		sf = 1.0;
		sb = 0.0;
		zf = Z_FROM_MESA(Zone);
		zb = Z_FROM_MESA(Zzero);
	}
}
#endif

static void ffbDDFogfv(GLcontext *ctx, GLenum pname, const GLfloat *param)
{
#ifdef STATE_TRACE
	fprintf(stderr, "ffbDDFogfv: pname(%s)\n", gl_lookup_enum_by_nr(pname));
#endif
}

static void ffbDDLineStipple(GLcontext *ctx, GLint factor, GLushort pattern)
{
	ffbContextPtr fmesa = FFB_CONTEXT(ctx);

#ifdef STATE_TRACE
	fprintf(stderr, "ffbDDLineStipple: factor(%d) pattern(%04x)\n",
		factor, pattern);
#endif
	if (ctx->Line.StippleFlag) {
		factor = ctx->Line.StippleFactor;
		pattern = ctx->Line.StipplePattern;
		if ((GLuint) factor > 15) {
			ctx->Driver.TriangleCaps &= ~DD_LINE_STIPPLE;
			fmesa->lpat = FFB_LPAT_BAD;
		} else {
			ctx->Driver.TriangleCaps |= DD_LINE_STIPPLE;
			fmesa->lpat = ((factor << FFB_LPAT_SCALEVAL_SHIFT) |
				       (0 << FFB_LPAT_PATLEN_SHIFT) |
				       ((pattern & 0xffff) << FFB_LPAT_PATTERN_SHIFT));
		}
	} else {
		fmesa->lpat = 0;
	}
}

void ffbXformAreaPattern(ffbContextPtr fmesa, const GLubyte *mask)
{
	__DRIdrawablePrivate *dPriv = fmesa->driDrawable;
	int i, lines, xoff;

	lines = 0;
	i = (dPriv->y + dPriv->h) & (32 - 1);
	xoff = dPriv->x & (32 - 1);
	while (lines++ < 32) {
		GLuint raw =
			(((GLuint)mask[0] << 24) |
			 ((GLuint)mask[1] << 16) |
			 ((GLuint)mask[2] <<  8) |
			 ((GLuint)mask[3] <<  0));

		fmesa->pattern[i] =
			(raw << xoff) | (raw >> (32 - xoff));
		i = (i - 1) & (32 - 1);
		mask += 4;
	}

	fmesa->state_dirty |= FFB_STATE_APAT;
	fmesa->state_fifo_ents += 32;
}

static void ffbDDPolygonStipple(GLcontext *ctx, const GLubyte *mask)
{
	ffbContextPtr fmesa = FFB_CONTEXT(ctx);

#ifdef STATE_TRACE
	fprintf(stderr, "ffbDDPolygonStipple: state(%d)\n",
		ctx->Polygon.StippleFlag);
#endif
	if (ctx->Polygon.StippleFlag) {
		ctx->Driver.TriangleCaps |= DD_TRI_STIPPLE;
	} else {
		ctx->Driver.TriangleCaps &= ~DD_TRI_STIPPLE;
	}
	ffbXformAreaPattern(fmesa, mask);
}

static void ffbDDEnable(GLcontext *ctx, GLenum cap, GLboolean state)
{
	ffbContextPtr fmesa = FFB_CONTEXT(ctx);
	unsigned int tmp;

#ifdef STATE_TRACE
	fprintf(stderr, "ffbDDEnable: %s state(%d)\n",
		gl_lookup_enum_by_nr(cap), state);
#endif
	switch (cap) {
	case GL_ALPHA_TEST:
		if (state)
			tmp = ffbComputeAlphaFunc(ctx);
		else
			tmp = FFB_XCLIP_TEST_ALWAYS;

		if (tmp != fmesa->xclip) {
			fmesa->xclip = tmp;
			fmesa->state_dirty |= FFB_STATE_XCLIP;
			fmesa->state_fifo_ents += 1;
		}
		break;

	case GL_BLEND:
		tmp = (fmesa->ppc & ~FFB_PPC_ABE_MASK);
		if (state) {
			tmp |= FFB_PPC_ABE_ENABLE;
		} else {
			tmp |= FFB_PPC_ABE_DISABLE;
		}
		if (fmesa->ppc != tmp) {
			fmesa->ppc = tmp;
			fmesa->state_dirty |= FFB_STATE_PPC;
			fmesa->state_fifo_ents += 1;
			ffbDDBlendFunc(ctx, 0, 0);
		}
		break;

	case GL_DEPTH_TEST:
		if (state)
			tmp = 0x0fffffff;
		else
			tmp = 0x00000000;
		if (tmp != fmesa->magnc) {
			unsigned int fbc = fmesa->fbc;
			fbc &= ~FFB_FBC_ZE_MASK;
			if (state)
				fbc |= FFB_FBC_ZE_ON;
			else
				fbc |= FFB_FBC_ZE_OFF;
			fmesa->fbc = fbc;
			ffbDDDepthFunc(ctx, ctx->Depth.Func);
			fmesa->magnc = tmp;
			fmesa->state_dirty |= FFB_STATE_MAGNC | FFB_STATE_FBC;
			fmesa->state_fifo_ents += 2;
		}
		break;

	case GL_SCISSOR_TEST:
		tmp = fmesa->ppc & ~FFB_PPC_VCE_MASK;
		if (state) {
			ffbDDScissor(ctx, ctx->Scissor.X, ctx->Scissor.Y,
				     ctx->Scissor.Width, ctx->Scissor.Height);
			tmp |= FFB_PPC_VCE_2D;
	        } else {
			tmp |= FFB_PPC_VCE_DISABLE;
		}
		break;

	case GL_STENCIL_TEST:
		if (!(fmesa->ffb_sarea->flags & FFB_DRI_FFB2PLUS)) {
			if (state)
				fmesa->bad_fragment_attrs |= FFB_BADATTR_STENCIL;
			else
				fmesa->bad_fragment_attrs &= ~FFB_BADATTR_STENCIL;
			break;
		}

		tmp = fmesa->fbc & ~FFB_FBC_YE_MASK;
		if (state) {
			ffbDDStencilFunc(ctx,
					 ctx->Stencil.Function,
					 ctx->Stencil.Ref,
					 ctx->Stencil.ValueMask);
			ffbDDStencilMask(ctx, ctx->Stencil.WriteMask);
			ffbDDStencilOp(ctx,
				       ctx->Stencil.FailFunc,
				       ctx->Stencil.ZFailFunc,
				       ctx->Stencil.ZPassFunc);
			tmp |= FFB_FBC_YE_MASK;
		} else {
			fmesa->stencil		= 0xf0000000;
			fmesa->stencilctl	= 0x33300000;
			fmesa->state_dirty |= FFB_STATE_STENCIL;
			fmesa->state_fifo_ents += 6;
			tmp |= FFB_FBC_YE_OFF;
		}
		if (tmp != fmesa->fbc) {
			fmesa->fbc = tmp;
			fmesa->state_dirty |= FFB_STATE_FBC;
			fmesa->state_fifo_ents += 1;
		}
		break;

	case GL_FOG:
		/* Until I implement the fog support... */
		if (state)
			fmesa->bad_fragment_attrs |= FFB_BADATTR_FOG;
		else
			fmesa->bad_fragment_attrs &= ~FFB_BADATTR_FOG;
		break;

	case GL_LINE_STIPPLE:
		if (! state)
			fmesa->lpat = 0;
		else
			ffbDDLineStipple(ctx,
					 ctx->Line.StippleFactor,
					 ctx->Line.StipplePattern);
		break;

	case GL_POLYGON_STIPPLE:
		/* Do nothing, we interrogate the state during
		 * reduced primitive changes.  Since our caller
		 * will set NEW_POLYGON in the ctx NewState this
		 * will cause the driver rasterization functions
		 * to be reevaluated, which will cause us to force
		 * a reduced primitive change next rendering pass
		 * and it all works out.
		 */
		break;

	default:
		break;
	};
}

void ffbSyncHardware(ffbContextPtr fmesa)
{
	ffb_fbcPtr ffb = fmesa->regs;
	unsigned int dirty;
	int i;

	FFBFifo(fmesa, fmesa->state_fifo_ents);

	dirty = fmesa->state_dirty;
	if (dirty & (FFB_STATE_FBC | FFB_STATE_PPC | FFB_STATE_DRAWOP |
		     FFB_STATE_ROP | FFB_STATE_LPAT | FFB_STATE_WID)) {
		if (dirty & FFB_STATE_FBC)
			ffb->fbc = fmesa->fbc;
		if (dirty & FFB_STATE_PPC)
			ffb->ppc = fmesa->ppc;
		if (dirty & FFB_STATE_DRAWOP)
			ffb->drawop = fmesa->drawop;
		if (dirty & FFB_STATE_ROP)
			ffb->rop = fmesa->rop;
		if (dirty & FFB_STATE_LPAT)
			ffb->rop = fmesa->lpat;
		if (dirty & FFB_STATE_WID)
			ffb->wid = fmesa->wid;
	}
	if (dirty & (FFB_STATE_PMASK | FFB_STATE_XPMASK | FFB_STATE_YPMASK |
		     FFB_STATE_ZPMASK | FFB_STATE_XCLIP | FFB_STATE_CMP |
		     FFB_STATE_MATCHAB | FFB_STATE_MAGNAB | FFB_STATE_MATCHC |
		     FFB_STATE_MAGNC)) {
		if (dirty & FFB_STATE_PMASK)
			ffb->pmask = fmesa->pmask;
		if (dirty & FFB_STATE_XPMASK)
			ffb->xpmask = fmesa->xpmask;
		if (dirty & FFB_STATE_YPMASK)
			ffb->ypmask = fmesa->ypmask;
		if (dirty & FFB_STATE_ZPMASK)
			ffb->zpmask = fmesa->zpmask;
		if (dirty & FFB_STATE_XCLIP)
			ffb->xclip = fmesa->xclip;
		if (dirty & FFB_STATE_CMP)
			ffb->cmp = fmesa->cmp;
		if (dirty & FFB_STATE_MATCHAB)
			ffb->matchab = fmesa->matchab;
		if (dirty & FFB_STATE_MAGNAB)
			ffb->magnab = fmesa->magnab;
		if (dirty & FFB_STATE_MATCHC)
			ffb->matchc = fmesa->matchc;
		if (dirty & FFB_STATE_MAGNC)
			ffb->magnc = fmesa->magnc;
	}

	if (dirty & FFB_STATE_DCUE) {
		ffb->dcss = fmesa->dcss;
		ffb->dcsf = fmesa->dcsf;
		ffb->dcsb = fmesa->dcsb;
		ffb->dczf = fmesa->dczf;
		ffb->dczb = fmesa->dczb;
		if (fmesa->ffb_sarea->flags & (FFB_DRI_FFB2 | FFB_DRI_FFB2PLUS)) {
			ffb->dcss1 = fmesa->dcss1;
			ffb->dcss2 = fmesa->dcss2;
			ffb->dcss3 = fmesa->dcss3;
			ffb->dcs2  = fmesa->dcs2;
			ffb->dcs3  = fmesa->dcs3;
			ffb->dcs4  = fmesa->dcs4;
			ffb->dcd2  = fmesa->dcd2;
			ffb->dcd3  = fmesa->dcd3;
			ffb->dcd4  = fmesa->dcd4;
		}
	}

	if (dirty & FFB_STATE_BLEND) {
		ffb->blendc  = fmesa->blendc;
		ffb->blendc1 = fmesa->blendc1;
		ffb->blendc2 = fmesa->blendc2;
	}

	if (dirty & FFB_STATE_CLIP) {
		ffb->vclipmin  = fmesa->vclipmin;
		ffb->vclipmax  = fmesa->vclipmax;
		ffb->vclipzmin = fmesa->vclipzmin;
		ffb->vclipzmax = fmesa->vclipzmax;
		for (i = 0; i < 4; i++) {
			ffb->auxclip[i].min = fmesa->aux_clips[i].min;
			ffb->auxclip[i].max = fmesa->aux_clips[i].max;
		}
	}

	if ((dirty & FFB_STATE_STENCIL) &&
	    (fmesa->ffb_sarea->flags & FFB_DRI_FFB2PLUS)) {
		ffb->stencil    = fmesa->stencil;
		ffb->stencilctl = fmesa->stencilctl;
		ffb->fbc = FFB_FBC_WB_C;
		ffb->rawstencilctl = (fmesa->stencilctl | (1 << 19));
		ffb->fbc = fmesa->fbc;
		ffb->consty = fmesa->consty;
	}

	if (dirty & FFB_STATE_APAT) {
		for (i = 0; i < 32; i++)
			ffb->pattern[i] = fmesa->pattern[i];
	}

	fmesa->state_dirty = 0;
	fmesa->state_fifo_ents = 0;
	fmesa->ffbScreen->rp_active = 1;
}

static void ffbDDRenderStart(GLcontext *ctx)
{
	ffbContextPtr fmesa = FFB_CONTEXT(ctx);

	LOCK_HARDWARE(fmesa);
	fmesa->hw_locked = 1;

	if (fmesa->state_dirty != 0)
		ffbSyncHardware(fmesa);
}

static void ffbDDRenderFinish(GLcontext *ctx)
{
	ffbContextPtr fmesa = FFB_CONTEXT(ctx);

	UNLOCK_HARDWARE(fmesa);
	fmesa->hw_locked = 0;
}

#define INTERESTED (~(NEW_MODELVIEW|NEW_PROJECTION|\
                      NEW_TEXTURE_MATRIX|\
                      NEW_USER_CLIP|NEW_CLIENT_STATE|\
                      NEW_TEXTURE_ENABLE))

static void ffbDDUpdateState(GLcontext *ctx)
{
	ffbContextPtr fmesa = FFB_CONTEXT(ctx);
	unsigned int flags;

	if (ctx->NewState & INTERESTED) {
		if (fmesa->SWrender ||
		    fmesa->bad_fragment_attrs != 0) {
			/* Force SW rendering. */
			fmesa->PointsFunc = NULL;
			fmesa->LineFunc = NULL;
			fmesa->TriangleFunc = NULL;
			fmesa->QuadFunc = NULL;
		} else {
			ffbDDChooseTriRenderState(ctx);
			ffbDDChooseLineRenderState(ctx);
			ffbDDChoosePointRenderState(ctx);
		}

		if (0)
			gl_print_tri_caps("tricaps", ctx->TriangleCaps);

		ffbChooseRasterSetupFunc(ctx);

		/* Force a reduced primitive change next rendering
		 * pass.
		 */
		ctx->PB->primitive = GL_POLYGON + 1;
	}

#if 0
	/* When the modelview matrix changes, this changes what
	 * the eye coordinates will be so we have to recompute
	 * the depth cueing parameters.
	 *
	 * XXX DD_HAVE_HARDWARE_FOG.
	 */
	if (ctx->Fog.Enabled && (ctx->NewState & NEW_MODELVIEW))
		ffb_update_fog();
#endif

	/* XXX It may be possible to eliminate all of Mesa's sw clip
	 * XXX processing using the hw clip registers we have.  If
	 * XXX this is correct, it is just a matter of verifying the
	 * XXX FFB coordinate overflow rules in ffb_vb.c and if they
	 * XXX are not violated we clear CLIP_MASK_ACTIVE in
	 * XXX VB->CullMode.
	 * XXX
	 * XXX We would specify the xyz min/max values in the primary
	 * XXX viewport clip registers, and the user specified
	 * XXX scissor clip can go into one of the auxilliary viewport
	 * XXX clips.
	 */
	flags = ctx->IndirectTriangles;
	if (fmesa->PointsFunc != NULL) {
		ctx->Driver.PointsFunc = fmesa->PointsFunc;
		flags &= ~DD_POINT_SW_RASTERIZE;
	}
	if (fmesa->LineFunc != NULL) {
		ctx->Driver.LineFunc = fmesa->LineFunc;
		flags &= ~DD_LINE_SW_RASTERIZE;
	}
	if (fmesa->TriangleFunc != NULL) {
		ctx->Driver.TriangleFunc = fmesa->TriangleFunc;
		ctx->Driver.QuadFunc = fmesa->QuadFunc;
		flags &= ~(DD_TRI_SW_RASTERIZE |
			   DD_QUAD_SW_RASTERIZE);
	}
	ctx->IndirectTriangles = flags;
}

void ffbDDInitStateFuncs(GLcontext *ctx)
{
	ffbContextPtr fmesa = FFB_CONTEXT(ctx);

	ctx->Driver.UpdateState = ffbDDUpdateState;
	ctx->Driver.Enable = ffbDDEnable;
	ctx->Driver.LightModelfv = NULL;
	ctx->Driver.AlphaFunc = ffbDDAlphaFunc;
	ctx->Driver.BlendEquation = ffbDDBlendEquation;
	ctx->Driver.BlendFunc = ffbDDBlendFunc;
	ctx->Driver.BlendFuncSeparate = ffbDDBlendFuncSeparate;
	ctx->Driver.DepthFunc = ffbDDDepthFunc;
	ctx->Driver.DepthMask = ffbDDDepthMask;
	ctx->Driver.Fogfv = ffbDDFogfv;
	ctx->Driver.LineStipple = ffbDDLineStipple;
	ctx->Driver.PolygonStipple = ffbDDPolygonStipple;
	ctx->Driver.Scissor = ffbDDScissor;
	ctx->Driver.CullFace = NULL;
	ctx->Driver.FrontFace = NULL;
	ctx->Driver.ColorMask = ffbDDColorMask;
	ctx->Driver.LogicOp = ffbDDLogicOp;
	ctx->Driver.ReducedPrimitiveChange = ffbDDReducedPrimitiveChange;
	ctx->Driver.RenderStart = ffbDDRenderStart; 
	ctx->Driver.RenderFinish = ffbDDRenderFinish; 

	if (fmesa->ffb_sarea->flags & FFB_DRI_FFB2PLUS) {
		ctx->Driver.StencilFunc = ffbDDStencilFunc;
		ctx->Driver.StencilMask = ffbDDStencilMask;
		ctx->Driver.StencilOp = ffbDDStencilOp;
	} else {
		ctx->Driver.StencilFunc = NULL;
		ctx->Driver.StencilMask = NULL;
		ctx->Driver.StencilOp = NULL;
	}

	ctx->Driver.SetDrawBuffer = ffbDDSetDrawBuffer;
	ctx->Driver.SetReadBuffer = ffbDDSetReadBuffer;
	ctx->Driver.Color = ffbDDSetColor;
	ctx->Driver.ClearColor = ffbDDClearColor;
	ctx->Driver.ClearDepth = ffbDDClearDepth;
	ctx->Driver.ClearStencil = ffbDDClearStencil;
	ctx->Driver.Dither = NULL;

	/* We will support color index modes later... -DaveM */
	ctx->Driver.Index = 0;
	ctx->Driver.ClearIndex = 0;
	ctx->Driver.IndexMask = 0;
}

void ffbDDInitContextHwState(GLcontext *ctx)
{
	ffbContextPtr fmesa = FFB_CONTEXT(ctx);
	int fifo_count = 0;
	int i;

	fmesa->hw_locked = 0;

	fmesa->bad_fragment_attrs = 0;
	fmesa->state_dirty = FFB_STATE_ALL;

	fifo_count = 1;
	fmesa->fbc = (FFB_FBC_WE_FORCEON | FFB_FBC_WM_COMBINED |
		      FFB_FBC_SB_BOTH | FFB_FBC_ZE_MASK |
		      FFB_FBC_YE_OFF | FFB_FBC_XE_OFF |
		      FFB_FBC_RGBE_MASK);
	if (ctx->Visual->DBflag) {
		/* Buffer B is the initial back buffer. */
		fmesa->back_buffer = 1;
		fmesa->fbc |= FFB_FBC_WB_BC | FFB_FBC_RB_B;
	} else {
		fmesa->back_buffer = 0;
		fmesa->fbc |= FFB_FBC_WB_A | FFB_FBC_RB_A;
	}

	fifo_count += 1;
	fmesa->ppc = (FFB_PPC_ACE_DISABLE | FFB_PPC_DCE_DISABLE |
		      FFB_PPC_ABE_DISABLE | FFB_PPC_VCE_DISABLE |
		      FFB_PPC_APE_DISABLE | FFB_PPC_TBE_OPAQUE |
		      FFB_PPC_ZS_CONST | FFB_PPC_YS_CONST |
		      FFB_PPC_XS_WID | FFB_PPC_CS_VAR);

	fifo_count += 3;
	fmesa->drawop = FFB_DRAWOP_RECTANGLE;

	/* GL_COPY is the default LogicOp. */
	fmesa->rop = (FFB_ROP_NEW << 16) | (FFB_ROP_NEW << 8) | FFB_ROP_NEW;

	/* No line patterning enabled. */
	fmesa->lpat = 0x00000000;

	/* We do not know the WID value until the first context switch. */
	fifo_count += 1;
	fmesa->wid = ~0;

	fifo_count += 5;

	/* ColorMask, all enabled. */
	fmesa->pmask  = 0xffffffff;

	fmesa->xpmask = 0x000000ff;
	fmesa->ypmask = 0x0000000f;
	fmesa->zpmask = 0x0fffffff;

	/* AlphaFunc GL_ALWAYS, AlphaRef 0 */
	fmesa->xclip  = FFB_XCLIP_TEST_ALWAYS | 0x00;

	/* This sets us up to use WID clipping (so the DRI clipping
	 * rectangle is unneeded by us).  All other match and magnitude
	 * tests are set to pass.
	 */
	fifo_count += 5;
	fmesa->cmp = ((FFB_CMP_MATCH_ALWAYS << 24) |	/* MATCH C  */
		      (FFB_CMP_MAGN_ALWAYS  << 16) |	/* MAGN  C  */
		      (FFB_CMP_MATCH_EQ     <<  8) |	/* MATCH AB */
		      (FFB_CMP_MAGN_ALWAYS  <<  0));	/* MAGN  AB */
	fmesa->matchab = 0xff000000;
	fmesa->magnab  = 0x00000000;
	fmesa->matchc  = 0x00000000;
	fmesa->magnc   = 0x00000000;

	/* Depth cue parameters, all zeros to start. */
	fifo_count += 14;
	fmesa->dcss  = 0x00000000;
	fmesa->dcsf  = 0x00000000;
	fmesa->dcsb  = 0x00000000;
	fmesa->dczf  = 0x00000000;
	fmesa->dczb  = 0x00000000;
	fmesa->dcss1 = 0x00000000;
	fmesa->dcss2 = 0x00000000;
	fmesa->dcss3 = 0x00000000;
	fmesa->dcs2  = 0x00000000;
	fmesa->dcs3  = 0x00000000;
	fmesa->dcs4  = 0x00000000;
	fmesa->dcd2  = 0x00000000;
	fmesa->dcd3  = 0x00000000;
	fmesa->dcd4  = 0x00000000;

	/* Alpha blending unit state. */
	fifo_count += 3;
	fmesa->blendc  = (1 << 0) | (0 << 2);  /* src(GL_ONE) | dst(GL_ZERO) */
	fmesa->blendc1 = 0x00000000;
	fmesa->blendc2 = 0x00000000;

	/* ViewPort clip state. */
	fifo_count += 4 + (4 * 2);
	fmesa->vclipmin  = 0x00000000;
	fmesa->vclipmax  = 0x00000000;
	fmesa->vclipzmin = 0x00000000;
	fmesa->vclipzmax = 0x00000000;
	for (i = 0; i < 4; i++) {
		fmesa->aux_clips[0].min = 0x00000000;
		fmesa->aux_clips[0].max = 0x00000000;
	}

	/* Stenciling state. */
	fifo_count += 6;
	fmesa->stencil    = 0xf0000000; /* Stencil MASK, Y plane */
	fmesa->stencilctl = 0x33300000; /* All stencil tests disabled */
	fmesa->consty	  = 0x0;

	/* Area pattern, used for polygon stipples. */
	fifo_count += 32;
	for (i = 0; i < 32; i++)
		fmesa->pattern[i] = 0x00000000;

	fmesa->state_fifo_ents = fifo_count;
	fmesa->state_all_fifo_ents = fifo_count;
}
