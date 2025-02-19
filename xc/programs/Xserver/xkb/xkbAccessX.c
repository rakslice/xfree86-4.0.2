/* $XConsortium: xkbAccessX.c /main/13 1996/12/02 10:24:00 lehors $ */
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
/* $XFree86: xc/programs/Xserver/xkb/xkbAccessX.c,v 1.5 2000/08/26 15:11:27 dawes Exp $ */

#include <stdio.h>
#include <math.h>
#ifdef __QNX__
#include <limits.h>
#endif
#define NEED_EVENTS 1
#include <X11/X.h>
#include <X11/Xproto.h>
#include <X11/keysym.h>
#include "inputstr.h"
#include "XKBsrv.h"
#if !defined(WIN32) && !defined(Lynx)
#include <sys/time.h>
#endif

int	XkbDfltRepeatDelay=	660;
int	XkbDfltRepeatInterval=	40;
pointer	XkbLastRepeatEvent=	NULL;

#define	DFLT_TIMEOUT_CTRLS (XkbAX_KRGMask|XkbStickyKeysMask|XkbMouseKeysMask)
#define	DFLT_TIMEOUT_OPTS  (XkbAX_IndicatorFBMask)

unsigned short	XkbDfltAccessXTimeout= 	120;
unsigned int	XkbDfltAccessXTimeoutMask= DFLT_TIMEOUT_CTRLS;
unsigned int	XkbDfltAccessXTimeoutValues= 0;
unsigned int	XkbDfltAccessXTimeoutOptionsMask= DFLT_TIMEOUT_OPTS;
unsigned int	XkbDfltAccessXTimeoutOptionsValues= 0;
unsigned int	XkbDfltAccessXFeedback= XkbAccessXFeedbackMask;
unsigned short	XkbDfltAccessXOptions=  XkbAX_AllOptionsMask & ~(XkbAX_IndicatorFBMask|XkbAX_SKReleaseFBMask|XkbAX_SKRejectFBMask);

void
#if NeedFunctionPrototypes
AccessXComputeCurveFactor(XkbSrvInfoPtr xkbi,XkbControlsPtr ctrls)
#else
AccessXComputeCurveFactor(xkbi,ctrls)
    XkbSrvInfoPtr	xkbi;
    XkbControlsPtr	ctrls;
#endif
{
    xkbi->mouseKeysCurve= 1.0+(((double)ctrls->mk_curve)*0.001);
    xkbi->mouseKeysCurveFactor= ( ((double)ctrls->mk_max_speed)/
	pow((double)ctrls->mk_time_to_max,xkbi->mouseKeysCurve));
    return;
}

void
#if NeedFunctionPrototypes
AccessXInit(DeviceIntPtr keybd)
#else
AccessXInit(keybd)
    DeviceIntPtr keybd;
#endif
{
XkbSrvInfoPtr	xkbi = keybd->key->xkbInfo;
XkbControlsPtr	ctrls = xkbi->desc->ctrls;

    xkbi->shiftKeyCount= 0;
    xkbi->mouseKeysCounter= 0;
    xkbi->inactiveKey= 0;
    xkbi->slowKey= 0;
    xkbi->repeatKey= 0;
    xkbi->krgTimerActive= _OFF_TIMER;
    xkbi->beepType= _BEEP_NONE;
    xkbi->beepCount= 0;
    xkbi->mouseKeyTimer= NULL;
    xkbi->slowKeysTimer= NULL;
    xkbi->bounceKeysTimer= NULL;
    xkbi->repeatKeyTimer= NULL;
    xkbi->krgTimer= NULL;
    xkbi->beepTimer= NULL;
    ctrls->repeat_delay = XkbDfltRepeatDelay;
    ctrls->repeat_interval = XkbDfltRepeatInterval;
    ctrls->debounce_delay = 300;
    ctrls->slow_keys_delay = 300;
    ctrls->mk_delay = 160;
    ctrls->mk_interval = 40;
    ctrls->mk_time_to_max = 30;
    ctrls->mk_max_speed = 30;
    ctrls->mk_curve = 500;
    ctrls->mk_dflt_btn = 1;
    ctrls->ax_timeout = XkbDfltAccessXTimeout;
    ctrls->axt_ctrls_mask = XkbDfltAccessXTimeoutMask;
    ctrls->axt_ctrls_values = XkbDfltAccessXTimeoutValues;
    ctrls->axt_opts_mask = XkbDfltAccessXTimeoutOptionsMask;
    ctrls->axt_opts_values = XkbDfltAccessXTimeoutOptionsValues;
    if (XkbDfltAccessXTimeout)
	ctrls->enabled_ctrls |= XkbAccessXTimeoutMask;
    else
	ctrls->enabled_ctrls &= ~XkbAccessXTimeoutMask;
    ctrls->enabled_ctrls |= XkbDfltAccessXFeedback;
    ctrls->ax_options = XkbDfltAccessXOptions; 
    AccessXComputeCurveFactor(xkbi,ctrls);
    return;
}

