.\" $XFree86: xc/programs/Xserver/hw/xfree86/drivers/chips/chips.cpp,v 1.9 2000/12/12 18:54:30 dawes Exp $
.\" shorthand for double quote that works everywhere.
.ds q \N'34'
.TH CHIPS __drivermansuffix__ "Version 4.0.2"  "XFree86"
.SH NAME
chips \- Chips and Technologies video driver
.SH SYNOPSIS
.nf
.B "Section \*qDevice\*q"
.BI "  Identifier \*q"  devname \*q
.B  "  Driver \*qchips\*q"
\ \ ...
.B EndSection
.fi
.SH DESCRIPTION
.B chips 
is an XFree86 driver for Chips and Technologies video processors.  The majority
of the Chips and Technologies chipsets are supported by this driver. In general
the limitation on the capabilities of this driver are determined by the 
chipset on which it is run. Where possible, this driver provides full
acceleration and supports the following depths: 1, 4, 8, 15, 16, 24 and on
the latest chipsets an 8+16 overlay mode. All visual types are supported for
depth 1, 4 and 8 and both TrueColor and DirectColor visuals are supported
where possible. Multi-head configurations are supported on PCI or AGP buses.
.SH SUPPORTED HARDWARE
The
.B chips
driver supports video processors on most of the bus types currently available.
The chipsets supported fall into one of three architectural classes. A
.B basic
architecture, the
.B WinGine
architecture and the newer
.B HiQV
architecture.
.PP
.B Basic Architecture
.PP
The supported chipsets are
.B ct65520, ct65525, ct65530, ct65535, ct65540, ct65545, ct65546
and 
.B ct65548
.PP
Color depths 1, 4 and 8 are supported on all chipsets, while depths 15, 16
and 24 are supported only on the
.B 65540, 65545, 65546
and 
.B 65548
chipsets. The driver is accelerated when used with the
.B 65545, 65546
or
.B 65548
chipsets, however the DirectColor visual is not available.
.PP
.B Wingine Architecture
.PP
The supported chipsets are
.B ct64200
and 
.B ct64300
.PP
Color depths 1, 4 and 8 are supported on both chipsets, while depths 15, 16
and 24 are supported only on the
.B 64300
chipsets. The driver is accelerated when used with the
.B 64300
chipsets, however the DirectColor visual is not available.
.PP
.B HiQV Architecture
.PP
The supported chipsets are
.B ct65550, ct65554, ct65555, ct68554, ct69000
and 
.B ct69030
.PP
Color depths 1, 4, 8, 15, 16, 24 and 8+16 are supported on all chipsets.
The DirectColor visual is supported on all color depths except the 8+16
overlay mode. Full acceleration is supplied for all chipsets.
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
"ct65520", "ct65525", "ct65530", "ct65535", "ct65540", "ct65545", "ct65546",
"ct65548", "ct65550", "ct65554", "ct65555", "ct68554", "ct69000", "ct69030",
"ct64200", "ct64300".
.RE
.PP
The driver will auto-detect the amount of video memory present for all
chipsets.  But maybe overridden with the
.B VideoRam
entry in the config file
.B \*qDevice\*q
section.
.PP
The following driver
.B Options
are supported, on one or more of the supported chipsets:
.TP
.BI "Option \*qNoAccel\*q \*q" boolean \*q
Disable or enable acceleration.  Default: acceleration is enabled.
.TP
.BI "Option \*qNoLinear\*q \*q" boolean \*q
Disables linear addressing in cases where it is enabled by default.
Default: off
.TP
.BI "Option \*qLinear\*q \*q" boolean \*q
Enables linear addressing in cases where it is disabled by default.
Default: off
.TP
.BI "Option \*qHWCursor\*q \*q" boolean \*q
Enable or disable the HW cursor.  Default: on.
.TP
.BI "Option \*qSWCursor\*q \*q" boolean \*q
Enable or disable the HW cursor.  Default: off.
.TP
.BI "Option \*qSTN\*q \*q" boolean \*q
Force detection of STN screen type. Default: off.
.TP
.BI "Option \*qUseModeline\*q \*q" boolean \*q
Reprogram flat panel timings with values from the modeline. Default: off
.TP
.BI "Option \*qFixPanelSize\*q \*q" boolean \*q
Reprogram flat panel size with values from the modeline. Default: off
.TP
.BI "Option \*qNoStretch\*q \*q" boolean \*q
This option disables the stretching on a mode on a flat panel to fill the
screen. Default: off
.TP
.BI "Option \*qLcdCenter\*q \*q" boolean \*q
Center the mode displayed on the flat panel on the screen. Default: off
.TP
.BI "Option \*qHWclocks\*q \*q" boolean \*q
Force the use of fixed hardware clocks on chips that support both fixed
and programmable clocks. Default: off
.TP
.BI "Option \*qUseVclk1\*q \*q" boolean \*q
Use the Vclk1 programable clock on
.B HiQV
chipsets instead of Vclk2. Default: off
.TP
.BI "Option \*qFPClock8\*q \*q" float \*q
.TP
.BI "Option \*qFPClock16\*q \*q" float \*q
.TP
.BI "Option \*qFPClock24\*q \*q" float \*q
.TP
.BI "Option \*qFPClock32\*q \*q" float \*q
Force the use of a particular video clock speed for use with the 
flat panel at a specified depth
.TP
.BI "Option \*qMMIO\*q \*q" boolean \*q
Force the use of memory mapped IO where it can be used. Default: off
.TP
.BI "Option \*qSuspendHack\*q \*q" boolean \*q
Force driver to leave centering and stretching registers alone. This
can fix some laptop suspend/resume problems. Default: off
.TP
.BI "Option \*qOverlay\*q"
Enable 8+24 overlay mode.  Only appropriate for depth 24.  Default: off.
.TP
.BI "Option \*qColorKey\*q \*q" integer \*q
Set the colormap index used for the transparency key for the depth 8 plane
when operating in 8+16 overlay mode.  The value must be in the range
2\-255.  Default: 255.
.TP
.BI "Option \*qVideoKey\*q \*q" integer \*q
This sets the default pixel value for the YUV video overlay key.
Default: undefined.
.TP
.BI "Option \*qShadowFB\*q \*q" boolean \*q
Enable or disable use of the shadow framebuffer layer.  See
shadowfb(__drivermansuffix__) for further information.  Default: off.
.TP
.BI "Option \*qSyncOnGreen\*q \*q" boolean \*q
Enable or disable combining the sync signals with the green signal.
Default: off.
.TP
.BI "Option \*qShowCache\*q \*q" boolean \*q
Enable or disable viewing offscreen memory. Used for debugging only
Default: off.
.TP
.BI "Option \*q18bitBus\*q \*q" boolean \*q
Force the driver to assume that the flat panel has an 18bit data bus.
Default: off.
.SH "SEE ALSO"
XFree86(1), XF86Config(__filemansuffix__), xf86config(1), Xserver(1), X(__miscmansuffix__)
.PP
You are also recommended to read the README.chips file that comes with all
XFree86 distributions, which discusses the
.B chips
driver in more detail.
.SH AUTHORS
Authors include: Jon Block, Mike Hollick, Regis Cridlig, Nozomi Ytow,
Egbert Eich, David Bateman and Xavier Ducoin

