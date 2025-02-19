.\" $XFree86: xc/programs/Xserver/hw/xfree86/drivers/trident/trident.cpp,v 1.8 2000/12/11 20:18:37 dawes Exp $ 
.\" shorthand for double quote that works everywhere.
.ds q \N'34'
.TH TRIDENT __drivermansuffix__ "Version 4.0.2"  "XFree86"
.SH NAME
trident \- Trident video driver
.SH SYNOPSIS
.nf
.B "Section \*qDevice\*q"
.BI "  Identifier \*q"  devname \*q
.B  "  Driver \*qtrident\*q"
\ \ ...
.B EndSection
.fi
.SH DESCRIPTION
.B trident
is an XFree86 driver for Trident video cards.  The driver is fully
accelerated, and provides support for the following framebuffer depths:
1, 4, 8, 15, 16, and 24. Multi-head configurations
are supported. The XvImage extension is supported on all Blade and Image
series cards. Currently the ZOOM feature doesn't work properly on the Image
series.
.SH SUPPORTED HARDWARE
The
.B trident
driver supports PCI,AGP and ISA video cards based on the following 
Trident chips:
.TP 12
.B Blade
Blade3D, CyberBlade series i1, i7 (DSTN), i1, i1 (DSTN), Ai1, Ai1 (DSTN),
CyberBlade/e4
.TP 12
.B Image
3DImage975, 3DImage985, Cyber9520, Cyber9525, Cyber9397, Cyber9397DVD
.TP 12
.B ProVidia
9682, 9685, Cyber9382, Cyber9385, Cyber9388
.TP 12
.B TGUI
9440AGi, 9660, 9680
.TP 12
.B 8900
8900D (ISA)
.SH CONFIGURATION DETAILS
Please refer to XF86Config(__filemansuffix__) for general configuration
details.  This section only covers configuration details specific to this
driver.
.PP
The following driver
.B Options
are supported:
.TP
.BI "Option \*qSWCursor\*q \*q" boolean \*q
Enable or disable the SW cursor.  Default: off.
.TP
.BI "Option \*qNoAccel\*q \*q" boolean \*q
Disable or enable acceleration.  Default: acceleration is enabled.
.TP
.BI "Option \*qPciRetry\*q \*q" boolean \*q
Enable or disable PCI retries.  Default: off.
.TP
.BI "Option \*qCyberShadow\*q \*q" boolean \*q
For Cyber chipsets only, turn off shadow registers. If you only see
a partial display - this may be the option for you. Default: on.
.TP
.BI "Option \*qShadowFB\*q \*q" boolean \*q
Enable or disable use of the shadow framebuffer layer.  See
shadowfb(__drivermansuffix__) for further information.  Default: off.
.TP
.BI "Option \*qVideoKey\*q \*q" integer \*q
This sets the default pixel value for the YUV video overlay key.
NOTE: Default is 0 for depth 15 and 24. This needs fixing.
Default: undefined.
.TP
.BI "Option \*qNoPciBurst\*q \*q" boolean \*q
Turn off PCI burst mode, PCI Bursting is on by default.
Default: off.
.SH "SEE ALSO"
XFree86(1), XF86Config(__filemansuffix__), xf86config(1), Xserver(1), X(__miscmansuffix__)
.SH AUTHOR
Author: Alan Hourihane
