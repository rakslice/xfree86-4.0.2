/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/chips/ct_bank.c,v 1.5 2000/04/04 19:25:05 dawes Exp $ */

/*
 * Copyright 1997
 * Digital Equipment Corporation. All rights reserved.
 * This software is furnished under license and may be used and copied only in 
 * accordance with the following terms and conditions.  Subject to these 
 * conditions, you may download, copy, install, use, modify and distribute 
 * this software in source and/or binary form. No title or ownership is 
 * transferred hereby.
 * 1) Any source code used, modified or distributed must reproduce and retain 
 *    this copyright notice and list of conditions as they appear in the 
 *    source file.
 *
 * 2) No right is granted to use any trade name, trademark, or logo of Digital 
 *    Equipment Corporation. Neither the "Digital Equipment Corporation" name 
 *    nor any trademark or logo of Digital Equipment Corporation may be used 
 *    to endorse or promote products derived from this software without the 
 *    prior written permission of Digital Equipment Corporation.
 *
 * 3) This software is provided "AS-IS" and any express or implied warranties,
 *    including but not limited to, any implied warranties of merchantability,
 *    fitness for a particular purpose, or non-infringement are disclaimed. In
 *    no event shall DIGITAL be liable for any damages whatsoever, and in 
 *    particular, DIGITAL shall not be liable for special, indirect, 
 *    consequential, or incidental damages or damages for lost profits, loss 
 *    of revenue or loss of use, whether such damages arise in contract, 
 *    negligence, tort, under statute, in equity, at law or otherwise, even if
 *    advised of the possibility of such damage. 
 */
#define PSZ 8

/*
 * Define DIRECT_REGISTER_ACCESS if you want to bypass the wrapped register
 * access functions
 */
/* #define DIRECT_REGISTER_ACCESS */

/* All drivers should typically include these */
#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86_ansic.h"

/* Everything using inb/outb, etc needs "compiler.h" */
#include "compiler.h"

/* Drivers for PCI hardware need this */
#include "xf86PciInfo.h"

/* Drivers that need to access the PCI config space directly need this */
#include "xf86Pci.h"

/* Driver specific headers */
#include "ct_driver.h"

#ifdef	__arm32__
/*#include <machine/sysarch.h>*/
#define	arm32_drain_writebuf()	sysarch(1, 0)
#define ChipsBank(pScreen) CHIPSPTR(xf86Screens[pScreen->myNum])->Bank
#endif

#ifdef DIRECT_REGISTER_ACCESS
int
CHIPSSetRead(ScreenPtr pScreen, int bank)
{ 
    outw(0x3D6, ((((bank << 3) & 0xFF) << 8) | 0x10));

#ifdef	__arm32__
    /* Must drain StrongARM write buffer on bank switch! */
    if (bank != ChipsBank(pScreen)) {
	arm32_drain_writebuf();
	ChipsBank(pScreen) = bank;
    }
#endif

    return 0;
}


int
CHIPSSetWrite(ScreenPtr pScreen, int bank)
{
    outw(0x3D6, ((((bank << 3) & 0xFF) << 8) | 0x11));

#ifdef	__arm32__
    /* Must drain StrongARM write buffer on bank switch! */
    if (bank != ChipsBank(pScreen)) {
	arm32_drain_writebuf();
	ChipsBank(pScreen) = bank;
    }
#endif

    return 0;
}


int
CHIPSSetReadWrite(ScreenPtr pScreen, int bank)
{
    outw(0x3D6, ((((bank << 3) & 0xFF) << 8) | 0x10));
    outw(0x3D6, ((((bank << 3) & 0xFF) << 8) | 0x11));

#ifdef	__arm32__
    /* Must drain StrongARM write buffer on bank switch! */
    if (bank != ChipsBank(pScreen)) {
	arm32_drain_writebuf();
	ChipsBank(pScreen) = bank;
    }
#endif

    return 0;
}

