.\" $TOG: Xserver.man /main/68 1998/02/09 14:12:35 kaleb $
.\" Copyright 1984 - 1991, 1993, 1994, 1998  The Open Group
.\" 
.\" All Rights Reserved.
.\" 
.\" The above copyright notice and this permission notice shall be included
.\" in all copies or substantial portions of the Software.
.\" 
.\" THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
.\" OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
.\" MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
.\" IN NO EVENT SHALL THE OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR
.\" OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
.\" ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
.\" OTHER DEALINGS IN THE SOFTWARE.
.\" 
.\" Except as contained in this notice, the name of The Open Group shall
.\" not be used in advertising or otherwise to promote the sale, use or
.\" other dealings in this Software without prior written authorization
.\" from The Open Group.
.\" $XFree86: xc/programs/Xserver/Xserver.cpp,v 1.1 2000/12/11 20:29:32 dawes Exp $
.TH XSERVER 1 "Release 6.4"  "X Version 11"
.SH NAME
Xserver \- X Window System display server
.SH SYNOPSIS
.B X
[option ...]
.SH DESCRIPTION
.I X
is the generic name for the X Window System display server.  It is
frequently a link or a copy of the appropriate server binary for
driving the most frequently used server on a given machine.
.SH "STARTING THE SERVER"
The X server is usually started from the X Display Manager program \fIxdm(1)\fP.
This utility is run from the system boot files and takes care of keeping
the server running, prompting for usernames and passwords, and starting up
the user sessions.
.PP
Installations that run more than one window system may need to use the
\fIxinit(1)\fP utility instead of \fIxdm\fP.  However, \fIxinit\fP is
to be considered a tool for building startup scripts and is not
intended for use by end users.  Site administrators are \fBstrongly\fP
urged to use \fIxdm\fP, or build other interfaces for novice users.
.PP
The X server may also be started directly by the user, though this
method is usually reserved for testing and is not recommended for
normal operation.  On some platforms, the user must have special
permission to start the X server, often because access to certain
devices (e.g. /dev/mouse) is restricted.
.PP
When the X server starts up, it typically takes over the display.  If
you are running on a workstation whose console is the display, you may
not be able to log into the console while the server is running.
.SH OPTIONS
All of the X servers accept the following command line options:
.TP 8
.B :\fIdisplaynumber\fP
the X server runs as the given \fIdisplaynumber\fP, which by default is 0.
If multiple X servers are to run simultaneously on a host, each must have
a unique display number.  See the DISPLAY
NAMES section of the \fIX(1)\fP manual page to learn how to specify
which display number clients should try to use.
.TP 8
.B \-a \fInumber\fP
sets pointer acceleration (i.e. the ratio of how much is reported to how much
the user actually moved the pointer).
.TP 8
.B \-ac
disables host-based access control mechanisms.  Enables access by any host,
and permits any host to modify the access control list.
Use with extreme caution.
This option exists primarily for running test suites remotely.
.TP 8
.B \-audit \fIlevel\fP
Sets the audit trail level.  The default level is 1, meaning only connection
rejections are reported.  Level 2 additionally reports all successful
connections and disconnects.  Level 4 enables messages from the
SECURITY extension, if present, including generation and revocation of
authorizations and violations of the security policy.
Level 0 turns off the audit trail.
Audit lines are sent as standard error output.
.TP 8
.B \-auth \fIauthorization-file\fP
Specifies a file which contains a collection of authorization records used
to authenticate access.  See also the \fIxdm\fP and \fIXsecurity\fP manual
pages.
.TP 8
.B bc
disables certain kinds of error checking, for bug compatibility with
previous releases (e.g., to work around bugs in R2 and R3 xterms and toolkits).
Deprecated.
.TP 8
.B \-bs
disables backing store support on all screens.
.TP 8
.B \-c
turns off key-click.
.TP 8
.B c \fIvolume\fP
sets key-click volume (allowable range: 0-100).
.TP 8
.B \-cc \fIclass\fP
sets the visual class for the root window of color screens.
The class numbers are as specified in the X protocol.
Not obeyed by all servers.
.TP 8
.B \-co \fIfilename\fP
sets name of RGB color database.  The default is <XRoot>/lib/X11/rgb,
where <XRoot> refers to the root of the X11 install tree.
.ig
.TP 8
.B \-config \fIfilename\fP
reads more options from the given file.  Options in the file may be separated
by newlines if desired.  If a '#' character appears on a line, all characters
between it and the next newline are ignored, providing a simple commenting
facility.  The \fB\-config\fP option itself may appear in the file.
.BR NOTE :
This option is disabled when the Xserver is run with an effective uid
different from the user's real uid.
..
.TP 8
.B \-core
causes the server to generate a core dump on fatal errors.
.TP 8
.B \-dpi \fIresolution\fP
sets the resolution of the screen, in dots per inch.
To be used when the server cannot determine the screen size from the hardware.
.TP 8
.B \-deferglyphs \fIwhichfonts\fP
specifies the types of fonts for which the server should attempt to use
deferred glyph loading.  \fIwhichfonts\fP can be all (all fonts),
none (no fonts), or 16 (16 bit fonts only).
.TP 8
.B \-f \fIvolume\fP
sets feep (bell) volume (allowable range: 0-100).
.TP 8
.B \-fc \fIcursorFont\fP
sets default cursor font.
.TP 8
.B \-fn \fIfont\fP
sets the default font.
.TP 8
.B \-fp \fIfontPath\fP
sets the search path for fonts.  This path is a comma separated list
of directories which the X server searches for font databases.
.TP 8
.B \-help
prints a usage message.
.TP 8
.B \-I
causes all remaining command line arguments to be ignored.
.TP 8
.B \-kb
disables the XKEYBOARD extension if present.
.TP 8
.B \-nolisten \fItrans-type\fP
Disable a transport type.  For example, TCP/IP connections can be disabled
with
.B \-nolisten tcp
.TP 8
.B \-noreset
prevents a server reset when the last client connection is closed.  This
overrides a previous
.B \-terminate
command line option.
.TP 8
.B \-p \fIminutes\fP
sets screen-saver pattern cycle time in minutes.
.TP 8
.B \-pn
permits the server to continue running if it fails to establish all of
its well-known sockets (connection points for clients), but
establishes at least one.
.TP 8
.B \-r
turns off auto-repeat.
.TP 8
.B r
turns on auto-repeat.
.TP 8
.B \-s \fIminutes\fP
sets screen-saver timeout time in minutes.
.TP 8
.B \-su
disables save under support on all screens.
.TP 8
.B \-t \fInumber\fP
sets pointer acceleration threshold in pixels (i.e. after how many pixels
pointer acceleration should take effect).
.TP 8
.B \-terminate
causes the server to terminate at server reset, instead of continuing to run.
This overrides a previous
.B \-noreset
command line option.
.TP 8
.B \-to \fIseconds\fP
sets default connection timeout in seconds.
.TP 8
.B \-tst
disables all testing extensions (e.g., XTEST, XTrap, XTestExtension1, RECORD).
.TP 8
.B tty\fIxx\fP
ignored, for servers started the ancient way (from init).
.TP 8
.B v
sets video-off screen-saver preference.
.TP 8
.B \-v
sets video-on screen-saver preference.
.TP 8
.B \-wm
forces the default backing-store of all windows to be WhenMapped.  This
is a backdoor way of getting backing-store to apply to all windows.
Although all mapped windows will have backing store, the backing store
attribute value reported by the server for a window will be the last
value established by a client.  If it has never been set by a client,
the server will report the default value, NotUseful.  This behavior is
required by the X protocol, which allows the server to exceed the
client's backing store expectations but does not provide a way to tell
the client that it is doing so.
.TP 8
.B \-x \fIextension\fP
loads the specified extension at init.
This is a no-op for most implementations.
.TP 8
.B [+-]xinerama
enable(+) or disable(-) XINERAMA extension. Default is disabled.
.SH SERVER DEPENDENT OPTIONS
Some X servers accept the following options:
.TP 8
.B \-ld \fIkilobytes\fP
sets the data space limit of the server to the specified number of kilobytes.
A value of zero makes the data size as large as possible.  The default value
of \-1 leaves the data space limit unchanged.
.TP 8
.B \-lf \fIfiles\fP
sets the number-of-open-files limit of the server to the specified number.
A value of zero makes the limit as large as possible.  The default value
of \-1 leaves the limit unchanged.
.TP 8
.B \-ls \fIkilobytes\fP
sets the stack space limit of the server to the specified number of kilobytes.
A value of zero makes the stack size as large as possible.  The default value
of \-1 leaves the stack space limit unchanged.
.TP 8
.B \-logo
turns on the X Window System logo display in the screen-saver.
There is currently no way to change this from a client.
.TP 8
.B nologo
turns off the X Window System logo display in the screen-saver.
There is currently no way to change this from a client.
.SH XDMCP OPTIONS
X servers that support XDMCP have the following options.
See the \fIX Display Manager Control Protocol\fP specification for more
information.
.TP 8
.B \-query \fIhost-name\fP
Enable XDMCP and send Query packets to the specified host.
.TP 8
.B \-broadcast
Enable XDMCP and broadcast BroadcastQuery packets to the network.  The
first responding display manager will be chosen for the session.
.TP 8
.B \-indirect \fIhost-name\fP
Enable XDMCP and send IndirectQuery packets to the specified host.
.TP 8
.B \-port \fIport-num\fP
Use an alternate port number for XDMCP packets.  Must be specified before
any \-query, \-broadcast or \-indirect options.
.TP 8
.B \-once
Causes the server to terminate (rather than reset) when the XDMCP session ends.
.TP 8
.B \-class \fIdisplay-class\fP
XDMCP has an additional display qualifier used in resource lookup for
display-specific options.  This option sets that value, by default it 
is "MIT-Unspecified" (not a very useful value).
.TP 8
.B \-cookie \fIxdm-auth-bits\fP
When testing XDM-AUTHENTICATION-1, a private key is shared between the
server and the manager.  This option sets the value of that private
data (not that it is very private, being on the command line!).
.TP 8
.B \-displayID \fIdisplay-id\fP
Yet another XDMCP specific value, this one allows the display manager to
identify each display so that it can locate the shared key.
.SH XKEYBOARD OPTIONS
X servers that support the XKEYBOARD extension accept the following options:
.TP 8
.B \-xkbdir \fIdirectory\fP
base directory for keyboard layout files
.TP 8
.B \-xkbmap \fIfilename\fP
keyboard description to load on startup
.TP 8
.B [+-]accessx
enable(+) or disable(-) AccessX key sequences
.TP 8
.B \-ar1 \fImilliseconds\fP
sets the length of time in milliseconds that a key must be depressed before
autorepeat starts
.TP 8
.B \-ar2 \fImilliseconds\fP
sets the length of time in milliseconds that should elapse between
autorepeat-generated keystrokes
.PP
Many servers also have device-specific command line options.  See the
manual pages for the individual servers for more details.
.SH SECURITY EXTENSION OPTIONS
X servers that support the SECURITY extension accept the following option:
.TP 8
.B \-sp \fIfilename\fP
causes
the server to attempt to read and interpret filename as a security policy
file with the format described below.  The file is read at
server startup and reread at each server reset.
.PP
The syntax of the security policy file is as follows.
Notation: "*" means zero or more occurrences of the preceding element,
and "+" means one or more occurrences.  To interpret <foo/bar>, ignore
the text after the /; it is used to distinguish between instances of
<foo> in the next section.
.PP
.nf
<policy file> ::= <version line> <other line>*

