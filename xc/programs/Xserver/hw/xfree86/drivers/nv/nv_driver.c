/* $XConsortium: nv_driver.c /main/3 1996/10/28 05:13:37 kaleb $ */
/*
 * Copyright 1996-1997  David J. McKay
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
 * DAVID J. MCKAY BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/* Hacked together from mga driver and 3.3.4 NVIDIA driver by Jarno Paananen
   <jpaana@s2.org> */

/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/nv/nv_driver.c,v 1.53 2000/12/14 16:33:09 eich Exp $ */

#include "nv_include.h"

/* Little hack to declare all the base pointers */
#define extern
#include "nvreg.h"
#undef extern
#include "nvvga.h"

#include "xf86int10.h"
#ifdef RENDER
#include "picturestr.h"
#endif

/*
 * Forward definitions for the functions that make up the driver.
 */
/* Mandatory functions */
static OptionInfoPtr NVAvailableOptions(int chipid, int busid);
static void    NVIdentify(int flags);
static Bool    NVProbe(DriverPtr drv, int flags);
static Bool    NVPreInit(ScrnInfoPtr pScrn, int flags);
static Bool    NVScreenInit(int Index, ScreenPtr pScreen, int argc,
                            char **argv);
static Bool    NVEnterVT(int scrnIndex, int flags);
static Bool    NVEnterVTFBDev(int scrnIndex, int flags);
static void    NVLeaveVT(int scrnIndex, int flags);
static Bool    NVCloseScreen(int scrnIndex, ScreenPtr pScreen);
static Bool    NVSaveScreen(ScreenPtr pScreen, int mode);

/* Optional functions */
static void    NVFreeScreen(int scrnIndex, int flags);
static int     NVValidMode(int scrnIndex, DisplayModePtr mode, Bool verbose,
                           int flags);

/* Internally used functions */

static Bool	NVMapMem(ScrnInfoPtr pScrn);
static Bool	NVMapMemFBDev(ScrnInfoPtr pScrn);
static Bool	NVUnmapMem(ScrnInfoPtr pScrn);
static void	NVSave(ScrnInfoPtr pScrn);
static void	NVRestore(ScrnInfoPtr pScrn);
static Bool	NVModeInit(ScrnInfoPtr pScrn, DisplayModePtr mode);


/*
 * This contains the functions needed by the server after loading the
 * driver module.  It must be supplied, and gets added the driver list by
 * the Module Setup funtion in the dynamic case.  In the static case a
 * reference to this is compiled in, and this requires that the name of
 * this DriverRec be an upper-case version of the driver name.
 */

DriverRec NV = {
        VERSION,
	"nv",
#if 0
        "accelerated driver for NVIDIA RIVA 128, TNT and TNT2 cards",
#endif
        NVIdentify,
        NVProbe,
	NVAvailableOptions,
        NULL,
        0
};

/* Supported chipsets */
static SymTabRec NVChipsets[] = {
    { NV_CHIP_RIVA128,    "RIVA128" },
    { NV_CHIP_TNT,        "RIVATNT" },
    { NV_CHIP_TNT2,       "RIVATNT2" },
    { NV_CHIP_UTNT2,      "RIVATNT2 (Ultra)" },
    { NV_CHIP_VTNT2,      "RIVATNT2 (Vanta)" },
    { NV_CHIP_UVTNT2,     "RIVATNT2 M64" },
    { NV_CHIP_ITNT2,      "RIVATNT2 (Integrated)" },
    { NV_CHIP_GEFORCE256, "GeForce 256" },
    { NV_CHIP_GEFORCEDDR, "GeForce DDR" },
    { NV_CHIP_QUADRO,     "Quadro" },
    { NV_CHIP_GEFORCE2GTS,  "GeForce2 GTS"},
    { NV_CHIP_GEFORCE2GTS_1,"GeForce2 GTS (rev 1)"},
    { NV_CHIP_GEFORCE2ULTRA,"GeForce2 ultra"},
    { NV_CHIP_QUADRO2PRO,   "Quadro 2 Pro"},
    { NV_CHIP_GEFORCE2MX,   "GeForce2 MX"},
    { NV_CHIP_GEFORCE2MXDDR, "GeForce2 MX DDR"},
    { NV_CHIP_QUADRO2MXR,   "Quadro 2 MXR"},
    { NV_CHIP_GEFORCE2GO,   "GeForce 2 Go"},
    {-1,                        NULL }
};

static PciChipsets NVPciChipsets[] = {
    { NV_CHIP_RIVA128,          NV_CHIP_RIVA128,        RES_SHARED_VGA },
    { NV_CHIP_TNT,              NV_CHIP_TNT,            RES_SHARED_VGA },
    { NV_CHIP_TNT2,             NV_CHIP_TNT2,           RES_SHARED_VGA },
    { NV_CHIP_UTNT2,            NV_CHIP_UTNT2,          RES_SHARED_VGA },
    { NV_CHIP_VTNT2,            NV_CHIP_VTNT2,          RES_SHARED_VGA },
    { NV_CHIP_UVTNT2,           NV_CHIP_UVTNT2,         RES_SHARED_VGA },
    { NV_CHIP_ITNT2,            NV_CHIP_ITNT2,          RES_SHARED_VGA },
    { NV_CHIP_GEFORCE256,       NV_CHIP_GEFORCE256,     RES_SHARED_VGA },
    { NV_CHIP_GEFORCEDDR,       NV_CHIP_GEFORCEDDR,     RES_SHARED_VGA },
    { NV_CHIP_QUADRO,           NV_CHIP_QUADRO,         RES_SHARED_VGA },
    { NV_CHIP_GEFORCE2GTS,      NV_CHIP_GEFORCE2GTS,    RES_SHARED_VGA },
    { NV_CHIP_GEFORCE2GTS_1,    NV_CHIP_GEFORCE2GTS_1,  RES_SHARED_VGA },
    { NV_CHIP_GEFORCE2ULTRA,    NV_CHIP_GEFORCE2ULTRA,  RES_SHARED_VGA },
    { NV_CHIP_QUADRO2PRO,       NV_CHIP_QUADRO2PRO,     RES_SHARED_VGA },
    { NV_CHIP_GEFORCE2MX,       NV_CHIP_GEFORCE2MX,     RES_SHARED_VGA },
    { NV_CHIP_GEFORCE2MXDDR,    NV_CHIP_GEFORCE2MXDDR,  RES_SHARED_VGA },
    { NV_CHIP_QUADRO2MXR,       NV_CHIP_QUADRO2MXR,     RES_SHARED_VGA },
    { NV_CHIP_GEFORCE2GO,       NV_CHIP_GEFORCE2GO,     RES_SHARED_VGA },
    { -1,                       -1,                     RES_UNDEFINED  }
};

/*
 * List of symbols from other modules that this module references.  This
 * list is used to tell the loader that it is OK for symbols here to be
 * unresolved providing that it hasn't been told that they haven't been
 * told that they are essential via a call to xf86LoaderReqSymbols() or
 * xf86LoaderReqSymLists().  The purpose is this is to avoid warnings about
 * unresolved symbols that are not required.
 */

static const char *vgahwSymbols[] = {
    "vgaHWGetHWRec",
    "vgaHWUnlock",
    "vgaHWInit",
    "vgaHWProtect",
    "vgaHWGetIOBase",
    "vgaHWMapMem",
    "vgaHWLock",
    "vgaHWFreeHWRec",
    "vgaHWSaveScreen",
    "vgaHWddc1SetSpeed",
#ifdef DPMSExtension
    "vgaHWDPMSSet",
#endif
    NULL
};

#ifndef NV_USE_FB
static const char *cfbSymbols[] = {
    "cfbScreenInit",
    "cfb16ScreenInit",
    "cfb24ScreenInit",
    "cfb32ScreenInit",
    "cfbBresS",
    "cfb16BresS",
    "cfb32BresS",
    NULL
};
#else
static const char *fbSymbols[] = {
    "fbScreenInit",
    "fbBres",
#ifdef RENDER
    "fbPictureInit",
#endif
    NULL
};
#endif

