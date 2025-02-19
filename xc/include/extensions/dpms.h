/* $TOG: dpms.h /main/1 1997/11/12 14:36:52 kaleb $ */
/*****************************************************************

Copyright (c) 1996 Digital Equipment Corporation, Maynard, Massachusetts.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
DIGITAL EQUIPMENT CORPORATION BE LIABLE FOR ANY CLAIM, DAMAGES, INCLUDING, 
BUT NOT LIMITED TO CONSEQUENTIAL OR INCIDENTAL DAMAGES, OR OTHER LIABILITY, 
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR 
IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of Digital Equipment Corporation 
shall not be used in advertising or otherwise to promote the sale, use or other
dealings in this Software without prior written authorization from Digital 
Equipment Corporation.

******************************************************************/
/* $XFree86: xc/include/extensions/dpms.h,v 3.4 2000/03/15 16:51:51 tsi Exp $ */

/*
 * HISTORY
 */
/*
 * @(#)RCSfile: dpms.h,v Revision: 1.1.4.2  (DEC) Date: 1995/11/21 19:34:17
 */


#define DPMSModeOn	0
#define DPMSModeStandby	1
#define DPMSModeSuspend	2
#define DPMSModeOff	3

#ifndef DPMS_SERVER

#include <X11/X.h>
#include <X11/Xmd.h>

extern Bool DPMSQueryExtension(Display *, int *, int *);
extern Status DPMSGetVersion(Display *, int *, int *);
extern Bool DPMSCapable(Display *);
extern Status DPMSSetTimeouts(Display *, CARD16, CARD16, CARD16);
extern Bool DPMSGetTimeouts(Display *, CARD16 *, CARD16 *, CARD16 *);
extern Status DPMSEnable(Display *);
extern Status DPMSDisable(Display *);
extern Status DPMSForceLevel(Display *, CARD16);
extern Status DPMSInfo(Display *, CARD16 *, BOOL *);
#endif


