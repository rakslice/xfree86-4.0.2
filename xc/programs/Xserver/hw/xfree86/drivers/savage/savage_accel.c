/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/savage/savage_accel.c,v 1.4 2000/12/07 20:26:22 dawes Exp $ */

/*
 *
 * Copyright 1995-1997 The XFree86 Project, Inc.
 *
 */

/*
 * The accel file for the Savage driver.  
 * 
 * Created 20/03/97 by Sebastien Marineau for 3.3.6
 * Modified 17-Nov-2000 by Tim Roberts for 4.0.1
 * Revision: 
 *
 */

#include "Xarch.h"
#include "xf86.h"
#include "xf86_ansic.h"
#include "xf86_OSproc.h"
#include "compiler.h"
#include "xaalocal.h"
#include "xaarop.h"
#include "xf86PciInfo.h"
#include "miline.h"

#include "savage_driver.h"
#include "savage_regs.h"
#include "savage_bci.h"


/* Globals used in driver */
extern pointer s3savMmioMem;
#ifdef __alpha__
extern pointer s3savMmioMemSparse;
#endif

/* Forward declaration of functions used in the driver */

static void SavageAccelSync( ScrnInfoPtr );

static void SavageSetupForScreenToScreenCopy(
    ScrnInfoPtr pScrn,
    int xdir, 
    int ydir,
    int rop,
    unsigned planemask,
    int transparency_color);

static void SavageSubsequentScreenToScreenCopy(
    ScrnInfoPtr pScrn,
    int x1,
    int y1,
    int x2,
    int y2,
    int w,
    int h);
 
static void SavageSetupForSolidFill(
    ScrnInfoPtr pScrn,
    int color, 
    int rop,
    unsigned planemask);

static void SavageSubsequentSolidFillRect(
    ScrnInfoPtr pScrn,
    int x,
    int y,
    int w,
    int h);

static void SavageSubsequentSolidBresenhamLine(
    ScrnInfoPtr pScrn,
    int x1,
    int y1,
    int e1,
    int e2,
    int err,
    int length,
    int octant);

static void SavageSubsequentSolidTwoPointLine(
    ScrnInfoPtr pScrn,
    int x1,
    int y1,
    int x2,
    int y2,
    int bias);

#if 0
static void SavageSetupForScreenToScreenColorExpand(
    ScrnInfoPtr pScrn,
    int bg,
    int fg,
    int rop,
    unsigned planemask);

static void SavageSubsequentScreenToScreenColorExpand(
    ScrnInfoPtr pScrn,
    int x,
    int y,
    int w,
    int h,
    int skipleft);
#endif

static void SavageSetupForCPUToScreenColorExpandFill(
    ScrnInfoPtr pScrn,
    int fg,
    int bg,
    int rop,
    unsigned planemask);

static void SavageSubsequentScanlineCPUToScreenColorExpandFill(
    ScrnInfoPtr pScrn,
    int x,
    int y,
    int w,
    int h,
    int skipleft);

static void SavageSubsequentColorExpandScanline(
    ScrnInfoPtr pScrn,
    int buffer_no);

static void SavageSetupForMono8x8PatternFill(
    ScrnInfoPtr pScrn,
    int patternx,
    int patterny,
    int fg, 
    int bg,
    int rop,
    unsigned planemask);

static void SavageSubsequentMono8x8PatternFillRect(
    ScrnInfoPtr pScrn,
    int pattern0,
    int pattern1,
    int x,
    int y,
    int w,
    int h);

static void SavageSetupForColor8x8PatternFill(
    ScrnInfoPtr pScrn,
    int patternx,
    int patterny,
    int rop,
    unsigned planemask,
    int trans_col);

static void SavageSubsequentColor8x8PatternFillRect(
    ScrnInfoPtr pScrn,
    int pattern0,
    int pattern1,
    int x,
    int y,
    int w,
    int h);

static void SavageSetClippingRectangle(
    ScrnInfoPtr pScrn,
    int x1,
    int y1,
    int x2,
    int y2);

static void SavageDisableClipping( ScrnInfoPtr );

#if 0
static void SavageSubsequentSolidFillTrap(
    ScrnInfoPtr pScrn,
    int y,
    int h,
    int left,
    int dxl,
    int dyl,
    int el,
    int right,
    int dxr,
    int dyr,
    int er);
#endif

/* from savage_image.c: */

void SavageSetupForImageWrite(
    ScrnInfoPtr pScrn,
    int rop,
    unsigned int planemask,
    int transparency_color,
    int bpp,
    int depth);

void SavageSubsequentImageWriteRect(
    ScrnInfoPtr pScrn,
    int x,
    int y,
    int w,
    int h,
    int skipleft);

void SavageWriteBitmapCPUToScreenColorExpand (
    ScrnInfoPtr pScrn,
    int x, int y, int w, int h,
    unsigned char * src,
    int srcwidth,
    int skipleft,
    int fg, int bg,
    int rop,
    unsigned int planemask
);