static const char *xaaSymbols[] = {
    "XAADestroyInfoRec",
    "XAACreateInfoRec",
    "XAAInit",
    NULL
};

static const char *ramdacSymbols[] = {
    "xf86InitCursor",
    "xf86CreateCursorInfoRec",
    "xf86DestroyCursorInfoRec",
    NULL
};

#define NVuseI2C 1

static const char *ddcSymbols[] = {
    "xf86PrintEDID",
    "xf86DoEDID_DDC1",
#if NVuseI2C
    "xf86DoEDID_DDC2",
#endif
    NULL
};

static const char *vbeSymbols[] = {
    "VBEInit",
    "vbeDoEDID",
    "vbeFree",
    NULL
};

static const char *i2cSymbols[] = {
    "xf86CreateI2CBusRec",
    "xf86I2CBusInit",
    NULL
};

static const char *shadowSymbols[] = {
    "ShadowFBInit",
    NULL
};

static const char *fbdevHWSymbols[] = {
    "fbdevHWInit",
    "fbdevHWUseBuildinMode",

    "fbdevHWGetDepth",
    "fbdevHWGetVidmem",

    /* colormap */
    "fbdevHWLoadPalette",

    /* ScrnInfo hooks */
    "fbdevHWSwitchMode",
    "fbdevHWAdjustFrame",
    "fbdevHWEnterVT",
    "fbdevHWLeaveVT",
    "fbdevHWValidMode",
    "fbdevHWRestore",
    "fbdevHWModeInit",
    "fbdevHWSave",

    "fbdevHWUnmapMMIO",
    "fbdevHWUnmapVidmem",
    "fbdevHWMapMMIO",
    "fbdevHWMapVidmem",

    NULL
};

static const char *int10Symbols[] = {
    "xf86InitInt10",
    "xf86FreeInt10",
    NULL
};


#ifdef XFree86LOADER

static MODULESETUPPROTO(nvSetup);

static XF86ModuleVersionInfo nvVersRec =
{
    "nv",
    MODULEVENDORSTRING,
    MODINFOSTRING1,
    MODINFOSTRING2,
    XF86_VERSION_CURRENT,
    NV_MAJOR_VERSION, NV_MINOR_VERSION, NV_PATCHLEVEL,
    ABI_CLASS_VIDEODRV,                     /* This is a video driver */
    ABI_VIDEODRV_VERSION,
    MOD_CLASS_VIDEODRV,
    {0,0,0,0}
};

XF86ModuleData nvModuleData = { &nvVersRec, nvSetup, NULL };
#endif


typedef enum {
    OPTION_SW_CURSOR,
    OPTION_HW_CURSOR,
    OPTION_NOACCEL,
    OPTION_SHOWCACHE,
    OPTION_SHADOW_FB,
    OPTION_FBDEV,
    OPTION_ROTATE
} NVOpts;


static OptionInfoRec NVOptions[] = {
    { OPTION_SW_CURSOR,         "SWcursor",     OPTV_BOOLEAN,   {0}, FALSE },
    { OPTION_HW_CURSOR,         "HWcursor",     OPTV_BOOLEAN,   {0}, FALSE },
    { OPTION_NOACCEL,           "NoAccel",      OPTV_BOOLEAN,   {0}, FALSE },
    { OPTION_SHOWCACHE,         "ShowCache",    OPTV_BOOLEAN,   {0}, FALSE },
    { OPTION_SHADOW_FB,         "ShadowFB",     OPTV_BOOLEAN,   {0}, FALSE },
    { OPTION_FBDEV,             "UseFBDev",     OPTV_BOOLEAN,   {0}, FALSE },
    { OPTION_ROTATE,		"Rotate",	OPTV_ANYSTR,	{0}, FALSE },
    { -1,                       NULL,           OPTV_NONE,      {0}, FALSE }
};

/*
 * This is intentionally screen-independent.  It indicates the binding
 * choice made in the first PreInit.
 */
static int pix24bpp = 0;

/* 
 * ramdac info structure initialization
 */
static NVRamdacRec DacInit = {
        FALSE, 0, 0, 0, 0, NULL, NULL, NULL, NULL, NULL, NULL,
        0, NULL, NULL, NULL, NULL, NULL
}; 



static Bool
NVGetRec(ScrnInfoPtr pScrn)
{
    DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO, "NVGetRec\n"));
    /*
     * Allocate an NVRec, and hook it into pScrn->driverPrivate.
     * pScrn->driverPrivate is initialised to NULL, so we can check if
     * the allocation has already been done.
     */
    if (pScrn->driverPrivate != NULL)
        return TRUE;

    pScrn->driverPrivate = xnfcalloc(sizeof(NVRec), 1);
    /* Initialise it */

    NVPTR(pScrn)->Dac = DacInit;
    return TRUE;
}

static void
NVFreeRec(ScrnInfoPtr pScrn)
{
    DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO, "NVFreeRec\n"));
    
    if (pScrn->driverPrivate == NULL)
        return;
    xfree(pScrn->driverPrivate);
    pScrn->driverPrivate = NULL;
}


#ifdef XFree86LOADER

static pointer
nvSetup(pointer module, pointer opts, int *errmaj, int *errmin)
{
    static Bool setupDone = FALSE;

    /* This module should be loaded only once, but check to be sure. */

    if (!setupDone) {
        setupDone = TRUE;
        xf86AddDriver(&NV, module, 0);

        /*
         * Modules that this driver always requires may be loaded here
         * by calling LoadSubModule().
         */

        /*
         * Tell the loader about symbols from other modules that this module
         * might refer to.
         */
        LoaderRefSymLists(vgahwSymbols, xaaSymbols,
#ifndef NV_USE_FB
                          cfbSymbols,
#else
                          fbSymbols,
#endif                          
                          ramdacSymbols, shadowSymbols,
                          i2cSymbols, ddcSymbols, vbeSymbols,
                          fbdevHWSymbols, int10Symbols, NULL);

        /*
         * The return value must be non-NULL on success even though there
         * is no TearDownProc.
         */
        return (pointer)1;
    } else {
        if (errmaj) *errmaj = LDR_ONCEONLY;
        return NULL;
    }
}


#endif /* XFree86LOADER */

static
OptionInfoPtr
NVAvailableOptions(int chipid, int busid)
{
    return NVOptions;
}

/* Mandatory */
static void
NVIdentify(int flags)
{
    xf86PrintChipsets(NV_NAME, "driver for NVIDIA chipsets", NVChipsets);
}


/* Mandatory */
static Bool
NVProbe(DriverPtr drv, int flags)
{
    int i;
    GDevPtr *devSections;
    int *usedChips;
    int numDevSections;
    int numUsed;
    Bool foundScreen = FALSE;

    /*
     * The aim here is to find all cards that this driver can handle,
     * and for the ones not already claimed by another driver, claim the
     * slot, and allocate a ScrnInfoRec.
     *
     * This should be a minimal probe, and it should under no circumstances
     * change the state of the hardware.  Because a device is found, don't
     * assume that it will be used.  Don't do any initialisations other than
     * the required ScrnInfoRec initialisations.  Don't allocate any new
     * data structures.
     */

    /*
     * Check if there has been a chipset override in the config file.
     * For this we must find out if there is an active device section which
     * is relevant, i.e., which has no driver specified or has THIS driver
     * specified.
     */

    if ((numDevSections = xf86MatchDevice(NV_DRIVER_NAME,
                                          &devSections)) <= 0) {
        /*
         * There's no matching device section in the config file, so quit
         * now.
         */
        return FALSE;
    }

    /*
     * We need to probe the hardware first.  We then need to see how this
     * fits in with what is given in the config file, and allow the config
     * file info to override any contradictions.
     */

    /*
     * All of the cards this driver supports are PCI, so the "probing" just
     * amounts to checking the PCI data that the server has already collected.
     */
    if (xf86GetPciVideoInfo() == NULL) {
        /*
         * We won't let anything in the config file override finding no
         * PCI video cards at all.  This seems reasonable now, but we'll see.
         */
        return FALSE;
    }

    /* This should match both vendors, PCI_VENDOR_NVIDIA_SGS and
       PCI_VENDOR_NVIDIA, see above */
    numUsed = xf86MatchPciInstances(NV_NAME, 0, NVChipsets, NVPciChipsets,
                                    devSections, numDevSections, drv,
                                    &usedChips);
                        
    /* Free it since we don't need that list after this */
    xfree(devSections);
    if (numUsed <= 0)
        return FALSE;

    if (flags & PROBE_DETECT)
	foundScreen = TRUE;
    else for (i = 0; i < numUsed; i++) {
        ScrnInfoPtr pScrn = NULL;
        
        /* Allocate a ScrnInfoRec and claim the slot */
        if ((pScrn = xf86ConfigPciEntity(pScrn, 0,usedChips[i],
					       NVPciChipsets, NULL, NULL, NULL,
					       NULL, NULL))) {
        
	    /* Fill in what we can of the ScrnInfoRec */
	    pScrn->driverVersion    = VERSION;
	    pScrn->driverName       = NV_DRIVER_NAME;
	    pScrn->name             = NV_NAME;
	    pScrn->Probe            = NVProbe;
	    pScrn->PreInit          = NVPreInit;
	    pScrn->ScreenInit       = NVScreenInit;
	    pScrn->SwitchMode       = NVSwitchMode;
	    pScrn->AdjustFrame      = NVAdjustFrame;
	    pScrn->EnterVT          = NVEnterVT;
	    pScrn->LeaveVT          = NVLeaveVT;
	    pScrn->FreeScreen       = NVFreeScreen;
	    pScrn->ValidMode        = NVValidMode;
	    foundScreen = TRUE;
	}    
    }
    xfree(usedChips);
    return foundScreen;
}

