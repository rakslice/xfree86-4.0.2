.\" $XFree86: xc/programs/Xserver/hw/xfree86/XF86Config.cpp,v 1.13 2000/12/11 20:18:01 dawes Exp $
.\" shorthand for double quote that works everywhere.
.ds q \N'34'
.TH XF86Config __filemansuffix__ "Version 4.0.2"  "XFree86"
.SH NAME
XF86Config - Configuration File for XFree86
.SH DESCRIPTION
.I XFree86
uses a configuration file called
.B XF86Config
for its initial setup.  This configuration file is searched for in the
following places when the server is started as a normal user:
.PP
.RS 4
.nf
.RI /etc/X11/ <cmdline>
.RI __projectroot__/etc/X11/ <cmdline>
.RB /etc/X11/ $XF86CONFIG
.RB __projectroot__/etc/X11/ $XF86CONFIG
/etc/X11/XF86Config-4
/etc/X11/XF86Config
/etc/XF86Config
.RI __projectroot__/etc/X11/XF86Config. <hostname>
__projectroot__/etc/X11/XF86Config-4
__projectroot__/etc/X11/XF86Config
.RI __projectroot__/lib/X11/XF86Config. <hostname>
__projectroot__/lib/X11/XF86Config-4
__projectroot__/lib/X11/XF86Config
.fi
.RE
.PP
where
.I <cmdline>
is a relative path (with no ".." components) specified with the
.B \-xf86config
command line option,
.B $XF86CONFIG
is the relative path (with no ".." components) specified by that
environment variable, and
.I <hostname>
is the machines hostname as reported by gethostname(3).
.PP
When the X server is started by the "root" user, the config file search
locations are as follows:
.PP
.RS 4
.nf
.I <cmdline>
.RI /etc/X11/ <cmdline>
.RI __projectroot__/etc/X11/ <cmdline>
.B $XF86CONFIG
.RB /etc/X11/ $XF86CONFIG
.RB __projectroot__/etc/X11/ $XF86CONFIG
.BR $HOME /XF86Config
/etc/X11/XF86Config-4
/etc/X11/XF86Config
/etc/XF86Config
.RI __projectroot__/etc/X11/XF86Config. <hostname>
__projectroot__/etc/X11/XF86Config-4
__projectroot__/etc/X11/XF86Config
.RI __projectroot__/lib/X11/XF86Config. <hostname>
__projectroot__/lib/X11/XF86Config-4
__projectroot__/lib/X11/XF86Config
.fi
.RE
.PP
where
.I <cmdline>
is the path specified with the
.B \-xf86config
command line option (which may be absolute or relative),
.B $XF86CONFIG
is the path specified by that
environment variable (absolute or relative),
.B $HOME
is the path specified by that environment variable (usually the home
directory), and
.I <hostname>
is the machines hostname as reported by gethostname(3).
.PP
The
.B XF86Config
file is composed of a number of sections which may be present in any
order.  Each section has
the form:
.PP
.RS 4
.nf
.\" Some man macros don't handle quotes in .BI, etc very well
.\" \fBSection "\fP\fISectionName\fP\fB"\fP
.BI "Section  \*q" SectionName \*q
.RI "    " SectionEntry
    ...