<version line> ::= <string/v> '\en'

<other line > ::= <comment> | <access rule> | <site policy> | <blank line>

<comment> ::= # <not newline>* '\en'

<blank line> ::= <space> '\en'

<site policy> ::= sitepolicy <string/sp> '\en'

<access rule> ::= property <property/ar> <window> <perms> '\en'

<property> ::= <string>

<window> ::= any | root | <required property>

<required property> ::= <property/rp> | <property with value>

<property with value> ::= <property/rpv> = <string/rv>

<perms> ::= [ <operation> | <action> | <space> ]*

<operation> ::= r | w | d

<action> ::= a | i | e

<string> ::= <dbl quoted string> | <single quoted string> | <unqouted string>

<dbl quoted string> ::= <space> " <not dqoute>* " <space>

<single quoted string> ::= <space> ' <not squote>* ' <space>

<unquoted string> ::= <space> <not space>+ <space>

<space> ::= [ ' ' | '\et' ]*

Character sets:

<not newline> ::= any character except '\en'
<not dqoute>  ::= any character except "
<not squote>  ::= any character except '
<not space>   ::= any character except those in <space>
.fi
.PP
The semantics associated with the above syntax are as follows.
.PP
<version line>, the first line in the file, specifies the file format
version.  If the server does not recognize the version <string/v>, it
ignores the rest of the file.  The version string for the file format
described here is "version-1" .
.PP
Once past the <version line>, lines that do not match the above syntax
are ignored.
.PP
<comment> lines are ignored.
.PP
<sitepolicy> lines are currently ignored.  They are intended to
specify the site policies used by the XC-QUERY-SECURITY-1
authorization method.
.PP
<access rule> lines specify how the server should react to untrusted
client requests that affect the X Window property named <property/ar>.
The rest of this section describes the interpretation of an
<access rule>.
.PP
For an <access rule> to apply to a given instance of <property/ar>,
<property/ar> must be on a window that is in the set of windows
specified by <window>.  If <window> is any, the rule applies to
<property/ar> on any window.  If <window> is root, the rule applies to
<property/ar> only on root windows.
.PP
If <window> is <required property>, the following apply.  If <required
property> is a <property/rp>, the rule applies when the window also
has that <property/rp>, regardless of its value.  If <required
property> is a <property with value>, <property/rpv> must also have
the value specified by <string/rv>.  In this case, the property must
have type STRING and format 8, and should contain one or more
null-terminated strings.  If any of the strings match <string/rv>, the
rule applies.
.PP
The definition of string matching is simple case-sensitive string
comparison with one elaboration: the occurence of the character '*' in
<string/rv> is a wildcard meaning "any string."  A <string/rv> can
contain multiple wildcards anywhere in the string.  For example, "x*"
matches strings that begin with x, "*x" matches strings that end with
x, "*x*" matches strings containing x, and "x*y*" matches strings that
start with x and subsequently contain y.
.PP
There may be multiple <access rule> lines for a given <property/ar>.
The rules are tested in the order that they appear in the file.  The
first rule that applies is used.
.PP
<perms> specify operations that untrusted clients may attempt, and
the actions that the server should take in response to those operations.
.PP
<operation> can be r (read), w (write), or d (delete).  The following
table shows how X Protocol property requests map to these operations
in The Open Group server implementation.
.PP
.nf
GetProperty	r, or r and d if delete = True
ChangeProperty	w
RotateProperties	r and w
DeleteProperty	d
ListProperties	none, untrusted clients can always list all properties
.fi
.PP
<action> can be a (allow), i (ignore), or e (error).  Allow means
execute the request as if it had been issued by a trusted client.
Ignore means treat the request as a no-op.  In the case of
GetProperty, ignore means return an empty property value if the
property exists, regardless of its actual value.  Error means do not
execute the request and return a BadAtom error with the atom set to
the property name.  Error is the default action for all properties,
including those not listed in the security policy file.
.PP
An <action> applies to all <operation>s that follow it, until the next
<action> is encountered.  Thus, irwad  means ignore read and write,
allow delete.
.PP
GetProperty and RotateProperties may do multiple operations (r and d,
or r and w).  If different actions apply to the operations, the most
severe action is applied to the whole request; there is no partial
request execution.  The severity ordering is: allow < ignore < error.
Thus, if the <perms> for a property are ired (ignore read, error
delete), and an untrusted client attempts GetProperty on that property
with delete = True, an error is returned, but the property value is
not.  Similarly, if any of the properties in a RotateProperties do not
allow both read and write, an error is returned without changing any
property values.
.PP
Here is an example security policy file.
.PP
.ta 3i 4i
.nf
version-1 

