.\" $XFree86: xc/programs/Xserver/hw/xfree86/drivers/vesa/vesa.cpp,v 1.2 2000/12/11 20:18:42 dawes Exp $ 
.\" shorthand for double quote that works everywhere.
.ds q \N'34'
.TH VESA __drivermansuffix__ "Version 4.0.2"  "XFree86"
.SH NAME
vesa \- Generic VESA video driver
.SH SYNOPSIS
.nf
.B "Section \*qDevice\*q"
.BI "  Identifier \*q"  devname \*q
.B  "  Driver \*qvesa\*q"
\ \ ...
.B EndSection
.fi
.SH DESCRIPTION
.B vesa
is an XFree86 driver for generic VESA video cards.  It can drive most
VESA-compatible video cards, but only makes use of the basic standard
VESA core that is common to these cards.  The driver supports depths 8, 15
16 and 24.
.SH SUPPORTED HARDWARE
The
.B vesa
driver supports most VESA-compatible video cards.  There are some known
exceptions, and those should be listed here.
.SH CONFIGURATION DETAILS
Please refer to XF86Config(__filemansuffix__) for general configuration
details.  This section only covers configuration details specific to this
driver.
.PP
The driver auto-detects the presence of VESA-compatible hardware.  The
.B ChipSet
name may optionally be specified in the config file
.B \*qDevice\*q
section, and will override the auto-detection:
.PP
.RS 4
"vesa"
.RE
.PP
The following driver
.B Options
are supported:
.TP
.BI "Option \*qShadowFB\*q \*q" boolean \*q
Enable or disable use of the shadow framebuffer layer.  See
shadowfb(__drivermansuffix__) for further information.  Default: on.

This option is recommended for performance reasons.
.SH "SEE ALSO"
XFree86(1), XF86Config(__filemansuffix__), xf86cfg(1), xf86config(1), Xserver(1), X(__miscmansuffix__)
.SH AUTHORS
Authors include: Paulo C�sar Pereira de Andrade.
