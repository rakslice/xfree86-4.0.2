/* $XFree86: xc/lib/GL/mesa/src/drv/tdfx/fxsetup.c,v 1.2 2000/12/08 19:36:23 alanh Exp $ */
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


/* fxsetup.c - 3Dfx VooDoo rendering mode setup functions */

#include "fxdrv.h"
#include "fxddtex.h"
#include "fxtexman.h"
#include "fxsetup.h"
#include "enums.h"


static GLboolean fxMultipassTexture(struct vertex_buffer *, GLuint);



void
fxTexValidate(GLcontext * ctx, struct gl_texture_object *tObj)
{
    fxMesaContext fxMesa = FX_CONTEXT(ctx);
    tfxTexInfo *ti = fxTMGetTexInfo(tObj);
    GLint minl, maxl;

    if (MESA_VERBOSE & VERBOSE_DRIVER) {
        fprintf(stderr, "fxmesa: fxTexValidate(...) Start\n");
    }

    if (ti->validated) {
        if (MESA_VERBOSE & VERBOSE_DRIVER) {
            fprintf(stderr,
                    "fxmesa: fxTexValidate(...) End (validated=GL_TRUE)\n");
        }
        return;
    }

    ti->tObj = tObj;
    minl = ti->minLevel = tObj->BaseLevel;
    maxl = ti->maxLevel = MIN2(tObj->MaxLevel, tObj->Image[0]->MaxLog2);

    fxTexGetInfo(ctx, tObj->Image[minl]->Width, tObj->Image[minl]->Height,
                 &(FX_largeLodLog2(ti->info)),
                 &(FX_aspectRatioLog2(ti->info)), &(ti->sScale),
                 &(ti->tScale), &(ti->int_sScale), &(ti->int_tScale), NULL,
                 NULL);

    if ((tObj->MinFilter != GL_NEAREST) && (tObj->MinFilter != GL_LINEAR))
        fxTexGetInfo(ctx, tObj->Image[maxl]->Width, tObj->Image[maxl]->Height,
                     &(FX_smallLodLog2(ti->info)), NULL,
                     NULL, NULL, NULL, NULL, NULL, NULL);
    else
        FX_smallLodLog2(ti->info) = FX_largeLodLog2(ti->info);

    fxTexGetFormat(tObj->Image[minl]->IntFormat, &(ti->info.format),
                   &(ti->baseLevelInternalFormat), NULL, NULL,
                   fxMesa->haveHwStencil);

    switch (tObj->WrapS) {
    case GL_CLAMP_TO_EDGE:
    case GL_CLAMP:
        ti->sClamp = 1;
        break;
    case GL_REPEAT:
        ti->sClamp = 0;
        break;
    default:
        ;                       /* silence compiler warning */
    }
    switch (tObj->WrapT) {
    case GL_CLAMP_TO_EDGE:
    case GL_CLAMP:
        ti->tClamp = 1;
        break;
    case GL_REPEAT:
        ti->tClamp = 0;
        break;
    default:
        ;                       /* silence compiler warning */
    }

    ti->validated = GL_TRUE;

    ti->info.data = NULL;

    if (MESA_VERBOSE & VERBOSE_DRIVER) {
        fprintf(stderr, "fxmesa: fxTexValidate(...) End\n");
    }
}

static void
fxPrintUnitsMode(const char *msg, GLuint mode)
{
    fprintf(stderr,
            "%s: (0x%x) %s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s\n",
            msg,
            mode,
            (mode & FX_UM_E0_REPLACE) ? "E0_REPLACE, " : "",
            (mode & FX_UM_E0_MODULATE) ? "E0_MODULATE, " : "",
            (mode & FX_UM_E0_DECAL) ? "E0_DECAL, " : "",
            (mode & FX_UM_E0_BLEND) ? "E0_BLEND, " : "",
            (mode & FX_UM_E1_REPLACE) ? "E1_REPLACE, " : "",
            (mode & FX_UM_E1_MODULATE) ? "E1_MODULATE, " : "",
            (mode & FX_UM_E1_DECAL) ? "E1_DECAL, " : "",
            (mode & FX_UM_E1_BLEND) ? "E1_BLEND, " : "",
            (mode & FX_UM_E0_ALPHA) ? "E0_ALPHA, " : "",
            (mode & FX_UM_E0_LUMINANCE) ? "E0_LUMINANCE, " : "",
            (mode & FX_UM_E0_LUMINANCE_ALPHA) ? "E0_LUMINANCE_ALPHA, " : "",
            (mode & FX_UM_E0_INTENSITY) ? "E0_INTENSITY, " : "",
            (mode & FX_UM_E0_RGB) ? "E0_RGB, " : "",
            (mode & FX_UM_E0_RGBA) ? "E0_RGBA, " : "",
            (mode & FX_UM_E1_ALPHA) ? "E1_ALPHA, " : "",
            (mode & FX_UM_E1_LUMINANCE) ? "E1_LUMINANCE, " : "",
            (mode & FX_UM_E1_LUMINANCE_ALPHA) ? "E1_LUMINANCE_ALPHA, " : "",
            (mode & FX_UM_E1_INTENSITY) ? "E1_INTENSITY, " : "",
            (mode & FX_UM_E1_RGB) ? "E1_RGB, " : "",
            (mode & FX_UM_E1_RGBA) ? "E1_RGBA, " : "",
            (mode & FX_UM_COLOR_ITERATED) ? "COLOR_ITERATED, " : "",
            (mode & FX_UM_COLOR_CONSTANT) ? "COLOR_CONSTANT, " : "",
            (mode & FX_UM_ALPHA_ITERATED) ? "ALPHA_ITERATED, " : "",
            (mode & FX_UM_ALPHA_CONSTANT) ? "ALPHA_CONSTANT, " : "");
}

static GLuint
fxGetTexSetConfiguration(GLcontext * ctx,
                         struct gl_texture_object *tObj0,
                         struct gl_texture_object *tObj1)
{
    GLuint unitsmode = 0;
    GLuint envmode = 0;
    GLuint ifmt = 0;

    if ((ctx->Light.ShadeModel == GL_SMOOTH) || 1 ||
        (ctx->Point.SmoothFlag) ||
        (ctx->Line.SmoothFlag) ||
        (ctx->Polygon.SmoothFlag))
        unitsmode |= FX_UM_ALPHA_ITERATED;
    else
        unitsmode |= FX_UM_ALPHA_CONSTANT;

    if (ctx->Light.ShadeModel == GL_SMOOTH || 1)
        unitsmode |= FX_UM_COLOR_ITERATED;
    else
        unitsmode |= FX_UM_COLOR_CONSTANT;



    /* 
       OpenGL Feeds Texture 0 into Texture 1
       Glide Feeds Texture 1 into Texture 0
     */
    if (tObj0) {
        tfxTexInfo *ti0 = fxTMGetTexInfo(tObj0);

        switch (ti0->baseLevelInternalFormat) {
        case GL_ALPHA:
            ifmt |= FX_UM_E0_ALPHA;
            break;
        case GL_LUMINANCE:
            ifmt |= FX_UM_E0_LUMINANCE;
            break;
        case GL_LUMINANCE_ALPHA:
            ifmt |= FX_UM_E0_LUMINANCE_ALPHA;
            break;
        case GL_INTENSITY:
            ifmt |= FX_UM_E0_INTENSITY;
            break;
        case GL_RGB:
            ifmt |= FX_UM_E0_RGB;
            break;
        case GL_RGBA:
            ifmt |= FX_UM_E0_RGBA;
            break;
        }

        switch (ctx->Texture.Unit[0].EnvMode) {
        case GL_DECAL:
            envmode |= FX_UM_E0_DECAL;
            break;
        case GL_MODULATE:
            envmode |= FX_UM_E0_MODULATE;
            break;
        case GL_REPLACE:
            envmode |= FX_UM_E0_REPLACE;
            break;
        case GL_BLEND:
            envmode |= FX_UM_E0_BLEND;
            break;
        case GL_ADD:
            envmode |= FX_UM_E0_ADD;
            break;
        default:
            /* do nothing */
            break;
        }
    }

    if (tObj1) {
        tfxTexInfo *ti1 = fxTMGetTexInfo(tObj1);

        switch (ti1->baseLevelInternalFormat) {
        case GL_ALPHA:
            ifmt |= FX_UM_E1_ALPHA;
            break;
        case GL_LUMINANCE:
            ifmt |= FX_UM_E1_LUMINANCE;
            break;
        case GL_LUMINANCE_ALPHA:
            ifmt |= FX_UM_E1_LUMINANCE_ALPHA;
            break;
        case GL_INTENSITY:
            ifmt |= FX_UM_E1_INTENSITY;
            break;
        case GL_RGB:
            ifmt |= FX_UM_E1_RGB;
            break;
        case GL_RGBA:
            ifmt |= FX_UM_E1_RGBA;
            break;
        default:
            /* do nothing */
            break;
        }

        switch (ctx->Texture.Unit[1].EnvMode) {
        case GL_DECAL:
            envmode |= FX_UM_E1_DECAL;
            break;
        case GL_MODULATE:
            envmode |= FX_UM_E1_MODULATE;
            break;
        case GL_REPLACE:
            envmode |= FX_UM_E1_REPLACE;
            break;
        case GL_BLEND:
            envmode |= FX_UM_E1_BLEND;
            break;
        case GL_ADD:
            envmode |= FX_UM_E1_ADD;
            break;
        default:
            /* do nothing */
            break;
        }
    }

    unitsmode |= (ifmt | envmode);

    if (MESA_VERBOSE & (VERBOSE_DRIVER | VERBOSE_TEXTURE))
        fxPrintUnitsMode("unitsmode", unitsmode);

    return unitsmode;
}

/************************************************************************/
/************************* Rendering Mode SetUp *************************/
/************************************************************************/

/************************* Single Texture Set ***************************/

static void
fxSetupSingleTMU_NoLock(fxMesaContext fxMesa, struct gl_texture_object *tObj)
{
    struct TdfxSharedState *shared = (struct TdfxSharedState *) fxMesa->glCtx->Shared->DriverData;
    tfxTexInfo *ti = fxTMGetTexInfo(tObj);
    const GLcontext *ctx = fxMesa->glCtx;

    /* Make sure we're not loaded incorrectly */
    if (ti->isInTM && !shared->umaTexMemory) {
        /* if doing filtering between mipmap levels, alternate mipmap levels
         * must be in alternate TMUs.
         */
        if (ti->LODblend) {
            if (ti->whichTMU != FX_TMU_SPLIT)
                fxTMMoveOutTM(fxMesa, tObj);
        }
        else {
            if (ti->whichTMU == FX_TMU_SPLIT)
                fxTMMoveOutTM(fxMesa, tObj);
        }
    }

    /* Make sure we're loaded correctly */
    if (!ti->isInTM) {
        /* Have to download the texture */
        if (shared->umaTexMemory) {
           fxTMMoveInTM_NoLock(fxMesa, tObj, FX_TMU0);
        }
        else {
           /* Voodoo3 (split texture memory) */
           if (ti->LODblend) {
               fxTMMoveInTM_NoLock(fxMesa, tObj, FX_TMU_SPLIT);
           }
           else {
               if (fxMesa->haveTwoTMUs) {
                   GLint memReq = FX_grTexTextureMemRequired_NoLock(
                                         GR_MIPMAPLEVELMASK_BOTH, &(ti->info));
                   if (shared->freeTexMem[FX_TMU0] > memReq) {
                       fxTMMoveInTM_NoLock(fxMesa, tObj, FX_TMU0);
                   }
                   else {
                       fxTMMoveInTM_NoLock(fxMesa, tObj, FX_TMU1);
                   }
               }
               else
                   fxTMMoveInTM_NoLock(fxMesa, tObj, FX_TMU0);
           }
        }
    }

    if (ti->LODblend && ti->whichTMU == FX_TMU_SPLIT) {
        /* mipmap levels split between texture banks */
        if (ti->info.format == GR_TEXFMT_P_8 && !ctx->Texture.SharedPalette) {
            if (MESA_VERBOSE & VERBOSE_DRIVER) {
                fprintf(stderr, "fxmesa: uploading texture palette\n");
            }
            FX_grTexDownloadTable_NoLock(GR_TMU0, GR_TEXTABLE_PALETTE_6666_EXT,
                                         &(ti->palette));
            FX_grTexDownloadTable_NoLock(GR_TMU1, GR_TEXTABLE_PALETTE_6666_EXT,
                                         &(ti->palette));
        }

        FX_grTexClampMode_NoLock(GR_TMU0, ti->sClamp, ti->tClamp);
        FX_grTexClampMode_NoLock(GR_TMU1, ti->sClamp, ti->tClamp);
        FX_grTexFilterMode_NoLock(GR_TMU0, ti->minFilt, ti->maxFilt);
        FX_grTexFilterMode_NoLock(GR_TMU1, ti->minFilt, ti->maxFilt);
        FX_grTexMipMapMode_NoLock(GR_TMU0, ti->mmMode, ti->LODblend);
        FX_grTexMipMapMode_NoLock(GR_TMU1, ti->mmMode, ti->LODblend);

        FX_grTexSource_NoLock(GR_TMU0, ti->tm[FX_TMU0]->startAddr,
                              GR_MIPMAPLEVELMASK_ODD, &(ti->info));
        FX_grTexSource_NoLock(GR_TMU1, ti->tm[FX_TMU1]->startAddr,
                              GR_MIPMAPLEVELMASK_EVEN, &(ti->info));
    }
    else {
        FxU32 tmu;

        if (ti->whichTMU == FX_TMU_BOTH)
            tmu = FX_TMU0;
        else
            tmu = ti->whichTMU;

        if (shared->umaTexMemory) {
            assert(ti->whichTMU == FX_TMU0);
            assert(tmu == FX_TMU0);
        }

        if (ti->info.format == GR_TEXFMT_P_8 && !ctx->Texture.SharedPalette) {
            if (MESA_VERBOSE & VERBOSE_DRIVER) {
                fprintf(stderr, "fxmesa: uploading texture palette\n");
            }
            FX_grTexDownloadTable_NoLock(tmu, GR_TEXTABLE_PALETTE_6666_EXT,
                                         &(ti->palette));
        }

        /* KW: The alternative is to do the download to the other tmu.  If
         * we get to this point, I think it means we are thrashing the
         * texture memory, so perhaps it's not a good idea.  
         */
        if (ti->LODblend && (MESA_VERBOSE & VERBOSE_DRIVER))
            fprintf(stderr,
                    "fxmesa: not blending texture - only on one tmu\n");

        FX_grTexClampMode_NoLock(tmu, ti->sClamp, ti->tClamp);
        FX_grTexFilterMode_NoLock(tmu, ti->minFilt, ti->maxFilt);
        FX_grTexMipMapMode_NoLock(tmu, ti->mmMode, FXFALSE);

        if (ti->tm[tmu]) {
           FX_grTexSource_NoLock(tmu, ti->tm[tmu]->startAddr,
                                 GR_MIPMAPLEVELMASK_BOTH, &(ti->info));
        }
    }
}

