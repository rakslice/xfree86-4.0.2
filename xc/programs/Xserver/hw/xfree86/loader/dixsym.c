/* $XFree86: xc/programs/Xserver/hw/xfree86/loader/dixsym.c,v 1.35 2000/11/22 07:19:44 keithp Exp $ */


/*
 *
 * Copyright 1995-1998 by Metro Link, Inc.
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

#undef DBMALLOC
#include "sym.h"
#include "colormap.h"
#include "cursor.h"
#include "dix.h"
#include "dixfont.h"
#include "dixstruct.h"
#include "misc.h"
#include "globals.h"
#include "os.h"
#include "resource.h"
#include "servermd.h"
#include "scrnintstr.h"
#include "windowstr.h"
#include "extension.h"
#include "extnsionst.h"
#include "swaprep.h"
#include "swapreq.h"
#include "inputstr.h"
#include "XIproto.h"
#include "exevents.h"
#include "extinit.h"
#ifdef XV
#include "xvmodproc.h"
#endif
#ifdef XFreeXDGA
#include "dgaproc.h"
#endif
#ifdef RENDER
#include "picturestr.h"
#include "mipict.h"
#endif

/* XXX This should be in a header somewhere */
extern void ClientSleepUntil(ClientPtr, TimeStamp, void(*)(ClientPtr, pointer),
			     pointer);
extern int ShmCompletionCode;
extern int BadShmSegCode;
extern RESTYPE ShmSegType;

/* DIX things */