int
CHIPSSetReadPlanar(ScreenPtr pScreen, int bank)
{
    outw(0x3D6, ((((bank << 5) & 0xFF) << 8) | 0x10));

#ifdef	__arm32__
    /* Must drain StrongARM write buffer on bank switch! */
    if (bank != ChipsBank(pScreen)) {
	arm32_drain_writebuf();
	ChipsBank(pScreen) = bank;
    }
#endif

    return 0;
}

int
CHIPSSetWritePlanar(ScreenPtr pScreen, int bank)
{
    outw(0x3D6, ((((bank << 5) & 0xFF) << 8) | 0x11));

#ifdef	__arm32__
    /* Must drain StrongARM write buffer on bank switch! */
    if (bank != ChipsBank(pScreen)) {
	arm32_drain_writebuf();
	ChipsBank(pScreen) = bank;
    }
#endif

    return 0;
}

int
CHIPSSetReadWritePlanar(ScreenPtr pScreen, int bank)
{
    outw(0x3D6, ((((bank << 5) & 0xFF) << 8) | 0x10));
    outw(0x3D6, ((((bank << 5) & 0xFF) << 8) | 0x11));

#ifdef	__arm32__
    /* Must drain StrongARM write buffer on bank switch! */
    if (bank != ChipsBank(pScreen)) {
	arm32_drain_writebuf();
	ChipsBank(pScreen) = bank;
    }
#endif

    return 0;
}

int
CHIPSWINSetRead(ScreenPtr pScreen, int bank)
{
    register unsigned char tmp;

    outw(0x3D6, ((((bank << 3) & 0xFF) << 8) | 0x10));
    outb(0x3D6, 0x0C);
    tmp = inb(0x3D7) & 0xEF;
    outw(0x3D6, (((((bank >> 1) & 0x10) | tmp) << 8) | 0x0C));

#ifdef	__arm32__
    /* Must drain StrongARM write buffer on bank switch! */
    if (bank != ChipsBank(pScreen)) {
	arm32_drain_writebuf();
	ChipsBank(pScreen) = bank;
    }
#endif

    return 0;
}


int
CHIPSWINSetWrite(ScreenPtr pScreen, int bank)
{
    register unsigned char tmp;

    outw(0x3D6, ((((bank << 3) & 0xFF) << 8) | 0x11));
    outb(0x3D6, 0x0C);
    tmp = inb(0x3D7) & 0xBF;
    outw(0x3D6, (((((bank << 1) & 0x40) | tmp) << 8) | 0x0C));

#ifdef	__arm32__
    /* Must drain StrongARM write buffer on bank switch! */
    if (bank != ChipsBank(pScreen)) {
	arm32_drain_writebuf();
	ChipsBank(pScreen) = bank;
    }
#endif

    return 0;
}

int
CHIPSWINSetReadWrite(ScreenPtr pScreen, int bank)
{
    register unsigned char tmp;

    outw(0x3D6, ((((bank << 3) & 0xFF) << 8) | 0x10));
    outw(0x3D6, ((((bank << 3) & 0xFF) << 8) | 0x11));
    outb(0x3D6, 0x0C);
    tmp = inb(0x3D7) & 0xAF;
    outw(0x3D6, (((((bank << 1) & 0x40) | ((bank >> 1) & 0x10) | tmp) << 8) | 0x0C));

#ifdef	__arm32__
    /* Must drain StrongARM write buffer on bank switch! */
    if (bank != ChipsBank(pScreen)) {
	arm32_drain_writebuf();
	ChipsBank(pScreen) = bank;
    }
#endif

    return 0;
}

int
CHIPSWINSetReadPlanar(ScreenPtr pScreen, int bank)
{
    register unsigned char tmp;

    outw(0x3D6, ((((bank << 5) & 0xFF) << 8) | 0x10));
    outb(0x3D6, 0x0C);
    tmp = inb(0x3D7) & 0xEF;
    outw(0x3D6, (((((bank << 1) & 0x10) | tmp) << 8) | 0x0C));

#ifdef	__arm32__
    /* Must drain StrongARM write buffer on bank switch! */
    if (bank != ChipsBank(pScreen)) {
	arm32_drain_writebuf();
	ChipsBank(pScreen) = bank;
    }
#endif

    return 0;
}