void SavageSetGBD( ScrnInfoPtr );

/*
 * This is used to cache the last known value for routines we want to
 * call from the debugger.
 */

static ScrnInfoPtr gpScrn = 0;




void
SavageInitialize2DEngine(ScrnInfoPtr pScrn)
{
    vgaHWPtr hwp = VGAHWPTR(pScrn);
    SavagePtr psav = SAVPTR(pScrn);
    unsigned int vgaCRIndex = hwp->IOBase + 4;
    unsigned int vgaCRReg = hwp->IOBase + 5;

    gpScrn = pScrn;

    VGAOUT16(vgaCRIndex, 0x0140);
    VGAOUT8(vgaCRIndex, 0x31);
    VGAOUT8(vgaCRReg, 0x0c);

    /* Setup plane masks */
    OUTREG(0x8128, ~0); /* enable all write planes */
    OUTREG(0x812C, ~0); /* enable all read planes */
    OUTREG16(0x8134, 0x27);
    OUTREG16(0x8136, 0x07);

    switch( psav->Chipset ) {

    case S3_SAVAGE3D:
    case S3_SAVAGE_MX:
	/* Disable BCI */
	OUTREG(0x48C18, INREG(0x48C18) & 0x3FF0);
	/* Disable shadow status update */
	OUTREG(0x48C0C, 0);
	/* Setup BCI command overflow buffer */
	OUTREG(0x48C14, (psav->cobOffset >> 11) | (psav->cobIndex << 29));
	/* Enable BCI and command overflow buffer */
	OUTREG(0x48C18, INREG(0x48C18) | 0x0C);
	break;

    case S3_SAVAGE4:
    case S3_PROSAVAGE:
	/* Disable BCI */
	OUTREG(0x48C18, INREG(0x48C18) & 0x3FF0);
	/* Disable shadow status update */
	OUTREG(0x48C0C, 0);
	/* Enabel BCI without the COB */
	OUTREG(0x48C18, INREG(0x48C18) | 0x08);
	break;

    case S3_SAVAGE2000:
	/* Disable BCI */
	OUTREG(0x48C18, 0);
	/* Disable shadow status update */
	OUTREG(0x48A30, 0);
	/* Setup BCI command overflow buffer */
	OUTREG(0x48C18, (psav->cobOffset >> 7) | (psav->cobIndex));
	/* Enable BCI and command overflow buffer */
	OUTREG(0x48C18, INREG(0x48C18) | 0x00280000 );
	break;
    }

    /* Use and set global bitmap descriptor. */

    /* For reasons I do not fully understand yet, on the Savage4, the */
    /* write to the GBD register, MM816C, does not "take" at this time. */
    /* Only the low-order byte is acknowledged, resulting in an incorrect */
    /* stride.  Writing the register later, after the mode switch, works */
    /* correctly.  This needs to get resolved. */

    psav->SavedGbd = 1 | 8 | BCI_BD_BW_DISABLE;
    BCI_BD_SET_BPP(psav->SavedGbd, pScrn->bitsPerPixel);
    BCI_BD_SET_STRIDE(psav->SavedGbd, pScrn->displayWidth);

    SavageSetGBD(pScrn);
} 


void
SavageSetGBD( ScrnInfoPtr pScrn )
{
    vgaHWPtr hwp = VGAHWPTR(pScrn);
    SavagePtr psav = SAVPTR(pScrn);
    unsigned int vgaCRIndex = hwp->IOBase + 4;
    unsigned int vgaCRReg = hwp->IOBase + 5;

    if( !psav->SavedGbd )
    {
	psav->SavedGbd = 1 | 8 | BCI_BD_BW_DISABLE;
	BCI_BD_SET_BPP(psav->SavedGbd, pScrn->bitsPerPixel);
	BCI_BD_SET_STRIDE(psav->SavedGbd, pScrn->displayWidth);
    }

    /* Turn on 16-bit register access. */

    VGAOUT8(vgaCRIndex, 0x31);
    VGAOUT8(vgaCRReg, 0x0c);

    /* Set stride to use GBD. */

    VGAOUT8(vgaCRIndex, 0x50);
    VGAOUT8(vgaCRReg, VGAIN8(vgaCRReg) | 0xC1);

    /* Enable 2D engine. */

    VGAOUT16(vgaCRIndex, 0x0140);

    /* Now set the GBD and SBDs. */

    OUTREG(0x8168,0);
    OUTREG(0x816C,psav->SavedGbd);
    OUTREG(0x8170,0);
    OUTREG(0x8174,psav->SavedGbd);
    OUTREG(0x8178,0);
    OUTREG(0x817C,psav->SavedGbd);

    OUTREG(0x81C8, pScrn->displayWidth << 4);
    OUTREG(0x81D8, pScrn->displayWidth << 4);
}


/* Acceleration init function, sets up pointers to our accelerated functions */

