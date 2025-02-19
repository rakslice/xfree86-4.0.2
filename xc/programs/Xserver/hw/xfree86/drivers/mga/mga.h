/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/mga/mga.h,v 1.70 2000/12/06 15:35:19 eich Exp $ */
/*
 * MGA Millennium (MGA2064W) functions
 *
 * Copyright 1996 The XFree86 Project, Inc.
 *
 * Authors
 *		Dirk Hohndel
 *			hohndel@XFree86.Org
 *		David Dawes
 *			dawes@XFree86.Org
 */

#ifndef MGA_H
#define MGA_H

#include "compiler.h"
#include "xaa.h"
#include "xf86Cursor.h"
#include "vgaHW.h"
#include "colormapst.h"
#include "xf86DDC.h"
#include "xf86xv.h"

#ifdef XF86DRI
#include "xf86drm.h"
#include "sarea.h"
#define _XF86DRI_SERVER_
#include "xf86dri.h"
#include "dri.h"
#include "GL/glxint.h"
#include "mga_dri.h"
#endif

#ifdef USEMGAHAL
#include "client.h"
#endif
#include "mga_bios.h"

#if !defined(EXTRADEBUG)
#define INREG8(addr) MMIO_IN8(pMga->IOBase, addr)
#define INREG16(addr) MMIO_IN16(pMga->IOBase, addr)
#define INREG(addr) MMIO_IN32(pMga->IOBase, addr)
#define OUTREG8(addr, val) MMIO_OUT8(pMga->IOBase, addr, val)
#define OUTREG16(addr, val) MMIO_OUT16(pMga->IOBase, addr, val)
#define OUTREG(addr, val) MMIO_OUT32(pMga->IOBase, addr, val)
#else /* !EXTRADEBUG */
CARD8 dbg_inreg8(ScrnInfoPtr,int,int);
CARD16 dbg_inreg16(ScrnInfoPtr,int,int);
CARD32 dbg_inreg32(ScrnInfoPtr,int,int);
void dbg_outreg8(ScrnInfoPtr,int,int);
void dbg_outreg16(ScrnInfoPtr,int,int);
void dbg_outreg32(ScrnInfoPtr,int,int);
#define INREG8(addr) dbg_inreg8(pScrn,addr,1)
#define INREG16(addr) dbg_inreg16(pScrn,addr,1)
#define INREG(addr) dbg_inreg32(pScrn,addr,1)
#define OUTREG8(addr,val) dbg_outreg8(pScrn,addr,val)
#define OUTREG16(addr,val) dbg_outreg16(pScrn,addr,val)
#define OUTREG(addr,val) dbg_outreg32(pScrn,addr,val)
#endif /* EXTRADEBUG */

#define PORT_OFFSET 	(0x1F00 - 0x300)

#define MGA_VERSION 4000
#define MGA_NAME "MGA"
#define MGA_C_NAME MGA
#define MGA_MODULE_DATA mgaModuleData
#define MGA_DRIVER_NAME "mga"
#define MGA_MAJOR_VERSION 1
#define MGA_MINOR_VERSION 0
#define MGA_PATCHLEVEL 0

typedef struct {
    unsigned char	ExtVga[6];
    unsigned char 	DacClk[6];
    unsigned char *     DacRegs;
    CARD32		Option;
    CARD32		Option2;
    CARD32		Option3;
} MGARegRec, *MGARegPtr;

typedef struct {
   int          brightness;
   int          contrast;
   FBLinearPtr	linear;
   RegionRec	clip;
   CARD32	colorKey;
   CARD32	videoStatus;
   Time		offTime;
   Time		freeTime;
   int		lastPort;
} MGAPortPrivRec, *MGAPortPrivPtr;

typedef struct {
    Bool	isHwCursor;
    int		CursorMaxWidth;
    int 	CursorMaxHeight;
    int		CursorFlags;
    int		CursorOffscreenMemSize;
    Bool	(*UseHWCursor)(ScreenPtr, CursorPtr);
    void	(*LoadCursorImage)(ScrnInfoPtr, unsigned char*);
    void	(*ShowCursor)(ScrnInfoPtr);
    void	(*HideCursor)(ScrnInfoPtr);
    void	(*SetCursorPosition)(ScrnInfoPtr, int, int);
    void	(*SetCursorColors)(ScrnInfoPtr, int, int);
    long	maxPixelClock;
    long	MemoryClock;
    MessageType ClockFrom;
    MessageType MemClkFrom;
    Bool	SetMemClk;
    void	(*LoadPalette)(ScrnInfoPtr, int, int*, LOCO*, VisualPtr);
    void	(*PreInit)(ScrnInfoPtr);
    void	(*Save)(ScrnInfoPtr, vgaRegPtr, MGARegPtr, Bool);
    void	(*Restore)(ScrnInfoPtr, vgaRegPtr, MGARegPtr, Bool);
    Bool	(*ModeInit)(ScrnInfoPtr, DisplayModePtr);
} MGARamdacRec, *MGARamdacPtr;


