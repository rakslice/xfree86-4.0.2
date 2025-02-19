/* $XFree86: xc/lib/GL/mesa/src/drv/tdfx/fxtritmp.h,v 1.3 2000/12/08 19:36:23 alanh Exp $ */
/*
 * Mesa 3-D graphics library
 * Version:  3.3
 *
 * Copyright (C) 1999-2000  Brian Paul   All Rights Reserved.
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
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *
 * Original Mesa / 3Dfx device driver (C) 1999 David Bucciarelli, by the
 * terms stated above.
 *
 * Thank you for your contribution, David!
 *
 * Please make note of the above copyright/license statement.  If you
 * contributed code or bug fixes to this code under the previous (GNU
 * Library) license and object to the new license, your code will be
 * removed at your request.  Please see the Mesa docs/COPYRIGHT file
 * for more information.
 *
 * Additional Mesa/3Dfx driver developers:
 *   Daryll Strauss <daryll@precisioninsight.com>
 *   Keith Whitwell <keith@precisioninsight.com>
 *
 * See fxapi.h for more revision/author details.
 */


static void TAG(fx_tri) (GLcontext * ctx, GLuint e1, GLuint e2, GLuint e3,
                         GLuint pv)
{
    fxMesaContext fxMesa = (fxMesaContext) ctx->DriverCtx;
    struct vertex_buffer *VB = ctx->VB;
    fxVertex *gWin = FX_DRIVER_DATA(VB)->verts;
    GrVertex *v1 = (GrVertex *) gWin[e1].f;
    GrVertex *v2 = (GrVertex *) gWin[e2].f;
    GrVertex *v3 = (GrVertex *) gWin[e3].f;

    (void) fxMesa;

    if (IND & (FX_TWOSIDE | FX_OFFSET)) {
        GLfloat ex = v1->x - v3->x;
        GLfloat ey = v1->y - v3->y;
        GLfloat fx = v2->x - v3->x;
        GLfloat fy = v2->y - v3->y;
        GLfloat c = ex * fy - ey * fx;

        if (IND & FX_TWOSIDE) {
            GLuint facing = (c < 0.0) ^ ctx->Polygon.FrontBit;
            GLubyte(*color)[4] = VB->Color[facing]->data;
            if (IND & FX_FLAT) {
                GOURAUD2(v1, color[pv]);
                GOURAUD2(v2, color[pv]);
                GOURAUD2(v3, color[pv]);
            }
            else {
                GOURAUD2(v1, color[e1]);
                GOURAUD2(v2, color[e2]);
                GOURAUD2(v3, color[e3]);
            }
        }

        /* Should apply a factor to ac to compensate for different x/y
         * scaling introduced in the Viewport matrix.
         *
         * The driver should supply scaling factors for 'factor' and 'units'.
         */
        if (IND & FX_OFFSET) {
            GLfloat offset = ctx->Polygon.OffsetUnits;

            if (c * c > 1e-16) {
                GLfloat factor = ctx->Polygon.OffsetFactor;
                GLfloat ez = v1->ooz - v3->ooz;
                GLfloat fz = v2->ooz - v3->ooz;
                GLfloat a = ey * fz - ez * fy;
                GLfloat b = ez * fx - ex * fz;
                GLfloat ic = 1.0 / c;
                GLfloat ac = a * ic;
                GLfloat bc = b * ic;
                if (ac < 0.0F)
                    ac = -ac;
                if (bc < 0.0F)
                    bc = -bc;
                offset += MAX2(ac, bc) * factor;
            }
            /* Probably a lot quicker just to nudge the z values and put
             * them back afterwards.
             */
            FX_grDepthBiasLevel(fxMesa, (int) offset);
        }
    }
    else if (IND & FX_FLAT) {
        GLubyte(*color)[4] = VB->Color[0]->data;
        GOURAUD2(v1, color[pv]);
        GOURAUD2(v2, color[pv]);
        GOURAUD2(v3, color[pv]);
    }

    if (IND & FX_FRONT_BACK) {
        FX_grColorMaskv(ctx, ctx->Color.ColorMask);
        FX_grDepthMask(fxMesa, FXFALSE);
        FX_grRenderBuffer(fxMesa, GR_BUFFER_BACKBUFFER);
    }

    if (IND & FX_ANTIALIAS)
        FX_grAADrawTriangle(fxMesa, v1, v2, v3, FXTRUE, FXTRUE, FXTRUE);
    else
        FX_grDrawTriangle(fxMesa, v1, v2, v3);

    /* Might be quicker to do two passes, one for each buffer?
     */
    if (IND & FX_FRONT_BACK) {
        FX_grColorMaskv(ctx, ctx->Color.ColorMask);

        if (ctx->Depth.Mask)
            FX_grDepthMask(fxMesa, FXTRUE);

        FX_grRenderBuffer(fxMesa, GR_BUFFER_FRONTBUFFER);

        if (IND & FX_ANTIALIAS)
            FX_grAADrawTriangle(fxMesa, v1, v2, v3, FXTRUE, FXTRUE, FXTRUE);
        else
            FX_grDrawTriangle(fxMesa, v1, v2, v3);
    }
}