/************************************************************************/
/*									*/
/* AccessXKeyboardEvent							*/
/*									*/
/*	Generate a synthetic keyboard event.				*/
/*									*/
/************************************************************************/
static void 
#if NeedFunctionPrototypes
AccessXKeyboardEvent(DeviceIntPtr	keybd,
				 BYTE		type,
				 BYTE		keyCode,
				 Bool		isRepeat)
#else
AccessXKeyboardEvent(keybd,type,keyCode,isRepeat)
    DeviceIntPtr keybd;
    BYTE	 type;
    BYTE	 keyCode;
    Bool	 isRepeat;
#endif
{
xEvent		xE;
#ifdef XINPUT
extern int	DeviceKeyPress;
#endif
    
    xE.u.u.type = type;
    xE.u.u.detail = keyCode;
    xE.u.keyButtonPointer.time = GetTimeInMillis();	    
#ifdef DEBUG
    if (xkbDebugFlags&0x8) {
	ErrorF("AXKE: Key %d %s\n",keyCode,(xE.u.u.type==KeyPress?"down":"up"));
    }
#endif

    if (_XkbIsPressEvent(type))
	XkbDDXKeyClick(keybd,keyCode,TRUE);
    else if (isRepeat)
	XkbLastRepeatEvent=	(pointer)&xE;
    XkbProcessKeyboardEvent(&xE,keybd,1L);
    XkbLastRepeatEvent= NULL;
    return;
    
} /* AccessXKeyboardEvent */

/************************************************************************/
/*									*/
/* AccessXKRGTurnOn							*/
/*									*/
/*	Turn the keyboard response group on.				*/
/*									*/
/************************************************************************/
static void
#if NeedFunctionPrototypes
AccessXKRGTurnOn(DeviceIntPtr dev,CARD16 KRGControl,xkbControlsNotify	*pCN)
#else
AccessXKRGTurnOn(dev,KRGControl,pCN)
    DeviceIntPtr	dev;
    CARD16 		KRGControl;
    xkbControlsNotify	*pCN;
#endif
{
XkbSrvInfoPtr		xkbi = dev->key->xkbInfo;
XkbControlsPtr		ctrls = xkbi->desc->ctrls;
XkbControlsRec		old;
XkbEventCauseRec	cause;
XkbSrvLedInfoPtr	sli;

    old= *ctrls;
    ctrls->enabled_ctrls |= (KRGControl&XkbAX_KRGMask);
    if (XkbComputeControlsNotify(dev,&old,ctrls,pCN,False))
	XkbSendControlsNotify(dev,pCN);
    cause.kc=		pCN->keycode;
    cause.event=	pCN->eventType;
    cause.mjr=		pCN->requestMajor;
    cause.mnr=		pCN->requestMinor;
    sli= XkbFindSrvLedInfo(dev,XkbDfltXIClass,XkbDfltXIId,0);
    XkbUpdateIndicators(dev,sli->usesControls,True,NULL,&cause);
    if (XkbAX_NeedFeedback(ctrls,XkbAX_FeatureFBMask))
	XkbDDXAccessXBeep(dev,_BEEP_FEATURE_ON,KRGControl);
    return;
    
} /* AccessXKRGTurnOn */

/************************************************************************/
/*									*/
/* AccessXKRGTurnOff							*/
/*									*/
/*	Turn the keyboard response group off.				*/
/*									*/
/************************************************************************/
static void 
#if NeedFunctionPrototypes
AccessXKRGTurnOff(DeviceIntPtr dev,xkbControlsNotify *pCN)
#else
AccessXKRGTurnOff(dev,pCN)
    DeviceIntPtr dev;
    xkbControlsNotify *pCN;
