/* $TOG: imInt.c /main/5 1998/05/30 21:11:16 kaleb $ */
/******************************************************************

           Copyright 1992, 1993, 1994 by FUJITSU LIMITED

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
/* $XFree86: xc/lib/X11/imInt.c,v 3.8 2000/06/13 02:28:28 dawes Exp $ */

#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xmd.h>
#include "Xlibint.h"
#include "Xlcint.h"
#include "Ximint.h"
#include "XimImSw.h"

Private Xim 		*_XimCurrentIMlist  = (Xim *)NULL;
Private int		 _XimCurrentIMcount = 0;

Private Bool
#if NeedFunctionPrototypes
_XimSetIMStructureList(
    Xim		  im)
#else
_XimSetIMStructureList(im)
    Xim		  im;
#endif
{
    register int  i;
    Xim		 *xim;

    if(!(_XimCurrentIMlist)) {
	if(!(_XimCurrentIMlist = (Xim *)Xmalloc(sizeof(Xim))))
	    return False;
	_XimCurrentIMlist[0] = im;
	_XimCurrentIMcount   = 1;
    }
    else {
	for(i = 0; i < _XimCurrentIMcount; i++) {
	    if(!( _XimCurrentIMlist[i])) {
		_XimCurrentIMlist[i] = im;
		break;
	    }
	}
	if(i >= _XimCurrentIMcount) {
	    if(!(xim = (Xim *)Xrealloc(_XimCurrentIMlist, 
					 ((i + 1) * sizeof(Xim)))))
		return False;
	    _XimCurrentIMlist			  = xim;
	    _XimCurrentIMlist[_XimCurrentIMcount] = im;
	    _XimCurrentIMcount++;
	}
    }
    return True;
}

Public void
_XimDestroyIMStructureList(im)
    Xim		  im;
{
    register int  i;

    for(i = 0; i < _XimCurrentIMcount; i++) {
	if(_XimCurrentIMlist[i] == im) {
	    _XimCurrentIMlist[i] = NULL;
	    break;
	}
    }
    return;
}

Public void
_XimServerDestroy(im_2_destroy)
    Xim		  im_2_destroy;
{
    register int  i;
    Xim		  im;
    XIC		  ic;

    for(i = 0; i < _XimCurrentIMcount; i++) {
	if(!(im = _XimCurrentIMlist[i]))
	    continue;
	/*
	 * Only continue if this im is the one to be destroyed.
	 */
	if (im != im_2_destroy)
	    continue;

	if (im->core.destroy_callback.callback)
	    (*im->core.destroy_callback.callback)((XIM)im,
			im->core.destroy_callback.client_data, NULL);
	for (ic = im->core.ic_chain; ic; ic = ic->core.next) {
	    if (ic->core.destroy_callback.callback) {
		(*ic->core.destroy_callback.callback)(ic,
			ic->core.destroy_callback.client_data, NULL);
	    }
	}
	_XimResetIMInstantiateCallback( im );
	(void)im->methods->close((XIM)im);
	Xfree(im);
	_XimCurrentIMlist[i] = NULL;
	return;
    }
}

#ifdef XIM_CONNECTABLE
Public void
_XimServerReconectableDestroy()
{
    register int  i;
    Xim		  im;
    XIC		  ic;

    for(i = 0; i < _XimCurrentIMcount; i++) {
	if(!(im = _XimCurrentIMlist[i]))
	    continue;

	if (im->core.destroy_callback.callback)
	    (*im->core.destroy_callback.callback)(im,
			im->core.destroy_callback.client_data, NULL);
	for (ic = im->core.ic_chain; ic; ic = ic->core.next) {
	    if (ic->core.destroy_callback.callback) {
		(*ic->core.destroy_callback.callback)(ic,
			ic->core.destroy_callback.client_data, NULL);
	    }
	}
	_XimResetIMInstantiateCallback( im );
	(void)im->methods->close((XIM)im);
    }
    return;
}
#endif /* XIM_CONNECTABLE */

