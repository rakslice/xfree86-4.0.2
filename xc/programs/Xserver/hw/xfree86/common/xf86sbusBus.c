/*
 * SBUS bus-specific code.
 *
 * Copyright (C) 2000 Jakub Jelinek (jakub@redhat.com)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * JAKUB JELINEK BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
/* $XFree86: xc/programs/Xserver/hw/xfree86/common/xf86sbusBus.c,v 3.5 2000/12/06 15:35:11 eich Exp $ */

#include <ctype.h>
#include <stdio.h>
#include <unistd.h>
#include "X.h"
#include "os.h"
#include "xf86.h"
#include "xf86Priv.h"
#include "xf86_OSlib.h"
#include "xf86Resources.h"
#include "xf86cmap.h"

#include "xf86Bus.h"

#include "xf86sbusBus.h"
#include "xf86Sbus.h"

#define FB_DEV_PATH "/dev/fb%d"

Bool sbusSlotClaimed = FALSE;

sbusDevicePtr *xf86SbusInfo = NULL;
static int xf86nSbusInfo;

#ifndef FBTYPE_SUNGP3
#define FBTYPE_SUNGP3 -1
#endif
#ifndef FBTYPE_MDICOLOR
#define FBTYPE_MDICOLOR -1
#endif
#ifndef FBTYPE_SUNLEO
#define FBTYPE_SUNLEO -1
#endif
#ifndef FBTYPE_TCXCOLOR
#define FBTYPE_TCXCOLOR -1
#endif
#ifndef FBTYPE_CREATOR
#define FBTYPE_CREATOR -1
#endif

struct sbus_devtable sbusDeviceTable[] = {
    { SBUS_DEVICE_BW2, FBTYPE_SUN2BW, "bwtwo", "Sun Monochrome (bwtwo)" },
    { SBUS_DEVICE_CG2, FBTYPE_SUN2COLOR, "cgtwo", "Sun Color2 (cgtwo)" },
    { SBUS_DEVICE_CG3, FBTYPE_SUN3COLOR, "cgthree", "Sun Color3 (cgthree)" },
    { SBUS_DEVICE_CG4, FBTYPE_SUN4COLOR, "cgfour", "Sun Color4 (cgfour)" },
    { SBUS_DEVICE_CG6, FBTYPE_SUNFAST_COLOR, "cgsix", "Sun GX" },
    { SBUS_DEVICE_CG8, FBTYPE_MEMCOLOR, "cgeight", "Sun CG8/RasterOps" },
    { SBUS_DEVICE_CG12, FBTYPE_SUNGP3, "cgtwelve", "Sun GS (cgtwelve)" },
    { SBUS_DEVICE_CG14, FBTYPE_MDICOLOR, "cgfourteen", "Sun SX" },
    { SBUS_DEVICE_GT, FBTYPE_SUNGT, "gt", "Sun Graphics Tower" },
    { SBUS_DEVICE_MGX, -1, "mgx", "Quantum 3D MGXplus" },
    { SBUS_DEVICE_LEO, FBTYPE_SUNLEO, "leo", "Sun ZX or Turbo ZX" },
    { SBUS_DEVICE_TCX, FBTYPE_TCXCOLOR, "tcx", "Sun TCX" },
    { SBUS_DEVICE_FFB, FBTYPE_CREATOR, "ffb", "Sun FFB" },
    { SBUS_DEVICE_FFB, FBTYPE_CREATOR, "afb", "Sun Elite3D" },
    { 0, 0, NULL }
};

static void
CheckSbusDevice(const char *device, int fbNum)
{
    int fd, i;
    struct fbgattr fbattr;
    sbusDevicePtr psdp;

    fd = open(device, O_RDONLY, 0);
    if (fd < 0)
	return;
    memset(&fbattr, 0, sizeof(fbattr));
    if (ioctl(fd, FBIOGATTR, &fbattr) < 0) {
	if (ioctl(fd, FBIOGTYPE, &fbattr.fbtype) < 0) {
	    close(fd);
	    return;
	}
    }
    close(fd);
    for (i = 0; sbusDeviceTable[i].devId; i++)
	if (sbusDeviceTable[i].fbType == fbattr.fbtype.fb_type)
	    break;
    if (! sbusDeviceTable[i].devId)
	return;
    xf86SbusInfo = xnfrealloc(xf86SbusInfo, sizeof(psdp) * (++xf86nSbusInfo + 1));
    xf86SbusInfo[xf86nSbusInfo] = NULL;
    xf86SbusInfo[xf86nSbusInfo - 1] = psdp = xnfcalloc(sizeof (sbusDevice), 1);
    psdp->devId = sbusDeviceTable[i].devId;
    psdp->fbNum = fbNum;
    psdp->device = xnfstrdup(device);
    psdp->width = fbattr.fbtype.fb_width;
    psdp->height = fbattr.fbtype.fb_height;
    psdp->fd = -1;
}