.B EndSection
.fi
.RE
.PP
The section names are:
.PP
.RS 4
.nf
.BR "Files          " "File pathnames"
.BR "ServerFlags    " "Server flags"
.BR "Module         " "Dynamic module loading"
.BR "InputDevice    " "Input device description"
.BR "Device         " "Graphics device description"
.BR "VideoAdaptor   " "Xv video adaptor description"
.BR "Monitor        " "Monitor description"
.BR "Modes          " "Video modes descriptions"
.BR "Screen         " "Screen configuration"
.BR "ServerLayout   " "Overall layout"
.BR "DRI            " "DRI-specific configuration"
.BR "Vendor         " "Vendor-specific configuration"
.fi
.RE
.PP
The following obsolete section names are still recognised for compatibility
purposes.  In new config files, the
.B InputDevice
section should be used instead.
.PP
.RS 4
.nf
.BR "Keyboard       " "Keyboard configuration"
.BR "Pointer        " "Pointer/mouse configuration"
.fi
.RE
.PP
The old
.B XInput
section is no longer recognised.
.PP
The
.B ServerLayout
sections are at the highest level.  They bind together the input and
output devices that will be used in a session.  The input devices
are described in the
.B InputDevice
sections.  Output devices usually consist of multiple independent
components (e.g., and graphics board and a monitor).  These multiple
components are bound together in the
.B Screen
sections, and it is these that are referenced by the
.B ServerLayout
section.  Each
.B Screen
section binds together a graphics board and a monitor.  The graphics
boards are described in the
.B Device
sections, and the monitors are described in the
.B Monitor
sections.
.PP
Config file keywords are case-insensitive, and "_" characters are
ignored.  Most strings (including
.B Option
names) are also case-insensitive, and insensitive to white space and
"_" characters.
.PP
Each config file entry usually take up a single line in the file.
They consist of a keyword, which is possibly followed by one or
more arguments, with the number and types of the arguments depending
on the keyword.  The argument types are:
.PP
.RS 4
.nf
.BR "Integer     " "an integer number in decimal, hex or octal"
.BR "Real        " "a floating point number"
.BR "String      " "a string enclosed in double quote marks (\*q)"
.fi
.RE
.PP
Note: hex integer values must be prefixed with "0x", and octal values
with "0".
.PP
A special keyword called
.B Option
may be used to provide free-form data to various components of the server.
The
.B Option
keyword takes either one or two string arguments.  The first is the option
name, and the optional second argument is the option value.  Some commonly
used option value types include:
.PP
.RS 4
.nf
.BR "Integer     " "an integer number in decimal, hex or octal"
.BR "Real        " "a floating point number"
.BR "String      " "a sequence of characters"
.BR "Boolean     " "a boolean value (see below)"
.BR "Frequency   " "a frequency value (see below)"
.fi
.RE
.PP
Note that
.I all
.B Option
values, not just strings, must be enclosed in quotes.
.PP
Boolean options may optionally have a value specified.  When no value
is specified, the option's value is
.BR TRUE .
The following boolean option values are recognised as
.BR TRUE :
.PP
.RS 4
.BR 1 ,
.BR on ,
.BR true ,
.B yes
.RE
.PP
and the following boolean option values are recognised as
.BR FALSE :
.PP
.RS 4
.BR 0 ,
.BR off ,
.BR false ,
.B no
.RE
.PP
If an option name is prefixed with
.RB \*q No \*q,
then the option value is negated.
.PP
Example: the following option entries are equivalent:
.PP
.RS 4
.nf
.B "Option \*qAccel\*q   \*qOff\*q"
.B "Option \*qNoAccel\*q"
.B "Option \*qNoAccel\*q \*qOn\*q"
.B "Option \*qAccel\*q   \*qfalse\*q"
.B "Option \*qAccel\*q   \*qno\*q"
.fi
.RE
.PP
Frequency option values consist of a real number that is optionally
followed by one of the following frequency units:
.PP
.RS 4
.BR Hz ,
.BR k ,
.BR kHz ,
.BR M ,
.B MHz
.RE
.PP
When the unit name is omitted, the correct units will be determined from
the value and the expectations of the appropriate range of the value.
It is recommended that the units always be specified when using frequency
option values to avoid any errors in determining the value.
.SH FILES SECTION
The
.B Files
section is used to specify some path names required by the server.
Some of these paths can also be set from the command line (see
.I Xserver(1)
and
.IR XFree86(1) ).
The command line settings override the values specified in the config
file.
The entries that can appear in this section are:
.TP 7
.BI "FontPath \*q" path \*q
sets the search path for fonts.  This path is a comma separated
list of font path elements which the X server searches for font databases.
Multiple
.B FontPath
entries may be specified, and they will be
concatenated to build up the fontpath used by the server.
Font path elements may be either absolute directory paths, or
a font server identifier.  Font server identifiers have the form:
.PP
.RS 11
.IR <trans> / <hostname> : <port-number>
.RE
.PP
.RS 7
where
.I <trans>
is the transport type to use to connect to the font server (e.g.,
.B unix
for UNIX-domain sockets or
.B tcp
for a TCP/IP connection),
.I <hostname>
is the hostname of the machine running the font server, and
.I <port-number>
is the port number that the font server is listening on (usually 7100).
.PP
When this entry is not specified in the config file, the server falls back
to the compiled-in default font path, which contains the following
font path elements:
.PP
.RS 4
.nf
__projectroot__/lib/X11/fonts/misc/
__projectroot__/lib/X11/fonts/Speedo/
__projectroot__/lib/X11/fonts/Type1/
__projectroot__/lib/X11/fonts/CID/
__projectroot__/lib/X11/fonts/75dpi/
__projectroot__/lib/X11/fonts/100dpi/
.fi
.RE
.PP
The recommended font path contains the following font path elements:
.PP
.RS 4
.nf
__projectroot__/lib/X11/fonts/local/
__projectroot__/lib/X11/fonts/misc/
__projectroot__/lib/X11/fonts/75dpi/:unscaled
__projectroot__/lib/X11/fonts/100dpi/:unscaled
__projectroot__/lib/X11/fonts/Type1/
__projectroot__/lib/X11/fonts/CID/
__projectroot__/lib/X11/fonts/Speedo/
__projectroot__/lib/X11/fonts/75dpi/
__projectroot__/lib/X11/fonts/100dpi/
.fi
.RE
.PP
Font path elements that are found to be invalid are removed from the
font path when the server starts up.
.RE
.TP 7
.BI "RGBPath \*q" path \*q
sets the path name for the RGB color database.
When this entry is not specified in the config file, the server falls back
to the compiled-in default RGB path, which is:
.PP
.RS 11
__projectroot__/lib/X11/rgb
.RE
.TP 7
.BI "ModulePath \*q" path \*q
sets the search path for loadable X server modules.  This path is a
comma separated list of directories which the X server searches for
loadable modules loading in the order specified.  Multiple
.B ModulePath
entries may be specified, and they will be concatenated to build the
module search path used by the server.
.\" The LogFile keyword is not currently implemented
.ig
.TP 7
.BI "LogFile \*q" path \*q
sets the name of the X server log file.  The default log file name is
.PP
.RS 11
.RI __logdir__/XFree86. <n> .log
.RE
.PP
.RS 7
where
.I <n>
is the display number for the X server.
..
.SH SERVERFLAGS SECTION
The
.B ServerFlags
section is used to specify some global
X server options.  All of the entries in this section are
.BR Options ,
although for compatibility purposes some of the old style entries are
still recognised.  Those old style entries are not documented here, and
using them is discouraged.
.PP
.B Options
specified in this section may be overridden by
.B Options
specified in the active
.B ServerLayout
section.  Options with command line equivalents are overridden when their
command line equivalent is used.  The options recognised by this section
are:
.TP 7
.BI "Option \*qNoTrapSignals\*q  \*q" boolean \*q
This prevents the X server from trapping a range of unexpected
fatal signals and exiting cleanly.  Instead, the X server will die
and drop core where the fault occurred.  The default behaviour is
for the X server exit cleanly, but still drop a core file.  In
general you never want to use this option unless you are debugging
an X server problem and know how to deal with the consequences.
.TP 7
.BI "Option \*qDontZap\*q  \*q" boolean \*q
This disallows the use of the
.B Ctrl+Alt+Backspace
sequence.  That sequence is normally used to terminate the X server.
When this option is enabled, that key sequence has no special meaning
and is passed to clients.  Default: off.
.TP 7
.BI "Option \*qDontZoom\*q  \*q" boolean \*q
This disallows the use of the
.B Ctrl+Alt+Keypad-Plus
and
.B Ctrl+Alt+Keypad-Minus
sequences.  These sequences allows you to switch between video modes.
When this option is enabled, those key sequences have no special meaning
and are passed to clients.  Default: off.
.TP 7
.BI "Option \*qDisableVidModeExtension\*q  \*q" boolean \*q
This disables the parts of the VidMode extension used by the xvidtune client
that can be used to change the video modes.  Default: the VidMode extension
is enabled.
.TP 7
.BI "Option \*qAllowNonLocalXvidtune\*q  \*q" boolean \*q
This allows the xvidtune client (and other clients that use the VidMode
extension) to connect from another host.  Default: off.
.TP 7
.BI "Option \*qDisableModInDev\*q  \*q" boolean \*q
This disables the parts of the XFree86-Misc extension that can be used to
modify the input device settings dynamically.  Default: that functionality
is enabled.
.TP 7
.BI "Option \*qAllowNonLocalModInDev\*q  \*q" boolean \*q
This allows a client to connect from another host and change keyboard
and mouse settings in the running server.  Default: off.
.TP 7
.BI "Option \*qAllowMouseOpenFail\*q  \*q" boolean \*q
This allows the server to start up even if the mouse device can't be
opened/initialised.  Default: false.
.TP 7
.BI "Option \*qVTInit\*q  \*q" command \*q
Runs
.I command
after the VT used by the server has been opened.
The command string is passed to "/bin/sh -c", and is run with the
real user's id with stdin and stdout set to the VT.  The purpose
of this option is to allow system dependent VT initialisation
commands to be run.  This option should rarely be needed.  Default: not set.
.TP 7
.BI "Option \*qVTSysReq\*q  \*q" boolean \*q
enables the SYSV-style VT switch sequence for non-SYSV systems
which support VT switching.  This sequence is
.B Alt-SysRq
followed
by a function key
.RB ( Fn ).
This prevents the X server trapping the
keys used for the default VT switch sequence, which means that clients can
access them.  Default: off.
.\" The following four options are "undocumented".
.ig
.TP 7
.BI "Option \*qPciProbe1\*q"
Use PCI probe method 1.  Default: set.
.TP 7
.BI "Option \*qPciProbe2\*q"
Use PCI probe method 2.  Default: not set.
.TP 7
.BI "Option \*qPciForceConfig1\*q"
Force the use PCI config type 1.  Default: not set.
.TP 7
.BI "Option \*qPciForceConfig2\*q"
Force the use PCI config type 2.  Default: not set.
..
.TP 7
.BI "Option \*qBlankTime\*q  \*q" time \*q
sets the inactivity timeout for the blanking phase of the screensaver.
.I time
is in minutes.  This is equivalent to the Xserver's `-s' flag, and the
value can be changed at run-time with \fIxset(1)\fP.  Default: 10 minutes.
.TP 7
.BI "Option \*qStandbyTime\*q  \*q" time \*q
sets the inactivity timeout for the "standby" phase of DPMS mode.
.I time
is in minutes, and the value can be changed at run-time with \fIxset(1)\fP.
Default: 20 minutes.
This is only suitable for VESA DPMS compatible monitors, and may not be
supported by all video drivers.  It is only enabled for screens that
have the
.B \*qDPMS\*q
option set (see the MONITOR section below). 
.TP 7
.BI "Option \*qSuspendTime\*q  \*q" time \*q
sets the inactivity timeout for the "suspend" phase of DPMS mode.
.I time
is in minutes, and the value can be changed at run-time with \fIxset(1)\fP.
Default: 30 minutes.
This is only suitable for VESA DPMS compatible monitors, and may not be
supported by all video drivers.  It is only enabled for screens that
have the
.B \*qDPMS\*q
option set (see the MONITOR section below). 
.TP 7
.BI "Option \*qOffTime\*q  \*q" time \*q
sets the inactivity timeout for the "off" phase of DPMS mode.
.I time
is in minutes, and the value can be changed at run-time with \fIxset(1)\fP.
Default: 40 minutes.
This is only suitable for VESA DPMS compatible monitors, and may not be
supported by all video drivers.  It is only enabled for screens that
have the
.B \*qDPMS\*q
option set (see the MONITOR section below). 
.TP 7
.BI "Option \*qPixmap\*q  \*q" bpp \*q
This sets the pixmap format to use for depth 24.  Allowed values for
.I bpp
are 24 and 32.  Default: 32 unless driver constraints don't allow this
(which is rare).  Note: some clients don't behave well when
this value is set to 24.
.TP 7
.BI "Option \*qPC98\*q  \*q" boolean \*q
Specify that the machine is a Japanese PC-98 machine.  This should not
be enabled for anything other than the Japanese-specific PC-98
architecture.  Default: auto-detected.
.\" Doubt this should be documented.
.ig
.TP 7
.BI "Option \*qEstimateSizesAggressively\*q  \*q" value \*q
This option affects the way that bus resource sizes are estimated.  Default: 0.
..
.TP 7
.BI "Option \*qNoPM\*q  \*q" boolean \*q
Disables something to do with power management events.  Default: PM enabled
on platforms that support it.
.TP 7
.BI "Option \*qXinerama\*q  \*q" boolean \*q
enable or disable XINERAMA extension. Default is disabled. 
.SH MODULE SECTION
The
.B Module
section is used to specify which X server modules should be loaded.
This section is ignored when the X server is built in static form.
The types of modules normally loaded in this section are X server
extension modules, and font rasteriser modules.  Most other module types
are loaded automatically when they are needed via other mechanisms.
.PP
Entries in this section may be in two forms.   The first and most commonly
used form is an entry that uses the
.B Load
keyword, as described here:
.TP 7
.BI "Load  \*q" modulename \*q
This instructs the server to load the module called
.IR modulename .
The module name given should be the module's standard name, not the
module file name.  The standard name is case-sensitive, and does not
include the "lib" prefix, or the ".a", ".o", or ".so" suffixes.
.PP
.RS 7
Example: the Type 1 font rasteriser can be loaded with the following entry:
.PP
.RS 4
.B "Load \*qtype1\*q"
.RE
.RE
.PP
The second form of entry is a
.BR SubSection,
with the subsection name being the module name, and the contents of the
.B SubSection
being
.B Options
that are passed to the module when it is loaded.
.PP
Example: the extmod module (which contains a miscellaneous group of
server extensions) can be loaded, with the XFree86-DGA extension
disabled by using the following entry:
.PP
.RS 4
.nf
.B "SubSection \*qextmod\*q"
.B "   Option  \*qomit XFree86-DGA\*q"
.B EndSubSection
.fi
.RE
.PP
Modules are searched for in each directory specified in the
.B ModulePath
search path, and in the drivers, input, extensions, fonts, and
internal subdirectories of each of those directories.
In addition to this, operating system specific subdirectories of all
the above are searched first if they exist.
.PP
To see what font and extension modules are available, check the contents
of the following directories:
.PP
.RS 4
.nf
__projectroot__/lib/modules/fonts
__projectroot__/lib/modules/extensions
.fi
.RE
.PP
The "bitmap" font modules is loaded automatically.  It is recommended
that at very least the "extmod" extension module be loaded.  If it isn't
some commonly used server extensions (like the SHAPE extension) will not be
available.
.SH INPUTDEVICE SECTION
The config file may have multiple
.B InputDevice
sections.  There will normally be at least two: one for the core (primary)
keyboard, and one of the core pointer.
.PP
.B InputDevice
sections have the following format:
.PP
.RS 4
.nf
.B  "Section \*qInputDevice\*q"
.BI "    Identifier \*q" name \*q
.BI "    Driver     \*q" inputdriver \*q
.I  "    options"
.I  "    ..."
.B  "EndSection"
.fi
.RE
.PP
The
.B Identifier
entry specifies the unique name for this input device.  The
.B Driver
entry specifies the name of the driver to use for this input device.
When using the loadable server, the input driver module
.RI \*q inputdriver \*q
will be loaded for each active
.B InputDevice
section.  An
.B InputDevice
section is considered active if it is referenced by an active
.B ServerLayout
section, or if it is referenced by the
.B \-keyboard
or
.B \-pointer
command line options.
The most commonly used input drivers are "keyboard" and "mouse".
.PP
.B InputDevice
sections recognise some driver-independent
.BR Options ,
which are described here.  See the individual input driver manual pages
for a description of the device-specific options.
.TP 7
.BI "Option \*qCorePointer\*q"
When this is set, the input device is installed as the core (primary)
pointer device.  There must be exactly one core pointer.  If this option
is not set here, or in the
.B ServerLayout
section, or from the
.B \-pointer
command line option, then the first input device that is capable of
being used as a core pointer will be selected as the core pointer.
This option is implicitly set when the obsolete
.B Pointer
section is used.
.TP 7
.BI "Option \*qCoreKeyboard\*q"
When this is set, the input device is to be installed as the core
(primary) keyboard device.  There must be exactly one core keyboard.  If
this option is not set here, in the
.B ServerLayout
section, or from the
.B \-keyboard
command line option, then the first input device that is capable of
being used as a core keyboard will be selected as the core keyboard.
This option is implicitly set when the obsolete
.B Keyboard
section is used.
.TP 7
.BI "Option \*qAlwaysCore\*q  \*q" boolean \*q
.TP 7
.BI "Option \*qSendCoreEvents\*q  \*q" boolean \*q
Both of these options are equivalent, and when enabled cause the
input device to always report core events.  This can be used, for
example, to allow an additional pointer device to generate core
pointer events (like moving the cursor, etc).
.TP 4
.BI "Option \*qHistorySize\*q  \*q" number \*q
Sets the motion history size.  Default: 0.
.TP 7
.BI "Option \*qSendDragEvents\*q  \*q" boolean \*q
???
.SH DEVICE SECTION
The config file may have multiple
.B Device
sections.  There must be at least one, for the video card being used.
.PP
.B Device
sections have the following format:
.PP
.RS 4
.nf
.B  "Section \*qDevice\*q"
.BI "    Identifier \*q" name \*q
.BI "    Driver     \*q" driver \*q
.I  "    entries"
.I  "    ..."
.B  "EndSection"
.fi
.RE
.PP
The
.B Identifier
entry specifies the unique name for this graphics device.  The
.B Driver
entry specifies the name of the driver to use for this graphics device.
When using the loadable server, the driver module
.RI \*q driver \*q
will be loaded for each active
.B Device
section.  A
.B Device
section is considered active if it is referenced by an active
.B Screen
section.
.PP
.B Device
sections recognise some driver-independent entries and
.BR Options ,
which are described here.  Not all drivers make use of these
driver-independent entries, and many of those that do don't require them
to be specified because the information is auto-detected.  See the
individual graphics driver manual pages for further information about
this, and for a description of the device-specific options.
Note that most of the
.B Options
listed here (but not the other entries) may be specified in the
.B Screen
section instead of here in the
.B Device
section.
.TP 7
.BI "BusID  \*q" bus-id \*q
This specifies the bus location of the graphics card.  For PCI/AGP cards,
the
.I bus-id
string has the form
.BI PCI: bus : device : function
(e.g., "PCI:1:0:0" might be appropriate for an AGP card).
This field is usually optional in single-head configurations when using
the primary graphics card.  In multi-head configurations, or when using
a secondary graphics card in a single-head configuration, this entry is
mandatory.  Its main purpose is to make an unambiguous connection between
the device section and the hardware it is representing.  This information
can usually be found by running the X server with the
.B \-scanpci
command line option.
.TP 7
.BI "Screen  " number
This option is mandatory for cards where a single PCI entity can drive more
than one display (i.e., multiple CRTCs sharing a single graphics accelerator
and video memory).  One
.B Device
section is required for each head, and this
parameter determines which head each of the
.B Device
sections applies to.  The legal values of
.I number
range from 0 to one less than the total number of heads per entity.
Most drivers require that the primary screen (0) be present.
.TP 7
.BI "Chipset  \*q" chipset \*q
This usually optional entry specifies the chipset used on the graphics
board.  In most cases this entry is not required because the drivers
will probe the hardware to determine the chipset type.  Don't
specify it unless the driver-specific documentation recommends that you
do.
.TP 7
.BI "Ramdac  \*q" ramdac-type \*q
This optional entry specifies the type of RAMDAC used on the graphics
board.  This is only used by a few of the drivers, and in most cases it
is not required because the drivers will probe the hardware to determine
the RAMDAC type where possible.  Don't specify it unless the
driver-specific documentation recommends that you do.
.TP 7
.BI "DacSpeed  " speed
.TP 7
.BI "DacSpeed  " "speed-8 speed-16 speed-24 speed-32"
This optional entry specifies the RAMDAC speed rating (which is usually
printed on the RAMDAC chip).  The speed is in MHz.  When one value is
given, it applies to all framebuffer pixel sizes.  When multiple values
are give, they apply to the framebuffer pixel sizes 8, 16, 24 and 32
respectively.  This is not used by many drivers, and only needs to be
specified when the speed rating of the RAMDAC is different from the
defaults built in to driver, or when the driver can't auto-detect the
correct defaults.  Don't specify it unless the driver-specific
documentation recommends that you do.
.TP 7
.BI "Clocks  " "clock ..."
specifies the pixel that are on your graphics board.  The clocks are in
MHz, and may be specified as a floating point number.  The value is
stored internally to the nearest kHz.  The ordering of the clocks is
important.  It must match the order in which they are selected on the
graphics board.  Multiple
.B Clocks
lines may be specified, and each is concatenated to form the list.  Most
drivers do not use this entry, and it is only required for some older
boards with non-programmable clocks.  Don't specify this entry unless
the driver-specific documentation explicitly recommends that you do.
.TP
.BI "ClockChip  \*q" clockchip-type \*q
This optional entry is used to specify the clock chip type on
graphics boards which have a programmable clock generator.  Only
a few X servers support programmable clock chips.  For details,
see the appropriate X server manual page.
.TP 7
.BI "VideoRam  " "mem"
This optional entry specifies the amount of video ram that is installed
on the graphics board. This is measured in kBytes.  In most cases this
is not required because the X server probes the graphics board to
determine this quantity.  The driver-specific documentation should
indicate when it might be needed.
.TP 7
.BI "BiosBase  " "baseaddress"
This optional entry specifies the base address of the video BIOS
for the VGA board.  This address is normally auto-detected, and should
only be specified if the driver-specific documentation recommends it.
.TP 7
.BI "MemBase  " "baseaddress"
This optional entry specifies the memory base address of a graphics
board's linear frame buffer.  This entry is not used by many drivers,
and it should only be specified if the driver-specific documentation
recommends it.
.TP 7
.BI "IOBase  " "baseaddress"
This optional entry specifies the IO base address.  This entry is not
used by many drivers, and it should only be specified if the
driver-specific documentation recommends it.
.TP 7
.BI "ChipID  " "id"
This optional entry specifies a numerical ID representing the chip type.
For PCI cards, it is usually the device ID.  This can be used to override
the auto-detection, but that should only be done when the driver-specific
documentation recommends it.
.TP 7
.BI "ChipRev  " "rev"
This optional entry specifies the chip revision number.  This can be
used to override the auto-detection, but that should only be done when
the driver-specific documentation recommends it.
.TP 7
.BI "TextClockFreq  " "freq"
This optional entry specifies the pixel clock frequency that is used
for the regular text mode.  The frequency is specified in MHz.  This is
rarely used.
.ig
.TP 7
This optional entry allows an IRQ number to be specified.
..
.TP 7
.B Options
Option flags may be specified in the
.B Device
sections.  These include driver-specific options and driver-independent
options.  The former are described in the driver-specific documentation.
Some of the latter are described below in the section about the
.B Screen
section, and they may also be included here.

