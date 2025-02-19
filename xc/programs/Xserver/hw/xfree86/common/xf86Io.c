/* $XFree86: xc/programs/Xserver/hw/xfree86/common/xf86Io.c,v 3.41 2000/11/06 19:24:06 dawes Exp $ */
/*
 * Copyright 1990,91 by Thomas Roell, Dinkelscherben, Germany.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Thomas Roell not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Thomas Roell makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * THOMAS ROELL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THOMAS ROELL BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 */
/* $XConsortium: xf86Io.c /main/27 1996/10/19 17:58:55 kaleb $ */

#define NEED_EVENTS
#include "X.h"
#include "Xproto.h"
#include "inputstr.h"
#include "scrnintstr.h"

#include "compiler.h"

#include "xf86.h"
#include "xf86Priv.h"
#define XF86_OS_PRIVS
#include "xf86_OSlib.h"
#include "mipointer.h"

#ifdef XINPUT
#include "xf86Xinput.h"
#include "XIproto.h"
#include "exevents.h"
#endif

#ifdef XKB
#include <X11/extensions/XKB.h>
#include <X11/extensions/XKBstr.h>
#include <X11/extensions/XKBsrv.h>
#endif

unsigned int xf86InitialCaps = 0;
unsigned int xf86InitialNum = 0;
unsigned int xf86InitialScroll = 0;

#include "atKeynames.h"

/*
 * xf86KbdBell --
 *	Ring the terminal/keyboard bell for an amount of time proportional to
 *      "loudness".
 */

void
xf86KbdBell(percent, pKeyboard, ctrl, unused)
     int           percent;          /* Percentage of full volume */
     DeviceIntPtr  pKeyboard;        /* Keyboard to ring */
     pointer	   ctrl;	
     int	   unused;	
{
  xf86SoundKbdBell(percent, xf86Info.bell_pitch, xf86Info.bell_duration);
}

#ifdef AMOEBA
#define LED_CAP	IOP_LED_CAP
#define LED_NUM	IOP_LED_NUM
#define LED_SCR	IOP_LED_SCROLL
#endif

#ifdef MINIX
#define LED_CAP KBD_LEDS_CAPS
#define LED_NUM KBD_LEDS_NUM
#define LED_SCR KBD_LEDS_SCROLL
#endif

void
xf86KbdLeds ()
{
  int leds = 0;
#ifdef XKB
  if (!noXkbExtension) {
    XkbEventCauseRec cause;
   XkbSetCauseUnknown(&cause);
    XkbUpdateIndicators((DeviceIntPtr)xf86Info.pKeyboard,
			XkbAllIndicatorsMask, False, NULL, &cause);
    return;
  }
#endif
#ifdef LED_CAP
  if (xf86Info.capsLock && !(xf86Info.xleds & XLED1))
    leds |= LED_CAP;

  if (xf86Info.numLock && !(xf86Info.xleds & XLED2))
    leds |= LED_NUM;

  if ((xf86Info.scrollLock || 
       xf86Info.modeSwitchLock || 
       xf86Info.composeLock) && 
      !(xf86Info.xleds & XLED3))
    leds |= LED_SCR;

  if ((xf86Info.leds & xf86Info.xleds) & XLED1) leds |= LED_CAP;
  if ((xf86Info.leds & xf86Info.xleds) & XLED2) leds |= LED_NUM;
  if ((xf86Info.leds & xf86Info.xleds) & XLED3) leds |= LED_SCR;

  xf86SetKbdLeds(leds);
#endif /* LED_CAP */
}

/*
 * xf86KbdCtrl --
 *      Alter some of the keyboard control parameters. All special protocol
 *      values are handled by dix (ProgChangeKeyboardControl)
 */

void
xf86KbdCtrl (pKeyboard, ctrl)
     DevicePtr     pKeyboard;        /* Keyboard to alter */
     KeybdCtrl     *ctrl;
{
  xf86Info.bell_pitch    = ctrl->bell_pitch;
  xf86Info.bell_duration = ctrl->bell_duration;
  xf86Info.autoRepeat    = ctrl->autoRepeat;
  xf86Info.leds          = (ctrl->leds & ~(XCAPS | XNUM | XSCR));

  xf86Info.composeLock   = (ctrl->leds & XCOMP) ? TRUE : FALSE;

  xf86KbdLeds();
}

/*
 * xf86InitKBD --
 *      Reinitialize the keyboard. Only set Lockkeys accrding to ours leds.
 *      Depress all other keys.
 */

