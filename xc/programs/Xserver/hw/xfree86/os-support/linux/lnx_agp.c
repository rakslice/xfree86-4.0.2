/*
 * Abstraction of the AGP GART interface.
 *
 * This version is for both Linux and FreeBSD.
 *
 * Copyright � 2000 VA Linux Systems, Inc.
 */

/* $XFree86: xc/programs/Xserver/hw/xfree86/os-support/linux/lnx_agp.c,v 3.4 2000/08/28 18:12:56 dawes Exp $ */

#include "X.h"
#include "xf86.h"
#include "xf86Priv.h"
#include "xf86_OSlib.h"
#include "xf86OSpriv.h"

#if defined(linux)
#include <sys/ioctl.h>
#include <linux/agpgart.h>
#elif defined(__FreeBSD__)
#include <sys/ioctl.h>
#include <sys/agpio.h>
#endif

#ifndef AGP_DEVICE
#define AGP_DEVICE		"/dev/agpgart"
#endif
/* AGP page size is independent of the host page size. */
#ifndef AGP_PAGE_SIZE
#define AGP_PAGE_SIZE		4096
#endif
#define AGPGART_MAJOR_VERSION	0
#define AGPGART_MINOR_VERSION	99

static int gartFd = -1;
static int acquiredScreen = -1;

/*
 * Open /dev/agpgart.  Keep it open until server exit.
 */

static Bool
GARTInit()
{
	static Bool initDone = FALSE;
	struct _agp_info agpinf;

	if (initDone)
		return (gartFd != -1);

	initDone = TRUE;

	if (gartFd == -1)
		gartFd = open(AGP_DEVICE, O_RDWR, 0);
	else
		return FALSE;

	if (gartFd == -1) {
		xf86Msg(X_ERROR, "Unable to open " AGP_DEVICE " (%s)\n",
			strerror(errno));
		return FALSE;
	}

	xf86AcquireGART(-1);
	/* Check the kernel driver version. */
	if (ioctl(gartFd, AGPIOC_INFO, &agpinf) != 0) {
		xf86Msg(X_ERROR, "GARTInit: AGPIOC_INFO failed (%s)\n",
			strerror(errno));
		close(gartFd);
		gartFd = -1;
		return FALSE;
	}
	xf86ReleaseGART(-1);

#if defined(linux)
	/* Should this look for version >= rather than version == ? */
	if (agpinf.version.major != AGPGART_MAJOR_VERSION &&
	    agpinf.version.minor != AGPGART_MINOR_VERSION) {
		xf86Msg(X_ERROR,
			"Kernel agpgart driver version is not current"
			" (%d.%d vs %d.%d)\n",
			agpinf.version.major, agpinf.version.minor,
			AGPGART_MAJOR_VERSION, AGPGART_MINOR_VERSION);
		close(gartFd);
		gartFd = -1;
		return FALSE;
	}
#endif
	
	return TRUE;
}

Bool
xf86AgpGARTSupported()
{
	return GARTInit();
}

AgpInfoPtr
xf86GetAGPInfo(int screenNum)
{
	struct _agp_info agpinf;
	AgpInfoPtr info;

	if (!GARTInit())
		return NULL;


	if ((info = xcalloc(sizeof(AgpInfo), 1)) == NULL) {
		xf86DrvMsg(screenNum, X_ERROR, "Failed to allocate AgpInfo\n");
		return NULL;
	}

	if (ioctl(gartFd, AGPIOC_INFO, &agpinf) != 0) {
		xf86DrvMsg(screenNum, X_ERROR,
			   "xf86GetAGPInfo: AGPIOC_INFO failed (%s)\n",
			   strerror(errno));
		return NULL;
	}

	info->bridgeId = agpinf.bridge_id;
	info->agpMode = agpinf.agp_mode;
	info->base = agpinf.aper_base;
	info->size = agpinf.aper_size;
	info->totalPages = agpinf.pg_total;
	info->systemPages = agpinf.pg_system;
	info->usedPages = agpinf.pg_used;

	return info;
}

/*
 * XXX If multiple screens can acquire the GART, should we have a reference
 * count instead of using acquiredScreen?
 */

Bool
xf86AcquireGART(int screenNum)
{
	if (screenNum != -1 && !GARTInit())
		return FALSE;

	if (screenNum == -1 || acquiredScreen != screenNum) {
		if (ioctl(gartFd, AGPIOC_ACQUIRE, 0) != 0) {
			xf86DrvMsg(screenNum, X_WARNING,
				   "AGPIOC_ACQUIRE failed (%s)\n",
				   strerror(errno));
			return FALSE;
		}
		acquiredScreen = screenNum;
	}
	return TRUE;
}

