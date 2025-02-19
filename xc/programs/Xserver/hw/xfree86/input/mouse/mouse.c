/* $XFree86: xc/programs/Xserver/hw/xfree86/input/mouse/mouse.c,v 1.38 2000/12/18 15:52:22 dawes Exp $ */
/*
 *
 * Copyright 1990,91 by Thomas Roell, Dinkelscherben, Germany.
 * Copyright 1993 by David Dawes <dawes@xfree86.org>
 * Copyright 1994-1999 by The XFree86 Project, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the names of copyright holders not be
 * used in advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.  The copyright holders
 * make no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without express or
 * implied warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF
 * CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */
/* Patch for PS/2 Intellimouse - Tim Goodwin 1997-11-06. */

/*
 * [JCH-96/01/21] Added fourth button support for PROT_GLIDEPOINT mouse
 * protocol.
 */

/*
 * [TVO-97/03/05] Added microsoft IntelliMouse support
 */

#define NEED_EVENTS
#include "X.h"
#include "Xproto.h"

#include "xf86.h"

#ifdef XINPUT
#include "XI.h"
#include "XIproto.h"
#include "extnsionst.h"
#include "extinit.h"
#else
#include "inputstr.h"
#endif

#include "xf86Xinput.h"
#include "xf86_OSproc.h"
#include "xf86OSmouse.h"
#define NEED_XF86_TYPES	/* for xisb.h when !XFree86LOADER */
#include "xf86_ansic.h"
#include "compiler.h"

#include "xisb.h"
#include "mouse.h"
#include "mousePriv.h"
#include "mipointer.h"

static InputInfoPtr MousePreInit(InputDriverPtr drv, IDevPtr dev, int flags);
#if 0
static void MouseUnInit(InputDriverPtr drv, InputInfoPtr pInfo, int flags);
#endif

static int MouseProc(DeviceIntPtr device, int what);
static Bool MouseConvert(LocalDevicePtr local, int first, int num, int v0,
		 	     int v1, int v2, int v3, int v4, int v5, int *x,
		 	     int *y);

static void MouseCtrl(DeviceIntPtr device, PtrCtrl *ctrl);
static void MousePostEvent(InputInfoPtr pInfo, int buttons,
			   int dx, int dy, int dz, int dw);
static void MouseReadInput(InputInfoPtr pInfo);

#undef MOUSE
InputDriverRec MOUSE = {
	1,
	"mouse",
	NULL,
	MousePreInit,
	/*MouseUnInit,*/NULL,
	NULL,
	0
};

/*
 * Microsoft (all serial models), Logitech MouseMan, First Mouse, etc,
 * ALPS GlidePoint, Thinking Mouse.
 */
static const char *msDefaults[] = {
	"BaudRate",	"1200",
	"DataBits",	"7",
	"StopBits",	"1",
	"Parity",	"None",
	"FlowControl",	"None",
	"VTime",	"0",
	"VMin",		"1",
	NULL
};
/* MouseSystems */
static const char *mscDefaults[] = {
	"BaudRate",	"1200",
	"DataBits",	"8",
	"StopBits",	"2",
	"Parity",	"None",
	"FlowControl",	"None",
	"VTime",	"0",
	"VMin",		"1",
	NULL
};
/* MMSeries */
static const char *mmDefaults[] = {
	"BaudRate",	"1200",
	"DataBits",	"8",
	"StopBits",	"1",
	"Parity",	"Odd",
	"FlowControl",	"None",
	"VTime",	"0",
	"VMin",		"1",
	NULL
};
/* Logitech series 9 */
static const char *logiDefaults[] = {
	"BaudRate",	"1200",
	"DataBits",	"8",
	"StopBits",	"2",
	"Parity",	"None",
	"FlowControl",	"None",
	"VTime",	"0",
	"VMin",		"1",
	NULL
};
/* Hitachi Tablet */
static const char *mmhitDefaults[] = {
	"BaudRate",	"1200",
	"DataBits",	"8",
	"StopBits",	"1",
	"Parity",	"None",
	"FlowControl",	"None",
	"VTime",	"0",
	"VMin",		"1",
	NULL
};
/* AceCad Tablet */
static const char *acecadDefaults[] = {
	"BaudRate",	"9600",
	"DataBits",	"8",
	"StopBits",	"1",
	"Parity",	"Odd",
	"FlowControl",	"None",
	"VTime",	"0",
	"VMin",		"1",
	NULL
};

static MouseProtocolRec mouseProtocols[] = {

    /* Serial protocols */
    { "Microsoft",		MSE_SERIAL,	msDefaults,	PROT_MS },
    { "MouseSystems",		MSE_SERIAL,	mscDefaults,	PROT_MSC },
    { "MMSeries",		MSE_SERIAL,	mmDefaults,	PROT_MM },
    { "Logitech",		MSE_SERIAL,	logiDefaults,	PROT_LOGI },
    { "MouseMan",		MSE_SERIAL,	msDefaults,	PROT_LOGIMAN },
    { "MMHitTab",		MSE_SERIAL,	mmhitDefaults,	PROT_MMHIT },
    { "GlidePoint",		MSE_SERIAL,	msDefaults,	PROT_GLIDE },
    { "IntelliMouse",		MSE_SERIAL,	msDefaults,	PROT_IMSERIAL },
    { "ThinkingMouse",		MSE_SERIAL,	msDefaults,	PROT_THINKING },
    { "AceCad",			MSE_SERIAL,	acecadDefaults,	PROT_ACECAD },

    /* Standard PS/2 */
    { "PS/2",			MSE_PS2,	NULL,		PROT_PS2 },

    /* Extended PS/2 */
    { "ImPS/2",			MSE_XPS2,	NULL,		PROT_IMPS2 },
    { "ExplorerPS/2",		MSE_XPS2,	NULL,		PROT_EXPPS2 },
    { "ThinkingMousePS/2",	MSE_XPS2,	NULL,		PROT_THINKPS2 },
    { "MouseManPlusPS/2",	MSE_XPS2,	NULL,		PROT_MMPS2 },
    { "GlidePointPS/2",		MSE_XPS2,	NULL,		PROT_GLIDEPS2 },
    { "NetMousePS/2",		MSE_XPS2,	NULL,		PROT_NETPS2 },
    { "NetScrollPS/2",		MSE_XPS2,	NULL,		PROT_NETSCPS2 },

    /* Bus Mouse */
    { "BusMouse",		MSE_BUS,	NULL,		PROT_BM },

    /* Auto-detect (PnP) */
    { "Auto",			MSE_AUTO,	NULL,		PROT_AUTO },

    /* Misc (usually OS-specific) */
    { "SysMouse",		MSE_MISC,	mscDefaults,	PROT_SYSMOUSE },

    /* end of list */
    { NULL,			MSE_NONE,	NULL,		PROT_UNKNOWN }
};

static MouseProtocolID
ProtocolNameToID(const char *name)
{
    int i;

    for (i = 0; mouseProtocols[i].name; i++)
	if (xf86NameCmp(name, mouseProtocols[i].name) == 0)
	    return mouseProtocols[i].id;
    return PROT_UNKNOWN;
}

static const char *
ProtocolIDToName(MouseProtocolID id)
{
    int i;

    switch (id) {
    case PROT_UNKNOWN:
	return "Unknown";
	break;
    case PROT_UNSUP:
	return "Unsupported";
	break;
    default:
	for (i = 0; mouseProtocols[i].name; i++)
	    if (id == mouseProtocols[i].id)
		return mouseProtocols[i].name;
	return "Invalid";
    }
}

const char *
xf86MouseProtocolIDToName(MouseProtocolID id)
{
	return ProtocolIDToName(id);
}

MouseProtocolID
xf86MouseProtocolNameToID(const char *name)
{
    return ProtocolNameToID(name);
}

static int
ProtocolIDToClass(MouseProtocolID id)
{
    int i;

    switch (id) {
    case PROT_UNKNOWN:
    case PROT_UNSUP:
	return MSE_NONE;
	break;
    default:
	for (i = 0; mouseProtocols[i].name; i++)
	    if (id == mouseProtocols[i].id)
		return mouseProtocols[i].class;
	return MSE_NONE;
    }
}

static MouseProtocolPtr
GetProtocol(MouseProtocolID id) {
    int i;

    switch (id) {
    case PROT_UNKNOWN:
    case PROT_UNSUP:
	return NULL;
	break;
    default:
	for (i = 0; mouseProtocols[i].name; i++)
	    if (id == mouseProtocols[i].id)
		return &mouseProtocols[i];
	return NULL;
    }
}

static OSMouseInfoPtr osInfo = NULL;

static Bool
InitProtocols(void)
{
    int classes;
    int i;
    const char *osname = NULL;

    if (osInfo)
	return TRUE;

    osInfo = xf86OSMouseInit(0);
    if (!osInfo)
	return FALSE;
    if (!osInfo->SupportedInterfaces)
	return FALSE;

    classes = osInfo->SupportedInterfaces();
    if (!classes)
	return FALSE;
    
    /* Mark unsupported interface classes. */
    for (i = 0; mouseProtocols[i].name; i++)
	if (!(mouseProtocols[i].class & classes))
	    mouseProtocols[i].id = PROT_UNSUP;

    for (i = 0; mouseProtocols[i].name; i++)
	if (mouseProtocols[i].class & MSE_MISC)
	    if (!osInfo->CheckProtocol ||
		!osInfo->CheckProtocol(mouseProtocols[i].name))
		mouseProtocols[i].id = PROT_UNSUP;

    /* NetBSD uses PROT_BM for "PS/2". */
    xf86GetOS(&osname, NULL, NULL, NULL);
    if (osname && xf86NameCmp(osname, "netbsd") == 0)
	for (i = 0; mouseProtocols[i].name; i++)
	    if (mouseProtocols[i].id == PROT_PS2)
		mouseProtocols[i].id = PROT_BM;

    return TRUE;
}

