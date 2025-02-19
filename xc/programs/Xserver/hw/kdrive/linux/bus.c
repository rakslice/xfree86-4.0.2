/*
 * $XFree86: xc/programs/Xserver/hw/kdrive/linux/bus.c,v 1.1 2000/09/22 06:25:09 keithp Exp $
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

#define NEED_EVENTS
#include "X.h"
#include "Xproto.h"
#include "inputstr.h"
#include "scrnintstr.h"
#include "kdrive.h"
#include "Xpoll.h"

/* /dev/adbmouse is a busmouse */

void
BusRead (int adbPort)
{
    unsigned char   buf[3];
    unsigned char   *b;
    int		    n;
    int		    dx, dy;
    unsigned long   flags;

    n = read (adbPort, buf, 3);
    if (n == 3)
    {
	flags = KD_MOUSE_DELTA;
	dx = (char) buf[1];
	dy = -(char) buf[2];
	if ((buf[0] & 4) == 0)
	    flags |= KD_BUTTON_1;
	if ((buf[0] & 2) == 0)
	    flags |= KD_BUTTON_2;
	if ((buf[0] & 1) == 0)
	    flags |= KD_BUTTON_3;
        KdEnqueueMouseEvent (flags, dx, dy);
    }
}

char	*BusNames[] = {
    "/dev/adbmouse",
    "/dev/mouse",
};

#define NUM_BUS_NAMES	(sizeof (BusNames) / sizeof (BusNames[0]))

int
BusInit (void)
{
    int	    i;
    int	    busPort;

    for (i = 0; i < NUM_BUS_NAMES; i++)
    {
	busPort = open (BusNames[i], 0);
	if (busPort >= 0)
	    return busPort;
    }
}

void
BusFini (int busPort)
{
    if (busPort >= 0)
	close (busPort);
}

KdMouseFuncs BusMouseFuncs = {
    BusInit,
    BusRead,
    BusFini
};
