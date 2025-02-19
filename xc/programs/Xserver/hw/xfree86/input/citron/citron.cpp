.\" Copyright (c) 2000 Peter Kunzmann <support@@citron.de>
.\"
.\" $XFree86: xc/programs/Xserver/hw/xfree86/input/citron/citron.cpp,v 1.4 2000/12/17 22:27:40 dawes Exp $
.\"
.\" shorthand for double quote that works everywhere.
.ds q \N'34'
.TH CITRON __drivermansuffix__ "Version 4.0.2" "XFREE86"
.SH NAME
citron \- Citron Infrared Touch Driver (CiTouch)
.SH SYNOPSIS
.nf
.B "Section \*qInputDevice\*q"
.BI "  Identifier \*q" idevname \*q
.B  "  Driver \*qcitron\*q"
.BI "  Option \*qDevice\*q   \*q" devpath \*q
\ \ ...
.B EndSection
.fi
.SH DESCRIPTION
.B citron 
is a XFree86 input driver for 
.I Citron Infrared Touch
devices.
.PP
The
.B citron
driver acts as a pointer input device, and may be used as the
X server's core pointer. It is connected via a "RS232" with the host.
.SH SUPPORTED HARDWARE
At the moment the following touches are supported. They are also
available as 
.I ZPress
touches.
 
.B IRT6I5-V2.x
 6.5 inch Infrared Touch

.B IRT10I4-V4.x
 10.4 inch Infrared Touch

.B IRT12I1-V2.x
 12.1 inch Infrared Touch

.B IRT15I1-V1.x 
 15.1 inch Infrared Touch

.SH CONFIGURATION DETAILS
Please refer to XF86Config(__filemansuffix__) for general configuration
details and for options that can be used with all input drivers.  This
section only covers configuration details specific to this driver.
For better understanding please read also the 
.B CTS
and various
.B IRT
manuals which are available in "pdf" format from Citron web page 
.B www.citron.de
or directly from Citron.

.PP
.PP
The following driver
.B Options
are supported:
.TP 7
.BI "Option \*qDevice\*q \*q" devpath \*q
Specify the device path for the citron touch.  Valid devices are:
.PP
.RS 12
/dev/ttyS0, /dev/ttyS1, ...
.RE
.RS 7
This option is mandatory.
.PP
It's important to specify the right device Note: com1 -> /dev/ttyS0, com2 -> /dev/ttyS1, ...

.RE
.TP 7
.BI "Option \*qScreenNumber\*q \*q" screennumber \*q
sets the
.I screennumber
for the 
.B citron
InputDevice.
.PP
.RS 7
.I Default: 
ScreenNumber: "0"

.RE
.TP 7
.BI "Option \*qMinX\*q \*q" value \*q
.TP 7
.BI "Option \*qMinY\*q \*q" value \*q
These are the minimum X and Y values for the 
.B citron
input device.
.PP
.RS 7
Note: MinX, MinY must be less than MaxX, MaxY.
.PP
.I Range: 
"0" - "65535"
.PP
.I Default:
MinX: "0"  MinY: "0"


.RE
.TP 7
.BI "Option \*qMaxX\*q \*q" value \*q
.TP 7
.BI "Option \*qMaxY\*q \*q" value \*q
These are the maximum X and Y values for the 
.B citron
input device.
.PP
.RS 7
Note: MaxX, MaxY must be greater than MinX, MinY.
.PP
.I Range: 
"0" - "65535"
.PP
.I Default:
MaxX: "65535"  MaxY: "65535"


.RE
.TP 7
.BI "Option \*qButtonNumber\*q \*q" value \*q
This value is responsible for the 
.I button number
that is returned within the xf86PostButton event message 
.PP
.RS 7
.I Range: 
"0" - "255"
.PP
.I Default:
"1"

