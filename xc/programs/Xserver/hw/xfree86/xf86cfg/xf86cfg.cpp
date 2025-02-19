.\"
.\" Copyright (c) 2000 by Conectiva S.A. (http://www.conectiva.com)
.\"
.\" Permission is hereby granted, free of charge, to any person obtaining a
.\" copy of this software and associated documentation files (the "Software"),
.\" to deal in the Software without restriction, including without limitation
.\" the rights to use, copy, modify, merge, publish, distribute, sublicense,
.\" and/or sell copies of the Software, and to permit persons to whom the
.\" Software is furnished to do so, subject to the following conditions:
.\"
.\" The above copyright notice and this permission notice shall be included in
.\" all copies or substantial portions of the Software.
.\"
.\" THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
.\" IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
.\" FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
.\" CONECTIVA LINUX BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
.\" WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
.\" OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
.\" SOFTWARE.
.\"
.\" Except as contained in this notice, the name of Conectiva Linux shall
.\" not be used in advertising or otherwise to promote the sale, use or other
.\" dealings in this Software without prior written authorization from
.\" Conectiva Linux.
.\"
.\" Author: Paulo C�sar Pereira de Andrade <pcpa@conectiva.com.br>
.\"
.\" $XFree86: xc/programs/Xserver/hw/xfree86/xf86cfg/xf86cfg.cpp,v 1.1 2000/12/11 20:48:56 dawes Exp $
.\"
.TH xf86cfg 1 "Version 4.0.2" "XFree86"
.SH NAME
xf86cfg - Graphical configuration tool for XFree86 4.0
.SH SYNOPSIS
.B xf86cfg
[-xf86config \fIXF86Config\fP] [-modulepath \fImoduledir\fP] 
[-fontpath \fIfontsdir\fP] [-toolkitoption ...]
.SH DESCRITPION
.I Xf86cfg
is a tool to configure \fIXFree86 4.0\fP, and can be used to either write the
initial configuration file or make customizations to the current configuration.
.PP
When the \fBDISPLAY\fP environment variable is not set, xf86cfg will run
the command \fIXFree86 -configure\fP to allow the xserver detect the
hardware in the computer, and write an initial \fIXF86Config\fP file
in the user's home directory. Then, it will start XFree86 and allow
customizations.
.br
If the \fBDISPLAY\fP environment variable is set, xf86cfg will read the
default \fIXF86Config\fP, that may not be the same being used by the current
server, and allow customizations.
.PP
To use an alternative location for modules or fonts the respective search
paths may be specified.
.PP
Unless there is an \fBApply\fP button in the current xf86cfg dialog, the
changes made will take place the next time \fIXFree86\fP is started.

.PP
Xf86cfg allows addition and configuration of new devices, such as video cards,
monitors, keyboards and mouses.
.PP
Screen layout configuration for xinerama or traditional multi-head is also
available.
.PP
Modelines can be configured or optimized.
.PP
AccessX basic configurations can be made in the xf86cfg's accessx section.

.SH OPTIONS
.TP 8
.I -xf86config
Specifies an alternate XF86Config file for configuration.
.TP 8
.I -modulepath
Specifies where xf86cfg, and the server it may start, should look for
XFree86 modules.
.TP 8
.I -serverpath
Specifies the complete path, not including the binary name, of the
XFree86 binary.
.TP 8
.I -fontpath
Specifies the path to the fonts that should be used by the server started
by xf86cfg.
.TP 8
.I -rgbpath
Specifies the path to the rgb.txt file that should be used by the server
started by xf86cfg, if any.
.TP 8
.I -textmode
If xf86cfg was compiled with support to ncurses, this option makes xf86cfg
enters a text mode interface.

.SH ENVIRONMENT
.TP 8
.I DISPLAY
Default host and display number
.TP 8
.I XWINHOME
Directory where XFree86 was installed, defaults to /usr/X11R6.
.TP 8
.I XENVIRONMENT
Name of a resource file that overrides the global resources
stored in the RESOURCE_MANAGER property

.SH FILES
.TP 8
.I /etc/XF86Config
Server configuration file
.TP 8
.I /etc/X11/XF86Config
Server configuration file
.TP 8
.I /usr/X11R6/etc/XF86Config
Server configuration file
.TP 8
.I <XRoot>/lib/X11/XF86Config.\fIhostname\fP
Server configuration file
.TP 8
.I <XRoot>/lib/X11/XF86Config
Server configuration file
.TP 8
.I <XRoot>/lib/X11/app-default/XF86Cfg
Specifies xf86cfg resources
.TP 8
.I <Xroot>/lib/X11/xkb/X0-config.keyboard
Keyboard specific configuration

.SH "SEE ALSO"
.IR XFree86 (1)
.IR XF86Config (__filemansuffix__)

.SH COPYRIGHT
.TP 8
Copyright 2000, Conectiva Linux S.A.
\fIhttp://www.conectiva.com\fP
.TP 8
Copyright 2000, The XFree86 Project
\fIhttp://www.XFree86.org\fP

.SH AUTHORS
.TP 8
Paulo C�sar Pereira de Andrade \fI<pcpa@conectiva.com.br>\fP
The XFree86 Project

.SH BUGS
Probably.
