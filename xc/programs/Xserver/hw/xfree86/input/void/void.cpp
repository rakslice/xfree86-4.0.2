.\" $XFree86: xc/programs/Xserver/hw/xfree86/input/void/void.cpp,v 1.5 2000/12/11 20:18:56 dawes Exp $ 
.\" shorthand for double quote that works everywhere.
.ds q \N'34'
.TH VOID __drivermansuffix__ "Version 4.0.2"  "XFree86"
.SH NAME
void \- null input driver
.SH SYNOPSIS
.nf
.B "Section \*qInputDevice\*q"
.BI "  Identifier \*q" idevname \*q
.B  "  Driver \*qvoid\*q"
\ \ ...
.B EndSection
.fi
.SH DESCRIPTION
.B void 
is an dummy/null XFree86 input driver.  It doesn't connect to any
physical device, and it never delivers any events.  It functions as
both a pointer and keyboard device, and may be used as X server's core
pointer and/or core keyboard.  It's purpose is to allow the X server
to operate without a core pointer and/or core keyboard.
.SH CONFIGURATION DETAILS
Please refer to XF86Config(__filemansuffix__) for general configuration
details and for options that can be used with all input drivers.  This
driver doesn't have any configuration options in addition to those.
.SH "SEE ALSO"
XFree86(1), XF86Config(__filemansuffix__), xf86config(1), Xserver(1), X(__miscmansuffix__).
.SH AUTHORS
Authors include...