Bool 
SavageInitAccel(ScreenPtr pScreen)
{
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    SavagePtr psav = SAVPTR(pScrn);
    XAAInfoRecPtr xaaptr;
    BoxRec AvailFBArea;

    /* Set-up our GE command primitive */
    
    if (pScrn->bitsPerPixel == 8) {
      psav->PlaneMask = 0xFF;
      }
    else if (pScrn->bitsPerPixel == 16) {
      psav->PlaneMask = 0xFFFF;
      }
    else if (pScrn->bitsPerPixel == 24) {
      psav->PlaneMask = 0xFFFFFF;
      }
    else if (pScrn->bitsPerPixel == 32) {
      psav->PlaneMask = 0xFFFFFFFF;
      }

    /* General acceleration flags */

    if (!(xaaptr = psav->AccelInfoRec = XAACreateInfoRec()))
	return FALSE;

    xaaptr->Flags = 0
    	| PIXMAP_CACHE
	| OFFSCREEN_PIXMAPS
	| LINEAR_FRAMEBUFFER
	;

    /* Clipping */

    xaaptr->SetClippingRectangle = SavageSetClippingRectangle;
    xaaptr->DisableClipping = SavageDisableClipping;
    xaaptr->ClippingFlags = 0
	| HARDWARE_CLIP_SOLID_FILL 
	| HARDWARE_CLIP_SOLID_LINE
	| HARDWARE_CLIP_DASHED_LINE
	| HARDWARE_CLIP_SCREEN_TO_SCREEN_COPY
	| HARDWARE_CLIP_MONO_8x8_FILL
	| HARDWARE_CLIP_COLOR_8x8_FILL
	;

    xaaptr->Sync = SavageAccelSync;


    /* ScreenToScreen copies */

#if 1
    xaaptr->SetupForScreenToScreenCopy = SavageSetupForScreenToScreenCopy;
    xaaptr->SubsequentScreenToScreenCopy = SavageSubsequentScreenToScreenCopy;
    xaaptr->ScreenToScreenCopyFlags = NO_TRANSPARENCY;
#endif


    /* Solid filled rectangles */

#if 1
    xaaptr->SetupForSolidFill = SavageSetupForSolidFill;
    xaaptr->SubsequentSolidFillRect = SavageSubsequentSolidFillRect;
    xaaptr->SolidFillFlags = NO_PLANEMASK;
#endif

    /* Mono 8x8 pattern fills */

#if 1
    xaaptr->SetupForMono8x8PatternFill = SavageSetupForMono8x8PatternFill;
    xaaptr->SubsequentMono8x8PatternFillRect 
    	= SavageSubsequentMono8x8PatternFillRect;
    xaaptr->Mono8x8PatternFillFlags = 0
	| HARDWARE_PATTERN_PROGRAMMED_BITS 
	| HARDWARE_PATTERN_SCREEN_ORIGIN
	| BIT_ORDER_IN_BYTE_LSBFIRST
	;
#endif

    /* Color 8x8 pattern fills */

#if 1
    xaaptr->SetupForColor8x8PatternFill =
            SavageSetupForColor8x8PatternFill;
    xaaptr->SubsequentColor8x8PatternFillRect =
            SavageSubsequentColor8x8PatternFillRect;
    xaaptr->Color8x8PatternFillFlags = 0
	| NO_TRANSPARENCY
    	| HARDWARE_PATTERN_PROGRAMMED_BITS
	| HARDWARE_PATTERN_PROGRAMMED_ORIGIN
	;
#endif

    /* Solid lines */

#if 1
    xaaptr->SolidLineFlags = NO_PLANEMASK;
    xaaptr->SetupForSolidLine = SavageSetupForSolidFill;
    xaaptr->SubsequentSolidBresenhamLine = SavageSubsequentSolidBresenhamLine;
    xaaptr->SubsequentSolidTwoPointLine = SavageSubsequentSolidTwoPointLine;
#if 0
    xaaptr->SubsequentSolidFillTrap = SavageSubsequentSolidFillTrap; 
#endif

    xaaptr->SolidBresenhamLineErrorTermBits = 16;
#endif

    /* ImageWrite */

    xaaptr->ImageWriteFlags = 0
	| NO_PLANEMASK
	| CPU_TRANSFER_PAD_DWORD
	| SCANLINE_PAD_DWORD
	| BIT_ORDER_IN_BYTE_MSBFIRST
	| LEFT_EDGE_CLIPPING
	;
    xaaptr->SetupForImageWrite = SavageSetupForImageWrite;
    xaaptr->SubsequentImageWriteRect = SavageSubsequentImageWriteRect;
    xaaptr->NumScanlineImageWriteBuffers = 1;
    xaaptr->ImageWriteBase = psav->BciMem;
    xaaptr->ImageWriteRange = 120 * 1024;

    /* WriteBitmap color expand */

#if 0
    xaaptr->WriteBitmapFlags = NO_PLANEMASK;
    xaaptr->WriteBitmap = SavageWriteBitmapCPUToScreenColorExpand;
#endif

    /* Screen to Screen color expansion.  Not implemented. */

#if 0
    xaaptr->SetupForScreenToScreenColorExpand =
             SavageSetupForScreenToScreenColorExpand;
    xaaptr->SubsequentScreenToScreenColorExpand =
             SavageSubsequentCPUToScreenColorExpand;
#endif

    /* CPU to Screen color expansion */

    xaaptr->ScanlineCPUToScreenColorExpandFillFlags = 0
	| NO_PLANEMASK
	| CPU_TRANSFER_PAD_DWORD
	| SCANLINE_PAD_DWORD
	| BIT_ORDER_IN_BYTE_MSBFIRST
	| LEFT_EDGE_CLIPPING
	;
    xaaptr->SetupForScanlineCPUToScreenColorExpandFill =
            SavageSetupForCPUToScreenColorExpandFill;
    xaaptr->SubsequentScanlineCPUToScreenColorExpandFill =
            SavageSubsequentScanlineCPUToScreenColorExpandFill;
    xaaptr->SubsequentColorExpandScanline =
	    SavageSubsequentColorExpandScanline;
    xaaptr->ColorExpandBase = psav->BciMem;
    xaaptr->ScanlineColorExpandBuffers = &xaaptr->ColorExpandBase;
    xaaptr->NumScanlineColorExpandBuffers = 1;

    /* Set up screen parameters. */

    psav->Bpp = pScrn->bitsPerPixel / 8;
    psav->Bpl = pScrn->displayWidth * psav->Bpp;
    psav->ScissB = psav->CursorKByte / psav->Bpl;
    if (psav->ScissB > 2047)
        psav->ScissB = 2047;

    /*
     * Finally, we set up the video memory space available to the pixmap
     * cache. In this case, all memory from the end of the virtual screen
     * to the end of the command overflow buffer can be used. If you haven't
     * enabled the PIXMAP_CACHE flag, then these lines can be omitted.
     */

    AvailFBArea.x1 = 0;
    AvailFBArea.y1 = 0;
    AvailFBArea.x2 = pScrn->displayWidth;
    AvailFBArea.y2 = psav->ScissB;
    xf86InitFBManager(pScreen, &AvailFBArea);

    return XAAInit(pScreen, xaaptr);
}




