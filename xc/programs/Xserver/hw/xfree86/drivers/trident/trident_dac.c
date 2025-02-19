/*
 * Copyright 1992-2000 by Alan Hourihane, Wigan, England.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Alan Hourihane not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Alan Hourihane makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * ALAN HOURIHANE DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL ALAN HOURIHANE BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 * Author:  Alan Hourihane, alanh@fairlite.demon.co.uk
 */
/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/trident/trident_dac.c,v 1.37 2000/12/14 19:29:44 dawes Exp $ */

#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86_ansic.h"
#include "xf86Version.h"
#include "xf86PciInfo.h"
#include "xf86Pci.h"

#include "vgaHW.h"

#include "trident.h"
#include "trident_regs.h"


static biosMode bios1[] = { 
    { 640, 480, 0x11 }
};

static biosMode bios4[] = {
    { 320, 200, 0xd },
    { 640, 200, 0xe },
    { 640, 350, 0x11 },
    { 640, 480, 0x12 },
    { 800, 600, 0x5b },
    { 1024, 768 , 0x5f },
    { 1280, 1024, 0x63 },
    { 1600, 1200, 0x65 }
};

static biosMode bios8[] = {	
    { 320, 200, 0x13 },
    { 640, 400, 0x5c },
    { 640, 480, 0x5d },
    { 720, 480, 0x60 },
    { 800, 600, 0x5e },
    { 1024, 768, 0x62 },
    { 1280, 1024, 0x64 },
    { 1600, 1200, 0x66 }
};

static biosMode bios15[] = {
    { 640, 400, 0x72 },
    { 640, 480, 0x74 },
    { 720, 480, 0x70 },
    { 800, 600, 0x76 },
    { 1024, 768, 0x78 },
    { 1280, 1024, 0x7a },
    { 1600, 1200, 0x7c }
};

static biosMode bios16[] = {
    { 640, 400, 0x73 },
    { 640, 480, 0x75 },
    { 720, 480, 0x71 },
    { 800, 600, 0x77 },
    { 1024, 768, 0x79 },
    { 1280, 1024, 0x7b },
    { 1600, 1200, 0x7d }
};

static biosMode bios24[] = {
    { 640, 400, 0x6b },
    { 640, 480, 0x6c },
    { 720, 480, 0x61 },
    { 800, 600, 0x6d },
    { 1024, 768, 0x6e }
};


int
TridentFindMode(int xres, int yres, int depth)
{
    int xres_s;
    int i, size;
    biosMode *mode;

    switch (depth) {
    case 1:
	size = sizeof(bios1) / sizeof(biosMode);
	mode = bios1;
	break;
    case 4:
	size = sizeof(bios4) / sizeof(biosMode);
	mode = bios4;
	break;
    case 8:
	size = sizeof(bios8) / sizeof(biosMode);
	mode = bios8;
	break;
    case 15:
	size = sizeof(bios15) / sizeof(biosMode);
	mode = bios15;
	break;
    case 16:
	size = sizeof(bios16) / sizeof(biosMode);
	mode = bios16;
	break;
    case 24:
	size = sizeof(bios24) / sizeof(biosMode);
	mode = bios24;
	break;
    default:
	return 0;
    }

    for (i = 0; i < size; i++) {
	if (xres <= mode[i].x_res) {
	    xres_s = mode[i].x_res;
	    for (; i < size; i++) {
		if (mode[i].x_res != xres_s)
		    return mode[i-1].mode;
		if (yres <= mode[i].y_res)
		    return mode[i].mode;
	    }
	}
    }
    return mode[size - 1].mode;
    
}

