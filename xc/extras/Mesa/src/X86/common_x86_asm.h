
/*
 * Mesa 3-D graphics library
 * Version:  3.4
 *
 * Copyright (C) 1999  Brian Paul   All Rights Reserved.
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
 */

/*
 * Check CPU capabilities & initialize optimized funtions for this particular
 * processor.
 *
 * Written by Holger Waechtler <holger@akaflieg.extern.tu-berlin.de>
 * Changed by Andre Werthmann <wertmann@cs.uni-potsdam.de> for using the
 * new Katmai functions
 *
 * Reimplemented by Gareth Hughes <gareth@valinux.com> in a more
 * future-proof manner, based on code in the Linux kernel.
 */

#ifndef __COMMON_X86_ASM_H__
#define __COMMON_X86_ASM_H__

#include "common_x86_features.h"

#ifdef HAVE_CONFIG_H
#include "conf.h"
#endif

#ifdef USE_X86_ASM
#include "x86.h"
#ifdef USE_3DNOW_ASM
#include "3dnow.h"
#endif
#ifdef USE_KATMAI_ASM
#include "katmai.h"
#endif
#endif

extern int gl_x86_cpu_features;

extern void gl_init_all_x86_transform_asm( void );
extern void gl_init_all_x86_shade_asm( void );
extern void gl_init_all_x86_vertex_asm( void );

#endif
