.\" $XFree86: xc/programs/Xserver/hw/xfree86/input/microtouch/microtouch.cpp,v 1.5 2000/12/11 20:18:51 dawes Exp $ 
.\" shorthand for double quote that works everywhere.
.ds q \N'34'
.TH MICROTOUCH __drivermansuffix__ "Version 4.0.2"  "XFree86"
.SH NAME
microtouch \- MicroTouch input driver
.SH SYNOPSIS
.nf
.B "Section \*qInputDevice\*q"
.BI "  Identifier \*q" idevname \*q
.B  "  Driver \*qmicrotouch\*q"
.BI "  Option \*qDevice\*q   \*q" devpath \*q
\ \ ...
.B EndSection
.fi
.SH DESCRIPTION
.B microtouch 
is an XFree86 input driver for MicroTouch devices...
.PP
The
.B microtouch
driver functions as a pointer input device, and may be used as the
X server's core pointer.
THIS MAN PAGE NEEDS TO BE FILLED IN.
.SH SUPPORTED HARDWARE
What is supported...
.SH CONFIGURATION DETAILS
Please refer to XF86Config(__filemansuffix__) for general configuration
details and for options that can be used with all input drivers.  This
section only covers configuration details specific to this driver.
.PP
Config details...
.SH "SEE ALSO"
XFree86(1), XF86Config(__filemansuffix__), xf86config(1), Xserver(1), X(__miscmansuffix__).
.SH AUTHORS
Authors include...
