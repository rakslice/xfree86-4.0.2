.\" $XFree86: xc/programs/Xserver/hw/xfree86/input/dynapro/dynapro.cpp,v 1.5 2000/12/11 20:18:48 dawes Exp $ 
.\" shorthand for double quote that works everywhere.
.ds q \N'34'
.TH DYNAPRO __drivermansuffix__ "Version 4.0.2"  "XFree86"
.SH NAME
dynapro \- Dynapro input driver
.SH SYNOPSIS
.nf
.B "Section \*qInputDevice\*q"
.BI "  Identifier \*q" idevname \*q
.B  "  Driver \*qdynapro\*q"
.BI "  Option \*qDevice\*q   \*q" devpath \*q
\ \ ...
.B EndSection
.fi
.SH DESCRIPTION
.B dynapro 
is an XFree86 input driver for Dynapro devices...
.PP
The
.B dynapro
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
