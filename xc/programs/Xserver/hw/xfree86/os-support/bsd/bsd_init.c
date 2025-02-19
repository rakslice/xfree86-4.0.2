/* $XFree86: xc/programs/Xserver/hw/xfree86/os-support/bsd/bsd_init.c,v 3.15 2000/11/14 21:59:24 dawes Exp $ */
/*
 * Copyright 1992 by Rich Murphey <Rich@Rice.edu>
 * Copyright 1993 by David Wexelblat <dwex@goblin.org>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the names of Rich Murphey and David Wexelblat 
 * not be used in advertising or publicity pertaining to distribution of 
 * the software without specific, written prior permission.  Rich Murphey and
 * David Wexelblat make no representations about the suitability of this 
 * software for any purpose.  It is provided "as is" without express or 
 * implied warranty.
 *
 * RICH MURPHEY AND DAVID WEXELBLAT DISCLAIM ALL WARRANTIES WITH REGARD TO 
 * THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND 
 * FITNESS, IN NO EVENT SHALL RICH MURPHEY OR DAVID WEXELBLAT BE LIABLE FOR 
 * ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER 
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF 
 * CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN 
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */
/* $XConsortium: bsd_init.c /main/8 1996/10/23 13:13:05 kaleb $ */

#include "X.h"

#include "compiler.h"

#include "xf86.h"
#include "xf86Priv.h"
#include "xf86_OSlib.h"

static Bool KeepTty = FALSE;
static int devConsoleFd = -1;
static int VTnum = -1;
static int initialVT = -1;

#ifdef PCCONS_SUPPORT
/* Stock 0.1 386bsd pccons console driver interface */
#ifndef __OpenBSD__
#  define PCCONS_CONSOLE_DEV1 "/dev/ttyv0"
#else
#  define PCCONS_CONSOLE_DEV1 "/dev/ttyC0"
#endif
#define PCCONS_CONSOLE_DEV2 "/dev/vga"
#define PCCONS_CONSOLE_MODE O_RDWR|O_NDELAY
#endif

#ifdef SYSCONS_SUPPORT
/* The FreeBSD 1.1 version syscons driver uses /dev/ttyv0 */
#define SYSCONS_CONSOLE_DEV1 "/dev/ttyv0"
#define SYSCONS_CONSOLE_DEV2 "/dev/vga"
#define SYSCONS_CONSOLE_MODE O_RDWR|O_NDELAY
#endif

#ifdef PCVT_SUPPORT
/* Hellmuth Michaelis' pcvt driver */
#ifndef __OpenBSD__
#  define PCVT_CONSOLE_DEV "/dev/ttyv0"
#else
#  define PCVT_CONSOLE_DEV "/dev/ttyC0"
#endif
#define PCVT_CONSOLE_MODE O_RDWR|O_NDELAY
#endif

#ifdef WSCONS_SUPPORT
/* NetBSD's new console driver */
#define WSCONS_PCVT_COMPAT_CONSOLE_DEV "/dev/ttyE0"
#endif

#define CHECK_DRIVER_MSG \
  "Check your kernel's console driver configuration and /dev entries"

static char *supported_drivers[] = {
#ifdef PCCONS_SUPPORT
	"pccons (with X support)",
#endif
#ifdef SYSCONS_SUPPORT
	"syscons",
#endif
#ifdef PCVT_SUPPORT
	"pcvt",
#endif
#ifdef WSCONS_SUPPORT
	"wscons",
#endif
};


/*
 * Functions to probe for the existance of a supported console driver.
 * Any function returns either a valid file descriptor (driver probed
 * succesfully), -1 (driver not found), or uses FatalError() if the
 * driver was found but proved to not support the required mode to run
 * an X server.
 */

typedef int (*xf86ConsOpen_t)(void);

#ifdef PCCONS_SUPPORT
static int xf86OpenPccons(void);
#endif /* PCCONS_SUPPORT */

