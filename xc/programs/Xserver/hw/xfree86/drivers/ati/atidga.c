/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/ati/atidga.c,v 1.4 2000/10/11 22:52:55 tsi Exp $ */
/*
 * Copyright 2000 by Marc Aurele La France (TSI @ UQV), tsi@ualberta.ca
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting documentation, and
 * that the name of Marc Aurele La France not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  Marc Aurele La France makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as-is" without express or implied warranty.
 *
 * MARC AURELE LA FRANCE DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS.  IN NO
 * EVENT SHALL MARC AURELE LA FRANCE BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#include "ati.h"
#include "atiadjust.h"
#include "atichip.h"
#include "atidac.h"
#include "atidga.h"
#include "atiident.h"
#include "atimode.h"
#include "atistruct.h"

#include "dgaproc.h"

/*
 * ATIDGAOpenFramebuffer --
 *
 * This function returns various framebuffer attributes to a DGA client.
 */
static Bool
ATIDGAOpenFramebuffer
(
    ScrnInfoPtr   pScreenInfo,
    char          **DeviceName,
    unsigned char **ApertureBase,
    int           *ApertureSize,
    int           *ApertureOffset,
    int           *flags
)
{
    ATIPtr pATI = ATIPTR(pScreenInfo);

    *DeviceName = NULL;         /* No special device */
    *ApertureBase = (unsigned char *)(pATI->LinearBase);
    *ApertureSize = pScreenInfo->videoRam * 1024;
    *ApertureOffset = 0;        /* Always */
    *flags = 0;                 /* Root premissions OS-dependent */

    return TRUE;
}

static int
BitsSet
(
    unsigned long data
)
{
    unsigned long mask = 1;
    int           set  = 0;

    for (;  mask;  mask <<= 1)
        if (data & mask)
            set++;

    return set;
}

/*
 * ATIDGASetMode --
 *
 * This function sets a graphics mode for a DGA client.
 */
static Bool
ATIDGASetMode
(
    ScrnInfoPtr pScreenInfo,
    DGAModePtr  pDGAMode
)
{
    ATIPtr         pATI      = ATIPTR(pScreenInfo);
    DisplayModePtr pMode;
    int            scrnIndex = pScreenInfo->pScreen->myNum;
    int            frameX0, frameY0;

    if (pDGAMode)
    {
        pMode = pDGAMode->mode;
        pATI->depth = pDGAMode->depth;
        pATI->bitsPerPixel = pDGAMode->bitsPerPixel;
        pATI->displayWidth =
            pDGAMode->bytesPerScanline * 8 / pATI->bitsPerPixel;
        pATI->weight.red = BitsSet(pDGAMode->red_mask);
        pATI->weight.green = BitsSet(pDGAMode->green_mask);
        pATI->weight.blue = BitsSet(pDGAMode->blue_mask);
        frameX0 = frameY0 = 0;
        if (!pATI->currentMode)
            pATI->currentMode = pScreenInfo->currentMode;
    }
    else
    {
        if (!(pMode = pATI->currentMode))
            return TRUE;

        pATI->depth = pScreenInfo->depth;
        pATI->bitsPerPixel = pScreenInfo->bitsPerPixel;
        pATI->displayWidth = pScreenInfo->displayWidth;
        pATI->weight = pScreenInfo->weight;
        frameX0 = pScreenInfo->frameX0;
        frameY0 = pScreenInfo->frameY0;
    }

    pATI->XModifier = pATI->bitsPerPixel / UnitOf(pATI->bitsPerPixel);
    ATIAdjustPreInit(pATI);
    ATIModePreInit(pScreenInfo, pATI, &pATI->NewHW);

    if (!(*pScreenInfo->SwitchMode)(scrnIndex, pMode, 0))
        return FALSE;
    if (!pDGAMode)
        pATI->currentMode = NULL;
    (*pScreenInfo->AdjustFrame)(scrnIndex, frameX0, frameY0, 0);

    return TRUE;
}

