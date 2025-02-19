/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/tdfx/tdfx_dga.c,v 1.3 2000/06/17 00:03:25 martin Exp $ */

#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86_ansic.h"
#include "dgaproc.h"

#include "tdfx.h"

static Bool TDFX_OpenFramebuffer(ScrnInfoPtr, char **, unsigned char **, 
				 int *, int *, int *);
static Bool TDFX_SetMode(ScrnInfoPtr, DGAModePtr);
static int  TDFX_GetViewport(ScrnInfoPtr);
static void TDFX_SetViewport(ScrnInfoPtr, int, int, int);
static void TDFX_FillRect(ScrnInfoPtr, int, int, int, int, unsigned long);
static void TDFX_BlitRect(ScrnInfoPtr, int, int, int, int, int, int);
static void TDFX_BlitTransRect(ScrnInfoPtr, int, int, int, int, int, int, 
			       unsigned long);


static
DGAFunctionRec TDFX_DGAFuncs = {
  TDFX_OpenFramebuffer,
  0,
  TDFX_SetMode,
  TDFX_SetViewport,
  TDFX_GetViewport,
  TDFXSync,
  TDFX_FillRect,
  TDFX_BlitRect,
  TDFX_BlitTransRect
};

/*
We should allow you to change the depth, but the accel driver isn't
ready to handle that yet. Also, we are currently taking the default
visual and using it for the visual reported by DGA. It would be
relatively easy to at least give DirectColor and TrueColor visuals
instead of just the default
*/
Bool
TDFXDGAInit(ScreenPtr pScreen)
{
  ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
  TDFXPtr pTDFX;
  DisplayModePtr pMode, firstMode;
  DGAModePtr modes=0, newmodes=0, currentMode;
  int num=0;
  Bool oneMore;

  pTDFX = TDFXPTR(pScrn);
  pMode = firstMode = pScrn->modes;

  while (pMode) {
    newmodes = xrealloc(modes, (num+1)*sizeof(DGAModeRec));
    oneMore = FALSE;
    if (!newmodes) {
      xfree(modes);
      return FALSE;
    }
    modes = newmodes;

SECOND_PASS:

    currentMode = modes+num;
    num++;
    currentMode->mode = pMode;
    currentMode->flags = DGA_CONCURRENT_ACCESS | DGA_PIXMAP_AVAILABLE;
    if (!pTDFX->NoAccel)
      currentMode->flags |= DGA_FILL_RECT | DGA_BLIT_RECT;
    if (pMode->Flags & V_DBLSCAN)
      currentMode->flags |= DGA_DOUBLESCAN;
    if (pMode->Flags & V_INTERLACE)
      currentMode->flags |= DGA_INTERLACED;
    currentMode->byteOrder = pScrn->imageByteOrder;
    currentMode->depth = pScrn->depth;
    currentMode->bitsPerPixel = pScrn->bitsPerPixel;
    currentMode->red_mask = pScrn->mask.red;
    currentMode->green_mask = pScrn->mask.green;
    currentMode->blue_mask = pScrn->mask.blue;
    currentMode->visualClass = pScrn->defaultVisual;
    currentMode->viewportWidth = pMode->HDisplay;
    currentMode->viewportHeight = pMode->VDisplay;
    currentMode->xViewportStep = (3 - pTDFX->cpp);
    currentMode->yViewportStep = 1;
    currentMode->viewportFlags = DGA_FLIP_RETRACE;
    currentMode->offset = pTDFX->fbOffset;
    currentMode->address = pTDFX->FbBase;

    if (oneMore) { /* first one is narrow width */
      currentMode->bytesPerScanline = ((pMode->HDisplay * pTDFX->cpp) + 3) & ~3L;
      currentMode->imageWidth = pMode->HDisplay;
      currentMode->imageHeight =  pScrn->virtualY;
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
      currentMode->bytesPerScanline = ((pScrn->displayWidth * pTDFX->cpp) + 3) & ~3L;
      currentMode->imageWidth = pScrn->displayWidth;
      currentMode->imageHeight =  pScrn->virtualY;
      currentMode->pixmapWidth = currentMode->imageWidth;
      currentMode->pixmapHeight = currentMode->imageHeight;
      currentMode->maxViewportX = currentMode->imageWidth - 
	currentMode->viewportWidth;
      /* this might need to get clamped to some maximum */
      currentMode->maxViewportY = currentMode->imageHeight -
	currentMode->viewportHeight;
    }	    
    
    pMode = pMode->next;
    if (pMode == firstMode) break;
  }
  
  pTDFX->DGAModes = modes;
  
  return DGAInit(pScreen, &TDFX_DGAFuncs, modes, num);  
}