/* Process options common to all mouse types. */
static void
MouseCommonOptions(InputInfoPtr pInfo)
{
    MouseDevPtr pMse;
    MessageType from = X_DEFAULT;
    char *s;

    pMse = pInfo->private;

    pMse->buttons = xf86SetIntOption(pInfo->options, "Buttons", 0);
    from = X_CONFIG;
    if (!pMse->buttons) {
	pMse->buttons = MSE_DFLTBUTTONS;
	from = X_DEFAULT;
    }
    xf86Msg(from, "%s: Buttons: %d\n", pInfo->name, pMse->buttons);
    
    pMse->emulate3Buttons = xf86SetBoolOption(pInfo->options,
					      "Emulate3Buttons", FALSE);
    pMse->emulate3Timeout = xf86SetIntOption(pInfo->options, "Emulate3Timeout",
					     50);
    if (pMse->emulate3Buttons) {
	xf86Msg(X_CONFIG, "%s: Emulate3Buttons, Emulate3Timeout: %d\n",
		pInfo->name, pMse->emulate3Timeout);
    }

    pMse->chordMiddle = xf86SetBoolOption(pInfo->options, "ChordMiddle", FALSE);
    if (pMse->chordMiddle)
	xf86Msg(X_CONFIG, "%s: ChordMiddle\n", pInfo->name);
    pMse->flipXY = xf86SetBoolOption(pInfo->options,"FlipXY",FALSE);
    if (pMse->flipXY)
	xf86Msg(X_CONFIG, "%s: FlipXY\n",pInfo->name);
    if (xf86SetBoolOption(pInfo->options,"InvX",FALSE)) {
	pMse->invX = -1;
	xf86Msg(X_CONFIG, "%s: InfX\n",pInfo->name);
    }    else
	pMse->invX = 1;
    if (xf86SetBoolOption(pInfo->options,"InvY",FALSE)) {
	pMse->invY = -1;
	xf86Msg(X_CONFIG, "%s: InfY\n",pInfo->name);
    }    else
	pMse->invY = 1;
    
    s = xf86SetStrOption(pInfo->options, "ZAxisMapping", NULL);
    if (s) {
	int b1 = 0, b2 = 0, b3 = 0, b4 = 0;
	char *msg = NULL;

	if (!xf86NameCmp(s, "x")) {
	    pMse->negativeZ = pMse->positiveZ = MSE_MAPTOX;
	    pMse->negativeW = pMse->positiveW = MSE_MAPTOX;
	    msg = xstrdup("X axis");
	} else if (!xf86NameCmp(s, "y")) {
	    pMse->negativeZ = pMse->positiveZ = MSE_MAPTOY;
	    pMse->negativeW = pMse->positiveW = MSE_MAPTOY;
	    msg = xstrdup("Y axis");
	} else if (sscanf(s, "%d %d %d %d", &b1, &b2, &b3, &b4) >= 2 &&
		 b1 > 0 && b1 <= MSE_MAXBUTTONS &&
		 b2 > 0 && b2 <= MSE_MAXBUTTONS) {
	    msg = xstrdup("buttons XX and YY");
	    if (msg)
		sprintf(msg, "buttons %d and %d", b1, b2);
	    pMse->negativeZ = pMse->negativeW = 1 << (b1-1);
	    pMse->positiveZ = pMse->positiveW = 1 << (b2-1);
	    if (b3 > 0 && b3 <= MSE_MAXBUTTONS &&
		b4 > 0 && b4 <= MSE_MAXBUTTONS) {
		if (msg)
		    xfree(msg);
		msg = xstrdup("buttons XX, YY, ZZ and WW");
		if (msg)
		    sprintf(msg, "buttons %d, %d, %d and %d", b1, b2, b3, b4);
		pMse->negativeW = 1 << (b3-1);
		pMse->positiveW = 1 << (b4-1);
	    }
	    if ( b1 > pMse->buttons ) pMse->buttons = b1;
	    if ( b2 > pMse->buttons ) pMse->buttons = b2;
	    if ( b3 > pMse->buttons ) pMse->buttons = b3;
	    if ( b4 > pMse->buttons ) pMse->buttons = b4;
	} else {
	    pMse->negativeZ = pMse->positiveZ = MSE_NOZMAP;
	    pMse->negativeW = pMse->positiveW = MSE_NOZMAP;
	}
	if (msg) {
	    xf86Msg(X_CONFIG, "%s: ZAxisMapping: %s\n", pInfo->name, msg);
	    xfree(msg);
	} else {
	    xf86Msg(X_WARNING, "%s: Invalid ZAxisMapping value: \"%s\"\n",
		    pInfo->name, s);
	}
    }
}

static InputInfoPtr
MousePreInit(InputDriverPtr drv, IDevPtr dev, int flags)
{
    InputInfoPtr pInfo;
    MouseDevPtr pMse;
    MessageType from = X_DEFAULT;
    const char *protocol;
    MouseProtocolID protocolID;
    MouseProtocolPtr pProto;

    if (!InitProtocols())
	return NULL;

    if (!(pInfo = xf86AllocateInput(drv, 0)))
	return NULL;

    /* Initialise the InputInfoRec. */
    pInfo->name = dev->identifier;
    pInfo->type_name = XI_MOUSE;
    pInfo->flags = XI86_POINTER_CAPABLE | XI86_SEND_DRAG_EVENTS;
    pInfo->device_control = MouseProc;
    pInfo->read_input = MouseReadInput;
    pInfo->motion_history_proc = xf86GetMotionEvents;
    pInfo->history_size = 0;
    pInfo->control_proc = NULL;
    pInfo->close_proc = NULL;
    pInfo->switch_mode = NULL;
    pInfo->conversion_proc = MouseConvert;
    pInfo->reverse_conversion_proc = NULL;
    pInfo->fd = -1;
    pInfo->dev = NULL;
    pInfo->private_flags = 0;
    pInfo->always_core_feedback = 0;
    pInfo->conf_idev = dev;

    /* Allocate the MouseDevRec and initialise it. */
    /*
     * XXX This should be done by a function in the core server since the
     * MouseDevRec is defined in the os-support layer.
     */
    if (!(pMse = xcalloc(sizeof(MouseDevRec), 1)))
	return pInfo;
    pInfo->private = pMse;
    pMse->Ctrl = MouseCtrl;
    pMse->PostEvent = MousePostEvent;
    pMse->CommonOptions = MouseCommonOptions;

    /* Find the protocol type. */
    protocol = xf86SetStrOption(dev->commonOptions, "Protocol", NULL);
    if (protocol) {
	from = X_CONFIG;
    } else if (osInfo->DefaultProtocol) {
	protocol = osInfo->DefaultProtocol();
	from = X_DEFAULT;
    }
    if (!protocol) {
	xf86Msg(X_ERROR, "%s: No Protocol specified\n", pInfo->name);
	return pInfo;
    }
    protocolID = ProtocolNameToID(protocol);
    switch (protocolID) {
    case PROT_UNKNOWN:
	/* Check for a builtin OS-specific protocol, and call its PreInit. */
	if (osInfo->CheckProtocol && osInfo->CheckProtocol(protocol)) {
	    if (osInfo->PreInit) {
		osInfo->PreInit(pInfo, protocol, 0);
	    }
	    return pInfo;
	}
	xf86Msg(X_ERROR, "%s: Unknown protocol \"%s\"\n", pInfo->name,
		protocol);
	return pInfo;
	break;
    case PROT_UNSUP:
	xf86Msg(X_ERROR,
		"%s: Protocol \"%s\" is not supported on this platform\n",
		pInfo->name, protocol);
	return pInfo;
	break;
    default:
	xf86Msg(from, "%s: Protocol: \"%s\"\n", pInfo->name, protocol);
    }

    if (!(pProto = GetProtocol(protocolID)))
	return pInfo;

    pMse->protocol = protocol;
    pMse->protocolID = protocolID;
    pMse->oldProtocolID = protocolID;  /* hack */
    pMse->origProtocolID = protocolID;
    pMse->origProtocol = protocol;
    pMse->class = ProtocolIDToClass(protocolID);

    /* Collect the options, and process the common options. */
    xf86CollectInputOptions(pInfo, pProto->defaults, NULL);
    xf86ProcessCommonOptions(pInfo, pInfo->options);

    /* XXX should handle this OS dependency elsewhere. */
#ifndef __OS2ELF__
    /* OS/2 has a mouse handled by the OS - it cannot fail here */

    /* Check if the device can be opened. */
    pInfo->fd = xf86OpenSerial(pInfo->options);
    if (pInfo->fd == -1) {
	if (xf86GetAllowMouseOpenFail())
	    xf86Msg(X_WARNING, "%s: cannot open input device\n", pInfo->name);
	else {
	    xf86Msg(X_ERROR, "%s: cannot open input device\n", pInfo->name);
	    xfree(pMse);
	    pInfo->private = NULL;
	    return pInfo;
	}
    }
    xf86CloseSerial(pInfo->fd);
#endif
    pInfo->fd = -1;

    pMse->CommonOptions(pInfo);

    pMse->sampleRate = xf86SetIntOption(pInfo->options, "SampleRate", 0);
    if (pMse->sampleRate) {
	xf86Msg(X_CONFIG, "%s: SampleRate: %d\n", pInfo->name,
		pMse->sampleRate);
    }
    pMse->baudRate = xf86SetIntOption(pInfo->options, "BaudRate", 0);
    if (pMse->baudRate) {
	xf86Msg(X_CONFIG, "%s: BaudRate: %d\n", pInfo->name,
		pMse->baudRate);
    }
    pMse->resolution = xf86SetIntOption(pInfo->options, "Resolution", 0);
    if (pMse->resolution) {
	xf86Msg(X_CONFIG, "%s: Resolution: %d\n", pInfo->name,
		pMse->resolution);
    }

    pMse->clearDTR = xf86SetBoolOption(pInfo->options, "ClearDTR", FALSE);
    pMse->clearRTS = xf86SetBoolOption(pInfo->options, "ClearRTS", FALSE);
    if (pMse->clearDTR || pMse->clearRTS) {
	xf86Msg(X_CONFIG, "%s: ", pInfo->name);
	if (pMse->clearDTR) {
	    xf86ErrorF("ClearDTR");
	    if (pMse->clearRTS)
		xf86ErrorF(", ");
	}
	if (pMse->clearRTS) {
	    xf86ErrorF("ClearRTS");
	}
	xf86ErrorF("\n");
    }

    pInfo->flags |= XI86_CONFIGURED;
    return pInfo;
}