/*
 * ATIDGASetViewport --
 *
 * This function sets the display start address for a DGA client.
 */
static void
ATIDGASetViewport
(
    ScrnInfoPtr pScreenInfo,
    int         x,
    int         y,
    int         flags
)
{
    (*pScreenInfo->AdjustFrame)(pScreenInfo->pScreen->myNum, x, y, flags);
}

/*
 * ATIDGAGetViewport --
 *
 * This function returns the current status of prior DGA requests to set the
 * adapter's display start address.
 */
static int
ATIDGAGetViewport
(
    ScrnInfoPtr pScreenInfo
)
{
    return 0;   /* There are never any pending requests */
}

static DGAFunctionRec ATIDGAFunctions =
{
    ATIDGAOpenFramebuffer,
    NULL,       /* CloseFramebuffer */
    ATIDGASetMode,
    ATIDGASetViewport,
    ATIDGAGetViewport,
    NULL,       /* Sync */
    NULL,       /* FillRect */
    NULL,       /* BlitRect */
    NULL,       /* BlitTransRect */
};

/*
 * ATIDGAAddModes --
 *
 * This function translates DisplayModeRec's into DGAModeRec's.
 */
static void
ATIDGAAddModes
(
    ScrnInfoPtr pScreenInfo,
    ATIPtr      pATI,
    int         depth,
    int         bitsPerPixel,
    int         redMask,
    int         greenMask,
    int         blueMask,
    int         visualClass
)
{
    DisplayModePtr pMode         = pScreenInfo->modes;
    DGAModePtr     pDGAMode;
    int            displayWidth  = pScreenInfo->displayWidth;
    int            videoBits     = pScreenInfo->videoRam * 1024 * 8;
    int            xViewportStep = 64 / UnitOf(bitsPerPixel);
    int            modePitch, bitsPerScanline, maxViewportY;

    if (bitsPerPixel != pScreenInfo->bitsPerPixel)
        displayWidth = 0;

    while (1)
    {
        /* Weed out multiscanned modes */
        if ((pMode->VScan <= 1) ||
            ((pMode->VScan == 2) && !(pMode->Flags & V_DBLSCAN)))
        {
            /*
             * For code simplicity, ensure DGA mode pitch is a multiple of 64
             * bytes.
             */
            if (!(modePitch = displayWidth))
            {
                modePitch = ((64 * 8) / UnitOf(bitsPerPixel)) - 1;
                modePitch = (pMode->HDisplay + modePitch) & ~modePitch;
            }

            /* Ensure the mode fits in video memory */
            if ((modePitch * bitsPerPixel * pMode->VDisplay) <= videoBits)
            {
                /* Stop generating modes on out-of-memory conditions */
                pDGAMode = xrealloc(pATI->pDGAMode,
                    (pATI->nDGAMode + 1) * SizeOf(DGAModeRec));
                if (!pDGAMode)
                    break;

                pATI->pDGAMode = pDGAMode;
                pDGAMode += pATI->nDGAMode;
                pATI->nDGAMode++;
                (void)memset(pDGAMode, 0, SizeOf(DGAModeRec));

                /* Fill in the mode structure */
                pDGAMode->mode = pMode;
                pDGAMode->flags = DGA_CONCURRENT_ACCESS;
                if (bitsPerPixel == pScreenInfo->bitsPerPixel)
                {
                    pDGAMode->flags |= DGA_PIXMAP_AVAILABLE;
                    pDGAMode->address = pATI->pMemory;

                    if (pATI->OptionAccel)
                        pDGAMode->flags &= ~DGA_CONCURRENT_ACCESS;
                }
                if ((pMode->Flags & V_DBLSCAN) || (pMode->VScan > 1))
                    pDGAMode->flags |= DGA_DOUBLESCAN;
                if (pMode->Flags & V_INTERLACE)
                    pDGAMode->flags |= DGA_INTERLACED;

                pDGAMode->byteOrder = pScreenInfo->imageByteOrder;
                pDGAMode->depth = depth;
                pDGAMode->bitsPerPixel = bitsPerPixel;
                pDGAMode->red_mask = redMask;
                pDGAMode->green_mask = greenMask;
                pDGAMode->blue_mask = blueMask;
                pDGAMode->visualClass = visualClass;

                pDGAMode->viewportWidth = pMode->HDisplay;
                pDGAMode->viewportHeight = pMode->VDisplay;
                pDGAMode->xViewportStep = xViewportStep;
                pDGAMode->yViewportStep = 1;

                bitsPerScanline = modePitch * bitsPerPixel;
                pDGAMode->bytesPerScanline = bitsPerScanline / 8;
                pDGAMode->imageWidth = pDGAMode->pixmapWidth = modePitch;
                pDGAMode->imageHeight = pDGAMode->pixmapHeight =
                    videoBits / bitsPerScanline;

                pDGAMode->maxViewportX =
                    pDGAMode->imageWidth - pDGAMode->viewportWidth;
                pDGAMode->maxViewportY =
                    pDGAMode->imageHeight - pDGAMode->viewportHeight;
                maxViewportY =
                    ((((pATI->AdjustMaxBase * 8) / bitsPerPixel) +
                      xViewportStep) / modePitch) - 1;
                if (maxViewportY < pDGAMode->maxViewportY)
                    pDGAMode->maxViewportY = maxViewportY;
            }
        }

        if ((pMode = pMode->next) == pScreenInfo->modes)
        {
            if (!displayWidth)
                break;

            displayWidth = 0;
        }
    }
}

