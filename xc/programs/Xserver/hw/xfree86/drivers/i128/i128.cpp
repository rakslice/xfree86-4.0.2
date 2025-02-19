.\" $XFree86: xc/programs/Xserver/hw/xfree86/drivers/i128/i128.cpp,v 1.2 2000/12/11 20:18:14 dawes Exp $ 
.\" shorthand for double quote that works everywhere.
.ds q \N'34'
.TH I128 __drivermansuffix__ "Version 4.0.2"  "XFree86"
.SH NAME
i128 \- Number 9 I128 video driver
.SH SYNOPSIS
.nf
.B "Section \*qDevice\*q"
.BI "  Identifier \*q"  devname \*q
.B  "  Driver \*qi128\*q"
\ \ ...
.B EndSection
.fi
.SH DESCRIPTION
.B i128 
is an XFree86 driver for Number 9 I128 video cards.  The driver is
accelerated and provides support for all versions of the I128 chip family,
including the SGI flatpanel configuration.  Multi-head configurations are
supported.
.SH SUPPORTED HARDWARE
The
.B i128
driver supports PCI and AGP video cards based on the following I128 chips:
.TP 12
.B I128 rev 1
(original)
.TP 12
.B I128-II
.TP 12
.B I128-T2R
Ticket 2 Ride
.TP 12
.B I128-T2R4
Ticket 2 Ride IV
.SH CONFIGURATION DETAILS
Please refer to XF86Config(__filemansuffix__) for general configuration
details.  This section only covers configuration details specific to this
driver.
.PP
The driver auto-detects the chipset type and may not be overridden.
.PP
The driver auto-detects the amount of video memory present for all
chips and may not be overridden.
.PP
The following driver
.B Options
are supported:
.TP
.BI "Option \*qHWCursor\*q \*q" boolean \*q
Enable or disable the HW cursor.  Default: on.
.TP
.BI "Option \*qNoAccel\*q \*q" boolean \*q
Disable or enable acceleration.  Default: acceleration is enabled.
.TP
.BI "Option \*qSyncOnGreen\*q \*q" boolean \*q
Enable or disable combining the sync signals with the green signal.
Default: off.
.TP
.BI "Option \*qDac6Bit\*q \*q" boolean \*q
Reduce DAC operations to 6 bits.
Default: false.
.TP
.BI "Option \*qDebug\*q \*q" boolean \*q
This turns on verbose debug information from the driver.
Default: off.
.SH "SEE ALSO"
XFree86(1), XF86Config(__filemansuffix__), xf86config(1), Xserver(1), X(__miscmansuffix__)
.SH AUTHORS
Authors include: Robin Cutshaw (driver), Galen Brooks (flatpanel support).
