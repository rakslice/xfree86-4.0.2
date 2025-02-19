/* $XFree86: xc/programs/Xserver/hw/xfree86/common/xf86pciBus.c,v 3.28 2000/12/08 20:13:35 eich Exp $ */

/*
 * Copyright (c) 1997-1999 by The XFree86 Project, Inc.
 */

/*
 * This file contains the interfaces to the bus-specific code
 */
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include "X.h"
#include "os.h"
#include "xf86Pci.h"
#include "xf86.h"
#include "xf86Priv.h"
#include "xf86Resources.h"
#include "xf86ScanPci.h"

/* Bus-specific headers */
#undef  DECLARE_CARD_DATASTRUCTURES
#define DECLARE_CARD_DATASTRUCTURES TRUE
#include "xf86PciInfo.h"
#include "xf86ScanPci.h"

#include "xf86Bus.h"

#define XF86_OS_PRIVS
#define NEED_OS_RAC_PROTOS
#include "xf86_OSproc.h"

#include "xf86RAC.h"

/* Bus-specific globals */
Bool pciSlotClaimed = FALSE;
pciConfigPtr *xf86PciInfo = NULL;		/* Full PCI probe info */
pciVideoPtr *xf86PciVideoInfo = NULL;		/* PCI probe for video hw */
pciAccPtr * xf86PciAccInfo = NULL;              /* PCI access related */

static resPtr pciAvoidRes = NULL;

/* PCI buses */
static PciBusPtr xf86PciBus = NULL;
/* Bus-specific probe/sorting functions */

/* PCI classes that get included in xf86PciVideoInfo */
#define PCIINFOCLASSES(b,s)						      \
    (((b) == PCI_CLASS_PREHISTORIC) ||					      \
     ((b) == PCI_CLASS_DISPLAY) ||					      \
     ((b) == PCI_CLASS_MULTIMEDIA && (s) == PCI_SUBCLASS_MULTIMEDIA_VIDEO) || \
     ((b) == PCI_CLASS_PROCESSOR && (s) == PCI_SUBCLASS_PROCESSOR_COPROC))

/*
 * PCI classes that have messages printed always.  The others are only
 * have a message printed when the vendor/dev IDs are recognised.
 */
#define PCIALWAYSPRINTCLASSES(b,s)					      \
    (((b) == PCI_CLASS_PREHISTORIC && (s) == PCI_SUBCLASS_PREHISTORIC_VGA) || \
     ((b) == PCI_CLASS_DISPLAY) ||					      \
     ((b) == PCI_CLASS_MULTIMEDIA && (s) == PCI_SUBCLASS_MULTIMEDIA_VIDEO))
 
/*
 * PCI classes for which potentially destructive checking of the map sizes
 * may be done.  Any classes where this may be unsafe should be omitted
 * from this list.
 */
#define PCINONSYSTEMCLASSES(b,s) PCIALWAYSPRINTCLASSES(b,s)

/* 
 * PCI classes that use RAC 
 */
#define PCISHAREDIOCLASSES(b,s)					      \
    (((b) == PCI_CLASS_PREHISTORIC && (s) == PCI_SUBCLASS_PREHISTORIC_VGA) || \
     ((b) == PCI_CLASS_DISPLAY && (s) == PCI_SUBCLASS_DISPLAY_VGA))

#define PCI_MEM32_LENGTH_MAX 0xFFFFFFFF

#undef MIN
#define MIN(x,y) ((x<y)?x:y)

#define B2M(tag,base) pciBusAddrToHostAddr(tag,PCI_MEM,base)
#define B2I(tag,base) (base)
#define B2H(tag,base,type) ((type & ResMem) ? (B2M(tag,base)) : (B2I(tag,base)))
#define M2B(tag,base) pciHostAddrToBusAddr(tag,PCI_IO,base)
#define I2B(tag,base) (base)
#define H2B(tag,base,type) (type & ResMem) ? M2B(tag,base) : I2B(tag,base)
#define TAG(pvp) (pciTag(pvp->bus,pvp->device,pvp->func))
#define SIZE(size) ((1 << size) - 1)
#define PCI_SIZE(type,tag,size) ((type & ResMem) \
                        ? pciBusAddrToHostAddr(tag,PCI_MEM_SIZE,size) \
                        : pciBusAddrToHostAddr(tag,PCI_IO_SIZE,size))
#define PCI_M_RANGE(range,tag,begin,end,type) \
                            { RANGE(range,B2M(tag,begin),B2M(tag,end),type); }
#define PCI_I_RANGE(range,tag,begin,end,type) \
                            { RANGE(range,B2I(tag,begin),B2I(tag,end),type); }
#define PCI_X_RANGE(range,tag,begin,end,type) \
{ if (type & ResMem)  PCI_M_RANGE(range,tag,begin,end,type); \
                else PCI_I_RANGE(range,tag,begin,end,type); } 
#define P_M_RANGE(range,tag,begin,size,type) \
                    PCI_M_RANGE(range,tag,begin,(begin + SIZE(size)),type)
#define P_I_RANGE(range,tag,begin,size,type) \
                    PCI_I_RANGE(range,tag,begin,(begin + SIZE(size)),type)
#define P_X_RANGE(range,tag,begin,size,type) \
{ if (type & ResMem)  P_M_RANGE(range,tag,begin,size,type); \
                else P_I_RANGE(range,tag,begin,size,type); }
#define PV_M_RANGE(range,pvp,i,type) \
                  P_M_RANGE(range,TAG(pvp),pvp->memBase[i],pvp->size[i],type)
#define PV_B_RANGE(range,pvp,type) \
                  P_M_RANGE(range,TAG(pvp),pvp->biosBase,pvp->biosSize,type)
#define PV_I_RANGE(range,pvp,i,type) \
                  P_I_RANGE(range,TAG(pvp),pvp->ioBase[i],pvp->size[i],type)

SymTabPtr xf86PCIVendorNameInfo;
pciVendorCardInfo *xf86PCICardInfo;
pciVendorDeviceInfo * xf86PCIVendorInfo;

static void
getPciClassFlags(pciConfigPtr *pcrpp);
static void
pciConvertListToHost(int bus, int dev, int func, resPtr list);

static void
FindPCIVideoInfo(void)
{
    pciConfigPtr pcrp, *pcrpp;
    int i = 0, j, k;
    int num = 0;
    pciVideoPtr info;
    Bool mem64 = FALSE;

    pcrpp = xf86PciInfo = xf86scanpci(0);
    getPciClassFlags(pcrpp);
    
    if (pcrpp == NULL) {
	xf86PciVideoInfo = NULL;
	return;
    }
    xf86PciBus = xf86GetPciBridgeInfo(xf86PciInfo);
    
    while ((pcrp = pcrpp[i])) {
	int baseclass;
	int subclass;

	if (pcrp->listed_class & 0xffff) {
	    baseclass = (pcrp->listed_class >> 8) & 0xff;
	    subclass = pcrp->listed_class & 0xff;
	} else {
	    baseclass = pcrp->pci_base_class;
	    subclass = pcrp->pci_sub_class;
	}
	
	if (PCIINFOCLASSES(baseclass, subclass)) {
	    num++;
	    xf86PciVideoInfo = xnfrealloc(xf86PciVideoInfo,
					  sizeof(pciVideoPtr) * (num + 1));
	    xf86PciVideoInfo[num] = NULL;
	    info = xf86PciVideoInfo[num - 1] = xnfalloc(sizeof(pciVideoRec));
	    info->validSize = FALSE;
	    info->vendor = pcrp->pci_vendor;
	    info->chipType = pcrp->pci_device;
	    info->chipRev = pcrp->pci_rev_id;
	    info->subsysVendor = pcrp->pci_subsys_vendor;
	    info->subsysCard = pcrp->pci_subsys_card;
	    info->bus = pcrp->busnum;
	    info->device = pcrp->devnum;
	    info->func = pcrp->funcnum;
	    info->class = baseclass;
	    info->subclass = pcrp->pci_sub_class;
	    info->interface = pcrp->pci_prog_if;
	    info->biosBase = PCIGETROM(pcrp->pci_baserom);
	    info->biosSize = pciGetBaseSize(pcrp->tag, 6, TRUE, NULL);
	    info->thisCard = pcrp;
	    info->validate = FALSE;
	    if ((PCISHAREDIOCLASSES(baseclass, subclass))
		&& (pcrp->pci_command & PCI_CMD_IO_ENABLE) &&
		(pcrp->pci_prog_if == 0)) {
		/* assumption: primary bus is always VGA */
	            primaryBus.type = BUS_PCI;
	            primaryBus.id.pci.bus = pcrp->busnum;
		    primaryBus.id.pci.device = pcrp->devnum;
		    primaryBus.id.pci.func = pcrp->funcnum;
	    }
	    
	    for (j = 0; j < 6; j++) {
		info->memBase[j] = 0;
		info->ioBase[j] = 0;
		if (PCINONSYSTEMCLASSES(info->class, info->subclass)) {
		    info->size[j]  = pciGetBaseSize(pcrp->tag, j, TRUE, 
						    &info->validSize);
		} else {
		    info->size[j] = pcrp->basesize[j];
		    info->validSize = pcrp->minBasesize;
		}
		/* pciGetBaseSize(pcrp->tag, j, FALSE, NULL) */
		info->type[j] = 0;
	    }

	    if (!(pcrp->pci_base0) && info->size[0]
		&& PCINONSYSTEMCLASSES(info->class, info->subclass))
		pcrp->pci_base0 = pciCheckForBrokenBase(pcrp->tag,0);
	    if (!(pcrp->pci_base1) && info->size[1]
		&& PCINONSYSTEMCLASSES(info->class, info->subclass))
		pcrp->pci_base1 = pciCheckForBrokenBase(pcrp->tag,1);
	    if (!(pcrp->pci_base2) && info->size[2]
		&& PCINONSYSTEMCLASSES(info->class, info->subclass))
		pcrp->pci_base2 = pciCheckForBrokenBase(pcrp->tag,2);
	    if (!(pcrp->pci_base3) && info->size[3]
		&& PCINONSYSTEMCLASSES(info->class, info->subclass))
		pcrp->pci_base3 = pciCheckForBrokenBase(pcrp->tag,3);
	    if (!(pcrp->pci_base4) && info->size[4]
		&& PCINONSYSTEMCLASSES(info->class, info->subclass))
		pcrp->pci_base4 = pciCheckForBrokenBase(pcrp->tag,4);
	    if (!(pcrp->pci_base5) && info->size[5]
		&& PCINONSYSTEMCLASSES(info->class, info->subclass))
		pcrp->pci_base5 = pciCheckForBrokenBase(pcrp->tag,5);
	    
	    /*
	     * 64-bit base addresses are checked for and avoided.
	     * XXX Should deal with them on platforms that support them.
	     */
	    if (pcrp->pci_base0) {
		if (pcrp->pci_base0 & PCI_MAP_IO) {
		    info->ioBase[0] = (memType)PCIGETIO(pcrp->pci_base0);
		    info->type[0] = pcrp->pci_base0 & PCI_MAP_IO_ATTR_MASK;
		} else {
		    info->type[0] = pcrp->pci_base0 & PCI_MAP_MEMORY_ATTR_MASK;
		    info->memBase[0] = (memType)PCIGETMEMORY(pcrp->pci_base0);
		    if (PCI_MAP_IS64BITMEM(pcrp->pci_base0)) {
			mem64 = TRUE;
#if defined LONG64 || defined WORD64
			  info->memBase[0] |= 
			    (memType)PCIGETMEMORY64HIGH(pcrp->pci_base1) << 32;
#else
			if (pcrp->pci_base1)
			  info->memBase[0] = 0;
#endif
		    } 
		}
	    }

	    if (pcrp->pci_base1 && !mem64) {
		if (pcrp->pci_base1 & PCI_MAP_IO) {
		    info->ioBase[1] = (memType)PCIGETIO(pcrp->pci_base1);
		    info->type[1] = pcrp->pci_base1 & PCI_MAP_IO_ATTR_MASK;
		} else {
		    info->type[1] = pcrp->pci_base1 & PCI_MAP_MEMORY_ATTR_MASK;
		    info->memBase[1] = (memType)PCIGETMEMORY(pcrp->pci_base1);
		    if (PCI_MAP_IS64BITMEM(pcrp->pci_base1)) {
			mem64 = TRUE;
#if defined LONG64 || defined WORD64
			  info->memBase[1] |= 
			    (memType)PCIGETMEMORY64HIGH(pcrp->pci_base2) << 32;
#else
			if (pcrp->pci_base2)
			  info->memBase[1] = 0;
#endif
		    }
		}
	    } else
		mem64 = FALSE;

	    if (pcrp->pci_base2 && !mem64) {
		if (pcrp->pci_base2 & PCI_MAP_IO) {
		    info->ioBase[2] = (memType)PCIGETIO(pcrp->pci_base2);
		    info->type[2] = pcrp->pci_base2 & PCI_MAP_IO_ATTR_MASK;
		} else {
		    info->type[2] = pcrp->pci_base2 & PCI_MAP_MEMORY_ATTR_MASK;
		    info->memBase[2] = (memType)PCIGETMEMORY(pcrp->pci_base2);
		    if (PCI_MAP_IS64BITMEM(pcrp->pci_base2)) {
			mem64 = TRUE;
#if defined LONG64 || defined WORD64
			info->memBase[2] |= 
			    (memType)PCIGETMEMORY64HIGH(pcrp->pci_base3) << 32;
#else
			if (pcrp->pci_base3)
			  info->memBase[2] = 0;
#endif
		    }
		}
	    } else
		mem64 = FALSE;

	    if (pcrp->pci_base3 && !mem64) {
		if (pcrp->pci_base3 & PCI_MAP_IO) {
		    info->ioBase[3] = (memType)PCIGETIO(pcrp->pci_base3);
		    info->type[3] = pcrp->pci_base3 & PCI_MAP_IO_ATTR_MASK;
		} else {
		    info->type[3] = pcrp->pci_base3 & PCI_MAP_MEMORY_ATTR_MASK;
		    info->memBase[3] = (memType)PCIGETMEMORY(pcrp->pci_base3);
		    if (PCI_MAP_IS64BITMEM(pcrp->pci_base3)) {
			mem64 = TRUE;
#if defined LONG64 || defined WORD64
			  info->memBase[3] |= 
			    (memType)PCIGETMEMORY64HIGH(pcrp->pci_base4) << 32;
#else
			if (pcrp->pci_base4)
			  info->memBase[3] = 0;
#endif
		    }
		}
	    } else
		mem64 = FALSE;

	    if (pcrp->pci_base4 && !mem64) {
		if (pcrp->pci_base4 & PCI_MAP_IO) {
		    info->ioBase[4] = (memType)PCIGETIO(pcrp->pci_base4);
		    info->type[4] = pcrp->pci_base4 & PCI_MAP_IO_ATTR_MASK;
		} else {
		    info->type[4] = pcrp->pci_base4 & PCI_MAP_MEMORY_ATTR_MASK;
		    info->memBase[4] = (memType)PCIGETMEMORY(pcrp->pci_base4);
		    if (PCI_MAP_IS64BITMEM(pcrp->pci_base4)) {
			mem64 = TRUE;
#if defined LONG64 || defined WORD64
			  info->memBase[4] |= 
			    (memType)PCIGETMEMORY64HIGH(pcrp->pci_base5) << 32;
#else
			if (pcrp->pci_base5)
			  info->memBase[4] = 0;
#endif
		    }
		}
	    } else
		mem64 = FALSE;

	    if (pcrp->pci_base5 && !mem64) {
		if (pcrp->pci_base5 & PCI_MAP_IO) {
		    info->ioBase[5] = (memType)PCIGETIO(pcrp->pci_base5);
		    info->type[5] = pcrp->pci_base5 & PCI_MAP_IO_ATTR_MASK;
		} else {
		    info->type[5] = pcrp->pci_base5 & PCI_MAP_MEMORY_ATTR_MASK;
		    info->memBase[5] = (memType)PCIGETMEMORY(pcrp->pci_base5);
		}
	    } else
		mem64 = FALSE;
	    info->listed_class = pcrp->listed_class;
	}
	i++;
    }

    /* Print a summary of the video devices found */
    {
	for (k = 0; k < num; k++) {
	    char *vendorname = NULL, *chipname = NULL;
	    Bool memdone = FALSE, iodone = FALSE;

	    i = 0; 
	    info = xf86PciVideoInfo[k];
	    while (xf86PCIVendorNameInfo[i].token) {
		if (xf86PCIVendorNameInfo[i].token == info->vendor) 
		    vendorname = (char *)xf86PCIVendorNameInfo[i].name;
		i++;
	    }
	    i = 0;
	    while(xf86PCIVendorInfo[i].VendorID) {
		if (xf86PCIVendorInfo[i].VendorID == info->vendor) {
		    j = 0;
		    while (xf86PCIVendorInfo[i].Device[j].DeviceName) {
			if (xf86PCIVendorInfo[i].Device[j].DeviceID ==
			    info->chipType) {
			    chipname =
				xf86PCIVendorInfo[i].Device[j].DeviceName;
			    break;
			}
			j++;
		    }
		    break;
		}
		i++;
	    }
	    if ((!vendorname || !chipname) &&
		!PCIALWAYSPRINTCLASSES(info->class, info->subclass))
		continue;
	    if (xf86IsPrimaryPci(info))
	    	xf86Msg(X_PROBED, "PCI:*(%d:%d:%d) ", info->bus, info->device,
		    info->func);
	    else
	    	xf86Msg(X_PROBED, "PCI: (%d:%d:%d) ", info->bus, info->device,
		    info->func);
	    if (vendorname)
		xf86ErrorF("%s ", vendorname);
	    else
		xf86ErrorF("unknown vendor (0x%04x) ", info->vendor);
	    if (chipname)
		xf86ErrorF("%s ", chipname);
	    else
		xf86ErrorF("unknown chipset (0x%04x) ", info->chipType);
	    xf86ErrorF("rev %d", info->chipRev);
	    for (i = 0; i < 6; i++) {
		if (info->memBase[i]) {
		    if (!memdone) {
			xf86ErrorF(", Mem @ ");
			memdone = TRUE;
		    } else
			xf86ErrorF(", ");
		    xf86ErrorF("0x%08x/%d", info->memBase[i], info->size[i]);
		}
	    }
	    for (i = 0; i < 6; i++) {
		if (info->ioBase[i]) {
		    if (!iodone) {
			xf86ErrorF(", I/O @ ");
			iodone = TRUE;
		    } else
			xf86ErrorF(", ");
		    xf86ErrorF("0x%04x/%d", info->ioBase[i], info->size[i]);
		}
	    }
	    xf86ErrorF("\n");
	}
    }
}

