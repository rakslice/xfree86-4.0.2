.\" $XFree86: xc/programs/Xserver/hw/xfree86/drivers/s3virge/s3virge.cpp,v 1.12 2000/12/12 18:54:31 dawes Exp $
.\" shorthand for double quote that works everywhere.
.ds q \N'34'
.TH s3virge __drivermansuffix__ "Version 4.0.2"  "XFree86"
.SH NAME
s3virge \- S3 ViRGE video driver
.SH SYNOPSIS
.B "Section \*qDevice\*q"
.br
.BI "  Identifier \*q"  devname \*q
.br
.B  "  Driver \*qs3virge\*q"
.br
\ \ ...
.br
\ \ [
.B "Option"
"optionname" ["optionvalue"]]
.br
.B EndSection
.SH DESCRIPTION
.B s3virge 
is an XFree86 driver for S3 based video cards.  The driver is fully
accelerated, and provides support for the following framebuffer depths:
8, 15, 16, and 24.  All
visual types are supported for depth 8, and TrueColor
visuals are supported for the other depths.
.SH SUPPORTED HARDWARE
The
.B s3virge
driver supports PCI and AGP video cards based on the following S3 chips:
.TP 12
.B ViRGE
86C325
.TP 12
.B ViRGE VX
86C988
.TP 12
.B ViRGE DX
86C375
.TP 12
.B ViRGE GX
86C385
.TP 12
.B ViRGE GX2
86C357
.TP 12
.B ViRGE MX
86C260
.TP 12
.B ViRGE MX+
86C280
.TP 12
.B Trio 3D
86C365
.TP 12
.B Trio 3D/2X
86C362, 86C368
.SH CONFIGURATION DETAILS
Please refer to XF86Config(__filemansuffix__) for general configuration
details.  This section only covers configuration details specific to this
driver.  All options names are case and white space insensitive when
parsed by the server, for example,  "virge vx" and "VIRGEvx" are equivalent.
.PP
The driver auto-detects the chipset type, but the following
.B ChipSet
names may optionally be specified in the config file
.B \*q"Device\*q"
section, and will override the auto-detection:
.PP
.RS 4
"virge", "86c325", "virge vx", "86c988", "virge dx", "86c375",
"virge gx", "86c385", "virge gx2", "86c357", "virge mx", "86c260",
"virge mx+", "86c280", "trio 3d", "86c365", "trio 3d/2x", "86c362",
"86c368".
.RE

.PP
The following Cursor
.B Options
are supported:
.TP
.BI "Option \*qHWCursor\*q [\*q" boolean \*q]
Enable or disable the HW cursor.  Default: on.
.TP
.BI "Option \*qSWCursor\*q [\*q" boolean \*q]
Inverse of "HWCursor".  Default: off.

.PP
The following display
.B Options
are supported:
.TP
.BI "Option \*qShadowFB\*q [\*q" boolean \*q]
Use shadow framebuffer.  Disables HW acceleration.  Default: off.
.TP
.BR "Option \*qRotate\*q \*q" cw " | " ccw \*q
Rotate the screen CW - clockwise or CCW - counter clockwise.
Disables HW Acceleration and HW Cursor, uses ShadowFB.
Default: no rotation.

.PP
The following video memory
.B Options
are supported:
.TP
.BI "Option \*qslow_edodram\*q"
Switch the standard ViRGE to 2-cycle edo mode. Try this
if you encounter pixel corruption on the ViRGE. Using this option will
cause a large decrease in performance.  Default: off.
.TP
.BI "Option \*qfpm_vram\*q"
Switch the ViRGE/VX to fast page mode vram mode.  Default: off.
.TP
.BR "Option \*qslow_dram " | " fast_dram\*q"
Change Trio 3D and 3D/2X memory options.  Default: Use BIOS defaults.
.TP
.BR "Option \*qearly_ras_precharge " | " late_ras_precharge\*q"
adjust memory parameters.  One
of these will us the same settings as your video card defaults, and
using neither in the config file does the same.  Default: none.
.TP
.BI "Option \*qset_mclk\*q \*q" integer \*q
sets the memory clock, where
.I integer
is in kHz, and
.I integer
<= 100000.  Default: probe the memory clock value,
and use it at server start.
.TP
.BI "Option \*qset_refclk\*q \*q" integer \*q
sets the ref clock for ViRGE MX, where
.I integer
is in kHz.  Default: probe the memory clock value,
and use it at server start.


