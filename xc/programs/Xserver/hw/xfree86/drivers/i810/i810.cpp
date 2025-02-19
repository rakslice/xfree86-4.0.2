.\" $XFree86: xc/programs/Xserver/hw/xfree86/drivers/i810/i810.cpp,v 1.6 2000/12/11 20:18:17 dawes Exp $ 
.\" shorthand for double quote that works everywhere.
.ds q \N'34'
.TH I810 __drivermansuffix__ "Version 4.0.2"  "XFree86"
.SH NAME
i810 \- Intel i810 video driver
.SH SYNOPSIS
.nf
.B "Section \*qDevice\*q"
.BI "  Identifier \*q"  devname \*q
.B  "  Driver \*qi810\*q"
\ \ ...
.B EndSection
.fi
.SH DESCRIPTION
.B i810
is an XFree86 driver for the Intel i810 family of graphics chipsets.
The driver supports depths 8, 15, 16 and 24.  All visual types are
supported in depth 8, other depths only support TrueColor.  The driver
supports hardware accelerated 3D via the Direct Rendering Infrastructure (DRI),
but only in depth 16.
.SH SUPPORTED HARDWARE
.B i810
supports the i810, i810-DC100, i810e and i815 chipsets.

.SH CONFIGURATION DETAILS
Please refer to XF86Config(__filemansuffix__) for general configuration
details.  This section only covers configuration details specific to this
driver.
.PP
The i810 has a unified memory architecture and uses system memory
for video ram.  By default 8 Megabytes of system memory are used
for graphics.  This amount may be changed with the
.B VideoRam 
entry in the config file 
.B "Device"
section.  It may be set to any power of two between 4 and 32 Megabytes
inclusive to allow the user to customize the balance between main
memory usage and graphics performance.  Too little memory reserved for
graphics can result in decreased 3D and 2D graphics performance and
features.
.PP
The following driver
.B Options
are supported
.TP
.BI "Option \*qNoAccel\*q \*q" boolean \*q
Disable or enable acceleration.  Default: acceleration is enabled.
.TP
.BI "Option \*qSWCursor\*q \*q" boolean \*q
Disable or enable software cursor.  Default: software cursor is disable
and a hardware cursor is used.
.TP
.BI "Option \*qColorKey\*q \*q" integer \*q
This sets the default pixel value for the YUV video overlay key.
Default: undefined.
.TP
.BI "Option \*qCacheLines\*q \*q" integer \*q
This allows the user to change the amount of graphics memory used for
2D acceleration and video.  Decreasing this amount leaves more for 3D
textures.  Increasing it can improve 2D performance at the expense of
3D performance.
Default: 256 to 768 depending on the resolution and depth.



.SH "SEE ALSO"
XFree86(1), XF86Config(__filemansuffix__), xf86config(1), Xserver(1), X(__miscmansuffix__)
.SH AUTHORS
Authors include: Keith Whitwell, and also Jonathan Bian, Matthew J Sottek, 
Jeff Hartmann, Mark Vojkovich, Alan Hourihane, H. J. Lu.
