/* $XFree86: xc/lib/GL/mesa/src/drv/r128/r128_texobj.h,v 1.3 2000/12/04 19:21:47 dawes Exp $ */
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

#ifndef _R128_TEXOBJ_H_
#define _R128_TEXOBJ_H_

#include "r128_sarea.h"
#include "mm.h"

/* Individual texture image information */
typedef struct {
    GLuint offset;	/* Offset into locally shared texture space (i.e.,
			   relative to bufAddr (below) */
    GLuint width;	/* Width of texture image */
    GLuint height;	/* Height of texture image */
} r128TexImage;

typedef struct r128_tex_obj r128TexObj, *r128TexObjPtr;

/* Texture object in locally shared texture space */
struct r128_tex_obj {
   r128TexObjPtr	next, prev;

   struct gl_texture_object *tObj;	/* Mesa texture object */

   PMemBlock		memBlock;	/* Memory block containing texture */
   CARD32		bufAddr;	/* Offset to start of locally
					   shared texture block */

   CARD32		dirty_images;	/* Flags for whether or not
					   images need to be uploaded to
					   local or AGP texture space */

   GLuint		age;
   GLint		bound;		/* Texture unit currently bound to */
   GLint		heap;		/* Texture heap currently stored in */
   r128TexImage		image[R128_TEX_MAXLEVELS]; /* Image data for all
						      mipmap levels */

   GLint		totalSize;	/* Total size of the texture
					   including all mipmap levels */
   GLuint		internFormat;	/* Internal GL format used to store
					   texture on card */
   CARD32		textureFormat;	/* Actual hardware format */
   GLint		texelBytes;	/* Number of bytes per texel */

   r128_texture_regs_t	setup;		/* Setup regs for texture */
};

#endif /* _R128_TEXOBJ_H_ */