#endif
{
XkbSrvInfoPtr		xkbi = dev->key->xkbInfo;
XkbControlsPtr		ctrls = xkbi->desc->ctrls;
XkbControlsRec		old;
XkbEventCauseRec	cause;
XkbSrvLedInfoPtr	sli;

    old = *ctrls;
    ctrls->enabled_ctrls &= ~XkbAX_KRGMask;
    if (XkbComputeControlsNotify(dev,&old,ctrls,pCN,False))
	XkbSendControlsNotify(dev,pCN);
    cause.kc=		pCN->keycode;
    cause.event=	pCN->eventType;
    cause.mjr=		pCN->requestMajor;
    cause.mnr=		pCN->requestMinor;
    sli= XkbFindSrvLedInfo(dev,XkbDfltXIClass,XkbDfltXIId,0);
    XkbUpdateIndicators(dev,sli->usesControls,True,NULL,&cause);
    if (XkbAX_NeedFeedback(ctrls,XkbAX_FeatureFBMask)) {
	unsigned changes= old.enabled_ctrls^ctrls->enabled_ctrls;
	XkbDDXAccessXBeep(dev,_BEEP_FEATURE_OFF,changes);
    }
    return;
    
} /* AccessXKRGTurnOff */

/************************************************************************/
/*									*/
/* AccessXStickyKeysTurnOn						*/
/*									*/
/*	Turn StickyKeys on.						*/
/*									*/
/************************************************************************/
static void
#if NeedFunctionPrototypes
AccessXStickyKeysTurnOn(DeviceIntPtr dev,xkbControlsNotify *pCN)
#else
AccessXStickyKeysTurnOn(dev,pCN)
    DeviceIntPtr	 dev;
    xkbControlsNotify	*pCN;
#endif
{
XkbSrvInfoPtr		xkbi = dev->key->xkbInfo;
XkbControlsPtr		ctrls = xkbi->desc->ctrls;
XkbControlsRec		old;
XkbEventCauseRec	cause;
XkbSrvLedInfoPtr	sli;

    old = *ctrls;
    ctrls->enabled_ctrls |= XkbStickyKeysMask;
    xkbi->shiftKeyCount = 0;
    if (XkbComputeControlsNotify(dev,&old,ctrls,pCN,False)) 
	XkbSendControlsNotify(dev,pCN);
    cause.kc=		pCN->keycode;
    cause.event=	pCN->eventType;
    cause.mjr=		pCN->requestMajor;
    cause.mnr=		pCN->requestMinor;
    sli= XkbFindSrvLedInfo(dev,XkbDfltXIClass,XkbDfltXIId,0);
    XkbUpdateIndicators(dev,sli->usesControls,True,NULL,&cause);
    if (XkbAX_NeedFeedback(ctrls,XkbAX_FeatureFBMask)) {
	XkbDDXAccessXBeep(dev,_BEEP_FEATURE_ON,XkbStickyKeysMask);
    }
    return;
    
} /* AccessXStickyKeysTurnOn */

/************************************************************************/
/*									*/
/* AccessXStickyKeysTurnOff						*/
/*									*/
/*	Turn StickyKeys off.						*/
/*									*/
/************************************************************************/
static void
#if NeedFunctionPrototypes
AccessXStickyKeysTurnOff(DeviceIntPtr dev,xkbControlsNotify *pCN)
#else
AccessXStickyKeysTurnOff(dev,pCN)
    DeviceIntPtr	 dev;
    xkbControlsNotify	*pCN;
#endif
{
XkbSrvInfoPtr		xkbi = dev->key->xkbInfo;
XkbControlsPtr		ctrls = xkbi->desc->ctrls;
XkbControlsRec		old;
XkbEventCauseRec	cause;
XkbSrvLedInfoPtr	sli;

    old = *ctrls;
    ctrls->enabled_ctrls &= ~XkbStickyKeysMask;
    xkbi->shiftKeyCount = 0;
    if (XkbComputeControlsNotify(dev,&old,ctrls,pCN,False))
	XkbSendControlsNotify(dev,pCN);

    cause.kc=		pCN->keycode;
    cause.event=	pCN->eventType;
    cause.mjr=		pCN->requestMajor;
    cause.mnr=		pCN->requestMinor;
    sli= XkbFindSrvLedInfo(dev,XkbDfltXIClass,XkbDfltXIId,0);
    XkbUpdateIndicators(dev,sli->usesControls,True,NULL,&cause);
    if (XkbAX_NeedFeedback(ctrls,XkbAX_FeatureFBMask)) {
	XkbDDXAccessXBeep(dev,_BEEP_FEATURE_OFF,XkbStickyKeysMask);
    }
#ifndef NO_CLEAR_LATCHES_FOR_STICKY_KEYS_OFF
    XkbClearAllLatchesAndLocks(dev,xkbi,False,&cause);
#endif
    return;
} /* AccessXStickyKeysTurnOff */