/*
 * fixPciSizeInfo() -- fix pci size info by testing it destructively
 * (if not already done), fix pciVideoInfo and entry in the resource
 * list.
 */
/*
 * Note: once we have OS support to read the sizes GetBaseSize() will
 * have to be wrapped by the OS layer. fixPciSizeInfo() should also
 * be wrapped by the OS layer to do nothing if the size is always
 * returned correctly by GetBaseSize(). It should however set validate
 * in pciVideoRec if validation is required. ValidatePci() also needs
 * to be wrapped by the OS layer. This may do nothing if the OS has
 * already taken care of validation. fixPciResource() may be moved to
 * OS layer with minimal changes. Once the wrapping layer is in place
 * the common level and drivers should not reference these functions
 * directly but thru the OS layer.
 */

static void
fixPciSizeInfo(int entityIndex)
{
    pciVideoPtr pvp;
    resPtr pAcc;
    PCITAG tag;
    int j;
    
    if (! (pvp = xf86GetPciInfoForEntity(entityIndex))) return;
    if (pvp->validSize) return;

    tag = pciTag(pvp->bus,pvp->device,pvp->func);
    
    for (j = 0; j < 6; j++) {
	pAcc = Acc;
	if (pvp->memBase[j]) 
	    while (pAcc) {
		if (((pAcc->res_type & (ResMem | ResBlock))
		     == (ResMem | ResBlock))
		    && (pAcc->block_begin == B2M(TAG(pvp),pvp->memBase[j])) 
		    && (pAcc->block_end == B2M(TAG(pvp),pvp->memBase[j]
		    + SIZE(pvp->size[j])))) break;
		pAcc = pAcc->next;
	    } else if (pvp->ioBase[j])
	    while (pAcc) {
		if (((pAcc->res_type & (ResIo | ResBlock)) ==
		     (ResIo | ResBlock))
		    && (pAcc->block_begin == B2I(TAG(pvp),pvp->ioBase[j]))
		    && (pAcc->block_end == B2I(TAG(pvp),pvp->ioBase[j]
		    + SIZE(pvp->size[j])))) break;
		pAcc = pAcc->next;
	    } else continue;
	pvp->size[j]  = pciGetBaseSize(tag, j, TRUE, &pvp->validSize);
	if (pAcc) {
	    pAcc->block_end = pvp->memBase[j] ?
		B2M(TAG(pvp),pvp->memBase[j] + SIZE(pvp->size[j]))
		: B2I(TAG(pvp),pvp->ioBase[j] + SIZE(pvp->size[j]));
	    pAcc->res_type &= ~ResEstimated;
	    pAcc->res_type |= ResBios;
	}
    }
    if (pvp->biosBase) {
	pAcc = Acc;
	while (pAcc) {
	    if (((pAcc->res_type & (ResMem | ResBlock)) == (ResMem | ResBlock))
		&& (pAcc->block_begin == B2M(TAG(pvp),pvp->biosBase))
		    && (pAcc->block_end == B2M(TAG(pvp),pvp->biosBase
		    + SIZE(pvp->biosSize)))) break;
	    pAcc = pAcc->next;
	}
	pvp->biosSize = pciGetBaseSize(tag, 6, TRUE, &pvp->validSize);
	if (pAcc) {
	    pAcc->block_end = B2M(TAG(pvp),pvp->biosBase+SIZE(pvp->biosSize));
	    pAcc->res_type &= ~ResEstimated;
	    pAcc->res_type |= ResBios;
	}
    }
}

/*
 * IO enable/disable related routines for PCI
 */
#define SETBITS PCI_CMD_IO_ENABLE
static void
pciIoAccessEnable(void* arg)
{
#ifdef DEBUG
    ErrorF("pciIoAccessEnable: 0x%05lx\n", *(PCITAG *)arg);
#endif
    ((pciArg*)arg)->ctrl |= SETBITS;
    ((pciArg*)arg)->func(((pciArg*)arg)->tag, PCI_CMD_STAT_REG,
			 ((pciArg*)arg)->ctrl);
}

static void
pciIoAccessDisable(void* arg)
{
#ifdef DEBUG
    ErrorF("pciIoAccessDisable: 0x%05lx\n", *(PCITAG *)arg);
#endif
    ((pciArg*)arg)->ctrl &= ~SETBITS;
    ((pciArg*)arg)->func(((pciArg*)arg)->tag, PCI_CMD_STAT_REG,
			 ((pciArg*)arg)->ctrl);
}

#undef SETBITS
#define SETBITS (PCI_CMD_IO_ENABLE | PCI_CMD_MEM_ENABLE)
static void
pciIo_MemAccessEnable(void* arg)
{
#ifdef DEBUG
    ErrorF("pciIo_MemAccessEnable: 0x%05lx\n", *(PCITAG *)arg);
#endif
    ((pciArg*)arg)->ctrl |= SETBITS;
    ((pciArg*)arg)->func(((pciArg*)arg)->tag, PCI_CMD_STAT_REG,
			 ((pciArg*)arg)->ctrl);
}

static void
pciIo_MemAccessDisable(void* arg)
{
#ifdef DEBUG
    ErrorF("pciIo_MemAccessDisable: 0x%05lx\n", *(PCITAG *)arg);
#endif
    ((pciArg*)arg)->ctrl &= ~SETBITS;
    ((pciArg*)arg)->func(((pciArg*)arg)->tag, PCI_CMD_STAT_REG,
			 ((pciArg*)arg)->ctrl);
}

#undef SETBITS
#define SETBITS (PCI_CMD_MEM_ENABLE)
static void
pciMemAccessEnable(void* arg)
{
#ifdef DEBUG
    ErrorF("pciMemAccessEnable: 0x%05lx\n", *(PCITAG *)arg);
#endif
    ((pciArg*)arg)->ctrl |= SETBITS;
    ((pciArg*)arg)->func(((pciArg*)arg)->tag, PCI_CMD_STAT_REG,
			 ((pciArg*)arg)->ctrl);
}

static void
pciMemAccessDisable(void* arg)
{
#ifdef DEBUG
    ErrorF("pciMemAccessDisable: 0x%05lx\n", *(PCITAG *)arg);
#endif
    ((pciArg*)arg)->ctrl &= ~SETBITS;
    ((pciArg*)arg)->func(((pciArg*)arg)->tag, PCI_CMD_STAT_REG,
			 ((pciArg*)arg)->ctrl);
}
#undef SETBITS

#define PCI_PCI_BRDG_CTRL_BASE (PCI_PCI_BRIDGE_CONTROL_REG & 0xFC)
#define SHIFT_BITS ((PCI_PCI_BRIDGE_CONTROL_REG & 0x3) << 3)
#define SETBITS (CARD32)((PCI_PCI_BRIDGE_VGA_EN) << SHIFT_BITS)
static void
pciBusAccessEnable(BusAccPtr ptr)
{
    ptr->busdep.pci.func(ptr->busdep.pci.acc,PCI_PCI_BRDG_CTRL_BASE,
			 SETBITS,SETBITS);
}

static void
pciBusAccessDisable(BusAccPtr ptr)
{
    ptr->busdep.pci.func(ptr->busdep.pci.acc,PCI_PCI_BRDG_CTRL_BASE,SETBITS,0);
}
#undef SETBITS
#undef SHIFT_BITS

static void
pciSetBusAccess(BusAccPtr ptr)
{
#ifdef DEBUG
    ErrorF("pciSetBusAccess: route VGA to bus %d\n", ptr->busdep.pci.bus);
#endif

    if (!ptr->primary && !ptr->current)
	return;
    
    if (ptr->current && ptr->current->disable_f)
	ptr->current->disable_f(ptr->current);
    ptr->current = NULL;
    
    /* walk down */
    while (ptr->primary) { /* no enable for top bus */
	if (ptr->primary->current != ptr) {
	    if (ptr->primary->current && ptr->primary->current->disable_f)
		ptr->primary->current->disable_f(ptr->primary->current);
	    if (ptr->enable_f) ptr->enable_f(ptr);
	    ptr->primary->current = ptr;
	}
	ptr = ptr->primary;
    }
}

/* move to OS layer */
static void
savePciState(PCITAG tag, pciSavePtr ptr)
{
    int i;
     
    ptr->command = pciReadLong(tag,PCI_CMD_STAT_REG);
    for (i=0; i < 6; i++) 
        ptr->base[i] = pciReadLong(tag,PCI_CMD_BASE_REG + i*4 );
    ptr->biosBase = pciReadLong(tag,PCI_CMD_BIOS_REG);
}

/* move to OS layer */
static void
restorePciState(PCITAG tag, pciSavePtr ptr)
{
    int i;
    
    /* disable card before setting anything */
    pciSetBitsLong(tag, PCI_CMD_STAT_REG, PCI_CMD_MEM_ENABLE
		   | PCI_CMD_IO_ENABLE , 0);
    pciWriteLong(tag,PCI_CMD_BIOS_REG,ptr->biosBase);
    for (i=0; i<6; i++)
        pciWriteLong(tag,PCI_CMD_BASE_REG + i*4, ptr->base[i]);        
    pciWriteLong(tag,PCI_CMD_STAT_REG,ptr->command);
}

/* move to OS layer */
static void
savePciBusState(BusAccPtr ptr)
{
    ptr->busdep.pci.save.io =
		pciReadWord(ptr->busdep.pci.acc,PCI_PCI_BRIDGE_IO_REG);
    ptr->busdep.pci.save.mem =
		pciReadWord(ptr->busdep.pci.acc,PCI_PCI_BRIDGE_MEM_REG);
    ptr->busdep.pci.save.pmem =
		pciReadWord(ptr->busdep.pci.acc,PCI_PCI_BRIDGE_PMEM_REG);
    ptr->busdep.pci.save.control =
		pciReadByte(ptr->busdep.pci.acc,PCI_PCI_BRIDGE_CONTROL_REG);
}