#ifdef SYSCONS_SUPPORT
static int xf86OpenSyscons(void);
#endif /* SYSCONS_SUPPORT */

#ifdef PCVT_SUPPORT
static int xf86OpenPcvt(void);
#endif /* PCVT_SUPPORT */

#ifdef WSCONS_SUPPORT
static int xf86OpenWScons(void);
#endif

/*
 * The sequence of the driver probes is important; start with the
 * driver that is best distinguishable, and end with the most generic
 * driver.  (Otherwise, pcvt would also probe as syscons, and either
 * pcvt or syscons might succesfully probe as pccons.)
 */
static xf86ConsOpen_t xf86ConsTab[] = {
#ifdef PCVT_SUPPORT
    xf86OpenPcvt,
#endif
#ifdef SYSCONS_SUPPORT
    xf86OpenSyscons,
#endif
#ifdef PCCONS_SUPPORT
    xf86OpenPccons,
#endif
#ifdef WSCONS_SUPPORT
    xf86OpenWScons,
#endif
    (xf86ConsOpen_t)NULL
};


void
xf86OpenConsole()
{
    int i, fd = -1;
    int result;
    xf86ConsOpen_t *driver;
#if defined (SYSCONS_SUPPORT) || defined (PCVT_SUPPORT)
    vtmode_t vtmode;
#endif
    
    if (serverGeneration == 1)
    {
	/* check if we are run with euid==0 */
	if (geteuid() != 0)
	{
	    FatalError("xf86OpenConsole: Server must be suid root\n");
	}

	if (!KeepTty)
	{
	    /*
	     * detaching the controlling tty solves problems of kbd character
	     * loss.  This is not interesting for CO driver, because it is 
	     * exclusive.
	     */
	    setpgrp(0, getpid());
	    if ((i = open("/dev/tty",O_RDWR)) >= 0)
	    {
		ioctl(i,TIOCNOTTY,(char *)0);
		close(i);
	    }
	}

	/* detect which driver we are running on */
	for (driver = xf86ConsTab; *driver; driver++)
	{
	    if ((fd = (*driver)()) >= 0)
		break;
	}

	/* Check that a supported console driver was found */
	if (fd < 0)
	{
	    char cons_drivers[80] = {0, };
	    for (i = 0; i < sizeof(supported_drivers) / sizeof(char *); i++)
	    {
		if (i)
		{
		    strcat(cons_drivers, ", ");
		}
		strcat(cons_drivers, supported_drivers[i]);
	    }
	    FatalError(
		"%s: No console driver found\n\tSupported drivers: %s\n\t%s\n",
		"xf86OpenConsole", cons_drivers, CHECK_DRIVER_MSG);
	}
#if 0 /* stdin is already closed in OsInit() */
	fclose(stdin);
#endif
	xf86Info.consoleFd = fd;
	xf86Info.screenFd = fd;

	switch (xf86Info.consType)
	{
#ifdef PCCONS_SUPPORT
	case PCCONS:
	    if (ioctl (xf86Info.consoleFd, CONSOLE_X_MODE_ON, 0) < 0)
	    {
		FatalError("%s: CONSOLE_X_MODE_ON failed (%s)\n%s\n", 
			   "xf86OpenConsole", strerror(errno),
			   CHECK_DRIVER_MSG);
	    }
	    /*
	     * Hack to prevent keyboard hanging when syslogd closes
	     * /dev/console
	     */
	    if ((devConsoleFd = open("/dev/console", O_WRONLY,0)) < 0)
	    {
		xf86Msg(X_WARNING,
			"xf86OpenConsole: couldn't open /dev/console (%s)\n",
			strerror(errno));
	    }
	    break;
#endif
#if defined (SYSCONS_SUPPORT) || defined (PCVT_SUPPORT)
	case SYSCONS:
	case PCVT:
	    /*
	     * First activate the #1 VT.  This is a hack to allow a server
	     * to be started while another one is active.  There should be
	     * a better way.
	     */
	    if (initialVT != 1) {

		if (ioctl(xf86Info.consoleFd, VT_ACTIVATE, 1) != 0)
		{
		    xf86Msg(X_WARNING,
				"xf86OpenConsole: VT_ACTIVATE failed\n");
		}
		sleep(1);
	    }
	   
	    /*
	     * now get the VT
	     */
	    SYSCALL(result =
		    ioctl(xf86Info.consoleFd, VT_ACTIVATE, xf86Info.vtno));
	    if (result != 0)
	    {
    	        xf86Msg(X_WARNING, "xf86OpenConsole: VT_ACTIVATE failed\n");
	    }
	    SYSCALL(result =
		    ioctl(xf86Info.consoleFd, VT_WAITACTIVE, xf86Info.vtno));
	    if (result != 0)
	    {
	        xf86Msg(X_WARNING, "xf86OpenConsole: VT_WAITACTIVE failed\n");
	    }

	    signal(SIGUSR1, xf86VTRequest);

	    vtmode.mode = VT_PROCESS;
	    vtmode.relsig = SIGUSR1;
	    vtmode.acqsig = SIGUSR1;
	    vtmode.frsig = SIGUSR1;
	    if (ioctl(xf86Info.consoleFd, VT_SETMODE, &vtmode) < 0) 
	    {
	        FatalError("xf86OpenConsole: VT_SETMODE VT_PROCESS failed\n");
	    }
#if !defined(USE_DEV_IO) && !defined(USE_I386_IOPL)
	    if (ioctl(xf86Info.consoleFd, KDENABIO, 0) < 0)
	    {
	        FatalError("xf86OpenConsole: KDENABIO failed (%s)\n",
		           strerror(errno));
	    }
#endif
	    if (ioctl(xf86Info.consoleFd, KDSETMODE, KD_GRAPHICS) < 0)
	    {
	        FatalError("xf86OpenConsole: KDSETMODE KD_GRAPHICS failed\n");
	    }
   	    break; 
#endif /* SYSCONS_SUPPORT || PCVT_SUPPORT */
#ifdef WSCONS_SUPPORT
	case WSCONS:
	    fprintf(stderr, "xf86OpenConsole\n");
	    /* xf86Info.consoleFd = open("/dev/wskbd0", 0); */
   	    break; 
#endif
        }
    }
    else 
    {
	/* serverGeneration != 1 */
#if defined (SYSCONS_SUPPORT) || defined (PCVT_SUPPORT)
    	if (xf86Info.consType == SYSCONS || xf86Info.consType == PCVT)
    	{
	    if (ioctl(xf86Info.consoleFd, VT_ACTIVATE, xf86Info.vtno) != 0)
	    {
	        xf86Msg(X_WARNING, "xf86OpenConsole: VT_ACTIVATE failed\n");
	    }
        }
#endif /* SYSCONS_SUPPORT || PCVT_SUPPORT */
    }
    return;
}


