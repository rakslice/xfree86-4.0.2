.\" $XFree86: xc/programs/Xserver/hw/xfree86/drivers/fbdev/fbdev.cpp,v 1.10 2000/12/11 22:40:25 dawes Exp $ 
.\" shorthand for double quote that works everywhere.
.ds q \N'34'
.TH FBDEV __drivermansuffix__ "Version 4.0.2"  "XFree86"
.SH NAME
fbdev \- video driver for framebuffer device
.SH SYNOPSIS
.nf
.B "Section \*qDevice\*q"
.BI "  Identifier \*q"  devname \*q
.B  "  Driver \*qfbdev\*q"
.BI "  BusID  \*qpci:" bus : dev : func \*q
\ \ ...
.B EndSection
.fi
.SH DESCRIPTION
.B fbdev
is an XFree86 driver for framebuffer devices.  This is a non-accelerated
driver, the following framebuffer depths are supported: 8, 15, 16, 24.
All visual types are supported for depth 8, and TrueColor visual is
supported for the other depths.  Multi-head configurations are supported.
.SH SUPPORTED HARDWARE
The 
.B fbdev
driver supports all hardware where a framebuffer driver is available.
fbdev uses the os-specific submodule fbdevhw(__drivermansuffix__) to talk
to the kernel
device driver.  Currently a fbdevhw module is available for linux.
.SH CONFIGURATION DETAILS
Please refer to XF86Config(__filemansuffix__) for general configuration
details.  This section only covers configuration details specific to
this driver.
.PP
For this driver it is not required to specify modes in the screen 
section of the config file.  The
.B fbdev
driver can pick up the currently used video mode from the framebuffer 
driver and will use it if there are no video modes configured.
.PP
For PCI boards you might have to add a BusID line to the Device
section.  See above for a sample line.  You can use "XFree86 -scanpci"
to figure out the correct values.
.PP
The following driver 
.B Options
are supported:
.TP
.BI "Option \*qfbdev\*q \*q" string \*q
The framebuffer device to use. Default: /dev/fb0.
.TP
.BI "Option \*qShadowFB\*q \*q" boolean \*q
Enable or disable use of the shadow framebuffer layer.  See
shadowfb(__drivermansuffix__) for further information.  Default: on.
.SH "SEE ALSO"
XFree86(1), XF86Config(__filemansuffix__), xf86config(1), Xserver(1),
X(__miscmansuffix__), fbdevhw(__drivermansuffix__)
.SH AUTHORS
Authors include: Gerd Knorr, Michel D�nzer, Geert Uytterhoeven
