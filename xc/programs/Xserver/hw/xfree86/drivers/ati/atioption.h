/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/ati/atioption.h,v 1.4 2000/10/11 22:52:57 tsi Exp $ */
/*
 * Copyright 1999 through 2000 by Marc Aurele La France (TSI @ UQV), tsi@ualberta.ca
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting documentation, and
 * that the name of Marc Aurele La France not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  Marc Aurele La France makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as-is" without express or implied warranty.
 *
 * MARC AURELE LA FRANCE DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS.  IN NO
 * EVENT SHALL MARC AURELE LA FRANCE BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef ___ATIOPTION_H___
#define ___ATIOPTION_H___ 1

#include "atiproto.h"

#include "xf86str.h"

/*
 * Documented XF86Config options.
 */
typedef enum
{
    ATI_OPTION_ACCEL,
    ATI_OPTION_CRT,
    ATI_OPTION_CSYNC,
    ATI_OPTION_LINEAR,
    ATI_OPTION_MMIO_CACHE,
    ATI_OPTION_PROBE_CLOCKS,
    ATI_OPTION_REFERENCE_CLOCK,
    ATI_OPTION_SHADOW_FB
} ATIPublicOptionType;

extern OptionInfoRec       ATIPublicOptions[];
extern const unsigned long ATIPublicOptionSize;

extern OptionInfoPtr       ATIAvailableOptions FunctionPrototype((int, int));

#endif /* ___ATIOPTION_H___ */
