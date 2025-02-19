/*
 * Copyright 1997,1998 by Alan Hourihane <alanh@fairlite.demon.co.uk>
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
 * Authors:  Alan Hourihane, <alanh@fairlite.demon.co.uk>
 *           Dirk Hohndel,   <hohndel@suse.de>
 *	     Stefan Dirsch,  <sndirsch@suse.de>
 *	     Helmut Fahrion, <hf@suse.de>
 *
 * this work is sponsored by S.u.S.E. GmbH, Fuerth, Elsa GmbH, Aachen and
 * Siemens Nixdorf Informationssysteme
 */
/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/glint/pm2v_dac.c,v 1.17 2000/09/11 16:58:56 alanh Exp $ */

#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86_ansic.h"

#include "xf86PciInfo.h"
#include "xf86Pci.h"

#include "glint_regs.h"
#include "glint.h"

static int
Shiftbpp(ScrnInfoPtr pScrn, int value)
{
    GLINTPtr pGlint = GLINTPTR(pScrn);
    /* shift horizontal timings for 64bit VRAM's or 32bit SGRAMs */
    int logbytesperaccess = 2;
	
    switch (pScrn->bitsPerPixel) {
    case 8:
	value >>= logbytesperaccess;
	pGlint->BppShift = logbytesperaccess;
	break;
    case 16:
	if (pGlint->DoubleBuffer) {
	    value >>= (logbytesperaccess-2);
	    pGlint->BppShift = logbytesperaccess-2;
	} else {
	    value >>= (logbytesperaccess-1);
	    pGlint->BppShift = logbytesperaccess-1;
	}
	break;
    case 24:
	value *= 3;
	value >>= logbytesperaccess;
	pGlint->BppShift = logbytesperaccess;
	break;
    case 32:
	value >>= (logbytesperaccess-2);
	pGlint->BppShift = logbytesperaccess-2;
	break;
    }
    return (value);
}

static unsigned long
PM2VDAC_CalculateClock
(
 unsigned long reqclock,		/* In kHz units */
 unsigned long refclock,		/* In kHz units */
 unsigned char *prescale,		/* ClkPreScale */
 unsigned char *feedback, 		/* ClkFeedBackScale */
 unsigned char *postscale		/* ClkPostScale */
 )
{
    int			f, pre, post;
    unsigned long	freq;
    long		freqerr = 1000;
    unsigned long  	actualclock = 0;
    unsigned char	divide[5] = { 1, 2, 4, 8, 16 };

    for (f=1;f<256;f++) {
	for (pre=1;pre<256;pre++) {
	    for (post=0;post<2;post++) { 
	    	freq = ((refclock * f) / (pre * (1 << divide[post])));
		if ((reqclock > freq - freqerr)&&(reqclock < freq + freqerr)){
		    freqerr = (reqclock > freq) ? 
					reqclock - freq : freq - reqclock;
		    *feedback = f;
		    *prescale = pre;
		    *postscale = post;
		    actualclock = freq;
		}
	    }
	}
    }

    return(actualclock);
}

