/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/apm/apm_driver.c,v 1.46 2000/12/02 15:30:31 tsi Exp $ */


#include "apm.h"
#include "xf86cmap.h"
#include "shadowfb.h"
#include "xf86Resources.h"
#include "xf86int10.h"
#include "xf86RAC.h"
#include "vbe.h"

#ifdef DPMSExtension
#include "opaque.h"
#include "extensions/dpms.h"
#endif

#define VERSION			4000
#define APM_NAME		"APM"
#define APM_DRIVER_NAME		"apm"
#define APM_MAJOR_VERSION       1
#define APM_MINOR_VERSION       0
#define APM_PATCHLEVEL          0
#ifndef PCI_CHIP_AT3D
#define PCI_CHIP_AT3D	0x643D
#endif

/* bytes to save for text/font data */
#define TEXT_AMOUNT 32768

/* Mandatory functions */
static OptionInfoPtr	ApmAvailableOptions(int chipid, int busid);
static void     ApmIdentify(int flags);
static Bool     ApmProbe(DriverPtr drv, int flags);
static Bool     ApmPreInit(ScrnInfoPtr pScrn, int flags);
static Bool     ApmScreenInit(int Index, ScreenPtr pScreen, int argc,
                                  char **argv);
static Bool     ApmEnterVT(int scrnIndex, int flags);
static void     ApmLeaveVT(int scrnIndex, int flags);
static Bool     ApmCloseScreen(int scrnIndex, ScreenPtr pScreen);
static void     ApmFreeScreen(int scrnIndex, int flags);
static int      ApmValidMode(int scrnIndex, DisplayModePtr mode,
                                 Bool verbose, int flags);
static Bool	ApmSaveScreen(ScreenPtr pScreen, int mode);
static void	ApmUnlock(ApmPtr pApm);
static void	ApmLock(ApmPtr pApm);
static void	ApmRestore(ScrnInfoPtr pScrn, vgaRegPtr vgaReg,
			    ApmRegPtr ApmReg);
static void	ApmLoadPalette(ScrnInfoPtr pScrn, int numColors, int *indices,
				LOCO *colors, VisualPtr pVisual);
#ifdef DPMSExtension
static void	ApmDisplayPowerManagementSet(ScrnInfoPtr pScrn,
					     int PowerManagementMode,
					     int flags);
#endif
static void	ApmProbeDDC(ScrnInfoPtr pScrn, int index);


int ApmPixmapIndex = -1;
static int ApmGeneration = -1;
static int pix24bpp = 0;

DriverRec APM = {
	VERSION,
	APM_DRIVER_NAME,
	ApmIdentify,
	ApmProbe,
	ApmAvailableOptions,
	NULL,
	0
};

static SymTabRec ApmChipsets[] = {
    { AP6422,	"AP6422"	},
    { AT24,	"AT24"		},
    { AT3D,	"AT3D"		},
    { -1,	NULL		}
};

static PciChipsets ApmPciChipsets[] = {
    { PCI_CHIP_AP6422,	PCI_CHIP_AP6422,	RES_SHARED_VGA },
    { PCI_CHIP_AT24,	PCI_CHIP_AT24,		RES_SHARED_VGA },
    { PCI_CHIP_AT3D,	PCI_CHIP_AT3D,		RES_SHARED_VGA },
    { -1,			-1,		RES_UNDEFINED }
};

static IsaChipsets ApmIsaChipsets[] = {
    { PCI_CHIP_AP6422,	RES_EXCLUSIVE_VGA},
    {-1,		RES_UNDEFINED}
};

typedef enum {
    OPTION_SET_MCLK,
    OPTION_SW_CURSOR,
    OPTION_HW_CURSOR,
    OPTION_NOLINEAR,
    OPTION_NOACCEL,
    OPTION_SHADOW_FB,
    OPTION_PCI_BURST,
    OPTION_PCI_RETRY
} ApmOpts;

static OptionInfoRec ApmOptions[] =
{
    {OPTION_SET_MCLK, "SetMclk", OPTV_FREQ,
	{0}, FALSE},
    {OPTION_SW_CURSOR, "SWcursor", OPTV_BOOLEAN,
	{0}, FALSE},
    {OPTION_HW_CURSOR, "HWcursor", OPTV_BOOLEAN,
	{0}, FALSE},
    {OPTION_NOLINEAR, "NoLinear", OPTV_BOOLEAN,
	{0}, FALSE},
    {OPTION_NOACCEL, "NoAccel", OPTV_BOOLEAN,
	{0}, FALSE},
    {OPTION_SHADOW_FB, "ShadowFB", OPTV_BOOLEAN,
	{0}, FALSE},
    {OPTION_PCI_BURST, "pci_burst", OPTV_BOOLEAN,
	{0}, FALSE},
    {OPTION_PCI_RETRY, "PciRetry", OPTV_BOOLEAN,
	{0}, FALSE},
    {-1, NULL, OPTV_NONE,
	{0}, FALSE}
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
    "vgaHWBlankScreen",
    "vgaHWCursor",
    "vgaHWFreeHWRec",
    "vgaHWGetHWRec",
    "vgaHWGetIOBase",
    "vgaHWInit",
    "vgaHWLock",
    "vgaHWMapMem",
    "vgaHWProtect",
    "vgaHWRestore",
    "vgaHWSave",
    "vgaHWSaveScreen",
    "vgaHWSetMmioFuncs",
    "vgaHWUnlock",
    NULL
};

static const char *xaaSymbols[] = {
    "XAACreateInfoRec",
    "XAACursorInfoRec",
    "XAACursorInit",
    "XAADestroyInfoRec",
    "XAAInit",
    "XAAPixmapIndex",
    "XAAQueryBestSize",
    "XAAReverseBitOrder",
    "XAARestoreCursor",
    "XAAScreenIndex",
    "XAAStippleScanlineFuncMSBFirst",
    "XAAGlyphScanlineFuncLSBFirst",
    "XAAWarpCursor",
    NULL
};

static const char *ramdacSymbols[] = {
    "xf86InitCursor",
    "xf86CreateCursorInfoRec",
    "xf86DestroyCursorInfoRec",
    NULL
};

static const char *vbeSymbols[] = {
    "VBEInit",
    "vbeDoEDID",
    NULL
};