static void
fxSelectSingleTMUSrc_NoLock(fxMesaContext fxMesa, GLint tmu, FxBool LODblend)
{
    if (MESA_VERBOSE & VERBOSE_DRIVER) {
        fprintf(stderr, "fxmesa: fxSelectSingleTMUSrc(%d,%d)\n", tmu,
                LODblend);
    }

    if (LODblend) {
        FX_grTexCombine_NoLock(GR_TMU0,
                               GR_COMBINE_FUNCTION_BLEND,
                               GR_COMBINE_FACTOR_ONE_MINUS_LOD_FRACTION,
                               GR_COMBINE_FUNCTION_BLEND,
                               GR_COMBINE_FACTOR_ONE_MINUS_LOD_FRACTION,
                               FXFALSE, FXFALSE);

        if (fxMesa->haveTwoTMUs) {
            const struct gl_shared_state *mesaShared = fxMesa->glCtx->Shared;
            const struct TdfxSharedState *shared = (struct TdfxSharedState *) mesaShared->DriverData;
            int tmu;

            if (shared->umaTexMemory)
                tmu = GR_TMU0;
            else
                tmu = GR_TMU1;

            FX_grTexCombine_NoLock(tmu,
                                   GR_COMBINE_FUNCTION_LOCAL,
                                   GR_COMBINE_FACTOR_NONE,
                                   GR_COMBINE_FUNCTION_LOCAL,
                                   GR_COMBINE_FACTOR_NONE, FXFALSE, FXFALSE);
        }
        fxMesa->tmuSrc = FX_TMU_SPLIT;
    }
    else {
        if (tmu != FX_TMU1) {
            FX_grTexCombine_NoLock(GR_TMU0,
                                   GR_COMBINE_FUNCTION_LOCAL,
                                   GR_COMBINE_FACTOR_NONE,
                                   GR_COMBINE_FUNCTION_LOCAL,
                                   GR_COMBINE_FACTOR_NONE, FXFALSE, FXFALSE);
            if (fxMesa->haveTwoTMUs) {
                FX_grTexCombine_NoLock(GR_TMU1,
                                       GR_COMBINE_FUNCTION_ZERO,
                                       GR_COMBINE_FACTOR_NONE,
                                       GR_COMBINE_FUNCTION_ZERO,
                                       GR_COMBINE_FACTOR_NONE, FXFALSE,
                                       FXFALSE);
            }
            fxMesa->tmuSrc = FX_TMU0;
        }
        else {
            FX_grTexCombine_NoLock(GR_TMU1,
                                   GR_COMBINE_FUNCTION_LOCAL,
                                   GR_COMBINE_FACTOR_NONE,
                                   GR_COMBINE_FUNCTION_LOCAL,
                                   GR_COMBINE_FACTOR_NONE, FXFALSE, FXFALSE);

            /* GR_COMBINE_FUNCTION_SCALE_OTHER doesn't work ?!? */

            FX_grTexCombine_NoLock(GR_TMU0,
                                   GR_COMBINE_FUNCTION_BLEND,
                                   GR_COMBINE_FACTOR_ONE,
                                   GR_COMBINE_FUNCTION_BLEND,
                                   GR_COMBINE_FACTOR_ONE, FXFALSE, FXFALSE);

            fxMesa->tmuSrc = FX_TMU1;
        }
    }
}


/*
 * Setup the texture env mode for a texture unit on Banshee/Voodoo3
 */
static void
SetupTexEnvVoodoo3(GLcontext *ctx, FxU32 unit, GLboolean iteratedRGBA,
                   GLenum envMode, GLenum baseFormat)
{
    GrCombineLocal_t localc, locala;
    if (iteratedRGBA)
       localc = locala = GR_COMBINE_LOCAL_ITERATED;
    else
       localc = locala = GR_COMBINE_LOCAL_CONSTANT;

    switch (envMode) {
    case GL_DECAL:
        FX_grAlphaCombine_NoLock(GR_COMBINE_FUNCTION_LOCAL,
                                 GR_COMBINE_FACTOR_NONE,
                                 locala, GR_COMBINE_OTHER_NONE, FXFALSE);

        FX_grColorCombine_NoLock(GR_COMBINE_FUNCTION_BLEND,
                                 GR_COMBINE_FACTOR_TEXTURE_ALPHA,
                                 localc, GR_COMBINE_OTHER_TEXTURE, FXFALSE);
        break;
    case GL_MODULATE:
        FX_grAlphaCombine_NoLock(GR_COMBINE_FUNCTION_SCALE_OTHER,
                                 GR_COMBINE_FACTOR_LOCAL,
                                 locala, GR_COMBINE_OTHER_TEXTURE, FXFALSE);

        if (baseFormat == GL_ALPHA)
            FX_grColorCombine_NoLock(GR_COMBINE_FUNCTION_LOCAL,
                                     GR_COMBINE_FACTOR_NONE,
                                     localc, GR_COMBINE_OTHER_NONE, FXFALSE);
        else
            FX_grColorCombine_NoLock(GR_COMBINE_FUNCTION_SCALE_OTHER,
                                     GR_COMBINE_FACTOR_LOCAL,
                                     localc, GR_COMBINE_OTHER_TEXTURE,
                                     FXFALSE);
        break;
    case GL_BLEND:
#if 0
        FX_grAlphaCombine_NoLock(GR_COMBINE_FUNCTION_SCALE_OTHER,
                                 GR_COMBINE_FACTOR_LOCAL,
                                 locala, GR_COMBINE_OTHER_TEXTURE, FXFALSE);
        if (baseFormat == GL_ALPHA)
            FX_grColorCombine_NoLock(GR_COMBINE_FUNCTION_LOCAL,
                                     GR_COMBINE_FACTOR_NONE,
                                     localc, GR_COMBINE_OTHER_NONE, FXFALSE);
        else
            FX_grColorCombine_NoLock
                (GR_COMBINE_FUNCTION_SCALE_OTHER_ADD_LOCAL,
                 GR_COMBINE_FACTOR_LOCAL, localc, GR_COMBINE_OTHER_TEXTURE,
                 FXTRUE);
        ctx->Driver.MultipassFunc = fxMultipassBlend;
#else
        if (MESA_VERBOSE & VERBOSE_DRIVER)
            fprintf(stderr, "fx Driver: GL_BLEND not yet supported\n");
#endif
        break;
    case GL_REPLACE:
        if ((baseFormat == GL_RGB) || (baseFormat == GL_LUMINANCE))
            FX_grAlphaCombine_NoLock(GR_COMBINE_FUNCTION_LOCAL,
                                     GR_COMBINE_FACTOR_NONE,
                                     locala, GR_COMBINE_OTHER_NONE, FXFALSE);
        else
            FX_grAlphaCombine_NoLock(GR_COMBINE_FUNCTION_SCALE_OTHER,
                                     GR_COMBINE_FACTOR_ONE,
                                     locala, GR_COMBINE_OTHER_TEXTURE,
                                     FXFALSE);

        if (baseFormat == GL_ALPHA)
            FX_grColorCombine_NoLock(GR_COMBINE_FUNCTION_LOCAL,
                                     GR_COMBINE_FACTOR_NONE,
                                     localc, GR_COMBINE_OTHER_NONE, FXFALSE);
        else
            FX_grColorCombine_NoLock(GR_COMBINE_FUNCTION_SCALE_OTHER,
                                     GR_COMBINE_FACTOR_ONE,
                                     localc, GR_COMBINE_OTHER_TEXTURE,
                                     FXFALSE);
        break;
    case GL_ADD:
        if (baseFormat == GL_ALPHA ||
            baseFormat == GL_LUMINANCE_ALPHA ||
            baseFormat == GL_RGBA) {
            /* product of texel and fragment alpha */
            FX_grAlphaCombine_NoLock(GR_COMBINE_FUNCTION_SCALE_OTHER,
                                     GR_COMBINE_FACTOR_LOCAL,
                                     locala, GR_COMBINE_OTHER_TEXTURE, FXFALSE);
        }
        else if (baseFormat == GL_LUMINANCE || baseFormat == GL_RGB) {
            /* fragment alpha is unchanged */
            FX_grAlphaCombine_NoLock(GR_COMBINE_FUNCTION_LOCAL,
                                     GR_COMBINE_FACTOR_NONE,
                                     locala, GR_COMBINE_OTHER_NONE, FXFALSE);
        }
        else {
            ASSERT(baseFormat == GL_INTENSITY);
            /* sum of texel and fragment alpha */
            FX_grAlphaCombine_NoLock(GR_COMBINE_FUNCTION_SCALE_OTHER_ADD_LOCAL,
                                     GR_COMBINE_FACTOR_ONE,
                                     locala, GR_COMBINE_OTHER_TEXTURE,
                                     FXFALSE);
        }
        if (baseFormat == GL_ALPHA) {
            /* rgb unchanged */
            FX_grColorCombine_NoLock(GR_COMBINE_FUNCTION_LOCAL,
                                     GR_COMBINE_FACTOR_NONE,
                                     localc, GR_COMBINE_OTHER_NONE, FXFALSE);
        }
        else {
            /* sum of texel and fragment rgb */
            FX_grColorCombine_NoLock(GR_COMBINE_FUNCTION_SCALE_OTHER_ADD_LOCAL,
                                     GR_COMBINE_FACTOR_ONE,
                                     localc, GR_COMBINE_OTHER_TEXTURE,
                                     FXFALSE);
        }
        break;
    default:
        gl_problem(ctx, "Bad baseFormat in SetupTexEnvVoodoo3");
    }
}


/*
 * These macros are used below when handling COMBINE_EXT.
 */
#define TEXENV_OPERAND_INVERTED(operand)                            \
  (((operand) == GL_ONE_MINUS_SRC_ALPHA)                            \
   || ((operand) == GL_ONE_MINUS_SRC_COLOR))
#define TEXENV_OPERAND_ALPHA(operand)                               \
  (((operand) == GL_SRC_ALPHA) || ((operand) == GL_ONE_MINUS_SRC_ALPHA))
#define TEXENV_SETUP_ARG_A(param, source, operand, iteratedAlpha)   \
    switch (source) {                                               \
    case GL_TEXTURE:                                                \
        param = GR_CMBX_LOCAL_TEXTURE_ALPHA;                        \
        break;                                                      \
    case GL_CONSTANT_EXT:                                           \
        param = GR_CMBX_TMU_CALPHA;                                 \
        break;                                                      \
    case GL_PRIMARY_COLOR_EXT:                                      \
        param = GR_CMBX_ITALPHA;                                    \
        break;                                                      \
    case GL_PREVIOUS_EXT:                                           \
        param = iteratedAlpha;                                      \
        break;                                                      \
    default:                                                        \
       /*                                                           \
        * This is here just to keep from getting                    \
        * compiler warnings.                                        \
        */                                                          \
        param = GR_CMBX_ZERO;                                       \
        break;                                                      \
    }

#define TEXENV_SETUP_ARG_RGB(param, source, operand, iteratedColor, iteratedAlpha) \
    if (!TEXENV_OPERAND_ALPHA(operand)) {                           \
        switch (source) {                                           \
        case GL_TEXTURE:                                            \
            param = GR_CMBX_LOCAL_TEXTURE_RGB;                      \
            break;                                                  \
        case GL_CONSTANT_EXT:                                       \
            param = GR_CMBX_TMU_CCOLOR;                             \
            break;                                                  \
        case GL_PRIMARY_COLOR_EXT:                                  \
            param = GR_CMBX_ITRGB;                                  \
            break;                                                  \
        case GL_PREVIOUS_EXT:                                       \
            param = iteratedColor;                                  \
            break;                                                  \
        default:                                                    \
           /*                                                       \
            * This is here just to keep from getting                \
            * compiler warnings.                                    \
            */                                                      \
            param = GR_CMBX_ZERO;                                   \
            break;                                                  \
        }                                                           \
    } else {                                                        \
        switch (source) {                                           \
        case GL_TEXTURE:                                            \
            param = GR_CMBX_LOCAL_TEXTURE_ALPHA;                    \
            break;                                                  \
        case GL_CONSTANT_EXT:                                       \
            param = GR_CMBX_TMU_CALPHA;                             \
            break;                                                  \
        case GL_PRIMARY_COLOR_EXT:                                  \
            param = GR_CMBX_ITALPHA;                                \
            break;                                                  \
        case GL_PREVIOUS_EXT:                                       \
            param = iteratedAlpha;                                  \
            break;                                                  \
        default:                                                    \
           /*                                                       \
            * This is here just to keep from getting                \
            * compiler warnings.                                    \
            */                                                      \
            param = GR_CMBX_ZERO;                                   \
            break;                                                  \
        }                                                           \
    }

#define TEXENV_SETUP_MODE_RGB(param, operand)                       \
    switch (operand) {                                              \
    case GL_SRC_COLOR:                                              \
    case GL_SRC_ALPHA:                                              \
        param = GR_FUNC_MODE_X;                                     \
        break;                                                      \
    case GL_ONE_MINUS_SRC_ALPHA:                                    \
    case GL_ONE_MINUS_SRC_COLOR:                                    \
        param = GR_FUNC_MODE_ONE_MINUS_X;                           \
        break;                                                      \
    default:                                                        \
        param = GR_FUNC_MODE_ZERO;                                  \
        break;                                                      \
    }

#define TEXENV_SETUP_MODE_A(param, operand)                         \
    switch (operand) {                                              \
    case GL_SRC_ALPHA:                                              \
        param = GR_FUNC_MODE_X;                                     \
        break;                                                      \
    case GL_ONE_MINUS_SRC_ALPHA:                                    \
        param = GR_FUNC_MODE_ONE_MINUS_X;                           \
        break;                                                      \
    default:                                                        \
        param = GR_FUNC_MODE_ZERO;                                  \
        break;                                                      \
    }

/*
 * Setup the texture env mode for a texture unit on Napalm.
 * If useIteratedRGBA is true, we'll feed the interpolated fragment
 * color into the combiner, else we'll feed in the upstream texture
 * unit's resultant color.
 */
static void
SetupTexEnvNapalm(GLcontext *ctx, FxU32 unit, GLboolean useIteratedRGBA,
                  struct gl_texture_unit *texUnit, GLenum baseFormat)
{
    GrTCCUColor_t incomingRGB, incomingAlpha;
    GLenum envMode = texUnit->EnvMode;

