/*
 * Copyright 1992-2000 by Alan Hourihane <alanh@fairlite.demon.co.uk>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Alan Hourihane not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Alan Hourihane makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * ALAN HOURIHANE DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL ALAN HOURIHANE BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 * Authors:  Alan Hourihane, <alanh@fairlite.demon.co.uk>
 */
/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/trident/trident.h,v 1.33 2000/12/08 09:05:16 alanh Exp $ */

#ifndef _TRIDENT_H_
#define _TRIDENT_H_

#include "xf86Cursor.h"
#include "xaa.h"
#include "xf86RamDac.h"
#include "compiler.h"
#include "vgaHW.h"
#include "xf86i2c.h"
#include "xf86int10.h"
#include "shadowfb.h"
#include "xf86xv.h"

typedef struct {
	unsigned char tridentRegs3x4[0x100];
	unsigned char tridentRegs3CE[0x100];
	unsigned char tridentRegs3C4[0x100];
	unsigned char tridentRegsDAC[0x01];
        unsigned char tridentRegsClock[0x03];
	unsigned char DacRegs[0x300];
} TRIDENTRegRec, *TRIDENTRegPtr;

#define TRIDENTPTR(p)	((TRIDENTPtr)((p)->driverPrivate))

typedef struct {
    ScrnInfoPtr		pScrn;
    pciVideoPtr		PciInfo;
    PCITAG		PciTag;
    EntityInfoPtr	pEnt;
    int			Chipset;
    int			DACtype;
    int			RamDac;
    int                 ChipRev;
    int			HwBpp;
    int			BppShift;
    CARD32		IOAddress;
    unsigned long	FbAddress;
    unsigned char *     IOBase;
#ifdef __alpha__
    unsigned char *     IOBaseDense;
#endif
    unsigned char *	FbBase;
    long		FbMapSize;
    Bool		NoAccel;
    Bool		HWCursor;
    Bool		UsePCIRetry;
    Bool		UsePCIBurst;
    Bool		NewClockCode;
    Bool		Clipping;
    Bool		DstEnable;
    Bool		ROP;
    Bool		HasSGRAM;
    Bool		MUX;
    Bool		IsCyber;
    Bool		CyberShadow;
    Bool		NoMMIO;
    Bool                MMIOonly;
    Bool		ShadowFB;
    Bool		Linear;
    DGAModePtr		DGAModes;
    int			numDGAModes;
    Bool		DGAactive;
    int			DGAViewportStatus;
    unsigned char *	ShadowPtr;
    int			ShadowPitch;
    RefreshAreaFuncPtr  RefreshArea;
    void	        (*PointerMoved)(int index, int x, int y);
    int                 Rotate;
    float		frequency;
    unsigned char	REGPCIReg;
    unsigned char	REGNewMode1;
    CARD8		SaveClock1;
    CARD8		SaveClock2;
    CARD8		SaveClock3;
    int			MinClock;
    int			MaxClock;
    int			MUXThreshold;
    int                 currentClock;
    int			MCLK;
    int			dwords;
    int			h;
    int			x;
    int			w;
    int			y;
    int                 lcdMode;
#ifdef READOUT
    Bool                DontSetClock;
#endif
    TRIDENTRegRec	SavedReg;
    TRIDENTRegRec	ModeReg;
    I2CBusPtr		DDC;
    CARD16		EngineOperation;
    CARD32		PatternLocation;
    CARD32		BltScanDirection;
    CARD32		DrawFlag;
    CARD16		LinePattern;
    RamDacRecPtr	RamDacRec;
    xf86CursorInfoPtr	CursorInfoRec;
    xf86Int10InfoPtr	Int10;
    XAAInfoRecPtr	AccelInfoRec;
    CloseScreenProcPtr	CloseScreen;
    ScreenBlockHandlerProcPtr BlockHandler;
    int                 panelWidth;
    int                 panelHeight;
    unsigned int	(*ddc1Read)(ScrnInfoPtr);
    CARD8*		XAAScanlineColorExpandBuffers[2];
    CARD8*		XAAImageScanlineBuffer[1];
    void                (*InitializeAccelerator)(ScrnInfoPtr);
#ifdef XvExtension
    void		(*VideoTimerCallback)(ScrnInfoPtr, Time);
    XF86VideoAdaptorPtr adaptor;
    int                 videoKey;
    int			hsync;
    int			vsync;
#endif
} TRIDENTRec, *TRIDENTPtr;

typedef struct {
    CARD8 mode;
    char* display;
    int clock;
    int shadow_0;
    int shadow_3;
    int shadow_4;
    int shadow_5;
    int shadow_6;
    int shadow_7;
    int shadow_10;
    int shadow_11;
    int shadow_16;
    int shadow_HiOrd;
} tridentLCD;

extern tridentLCD LCD[];

typedef struct {
    int x_res;
    int y_res;
    int mode;
} biosMode;

/* Prototypes */

Bool TRIDENTClockSelect(ScrnInfoPtr pScrn, int no);
Bool TRIDENTSwitchMode(int scrnIndex, DisplayModePtr mode, int flags);
void TRIDENTAdjustFrame(int scrnIndex, int x, int y, int flags);
Bool TRIDENTDGAInit(ScreenPtr pScreen);

