/* $XFree86: xc/programs/Xserver/hw/xfree86/ddc/xf86DDC.c,v 1.19 2000/11/16 19:44:55 eich Exp $ */

/* xf86DDC.c 
 * 
 * Copyright 1998,1999 by Egbert Eich <Egbert.Eich@Physik.TU-Darmstadt.DE>
 */
#include "misc.h"
#include "xf86.h"
#include "xf86_ansic.h"
#include "xf86_OSproc.h"
#include "xf86DDC.h"

const char *i2cSymbols[] = {
    "xf86CreateI2CDevRec",
    "xf86I2CDevInit",
    "xf86I2CWriteRead",
    "xf86DestroyI2CDevRec",
    NULL
};

#ifdef XFree86LOADER

static MODULESETUPPROTO(ddcSetup);

static XF86ModuleVersionInfo ddcVersRec =
{
    "ddc",
    MODULEVENDORSTRING,
    MODINFOSTRING1,
    MODINFOSTRING2,
    XF86_VERSION_CURRENT,
    1, 0, 0,
    ABI_CLASS_VIDEODRV,		/* needs the video driver ABI */
    ABI_VIDEODRV_VERSION,
    MOD_CLASS_NONE,
    {0,0,0,0}
};

XF86ModuleData ddcModuleData = { &ddcVersRec, ddcSetup, NULL };

static pointer
ddcSetup(pointer module, pointer opts, int *errmaj, int *errmin)
{
    static Bool setupDone = FALSE;

    if (!setupDone) {
	setupDone = TRUE;
	/*
	 * Tell the loader about symbols from other modules that this module
	 * might refer to.
	 */
	LoaderRefSymLists(i2cSymbols, NULL);

    } 
    /*
     * The return value must be non-NULL on success even though there
     * is no TearDownProc.
     */
    return (pointer)1;
}

#endif

#define RETRIES 4

extern unsigned char *GetEDID_DDC1(
    unsigned int *
);

extern int checksum(
    unsigned char *,
    int
);

static unsigned char *EDIDRead_DDC1(
    ScrnInfoPtr pScrn,
    void (*)(ScrnInfoPtr,xf86ddcSpeed), 
    unsigned int (*)(ScrnInfoPtr)
);

static Bool TestDDC1(
    ScrnInfoPtr pScrn,
    unsigned int (*)(ScrnInfoPtr)
);

static unsigned int *FetchEDID_DDC1(
    ScrnInfoPtr,
    register unsigned int (*)(ScrnInfoPtr)
);

static unsigned char* EDID1Read_DDC2(
    int scrnIndex, 
    I2CBusPtr pBus
);

static unsigned char * VDIFRead(
    int scrnIndex, 
    I2CBusPtr pBus, 
    int start
);

static unsigned char * DDCRead_DDC2(
    int scrnIndex,
    I2CBusPtr pBus, 
    int start, 
    int len
);

typedef enum {
    DDCOPT_NODDC1,
    DDCOPT_NODDC2,
    DDCOPT_NODDC
} DDCOpts;

static OptionInfoRec DDCOptions[] = {
    { DDCOPT_NODDC1,	"NoDDC1",	OPTV_BOOLEAN,	{0},	FALSE },
    { DDCOPT_NODDC2,	"NoDDC2",	OPTV_BOOLEAN,	{0},	FALSE },
    { DDCOPT_NODDC,	"NoDDC",	OPTV_BOOLEAN,	{0},	FALSE },
    { -1,		NULL,		OPTV_NONE,	{0},	FALSE },
};

#define nDDCOptions (sizeof(DDCOptions) / sizeof(DDCOptions[0]))

