.\" $XFree86: xc/programs/Xserver/hw/xfree86/input/wacom/wacom.cpp,v 1.9 2000/12/18 00:17:18 dawes Exp $ 
.\" shorthand for double quote that works everywhere.
.ds q \N'34'
.TH WACOM __drivermansuffix__ "Version 4.0.2"  "XFree86"
.SH NAME
wacom \- Wacom input driver
.SH SYNOPSIS
.nf
.B "Section \*qInputDevice\*q"
.BI "  Identifier \*q" idevname \*q
.B  "  Driver \*qwacom\*q"
.BI "  Option \*qDevice\*q   \*q" devpath \*q
\ \ ...
.B EndSection
.fi
.SH DESCRIPTION
.B wacom 
is an XFree86 input driver for Wacom devices.
.PP
The
.B wacom
driver functions as a pointer input device, and may be used as the
X server's core pointer.
.SH SUPPORTED HARDWARE
This driver supports the Wacom IV and Wacom V protocols.
Preliminary support is available for USB devices on some Linux platforms.
.SH CONFIGURATION DETAILS
Please refer to XF86Config(__filemansuffix__) for general configuration
details and for options that can be used with all input drivers.  This
section only covers configuration details specific to this driver.
.PP
Multiple instances of the Wacom devices can cohabit. It can be useful
to define multiple devices with different active zones. Each device
supports the following entries:
.RS 8
.TP 4
.B Option \fI"Type"\fP \fI"stylus"|"eraser"|"cursor"\fP
sets the type of tool the device represent. This option is mandatory.
.TP 4
.B Option \fI"Device"\fP \fI"path"\fP
sets the path to the special file which represents serial line where
the tablet is plugged.  You have to specify it for each subsection with
the same value if you want to have multiple devices with the same tablet.
This option is mandatory.
.TP 4
.B Option \fI"USB"\fP \fI"on"\fP
tells the driver to dialog with the tablet the USB way.  This option is
only available on some Linux platforms.
.TP 4
.B Option \fI"DeviceName"\fP \fI"name"\fP
sets the name of the X device.
.TP 4
.B Option \fI"Suppress"\fP \fI"Inumber"\fP
sets the position increment under which not to transmit coordinates.
This entry must be specified only in the first Wacom subsection if you have
multiple devices for one tablet. If you don't specify this entry, the default value
is computed to
.TP 4
.B Option \fI"Mode"\fP \fI"Relative"|"Absolute"\fP
sets the mode of the device.
.TP 4
.B Option \fI"Tilt"\fP \fI"on"\fP
enables tilt report if your tablet supports it (ROM version 1.4 and above).
If this is enabled, multiple devices at the same time will not be reported.
.TP 4
.B Option \fI"HistorySize"\fP \fI"number"\fP
sets the motion history size. By default the value is zero.
.TP 4
.B Option \fI"AlwaysCore"\fP \fI"on"\fP
enables the sharing of the core pointer. When this feature is enabled, the
device will take control of the core pointer (and thus will emit core events)
and at the same time will be able, when asked so, to report extended events.
You can use the last available integer feedback to control this feature. When
the value of the feedback is zero, the feature is disabled. The feature is
enabled for any other value.
.TP 4
.B Option \fI"TopX"\fP \fI"number"\fP
X coordinate of the top corner of the active zone.
.TP 4
.B Option \fI"TopY"\fP \fI"number"\fP
Y coordinate of the top corner of the active zone.
.TP 4
.B Option \fI"BottomX"\fP \fI"Inumber"\fP
X coordinate of the bottom corner of the active zone.
.TP 4
.B Option \fI"BottomY"\fP \fI"number"\fP
Y coordinate of the bottom corner of the active zone.
.TP 4
.B Option \fI"KeepShape"\fP \fI"on"\fP
When this option is enabled, the active zone  begins according to TopX
and TopY.  The bottom corner is adjusted to keep the ratio width/height
of the active zone the same as the screen while maximizing the area
described by TopX, TopY, BottomX, BottomY.
.TP 4
.B Option \fI"DebugLevel"\fP \fInumber \fP
sets the level of debugging info reported.
.TP 4
.B Option \fI"BaudRate"\fP \fI"38400"\fP, \fI"19200"\fP or \fI"9600"\fP (default)
changes the serial link speed. This option is only available for
wacom V models (Intuos).
.TP 4
.B Option \fI"Serial"\fP \fI"number"\fP
sets the serial number associated with the physical device. This allows
to have multiple devices of the same type (i.e. multiple pens). This
option is only available on wacom V devices (Intuos). To see which
serial number belongs to a device, you have to set the DebugLevel to 6 and
watch the output of the X server.
.TP 4
.B Option \fI"Threshold"\fP \fI"number"\fP
sets the pressure threshold used to generate a button 1 events of stylus
devices for some models of tablets (Intuos and Graphire).
.RE
.SH "SEE ALSO"
XFree86(1), XF86Config(__filemansuffix__), xf86config(1), Xserver(1), X(__miscmansuffix__).
.SH AUTHORS
Frederic Lepied <lepied@xfree86.org>
