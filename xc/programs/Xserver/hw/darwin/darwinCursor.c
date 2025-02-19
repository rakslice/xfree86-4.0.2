/**************************************************************
 *
 * Cursor support for Darwin X Server
 *
 * Three different cursor modes are possible:
 *  X (0)         - tracking via Darwin kernel,
 *                  display via X machine independent
 *  Kernel (1)    - tracking and display via Darwin kernel
 *                  (not currently supported)
 *  Hardware (2)  - tracking and display via hardware
 *
 * The X software cursor uses the Darwin software cursor
 * routines in IOFramebuffer.cpp to track the cursor, but
 * displays the cursor image using the X machine
 * independent display cursor routines in midispcur.c.
 *
 * The kernel cursor uses IOFramebuffer.cpp routines to
 * track and display the cursor. This gives better
 * performance as the display calls don't have to cross
 * the kernel boundary. Unfortunately, this mode has
 * synchronization issues with the user land X server
 * and isn't currently used.
 *
 * Hardware cursor support lets the hardware handle these
 * details.
 *
 * Kernel and hardware cursor mode only work for cursors
 * up to a certain size, currently 16x16 pixels. If a
 * bigger cursor is set, we fallback to X cursor mode.
 *
 * HISTORY:
 * 1.0 by Torrey T. Lyons, October 30, 2000
 *
 **************************************************************/
/* $XFree86: xc/programs/Xserver/hw/darwin/darwinCursor.c,v 1.2 2000/11/28 17:25:10 dawes Exp $ */

#include "scrnintstr.h"
#include "cursorstr.h"
#include "micmap.h"
#define NO_CFPLUGIN
#include <IOKit/graphics/IOGraphicsLib.h>
#include <IOKit/hidsystem/IOHIDLib.h>
#include "darwin.h"
#include "mipointrst.h"

#define DUMP_DARWIN_CURSOR FALSE

// The cursors format are documented in IOFramebufferShared.h.
#define RGBto34WithGamma(red, green, blue)  \
    (  0x000F                               \
     | (((red) & 0xF) << 12)                \
     | (((green) & 0xF) << 8)               \
     | (((blue) & 0xF) << 4) )
#define RGBto38WithGamma(red, green, blue)  \
    (  0xFF << 24                           \
     | (((red) & 0xFF) << 16)               \
     | (((green) & 0xFF) << 8)              \
     | (((blue) & 0xFF)) )
#define HighBitOf32 0x80000000

typedef struct {
    Bool                    canHWCursor;
    short                   cursorMode;
    RecolorCursorProcPtr    RecolorCursor;
    InstallColormapProcPtr  InstallColormap;
    QueryBestSizeProcPtr    QueryBestSize;
    miPointerSpriteFuncPtr  spriteFuncs;
    ColormapPtr             pInstalledMap;
} DarwinCursorScreenRec, *DarwinCursorScreenPtr;

extern DarwinFramebufferRec dfb;
static int darwinCursorScreenIndex = -1;
static unsigned long darwinCursorGeneration = 0;

/*
===========================================================================

 Pointer sprite functions

===========================================================================
*/

/*
    Realizing the Darwin hardware cursor (ie. converting from the
    X representation to the Darwin representation) is complicated
    by the fact that we have three different potential cursor
    formats to go to, one for each bit depth (8, 15, or 24).
    The Darwin formats are documented in IOFramebufferShared.h.
    X cursors are represented as two pieces, a source and a mask.
    The mask is a bitmap indicating which parts of the cursor are 
    transparent and which parts are drawn.  The source is a bitmap
    indicating which parts of the non-transparent portion of the the
    cursor should be painted in the foreground color and which should
    be painted in the background color. The bitmaps are given in
    32-bit format with least significant byte and bit first.
    (This is opposite PowerPC Darwin.)
*/

typedef struct {
    unsigned char image[CURSORWIDTH*CURSORHEIGHT];
    unsigned char mask[CURSORWIDTH*CURSORHEIGHT];
} cursorPrivRec, *cursorPrivPtr;

/*
 * DarwinRealizeCursor8
 * Convert the X cursor representation to an 8-bit depth
 * format for Darwin. This function assumes the maximum cursor
 * width is a multiple of 8.
 */