#ifdef PCCONS_SUPPORT

static int
xf86OpenPccons()
{
    int fd = -1;

    if ((fd = open(PCCONS_CONSOLE_DEV1, PCCONS_CONSOLE_MODE, 0))
	>= 0 ||
	(fd = open(PCCONS_CONSOLE_DEV2, PCCONS_CONSOLE_MODE, 0))
	>= 0)
    {
	if (ioctl(fd, CONSOLE_X_MODE_OFF, 0) < 0)
	{
	    FatalError(
		"%s: CONSOLE_X_MODE_OFF failed (%s)\n%s\n%s\n",
		"xf86OpenPccons",
		strerror(errno),
		"Was expecting pccons driver with X support",
		CHECK_DRIVER_MSG);
	}
	xf86Info.consType = PCCONS;
	xf86Msg(X_PROBED, "Using pccons driver with X support\n");
    }
    return fd;
}

#endif /* PCCONS_SUPPORT */

#ifdef SYSCONS_SUPPORT

static int
xf86OpenSyscons()
{
    int fd = -1;
    vtmode_t vtmode;
    char vtname[12];
    struct stat status;
    long syscons_version;
    MessageType from;

    /* Check for syscons */
    if ((fd = open(SYSCONS_CONSOLE_DEV1, SYSCONS_CONSOLE_MODE, 0)) >= 0
	|| (fd = open(SYSCONS_CONSOLE_DEV2, SYSCONS_CONSOLE_MODE, 0)) >= 0)
    {
	if (ioctl(fd, VT_GETMODE, &vtmode) >= 0)
	{
	    /* Get syscons version */
	    if (ioctl(fd, CONS_GETVERS, &syscons_version) < 0)
	    {
		syscons_version = 0;
	    }

	    xf86Info.vtno = VTnum;
	    from = X_CMDLINE;

#ifdef VT_GETACTIVE
	    if (ioctl(fd, VT_GETACTIVE, &initialVT) < 0)
		initialVT = -1;
#endif
	    if (xf86Info.vtno == -1)
	    {
		/*
		 * For old syscons versions (<0x100), VT_OPENQRY returns
		 * the current VT rather than the next free VT.  In this
		 * case, the server gets started on the current VT instead
		 * of the next free VT.
		 */

#if 0
		/* check for the fixed VT_OPENQRY */
		if (syscons_version >= 0x100)
		{
#endif
		    if (ioctl(fd, VT_OPENQRY, &xf86Info.vtno) < 0)
		    {
			/* No free VTs */
			xf86Info.vtno = -1;
		    }
#if 0
		}
#endif

		if (xf86Info.vtno == -1)
		{
		    /*
		     * All VTs are in use.  If initialVT was found, use it.
		     * Otherwise, if stdin is a VT, use that one.
		     * XXX stdin is already closed, so this won't work.
		     */
		    if (initialVT != -1)
		    {
			xf86Info.vtno = initialVT;
		    }
		    else if ((fstat(0, &status) >= 0)
			     && S_ISCHR(status.st_mode)
			     && (ioctl(0, VT_GETMODE, &vtmode) >= 0))
		    {
			/* stdin is a VT */
			xf86Info.vtno = minor(status.st_rdev) + 1;
		    }
		    else
		    {
			if (syscons_version >= 0x100)
			{
			    FatalError("%s: Cannot find a free VT\n",
				       "xf86OpenSyscons");
			}
			/* Should no longer reach here */
			FatalError("%s: %s %s\n\t%s %s\n",
				   "xf86OpenSyscons",
				   "syscons versions prior to 1.0 require",
				   "either the",
				   "server's stdin be a VT",
				   "or the use of the vtxx server option");
		    }
		}
		from = X_PROBED;
	    }

	    close(fd);
#ifndef __OpenBSD__
	    sprintf(vtname, "/dev/ttyv%01x", xf86Info.vtno - 1);
#else 
	    sprintf(vtname, "/dev/ttyC%01x", xf86Info.vtno - 1);
#endif	    
	    if ((fd = open(vtname, SYSCONS_CONSOLE_MODE, 0)) < 0)
	    {
		FatalError("xf86OpenSyscons: Cannot open %s (%s)\n",
			   vtname, strerror(errno));
	    }
	    if (ioctl(fd, VT_GETMODE, &vtmode) < 0)
	    {
		FatalError("xf86OpenSyscons: VT_GETMODE failed\n");
	    }
	    xf86Info.consType = SYSCONS;
	    xf86Msg(X_PROBED, "Using syscons driver with X support");
	    if (syscons_version >= 0x100)
	    {
		xf86ErrorF(" (version %d.%d)\n", syscons_version >> 8,
			   syscons_version & 0xFF);
	    }
	    else
	    {
		xf86ErrorF(" (version 0.x)\n");
	    }
	    xf86Msg(from, "using VT number %d\n\n", xf86Info.vtno);
	}
	else
	{
	    /* VT_GETMODE failed, probably not syscons */
	    close(fd);
	    fd = -1;
	}
    }
    return fd;
}

