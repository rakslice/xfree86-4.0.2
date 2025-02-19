/* $XConsortium: xkbActions.c /main/3 1996/03/01 14:31:12 kaleb $ */
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
/* $XFree86: xc/programs/Xserver/xkb/xkbActions.c,v 3.3 2000/04/04 19:25:23 dawes Exp $ */

#include <stdio.h>
#include <math.h>
#define NEED_EVENTS 1
#include <X11/X.h>
#include <X11/Xproto.h>
#include <X11/keysym.h>
#include "misc.h"
#include "inputstr.h"
#include "XKBsrv.h"
#include <ctype.h>

#ifdef XINPUT
extern	void	ProcessOtherEvent(
#if NeedFunctionPrototypes
    xEvent *		/* xE */,
    DeviceIntPtr 	/* dev */,
    int 		/* count */
#endif
);
#endif

/***====================================================================***/

static XkbAction
#if NeedFunctionPrototypes
_FixUpAction(XkbDescPtr xkb,XkbAction *act)
#else
_FixUpAction(xkb,act)
    XkbDescPtr		xkb;
    XkbAction *		act;
#endif
{
static XkbAction	fake;

    if (XkbIsPtrAction(act)&&(!(xkb->ctrls->enabled_ctrls&XkbMouseKeysMask))) {
	fake.type = XkbSA_NoAction;
	return fake;
    }
    if (XkbDisableLockActions) {
	switch (act->type) {
	    case XkbSA_LockMods:
		fake.mods.type  = XkbSA_SetMods;
		fake.mods.flags = 0;
		fake.mods.mask  = act->mods.mask;
		return fake;
	    case XkbSA_LatchMods:
		fake.mods.type  = XkbSA_SetMods;
		fake.mods.flags = 0;
		fake.mods.mask  = act->mods.mask;
		return fake;
	    case XkbSA_ISOLock:
		if (act->iso.flags&XkbSA_ISODfltIsGroup) {
		     fake.group.type = XkbSA_SetGroup;
		     fake.group.flags = act->iso.flags&XkbSA_GroupAbsolute;
		     XkbSASetGroup(&fake.group,XkbSAGroup(&act->iso));
		}
		else {
		     fake.mods.type  = XkbSA_SetMods;
		     fake.mods.flags = 0;
		     fake.mods.mask  = act->iso.mask;
		}
		return fake;
	    case XkbSA_LockGroup:
	    case XkbSA_LatchGroup:
		/* We want everything from the latch/lock action except the
		 * type should be changed to set.
		 */
		fake = *act;
		fake.group.type = XkbSA_SetGroup;
		return fake;
	}
    }
    else 
    if (xkb->ctrls->enabled_ctrls&XkbStickyKeysMask) {
	if (act->any.type==XkbSA_SetMods) {
	    fake.mods.type = XkbSA_LatchMods;
	    fake.mods.mask = act->mods.mask;
	    if (XkbAX_NeedOption(xkb->ctrls,XkbAX_LatchToLockMask))
		 fake.mods.flags= XkbSA_ClearLocks|XkbSA_LatchToLock;
	    else fake.mods.flags= XkbSA_ClearLocks;
	    return fake;
	}
	if (act->any.type==XkbSA_SetGroup) {
	    fake.group.type = XkbSA_LatchGroup;
	    if (XkbAX_NeedOption(xkb->ctrls,XkbAX_LatchToLockMask))
		 fake.group.flags= XkbSA_ClearLocks|XkbSA_LatchToLock;
	    else fake.group.flags= XkbSA_ClearLocks;
	    XkbSASetGroup(&fake.group,XkbSAGroup(&act->group));
	    return fake;
	}
    }
    return *act;
}

static XkbAction
#if NeedFunctionPrototypes
XkbGetKeyAction(XkbSrvInfoPtr xkbi,XkbStatePtr xkbState,CARD8 key)
#else
XkbGetKeyAction(xkbi,xkbState,key)
    XkbSrvInfoPtr	xkbi;
    XkbStatePtr		xkbState;
    CARD8 	 	key;
#endif
{
int			effectiveGroup;
int			col;
XkbDescPtr		xkb;
XkbKeyTypePtr		type;
XkbAction *		pActs;
static XkbAction 	fake;

    xkb= xkbi->desc;
    if (!XkbKeyHasActions(xkb,key) || !XkbKeycodeInRange(xkb,key)) {
	fake.type = XkbSA_NoAction;
	return fake;
    }
    pActs= XkbKeyActionsPtr(xkb,key);
    col= 0;
    effectiveGroup= xkbState->group;
    if (effectiveGroup!=XkbGroup1Index) {
	if (XkbKeyNumGroups(xkb,key)>(unsigned)1) {
	    if (effectiveGroup>=XkbKeyNumGroups(xkb,key)) {
		unsigned gi= XkbKeyGroupInfo(xkb,key);
		switch (XkbOutOfRangeGroupAction(gi)) {
		    default:
		    case XkbWrapIntoRange:
			effectiveGroup %= XkbKeyNumGroups(xkb,key);
			break;
		    case XkbClampIntoRange:
			effectiveGroup = XkbKeyNumGroups(xkb,key)-1;
			break;
		    case XkbRedirectIntoRange:
			effectiveGroup= XkbOutOfRangeGroupInfo(gi);
			if (effectiveGroup>=XkbKeyNumGroups(xkb,key))
			    effectiveGroup= 0;
			break;
		}
	    }
	}
	else effectiveGroup= XkbGroup1Index;
	col+= (effectiveGroup*XkbKeyGroupsWidth(xkb,key));
    }
    type= XkbKeyKeyType(xkb,key,effectiveGroup);
    if (type->map!=NULL) {
	register unsigned		i,mods;
	register XkbKTMapEntryPtr	entry;
	mods= xkbState->mods&type->mods.mask;
	for (entry= type->map,i=0;i<type->map_count;i++,entry++) {
	    if ((entry->active)&&(entry->mods.mask==mods)) {
		col+= entry->level;
		break;
	    }
	}
    }
    if (pActs[col].any.type==XkbSA_NoAction)
	return pActs[col];
    fake= _FixUpAction(xkb,&pActs[col]);
    return fake;
}

XkbAction
#if NeedFunctionPrototypes
XkbGetButtonAction(DeviceIntPtr kbd,DeviceIntPtr dev,int button)
#else
XkbGetButtonAction(kbd,dev,button)
    DeviceIntPtr	kbd;
    DeviceIntPtr	dev;
    int			button;
#endif
{
XkbAction fake;
   if ((dev->button)&&(dev->button->xkb_acts)) {
	if (dev->button->xkb_acts[button-1].any.type!=XkbSA_NoAction) {
	    fake= _FixUpAction(kbd->key->xkbInfo->desc,
					&dev->button->xkb_acts[button-1]);
	    return fake;
	}
   }
   fake.any.type= XkbSA_NoAction;
   return fake;
}