/* The sync function for the GE */
void
SavageAccelSync(ScrnInfoPtr pScrn)
{
    SavagePtr psav = SAVPTR(pScrn);
    WaitIdleEmpty();
}


/*
 * The XAA ROP helper routines all assume that a solid color is a 
 * "pattern".  The Savage chips, however, apply a non-stippled solid
 * color as "source".  Thus, we use a slightly customized version.
 */

static int
SavageHelpPatternROP(ScrnInfoPtr pScrn, int *fg, int *bg, int pm, int *rop)
{
    XAAInfoRecPtr infoRec = GET_XAAINFORECPTR_FROM_SCRNINFOPTR(pScrn);
    int ret = 0;
    
    pm &= infoRec->FullPlanemask;

    if(pm == infoRec->FullPlanemask) {
	if(!NO_SRC_ROP(*rop)) 
	   ret |= ROP_PAT;
	*rop = XAACopyROP[*rop];
    } else {	
	switch(*rop) {
	case GXnoop:
	    break;
	case GXset:
	case GXclear:
	case GXinvert:
	    ret |= ROP_PAT;
	    *fg = pm;
	    if(*bg != -1)
		*bg = pm;
	    break;
	default:
	    ret |= ROP_PAT | ROP_SRC;
	    break;
	}
	*rop = XAACopyROP_PM[*rop];
    }

    return ret;
}


static int
SavageHelpSolidROP(ScrnInfoPtr pScrn, int *fg, int pm, int *rop)
{
    XAAInfoRecPtr infoRec = GET_XAAINFORECPTR_FROM_SCRNINFOPTR(pScrn);
    int ret = 0;
    
    pm &= infoRec->FullPlanemask;

    if(pm == infoRec->FullPlanemask) {
	if(!NO_SRC_ROP(*rop)) 
	   ret |= ROP_PAT;
	*rop = XAACopyROP[*rop];
    } else {	
	switch(*rop) {
	case GXnoop:
	    break;
	case GXset:
	case GXclear:
	case GXinvert:
	    ret |= ROP_PAT;
	    *fg = pm;
	    break;
	default:
	    ret |= ROP_PAT | ROP_SRC;
	    break;
	}
	*rop = XAACopyROP_PM[*rop];
    }

    return ret;
}



/* These are the ScreenToScreen bitblt functions. We support all ROPs, all
 * directions, and a planemask by adjusting the ROP and using the mono pattern
 * registers.
 *
 * (That's a lie; we don't really support planemask.)
 */