Bool
TridentInit(ScrnInfoPtr pScrn, DisplayModePtr mode)
{
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);
    TRIDENTRegPtr pReg = &pTrident->ModeReg;
    int vgaIOBase;
    int offset = 0;
    int clock = pTrident->currentClock;
    CARD8 protect;

    vgaHWPtr hwp = VGAHWPTR(pScrn);
    vgaRegPtr vgaReg = &hwp->ModeReg;
    vgaHWGetIOBase(VGAHWPTR(pScrn));
    vgaIOBase = VGAHWPTR(pScrn)->IOBase;
    
    /* Unprotect */
    OUTB(0x3C4, 0x11);
    protect = INB(0x3C4);
    OUTB(0x3C5, 0x92);

    OUTB(0x3C4, 0x0B); INB(0x3C5); /* Ensure we are in New Mode */

    pReg->tridentRegs3x4[PixelBusReg] = 0x00;
    pReg->tridentRegsDAC[0x00] = 0x00;
    pReg->tridentRegs3C4[NewMode2] = 0x20;
    OUTB(0x3CE, MiscExtFunc);
    pReg->tridentRegs3CE[MiscExtFunc] = INB(0x3CF) & 0xF0;
    pReg->tridentRegs3x4[GraphEngReg] = 0x00; 
    pReg->tridentRegs3x4[PreEndControl] = 0;
    pReg->tridentRegs3x4[PreEndFetch] = 0;

    pReg->tridentRegs3x4[CRTHiOrd] = (((mode->CrtcVBlankEnd-1) & 0x400)>>4) |
 				     (((mode->CrtcVTotal - 2) & 0x400) >> 3) |
 				     ((mode->CrtcVSyncStart & 0x400) >> 5) |
 				     (((mode->CrtcVDisplay - 1) & 0x400) >> 6)|
 				     0x08;

    if (pTrident->IsCyber) {
	Bool LCDActive;
#ifdef READOUT
	Bool ShadowModeActive;
#endif
	int i = pTrident->lcdMode;
#ifdef READOUT
	OUTB(0x3CE, CyberControl);
	ShadowModeActive = ((INB(0x3CF) & 0x81) == 0x81);
#endif
	OUTB(0x3CE, FPConfig);
	LCDActive = (INB(0x3CF) & 0x10);
	
	OUTB(0x3CE, CyberEnhance); 
	pReg->tridentRegs3CE[CyberEnhance] = INB(0x3CF) & 0x8F;
	if (mode->CrtcVDisplay > 768)
	    pReg->tridentRegs3CE[CyberEnhance] |= 0x30;
	else
	if (mode->CrtcVDisplay > 600)
	    pReg->tridentRegs3CE[CyberEnhance] |= 0x20;
	else
	if (mode->CrtcVDisplay > 480)
	    pReg->tridentRegs3CE[CyberEnhance] |= 0x10;

	OUTB(0x3CE, CyberControl);
	pReg->tridentRegs3CE[CyberControl] = INB(0x3CF);

	OUTB(0x3CE,HorStretch);
	pReg->tridentRegs3CE[HorStretch] = INB(0x3CF);
	OUTB(0x3CE,VertStretch);
	pReg->tridentRegs3CE[VertStretch] = INB(0x3CF);
	
#ifdef READOUT
	if ((!((pReg->tridentRegs3CE[VertStretch] & 1) ||
	    (pReg->tridentRegs3CE[HorStretch] & 1)))
	    && (!LCDActive || ShadowModeActive)) {
	    unsigned char tmp;
	    
	    SHADOW_ENABLE(tmp);
	    OUTB(vgaIOBase + 4,0);
	    pReg->tridentRegs3x4[0x0] = INB(vgaIOBase + 5);
	    OUTB(vgaIOBase + 4,3);
	    pReg->tridentRegs3x4[0x3] = INB(vgaIOBase + 5);
	    OUTB(vgaIOBase + 4,4);
	    pReg->tridentRegs3x4[0x4] = INB(vgaIOBase + 5);
	    OUTB(vgaIOBase + 4,5);
  	    pReg->tridentRegs3x4[0x5] = INB(vgaIOBase + 5);
  	    OUTB(vgaIOBase + 4,0x6);
  	    pReg->tridentRegs3x4[0x6] = INB(vgaIOBase + 5);
  	    SHADOW_RESTORE(tmp);
 	} else
#endif
 	{
 	    if (i != 0xff) {
  		pReg->tridentRegs3x4[0x0] = LCD[i].shadow_0;
  		pReg->tridentRegs3x4[0x3] = LCD[i].shadow_3;
  		pReg->tridentRegs3x4[0x4] = LCD[i].shadow_4;
  		pReg->tridentRegs3x4[0x5] = LCD[i].shadow_5;
  		pReg->tridentRegs3x4[0x6] = LCD[i].shadow_6;
 		xf86DrvMsgVerb(pScrn->scrnIndex,X_INFO,1,
 			       "Overriding Horizontal timings.\n");
  	    }
  	}
 
 	if (i != 0xff) {
 	    pReg->tridentRegs3x4[0x7] = LCD[i].shadow_7;
 	    pReg->tridentRegs3x4[0x10] = LCD[i].shadow_10;
 	    pReg->tridentRegs3x4[0x11] = LCD[i].shadow_11;
 	    pReg->tridentRegs3x4[0x16] = LCD[i].shadow_16;
 	    if (LCDActive) 
 		pReg->tridentRegs3x4[CRTHiOrd] = LCD[i].shadow_HiOrd;
 	}
 	
 	xf86DrvMsgVerb(pScrn->scrnIndex,X_INFO,1,"Timing shadow registers:"
 		       "0x%2.2x 0x%2.2x 0x%2.2x 0x%2.2x 0x%2.2x 0x%2.2x "
 		       "0x%2.2x 0x%2.2x 0x%2.2x 0x%2.2x\n",
 		       pReg->tridentRegs3x4[0], pReg->tridentRegs3x4[3],
 		       pReg->tridentRegs3x4[4], pReg->tridentRegs3x4[5],
 		       pReg->tridentRegs3x4[6], pReg->tridentRegs3x4[7],
 		       pReg->tridentRegs3x4[0x10],pReg->tridentRegs3x4[0x11],
 		       pReg->tridentRegs3x4[0x16],
 		       pReg->tridentRegs3x4[CRTHiOrd]);
 	
  	/* copy over common bits from normal VGA */
  	
  	pReg->tridentRegs3x4[0x7] &= ~0x52;
	pReg->tridentRegs3x4[0x7] |= (vgaReg->CRTC[0x7] & 0x52);
	
	/* disable stretching, enable centering */
	pReg->tridentRegs3CE[VertStretch] &= 0xFE;
	pReg->tridentRegs3CE[VertStretch] |= 0x80;
	pReg->tridentRegs3CE[HorStretch] &= 0xFE;
	pReg->tridentRegs3CE[HorStretch] |= 0x80;
	
	{
	    int mul = pScrn->bitsPerPixel >> 3;
	    int val;
	    
	    if (!mul) mul = 1;
	    
	    /* this is what my BIOS does */ 
	    val = (pScrn->currentMode->HDisplay * mul / 8) + 16;
	    
	    pReg->tridentRegs3x4[PreEndControl] = 2 | ((val >> 8) & 1);
	    pReg->tridentRegs3x4[PreEndFetch] = val & 0xff;
	}
#if 0
	OUTB(vgaIOBase + 4,PreEndControl);
	pReg->tridentRegs3x4[PreEndControl] = INB(vgaIOBase + 5);
	OUTB(vgaIOBase + 4,PreEndFetch);
	pReg->tridentRegs3x4[PreEndFetch] = INB(vgaIOBase + 5);
#endif
	/* set mode */
	pReg->tridentRegs3CE[BiosMode] = TridentFindMode(
	    pScrn->currentMode->HDisplay,
	    pScrn->currentMode->VDisplay,
	    pScrn->depth);
	xf86DrvMsgVerb(pScrn->scrnIndex, X_INFO, 1, "Setting BIOS Mode: %x\n",
		       pReg->tridentRegs3CE[BiosMode]);
	
	/* no stretch */
	pReg->tridentRegs3CE[BiosReg] = 0;
	
	if (LCDActive) {
	    pReg->tridentRegs3CE[CyberControl] |= 0x81;
	    xf86DrvMsgVerb(pScrn->scrnIndex,X_INFO,1,"Shadow on\n");
	} else {
	    pReg->tridentRegs3CE[CyberControl] &= 0x7E;
	    xf86DrvMsgVerb(pScrn->scrnIndex,X_INFO,1,"Shadow off\n");
	}

	if (pTrident->CyberShadow) {
	    pReg->tridentRegs3CE[CyberControl] &= 0x7E;
	    xf86DrvMsgVerb(pScrn->scrnIndex,X_INFO,1,"Forcing Shadow off\n");
	}
    }

    /* Enable Chipset specific options */
    switch (pTrident->Chipset) {
	case CYBERBLADEI7:
	case CYBERBLADEI7D:
	case CYBERBLADEI1:
	case CYBERBLADEI1D:
	case CYBERBLADEAI1:
	case CYBERBLADEAI1D:
	case CYBERBLADEE4:
	case BLADE3D:
	    OUTB(vgaIOBase + 4, RAMDACTiming);
	    pReg->tridentRegs3x4[RAMDACTiming] = INB(vgaIOBase + 5) | 0x0F;
	    /* Fall Through */
	case CYBER9520:
	case CYBER9525DVD:
	case CYBER9397DVD:
	case CYBER9397:
	case CYBER9388:
	case IMAGE975:
	case IMAGE985:
	    if (pScrn->bitsPerPixel >= 8)
    	    	pReg->tridentRegs3CE[MiscExtFunc] |= 0x10;
	    if (!pReg->tridentRegs3x4[PreEndControl])
	    	pReg->tridentRegs3x4[PreEndControl] = 0x01;
	    if (!pReg->tridentRegs3x4[PreEndFetch])
	    	pReg->tridentRegs3x4[PreEndFetch] = 0xFF;
	    /* Fall Through */
	case PROVIDIA9685:
	    pReg->tridentRegs3x4[Enhancement0] = 0x40;
	    /* Fall Through */
	case PROVIDIA9682:
	    if (pTrident->UsePCIRetry) 
	    	pReg->tridentRegs3x4[PCIRetry] = 0xDF;
	    else
	    	pReg->tridentRegs3x4[PCIRetry] = 0x1F;
	    /* Fall Through */
	case TGUI9660:
	case TGUI9680:
	    if (pTrident->MUX && pScrn->bitsPerPixel == 8 && mode->CrtcHAdjusted) {
	    	pReg->tridentRegs3x4[PixelBusReg] |= 0x01; /* 16bit bus */
	    	pReg->tridentRegs3C4[NewMode2] |= 0x02; /* half clock */
    		pReg->tridentRegsDAC[0x00] |= 0x20;	/* mux mode */
	    }	
    }


    /* Defaults for all trident chipsets follows */
    switch (pScrn->bitsPerPixel) {
	case 1:
	case 4:
    	    offset = pScrn->displayWidth >> 4;
	    break;
	case 8:
	    pReg->tridentRegs3CE[MiscExtFunc] |= 0x02;
    	    offset = pScrn->displayWidth >> 3;
	    break;
	case 16:
	    pReg->tridentRegs3CE[MiscExtFunc] |= 0x02;
    	    offset = pScrn->displayWidth >> 2;
	    if (pScrn->depth == 15)
    	    	pReg->tridentRegsDAC[0x00] = 0x10;
	    else
	    	pReg->tridentRegsDAC[0x00] = 0x30;
    	    pReg->tridentRegs3x4[PixelBusReg] = 0x04;
	    /* Reload with any chipset specific stuff here */
	    if (pTrident->Chipset >= TGUI9660) 
		pReg->tridentRegs3x4[PixelBusReg] |= 0x01;
	    if (pTrident->Chipset == TGUI9440AGi) {
    	        pReg->tridentRegs3CE[MiscExtFunc] |= 0x08;/*Clock Division / 2*/
	        clock *= 2;	/* Double the clock */
	    }
	    break;
	case 24:
	    pReg->tridentRegs3CE[MiscExtFunc] |= 0x02;
    	    offset = (pScrn->displayWidth * 3) >> 3;
    	    pReg->tridentRegs3x4[PixelBusReg] = 0x29;
	    pReg->tridentRegsDAC[0x00] = 0xD0;
	    if (pTrident->Chipset == CYBERBLADEE4) {
    		OUTB(vgaIOBase+ 4, New32);
		pReg->tridentRegs3x4[New32] = INB(vgaIOBase + 5) & 0x7F;
	    }
	    break;
	case 32:
	    pReg->tridentRegs3CE[MiscExtFunc] |= 0x02;
    	    pReg->tridentRegs3CE[MiscExtFunc] |= 0x08; /* Clock Division by 2*/
	    clock *= 2;	/* Double the clock */
    	    offset = pScrn->displayWidth >> 1;
    	    pReg->tridentRegs3x4[PixelBusReg] = 0x09;
	    pReg->tridentRegsDAC[0x00] = 0xD0;
	    if (pTrident->Chipset == CYBERBLADEE4) {
    		OUTB(vgaIOBase+ 4, New32);
		pReg->tridentRegs3x4[New32] = INB(vgaIOBase + 5) | 0x80;
		/* With new mode 32bpp we set the packed flag */
    	    	pReg->tridentRegs3x4[PixelBusReg] |= 0x20;
	    }
	    break;
    }
    pReg->tridentRegs3x4[Offset] = offset & 0xFF;

    {
	CARD8 a, b;
	TGUISetClock(pScrn, clock, &a, &b);
	pReg->tridentRegsClock[0x00] = (INB(0x3CC) & 0xF3) | 0x08;
	pReg->tridentRegsClock[0x01] = a;
	pReg->tridentRegsClock[0x02] = b;
	if (pTrident->MCLK > 0) {
	    TGUISetMCLK(pScrn, pTrident->MCLK, &a, &b);
	    pReg->tridentRegsClock[0x03] = a;
	    pReg->tridentRegsClock[0x04] = b;
	}
    }

    pReg->tridentRegs3C4[NewMode1] = 0xC0;
    pReg->tridentRegs3C4[Protection] = 0x92;

    pReg->tridentRegs3x4[LinearAddReg] = 0;
    if (pTrident->Linear) {
	/* This is used for VLB, when we support it again in 4.0 */
	if (pTrident->Chipset < PROVIDIA9685)
    	    pReg->tridentRegs3x4[LinearAddReg] |=
					((pTrident->FbAddress >> 24) << 6)|
					((pTrident->FbAddress >> 20) & 0x0F);
	/* Turn on linear mapping */
    	pReg->tridentRegs3x4[LinearAddReg] |= 0x20; 
    } else {
	pReg->tridentRegs3CE[MiscExtFunc] |= 0x04;
    }
    
    pReg->tridentRegs3x4[CRTCModuleTest] = 
				(mode->Flags & V_INTERLACE ? 0x84 : 0x80);
    OUTB(vgaIOBase+ 4, InterfaceSel);
    pReg->tridentRegs3x4[InterfaceSel] = INB(vgaIOBase + 5) | 0x40;
    OUTB(vgaIOBase+ 4, Performance);
    pReg->tridentRegs3x4[Performance] = INB(vgaIOBase + 5) | 0x10;
    OUTB(vgaIOBase+ 4, DRAMControl);
    pReg->tridentRegs3x4[DRAMControl] = INB(vgaIOBase + 5) | 0x10;
    if (pTrident->IsCyber && !pTrident->MMIOonly)
	pReg->tridentRegs3x4[DRAMControl] |= 0x20;
    OUTB(vgaIOBase+ 4, AddColReg);
    pReg->tridentRegs3x4[AddColReg] = INB(vgaIOBase + 5) & 0xEF;
    pReg->tridentRegs3x4[AddColReg] |= (offset & 0x100) >> 4;

    if (pTrident->Chipset >= TGUI9660) {
    	pReg->tridentRegs3x4[AddColReg] &= 0xDF;
    	pReg->tridentRegs3x4[AddColReg] |= (offset & 0x200) >> 4;
    }
   
    if (IsPciCard && UseMMIO) {
    	if (!pTrident->NoAccel)
	    pReg->tridentRegs3x4[GraphEngReg] |= 0x80; 
    } else {
    	if (!pTrident->NoAccel)
	    pReg->tridentRegs3x4[GraphEngReg] |= 0x82; 
    }

    OUTB(0x3CE, MiscIntContReg);
    pReg->tridentRegs3CE[MiscIntContReg] = INB(0x3CF) | 0x04;

    /* Fix hashing problem in > 8bpp on 9320 chipset */
    if (pTrident->Chipset == CYBER9320 && pScrn->bitsPerPixel > 8) 
    	pReg->tridentRegs3CE[MiscIntContReg] &= ~0x80;

    OUTB(vgaIOBase+ 4, PCIReg);
    if (IsPciCard && UseMMIO)
    	pReg->tridentRegs3x4[PCIReg] = INB(vgaIOBase + 5) & 0xF9; 
    else
    	pReg->tridentRegs3x4[PCIReg] = INB(vgaIOBase + 5) & 0xF8; 

    /* Enable PCI Bursting on capable chips */
    if (pTrident->Chipset >= TGUI9660) {
	if(pTrident->UsePCIBurst) {
	    pReg->tridentRegs3x4[PCIReg] |= 0x06;
	} else {
	    pReg->tridentRegs3x4[PCIReg] &= 0xF9;
	}
    }
     /* restore */
    OUTB(0x3C4, 0x11);
    OUTB(0x3C5, protect);
   
    return(TRUE);
}

