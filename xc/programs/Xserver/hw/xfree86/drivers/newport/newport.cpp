.\" $XFree86: xc/programs/Xserver/hw/xfree86/drivers/newport/newport.cpp,v 1.1 2000/12/14 20:59:12 dawes Exp $ 
.\" shorthand for double quote that works everywhere.
.ds q \N'34'
.TH NEWPORT __drivermansuffix__ "Version 4.0.2"  "XFree86"
.SH NAME
newport \- Newport video driver
.SH SYNOPSIS
.nf
.B "Section \*qDevice\*q"
.BI "  Identifier \*q"  devname \*q
.B  "  Driver \*qnewport\*q"
\ \ ...
.B EndSection
.fi
.SH DESCRIPTION
.B newport
is an XFree86 driver for the SGI Indy's newport video cards.
.SH SUPPORTED HARDWARE
The
.B newport
driver supports the Newport(sometimes called XL) cards found in SGI Indys. It 
does not support the XZ boards. The driver is currently limited to 8bit only.
.SH CONFIGURATION DETAILS
Please refer to XF86Config(__filemansuffix__) for general configuration
details.  This section only covers configuration details specific to this
driver.
.PP
The driver auto-detects all device information necessary to initialize
the card.  However, if you have problems with auto-detection, you can
specify:
.br
.TP
.BI "Option \*qbitplanes\*q \*q" integer \*q
number of bitplanes of the board (8 or 24)
Default: auto-detected.
.SH "SEE ALSO"
XFree86(1), XF86Config(__filemansuffix__), xf86config(1), Xserver(1), X(__miscmansuffix__)
.SH AUTHORS
Authors:
Guido Guenther   \fIguido.guenther@gmx.net\fP