Bool
Permedia2VInit(ScrnInfoPtr pScrn, DisplayModePtr mode)
{
    GLINTPtr pGlint = GLINTPTR(pScrn);
    GLINTRegPtr pReg = &pGlint->ModeReg;
    CARD32 temp1, temp2, temp3, temp4;

    pReg->glintRegs[Aperture0 >> 3] = 0;
    pReg->glintRegs[Aperture1 >> 3] = 0;
    pReg->glintRegs[PMFramebufferWriteMask >> 3] = 0xFFFFFFFF;
    pReg->glintRegs[PMBypassWriteMask >> 3] = 0xFFFFFFFF;

    if (pGlint->UsePCIRetry) {
	pReg->glintRegs[DFIFODis >> 3] = 1;
	pReg->glintRegs[FIFODis >> 3] = 3;
    } else {
	pReg->glintRegs[DFIFODis >> 3] = 0;
	pReg->glintRegs[FIFODis >> 3] = 1;
    }

    if (pGlint->UseBlockWrite)
	pReg->glintRegs[PMMemConfig >> 3] = GLINT_READ_REG(PMMemConfig) | 1<<21;

    temp1 = mode->CrtcHSyncStart - mode->CrtcHDisplay;
    temp2 = mode->CrtcVSyncStart - mode->CrtcVDisplay;
    temp3 = mode->CrtcHSyncEnd - mode->CrtcHSyncStart;
    temp4 = mode->CrtcVSyncEnd - mode->CrtcVSyncStart;

    pReg->glintRegs[PMHTotal >> 3] = Shiftbpp(pScrn,mode->CrtcHTotal);
    pReg->glintRegs[PMHsEnd >> 3] = Shiftbpp(pScrn, temp1 + temp3);
    pReg->glintRegs[PMHsStart >> 3] = Shiftbpp(pScrn, temp1);
    pReg->glintRegs[PMHbEnd >> 3] = 
			Shiftbpp(pScrn,mode->CrtcHTotal-mode->CrtcHDisplay);
    pReg->glintRegs[PMScreenStride >> 3] = 
					Shiftbpp(pScrn,pScrn->displayWidth>>1);

    pReg->glintRegs[PMVTotal >> 3] = mode->CrtcVTotal;
    pReg->glintRegs[PMVsEnd >> 3] = temp2 + temp4;
    pReg->glintRegs[PMVsStart >> 3] = temp2;
    pReg->glintRegs[PMVbEnd >> 3] = mode->CrtcVTotal - mode->CrtcVDisplay;

    /* The hw cursor needs /VSYNC to recognize vert retrace. We'll stick
       both sync lines to active low here and if needed invert them
       using the RAMDAC's RDSyncControl below. */
    pReg->glintRegs[PMVideoControl >> 3] =
	(1 << 5) | (1 << 3) | 1;

    /* We stick the RAMDAC into 64bit mode */
    /* And reduce the horizontal timings and clock by half */
    pReg->glintRegs[PMVideoControl >> 3] |= 1<<16;
    pReg->glintRegs[PMHTotal >> 3] >>= 1;
    pReg->glintRegs[PMHsEnd >> 3] >>= 1;
    pReg->glintRegs[PMHsStart >> 3] >>= 1;
    pReg->glintRegs[PMHbEnd >> 3] >>= 1;

    pReg->glintRegs[VClkCtl >> 3] = (GLINT_READ_REG(VClkCtl) & 0xFFFFFFFC);
    pReg->glintRegs[PMScreenBase >> 3] = 0; 
    pReg->glintRegs[PMHTotal >> 3] -= 1; 
    pReg->glintRegs[PMHsStart >> 3] -= 1; /* PMHsStart */
    pReg->glintRegs[PMVTotal >> 3] -= 1; /* PMVTotal */

    pReg->glintRegs[ChipConfig >> 3] = GLINT_READ_REG(ChipConfig) & 0xFFFFFFDD;
    pReg->DacRegs[PM2VDACRDDACControl] = 0x80;
  
    {
	/* Get the programmable clock values */
    	unsigned char m,n,p;
    	unsigned long clockused;
	
    	clockused = PM2VDAC_CalculateClock(mode->Clock/2,pGlint->RefClock,
								&m,&n,&p);
	pReg->DacRegs[PM2VDACRDDClk0PreScale] = m;
	pReg->DacRegs[PM2VDACRDDClk0FeedbackScale] = n;
	pReg->DacRegs[PM2VDACRDDClk0PostScale] = p;
    }

    pReg->DacRegs[PM2VDACRDIndexControl] = 0x00;

    if (pScrn->rgbBits == 8)
        pReg->DacRegs[PM2VDACRDMiscControl] = 0x01; /* 8bit DAC */
    else
	pReg->DacRegs[PM2VDACRDMiscControl] = 0x00; /* 6bit DAC */

    pReg->DacRegs[PM2VDACRDSyncControl] = 0x00;
    if (!(mode->Flags & V_PHSYNC))
        pReg->DacRegs[PM2VDACRDSyncControl] |= 0x01; /* invert hsync */
    if (!(mode->Flags & V_PVSYNC))
        pReg->DacRegs[PM2VDACRDSyncControl] |= 0x08; /* invert vsync */

    switch (pScrn->bitsPerPixel)
    {
    case 8:
	pReg->DacRegs[PM2VDACRDPixelSize] = 0x00;
	pReg->DacRegs[PM2VDACRDColorFormat] = 0x2E;
    	break;
    case 16:
	pReg->DacRegs[PM2VDACRDPixelSize] = 0x01;
	if (pScrn->depth == 15)
	    pReg->DacRegs[PM2VDACRDColorFormat] = 0x61;
	else
	    pReg->DacRegs[PM2VDACRDColorFormat] = 0x70;
    	break;
    case 24:
	pReg->DacRegs[PM2VDACRDPixelSize] = 0x04;
	pReg->DacRegs[PM2VDACRDColorFormat] = 0x60;
    	break;
    case 32:
	pReg->DacRegs[PM2VDACRDPixelSize] = 0x02;
	pReg->DacRegs[PM2VDACRDColorFormat] = 0x20;
	if (pScrn->overlayFlags & OVERLAY_8_32_PLANAR) {
	    pReg->DacRegs[PM2VDACRDMiscControl] |= 0x10;
	    pReg->DacRegs[PM2VDACRDOverlayKey] = pScrn->colorKey;
	}
    	break;
    }

    return(TRUE);
}

