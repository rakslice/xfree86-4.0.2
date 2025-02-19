/*
 * $XFree86: xc/lib/Xft/xftstr.c,v 1.2 2000/12/14 23:03:57 keithp Exp $
 *
 * Copyright � 2000 Keith Packard, member of The XFree86 Project, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Keith Packard not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Keith Packard makes no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * KEITH PACKARD DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL KEITH PACKARD BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#include <stdlib.h>
#include <ctype.h>
#include "xftint.h"

char *
_XftSaveString (const char *s)
{
    char    *r;

    r = (char *) malloc (strlen (s) + 1);
    if (!r)
	return 0;
    strcpy (r, s);
    return r;
}

const char *
_XftGetInt(const char *ptr, int *val)
{
    if (*ptr == '*') {
	*val = -1;
	ptr++;
    } else
	for (*val = 0; *ptr >= '0' && *ptr <= '9';)
	    *val = *val * 10 + *ptr++ - '0';
    if (*ptr == '-')
	return ptr;
    return (char *) 0;
}

char *
_XftSplitStr (const char *field, char *save)
{
    char    *s = save;
    char    c;

    while (*field)
    {
	if (*field == '-')
	    break;
	c = *field++;
	*save++ = c;
    }
    *save = 0;
    return s;
}

char *
_XftDownStr (const char *field, char *save)
{
    char    *s = save;
    char    c;

    while (*field)
    {
	c = *field++;
	*save++ = c;
    }
    *save = 0;
    return s;
}

const char *
_XftSplitField (const char *field, char *save)
{
    char    c;

    while (*field)
    {
	if (*field == '-' || *field == '=')
	    break;
	c = *field++;
	*save++ = c;
    }
    *save = 0;
    return field;
}

const char *
_XftSplitValue (const char *field, char *save)
{
    char    c;

    while (*field)
    {
	if (*field == '-' || *field == ',')
	    break;
	c = *field++;
	*save++ = c;
    }
    *save = 0;
    if (*field)
	field++;
    return field;
}

int
_XftMatchSymbolic (XftSymbolic *s, int n, const char *name, int def)
{
    while (n--)
    {
	if (!_XftStrCmpIgnoreCase (s->name, name))
	    return s->value;
	s++;
    }
    return def;
}

int
_XftStrCmpIgnoreCase (const char *s1, const char *s2)
{
    char    c1, c2;
    
    for (;;) 
    {
	c1 = *s1++;
	c2 = *s2++;
	if (!c1 || !c2)
	    break;
	if (isupper (c1))
	    c1 = tolower (c1);
	if (isupper (c2))
	    c2 = tolower (c2);
	if (c1 != c2)
	    break;
    }
    return (int) c2 - (int) c1;
}
