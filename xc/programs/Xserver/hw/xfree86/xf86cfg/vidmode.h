/*
 * Copyright (c) 2000 by Conectiva S.A. (http://www.conectiva.com)
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *  
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * CONECTIVA LINUX BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 * 
 * Except as contained in this notice, the name of Conectiva Linux shall
 * not be used in advertising or otherwise to promote the sale, use or other
 * dealings in this Software without prior written authorization from
 * Conectiva Linux.
 *
 * Author: Paulo C�sar Pereira de Andrade <pcpa@conectiva.com.br>
 *
 * $XFree86: xc/programs/Xserver/hw/xfree86/xf86cfg/vidmode.h,v 1.1 2000/05/18 16:30:00 dawes Exp $
 */

#ifndef _xf86cfg_vidmode_h
#define _xf86cfg_vidmode_h

#include "xf86config.h"
#include <X11/extensions/xf86vmode.h>

/*
 * Types
 */
struct _xf86cfgVidMode {
    XF86ConfMonitorPtr monitor;
    int screen;
    int num_infos;
    XF86VidModeModeInfo **infos;
};

/*
 * Prototypes
 */
Bool VideoModeInitialize(void);
void VideoModeConfigureStart(void);
void VideoModeConfigureEnd(void);
void VidmodeRestoreAction(Widget, XEvent*, String*, Cardinal*);
void CancelForceAddModeAction(Widget, XEvent*, String*, Cardinal*);
void CancelTestModeAction(Widget, XEvent*, String*, Cardinal*);

/*
 * Initialization
 */
extern Widget vtune;

#endif /* _xf86cfg_vidmode_h */