static Bool
DarwinRealizeCursor8(
    ScreenPtr pScreen,
    CursorPtr pCursor)
{
    cursorPrivPtr   newCursor;
    unsigned char   *newSourceP, *newMaskP;
    CARD32          *oldSourceP, *oldMaskP;
    xColorItem      fgColor, bgColor;
    int             index, x, y, rowPad;
    int             cursorWidth, cursorHeight;
    ColormapPtr     pmap;

    // check cursor size just to be sure
    cursorWidth = pCursor->bits->width;
    cursorHeight = pCursor->bits->height;
    if (cursorHeight > CURSORHEIGHT || cursorWidth > CURSORWIDTH)
        return FALSE;

    // get cursor colors in colormap
    index = pScreen->myNum;
    pmap = miInstalledMaps[index];
    if (!pmap) return FALSE;

    fgColor.red = pCursor->foreRed;
    fgColor.green = pCursor->foreGreen;
    fgColor.blue = pCursor->foreBlue;
    FakeAllocColor(pmap, &fgColor);
    bgColor.red = pCursor->backRed;
    bgColor.green = pCursor->backGreen;
    bgColor.blue = pCursor->backBlue;
    FakeAllocColor(pmap, &bgColor);
    FakeFreeColor(pmap, fgColor.pixel);
    FakeFreeColor(pmap, bgColor.pixel);

    // allocate memory for new cursor image
    newCursor = xalloc( sizeof(cursorPrivRec) );
    if (!newCursor)
        return FALSE;
    memset( newCursor->image, pScreen->blackPixel, CURSORWIDTH*CURSORHEIGHT );
    memset( newCursor->mask, 0, CURSORWIDTH*CURSORHEIGHT );

    // convert to 8-bit Darwin cursor format
    oldSourceP = (CARD32 *) pCursor->bits->source;
    oldMaskP = (CARD32 *) pCursor->bits->mask;
    newSourceP = newCursor->image;
    newMaskP = newCursor->mask;
    rowPad = CURSORWIDTH - cursorWidth;

    for (y = 0; y < cursorHeight; y++) {
        for (x = 0; x < cursorWidth; x++) {
            if (*oldSourceP & (HighBitOf32 >> x))
                *newSourceP = fgColor.pixel;
            else
                *newSourceP = bgColor.pixel;
            if (*oldMaskP & (HighBitOf32 >> x))
                *newMaskP = 255;
            else
                *newSourceP = pScreen->blackPixel;
            newSourceP++; newMaskP++;
        }
        oldSourceP++; oldMaskP++;
        newSourceP += rowPad; newMaskP += rowPad;
    }

    // save the result
    pCursor->devPriv[pScreen->myNum] = (pointer) newCursor;
    return TRUE;
}


/*
 * DarwinRealizeCursor15
 * Convert the X cursor representation to an 15-bit depth
 * format for Darwin.
 */
