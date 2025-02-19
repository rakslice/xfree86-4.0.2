/* $XConsortium: imLcIc.c /main/6 1996/10/22 14:24:42 kaleb $ */
/******************************************************************

                Copyright 1992,1993, 1994 by FUJITSU LIMITED

Permission to use, copy, modify, distribute, and sell this software
and its documentation for any purpose is hereby granted without fee,
provided that the above copyright notice appear in all copies and
that both that copyright notice and this permission notice appear
in supporting documentation, and that the name of FUJITSU LIMITED
not be used in advertising or publicity pertaining to distribution
of the software without specific, written prior permission.
FUJITSU LIMITED makes no representations about the suitability of
this software for any purpose. 
It is provided "as is" without express or implied warranty.

FUJITSU LIMITED DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
EVENT SHALL FUJITSU LIMITED BE LIABLE FOR ANY SPECIAL, INDIRECT OR
CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
PERFORMANCE OF THIS SOFTWARE.

  Author: Takashi Fujiwara     FUJITSU LIMITED 
                               fujiwara@a80.tech.yk.fujitsu.co.jp

******************************************************************/
/* $XFree86: xc/lib/X11/imLcIc.c,v 1.3 2000/11/28 18:49:36 dawes Exp $ */

#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/Xmd.h>
#include "Xlibint.h"
#include "Xlcint.h"
#include "Ximint.h"

Private void
_XimLocalUnSetFocus(xic)
    XIC	 xic;
{
    Xic  ic = (Xic)xic;
    ((Xim)ic->core.im)->private.local.current_ic = (XIC)NULL;

    if (ic->core.focus_window)
	_XUnregisterFilter(ic->core.im->core.display,
			ic->core.focus_window, _XimLocalFilter, (XPointer)ic);
    return;
}

Private void
_XimLocalDestroyIC(xic)
    XIC	 xic;
{
    Xic	 ic = (Xic)xic;

    if(((Xim)ic->core.im)->private.local.current_ic == (XIC)ic) {
	((Xim)ic->core.im)->private.local.current_ic = (XIC)NULL;
    }
    if (ic->core.focus_window)
	_XUnregisterFilter(ic->core.im->core.display,
			ic->core.focus_window, _XimLocalFilter, (XPointer)ic);
    if(ic->private.local.ic_resources) {
	Xfree(ic->private.local.ic_resources);
	ic->private.local.ic_resources = NULL;
    }
    return;
}

Private void
_XimLocalSetFocus(xic)
    XIC	 xic;
{
    Xic	 ic = (Xic)xic;
    XIC	 current_ic = ((Xim)ic->core.im)->private.local.current_ic;

    if (current_ic == (XIC)ic)
	return;

    if (current_ic != (XIC)NULL) {
	_XimLocalUnSetFocus(current_ic);
    }
    ((Xim)ic->core.im)->private.local.current_ic = (XIC)ic;

    if (ic->core.focus_window)
	_XRegisterFilterByType(ic->core.im->core.display,
			ic->core.focus_window, KeyPress, KeyPress,
			_XimLocalFilter, (XPointer)ic);
    return;
}

Private void
_XimLocalReset(xic)
    XIC	 xic;
{
    Xic	 ic = (Xic)xic;
    ic->private.local.composed = (DefTree *)NULL;
    ic->private.local.context  = ((Xim)ic->core.im)->private.local.top;
}

Private char *
_XimLocalMbReset(xic)
    XIC	 xic;
{
    _XimLocalReset(xic);
    return (char *)NULL;
}

Private wchar_t *
_XimLocalWcReset(xic)
    XIC	 xic;
{
    _XimLocalReset(xic);
    return (wchar_t *)NULL;
}

Private XICMethodsRec Local_ic_methods = {
    _XimLocalDestroyIC, 	/* destroy */
    _XimLocalSetFocus,  	/* set_focus */
    _XimLocalUnSetFocus,	/* unset_focus */
    _XimLocalSetICValues,	/* set_values */
    _XimLocalGetICValues,	/* get_values */
    _XimLocalMbReset,		/* mb_reset */
    _XimLocalWcReset,		/* wc_reset */
    _XimLocalMbReset,		/* utf8_reset */
    _XimLocalMbLookupString,	/* mb_lookup_string */
    _XimLocalWcLookupString,	/* wc_lookup_string */
    _XimLocalUtf8LookupString	/* utf8_lookup_string */
};

Public XIC
_XimLocalCreateIC(im, values)
    XIM			 im;
    XIMArg		*values;
{
    Xic			 ic;
    XimDefICValues	 ic_values;
    XIMResourceList	 res;
    unsigned int	 num;
    int			 len;

    if((ic = (Xic)Xmalloc(sizeof(XicRec))) == (Xic)NULL) {
	return ((XIC)NULL);
    }
    bzero((char *)ic, sizeof(XicRec));

    ic->methods = &Local_ic_methods;
    ic->core.im = im;
    ic->private.local.context   = ((Xim)im)->private.local.top;
    ic->private.local.composed  = (DefTree *)NULL;

    num = im->core.ic_num_resources;
    len = sizeof(XIMResource) * num;
    if((res = (XIMResourceList)Xmalloc(len)) == (XIMResourceList)NULL) {
	goto Set_Error;
    }
    (void)memcpy((char *)res, (char *)im->core.ic_resources, len);
    ic->private.local.ic_resources     = res;
    ic->private.local.ic_num_resources = num;

    bzero((char *)&ic_values, sizeof(XimDefICValues));
    if(_XimCheckLocalInputStyle(ic, (XPointer)&ic_values, values,
				 im->core.styles, res, num) == False) {
	goto Set_Error;
    }

    _XimSetICMode(res, num, ic_values.input_style);

    if(_XimSetICValueData(ic, (XPointer)&ic_values,
			ic->private.local.ic_resources,
			ic->private.local.ic_num_resources,
			values, XIM_CREATEIC, True)) {
	goto Set_Error;
    }
    ic_values.filter_events = KeyPressMask;
    _XimSetCurrentICValues(ic, &ic_values);
    if(_XimSetICDefaults(ic, (XPointer)&ic_values,
				XIM_SETICDEFAULTS, res, num) == False) {
	goto Set_Error;
    }
    _XimSetCurrentICValues(ic, &ic_values);

    return((XIC)ic);

Set_Error :
    if (ic->private.local.ic_resources) {
	Xfree(ic->private.local.ic_resources);
	ic->private.local.ic_resources = NULL;
    }
    Xfree(ic);
    return((XIC)NULL);
}