void
xf86SbusProbe(void)
{
    int i, useProm = 0;
    char fbDevName[32];
    sbusDevicePtr psdp, *psdpp;

    xf86SbusInfo = xalloc(sizeof(psdp));
    *xf86SbusInfo = NULL;
    for (i = 0; i < 32; i++) {
	sprintf(fbDevName, FB_DEV_PATH, i);
	CheckSbusDevice(fbDevName, i);
    }
    if (sparcPromInit() >= 0) {
	useProm = 1;
	sparcPromAssignNodes();
    }
    for (psdpp = xf86SbusInfo, psdp = *psdpp; psdp; psdp = *++psdpp) {
	for (i = 0; sbusDeviceTable[i].devId; i++)
	    if (sbusDeviceTable[i].devId == psdp->devId)
		psdp->descr = sbusDeviceTable[i].descr;
	/* If we can use PROM information and found the PROM node for this device,
	 * we can tell more about the card.  */
	if (useProm && psdp->node.node) {
	    char *prop;
	    int len, chiprev, vmsize;

	    switch (psdp->devId) {
	    case SBUS_DEVICE_MGX:
		prop = sparcPromGetProperty(&psdp->node, "fb_size", &len);
		if (prop && len == 4 && *(int *)prop == 0x400000)
		    psdp->descr = "Quantum 3D MGXplus with 4M VRAM";
		break;
	    case SBUS_DEVICE_CG6:
		chiprev = 0;
		vmsize = 0;
		prop = sparcPromGetProperty(&psdp->node, "chiprev", &len);
		if (prop && len == 4)
		    chiprev = *(int *)prop;
		prop = sparcPromGetProperty(&psdp->node, "vmsize", &len);
		if (prop && len == 4)
		    vmsize = *(int *)prop;
		switch (chiprev) {
		case 1:
		case 2:
		case 3:
		case 4:
		    psdp->descr = "Sun Double width GX"; break;
		case 5:
		case 6:
		case 7:
		case 8:
		case 9:
		    psdp->descr = "Sun Single width GX"; break;
		case 11:
		    switch (vmsize) {
		    case 2: psdp->descr = "Sun Turbo GX with 1M VSIMM"; break;
		    case 4: psdp->descr = "Sun Turbo GX Plus"; break;
		    default: psdp->descr = "Sun Turbo GX"; break;
		    }
		}
		break;
	    case SBUS_DEVICE_CG14:
		prop = sparcPromGetProperty(&psdp->node, "reg", &len);
		vmsize = 0;
		if (prop && !(len % 12) && len > 0)
		    vmsize = *(int *)(prop + len - 4);
		switch (vmsize) {
		case 0x400000: psdp->descr = "Sun SX with 4M VSIMM"; break;
		case 0x800000: psdp->descr = "Sun SX with 8M VSIMM"; break;
		}
		break;
	    case SBUS_DEVICE_LEO:
		prop = sparcPromGetProperty(&psdp->node, "model", &len);
		if (prop && len > 0 && !strstr(prop, "501-2503"))
		    psdp->descr = "Sun Turbo ZX";
		break;
	    case SBUS_DEVICE_TCX:
		if (sparcPromGetBool(&psdp->node, "tcx-8-bit"))
		    psdp->descr = "Sun TCX (8bit)";
		else
		    psdp->descr = "Sun TCX (S24)";
		break;
	    case SBUS_DEVICE_FFB:
		prop = sparcPromGetProperty(&psdp->node, "name", &len);
		chiprev = 0;
		prop = sparcPromGetProperty(&psdp->node, "board_type", &len);
		if (prop && len == 4)
		    chiprev = *(int *)prop;
		if (strstr (prop, "afb")) {
		    if (chiprev == 3)
			psdp->descr = "Sun|Elite3D-M6 Horizontal";
		} else {
		    switch (chiprev) {
		    case 0x08: psdp->descr = "Sun FFB 67MHz Creator"; break;
		    case 0x0b: psdp->descr = "Sun FFB 67MHz Creator 3D"; break;
		    case 0x1b: psdp->descr = "Sun FFB 75MHz Creator 3D"; break;
		    case 0x20:
		    case 0x28: psdp->descr = "Sun FFB2 Vertical Creator"; break;
		    case 0x23:
		    case 0x2b: psdp->descr = "Sun FFB2 Vertical Creator 3D"; break;
		    case 0x30: psdp->descr = "Sun FFB2+ Vertical Creator"; break;
		    case 0x33: psdp->descr = "Sun FFB2+ Vertical Creator 3D"; break;
		    case 0x40:
		    case 0x48: psdp->descr = "Sun FFB2 Horizontal Creator"; break;
		    case 0x43:
		    case 0x4b: psdp->descr = "Sun FFB2 Horizontal Creator 3D"; break;
		    }
		}
		break;
	    }
	}
	if (useProm && psdp->node.node) {
	    char *promPath;
	    xf86Msg(X_PROBED, "SBUS:(0x%08x) %s", psdp->node.node, psdp->descr);
	    promPath = sparcPromNode2Pathname (&psdp->node);
	    if (promPath) {
		xf86ErrorF(" at %s", promPath);
		xfree(promPath);
	    }
	} else
	    xf86Msg(X_PROBED, "SBUS: %s", psdp->descr);
	xf86ErrorF("\n");
    }
    if (useProm)
	sparcPromClose();
}

