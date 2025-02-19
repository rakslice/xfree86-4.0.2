/* $TOG: xkb.c /main/22 1997/06/10 06:53:48 kaleb $ */
/************************************************************
Copyright (c) 1993 by Silicon Graphics Computer Systems, Inc.

Permission to use, copy, modify, and distribute this
software and its documentation for any purpose and without
fee is hereby granted, provided that the above copyright
notice appear in all copies and that both that copyright
notice and this permission notice appear in supporting
documentation, and that the name of Silicon Graphics not be 
used in advertising or publicity pertaining to distribution 
of the software without specific prior written permission.
Silicon Graphics makes no representation about the suitability 
of this software for any purpose. It is provided "as is"
without any express or implied warranty.

SILICON GRAPHICS DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS 
SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY 
AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL SILICON
GRAPHICS BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL 
DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, 
DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE 
OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION  WITH
THE USE OR PERFORMANCE OF THIS SOFTWARE.

********************************************************/
/* $XFree86: xc/programs/Xserver/xkb/xkb.c,v 3.13 2000/04/04 19:25:22 dawes Exp $ */

#include <stdio.h>
#include "X.h"
#define	NEED_EVENTS
#define	NEED_REPLIES
#include "Xproto.h"
#include "misc.h"
#include "inputstr.h"
#define	XKBSRV_NEED_FILE_FUNCS
#include "XKBsrv.h"
#include "extnsionst.h"

#include "XI.h"

	int	XkbEventBase;
	int	XkbErrorBase;
	int	XkbReqCode;
	int	XkbKeyboardErrorCode;
Atom	xkbONE_LEVEL;
Atom	xkbTWO_LEVEL;
Atom	xkbKEYPAD;
CARD32	xkbDebugFlags = 0;
CARD32	xkbDebugCtrls = 0;

#ifndef XKB_SRV_UNSUPPORTED_XI_FEATURES
#define	XKB_SRV_UNSUPPORTED_XI_FEATURES	XkbXI_KeyboardsMask
#endif

unsigned XkbXIUnsupported= XKB_SRV_UNSUPPORTED_XI_FEATURES;

RESTYPE	RT_XKBCLIENT;

/***====================================================================***/

#define	CHK_DEVICE(d,sp,lf) {\
    int why;\
    d = (DeviceIntPtr)lf((sp),&why);\
    if  (!dev) {\
	client->errorValue = _XkbErrCode2(why,(sp));\
	return XkbKeyboardErrorCode;\
    }\
}

#define	CHK_KBD_DEVICE(d,sp) 	CHK_DEVICE(d,sp,_XkbLookupKeyboard)
#define	CHK_LED_DEVICE(d,sp) 	CHK_DEVICE(d,sp,_XkbLookupLedDevice)
#define	CHK_BELL_DEVICE(d,sp) 	CHK_DEVICE(d,sp,_XkbLookupBellDevice)
#define	CHK_ANY_DEVICE(d,sp) 	CHK_DEVICE(d,sp,_XkbLookupAnyDevice)

#define	CHK_ATOM_ONLY2(a,ev,er) {\
	if (((a)==None)||(!ValidAtom((a)))) {\
	    (ev)= (XID)(a);\
	    return er;\
	}\
}
#define	CHK_ATOM_ONLY(a) \
	CHK_ATOM_ONLY2(a,client->errorValue,BadAtom)

#define	CHK_ATOM_OR_NONE3(a,ev,er,ret) {\
	if (((a)!=None)&&(!ValidAtom((a)))) {\
	    (ev)= (XID)(a);\
	    (er)= BadAtom;\
	    return ret;\
	}\
}
#define	CHK_ATOM_OR_NONE2(a,ev,er) {\
	if (((a)!=None)&&(!ValidAtom((a)))) {\
	    (ev)= (XID)(a);\
	    return er;\
	}\
}
#define	CHK_ATOM_OR_NONE(a) \
	CHK_ATOM_OR_NONE2(a,client->errorValue,BadAtom)

#define	CHK_MASK_LEGAL3(err,mask,legal,ev,er,ret)	{\
	if ((mask)&(~(legal))) { \
	    (ev)= _XkbErrCode2((err),((mask)&(~(legal))));\
	    (er)= BadValue;\
	    return ret;\
	}\
}
#define	CHK_MASK_LEGAL2(err,mask,legal,ev,er)	{\
	if ((mask)&(~(legal))) { \
	    (ev)= _XkbErrCode2((err),((mask)&(~(legal))));\
	    return er;\
	}\
}
#define	CHK_MASK_LEGAL(err,mask,legal) \
	CHK_MASK_LEGAL2(err,mask,legal,client->errorValue,BadValue)

#define	CHK_MASK_MATCH(err,affect,value) {\
	if ((value)&(~(affect))) { \
	    client->errorValue= _XkbErrCode2((err),((value)&(~(affect))));\
	    return BadMatch;\
	}\
}
#define	CHK_MASK_OVERLAP(err,m1,m2) {\
	if ((m1)&(m2)) { \
	    client->errorValue= _XkbErrCode2((err),((m1)&(m2)));\
	    return BadMatch;\
	}\
}
#define	CHK_KEY_RANGE2(err,first,num,x,ev,er) {\
	if (((unsigned)(first)+(num)-1)>(x)->max_key_code) {\
	    (ev)=_XkbErrCode4(err,(first),(num),(x)->max_key_code);\
	    return er;\
	}\
	else if ( (first)<(x)->min_key_code ) {\
	    (ev)=_XkbErrCode3(err+1,(first),xkb->min_key_code);\
	    return er;\
	}\
}
#define	CHK_KEY_RANGE(err,first,num,x)  \
	CHK_KEY_RANGE2(err,first,num,x,client->errorValue,BadValue)

#define	CHK_REQ_KEY_RANGE2(err,first,num,r,ev,er) {\
	if (((unsigned)(first)+(num)-1)>(r)->maxKeyCode) {\
	    (ev)=_XkbErrCode4(err,(first),(num),(r)->maxKeyCode);\
	    return er;\
	}\
	else if ( (first)<(r)->minKeyCode ) {\
	    (ev)=_XkbErrCode3(err+1,(first),(r)->minKeyCode);\
	    return er;\
	}\
}
#define	CHK_REQ_KEY_RANGE(err,first,num,r)  \
	CHK_REQ_KEY_RANGE2(err,first,num,r,client->errorValue,BadValue)

/***====================================================================***/

int
#if NeedFunctionPrototypes
ProcXkbUseExtension(ClientPtr client)
#else
ProcXkbUseExtension(client)
    ClientPtr client;
#endif
{
    REQUEST(xkbUseExtensionReq);
    xkbUseExtensionReply	rep;
    register int n;
    int	supported;

    REQUEST_SIZE_MATCH(xkbUseExtensionReq);
    if (stuff->wantedMajor != XkbMajorVersion) {
	/* pre-release version 0.65 is compatible with 1.00 */
	supported= ((XkbMajorVersion==1)&&
		    (stuff->wantedMajor==0)&&(stuff->wantedMinor==65));
    }
    else supported = 1;

#ifdef XKB_SWAPPING_BUSTED
    if (client->swapped)
	supported= 0;
#endif

    if ((supported) && (!(client->xkbClientFlags&_XkbClientInitialized))) {
	client->xkbClientFlags= _XkbClientInitialized;
	client->vMajor= stuff->wantedMajor;
	client->vMinor= stuff->wantedMinor;
    }
    else if (xkbDebugFlags&0x1) {
	ErrorF("Rejecting client %d (0x%x) (wants %d.%02d, have %d.%02d)\n",
					client->index, client->clientAsMask,
					stuff->wantedMajor,stuff->wantedMinor,
					XkbMajorVersion,XkbMinorVersion);
    }
    rep.type = X_Reply;
    rep.supported = supported;
    rep.length = 0;
    rep.sequenceNumber = client->sequence;
    rep.serverMajor = XkbMajorVersion;
    rep.serverMinor = XkbMinorVersion;
    if ( client->swapped ) {
	swaps(&rep.sequenceNumber, n);
	swaps(&rep.serverMajor, n);
	swaps(&rep.serverMinor, n);
    }
    WriteToClient(client,SIZEOF(xkbUseExtensionReply), (char *)&rep);
    return client->noClientException;
}

/***====================================================================***/

int
#if NeedFunctionPrototypes
ProcXkbSelectEvents(ClientPtr client)
#else
ProcXkbSelectEvents(client)
    ClientPtr client;
#endif
{
    unsigned		legal;
    DeviceIntPtr 	dev;
    XkbInterestPtr	masks;
    REQUEST(xkbSelectEventsReq);

    REQUEST_AT_LEAST_SIZE(xkbSelectEventsReq);

    if (!(client->xkbClientFlags&_XkbClientInitialized))
	return BadAccess;

    CHK_ANY_DEVICE(dev,stuff->deviceSpec);

    if (((stuff->affectWhich&XkbMapNotifyMask)!=0)&&(stuff->affectMap)) {
	client->mapNotifyMask&= ~stuff->affectMap;
	client->mapNotifyMask|= (stuff->affectMap&stuff->map);
    }
    if (stuff->affectWhich&(~XkbMapNotifyMask)==0) 
	return client->noClientException;

    masks = XkbFindClientResource((DevicePtr)dev,client);
    if (!masks){
	XID id = FakeClientID(client->index);
	AddResource(id,RT_XKBCLIENT,dev);
	masks= XkbAddClientResource((DevicePtr)dev,client,id);
    }
    if (masks) {
	union {
	    CARD8	*c8;
	    CARD16	*c16;
	    CARD32	*c32;
	} from,to;
	register unsigned bit,ndx,maskLeft,dataLeft,size;

	from.c8= (CARD8 *)&stuff[1];
	dataLeft= (stuff->length*4)-SIZEOF(xkbSelectEventsReq);
	maskLeft= (stuff->affectWhich&(~XkbMapNotifyMask));
	for (ndx=0,bit=1; (maskLeft!=0); ndx++, bit<<=1) {
	    if ((bit&maskLeft)==0)
		continue;
	    maskLeft&= ~bit;
	    switch (ndx) {
		case XkbNewKeyboardNotify:
		    to.c16= &client->newKeyboardNotifyMask;
		    legal= XkbAllNewKeyboardEventsMask;
		    size= 2;
		    break;
		case XkbStateNotify:
		    to.c16= &masks->stateNotifyMask;
		    legal= XkbAllStateEventsMask;
		    size= 2;
		    break;
		case XkbControlsNotify:
		    to.c32= &masks->ctrlsNotifyMask;
		    legal= XkbAllControlEventsMask;
		    size= 4;
		    break;
		case XkbIndicatorStateNotify:
		    to.c32= &masks->iStateNotifyMask;
		    legal= XkbAllIndicatorEventsMask;
		    size= 4;
		    break;
		case XkbIndicatorMapNotify:
		    to.c32= &masks->iMapNotifyMask;
		    legal= XkbAllIndicatorEventsMask;
		    size= 4;
		    break;
		case XkbNamesNotify:
		    to.c16= &masks->namesNotifyMask;
		    legal= XkbAllNameEventsMask;
		    size= 2;
		    break;
		case XkbCompatMapNotify:
		    to.c8= &masks->compatNotifyMask;
		    legal= XkbAllCompatMapEventsMask;
		    size= 1;
		    break;
		case XkbBellNotify:
		    to.c8= &masks->bellNotifyMask;
		    legal= XkbAllBellEventsMask;
		    size= 1;
		    break;
		case XkbActionMessage:
		    to.c8= &masks->actionMessageMask;
		    legal= XkbAllActionMessagesMask;
		    size= 1;
		    break;
		case XkbAccessXNotify:
		    to.c16= &masks->accessXNotifyMask;
		    legal= XkbAllAccessXEventsMask;
		    size= 2;
		    break;
		case XkbExtensionDeviceNotify:
		    to.c16= &masks->extDevNotifyMask;
		    legal= XkbAllExtensionDeviceEventsMask;
		    size= 2;
		    break;
		default:
		    client->errorValue = _XkbErrCode2(33,bit);
		    return BadValue;
	    }

	    if (stuff->clear&bit) {
		if (size==2)		to.c16[0]= 0;
		else if (size==4)	to.c32[0]= 0;
		else			to.c8[0]=  0;
	    }
	    else if (stuff->selectAll&bit) {
		if (size==2)		to.c16[0]= ~0;
		else if (size==4)	to.c32[0]= ~0;
		else			to.c8[0]=  ~0;
	    }
	    else {
		if (dataLeft<(size*2))
		    return BadLength;
		if (size==2) {
		    CHK_MASK_MATCH(ndx,from.c16[0],from.c16[1]);
		    CHK_MASK_LEGAL(ndx,from.c16[0],legal);
		    to.c16[0]&= ~from.c16[0];
		    to.c16[0]|= (from.c16[0]&from.c16[1]);
		}
		else if (size==4) {
		    CHK_MASK_MATCH(ndx,from.c32[0],from.c32[1]);
		    CHK_MASK_LEGAL(ndx,from.c32[0],legal);
		    to.c32[0]&= ~from.c32[0];
		    to.c32[0]|= (from.c32[0]&from.c32[1]);
		}
		else  {
		    CHK_MASK_MATCH(ndx,from.c8[0],from.c8[1]);
		    CHK_MASK_LEGAL(ndx,from.c8[0],legal);
		    to.c8[0]&= ~from.c8[0];
		    to.c8[0]|= (from.c8[0]&from.c8[1]);
		    size= 2;
		}
		from.c8+= (size*2);
		dataLeft-= (size*2);
	    }
	}
	if (dataLeft>2) {
	    ErrorF("Extra data (%d bytes) after SelectEvents\n",dataLeft);
	    return BadLength;
	}
	return client->noClientException;
    }
    return BadAlloc;
}

/***====================================================================***/

int
#if NeedFunctionPrototypes
ProcXkbBell(ClientPtr client)
#else
ProcXkbBell(client)
    ClientPtr client;
#endif
{
    REQUEST(xkbBellReq);
    DeviceIntPtr dev;
    WindowPtr	 pWin;
    int base;
    int newPercent,oldPitch,oldDuration;
    pointer ctrl;

    REQUEST_SIZE_MATCH(xkbBellReq);

    if (!(client->xkbClientFlags&_XkbClientInitialized))
	return BadAccess;

    CHK_BELL_DEVICE(dev,stuff->deviceSpec);
    CHK_ATOM_OR_NONE(stuff->name);

    if ((stuff->forceSound)&&(stuff->eventOnly)) {
	client->errorValue=_XkbErrCode3(0x1,stuff->forceSound,stuff->eventOnly);
	return BadMatch;
    }
    if (stuff->percent < -100 || stuff->percent > 100) {
	client->errorValue = _XkbErrCode2(0x2,stuff->percent);
	return BadValue;
    }
    if (stuff->duration<-1) {
	client->errorValue = _XkbErrCode2(0x3,stuff->duration);
	return BadValue;
    }
    if (stuff->pitch<-1) {
	client->errorValue = _XkbErrCode2(0x4,stuff->pitch);
	return BadValue;
    }

    if (stuff->bellClass == XkbDfltXIClass) {
	if (dev->kbdfeed!=NULL)
	     stuff->bellClass= KbdFeedbackClass;
	else stuff->bellClass= BellFeedbackClass;
    }
    if (stuff->bellClass == KbdFeedbackClass) {
	KbdFeedbackPtr	k;
	if (stuff->bellID==XkbDfltXIId) 
	    k= dev->kbdfeed;
	else {
	    for (k=dev->kbdfeed; k; k=k->next) {
		if (k->ctrl.id == stuff->bellID)
		    break;
	    }
	}
	if (!k) {
	    client->errorValue= _XkbErrCode2(0x5,stuff->bellID);
	    return BadValue;
	}
	base = k->ctrl.bell;
	ctrl = (pointer) &(k->ctrl);
	oldPitch= k->ctrl.bell_pitch;
	oldDuration= k->ctrl.bell_duration;
	if (stuff->pitch!=0) {
	    if (stuff->pitch==-1)
		 k->ctrl.bell_pitch= defaultKeyboardControl.bell_pitch;
	    else k->ctrl.bell_pitch= stuff->pitch;
	}
	if (stuff->duration!=0) {
	    if (stuff->duration==-1)
		 k->ctrl.bell_duration= defaultKeyboardControl.bell_duration;
	    else k->ctrl.bell_duration= stuff->duration;
	}
    }
    else if (stuff->bellClass == BellFeedbackClass) {
	BellFeedbackPtr	b;
	if (stuff->bellID==XkbDfltXIId)
	    b= dev->bell;
	else {
	    for (b=dev->bell; b; b=b->next) {
		if (b->ctrl.id == stuff->bellID)
		    break;
	    }
	}
	if (!b) {
	    client->errorValue = _XkbErrCode2(0x6,stuff->bellID);
	    return BadValue;
	}
	base = b->ctrl.percent;
	ctrl = (pointer) &(b->ctrl);
	oldPitch= b->ctrl.pitch;
	oldDuration= b->ctrl.duration;
	if (stuff->pitch!=0) {
	    if (stuff->pitch==-1)
		 b->ctrl.pitch= defaultKeyboardControl.bell_pitch;
	    else b->ctrl.pitch= stuff->pitch;
	}
	if (stuff->duration!=0) {
	    if (stuff->duration==-1)
		 b->ctrl.duration= defaultKeyboardControl.bell_duration;
	    else b->ctrl.duration= stuff->duration;
	}
    }
    else {
	client->errorValue = _XkbErrCode2(0x7,stuff->bellClass);;
	return BadValue;
    }
    if (stuff->window!=None) {
	pWin= (WindowPtr)LookupIDByType(stuff->window,RT_WINDOW);
	if (pWin==NULL) {
	    client->errorValue= stuff->window;
	    return BadValue;
	}
    }
    else pWin= NULL;

    newPercent= (base*stuff->percent)/100;
    if (stuff->percent < 0)
         newPercent= base+newPercent;
    else newPercent= base-newPercent+stuff->percent;
    XkbHandleBell(stuff->forceSound, stuff->eventOnly,
				dev, newPercent, ctrl, stuff->bellClass, 
				stuff->name, pWin, client);
    if ((stuff->pitch!=0)||(stuff->duration!=0)) {
	if (stuff->bellClass == KbdFeedbackClass) {
	    KbdFeedbackPtr	k;
	    k= (KbdFeedbackPtr)ctrl;
	    if (stuff->pitch!=0)
		k->ctrl.bell_pitch= oldPitch;
	    if (stuff->duration!=0)
		k->ctrl.bell_duration= oldDuration;
	}
	else {
	    BellFeedbackPtr	b;
	    b= (BellFeedbackPtr)ctrl;
	    if (stuff->pitch!=0)
		b->ctrl.pitch= oldPitch;
	    if (stuff->duration!=0)
		b->ctrl.duration= oldDuration;
	}
    }
    return Success;
}

/***====================================================================***/

int
#if NeedFunctionPrototypes
ProcXkbGetState(ClientPtr client)
#else
ProcXkbGetState(client)
    ClientPtr client;
#endif
{
    REQUEST(xkbGetStateReq);
    DeviceIntPtr	dev;
    xkbGetStateReply	 rep;
    XkbStateRec		*xkb;

    REQUEST_SIZE_MATCH(xkbGetStateReq);

    if (!(client->xkbClientFlags&_XkbClientInitialized))
	return BadAccess;

    CHK_KBD_DEVICE(dev,stuff->deviceSpec);

    xkb= &dev->key->xkbInfo->state;
    bzero(&rep,sizeof(xkbGetStateReply));
    rep.type= X_Reply;
    rep.sequenceNumber= client->sequence;
    rep.length = 0;
    rep.deviceID = dev->id;
    rep.mods = dev->key->state&0xff;
    rep.baseMods = xkb->base_mods;
    rep.lockedMods = xkb->locked_mods;
    rep.latchedMods = xkb->latched_mods;
    rep.group = xkb->group;
    rep.baseGroup = xkb->base_group;
    rep.latchedGroup = xkb->latched_group;
    rep.lockedGroup = xkb->locked_group;
    rep.compatState = xkb->compat_state;
    rep.ptrBtnState = xkb->ptr_buttons;
    if (client->swapped) {
	register int n;
	swaps(&rep.sequenceNumber,n);
	swaps(&rep.ptrBtnState,n);
    }
    WriteToClient(client, SIZEOF(xkbGetStateReply), (char *)&rep);
    return client->noClientException;
}

/***====================================================================***/

int
#if NeedFunctionPrototypes
ProcXkbLatchLockState(ClientPtr client)
#else
ProcXkbLatchLockState(client)
    ClientPtr client;
#endif
{
    int status;
    DeviceIntPtr dev;
    XkbStateRec	oldState,*newState;
    CARD16 changed;

    REQUEST(xkbLatchLockStateReq);
    REQUEST_SIZE_MATCH(xkbLatchLockStateReq);

    if (!(client->xkbClientFlags&_XkbClientInitialized))
	return BadAccess;

    CHK_KBD_DEVICE(dev,stuff->deviceSpec);
    CHK_MASK_MATCH(0x01,stuff->affectModLocks,stuff->modLocks);
    CHK_MASK_MATCH(0x01,stuff->affectModLatches,stuff->modLatches);

    status = Success;
    oldState= dev->key->xkbInfo->state;
    newState= &dev->key->xkbInfo->state;
    if ( stuff->affectModLocks ) {
	newState->locked_mods&= ~stuff->affectModLocks;
	newState->locked_mods|= (stuff->affectModLocks&stuff->modLocks);
    }
    if (( status == Success ) && stuff->lockGroup )
	newState->locked_group = stuff->groupLock;
    if (( status == Success ) && stuff->affectModLatches )
	status=XkbLatchModifiers(dev,stuff->affectModLatches,stuff->modLatches);
    if (( status == Success ) && stuff->latchGroup )
	status=XkbLatchGroup(dev,stuff->groupLatch);

    if ( status != Success )
	return status;

    XkbComputeDerivedState(dev->key->xkbInfo);
    dev->key->state= XkbStateFieldFromRec(newState);

    changed = XkbStateChangedFlags(&oldState,newState);
    if (changed) {
	xkbStateNotify	sn;
	sn.keycode= 0;
	sn.eventType= 0;
	sn.requestMajor = XkbReqCode;
	sn.requestMinor = X_kbLatchLockState;
	sn.changed= changed;
	XkbSendStateNotify(dev,&sn);
	changed= XkbIndicatorsToUpdate(dev,changed,False);
	if (changed) {
	    XkbEventCauseRec	cause;
	    XkbSetCauseXkbReq(&cause,X_kbLatchLockState,client);
	    XkbUpdateIndicators(dev,changed,True,NULL,&cause);
	}
    }
    return client->noClientException;
}

/***====================================================================***/

int
#if NeedFunctionPrototypes
ProcXkbGetControls(ClientPtr client)
#else
ProcXkbGetControls(client)
    ClientPtr client;
#endif
{
    xkbGetControlsReply rep;
    XkbControlsPtr	xkb;
    DeviceIntPtr 	dev;
    register int 	n;

    REQUEST(xkbGetControlsReq);
    REQUEST_SIZE_MATCH(xkbGetControlsReq);

    if (!(client->xkbClientFlags&_XkbClientInitialized))
	return BadAccess;

    CHK_KBD_DEVICE(dev,stuff->deviceSpec);
    
    xkb = dev->key->xkbInfo->desc->ctrls;
    rep.type = X_Reply;
    rep.length = (SIZEOF(xkbGetControlsReply)-
		  SIZEOF(xGenericReply)) >> 2;
    rep.sequenceNumber = client->sequence;
    rep.deviceID = ((DeviceIntPtr)dev)->id;
    rep.numGroups = xkb->num_groups;
    rep.groupsWrap = xkb->groups_wrap;
    rep.internalMods = xkb->internal.mask;
    rep.ignoreLockMods = xkb->ignore_lock.mask;
    rep.internalRealMods = xkb->internal.real_mods;
    rep.ignoreLockRealMods = xkb->ignore_lock.real_mods;
    rep.internalVMods = xkb->internal.vmods;
    rep.ignoreLockVMods = xkb->ignore_lock.vmods;
    rep.enabledCtrls = xkb->enabled_ctrls;
    rep.repeatDelay = xkb->repeat_delay;
    rep.repeatInterval = xkb->repeat_interval;
    rep.slowKeysDelay = xkb->slow_keys_delay;
    rep.debounceDelay = xkb->debounce_delay;
    rep.mkDelay = xkb->mk_delay;
    rep.mkInterval = xkb->mk_interval;
    rep.mkTimeToMax = xkb->mk_time_to_max;
    rep.mkMaxSpeed = xkb->mk_max_speed;
    rep.mkCurve = xkb->mk_curve;
    rep.mkDfltBtn = xkb->mk_dflt_btn;
    rep.axTimeout = xkb->ax_timeout;
    rep.axtCtrlsMask = xkb->axt_ctrls_mask;
    rep.axtCtrlsValues = xkb->axt_ctrls_values;
    rep.axtOptsMask = xkb->axt_opts_mask;
    rep.axtOptsValues = xkb->axt_opts_values;
    rep.axOptions = xkb->ax_options;
    memcpy(rep.perKeyRepeat,xkb->per_key_repeat,XkbPerKeyBitArraySize);
    if (client->swapped) {
    	swaps(&rep.sequenceNumber, n);
	swapl(&rep.length,n);
	swaps(&rep.internalVMods, n);
	swaps(&rep.ignoreLockVMods, n);
	swapl(&rep.enabledCtrls, n);
	swaps(&rep.repeatDelay, n);
	swaps(&rep.repeatInterval, n);
	swaps(&rep.slowKeysDelay, n);
	swaps(&rep.debounceDelay, n);
	swaps(&rep.mkDelay, n);
	swaps(&rep.mkInterval, n);
	swaps(&rep.mkTimeToMax, n);
	swaps(&rep.mkMaxSpeed, n);
	swaps(&rep.mkCurve, n);
	swaps(&rep.axTimeout, n);
	swapl(&rep.axtCtrlsMask, n);
	swapl(&rep.axtCtrlsValues, n);
	swaps(&rep.axtOptsMask, n);
	swaps(&rep.axtOptsValues, n);
	swaps(&rep.axOptions, n);
    }
    WriteToClient(client, SIZEOF(xkbGetControlsReply), (char *)&rep);
    return(client->noClientException);
}

int
#if NeedFunctionPrototypes
ProcXkbSetControls(ClientPtr client)
#else
ProcXkbSetControls(client)
    ClientPtr client;
#endif
{
    DeviceIntPtr 	dev;
    XkbSrvInfoPtr	xkbi;
    XkbControlsPtr	ctrl;
    XkbControlsRec	new,old;
    xkbControlsNotify	cn;
    XkbEventCauseRec	cause;
    XkbSrvLedInfoPtr	sli;

    REQUEST(xkbSetControlsReq);
    REQUEST_SIZE_MATCH(xkbSetControlsReq);

    if (!(client->xkbClientFlags&_XkbClientInitialized))
	return BadAccess;

    CHK_KBD_DEVICE(dev,stuff->deviceSpec);
    CHK_MASK_LEGAL(0x01,stuff->changeCtrls,XkbAllControlsMask);

    xkbi = dev->key->xkbInfo;
    ctrl = xkbi->desc->ctrls;
    new = *ctrl;
    XkbSetCauseXkbReq(&cause,X_kbSetControls,client);
    if (stuff->changeCtrls&XkbInternalModsMask) {
	CHK_MASK_MATCH(0x02,stuff->affectInternalMods,stuff->internalMods);
	CHK_MASK_MATCH(0x03,stuff->affectInternalVMods,stuff->internalVMods);
	new.internal.real_mods&=~stuff->affectInternalMods;
	new.internal.real_mods|=(stuff->affectInternalMods&stuff->internalMods);
	new.internal.vmods&=~stuff->affectInternalVMods;
	new.internal.vmods|= (stuff->affectInternalVMods&stuff->internalVMods);
	new.internal.mask= new.internal.real_mods|
	      XkbMaskForVMask(xkbi->desc,new.internal.vmods);
    }
    if (stuff->changeCtrls&XkbIgnoreLockModsMask) {
	CHK_MASK_MATCH(0x4,stuff->affectIgnoreLockMods,stuff->ignoreLockMods);
	CHK_MASK_MATCH(0x5,stuff->affectIgnoreLockVMods,stuff->ignoreLockVMods);
	new.ignore_lock.real_mods&=~stuff->affectIgnoreLockMods;
	new.ignore_lock.real_mods|=
	      (stuff->affectIgnoreLockMods&stuff->ignoreLockMods);
	new.ignore_lock.vmods&= ~stuff->affectIgnoreLockVMods;
	new.ignore_lock.vmods|=
	      (stuff->affectIgnoreLockVMods&stuff->ignoreLockVMods);
	new.ignore_lock.mask= new.ignore_lock.real_mods|
	      XkbMaskForVMask(xkbi->desc,new.ignore_lock.vmods);
    }
    CHK_MASK_MATCH(0x06,stuff->affectEnabledCtrls,stuff->enabledCtrls);
    if (stuff->affectEnabledCtrls) {
	CHK_MASK_LEGAL(0x07,stuff->affectEnabledCtrls,XkbAllBooleanCtrlsMask);
	new.enabled_ctrls&= ~stuff->affectEnabledCtrls;
	new.enabled_ctrls|= (stuff->affectEnabledCtrls&stuff->enabledCtrls);
    }
    if (stuff->changeCtrls&XkbRepeatKeysMask) {
	if ((stuff->repeatDelay<1)||(stuff->repeatInterval<1)) {
	   client->errorValue = _XkbErrCode3(0x08,stuff->repeatDelay,
							stuff->repeatInterval);
	   return BadValue;
	}
	new.repeat_delay = stuff->repeatDelay;
	new.repeat_interval = stuff->repeatInterval;
    }
    if (stuff->changeCtrls&XkbSlowKeysMask) {
	if (stuff->slowKeysDelay<1) {
	    client->errorValue = _XkbErrCode2(0x09,stuff->slowKeysDelay);
	    return BadValue;
	}
	new.slow_keys_delay = stuff->slowKeysDelay;
    }
    if (stuff->changeCtrls&XkbBounceKeysMask) {
	if (stuff->debounceDelay<1) {
	    client->errorValue = _XkbErrCode2(0x0A,stuff->debounceDelay);
	    return BadValue;
	}
	new.debounce_delay = stuff->debounceDelay;
    }
    if (stuff->changeCtrls&XkbMouseKeysMask) {
	if (stuff->mkDfltBtn>XkbMaxMouseKeysBtn) {
	    client->errorValue = _XkbErrCode2(0x0B,stuff->mkDfltBtn);
	    return BadValue;
	}
	new.mk_dflt_btn = stuff->mkDfltBtn;
    }
    if (stuff->changeCtrls&XkbMouseKeysAccelMask) {
	if ((stuff->mkDelay<1) || (stuff->mkInterval<1) ||
	    (stuff->mkTimeToMax<1) || (stuff->mkMaxSpeed<1)||
	    (stuff->mkCurve<-1000)) {
	    client->errorValue = _XkbErrCode2(0x0C,0);
	    return BadValue;
	}
	new.mk_delay = stuff->mkDelay;
	new.mk_interval = stuff->mkInterval;
	new.mk_time_to_max = stuff->mkTimeToMax;
	new.mk_max_speed = stuff->mkMaxSpeed;
	new.mk_curve = stuff->mkCurve;
	AccessXComputeCurveFactor(xkbi,&new);
    }
    if (stuff->changeCtrls&XkbGroupsWrapMask) {
	unsigned act,num;
	act= XkbOutOfRangeGroupAction(stuff->groupsWrap);
	switch (act) {
	    case XkbRedirectIntoRange:
		num= XkbOutOfRangeGroupNumber(stuff->groupsWrap);
		if (num>=new.num_groups) {
		    client->errorValue= _XkbErrCode3(0x0D,new.num_groups,num);
		    return BadValue;
		}
	    case XkbWrapIntoRange:
	    case XkbClampIntoRange:
		break;
	    default:
		client->errorValue= _XkbErrCode2(0x0E,act);
		return BadValue;
	}
	new.groups_wrap= stuff->groupsWrap;
    }
    CHK_MASK_LEGAL(0x0F,stuff->axOptions,XkbAX_AllOptionsMask);
    if (stuff->changeCtrls&XkbAccessXKeysMask)
	new.ax_options = stuff->axOptions&XkbAX_AllOptionsMask;
    else {
	if (stuff->changeCtrls&XkbStickyKeysMask) {
	   new.ax_options&= ~XkbAX_SKOptionsMask;
	   new.ax_options|= stuff->axOptions&XkbAX_SKOptionsMask;
	}
	if (stuff->changeCtrls&XkbAccessXFeedbackMask) {
	   new.ax_options&= ~XkbAX_FBOptionsMask;
	   new.ax_options|= stuff->axOptions&XkbAX_FBOptionsMask;
	}
    }

    if (stuff->changeCtrls&XkbAccessXTimeoutMask) {
	if (stuff->axTimeout<1) {
	    client->errorValue = _XkbErrCode2(0x10,stuff->axTimeout);
	    return BadValue;
	}
	CHK_MASK_MATCH(0x11,stuff->axtCtrlsMask,stuff->axtCtrlsValues);
	CHK_MASK_LEGAL(0x12,stuff->axtCtrlsMask,XkbAllBooleanCtrlsMask);
	CHK_MASK_MATCH(0x13,stuff->axtOptsMask,stuff->axtOptsValues);
	CHK_MASK_LEGAL(0x14,stuff->axtOptsMask,XkbAX_AllOptionsMask);
	new.ax_timeout = stuff->axTimeout;
	new.axt_ctrls_mask = stuff->axtCtrlsMask;
	new.axt_ctrls_values = (stuff->axtCtrlsValues&stuff->axtCtrlsMask);
	new.axt_opts_mask = stuff->axtOptsMask;
	new.axt_opts_values= (stuff->axtOptsValues&stuff->axtOptsMask);
    }
    if (stuff->changeCtrls&XkbPerKeyRepeatMask) {
	memcpy(new.per_key_repeat,stuff->perKeyRepeat,XkbPerKeyBitArraySize);
    }
    old= *ctrl;
    *ctrl= new;
    XkbDDXChangeControls(dev,&old,ctrl);
    if (XkbComputeControlsNotify(dev,&old,ctrl,&cn,False)) {
	cn.keycode= 0;
	cn.eventType = 0;
	cn.requestMajor = XkbReqCode;
	cn.requestMinor = X_kbSetControls;
	XkbSendControlsNotify(dev,&cn);
    }
    if ((sli= XkbFindSrvLedInfo(dev,XkbDfltXIClass,XkbDfltXIId,0))!=NULL)
	XkbUpdateIndicators(dev,sli->usesControls,True,NULL,&cause);
#ifndef NO_CLEAR_LATCHES_FOR_STICKY_KEYS_OFF
    /* If sticky keys were disabled, clear all locks and latches */
    if ((old.enabled_ctrls&XkbStickyKeysMask)&&
	(!(ctrl->enabled_ctrls&XkbStickyKeysMask))) {
	XkbClearAllLatchesAndLocks(dev,xkbi,True,&cause);
    }
#endif
    return client->noClientException;
}