    if (useIteratedRGBA) {
        incomingRGB = GR_CMBX_ITRGB;
        incomingAlpha = GR_CMBX_ITALPHA;
    }
    else {
        incomingRGB = GR_CMBX_OTHER_TEXTURE_RGB;
        incomingAlpha = GR_CMBX_OTHER_TEXTURE_ALPHA;
    }

    switch (envMode) {
    case GL_REPLACE:
        /* Setup RGB combiner */
        if (baseFormat == GL_ALPHA) {
            /* Rv = Rf */
             (*grTexColorCombineExtPtr)(unit,
                                        incomingRGB, GR_FUNC_MODE_X,
                                        GR_CMBX_ZERO, GR_FUNC_MODE_ZERO,
                                        GR_CMBX_ZERO, FXTRUE,
                                        GR_CMBX_ZERO, FXFALSE,
                                        0, FXFALSE);
        }
        else {
            /* Rv = Rt */
            (*grTexColorCombineExtPtr)(unit,
                                     GR_CMBX_LOCAL_TEXTURE_RGB, GR_FUNC_MODE_X,
                                     GR_CMBX_ZERO, GR_FUNC_MODE_X,
                                     GR_CMBX_ZERO, FXTRUE,
                                     GR_CMBX_ZERO, FXFALSE,
                                     0, FXFALSE);
        }
        /* Setup Alpha combiner */
        if (baseFormat == GL_LUMINANCE || baseFormat == GL_RGB) {
            /* Av = Af */
            (*grTexAlphaCombineExtPtr)(unit,
                                       GR_CMBX_ITALPHA, GR_FUNC_MODE_ZERO,
                                       GR_CMBX_ITALPHA, GR_FUNC_MODE_ZERO,
                                       GR_CMBX_ZERO, FXFALSE,
                                       incomingAlpha, FXFALSE,
                                       0, FXFALSE);
        }
        else {
            /* Av = At */
            (*grTexAlphaCombineExtPtr)(unit,
                                       GR_CMBX_ITALPHA, GR_FUNC_MODE_ZERO,
                                       GR_CMBX_ITALPHA, GR_FUNC_MODE_ZERO,
                                       GR_CMBX_ZERO, FXFALSE,
                                       GR_CMBX_LOCAL_TEXTURE_ALPHA, FXFALSE,
                                       0, FXFALSE);
        }
        break;
    case GL_MODULATE:
        /* Setup RGB combiner */
        if (baseFormat == GL_ALPHA) {
            /* Rv = Rf */
            (*grTexColorCombineExtPtr)(unit,
                                       incomingRGB, GR_FUNC_MODE_X,
                                       GR_CMBX_ZERO, GR_FUNC_MODE_ZERO,
                                       GR_CMBX_ZERO, FXTRUE,
                                       GR_CMBX_ZERO, FXFALSE,
                                       0, FXFALSE);
        }
        else {
            /* Result = Frag * Tex */
            (*grTexColorCombineExtPtr)(unit,
                                       incomingRGB, GR_FUNC_MODE_X,
                                       GR_CMBX_ZERO, GR_FUNC_MODE_ZERO,
                                       GR_CMBX_LOCAL_TEXTURE_RGB, FXFALSE,
                                       GR_CMBX_ZERO, FXFALSE,
                                       0, FXFALSE);
        }
        /* Setup Alpha combiner */
        if (baseFormat == GL_LUMINANCE || baseFormat == GL_RGB) {
            /* Av = Af */
            (*grTexAlphaCombineExtPtr)(unit,
                                       incomingAlpha, GR_FUNC_MODE_X,
                                       GR_CMBX_ITALPHA, GR_FUNC_MODE_ZERO,
                                       GR_CMBX_ZERO, FXTRUE,
                                       GR_CMBX_ZERO, FXFALSE,
                                       0, FXFALSE);
        }
        else {
            /* Av = Af * At */
            (*grTexAlphaCombineExtPtr)(unit,
                                   GR_CMBX_LOCAL_TEXTURE_ALPHA, GR_FUNC_MODE_X,
                                   GR_CMBX_ITALPHA, GR_FUNC_MODE_ZERO,
                                   incomingAlpha, FXFALSE,
                                   GR_CMBX_ZERO, FXFALSE,
                                   0, FXFALSE);
        }
        break;
    case GL_DECAL:
        /* Setup RGB combiner */
        if (baseFormat == GL_RGB) {
            /* Rv = Rt */
            (*grTexColorCombineExtPtr)(unit,
                                     GR_CMBX_LOCAL_TEXTURE_RGB, GR_FUNC_MODE_X,
                                     GR_CMBX_ZERO, GR_FUNC_MODE_X,
                                     GR_CMBX_ZERO, FXTRUE,
                                     GR_CMBX_ZERO, FXFALSE,
                                     0, FXFALSE);
        }
        else {
            /* Rv = Rf * (1 - At) + Rt * At */
            (*grTexColorCombineExtPtr)(unit,
                                    GR_CMBX_LOCAL_TEXTURE_RGB, GR_FUNC_MODE_X,
                                    incomingRGB, GR_FUNC_MODE_NEGATIVE_X,
                                    GR_CMBX_LOCAL_TEXTURE_ALPHA, FXFALSE,
                                    GR_CMBX_B, FXFALSE,
                                    0, FXFALSE);
        }
        /* Setup Alpha combiner */
        /* Av = Af */
        (*grTexAlphaCombineExtPtr)(unit,
                                   incomingAlpha, GR_FUNC_MODE_X,
                                   GR_CMBX_ITALPHA, GR_FUNC_MODE_ZERO,
                                   GR_CMBX_ZERO, FXTRUE,
                                   GR_CMBX_ZERO, FXFALSE,
                                   0, FXFALSE);
        break;
    case GL_BLEND:
        /* Setup RGB combiner */
        if (baseFormat == GL_ALPHA) {
            /* Rv = Rf */
            (*grTexColorCombineExtPtr)(unit,
                                       incomingRGB, GR_FUNC_MODE_X,
                                       GR_CMBX_ZERO, GR_FUNC_MODE_ZERO,
                                       GR_CMBX_ZERO, FXTRUE,
                                       GR_CMBX_ZERO, FXFALSE,
                                       0, FXFALSE);
        }
        else {
            /* Rv = Rf * (1 - Rt) + Rc * Rt */
            (*grTexColorCombineExtPtr)(unit,
                                       GR_CMBX_TMU_CCOLOR, GR_FUNC_MODE_X,
                                       incomingRGB, GR_FUNC_MODE_NEGATIVE_X,
                                       GR_CMBX_LOCAL_TEXTURE_RGB, FXFALSE,
                                       GR_CMBX_B, FXFALSE,
                                       0, FXFALSE);
        }
        /* Setup Alpha combiner */
        if (baseFormat == GL_LUMINANCE || baseFormat == GL_RGB) {
            /* Av = Af */
            (*grTexAlphaCombineExtPtr)(unit,
                                       incomingAlpha, GR_FUNC_MODE_X,
                                       GR_CMBX_ZERO, GR_FUNC_MODE_ZERO,
                                       GR_CMBX_ZERO, FXTRUE,
                                       GR_CMBX_ZERO, FXFALSE,
                                       0, FXFALSE);
        }
        else if (baseFormat == GL_INTENSITY) {
            /* Av = Af * (1 - It) + Ac * It */
            (*grTexAlphaCombineExtPtr)(unit,
                                      GR_CMBX_TMU_CALPHA, GR_FUNC_MODE_X,
                                      incomingAlpha, GR_FUNC_MODE_NEGATIVE_X,
                                      GR_CMBX_LOCAL_TEXTURE_ALPHA, FXFALSE,
                                      GR_CMBX_B, FXFALSE,
                                      0, FXFALSE);
        }
        else {
            /* Av = Af * At */
            (*grTexAlphaCombineExtPtr)(unit,
                                   GR_CMBX_LOCAL_TEXTURE_ALPHA, GR_FUNC_MODE_X,
                                   GR_CMBX_ITALPHA, GR_FUNC_MODE_ZERO,
                                   incomingAlpha, FXFALSE,
                                   GR_CMBX_ZERO, FXFALSE,
                                   0, FXFALSE);
        }
        /* Also have to set up the tex env constant color */
        {
            GrColor_t constColor;
            GLubyte *abgr = (GLubyte *) &constColor;
            abgr[0] = ctx->Texture.Unit[0].EnvColor[0] * 255.0F;
            abgr[1] = ctx->Texture.Unit[0].EnvColor[1] * 255.0F;
            abgr[2] = ctx->Texture.Unit[0].EnvColor[2] * 255.0F;
            abgr[3] = ctx->Texture.Unit[0].EnvColor[3] * 255.0F;
            (*grConstantColorValueExtPtr)(unit, constColor);
        }
        break;
    case GL_ADD:
        /* Setup RGB combiner */
        if (baseFormat == GL_ALPHA) {
            /* Rv = Rf */
            (*grTexColorCombineExtPtr)(unit,
                                       incomingRGB, GR_FUNC_MODE_X,
                                       GR_CMBX_ZERO, GR_FUNC_MODE_ZERO,
                                       GR_CMBX_ZERO, FXTRUE,
                                       GR_CMBX_ZERO, FXFALSE,
                                       0, FXFALSE);
        }
        else {
            /* Rv = Rf + Tt */
            (*grTexColorCombineExtPtr)(unit,
                                     incomingRGB, GR_FUNC_MODE_X,
                                     GR_CMBX_LOCAL_TEXTURE_RGB, GR_FUNC_MODE_X,
                                     GR_CMBX_ZERO, FXTRUE,
                                     GR_CMBX_ZERO, FXFALSE,
                                     0, FXFALSE);
        }
        /* Setup Alpha combiner */
        if (baseFormat == GL_LUMINANCE || baseFormat == GL_RGB) {
            /* Av = Af */
            (*grTexAlphaCombineExtPtr)(unit,
                                       incomingAlpha, GR_FUNC_MODE_X,
                                       GR_CMBX_ITALPHA, GR_FUNC_MODE_ZERO,
                                       GR_CMBX_ZERO, FXTRUE,
                                       GR_CMBX_ZERO, FXFALSE,
                                       0, FXFALSE);
        }
        else if (baseFormat == GL_INTENSITY) {
            /* Av = Af + It */
            (*grTexAlphaCombineExtPtr)(unit,
                                   incomingAlpha, GR_FUNC_MODE_X,
                                   GR_CMBX_LOCAL_TEXTURE_ALPHA, GR_FUNC_MODE_X,
                                   GR_CMBX_ZERO, FXTRUE,
                                   GR_CMBX_ZERO, FXFALSE,
                                   0, FXFALSE);
        }
        else {
            /* Av = Af * At */
            (*grTexAlphaCombineExtPtr)(unit,
                                   GR_CMBX_LOCAL_TEXTURE_ALPHA, GR_FUNC_MODE_X,
                                   GR_CMBX_ITALPHA, GR_FUNC_MODE_ZERO,
                                   incomingAlpha, FXFALSE,
                                   GR_CMBX_ZERO, FXFALSE,
                                   0, FXFALSE);
        }
        break;
    case GL_COMBINE_EXT:
        {
            FxU32 A_RGB, B_RGB, C_RGB, D_RGB;
            FxU32 Amode_RGB, Bmode_RGB;
            FxBool Cinv_RGB, Dinv_RGB, Ginv_RGB;
            FxU32 Shift_RGB;
            FxU32 A_A, B_A, C_A, D_A;
            FxU32 Amode_A, Bmode_A;
            FxBool Cinv_A, Dinv_A, Ginv_A;
            FxU32 Shift_A;
           /*
            *
            * In the formulas below, we write:
            *  o "1(x)" for the identity function applied to x,
            *    so 1(x) = x.
            *  o "0(x)" for the constant function 0, so
            *    0(x) = 0 for all values of x.
            *
            * Calculate the color combination.
            */
            Shift_RGB = texUnit->CombineScaleShiftRGB;
            Shift_A = texUnit->CombineScaleShiftA;
            switch (texUnit->CombineModeRGB) {
            case GL_REPLACE:
               /*
                * The formula is: Arg0
                * We implement this by the formula:
                *   (Arg0 + 0(0))*(1-0) + 0
                */
                TEXENV_SETUP_ARG_RGB(A_RGB,
                                     texUnit->CombineSourceRGB[0],
                                     texUnit->CombineOperandRGB[0],
                                     incomingRGB, incomingAlpha);
                TEXENV_SETUP_MODE_RGB(Amode_RGB,
                                      texUnit->CombineOperandRGB[0]);
                B_RGB = C_RGB = D_RGB = GR_CMBX_ZERO;
                Bmode_RGB = GR_FUNC_MODE_ZERO;
                Cinv_RGB = FXTRUE;
                Dinv_RGB = Ginv_RGB = FXFALSE;
                break;
            case GL_MODULATE:
               /*
                * The formula is: Arg0 * Arg1
                *
                * We implement this by the formula
                *   (Arg0 + 0(0)) * Arg1 + 0(0)
                */
                TEXENV_SETUP_ARG_RGB(A_RGB,
                                     texUnit->CombineSourceRGB[0],
                                     texUnit->CombineOperandRGB[0],
                                     incomingRGB, incomingAlpha);
                TEXENV_SETUP_MODE_RGB(Amode_RGB,
                                      texUnit->CombineOperandRGB[0]);
                B_RGB = GR_CMBX_ZERO;
                Bmode_RGB = GR_CMBX_ZERO;
                TEXENV_SETUP_ARG_RGB(C_RGB,
                                     texUnit->CombineSourceRGB[1],
                                     texUnit->CombineOperandRGB[1],
                                     incomingRGB, incomingAlpha);
                Cinv_RGB = TEXENV_OPERAND_INVERTED
                               (texUnit->CombineOperandRGB[1]);
                D_RGB = GR_CMBX_ZERO;
                Dinv_RGB = Ginv_RGB = FXFALSE;
                break;
            case GL_ADD:
               /*
                * The formula is Arg0 + Arg1
                */
                TEXENV_SETUP_ARG_RGB(A_RGB,
                                     texUnit->CombineSourceRGB[0],
                                     texUnit->CombineOperandRGB[0],
                                     incomingRGB, incomingAlpha);
                TEXENV_SETUP_MODE_RGB(Amode_RGB,
                                      texUnit->CombineOperandRGB[0]);
                TEXENV_SETUP_ARG_RGB(B_RGB,
                                     texUnit->CombineSourceRGB[1],
                                     texUnit->CombineOperandRGB[1],
                                     incomingRGB, incomingAlpha);
                TEXENV_SETUP_MODE_RGB(Bmode_RGB,
                                      texUnit->CombineOperandRGB[1]);
                C_RGB = D_RGB = GR_CMBX_ZERO;
                Cinv_RGB = FXTRUE;
                Dinv_RGB = Ginv_RGB = FXFALSE;
                break;
            case GL_ADD_SIGNED_EXT:
               /*
                * The formula is: Arg0 + Arg1 - 0.5.
                * We compute this by calculating:
                *      (Arg0 - 1/2) + Arg1         if op0 is SRC_{COLOR,ALPHA}
                *      Arg0 + (Arg1 - 1/2)         if op1 is SRC_{COLOR,ALPHA}
                * If both op0 and op1 are ONE_MINUS_SRC_{COLOR,ALPHA}
                * we cannot implement the formula properly.
                */
                TEXENV_SETUP_ARG_RGB(A_RGB,
                                     texUnit->CombineSourceRGB[0],
                                     texUnit->CombineOperandRGB[0],
                                     incomingRGB, incomingAlpha);
                TEXENV_SETUP_ARG_RGB(B_RGB,
                                     texUnit->CombineSourceRGB[1],
                                     texUnit->CombineOperandRGB[1],
                                     incomingRGB, incomingAlpha);
                if (!TEXENV_OPERAND_INVERTED(texUnit->CombineOperandRGB[0])) {
                   /*
                    * A is not inverted.  So, choose it.
                    */
                    Amode_RGB = GR_FUNC_MODE_X_MINUS_HALF;
                    if (!TEXENV_OPERAND_INVERTED
                            (texUnit->CombineOperandRGB[1])) {
                        Bmode_RGB = GR_FUNC_MODE_X;
                    } else {
                        Bmode_RGB = GR_FUNC_MODE_ONE_MINUS_X;
                    }
                } else {
                   /*
                    * A is inverted, so try to subtract 1/2
                    * from B.
                    */
                    Amode_RGB = GR_FUNC_MODE_ONE_MINUS_X;
                    if (!TEXENV_OPERAND_INVERTED
                            (texUnit->CombineOperandRGB[1])) {
                        Bmode_RGB = GR_FUNC_MODE_X_MINUS_HALF;
                    } else {
                       /*
                        * Both are inverted.  This is the case
                        * we cannot handle properly.  We just
                        * choose to not add the - 1/2.
                        */
                        Bmode_RGB = GR_FUNC_MODE_ONE_MINUS_X;
                    }
                }
                C_RGB = D_RGB = GR_CMBX_ZERO;
                Cinv_RGB = FXTRUE;
                Dinv_RGB = Ginv_RGB = FXFALSE;
                break;
            case GL_INTERPOLATE_EXT:
               /*
                * The formula is: Arg0 * Arg2 + Arg1 * (1 - Arg2).
                * We compute this by the formula:
                *            (Arg0 - Arg1) * Arg2 + Arg1
                *               == Arg0 * Arg2 - Arg1 * Arg2 + Arg1
                *               == Arg0 * Arg2 + Arg1 * (1 - Arg2)
                * However, if both Arg1 is ONE_MINUS_X, the HW does
                * not support it properly.
                */
                TEXENV_SETUP_ARG_RGB(A_RGB,
                                     texUnit->CombineSourceRGB[0],
                                     texUnit->CombineOperandRGB[0],
                                     incomingRGB, incomingAlpha);
                TEXENV_SETUP_MODE_RGB(Amode_RGB,
                                      texUnit->CombineOperandRGB[0]);
                TEXENV_SETUP_ARG_RGB(B_RGB,
                                     texUnit->CombineSourceRGB[1],
                                     texUnit->CombineOperandRGB[1],
                                     incomingRGB, incomingAlpha);
                if (!TEXENV_OPERAND_INVERTED(texUnit->CombineOperandRGB[1])) {
                    Bmode_RGB = GR_FUNC_MODE_NEGATIVE_X;
                } else {
                   /*
                    * This case is wrong.
                    */
                    Bmode_RGB = GR_FUNC_MODE_NEGATIVE_X;
                }
               /*
                * The Source/Operand for the C value must
                * specify some kind of alpha value.
                */
                TEXENV_SETUP_ARG_A(C_RGB,
                                   texUnit->CombineSourceRGB[2],
                                   texUnit->CombineOperandRGB[2],
                                   incomingAlpha);
                Cinv_RGB = FXFALSE;
                D_RGB = GR_CMBX_B;
                Dinv_RGB = Ginv_RGB = FXFALSE;
                break;
            default:
               /*
                * This is here mostly to keep from getting
                * a compiler warning about these not being set.
                * However, this should set all the texture values
                * to zero.
                */
                A_RGB = B_RGB = C_RGB = D_RGB = GR_CMBX_ZERO;
                Amode_RGB = Bmode_RGB = GR_FUNC_MODE_X;
                Cinv_RGB = Dinv_RGB = Ginv_RGB = FXFALSE;
                break;
            }
           /*
            * Calculate the alpha combination.
            */
            switch (texUnit->CombineModeA) {
            case GL_REPLACE:
               /*
                * The formula is: Arg0
                * We implement this by the formula:
                *   (Arg0 + 0(0))*(1-0) + 0
                */
                TEXENV_SETUP_ARG_A(A_A,
                                   texUnit->CombineSourceA[0],
                                   texUnit->CombineOperandA[0],
                                   incomingAlpha);
                TEXENV_SETUP_MODE_A(Amode_A,
                                    texUnit->CombineOperandA[0]);
                B_A = C_A = D_A = GR_CMBX_ZERO;
                Bmode_A = GR_FUNC_MODE_ZERO;
                Cinv_A = FXTRUE;
                Dinv_A = Ginv_A = FXFALSE;
                break;
            case GL_MODULATE:
               /*
                * The formula is: Arg0 * Arg1
                *
                * We implement this by the formula
                *   (Arg0 + 0(0)) * Arg1 + 0(0)
                */
                TEXENV_SETUP_ARG_A(A_A,
                                   texUnit->CombineSourceA[0],
                                   texUnit->CombineOperandA[0],
                                   incomingAlpha);
                TEXENV_SETUP_MODE_A(Amode_A,
                                    texUnit->CombineOperandA[0]);
                B_A = GR_CMBX_ZERO;
                Bmode_A = GR_CMBX_ZERO;
                TEXENV_SETUP_ARG_A(C_A,
                                   texUnit->CombineSourceA[1],
                                   texUnit->CombineOperandA[1],
                                   incomingAlpha);
                Cinv_A = TEXENV_OPERAND_INVERTED
                               (texUnit->CombineOperandA[1]);
                D_A = GR_CMBX_ZERO;
                Dinv_A = Ginv_A = FXFALSE;
                break;
            case GL_ADD:
               /*
                * The formula is Arg0 + Arg1
                */
                TEXENV_SETUP_ARG_A(A_A,
                                   texUnit->CombineSourceA[0],
                                   texUnit->CombineOperandA[0],
                                   incomingAlpha);
                TEXENV_SETUP_MODE_A(Amode_A,
                                    texUnit->CombineOperandA[0]);
                TEXENV_SETUP_ARG_A(B_A,
                                   texUnit->CombineSourceA[1],
                                   texUnit->CombineOperandA[1],
                                   incomingAlpha);
                TEXENV_SETUP_MODE_A(Bmode_A,
                                    texUnit->CombineOperandA[0]);
                C_A = D_A = GR_CMBX_ZERO;
                Cinv_A = FXTRUE;
                Dinv_A = Ginv_A = FXFALSE;
                break;
            case GL_ADD_SIGNED_EXT:
               /*
                * The formula is: Arg0 + Arg1 - 0.5.
                * We compute this by calculating:
                *      (Arg0 - 1/2) + Arg1         if op0 is SRC_{COLOR,ALPHA}
                *      Arg0 + (Arg1 - 1/2)         if op1 is SRC_{COLOR,ALPHA}
                * If both op0 and op1 are ONE_MINUS_SRC_{COLOR,ALPHA}
                * we cannot implement the formula properly.
                */
                TEXENV_SETUP_ARG_A(A_A,
                                   texUnit->CombineSourceA[0],
                                   texUnit->CombineOperandA[0],
                                   incomingAlpha);
                TEXENV_SETUP_ARG_A(B_A,
                                   texUnit->CombineSourceA[1],
                                   texUnit->CombineOperandA[1],
                                   incomingAlpha);
                if (!TEXENV_OPERAND_INVERTED(texUnit->CombineOperandA[0])) {
                   /*
                    * A is not inverted.  So, choose it.
                    */
                    Amode_A = GR_FUNC_MODE_X_MINUS_HALF;
                    if (!TEXENV_OPERAND_INVERTED
                            (texUnit->CombineOperandA[1])) {
                        Bmode_A = GR_FUNC_MODE_X;
                    } else {
                        Bmode_A = GR_FUNC_MODE_ONE_MINUS_X;
                    }
                } else {
                   /*
                    * A is inverted, so try to subtract 1/2
                    * from B.
                    */
                    Amode_A = GR_FUNC_MODE_ONE_MINUS_X;
                    if (!TEXENV_OPERAND_INVERTED
                            (texUnit->CombineOperandA[1])) {
                        Bmode_A = GR_FUNC_MODE_X_MINUS_HALF;
                    } else {
                       /*
                        * Both are inverted.  This is the case
                        * we cannot handle properly.  We just
                        * choose to not add the - 1/2.
                        */
                        Bmode_A = GR_FUNC_MODE_ONE_MINUS_X;
                    }
                }
                C_A = D_A = GR_CMBX_ZERO;
                Cinv_A = FXTRUE;
                Dinv_A = Ginv_A = FXFALSE;
                break;
            case GL_INTERPOLATE_EXT:
               /*
                * The formula is: Arg0 * Arg2 + Arg1 * (1 - Arg2).
                * We compute this by the formula:
                *            (Arg0 - Arg1) * Arg2 + Arg1
                *               == Arg0 * Arg2 - Arg1 * Arg2 + Arg1
                *               == Arg0 * Arg2 + Arg1 * (1 - Arg2)
                * However, if both Arg1 is ONE_MINUS_X, the HW does
                * not support it properly.
                */
                TEXENV_SETUP_ARG_A(A_A,
                                   texUnit->CombineSourceA[0],
                                   texUnit->CombineOperandA[0],
                                   incomingAlpha);
                TEXENV_SETUP_MODE_A(Amode_A,
                                    texUnit->CombineOperandA[0]);
                TEXENV_SETUP_ARG_A(B_A,
                                   texUnit->CombineSourceA[1],
                                   texUnit->CombineOperandA[1],
                                   incomingAlpha);
                if (!TEXENV_OPERAND_INVERTED(texUnit->CombineOperandA[1])) {
                    Bmode_A = GR_FUNC_MODE_NEGATIVE_X;
                } else {
                   /*
                    * This case is wrong.
                    */
                    Bmode_A = GR_FUNC_MODE_NEGATIVE_X;
                }
               /*
                * The Source/Operand for the C value must
                * specify some kind of alpha value.
                */
                TEXENV_SETUP_ARG_A(C_A,
                                   texUnit->CombineSourceA[2],
                                   texUnit->CombineOperandA[2],
                                   incomingAlpha);
                Cinv_A = FXFALSE;
                D_A = GR_CMBX_ZERO;
                Dinv_A = Ginv_A = FXFALSE;
                break;
            default:
               /*
                * This is here mostly to keep from getting
                * a compiler warning about these not being set.
                * However, this should set all the alpha values
                * to one.
                */
                A_A = B_A = C_A = D_A = GR_CMBX_ZERO;
                Amode_A = Bmode_A = GR_FUNC_MODE_X;
                Cinv_A = Dinv_A = FXFALSE;
                Ginv_A = FXTRUE;
                break;
            }
           /*
            * Call the functions.
            */
            (*grTexColorCombineExtPtr)(unit,
                                       A_RGB, Amode_RGB,
                                       B_RGB, Bmode_RGB,
                                       C_RGB, Cinv_RGB,
                                       D_RGB, Dinv_RGB,
                                       Shift_RGB, Ginv_RGB);
            (*grTexAlphaCombineExtPtr)(unit,
                                       A_A, Amode_A,
                                       B_A, Bmode_A,
                                       C_A, Cinv_A,
                                       D_A, Dinv_A,
                                       Shift_A, Ginv_A);
        }
    break;
    default:
        gl_problem(ctx, "Bad baseFormat in SetupTexEnvNapalm");
    }

