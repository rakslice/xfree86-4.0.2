/* $XConsortium: ct_driver.h /main/3 1996/10/27 11:49:29 kaleb $ */
/*
 * Modified 1996 by Egbert Eich <eich@xfree86.org>
 * Modified 1996 by David Bateman <dbateman@club-internet.fr>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of the authors not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  The authors makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * THE AUTHORS DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THE AUTHORS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/chips/ct_driver.h,v 1.30 2000/10/23 12:10:13 alanh Exp $ */


#ifndef _CT_DRIVER_H_
#define _CT_DRIVER_H_

#include "xaa.h"
#include "vbe.h"
#include "xaalocal.h"		/* XAA internals as we replace some of XAA */
#include "xf86Cursor.h"
#include "xf86i2c.h"
#include "xf86DDC.h"
#include "xf86xv.h"

/* Supported chipsets */
typedef enum {
    CHIPS_CT65520,
    CHIPS_CT65525,
    CHIPS_CT65530,
    CHIPS_CT65535,
    CHIPS_CT65540,
    CHIPS_CT65545,
    CHIPS_CT65546,
    CHIPS_CT65548,
    CHIPS_CT65550,
    CHIPS_CT65554,
    CHIPS_CT65555,
    CHIPS_CT68554,
    CHIPS_CT69000,
    CHIPS_CT69030,
    CHIPS_CT64200,
    CHIPS_CT64300
} CHIPSType;

/* Clock related */
typedef struct {
    unsigned char msr;		/* Dot Clock Related */
    unsigned char fcr;
    unsigned char xr02;
    unsigned char xr03;
    unsigned char xr33;
    unsigned char xr54;
    unsigned char fr03;
    int Clock;
    int FPClock;
} CHIPSClockReg, *CHIPSClockPtr;

typedef struct {
    unsigned int ProbedClk;
    unsigned int Max;		/* Memory Clock Related */
    unsigned int Clk;
    unsigned char M;
    unsigned char N;
    unsigned char P;
    unsigned char PSN;
    unsigned char xrCC;
    unsigned char xrCD;
    unsigned char xrCE;
} CHIPSMemClockReg, *CHIPSMemClockPtr;

#define TYPE_HW 0x01
#define TYPE_PROGRAMMABLE 0x02
#define GET_TYPE 0x0F
#define OLD_STYLE 0x10
#define NEW_STYLE 0x20
#define HiQV_STYLE 0x30
#define WINGINE_1_STYLE 0x40        /* 64300: external clock; 4 clocks    */
#define WINGINE_2_STYLE 0x50        /* 64300: internal clock; 2 hw-clocks */
#define GET_STYLE 0xF0
#define LCD_TEXT_CLK_FREQ 25000	    /* lcd textclock if TYPE_PROGRAMMABLE */
#define CRT_TEXT_CLK_FREQ 28322     /* crt textclock if TYPE_PROGRAMMABLE */
#define Fref 14318180               /* The reference clock in Hertz       */

/* The capability flags for the C&T chipsets */
#define ChipsLinearSupport	0x00000001
#define ChipsAccelSupport	0x00000002
#define ChipsFullMMIOSupport	0x00000004
#define ChipsMMIOSupport	0x00000008
#define ChipsHDepthSupport	0x00000010
#define ChipsDPMSSupport	0x00000020
#define ChipsTMEDSupport	0x00000040
#define ChipsGammaSupport	0x00000080
#define ChipsVideoSupport	0x00000100
#define ChipsDualChannelSupport	0x00000200

/* Options flags for the C&T chipsets */
#define ChipsHWCursor		0x00001000
#define ChipsShadowFB		0x00002000
#define ChipsOverlay8plus16	0x00004000
#define ChipsUseNewFB		0x00008000

/* Architecture type flags */
#define ChipsHiQV		0x00010000
#define ChipsWingine		0x00020000
#define IS_Wingine(x)		((x->Flags) & ChipsWingine)
#define IS_HiQV(x)		((x->Flags) & ChipsHiQV)