int
#if NeedFunctionPrototypes
XkbSetRepeatRate(DeviceIntPtr dev,int timeout,int interval,int major,int minor)
#else
XkbSetRepeatRate(dev,timeout,interval,major,minor)
    DeviceIntPtr	dev;
    int			timeout;
    int			interval;
    int			major;
    int			minor;
#endif
{
int	changed= 0;
XkbControlsRec old,*xkb;

    if ((!dev)||(!dev->key)||(!dev->key->xkbInfo))
	return 0;
    xkb= dev->key->xkbInfo->desc->ctrls;
    old= *xkb;
    if ((timeout!=0) && (xkb->repeat_delay!=timeout)) {
	xkb->repeat_delay= timeout;
	changed++;
    }
    if ((interval!=0) && (xkb->repeat_interval!=interval)) {
	xkb->repeat_interval= interval;
	changed++;
    }
    if (changed) {
	xkbControlsNotify	cn;
	XkbDDXChangeControls(dev,&old,xkb);
	if (XkbComputeControlsNotify(dev,&old,xkb,&cn,False)) {
	    cn.keycode= 0;
	    cn.eventType = 0;
	    cn.requestMajor = major;
	    cn.requestMinor = minor;
	    XkbSendControlsNotify(dev,&cn);
	}
    }
    return 1;
}

int
#if NeedFunctionPrototypes
XkbGetRepeatRate(DeviceIntPtr dev,int *timeout,int *interval)
#else
XkbGetRepeatRate(dev,timeout,interval)
    DeviceIntPtr	dev;
    int	*		timeout;
    int	*		interval;
#endif
{
XkbControlsPtr	xkb;

    if ((!dev)||(!dev->key)||(!dev->key->xkbInfo))
	return 0;
    xkb= dev->key->xkbInfo->desc->ctrls;
    if (timeout)	*timeout= xkb->repeat_delay;
    if (interval)	*interval= xkb->repeat_interval;
    return 1;
}

/***====================================================================***/

static int
#if NeedFunctionPrototypes
XkbSizeKeyTypes(XkbDescPtr xkb,xkbGetMapReply *rep)
#else
XkbSizeKeyTypes(xkb,rep)
    XkbDescPtr 		xkb;
    xkbGetMapReply *	rep;
#endif
{
    XkbKeyTypeRec 	*type;
    unsigned		i,len;

    len= 0;
    if (((rep->present&XkbKeyTypesMask)==0)||(rep->nTypes<1)||
	(!xkb)||(!xkb->map)||(!xkb->map->types)) {
	rep->present&= ~XkbKeyTypesMask;
	rep->firstType= rep->nTypes= 0;
	return 0;
    }
    type= &xkb->map->types[rep->firstType];
    for (i=0;i<rep->nTypes;i++,type++){
	len+= SIZEOF(xkbKeyTypeWireDesc);
	if (type->map_count>0) {
	    len+= (type->map_count*SIZEOF(xkbKTMapEntryWireDesc));
	    if (type->preserve)
		len+= (type->map_count*SIZEOF(xkbModsWireDesc));
	}
    }
    return len;
}

static char *
#if NeedFunctionPrototypes
XkbWriteKeyTypes(	XkbDescPtr		xkb,
			xkbGetMapReply *	rep,
			char *			buf,
			ClientPtr 		client)
#else
XkbWriteKeyTypes(xkb,rep,buf,client)
    XkbDescPtr		xkb;
    xkbGetMapReply *	rep;
    char *		buf;
    ClientPtr 		client;
#endif
{
    XkbKeyTypePtr	type;
    unsigned		i;
    xkbKeyTypeWireDesc *wire;

    type= &xkb->map->types[rep->firstType];
    for (i=0;i<rep->nTypes;i++,type++) {
	register unsigned n;
	wire= (xkbKeyTypeWireDesc *)buf;
	wire->mask = type->mods.mask;
	wire->realMods = type->mods.real_mods;
	wire->virtualMods = type->mods.vmods;
	wire->numLevels = type->num_levels;
	wire->nMapEntries = type->map_count;
	wire->preserve = (type->preserve!=NULL);
	if (client->swapped) {
	    register int n;
	    swaps(&wire->virtualMods,n);
	}	

	buf= (char *)&wire[1];
	if (wire->nMapEntries>0) {
	    xkbKTMapEntryWireDesc *	wire;
	    XkbKTMapEntryPtr		entry;
	    wire= (xkbKTMapEntryWireDesc *)buf;
	    entry= type->map;
	    for (n=0;n<type->map_count;n++,wire++,entry++) {
		wire->active= entry->active;
		wire->mask= entry->mods.mask;
		wire->level= entry->level;
		wire->realMods= entry->mods.real_mods;
		wire->virtualMods= entry->mods.vmods;
		if (client->swapped) {
		    register int n;
		    swaps(&wire->virtualMods,n);
		}
	    }
	    buf= (char *)wire;
	    if (type->preserve!=NULL) {
		xkbModsWireDesc *	pwire;
		XkbModsPtr		preserve;
		pwire= (xkbModsWireDesc *)buf;
		preserve= type->preserve;
		for (n=0;n<type->map_count;n++,pwire++,preserve++) {
		    pwire->mask= preserve->mask;
		    pwire->realMods= preserve->real_mods;
		    pwire->virtualMods= preserve->vmods;
		    if (client->swapped) {
			register int n;
			swaps(&pwire->virtualMods,n);
		    }
		}
		buf= (char *)pwire;
	    }
	}
    }
    return buf;
}

static int
#if NeedFunctionPrototypes
XkbSizeKeySyms(XkbDescPtr xkb,xkbGetMapReply *rep)
#else
XkbSizeKeySyms(xkb,rep)
    XkbDescPtr		xkb;
    xkbGetMapReply *	rep;
#endif
{
    XkbSymMapPtr	symMap;
    unsigned		i,len;
    unsigned		nSyms,nSymsThisKey;

    if (((rep->present&XkbKeySymsMask)==0)||(rep->nKeySyms<1)||
	(!xkb)||(!xkb->map)||(!xkb->map->key_sym_map)) {
	rep->present&= ~XkbKeySymsMask;
	rep->firstKeySym= rep->nKeySyms= 0;
	rep->totalSyms= 0;
	return 0;
    }
    len= rep->nKeySyms*SIZEOF(xkbSymMapWireDesc);
    symMap = &xkb->map->key_sym_map[rep->firstKeySym];
    for (i=nSyms=0;i<rep->nKeySyms;i++,symMap++) {
	if (symMap->offset!=0) {
	    nSymsThisKey= XkbNumGroups(symMap->group_info)*symMap->width;
	    nSyms+= nSymsThisKey;
	}
    }
    len+= nSyms*4;
    rep->totalSyms= nSyms;
    return len;
}

static int
#if NeedFunctionPrototypes
XkbSizeVirtualMods(XkbDescPtr xkb,xkbGetMapReply *rep)
#else
XkbSizeVirtualMods(xkb,rep)
    XkbDescPtr		xkb;
    xkbGetMapReply *	rep;
#endif
{
register unsigned i,nMods,bit;

    if (((rep->present&XkbVirtualModsMask)==0)||(rep->virtualMods==0)||
	(!xkb)||(!xkb->server)) {
	rep->present&= ~XkbVirtualModsMask;
	rep->virtualMods= 0;
	return 0;
    }
    for (i=nMods=0,bit=1;i<XkbNumVirtualMods;i++,bit<<=1) {
        if (rep->virtualMods&bit)
	    nMods++;
    }
    return XkbPaddedSize(nMods);
}

static char *
#if NeedFunctionPrototypes
XkbWriteKeySyms(XkbDescPtr xkb,xkbGetMapReply *rep,char *buf,ClientPtr client)
#else
XkbWriteKeySyms(xkb,rep,buf,client)
    XkbDescPtr		xkb;
    xkbGetMapReply *	rep;
    char *		buf;
    ClientPtr 		client;
#endif
{
register KeySym *	pSym;
XkbSymMapPtr		symMap;
xkbSymMapWireDesc *	outMap;
register unsigned	i;

    symMap = &xkb->map->key_sym_map[rep->firstKeySym];
    for (i=0;i<rep->nKeySyms;i++,symMap++) {
	outMap = (xkbSymMapWireDesc *)buf;
	outMap->ktIndex[0] = symMap->kt_index[0];
	outMap->ktIndex[1] = symMap->kt_index[1];
	outMap->ktIndex[2] = symMap->kt_index[2];
	outMap->ktIndex[3] = symMap->kt_index[3];
	outMap->groupInfo = symMap->group_info;
	outMap->width= symMap->width;
	outMap->nSyms = symMap->width*XkbNumGroups(symMap->group_info);
	buf= (char *)&outMap[1];
	if (outMap->nSyms==0)
	    continue;

	pSym = &xkb->map->syms[symMap->offset];
	memcpy((char *)buf,(char *)pSym,outMap->nSyms*4);
	if (client->swapped) {
	    register int n,nSyms= outMap->nSyms;
	    swaps(&outMap->nSyms,n);
	    while (nSyms-->0) {
		swapl(buf,n);
		buf+= 4;
	    }
	}
	else buf+= outMap->nSyms*4;
    }
    return buf;
}

static int
#if NeedFunctionPrototypes
XkbSizeKeyActions(XkbDescPtr xkb,xkbGetMapReply *rep)
#else
XkbSizeKeyActions(xkb,rep)
    XkbDescPtr		xkb;
    xkbGetMapReply *	rep;
#endif
{
    unsigned		i,len,nActs;
    register KeyCode	firstKey;

    if (((rep->present&XkbKeyActionsMask)==0)||(rep->nKeyActs<1)||
	(!xkb)||(!xkb->server)||(!xkb->server->key_acts)) {
	rep->present&= ~XkbKeyActionsMask;
	rep->firstKeyAct= rep->nKeyActs= 0;
	rep->totalActs= 0;
	return 0;
    }
    firstKey= rep->firstKeyAct;
    for (nActs=i=0;i<rep->nKeyActs;i++) {
	if (xkb->server->key_acts[i+firstKey]!=0)
	    nActs+= XkbKeyNumActions(xkb,i+firstKey);
    }
    len= XkbPaddedSize(rep->nKeyActs)+(nActs*SIZEOF(xkbActionWireDesc));
    rep->totalActs= nActs;
    return len;
}

static char *
#if NeedFunctionPrototypes
XkbWriteKeyActions(XkbDescPtr xkb,xkbGetMapReply *rep,char *buf,
							ClientPtr client)
#else
XkbWriteKeyActions(xkb,rep,buf,client)
    XkbDescPtr		xkb;
    xkbGetMapReply *	rep;
    char *		buf;
    ClientPtr		client;
#endif
{
    unsigned		i;
    CARD8 *		numDesc;
    XkbAnyAction *	actDesc;

    numDesc = (CARD8 *)buf;
    for (i=0;i<rep->nKeyActs;i++) {
	if (xkb->server->key_acts[i+rep->firstKeyAct]==0)
	     numDesc[i] = 0;
	else numDesc[i] = XkbKeyNumActions(xkb,(i+rep->firstKeyAct));
    }
    buf+= XkbPaddedSize(rep->nKeyActs);

    actDesc = (XkbAnyAction *)buf;
    for (i=0;i<rep->nKeyActs;i++) {
	if (xkb->server->key_acts[i+rep->firstKeyAct]!=0) {
	    unsigned int num;
	    num = XkbKeyNumActions(xkb,(i+rep->firstKeyAct));
	    memcpy((char *)actDesc,
		   (char*)XkbKeyActionsPtr(xkb,(i+rep->firstKeyAct)),
		   num*SIZEOF(xkbActionWireDesc));
	    actDesc+= num;
	}
    }
    buf = (char *)actDesc;
    return buf;
}

static int
#if NeedFunctionPrototypes
XkbSizeKeyBehaviors(XkbDescPtr xkb,xkbGetMapReply *rep)
#else
XkbSizeKeyBehaviors(xkb,rep)
    XkbDescPtr		xkb;
    xkbGetMapReply *	rep;
#endif
{
    unsigned		i,len,nBhvr;
    XkbBehavior *	bhv;

    if (((rep->present&XkbKeyBehaviorsMask)==0)||(rep->nKeyBehaviors<1)||
	(!xkb)||(!xkb->server)||(!xkb->server->behaviors)) {
	rep->present&= ~XkbKeyBehaviorsMask;
	rep->firstKeyBehavior= rep->nKeyBehaviors= 0;
	rep->totalKeyBehaviors= 0;
	return 0;
    }
    bhv= &xkb->server->behaviors[rep->firstKeyBehavior];
    for (nBhvr=i=0;i<rep->nKeyBehaviors;i++,bhv++) {
	if (bhv->type!=XkbKB_Default)
	    nBhvr++;
    }
    len= nBhvr*SIZEOF(xkbBehaviorWireDesc);
    rep->totalKeyBehaviors= nBhvr;
    return len;
}

static char *
#if NeedFunctionPrototypes
XkbWriteKeyBehaviors(XkbDescPtr xkb,xkbGetMapReply *rep,char *buf,
							ClientPtr client)
#else
XkbWriteKeyBehaviors(xkb,rep,buf,client)
    XkbDescRec		*xkb;
    xkbGetMapReply	*rep;
    char		*buf;
    ClientPtr		client;
#endif
{
    unsigned		i;
    xkbBehaviorWireDesc	*wire;
    XkbBehavior		*pBhvr;

    wire = (xkbBehaviorWireDesc *)buf;
    pBhvr= &xkb->server->behaviors[rep->firstKeyBehavior];
    for (i=0;i<rep->nKeyBehaviors;i++,pBhvr++) {
	if (pBhvr->type!=XkbKB_Default) {
	    wire->key=  i+rep->firstKeyBehavior;
	    wire->type= pBhvr->type;
	    wire->data= pBhvr->data;
	    wire++;
	}
    }
    buf = (char *)wire;
    return buf;
}

static int
#if NeedFunctionPrototypes
XkbSizeExplicit(XkbDescPtr xkb,xkbGetMapReply *rep)
#else
XkbSizeExplicit(xkb,rep)
    XkbDescPtr		xkb;
    xkbGetMapReply *	rep;
#endif
{
    unsigned	i,len,nRtrn;

    if (((rep->present&XkbExplicitComponentsMask)==0)||(rep->nKeyExplicit<1)||
	(!xkb)||(!xkb->server)||(!xkb->server->explicit)) {
	rep->present&= ~XkbExplicitComponentsMask;
	rep->firstKeyExplicit= rep->nKeyExplicit= 0;
	rep->totalKeyExplicit= 0;
	return 0;
    }
    for (nRtrn=i=0;i<rep->nKeyExplicit;i++) {
	if (xkb->server->explicit[i+rep->firstKeyExplicit]!=0)
	    nRtrn++;
    }
    rep->totalKeyExplicit= nRtrn;
    len= XkbPaddedSize(nRtrn*2); /* two bytes per non-zero explicit component */
    return len;
}

static char *
#if NeedFunctionPrototypes
XkbWriteExplicit(XkbDescPtr xkb,xkbGetMapReply *rep,char *buf,ClientPtr client)
#else
XkbWriteExplicit(xkb,rep,buf,client)
    XkbDescPtr		 xkb;
    xkbGetMapReply	*rep;
    char		*buf;
    ClientPtr		client;
#endif
{
unsigned	i;
char *		start;
unsigned char *	pExp;

    start= buf;
    pExp= &xkb->server->explicit[rep->firstKeyExplicit];
    for (i=0;i<rep->nKeyExplicit;i++,pExp++) {
	if (*pExp!=0) {
	    *buf++= i+rep->firstKeyExplicit;
	    *buf++= *pExp;
	}
    }
    i= XkbPaddedSize(buf-start)-(buf-start); /* pad to word boundary */
    return buf+i;
}

static int
#if NeedFunctionPrototypes
XkbSizeModifierMap(XkbDescPtr xkb,xkbGetMapReply *rep)
#else
XkbSizeModifierMap(xkb,rep)
    XkbDescPtr		xkb;
    xkbGetMapReply *	rep;
#endif
{
    unsigned	i,len,nRtrn;

    if (((rep->present&XkbModifierMapMask)==0)||(rep->nModMapKeys<1)||
	(!xkb)||(!xkb->map)||(!xkb->map->modmap)) {
	rep->present&= ~XkbModifierMapMask;
	rep->firstModMapKey= rep->nModMapKeys= 0;
	rep->totalModMapKeys= 0;
	return 0;
    }
    for (nRtrn=i=0;i<rep->nModMapKeys;i++) {
	if (xkb->map->modmap[i+rep->firstModMapKey]!=0)
	    nRtrn++;
    }
    rep->totalModMapKeys= nRtrn;
    len= XkbPaddedSize(nRtrn*2); /* two bytes per non-zero modmap component */
    return len;
}

static char *
#if NeedFunctionPrototypes
XkbWriteModifierMap(XkbDescPtr xkb,xkbGetMapReply *rep,char *buf,
							ClientPtr client)
#else
XkbWriteModifierMap(xkb,rep,buf,client)
    XkbDescPtr		 xkb;
    xkbGetMapReply	*rep;
    char		*buf;
    ClientPtr		client;
#endif
{
unsigned	i;
char *		start;
unsigned char *	pMap;

    start= buf;
    pMap= &xkb->map->modmap[rep->firstModMapKey];
    for (i=0;i<rep->nModMapKeys;i++,pMap++) {
	if (*pMap!=0) {
	    *buf++= i+rep->firstModMapKey;
	    *buf++= *pMap;
	}
    }
    i= XkbPaddedSize(buf-start)-(buf-start); /* pad to word boundary */
    return buf+i;
}

static int
#if NeedFunctionPrototypes
XkbSizeVirtualModMap(XkbDescPtr xkb,xkbGetMapReply *rep)
#else
XkbSizeVirtualModMap(xkb,rep)
    XkbDescPtr		xkb;
    xkbGetMapReply *	rep;
#endif
{
    unsigned	i,len,nRtrn;

    if (((rep->present&XkbVirtualModMapMask)==0)||(rep->nVModMapKeys<1)||
	(!xkb)||(!xkb->server)||(!xkb->server->vmodmap)) {
	rep->present&= ~XkbVirtualModMapMask;
	rep->firstVModMapKey= rep->nVModMapKeys= 0;
	rep->totalVModMapKeys= 0;
	return 0;
    }
    for (nRtrn=i=0;i<rep->nVModMapKeys;i++) {
	if (xkb->server->vmodmap[i+rep->firstVModMapKey]!=0)
	    nRtrn++;
    }
    rep->totalVModMapKeys= nRtrn;
    len= nRtrn*SIZEOF(xkbVModMapWireDesc);
    return len;
}

static char *
#if NeedFunctionPrototypes
XkbWriteVirtualModMap(XkbDescPtr xkb,xkbGetMapReply *rep,char *buf,
							ClientPtr client)
#else
XkbWriteVirtualModMap(xkb,rep,buf,client)
    XkbDescPtr		 xkb;
    xkbGetMapReply	*rep;
    char		*buf;
    ClientPtr		client;
#endif
{
unsigned		i;
xkbVModMapWireDesc *	wire;
unsigned short *	pMap;

    wire= (xkbVModMapWireDesc *)buf;
    pMap= &xkb->server->vmodmap[rep->firstVModMapKey];
    for (i=0;i<rep->nVModMapKeys;i++,pMap++) {
	if (*pMap!=0) {
	    wire->key= i+rep->firstVModMapKey;
	    wire->vmods= *pMap;
	    wire++;
	}
    }
    return (char *)wire;
}

static Status
#if NeedFunctionPrototypes
XkbComputeGetMapReplySize(XkbDescPtr xkb,xkbGetMapReply *rep)
#else
XkbComputeGetMapReplySize(xkb,rep)
    XkbDescPtr		xkb;
    xkbGetMapReply *	rep;
#endif
{
int	len;

    rep->minKeyCode= xkb->min_key_code;
    rep->maxKeyCode= xkb->max_key_code;
    len= XkbSizeKeyTypes(xkb,rep);
    len+= XkbSizeKeySyms(xkb,rep);
    len+= XkbSizeKeyActions(xkb,rep);
    len+= XkbSizeKeyBehaviors(xkb,rep);
    len+= XkbSizeVirtualMods(xkb,rep);
    len+= XkbSizeExplicit(xkb,rep);
    len+= XkbSizeModifierMap(xkb,rep);
    len+= XkbSizeVirtualModMap(xkb,rep);
    rep->length+= (len/4);
    return Success;
}

static int
#if NeedFunctionPrototypes
XkbSendMap(ClientPtr client,XkbDescPtr xkb,xkbGetMapReply *rep)
#else
XkbSendMap(client,xkb,rep)
    ClientPtr		client;
    XkbDescPtr		xkb;
    xkbGetMapReply *	rep;
#endif
{
unsigned	i,len;
char		*desc,*start;

    len= (rep->length*4)-(SIZEOF(xkbGetMapReply)-SIZEOF(xGenericReply));
    start= desc= (char *)ALLOCATE_LOCAL(len);
    if (!start)
	return BadAlloc;
    if ( rep->nTypes>0 )
	desc = XkbWriteKeyTypes(xkb,rep,desc,client);
    if ( rep->nKeySyms>0 )
	desc = XkbWriteKeySyms(xkb,rep,desc,client);
    if ( rep->nKeyActs>0 )
	desc = XkbWriteKeyActions(xkb,rep,desc,client);
    if ( rep->totalKeyBehaviors>0 )
	desc = XkbWriteKeyBehaviors(xkb,rep,desc,client);
    if ( rep->virtualMods ) {
	register int sz,bit;
	for (i=sz=0,bit=1;i<XkbNumVirtualMods;i++,bit<<=1) {
	    if (rep->virtualMods&bit) {
		desc[sz++]= xkb->server->vmods[i];
	    }
	}
	desc+= XkbPaddedSize(sz);
    }
    if ( rep->totalKeyExplicit>0 )
	desc= XkbWriteExplicit(xkb,rep,desc,client);
    if ( rep->totalModMapKeys>0 )
	desc= XkbWriteModifierMap(xkb,rep,desc,client);
    if ( rep->totalVModMapKeys>0 )
	desc= XkbWriteVirtualModMap(xkb,rep,desc,client);
    if ((desc-start)!=(len)) {
	ErrorF("BOGUS LENGTH in write keyboard desc, expected %d, got %d\n",
					len, desc-start);
    }
    if (client->swapped) {
	register int n;
	swaps(&rep->sequenceNumber,n);
	swapl(&rep->length,n);
	swaps(&rep->present,n);
	swaps(&rep->totalSyms,n);
	swaps(&rep->totalActs,n);
    }
    WriteToClient(client, (i=SIZEOF(xkbGetMapReply)), (char *)rep);
    WriteToClient(client, len, start);
    DEALLOCATE_LOCAL((char *)start);
    return client->noClientException;
}

int
#if NeedFunctionPrototypes
ProcXkbGetMap(ClientPtr client)
#else
ProcXkbGetMap(client)
    ClientPtr client;
#endif
{
    DeviceIntPtr	 dev;
    xkbGetMapReply	 rep;
    XkbDescRec		*xkb;
    int			 n,status;

    REQUEST(xkbGetMapReq);
    REQUEST_SIZE_MATCH(xkbGetMapReq);
    
    if (!(client->xkbClientFlags&_XkbClientInitialized))
	return BadAccess;

    CHK_KBD_DEVICE(dev,stuff->deviceSpec);
    CHK_MASK_OVERLAP(0x01,stuff->full,stuff->partial);
    CHK_MASK_LEGAL(0x02,stuff->full,XkbAllMapComponentsMask);
    CHK_MASK_LEGAL(0x03,stuff->partial,XkbAllMapComponentsMask);

    xkb= dev->key->xkbInfo->desc;
    bzero(&rep,sizeof(xkbGetMapReply));
    rep.type= X_Reply;
    rep.sequenceNumber= client->sequence;
    rep.length = (SIZEOF(xkbGetMapReply)-SIZEOF(xGenericReply))>>2;
    rep.deviceID = dev->id;
    rep.present = stuff->partial|stuff->full;
    rep.minKeyCode = xkb->min_key_code;
    rep.maxKeyCode = xkb->max_key_code;
    if ( stuff->full&XkbKeyTypesMask ) {
	rep.firstType = 0;
	rep.nTypes = xkb->map->num_types;
    }
    else if (stuff->partial&XkbKeyTypesMask) {
	if (((unsigned)stuff->firstType+stuff->nTypes)>xkb->map->num_types) {
	    client->errorValue = _XkbErrCode4(0x04,xkb->map->num_types,
					stuff->firstType,stuff->nTypes);
	    return BadValue;
	}
	rep.firstType = stuff->firstType;
	rep.nTypes = stuff->nTypes;
    }
    else rep.nTypes = 0;
    rep.totalTypes = xkb->map->num_types;

    n= XkbNumKeys(xkb);
    if ( stuff->full&XkbKeySymsMask ) {
	rep.firstKeySym = xkb->min_key_code;
	rep.nKeySyms = n;
    }
    else if (stuff->partial&XkbKeySymsMask) {
	CHK_KEY_RANGE(0x05,stuff->firstKeySym,stuff->nKeySyms,xkb);
	rep.firstKeySym = stuff->firstKeySym;
	rep.nKeySyms = stuff->nKeySyms;
    }
    else rep.nKeySyms = 0;
    rep.totalSyms= 0;

    if ( stuff->full&XkbKeyActionsMask ) {
	rep.firstKeyAct= xkb->min_key_code;
	rep.nKeyActs= n;
    }
    else if (stuff->partial&XkbKeyActionsMask) {
	CHK_KEY_RANGE(0x07,stuff->firstKeyAct,stuff->nKeyActs,xkb);
	rep.firstKeyAct= stuff->firstKeyAct;
	rep.nKeyActs= stuff->nKeyActs;
    }
    else rep.nKeyActs= 0;
    rep.totalActs= 0;

    if ( stuff->full&XkbKeyBehaviorsMask ) {
	rep.firstKeyBehavior = xkb->min_key_code;
	rep.nKeyBehaviors = n;
    }
    else if (stuff->partial&XkbKeyBehaviorsMask) {
	CHK_KEY_RANGE(0x09,stuff->firstKeyBehavior,stuff->nKeyBehaviors,xkb);
	rep.firstKeyBehavior= stuff->firstKeyBehavior;
	rep.nKeyBehaviors= stuff->nKeyBehaviors;
    }
    else rep.nKeyBehaviors = 0;
    rep.totalKeyBehaviors= 0;

    if (stuff->full&XkbVirtualModsMask)
	rep.virtualMods= ~0;
    else if (stuff->partial&XkbVirtualModsMask)
	rep.virtualMods= stuff->virtualMods;
    
    if (stuff->full&XkbExplicitComponentsMask) {
	rep.firstKeyExplicit= xkb->min_key_code;
	rep.nKeyExplicit= n;
    }
    else if (stuff->partial&XkbExplicitComponentsMask) {
	CHK_KEY_RANGE(0x0B,stuff->firstKeyExplicit,stuff->nKeyExplicit,xkb);
	rep.firstKeyExplicit= stuff->firstKeyExplicit;
	rep.nKeyExplicit= stuff->nKeyExplicit;
    }
    else rep.nKeyExplicit = 0;
    rep.totalKeyExplicit=  0;

    if (stuff->full&XkbModifierMapMask) {
	rep.firstModMapKey= xkb->min_key_code;
	rep.nModMapKeys= n;
    }
    else if (stuff->partial&XkbModifierMapMask) {
	CHK_KEY_RANGE(0x0D,stuff->firstModMapKey,stuff->nModMapKeys,xkb);
	rep.firstModMapKey= stuff->firstModMapKey;
	rep.nModMapKeys= stuff->nModMapKeys;
    }
    else rep.nModMapKeys = 0;
    rep.totalModMapKeys= 0;

    if (stuff->full&XkbVirtualModMapMask) {
	rep.firstVModMapKey= xkb->min_key_code;
	rep.nVModMapKeys= n;
    }
    else if (stuff->partial&XkbVirtualModMapMask) {
	CHK_KEY_RANGE(0x0F,stuff->firstVModMapKey,stuff->nVModMapKeys,xkb);
	rep.firstVModMapKey= stuff->firstVModMapKey;
	rep.nVModMapKeys= stuff->nVModMapKeys;
    }
    else rep.nVModMapKeys = 0;
    rep.totalVModMapKeys= 0;

    if ((status=XkbComputeGetMapReplySize(xkb,&rep))!=Success)
	return status;
    return XkbSendMap(client,xkb,&rep);
}

/***====================================================================***/

static int
#if NeedFunctionPrototypes
CheckKeyTypes(	ClientPtr	client,
		XkbDescPtr	xkb,
		xkbSetMapReq *	req,
		xkbKeyTypeWireDesc **wireRtrn,
		int	 *	nMapsRtrn,
		CARD8 *		mapWidthRtrn)
#else
CheckKeyTypes(client,xkb,req,wireRtrn,nMapsRtrn,mapWidthRtrn)
    ClientPtr		 client;
    XkbDescPtr	 	 xkb;
    xkbSetMapReq  *	 req;
    xkbKeyTypeWireDesc **wireRtrn;
    int	 *		 nMapsRtrn;
    CARD8 *		 mapWidthRtrn;