static const char *ddcSymbols[] = {
    "xf86PrintEDID",
    "xf86DoEDID_DDC1",
    "xf86DoEDID_DDC2",
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


#ifdef XFree86LOADER

static const char *cfbSymbols[] = {
    "xf1bppScreenInit",
    "xf4bppScreenInit",
    "cfbScreenInit",
    "cfb16ScreenInit",
    "cfb24ScreenInit",
    "cfb32ScreenInit",
    "cfb24_32ScreenInit",
    NULL
};

static XF86ModuleVersionInfo apmVersRec = {
    "apm",
    MODULEVENDORSTRING,
    MODINFOSTRING1,
    MODINFOSTRING2,
    XF86_VERSION_CURRENT,
    APM_MAJOR_VERSION, APM_MINOR_VERSION, APM_PATCHLEVEL,
    ABI_CLASS_VIDEODRV,			/* This is a video driver */
    ABI_VIDEODRV_VERSION,
    MOD_CLASS_VIDEODRV,
    {0,0,0,0}
};

static MODULESETUPPROTO(apmSetup);

/*
 * This is the module init data.
 * Its name has to be the driver name followed by ModuleData.
 */
XF86ModuleData apmModuleData = { &apmVersRec, apmSetup, NULL };

static pointer
apmSetup(pointer module, pointer opts, int *errmaj, int *errmain)
{
    static Bool setupDone = FALSE;

    if (!setupDone) {
	setupDone = TRUE;
	xf86AddDriver(&APM, module, 0);

	LoaderRefSymLists(vgahwSymbols, cfbSymbols, xaaSymbols, 
			  /*xf8_32bppSymbols,*/ ramdacSymbols, vbeSymbols,
			  ddcSymbols, i2cSymbols, shadowSymbols, NULL);

	return (pointer)1;
    }
    else {
	if (errmaj) *errmaj = LDR_ONCEONLY;
	return NULL;
    }
}
#endif

static Bool
ApmGetRec(ScrnInfoPtr pScrn)
{
    if (pScrn->driverPrivate)
	return TRUE;
    pScrn->driverPrivate = xnfcalloc(sizeof(ApmRec), 1);
    /* pScrn->driverPrivate != NULL at this point */

    return TRUE;
}

static void
ApmFreeRec(ScrnInfoPtr pScrn)
{
    if (pScrn->driverPrivate) {
	xfree(pScrn->driverPrivate);
	pScrn->driverPrivate = NULL;
    }
}


/* unlock Alliance registers */
static void
ApmUnlock(ApmPtr pApm)
{
    if (pApm->Chipset >= AT3D && !pApm->noLinear)
	ApmWriteSeq(0x10, 0x12);
    else
	wrinx(0x3C4, 0x10, 0x12);
}

/* lock Alliance registers */
static void
ApmLock(ApmPtr pApm)
{
    if (pApm->Chipset >= AT3D && !pApm->noLinear)
	ApmWriteSeq(0x10, pApm->savedSR10 ? 0 : 0x12);
    else
	wrinx(0x3C4, 0x10, pApm->savedSR10 ? 0 : 0x12);
}

static void
ApmIdentify(int flags)
{
    xf86PrintChipsets(APM_NAME, "driver for the Alliance chipsets",
		      ApmChipsets);
}

static
OptionInfoPtr
ApmAvailableOptions(int chipid, int busid)
{
    return ApmOptions;
}

static int
ApmFindIsaDevice(GDevPtr dev)
{
    char	save = rdinx(0x3C4, 0x10);
    int		i;
    int		apmChip = -1;

    /*
     * Start by probing the VGA chipset.
     */
    outw(0x3C4, 0x1210);
    if (rdinx(0x3C4, 0x11) == 'P' && rdinx(0x3C4, 0x12) == 'r' &&
	rdinx(0x3C4, 0x13) == 'o') {
	char	id_ap6420[] = "6420";
	char	id_ap6422[] = "6422";
	char	id_at24[]   = "6424";
	char	id_at3d[]   = "AT3D";
	char	idstring[]  = "    ";

	/*
	 * Must be an Alliance !!!
	 */
	for (i = 0; i < 4; i++)
	    idstring[i] = rdinx(0x3C4, 0x14 + i);
	if (!memcmp(id_ap6420, idstring, 4) ||
	    !memcmp(id_ap6422, idstring, 4))
	    apmChip = AP6422;
	else if (!memcmp(id_at24, idstring, 4))
	    apmChip = AT24;
	else if (!memcmp(id_at3d, idstring, 4))
	    apmChip = AT3D;
	if (apmChip >= 0) {
	    int	apm_xbase;

	    apm_xbase = (rdinx(0x3C4, 0x1F) << 8) | rdinx(0x3C4, 0x1E);

	    if (!(wrinx(0x3C4, 0x1D, 0xCA >> 2), inb(apm_xbase + 2))) {
		/*
		 * TODO Not PCI
		 */
	    }

	}
    }
    wrinx(0x3C4, 0x10, save);

    return apmChip;
}

static void
ApmAssignFPtr(ScrnInfoPtr pScrn)
{
    pScrn->driverVersion	= VERSION;
    pScrn->driverName		= APM_DRIVER_NAME;
    pScrn->name			= APM_NAME;
    pScrn->Probe		= ApmProbe;
    pScrn->PreInit		= ApmPreInit;
    pScrn->ScreenInit		= ApmScreenInit;
    pScrn->SwitchMode		= ApmSwitchMode;
    pScrn->AdjustFrame		= ApmAdjustFrame;
    pScrn->EnterVT		= ApmEnterVT;
    pScrn->LeaveVT		= ApmLeaveVT;
    pScrn->FreeScreen		= ApmFreeScreen;
    pScrn->ValidMode		= ApmValidMode;
}

static Bool
ApmProbe(DriverPtr drv, int flags)
{
    int			numDevSections, numUsed, i;
    GDevPtr		*DevSections;
    int			*usedChips;
    int			foundScreen = FALSE;

    /*
     * Check if there is a chipset override in the config file
     */
    if ((numDevSections = xf86MatchDevice(APM_DRIVER_NAME,
					   &DevSections)) <= 0)
	return FALSE;

    /*
     * We need to probe the hardware first. We then need to see how this
     * fits in with what is given in the config file, and allow the config
     * file info to override any contradictions.
     */

    if (xf86GetPciVideoInfo() == NULL) {
	return FALSE;
    }
    numUsed = xf86MatchPciInstances(APM_NAME, PCI_VENDOR_ALLIANCE,
		    ApmChipsets, ApmPciChipsets, DevSections, numDevSections,
		    drv, &usedChips);

    if (numUsed > 0) {
	if (flags & PROBE_DETECT)
	    foundScreen = TRUE;
	else for (i = 0; i < numUsed; i++) {
	    ScrnInfoPtr	pScrn;
	    
	    /*
	     * Allocate a ScrnInfoRec and claim the slot
	     */
	    pScrn = NULL;
	    if ((pScrn = xf86ConfigPciEntity(pScrn, 0, usedChips[i],
						   ApmPciChipsets, NULL,
						   NULL,NULL,NULL,NULL))){
		
		/*
		 * Fill in what we can of the ScrnInfoRec
		 */
		ApmAssignFPtr(pScrn);
		foundScreen = TRUE;
	    }
	}
    }

    /* Check for non-PCI cards */
    numUsed = xf86MatchIsaInstances(APM_NAME, ApmChipsets,
			ApmIsaChipsets, drv, ApmFindIsaDevice, DevSections,
			numDevSections, &usedChips);
    if (numUsed > 0) {
	if (flags & PROBE_DETECT)
	    foundScreen = TRUE;
	else for (i = 0; i < numUsed; i++) {
	    ScrnInfoPtr pScrn = NULL;
	    if ((pScrn = xf86ConfigIsaEntity(pScrn, 0, usedChips[i],
					     ApmIsaChipsets, NULL, NULL, NULL,
					     NULL, NULL))) {
	    /*
	     * Fill in what we can of the ScrnInfoRec
	     */
	    ApmAssignFPtr(pScrn);
	    foundScreen = TRUE;
	    }
	}
    }
    xfree(DevSections);
    return foundScreen;
}

/*
 * GetAccelPitchValues -
 *
 * This function returns a list of display width (pitch) values that can
 * be used in accelerated mode.
 */
static int *
GetAccelPitchValues(ScrnInfoPtr pScrn)
{
    int *linePitches = NULL;
    int linep[] = {640, 800, 1024, 1152, 1280, 1600, 0};

    if (sizeof linep > 0) {
	linePitches = (int *)xnfalloc(sizeof linep);
	memcpy(linePitches, linep, sizeof linep);
    }

    return linePitches;
}

static unsigned int
ddc1Read(ScrnInfoPtr pScrn)
{
    APMDECL(pScrn);
    unsigned char	tmp;

    tmp = RDXB_IOP(0xD0);
    WRXB_IOP(0xD0, tmp & 0x07);
    while (STATUS_IOP() & 0x800);             
    while (!(STATUS_IOP() & 0x800));             
    return (STATUS_IOP() & STATUS_SDA) != 0;
}

extern xf86MonPtr ConfiguredMonitor;

static void
ApmProbeDDC(ScrnInfoPtr pScrn, int index)
{
    vbeInfoPtr pVbe;

    if (xf86LoadSubModule(pScrn, "vbe")) {
        pVbe = VBEInit(NULL, index);
        ConfiguredMonitor = vbeDoEDID(pVbe, NULL);
    }
}

static Bool
ApmPreInit(ScrnInfoPtr pScrn, int flags)
{
    APMDECL(pScrn);
    EntityInfoPtr	pEnt;
    MessageType		from;
    char		*mod = NULL, *req = NULL;
    ClockRangePtr	clockRanges;
    int			i;
    xf86MonPtr		MonInfo = NULL;
    double		real;

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

    /* Allocate the ApmRec driverPrivate */
    if (!ApmGetRec(pScrn)) {
	return FALSE;
    }
    pApm = APMPTR(pScrn);

    /* Get the entity */
    pEnt = pApm->pEnt	= xf86GetEntityInfo(pScrn->entityList[0]);
    if (pEnt->location.type == BUS_PCI) {
	pApm->PciInfo	= xf86GetPciInfoForEntity(pEnt->index);
	pApm->PciTag	= pciTag(pApm->PciInfo->bus, pApm->PciInfo->device,
				 pApm->PciInfo->func);
    }
    else {
	pApm->PciInfo	= NULL;
	pApm->PciTag	= 0;
    }

    if (flags & PROBE_DETECT) {
        ApmProbeDDC(pScrn, pEnt->index);
        return TRUE;
    }

    /* The vgahw module should be allocated here when needed */
    if (!xf86LoadSubModule(pScrn, "vgahw"))
	return FALSE;

    xf86LoaderReqSymLists(vgahwSymbols, NULL);

    /*
     * Allocate a vgaHWRec
     */
    if (!vgaHWGetHWRec(pScrn))
	return FALSE;

    vgaHWGetIOBase(VGAHWPTR(pScrn));

    /* Set pScrn->monitor */
    pScrn->monitor = pScrn->confScreen->monitor;

    /*
     * The first thing we should figure out is the depth, bpp, etc.
     */
    if (!xf86SetDepthBpp(pScrn, 0, 0, 0, Support24bppFb | Support32bppFb)) {
	return FALSE;
    } else {
	/* Check that the returned depth is one we support */
	switch (pScrn->depth) {
	case 4:
	case 8:
	case 15:
	case 16:
	case 24:
	    /* OK */
	    break;
	default:
	    xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		       "Given depth (%d) is not supported by this driver\n",
		       pScrn->depth);
	    return FALSE;
	}
    }

    /*
     * This must happen after pScrn->display has been set because
     * xf86SetWeight references it.
     */
    if (pScrn->depth > 8) {
	/* The defaults are OK for us */
	rgb zeros = {0, 0, 0};

	if (!xf86SetWeight(pScrn, zeros, zeros)) {
	    return FALSE;
	} else {
	    /* XXX check that weight returned is supported */
            ;
        }
    }

    if (!xf86SetDefaultVisual(pScrn, -1)) {
	return FALSE;
    } else {
	if (pScrn->depth > 8 && pScrn->defaultVisual != TrueColor) {
	    xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "Given default visual"
		       " (%s) is not supported at depth %d\n",
		       xf86GetVisualName(pScrn->defaultVisual), pScrn->depth);
	    return FALSE;
	}
    }

    /* We use a programamble clock */
    pScrn->progClock = TRUE;

    /* Collect all of the relevant option flags (fill in pScrn->options) */
    xf86CollectOptions(pScrn, NULL);

    /* Process the options */
    xf86ProcessOptions(pScrn->scrnIndex, pScrn->options, ApmOptions);

    pApm->scrnIndex = pScrn->scrnIndex;
    /* Set the bits per RGB for 8bpp mode */
    if (pScrn->depth > 1 && pScrn->depth <= 8) {
	/* Default to 8 */
	pScrn->rgbBits = 8;
    }
    if (xf86ReturnOptValBool(ApmOptions, OPTION_NOLINEAR, FALSE)) {
	pApm->noLinear = TRUE;
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "No linear framebuffer\n");
    }
    from = X_DEFAULT;
    pApm->hwCursor = FALSE;
    if (xf86GetOptValBool(ApmOptions, OPTION_HW_CURSOR, &pApm->hwCursor))
	from = X_CONFIG;
    if (pApm->noLinear ||
	xf86ReturnOptValBool(ApmOptions, OPTION_SW_CURSOR, FALSE)) {
	from = X_CONFIG;
	pApm->hwCursor = FALSE;
    }
    xf86DrvMsg(pScrn->scrnIndex, from, "Using %s cursor\n",
		pApm->hwCursor ? "HW" : "SW");
    from = X_DEFAULT;
    if (pScrn->bitsPerPixel < 8)
	pApm->NoAccel = TRUE;
    if (xf86ReturnOptValBool(ApmOptions, OPTION_NOACCEL, FALSE)) {
	from = X_CONFIG;
	pApm->NoAccel = TRUE;
    }
    if (pApm->NoAccel)
	xf86DrvMsg(pScrn->scrnIndex, from, "Acceleration disabled\n");
    if (xf86GetOptValFreq(ApmOptions, OPTION_SET_MCLK, OPTUNITS_MHZ, &real)) {
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "MCLK used is %.1f MHz\n", real);
	pApm->MemClk = (int)(real * 1000.0);
    }
    if (xf86ReturnOptValBool(ApmOptions, OPTION_SHADOW_FB, FALSE)) {
	pApm->ShadowFB = TRUE;
	pApm->NoAccel = TRUE;
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG,
		"Using \"Shadow Framebuffer\" - acceleration disabled\n");
    }
    if (xf86ReturnOptValBool(ApmOptions, OPTION_PCI_RETRY, FALSE)) {
	if (xf86ReturnOptValBool(ApmOptions, OPTION_PCI_BURST, FALSE)) {
	  pApm->UsePCIRetry = TRUE;
	  xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "PCI retry enabled\n");
	}
	else
	  xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "\"pci_retry\" option requires pci_burst \"on\".\n");
    }

    /*
     * Set the Chipset and ChipRev, allowing config file entries to
     * override.
     */
    if (pEnt->device->chipset && *pEnt->device->chipset) {
	pScrn->chipset = pEnt->device->chipset;
        pApm->Chipset = xf86StringToToken(ApmChipsets, pScrn->chipset);
        from = X_CONFIG;
    } else if (pEnt->device->chipID >= 0) {
	pApm->Chipset = pEnt->device->chipID;
	pScrn->chipset = (char *)xf86TokenToString(ApmChipsets, pApm->Chipset);

	from = X_CONFIG;
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "ChipID override: 0x%04X\n",
		   pApm->Chipset);
    } else {
	from = X_PROBED;
	if (pApm->PciInfo)
	    pApm->Chipset = pApm->PciInfo->chipType;
	else
	    pApm->Chipset = pEnt->chipset;
	pScrn->chipset = (char *)xf86TokenToString(ApmChipsets, pApm->Chipset);
    }
    if (pScrn->bitsPerPixel == 24 && pApm->Chipset < AT24) {
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		   "Given depth (%d) is not supported by this driver\n",
		   pScrn->depth);
	return FALSE;
    }
    if (pEnt->device->chipRev >= 0) {
	pApm->ChipRev = pEnt->device->chipRev;
	xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "ChipRev override: %d\n",
		   pApm->ChipRev);
    } else if (pApm->PciInfo) {
	pApm->ChipRev = pApm->PciInfo->chipRev;
    }

    /*
     * This shouldn't happen because such problems should be caught in
     * ApmProbe(), but check it just in case.
     */
    if (pScrn->chipset == NULL) {
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		   "ChipID 0x%04X is not recognised\n", pApm->Chipset);
	return FALSE;
    }
    if (pApm->Chipset < 0) {
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		   "Chipset \"%s\" is not recognised\n", pScrn->chipset);
	return FALSE;
    }

    xf86DrvMsg(pScrn->scrnIndex, from, "Chipset: \"%s\"\n", pScrn->chipset);

    if (pEnt->device->MemBase != 0) {
	pApm->LinAddress = pEnt->device->MemBase;
	from = X_CONFIG;
    } else if (pApm->PciInfo) {
	pApm->LinAddress = pApm->PciInfo->memBase[0] & 0xFF800000;
	from = X_PROBED;
    } else {
	/*
	 * VESA local bus.
	 * Pray that 2048MB works.
	 */
	pApm->LinAddress = 0x80000000;
    }

    xf86DrvMsg(pScrn->scrnIndex, from, "Linear framebuffer at 0x%lX\n",
	       (unsigned long)pApm->LinAddress);

    if (xf86LoadSubModule(pScrn, "ddc")) {
	xf86LoaderReqSymLists(ddcSymbols, NULL);
	if (xf86LoadSubModule(pScrn, "i2c")) {
	    xf86LoaderReqSymLists(i2cSymbols, NULL);
	    pApm->I2C = TRUE;
	}
    }

    if (pApm->noLinear) {
	/*
	 * TODO not AT3D.
	 * XXX ICI XXX
	 */
	pApm->LinMapSize      =  4 * 1024 * 1024 /* 0x10000 */;
	pApm->FbMapSize       =  4 * 1024 * 1024 /* 0x10000 */;
	if (pApm->Chipset >= AT3D)
	    pApm->LinAddress +=  8 * 1024 * 1024 /* 0xA0000 */;
    }
    else {
	if (pApm->Chipset >= AT3D)
	    pApm->LinMapSize  = 16 * 1024 * 1024;
	else
	    pApm->LinMapSize  =  6 * 1024 * 1024;
	pApm->FbMapSize   =  4 * 1024 * 1024;
    }

    if (xf86LoadSubModule(pScrn, "int10")) {
	void	*ptr;

	xf86DrvMsg(pScrn->scrnIndex,X_INFO,"initializing int10\n");
	ptr = xf86InitInt10(pEnt->index);
	if (ptr)
	    xf86FreeInt10(ptr);
    }

    xf86RegisterResources(pEnt->index, NULL, ResNone);
    xf86SetOperatingState(RES_SHARED_VGA, pEnt->index, ResDisableOpr);
    pScrn->racMemFlags = 0;	/* For noLinear, access to 0xA0000 */
    if (pApm->VGAMap)
	pScrn->racIoFlags = 0;
    else
	pScrn->racIoFlags = RAC_COLORMAP | RAC_VIEWPORT;

    if (pEnt->device->videoRam != 0) {
	pScrn->videoRam = pEnt->device->videoRam;
	from = X_CONFIG;
    } else if (!pApm->noLinear && pApm->Chipset >= AT3D) {
	unsigned char		d9, db, uc;
	/*unsigned long		save;*/
	volatile unsigned char	*LinMap;

	LinMap = xf86MapPciMem(pScrn->scrnIndex, VIDMEM_MMIO,
				     pApm->PciTag, pApm->LinAddress,
				     pApm->LinMapSize);
	/*save = pciReadLong(pApm->PciTag, PCI_CMD_STAT_REG);
	pciWriteLong(pApm->PciTag, PCI_CMD_STAT_REG, save | PCI_CMD_MEM_ENABLE);*/
	d9 = LinMap[0xFFECD9];
	db = LinMap[0xFFECDB];
	LinMap[0xFFECDB] = (db & 0xF4) | 0x0A;
	LinMap[0xFFECD9] = (d9 & 0xCF) | 0x20;
	LinMap[0xFFF3C4] = 0x1C;
	uc = LinMap[0xFFF3C5];
	LinMap[0xFFF3C5] = 0x3F;
	LinMap[0xFFF3C4] = 0x20;
	pScrn->videoRam = LinMap[0xFFF3C5] * 64;
	LinMap[0xFFF3C4] = 0x10;
	pApm->savedSR10 = LinMap[0xFFF3C5];
	LinMap[0xFFF3C4] = 0x1E;
	pApm->xbase  = LinMap[0xFFF3C5];
	LinMap[0xFFF3C4] = 0x1F;
	pApm->xbase |= LinMap[0xFFF3C5] << 8;
	LinMap[0xFFF3C4] = 0x1C;
	LinMap[0xFFF3C5] = uc;
	LinMap[0xFFECDB] = db;
	LinMap[0xFFECD9] = d9;
	/*pciWriteLong(pApm->PciTag, PCI_CMD_STAT_REG, save);*/
	xf86UnMapVidMem(pScrn->scrnIndex, (pointer)LinMap, pApm->LinMapSize);
	from = X_PROBED;
    }
    else {
	/*unsigned long		save;

	save = pciReadLong(pApm->PciTag, PCI_CMD_STAT_REG);
	pciWriteLong(pApm->PciTag, PCI_CMD_STAT_REG, save | PCI_CMD_IO_ENABLE);*/
	pApm->savedSR10 = rdinx(0x3C4, 0x10);
	wrinx(0x3C4, 0x10, 0x12);
	pScrn->videoRam = rdinx(0x3C4, 0x20) * 64;
	pApm->xbase = (rdinx(0x3C4, 0x1F) << 8) | rdinx(0x3C4, 0x1E);
	wrinx(0x3C4, 0x10, pApm->savedSR10 ? 0 : 0x12);
	/*pciWriteLong(pApm->PciTag, PCI_CMD_STAT_REG, save);*/
	from = X_PROBED;
    }
    if (pApm->Chipset < AT3D && pScrn->videoRam >= 4096)
	pScrn->videoRam -= 32;

    xf86DrvMsg(pScrn->scrnIndex, from, "VideoRAM: %d kByte\n",
               pScrn->videoRam);

    if (!xf86IsPc98()) {
	vgaHWGetIOBase(VGAHWPTR(pScrn));
	VGAHWPTR(pScrn)->MapSize = 0x10000;
	vgaHWMapMem(pScrn);
	if (pApm->I2C) {
	    if (!ApmI2CInit(pScrn)) {
		xf86DrvMsg(pScrn->scrnIndex,X_ERROR,"I2C initialization failed\n");
	    }
	    else {
		MonInfo = xf86DoEDID_DDC2(pScrn->scrnIndex,pApm->I2CPtr);
	    }
	}
	if (!MonInfo)
	    MonInfo = xf86DoEDID_DDC1(pScrn->scrnIndex,vgaHWddc1SetSpeed,ddc1Read);
	if (MonInfo)
	    xf86PrintEDID(MonInfo);
	pScrn->monitor->DDC = MonInfo;
    }

    /* The gamma fields must be initialised when using the new cmap code */
    if (pScrn->depth > 1) {
	Gamma zeros = {0.0, 0.0, 0.0};

	if (!xf86SetGamma(pScrn, zeros)) {
	    return FALSE;
	}
    }

    pApm->MinClock = 23125;
    xf86DrvMsg(pScrn->scrnIndex, X_DEFAULT, "Min pixel clock set to %d MHz\n",
	       pApm->MinClock / 1000);

    /*
     * If the user has specified ramdac speed in the XF86Config
     * file, we respect that setting.
     */
    from = X_DEFAULT;
    if (pEnt->device->dacSpeeds[0]) {
	int speed = 0;

	switch (pScrn->bitsPerPixel) {
	case 4:
	case 8:
	   speed = pEnt->device->dacSpeeds[DAC_BPP8];
	   break;
	case 16:
	   speed = pEnt->device->dacSpeeds[DAC_BPP16];
	   break;
	case 24:
	   speed = pEnt->device->dacSpeeds[DAC_BPP24];
	   break;
	case 32:
	   speed = pEnt->device->dacSpeeds[DAC_BPP32];
	   break;
	}
	if (speed == 0)
	    pApm->MaxClock = pEnt->device->dacSpeeds[0];
	else
	    pApm->MaxClock = speed;
	from = X_CONFIG;
    } else {
	switch(pApm->Chipset)
	{
	  /* These values come from the Manual for AT24 and AT3D 
	     in the overview of various modes. I've taken the largest
	     number for the different modes. Alliance wouldn't 
	     tell me what the maximum frequency was, so...
	   */
	  case AT24:
	       switch(pScrn->bitsPerPixel)
	       {
		 case 4:
		 case 8:
		      pApm->MaxClock = 160000;
		      break;
		 case 16:
		      pApm->MaxClock = 144000;
		      break;
		 case 24:
		      pApm->MaxClock = 75000; /* Hmm. */
		      break;
		 case 32:
		      pApm->MaxClock = 94500;
		      break;
		 default:
		      return FALSE;
	       }
	       break;
	  case AT3D:
	       switch(pScrn->bitsPerPixel)
	       {
		 case 4:
		 case 8:
		      pApm->MaxClock = 175500;
		      break;
		 case 16:
		      pApm->MaxClock = 144000;
		      break;
		 case 24:
		      pApm->MaxClock = 94000; /* Changed from 75000 by Greni� */
		      break;
		 case 32:
		      pApm->MaxClock = 94500;
		      break;
		 default:
		      return FALSE;
	       }
	       break;
	  case AP6422:
	       switch(pScrn->bitsPerPixel)
	       {
		 case 4:
		 case 8:
		      pApm->MaxClock = 135000;
		      break;
		 case 16:
		      pApm->MaxClock = 75000;
		      break;
		 case 32:
		      pApm->MaxClock = 60000;
		      break;
		 default:
		      return FALSE;
	       }
	       break;
	  default:
	       pApm->MaxClock = 135000;
	       break;
	}
    }
    xf86DrvMsg(pScrn->scrnIndex, from, "Max pixel clock is %d MHz\n",
	       pApm->MaxClock / 1000);

    /*
     * Setup the ClockRanges, which describe what clock ranges are available,
     * and what sort of modes they can be used for.
     */
    clockRanges = (ClockRangePtr)xnfcalloc(sizeof(ClockRange), 1);
    clockRanges->next = NULL;
    clockRanges->minClock = pApm->MinClock;
    clockRanges->maxClock = pApm->MaxClock;
    clockRanges->clockIndex = -1;		/* programmable */
    clockRanges->interlaceAllowed = FALSE;	/* XXX change this */
    clockRanges->doubleScanAllowed = FALSE;	/* XXX check this */

    /* Select valid modes from those available */
    if (pApm->NoAccel) {
	/*
	 * XXX Assuming min pitch 256, max 2048
	 * XXX Assuming min height 128, max 2048
	 */
	i = xf86ValidateModes(pScrn, pScrn->monitor->Modes,
			      pScrn->display->modes, clockRanges,
			      NULL, 256, 2048,
			      pScrn->bitsPerPixel, 128, 2048,
			      pScrn->display->virtualX,
			      pScrn->display->virtualY,
			      pApm->FbMapSize,
			      LOOKUP_BEST_REFRESH);
    } else {
	/*
	 * XXX Assuming min height 128, max 2048
	 */
	i = xf86ValidateModes(pScrn, pScrn->monitor->Modes,
			      pScrn->display->modes, clockRanges,
			      GetAccelPitchValues(pScrn), 0, 0,
			      pScrn->bitsPerPixel, 128, 2048,
			      pScrn->display->virtualX,
			      pScrn->display->virtualY,
			      pApm->FbMapSize,
			      LOOKUP_BEST_REFRESH);
    }

    if (i == -1) {
	ApmFreeRec(pScrn);
	return FALSE;
    }

    /* Prune the modes marked as invalid */
    xf86PruneDriverModes(pScrn);

    if (i == 0 || pScrn->modes == NULL) {
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "No valid modes found\n");
	ApmFreeRec(pScrn);
	return FALSE;
    }

    xf86SetCrtcForModes(pScrn, INTERLACE_HALVE_V);

    /* Set the current mode to the first in the list */
    pScrn->currentMode = pScrn->modes;

    /* Print the list of modes being used */
    xf86PrintModes(pScrn);

    /* Set display resolution */
    xf86SetDpi(pScrn, 0, 0);

    /* Load bpp-specific modules */
    switch (pScrn->bitsPerPixel) {
    case 1:
	mod = "xf1bpp";
	req = "xf1bppScreenInit";
	break;
    case 4:
	mod = "xf4bpp";
	req = "xf4bppScreenInit";
	break;
    case 8:
	mod = "cfb";
	req = "cfbScreenInit";
	break;
    case 16:
	mod = "cfb16";
	req = "cfb16ScreenInit";
	break;
    case 24:
	if (pix24bpp == 24) {
	    mod = "cfb24";
	    req = "cfb24ScreenInit";
	} else {
	    mod = "xf24_32bpp";
	    req = "cfb24_32ScreenInit";
	}
	break;
    case 32:
	mod = "cfb32";
	req = "cfb32ScreenInit";
	break;
    }

    if (mod && xf86LoadSubModule(pScrn, mod) == NULL) {
	ApmFreeRec(pScrn);
	return FALSE;
    }

    xf86LoaderReqSymbols(req, NULL);

    /* Load XAA if needed */
    if (!pApm->NoAccel) {
	if (!xf86LoadSubModule(pScrn, "xaa")) {
	    ApmFreeRec(pScrn);
	    return FALSE;
	}
	xf86LoaderReqSymLists(xaaSymbols, NULL);
    }

    /* Load ramdac if needed */
    if (pApm->hwCursor) {
	if (!xf86LoadSubModule(pScrn, "ramdac")) {
	    ApmFreeRec(pScrn);
	    return FALSE;
	}
	xf86LoaderReqSymLists(ramdacSymbols, NULL);
    }

    /* Load shadowfb if needed */
    if (pApm->ShadowFB) {
	if (!xf86LoadSubModule(pScrn, "shadowfb")) {
	    ApmFreeRec(pScrn);
	    return FALSE;
	}
	xf86LoaderReqSymLists(shadowSymbols, NULL);
    }

    pApm->CurrentLayout.displayWidth	= pScrn->virtualX;
    pApm->CurrentLayout.displayHeight	= pScrn->virtualY;
    pApm->CurrentLayout.bitsPerPixel	= pScrn->bitsPerPixel;
    pApm->CurrentLayout.bytesPerScanline= (pApm->CurrentLayout.displayWidth * pApm->CurrentLayout.bitsPerPixel) >> 3;
    pApm->CurrentLayout.depth		= pScrn->depth;
    pApm->CurrentLayout.Scanlines	= 2 * (pScrn->videoRam << 10) / pApm->CurrentLayout.bytesPerScanline;
    if (pScrn->bitsPerPixel == 24)
	pApm->CurrentLayout.mask32	= 3;
    else
	pApm->CurrentLayout.mask32	= 32 / pScrn->bitsPerPixel - 1;

    return TRUE;
}

