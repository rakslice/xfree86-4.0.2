/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/ati/r128_dri.h,v 1.4 2000/12/04 19:21:52 dawes Exp $ */
/*
 * Copyright 1999, 2000 ATI Technologies Inc., Markham, Ontario,
 *                      Precision Insight, Inc., Cedar Park, Texas, and
 *                      VA Linux Systems Inc., Fremont, California.
 *
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation on the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NON-INFRINGEMENT.  IN NO EVENT SHALL ATI, PRECISION INSIGHT, VA LINUX
 * SYSTEMS AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * Authors:
 *   Kevin E. Martin <martin@valinux.com>
 *   Rickard E. Faith <faith@valinux.com>
 *   Gareth Hughes <gareth@valinux.com>
 *
 */

#ifndef _R128_DRI_
#define _R128_DRI_

#include "xf86drm.h"
#include "xf86drmR128.h"

/* DRI Driver defaults */
#define R128_DEFAULT_CCE_PIO_MODE R128_PM4_64PIO_64VCBM_64INDBM
#define R128_DEFAULT_CCE_BM_MODE  R128_PM4_64BM_64VCBM_64INDBM
#define R128_DEFAULT_AGP_MODE     2
#define R128_DEFAULT_AGP_SIZE     8 /* MB (must be a power of 2 and > 4MB) */
#define R128_DEFAULT_RING_SIZE    1 /* MB (must be page aligned) */
#define R128_DEFAULT_BUFFER_SIZE  2 /* MB (must be page aligned) */
#define R128_DEFAULT_AGP_TEX_SIZE 1 /* MB (must be page aligned) */

#define R128_DEFAULT_CCE_TIMEOUT  10000  /* usecs */

#define R128_AGP_MAX_MODE         4

#define R128_CARD_TYPE_R128          1
#define R128_CARD_TYPE_R128_PRO      2
#define R128_CARD_TYPE_R128_MOBILITY 3

#define R128CCE_USE_RING_BUFFER(m)                                        \
(((m) == R128_PM4_192BM) ||                                               \
 ((m) == R128_PM4_128BM_64INDBM) ||                                       \
 ((m) == R128_PM4_64BM_128INDBM) ||                                       \
 ((m) == R128_PM4_64BM_64VCBM_64INDBM))

typedef struct {
    /* MMIO register data */
    drmHandle     registerHandle;
    drmSize       registerSize;

    /* CCE ring buffer data */
    drmHandle     ringHandle;
    drmSize       ringMapSize;
    int           ringSize;

    /* CCE ring read pointer data */
    drmHandle     ringReadPtrHandle;
    drmSize       ringReadMapSize;

    /* CCE vertex/indirect buffer data */
    drmHandle     bufHandle;
    drmSize       bufMapSize;
    int           bufOffset;

    /* CCE AGP Texture data */
    drmHandle     agpTexHandle;
    drmSize       agpTexMapSize;
    int           log2AGPTexGran;
    int           agpTexOffset;

    /* DRI screen private data */
    int           deviceID;     /* PCI device ID */
    int           width;        /* Width in pixels of display */
    int           height;       /* Height in scanlines of display */
    int           depth;        /* Depth of display (8, 15, 16, 24) */
    int           bpp;          /* Bit depth of display (8, 16, 24, 32) */

    int           frontOffset;  /* Start of front buffer */
    int           frontPitch;
    int           backOffset;   /* Start of shared back buffer */
    int           backPitch;
    int           depthOffset;  /* Start of shared depth buffer */
    int           depthPitch;
    int           spanOffset;   /* Start of scratch spanline */
    int           textureOffset;/* Start of texture data in frame buffer */
    int           textureSize;
    int           log2TexGran;

    int           IsPCI;        /* Current card is a PCI card */
    int           AGPMode;

    int           CCEMode;      /* CCE mode that server/clients use */
    int           CCEFifoSize;  /* Size of the CCE command FIFO */
} R128DRIRec, *R128DRIPtr;

#endif