unsigned int Tridentddc1Read(ScrnInfoPtr pScrn);
void TVGARestore(ScrnInfoPtr pScrn, TRIDENTRegPtr tridentReg);
void TVGASave(ScrnInfoPtr pScrn, TRIDENTRegPtr tridentReg);
Bool TVGAInit(ScrnInfoPtr pScrn, DisplayModePtr mode);
void TridentRestore(ScrnInfoPtr pScrn, TRIDENTRegPtr tridentReg);
void TridentSave(ScrnInfoPtr pScrn, TRIDENTRegPtr tridentReg);
Bool TridentInit(ScrnInfoPtr pScrn, DisplayModePtr mode);
Bool TridentAccelInit(ScreenPtr pScreen);
Bool ImageAccelInit(ScreenPtr pScreen);
Bool BladeAccelInit(ScreenPtr pScreen);
Bool TridentHWCursorInit(ScreenPtr pScreen);
int TridentFindMode(int xres, int yres, int depth);
void TGUISetClock(ScrnInfoPtr pScrn, int clock, unsigned char *a, unsigned char *b);
void TGUISetMCLK(ScrnInfoPtr pScrn, int clock, unsigned char *a, unsigned char *b);
void TridentOutIndReg(ScrnInfoPtr pScrn,
		     CARD32 reg, unsigned char mask, unsigned char data);
unsigned char TridentInIndReg(ScrnInfoPtr pScrn, CARD32 reg);
void TridentWriteAddress(ScrnInfoPtr pScrn, CARD32 index);
void TridentReadAddress(ScrnInfoPtr pScrn, CARD32 index);
void TridentWriteData(ScrnInfoPtr pScrn, unsigned char data);
unsigned char TridentReadData(ScrnInfoPtr pScrn);
void TridentLoadPalette(ScrnInfoPtr pScrn, int numColors, int *indicies, LOCO *colors, VisualPtr pVisual);
void TridentSetOverscan(ScrnInfoPtr pScrn, int overscan);
int TGUISetRead(ScreenPtr pScreen, int bank);
int TGUISetWrite(ScreenPtr pScreen, int bank);
int TGUISetReadWrite(ScreenPtr pScreen, int bank);
int TVGA8900SetRead(ScreenPtr pScreen, int bank);
int TVGA8900SetWrite(ScreenPtr pScreen, int bank);
int TVGA8900SetReadWrite(ScreenPtr pScreen, int bank);
void TridentFindClock(ScrnInfoPtr pScrn, int clock);
float CalculateMCLK(ScrnInfoPtr pScrn);
void TRIDENTRefreshArea(ScrnInfoPtr pScrn, int num, BoxPtr pbox);
void TRIDENTShadowUpdate (ScreenPtr pScreen, PixmapPtr pShadow, 
			  RegionPtr damage);
void TRIDENTPointerMoved(int index, int x, int y);
void TRIDENTRefreshArea8(ScrnInfoPtr pScrn, int num, BoxPtr pbox);
void TRIDENTRefreshArea16(ScrnInfoPtr pScrn, int num, BoxPtr pbox);
void TRIDENTRefreshArea24(ScrnInfoPtr pScrn, int num, BoxPtr pbox);
void TRIDENTRefreshArea32(ScrnInfoPtr pScrn, int num, BoxPtr pbox);

/*
 * Trident Chipset Definitions
 */

/* Supported chipsets */
typedef enum {
    TVGA8200LX,
    TVGA8800BR,
    TVGA8800CS,
    TVGA8900B,
    TVGA8900C,
    TVGA8900CL,
    TVGA8900D,
    TVGA9000,
    TVGA9000i,
    TVGA9100B,
    TVGA9200CXr,
    TGUI9400CXi,
    TGUI9420DGi,
    TGUI9430DGi,
    TGUI9440AGi,
    CYBER9320,
    TGUI9660,
    TGUI9680,
    PROVIDIA9682,
    PROVIDIA9685,
    CYBER9382,
    CYBER9385,
    CYBER9388,
    CYBER9397,
    CYBER9397DVD,
    CYBER9520,
    CYBER9525DVD,
    IMAGE975,
    IMAGE985,
    BLADE3D,
    CYBERBLADEI7,
    CYBERBLADEI7D,
    CYBERBLADEI1,
    CYBERBLADEI1D,
    CYBERBLADEAI1,
    CYBERBLADEAI1D,
    CYBERBLADEE4
} TRIDENTType;

#define UseMMIO		(pTrident->NoMMIO == FALSE)

#define IsPciCard	(pTrident->pEnt->location.type == BUS_PCI)

#define IsPrimaryCard	((xf86IsPrimaryPci(pTrident->PciInfo)) || \
			 (xf86IsPrimaryIsa()))

#define HAS_DST_TRANS	((pTrident->Chipset == PROVIDIA9682) || \
			 (pTrident->Chipset == PROVIDIA9685))

#define Is3Dchip	((pTrident->Chipset == CYBER9388) || \
			 (pTrident->Chipset == CYBER9397) || \
			 (pTrident->Chipset == CYBER9397DVD) || \
			 (pTrident->Chipset == CYBER9520) || \
			 (pTrident->Chipset == CYBER9525DVD) || \
			 (pTrident->Chipset == CYBERBLADEE4)  || \
			 (pTrident->Chipset == IMAGE975)  || \
			 (pTrident->Chipset == IMAGE985)  || \
			 (pTrident->Chipset == CYBERBLADEI7)  || \
			 (pTrident->Chipset == CYBERBLADEI7D)  || \
			 (pTrident->Chipset == CYBERBLADEI1)  || \
			 (pTrident->Chipset == CYBERBLADEI1D)  || \
			 (pTrident->Chipset == CYBERBLADEAI1)  || \
			 (pTrident->Chipset == CYBERBLADEAI1D)  || \
			 (pTrident->Chipset == BLADE3D))

/*
 * Trident DAC's
 */

#define TKD8001		0
#define TGUIDAC		1

#endif /* _TRIDENT_H_ */