xf86MonPtr 
xf86DoEDID_DDC1(
    int scrnIndex, void (*DDC1SetSpeed)(ScrnInfoPtr, xf86ddcSpeed), 
    unsigned int (*DDC1Read)(ScrnInfoPtr)
)
{
    ScrnInfoPtr pScrn = xf86Screens[scrnIndex];
    unsigned char *EDID_block = NULL;
    xf86MonPtr tmp = NULL;
    int sigio;
    /* Default DDC and DDC1 to enabled. */
    Bool noddc = FALSE, noddc1 = FALSE;
    OptionInfoRec options[nDDCOptions];

    (void)memcpy(options, DDCOptions, sizeof(DDCOptions));
    xf86ProcessOptions(pScrn->scrnIndex, pScrn->options, options);

    xf86GetOptValBool(options, DDCOPT_NODDC, &noddc);
    xf86GetOptValBool(options, DDCOPT_NODDC1, &noddc1);
    
    if (noddc || noddc1)
	return NULL;
    
    sigio = xf86BlockSIGIO();
    EDID_block = EDIDRead_DDC1(pScrn,DDC1SetSpeed,DDC1Read);
    xf86UnblockSIGIO(sigio);

    if (EDID_block){
	tmp = xf86InterpretEDID(scrnIndex,EDID_block);
    }
#ifdef DEBUG
	else ErrorF("No EDID block returned\n");
    if (!tmp)
	ErrorF("Cannot interpret EDID block\n");
#endif
	return tmp;
}

xf86MonPtr
xf86DoEDID_DDC2(int scrnIndex, I2CBusPtr pBus)
{
    ScrnInfoPtr pScrn = xf86Screens[scrnIndex];
    unsigned char *EDID_block = NULL;
    unsigned char *VDIF_Block = NULL;
    xf86MonPtr tmp = NULL;
    /* Default DDC and DDC2 to enabled. */
    Bool noddc = FALSE, noddc2 = FALSE;
    OptionInfoRec options[nDDCOptions];

    (void)memcpy(options, DDCOptions, sizeof(DDCOptions));
    xf86ProcessOptions(pScrn->scrnIndex, pScrn->options, options);

    xf86GetOptValBool(options, DDCOPT_NODDC, &noddc);
    xf86GetOptValBool(options, DDCOPT_NODDC2, &noddc2);
    
    if (noddc || noddc2)
	return NULL;

    EDID_block = EDID1Read_DDC2(scrnIndex,pBus);

    if (EDID_block){
	tmp = xf86InterpretEDID(scrnIndex,EDID_block);
    } else {
#ifdef DEBUG
	ErrorF("No EDID block returned\n");
#endif
	return NULL;
    }
#ifdef DEBUG
    if (!tmp)
	ErrorF("Cannot interpret EDID block\n");
    ErrorF("Sections to follow: %i\n",tmp->no_sections);
#endif
    VDIF_Block = 
	VDIFRead(scrnIndex, pBus, EDID1_LEN * (tmp->no_sections + 1));    
    tmp->vdif = xf86InterpretVdif(VDIF_Block);

    return tmp;
}

/* 
 * read EDID record , pass it to callback function to interpret.
 * callback function will store it for further use by calling
 * function; it will also decide if we need to reread it 
 */
static unsigned char *
EDIDRead_DDC1(ScrnInfoPtr pScrn, void (*DDCSpeed)(ScrnInfoPtr,xf86ddcSpeed), 
              unsigned int (*read_DDC)(ScrnInfoPtr))
{
    unsigned char *EDID_block = NULL;
    int count = RETRIES;

    if (!read_DDC) { 
	xf86DrvMsg(pScrn->scrnIndex, X_PROBED, 
		   "chipset doesn't support DDC1\n");
	return NULL; 
    };

    if (TestDDC1(pScrn,read_DDC)==-1) { 
	xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "No DDC signal\n"); 
	return NULL; 
    };

    if (DDCSpeed) DDCSpeed(pScrn,DDC_FAST);
    do {
	EDID_block = GetEDID_DDC1(FetchEDID_DDC1(pScrn,read_DDC)); 
	count --;
    } while (!EDID_block && count);
    if (DDCSpeed) DDCSpeed(pScrn,DDC_SLOW);

    return EDID_block;
}