    /* setup Color and Alpha combine always the same */
    (*grColorCombineExtPtr)(GR_CMBX_TEXTURE_RGB, GR_FUNC_MODE_X, 
                            GR_CMBX_ZERO, GR_FUNC_MODE_X, 
                            GR_CMBX_ZERO, FXTRUE,
                            GR_CMBX_ZERO, FXFALSE,
                            0, FXFALSE);
    (*grAlphaCombineExtPtr)(GR_CMBX_TEXTURE_ALPHA, GR_FUNC_MODE_X, 
                            GR_CMBX_ZERO, GR_FUNC_MODE_X, 
                            GR_CMBX_ZERO, FXTRUE,
                            GR_CMBX_ZERO, FXFALSE,
                            0, FXFALSE);
}


static void
fxSetupTextureSingleTMU_NoLock(GLcontext * ctx, GLuint textureset)
{
    fxMesaContext fxMesa = (fxMesaContext) ctx->DriverCtx;
    GLuint unitsmode;
    GLint ifmt;
    tfxTexInfo *ti;
    struct gl_texture_object *tObj =
        ctx->Texture.Unit[textureset].CurrentD[2];
    int tmu;

    if (MESA_VERBOSE & VERBOSE_DRIVER) {
        fprintf(stderr, "fxmesa: fxSetupTextureSingleTMU(...) Start\n");
    }

    ti = fxTMGetTexInfo(tObj);
    fxTexValidate(ctx, tObj);

    fxSetupSingleTMU_NoLock(fxMesa, tObj);

    if (ti->whichTMU == FX_TMU_BOTH)
        tmu = FX_TMU0;
    else
        tmu = ti->whichTMU;

    if (fxMesa->tmuSrc != tmu)
        fxSelectSingleTMUSrc_NoLock(fxMesa, tmu, ti->LODblend);

    fxMesa->stw_hint_state = 0;
    FX_grHints_NoLock(GR_HINT_STWHINT, 0);

    ifmt = ti->baseLevelInternalFormat;

    if (/*0*/fxMesa->isNapalm) {
        SetupTexEnvNapalm(ctx, tmu, GL_TRUE,
                          &(ctx->Texture.Unit[textureset]),
                          ti->baseLevelInternalFormat);
    }
    else {
        GLboolean iteratedRGBA = GL_FALSE;
        if (textureset == 0 || !fxMesa->haveTwoTMUs)
            unitsmode = fxGetTexSetConfiguration(ctx, tObj, NULL);
        else
            unitsmode = fxGetTexSetConfiguration(ctx, NULL, tObj);
        if ((unitsmode & FX_UM_ALPHA_ITERATED) ||
            (unitsmode & FX_UM_COLOR_ITERATED)) {
            iteratedRGBA = GL_TRUE;
        }
        SetupTexEnvVoodoo3(ctx, tmu, iteratedRGBA,
                           ctx->Texture.Unit[textureset].EnvMode,
                           ti->baseLevelInternalFormat);
    }

    if (MESA_VERBOSE & VERBOSE_DRIVER) {
        fprintf(stderr, "fxmesa: fxSetupTextureSingleTMU(...) End\n");
    }
}