.SH VIDEOADAPTOR SECTION
Nobody wants to say how this works.  Maybe nobody knows ...

.SH MONITOR SECTION
The config file may have multiple
.B Monitor
sections.  There must be at least one, for the monitor being used.
.PP
.B Monitor
sections have the following format:
.PP
.RS 4
.nf
.B  "Section \*qMonitor\*q"
.BI "    Identifier \*q" name \*q
.I  "    entries"
.I  "    ..."
.B  "EndSection"
.fi
.RE
.PP
The
.B Identifier
entry specifies the unique name for this monitor.  The
.B Monitor
section provides information about the specifications of the monitor,
monitor-specific
.BR Options ,
and information about the video modes to use with the monitor.  Specifying
video modes is optional because the server now has a built-in list of
VESA standard modes.  When modes are specified explicitly in the
.B Monitor
section (with the
.BR Modes ,
.BR ModeLine ,
or
.B UseModes
keywords), built-in modes with the same names are not included.  Built-in
modes with different names are, however, still implicitly included.
.PP
The entries that may be used in
.B Monitor
sections are described below.
.TP 7
.BI "VendorName  \*q" vendor \*q
This optional entry specifies the monitor's manufacturer.
.TP 7
.BI "ModelName  \*q" model \*q
This optional entry specifies the monitor's model.
.TP 7
.BI "HorizSync  " "horizsync-range"
gives the range(s) of horizontal sync frequencies supported by the
monitor.
.I horizsync-range
may be a comma separated list of either discrete values or ranges of
values.  A range of values is two values separated by a dash.  By default
the values are in units of kHz.  They may be specified in MHz or Hz if
.B MHz
or
.B Hz
is added to the end of the line.  The data given here is used by the X
server to determine if video modes are within the specifications of the
monitor.  This information should be available in the monitor's handbook.
If this entry is omitted, a default range of 28\-33kHz is used.
.TP 7
.BI "VertRefresh  " "vertrefresh-range"
gives the range(s) of vertical refresh frequencies supported by the
monitor.
.I vertrefresh-range
may be a comma separated list of either discrete values or ranges of
values.  A range of values is two values separated by a dash.  By default
the values are in units of Hz.  They may be specified in MHz or kHz if
.B MHz
or
.B kHz
is added to the end of the line.  The data given here is used by the X
server to determine if video modes are within the specifications of the
monitor.  This information should be available in the monitor's handbook.
If this entry is omitted, a default range of 43-72Hz is used.
.TP 7
.BI "Gamma  " "gamma-value"
.TP 7
.BI "Gamma  " "red-gamma green-gamma blue-gamma"
This is an optional entry that can be used to specify the gamma
correction for the monitor.  It may be specified as either a single
value or as three separate RGB values.  The values should be in the range
0.1 to 10.0, and the default is 1.0.  Not all drivers are capable
of using this information.
.TP 7
.BI "UseModes  \*q" modesection-id \*q
Include the set of modes listed in the
.B Modes
section called
.IR modesection-id.
This make all of the modes defined in that section available for use
by this monitor.
.TP 7
.B Mode \fI"name"\fP
This is an optional multi-line entry that can be used to provide
definitions for video modes for the monitor.  In most cases this isn't
necessary because the built-in set of VESA standard modes will be
sufficient.  The
.B Mode
keyword indicates the start of a multi-line video mode description.
The mode description is terminated with the
.B EndMode
keyword.  The mode description consists of the following entries:
.RS 7
.TP 4
.BI "DotClock  " clock
is the dot (pixel) clock rate to be used for the mode.
.TP 4
.BI "HTimings  " "hdisp hsyncstart hsyncend htotal"
specifies the horizontal timings for the mode.
.TP 4
.BI "VTimings  " "vdisp vsyncstart vsyncend vtotal"
specifies the vertical timings for the mode.
.TP 4
.BI "Flags  \*q" flag \*q " ..."
specifies an optional set of mode flags, each of which is a separate
string in double quotes.
.B \*qInterlace\*q
indicates that the mode is interlaced.
.B \*qDoubleScan\*q
indicates a mode where each scanline is doubled.
.B \*q+HSync\*q
and
.B \*q\-HSync\*q
can be used to select the polarity of the HSync signal.
.B \*q+VSync\*q
and
.B \*q\-VSync\*q
can be used to select the polarity of the VSync signal.
.B \*qComposite\*q
can be used to specify composite sync on hardware where this is supported.
Additionally, on some hardware,
.B \*q+CSync\*q
and
.B \*q\-CSync\*q
may be used to select the composite sync polarity.
.TP 4
.BI "HSkew  " hskew
specifies the number of pixels (towards the right edge of the screen)
by which the display enable signal is to be skewed.  Not all drivers
use this information.  This option might become necessary to override
the default value supplied by the server (if any).  "Roving" horizontal
lines indicate this value needs to be increased.  If the last few pixels
on a scan line appear on the left of the screen, this value should be
decreased.
.TP 4
.BI "VScan  " vscan
specifies the number of times each scanline is painted on the screen.
Not all drivers use this information.  Values less than 1 are treated
as 1, which is the default.  Generally, the
.B \*qDoubleScan\*q
.B Flag
mentioned above doubles this value.
.RE
.TP 7
.BI "ModeLine  \*q" name \*q " mode-description"
This entry is a more compact version of the
.B Mode
entry, and it also can be used to specify video modes for the monitor.
is a single line format for specifying video modes.  In most cases this
isn't necessary because the built-in set of VESA standard modes will be
sufficient.
.PP
.RS 7
The
.I mode-description
is in four sections, the first three of which are mandatory.  The first
is the dot (pixel) clock.  This is a single number specifying the pixel
clock rate for the mode in MHz.  The second section is a list of four
numbers specifying the horizontal timings.  These numbers are the
.IR hdisp ,
.IR hsyncstart ,
.IR hsyncend ,
and
.I htotal
values.  The third section is a list of four numbers specifying the
vertical timings.  These numbers are the
.IR vdisp ,
.IR vsyncstart ,
.IR vsyncend ,
and
.I vtotal
values.  The final section is a list of flags specifying other
characteristics of the mode.
.B Interlace
indicates that the mode is interlaced.
.B DoubleScan
indicates a mode where each scanline is doubled.
.B +HSync
and
.B \-HSync
can be used to select the polarity of the HSync signal.
.B +VSync
and
.B \-VSync
can be used to select the polarity of the VSync signal.
.B Composite
can be used to specify composite sync on hardware where this is supported.
Additionally, on some hardware,
.B +CSync
and
.B \-CSync
may be used to select the composite sync polarity.  The
.B HSkew
and
.B VScan
options mentioned above in the
.B Modes
entry description can also be used here.
.RE
.TP 7
.B Options
Some
.B Option
flags that may be useful to include in
.B Monitor
sections (when needed) include
.BR \*qDPMS\*q ,
and
.BR \*qSyncOnGreen\*q .

