/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/chips/ct_cursor.c,v 1.20 2000/04/04 19:25:06 dawes Exp $ */

/*
 * Copyright 1994  The XFree86 Project
 *
 * Permission is hereby granted, free of charge, to any person obtaining a 
 * copy of this software and associated documentation files (the "Software"), 
 * to deal in the Software without restriction, including without limitation 
 * the rights to use, copy, modify, merge, publish, distribute, sublicense, 
 * and/or sell copies of the Software, and to permit persons to whom the 
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL 
 * DAVID WEXELBLAT BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF 
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE 
 * SOFTWARE.
 * 
 * Hardware Cursor for Trident utilizing XAA Cursor code.
 * Written by Alan Hourihane <alanh@fairlite.demon.co.uk>
 * Modified for Chips and Technologies by David Bateman <dbateman@eng.uts.edu.au>
 */

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

/* All drivers using the vgahw module need this */
/* This driver needs to be modified to not use vgaHW for multihead operation */
#include "vgaHW.h"

#include "xf86Cursor.h"

/* Driver specific headers */
#include "ct_driver.h"

/* Sync function, maybe this should check infoRec->NeedToSync before syncing */
#define CURSOR_SYNC(pScrn) \
    if (IS_HiQV(cPtr)) { \
	CHIPSHiQVSync(pScrn); \
    } else { \
	if(!cPtr->UseMMIO) { \
	    CHIPSSync(pScrn); \
	} else { \
	    CHIPSMMIOSync(pScrn); \
	} \
    }


static void
CHIPSShowCursor(ScrnInfoPtr pScrn)
{
    CHIPSPtr cPtr = CHIPSPTR(pScrn);
    unsigned char tmp;

    CURSOR_SYNC(pScrn);
    
    /* turn the cursor on */
    if (IS_HiQV(cPtr)) {
	tmp = cPtr->readXR(cPtr, 0xA0);
	cPtr->writeXR(cPtr, 0xA0, (tmp & 0xF8) | 5);
    } else {
	if(!cPtr->UseMMIO) {
	    HW_DEBUG(0x8);
	    outw(DR(0x8), 0x21);
	} else {
	    HW_DEBUG(DR(8));
	    /*  Used to be: MMIOmemw(MR(8)) = 0x21; */
	    MMIOmeml(MR(8)) = 0x21;
	}
    }
    cPtr->HWCursorShown = TRUE;
}

static void
CHIPSHideCursor(ScrnInfoPtr pScrn)
{
    CHIPSPtr cPtr = CHIPSPTR(pScrn);
    unsigned char tmp;
  
    CURSOR_SYNC(pScrn);

    /* turn the cursor off */
    if (IS_HiQV(cPtr)) {
	tmp = cPtr->readXR(cPtr, 0xA0);
	cPtr->writeXR(cPtr, 0xA0, tmp & 0xF8);
    } else {
	if(!cPtr->UseMMIO) {
	    HW_DEBUG(0x8);
	    outw(DR(0x8), 0x20);
	} else {
	    HW_DEBUG(DR(0x8));
	    /* Used to be: MMIOmemw(DR(0x8)) = 0x20; */
	    MMIOmeml(DR(0x8)) = 0x20;
	}
    }
    cPtr->HWCursorShown = FALSE;
}

static void
CHIPSSetCursorPosition(ScrnInfoPtr pScrn, int x, int y)
{
    CHIPSPtr cPtr = CHIPSPTR(pScrn);
    
    CURSOR_SYNC(pScrn);

    if (pScrn->currentMode->Flags & V_DBLSCAN)
	y *= 2;

    if (x < 0)
	x = ~(x-1) | 0x8000;
    if (y < 0)
	y = ~(y-1) | 0x8000;

    /* Program the cursor origin (offset into the cursor bitmap). */
    if (IS_HiQV(cPtr)) {
	cPtr->writeXR(cPtr, 0xA4, x & 0xFF);
	cPtr->writeXR(cPtr, 0xA5, (x >> 8) & 0x87);
	cPtr->writeXR(cPtr, 0xA6, y & 0xFF);
	cPtr->writeXR(cPtr, 0xA7, (y >> 8) & 0x87);
    } else {
	CARD32 xy;

	xy = y;
	xy = (xy << 16) | x;
	if(!cPtr->UseMMIO) {
	    HW_DEBUG(0xB);
	    outl(DR(0xB), xy);
	} else {
	    HW_DEBUG(MR(0xB));
	    MMIOmeml(MR(0xB)) = xy;
	}
    }
}