static void
fxSetupTextureSingleTMU(GLcontext * ctx, GLuint textureset)
{
    fxMesaContext fxMesa = (fxMesaContext) ctx->DriverCtx;
    BEGIN_BOARD_LOCK(fxMesa);
    fxSetupTextureSingleTMU_NoLock(ctx, textureset);
    END_BOARD_LOCK(fxMesa);
}

/************************* Double Texture Set ***************************/

static void
fxSetupDoubleTMU_NoLock(fxMesaContext fxMesa,
                        struct gl_texture_object *tObj0,
                        struct gl_texture_object *tObj1)
{
#define T0_NOT_IN_TMU  0x01
#define T1_NOT_IN_TMU  0x02
#define T0_IN_TMU0     0x04
#define T1_IN_TMU0     0x08
#define T0_IN_TMU1     0x10
#define T1_IN_TMU1     0x20

    const struct gl_shared_state *mesaShared = fxMesa->glCtx->Shared;
    const struct TdfxSharedState *shared = (struct TdfxSharedState *) mesaShared->DriverData;
    const GLcontext *ctx = fxMesa->glCtx;
    tfxTexInfo *ti0 = fxTMGetTexInfo(tObj0);
    tfxTexInfo *ti1 = fxTMGetTexInfo(tObj1);
    GLuint tstate = 0;
    int tmu0 = 0, tmu1 = 1;

    if (MESA_VERBOSE & VERBOSE_DRIVER) {
        fprintf(stderr, "fxmesa: fxSetupDoubleTMU(...)\n");
    }

    /* We shouldn't need to do this. There is something wrong with
       mutlitexturing when the TMUs are swapped. So, we're forcing
       them to always be loaded correctly. !!! */
    if (ti0->whichTMU == FX_TMU1)
        fxTMMoveOutTM_NoLock(fxMesa, tObj0);
    if (ti1->whichTMU == FX_TMU0)
        fxTMMoveOutTM_NoLock(fxMesa, tObj1);

    if (ti0->isInTM) {
        switch (ti0->whichTMU) {
        case FX_TMU0:
            tstate |= T0_IN_TMU0;
            break;
        case FX_TMU1:
            tstate |= T0_IN_TMU1;
            break;
        case FX_TMU_BOTH:
            tstate |= T0_IN_TMU0 | T0_IN_TMU1;
            break;
        case FX_TMU_SPLIT:
            tstate |= T0_NOT_IN_TMU;
            break;
        }
    }
    else
        tstate |= T0_NOT_IN_TMU;

    if (ti1->isInTM) {
        switch (ti1->whichTMU) {
        case FX_TMU0:
            tstate |= T1_IN_TMU0;
            break;
        case FX_TMU1:
            tstate |= T1_IN_TMU1;
            break;
        case FX_TMU_BOTH:
            tstate |= T1_IN_TMU0 | T1_IN_TMU1;
            break;
        case FX_TMU_SPLIT:
            tstate |= T1_NOT_IN_TMU;
            break;
        }
    }
    else
        tstate |= T1_NOT_IN_TMU;

    ti0->lastTimeUsed = fxMesa->texBindNumber;
    ti1->lastTimeUsed = fxMesa->texBindNumber;

    /* Move texture maps into TMUs */

    if (!(((tstate & T0_IN_TMU0) && (tstate & T1_IN_TMU1)) ||
          ((tstate & T0_IN_TMU1) && (tstate & T1_IN_TMU0)))) {
        if (tObj0 == tObj1) {
            if (shared->umaTexMemory)
               fxTMMoveInTM_NoLock(fxMesa, tObj1, FX_TMU0);
            else
               fxTMMoveInTM_NoLock(fxMesa, tObj1, FX_TMU_BOTH);
        }
        else {
            /* Find the minimal way to correct the situation */
            if ((tstate & T0_IN_TMU0) || (tstate & T1_IN_TMU1)) {
                /* We have one in the standard order, setup the other */
                if (tstate & T0_IN_TMU0) { /* T0 is in TMU0, put T1 in TMU1 */
                    if (shared->umaTexMemory)
                        fxTMMoveInTM_NoLock(fxMesa, tObj1, FX_TMU0);
                    else
                        fxTMMoveInTM_NoLock(fxMesa, tObj1, FX_TMU1);
                }
                else {
                    fxTMMoveInTM_NoLock(fxMesa, tObj0, FX_TMU0);
                }
                /* tmu0 and tmu1 are setup */
            }
            else if ((tstate & T0_IN_TMU1) || (tstate & T1_IN_TMU0)) {
                /* we have one in the reverse order, setup the other */
                if (tstate & T1_IN_TMU0) { /* T1 is in TMU0, put T0 in TMU1 */
                    if (shared->umaTexMemory)
                        fxTMMoveInTM_NoLock(fxMesa, tObj0, FX_TMU0);
                    else
                        fxTMMoveInTM_NoLock(fxMesa, tObj0, FX_TMU1);
                }
                else {
                    fxTMMoveInTM_NoLock(fxMesa, tObj1, FX_TMU0);
                }
                tmu0 = 1;
                tmu1 = 0;
            }
            else {              /* Nothing is loaded */
                fxTMMoveInTM_NoLock(fxMesa, tObj0, FX_TMU0);
                if (shared->umaTexMemory)
                    fxTMMoveInTM_NoLock(fxMesa, tObj1, FX_TMU0);
                else
                    fxTMMoveInTM_NoLock(fxMesa, tObj1, FX_TMU1);
                /* tmu0 and tmu1 are setup */
            }
        }
    }

    if (!ctx->Texture.SharedPalette) {
        if (ti0->info.format == GR_TEXFMT_P_8) {
            if (MESA_VERBOSE & VERBOSE_DRIVER) {
                fprintf(stderr, "fxmesa: uploading texture palette TMU0\n");
            }
            FX_grTexDownloadTable_NoLock(tmu0, GR_TEXTABLE_PALETTE_6666_EXT,
                                         &(ti0->palette));
        }

        if (ti1->info.format == GR_TEXFMT_P_8) {
            if (MESA_VERBOSE & VERBOSE_DRIVER) {
                fprintf(stderr, "fxmesa: uploading texture palette TMU1\n");
            }
            FX_grTexDownloadTable_NoLock(tmu1, GR_TEXTABLE_PALETTE_6666_EXT,
                                         &(ti1->palette));
        }
    }

    FX_grTexSource_NoLock(tmu0, ti0->tm[tmu0]->startAddr,
                          GR_MIPMAPLEVELMASK_BOTH, &(ti0->info));
    FX_grTexClampMode_NoLock(tmu0, ti0->sClamp, ti0->tClamp);
    FX_grTexFilterMode_NoLock(tmu0, ti0->minFilt, ti0->maxFilt);
    FX_grTexMipMapMode_NoLock(tmu0, ti0->mmMode, FXFALSE);

    if (shared->umaTexMemory)
        FX_grTexSource_NoLock(tmu1, ti1->tm[tmu0]->startAddr,
                              GR_MIPMAPLEVELMASK_BOTH, &(ti1->info));
    else
        FX_grTexSource_NoLock(tmu1, ti1->tm[tmu1]->startAddr,
                              GR_MIPMAPLEVELMASK_BOTH, &(ti1->info));

    FX_grTexClampMode_NoLock(tmu1, ti1->sClamp, ti1->tClamp);
    FX_grTexFilterMode_NoLock(tmu1, ti1->minFilt, ti1->maxFilt);
    FX_grTexMipMapMode_NoLock(tmu1, ti1->mmMode, FXFALSE);

#undef T0_NOT_IN_TMU
#undef T1_NOT_IN_TMU
#undef T0_IN_TMU0
#undef T1_IN_TMU0
#undef T0_IN_TMU1
#undef T1_IN_TMU1
}

