/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/savage/savage_cursor.c,v 1.1 2000/12/02 01:16:11 dawes Exp $ */

/*
 * Hardware cursor support for S3 Savage 4.0 driver. Taken with
 * very few changes from the s3virge cursor file.
 *
 * S. Marineau, 19/04/97.
 * Modified by Amancio Hasty and Jon Tombs
 * Ported to 4.0 by Tim Roberts.
 */

#include "savage_driver.h"

static void SavageLoadCursorImage(ScrnInfoPtr pScrn, unsigned char *src);
static void SavageShowCursor(ScrnInfoPtr pScrn);
static void SavageHideCursor(ScrnInfoPtr pScrn);
static void SavageSetCursorPosition(ScrnInfoPtr pScrn, int x, int y);
static void SavageSetCursorColors(ScrnInfoPtr pScrn, int bg, int fg);


/*
 * Read/write to the DAC via MMIO 
 */

#define inCRReg(reg) (VGAHWPTR(pScrn))->readCrtc( VGAHWPTR(pScrn), reg )
#define outCRReg(reg, val) (VGAHWPTR(pScrn))->writeCrtc( VGAHWPTR(pScrn), reg, val )


#define MAX_CURS 64


Bool 
SavageHWCursorInit(ScreenPtr pScreen)
{
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    SavagePtr psav = SAVPTR(pScrn);
    xf86CursorInfoPtr infoPtr;

    infoPtr = xf86CreateCursorInfoRec();
    if(!infoPtr) 
        return FALSE;
    
    psav->CursorInfoRec = infoPtr;

    infoPtr->MaxWidth = MAX_CURS;
    infoPtr->MaxHeight = MAX_CURS;
    infoPtr->Flags = HARDWARE_CURSOR_SOURCE_MASK_INTERLEAVE_16 |
		     HARDWARE_CURSOR_SWAP_SOURCE_AND_MASK |
		     HARDWARE_CURSOR_AND_SOURCE_WITH_MASK |
		     HARDWARE_CURSOR_BIT_ORDER_MSBFIRST |
	             HARDWARE_CURSOR_INVERT_MASK;

    /*
     * The /MX family is apparently unique among the Savages, in that
     * the cursor color is always straight RGB.  The rest of the Savages
     * use palettized values at 8-bit when not clock doubled.
     */

    if(
        ( (inCRReg(0x18) & 0x80) && (inCRReg(0x15) & 0x50) )
	||
	(psav->Chipset == S3_SAVAGE_MX)
    )
       infoPtr->Flags |= HARDWARE_CURSOR_TRUECOLOR_AT_8BPP;

    infoPtr->SetCursorColors = SavageSetCursorColors;
    infoPtr->SetCursorPosition = SavageSetCursorPosition;
    infoPtr->LoadCursorImage = SavageLoadCursorImage;
    infoPtr->HideCursor = SavageHideCursor;
    infoPtr->ShowCursor = SavageShowCursor;
    infoPtr->UseHWCursor = NULL;

    if( !psav->CursorKByte )
	psav->CursorKByte = pScrn->videoRam - 4;

    return xf86InitCursor(pScreen, infoPtr);
}



void
SavageShowCursor(ScrnInfoPtr pScrn)
{
   /* Turn cursor on. */
   outCRReg( 0x45, inCRReg(0x45) | 0x01 );
}


void
SavageHideCursor(ScrnInfoPtr pScrn)
{
   /* Turn cursor off. */
   outCRReg( 0x45, inCRReg(0x45) & 0xfe );
}


static void
SavageLoadCursorImage(
    ScrnInfoPtr pScrn,
    unsigned char* src)
{
    SavagePtr psav = SAVPTR(pScrn);

    /* Set cursor location in frame buffer.  */
    outCRReg( 0x4d, (0xff & psav->CursorKByte));
    outCRReg( 0x4c, (0xff00 & psav->CursorKByte) >> 8);

    /* Upload the cursor image to the frame buffer. */
    memcpy(psav->FBBase + psav->CursorKByte * 1024, src, 1024);

    if( S3_SAVAGE4_SERIES( psav->Chipset ) ) {
	/*
	 * Bug in Savage4 Rev B requires us to do an MMIO read after
	 * loading the cursor.
	 */
	volatile unsigned int i = ALT_STATUS_WORD0;
    }
}