void
Permedia2VSave(ScrnInfoPtr pScrn, GLINTRegPtr glintReg)
{
    GLINTPtr pGlint = GLINTPTR(pScrn);
    int i;

    glintReg->glintRegs[ChipConfig >> 3] = GLINT_READ_REG(ChipConfig);
    glintReg->glintRegs[Aperture0 >> 3]  = GLINT_READ_REG(Aperture0);
    glintReg->glintRegs[Aperture1 >> 3]  = GLINT_READ_REG(Aperture1);
    glintReg->glintRegs[PMFramebufferWriteMask >> 3] = 
					GLINT_READ_REG(PMFramebufferWriteMask);
    glintReg->glintRegs[PMBypassWriteMask >> 3] = 
					GLINT_READ_REG(PMBypassWriteMask);
    glintReg->glintRegs[DFIFODis >> 3]  = GLINT_READ_REG(DFIFODis);
    glintReg->glintRegs[FIFODis >> 3]  = GLINT_READ_REG(FIFODis);
    /* We only muck about with PMMemConfig, if user wants to */
    if (pGlint->UseBlockWrite)
	glintReg->glintRegs[PMMemConfig >> 3] = GLINT_READ_REG(PMMemConfig);
    glintReg->glintRegs[PMHTotal >> 3] = GLINT_READ_REG(PMHTotal);
    glintReg->glintRegs[PMHbEnd >> 3] = GLINT_READ_REG(PMHbEnd);
    glintReg->glintRegs[PMHbEnd >> 3] = GLINT_READ_REG(PMHgEnd);
    glintReg->glintRegs[PMScreenStride >> 3] = GLINT_READ_REG(PMScreenStride);
    glintReg->glintRegs[PMHsStart >> 3] = GLINT_READ_REG(PMHsStart);
    glintReg->glintRegs[PMHsEnd >> 3] = GLINT_READ_REG(PMHsEnd);
    glintReg->glintRegs[PMVTotal >> 3] = GLINT_READ_REG(PMVTotal);
    glintReg->glintRegs[PMVbEnd >> 3] = GLINT_READ_REG(PMVbEnd);
    glintReg->glintRegs[PMVsStart >> 3] = GLINT_READ_REG(PMVsStart);
    glintReg->glintRegs[PMVsEnd >> 3] = GLINT_READ_REG(PMVsEnd);
    glintReg->glintRegs[PMScreenBase >> 3] = GLINT_READ_REG(PMScreenBase);
    glintReg->glintRegs[PMVideoControl >> 3] = GLINT_READ_REG(PMVideoControl);
    glintReg->glintRegs[VClkCtl >> 3] = GLINT_READ_REG(VClkCtl);

    for (i=0;i<768;i++) {
    	Permedia2ReadAddress(pScrn, i);
	glintReg->cmap[i] = Permedia2ReadData(pScrn);
    }

    glintReg->DacRegs[PM2VDACRDIndexControl] = 
				Permedia2vInIndReg(pScrn, PM2VDACRDIndexControl);
    glintReg->DacRegs[PM2VDACRDOverlayKey] = 
				Permedia2vInIndReg(pScrn, PM2VDACRDOverlayKey);
    glintReg->DacRegs[PM2VDACRDSyncControl] = 
				Permedia2vInIndReg(pScrn, PM2VDACRDSyncControl);
    glintReg->DacRegs[PM2VDACRDMiscControl] = 
				Permedia2vInIndReg(pScrn, PM2VDACRDMiscControl);
    glintReg->DacRegs[PM2VDACRDDACControl] = 
				Permedia2vInIndReg(pScrn, PM2VDACRDDACControl);
    glintReg->DacRegs[PM2VDACRDPixelSize] = 
				Permedia2vInIndReg(pScrn, PM2VDACRDPixelSize);
    glintReg->DacRegs[PM2VDACRDColorFormat] = 
				Permedia2vInIndReg(pScrn, PM2VDACRDColorFormat);

    glintReg->DacRegs[PM2VDACRDDClk0PreScale] = Permedia2vInIndReg(pScrn, PM2VDACRDDClk0PreScale);
    glintReg->DacRegs[PM2VDACRDDClk0FeedbackScale] = Permedia2vInIndReg(pScrn, PM2VDACRDDClk0FeedbackScale);
    glintReg->DacRegs[PM2VDACRDDClk0PostScale] = Permedia2vInIndReg(pScrn, PM2VDACRDDClk0PostScale);
}

