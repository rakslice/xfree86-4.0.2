/*
 * Copyright 1990, 1991 by Thomas Roell, Dinkelscherben, Germany
 * Copyright 1992 by David Dawes <dawes@XFree86.org>
 * Copyright 1992 by Jim Tsillas <jtsilla@damon.ccs.northeastern.edu>
 * Copyright 1992 by Rich Murphey <Rich@Rice.edu>
 * Copyright 1992 by Robert Baron <Robert.Baron@ernst.mach.cs.cmu.edu>
 * Copyright 1992 by Orest Zborowski <obz@eskimo.com>
 * Copyright 1993 by Vrije Universiteit, The Netherlands
 * Copyright 1993 by David Wexelblat <dwex@XFree86.org>
 * Copyright 1994, 1996 by Holger Veit <Holger.Veit@gmd.de>
 * Copyright 1994-1999 by The XFree86 Project, Inc
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the names of the above listed copyright holders 
 * not be used in advertising or publicity pertaining to distribution of 
 * the software without specific, written prior permission.  The above listed
 * copyright holders make no representations about the suitability of this 
 * software for any purpose.  It is provided "as is" without express or 
 * implied warranty.
 *
 * THE ABOVE LISTED COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD 
 * TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY 
 * AND FITNESS, IN NO EVENT SHALL THE ABOVE LISTED COPYRIGHT HOLDERS BE 
 * LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY 
 * DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER 
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING 
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */

/*
 * The ARM32 code here carries the following copyright:
 *
 * Copyright 1997
 * Digital Equipment Corporation. All rights reserved.
 * This software is furnished under license and may be used and copied only in 
 * accordance with the following terms and conditions.  Subject to these
 * conditions, you may download, copy, install, use, modify and distribute
 * this software in source and/or binary form. No title or ownership is
 * transferred hereby.
 *
 * 1) Any source code used, modified or distributed must reproduce and retain
 *    this copyright notice and list of conditions as they appear in the
 *    source file.
 *
 * 2) No right is granted to use any trade name, trademark, or logo of Digital 
 *    Equipment Corporation. Neither the "Digital Equipment Corporation"
 *    name nor any trademark or logo of Digital Equipment Corporation may be
 *    used to endorse or promote products derived from this software without
 *    the prior written permission of Digital Equipment Corporation.
 *
 * 3) This software is provided "AS-IS" and any express or implied warranties,
 *    including but not limited to, any implied warranties of merchantability,
 *    fitness for a particular purpose, or non-infringement are disclaimed.
 *    In no event shall DIGITAL be liable for any damages whatsoever, and in
 *    particular, DIGITAL shall not be liable for special, indirect,
 *    consequential, or incidental damages or damages for lost profits, loss
 *    of revenue or loss of use, whether such damages arise in contract, 
 *    negligence, tort, under statute, in equity, at law or otherwise, even
 *    if advised of the possibility of such damage. 
 *
 */

/* $XFree86: xc/programs/Xserver/hw/xfree86/os-support/xf86_OSproc.h,v 3.49 2000/12/07 15:43:45 tsi Exp $ */

#ifndef _XF86_OSPROC_H
#define _XF86_OSPROC_H

#ifdef XF86_OS_PRIVS
#include "xf86Pci.h"
#endif

/*
 * The actual prototypes have been pulled into this seperate file so
 * that they can can be used without pulling in all of the OS specific
 * stuff like sys/stat.h, etc. This casues problem for loadable modules.
 */ 

/*
 * Flags for xf86MapVidMem().  Multiple flags can be or'd together.  The
 * flags may be used as hints.  For example it would be permissible to
 * enable write combining for memory marked only for framebuffer use.
 */

#define VIDMEM_FRAMEBUFFER	0x01	/* memory for framebuffer use */
#define VIDMEM_MMIO		0x02	/* memory for I/O use */
#define VIDMEM_MMIO_32BIT	0x04	/* memory accesses >= 32bit */
#define VIDMEM_READSIDEEFFECT	0x08	/* reads can have side-effects */
#define VIDMEM_SPARSE		0x10	/* sparse mapping required
					 * assumed when VIDMEM_MMIO is
					 * set. May be used with
					 * VIDMEM_FRAMEBUFFER) */

/*
 * OS-independent modem state flags for xf86SetSerialModemState() and
 * xf86GetSerialModemState().
 */
#define XF86_M_LE		0x001	/* line enable */
#define XF86_M_DTR		0x002	/* data terminal ready */
#define XF86_M_RTS		0x004	/* request to send */
#define XF86_M_ST		0x008	/* secondary transmit */
#define XF86_M_SR		0x010	/* secondary receive */
#define XF86_M_CTS		0x020	/* clear to send */
#define XF86_M_CAR		0x040	/* carrier detect */
#define XF86_M_RNG		0x080	/* ring */
#define XF86_M_DSR		0x100	/* data set ready */

#ifdef XF86_OS_PRIVS
extern void xf86WrapperInit(void);
#endif

#ifndef NO_OSLIB_PROTOTYPES
/*
 * This is to prevent re-entrancy to FatalError() when aborting.
 * Anything that can be called as a result of AbortDDX() should use this
 * instead of FatalError().
 */

#define xf86FatalError(a, b) \
	if (dispatchException & DE_TERMINATE) { \
		ErrorF(a, b); \
		return; \
	} else FatalError(a, b)

/***************************************************************************/
/* Prototypes                                                              */
/***************************************************************************/

#include <X11/Xfuncproto.h>
#include "opaque.h"

_XFUNCPROTOBEGIN

