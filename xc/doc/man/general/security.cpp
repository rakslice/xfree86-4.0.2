.\" $TOG: security.cpp /main/13 1997/10/13 14:20:52 kaleb $
.\" Copyright (c) 1993, 1994  X Consortium
.\" 
.\" Permission is hereby granted, free of charge, to any person obtaining
.\" a copy of this software and associated documentation files (the
.\" "Software"), to deal in the Software without restriction, including
.\" without limitation the rights to use, copy, modify, merge, publish,
.\" distribute, sublicense, and/or sell copies of the Software, and to
.\" permit persons to whom the Software is furnished to do so, subject to
.\" the following conditions:
.\" 
.\" The above copyright notice and this permission notice shall be
.\" included in all copies or substantial portions of the Software.
.\" 
.\" THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
.\" EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
.\" MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
.\" IN NO EVENT SHALL THE X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR
.\" OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
.\" ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
.\" OTHER DEALINGS IN THE SOFTWARE.
.\" 
.\" Except as contained in this notice, the name of the X Consortium shall
.\" not be used in advertising or otherwise to promote the sale, use or
.\" other dealings in this Software without prior written authorization
.\" from the X Consortium.
.\"
.nr )S 12
.TH XSECURITY __miscmansuffix__ "Release 6.3" "X Version 11"
.SH NAME
Xsecurity \- X display access control
.SH SYNOPSIS
.PP
X provides mechanism for implementing many access control systems.
The sample implementation includes five mechanisms:
.nf
.br
.ta 3.4i
    Host Access	Simple host-based access control.
    MIT-MAGIC-COOKIE-1	Shared plain-text "cookies".
    XDM-AUTHORIZATION-1	Secure DES based private-keys.
    SUN-DES-1	Based on Sun's secure rpc system.
    MIT-KERBEROS-5	Kerberos Version 5 user-to-user.
.fi
.SH "ACCESS SYSTEM DESCRIPTIONS"
.IP "Host Access"
Any client on a host in the host access control list is allowed access to
the X server.  This system can work reasonably well in an environment
where everyone trusts everyone, or when only a single person can log in
to a given machine, and is easy to use when the list of hosts used is small.
This system does not work well when multiple people can log in to a single
machine and mutual trust does not exist.
The list of allowed hosts is stored in the X server and can be changed with
the \fIxhost\fP command.
When using the more secure mechanisms listed below, the host list is
normally configured to be the empty list, so that only authorized
programs can connect to the display.
.IP "MIT-MAGIC-COOKIE-1"
When using MIT-MAGIC-COOKIE-1,
the client sends a 128 bit "cookie"
along with the connection setup information.
If the cookie presented by the client matches one
that the X server has, the connection is allowed access.
The cookie is chosen so that it is hard to guess;
\fIxdm\fP generates such cookies automatically when this form of
access control is used.
The user's copy of
the cookie is usually stored in the \fI.Xauthority\fP file in the home
directory, although the environment variable \fBXAUTHORITY\fP can be used
to specify an alternate location.
\fIXdm\fP automatically passes a cookie to the server for each new
login session, and stores the cookie in the user file at login.
.IP
The cookie is transmitted on the network without encryption, so
there is nothing to prevent a network snooper from obtaining the data
and using it to gain access to the X server.  This system is useful in an
environment where many users are running applications on the same machine
and want to avoid interference from each other, with the caveat that this
control is only as good as the access control to the physical network.
In environments where network-level snooping is difficult, this system
can work reasonably well.
.IP "XDM-AUTHORIZATION-1"
Sites in the United States can use a DES-based access control
mechanism called XDM-AUTHORIZATION-1.
It is similar in usage to MIT-MAGIC-COOKIE-1 in that a key is
stored in the \fI.Xauthority\fP file and is shared with the X server.
However,
this key consists of two parts - a 56 bit DES encryption key and 64 bits of
random data used as the authenticator.
.IP
When connecting to the X server, the application generates 192 bits of data
by combining the current time in seconds (since 00:00 1/1/1970 GMT) along
with 48 bits of "identifier".  For TCP/IP connections, the identifier is
the address plus port number; for local connections it is the process ID
and 32 bits to form a unique id (in case multiple connections to the same
server are made from a single process).  This 192 bit packet is then
encrypted using the DES key and sent to the X server, which is able to
verify if the requestor is authorized to connect by decrypting with the
same DES key and validating the authenticator and additional data.
This system is useful in many environments where host-based access control
is inappropriate and where network security cannot be ensured.
.IP "SUN-DES-1"
Recent versions of SunOS (and some other systems) have included a
secure public key remote procedure call system.  This system is based
on the notion of a network principal; a user name and NIS domain pair.
Using this system, the X server can securely discover the actual user
name of the requesting process.  It involves encrypting data with the
X server's public key, and so the identity of the user who started the
X server is needed for this; this identity is stored in the \fI.Xauthority\fP
file.  By extending the semantics of "host address" to include this notion of
network principal, this form of access control is very easy to use.
.IP
To allow access by a new user, use \fIxhost\fP.  For example,
.nf
    xhost keith@ ruth@mit.edu
.fi
adds "keith" from the NIS domain of the local machine, and "ruth" in
the "mit.edu" NIS domain.  For keith or ruth to successfully connect
to the display, they must add the principal who started the server to
their \fI.Xauthority\fP file.  For example:
.nf
    xauth add expo.lcs.mit.edu:0 SUN-DES-1 unix.expo.lcs.mit.edu@our.domain.edu
