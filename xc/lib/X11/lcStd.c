/* $XConsortium: lcStd.c /main/5 1996/10/22 17:22:55 kaleb $ */
/*
 * Copyright 1992, 1993 by TOSHIBA Corp.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of TOSHIBA not be used in advertising
 * or publicity pertaining to distribution of the software without specific,
 * written prior permission. TOSHIBA make no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * TOSHIBA DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
 * ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
 * TOSHIBA BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
 * ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
 * ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 *
 * Author: Katsuhisa Yano	TOSHIBA Corp.
 *			   	mopi@osa.ilab.toshiba.co.jp
 */
/* $XFree86: xc/lib/X11/lcStd.c,v 1.4 2000/11/28 18:49:49 dawes Exp $ */

#include "Xlibint.h"
#include "XlcPubI.h"

int
_Xlcmbtowc(lcd, wstr, str, len)
    XLCd lcd;
    wchar_t *wstr;
    char *str;
    int len;
{
    static XLCd last_lcd = NULL;
    static XlcConv conv = NULL;
    XPointer from, to;
    int from_left, to_left;
    wchar_t tmp_wc;

    if (lcd == NULL) {
	lcd = _XlcCurrentLC();
	if (lcd == NULL)
	    return -1;
    }
    if (str == NULL)
	return XLC_PUBLIC(lcd, is_state_depend);

    if (conv && lcd != last_lcd) {
	_XlcCloseConverter(conv);
	conv = NULL;
    }

    last_lcd = lcd;

    if (conv == NULL) {
	conv = _XlcOpenConverter(lcd, XlcNMultiByte, lcd, XlcNWideChar);
	if (conv == NULL)
	    return -1;
    }

    from = (XPointer) str;
    from_left = len;
    to = (XPointer) (wstr ? wstr : &tmp_wc);
    to_left = 1;

    if (_XlcConvert(conv, &from, &from_left, &to, &to_left, NULL, 0) < 0)
	return -1;

    return (len - from_left);
}

int
_Xlcwctomb(lcd, str, wc)
    XLCd lcd;
    char *str;
    wchar_t wc;
{
    static XLCd last_lcd = NULL;
    static XlcConv conv = NULL;
    XPointer from, to;
    int from_left, to_left, length;

    if (lcd == NULL) {
	lcd = _XlcCurrentLC();
	if (lcd == NULL)
	    return -1;
    }
    if (str == NULL)
	return XLC_PUBLIC(lcd, is_state_depend);

    if (conv && lcd != last_lcd) {
	_XlcCloseConverter(conv);
	conv = NULL;
    }

    last_lcd = lcd;

    if (conv == NULL) {
	conv = _XlcOpenConverter(lcd, XlcNWideChar, lcd, XlcNMultiByte);
	if (conv == NULL)
	    return -1;
    }

    from = (XPointer) &wc;
    from_left = 1;
    to = (XPointer) str;
    length = to_left = XLC_PUBLIC(lcd, mb_cur_max);

    if (_XlcConvert(conv, &from, &from_left, &to, &to_left, NULL, 0) < 0)
	return -1;

    return (length - to_left);
}

int
_Xlcmbstowcs(lcd, wstr, str, len)
    XLCd lcd;
    wchar_t *wstr;
    char *str;
    int len;
{
    XlcConv conv;
    XPointer from, to;
    int from_left, to_left, ret;

    if (lcd == NULL) {
	lcd = _XlcCurrentLC();
	if (lcd == NULL)
	    return -1;
    }
    
    conv = _XlcOpenConverter(lcd, XlcNMultiByte, lcd, XlcNWideChar);
    if (conv == NULL)
	return -1;

    from = (XPointer) str;
    from_left = strlen(str);
    to = (XPointer) wstr;
    to_left = len;

    if (_XlcConvert(conv, &from, &from_left, &to, &to_left, NULL, 0) < 0)
	ret = -1;
    else {
	ret = len - to_left;
	if (wstr && to_left > 0)
	    wstr[ret] = (wchar_t) 0;
    }

    _XlcCloseConverter(conv);

    return ret;
}