/*
 * Parse a BUS ID string, and return the SBUS bus parameters if it was
 * in the correct format for a SBUS bus id.
 */

Bool
xf86ParseSbusBusString(const char *busID, int *fbNum)
{
    /*
     * The format is assumed to be one of:
     * "fbN", e.g. "fb1", which means the device corresponding to /dev/fbN
     * "nameN", e.g. "cgsix0", which means Nth instance of card NAME
     * "/prompath", e.g. "/sbus@0,10001000/cgsix@3,0" which is PROM pathname
     * to the device.
     */

    const char *id;
    int i, len;

    if (StringToBusType(busID, &id) != BUS_SBUS)
	return FALSE;

    if (*id != '/') {
	if (!strncmp (id, "fb", 2)) {
	    if (!isdigit(id[2]))
		return FALSE;
	    *fbNum = atoi(id + 2);
	    return TRUE;
	} else {
	    sbusDevicePtr *psdpp;
	    int devId;

	    for (i = 0, len = 0; sbusDeviceTable[i].devId; i++) {
		len = strlen(sbusDeviceTable[i].promName);
		if (!strncmp (sbusDeviceTable[i].promName, id, len)
		    && isdigit(id + len))
		    break;
	    }
	    devId = sbusDeviceTable[i].devId;
	    if (!devId) return FALSE;
	    i = atoi(id + len);
	    for (psdpp = xf86SbusInfo; *psdpp; ++psdpp) {
		if ((*psdpp)->devId != devId)
		    continue;
		if (!i) {
		    *fbNum = (*psdpp)->fbNum;
		    return TRUE;
		}
		i--;
	    }
	}
	return FALSE;
    }

    if (sparcPromInit() >= 0) {
	i = sparcPromPathname2Node(id);
	sparcPromClose();
	if (i) {
	    sbusDevicePtr *psdpp;
	    for (psdpp = xf86SbusInfo; *psdpp; ++psdpp) {
		if ((*psdpp)->node.node == i) {
		    *fbNum = (*psdpp)->fbNum;
		    return TRUE;
		}
	    }
	}
    }
    return FALSE;
}

/*
 * Compare a BUS ID string with a SBUS bus id.  Return TRUE if they match.
 */

Bool
xf86CompareSbusBusString(const char *busID, int fbNum)
{
    int iFbNum;

    if (xf86ParseSbusBusString(busID, &iFbNum)) {
	return fbNum == iFbNum;
    } else {
	return FALSE;
    }
}

/*
 * Check if the slot requested is free.  If it is already in use, return FALSE.
 */

Bool
xf86CheckSbusSlot(int fbNum)
{
    int i;
    EntityPtr p;

    for (i = 0; i < xf86NumEntities; i++) {
	p = xf86Entities[i];
	/* Check if this SBUS slot is taken */
	if (p->busType == BUS_SBUS && p->sbusBusId.fbNum == fbNum)
	    return FALSE;
    }

    return TRUE;
}

/*
 * If the slot requested is already in use, return -1.
 * Otherwise, claim the slot for the screen requesting it.
 */