.fi
This system only works on machines which support Secure RPC, and only for
users which have set up the appropriate public/private key pairs on their
system.  See the Secure RPC documentation for details.
To access the display from a remote host, you may have to do a
\fIkeylogin\fP on the remote host first.
.IP MIT-KERBEROS-5
Kerberos is a network-based authentication scheme developed by MIT for
Project Athena.  It allows mutually suspicious principals to
authenticate each other as long as each trusts a third party,
Kerberos.  Each principal has a secret key known only to it and
Kerberos.  Principals includes servers, such as an FTP server or X
server, and human users, whose key is their password.  Users gain
access to services by getting Kerberos tickets for those services from
a Kerberos server.  Since the X server has no place to store a secret
key, it shares keys with the user who logs in.  X authentication thus
uses the user-to-user scheme of Kerberos version 5.
.IP
When you log in via \fIxdm\fP, \fIxdm\fP will use your password to
obtain the initial Kerberos tickets.  \fIxdm\fP stores the tickets in
a credentials cache file and sets the environment variable
\fIKRB5CCNAME\fP to point to the file.  The credentials cache is
destroyed when the session ends to reduce the chance of the tickets
being stolen before they expire.
.IP
Since Kerberos is a user-based authorization protocol, like the
SUN-DES-1 protocol, the owner of a display can enable
and disable specific users, or Kerberos principals.
The \fIxhost\fP client is used to enable or disable authorization.
For example,
.nf
    xhost krb5:judy krb5:gildea@x.org
.fi
adds "judy" from the Kerberos realm of the local machine, and "gildea"
from the "x.org" realm.
.SH "THE AUTHORIZATION FILE"
.PP
Except for Host Access control, each of these systems uses data stored in
the \fI.Xauthority\fP file to generate the correct authorization information
to pass along to the X server at connection setup.  MIT-MAGIC-COOKIE-1 and
XDM-AUTHORIZATION-1 store secret data in the file; so anyone who can read
the file can gain access to the X server.  SUN-DES-1 stores only the
identity of the principal who started the server
(unix.\fIhostname\fP@\fIdomain\fP when the server is started by \fIxdm\fP),
and so it is not useful to anyone not authorized to connect to the server.
.PP
Each entry in the \fI.Xauthority\fP file matches a certain connection family
(TCP/IP, DECnet or local connections) and X display name (hostname plus display
number).  This allows multiple authorization entries for different displays
to share the same data file.  A special connection family (FamilyWild, value
65535) causes an entry to match every display, allowing the entry to be used
for all connections.  Each entry additionally contains the authorization
name and whatever private authorization data is needed by that authorization
type to generate the correct information at connection setup time.
.PP
The \fIxauth\fP program manipulates the \fI.Xauthority\fP file format.
It understands the semantics of the connection families and address formats,
displaying them in an easy to understand format.  It also understands that
SUN-DES-1 and MIT-KERBEROS-5 use
string values for the authorization data, and displays
them appropriately.
.PP
The X server (when running on a workstation) reads authorization
information from a file name passed on the command line with the \fI\-auth\fP
option (see the \fIXserver\fP manual page).  The authorization entries in
the file are used to control access to the server.  In each of the
authorization schemes listed above, the data needed by the server to initialize
an authorization scheme is identical to the data needed by the client to
generate the appropriate authorization information, so the same file can be
used by both processes.  This is especially useful when \fIxinit\fP is used.
.IP "MIT-MAGIC-COOKIE-1"
This system uses 128 bits of data shared between the user and the X server.
Any collection of bits can be used.  \fIXdm\fP generates these keys using a
cryptographically secure pseudo random number generator, and so the key to
the next session cannot be computed from the current session key.
.IP "XDM-AUTHORIZATION-1"
This system uses two pieces of information.  First, 64 bits of random data,
second a 56 bit DES encryption key (again, random data) stored
in 8 bytes, the last byte of which is ignored.  \fIXdm\fP generates these keys
using the same random number generator as is used for MIT-MAGIC-COOKIE-1.
.IP "SUN-DES-1"
This system needs a string representation of the principal which identifies
the associated X server.
This information is used to encrypt the client's authority information
when it is sent to the X server.
When \fIxdm\fP starts the X server, it uses the root
principal for the machine on which it is running
(unix.\fIhostname\fP@\fIdomain\fP, e.g.,
"unix.expire.lcs.mit.edu@our.domain.edu").  Putting the correct principal
name in the \fI.Xauthority\fP file causes Xlib to generate the appropriate
authorization information using the secure RPC library.
.IP "MIT-KERBEROS-5"
Kerberos reads tickets from the cache pointed to by the
\fIKRB5CCNAME\fP environment variable, so does not use any data from
the \fI.Xauthority\fP file.  An entry with no data must still exist to tell
clients that MIT-KERBEROS-5 is available.
.IP
Unlike the \fI.Xauthority\fP file for clients, the authority file
passed by xdm to 
a local X server (with ``\fB\-auth\fP \fIfilename\fP'', see xdm(1))
does contain the name of the credentials cache, since 
the X server will not have the 
\fIKRB5CCNAME\fP environment variable set.
The data of the MIT-KERBEROS-5 entry is the credentials cache name and
has the form ``UU:FILE:\fIfilename\fP'', where \fIfilename\fP is the
name of the credentials cache file created by xdm.  Note again that
this form is \fInot\fP used by clients.
.SH FILES
\&.Xauthority
.SH "SEE ALSO"
X(__miscmansuffix__), xdm(1), xauth(1), xhost(1), xinit(1), Xserver(1)