void
Permedia2VRestore(ScrnInfoPtr pScrn, GLINTRegPtr glintReg)
{
    GLINTPtr pGlint = GLINTPTR(pScrn);
    CARD32 temp;
    int i;

#if 0
    GLINT_SLOW_WRITE_REG(0, ResetStatus);
    while(GLINT_READ_REG(ResetStatus) != 0) {
	xf86MsgVerb(X_INFO, 2, "Resetting Engine - Please Wait.\n");
    };
#endif

    GLINT_SLOW_WRITE_REG(glintReg->glintRegs[ChipConfig >> 3], ChipConfig);
    GLINT_SLOW_WRITE_REG(glintReg->glintRegs[Aperture0 >> 3], Aperture0);
    GLINT_SLOW_WRITE_REG(glintReg->glintRegs[Aperture1 >> 3], Aperture1);
    GLINT_SLOW_WRITE_REG(glintReg->glintRegs[PMFramebufferWriteMask >> 3], 
							PMFramebufferWriteMask);
    GLINT_SLOW_WRITE_REG(glintReg->glintRegs[PMBypassWriteMask >> 3], 
							PMBypassWriteMask);
    GLINT_SLOW_WRITE_REG(glintReg->glintRegs[DFIFODis >> 3], DFIFODis);
    GLINT_SLOW_WRITE_REG(glintReg->glintRegs[FIFODis >> 3], FIFODis);
    /* We only muck about with PMMemConfig, if user wants to */
    if (pGlint->UseBlockWrite)
    	GLINT_SLOW_WRITE_REG(glintReg->glintRegs[PMMemConfig >> 3],PMMemConfig);
    GLINT_SLOW_WRITE_REG(glintReg->glintRegs[PMVideoControl >> 3], 
								PMVideoControl);
    GLINT_SLOW_WRITE_REG(glintReg->glintRegs[PMHbEnd >> 3], PMHgEnd);
    GLINT_SLOW_WRITE_REG(glintReg->glintRegs[PMScreenBase >> 3], PMScreenBase);
    GLINT_SLOW_WRITE_REG(glintReg->glintRegs[VClkCtl >> 3], VClkCtl);
    GLINT_SLOW_WRITE_REG(glintReg->glintRegs[PMScreenStride >> 3], 
								PMScreenStride);
    GLINT_SLOW_WRITE_REG(glintReg->glintRegs[PMHTotal >> 3], PMHTotal);
    GLINT_SLOW_WRITE_REG(glintReg->glintRegs[PMHbEnd >> 3], PMHbEnd);
    GLINT_SLOW_WRITE_REG(glintReg->glintRegs[PMHsStart >> 3], PMHsStart);
    GLINT_SLOW_WRITE_REG(glintReg->glintRegs[PMHsEnd >> 3], PMHsEnd);
    GLINT_SLOW_WRITE_REG(glintReg->glintRegs[PMVTotal >> 3], PMVTotal);
    GLINT_SLOW_WRITE_REG(glintReg->glintRegs[PMVbEnd >> 3], PMVbEnd);
    GLINT_SLOW_WRITE_REG(glintReg->glintRegs[PMVsStart >> 3], PMVsStart);
    GLINT_SLOW_WRITE_REG(glintReg->glintRegs[PMVsEnd >> 3], PMVsEnd);

    Permedia2vOutIndReg(pScrn, PM2VDACRDIndexControl, 0x00, 
				glintReg->DacRegs[PM2VDACRDIndexControl]);
    Permedia2vOutIndReg(pScrn, PM2VDACRDOverlayKey, 0x00, 
				glintReg->DacRegs[PM2VDACRDOverlayKey]);
    Permedia2vOutIndReg(pScrn, PM2VDACRDSyncControl, 0x00, 
				glintReg->DacRegs[PM2VDACRDSyncControl]);
    Permedia2vOutIndReg(pScrn, PM2VDACRDMiscControl, 0x00, 
				glintReg->DacRegs[PM2VDACRDMiscControl]);
    Permedia2vOutIndReg(pScrn, PM2VDACRDDACControl, 0x00, 
				glintReg->DacRegs[PM2VDACRDDACControl]);
    Permedia2vOutIndReg(pScrn, PM2VDACRDPixelSize, 0x00, 
				glintReg->DacRegs[PM2VDACRDPixelSize]);
    Permedia2vOutIndReg(pScrn, PM2VDACRDColorFormat, 0x00, 
				glintReg->DacRegs[PM2VDACRDColorFormat]);

    for (i=0;i<768;i++) {
    	Permedia2WriteAddress(pScrn, i);
	Permedia2WriteData(pScrn, glintReg->cmap[i]);
    }

    temp = Permedia2vInIndReg(pScrn, PM2VDACIndexClockControl) & 0xFC;
    Permedia2vOutIndReg(pScrn, PM2VDACRDDClk0PreScale, 0x00, 
				glintReg->DacRegs[PM2VDACRDDClk0PreScale]);
    Permedia2vOutIndReg(pScrn, PM2VDACRDDClk0FeedbackScale, 0x00, 
				glintReg->DacRegs[PM2VDACRDDClk0FeedbackScale]);
    Permedia2vOutIndReg(pScrn, PM2VDACRDDClk0PostScale, 0x00, 
				glintReg->DacRegs[PM2VDACRDDClk0PostScale]);
    Permedia2vOutIndReg(pScrn, PM2VDACIndexClockControl, 0x00, temp|0x03);
}