.RE
.TP 7
.BI "Option \*qButtonThreshold\*q \*q" value \*q
This value is responsible for the 
.I button threshold.
It changes the pressure sensitivity of the touch. A higher number
corresponds to a higher pressure.
.PP
.RS 7
Note: This feature is only available with pressure sensitive hardware.
.PP
.I Range: 
"0" - "255"
.PP
.I Default:
"20"

.RE
.TP 7
.B Sleep-Mode
If the IRT is in 
.I Doze-Mode
and Touch Zone is not interrupted for another 
certain span of time, the so-called
.I Sleep-Mode
is activated. The 
.I Sleep-Mode
decreases the scan rate of the beams even further than
the 
.I Doze-Mode
does (see below). This way the life expectancy of the beams is 
prolonged and the power consumption of the IRT is reduced. 
As soon as an interruption of the Touch Zone is detected, the 
.I Sleep-Mode
is deactivated and the Touch Zone will again be scanned with 
the maximum speed. With the Sleep-Mode activated, 
depending on the set scan rate the IRT's response time can be
considerably longer as in normal operation. If, for example, 
a scan rate of 500 ms / scan is set, it may last up to a half 
of a second until the IRT detects the interruption and deactivates 
the 
.I Sleep-Mode.

.PP
.RE
.TP 7
.BI "Option \*qSleepMode\*q \*q" mode \*q
This value is responsible for the 
.I sleep-mode
of the touch. 
.RS 7
Determines the behaviour of the Sleep-Mode.

.B 0x00 
 No message at either activation or deactivation

.B 0x01
 Message at activation

.B 0x02 
 Message at deactivation

.B 0x03 
 Message at activation and deactivation

.B 0x10
GP_OUT output set according to the Sleep-Mode status

.I Values: 
"0" "1" "2" "3" "16"

.I Default:
"0"

.RE
.TP 7
.BI "Option \*qSleepTime\*q \*q" time \*q
This value is responsible for the 
.I sleep-time
of the touch. It is the activation time in seconds 
("0" = immediately activated, "65535" = always deactivated). 
.RS 7
.PP
.I Range: 
"0" - "65535" [s]
.PP
.I Default:
"65535" => deactivated

.RE
.TP 7
.BI "Option \*qSleepScan\*q \*q" scan \*q
This value is responsible for the 
.I scan-time
of the touch. This is the time interval between two scan operations
while in Sleep-Mode. The time interval is set in steps
of milliseconds. 
.RS 7
.PP
.I Range: 
"0" - "65535" [ms]
.PP
.I Default:
"500"

.RE
.TP 7
.BI "Option \*qPWMActive\*q \*q" value \*q
This value determines the mark-to-space ratio of the 
.I PWM
output while in normal operation (sleep-mode not active).
Higher values result in longer pulse widths. This output 
signal can be used in conjunction with the 
.I Citron AWBI
to do backlight-dimming via the touch.
.RS 7
.PP
.I Range: 
"0" - "255" 
.PP
.I Default:
"255" (max. brightness)


.RE
.TP 7
.BI "Option \*qPWMSleep\*q \*q" value \*q
This value determines the mark-to-space ratio of the 
.I PWM
output while in sleep-mode (->
.I SleepMode, SleepScan, SleepTime
) operation (sleep-mode active).
Higher values result in longer pulse widths.
.RS 7
.PP
.I Range: 
"0" - "255" 
.PP
.I Default:
"255" (max. brightness)

.RE
.TP 7
.BI "Option \*qClickMode\*q \*q" mode \*q
With mode one can select between 5 
.I ClickModes

.B \*q1\*q
= ClickMode Enter

With this mode every interruption of the infrared beams will
activate a ButtonPress event and after the interruption a
ButtonRelease event will be sent.

.B \*q2\*q
= ClickMode Dual

With this mode every interruption will sent a Proximity event and
every second interruption a ButtonPress event. With the release of
the interruption (while one interruption is still active) a
ButtonRelease event will be sent.

.B \*q3\*q
= ClickMode Dual Exit