static void
SavageSetCursorPosition(
     ScrnInfoPtr pScrn,
     int x, 
     int y)
{
    unsigned char xoff, yoff;

    /* adjust for frame buffer base address granularity */
    if (pScrn->bitsPerPixel == 8)
	x += ((pScrn->frameX0) & 3);
    else if (pScrn->bitsPerPixel == 16)
	x += ((pScrn->frameX0) & 1);
    else if (pScrn->bitsPerPixel == 32)
	x += ((pScrn->frameX0+2) & 3) - 2;

    /*
    * Make these even when used.  There is a bug/feature on at least
    * some chipsets that causes a "shadow" of the cursor in interlaced
    * mode.  Making this even seems to have no visible effect, so just
    * do it for the generic case.
    */

    if (x < 0) {
	xoff = ((-x) & 0xFE);
	x = 0;
    } else {
	xoff = 0;
    }

    if (y < 0) {
	yoff = ((-y) & 0xFE);
	y = 0;
    } else {
	yoff = 0;
    }

    /* This is the recomended order to move the cursor */
    outCRReg( 0x46, (x & 0xff00)>>8 );
    outCRReg( 0x47, (x & 0xff) );
    outCRReg( 0x49, (y & 0xff) );
    outCRReg( 0x4e, xoff );
    outCRReg( 0x4f, yoff );
    outCRReg( 0x48, (y & 0xff00)>>8 );
}


static void 
SavageSetCursorColors(
    ScrnInfoPtr pScrn,
    int bg,
    int fg)
{
    SavagePtr psav = SAVPTR(pScrn);
    Bool bNeedExtra = FALSE;

    /* Clock doubled modes need an extra cursor stack write. */
    bNeedExtra =
        (psav->CursorInfoRec->Flags & HARDWARE_CURSOR_TRUECOLOR_AT_8BPP);

    if( psav->Chipset == S3_SAVAGE_MX )
        bNeedExtra = TRUE;

    switch (pScrn->bitsPerPixel) {
    case 16:
	if (pScrn->weight.green == 5) {
	    fg = ((fg & 0xf80000) >> 9) |
		((fg & 0xf800) >> 6) |
		((fg & 0xf8) >> 3);
	    bg = ((bg & 0xf80000) >> 9) |
		((bg & 0xf800) >> 6) |
		((bg & 0xf8) >> 3);
	} else {
	    fg = ((fg & 0xf80000) >> 8) |
		((fg & 0xfc00) >> 5) |
		((fg & 0xf8) >> 3);
	    bg = ((bg & 0xf80000) >> 8) |
		((bg & 0xfc00) >> 5) |
		((bg & 0xf8) >> 3);
	}
	/* Reset the cursor color stack pointer */
        inCRReg( 0x45 );
        outCRReg( 0x4a, fg );
        outCRReg( 0x4a, fg>>8 );
	if( bNeedExtra )
	{
	    outCRReg( 0x4a, fg );
	    outCRReg( 0x4a, fg>>8 );
	}
	/* Reset the cursor color stack pointer */
        inCRReg( 0x45 );
        outCRReg( 0x4b, bg );
        outCRReg( 0x4b, bg>>8 );
	if( bNeedExtra )
	{
	    outCRReg( 0x4b, bg );
	    outCRReg( 0x4b, bg>>8 );
	}
        break;
    case 8:
	if( !bNeedExtra )
	{
	    /* Reset the cursor color stack pointer */
	    inCRReg(0x45);
	    /* Write foreground */
	    outCRReg(0x4a, fg);
	    outCRReg(0x4a, fg);
	    /* Reset the cursor color stack pointer */
	    inCRReg(0x45);
	    /* Write background */
	    outCRReg(0x4b, bg);
	    outCRReg(0x4b, bg);
	    break;
	}
	/* else */
	/* FALLTHROUGH */
    case 24:
    case 32:
	/* Do it straight, full 24 bit color. */
      
	/* Reset the cursor color stack pointer */
	inCRReg(0x45);
	/* Write low, mid, high bytes - foreground */
	outCRReg(0x4a, fg);
	outCRReg(0x4a, fg >> 8);
	outCRReg(0x4a, fg >> 16);
	/* Reset the cursor color stack pointer */
	inCRReg(0x45);
	/* Write low, mid, high bytes - background */
	outCRReg(0x4b, bg);
	outCRReg(0x4b, bg >> 8);
	outCRReg(0x4b, bg >> 16);
	break;
    }
}