void
xf86InitKBD(init)
Bool init;
{
  char            leds = 0, rad;
  unsigned int    i;
  xEvent          kevent;
  DeviceIntPtr    pKeyboard = xf86Info.pKeyboard;
  KeyClassRec     *keyc = xf86Info.pKeyboard->key;
  KeySym          *map = keyc->curKeySyms.map;

#ifndef MACH386
  kevent.u.keyButtonPointer.time = GetTimeInMillis();
  kevent.u.keyButtonPointer.rootX = 0;
  kevent.u.keyButtonPointer.rootY = 0;

  /*
   * Hmm... here is the biggest hack of every time !
   * It may be possible that a switch-vt procedure has finished BEFORE
   * you released all keys neccessary to do this. That peculiar behavior
   * can fool the X-server pretty much, cause it assumes that some keys
   * were not released. TWM may stuck alsmost completly....
   * OK, what we are doing here is after returning from the vt-switch
   * exeplicitely unrelease all keyboard keys before the input-devices
   * are reenabled.
   */
  for (i = keyc->curKeySyms.minKeyCode, map = keyc->curKeySyms.map;
       i < keyc->curKeySyms.maxKeyCode;
       i++, map += keyc->curKeySyms.mapWidth)
    if (KeyPressed(i))
      {
        switch (*map) {
	/* Don't release the lock keys */
        case XK_Caps_Lock:
        case XK_Shift_Lock:
        case XK_Num_Lock:
        case XK_Scroll_Lock:
        case XK_Kana_Lock:
	  break;
        default:
	  kevent.u.u.detail = i;
	  kevent.u.u.type = KeyRelease;
	  (* pKeyboard->public.processInputProc)(&kevent, pKeyboard, 1);
        }
      }
#endif /* MACH386 */
  
  xf86Info.scanPrefix      = 0;

  if (init)
    {
      /*
       * we must deal here with the fact, that on some cases the numlock or
       * capslock key are enabled BEFORE the server is started up. So look
       * here at the state on the according LEDS to determine whether a
       * lock-key is already set.
       */

      xf86Info.capsLock        = FALSE;
      xf86Info.numLock         = FALSE;
      xf86Info.scrollLock      = FALSE;
      xf86Info.modeSwitchLock  = FALSE;
      xf86Info.composeLock     = FALSE;
    
#ifdef LED_CAP
#ifdef INHERIT_LOCK_STATE
      leds = xf86Info.leds;

      for (i = keyc->curKeySyms.minKeyCode, map = keyc->curKeySyms.map;
           i < keyc->curKeySyms.maxKeyCode;
           i++, map += keyc->curKeySyms.mapWidth)

        switch(*map) {

        case XK_Caps_Lock:
        case XK_Shift_Lock:
          if (leds & LED_CAP) 
	    {
	      xf86InitialCaps = i;
	      xf86Info.capsLock = TRUE;
	    }
          break;

        case XK_Num_Lock:
          if (leds & LED_NUM)
	    {
	      xf86InitialNum = i;
	      xf86Info.numLock = TRUE;
	    }
          break;

        case XK_Scroll_Lock:
        case XK_Kana_Lock:
          if (leds & LED_SCR)
	    {
	      xf86InitialScroll = i;
	      xf86Info.scrollLock = TRUE;
	    }
          break;
        }
#endif /* INHERIT_LOCK_STATE */
      xf86SetKbdLeds(leds);
#endif /* LED_CAP */

      if      (xf86Info.kbdDelay <= 375) rad = 0x00;
      else if (xf86Info.kbdDelay <= 625) rad = 0x20;
      else if (xf86Info.kbdDelay <= 875) rad = 0x40;
      else                               rad = 0x60;
    
      if      (xf86Info.kbdRate <=  2)   rad |= 0x1F;
      else if (xf86Info.kbdRate >= 30)   rad |= 0x00;
      else                               rad |= ((58 / xf86Info.kbdRate) - 2);
    
      xf86SetKbdRepeat(rad);
    }
}

/*
 * xf86KbdProc --
 *	Handle the initialization, etc. of a keyboard.
 */