# Allow reading of application resources, but not writing.
property RESOURCE_MANAGER	root	ar iw
property SCREEN_RESOURCES	root	ar iw

# Ignore attempts to use cut buffers.  Giving errors causes apps to crash,
# and allowing access may give away too much information.
property CUT_BUFFER0	root	irw
property CUT_BUFFER1	root	irw
property CUT_BUFFER2	root	irw
property CUT_BUFFER3	root	irw
property CUT_BUFFER4	root	irw
property CUT_BUFFER5	root	irw
property CUT_BUFFER6	root	irw
property CUT_BUFFER7	root	irw

# If you are using Motif, you probably want these.
property _MOTIF_DEFAULT_BINDINGS	root	ar iw
property _MOTIF_DRAG_WINDOW	root	ar iw
property _MOTIF_DRAG_TARGETS	any 	ar iw
property _MOTIF_DRAG_ATOMS	any 	ar iw
property _MOTIF_DRAG_ATOM_PAIRS	any 	ar iw

# The next two rules let xwininfo -tree work when untrusted.
property WM_NAME	any	ar

# Allow read of WM_CLASS, but only for windows with WM_NAME.
# This might be more restrictive than necessary, but demonstrates
# the <required property> facility, and is also an attempt to
# say "top level windows only."
property WM_CLASS	WM_NAME	ar

