/* $XFree86: xc/programs/Xserver/hw/xfree86/os-support/bsd/bsd_mouse.c,v 1.17 2000/12/18 15:52:25 dawes Exp $ */

/*
 * Copyright 1999 by The XFree86 Project, Inc.
 */

#include "X.h"
#include "xf86.h"
#include "xf86Priv.h"
#include "xf86_OSlib.h"
#include "xf86Xinput.h"
#include "xf86OSmouse.h"
#include "xisb.h"
#include "mipointer.h"
#ifdef WSCONS_SUPPORT
#include <dev/wscons/wsconsio.h>
#endif
#ifdef USBMOUSE_SUPPORT
#include "usb.h"

#define HUP_GENERIC_DESKTOP     0x0001
#define HUP_BUTTON              0x0009

#define HUG_X                   0x0030
#define HUG_Y                   0x0031
#define HUG_Z                   0x0032
#define HUG_WHEEL               0x0038

#define HID_USAGE2(p,u) (((p) << 16) | u)

/* The UMS mices have middle button as number 3 */
#define UMS_BUT(i) ((i) == 0 ? 2 : (i) == 1 ? 0 : (i) == 2 ? 1 : (i))
#endif /* USBMOUSE_SUPPORT */

#ifdef WSCONS_SUPPORT
static void wsconsSigioReadInput (int fd, void *closure);
#endif
#ifdef USBMOUSE_SUPPORT
static void usbSigioReadInput (int fd, void *closure);
#endif

static int
SupportedInterfaces(void)
{
#if defined(__NetBSD__)
    return MSE_SERIAL | MSE_BUS | MSE_PS2 | MSE_AUTO;
#elif defined(__FreeBSD__)
    return MSE_SERIAL | MSE_BUS | MSE_PS2 | MSE_AUTO | MSE_MISC;
#else
    return MSE_SERIAL | MSE_BUS | MSE_PS2 | MSE_XPS2 | MSE_AUTO;
#endif
}

/* Names of protocols that are handled internally here. */
static const char *internalNames[] = {
#if defined(WSCONS_SUPPORT)
	"WSMouse",
#endif
#if defined(USBMOUSE_SUPPORT)
	"usb",
#endif
	NULL
};

/*
 * Names of MSC_MISC protocols that the OS supports.  These are decoded by
 * main "mouse" driver.
 */
static const char *miscNames[] = {
#if defined(__FreeBSD__)
	"SysMouse",
#endif
	NULL
};

static const char **
BuiltinNames(void)
{
    return internalNames;
}

static Bool
CheckProtocol(const char *protocol)
{
    int i;

    for (i = 0; internalNames[i]; i++)
	if (xf86NameCmp(protocol, internalNames[i]) == 0)
	    return TRUE;
    for (i = 0; miscNames[i]; i++)
	if (xf86NameCmp(protocol, miscNames[i]) == 0)
	    return TRUE;
    return FALSE;
}

static const char *
DefaultProtocol(void)
{
#if defined(__FreeBSD__)
    return "Auto";
#else
    return NULL;
#endif
}

#if defined(__FreeBSD__) && defined(MOUSE_PROTO_SYSMOUSE)
static struct {
	int dproto;
	const char *name;
} devproto[] = {
	{ MOUSE_PROTO_MS,		"Microsoft" },
	{ MOUSE_PROTO_MSC,		"MouseSystems" },
	{ MOUSE_PROTO_LOGI,		"Logitech" },
	{ MOUSE_PROTO_MM,		"MMSeries" },
	{ MOUSE_PROTO_LOGIMOUSEMAN,	"MouseMan" },
	{ MOUSE_PROTO_BUS,		"BusMouse" },
	{ MOUSE_PROTO_INPORT,		"BusMouse" },
	{ MOUSE_PROTO_PS2,		"PS/2" },
	{ MOUSE_PROTO_HITTAB,		"MMHitTab" },
	{ MOUSE_PROTO_GLIDEPOINT,	"GlidePoint" },
	{ MOUSE_PROTO_INTELLI,		"Intellimouse" },
	{ MOUSE_PROTO_THINK,		"ThinkingMouse" },
	{ MOUSE_PROTO_SYSMOUSE,		"SysMouse" }
};
	
