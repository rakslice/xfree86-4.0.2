/* $TOG: utils.c /main/21 1998/02/11 10:07:08 kaleb $ */
/*
 * misc os utilities
 */
/*
 
Copyright 1990, 1991, 1998  The Open Group

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

 * Copyright 1990, 1991 Network Computing Devices;
 * Portions Copyright 1987 by Digital Equipment Corporation 
 *
 * Permission to use, copy, modify, distribute, and sell this software and
 * its documentation for any purpose is hereby granted without fee, provided
 * that the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the names of Network Computing Devices, or Digital
 * not be used in advertising or publicity pertaining to distribution
 * of the software without specific, written prior permission.
 *
 * NETWORK COMPUTING DEVICES, AND DIGITAL DISCLAIM ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL NETWORK COMPUTING DEVICES,
 * OR DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
 * ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF
 * THIS SOFTWARE.
 */
/* $XFree86: xc/programs/xfs/os/utils.c,v 3.11 2000/11/30 23:30:11 dawes Exp $ */

#include	<stdio.h>
#include	<X11/Xos.h>
#ifndef X_NOT_STDC_ENV
#include	<stdlib.h>
#endif
#include	"misc.h"
#include	"globals.h"
#include	<signal.h>
#ifdef MEMBUG
#include	<util/memleak/memleak.h>
#endif
#include	<sys/wait.h>
#include	<unistd.h>
#include	<pwd.h>
#include	<grp.h>
#include	<sys/types.h>

#ifndef X_NOT_POSIX
#ifdef _POSIX_SOURCE
#include <limits.h>
#else
#define _POSIX_SOURCE
#include <limits.h>
#undef _POSIX_SOURCE
#endif
#endif /* X_NOT_POSIX */
#ifndef PATH_MAX
#include <sys/param.h>
#ifndef PATH_MAX
#ifdef MAXPATHLEN
#define PATH_MAX MAXPATHLEN
#else
#define PATH_MAX 1024
#endif
#endif
#endif /* PATH_MAX */

#if defined(X_NOT_POSIX) && (defined(SYSV) || defined(SVR4))
#define SIGNALS_RESET_WHEN_CAUGHT
#endif

#ifdef X_NOT_STDC_ENV
char *realloc();
#else
#ifdef abs
#undef abs
#endif
#include <stdlib.h>
#endif

extern char *configfilename;
static Bool dropPriv = FALSE; /* whether or not to drop root privileges */
#ifdef DEFAULT_DAEMON
static Bool becomeDaemon = TRUE; /* whether to become a daemon or not */
#else
static Bool becomeDaemon = FALSE; /* whether to become a daemon or not */
#endif
static const char *userId = NULL;
char       *progname;
Bool        CloneSelf;
Bool        portFromCmdline = FALSE;

OldListenRec *OldListen = NULL;
int 	     OldListenCount = 0;

/* ARGSUSED */
SIGVAL
AutoResetServer(int n)
{

#ifdef DEBUG
    fprintf(stderr, "got a reset signal\n");
#endif

    dispatchException |= DE_RESET;
    isItTimeToYield = TRUE;

#ifdef SIGNALS_RESET_WHEN_CAUGHT
    signal(SIGHUP, AutoResetServer);
#endif
}

/* ARGSUSED */
SIGVAL
GiveUp(int n)
{

#ifdef DEBUG
    fprintf(stderr, "got a TERM signal\n");
#endif

    dispatchException |= DE_TERMINATE;
    isItTimeToYield = TRUE;
}

/* ARGSUSED */
SIGVAL
ServerReconfig(int n)
{

#ifdef DEBUG
    fprintf(stderr, "got a re-config signal\n");
#endif

    dispatchException |= DE_RECONFIG;
    isItTimeToYield = TRUE;

#ifdef SIGNALS_RESET_WHEN_CAUGHT
    signal(SIGUSR1, ServerReconfig);
#endif
}

/* ARGSUSED */
SIGVAL
ServerCacheFlush(int n)
{

#ifdef DEBUG
    fprintf(stderr, "got a flush signal\n");
#endif

    dispatchException |= DE_FLUSH;
    isItTimeToYield = TRUE;

#ifdef SIGNALS_RESET_WHEN_CAUGHT
    signal(SIGUSR2, ServerCacheFlush);
#endif
}