/***====================================================================***/

#define	SYNTHETIC_KEYCODE	1
#define	BTN_ACT_FLAG		0x100

typedef struct _XkbFilter {
	CARD16			  keycode;
	CARD8			  what;
	CARD8			  active;
	CARD8			  filterOthers;
	CARD32			  priv;
	XkbAction		  upAction;
	int			(*filter)(
#if NeedFunctionPrototypes
					XkbSrvInfoPtr 		/* xkbi */,
					struct _XkbFilter *	/* filter */,
					unsigned		/* keycode */,
					XkbAction *		/* action */
#endif
				  );
	struct _XkbFilter	 *next;
} XkbFilterRec,*XkbFilterPtr;

static int
#if NeedFunctionPrototypes
_XkbFilterSetState(	XkbSrvInfoPtr	xkbi,
			XkbFilterPtr	filter,
			unsigned	keycode,
			XkbAction *pAction)
#else
_XkbFilterSetState(xkbi,filter,keycode,pAction)
    XkbSrvInfoPtr	xkbi;
    XkbFilterPtr	filter;
    unsigned		keycode;
    XkbAction *		pAction;
#endif
{

    if (filter->keycode==0) {		/* initial press */
	filter->keycode = keycode;
	filter->active = 1;
	filter->filterOthers = ((pAction->mods.mask&XkbSA_ClearLocks)!=0);
	filter->priv = 0;
	filter->filter = _XkbFilterSetState;
	if (pAction->type==XkbSA_SetMods) {
	    filter->upAction = *pAction;
	    xkbi->setMods= pAction->mods.mask;
	}
	else {
	    xkbi->groupChange = XkbSAGroup(&pAction->group);
	    if (pAction->group.flags&XkbSA_GroupAbsolute)
		xkbi->groupChange-= xkbi->state.base_group;
	    filter->upAction= *pAction;
	    XkbSASetGroup(&filter->upAction.group,xkbi->groupChange);
	}
    }
    else if (filter->keycode==keycode) {
	if (filter->upAction.type==XkbSA_SetMods) {
	    xkbi->clearMods = filter->upAction.mods.mask;
	    if (filter->upAction.mods.flags&XkbSA_ClearLocks) {
		xkbi->state.locked_mods&= ~filter->upAction.mods.mask;
	    }
	}
	else {
	    if (filter->upAction.group.flags&XkbSA_ClearLocks) {
		xkbi->state.locked_group = 0;
	    }
	    xkbi->groupChange = -XkbSAGroup(&filter->upAction.group);
	}
	filter->active = 0;
    }
    else {
	filter->upAction.mods.flags&= ~XkbSA_ClearLocks;
	filter->filterOthers = 0;
    }
    return 1;
}

#define	LATCH_KEY_DOWN	1
#define	LATCH_PENDING	2
#define	NO_LATCH	3

static int
#if NeedFunctionPrototypes
_XkbFilterLatchState(	XkbSrvInfoPtr	xkbi,
			XkbFilterPtr	filter,
			unsigned	keycode,
			XkbAction *	pAction)
#else
_XkbFilterLatchState(xkbi,filter,keycode,pAction)
    XkbSrvInfoPtr	xkbi;
    XkbFilterPtr	filter;
    unsigned		keycode;
    XkbAction *		pAction;
#endif
{

    if (filter->keycode==0) {			/* initial press */
	filter->keycode = keycode;
	filter->active = 1;
	filter->filterOthers = 1;
	filter->priv = LATCH_KEY_DOWN;
	filter->filter = _XkbFilterLatchState;
	if (pAction->type==XkbSA_LatchMods) {
	    filter->upAction = *pAction;
	    xkbi->setMods = pAction->mods.mask;
	}
	else {
	    xkbi->groupChange = XkbSAGroup(&pAction->group);
	    if (pAction->group.flags&XkbSA_GroupAbsolute)
		 xkbi->groupChange-= xkbi->state.base_group;
	    filter->upAction= *pAction;
	    XkbSASetGroup(&filter->upAction.group,xkbi->groupChange);
	}
    }
    else if ( pAction && (filter->priv==LATCH_PENDING) ) {
	if (((1<<pAction->type)&XkbSA_BreakLatch)!=0) {
	    filter->active = 0;
	    if (filter->upAction.type==XkbSA_LatchMods)
		 xkbi->state.latched_mods&= ~filter->upAction.mods.mask;
	    else xkbi->state.latched_group-=XkbSAGroup(&filter->upAction.group);
	}
	else if ((pAction->type==filter->upAction.type)&&
		 (pAction->mods.flags==filter->upAction.mods.flags)&&
		 (pAction->mods.mask==filter->upAction.mods.mask)) {
	    if (filter->upAction.mods.flags&XkbSA_LatchToLock) {
		XkbControlsPtr ctrls= xkbi->desc->ctrls;
		if (filter->upAction.type==XkbSA_LatchMods)
		     pAction->mods.type= XkbSA_LockMods;
		else pAction->group.type= XkbSA_LockGroup;
		if (XkbAX_NeedFeedback(ctrls,XkbAX_StickyKeysFBMask)&&
		    		(ctrls->enabled_ctrls&XkbStickyKeysMask)) {
		    XkbDDXAccessXBeep(xkbi->device,_BEEP_STICKY_LOCK,
						XkbStickyKeysMask);
		}
	    }
	    else {
		if (filter->upAction.type==XkbSA_LatchMods)
		     pAction->mods.type= XkbSA_SetMods;
		else pAction->group.type= XkbSA_SetGroup;
	    }
	    if (filter->upAction.type==XkbSA_LatchMods)
		 xkbi->state.latched_mods&= ~filter->upAction.mods.mask;
	    else xkbi->state.latched_group-=XkbSAGroup(&filter->upAction.group);
	    filter->active = 0;
	}
    }
    else if (filter->keycode==keycode) {	/* release */
	XkbControlsPtr	ctrls= xkbi->desc->ctrls;
	int		needBeep;
	int		beepType= _BEEP_NONE;

	needBeep= ((ctrls->enabled_ctrls&XkbStickyKeysMask)&&
			XkbAX_NeedFeedback(ctrls,XkbAX_StickyKeysFBMask));
	if (filter->upAction.type==XkbSA_LatchMods) {
	    xkbi->clearMods = filter->upAction.mods.mask;
	    if ((filter->upAction.mods.flags&XkbSA_ClearLocks)&&
		 (xkbi->clearMods&xkbi->state.locked_mods)==xkbi->clearMods) {
		xkbi->state.locked_mods&= ~xkbi->clearMods;
		filter->priv= NO_LATCH;
		beepType= _BEEP_STICKY_UNLOCK;
	    }
	}
	else {
	    xkbi->groupChange = -XkbSAGroup(&filter->upAction.group);
	    if ((filter->upAction.group.flags&XkbSA_ClearLocks)&&
						(xkbi->state.locked_group)) {
		xkbi->state.locked_group = 0;
		filter->priv = NO_LATCH;
		beepType= _BEEP_STICKY_UNLOCK;
	    }
	}
	if (filter->priv==NO_LATCH) {
	    filter->active= 0;
	}
	else {
	    filter->priv= LATCH_PENDING;
	    if (filter->upAction.type==XkbSA_LatchMods) {
		xkbi->state.latched_mods |= filter->upAction.mods.mask;
		needBeep = xkbi->state.latched_mods ? needBeep : 0;
		xkbi->state.latched_mods |= filter->upAction.mods.mask;
	    }
	    else {
		xkbi->state.latched_group+= XkbSAGroup(&filter->upAction.group);
	    }
	    if (needBeep && (beepType==_BEEP_NONE))
		beepType= _BEEP_STICKY_LATCH;
	}
	if (needBeep && (beepType!=_BEEP_NONE))
	    XkbDDXAccessXBeep(xkbi->device,beepType,XkbStickyKeysMask);
    }
    else if (filter->priv==LATCH_KEY_DOWN) {
	filter->priv= NO_LATCH;
	filter->filterOthers = 0;
    }
    return 1;
}