With this mode every interruption will sent a ProximityIn event and
every second interruption a ButtonPress event. With the release of
the interruption (while one interruption is still active) no
ButtonRelease event will be sent. Only if all interruptions are released
a ButtonRelease followed by a ProximityOut event will be sent.

.B \*q4\*q
= ClickMode ZPress

With this mode every interruption will sent a ProximityIn event. Only if
a certain pressure is exceeded a ButtonPress event will occur. If the
pressure falls below a certain limit a ButtonRelease event will be sent.
After also the interruption is released a ProximityOut event is generated.

.B \*q5\*q
= ClickMode ZPress Exit

This mode is similar to "Clickmode Dual Exit". 
The first interruption of the beams will sent a ProximityIn event. Only if
a certain pressure is exceeded a ButtonPress event will occur. If the
pressure falls below a certain limit no ButtonRelease event will be sent.
After the interruption is also released a ButtonRelease followed by
a ProximityOut event is generated.
.RS 7
.PP
.I Range: 
"1" - "5" 
.PP
.I Default:
"1" (ClickMode Enter)

.RE
.TP 7
.BI "Option \*qOrigin\*q \*q" value \*q
This value sets the coordinates origin to one of the four corners of 
the screen.
The following values are accepted:
"0" TOPLEFT: Origin set to the left-hand side top corner.
"1" TOPRIGHT: Origin set to the right-hand side top corner.
"2" BOTTOMRIGHT: Origin set to the right-hand side bottom corner.
"3" BOTTOMLEFT: Origin set to the left-hand side bottom corner.
.RS 7
.PP
.I Range: 
"0" - "3" 
.PP
.I Default:
"0" (TOPLEFT)

.RE
.TP 7
.B "Doze-Mode"
If for a certain span of time the Touch Zone is not interrupted,
the so-called Doze-Mode is automatically activated. The activated 
Doze-Mode slightly decreases the scan rate of the beams. This way 
the power consumption of the IRT is reduced. As soon as an 
interruption of the Touch Zone is detected, the Doze-Mode
is deactivated and the Touch Zone will again be scanned with 
the maximum speed.

.RE
.TP 7
.BI "Option \*qDozeMode\*q \*q" mode \*q
This value is responsible for the 
.I doze-mode
of the touch. 
.RS 7
.PP
Determines the behaviour of the Doze-Mode.
.PP
.B 0x00
No message at either activation or deactivation

.B 0x01
Message at activation

.B 0x02
Message at deactivation

.B 0x03
Message at activation and deactivation

.B 0x10
GP_OUT output set according to the Doze-Mode status

If the GP_OUT output is already controlled by the 
.I Sleep-Mode
it is no longer available as an output port anymore.
.PP
.I Values: 
"0" "1" "2" "3" "16"
.PP
.I Default:
"0"



.RE
.TP 7
.BI "Option \*qDozeTime\*q \*q" time \*q
This value is responsible for the 
.I doze-time
of the touch. It is the activation time in seconds 
("0" = immediately activated, "65535" = always deactivated). 
.RS 7
.PP
.I Range: 
"0" - "65535" [s]
.PP
.I Default:
"65535" => deactivated


.RE
.TP 7
.BI "Option \*qDozeScan\*q \*q" scan \*q
This value is responsible for the 
.I scan-time
of the touch. This is the time interval between two scan operations
while in Doze-Mode. The time interval is set in steps
of milliseconds. 
.RS 7
.PP
.I Range: 
"0" - "65535" [ms]
.PP
.I Default:
"500"

.RE
.TP 7
.BI "Option \*qDeltaX\*q \*q" value \*q
This value determines a virtual area at the left and right
side of the current cursor position where the cursor didn't move.
Within this area no "MotionNotify" event will be sent.
.RS 7
.PP
.I Range: 
"0" - "255" 
.PP
.I Default:
"0" (no deltaX)