void
TridentRestore(ScrnInfoPtr pScrn, TRIDENTRegPtr tridentReg)
{
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);
    CARD8 temp;
    int vgaIOBase;
    vgaHWGetIOBase(VGAHWPTR(pScrn));
    vgaIOBase = VGAHWPTR(pScrn)->IOBase;

    if (pTrident->Chipset > PROVIDIA9685) {
    	OUTB(0x3C4, Protection);
    	OUTB(0x3C5, 0x92);
    }

    /* Goto New Mode */
    OUTB(0x3C4, 0x0B);
    temp = INB(0x3C5);

    /* Unprotect registers */
    OUTW(0x3C4, ((0xC0 ^ 0x02) << 8) | NewMode1);
    
    temp = INB(0x3C8);
    temp = INB(0x3C6);
    temp = INB(0x3C6);
    temp = INB(0x3C6);
    temp = INB(0x3C6);
    OUTB(0x3C6, tridentReg->tridentRegsDAC[0x00]);
    temp = INB(0x3C8);

    OUTW_3x4(CRTCModuleTest);
    OUTW_3x4(LinearAddReg);
    OUTW_3C4(NewMode2);
    OUTW_3x4(CursorControl);
    OUTW_3x4(CRTHiOrd);
    OUTW_3x4(AddColReg);
    OUTW_3x4(GraphEngReg);
    OUTW_3x4(Performance);
    OUTW_3x4(InterfaceSel);
    OUTW_3x4(DRAMControl);
    OUTW_3x4(PixelBusReg);
    OUTW_3x4(PCIReg);
    OUTW_3x4(PCIRetry);
    OUTW_3CE(MiscIntContReg);
    OUTW_3CE(MiscExtFunc);
    OUTW_3x4(Offset);
    OUTW_3x4(PreEndControl);
    OUTW_3x4(PreEndFetch);
    if (pTrident->Chipset >= PROVIDIA9685) OUTW_3x4(Enhancement0);
    if (pTrident->Chipset >= BLADE3D)      OUTW_3x4(RAMDACTiming);
    if (pTrident->Chipset == CYBERBLADEE4) OUTW_3x4(New32);
    if (pTrident->IsCyber) {
	CARD8 tmp;

	OUTW_3CE(VertStretch);
	OUTW_3CE(HorStretch);
	OUTW_3CE(BiosMode);
	OUTW_3CE(BiosReg);	
    	OUTW_3CE(CyberControl);
    	OUTW_3CE(CyberEnhance);
	SHADOW_ENABLE(tmp);
	OUTW_3x4(0x0);
	OUTW_3x4(0x3);
	OUTW_3x4(0x4);
	OUTW_3x4(0x5);
	OUTW_3x4(0x6);
	OUTW_3x4(0x7);	
	OUTW_3x4(0x10);
	OUTW_3x4(0x11); 
	OUTW_3x4(0x16);
	SHADOW_RESTORE(tmp);
    }
 
    if (Is3Dchip) {
#ifdef READOUT
	if (!pTrident->DontSetClock)
#endif
	{
	    OUTW(0x3C4, (tridentReg->tridentRegsClock[0x01])<<8 | ClockLow);
	    OUTW(0x3C4, (tridentReg->tridentRegsClock[0x02])<<8 | ClockHigh);
	}
	if (pTrident->MCLK > 0) {
	    OUTW(0x3C4,(tridentReg->tridentRegsClock[0x03])<<8 | MCLKLow);
	    OUTW(0x3C4,(tridentReg->tridentRegsClock[0x04])<<8 | MCLKHigh);
	}
    } else {
#ifdef READOUT
	if (!pTrident->DontSetClock)
#endif
	{
	    OUTB(0x43C8, tridentReg->tridentRegsClock[0x01]);
	    OUTB(0x43C9, tridentReg->tridentRegsClock[0x02]);
	}
	if (pTrident->MCLK > 0) {
	    OUTB(0x43C6, tridentReg->tridentRegsClock[0x03]);
	    OUTB(0x43C7, tridentReg->tridentRegsClock[0x04]);
	}
    }
