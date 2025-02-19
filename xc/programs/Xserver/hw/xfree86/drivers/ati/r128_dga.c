/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/ati/r128_dga.c,v 1.4 2000/11/21 23:10:33 tsi Exp $ */
/*
 * Authors:
 *   Ove K�ven <ovek@transgaming.com>,
 *    borrowing some code from the Chips and MGA drivers.
 */

				/* Driver data structures */
#include "r128.h"
#include "r128_probe.h"

				/* X and server generic header files */
#include "xf86.h"

				/* DGA support */
#include "dgaproc.h"


static Bool R128_OpenFramebuffer(ScrnInfoPtr, char **, unsigned char **,
					int *, int *, int *);
static Bool R128_SetMode(ScrnInfoPtr, DGAModePtr);
static int  R128_GetViewport(ScrnInfoPtr);
static void R128_SetViewport(ScrnInfoPtr, int, int, int);
static void R128_FillRect(ScrnInfoPtr, int, int, int, int, unsigned long);
static void R128_BlitRect(ScrnInfoPtr, int, int, int, int, int, int);
#if 0
static void R128_BlitTransRect(ScrnInfoPtr, int, int, int, int, int, int,
					unsigned long);
#endif

static
DGAFunctionRec R128_DGAFuncs = {
   R128_OpenFramebuffer,
   NULL,
   R128_SetMode,
   R128_SetViewport,
   R128_GetViewport,
   R128WaitForIdle,
   R128_FillRect,
   R128_BlitRect,
#if 0
   R128_BlitTransRect
#else
   NULL
#endif
};


static DGAModePtr
R128SetupDGAMode(
   ScrnInfoPtr pScrn,
   DGAModePtr modes,
   int *num,
   int bitsPerPixel,
   int depth,
   Bool pixmap,
   int secondPitch,
   unsigned long red,
   unsigned long green,
   unsigned long blue,
   short visualClass
){
   R128InfoPtr info = R128PTR(pScrn);
   DGAModePtr newmodes = NULL, currentMode;
   DisplayModePtr pMode, firstMode;
   int otherPitch, Bpp = bitsPerPixel >> 3;
   Bool oneMore;

   pMode = firstMode = pScrn->modes;

   while(pMode) {

	otherPitch = secondPitch ? secondPitch : pMode->HDisplay;

	if(pMode->HDisplay != otherPitch) {
	    newmodes = xrealloc(modes, (*num + 2) * sizeof(DGAModeRec));
	    oneMore = TRUE;
	} else {
	    newmodes = xrealloc(modes, (*num + 1) * sizeof(DGAModeRec));
	    oneMore = FALSE;
	}

	if(!newmodes) {
	   xfree(modes);
	   return NULL;
	}
	modes = newmodes;

SECOND_PASS:

	currentMode = modes + *num;
	(*num)++;

	currentMode->mode = pMode;
	/* FIXME: is concurrent access really possible? */
	currentMode->flags = DGA_CONCURRENT_ACCESS;
	if(pixmap)
	   currentMode->flags |= DGA_PIXMAP_AVAILABLE;
	if(info->accel)
	   currentMode->flags |= DGA_FILL_RECT | DGA_BLIT_RECT;
	if(pMode->Flags & V_DBLSCAN)
	   currentMode->flags |= DGA_DOUBLESCAN;
	if(pMode->Flags & V_INTERLACE)
	   currentMode->flags |= DGA_INTERLACED;
	currentMode->byteOrder = pScrn->imageByteOrder;
	currentMode->depth = depth;
	currentMode->bitsPerPixel = bitsPerPixel;
	currentMode->red_mask = red;
	currentMode->green_mask = green;
	currentMode->blue_mask = blue;
	currentMode->visualClass = visualClass;
	currentMode->viewportWidth = pMode->HDisplay;
	currentMode->viewportHeight = pMode->VDisplay;
	currentMode->xViewportStep = 8;
	currentMode->yViewportStep = 1;
	currentMode->viewportFlags = DGA_FLIP_RETRACE;
	currentMode->offset = 0;
	currentMode->address = (unsigned char*)info->LinearAddr;

	if(oneMore) { /* first one is narrow width */
	    currentMode->bytesPerScanline = ((pMode->HDisplay * Bpp) + 3) & ~3L;
	    currentMode->imageWidth = pMode->HDisplay;
	    currentMode->imageHeight = pMode->VDisplay;
	    currentMode->pixmapWidth = currentMode->imageWidth;
	    currentMode->pixmapHeight = currentMode->imageHeight;
	    currentMode->maxViewportX = currentMode->imageWidth -
					currentMode->viewportWidth;
	    /* this might need to get clamped to some maximum */
	    currentMode->maxViewportY = currentMode->imageHeight -
					currentMode->viewportHeight;
	    oneMore = FALSE;
	    goto SECOND_PASS;
	} else {
	    currentMode->bytesPerScanline = ((otherPitch * Bpp) + 3) & ~3L;
	    currentMode->imageWidth = otherPitch;
	    currentMode->imageHeight = pMode->VDisplay;
	    currentMode->pixmapWidth = currentMode->imageWidth;
	    currentMode->pixmapHeight = currentMode->imageHeight;
	    currentMode->maxViewportX = currentMode->imageWidth -
					currentMode->viewportWidth;
	    /* this might need to get clamped to some maximum */
	    currentMode->maxViewportY = currentMode->imageHeight -
					currentMode->viewportHeight;
	}

	pMode = pMode->next;
	if(pMode == firstMode)
	   break;
   }

   return modes;
}