/* ARGSUSED */
SIGVAL
CleanupChild(int n)
{

#ifdef DEBUG
    fprintf(stderr, "got a child signal\n");
#endif

    wait(NULL);

#ifdef SIGNALS_RESET_WHEN_CAUGHT
    signal(SIGCHLD, CleanupChild);
#endif
}

long
GetTimeInMillis(void)
{
    struct timeval tp;

    X_GETTIMEOFDAY(&tp);
    return ((tp.tv_sec * 1000) + (tp.tv_usec / 1000));
}

static void
usage(void)
{
    fprintf(stderr, "usage: %s [-config config_file] [-port tcp_port] [-droppriv] [-daemon] [-nodaemon] [-user user_name]\n",
	    progname);
    exit(1);
}

void
OsInitAllocator (void)
{
#ifdef MEMBUG
    CheckMemory ();
#endif
}


/*
 * The '-ls' option is used for cloning the font server.
 *
 * We expect a single string of the form...
 *
 *     transport_id/fd/portnum[,transport_id/fd/portnum]...
 *
 * [] denotes optional and ... denotes repitition.
 *
 * The string must be _exactly_ in the above format.  Since this is
 * an internal option used by the font server, it's ok to be strict
 * about the format of the string.
 */

void
ProcessLSoption (char *str)
{
    char *ptr = str;
    char *slash;
    char number[20];
    int count = 0;
    int len, i;

    while (*ptr != '\0')
    {
	if (*ptr == ',')
	    count++;
	ptr++;
    }
  
    OldListenCount = count + 1;
    OldListen = (OldListenRec *) malloc (
	OldListenCount * sizeof (OldListenRec));

    ptr = str;

    for (i = 0; i < OldListenCount; i++)
    {
	slash = (char *) strchr (ptr, '/');
	len = slash - ptr;
	strncpy (number, ptr, len);
	number[len] = '\0';
	OldListen[i].trans_id = atoi (number);

	ptr = slash + 1;

	slash = (char *) strchr (ptr, '/');
	len = slash - ptr;
	strncpy (number, ptr, len);
	number[len] = '\0';
	OldListen[i].fd = atoi (number);

	ptr = slash + 1;

	if (i == OldListenCount - 1)
	    OldListen[i].portnum = atoi (ptr);
	else
	{
	    char *comma = (char *) strchr (ptr, ',');

	    len = comma - ptr;
	    strncpy (number, ptr, len);
	    number[len] = '\0';
	    OldListen[i].portnum = atoi (number);

	    ptr = comma + 1;
	}
    }
}



/* ARGSUSED */
void
ProcessCmdLine(int argc, char **argv)
{
    int         i;

    progname = argv[0];
    for (i = 1; i < argc; i++) {
	if (!strcmp(argv[i], "-port")) {
	    if (argv[i + 1]) {
		ListenPort = atoi(argv[++i]);
		portFromCmdline = TRUE;
	    } else
		usage();
	} else if (!strcmp(argv[i], "-ls")) {
	    if (argv[i + 1])
		ProcessLSoption (argv[++i]);
	    else
		usage();
	} else if (!strcmp(argv[i], "-droppriv")) {
	        dropPriv = TRUE;
	} else if (!strcmp(argv[i], "-daemon")) {
	        becomeDaemon = TRUE;
	} else if (!strcmp(argv[i], "-nodaemon")) {
	        becomeDaemon = FALSE;
	} else if (!strcmp(argv[i], "-user")) {
	    if (argv[i + 1])
		userId = argv[++i];
	    else
		usage();
	} else if (!strcmp(argv[i], "-cf") || !strcmp(argv[i], "-config")) {
	    if (argv[i + 1])
		configfilename = argv[++i];
	    else
		usage();
	}
#ifdef MEMBUG
	else if ( strcmp( argv[i], "-alloc") == 0)
	{
	    extern unsigned long    MemoryFail;
	    if(++i < argc)
	        MemoryFail = atoi(argv[i]);
	    else
		usage ();
	}
#endif
	else
	    usage();
    }
}


#ifndef SPECIAL_MALLOC

unsigned long	Must_have_memory;

#ifdef MEMBUG
#define MEM_FAIL_SCALE	100000
unsigned long	MemoryFail;