int
xf86ClaimSbusSlot(sbusDevicePtr psdp, DriverPtr drvp,
		  GDevPtr dev, Bool active)
{
    EntityPtr p = NULL;

    int num;

    if (xf86CheckSbusSlot(psdp->fbNum)) {
        num = xf86AllocateEntity();
        p = xf86Entities[num];
        p->driver = drvp;
        p->chipset = -1;
        p->busType = BUS_SBUS;
        xf86AddDevToEntity(num, dev);
        p->sbusBusId.fbNum = psdp->fbNum;
        p->active = active;
        p->inUse = FALSE;
        /* Here we initialize the access structure */
        p->access = xnfcalloc(1,sizeof(EntityAccessRec));
	p->access->fallback = &AccessNULL;
        p->access->pAccess = &AccessNULL;
	sbusSlotClaimed = TRUE;
	return num;
    } else
	return -1;
}

int
xf86MatchSbusInstances(const char *driverName, int sbusDevId, 
		       GDevPtr *devList, int numDevs, DriverPtr drvp,
		       int **foundEntities)
{
    int i,j;
    sbusDevicePtr psdp, *psdpp;
    int numClaimedInstances = 0;
    int allocatedInstances = 0;
    int numFound = 0;
    GDevPtr devBus = NULL;
    GDevPtr dev = NULL;
    int *retEntities = NULL;
    int useProm = 0;

    struct Inst {
	sbusDevicePtr	sbus;
	GDevPtr		dev;
	Bool		claimed;  /* BusID matches with a device section */
    } *instances = NULL;

    *foundEntities = NULL;
    for (psdpp = xf86SbusInfo, psdp = *psdpp; psdp; psdp = *++psdpp) {
	if (psdp->devId != sbusDevId)
	    continue;
	if (psdp->fd == -2)
	    continue;
	++allocatedInstances;
	instances = xnfrealloc(instances,
			       allocatedInstances * sizeof(struct Inst));
	instances[allocatedInstances - 1].sbus = psdp;
	instances[allocatedInstances - 1].dev = NULL;
	instances[allocatedInstances - 1].claimed = FALSE;
	numFound++;
    }

    /*
     * This may be debatable, but if no SBUS devices with a matching vendor
     * type is found, return zero now.  It is probably not desirable to
     * allow the config file to override this.
     */
    if (allocatedInstances <= 0) {
	xfree(instances);
	return 0;
    }

    if (xf86DoProbe) {
	xfree(instances);
	return numFound;
    }

    if (sparcPromInit() >= 0)
	useProm = 1;

    if (xf86DoConfigure && xf86DoConfigurePass1) {
	GDevPtr pGDev;
	int actualcards = 0;
	for (i = 0; i < allocatedInstances; i++) {
	    actualcards++;
	    pGDev = xf86AddBusDeviceToConfigure(drvp->driverName, BUS_SBUS,
						instances[i].sbus, -1);
	    if (pGDev) {
		/*
		 * XF86Match???Instances() treat chipID and chipRev as
		 * overrides, so clobber them here.
		 */
		pGDev->chipID = pGDev->chipRev = -1;
	    }
	}
	xfree(instances);
	if (useProm)
	    sparcPromClose();
	return actualcards;
    }

#ifdef DEBUG
    ErrorF("%s instances found: %d\n", driverName, allocatedInstances);
#endif

    for (i = 0; i < allocatedInstances; i++) {
	char *promPath = NULL;

	psdp = instances[i].sbus;
	devBus = NULL;
	dev = NULL;
	if (useProm && psdp->node.node)
	    promPath = sparcPromNode2Pathname(&psdp->node);

	for (j = 0; j < numDevs; j++) {
	    if (devList[j]->busID && *devList[j]->busID) {
		if (xf86CompareSbusBusString(devList[j]->busID, psdp->fbNum)) {
		    if (devBus)
			xf86MsgVerb(X_WARNING,0,
			    "%s: More than one matching Device section for "
			    "instance (BusID: %s) found: %s\n",
			    driverName,devList[j]->identifier,
			    devList[j]->busID);
		    else
			devBus = devList[j];
		} 
	    } else {
		if (!dev && !devBus) {
		    if (promPath)
			xf86Msg(X_PROBED, "Assigning device section with no busID to SBUS:%s\n",
				promPath);
		    else
			xf86Msg(X_PROBED, "Assigning device section with no busID to SBUS:fb%d\n",
				psdp->fbNum);
		    dev = devList[j];
		} else
		    xf86MsgVerb(X_WARNING, 0,
			    "%s: More than one matching Device section "
			    "found: %s\n", driverName, devList[j]->identifier);
	    }
	}
	if (devBus) dev = devBus;  /* busID preferred */ 
	if (!dev && psdp->fd != -2) {
	    if (promPath) {
		xf86MsgVerb(X_WARNING, 0, "%s: No matching Device section "
			    "for instance (BusID SBUS:%s) found\n",
			    driverName, promPath);
	    } else
		xf86MsgVerb(X_WARNING, 0, "%s: No matching Device section "
			    "for instance (BusID SBUS:fb%d) found\n",
			    driverName, psdp->fbNum);
	} else if (dev) {
	    numClaimedInstances++;
	    instances[i].claimed = TRUE;
	    instances[i].dev = dev;
	}
	if (promPath)
	    xfree(promPath);
    }

#ifdef DEBUG
    ErrorF("%s instances found: %d\n", driverName, numClaimedInstances);
#endif

    /*
     * Of the claimed instances, check that another driver hasn't already
     * claimed its slot.
     */
    numFound = 0;
    for (i = 0; i < allocatedInstances && numClaimedInstances > 0; i++) {
	if (!instances[i].claimed)
	    continue;
	psdp = instances[i].sbus;
	if (!xf86CheckSbusSlot(psdp->fbNum))
	    continue;

#ifdef DEBUG
	ErrorF("%s: card at fb%d %08x is claimed by a Device section\n",
	       driverName, psdp->fbNum, psdp->node.node);
#endif
	
	/* Allocate an entry in the lists to be returned */
	numFound++;
	retEntities = xnfrealloc(retEntities, numFound * sizeof(int));
	retEntities[numFound - 1]
	    = xf86ClaimSbusSlot(psdp, drvp, instances[i].dev,instances[i].dev->active ?
				TRUE : FALSE);
    }
    xfree(instances);
    if (numFound > 0) {
	*foundEntities = retEntities;
    }

    if (useProm)
	sparcPromClose();

    return numFound;
}