static int
#if NeedFunctionPrototypes
_XkbFilterLockState(	XkbSrvInfoPtr	xkbi,
			XkbFilterPtr	filter,
			unsigned	keycode,
			XkbAction *	pAction)
#else
_XkbFilterLockState(xkbi,filter,keycode,pAction)
    XkbSrvInfoPtr	xkbi;
    XkbFilterPtr	filter;
    unsigned		keycode;
    XkbAction *		pAction;
#endif
{

    if (pAction&&(pAction->type==XkbSA_LockGroup)) {
	if (pAction->group.flags&XkbSA_GroupAbsolute)
	     xkbi->state.locked_group= XkbSAGroup(&pAction->group);
	else xkbi->state.locked_group+= XkbSAGroup(&pAction->group);
	return 1;
    }
    if (filter->keycode==0) {		/* initial press */
	filter->keycode = keycode;
	filter->active = 1;
	filter->filterOthers = 0;
	filter->priv = 0;
	filter->filter = _XkbFilterLockState;
	filter->upAction = *pAction;
	xkbi->state.locked_mods^= pAction->mods.mask;
	xkbi->setMods = pAction->mods.mask;
    }
    else if (filter->keycode==keycode) {
	filter->active = 0;
	xkbi->clearMods = filter->upAction.mods.mask;
    }
    return 1;
}

#define	ISO_KEY_DOWN		0
#define	NO_ISO_LOCK		1

static int
#if NeedFunctionPrototypes
_XkbFilterISOLock(	XkbSrvInfoPtr	xkbi,
			XkbFilterPtr	filter,
			unsigned	keycode,
			XkbAction *	pAction)
#else
_XkbFilterISOLock(xkbi,filter,keycode,pAction)
    XkbSrvInfoPtr	xkbi;
    XkbFilterPtr	filter;
    unsigned		keycode;
    XkbAction *		pAction;
#endif
{

    if (filter->keycode==0) {		/* initial press */
	CARD8	flags= pAction->iso.flags;

	filter->keycode = keycode;
	filter->active = 1;
	filter->filterOthers = 1;
	filter->priv = ISO_KEY_DOWN;
	filter->upAction = *pAction;
	filter->filter = _XkbFilterISOLock;
	if (flags&XkbSA_ISODfltIsGroup) {
	    xkbi->groupChange = XkbSAGroup(&pAction->iso);
	    xkbi->setMods = 0;
	}
	else {
	    xkbi->setMods = pAction->iso.mask;
	    xkbi->groupChange = 0;
	}
	if ((!(flags&XkbSA_ISONoAffectMods))&&(xkbi->state.base_mods)) {
	    filter->priv= NO_ISO_LOCK;
	    xkbi->state.locked_mods^= xkbi->state.base_mods;
	}
	if ((!(flags&XkbSA_ISONoAffectGroup))&&(xkbi->state.base_group)) {
/* 6/22/93 (ef) -- lock groups if group key is down first */
	}
	if (!(flags&XkbSA_ISONoAffectPtr)) {
/* 6/22/93 (ef) -- lock mouse buttons if they're down */
	}
    }
    else if (filter->keycode==keycode) {
	CARD8	flags= filter->upAction.iso.flags;

	if (flags&XkbSA_ISODfltIsGroup) {
	    xkbi->groupChange = -XkbSAGroup(&filter->upAction.iso);
	    xkbi->clearMods = 0;
	    if (filter->priv==ISO_KEY_DOWN)
		xkbi->state.locked_group+= XkbSAGroup(&filter->upAction.iso);
	}
	else {
	    xkbi->clearMods= filter->upAction.iso.mask;
	    xkbi->groupChange= 0;
	    if (filter->priv==ISO_KEY_DOWN)
		xkbi->state.locked_mods^= filter->upAction.iso.mask;
	}
	filter->active = 0;
    }
    else if (pAction) {
	CARD8	flags= filter->upAction.iso.flags;

	switch (pAction->type) {
	    case XkbSA_SetMods: case XkbSA_LatchMods:
		if (!(flags&XkbSA_ISONoAffectMods)) {
		    pAction->type= XkbSA_LockMods;
		    filter->priv= NO_ISO_LOCK;
		}
		break;
	    case XkbSA_SetGroup: case XkbSA_LatchGroup:
		if (!(flags&XkbSA_ISONoAffectGroup)) {
		    pAction->type= XkbSA_LockGroup;
		    filter->priv= NO_ISO_LOCK;
		}
		break;
	    case XkbSA_PtrBtn:
		if (!(flags&XkbSA_ISONoAffectPtr)) {
		     pAction->type= XkbSA_LockPtrBtn;
		     filter->priv= NO_ISO_LOCK;
		}
		break;
	    case XkbSA_SetControls:
		if (!(flags&XkbSA_ISONoAffectCtrls)) {
		    pAction->type= XkbSA_LockControls;
		    filter->priv= NO_ISO_LOCK;
		}
		break;
	}
    }
    return 1;
}


static CARD32
#if NeedFunctionPrototypes
_XkbPtrAccelExpire(OsTimerPtr timer,CARD32 now,pointer arg)
#else
_XkbPtrAccelExpire(timer,now,arg)
    OsTimerPtr	 timer;
    CARD32	 now;
    pointer	 arg;
