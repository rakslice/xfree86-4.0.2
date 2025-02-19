/*
 * Id: newport_driver.c,v 1.2 2000/11/29 20:58:10 agx Exp $ 
 *
 * Driver for the SGI Indy's Newport graphics card
 * 
 * This driver is based on the newport.c & newport_con.c kernel code
 *
 * (c) 2000 Guido Guenther <guido.guenther@gmx.net>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is fur-
 * nished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FIT-
 * NESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * XFREE86 PROJECT BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CON-
 * NECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name of the XFree86 Project shall not
 * be used in advertising or otherwise to promote the sale, use or other deal-
 * ings in this Software without prior written authorization from the XFree86
 * Project.
 *
 */
/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/newport/newport_driver.c,v 1.6 2000/12/14 20:59:12 dawes Exp $ */

/* function prototypes, common data structures & generic includes */
#include "newport.h"

/* Drivers using the mi SW cursor need: */
#include "mipointer.h"
/* Drivers using the mi implementation of backing store need: */
#include "mibstore.h"
/* Drivers using the mi colourmap code need: */
#include "micmap.h"

/* Drivers using cfb need: */
#define PSZ 8
#include "cfb.h"
#undef PSZ
#include "cfb24.h"
#include "cfb24_32.h"

/* Drivers using the shadow frame buffer need: */
#include "shadowfb.h"

/* Xv Extension */
#include "xf86xv.h"
#include "Xv.h"

/* Temporary workaround.  A module really shouldn't need this */
#ifndef XFree86LOADER
# include "xf86_OSlib.h"
# ifndef MAP_FAILED
#  define MAP_FAILED ((pointer)(-1))
# endif
#endif

#define VERSION			4000
#define NEWPORT_NAME		"Newport"
#define NEWPORT_DRIVER_NAME	"newport"
#define NEWPORT_MAJOR_VERSION	0
#define NEWPORT_MINOR_VERSION	1	
#define NEWPORT_PATCHLEVEL	1


/* Prototypes ------------------------------------------------------- */
static void	NewportIdentify(int flags);
static OptionInfoPtr NewportAvailableOptions(int chipid, int busid);
static Bool NewportProbe(DriverPtr drv, int flags);
static Bool NewportPreInit(ScrnInfoPtr pScrn, int flags);
static Bool NewportScreenInit(int Index, ScreenPtr pScreen, int argc, char **argv);
static Bool NewportEnterVT(int scrnIndex, int flags);
static void NewportLeaveVT(int scrnIndex, int flags);
static Bool NewportCloseScreen(int scrnIndex, ScreenPtr pScreen);
static Bool NewportSaveScreen(ScreenPtr pScreen, int mode);
static unsigned NewportHWProbe(unsigned probedIDs[]);	/* return number of found boards */
static Bool NewportModeInit(ScrnInfoPtr pScrn, DisplayModePtr mode);
static void NewportRestore(ScrnInfoPtr pScrn, Bool Closing);
static Bool NewportGetRec(ScrnInfoPtr pScrn);
static Bool NewportFreeRec(ScrnInfoPtr pScrn);
static Bool NewportMapRegs(ScrnInfoPtr pScrn);
static void NewportUnmapRegs(ScrnInfoPtr pScrn);
static Bool NewportProbeCardInfo(ScrnInfoPtr pScrn);
/* ------------------------------------------------------------------ */

DriverRec NEWPORT = {
        VERSION,
	NEWPORT_DRIVER_NAME,
        NewportIdentify,
        NewportProbe,
        NewportAvailableOptions,
        NULL,
	0
};

/* Supported "chipsets" */
#define CHIP_XL		0x1

static SymTabRec NewportChipsets[] = {
    { CHIP_XL, "XL" },
    {-1, NULL }
};

/* List of Symbols from other modules that this module references */

static const char *cfbSymbols[] = {
	"cfbScreenInit",
	NULL
};	

static const char *shadowSymbols[] = {
	"ShadowFBInit",
	NULL
};

#ifdef XFree86LOADER

static MODULESETUPPROTO(newportSetup);

static XF86ModuleVersionInfo newportVersRec =
{
	"newport",
	MODULEVENDORSTRING,
	MODINFOSTRING1,
	MODINFOSTRING2,
	XF86_VERSION_CURRENT,
	NEWPORT_MAJOR_VERSION, NEWPORT_MINOR_VERSION, NEWPORT_PATCHLEVEL,
	ABI_CLASS_VIDEODRV,
	ABI_VIDEODRV_VERSION,
	MOD_CLASS_VIDEODRV,
	{0,0,0,0}
};