static CARD32
#if NeedFunctionPrototypes
AccessXKRGExpire(OsTimerPtr timer,CARD32 now,pointer arg)
#else
AccessXKRGExpire(timer,now,arg)
    OsTimerPtr	 timer;
    CARD32	 now;
    pointer	 arg;
#endif
{
XkbSrvInfoPtr		xkbi= ((DeviceIntPtr)arg)->key->xkbInfo;
xkbControlsNotify	cn;

    if (xkbi->krgTimerActive==_KRG_WARN_TIMER) {
	XkbDDXAccessXBeep((DeviceIntPtr)arg,_BEEP_SLOW_WARN,XkbStickyKeysMask);
	xkbi->krgTimerActive= _KRG_TIMER;
	return 4000;
    }
    xkbi->krgTimerActive= _OFF_TIMER;
    cn.keycode = 0;
    cn.eventType = 0;
    cn.requestMajor = 0;
    cn.requestMinor = 0;
    if (xkbi->desc->ctrls->enabled_ctrls&XkbSlowKeysMask)
	 AccessXKRGTurnOff((DeviceIntPtr)arg,&cn);
    else AccessXKRGTurnOn((DeviceIntPtr)arg,XkbSlowKeysMask,&cn);
    return 0;
}

static CARD32
#if NeedFunctionPrototypes
AccessXRepeatKeyExpire(OsTimerPtr timer,CARD32 now,pointer arg)
#else
AccessXRepeatKeyExpire(timer,now,arg)
    OsTimerPtr	 timer;
    CARD32	 now;
    pointer	 arg;
#endif
{
XkbSrvInfoPtr	xkbi= ((DeviceIntPtr)arg)->key->xkbInfo;
KeyCode		key;

    if (xkbi->repeatKey==0)
	return 0;
    key= xkbi->repeatKey;
    AccessXKeyboardEvent((DeviceIntPtr)arg,KeyRelease,key,True);
    AccessXKeyboardEvent((DeviceIntPtr)arg,KeyPress,key,True);
    return xkbi->desc->ctrls->repeat_interval;
}

void
#if NeedFunctionPrototypes
AccessXCancelRepeatKey(XkbSrvInfoPtr xkbi,KeyCode key)
#else
AccessXCancelRepeatKey(xkbi,key)
    XkbSrvInfoPtr	xkbi;
    KeyCode		key;
#endif
{
    if (xkbi->repeatKey==key)
	xkbi->repeatKey= 0;
    return;
}

static CARD32
#if NeedFunctionPrototypes
AccessXSlowKeyExpire(OsTimerPtr timer,CARD32 now,pointer arg)
#else
AccessXSlowKeyExpire(timer,now,arg)
    OsTimerPtr	 timer;
    CARD32	 now;
    pointer	 arg;
#endif
{
DeviceIntPtr	keybd;
XkbSrvInfoPtr	xkbi;
XkbDescPtr	xkb;
XkbControlsPtr	ctrls;

    keybd= 	(DeviceIntPtr)arg;
    xkbi= 	keybd->key->xkbInfo;
    xkb= 	xkbi->desc;
    ctrls= 	xkb->ctrls;
    if (xkbi->slowKey!=0) {
	xkbAccessXNotify ev;
	KeySym *sym= XkbKeySymsPtr(xkb,xkbi->slowKey);
	ev.detail= XkbAXN_SKAccept;
	ev.keycode= xkbi->slowKey;
	ev.slowKeysDelay= ctrls->slow_keys_delay;
	ev.debounceDelay= ctrls->debounce_delay;
	XkbSendAccessXNotify(keybd,&ev);
	if (XkbAX_NeedFeedback(ctrls,XkbAX_SKAcceptFBMask))
	    XkbDDXAccessXBeep(keybd,_BEEP_SLOW_ACCEPT,XkbSlowKeysMask);
	AccessXKeyboardEvent(keybd,KeyPress,xkbi->slowKey,False);
	/* check for magic sequences */
	if ((ctrls->enabled_ctrls&XkbAccessXKeysMask) &&
	    ((sym[0]==XK_Shift_R)||(sym[0]==XK_Shift_L)))
	    xkbi->shiftKeyCount++;

	/* Start repeating if necessary.  Stop autorepeating if the user
	 * presses a non-modifier key that doesn't autorepeat.
	 */
	if (keybd->kbdfeed->ctrl.autoRepeat && 
	    ((xkbi->slowKey != xkbi->mouseKey) || (!xkbi->mouseKeysAccel)) &&
	     (ctrls->enabled_ctrls&XkbRepeatKeysMask)) {
#ifndef AIXV3
	    if (BitIsOn(keybd->kbdfeed->ctrl.autoRepeats,xkbi->slowKey))
#endif
	    {
		xkbi->repeatKey = xkbi->slowKey;
		xkbi->repeatKeyTimer= TimerSet(xkbi->repeatKeyTimer,
					0, ctrls->repeat_delay,
					AccessXRepeatKeyExpire, (pointer)keybd);
	    }
	}
    }
    return 0;
}