#endif
{
unsigned		nMaps;
register unsigned	i,n;
register CARD8 *	map;
register xkbKeyTypeWireDesc	*wire = *wireRtrn;

    if (req->firstType>((unsigned)xkb->map->num_types)) {
	*nMapsRtrn = _XkbErrCode3(0x01,req->firstType,xkb->map->num_types);
	return 0;
    }
    if (req->flags&XkbSetMapResizeTypes) {
	nMaps = req->firstType+req->nTypes;
	if (nMaps<XkbNumRequiredTypes) {  /* canonical types must be there */
	    *nMapsRtrn= _XkbErrCode4(0x02,req->firstType,req->nTypes,4);
	    return 0;
	}
    }
    else if (req->present&XkbKeyTypesMask) {
	nMaps = xkb->map->num_types;
	if ((req->firstType+req->nTypes)>nMaps) {
	    *nMapsRtrn = req->firstType+req->nTypes;
	    return 0;
	}
    }
    else {
	*nMapsRtrn = xkb->map->num_types;
	for (i=0;i<xkb->map->num_types;i++) {
	    mapWidthRtrn[i] = xkb->map->types[i].num_levels;
	}
	return 1;
    }

    for (i=0;i<req->firstType;i++) {
	mapWidthRtrn[i] = xkb->map->types[i].num_levels;
    }
    for (i=0;i<req->nTypes;i++) {
	unsigned	width;
	if (client->swapped) {
	    register int s;
	    swaps(&wire->virtualMods,s);
	}
	n= i+req->firstType;
	width= wire->numLevels;
	if (width<1) {
	    *nMapsRtrn= _XkbErrCode3(0x04,n,width);
	    return 0;
	}
	else if ((n==XkbOneLevelIndex)&&(width!=1)) { /* must be width 1 */
	    *nMapsRtrn= _XkbErrCode3(0x05,n,width);
	    return 0;
	}
	else if ((width!=2)&&
		 ((n==XkbTwoLevelIndex)||(n==XkbKeypadIndex)||
		  (n==XkbAlphabeticIndex))) {
	    /* TWO_LEVEL, ALPHABETIC and KEYPAD must be width 2 */
	    *nMapsRtrn= _XkbErrCode3(0x05,n,width);
	    return 0;
	}
	if (wire->nMapEntries>0) {
	    xkbKTSetMapEntryWireDesc *	mapWire;
	    xkbModsWireDesc *		preWire;
	    mapWire= (xkbKTSetMapEntryWireDesc *)&wire[1];
	    preWire= (xkbModsWireDesc *)&mapWire[wire->nMapEntries];
	    for (n=0;n<wire->nMapEntries;n++) {
		if (client->swapped) {
		    register int s;
		    swaps(&mapWire[n].virtualMods,s);
		}
		if (mapWire[n].realMods&(~wire->realMods)) {
		    *nMapsRtrn= _XkbErrCode4(0x06,n,mapWire[n].realMods,
						 wire->realMods);
		    return 0;
		}
		if (mapWire[n].virtualMods&(~wire->virtualMods)) {
		    *nMapsRtrn= _XkbErrCode3(0x07,n,mapWire[n].virtualMods);
		    return 0;
		}
		if (mapWire[n].level>=wire->numLevels) {
		    *nMapsRtrn= _XkbErrCode4(0x08,n,wire->numLevels,
						 mapWire[n].level);
		    return 0;
		}
		if (wire->preserve) {
		    if (client->swapped) {
			register int s;
			swaps(&preWire[n].virtualMods,s);
		    }
		    if (preWire[n].realMods&(~mapWire[n].realMods)) {
			*nMapsRtrn= _XkbErrCode4(0x09,n,preWire[n].realMods,
							mapWire[n].realMods);
			return 0;
		    }
		    if (preWire[n].virtualMods&(~mapWire[n].virtualMods)) {
			*nMapsRtrn=_XkbErrCode3(0x0a,n,preWire[n].virtualMods);
			return 0;
		    }
		}
	    }
	    if (wire->preserve)
		 map= (CARD8 *)&preWire[wire->nMapEntries];
	    else map= (CARD8 *)&mapWire[wire->nMapEntries];
	}
	else map= (CARD8 *)&wire[1];
	mapWidthRtrn[i+req->firstType] = wire->numLevels;
	wire= (xkbKeyTypeWireDesc *)map;
    }
    for (i=req->firstType+req->nTypes;i<nMaps;i++) {
	mapWidthRtrn[i] = xkb->map->types[i].num_levels;
    }
    *nMapsRtrn = nMaps;
    *wireRtrn = wire;
    return 1;
}

static int
#if NeedFunctionPrototypes
CheckKeySyms(	ClientPtr		client,
		XkbDescPtr		xkb,
		xkbSetMapReq *		req,
		int			nTypes,
		CARD8 *	 		mapWidths,
		CARD16 *	 	symsPerKey,
		xkbSymMapWireDesc **	wireRtrn,
		int *			errorRtrn)
#else
CheckKeySyms(client,xkb,req,nTypes,mapWidths,symsPerKey,wireRtrn,errorRtrn)
    ClientPtr		client;
    XkbDescPtr		xkb;
    xkbSetMapReq *	req;
    int			nTypes;
    CARD8 *	 	mapWidths;
    CARD16 *	 	symsPerKey;
    xkbSymMapWireDesc **wireRtrn;
    int *		errorRtrn;
#endif
{
register unsigned	i;
XkbSymMapPtr		map;
xkbSymMapWireDesc*	wire = *wireRtrn;

    if (!(XkbKeySymsMask&req->present))
	return 1;
    CHK_REQ_KEY_RANGE2(0x11,req->firstKeySym,req->nKeySyms,req,(*errorRtrn),0);
    map = &xkb->map->key_sym_map[xkb->min_key_code];
    for (i=xkb->min_key_code;i<(unsigned)req->firstKeySym;i++,map++) {
	register int g,ng,w;
	ng= XkbNumGroups(map->group_info);
	for (w=g=0;g<ng;g++) {
	    if (map->kt_index[g]>=(unsigned)nTypes) {
		*errorRtrn = _XkbErrCode4(0x13,i,g,map->kt_index[g]);
		return 0;
	    }
	    if (mapWidths[map->kt_index[g]]>w)
		w= mapWidths[map->kt_index[g]];
	}
	symsPerKey[i] = w*ng;
    }
    for (i=0;i<req->nKeySyms;i++) {
	KeySym *pSyms;
	register unsigned nG;
	if (client->swapped) {
	    swaps(&wire->nSyms,nG);
	}
	nG = XkbNumGroups(wire->groupInfo);
	if (nG>XkbNumKbdGroups) {
	    *errorRtrn = _XkbErrCode3(0x14,i+req->firstKeySym,nG);
	    return 0;
	}
	if (nG>0) {
	    register int g,w;
	    for (g=w=0;g<nG;g++) {
		if (wire->ktIndex[g]>=(unsigned)nTypes) {
		    *errorRtrn= _XkbErrCode4(0x15,i+req->firstKeySym,g,
		    					   wire->ktIndex[g]);
		    return 0;
		}
		if (mapWidths[wire->ktIndex[g]]>w)
		    w= mapWidths[wire->ktIndex[g]];
	    }
	    if (wire->width!=w) {
		*errorRtrn= _XkbErrCode3(0x16,i+req->firstKeySym,wire->width);
		return 0;
	    }
	    w*= nG;
	    symsPerKey[i+req->firstKeySym] = w;
	    if (w!=wire->nSyms) {
		*errorRtrn=_XkbErrCode4(0x16,i+req->firstKeySym,wire->nSyms,w);
		return 0;
	    }
	}
	else if (wire->nSyms!=0) {
	    *errorRtrn = _XkbErrCode3(0x17,i+req->firstKeySym,wire->nSyms);
	    return 0;
	}
	pSyms = (KeySym *)&wire[1];
	wire = (xkbSymMapWireDesc *)&pSyms[wire->nSyms];
    }

    map = &xkb->map->key_sym_map[i];
    for (;i<=(unsigned)xkb->max_key_code;i++,map++) {
	register int g,nG,w;
	nG= XkbKeyNumGroups(xkb,i);
	for (w=g=0;g<nG;g++)  {
	    if (map->kt_index[g]>=(unsigned)nTypes) {
		*errorRtrn = _XkbErrCode4(0x18,i,g,map->kt_index[g]);
		return 0;
	    }
	    if (mapWidths[map->kt_index[g]]>w)
		    w= mapWidths[map->kt_index[g]];
	}
	symsPerKey[i] = w*nG;
    }
    *wireRtrn = wire;
    return 1;
}

static int
#if NeedFunctionPrototypes
CheckKeyActions(	XkbDescPtr	xkb,
			xkbSetMapReq *	req,
			int		nTypes,
			CARD8 *		mapWidths,
			CARD16 *	symsPerKey,
			CARD8 **	wireRtrn,
			int *		nActsRtrn)
#else
CheckKeyActions(xkb,req,nTypes,mapWidths,symsPerKey,wireRtrn,nActsRtrn)
    XkbDescRec		 *xkb;
    xkbSetMapReq	 *req;
    int			  nTypes;
    CARD8		 *mapWidths;
    CARD16		 *symsPerKey;
    CARD8		**wireRtrn;
    int			 *nActsRtrn;
#endif
{
int			 nActs;
CARD8 *			 wire = *wireRtrn;
register unsigned	 i;

    if (!(XkbKeyActionsMask&req->present))
	return 1;
    CHK_REQ_KEY_RANGE2(0x21,req->firstKeyAct,req->nKeyActs,req,(*nActsRtrn),0);
    for (nActs=i=0;i<req->nKeyActs;i++) {
	if (wire[0]!=0) {
	    if (wire[0]==symsPerKey[i+req->firstKeyAct])
		nActs+= wire[0];
	    else {
		*nActsRtrn= _XkbErrCode3(0x23,i+req->firstKeyAct,wire[0]);
		return 0;
	    }
	}
	wire++;
    }
    if (req->nKeyActs%4)
	wire+= 4-(req->nKeyActs%4);
    *wireRtrn = (CARD8 *)(((XkbAnyAction *)wire)+nActs);
    *nActsRtrn = nActs;
    return 1;
}

static int
#if NeedFunctionPrototypes
CheckKeyBehaviors(	XkbDescPtr 		xkb,
			xkbSetMapReq *		req,
			xkbBehaviorWireDesc **	wireRtrn,
			int *			errorRtrn)
#else
CheckKeyBehaviors(xkb,req,wireRtrn,errorRtrn)
    XkbDescRec	 	 *xkb;
    xkbSetMapReq	 *req;
    xkbBehaviorWireDesc	**wireRtrn;
    int			 *errorRtrn;
#endif
{
register xkbBehaviorWireDesc *	wire = *wireRtrn;
register XkbServerMapPtr	server = xkb->server;
register unsigned	 	i;
unsigned			first,last;

    if ((req->present&XkbKeyBehaviorsMask==0)||(req->nKeyBehaviors<1)) {
	req->present&= ~XkbKeyBehaviorsMask;
	req->nKeyBehaviors= 0;
	return 1;
    }
    first= req->firstKeyBehavior;
    last=  req->firstKeyBehavior+req->nKeyBehaviors-1;
    if (first<req->minKeyCode) {
	*errorRtrn = _XkbErrCode3(0x31,first,req->minKeyCode);
	return 0;
    }
    if (last>req->maxKeyCode) {
	*errorRtrn = _XkbErrCode3(0x32,last,req->maxKeyCode);
	return 0;
    }
	
    for (i=0;i<req->totalKeyBehaviors;i++,wire++) {
	if ((wire->key<first)||(wire->key>last)) {
	    *errorRtrn = _XkbErrCode4(0x33,first,last,wire->key);
	    return 0;
	}
	if ((wire->type&XkbKB_Permanent)&&
	    ((server->behaviors[wire->key].type!=wire->type)||
	     (server->behaviors[wire->key].data!=wire->data))) {
	    *errorRtrn = _XkbErrCode3(0x33,wire->key,wire->type);
	    return 0;
	}
	if ((wire->type==XkbKB_RadioGroup)&&
		((wire->data&(~XkbKB_RGAllowNone))>XkbMaxRadioGroups)) {
	    *errorRtrn= _XkbErrCode4(0x34,wire->key,wire->data,
							XkbMaxRadioGroups);
	    return 0;
	}
	if ((wire->type==XkbKB_Overlay1)||(wire->type==XkbKB_Overlay2)) {
	    CHK_KEY_RANGE2(0x35,wire->key,1,xkb,*errorRtrn,0);
	}
    }
    *wireRtrn = wire;
    return 1;
}

static int
#if NeedFunctionPrototypes
CheckVirtualMods(	XkbDescRec *	xkb,
			xkbSetMapReq *	req,
			CARD8 **	wireRtrn,
			int *		errorRtrn)
#else
CheckVirtualMods(xkb,req,wireRtrn,errorRtrn)
    XkbDescPtr		xkb;
    xkbSetMapReq *	req;
    CARD8 **		wireRtrn;
    int *		errorRtrn;
#endif
{
register CARD8		*wire = *wireRtrn;
register unsigned 	 i,nMods,bit;

    if (((req->present&XkbVirtualModsMask)==0)||(req->virtualMods==0))
	return 1;
    for (i=nMods=0,bit=1;i<XkbNumVirtualMods;i++,bit<<=1) {
	if (req->virtualMods&bit)
	    nMods++;
    }
    *wireRtrn= (wire+XkbPaddedSize(nMods));
    return 1;
}

static int
#if NeedFunctionPrototypes
CheckKeyExplicit(	XkbDescPtr	xkb,
			xkbSetMapReq *	req,
			CARD8 **	wireRtrn,
			int	*	errorRtrn)
#else
CheckKeyExplicit(xkb,req,wireRtrn,errorRtrn)
    XkbDescPtr		xkb;
    xkbSetMapReq *	req;
    CARD8 **		wireRtrn;
    int	*		errorRtrn;
#endif
{
register CARD8 *	wire = *wireRtrn;
CARD8	*		start;
register unsigned 	i;
int			first,last;

    if (((req->present&XkbExplicitComponentsMask)==0)||(req->nKeyExplicit<1)) {
	req->present&= ~XkbExplicitComponentsMask;
	req->nKeyExplicit= 0;
	return 1;
    }
    first= req->firstKeyExplicit;
    last=  first+req->nKeyExplicit-1;
    if (first<req->minKeyCode) {
	*errorRtrn = _XkbErrCode3(0x51,first,req->minKeyCode);
	return 0;
    }
    if (last>req->maxKeyCode) {
	*errorRtrn = _XkbErrCode3(0x52,last,req->maxKeyCode);
	return 0;
    }
    start= wire; 
    for (i=0;i<req->totalKeyExplicit;i++,wire+=2) {
	if ((wire[0]<first)||(wire[0]>last)) {
	    *errorRtrn = _XkbErrCode4(0x53,first,last,wire[0]);
	    return 0;
	}
	if (wire[1]&(~XkbAllExplicitMask)) {
	     *errorRtrn= _XkbErrCode3(0x52,~XkbAllExplicitMask,wire[1]);
	     return 0;
	}
    }
    wire+= XkbPaddedSize(wire-start)-(wire-start);
    *wireRtrn= wire;
    return 1;
}

static int
#if NeedFunctionPrototypes
CheckModifierMap(XkbDescPtr xkb,xkbSetMapReq *req,CARD8 **wireRtrn,int *errRtrn)
#else
CheckModifierMap(xkb,req,wireRtrn,errRtrn)
    XkbDescPtr		xkb;
    xkbSetMapReq *	req;
    CARD8 **		wireRtrn;
    int	*		errRtrn;
#endif
{
register CARD8 *	wire = *wireRtrn;
CARD8	*		start;
register unsigned 	i;
int			first,last;

    if (((req->present&XkbModifierMapMask)==0)||(req->nModMapKeys<1)) {
	req->present&= ~XkbModifierMapMask;
	req->nModMapKeys= 0;
	return 1;
    }
    first= req->firstModMapKey;
    last=  first+req->nModMapKeys-1;
    if (first<req->minKeyCode) {
	*errRtrn = _XkbErrCode3(0x61,first,req->minKeyCode);
	return 0;
    }
    if (last>req->maxKeyCode) {
	*errRtrn = _XkbErrCode3(0x62,last,req->maxKeyCode);
	return 0;
    }
    start= wire; 
    for (i=0;i<req->totalModMapKeys;i++,wire+=2) {
	if ((wire[0]<first)||(wire[0]>last)) {
	    *errRtrn = _XkbErrCode4(0x63,first,last,wire[0]);
	    return 0;
	}
    }
    wire+= XkbPaddedSize(wire-start)-(wire-start);
    *wireRtrn= wire;
    return 1;
}

static int
#if NeedFunctionPrototypes
CheckVirtualModMap(	XkbDescPtr xkb,
			xkbSetMapReq *req,
			xkbVModMapWireDesc **wireRtrn,
			int *errRtrn)
#else
CheckVirtualModMap(xkb,req,wireRtrn,errRtrn)
    XkbDescPtr			xkb;
    xkbSetMapReq *		req;
    xkbVModMapWireDesc **	wireRtrn;
    int	*			errRtrn;
#endif
{
register xkbVModMapWireDesc *	wire = *wireRtrn;
register unsigned 		i;
int				first,last;

    if (((req->present&XkbVirtualModMapMask)==0)||(req->nVModMapKeys<1)) {
	req->present&= ~XkbVirtualModMapMask;
	req->nVModMapKeys= 0;
	return 1;
    }
    first= req->firstVModMapKey;
    last=  first+req->nVModMapKeys-1;
    if (first<req->minKeyCode) {
	*errRtrn = _XkbErrCode3(0x71,first,req->minKeyCode);
	return 0;
    }
    if (last>req->maxKeyCode) {
	*errRtrn = _XkbErrCode3(0x72,last,req->maxKeyCode);
	return 0;
    }
    for (i=0;i<req->totalVModMapKeys;i++,wire++) {
	if ((wire->key<first)||(wire->key>last)) {
	    *errRtrn = _XkbErrCode4(0x73,first,last,wire->key);
	    return 0;
	}
    }
    *wireRtrn= wire;
    return 1;
}

static char *
#if NeedFunctionPrototypes
SetKeyTypes(	XkbDescPtr		xkb,
		xkbSetMapReq *		req,
		xkbKeyTypeWireDesc *	wire,
		XkbChangesPtr		changes)
#else
SetKeyTypes(xkb,req,wire,changes)
    XkbDescPtr		xkb;
    xkbSetMapReq *	req;
    xkbKeyTypeWireDesc *wire;
    XkbChangesPtr	changes;
#endif
{
register unsigned	i;
unsigned		first,last;
CARD8			*map;

    if ((unsigned)(req->firstType+req->nTypes)>xkb->map->size_types) {
	i= req->firstType+req->nTypes;
	if (XkbAllocClientMap(xkb,XkbKeyTypesMask,i)!=Success) {
	    return NULL;
	}
    }
    if ((unsigned)(req->firstType+req->nTypes)>xkb->map->num_types)
	xkb->map->num_types= req->firstType+req->nTypes;

    for (i=0;i<req->nTypes;i++) {
	XkbKeyTypePtr		pOld;
	register unsigned 	n;

	if (XkbResizeKeyType(xkb,i+req->firstType,wire->nMapEntries,
				wire->preserve,wire->numLevels)!=Success) {
	    return NULL;
	}
	pOld = &xkb->map->types[i+req->firstType];
	map = (CARD8 *)&wire[1];

	pOld->mods.real_mods = wire->realMods;
	pOld->mods.vmods= wire->virtualMods;
	pOld->num_levels = wire->numLevels;
	pOld->map_count= wire->nMapEntries;

	pOld->mods.mask= pOld->mods.real_mods|
					XkbMaskForVMask(xkb,pOld->mods.vmods);

	if (wire->nMapEntries) {
	    xkbKTSetMapEntryWireDesc *mapWire;
	    xkbModsWireDesc *preWire;
	    unsigned tmp;
	    mapWire= (xkbKTSetMapEntryWireDesc *)map;
	    preWire= (xkbModsWireDesc *)&mapWire[wire->nMapEntries];
	    for (n=0;n<wire->nMapEntries;n++) {
		pOld->map[n].active= 1;
		pOld->map[n].mods.mask= mapWire[n].realMods;
		pOld->map[n].mods.real_mods= mapWire[n].realMods;
		pOld->map[n].mods.vmods= mapWire[n].virtualMods;
		pOld->map[n].level= mapWire[n].level;
		if (mapWire[n].virtualMods!=0) {
		    tmp= XkbMaskForVMask(xkb,mapWire[n].virtualMods);
		    pOld->map[n].active= (tmp!=0);
		    pOld->map[n].mods.mask|= tmp;
		}
		if (wire->preserve) {
		    pOld->preserve[n].real_mods= preWire[n].realMods;
		    pOld->preserve[n].vmods= preWire[n].virtualMods;
		    tmp= XkbMaskForVMask(xkb,preWire[n].virtualMods);
		    pOld->preserve[n].mask= preWire[n].realMods|tmp;
		}
	    }
	    if (wire->preserve)
		 map= (CARD8 *)&preWire[wire->nMapEntries];
	    else map= (CARD8 *)&mapWire[wire->nMapEntries];
	}
	else map= (CARD8 *)&wire[1];
	wire = (xkbKeyTypeWireDesc *)map;
    }
    first= req->firstType;
    last= first+req->nTypes-1; /* last changed type */
    if (changes->map.changed&XkbKeyTypesMask) {
	int oldLast;
	oldLast= changes->map.first_type+changes->map.num_types-1;
	if (changes->map.first_type<first)
	    first= changes->map.first_type;
	if (oldLast>last)
	    last= oldLast;
    }
    changes->map.changed|= XkbKeyTypesMask;
    changes->map.first_type = first;
    changes->map.num_types = (last-first)+1;
    return (char *)wire;
}

static char *
#if NeedFunctionPrototypes
SetKeySyms(	ClientPtr		client,
		XkbDescPtr		xkb,
		xkbSetMapReq *		req,
		xkbSymMapWireDesc *	wire,
		XkbChangesPtr 		changes,
		DeviceIntPtr		dev)
#else
SetKeySyms(client,xkb,req,wire,changes,dev)
    ClientPtr		client;
    XkbDescPtr		xkb;
    xkbSetMapReq *	req;
    xkbSymMapWireDesc *	wire;
    XkbChangesPtr 	changes;
    DeviceIntPtr	dev;
#endif
{
register unsigned 	i,s;
XkbSymMapPtr		oldMap;
KeySym *		newSyms;
KeySym *		pSyms;
unsigned		first,last;

    oldMap = &xkb->map->key_sym_map[req->firstKeySym];
    for (i=0;i<req->nKeySyms;i++,oldMap++) {
	pSyms = (KeySym *)&wire[1];
	if (wire->nSyms>0) {
	    newSyms = XkbResizeKeySyms(xkb,i+req->firstKeySym,wire->nSyms);
	    for (s=0;s<wire->nSyms;s++) {
		newSyms[s]= pSyms[s];
	    }
	    if (client->swapped) {
		int n;
		for (s=0;s<wire->nSyms;s++) {
		    swapl(&newSyms[s],n);
		}
	    }
	}
	oldMap->kt_index[0] = wire->ktIndex[0];
	oldMap->kt_index[1] = wire->ktIndex[1];
	oldMap->kt_index[2] = wire->ktIndex[2];
	oldMap->kt_index[3] = wire->ktIndex[3];
	oldMap->group_info = wire->groupInfo;
	oldMap->width = wire->width;
	wire= (xkbSymMapWireDesc *)&pSyms[wire->nSyms];
    }
    first= req->firstKeySym;
    last= first+req->nKeySyms-1;
    if (changes->map.changed&XkbKeySymsMask) {
	int oldLast= (changes->map.first_key_sym+changes->map.num_key_syms-1);
	if (changes->map.first_key_sym<first)
	    first= changes->map.first_key_sym;
	if (oldLast>last)
	    last= oldLast;
    }
    changes->map.changed|= XkbKeySymsMask;
    changes->map.first_key_sym = first;
    changes->map.num_key_syms = (last-first+1);

    s= 0;
    for (i=xkb->min_key_code;i<=xkb->max_key_code;i++) {
	if (XkbKeyNumGroups(xkb,i)>s)
	    s= XkbKeyNumGroups(xkb,i);
    }
    if (s!=xkb->ctrls->num_groups) {
	xkbControlsNotify	cn;
	XkbControlsRec		old;
	cn.keycode= 0;
	cn.eventType= 0;
	cn.requestMajor= XkbReqCode;
	cn.requestMinor= X_kbSetMap;
	old= *xkb->ctrls;
	xkb->ctrls->num_groups= s;
	if (XkbComputeControlsNotify(dev,&old,xkb->ctrls,&cn,False))
	    XkbSendControlsNotify(dev,&cn);
    }
    return (char *)wire;
}

static char *
#if NeedFunctionPrototypes
SetKeyActions(	XkbDescPtr	xkb,
		xkbSetMapReq *	req,
		CARD8 *		wire,
		XkbChangesPtr	changes)
#else
SetKeyActions(xkb,req,wire,changes)
    XkbDescPtr		xkb;
    xkbSetMapReq *	req;
    CARD8 *		wire;
    XkbChangesPtr	changes;
#endif
{
register unsigned	i,first,last;
CARD8 *			nActs = wire;
XkbAction *		newActs;
    
    wire+= XkbPaddedSize(req->nKeyActs);
    for (i=0;i<req->nKeyActs;i++) {
	if (nActs[i]==0)
	    xkb->server->key_acts[i+req->firstKeyAct]= 0;
	else {
	    newActs= XkbResizeKeyActions(xkb,i+req->firstKeyAct,nActs[i]);
	    memcpy((char *)newActs,(char *)wire,
					nActs[i]*SIZEOF(xkbActionWireDesc));
	    wire+= nActs[i]*SIZEOF(xkbActionWireDesc);
	}
    }
    first= req->firstKeyAct;
    last= (first+req->nKeyActs-1);
    if (changes->map.changed&XkbKeyActionsMask) {
	int oldLast;
	oldLast= changes->map.first_key_act+changes->map.num_key_acts-1;
	if (changes->map.first_key_act<first)
	    first= changes->map.first_key_act;
	if (oldLast>last)
	    last= oldLast;
    }
    changes->map.changed|= XkbKeyActionsMask;
    changes->map.first_key_act= first;
    changes->map.num_key_acts= (last-first+1);
    return (char *)wire;
}

static char *
#if NeedFunctionPrototypes
SetKeyBehaviors(	XkbSrvInfoPtr	 xkbi,
    			xkbSetMapReq	*req,
    			xkbBehaviorWireDesc	*wire,
    			XkbChangesPtr	 changes)
#else
SetKeyBehaviors(xkbi,req,wire,changes)
    XkbSrvInfoPtr	xkbi;
    xkbSetMapReq *	req;
    xkbBehaviorWireDesc*wire;
    XkbChangesPtr	changes;
#endif
{
register unsigned i;
int maxRG = -1;
XkbDescPtr       xkb = xkbi->desc;
XkbServerMapPtr	 server = xkb->server;
unsigned	 first,last;

    first= req->firstKeyBehavior;
    last= req->firstKeyBehavior+req->nKeyBehaviors-1;
    bzero(&server->behaviors[first],req->nKeyBehaviors*sizeof(XkbBehavior));
    for (i=0;i<req->totalKeyBehaviors;i++) {
	if ((server->behaviors[wire->key].type&XkbKB_Permanent)==0) {
	    server->behaviors[wire->key].type= wire->type;
	    server->behaviors[wire->key].data= wire->data;
	    if ((wire->type==XkbKB_RadioGroup)&&(((int)wire->data)>maxRG))
		maxRG= wire->data + 1;
	}
	wire++;
    }

    if (maxRG>(int)xkbi->nRadioGroups) {
        int sz = maxRG*sizeof(XkbRadioGroupRec);
        if (xkbi->radioGroups)
             xkbi->radioGroups=(XkbRadioGroupPtr)_XkbRealloc(xkbi->radioGroups,sz);
        else xkbi->radioGroups= (XkbRadioGroupPtr)_XkbCalloc(1, sz);
        if (xkbi->radioGroups) {
             if (xkbi->nRadioGroups)
                bzero(&xkbi->radioGroups[xkbi->nRadioGroups],
                        (maxRG-xkbi->nRadioGroups)*sizeof(XkbRadioGroupRec));
             xkbi->nRadioGroups= maxRG;
        }
        else xkbi->nRadioGroups= 0;
        /* should compute members here */
    }
    if (changes->map.changed&XkbKeyBehaviorsMask) {
	unsigned oldLast;
	oldLast= changes->map.first_key_behavior+
					changes->map.num_key_behaviors-1;
        if (changes->map.first_key_behavior<req->firstKeyBehavior)
             first= changes->map.first_key_behavior;
        if (oldLast>last)
            last= oldLast;
    }
    changes->map.changed|= XkbKeyBehaviorsMask;
    changes->map.first_key_behavior = first;
    changes->map.num_key_behaviors = (last-first+1);
    return (char *)wire;
}

static char *
#if NeedFunctionPrototypes
SetVirtualMods(XkbSrvInfoPtr xkbi,xkbSetMapReq *req,CARD8 *wire,
						XkbChangesPtr changes)
#else
SetVirtualMods(xkbi,req,wire,changes)
    XkbSrvInfoPtr	xkbi;
    xkbSetMapReq *	req;
    CARD8 *		wire;
    XkbChangesPtr 	changes;
#endif
{
register int 		i,bit,nMods;
XkbServerMapPtr		srv = xkbi->desc->server;

    if (((req->present&XkbVirtualModsMask)==0)||(req->virtualMods==0))
	return (char *)wire;
    for (i=nMods=0,bit=1;i<XkbNumVirtualMods;i++,bit<<=1) {
	if (req->virtualMods&bit) {
	    if (srv->vmods[i]!=wire[nMods]) {
		changes->map.changed|= XkbVirtualModsMask;
		changes->map.vmods|= bit;
		srv->vmods[i]= wire[nMods];
	    }
	    nMods++;
	}
    }
    return (char *)(wire+XkbPaddedSize(nMods));
}

static char *
#if NeedFunctionPrototypes
SetKeyExplicit(XkbSrvInfoPtr xkbi,xkbSetMapReq *req,CARD8 *wire,
							XkbChangesPtr changes)
#else
SetKeyExplicit(xkbi,req,wire,changes)
    XkbSrvInfoPtr	xkbi;
    xkbSetMapReq *	req;
    CARD8 *		wire;
    XkbChangesPtr 	changes;
#endif
{
register unsigned	i,first,last;
XkbServerMapPtr		xkb = xkbi->desc->server;
CARD8 *			start;

    start= wire;
    first= req->firstKeyExplicit;
    last=  req->firstKeyExplicit+req->nKeyExplicit-1;
    bzero(&xkb->explicit[first],req->nKeyExplicit);
    for (i=0;i<req->totalKeyExplicit;i++,wire+= 2) {
	xkb->explicit[wire[0]]= wire[1];
    }
    if (first>0) {
	if (changes->map.changed&XkbExplicitComponentsMask) {
	    int oldLast;
	    oldLast= changes->map.first_key_explicit+
					changes->map.num_key_explicit-1;
	    if (changes->map.first_key_explicit<first)
		first= changes->map.first_key_explicit;
	    if (oldLast>last)
		last= oldLast;
	}
	changes->map.first_key_explicit= first;
	changes->map.num_key_explicit= (last-first)+1;
    }
    wire+= XkbPaddedSize(wire-start)-(wire-start);
    return (char *)wire;
}

static char *
#if NeedFunctionPrototypes
SetModifierMap(	XkbSrvInfoPtr	xkbi,
		xkbSetMapReq *	req,
		CARD8 *		wire,
		XkbChangesPtr	changes)
#else
SetModifierMap(xkbi,req,wire,changes)
    XkbSrvInfoPtr	xkbi;
    xkbSetMapReq *	req;
    CARD8 *		wire;
    XkbChangesPtr	changes;