/* Usually mandatory */
Bool
NVSwitchMode(int scrnIndex, DisplayModePtr mode, int flags)
{
    DEBUG(xf86DrvMsg(scrnIndex, X_INFO, "NVSwitchMode\n"));
    return NVModeInit(xf86Screens[scrnIndex], mode);
}

/*
 * This function is used to initialize the Start Address - the first
 * displayed location in the video memory.
 */
/* Usually mandatory */
void 
NVAdjustFrame(int scrnIndex, int x, int y, int flags)
{
    ScrnInfoPtr pScrn = xf86Screens[scrnIndex];
    int startAddr;
    NVPtr pNv = NVPTR(pScrn);
    NVFBLayout *pLayout = &pNv->CurrentLayout;

    DEBUG(xf86DrvMsg(scrnIndex, X_INFO, "NVAdjustFrame\n"));

    if(pNv->ShowCache && y && pScrn->vtSema) 
	y += pScrn->virtualY - 1;	

    startAddr = (((y*pLayout->displayWidth)+x)*(pLayout->bitsPerPixel/8));
    pNv->riva.SetStartAddress(&pNv->riva, startAddr);
}


/*
 * This is called when VT switching back to the X server.  Its job is
 * to reinitialise the video mode.
 *
 * We may wish to unmap video/MMIO memory too.
 */

/* Mandatory */
static Bool
NVEnterVT(int scrnIndex, int flags)
{
    ScrnInfoPtr pScrn = xf86Screens[scrnIndex];
    NVPtr pNv = NVPTR(pScrn);
    vgaHWPtr hwp = VGAHWPTR(pScrn);

    DEBUG(xf86DrvMsg(scrnIndex, X_INFO, "NVEnterVT\n"));

    vgaHWUnlock(hwp);
    pNv->riva.LockUnlock(&pNv->riva, 0);
    if (!NVModeInit(pScrn, pScrn->currentMode))
        return FALSE;
    NVAdjustFrame(scrnIndex, pScrn->frameX0, pScrn->frameY0, 0);
    return TRUE;
}

static Bool
NVEnterVTFBDev(int scrnIndex, int flags)
{
    DEBUG(xf86DrvMsg(scrnIndex, X_INFO, "NVEnterVTFBDev\n"));

    fbdevHWEnterVT(scrnIndex,flags);
    return TRUE;
}

/*
 * This is called when VT switching away from the X server.  Its job is
 * to restore the previous (text) mode.
 *
 * We may wish to remap video/MMIO memory too.
 */

/* Mandatory */
static void
NVLeaveVT(int scrnIndex, int flags)
{
    ScrnInfoPtr pScrn = xf86Screens[scrnIndex];
    NVPtr pNv = NVPTR(pScrn);
    vgaHWPtr hwp = VGAHWPTR(pScrn);

    DEBUG(xf86DrvMsg(scrnIndex, X_INFO, "NVLeaveVT\n"));

    NVRestore(pScrn);
    pNv->riva.LockUnlock(&pNv->riva, 1);
    vgaHWLock(hwp);
}

/*
 * This is called at the end of each server generation.  It restores the
 * original (text) mode.  It should also unmap the video memory, and free
 * any per-generation data allocated by the driver.  It should finish
 * by unwrapping and calling the saved CloseScreen function.
 */

/* Mandatory */
static Bool
NVCloseScreen(int scrnIndex, ScreenPtr pScreen)
{
    ScrnInfoPtr pScrn = xf86Screens[scrnIndex];
    vgaHWPtr hwp = VGAHWPTR(pScrn);
    NVPtr pNv = NVPTR(pScrn);

    DEBUG(xf86DrvMsg(scrnIndex, X_INFO, "NVCloseScreen\n"));

    if (pScrn->vtSema) {
        NVRestore(pScrn);
        pNv->riva.LockUnlock(&pNv->riva, 1);
        vgaHWLock(hwp);
    }

    NVUnmapMem(pScrn);
    vgaHWUnmapMem(pScrn);
    if (pNv->AccelInfoRec)
        XAADestroyInfoRec(pNv->AccelInfoRec);
    if (pNv->CursorInfoRec)
        xf86DestroyCursorInfoRec(pNv->CursorInfoRec);
    if (pNv->ShadowPtr)
        xfree(pNv->ShadowPtr);
    if (pNv->DGAModes)
        xfree(pNv->DGAModes);
    if ( pNv->expandBuffer )
        xfree(pNv->expandBuffer);

    pScrn->vtSema = FALSE;
    pScreen->CloseScreen = pNv->CloseScreen;
    return (*pScreen->CloseScreen)(scrnIndex, pScreen);
}

/* Free up any persistent data structures */

/* Optional */
static void
NVFreeScreen(int scrnIndex, int flags)
{
    DEBUG(xf86DrvMsg(scrnIndex, X_INFO, "NVFreeScreen\n"));
    /*
     * This only gets called when a screen is being deleted.  It does not
     * get called routinely at the end of a server generation.
     */
    if (xf86LoaderCheckSymbol("vgaHWFreeHWRec"))
	vgaHWFreeHWRec(xf86Screens[scrnIndex]);
    NVFreeRec(xf86Screens[scrnIndex]);
}


/* Checks if a mode is suitable for the selected chipset. */

/* Optional */
static int
NVValidMode(int scrnIndex, DisplayModePtr mode, Bool verbose, int flags)
{
    DEBUG(xf86DrvMsg(scrnIndex, X_INFO, "NVValidMode\n"));
    /* HACK HACK HACK */
    return (MODE_OK);
}

static xf86MonPtr
nvDoDDC2(ScrnInfoPtr pScrn)
{
    NVPtr pNv = NVPTR(pScrn);
    xf86MonPtr MonInfo = NULL;

    if (!pNv->i2cInit) return NULL;

    /* - DDC can use I2C bus */
    /* Load I2C if we have the code to use it */
    if ( xf86LoadSubModule(pScrn, "i2c") ) {
        xf86LoaderReqSymLists(i2cSymbols,NULL);
        if (pNv->i2cInit(pScrn)) {
	    DEBUG(ErrorF("I2C initialized on %p\n",pNv->I2C));
	    if ((MonInfo = xf86DoEDID_DDC2(pScrn->scrnIndex,pNv->I2C))) {  
	        xf86DrvMsg(pScrn->scrnIndex, X_INFO, "DDC Monitor info: %p\n",
			   MonInfo);
		xf86PrintEDID( MonInfo );
		xf86DrvMsg(pScrn->scrnIndex, X_INFO, "end of DDC Monitor "
			   "info\n\n");
		xf86SetDDCproperties(pScrn,MonInfo);
	    }
	}
    }
    return MonInfo;
}