#ifdef READOUT
    if (!pTrident->DontSetClock)
#endif
    {
	OUTB(0x3C2, tridentReg->tridentRegsClock[0x00]);
    }
    
    if (pTrident->Chipset > PROVIDIA9685) {
    	OUTB(0x3C4, Protection);
    	OUTB(0x3C5, tridentReg->tridentRegs3C4[Protection]);
    }

    OUTW(0x3C4, ((tridentReg->tridentRegs3C4[NewMode1] ^ 0x02) << 8)| NewMode1);
}

void
TridentSave(ScrnInfoPtr pScrn, TRIDENTRegPtr tridentReg)
{
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);
    CARD8 temp;
    int vgaIOBase;
    vgaHWGetIOBase(VGAHWPTR(pScrn));
    vgaIOBase = VGAHWPTR(pScrn)->IOBase;

    /* Goto New Mode */
    OUTB(0x3C4, 0x0B);
    temp = INB(0x3C5);

    INB_3C4(NewMode1);
    if (pTrident->Chipset > PROVIDIA9685)
    	INB_3C4(Protection);
    
    /* Unprotect registers */
    OUTW(0x3C4, ((0xC0 ^ 0x02) << 8) | NewMode1);
    OUTW(vgaIOBase + 4, (0x92 << 8) | NewMode1);

    INB_3x4(Offset);
    INB_3x4(LinearAddReg);
    INB_3x4(CRTCModuleTest);
    INB_3x4(CRTHiOrd);
    INB_3x4(Performance);
    INB_3x4(InterfaceSel);
    INB_3x4(DRAMControl);
    INB_3x4(AddColReg);
    INB_3x4(PixelBusReg);
    INB_3x4(GraphEngReg);
    INB_3x4(PCIReg);
    INB_3x4(PCIRetry);
    INB_3x4(PreEndControl);
    INB_3x4(PreEndFetch);
    if (pTrident->Chipset >= PROVIDIA9685) INB_3x4(Enhancement0);
    if (pTrident->Chipset >= BLADE3D)      INB_3x4(RAMDACTiming);
    if (pTrident->Chipset == CYBERBLADEE4) INB_3x4(New32);
    if (pTrident->IsCyber) {
	CARD8 tmp;
	INB_3CE(VertStretch);
	INB_3CE(HorStretch);
	INB_3CE(BiosMode);
	INB_3CE(BiosReg);	
    	INB_3CE(CyberControl);
    	INB_3CE(CyberEnhance);
	SHADOW_ENABLE(tmp);
	INB_3x4(0x0);
	INB_3x4(0x3);
	INB_3x4(0x4);
	INB_3x4(0x5);
	INB_3x4(0x6);
	INB_3x4(0x7);
	INB_3x4(0x10);
	INB_3x4(0x11);
	INB_3x4(0x16);
	SHADOW_RESTORE(tmp);
    }

    /* save cursor registers */
    INB_3x4(CursorControl);

    INB_3CE(MiscExtFunc);
    INB_3CE(MiscIntContReg);

    temp = INB(0x3C8);
    temp = INB(0x3C6);
    temp = INB(0x3C6);
    temp = INB(0x3C6);
    temp = INB(0x3C6);
    tridentReg->tridentRegsDAC[0x00] = INB(0x3C6);
    temp = INB(0x3C8);

    tridentReg->tridentRegsClock[0x00] = INB(0x3CC);
    if (Is3Dchip) {
	OUTB(0x3C4, ClockLow);
	tridentReg->tridentRegsClock[0x01] = INB(0x3C5);
	OUTB(0x3C4, ClockHigh);
	tridentReg->tridentRegsClock[0x02] = INB(0x3C5);
	if (pTrident->MCLK > 0) {
	    OUTB(0x3C4, MCLKLow);
	    tridentReg->tridentRegsClock[0x03] = INB(0x3C5);
	    OUTB(0x3C4, MCLKHigh);
	    tridentReg->tridentRegsClock[0x04] = INB(0x3C5);
	}
    } else {
	tridentReg->tridentRegsClock[0x01] = INB(0x43C8);
	tridentReg->tridentRegsClock[0x02] = INB(0x43C9);
	if (pTrident->MCLK > 0) {
	    tridentReg->tridentRegsClock[0x03] = INB(0x43C6);
	    tridentReg->tridentRegsClock[0x04] = INB(0x43C7);
	}
    }

    INB_3C4(NewMode2);

    /* Protect registers */
    OUTW_3C4(NewMode1);
}