/*
 * This array is indexed by the MouseProtocolID values, so the order of the entries
 * must match that of the MouseProtocolID enum in mouse.h.
 */
static unsigned char proto[PROT_NUMPROTOS][8] = {
  /* --header--  ---data--- packet -4th-byte-  mouse   */
  /* mask   id   mask   id  bytes  mask   id   flags   */
							    /* Serial mice */
  {  0x40, 0x40, 0x40, 0x00,  3,  ~0x23, 0x00, MPF_NONE },  /* MicroSoft */
  {  0xf8, 0x80, 0x00, 0x00,  5,   0x00, 0xff, MPF_SAFE },  /* MouseSystems */
  {  0xe0, 0x80, 0x80, 0x00,  3,   0x00, 0xff, MPF_NONE },  /* MMSeries */
  {  0xe0, 0x80, 0x80, 0x00,  3,   0x00, 0xff, MPF_NONE },  /* Logitech */
  {  0x40, 0x40, 0x40, 0x00,  3,  ~0x23, 0x00, MPF_NONE },  /* MouseMan */
  {  0xe0, 0x80, 0x80, 0x00,  3,   0x00, 0xff, MPF_NONE },  /* MM_HitTablet */
  {  0x40, 0x40, 0x40, 0x00,  3,  ~0x33, 0x00, MPF_NONE },  /* GlidePoint */
  {  0x40, 0x40, 0x40, 0x00,  3,  ~0x3f, 0x00, MPF_NONE },  /* IntelliMouse */
  {  0x40, 0x40, 0x40, 0x00,  3,  ~0x33, 0x00, MPF_NONE },  /* ThinkingMouse */
  {  0x80, 0x80, 0x80, 0x00,  3,   0x00, 0xff, MPF_NONE },  /* ACECAD */
							    /* PS/2 variants */
  {  0xc0, 0x00, 0x00, 0x00,  3,   0x00, 0xff, MPF_NONE },  /* PS/2 mouse */
  {  0x08, 0x08, 0x00, 0x00,  4,   0x00, 0xff, MPF_NONE },  /* IntelliMouse */
  {  0x08, 0x08, 0x00, 0x00,  4,   0x00, 0xff, MPF_NONE },  /* Explorer */
  {  0x80, 0x80, 0x00, 0x00,  3,   0x00, 0xff, MPF_NONE },  /* ThinkingMouse */
  {  0x08, 0x08, 0x00, 0x00,  3,   0x00, 0xff, MPF_NONE },  /* MouseMan+ */
  {  0xc0, 0x00, 0x00, 0x00,  3,   0x00, 0xff, MPF_NONE },  /* GlidePoint */
  {  0x08, 0x08, 0x00, 0x00,  4,   0x00, 0xff, MPF_NONE },  /* NetMouse */
  {  0xc0, 0x00, 0x00, 0x00,  6,   0x00, 0xff, MPF_NONE },  /* NetScroll */
							    /* Bus Mouse */
  {  0xf8, 0x80, 0x00, 0x00,  5,   0x00, 0xff, MPF_NONE },  /* BusMouse */
  {  0xf8, 0x80, 0x00, 0x00,  5,   0x00, 0xff, MPF_NONE },  /* Auto (dummy) */
  {  0xf8, 0x80, 0x00, 0x00,  8,   0x00, 0xff, MPF_NONE },  /* SysMouse */
};

/*
 * SetupMouse --
 *	Sets up the mouse parameters
 */