static xf86MonPtr
nvDoDDC1(ScrnInfoPtr pScrn)
{
    NVPtr pNv = NVPTR(pScrn);
    xf86MonPtr MonInfo = NULL;

    if (!pNv->ddc1Read || !pNv->DDC1SetSpeed) return NULL;
    if (!pNv->Primary 
	&& (pNv->DDC1SetSpeed == vgaHWddc1SetSpeed)) return NULL;

    if ((MonInfo = xf86DoEDID_DDC1(pScrn->scrnIndex, pNv->DDC1SetSpeed,
				  pNv->ddc1Read ))) {
        xf86DrvMsg(pScrn->scrnIndex, X_INFO, "DDC Monitor info: %p\n",
		   MonInfo);
	xf86PrintEDID( MonInfo );
	xf86DrvMsg(pScrn->scrnIndex, X_INFO, "end of DDC Monitor info\n\n");
	xf86SetDDCproperties(pScrn,MonInfo);
    }
    return MonInfo;
}
 
static xf86MonPtr
nvDoDDCVBE(ScrnInfoPtr pScrn)
{
    NVPtr pNv = NVPTR(pScrn);
    xf86MonPtr MonInfo = NULL;
    vbeInfoPtr pVbe;

    if (xf86LoadSubModule(pScrn, "vbe")) {
        xf86LoaderReqSymLists(vbeSymbols,NULL);
	pVbe = VBEInit(pNv->pInt,pNv->pEnt->index);
	if (pVbe) {
	    if ((MonInfo = vbeDoEDID(pVbe,NULL))) {
	        xf86DrvMsg(pScrn->scrnIndex, X_INFO, "DDC Monitor info: %p\n",
 			   MonInfo);
 		xf86PrintEDID( MonInfo );       
 		xf86DrvMsg(pScrn->scrnIndex, X_INFO, "end of DDC Monitor info\n\n");
 		xf86SetDDCproperties(pScrn,MonInfo);
 	    }
 	    vbeFree(pVbe);
 	} else 
 	  xf86FreeInt10(pNv->pInt);
 	pNv->pInt = NULL;
     }
     return MonInfo;
}
 

/* Internally used */
static xf86MonPtr
NVdoDDC(ScrnInfoPtr pScrn)
{
    vgaHWPtr hwp;
    NVPtr pNv;
    NVRamdacPtr NVdac;
    xf86MonPtr MonInfo = NULL;

    hwp = VGAHWPTR(pScrn);
    pNv = NVPTR(pScrn);
    NVdac = &pNv->Dac;

    /* Load DDC if we have the code to use it */

    if (!xf86LoadSubModule(pScrn, "ddc")) return NULL;
    
    xf86LoaderReqSymLists(ddcSymbols, NULL);

    /*    if ((MonInfo = nvDoDDCVBE(pScrn))) return MonInfo;      */

    /* Enable access to extended registers */
    vgaHWUnlock(hwp);
    pNv->riva.LockUnlock(&pNv->riva, 0);
    /* Save the current state */
    NVSave(pScrn);

    if ((MonInfo = nvDoDDC2(pScrn))) goto done;
#if 0 /* disable for now - causes problems on AXP */
    if ((MonInfo = nvDoDDC1(pScrn))) goto done;
#endif

 done:
    /* Restore previous state */
    NVRestore(pScrn);
    pNv->riva.LockUnlock(&pNv->riva, 1);
    vgaHWLock(hwp);

    return MonInfo;
}

static void
nvProbeDDC(ScrnInfoPtr pScrn, int index)
{
    vbeInfoPtr pVbe;

    if (xf86LoadSubModule(pScrn, "vbe")) {
        pVbe = VBEInit(NULL,index);
        ConfiguredMonitor = vbeDoEDID(pVbe, NULL);
    }
}

/* Mandatory */
Bool
NVPreInit(ScrnInfoPtr pScrn, int flags)
{
    NVPtr pNv;
    MessageType from;
    int i;
    int bytesPerPixel;
    ClockRangePtr clockRanges;
    char *mod = NULL;
    const char *reqSym = NULL;
    const char *s;

    if (flags & PROBE_DETECT) {
        nvProbeDDC( pScrn, xf86GetEntityInfo(pScrn->entityList[0])->index );
	return TRUE;
    }


    DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO, "NVPreInit\n"));
    /*
     * Note: This function is only called once at server startup, and
     * not at the start of each server generation.  This means that
     * only things that are persistent across server generations can
     * be initialised here.  xf86Screens[] is (pScrn is a pointer to one
     * of these).  Privates allocated using xf86AllocateScrnInfoPrivateIndex()  
     * are too, and should be used for data that must persist across
     * server generations.
     *
     * Per-generation data should be allocated with
     * AllocateScreenPrivateIndex() from the ScreenInit() function.
     */

    /* Check the number of entities, and fail if it isn't one. */
    if (pScrn->numEntities != 1)
	return FALSE;

    /* Allocate the NVRec driverPrivate */
    if (!NVGetRec(pScrn)) {
	return FALSE;
    }
    pNv = NVPTR(pScrn);

    /* Get the entity, and make sure it is PCI. */
    pNv->pEnt = xf86GetEntityInfo(pScrn->entityList[0]);
    if (pNv->pEnt->location.type != BUS_PCI)
	return FALSE;
 
    /* Find the PCI info for this screen */
    pNv->PciInfo = xf86GetPciInfoForEntity(pNv->pEnt->index);
    pNv->PciTag = pciTag(pNv->PciInfo->bus, pNv->PciInfo->device,
			  pNv->PciInfo->func);

    pNv->Primary = xf86IsPrimaryPci(pNv->PciInfo);

    /* Initialize the card through int10 interface if needed */
#if 0
     if ( !pNv->Primary &&)