XF86ModuleData newportModuleData = { &newportVersRec, newportSetup, NULL };

static pointer
newportSetup(pointer module, pointer opts, int *errmaj, int *errmin)
{
	static Bool setupDone = FALSE;

/* This module should be loaded only once, but check to be sure. */
	if (!setupDone) {
		/*
		 * Modules that this driver always requires may be loaded
		 * here  by calling LoadSubModule().
		 */
		setupDone = TRUE;
		xf86AddDriver(&NEWPORT, module, 0);

		/*
		 * Tell the loader about symbols from other modules that this module
		 * might refer to.
		 *
		 */
		LoaderRefSymLists( cfbSymbols, shadowSymbols, NULL);


		/*
		 * The return value must be non-NULL on success even though
		 * there is no TearDownProc.
		 */
	return (pointer)1;
       } else {
	if (errmaj) *errmaj = LDR_ONCEONLY;
	return NULL;
       }
}

#endif /* XFree86LOADER */

typedef enum {
	OPTION_BITPLANES,
	OPTION_BUS_ID
} NewportOpts;

/* Supported options */
static OptionInfoRec NewportOptions [] = {
	{ OPTION_BITPLANES, "bitplanes", OPTV_INTEGER, {0}, FALSE },
	{ OPTION_BUS_ID, "BusID", OPTV_INTEGER, {0}, FALSE },
	{ -1, NULL, OPTV_NONE, {0}, FALSE }
};

/* ------------------------------------------------------------------ */

static Bool
NewportGetRec(ScrnInfoPtr pScrn)
{
	NewportPtr pNewport;
	if (pScrn->driverPrivate != NULL)
		return TRUE;
	pScrn->driverPrivate = xnfcalloc(sizeof(NewportRec), 1);
	
	pNewport = NEWPORTPTR(pScrn);
	pNewport->pNewportRegs = NULL;

	return TRUE;
}

static Bool
NewportFreeRec(ScrnInfoPtr pScrn)
{
	if (pScrn->driverPrivate == NULL)
		return TRUE;
	xfree(pScrn->driverPrivate);
	pScrn->driverPrivate = NULL;
	return TRUE;
}

static void
NewportIdentify(int flags)
{
	xf86PrintChipsets( NEWPORT_NAME, "driver for Newport Graphics Card", NewportChipsets);
}

static Bool
NewportProbe(DriverPtr drv, int flags)
{
	int numDevSections, numUsed, i, j, busID;
	Bool foundScreen = FALSE;
	GDevPtr *devSections;
	GDevPtr dev = NULL;
	resRange range[] = { {ResExcMemBlock ,0,0}, _END };
	unsigned probedIDs[NEWPORT_MAX_BOARDS];
	memType base;

	if ((numDevSections = xf86MatchDevice(NEWPORT_DRIVER_NAME, &devSections)) <= 0) 
                return FALSE;
	numUsed = NewportHWProbe(probedIDs);
	if ( numUsed <= 0 ) 
		return FALSE;

	if(flags & PROBE_DETECT) 
		foundScreen = TRUE;
	else {
		for (i = 0; i < numDevSections; i++) {
			dev = devSections[i];
			busID =  xf86SetIntOption(dev->options, "BusID", 0);

			for( j = 0; j < numUsed; j++) {
				if ( busID == probedIDs[j] ) {
					int entity;
					ScrnInfoPtr pScrn = NULL;

					/* This is a hack because don't have the RAC info(and don't want it).  
					 * Set it as an ISA entity to get the entity field set up right.
					 */
					entity = xf86ClaimIsaSlot(drv, 0, dev, TRUE);
					base = (NEWPORT_BASE_ADDR0 + busID * NEWPORT_BASE_OFFSET);
					RANGE(range[0], base, base + sizeof(NewportRegs),\
							ResExcMemBlock);
					pScrn = xf86ConfigIsaEntity(pScrn, 0, entity, NULL, range, \
							NULL, NULL, NULL, NULL);
					/* Allocate a ScrnInfoRec */
					pScrn->driverVersion = VERSION;
					pScrn->driverName    = NEWPORT_DRIVER_NAME;
					pScrn->name          = NEWPORT_NAME;
					pScrn->Probe         = NewportProbe;
					pScrn->PreInit       = NewportPreInit;
					pScrn->ScreenInit    = NewportScreenInit;
					pScrn->EnterVT       = NewportEnterVT;
					pScrn->LeaveVT       = NewportLeaveVT;
					pScrn->driverPrivate = (void*)busID;
					foundScreen = TRUE;
					break;
				}
			}
		}
	}
	xfree(devSections);
	return foundScreen;
}

