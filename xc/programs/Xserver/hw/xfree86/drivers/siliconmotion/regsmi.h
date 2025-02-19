/* Header:   //Mercury/Projects/archives/XFree86/4.0/regsmi.h-arc   1.11   14 Sep 2000 11:17:30   Frido  $ */

/*
Copyright (C) 1994-1999 The XFree86 Project, Inc.  All Rights Reserved.
Copyright (C) 2000 Silicon Motion, Inc.  All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FIT-
NESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
XFREE86 PROJECT BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the names of the XFree86 Project and
Silicon Motion shall not be used in advertising or otherwise to promote the
sale, use or other dealings in this Software without prior written
authorization from the XFree86 Project and SIlicon Motion.
*/
/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/siliconmotion/regsmi.h,v 1.1 2000/11/28 20:59:19 dawes Exp $ */

#ifndef _REGSMI_H
#define _REGSMI_H

#define SMI_LYNX_SERIES(chip)	((chip & 0xF0F0) == 0x0010)
#define SMI_LYNX3D_SERIES(chip)	((chip & 0xF0F0) == 0x0020)
#define SMI_LYNXEM_SERIES(chip) ((chip & 0xFFF0) == 0x0710)
#define SMI_LYNXM_SERIES(chip)	((chip & 0xFF00) == 0x0700)

/* Chip tags */
#define PCI_SMI_VENDOR_ID	PCI_VENDOR_SMI
#define SMI_UNKNOWN			0
#define SMI_LYNX			PCI_CHIP_SMI910
#define SMI_LYNXE			PCI_CHIP_SMI810
#define SMI_LYNX3D			PCI_CHIP_SMI820
#define SMI_LYNXEM			PCI_CHIP_SMI710
#define SMI_LYNXEMplus		PCI_CHIP_SMI712
#define SMI_LYNX3DM			PCI_CHIP_SMI720

/* I/O Functions */
static __inline__ CARD8
VGAIN8_INDEX(SMIPtr pSmi, int indexPort, int dataPort, CARD8 index)
{
	if (pSmi->IOBase)
	{
		MMIO_OUT8(pSmi->IOBase, indexPort, index);
		return(MMIO_IN8(pSmi->IOBase, dataPort));
	}
	else
	{
		outb(indexPort, index);
		return(inb(dataPort));
	}
}

static __inline__ void
VGAOUT8_INDEX(SMIPtr pSmi, int indexPort, int dataPort, CARD8 index, CARD8 data)
{
	if (pSmi->IOBase)
	{
		MMIO_OUT8(pSmi->IOBase, indexPort, index);
		MMIO_OUT8(pSmi->IOBase, dataPort, data);
	}
	else
	{
		outb(indexPort, index);
		outb(dataPort, data);
	}
}

static __inline__ CARD8
VGAIN8(SMIPtr pSmi, int port)
{
	if (pSmi->IOBase)
	{
		return(MMIO_IN8(pSmi->IOBase, port));
	}
	else
	{
		return(inb(port));
	}
}

static __inline__ void
VGAOUT8(SMIPtr pSmi, int port, CARD8 data)
{
	if (pSmi->IOBase)
	{
		MMIO_OUT8(pSmi->IOBase, port, data);
	}
	else
	{
		outb(port, data);
	}
}

#define OUT_SEQ(pSmi, index, data)	\
		VGAOUT8_INDEX((pSmi), VGA_SEQ_INDEX, VGA_SEQ_DATA, (index), (data))
#define IN_SEQ(pSmi, index)			\
		VGAIN8_INDEX((pSmi), VGA_SEQ_INDEX, VGA_SEQ_DATA, (index))