#endif
       if (xf86LoadSubModule(pScrn, "int10")){
 
 	xf86LoaderReqSymLists(int10Symbols, NULL);
 
         xf86DrvMsg(pScrn->scrnIndex, X_INFO, "Initializing int10\n");
         pNv->pInt = xf86InitInt10(pNv->pEnt->index);
     }
   
    {
        resRange vgaio[] =      { {ResShrIoBlock,0x3B0,0x3BB},
                                  {ResShrIoBlock,0x3C0,0x3DF},
                                  _END };
 	resRange vgamem[] =	{ {ResShrMemBlock,0xA0000,0xAFFFF},
 				  {ResShrMemBlock,0xB8000,0xBFFFF},
                                  {ResShrMemBlock,0xB0000,0xB7FFF},
 				  _END };
        xf86SetOperatingState(vgaio, pNv->pEnt->index, ResUnusedOpr);
 	xf86SetOperatingState(vgamem, pNv->pEnt->index, ResDisableOpr);
    }

    /* Set pScrn->monitor */
    pScrn->monitor = pScrn->confScreen->monitor;

    /*
     * Set the Chipset and ChipRev, allowing config file entries to
     * override.
     */
    if (pNv->pEnt->device->chipset && *pNv->pEnt->device->chipset) {
	pScrn->chipset = pNv->pEnt->device->chipset;
        pNv->Chipset = xf86StringToToken(NVChipsets, pScrn->chipset);
        from = X_CONFIG;
    } else if (pNv->pEnt->device->chipID >= 0) {
	pNv->Chipset = pNv->pEnt->device->chipID;
	pScrn->chipset = (char *)xf86TokenToString(NVChipsets, pNv->Chipset);
	from = X_CONFIG;
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "ChipID override: 0x%04X\n",
		   pNv->Chipset);
    } else {
	from = X_PROBED;
	pNv->Chipset = (pNv->PciInfo->vendor << 16) |pNv->PciInfo->chipType;
	pScrn->chipset = (char *)xf86TokenToString(NVChipsets, pNv->Chipset);
    }
    if (pNv->pEnt->device->chipRev >= 0) {
	pNv->ChipRev = pNv->pEnt->device->chipRev;
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "ChipRev override: %d\n",
		   pNv->ChipRev);
    } else {
	pNv->ChipRev = pNv->PciInfo->chipRev;
    }

    /*
     * This shouldn't happen because such problems should be caught in
     * NVProbe(), but check it just in case.
     */
    if (pScrn->chipset == NULL) {
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		   "ChipID 0x%04X is not recognised\n", pNv->Chipset);
	return FALSE;
    }
    if (pNv->Chipset < 0) {
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		   "Chipset \"%s\" is not recognised\n", pScrn->chipset);
	return FALSE;
    }

    xf86DrvMsg(pScrn->scrnIndex, from, "Chipset: \"%s\"\n", pScrn->chipset);


    /*
     * The first thing we should figure out is the depth, bpp, etc.
     * Our default depth is 8, so pass it to the helper function.
     */

    if (!xf86SetDepthBpp(pScrn, 8, 8, 8, Support32bppFb)) {
	return FALSE;
    } else {
	/* Check that the returned depth is one we support */
	switch (pScrn->depth) {
            case 8:
            case 15:
            case 24:
                /* OK */
                break;
            case 16:
                if(pNv->Chipset == NV_CHIP_RIVA128) {
                    xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
                        "The Riva 128 chipset does not support depth 16.  "
			"Using depth 15 instead\n");
                    pScrn->depth = 15;
                }
                break;
            default:
                xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
                    "Given depth (%d) is not supported by this driver\n",
                    pScrn->depth);
                return FALSE;
	}
    }
    xf86PrintDepthBpp(pScrn);

    /* Get the depth24 pixmap format */
    if (pScrn->depth == 24 && pix24bpp == 0)
	pix24bpp = xf86GetBppFromDepth(pScrn, 24);

    /*
     * This must happen after pScrn->display has been set because
     * xf86SetWeight references it.
     */
    if (pScrn->depth > 8) {
	/* The defaults are OK for us */
	rgb zeros = {0, 0, 0};

	if (!xf86SetWeight(pScrn, zeros, zeros))
	    return FALSE;
    }

    if (!xf86SetDefaultVisual(pScrn, -1)) {
	return FALSE;
    } else {
	/* We don't currently support DirectColor at > 8bpp */
	if (pScrn->depth > 8 && (pScrn->defaultVisual != TrueColor)) {
	    xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "Given default visual"
		       " (%s) is not supported at depth %d\n",
		       xf86GetVisualName(pScrn->defaultVisual), pScrn->depth);
	    return FALSE;
	}
    }

    bytesPerPixel = pScrn->bitsPerPixel / 8;

    /* The vgahw module should be loaded here when needed */
    if (!xf86LoadSubModule(pScrn, "vgahw"))
	return FALSE;

    xf86LoaderReqSymLists(vgahwSymbols, NULL);

    /*
     * Allocate a vgaHWRec
     */
    if (!vgaHWGetHWRec(pScrn))
	return FALSE;

    /* We use a programamble clock */
    pScrn->progClock = TRUE;

    /* Collect all of the relevant option flags (fill in pScrn->options) */
    xf86CollectOptions(pScrn, NULL);

    /* Process the options */
    xf86ProcessOptions(pScrn->scrnIndex, pScrn->options, NVOptions);

    /* Set the bits per RGB for 8bpp mode */
    if (pScrn->depth == 8)
	pScrn->rgbBits = 8;

    from = X_DEFAULT;
    pNv->HWCursor = TRUE;
    /*
     * The preferred method is to use the "hw cursor" option as a tri-state
     * option, with the default set above.
     */
    if (xf86GetOptValBool(NVOptions, OPTION_HW_CURSOR, &pNv->HWCursor)) {
	from = X_CONFIG;
    }
    /* For compatibility, accept this too (as an override) */
    if (xf86ReturnOptValBool(NVOptions, OPTION_SW_CURSOR, FALSE)) {
	from = X_CONFIG;
	pNv->HWCursor = FALSE;
    }
    xf86DrvMsg(pScrn->scrnIndex, from, "Using %s cursor\n",
		pNv->HWCursor ? "HW" : "SW");
    if (xf86ReturnOptValBool(NVOptions, OPTION_NOACCEL, FALSE)) {
	pNv->NoAccel = TRUE;
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "Acceleration disabled\n");
    }
    if (xf86ReturnOptValBool(NVOptions, OPTION_SHOWCACHE, FALSE)) {
	pNv->ShowCache = TRUE;
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "ShowCache enabled\n");
    }
    if (xf86ReturnOptValBool(NVOptions, OPTION_SHADOW_FB, FALSE)) {
	pNv->ShadowFB = TRUE;
	pNv->NoAccel = TRUE;
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, 
		"Using \"Shadow Framebuffer\" - acceleration disabled\n");
    }
    if (xf86ReturnOptValBool(NVOptions, OPTION_FBDEV, FALSE)) {
	pNv->FBDev = TRUE;
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, 
		"Using framebuffer device\n");
    }
    if (pNv->FBDev) {
	/* check for linux framebuffer device */
	if (!xf86LoadSubModule(pScrn, "fbdevhw"))
	    return FALSE;
	xf86LoaderReqSymLists(fbdevHWSymbols, NULL);
	if (!fbdevHWInit(pScrn, pNv->PciInfo, NULL))
	    return FALSE;
	pScrn->SwitchMode    = fbdevHWSwitchMode;
	pScrn->AdjustFrame   = fbdevHWAdjustFrame;
	pScrn->EnterVT       = NVEnterVTFBDev;
	pScrn->LeaveVT       = fbdevHWLeaveVT;
	pScrn->ValidMode     = fbdevHWValidMode;
    }
    pNv->Rotate = 0;
    if ((s = xf86GetOptValString(NVOptions, OPTION_ROTATE))) {
      if(!xf86NameCmp(s, "CW")) {
	pNv->ShadowFB = TRUE;
	pNv->NoAccel = TRUE;
	pNv->HWCursor = FALSE;
	pNv->Rotate = 1;
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, 
		"Rotating screen clockwise - acceleration disabled\n");
      } else
      if(!xf86NameCmp(s, "CCW")) {
	pNv->ShadowFB = TRUE;
	pNv->NoAccel = TRUE;
	pNv->HWCursor = FALSE;
	pNv->Rotate = -1;
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, 
		"Rotating screen counter clockwise - acceleration disabled\n");
      } else {
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, 
		"\"%s\" is not a valid value for Option \"Rotate\"\n", s);
	xf86DrvMsg(pScrn->scrnIndex, X_INFO, 
		"Valid options are \"CW\" or \"CCW\"\n");
      }
    }
    
    if (pNv->pEnt->device->MemBase != 0) {
	/* Require that the config file value matches one of the PCI values. */
	if (!xf86CheckPciMemBase(pNv->PciInfo, pNv->pEnt->device->MemBase)) {
	    xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		"MemBase 0x%08lX doesn't match any PCI base register.\n",
		pNv->pEnt->device->MemBase);
	    NVFreeRec(pScrn);
	    return FALSE;
	}
	pNv->FbAddress = pNv->pEnt->device->MemBase;
	from = X_CONFIG;
    } else {
	int i = 1;
	pNv->FbBaseReg = i;
	if (pNv->PciInfo->memBase[i] != 0) {
	    pNv->FbAddress = pNv->PciInfo->memBase[i] & 0xff800000;
	    from = X_PROBED;
	} else {
	    xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
			   "No valid FB address in PCI config space\n");
	    NVFreeRec(pScrn);
	    return FALSE;
	}
    }
    xf86DrvMsg(pScrn->scrnIndex, from, "Linear framebuffer at 0x%lX\n",
	       (unsigned long)pNv->FbAddress);

    if (pNv->pEnt->device->IOBase != 0) {
	/* Require that the config file value matches one of the PCI values. */
	if (!xf86CheckPciMemBase(pNv->PciInfo, pNv->pEnt->device->IOBase)) {
	    xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		"IOBase 0x%08lX doesn't match any PCI base register.\n",
		pNv->pEnt->device->IOBase);
	    NVFreeRec(pScrn);
	    return FALSE;
	}
	pNv->IOAddress = pNv->pEnt->device->IOBase;
	from = X_CONFIG;
    } else {
	int i = 0;
	if (pNv->PciInfo->memBase[i] != 0) {
	    pNv->IOAddress = pNv->PciInfo->memBase[i] & 0xffffc000;
	    from = X_PROBED;
	} else {
	    xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
			"No valid MMIO address in PCI config space\n");
	    NVFreeRec(pScrn);
	    return FALSE;
	}
    }
    xf86DrvMsg(pScrn->scrnIndex, from, "MMIO registers at 0x%lX\n",
	       (unsigned long)pNv->IOAddress);
     
    if (xf86RegisterResources(pNv->pEnt->index, NULL, ResExclusive)) {
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		"xf86RegisterResources() found resource conflicts\n");
	NVFreeRec(pScrn);
	return FALSE;
    }

    switch (pNv->Chipset) {
        case NV_CHIP_RIVA128:
            NV3Setup(pScrn);
            break;
            
        case NV_CHIP_TNT:
        case NV_CHIP_TNT2:
        case NV_CHIP_UTNT2:
        case NV_CHIP_VTNT2:
        case NV_CHIP_UVTNT2:
        case NV_CHIP_ITNT2:
            NV4Setup(pScrn);
            break;

        case NV_CHIP_GEFORCE256:
        case NV_CHIP_GEFORCEDDR:
        case NV_CHIP_QUADRO:
        case NV_CHIP_GEFORCE2GTS:
        case NV_CHIP_GEFORCE2GTS_1:
        case NV_CHIP_GEFORCE2ULTRA:
        case NV_CHIP_QUADRO2PRO:
        case NV_CHIP_GEFORCE2MX:
        case NV_CHIP_GEFORCE2MXDDR:
        case NV_CHIP_QUADRO2MXR:
	case NV_CHIP_GEFORCE2GO:
            NV10Setup(pScrn);
            break;
    }

    /*
     * fill riva structure etc.
     */
    (*pNv->PreInit)(pScrn);
    

    /*
     * If the user has specified the amount of memory in the XF86Config
     * file, we respect that setting.
     */
    if (pNv->pEnt->device->videoRam != 0) {
	pScrn->videoRam = pNv->pEnt->device->videoRam;
	from = X_CONFIG;
    } else {
	if (pNv->FBDev) {
	    pScrn->videoRam = fbdevHWGetVidmem(pScrn)/1024;
	} else {
            pScrn->videoRam = pNv->riva.RamAmountKBytes;
	}
	from = X_PROBED;
    }
    xf86DrvMsg(pScrn->scrnIndex, from, "VideoRAM: %d kBytes\n",
               pScrn->videoRam);
	
    pNv->FbMapSize = pScrn->videoRam * 1024;

    /* Read and print the Monitor DDC info */
    pScrn->monitor->DDC = NVdoDDC(pScrn);
