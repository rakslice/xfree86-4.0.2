/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/rendition/rendition.c,v 1.38 2000/12/14 16:33:10 eich Exp $ */
/*
 * Copyright (C) 1998 The XFree86 Project, Inc.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * XFREE86 PROJECT BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name of the XFree86 Project shall
 * not be used in advertising or otherwise to promote the sale, use or other
 * dealings in this Software without prior written authorization from the
 * XFree86 Project.
 */

/*
 * This is essentially a transfer of the 3.3 sources written by 
 * Marc Langenbach and Tim Rowley.
 *
 * The initial port of this driver to XFree86 4.0 was done by
 * Marc Langenbach <mlangen@studcs.uni-sb.de>
 * Additions, updates and bugfixes by Dejan Ilic <dejan.ilic@home.se>
 */

/*
 * Activate acceleration code or not.
 *
 *         WARNING BUGGY !!!
 * Yes, you activate it on your own risk.
 */
#define USE_ACCEL 0


/*
 * includes 
 */

#include "rendition.h"
#include "rendition_options.h"

#include "hwcursor.h"
#include "xf86int10.h"

#include "vtypes.h"
#include "vboard.h"
#include "vmisc.h"
#include "vmodes.h"
#include "vvga.h"
#include "accel.h"
#include "vramdac.h"
#include "rendition_shadow.h"
#include "vbe.h"

/*
 * defines
 */

#undef DEBUG

#define RENDITION_NAME            "Rendition"
#define RENDITION_DRIVER_NAME     "rendition"
#define RENDITION_VERSION_NAME    "4.0"
#define RENDITION_VERSION_MAJOR   4
#define RENDITION_VERSION_MINOR   0
#define RENDITION_PATCHLEVEL      0
#define RENDITION_VERSION_CURRENT ((RENDITION_VERSION_MAJOR << 24) | \
                 (RENDITION_VERSION_MINOR << 16) | RENDITION_PATCHLEVEL)

/* 
 * local function prototypes
 */

static OptionInfoPtr renditionAvailableOptions(int, int);
static void       renditionIdentify(int);
static Bool       renditionProbe(DriverPtr, int);
static Bool       renditionPreInit(ScrnInfoPtr, int);
static Bool       renditionScreenInit(int, ScreenPtr, int, char **);
static Bool       renditionSwitchMode(int, DisplayModePtr, int);
static void       renditionAdjustFrame(int, int, int, int);
static Bool       renditionEnterVT(int, int);
static void       renditionLeaveVT(int, int);
static void       renditionFreeScreen(int, int);

static ModeStatus renditionValidMode(int, DisplayModePtr, Bool, int);
static Bool renditionMapMem(ScrnInfoPtr pScreenInfo);
static Bool renditionUnmapMem(ScrnInfoPtr pScreenInfo);

static xf86MonPtr renditionDDC(ScrnInfoPtr pScreenInfo);
static unsigned int renditionDDC1Read (ScrnInfoPtr pScreenInfo);
static void renditionProbeDDC(ScrnInfoPtr pScrn, int index);

static void renditionLoadPalette(ScrnInfoPtr, int, int *, LOCO *, VisualPtr);


/* 
 * global data
 */

OptionInfoRec renditionOptions[]={
    { OPTION_FBWC,      "FramebufferWC", OPTV_BOOLEAN, {0}, FALSE },
    { OPTION_SW_CURSOR, "SW_Cursor", OPTV_BOOLEAN, {0}, FALSE },
    { OPTION_NOACCEL,   "NoAccel",  OPTV_BOOLEAN, {0}, FALSE },
    { OPTION_OVERCLOCK_MEM,"Overclock_Mem",  OPTV_BOOLEAN, {0}, FALSE },
    { OPTION_NO_DDC,    "NoDDC",    OPTV_BOOLEAN, {0}, FALSE },
    { OPTION_SHADOW_FB, "ShadowFB", OPTV_BOOLEAN, {0}, FALSE },
    { OPTION_ROTATE,    "Rotate",   OPTV_ANYSTR,  {0}, FALSE },
    { -1,                NULL,      OPTV_NONE,    {0}, FALSE }
};

DriverRec RENDITION={
    RENDITION_VERSION_CURRENT,
    "rendition",
    renditionIdentify,
    renditionProbe,
    renditionAvailableOptions,
    NULL,
    0
};

static const char *vgahwSymbols[]={
    "vgaHWGetHWRec",
    "vgaHWUnlock",
    "vgaHWInit",
    "vgaHWProtect",
    "vgaHWGetIOBase",
    "vgaHWMapMem",
    "vgaHWUnmapMem",
    "vgaHWLock",
    "vgaHWFreeHWRec",
    "vgaHWSaveScreen",
    "vgaHWSave",
    "vgaHWRestore",
    "vgaHWGetIndex",
    "vgaHWDPMSSet",
    "vgaHWBlankScreen",
    "vgaHWHandleColormaps",
    NULL
};