static const char *
SetupAuto(InputInfoPtr pInfo, int *protoPara)
{
    int i;
    mousehw_t hw;
    mousemode_t mode;

    if (pInfo->fd == -1)
	return NULL;

    /* set the driver operation level, if applicable */
    i = 1;
    ioctl(pInfo->fd, MOUSE_SETLEVEL, &i);
    
    /* interrogate the driver and get some intelligence on the device. */
    hw.iftype = MOUSE_IF_UNKNOWN;
    hw.model = MOUSE_MODEL_GENERIC;
    ioctl(pInfo->fd, MOUSE_GETHWINFO, &hw);
    if (ioctl(pInfo->fd, MOUSE_GETMODE, &mode) == 0) {
	for (i = 0; i < sizeof(devproto)/sizeof(devproto[0]); ++i) {
	    if (mode.protocol == devproto[i].dproto) {
		/* override some parameters */
		if (protoPara) {
		    protoPara[4] = mode.packetsize;
		    protoPara[0] = mode.syncmask[0];
		    protoPara[1] = mode.syncmask[1];
		}
		return devproto[i].name;
	    }
	}
    }
    return NULL;
}

static void
SetSysMouseRes(InputInfoPtr pInfo, const char *protocol, int rate, int res)
{
    mousemode_t mode;
    MouseDevPtr pMse;

    pMse = pInfo->private;

    mode.rate = rate > 0 ? rate : -1;
    mode.resolution = res > 0 ? res : -1;
    mode.accelfactor = -1;
#if defined(__FreeBSD__)
    if (xf86NameCmp(pMse->origProtocol, "Auto") == 0) {
	/*
	 * As the FreeBSD sysmouse driver defaults to protocol level 0
	 * everytime it is opened we enforce protocol level 1 again at
	 * this point.
	 */
	mode.level = 1;
    } else
	mode.level = -1;
#else
    mode.level = -1;
#endif
    ioctl(pInfo->fd, MOUSE_SETMODE, &mode);
}
#endif

#if defined(WSCONS_SUPPORT)
#define NUMEVENTS 64

static int
wsconsMouseProc(DeviceIntPtr pPointer, int what)
{
    InputInfoPtr pInfo;
    MouseDevPtr pMse;
    unsigned char map[MSE_MAXBUTTONS + 1];
    int nbuttons;

    pInfo = pPointer->public.devicePrivate;
    pMse = pInfo->private;
    pMse->device = pPointer;

    switch (what) {
    case DEVICE_INIT: 
	pPointer->public.on = FALSE;

	for (nbuttons = 0; nbuttons < MSE_MAXBUTTONS; ++nbuttons)
	    map[nbuttons + 1] = nbuttons + 1;

	InitPointerDeviceStruct((DevicePtr)pPointer, 
				map, 
				min(pMse->buttons, MSE_MAXBUTTONS),
				miPointerGetMotionEvents, 
				pMse->Ctrl,
				miPointerGetMotionBufferSize());

	/* X valuator */
	xf86InitValuatorAxisStruct(pPointer, 0, 0, -1, 1, 0, 1);
	xf86InitValuatorDefaults(pPointer, 0);
	/* Y valuator */
	xf86InitValuatorAxisStruct(pPointer, 1, 0, -1, 1, 0, 1);
	xf86InitValuatorDefaults(pPointer, 1);
	xf86MotionHistoryAllocate(pInfo);
	break;

    case DEVICE_ON:
	pInfo->fd = xf86OpenSerial(pInfo->options);
	if (pInfo->fd == -1)
	    xf86Msg(X_WARNING, "%s: cannot open input device\n", pInfo->name);
	else {
	    pMse->buffer = XisbNew(pInfo->fd,
			      NUMEVENTS * sizeof(struct wscons_event));
	    if (!pMse->buffer) {
		xfree(pMse);
		xf86CloseSerial(pInfo->fd);
		pInfo->fd = -1;
	    } else {
		xf86FlushInput(pInfo->fd);
		if (!xf86InstallSIGIOHandler (pInfo->fd, wsconsSigioReadInput, pInfo))
		    AddEnabledDevice(pInfo->fd);
	    }
	}
	pMse->lastButtons = 0;
	pMse->emulateState = 0;
	pPointer->public.on = TRUE;
	break;

    case DEVICE_OFF:
    case DEVICE_CLOSE:
	if (pInfo->fd != -1) {
	    RemoveEnabledDevice(pInfo->fd);
	    if (pMse->buffer) {
		XisbFree(pMse->buffer);
		pMse->buffer = NULL;
	    }
	    xf86CloseSerial(pInfo->fd);
	    pInfo->fd = -1;
	}
	pPointer->public.on = FALSE;
	usleep(300000);
	break;
    }
    return Success;
}