/* public functions */
extern Bool xf86LinearVidMem(void);
extern Bool xf86CheckMTRR(int); 
extern pointer xf86MapVidMem(int, int, unsigned long, unsigned long);
extern void xf86UnMapVidMem(int, pointer, unsigned long);
extern void xf86MapReadSideEffects(int, int, pointer, unsigned long);
extern int xf86ReadBIOS(unsigned long, unsigned long, unsigned char *, int);
extern void xf86EnableIO(void);
extern void xf86DisableIO(void);
extern Bool xf86DisableInterrupts(void);
extern void xf86EnableInterrupts(void);
extern void xf86SetTVOut(int);
extern void xf86SetRGBOut(void);
extern void xf86SoundKbdBell(int, int, int);
#if defined(QNX4)
#pragma aux xf86BusToMem modify [eax ebx ecx edx esi edi];
#pragma aux xf86MemToBus modify [eax ebx ecx edx esi edi];
#endif
extern void xf86BusToMem(unsigned char *, unsigned char *, int);
extern void xf86MemToBus(unsigned char *, unsigned char *, int);
extern void xf86IODelay(void);
extern void xf86UDelay(long usec);
extern void xf86SlowBcopy(unsigned char *, unsigned char *, int);
extern int xf86OpenSerial(pointer options);
extern int xf86SetSerial(int fd, pointer options);
extern int xf86SetSerialSpeed(int fd, int speed);
extern int xf86ReadSerial(int fd, void *buf, int count);
extern int xf86WriteSerial(int fd, const void *buf, int count);
extern int xf86CloseSerial(int fd);
extern int xf86FlushInput(int fd);
extern int xf86WaitForInput(int fd, int timeout);
extern int xf86SerialSendBreak(int fd, int duration);
extern int xf86SetSerialModemState(int fd, int state);
extern int xf86GetSerialModemState(int fd);
extern int xf86SerialModemSetBits(int fd, int bits);
extern int xf86SerialModemClearBits(int fd, int bits);
extern int xf86LoadKernelModule(const char *pathname);

/* AGP GART interface */

typedef struct _AgpInfo {
	CARD32		bridgeId;
	CARD32		agpMode;
	unsigned long	base;
	unsigned long	size;
	unsigned long	totalPages;
	unsigned long	systemPages;
	unsigned long	usedPages;
} AgpInfo, *AgpInfoPtr;

extern Bool xf86AgpGARTSupported(void);
extern AgpInfoPtr xf86GetAGPInfo(int screenNum);
extern Bool xf86AcquireGART(int screenNum);
extern Bool xf86ReleaseGART(int screenNum);
extern int xf86AllocateGARTMemory(int screenNum, unsigned long size, int type,
				  unsigned long *physical);
extern Bool xf86BindGARTMemory(int screenNum, int key, unsigned long offset);
extern Bool xf86UnbindGARTMemory(int screenNum, int key);
extern Bool xf86EnableAGP(int screenNum, CARD32 mode);

#if defined(__alpha__)
extern void xf86JensenMemToBus(char *, long, long, int);
extern void xf86JensenBusToMem(char *, char *, unsigned long, int);
extern void xf86SlowBCopyFromBus(unsigned char *, unsigned char *, int);
extern void xf86SlowBCopyToBus(unsigned char *, unsigned char *, int);
#endif /* __alpha__ */

/* These routines are in shared/sigio.c and are not loaded as part of the
   module.  These routines are small, and the code if very POSIX-signal (or
   OS-signal) specific, so it seemed better to provide more complex
   wrappers than to wrap each individual function called. */
extern int xf86InstallSIGIOHandler(int fd, void (*f)(int, void *), void *);
extern int xf86RemoveSIGIOHandler(int fd);
extern int xf86BlockSIGIO (void);
extern void xf86UnblockSIGIO (int);
#ifdef XFree86Server
extern void xf86AssertBlockedSIGIO (char *);
#endif
extern Bool xf86SIGIOSupported (void);

#ifdef XF86_OS_PRIVS
typedef void (*PMClose)(void);
extern void xf86OpenConsole(void);
extern void xf86CloseConsole(void);
extern Bool xf86VTSwitchPending(void);
extern Bool xf86VTSwitchAway(void);
extern Bool xf86VTSwitchTo(void);
extern void xf86VTRequest(int sig);
extern int xf86ProcessArgument(int, char **, int);
extern void xf86UseMsg(void);
extern void xf86SetKbdLeds(int);
extern int xf86GetKbdLeds(void);
extern void xf86SetKbdRepeat(char);
extern void xf86KbdInit(void);
extern int xf86KbdOn(void);
extern int xf86KbdOff(void);
extern void xf86KbdEvents(void);
#ifdef XQUEUE
extern int  xf86XqueKbdProc(DeviceIntPtr, int);
extern void xf86XqueEvents(void);
#endif
extern PMClose xf86OSPMOpen(void);

#ifdef NEED_OS_RAC_PROTOS
/* RAC-related privs */
/* internal to os-support layer */
resPtr xf86StdBusAccWindowsFromOS(void);
resPtr xf86StdPciAccWindowsFromOS(void);
resPtr xf86StdIsaAccWindowsFromOS(void);
resPtr xf86StdAccResFromOS(resPtr ret);

/* available to the common layer */
resPtr xf86BusAccWindowsFromOS(void);
resPtr xf86PciBusAccWindowsFromOS(void);
resPtr xf86IsaBusAccWindowsFromOS(void);
resPtr xf86AccResFromOS(resPtr ret);
#endif /* NEED_OS_RAC_PROTOS */

Bool xf86GetPciSizeFromOS(PCITAG tag, int index, int* bits);

#endif /* XF86_OS_PRIVS */


_XFUNCPROTOEND
#endif /* NO_OSLIB_PROTOTYPES */

#endif /* _XF86_OSPROC_H */