#endif
{
register unsigned	i,first,last;
XkbClientMapPtr		xkb = xkbi->desc->map;
CARD8 *			start;

    start= wire;
    first= req->firstModMapKey;
    last=  req->firstModMapKey+req->nModMapKeys-1;
    bzero(&xkb->modmap[first],req->nModMapKeys);
    for (i=0;i<req->totalModMapKeys;i++,wire+= 2) {
	xkb->modmap[wire[0]]= wire[1];
    }
    if (first>0) {
	if (changes->map.changed&XkbModifierMapMask) {
	    int oldLast;
	    oldLast= changes->map.first_modmap_key+
						changes->map.num_modmap_keys-1;
	    if (changes->map.first_modmap_key<first)
		first= changes->map.first_modmap_key;
	    if (oldLast>last)
		last= oldLast;
	}
	changes->map.first_modmap_key= first;
	changes->map.num_modmap_keys= (last-first)+1;
    }
    wire+= XkbPaddedSize(wire-start)-(wire-start);
    return (char *)wire;
}

static char *
#if NeedFunctionPrototypes
SetVirtualModMap(	XkbSrvInfoPtr		xkbi,
			xkbSetMapReq *		req,
			xkbVModMapWireDesc *	wire,
			XkbChangesPtr 		changes)
#else
SetVirtualModMap(xkbi,req,wire,changes)
    XkbSrvInfoPtr	xkbi;
    xkbSetMapReq *	req;
    xkbVModMapWireDesc *wire;
    XkbChangesPtr 	changes;
#endif
{
register unsigned	i,first,last;
XkbServerMapPtr		srv = xkbi->desc->server;

    first= req->firstVModMapKey;
    last=  req->firstVModMapKey+req->nVModMapKeys-1;
    bzero(&srv->vmodmap[first],req->nVModMapKeys*sizeof(unsigned short));
    for (i=0;i<req->totalVModMapKeys;i++,wire++) {
	srv->vmodmap[wire->key]= wire->vmods;
    }
    if (first>0) {
	if (changes->map.changed&XkbVirtualModMapMask) {
	    int oldLast;
	    oldLast= changes->map.first_vmodmap_key+
					changes->map.num_vmodmap_keys-1;
	    if (changes->map.first_vmodmap_key<first)
		first= changes->map.first_vmodmap_key;
	    if (oldLast>last)
		last= oldLast;
	}
	changes->map.first_vmodmap_key= first;
	changes->map.num_vmodmap_keys= (last-first)+1;
    }
    return (char *)wire;
}

int
#if NeedFunctionPrototypes
ProcXkbSetMap(ClientPtr client)
#else
ProcXkbSetMap(client)
    ClientPtr client;
#endif
{
    DeviceIntPtr	dev;
    XkbSrvInfoPtr	xkbi;
    XkbDescPtr		xkb;
    XkbChangesRec	change;
    XkbEventCauseRec	cause;
    int			nTypes,nActions,error;
    char *		tmp;
    CARD8	 	mapWidths[XkbMaxLegalKeyCode+1];
    CARD16	 	symsPerKey[XkbMaxLegalKeyCode+1];
    Bool		sentNKN;

    REQUEST(xkbSetMapReq);
    REQUEST_AT_LEAST_SIZE(xkbSetMapReq);

    if (!(client->xkbClientFlags&_XkbClientInitialized))
	return BadAccess;

    CHK_KBD_DEVICE(dev,stuff->deviceSpec);
    CHK_MASK_LEGAL(0x01,stuff->present,XkbAllMapComponentsMask);

    XkbSetCauseXkbReq(&cause,X_kbSetMap,client);
    xkbi= dev->key->xkbInfo;
    xkb = xkbi->desc;

    if ((xkb->min_key_code!=stuff->minKeyCode)||
    				(xkb->max_key_code!=stuff->maxKeyCode)) {
	if (client->vMajor!=1) { /* pre 1.0 versions of Xlib have a bug */
	    stuff->minKeyCode= xkb->min_key_code;
	    stuff->maxKeyCode= xkb->max_key_code;
	}
	else {
	    if ((stuff->minKeyCode<XkbMinLegalKeyCode)||
				(stuff->maxKeyCode>XkbMaxLegalKeyCode)) {
		client->errorValue= _XkbErrCode3(2,stuff->minKeyCode,
							stuff->maxKeyCode);
		return BadValue;
	    }
	    if (stuff->minKeyCode>stuff->maxKeyCode) {
		client->errorValue= _XkbErrCode3(3,stuff->minKeyCode,
							stuff->maxKeyCode);
		return BadMatch;
	    }
	}
    }

    tmp = (char *)&stuff[1];
    if ((stuff->present&XkbKeyTypesMask)&&
	(!CheckKeyTypes(client,xkb,stuff,(xkbKeyTypeWireDesc **)&tmp,
						&nTypes,mapWidths))) {
	client->errorValue = nTypes;
	return BadValue;
    }
    if ((stuff->present&XkbKeySymsMask)&&
	(!CheckKeySyms(client,xkb,stuff,nTypes,mapWidths,symsPerKey,
					(xkbSymMapWireDesc **)&tmp,&error))) {
	client->errorValue = error;
	return BadValue;
    }

    if ((stuff->present&XkbKeyActionsMask)&&
	(!CheckKeyActions(xkb,stuff,nTypes,mapWidths,symsPerKey,
						(CARD8 **)&tmp,&nActions))) {
	client->errorValue = nActions;
	return BadValue;
    }

    if ((stuff->present&XkbKeyBehaviorsMask)&&
	(!CheckKeyBehaviors(xkb,stuff,(xkbBehaviorWireDesc**)&tmp,&error))) {
	client->errorValue = error;
	return BadValue;
    }

    if ((stuff->present&XkbVirtualModsMask)&&
	(!CheckVirtualMods(xkb,stuff,(CARD8 **)&tmp,&error))) {
	client->errorValue= error;
	return BadValue;
    }
    if ((stuff->present&XkbExplicitComponentsMask)&&
	(!CheckKeyExplicit(xkb,stuff,(CARD8 **)&tmp,&error))) {
	client->errorValue= error;
	return BadValue;
    }
    if ((stuff->present&XkbModifierMapMask)&&
	(!CheckModifierMap(xkb,stuff,(CARD8 **)&tmp,&error))) {
	client->errorValue= error;
	return BadValue;
    }
    if ((stuff->present&XkbVirtualModMapMask)&&
	(!CheckVirtualModMap(xkb,stuff,(xkbVModMapWireDesc **)&tmp,&error))) {
	client->errorValue= error;
	return BadValue;
    }
    if (((tmp-((char *)stuff))/4)!=stuff->length) {
	ErrorF("Internal error! Bad length in XkbSetMap (after check)\n");
	client->errorValue = tmp-((char *)&stuff[1]);
	return BadLength;
    }
    bzero(&change,sizeof(change));
    sentNKN= False;
    if ((xkb->min_key_code!=stuff->minKeyCode)||
    				(xkb->max_key_code!=stuff->maxKeyCode)) {
	Status			status;
	xkbNewKeyboardNotify	nkn;
	nkn.deviceID= nkn.oldDeviceID= dev->id;
	nkn.oldMinKeyCode= xkb->min_key_code;
	nkn.oldMaxKeyCode= xkb->max_key_code;
	status= XkbChangeKeycodeRange(xkb,stuff->minKeyCode,stuff->maxKeyCode,
								&change);
	if (status!=Success)
	    return status;
	nkn.minKeyCode= xkb->min_key_code;
	nkn.maxKeyCode= xkb->max_key_code;
	nkn.requestMajor= XkbReqCode;
	nkn.requestMinor= X_kbSetMap;
	nkn.changed= XkbNKN_KeycodesMask;
	XkbSendNewKeyboardNotify(dev,&nkn);
	sentNKN= True;
    }
    tmp = (char *)&stuff[1];
    if (stuff->present&XkbKeyTypesMask) {
	tmp = SetKeyTypes(xkb,stuff,(xkbKeyTypeWireDesc *)tmp,&change);
	if (!tmp)	goto allocFailure;
    }
    if (stuff->present&XkbKeySymsMask) {
	tmp = SetKeySyms(client,xkb,stuff,(xkbSymMapWireDesc *)tmp,&change,dev);
	if (!tmp)	goto allocFailure;
    }
    if (stuff->present&XkbKeyActionsMask) {
	tmp = SetKeyActions(xkb,stuff,(CARD8 *)tmp,&change);
	if (!tmp)	goto allocFailure;
    }
    if (stuff->present&XkbKeyBehaviorsMask) {
	tmp= SetKeyBehaviors(xkbi,stuff,(xkbBehaviorWireDesc *)tmp,&change);
	if (!tmp)	goto allocFailure;
    }
    if (stuff->present&XkbVirtualModsMask)
	tmp= SetVirtualMods(xkbi,stuff,(CARD8 *)tmp,&change);
    if (stuff->present&XkbExplicitComponentsMask)
	tmp= SetKeyExplicit(xkbi,stuff,(CARD8 *)tmp,&change);
    if (stuff->present&XkbModifierMapMask)
	tmp= SetModifierMap(xkbi,stuff,(CARD8 *)tmp,&change);
    if (stuff->present&XkbVirtualModMapMask)
	tmp= SetVirtualModMap(xkbi,stuff,(xkbVModMapWireDesc *)tmp,&change);
    if (((tmp-((char *)stuff))/4)!=stuff->length) {
	ErrorF("Internal error! Bad length in XkbSetMap (after set)\n");
	client->errorValue = tmp-((char *)&stuff[1]);
	return BadLength;
    }
    if (stuff->flags&XkbSetMapRecomputeActions) {
	KeyCode		first,last,firstMM,lastMM;
	if (change.map.num_key_syms>0) {
	    first= change.map.first_key_sym;
	    last= first+change.map.num_key_syms-1;
	}
	else first= last= 0;
	if (change.map.num_modmap_keys>0) {
	    firstMM= change.map.first_modmap_key;
	    lastMM= first+change.map.num_modmap_keys-1;
	}
	else firstMM= lastMM= 0;
	if ((last>0) && (lastMM>0)) {
	    if (firstMM<first)
		first= firstMM;
	    if (lastMM>last)
		last= lastMM;
	}
	else if (lastMM>0) {
	    first= firstMM;
	    last= lastMM;
	}
	if (last>0) {
	    unsigned check= 0;
	    XkbUpdateActions(dev,first,(last-first+1),&change,&check,&cause);
	    if (check)
		XkbCheckSecondaryEffects(xkbi,check,&change,&cause);
	}
    }
    if (!sentNKN)
	XkbSendNotification(dev,&change,&cause);

    XkbUpdateCoreDescription(dev,False);
    return client->noClientException;
allocFailure:
    return BadAlloc;
}

/***====================================================================***/

static Status
#if NeedFunctionPrototypes
XkbComputeGetCompatMapReplySize(	XkbCompatMapPtr 	compat,
					xkbGetCompatMapReply *	rep)
#else
XkbComputeGetCompatMapReplySize(compat,rep)
    XkbCompatMapPtr		compat;
    xkbGetCompatMapReply *	rep;
#endif
{
unsigned	 size,nGroups;

    nGroups= 0;
    if (rep->groups!=0) {
	register int i,bit;
	for (i=0,bit=1;i<XkbNumKbdGroups;i++,bit<<=1) {
	    if (rep->groups&bit)
		nGroups++;
	}
    }
    size= nGroups*SIZEOF(xkbModsWireDesc);
    size+= (rep->nSI*SIZEOF(xkbSymInterpretWireDesc));
    rep->length= size/4;
    return Success;
}

static int
#if NeedFunctionPrototypes
XkbSendCompatMap(	ClientPtr 		client,
			XkbCompatMapPtr 	compat,
			xkbGetCompatMapReply *	rep)
#else
XkbSendCompatMap(client,compat,rep)
    ClientPtr			client;
    XkbCompatMapPtr		compat;
    xkbGetCompatMapReply *	rep;
#endif
{
char	*	data;
int		size;

    size= rep->length*4;
    if (size>0) {
	data = (char *)ALLOCATE_LOCAL(size);
	if (data) {
	    register unsigned i,bit;
	    xkbModsWireDesc *	grp;
	    XkbSymInterpretPtr	sym= &compat->sym_interpret[rep->firstSI];
	    xkbSymInterpretWireDesc *wire = (xkbSymInterpretWireDesc *)data;
	    for (i=0;i<rep->nSI;i++,sym++,wire++) {
		wire->sym= sym->sym;
		wire->mods= sym->mods;
		wire->match= sym->match;
		wire->virtualMod= sym->virtual_mod;
		wire->flags= sym->flags;
		memcpy((char*)&wire->act,(char*)&sym->act,sz_xkbActionWireDesc);
		if (client->swapped) {
		    register int n;
		    swapl(&wire->sym,n);
		}
	    }
	    if (rep->groups) {
		grp = (xkbModsWireDesc *)wire;
		for (i=0,bit=1;i<XkbNumKbdGroups;i++,bit<<=1) {
		    if (rep->groups&bit) {
			grp->mask= compat->groups[i].mask;
			grp->realMods= compat->groups[i].real_mods;
			grp->virtualMods= compat->groups[i].vmods;
			if (client->swapped) {
			    register int n;
			    swaps(&grp->virtualMods,n);
			}
			grp++;
		    }
		}
		wire= (xkbSymInterpretWireDesc*)grp;
	    }
	}
	else return BadAlloc;
    }
    else data= NULL;

    if (client->swapped) {
	register int n;
	swaps(&rep->sequenceNumber,n);
	swapl(&rep->length,n);
	swaps(&rep->firstSI,n);
	swaps(&rep->nSI,n);
	swaps(&rep->nTotalSI,n);
    }

    WriteToClient(client, SIZEOF(xkbGetCompatMapReply), (char *)rep);
    if (data) {
	WriteToClient(client, size, data);
	DEALLOCATE_LOCAL((char *)data);
    }
    return client->noClientException;
}

int
#if NeedFunctionPrototypes
ProcXkbGetCompatMap(ClientPtr client)
#else
ProcXkbGetCompatMap(client)
    ClientPtr client;
#endif
{
    xkbGetCompatMapReply 	rep;
    DeviceIntPtr 		dev;
    XkbDescPtr			xkb;
    XkbCompatMapPtr		compat;

    REQUEST(xkbGetCompatMapReq);
    REQUEST_SIZE_MATCH(xkbGetCompatMapReq);

    if (!(client->xkbClientFlags&_XkbClientInitialized))
	return BadAccess;

    CHK_KBD_DEVICE(dev,stuff->deviceSpec);

    xkb = dev->key->xkbInfo->desc;
    compat= xkb->compat;

    rep.type = X_Reply;
    rep.deviceID = dev->id;
    rep.sequenceNumber = client->sequence;
    rep.length = 0;
    rep.firstSI = stuff->firstSI;
    rep.nSI = stuff->nSI;
    if (stuff->getAllSI) {
	rep.firstSI = 0;
	rep.nSI = compat->num_si;
    }
    else if ((((unsigned)stuff->nSI)>0)&&
		((unsigned)(stuff->firstSI+stuff->nSI-1)>=compat->num_si)) {
	client->errorValue = _XkbErrCode2(0x05,compat->num_si);
	return BadValue;
    }
    rep.nTotalSI = compat->num_si;
    rep.groups= stuff->groups;
    XkbComputeGetCompatMapReplySize(compat,&rep);
    return XkbSendCompatMap(client,compat,&rep);
}

int
#if NeedFunctionPrototypes
ProcXkbSetCompatMap(ClientPtr client)
#else
ProcXkbSetCompatMap(client)
    ClientPtr client;
#endif
{
    DeviceIntPtr 	dev;
    XkbSrvInfoPtr 	xkbi;
    XkbDescPtr		xkb;
    XkbCompatMapPtr 	compat;
    char	*	data;
    int		 	nGroups;
    register unsigned	i,bit;

    REQUEST(xkbSetCompatMapReq);
    REQUEST_AT_LEAST_SIZE(xkbSetCompatMapReq);

    if (!(client->xkbClientFlags&_XkbClientInitialized))
	return BadAccess;

    CHK_KBD_DEVICE(dev,stuff->deviceSpec);

    data = (char *)&stuff[1];
    xkbi = dev->key->xkbInfo;
    xkb= xkbi->desc;
    compat= xkb->compat;
    if ((stuff->nSI>0)||(stuff->truncateSI)) {
	xkbSymInterpretWireDesc *wire;
	if (stuff->firstSI>compat->num_si) {
	    client->errorValue = _XkbErrCode2(0x02,compat->num_si);
	    return BadValue;
	}
	wire= (xkbSymInterpretWireDesc *)data;
	wire+= stuff->nSI;
	data = (char *)wire;
    }
    nGroups= 0;
    if (stuff->groups!=0) {
	for (i=0,bit=1;i<XkbNumKbdGroups;i++,bit<<=1) {
	    if ( stuff->groups&bit )
		nGroups++;
	}
    }
    data+= nGroups*SIZEOF(xkbModsWireDesc);
    if (((data-((char *)stuff))/4)!=stuff->length) {
	return BadLength;
    }
    data = (char *)&stuff[1];
    if (stuff->nSI>0) {
	xkbSymInterpretWireDesc *wire = (xkbSymInterpretWireDesc *)data;
	XkbSymInterpretPtr	sym;
	if ((unsigned)(stuff->firstSI+stuff->nSI)>compat->num_si) {
	    compat->num_si= stuff->firstSI+stuff->nSI;
	    compat->sym_interpret= _XkbTypedRealloc(compat->sym_interpret,
						   compat->num_si,
						   XkbSymInterpretRec);
	    if (!compat->sym_interpret) {
		compat->num_si= 0;
		return BadAlloc;
	    }
	}
	else if (stuff->truncateSI) {
	    compat->num_si = stuff->firstSI+stuff->nSI;
	}
	sym = &compat->sym_interpret[stuff->firstSI];
	for (i=0;i<stuff->nSI;i++,wire++,sym++) {
	    if (client->swapped) {
		register int n;
		swapl(&wire->sym,n);
	    }
	    sym->sym= wire->sym;
	    sym->mods= wire->mods;
	    sym->match= wire->match;
	    sym->flags= wire->flags;
	    sym->virtual_mod= wire->virtualMod;
	    memcpy((char *)&sym->act,(char *)&wire->act,
	    					SIZEOF(xkbActionWireDesc));
	}
	data = (char *)wire;
    }
    else if (stuff->truncateSI) {
	compat->num_si = stuff->firstSI;
    }

    if (stuff->groups!=0) {
	register unsigned i,bit;
	xkbModsWireDesc *wire = (xkbModsWireDesc *)data;
	for (i=0,bit=1;i<XkbNumKbdGroups;i++,bit<<=1) {
	    if (stuff->groups&bit) {
		if (client->swapped) {
		    register int n;
		    swaps(&wire->virtualMods,n);
		}
		compat->groups[i].mask= wire->realMods;
		compat->groups[i].real_mods= wire->realMods;
		compat->groups[i].vmods= wire->virtualMods;
		if (wire->virtualMods!=0) {
		    unsigned tmp;
		    tmp= XkbMaskForVMask(xkb,wire->virtualMods);
		    compat->groups[i].mask|= tmp;
		}
		data+= SIZEOF(xkbModsWireDesc);
		wire= (xkbModsWireDesc *)data;
	    }
	}
    }
    i= XkbPaddedSize((data-((char *)stuff)));
    if ((i/4)!=stuff->length) {
	ErrorF("Internal length error on read in ProcXkbSetCompatMap\n");
	return BadLength;
    }
    
    if (dev->xkb_interest) {
	xkbCompatMapNotify ev;
	ev.deviceID = dev->id;
	ev.changedGroups = stuff->groups;
	ev.firstSI = stuff->firstSI;
	ev.nSI = stuff->nSI;
	ev.nTotalSI = compat->num_si;
	XkbSendCompatMapNotify(dev,&ev);
    }

    if (stuff->recomputeActions) {
	XkbChangesRec		change;
	unsigned		check;
	XkbEventCauseRec	cause;

	XkbSetCauseXkbReq(&cause,X_kbSetCompatMap,client);
	bzero(&change,sizeof(XkbChangesRec));
	XkbUpdateActions(dev,xkb->min_key_code,XkbNumKeys(xkb),&change,&check,
									&cause);
	if (check)
	    XkbCheckSecondaryEffects(xkbi,check,&change,&cause);
	XkbUpdateCoreDescription(dev,False);
	XkbSendNotification(dev,&change,&cause);
    }
    return client->noClientException;
}

/***====================================================================***/

int
#if NeedFunctionPrototypes
ProcXkbGetIndicatorState(ClientPtr client)
#else
ProcXkbGetIndicatorState(client)
    ClientPtr client;
#endif
{
    xkbGetIndicatorStateReply 	rep;
    XkbSrvLedInfoPtr		sli;
    DeviceIntPtr 		dev;
    register int 		i;

    REQUEST(xkbGetIndicatorStateReq);
    REQUEST_SIZE_MATCH(xkbGetIndicatorStateReq);

    if (!(client->xkbClientFlags&_XkbClientInitialized))
	return BadAccess;

    CHK_KBD_DEVICE(dev,stuff->deviceSpec);

    sli= XkbFindSrvLedInfo(dev,XkbDfltXIClass,XkbDfltXIId,
						XkbXI_IndicatorStateMask);
    if (!sli)
	return BadAlloc;

    rep.type = X_Reply;
    rep.sequenceNumber = client->sequence;
    rep.length = 0;
    rep.deviceID = dev->id;
    rep.state = sli->effectiveState;

    if (client->swapped) {
	swaps(&rep.sequenceNumber,i);
	swapl(&rep.state,i);
    }
    WriteToClient(client, SIZEOF(xkbGetIndicatorStateReply), (char *)&rep);
    return client->noClientException;
}

/***====================================================================***/

Status
#if NeedFunctionPrototypes
XkbComputeGetIndicatorMapReplySize(
    XkbIndicatorPtr		indicators,
    xkbGetIndicatorMapReply	*rep)
#else
XkbComputeGetIndicatorMapReplySize(indicators,rep)
    XkbIndicatorPtr		indicators;
    xkbGetIndicatorMapReply	*rep;
#endif
{
register int 	i,bit;
int		nIndicators;

    rep->realIndicators = indicators->phys_indicators;
    for (i=nIndicators=0,bit=1;i<XkbNumIndicators;i++,bit<<=1) {
	if (rep->which&bit)
	    nIndicators++;
    }
    rep->length = (nIndicators*SIZEOF(xkbIndicatorMapWireDesc))/4;
    return Success;
}

int
#if NeedFunctionPrototypes
XkbSendIndicatorMap(	ClientPtr			client,
			XkbIndicatorPtr			indicators,
			xkbGetIndicatorMapReply *	rep)
#else
XkbSendIndicatorMap(client,indicators,rep)
    ClientPtr			client;
    XkbIndicatorPtr		indicators;
    xkbGetIndicatorMapReply *	rep;
#endif
{
int 			length;
CARD8 *			map;
register int		i;
register unsigned	bit;

    length = rep->length*4;
    if (length>0) {
	CARD8 *to;
	to= map= (CARD8 *)ALLOCATE_LOCAL(length);
	if (map) {
	    xkbIndicatorMapWireDesc  *wire = (xkbIndicatorMapWireDesc *)to;
	    for (i=0,bit=1;i<XkbNumIndicators;i++,bit<<=1) {
		if (rep->which&bit) {
		    wire->flags= indicators->maps[i].flags;
		    wire->whichGroups= indicators->maps[i].which_groups;
		    wire->groups= indicators->maps[i].groups;
		    wire->whichMods= indicators->maps[i].which_mods;
		    wire->mods= indicators->maps[i].mods.mask;
		    wire->realMods= indicators->maps[i].mods.real_mods;
		    wire->virtualMods= indicators->maps[i].mods.vmods;
		    wire->ctrls= indicators->maps[i].ctrls;
		    if (client->swapped) {
			register int n;
			swaps(&wire->virtualMods,n);
			swapl(&wire->ctrls,n);
		    }
		    wire++;
		}
	    }
	    to = (CARD8 *)wire;
	    if ((to-map)!=length) {
		client->errorValue = _XkbErrCode2(0xff,length);
		return BadLength;
	    }
	}
	else return BadAlloc;
    }
    else map = NULL;
    if (client->swapped) {
	swaps(&rep->sequenceNumber,i);
	swapl(&rep->length,i);
	swapl(&rep->which,i);
	swapl(&rep->realIndicators,i);
    }
    WriteToClient(client, SIZEOF(xkbGetIndicatorMapReply), (char *)rep);
    if (map) {
	WriteToClient(client, length, (char *)map);
	DEALLOCATE_LOCAL((char *)map);
    }
    return client->noClientException;
}

int
#if NeedFunctionPrototypes
ProcXkbGetIndicatorMap(ClientPtr client)
#else
ProcXkbGetIndicatorMap(client)
    ClientPtr client;
#endif
{
xkbGetIndicatorMapReply rep;
DeviceIntPtr		dev;
XkbDescPtr		xkb;
XkbIndicatorPtr		leds;

    REQUEST(xkbGetIndicatorMapReq);
    REQUEST_SIZE_MATCH(xkbGetIndicatorMapReq);

    if (!(client->xkbClientFlags&_XkbClientInitialized))
	return BadAccess;

    CHK_KBD_DEVICE(dev,stuff->deviceSpec);

    xkb= dev->key->xkbInfo->desc;
    leds= xkb->indicators;

    rep.type = X_Reply;
    rep.sequenceNumber = client->sequence;
    rep.length = 0;
    rep.deviceID = dev->id;
    rep.which = stuff->which;
    XkbComputeGetIndicatorMapReplySize(leds,&rep);
    return XkbSendIndicatorMap(client,leds,&rep);
}

int
#if NeedFunctionPrototypes
ProcXkbSetIndicatorMap(ClientPtr client)
#else
ProcXkbSetIndicatorMap(client)
    ClientPtr client;
#endif
{
    register int 	i,bit;
    int			nIndicators,why;
    DeviceIntPtr 	dev;
    XkbSrvInfoPtr	xkbi;
    xkbIndicatorMapWireDesc *from;
    XkbSrvLedInfoPtr	sli;
    XkbEventCauseRec	cause;

    REQUEST(xkbSetIndicatorMapReq);
    REQUEST_AT_LEAST_SIZE(xkbSetIndicatorMapReq);

    if (!(client->xkbClientFlags&_XkbClientInitialized))
	return BadAccess;

    dev = _XkbLookupKeyboard(stuff->deviceSpec,&why);
    if (!dev) {
	client->errorValue = _XkbErrCode2(why,stuff->deviceSpec);
	return XkbKeyboardErrorCode;
    }
    xkbi= dev->key->xkbInfo;

    if (stuff->which==0)
	return client->noClientException;

    for (nIndicators=i=0,bit=1;i<XkbNumIndicators;i++,bit<<=1) {
	if (stuff->which&bit)
	    nIndicators++;
    }
    if (stuff->length!=((SIZEOF(xkbSetIndicatorMapReq)+
			(nIndicators*SIZEOF(xkbIndicatorMapWireDesc)))/4)) {
	return BadLength;
    }

    sli= XkbFindSrvLedInfo(dev,XkbDfltXIClass,XkbDfltXIId,
						XkbXI_IndicatorMapsMask);
    if (!sli)
	return BadAlloc;

    from = (xkbIndicatorMapWireDesc *)&stuff[1];
    for (i=0,bit=1;i<XkbNumIndicators;i++,bit<<=1) {
	if (stuff->which&bit) {
	    if (client->swapped) {
		register int n;
		swaps(&from->virtualMods,n);
		swapl(&from->ctrls,n);
	    }
	    CHK_MASK_LEGAL(i,from->whichGroups,XkbIM_UseAnyGroup);
	    CHK_MASK_LEGAL(i,from->whichMods,XkbIM_UseAnyMods);
	    from++;
	}
    }

    from = (xkbIndicatorMapWireDesc *)&stuff[1];
    for (i=0,bit=1;i<XkbNumIndicators;i++,bit<<=1) {
	if (stuff->which&bit) {
	    sli->maps[i].flags = from->flags;
	    sli->maps[i].which_groups = from->whichGroups;
	    sli->maps[i].groups = from->groups;
	    sli->maps[i].which_mods = from->whichMods;
	    sli->maps[i].mods.mask = from->mods;
	    sli->maps[i].mods.real_mods = from->mods;
	    sli->maps[i].mods.vmods= from->virtualMods;
	    sli->maps[i].ctrls = from->ctrls;
	    if (from->virtualMods!=0) {
		unsigned tmp;
		tmp= XkbMaskForVMask(xkbi->desc,from->virtualMods);
		sli->maps[i].mods.mask= from->mods|tmp;
	    }
	    from++;
	}
    }

    XkbSetCauseXkbReq(&cause,X_kbSetIndicatorMap,client);
    XkbApplyLedMapChanges(dev,sli,stuff->which,NULL,NULL,&cause);
    return client->noClientException;
}

/***====================================================================***/

int
#if NeedFunctionPrototypes
ProcXkbGetNamedIndicator(ClientPtr client)
#else
ProcXkbGetNamedIndicator(client)
    ClientPtr client;
#endif
{
    DeviceIntPtr 		dev;
    xkbGetNamedIndicatorReply 	rep;
    register int		i;
    XkbSrvLedInfoPtr		sli;
    XkbIndicatorMapPtr		map;
    Bool			supported;

    REQUEST(xkbGetNamedIndicatorReq);
    REQUEST_SIZE_MATCH(xkbGetNamedIndicatorReq);

    if (!(client->xkbClientFlags&_XkbClientInitialized))
	return BadAccess;

    CHK_LED_DEVICE(dev,stuff->deviceSpec);
    CHK_ATOM_ONLY(stuff->indicator);

    sli= XkbFindSrvLedInfo(dev,stuff->ledClass,stuff->ledID,0);
    if (!sli)
	return BadAlloc;

    supported= True;
    if (XkbXIUnsupported&XkbXI_IndicatorsMask) {
	if ((dev!=(DeviceIntPtr)LookupKeyboardDevice())||
					((sli->flags&XkbSLI_IsDefault)==0)) {
	    supported= False;
	}
    }

    if (supported) {
	i= 0;
	map= NULL;
	if ((sli->names)&&(sli->maps)) {
	    for (i=0;i<XkbNumIndicators;i++) {
		if (stuff->indicator==sli->names[i]) {
		    map= &sli->maps[i];
		    break;
		}
	    }
	}
    }

    rep.type= X_Reply;
    rep.length = 0;
    rep.sequenceNumber = client->sequence;
    rep.deviceID = dev->id;
    rep.indicator= stuff->indicator;
    if ((map!=NULL)&&(supported)) {
	rep.found= 		True;
	rep.on=			((sli->effectiveState&(1<<i))!=0);
	rep.realIndicator=	((sli->physIndicators&(1<<i))!=0);
	rep.ndx= 		i;
	rep.flags= 		map->flags;
	rep.whichGroups= 	map->which_groups;
	rep.groups= 		map->groups;
	rep.whichMods= 		map->which_mods;
	rep.mods= 		map->mods.mask;
	rep.realMods= 		map->mods.real_mods;
	rep.virtualMods= 	map->mods.vmods;
	rep.ctrls= 		map->ctrls;
	rep.supported= 		True;
    }
    else  {
	rep.found= 		False;
	rep.on= 		False;
	rep.realIndicator= 	False;
	rep.ndx= 		XkbNoIndicator;
	rep.flags= 		0;
	rep.whichGroups= 	0;
	rep.groups= 		0;
	rep.whichMods= 		0;
	rep.mods=		0;
	rep.realMods= 		0;
	rep.virtualMods= 	0;
	rep.ctrls= 		0;
	rep.supported= 		supported;
    }
    if ( client->swapped ) {
	register int n;
	swapl(&rep.length,n);
	swaps(&rep.sequenceNumber,n);
	swapl(&rep.indicator,n);
	swaps(&rep.virtualMods,n);
	swapl(&rep.ctrls,n);
    }

    WriteToClient(client,SIZEOF(xkbGetNamedIndicatorReply), (char *)&rep);
    if (!supported) {
	xkbExtensionDeviceNotify        edev;

	bzero(&edev,sizeof(xkbExtensionDeviceNotify));
	edev.reason=            XkbXI_UnsupportedFeatureMask;
	edev.ledClass=          stuff->ledClass;
	edev.ledID=             stuff->ledID;
	edev.ledsDefined=       sli->namesPresent|sli->mapsPresent;
	edev.ledState=          sli->effectiveState;
	edev.firstBtn=          0;
	edev.nBtns=             0;
	edev.unsupported=       XkbXIUnsupported&XkbXI_IndicatorsMask;
	edev.supported=         XkbXI_AllFeaturesMask&(~XkbXIUnsupported);
	XkbSendExtensionDeviceNotify(dev,client,&edev);
    }
    return client->noClientException;
}

