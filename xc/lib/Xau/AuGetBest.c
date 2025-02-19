/* $TOG: AuGetBest.c /main/24 1998/02/06 14:14:47 kaleb $ */

/*

Copyright 1988, 1998  The Open Group

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
/* $XFree86: xc/lib/Xau/AuGetBest.c,v 1.3 2000/06/14 00:16:06 dawes Exp $ */

#include <X11/Xauth.h>
#include <X11/Xos.h>
#ifdef XTHREADS
#include <X11/Xthreads.h>
#endif
#ifdef hpux
#define X_INCLUDE_NETDB_H
#define XOS_USE_NO_LOCKING
#include <X11/Xos_r.h>
#endif

static int
binaryEqual (_Xconst char *a, _Xconst char *b, int len)
{
    while (len--)
	if (*a++ != *b++)
	    return 0;
    return 1;
}

#if NeedFunctionPrototypes
Xauth *
XauGetBestAuthByAddr (
#if NeedWidePrototypes
    unsigned int	family,
    unsigned int	address_length,
#else
    unsigned short	family,
    unsigned short	address_length,
#endif
    _Xconst char*	address,
#if NeedWidePrototypes
    unsigned int	number_length,
#else
    unsigned short	number_length,
#endif
    _Xconst char*	number,
    int			types_length,
    char**		types,
    _Xconst int*	type_lengths)
#else
Xauth *
XauGetBestAuthByAddr (family, address_length, address,
			  number_length, number,
			  types_length, types, type_lengths)
    unsigned short	family;
    unsigned short	address_length;
    char		*address;
    unsigned short	number_length;
    char		*number;
    int			types_length;
    char		**types;
    int			*type_lengths;
#endif
{
    FILE    *auth_file;
    char    *auth_name;
    Xauth   *entry;
    Xauth   *best;
    int	    best_type;
    int	    type;
#ifdef hpux
    char		*fully_qual_address;
    unsigned short	fully_qual_address_length;
#endif

    auth_name = XauFileName ();
    if (!auth_name)
	return 0;
    if (access (auth_name, R_OK) != 0)		/* checks REAL id */
	return 0;
    auth_file = fopen (auth_name, "rb");
    if (!auth_file)
	return 0;

#ifdef hpux
    if (family == FamilyLocal) {
	_Xgethostbynameparams hparams;
	struct hostent *hostp;

	/* make sure we try fully-qualified hostname */
	if ((hostp = _XGethostbyname(address,hparams)) != NULL) {
	    fully_qual_address = hostp->h_name;
	    fully_qual_address_length = strlen(fully_qual_address);
	}
	else
	{
	    fully_qual_address = NULL;
	    fully_qual_address_length = 0;
	}
    }
#endif /* hpux */

    best = 0;
    best_type = types_length;
    for (;;) {
	entry = XauReadAuth (auth_file);
	if (!entry)
	    break;
	/*
	 * Match when:
	 *   either family or entry->family are FamilyWild or
	 *    family and entry->family are the same and
	 *     address and entry->address are the same
	 *  and
	 *   either number or entry->number are empty or
	 *    number and entry->number are the same
	 *  and
	 *   either name or entry->name are empty or
	 *    name and entry->name are the same
	 */

	if ((family == FamilyWild || entry->family == FamilyWild ||
	     (entry->family == family &&
	     ((address_length == entry->address_length &&
	      binaryEqual (entry->address, address, (int)address_length))
#ifdef hpux
	     || (family == FamilyLocal &&
		fully_qual_address_length == entry->address_length &&
	     	binaryEqual (entry->address, fully_qual_address,
		    (int) fully_qual_address_length))
#endif
	    ))) &&
	    (number_length == 0 || entry->number_length == 0 ||
	     (number_length == entry->number_length &&
	      binaryEqual (entry->number, number, (int)number_length))))
	{
	    if (best_type == 0)
	    {
		best = entry;
		break;
	    }
	    for (type = 0; type < best_type; type++)
		if (type_lengths[type] == entry->name_length &&
		    !(strncmp (types[type], entry->name, entry->name_length)))
		{
		    break;
		}
	    if (type < best_type)
	    {
		if (best)
		    XauDisposeAuth (best);
		best = entry;
		best_type = type;
		if (type == 0)
		    break;
		continue;
	    }
	}
	XauDisposeAuth (entry);
    }
    (void) fclose (auth_file);
    return best;
}