.SH MODES SECTION
The config file may have multiple
.B Modes
sections, or none.  These sections provide a way of defining sets of
video modes independently of the
.B Monitor
sections.
.B Monitor
sections may include the definitions provided in these sections by
using the
.B UseModes
keyword.  In most cases the
.B Modes
sections are not necessary because the built-in set of VESA standard modes
will be sufficient.
.PP
.B Modes
sections have the following format:
.PP
.RS 4
.nf
.B  "Section \*qModes\*q"
.BI "    Identifier \*q" name \*q
.I  "    entries"
.I  "    ..."
.B  "EndSection"
.fi
.RE
.PP
The
.B Identifier
entry specifies the unique name for this set of mode descriptions.
The other entries permitted in
.B Modes
sections are the
.B Mode
and
.B ModeLine
entries that are described above in the
.B Monitor
section.
.SH SCREEN SECTION
The config file may have multiple
.B Screen
sections.  There must be at least one, for the "screen" being used.
A "screen" represents the binding of a graphics device
.RB ( Device
section) and a monitor
.RB ( Monitor
section).  A
.B Screen
section is considered "active" if it is referenced by an active
.B ServerLayout
section or by the
.B \-screen
command line option.  If neither of those is present, the first
.B Screen
section found in the config file is considered the active one.
.PP
.B Screen
sections have the following format:
.PP
.RS 4
.nf
.B  "Section \*qScreen\*q"
.BI "    Identifier \*q" name \*q
.BI "    Device     \*q" devid \*q
.BI "    Monitor    \*q" monid \*q
.I  "    entries"
.I  "    ..."
.BI "    SubSection \*qDisplay\*q"
.I  "       entries"
.I  "       ...
.B  "    EndSubSection"
.I  "    ..."
.B  "EndSection"
.fi
.RE
.PP
The
.B Identifier
entry specifies the unique name for this screen.  The
.B Screen
section provides information specific to the whole screen, including
screen-specific
.BR Options .
In multi-head configurations, there will be multiple active
.B Screen
sections, one for each head.
The entries available
for this section are:
.TP 7
.BI "Device  \*q" device-id \*q
This specifies the
.B Device
section to be used for this screen.  This is what ties a specific
graphics card to a screen.  The
.I device-id
must match the
.B Identifier
of a
.B Device
section in the config file.
.TP 7
.BI "Monitor  \*q" monitor-id \*q
specifies which monitor description is to be used for this screen.
.TP 7
.BI "VideoAdaptor  \*q" xv-id \*q
specifies an optional Xv video adaptor description to be used with this
screen.
.TP 7
.BI "DefaultDepth  " depth
specifies which color depth the server should use by default.  The
.B \-depth
command line option can be used to override this.  If neither is specified,
the default depth is driver-specific, but in most cases is 8.
.TP 7
.BI "DefaultFbBpp  " bpp
specifies which framebuffer layout to use by default.  The
.B \-fbbpp
command line option can be used to override this.  In most cases the
driver will chose the best default value for this.  The only case where
there is even a choice in this value is for depth 24, where some hardware
supports both a packed 24 bit framebuffer layout and a sparse 32 bit
framebuffer layout.
.TP 7
.B Options
Various
.B Option
flags may be specified in the
.B Screen
section.  Some are driver-specific and are described in the driver
documentation.  Others are driver-independent, and will eventually be
described here.
.PP
Each
.B Screen
section must contain one or more
.B Display
subsections.  Those subsections provide depth/fbbpp specific configuration
information, and the one chosen depends on the depth and/or fbbpp that
is being used for the screen.  The
.B Display
subsection format is described in the section below.