static void 
SavageSetupForScreenToScreenCopy(
    ScrnInfoPtr pScrn,
    int xdir, 
    int ydir,
    int rop,
    unsigned planemask,
    int transparency_color)
{
    SavagePtr psav = SAVPTR(pScrn);
    int cmd;

    cmd = BCI_CMD_RECT | BCI_CMD_DEST_GBD | BCI_CMD_SRC_GBD;
    BCI_CMD_SET_ROP( cmd, XAACopyROP[rop] );
    if (transparency_color != -1)
        cmd |= BCI_CMD_SEND_COLOR | BCI_CMD_SRC_TRANSPARENT;

    if (xdir == 1 ) cmd |= BCI_CMD_RECT_XP;
    if (ydir == 1 ) cmd |= BCI_CMD_RECT_YP;

    psav->SavedBciCmd = cmd;
    psav->SavedBgColor = transparency_color;
}

static void 
SavageSubsequentScreenToScreenCopy(
    ScrnInfoPtr pScrn,
    int x1,
    int y1,
    int x2,
    int y2,
    int w,
    int h)
{
    SavagePtr psav = SAVPTR(pScrn);
    BCI_GET_PTR;

    if (!w || !h) return;
    if (!(psav->SavedBciCmd & BCI_CMD_RECT_XP)) {
        w --;
        x1 += w;
        x2 += w;
        w ++;
    }
    if (!(psav->SavedBciCmd & BCI_CMD_RECT_YP)) {
        h --;
        y1 += h;
        y2 += h;
        h ++;
    }

    WaitQueue(6);
    BCI_SEND(psav->SavedBciCmd);
    if (psav->SavedBgColor != -1) 
	BCI_SEND(psav->SavedBgColor);
    BCI_SEND(BCI_X_Y(x1, y1));
    BCI_SEND(BCI_X_Y(x2, y2));
    BCI_SEND(BCI_W_H(w, h));
}


/*
 * SetupForSolidFill is also called to set up for lines.
 */ 

static void 
SavageSetupForSolidFill(
    ScrnInfoPtr pScrn,
    int color, 
    int rop,
    unsigned planemask)
{
    SavagePtr psav = SAVPTR(pScrn);
    XAAInfoRecPtr xaaptr = GET_XAAINFORECPTR_FROM_SCRNINFOPTR( pScrn );
    int cmd;
    int mix;

    cmd = BCI_CMD_RECT
        | BCI_CMD_RECT_XP | BCI_CMD_RECT_YP
        | BCI_CMD_DEST_GBD | BCI_CMD_SRC_SOLID;

    /* Don't send a color if we don't have to. */

    if( rop == GXcopy )
    {
	if( color == 0 )
	    rop = GXclear;
	else if( color == xaaptr->FullPlanemask )
	    rop = GXset;
    }

    mix = SavageHelpSolidROP( pScrn, &color, planemask, &rop );

    if( mix & ROP_PAT )
	cmd |= BCI_CMD_SEND_COLOR;

    BCI_CMD_SET_ROP( cmd, rop );

    psav->SavedBciCmd = cmd;
    psav->SavedFgColor = color;
}
    
    
static void 
SavageSubsequentSolidFillRect(
    ScrnInfoPtr pScrn,
    int x,
    int y,
    int w,
    int h)
{
    SavagePtr psav = SAVPTR(pScrn);
    BCI_GET_PTR;

    WaitQueue(5);

    BCI_SEND(psav->SavedBciCmd);
    if( psav->SavedBciCmd & BCI_CMD_SEND_COLOR )
	BCI_SEND(psav->SavedFgColor);
    BCI_SEND(BCI_X_Y(x, y));
    BCI_SEND(BCI_W_H(w, h));
}

#if 0
static void
SavageSetupForScreenToScreenColorExpand(
    ScrnInfoPtr pScrn,
    int bg,
    int fg,
    int rop,
    unsigned planemask)
{
/*    SavagePtr psav = SAVPTR(pScrn); */
}

static void
SavageSubsequentScreenToScreenColorExpand(
    ScrnInfoPtr pScrn,
    int x,
    int y,
    int w,
    int h,
    int skipleft)
{
/*    SavagePtr psav = SAVPTR(pScrn); */
}
#endif


static void
SavageSetupForCPUToScreenColorExpandFill(
    ScrnInfoPtr pScrn,
    int fg,
    int bg,
    int rop,
    unsigned planemask)
{
    SavagePtr psav = SAVPTR(pScrn);
    int cmd;
    int mix;

    cmd = BCI_CMD_RECT | BCI_CMD_RECT_XP | BCI_CMD_RECT_YP
	| BCI_CMD_CLIP_LR
        | BCI_CMD_DEST_GBD | BCI_CMD_SRC_MONO;

    mix = SavageHelpPatternROP( pScrn, &fg, &bg, planemask, &rop );

    if( mix & ROP_PAT )
        cmd |= BCI_CMD_SEND_COLOR;

    BCI_CMD_SET_ROP( cmd, rop );

    if (bg != -1)
        cmd |= BCI_CMD_SEND_COLOR;
    else 
	cmd |= BCI_CMD_SRC_TRANSPARENT;

    psav->SavedBciCmd = cmd;
    psav->SavedFgColor = fg;
    psav->SavedBgColor = bg;
}