static CARD32
#if NeedFunctionPrototypes
AccessXBounceKeyExpire(OsTimerPtr timer,CARD32 now,pointer arg)
#else
AccessXBounceKeyExpire(timer,now,arg)
    OsTimerPtr	 timer;
    CARD32	 now;
    pointer	 arg;
#endif
{
XkbSrvInfoPtr	xkbi= ((DeviceIntPtr)arg)->key->xkbInfo;

    xkbi->inactiveKey= 0;
    return 0;
}

static CARD32
#if NeedFunctionPrototypes
AccessXTimeoutExpire(OsTimerPtr timer,CARD32 now,pointer arg)
#else
AccessXTimeoutExpire(timer,now,arg)
    OsTimerPtr	 timer;
    CARD32	 now;
    pointer	 arg;
#endif
{
DeviceIntPtr		dev = (DeviceIntPtr)arg;
XkbSrvInfoPtr		xkbi= dev->key->xkbInfo;
XkbControlsPtr		ctrls= xkbi->desc->ctrls;
XkbControlsRec		old;
xkbControlsNotify	cn;
XkbEventCauseRec	cause;
XkbSrvLedInfoPtr	sli;

    if (xkbi->lastPtrEventTime) {
	unsigned timeToWait = (ctrls->ax_timeout*1000);
	unsigned timeElapsed = (now-xkbi->lastPtrEventTime);

	if (timeToWait > timeElapsed)
	    return (timeToWait - timeElapsed);
    }
    old= *ctrls;
    xkbi->shiftKeyCount= 0;
    ctrls->enabled_ctrls&= ~ctrls->axt_ctrls_mask;
    ctrls->enabled_ctrls|= 
	(ctrls->axt_ctrls_values&ctrls->axt_ctrls_mask);
    if (ctrls->axt_opts_mask) {
	ctrls->ax_options&= ~ctrls->axt_opts_mask;
	ctrls->ax_options|= (ctrls->axt_opts_values&ctrls->axt_opts_mask);
    }
    if (XkbComputeControlsNotify(dev,&old,ctrls,&cn,False)) {
	cn.keycode = 0;
	cn.eventType = 0;
	cn.requestMajor = 0;
	cn.requestMinor = 0;
	XkbSendControlsNotify(dev,&cn);
    }
    XkbSetCauseUnknown(&cause);
    sli= XkbFindSrvLedInfo(dev,XkbDfltXIClass,XkbDfltXIId,0);
    XkbUpdateIndicators(dev,sli->usesControls,True,NULL,&cause);
    if (ctrls->ax_options!=old.ax_options) {
	unsigned set,cleared,bell;
	set= ctrls->ax_options&(~old.ax_options);
	cleared= (~ctrls->ax_options)&old.ax_options;
	if (set && cleared)	bell= _BEEP_FEATURE_CHANGE;
	else if (set)		bell= _BEEP_FEATURE_ON;
	else 			bell= _BEEP_FEATURE_OFF;
	XkbDDXAccessXBeep(dev,bell,XkbAccessXTimeoutMask);
    }
    xkbi->krgTimerActive= _OFF_TIMER;
    return 0;
}


/************************************************************************/
/*									*/
/* AccessXFilterPressEvent						*/
/*									*/
/* Filter events before they get any further if SlowKeys is turned on.	*/
/* In addition, this routine handles the ever so popular magic key	*/
/* acts for turning various accessibility features on/off.		*/
/*									*/
/* Returns TRUE if this routine has discarded the event.		*/
/* Returns FALSE if the event needs further processing.			*/
/*									*/
/************************************************************************/
Bool
#if NeedFunctionPrototypes
AccessXFilterPressEvent(	register xEvent *	xE, 
				register DeviceIntPtr	keybd, 
				int			count)
#else
AccessXFilterPressEvent(xE,keybd,count)
    register xEvent 		*xE;
    register DeviceIntPtr	keybd;
    int				count;    
