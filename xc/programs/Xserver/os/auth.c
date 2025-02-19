/* $TOG: auth.c /main/28 1998/02/09 15:11:49 kaleb $ */
/*

Copyright 1988, 1998  The Open Group

All Rights Reserved.

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall
not be used in advertising or otherwise to promote the sale, use or
other dealings in this Software without prior written authorization
from The Open Group.

*/
/* $XFree86: xc/programs/Xserver/os/auth.c,v 1.5 2000/08/04 16:13:44 eich Exp $ */

/*
 * authorization hooks for the server
 * Author:  Keith Packard, MIT X Consortium
 */

#ifdef K5AUTH
# include   <krb5/krb5.h>
#endif
# include   "X.h"
# include   "Xauth.h"
# include   "misc.h"
# include   "osdep.h"
# include   "dixstruct.h"
# include   <sys/types.h>
# include   <sys/stat.h>
#ifdef XCSECURITY
#define _SECURITY_SERVER
# include   "extensions/security.h"
#endif
#ifdef WIN32
#include "Xw32defs.h"
#endif

struct protocol {
    unsigned short   name_length;
    char    *name;
    AuthAddCFunc	Add;	/* new authorization data */
    AuthCheckFunc	Check;	/* verify client authorization data */
    AuthRstCFunc	Reset;	/* delete all authorization data entries */
    AuthToIDFunc	ToID;	/* convert cookie to ID */
    AuthFromIDFunc	FromID;	/* convert ID to cookie */
    AuthRemCFunc	Remove;	/* remove a specific cookie */
#ifdef XCSECURITY
    AuthGenCFunc	Generate;
#endif
};

static struct protocol   protocols[] = {
{   (unsigned short) 18,    "MIT-MAGIC-COOKIE-1",
		MitAddCookie,	MitCheckCookie,	MitResetCookie,
		MitToID,	MitFromID,	MitRemoveCookie,
#ifdef XCSECURITY
		MitGenerateCookie
#endif
},
#ifdef HASXDMAUTH
{   (unsigned short) 19,    "XDM-AUTHORIZATION-1",
		XdmAddCookie,	XdmCheckCookie,	XdmResetCookie,
		XdmToID,	XdmFromID,	XdmRemoveCookie,
#ifdef XCSECURITY
		NULL
#endif
},
#endif
#ifdef SECURE_RPC
{   (unsigned short) 9,    "SUN-DES-1",
		SecureRPCAdd,	SecureRPCCheck,	SecureRPCReset,
		SecureRPCToID,	SecureRPCFromID,SecureRPCRemove,
#ifdef XCSECURITY
		NULL
#endif
},
#endif
#ifdef K5AUTH
{   (unsigned short) 14, "MIT-KERBEROS-5",
		K5Add, K5Check, K5Reset,
		K5ToID, K5FromID, K5Remove,
#ifdef XCSECURITY
		NULL
#endif
},
#endif
#ifdef XCSECURITY
{   (unsigned short) XSecurityAuthorizationNameLen,
	XSecurityAuthorizationName,
		NULL, AuthSecurityCheck, NULL,
		NULL, NULL, NULL,
		NULL
},
#endif
};

# define NUM_AUTHORIZATION  (sizeof (protocols) /\
			     sizeof (struct protocol))

/*
 * Initialize all classes of authorization by reading the
 * specified authorization file
 */

static char *authorization_file = (char *)NULL;

static Bool ShouldLoadAuth = TRUE;

void
InitAuthorization (char *file_name)
{
    authorization_file = file_name;
}

int
LoadAuthorization (void)
{
    FILE    *f;
    Xauth   *auth;
    int	    i;
    int	    count = 0;
#if !defined(WIN32) && !defined(__EMX__)
    char    *buf;
#endif

    ShouldLoadAuth = FALSE;
    if (!authorization_file)
	return 0;
#if !defined(WIN32) && !defined(__EMX__)
    buf = xalloc (strlen(authorization_file) + 5);
    if (!buf)
	return 0;
    sprintf (buf, "cat %s", authorization_file);
    f = Popen (buf, "r");
    xfree (buf);
#else
    f = fopen (authorization_file, "r");
#endif
    if (!f)
	return 0;

    while ((auth = XauReadAuth (f)) != 0) {
	for (i = 0; i < NUM_AUTHORIZATION; i++) {
	    if (protocols[i].name_length == auth->name_length &&
		memcmp (protocols[i].name, auth->name, (int) auth->name_length) == 0 &&
		protocols[i].Add)
	    {
		++count;
		(*protocols[i].Add) (auth->data_length, auth->data,
					 FakeClientID(0));
	    }
	}
	XauDisposeAuth (auth);
    }

#if !defined(WIN32) && !defined(__EMX__)
    Pclose (f);
#else
    fclose (f);
#endif
    return count;
}

#ifdef XDMCP
/*
 * XdmcpInit calls this function to discover all authorization
 * schemes supported by the display
 */