/* Not worth the space?
 */
static void TAG(fx_quad) (GLcontext * ctx, GLuint e1, GLuint e2, GLuint e3,
                          GLuint e4, GLuint pv)
{
    fxMesaContext fxMesa = (fxMesaContext) ctx->DriverCtx;
    struct vertex_buffer *VB = ctx->VB;
    fxVertex *gWin = FX_DRIVER_DATA(VB)->verts;
    GrVertex *v1 = (GrVertex *) gWin[e1].f;
    GrVertex *v2 = (GrVertex *) gWin[e2].f;
    GrVertex *v3 = (GrVertex *) gWin[e3].f;
    GrVertex *v4 = (GrVertex *) gWin[e4].f;

    (void) fxMesa;

    if (IND & (FX_TWOSIDE | FX_OFFSET)) {
        GLfloat ex = v3->x - v1->x;
        GLfloat ey = v3->y - v1->y;
        GLfloat fx = v4->x - v2->x;
        GLfloat fy = v4->y - v2->y;
        GLfloat c = ex * fy - ey * fx;

        if (IND & FX_TWOSIDE) {
            GLuint facing = (c < 0.0) ^ ctx->Polygon.FrontBit;
            GLubyte(*color)[4] = VB->Color[facing]->data;
            if (IND & FX_FLAT) {
                GOURAUD2(v1, color[pv]);
                GOURAUD2(v2, color[pv]);
                GOURAUD2(v3, color[pv]);
                GOURAUD2(v4, color[pv]);
            }
            else {
                GOURAUD2(v1, color[e1]);
                GOURAUD2(v2, color[e2]);
                GOURAUD2(v3, color[e3]);
                GOURAUD2(v4, color[e4]);
            }
        }

        /* Should apply a factor to ac to compensate for different x/y
         * scaling introduced in the Viewport matrix.
         *
         * The driver should supply scaling factors for 'factor' and 'units'.
         */
        if (IND & FX_OFFSET) {
            GLfloat offset = ctx->Polygon.OffsetUnits;

            if (c * c > 1e-16) {
                GLfloat factor = ctx->Polygon.OffsetFactor;
                GLfloat ez = v3->ooz - v1->ooz;
                GLfloat fz = v4->ooz - v2->ooz;
                GLfloat a = ey * fz - ez * fy;
                GLfloat b = ez * fx - ex * fz;
                GLfloat ic = 1.0 / c;
                GLfloat ac = a * ic;
                GLfloat bc = b * ic;
                if (ac < 0.0F)
                    ac = -ac;
                if (bc < 0.0F)
                    bc = -bc;
                offset += MAX2(ac, bc) * factor;
            }
            /* Probably a lot quicker just to nudge the z values and put
             * them back afterwards.
             */
            FX_grDepthBiasLevel(fxMesa, (int) offset);
        }
    }
    else if (IND & FX_FLAT) {
        GLubyte(*color)[4] = VB->Color[0]->data;
        GOURAUD2(v1, color[pv]);
        GOURAUD2(v2, color[pv]);
        GOURAUD2(v3, color[pv]);
        GOURAUD2(v4, color[pv]);
    }

    if (IND & FX_FRONT_BACK) {
        FX_grColorMaskv(ctx, ctx->Color.ColorMask);
        FX_grDepthMask(fxMesa, FXFALSE);
        FX_grRenderBuffer(fxMesa, GR_BUFFER_BACKBUFFER);
    }

    if (IND & FX_ANTIALIAS) {
        FX_grAADrawTriangle(fxMesa, v1, v2, v4, FXTRUE, FXTRUE, FXTRUE);
        FX_grAADrawTriangle(fxMesa, v2, v3, v4, FXTRUE, FXTRUE, FXTRUE);
    }
    else {
        FX_grDrawTriangle(fxMesa, v1, v2, v4);
        FX_grDrawTriangle(fxMesa, v2, v3, v4);
    }

    /* Might be quicker to do two passes, one for each buffer?
     */
    if (IND & FX_FRONT_BACK) {
        FX_grColorMaskv(ctx, ctx->Color.ColorMask);

        if (ctx->Depth.Mask)
            FX_grDepthMask(fxMesa, FXTRUE);

        FX_grRenderBuffer(fxMesa, GR_BUFFER_FRONTBUFFER);

        if (IND & FX_ANTIALIAS) {
            FX_grAADrawTriangle(fxMesa, v1, v2, v4, FXTRUE, FXTRUE, FXTRUE);
            FX_grAADrawTriangle(fxMesa, v2, v3, v4, FXTRUE, FXTRUE, FXTRUE);
        }
        else {
            FX_grDrawTriangle(fxMesa, v1, v2, v4);
            FX_grDrawTriangle(fxMesa, v2, v3, v4);
        }
    }
}