int
CHIPSWINSetWritePlanar(ScreenPtr pScreen, int bank)
{
    register unsigned char tmp;

    outw(0x3D6, ((((bank << 5) & 0xFF) << 8) | 0x11));
    outb(0x3D6, 0x0C);
    tmp = inb(0x3D7) & 0xBF;
    outw(0x3D6, (((((bank << 3) & 0x40) | tmp) << 8) | 0x0C));

#ifdef	__arm32__
    /* Must drain StrongARM write buffer on bank switch! */
    if (bank != ChipsBank(pScreen)) {
	arm32_drain_writebuf();
	ChipsBank(pScreen) = bank;
    }
#endif

    return 0;
}

int
CHIPSWINSetReadWritePlanar(ScreenPtr pScreen, int bank)
{
    register unsigned char tmp;

    outw(0x3D6, ((((bank << 5) & 0xFF) << 8) | 0x10));
    outw(0x3D6, ((((bank << 5) & 0xFF) << 8) | 0x11));
    outb(0x3D6, 0x0C);
    tmp = inb(0x3D7) & 0xAF;
    outw(0x3D6, (((((bank << 3) & 0x40) | ((bank << 1) & 0x10) | tmp) << 8) | 0x0C));

#ifdef	__arm32__
    /* Must drain StrongARM write buffer on bank switch! */
    if (bank != ChipsBank(pScreen)) {
	arm32_drain_writebuf();
	ChipsBank(pScreen) = bank;
    }
#endif

    return 0;
}

int
CHIPSHiQVSetReadWrite(ScreenPtr pScreen, int bank)
{
    outw(0x3D6, (((bank & 0x7F) << 8) | 0x0E));

#ifdef	__arm32__
    /* Must drain StrongARM write buffer on bank switch! */
    if (bank != ChipsBank(pScreen)) {
	arm32_drain_writebuf();
	ChipsBank(pScreen) = bank;
    }
#endif

    return 0;
}

int
CHIPSHiQVSetReadWritePlanar(ScreenPtr pScreen, int bank)
{
    outw(0x3D6, ((((bank << 2) & 0x7F) << 8) | 0x0E));

#ifdef	__arm32__
    /* Must drain StrongARM write buffer on bank switch! */
    if (bank != ChipsBank(pScreen)) {
	arm32_drain_writebuf();
	ChipsBank(pScreen) = bank;
    }
#endif

    return 0;
}

#else /* DIRECT_REGISTER_ACCESS */

int
CHIPSSetRead(ScreenPtr pScreen, int bank)
{ 
    CHIPSPtr cPtr = CHIPSPTR(xf86Screens[pScreen->myNum]);
  
    cPtr->writeXR(cPtr, 0x10, ((bank << 3) & 0xFF));

#ifdef	__arm32__
    /* Must drain StrongARM write buffer on bank switch! */
    if (bank != cPtr->Bank) {
	arm32_drain_writebuf();
	cPtr->Bank = bank;
    }
#endif

    return 0;
}


int
CHIPSSetWrite(ScreenPtr pScreen, int bank)
{
    CHIPSPtr cPtr = CHIPSPTR(xf86Screens[pScreen->myNum]);
  
    cPtr->writeXR(cPtr, 0x11, ((bank << 3) & 0xFF));

#ifdef	__arm32__
    /* Must drain StrongARM write buffer on bank switch! */
    if (bank != cPtr->Bank) {
	arm32_drain_writebuf();
	cPtr->Bank = bank;
    }
#endif

    return 0;
}