#if 0
    /*
     * This code was for testing. It will be removed as soon
     * as this is integrated into the common level.
     */
    if ((!pScrn->monitor->nHsync || !pScrn->monitor->nVrefresh)
 	&& pScrn->monitor->DDC) {
 	int i;
 	int h = (!pScrn->monitor->nHsync) ? 0 : -1;
 	int v = (!pScrn->monitor->nVrefresh) ? 0 : -1;
 	xf86MonPtr pMon = (xf86MonPtr)pScrn->monitor->DDC;
 	for (i = 0; i < DET_TIMINGS; i++) {
 	    if (pMon->det_mon[i].type == DS_RANGES) {
 		if (h != -1) {
 		    pScrn->monitor->hsync[h].lo
 			= pMon->det_mon[i].section.ranges.min_h;
 		    pScrn->monitor->hsync[h++].hi
 			= pMon->det_mon[i].section.ranges.max_h;
 		}
 		if (v != -1) {
 		    pScrn->monitor->vrefresh[v].lo
 			= pMon->det_mon[i].section.ranges.min_v;
 		    pScrn->monitor->vrefresh[v++].hi
 			= pMon->det_mon[i].section.ranges.max_v;
 		}
 	    }
 	}
 	if (h != -1) pScrn->monitor->nHsync = h;
 	if (v != -1) pScrn->monitor->nVrefresh = v;
     }     
#endif
    /*
     * If the driver can do gamma correction, it should call xf86SetGamma()
     * here.
     */

    {
	Gamma zeros = {0.0, 0.0, 0.0};

	if (!xf86SetGamma(pScrn, zeros)) {
	    return FALSE;
	}
    }

    pNv->FbUsableSize = pNv->FbMapSize;

    /* Remove reserved memory from end of buffer */
    switch( pNv->riva.Architecture ) {
        case NV_ARCH_03:
            pNv->FbUsableSize -= 32 * 1024;
            break;
        case NV_ARCH_04:
        case NV_ARCH_10:
            pNv->FbUsableSize -= 128 * 1024;
            break;
    }


    /*
     * Setup the ClockRanges, which describe what clock ranges are available,
     * and what sort of modes they can be used for.
     */

    pNv->MinClock = 12000;
    pNv->MaxClock = pNv->riva.MaxVClockFreqKHz;

    clockRanges = xnfcalloc(sizeof(ClockRange), 1);
    clockRanges->next = NULL;
    clockRanges->minClock = pNv->MinClock;
    clockRanges->maxClock = pNv->MaxClock;
    clockRanges->clockIndex = -1;		/* programmable */
    clockRanges->interlaceAllowed = TRUE;
    clockRanges->doubleScanAllowed = TRUE;

    /*
     * xf86ValidateModes will check that the mode HTotal and VTotal values
     * don't exceed the chipset's limit if pScrn->maxHValue and
     * pScrn->maxVValue are set.  Since our NVValidMode() already takes
     * care of this, we don't worry about setting them here.
     */
    i = xf86ValidateModes(pScrn, pScrn->monitor->Modes,
                          pScrn->display->modes, clockRanges,
                          NULL, 256, 2048,
                          32 * pScrn->bitsPerPixel, 128, 2048,
                          pScrn->display->virtualX,
                          pScrn->display->virtualY,
                          pNv->FbUsableSize,
                          LOOKUP_BEST_REFRESH);

    if (i < 1 && pNv->FBDev) {
	fbdevHWUseBuildinMode(pScrn);
	pScrn->displayWidth = pScrn->virtualX; /* FIXME: might be wrong */
	i = 1;
    }
    if (i == -1) {
	NVFreeRec(pScrn);
	return FALSE;
    }

    /* Prune the modes marked as invalid */
    xf86PruneDriverModes(pScrn);

    if (i == 0 || pScrn->modes == NULL) {
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "No valid modes found\n");
	NVFreeRec(pScrn);
	return FALSE;
    }

    /*
     * Set the CRTC parameters for all of the modes based on the type
     * of mode, and the chipset's interlace requirements.
     *
     * Calling this is required if the mode->Crtc* values are used by the
     * driver and if the driver doesn't provide code to set them.  They
     * are not pre-initialised at all.
     */
    xf86SetCrtcForModes(pScrn, INTERLACE_HALVE_V);

    /* Set the current mode to the first in the list */
    pScrn->currentMode = pScrn->modes;

    /* Print the list of modes being used */
    xf86PrintModes(pScrn);

    /* Set display resolution */
    xf86SetDpi(pScrn, 0, 0);


    /*
     * XXX This should be taken into account in some way in the mode valdation
     * section.
     */

#ifndef NV_USE_FB    
    /* Load bpp-specific modules */
    switch (pScrn->bitsPerPixel) {
        case 8:
            mod = "cfb";
            reqSym = "cfbScreenInit";
            break;
        case 16:
            mod = "cfb16";
            reqSym = "cfb16ScreenInit";
            break;
        case 32:
            mod = "cfb32";
            reqSym = "cfb32ScreenInit";
            break;
        default:
            return FALSE;
    }
    if (mod && xf86LoadSubModule(pScrn, mod) == NULL) {
	NVFreeRec(pScrn);
	return FALSE;
    }

    xf86LoaderReqSymbols(reqSym, NULL);