int
#if NeedFunctionPrototypes
ProcXkbSetNamedIndicator(ClientPtr client)
#else
ProcXkbSetNamedIndicator(client)
    ClientPtr client;
#endif
{
    DeviceIntPtr 		dev,kbd;
    XkbIndicatorMapPtr		map;
    XkbSrvLedInfoPtr 		sli;
    register int		led;
    unsigned			extDevReason;
    unsigned			statec,namec,mapc;
    XkbEventCauseRec		cause;
    xkbExtensionDeviceNotify	ed;
    XkbChangesRec		changes;

    REQUEST(xkbSetNamedIndicatorReq);
    REQUEST_SIZE_MATCH(xkbSetNamedIndicatorReq);

    if (!(client->xkbClientFlags&_XkbClientInitialized))
	return BadAccess;

    CHK_LED_DEVICE(dev,stuff->deviceSpec);
    CHK_ATOM_ONLY(stuff->indicator);
    CHK_MASK_LEGAL(0x10,stuff->whichGroups,XkbIM_UseAnyGroup);
    CHK_MASK_LEGAL(0x11,stuff->whichMods,XkbIM_UseAnyMods);

    extDevReason= 0;

    sli= XkbFindSrvLedInfo(dev,stuff->ledClass,stuff->ledID,
							XkbXI_IndicatorsMask);
    if (!sli)
	return BadAlloc;

    if (XkbXIUnsupported&XkbXI_IndicatorsMask) {
	if ((dev!=(DeviceIntPtr)LookupKeyboardDevice())||
					((sli->flags&XkbSLI_IsDefault)==0)) {
	    bzero(&ed,sizeof(xkbExtensionDeviceNotify));
	    ed.reason=            XkbXI_UnsupportedFeatureMask;
	    ed.ledClass=          stuff->ledClass;
	    ed.ledID=             stuff->ledID;
	    ed.ledsDefined=       sli->namesPresent|sli->mapsPresent;
	    ed.ledState=          sli->effectiveState;
	    ed.firstBtn=          0;
	    ed.nBtns=             0;
	    ed.unsupported=       XkbXIUnsupported&XkbXI_IndicatorsMask;
	    ed.supported=         XkbXI_AllFeaturesMask&(~XkbXIUnsupported);
	    XkbSendExtensionDeviceNotify(dev,client,&ed);
	    return client->noClientException;
	}
    }

    statec= mapc= namec= 0;
    map= NULL;
    if (sli->names && sli->maps) {
	for (led=0;(led<XkbNumIndicators)&&(map==NULL);led++) {
	    if (sli->names[led]==stuff->indicator) {
		map= &sli->maps[led];
		break;
	    }
	}
    }
    if (map==NULL) {
	if (!stuff->createMap)
	    return client->noClientException;
	for (led=0,map=NULL;(led<XkbNumIndicators)&&(map==NULL);led++) {
	    if ((sli->names[led]==None)&&(!XkbIM_InUse(&sli->maps[led]))) {
		map= &sli->maps[led];
		sli->names[led]= stuff->indicator;
		break;
	    }
	}
	if (map==NULL)
	    return client->noClientException;
	namec|= (1<<led);
	sli->namesPresent|= ((stuff->indicator!=None)?(1<<led):0);
	extDevReason|= XkbXI_IndicatorNamesMask;
    }

    if (stuff->setMap) {
	map->flags = stuff->flags;
	map->which_groups = stuff->whichGroups;
	map->groups = stuff->groups;
	map->which_mods = stuff->whichMods;
	map->mods.mask = stuff->realMods;
	map->mods.real_mods = stuff->realMods;
	map->mods.vmods= stuff->virtualMods;
	map->ctrls = stuff->ctrls;
	mapc|= (1<<led);
    }
    if ((stuff->setState)&&((map->flags&XkbIM_NoExplicit)==0)) {
	if (stuff->on)	sli->explicitState|=  (1<<led);
	else		sli->explicitState&= ~(1<<led);
	statec|= ((sli->effectiveState^sli->explicitState)&(1<<led));
    }
    bzero((char *)&ed,sizeof(xkbExtensionDeviceNotify));
    bzero((char *)&changes,sizeof(XkbChangesRec));
    XkbSetCauseXkbReq(&cause,X_kbSetNamedIndicator,client);
    if (namec)
	XkbApplyLedNameChanges(dev,sli,namec,&ed,&changes,&cause);
    if (mapc)
	XkbApplyLedMapChanges(dev,sli,mapc,&ed,&changes,&cause);
    if (statec)
	XkbApplyLedStateChanges(dev,sli,statec,&ed,&changes,&cause);

    kbd= dev;
    if ((sli->flags&XkbSLI_HasOwnState)==0)
	kbd= (DeviceIntPtr)LookupKeyboardDevice();
    XkbFlushLedEvents(dev,kbd,sli,&ed,&changes,&cause);
    return client->noClientException;
}

/***====================================================================***/

static CARD32
#if NeedFunctionPrototypes
_XkbCountAtoms(Atom *atoms,int maxAtoms,int *count)
#else
_XkbCountAtoms(atoms,maxAtoms,count)
    Atom *atoms;
    int   maxAtoms;
    int  *count;
#endif
{
register unsigned int i,bit,nAtoms;
register CARD32 atomsPresent;

    for (i=nAtoms=atomsPresent=0,bit=1;i<maxAtoms;i++,bit<<=1) {
	if (atoms[i]!=None) {
	    atomsPresent|= bit;
	    nAtoms++;
	}
    }
    if (count)
	*count= nAtoms;
    return atomsPresent;
}

static char *
#if NeedFunctionPrototypes
_XkbWriteAtoms(char *wire,Atom *atoms,int maxAtoms,int swap)
#else
_XkbWriteAtoms(wire,atoms,maxAtoms,swap)
    char *wire;
    Atom *atoms;
    int   maxAtoms;
    int   swap;
#endif
{
register unsigned int i;
Atom *atm;

    atm = (Atom *)wire;
    for (i=0;i<maxAtoms;i++) {
	if (atoms[i]!=None) {
	    *atm= atoms[i];
	    if (swap) {
		register int n;
		swapl(atm,n);
	    }
	    atm++;
	}
    }
    return (char *)atm;
}

static Status
#if NeedFunctionPrototypes
XkbComputeGetNamesReplySize(XkbDescPtr xkb,xkbGetNamesReply *rep)
#else
XkbComputeGetNamesReplySize(xkb,rep)
    XkbDescPtr		xkb;
    xkbGetNamesReply *	rep;
#endif
{
register unsigned	which,length;
register int		i;

    rep->minKeyCode= xkb->min_key_code;
    rep->maxKeyCode= xkb->max_key_code;
    which= rep->which;
    length= 0;
    if (xkb->names!=NULL) {
	 if (which&XkbKeycodesNameMask)		length++;
	 if (which&XkbGeometryNameMask)		length++;
	 if (which&XkbSymbolsNameMask)		length++;
	 if (which&XkbPhysSymbolsNameMask)	length++;
	 if (which&XkbTypesNameMask)		length++;
	 if (which&XkbCompatNameMask)		length++;
    }
    else which&= ~XkbComponentNamesMask;

    if (xkb->map!=NULL) {
	if (which&XkbKeyTypeNamesMask)
	    length+= xkb->map->num_types;
	rep->nTypes= xkb->map->num_types;
	if (which&XkbKTLevelNamesMask) {
	    XkbKeyTypePtr	pType = xkb->map->types;
	    int			nKTLevels = 0;

	    length+= XkbPaddedSize(xkb->map->num_types)/4;
	    for (i=0;i<xkb->map->num_types;i++,pType++) {
		if (pType->level_names!=NULL)
		    nKTLevels+= pType->num_levels;
	    }
	    rep->nKTLevels= nKTLevels;
	    length+= nKTLevels;
	}
    }
    else {
	rep->nTypes=    0;
	rep->nKTLevels= 0;
	which&= ~(XkbKeyTypeNamesMask|XkbKTLevelNamesMask);
    }

    rep->minKeyCode= xkb->min_key_code;
    rep->maxKeyCode= xkb->max_key_code;
    rep->indicators= 0;
    rep->virtualMods= 0;
    rep->groupNames= 0;
    if (xkb->names!=NULL) {
	if (which&XkbIndicatorNamesMask) {
	    int nLeds;
	    rep->indicators= 
		_XkbCountAtoms(xkb->names->indicators,XkbNumIndicators,&nLeds);
	    length+= nLeds;
	    if (nLeds==0)
		which&= ~XkbIndicatorNamesMask;
	}

	if (which&XkbVirtualModNamesMask) {
	    int nVMods;
	    rep->virtualMods= 
		_XkbCountAtoms(xkb->names->vmods,XkbNumVirtualMods,&nVMods);
	    length+= nVMods;
	    if (nVMods==0)
		which&= ~XkbVirtualModNamesMask;
	}

	if (which&XkbGroupNamesMask) {
	    int nGroups;
	    rep->groupNames=
		_XkbCountAtoms(xkb->names->groups,XkbNumKbdGroups,&nGroups);
	    length+= nGroups;
	    if (nGroups==0)
		which&= ~XkbGroupNamesMask;
	}

	if ((which&XkbKeyNamesMask)&&(xkb->names->keys))
	     length+= rep->nKeys;
	else which&= ~XkbKeyNamesMask;

	if ((which&XkbKeyAliasesMask)&&
	    (xkb->names->key_aliases)&&(xkb->names->num_key_aliases>0)) {
	    rep->nKeyAliases= xkb->names->num_key_aliases;
	    length+= rep->nKeyAliases*2;
	} 
	else {
	    which&= ~XkbKeyAliasesMask;
	    rep->nKeyAliases= 0;
	}

	if ((which&XkbRGNamesMask)&&(xkb->names->num_rg>0))
	     length+= xkb->names->num_rg;
	else which&= ~XkbRGNamesMask;
    }
    else {
	which&= ~(XkbIndicatorNamesMask|XkbVirtualModNamesMask);
	which&= ~(XkbGroupNamesMask|XkbKeyNamesMask|XkbKeyAliasesMask);
	which&= ~XkbRGNamesMask;
    }

    rep->length= length;
    rep->which= which;
    return Success;
}

static int
#if NeedFunctionPrototypes
XkbSendNames(ClientPtr client,XkbDescPtr xkb,xkbGetNamesReply *rep)
#else
XkbSendNames(client,xkb,rep)
    ClientPtr		client;
    XkbDescPtr		xkb;
    xkbGetNamesReply *	rep;
#endif
{
register unsigned 	i,length,which;
char *			start;
char *			desc;

    length= rep->length*4;
    which= rep->which;
    if (client->swapped) {
	register int n;
	swaps(&rep->sequenceNumber,n);
	swapl(&rep->length,n);
	swapl(&rep->which,n);
	swaps(&rep->virtualMods,n);
	swapl(&rep->indicators,n);
    }

    start = desc = (char *)ALLOCATE_LOCAL(length);
    if ( !start )
	return BadAlloc;
    if (which&XkbKeycodesNameMask) {
	*((CARD32 *)desc)= xkb->names->keycodes;
	if (client->swapped) {
	    register int n;
	    swapl(desc,n);
	}
	desc+= 4;
    }
    if (which&XkbGeometryNameMask)  {
	*((CARD32 *)desc)= xkb->names->geometry;
	if (client->swapped) {
	    register int n;
	    swapl(desc,n);
	}
	desc+= 4;
    }
    if (which&XkbSymbolsNameMask) {
	*((CARD32 *)desc)= xkb->names->symbols;
	if (client->swapped) {
	    register int n;
	    swapl(desc,n);
	}
	desc+= 4;
    }
    if (which&XkbPhysSymbolsNameMask) {
	register CARD32 *atm= (CARD32 *)desc;
	atm[0]= (CARD32)xkb->names->phys_symbols;
	if (client->swapped) {
	    register int n;
	    swapl(&atm[0],n);
	}
	desc+= 4;
    }
    if (which&XkbTypesNameMask) {
	*((CARD32 *)desc)= (CARD32)xkb->names->types;
	if (client->swapped) {
	    register int n;
	    swapl(desc,n);
	}
	desc+= 4;
    }
    if (which&XkbCompatNameMask) {
	*((CARD32 *)desc)= (CARD32)xkb->names->compat;
	if (client->swapped) {
	    register int n;
	    swapl(desc,n);
	}
	desc+= 4;
    }
    if (which&XkbKeyTypeNamesMask) {
	register CARD32 *atm= (CARD32 *)desc;
	register XkbKeyTypePtr type= xkb->map->types;

	for (i=0;i<xkb->map->num_types;i++,atm++,type++) {
	    *atm= (CARD32)type->name;
	    if (client->swapped) {
		register int n;
		swapl(atm,n);
	    }
	}
	desc= (char *)atm;
    }
    if (which&XkbKTLevelNamesMask) {
	XkbKeyTypePtr type = xkb->map->types;
	register CARD32 *atm;
	for (i=0;i<rep->nTypes;i++,type++) {
	    *desc++ = type->num_levels;
	}
	desc+= XkbPaddedSize(rep->nTypes)-rep->nTypes;

	atm= (CARD32 *)desc;
	type = xkb->map->types;
	for (i=0;i<xkb->map->num_types;i++,type++) {
	    register unsigned l;
	    if (type->level_names) {
		for (l=0;l<type->num_levels;l++,atm++) {
		    *atm= type->level_names[l];
		    if (client->swapped) {
			register unsigned n;
			swapl(atm,n);
		    }
		}
		desc+= type->num_levels*4;
	    }
	}
    }
    if (which&XkbIndicatorNamesMask) {
	desc= _XkbWriteAtoms(desc,xkb->names->indicators,XkbNumIndicators,
							 client->swapped);
    }
    if (which&XkbVirtualModNamesMask) {
	desc= _XkbWriteAtoms(desc,xkb->names->vmods,XkbNumVirtualMods,
							client->swapped);
    }
    if (which&XkbGroupNamesMask) {
	desc= _XkbWriteAtoms(desc,xkb->names->groups,XkbNumKbdGroups,
							client->swapped);
    }
    if (which&XkbKeyNamesMask) {
	for (i=0;i<rep->nKeys;i++,desc+= sizeof(XkbKeyNameRec)) {
	    *((XkbKeyNamePtr)desc)= xkb->names->keys[i+rep->firstKey];
	}
    }
    if (which&XkbKeyAliasesMask) {
	XkbKeyAliasPtr	pAl;
	pAl= xkb->names->key_aliases;
	for (i=0;i<rep->nKeyAliases;i++,pAl++,desc+=2*XkbKeyNameLength) {
	    *((XkbKeyAliasPtr)desc)= *pAl;
	}
    }
    if ((which&XkbRGNamesMask)&&(rep->nRadioGroups>0)) {
	register CARD32	*atm= (CARD32 *)desc;
	for (i=0;i<rep->nRadioGroups;i++,atm++) {
	    *atm= (CARD32)xkb->names->radio_groups[i];
	    if (client->swapped) {
		register unsigned n;
		swapl(atm,n);
	    }
	}
	desc+= rep->nRadioGroups*4;
    }
    if ((desc-start)!=(length)) {
	ErrorF("BOGUS LENGTH in write names, expected %d, got %d\n",
					length, desc-start);
    }
    WriteToClient(client, SIZEOF(xkbGetNamesReply), (char *)rep);
    WriteToClient(client, length, start);
    DEALLOCATE_LOCAL((char *)start);
    return client->noClientException;
}

int
#if NeedFunctionPrototypes
ProcXkbGetNames(ClientPtr client)
#else
ProcXkbGetNames(client)
    ClientPtr client;
#endif
{
    DeviceIntPtr	dev;
    XkbDescPtr		xkb;
    xkbGetNamesReply 	rep;

    REQUEST(xkbGetNamesReq);
    REQUEST_SIZE_MATCH(xkbGetNamesReq);

    if (!(client->xkbClientFlags&_XkbClientInitialized))
	return BadAccess;

    CHK_KBD_DEVICE(dev,stuff->deviceSpec);
    CHK_MASK_LEGAL(0x01,stuff->which,XkbAllNamesMask);

    xkb = dev->key->xkbInfo->desc;
    rep.type= X_Reply;
    rep.sequenceNumber= client->sequence;
    rep.length = 0;
    rep.deviceID = dev->id;
    rep.which = stuff->which;
    rep.nTypes = xkb->map->num_types;
    rep.firstKey = xkb->min_key_code;
    rep.nKeys = XkbNumKeys(xkb);
    if (xkb->names!=NULL) {
	rep.nKeyAliases= xkb->names->num_key_aliases;
	rep.nRadioGroups = xkb->names->num_rg;
    }
    else {
	rep.nKeyAliases= rep.nRadioGroups= 0;
    }
    XkbComputeGetNamesReplySize(xkb,&rep);
    return XkbSendNames(client,xkb,&rep);
}

/***====================================================================***/

static CARD32 *
#if NeedFunctionPrototypes
_XkbCheckAtoms(CARD32 *wire,int nAtoms,int swapped,Atom *pError)
#else
_XkbCheckAtoms(wire,nAtoms,swapped,pError)
    CARD32 *wire;
    int   nAtoms;
    int   swapped;
    Atom *pError;
#endif
{
register int i;

    for (i=0;i<nAtoms;i++,wire++) {
	if (swapped) {
	    register int n;
	    swapl(wire,n);
	}
	if ((((Atom)*wire)!=None)&&(!ValidAtom((Atom)*wire))) {
	    *pError= ((Atom)*wire);
	    return NULL;
	}
    }
    return wire;
}

static CARD32 *
#if NeedFunctionPrototypes
_XkbCheckMaskedAtoms(CARD32 *wire,int nAtoms,CARD32 present,int swapped,
								Atom *pError)
#else
_XkbCheckMaskedAtoms(wire,nAtoms,present,swapped,pError)
    CARD32	*wire;
    int   	 nAtoms;
    CARD32	 present;
    int		 swapped;
    Atom	*pError;
#endif
{
register unsigned i,bit;

    for (i=0,bit=1;(i<nAtoms)&&(present);i++,bit<<=1) {
	if ((present&bit)==0)
	    continue;
	if (swapped) {
	    register int n;
	    swapl(wire,n);
	}
	if ((((Atom)*wire)!=None)&&(!ValidAtom(((Atom)*wire)))) {
	    *pError= (Atom)*wire;
	    return NULL;
	}
	wire++;
    }
    return wire;
}

static Atom *
#if NeedFunctionPrototypes
_XkbCopyMaskedAtoms(	Atom	*wire,
    			Atom	*dest,
			int   	 nAtoms,
			CARD32	 present)
#else
_XkbCopyMaskedAtoms(wire,dest,nAtoms,present)
    Atom	*wire;
    Atom	*dest;
    int   	 nAtoms;
    CARD32	 present;
#endif
{
register int i,bit;

    for (i=0,bit=1;(i<nAtoms)&&(present);i++,bit<<=1) {
	if ((present&bit)==0)
	    continue;
	dest[i]= *wire++;
    }
    return wire;
}

static Bool
#if NeedFunctionPrototypes
_XkbCheckTypeName(Atom name,int typeNdx)
#else
_XkbCheckTypeName(name,typeNdx)
    Atom	name;
    int		typeNdx;
#endif
{
char *	str;

    str= NameForAtom(name);
    if ((strcmp(str,"ONE_LEVEL")==0)||(strcmp(str,"TWO_LEVEL")==0)||
	(strcmp(str,"ALPHABETIC")==0)||(strcmp(str,"KEYPAD")==0))
	return False;
    return True;
}

int
#if NeedFunctionPrototypes
ProcXkbSetNames(ClientPtr client)
#else
ProcXkbSetNames(client)
    ClientPtr client;
#endif
{
    DeviceIntPtr	 dev;
    XkbDescRec		*xkb;
    XkbNamesRec		*names;
    xkbNamesNotify	 nn;
    CARD32		*tmp;
    Atom		 bad;

    REQUEST(xkbSetNamesReq);
    REQUEST_AT_LEAST_SIZE(xkbSetNamesReq);

    if (!(client->xkbClientFlags&_XkbClientInitialized))
	return BadAccess;

    CHK_KBD_DEVICE(dev,stuff->deviceSpec);
    CHK_MASK_LEGAL(0x01,stuff->which,XkbAllNamesMask);

    xkb = dev->key->xkbInfo->desc;
    names = xkb->names;
    tmp = (CARD32 *)&stuff[1];

    if (stuff->which&XkbKeycodesNameMask) {
	tmp= _XkbCheckAtoms(tmp,1,client->swapped,&bad);
	if (!tmp) {
	    client->errorValue = bad;
	    return BadAtom;
	}
    }
    if (stuff->which&XkbGeometryNameMask) {
	tmp= _XkbCheckAtoms(tmp,1,client->swapped,&bad);
	if (!tmp) {
	    client->errorValue = bad;
	    return BadAtom;
	}
    }
    if (stuff->which&XkbSymbolsNameMask) {
	tmp= _XkbCheckAtoms(tmp,1,client->swapped,&bad);
	if (!tmp) {
	    client->errorValue = bad;
	    return BadAtom;
	}
    }
    if (stuff->which&XkbPhysSymbolsNameMask) {
	tmp= _XkbCheckAtoms(tmp,1,client->swapped,&bad);
	if (!tmp) {
	    client->errorValue= bad;
	    return BadAtom;
	}
    }
    if (stuff->which&XkbTypesNameMask) {
	tmp= _XkbCheckAtoms(tmp,1,client->swapped,&bad);
	if (!tmp) {
	    client->errorValue = bad;
	    return BadAtom;
	}
    }
    if (stuff->which&XkbCompatNameMask) {
	tmp= _XkbCheckAtoms(tmp,1,client->swapped,&bad);
	if (!tmp) {
	    client->errorValue = bad;
	    return BadAtom;
	}
    }
    if (stuff->which&XkbKeyTypeNamesMask) {
	register int i;
	CARD32	*old;
	if ( stuff->nTypes<1 ) {
	    client->errorValue = _XkbErrCode2(0x02,stuff->nTypes);
	    return BadValue;
	}
	if ((unsigned)(stuff->firstType+stuff->nTypes-1)>=xkb->map->num_types) {
	    client->errorValue = _XkbErrCode4(0x03,stuff->firstType,
							stuff->nTypes,
							xkb->map->num_types);
	    return BadValue;
	}
	if (((unsigned)stuff->firstType)<=XkbLastRequiredType) {
	    client->errorValue = _XkbErrCode2(0x04,stuff->firstType);
	    return BadAccess;
	}
	old= tmp;
	tmp= _XkbCheckAtoms(tmp,stuff->nTypes,client->swapped,&bad);
	if (!tmp) {
	    client->errorValue= bad;
	    return BadAtom;
	}
	for (i=0;i<stuff->nTypes;i++,old++) {
	    if (!_XkbCheckTypeName((Atom)*old,stuff->firstType+i))
		client->errorValue= _XkbErrCode2(0x05,i);
	}
    }
    if (stuff->which&XkbKTLevelNamesMask) {
	register unsigned i;
	XkbKeyTypePtr	type;
	CARD8 *		width;
	if ( stuff->nKTLevels<1 ) {
	    client->errorValue = _XkbErrCode2(0x05,stuff->nKTLevels);
	    return BadValue;
	}
	if ((unsigned)(stuff->firstKTLevel+stuff->nKTLevels-1)>=
							xkb->map->num_types) {
	    client->errorValue = _XkbErrCode4(0x06,stuff->firstKTLevel,
				stuff->nKTLevels,xkb->map->num_types);
	    return BadValue;
	}
	width = (CARD8 *)tmp;
	tmp= (CARD32 *)(((char *)tmp)+XkbPaddedSize(stuff->nKTLevels));
	type = &xkb->map->types[stuff->firstKTLevel];
	for (i=0;i<stuff->nKTLevels;i++,type++) {
	    if (width[i]==0)
		continue;
	    else if (width[i]!=type->num_levels) {
		client->errorValue= _XkbErrCode4(0x07,i+stuff->firstKTLevel,
						type->num_levels,width[i]);
		return BadMatch;
	    }
	    tmp= _XkbCheckAtoms(tmp,width[i],client->swapped,&bad);
	    if (!tmp) {
		client->errorValue= bad;
		return BadAtom;
	    }
	}
    }
    if (stuff->which&XkbIndicatorNamesMask) {
	if (stuff->indicators==0) {
	    client->errorValue= 0x08;
	    return BadMatch;
	}
	tmp= _XkbCheckMaskedAtoms(tmp,XkbNumIndicators,stuff->indicators,
							client->swapped,&bad);
	if (!tmp) {
	    client->errorValue= bad;
	    return BadAtom;
	}
    }
    if (stuff->which&XkbVirtualModNamesMask) {
	if (stuff->virtualMods==0) {
	    client->errorValue= 0x09;
	    return BadMatch;
	}
	tmp= _XkbCheckMaskedAtoms(tmp,XkbNumVirtualMods,
						(CARD32)stuff->virtualMods,
						client->swapped,&bad);
	if (!tmp) {
	    client->errorValue = bad;
	    return BadAtom;
	}
    }
    if (stuff->which&XkbGroupNamesMask) {
	if (stuff->groupNames==0) {
	    client->errorValue= 0x0a;
	    return BadMatch;
	}
	tmp= _XkbCheckMaskedAtoms(tmp,XkbNumKbdGroups,
						(CARD32)stuff->groupNames,
						client->swapped,&bad);
	if (!tmp) {
	    client->errorValue = bad;
	    return BadAtom;
	}
    }
    if (stuff->which&XkbKeyNamesMask) {
	if (stuff->firstKey<(unsigned)xkb->min_key_code) {
	    client->errorValue= _XkbErrCode3(0x0b,xkb->min_key_code,
							stuff->firstKey);
	    return BadValue;
	}
	if (((unsigned)(stuff->firstKey+stuff->nKeys-1)>xkb->max_key_code)||
							(stuff->nKeys<1)) {
	    client->errorValue= _XkbErrCode4(0x0c,xkb->max_key_code,
						stuff->firstKey,stuff->nKeys);
	    return BadValue;
	}
	tmp+= stuff->nKeys;
    }
    if ((stuff->which&XkbKeyAliasesMask)&&(stuff->nKeyAliases>0)) {
	tmp+= stuff->nKeyAliases*2;
    }
    if (stuff->which&XkbRGNamesMask) {
	if ( stuff->nRadioGroups<1 ) {
	    client->errorValue= _XkbErrCode2(0x0d,stuff->nRadioGroups);
	    return BadValue;
	}
	tmp= _XkbCheckAtoms(tmp,stuff->nRadioGroups,client->swapped,&bad);
	if (!tmp) {
	    client->errorValue= bad;
	    return BadAtom;
	}
    }
    if ((tmp-((CARD32 *)stuff))!=stuff->length) {
	client->errorValue = stuff->length;
	return BadLength;
    }
    if (XkbAllocNames(xkb,stuff->which,stuff->nRadioGroups,
					stuff->nKeyAliases)!=Success) {
	return BadAlloc;
    }

    /* everything is okay -- update names */
    bzero(&nn,sizeof(xkbNamesNotify));
    nn.changed= stuff->which;
    tmp = (CARD32 *)&stuff[1];
    if (stuff->which&XkbKeycodesNameMask)
	names->keycodes= *tmp++;
    if (stuff->which&XkbGeometryNameMask)
	names->geometry= *tmp++;
    if (stuff->which&XkbSymbolsNameMask)
	names->symbols= *tmp++;
    if (stuff->which&XkbPhysSymbolsNameMask)
	names->phys_symbols= *tmp++;
    if (stuff->which&XkbTypesNameMask)
	names->types= *tmp++;
    if (stuff->which&XkbCompatNameMask)
	names->compat= *tmp++;
    if ((stuff->which&XkbKeyTypeNamesMask)&&(stuff->nTypes>0)) {
	register unsigned i;
	register XkbKeyTypePtr type;

	type= &xkb->map->types[stuff->firstType];
	for (i=0;i<stuff->nTypes;i++,type++) {
	    type->name= *tmp++;
	}
	nn.firstType= stuff->firstType;
	nn.nTypes= stuff->nTypes;
    }
    if (stuff->which&XkbKTLevelNamesMask) {
	register XkbKeyTypePtr	type;
	register unsigned i;
	CARD8 *width;

	width = (CARD8 *)tmp;
	tmp= (CARD32 *)(((char *)tmp)+XkbPaddedSize(stuff->nKTLevels));
	type= &xkb->map->types[stuff->firstKTLevel];
	for (i=0;i<stuff->nKTLevels;i++,type++) {
	    if (width[i]>0) {
		if (type->level_names) {
		    register unsigned n;
		    for (n=0;n<width[i];n++) {
			type->level_names[n]= tmp[n];
		    }
		}
		tmp+= width[i];
	    }
	}
	nn.firstLevelName= 0;
	nn.nLevelNames= stuff->nTypes;
    }
    if (stuff->which&XkbIndicatorNamesMask) {
	tmp= _XkbCopyMaskedAtoms(tmp,names->indicators,XkbNumIndicators,
							stuff->indicators);
	nn.changedIndicators= stuff->indicators;
    }
    if (stuff->which&XkbVirtualModNamesMask) {
	tmp= _XkbCopyMaskedAtoms(tmp,names->vmods,XkbNumVirtualMods,
							stuff->virtualMods);
	nn.changedVirtualMods= stuff->virtualMods;
    }
    if (stuff->which&XkbGroupNamesMask) {
	tmp= _XkbCopyMaskedAtoms(tmp,names->groups,XkbNumKbdGroups,
							stuff->groupNames);
	nn.changedVirtualMods= stuff->groupNames;
    }
    if (stuff->which&XkbKeyNamesMask) {
	memcpy((char*)&names->keys[stuff->firstKey],(char *)tmp,
						stuff->nKeys*XkbKeyNameLength);
	tmp+= stuff->nKeys;
	nn.firstKey= stuff->firstKey;
	nn.nKeys= stuff->nKeys;
    }
    if (stuff->which&XkbKeyAliasesMask) {
	if (stuff->nKeyAliases>0) {
	    register int na= stuff->nKeyAliases;	
	    if (XkbAllocNames(xkb,XkbKeyAliasesMask,0,na)!=Success)
		return BadAlloc;
	    memcpy((char *)names->key_aliases,(char *)tmp,
				stuff->nKeyAliases*sizeof(XkbKeyAliasRec));
	    tmp+= stuff->nKeyAliases*2;
	}
	else if (names->key_aliases!=NULL) {
	    _XkbFree(names->key_aliases);
	    names->key_aliases= NULL;
	    names->num_key_aliases= 0;
	}
	nn.nAliases= names->num_key_aliases;
    }
    if (stuff->which&XkbRGNamesMask) {
	if (stuff->nRadioGroups>0) {
	    register unsigned i,nrg;
	    nrg= stuff->nRadioGroups;
	    if (XkbAllocNames(xkb,XkbRGNamesMask,nrg,0)!=Success)
		return BadAlloc;

	    for (i=0;i<stuff->nRadioGroups;i++) {
		names->radio_groups[i]= tmp[i];
	    }
	    tmp+= stuff->nRadioGroups;
	}
	else if (names->radio_groups) {
	    _XkbFree(names->radio_groups);
	    names->radio_groups= NULL;
	    names->num_rg= 0;
	}
	nn.nRadioGroups= names->num_rg;
    }
    if (nn.changed) {
	Bool needExtEvent;
	needExtEvent= (nn.changed&XkbIndicatorNamesMask)!=0;
	XkbSendNamesNotify(dev,&nn);
	if (needExtEvent) {
	    XkbSrvLedInfoPtr		sli;
	    xkbExtensionDeviceNotify	edev;
	    register int		i;
	    register unsigned		bit;

	    sli= XkbFindSrvLedInfo(dev,XkbDfltXIClass,XkbDfltXIId,
							XkbXI_IndicatorsMask);
	    sli->namesPresent= 0;
	    for (i=0,bit=1;i<XkbNumIndicators;i++,bit<<=1) {
		if (names->indicators[i]!=None)
		    sli->namesPresent|= bit;
	    }
	    bzero(&edev,sizeof(xkbExtensionDeviceNotify));
	    edev.reason=	XkbXI_IndicatorNamesMask;
	    edev.ledClass=	KbdFeedbackClass;
	    edev.ledID=		dev->kbdfeed->ctrl.id;
	    edev.ledsDefined= 	sli->namesPresent|sli->mapsPresent;
	    edev.ledState=	sli->effectiveState;
	    edev.firstBtn=	0;
	    edev.nBtns=		0;
	    edev.supported=	XkbXI_AllFeaturesMask;
	    edev.unsupported=	0;
	    XkbSendExtensionDeviceNotify(dev,client,&edev);
	}
    }
    return client->noClientException;
}