int
CHIPSSetReadWrite(ScreenPtr pScreen, int bank)
{
    CHIPSPtr cPtr = CHIPSPTR(xf86Screens[pScreen->myNum]);
  
    cPtr->writeXR(cPtr, 0x10, ((bank << 3) & 0xFF));
    cPtr->writeXR(cPtr, 0x11, ((bank << 3) & 0xFF));

#ifdef	__arm32__
    /* Must drain StrongARM write buffer on bank switch! */
    if (bank != cPtr->Bank) {
	arm32_drain_writebuf();
	cPtr->Bank = bank;
    }
#endif

    return 0;
}

int
CHIPSSetReadPlanar(ScreenPtr pScreen, int bank)
{
    CHIPSPtr cPtr = CHIPSPTR(xf86Screens[pScreen->myNum]);
  
    cPtr->writeXR(cPtr, 0x10, ((bank << 5) & 0xFF));

#ifdef	__arm32__
    /* Must drain StrongARM write buffer on bank switch! */
    if (bank != cPtr->Bank) {
	arm32_drain_writebuf();
	cPtr->Bank = bank;
    }
#endif

    return 0;
}

int
CHIPSSetWritePlanar(ScreenPtr pScreen, int bank)
{
    CHIPSPtr cPtr = CHIPSPTR(xf86Screens[pScreen->myNum]);
  
    cPtr->writeXR(cPtr, 0x11, ((bank << 5) & 0xFF));

#ifdef	__arm32__
    /* Must drain StrongARM write buffer on bank switch! */
    if (bank != cPtr->Bank) {
	arm32_drain_writebuf();
	cPtr->Bank = bank;
    }
#endif

    return 0;
}

int
CHIPSSetReadWritePlanar(ScreenPtr pScreen, int bank)
{
    CHIPSPtr cPtr = CHIPSPTR(xf86Screens[pScreen->myNum]);
  
    cPtr->writeXR(cPtr, 0x10, ((bank << 5) & 0xFF));
    cPtr->writeXR(cPtr, 0x11, ((bank << 5) & 0xFF));

#ifdef	__arm32__
    /* Must drain StrongARM write buffer on bank switch! */
    if (bank != cPtr->Bank) {
	arm32_drain_writebuf();
	cPtr->Bank = bank;
    }
#endif

    return 0;
}

int
CHIPSWINSetRead(ScreenPtr pScreen, int bank)
{
    CHIPSPtr cPtr = CHIPSPTR(xf86Screens[pScreen->myNum]);
    register unsigned char tmp;
  
    cPtr->writeXR(cPtr, 0x10, ((bank << 3) & 0xFF));
    tmp = cPtr->readXR(cPtr, 0x0C) & 0xEF;
    cPtr->writeXR(cPtr, 0x0C, ((bank >> 1) & 0x10) | tmp);

#ifdef	__arm32__
    /* Must drain StrongARM write buffer on bank switch! */
    if (bank != cPtr->Bank) {
	arm32_drain_writebuf();
	cPtr->Bank = bank;
    }
#endif

    return 0;
}


int
CHIPSWINSetWrite(ScreenPtr pScreen, int bank)
{
    CHIPSPtr cPtr = CHIPSPTR(xf86Screens[pScreen->myNum]);
    register unsigned char tmp;

    cPtr->writeXR(cPtr, 0x11, ((bank << 3) & 0xFF));
    tmp = cPtr->readXR(cPtr, 0x0C) & 0xBF;
    cPtr->writeXR(cPtr, 0x0C, ((bank << 1) & 0x40) | tmp);

#ifdef	__arm32__
    /* Must drain StrongARM write buffer on bank switch! */
    if (bank != cPtr->Bank) {
	arm32_drain_writebuf();
	cPtr->Bank = bank;
    }
#endif

    return 0;
}