static Bool
SetupMouse(InputInfoPtr pInfo)
{
    /*
    ** The following lines take care of the Logitech MouseMan protocols.
    ** The "Logitech" protocol is for the old "series 9" Logitech products.
    ** All products since then use the "MouseMan" protocol.  Some models
    ** were programmable, but most (all?) of the current models are not.
    **
    ** NOTE: There are different versions of both MouseMan and TrackMan!
    **       Hence I add another protocol PROT_LOGIMAN, which the user can
    **       specify as MouseMan in his XF86Config file. This entry was
    **       formerly handled as a special case of PROT_MS. However, people
    **       who don't have the middle button problem, can still specify
    **       Microsoft and use PROT_MS.
    **
    ** By default, these mice should use a 3 byte Microsoft protocol
    ** plus a 4th byte for the middle button. However, the mouse might
    ** have switched to a different protocol before we use it, so I send
    ** the proper sequence just in case.
    **
    ** NOTE: - all commands to (at least the European) MouseMan have to
    **         be sent at 1200 Baud.
    **       - each command starts with a '*'.
    **       - whenever the MouseMan receives a '*', it will switch back
    **	 to 1200 Baud. Hence I have to select the desired protocol
    **	 first, then select the baud rate.
    **
    ** The protocols supported by the (European) MouseMan are:
    **   -  5 byte packed binary protocol, as with the Mouse Systems
    **      mouse. Selected by sequence "*U".
    **   -  2 button 3 byte MicroSoft compatible protocol. Selected
    **      by sequence "*V".
    **   -  3 button 3+1 byte MicroSoft compatible protocol (default).
    **      Selected by sequence "*X".
    **
    ** The following baud rates are supported:
    **   -  1200 Baud (default). Selected by sequence "*n".
    **   -  9600 Baud. Selected by sequence "*q".
    **
    ** Selecting a sample rate is no longer supported with the MouseMan!
    **               [CHRIS-211092]
    */

    MouseDevPtr pMse;
    unsigned char *param;
    int paramlen;
    int i;
    int speed;
    int protoPara[8] = {-1, -1, -1, -1, -1, -1, -1, -1};
    const char *name = NULL;
    const char *s;
    Bool automatic = FALSE;
    unsigned char c;
    pointer options;

    pMse = pInfo->private;
    /* Handle the "Auto" protocol. */
    if (pMse->origProtocolID == PROT_AUTO) {
	MouseProtocolID protocolID = PROT_UNKNOWN;
	
	automatic = TRUE;

	/* Check if the OS has a detection mechanism. */
	if (osInfo->SetupAuto) {
	    name = osInfo->SetupAuto(pInfo, protoPara);
	    if (name) {
		protocolID = ProtocolNameToID(name);
		switch (protocolID) {
		case PROT_UNKNOWN:
		    /* Check for a builtin OS-specific protocol. */
		    if (osInfo->CheckProtocol && osInfo->CheckProtocol(name)) {
			/* XXX need to handle auto-detected builtin protocols */
		    } else
			name = NULL;
		    break;
		case PROT_UNSUP:
		    name = NULL;
		    break;
		default:
		    break;
		}
	    }
	}
#ifdef PNP_MOUSE
	if (!name) {
	    /* A PnP serial mouse? */
	    protocolID = MouseGetPnpProtocol(pInfo);
	    if (protocolID >= 0 && protocolID < PROT_NUMPROTOS) {
		name = ProtocolIDToName(protocolID);
		xf86Msg(X_PROBED, "%s: PnP-detected protocol: \"%s\"\n",
			pInfo->name, name);
	    }
	}
#endif
	if (name) {
	    pMse->protocol = name;
	    pMse->protocolID = protocolID;
	}
    }
    memcpy(pMse->protoPara, proto[pMse->protocolID], sizeof(pMse->protoPara));
    if (automatic) {
	
	if (name) {
	    /* Possible protoPara overrides from SetupAuto. */
	    for (i = 0; i < sizeof(pMse->protoPara); i++)
		if (protoPara[i] != -1)
		    pMse->protoPara[i] = protoPara[i];
	} else {
	    xf86Msg(X_ERROR, "%s: cannot determine the mouse protocol\n",
		    pInfo->name);
	    return FALSE;
	}
    }
    /*
     * If protocol has changed fetch the default options
     * for the new protocol.
     */
    if (pMse->oldProtocolID != pMse->protocolID) {
	pointer tmp = NULL;
	if (mouseProtocols[pMse->protocolID].defaults)
	    tmp = xf86OptionListCreate(
		mouseProtocols[pMse->protocolID].defaults, -1, 0);
	pInfo->options = xf86OptionListMerge(pInfo->options, tmp);
	/* baudrate is not explicitely set: fetch the default one */
	if (!pMse->baudRate)
	    pMse->baudRate = xf86SetIntOption(pInfo->options, "BaudRate", 0);
	pMse->oldProtocolID = pMse->protocolID; /* hack */
    }
    /*
     * Write the baudrate back th the option list so that the serial
     * interface code can access the new value.
     */
    if (pMse->baudRate)
	xf86ReplaceIntOption(pInfo->options, "BaudRate", pMse->baudRate);

    /* Set the port parameters. */
    if (!automatic)
	xf86SetSerial(pInfo->fd, pInfo->options);
    param = NULL;
    paramlen = 0;
    switch (pMse->protocolID) {
    case PROT_LOGI:		/* Logitech Mice */
        /* 
	 * The baud rate selection command must be sent at the current
	 * baud rate; try all likely settings.
	 */
	speed = pMse->baudRate;
	switch (speed) {
	case 9600:
	    s = "*q";
	    break;
	case 4800:
	    s = "*p";
	    break;
	case 2400:
	    s = "*o";
	    break;
	case 1200:
	    s = "*n";
	    break;
	default:
	    /* Fallback value */
	    speed = 1200;
	    s = "*n";
	}
	xf86SetSerialSpeed(pInfo->fd, 9600);
	xf86WriteSerial(pInfo->fd, s, 2);
	usleep(100000);
	xf86SetSerialSpeed(pInfo->fd, 4800);
	xf86WriteSerial(pInfo->fd, s, 2);
	usleep(100000);
	xf86SetSerialSpeed(pInfo->fd, 2400);
	xf86WriteSerial(pInfo->fd, s, 2);
	usleep(100000);
	xf86SetSerialSpeed(pInfo->fd, 1200);
	xf86WriteSerial(pInfo->fd, s, 2);
	usleep(100000);
	xf86SetSerialSpeed(pInfo->fd, speed);

        /* Select MM series data format. */
	xf86WriteSerial(pInfo->fd, "S", 1);
	usleep(100000);
	/* Set the parameters up for the MM series protocol. */
	options = pInfo->options;
	xf86CollectInputOptions(pInfo, mmDefaults, NULL);
	xf86SetSerial(pInfo->fd, pInfo->options);
	pInfo->options = options;

        /* Select report rate/frequency. */
	if      (pMse->sampleRate <=   0)  c = 'O';  /* 100 */
	else if (pMse->sampleRate <=  15)  c = 'J';  /*  10 */
	else if (pMse->sampleRate <=  27)  c = 'K';  /*  20 */
	else if (pMse->sampleRate <=  42)  c = 'L';  /*  35 */
	else if (pMse->sampleRate <=  60)  c = 'R';  /*  50 */
	else if (pMse->sampleRate <=  85)  c = 'M';  /*  67 */
	else if (pMse->sampleRate <= 125)  c = 'Q';  /* 100 */
	else                               c = 'N';  /* 150 */
	xf86WriteSerial(pInfo->fd, &c, 1);
	break;

    case PROT_LOGIMAN:
	speed = pMse->baudRate;
	switch (speed) {
	case 9600:
	    s = "*q";
	    break;
	case 1200:
	    s = "*n";
	    break;
	default:
	    /* Fallback value */
	    speed = 1200;
	    s = "*n";
	}
	xf86SetSerialSpeed(pInfo->fd, 1200);
	xf86WriteSerial(pInfo->fd, "*n", 2);
        xf86WriteSerial(pInfo->fd, "*X", 2);
	xf86WriteSerial(pInfo->fd, s, 2);
	usleep(100000);
	xf86SetSerialSpeed(pInfo->fd, speed);
        break;

    case PROT_MMHIT:		/* MM_HitTablet */
	/*
	 * Initialize Hitachi PUMA Plus - Model 1212E to desired settings.
	 * The tablet must be configured to be in MM mode, NO parity,
	 * Binary Format.  pMse->sampleRate controls the sensitivity
	 * of the tablet.  We only use this tablet for it's 4-button puck
	 * so we don't run in "Absolute Mode".
	 */
	xf86WriteSerial(pInfo->fd, "z8", 2);	/* Set Parity = "NONE" */
	usleep(50000);
	xf86WriteSerial(pInfo->fd, "zb", 2);	/* Set Format = "Binary" */
	usleep(50000);
	xf86WriteSerial(pInfo->fd, "@", 1);	/* Set Report Mode = "Stream" */
	usleep(50000);
	xf86WriteSerial(pInfo->fd, "R", 1);	/* Set Output Rate = "45 rps" */
	usleep(50000);
	xf86WriteSerial(pInfo->fd, "I\x20", 2);	/* Set Incrememtal Mode "20" */
	usleep(50000);
	xf86WriteSerial(pInfo->fd, "E", 1);	/* Set Data Type = "Relative */
	usleep(50000);
	/*
	 * These sample rates translate to 'lines per inch' on the Hitachi
	 * tablet.
	 */
	if      (pMse->sampleRate <=   40) c = 'g';
	else if (pMse->sampleRate <=  100) c = 'd';
	else if (pMse->sampleRate <=  200) c = 'e';
	else if (pMse->sampleRate <=  500) c = 'h';
	else if (pMse->sampleRate <= 1000) c = 'j';
	else                               c = 'd';
	xf86WriteSerial(pInfo->fd, &c, 1);
	usleep(50000);
	xf86WriteSerial(pInfo->fd, "\021", 1);	/* Resume DATA output */
        break;

    case PROT_THINKING:		/* ThinkingMouse */
	/* This mouse may send a PnP ID string, ignore it. */
	usleep(200000);
	xf86FlushInput(pInfo->fd);
	/* Send the command to initialize the beast. */
	for (s = "E5E5"; *s; ++s) {
	    xf86WriteSerial(pInfo->fd, s, 1);
	    if ((xf86WaitForInput(pInfo->fd, 1000000) <= 0))
		break;
	    xf86ReadSerial(pInfo->fd, &c, 1);
	    if (c != *s)
		break;
	}
	break;

    case PROT_MSC:		/* MouseSystems Corp */
	usleep(100000);
	xf86FlushInput(pInfo->fd);
        break;

    case PROT_ACECAD:
	/* initialize */
	/* A nul character resets. */
	xf86WriteSerial(pInfo->fd, "", 1);
	usleep(50000);
	/* Stream out relative mode high resolution increments of 1. */
	xf86WriteSerial(pInfo->fd, "@EeI!", 5);
	break;

    case PROT_BM:		/* bus/InPort mouse */
	if (osInfo->SetBMRes)
	    osInfo->SetBMRes(pInfo, pMse->protocol, pMse->sampleRate,
			     pMse->resolution);
	break;

    case PROT_IMPS2:		/* IntelliMouse */
	{
	    static unsigned char s[] = { 243, 200, 243, 100, 243, 80, };

	    param = s;
	    paramlen = sizeof(s);
	}
	break;

    case PROT_EXPPS2:		/* IntelliMouse Explorer */
	{
	    static unsigned char s[] = { 243, 200, 243, 200, 243, 80, };

	    param = s;
	    paramlen = sizeof(s);
	}
	break;

    case PROT_NETPS2:		/* NetMouse, NetMouse Pro, Mie Mouse */
    case PROT_NETSCPS2:		/* NetScroll */
	{
	    static unsigned char s[] = { 232, 3, 230, 230, 230, };

	    param = s;
	    paramlen = sizeof(s);
	}
	break;

    case PROT_MMPS2:		/* MouseMan+, FirstMouse+ */
	{
	    static unsigned char s[] = { 230, 232, 0, 232, 3, 232, 2, 232, 1,
					 230, 232, 3, 232, 1, 232, 2, 232, 3, };
	    param = s;
	    paramlen = sizeof(s);
	}
	break;

    case PROT_THINKPS2:		/* ThinkingMouse */
	{
	    static unsigned char s[] = { 243, 10, 232,  0, 243, 20, 243, 60,
					 243, 40, 243, 20, 243, 20, 243, 60,
					 243, 40, 243, 20, 243, 20, };
	    param = s;
	    paramlen = sizeof(s);
	}

    case PROT_SYSMOUSE:
	if (osInfo->SetMiscRes)
	    osInfo->SetMiscRes(pInfo, pMse->protocol, pMse->sampleRate,
			       pMse->resolution);
	break;

    default:
	/* Nothing to do. */
	break;
    }

    if (paramlen > 0) {
#ifdef EXTMOUSEDEBUG
	for (i = 0; i < paramlen; ++i) {
	    if (xf86WriteSerial(pInfo->fd, &param[i], 1) != 1)
		ErrorF("SetupMouse: Write to mouse failed (%s)\n",
		       strerror(errno));
	    usleep(30000);
	    xf86ReadSerial(pInfo->fd, &c, 1);
	    ErrorF("SetupMouse: got %02x\n", c);
	}
#else
	if (xf86WriteSerial(pInfo->fd, param, paramlen) != paramlen)
	    xf86Msg(X_ERROR, "%s: Write to mouse failed\n", pInfo->name);
#endif
 	usleep(30000);
 	xf86FlushInput(pInfo->fd);
    }
    if (pMse->class & (MSE_PS2 | MSE_XPS2)) {
	if (osInfo->SetPS2Res) {
	    osInfo->SetPS2Res(pInfo, pMse->protocol, pMse->sampleRate,
			      pMse->resolution);
	} else {
	    unsigned char c2[2];

	    c = 230;		/* 1:1 scaling */
	    xf86WriteSerial(pInfo->fd, &c, 1);
	    c = 244;		/* enable mouse */
	    xf86WriteSerial(pInfo->fd, &c, 1);
	    c2[0] = 243;	/* set sampling rate */
	    if (pMse->sampleRate > 0) {
 		if (pMse->sampleRate >= 200)
 		    c2[1] = 200;
 		else if (pMse->sampleRate >= 100)
 		    c2[1] = 100;
  		else if (pMse->sampleRate >= 80)
 		    c2[1] = 80;
		else if (pMse->sampleRate >= 60)
 		    c2[1] = 60;
 		else if (pMse->sampleRate >= 40)
 		    c2[1] = 40;
 		else
 		    c2[1] = 20;
	    } else {
 		c2[1] = 100;
	    }
	    xf86WriteSerial(pInfo->fd, c2, 2);
	    c2[0] = 232;	/* set device resolution */
	    if (pMse->resolution > 0) {
		if (pMse->resolution >= 200)
		    c2[1] = 3;
		else if (pMse->resolution >= 100)
		    c2[1] = 2;
		else if (pMse->resolution >= 50)
		    c2[1] = 1;
		else
		    c2[1] = 0;
	    } else {
		c2[1] = 2;
	    }
	    xf86WriteSerial(pInfo->fd, c2, 2);
	    usleep(30000);
	    xf86FlushInput(pInfo->fd);
	}
    }

    pMse->protoBufTail = 0;
    pMse->inSync = 0;

    return TRUE;
}
 