/* move to OS layer */
static void
restorePciBusState(BusAccPtr ptr)
{
    pciWriteWord(ptr->busdep.pci.acc,PCI_PCI_BRIDGE_IO_REG,
		 ptr->busdep.pci.save.io);
    pciWriteWord(ptr->busdep.pci.acc,PCI_PCI_BRIDGE_MEM_REG,
		 ptr->busdep.pci.save.mem);
    pciWriteWord(ptr->busdep.pci.acc,PCI_PCI_BRIDGE_PMEM_REG,
		 ptr->busdep.pci.save.pmem);
    pciWriteByte(ptr->busdep.pci.acc,PCI_PCI_BRIDGE_CONTROL_REG,
		 ptr->busdep.pci.save.control);
}


static void
disablePciBios(PCITAG tag)
{
    pciSetBitsLong(tag, PCI_CMD_BIOS_REG, PCI_CMD_BIOS_ENABLE, 0);
}

/* ????? */
static void
correctPciSize(memType base, int oldsize, int newsize, long type)
{
    pciConfigPtr pcrp, *pcrpp;
    pciVideoPtr pvp, *pvpp;
    CARD32 *basep;
    int i;
    int old_bits =0, new_bits = 0;

    while (oldsize & 1) {
	old_bits ++;
	oldsize >>= 1;
    }
    while (newsize & 1) {
	new_bits ++;
	newsize >>= 1;
    }
    
    for (pcrpp = xf86PciInfo, pcrp = *pcrpp; pcrp; pcrp = *++(pcrpp)) {
	
	/* Only process devices with type 0 headers */
	if ((pcrp->pci_header_type & 0x7f) != 0)
	    continue;

	basep = &pcrp->pci_base0;
	for (i = 0; i < 6; i++) {
	    int j = i;
	    if (basep[i] && (pcrp->basesize[i] == old_bits))
		if (((type & ResIo) && PCI_MAP_IS_IO(basep[i])
		     && (B2I(pcrp->tag,PCIGETIO(basep[i])) == base)) 
		    || ((type & ResMem) && PCI_MAP_IS_MEM(basep[i])
			&& (((!PCI_MAP_IS64BITMEM(basep[i])) 
			     && (B2M(pcrp->tag,PCIGETMEMORY(basep[i])) == base))
#if defined LONG64 || defined WORD64
			    || (B2M(pcrp->tag,PCIGETMEMORY64(basep[i])) == base)
#endif 
			    ))) {
		    pcrp->basesize[j] = new_bits;
		    return;
		}
	}
    }

    if (xf86PciVideoInfo)
	for (pvpp = xf86PciVideoInfo, pvp = *pvpp; pvp; pvp = *(++pvpp)) {

	    for (i = 0; i < 6; i++) {
		if (pvp->size[i] == old_bits) {
		    if (((type & ResIo) && pvp->ioBase[i]
			 && (B2I(TAG(pvp),pvp->ioBase[i]) == base)) || 
			((type & ResMem) && pvp->memBase[i] 
			  && (B2M(TAG(pvp),pvp->memBase[i]) == base))) {
			pvp->size[i] = new_bits;
			return;
		    }
		}
	    }
	}
}

/* ????? */
static void
removeOverlapsWithBridges(int busIndex, resPtr target)
{
    PciBusPtr pbp;
    resPtr tmp,bridgeRes = NULL;
    resRange range = target->val;

    if (!ResIsEstimated(&target->val))
	return;
    
    for (pbp=xf86PciBus; pbp; pbp = pbp->next) {
	if (pbp->primary == busIndex) {
	    tmp = xf86DupResList(pbp->preferred_io);
	    bridgeRes = xf86JoinResLists(tmp,bridgeRes);
	    tmp = xf86DupResList(pbp->preferred_mem);
	    bridgeRes = xf86JoinResLists(tmp,bridgeRes);
	    tmp = xf86DupResList(pbp->preferred_pmem);
	    bridgeRes = xf86JoinResLists(tmp,bridgeRes);
	}
    }
    
    RemoveOverlaps(target,bridgeRes,TRUE);
    if (range.rEnd > target->block_end) {
	correctPciSize(range.rBegin,range.rEnd - range.rBegin,
		       target->block_end - target->block_begin,
		       target->res_type);
    }
    xf86FreeResList(bridgeRes);
}
    
/* ????? */
static void
xf86GetPciRes(resPtr *activeRes, resPtr *inactiveRes)
{
    pciConfigPtr pcrp, *pcrpp;
    pciVideoPtr pvp, *pvpp;
    CARD32 *basep;
    int i;
    resPtr pRes, tmp;
    resRange range;
    long resMisc;

    if (activeRes)
	*activeRes = NULL;
    if (inactiveRes)
	*inactiveRes = NULL;

    if (!activeRes || !inactiveRes || !xf86PciInfo)
	return;

    if (xf86PciVideoInfo)
	for (pvpp = xf86PciVideoInfo, pvp = *pvpp; pvp; pvp = *(++pvpp)) {
	    resPtr *res;

	    if (PCINONSYSTEMCLASSES(pvp->class, pvp->subclass)) 
		resMisc = ResBios;
	    else 
		resMisc = 0;
	    
	    if (((pciConfigPtr)pvp->thisCard)->pci_command
		& (PCI_CMD_IO_ENABLE | PCI_CMD_MEM_ENABLE))
		res = activeRes;
	    else
		res = inactiveRes;
	    
	    if (!pvp->validSize)
		resMisc |= ResEstimated;
	    
	    for (i = 0; i < 6; i++) {
		if (pvp->ioBase[i]) {
		    PV_I_RANGE(range,pvp,i,ResExcIoBlock | resMisc);
		    tmp = xf86AddResToList(NULL, &range, -1);
		    removeOverlapsWithBridges(pvp->bus,tmp);
		    *res = xf86JoinResLists(tmp,*res);
		} else if (pvp->memBase[i]) {
		    PV_M_RANGE(range, pvp,i, ResExcMemBlock | resMisc);
		    tmp = xf86AddResToList(NULL, &range, -1);
		    removeOverlapsWithBridges(pvp->bus,tmp);
		    *res = xf86JoinResLists(tmp,*res);
		}
	    }
	    /* FIXME!!!: Don't use BIOS resources for overlap
	     * checking but reserve them!
	     */
	    if (pvp->biosBase) {
		PV_B_RANGE(range, pvp, ResExcMemBlock | resMisc);
		tmp = xf86AddResToList(NULL, &range, -1);
		removeOverlapsWithBridges(pvp->bus,tmp);
		*res = xf86JoinResLists(tmp,*res);
	    }
	}

    for (pcrpp = xf86PciInfo, pcrp = *pcrpp; pcrp; pcrp = *++(pcrpp)) {
	resPtr *res;
	
	if (PCIINFOCLASSES(pcrp->pci_base_class, pcrp->pci_sub_class))
	    continue;
	
	/* Only process devices with type 0 headers */
	if ((pcrp->pci_header_type & 0x7f) != 0)
	    continue;

	if (!pcrp->minBasesize)
	    resMisc = ResEstimated;
	else
	    resMisc = 0;
	
	if ((pcrp->pci_command & (PCI_CMD_IO_ENABLE | PCI_CMD_MEM_ENABLE)))
	    res = activeRes;
	else
	    res = inactiveRes;
	
	basep = &pcrp->pci_base0;
	for (i = 0; i < 6; i++) {
	    if (basep[i]) {
	        if (PCI_MAP_IS_IO(basep[i]))
		    P_I_RANGE(range,pcrp->tag,PCIGETIO(basep[i]),
			      pcrp->basesize[i], ResExcIoBlock | resMisc)
		else if (!PCI_MAP_IS64BITMEM(basep[i])) 
		    P_M_RANGE(range,pcrp->tag,PCIGETMEMORY(basep[i]),
			   pcrp->basesize[i], ResExcMemBlock | resMisc)
		else {
		    i++;
#if defined LONG64 || defined WORD64
		    P_M_RANGE(range,pcrp->tag,PCIGETMEMORY64(basep[i-1]),
			  pcrp->basesize[i-1], ResExcMemBlock | resMisc)
#else
		      continue;
#endif
		}
		if (range.rBegin) { /* catch cases where PCI base is unset */
		    tmp = xf86AddResToList(NULL, &range, -1);
		    removeOverlapsWithBridges(pcrp->busnum,tmp);
		    *res = xf86JoinResLists(tmp,*res);
		}
	    }
	}
	if (pcrp->pci_baserom) {
	    P_M_RANGE(range,pcrp->tag,PCIGETROM(pcrp->pci_baserom),
		  pcrp->basesize[6], ResExcMemBlock | resMisc);
	    if (range.rBegin) {
		tmp = xf86AddResToList(NULL, &range, -1);
		removeOverlapsWithBridges(pcrp->busnum,tmp);
		*res = xf86JoinResLists(tmp,*res);
	    }
	}
    }

    if (*activeRes) {
	xf86MsgVerb(X_INFO, 3, "Active PCI resource ranges:\n");
	xf86PrintResList(3, *activeRes);
    }
    if (*inactiveRes) {
	xf86MsgVerb(X_INFO, 3, "Inactive PCI resource ranges:\n");
	xf86PrintResList(3, *inactiveRes);
    }

    /*
     * Adjust ranges based on the assumption that there are no real
     * overlaps in the PCI base allocations.  This assumption should be
     * reasonable in most cases.  It may be possible to refine the
     * approximated PCI base sizes by considering bus mapping information
     * from PCI-PCI bridges.
     */

    if (*activeRes) {
	/* Check for overlaps */
	for (pRes = *activeRes; pRes; pRes = pRes->next) {
	    if (ResIsEstimated(&pRes->val)) {
		range = pRes->val;

		RemoveOverlaps(pRes, *activeRes, TRUE);
		if (xf86Info.estimateSizesAggressively > 0)
		    RemoveOverlaps(pRes, *inactiveRes, TRUE);
		
		if (range.rEnd > pRes->block_end) {
		    correctPciSize(range.rBegin,range.rEnd - range.rBegin,
				   pRes->block_end - pRes->block_begin,
				   pRes->res_type);
		    xf86MsgVerb(X_INFO, 3,
				"PCI %s resource overlap reduced 0x%08x"
				" from 0x%08x to 0x%08x\n",
				(pRes->res_type & ResMem) ? "Memory" : "I/O",
				range.rBegin, range.rEnd, pRes->block_end);
		}
	    }
	}
	xf86MsgVerb(X_INFO, 3,
	    "Active PCI resource ranges after removing overlaps:\n");
	xf86PrintResList(3, *activeRes);
    }

    if (*inactiveRes && (xf86Info.estimateSizesAggressively > 1)) {
	/* Check for overlaps */
	for (pRes = *inactiveRes; pRes; pRes = pRes->next) {
	    if (ResIsEstimated(&pRes->val)) {
		range = pRes->val;

		RemoveOverlaps(pRes, *activeRes, TRUE);
		RemoveOverlaps(pRes, *inactiveRes, TRUE);
		
		if (range.rEnd > pRes->block_end) {
		    correctPciSize(range.rBegin,range.rEnd - range.rBegin,
				   pRes->block_end - pRes->block_begin,
				   pRes->res_type);
		    xf86MsgVerb(X_INFO, 3,
				"PCI %s resource overlap reduced 0x%08x"
				" from 0x%08x to 0x%08x\n",
				(pRes->res_type & ResMem) ? "Memory" : "I/O",
				range.rBegin, range.rEnd, pRes->block_end);
		}
		
	    }
	}
	xf86MsgVerb(X_INFO, 3,
	    "Ative PCI resource ranges after removing overlaps:\n");
	xf86PrintResList(3, *inactiveRes);
    }
}

resPtr
ResourceBrokerInitPci(resPtr *osRes)
{
    resPtr activeRes, inactiveRes;
    resPtr tmp;
    
    /* Get bus-specific system resources (PCI) */
    xf86GetPciRes(&activeRes, &inactiveRes);

    /*
     * Adjust OS-reported resource ranges based on the assumption that there
     * are no overlaps with the PCI base allocations.  This should be a good
     * assumption because writes to PCI address space won't be routed directly
     * to host memory.
     */

    for (tmp = *osRes; tmp; tmp = tmp->next) 
	RemoveOverlaps(tmp, activeRes, FALSE);

    xf86MsgVerb(X_INFO, 3, "OS-reported resource ranges after removing"
		" overlaps with PCI:\n");
    xf86PrintResList(3, *osRes);

    pciAvoidRes = xf86AddRangesToList(pciAvoidRes,PciAvoid,-1);
    for (tmp = pciAvoidRes; tmp; tmp = tmp->next) 
	RemoveOverlaps(tmp, activeRes, FALSE);
    tmp = xf86DupResList(*osRes);
    pciAvoidRes = xf86JoinResLists(pciAvoidRes,tmp);
    
    return (xf86JoinResLists(activeRes,inactiveRes));
}


/*
 * PCI Resource modification
 */
