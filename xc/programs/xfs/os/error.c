/* $TOG: error.c /main/10 1998/02/11 10:06:39 kaleb $ */
/*
 * error message handling
 */
/*
Copyright 1994, 1998  The Open Group

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
 * Copyright 1991 Network Computing Devices;
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
 * NETWORK COMPUTING DEVICES, DIGITAL DISCLAIM ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS.  IN NO EVENT SHALL NETWORK COMPUTING DEVICES,
 * OR DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
 * ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF
 * THIS SOFTWARE.
 */
/* $XFree86: xc/programs/xfs/os/error.c,v 1.6 2000/11/30 23:30:10 dawes Exp $ */

#include	<stdio.h>
#include	<stdarg.h>
#include	<X11/Xos.h>

#ifndef X_NOT_POSIX
#ifdef _POSIX_SOURCE
#include <limits.h>
#else
#define _POSIX_SOURCE
#include <limits.h>
#undef _POSIX_SOURCE
#endif
#endif
#ifndef PATH_MAX
#include <sys/param.h>
#ifndef PATH_MAX
#ifdef MAXPATHLEN
#define PATH_MAX MAXPATHLEN
#else
#define PATH_MAX 1024
#endif
#endif
#endif

#ifdef USE_SYSLOG
#include	<syslog.h>
#endif

#include	<errno.h>

#include	"misc.h"

extern char *progname;

Bool        UseSyslog;
#ifdef USE_SYSLOG
Bool        log_open = FALSE;
#endif
char        ErrorFile[PATH_MAX];

static void
abort_server(void)
{
    fflush(stderr);

#ifdef SABER
    saber_stop();
#else
    abort();
#endif
}

void
InitErrors(void)
{
    int         i;

#ifdef USE_SYSLOG
    if (UseSyslog && !log_open) {
	openlog("Font Server", LOG_PID, LOG_LOCAL0);
	log_open = TRUE;
	return;
    }
#endif

    if (ErrorFile[0]) {
	i = creat(ErrorFile, 0666);
	if (i != -1) {
	    dup2(i, 2);
	    close(i);
	} else {
	    ErrorF("Can't open error file \"%s\"\n", ErrorFile);
	}
    }
}

void
CloseErrors(void)
{
#ifdef USE_SYSLOG
    if (UseSyslog) {
	closelog();
	log_open = FALSE;
	return;
    }
#endif
}

void
Error(char *str)
{
#ifdef USE_SYSLOG
    if (UseSyslog) {
	syslog(LOG_ERR, "%s: %s", str, strerror(errno));
	return;
    }
#endif
    perror(str);
}

/*
 * used for informational messages
 */
/* VARARGS1 */
void
NoticeF(char *f, ...)
{
    /* XXX should Notices just be ignored if not using syslog? */
    va_list args;
    va_start(args, f);
#ifdef USE_SYSLOG
    if (UseSyslog) {
	vsyslog(LOG_NOTICE, f, args);
	return;
    }
#endif
    fprintf(stderr, "%s notice: ", progname);
    vfprintf(stderr, f, args);
    va_end(args);
}

/*
 * used for non-fatal error messages
 */
/* VARARGS1 */
void
ErrorF(char * f, ...)
{
    va_list args;
    va_start(args, f);
#ifdef USE_SYSLOG
    if (UseSyslog) {
	vsyslog(LOG_NOTICE, f, args);
	return;
    }
#endif
    fprintf(stderr, "%s error: ", progname);
    vfprintf(stderr, f, args);
    va_end(args);
}

/* VARARGS1 */
void
FatalError(char * f, ...)
{
    va_list args;
    va_start(args, f);
    fprintf(stderr, "%s error: ", progname);
    vfprintf(stderr, f, args);
    va_end(args);
    abort_server();
    /* NOTREACHED */
}