#else
    if (xf86LoadSubModule(pScrn, "fb") == NULL) {
	NVFreeRec(pScrn);
	return FALSE;
    }

    xf86LoaderReqSymLists(fbSymbols, NULL);
#endif
    
    /* Load XAA if needed */
    if (!pNv->NoAccel) {
	if (!xf86LoadSubModule(pScrn, "xaa")) {
	    NVFreeRec(pScrn);
	    return FALSE;
	}
	xf86LoaderReqSymLists(xaaSymbols, NULL);
    }

    /* Load ramdac if needed */
    if (pNv->HWCursor) {
	if (!xf86LoadSubModule(pScrn, "ramdac")) {
	    NVFreeRec(pScrn);
	    return FALSE;
	}
	xf86LoaderReqSymLists(ramdacSymbols, NULL);
    }

    /* Load shadowfb if needed */
    if (pNv->ShadowFB) {
	if (!xf86LoadSubModule(pScrn, "shadowfb")) {
	    NVFreeRec(pScrn);
	    return FALSE;
	}
	xf86LoaderReqSymLists(shadowSymbols, NULL);
    }

    pNv->CurrentLayout.bitsPerPixel = pScrn->bitsPerPixel;
    pNv->CurrentLayout.depth = pScrn->depth;
    pNv->CurrentLayout.displayWidth = pScrn->displayWidth;
    pNv->CurrentLayout.weight.red = pScrn->weight.red;
    pNv->CurrentLayout.weight.green = pScrn->weight.green;
    pNv->CurrentLayout.weight.blue = pScrn->weight.blue;
    pNv->CurrentLayout.mode = pScrn->currentMode;

    return TRUE;
}


/*
 * Map the framebuffer and MMIO memory.
 */

static Bool
NVMapMem(ScrnInfoPtr pScrn)
{
    NVPtr pNv;
        
    DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO, "NVMapMem\n"));
    pNv = NVPTR(pScrn);

    /*
     * Map IO registers to virtual address space
     */ 
    pNv->IOBase = xf86MapPciMem(pScrn->scrnIndex,
                                VIDMEM_MMIO | VIDMEM_READSIDEEFFECT,
                                pNv->PciTag, pNv->IOAddress, 0x1000000);
    if (pNv->IOBase == NULL)
	return FALSE;

    pNv->FbBase = xf86MapPciMem(pScrn->scrnIndex, VIDMEM_FRAMEBUFFER,
				 pNv->PciTag, pNv->FbAddress,
				 pNv->FbMapSize);
    if (pNv->FbBase == NULL)
	return FALSE;

    pNv->FbStart = pNv->FbBase;

    return TRUE;
}

Bool
NVMapMemFBDev(ScrnInfoPtr pScrn)
{
    NVPtr pNv;

    DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO, "NVMamMemFBDev\n"));
    pNv = NVPTR(pScrn);

    pNv->FbBase = fbdevHWMapVidmem(pScrn);
    if (pNv->FbBase == NULL)
        return FALSE;

    pNv->IOBase = fbdevHWMapMMIO(pScrn);
    if (pNv->IOBase == NULL)
        return FALSE;

    pNv->FbStart = pNv->FbBase;

    return TRUE;
}

/*
 * Unmap the framebuffer and MMIO memory.
 */

static Bool
NVUnmapMem(ScrnInfoPtr pScrn)
{
    NVPtr pNv;
    
    DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO, "NVUnmapMem\n"));
    pNv = NVPTR(pScrn);

    /*
     * Unmap IO registers to virtual address space
     */ 
    xf86UnMapVidMem(pScrn->scrnIndex, (pointer)pNv->IOBase, 0x1000000);
    pNv->IOBase = NULL;

    xf86UnMapVidMem(pScrn->scrnIndex, (pointer)pNv->FbBase, pNv->FbMapSize);
    pNv->FbBase = NULL;
    pNv->FbStart = NULL;

    return TRUE;
}


/*
 * Initialise a new mode.  This is currently still using the old
 * "initialise struct, restore/write struct to HW" model.  That could
 * be changed.
 */

static Bool
NVModeInit(ScrnInfoPtr pScrn, DisplayModePtr mode)
{
    vgaHWPtr hwp = VGAHWPTR(pScrn);
    vgaRegPtr vgaReg;
    NVPtr pNv = NVPTR(pScrn);
    NVRegPtr nvReg;

    DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO, "NVModeInit\n"));
    
    /* Initialise the ModeReg values */
    if (!vgaHWInit(pScrn, mode))
	return FALSE;
    pScrn->vtSema = TRUE;

    if ( pNv->ModeInit ) {
        if (!(*pNv->ModeInit)(pScrn, mode))
            return FALSE;
    }

    /* Program the registers */
    vgaHWProtect(pScrn, TRUE);
    vgaReg = &hwp->ModeReg;
    nvReg = &pNv->ModeReg;

    if ( pNv->Restore )
        (*pNv->Restore)(pScrn, vgaReg, nvReg, FALSE);

    vgaHWProtect(pScrn, FALSE);

    pNv->CurrentLayout.mode = mode;

    return TRUE;
}

/*
 * Restore the initial (text) mode.
 */
static void 
NVRestore(ScrnInfoPtr pScrn)
{
    vgaHWPtr hwp = VGAHWPTR(pScrn);
    vgaRegPtr vgaReg = &hwp->SavedReg;
    NVPtr pNv = NVPTR(pScrn);
    NVRegPtr nvReg = &pNv->SavedReg;

    DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO, "NVRestore\n"));
    /* Only restore text mode fonts/text for the primary card */
    vgaHWProtect(pScrn, TRUE);
    if (pNv->Primary)
        (*pNv->Restore)(pScrn, vgaReg, nvReg, TRUE);
    else
        vgaHWRestore(pScrn, vgaReg, VGA_SR_MODE);
    vgaHWProtect(pScrn, FALSE);
}


/* Mandatory */

/* This gets called at the start of each server generation */

static Bool
NVScreenInit(int scrnIndex, ScreenPtr pScreen, int argc, char **argv)
{
    ScrnInfoPtr pScrn;
    vgaHWPtr hwp;
    NVPtr pNv;
    NVRamdacPtr NVdac;
    int ret;
    VisualPtr visual;
    unsigned char *FBStart;
    int width, height, displayWidth;

    /* 
     * First get the ScrnInfoRec
     */
    pScrn = xf86Screens[pScreen->myNum];

    DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO, "NVScreenInit\n"));

    hwp = VGAHWPTR(pScrn);
    pNv = NVPTR(pScrn);
    NVdac = &pNv->Dac;

    /* Map the NV memory and MMIO areas */
    if (pNv->FBDev) {
	if (!NVMapMemFBDev(pScrn))
	    return FALSE;
    } else {
	if (!NVMapMem(pScrn))
	    return FALSE;
    }
    DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO, "- Mem Mapped\n"));
    
    /* Map the VGA memory when the primary video */
    if (pNv->Primary && !pNv->FBDev) {
	hwp->MapSize = 0x10000;
	if (!vgaHWMapMem(pScrn))
	    return FALSE;
    }
    DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO, "- VGA Mapped\n"));

    if (pNv->FBDev) {
	fbdevHWSave(pScrn);
	if (!fbdevHWModeInit(pScrn, pScrn->currentMode))
	    return FALSE;
    } else {
	/* Save the current state */
        vgaHWUnlock(hwp);
        pNv->riva.LockUnlock(&pNv->riva, 0);
	NVSave(pScrn);
	/* Initialise the first mode */
	if (!NVModeInit(pScrn, pScrn->currentMode))
	    return FALSE;
    }

    DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO, "- State saved\n"));

    /* Darken the screen for aesthetic reasons and set the viewport */
    NVSaveScreen(pScreen, SCREEN_SAVER_ON);
    pScrn->AdjustFrame(scrnIndex, pScrn->frameX0, pScrn->frameY0, 0);

    DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO, "- Blanked\n"));

    /*
     * The next step is to setup the screen's visuals, and initialise the
     * framebuffer code.  In cases where the framebuffer's default
     * choices for things like visual layouts and bits per RGB are OK,
     * this may be as simple as calling the framebuffer's ScreenInit()
     * function.  If not, the visuals will need to be setup before calling
     * a fb ScreenInit() function and fixed up after.
     *
     * For most PC hardware at depths >= 8, the defaults that cfb uses
     * are not appropriate.  In this driver, we fixup the visuals after.
     */

    /*
     * Reset the visual list.
     */
    miClearVisualTypes();

    /* Setup the visuals we support. */