static void
MouseReadInput(InputInfoPtr pInfo)
{
    MouseDevPtr pMse;
    int j, buttons, dx, dy, dz, dw, baddata;
    int pBufP;
    int c;
    unsigned char *pBuf, u;

    pMse = pInfo->private;
    pBufP = pMse->protoBufTail;
    pBuf = pMse->protoBuf;

    /*
     * Set blocking to -1 on the first call because we know there is data to
     * read. Xisb automatically clears it after one successful read so that
     * succeeding reads are preceeded by a select with a 0 timeout to prevent
     * read from blocking indefinitely.
     */
    XisbBlockDuration(pMse->buffer, -1);

    while ((c = XisbRead(pMse->buffer)) >= 0) {
	u = (unsigned char)c;
	if (pBufP >= pMse->protoPara[4]) {
	    /*
	     * Buffer contains a full packet, which has already been processed:
	     * Empty the buffer and check for optional 4th byte, which will be
	     * processed directly, without being put into the buffer first.
	     */
	    pBufP = 0;

	    if ((u & pMse->protoPara[0]) != pMse->protoPara[1] &&
		(u & pMse->protoPara[5]) == pMse->protoPara[6]) {
		/*
		 * Hack for Logitech MouseMan Mouse - Middle button
		 *
		 * Unfortunately this mouse has variable length packets: the
		 * standard Microsoft 3 byte packet plus an optional 4th byte
		 * whenever the middle button status changes.
		 *
		 * We have already processed the standard packet with the
		 * movement and button info.  Now post an event message with
		 * the old status of the left and right buttons and the
		 * updated middle button.
		 */
		/*
		 * Even worse, different MouseMen and TrackMen differ in the
		 * 4th byte: some will send 0x00/0x20, others 0x01/0x21, or
		 * even 0x02/0x22, so I have to strip off the lower bits.
		 * [CHRIS-211092]
		 *
		 * [JCH-96/01/21]
		 * HACK for ALPS "fourth button".  (It's bit 0x10 of the
		 * "fourth byte" and it is activated by tapping the glidepad
		 * with the finger! 8^) We map it to bit bit3, and the
		 * reverse map in xf86Events just has to be extended so that
		 * it is identified as Button 4.  The lower half of the
		 * reverse-map may remain unchanged.
		 */
		/*
		 * [KAZU-030897]
		 * Receive the fourth byte only when preceeding three bytes
		 * have been detected (pBufP >= pMse->protoPara[4]).  In the
		 * previous versions, the test was pBufP == 0; we may have
		 * mistakingly received a byte even if we didn't see anything
		 * preceeding the byte.
		 */

#ifdef EXTMOUSEDEBUG
		ErrorF("mouse 4th byte %02x",u);
#endif
		dx = dy = dz = dw = 0;
		buttons = 0;
		switch (pMse->protocolID) {

		/*
		 * [KAZU-221197]
		 * IntelliMouse, NetMouse (including NetMouse Pro) and Mie
		 * Mouse always send the fourth byte, whereas the fourth byte
		 * is optional for GlidePoint and ThinkingMouse.  The fourth
		 * byte is also optional for MouseMan+ and FirstMouse+ in
		 * their native mode.  It is always sent if they are in the
		 * IntelliMouse compatible mode.
		 */ 
		case PROT_IMSERIAL:	/* IntelliMouse, NetMouse, Mie Mouse, 
					   MouseMan+ */
		    dz = (u & 0x08) ?
				(u & 0x0f) - 16 : (u & 0x0f);
		    if ((dz >= 7) || (dz <= -7))
			dz = 0;
		    buttons |=  ((int)(u & 0x10) >> 3)
			      | ((int)(u & 0x20) >> 2) 
			      | (pMse->lastButtons & 0x05);
		    break;

		case PROT_GLIDE:
		case PROT_THINKING:
		    buttons |= ((int)(u & 0x10) >> 1);
		    /* fall through */

		default:
		    buttons |= ((int)(u & 0x20) >> 4) |
			       (pMse->lastButtons & 0x05);
		    break;
		}
		goto post_event;
	    }
	}
	/* End of packet buffer flush and 4th byte hack. */

	/*
	 * Append next byte to buffer (which is empty or contains an
	 * incomplete packet); iterate if packet (still) not complete.
	 */
	pBuf[pBufP++] = u;
	if (pBufP != pMse->protoPara[4]) continue;
#ifdef EXTMOUSEDEBUG2
	{
	    int i;
	    ErrorF("received %d bytes",pBufP);
	    for ( i=0; i < pBufP; i++)
		ErrorF(" %02x",pBuf[i]);
	    ErrorF("\n");
	}
#endif

	/*
	 * Hack for resyncing: We check here for a package that is:
	 *  a) illegal (detected by wrong data-package header)
	 *  b) invalid (0x80 == -128 and that might be wrong for MouseSystems)
	 *  c) bad header-package
	 *
	 * NOTE: b) is a violation of the MouseSystems-Protocol, since values
	 *       of -128 are allowed, but since they are very seldom we can
	 *       easily  use them as package-header with no button pressed.
	 * NOTE/2: On a PS/2 mouse any byte is valid as a data byte.
	 *       Furthermore, 0x80 is not valid as a header byte. For a PS/2
	 *       mouse we skip checking data bytes.  For resyncing a PS/2
	 *       mouse we require the two most significant bits in the header
	 *       byte to be 0. These are the overflow bits, and in case of
	 *       an overflow we actually lose sync. Overflows are very rare,
	 *       however, and we quickly gain sync again after an overflow
	 *       condition. This is the best we can do. (Actually, we could
	 *       use bit 0x08 in the header byte for resyncing, since that
	 *       bit is supposed to be always on, but nobody told Microsoft...)
	 */
	
	/*
	 * [KAZU,OYVIND-120398]
	 * The above hack is wrong!  Because of b) above, we shall see
	 * erroneous mouse events so often when the MouseSystem mouse is
	 * moved quickly.  As for the PS/2 and its variants, we don't need 
	 * to treat them as special cases, because protoPara[2] and 
	 * protoPara[3] are both 0x00 for them, thus, any data bytes will 
	 * never be discarded.  0x80 is rejected for MMSeries, Logitech 
	 * and MMHittab protocols, because protoPara[2] and protoPara[3] 
	 * are 0x80 and 0x00 respectively.  The other protocols are 7-bit 
	 * protocols; there is no use checking 0x80.  
	 * 
	 * All in all we should check the condition a) only.
	 */

	/*
	 * [OYVIND-120498]
	 * Check packet for valid data:
	 * If driver is in sync with datastream, the packet is considered
	 * bad if any byte (header and/or data) contains an invalid value.
	 * 
	 * If packet is bad, we discard the first byte and shift the buffer.
	 * Next iteration will then check the new situation for validity.
	 * 
	 * If flag MF_SAFE is set in proto[7] and the driver
	 * is out of sync, the packet is also considered bad if
	 * any of the data bytes contains a valid header byte value.
	 * This situation could occur if the buffer contains
	 * the tail of one packet and the header of the next.
	 *
	 * Note: The driver starts in out-of-sync mode (pMse->inSync = 0).
	 */

	baddata = 0;

	/* All databytes must be valid. */
	for (j = 1; j < pBufP; j++ )
	    if ((pBuf[j] & pMse->protoPara[2]) != pMse->protoPara[3])
		baddata = 1;

	/* If out of sync, don't mistake a header byte for data. */
	if ((pMse->protoPara[7] & MPF_SAFE) && !pMse->inSync)
	    for (j = 1; j < pBufP; j++ )
		if ((pBuf[j] & pMse->protoPara[0]) == pMse->protoPara[1])
		    baddata = 1;

	/* Accept or reject the packet ? */
	if ((pBuf[0] & pMse->protoPara[0]) != pMse->protoPara[1] || baddata) {
#ifdef EXTMOUSEDEBUG
	    if (pMse->inSync)
		ErrorF("mouse driver lost sync\n");
	    ErrorF("skipping byte %02x\n",*pBuf);
#endif
	    pMse->protoBufTail = --pBufP;
	    for (j = 0; j < pBufP; j++)
		pBuf[j] = pBuf[j+1];
	    pMse->inSync = 0;
	    continue;
	}

	if (!pMse->inSync) {
#ifdef EXTMOUSEDEBUG
	    ErrorF("mouse driver back in sync\n");
#endif
	    pMse->inSync = 1;
	}

	/*
	 * Packet complete and verified, now process it ...
	 */

	dz = dw = 0;
	switch (pMse->protocolID) {
	case PROT_LOGIMAN:	/* MouseMan / TrackMan   [CHRIS-211092] */
	case PROT_MS:		/* Microsoft */
	    if (pMse->chordMiddle)
		buttons = (((int) pBuf[0] & 0x30) == 0x30) ? 2 :
				  ((int)(pBuf[0] & 0x20) >> 3)
				| ((int)(pBuf[0] & 0x10) >> 4);
	    else
        	buttons = (pMse->lastButtons & 2)
			| ((int)(pBuf[0] & 0x20) >> 3)
			| ((int)(pBuf[0] & 0x10) >> 4);
	    dx = (char)(((pBuf[0] & 0x03) << 6) | (pBuf[1] & 0x3F));
	    dy = (char)(((pBuf[0] & 0x0C) << 4) | (pBuf[2] & 0x3F));
	    break;

	case PROT_GLIDE:	/* ALPS GlidePoint */
	case PROT_THINKING:	/* ThinkingMouse */
	case PROT_IMSERIAL:	/* IntelliMouse, NetMouse, Mie Mouse, MouseMan+ */
	    buttons =  (pMse->lastButtons & (8 + 2))
		     | ((int)(pBuf[0] & 0x20) >> 3)
		     | ((int)(pBuf[0] & 0x10) >> 4);
	    dx = (char)(((pBuf[0] & 0x03) << 6) | (pBuf[1] & 0x3F));
	    dy = (char)(((pBuf[0] & 0x0C) << 4) | (pBuf[2] & 0x3F));
	    break;

	case PROT_MSC:		/* Mouse Systems Corp */
	    buttons = (~pBuf[0]) & 0x07;
	    dx =    (char)(pBuf[1]) + (char)(pBuf[3]);
	    dy = - ((char)(pBuf[2]) + (char)(pBuf[4]));
	    break;
      
	case PROT_MMHIT:	/* MM_HitTablet */
	    buttons = pBuf[0] & 0x07;
	    if (buttons != 0)
		buttons = 1 << (buttons - 1);
	    dx = (pBuf[0] & 0x10) ?   pBuf[1] : - pBuf[1];
	    dy = (pBuf[0] & 0x08) ? - pBuf[2] :   pBuf[2];
	    break;

	case PROT_ACECAD:	/* ACECAD */
	    /* ACECAD is almost exactly like MM but the buttons are different */
	    buttons = (pBuf[0] & 0x02) | ((pBuf[0] & 0x04) >> 2) |
		      ((pBuf[0] & 1) << 2);
	    dx = (pBuf[0] & 0x10) ?   pBuf[1] : - pBuf[1];
	    dy = (pBuf[0] & 0x08) ? - pBuf[2] :   pBuf[2];
	    break;

	case PROT_MM:		/* MM Series */
	case PROT_LOGI:		/* Logitech Mice */
	    buttons = pBuf[0] & 0x07;
	    dx = (pBuf[0] & 0x10) ?   pBuf[1] : - pBuf[1];
	    dy = (pBuf[0] & 0x08) ? - pBuf[2] :   pBuf[2];
	    break;

	case PROT_BM:		/* BusMouse */
	    buttons = (~pBuf[0]) & 0x07;
	    dx =   (char)pBuf[1];
	    dy = - (char)pBuf[2];
	    break;

	case PROT_PS2:		/* PS/2 mouse */
	    buttons = (pBuf[0] & 0x04) >> 1 |       /* Middle */
		      (pBuf[0] & 0x02) >> 1 |       /* Right */
		      (pBuf[0] & 0x01) << 2;        /* Left */
	    dx = (pBuf[0] & 0x10) ?    (int)pBuf[1]-256  :  (int)pBuf[1];
	    dy = (pBuf[0] & 0x20) ?  -((int)pBuf[2]-256) : -(int)pBuf[2];
	    break;

	/* PS/2 mouse variants */
	case PROT_IMPS2:	/* IntelliMouse PS/2 */
	case PROT_NETPS2:	/* NetMouse PS/2 */
	    buttons = (pBuf[0] & 0x04) >> 1 |       /* Middle */
		      (pBuf[0] & 0x02) >> 1 |       /* Right */
		      (pBuf[0] & 0x01) << 2 |       /* Left */
		      (pBuf[0] & 0x40) >> 3 |       /* button 4 */
		      (pBuf[0] & 0x80) >> 3;        /* button 5 */
	    dx = (pBuf[0] & 0x10) ?    pBuf[1]-256  :  pBuf[1];
	    dy = (pBuf[0] & 0x20) ?  -(pBuf[2]-256) : -pBuf[2];
	    dz = (char)pBuf[3];
	    if ((dz >= 7) || (dz <= -7))
		dz = 0;
	    break;

	case PROT_EXPPS2:	/* IntelliMouse Explorer PS/2 */
	    buttons = (pBuf[0] & 0x04) >> 1 |       /* Middle */
		      (pBuf[0] & 0x02) >> 1 |       /* Right */
		      (pBuf[0] & 0x01) << 2 |       /* Left */
		      (pBuf[3] & 0x10) >> 1 |       /* button 4 */
		      (pBuf[3] & 0x20) >> 1;        /* button 5 */
	    dx = (pBuf[0] & 0x10) ?    pBuf[1]-256  :  pBuf[1];
	    dy = (pBuf[0] & 0x20) ?  -(pBuf[2]-256) : -pBuf[2];
	    dz = (pBuf[3] & 0x08) ? (pBuf[3] & 0x0f) - 16 : (pBuf[3] & 0x0f);
	    break;

	case PROT_MMPS2:	/* MouseMan+ PS/2 */
	    buttons = (pBuf[0] & 0x04) >> 1 |       /* Middle */
		      (pBuf[0] & 0x02) >> 1 |       /* Right */
		      (pBuf[0] & 0x01) << 2;        /* Left */
	    dx = (pBuf[0] & 0x10) ? pBuf[1] - 256 : pBuf[1];
	    if (((pBuf[0] & 0x48) == 0x48) &&
		(abs(dx) > 191) &&
		((((pBuf[2] & 0x03) << 2) | 0x02) == (pBuf[1] & 0x0f))) {
		/* extended data packet */
		switch ((((pBuf[0] & 0x30) >> 2) | ((pBuf[1] & 0x30) >> 4))) {
		case 1:		/* wheel data packet */
		    buttons |= ((pBuf[2] & 0x10) ? 0x08 : 0) | /* 4th button */
		               ((pBuf[2] & 0x20) ? 0x10 : 0);  /* 5th button */
		    dx = dy = 0;
		    dz = (pBuf[2] & 0x08) ? (pBuf[2] & 0x0f) - 16 :
					    (pBuf[2] & 0x0f);
		    break;
		case 2:		/* Logitech reserves this packet type */
		    /* 
		     * IBM ScrollPoint uses this packet to encode its
		     * stick movement.
		     */
		    buttons |= (pMse->lastButtons & ~0x07);
		    dx = dy = 0;
		    dz = (pBuf[2] & 0x80) ? ((pBuf[2] >> 4) & 0x0f) - 16 :
					    ((pBuf[2] >> 4) & 0x0f);
		    dw = (pBuf[2] & 0x08) ? (pBuf[2] & 0x0f) - 16 :
					    (pBuf[2] & 0x0f);
		    break;
		case 0:		/* device type packet - shouldn't happen */
		default:
		    buttons |= (pMse->lastButtons & ~0x07);
		    dx = dy = 0;
		    dz = 0;
		    break;
		}
	    } else {
		buttons |= (pMse->lastButtons & ~0x07);
		dx = (pBuf[0] & 0x10) ?    pBuf[1]-256  :  pBuf[1];
		dy = (pBuf[0] & 0x20) ?  -(pBuf[2]-256) : -pBuf[2];
	    }
	    break;

	case PROT_GLIDEPS2:	/* GlidePoint PS/2 */
	    buttons = (pBuf[0] & 0x04) >> 1 |       /* Middle */
		      (pBuf[0] & 0x02) >> 1 |       /* Right */
		      (pBuf[0] & 0x01) << 2 |       /* Left */
		      ((pBuf[0] & 0x08) ? 0 : 0x08);/* fourth button */
	    dx = (pBuf[0] & 0x10) ?    pBuf[1]-256  :  pBuf[1];
	    dy = (pBuf[0] & 0x20) ?  -(pBuf[2]-256) : -pBuf[2];
	    break;

	case PROT_NETSCPS2:	/* NetScroll PS/2 */
	    buttons = (pBuf[0] & 0x04) >> 1 |       /* Middle */
		      (pBuf[0] & 0x02) >> 1 |       /* Right */
		      (pBuf[0] & 0x01) << 2 |       /* Left */
		      ((pBuf[3] & 0x02) ? 0x08 : 0) | /* button 4 */
		      ((pBuf[3] & 0x01) ? 0x10 : 0);  /* button 5 */
	    dx = (pBuf[0] & 0x10) ?    pBuf[1]-256  :  pBuf[1];
	    dy = (pBuf[0] & 0x20) ?  -(pBuf[2]-256) : -pBuf[2];
	    dz = (pBuf[3] & 0x10) ? pBuf[4] - 256 : pBuf[4];
	    break;

	case PROT_THINKPS2:	/* ThinkingMouse PS/2 */
	    buttons = (pBuf[0] & 0x04) >> 1 |       /* Middle */
		      (pBuf[0] & 0x02) >> 1 |       /* Right */
		      (pBuf[0] & 0x01) << 2 |       /* Left */
		      ((pBuf[0] & 0x08) ? 0x08 : 0);/* fourth button */
	    pBuf[1] |= (pBuf[0] & 0x40) ? 0x80 : 0x00;
	    dx = (pBuf[0] & 0x10) ?    pBuf[1]      :  pBuf[1];
	    dy = (pBuf[0] & 0x20) ?  -(pBuf[2]-256) : -pBuf[2];
	    break;

	case PROT_SYSMOUSE:	/* sysmouse */
	    buttons = (~pBuf[0]) & 0x07;
	    dx =    (char)(pBuf[1]) + (char)(pBuf[3]);
	    dy = - ((char)(pBuf[2]) + (char)(pBuf[4]));
	    /* FreeBSD sysmouse sends additional data bytes */
	    if (pMse->protoPara[4] >= 8) {
		dz = ((char)(pBuf[5] << 1) + (char)(pBuf[6] << 1)) / 2;
		buttons |= (int)(~pBuf[7] & 0x07) << 3;
	    }
	    break;

	default: /* There's a table error */
#ifdef EXTMOUSEDEBUG
	    ErrorF("mouse table error\n");
#endif
	    continue;
	}

#ifdef EXTMOUSEDEBUG
	ErrorF("packet");
	for ( j=0; j < pBufP; j++)
	    ErrorF(" %02x",pBuf[j]);
#endif

post_event:
	/* post an event */
	pMse->PostEvent(pInfo, buttons, dx, dy, dz, dw);

	/* 
	 * We don't reset pBufP here yet, as there may be an additional data
	 * byte in some protocols. See above.
	 */
    }
    pMse->protoBufTail = pBufP;
}