#endif
{
XkbSrvInfoPtr	xkbi = keybd->key->xkbInfo;
XkbControlsPtr	ctrls = xkbi->desc->ctrls;
Bool		ignoreKeyEvent = FALSE;
KeyCode		key = xE->u.u.detail;
KeySym *	sym = XkbKeySymsPtr(xkbi->desc,key);

    if (ctrls->enabled_ctrls&XkbAccessXKeysMask) {
	/* check for magic sequences */
	if ((sym[0]==XK_Shift_R)||(sym[0]==XK_Shift_L)) {
	    if (XkbAX_NeedFeedback(ctrls,XkbAX_SlowWarnFBMask)) {
		xkbi->krgTimerActive = _KRG_WARN_TIMER;
		xkbi->krgTimer= TimerSet(xkbi->krgTimer, 0, 4000,
					AccessXKRGExpire, (pointer)keybd);
	    }
	    else {
		xkbi->krgTimerActive = _KRG_TIMER;
		xkbi->krgTimer= TimerSet(xkbi->krgTimer, 0, 8000,
					AccessXKRGExpire, (pointer)keybd);
	    }
	    if (!(ctrls->enabled_ctrls & XkbSlowKeysMask)) {
		CARD32 now= GetTimeInMillis();
		if ((now-xkbi->lastShiftEventTime)>15000)
		     xkbi->shiftKeyCount= 1;
		else xkbi->shiftKeyCount++;
		xkbi->lastShiftEventTime= now;
	    }
	}
	else {
	    if (xkbi->krgTimerActive) {
		xkbi->krgTimer= TimerSet(xkbi->krgTimer,0, 0, NULL, NULL);
		xkbi->krgTimerActive= _OFF_TIMER;
	    }
	}
    }
	
    /* Don't transmit the KeyPress if SlowKeys is turned on;
     * The wakeup handler will synthesize one for us if the user
     * has held the key long enough.
     */
    if (ctrls->enabled_ctrls & XkbSlowKeysMask) {
	xkbAccessXNotify	ev;
	ev.detail= XkbAXN_SKPress;
	ev.keycode= key;
	ev.slowKeysDelay= ctrls->slow_keys_delay;
	ev.debounceDelay= ctrls->debounce_delay;
	XkbSendAccessXNotify(keybd,&ev);
	if (XkbAX_NeedFeedback(ctrls,XkbAX_SKPressFBMask))
	    XkbDDXAccessXBeep(keybd,_BEEP_SLOW_PRESS,XkbSlowKeysMask);
	xkbi->slowKey= key;
	xkbi->slowKeysTimer = TimerSet(xkbi->slowKeysTimer,
				 0, ctrls->slow_keys_delay,
				 AccessXSlowKeyExpire, (pointer)keybd);
	ignoreKeyEvent = TRUE;
    }

    /* Don't transmit the KeyPress if BounceKeys is turned on
     * and the user pressed the same key within a given time period
     * from the last release.
     */
    else if ((ctrls->enabled_ctrls & XkbBounceKeysMask) && 
					(key == xkbi->inactiveKey)) {
	if (XkbAX_NeedFeedback(ctrls,XkbAX_BKRejectFBMask))
	    XkbDDXAccessXBeep(keybd,_BEEP_BOUNCE_REJECT,XkbBounceKeysMask);
	ignoreKeyEvent = TRUE;
    }

    if ((ctrls->enabled_ctrls & XkbMouseKeysAccelMask) && 
					(key == xkbi->mouseKey)) {
        ignoreKeyEvent = TRUE;
    }

    /* Start repeating if necessary.  Stop autorepeating if the user
     * presses a non-modifier key that doesn't autorepeat.
     */
    if (XkbDDXUsesSoftRepeat(keybd)) {
	if ((keybd->kbdfeed->ctrl.autoRepeat) &&
		((ctrls->enabled_ctrls&(XkbSlowKeysMask|XkbRepeatKeysMask))==
							XkbRepeatKeysMask)) {
#ifndef AIXV3
	    if (BitIsOn(keybd->kbdfeed->ctrl.autoRepeats,key))
#endif
	    {
#ifdef DEBUG
		if (xkbDebugFlags&0x10)
		    ErrorF("Starting software autorepeat...\n");
#endif	    
		xkbi->repeatKey = key;
		xkbi->repeatKeyTimer= TimerSet(xkbi->repeatKeyTimer,
					0, ctrls->repeat_delay,
					AccessXRepeatKeyExpire, (pointer)keybd);
	    }
	}
    }
    
    /* Check for two keys being pressed at the same time.  This section
     * essentially says the following:
     *
     *	If StickyKeys is on, and a modifier is currently being held down,
     *  and one of the following is true:  the current key is not a modifier
     *  or the currentKey is a modifier, but not the only modifier being
     *  held down, turn StickyKeys off if the TwoKeys off ctrl is set.
     */
    if ((ctrls->enabled_ctrls & XkbStickyKeysMask) && 
				(xkbi->state.base_mods!=0) &&
				(XkbAX_NeedOption(ctrls,XkbAX_TwoKeysMask))) {
	xkbControlsNotify cn;
	cn.keycode = key;
	cn.eventType = KeyPress;
	cn.requestMajor = 0;
	cn.requestMinor = 0;
	AccessXStickyKeysTurnOff(keybd,&cn);
    }
    
    if (!ignoreKeyEvent)
	XkbProcessKeyboardEvent(xE,keybd,count);
    return ignoreKeyEvent;
} /* AccessXFilterPressEvent */