#define WRITE_DPR(pSmi, dpr, data)	MMIO_OUT32(pSmi->DPRBase, dpr, data); DEBUG((VERBLEV, "DPR%02X = %08X\n", dpr, data))
#define READ_DPR(pSmi, dpr)			MMIO_IN32(pSmi->DPRBase, dpr)
#define WRITE_VPR(pSmi, vpr, data)	MMIO_OUT32(pSmi->VPRBase, vpr, data); DEBUG((VERBLEV, "VPR%02X = %08X\n", vpr, data))
#define READ_VPR(pSmi, vpr)			MMIO_IN32(pSmi->VPRBase, vpr)
#define WRITE_CPR(pSmi, cpr, data)	MMIO_OUT32(pSmi->CPRBase, cpr, data); DEBUG((VERBLEV, "CPR%02X = %08X\n", cpr, data))
#define READ_CPR(pSmi, cpr)			MMIO_IN32(pSmi->CPRBase, cpr)

/* 2D Engine commands */
#define SMI_TRANSPARENT_SRC		0x00000100
#define SMI_TRANSPARENT_DEST	0x00000300

#define SMI_OPAQUE_PXL			0x00000000
#define SMI_TRANSPARENT_PXL		0x00000400

#define SMI_MONO_PACK_8			0x00001000
#define SMI_MONO_PACK_16		0x00002000
#define SMI_MONO_PACK_32		0x00003000

#define SMI_ROP2_SRC			0x00008000
#define SMI_ROP2_PAT			0x0000C000
#define SMI_ROP3				0x00000000

#define SMI_BITBLT				0x00000000
#define SMI_RECT_FILL			0x00010000
#define SMI_TRAPEZOID_FILL		0x00030000
#define SMI_SHORT_STROKE    	0x00060000
#define SMI_BRESENHAM_LINE		0x00070000
#define SMI_HOSTBLT_WRITE		0x00080000
#define SMI_HOSTBLT_READ		0x00090000
#define SMI_ROTATE_BLT			0x000B0000

#define SMI_SRC_COLOR			0x00000000
#define SMI_SRC_MONOCHROME		0x00400000

#define SMI_GRAPHICS_STRETCH	0x00800000

#define SMI_ROTATE_CW			0x01000000
#define SMI_ROTATE_CCW			0x02000000

#define SMI_MAJOR_X				0x00000000
#define SMI_MAJOR_Y				0x04000000

#define SMI_LEFT_TO_RIGHT		0x00000000
#define SMI_RIGHT_TO_LEFT		0x08000000

#define SMI_COLOR_PATTERN		0x40000000
#define SMI_MONO_PATTERN		0x00000000

#define SMI_QUICK_START			0x10000000
#define SMI_START_ENGINE		0x80000000

#define MAXLOOP 0x100000	/* timeout value for engine waits */

#define ENGINE_IDLE()														   \
		((VGAIN8_INDEX(pSmi, VGA_SEQ_INDEX, VGA_SEQ_DATA, 0x16) & 0x08) == 0)
#define FIFO_EMPTY()														   \
		((VGAIN8_INDEX(pSmi, VGA_SEQ_INDEX, VGA_SEQ_DATA, 0x16) & 0x10) != 0)

/* Wait until "v" queue entries are free */
#define	WaitQueue(v)														   \
	do																		   \
	{																		   \
		if (pSmi->NoPCIRetry)												   \
		{																	   \
			int loop = MAXLOOP; mem_barrier();								   \
			while (!FIFO_EMPTY())											   \
				if (loop-- == 0) break;										   \
			if (loop <= 0) SMI_GEReset(pScrn, 1, __LINE__, __FILE__);		   \
		}																	   \
	} while (0)

/* Wait until GP is idle */
#define WaitIdle()															   \
	do																		   \
	{																		   \
		int loop = MAXLOOP; mem_barrier();									   \
		while (!ENGINE_IDLE())												   \
			if (loop-- == 0) break;											   \
		if (loop <= 0) SMI_GEReset(pScrn, 1, __LINE__, __FILE__);			   \
	}																		   \
	while (0)

/* Wait until GP is idle and queue is empty */
#define	WaitIdleEmpty()														   \
	do																		   \
	{																		   \
		WaitQueue(MAXFIFO);													   \
		WaitIdle();															   \
	}																		   \
	while (0)

#define RGB8_PSEUDO      (-1)
#define RGB16_565         0
#define RGB16_555         1
#define RGB32_888         2

#endif /* _REGSMI_H */