static void 
Permedia2vShowCursor(ScrnInfoPtr pScrn)
{
    /* Enable cursor - X11 mode */
    Permedia2vOutIndReg(pScrn, PM2VDACRDCursorMode, 0x00, 0x11);
}

static void
Permedia2vHideCursor(ScrnInfoPtr pScrn)
{
    /* Disable cursor - X11 mode */
    Permedia2vOutIndReg(pScrn, PM2VDACRDCursorMode, 0x00, 0x10);
}

static void
Permedia2vLoadCursorImage(
    ScrnInfoPtr pScrn, 
    unsigned char *src
)
{
    int i;
       
    for (i=0; i<1024; i++) 
    	Permedia2vOutIndReg(pScrn, PM2VDACRDCursorPattern+i, 0x00, *(src++));
}

static void
Permedia2vSetCursorPosition(
   ScrnInfoPtr pScrn, 
   int x, int y
)
{
    x += 64;
    y += 64;
    /* Output position - "only" 11 bits of location documented */
   
    Permedia2vOutIndReg(pScrn, PM2VDACRDCursorHotSpotX, 0x00, 0x3f);
    Permedia2vOutIndReg(pScrn, PM2VDACRDCursorHotSpotY, 0x00, 0x3f);
    Permedia2vOutIndReg(pScrn, PM2VDACRDCursorXLow, 0x00, x & 0xFF);
    Permedia2vOutIndReg(pScrn, PM2VDACRDCursorXHigh, 0x00, (x>>8) & 0x0F);
    Permedia2vOutIndReg(pScrn, PM2VDACRDCursorYLow, 0x00, y & 0xFF);
    Permedia2vOutIndReg(pScrn, PM2VDACRDCursorYHigh, 0x00, (y>>8) & 0x0F);
    Permedia2vOutIndReg(pScrn, PM2DACCursorControl, 0x00, 0x00);
}