/*
 * xf86GetSbusInfoForEntity() -- Get the sbusDevicePtr of entity.
 */
sbusDevicePtr
xf86GetSbusInfoForEntity(int entityIndex)
{
    sbusDevicePtr *psdpp;
    EntityPtr p = xf86Entities[entityIndex];

    if (entityIndex >= xf86NumEntities
	|| p->busType != BUS_SBUS) return NULL;

    for (psdpp = xf86SbusInfo; *psdpp != NULL; psdpp++) {
	if (p->sbusBusId.fbNum == (*psdpp)->fbNum)
	    return (*psdpp);
    }
    return NULL;
}

int
xf86GetEntityForSbusInfo(sbusDevicePtr psdp)
{
    int i;

    for (i = 0; i < xf86NumEntities; i++) {
	EntityPtr p = xf86Entities[i];
	if (p->busType != BUS_SBUS) continue;

	if (p->sbusBusId.fbNum == psdp->fbNum)
	    return i;
    }
    return -1;
}

void
xf86SbusUseBuiltinMode(ScrnInfoPtr pScrn, sbusDevicePtr psdp)
{
    DisplayModePtr mode;

    mode = xnfcalloc(sizeof(DisplayModeRec), 1);
    mode->name = "current";
    mode->next = mode;
    mode->prev = mode;
    mode->type = M_T_BUILTIN;
    mode->Clock = 100000000;
    mode->HDisplay = psdp->width;
    mode->HSyncStart = psdp->width;
    mode->HSyncEnd = psdp->width;
    mode->HTotal = psdp->width;
    mode->VDisplay = psdp->height;
    mode->VSyncStart = psdp->height;
    mode->VSyncEnd = psdp->height;
    mode->VTotal = psdp->height;
    mode->SynthClock = mode->Clock;
    mode->CrtcHDisplay = mode->HDisplay;
    mode->CrtcHSyncStart = mode->HSyncStart;
    mode->CrtcHSyncEnd = mode->HSyncEnd;
    mode->CrtcHTotal = mode->HTotal;
    mode->CrtcVDisplay = mode->VDisplay;
    mode->CrtcVSyncStart = mode->VSyncStart;
    mode->CrtcVSyncEnd = mode->VSyncEnd;
    mode->CrtcVTotal = mode->VTotal;
    mode->CrtcHAdjusted = FALSE;
    mode->CrtcVAdjusted = FALSE;
    pScrn->modes = mode;
    pScrn->virtualX = psdp->width;
    pScrn->virtualY = psdp->height;
}