static void 
TridentShowCursor(ScrnInfoPtr pScrn) 
{
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);
    int vgaIOBase;
    vgaHWGetIOBase(VGAHWPTR(pScrn));
    vgaIOBase = VGAHWPTR(pScrn)->IOBase;

    /* 64x64 */
    OUTW(vgaIOBase + 4, 0xC150);
}

static void 
TridentHideCursor(ScrnInfoPtr pScrn) {
    int vgaIOBase;
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);
    vgaHWGetIOBase(VGAHWPTR(pScrn));
    vgaIOBase = VGAHWPTR(pScrn)->IOBase;

    OUTW(vgaIOBase + 4, 0x4150);
}

static void 
TridentSetCursorPosition(ScrnInfoPtr pScrn, int x, int y)
{
    int vgaIOBase;
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);
    vgaHWGetIOBase(VGAHWPTR(pScrn));
    vgaIOBase = VGAHWPTR(pScrn)->IOBase;
    if (x < 0) {
    	OUTW(vgaIOBase + 4, (-x)<<8 | 0x46);
	x = 0;
    } else
    	OUTW(vgaIOBase + 4, 0x0046);
 
    if (y < 0) {
    	OUTW(vgaIOBase + 4, (-y)<<8 | 0x47);
	y = 0;
    } else
    	OUTW(vgaIOBase + 4, 0x0047);

    OUTW(vgaIOBase + 4, (x&0xFF)<<8 | 0x40);
    OUTW(vgaIOBase + 4, (x&0x0F00)  | 0x41);
    OUTW(vgaIOBase + 4, (y&0xFF)<<8 | 0x42);
    OUTW(vgaIOBase + 4, (y&0x0F00)  | 0x43);
}