int
xf86KbdProc (pKeyboard, what)
     DeviceIntPtr pKeyboard;	/* Keyboard to manipulate */
     int       what;	    	/* What to do to it */
{
  KeySymsRec           keySyms;
  CARD8                modMap[MAP_LENGTH];
  int                  kbdFd;

  switch (what) {

  case DEVICE_INIT:
    /*
     * First open and find the current state of the keyboard.
     */

    xf86KbdInit();

    xf86KbdGetMapping(&keySyms, modMap);
    

#ifndef XKB
    defaultKeyboardControl.leds = xf86GetKbdLeds();
#else
    defaultKeyboardControl.leds = 0;
#endif

    /*
     * Perform final initialization of the system private keyboard
     * structure and fill in various slots in the device record
     * itself which couldn't be filled in before.
     */

    pKeyboard->public.on = FALSE;

#ifdef XKB
    if (noXkbExtension) {
#endif
    InitKeyboardDeviceStruct((DevicePtr)xf86Info.pKeyboard,
			     &keySyms,
			     modMap,
			     xf86KbdBell,
			     (KbdCtrlProcPtr)xf86KbdCtrl);
#ifdef XKB
    } else {
 	XkbComponentNamesRec names;
	if (XkbInitialMap) {
	    if ((xf86Info.xkbkeymap = strchr(XkbInitialMap, '/')) != NULL)
		xf86Info.xkbkeymap++;
	    else
		xf86Info.xkbkeymap = XkbInitialMap;
	}
	if (xf86Info.xkbkeymap) {
	    names.keymap = xf86Info.xkbkeymap;
	    names.keycodes = NULL;
	    names.types = NULL;
	    names.compat = NULL;
	    names.symbols = NULL;
	    names.geometry = NULL;
	} else {
	    names.keymap = NULL;
	    names.keycodes = xf86Info.xkbkeycodes;
	    names.types = xf86Info.xkbtypes;
	    names.compat = xf86Info.xkbcompat;
	    names.symbols = xf86Info.xkbsymbols;
	    names.geometry = xf86Info.xkbgeometry;
	}
	if ((xf86Info.xkbkeymap || xf86Info.xkbcomponents_specified)
	   && (xf86Info.xkbmodel == NULL || xf86Info.xkblayout == NULL)) {
		xf86Info.xkbrules = NULL;
	}
	XkbSetRulesDflts(xf86Info.xkbrules, xf86Info.xkbmodel,
			 xf86Info.xkblayout, xf86Info.xkbvariant,
			 xf86Info.xkboptions);
	XkbInitKeyboardDeviceStruct(pKeyboard, 
				    &names,
				    &keySyms, 
				    modMap, 
				    xf86KbdBell,
				    (KbdCtrlProcPtr)xf86KbdCtrl);
    }
#endif
    
    xf86InitKBD(TRUE);
    break;
    
  case DEVICE_ON:
    /*
     * Set the keyboard into "direct" mode and turn on
     * event translation.
     */

    kbdFd = xf86KbdOn();
    /*
     * Discard any pending input after a VT switch to prevent the server
     * passing on parts of the VT switch sequence.
     */
    sleep(1);
    if (kbdFd != -1) {
	char buf[16];
	read(kbdFd, buf, 16);
    }

#if !defined(__EMX__) && !defined(__CYGWIN__)  /* Under EMX, keyboard cannot be select()'ed */
    if (kbdFd != -1)
      AddEnabledDevice(kbdFd);
#endif  /* __EMX__ */

    pKeyboard->public.on = TRUE;
    xf86InitKBD(FALSE);
    break;
    
  case DEVICE_CLOSE:
  case DEVICE_OFF:
    /*
     * Restore original keyboard directness and translation.
     */

    kbdFd = xf86KbdOff();

    if (kbdFd != -1)
      RemoveEnabledDevice(kbdFd);

    pKeyboard->public.on = FALSE;
    break;

  }
  return (Success);
}

#if !defined(AMOEBA) && !(defined (sun) && defined(i386) && defined (SVR4)) && !defined(MINIX) && !(defined(__mips__) && !defined(linux)) && !defined(QNX4)
/*
 * These are getting tossed in here until I can think of where
 * they really belong
 */
CARD32
GetTimeInMillis()
{
    struct timeval  tp;

    gettimeofday(&tp, 0);
    return(tp.tv_sec * 1000) + (tp.tv_usec / 1000);
}
#endif /* !AMOEBA && !(sun || SVR4) && !MINIX */

#ifdef WSCONS_SUPPORT

#define NUMEVENTS 64

static void
wsconssig(fd, closure)
    int fd;
    void *closure;
{
    static struct wscons_event events[NUMEVENTS];
    int n, i;

    n = read(fd, events, sizeof events);
    if (n <= 0)
	return;
    n /= sizeof(struct wscons_event);
    for (i = 0; i < n; i++)
	xf86PostWSKbdEvent(&events[i]);
}

int
xf86WSKbdProc(pKeyboard, what)
   DeviceIntPtr pKeyboard;	/* Keyboard to manipulate */
   int what;			/* What to do to it */
{
    switch (what) {
    case DEVICE_INIT:
	xf86FlushInput(xf86Info.kbdFd);
	xf86InstallSIGIOHandler(xf86Info.kbdFd, wsconssig, pKeyboard);
    }
    return xf86KbdProc(pKeyboard, what);
}
#endif /* WSCONS_SUPPORT */