/*
 * Map the framebuffer and MMIO memory.
 */

static Bool
ApmMapMem(ScrnInfoPtr pScrn)
{
    APMDECL(pScrn);
    vgaHWPtr	hwp = VGAHWPTR(pScrn);

    pApm->LinMap = xf86MapPciMem(pScrn->scrnIndex, VIDMEM_FRAMEBUFFER,
				 pApm->PciTag,
				 (unsigned long)pApm->LinAddress,
				 pApm->LinMapSize);
    if (pApm->LinMap == NULL)
	return FALSE;

    if (!pApm->noLinear) {
	if (pApm->Chipset >= AT3D) {
	    pApm->FbBase = (void *)(((char *)pApm->LinMap) + 0x800000);
	    pApm->VGAMap = ((char *)pApm->LinMap) + 0xFFF000;
	    pApm->MemMap = ((char *)pApm->LinMap) + 0xFFEC00;
	    pApm->BltMap = (void *)(((char *)pApm->LinMap) + 0x3F8000);
	}
	else {
	    pApm->FbBase = (void *)pApm->LinMap;
	    pApm->VGAMap = NULL;
	    if (pScrn->videoRam == 6 * 1024 - 32) {
		pApm->MemMap = ((char *)pApm->LinMap) + 0x5FF800;
		pApm->BltMap = (void *)(((char *)pApm->LinMap) + 0x5F8000);
	    }
	    else {
		pApm->MemMap = ((char *)pApm->LinMap) + 0x3FF800;
		pApm->BltMap = (void *)(((char *)pApm->LinMap) + 0x3F8000);
	    }
	}

	/*
	 * Initialize chipset
	 */
	pApm->c9 = RDXB(0xC9);
	if (pApm->Chipset >= AT3D) {
	    pApm->d9 = RDXB(0xD9);
	    pApm->db = RDXB(0xDB);

	    /* If you change these two, change them also in apm_funcs.c */
	    WRXB(0xDB, (pApm->db & 0xF4) | 0x0A);
	    WRXB(0xD9, (pApm->d9 & 0xCF) | 0x20);

	    vgaHWSetMmioFuncs(hwp, (CARD8 *)pApm->LinMap, 0xFFF000);
	}
	if (pApm->Chipset >= AP6422)
	    WRXB(0xC9, pApm->c9 | 0x10);
    }
    else {
	pApm->FbBase = pApm->LinMap;

	/*
	 * Initialize chipset
	 */
	if (pApm->Chipset >= AT3D) {
	    pApm->d9 = RDXB_IOP(0xD9);
	    pApm->db = RDXB_IOP(0xDB);
	    WRXB_IOP(0xDB, pApm->db & 0xF4);
	}
    }
    /*
     * Save color mode
     */
    pApm->MiscOut = hwp->readMiscOut(hwp);

    return TRUE;
}

