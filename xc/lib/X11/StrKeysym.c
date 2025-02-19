/* $TOG: StrKeysym.c /main/18 1998/02/06 17:54:40 kaleb $ */
/*

Copyright 1985, 1987, 1990, 1998  The Open Group

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
/* $XFree86: xc/lib/X11/StrKeysym.c,v 3.3 2000/12/02 01:16:03 dawes Exp $ */

#include "Xlibint.h"
#include <X11/Xresource.h>
#include <X11/keysymdef.h>
#ifdef X_NOT_STDC_ENV
extern char *getenv();
#endif

extern XrmQuark _XrmInternalStringToQuark();

#ifdef __STDC__
#define Const const
#else
#define Const /**/
#endif

typedef unsigned long Signature;

#define NEEDKTABLE
#include "ks_tables.h"

#ifndef KEYSYMDB
#define KEYSYMDB "/usr/lib/X11/XKeysymDB"
#endif

static Bool initialized;
static XrmDatabase keysymdb;
static XrmQuark Qkeysym[2];

XrmDatabase
_XInitKeysymDB()
{
    if (!initialized)
    {
	char *dbname;

	XrmInitialize();
	/* use and name of this env var is not part of the standard */
	/* implementation-dependent feature */
	dbname = getenv("XKEYSYMDB");
	if (!dbname)
	    dbname = KEYSYMDB;
	keysymdb = XrmGetFileDatabase(dbname);
	if (keysymdb)
	    Qkeysym[0] = XrmStringToQuark("Keysym");
	initialized = True;
    }
    return keysymdb;
}

#if NeedFunctionPrototypes
KeySym XStringToKeysym(s)
    _Xconst char *s;
#else
KeySym XStringToKeysym(s)
    char *s;
#endif
{
    register int i, n;
    int h;
    register Signature sig = 0;
    register Const char *p = s;
    register int c;
    register int idx;
    Const unsigned char *entry;
    unsigned char sig1, sig2;
    KeySym val;

    while ((c = *p++))
	sig = (sig << 1) + c;
    i = sig % KTABLESIZE;
    h = i + 1;
    sig1 = (sig >> 8) & 0xff;
    sig2 = sig & 0xff;
    n = KMAXHASH;
    while ((idx = hashString[i]))
    {
	entry = &_XkeyTable[idx];
	if ((entry[0] == sig1) && (entry[1] == sig2) &&
	    !strcmp(s, (char *)entry + 4))
	{
	    val = (entry[2] << 8) | entry[3];
	    if (!val)
		val = XK_VoidSymbol;
	    return val;
	}
	if (!--n)
	    break;
	i += h;
	if (i >= KTABLESIZE)
	    i -= KTABLESIZE;
    }

    if (!initialized)
	(void)_XInitKeysymDB();
    if (keysymdb)
    {
	XrmValue result;
	XrmRepresentation from_type;
	char c;
	XrmQuark names[2];

	names[0] = _XrmInternalStringToQuark(s, p - s - 1, sig, False);
	names[1] = NULLQUARK;
	(void)XrmQGetResource(keysymdb, names, Qkeysym, &from_type, &result);
	if (result.addr && (result.size > 1))
	{
	    val = 0;
	    for (i = 0; i < result.size - 1; i++)
	    {
		c = ((char *)result.addr)[i];
		if ('0' <= c && c <= '9') val = (val<<4)+c-'0';
		else if ('a' <= c && c <= 'f') val = (val<<4)+c-'a'+10;
		else if ('A' <= c && c <= 'F') val = (val<<4)+c-'A'+10;
		else return NoSymbol;
	    }
	    return val;
	}
    }

    if (*s == 'U') {
    	val = 0;
        for (p = &s[1]; *p; p++) {
            c = *p;
	    if ('0' <= c && c <= '9') val = (val<<4)+c-'0';
	    else if ('a' <= c && c <= 'f') val = (val<<4)+c-'a'+10;
	    else if ('A' <= c && c <= 'F') val = (val<<4)+c-'A'+10;
	    else return NoSymbol;

	}
	if (val >= 0x01000000)
	    return NoSymbol;
        return val | 0x01000000;
    }
    return (NoSymbol);
}