#endif
{
XkbSrvInfoPtr	xkbi= (XkbSrvInfoPtr)arg;
XkbControlsPtr	ctrls= xkbi->desc->ctrls;
int		dx,dy;

    if (xkbi->mouseKey==0)
	return 0;

    if (xkbi->mouseKeysAccel) {
	if ((xkbi->mouseKeysCounter)<ctrls->mk_time_to_max) {
	    double step;
	    xkbi->mouseKeysCounter++;
	    step= xkbi->mouseKeysCurveFactor*
		 pow((double)xkbi->mouseKeysCounter,xkbi->mouseKeysCurve);
	    if (xkbi->mouseKeysDX<0)
		 dx= floor( ((double)xkbi->mouseKeysDX)*step );
	    else dx=  ceil( ((double)xkbi->mouseKeysDX)*step );
	    if (xkbi->mouseKeysDY<0)
		 dy= floor( ((double)xkbi->mouseKeysDY)*step );
	    else dy=  ceil( ((double)xkbi->mouseKeysDY)*step );
	}
	else {
	    dx= xkbi->mouseKeysDX*ctrls->mk_max_speed;
	    dy= xkbi->mouseKeysDY*ctrls->mk_max_speed;
	}
	if (xkbi->mouseKeysFlags&XkbSA_MoveAbsoluteX)
	    dx= xkbi->mouseKeysDX;
	if (xkbi->mouseKeysFlags&XkbSA_MoveAbsoluteY)
	    dy= xkbi->mouseKeysDY;
    }
    else {
	dx= xkbi->mouseKeysDX;
	dy= xkbi->mouseKeysDY;
    }
    XkbDDXFakePointerMotion(xkbi->mouseKeysFlags,dx,dy);
    return xkbi->desc->ctrls->mk_interval;
}

static int
#if NeedFunctionPrototypes
_XkbFilterPointerMove(	XkbSrvInfoPtr	xkbi,
			XkbFilterPtr	filter,
			unsigned	keycode,
			XkbAction *	pAction)
#else
_XkbFilterPointerMove(xkbi,filter,keycode,pAction)
    XkbSrvInfoPtr	xkbi;
    XkbFilterPtr	filter;
    unsigned		keycode;
    XkbAction *		pAction;
#endif
{
int	x,y;
Bool	accel;

    if (filter->keycode==0) {		/* initial press */
	filter->keycode = keycode;
	filter->active = 1;
	filter->filterOthers = 0;
	filter->priv=0;
	filter->filter = _XkbFilterPointerMove;
	filter->upAction= *pAction;
	xkbi->mouseKeysCounter= 0;
	xkbi->mouseKey= keycode;
	accel= ((pAction->ptr.flags&XkbSA_NoAcceleration)==0);
	x= XkbPtrActionX(&pAction->ptr);
	y= XkbPtrActionY(&pAction->ptr);
	XkbDDXFakePointerMotion(pAction->ptr.flags,x,y);
	AccessXCancelRepeatKey(xkbi,keycode);
	xkbi->mouseKeysAccel= accel&&
		(xkbi->desc->ctrls->enabled_ctrls&XkbMouseKeysAccelMask);
	if (xkbi->mouseKeysAccel) {
	    xkbi->mouseKeysFlags= pAction->ptr.flags;
	    xkbi->mouseKeysDX= XkbPtrActionX(&pAction->ptr);
	    xkbi->mouseKeysDY= XkbPtrActionY(&pAction->ptr);
	    xkbi->mouseKeyTimer= TimerSet(xkbi->mouseKeyTimer, 0,
				xkbi->desc->ctrls->mk_delay,
				_XkbPtrAccelExpire,(pointer)xkbi);
	}
    }
    else if (filter->keycode==keycode) {
	filter->active = 0;
	if (xkbi->mouseKey==keycode) {
	    xkbi->mouseKey= 0;
	    xkbi->mouseKeyTimer= TimerSet(xkbi->mouseKeyTimer, 0, 0,
							NULL, NULL);
	}
    }
    return 0;
}

static int
#if NeedFunctionPrototypes
_XkbFilterPointerBtn(	XkbSrvInfoPtr	xkbi,
			XkbFilterPtr	filter,
			unsigned	keycode,
			XkbAction *	pAction)
#else
_XkbFilterPointerBtn(xkbi,filter,keycode,pAction)
    XkbSrvInfoPtr	xkbi;
    XkbFilterPtr	filter;
    unsigned		keycode;
    XkbAction *		pAction;
#endif
{
    if (filter->keycode==0) {		/* initial press */
	int	button= pAction->btn.button;

	if (button==XkbSA_UseDfltButton)
	    button = xkbi->desc->ctrls->mk_dflt_btn;

	filter->keycode = keycode;
	filter->active = 1;
	filter->filterOthers = 0;
	filter->priv=0;
	filter->filter = _XkbFilterPointerBtn;
	filter->upAction= *pAction;
	filter->upAction.btn.button= button;
	switch (pAction->type) {
	    case XkbSA_LockPtrBtn:
		if (((xkbi->lockedPtrButtons&(1<<button))==0)&&
			((pAction->btn.flags&XkbSA_LockNoLock)==0)) {
		    xkbi->lockedPtrButtons|= (1<<button);
		    AccessXCancelRepeatKey(xkbi,keycode);
		    XkbDDXFakePointerButton(ButtonPress,button);
		    filter->upAction.type= XkbSA_NoAction;
		}
		break;
	    case XkbSA_PtrBtn:
		{
		    register int i,nClicks;
		    AccessXCancelRepeatKey(xkbi,keycode);
		    if (pAction->btn.count>0) {
			nClicks= pAction->btn.count;
			for (i=0;i<nClicks;i++) {
			    XkbDDXFakePointerButton(ButtonPress,button);
			    XkbDDXFakePointerButton(ButtonRelease,button);
			}
			filter->upAction.type= XkbSA_NoAction;
		    }
		    else XkbDDXFakePointerButton(ButtonPress,button);
		}
		break;
	    case XkbSA_SetPtrDflt:
		{
		    XkbControlsPtr	ctrls= xkbi->desc->ctrls;
		    XkbControlsRec	old;
		    xkbControlsNotify	cn;

		    old= *ctrls;
		    AccessXCancelRepeatKey(xkbi,keycode);
		    switch (pAction->dflt.affect) {
			case XkbSA_AffectDfltBtn:
			    if (pAction->dflt.flags&XkbSA_DfltBtnAbsolute)
				ctrls->mk_dflt_btn= 
					XkbSAPtrDfltValue(&pAction->dflt);
			    else {
				ctrls->mk_dflt_btn+=
					XkbSAPtrDfltValue(&pAction->dflt);
				if (ctrls->mk_dflt_btn>5)
				    ctrls->mk_dflt_btn= 5;
				else if (ctrls->mk_dflt_btn<1)
				    ctrls->mk_dflt_btn= 1;
			    }
			    break;
			default:
			    ErrorF(
		"Attempt to change unknown pointer default (%d) ignored\n",
							pAction->dflt.affect);
			    break;
		    }
		    if (XkbComputeControlsNotify(xkbi->device,
						&old,xkbi->desc->ctrls,
						&cn,False)) {
			cn.keycode = keycode;
			cn.eventType = KeyPress;
			cn.requestMajor = 0;
			cn.requestMinor = 0;
			XkbSendControlsNotify(xkbi->device,&cn);
		    }
		}
		break;
	}
    }
    else if (filter->keycode==keycode) {
	int	button= filter->upAction.btn.button;

	switch (filter->upAction.type) {
	    case XkbSA_LockPtrBtn:
		if (((filter->upAction.btn.flags&XkbSA_LockNoUnlock)!=0)||
				((xkbi->lockedPtrButtons&(1<<button))==0)) {
		    break;
		}
		xkbi->lockedPtrButtons&= ~(1<<button);
	    case XkbSA_PtrBtn:
		XkbDDXFakePointerButton(ButtonRelease,button);
		break;
	}
	filter->active = 0;
    }
    return 0;
}