/*
 * Unmap the framebuffer and MMIO memory
 */

static Bool
ApmUnmapMem(ScrnInfoPtr pScrn)
{
    APMDECL(pScrn);
    vgaHWPtr	hwp = VGAHWPTR(pScrn);

    /*
     * Reset color mode
     */
    hwp->writeMiscOut(hwp, pApm->MiscOut);
    if (pApm->LinMap) {
	if (pApm->Chipset >= AT3D) {
	    if (!pApm->noLinear) {
		WRXB(0xD9, pApm->d9);
		WRXB(0xDB, pApm->db);
	    }
	    else {
		WRXB_IOP(0xD9, pApm->d9);
		WRXB_IOP(0xDB, pApm->db);
	    }
	}
	WRXB(0xC9, pApm->c9);
	xf86UnMapVidMem(pScrn->scrnIndex, (pointer)pApm->LinMap, pApm->LinMapSize);
	pApm->LinMap = NULL;
    }
    else if (pApm->FbBase)
	xf86UnMapVidMem(pScrn->scrnIndex, (pointer)pApm->LinMap, 0x10000);

    return TRUE;
}

/*
 * This function saves the video state.
 */
static void
ApmSave(ScrnInfoPtr pScrn)
{
    APMDECL(pScrn);
    ApmRegPtr	ApmReg = &pApm->SavedReg;
    vgaHWPtr	vgaHWP = VGAHWPTR(pScrn);

    if (pApm->VGAMap) {
	ApmReg->SEQ[0x1B] = ApmReadSeq(0x1B);
	ApmReg->SEQ[0x1C] = ApmReadSeq(0x1C);

	/*
	 * Save fonts
	 */
	if (!(vgaHWP->SavedReg.Attribute[0x10] & 1)) {
	    if (pApm->FontInfo || (pApm->FontInfo = (pointer)xalloc(TEXT_AMOUNT))) {
		int locked;

		locked = ApmReadSeq(0x10);
		if (locked)
		    ApmWriteSeq(0x10, 0x12);
		ApmWriteSeq(0x1C, 0x3F);
		memcpy(pApm->FontInfo, pApm->FbBase, TEXT_AMOUNT);
		ApmWriteSeq(0x1C, ApmReg->SEQ[0x1C]);
		if (locked)
		    ApmWriteSeq(0x10, 0);
	    }
	}
	/*
	 * This function will handle creating the data structure and filling
	 * in the generic VGA portion.
	 */
	vgaHWSave(pScrn, &vgaHWP->SavedReg, VGA_SR_MODE | VGA_SR_CMAP);

	/* Hardware cursor registers. */
	ApmReg->EX[XR140] = RDXL(0x140);
	ApmReg->EX[XR144] = RDXW(0x144);
	ApmReg->EX[XR148] = RDXL(0x148);
	ApmReg->EX[XR14C] = RDXW(0x14C);

	ApmReg->CRT[0x19] = ApmReadCrtc(0x19);
	ApmReg->CRT[0x1A] = ApmReadCrtc(0x1A);
	ApmReg->CRT[0x1B] = ApmReadCrtc(0x1B);
	ApmReg->CRT[0x1C] = ApmReadCrtc(0x1C);
	ApmReg->CRT[0x1D] = ApmReadCrtc(0x1D);
	ApmReg->CRT[0x1E] = ApmReadCrtc(0x1E);

	/* RAMDAC registers. */
	ApmReg->EX[XRE8] = RDXL(0xE8);
	ApmReg->EX[XREC] = RDXL(0xEC);

	/* Color correction */
	ApmReg->EX[XRE0] = RDXL(0xE0);

	ApmReg->EX[XR80] = RDXB(0x80);
    }
    else {
	/*
	 * This function will handle creating the data structure and filling
	 * in the generic VGA portion.
	 */
	vgaHWSave(pScrn, &vgaHWP->SavedReg, VGA_SR_ALL);

	ApmReg->SEQ[0x1B] = rdinx(0x3C4, 0x1B);
	ApmReg->SEQ[0x1C] = rdinx(0x3C4, 0x1C);

	/* Hardware cursor registers. */
	if (pApm->noLinear) {
	    ApmReg->EX[XR140] = RDXL_IOP(0x140);
	    ApmReg->EX[XR144] = RDXW_IOP(0x144);
	    ApmReg->EX[XR148] = RDXL_IOP(0x148);
	    ApmReg->EX[XR14C] = RDXW_IOP(0x14C);
	}
	else {
	    ApmReg->EX[XR140] = RDXL(0x140);
	    ApmReg->EX[XR144] = RDXW(0x144);
	    ApmReg->EX[XR148] = RDXL(0x148);
	    ApmReg->EX[XR14C] = RDXW(0x14C);
	}

	ApmReg->CRT[0x19] = rdinx(0x3D4, 0x19);
	ApmReg->CRT[0x1A] = rdinx(0x3D4, 0x1A);
	ApmReg->CRT[0x1B] = rdinx(0x3D4, 0x1B);
	ApmReg->CRT[0x1C] = rdinx(0x3D4, 0x1C);
	ApmReg->CRT[0x1D] = rdinx(0x3D4, 0x1D);
	ApmReg->CRT[0x1E] = rdinx(0x3D4, 0x1E);

	if (pApm->noLinear) {
	    /* RAMDAC registers. */
	    ApmReg->EX[XRE8] = RDXL_IOP(0xE8);
	    ApmReg->EX[XREC] = RDXL_IOP(0xEC);

	    /* Color correction */
	    ApmReg->EX[XRE0] = RDXL_IOP(0xE0);

	    ApmReg->EX[XR80] = RDXB_IOP(0x80);
	}
	else {
	    /* RAMDAC registers. */
	    ApmReg->EX[XRE8] = RDXL(0xE8);
	    ApmReg->EX[XREC] = RDXL(0xEC);

	    /* Color correction */
	    ApmReg->EX[XRE0] = RDXL(0xE0);

	    ApmReg->EX[XR80] = RDXB(0x80);
	}
    }
}