# These next three let xlsclients work untrusted.  Think carefully
# before including these; giving away the client machine name and command
# may be exposing too much.
property WM_STATE	WM_NAME	ar
property WM_CLIENT_MACHINE	WM_NAME	ar
property WM_COMMAND	WM_NAME	ar

# To let untrusted clients use the standard colormaps created by
# xstdcmap, include these lines.
property RGB_DEFAULT_MAP	root	ar
property RGB_BEST_MAP	root	ar
property RGB_RED_MAP	root	ar
property RGB_GREEN_MAP	root	ar
property RGB_BLUE_MAP	root	ar
property RGB_GRAY_MAP	root	ar

# To let untrusted clients use the color management database created
# by xcmsdb, include these lines.
property XDCCC_LINEAR_RGB_CORRECTION	root	ar
property XDCCC_LINEAR_RGB_MATRICES	root	ar
property XDCCC_GRAY_SCREENWHITEPOINT	root	ar
property XDCCC_GRAY_CORRECTION	root	ar

# To let untrusted clients use the overlay visuals that many vendors
# support, include this line.
property SERVER_OVERLAY_VISUALS	root	ar

# Dumb examples to show other capabilities.

# oddball property names and explicit specification of error conditions
property "property with spaces"	'property with "'	aw er ed