static void
TridentSetCursorColors(ScrnInfoPtr pScrn, int bg, int fg)
{
    int vgaIOBase;
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);
    vgaHWGetIOBase(VGAHWPTR(pScrn));
    vgaIOBase = VGAHWPTR(pScrn)->IOBase;

    OUTW(vgaIOBase + 4, (fg & 0x000000FF)<<8  | 0x48);
    OUTW(vgaIOBase + 4, (fg & 0x0000FF00)     | 0x49);
    OUTW(vgaIOBase + 4, (fg & 0x00FF0000)>>8  | 0x4A);
    OUTW(vgaIOBase + 4, (fg & 0xFF000000)>>16 | 0x4B);
    OUTW(vgaIOBase + 4, (bg & 0x000000FF)<<8  | 0x4C);
    OUTW(vgaIOBase + 4, (bg & 0x0000FF00)     | 0x4D);
    OUTW(vgaIOBase + 4, (bg & 0x00FF0000)>>8  | 0x4E);
    OUTW(vgaIOBase + 4, (bg & 0xFF000000)>>16 | 0x4F);
}

static void
TridentLoadCursorImage(
    ScrnInfoPtr pScrn, 
    CARD8 *src
)
{
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);
    int vgaIOBase;
    vgaHWGetIOBase(VGAHWPTR(pScrn));
    vgaIOBase = VGAHWPTR(pScrn)->IOBase;

    memcpy((CARD8 *)pTrident->FbBase + (pScrn->videoRam * 1024) - 4096,
			src, pTrident->CursorInfoRec->MaxWidth * 
			pTrident->CursorInfoRec->MaxHeight / 4);

    OUTW(vgaIOBase + 4, (((pScrn->videoRam-4) & 0xFF) << 8) | 0x44);
    OUTW(vgaIOBase + 4, ((pScrn->videoRam-4) & 0xFF00) | 0x45);
}

