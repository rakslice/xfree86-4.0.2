/* $XFree86: xc/programs/Xserver/hw/xfree86/loader/xf86sym.c,v 1.178 2000/12/08 22:31:52 dawes Exp $ */

/*
 *
 * Copyright 1995,96 by Metro Link, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Metro Link, Inc. not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Metro Link, Inc. makes no
 * representations about the suitability of this software for any purpose.
 *  It is provided "as is" without express or implied warranty.
 *
 * METRO LINK, INC. DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL METRO LINK, INC. BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
#include <fcntl.h>
#include "sym.h"
#include "misc.h"
#include "mi.h"
#include "cursor.h"
#include "mipointer.h"
#include "loaderProcs.h"
#include "xf86Pci.h"
#include "xf86.h"
#include "xf86Resources.h"
#include "xf86_OSproc.h"
#define DECLARE_CARD_DATASTRUCTURES
#include "xf86PciInfo.h"
#include "xf86Parser.h"
#include "xf86Config.h"
#ifdef XINPUT
# include "xf86Xinput.h"
#endif
#include "xf86OSmouse.h"
#include "xf86xv.h"
#include "xf86cmap.h"
#include "xf86fbman.h"
#include "dgaproc.h"
#include "vidmodeproc.h"
#include "xf86miscproc.h"
#include "loader.h"
#define DONT_DEFINE_WRAPPERS
#include "xf86_ansic.h"
#include "xisb.h"
#include "xf86Priv.h"
#include "vbe.h"
#include "xf86sbusBus.h"
#ifdef __alpha__
/* MMIO function prototypes */
#include "compiler.h"
#endif

#ifdef __FreeBSD__
/* XXX used in drmOpen(). This should change to use a less os-specific
 * method. */
int sysctlbyname(const char*, void *, size_t *, void *, size_t);
#endif

/* XXX Should get all of these from elsewhere */
#if defined(PowerMAX_OS) || (defined(sun) && defined(SVR4))
# undef inb
# undef inw
# undef inl
# undef outb
# undef outw
# undef outl

extern void outb(unsigned int a, unsigned char b);
extern void outw(unsigned int a, unsigned short w);
extern void outl(unsigned int a, unsigned long l);
extern unsigned char  inb(unsigned int a);
extern unsigned short inw(unsigned int a);
extern unsigned long  inl(unsigned int a);
#endif

#if defined(__alpha__)
# ifdef linux
extern unsigned long _bus_base(void);
extern void _outb(char val, unsigned short port);
extern void _outw(short val, unsigned short port);
extern void _outl(int val, unsigned short port);
extern unsigned int _inb(unsigned short port);
extern unsigned int _inw(unsigned short port);
extern unsigned int _inl(unsigned short port);
# endif

# ifdef __FreeBSD__ 
#  include <sys/types.h>
extern void outb(u_int32_t port, u_int8_t val);
extern void outw(u_int32_t port, u_int16_t val);
extern void outl(u_int32_t port, u_int32_t val);
extern u_int8_t inb(u_int32_t port);
extern u_int16_t inw(u_int32_t port);
extern u_int32_t inl(u_int32_t port);
# endif

extern void* __divl(long, long);
extern void* __reml(long, long);
extern void* __divlu(long, long);
extern void* __remlu(long, long);
extern void* __divq(long, long);
extern void* __divqu(long, long);
extern void* __remq(long, long);
extern void* __remqu(long, long);
#endif

#if defined(__ia64__)
extern long __divdf3(long, long);
extern long __divdi3(long, long);
extern long __divsf3(long, long);
extern long __moddi3(long, long);
extern long __udivdi3(long, long);
extern long __umoddi3(long, long);
extern void _outb(char val, unsigned short port);
extern void _outw(short val, unsigned short port);
extern void _outl(int val, unsigned short port);
extern unsigned int _inb(unsigned short port);
extern unsigned int _inw(unsigned short port);
extern unsigned int _inl(unsigned short port);
#endif

#if defined(__powerpc__) && (defined(Lynx) || defined(linux))
void eieio();
void _restf14();
void _restf17();
void _restf18();
void _restf19();
void _restf20();
void _restf22();
void _restf23();
void _restf24();
void _restf25();
void _restf26();
void _restf27();
void _restf28();
void _restf29();
void _savef14();
void _savef17();
void _savef18();
void _savef19();
void _savef20();
void _savef22();
void _savef23();
void _savef24();
void _savef25();
void _savef26();
void _savef27();
void _savef28();
void _savef29();

/* even if we compile without -DNO_INLINE we still provide
 * the usual port i/o functions for module use
 */

extern volatile unsigned char *ioBase;

/* XXX Should get all of these from elsewhere */

extern void outb(unsigned short, unsigned char);
extern void outw(unsigned short, unsigned short);
extern void outl(unsigned short, unsigned int);
extern unsigned int inb(unsigned short);
extern unsigned int inw(unsigned short);
extern unsigned int inl(unsigned short);
extern unsigned long ldq_u(void *);
extern unsigned long ldl_u(void *);
extern unsigned short ldw_u(void *);
extern void stl_u(unsigned long, void *);
extern void stq_u(unsigned long, void *);
extern void stw_u(unsigned short, void *);
extern void mem_barrier(void);
extern void write_mem_barrier(void);
extern void stl_brx(unsigned long, volatile unsigned char *, int);
extern void stw_brx(unsigned short, volatile unsigned char *, int);
extern unsigned long ldl_brx(volatile unsigned char *, int);
extern unsigned short ldw_brx(volatile unsigned char *, int);
extern unsigned char rdinx(unsigned short, unsigned char);
extern void wrinx(unsigned short, unsigned char, unsigned char);
extern void modinx(unsigned short, unsigned char, unsigned char, unsigned char);
extern int testrg(unsigned short, unsigned char);
extern int testinx2(unsigned short, unsigned char, unsigned char);
extern int testinx(unsigned short, unsigned char);
#endif