# Allow deletion of Woo-Hoo if window also has property OhBoy with value
# ending in "son".  Reads and writes will cause an error.
property Woo-Hoo	OhBoy = "*son"	ad

.fi
.SH "NETWORK CONNECTIONS"
The X server supports client connections via a platform-dependent subset of
the following transport types: TCP\/IP, Unix Domain sockets, DECnet,
and several varieties of SVR4 local connections.  See the DISPLAY
NAMES section of the \fIX(__miscmansuffix__)\fP manual page to learn how to specify
which transport type clients should try to use.
.SH GRANTING ACCESS
The X server implements a platform-dependent subset of the following
authorization protocols: MIT-MAGIC-COOKIE-1, XDM-AUTHORIZATION-1,
SUN-DES-1, and MIT-KERBEROS-5.  See the \fIXsecurity(1)\fP manual page
for information on the operation of these protocols.
.PP
Authorization data required by the above protocols is passed to the
server in a private file named with the \fB\-auth\fP command line
option.  Each time the server is about to accept the first connection
after a reset (or when the server is starting), it reads this file.
If this file contains any authorization records, the local host is not
automatically allowed access to the server, and only clients which
send one of the authorization records contained in the file in the
connection setup information will be allowed access.  See the
\fIXau\fP manual page for a description of the binary format of this
file.  See \fIxauth(1)\fP for maintenance of this file, and distribution
of its contents to remote hosts.
.PP
The X server also uses a host-based access control list for deciding
whether or not to accept connections from clients on a particular machine.
If no other authorization mechanism is being used,
this list initially consists of the host on which the server is running as
well as any machines listed in the file \fI/etc/X\fBn\fI.hosts\fR, where
\fBn\fP is the display number of the server.  Each line of the file should
contain either an Internet hostname (e.g. expo.lcs.mit.edu) or a DECnet
hostname in double colon format (e.g. hydra::).  There should be no leading
or trailing spaces on any lines.  For example:
.sp
.in +8
.nf 
joesworkstation
corporate.company.com
star::
bigcpu::
.fi
.in -8
.PP
Users can add or remove hosts from this list and enable or disable access
control using the \fIxhost\fP command from the same machine as the server.
.PP
If the X FireWall Proxy (\fIxfwp\fP) is being used without a sitepolicy,
host-based authorization must be turned on for clients to be able to 
connect to the X server via the \fIxfwp\fP.  If \fIxfwp\fP is run without 
a configuration file and thus no sitepolicy is defined, if \fIxfwp\fP 
is using an X server where xhost + has been run to turn off host-based 
authorization checks, when a client tries to connect to this X server 
via \fIxfwp\fP, the X server will deny the connection.  See \fIxfwp(1)\fP
for more information about this proxy.
.PP
The X protocol intrinsically does not have any notion of window operation
permissions or place any restrictions on what a client can do; if a program can
connect to a display, it has full run of the screen.  
X servers that support the SECURITY extension fare better because clients
can be designated untrusted via the authorization they use to connect; see
the \fIxauth(1)\fP manual page for details.  Restrictions are imposed
on untrusted clients that curtail the mischief they can do.  See the SECURITY
extension specification for a complete list of these restrictions.
.PP
Sites that have better
authentication and authorization systems might wish to make
use of the hooks in the libraries and the server to provide additional
security models.
.SH SIGNALS
The X server attaches special meaning to the following signals:
.TP 8
.I SIGHUP
This signal causes the server to close all existing connections, free all
resources, and restore all defaults.  It is sent by the display manager
whenever the main user's main application (usually an \fIxterm\fP or window
manager) exits to force the server to clean up and prepare for the next
user.
.TP 8
.I SIGTERM
This signal causes the server to exit cleanly.
.TP 8
.I SIGUSR1
This signal is used quite differently from either of the above.  When the
server starts, it checks to see if it has inherited SIGUSR1 as SIG_IGN
instead of the usual SIG_DFL.  In this case, the server sends a SIGUSR1 to
its parent process after it has set up the various connection schemes.
\fIXdm\fP uses this feature to recognize when connecting to the server
is possible.
.SH FONTS
The X server
can obtain fonts from directories and/or from font servers.
The list of directories and font servers
the X server uses when trying to open a font is controlled
by the \fIfont path\fP.  
.LP
The default font path is
"<XRoot>/lib/X11/fonts/misc/,
<XRoot>/lib/X11/fonts/Speedo/,
<XRoot>/lib/X11/fonts/Type1/,
<XRoot>/lib/X11/fonts/75dpi/,
<XRoot>/lib/X11/fonts/100dpi/" .
where <XRoot> refers to the root of the X11 install tree.
.LP
The font path can be set with the \fB\-fp\fP option or by \fIxset(1)\fP
after the server has started.
.SH FILES
.TP 30
/etc/X\fBn\fP.hosts
Initial access control list for display number \fBn\fP
.TP 30
<XRoot>/lib/X11/fonts/misc, <XRoot>/lib/X11/fonts/75dpi, <XRoot>/lib/X11/fonts/100dpi 
Bitmap font directories
.TP 30
<XRoot>/lib/X11/fonts/Speedo, <XRoot>/lib/X11/fonts/Type1
Outline font directories
.TP 30
<XRoot>/lib/X11/fonts/PEX
PEX font directories
.TP 30
<XRoot>/lib/X11/rgb.txt
Color database
.TP 30
/tmp/.X11-unix/X\fBn\fP
Unix domain socket for display number \fBn\fP
.TP 30
/tmp/rcX\fBn\fP
Kerberos 5 replay cache for display number \fBn\fP
.TP 30
/usr/adm/X\fBn\fPmsgs
Error log file for display number \fBn\fP if run from \fIinit(8)\fP
.TP 30
<XRoot>/lib/X11/xdm/xdm-errors
Default error log file if the server is run from \fIxdm(1)\fP
.LP
Note: <XRoot> refers to the root of the X11 install tree.
.SH "SEE ALSO"
General information: X(__miscmansuffix__)
.PP
Protocols:
.I "X Window System Protocol,"
.I "The X Font Service Protocol,"
.I "X Display Manager Control Protocol"
.PP
Fonts: bdftopcf(1), mkfontdir(1), xfs(1), xlsfonts(1), xfontsel(1), xfd(1),
.I "X Logical Font Description Conventions"
.PP
Security: Xsecurity(__miscmansuffix__), xauth(1), Xau(1), xdm(1), xhost(1), xfwp(1)
.I "Security Extension Specification"
.PP
Starting the server: xdm(1), xinit(1)
.PP
Controlling the server once started: xset(1), xsetroot(1), xhost(1)
.PP
Server-specific man pages: 
Xdec(1), XmacII(1), Xsun(1), Xnest(1), Xvfb(1),
XFree86(1), Xdarwin(1).
.PP
Server internal documentation:
.I "Definition of the Porting Layer for the X v11 Sample Server"
.SH AUTHORS
The sample server was originally written by Susan Angebranndt, Raymond
Drewry, Philip Karlton, and Todd Newman, from Digital Equipment
Corporation, with support from a large cast.  It has since been
extensively rewritten by Keith Packard and Bob Scheifler, from MIT.
Dave Wiggins took over post-R5 and made substantial improvements.
