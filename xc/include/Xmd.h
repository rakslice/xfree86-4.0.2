/* $XFree86: xc/include/Xmd.h,v 3.8 2000/08/23 22:10:01 tsi Exp $ */
/***********************************************************

Copyright 1987, 1998  The Open Group

All Rights Reserved.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from The Open Group.


Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the name of Digital not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.  

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/
#ifndef XMD_H
#define XMD_H 1
/* $TOG: Xmd.h /main/36 1998/02/09 11:18:48 kaleb $ */
/*
 *  Xmd.h: MACHINE DEPENDENT DECLARATIONS.
 */

/*
 * Special per-machine configuration flags.
 */
#ifdef CRAY
#define WORD64				/* 64-bit architecture */
#endif
#if defined(__alpha) || defined(__alpha__) || \
    defined(__ia64__) || defined(ia64)
#define LONG64				/* 32/64-bit architecture */
#endif
#ifdef __sgi
#if (_MIPS_SZLONG == 64)
#define LONG64
#endif
#endif

/*
 * Stuff to handle large architecture machines; the constants were generated
 * on a 32-bit machine and must coorespond to the protocol.
 */
#ifdef WORD64
#define MUSTCOPY
#endif /* WORD64 */


/*
 * Definition of macro used to set constants for size of network structures;
 * machines with preprocessors that can't handle all of the sz_ symbols
 * can define this macro to be sizeof(x) if and only if their compiler doesn't
 * pad out structures (esp. the xTextElt structure which contains only two 
 * one-byte fields).  Network structures should always define sz_symbols.
 *
 * The sz_ prefix is used instead of something more descriptive so that the
 * symbols are no more than 32 characters long (which causes problems for some
 * compilers and preprocessors).
 *
 * The extra indirection in the __STDC__ case is to get macro arguments to
 * expand correctly before the concatenation, rather than afterward.
 */
#if ((defined(__STDC__) || defined(__cplusplus) || defined(c_plusplus)) && !defined(UNIXCPP)) || defined(ANSICPP)
#define _SIZEOF(x) sz_##x
#define SIZEOF(x) _SIZEOF(x)
#else
#define SIZEOF(x) sz_/**/x
#endif /* if ANSI C compiler else not */

/*
 * Bitfield suffixes for the protocol structure elements, if you
 * need them.  Note that bitfields are not guarranteed to be signed
 * (or even unsigned) according to ANSI C.
 */
#ifdef WORD64
typedef long INT64;
typedef unsigned long CARD64;
#define B32 :32
#define B16 :16
#ifdef UNSIGNEDBITFIELDS
typedef unsigned int INT32;
typedef unsigned int INT16;
#else
#ifdef __STDC__
typedef signed int INT32;
typedef signed int INT16;
#else
typedef int INT32;
typedef int INT16;
#endif
#endif
#else
#define B32
#define B16
#ifdef LONG64
typedef long INT64;
typedef int INT32;
#else
typedef long INT32;
#endif
typedef short INT16;
#endif

#if defined(__STDC__) || defined(sgi) || defined(AIXV3)
typedef signed char    INT8;
#else
typedef char           INT8;
#endif

#ifdef LONG64
typedef unsigned long CARD64;
typedef unsigned int CARD32;
#else
typedef unsigned long CARD32;
#endif
typedef unsigned short CARD16;
typedef unsigned char  CARD8;

typedef CARD32		BITS32;
typedef CARD16		BITS16;

#ifndef __CYGWIN__
#ifndef __EMX__
typedef CARD8		BYTE;
typedef CARD8           BOOL;
#else
/*
 * This is bad style, but the central include file <os2.h> declares them
 * as well
 */
#define BYTE		CARD8
#define BOOL		CARD8
#endif
#endif

#ifdef __CYGWIN__
#undef BYTE
#undef BOOL
#define BYTE CARD8
#define BOOL CARD8
#endif

/*
 * definitions for sign-extending bitfields on 64-bit architectures
 */
#if defined(WORD64) && defined(UNSIGNEDBITFIELDS)
#define cvtINT8toInt(val)   (((val) & 0x00000080) ? ((val) | 0xffffffffffffff00) : (val))
#define cvtINT16toInt(val)  (((val) & 0x00008000) ? ((val) | 0xffffffffffff0000) : (val))
#define cvtINT32toInt(val)  (((val) & 0x80000000) ? ((val) | 0xffffffff00000000) : (val))
#define cvtINT8toShort(val)  cvtINT8toInt(val)
#define cvtINT16toShort(val) cvtINT16toInt(val)
#define cvtINT32toShort(val) cvtINT32toInt(val)
#define cvtINT8toLong(val)  cvtINT8toInt(val)
#define cvtINT16toLong(val) cvtINT16toInt(val)
#define cvtINT32toLong(val) cvtINT32toInt(val)
#else
#define cvtINT8toInt(val) (val)
#define cvtINT16toInt(val) (val)
#define cvtINT32toInt(val) (val)
#define cvtINT8toShort(val) (val)
#define cvtINT16toShort(val) (val)
#define cvtINT32toShort(val) (val)
#define cvtINT8toLong(val) (val)
#define cvtINT16toLong(val) (val)
#define cvtINT32toLong(val) (val)
#endif /* WORD64 and UNSIGNEDBITFIELDS */



#ifdef MUSTCOPY
/*
 * This macro must not cast or else pointers will get aligned and be wrong
 */
#define NEXTPTR(p,t)  (((char *) p) + SIZEOF(t))
#else /* else not MUSTCOPY, this is used for 32-bit machines */
/*
 * this version should leave result of type (t *), but that should only be 
 * used when not in MUSTCOPY
 */  
#define NEXTPTR(p,t) (((t *)(p)) + 1)
#endif /* MUSTCOPY - used machines whose C structs don't line up with proto */

#endif /* XMD_H */