int
CHIPSWINSetReadWrite(ScreenPtr pScreen, int bank)
{
    CHIPSPtr cPtr = CHIPSPTR(xf86Screens[pScreen->myNum]);
    register unsigned char tmp;

    cPtr->writeXR(cPtr, 0x10, ((bank << 3) & 0xFF));
    cPtr->writeXR(cPtr, 0x11, ((bank << 3) & 0xFF));
    tmp = cPtr->readXR(cPtr, 0x0C) & 0xAF;
    cPtr->writeXR(cPtr, 0x0C, ((bank << 1) & 0x40) | ((bank >> 1) & 0x10) | tmp);

#ifdef	__arm32__
    /* Must drain StrongARM write buffer on bank switch! */
    if (bank != cPtr->Bank) {
	arm32_drain_writebuf();
	cPtr->Bank = bank;
    }
#endif

    return 0;
}

int
CHIPSWINSetReadPlanar(ScreenPtr pScreen, int bank)
{
    CHIPSPtr cPtr = CHIPSPTR(xf86Screens[pScreen->myNum]);
    register unsigned char tmp;

    cPtr->writeXR(cPtr, 0x10, ((bank << 5) & 0xFF));
    tmp = cPtr->readXR(cPtr, 0x0C) & 0xEF;
    cPtr->writeXR(cPtr, 0x0C, ((bank << 1) & 0x10) | tmp);

#ifdef	__arm32__
    /* Must drain StrongARM write buffer on bank switch! */
    if (bank != cPtr->Bank) {
	arm32_drain_writebuf();
	cPtr->Bank = bank;
    }
#endif

    return 0;
}

int
CHIPSWINSetWritePlanar(ScreenPtr pScreen, int bank)
{
    CHIPSPtr cPtr = CHIPSPTR(xf86Screens[pScreen->myNum]);
    register unsigned char tmp;

    cPtr->writeXR(cPtr, 0x11, ((bank << 5) & 0xFF));
    tmp = cPtr->readXR(cPtr, 0x0C) & 0xBF;
    cPtr->writeXR(cPtr, 0x0C, ((bank << 3) & 0x40) | tmp);

#ifdef	__arm32__
    /* Must drain StrongARM write buffer on bank switch! */
    if (bank != cPtr->Bank) {
	arm32_drain_writebuf();
	cPtr->Bank = bank;
    }
#endif

    return 0;
}

int
CHIPSWINSetReadWritePlanar(ScreenPtr pScreen, int bank)
{
    CHIPSPtr cPtr = CHIPSPTR(xf86Screens[pScreen->myNum]);
    register unsigned char tmp;

    cPtr->writeXR(cPtr, 0x10, ((bank << 5) & 0xFF));
    cPtr->writeXR(cPtr, 0x11, ((bank << 5) & 0xFF));
    tmp = cPtr->readXR(cPtr, 0x0C) & 0xAF;
    cPtr->writeXR(cPtr, 0x0C, ((bank << 3) & 0x40) | ((bank << 1) & 0x10) | tmp);

#ifdef	__arm32__
    /* Must drain StrongARM write buffer on bank switch! */
    if (bank != cPtr->Bank) {
	arm32_drain_writebuf();
	cPtr->Bank = bank;
    }
#endif

    return 0;
}

int
CHIPSHiQVSetReadWrite(ScreenPtr pScreen, int bank)
{
    CHIPSPtr cPtr = CHIPSPTR(xf86Screens[pScreen->myNum]);

    cPtr->writeXR(cPtr, 0x0E, bank & 0x7F);

#ifdef	__arm32__
    /* Must drain StrongARM write buffer on bank switch! */
    if (bank != cPtr->Bank) {
	arm32_drain_writebuf();
	cPtr->Bank = bank;
    }
#endif

    return 0;
}

int
CHIPSHiQVSetReadWritePlanar(ScreenPtr pScreen, int bank)
{
    CHIPSPtr cPtr = CHIPSPTR(xf86Screens[pScreen->myNum]);

    cPtr->writeXR(cPtr, 0x0E, (bank << 2) & 0x7F);

#ifdef	__arm32__
    /* Must drain StrongARM write buffer on bank switch! */
    if (bank != cPtr->Bank) {
	arm32_drain_writebuf();
	cPtr->Bank = bank;
    }
#endif

    return 0;
}
#endif
