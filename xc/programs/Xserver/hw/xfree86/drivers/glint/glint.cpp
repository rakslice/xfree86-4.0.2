.\" $XFree86: xc/programs/Xserver/hw/xfree86/drivers/glint/glint.cpp,v 1.10 2000/12/11 20:18:13 dawes Exp $ 
.\" shorthand for double quote that works everywhere.
.ds q \N'34'
.TH GLINT __drivermansuffix__ "Version 4.0.2"  "XFree86"
.SH NAME
glint \- GLINT/Permedia video driver
.SH SYNOPSIS
.nf
.B "Section \*qDevice\*q"
.BI "  Identifier \*q"  devname \*q
.B  "  Driver \*qglint\*q"
\ \ ...
.B EndSection
.fi
.SH DESCRIPTION
.B glint 
is an XFree86 driver for 3Dlabs & Texas Instruments GLINT/Permedia based video
cards. The driver is rather fully accelerated, and provides support for the
following framebuffer depths: 8, 15 (may give bad results with FBDev support),
16, 24 (32 bpp recommended, 24 bpp has problems), 30, and an 8+24 overlay mode.
.SH SUPPORTED HARDWARE
The
.B glint
driver supports 3Dlabs (GLINT MX, GLINT 500TX, GLINT GAMMA, Permedia,
Permedia 2, Permedia 2v, Permedia 3) and Texas Instruments (Permedia,
Permedia 2) chips.
.SH CONFIGURATION DETAILS
Please refer to XF86Config(__filemansuffix__) for general configuration
details.  This section only covers configuration details specific to this
driver.
.PP
The driver auto-detects the chipset type, but the following
.B ChipSet
names may optionally be specified in the config file
.B \*qDevice\*q
section, and will override the auto-detection:
.PP
.RS 4
"ti_pm2", "ti_pm", "pm3", "pm2v", "pm2", "pm", "500tx", "mx", "gamma".
.RE
.PP
The driver will try to auto-detect the amount of video memory present for all
chips.  If it's not detected correctly, the actual amount of video memory should
be specified with a
.B VideoRam
entry in the config file
.B \*qDevice\*q
section.
.PP
Additionally, you may need to specify the bus ID of your card with a
.B BusID
entry in the config file
.B \*qDevice\*q
section, especially with FBDev support.
.PP
The following driver
.B Options
are supported:
.TP
.BI "Option \*qHWCursor\*q \*q" boolean \*q
Enable or disable the HW cursor.  Default: on.
.TP
.BI "Option \*qSWCursor\*q \*q" boolean \*q
Enable or disable the SW cursor.  Default: off.
This option disables the
.B HWCursor
option and vice versa.
.TP
.BI "Option \*qNoAccel\*q \*q" boolean \*q
Disable or enable acceleration.  Default: acceleration is enabled.
.TP
.BI "Option \*qOverlay\*q"
Enable 8+24 overlay mode.  Only appropriate for depth 24, 32 bpp.
.RB ( Note:
This hasn't been tested with FBDev support and probably won't work.) 
Recognized values are: "8,24", "24,8". Default: off.
.TP
.BI "Option \*qPciRetry\*q \*q" boolean \*q
Enable or disable PCI retries.
.RB ( Note:
This doesn't work with Permedia2 based cards for Amigas.)  Default: off.
.TP
.BI "Option \*qShadowFB\*q \*q" boolean \*q
Enable or disable use of the shadow framebuffer layer.  See
shadowfb(__drivermansuffix__) for further information.
.RB ( Note:
This disables hardware acceleration.)  Default: off.
.TP
.BI "Option \*qUseFBDev\*q \*q" boolean \*q
Enable or disable use of an OS-specific fb interface (which is not supported
on all OSs).  See fbdevhw(__drivermansuffix__) for further information.
Default: off.
.ig
.TP
.BI "Option \*qRGBbits\*q \*q" integer \*q
Each gun of the RGB triple can have either 8 or 10 bits.  Default: 8
..
.TP
.BI "Option \*qBlockWrite\*q \*q" boolean \*q
Enable or disable block writes for the various Permedia 2 chips. This improves
acceleration in general, but disables it for some special cases.  Default: off.
.TP
.BI "Option \*qFireGL3000\*q \*q" boolean \*q
If you have a card of the same name, turn this on.  Default: off.
.TP
.BI "Option \*qSetMClk\*q \*q" freq \*q
The driver will try to auto-detect the memory clock for all chips.  If it's not
detected correctly, the actual value (in MHz) should be specified with this
option.
.SH "SEE ALSO"
XFree86(1), XF86Config(__filemansuffix__), xf86config(1), Xserver(1), X(__miscmansuffix__)
.SH AUTHORS
Authors include: Alan Hourihane, Dirk Hohndel, Stefan Dirsch, Michel D�nzer,
Sven Luther