void
RegisterAuthorizations (void)
{
    int	    i;

    for (i = 0; i < NUM_AUTHORIZATION; i++)
	XdmcpRegisterAuthorization (protocols[i].name,
				    (int)protocols[i].name_length);
}
#endif

XID
CheckAuthorization (
    unsigned int name_length,
    char	*name,
    unsigned int data_length,
    char	*data,
    ClientPtr client,
    char	**reason)	/* failure message.  NULL for default msg */
{
    int	i;
    struct stat buf;
    static time_t lastmod = 0;

    if (!authorization_file || stat(authorization_file, &buf))
    {
	lastmod = 0;
	ShouldLoadAuth = TRUE;	/* stat lost, so force reload */
    }
    else if (buf.st_mtime > lastmod)
    {
	lastmod = buf.st_mtime;
	ShouldLoadAuth = TRUE;
    }
    if (ShouldLoadAuth)
    {
	if (LoadAuthorization())
	    DisableLocalHost(); /* got at least one */
	else
	    EnableLocalHost ();
    }
    if (name_length)
	for (i = 0; i < NUM_AUTHORIZATION; i++) {
	    if (protocols[i].name_length == name_length &&
		memcmp (protocols[i].name, name, (int) name_length) == 0)
	    {
		return (*protocols[i].Check) (data_length, data, client, reason);
	    }
	}
    return (XID) ~0L;
}

void
ResetAuthorization (void)
{
    int	i;

    for (i = 0; i < NUM_AUTHORIZATION; i++)
	if (protocols[i].Reset)
	    (*protocols[i].Reset)();
    ShouldLoadAuth = TRUE;
}

XID
AuthorizationToID (
	unsigned short	name_length,
	char		*name,
	unsigned short	data_length,
	char		*data)
{
    int	i;

    for (i = 0; i < NUM_AUTHORIZATION; i++) {
    	if (protocols[i].name_length == name_length &&
	    memcmp (protocols[i].name, name, (int) name_length) == 0 &&
	    protocols[i].ToID)
    	{
	    return (*protocols[i].ToID) (data_length, data);
    	}
    }
    return (XID) ~0L;
}

int
AuthorizationFromID (
	XID 		id,
	unsigned short	*name_lenp,
	char		**namep,
	unsigned short	*data_lenp,
	char		**datap)
{
    int	i;

    for (i = 0; i < NUM_AUTHORIZATION; i++) {
	if (protocols[i].FromID &&
	    (*protocols[i].FromID) (id, data_lenp, datap)) {
	    *name_lenp = protocols[i].name_length;
	    *namep = protocols[i].name;
	    return 1;
	}
    }
    return 0;
}

int
RemoveAuthorization (
	unsigned short	name_length,
	char		*name,
	unsigned short	data_length,
	char		*data)
{
    int	i;

    for (i = 0; i < NUM_AUTHORIZATION; i++) {
    	if (protocols[i].name_length == name_length &&
	    memcmp (protocols[i].name, name, (int) name_length) == 0 &&
	    protocols[i].Remove)
    	{
	    return (*protocols[i].Remove) (data_length, data);
    	}
    }
    return 0;
}

int
AddAuthorization (unsigned name_length, char *name, unsigned data_length, char *data)
{
    int	i;

    for (i = 0; i < NUM_AUTHORIZATION; i++) {
    	if (protocols[i].name_length == name_length &&
	    memcmp (protocols[i].name, name, (int) name_length) == 0 &&
	    protocols[i].Add)
    	{
	    return (*protocols[i].Add) (data_length, data, FakeClientID(0));
    	}
    }
    return 0;
}

#ifdef XCSECURITY

XID
GenerateAuthorization(
	unsigned name_length,
	char	*name,
	unsigned data_length,
	char	*data,
	unsigned *data_length_return,
	char	**data_return)
{
    int	i;

    for (i = 0; i < NUM_AUTHORIZATION; i++) {
    	if (protocols[i].name_length == name_length &&
	    memcmp (protocols[i].name, name, (int) name_length) == 0 &&
	    protocols[i].Generate)
    	{
	    return (*protocols[i].Generate) (data_length, data,
			FakeClientID(0), data_length_return, data_return);
    	}
    }
    return -1;
}

/* A random number generator that is more unpredictable
   than that shipped with some systems.
   This code is taken from the C standard. */

static unsigned long int next = 1;

static int
xdm_rand(void)
{
    next = next * 1103515245 + 12345;
    return (unsigned int)(next/65536) % 32768;
}

static void
xdm_srand(unsigned int seed)
{
    next = seed;
}

void
GenerateRandomData (int len, char *buf)
{
    static int seed;
    int value;
    int i;

    seed += GetTimeInMillis();
    xdm_srand (seed);
    for (i = 0; i < len; i++)
    {
	value = xdm_rand ();
	buf[i] ^= (value & 0xff00) >> 8;
    }

    /* XXX add getrusage, popen("ps -ale") */
}

#endif /* XCSECURITY */