static void
SavageSubsequentScanlineCPUToScreenColorExpandFill(
    ScrnInfoPtr pScrn,
    int x,
    int y,
    int w,
    int h,
    int skipleft)
{
    SavagePtr psav = SAVPTR(pScrn);
    BCI_GET_PTR;

    /* XAA will be sending bitmap data next.  */
    /* We should probably wait for empty/idle here. */

    WaitQueue(20);

    BCI_SEND(psav->SavedBciCmd);
    BCI_SEND(BCI_CLIP_LR(x+skipleft, x+w-1));
    w = (w + 31) & ~31;
    if( psav->SavedBciCmd & BCI_CMD_SEND_COLOR )
	BCI_SEND(psav->SavedFgColor);
    if( psav->SavedBgColor != -1 )
	BCI_SEND(psav->SavedBgColor);
    BCI_SEND(BCI_X_Y(x, y));
    BCI_SEND(BCI_W_H(w, 1));
    
    psav->Rect.x = x;
    psav->Rect.y = y + 1;
    psav->Rect.width = w;
    psav->Rect.height = h - 1;
}

static void
SavageSubsequentColorExpandScanline(
    ScrnInfoPtr pScrn,
    int buffer_no)
{
    /* This gets call after each scanline's image data has been sent. */
    SavagePtr psav = SAVPTR(pScrn);
    xRectangle xr = psav->Rect;
    BCI_GET_PTR;

    if( xr.height )
    {
	WaitQueue(20);
	BCI_SEND(BCI_X_Y( xr.x, xr.y));
	BCI_SEND(BCI_W_H( xr.width, 1 ));
        psav->Rect.height--;
	psav->Rect.y++;
    }
}


/*
 * The meaning of the two pattern paremeters to Setup & Subsequent for
 * Mono8x8Patterns varies depending on the flag bits.  We specify
 * HW_PROGRAMMED_BITS, which means our hardware can handle 8x8 patterns
 * without caching in the frame buffer.  Thus, Setup gets the pattern bits.
 * There is no way with BCI to rotate an 8x8 pattern, so we do NOT specify
 * HW_PROGRAMMED_ORIGIN.  XAA wil rotate it for us and pass the rotated
 * pattern to both Setup and Subsequent.  If we DID specify PROGRAMMED_ORIGIN,
 * then Setup would get the unrotated pattern, and Subsequent gets the
 * origin values.
 */

static void
SavageSetupForMono8x8PatternFill(
    ScrnInfoPtr pScrn,
    int patternx,
    int patterny,
    int fg, 
    int bg,
    int rop,
    unsigned planemask)
{
    SavagePtr psav = SAVPTR(pScrn);
    int cmd;
    int mix;

    mix = XAAHelpPatternROP( pScrn, &fg, &bg, planemask, &rop );

    cmd = BCI_CMD_RECT | BCI_CMD_RECT_XP | BCI_CMD_RECT_YP
        | BCI_CMD_DEST_GBD | BCI_CMD_PAT_MONO;

    if( mix & ROP_PAT )
	cmd |= BCI_CMD_SEND_COLOR;

    if (bg == -1)
	cmd |= BCI_CMD_PAT_TRANSPARENT;

    BCI_CMD_SET_ROP(cmd, rop);

    psav->SavedBciCmd = cmd;
    psav->SavedFgColor = fg;
    psav->SavedBgColor = bg;
}


static void
SavageSubsequentMono8x8PatternFillRect(
    ScrnInfoPtr pScrn,
    int pattern0,
    int pattern1,
    int x,
    int y,
    int w,
    int h)
{
    SavagePtr psav = SAVPTR(pScrn);
    BCI_GET_PTR;

    WaitQueue(7);
    BCI_SEND(psav->SavedBciCmd);
    if( psav->SavedBciCmd & BCI_CMD_SEND_COLOR )
	BCI_SEND(psav->SavedFgColor);
    if( psav->SavedBgColor != -1 )
	BCI_SEND(psav->SavedBgColor);
    BCI_SEND(BCI_X_Y(x, y));
    BCI_SEND(BCI_W_H(w, h));
    BCI_SEND(pattern0);
    BCI_SEND(pattern1);
}


static void 
SavageSetupForColor8x8PatternFill(
    ScrnInfoPtr pScrn,
    int patternx,
    int patterny,
    int rop,
    unsigned planemask,
    int trans_col)
{
    SavagePtr psav = SAVPTR(pScrn);

    int cmd;
    int mix;
    unsigned int bd;
    int pat_offset;
    
    /* ViRGEs and Savages do not support transparent color patterns. */
    /* We set the NO_TRANSPARENCY bit, so we should never receive one. */

    pat_offset = (int) (patternx * psav->Bpp + patterny * psav->Bpl);

    cmd = BCI_CMD_RECT | BCI_CMD_RECT_XP | BCI_CMD_RECT_YP
        | BCI_CMD_DEST_GBD | BCI_CMD_PAT_SBD_COLOR_NEW;
        
    mix = SavageHelpSolidROP( pScrn, &trans_col, planemask, &rop );

    BCI_CMD_SET_ROP(cmd, rop);
    bd = BCI_BD_BW_DISABLE;
    BCI_BD_SET_BPP(bd, pScrn->bitsPerPixel);
    BCI_BD_SET_STRIDE(bd, 8);

    psav->SavedBciCmd = cmd;
    psav->SavedSbdOffset = pat_offset;
    psav->SavedSbd = bd;
    psav->SavedBgColor = trans_col;
}