static Bool
TDFX_SetMode(ScrnInfoPtr pScrn, DGAModePtr pMode)
{
   static int OldDisplayWidth[MAXSCREENS];
   int index = pScrn->pScreen->myNum;

   TDFXPtr pTDFX = TDFXPTR(pScrn);

   if (!pMode) { /* restore the original mode */
     /* put the ScreenParameters back */
     pScrn->displayWidth = OldDisplayWidth[index];
     TDFXSwitchMode(index, pScrn->currentMode, 0);
     pTDFX->DGAactive = FALSE;
   } else {
     if (!pTDFX->DGAactive) {  /* save the old parameters */
       OldDisplayWidth[index] = pScrn->displayWidth;
       
       pTDFX->DGAactive = TRUE;
     }

     pScrn->displayWidth = pMode->bytesPerScanline / pTDFX->cpp;

     TDFXSwitchMode(index, pMode->mode, 0);
   }
   
   return TRUE;
}

static int  
TDFX_GetViewport(ScrnInfoPtr pScrn)
{
    TDFXPtr pTDFX = TDFXPTR(pScrn);

    return pTDFX->DGAViewportStatus;
}

static void 
TDFX_SetViewport(ScrnInfoPtr pScrn, int x, int y, int flags)
{
   TDFXPtr pTDFX = TDFXPTR(pScrn);

   TDFXAdjustFrame(pScrn->pScreen->myNum, x, y, flags);
   pTDFX->DGAViewportStatus = 0;  /* TDFXAdjustFrame loops until finished */
}

static void 
TDFX_FillRect(ScrnInfoPtr pScrn, int x, int y, int w, int h, 
	      unsigned long color)
{
  TDFXPtr pTDFX = TDFXPTR(pScrn);

  if (pTDFX->AccelInfoRec) {
    (*pTDFX->AccelInfoRec->SetupForSolidFill)(pScrn, color, GXcopy, ~0);
    (*pTDFX->AccelInfoRec->SubsequentSolidFillRect)(pScrn, x, y, w, h);
  }
}

static void 
TDFX_BlitRect(ScrnInfoPtr pScrn, int srcx, int srcy, int w, int h, 
	      int dstx, int dsty)
{
  TDFXPtr pTDFX = TDFXPTR(pScrn);

  if (pTDFX->AccelInfoRec) {
    int xdir = ((srcx < dstx) && (srcy == dsty)) ? -1 : 1;
    int ydir = (srcy < dsty) ? -1 : 1;

    (*pTDFX->AccelInfoRec->SetupForScreenToScreenCopy)(pScrn, xdir, ydir, 
						       GXcopy, ~0, -1);
    (*pTDFX->AccelInfoRec->SubsequentScreenToScreenCopy)(pScrn, srcx, srcy, 
							 dstx, dsty, w, h);
  }
}


static void 
TDFX_BlitTransRect(
   ScrnInfoPtr pScrn, 
   int srcx, int srcy, 
   int w, int h, 
   int dstx, int dsty,
   unsigned long color
){
  /* this one should be separate since the XAA function would
     prohibit usage of ~0 as the key */
}


static Bool 
TDFX_OpenFramebuffer(
   ScrnInfoPtr pScrn, 
   char **name,
   unsigned char **mem,
   int *size,
   int *offset,
   int *flags
){
    TDFXPtr pTDFX = TDFXPTR(pScrn);

    *name = NULL; 		/* no special device */
    *mem = (unsigned char*)pTDFX->LinearAddr[0];
    *size = pTDFX->FbMapSize;
    *offset = 0;
    *flags = DGA_NEED_ROOT;

    return TRUE;
}