Private char	*
#if NeedFunctionPrototypes
_XimStrstr(
    register char	*src,
    register char	*dest)
#else
_XimStrstr(src, dest)
    register char	*src, *dest;
#endif
{
    int			 len;
    
    len = strlen(dest);
    while ((src = strchr(src, *dest)) != 0) {
	if(!strncmp(src, dest, len))
	    return src;
	src++;
    }
    return NULL;
}

Private char *
#if NeedFunctionPrototypes
_XimMakeImName(
    XLCd	   lcd)
#else
_XimMakeImName(lcd)
    XLCd	   lcd;
#endif
{
    char* begin = NULL;
    char* end = NULL;
    char* ret = NULL;
    char* ximmodifier = XIMMODIFIER;

    if(lcd->core->modifiers != NULL && *lcd->core->modifiers != '\0') {
	begin = _XimStrstr(lcd->core->modifiers, ximmodifier);
	if (begin != NULL) {
	    end = begin += strlen(ximmodifier);
	    while (*end && *end != '@')
		end++;
	    ret = Xmalloc(end - begin + 1);
	    if (ret != NULL) {
		if (begin != NULL && end != NULL) {
		    (void)strncpy(ret, begin, end - begin);
		    ret[end - begin] = '\0';
		}
	    }
	    return ret;
	}
    }

    /* else return an empty string */
    ret = Xmalloc(1);
    if (ret != NULL) {
	ret[0] = '\0';
    }
    return ret;
}

Private XIM
#if NeedFunctionPrototypes
_XimOpenIM(
    XLCd		 lcd,
    Display		*dpy,
    XrmDatabase		 rdb,
    char		*res_name,
    char		*res_class)
#else
_XimOpenIM(lcd, dpy, rdb, res_name, res_class)
    XLCd		 lcd;
    Display		*dpy;
    XrmDatabase		 rdb;
    char		*res_name, *res_class;
#endif
{
    Xim			 im;
    register int	 i;
    
    if (!(im = (Xim)Xmalloc(sizeof(XimRec))))
	return (XIM)NULL;
    bzero(im, sizeof(XimRec));

    im->core.lcd       = lcd;
    im->core.ic_chain  = (XIC)NULL;
    im->core.display   = dpy;
    im->core.rdb       = rdb;
    im->core.res_name  = NULL;
    im->core.res_class = NULL;
    if((res_name != NULL) && (*res_name != '\0')){
	if(!(im->core.res_name  = (char *)Xmalloc(strlen(res_name)+1)))
	    goto Error1;
	strcpy(im->core.res_name,res_name);
    }
    if((res_class != NULL) && (*res_class != '\0')){
	if(!(im->core.res_class = (char *)Xmalloc(strlen(res_class)+1)))
	    goto Error2;
	strcpy(im->core.res_class,res_class);
    }
    if(!(im->core.im_name = _XimMakeImName(lcd)))
	goto Error3;

    for(i= 0; ; i++) {
	if(_XimImSportRec[i].checkprocessing(im)) {
	    if(!(_XimImSportRec[i].im_open(im)))
		goto Error4;
	    if(!_XimSetIMStructureList(im))
		goto Error4;
	    return (XIM)im;
	}
    }

Error4 :
    _XimImSportRec[i].im_free(im);
    Xfree(im);
    return NULL;
Error3 :
    if(im->core.im_name)
	Xfree(im->core.im_name);
Error2:
    if(im->core.res_class)
	Xfree(im->core.res_class);
Error1:
    if(im->core.res_name)
	Xfree(im->core.res_name);
    Xfree(im);
    return NULL;
}

Public Bool
_XInitIM(lcd)
    XLCd	 lcd;
{
    if(lcd == (XLCd)NULL)
	return False;
    lcd->methods->open_im = _XimOpenIM;
    lcd->methods->register_callback = _XimRegisterIMInstantiateCallback;
    lcd->methods->unregister_callback = _XimUnRegisterIMInstantiateCallback;
    return True;
}
