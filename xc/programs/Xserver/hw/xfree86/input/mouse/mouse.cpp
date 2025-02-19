.\" $XFree86: xc/programs/Xserver/hw/xfree86/input/mouse/mouse.cpp,v 1.6 2000/12/12 18:54:32 dawes Exp $ 
.\" shorthand for double quote that works everywhere.
.ds q \N'34'
.TH MOUSE __drivermansuffix__ "Version 4.0.2"  "XFree86"
.SH NAME
mouse \- Mouse input driver
.SH SYNOPSIS
.nf
.B "Section \*qInputDevice\*q"
.BI "  Identifier \*q" idevname \*q
.B  "  Driver \*qmouse\*q"
.BI "  Option \*qProtocol\*q \*q" protoname \*q
.BI "  Option \*qDevice\*q   \*q" devpath \*q
\ \ ...
.B EndSection
.fi
.SH DESCRIPTION
.B mouse 
is an XFree86 input driver for mice.  The driver supports most available
mouse types and interfaces.  USB mice are only supported on some OSs,
and the level of support for PS/2 mice depends on the OS.
.PP
The
.B mouse
driver functions as a pointer input device, and may be used as the
X server's core pointer.  Multiple mice are supported by multiple
instances of this driver.
.SH SUPPORTED HARDWARE
There is a detailed list of hardware that the
.B mouse
driver supports in the
.I README.mouse
document.  This can be found
in __projectroot__/lib/X11/doc/, or online at
http://www.xfree86.org/current/mouse.html.
.SH CONFIGURATION DETAILS
Please refer to XF86Config(__filemansuffix__) for general configuration
details and for options that can be used with all input drivers.  This
section only covers configuration details specific to this driver.
.PP
The driver can auto-detect the mouse type on some platforms  On some
platforms this is limited to plug and play serial mice, and on some the
auto-detection works for any mouse that the OS's kernel driver supports.
On others, it is always necessary to specify the mouse protocol in the
config file.  The
.I README.mouse
document contains some detailed information about this.
.PP
The following driver
.B Options
are supported:
.TP 7
.BI "Option \*qProtocol\*q \*q" string \*q
Specify the mouse protocol.  Valid protocol types include:
.PP
.RS 12
Auto, Microsoft, MouseSystems, MMSeries, Logitech, MouseMan, MMHitTab,
GlidePoint, IntelliMouse, ThinkingMouse, AceCad, PS/2, ImPS/2,
ExplorerPS/2, ThinkingMousePS/2, MouseManPlusPS/2, GlidePointPS/2,
NetMousePS/2, NetScrollPS/2, BusMouse, SysMouse, WSMouse, USB, Xqueue.
.RE
.PP
.RS 7
Not all protocols are supported on all platforms.  The "Auto" platform
specifies that protocol auto-detection should be attempted.  There is no
default protocol setting, and specifying this option is mandatory.
.RE
.TP 7
.BI "Option \*qDevice\*q \*q" string \*q
Specifies the device through which the mouse can be accessed.  A common
setting is "/dev/mouse", which is often a symbolic link to the real
device.  This option is mandatory, and there is no default setting.
.TP 7
.BI "Option \*qButtons\*q \*q" integer \*q
Specifies the number of mouse buttons.  In cases where the number of buttons
cannot be auto-detected, the default value is 3.
.TP 7
.BI "Option \*qEmulate3Buttons\*q \*q" boolean \*q
Enable/disable the emulation of the third (middle) mouse button for mice
which only have two physical buttons.  The third button is emulated by
pressing both buttons simultaneously.  Default: off
.TP 7
.BI "Option \*qEmulate3Timeout\*q \*q" integer \*q
Sets the timeout (in milliseconds) that the driver waits before deciding
if two buttons where pressed "simultaneously" when 3 button emulation is
enabled.  Default: 50.
.TP 7
.BI "Option \*qChordMiddle\*q \*q" boolean \*q
Enable/disable handling of mice that send left+right events when the middle
button is used.  Default: off.
.TP 7
.BI "Option \*qZAxisMapping\*q \*qX\*q"
.TP 7
.BI "Option \*qZAxisMapping\*q \*qY\*q"
.TP 7
.BI "Option \*qZAxisMapping\*q \*q" "N1 N2" \*q
.TP 7
.BI "Option \*qZAxisMapping\*q \*q" "N1 N2 N3 N4" \*q
Set the mapping for the Z axis (wheel) motion to buttons or another axis
.RB ( X
or
.BR Y ).
Button number
.I N1
is mapped to the negative Z axis motion and button number
.I N2
is mapped to the positive Z axis motion.  For mice with two wheels,
four button numbers can be specified, with the negative and positive motion
of the second wheel mapped respectively to buttons number
.I N3
and
.IR N4 .
.TP 7
.BI "Option \*qFlipXY\*q \*q" boolean \*q
Enable/disable swapping the X and Y axes.  Default: off.
.TP 7
.BI "Option \*qSampleRate\*q \*q" integer \*q
Sets the number of motion/button events the mouse sends per second.  Setting
this is only supported for some mice, including some Logitech mice and
some PS/2 mice on some platforms.  Default: whatever the mouse is
already set to.
.TP 7
.BI "Option \*qResolution\*q \*q" integer \*q
Sets the resolution of the device in counts per inch.  Setting this is
only supported for some mice, including some PS/2 mice on some platforms.
Default: whatever the mouse is already set to.
.TP 7
.BI "Option \*qClearDTR\*q \*q" boolean \*q
Enable/disable clearing the DTR line on the serial port used by the mouse.
Some dual-protocol mice require the DTR line to be cleared to operate
in the non-default protocol.  This option is for serial mice only.
Default: off.
.TP 7
.BI "Option \*qClearRTS\*q \*q" boolean \*q
Enable/disable clearing the RTS line on the serial port used by the mouse.
Some dual-protocol mice require the RTS line to be cleared to operate
in the non-default protocol.  This option is for serial mice only.
Default: off.
.TP 7
.BI "Option \*qBaudRate\*q \*q" integer \*q
Set the baud rate to use for communicating with a serial mouse.  This
option should rarely be required because the default is correct for almost
all situations.  Valid values include: 300, 1200, 2400, 4800, 9600, 19200.
Default: 1200.
.PP
There are some other options that may be used to control various parameters
for serial port communication, but they are not documented here because
the driver sets them correctly for each mouse protocol type.
.SH "SEE ALSO"
XFree86(1), XF86Config(__filemansuffix__), xf86config(1), Xserver(1), X(__miscmansuffix__),
README.mouse.