static void
SavageSubsequentColor8x8PatternFillRect(
    ScrnInfoPtr pScrn,
    int patternx,
    int patterny,
    int x,
    int y,
    int w,
    int h)
{
    SavagePtr psav = SAVPTR(pScrn);
    BCI_GET_PTR;

    WaitQueue(5);
    BCI_SEND(psav->SavedBciCmd);
    BCI_SEND(psav->SavedSbdOffset);
    BCI_SEND(psav->SavedSbd);
    BCI_SEND(BCI_X_Y(x, y));
    BCI_SEND(BCI_W_H(w, h));
}


static void
SavageSubsequentSolidBresenhamLine(
    ScrnInfoPtr pScrn,
    int x1,
    int y1,
    int e1,
    int e2,
    int err,
    int length,
    int octant)
{
    SavagePtr psav = SAVPTR(pScrn);
    BCI_GET_PTR;
    int cmd;

    cmd = (psav->SavedBciCmd & 0x00ffffff);
    cmd |= BCI_CMD_LINE_LAST_PIXEL;

#ifdef DEBUG_EXTRA
    ErrorF("BresenhamLine, (%4d,%4d), len %4d, oct %d, err %4d,%4d, clr %08x\n",
        x1, y1, length, octant, e2, e1, psav->SavedFgColor );
#endif

    WaitQueue( 5 );
    BCI_SEND(cmd);
    if( cmd & BCI_CMD_SEND_COLOR )
	BCI_SEND( psav->SavedFgColor );
    BCI_SEND(BCI_LINE_X_Y(x1, y1));
    BCI_SEND(BCI_LINE_STEPS(e2-e1, e2));
    BCI_SEND(BCI_LINE_MISC(length, 
    			   !!(octant & YMAJOR),
			   !(octant & XDECREASING),
			   !(octant & YDECREASING),
			   e2+err));
}


static void 
SavageSubsequentSolidTwoPointLine(
    ScrnInfoPtr pScrn,
    int x1,
    int y1,
    int x2,
    int y2,
    int bias)
{
    SavagePtr psav = SAVPTR(pScrn);
    BCI_GET_PTR;

    int cmd;
    int dx, dy;
    int min, max, xp, yp, ym;

    dx = x2 - x1;
    dy = y2 - y1;

#ifdef DEBUG_EXTRA
    ErrorF("TwoPointLine, (%4d,%4d)-(%4d,%4d), clr %08x, last pt %s\n",
        x1, y1, x2, y2, psav->SavedFgColor, (bias & 0x100)?"NO ":"YES");
#endif

    xp = (dx >= 0);
    if( !xp ) {
	dx = -dx;
    }

    yp = (dy >= 0);
    if( !yp ) {
	dy = -dy;
    }

    ym = (dy > dx);
    if( ym ) {
	max = dy;
	min = dx;
    }
    else {
	max = dx;
	min = dy;
    }

    if( !(bias & 0x100) ) {
	max++;
    }

    cmd = (psav->SavedBciCmd & 0x00ffffff);
    cmd |= BCI_CMD_LINE_LAST_PIXEL;

    WaitQueue(5);
    BCI_SEND( cmd );
    if( cmd & BCI_CMD_SEND_COLOR )
	BCI_SEND( psav->SavedFgColor );
    BCI_SEND( BCI_LINE_X_Y( x1, y1 ) );
    BCI_SEND( BCI_LINE_STEPS( 2 * (min - max), 2 * min ) );
    BCI_SEND( BCI_LINE_MISC( max, ym, xp, yp, 2 * min - max ) );
}



static void 
SavageSetClippingRectangle(
    ScrnInfoPtr pScrn,
    int x1,
    int y1,
    int x2,
    int y2)
{
    SavagePtr psav = SAVPTR(pScrn);
    BCI_GET_PTR;
    int cmd;

#ifdef DEBUG_EXTRA
    ErrorF("ClipRect, (%4d,%4d)-(%4d,%4d) \n", x1, y1, x2, y2 );
#endif

    cmd = BCI_CMD_NOP | BCI_CMD_CLIP_NEW;
    WaitQueue(3);
    BCI_SEND(cmd);
    BCI_SEND(BCI_CLIP_TL(y1, x1));
    BCI_SEND(BCI_CLIP_BR(y2, x2));
    psav->SavedBciCmd |= BCI_CMD_CLIP_CURRENT;
}