static Bool 
TridentUseHWCursor(ScreenPtr pScreen, CursorPtr pCurs)
{
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);
    
    if (pTrident->MUX && pScrn->bitsPerPixel == 8) return FALSE;

    if (!pTrident->HWCursor) return FALSE;

    return TRUE;
}

Bool 
TridentHWCursorInit(ScreenPtr pScreen)
{
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);
    xf86CursorInfoPtr infoPtr;
    int memory = pScrn->displayWidth * pScrn->virtualY * pScrn->bitsPerPixel/8;

    if (memory > (pScrn->videoRam * 1024 - 4096)) return FALSE;
    infoPtr = xf86CreateCursorInfoRec();
    if(!infoPtr) return FALSE;
    
    pTrident->CursorInfoRec = infoPtr;

    infoPtr->MaxWidth = 64;
    infoPtr->MaxHeight = 64;
    infoPtr->Flags = HARDWARE_CURSOR_BIT_ORDER_MSBFIRST |
		HARDWARE_CURSOR_SWAP_SOURCE_AND_MASK |
		HARDWARE_CURSOR_SOURCE_MASK_INTERLEAVE_32;
    infoPtr->SetCursorColors = TridentSetCursorColors;
    infoPtr->SetCursorPosition = TridentSetCursorPosition;
    infoPtr->LoadCursorImage = TridentLoadCursorImage;
    infoPtr->HideCursor = TridentHideCursor;
    infoPtr->ShowCursor = TridentShowCursor;
    infoPtr->UseHWCursor = TridentUseHWCursor;

    return(xf86InitCursor(pScreen, infoPtr));
}