#define DRAW_LINE(fxMesa, tmp0, tmp1, width)		\
  if (width <= 1.0) {					\
    FX_grDrawLine(fxMesa, tmp0, tmp1);			\
  }							\
  else {						\
    GrVertex verts[4];					\
    float dx, dy, ix, iy;				\
							\
    dx = tmp0->x - tmp1->x;				\
    dy = tmp0->y - tmp1->y;				\
							\
    if (dx * dx > dy * dy) {				\
      iy = width * .5;					\
      ix = 0;						\
    }							\
    else {						\
      iy = 0;						\
      ix = width * .5;					\
    }							\
							\
    verts[0] = *tmp0;					\
    verts[1] = *tmp0;					\
    verts[2] = *tmp1;					\
    verts[3] = *tmp1;					\
							\
    verts[0].x = tmp0->x - ix;				\
    verts[0].y = tmp0->y - iy;				\
							\
    verts[1].x = tmp0->x + ix;				\
    verts[1].y = tmp0->y + iy;				\
							\
    verts[2].x = tmp1->x + ix;				\
    verts[2].y = tmp1->y + iy;				\
							\
    verts[3].x = tmp1->x - ix;				\
    verts[3].y = tmp1->y - iy;				\
							\
    FX_grDrawPolygonVertexList(fxMesa, 4, verts);	\
  }


#if (IND & FX_OFFSET) == 0
static void TAG(fx_line) (GLcontext * ctx, GLuint e1, GLuint e2, GLuint pv)
{
    fxMesaContext fxMesa = (fxMesaContext) ctx->DriverCtx;
    struct vertex_buffer *VB = ctx->VB;
    fxVertex *gWin = FX_DRIVER_DATA(VB)->verts;
    GLubyte(*const color)[4] = VB->ColorPtr->data;
    GrVertex *v1 = (GrVertex *) gWin[e1].f;
    GrVertex *v2 = (GrVertex *) gWin[e2].f;
    GLfloat w = ctx->Line.Width * .5;

    if (IND & FX_FLAT) {
        GOURAUD2(v1, color[pv]);
        GOURAUD2(v2, color[pv]);
    }
    else if (IND & FX_TWOSIDE) {
        /* XXX use signed area of the polygon to determine front/back color choice */
        GOURAUD2(v1, color[e1]);
        GOURAUD2(v2, color[e2]);
    }

    if (IND & FX_FRONT_BACK) {
        FX_grColorMaskv(ctx, ctx->Color.ColorMask);
        FX_grDepthMask(fxMesa, FXFALSE);
        FX_grRenderBuffer(fxMesa, GR_BUFFER_BACKBUFFER);
    }

    if (IND & FX_ANTIALIAS)
        FX_grAADrawLine(fxMesa, v1, v2);
    else
        DRAW_LINE(fxMesa, v1, v2, w);

    if (IND & FX_FRONT_BACK) {
        FX_grColorMaskv(ctx, ctx->Color.ColorMask);

        if (ctx->Depth.Mask)
            FX_grDepthMask(fxMesa, FXTRUE);

        FX_grRenderBuffer(fxMesa, GR_BUFFER_FRONTBUFFER);

        if (IND & FX_ANTIALIAS)
            FX_grAADrawLine(fxMesa, v1, v2);
        else
            DRAW_LINE(fxMesa, v1, v2, w);
    }
}
#endif