static int
#if NeedFunctionPrototypes
_XkbFilterControls(	XkbSrvInfoPtr	xkbi,
			XkbFilterPtr	filter,
			unsigned	keycode,
			XkbAction *	pAction)
#else
_XkbFilterControls(xkbi,filter,keycode,pAction)
    XkbSrvInfoPtr	xkbi;
    XkbFilterPtr	filter;
    unsigned		keycode;
    XkbAction *		pAction;
#endif
{
XkbControlsRec		old;
XkbControlsPtr		ctrls;
DeviceIntPtr		kbd;
unsigned int		change;
XkbEventCauseRec	cause;

    kbd= xkbi->device;
    ctrls= xkbi->desc->ctrls;
    old= *ctrls;
    if (filter->keycode==0) {		/* initial press */
	filter->keycode = keycode;
	filter->active = 1;
	filter->filterOthers = 0;
	change= XkbActionCtrls(&pAction->ctrls);
	filter->priv = change;
	filter->filter = _XkbFilterControls;
	filter->upAction = *pAction;

	if (pAction->type==XkbSA_LockControls) {
	    filter->priv= (ctrls->enabled_ctrls&change);
	    change&= ~ctrls->enabled_ctrls;
	}

	if (change) {
	    xkbControlsNotify	cn;
	    XkbSrvLedInfoPtr	sli;

	    ctrls->enabled_ctrls|= change;
	    if (XkbComputeControlsNotify(kbd,&old,ctrls,&cn,False)) {
		cn.keycode = keycode;
		cn.eventType = KeyPress;
		cn.requestMajor = 0;
		cn.requestMinor = 0;
		XkbSendControlsNotify(kbd,&cn);
	    }

	    XkbSetCauseKey(&cause,keycode,KeyPress);

	    /* If sticky keys were disabled, clear all locks and latches */
	    if ((old.enabled_ctrls&XkbStickyKeysMask)&&
		(!(ctrls->enabled_ctrls&XkbStickyKeysMask))) {
		XkbClearAllLatchesAndLocks(kbd,xkbi,False,&cause);
    	    }
	    sli= XkbFindSrvLedInfo(kbd,XkbDfltXIClass,XkbDfltXIId,0);
	    XkbUpdateIndicators(kbd,sli->usesControls,True,NULL,&cause);
	    if (XkbAX_NeedFeedback(ctrls,XkbAX_FeatureFBMask))
		XkbDDXAccessXBeep(kbd,_BEEP_FEATURE_ON,change);
	}
    }
    else if (filter->keycode==keycode) {
	change= filter->priv;
	if (change) {
	    xkbControlsNotify 	cn;
	    XkbSrvLedInfoPtr	sli;

	    ctrls->enabled_ctrls&= ~change;
	    if (XkbComputeControlsNotify(kbd,&old,ctrls,&cn,False)) {
		cn.keycode = keycode;
		cn.eventType = KeyRelease;
		cn.requestMajor = 0;
		cn.requestMinor = 0;
		XkbSendControlsNotify(kbd,&cn);
	    }

	    XkbSetCauseKey(&cause,keycode,KeyRelease);
	    /* If sticky keys were disabled, clear all locks and latches */
	    if ((old.enabled_ctrls&XkbStickyKeysMask)&&
		(!(ctrls->enabled_ctrls&XkbStickyKeysMask))) {
		XkbClearAllLatchesAndLocks(kbd,xkbi,False,&cause);
    	    }
	    sli= XkbFindSrvLedInfo(kbd,XkbDfltXIClass,XkbDfltXIId,0);
	    XkbUpdateIndicators(kbd,sli->usesControls,True,NULL,&cause);
	    if (XkbAX_NeedFeedback(ctrls,XkbAX_FeatureFBMask))
		XkbDDXAccessXBeep(kbd,_BEEP_FEATURE_OFF,change);
	}
	filter->keycode= 0;
	filter->active= 0;
    }
    return 1;
}

static int
#if NeedFunctionPrototypes
_XkbFilterActionMessage(XkbSrvInfoPtr	xkbi,
			XkbFilterPtr	filter,
			unsigned	keycode,
			XkbAction *	pAction)
#else
_XkbFilterActionMessage(xkbi,filter,keycode,pAction)
    XkbSrvInfoPtr	xkbi;
    XkbFilterPtr	filter;
    unsigned		keycode;
    XkbAction *		pAction;
#endif
{
XkbMessageAction *	pMsg;
DeviceIntPtr		kbd;

    kbd= xkbi->device;
    if (filter->keycode==0) {		/* initial press */
	pMsg= &pAction->msg;
	if ((pMsg->flags&XkbSA_MessageOnRelease)||
	    ((pMsg->flags&XkbSA_MessageGenKeyEvent)==0)) {
	    filter->keycode = keycode;
	    filter->active = 1;
	    filter->filterOthers = 0;
	    filter->priv = 0;
	    filter->filter = _XkbFilterActionMessage;
	    filter->upAction = *pAction;
	}
	if (pMsg->flags&XkbSA_MessageOnPress)  {
	    xkbActionMessage	msg;

	    msg.keycode= keycode;
	    msg.press= 1;
	    msg.keyEventFollows=((pMsg->flags&XkbSA_MessageGenKeyEvent)!=0);
	    memcpy((char *)msg.message,
				(char *)pMsg->message,XkbActionMessageLength);
	    XkbSendActionMessage(kbd,&msg);
	}
	return ((pAction->msg.flags&XkbSA_MessageGenKeyEvent)!=0);
    }
    else if (filter->keycode==keycode) {
	pMsg= &filter->upAction.msg;
	if (pMsg->flags&XkbSA_MessageOnRelease) {
	    xkbActionMessage	msg;

	    msg.keycode= keycode;
	    msg.press= 0;
	    msg.keyEventFollows=((pMsg->flags&XkbSA_MessageGenKeyEvent)!=0);
	    memcpy((char *)msg.message,(char *)pMsg->message,
						XkbActionMessageLength);
	    XkbSendActionMessage(kbd,&msg);
	}
	filter->keycode= 0;
	filter->active= 0;
	return ((pMsg->flags&XkbSA_MessageGenKeyEvent)!=0);
    }
    return 0;
}

