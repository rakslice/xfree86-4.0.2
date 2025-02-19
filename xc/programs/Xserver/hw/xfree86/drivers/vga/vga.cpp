.\" $XFree86: xc/programs/Xserver/hw/xfree86/drivers/vga/vga.cpp,v 1.7 2000/12/11 20:18:43 dawes Exp $ 
.\" shorthand for double quote that works everywhere.
.ds q \N'34'
.TH VGA __drivermansuffix__ "Version 4.0.2"  "XFree86"
.SH NAME
vga \- Generic VGA video driver
.SH SYNOPSIS
.nf
.B "Section \*qDevice\*q"
.BI "  Identifier \*q"  devname \*q
.B  "  Driver \*qvga\*q"
\ \ ...
.B EndSection
.fi
.SH DESCRIPTION
.B vga 
is an XFree86 driver for generic VGA video cards.  It can drive most
VGA-compatible video cards, but only makes use of the basic standard
VGA core that is common to these cards.  The driver supports depths 1, 4
and 8.  All relevant visual types are supported at each depth.
Multi-head configurations
are supported in combination with some other drivers, but only when the
.B vga
driver is driving the primary head.
.SH SUPPORTED HARDWARE
The
.B vga
driver supports most VGA-compatible video cards.  There are some known
exceptions, and those should be listed here.
.SH CONFIGURATION DETAILS
Please refer to XF86Config(__filemansuffix__) for general configuration
details.  This section only covers configuration details specific to this
driver.
.PP
The driver auto-detects the presence of VGA-compatible hardware.  The
.B ChipSet
name may optionally be specified in the config file
.B \*qDevice\*q
section, and will override the auto-detection:
.PP
.RS 4
"generic"
.RE
.PP
The driver will only use 64k of video memory for depth 1 and depth 8 operation,
and 256k of video memory for depth 4 (this is the standard VGA limit).
.PP
When operating at depth 8, only a single built-in 320x200 video mode is
available.  At other depths there is more flexibility regarding mode choice.
.PP
The following driver
.B Options
are supported:
.TP
.BI "Option \*qShadowFB\*q \*q" boolean \*q
Enable or disable use of the shadow framebuffer layer.  See
shadowfb(__drivermansuffix__) for further information.  Default: off.

This option is recommended for performance reasons when running at depths
1 and 4, especially when using modern PCI-based hardware.  It is required
when using those depths in a multi-head configuration where one or more
of the other screens is operating at a different depth.
.SH "SEE ALSO"
XFree86(1), XF86Config(__filemansuffix__), xf86config(1), Xserver(1), X(__miscmansuffix__)
.SH AUTHORS
Authors include: Marc La France, David Dawes, and Dirk Hohndel.
