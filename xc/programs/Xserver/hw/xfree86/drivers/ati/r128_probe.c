/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/ati/r128_probe.c,v 1.6 2000/12/13 02:45:00 tsi Exp $ */
/*
 * Copyright 1999, 2000 ATI Technologies Inc., Markham, Ontario,
 *                      Precision Insight, Inc., Cedar Park, Texas, and
 *                      VA Linux Systems Inc., Fremont, California.
 *
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation on the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NON-INFRINGEMENT.  IN NO EVENT SHALL ATI, PRECISION INSIGHT, VA LINUX
 * SYSTEMS AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * Authors:
 *   Rickard E. Faith <faith@valinux.com>
 *   Kevin E. Martin <martin@valinux.com>
 *
 * Modified by Marc Aurele La France <tsi@xfree86.org> for ATI driver merge.
 */

#include "atimodule.h"
#include "ativersion.h"

#include "r128_probe.h"
#include "r128_version.h"

#include "xf86PciInfo.h"

#include "xf86.h"
#include "xf86_ansic.h"
#include "xf86Resources.h"

SymTabRec R128Chipsets[] = {
    { PCI_CHIP_RAGE128RE, "ATI Rage 128 RE (PCI)" },
    { PCI_CHIP_RAGE128RF, "ATI Rage 128 RF (AGP)" },
    { PCI_CHIP_RAGE128RG, "ATI Rage 128 RG (AGP)" },
    { PCI_CHIP_RAGE128RK, "ATI Rage 128 RK (PCI)" },
    { PCI_CHIP_RAGE128RL, "ATI Rage 128 RL (AGP)" },
    { PCI_CHIP_RAGE128PF, "ATI Rage 128 Pro PF (AGP)" },
    { PCI_CHIP_RAGE128LE, "ATI Rage 128 Mobility LE (PCI)" },
    { PCI_CHIP_RAGE128LF, "ATI Rage 128 Mobility LF (AGP)" },
    { PCI_CHIP_RAGE128MF, "ATI Rage 128 Mobility MF (AGP)" },
    { PCI_CHIP_RAGE128ML, "ATI Rage 128 Mobility ML (AGP)" },
    { -1,                 NULL }
};

PciChipsets R128PciChipsets[] = {
    { PCI_CHIP_RAGE128RE, PCI_CHIP_RAGE128RE, RES_SHARED_VGA },
    { PCI_CHIP_RAGE128RF, PCI_CHIP_RAGE128RF, RES_SHARED_VGA },
    { PCI_CHIP_RAGE128RG, PCI_CHIP_RAGE128RG, RES_SHARED_VGA },
    { PCI_CHIP_RAGE128RK, PCI_CHIP_RAGE128RK, RES_SHARED_VGA },
    { PCI_CHIP_RAGE128RL, PCI_CHIP_RAGE128RL, RES_SHARED_VGA },
    { PCI_CHIP_RAGE128PF, PCI_CHIP_RAGE128PF, RES_SHARED_VGA },
    { PCI_CHIP_RAGE128LE, PCI_CHIP_RAGE128LE, RES_SHARED_VGA },
    { PCI_CHIP_RAGE128LF, PCI_CHIP_RAGE128LF, RES_SHARED_VGA },
    { PCI_CHIP_RAGE128MF, PCI_CHIP_RAGE128MF, RES_SHARED_VGA },
    { PCI_CHIP_RAGE128ML, PCI_CHIP_RAGE128ML, RES_SHARED_VGA },
    { -1,                 -1,                 RES_UNDEFINED }
};

/* Return the options for supported chipset 'n'; NULL otherwise */
OptionInfoPtr
R128AvailableOptions(int chipid, int busid)
{
    int i;

    /*
     * Return options defined in the r128 submodule which will have been
     * loaded by this point.
     */
    for (i = 0; R128PciChipsets[i].PCIid > 0; i++) {
	if (chipid == R128PciChipsets[i].PCIid)
	    return R128Options;
    }
    return NULL;
}

/* Return the string name for supported chipset 'n'; NULL otherwise. */
void
R128Identify(int flags)
{
    xf86PrintChipsets(R128_NAME,
		      "Driver for ATI Rage 128 chipsets",
		      R128Chipsets);
}