#endif

/* FSalloc -- FS's internal memory allocator.  Why does it return unsigned
 * int * instead of the more common char *?  Well, if you read K&R you'll
 * see they say that alloc must return a pointer "suitable for conversion"
 * to whatever type you really want.  In a full-blown generic allocator
 * there's no way to solve the alignment problems without potentially
 * wasting lots of space.  But we have a more limited problem. We know
 * we're only ever returning pointers to structures which will have to
 * be long word aligned.  So we are making a stronger guarantee.  It might
 * have made sense to make FSalloc return char * to conform with people's
 * expectations of malloc, but this makes lint happier.
 */

pointer 
FSalloc (unsigned long amount)
{
    register pointer  ptr;
	
    if ((long)amount < 0)
	return 0;
    if (amount == 0)
	amount++;
    /* aligned extra on long word boundary */
    amount = (amount + 3) & ~3;
#ifdef MEMBUG
    if (!Must_have_memory && MemoryFail &&
	((random() % MEM_FAIL_SCALE) < MemoryFail))
	return 0;
    if (ptr = (pointer)fmalloc(amount))
	return ptr;
#else
    if ((ptr = (pointer)malloc(amount)) != 0)
	return ptr;
#endif
    if (Must_have_memory)
	FatalError("Out of memory\n");
    return 0;
}

/*****************
 * FScalloc
 *****************/

pointer
FScalloc (unsigned long amount)
{
    pointer ret;

    ret = FSalloc (amount);
    if (ret)
	bzero ((char *) ret, (int) amount);
    return ret;
}

/*****************
 * FSrealloc
 *****************/

pointer
FSrealloc (pointer ptr, unsigned long amount)
{
#ifdef MEMBUG
    if (!Must_have_memory && MemoryFail &&
	((random() % MEM_FAIL_SCALE) < MemoryFail))
	return 0;
    ptr = (pointer)frealloc((char *) ptr, amount);
    if (ptr)
	return ptr;
#else
    if ((long)amount <= 0)
    {
	if (ptr && !amount)
	    free(ptr);
	return 0;
    }
    amount = (amount + 3) & ~3;
    if (ptr)
        ptr = (pointer)realloc((char *)ptr, amount);
    else
	ptr = (pointer)malloc(amount);
    if (ptr)
        return ptr;
#endif
    if (Must_have_memory)
	FatalError("Out of memory\n");
    return 0;
}
                    
/*****************
 *  FSfree
 *    calls free 
 *****************/    

void
FSfree(pointer ptr)
{
#ifdef MEMBUG
    if (ptr)
	ffree((char *)ptr); 
#else
    if (ptr)
	free((char *)ptr); 
#endif
}

#endif /* SPECIAL_MALLOC */


void
SetUserId(void)
{
    /* become xfs user (or other specified on command line) if possible */
    if ((geteuid() == 0) && (dropPriv || userId)) {
	const char *user;
	struct passwd *pwent;

	if (!userId)
	    user = "xfs";
	else
	    user = userId;
	pwent = getpwnam(user);
	if (pwent) {
	    if (setgid(pwent->pw_gid)) {
		ErrorF("fatal: couldn't set groupid to xfs user's group\n");
		exit(1);
	    }
	    if (setgroups(0, NULL)) {
		ErrorF("fatal: couldn't drop supplementary groups\n");
		exit(1);
	    }
	    if (initgroups(user, pwent->pw_gid)) {
		ErrorF("fatal: couldn't init supplementary groups\n");
		exit(1);
	    }
	    if (setuid(pwent->pw_uid)) {
		ErrorF("fatal: couldn't set userid to %s user\n", user);
		exit(1);
	    }
	}
    } else if (dropPriv || userId) {
	ErrorF("fatal: -droppriv or -user flag specified, but xfs not run as root\n");
	exit(1);
    }
}


void
SetDaemonState(void)
{
    int oldpid;

    if (becomeDaemon) {
	BecomeOrphan();
	BecomeDaemon();
	if ((oldpid = StorePid())) {
	    if (oldpid == -1)
		ErrorF ("can't create/lock pid file\n");
	    else
		ErrorF ("can't lock pid file, another xfs is running (pid %s)\n",
			oldpid);
	    exit(1);
	}
    }
}