/* Acceleration flags for the C&T chipsets */
#define ChipsColorTransparency	0x0100000
#define ChipsImageReadSupport	0x0200000

/* Overlay Transparency Key */
#define TRANSPARENCY_KEY 255

/* Flag Bus Types */
#define ChipsUnknown	0
#define ChipsISA	1
#define ChipsVLB	2
#define ChipsPCI	3
#define ChipsCPUDirect	4
#define ChipsPIB	5
#define ChipsMCB	6

/* Macro's to select the 32 bit acceleration registers */
#define DR(x) cPtr->Regs32[x]	/* For CT655xx naming scheme  */
#define MR(x) cPtr->Regs32[x]	/* CT655xx MMIO naming scheme */
#define BR(x) cPtr->Regs32[x]	/* For HiQV naming scheme     */
#define MMIOmeml(x) *(CARD32 *)(cPtr->MMIOBase + (x))
#if 0
#define MMIOmemw(x) *(CARD16 *)(cPtr->MMIOBase + (x))
#endif
/* Monitor or flat panel type flags */
#define ChipsCRT	0x0010
#define ChipsLCD	0x1000
#define ChipsTFT	0x0100
#define ChipsDS		0x0200
#define ChipsDD		0x0400
#define ChipsSS		0x0800
#define IS_STN(x)	((x) & 0xE00)

/* Storage for the registers of the C&T chipsets */
typedef struct {
	unsigned char XR[0xFF];
	unsigned char CR[0x80];
	unsigned char FR[0x80];
	unsigned char MR[0x80];
	unsigned char MSS;
	unsigned char IOSS;
	CHIPSClockReg Clock;
} CHIPSRegRec, *CHIPSRegPtr;

/* Storage for the flat panel size */
typedef struct {
    int HDisplay;
    int HRetraceStart;
    int HRetraceEnd;
    int HTotal;
    int VDisplay;
    int VRetraceStart;
    int VTotal;
} CHIPSPanelSizeRec, *CHIPSPanelSizePtr;

/* Some variables needed in the XAA acceleration */
typedef struct {
    /* General variable */ 
    unsigned int CommandFlags;
    unsigned int BytesPerPixel;
    unsigned int BitsPerPixel;
    unsigned int FbOffset;
    unsigned int PitchInBytes;
    unsigned int ScratchAddress;
    /* 64k for color expansion and imagewrites */
    unsigned char * BltDataWindow;
    /* Hardware cursor address */
    unsigned int CursorAddress;
    Bool UseHWCursor;
    /* Boundaries of the pixmap cache */
    unsigned int CacheStart;
    unsigned int CacheEnd;
    /* Storage for pattern mask */
    int planemask;
    /* Storage for foreground and background color */
    int fgColor;
    int bgColor;
    /* For the 8x8 pattern fills */
    int patternyrot;
    /* For cached stipple fills */
    int SlotWidth;
    /* Variables for the 24bpp fill */
    unsigned char fgpixel;
    unsigned char bgpixel;
    unsigned char xorpixel;
    Bool fastfill;
    Bool rgb24equal;
    int fillindex;
    unsigned int width24bpp;
    unsigned int color24bpp;
    unsigned int rop24bpp;
} CHIPSACLRec, *CHIPSACLPtr;
#define CHIPSACLPTR(p)	&((CHIPSPtr)((p)->driverPrivate))->Accel

/* Storage for some register values that are messed up by suspend/resumes */
typedef struct {
    unsigned char xr02;
    unsigned char xr03;
    unsigned char xr14;
    unsigned char xr15;
    unsigned char vgaIOBaseFlag;
} CHIPSSuspendHackRec, *CHIPSSuspendHackPtr;