Bool
xf86ReleaseGART(int screenNum)
{
	if (screenNum != -1 && !GARTInit())
		return FALSE;

	if (acquiredScreen == screenNum) {
		if (ioctl(gartFd, AGPIOC_RELEASE, 0) != 0) {
			xf86DrvMsg(screenNum, X_WARNING,
				   "AGPIOC_RELEASE failed (%s)\n",
				   strerror(errno));
			return FALSE;
		}
		acquiredScreen = -1;
		return TRUE;
	}
	return FALSE;
}

int
xf86AllocateGARTMemory(int screenNum, unsigned long size, int type,
			unsigned long *physical)
{
	struct _agp_allocate alloc;
	int pages;

	/*
	 * Allocates "size" bytes of GART memory (rounds up to the next
	 * page multiple) or type "type".  A handle (key) for the allocated
	 * memory is returned.  On error, the return value is -1.
	 */

	if (!GARTInit() || acquiredScreen != screenNum)
		return -1;

	pages = (size / AGP_PAGE_SIZE);
	if (size % AGP_PAGE_SIZE != 0)
		pages++;

	/* XXX check for pages == 0? */

	alloc.pg_count = pages;
	alloc.type = type;

	if (ioctl(gartFd, AGPIOC_ALLOCATE, &alloc) != 0) {
		xf86DrvMsg(screenNum, X_WARNING, "xf86AllocateGARTMemory: "
			   "allocation of %d pages failed\n\t(%s)\n", pages,
			   strerror(errno));
		return -1;
	}

	if (physical)
		*physical = alloc.physical;

	return alloc.key;
}


/* Bind GART memory with "key" at "offset" */
Bool
xf86BindGARTMemory(int screenNum, int key, unsigned long offset)
{
	struct _agp_bind bind;
	int pageOffset;

	if (!GARTInit() || acquiredScreen != screenNum)
		return FALSE;

	if (acquiredScreen != screenNum) {
		xf86DrvMsg(screenNum, X_ERROR,
			   "AGP not acquired by this screen\n");
		return FALSE;
	}

	if (offset % AGP_PAGE_SIZE != 0) {
		xf86DrvMsg(screenNum, X_WARNING, "xf86BindGARTMemory: "
			   "offset (0x%x) is not page-aligned (%d)\n",
			   offset, AGP_PAGE_SIZE);
		return FALSE;
	}
	pageOffset = offset / AGP_PAGE_SIZE;

	bind.pg_start = pageOffset;
	bind.key = key;

	if (ioctl(gartFd, AGPIOC_BIND, &bind) != 0) {
		xf86DrvMsg(screenNum, X_WARNING, "xf86BindGARTMemory: "
			   "binding of gart memory with key %d\n"
			   "\tat offset 0x%x failed (%s)\n",
			   key, offset, strerror(errno));
		return FALSE;
	}

	return TRUE;
}


/* Unbind GART memory with "key" */
Bool
xf86UnbindGARTMemory(int screenNum, int key)
{
	struct _agp_unbind unbind;

	if (!GARTInit() || acquiredScreen != screenNum)
		return FALSE;

	if (acquiredScreen != screenNum) {
		xf86DrvMsg(screenNum, X_ERROR,
			   "AGP not acquired by this screen\n");
		return FALSE;
	}

	unbind.priority = 0;
	unbind.key = key;

	if (ioctl(gartFd, AGPIOC_UNBIND, &unbind) != 0) {
		xf86DrvMsg(screenNum, X_WARNING, "xf86UnbindGARTMemory: "
			   "unbinding of gart memory with key %d "
			   "failed (%s)\n", key, strerror(errno));
		return FALSE;
	}

	return TRUE;
}


/* XXX Interface may change. */
Bool
xf86EnableAGP(int screenNum, CARD32 mode)
{
	agp_setup setup;

	if (!GARTInit() || acquiredScreen != screenNum)
		return FALSE;

	setup.agp_mode = mode;
	if (ioctl(gartFd, AGPIOC_SETUP, &setup) != 0) {
		xf86DrvMsg(screenNum, X_WARNING, "xf86EnableAGP: "
			   "AGPIOC_SETUP with mode %d failed (%s)\n",
			   mode, strerror(errno));
		return FALSE;
	}

	return TRUE;
}