static void
wsconsReadInput(InputInfoPtr pInfo)
{
    MouseDevPtr pMse;
    static struct wscons_event eventList[NUMEVENTS];
    int n, c; 
    struct wscons_event *event = eventList;
    unsigned char *pBuf;

    pMse = pInfo->private;

    XisbBlockDuration(pMse->buffer, -1);
    pBuf = (unsigned char *)eventList;
    n = 0;
    while ((c = XisbRead(pMse->buffer)) >= 0 && n < sizeof(eventList)) {
	pBuf[n++] = (unsigned char)c;
    }

    if (n == 0)
	return;

    n /= sizeof(struct wscons_event);
    while( n-- ) {
	int buttons = pMse->lastButtons;
	int dx = 0, dy = 0, dz = 0, dw = 0;
	switch (event->type) {
	case WSCONS_EVENT_MOUSE_UP:
#define BUTBIT (1 << (event->value <= 2 ? 2 - event->value : event->value))
	    buttons &= ~BUTBIT;
	    break;
	case WSCONS_EVENT_MOUSE_DOWN:
	    buttons |= BUTBIT;
	    break;
	case WSCONS_EVENT_MOUSE_DELTA_X:
	    dx = event->value;
	    break;
	case WSCONS_EVENT_MOUSE_DELTA_Y:
	    dy = -event->value;
	    break;
#ifdef WSCONS_EVENT_MOUSE_DELTA_Z
	case WSCONS_EVENT_MOUSE_DELTA_Z:
	    dz = event->value;
	    break;
#endif
	default:
	    xf86Msg(X_WARNING, "%s: bad wsmouse event type=%d\n", pInfo->name,
		    event->type);
	    continue;
	}

	pMse->PostEvent(pInfo, buttons, dx, dy, dz, dw);
	++event;
    }
    return;
}

static void
wsconsSigioReadInput (int fd, void *closure)
{
    wsconsReadInput ((InputInfoPtr) closure);
}


/* This function is called when the protocol is "wsmouse". */
static Bool
wsconsPreInit(InputInfoPtr pInfo, const char *protocol, int flags)
{
    MouseDevPtr pMse = pInfo->private;

    pMse->protocol = protocol;
    xf86Msg(X_CONFIG, "%s: Protocol: %s\n", pInfo->name, protocol);

    /* Collect the options, and process the common options. */
    xf86CollectInputOptions(pInfo, NULL, NULL);
    xf86ProcessCommonOptions(pInfo, pInfo->options);

    /* Check if the device can be opened. */
    pInfo->fd = xf86OpenSerial(pInfo->options);
    if (pInfo->fd == -1) {
	if (xf86GetAllowMouseOpenFail())
	    xf86Msg(X_WARNING, "%s: cannot open input device\n", pInfo->name);
	else {
	    xf86Msg(X_ERROR, "%s: cannot open input device\n", pInfo->name);
	    xfree(pMse);
	    return FALSE;
	}
    }
    xf86CloseSerial(pInfo->fd);
    pInfo->fd = -1;

    /* Process common mouse options (like Emulate3Buttons, etc). */
    pMse->CommonOptions(pInfo);

    /* Setup the local procs. */
    pInfo->device_control = wsconsMouseProc;
    pInfo->read_input = wsconsReadInput;

    pInfo->flags |= XI86_CONFIGURED;
    return TRUE;
}
#endif

#if defined(USBMOUSE_SUPPORT)

typedef struct _UsbMseRec {
    int packetSize;
    int iid;
    hid_item_t loc_x;		/* x locator item */
    hid_item_t loc_y;		/* y locator item */
    hid_item_t loc_z;		/* z (wheel) locator item */
    hid_item_t loc_btn[MSE_MAXBUTTONS]; /* buttons locator items */
   unsigned char *buffer;
} UsbMseRec, *UsbMsePtr;