/* The functions to access the C&T extended registers */
typedef struct _CHIPSRec *CHIPSPtr;
typedef CARD8 (*chipsReadXRPtr)(CHIPSPtr cPtr, CARD8 index);
typedef void (*chipsWriteXRPtr)(CHIPSPtr cPtr, CARD8 index, CARD8 value);
typedef CARD8 (*chipsReadFRPtr)(CHIPSPtr cPtr, CARD8 index);
typedef void (*chipsWriteFRPtr)(CHIPSPtr cPtr, CARD8 index, CARD8 value);
typedef CARD8 (*chipsReadMRPtr)(CHIPSPtr cPtr, CARD8 index);
typedef void (*chipsWriteMRPtr)(CHIPSPtr cPtr, CARD8 index, CARD8 value);
typedef CARD8 (*chipsReadMSSPtr)(CHIPSPtr cPtr);
typedef void (*chipsWriteMSSPtr)(CHIPSPtr cPtr, CARD8 value);
typedef CARD8 (*chipsReadIOSSPtr)(CHIPSPtr cPtr);
typedef void (*chipsWriteIOSSPtr)(CHIPSPtr cPtr, CARD8 value);

/* The privates of the C&T driver */
#define CHIPSPTR(p)	((CHIPSPtr)((p)->driverPrivate))

typedef struct _CHIPSRec {
    pciVideoPtr		PciInfo;
    PCITAG		PciTag;
    int			Chipset;
    EntityInfoPtr       pEnt;
    CARD32		IOAddress;
    unsigned long	FbAddress;
    unsigned int	IOBase;
    unsigned char *	FbBase;
    unsigned char *	MMIOBase;
    unsigned char *	MMIOBaseVGA;
    long		FbMapSize;
    unsigned char *	ShadowPtr;
    int			ShadowPitch;
    int                 Rotate;
    void		(*PointerMoved)(int index, int x, int y);
    int                 FbOffset16;
    int                 FbSize16;  
    OptionInfoPtr	Options;
    CHIPSPanelSizeRec	PanelSize;
    int			FrameBufferSize;
    Bool		SyncResetIgn;
    Bool		UseMMIO;
    Bool		UseFullMMIO;
    Bool		UseDualChannel;
    int			Monitor;
    int			MinClock;
    int			MaxClock;
    CHIPSClockReg	SaveClock;		/* Storage for ClockSelect */
    CHIPSMemClockReg	MemClock;
    unsigned char	ClockType;
    unsigned char	CRTClk[4];
    unsigned char       FPClk[4];
    int                 FPclock;
    int                 FPclkInx;
    int                 CRTclkInx;
    Bool                FPClkModified;
    int			ClockMulFactor;
    int			Rounding;
    CHIPSSuspendHackRec	SuspendHack;
    CARD32		PanelType;
    CHIPSRegRec		ModeReg;
    CHIPSRegRec		SavedReg;
    unsigned int *	Regs32;
    unsigned int	Flags;
    CARD32		Bus;
    XAAInfoRecPtr	AccelInfoRec;
    xf86CursorInfoPtr	CursorInfoRec;
    CHIPSACLRec		Accel;
    unsigned int	HWCursorContents;
    Bool		HWCursorShown;
    DGAModePtr		DGAModes;
    int			numDGAModes;
    Bool		DGAactive;
    int			DGAViewportStatus;
    CloseScreenProcPtr	CloseScreen;
    ScreenBlockHandlerProcPtr BlockHandler;
    int			videoKey;
    XF86VideoAdaptorPtr	adaptor;
    int			OverlaySkewX;
    int			OverlaySkewY;
    int			VideoZoomMax;

#ifdef __arm32__
#ifdef __NetBSD__
    int			TVMode;
#endif
    int			Bank;
#endif
    unsigned char       ddc_mask;
    I2CBusPtr           I2C;
    vbeInfoPtr          pVbe;
    chipsReadXRPtr	readXR;
    chipsWriteXRPtr	writeXR;
    chipsReadFRPtr	readFR;
    chipsWriteFRPtr	writeFR;
    chipsReadMRPtr	readMR;
    chipsWriteMRPtr	writeMR;
    chipsReadMSSPtr	readMSS;
    chipsWriteMSSPtr	writeMSS;
    chipsReadIOSSPtr	readIOSS;
    chipsWriteIOSSPtr	writeIOSS;
} CHIPSRec;