/* test if DDC1  return 0 if not */
static Bool
TestDDC1(ScrnInfoPtr pScrn, unsigned int (*read_DDC)(ScrnInfoPtr))
{
    int old, count;

    old = read_DDC(pScrn);
    count = HEADER * BITS_PER_BYTE;
    do {
	/* wait for next retrace */
	if (old != read_DDC(pScrn)) break;
    } while(count--);
    return (count);
}

/* fetch entire EDID record; DDC bit needs to be masked */
static unsigned int * 
FetchEDID_DDC1(register ScrnInfoPtr pScrn,
	       register unsigned int (*read_DDC)(ScrnInfoPtr))
{
    int count = NUM;
    unsigned int *ptr, *xp;

    ptr=xp=xalloc(sizeof(int)*NUM); 

    if (!ptr)  return NULL;
    do {
	/* wait for next retrace */
	*xp = read_DDC(pScrn);
	xp++;
    } while(--count);
    return (ptr);
}

static unsigned char*
EDID1Read_DDC2(int scrnIndex, I2CBusPtr pBus)
{
    return  DDCRead_DDC2(scrnIndex, pBus, 0, EDID1_LEN);
}

static unsigned char*
VDIFRead(int scrnIndex, I2CBusPtr pBus, int start)
{
    unsigned char * Buffer, *v_buffer = NULL, *v_bufferp = NULL;
    int i, num = 0;

    /* read VDIF length in 64 byte blocks */
    Buffer = DDCRead_DDC2(scrnIndex, pBus,start,64);
    if (Buffer == NULL)
	return NULL;
#ifdef DEBUG
    ErrorF("number of 64 bit blocks: %i\n",Buffer[0]);
#endif
    if ((num = Buffer[0]) > 0)
	v_buffer = v_bufferp = xalloc(sizeof(unsigned char) * 64 * num);

    for (i = 0; i < num; i++) {
	Buffer = DDCRead_DDC2(scrnIndex, pBus,start,64);
	if (Buffer == NULL) {
	    xfree (v_buffer);
	    return NULL;
	}
	memcpy(v_bufferp,Buffer,63); /* 64th byte is checksum */
	xfree(Buffer);
	v_bufferp += 63;
    }
    return v_buffer;
}

static unsigned char *
DDCRead_DDC2(int scrnIndex, I2CBusPtr pBus, int start, int len)
{
    I2CDevPtr dev;
    unsigned char W_Buffer[2];
    int w_bytes;
    unsigned char *R_Buffer;
    int i;
    
    xf86LoaderReqSymLists(i2cSymbols, NULL);
    dev = xf86CreateI2CDevRec();
    dev->DevName = "ddc2";
    dev->SlaveAddr = 0xA0;
    dev->ByteTimeout = 2200; /* VESA DDC spec 3 p. 43 (+10 %) */
    dev->StartTimeout = 550;
    dev->BitTimeout = 40;
    dev->ByteTimeout = 40;
    dev->AcknTimeout = 40;

    dev->pI2CBus = pBus;
    if (! xf86I2CDevInit(dev)) {
	xf86DrvMsg(X_PROBED,scrnIndex,"No DDC2 device\n");
	return NULL;
    }
    if (start < 0x100) {
	w_bytes = 1;
	W_Buffer[0] = start;
    } else {
	w_bytes = 2;
	W_Buffer[0] = start & 0xFF;
	W_Buffer[1] = (start & 0xFF00) >> 8;
    }
    R_Buffer = xcalloc(1,sizeof(unsigned char) 
					* (len));
    for (i=0; i<RETRIES; i++) {
	if (xf86I2CWriteRead(dev, W_Buffer,w_bytes, R_Buffer,len)) {
	    if (!checksum(R_Buffer,len)) { 
		xf86DestroyI2CDevRec(dev,TRUE);
		return R_Buffer;
	    }
#ifdef DEBUG
	    else ErrorF("Checksum error in EDID block\n");
#endif
	}
#ifdef DEBUG
	else ErrorF("Error reading EDID block\n");
#endif
    }
    
    xf86DestroyI2CDevRec(dev,TRUE);
    xfree(R_Buffer);
    return NULL;
}