unsigned int
Tridentddc1Read(ScrnInfoPtr pScrn)
{
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);
    int vgaIOBase = VGAHWPTR(pScrn)->IOBase;
    CARD8 temp;

    /* New mode */
    OUTB(0x3C4, 0x0B); temp = INB(0x3C5);

    OUTB(vgaIOBase + 4, NewMode1);
    temp = INB(vgaIOBase + 5);
    OUTB(vgaIOBase + 5, temp | 0x80);

    /* Define SDA as input */
    OUTW(vgaIOBase + 4, (0x04 << 8) | I2C);

    OUTW(vgaIOBase + 4, (temp << 8) | NewMode1);

    /* Wait until vertical retrace is in progress. */
    while (INB(vgaIOBase + 0xA) & 0x08);
    while (!(INB(vgaIOBase + 0xA) & 0x08));

    /* Get the result */
    OUTB(vgaIOBase + 4, I2C);
    return ( INB(vgaIOBase + 5) & 0x01 );
}

void TridentSetOverscan(
    ScrnInfoPtr pScrn, 
    int overscan
){
    vgaHWPtr hwp = VGAHWPTR(pScrn);

    if (overscan < 0 || overscan > 255)
	return;

    hwp->enablePalette(hwp);
    hwp->writeAttr(hwp, OVERSCAN, overscan);
    hwp->disablePalette(hwp);
}

void TridentLoadPalette(
    ScrnInfoPtr pScrn, 
    int numColors, 
    int *indicies,
    LOCO *colors,
    VisualPtr pVisual
){
    vgaHWPtr hwp = VGAHWPTR(pScrn);
    TRIDENTPtr pTrident = TRIDENTPTR(pScrn);
    int i, index;
    int sig;
#if 0
    sig = xf86BlockSIGIO ();
#endif
    for(i = 0; i < numColors; i++) {
	index = indicies[i];
    	OUTB(0x3C6, 0xFF);
	DACDelay(hwp);
        OUTB(0x3c8, index);
	DACDelay(hwp);
        OUTB(0x3c9, colors[index].red);
	DACDelay(hwp);
        OUTB(0x3c9, colors[index].green);
	DACDelay(hwp);
        OUTB(0x3c9, colors[index].blue);
	DACDelay(hwp);
    }
#if 0
    xf86UnblockSIGIO (sig);
#endif
}