.PP
The following acceleration and graphics engine
.B Options
are supported:
.TP
.B "Option \*qNoAccel\*q"
Disable acceleration.  Very useful for determining if the
driver has problems with drawing and acceleration routines.  This is the first
option to try if your server runs but you see graphic corruption on the screen.
Using it decreases performance, as it uses software emulation for drawing
operations the video driver can accelerate with hardware.
Default: acceleration is enabled.
.TP
.B "Option \*qUseFB\*q"
There are two framebuffer rendering methods.  fb and cfb.  Both are
available in the driver.  fb is the newer and default method.  To switch
back to cfb use this option with no, off or other negative parameter.
Default: on.
.TP
.BR "Option \*qfifo_aggressive " | " fifo_moderate " | " fifo_conservative\*q"
alter the settings
for the threshold at which the pixel FIFO takes over the internal 
memory bus to refill itself. The smaller this threshold, the better
the acceleration performance of the card. You may try the fastest 
setting
.RB ( "fifo_aggressive" )
and move down if you encounter pixel corruption.
The optimal setting will probably depend on dot-clock and on color 
depth. Note that specifying any of these options will also alter other
memory settings which may increase performance, so trying
.B "fifo_conservative"
will in most cases be a slight benefit (this uses the chip defaults).
If pixel corruption or transient streaking is observed during drawing
operations then removing any fifo options is recommended.  Default: none.

.PP
The following PCI bus
.B Options
are supported:
.TP
.BI "Option \*qpci_burst\*q [\*q" boolean \*q]
will enable PCI burst mode. This should work on all but a
few broken PCI chipsets, and will increase performance.  Default: off.
.TP
.BI "Option \*qpci_retry\*q [\*q" boolean \*q]
will allow the driver to rely on PCI Retry to program the 
ViRGE registers. 
.B "pci_burst"
must be enabled for this to work. 
This will increase performance, especially for small fills/blits, 
because the driver does not have to poll the ViRGE before sending it 
commands to make sure it is ready. It should work on most 
recent PCI chipsets.  Default: off.
.PP
The following ViRGE MX LCD
.B Options
are supported:
.TP
.BI "Option \*qlcd_center\*q"
.TP
.BI "Option \*qset_lcdclk\*q \*q" integer \*q
allows setting the clock for a ViRGE MX LCD display. 
.I integer
is in Hz.  Default: use probed value.

.PP
The following additional
.B Options
are supported:
.TP
.BI "Option \*qShowCache\*q [\*q" boolean \*q]
Enable or disable viewing offscreen cache memory.  A
development debug option.  Default: off.

.SH SEE ALSO
XFree86(1), XF86Config(__filemansuffix__), xf86config(1), Xserver(1), X(__miscmansuffix__)

.SH SUPPORT
For assistance with this driver, or XFree86 in general, check the XFree86 web
site at http://www.xfree86.org.  A FAQ is available on the web site at
http://www.xfree86.org/FAQ/.  If you find a problem with XFree86 or have a
question not answered in the FAQ please use our bug report form available on
the web site or send mail to XFree86@XFree86.org.  When reporting problems
with the driver send as much detail as possible, including chipset type, a 
server output log, and operating system specifics.

.SH AUTHORS
Kevin Brosius, 
Matt Grossman, 
Harald Koenig,
Sebastien Marineau,
Mark Vojkovich.