LOOKUP dixLookupTab[] = {

  /* dix */
  /* atom.c */
  SYMFUNC(MakeAtom)
  SYMFUNC(ValidAtom)
  /* colormap.c */
  SYMFUNC(AllocColor)
  SYMFUNC(CreateColormap)
  SYMFUNC(FakeAllocColor)
  SYMFUNC(FakeFreeColor)
  SYMFUNC(FreeColors)
  SYMFUNC(StoreColors)
  SYMFUNC(TellLostMap)
  SYMFUNC(TellGainedMap)
  SYMFUNC(QueryColors)
  /* cursor.c */
  SYMFUNC(FreeCursor)
  /* devices.c */
  SYMFUNC(Ones)
  SYMFUNC(InitButtonClassDeviceStruct)
  SYMFUNC(InitFocusClassDeviceStruct)
  SYMFUNC(InitLedFeedbackClassDeviceStruct)
  SYMFUNC(InitPtrFeedbackClassDeviceStruct)
  SYMFUNC(InitValuatorClassDeviceStruct)
  SYMFUNC(InitKeyClassDeviceStruct)
  /* dispatch.c */
  SYMFUNC(SetInputCheck)
  SYMFUNC(SendErrorToClient)
  SYMFUNC(UpdateCurrentTime)
  SYMFUNC(UpdateCurrentTimeIf)
  SYMVAR(dispatchException)
  SYMVAR(isItTimeToYield)
  SYMVAR(ClientStateCallback)
  SYMVAR(ServerGrabCallback)
  /* dixfonts.c */
  SYMFUNC(CloseFont)
  SYMFUNC(FontToXError)
  SYMFUNC(LoadGlyphs)
  SYMVAR(fpe_functions)
  /* dixutils.c */
  SYMFUNC(AddCallback)
  SYMFUNC(ClientSleep)
  SYMFUNC(ClientTimeToServerTime)
  SYMFUNC(ClientWakeup)
  SYMFUNC(CompareTimeStamps)
  SYMFUNC(CopyISOLatin1Lowered)
  SYMFUNC(DeleteCallback)
  SYMFUNC(LookupClient)
  SYMFUNC(LookupDrawable)
  SYMFUNC(LookupWindow)
  SYMFUNC(NoopDDA)
  SYMFUNC(QueueWorkProc)
  SYMFUNC(RegisterBlockAndWakeupHandlers)
  SYMFUNC(RemoveBlockAndWakeupHandlers)
  SYMFUNC(SecurityLookupDrawable)
  SYMFUNC(SecurityLookupWindow)
  /* events.c */
  SYMFUNC(CheckCursorConfinement)
  SYMFUNC(DeliverEvents)
  SYMFUNC(NewCurrentScreen)
  SYMFUNC(PointerConfinedToScreen)
  SYMFUNC(TryClientEvents)
  SYMFUNC(WriteEventsToClient)
  SYMVAR(DeviceEventCallback)
  SYMVAR(EventCallback)
  SYMVAR(inputInfo)
  SYMVAR(SetCriticalEvent)
  /* property.c */
  SYMFUNC(ChangeWindowProperty)
  /* extension.c */
  SYMFUNC(AddExtension)
  SYMFUNC(AddExtensionAlias)
  SYMFUNC(CheckExtension)
  SYMFUNC(DeclareExtensionSecurity)
  SYMFUNC(MinorOpcodeOfRequest)
  SYMFUNC(StandardMinorOpcode)
  /* gc.c */
  SYMFUNC(CopyGC)
  SYMFUNC(CreateGC)
  SYMFUNC(CreateScratchGC)
  SYMFUNC(ChangeGC)
  SYMFUNC(dixChangeGC)
  SYMFUNC(DoChangeGC)
  SYMFUNC(FreeGC)
  SYMFUNC(FreeScratchGC)
  SYMFUNC(GetScratchGC)
  SYMFUNC(SetClipRects)
  SYMFUNC(ValidateGC)
  SYMFUNC(VerifyRectOrder)
  SYMFUNC(SetDashes)
  /* globals.c */
#ifdef DPMSExtension
  SYMVAR(DPMSEnabled)
  SYMVAR(DPMSCapableFlag)
  SYMVAR(DPMSOffTime)
  SYMVAR(DPMSPowerLevel)
  SYMVAR(DPMSStandbyTime)
  SYMVAR(DPMSSuspendTime)
  SYMVAR(DPMSEnabledSwitch)
  SYMVAR(DPMSDisabledSwitch)
  SYMVAR(defaultDPMSEnabled)
#endif
#ifdef XV
  SYMVAR(XvScreenInitProc)
  SYMVAR(XvGetScreenIndexProc)
  SYMVAR(XvGetRTPortProc)
#endif
  SYMVAR(ScreenSaverBlanking)
  SYMVAR(WindowTable)
  SYMVAR(clients)
  SYMVAR(currentMaxClients)
  SYMVAR(currentTime)
  SYMVAR(defaultColorVisualClass)
  SYMVAR(globalSerialNumber)
  SYMVAR(lastDeviceEventTime)
  SYMVAR(monitorResolution)
  SYMVAR(permitOldBugs)
  SYMVAR(screenInfo)
  SYMVAR(serverClient)
  SYMVAR(serverGeneration)
  /* pixmap.c */
  SYMFUNC(AllocatePixmap)
  SYMFUNC(GetScratchPixmapHeader)
  SYMFUNC(FreeScratchPixmapHeader)
  SYMVAR(PixmapWidthPaddingInfo)
  /* privates.c */
  SYMFUNC(AllocateClientPrivate)
  SYMFUNC(AllocateClientPrivateIndex)
  SYMFUNC(AllocateGCPrivate)
  SYMFUNC(AllocateGCPrivateIndex)
  SYMFUNC(AllocateWindowPrivate)
  SYMFUNC(AllocateWindowPrivateIndex)
  SYMFUNC(AllocateScreenPrivateIndex)
  SYMFUNC(AllocateColormapPrivateIndex)
  /* resource.c */
  SYMFUNC(AddResource)
  SYMFUNC(ChangeResourceValue)
  SYMFUNC(CreateNewResourceClass)
  SYMFUNC(CreateNewResourceType)
  SYMFUNC(FakeClientID)
  SYMFUNC(FreeResource)
  SYMFUNC(FreeResourceByType)
  SYMFUNC(GetXIDList)
  SYMFUNC(GetXIDRange)
  SYMFUNC(LookupIDByType)
  SYMFUNC(LookupIDByClass)
  SYMFUNC(LegalNewID)
  SYMFUNC(SecurityLookupIDByClass)
  SYMFUNC(SecurityLookupIDByType)
  /* swaprep.c */
  SYMFUNC(CopySwap32Write)
  SYMFUNC(Swap32Write)
  SYMFUNC(SwapConnSetupInfo)
  SYMFUNC(SwapConnSetupPrefix)
  SYMFUNC(SwapShorts)
  SYMFUNC(SwapLongs)
  /* swapreq.c */
  SYMFUNC(SwapColorItem)
  /* tables.c */
  SYMVAR(EventSwapVector)
  /* window.c */
  SYMFUNC(ChangeWindowAttributes)
  SYMFUNC(CheckWindowOptionalNeed)
  SYMFUNC(CreateUnclippedWinSize)
  SYMFUNC(CreateWindow)
  SYMFUNC(FindWindowWithOptional)
  SYMFUNC(GravityTranslate)
  SYMFUNC(MakeWindowOptional)
  SYMFUNC(MapWindow)
  SYMFUNC(MoveWindowInStack)
  SYMFUNC(NotClippedByChildren)
  SYMFUNC(ResizeChildrenWinSize)
  SYMFUNC(SaveScreens)
  SYMFUNC(SendVisibilityNotify)
  SYMFUNC(SetWinSize)
  SYMFUNC(SetBorderSize)
  SYMFUNC(TraverseTree)
  SYMFUNC(UnmapWindow)
  SYMFUNC(WalkTree)
  SYMFUNC(WindowsRestructured)
  SYMVAR(deltaSaveUndersViewable)
  SYMVAR(numSaveUndersViewable)
  SYMVAR(savedScreenInfo)
  SYMVAR(screenIsSaved)

  /*os/ */
  /* access.c */
  SYMFUNC(LocalClient)
  /* util.c */
  SYMFUNC(Error)
  SYMFUNC(VErrorF)
  SYMFUNC(ErrorF)
  SYMFUNC(FatalError)
  SYMFUNC(Xstrdup)
  SYMVAR(Must_have_memory)
  /* xalloc.c */
  SYMFUNC(XNFalloc)
  SYMFUNC(XNFcalloc)
  SYMFUNC(XNFrealloc)
  SYMFUNC(Xalloc)
  SYMFUNC(Xcalloc)
  SYMFUNC(Xfree)
  SYMFUNC(Xrealloc)
  /* WaitFor.c */
  SYMFUNC(ScreenSaverTime)
  SYMFUNC(TimerFree)
  SYMFUNC(TimerSet)
  SYMFUNC(TimerCancel)
  /* io.c */
  SYMFUNC(WriteToClient)
  SYMFUNC(SetCriticalOutputPending)
  SYMVAR(FlushCallback)
  SYMVAR(ReplyCallback)
  SYMVAR(SkippedRequestsCallback)
  SYMFUNC(ResetCurrentRequest)
  /* connection.c */
  SYMFUNC(IgnoreClient)
  SYMFUNC(AttendClient)
  SYMFUNC(AddEnabledDevice)
  SYMFUNC(RemoveEnabledDevice)
  SYMVAR(GrabInProgress)
  /* utils.c */
  SYMFUNC(AdjustWaitForDelay)
  SYMVAR(noTestExtensions)

  /* devices.c */
  SYMFUNC(InitPointerDeviceStruct)
#ifdef XINPUT
  /* Xi */
  /* exevents.c */
  SYMFUNC(InitValuatorAxisStruct)
  SYMFUNC(InitProximityClassDeviceStruct)
  /* extinit.c */
  SYMFUNC(AssignTypeAndName)
#endif

#ifdef XFreeXDGA
  /* xf86DGA.c */
  SYMVAR(XDGAEventBase)
#endif

  /* libfont.a */
  SYMFUNC(GetGlyphs)
  SYMFUNC(QueryGlyphExtents)

  /* libXext.a */
  SYMFUNC(ClientSleepUntil)
  SYMVAR(ShmCompletionCode)
  SYMVAR(BadShmSegCode)
  SYMVAR(ShmSegType)

  /* librender.a */
#ifdef RENDER
  SYMFUNC(PictureInit)
  SYMFUNC(miPictureInit)
  SYMFUNC(miComputeCompositeRegion)
  SYMFUNC(miGlyphs)
  SYMFUNC(miCompositeRects)
  SYMVAR(PictureScreenPrivateIndex)
#endif

  { 0, 0 },

};