/*
 * MouseCtrl --
 *      Alter the control parameters for the mouse. Note that all special
 *      protocol values are handled by dix.
 */

static void
MouseCtrl(DeviceIntPtr device, PtrCtrl *ctrl)
{
    InputInfoPtr pInfo;
    MouseDevPtr pMse;

    pInfo = device->public.devicePrivate;
    pMse = pInfo->private;

#ifdef EXTMOUSEDEBUG
    ErrorF("MouseCtrl pMse=%p\n", pMse);
#endif
    
    pMse->num       = ctrl->num;
    pMse->den       = ctrl->den;
    pMse->threshold = ctrl->threshold;
}

/*
 ***************************************************************************
 *
 * MouseProc --
 *
 ***************************************************************************
 */
static int
MouseProc(DeviceIntPtr device, int what)
{
    InputInfoPtr pInfo;
    MouseDevPtr pMse;
    unsigned char map[MSE_MAXBUTTONS + 1];
    int i, blocked;

    pInfo = device->public.devicePrivate;
    pMse = pInfo->private;
    pMse->device = device;
    
    switch (what)
    {
    case DEVICE_INIT:
	device->public.on = FALSE;
	/*
	 * [KAZU-241097] We don't know exactly how many buttons the
	 * device has, so setup the map with the maximum number.
	 */
	for (i = 0; i < MSE_MAXBUTTONS; i++)
	    map[i + 1] = i + 1;

	InitPointerDeviceStruct((DevicePtr)device, map,
				min(pMse->buttons, MSE_MAXBUTTONS),
				miPointerGetMotionEvents, pMse->Ctrl,
				miPointerGetMotionBufferSize());

	/* X valuator */
	xf86InitValuatorAxisStruct(device, 0, 0, -1, 1, 0, 1);
	xf86InitValuatorDefaults(device, 0);
	/* Y valuator */
	xf86InitValuatorAxisStruct(device, 1, 0, -1, 1, 0, 1);
	xf86InitValuatorDefaults(device, 1);
	xf86MotionHistoryAllocate(pInfo);

#ifdef EXTMOUSEDEBUG
	ErrorF("assigning %p atom=%d name=%s\n", device, pInfo->atom,
		pInfo->name);
#endif
	break;

    case DEVICE_ON:
	pInfo->fd = xf86OpenSerial(pInfo->options);
	if (pInfo->fd == -1)
	    xf86Msg(X_WARNING, "%s: cannot open input device\n", pInfo->name);
	else {
	    pMse->buffer = XisbNew(pInfo->fd, 64);
	    if (!pMse->buffer) {
		xf86CloseSerial(pInfo->fd);
		pInfo->fd = -1;
	    } else {
		if (!SetupMouse(pInfo)) {
		    xf86CloseSerial(pInfo->fd);
		    pInfo->fd = -1;
		    XisbFree(pMse->buffer);
		    pMse->buffer = NULL;
		} else {
		    xf86FlushInput(pInfo->fd);
		    if (pMse->protocolID == PROT_PS2)
			xf86WriteSerial(pInfo->fd, "\364", 1);
		    xf86AddEnabledDevice(pInfo);
		}
	    }
	}
	pMse->lastButtons = 0;
	pMse->emulateState = 0;
	device->public.on = TRUE;
	/*
	 * send button up events for sanity. If no button down is pending
	 * xf86PostButtonEvent() will discard them. So we are on the safe side.
	 */
	blocked = xf86BlockSIGIO ();
	for (i = 1; i <= 5; i++)
	    xf86PostButtonEvent(device,0,i,0,0,0);
	xf86UnblockSIGIO (blocked);
	break;
	    
    case DEVICE_OFF:
    case DEVICE_CLOSE:
	if (pInfo->fd != -1) {
	    xf86RemoveEnabledDevice(pInfo);
	    if (pMse->buffer) {
		XisbFree(pMse->buffer);
		pMse->buffer = NULL;
	    }
	    xf86CloseSerial(pInfo->fd);
	    pInfo->fd = -1;
	}
	device->public.on = FALSE;
	usleep(300000);
	break;
    }
    return Success;
}