static void
fxSetupTextureDoubleTMU_NoLock(GLcontext * ctx)
{
    fxMesaContext fxMesa = (fxMesaContext) ctx->DriverCtx;
    GrCombineLocal_t localc, locala;
    tfxTexInfo *ti0, *ti1;
    struct gl_texture_object *tObj0 = ctx->Texture.Unit[0].CurrentD[2];
    struct gl_texture_object *tObj1 = ctx->Texture.Unit[1].CurrentD[2];
    GLuint envmode, ifmt, unitsmode;
    int tmu0 = 0, tmu1 = 1;

    if (MESA_VERBOSE & VERBOSE_DRIVER) {
        fprintf(stderr, "fxmesa: fxSetupTextureDoubleTMU(...) Start\n");
    }

    ti0 = fxTMGetTexInfo(tObj0);
    fxTexValidate(ctx, tObj0);

    ti1 = fxTMGetTexInfo(tObj1);
    fxTexValidate(ctx, tObj1);

    fxSetupDoubleTMU_NoLock(fxMesa, tObj0, tObj1);

    unitsmode = fxGetTexSetConfiguration(ctx, tObj0, tObj1);

    fxMesa->stw_hint_state |= GR_STWHINT_ST_DIFF_TMU1;
    FX_grHints_NoLock(GR_HINT_STWHINT, fxMesa->stw_hint_state);

    envmode = unitsmode & FX_UM_E_ENVMODE;
    ifmt = unitsmode & FX_UM_E_IFMT;

    if (unitsmode & FX_UM_ALPHA_ITERATED)
        locala = GR_COMBINE_LOCAL_ITERATED;
    else
        locala = GR_COMBINE_LOCAL_CONSTANT;

    if (unitsmode & FX_UM_COLOR_ITERATED)
        localc = GR_COMBINE_LOCAL_ITERATED;
    else
        localc = GR_COMBINE_LOCAL_CONSTANT;


    if (MESA_VERBOSE & (VERBOSE_DRIVER | VERBOSE_TEXTURE))
        fprintf(stderr, "fxMesa: fxSetupTextureDoubleTMU, envmode is %s/%s\n",
                gl_lookup_enum_by_nr(ctx->Texture.Unit[0].EnvMode),
                gl_lookup_enum_by_nr(ctx->Texture.Unit[1].EnvMode));


    if ((ti0->whichTMU == FX_TMU1) || (ti1->whichTMU == FX_TMU0)) {
        tmu0 = 1;
        tmu1 = 0;
    }
    fxMesa->tmuSrc = FX_TMU_BOTH;

    if (fxMesa->isNapalm) {
        /* Remember, Glide has its texture units numbered in backward
         * order compared to OpenGL.
         */
        SetupTexEnvNapalm(ctx, FX_TMU1, GL_TRUE, &(ctx->Texture.Unit[0]),
                          ti0->baseLevelInternalFormat);
        SetupTexEnvNapalm(ctx, FX_TMU0, GL_FALSE, &(ctx->Texture.Unit[1]),
                          ti1->baseLevelInternalFormat);
    }
    else {
        switch (envmode) {
        case (FX_UM_E0_MODULATE | FX_UM_E1_MODULATE):
            {
                GLboolean isalpha[FX_NUM_TMU];

                if (ti0->baseLevelInternalFormat == GL_ALPHA)
                    isalpha[tmu0] = GL_TRUE;
                else
                    isalpha[tmu0] = GL_FALSE;

                if (ti1->baseLevelInternalFormat == GL_ALPHA)
                    isalpha[tmu1] = GL_TRUE;
                else
                    isalpha[tmu1] = GL_FALSE;

                if (isalpha[FX_TMU1])
                    FX_grTexCombine_NoLock(GR_TMU1,
                                           GR_COMBINE_FUNCTION_ZERO,
                                           GR_COMBINE_FACTOR_NONE,
                                           GR_COMBINE_FUNCTION_LOCAL,
                                           GR_COMBINE_FACTOR_NONE, FXTRUE,
                                           FXFALSE);
                else
                    FX_grTexCombine_NoLock(GR_TMU1, GR_COMBINE_FUNCTION_LOCAL,
                                           GR_COMBINE_FACTOR_NONE,
                                           GR_COMBINE_FUNCTION_LOCAL,
                                           GR_COMBINE_FACTOR_NONE, FXFALSE,
                                           FXFALSE);

                if (isalpha[FX_TMU0])
                    FX_grTexCombine_NoLock(GR_TMU0,
                                           GR_COMBINE_FUNCTION_BLEND_OTHER,
                                           GR_COMBINE_FACTOR_ONE,
                                           GR_COMBINE_FUNCTION_BLEND_OTHER,
                                           GR_COMBINE_FACTOR_LOCAL, FXFALSE,
                                           FXFALSE);
                else
                    FX_grTexCombine_NoLock(GR_TMU0,
                                           GR_COMBINE_FUNCTION_BLEND_OTHER,
                                           GR_COMBINE_FACTOR_LOCAL,
                                           GR_COMBINE_FUNCTION_BLEND_OTHER,
                                           GR_COMBINE_FACTOR_LOCAL, FXFALSE,
                                           FXFALSE);

                FX_grColorCombine_NoLock(GR_COMBINE_FUNCTION_SCALE_OTHER,
                                         GR_COMBINE_FACTOR_LOCAL,
                                         localc, GR_COMBINE_OTHER_TEXTURE,
                                         FXFALSE);

                FX_grAlphaCombine_NoLock(GR_COMBINE_FUNCTION_SCALE_OTHER,
                                         GR_COMBINE_FACTOR_LOCAL,
                                         locala, GR_COMBINE_OTHER_TEXTURE,
                                         FXFALSE);
                break;
            }
        case (FX_UM_E0_REPLACE | FX_UM_E1_BLEND): /* Only for GLQuake */
            if (tmu1 == FX_TMU1) {
                FX_grTexCombine_NoLock(GR_TMU1,
                                       GR_COMBINE_FUNCTION_LOCAL,
                                       GR_COMBINE_FACTOR_NONE,
                                       GR_COMBINE_FUNCTION_LOCAL,
                                       GR_COMBINE_FACTOR_NONE, FXTRUE, FXFALSE);

                FX_grTexCombine_NoLock(GR_TMU0,
                                       GR_COMBINE_FUNCTION_BLEND_OTHER,
                                       GR_COMBINE_FACTOR_LOCAL,
                                       GR_COMBINE_FUNCTION_BLEND_OTHER,
                                       GR_COMBINE_FACTOR_LOCAL, FXFALSE, FXFALSE);
            }
            else {
                FX_grTexCombine_NoLock(GR_TMU1,
                                       GR_COMBINE_FUNCTION_LOCAL,
                                       GR_COMBINE_FACTOR_NONE,
                                       GR_COMBINE_FUNCTION_LOCAL,
                                       GR_COMBINE_FACTOR_NONE, FXFALSE, FXFALSE);

                FX_grTexCombine_NoLock(GR_TMU0,
                                       GR_COMBINE_FUNCTION_BLEND_OTHER,
                                       GR_COMBINE_FACTOR_ONE_MINUS_LOCAL,
                                       GR_COMBINE_FUNCTION_BLEND_OTHER,
                                       GR_COMBINE_FACTOR_ONE_MINUS_LOCAL,
                                       FXFALSE, FXFALSE);
            }

            FX_grAlphaCombine_NoLock(GR_COMBINE_FUNCTION_LOCAL,
                                     GR_COMBINE_FACTOR_NONE,
                                     locala, GR_COMBINE_OTHER_NONE, FXFALSE);

            FX_grColorCombine_NoLock(GR_COMBINE_FUNCTION_SCALE_OTHER,
                                     GR_COMBINE_FACTOR_ONE,
                                     localc, GR_COMBINE_OTHER_TEXTURE, FXFALSE);
            break;
        case (FX_UM_E0_REPLACE | FX_UM_E1_MODULATE): /* Quake 2 and 3 */
            if (tmu1 == FX_TMU1) {
                FX_grTexCombine_NoLock(GR_TMU1,
                                       GR_COMBINE_FUNCTION_LOCAL,
                                       GR_COMBINE_FACTOR_NONE,
                                       GR_COMBINE_FUNCTION_ZERO,
                                       GR_COMBINE_FACTOR_NONE, FXFALSE, FXTRUE);

                FX_grTexCombine_NoLock(GR_TMU0,
                                       GR_COMBINE_FUNCTION_BLEND_OTHER,
                                       GR_COMBINE_FACTOR_LOCAL,
                                       GR_COMBINE_FUNCTION_BLEND_OTHER,
                                       GR_COMBINE_FACTOR_LOCAL, FXFALSE, FXFALSE);

            }
            else {
                FX_grTexCombine_NoLock(GR_TMU1,
                                       GR_COMBINE_FUNCTION_LOCAL,
                                       GR_COMBINE_FACTOR_NONE,
                                       GR_COMBINE_FUNCTION_LOCAL,
                                       GR_COMBINE_FACTOR_NONE, FXFALSE, FXFALSE);

                FX_grTexCombine_NoLock(GR_TMU0,
                                       GR_COMBINE_FUNCTION_BLEND_OTHER,
                                       GR_COMBINE_FACTOR_LOCAL,
                                       GR_COMBINE_FUNCTION_BLEND_OTHER,
                                       GR_COMBINE_FACTOR_ONE, FXFALSE, FXFALSE);
            }

            if (ti0->baseLevelInternalFormat == GL_RGB)
                FX_grAlphaCombine_NoLock(GR_COMBINE_FUNCTION_LOCAL,
                                         GR_COMBINE_FACTOR_NONE,
                                         locala, GR_COMBINE_OTHER_NONE, FXFALSE);
            else
                FX_grAlphaCombine_NoLock(GR_COMBINE_FUNCTION_SCALE_OTHER,
                                         GR_COMBINE_FACTOR_ONE,
                                         locala, GR_COMBINE_OTHER_NONE, FXFALSE);


            FX_grColorCombine_NoLock(GR_COMBINE_FUNCTION_SCALE_OTHER,
                                     GR_COMBINE_FACTOR_ONE,
                                     localc, GR_COMBINE_OTHER_TEXTURE, FXFALSE);
            break;


        case (FX_UM_E0_MODULATE | FX_UM_E1_ADD): /* Quake 3 Sky */
            {
                GLboolean isalpha[FX_NUM_TMU];

                if (ti0->baseLevelInternalFormat == GL_ALPHA)
                    isalpha[tmu0] = GL_TRUE;
                else
                    isalpha[tmu0] = GL_FALSE;

                if (ti1->baseLevelInternalFormat == GL_ALPHA)
                    isalpha[tmu1] = GL_TRUE;
                else
                    isalpha[tmu1] = GL_FALSE;

                if (isalpha[FX_TMU1])
                    FX_grTexCombine_NoLock(GR_TMU1,
                                           GR_COMBINE_FUNCTION_ZERO,
                                           GR_COMBINE_FACTOR_NONE,
                                           GR_COMBINE_FUNCTION_LOCAL,
                                           GR_COMBINE_FACTOR_NONE, FXTRUE,
                                           FXFALSE);
                else
                    FX_grTexCombine_NoLock(GR_TMU1, GR_COMBINE_FUNCTION_LOCAL,
                                           GR_COMBINE_FACTOR_NONE,
                                           GR_COMBINE_FUNCTION_LOCAL,
                                           GR_COMBINE_FACTOR_NONE, FXFALSE,
                                           FXFALSE);

                if (isalpha[FX_TMU0])
                    FX_grTexCombine_NoLock(GR_TMU0,
                                           GR_COMBINE_FUNCTION_SCALE_OTHER,
                                           GR_COMBINE_FACTOR_ONE,
                                           GR_COMBINE_FUNCTION_SCALE_OTHER_ADD_LOCAL,
                                           GR_COMBINE_FACTOR_ONE, FXFALSE,
                                           FXFALSE);
                else
                    FX_grTexCombine_NoLock(GR_TMU0,
                                           GR_COMBINE_FUNCTION_SCALE_OTHER_ADD_LOCAL,
                                           GR_COMBINE_FACTOR_ONE,
                                           GR_COMBINE_FUNCTION_SCALE_OTHER_ADD_LOCAL,
                                           GR_COMBINE_FACTOR_ONE, FXFALSE,
                                           FXFALSE);

                FX_grColorCombine_NoLock(GR_COMBINE_FUNCTION_SCALE_OTHER,
                                         GR_COMBINE_FACTOR_LOCAL,
                                         localc, GR_COMBINE_OTHER_TEXTURE,
                                         FXFALSE);

                FX_grAlphaCombine_NoLock(GR_COMBINE_FUNCTION_SCALE_OTHER,
                                         GR_COMBINE_FACTOR_LOCAL,
                                         locala, GR_COMBINE_OTHER_TEXTURE,
                                         FXFALSE);
                break;
            }
        default:
            fprintf(stderr, "Unexpected dual texture mode encountered\n");
            break;
        }
    }

    if (MESA_VERBOSE & VERBOSE_DRIVER) {
        fprintf(stderr, "fxmesa: fxSetupTextureDoubleTMU(...) End\n");
    }
}

/************************* No Texture ***************************/

static void
fxSetupTextureNone_NoLock(GLcontext * ctx)
{
    /*fxMesaContext fxMesa = (fxMesaContext) ctx->DriverCtx;*/
    GrCombineLocal_t localc, locala;

    if (MESA_VERBOSE & VERBOSE_DRIVER) {
        fprintf(stderr, "fxmesa: fxSetupTextureNone(...)\n");
    }

    if ((ctx->Light.ShadeModel == GL_SMOOTH) || 1 ||
        (ctx->Point.SmoothFlag) ||
        (ctx->Line.SmoothFlag) ||
        (ctx->Polygon.SmoothFlag)) locala = GR_COMBINE_LOCAL_ITERATED;
    else
        locala = GR_COMBINE_LOCAL_CONSTANT;

    if (ctx->Light.ShadeModel == GL_SMOOTH || 1)
        localc = GR_COMBINE_LOCAL_ITERATED;
    else
        localc = GR_COMBINE_LOCAL_CONSTANT;

    FX_grAlphaCombine_NoLock(GR_COMBINE_FUNCTION_LOCAL,
                             GR_COMBINE_FACTOR_NONE,
                             locala, GR_COMBINE_OTHER_NONE, FXFALSE);

    FX_grColorCombine_NoLock(GR_COMBINE_FUNCTION_LOCAL,
                             GR_COMBINE_FACTOR_NONE,
                             localc, GR_COMBINE_OTHER_NONE, FXFALSE);
}

/************************************************************************/
/************************** Texture Mode SetUp **************************/
/************************************************************************/

static void
fxSetupTexture_NoLock(GLcontext * ctx)
{
    fxMesaContext fxMesa = (fxMesaContext) ctx->DriverCtx;
    GLuint tex2Denabled;

    if (MESA_VERBOSE & VERBOSE_DRIVER) {
        fprintf(stderr, "fxmesa: fxSetupTexture(...)\n");
    }

    /* Disable multipass texturing.
     */
    ctx->Driver.MultipassFunc = 0;

    /* Texture Combine, Color Combine and Alpha Combine.
     */
    tex2Denabled = (ctx->Texture.ReallyEnabled & TEXTURE0_2D);

    if (fxMesa->emulateTwoTMUs)
        tex2Denabled |= (ctx->Texture.ReallyEnabled & TEXTURE1_2D);

    switch (tex2Denabled) {
    case TEXTURE0_2D:
        fxSetupTextureSingleTMU_NoLock(ctx, 0);
        break;
    case TEXTURE1_2D:
        fxSetupTextureSingleTMU_NoLock(ctx, 1);
        break;
    case (TEXTURE0_2D | TEXTURE1_2D):
        if (fxMesa->haveTwoTMUs)
            fxSetupTextureDoubleTMU_NoLock(ctx);
        else {
            if (MESA_VERBOSE & VERBOSE_DRIVER)
                fprintf(stderr, "fxmesa: enabling fake multitexture\n");

            fxSetupTextureSingleTMU_NoLock(ctx, 0);
            ctx->Driver.MultipassFunc = fxMultipassTexture;
        }
        break;
    default:
        fxSetupTextureNone_NoLock(ctx);
        break;
    }
}

static void
fxSetupTexture(GLcontext * ctx)
{
    fxMesaContext fxMesa = FX_CONTEXT(ctx);
    BEGIN_BOARD_LOCK(fxMesa);
    fxSetupTexture_NoLock(ctx);
    END_BOARD_LOCK(fxMesa);
}

/************************************************************************/
/**************************** Blend SetUp *******************************/
/************************************************************************/

void
fxDDBlendFunc(GLcontext * ctx, GLenum sfactor, GLenum dfactor)
{
    fxMesaContext fxMesa = (fxMesaContext) ctx->DriverCtx;
    tfxUnitsState *us = &fxMesa->unitsState;
    GrAlphaBlendFnc_t sfact, dfact, asfact, adfact;

    /* From the Glide documentation:
       For alpha source and destination blend function factor
       parameters, Voodoo Graphics supports only
       GR_BLEND_ZERO and GR_BLEND_ONE.
     */

    switch (sfactor) {
    case GL_ZERO:
        asfact = sfact = GR_BLEND_ZERO;
        break;
    case GL_ONE:
        asfact = sfact = GR_BLEND_ONE;
        break;
    case GL_DST_COLOR:
        sfact = GR_BLEND_DST_COLOR;
        asfact = GR_BLEND_ONE;
        break;
    case GL_ONE_MINUS_DST_COLOR:
        sfact = GR_BLEND_ONE_MINUS_DST_COLOR;
        asfact = GR_BLEND_ONE;
        break;
    case GL_SRC_ALPHA:
        sfact = GR_BLEND_SRC_ALPHA;
        asfact = GR_BLEND_ONE;
        break;
    case GL_ONE_MINUS_SRC_ALPHA:
        sfact = GR_BLEND_ONE_MINUS_SRC_ALPHA;
        asfact = GR_BLEND_ONE;
        break;
    case GL_DST_ALPHA:
        sfact = GR_BLEND_DST_ALPHA;
        asfact = GR_BLEND_ONE;
        break;
    case GL_ONE_MINUS_DST_ALPHA:
        sfact = GR_BLEND_ONE_MINUS_DST_ALPHA;
        asfact = GR_BLEND_ONE;
        break;
    case GL_SRC_ALPHA_SATURATE:
        sfact = GR_BLEND_ALPHA_SATURATE;
        asfact = GR_BLEND_ONE;
        break;
    case GL_SRC_COLOR:
    case GL_ONE_MINUS_SRC_COLOR:
        /* USELESS */
        asfact = sfact = GR_BLEND_ONE;
        break;
    default:
        asfact = sfact = GR_BLEND_ONE;
        break;
    }

    if ((sfact != us->blendSrcFuncRGB) || (asfact != us->blendSrcFuncAlpha)) {
        us->blendSrcFuncRGB = sfact;
        us->blendSrcFuncAlpha = asfact;
        fxMesa->new_state |= FX_NEW_BLEND;
        ctx->Driver.RenderStart = fxSetupFXUnits;
    }

    switch (dfactor) {
    case GL_ZERO:
        adfact = dfact = GR_BLEND_ZERO;
        break;
    case GL_ONE:
        adfact = dfact = GR_BLEND_ONE;
        break;
    case GL_SRC_COLOR:
        dfact = GR_BLEND_SRC_COLOR;
        adfact = GR_BLEND_ZERO;
        break;
    case GL_ONE_MINUS_SRC_COLOR:
        dfact = GR_BLEND_ONE_MINUS_SRC_COLOR;
        adfact = GR_BLEND_ZERO;
        break;
    case GL_SRC_ALPHA:
        dfact = GR_BLEND_SRC_ALPHA;
        adfact = GR_BLEND_ZERO;
        break;
    case GL_ONE_MINUS_SRC_ALPHA:
        dfact = GR_BLEND_ONE_MINUS_SRC_ALPHA;
        adfact = GR_BLEND_ZERO;
        break;
    case GL_DST_ALPHA:
        /* dfact=GR_BLEND_DST_ALPHA; */
        /* We can't do DST_ALPHA */
        dfact = GR_BLEND_ONE;
        adfact = GR_BLEND_ZERO;
        break;
    case GL_ONE_MINUS_DST_ALPHA:
        /* dfact=GR_BLEND_ONE_MINUS_DST_ALPHA; */
        /* We can't do DST_ALPHA */
        dfact = GR_BLEND_ZERO;
        adfact = GR_BLEND_ZERO;
        break;
    case GL_SRC_ALPHA_SATURATE:
    case GL_DST_COLOR:
    case GL_ONE_MINUS_DST_COLOR:
        /* USELESS */
        adfact = dfact = GR_BLEND_ZERO;
        break;
    default:
        adfact = dfact = GR_BLEND_ZERO;
        break;
    }

    if ((dfact != us->blendDstFuncRGB) || (adfact != us->blendDstFuncAlpha)) {
        us->blendDstFuncRGB = dfact;
        us->blendDstFuncAlpha = adfact;
        fxMesa->new_state |= FX_NEW_BLEND;
        ctx->Driver.RenderStart = fxSetupFXUnits;
    }
}