static int
#if NeedFunctionPrototypes
_XkbFilterRedirectKey(	XkbSrvInfoPtr	xkbi,
			XkbFilterPtr	filter,
			unsigned	keycode,
			XkbAction *	pAction)
#else
_XkbFilterRedirectKey(xkbi,filter,keycode,pAction)
    XkbSrvInfoPtr	xkbi;
    XkbFilterPtr	filter;
    unsigned		keycode;
    XkbAction *		pAction;
#endif
{
unsigned	realMods;
xEvent 		ev;
int		x,y,kc;
XkbStateRec	old;
unsigned	mods,mask,oldCoreState,oldCorePrevState;

    if ((filter->keycode!=0)&&(filter->keycode!=keycode))
	return 1;

    GetSpritePosition(&x,&y);
    ev.u.keyButtonPointer.time = GetTimeInMillis();
    ev.u.keyButtonPointer.rootX = x;
    ev.u.keyButtonPointer.rootY = y;

    mask= XkbSARedirectVModsMask(&pAction->redirect);
    mods= XkbSARedirectVMods(&pAction->redirect);
    if (mask)	XkbVirtualModsToReal(xkbi->desc,mask,&mask);
    if (mods)	XkbVirtualModsToReal(xkbi->desc,mods,&mods);
    mask|= pAction->redirect.mods_mask;
    mods|= pAction->redirect.mods;

    if (filter->keycode==0) {		/* initial press */
	if ((pAction->redirect.new_key<xkbi->desc->min_key_code)||
	    (pAction->redirect.new_key>xkbi->desc->max_key_code)) {
	    return 1;
	}
	filter->keycode = keycode;
	filter->active = 1;
	filter->filterOthers = 0;
	filter->priv = 0;
	filter->filter = _XkbFilterRedirectKey;
	filter->upAction = *pAction;

	ev.u.u.type = KeyPress;
	ev.u.u.detail = pAction->redirect.new_key;

	if ( mask || mods ) {
	    old= xkbi->state;
	    oldCoreState= xkbi->device->key->state;
	    oldCorePrevState= xkbi->device->key->prev_state;
	    xkbi->state.base_mods&= ~mask;
	    xkbi->state.base_mods|= (mods&mask);
	    xkbi->state.latched_mods&= ~mask;
	    xkbi->state.latched_mods|= (mods&mask);
	    xkbi->state.locked_mods&= ~mask;
	    xkbi->state.locked_mods|= (mods&mask);
	    XkbComputeDerivedState(xkbi);
	    xkbi->device->key->state= xkbi->device->key->prev_state= 
							xkbi->state.mods;
	}

	realMods = xkbi->device->key->modifierMap[ev.u.u.detail];
	xkbi->device->key->modifierMap[ev.u.u.detail] = 0;
	CoreProcessKeyboardEvent(&ev,xkbi->device,1);
	xkbi->device->key->modifierMap[ev.u.u.detail] = realMods;
	
	if ( mask || mods ) {
	    xkbi->device->key->state= oldCoreState;
	    xkbi->device->key->prev_state= oldCorePrevState;
	    xkbi->state= old;
	}
    }
    else if (filter->keycode==keycode) {

	ev.u.u.type = KeyRelease;
	ev.u.u.detail = filter->upAction.redirect.new_key;

	if ( mask || mods ) {
	    old= xkbi->state;
	    oldCoreState= xkbi->device->key->state;
	    oldCorePrevState= xkbi->device->key->prev_state;
	    xkbi->state.base_mods&= ~mask;
	    xkbi->state.base_mods|= (mods&mask);
	    xkbi->state.latched_mods&= ~mask;
	    xkbi->state.latched_mods|= (mods&mask);
	    xkbi->state.locked_mods&= ~mask;
	    xkbi->state.locked_mods|= (mods&mask);
	    XkbComputeDerivedState(xkbi);
	    xkbi->device->key->state= xkbi->device->key->prev_state= 
							xkbi->state.mods;
	}

	realMods = xkbi->device->key->modifierMap[ev.u.u.detail];
	xkbi->device->key->modifierMap[ev.u.u.detail] = 0;
	CoreProcessKeyboardEvent(&ev,xkbi->device,1);
	xkbi->device->key->modifierMap[ev.u.u.detail] = realMods;

	if ( mask || mods ) {
	    xkbi->device->key->state= oldCoreState;
	    xkbi->device->key->prev_state= oldCorePrevState;
	    xkbi->state= old;
	}

	filter->keycode= 0;
	filter->active= 0;
    }
    return 1;
}

#ifdef XINPUT

static int
#if NeedFunctionPrototypes
_XkbFilterDeviceBtn(	XkbSrvInfoPtr	xkbi,
			XkbFilterPtr	filter,
			unsigned	keycode,
			XkbAction *	pAction)
#else
_XkbFilterDeviceBtn(xkbi,filter,keycode,pAction)
    XkbSrvInfoPtr	xkbi;
    XkbFilterPtr	filter;
    unsigned		keycode;
    XkbAction *		pAction;
#endif
{
DeviceIntPtr	dev;
int		button;

    if (filter->keycode==0) {		/* initial press */
	dev= _XkbLookupButtonDevice(pAction->devbtn.device,NULL);
	if ((!dev)||(!dev->public.on)||(&dev->public==LookupPointerDevice()))
	    return 1;

	button= pAction->devbtn.button;
	if ((button<1)||(button>dev->button->numButtons))
	    return 1;

	filter->keycode = keycode;
	filter->active = 1;
	filter->filterOthers = 0;
	filter->priv=0;
	filter->filter = _XkbFilterDeviceBtn;
	filter->upAction= *pAction;
	switch (pAction->type) {
	    case XkbSA_LockDeviceBtn:
		if ((pAction->devbtn.flags&XkbSA_LockNoLock)||
		    (dev->button->down[button/8]&(1L<<(button%8))))
		    return 0;
		XkbDDXFakeDeviceButton(dev,True,button);
		filter->upAction.type= XkbSA_NoAction;
		break;
	    case XkbSA_DeviceBtn:
		if (pAction->devbtn.count>0) {
		    int nClicks,i;
		    nClicks= pAction->btn.count;
		    for (i=0;i<nClicks;i++) {
			XkbDDXFakeDeviceButton(dev,True,button);
			XkbDDXFakeDeviceButton(dev,False,button);
		    }
		    filter->upAction.type= XkbSA_NoAction;
		}
		else XkbDDXFakeDeviceButton(dev,True,button);
		break;
	}
    }
    else if (filter->keycode==keycode) {
	int	button;

	filter->active= 0;
	dev= _XkbLookupButtonDevice(filter->upAction.devbtn.device,NULL);
	if ((!dev)||(!dev->public.on)||(&dev->public==LookupPointerDevice()))
	    return 1;

	button= filter->upAction.btn.button;
	switch (filter->upAction.type) {
	    case XkbSA_LockDeviceBtn:
		if ((filter->upAction.devbtn.flags&XkbSA_LockNoUnlock)||
		    ((dev->button->down[button/8]&(1L<<(button%8)))==0))
		    return 0;
		XkbDDXFakeDeviceButton(dev,False,button);
		break;
	    case XkbSA_DeviceBtn:
		XkbDDXFakeDeviceButton(dev,False,button);
		break;
	}
	filter->active = 0;
    }
    return 0;
}
#endif