/*
 ***************************************************************************
 *
 * MouseConvert --
 *	Convert valuators to X and Y.
 *
 ***************************************************************************
 */
static Bool
MouseConvert(InputInfoPtr pInfo, int first, int num, int v0, int v1, int v2,
	     int v3, int v4, int v5, int *x, int *y)
{
    if (first != 0 || num != 2)
	return FALSE;

    *x = v0;
    *y = v1;

    return TRUE;
}

static CARD32
buttonTimer(OsTimerPtr timer, CARD32 now, pointer arg)
{
    InputInfoPtr pInfo;
    MouseDevPtr pMse;
    int	sigstate;

    pInfo = arg;
    pMse = pInfo->private;

    sigstate = xf86BlockSIGIO ();
    pMse->PostEvent(pInfo, pMse->truebuttons, 0, 0, 0, 0);
    xf86UnblockSIGIO (sigstate);
    return 0;
}

/*
 * Lets create a simple finite-state machine:
 *
 *   state[?][0]: action1
 *   state[?][1]: action2
 *   state[?][2]: next state
 *
 *   action > 0: ButtonPress
 *   action = 0: nothing
 *   action < 0: ButtonRelease
 *
 * Why this stuff ??? Normally you cannot press both mousebuttons together, so
 * the mouse reports both pressed at the same time ...
 */

