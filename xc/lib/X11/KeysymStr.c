/* $TOG: KeysymStr.c /main/13 1998/02/06 17:38:27 kaleb $ */

/*

Copyright 1990, 1998  The Open Group

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
/* $XFree86: xc/lib/X11/KeysymStr.c,v 3.5 2000/12/06 22:00:42 dawes Exp $ */

#include "Xlibint.h"
#include <X11/Xresource.h>
#include <X11/keysymdef.h>

#include <stdio.h> /* sprintf */

#ifdef __STDC__
#define Const const
#else
#define Const /**/
#endif

typedef unsigned long Signature;

#define NEEDVTABLE
#include "ks_tables.h"

extern XrmDatabase _XInitKeysymDB();
extern Const unsigned char _XkeyTable[];


typedef struct _GRNData {
    char *name;
    XrmRepresentation type;
    XrmValuePtr value;
} GRNData;

#if NeedFunctionPrototypes
/*ARGSUSED*/
static Bool SameValue(
    XrmDatabase*	db,
    XrmBindingList      bindings,
    XrmQuarkList	quarks,
    XrmRepresentation*  type,
    XrmValuePtr		value,
    XPointer		data
)
#else
static Bool SameValue(db, bindings, quarks, type, value, data)
    XrmDatabase		*db;
    XrmBindingList      bindings;
    XrmQuarkList	quarks;
    XrmRepresentation   *type;
    XrmValuePtr		value;
    XPointer		data;
#endif
{
    GRNData *gd = (GRNData *)data;

    if ((*type == gd->type) && (value->size == gd->value->size) &&
	!strncmp((char *)value->addr, (char *)gd->value->addr, value->size))
    {
	gd->name = XrmQuarkToString(*quarks); /* XXX */
	return True;
    }
    return False;
}

char *XKeysymToString(ks)
    KeySym ks;
{
    register int i, n;
    int h;
    register int idx;
    Const unsigned char *entry;
    unsigned char val1, val2;
    XrmDatabase keysymdb;

    if (!ks || (ks & ((unsigned long) ~0x1fffffff)) != 0)
	return ((char *)NULL);
    if (ks == XK_VoidSymbol)
	ks = 0;
    if (ks <= 0xffff)
    {
	val1 = ks >> 8;
	val2 = ks & 0xff;
	i = ks % VTABLESIZE;
	h = i + 1;
	n = VMAXHASH;
	while ((idx = hashKeysym[i]))
	{
	    entry = &_XkeyTable[idx];
	    if ((entry[0] == val1) && (entry[1] == val2))
		return ((char *)entry + 2);
	    if (!--n)
		break;
	    i += h;
	    if (i >= VTABLESIZE)
		i -= VTABLESIZE;
	}
    }

    if ((keysymdb = _XInitKeysymDB()))
    {
	char buf[9];
	XrmValue resval;
	XrmQuark empty = NULLQUARK;
	GRNData data;

	sprintf(buf, "%lX", ks);
	resval.addr = (XPointer)buf;
	resval.size = strlen(buf) + 1;
	data.name = (char *)NULL;
	data.type = XrmPermStringToQuark("String");
	data.value = &resval;
	(void)XrmEnumerateDatabase(keysymdb, &empty, &empty, XrmEnumAllLevels,
				   SameValue, (XPointer)&data);
        if (data.name)
	    return data.name;
    }
    if ((ks & 0xff000000) == 0x01000000){
        KeySym val = ks & 0xffffff;
        char *s;
        int i;
        if (val & 0xff0000)
            i = 10;
        else
            i = 6;
        s = Xmalloc(i);
        if (s == NULL)
            return s;
        i--;
        s[i--] = '\0';
        for (; i; i--){
            val1 = val & 0xf;
            val >>= 4;
            if (val1 < 10)
                s[i] = '0'+ val1;
            else
                s[i] = 'A'+ val1 - 10;
        }
        s[i] = 'U';
        return s; 
    }
    return ((char *) NULL);
}