typedef struct {
    int bitsPerPixel;
    int depth;
    int displayWidth;
    rgb weight;
    Bool Overlay8Plus24;
    DisplayModePtr mode;
} MGAFBLayout;

/* Card-specific driver information */

typedef struct {
    Bool update;
    unsigned char red;
    unsigned char green;
    unsigned char blue;
} MGAPaletteInfo;

#define MGAPTR(p) ((MGAPtr)((p)->driverPrivate))

#ifdef DISABLE_VGA_IO
typedef struct mgaSave {
    pciVideoPtr pvp;
    Bool enable;
} MgaSave, *MgaSavePtr;
#endif

typedef struct {
    int			lastInstance;
#ifdef USEMGAHAL
    LPCLIENTDATA	pClientStruct;
    LPBOARDHANDLE	pBoard;
    LPMGAHWINFO		pMgaHwInfo;
#endif
    int			refCount;
    CARD32		masterFbAddress;
    long		masterFbMapSize;
    CARD32		slaveFbAddress;
    long		slaveFbMapSize;
    int			mastervideoRam;
    int			slavevideoRam;
    Bool		directRenderingEnabled;
    ScrnInfoPtr 	pScrn_1;
    ScrnInfoPtr 	pScrn_2;
} MGAEntRec, *MGAEntPtr;