static int
usbMouseProc(DeviceIntPtr pPointer, int what)
{
    InputInfoPtr pInfo;
    MouseDevPtr pMse;
    UsbMsePtr pUsbMse;
    unsigned char map[MSE_MAXBUTTONS + 1];
    int nbuttons;

    pInfo = pPointer->public.devicePrivate;
    pMse = pInfo->private;
    pMse->device = pPointer;
    pUsbMse = pMse->mousePriv;

    switch (what) {
    case DEVICE_INIT: 
	pPointer->public.on = FALSE;

	for (nbuttons = 0; nbuttons < MSE_MAXBUTTONS; ++nbuttons)
	    map[nbuttons + 1] = nbuttons + 1;

	InitPointerDeviceStruct((DevicePtr)pPointer, 
				map, 
				min(pMse->buttons, MSE_MAXBUTTONS),
				miPointerGetMotionEvents, 
				pMse->Ctrl,
				miPointerGetMotionBufferSize());

	/* X valuator */
	xf86InitValuatorAxisStruct(pPointer, 0, 0, -1, 1, 0, 1);
	xf86InitValuatorDefaults(pPointer, 0);
	/* Y valuator */
	xf86InitValuatorAxisStruct(pPointer, 1, 0, -1, 1, 0, 1);
	xf86InitValuatorDefaults(pPointer, 1);
	xf86MotionHistoryAllocate(pInfo);
	break;

    case DEVICE_ON:
	pInfo->fd = xf86OpenSerial(pInfo->options);
	if (pInfo->fd == -1)
	    xf86Msg(X_WARNING, "%s: cannot open input device\n", pInfo->name);
	else {
	    pMse->buffer = XisbNew(pInfo->fd, pUsbMse->packetSize);
	    if (!pMse->buffer) {
		xfree(pMse);
		xf86CloseSerial(pInfo->fd);
		pInfo->fd = -1;
	    } else {
		xf86FlushInput(pInfo->fd);
		if (!xf86InstallSIGIOHandler (pInfo->fd, usbSigioReadInput, 
					      pInfo))
		    AddEnabledDevice(pInfo->fd);
	    }
	}
	pMse->lastButtons = 0;
	pMse->emulateState = 0;
	pPointer->public.on = TRUE;
	break;

    case DEVICE_OFF:
    case DEVICE_CLOSE:
	if (pInfo->fd != -1) {
	    RemoveEnabledDevice(pInfo->fd);
	    if (pUsbMse->packetSize > 8 && pUsbMse->buffer) {
		xfree(pUsbMse->buffer);
	    }
	    if (pMse->buffer) {
		XisbFree(pMse->buffer);
		pMse->buffer = NULL;
	    }
	    xf86CloseSerial(pInfo->fd);
	    pInfo->fd = -1;
	}
	pPointer->public.on = FALSE;
	usleep(300000);
	break;
    }
    return Success;
}

static void
usbReadInput(InputInfoPtr pInfo)
{
    MouseDevPtr pMse;
    UsbMsePtr pUsbMse;
    int buttons = pMse->lastButtons;
    int dx = 0, dy = 0, dz = 0, dw = 0;
    int n, c; 
    unsigned char *pBuf;

    pMse = pInfo->private;
    pUsbMse = pMse->mousePriv;

    XisbBlockDuration(pMse->buffer, -1);
    pBuf = pUsbMse->buffer;
    n = 0;
    while ((c = XisbRead(pMse->buffer)) >= 0 && n < pUsbMse->packetSize) {
	pBuf[n++] = (unsigned char)c;
    }
    if (n == 0)
	return;
    if (n != pUsbMse->packetSize) {
	xf86Msg(X_WARNING, "%s: incomplete packet, size %d\n", pInfo->name,
		n);
    }
    /* discard packets with an id that don't match the mouse */
    /* XXX this is probably not the right thing */
    if (pUsbMse->iid != 0) {
	if (*pBuf++ != pUsbMse->iid) 
	    return;
    }
    dx = hid_get_data(pBuf, &pUsbMse->loc_x);
    dy = hid_get_data(pBuf, &pUsbMse->loc_y);
    dz = hid_get_data(pBuf, &pUsbMse->loc_z);

    buttons = 0;
    for (n = 0; n < pMse->buttons; n++) {
	if (hid_get_data(pBuf, &pUsbMse->loc_btn[n])) 
	    buttons |= (1 << UMS_BUT(n));
    }
    pMse->PostEvent(pInfo, buttons, dx, dy, dz, dw);
    return;
}

static void
usbSigioReadInput (int fd, void *closure)
{
    usbReadInput ((InputInfoPtr) closure);
}