#define WITHIN(v,c1,c2) (((v) >= (c1)) && ((v) <= (c2)))

static unsigned
comp_lmn(ApmPtr pApm, long clock)
{
  int     n, m, l, f;
  double  fvco;
  double  fout;
  double  fmax, fmin;
  double  fref;
  double  fvco_goal;
  double  k, c;

  if (pApm->Chipset >= AT3D)
    fmax = 370000.0;
  else
    fmax = 250000.0;

  fref = 14318.0;
  fmin = fmax / 2.0;

  for (m = 1; m <= 5; m++)
  {
    for (l = 3; l >= 0; l--)
    {
      for (n = 8; n <= 127; n++)
      {
        fout = ((double)(n + 1) * fref)/((double)(m + 1) * (1 << l));
        fvco_goal = (double)clock * (double)(1 << l);
        fvco = fout * (double)(1 << l);
        if (!WITHIN(fvco, 0.995*fvco_goal, 1.005*fvco_goal))
          continue;
        if (!WITHIN(fvco, fmin, fmax))
          continue;
        if (!WITHIN(fvco / (double)(n+1), 300.0, 300000.0))
          continue;
        if (!WITHIN(fref / (double)(m+1), 300.0, 300000.0))
          continue;

        /* The following formula was empirically derived by
           matching a number of fvco values with acceptable
           values of f.

           (fvco can be 185MHz - 370MHz on AT3D)
           (fvco can be 125MHz - 250MHz on AT24/AP6422)

           The table that was measured up follows:

           AT3D

           fvco       f
           (125)     (x-7) guess
           200       5-7
           219       4-7
           253       3-6
           289       2-5
           320       0-4
           (400)     (0-x) guess

           AT24

           fvco       f
           126       7
           200       5-7
           211       4-7

           AP6422

           fvco       f
           126       7
           169       5-7
           200       4-5
           211       4-5

           From this, a function "f = k * fvco + c" was derived.

           For AT3D, this table was measured with MCLK == 50MHz.
           The driver has since been set to use MCLK == 57.3MHz for,
           but I don't think that makes a difference here.
         */

        if (pApm->Chipset >= AT24)
        {
          k = 7.0 / (175.0 - 380.0);
          c = -k * 380.0;
          f = (int)(k * fvco/1000.0 + c + 0.5);
          if (f > 7) f = 7;
          if (f < 0) f = 0;
        }

        if (pApm->Chipset < AT24) /* i.e AP6422 */
        {
          c = (211.0*6.0-169.0*4.5)/(211.0-169.0);
          k = (4.5-c)/211.0;
          f = (int)(k * fvco/1000.0 + c + 0.5);
          if (f > 7) f = 7;
          if (f < 0) f = 0;
        }

        return (n << 16) | (m << 8) | (l << 2) | (f << 4);
      }
    }
  }
  xf86DrvMsg(pApm->scrnIndex, X_PROBED,
		"Cannot find register values for clock %6.2f MHz. "
		"Please use a (slightly) different clock.\n",
		 (double)clock / 1000.0);
  return 0;
}

/*
 * Initialise a new mode.  This is currently still using the old
 * "initialise struct, restore/write struct to HW" model.  That could
 * be changed.
 */

