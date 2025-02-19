/* $XFree86: xc/programs/Xserver/Xext/xf86miscproc.h,v 1.2 2000/04/17 16:29:48 eich Exp $ */

/* Prototypes for Pointer/Keyboard functions that the DDX must provide */

#ifndef _XF86MISCPROC_H_
#define _XF86MISCPROC_H_

typedef enum {
    MISC_MSE_PROTO,
    MISC_MSE_BAUDRATE,
    MISC_MSE_SAMPLERATE,
    MISC_MSE_RESOLUTION,
    MISC_MSE_BUTTONS,
    MISC_MSE_EM3BUTTONS,
    MISC_MSE_EM3TIMEOUT,
    MISC_MSE_CHORDMIDDLE,
    MISC_MSE_FLAGS
} MiscExtMseValType;

typedef enum {
    MISC_KBD_TYPE,
    MISC_KBD_RATE,
    MISC_KBD_DELAY,
    MISC_KBD_SERVNUMLOCK
} MiscExtKbdValType;

typedef enum {
    MISC_RET_SUCCESS,
    MISC_RET_BADVAL,
    MISC_RET_BADMSEPROTO,
    MISC_RET_BADBAUDRATE,
    MISC_RET_BADFLAGS,
    MISC_RET_BADCOMBO,
    MISC_RET_BADKBDTYPE,
    MISC_RET_NOMODULE
} MiscExtReturn;

typedef enum {
    MISC_POINTER,
    MISC_KEYBOARD
} MiscExtStructType;

#define MISC_MSEFLAG_CLEARDTR	1
#define MISC_MSEFLAG_CLEARRTS	2
#define MISC_MSEFLAG_REOPEN	128

void XFree86MiscExtensionInit(void);

Bool MiscExtGetMouseSettings(pointer *mouse, char **devname);
int  MiscExtGetMouseValue(pointer mouse, MiscExtMseValType valtype);
Bool MiscExtSetMouseValue(pointer mouse, MiscExtMseValType valtype, int value);
Bool MiscExtGetKbdSettings(pointer *kbd);
int  MiscExtGetKbdValue(pointer kbd, MiscExtKbdValType valtype);
Bool MiscExtSetKbdValue(pointer kbd, MiscExtKbdValType valtype, int value);
pointer MiscExtCreateStruct(MiscExtStructType mse_or_kbd);
void    MiscExtDestroyStruct(pointer structure, MiscExtStructType mse_or_kbd);
MiscExtReturn MiscExtApply(pointer structure, MiscExtStructType mse_or_kbd);

#endif