/* most of this is from DESIGN.TXT s20.3.6 */
static Bool 
NewportPreInit(ScrnInfoPtr pScrn, int flags)
{
	int i, busID;
	NewportPtr pNewport;
	MessageType from;
	ClockRangePtr clockRanges;
	char *mod=0, *reqSym=0;

	if (flags & PROBE_DETECT) return FALSE;

	if (pScrn->numEntities != 1)
		return FALSE;

	busID = (int)(pScrn->driverPrivate);
	pScrn->driverPrivate = NULL;
	
	/* Fill in the monitor field */
	pScrn->monitor = pScrn->confScreen->monitor;

	if (!xf86SetDepthBpp(pScrn, 8, 8, 8, 
			Support24bppFb | SupportConvert32to24 | 
				PreferConvert32to24 ))
		return FALSE;

	switch( pScrn->depth ) {
			/* check if the returned depth is one we support */
		case 8:
			/* OK */
			break;
		default: 
			xf86DrvMsg(pScrn->scrnIndex, X_ERROR, 
			"Given depth (%d) is not supported by Newport driver\n",
			pScrn->depth);
			return FALSE;
	}
	xf86PrintDepthBpp(pScrn);

	/* Set bits per RGB for 8bpp */
	if( pScrn->depth == 8)
		pScrn->rgbBits = 8;

	/* Set Default Weight */
	if( pScrn->depth > 8 ) {
		rgb zeros = {0, 0, 0};
		if (!xf86SetWeight(pScrn, zeros, zeros))
			return FALSE;
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

	{ /* Set default Gamma */
		Gamma zeros = {0.0, 0.0, 0.0};

		if (!xf86SetGamma(pScrn, zeros)) {
			return FALSE;
         	}
	}

	/* Allocate the NewportRec driverPrivate */
	if (!NewportGetRec(pScrn)) {
		return FALSE;
	} 
	pNewport = NEWPORTPTR(pScrn);
	pNewport->busID = busID;

	/* We use a programamble clock */
	pScrn->progClock = TRUE;

	/* Fill in pScrn->options) */
	xf86CollectOptions(pScrn, NULL);
	xf86ProcessOptions(pScrn->scrnIndex, pScrn->options, NewportOptions);

	/* Set fields in ScreenInfoRec && NewportRec */
    	pScrn->videoRam = 1280 * (pScrn->bitsPerPixel >> 3);

	/* get revisions of REX3, etc. */
	if( ! NewportMapRegs(pScrn))
		return FALSE;
	NewportProbeCardInfo(pScrn);
	NewportUnmapRegs(pScrn);

	from=X_PROBED;
	xf86DrvMsg(pScrn->scrnIndex, from,
		"Newport Graphics Revisions: Board: %d, Rex3: %d, Cmap: %c, Xmap9: %d\n",
		pNewport->board_rev, pNewport->rex3_rev, 
		pNewport->cmap_rev, pNewport->xmap9_rev);

	if ( (xf86GetOptValInteger(NewportOptions, OPTION_BITPLANES, &pNewport->bitplanes)))
	from = X_CONFIG;
	xf86DrvMsg(pScrn->scrnIndex, from, "Newport has %d bitplanes\n", pNewport->bitplanes);

	if ( pScrn->depth > pNewport->bitplanes ) {
		xf86DrvMsg(pScrn->scrnIndex, X_ERROR, \
			"Display depth(%d) > number of bitplanes on Newport board(%d)\n", \
			pScrn->depth, pNewport->bitplanes);
		return FALSE;
	}
	if ( ( pNewport->bitplanes != 8 ) && ( pNewport->bitplanes != 24 ) ) {
		xf86DrvMsg(pScrn->scrnIndex, X_ERROR, \
			"Number of bitplanes on newport must be either 8 or 24 not %d\n", \
			pNewport->bitplanes);
		return FALSE;
	}
	
	/* Set up clock ranges that are alway ok */
	/* XXX: Should use the correct data from the specs(which specs?) here */
	clockRanges = xnfalloc(sizeof(ClockRange));
	clockRanges->next = NULL;
	clockRanges->minClock = 10000;
	clockRanges->maxClock = 300000;
	clockRanges->clockIndex = -1;         /* programmable */
	clockRanges->interlaceAllowed = TRUE;
	clockRanges->doubleScanAllowed = TRUE;
	
	/* see above note */
	/* There is currently only an 1280x1024 mode */
	i = xf86ValidateModes(pScrn, pScrn->monitor->Modes,
			pScrn->display->modes, clockRanges, 
			NULL, 256, 2048,
			pScrn->bitsPerPixel, 128, 2048,
			pScrn->display->virtualX,
			pScrn->display->virtualY,
			pScrn->videoRam * 1024,
			LOOKUP_BEST_REFRESH);

	if (i == -1) { 
		NewportFreeRec(pScrn); 
		return FALSE;
	}

	xf86PruneDriverModes(pScrn);
	if( i == 0 || pScrn->modes == NULL) {
		xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "No valid modes found\n");
		NewportFreeRec(pScrn);
		return FALSE;
	}

	/* unnecessary, but do it to get a valid ScrnInfoRec */
	xf86SetCrtcForModes(pScrn, INTERLACE_HALVE_V);
	
	/* Set the current mode to the first in the list */
	pScrn->currentMode = pScrn->modes;
	
  	/* Print the list of modes being used */
	xf86PrintModes(pScrn);
	xf86SetDpi (pScrn, 0, 0);

	switch(pScrn->bitsPerPixel) {
		case 8: 
			mod = "cfb";
			reqSym = "cfbScreenInit";
			break;
	}
	if ( mod && (!xf86LoadSubModule(pScrn, mod))) {
		NewportFreeRec(pScrn);
		return FALSE;
	}
	xf86LoaderReqSymbols( reqSym, NULL);

	/* Load ShadowFB module */
	if (!xf86LoadSubModule(pScrn, "shadowfb")) {
		NewportFreeRec(pScrn);
		return FALSE;
	}
	xf86LoaderReqSymLists(shadowSymbols, NULL);

	return TRUE;
}