typedef struct _CHIPSi2c {
  unsigned char i2cClockBit;
  unsigned char i2cDataBit;
  CHIPSPtr cPtr;
} CHIPSI2CRec, *CHIPSI2CPtr;

/* External variables */
extern int ChipsAluConv[];
extern int ChipsAluConv2[];
extern int ChipsAluConv3[];
extern unsigned int ChipsReg32[];
extern unsigned int ChipsReg32HiQV[];

/* Prototypes */

void CHIPSAdjustFrame(int scrnIndex, int x, int y, int flags);
Bool CHIPSSwitchMode(int scrnIndex, DisplayModePtr mode, int flags);

/* video */
void CHIPSInitVideo(ScreenPtr pScreen);
void CHIPSResetVideo(ScrnInfoPtr pScrn);

/* banking */
int CHIPSSetRead(ScreenPtr pScreen, int bank);
int CHIPSSetWrite(ScreenPtr pScreen, int bank);
int CHIPSSetReadWrite(ScreenPtr pScreen, int bank);
int CHIPSSetReadPlanar(ScreenPtr pScreen, int bank);
int CHIPSSetWritePlanar(ScreenPtr pScreen, int bank);
int CHIPSSetReadWritePlanar(ScreenPtr pScreen, int bank);
int CHIPSWINSetRead(ScreenPtr pScreen, int bank);
int CHIPSWINSetWrite(ScreenPtr pScreen, int bank);
int CHIPSWINSetReadWrite(ScreenPtr pScreen, int bank);
int CHIPSWINSetReadPlanar(ScreenPtr pScreen, int bank);
int CHIPSWINSetWritePlanar(ScreenPtr pScreen, int bank);
int CHIPSWINSetReadWritePlanar(ScreenPtr pScreen, int bank);
int CHIPSHiQVSetReadWrite(ScreenPtr pScreen, int bank);
int CHIPSHiQVSetReadWritePlanar(ScreenPtr pScreen, int bank);

/* acceleration */
Bool CHIPSAccelInit(ScreenPtr pScreen);
void CHIPSSync(ScrnInfoPtr pScrn);
Bool CHIPSMMIOAccelInit(ScreenPtr pScreen);
void CHIPSMMIOSync(ScrnInfoPtr pScrn);
Bool CHIPSHiQVAccelInit(ScreenPtr pScreen);
void CHIPSHiQVSync(ScrnInfoPtr pScrn);
Bool CHIPSCursorInit(ScreenPtr pScreen);

/* register access functions */
void CHIPSSetStdExtFuncs(CHIPSPtr cPtr);
void CHIPSSetMmioExtFuncs(CHIPSPtr cPtr);
void CHIPSHWSetMmioFuncs(ScrnInfoPtr pScrn, CARD8 *base, int offset);

/* ddc */
extern void chips_ddc1(ScrnInfoPtr pScrn);
extern Bool chips_i2cInit(ScrnInfoPtr pScrn);

/* dga */
Bool CHIPSDGAInit(ScreenPtr pScreen);

/* shadow fb */
void     chipsRefreshArea(ScrnInfoPtr pScrn, int num, BoxPtr pbox);
void     chipsRefreshArea8(ScrnInfoPtr pScrn, int num, BoxPtr pbox);
void     chipsRefreshArea16(ScrnInfoPtr pScrn, int num, BoxPtr pbox);
void     chipsRefreshArea24(ScrnInfoPtr pScrn, int num, BoxPtr pbox);
void     chipsRefreshArea32(ScrnInfoPtr pScrn, int num, BoxPtr pbox);
void     chipsPointerMoved(int index, int x, int y);


/* To aid debugging of 32 bit register access we make the following defines */
/*
#define DEBUG
#define CT_HW_DEBUG 
*/
#if defined(DEBUG) & defined(CT_HW_DEBUG)
#define HW_DEBUG(x) {usleep(500000); ErrorF("Register/Address: 0x%X\n",x);}
#else
#define HW_DEBUG(x) 
#endif
#endif