#ifndef NV_USE_FB
      if (pScrn->bitsPerPixel > 8) {
          if (!miSetVisualTypes(pScrn->depth, TrueColorMask, pScrn->rgbBits,
                                pScrn->defaultVisual))
              return FALSE;
    } else 
#endif
    {
          if (!miSetVisualTypes(pScrn->depth, 
                                miGetDefaultVisualMask(pScrn->depth),
                                pScrn->rgbBits, pScrn->defaultVisual))
	  return FALSE;
     }
#ifdef NV_USE_FB
      if (!miSetPixmapDepths ()) return FALSE;
#endif

    DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO, "- Visuals set up\n"));

    /*
     * Call the framebuffer layer's ScreenInit function, and fill in other
     * pScreen fields.
     */

    width = pScrn->virtualX;
    height = pScrn->virtualY;
    displayWidth = pScrn->displayWidth;


    if(pNv->Rotate) {
	height = pScrn->virtualX;
	width = pScrn->virtualY;
    }

    if(pNv->ShadowFB) {
 	pNv->ShadowPitch = BitmapBytePad(pScrn->bitsPerPixel * width);
        pNv->ShadowPtr = xalloc(pNv->ShadowPitch * height);
	displayWidth = pNv->ShadowPitch / (pScrn->bitsPerPixel >> 3);
        FBStart = pNv->ShadowPtr;
    } else {
	pNv->ShadowPtr = NULL;
	FBStart = pNv->FbStart;
    }

    switch (pScrn->bitsPerPixel) {
#ifndef NV_USE_FB        
        case 8:
            ret = cfbScreenInit(pScreen, FBStart, width, height,
                                pScrn->xDpi, pScrn->yDpi, displayWidth);
            break;
        case 16:
            ret = cfb16ScreenInit(pScreen, FBStart, width, height,
                                  pScrn->xDpi, pScrn->yDpi, displayWidth);
            break;
        case 32:
            ret = cfb32ScreenInit(pScreen, FBStart, width, height,
                                  pScrn->xDpi, pScrn->yDpi, displayWidth);
            break;
#else
        case 8:
        case 16:
        case 32:
            ret = fbScreenInit(pScreen, FBStart, pScrn->virtualX,
                               pScrn->virtualY, pScrn->xDpi, pScrn->yDpi,
                               displayWidth, pScrn->bitsPerPixel);
#ifdef RENDER
            if (ret)
                fbPictureInit (pScreen, 0, 0);
#endif
            break;
#endif           
        default:
            xf86DrvMsg(scrnIndex, X_ERROR,
                       "Internal error: invalid bpp (%d) in NVScreenInit\n",
                       pScrn->bitsPerPixel);
            ret = FALSE;
            break;
    }
    if (!ret)
	return FALSE;

    DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO, "- cfb set up\n"));

    if (pScrn->bitsPerPixel > 8) {
        /* Fixup RGB ordering */
        visual = pScreen->visuals + pScreen->numVisuals;
        while (--visual >= pScreen->visuals) {
	    if ((visual->class | DynamicClass) == DirectColor) {
		visual->offsetRed = pScrn->offset.red;
		visual->offsetGreen = pScrn->offset.green;
		visual->offsetBlue = pScrn->offset.blue;
		visual->redMask = pScrn->mask.red;
		visual->greenMask = pScrn->mask.green;
		visual->blueMask = pScrn->mask.blue;
	    }
	}
    }

    xf86SetBlackWhitePixels(pScreen);

    DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO, "- B & W\n"));

    if(!pNv->ShadowFB) /* hardware cursor needs to wrap this layer */
	NVDGAInit(pScreen);
    
    if (!pNv->NoAccel)
	NVAccelInit(pScreen);
    
    miInitializeBackingStore(pScreen);
    xf86SetBackingStore(pScreen);
    xf86SetSilkenMouse(pScreen);

    DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO, "- Backing store set up\n"));

    /* Initialize software cursor.  
	Must precede creation of the default colormap */
    miDCInitialize(pScreen, xf86GetPointerScreenFuncs());

    DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO, "- SW cursor set up\n"));

    /* Initialize HW cursor layer. 
	Must follow software cursor initialization*/
    if (pNv->HWCursor) { 
	if(!NVCursorInit(pScreen))
	    xf86DrvMsg(pScrn->scrnIndex, X_ERROR, 
		"Hardware cursor initialization failed\n");
    }

    /* Initialise default colourmap */
    if (!miCreateDefColormap(pScreen))
	return FALSE;

    DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO, "- Def Color map set up\n"));
    
    /* Initialize colormap layer.  
	Must follow initialization of the default colormap */
    if(!xf86HandleColormaps(pScreen, 256, 
		(pNv->riva.Architecture == 3) ? 6 : 8,
		(pNv->FBDev ? fbdevHWLoadPalette : NVdac->LoadPalette), 
		NULL, CMAP_RELOAD_ON_MODE_SWITCH))
	return FALSE;

    DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO, "- Palette loaded\n"));
    
    if(pNv->ShadowFB) {
	RefreshAreaFuncPtr refreshArea = NVRefreshArea;

	if(pNv->Rotate) {
   	   pNv->PointerMoved = pScrn->PointerMoved;
	   pScrn->PointerMoved = NVPointerMoved;

	   switch(pScrn->bitsPerPixel) {
               case 8:	refreshArea = NVRefreshArea8;	break;
               case 16:	refreshArea = NVRefreshArea16;	break;
               case 32:	refreshArea = NVRefreshArea32;	break;
	   }
	}

	ShadowFBInit(pScreen, refreshArea);
    }

#ifdef DPMSExtension
    /* Call the vgaHW DPMS function directly.
       XXX There must be a way to get all the DPMS modes. */
    xf86DPMSInit(pScreen, vgaHWDPMSSet, 0);
    DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO, "- DPMS set up\n"));
#endif

    DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO, "- Color maps etc. set up\n"));
    
    pScrn->memPhysBase = pNv->FbAddress;
    pScrn->fbOffset = 0;

#ifdef XvExtension
    {
	XF86VideoAdaptorPtr *ptr;
	int n;
	
	n = xf86XVListGenericAdaptors(pScrn,&ptr);
	if (n) { 
	    xf86XVScreenInit(pScreen, ptr, n);
	}
    }
    DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO, "- Xv set up\n"));
#endif

    pScreen->SaveScreen = NVSaveScreen;

    /* Wrap the current CloseScreen function */
    pNv->CloseScreen = pScreen->CloseScreen;
    pScreen->CloseScreen = NVCloseScreen;

    /* Report any unused options (only for the first generation) */
    if (serverGeneration == 1) {
	xf86ShowUnusedOptions(pScrn->scrnIndex, pScrn->options);
    }
    /* Done */
    DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO, "- Done\n"));
    return TRUE;
}

/* Free up any persistent data structures */


/* Do screen blanking */

/* Mandatory */
static Bool
NVSaveScreen(ScreenPtr pScreen, int mode)
{
    return vgaHWSaveScreen(pScreen, mode);
}

static void
NVSave(ScrnInfoPtr pScrn)
{
    NVPtr pNv = NVPTR(pScrn);
    NVRegPtr nvReg = &pNv->SavedReg;
    vgaHWPtr pVga = VGAHWPTR(pScrn);
    vgaRegPtr vgaReg = &pVga->SavedReg;

    DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO, "NVSave\n"));
    vgaHWSave(pScrn, vgaReg, VGA_SR_CMAP|VGA_SR_MODE|VGA_SR_FONTS);
    pNv->riva.UnloadStateExt(&pNv->riva, nvReg);
}