/* Return TRUE if chipset is present; FALSE otherwise. */
Bool
R128Probe(DriverPtr drv, int flags)
{
    int           numUsed;
    int           numDevSections, nATIGDev, nR128GDev;
    int           *usedChips;
    GDevPtr       *devSections, *ATIGDevs, *R128GDevs;
    EntityInfoPtr pEnt;
    Bool          foundScreen = FALSE;
    int           i;

    if (!xf86GetPciVideoInfo()) return FALSE;

    /* Collect unclaimed device sections for both driver names */
    nATIGDev = xf86MatchDevice(ATI_NAME, &ATIGDevs);
    nR128GDev = xf86MatchDevice(R128_NAME, &R128GDevs);

    if (!(numDevSections = nATIGDev + nR128GDev)) return FALSE;

    if (!ATIGDevs) {
	if (!(devSections = R128GDevs))
	    numDevSections = 1;
	else
	    numDevSections = nR128GDev;
    } if (!R128GDevs) {
	devSections = ATIGDevs;
	numDevSections = nATIGDev;
    } else {
	/* Combine into one list */
	devSections = xnfalloc((numDevSections + 1) * sizeof(GDevPtr));
	(void)memcpy(devSections,
		     ATIGDevs, nATIGDev * sizeof(GDevPtr));
	(void)memcpy(devSections + nATIGDev,
		     R128GDevs, nR128GDev * sizeof(GDevPtr));
	devSections[numDevSections] = NULL;
	xfree(ATIGDevs);
	xfree(R128GDevs);
    }

    numUsed = xf86MatchPciInstances(R128_NAME,
				    PCI_VENDOR_ATI,
				    R128Chipsets,
				    R128PciChipsets,
				    devSections,
				    numDevSections,
				    drv,
				    &usedChips);

    if (numUsed<=0) return FALSE;

    if (flags & PROBE_DETECT)
	foundScreen = TRUE;
    else for (i = 0; i < numUsed; i++) {
	pEnt = xf86GetEntityInfo(usedChips[i]);

	if (pEnt->active) {
	    ScrnInfoPtr pScrn    = xf86AllocateScreen(drv, 0);

#ifdef XFree86LOADER
	    if (!xf86LoadSubModule(pScrn, "r128")) {
		xf86Msg(X_ERROR,
		    R128_NAME ":  Failed to load \"r128\" module.\n");
		xf86DeleteScreen(pScrn->scrnIndex, 0);
		continue;
	    }

	    xf86LoaderReqSymLists(R128Symbols, NULL);

	    /* Workaround for possible loader bug */
#	    define R128PreInit     \
		(xf86PreInitProc*)    LoaderSymbol("R128PreInit")
#	    define R128ScreenInit  \
		(xf86ScreenInitProc*) LoaderSymbol("R128ScreenInit")
#	    define R128SwitchMode  \
		(xf86SwitchModeProc*) LoaderSymbol("R128SwitchMode")
#	    define R128AdjustFrame \
		(xf86AdjustFrameProc*)LoaderSymbol("R128AdjustFrame")
#	    define R128EnterVT     \
		(xf86EnterVTProc*)    LoaderSymbol("R128EnterVT")
#	    define R128LeaveVT     \
		(xf86LeaveVTProc*)    LoaderSymbol("R128LeaveVT")
#	    define R128FreeScreen  \
		(xf86FreeScreenProc*) LoaderSymbol("R128FreeScreen")
#	    define R128ValidMode   \
		(xf86ValidModeProc*)  LoaderSymbol("R128ValidMode")

#endif

	    pScrn->driverVersion = R128_VERSION_CURRENT;
	    pScrn->driverName    = R128_DRIVER_NAME;
	    pScrn->name          = R128_NAME;
	    pScrn->Probe         = R128Probe;
	    pScrn->PreInit       = R128PreInit;
	    pScrn->ScreenInit    = R128ScreenInit;
	    pScrn->SwitchMode    = R128SwitchMode;
	    pScrn->AdjustFrame   = R128AdjustFrame;
	    pScrn->EnterVT       = R128EnterVT;
	    pScrn->LeaveVT       = R128LeaveVT;
	    pScrn->FreeScreen    = R128FreeScreen;
	    pScrn->ValidMode     = R128ValidMode;

	    foundScreen          = TRUE;

	    xf86ConfigActivePciEntity(pScrn, usedChips[i], R128PciChipsets,
				      0, 0, 0, 0, 0);
	}
	xfree(pEnt);
    }

    xfree(usedChips);
    xfree(devSections);

    return foundScreen;
}
