.\" $XFree86: xc/programs/Xserver/hw/xfree86/input/keyboard/keyboard.cpp,v 1.5 2000/12/11 20:18:50 dawes Exp $ 
.\" shorthand for double quote that works everywhere.
.ds q \N'34'
.TH KEYBOARD __drivermansuffix__ "Version 4.0.2"  "XFree86"
.SH NAME
keyboard \- Keyboard input driver
.SH SYNOPSIS
.nf
.B "Section \*qInputDevice\*q"
.BI "  Identifier \*q" idevname \*q
.B  "  Driver \*qkeyboard\*q"
\ \ ...
.B EndSection
.fi
.SH DESCRIPTION
.B keyboard 
is an XFree86 input driver for keyboards.  The driver supports the standard
OS-provided keyboard interface.  This driver is currently built-in to
the core X server.
.PP
The
.B keyboard
driver functions as a keyboard input device, and may be used as the
X server's core keyboard.  This driver is currently built-in to the core
X server, and multiple instances are not yet supported.
.SH CONFIGURATION DETAILS
Please refer to XF86Config(__filemansuffix__) for general configuration
details and for options that can be used with all input drivers.  This
section only covers configuration details specific to this driver.
.PP
The following driver
.B Options
are supported:
.TP 7
.BI "Option \*qProtocol\*q \*q" string \*q
Specify the mouse protocol.  Valid protocol types include:
.PP
.RS 12
Standard, Xqueue.
.RE
.PP
.RS 7
Not all protocols are supported on all platforms.  Default: "Standard".
.RE
.TP 7
.BI "Option \*qAutoRepeat\*q \*q" "delay rate" \*q
sets the auto repeat behaviour for the keyboard.  This is not implemented
on all platforms.
.I delay
is the time in milliseconds before a key starts repeating.
.I rate
is the number of times a key repeats per second.  Default: "500 30".
.TP 7
.BI "Option \*qXLeds\*q \*q" ledlist \*q
makes the keyboard LEDs specified in
.I ledlist
available for client use instead of their traditional function
(Scroll Lock, Caps Lock and Num Lock).  The numbers in the list are
in the range 1 to 3.  Default: empty list.
.TP 7
.BI "Option \*qXkbDisable\*q \*q" boolean \*q
disable/enable the XKEYBOARD extension.  The \-kb command line
option overrides this config file option.  Default: XKB is enabled.
.TP 7
.BI "Option \*qXkbRules\*q \*q" rules \*q
specifies which XKB rules file to use for interpreting the
.BR XkbModel ,
.BR XkbLayout ,
.BR XkbVariant ,
and
.B XkbOptions
settings.  Default: "xfree86" for most platforms, but "xfree98" for the
Japanese PC-98 platforms.
.TP 7
.BI "Option \*qXkbModel\*q \*q" modelname \*q
specifies the XKB keyboard model name.  Default: "pc101" for most platforms,
but "pc98" for the Japanese PC-98 platforms, and "pc101_sol8x86" for
Solaris 8 on x86.
.TP 7
.BI "Option \*qXkbLayout\*q \*q" layoutname \*q
specifies the XKB keyboard layout name.  This is usually the country or
language type of the keyboard.  Default: "us" for most platforms, but
"nec/jp" for the Japanese PC-98 platforms.
.TP 7
.BI "Option \*qXkbVariant\*q \*q" variants \*q
specifies the XKB keyboard variant components.  These can be used to
enhance the keyboard layout details.  Default: not set.
.TP 7
.BI "Option \*qXkbOptions\*q \*q" options \*q
specifies the XKB keyboard option components.  These can be used to
enhance the keyboard behaviour.  Default: not set.
.PP
Some other XKB-related options are available, but they are incompatible
with the ones listed above and are not recommended, so they are not
documented here.
.SH "SEE ALSO"
XFree86(1), XF86Config(__filemansuffix__), xf86config(1), Xserver(1), X(__miscmansuffix__).