static void
CHIPSSetCursorColors(ScrnInfoPtr pScrn, int bg, int fg)
{
    CHIPSPtr cPtr = CHIPSPTR(pScrn);
    vgaHWPtr hwp = VGAHWPTR(pScrn);
    CARD32 packedcolfg, packedcolbg;
    
    CURSOR_SYNC(pScrn);

    if (IS_HiQV(cPtr)) {
	unsigned char xr80;

	/* Enable extended palette addressing */
	xr80 = cPtr->readXR(cPtr, 0x80);
	cPtr->writeXR(cPtr, 0x80, xr80 | 0x1);

	/* Write the new colours to the extended VGA palette. Palette
	 * index is incremented after each write, so only write index
	 * once 
	 */
	hwp->writeDacWriteAddr(hwp, 0x04);
	if (xr80 & 0x80) {
	    /* 8bit DAC */
	    hwp->writeDacData(hwp, (bg >> 16) & 0xFF);
	    hwp->writeDacData(hwp, (bg >> 8) & 0xFF);
	    hwp->writeDacData(hwp, bg & 0xFF);
	    hwp->writeDacData(hwp, (fg >> 16) & 0xFF);
	    hwp->writeDacData(hwp, (fg >> 8) & 0xFF);
	    hwp->writeDacData(hwp, fg & 0xFF);
	} else {
	    /* 6bit DAC */
	    hwp->writeDacData(hwp, (bg >> 18) & 0xFF);
	    hwp->writeDacData(hwp, (bg >> 10) & 0xFF);
	    hwp->writeDacData(hwp, (bg >> 2) & 0xFF);
	    hwp->writeDacData(hwp, (fg >> 18) & 0xFF);
	    hwp->writeDacData(hwp, (fg >> 10) & 0xFF);
	    hwp->writeDacData(hwp, (fg >> 2) & 0xFF);
	}
	/* Enable normal palette addressing */
	cPtr->writeXR(cPtr, 0x80, xr80);
    } else if (IS_Wingine(cPtr)) {
	outl(DR(0xA), (bg & 0xFFFFFF));
	outl(DR(0x9), (fg & 0xFFFFFF));
    } else {
	packedcolfg =  ((fg & 0xF80000) >> 8) | ((fg & 0xFC00) >> 5)
	    | ((fg & 0xF8) >> 3);
	packedcolbg =  ((bg & 0xF80000) >> 8) | ((bg & 0xFC00) >> 5)
	    | ((bg & 0xF8) >> 3);
	packedcolfg = (packedcolfg << 16) | packedcolbg;
	if(!cPtr->UseMMIO) {
	    HW_DEBUG(0x9);
	    outl(DR(0x9), packedcolfg);
	} else {
	    MMIOmeml(MR(0x9)) = packedcolfg;
	    HW_DEBUG(MR(0x9));
	}
    }
}