#endif /* SYSCONS_SUPPORT */


#ifdef PCVT_SUPPORT

static int
xf86OpenPcvt()
{
    /* This looks much like syscons, since pcvt is API compatible */
    int fd = -1;
    vtmode_t vtmode;
    char vtname[12], *vtprefix;
    struct stat status;
    struct pcvtid pcvt_version;

#ifndef __OpenBSD__
    vtprefix = "/dev/ttyv";
#else
    vtprefix = "/dev/ttyC";
#endif

    fd = open(PCVT_CONSOLE_DEV, PCVT_CONSOLE_MODE, 0);
#ifdef WSCONS_SUPPORT
    if (fd < 0)
    {
	fd = open(WSCONS_PCVT_COMPAT_CONSOLE_DEV, PCVT_CONSOLE_MODE, 0);
	vtprefix = "/dev/ttyE";
    }
#endif
    if (fd >= 0) 
    {
	if (ioctl(fd, VGAPCVTID, &pcvt_version) >= 0)
	{
	    if(ioctl(fd, VT_GETMODE, &vtmode) < 0)
	    {
		FatalError("%s: VT_GETMODE failed\n%s%s\n%s\n",
			   "xf86OpenPcvt",
			   "Found pcvt driver but X11 seems to be",
			   " not supported.", CHECK_DRIVER_MSG);
	    }

	    xf86Info.vtno = VTnum;
		
	    if (ioctl(fd, VT_GETACTIVE, &initialVT) < 0)
		initialVT = -1;

	    if (xf86Info.vtno == -1)
	    {
		if (ioctl(fd, VT_OPENQRY, &xf86Info.vtno) < 0)
		{
		    /* No free VTs */
		    xf86Info.vtno = -1;
		}

		if (xf86Info.vtno == -1)
		{
		    /*
		     * All VTs are in use.  If initialVT was found, use it.
		     * Otherwise, if stdin is a VT, use that one.
		     * XXX stdin is already closed, so this won't work.
		     */
		    if (initialVT != -1)
		    {
			xf86Info.vtno = initialVT;
		    }
		    else if ((fstat(0, &status) >= 0)
			     && S_ISCHR(status.st_mode)
			     && (ioctl(0, VT_GETMODE, &vtmode) >= 0))
		    {
			/* stdin is a VT */
			xf86Info.vtno = minor(status.st_rdev) + 1;
		    }
		    else
		    {
			FatalError("%s: Cannot find a free VT\n",
				   "xf86OpenPcvt");
		    }
		}
	    }

	    close(fd);
            sprintf(vtname, "%s%01x", vtprefix, xf86Info.vtno - 1);
	    if ((fd = open(vtname, PCVT_CONSOLE_MODE, 0)) < 0)
	    {
		FatalError("xf86OpenPcvt: Cannot open %s (%s)\n",
			   vtname, strerror(errno));
	    }
	    if (ioctl(fd, VT_GETMODE, &vtmode) < 0)
	    {
		FatalError("xf86OpenPcvt: VT_GETMODE failed\n");
	    }
	    xf86Info.consType = PCVT;
	    xf86Msg(X_PROBED, "Using pcvt driver (version %d.%d)\n",
		       pcvt_version.rmajor, pcvt_version.rminor);
	}
	else
	{
	    /* Not pcvt */
	    close(fd);
	    fd = -1;
	}
    }
    return fd;
}

