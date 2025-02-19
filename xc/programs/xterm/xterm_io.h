/*
 * $XFree86: xc/programs/xterm/xterm_io.h,v 1.1 2000/12/06 10:19:43 dickey Exp $
 */

/*
 * Copyright 2000 by Thomas E. Dickey
 *
 *                         All Rights Reserved
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE ABOVE LISTED COPYRIGHT HOLDER(S) BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name(s) of the above copyright
 * holders shall not be used in advertising or otherwise to promote the
 * sale, use or other dealings in this Software without prior written
 * authorization.
 */

#ifndef	included_xterm_io_h
#define	included_xterm_io_h

#include <xterm.h>

/*
 * System-specific definitions (keep these chunks one-per-system!).
 *
 * FIXME:  some, such as those defining USE_TERMIOS should be moved to xterm.h
 * as they are integrated with the configure script.
 */
#ifdef AMOEBA
#define USE_TERMIOS
#define _POSIX_SOURCE
#endif

#ifdef CSRG_BASED
#define USE_TERMIOS
#endif

#ifdef __CYGWIN__
#define ATT
#define SVR4
#define SYSV
#define USE_SYSV_TERMIO
#endif

#ifdef __EMX__
#define USE_SYSV_TERMIO
#endif

#ifdef __FreeBSD__
#define USE_POSIX_TERMIOS
#endif

#ifdef linux
#define USE_TERMIOS
#endif

#ifdef Lynx
#define USE_SYSV_TERMIO
#endif

#ifdef macII
#undef SYSV				/* pretend to be bsd (sgtty.h) */
#endif /* macII */

#ifdef MINIX
#define USE_SYSV_TERMIO
#define USE_TERMIOS
#endif

#ifdef __MVS__
#define SVR4
#define USE_POSIX_TERMIOS
#endif

#ifdef __QNX__
#define USE_POSIX_TERMIOS
#endif

#if defined(__osf__)
#define USE_POSIX_TERMIOS
#undef SYSV
#endif

/*
 * Indirect system dependencies
 */
#if defined(SVR4) && !defined(__sgi)
#define USE_TERMIOS
#endif

#ifdef SYSV
#define USE_SYSV_TERMIO
#endif

#if defined(USE_POSIX_TERMIOS) && !defined(USE_TERMIOS)
#define USE_TERMIOS
#endif

/*
 * Low-level ioctl, where it is needed or non-conflicting with termio/etc.
 */
#ifdef __QNX__
#include <ioctl.h>
#elif !defined(__CYGWIN__)
#include <sys/ioctl.h>
#endif

/*
 * Terminal I/O includes (termio, termios, sgtty headers).
 */
#if defined(USE_POSIX_TERMIOS)
#include <termios.h>
#elif defined(USE_TERMIOS)
#include <termios.h>
/* this hacked termios support only works on SYSV */
#define USE_ANY_SYSV_TERMIO
#define termio termios
#undef  TCGETA
#define TCGETA TCGETS
#undef  TCSETA
#define TCSETA TCSETS
#undef  TCSETAW
#define TCSETAW TCSETSW
#elif defined(USE_SYSV_TERMIO)
# define USE_ANY_SYSV_TERMIO
# ifdef Lynx
#  include <termio.h>
# else
#  include <sys/termio.h>
# endif
#elif defined(SYSV) || defined(ISC)
# include <sys/termio.h>
#elif !defined(VMS)
# include <sgtty.h>
#endif /* USE_POSIX_TERMIOS */

/*
 * Stream includes, which declare struct winsize or ttysize.
 */
#ifdef SYSV
#ifdef USE_USG_PTYS
#include <sys/stream.h>			/* get typedef used in ptem.h */
#if !defined(SVR4) || defined(SCO325)
#include <sys/ptem.h>			/* get struct winsize */
#endif
#endif /* USE_USG_PTYS */
#elif defined(sun) && !defined(SVR4)
#include <sys/ttycom.h>
#ifdef TIOCSWINSZ
#undef TIOCSSIZE
#endif
#endif /* SYSV */

/*
 * Special cases (structures and definitions that have to be adjusted).
 */
#if defined(__CYGWIN__) && !defined(TIOCSPGRP)
#include <termios.h>
#define TIOCSPGRP (_IOW('t', 118, pid_t))
#endif

#ifdef __EMX__
#define XFREE86_PTY	0x76

#define XTY_TIOCSETA	0x48
#define XTY_TIOCSETAW	0x49
#define XTY_TIOCSETAF	0x4a
#define XTY_TIOCCONS	0x4d
#define XTY_TIOCSWINSZ	0x53
#define XTY_ENADUP	0x5a
#define XTY_TRACE	0x5b
#define XTY_TIOCGETA	0x65
#define XTY_TIOCGWINSZ	0x66
#define PTMS_GETPTY	0x64
#define PTMS_BUFSZ	14

#ifndef NCCS
#define NCCS 11
#endif

#define TIOCSWINSZ	113
#define TIOCGWINSZ	117

struct pt_termios
{
        unsigned short  c_iflag;
        unsigned short  c_oflag;
        unsigned short  c_cflag;
        unsigned short  c_lflag;
        unsigned char   c_cc[NCCS];
        long            _reserved_[4];
};

struct winsize {
	unsigned short	ws_row;		/* rows, in characters */
	unsigned short	ws_col;		/* columns, in characters */
	unsigned short	ws_xpixel;	/* horizontal size, pixels */
	unsigned short	ws_ypixel;	/* vertical size, pixels */
};

extern int ptioctl(int fd, int func, void* data);
#define ioctl ptioctl

#endif /* __EMX__ */

#ifdef ISC
#define TIOCGPGRP TCGETPGRP
#define TIOCSPGRP TCSETPGRP
#endif

#ifdef Lynx
#include <resource.h>
#elif !(defined(SYSV) || defined(linux) || defined(VMS))
#include <sys/resource.h>
#endif

#ifdef macII
#undef FIOCLEX
#undef FIONCLEX
#endif /* macII */

#ifdef MINIX
#define termio termios
#define TCGETA TCGETS
#define TCSETAW TCSETSW
#endif

#ifdef __QNX__
#undef TIOCSLTC			/* <sgtty.h> conflicts with <termios.h> */
#undef TIOCLSET
#endif

#if defined(__sgi) && (OSMAJORVERSION >= 5)
#undef TIOCLSET				/* defined, but not useable */
#endif

#if defined(__GNU__) || defined(__MVS__) || defined(__osf__)
#undef TIOCLSET
#undef TIOCSLTC
#endif

#if defined (__sgi) || (defined(__linux__) && defined(__sparc__))
#undef TIOCLSET /* XXX why is this undef-ed again? */
#endif

#ifdef sun
#include <sys/filio.h>
#endif

#if defined(TIOCSLTC) && ! (defined(linux) || defined(__MVS__) || defined(Lynx) || defined(SVR4))
#define HAS_LTCHARS
#endif

#endif	/* included_xterm_io_h */