/************************************************************************/
/*									*/
/* AccessXFilterReleaseEvent						*/
/*									*/
/* Filter events before they get any further if SlowKeys is turned on.	*/
/* In addition, this routine handles the ever so popular magic key	*/
/* acts for turning various accessibility features on/off.		*/
/*									*/
/* Returns TRUE if this routine has discarded the event.		*/
/* Returns FALSE if the event needs further processing.			*/
/*									*/
/************************************************************************/
Bool
#if NeedFunctionPrototypes
AccessXFilterReleaseEvent(	register xEvent *	xE, 
				register DeviceIntPtr	keybd, 
				int			count)
#else
AccessXFilterReleaseEvent(xE,keybd,count)
    register xEvent 		*xE;
    register DeviceIntPtr	keybd;
    int				count;    
#endif
{
XkbSrvInfoPtr	xkbi = keybd->key->xkbInfo;
XkbControlsPtr	ctrls = xkbi->desc->ctrls;
KeyCode		key = xE->u.u.detail;
Bool		ignoreKeyEvent = FALSE;
    
    /* Don't transmit the KeyRelease if BounceKeys is on and
     * this is the release of a key that was ignored due to 
     * BounceKeys.
     */
    if (ctrls->enabled_ctrls & XkbBounceKeysMask) {
	if ((key!=xkbi->mouseKey)&&(!BitIsOn(keybd->key->down,key)))
	    ignoreKeyEvent = TRUE;
	xkbi->inactiveKey= key;
	xkbi->bounceKeysTimer= TimerSet(xkbi->bounceKeysTimer, 0,
					ctrls->debounce_delay,
					AccessXBounceKeyExpire, (pointer)keybd);
    }

    /* Don't transmit the KeyRelease if SlowKeys is turned on and
     * the user didn't hold the key long enough.  We know we passed
     * the key if the down bit was set by CoreProcessKeyboadEvent.
     */
    if (ctrls->enabled_ctrls & XkbSlowKeysMask) {
	xkbAccessXNotify	ev;
	unsigned		beep_type;
	ev.keycode= key;
	ev.slowKeysDelay= ctrls->slow_keys_delay;
	ev.debounceDelay= ctrls->debounce_delay;
	if (BitIsOn(keybd->key->down,key) | (xkbi->mouseKey == key)) {
	    ev.detail= XkbAXN_SKRelease;
	    beep_type= _BEEP_SLOW_RELEASE;
	}
	else {
	    ev.detail= XkbAXN_SKReject;
	    beep_type= _BEEP_SLOW_REJECT;
	    ignoreKeyEvent = TRUE;
	}
	XkbSendAccessXNotify(keybd,&ev);
	if (XkbAX_NeedFeedback(ctrls,XkbAX_SKRejectFBMask)) {
	    XkbDDXAccessXBeep(keybd,beep_type,XkbSlowKeysMask);
	}
	if (xkbi->slowKey==key)
	    xkbi->slowKey= 0;
    }

    /* Stop Repeating if the user releases the key that is currently
     * repeating.
     */
    if (xkbi->repeatKey==key) {
	xkbi->repeatKey= 0;
    }

    if ((ctrls->enabled_ctrls&XkbAccessXTimeoutMask)&&(ctrls->ax_timeout>0)) {
	xkbi->lastPtrEventTime= 0;
	xkbi->krgTimer= TimerSet(xkbi->krgTimer, 0, 
					ctrls->ax_timeout*1000,
					AccessXTimeoutExpire, (pointer)keybd);
	xkbi->krgTimerActive= _ALL_TIMEOUT_TIMER;
    }
    else if (xkbi->krgTimerActive!=_OFF_TIMER) {
	xkbi->krgTimer= TimerSet(xkbi->krgTimer, 0, 0, NULL, NULL);
	xkbi->krgTimerActive= _OFF_TIMER;
    }
	
    /* Keep track of how many times the Shift key has been pressed.
     * If it has been pressed and released 5 times in a row, toggle
     * the state of StickyKeys.
     */
    if ((!ignoreKeyEvent)&&(xkbi->shiftKeyCount)) {
	KeySym *pSym= XkbKeySymsPtr(xkbi->desc,key);
	if ((pSym[0]!=XK_Shift_L)&&(pSym[0]!=XK_Shift_R)) {
	    xkbi->shiftKeyCount= 0;
	}
	else if (xkbi->shiftKeyCount>=5) {
	     xkbControlsNotify cn;
	     cn.keycode = key;
	     cn.eventType = KeyPress;
	     cn.requestMajor = 0;
	     cn.requestMinor = 0;
	     if (ctrls->enabled_ctrls & XkbStickyKeysMask)
		AccessXStickyKeysTurnOff(keybd,&cn);
	     else
		AccessXStickyKeysTurnOn(keybd,&cn);
	     xkbi->shiftKeyCount= 0;
	}
    }
    
    if (!ignoreKeyEvent)
	XkbProcessKeyboardEvent(xE,keybd,count);
    return ignoreKeyEvent;
    
} /* AccessXFilterReleaseEvent */

