.\" $XFree86: xc/programs/Xserver/hw/xfree86/drivers/glide/glide.cpp,v 1.11 2000/12/11 20:18:12 dawes Exp $
.\" shorthand for double quote that works everywhere.
.ds q \N'34'
.TH GLIDE __drivermansuffix__ "Version 4.0.2"  "XFree86"
.SH NAME
glide \- Glide video driver
.SH SYNOPSIS
.nf
.B "Section \*qDevice\*q"
.BI "  Identifier \*q"  devname \*q
.B  "  Driver \*qglide\*q"
\ \ ...
.B EndSection
.fi
.SH READ THIS IF NOTHING ELSE
This driver has a special requirement that needs to be fulfilled
before it will work: You need Glide installed and you need to make a link for the libglide2x.so
file. Read the second paragraph in the description below to find out how.
.SH DESCRIPTION
.B glide 
is an XFree86 driver for Glide capable video boards (such as 3Dfx
Voodoo boards). This driver is mainly for Voodoo 1 and Voodoo 2 boards, later
boards from 3Dfx have 2D built-in and you should preferably use a driver separate for
those boards or the fbdev(__drivermansuffix__) driver.
This driver is a bit special because Voodoo 1 and 2 boards are
very much NOT made for running 2D graphics. Therefore, this driver
uses no hardware acceleration (since there is no acceleration for 2D,
only 3D). Instead it is implemented with the help of a "shadow"
framebuffer that resides entirely in RAM. Selected portions of this
shadow framebuffer are then copied out to the Voodoo board at the right
time. Because of this, the speed of the driver is very dependent on
the CPU. But since the CPU is nowadays actually rather fast at moving
data, we get very good speed anyway, especially since the whole shadow
framebuffer is in cached RAM.
.PP
This driver requires that you have installed Glide. (Which can, at the
time of this writing, be found at
http://glide.xxedgexx.com/3DfxRPMS.html). Also, you need to tell
XFree86 where the libglide2x.so file is placed by making a soft link
in the /usr/X11R6/lib/modules directory that points to the libglide2x.so
file. For example (if your libglide2x.so file is in /usr/lib):
.PP
  # ln -s /usr/lib/libglide2x.so /usr/X11R6/lib/modules
.PP
If you have installed /dev/3dfx, the driver will be able to turn on
the MTRR registers (through the glide library) if you have a CPU with
such registers (see http://glide.xxedgexx.com/MTRR.html). This will
speed up copying data to the Voodoo board by as much as 2.7 times and
is very noticeable since this driver copies a lot of
data... Highly recommended.
.PP
This driver supports 16 and 24 bit color modes. The 24 bit color mode
uses a 32 bit framebuffer (it has no support for 24 bit packed-pixel
framebuffers). Notice that the Voodoo boards can only display 16 bit
color, but the shadow framebuffer can be run in 24 bit color. The
point of supporting 24 bit mode is that this enables you to run in a
multihead configuration with Xinerama together with another board that
runs in real 24 bit color mode. (All boards must run the same color
depth when you use Xinerama).
.PP
Resolutions supported are: 640x480, 800x600, 960x720, 1024x768,
1280x1024 and 1600x1200. Note that not all modes will work on all
Voodoo boards. It seems that Voodoo 2 boards support no higher than
1024x768 and Voodoo 1 boards can go to 800x600. If you see a message like this in the output from the server:
.PP
  (EE) GLIDE(0): grSstWinOpen returned ...
.PP
Then you are probably trying to use a resolution that is supported by
the driver but not supported by the hardware.
.PP
Refresh rates supported are: 60Hz, 75Hz and 85Hz. The refresh rate
used is derived from the normal mode line according
to the following table:
.TP 28
Mode-line refresh rate
Used refresh rate
.TP 28
   0-74 Hz
  60 Hz
.TP 28
  74-84 Hz
  75 Hz
.TP 28
  84-   Hz
  85 Hz
.PP
Thus, if you use a modeline that for example has a 70Hz refresh rate 
you will only get a 60Hz refresh rate in actuality.
.PP
Selecting which Voodoo board to use with the driver is done by using
an option called "GlideDevice" in the "Device" section. (If you don't
have this option present then the first board found will be selected for that Device section). For
example: To use the first Voodoo board, use a "Device" section like
this, for example:
.PP
Section "Device"
.br
   Identifier  "Voodoo"
.br
   Driver      "glide"
.br
   Option      "dpms" "on"
.br
   Option      "GlideDevice" "0"
.br
EndSection
.PP
And if you have more than one Voodoo board, add another "Device"
section with a GlideDevice option with value 1, and so on. (You can use more than one
Voodoo board, but SLI configured boards will be treated as a single board.)
.PP
Multihead and Xinerama configurations are supported.
.PP
Limited support for DPMS screen saving is available. The "standby" and
"suspend" modes are just painting the screen black. The "off" mode turns
the Voodoo board off and thus works correctly.
.PP
This driver does not support a virtual screen size different from the display size.
.SH SUPPORTED HARDWARE
The
.B glide
driver supports any board that can be used with Glide (such as 3Dfx Voodoo boards)
.SH CONFIGURATION DETAILS
Please refer to XF86Config(__filemansuffix__) for general configuration
details.  This section only covers configuration details specific to this
driver.
.PP
The following driver
.B Options
are supported:
.TP
.BI "Option \*qOnAtExit\*q \*q" boolean \*q
If true, will leave the Voodoo board on when the server exits. Useful in a multihead setup when
only the Voodoo board is connected to a second monitor and you don't want that monitor to lose
signal when you quit the server. Put this option in the Device section.
Default: off.
.TP
.BI "Option \*qGlideDevice\*q \*q" integer \*q
Selects which Voodoo board to use. (Or boards, in an SLI configuration).
The value should be 0 for the first board, 1 for the second and so on.
If it is not present, the first Voodoo board found will be selected.
Put this option in the Device section.
.SH "EXAMPLE"
Here is an example of a part of an XF86Config file that uses a multihead
configuration with two monitors. The first monitor is driven by the
fbdev video driver and the second monitor is driven by the glide
driver.
.PP
.br
Section "Monitor"
.br
   Identifier      "Monitor 1"
.br
   VendorName      "Unknown"
.br
   ModelName       "Unknown"
.br
   HorizSync       30-70
.br
   VertRefresh     50-80
.br

.br
   # 1024x768 @ 76 Hz, 62.5 kHz hsync
.br
   Modeline "1024x768" 85 1024 1032 1152 1360 768 784 787 823
.br
EndSection
.br

.br
Section "Monitor"
.br
   Identifier      "Monitor 2"
.br
   VendorName      "Unknown"
.br
   ModelName       "Unknown"
.br
   HorizSync       30-70
.br
   VertRefresh     50-80
.br

.br
   # 1024x768 @ 76 Hz, 62.5 kHz hsync
.br
   Modeline "1024x768" 85 1024 1032 1152 1360 768 784 787 823
.br
EndSection
.br

.br
Section "Device"
.br
   Identifier  "fb"
.br
   Driver      "fbdev"
.br
   Option      "shadowfb"
.br
   Option      "dpms" "on"
.br
   # My video card is on the AGP bus which is usually
.br
   # located as PCI bus 1, device 0, function 0.
.br
   BusID       "PCI:1:0:0"
.br
EndSection
.br

.br
Section "Device"
.br
   # I have a Voodoo 2 board
.br
   Identifier  "Voodoo"
.br
   Driver      "glide"
.br
   Option      "dpms" "on"
.br
   # The next line says I want to use the first board.
.br
   Option      "GlideDevice" "0"
.br
EndSection
.br

.br
Section "Screen"
.br
  Identifier	"Screen 1"
.br
  Device	"fb"
.br
  Monitor	"Monitor 1"
.br
  DefaultDepth	16
.br
  Subsection "Display"
.br
    Depth	16
.br
    Modes	"1024x768"
.br
  EndSubSection
.br
EndSection
.br

.br
Section "Screen"
.br
  Identifier	"Screen 2"
.br
  Device	"Voodoo"
.br
  Monitor	"Monitor 2"
.br
  DefaultDepth	16
.br
  Subsection "Display"
.br
    Depth	16
.br
    Modes	"1024x768"
.br
  EndSubSection
.br
EndSection
.br

.br
Section "ServerLayout"
.br
  Identifier	"Main Layout"
.br
  # Screen 1 is to the right and screen 2 is to the left
.br
  Screen	"Screen 2" 
.br
  Screen	"Screen 1" "" "" "Screen 2" ""
.br
EndSection
.PP
If you use this configuration file and start the server with the
+xinerama command line option, the two monitors will be showing a
single large area where windows can be moved between monitors and
overlap from one monitor to the other. Starting the X server with the
Xinerama extension can be done for example like this:
.PP
$ xinit -- +xinerama
.SH "SEE ALSO"
XFree86(1), XF86Config(__filemansuffix__), xf86config(1), Xserver(1), X(__miscmansuffix__)
.SH AUTHORS
Author: Henrik Harmsen.