/* XXX This needs to be cleaned up for the new design */

#ifdef DPMSExtension
extern void DPMSSet(CARD16);
#endif

/* XFree86 things */

LOOKUP xfree86LookupTab[] = {

   /* Public OSlib functions */
   SYMFUNC(xf86ReadBIOS)
   SYMFUNC(xf86EnableIO)
   SYMFUNC(xf86DisableIO)
   SYMFUNC(xf86DisableInterrupts)
   SYMFUNC(xf86EnableInterrupts)
   SYMFUNC(xf86LinearVidMem)
   SYMFUNC(xf86CheckMTRR)
   SYMFUNC(xf86MapVidMem)
   SYMFUNC(xf86UnMapVidMem)
   SYMFUNC(xf86MapReadSideEffects)
   SYMFUNC(xf86UDelay) 
   SYMFUNC(xf86IODelay)
   SYMFUNC(xf86SlowBcopy)
#ifdef __alpha__
   SYMFUNC(xf86SlowBCopyToBus)
   SYMFUNC(xf86SlowBCopyFromBus)
#endif
   SYMFUNC(xf86BusToMem)
   SYMFUNC(xf86MemToBus)
   SYMFUNC(xf86OpenSerial)
   SYMFUNC(xf86SetSerial)
   SYMFUNC(xf86SetSerialSpeed)
   SYMFUNC(xf86ReadSerial)
   SYMFUNC(xf86WriteSerial)
   SYMFUNC(xf86CloseSerial)
   SYMFUNC(xf86GetErrno)
   SYMFUNC(xf86WaitForInput)
   SYMFUNC(xf86SerialSendBreak)
   SYMFUNC(xf86FlushInput)
   SYMFUNC(xf86SetSerialModemState)
   SYMFUNC(xf86GetSerialModemState)
   SYMFUNC(xf86SerialModemSetBits)
   SYMFUNC(xf86SerialModemClearBits)
   SYMFUNC(xf86LoadKernelModule)
   SYMFUNC(xf86OSMouseInit)
   SYMFUNC(xf86AgpGARTSupported)
   SYMFUNC(xf86GetAGPInfo)
   SYMFUNC(xf86AcquireGART)
   SYMFUNC(xf86ReleaseGART)
   SYMFUNC(xf86AllocateGARTMemory)
   SYMFUNC(xf86BindGARTMemory)
   SYMFUNC(xf86UnbindGARTMemory)
   SYMFUNC(xf86EnableAGP)
   SYMFUNC(xf86SoundKbdBell)
   
#ifdef XINPUT
/* XISB routines  (Merged from Metrolink tree) */
   SYMFUNC(XisbNew)
   SYMFUNC(XisbFree)
   SYMFUNC(XisbRead)
   SYMFUNC(XisbWrite)
   SYMFUNC(XisbTrace)
   SYMFUNC(XisbBlockDuration)
#endif

   /* xf86Bus.c */
   SYMFUNC(xf86CheckPciSlot)
   SYMFUNC(xf86ClaimPciSlot)
   SYMFUNC(xf86GetPciVideoInfo)
   SYMFUNC(xf86GetPciEntity)
   SYMFUNC(xf86GetPciConfigInfo)
   SYMFUNC(xf86SetPciVideo)
   SYMFUNC(xf86ClaimIsaSlot)
   SYMFUNC(xf86ClaimFbSlot)
   SYMFUNC(xf86ParsePciBusString)
   SYMFUNC(xf86ComparePciBusString)
   SYMFUNC(xf86ParseIsaBusString)
   SYMFUNC(xf86EnableAccess)
   SYMFUNC(xf86SetCurrentAccess)
   SYMFUNC(xf86IsPrimaryPci)
   SYMFUNC(xf86IsPrimaryIsa)
   SYMFUNC(xf86CheckPciGAType)
   SYMFUNC(xf86PrintResList)
   SYMFUNC(xf86AddResToList)
   SYMFUNC(xf86JoinResLists)
   SYMFUNC(xf86DupResList)
   SYMFUNC(xf86FreeResList)
   SYMFUNC(xf86ClaimFixedResources)
   SYMFUNC(xf86AddEntityToScreen)
   SYMFUNC(xf86SetEntityInstanceForScreen)
   SYMFUNC(xf86RemoveEntityFromScreen)
   SYMFUNC(xf86GetEntityInfo)
   SYMFUNC(xf86GetNumEntityInstances)
   SYMFUNC(xf86GetDevFromEntity)
   SYMFUNC(xf86GetPciInfoForEntity)
   SYMFUNC(xf86SetEntityFuncs)
   SYMFUNC(xf86DeallocateResourcesForEntity)
   SYMFUNC(xf86RegisterResources)
   SYMFUNC(xf86CheckPciMemBase)
   SYMFUNC(xf86SetAccessFuncs)
   SYMFUNC(xf86IsEntityPrimary)
   SYMFUNC(xf86FixPciResource)
   SYMFUNC(xf86SetOperatingState)
   SYMFUNC(xf86EnterServerState)
   SYMFUNC(xf86GetBlock)
   SYMFUNC(xf86GetSparse)
   SYMFUNC(xf86ReallocatePciResources)
   SYMFUNC(xf86ChkConflict)
   SYMFUNC(xf86IsPciDevPresent)
   SYMFUNC(xf86FindScreenForEntity)
   SYMFUNC(xf86FindPciDeviceVendor)
   SYMFUNC(xf86FindPciClass)
   SYMFUNC(xf86RegisterStateChangeNotificationCallback)
   SYMFUNC(xf86DeregisterStateChangeNotificationCallback)
   SYMFUNC(xf86NoSharedResources)
#ifdef async
   SYMFUNC(xf86QueueAsyncEvent)
#endif
   /* Shared Accel Accessor Functions */
   SYMFUNC(xf86GetLastScrnFlag)
   SYMFUNC(xf86SetLastScrnFlag)
   SYMFUNC(xf86IsEntityShared)
   SYMFUNC(xf86SetEntityShared)
   SYMFUNC(xf86IsEntitySharable)
   SYMFUNC(xf86SetEntitySharable)
   SYMFUNC(xf86IsPrimInitDone)
   SYMFUNC(xf86SetPrimInitDone)
   SYMFUNC(xf86ClearPrimInitDone)
   SYMFUNC(xf86AllocateEntityPrivateIndex)
   SYMFUNC(xf86GetEntityPrivate)
     
   /* xf86Configure.c */
   SYMFUNC(xf86AddDeviceToConfigure)
	   
   /* xf86Cursor.c  XXX not all of these should be exported */
   SYMFUNC(xf86LockZoom)
   SYMFUNC(xf86SetViewport)
   SYMFUNC(xf86ZoomLocked)
   SYMFUNC(xf86ZoomViewport)
   SYMFUNC(xf86GetPointerScreenFuncs)

   /* xf86DGA.c */
   /* For drivers */
   SYMFUNC(DGAInit)
   /* For extmod */
   SYMFUNC(DGAAvailable)
   SYMFUNC(DGAActive)
   SYMFUNC(DGASetMode)
   SYMFUNC(DGASetInputMode)
   SYMFUNC(DGASelectInput)
   SYMFUNC(DGAGetViewportStatus)
   SYMFUNC(DGASetViewport)
   SYMFUNC(DGAInstallCmap)
   SYMFUNC(DGASync)
   SYMFUNC(DGAFillRect)
   SYMFUNC(DGABlitRect)
   SYMFUNC(DGABlitTransRect)
   SYMFUNC(DGAGetModes)
   SYMFUNC(DGAGetOldDGAMode)
   SYMFUNC(DGAGetModeInfo)
   SYMFUNC(DGAChangePixmapMode)
   SYMFUNC(DGACreateColormap)
   SYMFUNC(DGAOpenFramebuffer)
   SYMFUNC(DGACloseFramebuffer)

   /* xf86DPMS.c */
   SYMFUNC(xf86DPMSInit)

   /* xf86Events.c */
   SYMFUNC(SetTimeSinceLastInputEvent)
   SYMFUNC(xf86AddInputHandler)
   SYMFUNC(xf86RemoveInputHandler)
   SYMFUNC(xf86DisableInputHandler)
   SYMFUNC(xf86EnableInputHandler)
   SYMFUNC(xf86AddEnabledDevice)
   SYMFUNC(xf86RemoveEnabledDevice)
   SYMFUNC(xf86InterceptSignals)
   SYMFUNC(xf86EnableVTSwitch)

   /* xf86Helper.c */
   SYMFUNC(xf86AddDriver)
   SYMFUNC(xf86AddInputDriver)
   SYMFUNC(xf86DeleteDriver)
   SYMFUNC(xf86DeleteInput)
   SYMFUNC(xf86AllocateInput)
   SYMFUNC(xf86AllocateScreen)
   SYMFUNC(xf86DeleteScreen)
   SYMFUNC(xf86AllocateScrnInfoPrivateIndex)
   SYMFUNC(xf86AddPixFormat)
   SYMFUNC(xf86SetDepthBpp)
   SYMFUNC(xf86PrintDepthBpp)
   SYMFUNC(xf86SetWeight)
   SYMFUNC(xf86SetDefaultVisual)
   SYMFUNC(xf86SetGamma)
   SYMFUNC(xf86SetDpi)
   SYMFUNC(xf86SetBlackWhitePixels)
   SYMFUNC(xf86EnableDisableFBAccess)
   SYMFUNC(xf86VDrvMsgVerb)
   SYMFUNC(xf86DrvMsgVerb)
   SYMFUNC(xf86DrvMsg)
   SYMFUNC(xf86MsgVerb)
   SYMFUNC(xf86Msg)
   SYMFUNC(xf86ErrorFVerb)
   SYMFUNC(xf86ErrorF)
   SYMFUNC(xf86TokenToString)
   SYMFUNC(xf86StringToToken)
   SYMFUNC(xf86ShowClocks)
   SYMFUNC(xf86PrintChipsets)
   SYMFUNC(xf86MatchDevice)
   SYMFUNC(xf86MatchPciInstances)
   SYMFUNC(xf86MatchIsaInstances)
   SYMFUNC(xf86GetVerbosity)
   SYMFUNC(xf86GetVisualName)
   SYMFUNC(xf86GetPix24)
   SYMFUNC(xf86GetDepth)
   SYMFUNC(xf86GetWeight)
   SYMFUNC(xf86GetGamma)
   SYMFUNC(xf86GetFlipPixels)
   SYMFUNC(xf86GetServerName)
   SYMFUNC(xf86ServerIsExiting)
   SYMFUNC(xf86ServerIsOnlyDetecting)
   SYMFUNC(xf86ServerIsOnlyProbing)
   SYMFUNC(xf86ServerIsResetting)
   SYMFUNC(xf86CaughtSignal)
   SYMFUNC(xf86GetVidModeAllowNonLocal)
   SYMFUNC(xf86GetVidModeEnabled)
   SYMFUNC(xf86GetModInDevAllowNonLocal)
   SYMFUNC(xf86GetModInDevEnabled)
   SYMFUNC(xf86GetAllowMouseOpenFail)
   SYMFUNC(xf86IsPc98)
   SYMFUNC(xf86GetClocks)
   SYMFUNC(xf86SetPriority)
   SYMFUNC(xf86LoadDrvSubModule)
   SYMFUNC(xf86LoadSubModule)
   SYMFUNC(xf86LoadOneModule)
   SYMFUNC(xf86UnloadSubModule)
   SYMFUNC(xf86LoaderCheckSymbol)
   SYMFUNC(xf86LoaderReqSymLists)
   SYMFUNC(xf86LoaderReqSymbols)
   SYMFUNC(xf86SetBackingStore)
   SYMFUNC(xf86SetSilkenMouse)
   /* SYMFUNC(xf86NewSerialNumber) */
   SYMFUNC(xf86FindXvOptions)
   SYMFUNC(xf86GetOS)
   SYMFUNC(xf86ConfigPciEntity)
   SYMFUNC(xf86ConfigIsaEntity)
   SYMFUNC(xf86ConfigFbEntity)
   SYMFUNC(xf86ConfigActivePciEntity)
   SYMFUNC(xf86ConfigActiveIsaEntity)
   SYMFUNC(xf86ConfigPciEntityInactive)
   SYMFUNC(xf86ConfigIsaEntityInactive)
   SYMFUNC(xf86IsScreenPrimary)
   SYMFUNC(xf86RegisterRootWindowProperty)
   SYMFUNC(xf86IsUnblank)

#ifdef __sparc__
   /* xf86sbusBus.c */
   SYMFUNC(xf86MatchSbusInstances)
   SYMFUNC(xf86GetSbusInfoForEntity)
   SYMFUNC(xf86GetEntityForSbusInfo)
   SYMFUNC(xf86SbusUseBuiltinMode)
   SYMFUNC(xf86MapSbusMem)
   SYMFUNC(xf86UnmapSbusMem)
   SYMFUNC(xf86SbusHideOsHwCursor)
   SYMFUNC(xf86SbusSetOsHwCursorCmap)
   SYMFUNC(xf86SbusHandleColormaps)
   SYMFUNC(sparcPromInit)
   SYMFUNC(sparcPromClose)
   SYMFUNC(sparcPromGetProperty)
   SYMFUNC(sparcPromGetBool)
#endif

   /* xf86Init.c */
   SYMFUNC(xf86GetPixFormat)
   SYMFUNC(xf86GetBppFromDepth)

   /* xf86Mode.c */
   SYMFUNC(xf86GetNearestClock)
   SYMFUNC(xf86ModeStatusToString)
   SYMFUNC(xf86LookupMode)
   SYMFUNC(xf86CheckModeForMonitor)
   SYMFUNC(xf86InitialCheckModeForDriver)
   SYMFUNC(xf86CheckModeForDriver)
   SYMFUNC(xf86ValidateModes)
   SYMFUNC(xf86DeleteMode)
   SYMFUNC(xf86PruneDriverModes)
   SYMFUNC(xf86SetCrtcForModes)
   SYMFUNC(xf86PrintModes)
   SYMFUNC(xf86ShowClockRanges)

   /* xf86Option.c */
   SYMFUNC(xf86CollectOptions)
   SYMFUNC(xf86CollectInputOptions)
   /* Merging of XInput stuff	*/
   SYMFUNC(xf86AddNewOption)
   SYMFUNC(xf86SetBoolOption)
   SYMFUNC(xf86NewOption)
   SYMFUNC(xf86NextOption)
   SYMFUNC(xf86OptionListCreate)
   SYMFUNC(xf86OptionListMerge)
   SYMFUNC(xf86OptionListFree)
   SYMFUNC(xf86OptionName)
   SYMFUNC(xf86OptionValue)
   SYMFUNC(xf86OptionListReport)
   SYMFUNC(xf86SetIntOption)
   SYMFUNC(xf86SetStrOption)
   SYMFUNC(xf86ReplaceIntOption)
   SYMFUNC(xf86ReplaceStrOption)
   SYMFUNC(xf86ReplaceBoolOption)
   SYMFUNC(xf86FindOption)
   SYMFUNC(xf86FindOptionValue)
   SYMFUNC(xf86MarkOptionUsed)
   SYMFUNC(xf86MarkOptionUsedByName)
   SYMFUNC(xf86CheckIfOptionUsed)
   SYMFUNC(xf86CheckIfOptionUsedByName)
   SYMFUNC(xf86ShowUnusedOptions)
   SYMFUNC(xf86ProcessOptions)
   SYMFUNC(xf86TokenToOptinfo)
   SYMFUNC(xf86TokenToOptName)
   SYMFUNC(xf86IsOptionSet)
   SYMFUNC(xf86GetOptValString)
   SYMFUNC(xf86GetOptValInteger)
   SYMFUNC(xf86GetOptValULong)
   SYMFUNC(xf86GetOptValReal)
   SYMFUNC(xf86GetOptValFreq)
   SYMFUNC(xf86GetOptValBool)
   SYMFUNC(xf86ReturnOptValBool)
   SYMFUNC(xf86NameCmp)
   SYMFUNC(xf86InitValuatorAxisStruct)
   SYMFUNC(xf86InitValuatorDefaults)
   

   /* xf86fbman.c */
   SYMFUNC(xf86InitFBManager)
   SYMFUNC(xf86InitFBManagerRegion)
   SYMFUNC(xf86RegisterFreeBoxCallback)
   SYMFUNC(xf86FreeOffscreenArea)
   SYMFUNC(xf86AllocateOffscreenArea)
   SYMFUNC(xf86AllocateLinearOffscreenArea)
   SYMFUNC(xf86ResizeOffscreenArea)
   SYMFUNC(xf86FBManagerRunning)
   SYMFUNC(xf86QueryLargestOffscreenArea)
   SYMFUNC(xf86PurgeUnlockedOffscreenAreas)
   SYMFUNC(xf86RegisterOffscreenManager)
   SYMFUNC(xf86AllocateOffscreenLinear)
   SYMFUNC(xf86ResizeOffscreenLinear)
   SYMFUNC(xf86QueryLargestOffscreenLinear)
   SYMFUNC(xf86FreeOffscreenLinear)


   /* xf86cmap.c */
   SYMFUNC(xf86HandleColormaps)

   /* xf86xv.c */
   SYMFUNC(xf86XVScreenInit)
   SYMFUNC(xf86XVRegisterGenericAdaptorDriver)
   SYMFUNC(xf86XVListGenericAdaptors)
   SYMFUNC(xf86XVRegisterOffscreenImages)
   SYMFUNC(xf86XVQueryOffscreenImages)
   SYMFUNC(xf86XVAllocateVideoAdaptorRec)
   SYMFUNC(xf86XVFreeVideoAdaptorRec)

   /* xf86VidMode.c */
   SYMFUNC(VidModeExtensionInit)
#ifdef XF86VIDMODE
   SYMFUNC(VidModeGetCurrentModeline)
   SYMFUNC(VidModeGetFirstModeline)
   SYMFUNC(VidModeGetNextModeline)
   SYMFUNC(VidModeDeleteModeline)
   SYMFUNC(VidModeZoomViewport)
   SYMFUNC(VidModeGetViewPort)
   SYMFUNC(VidModeSetViewPort)
   SYMFUNC(VidModeSwitchMode)
   SYMFUNC(VidModeLockZoom)
   SYMFUNC(VidModeGetMonitor)
   SYMFUNC(VidModeGetNumOfClocks)
   SYMFUNC(VidModeGetClocks)
   SYMFUNC(VidModeCheckModeForMonitor)
   SYMFUNC(VidModeCheckModeForDriver)
   SYMFUNC(VidModeSetCrtcForMode)
   SYMFUNC(VidModeAddModeline)
   SYMFUNC(VidModeGetDotClock)
   SYMFUNC(VidModeGetNumOfModes)
   SYMFUNC(VidModeSetGamma)
   SYMFUNC(VidModeGetGamma)
   SYMFUNC(VidModeCreateMode)
   SYMFUNC(VidModeCopyMode)
   SYMFUNC(VidModeGetModeValue)
   SYMFUNC(VidModeSetModeValue)
   SYMFUNC(VidModeGetMonitorValue)
#endif

   /* xf86MiscExt.c */
#ifdef XF86MISC
   SYMFUNC(MiscExtGetMouseSettings)
   SYMFUNC(MiscExtGetMouseValue)
   SYMFUNC(MiscExtSetMouseValue)
   SYMFUNC(MiscExtGetKbdSettings)
   SYMFUNC(MiscExtGetKbdValue)
   SYMFUNC(MiscExtSetKbdValue)
   SYMFUNC(MiscExtCreateStruct)
   SYMFUNC(MiscExtDestroyStruct)
   SYMFUNC(MiscExtApply)
#endif

   /* Misc */
   SYMFUNC(GetTimeInMillis)

   /* xf86Xinput.c */
   SYMFUNC(xf86ProcessCommonOptions)
#ifdef XINPUT
   SYMFUNC(xf86IsCorePointer)
   SYMFUNC(xf86PostMotionEvent)
   SYMFUNC(xf86PostProximityEvent)
   SYMFUNC(xf86PostButtonEvent)
   SYMFUNC(xf86PostKeyEvent)
   SYMFUNC(xf86GetMotionEvents)
   SYMFUNC(xf86MotionHistoryAllocate)
   SYMFUNC(xf86FirstLocalDevice)
/* The following segment merged from Metrolink tree */
   SYMFUNC(xf86XInputSetScreen)
   SYMFUNC(xf86ScaleAxis)
   SYMFUNC(xf86XInputSetSendCoreEvents)
/* End merged segment */
#endif
#ifdef DPMSExtension
   SYMFUNC(DPMSSet)
#endif
/* xf86Debug.c */
#ifdef BUILDDEBUG
   SYMFUNC(xf86Break1)
   SYMFUNC(xf86Break2)
   SYMFUNC(xf86Break3)
   SYMFUNC(xf86SPTimestamp)
   SYMFUNC(xf86STimestamp)
#endif
   
#if 0 /* we want to move the hw stuff in a module */
   SYMFUNC(xf86dactopel)
   SYMFUNC(xf86dactocomm)
   SYMFUNC(xf86getdaccomm)
   SYMFUNC(xf86setdaccomm)
   SYMFUNC(xf86setdaccommbit)
   SYMFUNC(xf86clrdaccommbit)
   SYMFUNC(s3IBMRGB_Probe)
   SYMFUNC(s3IBMRGB_Init)
   SYMFUNC(s3InIBMRGBIndReg)
   SYMFUNC(Ti3025SetClock)
   SYMFUNC(Ti3026SetClock)
   SYMFUNC(Ti3030SetClock)
   SYMFUNC(AltICD2061SetClock)
   SYMFUNC(SC11412SetClock)
   SYMFUNC(ICS2595SetClock)
   SYMFUNC(Att409SetClock)
   SYMFUNC(Chrontel8391SetClock)
   SYMFUNC(IBMRGBSetClock)
   SYMFUNC(ICS5342SetClock)
   SYMFUNC(S3TrioSetClock)
   SYMFUNC(S3Trio64V2SetClock)
   SYMFUNC(S3gendacSetClock)
   SYMFUNC(STG1703SetClock)
   SYMFUNC(ET6000SetClock)
   SYMFUNC(S3AuroraSetClock)
   SYMFUNC(commonCalcClock)
   SYMFUNC(xf86writepci)
   SYMFUNC(dacOutTi3026IndReg)
   SYMFUNC(dacInTi3026IndReg)
   SYMFUNC(s3OutIBMRGBIndReg)
   SYMFUNC(CirrusFindClock)
   SYMFUNC(CirrusSetClock)
   SYMFUNC(STG1703getIndex)
   SYMFUNC(STG1703setIndex)
   SYMFUNC(STG1703magic)
   SYMFUNC(gendacMNToClock)
   SYMFUNC(Et4000AltICD2061SetClock)
   SYMFUNC(ET4000stg1703SetClock)
   SYMFUNC(ET4000gendacSetClock)

#endif
   
   SYMFUNC(pciFindFirst)
   SYMFUNC(pciFindNext)
   SYMFUNC(pciWriteByte)
   SYMFUNC(pciWriteWord)
   SYMFUNC(pciWriteLong)
   SYMFUNC(pciReadByte)
   SYMFUNC(pciReadWord)
   SYMFUNC(pciReadLong)
   SYMFUNC(pciSetBitsLong)
   SYMFUNC(pciTag)
   SYMFUNC(pciBusAddrToHostAddr)
   SYMFUNC(pciHostAddrToBusAddr)
   SYMFUNC(xf86MapPciMem)
   SYMFUNC(xf86scanpci)
   SYMFUNC(xf86ReadPciBIOS)
   SYMFUNC(AllocatePixmapPrivateIndex)
   SYMFUNC(AllocatePixmapPrivate)

   /* Loader functions */
   SYMFUNC(LoaderDefaultFunc)
   SYMFUNC(LoadSubModule)
   SYMFUNC(DuplicateModule)
   SYMFUNC(LoaderErrorMsg)
   SYMFUNC(LoaderCheckUnresolved)
   SYMFUNC(LoadExtension)
   SYMFUNC(LoadFont)
   SYMFUNC(LoaderReqSymbols)
   SYMFUNC(LoaderReqSymLists)
   SYMFUNC(LoaderRefSymbols)
   SYMFUNC(LoaderRefSymLists)
   SYMFUNC(UnloadSubModule)
   SYMFUNC(LoaderSymbol)
   SYMFUNC(LoaderListDirs)
   SYMFUNC(LoaderFreeDirList)
   SYMFUNC(LoaderGetOS)

   /*
    * these here are our own interfaces to libc functions
    */
   SYMFUNC(xf86abort)
   SYMFUNC(xf86abs)
   SYMFUNC(xf86acos)
   SYMFUNC(xf86asin)
   SYMFUNC(xf86atan)
   SYMFUNC(xf86atan2)
   SYMFUNC(xf86atof)
   SYMFUNC(xf86atoi)
   SYMFUNC(xf86atol)
   SYMFUNC(xf86bsearch)
   SYMFUNC(xf86ceil)
   SYMFUNC(xf86calloc)
   SYMFUNC(xf86clearerr)
   SYMFUNC(xf86close)
   SYMFUNC(xf86cos)
   SYMFUNC(xf86exit)
   SYMFUNC(xf86exp)
   SYMFUNC(xf86fabs)
   SYMFUNC(xf86fclose)
   SYMFUNC(xf86feof)
   SYMFUNC(xf86ferror)
   SYMFUNC(xf86fflush)
   SYMFUNC(xf86fgetc)
   SYMFUNC(xf86fgetpos)
   SYMFUNC(xf86fgets)
   SYMFUNC(xf86floor)
   SYMFUNC(xf86fmod)
   SYMFUNC(xf86fopen)
   SYMFUNC(xf86fprintf)
   SYMFUNC(xf86fputc)
   SYMFUNC(xf86fputs)
   SYMFUNC(xf86fread)
   SYMFUNC(xf86free)
   SYMFUNC(xf86freopen)
   SYMFUNC(xf86frexp)
   SYMFUNC(xf86fscanf)
   SYMFUNC(xf86fseek)
   SYMFUNC(xf86fsetpos)
   SYMFUNC(xf86ftell)
   SYMFUNC(xf86fwrite)
   SYMFUNC(xf86getc)
   SYMFUNC(xf86getenv)
   SYMFUNC(xf86getpagesize)
   SYMFUNC(xf86hypot)
   SYMFUNC(xf86ioctl)
   SYMFUNC(xf86isalnum)
   SYMFUNC(xf86isalpha)
   SYMFUNC(xf86iscntrl)
   SYMFUNC(xf86isdigit)
   SYMFUNC(xf86isgraph)
   SYMFUNC(xf86islower)
   SYMFUNC(xf86isprint)
   SYMFUNC(xf86ispunct)
   SYMFUNC(xf86isspace)
   SYMFUNC(xf86isupper)
   SYMFUNC(xf86isxdigit)
   SYMFUNC(xf86labs)
   SYMFUNC(xf86ldexp)
   SYMFUNC(xf86log)
   SYMFUNC(xf86log10)
   SYMFUNC(xf86lseek)
   SYMFUNC(xf86malloc)
   SYMFUNC(xf86memchr)
   SYMFUNC(xf86memcmp)
   SYMFUNC(xf86memcpy)
#if (defined(__powerpc__) && (defined(Lynx) || defined(linux))) || defined(__sparc__) || defined(__ia64__)
   /*
    * Some PPC, SPARC, and IA64 compilers generate calls to memcpy to handle
    * structure copies.  This causes a problem both here and in shared
    * libraries as there is no way to map the name of the call to the
    * correct function.
    */
   SYMFUNC(memcpy)
   /*
    * Some PPC, SPARC, and IA64 compilers generate calls to memset to handle 
    * aggregate initializations.
    */
   SYMFUNC(memset)
#endif
   SYMFUNC(xf86memmove)
   SYMFUNC(xf86memset)
   SYMFUNC(xf86mmap)
   SYMFUNC(xf86modf)
   SYMFUNC(xf86munmap)
   SYMFUNC(xf86open)
   SYMFUNC(xf86perror)
   SYMFUNC(xf86pow)
   SYMFUNC(xf86printf)
   SYMFUNC(xf86qsort)
   SYMFUNC(xf86read)
   SYMFUNC(xf86realloc)
   SYMFUNC(xf86remove)
   SYMFUNC(xf86rename)
   SYMFUNC(xf86rewind)
   SYMFUNC(xf86setbuf)
   SYMFUNC(xf86setvbuf)
   SYMFUNC(xf86sin)
   SYMFUNC(xf86snprintf)
   SYMFUNC(xf86sprintf)
   SYMFUNC(xf86sqrt)
   SYMFUNC(xf86sscanf)
   SYMFUNC(xf86strcat)
   SYMFUNC(xf86strcmp)
   SYMFUNC(xf86strcasecmp)
   SYMFUNC(xf86strcpy)
   SYMFUNC(xf86strcspn)
   SYMFUNC(xf86strerror)
   SYMFUNC(xf86strlen)
   SYMFUNC(xf86strncmp)
   SYMFUNC(xf86strncasecmp)
   SYMFUNC(xf86strncpy)
   SYMFUNC(xf86strpbrk)
   SYMFUNC(xf86strchr)
   SYMFUNC(xf86strrchr)
   SYMFUNC(xf86strspn)
   SYMFUNC(xf86strstr)
   SYMFUNC(xf86strtod)
   SYMFUNC(xf86strtok)
   SYMFUNC(xf86strtol)
   SYMFUNC(xf86strtoul)
   SYMFUNC(xf86tan)
   SYMFUNC(xf86tmpfile)
   SYMFUNC(xf86tolower)
   SYMFUNC(xf86toupper)
   SYMFUNC(xf86ungetc)
   SYMFUNC(xf86vfprintf)
   SYMFUNC(xf86vsnprintf)
   SYMFUNC(xf86vsprintf)
   SYMFUNC(xf86write)
  
/* non-ANSI C functions */
   SYMFUNC(xf86opendir)
   SYMFUNC(xf86closedir)
   SYMFUNC(xf86readdir)
   SYMFUNC(xf86rewinddir)
   SYMFUNC(xf86ffs)
   SYMFUNC(xf86strdup)
   SYMFUNC(xf86bzero)
   SYMFUNC(xf86usleep)
   SYMFUNC(xf86execl)

   SYMFUNC(xf86getsecs)
   SYMFUNC(xf86fpossize)      /* for returning sizeof(fpos_t) */

				/* These provide for DRI support. */
   SYMFUNC(xf86stat)
   SYMFUNC(xf86fstat)
   SYMFUNC(xf86access)
   SYMFUNC(xf86geteuid)
   SYMFUNC(xf86getegid)
   SYMFUNC(xf86getpid)
   SYMFUNC(xf86mknod)
   SYMFUNC(xf86chmod)
   SYMFUNC(xf86chown)
   SYMFUNC(xf86sleep)
   SYMFUNC(xf86mkdir)
   SYMFUNC(xf86shmget)
   SYMFUNC(xf86shmat)
   SYMFUNC(xf86shmdt)
   SYMFUNC(xf86shmctl)
   SYMFUNC(xf86setjmp)
   SYMFUNC(xf86longjmp)
#ifdef XF86DRI
				/* These may have more general uses, but
                                   for now, they are only used by the DRI.
                                   Loading them only when the DRI is built
                                   may make porting (the non-DRI portions
                                   of the X server) easier. */
   SYMFUNC(xf86InstallSIGIOHandler)
   SYMFUNC(xf86RemoveSIGIOHandler)
# if defined(__alpha__) && defined(linux)
   SYMFUNC(_bus_base)
# endif
#endif
   SYMFUNC(xf86BlockSIGIO)
   SYMFUNC(xf86UnblockSIGIO)
  
#if defined(__alpha__)
   SYMFUNC(__divl)
   SYMFUNC(__reml)
   SYMFUNC(__divlu)
   SYMFUNC(__remlu)
   SYMFUNC(__divq)
   SYMFUNC(__divqu)
   SYMFUNC(__remq)
   SYMFUNC(__remqu)

# ifdef linux
   SYMFUNC(_outw)
   SYMFUNC(_outb)
   SYMFUNC(_outl)
   SYMFUNC(_inb)
   SYMFUNC(_inw)
   SYMFUNC(_inl)
# else
   SYMFUNC(outw)
   SYMFUNC(outb)
   SYMFUNC(outl)
   SYMFUNC(inb)
   SYMFUNC(inw)
   SYMFUNC(inl)
# endif
   SYMFUNC(xf86ReadMmio32)
   SYMFUNC(xf86ReadMmio16)
   SYMFUNC(xf86ReadMmio8)
   SYMFUNC(xf86WriteMmio32)
   SYMFUNC(xf86WriteMmio16)
   SYMFUNC(xf86WriteMmio8)
   SYMFUNC(xf86WriteMmioNB32)
   SYMFUNC(xf86WriteMmioNB16)
   SYMFUNC(xf86WriteMmioNB8)
   SYMFUNC(memcpy)
#endif
#if defined(sun) && defined(SVR4)
   SYMFUNC(inb)
   SYMFUNC(inw)
   SYMFUNC(inl)
   SYMFUNC(outb)
   SYMFUNC(outw)
   SYMFUNC(outl)
#endif
#if defined(__powerpc__) && !defined(__OpenBSD__)
   SYMFUNC(inb)
   SYMFUNC(inw)
   SYMFUNC(inl)
   SYMFUNC(outb)
   SYMFUNC(outw)
   SYMFUNC(outl)
# if defined(NO_INLINE) || defined(Lynx)
   SYMFUNC(mem_barrier)
   SYMFUNC(ldl_u)
   SYMFUNC(eieio)
   SYMFUNC(ldl_brx)
   SYMFUNC(ldw_brx)
   SYMFUNC(stl_brx)
   SYMFUNC(stw_brx)
   SYMFUNC(ldq_u)
   SYMFUNC(ldw_u)
   SYMFUNC(stl_u)
   SYMFUNC(stq_u)
   SYMFUNC(stw_u)
   SYMFUNC(write_mem_barrier)
# endif
   SYMFUNC(rdinx)
   SYMFUNC(wrinx)
   SYMFUNC(modinx)
   SYMFUNC(testrg)
   SYMFUNC(testinx2)
   SYMFUNC(testinx)
# if defined(Lynx)
   SYMFUNC(_restf14)
   SYMFUNC(_restf17)
   SYMFUNC(_restf18)
   SYMFUNC(_restf19)
   SYMFUNC(_restf20)
   SYMFUNC(_restf22)
   SYMFUNC(_restf23)
   SYMFUNC(_restf24)
   SYMFUNC(_restf25)
   SYMFUNC(_restf26)
   SYMFUNC(_restf27)
   SYMFUNC(_restf28)
   SYMFUNC(_restf29)
   SYMFUNC(_savef14)
   SYMFUNC(_savef17)
   SYMFUNC(_savef18)
   SYMFUNC(_savef19)
   SYMFUNC(_savef20)
   SYMFUNC(_savef22)
   SYMFUNC(_savef23)
   SYMFUNC(_savef24)
   SYMFUNC(_savef25)
   SYMFUNC(_savef26)
   SYMFUNC(_savef27)
   SYMFUNC(_savef28)
   SYMFUNC(_savef29)
# endif
# if PPCIO_DEBUG
   SYMFUNC(debug_inb)
   SYMFUNC(debug_inw)
   SYMFUNC(debug_inl)
   SYMFUNC(debug_outb)
   SYMFUNC(debug_outw)
   SYMFUNC(debug_outl)
# endif
#endif
#if defined(__ia64__)
   SYMFUNC(__divdf3)
   SYMFUNC(__divdi3)
   SYMFUNC(__divsf3)
   SYMFUNC(__moddi3)
   SYMFUNC(__udivdi3)
   SYMFUNC(__umoddi3)
   SYMFUNC(_outw)
   SYMFUNC(_outb)
   SYMFUNC(_outl)
   SYMFUNC(_inb)
   SYMFUNC(_inw)
   SYMFUNC(_inl)
#endif

#ifdef __FreeBSD__
   SYMFUNC(sysctlbyname)
#endif

/*
 * and now some variables
 */

   SYMVAR(xf86stdin)
   SYMVAR(xf86stdout)
   SYMVAR(xf86stderr)
   SYMVAR(xf86errno)
   SYMVAR(xf86HUGE_VAL)

   /* General variables (from xf86.h) */
   SYMVAR(xf86ScreenIndex)
   SYMVAR(xf86PixmapIndex)
   SYMVAR(xf86Screens)
   SYMVAR(byte_reversed)
   /* debugging variables */
   SYMVAR(xf86DummyVar1)
   SYMVAR(xf86DummyVar2)
   SYMVAR(xf86DummyVar3)

   /* variables for PCI devices and cards from xf86Bus.c */
   SYMVAR(xf86PCICardInfo)
   SYMVAR(xf86PCIVendorInfo)
   SYMVAR(xf86PCIVendorNameInfo)
#ifdef async
   SYMVAR(xf86CurrentScreen)
#endif
   /* predefined resource lists from xf86Bus.h */
   SYMVAR(resVgaExclusive)
   SYMVAR(resVgaShared)
   SYMVAR(resVgaMemShared)
   SYMVAR(resVgaIoShared)
   SYMVAR(resVgaUnusedExclusive)
   SYMVAR(resVgaUnusedShared)
   SYMVAR(resVgaSparseExclusive)
   SYMVAR(resVgaSparseShared)
   SYMVAR(res8514Exclusive)
   SYMVAR(res8514Shared)
   SYMVAR(PciAvoid)

#if defined(__powerpc__) && (!defined(NO_INLINE) || defined(Lynx)) && !defined(__OpenBSD__)
   SYMVAR(ioBase)
#endif

   /* Globals from xf86Globals.c and xf86Priv.h */
   SYMVAR(xf86ConfigDRI)

   /* Globals from xf86Configure.c */
   SYMVAR(ConfiguredMonitor)

   /* Pci.c */
   SYMVAR(pciNumBuses)
 
  { 0, 0 },

};