/***====================================================================***/

#include "XKBgeom.h"

#define	XkbSizeCountedString(s)  ((s)?((((2+strlen(s))+3)/4)*4):4)

static char *
#if NeedFunctionPrototypes
XkbWriteCountedString(char *wire,char *str,Bool swap)
#else
XkbWriteCountedString(wire,str,swap)
    char *	wire;
    char *	str;
    Bool	swap;
#endif
{
CARD16	len,*pLen;

    len= (str?strlen(str):0);
    pLen= (CARD16 *)wire;
    *pLen= len;
    if (swap) {
	register int n;
	swaps(pLen,n);
    }
    memcpy(&wire[2],str,len);
    wire+= ((2+len+3)/4)*4;
    return wire;
}

static int
#if NeedFunctionPrototypes
XkbSizeGeomProperties(XkbGeometryPtr geom)
#else
XkbSizeGeomProperties(geom)
    XkbGeometryPtr	geom;
#endif
{
register int 	i,size;
XkbPropertyPtr	prop;
    
    for (size=i=0,prop=geom->properties;i<geom->num_properties;i++,prop++) {
	size+= XkbSizeCountedString(prop->name);
	size+= XkbSizeCountedString(prop->value);
    }
    return size;
}

static char *
#if NeedFunctionPrototypes
XkbWriteGeomProperties(char *wire,XkbGeometryPtr geom,Bool swap)
#else
XkbWriteGeomProperties(wire,geom,swap)
    char *		wire;
    XkbGeometryPtr	geom;
    Bool		swap;
#endif
{
register int 	i;
register XkbPropertyPtr	prop;
    
    for (i=0,prop=geom->properties;i<geom->num_properties;i++,prop++) {
	wire= XkbWriteCountedString(wire,prop->name,swap);
	wire= XkbWriteCountedString(wire,prop->value,swap);
    }
    return wire;
}

static int
#if NeedFunctionPrototypes
XkbSizeGeomKeyAliases(XkbGeometryPtr geom)
#else
XkbSizeGeomKeyAliases(geom)
    XkbGeometryPtr	geom;
#endif
{
    return geom->num_key_aliases*(2*XkbKeyNameLength);
}

static char *
#if NeedFunctionPrototypes
XkbWriteGeomKeyAliases(char *wire,XkbGeometryPtr geom,Bool swap)
#else
XkbWriteGeomKeyAliases(wire,geom,swap)
    char *		wire;
    XkbGeometryPtr	geom;
    Bool		swap;
#endif
{
register int sz;
    
    sz= geom->num_key_aliases*(XkbKeyNameLength*2);
    if (sz>0) {
	memcpy(wire,(char *)geom->key_aliases,sz);
	wire+= sz;
    }
    return wire;
}

static int
#if NeedFunctionPrototypes
XkbSizeGeomColors(XkbGeometryPtr geom)
#else
XkbSizeGeomColors(geom)
    XkbGeometryPtr	geom;
#endif
{
register int 		i,size;
register XkbColorPtr	color;

    for (i=size=0,color=geom->colors;i<geom->num_colors;i++,color++) {
	size+= XkbSizeCountedString(color->spec);
    }
    return size;
}

static char *
#if NeedFunctionPrototypes
XkbWriteGeomColors(char *wire,XkbGeometryPtr geom,Bool swap)
#else
XkbWriteGeomColors(wire,geom,swap)
    char *		wire;
    XkbGeometryPtr	geom;
    Bool		swap;
#endif
{
register int		i;
register XkbColorPtr	color;

    for (i=0,color=geom->colors;i<geom->num_colors;i++,color++) {
	wire= XkbWriteCountedString(wire,color->spec,swap);
    }
    return wire;
}

static int
#if NeedFunctionPrototypes
XkbSizeGeomShapes(XkbGeometryPtr geom)
#else
XkbSizeGeomShapes(geom)
    XkbGeometryPtr	geom;
#endif
{
register int		i,size;
register XkbShapePtr	shape;

    for (i=size=0,shape=geom->shapes;i<geom->num_shapes;i++,shape++) {
	register int		n;
	register XkbOutlinePtr	ol;
	size+= SIZEOF(xkbShapeWireDesc);
	for (n=0,ol=shape->outlines;n<shape->num_outlines;n++,ol++) {
	    size+= SIZEOF(xkbOutlineWireDesc);
	    size+= ol->num_points*SIZEOF(xkbPointWireDesc);
	}
    }
    return size;
}

static char *
#if NeedFunctionPrototypes
XkbWriteGeomShapes(char *wire,XkbGeometryPtr geom,Bool swap)
#else
XkbWriteGeomShapes(wire,geom,swap)
    char *		wire;
    XkbGeometryPtr	geom;
    Bool		swap;
#endif
{
int			i;
XkbShapePtr		shape;
xkbShapeWireDesc *	shapeWire;

    for (i=0,shape=geom->shapes;i<geom->num_shapes;i++,shape++) {
	register int 		o;
	XkbOutlinePtr		ol;
	xkbOutlineWireDesc *	olWire;
	shapeWire= (xkbShapeWireDesc *)wire;
	shapeWire->name= shape->name;
	shapeWire->nOutlines= shape->num_outlines;
	if (shape->primary!=NULL)
	     shapeWire->primaryNdx= XkbOutlineIndex(shape,shape->primary);
	else shapeWire->primaryNdx= XkbNoShape;
	if (shape->approx!=NULL)
	     shapeWire->approxNdx= XkbOutlineIndex(shape,shape->approx);
	else shapeWire->approxNdx= XkbNoShape;
	if (swap) {
	    register int n;
	    swapl(&shapeWire->name,n);
	}
	wire= (char *)&shapeWire[1];
	for (o=0,ol=shape->outlines;o<shape->num_outlines;o++,ol++) {
	    register int	p;
	    XkbPointPtr		pt;
	    xkbPointWireDesc *	ptWire;
	    olWire= (xkbOutlineWireDesc *)wire;
	    olWire->nPoints= ol->num_points;
	    olWire->cornerRadius= ol->corner_radius;
	    wire= (char *)&olWire[1];
	    ptWire= (xkbPointWireDesc *)wire;
	    for (p=0,pt=ol->points;p<ol->num_points;p++,pt++) {
		ptWire[p].x= pt->x;
		ptWire[p].y= pt->y;
		if (swap) {
		    register int n;
		    swaps(&ptWire[p].x,n);
		    swaps(&ptWire[p].y,n);
		}
	    }
	    wire= (char *)&ptWire[ol->num_points];
	}
    }
    return wire;
}

static int
#if NeedFunctionPrototypes
XkbSizeGeomDoodads(int num_doodads,XkbDoodadPtr doodad)
#else
XkbSizeGeomDoodads(num_doodads,doodad)
    int			num_doodads;
    XkbDoodadPtr	doodad;
#endif
{
register int	i,size;

    for (i=size=0;i<num_doodads;i++,doodad++) {
	size+= SIZEOF(xkbAnyDoodadWireDesc);
	if (doodad->any.type==XkbTextDoodad) {
	    size+= XkbSizeCountedString(doodad->text.text);
	    size+= XkbSizeCountedString(doodad->text.font);
	}
	else if (doodad->any.type==XkbLogoDoodad) {
	    size+= XkbSizeCountedString(doodad->logo.logo_name);
	}
    }
    return size;
}

static char *
#if NeedFunctionPrototypes
XkbWriteGeomDoodads(char *wire,int num_doodads,XkbDoodadPtr doodad,Bool swap)
#else
XkbWriteGeomDoodads(wire,num_doodads,doodad,swap)
    char *		wire;
    int			num_doodads;
    XkbDoodadPtr	doodad;
    Bool		swap;
#endif
{
register int		i;
xkbDoodadWireDesc *	doodadWire;

    for (i=0;i<num_doodads;i++,doodad++) {
	doodadWire= (xkbDoodadWireDesc *)wire;
	wire= (char *)&doodadWire[1];
	bzero(doodadWire,SIZEOF(xkbDoodadWireDesc));
	doodadWire->any.name= doodad->any.name;
	doodadWire->any.type= doodad->any.type;
	doodadWire->any.priority= doodad->any.priority;
	doodadWire->any.top= doodad->any.top;
	doodadWire->any.left= doodad->any.left;
	if (swap) {
	    register int n;
	    swapl(&doodadWire->any.name,n);
	    swaps(&doodadWire->any.top,n);
	    swaps(&doodadWire->any.left,n);
	}
	switch (doodad->any.type) {
	    case XkbOutlineDoodad:
	    case XkbSolidDoodad:
		doodadWire->shape.angle= doodad->shape.angle;
		doodadWire->shape.colorNdx= doodad->shape.color_ndx;
		doodadWire->shape.shapeNdx= doodad->shape.shape_ndx;
		if (swap) {
		    register int n;
		    swaps(&doodadWire->shape.angle,n);
		}
		break;
	    case XkbTextDoodad:
		doodadWire->text.angle= doodad->text.angle;
		doodadWire->text.width= doodad->text.width;
		doodadWire->text.height= doodad->text.height;
		doodadWire->text.colorNdx= doodad->text.color_ndx;
		if (swap) {
		    register int n;
		    swaps(&doodadWire->text.angle,n);
		    swaps(&doodadWire->text.width,n);
		    swaps(&doodadWire->text.height,n);
		}
		wire= XkbWriteCountedString(wire,doodad->text.text,swap);
		wire= XkbWriteCountedString(wire,doodad->text.font,swap);
		break;
	    case XkbIndicatorDoodad:
		doodadWire->indicator.shapeNdx= doodad->indicator.shape_ndx;
		doodadWire->indicator.onColorNdx=doodad->indicator.on_color_ndx;
		doodadWire->indicator.offColorNdx=
						doodad->indicator.off_color_ndx;
		break;
	    case XkbLogoDoodad:
		doodadWire->logo.angle= doodad->logo.angle;
		doodadWire->logo.colorNdx= doodad->logo.color_ndx;
		doodadWire->logo.shapeNdx= doodad->logo.shape_ndx;
		wire= XkbWriteCountedString(wire,doodad->logo.logo_name,swap);
		break;
	    default:
		ErrorF("Unknown doodad type %d in XkbWriteGeomDoodads\n");
		ErrorF("Ignored\n");
		break;
	}
    }
    return wire;
}

static char *
#if NeedFunctionPrototypes
XkbWriteGeomOverlay(char *wire,XkbOverlayPtr ol,Bool swap)
#else
XkbWriteGeomOverlay(wire,ol,swap)
    char *		wire;
    XkbOverlayPtr	ol;
    Bool		swap;
#endif
{
register int		r;
XkbOverlayRowPtr	row;
xkbOverlayWireDesc *	olWire;

   olWire= (xkbOverlayWireDesc *)wire;
   olWire->name= ol->name;
   olWire->nRows= ol->num_rows;
   if (swap) {
	register int n;
	swapl(&olWire->name,n);
   }
   wire= (char *)&olWire[1];
   for (r=0,row=ol->rows;r<ol->num_rows;r++,row++) {
   	unsigned int		k;
	XkbOverlayKeyPtr	key;
	xkbOverlayRowWireDesc *	rowWire;
	rowWire= (xkbOverlayRowWireDesc *)wire;
	rowWire->rowUnder= row->row_under;
	rowWire->nKeys= row->num_keys;
	wire= (char *)&rowWire[1];
	for (k=0,key=row->keys;k<row->num_keys;k++,key++) {
	    xkbOverlayKeyWireDesc *	keyWire;
	    keyWire= (xkbOverlayKeyWireDesc *)wire;
	    memcpy(keyWire->over,key->over.name,XkbKeyNameLength);
	    memcpy(keyWire->under,key->under.name,XkbKeyNameLength);
	    wire= (char *)&keyWire[1];
	}
   }
   return wire;
}

static int
#if NeedFunctionPrototypes
XkbSizeGeomSections(XkbGeometryPtr geom)
#else
XkbSizeGeomSections(geom)
    XkbGeometryPtr	geom;
#endif
{
register int 	i,size;
XkbSectionPtr	section;

    for (i=size=0,section=geom->sections;i<geom->num_sections;i++,section++) {
	size+= SIZEOF(xkbSectionWireDesc);
	if (section->rows) {
	    int		r;
	    XkbRowPtr	row;
	    for (r=0,row=section->rows;r<section->num_rows;row++,r++) {
		size+= SIZEOF(xkbRowWireDesc);
		size+= row->num_keys*SIZEOF(xkbKeyWireDesc);
	    }
	}
	if (section->doodads)
	    size+= XkbSizeGeomDoodads(section->num_doodads,section->doodads);
	if (section->overlays) {
	    int			o;
	    XkbOverlayPtr	ol;
	    for (o=0,ol=section->overlays;o<section->num_overlays;o++,ol++) {
		int			r;
		XkbOverlayRowPtr	row;
		size+= SIZEOF(xkbOverlayWireDesc);
		for (r=0,row=ol->rows;r<ol->num_rows;r++,row++) {
		   size+= SIZEOF(xkbOverlayRowWireDesc);
		   size+= row->num_keys*SIZEOF(xkbOverlayKeyWireDesc);
		}
	    }
	}
    }
    return size;
}

static char *
#if NeedFunctionPrototypes
XkbWriteGeomSections(char *wire,XkbGeometryPtr geom,Bool swap)
#else
XkbWriteGeomSections(wire,geom,swap)
    char *		wire;
    XkbGeometryPtr	geom;
    Bool		swap;
#endif
{
register int		i;
XkbSectionPtr		section;
xkbSectionWireDesc *	sectionWire;

    for (i=0,section=geom->sections;i<geom->num_sections;i++,section++) {
	sectionWire= (xkbSectionWireDesc *)wire;
	sectionWire->name= section->name;
	sectionWire->top= section->top;
	sectionWire->left= section->left;
	sectionWire->width= section->width;
	sectionWire->height= section->height;
	sectionWire->angle= section->angle;
	sectionWire->priority= section->priority;
	sectionWire->nRows= section->num_rows;
	sectionWire->nDoodads= section->num_doodads;
	sectionWire->nOverlays= section->num_overlays;
	sectionWire->pad= 0;
	if (swap) {
	    register int n;
	    swapl(&sectionWire->name,n);
	    swaps(&sectionWire->top,n);
	    swaps(&sectionWire->left,n);
	    swaps(&sectionWire->width,n);
	    swaps(&sectionWire->height,n);
	    swaps(&sectionWire->angle,n);
	}
	wire= (char *)&sectionWire[1];
	if (section->rows) {
	    int			r;
	    XkbRowPtr		row;
	    xkbRowWireDesc *	rowWire;
	    for (r=0,row=section->rows;r<section->num_rows;r++,row++) {
		rowWire= (xkbRowWireDesc *)wire;
		rowWire->top= row->top;
		rowWire->left= row->left;
		rowWire->nKeys= row->num_keys;
		rowWire->vertical= row->vertical;
		rowWire->pad= 0;
		if (swap) {
		    register int n;
		    swaps(&rowWire->top,n);
		    swaps(&rowWire->left,n);
		}
		wire= (char *)&rowWire[1];
		if (row->keys) {
		    int			k;
		    XkbKeyPtr		key;
		    xkbKeyWireDesc *	keyWire;
		    keyWire= (xkbKeyWireDesc *)wire;
		    for (k=0,key=row->keys;k<row->num_keys;k++,key++) {
			memcpy(keyWire[k].name,key->name.name,XkbKeyNameLength);
			keyWire[k].gap= key->gap;
			keyWire[k].shapeNdx= key->shape_ndx;
			keyWire[k].colorNdx= key->color_ndx;
			if (swap) {
			    register int n;
			    swaps(&keyWire[k].gap,n);
			}
		    }
		    wire= (char *)&keyWire[row->num_keys];
		}
	    }
	}
	if (section->doodads) {
	    wire= XkbWriteGeomDoodads(wire,
	    			      section->num_doodads,section->doodads,
				      swap);
	}
	if (section->overlays) {
	    register int o;
	    for (o=0;o<section->num_overlays;o++) {
		wire= XkbWriteGeomOverlay(wire,&section->overlays[o],swap);
	    }
	}
    }
    return wire;
}

static Status
#if NeedFunctionPrototypes
XkbComputeGetGeometryReplySize(	XkbGeometryPtr		geom,
				xkbGetGeometryReply *	rep,
				Atom			name)
#else
XkbComputeGetGeometryReplySize(geom,rep,name)
    XkbGeometryPtr		geom;
    xkbGetGeometryReply *	rep;
    Atom			name;
#endif
{
int	len;

    if (geom!=NULL) {
	len= XkbSizeCountedString(geom->label_font);
	len+= XkbSizeGeomProperties(geom);
	len+= XkbSizeGeomColors(geom);
	len+= XkbSizeGeomShapes(geom);
	len+= XkbSizeGeomSections(geom);
	len+= XkbSizeGeomDoodads(geom->num_doodads,geom->doodads);
	len+= XkbSizeGeomKeyAliases(geom);
	rep->length= len/4;
	rep->found= True;
	rep->name= geom->name;
	rep->widthMM= geom->width_mm;
	rep->heightMM= geom->height_mm;
	rep->nProperties= geom->num_properties;
	rep->nColors= geom->num_colors;
	rep->nShapes= geom->num_shapes;
	rep->nSections= geom->num_sections;
	rep->nDoodads= geom->num_doodads;
	rep->nKeyAliases= geom->num_key_aliases;
	rep->baseColorNdx= XkbGeomColorIndex(geom,geom->base_color);
	rep->labelColorNdx= XkbGeomColorIndex(geom,geom->label_color);
    }
    else {
	rep->length= 0;
	rep->found= False;
	rep->name= name;
	rep->widthMM= rep->heightMM= 0;
	rep->nProperties= rep->nColors= rep->nShapes= 0;
	rep->nSections= rep->nDoodads= 0;
	rep->nKeyAliases= 0;
	rep->labelColorNdx= rep->baseColorNdx= 0;
    }
    return Success;
}

static int
#if NeedFunctionPrototypes
XkbSendGeometry(	ClientPtr		client,
			XkbGeometryPtr		geom,
			xkbGetGeometryReply *	rep,
			Bool			freeGeom)
#else
XkbSendGeometry(client,geom,rep,freeGeom)
    ClientPtr		client;
    XkbGeometryPtr	geom;
    xkbGetGeometryReply	*rep;
    Bool		freeGeom;
#endif
{
    char	*desc,*start;
    int		 len;

    if (geom!=NULL) {
	len= rep->length*4;
	start= desc= (char *)ALLOCATE_LOCAL(len);
	if (!start)
	    return BadAlloc;
	desc=  XkbWriteCountedString(desc,geom->label_font,client->swapped);
	if ( rep->nProperties>0 )
	    desc = XkbWriteGeomProperties(desc,geom,client->swapped);
	if ( rep->nColors>0 )
	    desc = XkbWriteGeomColors(desc,geom,client->swapped);
	if ( rep->nShapes>0 )
	    desc = XkbWriteGeomShapes(desc,geom,client->swapped);
	if ( rep->nSections>0 )
	    desc = XkbWriteGeomSections(desc,geom,client->swapped);
	if ( rep->nDoodads>0 )
	    desc = XkbWriteGeomDoodads(desc,geom->num_doodads,geom->doodads,
							  client->swapped);
	if ( rep->nKeyAliases>0 )
	    desc = XkbWriteGeomKeyAliases(desc,geom,client->swapped);
	if ((desc-start)!=(len)) {
	    ErrorF("BOGUS LENGTH in XkbSendGeometry, expected %d, got %d\n",
							len, desc-start);
	}
    }
    else {
	len= 0;
	start= NULL;
    }
    if (client->swapped) {
	register int n;
	swaps(&rep->sequenceNumber,n);
	swapl(&rep->length,n);
	swapl(&rep->name,n);
	swaps(&rep->widthMM,n);
	swaps(&rep->heightMM,n);
	swaps(&rep->nProperties,n);
	swaps(&rep->nColors,n);
	swaps(&rep->nShapes,n);
	swaps(&rep->nSections,n);
	swaps(&rep->nDoodads,n);
	swaps(&rep->nKeyAliases,n);
    }
    WriteToClient(client, SIZEOF(xkbGetGeometryReply), (char *)rep);
    if (len>0)
	WriteToClient(client, len, start);
    if (start!=NULL)
	DEALLOCATE_LOCAL((char *)start);
    if (freeGeom)
	XkbFreeGeometry(geom,XkbGeomAllMask,True);
    return client->noClientException;
}

int
#if NeedFunctionPrototypes
ProcXkbGetGeometry(ClientPtr client)
#else
ProcXkbGetGeometry(client)
    ClientPtr client;
#endif
{
    DeviceIntPtr 	dev;
    xkbGetGeometryReply rep;
    XkbGeometryPtr	geom;
    Bool		shouldFree;
    Status		status;

    REQUEST(xkbGetGeometryReq);
    REQUEST_SIZE_MATCH(xkbGetGeometryReq);

    if (!(client->xkbClientFlags&_XkbClientInitialized))
	return BadAccess;

    CHK_KBD_DEVICE(dev,stuff->deviceSpec);
    CHK_ATOM_OR_NONE(stuff->name);

    geom= XkbLookupNamedGeometry(dev,stuff->name,&shouldFree);
    rep.type= X_Reply;
    rep.deviceID= dev->id;
    rep.sequenceNumber= client->sequence;
    rep.length= 0;
    status= XkbComputeGetGeometryReplySize(geom,&rep,stuff->name);
    if (status!=Success)
	 return status;
    else return XkbSendGeometry(client,geom,&rep,shouldFree);
}

/***====================================================================***/

static char *
#if NeedFunctionPrototypes
_GetCountedString(char **wire_inout,Bool swap)
#else
_GetCountedString(wire_inout,swap)
    char **	wire_inout;
    Bool	swap;
#endif
{
char *	wire,*str;
CARD16	len,*plen;

    wire= *wire_inout;
    plen= (CARD16 *)wire;
    if (swap) {
	register int n;
	swaps(plen,n);
    }
    len= *plen;
    str= (char *)_XkbAlloc(len+1);
    if (str) {
	memcpy(str,&wire[2],len);
	str[len]= '\0';
    }
    wire+= XkbPaddedSize(len+2);
    *wire_inout= wire;
    return str;
}

static Status
#if NeedFunctionPrototypes
_CheckSetDoodad(	char **		wire_inout,
			XkbGeometryPtr	geom,
			XkbSectionPtr	section,
			ClientPtr	client)
#else
_CheckSetDoodad(wire_inout,geom,section,client)
    char **		wire_inout;
    XkbGeometryPtr	geom;
    XkbSectionPtr	section;
    ClientPtr		client;
#endif
{
char *			wire;
xkbDoodadWireDesc *	dWire;
XkbDoodadPtr		doodad;

    dWire= (xkbDoodadWireDesc *)(*wire_inout);
    wire= (char *)&dWire[1];
    if (client->swapped) {
	register int n;
	swapl(&dWire->any.name,n);
	swaps(&dWire->any.top,n);
	swaps(&dWire->any.left,n);
	swaps(&dWire->any.angle,n);
    }
    CHK_ATOM_ONLY(dWire->any.name);
    doodad= XkbAddGeomDoodad(geom,section,dWire->any.name);
    if (!doodad)
	return BadAlloc;
    doodad->any.type= dWire->any.type;
    doodad->any.priority= dWire->any.priority;
    doodad->any.top= dWire->any.top;
    doodad->any.left= dWire->any.left;
    doodad->any.angle= dWire->any.angle;
    switch (doodad->any.type) {
	case XkbOutlineDoodad:
	case XkbSolidDoodad:
	    if (dWire->shape.colorNdx>=geom->num_colors) {
		client->errorValue= _XkbErrCode3(0x40,geom->num_colors,
							dWire->shape.colorNdx);
		return BadMatch;
	    }
	    if (dWire->shape.shapeNdx>=geom->num_shapes) {
		client->errorValue= _XkbErrCode3(0x41,geom->num_shapes,
							dWire->shape.shapeNdx);
		return BadMatch;
	    }
	    doodad->shape.color_ndx= dWire->shape.colorNdx;
	    doodad->shape.shape_ndx= dWire->shape.shapeNdx;
	    break;
	case XkbTextDoodad:
	    if (dWire->text.colorNdx>=geom->num_colors) {
		client->errorValue= _XkbErrCode3(0x42,geom->num_colors,
							dWire->text.colorNdx);
		return BadMatch;
	    }
	    if (client->swapped) {
		register int n;
		swaps(&dWire->text.width,n);
		swaps(&dWire->text.height,n);
	    }
	    doodad->text.width= dWire->text.width;
	    doodad->text.height= dWire->text.height;
	    doodad->text.color_ndx= dWire->text.colorNdx;
	    doodad->text.text= _GetCountedString(&wire,client->swapped);
	    doodad->text.font= _GetCountedString(&wire,client->swapped);
	    break;
	case XkbIndicatorDoodad:
	    if (dWire->indicator.onColorNdx>=geom->num_colors) {
		client->errorValue= _XkbErrCode3(0x43,geom->num_colors,
						dWire->indicator.onColorNdx);
		return BadMatch;
	    }
	    if (dWire->indicator.offColorNdx>=geom->num_colors) {
		client->errorValue= _XkbErrCode3(0x44,geom->num_colors,
						dWire->indicator.offColorNdx);
		return BadMatch;
	    }
	    if (dWire->indicator.shapeNdx>=geom->num_shapes) {
		client->errorValue= _XkbErrCode3(0x45,geom->num_shapes,
						dWire->indicator.shapeNdx);
		return BadMatch;
	    }
	    doodad->indicator.shape_ndx= dWire->indicator.shapeNdx;
	    doodad->indicator.on_color_ndx= dWire->indicator.onColorNdx;
	    doodad->indicator.off_color_ndx= dWire->indicator.offColorNdx;
	    break;
	case XkbLogoDoodad:
	    if (dWire->logo.colorNdx>=geom->num_colors) {
		client->errorValue= _XkbErrCode3(0x46,geom->num_colors,
							dWire->logo.colorNdx);
		return BadMatch;
	    }
	    if (dWire->logo.shapeNdx>=geom->num_shapes) {
		client->errorValue= _XkbErrCode3(0x47,geom->num_shapes,
							dWire->logo.shapeNdx);
		return BadMatch;
	    }
	    doodad->logo.color_ndx= dWire->logo.colorNdx;
	    doodad->logo.shape_ndx= dWire->logo.shapeNdx;
	    doodad->logo.logo_name= _GetCountedString(&wire,client->swapped);
	    break;
	default:
	    client->errorValue= _XkbErrCode2(0x4F,dWire->any.type);
	    return BadValue;
    }
    *wire_inout= wire;
    return Success;
}

static Status
#if NeedFunctionPrototypes
_CheckSetOverlay(	char **		wire_inout,
			XkbGeometryPtr	geom,
			XkbSectionPtr	section,
			ClientPtr	client)
#else
_CheckSetOverlay(wire_inout,geom,section,client)
    char **		wire_inout;
    XkbGeometryPtr	geom;
    XkbSectionPtr	section;
    ClientPtr		client;
#endif
{
register int		r;
char *			wire;
XkbOverlayPtr		ol;
xkbOverlayWireDesc *	olWire;
xkbOverlayRowWireDesc *	rWire;

    wire= *wire_inout;
    olWire= (xkbOverlayWireDesc *)wire;
    if (client->swapped) {
	register int n;
	swapl(&olWire->name,n);
    }
    CHK_ATOM_ONLY(olWire->name);
    ol= XkbAddGeomOverlay(section,olWire->name,olWire->nRows);
    rWire= (xkbOverlayRowWireDesc *)&olWire[1];
    for (r=0;r<olWire->nRows;r++) {
	register int		k;
	xkbOverlayKeyWireDesc *	kWire;
	XkbOverlayRowPtr	row;

	if (rWire->rowUnder>section->num_rows) {
	    client->errorValue= _XkbErrCode4(0x20,r,section->num_rows,
							rWire->rowUnder);
	    return BadMatch;
	}
	row= XkbAddGeomOverlayRow(ol,rWire->rowUnder,rWire->nKeys);
	kWire= (xkbOverlayKeyWireDesc *)&rWire[1];
	for (k=0;k<rWire->nKeys;k++,kWire++) {
	    if (XkbAddGeomOverlayKey(ol,row,
	    		(char *)kWire->over,(char *)kWire->under)==NULL) {
		client->errorValue= _XkbErrCode3(0x21,r,k);
		return BadMatch;
	    }	
	}
	rWire= (xkbOverlayRowWireDesc *)kWire;
    }
    olWire= (xkbOverlayWireDesc *)rWire;
    wire= (char *)olWire;
    *wire_inout= wire;
    return Success;
}

static Status
#if NeedFunctionPrototypes
_CheckSetSections( 	XkbGeometryPtr		geom,
			xkbSetGeometryReq *	req,
			char **			wire_inout,
			ClientPtr		client)
#else
_CheckSetSections(geom,req,wire_inout,client)
    XkbGeometryPtr	geom;
    xkbSetGeometryReq *	req;
    char **		wire_inout;
    ClientPtr		client;
#endif
{
Status			status;
register int		s;
char *			wire;
xkbSectionWireDesc *	sWire;
XkbSectionPtr		section;

    wire= *wire_inout;
    if (req->nSections<1)
	return Success;
    sWire= (xkbSectionWireDesc *)wire;
    for (s=0;s<req->nSections;s++) {
	register int		r;
	xkbRowWireDesc *	rWire;
	if (client->swapped) {
	    register int n;
	    swapl(&sWire->name,n);
	    swaps(&sWire->top,n);
	    swaps(&sWire->left,n);
	    swaps(&sWire->width,n);
	    swaps(&sWire->height,n);
	    swaps(&sWire->angle,n);
	}
	CHK_ATOM_ONLY(sWire->name);
	section= XkbAddGeomSection(geom,sWire->name,sWire->nRows,
					sWire->nDoodads,sWire->nOverlays);
	if (!section)
	    return BadAlloc;
	section->priority=	sWire->priority;
	section->top=		sWire->top;
	section->left=		sWire->left;
	section->width=		sWire->width;
	section->height=	sWire->height;
	section->angle=		sWire->angle;
	rWire= (xkbRowWireDesc *)&sWire[1];
	for (r=0;r<sWire->nRows;r++) {
	    register int	k;
	    XkbRowPtr		row;
	    xkbKeyWireDesc *	kWire;
	    if (client->swapped) {
		register int n;
		swaps(&rWire->top,n);
		swaps(&rWire->left,n);
	    }
	    row= XkbAddGeomRow(section,rWire->nKeys);
	    if (!row)
		return BadAlloc;
	    row->top= rWire->top;
	    row->left= rWire->left;
	    row->vertical= rWire->vertical;
	    kWire= (xkbKeyWireDesc *)&rWire[1];
	    for (k=0;k<rWire->nKeys;k++) {
		XkbKeyPtr	key;
		key= XkbAddGeomKey(row);
		if (!key)
		    return BadAlloc;
		memcpy(key->name.name,kWire[k].name,XkbKeyNameLength);
		key->gap= kWire[k].gap;
		key->shape_ndx= kWire[k].shapeNdx;
		key->color_ndx= kWire[k].colorNdx;
		if (key->shape_ndx>=geom->num_shapes) {
		    client->errorValue= _XkbErrCode3(0x10,key->shape_ndx,
							  geom->num_shapes);
		    return BadMatch;
		}
		if (key->color_ndx>=geom->num_colors) {
		    client->errorValue= _XkbErrCode3(0x11,key->color_ndx,
							  geom->num_colors);
		    return BadMatch;
		}
	    }
	    rWire= (xkbRowWireDesc *)&kWire[rWire->nKeys];
	}
	wire= (char *)rWire;
	if (sWire->nDoodads>0) {
	    register int d;
	    for (d=0;d<sWire->nDoodads;d++) {
		status=_CheckSetDoodad(&wire,geom,section,client);
		if (status!=Success)
		    return status;
	    }
	}
	if (sWire->nOverlays>0) {
	    register int o;
	    for (o=0;o<sWire->nOverlays;o++) {
		status= _CheckSetOverlay(&wire,geom,section,client);
		if (status!=Success)
		    return status;
	    }
	}
	sWire= (xkbSectionWireDesc *)wire;
    }
    wire= (char *)sWire;
    *wire_inout= wire;
    return Success;
}