Bool
R128DGAInit(ScreenPtr pScreen)
{
   ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
   R128InfoPtr info = R128PTR(pScrn);
   DGAModePtr modes = NULL;
   int num = 0;

   /* 8 */
   modes = R128SetupDGAMode (pScrn, modes, &num, 8, 8,
		(pScrn->bitsPerPixel == 8),
		(pScrn->bitsPerPixel != 8) ? 0 : pScrn->displayWidth,
		0, 0, 0, PseudoColor);

   /* 15 */
   modes = R128SetupDGAMode (pScrn, modes, &num, 16, 15,
		(pScrn->bitsPerPixel == 16),
		(pScrn->depth != 15) ? 0 : pScrn->displayWidth,
		0x7c00, 0x03e0, 0x001f, TrueColor);

   modes = R128SetupDGAMode (pScrn, modes, &num, 16, 15,
		(pScrn->bitsPerPixel == 16),
		(pScrn->depth != 15) ? 0 : pScrn->displayWidth,
		0x7c00, 0x03e0, 0x001f, DirectColor);

   /* 16 */
   modes = R128SetupDGAMode (pScrn, modes, &num, 16, 16,
		(pScrn->bitsPerPixel == 16),
		(pScrn->depth != 16) ? 0 : pScrn->displayWidth,
		0xf800, 0x07e0, 0x001f, TrueColor);

   modes = R128SetupDGAMode (pScrn, modes, &num, 16, 16,
		(pScrn->bitsPerPixel == 16),
		(pScrn->depth != 16) ? 0 : pScrn->displayWidth,
		0xf800, 0x07e0, 0x001f, DirectColor);

   /* 24 */
   modes = R128SetupDGAMode (pScrn, modes, &num, 24, 24,
		(pScrn->bitsPerPixel == 24),
		(pScrn->bitsPerPixel != 24) ? 0 : pScrn->displayWidth,
		0xff0000, 0x00ff00, 0x0000ff, TrueColor);

   modes = R128SetupDGAMode (pScrn, modes, &num, 24, 24,
		(pScrn->bitsPerPixel == 24),
		(pScrn->bitsPerPixel != 24) ? 0 : pScrn->displayWidth,
		0xff0000, 0x00ff00, 0x0000ff, DirectColor);

   /* 32 */
   modes = R128SetupDGAMode (pScrn, modes, &num, 32, 24,
		(pScrn->bitsPerPixel == 32),
		(pScrn->bitsPerPixel != 32) ? 0 : pScrn->displayWidth,
		0xff0000, 0x00ff00, 0x0000ff, TrueColor);

   modes = R128SetupDGAMode (pScrn, modes, &num, 32, 24,
		(pScrn->bitsPerPixel == 32),
		(pScrn->bitsPerPixel != 32) ? 0 : pScrn->displayWidth,
		0xff0000, 0x00ff00, 0x0000ff, DirectColor);

   info->numDGAModes = num;
   info->DGAModes = modes;

   return DGAInit(pScreen, &R128_DGAFuncs, modes, num);
}