/* XXX not done yet, but it looks do-able in the hardware */
void
fxDDBlendFuncSeparate(GLcontext *ctx,
                      GLenum sfactorRGB, GLenum sfactorA,
                      GLenum dfactorRGB, GLenum dfactorA)
{
#if 000
    fxMesaContext fxMesa = (fxMesaContext) ctx->DriverCtx;
    tfxUnitsState *us = &fxMesa->unitsState;
    GrAlphaBlendFnc_t sfact, dfact, asfact, adfact;

    /* From the Glide documentation:
       For alpha source and destination blend function factor
       parameters, Voodoo Graphics supports only
       GR_BLEND_ZERO and GR_BLEND_ONE.
     */

    switch (sfactor) {
    case GL_ZERO:
        asfact = sfact = GR_BLEND_ZERO;
        break;
    case GL_ONE:
        asfact = sfact = GR_BLEND_ONE;
        break;
    case GL_DST_COLOR:
        sfact = GR_BLEND_DST_COLOR;
        asfact = GR_BLEND_ONE;
        break;
    case GL_ONE_MINUS_DST_COLOR:
        sfact = GR_BLEND_ONE_MINUS_DST_COLOR;
        asfact = GR_BLEND_ONE;
        break;
    case GL_SRC_ALPHA:
        sfact = GR_BLEND_SRC_ALPHA;
        asfact = GR_BLEND_ONE;
        break;
    case GL_ONE_MINUS_SRC_ALPHA:
        sfact = GR_BLEND_ONE_MINUS_SRC_ALPHA;
        asfact = GR_BLEND_ONE;
        break;
    case GL_DST_ALPHA:
        sfact = GR_BLEND_DST_ALPHA;
        asfact = GR_BLEND_ONE;
        break;
    case GL_ONE_MINUS_DST_ALPHA:
        sfact = GR_BLEND_ONE_MINUS_DST_ALPHA;
        asfact = GR_BLEND_ONE;
        break;
    case GL_SRC_ALPHA_SATURATE:
        sfact = GR_BLEND_ALPHA_SATURATE;
        asfact = GR_BLEND_ONE;
        break;
    case GL_SRC_COLOR:
    case GL_ONE_MINUS_SRC_COLOR:
        /* USELESS */
        asfact = sfact = GR_BLEND_ONE;
        break;
    default:
        asfact = sfact = GR_BLEND_ONE;
        break;
    }

    if ((sfact != us->blendSrcFuncRGB) || (asfact != us->blendSrcFuncAlpha)) {
        us->blendSrcFuncRGB = sfact;
        us->blendSrcFuncAlpha = asfact;
        fxMesa->new_state |= FX_NEW_BLEND;
        ctx->Driver.RenderStart = fxSetupFXUnits;
    }

    switch (dfactor) {
    case GL_ZERO:
        adfact = dfact = GR_BLEND_ZERO;
        break;
    case GL_ONE:
        adfact = dfact = GR_BLEND_ONE;
        break;
    case GL_SRC_COLOR:
        dfact = GR_BLEND_SRC_COLOR;
        adfact = GR_BLEND_ZERO;
        break;
    case GL_ONE_MINUS_SRC_COLOR:
        dfact = GR_BLEND_ONE_MINUS_SRC_COLOR;
        adfact = GR_BLEND_ZERO;
        break;
    case GL_SRC_ALPHA:
        dfact = GR_BLEND_SRC_ALPHA;
        adfact = GR_BLEND_ZERO;
        break;
    case GL_ONE_MINUS_SRC_ALPHA:
        dfact = GR_BLEND_ONE_MINUS_SRC_ALPHA;
        adfact = GR_BLEND_ZERO;
        break;
    case GL_DST_ALPHA:
        /* dfact=GR_BLEND_DST_ALPHA; */
        /* We can't do DST_ALPHA */
        dfact = GR_BLEND_ONE;
        adfact = GR_BLEND_ZERO;
        break;
    case GL_ONE_MINUS_DST_ALPHA:
        /* dfact=GR_BLEND_ONE_MINUS_DST_ALPHA; */
        /* We can't do DST_ALPHA */
        dfact = GR_BLEND_ZERO;
        adfact = GR_BLEND_ZERO;
        break;
    case GL_SRC_ALPHA_SATURATE:
    case GL_DST_COLOR:
    case GL_ONE_MINUS_DST_COLOR:
        /* USELESS */
        adfact = dfact = GR_BLEND_ZERO;
        break;
    default:
        adfact = dfact = GR_BLEND_ZERO;
        break;
    }

    if ((dfact != us->blendDstFuncRGB) || (adfact != us->blendDstFuncAlpha)) {
        us->blendDstFuncRGB = dfact;
        us->blendDstFuncAlpha = adfact;
        fxMesa->new_state |= FX_NEW_BLEND;
        ctx->Driver.RenderStart = fxSetupFXUnits;
    }
#endif
}


static void
fxSetupBlend(GLcontext * ctx)
{
    fxMesaContext fxMesa = FX_CONTEXT(ctx);
    tfxUnitsState *us = &fxMesa->unitsState;

    if (us->blendEnabled)
        FX_grAlphaBlendFunction(us->blendSrcFuncRGB, us->blendDstFuncRGB,
                                us->blendSrcFuncAlpha, us->blendDstFuncAlpha);
    else
        FX_grAlphaBlendFunction(GR_BLEND_ONE, GR_BLEND_ZERO,
                                GR_BLEND_ONE, GR_BLEND_ZERO);
}


/************************************************************************/
/************************** Alpha Test SetUp ****************************/
/************************************************************************/

void
fxDDAlphaFunc(GLcontext * ctx, GLenum func, GLclampf ref)
{
    fxMesaContext fxMesa = FX_CONTEXT(ctx);
    tfxUnitsState *us = &fxMesa->unitsState;
    GrCmpFnc_t newfunc;

    switch (func) {
    case GL_NEVER:
        newfunc = GR_CMP_NEVER;
        break;
    case GL_LESS:
        newfunc = GR_CMP_LESS;
        break;
    case GL_EQUAL:
        newfunc = GR_CMP_EQUAL;
        break;
    case GL_LEQUAL:
        newfunc = GR_CMP_LEQUAL;
        break;
    case GL_GREATER:
        newfunc = GR_CMP_GREATER;
        break;
    case GL_NOTEQUAL:
        newfunc = GR_CMP_NOTEQUAL;
        break;
    case GL_GEQUAL:
        newfunc = GR_CMP_GEQUAL;
        break;
    case GL_ALWAYS:
        newfunc = GR_CMP_ALWAYS;
        break;
    default:
        gl_problem(ctx, "fx Driver: internal error in fxDDAlphaFunc()\n");
        return;
    }

    if (newfunc != us->alphaTestFunc) {
        us->alphaTestFunc = newfunc;
        fxMesa->new_state |= FX_NEW_ALPHA;
        ctx->Driver.RenderStart = fxSetupFXUnits;
    }

    if (ctx->Color.AlphaRef != us->alphaTestRefValue) {
        us->alphaTestRefValue = ctx->Color.AlphaRef;
        fxMesa->new_state |= FX_NEW_ALPHA;
        ctx->Driver.RenderStart = fxSetupFXUnits;
    }
}

static void
fxSetupAlphaTest(GLcontext * ctx)
{
    fxMesaContext fxMesa = (fxMesaContext) ctx->DriverCtx;
    tfxUnitsState *us = &fxMesa->unitsState;

    if (us->alphaTestEnabled) {
        FX_grAlphaTestFunction(fxMesa, us->alphaTestFunc);
        FX_grAlphaTestReferenceValue(fxMesa, us->alphaTestRefValue);
    }
    else
        FX_grAlphaTestFunction(fxMesa, GR_CMP_ALWAYS);
}


/*
 * Evaluate all depth-test state and make the Glide calls.
 */
static void
fxSetupDepthTest(GLcontext * ctx)
{
    fxMesaContext fxMesa = (fxMesaContext) ctx->DriverCtx;
    if (ctx->Depth.Test) {
        GrCmpFnc_t dfunc;
        switch (ctx->Depth.Func) {
        case GL_NEVER:
            dfunc = GR_CMP_NEVER;
            break;
        case GL_LESS:
            dfunc = GR_CMP_LESS;
            break;
        case GL_GEQUAL:
            dfunc = GR_CMP_GEQUAL;
            break;
        case GL_LEQUAL:
            dfunc = GR_CMP_LEQUAL;
            break;
        case GL_GREATER:
            dfunc = GR_CMP_GREATER;
            break;
        case GL_NOTEQUAL:
            dfunc = GR_CMP_NOTEQUAL;
            break;
        case GL_EQUAL:
            dfunc = GR_CMP_EQUAL;
            break;
        case GL_ALWAYS:
            dfunc = GR_CMP_ALWAYS;
            break;
        default:
            gl_problem(ctx, "bad depth mode in fxSetupDepthTest");
            dfunc = GR_CMP_ALWAYS;
        }
        FX_grDepthBufferFunction(fxMesa, dfunc);
        FX_grDepthMask(fxMesa, ctx->Depth.Mask);
    }
    else {
        /* depth test always passes, don't update Z buffer */
        FX_grDepthBufferFunction(fxMesa, GR_CMP_ALWAYS);
        FX_grDepthMask(fxMesa, FXFALSE);
    }
}


/*
 * Evaluate all stencil state and make the Glide calls.
 */
GrStencil_t
fxConvertGLStencilOp(GLenum op)
{
    switch (op) {
    case GL_KEEP:
        return GR_STENCILOP_KEEP;
    case GL_ZERO:
        return GR_STENCILOP_ZERO;
    case GL_REPLACE:
        return GR_STENCILOP_REPLACE;
    case GL_INCR:
        return GR_STENCILOP_INCR_CLAMP;
    case GL_DECR:
        return GR_STENCILOP_DECR_CLAMP;
    case GL_INVERT:
        return GR_STENCILOP_INVERT;
    default:
        gl_problem(NULL, "bad stencil op in fxConvertGLStencilOp");
    }
    return GR_STENCILOP_KEEP;   /* never get, silence compiler warning */
}

/*
 * This function is called just before any rendering is done.
 * It will validate the stencil parameters.
 */
static void
fxSetupStencilTest(GLcontext * ctx)
{
    fxMesaContext fxMesa = FX_CONTEXT(ctx);
    if (fxMesa->haveHwStencil) {
        if (ctx->Stencil.Enabled) {
            GrStencil_t sfail = fxConvertGLStencilOp(ctx->Stencil.FailFunc);
            GrStencil_t zfail = fxConvertGLStencilOp(ctx->Stencil.ZFailFunc);
            GrStencil_t zpass = fxConvertGLStencilOp(ctx->Stencil.ZPassFunc);
            FX_grStencilOp(fxMesa, sfail, zfail, zpass);
            FX_grStencilFunc(fxMesa, ctx->Stencil.Function - GL_NEVER,
                             ctx->Stencil.Ref, ctx->Stencil.ValueMask);
            FX_grStencilMask(fxMesa, ctx->Stencil.WriteMask);
            FX_grEnable(fxMesa, GR_STENCIL_MODE_EXT);
        }
        else {
            FX_grDisable(fxMesa, GR_STENCIL_MODE_EXT);
        }
    }
}


/*
 * Set the state so that stencil is either enabled or disabled.
 * This is called from Mesa only.  Glide is invoked at
 * setup time, not now.
 */
static void
fxDDEnableStencil(GLcontext * ctx, GLboolean state)
{
    fxMesaContext fxMesa = FX_CONTEXT(ctx);
    (void) state;
    fxMesa->new_state |= FX_NEW_STENCIL;
    ctx->Driver.RenderStart = fxSetupFXUnits;
}


/************************************************************************/
/**************************** Color Mask SetUp **************************/
/************************************************************************/

GLboolean
fxDDColorMask(GLcontext *ctx,
              GLboolean r, GLboolean g, GLboolean b, GLboolean a)
{
    fxMesaContext fxMesa = (fxMesaContext) ctx->DriverCtx;
    fxMesa->new_state |= FX_NEW_COLOR_MASK;
    ctx->Driver.RenderStart = fxSetupFXUnits;
    (void) r;
    (void) g;
    (void) b;
    (void) a;
    return GL_FALSE;
}