static Status
#if NeedFunctionPrototypes
_CheckSetShapes( 	XkbGeometryPtr		geom,
			xkbSetGeometryReq *	req,
			char **			wire_inout,
			ClientPtr		client)
#else
_CheckSetShapes(geom,req,wire_inout,client)
    XkbGeometryPtr	geom;
    xkbSetGeometryReq *	req;
    char **		wire_inout;
    ClientPtr		client;
#endif
{
register int	i;
char *		wire;

    wire= *wire_inout;
    if (req->nShapes<1) {
	client->errorValue= _XkbErrCode2(0x06,req->nShapes);
	return BadValue;
    }
    else {
	xkbShapeWireDesc *	shapeWire;
	XkbShapePtr		shape;
	register int		o;
	shapeWire= (xkbShapeWireDesc *)wire;
	for (i=0;i<req->nShapes;i++) {
	    xkbOutlineWireDesc *	olWire;
	    XkbOutlinePtr		ol;
	    shape= XkbAddGeomShape(geom,shapeWire->name,shapeWire->nOutlines);
	    if (!shape)
		return BadAlloc;
	    olWire= (xkbOutlineWireDesc *)(&shapeWire[1]);
	    for (o=0;o<shapeWire->nOutlines;o++) {
		register int		p;
		XkbPointPtr		pt;
		xkbPointWireDesc *	ptWire;

		ol= XkbAddGeomOutline(shape,olWire->nPoints);
		if (!ol)
		    return BadAlloc;
		ol->corner_radius=	olWire->cornerRadius;
		ptWire= (xkbPointWireDesc *)&olWire[1];
		for (p=0,pt=ol->points;p<olWire->nPoints;p++,pt++) {
		    pt->x= ptWire[p].x;
		    pt->y= ptWire[p].y;
		    if (client->swapped) {
			register int n;
			swaps(&pt->x,n);
			swaps(&pt->y,n);
		    }
		}
		ol->num_points= olWire->nPoints;
		olWire= (xkbOutlineWireDesc *)(&ptWire[olWire->nPoints]);
	    }
	    if (shapeWire->primaryNdx!=XkbNoShape)
		shape->primary= &shape->outlines[shapeWire->primaryNdx];
	    if (shapeWire->approxNdx!=XkbNoShape)
		shape->approx= &shape->outlines[shapeWire->approxNdx];
	    shapeWire= (xkbShapeWireDesc *)olWire;
	}
	wire= (char *)shapeWire;
    }
    if (geom->num_shapes!=req->nShapes) {
	client->errorValue= _XkbErrCode3(0x07,geom->num_shapes,req->nShapes);
	return BadMatch;
    }

    *wire_inout= wire;
    return Success;
}

static Status
#if NeedFunctionPrototypes
_CheckSetGeom(	XkbGeometryPtr		geom,
		xkbSetGeometryReq *	req,
		ClientPtr 		client)
#else
_CheckSetGeom(geom,req,client)
    XkbGeometryPtr	geom;
    xkbSetGeometryReq *	req;
    ClientPtr		client;
#endif
{
register int	i;
Status		status;
char *		wire;

    wire= (char *)&req[1];
    geom->label_font= _GetCountedString(&wire,client->swapped);

    for (i=0;i<req->nProperties;i++) {
	char *name,*val;
	name= _GetCountedString(&wire,client->swapped);
	val= _GetCountedString(&wire,client->swapped);
	if ((!name)||(!val)||(XkbAddGeomProperty(geom,name,val)==NULL))
	    return BadAlloc;
    }

    if (req->nColors<2) {
	client->errorValue= _XkbErrCode3(0x01,2,req->nColors);
	return BadValue;
    }
    if (req->baseColorNdx>req->nColors) {
	client->errorValue=_XkbErrCode3(0x03,req->nColors,req->baseColorNdx);
	return BadMatch;
    }
    if (req->labelColorNdx>req->nColors) {
	client->errorValue= _XkbErrCode3(0x03,req->nColors,req->labelColorNdx);
	return BadMatch;
    }
    if (req->labelColorNdx==req->baseColorNdx) {
	client->errorValue= _XkbErrCode3(0x04,req->baseColorNdx,
							req->labelColorNdx);
	return BadMatch;
    }

    for (i=0;i<req->nColors;i++) {
	char *name;
	name= _GetCountedString(&wire,client->swapped);
	if ((!name)||(!XkbAddGeomColor(geom,name,geom->num_colors)))
	    return BadAlloc;
    }
    if (req->nColors!=geom->num_colors) {
	client->errorValue= _XkbErrCode3(0x05,req->nColors,geom->num_colors);
	return BadMatch;
    }
    geom->label_color= &geom->colors[req->labelColorNdx];
    geom->base_color= &geom->colors[req->baseColorNdx];

    if ((status=_CheckSetShapes(geom,req,&wire,client))!=Success)
	return status;

    if ((status=_CheckSetSections(geom,req,&wire,client))!=Success)
	return status;

    for (i=0;i<req->nDoodads;i++) {
	status=_CheckSetDoodad(&wire,geom,NULL,client);
	if (status!=Success)
	    return status;
    }

    for (i=0;i<req->nKeyAliases;i++) {
	if (XkbAddGeomKeyAlias(geom,&wire[XkbKeyNameLength],wire)==NULL)
	    return BadAlloc;
	wire+= 2*XkbKeyNameLength;
    }
    return Success;
}

int
#if NeedFunctionPrototypes
ProcXkbSetGeometry(ClientPtr client)
#else
ProcXkbSetGeometry(client)
    ClientPtr client;
#endif
{
    DeviceIntPtr 	dev;
    XkbGeometryPtr	geom,old;
    XkbGeometrySizesRec	sizes;
    Status		status;
    XkbDescPtr		xkb;
    Bool		new_name;
    xkbNewKeyboardNotify	nkn;

    REQUEST(xkbSetGeometryReq);
    REQUEST_AT_LEAST_SIZE(xkbSetGeometryReq);

    if (!(client->xkbClientFlags&_XkbClientInitialized))
	return BadAccess;

    CHK_KBD_DEVICE(dev,stuff->deviceSpec);
    CHK_ATOM_OR_NONE(stuff->name);

    xkb= dev->key->xkbInfo->desc;
    old= xkb->geom;
    xkb->geom= NULL;

    sizes.which= 		XkbGeomAllMask;
    sizes.num_properties=	stuff->nProperties;
    sizes.num_colors=	  	stuff->nColors;
    sizes.num_shapes=	  	stuff->nShapes;
    sizes.num_sections=	  	stuff->nSections;
    sizes.num_doodads=	  	stuff->nDoodads;
    sizes.num_key_aliases=	stuff->nKeyAliases;
    if ((status= XkbAllocGeometry(xkb,&sizes))!=Success) {
	xkb->geom= old;
	return status;
    }
    geom= xkb->geom;
    geom->name= stuff->name;
    geom->width_mm= stuff->widthMM;
    geom->height_mm= stuff->heightMM;
    if ((status= _CheckSetGeom(geom,stuff,client))!=Success) {
	XkbFreeGeometry(geom,XkbGeomAllMask,True);
	xkb->geom= old;
	return status;
    }
    new_name= (xkb->names->geometry!=geom->name);
    xkb->names->geometry= geom->name;
    if (old)
    	XkbFreeGeometry(old,XkbGeomAllMask,True);
    if (new_name) {
	xkbNamesNotify	nn;
	bzero(&nn,sizeof(xkbNamesNotify));
	nn.changed= XkbGeometryNameMask;
	XkbSendNamesNotify(dev,&nn);
    }
    nkn.deviceID= nkn.oldDeviceID= dev->id;
    nkn.minKeyCode= nkn.oldMinKeyCode= xkb->min_key_code;
    nkn.maxKeyCode= nkn.oldMaxKeyCode= xkb->max_key_code;
    nkn.requestMajor=	XkbReqCode;
    nkn.requestMinor=	X_kbSetGeometry;
    nkn.changed=	XkbNKN_GeometryMask;
    XkbSendNewKeyboardNotify(dev,&nkn);
    return Success;
}

/***====================================================================***/

int
#if NeedFunctionPrototypes
ProcXkbPerClientFlags(ClientPtr client)
#else
ProcXkbPerClientFlags(client)
    ClientPtr client;
#endif
{
    DeviceIntPtr 		dev;
    xkbPerClientFlagsReply 	rep;
    XkbInterestPtr		interest;

    REQUEST(xkbPerClientFlagsReq);
    REQUEST_SIZE_MATCH(xkbPerClientFlagsReq);

    if (!(client->xkbClientFlags&_XkbClientInitialized))
	return BadAccess;

    CHK_KBD_DEVICE(dev,stuff->deviceSpec);
    CHK_MASK_LEGAL(0x01,stuff->change,XkbPCF_AllFlagsMask);
    CHK_MASK_MATCH(0x02,stuff->change,stuff->value);

    interest = XkbFindClientResource((DevicePtr)dev,client);
    rep.type= X_Reply;
    rep.length = 0;
    rep.sequenceNumber = client->sequence;
    if (stuff->change) {
	client->xkbClientFlags&= ~stuff->change;
	client->xkbClientFlags|= stuff->value;
    }
    if (stuff->change&XkbPCF_AutoResetControlsMask) {
	Bool	want;
	want= stuff->value&XkbPCF_AutoResetControlsMask;
	if (interest && !want) {
	    interest->autoCtrls= interest->autoCtrlValues= 0;
	}
	else if (want && (!interest)) {
	    XID id = FakeClientID(client->index);
	    AddResource(id,RT_XKBCLIENT,dev);
	    interest= XkbAddClientResource((DevicePtr)dev,client,id);
	    if (!interest)
		return BadAlloc;
	}
	if (interest && want ) {
	    register unsigned affect;
	    affect= stuff->ctrlsToChange;

	    CHK_MASK_LEGAL(0x03,affect,XkbAllBooleanCtrlsMask);
	    CHK_MASK_MATCH(0x04,affect,stuff->autoCtrls);
	    CHK_MASK_MATCH(0x05,stuff->autoCtrls,stuff->autoCtrlValues);

	    interest->autoCtrls&= ~affect;
	    interest->autoCtrlValues&= ~affect;
	    interest->autoCtrls|= stuff->autoCtrls&affect;
	    interest->autoCtrlValues|= stuff->autoCtrlValues&affect;
	}
    }
    rep.supported = XkbPCF_AllFlagsMask;
    rep.value= client->xkbClientFlags&XkbPCF_AllFlagsMask;
    if (interest) {
	rep.autoCtrls= interest->autoCtrls;
	rep.autoCtrlValues= interest->autoCtrlValues;
    }
    else {
	rep.autoCtrls= rep.autoCtrlValues= 0;
    }
    if ( client->swapped ) {
	register int n;
	swaps(&rep.sequenceNumber, n);
	swapl(&rep.supported,n);
	swapl(&rep.value,n);
	swapl(&rep.autoCtrls,n);
	swapl(&rep.autoCtrlValues,n);
    }
    WriteToClient(client,SIZEOF(xkbPerClientFlagsReply), (char *)&rep);
    return client->noClientException;
}

/***====================================================================***/

/* all latin-1 alphanumerics, plus parens, minus, underscore, slash */
/* and wildcards */
static unsigned char componentSpecLegal[] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0xa7, 0xff, 0x83,
        0xfe, 0xff, 0xff, 0x87, 0xfe, 0xff, 0xff, 0x07,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0x7f, 0xff
};

/* same as above but accepts percent, plus and bar too */
static unsigned char componentExprLegal[] = {
        0x00, 0x00, 0x00, 0x00, 0x20, 0xaf, 0xff, 0x83,
        0xfe, 0xff, 0xff, 0x87, 0xfe, 0xff, 0xff, 0x17,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0x7f, 0xff
};

static char *
#if NeedFunctionPrototypes
GetComponentSpec(unsigned char **pWire,Bool allowExpr,int *errRtrn)
#else
GetComponentSpec(pWire,allowExpr,errRtrn)
   unsigned char **	pWire;
   Bool			allowExpr;
   int *		errRtrn;
#endif
{
int		len;
register int	i;
unsigned char	*wire,*str,*tmp,*legal;

    if (allowExpr)	legal= &componentExprLegal[0];
    else		legal= &componentSpecLegal[0];

    wire= *pWire;
    len= (*(unsigned char *)wire++);
    if (len>0) {
	str= (unsigned char *)_XkbCalloc(1, len+1);
	if (str) {
	    tmp= str;
	    for (i=0;i<len;i++) {
		if (legal[(*wire)/8]&(1<<((*wire)%8)))
		    *tmp++= *wire++;
		else wire++;
	    }
	    if (tmp!=str)
		*tmp++= '\0';
	    else {
		_XkbFree(str);
		str= NULL;
	    }
	}
	else {
	    *errRtrn= BadAlloc;
	}
    }
    else {
	str= NULL;
    }
    *pWire= wire;
    return (char *)str;
}

/***====================================================================***/

int
#if NeedFunctionPrototypes
ProcXkbListComponents(ClientPtr client)
#else
ProcXkbListComponents(client)
    ClientPtr client;
#endif
{
    DeviceIntPtr 		dev;
    xkbListComponentsReply 	rep;
    unsigned			len;
    int				status;
    unsigned char *		str;
    XkbSrvListInfoRec		list;

    REQUEST(xkbListComponentsReq);
    REQUEST_AT_LEAST_SIZE(xkbListComponentsReq);

    if (!(client->xkbClientFlags&_XkbClientInitialized))
	return BadAccess;

    CHK_KBD_DEVICE(dev,stuff->deviceSpec);

    status= Success;
    str= (unsigned char *)&stuff[1];
    bzero(&list,sizeof(XkbSrvListInfoRec));
    list.maxRtrn= stuff->maxNames;
    list.pattern[_XkbListKeymaps]= GetComponentSpec(&str,False,&status);
    list.pattern[_XkbListKeycodes]= GetComponentSpec(&str,False,&status);
    list.pattern[_XkbListTypes]= GetComponentSpec(&str,False,&status);
    list.pattern[_XkbListCompat]= GetComponentSpec(&str,False,&status);
    list.pattern[_XkbListSymbols]= GetComponentSpec(&str,False,&status);
    list.pattern[_XkbListGeometry]= GetComponentSpec(&str,False,&status);
    if (status!=Success)
	return status;
    len= str-((unsigned char *)stuff);
    if ((XkbPaddedSize(len)/4)!=stuff->length)
	return BadLength;
    if ((status=XkbDDXList(dev,&list,client))!=Success) {
	if (list.pool) {
	    _XkbFree(list.pool);
	    list.pool= NULL;
	}
	return status;
    }
    bzero(&rep,sizeof(xkbListComponentsReply));
    rep.type= X_Reply;
    rep.deviceID = dev->id;
    rep.sequenceNumber = client->sequence;
    rep.length = XkbPaddedSize(list.nPool)/4;
    rep.nKeymaps = list.nFound[_XkbListKeymaps];
    rep.nKeycodes = list.nFound[_XkbListKeycodes];
    rep.nTypes = list.nFound[_XkbListTypes];
    rep.nCompatMaps = list.nFound[_XkbListCompat];
    rep.nSymbols = list.nFound[_XkbListSymbols];
    rep.nGeometries = list.nFound[_XkbListGeometry];
    rep.extra=	0;
    if (list.nTotal>list.maxRtrn)
	rep.extra = (list.nTotal-list.maxRtrn);
    if (client->swapped) {
	register int n;
	swaps(&rep.sequenceNumber,n);
	swapl(&rep.length,n);
	swaps(&rep.nKeymaps,n);
	swaps(&rep.nKeycodes,n);
	swaps(&rep.nTypes,n);
	swaps(&rep.nCompatMaps,n);
	swaps(&rep.nSymbols,n);
	swaps(&rep.nGeometries,n);
	swaps(&rep.extra,n);
    }
    WriteToClient(client,SIZEOF(xkbListComponentsReply),(char *)&rep);
    if (list.nPool && list.pool) {
	WriteToClient(client,XkbPaddedSize(list.nPool), (char *)list.pool);
	_XkbFree(list.pool);
	list.pool= NULL;
    }
    return client->noClientException;
}

/***====================================================================***/

int
#if NeedFunctionPrototypes
ProcXkbGetKbdByName(ClientPtr client)
#else
ProcXkbGetKbdByName(client)
    ClientPtr client;
#endif
{
    DeviceIntPtr 		dev;
    XkbFileInfo			finfo;
    xkbGetKbdByNameReply 	rep;
    xkbGetMapReply		mrep;
    xkbGetCompatMapReply	crep;
    xkbGetIndicatorMapReply	irep;
    xkbGetNamesReply		nrep;
    xkbGetGeometryReply		grep;
    XkbComponentNamesRec	names;
    XkbDescPtr			xkb;
    unsigned char *		str;
    char 			mapFile[PATH_MAX];
    unsigned			len;
    unsigned			fwant,fneed,reported;
    int				status;
    Bool			geom_changed;

    REQUEST(xkbGetKbdByNameReq);
    REQUEST_AT_LEAST_SIZE(xkbGetKbdByNameReq);

    if (!(client->xkbClientFlags&_XkbClientInitialized))
	return BadAccess;

    CHK_KBD_DEVICE(dev,stuff->deviceSpec);

    xkb = dev->key->xkbInfo->desc;
    status= Success;
    str= (unsigned char *)&stuff[1];
    names.keymap= GetComponentSpec(&str,True,&status);
    names.keycodes= GetComponentSpec(&str,True,&status);
    names.types= GetComponentSpec(&str,True,&status);
    names.compat= GetComponentSpec(&str,True,&status);
    names.symbols= GetComponentSpec(&str,True,&status);
    names.geometry= GetComponentSpec(&str,True,&status);
    if (status!=Success)
	return status;
    len= str-((unsigned char *)stuff);
    if ((XkbPaddedSize(len)/4)!=stuff->length)
	return BadLength;

    CHK_MASK_LEGAL(0x01,stuff->want,XkbGBN_AllComponentsMask);
    CHK_MASK_LEGAL(0x02,stuff->need,XkbGBN_AllComponentsMask);
    
    if (stuff->load)
	 fwant= XkbGBN_AllComponentsMask;
    else fwant= stuff->want|stuff->need;
    if (!names.keymap) {
	if ((!names.compat)&&
    		(fwant&(XkbGBN_CompatMapMask|XkbGBN_IndicatorMapMask))) {
	    names.compat= _XkbDupString("%");
	}
	if ((!names.types)&&(fwant&(XkbGBN_TypesMask))) {
	    names.types= _XkbDupString("%");
	}
	if ((!names.symbols)&&(fwant&XkbGBN_SymbolsMask)) {
	    names.symbols= _XkbDupString("%");
	}
	geom_changed= ((names.geometry!=NULL)&&(strcmp(names.geometry,"%")!=0));
	if ((!names.geometry)&&(fwant&XkbGBN_GeometryMask)) {
	    names.geometry= _XkbDupString("%");
	    geom_changed= False;
	}
    }
    else {
	geom_changed= True;
    }

    bzero(mapFile,PATH_MAX);
    rep.type= X_Reply;
    rep.deviceID = dev->id;
    rep.sequenceNumber = client->sequence;
    rep.length = 0;
    rep.minKeyCode = xkb->min_key_code;
    rep.maxKeyCode = xkb->max_key_code;
    rep.loaded=	False;
    fwant= XkbConvertGetByNameComponents(True,stuff->want)|XkmVirtualModsMask;
    fneed= XkbConvertGetByNameComponents(True,stuff->need);
    rep.reported= XkbConvertGetByNameComponents(False,fwant|fneed);
    if (stuff->load) {
	fneed|= XkmKeymapRequired;
	fwant|= XkmKeymapLegal;
    }
    if ((fwant|fneed)&XkmSymbolsMask) {
	fneed|= XkmKeyNamesIndex|XkmTypesIndex;
	fwant|= XkmIndicatorsIndex;
    }
    rep.found = XkbDDXLoadKeymapByNames(dev,&names,fwant,fneed,&finfo,
							mapFile,PATH_MAX);
    rep.newKeyboard= False;
    rep.pad1= rep.pad2= rep.pad3= rep.pad4= 0;

    stuff->want|= stuff->need;
    if (finfo.xkb==NULL)
	rep.reported= 0;
    else {
	if (stuff->load)
	    rep.loaded= True;
	if (stuff->load || 
		((rep.reported&XkbGBN_SymbolsMask) && (finfo.xkb->compat))) {
	    XkbChangesRec changes;
	    bzero(&changes,sizeof(changes));
	    XkbUpdateDescActions(finfo.xkb,
			finfo.xkb->min_key_code,XkbNumKeys(finfo.xkb),
			&changes);
	}

	if (finfo.xkb->map==NULL)
	    rep.reported&= ~(XkbGBN_SymbolsMask|XkbGBN_TypesMask);
	else if (rep.reported&(XkbGBN_SymbolsMask|XkbGBN_TypesMask)) {
	    mrep.type= X_Reply;
	    mrep.deviceID = dev->id;
	    mrep.sequenceNumber= client->sequence;
	    mrep.length = ((SIZEOF(xkbGetMapReply)-SIZEOF(xGenericReply))>>2);
	    mrep.minKeyCode = finfo.xkb->min_key_code;
	    mrep.maxKeyCode = finfo.xkb->max_key_code;
	    mrep.present = 0;
	    mrep.totalSyms = mrep.totalActs =
		mrep.totalKeyBehaviors= mrep.totalKeyExplicit= 
		mrep.totalModMapKeys= 0;
	    if (rep.reported&(XkbGBN_TypesMask|XkbGBN_ClientSymbolsMask)) {
		mrep.present|= XkbKeyTypesMask;
		mrep.firstType = 0;
		mrep.nTypes = mrep.totalTypes= finfo.xkb->map->num_types;
	    }
	    else {
		mrep.firstType = mrep.nTypes= 0;
		mrep.totalTypes= 0;
	    }
	    if (rep.reported&XkbGBN_ClientSymbolsMask) {
		mrep.present|= (XkbKeySymsMask|XkbModifierMapMask);
		mrep.firstKeySym = mrep.firstModMapKey= finfo.xkb->min_key_code;
		mrep.nKeySyms = mrep.nModMapKeys= XkbNumKeys(finfo.xkb);
	    }
	    else {
		mrep.firstKeySym= mrep.firstModMapKey= 0;
		mrep.nKeySyms= mrep.nModMapKeys= 0;
	    }
	    if (rep.reported&XkbGBN_ServerSymbolsMask) {
		mrep.present|= XkbAllServerInfoMask;
		mrep.virtualMods= ~0;
		mrep.firstKeyAct = mrep.firstKeyBehavior = 
			mrep.firstKeyExplicit = finfo.xkb->min_key_code;
		mrep.nKeyActs = mrep.nKeyBehaviors = 
			mrep.nKeyExplicit = XkbNumKeys(finfo.xkb);
	    }
	    else {
		mrep.virtualMods= 0;
		mrep.firstKeyAct= mrep.firstKeyBehavior= 
			mrep.firstKeyExplicit = 0;
		mrep.nKeyActs= mrep.nKeyBehaviors= mrep.nKeyExplicit= 0;
	    }
	    XkbComputeGetMapReplySize(finfo.xkb,&mrep);
	    rep.length+= SIZEOF(xGenericReply)/4+mrep.length;
	}
	if (finfo.xkb->compat==NULL)
	    rep.reported&= ~XkbGBN_CompatMapMask;
	else if (rep.reported&XkbGBN_CompatMapMask) {
	    crep.type= X_Reply;
	    crep.deviceID= dev->id;
	    crep.sequenceNumber= client->sequence;
	    crep.length= 0;
	    crep.groups= XkbAllGroupsMask;
	    crep.firstSI= 0;
	    crep.nSI= crep.nTotalSI= finfo.xkb->compat->num_si;
	    XkbComputeGetCompatMapReplySize(finfo.xkb->compat,&crep);
	    rep.length+= SIZEOF(xGenericReply)/4+crep.length;
	}
	if (finfo.xkb->indicators==NULL)
	    rep.reported&= ~XkbGBN_IndicatorMapMask;
	else if (rep.reported&XkbGBN_IndicatorMapMask) {
	    irep.type= X_Reply;
	    irep.deviceID= dev->id;
	    irep.sequenceNumber= client->sequence;
	    irep.length= 0;
	    irep.which= XkbAllIndicatorsMask;
	    XkbComputeGetIndicatorMapReplySize(finfo.xkb->indicators,&irep);
	    rep.length+= SIZEOF(xGenericReply)/4+irep.length;
	}
	if (finfo.xkb->names==NULL)
	    rep.reported&= ~(XkbGBN_OtherNamesMask|XkbGBN_KeyNamesMask);
	else if (rep.reported&(XkbGBN_OtherNamesMask|XkbGBN_KeyNamesMask)) {
	    nrep.type= X_Reply;
	    nrep.deviceID= dev->id;
	    nrep.sequenceNumber= client->sequence;
	    nrep.length= 0;
	    nrep.minKeyCode= finfo.xkb->min_key_code;
	    nrep.maxKeyCode= finfo.xkb->max_key_code;
	    if (rep.reported&XkbGBN_OtherNamesMask) {
		nrep.which= XkbAllNamesMask;
		if (finfo.xkb->map!=NULL)
		     nrep.nTypes= finfo.xkb->map->num_types;
		else nrep.nTypes= 0;
		nrep.nKTLevels= 0;
		nrep.groupNames= XkbAllGroupsMask;
		nrep.virtualMods= XkbAllVirtualModsMask;
		nrep.indicators= XkbAllIndicatorsMask;
		nrep.nRadioGroups= finfo.xkb->names->num_rg;
	    }
	    else {
		nrep.which= 0;
		nrep.nTypes= 0;
		nrep.nKTLevels= 0;
		nrep.groupNames= 0;
		nrep.virtualMods= 0;
		nrep.indicators= 0;
		nrep.nRadioGroups= 0;
	    }
	    if (rep.reported&XkbGBN_KeyNamesMask) {
		nrep.which|= XkbKeyNamesMask;
		nrep.firstKey= finfo.xkb->min_key_code;
		nrep.nKeys= XkbNumKeys(finfo.xkb);
		nrep.nKeyAliases= finfo.xkb->names->num_key_aliases;
		if (nrep.nKeyAliases)
		    nrep.which|= XkbKeyAliasesMask;
	    }
	    else {
		nrep.which&= ~(XkbKeyNamesMask|XkbKeyAliasesMask);
		nrep.firstKey= nrep.nKeys= 0;
		nrep.nKeyAliases= 0;
	    }
	    XkbComputeGetNamesReplySize(finfo.xkb,&nrep);
	    rep.length+= SIZEOF(xGenericReply)/4+nrep.length;
	}
	if (finfo.xkb->geom==NULL)
	    rep.reported&= ~XkbGBN_GeometryMask;
	else if (rep.reported&XkbGBN_GeometryMask) {
	    grep.type= X_Reply;
	    grep.deviceID= dev->id;
	    grep.sequenceNumber= client->sequence;
	    grep.length= 0;
	    grep.found= True;
	    grep.pad= 0;
	    grep.widthMM= grep.heightMM= 0;
	    grep.nProperties= grep.nColors= grep.nShapes= 0;
	    grep.nSections= grep.nDoodads= 0;
	    grep.baseColorNdx= grep.labelColorNdx= 0;
	    XkbComputeGetGeometryReplySize(finfo.xkb->geom,&grep,None);
	    rep.length+= SIZEOF(xGenericReply)/4+grep.length;
	}
    }

    reported= rep.reported;
    if ( client->swapped ) {
	register int n;
	swaps(&rep.sequenceNumber,n);
	swapl(&rep.length,n);
	swaps(&rep.found,n);
	swaps(&rep.reported,n);
    }
    WriteToClient(client,SIZEOF(xkbGetKbdByNameReply), (char *)&rep);
    if (reported&(XkbGBN_SymbolsMask|XkbGBN_TypesMask))
	XkbSendMap(client,finfo.xkb,&mrep);
    if (reported&XkbGBN_CompatMapMask)
	XkbSendCompatMap(client,finfo.xkb->compat,&crep);
    if (reported&XkbGBN_IndicatorMapMask)
	XkbSendIndicatorMap(client,finfo.xkb->indicators,&irep);
    if (reported&(XkbGBN_KeyNamesMask|XkbGBN_OtherNamesMask))
	XkbSendNames(client,finfo.xkb,&nrep);
    if (reported&XkbGBN_GeometryMask)
	XkbSendGeometry(client,finfo.xkb->geom,&grep,False);
    if (rep.loaded) {
	XkbDescPtr		old_xkb;
	xkbNewKeyboardNotify 	nkn;
	int 			i,nG,nTG;
	old_xkb= xkb;
	xkb= finfo.xkb;
	dev->key->xkbInfo->desc= xkb;
	finfo.xkb= old_xkb; /* so it'll get freed automatically */

	if (dev->kbdfeed && dev->kbdfeed->xkb_sli) {
	    XkbFreeSrvLedInfo(dev->kbdfeed->xkb_sli);
	    dev->kbdfeed->xkb_sli= NULL;
	}
	*xkb->ctrls= *old_xkb->ctrls;
	for (nG=nTG=0,i=xkb->min_key_code;i<=xkb->max_key_code;i++) {
	    nG= XkbKeyNumGroups(xkb,i);
	    if (nG>=XkbNumKbdGroups) {
		nTG= XkbNumKbdGroups;
		break;
	    }
	    if (nG>nTG) {
		nTG= nG;
	    }
	}
	xkb->ctrls->num_groups= nTG;

	memcpy(dev->key->modifierMap,xkb->map->modmap,xkb->max_key_code+1);
	XkbUpdateCoreDescription(dev,True);

	nkn.deviceID= nkn.oldDeviceID= dev->id;
	nkn.minKeyCode= finfo.xkb->min_key_code;
	nkn.maxKeyCode= finfo.xkb->max_key_code;
	nkn.oldMinKeyCode= xkb->min_key_code;
	nkn.oldMaxKeyCode= xkb->max_key_code;
	nkn.requestMajor= XkbReqCode;
	nkn.requestMinor= X_kbGetKbdByName;
	nkn.changed= XkbNKN_KeycodesMask;
	if (geom_changed)
	    nkn.changed|= XkbNKN_GeometryMask;
	XkbSendNewKeyboardNotify(dev,&nkn);
    }
    if ((finfo.xkb!=NULL)&&(finfo.xkb!=xkb)) {
	XkbFreeKeyboard(finfo.xkb,XkbAllComponentsMask,True);
	finfo.xkb= NULL;
    }
    if (names.keymap)	{ _XkbFree(names.keymap); names.keymap= NULL; }
    if (names.keycodes)	{ _XkbFree(names.keycodes); names.keycodes= NULL; }
    if (names.types)	{ _XkbFree(names.types); names.types= NULL; }
    if (names.compat)	{ _XkbFree(names.compat); names.compat= NULL; }
    if (names.symbols)	{ _XkbFree(names.symbols); names.symbols= NULL; }
    if (names.geometry)	{ _XkbFree(names.geometry); names.geometry= NULL; }
    return client->noClientException;
}