static Bool
R128_SetMode(
   ScrnInfoPtr pScrn,
   DGAModePtr pMode
){
   static R128FBLayout SavedLayouts[MAXSCREENS];
   int indx = pScrn->pScreen->myNum;
   R128InfoPtr info = R128PTR(pScrn);

   if(!pMode) { /* restore the original mode */
	/* put the ScreenParameters back */
	if(info->DGAactive)
	    memcpy(&info->CurrentLayout, &SavedLayouts[indx], sizeof(R128FBLayout));

	pScrn->currentMode = info->CurrentLayout.mode;

	R128SwitchMode(indx, pScrn->currentMode, 0);
	R128AdjustFrame(indx, 0, 0, 0);
	info->DGAactive = FALSE;
   } else {
	if(!info->DGAactive) {  /* save the old parameters */
	    memcpy(&SavedLayouts[indx], &info->CurrentLayout, sizeof(R128FBLayout));
	    info->DGAactive = TRUE;
	}

	info->CurrentLayout.bitsPerPixel = pMode->bitsPerPixel;
	info->CurrentLayout.depth = pMode->depth;
	info->CurrentLayout.displayWidth = pMode->bytesPerScanline /
					    (pMode->bitsPerPixel >> 3);
	info->CurrentLayout.pixel_bytes = pMode->bitsPerPixel / 8;
	info->CurrentLayout.pixel_code  = (pMode->bitsPerPixel != 16
					  ? pMode->bitsPerPixel
					  : pMode->depth);
	/* R128ModeInit() will set the mode field */

	R128SwitchMode(indx, pMode->mode, 0);
   }

   return TRUE;
}



static int
R128_GetViewport(
  ScrnInfoPtr pScrn
){
    R128InfoPtr info = R128PTR(pScrn);

    return info->DGAViewportStatus;
}


static void
R128_SetViewport(
   ScrnInfoPtr pScrn,
   int x, int y,
   int flags
){
   R128InfoPtr info = R128PTR(pScrn);

   R128AdjustFrame(pScrn->pScreen->myNum, x, y, flags);
   info->DGAViewportStatus = 0;  /* FIXME */
}


static void
R128_FillRect (
   ScrnInfoPtr pScrn,
   int x, int y, int w, int h,
   unsigned long color
){
    R128InfoPtr info = R128PTR(pScrn);

    if(info->accel) {
	(*info->accel->SetupForSolidFill)(pScrn, color, GXcopy, (CARD32)(~0));
	(*info->accel->SubsequentSolidFillRect)(pScrn, x, y, w, h);
	SET_SYNC_FLAG(info->accel);
    }
}

static void
R128_BlitRect(
   ScrnInfoPtr pScrn,
   int srcx, int srcy,
   int w, int h,
   int dstx, int dsty
){
    R128InfoPtr info = R128PTR(pScrn);

    if(info->accel) {
	int xdir = ((srcx < dstx) && (srcy == dsty)) ? -1 : 1;
	int ydir = (srcy < dsty) ? -1 : 1;

	(*info->accel->SetupForScreenToScreenCopy)(
		pScrn, xdir, ydir, GXcopy, (CARD32)(~0), -1);
	(*info->accel->SubsequentScreenToScreenCopy)(
		pScrn, srcx, srcy, dstx, dsty, w, h);
	SET_SYNC_FLAG(info->accel);
    }
}


#if 0
static void
R128_BlitTransRect(
   ScrnInfoPtr pScrn,
   int srcx, int srcy,
   int w, int h,
   int dstx, int dsty,
   unsigned long color
){
  /* this one should be separate since the XAA function would
     prohibit usage of ~0 as the key */
}
#endif


static Bool
R128_OpenFramebuffer(
   ScrnInfoPtr pScrn,
   char **name,
   unsigned char **mem,
   int *size,
   int *offset,
   int *flags
){
    R128InfoPtr info = R128PTR(pScrn);

    *name = NULL;               /* no special device */
    *mem = (unsigned char*)info->LinearAddr;
    *size = info->FbMapSize;
    *offset = 0;
    *flags = /* DGA_NEED_ROOT */ 0; /* don't need root, just /dev/mem access */

    return TRUE;
}