static void
Permedia2vSetCursorColors(
   ScrnInfoPtr pScrn, 
   int bg, int fg
)
{
    /* A 2 color cursor uses last 2 indexes into hardware cursor palette */
    Permedia2vOutIndReg(pScrn, PM2VDACRDCursorPalette+39, 0x00, (fg>>16)&0xff);
    Permedia2vOutIndReg(pScrn, PM2VDACRDCursorPalette+40, 0x00, (fg>>8)&0xff);
    Permedia2vOutIndReg(pScrn, PM2VDACRDCursorPalette+41, 0x00, fg & 0xff);

    Permedia2vOutIndReg(pScrn, PM2VDACRDCursorPalette+42, 0x00, (bg>>16)&0xff);
    Permedia2vOutIndReg(pScrn, PM2VDACRDCursorPalette+43, 0x00, (bg>>8)&0xff);
    Permedia2vOutIndReg(pScrn, PM2VDACRDCursorPalette+44, 0x00, bg & 0xff);
}

static Bool 
Permedia2vUseHWCursor(ScreenPtr pScr, CursorPtr pCurs)
{
    return TRUE;
}

Bool 
Permedia2vHWCursorInit(ScreenPtr pScreen)
{
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    GLINTPtr pGlint = GLINTPTR(pScrn);
    xf86CursorInfoPtr infoPtr;

    infoPtr = xf86CreateCursorInfoRec();
    if(!infoPtr) return FALSE;
    
    pGlint->CursorInfoRec = infoPtr;

    infoPtr->MaxWidth = 64;
    infoPtr->MaxHeight = 64;
    infoPtr->Flags = HARDWARE_CURSOR_TRUECOLOR_AT_8BPP |
		HARDWARE_CURSOR_SOURCE_MASK_INTERLEAVE_1;
    infoPtr->SetCursorColors = Permedia2vSetCursorColors;
    infoPtr->SetCursorPosition = Permedia2vSetCursorPosition;
    infoPtr->LoadCursorImage = Permedia2vLoadCursorImage;
    infoPtr->HideCursor = Permedia2vHideCursor;
    infoPtr->ShowCursor = Permedia2vShowCursor;
    infoPtr->UseHWCursor = Permedia2vUseHWCursor;

    return(xf86InitCursor(pScreen, infoPtr));
}