static Bool
DarwinRealizeCursor15(
    ScreenPtr       pScreen,
    CursorPtr       pCursor)
{
    unsigned short  *newCursor;
    unsigned short  fgPixel, bgPixel;
    unsigned short  *newSourceP;
    CARD32          *oldSourceP, *oldMaskP;
    int             x, y, rowPad;
    int             cursorWidth, cursorHeight;

    // check cursor size just to be sure
    cursorWidth = pCursor->bits->width;
    cursorHeight = pCursor->bits->height;
    if (cursorHeight > CURSORHEIGHT || cursorWidth > CURSORWIDTH)
       return FALSE;

    // allocate memory for new cursor image
    newCursor = xalloc( CURSORWIDTH*CURSORHEIGHT*sizeof(short) );
    if (!newCursor)
        return FALSE;
    memset( newCursor, 0, CURSORWIDTH*CURSORHEIGHT*sizeof(short) );

    // calculate pixel values
    fgPixel = RGBto34WithGamma( pCursor->foreRed, pCursor->foreGreen,
                                pCursor->foreBlue );
    bgPixel = RGBto34WithGamma( pCursor->backRed, pCursor->backGreen,
                                pCursor->backBlue );

    // convert to 15-bit Darwin cursor format
    oldSourceP = (CARD32 *) pCursor->bits->source;
    oldMaskP = (CARD32 *) pCursor->bits->mask;
    newSourceP = newCursor;
    rowPad = CURSORWIDTH - cursorWidth;

    for (y = 0; y < cursorHeight; y++) {
        for (x = 0; x < cursorWidth; x++) {
            if (*oldMaskP & (HighBitOf32 >> x)) {
                if (*oldSourceP & (HighBitOf32 >> x))
                    *newSourceP = fgPixel;
                else
                    *newSourceP = bgPixel;
            } else {
                *newSourceP = 0;
            }
            newSourceP++;
        }
        oldSourceP++; oldMaskP++;
        newSourceP += rowPad;
    }

#if DUMP_DARWIN_CURSOR
    // Write out the cursor
    ErrorF("Cursor: 0x%x\n", pCursor);
    ErrorF("Width = %i, Height = %i, RowPad = %i\n", cursorWidth,
            cursorHeight, rowPad);
    for (y = 0; y < cursorHeight; y++) {
        newSourceP = newCursor + y*CURSORWIDTH;
        for (x = 0; x < cursorWidth; x++) {
            if (*newSourceP == fgPixel)
                ErrorF("x");
            else if (*newSourceP == bgPixel)
                ErrorF("o");
            else
                ErrorF(" ");
            newSourceP++;
        }
        ErrorF("\n");
    }
#endif

    // save the result
    pCursor->devPriv[pScreen->myNum] = (pointer) newCursor;
    return TRUE;
}


/*
 * DarwinRealizeCursor24
 * Convert the X cursor representation to an 24-bit depth
 * format for Darwin. This function assumes the maximum cursor
 * width is a multiple of 8.
 */
static Bool
DarwinRealizeCursor24(
    ScreenPtr       pScreen,
    CursorPtr       pCursor)
{
    unsigned int    *newCursor;
    unsigned int    fgPixel, bgPixel;
    unsigned int    *newSourceP;
    CARD32          *oldSourceP, *oldMaskP;
    int             x, y, rowPad;
    int             cursorWidth, cursorHeight;

    // check cursor size just to be sure
    cursorWidth = pCursor->bits->width;
    cursorHeight = pCursor->bits->height;
    if (cursorHeight > CURSORHEIGHT || cursorWidth > CURSORWIDTH)
       return FALSE;

    // allocate memory for new cursor image
    newCursor = xalloc( CURSORWIDTH*CURSORHEIGHT*sizeof(int) );
    if (!newCursor)
        return FALSE;
    memset( newCursor, 0, CURSORWIDTH*CURSORHEIGHT*sizeof(int) );

    // calculate pixel values
    fgPixel = RGBto38WithGamma( pCursor->foreRed, pCursor->foreGreen,
                                pCursor->foreBlue );
    bgPixel = RGBto38WithGamma( pCursor->backRed, pCursor->backGreen,
                                pCursor->backBlue );

    // convert to 24-bit Darwin cursor format
    oldSourceP = (CARD32 *) pCursor->bits->source;
    oldMaskP = (CARD32 *) pCursor->bits->mask;
    newSourceP = newCursor;
    rowPad = CURSORWIDTH - cursorWidth;

    for (y = 0; y < cursorHeight; y++) {
        for (x = 0; x < cursorWidth; x++) {
            if (*oldMaskP & (HighBitOf32 >> x)) {
                if (*oldSourceP & (HighBitOf32 >> x))
                    *newSourceP = fgPixel;
                else
                    *newSourceP = bgPixel;
            } else {
                *newSourceP = 0;
            }
            newSourceP++;
        }
        oldSourceP++; oldMaskP++;
        newSourceP += rowPad;
    }

#if DUMP_DARWIN_CURSOR
    // Write out the cursor
    ErrorF("Cursor: 0x%x\n", pCursor);
    ErrorF("Width = %i, Height = %i, RowPad = %i\n", cursorWidth,
            cursorHeight, rowPad);
    for (y = 0; y < cursorHeight; y++) {
        newSourceP = newCursor + y*CURSORWIDTH;
        for (x = 0; x < cursorWidth; x++) {
            if (*newSourceP == fgPixel)
                ErrorF("x");
            else if (*newSourceP == bgPixel)
                ErrorF("o");
            else
                ErrorF(" ");
            newSourceP++;
        }
        ErrorF("\n");
    }
#endif

    // save the result
    pCursor->devPriv[pScreen->myNum] = (pointer) newCursor;
    return TRUE;
}