static void
CHIPSLoadCursorImage(ScrnInfoPtr pScrn, unsigned char *src)
{
    CHIPSPtr cPtr = CHIPSPTR(pScrn);
    CHIPSACLPtr cAcl = CHIPSACLPTR(pScrn);

    CURSOR_SYNC(pScrn);

    if (IS_Wingine(cPtr)) {
	int i;
	CARD32 *tmp = (CARD32 *)src;
	
	outl(DR(0x8),0x20);
	for (i=0; i<64; i++) {
	    outl(DR(0xC),*(CARD32 *)tmp);
	    tmp++;
	}
    } else {
	if (cPtr->Flags & ChipsLinearSupport) {
	    memcpy((unsigned char *)cPtr->FbBase + cAcl->CursorAddress,
			src, cPtr->CursorInfoRec->MaxWidth * 
			cPtr->CursorInfoRec->MaxHeight / 4);
	} else {
	    /*
	     * The cursor can only be in the last 16K of video memory,
	     * which fits in the last banking window.
	     */
	    if (IS_HiQV(cPtr))
		if (pScrn->bitsPerPixel < 8)
		    CHIPSHiQVSetReadWritePlanar(pScrn->pScreen, 
					    (int)(cAcl->CursorAddress >> 16));
		else
		    CHIPSHiQVSetReadWrite(pScrn->pScreen,
					    (int)(cAcl->CursorAddress >> 16));
	    else
		if (pScrn->bitsPerPixel < 8)
		    CHIPSSetWritePlanar(pScrn->pScreen,
					    (int)(cAcl->CursorAddress >> 16));
		else
		    CHIPSSetWrite(pScrn->pScreen,
					    (int)(cAcl->CursorAddress >> 16));
	    memcpy((unsigned char *)cPtr->FbBase + (cAcl->CursorAddress &
			0xFFFF), src,  cPtr->CursorInfoRec->MaxWidth * 
			cPtr->CursorInfoRec->MaxHeight / 4);
	}
    }

    /* set cursor address here or we loose the cursor on video mode change */
    if (IS_HiQV(cPtr)) {
	cPtr->writeXR(cPtr, 0xA2, (cAcl->CursorAddress >> 8) & 0xFF);
	cPtr->writeXR(cPtr, 0xA3, (cAcl->CursorAddress >> 16) & 0x3F);
    } else if (!IS_Wingine(cPtr)) {
	if (!cPtr->UseMMIO) {
	    HW_DEBUG(0xC);
	    outl(DR(0xC), cAcl->CursorAddress);
	} else {
	    HW_DEBUG(MR(0xC));
	    MMIOmeml(MR(0xC)) = cAcl->CursorAddress;
	}
    }
}

static Bool
CHIPSUseHWCursor(ScreenPtr pScr, CursorPtr pCurs)
{
    CHIPSACLPtr cAcl = CHIPSACLPTR(xf86Screens[pScr->myNum]);

    return cAcl->UseHWCursor;
}

Bool
CHIPSCursorInit(ScreenPtr pScreen)
{
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    CHIPSPtr cPtr = CHIPSPTR(pScrn);
    xf86CursorInfoPtr infoPtr;
    
    infoPtr = xf86CreateCursorInfoRec();
    if(!infoPtr) return FALSE;

    cPtr->CursorInfoRec = infoPtr;

    infoPtr->Flags = HARDWARE_CURSOR_BIT_ORDER_MSBFIRST |
	HARDWARE_CURSOR_INVERT_MASK |
	HARDWARE_CURSOR_SWAP_SOURCE_AND_MASK |
	HARDWARE_CURSOR_TRUECOLOR_AT_8BPP;

    if (IS_HiQV(cPtr)) {
	infoPtr->Flags |= HARDWARE_CURSOR_SOURCE_MASK_INTERLEAVE_64;
	infoPtr->MaxHeight = 64;
	infoPtr->MaxWidth = 64;
    } else if (IS_Wingine(cPtr)) {
	infoPtr->Flags |= HARDWARE_CURSOR_SOURCE_MASK_NOT_INTERLEAVED;
	infoPtr->MaxHeight = 32;
	infoPtr->MaxWidth = 32;      
    } else {
	infoPtr->Flags |= HARDWARE_CURSOR_SOURCE_MASK_INTERLEAVE_8;
	infoPtr->MaxHeight = 32;
	infoPtr->MaxWidth = 32;
    }

    infoPtr->SetCursorColors = CHIPSSetCursorColors;
    infoPtr->SetCursorPosition = CHIPSSetCursorPosition;
    infoPtr->LoadCursorImage = CHIPSLoadCursorImage;
    infoPtr->HideCursor = CHIPSHideCursor;
    infoPtr->ShowCursor = CHIPSShowCursor;
    infoPtr->UseHWCursor = CHIPSUseHWCursor;

    return(xf86InitCursor(pScreen, infoPtr));
}