static void
fxSetupColorMask(GLcontext *ctx)
{
    if (ctx->Color.DrawBuffer == GL_NONE) {
        FX_grColorMask(ctx, GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    }
    else {
        fxMesaContext fxMesa = FX_CONTEXT(ctx);
        /* XXX need to call grRenderBuffer to work around strange mask bug */
        FX_grRenderBuffer(fxMesa, fxMesa->currentFB);
        FX_grColorMaskv(ctx, ctx->Color.ColorMask);
    }
}


/************************************************************************/
/**************************** Fog Mode SetUp ****************************/
/************************************************************************/

/*
 * This is called during state update in order to update the Glide fog state.
 */
static void
fxSetupFog(GLcontext * ctx)
{
   fxMesaContext fxMesa = FX_CONTEXT(ctx);
    if (ctx->Fog.Enabled && ctx->FogMode == FOG_FRAGMENT) {

        /* update fog color */
        GLubyte col[4];
        col[0] = (unsigned int) (255 * ctx->Fog.Color[0]);
        col[1] = (unsigned int) (255 * ctx->Fog.Color[1]);
        col[2] = (unsigned int) (255 * ctx->Fog.Color[2]);
        col[3] = (unsigned int) (255 * ctx->Fog.Color[3]);
        FX_grFogColorValue(fxMesa, FXCOLOR4(col));

        if (fxMesa->fogTableMode != ctx->Fog.Mode ||
            fxMesa->fogDensity != ctx->Fog.Density ||
            fxMesa->fogStart != ctx->Fog.Start ||
            fxMesa->fogEnd != ctx->Fog.End) {
            /* reload the fog table */
            switch (ctx->Fog.Mode) {
            case GL_LINEAR:
                guFogGenerateLinear(fxMesa->fogTable, ctx->Fog.Start,
                                    ctx->Fog.End);
                break;
            case GL_EXP:
                guFogGenerateExp(fxMesa->fogTable, ctx->Fog.Density);
                break;
            case GL_EXP2:
                guFogGenerateExp2(fxMesa->fogTable, ctx->Fog.Density);
                break;
            default:
                ;
            }
            fxMesa->fogTableMode = ctx->Fog.Mode;
            fxMesa->fogDensity = ctx->Fog.Density;
            fxMesa->fogStart = ctx->Fog.Start;
            fxMesa->fogEnd = ctx->Fog.End;
        }

        FX_grFogTable(fxMesa, fxMesa->fogTable);
        FX_grFogMode(fxMesa, GR_FOG_WITH_TABLE);
    }
    else {
        FX_grFogMode(fxMesa, GR_FOG_DISABLE);
    }
}

void
fxDDFogfv(GLcontext * ctx, GLenum pname, const GLfloat * params)
{
    FX_CONTEXT(ctx)->new_state |= FX_NEW_FOG;
    ctx->Driver.RenderStart = fxSetupFXUnits; /* XXX why is this here? */
}

/************************************************************************/
/************************** Scissor Test SetUp **************************/
/************************************************************************/

/* This routine is used in managing the lock state, and therefore can't lock */
void
fxSetScissorValues(GLcontext * ctx)
{
    fxMesaContext fxMesa = (fxMesaContext) ctx->DriverCtx;
    int xmin, xmax, ymin, ymax;

    if (ctx->Scissor.Enabled) {
        xmin = ctx->Scissor.X;
        xmax = ctx->Scissor.X + ctx->Scissor.Width;
        ymin = ctx->Scissor.Y;
        ymax = ctx->Scissor.Y + ctx->Scissor.Height;
    }
    else {
        xmin = 0;
        ymin = 0;
        xmax = fxMesa->width;
        ymax = fxMesa->height;
    }
    /* translate to screen coords */
    xmin += fxMesa->x_offset;
    xmax += fxMesa->x_offset;
    ymin += fxMesa->y_delta;
    ymax += fxMesa->y_delta;

    /* intersect scissor region with first clip rect */
    if (xmin < fxMesa->clipMinX)
        xmin = fxMesa->clipMinX;
    else if (xmin > fxMesa->clipMaxX)
        xmin = fxMesa->clipMaxX;

    if (xmax > fxMesa->clipMaxX)
        xmax = fxMesa->clipMaxX;

    if (ymin < fxMesa->screen_height - fxMesa->clipMaxY)
        ymin = fxMesa->screen_height - fxMesa->clipMaxY;
    else if (ymin > fxMesa->screen_height - fxMesa->clipMinY)
        ymin = fxMesa->screen_height - fxMesa->clipMinY;

    if (ymax > fxMesa->screen_height - fxMesa->clipMinY)
        ymax = fxMesa->screen_height - fxMesa->clipMinY;

    /* prevent wrap-around problems */
    if (xmax < xmin)
       xmax = xmin;
    if (ymax < ymin)
       ymax = ymin;

    FX_grClipWindow_NoLock(xmin, ymin, xmax, ymax);
}

static void
fxSetupScissor(GLcontext * ctx)
{
    fxMesaContext fxMesa = (fxMesaContext) ctx->DriverCtx;
    if (!fxMesa->needClip) {
        BEGIN_BOARD_LOCK(fxMesa);
        fxSetScissorValues(ctx);
        END_BOARD_LOCK(fxMesa);
    }
}

void
fxDDScissor(GLcontext * ctx, GLint x, GLint y, GLsizei w, GLsizei h)
{
    FX_CONTEXT(ctx)->new_state |= FX_NEW_SCISSOR;
    ctx->Driver.RenderStart = fxSetupFXUnits;
}

/************************************************************************/
/*************************** Cull mode setup ****************************/
/************************************************************************/


void
fxDDCullFace(GLcontext * ctx, GLenum mode)
{
    (void) mode;
    FX_CONTEXT(ctx)->new_state |= FX_NEW_CULL;
    ctx->Driver.RenderStart = fxSetupFXUnits;
}

void
fxDDFrontFace(GLcontext * ctx, GLenum mode)
{
    (void) mode;
    FX_CONTEXT(ctx)->new_state |= FX_NEW_CULL;
    ctx->Driver.RenderStart = fxSetupFXUnits;
}


static void
fxSetupCull(GLcontext * ctx)
{
    fxMesaContext fxMesa = FX_CONTEXT(ctx);
    if (ctx->Polygon.CullFlag) {
        switch (ctx->Polygon.CullFaceMode) {
        case GL_BACK:
            if (ctx->Polygon.FrontFace == GL_CCW)
                FX_CONTEXT(ctx)->cullMode = GR_CULL_NEGATIVE;
            else
                FX_CONTEXT(ctx)->cullMode = GR_CULL_POSITIVE;
            break;
        case GL_FRONT:
            if (ctx->Polygon.FrontFace == GL_CCW)
                FX_CONTEXT(ctx)->cullMode = GR_CULL_POSITIVE;
            else
                FX_CONTEXT(ctx)->cullMode = GR_CULL_NEGATIVE;
            break;
        case GL_FRONT_AND_BACK:
            FX_CONTEXT(ctx)->cullMode = GR_CULL_DISABLE;
            break;
        default:
            break;
        }
    }
    else {
        FX_CONTEXT(ctx)->cullMode = GR_CULL_DISABLE;
    }
    FX_grCullMode(fxMesa, FX_CONTEXT(ctx)->cullMode);
}


/************************************************************************/
/****************************** DD Enable ******************************/
/************************************************************************/

void
fxDDEnable(GLcontext * ctx, GLenum cap, GLboolean state)
{
    fxMesaContext fxMesa = FX_CONTEXT(ctx);
    tfxUnitsState *us = &fxMesa->unitsState;

    if (MESA_VERBOSE & VERBOSE_DRIVER) {
        fprintf(stderr, "fxmesa: fxDDEnable(...)\n");
    }

    switch (cap) {
    case GL_ALPHA_TEST:
        if (state != us->alphaTestEnabled) {
            us->alphaTestEnabled = state;
            fxMesa->new_state |= FX_NEW_ALPHA;
            ctx->Driver.RenderStart = fxSetupFXUnits;
        }
        break;
    case GL_BLEND:
        if (state != us->blendEnabled) {
            us->blendEnabled = state;
            fxMesa->new_state |= FX_NEW_BLEND;
            ctx->Driver.RenderStart = fxSetupFXUnits;
        }
        break;
    case GL_DEPTH_TEST:
        fxMesa->new_state |= FX_NEW_DEPTH;
        ctx->Driver.RenderStart = fxSetupFXUnits;
        break;
    case GL_DITHER:
        if (state)
            FX_grDitherMode(fxMesa, GR_DITHER_4x4);
        else
            FX_grDitherMode(fxMesa, GR_DITHER_DISABLE);
        break;
    case GL_SCISSOR_TEST:
        fxMesa->new_state |= FX_NEW_SCISSOR;
        ctx->Driver.RenderStart = fxSetupFXUnits;
        break;
    case GL_SHARED_TEXTURE_PALETTE_EXT:
        fxDDTexUseGlbPalette(ctx, state);
        break;
    case GL_FOG:
        fxMesa->new_state |= FX_NEW_FOG;
        ctx->Driver.RenderStart = fxSetupFXUnits;
        break;
    case GL_CULL_FACE:
        fxMesa->new_state |= FX_NEW_CULL;
        ctx->Driver.RenderStart = fxSetupFXUnits;
        break;
    case GL_LINE_SMOOTH:
    case GL_LINE_STIPPLE:
    case GL_POINT_SMOOTH:
    case GL_POLYGON_SMOOTH:
    case GL_TEXTURE_2D:
        fxMesa->new_state |= FX_NEW_TEXTURING;
        ctx->Driver.RenderStart = fxSetupFXUnits;
        break;
    case GL_STENCIL_TEST:
        fxMesa->new_state |= FX_NEW_STENCIL;
        ctx->Driver.RenderStart = fxSetupFXUnits;
        break;
    default:
        ;                       /* no-op */
    }
}


#if 0
/*
  Multipass to do GL_BLEND texture functions
  Cf*(1-Ct) has already been written to the buffer during the first pass
  Cc*Ct gets written during the second pass (in this function)
  Everything gets reset in the third call (in this function)
*/
static GLboolean
fxMultipassBlend(struct vertex_buffer *VB, GLuint pass)
{
    GLcontext *ctx = VB->ctx;
    fxMesaContext fxMesa = FX_CONTEXT(ctx);

    switch (pass) {
    case 1:
        /* Add Cc*Ct */
        fxMesa->restoreUnitsState = fxMesa->unitsState;
        if (ctx->Depth.Mask) {
            /* We don't want to check or change the depth buffers */
            switch (ctx->Depth.Func) {
            case GL_NEVER:
            case GL_ALWAYS:
                break;
            default:
                fxDDDepthFunc(ctx, GL_EQUAL);
                break;
            }
            fxDDDepthMask(ctx, FALSE);
        }
        /*
         * Disable stencil as well.
         */
        if (ctx->Stencil.Enabled) {
            fxDDEnableStencil(ctx, GL_FALSE);
        }
        /* Enable Cc*Ct mode */
        /* XXX Set the Constant Color ? */
        fxDDEnable(ctx, GL_BLEND, GL_TRUE);
        fxDDBlendFunc(ctx, XXX, XXX);
        fxSetupTextureSingleTMU(ctx, XXX);
        fxSetupBlend(ctx);
        fxSetupDepthTest(ctx);
        break;

    case 2:
        /* Reset everything back to normal */
        fxMesa->unitsState = fxMesa->restoreUnitsState;
        fxMesa->setupdone &= XXX;
        fxSetupTextureSingleTMU(ctx, XXX);
        fxSetupBlend(ctx);
        fxSetupDepthTest(ctx);
        fxSetupStencilText(ctx);
        break;
    }

    return pass == 1;
}
#endif

/************************************************************************/
/******************** Fake Multitexture Support *************************/
/************************************************************************/

/* Its considered cheeky to try to fake ARB multitexture by doing
 * multipass rendering, because it is not possible to emulate the full
 * spec in this way.  The fact is that the voodoo 2 supports only a
 * subset of the possible multitexturing modes, and it is possible to
 * support almost the same subset using multipass blending on the
 * voodoo 1.  In all other cases for both voodoo 1 and 2, we fall back
 * to software rendering, satisfying the spec if not the user.  
 */
static GLboolean
fxMultipassTexture(struct vertex_buffer *VB, GLuint pass)
{
    GLcontext *ctx = VB->ctx;
    fxVertex *v = FX_DRIVER_DATA(VB)->verts;
    fxVertex *last = FX_DRIVER_DATA(VB)->last_vert;
    fxMesaContext fxMesa = FX_CONTEXT(ctx);

    switch (pass) {
    case 1:
        if (MESA_VERBOSE &
            (VERBOSE_DRIVER | VERBOSE_PIPELINE | VERBOSE_TEXTURE))
                fprintf(stderr, "fxmesa: Second texture pass\n");

        for (; v != last; v++) {
            v->f[S0COORD] = v->f[S1COORD];
            v->f[T0COORD] = v->f[T1COORD];
        }

        fxMesa->restoreUnitsState = fxMesa->unitsState;
        fxMesa->tmu_source[0] = 1;

        if (ctx->Depth.Mask) {
            switch (ctx->Depth.Func) {
            case GL_NEVER:
            case GL_ALWAYS:
                break;
            default:
                /*fxDDDepthFunc( ctx, GL_EQUAL ); */
                FX_grDepthBufferFunction(fxMesa, GR_CMP_EQUAL);
                break;
            }

            /*fxDDDepthMask( ctx, GL_FALSE ); */
            FX_grDepthMask(fxMesa, FXFALSE);
        }
        fxDDEnableStencil(ctx, GL_FALSE);
        if (ctx->Texture.Unit[1].EnvMode == GL_MODULATE) {
            fxDDEnable(ctx, GL_BLEND, GL_TRUE);
            fxDDBlendFunc(ctx, GL_DST_COLOR, GL_ZERO);
        }

        fxSetupTextureSingleTMU(ctx, 1);
        fxSetupBlend(ctx);
        fxSetupDepthTest(ctx);
        break;

    case 2:
        /* Restore original state.  
         */
        fxMesa->tmu_source[0] = 0;
        fxMesa->unitsState = fxMesa->restoreUnitsState;
        fxMesa->setupdone &= ~SETUP_TMU0;
        fxSetupTextureSingleTMU(ctx, 0);
        fxSetupBlend(ctx);
        fxSetupDepthTest(ctx);
        fxSetupStencilTest(ctx);
        break;
    }

    return pass == 1;
}


/************************************************************************/
/************************** Changes to units state **********************/
/************************************************************************/


/* All units setup is handled under texture setup.
 */
void
fxDDShadeModel(GLcontext * ctx, GLenum mode)
{
    FX_CONTEXT(ctx)->new_state |= FX_NEW_TEXTURING;
    ctx->Driver.RenderStart = fxSetupFXUnits;
}



/************************************************************************/
/****************************** Units SetUp *****************************/
/************************************************************************/
static void
gl_print_fx_state_flags(const char *msg, GLuint flags)
{
    fprintf(stderr,
            "%s: (0x%x) %s%s%s%s%s%s%s\n",
            msg,
            flags,
            (flags & FX_NEW_TEXTURING) ? "texture, " : "",
            (flags & FX_NEW_BLEND) ? "blend, " : "",
            (flags & FX_NEW_ALPHA) ? "alpha, " : "",
            (flags & FX_NEW_FOG) ? "fog, " : "",
            (flags & FX_NEW_SCISSOR) ? "scissor, " : "",
            (flags & FX_NEW_COLOR_MASK) ? "colormask, " : "",
            (flags & FX_NEW_CULL) ? "cull, " : "");
}

void
fxSetupFXUnits(GLcontext * ctx)
{
    fxMesaContext fxMesa = (fxMesaContext) ctx->DriverCtx;
    GLuint newstate = fxMesa->new_state;

    if (MESA_VERBOSE & VERBOSE_DRIVER)
        gl_print_fx_state_flags("fxmesa: fxSetupFXUnits", newstate);

    if (newstate) {
        if (newstate & FX_NEW_TEXTURING)
            fxSetupTexture(ctx);

        if (newstate & FX_NEW_BLEND)
            fxSetupBlend(ctx);

        if (newstate & FX_NEW_ALPHA)
            fxSetupAlphaTest(ctx);

        if (newstate & FX_NEW_DEPTH)
            fxSetupDepthTest(ctx);

        if (newstate & FX_NEW_STENCIL)
            fxSetupStencilTest(ctx);

        if (newstate & FX_NEW_FOG)
            fxSetupFog(ctx);

        if (newstate & FX_NEW_SCISSOR)
            fxSetupScissor(ctx);

        if (newstate & FX_NEW_COLOR_MASK)
            fxSetupColorMask(ctx);

        if (newstate & FX_NEW_CULL)
            fxSetupCull(ctx);
        fxMesa->new_state = 0;
/*       ctx->Driver.RenderStart = 0; */
    }
}
