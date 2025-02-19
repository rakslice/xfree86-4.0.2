.\" $XFree86: xc/programs/Xserver/hw/xfree86/XFree86.cpp,v 1.4 2000/12/11 20:29:50 dawes Exp $ 
.TH XFree86 1 "Version 4.0.2"  "XFree86"
.SH NAME
XFree86 - X11R6 X server
.SH SYNOPSIS
.B XFree86
[:display] [option ...]
.SH DESCRIPTION
XFree86 is an X servers for UNIX-like OSs on Intel x86 and other platforms.
This work is derived from
.I "X386\ 1.2"
which was contributed to X11R5 by Snitily Graphics Consulting Service.
The current XFree86 release is based on X11R6.3.
The XFree86 X server architecture was redesigned for the 4.0 release, and
it includes among other things a loadable module system donated by
Metro Link, Inc.
.SH CONFIGURATIONS
.PP
.I XFree86
operates under the following operating systems:
.RS .5i
.na
.PP
-- SVR3.2: SCO 3.2.2, 3.2.4, ISC 3.x, 4.x
.br
-- SVR4.0: ESIX, Microport, Dell, UHC, Consensys, MST, ISC, AT&T, NCR
.br
-- SVR4.2: Consensys, Univel (UnixWare)
.br
-- Solaris (x86) 2.5, 2.6
.br
-- FreeBSD 2.1.x, 2.2.x, 3.0-current
.br
-- NetBSD 1.2, 1.3
.br
-- OpenBSD
.ig
.br
-- BSD/386 version 1.1 and BSD/OS 2.0
.br
-- Mach (from CMU)
..
.br
-- Linux
.ig
.br
-- Amoeba version 5.1
.br
-- Minix-386vm version 1.6.25.1
..
.br
-- LynxOS AT versions 2.2.1, 2.3.0 and 2.4.0, LynxOS microSPARC 2.4.0
.ad
.RE
.PP
.SH "NETWORK CONNECTIONS"
\fIXFree86\fP supports connections made using the following reliable
byte-streams:
.TP 4
.I "Local"
\fIXFree86\fP supports local connections via Streams pipe via various mechanisms,
using the following paths (\fIn\fP represents the display number):
.sp .5v
.in 8
.nf
/dev/X/server.\fBn\fR (SVR3 and SVR4)
/dev/X/Nserver.\fBn\fR (SVR4)
.ig
/tmp/.X11-unix/X\fBn\fR (ISC SVR3)
..
/dev/X\fBn\fRS and /dev/X\fBn\fRR (SCO SVR3)
.fi
.in
.sp .5v
On SVR4.0.4, if the \fIAdvanced Compatibility Package\fP 
is installed, and in SVR4.2, \fIXFree86\fP supports local connections 
from clients for SCO XSight/ODT, and (with modifications to the binary) 
clients for ISC SVR3.
.TP 4
.I "Unix Domain"
\fIXFree86\fP uses \fI/tmp/.X11-unix/X\fBn\fR as the filename for the socket,
where \fIn\fP is the display number.
.TP 4
.I TCP\/IP
\fIXFree86\fP listens on port htons(6000+\fIn\fP), where \fIn\fP is the display
number.
.ig
.TP 4
.I "Amoeba RPC"
This is the default communication medium used under native Amoeba.
Note that under Amoeba, the server should be started
with a ``\fIhostname\fP:\fIdisplaynumber\fP'' argument.
..
.SH "ENVIRONMENT VARIABLES"
For operating systems that support local connections other than Unix Domain
sockets (SVR3 and SVR4), there is a compiled-in list specifying the order 
in which local connections should be attempted.  This list can be overridden by
the \fIXLOCAL\fP environment variable described below.  If the display name 
indicates a best-choice connection should be made (e.g. \fI:0.0\fP), each 
connection mechanism is tried until a connection succeeds or no more 
mechanisms are available.  Note: for these OSs, the Unix Domain socket
connection is treated differently from the other local connection types.
To use it the connection must be made to \fIunix:0.0\fP.
.PP
The \fIXLOCAL\fP environment variable should contain a list of one more
more of the following:
.sp .5v
.in 8
.nf
NAMED
PTS
SCO
ISC
.fi
.in
.sp .5v
which represent SVR4 Named Streams pipe, Old-style USL
Streams pipe, SCO XSight Streams pipe, and ISC Streams pipe, respectively.
You can select a single mechanism (e.g. \fIXLOCAL=NAMED\fP), or an ordered
list (e.g. \fIXLOCAL="NAMED:PTS:SCO"\fP).  This variable overrides the
compiled-in defaults.  For SVR4 it is recommended that \fINAMED\fP be
the first preference connection.  The default setting is
\fIPTS:NAMED:ISC:SCO\fP.
.PP
To globally override the compiled-in defaults, you should define (and
export if using \fIsh\fP or \fIksh\fP) \fIXLOCAL\fP globally.  If you
use \fIstartx/xinit\fP, the definition should be at the top of
your \fI.xinitrc\fP file.  If you use \fIxdm\fP, the definitions should be
early on in the \fI<XRoot>/lib/X11/xdm/Xsession\fP script.
.SH OPTIONS
In addition to the normal server options described in the \fIXserver(1)\fP
manual page, \fIXFree86\fP accepts the following command line switches:
.TP 8
.B vt\fIXX\fP
\fIXX\fP specifies the Virtual Terminal device number which 
\fIXFree86\fP will use.  Without this option, \fIXFree86\fP will pick the first
available Virtual Terminal that it can locate.  This option applies only
to SVR3, SVR4, Linux, and BSD OSs with the `syscons' or `pcvt' driver.
.TP 8
.B -crt /dev/tty\fIXX\fP
SCO only.  This is the same as the \fBvt\fP option, and is provided for
compatibility with the native SCO X server.
.TP 8
.B \-probeonly
Causes the server to exit after the device probing stage.  The XF86Config file
is still used when this option is given, so information that can be
auto-detected should be commented out.
.TP 8
.B \-quiet
Suppress most informational messages at startup.
.TP 8
.B \-bpp \fIn\fP
No longer supported.  Use \fB\-depth\fP to set the color depth, and
use \fB\-fbbpp\fP if you really need to force a non-default
framebuffer (hardware) pixel format.
.TP 8
.B \-depth \fIn\fP
Sets the default color depth.  Legal values are 8, 15, 16, and 24.
Not all servers support all values.
.TP 8
.B \-fbbpp \fIn\fP
Sets the number of framebuffer bits per pixel.  You should only set
this if you're sure it's necessary; normally the server can deduce the
correct value from \fB\-depth\fP above.  Useful if you want to run a
depth 24 configuration with a 24 bpp framebuffer rather than the
(possibly default) 32 bpp framebuffer.  Legal values are 8, 16, 24,
32.  Not all servers support all values.
.TP 8
.B \-weight \fInnn\fP
Set RGB weighting at 16 bpp.  The default is 565.  This applies
only to those servers which support 16 bpp.
.TP 8
.B \-flipPixels
Swap the default values for the black and white pixels.
.TP 8
.B \-disableVidMode
Disable the the parts of the VidMode extension used by the xvidtune client
that can be used to change the video modes.
.TP 8
.B \-allowNonLocalXvidtune
Allow the xvidtune client to connect from another host.  By default non-local
connections are not allowed.
.TP 8
.B \-disableModInDev
Disable dynamic modification of input device settings.
.TP 8
.B \-allowNonLocalModInDev
Allow changes to keyboard and mouse settings from non-local clients.
By default, connections from non-local clients are not allowed to do this.
.TP
.B \-allowMouseOpenFail
Allow the server to start up even if the mouse device can't be opened or
initialised.
.TP 8
.B \-gamma \fIvalue\fP
Set the gamma correction.  \fIvalue\fP must be between 0.1 and 10.  The
default is 1.0
This value is applied equally to the R, G and B values.  Not all servers
support this.
.TP 8
.B \-rgamma \fIvalue\fP
Set the red gamma correction.  \fIvalue\fP must be between 0.1 and 10.  The
default is 1.0
Not all servers support this.
.TP 8
.B \-ggamma \fIvalue\fP
Set the green gamma correction.  \fIvalue\fP must be between 0.1 and 10.  The
default is 1.0
Not all servers support this.
.TP 8
.B \-bgamma \fIvalue\fP
Set the blue gamma correction.  \fIvalue\fP must be between 0.1 and 10.  The
default is 1.0
Not all servers support this.
.TP 8
.B \-showconfig
Print out the server version, patchlevel, and a list of screen drivers
configured in the server.
.TP 8
.B \-verbose
Multiple occurrences of this flag increase the amount of information printed on
stderr (more than the default).
.TP 8
.B \-version
Same as \fB\-showconfig\fP.
.TP 8
.B \-xf86config \fIfile\fP
Read the server configuration from \fIfile\fP.  This option will work
for any file when the server is run as root (i.e, with real-uid 0), or
for files relative to a directory in the config search path for all
other users. 
.TP 8
.B \-keeptty
Prevent the server from detaching its initial controlling terminal.  This
option is only useful when debugging the server.
.SH "KEYBOARD"
Multiple key presses recognized directly by \fIXFree86\fP are:
.TP 8
.B Ctrl+Alt+Backspace
Immediately kills the server -- no questions asked.  (Can be disabled by
specifying "DontZap" in the \fBServerFlags\fP section of the XF86Config file.)
.TP 8
.B Ctrl+Alt+Keypad-Plus
Change video mode to next one specified in the configuration file,
(increasing video resolution order).
.TP 8
.B Ctrl+Alt+Keypad-Minus
Change video mode to previous one specified in the configuration file,
(decreasing video resolution order).
.TP 8
.B Ctrl+Alt+F1...F12
For BSD systems using the syscons driver and Linux, these keystroke
combinations are used to switch to Virtual 
Console 1 through 12.
.SH SETUP
.I XFree86
uses a configuration file called \fBXF86Config\fP for its initial setup.  
Refer to the
.I XF86Config(4/5)
manual page for more information.
.SH FILES
.TP 30
/etc/XF86Config
Server configuration file
.TP 30
/etc/X11/XF86Config
Server configuration file
.TP 30
/usr/X11R6/etc/XF86Config
Server configuration file
.TP 30
<XRoot>/lib/X11/XF86Config.\fIhostname\fP
Server configuration file
.TP 30
<XRoot>/lib/X11/XF86Config
Server configuration file
.TP 30
<XRoot>/bin/\(**
Client binaries
.TP 30
<XRoot>/include/\(**
Header files
.TP 30
<XRoot>/lib/\(**
Libraries
.TP 30
<XRoot>/lib/X11/fonts/\(**
Fonts
.TP 30
<XRoot>/lib/X11/rgb.txt
Color names to RGB mapping
.TP 30
<XRoot>/lib/X11/XErrorDB
Client error message database
.TP 30
<XRoot>/lib/X11/app-defaults/\(**
Client resource specifications
.TP 30
<XRoot>/man/man?/\(**
Manual pages
.TP 30
/etc/X\fIn\fP.hosts
Initial access control list for display \fIn\fP
.LP
Note: <XRoot> refers to the root of the X11 install tree.
.SH "SEE ALSO"
X(__miscmansuffix__), Xserver(1), xdm(1), xinit(1),
XF86Config(__filemansuffix__), xf86config(1), xf86cfg(1), xvidtune(1)
.SH AUTHORS
.PP
For X11R5, \fIXF86 1.2\fP was provided by:
.TP 8
Thomas Roell,      \fIroell@informatik.tu-muenchen.de\fP
TU-Muenchen:  Server and SVR4 stuff
.TP 8
Mark W. Snitily,   \fImark@sgcs.com\fP
SGCS:  SVR3 support, X Consortium Sponsor
.PP
 ... and many more people out there on the net who helped with ideas and
bug-fixes.
.PP
XFree86 was integrated into X11R6 by the following team:
.PP
.nf
Stuart Anderson    \fIanderson@metrolink.com\fP
Doug Anson         \fIdanson@lgc.com\fP
Gertjan Akkerman   \fIakkerman@dutiba.twi.tudelft.nl\fP
Mike Bernson       \fImike@mbsun.mlb.org\fP
Robin Cutshaw      \fIrobin@XFree86.org\fP
David Dawes        \fIdawes@XFree86.org\fP
Marc Evans         \fImarc@XFree86.org\fP
Pascal Haible      \fIhaible@izfm.uni-stuttgart.de\fP
Matthieu Herrb     \fIMatthieu.Herrb@laas.fr\fP
Dirk Hohndel       \fIhohndel@XFree86.org\fP
David Holland      \fIdavidh@use.com\fP
Alan Hourihane     \fIalanh@fairlite.demon.co.uk\fP
Jeffrey Hsu        \fIhsu@soda.berkeley.edu\fP
Glenn Lai          \fIglenn@cs.utexas.edu\fP
Ted Lemon          \fImellon@ncd.com\fP
Rich Murphey       \fIrich@XFree86.org\fP
Hans Nasten        \fInasten@everyware.se\fP
Mark Snitily       \fImark@sgcs.com\fP
Randy Terbush      \fIrandyt@cse.unl.edu\fP
Jon Tombs          \fItombs@XFree86.org\fP
Kees Verstoep      \fIversto@cs.vu.nl\fP
Paul Vixie         \fIpaul@vix.com\fP
Mark Weaver        \fIMark_Weaver@brown.edu\fP
David Wexelblat    \fIdwex@XFree86.org\fP
Philip Wheatley    \fIPhilip.Wheatley@ColumbiaSC.NCR.COM\fP
Thomas Wolfram     \fIwolf@prz.tu-berlin.de\fP
Orest Zborowski    \fIorestz@eskimo.com\fP
.fi
.PP
The \fIXFree86\fP enhancement package was provided by:
.TP 8
David Dawes,       \fIdawes@XFree86.org\fP
Release coordination, administration of FTP repository and mailing lists.
Source tree management
and integration, accelerated server integration, fixing, and coding.
.TP 8
Glenn Lai,         \fIglenn@cs.utexas.edu\fP
The SpeedUp code for ET4000 based SVGA cards, and ET4000/W32 accelerated
server.
.TP 8
Jim Tsillas,       \fIjtsilla@ccs.neu.edu\fP
Many server speedups from the fX386 series of enhancements.
.TP 8
David Wexelblat,   \fIdwex@XFree86.org\fP
Integration of the fX386 code into the default server, 
many driver fixes, and driver documentation, assembly of the VGA 
card/monitor database, development of the generic video mode listing.
Accelerated server integration, fixing, and coding.
.TP 8
Dirk Hohndel,      \fIhohndel@XFree86.org\fP
Linux shared libraries and release coordination.  Accelerated server
integration and fixing.  Generic administrivia and documentation.
.PP
.TP 8
Amancio Hasty Jr., \fIhasty@netcom.com\fP
Porting to \fB386BSD\fP version 0.1 and XS3 development.
.TP 8
Rich Murphey,      \fIrich@XFree86.org\fP
Ported to \fB386BSD\fP version 0.1 based on the original port by Pace Willison.
Support for \fB386BSD\fP, \fBFreeBSD\fP, and \fBNetBSD\fP.
.TP 8
Robert Baron,      \fIRobert.Baron@ernst.mach.cs.cmu.edu\fP
Ported to \fBMach\fP.
.TP 8
Orest Zborowski,   \fIorestz@eskimo.com\fP
Ported to \fBLinux\fP.
.TP 8
Doug Anson,        \fIdanson@lgc.com\fP
Ported to \fBSolaris x86\fP.
.TP 8
David Holland,     \fIdavidh@use.com\fP
Ported to \fBSolaris x86\fP.
.TP 8
David McCullough,  \fIdavidm@stallion.oz.au\fP
Ported to \fBSCO SVR3\fP.
.TP 8
Michael Rohleder,  \fImichael.rohleder@stadt-frankfurt.de\fP
Ported to \fBISC SVR3\fP.
.TP 8
Kees Verstoep,     \fIversto@cs.vu.nl\fP
Ported to \fBAmoeba\fP based on Leendert van Doorn's original Amoeba port of
X11R5.
.TP 8
Marc Evans,        \fIMarc@XFree86.org\fP
Ported to \fBOSF/1\fP.
.TP 8
Philip Homburg,    \fIphilip@cs.vu.nl\fP
Ported to \fBMinix-386vm\fP.
.TP 8
Thomas Mueller,    \fItm@systrix.de\fP
Ported to \fBLynxOS\fP.
.TP 8
Jon Tombs,         \fItombs@XFree86.org\fP
S3 server and accelerated server coordination.
.TP 8
Harald Koenig,     \fIkoenig@tat.physik.uni-tuebingen.de\fP
S3 server development.
.TP 8
Bernhard Bender,   \fIbr@elsa.mhs.compuserve.com\fP
S3 server development.
.TP 8
Kevin Martin,      \fImartin@cs.unc.edu\fP
Overall work on the base accelerated servers (ATI and 8514/A), and Mach64
server.
.TP 8
Rik Faith,         \fIfaith@cs.unc.edu\fP
Overall work on the base accelerated servers (ATI and 8514/A).
.TP 8
Tiago Gons,        \fItiago@comosjn.hobby.nl\fP
Mach8 and 8514/A server development
.TP 8
Hans Nasten,       \fInasten@everyware.se\fP
Mach8, 8514/A, and S3 server development and BSD/386 support
.TP 8
Mike Bernson,      \fImike@mbsun.mlb.org\fP
Mach32 server development.
.TP 8
Mark Weaver,       \fIMark_Weaver@brown.edu\fP
Mach32 server development.
.TP 8
Craig Groeschel,   \fIcraig@metrolink.com\fP
Mach32 server development.
.TP 8
Henry Worth,       \fIHenry.Worth@amail.amdahl.com\fP
AGX server.
.TP 8
Erik Nygren,       \fInygren@mit.edu\fP
P9000 server.
.TP 8
Harry Langenbacher \fIharry@brain.jpl.nasa.gov\fP
P9000 server.
.TP 8
Chris Mason,       \fImason@mail.csh.rit.edu\fP
P9000 server.
.TP 8
Henrik Harmsen     \fIharmsen@eritel.se\fP
P9000 server.
.TP 8
Simon Cooper,      \fIscooper@vizlab.rutgers.edu\fP
Cirrus accelerated code (based on work by Bill Reynolds).
.TP 8
Harm Hanemaayer,   \fIhhanemaa@cs.ruu.nl\fP
Cirrus accelerated code, and ARK driver.
.TP 8
Thomas Zerucha,    \fIzerucha@shell.portal.com\fP
Support for Cirrus CL-GD7543.
.TP 8
Leon Bottou,       \fIbottou@laforia.ibp.fr\fP
ARK driver.
.TP 8
Mike Tierney,      \fIfloyd@eng.umd.edu\fP
WD accelerated code.
.TP 8
Bill Conn,         \fIconn@bnr.ca\fP
WD accelerated code.
.TP 8
Brad Bosch,        \fIbrad@lachman.com\fP
WD 90C24A support.
.TP 8
Alan Hourihane,    \fIalanh@fairlite.demon.co.uk\fP
Trident SVGA driver, SiS SVGA driver and DEC 21030 server.
.TP 8
Marc Aurele La France, \fItsi@ualberta.ca\fP
ATI SVGA driver
.TP 8
Steve Goldman,     \fIsgoldman@encore.com\fP
Oak 067/077 SVGA driver.
.TP 8
Jorge Delgado,     \fIernar@dit.upm.es\fP
Oak SVGA driver, and 087 accelerated code.
.TP 8
Bill Conn,         \fIconn@bnr.ca\fP
WD accelerated code.
.TP 8
Paolo Severini,    \fIlendl@dist.dist.unige.it\fP
AL2101 SVGA driver
.TP 8
Ching-Tai Chiu,    \fIcchiu@netcom.com\fP
Avance Logic ALI SVGA driver
.TP 8
Manfred Brands,    \fImb@oceonics.nl\fP
Cirrus 64xx SVGA driver
.TP 8
Randy Hendry,      \fIrandy@sgi.com\fP
Cirrus 6440 support in the cl64xx SVGA driver
.TP 8
Frank Dikker,      \fIdikker@cs.utwente.nl\fP
MX SVGA driver
.TP 8
Regis Cridlig,     \fIcridlig@dmi.ens.fr\fP
Chips & Technologies driver
.TP 8
Jon Block,         \fIblock@frc.com\fP
Chips & Technologies driver
.TP 8
Mike Hollick,      \fIhollick@graphics.cis.upenn.edu\fP
Chips & Technologies driver
.TP 8
Nozomi Ytow
Chips & Technologies driver
.TP 8
Egbert Eich,       \fIEgbert.Eich@Physik.TH-Darmstadt.DE\fP
Chips & Technologies driver
.TP 8
David Bateman,     \fIdbateman@ee.uts.edu.au\fP
Chips & Technologies driver
.TP 8
Xavier Ducoin,     \fIxavier@rd.lectra.fr\fP
Chips & Technologies driver
.TP 8
Peter Trattler,    \fIpeter@sbox.tu-graz.ac.at\fP
RealTek SVGA driver
.TP 8
Craig Struble,     \fIcstruble@acm.vt.edu\fP
Video7 SVGA driver
.TP 8
Gertjan Akkerman,  \fIakkerman@dutiba.twi.tudelft.nl\fP
16 colour VGA server, and XF86Config parser.
.TP 8
Davor Matic,       \fIdmatic@Athena.MIT.EDU\fP
Hercules driver.
.TP 8
Pascal Haible,     \fIhaible@izfm.uni-stuttgart.de\fP
Banked monochrome VGA support, Hercules support, and mono frame buffer
support for dumb monochrome devices
.TP 8
Martin Schaller,
.TP 8
Geert Uytterhoeven,\fIGeert.Uytterhoeven@cs.kuleuven.ac.be\fP
Linux/m68k Frame Buffer Device driver
.TP 8
Andreas Schwab,    \fIschwab@issan.informatik.uni-dortmund.de\fP
Linux/m68k Frame Buffer Device driver
.TP 8
Guenther Kelleter, \fIguenther@Pool.Informatik.RWTH-Aachen.de\fP
Linux/m68k Frame Buffer Device driver
.TP 8
Frederic Lepied,   \fILepied@XFree86.Org\fP
XInput extension integration. Wacom, joystick and extended mouse drivers.
.TP 8
Patrick Lecoanet,   \fIlecoanet@cena.dgac.fr\fP
Elographics touchscreen driver.
.TP 8
Steven Lang,        \fItiger@tyger.org\fP
SummaSketch tablet driver.
.PP
 ... and many more people out there on the net who helped with beta-testing
this enhancement.
.PP
\fIXFree86\fP source is available from the FTP server
\fIftp.XFree86.org\fP, among others.  Send email to
\fIXFree86@XFree86.org\fP for details.
.\" $XConsortium: XFree86.man /main/25 1996/12/09 17:33:22 kaleb $