static	int		szFilters = 0;
static	XkbFilterPtr	filters = NULL;

static XkbFilterPtr
_XkbNextFreeFilter(
#if NeedFunctionPrototypes
	void
#endif
)
{
register int	i;

    if (szFilters==0) {
	szFilters = 4;
	filters = _XkbTypedCalloc(szFilters,XkbFilterRec);
	/* 6/21/93 (ef) -- XXX! deal with allocation failure */
    }
    for (i=0;i<szFilters;i++) {
	if (!filters[i].active) {
	    filters[i].keycode = 0;
	    return &filters[i];
	}
    }
    szFilters*=2;
    filters= _XkbTypedRealloc(filters,szFilters,XkbFilterRec);
    /* 6/21/93 (ef) -- XXX! deal with allocation failure */
    bzero(&filters[szFilters/2],(szFilters/2)*sizeof(XkbFilterRec));
    return &filters[szFilters/2];
}

static int
#if NeedFunctionPrototypes
_XkbApplyFilters(XkbSrvInfoPtr xkbi,unsigned kc,XkbAction *pAction)
#else
_XkbApplyFilters(xkbi,kc,pAction)
    XkbSrvInfoPtr	xkbi;
    unsigned		kc;
    XkbAction *		pAction;
#endif
{
register int	i,send;

    send= 1;
    for (i=0;i<szFilters;i++) {
	if ((filters[i].active)&&(filters[i].filter))
	    send= ((*filters[i].filter)(xkbi,&filters[i],kc,pAction)&&send);
    }
    return send;
}

void
#if NeedFunctionPrototypes
XkbHandleActions(DeviceIntPtr dev,DeviceIntPtr kbd,xEvent *xE,int count)
#else
XkbHandleActions(dev,kbd,xE,count)
    DeviceIntPtr  	dev;
    DeviceIntPtr  	kbd;
    xEvent *		xE;
    int		  	count;
#endif
{
int		key,bit,i;
CARD8		realMods;
XkbSrvInfoPtr	xkbi;
KeyClassPtr	keyc;
int		changed,sendEvent;
Bool		genStateNotify;
XkbStateRec	oldState;
XkbAction	act;
XkbFilterPtr	filter;
Bool		keyEvent;
Bool		pressEvent;
#ifdef XINPUT
Bool		xiEvent;
#endif

    keyc= kbd->key;
    xkbi= keyc->xkbInfo;
    key= xE->u.u.detail;
    if ((xkbi->flags&_XkbStateNotifyInProgress)==0) {
	oldState= xkbi->state;
	xkbi->flags|= _XkbStateNotifyInProgress;
	genStateNotify= True;
    }
    else genStateNotify= False;

    xkbi->clearMods = xkbi->setMods = 0;
    xkbi->groupChange = 0;

    sendEvent = 1;
#ifdef XINPUT
    keyEvent= ((xE->u.u.type==KeyPress)||(xE->u.u.type==DeviceKeyPress)||
		(xE->u.u.type==KeyRelease)||(xE->u.u.type==DeviceKeyRelease));
    pressEvent= (xE->u.u.type==KeyPress)||(xE->u.u.type==DeviceKeyPress)||
		 (xE->u.u.type==ButtonPress)||(xE->u.u.type==DeviceButtonPress);
    xiEvent= (xE->u.u.type==DeviceKeyPress)||(xE->u.u.type==DeviceKeyRelease)||
	     (xE->u.u.type==DeviceButtonPress)||
	     (xE->u.u.type==DeviceButtonRelease);
#else
    keyEvent= (xE->u.u.type==KeyPress)||(xE->u.u.type==KeyRelease);
    pressEvent= (xE->u.u.type==KeyPress)||(xE->u.u.type==ButtonPress);
#endif

    if (pressEvent) {
	if (keyEvent)	
	    act = XkbGetKeyAction(xkbi,&xkbi->state,key);
	else {
	    act = XkbGetButtonAction(kbd,dev,key);
	    key|= BTN_ACT_FLAG;
	}
	sendEvent = _XkbApplyFilters(xkbi,key,&act);
	if (sendEvent) {
	    switch (act.type) {
		case XkbSA_SetMods:
		case XkbSA_SetGroup:
		    filter = _XkbNextFreeFilter();
		    sendEvent = _XkbFilterSetState(xkbi,filter,key,&act);
		    break;
		case XkbSA_LatchMods:
		case XkbSA_LatchGroup:
		    filter = _XkbNextFreeFilter();
		    sendEvent=_XkbFilterLatchState(xkbi,filter,key,&act);
		    break;
		case XkbSA_LockMods:
		case XkbSA_LockGroup:
		    filter = _XkbNextFreeFilter();
		    sendEvent=_XkbFilterLockState(xkbi,filter,key,&act);
		    break;
		case XkbSA_ISOLock:
		    filter = _XkbNextFreeFilter();
		    sendEvent=_XkbFilterISOLock(xkbi,filter,key,&act);
		    break;
		case XkbSA_MovePtr:
		    filter = _XkbNextFreeFilter();
		    sendEvent= _XkbFilterPointerMove(xkbi,filter,key,&act);
		    break;
		case XkbSA_PtrBtn:
		case XkbSA_LockPtrBtn:
		case XkbSA_SetPtrDflt:
		    filter = _XkbNextFreeFilter();
		    sendEvent= _XkbFilterPointerBtn(xkbi,filter,key,&act);
		    break;
		case XkbSA_Terminate:
		    sendEvent= XkbDDXTerminateServer(dev,key,&act);
		    break;
		case XkbSA_SwitchScreen:
		    sendEvent= XkbDDXSwitchScreen(dev,key,&act);
		    break;
		case XkbSA_SetControls:
		case XkbSA_LockControls:
		    filter = _XkbNextFreeFilter();
		    sendEvent=_XkbFilterControls(xkbi,filter,key,&act);
		    break;
		case XkbSA_ActionMessage:
		    filter = _XkbNextFreeFilter();
		    sendEvent=_XkbFilterActionMessage(xkbi,filter,key,&act);
		    break;
		case XkbSA_RedirectKey:
		    filter = _XkbNextFreeFilter();
		    sendEvent= _XkbFilterRedirectKey(xkbi,filter,key,&act);
		    break;
#ifdef XINPUT
		case XkbSA_DeviceBtn:
		case XkbSA_LockDeviceBtn:
		    filter = _XkbNextFreeFilter();
		    sendEvent= _XkbFilterDeviceBtn(xkbi,filter,key,&act);
		    break;
#endif
	    }
	}
    }
    else {
	if (!keyEvent)
	    key|= BTN_ACT_FLAG;
	sendEvent = _XkbApplyFilters(xkbi,key,NULL);
    }

    if (xkbi->groupChange!=0)
	xkbi->state.base_group+= xkbi->groupChange;
    if (xkbi->setMods) {
	for (i=0,bit=1; xkbi->setMods; i++,bit<<=1 ) {
	    if (xkbi->setMods&bit) {
		keyc->modifierKeyCount[i]++;
		xkbi->state.base_mods|= bit;
		xkbi->setMods&= ~bit;
	    }
	}
    }
    if (xkbi->clearMods) {
	for (i=0,bit=1; xkbi->clearMods; i++,bit<<=1 ) {
	    if (xkbi->clearMods&bit) {
		keyc->modifierKeyCount[i]--;
		if (keyc->modifierKeyCount[i]<=0) {
		    xkbi->state.base_mods&= ~bit;
		    keyc->modifierKeyCount[i] = 0;
		}
		xkbi->clearMods&= ~bit;
	    }
	}
    }

    if (sendEvent) {
#ifdef XINPUT
	if (xiEvent)
	    ProcessOtherEvent(xE,dev,count);
	else 
#endif
	if (keyEvent) {
	    realMods = keyc->modifierMap[key];
	    keyc->modifierMap[key] = 0;
	    CoreProcessKeyboardEvent(xE,dev,count);
	    keyc->modifierMap[key] = realMods;
	}
	else CoreProcessPointerEvent(xE,dev,count);
    }
    xkbi->prev_state= oldState;
    XkbComputeDerivedState(xkbi);
    keyc->prev_state= keyc->state;
    keyc->state= XkbStateFieldFromRec(&xkbi->state);
    changed = XkbStateChangedFlags(&oldState,&xkbi->state);
    if (genStateNotify) {
	if (changed) {
	    xkbStateNotify	sn;
	    sn.keycode= key;
	    sn.eventType= xE->u.u.type;
	    sn.requestMajor = sn.requestMinor = 0;
	    sn.changed= changed;
	    XkbSendStateNotify(dev,&sn);
	}
	xkbi->flags&= ~_XkbStateNotifyInProgress;
    }
    changed= XkbIndicatorsToUpdate(dev,changed,False);
    if (changed) {
	XkbEventCauseRec	cause;
	XkbSetCauseKey(&cause,key,xE->u.u.type);
	XkbUpdateIndicators(dev,changed,True,NULL,&cause);
    }
    return;
}