static int sbusPaletteIndex = -1;
static unsigned long sbusPaletteGeneration = 0;
typedef struct _sbusCmap {
    sbusDevicePtr psdp;
    CloseScreenProcPtr CloseScreen;
    Bool origCmapValid;
    unsigned char origRed[16];
    unsigned char origGreen[16];
    unsigned char origBlue[16];
} sbusCmapRec, *sbusCmapPtr;

#define SBUSCMAPPTR(pScreen) ((sbusCmapPtr)((pScreen)->devPrivates[sbusPaletteIndex].ptr))

static void
xf86SbusCmapLoadPalette(ScrnInfoPtr pScrn, int numColors, int *indices,
			LOCO *colors, VisualPtr pVisual)
{
    int i, index;
    sbusCmapPtr cmap;
    struct fbcmap fbcmap;
    unsigned char *data = ALLOCATE_LOCAL(numColors*3);
                             
    cmap = SBUSCMAPPTR(pScrn->pScreen);
    if (!cmap) return;
    fbcmap.count = 0;
    fbcmap.index = indices[0];
    fbcmap.red = data;
    fbcmap.green = data + numColors;
    fbcmap.blue = fbcmap.green + numColors;
    for (i = 0; i < numColors; i++) {
	index = indices[i];
	if (fbcmap.count && index != fbcmap.index + fbcmap.count) {
	    ioctl (cmap->psdp->fd, FBIOPUTCMAP, &fbcmap);
	    fbcmap.count = 0;
	    fbcmap.index = index;
	}
	fbcmap.red[fbcmap.count] = colors[index].red;
	fbcmap.green[fbcmap.count] = colors[index].green;
	fbcmap.blue[fbcmap.count++] = colors[index].blue;
    }
    ioctl (cmap->psdp->fd, FBIOPUTCMAP, &fbcmap);
    DEALLOCATE_LOCAL(data);
}

static Bool
xf86SbusCmapCloseScreen(int i, ScreenPtr pScreen)
{
    sbusCmapPtr cmap;
    struct fbcmap fbcmap;
                         
    cmap = SBUSCMAPPTR(pScreen);
    if (cmap->origCmapValid) {
	fbcmap.index = 0;
	fbcmap.count = 16;
	fbcmap.red = cmap->origRed;
	fbcmap.green = cmap->origGreen;
	fbcmap.blue = cmap->origBlue;
	ioctl (cmap->psdp->fd, FBIOPUTCMAP, &fbcmap);
    }
    pScreen->CloseScreen = cmap->CloseScreen;
    xfree (cmap);
    return (*pScreen->CloseScreen) (i, pScreen);
}    

Bool
xf86SbusHandleColormaps(ScreenPtr pScreen, sbusDevicePtr psdp)
{
    sbusCmapPtr cmap;
    struct fbcmap fbcmap;
    unsigned char data[2];

    if(sbusPaletteGeneration != serverGeneration) {
	if((sbusPaletteIndex = AllocateScreenPrivateIndex()) < 0)
	    return FALSE;
	sbusPaletteGeneration = serverGeneration;
    }
    cmap = xnfcalloc(1, sizeof(sbusCmapRec));
    pScreen->devPrivates[sbusPaletteIndex].ptr = cmap;
    cmap->psdp = psdp;
    fbcmap.index = 0;
    fbcmap.count = 16;
    fbcmap.red = cmap->origRed;
    fbcmap.green = cmap->origGreen;
    fbcmap.blue = cmap->origBlue;
    if (ioctl (psdp->fd, FBIOGETCMAP, &fbcmap) >= 0)
	cmap->origCmapValid = TRUE;
    fbcmap.index = 0;
    fbcmap.count = 2;
    fbcmap.red = data;
    fbcmap.green = data;
    fbcmap.blue = data;
    if (pScreen->whitePixel == 0) {
	data[0] = 255;
	data[1] = 0;
    } else {
	data[0] = 0;
	data[1] = 255;
    }
    ioctl (psdp->fd, FBIOPUTCMAP, &fbcmap);
    cmap->CloseScreen = pScreen->CloseScreen;
    pScreen->CloseScreen = xf86SbusCmapCloseScreen;
    return xf86HandleColormaps(pScreen, 256, 8,
			       xf86SbusCmapLoadPalette, NULL, 0);
}