typedef struct {
#ifdef USEMGAHAL
    LPCLIENTDATA	pClientStruct;
    LPBOARDHANDLE	pBoard;
    LPMGAMODEINFO	pMgaModeInfo;
    LPMGAHWINFO		pMgaHwInfo;
#endif
    EntityInfoPtr	pEnt;
    MGABiosInfo		Bios;
    MGABios2Info	Bios2;
    pciVideoPtr		PciInfo;
    PCITAG		PciTag;
    xf86AccessRec	Access;
    int			Chipset;
    int                 ChipRev;
    Bool		Primary;
    Bool		Interleave;
    int			HwBpp;
    int			Roundings[4];
    int			BppShifts[4];
    Bool		HasFBitBlt;
    Bool		OverclockMem;
    int			YDstOrg;
    int			DstOrg;
    int			SrcOrg;
    unsigned long	IOAddress;
    unsigned long	FbAddress;
    unsigned long	ILOADAddress;
    int			FbBaseReg;
    unsigned long	BiosAddress;
    MessageType		BiosFrom;
    unsigned char *     IOBase;
    unsigned char *     IOBaseDense;
    unsigned char *	FbBase;
    unsigned char *	ILOADBase;
    unsigned char *	FbStart;
    long		FbMapSize;
    long		FbUsableSize;
    long		FbCursorOffset;
    MGARamdacRec	Dac;
    Bool		HasSDRAM;
    Bool		NoAccel;
    Bool		SyncOnGreen;
    Bool		Dac6Bit;
    Bool		HWCursor;
    Bool		UsePCIRetry;
    Bool		ShowCache;
    Bool		Overlay8Plus24;
    Bool		ShadowFB;
    unsigned char *	ShadowPtr;
    int			ShadowPitch;
    int			MemClk;
    int			MinClock;
    int			MaxClock;
    MGARegRec		SavedReg;
    MGARegRec		ModeReg;
    int			MaxFastBlitY;
    CARD32		BltScanDirection;
    CARD32		FilledRectCMD;
    CARD32		SolidLineCMD;
    CARD32		PatternRectCMD;
    CARD32		DashCMD;
    CARD32		NiceDashCMD;
    CARD32		AccelFlags;
    CARD32		PlaneMask;
    CARD32		FgColor;
    CARD32		BgColor;
    CARD32		MAccess;
    int			FifoSize;
    int			StyleLen;
    XAAInfoRecPtr	AccelInfoRec;
    xf86CursorInfoPtr	CursorInfoRec;
    DGAModePtr		DGAModes;
    int			numDGAModes;
    Bool		DGAactive;
    int			DGAViewportStatus;
    CARD32		*Atype;
    CARD32		*AtypeNoBLK;
    void		(*PreInit)(ScrnInfoPtr pScrn);
    void		(*Save)(ScrnInfoPtr, vgaRegPtr, MGARegPtr, Bool);
    void		(*Restore)(ScrnInfoPtr, vgaRegPtr, MGARegPtr, Bool);
    Bool		(*ModeInit)(ScrnInfoPtr, DisplayModePtr);
    void		(*PointerMoved)(int index, int x, int y);
    CloseScreenProcPtr	CloseScreen;
    ScreenBlockHandlerProcPtr BlockHandler;
    unsigned int	(*ddc1Read)(ScrnInfoPtr);
    void (*DDC1SetSpeed)(ScrnInfoPtr, xf86ddcSpeed);
    Bool		(*i2cInit)(ScrnInfoPtr);
    I2CBusPtr		I2C;
    Bool		FBDev;
    int			colorKey;
    int			videoKey;
    int			fifoCount;
    int			Rotate;
    MGAFBLayout		CurrentLayout;
    Bool		DrawTransparent;
    int			MaxBlitDWORDS;
    Bool		TexturedVideo;
    MGAPortPrivPtr	portPrivate;
    int 		numXAALines;
    unsigned char	*ScratchBuffer;
    unsigned char	*ColorExpandBase;
    int			expandRows;
    int			expandDWORDs;
    int			expandRemaining;
    int			expandHeight;
    int			expandY;
#ifdef XF86DRI
    int 		agp_mode;
    Bool		ReallyUseIrqZero;
    Bool		have_quiescense;
    Bool 		directRenderingEnabled;
    DRIInfoPtr 		pDRIInfo;
    int 		drmSubFD;
    int 		numVisualConfigs;
    __GLXvisualConfig*	pVisualConfigs;
    MGAConfigPrivPtr 	pVisualConfigsPriv;
    MGARegRec		DRContextRegs;
    MGADRIServerPrivatePtr  DRIServerInfo;
    void		(*GetQuiescence)(ScrnInfoPtr pScrn);
#endif
    XF86VideoAdaptorPtr adaptor;
    Bool		SecondCrtc;
    GDevPtr		device;
    /* The hardware's real SrcOrg */
    int			realSrcOrg;
    MGAEntPtr		entityPrivate;
    void		(*SetupForSolidFill)(ScrnInfoPtr pScrn, int color,
					     int rop, unsigned int planemask);
    void		(*SubsequentSolidFillRect)(ScrnInfoPtr pScrn,
					     int x, int y, int w, int h);
    void		(*RestoreAccelState)(ScrnInfoPtr pScrn);
    int			allowedWidth;
    void		(*VideoTimerCallback)(ScrnInfoPtr, Time);
    void		(*PaletteLoadCallback)(ScrnInfoPtr);
    void		(*RenderCallback)(ScrnInfoPtr);
    Time		RenderTime;
    MGAPaletteInfo	palinfo[256];  /* G400 hardware bug workaround */
    FBLinearPtr		LinearScratch;
    Bool                softbooted;
#ifdef USEMGAHAL
    Bool                HALLoaded;
#endif
} MGARec, *MGAPtr;

#ifdef XF86DRI
extern void GlxSetVisualConfigs(int nconfigs, __GLXvisualConfig *configs,
				void **configprivs);
#endif


extern CARD32 MGAAtype[16];
extern CARD32 MGAAtypeNoBLK[16];

#define USE_RECTS_FOR_LINES	0x00000001
#define FASTBLT_BUG		0x00000002
#define CLIPPER_ON		0x00000004
#define BLK_OPAQUE_EXPANSION	0x00000008
#define TRANSC_SOLID_FILL	0x00000010
#define	NICE_DASH_PATTERN	0x00000020
#define	TWO_PASS_COLOR_EXPAND	0x00000040
#define	MGA_NO_PLANEMASK	0x00000080
#define USE_LINEAR_EXPANSION	0x00000100
#define LARGE_ADDRESSES		0x00000200

#define MGAIOMAPSIZE		0x00004000
#define MGAILOADMAPSIZE		0x00400000

#define TRANSPARENCY_KEY	255
#define KEY_COLOR		0

