/* $TOG: GetPntMap.c /main/14 1998/05/30 08:06:05 kaleb $ */
/*

Copyright 1986, 1998  The Open Group

All Rights Reserved.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from The Open Group.

*/

/* $XFree86: xc/lib/X11/GetPntMap.c,v 1.4 2000/09/26 15:56:51 tsi Exp $ */

#define NEED_REPLIES
#include "Xlibint.h"

#ifdef MIN		/* some systems define this in <sys/param.h> */
#undef MIN
#endif
#define MIN(a, b) ((a) < (b) ? (a) : (b))

int XGetPointerMapping (dpy, map, nmaps)
    register Display *dpy;
    unsigned char *map;	/* RETURN */
    int nmaps;

{
    unsigned char mapping[256];	/* known fixed size */
    long nbytes, remainder = 0;
    xGetPointerMappingReply rep;
    register xReq *req;

    LockDisplay(dpy);
    GetEmptyReq(GetPointerMapping, req);
    if (! _XReply(dpy, (xReply *)&rep, 0, xFalse)) {
	UnlockDisplay(dpy);
	SyncHandle();
	return 0;
    }

    nbytes = (long)rep.length << 2;

    /* Don't count on the server returning a valid value */
    if (nbytes > sizeof mapping) {
	remainder = nbytes - sizeof mapping;
	nbytes = sizeof mapping;
    }
    _XRead (dpy, (char *)mapping, nbytes);
    /* don't return more data than the user asked for. */
    if (rep.nElts) {
	    memcpy ((char *) map, (char *) mapping, 
		MIN((int)rep.nElts, nmaps) );
	}

    if (remainder) 
	_XEatData(dpy, (unsigned long)remainder);

    UnlockDisplay(dpy);
    SyncHandle();
    return ((int) rep.nElts);
}

#if NeedFunctionPrototypes
KeySym *XGetKeyboardMapping (Display *dpy,
#if NeedWidePrototypes
			     unsigned int first_keycode,
#else
			     KeyCode first_keycode,
#endif
			     int count,
			     int *keysyms_per_keycode)
#else
KeySym *XGetKeyboardMapping (dpy, first_keycode, count, keysyms_per_keycode)
    register Display *dpy;
    KeyCode first_keycode;
    int count;
    int *keysyms_per_keycode;		/* RETURN */
#endif
{
    long nbytes;
    unsigned long nkeysyms;
    register KeySym *mapping = NULL;
    xGetKeyboardMappingReply rep;
    register xGetKeyboardMappingReq *req;

    LockDisplay(dpy);
    GetReq(GetKeyboardMapping, req);
    req->firstKeyCode = first_keycode;
    req->count = count;
    if (! _XReply(dpy, (xReply *)&rep, 0, xFalse)) {
	UnlockDisplay(dpy);
	SyncHandle();
	return (KeySym *) NULL;
    }

    nkeysyms = (unsigned long) rep.length;
    if (nkeysyms > 0) {
	nbytes = nkeysyms * sizeof (KeySym);
	mapping = (KeySym *) Xmalloc ((unsigned) nbytes);
	nbytes = nkeysyms << 2;
	if (! mapping) {
	    _XEatData(dpy, (unsigned long) nbytes);
	    UnlockDisplay(dpy);
	    SyncHandle();
	    return (KeySym *) NULL;
	}
	_XRead32 (dpy, (long *) mapping, nbytes);
    }
    *keysyms_per_keycode = rep.keySymsPerKeyCode;
    UnlockDisplay(dpy);
    SyncHandle();
    return (mapping);
}