/*
 * DarwinRealizeCursor
 * 
 */
static Bool
DarwinRealizeCursor(
    ScreenPtr       pScreen,
    CursorPtr       pCursor)
{
    Bool                        result;
    DarwinCursorScreenPtr       ScreenPriv = (DarwinCursorScreenPtr)
        pScreen->devPrivates[darwinCursorScreenIndex].ptr;

    if ((pCursor->bits->height > CURSORHEIGHT) ||
        (pCursor->bits->width > CURSORWIDTH) ||
        // FIXME: this condition is not needed after kernel cursor works
        !ScreenPriv->canHWCursor) {
        result = (*ScreenPriv->spriteFuncs->RealizeCursor)(pScreen, pCursor);
    } else if (dfb.bitsPerPixel == 8) {
        result = DarwinRealizeCursor8(pScreen, pCursor);
    } else if (dfb.bitsPerPixel == 16) {
        result = DarwinRealizeCursor15(pScreen, pCursor);
    } else {
        result = DarwinRealizeCursor24(pScreen, pCursor);
    }

    return result;
}


/*
 * DarwinUnrealizeCursor
 * 
 */
static Bool
DarwinUnrealizeCursor(
    ScreenPtr pScreen,
    CursorPtr pCursor)
{
    Bool                        result;
    DarwinCursorScreenPtr       ScreenPriv = (DarwinCursorScreenPtr)
        pScreen->devPrivates[darwinCursorScreenIndex].ptr;

    if ((pCursor->bits->height > CURSORHEIGHT) ||
        (pCursor->bits->width > CURSORWIDTH) ||
        // FIXME: this condition is not needed after kernel cursor works
        !ScreenPriv->canHWCursor) {
        result = (*ScreenPriv->spriteFuncs->UnrealizeCursor)(pScreen, pCursor);
    } else {
        xfree( pCursor->devPriv[pScreen->myNum] );
        result = TRUE;
    }

    return result;
}


/*
 * DarwinSetCursor
 * Set the cursor sprite and position
 * Use hardware cursor if possible
 */