static signed char stateTab[48][3] = {

/* nothing pressed */
  {  0,  0,  0 },	
  {  0,  0,  8 },	/* 1 right -> delayed right */
  {  0,  0,  0 },       /* 2 nothing */
  {  0,  0,  8 },	/* 3 right -> delayed right */
  {  0,  0, 16 },	/* 4 left -> delayed left */
  {  2,  0, 24 },       /* 5 left & right (middle press) -> middle pressed */
  {  0,  0, 16 },	/* 6 left -> delayed left */
  {  2,  0, 24 },       /* 7 left & right (middle press) -> middle pressed */

/* delayed right */
  {  1, -1,  0 },	/* 8 nothing (right event) -> init */
  {  1,  0, 32 },       /* 9 right (right press) -> right pressed */
  {  1, -1,  0 },	/* 10 nothing (right event) -> init */
  {  1,  0, 32 },       /* 11 right (right press) -> right pressed */
  {  1, -1, 16 },       /* 12 left (right event) -> delayed left */
  {  2,  0, 24 },       /* 13 left & right (middle press) -> middle pressed */
  {  1, -1, 16 },       /* 14 left (right event) -> delayed left */
  {  2,  0, 24 },       /* 15 left & right (middle press) -> middle pressed */

/* delayed left */
  {  3, -3,  0 },	/* 16 nothing (left event) -> init */
  {  3, -3,  8 },       /* 17 right (left event) -> delayed right */
  {  3, -3,  0 },	/* 18 nothing (left event) -> init */
  {  3, -3,  8 },       /* 19 right (left event) -> delayed right */
  {  3,  0, 40 },	/* 20 left (left press) -> pressed left */
  {  2,  0, 24 },	/* 21 left & right (middle press) -> pressed middle */
  {  3,  0, 40 },	/* 22 left (left press) -> pressed left */
  {  2,  0, 24 },	/* 23 left & right (middle press) -> pressed middle */

/* pressed middle */
  { -2,  0,  0 },	/* 24 nothing (middle release) -> init */
  { -2,  0,  0 },	/* 25 right (middle release) -> init */
  { -2,  0,  0 },	/* 26 nothing (middle release) -> init */
  { -2,  0,  0 },	/* 27 right (middle release) -> init */
  { -2,  0,  0 },	/* 28 left (middle release) -> init */
  {  0,  0, 24 },	/* 29 left & right -> pressed middle */
  { -2,  0,  0 },	/* 30 left (middle release) -> init */
  {  0,  0, 24 },	/* 31 left & right -> pressed middle */

/* pressed right */
  { -1,  0,  0 },	/* 32 nothing (right release) -> init */
  {  0,  0, 32 },	/* 33 right -> pressed right */
  { -1,  0,  0 },	/* 34 nothing (right release) -> init */
  {  0,  0, 32 },	/* 35 right -> pressed right */
  { -1,  0, 16 },	/* 36 left (right release) -> delayed left */
  { -1,  2, 24 },	/* 37 left & right (r rel, m prs) -> middle pressed */
  { -1,  0, 16 },	/* 38 left (right release) -> delayed left */
  { -1,  2, 24 },	/* 39 left & right (r rel, m prs) -> middle pressed */

/* pressed left */
  { -3,  0,  0 },	/* 40 nothing (left release) -> init */
  { -3,  0,  8 },	/* 41 right (left release) -> delayed right */
  { -3,  0,  0 },	/* 42 nothing (left release) -> init */
  { -3,  0,  8 },	/* 43 right (left release) -> delayed right */
  {  0,  0, 40 },	/* 44 left -> left pressed */
  { -3,  2, 24 },	/* 45 left & right (l rel, mprs) -> middle pressed */
  {  0,  0, 40 },	/* 46 left -> left pressed */
  { -3,  2, 24 },	/* 47 left & right (l rel, mprs) -> middle pressed */
};


/*
 * Table to allow quick reversal of natural button mapping to correct mapping
 */

/*
 * [JCH-96/01/21] The ALPS GlidePoint pad extends the MS protocol
 * with a fourth button activated by tapping the PAD.
 * The 2nd line corresponds to 4th button on; the drv sends
 * the buttons in the following map (MSBit described first) :
 * 0 | 4th | 1st | 2nd | 3rd
 * And we remap them (MSBit described first) :
 * 0 | 4th | 3rd | 2nd | 1st
 */
static char reverseMap[32] = { 0,  4,  2,  6,  1,  5,  3,  7,
			       8, 12, 10, 14,  9, 13, 11, 15,
			      16, 20, 18, 22, 17, 21, 19, 23,
			      24, 28, 26, 30, 25, 29, 27, 31};


static char hitachMap[16] = {  0,  2,  1,  3, 
			       8, 10,  9, 11,
			       4,  6,  5,  7,
			      12, 14, 13, 15 };

#define reverseBits(map, b)	(((b) & ~0x0f) | map[(b) & 0x0f])

static void
MouseDoPostEvent(InputInfoPtr pInfo, int buttons, int dx, int dy)
{
    static OsTimerPtr timer = NULL;
    MouseDevPtr pMse;
    int truebuttons;
    int id, change;

    pMse = pInfo->private;

    truebuttons = buttons;
    if (pMse->protocolID == PROT_MMHIT)
	buttons = reverseBits(hitachMap, buttons);
    else
	buttons = reverseBits(reverseMap, buttons);

    if (dx || dy)
	xf86PostMotionEvent(pInfo->dev, 0, 0, 2, dx, dy);

    if (pMse->emulate3Buttons) {
	/*
	 * Hack to operate the middle button even with Emulate3Buttons set.
	 * Modifying the state table to keep track of the middle button state
	 * would nearly double its size, so I'll stick with this fix.  - TJW
	 */
	if (pMse->protocolID == PROT_MMHIT)
	    change = buttons ^ reverseBits(hitachMap, pMse->lastButtons);
	else
	    change = buttons ^ reverseBits(reverseMap, pMse->lastButtons);

	/*
	 * process button 2, 4 and above
	 */
	change &= ~0x05;
	while (change) {
	    id = ffs(change);
	    change &= ~(1 << (id - 1));
	    xf86PostButtonEvent(pInfo->dev, 0, id,
				(buttons & (1 << (id - 1))), 0, 0);
	}

	/*
	 * emulate the third button by the other two
	 */
	buttons &= 0x07;
	if ((id = stateTab[buttons + pMse->emulateState][0]) != 0)
	    xf86PostButtonEvent(pInfo->dev, 0, abs(id), (id >= 0), 0, 0);

	if ((id = stateTab[buttons + pMse->emulateState][1]) != 0)
	    xf86PostButtonEvent(pInfo->dev, 0, abs(id), (id >= 0), 0, 0);

	pMse->emulateState = stateTab[buttons + pMse->emulateState][2];
	if (stateTab[buttons + pMse->emulateState][0] ||
	    stateTab[buttons + pMse->emulateState][1]) {
	    pMse->truebuttons = truebuttons;
	    timer = TimerSet(timer, 0, pMse->emulate3Timeout, buttonTimer,
			     pInfo);
	} else {
	    if (timer) {
		TimerFree(timer);
		timer = NULL;
	    }
	}
    } else {
	/*
	 * real three button event
	 * Note that pMse.lastButtons has the hardware button mapping which
	 * is the reverse of the button mapping reported to the server.
	 */
	if (pMse->protocolID == PROT_MMHIT)
	    change = buttons ^ reverseBits(hitachMap, pMse->lastButtons);
	else
	    change = buttons ^ reverseBits(reverseMap, pMse->lastButtons);
	while (change) {
	    id = ffs(change);
	    change &= ~(1 << (id - 1));
	    xf86PostButtonEvent(pInfo->dev, 0, id,
				(buttons & (1 << (id - 1))), 0, 0);
	}
    }
    pMse->lastButtons = truebuttons;

}

static void
MousePostEvent(InputInfoPtr pInfo, int buttons, int dx, int dy, int dz, int dw)
{
    MouseDevPtr pMse;
    int zbutton = 0;


    pMse = pInfo->private;

    /* Map the Z axis movement. */
    /* XXX Could this go in the conversion_proc? */
    switch (pMse->negativeZ) {
    case MSE_NOZMAP:	/* do nothing */
	break;
    case MSE_MAPTOX:
	if (dz != 0) {
	    dx = dz;
	    dz = 0;
	}
	break;
    case MSE_MAPTOY:
	if (dz != 0) {
	    dy = dz;
	    dz = 0;
	}
	break;
    default:	/* buttons */
	buttons &= ~(pMse->negativeZ | pMse->positiveZ
		   | pMse->negativeW | pMse->positiveW);
	if (dw < 0 || dz < -1)
	    zbutton = pMse->negativeW;
	else if (dz < 0)
	    zbutton = pMse->negativeZ;
	else if (dw > 0 || dz > 1)
	    zbutton = pMse->positiveW;
	else if (dz > 0)
	    zbutton = pMse->positiveZ;
	buttons |= zbutton;
	dz = 0;
	break;
    }
    dx = pMse->invX * dx;
    dy = pMse->invY * dy;
    if (pMse->flipXY) {
	int tmp = dx;
	dx = dy;
	dy = tmp;
    }
    MouseDoPostEvent(pInfo, buttons, dx, dy);

    /*
     * If dz has been mapped to a button `down' event, we need to cook up
     * a corresponding button `up' event.
     */
    if (zbutton) {
	buttons &= ~zbutton;
	MouseDoPostEvent(pInfo, buttons, 0, 0);
    }
}

#ifdef XFree86LOADER
static void
xf86MouseUnplug(pointer	p)
{
}
static pointer
xf86MousePlug(pointer	module,
	    pointer	options,
	    int		*errmaj,
	    int		*errmin)
{
    xf86AddInputDriver(&MOUSE, module, 0);

    return module;
}

static XF86ModuleVersionInfo xf86MouseVersionRec =
{
    "mouse",
    MODULEVENDORSTRING,
    MODINFOSTRING1,
    MODINFOSTRING2,
    XF86_VERSION_CURRENT,
    1, 0, 0,
    ABI_CLASS_XINPUT,
    ABI_XINPUT_VERSION,
    MOD_CLASS_XINPUT,
    {0, 0, 0, 0}		/* signature, to be patched into the file by */
				/* a tool */
};

XF86ModuleData mouseModuleData = {&xf86MouseVersionRec,
				  xf86MousePlug,
				  xf86MouseUnplug};

#endif /* XFree86LOADER */