int
#if NeedFunctionPrototypes
XkbLatchModifiers(DeviceIntPtr pXDev,CARD8 mask,CARD8 latches)
#else
XkbLatchModifiers(pXDev,mask,latches)
    DeviceIntPtr 	pXDev;
    CARD8 		mask;
    CARD8 		latches;
#endif
{
XkbSrvInfoPtr	xkbi;
XkbFilterPtr	filter;
XkbAction	act;
unsigned	clear;

    if ( pXDev && pXDev->key && pXDev->key->xkbInfo ) {
	xkbi = pXDev->key->xkbInfo;
	clear= (mask&(~latches));
	xkbi->state.latched_mods&= ~clear;
	/* Clear any pending latch to locks.
	 */
	act.type = XkbSA_NoAction;
	_XkbApplyFilters(xkbi,SYNTHETIC_KEYCODE,&act);
	act.type = XkbSA_LatchMods;
	act.mods.flags = 0;
	act.mods.mask  = mask&latches;
	filter = _XkbNextFreeFilter();
	_XkbFilterLatchState(xkbi,filter,SYNTHETIC_KEYCODE,&act);
	_XkbFilterLatchState(xkbi,filter,SYNTHETIC_KEYCODE,(XkbAction *)NULL);
	return Success;
    }
    return BadValue;
}

int
#if NeedFunctionPrototypes
XkbLatchGroup(DeviceIntPtr pXDev,int group)
#else
XkbLatchGroup(pXDev,group)
    DeviceIntPtr  pXDev;
    int		  group;
#endif
{
XkbSrvInfoPtr	xkbi;
XkbFilterPtr	filter;
XkbAction	act;

    if ( pXDev && pXDev->key && pXDev->key->xkbInfo ) {
	xkbi = pXDev->key->xkbInfo;
	act.type = XkbSA_LatchGroup;
	act.group.flags = 0;
	XkbSASetGroup(&act.group,group);
	filter = _XkbNextFreeFilter();
	_XkbFilterLatchState(xkbi,filter,SYNTHETIC_KEYCODE,&act);
	_XkbFilterLatchState(xkbi,filter,SYNTHETIC_KEYCODE,(XkbAction *)NULL);
	return Success;
    }
    return BadValue;
}

/***====================================================================***/

void
#if NeedFunctionPrototypes
XkbClearAllLatchesAndLocks(	DeviceIntPtr		dev,
				XkbSrvInfoPtr		xkbi,
				Bool			genEv,
				XkbEventCausePtr	cause)
#else
XkbClearAllLatchesAndLocks(dev,xkbi,genEv,cause)
    DeviceIntPtr	dev;
    XkbSrvInfoPtr	xkbi;
    Bool		genEv;
    XkbEventCausePtr	cause;
#endif
{
XkbStateRec	os;
xkbStateNotify	sn;

    sn.changed= 0;
    os= xkbi->state;
    if (os.latched_mods) { /* clear all latches */
	XkbLatchModifiers(dev,~0,0);
	sn.changed|= XkbModifierLatchMask;
    }
    if (os.latched_group) {
	XkbLatchGroup(dev,0);
	sn.changed|= XkbGroupLatchMask;
    }
    if (os.locked_mods) {
	xkbi->state.locked_mods= 0;
	sn.changed|= XkbModifierLockMask;
    }
    if (os.locked_group) {
	xkbi->state.locked_group= 0;
	sn.changed|= XkbGroupLockMask;
    }
    if ( genEv && sn.changed) {
	CARD32 	changed;

	XkbComputeDerivedState(xkbi);
	sn.keycode= 		cause->kc;
	sn.eventType=		cause->event;
	sn.requestMajor= 	cause->mjr;
	sn.requestMinor= 	cause->mnr;
	sn.changed= XkbStateChangedFlags(&os,&xkbi->state);
	XkbSendStateNotify(dev,&sn);
	changed= XkbIndicatorsToUpdate(dev,sn.changed,False);
	if (changed) {
	    XkbUpdateIndicators(dev,changed,True,NULL,cause);
	}
    }
    return;
}