.SH DISPLAY SUBSECTION
Each
.B Screen
section may have multiple
.B Display
subsections.  There must be at least one, which matches the depth
and/or fbbpp values that are being used for the screen.  The "active"
.B Display
subsection is the first that matches the depth and/or fbbpp values being
used.
.PP
.B Display
subsections have the following format:
.PP
.RS 4
.nf
.B  "    SubSection \*qDisplay\*q"
.BI "        Depth  " depth
.I  "        entries"
.I  "        ..."
.B  "    EndSubSection"
.fi
.RE
.PP
.TP 7
.BI "Depth  " depth
This entry specifies what colour depth the
.B Display
subsection is to be used for.  This entry is usually mandatory,
but it may be omitted in some cases providing an
.B FbBpp
entry is present.  The range of
.I depth
values that are allowed depends on the driver.  Most driver support
8, 15, 16 and 24.  Some also support 1 and/or 4, and some may support
other values (like 30).  Note:
.I depth
means the number of bits in a pixel that are actually used to determine
the pixel colour.  32 is not a valid
.I depth
value.  Most hardware that uses 32 bits per pixel only uses 24 of them
to hold the colour information, which means that the colour depth is
24, not 32.
.TP 7
.BI "FbBpp  " bpp
This entry specifies the framebuffer format this
.B Display
subsection is to be used for.  This entry is only needed when providing
depth 24 configurations that allow a choice between a 24 bpp packed
framebuffer format and a 32bpp sparse framebuffer format.  In most cases
this entry should not be used.
.TP 7
.BI "Weight  " "red-weight green-weight blue-weight"
This optional entry specifies the relative RGB weighting to be used
for a screen is being used at depth 16 for drivers that allow multiple
formats.  This may also be specified from the command line with the
.B \-weight
option (see
.IR XFree86(1) ).
.TP 7
.BI "Virtual  " "xdim ydim"
This optional entry specifies the virtual screen resolution to be used.
.I xdim
must be a multiple of either 8 or 16 for most drivers, and a multiple
of 32 when running in monochrome mode.  The given value will be rounded
down if this is not the case.  Video modes which are too large for the
specified virtual size will be rejected.  If this entry is not present,
the virtual screen resolution will be set to accommodate all the valid
video modes given in the
.B Modes
entry.  Some drivers/hardware combinations do not support virtual screens.
Refer to the appropriate driver-specific documentation for details.
.TP 7
.BI "ViewPort  " "x0 y0"
This optional entry sets the upper left corner of the initial display.
This is only relevant when the virtual screen resolution is different
from the resolution of the initial video mode.  If this entry is not
given, then the initial display will be centered in the virtual display
area.
.TP 7
.BI "Modes  \*q" mode-name \*q " ..."
This entry is highly desirable for most drivers, and it specifies the list
of video modes to use.  Each
.I mode-name
specified must be in double quotes.  They must correspond to those
specified or referenced in the appropriate
.B Monitor
section (including implicitly referenced built-in VESA standard modes).
The server will delete modes from this list which don't satisfy various
requirements.  The first valid mode in this list will be the default
display mode for startup.  The list of valid modes is converted internally
into a circular list.  It is possible to switch to the next mode with
.B Ctrl+Alt+Keypad-Plus
and to the previous mode with
.BR Ctrl+Alt+Keypad-Minus .
When this entry is omitted, the largest valid mode referenced by the
appropriate
.B Monitor
section will be used.
.TP 7
.BI "Visual  \*q" visual-name \*q
This optional entry sets the default root visual type.  This may also
be specified from the command line (see the
.I Xserver(1)
man page).  The visual types available for depth 8 are (default is
.BR PseudoColor ):
.PP
.RS 11
.nf
.B StaticGray
.B GrayScale
.B StaticColor
.B PseudoColor
.B TrueColor
.B DirectColor
.fi
.RE
.PP
.RS 7
The visual type available for the depths 15, 16 and 24 are (default is
.BR TrueColor ):
.PP
.RS 4
.nf
.B TrueColor
.B DirectColor
.fi
.RE
.PP
Not all drivers support
.B DirectColor
at these depths.
.PP
The visual types available for the depth 4 are (default is
.BR StaticColor ):
.PP
.RS 4
.nf
.B StaticGray
.B GrayScale
.B StaticColor
.B PseudoColor
.fi
.RE
.PP
The visual type available for the depth 1 (monochrome) is
.BR StaticGray .
.RE
.TP 7
.BI "Black  " "red green blue"
This optional entry allows the "black" colour to be specified.  This
.TP 7
.BI "White  " "red green blue"
This optional entry allows the "white" colour to be specified.  This
is only supported at depth 1.  The default is white.
.TP 7
.B Options
Option flags may be specified in the
.B Display
subsections.  These may include driver-specific options and
driver-independent options.  The former are described in the
driver-specific documentation.  Some of the latter are described above
in the section about the
.B Screen
section, and they may also be included here.
.SH SERVERLAYOUT SECTION
The config file may have multiple
.B ServerLayout
sections.  
A "server layout" represents the binding of one or more screens
.RB ( Screen
sections) and one or more input devices
.RB ( InputDevice
sections) to form a complete configuration.  In multi-head configurations,
it also specifies the relative layout of the heads.  A
.B ServerLayout
section is considered "active" if it is referenced by the
.B \-layout
command line option.  If that option is not used, the first
.B ServerLayout
section found in the config file is considered the active one.  If no
.B ServerLayout
sections are present, the single active screen and two active (core)
input devices are selected as described in the relevant sections above.
.PP
.B ServerLayout
sections have the following format:
.PP
.RS 4
.nf
.B  "Section \*qServerLayout\*q"
.BI "    Identifier   \*q" name \*q
.BI "    Screen       \*q" screen-id \*q
.I  "    ..."
.BI "    InputDevice  \*q" idev-id \*q
.I  "    ..."
.I  "    options"
.I  "    ..."
.B  "EndSection"
.fi
.RE
.PP
The
.B Identifier
entry specifies the unique name for this server layout.  The
.B ServerLayout
section provides information specific to the whole session, including
session-specific
.BR Options .
The
.B ServerFlags
options (described above) may be specified here, and ones given here
override those given in the
.B ServerFlags
section.
.PP
The entries that may be used in this section are described here.
.TP 7
.BI "Screen  " "screen-num" " \*qscreen-id\*q " "position-information"
One of these entries must be given for each screen being used in
a session.  The
.I screen-id
field is mandatory, and specifies the
.B Screen
section being referenced.  The
.I screen-num
field is optional, and may be used to specify the screen number
in multi-head configurations.  When this field is omitted, the
screens will be numbered in the order that they are listed in.
The numbering starts from 0, and must be consecutive.  The
.I position-information
field describes the way multiple screens are positioned.  There are
a number of different ways that this information can be provided:
.RS 7
.TP 4
.BI "Absolute  " "x y"
This says that the upper left corner's coordinates are
.RI ( x , y ).
If the coordinates are omitted or if no positioning information
is given, (0,0) is assumed.
.TP 4
.BI "RightOf   \*q" screen-id \*q
.TP 4
.BI "LeftOf    \*q" screen-id \*q
.TP 4
.BI "Above     \*q" screen-id \*q
.TP 4
.BI "Below     \*q" screen-id \*q
.TP 4
.BI "Relative  \*q" screen-id \*q " x y"
These give the screen's location relative to another screen.
.RE
.TP 7
.BI "InputDevice  \*q" idev-id "\*q \*q" option \*q " ..."
One of these entries must be given for each input device being used in
a session.  Normally at least two are required, one each for the core
pointer and keyboard devices.  The
.I idev-id
field is mandatory, and specifies the name of the
.B InputDevice
section being referenced.  Multiple
.I option
fields may be specified, each in double quotes.  The options permitted
here are any that may also be given in the
.B InputDevice
sections.  Normally only session-specific input device options would
be used here.  The most commonly used options are:
.PP
.RS 11
.nf
.B \*qCorePointer\*q
.B \*qCoreKeyboard\*q
.B \*qSendCoreEvents\*q
.fi
.RE
.PP
.RS 7
and the first two should normally be used to indicate the core pointer
and core keyboard devices respectively.
.RE
.TP 7
.B Options
Any option permitted in the
.B ServerFlags
section may also be specified here.  When the same option appears in both
places, the value given here overrides the one given in the
.B ServerFlags
section.
.PP
Here is an example of a
.B ServerLayout
section for a dual headed configuration with two mice:
.PP
.RS 4
.nf
.B "Section \*qServerLayout\*q"
.B "    Identifier  \*qLayout 1\*q"
.B "    Screen      \*qMGA 1\*q"
.B "    Screen      \*qMGA 2\*q RightOf \*qMGA 1\*q"
.B "    InputDevice \*qKeyboard 1\*q \*qCoreKeyboard\*q"
.B "    InputDevice \*qMouse 1\*q    \*qCorePointer\*q"
.B "    InputDevice \*qMouse 2\*q    \*qSendCoreEvents\*q"
.B "    Option      \*qBlankTime\*q  \*q5\*q"
.B "EndSection"
.fi
.RE
.SH DRI SECTION
This optional section is used to provide some information for the
Direct Rendering Infrastructure.  Details about the format of this section
can be found in the README.DRI document, which is also available on-line
at
.IR <http://www.xfree86.org/current/DRI.html> .
.SH VENDOR SECTION
The optional
.B Vendor
section may be used to provide vendor-specific configuration information.
Multiple
.B Vendor
sections may be present, and they may contain an
.B Identifier
entry and multiple
.B Option
flags.  The data therein is not used in this release.
.PP
.SH FILES
For an example of an XF86Config file, see the file installed as
__projectroot__/lib/X11/XF86Config.eg.
.fi
.SH "SEE ALSO"
X(__miscmansuffix__), Xserver(1), XFree86(1),
apm(__drivermansuffix__),
ati(__drivermansuffix__),
chips(__drivermansuffix__),
cirrus(__drivermansuffix__),
cyrix(__drivermansuffix__),
fbdev(__drivermansuffix__),
glide(__drivermansuffix__),
glint(__drivermansuffix__),
i128(__drivermansuffix__),
i740(__drivermansuffix__),
i810(__drivermansuffix__),
imstt(__drivermansuffix__),
mga(__drivermansuffix__),
neomagic(__drivermansuffix__),
nv(__drivermansuffix__),
r128(__drivermansuffix__),
rendition(__drivermansuffix__),
s3virge(__drivermansuffix__),
siliconmotion(__drivermansuffix__),
sis(__drivermansuffix__),
sunbw2(__drivermansuffix__),
suncg14(__drivermansuffix__),
suncg3(__drivermansuffix__),
suncg6(__drivermansuffix__),
sunffb(__drivermansuffix__),
sunleo(__drivermansuffix__),
suntcx(__drivermansuffix__),
tdfx(__drivermansuffix__),
tga(__drivermansuffix__),
trident(__drivermansuffix__),
tseng(__drivermansuffix__),
v4l(__drivermansuffix__),
vesa(__drivermansuffix__),
vga(__drivermansuffix__),
.br
README
.IR <http://www.xfree86.org/current/README.html> ,
.br
RELNOTES
.IR <http://www.xfree86.org/current/RELNOTES.html> ,
.br
README.mouse
.IR <http://www.xfree86.org/current/mouse.html> ,
.br
README.DRI
.IR <http://www.xfree86.org/current/DRI.html> ,
.br
Status
.IR <http://www.xfree86.org/current/Status.html> ,
.br
Install
.IR <http://www.xfree86.org/current/Install.html> .
.SH AUTHORS
This manual page was largely rewritten for XFree86 4.0 by David Dawes
.IR <dawes@xfree86.org> .