static Bool
ApmModeInit(ScrnInfoPtr pScrn, DisplayModePtr mode)
{
    APMDECL(pScrn);
    ApmRegPtr	ApmReg = &pApm->ModeReg;
    vgaHWPtr	hwp;


    /* set clockIndex to "2" for programmable clocks */
    if (pScrn->progClock)
	mode->ClockIndex = 2;

    /* prepare standard VGA register contents */
    if (!vgaHWInit(pScrn, mode))
	return FALSE;
    pScrn->vtSema = TRUE;
    hwp = VGAHWPTR(pScrn);

    hwp->writeMiscOut(hwp, pApm->MiscOut | 0x0F);

    if (xf86IsPc98())
       outb(0xFAC, 0xFF);

    memcpy(ApmReg, &pApm->SavedReg, sizeof pApm->SavedReg);

    /*
     * The APM chips have a scale factor of 8 for the
     * scanline offset. There are four extended bit in addition
     * to the 8 VGA bits.
     */
    {
	int offset;

	offset = (pApm->CurrentLayout.displayWidth *
		  pApm->CurrentLayout.bitsPerPixel / 8)	>> 3;
	hwp->ModeReg.CRTC[0x13] = offset;
	/* Bit 8 resides at CR1C bits 7:4. */
	ApmReg->CRT[0x1C] = (offset & 0xF00) >> 4;
    }

    /* Set pixel depth. */
    switch(pApm->CurrentLayout.bitsPerPixel)
    {
    case 4:
	 ApmReg->EX[XR80] = 0x01;
	 break;
    case 8:
	 ApmReg->EX[XR80] = 0x02;
	 break;
    case 16:
	 if (pApm->CurrentLayout.depth == 15)
	     ApmReg->EX[XR80] = 0x0C;
	 else
	     ApmReg->EX[XR80] = 0x0D;
	 break;
    case 24:
	 ApmReg->EX[XR80] = 0x0E;
	 break;
    case 32:
	 ApmReg->EX[XR80] = 0x0F;
	 break;
    default:
	 FatalError("Unsupported bit depth %d\n", pApm->CurrentLayout.depth);
	 break;
    }

    /* Set banking register to zero. */
    ApmReg->EX[XRC0] = 0;

    /* Handle the CRTC overflow bits. */
    {
	unsigned char val;
	/* Vertical Overflow. */
	val = 0;
	if ((mode->CrtcVTotal - 2) & 0x400)
	    val |= 0x01;
	if ((mode->CrtcVDisplay - 1) & 0x400)
	    val |= 0x02;
	/* VBlankStart is equal to VSyncStart + 1. */
	if (mode->CrtcVSyncStart & 0x400)
	    val |= 0x04;
	/* VRetraceStart is equal to VSyncStart + 1. */
	if (mode->CrtcVSyncStart & 0x400)
	    val |= 0x08;
	ApmReg->CRT[0x1A] = val;

	/* Horizontal Overflow. */
	val = 0;
	if ((mode->CrtcHTotal / 8 - 5) & 0x100)
	    val |= 1;
	if ((mode->CrtcHDisplay / 8 - 1) & 0x100)
	    val |= 2;
	/* HBlankStart is equal to HSyncStart - 1. */
	if ((mode->CrtcHSyncStart / 8 - 1) & 0x100)
	    val |= 4;
	/* HRetraceStart is equal to HSyncStart. */
	if ((mode->CrtcHSyncStart / 8) & 0x100)
	    val |= 8;
	ApmReg->CRT[0x1B] = val;

	/* Assume the CRTC is not KGA (see vgaHWInit) */
	hwp->ModeReg.CRTC[3] = (hwp->ModeReg.CRTC[3] & 0xE0) |
				(((mode->CrtcHBlankEnd >> 3) - 1) & 0x1F);
	hwp->ModeReg.CRTC[5]  = ((((mode->CrtcHBlankEnd >> 3) - 1) & 0x20) << 2)
				| (hwp->ModeReg.CRTC[5] & 0x7F);
	hwp->ModeReg.CRTC[22] = (mode->CrtcVBlankEnd - 1) & 0xFF;
    }
    ApmReg->CRT[0x1E] = 1;          /* disable autoreset feature */

    /* Program clock select. */
    ApmReg->EX[XREC] = comp_lmn(pApm, mode->Clock);
    if (!ApmReg->EX[XREC])
      return FALSE;
    hwp->ModeReg.MiscOutReg |= 0x0C;

    /* Set up the RAMDAC registers. */

    if (pApm->CurrentLayout.bitsPerPixel > 8)
	/* Get rid of white border. */
	hwp->ModeReg.Attribute[0x11] = 0x00;
    else
	hwp->ModeReg.Attribute[0x11] = 0xFF;
    if (pApm->MemClk)
	ApmReg->EX[XRE8] = comp_lmn(pApm, pApm->MemClk);
    else if (pApm->Chipset >= AT3D)
	ApmReg->EX[XRE8] = 0x071F01E8; /* Enable 58MHz MCLK (actually 57.3 MHz)
				       This is what is used in the Windows
				       drivers. The BIOS sets it to 50MHz. */
    else if (!pApm->noLinear)
	ApmReg->EX[XRE8] = RDXL(0xE8); /* No change */
    else
	ApmReg->EX[XRE8] = RDXL_IOP(0xE8); /* No change */

    ApmReg->EX[XRE0] = 0x10;

    /* If you change it, change in apm_funcs.c as well */
    if (pApm->Chipset >= AT3D) {
	ApmReg->SEQ[0x1B] = 0x20;
	ApmReg->SEQ[0x1C] = 0x2F;
    }
    else {
	ApmReg->SEQ[0x1B] = 0x24;
	if (pScrn->videoRam >= 6 * 1024)
	    ApmReg->SEQ[0x1C] = 0x2F;
	else
	    ApmReg->SEQ[0x1C] = 0x2D;
    }

    /* ICICICICI */
    ApmRestore(pScrn, &hwp->ModeReg, ApmReg);

    return TRUE;
}

/*
 * Restore the initial mode.
 */
static void
ApmRestore(ScrnInfoPtr pScrn, vgaRegPtr vgaReg, ApmRegPtr ApmReg)
{
    APMDECL(pScrn);

    vgaHWProtect(pScrn, TRUE);
    ApmUnlock(pApm);

    if (pApm->VGAMap) {
	/*
	 * Restore fonts
	 */
	if (!(vgaReg->Attribute[0x10] & 1) && pApm->FontInfo) {
	    ApmWriteSeq(0x1C, 0x3F);
	    memcpy(pApm->FbBase, pApm->FontInfo, TEXT_AMOUNT);
	}

	/* Set aperture index to 0. */
	WRXW(0xC0, 0);

	/*
	 * Write the extended registers first
	 */
	ApmWriteSeq(0x1B, ApmReg->SEQ[0x1B]);
	ApmWriteSeq(0x1C, ApmReg->SEQ[0x1C]);

	/* Hardware cursor registers. */
	WRXL(0x140, ApmReg->EX[XR140]);
	WRXW(0x144, ApmReg->EX[XR144]);
	WRXL(0x148, ApmReg->EX[XR148]);
	WRXW(0x14C, ApmReg->EX[XR14C]);

	ApmWriteCrtc(0x19, ApmReg->CRT[0x19]);
	ApmWriteCrtc(0x1A, ApmReg->CRT[0x1A]);
	ApmWriteCrtc(0x1B, ApmReg->CRT[0x1B]);
	ApmWriteCrtc(0x1C, ApmReg->CRT[0x1C]);
	ApmWriteCrtc(0x1D, ApmReg->CRT[0x1D]);
	ApmWriteCrtc(0x1E, ApmReg->CRT[0x1E]);

	/* RAMDAC registers. */
	WRXL(0xE8, ApmReg->EX[XRE8]);
	WRXL(0xEC, ApmReg->EX[XREC] & ~(1 << 7));
	WRXL(0xEC, ApmReg->EX[XREC] | (1 << 7)); /* Do a PLL resync */

	/* Color correction */
	WRXL(0xE0, ApmReg->EX[XRE0]);

	WRXB(0x80, ApmReg->EX[XR80]);

	/*
	 * This function handles restoring the generic VGA registers.
	 */
	vgaHWRestore(pScrn, vgaReg, VGA_SR_MODE | VGA_SR_CMAP);
    }
    else {
	/* Set aperture index to 0. */
	if (pApm->noLinear)
	    WRXW_IOP(0xC0, 0);
	else
	    WRXW(0xC0, 0);

	/*
	 * Write the extended registers first
	 */
	wrinx(0x3C4, 0x1B, ApmReg->SEQ[0x1B]);
	wrinx(0x3C4, 0x1C, ApmReg->SEQ[0x1C]);

	/* Hardware cursor registers. */
	if (pApm->noLinear) {
	    WRXL_IOP(0x140, ApmReg->EX[XR140]);
	    WRXW_IOP(0x144, ApmReg->EX[XR144]);
	    WRXL_IOP(0x148, ApmReg->EX[XR148]);
	    WRXW_IOP(0x14C, ApmReg->EX[XR14C]);
	}
	else {
	    WRXL(0x140, ApmReg->EX[XR140]);
	    WRXW(0x144, ApmReg->EX[XR144]);
	    WRXL(0x148, ApmReg->EX[XR148]);
	    WRXW(0x14C, ApmReg->EX[XR14C]);
	}

	wrinx(0x3D4, 0x19, ApmReg->CRT[0x19]);
	wrinx(0x3D4, 0x1A, ApmReg->CRT[0x1A]);
	wrinx(0x3D4, 0x1B, ApmReg->CRT[0x1B]);
	wrinx(0x3D4, 0x1C, ApmReg->CRT[0x1C]);
	wrinx(0x3D4, 0x1D, ApmReg->CRT[0x1D]);
	wrinx(0x3D4, 0x1E, ApmReg->CRT[0x1E]);

	/* RAMDAC registers. */
	if (pApm->noLinear) {
	    WRXL_IOP(0xE8, ApmReg->EX[XRE8]);
	    WRXL_IOP(0xEC, ApmReg->EX[XREC] & ~(1 << 7));
	    WRXL_IOP(0xEC, ApmReg->EX[XREC] | (1 << 7)); /* Do a PLL resync */
	}
	else {
	    WRXL(0xE8, ApmReg->EX[XRE8]);
	    WRXL(0xEC, ApmReg->EX[XREC] & ~(1 << 7));
	    WRXL(0xEC, ApmReg->EX[XREC] | (1 << 7)); /* Do a PLL resync */
	}

	/* Color correction */
	if (pApm->noLinear)
	    WRXL_IOP(0xE0, ApmReg->EX[XRE0]);
	else
	    WRXL(0xE0, ApmReg->EX[XRE0]);

	if (pApm->noLinear)
	    WRXB_IOP(0x80, ApmReg->EX[XR80]);
	else
	    WRXB(0x80, ApmReg->EX[XR80]);

	/*
	 * This function handles restoring the generic VGA registers.
	 */
	vgaHWRestore(pScrn, vgaReg, VGA_SR_ALL);
    }

    vgaHWProtect(pScrn, FALSE);
}