static void SavageDisableClipping( ScrnInfoPtr pScrn )
{
    SavagePtr psav = SAVPTR(pScrn);
#ifdef DEBUG_EXTRA
    ErrorF("Kill ClipRect\n");
#endif
    psav->SavedBciCmd &= ~BCI_CMD_CLIP_NEW;
}


/* Trapezoid solid fills. XAA passes the coordinates of the top start
 * and end points, and the slopes of the left and right vertexes. We
 * use this info to generate the bottom points. We use a mixture of
 * floating-point and fixed point logic; the biases are done in fixed
 * point. Essentially, these were determined experimentally. The function
 * passes xtest, but I suspect that it will not match cfb for large polygons.
 *
 * This code is from the ViRGE.  We need to modify it for Savage if we ever
 * decide to turn it on.
 */

#if 0
void
SavageSubsequentSolidFillTrap(
    ScrnInfoPtr pScrn,
    int y,
    int h,
    int left,
    int dxl,
    int dyl,
    int el,
    int right,
    int dxr,
    int dyr,
    int er)
{
    int l_xdelta, r_xdelta;
    double lendx, rendx, dl_delta, dr_delta;
    int lbias, rbias;
    unsigned int cmd;
    double l_sgn = -1.0, r_sgn = -1.0;

    cmd |= (CMD_POLYFILL | CMD_AUTOEXEC | MIX_MONO_PATT) ;
    cmd |= (psav->SavedRectCmdForLine & (0xff << 17));
   
    l_xdelta = -(dxl << 20)/ dyl;
    r_xdelta = -(dxr << 20)/ dyr;

    dl_delta = -(double) dxl / (double) dyl;
    dr_delta = -(double) dxr / (double) dyr;
    if (dl_delta < 0.0) l_sgn = 1.0;
    if (dr_delta < 0.0) r_sgn = 1.0;
   
    lendx = l_sgn * ((double) el / (double) dyl) + left + ((h - 1) * dxl) / (double) dyl;
    rendx = r_sgn * ((double) er / (double) dyr) + right + ((h - 1) * dxr) / (double) dyr;

    /* We now have four cases */

    if (fabs(dl_delta) > 1.0) {  /* XMAJOR line */
        if (dxl > 0) { lbias = ((1 << 20) - h); }
        else { lbias = 0; }
        }
    else {
        if (dxl > 0) { lbias = ((1 << 20) - 1) + l_xdelta / 2; }
        else { lbias = 0; }
        }

    if (fabs(dr_delta) > 1.0) {   /* XMAJOR line */
        if (dxr > 0) { rbias = (1 << 20); }
        else { rbias = ((1 << 20) - 1); }
        }
    else {
        if (dxr > 0) { rbias = (1 << 20); }
        else { rbias = ((1 << 20) - 1); }
        }

    WaitQueue(8);
    CACHE_SETP_CMD_SET(cmd);
    SETP_PRDX(r_xdelta);
    SETP_PLDX(l_xdelta);
    SETP_PRXSTART(((int) (rendx * (double) (1 << 20))) + rbias);
    SETP_PLXSTART(((int) (lendx * (double) (1 << 20))) + lbias);

    SETP_PYSTART(y + h - 1);
    SETP_PYCNT((h) | 0x30000000);

    CACHE_SETB_CMD_SET(psav->SavedRectCmdForLine);

}
#endif

/* Routines for debugging. */

unsigned long
writedw( unsigned long addr, unsigned long value )
{
    SavagePtr psav = SAVPTR(gpScrn);
    OUTREG( addr, value );
    return INREG( addr );
}

unsigned long
readdw( unsigned long addr )
{
    SavagePtr psav = SAVPTR(gpScrn);
    return INREG( addr );
}

unsigned long
readfb( unsigned long addr )
{
    SavagePtr psav = SAVPTR(gpScrn);
    char * videobuffer = (char *) psav->FBBase;
    return *(volatile unsigned long*)(videobuffer + (addr & ~3) );
}

unsigned long
writefb( unsigned long addr, unsigned long value )
{
    SavagePtr psav = SAVPTR(gpScrn);
    char * videobuffer = (char *) psav->FBBase;
    *(unsigned long*)(videobuffer + (addr & ~3)) = value;
    return *(volatile unsigned long*)(videobuffer + (addr & ~3) );
}

void
writescan( unsigned long scan, unsigned long color )
{
    SavagePtr psav = SAVPTR(gpScrn);
    int i;
    char * videobuffer = (char *)psav->FBBase;
    videobuffer += scan * gpScrn->displayWidth * gpScrn->bitsPerPixel >> 3;
    for( i = gpScrn->displayWidth; --i; ) {
	switch( gpScrn->bitsPerPixel ) {
	    case 8: 
		*videobuffer++ = color; 
		break;
	    case 16: 
		*(unsigned short*)videobuffer = color;
		videobuffer += 2;
		break;
	    case 32:
		*(unsigned long*)videobuffer = color;
		videobuffer += 4;
		break;
	}
    }
}
