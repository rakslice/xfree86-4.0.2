/* $XConsortium: XpScreens.c /main/2 1996/11/16 15:22:09 rws $ */
/******************************************************************************
 ******************************************************************************
 **
 ** (c) Copyright 1996 Hewlett-Packard Company
 ** (c) Copyright 1996 International Business Machines Corp.
 ** (c) Copyright 1996 Sun Microsystems, Inc.
 ** (c) Copyright 1996 Novell, Inc.
 ** (c) Copyright 1996 Digital Equipment Corp.
 ** (c) Copyright 1996 Fujitsu Limited
 ** (c) Copyright 1996 Hitachi, Ltd.
 ** 
 ** Permission is hereby granted, free of charge, to any person obtaining a copy
 ** of this software and associated documentation files (the "Software"), to deal
 ** in the Software without restriction, including without limitation the rights
 ** to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 ** copies of the Software, and to permit persons to whom the Software is
 ** furnished to do so, subject to the following conditions:
 **
 ** The above copyright notice and this permission notice shall be included in
 ** all copies or substantial portions of the Software.
 **
 ** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 ** IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 ** FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 ** COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 ** IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 ** CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 **
 ** Except as contained in this notice, the names of the copyright holders shall
 ** not be used in advertising or otherwise to promote the sale, use or other
 ** dealings in this Software without prior written authorization from said
 ** copyright holders.
 **
 ******************************************************************************
 *****************************************************************************/
/* $XFree86: xc/lib/Xp/XpScreens.c,v 1.3 2000/09/26 15:56:58 tsi Exp $ */

#define NEED_REPLIES

#include "Printstr.h"
#include "Xlibint.h"
#include "XpExtUtil.h"


Screen **
XpQueryScreens (
    Display     *dpy,
    int         *list_count             /* return value */
)
{
    xPrintQueryScreensReq     *req;
    xPrintQueryScreensReply   rep;

    /* For decoding the variable portion of Reply */
    CARD32	rootWindow;

    /* For converting root winID to corresponding ScreenPtr */
    Screen **scr_list;
    Screen *checkScr;
    int    i,j;

    XExtDisplayInfo *info = (XExtDisplayInfo *) xp_find_display (dpy);


    if (XpCheckExtInit(dpy, XP_DONT_CHECK) == -1)
        return ( (Screen **) NULL ); /* No such extension */

    LockDisplay (dpy);

    GetReq(PrintQueryScreens,req);
    req->reqType = info->codes->major_opcode;
    req->printReqType = X_PrintQueryScreens;

    if (! _XReply (dpy, (xReply *) &rep, 0, xFalse)) {
        UnlockDisplay(dpy);
        SyncHandle();
        return ( (Screen **) NULL ); /* error */
    }

    *list_count = rep.listCount;

    if (*list_count) {
	scr_list = (Screen **)
		   Xmalloc( (unsigned) (sizeof(Screen *) * *list_count) );

	if (!scr_list) {
            UnlockDisplay(dpy);
            SyncHandle();
            return ( (Screen **) NULL ); /* malloc error */
	}

	for ( i = 0; i < *list_count; i++ ) {
	    /*
	     * Pull printer length and then name.
	     */
	    _XRead32 (dpy, (long *) &rootWindow, (long) sizeof(rootWindow) );

	    for ( j = 0; j < XScreenCount(dpy); j++ ) {
		checkScr = XScreenOfDisplay(dpy, j);
		if ( XRootWindowOfScreen(checkScr) == (Window) rootWindow  ) {
		    scr_list[i] = checkScr;
		    break;
		}
	    }
	}
    }
    else {
	scr_list = (Screen **) NULL;
    }

    UnlockDisplay(dpy);
    SyncHandle();

    return ( scr_list );
}