#if (IND & FX_OFFSET) == 0

/*
 * Draw large points (size > 1) with a polygon.
 */
#define DRAW_POINT(i, radius, color)				\
    do {							\
	GrVertex verts[4], *tmp;				\
	tmp = (GrVertex *) gWin[i].f;				\
        GOURAUD2(tmp, color);                                   \
	verts[0] = *tmp;					\
	verts[1] = *tmp;					\
	verts[2] = *tmp;					\
	verts[3] = *tmp;					\
	verts[0].x = verts[3].x = tmp->x + radius;		\
	verts[0].y = verts[1].y = tmp->y + radius;		\
	verts[2].x = verts[1].x = tmp->x - radius;		\
	verts[2].y = verts[3].y = tmp->y - radius;		\
	FX_grDrawPolygonVertexList(fxMesa, 4, verts);		\
    } while (0)


static void TAG(fx_points) (GLcontext * ctx, GLuint first, GLuint last)
{
    fxMesaContext fxMesa = (fxMesaContext) ctx->DriverCtx;
    const struct vertex_buffer *VB = ctx->VB;
    const fxVertex *gWin = FX_DRIVER_DATA(VB)->verts;
    GLubyte (*color)[4] = VB->ColorPtr->data;
    GLuint i;
    const GLfloat radius = ctx->Point.Size * .5;

    (void) color;
    (void) fxMesa;

    if (IND & FX_FRONT_BACK) {
        FX_grColorMaskv(ctx, ctx->Color.ColorMask);
        FX_grDepthMask(fxMesa, FXFALSE);
        FX_grRenderBuffer(fxMesa, GR_BUFFER_BACKBUFFER);
    }

    if (!VB->ClipOrMask) {
        for (i = first; i <= last; i++) {
            DRAW_POINT(i, radius, color[i]);
        }
    }
    else {
        for (i = first; i <= last; i++) {
            if (VB->ClipMask[i] == 0) {
                DRAW_POINT(i, radius, color[i]);
            }
        }
    }

    if (IND & FX_FRONT_BACK) {
        FX_grColorMaskv(ctx, ctx->Color.ColorMask);
        if (ctx->Depth.Mask)
            FX_grDepthMask(fxMesa, FXTRUE);
        FX_grRenderBuffer(fxMesa, GR_BUFFER_FRONTBUFFER);

        if (!VB->ClipOrMask) {
            for (i = first; i <= last; i++) {
                DRAW_POINT(i, radius, color[i]);
            }
        }
        else {
            for (i = first; i <= last; i++) {
                if (VB->ClipMask[i] == 0) {
                    DRAW_POINT(i, radius, color[i]);
                }
            }
        }
    }
}

#endif



static void TAG(init) (void)
{
    tri_tab[IND] = TAG(fx_tri);
    quad_tab[IND] = TAG(fx_quad);

#if ((IND & FX_OFFSET) == 0)
    line_tab[IND] = TAG(fx_line);
    points_tab[IND] = TAG(fx_points);
#else
    line_tab[IND] = line_tab[IND & ~FX_OFFSET];
    points_tab[IND] = points_tab[IND & ~FX_OFFSET];
#endif
}

#undef IND
#undef TAG
#undef FLAT_COLOR
#undef DRAW_POINT