/* This function is called when the protocol is "usb". */
static Bool
usbPreInit(InputInfoPtr pInfo, const char *protocol, int flags)
{
    MouseDevPtr pMse = pInfo->private;
    UsbMsePtr pUsbMse;
    report_desc_t reportDesc;
    int i;

    pUsbMse = xalloc(sizeof(UsbMseRec));
    if (pUsbMse == NULL) {
	xf86Msg(X_ERROR, "%s: cannot allocate UsbMouseRec\n", pInfo->name);
	xfree(pMse);
	return FALSE;
    }

    pMse->protocol = protocol;
    xf86Msg(X_CONFIG, "%s: Protocol: %s\n", pInfo->name, protocol);

    /* Collect the options, and process the common options. */
    xf86CollectInputOptions(pInfo, NULL, NULL);
    xf86ProcessCommonOptions(pInfo, pInfo->options);

    /* Check if the device can be opened. */
    pInfo->fd = xf86OpenSerial(pInfo->options);
    if (pInfo->fd == -1) {
	if (xf86GetAllowMouseOpenFail())
	    xf86Msg(X_WARNING, "%s: cannot open input device\n", pInfo->name);
	else {
	    xf86Msg(X_ERROR, "%s: cannot open input device\n", pInfo->name);
	    xfree(pUsbMse);
	    xfree(pMse);
	    return FALSE;
	}
    }
    /* Get USB informations */
    reportDesc = hid_get_report_desc(pInfo->fd);
    /* Get packet size & iid */
    pUsbMse->packetSize = hid_report_size(reportDesc, hid_input,
					      &pUsbMse->iid);
    /* Allocate buffer */
    if (pUsbMse->packetSize <= 8) {
	pUsbMse->buffer = pMse->protoBuf;
    } else {
	pUsbMse->buffer = xalloc(pUsbMse->packetSize);
    }
    if (pUsbMse->buffer == NULL) {
	xf86Msg(X_ERROR, "%s: cannot allocate buffer\n", pInfo->name);
	xfree(pUsbMse);
	xfree(pMse);
	xf86CloseSerial(pInfo->fd);
	return FALSE;
    }
    if (hid_locate(reportDesc, HID_USAGE2(HUP_GENERIC_DESKTOP, HUG_X),
		   hid_input, &pUsbMse->loc_x) < 0) {
	xf86Msg(X_WARNING, "%s: no x locator\n");
    }
    if (hid_locate(reportDesc, HID_USAGE2(HUP_GENERIC_DESKTOP, HUG_Y),
		   hid_input, &pUsbMse->loc_y) < 0) {
	xf86Msg(X_WARNING, "%s: no y locator\n");
    }
    if (hid_locate(reportDesc, HID_USAGE2(HUP_GENERIC_DESKTOP, HUG_WHEEL),
		   hid_input, &pUsbMse->loc_z) < 0) {
    }
    /* Probe for number of buttons */
    for (i = 1; i <= MSE_MAXBUTTONS; i++) {
	if (!hid_locate(reportDesc, HID_USAGE2(HUP_BUTTON, i),
			hid_input, &pUsbMse->loc_btn[i-1])) 
	    break;
    }
    pMse->buttons = i-1;

    xf86CloseSerial(pInfo->fd);
    pInfo->fd = -1;

    /* Private structure */
    pMse->mousePriv = pUsbMse;

    /* Process common mouse options (like Emulate3Buttons, etc). */
    pMse->CommonOptions(pInfo);

    /* Setup the local procs. */
    pInfo->device_control = usbMouseProc;
    pInfo->read_input = usbReadInput;

    pInfo->flags |= XI86_CONFIGURED;
    return TRUE;
}
#endif /* USBMOUSE */

OSMouseInfoPtr
xf86OSMouseInit(int flags)
{
    OSMouseInfoPtr p;

    p = xcalloc(sizeof(OSMouseInfoRec), 1);
    if (!p)
	return NULL;
    p->SupportedInterfaces = SupportedInterfaces;
    p->BuiltinNames = BuiltinNames;
    p->DefaultProtocol = DefaultProtocol;
    p->CheckProtocol = CheckProtocol;
#if defined(__FreeBSD__) && defined(MOUSE_PROTO_SYSMOUSE)
    p->SetupAuto = SetupAuto;
    p->SetPS2Res = SetSysMouseRes;
    p->SetBMRes = SetSysMouseRes;
    p->SetMiscRes = SetSysMouseRes;
#endif
    /* XXX This assumes that WSCONS and USB are mutually exclusive. */
#if defined(WSCONS_SUPPORT)
    p->PreInit = wsconsPreInit;
#endif
#if defined(USBMOUSE_SUPPORT)
    p->PreInit = usbPreInit;
#endif
    return p;
}