.RE
.TP 7
.BI "Option \*qDeltaY\*q \*q" value \*q
This value determines a virtual area at the top and bottom
of the current cursor position where the cursor didn't move.
Within this area no "MotionNotify" event will be sent.
.RS 7
.PP
.I Range: 
"0" - "255" 
.PP
.I Default:
"0" (no deltaY)

.RE
.TP 7
.BI "Option \*qBeep\*q \*q" value \*q
This value determines if a "ButtonPress" and/or a "ButtonRelease"
event should sound the buzzer. "0" deactivates the buzzer while
every other value will activate it.
.RS 7
.PP
.I Range: 
"0" - "1" 
.PP
.I Default:
"0" (deactivated)

.RE
.TP 7
.BI "Option \*qPressVol\*q \*q" value \*q
This value determines the volume of the buzzer (0-100%)
when a "ButtonPress" event is sent.
.RS 7
.PP
.I Range: 
"0" - "100" 
.PP
.I Default:
"100" 


.RE
.TP 7
.BI "Option \*qPressPitch\*q \*q" value \*q
This value determines the pitch of the tone
when a "ButtonPress" event is sent.
.RS 7
.PP
.I Range: 
"0" - "3000" 
.PP
.I Default:
"880" 



.RE
.TP 7
.BI "Option \*qPressDur\*q \*q" value \*q
This value determines the duration of the tone in ms
when a "ButtonPress" event is sent.
.RS 7
.PP
.I Range: 
"0" - "255" 
.PP
.I Default:
"15" 

.RE
.TP 7
.BI "Option \*qReleaseVol\*q \*q" value \*q
This value determines the volume of the buzzer (0-100%)
when a "ButtonRelease" event is sent.
.RS 7
.PP
.I Range: 
"0" - "100" 
.PP
.I Default:
"100" 


.RE
.TP 7
.BI "Option \*qReleasePitch\*q \*q" value \*q
This value determines the pitch of the tone when
when a "ButtonRelease" event is sent.
.RS 7
.PP
.I Range: 
"0" - "3000" 
.PP
.I Default:
"1200" 



.RE
.TP 7
.BI "Option \*qReleseDur\*q \*q" value \*q
This value determines the duration of the tone in ms when
when a "ButtonRelease" event is sent.
.RS 7
.PP
.I Range: 
"0" - "255" 
.PP
.I Default:
"10" 



.RE
.TP 7
.BI "Option \*qBeamTimeout\*q \*q" value \*q
Determines the time span in seconds, that has to elapse before a beam is 
considered defective, blanked-out and excluded from the coordinates 
evaluation.
.RS 7
.PP
.I Range: 
"0" - "65535" 
.PP
.I Default:
"30" (30 seconds)




.RE
.TP 7
.BI "Option \*qTouchTime\*q \*q" value \*q
Determines the minimum time span in steps of 10ms for a valid 
interruption. In order for an interruption to be
reported to the host computer as valid, it needs to remain at 
the same spot for at least the time span declared here.
.RS 7
.PP
.I Range: 
"0" - "255" 
.PP
.I Default:
"0" (=6,5 ms)


.RE
.TP 7
.BI "Option \*qEnterCount\*q \*q" count \*q
Number of skipped "enter reports". Reports are sent approx. 
every 20ms.
.RS 7
.PP
.I Range: 
"0" - "31" 
.PP
.I Default:
"3" (3 skipped messages = 60ms)


.RE
.TP 7
.BI "Option \*qDualCount\*q \*q" count \*q
Number of skipped "dual touch error". Reports are sent approx. 
every 20ms. This option is only available for "ZPress" and 
"ZPress Exit" modes.
.RS 7
.PP
.I Range: 
"0" - "31" 
.PP
.I Default:
"2" (2 skipped messages = 40ms)


.SH "SEE ALSO"
XFree86(1), XF86Config(__filemansuffix__), xf86config(1), Xserver(1), X(__miscmansuffix__).
.SH AUTHORS
2000 - written  by  Citron GmbH (support@citron.de)