/* Refresh a region of the shadow framebuffer to the screen */
static void
ApmRefreshArea(ScrnInfoPtr pScrn, int num, BoxPtr pbox)
{
    APMDECL(pScrn);
    int width, height, Bpp, FBPitch;
    unsigned char *src, *dst;

    Bpp = pApm->CurrentLayout.bitsPerPixel >> 3;
    FBPitch = pApm->CurrentLayout.bytesPerScanline;

    while(num--) {
	width = (pbox->x2 - pbox->x1) * Bpp;
	height = pbox->y2 - pbox->y1;
	src = pApm->ShadowPtr + (pbox->y1 * pApm->ShadowPitch) +
						(pbox->x1 * Bpp);
	dst = (unsigned char *)pApm->FbBase + (pbox->y1 * FBPitch) + (pbox->x1 * Bpp);

	while(height--) {
	    memcpy(dst, src, width);
	    dst += FBPitch;
	    src += pApm->ShadowPitch;
	}

	pbox++;
    }
}


/* Mandatory */

/* This gets called at the start of each server generation */

static Bool
ApmScreenInit(int scrnIndex, ScreenPtr pScreen, int argc, char **argv)
{
    ScrnInfoPtr		pScrn = xf86Screens[pScreen->myNum];
    APMDECL(pScrn);
    int			ret;
    unsigned char	*FbBase;

    pApm->pScreen = pScreen;

    /* Map the chip memory and MMIO areas */
    if (pApm->noLinear) {
	pApm->saveCmd = pciReadLong(pApm->PciTag, PCI_CMD_STAT_REG);
	pciWriteLong(pApm->PciTag, PCI_CMD_STAT_REG, pApm->saveCmd | (PCI_CMD_IO_ENABLE|PCI_CMD_MEM_ENABLE));
	pApm->FbBase = xf86MapPciMem(pScrn->scrnIndex, VIDMEM_FRAMEBUFFER,
				 pApm->PciTag, 0xA0000, 0x10000);
    }
    else
	if (!ApmMapMem(pScrn))
	    return FALSE;

    /* No memory reserved yet */
    pApm->OffscreenReserved = 0;

    /* Save the current state */
    ApmSave(pScrn);

    /* Initialise the first mode */
    ApmModeInit(pScrn, pScrn->currentMode);
    pApm->CurrentLayout.pMode = pScrn->currentMode;

    /* Darken the screen for aesthetic reasons and set the viewport */
    ApmSaveScreen(pScreen, SCREEN_SAVER_ON);
    ApmAdjustFrame(scrnIndex, pScrn->frameX0, pScrn->frameY0, 0);

    /*
     * The next step is to setup the screen's visuals, and initialise the
     * framebuffer code.  In cases where the framebuffer's default
     * choices for things like visual layouts and bits per RGB are OK,
     * this may be as simple as calling the framebuffer's ScreenInit()
     * function.  If not, the visuals will need to be setup before calling
     * a fb ScreenInit() function and fixed up after.
     *
     * XXX NOTE: cfbScreenInit() will not result in the default visual
     * being set correctly when there is a screen-specific value given
     * in the config file as opposed to a global value given on the
     * command line.  Saving and restoring 'defaultColorVisualClass'
     * around the fb's ScreenInit() solves this problem.
     *
     * For most PC hardware at depths >= 8, the defaults that cfb uses
     * are not appropriate.  In this driver, we fixup the visuals after.
     */

    /*
     * Reset cfb's visual list.
     */
    miClearVisualTypes();

    /* Setup the visuals we support. */

    /*
     * For bpp > 8, the default visuals are not acceptable because we only
     * support TrueColor and not DirectColor.  To deal with this, call
     * miSetVisualTypes for each visual supported.
     */

    if (pApm->CurrentLayout.bitsPerPixel > 8) {
	if (!miSetVisualTypes(pScrn->depth, TrueColorMask, pScrn->rgbBits,
				pScrn->defaultVisual))
	    return FALSE;
    } else {
	if (!miSetVisualTypes(pScrn->depth,
			      miGetDefaultVisualMask(pScrn->depth),
			      pScrn->rgbBits, pScrn->defaultVisual))
	    return FALSE;
    }

    /*
     * Call the framebuffer layer's ScreenInit function, and fill in other
     * pScreen fields.
     */

    if(pApm->ShadowFB) {
	pApm->ShadowPitch =
		((pScrn->virtualX * pScrn->bitsPerPixel >> 3) + 3) & ~3L;
	pApm->ShadowPtr = xalloc(pApm->ShadowPitch * pScrn->virtualY);
	FbBase = pApm->ShadowPtr;
    } else {
	pApm->ShadowPtr = NULL;
	FbBase = pApm->FbBase;
    }

    /* Reserve memory */
    ApmHWCursorReserveSpace(pApm);
    ApmAccelReserveSpace(pApm);

    switch (pScrn->bitsPerPixel) {
    case 1:
	ret = xf1bppScreenInit(pScreen, FbBase,
			pScrn->virtualX, pScrn->virtualY,
			pScrn->xDpi, pScrn->yDpi,
			pScrn->displayWidth);
	break;
    case 4:
	ret = xf4bppScreenInit(pScreen, FbBase,
			pScrn->virtualX, pScrn->virtualY,
			pScrn->xDpi, pScrn->yDpi,
			pScrn->displayWidth);
	break;
    case 8:
	ret = cfbScreenInit(pScreen, FbBase, pScrn->virtualX,
	    pScrn->virtualY, pScrn->xDpi, pScrn->yDpi,
	    pScrn->displayWidth);
	break;
    case 16:
	ret = cfb16ScreenInit(pScreen, FbBase, pScrn->virtualX,
	    pScrn->virtualY, pScrn->xDpi, pScrn->yDpi,
	    pScrn->displayWidth);
	break;
    case 24:
	if (pix24bpp == 24)
	    ret = cfb24ScreenInit(pScreen, FbBase,
			pScrn->virtualX, pScrn->virtualY,
			pScrn->xDpi, pScrn->yDpi,
			pScrn->displayWidth);
	else
	    ret = cfb24_32ScreenInit(pScreen, FbBase,
			pScrn->virtualX, pScrn->virtualY,
			pScrn->xDpi, pScrn->yDpi,
			pScrn->displayWidth);
	break;
    case 32:
	ret = cfb32ScreenInit(pScreen, FbBase, pScrn->virtualX,
	    pScrn->virtualY, pScrn->xDpi, pScrn->yDpi,
	    pScrn->displayWidth);
	break;
    default:
	xf86DrvMsg(scrnIndex, X_ERROR,
	    "Internal error: invalid bpp (%d) in ApmScrnInit\n",
	    pScrn->bitsPerPixel);
	ret = FALSE;
	break;
    }
    if (!ret)
	return FALSE;

    if (pScrn->bitsPerPixel > 8) {
	VisualPtr	visual;

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

    if (!pApm->ShadowFB) {       /* hardware cursor needs to wrap this layer */
	if(!ApmDGAInit(pScreen)) {
	    xf86DrvMsg(pScrn->scrnIndex,X_ERROR,"DGA initialization failed\n");
	}
    }

    miInitializeBackingStore(pScreen);
    xf86SetBackingStore(pScreen);
    xf86SetSilkenMouse(pScreen);

    /* Initialise cursor functions */
    miDCInitialize (pScreen, xf86GetPointerScreenFuncs());

    /* Initialize HW cursor layer (after DGA and SW cursor) */
    if (pApm->hwCursor) {
	if (!ApmHWCursorInit(pScreen))
            xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
                "Hardware cursor initialization failed\n");
    }

    /*
     * Initialize the acceleration interface.
     */
    if (!pApm->NoAccel) {
	ApmAccelInit(pScreen);		/* set up XAA interface */
    }

    /* Initialise default colourmap */
    if (!miCreateDefColormap(pScreen))
	return FALSE;

    /*
     * Initialize colormap layer.
     * Must follow initialization of the default colormap.
     */
    if (!xf86HandleColormaps(pScreen, 256, 8, ApmLoadPalette, NULL,
				CMAP_RELOAD_ON_MODE_SWITCH)) {
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "Colormap initialization failed\n");
	return FALSE;
    }

    if (pApm->ShadowFB)
	ShadowFBInit(pScreen, ApmRefreshArea);

#ifdef DPMSExtension
    xf86DPMSInit(pScreen, ApmDisplayPowerManagementSet, 0);
#endif

    if (pApm->noLinear)
	ApmInitVideo_IOP(pScreen);
    else
	ApmInitVideo(pScreen);

    pScreen->SaveScreen  = ApmSaveScreen;

    pApm->CloseScreen = pScreen->CloseScreen;
    pScreen->CloseScreen = ApmCloseScreen;

    /* Report any unused options (only for the first generation) */
    if (serverGeneration == 1) {
	xf86ShowUnusedOptions(pScrn->scrnIndex, pScrn->options);
    }

    if (ApmGeneration != serverGeneration) {
	if ((ApmPixmapIndex = AllocatePixmapPrivateIndex()) < 0)
	    return FALSE;
	ApmGeneration = serverGeneration;
    }

    if (!AllocatePixmapPrivate(pScreen, ApmPixmapIndex, sizeof(ApmPixmapRec)))
	return FALSE;

    /* Done */
    return TRUE;
}