static void
DarwinSetCursor(
    ScreenPtr       pScreen,
    CursorPtr       pCursor,
    int             x,
    int             y)
{
    kern_return_t               kr;
    StdFBShmem_t                *cshmem = dfb.cursorShmem;
    DarwinCursorScreenPtr       ScreenPriv = (DarwinCursorScreenPtr)
        pScreen->devPrivates[darwinCursorScreenIndex].ptr;

    // are we supposed to remove the cursor?
    if (!pCursor) {
        if (ScreenPriv->cursorMode == 0)
            (*ScreenPriv->spriteFuncs->SetCursor)(pScreen, 0, x, y);
        else {
            if (!cshmem->cursorShow) {
                cshmem->cursorShow++;
                if (cshmem->hardwareCursorActive) {
                    kr = IOFBSetCursorVisible(dfb.fbService, FALSE);
                    kern_assert( kr );
                }
            }
        }
        return;
    } 
 
    // can we use the kernel or hardware cursor?
    if ((pCursor->bits->height <= CURSORHEIGHT) &&
        (pCursor->bits->width <= CURSORWIDTH) &&
        // FIXME: condition not needed when kernel cursor works
        ScreenPriv->canHWCursor) {

        if (ScreenPriv->cursorMode == 0)    // remove the X cursor
            (*ScreenPriv->spriteFuncs->SetCursor)(pScreen, 0, x, y);
        ScreenPriv->cursorMode = 1;         // kernel cursor

        // change the cursor image in shared memory
        if (dfb.bitsPerPixel == 8) {
            cursorPrivPtr newCursor =
                    (cursorPrivPtr) pCursor->devPriv[pScreen->myNum];
            memcpy(cshmem->cursor.bw8.image[0], newCursor->image,
                        CURSORWIDTH*CURSORHEIGHT);
            memcpy(cshmem->cursor.bw8.mask[0], newCursor->mask,
                        CURSORWIDTH*CURSORHEIGHT);
        } else if (dfb.bitsPerPixel == 16) {
            unsigned short *newCursor =
                    (unsigned short *) pCursor->devPriv[pScreen->myNum];
            memcpy(cshmem->cursor.rgb.image[0], newCursor,
                        2*CURSORWIDTH*CURSORHEIGHT);
        } else {
            unsigned int *newCursor =
                    (unsigned int *) pCursor->devPriv[pScreen->myNum];
            memcpy(cshmem->cursor.rgb24.image[0], newCursor,
                        4*CURSORWIDTH*CURSORHEIGHT);
        }

        // FIXME: We always use a full size cursor, even if the image
        // is smaller because I couldn't get the padding to come out
        // right otherwise.
        cshmem->cursorSize[0].width = CURSORWIDTH;
        cshmem->cursorSize[0].height = CURSORHEIGHT;
        cshmem->hotSpot[0].x = pCursor->bits->xhot;
        cshmem->hotSpot[0].y = pCursor->bits->yhot;

        // try to use a hardware cursor
        if (ScreenPriv->canHWCursor) {
            kr = IOFBSetNewCursor(dfb.fbService, 0, 0, 0);
            // FIXME: this is a fatal error without the kernel cursor
            kern_assert( kr );
#if 0
            if (kr != KERN_SUCCESS) {
                ErrorF("Could not set new cursor with kernel return 0x%x.\n", kr);
                ScreenPriv->canHWCursor = FALSE;
            }
#endif
        }

        // make the new cursor visible
        if (cshmem->cursorShow)
            cshmem->cursorShow--;

        if (!cshmem->cursorShow && ScreenPriv->canHWCursor) {
            kr = IOFBSetCursorVisible(dfb.fbService, TRUE);
            // FIXME: this is a fatal error without the kernel cursor
            kern_assert( kr );
#if 0
            if (kr != KERN_SUCCESS) {
                ErrorF("Couldn't set hardware cursor visible with kernel return 0x%x.\n", kr);
                ScreenPriv->canHWCursor = FALSE;
            } else
#endif
                ScreenPriv->cursorMode = 2;     // hardware cursor
        }

	return; 
    }

    // otherwise we use a software cursor
    if (ScreenPriv->cursorMode) {
	/* remove the kernel or hardware cursor */
	DarwinSetCursor(pScreen, 0, x, y);
    }

    ScreenPriv->cursorMode = 0;
    (*ScreenPriv->spriteFuncs->SetCursor)(pScreen, pCursor, x, y);
}


/*
 * DarwinMoveCursor
 * Move the cursor. This is a noop for a kernel or hardware cursor.
 */
static void
DarwinMoveCursor(
    ScreenPtr   pScreen,
    int         x,
    int         y)
{
    DarwinCursorScreenPtr ScreenPriv = (DarwinCursorScreenPtr)
        pScreen->devPrivates[darwinCursorScreenIndex].ptr;

    // only the X cursor needs to be explicitly moved
    if (!ScreenPriv->cursorMode)
	(*ScreenPriv->spriteFuncs->MoveCursor)(pScreen, x, y);
}

static miPointerSpriteFuncRec darwinSpriteFuncsRec = {
    DarwinRealizeCursor,
    DarwinUnrealizeCursor,
    DarwinSetCursor,
    DarwinMoveCursor
};


/*
===========================================================================

 Pointer screen functions

===========================================================================
*/

/*
 * DarwinCursorOffScreen
 */
static Bool DarwinCursorOffScreen(ScreenPtr *pScreen, int *x, int *y)
{	return FALSE;
}


/*
 * DarwinCrossScreen
 */
static void DarwinCrossScreen(ScreenPtr pScreen, Bool entering)
{	return;
}


/*
 * DarwinWarpCursor
 * Change the cursor position without generating an event or motion history
 */
static void
DarwinWarpCursor(
    ScreenPtr               pScreen,
    int                     x,
    int                     y)
{
    kern_return_t           kr;

    kr = IOHIDSetMouseLocation( dfb.hidService, x, y );
    if (kr != KERN_SUCCESS) {
        ErrorF("Could not set cursor position with kernel return 0x%x.\n", kr);
    }
    miPointerWarpCursor(pScreen, x, y);
}