/***====================================================================***/

static int
#if NeedFunctionPrototypes
ComputeDeviceLedInfoSize(	DeviceIntPtr		dev,
				unsigned int		what,
				XkbSrvLedInfoPtr	sli)
#else
ComputeDeviceLedInfoSize(dev,what,sli)
    DeviceIntPtr		dev;
    unsigned int		what;
    XkbSrvLedInfoPtr		sli;
#endif
{
int			nNames,nMaps;
register unsigned 	n,bit;

    if (sli==NULL)
	return 0;
    nNames= nMaps= 0;
    if ((what&XkbXI_IndicatorNamesMask)==0)
	sli->namesPresent= 0;
    if ((what&XkbXI_IndicatorMapsMask)==0)
	sli->mapsPresent= 0;

    for (n=0,bit=1;n<XkbNumIndicators;n++,bit<<=1) {
	if (sli->names && sli->names[n]!=None) {
	    sli->namesPresent|= bit;
	    nNames++;
	}
	if (sli->maps && XkbIM_InUse(&sli->maps[n])) {
	    sli->mapsPresent|= bit;
	    nMaps++;
	}
    }
    return (nNames*4)+(nMaps*SIZEOF(xkbIndicatorMapWireDesc));
}

static int 
#if NeedFunctionPrototypes
CheckDeviceLedFBs(	DeviceIntPtr			dev,
			int				class,
			int				id,
			xkbGetDeviceInfoReply *		rep,
			ClientPtr			client)
#else
CheckDeviceLedFBs(dev,class,id,rep,client)
    DeviceIntPtr		dev;
    int				class;
    int				id;
    xkbGetDeviceInfoReply *	rep;
    ClientPtr			client;
#endif
{
int			nFBs= 0;
int			length= 0;
Bool			classOk;

    if (class==XkbDfltXIClass) {
	if (dev->kbdfeed)	class= KbdFeedbackClass;
	else if (dev->leds)	class= LedFeedbackClass;
	else {
	    client->errorValue= _XkbErrCode2(XkbErr_BadClass,class);
	    return XkbKeyboardErrorCode;
	}
    }
    classOk= False;
    if ((dev->kbdfeed)&&((class==KbdFeedbackClass)||(class==XkbAllXIClasses))) {
	KbdFeedbackPtr kf;
	classOk= True;
	for (kf= dev->kbdfeed;(kf);kf=kf->next) {
	    if ((id!=XkbAllXIIds)&&(id!=XkbDfltXIId)&&(id!=kf->ctrl.id))
		continue;
	    nFBs++;
	    length+= SIZEOF(xkbDeviceLedsWireDesc);
	    if (!kf->xkb_sli)
		kf->xkb_sli= XkbAllocSrvLedInfo(dev,kf,NULL,0);
	    length+= ComputeDeviceLedInfoSize(dev,rep->present,kf->xkb_sli);
	    if (id!=XkbAllXIIds)
		break;
	}
    }
    if ((dev->leds)&&((class==LedFeedbackClass)||(class==XkbAllXIClasses))) {
	LedFeedbackPtr lf;
	classOk= True;
	for (lf= dev->leds;(lf);lf=lf->next) {
	    if ((id!=XkbAllXIIds)&&(id!=XkbDfltXIId)&&(id!=lf->ctrl.id))
		continue;
	    nFBs++;
	    length+= SIZEOF(xkbDeviceLedsWireDesc);
	    if (!lf->xkb_sli)
		lf->xkb_sli= XkbAllocSrvLedInfo(dev,NULL,lf,0);
	    length+= ComputeDeviceLedInfoSize(dev,rep->present,lf->xkb_sli);
	    if (id!=XkbAllXIIds)
		break;
	}
    }
    if (nFBs>0) {
	if (rep->supported&XkbXI_IndicatorsMask) {
	    rep->nDeviceLedFBs= nFBs;
	    rep->length+= (length/4);
	}
	return Success;
    }
    if (classOk) client->errorValue= _XkbErrCode2(XkbErr_BadId,id);
    else	 client->errorValue= _XkbErrCode2(XkbErr_BadClass,class);
    return XkbKeyboardErrorCode;
}

static int
#if NeedFunctionPrototypes
SendDeviceLedInfo(	XkbSrvLedInfoPtr	sli,
			ClientPtr		client)
#else
SendDeviceLedInfo(sli,client)
    XkbSrvLedInfoPtr	sli;
    ClientPtr		client;
#endif
{
xkbDeviceLedsWireDesc	wire;
int			length;

    length= 0;
    wire.ledClass= 		sli->class;
    wire.ledID= 		sli->id;
    wire.namesPresent= 		sli->namesPresent;
    wire.mapsPresent=   	sli->mapsPresent;
    wire.physIndicators= 	sli->physIndicators;
    wire.state=			sli->effectiveState;
    if (client->swapped) {
	register int n;
	swaps(&wire.ledClass,n);
	swaps(&wire.ledID,n);
	swapl(&wire.namesPresent,n);
	swapl(&wire.mapsPresent,n);
	swapl(&wire.physIndicators,n);
	swapl(&wire.state,n);
    }
    WriteToClient(client,SIZEOF(xkbDeviceLedsWireDesc),(char *)&wire);
    length+= SIZEOF(xkbDeviceLedsWireDesc);
    if (sli->namesPresent|sli->mapsPresent) {
	register unsigned i,bit;
	if (sli->namesPresent) {
	    CARD32	awire;
	    for (i=0,bit=1;i<XkbNumIndicators;i++,bit<<=1) {
		if (sli->namesPresent&bit) {
		    awire= (CARD32)sli->names[i];
		    if (client->swapped) {
			register int n;
			swapl(&awire,n);
		    }
		    WriteToClient(client,4,(char *)&awire);
		    length+= 4;
		}
	    }
	}
	if (sli->mapsPresent) {
	    for (i=0,bit=1;i<XkbNumIndicators;i++,bit<<=1) {
		xkbIndicatorMapWireDesc	iwire;
		if (sli->mapsPresent&bit) {
		    iwire.flags= 	sli->maps[i].flags;
		    iwire.whichGroups=	sli->maps[i].which_groups;
		    iwire.groups=	sli->maps[i].groups;
		    iwire.whichMods=	sli->maps[i].which_mods;
		    iwire.mods=		sli->maps[i].mods.mask;
		    iwire.realMods=	sli->maps[i].mods.real_mods;
		    iwire.virtualMods=	sli->maps[i].mods.vmods;
		    iwire.ctrls= 	sli->maps[i].ctrls;
		    if (client->swapped) {
			register int n;
			swaps(&iwire.virtualMods,n);
			swapl(&iwire.ctrls,n);
		    }
		    WriteToClient(client,SIZEOF(xkbIndicatorMapWireDesc),
								(char *)&iwire);
		    length+= SIZEOF(xkbIndicatorMapWireDesc);
		}
	    }
	}
    }
    return length;
}

static int
#if NeedFunctionPrototypes
SendDeviceLedFBs(	DeviceIntPtr	dev,
			int		class,
			int		id,
			unsigned	wantLength,
			ClientPtr	client)
#else
SendDeviceLedFBs(dev,class,id,wantLength,client)
    DeviceIntPtr	dev;
    int			class;
    int			id;
    unsigned		wantLength;
    ClientPtr		client;
#endif
{
int			length= 0;

    if (class==XkbDfltXIClass) {
	if (dev->kbdfeed)	class= KbdFeedbackClass;
	else if (dev->leds)	class= LedFeedbackClass;
    }
    if ((dev->kbdfeed)&&
	((class==KbdFeedbackClass)||(class==XkbAllXIClasses))) {
	KbdFeedbackPtr kf;
	for (kf= dev->kbdfeed;(kf);kf=kf->next) {
	    if ((id==XkbAllXIIds)||(id==XkbDfltXIId)||(id==kf->ctrl.id)) {
		length+= SendDeviceLedInfo(kf->xkb_sli,client);
		if (id!=XkbAllXIIds)
		    break;
	    }
	}
    }
    if ((dev->leds)&&
	((class==LedFeedbackClass)||(class==XkbAllXIClasses))) {
	LedFeedbackPtr lf;
	for (lf= dev->leds;(lf);lf=lf->next) {
	    if ((id==XkbAllXIIds)||(id==XkbDfltXIId)||(id==lf->ctrl.id)) {
		length+= SendDeviceLedInfo(lf->xkb_sli,client);
		if (id!=XkbAllXIIds)
		    break;
	    }
	}
    }
    if (length==wantLength)
	 return Success;
    else return BadLength;
}

int
#if NeedFunctionPrototypes
ProcXkbGetDeviceInfo(ClientPtr client)
#else
ProcXkbGetDeviceInfo(client)
    ClientPtr client;
#endif
{
DeviceIntPtr		dev;
xkbGetDeviceInfoReply	rep;
int			status,nDeviceLedFBs;
unsigned		length,nameLen;
CARD16			ledClass,ledID;
unsigned		wanted,supported;
char *			str;

    REQUEST(xkbGetDeviceInfoReq);
    REQUEST_SIZE_MATCH(xkbGetDeviceInfoReq);

    if (!(client->xkbClientFlags&_XkbClientInitialized))
	return BadAccess;

    wanted= stuff->wanted;

    CHK_ANY_DEVICE(dev,stuff->deviceSpec);
    CHK_MASK_LEGAL(0x01,wanted,XkbXI_AllDeviceFeaturesMask);

    if ((!dev->button)||((stuff->nBtns<1)&&(!stuff->allBtns)))
	wanted&= ~XkbXI_ButtonActionsMask;
    if ((!dev->kbdfeed)&&(!dev->leds))
	wanted&= ~XkbXI_IndicatorsMask;
    wanted&= ~XkbXIUnsupported;

    nameLen= XkbSizeCountedString(dev->name);
    bzero((char *)&rep,SIZEOF(xkbGetDeviceInfoReply));
    rep.type = X_Reply;
    rep.deviceID= dev->id;
    rep.sequenceNumber = client->sequence;
    rep.length = nameLen/4;
    rep.present = wanted;
    rep.supported = XkbXI_AllDeviceFeaturesMask&(~XkbXIUnsupported);
    rep.unsupported = XkbXIUnsupported;
    rep.firstBtnWanted = rep.nBtnsWanted = 0;
    rep.firstBtnRtrn = rep.nBtnsRtrn = 0;
    if (dev->button)
	 rep.totalBtns= dev->button->numButtons;
    else rep.totalBtns= 0;
    rep.devType=	dev->type;
    rep.hasOwnState=	(dev->key && dev->key->xkbInfo);
    rep.nDeviceLedFBs = 0;
    if (dev->kbdfeed)	rep.dfltKbdFB= dev->kbdfeed->ctrl.id;
    else		rep.dfltKbdFB= XkbXINone;
    if (dev->leds)	rep.dfltLedFB= dev->leds->ctrl.id;
    else		rep.dfltLedFB= XkbXINone;

    ledClass= stuff->ledClass;
    ledID= stuff->ledID;

    rep.firstBtnWanted= rep.nBtnsWanted= 0;
    rep.firstBtnRtrn= rep.nBtnsRtrn= 0;
    if (wanted&XkbXI_ButtonActionsMask) {
	if (stuff->allBtns) {
	    stuff->firstBtn= 0;
	    stuff->nBtns= dev->button->numButtons;
	}

	if ((stuff->firstBtn+stuff->nBtns)>dev->button->numButtons) {
	    client->errorValue = _XkbErrCode4(0x02,dev->button->numButtons,
							stuff->firstBtn,
							stuff->nBtns);
	    return BadValue;
	}
	else {
	    rep.firstBtnWanted= stuff->firstBtn;
	    rep.nBtnsWanted= stuff->nBtns;
	    if (dev->button->xkb_acts!=NULL) {
		XkbAction *act;
		register int i;

		rep.firstBtnRtrn= stuff->firstBtn;
		rep.nBtnsRtrn= stuff->nBtns;
		act= &dev->button->xkb_acts[rep.firstBtnWanted];
		for (i=0;i<rep.nBtnsRtrn;i++,act++) {
		    if (act->type!=XkbSA_NoAction)
			break;
		}
		rep.firstBtnRtrn+=	i;
		rep.nBtnsRtrn-=		i;
		act= &dev->button->xkb_acts[rep.firstBtnRtrn+rep.nBtnsRtrn-1];
		for (i=0;i<rep.nBtnsRtrn;i++,act--) {
		    if (act->type!=XkbSA_NoAction)
			break;
		}
		rep.nBtnsRtrn-=		i;
	    }
	    rep.length+= (rep.nBtnsRtrn*SIZEOF(xkbActionWireDesc))/4;
	}
    }

    if (wanted&XkbXI_IndicatorsMask) {
	status= CheckDeviceLedFBs(dev,ledClass,ledID,&rep,client);
	if (status!=Success)
	    return status;
    }
    length= rep.length*4;
    supported= rep.supported;
    nDeviceLedFBs = rep.nDeviceLedFBs;
    if (client->swapped) {
	register int n;
	swaps(&rep.sequenceNumber,n);
	swapl(&rep.length,n);
	swaps(&rep.present,n);
	swaps(&rep.supported,n);
	swaps(&rep.unsupported,n);
	swaps(&rep.nDeviceLedFBs,n);
	swapl(&rep.type,n);
    }
    WriteToClient(client,SIZEOF(xkbGetDeviceInfoReply), (char *)&rep);

    str= (char*) ALLOCATE_LOCAL(nameLen);
    if (!str) 
	return BadAlloc;
    XkbWriteCountedString(str,dev->name,client->swapped);
    WriteToClient(client,nameLen,str);
    DEALLOCATE_LOCAL(str);
    length-= nameLen;

    if (rep.nBtnsRtrn>0) {
	int			sz;
	xkbActionWireDesc *	awire;
	sz= rep.nBtnsRtrn*SIZEOF(xkbActionWireDesc);
	awire= (xkbActionWireDesc *)&dev->button->xkb_acts[rep.firstBtnRtrn];
	WriteToClient(client,sz,(char *)awire);
	length-= sz;
    }
    if (nDeviceLedFBs>0) {
	status= SendDeviceLedFBs(dev,ledClass,ledID,length,client);
	if (status!=Success)
	    return status;
    }
    else if (length!=0)  {
#ifdef DEBUG
	ErrorF("Internal Error!  BadLength in ProcXkbGetDeviceInfo\n");
	ErrorF("                 Wrote %d fewer bytes than expected\n",length);
#endif
	return BadLength;
    }
    if (stuff->wanted&(~supported)) {
	xkbExtensionDeviceNotify ed;
	bzero((char *)&ed,SIZEOF(xkbExtensionDeviceNotify));
	ed.ledClass=		ledClass;
	ed.ledID=		ledID;
	ed.ledsDefined= 	0;
	ed.ledState=		0;
	ed.firstBtn= ed.nBtns=	0;
	ed.reason=		XkbXI_UnsupportedFeatureMask;
	ed.supported=		supported;
	ed.unsupported=		stuff->wanted&(~supported);
	XkbSendExtensionDeviceNotify(dev,client,&ed);
    }
    return client->noClientException;
}

static char *
#if NeedFunctionPrototypes
CheckSetDeviceIndicators(	char *		wire,
				DeviceIntPtr	dev,
				int		num,
				int *		status_rtrn,
				ClientPtr	client)
#else
CheckSetDeviceIndicators(wire,dev,num,status_rtrn,client)
    char *		wire;
    DeviceIntPtr	dev;
    int			num;
    int *		status_rtrn;
    ClientPtr		client;
#endif
{
xkbDeviceLedsWireDesc *	ledWire;
int			i;
XkbSrvLedInfoPtr 	sli;

    ledWire= (xkbDeviceLedsWireDesc *)wire;
    for (i=0;i<num;i++) {
	if (client->swapped) {
	   register int n;
	   swaps(&ledWire->ledClass,n);
	   swaps(&ledWire->ledID,n);
	   swapl(&ledWire->namesPresent,n);
	   swapl(&ledWire->mapsPresent,n);
	   swapl(&ledWire->physIndicators,n);
	}

        sli= XkbFindSrvLedInfo(dev,ledWire->ledClass,ledWire->ledID,
							XkbXI_IndicatorsMask);
	if (sli!=NULL) {
	    register int n;
	    register unsigned bit;
	    int nMaps,nNames;
	    CARD32 *atomWire;
	    xkbIndicatorMapWireDesc *mapWire;

	    nMaps= nNames= 0;
	    for (n=0,bit=1;n<XkbNumIndicators;n++,bit<<=1) {
		if (ledWire->namesPresent&bit)
		    nNames++;
		if (ledWire->mapsPresent&bit)
		    nMaps++;
	    }
	    atomWire= (CARD32 *)&ledWire[1];
	    if (nNames>0) {
		for (n=0;n<nNames;n++) {
		    if (client->swapped) {
			register int t;
			swapl(atomWire,t);
		    }
		    CHK_ATOM_OR_NONE3(((Atom)(*atomWire)),client->errorValue,
							*status_rtrn,NULL);
		    atomWire++;
		}
	    }
	    mapWire= (xkbIndicatorMapWireDesc *)atomWire;
	    if (nMaps>0) {
		for (n=0;n<nMaps;n++) {
		    if (client->swapped) {
			register int t;
			swaps(&mapWire->virtualMods,t);
			swapl(&mapWire->ctrls,t);
		    }
		    CHK_MASK_LEGAL3(0x21,mapWire->whichGroups,
						XkbIM_UseAnyGroup,
						client->errorValue,
						*status_rtrn,NULL);
		    CHK_MASK_LEGAL3(0x22,mapWire->whichMods,XkbIM_UseAnyMods,
						client->errorValue,
						*status_rtrn,NULL);
		    mapWire++;
		}
	    }
	    ledWire= (xkbDeviceLedsWireDesc *)mapWire;
	}
	else {
	    /* SHOULD NEVER HAPPEN */
	    return (char *)ledWire;
	}
    }
    return (char *)ledWire;
}

static char *
#if NeedFunctionPrototypes
SetDeviceIndicators(	char *			wire,
			DeviceIntPtr		dev,
			unsigned		changed,
			int			num,
			int *			status_rtrn,
			ClientPtr		client,
			xkbExtensionDeviceNotify *ev)
#else
SetDeviceIndicators(wire,dev,changed,num,status_rtrn,client,ev)
    char *		wire;
    DeviceIntPtr	dev;
    unsigned		changed;
    int			num;
    ClientPtr		client;
    int *		status_rtrn;
    xkbExtensionDeviceNotify *ev;
#endif
{
xkbDeviceLedsWireDesc *		ledWire;
int				i;
XkbEventCauseRec		cause;
unsigned			namec,mapc,statec;
xkbExtensionDeviceNotify	ed;
XkbChangesRec			changes;
DeviceIntPtr			kbd;

    bzero((char *)&ed,sizeof(xkbExtensionDeviceNotify));
    bzero((char *)&changes,sizeof(XkbChangesRec));
    XkbSetCauseXkbReq(&cause,X_kbSetDeviceInfo,client);
    ledWire= (xkbDeviceLedsWireDesc *)wire;
    for (i=0;i<num;i++) {
	register int			n;
	register unsigned 		bit;
	CARD32 *			atomWire;
	xkbIndicatorMapWireDesc *	mapWire;
	XkbSrvLedInfoPtr		sli;

	namec= mapc= statec= 0;
    	sli= XkbFindSrvLedInfo(dev,ledWire->ledClass,ledWire->ledID,
						XkbXI_IndicatorMapsMask);
	if (!sli) {
	    /* SHOULD NEVER HAPPEN!! */
	    return (char *)ledWire;
	}

	atomWire= (CARD32 *)&ledWire[1];
	if (changed&XkbXI_IndicatorNamesMask) {
	    namec= sli->namesPresent|ledWire->namesPresent;
	    bzero((char *)sli->names,XkbNumIndicators*sizeof(Atom));
	}
	if (ledWire->namesPresent) {
	    sli->namesPresent= ledWire->namesPresent;
	    bzero((char *)sli->names,XkbNumIndicators*sizeof(Atom));
	    for (n=0,bit=1;n<XkbNumIndicators;n++,bit<<=1) {
		if (ledWire->namesPresent&bit) {
		     sli->names[n]= (Atom)*atomWire;
		     if (sli->names[n]==None)
			ledWire->namesPresent&= ~bit;
		     atomWire++; 
		}
	    }
	}
	mapWire= (xkbIndicatorMapWireDesc *)atomWire;
	if (changed&XkbXI_IndicatorMapsMask) {
	    mapc= sli->mapsPresent|ledWire->mapsPresent;
	    sli->mapsPresent= ledWire->mapsPresent;
	    bzero((char*)sli->maps,XkbNumIndicators*sizeof(XkbIndicatorMapRec));
	}
	if (ledWire->mapsPresent) {
	    for (n=0,bit=1;n<XkbNumIndicators;n++,bit<<=1) {
		if (ledWire->mapsPresent&bit) {
		    sli->maps[n].flags=		mapWire->flags;
		    sli->maps[n].which_groups=	mapWire->whichGroups;
		    sli->maps[n].groups=	mapWire->groups;
		    sli->maps[n].which_mods=	mapWire->whichMods;
		    sli->maps[n].mods.mask=	mapWire->mods;
		    sli->maps[n].mods.real_mods=mapWire->realMods;
		    sli->maps[n].mods.vmods=	mapWire->virtualMods;
		    sli->maps[n].ctrls=		mapWire->ctrls;
		    mapWire++; 
		}
	    }
	}
	if (changed&XkbXI_IndicatorStateMask) {
	    statec= sli->effectiveState^ledWire->state;
	    sli->explicitState&= ~statec;
	    sli->explicitState|= (ledWire->state&statec);
	}
	if (namec)
	    XkbApplyLedNameChanges(dev,sli,namec,&ed,&changes,&cause);
	if (mapc)
	    XkbApplyLedMapChanges(dev,sli,mapc,&ed,&changes,&cause);
	if (statec)
	    XkbApplyLedStateChanges(dev,sli,statec,&ed,&changes,&cause);

	kbd= dev;
	if ((sli->flags&XkbSLI_HasOwnState)==0)
	    kbd= (DeviceIntPtr)LookupKeyboardDevice();

	XkbFlushLedEvents(dev,kbd,sli,&ed,&changes,&cause);
	ledWire= (xkbDeviceLedsWireDesc *)mapWire;
    }
    return (char *)ledWire;
}

int
#if NeedFunctionPrototypes
ProcXkbSetDeviceInfo(ClientPtr client)
#else
ProcXkbSetDeviceInfo(client)
    ClientPtr client;
#endif
{
DeviceIntPtr		dev;
unsigned		change;
char *			wire;
xkbExtensionDeviceNotify ed;

    REQUEST(xkbSetDeviceInfoReq);
    REQUEST_AT_LEAST_SIZE(xkbSetDeviceInfoReq);

    if (!(client->xkbClientFlags&_XkbClientInitialized))
	return BadAccess;

    change= stuff->change;

    CHK_ANY_DEVICE(dev,stuff->deviceSpec);
    CHK_MASK_LEGAL(0x01,change,(XkbXI_AllFeaturesMask&(~XkbXI_KeyboardsMask)));

    wire= (char *)&stuff[1];
    if (change&XkbXI_ButtonActionsMask) {
	if (!dev->button) {
	    client->errorValue = _XkbErrCode2(XkbErr_BadClass,ButtonClass);
	    return XkbKeyboardErrorCode;
	}
	if ((stuff->firstBtn+stuff->nBtns)>dev->button->numButtons) {
	    client->errorValue= _XkbErrCode4(0x02,stuff->firstBtn,stuff->nBtns,
						dev->button->numButtons);
	    return BadMatch;
	}
	wire+= (stuff->nBtns*SIZEOF(xkbActionWireDesc));
    }
    if (stuff->change&XkbXI_IndicatorsMask) {
	int status= Success;
	wire= CheckSetDeviceIndicators(wire,dev,stuff->nDeviceLedFBs,
							&status,client);
	if (status!=Success)
	    return status;
    }
    if (((wire-((char *)stuff))/4)!=stuff->length)
	return BadLength;

    bzero((char *)&ed,SIZEOF(xkbExtensionDeviceNotify));
    ed.deviceID=	dev->id;
    wire= (char *)&stuff[1];
    if (change&XkbXI_ButtonActionsMask) {
	int			nBtns,sz,i;
	XkbAction *		acts;
	DeviceIntPtr		kbd;

	nBtns= dev->button->numButtons;
	acts= dev->button->xkb_acts;
	if (acts==NULL) {
	    acts= _XkbTypedCalloc(nBtns,XkbAction);
	    if (!acts)
		return BadAlloc;
	    dev->button->xkb_acts= acts;
	}
	sz= stuff->nBtns*SIZEOF(xkbActionWireDesc);
	memcpy((char *)&acts[stuff->firstBtn],(char *)wire,sz);
	wire+= sz;
	ed.reason|=	XkbXI_ButtonActionsMask;
	ed.firstBtn=	stuff->firstBtn;
	ed.nBtns=	stuff->nBtns;

	if (dev->key)	kbd= dev;
	else		kbd= (DeviceIntPtr)LookupKeyboardDevice();
	acts= &dev->button->xkb_acts[stuff->firstBtn];
	for (i=0;i<stuff->nBtns;i++,acts++) {
	    if (acts->type!=XkbSA_NoAction)
		XkbSetActionKeyMods(kbd->key->xkbInfo->desc,acts,0);
	}
    }
    if (stuff->change&XkbXI_IndicatorsMask) {
	int status= Success;
	wire= SetDeviceIndicators(wire,dev,change,stuff->nDeviceLedFBs,
							&status,client,&ed);
	if (status!=Success)
	    return status;
    }
    if ((stuff->change)&&(ed.reason))
	XkbSendExtensionDeviceNotify(dev,client,&ed);
    return client->noClientException;
}

/***====================================================================***/

int
#if NeedFunctionPrototypes
ProcXkbSetDebuggingFlags(ClientPtr client)
#else
ProcXkbSetDebuggingFlags(client)
    ClientPtr client;
#endif
{
CARD32 				newFlags,newCtrls,extraLength;
xkbSetDebuggingFlagsReply 	rep;

    REQUEST(xkbSetDebuggingFlagsReq);
    REQUEST_AT_LEAST_SIZE(xkbSetDebuggingFlagsReq);

    newFlags=  xkbDebugFlags&(~stuff->affectFlags);
    newFlags|= (stuff->flags&stuff->affectFlags);
    newCtrls=  xkbDebugCtrls&(~stuff->affectCtrls);
    newCtrls|= (stuff->ctrls&stuff->affectCtrls);
    if (xkbDebugFlags || newFlags || stuff->msgLength) {
	ErrorF("XkbDebug: Setting debug flags to 0x%x\n",newFlags);
	if (newCtrls!=xkbDebugCtrls)
	    ErrorF("XkbDebug: Setting debug controls to 0x%x\n",newCtrls);
    }
    extraLength= (stuff->length<<2)-sz_xkbSetDebuggingFlagsReq;
    if (stuff->msgLength>0) {
	char *msg;
	if (extraLength<XkbPaddedSize(stuff->msgLength)) {
	    ErrorF("XkbDebug: msgLength= %d, length= %d (should be %d)\n",
			stuff->msgLength,extraLength,
			XkbPaddedSize(stuff->msgLength));
	    return BadLength;
	}
	msg= (char *)&stuff[1];
	if (msg[stuff->msgLength-1]!='\0') {
	    ErrorF("XkbDebug: message not null-terminated\n");
	    return BadValue;
	}
	ErrorF("XkbDebug: %s\n",msg);
    }
    xkbDebugFlags = newFlags;
    xkbDebugCtrls = newCtrls;

    XkbDisableLockActions= (xkbDebugCtrls&XkbDF_DisableLocks);

    rep.type= X_Reply;
    rep.length = 0;
    rep.sequenceNumber = client->sequence;
    rep.currentFlags = newFlags;
    rep.currentCtrls = newCtrls;
    rep.supportedFlags = ~0;
    rep.supportedCtrls = ~0;
    if ( client->swapped ) {
	register int n;
	swaps(&rep.sequenceNumber, n);
	swapl(&rep.currentFlags, n);
	swapl(&rep.currentCtrls, n);
	swapl(&rep.supportedFlags, n);
	swapl(&rep.supportedCtrls, n);
    }
    WriteToClient(client,SIZEOF(xkbSetDebuggingFlagsReply), (char *)&rep);
    return client->noClientException;
}

/***====================================================================***/

static int
#if NeedFunctionPrototypes
ProcXkbDispatch (ClientPtr client)
#else
ProcXkbDispatch (client)
    ClientPtr client;
#endif
{
    REQUEST(xReq);
    switch (stuff->data)
    {
    case X_kbUseExtension:
	return ProcXkbUseExtension(client);
    case X_kbSelectEvents:
	return ProcXkbSelectEvents(client);
    case X_kbBell:
	return ProcXkbBell(client);
    case X_kbGetState:
	return ProcXkbGetState(client);
    case X_kbLatchLockState:
	return ProcXkbLatchLockState(client);
    case X_kbGetControls:
	return ProcXkbGetControls(client);
    case X_kbSetControls:
	return ProcXkbSetControls(client);
    case X_kbGetMap:
	return ProcXkbGetMap(client);
    case X_kbSetMap:
	return ProcXkbSetMap(client);
    case X_kbGetCompatMap:
	return ProcXkbGetCompatMap(client);
    case X_kbSetCompatMap:
	return ProcXkbSetCompatMap(client);
    case X_kbGetIndicatorState:
	return ProcXkbGetIndicatorState(client);
    case X_kbGetIndicatorMap:
	return ProcXkbGetIndicatorMap(client);
    case X_kbSetIndicatorMap:
	return ProcXkbSetIndicatorMap(client);
    case X_kbGetNamedIndicator:
	return ProcXkbGetNamedIndicator(client);
    case X_kbSetNamedIndicator:
	return ProcXkbSetNamedIndicator(client);
    case X_kbGetNames:
	return ProcXkbGetNames(client);
    case X_kbSetNames:
	return ProcXkbSetNames(client);
    case X_kbGetGeometry:
	return ProcXkbGetGeometry(client);
    case X_kbSetGeometry:
	return ProcXkbSetGeometry(client);
    case X_kbPerClientFlags:
	return ProcXkbPerClientFlags(client);
    case X_kbListComponents:
	return ProcXkbListComponents(client);
    case X_kbGetKbdByName:
	return ProcXkbGetKbdByName(client);
    case X_kbGetDeviceInfo:
	return ProcXkbGetDeviceInfo(client);
    case X_kbSetDeviceInfo:
	return ProcXkbSetDeviceInfo(client);
    case X_kbSetDebuggingFlags:
	return ProcXkbSetDebuggingFlags(client);
    default:
	return BadRequest;
    }
}

static int
#if NeedFunctionPrototypes
XkbClientGone(pointer data,XID id)
#else
XkbClientGone(data,id)
    pointer data;
    XID id;
#endif
{
    DevicePtr	pXDev = (DevicePtr)data;

    if (!XkbRemoveResourceClient(pXDev,id)) {
	ErrorF("Internal Error! bad RemoveResourceClient in XkbClientGone\n");
    }
    return 1;
}

/*ARGSUSED*/
static void
#if NeedFunctionPrototypes
XkbResetProc(ExtensionEntry *extEntry)
#else
XkbResetProc(extEntry)
    ExtensionEntry *extEntry;
#endif
{
}

void
#if NeedFunctionPrototypes
XkbExtensionInit(void)
#else
XkbExtensionInit()
#endif
{
    ExtensionEntry *extEntry;

    if (extEntry = AddExtension(XkbName, XkbNumberEvents, XkbNumberErrors,
				 ProcXkbDispatch, SProcXkbDispatch,
				 XkbResetProc, StandardMinorOpcode)) {
	XkbReqCode = (unsigned char)extEntry->base;
	XkbEventBase = (unsigned char)extEntry->eventBase;
	XkbErrorBase = (unsigned char)extEntry->errorBase;
	XkbKeyboardErrorCode = XkbErrorBase+XkbKeyboard;
	RT_XKBCLIENT = CreateNewResourceType(XkbClientGone);
    }
    return;
}