/*
 * ATIDGAInit --
 *
 * This function initialises the driver's support for the DGA extension.
 */
Bool
ATIDGAInit
(
    ScrnInfoPtr pScreenInfo,
    ScreenPtr   pScreen,
    ATIPtr      pATI
)
{
    if (!pATI->nDGAMode)
    {

#ifndef AVOID_CPIO

        /*
         * Contrary to previous extension versions, DGA 2 does not support
         * banked framebuffers.  Also, disable DGA when non-DGA server modes
         * are planar.
         */
        if (pATI->BankInfo.BankSize || (pScreenInfo->depth <= 4))
            return FALSE;

#endif /* AVOID_CPIO */

        ATIDGAAddModes(pScreenInfo, pATI,
            8, 8, 0, 0, 0, PseudoColor);

        if ((pATI->Chip >= ATI_CHIP_264CT) &&
            (pATI->Chipset == ATI_CHIPSET_ATI))
        {
            ATIDGAAddModes(pScreenInfo, pATI, 15, 16,
                0x7C00U, 0x03E0U, 0x001FU, TrueColor);

            ATIDGAAddModes(pScreenInfo, pATI, 16, 16,
                0xF800U, 0x07E0U, 0x001FU, TrueColor);

            ATIDGAAddModes(pScreenInfo, pATI, 24, 24,
                0x00FF0000U, 0x0000FF00U, 0x000000FFU, TrueColor);

            ATIDGAAddModes(pScreenInfo, pATI, 24, 32,
                0x00FF0000U, 0x0000FF00U, 0x000000FFU, TrueColor);

            if (pATI->DAC != ATI_DAC_INTERNAL)      /* Not first revision */
            {
                ATIDGAAddModes(pScreenInfo, pATI, 15, 16,
                    0x7C00U, 0x03E0U, 0x001FU, DirectColor);

                ATIDGAAddModes(pScreenInfo, pATI, 16, 16,
                    0xF800U, 0x07E0U, 0x001FU, DirectColor);

                ATIDGAAddModes(pScreenInfo, pATI, 24, 24,
                    0x00FF0000U, 0x0000FF00U, 0x000000FFU, DirectColor);

                ATIDGAAddModes(pScreenInfo, pATI, 24, 32,
                    0x00FF0000U, 0x0000FF00U, 0x000000FFU, DirectColor);
            }
        }
    }

    return DGAInit(pScreen, &ATIDGAFunctions, pATI->pDGAMode, pATI->nDGAMode);
}