static Bool 
NewportScreenInit(int index, ScreenPtr pScreen, int argc, char **argv)
{
	ScrnInfoPtr pScrn;
	NewportPtr pNewport;
	VisualPtr visual;
	BOOL ret;
	int i;

	/* First get a pointer to our private info */
	pScrn = xf86Screens[pScreen->myNum];
	pNewport = NEWPORTPTR(pScrn);

	/* map the Newportregs until the server dies */
	if( ! NewportMapRegs(pScrn)) 
		return FALSE;

	/* Reset visual list. */
	miClearVisualTypes();

	if (!miSetVisualTypes(pScrn->depth, pScrn->depth != 8 ? TrueColorMask :
					miGetDefaultVisualMask(pScrn->depth),
				pScrn->rgbBits, pScrn->defaultVisual))
		return FALSE;
	
	pNewport->Bpp = pScrn->bitsPerPixel >> 3;
	/* Setup the stuff for the shadow framebuffer */
	pNewport->ShadowPitch = (( pScrn->virtualX * pNewport->Bpp ) + 3) & ~3L;
	pNewport->ShadowPtr = xnfalloc(pNewport->ShadowPitch * pScrn->virtualY);
	
	if (!NewportModeInit(pScrn, pScrn->currentMode))
			return FALSE;

	switch( pScrn->bitsPerPixel) {
		case 8:
			ret=cfbScreenInit(pScreen, pNewport->ShadowPtr,
				pScrn->virtualX, pScrn->virtualY,
				pScrn->xDpi, pScrn->yDpi,
				pScrn->displayWidth);
			break;
		default:
			xf86Msg(X_ERROR,
				"Internal Error: Display depth not supported in NewportScreenInit.\n");
			ret=FALSE;
			break;
	}

	if(!ret)
		return FALSE;

	/* we need rgb ordering if bitsPerPixel > 8 */
	if (pScrn->bitsPerPixel > 8) {
		for (i = 0, visual = pScreen->visuals;
			i < pScreen->numVisuals; i++, visual++) {
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


	miInitializeBackingStore(pScreen);
	xf86SetBackingStore(pScreen);

	xf86SetBlackWhitePixels(pScreen);

	/* Initialize software cursor */
	if(!miDCInitialize(pScreen, xf86GetPointerScreenFuncs()))
		return FALSE;
	
	/* Initialise default colourmap */
	if (!miCreateDefColormap(pScreen))
		return FALSE;

	/* Install our LoadPalette funciton */
	if(!xf86HandleColormaps(pScreen, 256, 8, NewportLoadPalette, 0,
				CMAP_RELOAD_ON_MODE_SWITCH ))
		return FALSE;

	/* Initialise shadow frame buffer */
	ShadowFBInit(pScreen, &NewportRefreshArea8);

#ifdef XvExtension
	{
		XF86VideoAdaptorPtr *ptr;
		int n;

		n = xf86XVListGenericAdaptors(pScrn,&ptr);
		if (n) {
			xf86XVScreenInit(pScreen, ptr, n);
	        }
        }
#endif


	pScreen->SaveScreen = NewportSaveScreen;
	/* Wrap the current CloseScreen function */
	pNewport->CloseScreen = pScreen->CloseScreen;
	pScreen->CloseScreen = NewportCloseScreen;

	if (serverGeneration == 1) {
		xf86ShowUnusedOptions(pScrn->scrnIndex, pScrn->options);
	}
	
	return TRUE;
}

/* called when switching away from a VT */
static Bool
NewportEnterVT(int scrnIndex, int flags)
{
	ScrnInfoPtr pScrn = xf86Screens[scrnIndex];
	return NewportModeInit(pScrn, pScrn->currentMode);
}

/* called when switching to a VT */
static void
NewportLeaveVT(int scrnIndex, int flags)
{
	ScrnInfoPtr pScrn = xf86Screens[scrnIndex];
	NewportRestore(pScrn, FALSE);
}

/* called at the end of each server generation */
static Bool
NewportCloseScreen(int scrnIndex, ScreenPtr pScreen)
{
	ScrnInfoPtr pScrn = xf86Screens[scrnIndex];
	NewportPtr pNewport = NEWPORTPTR(pScrn);

	NewportRestore(pScrn, TRUE);
	if (pNewport->ShadowPtr)
		xfree(pNewport->ShadowPtr);

	/* unmap the Newport's registers from memory */
	NewportUnmapRegs(pScrn);
	pScrn->vtSema = FALSE;
 
	pScreen->CloseScreen = pNewport->CloseScreen;
	return (*pScreen->CloseScreen)(scrnIndex, pScreen);
}

/* Blank or unblank the screen */ 
static Bool
NewportSaveScreen(ScreenPtr pScreen, int mode)
{
	ScrnInfoPtr pScrn;
	NewportRegsPtr pNewportRegs;
	Bool unblank;
	unsigned short treg;

	unblank = xf86IsUnblank(mode);
	pScrn = xf86Screens[pScreen->myNum];
	pNewportRegs = NEWPORTPTR(pScrn)->pNewportRegs;
	
	if (unblank) {
		treg = NewportVc2Get(pNewportRegs, VC2_IREG_CONTROL);
	        NewportVc2Set( pNewportRegs, VC2_IREG_CONTROL, (treg | VC2_CTRL_EDISP));
    	} else {
		treg = NewportVc2Get(pNewportRegs, VC2_IREG_CONTROL);
		NewportVc2Set( pNewportRegs, VC2_IREG_CONTROL, (treg & ~(VC2_CTRL_EDISP)));
        }
	return TRUE;
}


static OptionInfoPtr
NewportAvailableOptions(int chipid, int busid)
{
	return NewportOptions;
}


/* This sets up the actual mode on the Newport */
static Bool 
NewportModeInit(ScrnInfoPtr pScrn, DisplayModePtr mode)
{
	int width, height;
	NewportPtr pNewport = NEWPORTPTR(pScrn);
	NewportRegsPtr pNewportRegs = NEWPORTREGSPTR(pScrn);

	width = mode->HDisplay;
	height = mode->VDisplay;
	if (width != 1280 || height != 1024) {
		xf86DrvMsg(pScrn->scrnIndex, X_ERROR, \
		"Width = %d and height = %d is not supported by by this driver\n", width, height);
	}

	pScrn->vtSema=TRUE;
	/* first backup the necessary registers... */
	pNewport->txt_drawmode1 = pNewportRegs->set.drawmode1;
	pNewport->txt_vc2ctrl = NewportVc2Get( pNewportRegs, VC2_IREG_CONTROL);
	NewportBackupPalette(pScrn);

	/* ...then  setup the hardware */
	/*
	 * XXX: set the frambuffer layout to either 24 or 8 bpp here - more specs needed
	 * XXX: Lazy mode on: simply rely on the prom since it does such a good job
	 */
	if( pNewport->Bpp == 1) {
		pNewport->drawmode1 = pNewport->txt_drawmode1;
	} 
	
	return TRUE;
}


/* 
 * This will acutally restore the saved state 
 * (either when switching back to a VT or when the server is going down)
 * Closing is true if the X server is really going down 
 */
static void
NewportRestore(ScrnInfoPtr pScrn, Bool Closing)
{
	NewportPtr pNewport = NEWPORTPTR(pScrn);
	NewportRegsPtr pNewportRegs = pNewport->pNewportRegs;

	/* Restore backed up registers */
	pNewportRegs->set.drawmode1 = pNewport->txt_drawmode1;
	NewportVc2Set( pNewportRegs, VC2_IREG_CONTROL, pNewport->txt_vc2ctrl);
	NewportRestorePalette(pScrn);
}


/* Probe for the Newport card ;) */
/* XXX: we need a better probe here in order to support multihead! */
static unsigned
NewportHWProbe(unsigned probedIDs[])
{
	FILE* cpuinfo;		
	char line[80];
	unsigned hasNewport = 0;
	cpuinfo = fopen("/proc/cpuinfo","r");
	while(fgets(line,80,cpuinfo) != NULL) {	
		if(strstr(line, "SGI Indy") != NULL) {
			hasNewport = 1;
			break;
		}
	}
	fclose(cpuinfo);	

	probedIDs[0] = 0;
	return hasNewport;	
}

/* Probe for Chipset revisions */
static Bool NewportProbeCardInfo(ScrnInfoPtr pScrn)
{
	unsigned int tmp,cmap_rev;
	NewportPtr pNewport = NEWPORTPTR(pScrn);
	NewportRegsPtr pNewportRegs = pNewport->pNewportRegs;

	NewportWait(pNewportRegs); 
	pNewportRegs->set.dcbmode = (DCB_CMAP0 | NCMAP_PROTOCOL |
			NCMAP_REGADDR_RREG | NPORT_DMODE_W1);
	tmp = pNewportRegs->set.dcbdata0.bytes.b3;
	pNewport->board_rev = (tmp >> 4) & 7;
	pNewport->bitplanes = ((pNewport->board_rev > 1) && (tmp & 0x80)) ? 8 : 24;
	cmap_rev = tmp & 7;
	pNewport->cmap_rev = (char)('A'+(cmap_rev ? (cmap_rev+1):0));
	pNewport->rex3_rev = (pNewportRegs->cset.ustat) & 7;

	pNewportRegs->set.dcbmode = (DCB_XMAP0 | R_DCB_XMAP9_PROTOCOL |
					XM9_CRS_REVISION | NPORT_DMODE_W1);
	pNewport->xmap9_rev = (pNewportRegs->set.dcbdata0.bytes.b3) & 7;
	
	return TRUE;
}


/* map NewportRegs */
static Bool
NewportMapRegs(ScrnInfoPtr pScrn)
{
	NewportPtr pNewport = NEWPORTPTR(pScrn);

	pNewport->pNewportRegs = xf86MapVidMem(pScrn->scrnIndex, 
			VIDMEM_MMIO,
			NEWPORT_BASE_ADDR0 + pNewport->busID * NEWPORT_BASE_OFFSET,
			 sizeof(NewportRegs));
	if ( ! pNewport->pNewportRegs ) 
		return FALSE;
	return TRUE;
}

/* unmap NewportRegs */
static void
NewportUnmapRegs(ScrnInfoPtr pScrn)
{
	NewportPtr pNewport = NEWPORTPTR(pScrn);

	xf86UnMapVidMem( pScrn->scrnIndex, pNewport->pNewportRegs,
			sizeof(NewportRegs));
	pNewport->pNewportRegs = NULL;
}