int
_Xlcwcstombs(lcd, str, wstr, len)
    XLCd lcd;
    char *str;
    wchar_t *wstr;
    int len;
{
    XlcConv conv;
    XPointer from, to;
    int from_left, to_left, ret;

    if (lcd == NULL) {
	lcd = _XlcCurrentLC();
	if (lcd == NULL)
	    return -1;
    }

    conv = _XlcOpenConverter(lcd, XlcNWideChar, lcd, XlcNMultiByte);
    if (conv == NULL)
	return -1;

    from = (XPointer) wstr;
    from_left = _Xwcslen(wstr);
    to = (XPointer) str;
    to_left = len;

    if (_XlcConvert(conv, &from, &from_left, &to, &to_left, NULL, 0) < 0)
	ret = -1;
    else {
	ret = len - to_left;
	if (str && to_left > 0)
	    str[ret] = '\0';
    }

    _XlcCloseConverter(conv);

    return ret;
}


int
_Xmbtowc(wstr, str, len)
    wchar_t *wstr;
#ifdef ISC
    char const *str;
    size_t len;
#else
    char *str;
    int len;
#endif
{
    return _Xlcmbtowc((XLCd) NULL, wstr, str, len);
}

int
_Xmblen(str, len)
#ifdef ISC
    char const *str;
    size_t len;
#else
    char *str;
    int len;
#endif
{
    return _Xmbtowc((wchar_t *) NULL, str, len);
}

int
_Xwctomb(str, wc)
    char *str;
    wchar_t wc;
{
    return _Xlcwctomb((XLCd) NULL, str, wc);
}

int
_Xmbstowcs(wstr, str, len)
    wchar_t *wstr;
    char *str;
    int len;
{
    return _Xlcmbstowcs((XLCd) NULL, wstr, str, len);
}

int
_Xwcstombs(str, wstr, len)
    char *str;
    wchar_t *wstr;
    int len;
{
    return _Xlcwcstombs((XLCd) NULL, str, wstr, len);
}

wchar_t *
_Xwcscpy(wstr1, wstr2)
    register wchar_t *wstr1, *wstr2;
{
    wchar_t *wstr_tmp = wstr1;

    while ((*wstr1++ = *wstr2++))
	;

    return wstr_tmp;
}

wchar_t *
_Xwcsncpy(wstr1, wstr2, len)
    register wchar_t *wstr1, *wstr2;
    register int len;
{
    wchar_t *wstr_tmp = wstr1;

    while (len-- > 0)
	if (!(*wstr1++ = *wstr2++))
	    break;

    while (len-- > 0)
	*wstr1++ = (wchar_t) 0;

    return wstr_tmp;
}

int
_Xwcslen(wstr)
    register wchar_t *wstr;
{
    register wchar_t *wstr_ptr = wstr;

    while (*wstr_ptr)
	wstr_ptr++;
    
    return wstr_ptr - wstr;
}

int
_Xwcscmp(wstr1, wstr2)
    register wchar_t *wstr1, *wstr2;
{
    for ( ; *wstr1 && *wstr2; wstr1++, wstr2++)
	if (*wstr1 != *wstr2)
	    break;

    return *wstr1 - *wstr2;
}

int
_Xwcsncmp(wstr1, wstr2, len)
    register wchar_t *wstr1, *wstr2;
    register int len;
{
    for ( ; *wstr1 && *wstr2 && len > 0; wstr1++, wstr2++, len--)
	if (*wstr1 != *wstr2)
	    break;

    if (len <= 0)
	return 0;

    return *wstr1 - *wstr2;
}


int
_Xlcmbstoutf8(lcd, ustr, str, len)
    XLCd lcd;
    char *ustr;
    const char *str;
    int len;
{
    XlcConv conv;
    XPointer from, to;
    int from_left, to_left, ret;

    if (lcd == NULL) {
	lcd = _XlcCurrentLC();
	if (lcd == NULL)
	    return -1;
    }

    conv = _XlcOpenConverter(lcd, XlcNMultiByte, lcd, XlcNUtf8String);
    if (conv == NULL)
	return -1;

    from = (XPointer) str;
    from_left = strlen(str);
    to = (XPointer) ustr;
    to_left = len;

    if (_XlcConvert(conv, &from, &from_left, &to, &to_left, NULL, 0) < 0)
	ret = -1;
    else {
	ret = len - to_left;
	if (ustr && to_left > 0)
	    ustr[ret] = '\0';
    }

    _XlcCloseConverter(conv);

    return ret;
}

int
_Xmbstoutf8(ustr, str, len)
    char *ustr;
    const char *str;
    int len;
{
    return _Xlcmbstoutf8((XLCd) NULL, ustr, str, len);
}