/* mandatory */
static void
ApmLoadPalette(ScrnInfoPtr pScrn, int numColors, int *indices, LOCO *colors,
	       VisualPtr pVisual)
{
    APMDECL(pScrn);
    int i, index, last = -1;

    if (pApm->VGAMap) {
	for (i = 0; i < numColors; i++) {
	    index = indices[i];
	    if (index != last)
		ApmWriteDacWriteAddr(index);
	    last = index + 1;
	    ApmWriteDacData(colors[index].red);
	    ApmWriteDacData(colors[index].green);
	    ApmWriteDacData(colors[index].blue);
	}
    }
    else {
	for (i = 0; i < numColors; i++) {
	    index = indices[i];
	    if (index != last) 
		outb(0x3C8, index);
	    last = index + 1;
	    outb(0x3C9, colors[index].red);
	    outb(0x3C9, colors[index].green);
	    outb(0x3C9, colors[index].blue);
	}
    }
}

/* Usually mandatory */
Bool
ApmSwitchMode(int scrnIndex, DisplayModePtr mode, int flags)
{
    return ApmModeInit(xf86Screens[scrnIndex], mode);
}

/*
 * This function is used to initialize the Start Address - the first
 * displayed location in the video memory.
 */
/* Usually mandatory */
void
ApmAdjustFrame(int scrnIndex, int x, int y, int flags)
{
    ScrnInfoPtr pScrn = xf86Screens[scrnIndex];
    APMDECL(pScrn);
    int Base;

    if (pApm->CurrentLayout.bitsPerPixel == 24)
	x = (x + 3) & ~3;
    Base = ((y * pApm->CurrentLayout.displayWidth + x) * (pApm->CurrentLayout.bitsPerPixel / 8)) >> 2;
    /*
     * These are the generic starting address registers.
     */
    if (pApm->VGAMap) {
	ApmWriteCrtc(0x0C, Base >> 8);
	ApmWriteCrtc(0x0D, Base);

	/*
	 * Here the high-order bits are masked and shifted, and put into
	 * the appropriate extended registers.
	 */
	ApmWriteCrtc(0x1C, (ApmReadCrtc(0x1C) & 0xF0) | ((Base & 0x0F0000) >> 16));
    }
    else {
	outw(0x3D4, (Base & 0x00FF00) | 0x0C);
	outw(0x3D4, ((Base & 0x00FF) << 8) | 0x0D);

	/*
	 * Here the high-order bits are masked and shifted, and put into
	 * the appropriate extended registers.
	 */
	modinx(0x3D4, 0x1C, 0x0F, (Base & 0x0F0000) >> 16);
    }
}

/*
 * This is called when VT switching back to the X server.  Its job is
 * to reinitialise the video mode.
 *
 * We may wish to unmap video/MMIO memory too.
 */

/* Mandatory */
static Bool
ApmEnterVT(int scrnIndex, int flags)
{
    ScrnInfoPtr pScrn = xf86Screens[scrnIndex];
    APMDECL(pScrn);
    vgaHWPtr	hwp = VGAHWPTR(pScrn);

    if (pApm->Chipset >= AT3D) {
	if (!pApm->noLinear) {
	    /* If you change it, change it also in apm_funcs.c */
	    WRXB(0xDB, (pApm->db & 0xF4) | 0x0A | pApm->Rush);
	    WRXB(0xD9, (pApm->d9 & 0xCF) | 0x20);
	}
	else {
	    WRXB_IOP(0xDB, pApm->db & 0xF4);
	}
    }
    if (pApm->Chipset >= AP6422)
	WRXB(0xC9, pApm->c9 | 0x10);
    ApmUnlock(APMPTR(pScrn));
    vgaHWUnlock(hwp);
    /*
     * Set color mode
     */
    hwp->writeMiscOut(hwp, pApm->MiscOut | 0x0F);

    if (!ApmModeInit(pScrn, pScrn->currentMode))
	return FALSE;
    ApmAdjustFrame(scrnIndex, pScrn->frameX0, pScrn->frameY0, 0);

    return TRUE;
}

/* Mandatory */
static void
ApmLeaveVT(int scrnIndex, int flags)
{
    ScrnInfoPtr pScrn = xf86Screens[scrnIndex];
    APMDECL(pScrn);
    vgaHWPtr	hwp = VGAHWPTR(pScrn);

    ApmRestore(pScrn, &VGAHWPTR(pScrn)->SavedReg, &pApm->SavedReg);
    /*
     * Reset color mode
     */
    hwp->writeMiscOut(hwp, pApm->MiscOut);
    vgaHWLock(VGAHWPTR(pScrn));
    ApmLock(pApm);
    if (pApm->Chipset >= AT3D) {
	if (!pApm->noLinear) {
	    WRXB(0xD9, pApm->d9);
	    WRXB(0xDB, pApm->db);
	}
	else {
	    WRXB_IOP(0xD9, pApm->d9);
	    WRXB_IOP(0xDB, pApm->db);
	}
    }
    WRXB(0xC9, pApm->c9);

    if (xf86IsPc98())
	outb(0xFAC, 0xFE);
}

/*
 * This is called at the end of each server generation.  It restores the
 * original (text) mode.  It should really also unmap the video memory too.
 */

/* Mandatory */
static Bool
ApmCloseScreen(int scrnIndex, ScreenPtr pScreen)
{
    ScrnInfoPtr pScrn = xf86Screens[scrnIndex];
    vgaHWPtr	hwp = VGAHWPTR(pScrn);
    APMDECL(pScrn);

    if (pScrn->vtSema) {
	ApmRestore(pScrn, &VGAHWPTR(pScrn)->SavedReg, &pApm->SavedReg);
	vgaHWLock(hwp);
	ApmUnmapMem(pScrn);
    }
    if(pApm->AccelInfoRec)
	XAADestroyInfoRec(pApm->AccelInfoRec);
    if(pApm->DGAXAAInfo)
	XAADestroyInfoRec(pApm->DGAXAAInfo);
    pApm->AccelInfoRec = NULL;
    if(pApm->CursorInfoRec)
	xf86DestroyCursorInfoRec(pApm->CursorInfoRec);
    pApm->CursorInfoRec = NULL;
    if (pApm->DGAModes)
	xfree(pApm->DGAModes);
    if (pApm->I2CPtr)
	xf86DestroyI2CBusRec(pApm->I2CPtr, TRUE, TRUE);
    pApm->I2CPtr = NULL;
    if (pApm->adaptor)
	xfree(pApm->adaptor);

    pScrn->vtSema = FALSE;

    if (xf86IsPc98())
	outb(0xFAC, 0xFE);

    pScreen->CloseScreen = pApm->CloseScreen;
    return (*pScreen->CloseScreen)(scrnIndex, pScreen);
}

/* Free up any per-generation data structures */

/* Optional */
static void
ApmFreeScreen(int scrnIndex, int flags)
{
    if (xf86LoaderCheckSymbol("vgaHWFreeHWRec"))
	vgaHWFreeHWRec(xf86Screens[scrnIndex]);
    ApmFreeRec(xf86Screens[scrnIndex]);
}

/* Checks if a mode is suitable for the selected chipset. */

/* Optional */
static int
ApmValidMode(int scrnIndex, DisplayModePtr mode, Bool verbose, int flags)
{
    if (mode->Flags & V_INTERLACE)
	return(MODE_BAD);

    return(MODE_OK);
}


/*
 * ApmDisplayPowerManagementSet --
 *
 * Sets VESA Display Power Management Signaling (DPMS) Mode.
 */
#ifdef DPMSExtension
static void
ApmDisplayPowerManagementSet(ScrnInfoPtr pScrn, int PowerManagementMode,
			     int flags)
{
    APMDECL(pScrn);
    unsigned char dpmsreg, tmp;

    switch (PowerManagementMode)
    {
    case DPMSModeOn:
	/* Screen: On; HSync: On, VSync: On */
	dpmsreg = 0x00;
	break;
    case DPMSModeStandby:
	/* Screen: Off; HSync: Off, VSync: On */
	dpmsreg = 0x01;
	break;
    case DPMSModeSuspend:
	/* Screen: Off; HSync: On, VSync: Off */
	dpmsreg = 0x02;
	break;
    case DPMSModeOff:
	/* Screen: Off; HSync: Off, VSync: Off */
	dpmsreg = 0x03;
	break;
    default:
	dpmsreg = 0;
    }
    if (pApm->noLinear) {
	tmp = RDXB_IOP(0xD0);
	WRXB_IOP(0xD0, (tmp & 0xFC) | dpmsreg);
    } else {
	tmp = RDXB(0xD0);
	WRXB(0xD0, (tmp & 0xFC) | dpmsreg);
    }
}
#endif

static Bool
ApmSaveScreen(ScreenPtr pScreen, int mode)
{
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    Bool unblank;

    unblank = xf86IsUnblank(mode);

    if (unblank)
	SetTimeSinceLastInputEvent();

    if (pScrn->vtSema)
	vgaHWBlankScreen(pScrn, unblank);
    return TRUE;
}

#ifdef APM_DEBUG
unsigned char _L_ACR(unsigned char *x);
unsigned char _L_ACR(unsigned char *x)
{
    return *x;
}

unsigned short _L_ASR(unsigned short *x);
unsigned short _L_ASR(unsigned short *x)
{
    return *x;
}

unsigned int _L_AIR(unsigned int *x);
unsigned int _L_AIR(unsigned int *x)
{
    return *x;
}

void _L_ACW(char *x, char y);
void _L_ACW(char *x, char y)
{
    *x = y;
}

void _L_ASW(short *x, short y);
void _L_ASW(short *x, short y)
{
    *x = y;
}

void _L_AIW(int *x, int y);
void _L_AIW(int *x, int y)
{
    *x = y;
}
#endif