#endif /* PCVT_SUPPORT */

#ifdef WSCONS_SUPPORT

static int
xf86OpenWScons()
{
    int fd = -1;
    int mode = WSDISPLAYIO_MODE_MAPPED;
    int i;
    char ttyname[16];

    /* XXX Is this ok? */
    for (i = 0; i < 8; i++) {
	sprintf(ttyname, "/dev/ttyE%d", i);
	if ((fd = open(ttyname, 2)) != -1)
	    break;
    }
    if (fd != -1) {
	if (ioctl(fd, WSDISPLAYIO_SMODE, &mode) < 0) {
	    FatalError("%s: WSDISPLAYIO_MODE_MAPPED failed (%s)\n%s\n", 
		       "xf86OpenConsole", strerror(errno),
		       CHECK_DRIVER_MSG);
	}
	xf86Info.consType = WSCONS;
	xf86Msg(X_PROBED, "Using wscons driver\n");
    }
    return fd;
}

#endif /* WSCONS_SUPPORT */

void
xf86CloseConsole()
{
#if defined(SYSCONS_SUPPORT) || defined(PCVT_SUPPORT)
    struct vt_mode   VT;
#endif

    switch (xf86Info.consType)
    {
#ifdef PCCONS_SUPPORT
    case PCCONS:
	ioctl (xf86Info.consoleFd, CONSOLE_X_MODE_OFF, 0);
	break;
#endif /* PCCONS_SUPPORT */
#if defined (SYSCONS_SUPPORT) || defined (PCVT_SUPPORT)
    case SYSCONS:
    case PCVT:
        ioctl(xf86Info.consoleFd, KDSETMODE, KD_TEXT);  /* Back to text mode */
        if (ioctl(xf86Info.consoleFd, VT_GETMODE, &VT) != -1)
        {
	    VT.mode = VT_AUTO;
	    ioctl(xf86Info.consoleFd, VT_SETMODE, &VT); /* dflt vt handling */
        }
#if !defined(USE_DEV_IO) && !defined(USE_I386_IOPL)
        if (ioctl(xf86Info.consoleFd, KDDISABIO, 0) < 0)
        {
            xf86FatalError("xf86CloseConsole: KDDISABIO failed (%s)\n",
	                   strerror(errno));
        }
#endif
	if (initialVT != -1)
		ioctl(xf86Info.consoleFd, VT_ACTIVATE, initialVT);
        break;
#endif /* SYSCONS_SUPPORT || PCVT_SUPPORT */
#ifdef WSCONS_SUPPORT
    case WSCONS:
      {
	int mode = WSDISPLAYIO_MODE_EMUL;
	ioctl(xf86Info.screenFd, WSDISPLAYIO_SMODE, &mode);
	break;
      }
#endif
    }

    if (xf86Info.screenFd != xf86Info.consoleFd)
    {
	close(xf86Info.screenFd);
	close(xf86Info.consoleFd);
	if ((xf86Info.consoleFd = open("/dev/console",O_RDONLY,0)) <0)
	{
	    xf86FatalError("xf86CloseConsole: Cannot open /dev/console (%s)\n",
			   strerror(errno));
	}
    }
    close(xf86Info.consoleFd);
    if (devConsoleFd >= 0)
	close(devConsoleFd);
    return;
}

int
xf86ProcessArgument(int argc, char *argv[], int i)
{
	/*
	 * Keep server from detaching from controlling tty.  This is useful 
	 * when debugging (so the server can receive keyboard signals.
	 */
	if (!strcmp(argv[i], "-keeptty"))
	{
		KeepTty = TRUE;
		return(1);
	}
#if defined (SYSCONS_SUPPORT) || defined (PCVT_SUPPORT)
	if ((argv[i][0] == 'v') && (argv[i][1] == 't'))
	{
		if (sscanf(argv[i], "vt%2d", &VTnum) == 0 ||
		    VTnum < 1 || VTnum > 12)
		{
			UseMsg();
			VTnum = -1;
			return(0);
		}
		return(1);
	}
#endif /* SYSCONS_SUPPORT || PCVT_SUPPORT */
	return(0);
}

void
xf86UseMsg()
{
#if defined (SYSCONS_SUPPORT) || defined (PCVT_SUPPORT)
	ErrorF("vtXX                   use the specified VT number (1-12)\n");
#endif /* SYSCONS_SUPPORT || PCVT_SUPPORT */
	ErrorF("-keeptty               ");
	ErrorF("don't detach controlling tty (for debugging only)\n");
	return;
}