#define MGA_FRONT	0x1
#define MGA_BACK	0x2
#define MGA_DEPTH	0x4


/* Prototypes */

void MGAAdjustFrame(int scrnIndex, int x, int y, int flags);
Bool MGASwitchMode(int scrnIndex, DisplayModePtr mode, int flags);

void MGA2064SetupFuncs(ScrnInfoPtr pScrn);
void MGAGSetupFuncs(ScrnInfoPtr pScrn);

void MGAStormSync(ScrnInfoPtr pScrn);
void MGAStormEngineInit(ScrnInfoPtr pScrn);
Bool MGAStormAccelInit(ScreenPtr pScreen);
Bool MGAHWCursorInit(ScreenPtr pScreen);

Bool Mga8AccelInit(ScreenPtr pScreen);
Bool Mga16AccelInit(ScreenPtr pScreen);
Bool Mga24AccelInit(ScreenPtr pScreen);
Bool Mga32AccelInit(ScreenPtr pScreen);

void Mga8InitSolidFillRectFuncs(MGAPtr pMga);
void Mga16InitSolidFillRectFuncs(MGAPtr pMga);
void Mga24InitSolidFillRectFuncs(MGAPtr pMga);
void Mga32InitSolidFillRectFuncs(MGAPtr pMga);

void MGAPolyArcThinSolid(DrawablePtr, GCPtr, int, xArc*);

Bool MGADGAInit(ScreenPtr pScreen);

Bool MGADRIScreenInit(ScreenPtr pScreen);
void MGADRICloseScreen(ScreenPtr pScreen);
Bool MGADRIFinishScreenInit(ScreenPtr pScreen);
void MGASwapContext(ScreenPtr pScreen);
void MGASwapContext_shared(ScreenPtr pScreen);
Bool mgaConfigureWarp(ScrnInfoPtr pScrn);
unsigned int mgaInstallMicrocode(ScreenPtr pScreen, int agp_offset);
unsigned int mgaGetMicrocodeSize(ScreenPtr pScreen);
void MGASelectBuffer(ScrnInfoPtr pScrn, int which);
Bool MgaCleanupDma(ScrnInfoPtr pScrn);
Bool MgaInitDma(ScrnInfoPtr pScrn, int prim_size);
#ifdef XF86DRI
Bool MgaLockUpdate(ScrnInfoPtr pScrn, drmLockFlags flags);
void mgaGetQuiescence(ScrnInfoPtr pScrn);
void mgaGetQuiescence_shared(ScrnInfoPtr pScrn);
#endif
void MGARefreshArea(ScrnInfoPtr pScrn, int num, BoxPtr pbox);
void MGARefreshArea8(ScrnInfoPtr pScrn, int num, BoxPtr pbox);
void MGARefreshArea16(ScrnInfoPtr pScrn, int num, BoxPtr pbox);
void MGARefreshArea24(ScrnInfoPtr pScrn, int num, BoxPtr pbox);
void MGARefreshArea32(ScrnInfoPtr pScrn, int num, BoxPtr pbox);

void Mga8SetupForScreenToScreenCopy(ScrnInfoPtr pScrn, int xdir,
				int ydir, int rop, unsigned int planemask,
				int trans);
void Mga16SetupForScreenToScreenCopy(ScrnInfoPtr pScrn, int xdir,
				int ydir, int rop, unsigned int planemask,
				int trans);
void Mga24SetupForScreenToScreenCopy(ScrnInfoPtr pScrn, int xdir,
				int ydir, int rop, unsigned int planemask,
				int trans);
void Mga32SetupForScreenToScreenCopy(ScrnInfoPtr pScrn, int xdir,
				int ydir, int rop, unsigned int planemask,
				int trans);

void Mga8SetupForSolidFill(ScrnInfoPtr pScrn, int color, int rop,
				unsigned int planemask);
void Mga16SetupForSolidFill(ScrnInfoPtr pScrn, int color, int rop,
				unsigned int planemask);
void Mga24SetupForSolidFill(ScrnInfoPtr pScrn, int color, int rop,
				unsigned int planemask);
void Mga32SetupForSolidFill(ScrnInfoPtr pScrn, int color, int rop,
				unsigned int planemask);

void MGAPointerMoved(int index, int x, int y);

void MGAInitVideo(ScreenPtr pScreen);
void MGAResetVideo(ScrnInfoPtr pScrn); 

#endif