static miPointerScreenFuncRec darwinScreenFuncsRec = {
  DarwinCursorOffScreen,
  DarwinCrossScreen,
  DarwinWarpCursor,
};


/*
===========================================================================

 Other screen functions

===========================================================================
*/

/*
 * DarwinCursorQueryBestSize
 * Handle queries for best cursor size
 */
static void
DarwinCursorQueryBestSize(
   int              class, 
   unsigned short   *width,
   unsigned short   *height,
   ScreenPtr        pScreen)
{
    DarwinCursorScreenPtr ScreenPriv = (DarwinCursorScreenPtr)
        pScreen->devPrivates[darwinCursorScreenIndex].ptr;

    if (class == CursorShape) {
        *width = CURSORWIDTH;
        *height = CURSORHEIGHT;
    } else
        (*ScreenPriv->QueryBestSize)(class, width, height, pScreen);
}


/*
 * DarwinInitCursor
 * Initialize cursor support
 */
Bool 
DarwinInitCursor(
    ScreenPtr	pScreen)
{
    DarwinCursorScreenPtr   ScreenPriv;
    miPointerScreenPtr	    PointPriv;
    kern_return_t           kr;
 
    // start with no cursor displayed
    if (!dfb.cursorShmem->cursorShow++) {
        if (dfb.cursorShmem->hardwareCursorActive) {
            kr = IOFBSetCursorVisible(dfb.fbService, FALSE);
            kern_assert( kr );
        }
    }

    // initialize software cursor handling (always needed as backup)
    if (!miDCInitialize(pScreen, &darwinScreenFuncsRec)) {
        return FALSE;
    }

    // allocate private storage for this screen's hardware cursor info
    if (darwinCursorGeneration != serverGeneration) {
        if ((darwinCursorScreenIndex = AllocateScreenPrivateIndex()) < 0)
            return FALSE;
        darwinCursorGeneration = serverGeneration; 	
    }

    ScreenPriv = xcalloc( 1, sizeof(DarwinCursorScreenRec) );
    if (!ScreenPriv) return FALSE;

    pScreen->devPrivates[darwinCursorScreenIndex].ptr = (pointer) ScreenPriv;

    // check if a hardware cursor is supported
    if (!dfb.cursorShmem->hardwareCursorCapable) {
        ScreenPriv->canHWCursor = FALSE;
        ErrorF("Hardware cursor not supported.\n");
    } else {
        // we need to make sure that the hardware cursor really works
        ScreenPriv->canHWCursor = TRUE;
        kr = IOFBSetNewCursor(dfb.fbService, 0, 0, 0);
        if (kr != KERN_SUCCESS) {
            ErrorF("Could not set hardware cursor with kernel return 0x%x.\n", kr);
            ScreenPriv->canHWCursor = FALSE;
        }
        kr = IOFBSetCursorVisible(dfb.fbService, TRUE);
        if (kr != KERN_SUCCESS) {
            ErrorF("Couldn't set hardware cursor visible with kernel return 0x%x.\n", kr);
            ScreenPriv->canHWCursor = FALSE;
        }
        IOFBSetCursorVisible(dfb.fbService, FALSE);
    }

    ScreenPriv->cursorMode = 0;
    ScreenPriv->pInstalledMap = NULL;

    // override some screen procedures
    ScreenPriv->QueryBestSize = pScreen->QueryBestSize;
    pScreen->QueryBestSize = DarwinCursorQueryBestSize;
//    ScreenPriv->ConstrainCursor = pScreen->ConstrainCursor;
//    pScreen->ConstrainCursor = DarwinConstrainCursor;

    // initialize hardware cursor handling
    PointPriv = (miPointerScreenPtr)
                    pScreen->devPrivates[miPointerScreenIndex].ptr;

    ScreenPriv->spriteFuncs = PointPriv->spriteFuncs;
    PointPriv->spriteFuncs = &darwinSpriteFuncsRec; 

    /* Other routines that might be overridden */
/*
    CursorLimitsProcPtr		CursorLimits;
    RecolorCursorProcPtr	RecolorCursor;
*/

    return TRUE;
}
