.\" $XFree86: xc/programs/Xserver/hw/xfree86/drivers/neomagic/neomagic.cpp,v 1.7 2000/12/14 08:11:15 herrb Exp $ 
.\" shorthand for double quote that works everywhere.
.ds q \N'34'
.TH NEOMAGIC __drivermansuffix__ "Version 4.0.2"  "XFree86"
.SH NAME
neomagic \- Neomagic video driver
.SH SYNOPSIS
.nf
.B "Section \*qDevice\*q"
.BI "  Identifier \*q"  devname \*q
.B  "  Driver \*qneomagic\*q"
\ \ ...
.B EndSection
.fi
.SH DESCRIPTION
.B neomagic 
is an XFree86 driver for the Neomagic graphics chipsets found in many
laptop computers.  
.SH SUPPORTED HARDWARE
.B neomagic
supports the following chipsets:
.PP
.TP 
MagicGraph 128    (NM2070)
.TP
MagicGraph 128V   (NM2090)
.TP 
MagicGraph 128ZV  (NM2093)
.TP 
MagicGraph 128ZV+ (NM2097)
.TP 
MagicGraph 128XD  (NM2160)
.TP 
MagicGraph 256AV  (NM2200)
.TP 
MagicGraph 256AV+ (NM2230)
.TP 
MagicGraph 256ZX  (NM2360)
.TP 
MagicGraph 256XL+ (NM2380)
.PP
The driver supports depths 8, 15, 16 and 24 for all chipsets except the
NM2070 which does not support depth 24.  All depths are accelerated except for
depth 24 which is only accelerated on NM2200
and newer models.  All visuals are supported in depth 8.  TrueColor and
DirectColor visuals are supported in the other depths.

.SH CONFIGURATION DETAILS
Please refer to XF86Config(__filemansuffix__) for general configuration
details.  This section only covers configuration details specific to this
driver.
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
.BI "Option \*qPCIBurst\*q \*q" boolean \*q
Disable or enable PCI burst modes.  Default: enabled.
.TP
.BI "Option \*qRotate\*q \*qCW\*q"
.TP
.BI "Option \*qRotate\*q \*qCCW\*q"
Rotate the display clockwise or counterclockwise.  This mode is unaccelerated.
Default: no rotation.
.TP
.BI "Option \*qShadowFB\*q \*q" boolean \*q
Enable or disable use of the shadow framebuffer layer.  See
shadowfb(__drivermansuffix__) for further information.  Default: off.
.PP
.B Note
.br
On some laptops using the 2160 chipset (MagicGraph 128XD) the
following options are needed to avoid a lock-up of the graphic engine:
.nf
    Option "XaaNoScanlineImageWriteRect"
    Option "XaaNoScanlineCPUToScreenColorExpandFill"
.fi

.SH "SEE ALSO"
XFree86(1), XF86Config(__filemansuffix__), xf86config(1), Xserver(1), X(__miscmansuffix__)
.SH AUTHORS
Authors include: Jens Owen, Kevin E. Martin, and also Egbert Eich,  
Mark Vojkovich, Alan Hourihane.
