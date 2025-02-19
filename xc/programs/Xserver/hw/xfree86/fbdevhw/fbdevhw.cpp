.\" $XFree86: xc/programs/Xserver/hw/xfree86/fbdevhw/fbdevhw.cpp,v 1.4 2000/12/11 20:29:50 dawes Exp $ 
.TH FBDEVHW __drivermansuffix__ "Version 4.0.2"  "XFree86"
.SH NAME
fbdevhw \- os-specific submodule for framebuffer device access
.SH DESCRIPTION
.B fbdevhw
provides functions for talking to a framebuffer device.  It is
os-specific.  It is a submodule used by other video drivers.
A
.B fbdevhw
module is currently available for linux framebuffer devices.
.PP
fbdev(__drivermansuffix__) is a non-accelerated driver which runs on top of the
fbdevhw module.  fbdevhw can be used by other drivers too, this
is usually activated with `Option "UseFBDev"' in the device section.
.SH "SEE ALSO"
XFree86(1), XF86Config(__filemansuffix__), xf86config(1), Xserver(1), X(__miscmansuffix__),
fbdev(__drivermansuffix__)
.SH AUTHORS
Authors include: Gerd Knorr, based on the XF68_FBDev Server code
(Martin Schaller, Geert Uytterhoeven).