static const char *ramdacSymbols[] = {
    "xf86InitCursor",
    "xf86CreateCursorInfoRec",
    "xf86DestroyCursorInfoRec",
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

static const char *ddcSymbols[] = {
    "xf86PrintEDID",
    "xf86DoEDID_DDC1",
    NULL
};

static const char *int10Symbols[] = {
    "xf86InitInt10",
    NULL
};

static const char *fbSymbols[]={
    "xf1bppScreenInit",
    "xf4bppScreenInit",
    "cfbScreenInit",
    "cfb16ScreenInit",
    "cfb32ScreenInit",
    NULL
};

static const char *shadowfbSymbols[] = {
    "ShadowFBInit",
    NULL
};

static const char *vbeSymbols[] = {
    "VBEInit",
    "vbeDoEDID",
    NULL
};



#ifdef XFree86LOADER

/* Module loader interface */

static MODULESETUPPROTO(renditionSetup);

static XF86ModuleVersionInfo renditionVersionRec = {
    RENDITION_DRIVER_NAME,
    MODULEVENDORSTRING,
    MODINFOSTRING1,
    MODINFOSTRING2,
    XF86_VERSION_CURRENT,
    RENDITION_VERSION_MAJOR, RENDITION_VERSION_MINOR, RENDITION_PATCHLEVEL,
    ABI_CLASS_VIDEODRV,
    ABI_VIDEODRV_VERSION,
    MOD_CLASS_VIDEODRV,
    {0, 0, 0, 0}
};

XF86ModuleData renditionModuleData = 
               { &renditionVersionRec, renditionSetup, NULL };

static pointer
renditionSetup(pointer Module, pointer Options, int *ErrorMajor, 
               int *ErrorMinor)
{
    static Bool Initialised=FALSE;

    if (!Initialised) {
        Initialised=TRUE;
        xf86AddDriver(&RENDITION, Module, 0);
        LoaderRefSymLists(vgahwSymbols, ramdacSymbols, fbSymbols, 
			  xaaSymbols, ddcSymbols, int10Symbols,
			  shadowfbSymbols, vbeSymbols, NULL);
        return (pointer)TRUE;
    }

    if (ErrorMajor)
        *ErrorMajor=LDR_ONCEONLY;

    return NULL;
}

#endif


enum renditionTypes {
    CHIP_RENDITION_V1000,
    CHIP_RENDITION_V2x00
};

/* supported chipsets */
static SymTabRec renditionChipsets[] = {
    {CHIP_RENDITION_V1000, "V1000"},
    {CHIP_RENDITION_V2x00, "V2100/V2200"},
    {-1,                   NULL}
};

static PciChipsets renditionPCIchipsets[] = {
  { CHIP_RENDITION_V1000, PCI_CHIP_V1000, RES_SHARED_VGA },
  { CHIP_RENDITION_V2x00, PCI_CHIP_V2x00, RES_SHARED_VGA },
  { -1,                   -1,             RES_UNDEFINED }
};

/*
 * functions
 */

static OptionInfoPtr
renditionAvailableOptions(int chipid, int busid)
{
    return renditionOptions;
}

static void
renditionIdentify(int flags)
{
    xf86PrintChipsets(RENDITION_NAME,
        "rendition driver (version " RENDITION_VERSION_NAME ") for chipsets",
        renditionChipsets);
}



/*
 * This function is called once, at the start of the first server generation to
 * do a minimal probe for supported hardware.
 */
static Bool
renditionProbe(DriverPtr drv, int flags)
{
    Bool foundScreen=FALSE;
    int numDevSections, numUsed;
    GDevPtr *devSections;
    int *usedChips;
    int c;

    /* Find the config file Device sections that match this
     * driver, and return if there are none. */
    if ((numDevSections=xf86MatchDevice(RENDITION_NAME, &devSections)) <= 0)
        return FALSE;
  
    /* PCI BUS */
    if (xf86GetPciVideoInfo()) {
        numUsed=xf86MatchPciInstances(RENDITION_NAME, PCI_VENDOR_RENDITION,
                    renditionChipsets, renditionPCIchipsets, 
                    devSections, numDevSections, drv, &usedChips);

	xfree(devSections);
	if (numUsed <= 0)
	    return FALSE;

        if (flags & PROBE_DETECT)
            foundScreen = TRUE;
        else for (c=0; c<numUsed; c++) {
            ScrnInfoPtr pScrn;
            /* Allocate a ScrnInfoRec and claim the slot */
            pScrn=NULL;
            if ((pScrn = xf86ConfigPciEntity(pScrn, 0,usedChips[c],
						   renditionPCIchipsets, NULL,
						   NULL, NULL, NULL, NULL))) {
						       
		pScrn->driverVersion=RENDITION_VERSION_CURRENT;
		pScrn->driverName   =RENDITION_DRIVER_NAME;
		pScrn->name         =RENDITION_NAME;
		pScrn->Probe        =renditionProbe;
		pScrn->PreInit      =renditionPreInit;
		pScrn->ScreenInit   =renditionScreenInit;
		pScrn->SwitchMode   =renditionSwitchMode;
		pScrn->AdjustFrame  =renditionAdjustFrame;
		pScrn->EnterVT      =renditionEnterVT;
		pScrn->LeaveVT      =renditionLeaveVT;
		pScrn->FreeScreen   =renditionFreeScreen;
		pScrn->ValidMode    =renditionValidMode;
		foundScreen=TRUE;
	    }
        }
    }
    xfree(usedChips);
    return foundScreen;
}



static Bool
renditionClockSelect(ScrnInfoPtr pScreenInfo, int ClockNumber)
{
        static CARD8 save_misc;

        switch (ClockNumber)
        {
            case CLK_REG_SAVE:
                save_misc = inb(0x3CC);
                break;

            case CLK_REG_RESTORE:
                outb(0x3C2, save_misc);
                break;

            default:
                outb(0x3C2, (save_misc & 0xF3) | ((ClockNumber << 2) & 0x0C));
                break;
        }

    return TRUE;
}


static renditionPtr
renditionGetRec(ScrnInfoPtr pScreenInfo)
{
#ifdef DEBUG
    ErrorF("GetRec ...!!!!\n");
    sleep(1);
#endif
    if (!pScreenInfo->driverPrivate)
        pScreenInfo->driverPrivate=xcalloc(sizeof(renditionRec), 1);

    /* perhaps some initialization? <ml> */

#ifdef DEBUG
    ErrorF("GetRec ...!!!!\n");
    sleep(1);
#endif
    return (renditionPtr)pScreenInfo->driverPrivate;
}


static void
renditionFreeRec(ScrnInfoPtr pScreenInfo)
{
#ifdef DEBUG
    ErrorF("FreeRec...!!!!\n");
    sleep(1);
#endif
    if (xf86LoaderCheckSymbol("vgaHWFreeHWRec"))
	vgaHWFreeHWRec(pScreenInfo);
    xfree(pScreenInfo->driverPrivate);
    pScreenInfo->driverPrivate=NULL;

#ifdef DEBUG
    ErrorF("FreeRec OK...!!!!\n");
    sleep(1);
#endif
}


static void
renditionProtect(ScrnInfoPtr pScreenInfo, Bool On)
{
#ifdef DEBUG
    ErrorF("Protect...!!!!\n");
    sleep(1);
#endif

    vgaHWProtect(pScreenInfo, On);

#ifdef DEBUG
    ErrorF("Protect OK...!!!!\n");
    sleep(1);
#endif
}


static Bool
renditionSaveScreen(ScreenPtr pScreen, int mode)
{
#ifdef DEBUG
    ErrorF("Savescreen...!!!!\n");
    sleep(1);
#endif

    return vgaHWSaveScreen(pScreen, mode);
}

static void
renditionBlankScreen(ScrnInfoPtr pScreenInfo, Bool Unblank)
{
#ifdef DEBUG
    ErrorF("Blankscreen...!!!!\n");
    sleep(1);
#endif

    vgaHWBlankScreen(pScreenInfo, Unblank);
#ifdef DEBUG
    ErrorF("Blankscreen OK...!!!!\n");
    sleep(1);
#endif
}


/*
 * This function is called once for each screen at the start of the first
 * server generation to initialise the screen for all server generations.
 */
static Bool
renditionPreInit(ScrnInfoPtr pScreenInfo, int flags)
{
    static ClockRange renditionClockRange = {NULL, 0, 135000, -1, FALSE, TRUE, 1, 1, 0};
    MessageType       From;
    int               i, videoRam, Rounding, nModes = 0;
    char             *Module;
    const char       *Sym;
    vgaHWPtr          pvgaHW;
    renditionPtr      pRendition;
    char             *in_string;
    
#ifdef DEBUG
    ErrorF("Rendition: renditionPreInit() called\n");
#endif

    /* Check the number of entities, and fail if it isn't one. */
    if (pScreenInfo->numEntities != 1)
	return FALSE;

    /* allocate driver private structure */
    if (!renditionGetRec(pScreenInfo))
        return FALSE;

    pRendition=RENDITIONPTR(pScreenInfo);

    /* Get the entity, and make sure it is PCI. */
    pRendition->pEnt = xf86GetEntityInfo(pScreenInfo->entityList[0]);
    if (pRendition->pEnt->location.type != BUS_PCI)
	return FALSE;

    if (flags & PROBE_DETECT) {
        renditionProbeDDC(pScreenInfo, pRendition->pEnt->index);
        return TRUE;
    }

    /* set the monitor */
    pScreenInfo->monitor=pScreenInfo->confScreen->monitor;

    /* Initialize the card through int10 interface if needed */
    if (xf86LoadSubModule(pScreenInfo, "int10")){
        xf86Int10InfoPtr pInt=NULL;

        xf86LoaderReqSymLists(int10Symbols, NULL);

        xf86DrvMsg(pScreenInfo->scrnIndex, X_INFO, "Initializing int10\n");
        pInt = xf86InitInt10(pRendition->pEnt->index);
        xf86FreeInt10(pInt);
    }

    /* Find the PCI info for this screen */
    pRendition->PciInfo = xf86GetPciInfoForEntity(pRendition->pEnt->index);
    pRendition->pcitag= pciTag(pRendition->PciInfo->bus,
               pRendition->PciInfo->device, pRendition->PciInfo->func);

    /*
     * XXX This could be refined if some VGA memory resources are not
     * decoded in operating mode.
     */
    {
        resRange vgamem[] =     { {ResShrMemBlock,0xA0000,0xAFFFF},
                                  {ResShrMemBlock,0xB0000,0xB7FFF},
                                  {ResShrMemBlock,0xB8000,0xBFFFF},
                                  _END };
        xf86SetOperatingState(vgamem, pRendition->pEnt->index, ResUnusedOpr);
    }
    /* Operations for which memory access is required. */
    pScreenInfo->racMemFlags = RAC_FB | RAC_COLORMAP | RAC_CURSOR | RAC_VIEWPORT;
    /* Operations for which I/O access is required. (XXX Check this) */
    pScreenInfo->racIoFlags = RAC_FB | RAC_COLORMAP | RAC_CURSOR | RAC_VIEWPORT;
    
    /* determine depth, bpp, etc. */
    if (!xf86SetDepthBpp(pScreenInfo, 8, 8, 8, Support32bppFb))
        return FALSE;

    switch (pScreenInfo->depth) {
        case 8:   Module = "cfb";   Sym = "cfbScreenInit";   break;

        case 15:  if (PCI_CHIP_V1000==pRendition->PciInfo->chipType){
                    Module = "cfb16"; Sym = "cfbScreenInit16"; break;
	          }
                  else {
		    xf86DrvMsg(pScreenInfo->scrnIndex, X_ERROR,
		       "Given depth (%d) is not supported by this chipset.\n",
		       pScreenInfo->depth);
		    return FALSE;
		  }
        case 16:  Module = "cfb16"; Sym = "cfbScreenInit16"; break;

        case 24:  Module = "cfb32"; Sym = "cfbScreenInit32"; break;

        default:
            xf86DrvMsg(pScreenInfo->scrnIndex, X_ERROR,
                "Given depth (%d) is not supported by this driver.\n",
                pScreenInfo->depth);
            return FALSE;
    }



    /* collect all of the options flags and process them */

    xf86CollectOptions(pScreenInfo, NULL);
    xf86ProcessOptions(pScreenInfo->scrnIndex, pScreenInfo->options, 
        renditionOptions);


    /* Ensure depth-specific entry points are available */
    if (!xf86LoadSubModule(pScreenInfo, Module))
      return FALSE;

    xf86LoaderReqSymbols(Sym, NULL);

    /* determine colour weights */
    pScreenInfo->rgbBits=8;
    
    if (pScreenInfo->depth > 8) {
      rgb defaultWeight = {0, 0, 0};
      rgb defaultMask = {0, 0, 0};

      xf86PrintDepthBpp(pScreenInfo);

      /* Standard defaults are OK if depths are OK */
      if (!xf86SetWeight(pScreenInfo, defaultWeight, defaultMask))
        return FALSE;
      else{
	/* XXX:  Check that returned weight is supported */
      }
    }

    /* determine default visual */
    if (!xf86SetDefaultVisual(pScreenInfo, -1))
      return FALSE;

    /* the gamma fields must be initialised when using the new cmap code */
    if (pScreenInfo->depth > 1) {
        Gamma zeros = {0.0, 0.0, 0.0};

        if (!xf86SetGamma(pScreenInfo, zeros))
            return FALSE;
    }

    /* the Rendition chips have a programmable clock */
    pScreenInfo->progClock=TRUE;

    /* set various fields according to the given options */
    /* to be filled in <ml> */

    if (PCI_CHIP_V1000==pRendition->PciInfo->chipType){
      pRendition->board.chip=V1000_DEVICE;
    }
    else {
      pRendition->board.chip=V2000_DEVICE;
      renditionClockRange.maxClock = 170000;
      renditionClockRange.clockIndex = -1;
    }

    pRendition->board.accel=0;
    pRendition->board.io_base=pRendition->PciInfo->ioBase[1];
    pRendition->board.mmio_base=0;
    pRendition->board.vmmio_base=0;
    pRendition->board.mem_size=0;
    pRendition->board.mem_base=(vu8 *)pRendition->PciInfo->memBase[0];
    pRendition->board.vmem_base=NULL;
    pRendition->board.init=0;

    if (pScreenInfo->chipset)
        xf86DrvMsg(pScreenInfo->scrnIndex, X_CONFIG, "Chipset: \"%s\".\n",
            pScreenInfo->chipset);
    else
        xf86DrvMsg(pScreenInfo->scrnIndex, X_PROBED, "Chipset: \"%s\".\n",
            renditionChipsets[
        pRendition->board.chip==V1000_DEVICE ? 0:1].name);

    /* I do not get the IO base addres <ml> */
    /* XXX Is this still true?  If so, the wrong base is being checked */
    xf86DrvMsg(pScreenInfo->scrnIndex, X_PROBED,
	       "Rendition %s @ %x/%x\n",
	       renditionChipsets[pRendition->board.chip==V1000_DEVICE ? 0:1].name,
	       pRendition->board.io_base,
	       pRendition->board.mem_base);

    /* First of all get a "clean" starting state */
#if 1
    verite_resetboard(pScreenInfo);
#endif

    /* determine video ram -- to do so, we assume a full size memory of 16M,
     * then map it and use verite_getmemorysize() to determine the real amount of
     * memory */
    pScreenInfo->videoRam=pRendition->board.mem_size=16<<20;
    renditionMapMem(pScreenInfo);
    videoRam=verite_getmemorysize(pScreenInfo)>>10;

    /* Unmaping delayed until after micrcode loading */
      /****************************************/
      /* Reserv memory and load the microcode */
      /****************************************/
#if USE_ACCEL
    if (!xf86ReturnOptValBool(renditionOptions, OPTION_NOACCEL,0)) {
      RENDITIONAccelPreInit (pScreenInfo);
    }
    else
      xf86DrvMsg(pScreenInfo->scrnIndex, X_CONFIG,
		 ("Skipping acceleration on users request\n"));
#else
    xf86DrvMsg(pScreenInfo->scrnIndex, X_WARNING,
	       ("Skipping acceleration\n"));
#endif

    xf86MarkOptionUsedByName(renditionOptions,"NoAccel");

    From = X_PROBED;
    xf86DrvMsg(pScreenInfo->scrnIndex, From, "videoRam: %d kBytes\n", videoRam);
    pScreenInfo->videoRam=videoRam;
    pRendition->board.mem_size=videoRam;


    /* Load the needed symbols */

    if (!xf86LoadSubModule(pScreenInfo, "vgahw")){
        return FALSE;
    }
    xf86LoaderReqSymLists(vgahwSymbols, NULL);

    pRendition->board.shadowfb=TRUE;

    if ((in_string = xf86GetOptValString(renditionOptions, OPTION_ROTATE))) {
      if(!xf86NameCmp(in_string, "CW")) {
	/* accel is disabled below for shadowFB */
	pRendition->board.shadowfb = TRUE;
	pRendition->board.rotate = 1;
	xf86DrvMsg(pScreenInfo->scrnIndex, X_CONFIG,
		   "Rotating screen clockwise - acceleration disabled\n");
      } else if(!xf86NameCmp(in_string, "CCW")) {
	pRendition->board.shadowfb = TRUE;
	pRendition->board.rotate = -1;
	xf86DrvMsg(pScreenInfo->scrnIndex, X_CONFIG,  "Rotating screen "
		   "counter clockwise - acceleration disabled\n");
      } else {
	xf86DrvMsg(pScreenInfo->scrnIndex, X_CONFIG, "\"%s\" is not a valid"
		   "value for Option \"Rotate\"\n", in_string);
	xf86DrvMsg(pScreenInfo->scrnIndex, X_INFO,
		   "Valid options are \"CW\" or \"CCW\"\n");
      }
    }
    xf86MarkOptionUsedByName(renditionOptions,"Rotate");

    if (xf86ReturnOptValBool(renditionOptions, OPTION_SHADOW_FB,1)||
	pRendition->board.rotate) {
      if (!xf86LoadSubModule(pScreenInfo, "shadowfb")) {
	xf86DrvMsg(pScreenInfo->scrnIndex, X_WARNING,
	 "Oops, \"ShadowFB\" module loading failed, disabling ShadowFB!\n");
      }
      else{
	xf86LoaderReqSymLists(shadowfbSymbols, NULL);
	pRendition->board.shadowfb=TRUE;
	xf86DrvMsg(pScreenInfo->scrnIndex, X_INFO,
		   "Using \"Shadow Framebuffer\"\n");
      }
    }
    else {
      pRendition->board.shadowfb=FALSE;
      xf86DrvMsg(pScreenInfo->scrnIndex, X_CONFIG,
		 "\"Shadow Framebuffer\" disabled\n");
    }
    xf86MarkOptionUsedByName(renditionOptions,"ShadowFB");


    /* Load Ramdac module if needed */
    if (!xf86ReturnOptValBool(renditionOptions, OPTION_SW_CURSOR,0) &&
	!pRendition->board.rotate){
      if (!xf86LoadSubModule(pScreenInfo, "ramdac")) {
	return FALSE;
      }
      xf86LoaderReqSymLists(ramdacSymbols, NULL);
    }
    xf86MarkOptionUsedByName(renditionOptions,"SWCursor");

#if USE_ACCEL
    /* Load XAA if needed */
    if (!xf86ReturnOptValBool(renditionOptions, OPTION_NOACCEL,0) &&
	!pRendition->board.rotate) {
      if (!xf86LoadSubModule(pScreenInfo, "xaa")) {
	return FALSE;
      }
      xf86LoaderReqSymLists(xaaSymbols, NULL);
    }
#endif

#if 0
    /* Load DDC module if needed */
    if (!xf86ReturnOptValBool(renditionOptions, OPTION_NO_DDC,0)){
      if (!xf86LoadSubModule(pScreenInfo, "ddc")) {
	xf86DrvMsg(pScreenInfo->scrnIndex, X_ERROR,
		   ("Loading of DDC library failed, skipping DDC-probe\n"));
      }
      else {
	xf86LoaderReqSymLists(ddcSymbols, NULL);
	pScreenInfo->monitor->DDC = renditionDDC(pScreenInfo);
      }
    }
    else {
      xf86DrvMsg(pScreenInfo->scrnIndex, X_CONFIG,
		 ("Skipping DDC probe on users request\n"));
    }
#else
    /* Load DDC module if needed */
    if (!xf86ReturnOptValBool(renditionOptions, OPTION_NO_DDC,0)){
      if (!xf86LoadSubModule(pScreenInfo, "vbe")) {
	xf86DrvMsg(pScreenInfo->scrnIndex, X_ERROR,
		   ("Loading of DDC library failed, skipping DDC-probe\n"));
      }
      else {
	xf86LoaderReqSymLists(vbeSymbols, NULL);
        renditionProbeDDC(pScreenInfo, pRendition->pEnt->index);
      }
    }
    else {
      xf86DrvMsg(pScreenInfo->scrnIndex, X_CONFIG,
		 ("Skipping DDC probe on users request\n"));
    }
#endif

    /***********************************************/
    /* ensure vgahw private structure is allocated */

    if (!vgaHWGetHWRec(pScreenInfo))
        return FALSE;

    pvgaHW = VGAHWPTR(pScreenInfo);
    pvgaHW->MapSize = 0x00010000;       /* Standard 64kB VGA window */
    vgaHWGetIOBase(pvgaHW);             /* Get VGA I/O base */

    /*
     * Determine clocks.  Limit them to the first four because that's all that
     * can be addressed.
     * XXX Aren't the clocks programmable?  If so, this discrete clock stuff
     * shouldn't be used.
     */
    if ((pScreenInfo->numClocks = pRendition->pEnt->device->numclocks))
    {
        if (pScreenInfo->numClocks > 4)
            pScreenInfo->numClocks = 4;
        for (i = 0;  i < pScreenInfo->numClocks;  i++)
            pScreenInfo->clock[i] = pRendition->pEnt->device->clock[i];
        From = X_CONFIG;
    }
    else
    {
        xf86GetClocks(pScreenInfo, 4, renditionClockSelect, renditionProtect,
            renditionBlankScreen, VGAHW_GET_IOBASE() + 0x0A, 0x08, 1, 28322);
        From = X_PROBED;
    }
    xf86ShowClocks(pScreenInfo, From);

    if (pScreenInfo->display->modes && pScreenInfo->display->modes[0])
    {
        /* Set the virtual X rounding (in bits) */
        if (pScreenInfo->depth == 8)
            Rounding = 16 * 8;
        else
            Rounding = 16;

        /*
         * Validate the modes.  Note that the limits passed to
         * xf86ValidateModes() are VGA CRTC architectural limits.
         */
        pScreenInfo->maxHValue = 2080;
        pScreenInfo->maxVValue = 1025;
        nModes = xf86ValidateModes(pScreenInfo,
            pScreenInfo->monitor->Modes, pScreenInfo->display->modes,
            &renditionClockRange, NULL, 8, 2040, Rounding, 1, 1024,
            pScreenInfo->display->virtualX, pScreenInfo->display->virtualY,
            0x10000, LOOKUP_CLOSEST_CLOCK | LOOKUP_CLKDIV2);

        if (nModes < 0)
            return FALSE;

        /* Remove invalid modes */
        xf86PruneDriverModes(pScreenInfo);
    }

    /* Set CRTC values for the modes */
    xf86SetCrtcForModes(pScreenInfo, 0);

    /* Set current mode to the first in list */
    pScreenInfo->currentMode = pScreenInfo->modes;

    /* Print mode list */
    xf86PrintModes(pScreenInfo);

    /* Set display resolution */
    xf86SetDpi(pScreenInfo, 0, 0);

    /* Only one chipset here */
    if (!pScreenInfo->chipset)
        pScreenInfo->chipset = (char *)renditionChipsets[0].name;

    if(!xf86ReturnOptValBool(renditionOptions, OPTION_SW_CURSOR,0)){
      if(!pRendition->board.rotate)
	/* Do preemtive things for HW cursor */
	RenditionHWCursorPreInit(pScreenInfo);
      else{
	xf86DrvMsg(pScreenInfo->scrnIndex, X_WARNING,
		   "Hardware cursor not supported on rotated screen\n");
	xf86DrvMsg(pScreenInfo->scrnIndex, X_INFO,
		   "Software cursor activated\n");
      }
    }
    else 
      xf86DrvMsg(pScreenInfo->scrnIndex, X_CONFIG,
		 "Software cursor selected\n");

    renditionUnmapMem(pScreenInfo);

#ifdef DEBUG
    ErrorF("PreInit OK...!!!!\n");
    sleep(2);
#endif

    return TRUE;        /* Tada! */
}


/* Save mode on server entry */
static void
renditionSave(ScrnInfoPtr pScreenInfo)
{
#ifdef DEBUG
    ErrorF("Save...!!!!\n");
    sleep(1);
#endif
    vgaHWSave(pScreenInfo, &VGAHWPTR(pScreenInfo)->SavedReg,VGA_SR_ALL);
	
#ifdef DEBUG
    ErrorF("Save OK...!!!!\n");
    sleep(1);
#endif
}


/* Restore the mode that was saved on server entry */
static void
renditionRestore(ScrnInfoPtr pScreenInfo)
{
#ifdef DEBUG
    ErrorF("Restore...!!!!\n");
    sleep(1);
#endif

    vgaHWProtect(pScreenInfo, TRUE);
    vgaHWRestore(pScreenInfo, &VGAHWPTR(pScreenInfo)->SavedReg, VGA_SR_ALL);
    vgaHWProtect(pScreenInfo, FALSE);

    verite_setmode(pScreenInfo, &RENDITIONPTR(pScreenInfo)->mode);
#ifdef DEBUG
    ErrorF("Restore OK...!!!!\n");
    sleep(1);
#endif
}


/* Set a graphics mode */
static Bool
renditionSetMode(ScrnInfoPtr pScreenInfo, DisplayModePtr pMode)
{
    struct verite_modeinfo_t *modeinfo=&RENDITIONPTR(pScreenInfo)->mode;
    vgaHWPtr pvgaHW;

#ifdef DEBUG
    ErrorF("RENDITION: renditionSetMode() called\n");
    ErrorF("Setmode...!!!!\n");
    sleep(1);
#endif

    pvgaHW = VGAHWPTR(pScreenInfo);
    /* construct a modeinfo for the verite_setmode function */
    modeinfo->clock=pMode->SynthClock;
    modeinfo->hdisplay=pMode->HDisplay;
    modeinfo->hsyncstart=pMode->HSyncStart;
    modeinfo->hsyncend=pMode->HSyncEnd;
    modeinfo->htotal=pMode->HTotal;
    modeinfo->hskew=pMode->HSkew;
    modeinfo->vdisplay=pMode->VDisplay;
    modeinfo->vsyncstart=pMode->VSyncStart;
    modeinfo->vsyncend=pMode->VSyncEnd;
    modeinfo->vtotal=pMode->VTotal;

    modeinfo->screenwidth=pMode->HDisplay;
    modeinfo->virtualwidth=pScreenInfo->virtualX&0xfff8;
    modeinfo->screenheight=pMode->VDisplay;
    modeinfo->virtualheight=pScreenInfo->virtualY&0xfff8;

    if ((pMode->Flags&(V_PHSYNC|V_NHSYNC)) 
        && (pMode->Flags&(V_PVSYNC|V_NVSYNC))) {
        modeinfo->hsynchi=((pMode->Flags&V_PHSYNC) == V_PHSYNC);
        modeinfo->vsynchi=((pMode->Flags&V_PVSYNC) == V_PVSYNC);
    }
    else {
        int VDisplay=pMode->VDisplay;
        if (pMode->Flags & V_DBLSCAN)
            VDisplay*=2;
        if (VDisplay < 400) {
            /* +hsync -vsync */
            modeinfo->hsynchi=1;
            modeinfo->vsynchi=0;
        }
        else if (VDisplay < 480) {
            /* -hsync +vsync */
            modeinfo->hsynchi=0;
            modeinfo->vsynchi=1;
        }
        else if (VDisplay < 768) {
            /* -hsync -vsync */
            modeinfo->hsynchi=0;
            modeinfo->vsynchi=0;
        }
        else {
            /* +hsync +vsync */
            modeinfo->hsynchi=1;
            modeinfo->vsynchi=1;
        }
    }

    switch (pScreenInfo->bitsPerPixel) {
        case 8:
            modeinfo->bitsperpixel=8;
            modeinfo->pixelformat=V_PIXFMT_8I;
            break;
        case 16:
            modeinfo->bitsperpixel=16;
            if (pScreenInfo->weight.green == 5)
                /* on a V1000, this looks too 'red/magenta' <ml> */
                modeinfo->pixelformat=V_PIXFMT_1555;
            else
                modeinfo->pixelformat=V_PIXFMT_565;
            break;
        case 32:
            modeinfo->bitsperpixel=32;
            modeinfo->pixelformat=V_PIXFMT_8888;
            break;
    }
    modeinfo->fifosize=128;
    modeinfo->flags=pMode->Flags;

    verite_setmode(pScreenInfo,&RENDITIONPTR(pScreenInfo)->mode);

#ifdef DEBUG
    ErrorF("Setmode OK...!!!!\n");
    sleep(1);
#endif
    return TRUE;
}


static Bool
renditionEnterGraphics(ScreenPtr pScreen, ScrnInfoPtr pScreenInfo)
{
    vgaHWPtr pvgaHW = VGAHWPTR(pScreenInfo);

#ifdef DEBUG
    ErrorF("RENDITION: renditionEnterGraphics() called\n");
    sleep(2);
#endif

    /* Map VGA aperture */
    if (!vgaHWMapMem(pScreenInfo))
        return FALSE;

    /* Unlock VGA registers */
    vgaHWUnlock(pvgaHW);

    /* Save the current state and setup the current mode */
    renditionSave(pScreenInfo);

    if (!renditionSetMode(pScreenInfo, pScreenInfo->currentMode))
        return FALSE;

    /* Possibly blank the screen */
    if (pScreen)
        renditionSaveScreen(pScreen, SCREEN_SAVER_ON);

    (*pScreenInfo->AdjustFrame)(pScreenInfo->scrnIndex,
        pScreenInfo->frameX0, pScreenInfo->frameY0, 0);

#ifdef DEBUG
    ErrorF("Entergraphics OK...!!!!\n");
    sleep(1);
#endif

    return TRUE;
}


static void
renditionLeaveGraphics(ScrnInfoPtr pScreenInfo)
{
#ifdef DEBUG
    ErrorF("RENDITION: renditionLeaveGraphics() called\n");

    ErrorF("Leavegraphics..!!!!\n");
    sleep(1);
#endif

    renditionRestore(pScreenInfo);
    vgaHWLock(VGAHWPTR(pScreenInfo));

    verite_textmode(&RENDITIONPTR(pScreenInfo)->board);
#ifdef DEBUG
    ErrorF("Leavegraphics OK...!!!!\n");
    sleep(1);
#endif
}


/* Unravel the screen */
static Bool
renditionCloseScreen(int scrnIndex, ScreenPtr pScreen)
{
    ScrnInfoPtr pScreenInfo = xf86Screens[scrnIndex];
    renditionPtr prenditionPriv=renditionGetRec(pScreenInfo);
    Bool Closed = TRUE;

#ifdef DEBUG
    ErrorF("RENDITION: renditionCloseScreen() called\n");

    sleep(1);
#endif

    if (prenditionPriv->board.hwcursor_used)
      RenditionHWCursorRelease(pScreenInfo);

    if (prenditionPriv->board.accel)
      RENDITIONAccelNone(pScreenInfo);

    if (prenditionPriv && (pScreen->CloseScreen = prenditionPriv->CloseScreen))
    {
        prenditionPriv->CloseScreen = NULL;
        Closed = (*pScreen->CloseScreen)(scrnIndex, pScreen);
    }
    
    if (pScreenInfo->vtSema)
	renditionLeaveGraphics(pScreenInfo);
    pScreenInfo->vtSema = FALSE;

#ifdef DEBUG
    ErrorF("Closescreen OK...!!!!\n");
    sleep(1);
#endif
    return Closed;
}


#ifdef DPMSExtension
static void
renditionDPMSSet(ScrnInfoPtr pScreen, int mode, int flags)
{
#ifdef DEBUG
    ErrorF("RENDITION: renditionDPMSSet() called\n");
#endif

    vgaHWDPMSSet(pScreen, mode, flags);
}
#endif


static Bool
renditionScreenInit(int scrnIndex, ScreenPtr pScreen, int argc, char **argv)
{
    ScrnInfoPtr pScreenInfo = xf86Screens[scrnIndex];
    renditionPtr pRendition = RENDITIONPTR(pScreenInfo);
    renditionPtr prenditionPriv;
    Bool Inited = FALSE;
    unsigned char *FBBase;
    VisualPtr visual;
    int displayWidth,width,height;

    vgaHWPtr          pvgaHW;

#ifdef DEBUG
    ErrorF("RENDITION: renditionScreenInit() called\n");
    sleep(1);
#endif

    /* Get driver private */
    prenditionPriv=renditionGetRec(pScreenInfo);
    if (NULL == prenditionPriv) /* xcalloc failed */
      return FALSE;

    /* Initialise graphics mode */
    if (!renditionEnterGraphics(pScreen, pScreenInfo))
        return FALSE;

    /* Get vgahw private     */
    pvgaHW = VGAHWPTR(pScreenInfo);

    miClearVisualTypes();

    if (!miSetVisualTypes(pScreenInfo->depth,
              miGetDefaultVisualMask(pScreenInfo->depth),
              pScreenInfo->rgbBits, pScreenInfo->defaultVisual))
      return FALSE;

    if (pRendition->board.rotate) {
      height = pScreenInfo->virtualX;
      width = pScreenInfo->virtualY;
    } else {
      width = pScreenInfo->virtualX;
      height = pScreenInfo->virtualY;
    }

    if(pRendition->board.shadowfb) {
      pRendition->board.shadowPitch = BitmapBytePad(pScreenInfo->bitsPerPixel * width);
      pRendition->board.shadowPtr = xalloc(pRendition->board.shadowPitch * height);
      displayWidth = pRendition->board.shadowPitch / 
	             (pScreenInfo->bitsPerPixel >> 3);
      FBBase = pRendition->board.shadowPtr;
    } else {
      pRendition->board.shadowPtr = NULL;
      FBBase = pRendition->board.vmem_base+prenditionPriv->board.fbOffset;
      displayWidth=pScreenInfo->displayWidth;
    }


    /* initialise the framebuffer */
    switch (pScreenInfo->bitsPerPixel)
    {
        case 8:
            Inited = cfbScreenInit(pScreen, FBBase,
				   width, height,
				   pScreenInfo->xDpi, pScreenInfo->yDpi,
				   displayWidth);
            break;
        case 16:
            Inited = cfb16ScreenInit(pScreen, FBBase,
				     width, height,
				     pScreenInfo->xDpi, pScreenInfo->yDpi,
				     displayWidth);
	    break;
        case 32:
            Inited = cfb32ScreenInit(pScreen, FBBase,
				     width, height,
				     pScreenInfo->xDpi, pScreenInfo->yDpi,
				     displayWidth);
	    break;
    default:
        xf86DrvMsg(scrnIndex, X_ERROR,
                   "Internal error: invalid bpp (%d) in renditionScreenInit\n",
                   pScreenInfo->bitsPerPixel);
        break;
    }

    if (!Inited)
        return FALSE;


    if (pScreenInfo->bitsPerPixel > 8) {
        /* Fixup RGB ordering */
        visual=pScreen->visuals+pScreen->numVisuals;
        while (--visual >= pScreen->visuals) {
	  if ((visual->class | DynamicClass) == DirectColor){
	    visual->offsetRed = pScreenInfo->offset.red;
	    visual->offsetGreen = pScreenInfo->offset.green;
	    visual->offsetBlue = pScreenInfo->offset.blue;
	    visual->redMask = pScreenInfo->mask.red;
	    visual->greenMask = pScreenInfo->mask.green;
	    visual->blueMask = pScreenInfo->mask.blue;
	  }

#if 0   /* This code is never called ? <DI> */
	  else {
	    switch (pScreenInfo->bitsPerPixel)
	      {
	      case 16:
		if (pScreenInfo->weight.green == 5){
		  /* weight 555 mode */
		  ErrorF("RENDITION: 15bit mode\n");
		  visual->offsetRed=10;
		  visual->offsetGreen=5;
		  visual->offsetBlue=0;
		  visual->redMask=0x7c00;
		  visual->greenMask=0x3e0;
		  visual->blueMask=0x1f;
		}
		else{
		  ErrorF("RENDITION: 16bit mode\n");
		  visual->offsetRed=11;
		  visual->offsetGreen=5;
		  visual->offsetBlue=0;
		  visual->redMask=0xf800;
		  visual->greenMask=0x7e0;
		  visual->blueMask=0x1f;
		}
		break;
		
	      case 32:
		ErrorF("RENDITION: 32bit mode\n");
		visual->offsetRed=16;
		visual->offsetGreen=8;
		visual->offsetBlue=0;
		visual->redMask=0xff0000;
		visual->greenMask=0xff00;
		visual->blueMask=0xff;
		break;
	      }
            }
#endif  /* Never used fixup code */
	}
    }

    xf86SetBlackWhitePixels(pScreen);
    miInitializeBackingStore(pScreen);
   
    /*********************************************************/
    /* The actual setup of the driver-specific code          */
    /* has to be after cfb*ScreenInit and before cursor init */
    /*********************************************************/
#if USE_ACCEL
    if (!xf86ReturnOptValBool(renditionOptions, OPTION_NOACCEL,0)) 
      RENDITIONAccelXAAInit (pScreen);
#endif

    /* Initialise cursor functions */
    xf86SetSilkenMouse(pScreen);
    miDCInitialize(pScreen, xf86GetPointerScreenFuncs());

    if(!xf86ReturnOptValBool(renditionOptions, OPTION_SW_CURSOR,0)&&
       !pRendition->board.rotate){
      /* Initialise HW cursor */
      if(!RenditionHWCursorInit(scrnIndex, pScreen)){
	xf86DrvMsg(pScreenInfo->scrnIndex, X_ERROR,
		   "Hardware Cursor initalization failed!!\n");
      }
    }

    if (pRendition->board.shadowfb) {
      RefreshAreaFuncPtr refreshArea = renditionRefreshArea;

      if(pRendition->board.rotate) {
	if (!pRendition->board.PointerMoved) {
	  pRendition->board.PointerMoved = pScreenInfo->PointerMoved;
	  pScreenInfo->PointerMoved = renditionPointerMoved;
	}

	switch(pScreenInfo->bitsPerPixel) {
	case 8:         refreshArea = renditionRefreshArea8;  break;
	case 16:        refreshArea = renditionRefreshArea16; break;
	case 24:        refreshArea = renditionRefreshArea24; break;
	case 32:        refreshArea = renditionRefreshArea32; break;
	}
      }

      ShadowFBInit(pScreen, refreshArea);
    }

    /* Setup default colourmap */
    if (!miCreateDefColormap(pScreen))
      return FALSE;

    /* Try the new code based on the new colormap layer */
    if (pScreenInfo->depth > 1)
      if (!xf86HandleColormaps(pScreen, 256, pScreenInfo->rgbBits,
			       renditionLoadPalette, NULL,
			       CMAP_LOAD_EVEN_IF_OFFSCREEN|
			       CMAP_RELOAD_ON_MODE_SWITCH)) {
        xf86DrvMsg(pScreenInfo->scrnIndex, X_ERROR, "Colormap initialization failed\n");
        return FALSE;
      }


#ifdef DPMSExtension
    xf86DPMSInit(pScreen, renditionDPMSSet, 0);
#endif


    if (xf86ReturnOptValBool(renditionOptions, OPTION_OVERCLOCK_MEM,0)) {
      RENDITIONPTR(pScreenInfo)->board.overclock_mem=TRUE;
    }
    xf86MarkOptionUsedByName(renditionOptions,"Overclock_Mem");

    /* Wrap the screen's CloseScreen vector and set its SaveScreen vector */
    prenditionPriv->CloseScreen = pScreen->CloseScreen;
    pScreen->CloseScreen = renditionCloseScreen;
    pScreen->SaveScreen = renditionSaveScreen;

    if (!Inited)
        renditionCloseScreen(scrnIndex, pScreen);

    if (serverGeneration == 1)
      xf86ShowUnusedOptions(pScreenInfo->scrnIndex, pScreenInfo->options);

#ifdef DEBUG
    ErrorF("ScreenInit OK...!!!!\n");
    sleep(1);
#endif
    return Inited;
}

static Bool
renditionSwitchMode(int scrnIndex, DisplayModePtr pMode, int flags)
{
#ifdef DEBUG
    ErrorF("RENDITION: renditionSwitchMode() called\n");
#endif
    return renditionSetMode(xf86Screens[scrnIndex], pMode);
}


static void
renditionAdjustFrame(int scrnIndex, int x, int y, int flags)
{
        ScrnInfoPtr pScreenInfo=xf86Screens[scrnIndex];
	renditionPtr pRendition = RENDITIONPTR(pScreenInfo);

        int offset, virtualwidth, bitsPerPixel;

#ifdef DEBUG
    ErrorF("RENDITION: renditionAdjustFrame() called\n");
#endif

    bitsPerPixel=pScreenInfo->bitsPerPixel;
    virtualwidth=pRendition->mode.virtualwidth;
    offset=(y*virtualwidth+x)*(bitsPerPixel>>3);

    offset+= pRendition->board.fbOffset;

#ifdef DEBUG
    ErrorF ("MOVING SCREEN %d bytes!!\n",offset);
#endif
    verite_setframebase(pScreenInfo, offset);
}


static Bool
renditionEnterVT(int scrnIndex, int flags)
{
#ifdef DEBUG
    ErrorF("RENDITION: renditionEnterVT() called\n");
#endif
     return renditionEnterGraphics(NULL, xf86Screens[scrnIndex]);
}


static void
renditionLeaveVT(int scrnIndex, int flags)
{
#ifdef DEBUG
    ErrorF("RENDITION: renditionLeaveVT() called\n");
#endif
    renditionLeaveGraphics(xf86Screens[scrnIndex]);
}


static void
renditionFreeScreen(int scrnIndex, int flags)
{
    renditionFreeRec(xf86Screens[scrnIndex]);
}


static ModeStatus
renditionValidMode(int scrnIndex, DisplayModePtr pMode, Bool Verbose, int flags)
{
    if (pMode->Flags & V_INTERLACE)
        return MODE_NO_INTERLACE;

    return MODE_OK;
}

static Bool
renditionMapMem(ScrnInfoPtr pScreenInfo)
{
  Bool WriteCombine;
  int mapOption;

#ifdef DEBUG
  ErrorF("Mapping ...\n");
  ErrorF("%d %d %d %x %d\n", pScreenInfo->scrnIndex, VIDMEM_FRAMEBUFFER, 
     RENDITIONPTR(pScreenInfo)->pcitag,
     RENDITIONPTR(pScreenInfo)->board.mem_base, pScreenInfo->videoRam);
#endif

  if (RENDITIONPTR(pScreenInfo)->board.chip==V1000_DEVICE){
    /* Some V1000 boards are known to have problems with Write-Combining */
    /* V2x00 also found to have similar problems with memcpy & WC ! */
    WriteCombine = 0;
  }
  else{
    /* Activate Write_Combine if possible */
    WriteCombine = 1;
  }
  /* Override on users request */
  WriteCombine=xf86ReturnOptValBool(renditionOptions, OPTION_FBWC, WriteCombine);
  if (WriteCombine){
    xf86DrvMsg(pScreenInfo->scrnIndex, X_CONFIG,
	       ("Requesting Write-Combined memory access\n"));
    mapOption = VIDMEM_FRAMEBUFFER;
  }
  else {
    xf86DrvMsg(pScreenInfo->scrnIndex, X_CONFIG,
	       ("Requesting MMIO-style memory access\n"));
    mapOption = VIDMEM_MMIO;
  }

  xf86MarkOptionUsedByName(renditionOptions,"FramebufferWC");

    RENDITIONPTR(pScreenInfo)->board.vmem_base=
        xf86MapPciMem(pScreenInfo->scrnIndex, mapOption,
        RENDITIONPTR(pScreenInfo)->pcitag,
        (unsigned long)RENDITIONPTR(pScreenInfo)->board.mem_base,
	pScreenInfo->videoRam);
    return TRUE;

#ifdef DEBUG0
    ErrorF("Done\n");
#endif
}

static Bool
renditionUnmapMem(ScrnInfoPtr pScreenInfo)
{
#ifdef DEBUG
  ErrorF("Unmapping ...\n");
#endif
    xf86UnMapVidMem(pScreenInfo->scrnIndex,
        RENDITIONPTR(pScreenInfo)->board.mem_base, pScreenInfo->videoRam);
    return TRUE;
#ifdef DEBUG0
    ErrorF("Done\n");
#endif
}

static void
renditionLoadPalette(ScrnInfoPtr pScreenInfo, int numColors,
		     int *indices, LOCO *colors,
		     VisualPtr pVisual)
{
  verite_setpalette(pScreenInfo, numColors, indices, colors, pVisual);
}


static xf86MonPtr
renditionDDC (ScrnInfoPtr pScreenInfo)
{
  renditionPtr pRendition = RENDITIONPTR(pScreenInfo);
  vu16 iob=pRendition->board.io_base;
  vu32 temp;

  xf86MonPtr MonInfo = NULL;
  temp = verite_in32(iob+CRTCCTL); /* Remember original value */

  /* Enable DDC1 */
  verite_out32(iob+CRTCCTL,(temp|
		       CRTCCTL_ENABLEDDC|
		       CRTCCTL_VSYNCENABLE|
		       CRTCCTL_VIDEOENABLE));

  MonInfo = xf86DoEDID_DDC1(pScreenInfo->scrnIndex,
			    vgaHWddc1SetSpeed,
			    renditionDDC1Read );

  verite_out32(iob+CRTCCTL,temp); /* return the original values */

  xf86DrvMsg(pScreenInfo->scrnIndex, X_INFO,
	     "DDC Monitor info: %p\n", MonInfo);

  xf86PrintEDID( MonInfo );
  xf86DrvMsg(pScreenInfo->scrnIndex, X_INFO,
	     "end of DDC Monitor info\n\n");

  /* xf86SetDDCproperties(pScreenInfo, MonInfo); */
  return MonInfo;
}

static unsigned int
renditionDDC1Read (ScrnInfoPtr pScreenInfo)
{
  renditionPtr pRendition = RENDITIONPTR(pScreenInfo);
  vu16 iob=pRendition->board.io_base;
  vu32 value = 0;

  /* wait for Vsync */
  while (!(verite_in32(iob+CRTCSTATUS) & CRTCSTATUS_VERT_SYNC));
  while (verite_in32(iob+CRTCSTATUS) & CRTCSTATUS_VERT_SYNC);

  /* Read the value */
  value = verite_in32(iob+CRTCCTL) & CRTCCTL_DDCDATA;
  return value;
}

void
renditionProbeDDC(ScrnInfoPtr pScreenInfo, int index)
{
  vbeInfoPtr pVbe;
  if (xf86LoadSubModule(pScreenInfo, "vbe")) {
    xf86LoaderReqSymLists(vbeSymbols, NULL);

    pVbe = VBEInit(NULL,index);
    ConfiguredMonitor = vbeDoEDID(pVbe, NULL);
  }
}