static Bool
fixPciResource(int prt, memType alignment, pciVideoPtr pvp, long type)
{
    int  res_n;
    memType *p_base;
    int *p_size;
    unsigned char p_type;
    resPtr AccTmp = NULL;
    resPtr orgAcc = NULL;
    resPtr *pAcc = &AccTmp;
    resPtr avoid = NULL;
    resRange range;
    resPtr resSize = NULL;
    resPtr w_tmp, w = NULL, w_2nd = NULL;
    PCITAG tag;
    PciBusPtr pbp = xf86PciBus, pbp1 = xf86PciBus;
    pciConfigPtr pcp;
    resPtr tmp;
    
    if (!pvp) return FALSE;
    tag = pciTag(pvp->bus,pvp->device,pvp->func);
    pcp = pvp->thisCard;

    type &= ResAccMask;
    if (!type) type = ResShared;
    if (prt < 6) {
	if (pvp->memBase[prt]) {
	    type |= ResMem;
	    res_n = prt;
	    p_base = &(pvp->memBase[res_n]);
	    p_size = &(pvp->size[res_n]);
	    p_type = pvp->type[res_n];
	    if (!PCI_MAP_IS64BITMEM(pvp->type[res_n])) {
	      PCI_M_RANGE(range,tag,0,0xffffffff,ResExcMemBlock);
	      resSize = xf86AddResToList(resSize,&range,-1);
	    }
	} else if (pvp->ioBase[prt]){
	    type |= ResIo;
	    res_n = prt;
	    p_base = &(pvp->ioBase[res_n]);
	    p_size = &(pvp->size[res_n]);
	    p_type = pvp->type[res_n];
	} else return FALSE;
    } else if (prt == 6) {
	type |= ResMem;
	res_n = 0xff;	/* special flag for bios rom */
	p_base = &(pvp->biosBase);
	p_size = &(pvp->biosSize);
	/* XXX This should also include the PCI_MAP_MEMORY_TYPE_MASK part */
	p_type = 0;
	PCI_M_RANGE(range,tag,0,0xffffffff,ResExcMemBlock);
	resSize = xf86AddResToList(resSize,&range,-1);
    } else return FALSE;

    if (! *p_base) return FALSE;
    
    type |= ResBlock;
    
    /* setup avoid: PciAvoid is bus range: convert later */
    avoid = xf86DupResList(pciAvoidRes);

    while (pbp) {
	if (pbp->secondary == pvp->bus) {
	    if (type & ResMem) {
		if (((p_type & PCI_MAP_MEMORY_CACHABLE)
#if 0 /*EE*/
		     || (res_n == 0xff)/* bios should also be prefetchable */
#endif
		     )) {
		    if (pbp->preferred_pmem)
			w = xf86FindIntersectOfLists(pbp->preferred_pmem,
						     ResRange);
		    else if (pbp->pmem)
			w = xf86FindIntersectOfLists(pbp->pmem,ResRange);
		    
		    if (pbp->preferred_mem) 
			w_2nd = xf86FindIntersectOfLists(pbp->preferred_mem,
							 ResRange);
		    else if (pbp->mem) 
			w_2nd = xf86FindIntersectOfLists(pbp->mem,
							 ResRange);
		} else {
		    if (pbp->preferred_mem)
			w = xf86FindIntersectOfLists(pbp->preferred_mem,
						     ResRange);
		    else if (pbp->mem)
			w = xf86FindIntersectOfLists(pbp->mem,ResRange);
		}
	    } else {
		if (pbp->preferred_io) 
		    w = xf86FindIntersectOfLists(pbp->preferred_io,ResRange);
		if (pbp->io) 
		    w = xf86FindIntersectOfLists(pbp->io,ResRange);
	    }
	    
	    while (pbp1) {
		if (pbp1->primary == pvp->bus) {
		    if (type & ResMem) {
			tmp = xf86DupResList(pbp1->preferred_pmem);
			avoid = xf86JoinResLists(avoid,tmp);
			tmp = xf86DupResList(pbp1->preferred_mem);
			avoid = xf86JoinResLists(avoid,tmp);
		    } else {
			tmp = xf86DupResList(pbp1->preferred_io);
			avoid = xf86JoinResLists(avoid,tmp);
		    }
		}	
		pbp1 = pbp1->next;
	    }
	    break;
	}
	pbp = pbp->next;
    }
    
    /* convert bus based entries in avoid list to host base */
    pciConvertListToHost(pvp->bus,pvp->device,pvp->func, avoid);
    
    if (!w)
	w = xf86DupResList(ResRange);
    xf86MsgVerb(X_INFO, 3, "window:\n");
    xf86PrintResList(3, w);
    xf86MsgVerb(X_INFO, 3, "resSize:\n");
    xf86PrintResList(3, resSize);
    
    if (resSize) {
	w_tmp = w;
	w = xf86FindIntersectOfLists(w,resSize);
	xf86FreeResList(w_tmp);
	if (w_2nd) {
	    w_tmp = w_2nd;
	    w_2nd = xf86FindIntersectOfLists(w_2nd,resSize);
	    xf86FreeResList(w_tmp);
	}
	xf86FreeResList(resSize);
    }
    xf86MsgVerb(X_INFO, 3, "window fixed:\n");
    xf86PrintResList(3, w);

    if (!alignment)
	alignment = (1 << (*p_size)) - 1;
    
    /* Access list holds bios resources -- remove this one */
#ifdef NOTYET
    AccTmp = xf86DupResList(Acc);
    while ((*pAcc)) {
	if ((((*pAcc)->res_type & (type & ~ResAccMask))
	     == (type & ~ResAccMask))
	    && ((*pAcc)->block_begin == (B2H(tag,(*p_base),type)))
	    && ((*pAcc)->block_end == (B2H(tag,
					   (*p_base)+SIZE(*p_size),type)))) {
	    resPtr acc_tmp = (*pAcc)->next;
	    xfree((*pAcc));
	    (*pAcc) = acc_tmp;
	    break;
	} else
	    pAcc = &((*pAcc)->next);
    }
    /* check if we really need to fix anything */
    P_X_RANGE(range,tag,(*p_base),(*p_base) + SIZE((*p_size)),type);
    if (!ChkConflict(&range,avoid,SETUP)
	&& !ChkConflict(&range,AccTmp,SETUP)
	&& ((B2H(tag,(*p_base),type) & PCI_SIZE(type,tag,alignment)
	     == range->block_begin)
	&& ((xf86IsSubsetOf(range,w)
	    || (w_2nd && xf86IsSubsetOf(range,w_2n))))) {
#ifdef DEBUG
	    ErrorF("nothing to fix\n");
#endif
	xf86FreeResList(AccTmp);
	xf86FreeResList(w);
	xf86FreeResList(w_2nd);
	xf86FreeResList(avoid);
	return TRUE;
    } else {
#ifdef DEBUG
	    ErrorF("removing old resource\n");
#endif
	orgAcc = Acc;
	Acc = AccTmp;
    }
#else
    orgAcc = xf86DupResList(Acc);
    pAcc = &Acc;
    while ((*pAcc)) {
	if ((((*pAcc)->res_type & (type & ~ResAccMask))
	     == (type & ~ResAccMask))
	    && ((*pAcc)->block_begin == B2H(tag,(*p_base),type))
	    && ((*pAcc)->block_end == B2H(tag,(*p_base) + SIZE(*p_size),
					  type))) {
#ifdef DEBUG
	    ErrorF("removing old resource\n");
#endif
	    (*pAcc) = (*pAcc)->next;
	    break;
	} else
	    pAcc = &((*pAcc)->next);
    }
#endif
    
#ifdef DEBUG
    ErrorF("base: 0x%lx alignment: 0x%lx host alignment: 0x%lx size[bit]: 0x%x\n",
	   (*p_base),alignment,PCI_SIZE(type,tag,alignment),(*p_size));
    xf86MsgVerb(X_INFO, 3, "window:\n");
    xf86PrintResList(3, w);
    if (w_2nd)
	xf86MsgVerb(X_INFO, 3, "2nd window:\n");
    xf86PrintResList(3, w_2nd);
    xf86ErrorFVerb(3,"avoid:\n");
    xf86PrintResList(3,avoid);
#endif
    w_tmp = w;
    while (w) {
	if (type & w->res_type & ResPhysMask) {
#ifdef DEBUG
	    ErrorF("block_begin: 0x%lx block_end: 0x%lx\n",w->block_begin,
		   w->block_end);
#endif
	    range = xf86GetBlock(type,PCI_SIZE(type,tag,alignment + 1),
				 w->block_begin, w->block_end,
				 PCI_SIZE(type,tag,alignment),avoid);
	    if (range.type != ResEnd)
		break;
	}
	w = w->next;
    }
    xf86FreeResList(w_tmp);
    /* if unsuccessful and memory prefetchable try non-prefetchable */
    if (range.type == ResEnd && w_2nd) {
	w_tmp = w_2nd;
	while (w_2nd) {
	    if (type & w_2nd->res_type & ResPhysMask) {
#ifdef DEBUG
	    ErrorF("block_begin: 0x%lx block_end: 0x%lx\n",w_2nd->block_begin,
		   w_2nd->block_end);
#endif
	    range = xf86GetBlock(type,PCI_SIZE(type,tag,alignment + 1),
				 w_2nd->block_begin, w_2nd->block_end,
				 PCI_SIZE(type,tag,alignment),avoid);
		if (range.type != ResEnd)
		    break;
	    }
	w_2nd = w_2nd->next;
	}
	xf86FreeResList(w_tmp);
    }
    xf86FreeResList(avoid);

    if (range.type == ResEnd) {
	xf86MsgVerb(X_ERROR,3,"Cannot find a replacement memory range\n");
	xf86FreeResList(Acc);
	Acc = orgAcc;
	return FALSE;
    }
    xf86FreeResList(orgAcc);
#ifdef DEBUG
    ErrorF("begin: 0x%lx, end: 0x%lx\n",range.a,range.b);
#endif
    
    (*p_size) = 0;
    while (alignment >> (*p_size))
	(*p_size)++;
    (*p_base) = H2B(tag,range.rBegin,type);
#ifdef DEBUG
    ErrorF("New PCI res %i base: 0x%lx, size: 0x%lx, type %s\n",
	   res_n,(*p_base),(1 << (*p_size)), (type & ResMem) ? "Mem" : "Io");
#endif
    if (res_n != 0xff) {
	if (type & ResMem)
	    pvp->memBase[prt] = range.rBegin;
	else
	    pvp->ioBase[prt] = range.rBegin;
	((CARD32 *)(&(pcp->pci_base0)))[res_n] =
	    (CARD32)(*p_base) | (CARD32)(p_type);
	pciWriteLong(tag, PCI_CMD_BASE_REG + res_n * sizeof(CARD32),
		     ((CARD32 *)(&(pcp->pci_base0)))[res_n]);
	if (PCI_MAP_IS64BITMEM(p_type)) {
#if defined LONG64 || defined WORD64
	    ((CARD32 *)(&(pcp->pci_base0)))[res_n + 1] =
		(CARD32)(*p_base >> 32);
	    pciWriteLong(tag, PCI_CMD_BASE_REG + (res_n + 1) * sizeof(CARD32),
	    		 ((CARD32 *)(&(pcp->pci_base0)))[res_n + 1]);
#else
	    ((CARD32 *)(&(pcp->pci_base0)))[res_n + 1] = 0;
	    pciWriteLong(tag, PCI_CMD_BASE_REG + (res_n + 1) * sizeof(CARD32),
			 0);
#endif
	}
    } else {
	pvp->biosBase = range.rBegin;
	pcp->pci_baserom = (pciReadLong(tag,PCI_CMD_BIOS_REG) & 0x01) |
	    (CARD32)(*p_base);
	pciWriteLong(tag, PCI_CMD_BIOS_REG, pcp->pci_baserom);
    }
    /* @@@ fake BIOS allocated resource */
    range.type |= ResBios;
    Acc = xf86AddResToList(Acc, &range,-1);
    
    return TRUE;
    
}

Bool
xf86FixPciResource(int entityIndex, int prt, memType alignment,
		    long type)
{
    pciVideoPtr pvp = xf86GetPciInfoForEntity(entityIndex);
    return fixPciResource(prt, alignment, pvp, type);
}

resPtr
xf86ReallocatePciResources(int entityIndex, resPtr pRes)
{
    pciVideoPtr pvp = xf86GetPciInfoForEntity(entityIndex);
    resPtr pBad = NULL,pResTmp;
    unsigned int prt = 0;
    int i;
    
    if (!pvp) return pRes;

    while (pRes) {
	switch (pRes->res_type & ResPhysMask) {
	case ResMem:
	    if (pRes->block_begin == B2M(TAG(pvp),pvp->biosBase) &&
		pRes->block_end == B2M(TAG(pvp),pvp->biosBase
				       + SIZE(pvp->biosSize)))
		prt = 6;
	    else for (i = 0 ; i < 6; i++) 
		if ((pRes->block_begin == B2M(TAG(pvp),pvp->memBase[i]))
		    && (pRes->block_end == B2M(TAG(pvp),pvp->memBase[i]
					      + SIZE(pvp->size[i])))) {
		    prt = i;
		    break;
		}
	    break;
	case ResIo:
	    for (i = 0 ; i < 6; i++) 
		if (pRes->block_begin == B2I(TAG(pvp),pvp->ioBase[i])
		    && pRes->block_end == B2I(TAG(pvp),pvp->ioBase[i]
		    + SIZE(pvp->size[i]))) {
		    prt = i;
		    break;
		}
	    break;
	}

	if (!prt) return pRes;

	pResTmp = pRes->next;
	if (! fixPciResource(prt, 0, pvp, pRes->res_type)) {
	    pRes->next = pBad;
	    pBad = pRes;
	} else
	    xfree(pRes);
	
	pRes = pResTmp;
    }
    return pBad;
}

/*
 * BIOS releated
 */

memType
getValidBIOSBase(PCITAG tag, int *num)
{
    pciVideoPtr pvp = NULL;
    PciBusPtr pbp, pbp1;
    resPtr m = NULL;
    resPtr tmp, avoid;
    resRange range;
    int n = 0;
    CARD32 biosSize, alignment;

    if (!xf86PciVideoInfo) return 0;
    
    while ((pvp = xf86PciVideoInfo[n++])) {
	if (pciTag(pvp->bus,pvp->device,pvp->func) == tag)
	    break;
    }
    if (!pvp) return 0;

    biosSize = pvp->biosSize;
    alignment = (1 << biosSize) - 1;
    if (biosSize > 24)
	biosSize = 24;
    avoid = xf86DupResList(pciAvoidRes);

    pbp = pbp1 = xf86PciBus;
    while (pbp) {
	if (pbp->secondary == pvp->bus) {
	    if (pbp->preferred_pmem)
	        tmp = xf86DupResList(pbp->preferred_pmem);
	    else
	        tmp = xf86DupResList(pbp->pmem);
	    m = xf86JoinResLists(m,tmp);
	    if (pbp->preferred_mem)
	        tmp = xf86DupResList(pbp->preferred_mem);
	    else
	        tmp = xf86DupResList(pbp->mem);
	    m = xf86JoinResLists(m,tmp);
	    tmp = m;
	    while (tmp) {
		tmp->block_end = MIN(tmp->block_end,PCI_MEM32_LENGTH_MAX);
		tmp = tmp->next;
	    }
	}
	while (pbp1) {
	    if (pbp1->primary == pvp->bus) {
		tmp = xf86DupResList(pbp1->preferred_pmem);
		avoid = xf86JoinResLists(avoid,tmp);
		tmp = xf86DupResList(pbp1->pmem);
		avoid = xf86JoinResLists(avoid,tmp);
		tmp = xf86DupResList(pbp1->preferred_mem);
		avoid = xf86JoinResLists(avoid,tmp);
		tmp = xf86DupResList(pbp1->mem);
		avoid = xf86JoinResLists(avoid,tmp);
	    }
	    pbp1 = pbp1->next;
	}	
	pbp = pbp->next;
    }	
    pciConvertListToHost(pvp->bus,pvp->device,pvp->func, avoid);

    if (pvp->biosBase) { /* try biosBase first */
	P_M_RANGE(range, TAG(pvp),pvp->biosBase,biosSize,ResExcMemBlock);
	if (xf86IsSubsetOf(range,m) && ! ChkConflict(&range,avoid,SETUP)) {
	    xf86FreeResList(avoid);
	    xf86FreeResList(m);
	    return pvp->biosBase;
	}
    }

    /* Validate alternate base, and, on failure, look for another one */
    if ((*num < 0) || (*num > 5) ||
	!pvp->memBase[*num] || (pvp->size[*num] < biosSize)) {
	*num = -1;
	for (n = 0;  n <= 5;  n++) {
	    if (pvp->memBase[n] && (pvp->size[n] >= biosSize)) {
		*num = n;
		break;
	    }
	}
    }

    if (*num >= 0) {
	/* then try suggested memBase */
	/* keep bios size ! */
	P_M_RANGE(range,TAG(pvp),pvp->memBase[*num],biosSize,ResExcMemBlock);
	if (xf86IsSubsetOf(range,m) && !ChkConflict(&range,avoid,SETUP)) {
	    xf86FreeResList(avoid);
	    xf86FreeResList(m);
	    return pvp->memBase[*num];
	}
    }
    while (m) {
	range = xf86GetBlock(ResExcMemBlock,
			     PCI_SIZE(ResMem,TAG(pvp),(1 << biosSize)),
			     m->block_begin, m->block_end,
			     PCI_SIZE(ResMem,TAG(pvp),alignment), avoid);
	if (range.type != ResEnd)
	    break;
	m = m->next;
    }
    
    xf86FreeResList(avoid);
    xf86FreeResList(m);
    xf86MsgVerb(X_INFO,5,"GetVaildBIOSBase for %x:%x:%x: BIOSbase 0x%lx\n",
		pvp->bus,pvp->device,pvp->func, 
		(memType)M2B(TAG(pvp),range.rBase));
    return M2B(TAG(pvp),range.rBase);
}

/*
 * xf86Bus.c interface
 */

void
xf86PciProbe(void)
{
    typedef void DataSetupFuncType(SymTabPtr *, pciVendorDeviceInfo **,
				   pciVendorCardInfo **);
    DataSetupFuncType *DataSetupFunc;
#ifdef XFree86LOADER
    /* 
     * we need to get the pointer to the pci data structures initialized
     */

    DataSetupFunc = (DataSetupFuncType *)LoaderSymbol("xf86SetupPciData");
#else
    DataSetupFunc = xf86SetupScanPci;
#endif

    (*DataSetupFunc)(&xf86PCIVendorNameInfo, &xf86PCIVendorInfo, &
		      xf86PCICardInfo);
    FindPCIVideoInfo();
}

static void alignBridgeRanges(PciBusPtr PciBusBase, PciBusPtr primary);

static void
printBridgeInfo(PciBusPtr PciBus) 
{
  xf86MsgVerb(X_INFO, 3, "Bus %d: bridge is at (%d:%d:%d), "
	      "(%d,%d,%d), BCTRL: 0x%02x (VGA_EN is %s)\n",
	      PciBus->secondary,PciBus->brbus, PciBus->brdev,
	      PciBus->brfunc, PciBus->primary,
	      PciBus->secondary, PciBus->subordinate,
	      PciBus->brcontrol,
	      (PciBus->brcontrol & PCI_PCI_BRIDGE_VGA_EN)
	      ? "set" : "cleared");
  xf86MsgVerb(X_INFO, 3, "Bus %d I/O range:\n",
	      PciBus->secondary);
  xf86PrintResList(3, PciBus->preferred_io);
  xf86MsgVerb(X_INFO, 3,
	      "Bus %d non-prefetchable memory range:\n",
	      PciBus->secondary);
  xf86PrintResList(3, PciBus->preferred_mem);
  xf86MsgVerb(X_INFO, 3, "Bus %d prefetchable memory range:"
	      "\n",PciBus->secondary);
  xf86PrintResList(3, PciBus->preferred_pmem);
}

/*
 * This Sun PCI-->PCI bridge must be handled specially since it does
 * not report the decoded I/O and MEM ranges in the usual way.
 */
#define APB_IO_ADDRESS_MAP	0xde
#define APB_MEM_ADDRESS_MAP	0xdf

static void
get_sun_apb_ranges(PciBusPtr PciBus, pciConfigPtr pcrp)
{
    unsigned char iomap, memmap;
    resRange range;
    int i;

    iomap = pciReadByte(pcrp->tag, APB_IO_ADDRESS_MAP);
    memmap = pciReadByte(pcrp->tag, APB_MEM_ADDRESS_MAP);

    /* if (pcrp->pci_command & PCI_CMD_IO_ENABLE) */ {	/* ??? */
	for (i = 0; i < 8; i++) {
	    if ((iomap & (1 << i)) != 0) {
		PCI_I_RANGE(range, pcrp->tag,
		    (i << 21), (i << 21) + ((1 << 21) - 1),
		    ResIo | ResBlock | ResExclusive);
		PciBus->io = xf86AddResToList(PciBus->io, &range, -1);
	    }
	}
    }

    /* if (pcrp->pci_command & PCI_CMD_MEM_ENABLE) */ {	/* ??? */
	for (i = 0; i < 8; i++) {
	    if ((memmap & (1 << i)) != 0) {
		PCI_M_RANGE(range, pcrp->tag,
		    (i << 29), (i << 29) + ((1 << 29) - 1),
		    ResMem | ResBlock | ResExclusive);
		PciBus->mem = xf86AddResToList(PciBus->mem, &range, -1);
	    }
	}
    }
}

PciBusPtr
xf86GetPciBridgeInfo(const pciConfigPtr *pciInfo)
{
    const pciConfigPtr *pcrpp;
    pciConfigPtr pcrp;
    resRange range;
    PciBusPtr PciBus, PciBusBase = NULL;
    PciBusPtr *pnPciBus = &PciBusBase;
    int MaxBus = 0;
    int i;
    memType base, limit;
    
    if (pciInfo == NULL) return NULL;
    
    /* Add each PCI-PCI bridge */
    for (pcrpp = pciInfo, pcrp = *pcrpp; pcrp; pcrp = *(++pcrpp)) {
	if (pcrp->busnum > MaxBus)
	    MaxBus = pcrp->busnum;
	if ((pcrp->pci_base_class == PCI_CLASS_BRIDGE) 
	    || (((pcrp->listed_class >> 8) & 0xff) == PCI_CLASS_BRIDGE)) {
	    int sub_class; 
	    sub_class = (pcrp->listed_class & 0xffff) 
	      ? (pcrp->listed_class & 0xff) : pcrp->pci_sub_class; 
	    switch (sub_class) {
	    case PCI_SUBCLASS_BRIDGE_PCI:
		/* something fishy about the header? If so: just ignore! */
		if ((pcrp->pci_header_type & 0x7f) != 0x01) {
		    xf86MsgVerb(3,X_WARNING,"PCI-PCI bridge at %x:%x:%x has "
				"funny header: 0x%x",pcrp->busnum,pcrp->devnum,
				pcrp->funcnum,pcrp->pci_header_type);
		    break;
		}
		*pnPciBus = PciBus = xnfcalloc(1, sizeof(PciBusRec));
		pnPciBus = &PciBus->next;
		PciBus->secondary = pcrp->pci_secondary_bus_number;
		PciBus->primary = pcrp->pci_primary_bus_number;
		PciBus->subordinate = pcrp->pci_subordinate_bus_number;
		PciBus->brbus = pcrp->busnum;
		PciBus->brdev = pcrp->devnum;
		PciBus->brfunc = pcrp->funcnum;
		PciBus->subclass = sub_class;
		PciBus->interface = pcrp->pci_prog_if;
		PciBus->brcontrol = pcrp->pci_bridge_control;
		if (pcrp->pci_vendor == PCI_VENDOR_SUN &&
		    pcrp->pci_device == 0x5000) {
			get_sun_apb_ranges(PciBus, pcrp);
			break;
		}
		if ((pcrp->pci_command & PCI_CMD_IO_ENABLE) &&
		    (pcrp->pci_upper_io_base || pcrp->pci_io_base ||
		     pcrp->pci_upper_io_limit || pcrp->pci_io_limit)) {
		    base = (pcrp->pci_upper_io_base << 16) |
			((pcrp->pci_io_base & 0xf0u) << 8);
		    limit = (pcrp->pci_upper_io_limit << 16) |
			((pcrp->pci_io_limit & 0xf0u) << 8) | 0x0fff;
		    /*
		     * Deal with bridge ISA mode (256 wide ranges spaced 1K
		     * apart, but only in the first 64K).
		     */
		    if (pcrp->pci_bridge_control & PCI_PCI_BRIDGE_ISA_EN) {
			while ((base <= (CARD16)(-1)) && (base <= limit)) {
			    PCI_I_RANGE(range, pcrp->tag,
				base, base + (CARD8)(-1),
				ResIo | ResBlock | ResExclusive);
			    PciBus->preferred_io = xf86AddResToList(
				PciBus->preferred_io,&range, -1);
			    base += 0x0400;
			}
		    }
		    if (base <= limit) {
			PCI_I_RANGE(range, pcrp->tag, base, limit,
			    ResIo | ResBlock | ResExclusive);
			PciBus->preferred_io = xf86AddResToList(
			    PciBus->preferred_io, &range, -1);
		    }
		}
		if (pcrp->pci_command & PCI_CMD_MEM_ENABLE) {
		  /*
		   * The P2P spec requires these next two, but some bridges
		   * don't comply.  Err on the side of caution, making the not
		   * so bold assumption that no bridge would ever re-route the
		   * bottom megabyte.
		   */
		  if (pcrp->pci_mem_base || pcrp->pci_mem_limit) {
                    base = pcrp->pci_mem_base & 0xfff0u;
                    limit = pcrp->pci_mem_limit & 0xfff0u;
		    if (base <= limit) {
			PCI_M_RANGE(range, pcrp->tag,
				    base << 16, (limit << 16) | 0x0fffff,
				    ResMem | ResBlock | ResExclusive);
			PciBus->preferred_mem 
			    = xf86AddResToList(NULL, &range, -1);
		    }
		  }

		  if (pcrp->pci_prefetch_mem_base ||
		      pcrp->pci_prefetch_mem_limit ||
		      pcrp->pci_prefetch_upper_mem_base ||
		      pcrp->pci_prefetch_upper_mem_limit) {
                    base = pcrp->pci_prefetch_mem_base & 0xfff0u;
                    limit = pcrp->pci_prefetch_mem_limit & 0xfff0u;
#if defined(LONG64) || defined(WORD64)
		    base |= (memType)pcrp->pci_prefetch_upper_mem_base << 16;
		    limit |= (memType)pcrp->pci_prefetch_upper_mem_limit << 16;
#endif
		    if (base <= limit) {
			PCI_M_RANGE(range, pcrp->tag,
				    base << 16, (limit << 16) | 0xfffff,
				    ResMem | ResBlock | ResExclusive);
			PciBus->preferred_pmem 
			    = xf86AddResToList(NULL, &range, -1);
		    }
		  }
		}
		break;
	    case PCI_SUBCLASS_BRIDGE_ISA:
		*pnPciBus = PciBus = xnfcalloc(1, sizeof(PciBusRec));
		pnPciBus = &PciBus->next;
		PciBus->primary = 0;
		PciBus->secondary = -1;
		PciBus->brbus = pcrp->busnum;
		PciBus->brdev = pcrp->devnum;
		PciBus->brfunc = pcrp->funcnum;
		PciBus->subclass = sub_class;
		xf86MsgVerb(X_INFO,3,"PCI-to-ISA bridge:\n");
		break;
	    case PCI_SUBCLASS_BRIDGE_HOST:
		*pnPciBus = PciBus = xnfcalloc(1, sizeof(PciBusRec));
		pnPciBus = &PciBus->next;
		PciBus->primary = -1;
		PciBus->secondary = -1; /* to be set below */
		PciBus->subclass = sub_class;
		PciBus->preferred_io = xf86ExtractTypeFromList(
		    xf86PciBusAccWindowsFromOS(),ResIo);
		PciBus->preferred_mem = xf86ExtractTypeFromList(
		    xf86PciBusAccWindowsFromOS(),ResMem);
		PciBus->preferred_pmem = xf86ExtractTypeFromList(
		    xf86PciBusAccWindowsFromOS(),ResMem);
		xf86MsgVerb(X_INFO,3,"Host-to-PCI bridge:\n");
		break;
	    default:
		break;
	    }
	}
    }
    for (i = 0; i <= MaxBus; i++) { /* find PCI buses not attached to bridge */
	for (PciBus = PciBusBase; PciBus; PciBus = PciBus->next)
	    if (PciBus->secondary == i) break;
	if (!PciBus) {  /* We assume it's behind a HOST-PCI bridge */
	    int minTag = 0xFFFFFF; /*find the 'smallest' free HOST-PCI bridge*/
	    int tag;               /*'small' is in the order of pciTag()     */
	    PciBusPtr PciBusFound = NULL;
	    for (PciBus = PciBusBase; PciBus; PciBus = PciBus->next)
		if ((PciBus->subclass == PCI_SUBCLASS_BRIDGE_HOST) &&
		    (PciBus->secondary == -1) &&
		    ((tag = pciTag(PciBus->brbus,PciBus->brdev,PciBus->brfunc))
		     < minTag) )  {
		    minTag = tag;
		    PciBusFound = PciBus;
		}
	    if (PciBusFound)      
		PciBusFound->secondary = i;
	    else {  /* if nothing found it may not be visible: create new */
		*pnPciBus = PciBus = xnfcalloc(1, sizeof(PciBusRec));
		pnPciBus = &PciBus->next;
		PciBus->primary = -1;
		PciBus->secondary = i;
		PciBus->subclass = PCI_SUBCLASS_BRIDGE_HOST;
		PciBus->preferred_io = xf86ExtractTypeFromList(
		    xf86PciBusAccWindowsFromOS(),ResIo);
		PciBus->preferred_mem = xf86ExtractTypeFromList(
		    xf86PciBusAccWindowsFromOS(),ResMem);
		PciBus->preferred_pmem = xf86ExtractTypeFromList(
		    xf86PciBusAccWindowsFromOS(),ResMem);
		xf86MsgVerb(X_INFO,3,"Host-to-PCI bridge:\n");
	    }
	}
    }
    
    for (PciBus = PciBusBase; PciBus; PciBus = PciBus->next) {
	if (PciBus->subclass == PCI_SUBCLASS_BRIDGE_HOST) {
	    alignBridgeRanges(PciBusBase, PciBus);
	}
    }
    for (PciBus = PciBusBase; PciBus; PciBus = PciBus->next) {
	if (PciBus->subclass == PCI_SUBCLASS_BRIDGE_PCI &&
	    PciBus->interface == PCI_IF_BRIDGE_PCI_SUBTRACTIVE) {
	    PciBusPtr PciBus1;
	    for (PciBus1 = PciBusBase; PciBus1; PciBus1 = PciBus->next) {
		if (PciBus1->secondary == PciBus->primary) {
		    PciBus->io = PciBus1->io ? PciBus1->io
			: PciBus1->preferred_io;
		    PciBus->mem = PciBus1->mem ? PciBus1->mem
			: PciBus1->preferred_mem;
		    PciBus->pmem = PciBus1->pmem ? PciBus1->pmem
			: PciBus1->preferred_pmem;
		    xf86MsgVerb(X_INFO,3,"Subtractive PCI-to-PCI bridge:\n");
		    break;
		}
	    }
	}
	printBridgeInfo(PciBus);
    }
    
    return PciBusBase;
    
}

static void
alignBridgeRanges(PciBusPtr PciBusBase, PciBusPtr primary)
{
    PciBusPtr PciBus;

    for (PciBus = PciBusBase; PciBus; PciBus = PciBus->next) {
	if ((PciBus->primary == primary->secondary)
	    && (PciBus->subclass == PCI_SUBCLASS_BRIDGE_PCI)) {
	    resPtr tmp;
	    tmp = xf86FindIntersectOfLists(primary->preferred_io,
					   PciBus->preferred_io);
		    xf86FreeResList(PciBus->preferred_io);
		    PciBus->preferred_io = tmp;
		    tmp = xf86FindIntersectOfLists(primary->preferred_pmem,
						   PciBus->preferred_pmem);
		    xf86FreeResList(PciBus->preferred_pmem);
		    PciBus->preferred_pmem = tmp;
		    tmp = xf86FindIntersectOfLists(primary->preferred_mem,
						   PciBus->preferred_mem);
		    xf86FreeResList(PciBus->preferred_mem);
		    PciBus->preferred_mem = tmp;
		    xf86MsgVerb(X_INFO,3,"PCI-to-PCI bridge:\n");
		    alignBridgeRanges(PciBusBase, PciBus);
	}
    }
}

void
ValidatePci(void)
{
    pciVideoPtr pvp, pvp1;
    PciBusPtr pbp, pbp1;
    pciConfigPtr pcrp, *pcrpp;
    CARD32 *basep;
    resPtr Sys;
    resPtr Fix;
    resRange range;
    int n = 0, m, i;

    if (!xf86PciVideoInfo) return;

    /*
     * Mark all pciInfoRecs that need to be validated. These are
     * the ones which have been assigned to a screen.
     */
    Sys = NULL;
    for (i=0; i<xf86NumScreens; i++) {
	for (m = 0; m < xf86Screens[i]->numEntities; m++)
	    if ((pvp = xf86GetPciInfoForEntity(xf86Screens[i]->entityList[m])))
		pvp->validate = TRUE;
    }
    
    /*
     * Collect all background PCI resources we need to validate against.
     * These are all resources which don't belong to PCINONSYSTEMCLASSES
     * and which have not been assigned to an entity.
     */
    /* First get the PCIINFOCLASSES */
    m = 0;
    while ((pvp = xf86PciVideoInfo[m++])) {
	/* is it a PCINONSYSTEMCLASS? */
	if (PCINONSYSTEMCLASSES(pvp->class, pvp->subclass))
	    continue;
	/* has it an Entity assigned to it? */
	for (i=0; i<xf86NumEntities; i++) {
	    EntityPtr p = xf86Entities[i];
	    if (p->busType != BUS_PCI)
		continue;
	    if (p->pciBusId.bus == pvp->bus
		&& p->pciBusId.device == pvp->device
		&& p->pciBusId.func == pvp->func)
		break;
	}
	if (i != xf86NumEntities) /* found an Entity for this one */
	    continue;
	
	for (i = 0; i<6; i++) {
	    if (pvp->ioBase[i]) {
		PV_I_RANGE(range,pvp,i,ResExcIoBlock);
		Sys = xf86AddResToList(Sys,&range,-1);
	    } else if (pvp->memBase[i]) {
		PV_M_RANGE(range,pvp,i,ResExcMemBlock);
		Sys = xf86AddResToList(Sys,&range,-1);
	    }
	}
    }
    for (pcrpp = xf86PciInfo, pcrp = *pcrpp; pcrp; pcrp = *++(pcrpp)) {

	if (PCIINFOCLASSES(pcrp->pci_base_class, pcrp->pci_sub_class))
	    continue;
	
	if ((pcrp->pci_header_type & 0x7f) != 0)
	    continue;

	basep = &pcrp->pci_base0;
	for (i = 0; i < 6; i++) {
	    if (basep[i]) {
		if (PCI_MAP_IS_IO(basep[i])) 
		    P_I_RANGE(range,pcrp->tag,PCIGETIO(basep[i]),
			      pcrp->basesize[i], ResExcIoBlock)
		else if (!PCI_MAP_IS64BITMEM(basep[i]))
		    P_M_RANGE(range,pcrp->tag,PCIGETMEMORY(basep[i]),
			      pcrp->basesize[i],ResExcMemBlock)
		else {
		    i++;
#if defined LONG64 || defined WORD64
		    P_M_RANGE(range,pcrp->tag,PCIGETMEMORY64(basep[i-1]),
			      pcrp->basesize[i-1],ResExcMemBlock)
#else
			continue;
#endif
		} 
		Sys = xf86AddResToList(Sys, &range, -1);
	    }
	}
	if (pcrp->pci_baserom) {
	    P_M_RANGE(range,pcrp->tag,PCIGETROM(pcrp->pci_baserom),
		      pcrp->basesize[6],ResExcMemBlock);
	    Sys = xf86AddResToList(Sys, &range, -1);
	}
    }

    /*
     * The order the video devices are listed in is
     * just right: the lower buses come first.
     * This way we attempt to fix a conflict of
     * a lower bus device with a higher bus device
     * where we have more room to find different
     * resources.
     */
    while ((pvp = xf86PciVideoInfo[n++])) {
	resPtr res_mp = NULL, res_m_io = NULL;
	resPtr NonSys = NULL;
	resPtr tmp, avoid = NULL;

	if (!pvp->validate) continue;
	NonSys = xf86DupResList(Sys);
	m = n;
	while ((pvp1 = xf86PciVideoInfo[m++])) {
	    if (!pvp1->validate) continue;
	    for (i = 0; i<6; i++) {
		if (pvp1->ioBase[i]) {
		    PV_I_RANGE(range,pvp1,i,ResExcIoBlock);
		    NonSys = xf86AddResToList(NonSys,&range,-1);
		} else if (pvp1->memBase[i]) {
		    PV_M_RANGE(range,pvp1,i,ResExcMemBlock);
		    NonSys = xf86AddResToList(NonSys,&range,-1);
		}
	    }
	}
#ifdef DEBUG
	xf86MsgVerb(X_INFO, 3,"NonSys:\n");
	xf86PrintResList(3,NonSys);
#endif
	pbp = pbp1 = xf86PciBus;
	while (pbp) {
	    if (pbp->secondary == pvp->bus) {
		if (pbp->preferred_pmem) {
		    /* keep prefetchable separate */
		    res_mp = xf86FindIntersectOfLists(pbp->preferred_pmem,
						      ResRange);
		}
		if (pbp->pmem) {
		    res_mp = xf86FindIntersectOfLists(pbp->pmem, ResRange);
		}
		if (pbp->preferred_mem) {
		    res_m_io = xf86FindIntersectOfLists(pbp->preferred_mem,
							ResRange);
		}
		if (pbp->mem) {
		    res_m_io = xf86FindIntersectOfLists(pbp->mem, ResRange);
		}
		if (pbp->preferred_io) {
		    res_m_io = xf86JoinResLists(res_m_io,
			xf86FindIntersectOfLists(pbp->preferred_io,ResRange));
		}
		if (pbp->io) {
		    res_m_io = xf86JoinResLists(res_m_io,
			xf86FindIntersectOfLists(pbp->preferred_io,ResRange));
		}
	    }
	    while (pbp1) {
		if (pbp1->primary == pvp->bus) {
		    tmp = xf86DupResList(pbp1->preferred_pmem);
		    avoid = xf86JoinResLists(avoid,tmp);
		    tmp = xf86DupResList(pbp1->preferred_mem);
		    avoid = xf86JoinResLists(avoid,tmp);
		    tmp = xf86DupResList(pbp1->preferred_io);
		    avoid = xf86JoinResLists(avoid,tmp);
		}
		pbp1 = pbp1->next;
	    }	
	    pbp = pbp->next;
	}
	if (res_m_io == NULL)
	   res_m_io = xf86DupResList(ResRange);

	pciConvertListToHost(pvp->bus,pvp->device,pvp->func, avoid);

#ifdef DEBUG
	xf86MsgVerb(X_INFO, 3,"avoid:\n");
	xf86PrintResList(3,avoid);
	xf86MsgVerb(X_INFO, 3,"prefetchable Memory:\n");
	xf86PrintResList(3,res_mp);
	xf86MsgVerb(X_INFO, 3,"MEM/IO:\n");
	xf86PrintResList(3,res_m_io);
#endif
	Fix = NULL;
	for (i = 0; i < 6; i++) {
	    int j;
	    resPtr own = NULL;
	    for (j = i+1; j < 6; j++) {
		if (pvp->ioBase[j]) {
		    PV_I_RANGE(range,pvp,j,ResExcIoBlock);
		    own = xf86AddResToList(own,&range,-1);
		} else if (pvp->memBase[j]) {
		    PV_M_RANGE(range,pvp,j,ResExcMemBlock);
		    own = xf86AddResToList(own,&range,-1);
		}
	    }
#ifdef DEBUG
	xf86MsgVerb(X_INFO, 3,"own:\n");
	xf86PrintResList(3,own);
#endif
	    if (pvp->ioBase[i]) {
		PV_I_RANGE(range,pvp,i,ResExcIoBlock);
		if (xf86IsSubsetOf(range,res_m_io)
		    && ! ChkConflict(&range,own,SETUP)
		    && ! ChkConflict(&range,avoid,SETUP)
		    && ! ChkConflict(&range,NonSys,SETUP))
		    continue;
		xf86MsgVerb(X_WARNING, 0,
			"****INVALID IO ALLOCATION**** b: 0x%lx e: 0x%lx "
			"correcting\a\n", range.rBegin,range.rEnd);
#ifdef DEBUG
		sleep(2);
#endif
		fixPciResource(i, 0, pvp, range.type);
	    } else if (pvp->memBase[i]) {
		PV_M_RANGE(range,pvp,i,ResExcMemBlock);
		if (pvp->type[i] & PCI_MAP_MEMORY_CACHABLE) {
		    if (xf86IsSubsetOf(range,res_mp)
			&& ! ChkConflict(&range,own,SETUP)
			&& ! ChkConflict(&range,avoid,SETUP)
			&& ! ChkConflict(&range,NonSys,SETUP))
			continue;
		}
		if (xf86IsSubsetOf(range,res_m_io)
		    && ! ChkConflict(&range,own,SETUP)
		    && ! ChkConflict(&range,avoid,SETUP)
		    && ! ChkConflict(&range,NonSys,SETUP))
		    continue;
		xf86MsgVerb(X_WARNING, 0,
			"****INVALID MEM ALLOCATION**** b: 0x%lx e: 0x%lx "
			"correcting\a\n", range.rBegin,range.rEnd);
		if (ChkConflict(&range,own,SETUP)) {
		    xf86MsgVerb(X_INFO,3,"own\n");
		    xf86PrintResList(3,own);
		}
		if (ChkConflict(&range,avoid,SETUP)) {
		    xf86MsgVerb(X_INFO,3,"avoid\n");
		    xf86PrintResList(3,avoid);
		}
		if (ChkConflict(&range,NonSys,SETUP)) {
		    xf86MsgVerb(X_INFO,3,"NonSys\n");
		    xf86PrintResList(3,NonSys);
		}

#ifdef DEBUG
		sleep(2);
#endif
		fixPciResource(i, 0, pvp, range.type);
	    }
	    xf86FreeResList(own);
	}
	xf86FreeResList(avoid);
	xf86FreeResList(NonSys);
	xf86FreeResList(res_mp);
	xf86FreeResList(res_m_io);
    }
    xf86FreeResList(Sys);
    return;
}
    
resList
GetImplicitPciResources(int entityIndex)
{
    pciVideoPtr pvp;
    int i;
    resList list = NULL;
    int num = 0;
    
    if (! (pvp = xf86GetPciInfoForEntity(entityIndex))) return NULL;

    for (i = 0; i < 6; i++) {
	if (pvp->ioBase[i]) {
	    list = xnfrealloc(list,sizeof(resRange) * (++num));
	    PV_I_RANGE(list[num - 1],pvp,i,ResShrIoBlock | ResBios); 
	} else if (pvp->memBase[i]) {
	    list = xnfrealloc(list,sizeof(resRange) * (++num));
	    PV_M_RANGE(list[num - 1],pvp,i,ResShrMemBlock | ResBios);
	}
    }
#if 0
    if (pvp->biosBase) {
	list = xnfrealloc(list,sizeof(resRange) * (++num));
	PV_B_RANGE(list[num - 1],pvp,ResShrMemBlock | ResBios);
    }
#endif
    list = xnfrealloc(list,sizeof(resRange) * (++num));
    list[num - 1].type = ResEnd;
    
    return list;
}

void
initPciState(void)
{
    int i = 0;
    int j = 0;
    pciVideoPtr pvp; 
    pciAccPtr pcaccp;

    if (xf86PciAccInfo != NULL)
	return;
  
    if (xf86PciVideoInfo == NULL)
	return;

    while ((pvp = xf86PciVideoInfo[i]) != NULL) {
  	i++;
  	    j++;
  	    xf86PciAccInfo = xnfrealloc(xf86PciAccInfo,
  					sizeof(pciAccPtr) * (j + 1));
  	    xf86PciAccInfo[j] = NULL;
  	    pcaccp = xf86PciAccInfo[j - 1] = xnfalloc(sizeof(pciAccRec));
	    pcaccp->busnum = pvp->bus; 
 	    pcaccp->devnum = pvp->device; 
 	    pcaccp->funcnum = pvp->func;
 	    pcaccp->arg.tag = pciTag(pvp->bus, pvp->device, pvp->func);
 	    pcaccp->arg.func =
	        (WriteProcPtr)pciLongFunc(pcaccp->arg.tag,WRITE);
  	    pcaccp->ioAccess.AccessDisable = pciIoAccessDisable;
  	    pcaccp->ioAccess.AccessEnable = pciIoAccessEnable;
  	    pcaccp->ioAccess.arg = &pcaccp->arg;
	    pcaccp->io_memAccess.AccessDisable = pciIo_MemAccessDisable;
	    pcaccp->io_memAccess.AccessEnable = pciIo_MemAccessEnable;
	    pcaccp->io_memAccess.arg = &pcaccp->arg;
	    pcaccp->memAccess.AccessDisable = pciMemAccessDisable;
	    pcaccp->memAccess.AccessEnable = pciMemAccessEnable;
	    pcaccp->memAccess.arg = &pcaccp->arg;
 	    if (PCISHAREDIOCLASSES(pvp->class, pvp->subclass))
 		pcaccp->ctrl = TRUE;
 	    else
 		pcaccp->ctrl = FALSE;
 	    savePciState(pcaccp->arg.tag, &pcaccp->save);
	    pcaccp->arg.ctrl = pcaccp->save.command;
    }
}

/*
 * initPciBusState() - fill out the BusAccRec for a PCI bus.
 * Theory: each bus is associated with one bridge connecting it
 * to its parent bus. The address of a bridge is therefore stored
 * in the BusAccRec of the bus it connects to. Each bus can
 * have several bridges connecting secondary buses to it. Only one
 * of these bridges can be open. Therefore the status of a bridge
 * associated with a bus is stored in the BusAccRec of the parent
 * the bridge connects to. The first member of the structure is
 * a pointer to a function that open access to this bus. This function
 * receives a pointer to the structure itself as argument. This
 * design should be common to BusAccRecs of any type of buses we
 * support. The remeinder of the structure is bus type specific.
 * In this case it contains a pointer to the structure of the
 * parent bus. Thus enabling access to a specific bus is simple:
 * 1. Close any bridge going to secondary buses.
 * 2. Climb down the ladder and enable any bridge on buses
 *    on the path from the CPU to this bus.
 */
 
void
initPciBusState(void)
{
    BusAccPtr pbap, pbap_tmp;
    PciBusPtr pbp = xf86PciBus;

    while (pbp) {
	pbap = xnfcalloc(1,sizeof(BusAccRec));
	pbap->busdep.pci.bus = pbp->secondary;
	pbap->busdep.pci.primary_bus = pbp->primary;
	pbap->busdep_type = BUS_PCI;
	pbap->busdep.pci.acc = PCITAG_SPECIAL;
	switch (pbp->subclass) {
	case PCI_SUBCLASS_BRIDGE_HOST:
#ifdef DEBUG
	    ErrorF("setting up HOST: %i\n",pbap->busdep.pci.bus);
#endif
	    pbap->type = BUS_PCI;
	    pbap->set_f = pciSetBusAccess;
	    break;
	case PCI_SUBCLASS_BRIDGE_PCI:
#ifdef DEBUG
	    ErrorF("setting up PCI: %i\n",pbap->busdep.pci.bus);
#endif
	    pbap->type = BUS_PCI;
	    pbap->save_f = savePciBusState;
	    pbap->restore_f = restorePciBusState;
	    pbap->set_f = pciSetBusAccess;
	    pbap->enable_f = pciBusAccessEnable;
	    pbap->disable_f = pciBusAccessDisable;
	    pbap->busdep.pci.acc = pciTag(pbp->brbus,pbp->brdev,pbp->brfunc);
	    pbap->busdep.pci.func =
		(SetBitsProcPtr)pciLongFunc(pbap->busdep.pci.acc,SET_BITS);
	    savePciBusState(pbap);
	    break;
	case PCI_SUBCLASS_BRIDGE_ISA:
#ifdef DEBUG
	    ErrorF("setting up ISA: %i\n",pbap->busdep.pci.bus);
#endif
	    pbap->type = BUS_ISA;
	    pbap->set_f = pciSetBusAccess;
	    break;
	}
	pbap->next = xf86BusAccInfo;
	xf86BusAccInfo = pbap;
	pbp = pbp->next;
    }
    
    pbap = xf86BusAccInfo;
    
    while (pbap) {
	pbap->primary = NULL;
	if (pbap->busdep_type == BUS_PCI
	    && pbap->busdep.pci.primary_bus > -1) {
	    pbap_tmp = xf86BusAccInfo;
	    while (pbap_tmp) {
		if (pbap_tmp->busdep_type == BUS_PCI && 
		    pbap_tmp->busdep.pci.bus == pbap->busdep.pci.primary_bus) {
		    pbap->primary = pbap_tmp;
		    break;
		}
		pbap_tmp = pbap_tmp->next;
	    }
	}
	pbap = pbap->next;
    }
}

void 
PciStateEnter(void)
{
    pciAccPtr paccp;
    int i = 0;
    
    if (xf86PciAccInfo == NULL) 
	return;

    while ((paccp = xf86PciAccInfo[i]) != NULL) {
	i++;
 	if (!paccp->ctrl)
 	    continue;
	savePciState(paccp->arg.tag, &paccp->save);
	restorePciState(paccp->arg.tag, &paccp->restore);
	paccp->arg.ctrl = paccp->restore.command;
    }
}

void
PciBusStateEnter(void)
{
    BusAccPtr pbap = xf86BusAccInfo;

    while (pbap) {
	if (pbap->save_f)
	    pbap->save_f(pbap);
	pbap = pbap->next;
    }
}

void 
PciStateLeave(void)
{
    pciAccPtr paccp;
    int i = 0;

    if (xf86PciAccInfo == NULL) 
	return;

    while ((paccp = xf86PciAccInfo[i]) != NULL) {
	i++;
	if (!paccp->ctrl)
	    continue;
	savePciState(paccp->arg.tag, &paccp->restore);
	restorePciState(paccp->arg.tag, &paccp->save);
    }
}

void
PciBusStateLeave(void)
{
    BusAccPtr pbap = xf86BusAccInfo;

    while (pbap) {
	if (pbap->restore_f)
	    pbap->restore_f(pbap);
	pbap = pbap->next;
    }
}

void 
DisablePciAccess(void)
{
    int i = 0;
    pciAccPtr paccp;
    if (xf86PciAccInfo == NULL)
	return;

    while ((paccp = xf86PciAccInfo[i]) != NULL) {
	i++;
	if (!paccp->ctrl) /* disable devices that are under control initially*/
	    continue;
	pciIo_MemAccessDisable(paccp->io_memAccess.arg);
    }
}

void
DisablePciBusAccess(void)
{
    BusAccPtr pbap = xf86BusAccInfo;

    while (pbap) {
	if (pbap->disable_f)
	    pbap->disable_f(pbap);
	if (pbap->primary)
	    pbap->primary->current = NULL;
	pbap = pbap->next;
    }
}

/*
 * Public functions
 */

Bool
xf86IsPciDevPresent(int bus, int dev, int func)
{
    int i = 0;
    pciConfigPtr pcp;
    
    while ((pcp = xf86PciInfo[i]) != NULL) {
	if ((pcp->busnum == bus)
	    && (pcp->devnum == dev)
	    && (pcp->funcnum == func))
	    return TRUE;
	i++;
    }
    return FALSE;
}

/*
 * If the slot requested is already in use, return -1.
 * Otherwise, claim the slot for the screen requesting it.
 */

int
xf86ClaimPciSlot(int bus, int device, int func, DriverPtr drvp,
		 int chipset, GDevPtr dev, Bool active)
{
    EntityPtr p = NULL;
    pciAccPtr *ppaccp = xf86PciAccInfo;
    BusAccPtr pbap = xf86BusAccInfo;
    
    int num;
    
    if (xf86CheckPciSlot(bus, device, func)) {
	num = xf86AllocateEntity();
	p = xf86Entities[num];
	p->driver = drvp;
	p->chipset = chipset;
	p->busType = BUS_PCI;
	p->pciBusId.bus = bus;
	p->pciBusId.device = device;
	p->pciBusId.func = func;
	p->active = active;
	p->inUse = FALSE;
        xf86AddDevToEntity(num, dev);
	/* Here we initialize the access structure */
	p->access = xnfcalloc(1,sizeof(EntityAccessRec));
	while (ppaccp && *ppaccp) {
	    if ((*ppaccp)->busnum == bus
		&& (*ppaccp)->devnum == device
		&& (*ppaccp)->funcnum == func) {
		p->access->fallback = &(*ppaccp)->io_memAccess;
		p->access->pAccess = &(*ppaccp)->io_memAccess;
 		(*ppaccp)->ctrl = TRUE; /* mark control if not already */
		break;
	    }
	    ppaccp++;
	}
	if (!ppaccp || !*ppaccp) {
	    p->access->fallback = &AccessNULL;
	    p->access->pAccess = &AccessNULL;
	}
	
	p->busAcc = NULL;
	while (pbap) {
	    if (pbap->type == BUS_PCI && pbap->busdep.pci.bus == bus)
		p->busAcc = pbap;
	    pbap = pbap->next;
	}
	fixPciSizeInfo(num);

	/* in case bios is enabled disable it */
	disablePciBios(pciTag(bus,device,func));
	pciSlotClaimed = TRUE;
	
 	return num;
    } else
 	return -1;
}

/*
 * Get xf86PciVideoInfo for a driver.
 */
pciVideoPtr *
xf86GetPciVideoInfo(void)
{
    return xf86PciVideoInfo;
}

/* --- Used by ATI driver, but also more generally useful */

/*
 * Get the full xf86scanpci data.
 */
pciConfigPtr *
xf86GetPciConfigInfo(void)
{
    return xf86PciInfo;
}

/*
 * Enable a device and route VGA to it.  This is intended for a driver's
 * Probe(), before creating EntityRec's.  Only one device can be thus enabled
 * at any one time, and should be disabled when the driver is done with it.
 *
 * The following special calls are also available:
 *
 * pvp == NULL && rt == NONE    disable previously enabled device
 * pvp != NULL && rt == NONE    ensure device is disabled
 * pvp == NULL && rt != NONE    disable >all< subsequent calls to this function
 *                              (done from xf86PostProbe())
 * The last combination has been removed! To do this cleanly we have
 * to implement stages and need to test at each stage dependent function
 * if it is allowed to execute.
 *
 * The device represented by pvp may not have been previously claimed.
 */
void
xf86SetPciVideo(pciVideoPtr pvp, resType rt)
{
    static BusAccPtr pbap = NULL;
    static xf86AccessPtr pAcc = NULL;
    static Bool DoneProbes = FALSE;
    pciAccPtr pcaccp;
    int i;

    if (DoneProbes)
	return;

    /* Disable previous access */
    if (pAcc) {
	if (pAcc->AccessDisable)
	    (*pAcc->AccessDisable)(pAcc->arg);
	pAcc = NULL;
    }
    if (pbap) {
	while (pbap->primary) {
	    if (pbap->disable_f)
		(*pbap->disable_f)(pbap);
	    pbap->primary->current = NULL;
	    pbap = pbap->primary;
	}
	pbap = NULL;
    }

    /* Check for xf86PostProbe's magic combo */
    if (!pvp) {
	if (rt != NONE)
	    DoneProbes = TRUE;
	return;
    }

    /* Validate device */
    if (!xf86PciVideoInfo || !xf86PciAccInfo || !xf86BusAccInfo)
	return;

    for (i = 0; pvp != xf86PciVideoInfo[i]; i++)
	if (!xf86PciVideoInfo[i])
	    return;

    /* Ignore request for claimed adapters */
    if (!xf86CheckPciSlot(pvp->bus, pvp->device, pvp->func))
	return;

    /* Find pciAccRec structure */
    for (i = 0; ; i++) {
	if (!(pcaccp = xf86PciAccInfo[i]))
	    return;
	if ((pvp->bus == pcaccp->busnum) &&
	    (pvp->device == pcaccp->devnum) &&
	    (pvp->func == pcaccp->funcnum))
	    break;
    }

    if (rt == NONE) {
	/* This is a call to ensure the adapter is disabled */
	if (pcaccp->io_memAccess.AccessDisable)
	    (*pcaccp->io_memAccess.AccessDisable)(pcaccp->io_memAccess.arg);
	return;
    }

    /* Find BusAccRec structure */
    for (pbap = xf86BusAccInfo; ; pbap = pbap->next) {
	if (!pbap)
	    return;
	if (pvp->bus == pbap->busdep.pci.bus)
	    break;
    }

    /* Route VGA */
    if (pbap->set_f)
	(*pbap->set_f)(pbap);

    /* Enable device */
    switch (rt) {
    case IO:
	pAcc = &pcaccp->ioAccess;
	break;
    case MEM_IO:
	pAcc = &pcaccp->io_memAccess;
	break;
    case MEM:
	pAcc = &pcaccp->memAccess;
	break;
    default:	/* no compiler noise */
	break;
    }

    if (pAcc && pAcc->AccessEnable)
	(*pAcc->AccessEnable)(pAcc->arg);
}

/*
 * Parse a BUS ID string, and return the PCI bus parameters if it was
 * in the correct format for a PCI bus id.
 */

Bool
xf86ParsePciBusString(const char *busID, int *bus, int *device, int *func)
{
    /*
     * The format is assumed to be "bus:device:func", where bus, device
     * and func are decimal integers.  func may be omitted and assumed to
     * be zero, although it doing this isn't encouraged.
     */

    char *p, *s;
    const char *id;
    int i;

    if (StringToBusType(busID, &id) != BUS_PCI)
	return FALSE;

    s = xstrdup(id);
    p = strtok(s, ":");
    if (p == NULL || *p == 0) {
	xfree(s);
	return FALSE;
    }
    for (i = 0; p[i] != 0; i++) {
	if (!isdigit(p[i])) {
	    xfree(s);
	    return FALSE;
	}
    }
    *bus = atoi(p);
    p = strtok(NULL, ":");
    if (p == NULL || *p == 0) {
	xfree(s);
	return FALSE;
    }
    for (i = 0; p[i] != 0; i++) {
	if (!isdigit(p[i])) {
	    xfree(s);
	    return FALSE;
	}
    }
    *device = atoi(p);
    *func = 0;
    p = strtok(NULL, ":");
    if (p == NULL || *p == 0) {
	xfree(s);
	return TRUE;
    }
    for (i = 0; p[i] != 0; i++) {
	if (!isdigit(p[i])) {
	    xfree(s);
	    return FALSE;
	}
    }
    *func = atoi(p);
    xfree(s);
    return TRUE;
}

/*
 * Compare a BUS ID string with a PCI bus id.  Return TRUE if they match.
 */

Bool
xf86ComparePciBusString(const char *busID, int bus, int device, int func)
{
    int ibus, idevice, ifunc;

    if (xf86ParsePciBusString(busID, &ibus, &idevice, &ifunc)) {
	return bus == ibus && device == idevice && func == ifunc;
    } else {
	return FALSE;
    }
}

/*
 * xf86IsPrimaryPci() -- return TRUE if primary device
 * is PCI and bus, dev and func numbers match.
 */
 
Bool
xf86IsPrimaryPci(pciVideoPtr pPci)
{
    if (primaryBus.type != BUS_PCI) return FALSE;
    return (pPci->bus == primaryBus.id.pci.bus &&
	    pPci->device == primaryBus.id.pci.device &&
	    pPci->func == primaryBus.id.pci.func);
}

/*
 * xf86CheckPciGAType() -- return type of PCI graphics adapter.
 */
int
xf86CheckPciGAType(pciVideoPtr pPci)
{
    int i = 0;
    pciConfigPtr pcp;
    
    while ((pcp = xf86PciInfo[i]) != NULL) { 
	if (pPci->bus == pcp->busnum && pPci->device == pcp->devnum
	    && pPci->func == pcp->funcnum) {
	    if (pcp->pci_base_class == PCI_CLASS_PREHISTORIC &&
		pcp->pci_sub_class == PCI_SUBCLASS_PREHISTORIC_VGA)
		return PCI_CHIP_VGA ;
	    if (pcp->pci_base_class == PCI_CLASS_DISPLAY &&
		pcp->pci_sub_class == PCI_SUBCLASS_DISPLAY_VGA) {
		if (pcp->pci_prog_if == 0)
		    return PCI_CHIP_VGA ; 
		if (pcp->pci_prog_if == 1)
		    return PCI_CHIP_8514;
	    }
	    return -1;
	}
    i++;
    }
    return -1;
}

/*
 * xf86GetPciInfoForEntity() -- Get the pciVideoRec of entity.
 */
pciVideoPtr
xf86GetPciInfoForEntity(int entityIndex)
{
    pciVideoPtr *ppPci;
    EntityPtr p = xf86Entities[entityIndex];
    
    if (entityIndex >= xf86NumEntities
	|| p->busType != BUS_PCI) return NULL;
    
    for (ppPci = xf86PciVideoInfo; *ppPci != NULL; ppPci++) {
	if (p->pciBusId.bus == (*ppPci)->bus &&
	    p->pciBusId.device == (*ppPci)->device &&
	    p->pciBusId.func == (*ppPci)->func) 
	    return (*ppPci);
    }
    return NULL;
}

int
xf86GetPciEntity(int bus, int dev, int func)
{
    int i;
    
    for (i = 0; i < xf86NumEntities; i++) {
	EntityPtr p = xf86Entities[i];
	if (p->busType != BUS_PCI) continue;
	
	if (p->pciBusId.bus == bus &&
	    p->pciBusId.device == dev &&
	    p->pciBusId.func == func) 
	    return i;
    }
    return -1;
}

/*
 * xf86CheckPciMemBase() checks that the memory base value matches one of the
 * PCI base address register values for the given PCI device.
 */
Bool
xf86CheckPciMemBase(pciVideoPtr pPci, memType base)
{
    int i;

    for (i = 0; i < 6; i++)
	if (base == pPci->memBase[i])
	    return TRUE;
    return FALSE;
}

/*
 * Check if the slot requested is free.  If it is already in use, return FALSE.
 */

Bool
xf86CheckPciSlot(int bus, int device, int func)
{
    int i;
    EntityPtr p;

    for (i = 0; i < xf86NumEntities; i++) {
	p = xf86Entities[i];
	/* Check if this PCI slot is taken */
	if (p->busType == BUS_PCI && p->pciBusId.bus == bus &&
	    p->pciBusId.device == device && p->pciBusId.func == func)
	    return FALSE;
    }
    
    return TRUE;
}


CARD32 (*FindPCIClassInCardList)(
    unsigned short vendorID, unsigned short subsystemID);
CARD32 (*FindPCIClassInDeviceList)(
    unsigned short vendorID, unsigned short deviceID);

static void
getPciClassFlags(pciConfigPtr *pcrpp)
{
    pciConfigPtr pcrp;
    int i = 0;
#ifdef XFree86LOADER
    pointer scan_mod = NULL;
#endif

    if (!pcrpp)
	return;

#ifdef XFree86LOADER
    if (!(scan_mod = xf86LoadOneModule("scanpci", NULL))) {
	xf86Msg(X_WARNING,"Could not load scanpci module\n");
	return;
    } else {
	FindPCIClassInCardList = (CARD32 (*)(unsigned short, unsigned short))
	    LoaderSymbol("xf86FindPCIClassInCardList");
	FindPCIClassInDeviceList = (CARD32 (*)(unsigned short, unsigned short))
	    LoaderSymbol("xf86FindPCIClassInDeviceList");
    }
#else
    FindPCIClassInCardList = xf86FindPCIClassInCardList;
    FindPCIClassInDeviceList = xf86FindPCIClassInDeviceList;
#endif
    
    while ((pcrp = pcrpp[i])) {
	if (!(pcrp->listed_class = FindPCIClassInCardList(
	    pcrp->pci_subsys_vendor,pcrp->pci_subsys_card)))
	    pcrp->listed_class = FindPCIClassInDeviceList(
		pcrp->pci_vendor,pcrp->pci_device);
	i++;
    }

#ifdef XFree86LOADER
    UnloadModule(scan_mod);
#endif
}

/*
 * xf86FindPciVendorDevice() xf86FindPciClass(): These functions
 * are ment to be used by the pci bios emulation. Some bioses
 * need to see if there are _other_ chips of the same type around
 * so by setting pvp_exclude one pci device can be explicitely
 * _excluded if required.
 */
pciVideoPtr
xf86FindPciDeviceVendor(CARD16 vendorID, CARD16 deviceID,
			char n, pciVideoPtr pvp_exclude)
{
    pciVideoPtr pvp, *ppvp;
    n++;

    for (ppvp = xf86PciVideoInfo, pvp =*ppvp; pvp ; pvp = *(++ppvp)) {
	if (pvp == pvp_exclude) continue;
	if ((pvp->vendor == vendorID) && (pvp->chipType == deviceID)) {
	    if (!(--n)) break;
	}
    }
    return pvp;
}

pciVideoPtr
xf86FindPciClass(CARD8 intf, CARD8 subClass, CARD16 class,
		 char n, pciVideoPtr pvp_exclude)
{
    pciVideoPtr pvp, *ppvp;
    n++;
    
    for (ppvp = xf86PciVideoInfo, pvp =*ppvp; pvp ; pvp = *(++ppvp)) {
	if (pvp == pvp_exclude) continue;
	if ((pvp->interface == intf) && (pvp->subclass == subClass)
	    && (pvp->class == class)) {
	    if (!(--n)) break;
	}
    }
    return pvp;
}

/*
 * This attempts to detect a multi-device card and sets up a list
 * of pci tags of the devices of this card. On some of these
 * cards the BIOS is not visible from all chipsets. We therefore
 * need to use the BIOS from a chipset where it is visible.
 * We do the following heuristics:
 * If we detect only identical pci devices on a bus we assume it's
 * a multi-device card. This assumption isn't true always, however.
 * One might just use identical cards on a bus. We therefore don't
 * detect this situation when we set up the PCI video info. Instead
 * we wait until an attempt to read the BIOS fails.
 */
int
pciTestMultiDeviceCard(int bus, int dev, int func, PCITAG** pTag)
{
  pciConfigPtr *ppcrp = xf86PciInfo;
  pciConfigPtr pcrp = NULL;
  int i,j;
  Bool multicard = FALSE;
  Bool multifunc = FALSE;
  char str[256];
  char *str1;
  
  str1 = str;
  if (!pTag) 
    return 0;

  *pTag = NULL;
 
  for (i=0; i < 8; i++) {
    j = 0;

    while (ppcrp[j]) {
      if (ppcrp[j]->busnum == bus && ppcrp[j]->funcnum == i) {
	pcrp = ppcrp[j];
	break;
      }
      j++;
    }

    if (!pcrp) return 0;

    /* 
     * we check all functions here: since multifunc devices need
     * to implement func 0 we catch all devices on the bus when
     * i = 0
     */
    if (pcrp->pci_header_type &0x80) 
	multifunc = TRUE;
    
    j = 0;
    
    while (ppcrp[j]) {
      if (ppcrp[j]->busnum == bus && ppcrp[j]->funcnum == i
	  && ppcrp[j]->devnum != pcrp->devnum) {
	/* don't test subsys ID here. It might be set by POST 
	   - however some cards might not have been POSTed */
	if (ppcrp[j]->pci_device_vendor != pcrp->pci_device_vendor 
	    || ppcrp[j]->pci_header_type != pcrp->pci_header_type ) 
	  return 0;
	else
	  multicard = TRUE;
      }
      j++;
    }
    if (!multifunc)
      break;
  }

  if (!multicard) 
    return 0;

  j = 0;
  i = 0;
  while (ppcrp[i]) {
    if (ppcrp[i]->busnum == bus && ppcrp[i]->funcnum == func) {
      str1 += sprintf(str1,"[%x:%x:%x]",ppcrp[i]->busnum,
		      ppcrp[i]->devnum,ppcrp[i]->funcnum);
      *pTag = xnfrealloc(*pTag,sizeof(PCITAG) * (j + 1));
      (*pTag)[j++] = pciTag(ppcrp[i]->busnum,
			      ppcrp[i]->devnum,ppcrp[i]->funcnum);
    }
    i++;
  }
  xf86MsgVerb(X_INFO,3,"Multi Device Card detected: %s\n",str);
  return j;
}

static void
pciTagConvertRange2Host(PCITAG tag, resRange *pRange)
{
    switch(pRange->type & ResPhysMask) {
    case ResMem:
	switch(pRange->type & ResExtMask) {
	case ResBlock:
	    pRange->rBegin = pciBusAddrToHostAddr(tag,PCI_MEM, pRange->rBegin);
	    pRange->rEnd = pciBusAddrToHostAddr(tag,PCI_MEM, pRange->rEnd);
	    break;
	case ResSparse:
	    pRange->rBase = pciBusAddrToHostAddr(tag,PCI_MEM_SPARSE_BASE,
						  pRange->rBegin);
	    pRange->rMask = pciBusAddrToHostAddr(tag,PCI_MEM_SPARSE_MASK,
						pRange->rEnd);
	    break;
	}
	break;
    case ResIo:
	switch(pRange->type & ResExtMask) {
	case ResBlock:
	    pRange->rBegin = pciBusAddrToHostAddr(tag,PCI_IO, pRange->rBegin);
	    pRange->rEnd = pciBusAddrToHostAddr(tag,PCI_IO, pRange->rEnd);
	    break;
	case ResSparse:
	    pRange->rBase = pciBusAddrToHostAddr(tag,PCI_IO_SPARSE_BASE
						  , pRange->rBegin);
	    pRange->rMask = pciBusAddrToHostAddr(tag,PCI_IO_SPARSE_MASK
						, pRange->rEnd);
	    break;
	}
	break;
    }
}

static void
pciConvertListToHost(int bus, int dev, int func, resPtr list)
{
    PCITAG tag = pciTag(bus,dev,func);
    while (list) {
	pciTagConvertRange2Host(tag, &list->val);
	list = list->next;
    }
}


void
pciConvertRange2Host(int entityIndex, resRange *pRange)
{
    PCITAG tag;
    pciVideoPtr pvp;

    pvp = xf86GetPciInfoForEntity(entityIndex);
    if (!pvp) return;
    tag = TAG(pvp);
    pciTagConvertRange2Host(tag, pRange);
}