/************************************************************************/
/*									*/
/* ProcessPointerEvent							*/
/*									*/
/* This routine merely sets the shiftKeyCount and clears the keyboard   */
/* response group timer (if necessary) on a mouse event.  This is so	*/
/* multiple shifts with just the mouse and shift-drags with the mouse	*/
/* don't accidentally turn on StickyKeys or the Keyboard Response Group.*/
/*									*/
/************************************************************************/
void
#if NeedFunctionPrototypes
ProcessPointerEvent(	register xEvent  *	xE, 
			register DeviceIntPtr	mouse, 
			int		        count)
#else
ProcessPointerEvent(xE,mouse,count)
    register xEvent 		*xE;
    register DeviceIntPtr	mouse;
    int		        	count;
#endif
{
DeviceIntPtr	dev = (DeviceIntPtr)LookupKeyboardDevice();
XkbSrvInfoPtr	xkbi = dev->key->xkbInfo;
unsigned 	changed = 0;

    xkbi->shiftKeyCount = 0;
    xkbi->lastPtrEventTime= xE->u.keyButtonPointer.time;

    if (xE->u.u.type==ButtonPress) {
	    changed |= XkbPointerButtonMask;
    }
    else if (xE->u.u.type==ButtonRelease) {
	xkbi->lockedPtrButtons&= ~(1<<(xE->u.u.detail&0x7));
	changed |= XkbPointerButtonMask;
    }
    CoreProcessPointerEvent(xE,mouse,count);

    xkbi->state.ptr_buttons = mouse->button->state;
    
    /* clear any latched modifiers */
    if ( xkbi->state.latched_mods && (xE->u.u.type==ButtonRelease) ) {
	unsigned 		changed_leds;
	XkbStateRec		oldState;
	XkbSrvLedInfoPtr	sli;

	sli= XkbFindSrvLedInfo(dev,XkbDfltXIClass,XkbDfltXIId,0);
	oldState= xkbi->state;
	XkbLatchModifiers(dev,0xFF,0x00);

	XkbComputeDerivedState(xkbi);
	changed |= XkbStateChangedFlags(&oldState,&xkbi->state);
	if (changed&sli->usedComponents) {
	    changed_leds= XkbIndicatorsToUpdate(dev,changed,False);
	    if (changed_leds) {
		XkbEventCauseRec	cause;
		XkbSetCauseKey(&cause,(xE->u.u.detail&0x7),xE->u.u.type);
		XkbUpdateIndicators(dev,changed_leds,True,NULL,&cause);
	    }
	}
	dev->key->state= XkbStateFieldFromRec(&xkbi->state);
    }

    if (((xkbi->flags&_XkbStateNotifyInProgress)==0)&&(changed!=0)) {
	xkbStateNotify	sn;
	sn.keycode= xE->u.u.detail;
	sn.eventType= xE->u.u.type;
	sn.requestMajor = sn.requestMinor = 0;
	sn.changed= changed;
	XkbSendStateNotify(dev,&sn);
    }

} /* ProcessPointerEvent */




