/* $XFree86: xc/programs/Xserver/hw/xwin/ddraw.h,v 1.1 2000/08/10 17:40:37 dawes Exp $ */

#ifndef __WINE_DDRAW_H
#define __WINE_DDRAW_H

#include <X11/Xlib.h>
#ifdef HAVE_LIBXXSHM
#include <X11/extensions/XShm.h>
#endif /* defined(HAVE_LIBXXSHM) */

#include "winnt.h" /* LARGE_INTEGER ... */
#include "wingdi.h" /* PALETTE stuff ... */
#include "wine/obj_base.h"

#ifndef	DIRECTDRAW_VERSION
#define	DIRECTDRAW_VERSION	0x0500
#endif /* DIRECTDRAW_VERSION */

/*****************************************************************************
 * Predeclare the interfaces
 */
DEFINE_GUID( CLSID_DirectDraw,		0xD7B70EE0,0x4340,0x11CF,0xB0,0x63,0x00,0x20,0xAF,0xC2,0xCD,0x35 );
DEFINE_GUID( CLSID_DirectDrawClipper,	0x593817A0,0x7DB3,0x11CF,0xA2,0xDE,0x00,0xAA,0x00,0xb9,0x33,0x56 );
DEFINE_GUID( IID_IDirectDraw,		0x6C14DB80,0xA733,0x11CE,0xA5,0x21,0x00,0x20,0xAF,0x0B,0xE5,0x60 );
DEFINE_GUID( IID_IDirectDraw2,		0xB3A6F3E0,0x2B43,0x11CF,0xA2,0xDE,0x00,0xAA,0x00,0xB9,0x33,0x56 );
DEFINE_GUID( IID_IDirectDraw4,          0x9c59509a,0x39bd,0x11d1,0x8c,0x4a,0x00,0xc0,0x4f,0xd9,0x30,0xc5 );
DEFINE_GUID( IID_IDirectDrawSurface,	0x6C14DB81,0xA733,0x11CE,0xA5,0x21,0x00,0x20,0xAF,0x0B,0xE5,0x60 );
DEFINE_GUID( IID_IDirectDrawSurface2,	0x57805885,0x6eec,0x11cf,0x94,0x41,0xa8,0x23,0x03,0xc1,0x0e,0x27 );
DEFINE_GUID( IID_IDirectDrawSurface3,	0xDA044E00,0x69B2,0x11D0,0xA1,0xD5,0x00,0xAA,0x00,0xB8,0xDF,0xBB );
DEFINE_GUID( IID_IDirectDrawSurface4,   0x0B2B8630,0xAD35,0x11D0,0x8E,0xA6,0x00,0x60,0x97,0x97,0xEA,0x5B );
DEFINE_GUID( IID_IDirectDrawPalette,	0x6C14DB84,0xA733,0x11CE,0xA5,0x21,0x00,0x20,0xAF,0x0B,0xE5,0x60 );
DEFINE_GUID( IID_IDirectDrawClipper,	0x6C14DB85,0xA733,0x11CE,0xA5,0x21,0x00,0x20,0xAF,0x0B,0xE5,0x60 );
DEFINE_GUID( IID_IDirectDrawColorControl,0x4B9F0EE0,0x0D7E,0x11D0,0x9B,0x06,0x00,0xA0,0xC9,0x03,0xA3,0xB8 );

typedef struct IDirectDraw IDirectDraw,*LPDIRECTDRAW;
typedef struct IDirectDraw2 IDirectDraw2,*LPDIRECTDRAW2;
typedef struct IDirectDraw4 IDirectDraw4,*LPDIRECTDRAW4;
typedef struct IDirectDrawClipper IDirectDrawClipper,*LPDIRECTDRAWCLIPPER;
typedef struct IDirectDrawPalette IDirectDrawPalette,*LPDIRECTDRAWPALETTE;
typedef struct IDirectDrawSurface IDirectDrawSurface,*LPDIRECTDRAWSURFACE;
typedef struct IDirectDrawSurface2 IDirectDrawSurface2,*LPDIRECTDRAWSURFACE2;
typedef struct IDirectDrawSurface3 IDirectDrawSurface3,*LPDIRECTDRAWSURFACE3;
typedef struct IDirectDrawSurface4 IDirectDrawSurface4,*LPDIRECTDRAWSURFACE4;
typedef struct IDirectDrawColorControl IDirectDrawColorControl,*LPDIRECTDRAWCOLORCONTROL;


#define DDENUMRET_CANCEL	0
#define DDENUMRET_OK		1

#define DD_OK			0


#define _FACDD		0x876
#define MAKE_DDHRESULT( code )  MAKE_HRESULT( 1, _FACDD, code )

#define DDERR_ALREADYINITIALIZED		MAKE_DDHRESULT( 5 )
#define DDERR_CANNOTATTACHSURFACE		MAKE_DDHRESULT( 10 )
#define DDERR_CANNOTDETACHSURFACE		MAKE_DDHRESULT( 20 )
#define DDERR_CURRENTLYNOTAVAIL			MAKE_DDHRESULT( 40 )
#define DDERR_EXCEPTION				MAKE_DDHRESULT( 55 )
#define DDERR_GENERIC				E_FAIL
#define DDERR_HEIGHTALIGN			MAKE_DDHRESULT( 90 )
#define DDERR_INCOMPATIBLEPRIMARY		MAKE_DDHRESULT( 95 )
#define DDERR_INVALIDCAPS			MAKE_DDHRESULT( 100 )
#define DDERR_INVALIDCLIPLIST			MAKE_DDHRESULT( 110 )
#define DDERR_INVALIDMODE			MAKE_DDHRESULT( 120 )
#define DDERR_INVALIDOBJECT			MAKE_DDHRESULT( 130 )
#define DDERR_INVALIDPARAMS			E_INVALIDARG
#define DDERR_INVALIDPIXELFORMAT		MAKE_DDHRESULT( 145 )
#define DDERR_INVALIDRECT			MAKE_DDHRESULT( 150 )
#define DDERR_LOCKEDSURFACES			MAKE_DDHRESULT( 160 )
#define DDERR_NO3D				MAKE_DDHRESULT( 170 )
#define DDERR_NOALPHAHW				MAKE_DDHRESULT( 180 )
#define DDERR_NOCLIPLIST			MAKE_DDHRESULT( 205 )
#define DDERR_NOCOLORCONVHW			MAKE_DDHRESULT( 210 )
#define DDERR_NOCOOPERATIVELEVELSET		MAKE_DDHRESULT( 212 )
#define DDERR_NOCOLORKEY			MAKE_DDHRESULT( 215 )
#define DDERR_NOCOLORKEYHW			MAKE_DDHRESULT( 220 )
#define DDERR_NODIRECTDRAWSUPPORT		MAKE_DDHRESULT( 222 )
#define DDERR_NOEXCLUSIVEMODE			MAKE_DDHRESULT( 225 )
#define DDERR_NOFLIPHW				MAKE_DDHRESULT( 230 )
#define DDERR_NOGDI				MAKE_DDHRESULT( 240 )
#define DDERR_NOMIRRORHW			MAKE_DDHRESULT( 250 )
#define DDERR_NOTFOUND				MAKE_DDHRESULT( 255 )
#define DDERR_NOOVERLAYHW			MAKE_DDHRESULT( 260 )
#define DDERR_NORASTEROPHW			MAKE_DDHRESULT( 280 )
#define DDERR_NOROTATIONHW			MAKE_DDHRESULT( 290 )
#define DDERR_NOSTRETCHHW			MAKE_DDHRESULT( 310 )
#define DDERR_NOT4BITCOLOR			MAKE_DDHRESULT( 316 )
#define DDERR_NOT4BITCOLORINDEX			MAKE_DDHRESULT( 317 )
#define DDERR_NOT8BITCOLOR			MAKE_DDHRESULT( 320 )
#define DDERR_NOTEXTUREHW			MAKE_DDHRESULT( 330 )
#define DDERR_NOVSYNCHW				MAKE_DDHRESULT( 335 )
#define DDERR_NOZBUFFERHW			MAKE_DDHRESULT( 340 )
#define DDERR_NOZOVERLAYHW			MAKE_DDHRESULT( 350 )
#define DDERR_OUTOFCAPS				MAKE_DDHRESULT( 360 )
#define DDERR_OUTOFMEMORY			E_OUTOFMEMORY
#define DDERR_OUTOFVIDEOMEMORY			MAKE_DDHRESULT( 380 )
#define DDERR_OVERLAYCANTCLIP			MAKE_DDHRESULT( 382 )
#define DDERR_OVERLAYCOLORKEYONLYONEACTIVE	MAKE_DDHRESULT( 384 )
#define DDERR_PALETTEBUSY			MAKE_DDHRESULT( 387 )
#define DDERR_COLORKEYNOTSET			MAKE_DDHRESULT( 400 )
#define DDERR_SURFACEALREADYATTACHED		MAKE_DDHRESULT( 410 )
#define DDERR_SURFACEALREADYDEPENDENT		MAKE_DDHRESULT( 420 )
#define DDERR_SURFACEBUSY			MAKE_DDHRESULT( 430 )
#define DDERR_CANTLOCKSURFACE			MAKE_DDHRESULT( 435 )
#define DDERR_SURFACEISOBSCURED			MAKE_DDHRESULT( 440 )
#define DDERR_SURFACELOST			MAKE_DDHRESULT( 450 )
#define DDERR_SURFACENOTATTACHED		MAKE_DDHRESULT( 460 )
#define DDERR_TOOBIGHEIGHT			MAKE_DDHRESULT( 470 )
#define DDERR_TOOBIGSIZE			MAKE_DDHRESULT( 480 )
#define DDERR_TOOBIGWIDTH			MAKE_DDHRESULT( 490 )
#define DDERR_UNSUPPORTED			E_NOTIMPL
#define DDERR_UNSUPPORTEDFORMAT			MAKE_DDHRESULT( 510 )
#define DDERR_UNSUPPORTEDMASK			MAKE_DDHRESULT( 520 )
#define DDERR_VERTICALBLANKINPROGRESS		MAKE_DDHRESULT( 537 )
#define DDERR_WASSTILLDRAWING			MAKE_DDHRESULT( 540 )
#define DDERR_XALIGN				MAKE_DDHRESULT( 560 )
#define DDERR_INVALIDDIRECTDRAWGUID		MAKE_DDHRESULT( 561 )
#define DDERR_DIRECTDRAWALREADYCREATED		MAKE_DDHRESULT( 562 )
#define DDERR_NODIRECTDRAWHW			MAKE_DDHRESULT( 563 )
#define DDERR_PRIMARYSURFACEALREADYEXISTS	MAKE_DDHRESULT( 564 )
#define DDERR_NOEMULATION			MAKE_DDHRESULT( 565 )
#define DDERR_REGIONTOOSMALL			MAKE_DDHRESULT( 566 )
#define DDERR_CLIPPERISUSINGHWND		MAKE_DDHRESULT( 567 )
#define DDERR_NOCLIPPERATTACHED			MAKE_DDHRESULT( 568 )
#define DDERR_NOHWND				MAKE_DDHRESULT( 569 )
#define DDERR_HWNDSUBCLASSED			MAKE_DDHRESULT( 570 )
#define DDERR_HWNDALREADYSET			MAKE_DDHRESULT( 571 )
#define DDERR_NOPALETTEATTACHED			MAKE_DDHRESULT( 572 )
#define DDERR_NOPALETTEHW			MAKE_DDHRESULT( 573 )
#define DDERR_BLTFASTCANTCLIP			MAKE_DDHRESULT( 574 )
#define DDERR_NOBLTHW				MAKE_DDHRESULT( 575 )
#define DDERR_NODDROPSHW			MAKE_DDHRESULT( 576 )
#define DDERR_OVERLAYNOTVISIBLE			MAKE_DDHRESULT( 577 )
#define DDERR_NOOVERLAYDEST			MAKE_DDHRESULT( 578 )
#define DDERR_INVALIDPOSITION			MAKE_DDHRESULT( 579 )
#define DDERR_NOTAOVERLAYSURFACE		MAKE_DDHRESULT( 580 )
#define DDERR_EXCLUSIVEMODEALREADYSET		MAKE_DDHRESULT( 581 )
#define DDERR_NOTFLIPPABLE			MAKE_DDHRESULT( 582 )
#define DDERR_CANTDUPLICATE			MAKE_DDHRESULT( 583 )
#define DDERR_NOTLOCKED				MAKE_DDHRESULT( 584 )
#define DDERR_CANTCREATEDC			MAKE_DDHRESULT( 585 )
#define DDERR_NODC				MAKE_DDHRESULT( 586 )
#define DDERR_WRONGMODE				MAKE_DDHRESULT( 587 )
#define DDERR_IMPLICITLYCREATED			MAKE_DDHRESULT( 588 )
#define DDERR_NOTPALETTIZED			MAKE_DDHRESULT( 589 )
#define DDERR_UNSUPPORTEDMODE			MAKE_DDHRESULT( 590 )
#define DDERR_NOMIPMAPHW			MAKE_DDHRESULT( 591 )
#define DDERR_INVALIDSURFACETYPE		MAKE_DDHRESULT( 592 )
#define DDERR_NOOPTIMIZEHW			MAKE_DDHRESULT( 600 )
#define DDERR_NOTLOADED				MAKE_DDHRESULT( 601 )
#define DDERR_NOFOCUSWINDOW			MAKE_DDHRESULT( 602 )
#define DDERR_DCALREADYCREATED			MAKE_DDHRESULT( 620 )
#define DDERR_NONONLOCALVIDMEM			MAKE_DDHRESULT( 630 )
#define DDERR_CANTPAGELOCK			MAKE_DDHRESULT( 640 )
#define DDERR_CANTPAGEUNLOCK			MAKE_DDHRESULT( 660 )
#define DDERR_NOTPAGELOCKED			MAKE_DDHRESULT( 680 )
#define DDERR_MOREDATA				MAKE_DDHRESULT( 690 )
#define DDERR_VIDEONOTACTIVE			MAKE_DDHRESULT( 695 )
#define DDERR_DEVICEDOESNTOWNSURFACE		MAKE_DDHRESULT( 699 )
#define DDERR_NOTINITIALIZED			CO_E_NOTINITIALIZED

/* dwFlags for Blt* */
#define DDBLT_ALPHADEST				0x00000001
#define DDBLT_ALPHADESTCONSTOVERRIDE		0x00000002
#define DDBLT_ALPHADESTNEG			0x00000004
#define DDBLT_ALPHADESTSURFACEOVERRIDE		0x00000008
#define DDBLT_ALPHAEDGEBLEND			0x00000010
#define DDBLT_ALPHASRC				0x00000020
#define DDBLT_ALPHASRCCONSTOVERRIDE		0x00000040
#define DDBLT_ALPHASRCNEG			0x00000080
#define DDBLT_ALPHASRCSURFACEOVERRIDE		0x00000100
#define DDBLT_ASYNC				0x00000200
#define DDBLT_COLORFILL				0x00000400
#define DDBLT_DDFX				0x00000800
#define DDBLT_DDROPS				0x00001000
#define DDBLT_KEYDEST				0x00002000
#define DDBLT_KEYDESTOVERRIDE			0x00004000
#define DDBLT_KEYSRC				0x00008000
#define DDBLT_KEYSRCOVERRIDE			0x00010000
#define DDBLT_ROP				0x00020000
#define DDBLT_ROTATIONANGLE			0x00040000
#define DDBLT_ZBUFFER				0x00080000
#define DDBLT_ZBUFFERDESTCONSTOVERRIDE		0x00100000
#define DDBLT_ZBUFFERDESTOVERRIDE		0x00200000
#define DDBLT_ZBUFFERSRCCONSTOVERRIDE		0x00400000
#define DDBLT_ZBUFFERSRCOVERRIDE		0x00800000
#define DDBLT_WAIT				0x01000000
#define DDBLT_DEPTHFILL				0x02000000

/* dwTrans for BltFast */
#define DDBLTFAST_NOCOLORKEY			0x00000000
#define DDBLTFAST_SRCCOLORKEY			0x00000001
#define DDBLTFAST_DESTCOLORKEY			0x00000002
#define DDBLTFAST_WAIT				0x00000010

/* dwFlags for Flip */
#define DDFLIP_WAIT	0x00000001
#define DDFLIP_EVEN	0x00000002 /* only valid for overlay */
#define DDFLIP_ODD	0x00000004 /* only valid for overlay */

/* dwFlags for GetBltStatus */
#define DDGBS_CANBLT				0x00000001
#define DDGBS_ISBLTDONE				0x00000002

/* DDSCAPS.dwCaps */
/* reserved1, was 3d capable */
#define DDSCAPS_RESERVED1		0x00000001
/* surface contains alpha information */
#define DDSCAPS_ALPHA			0x00000002
/* this surface is a backbuffer */
#define DDSCAPS_BACKBUFFER		0x00000004
/* complex surface structure */
#define DDSCAPS_COMPLEX			0x00000008
/* part of surface flipping structure */
#define DDSCAPS_FLIP			0x00000010
/* this surface is the frontbuffer surface */
#define DDSCAPS_FRONTBUFFER		0x00000020
/* this is a plain offscreen surface */
#define DDSCAPS_OFFSCREENPLAIN		0x00000040
/* overlay */
#define DDSCAPS_OVERLAY			0x00000080
/* palette objects can be created and attached to us */
#define DDSCAPS_PALETTE			0x00000100
/* primary surface (the one the user looks at currently)(right eye)*/
#define DDSCAPS_PRIMARYSURFACE		0x00000200
/* primary surface for left eye */
#define DDSCAPS_PRIMARYSURFACELEFT	0x00000400
/* surface exists in systemmemory */
#define DDSCAPS_SYSTEMMEMORY		0x00000800
/* surface can be used as a texture */
#define DDSCAPS_TEXTURE		        0x00001000
/* surface may be destination for 3d rendering */
#define DDSCAPS_3DDEVICE		0x00002000
/* surface exists in videomemory */
#define DDSCAPS_VIDEOMEMORY		0x00004000
/* surface changes immediately visible */
#define DDSCAPS_VISIBLE			0x00008000
/* write only surface */
#define DDSCAPS_WRITEONLY		0x00010000
/* zbuffer surface */
#define DDSCAPS_ZBUFFER			0x00020000
/* has its own DC */
#define DDSCAPS_OWNDC			0x00040000
/* surface should be able to receive live video */
#define DDSCAPS_LIVEVIDEO		0x00080000
/* should be able to have a hw codec decompress stuff into it */
#define DDSCAPS_HWCODEC			0x00100000
/* mode X (320x200 or 320x240) surface */
#define DDSCAPS_MODEX			0x00200000
/* one mipmap surface (1 level) */
#define DDSCAPS_MIPMAP			0x00400000
#define DDSCAPS_RESERVED2		0x00800000
/* memory allocation delayed until Load() */
#define DDSCAPS_ALLOCONLOAD		0x04000000
/* Indicates that the surface will recieve data from a video port */
#define DDSCAPS_VIDEOPORT		0x08000000
/* surface is in local videomemory */
#define DDSCAPS_LOCALVIDMEM		0x10000000
/* surface is in nonlocal videomemory */
#define DDSCAPS_NONLOCALVIDMEM		0x20000000
/* surface is a standard VGA mode surface (NOT ModeX) */
#define DDSCAPS_STANDARDVGAMODE		0x40000000
/* optimized? surface */
#define DDSCAPS_OPTIMIZED		0x80000000

typedef struct _DDSCAPS {
	DWORD	dwCaps;	/* capabilities of surface wanted */
} DDSCAPS,*LPDDSCAPS;

#define	DD_ROP_SPACE	(256/32)	/* space required to store ROP array */

typedef struct _DDCAPS_DX3
{
    DWORD	dwSize;                 /* size of the DDDRIVERCAPS structure */
    DWORD	dwCaps;                 /* driver specific capabilities */
    DWORD	dwCaps2;                /* more driver specific capabilites */
    DWORD	dwCKeyCaps;             /* color key capabilities of the surface */
    DWORD	dwFXCaps;               /* driver specific stretching and effects capabilites */
    DWORD	dwFXAlphaCaps;          /* alpha driver specific capabilities */
    DWORD	dwPalCaps;              /* palette capabilities */
    DWORD	dwSVCaps;               /* stereo vision capabilities */
    DWORD	dwAlphaBltConstBitDepths;       /* DDBD_2,4,8 */
    DWORD	dwAlphaBltPixelBitDepths;       /* DDBD_1,2,4,8 */
    DWORD	dwAlphaBltSurfaceBitDepths;     /* DDBD_1,2,4,8 */
    DWORD	dwAlphaOverlayConstBitDepths;   /* DDBD_2,4,8 */
    DWORD	dwAlphaOverlayPixelBitDepths;   /* DDBD_1,2,4,8 */
    DWORD	dwAlphaOverlaySurfaceBitDepths; /* DDBD_1,2,4,8 */
    DWORD	dwZBufferBitDepths;             /* DDBD_8,16,24,32 */
    DWORD	dwVidMemTotal;          /* total amount of video memory */
    DWORD	dwVidMemFree;           /* amount of free video memory */
    DWORD	dwMaxVisibleOverlays;   /* maximum number of visible overlays */
    DWORD	dwCurrVisibleOverlays;  /* current number of visible overlays */
    DWORD	dwNumFourCCCodes;       /* number of four cc codes */
    DWORD	dwAlignBoundarySrc;     /* source rectangle alignment */
    DWORD	dwAlignSizeSrc;         /* source rectangle byte size */
    DWORD	dwAlignBoundaryDest;    /* dest rectangle alignment */
    DWORD	dwAlignSizeDest;        /* dest rectangle byte size */
    DWORD	dwAlignStrideAlign;     /* stride alignment */
    DWORD	dwRops[DD_ROP_SPACE];   /* ROPS supported */
    DDSCAPS	ddsCaps;                /* DDSCAPS structure has all the general capabilities */
    DWORD	dwMinOverlayStretch;    /* minimum overlay stretch factor multiplied by 1000, eg 1000 == 1.0, 1300 == 1.3 */
    DWORD	dwMaxOverlayStretch;    /* maximum overlay stretch factor multiplied by 1000, eg 1000 == 1.0, 1300 == 1.3 */
    DWORD	dwMinLiveVideoStretch;  /* minimum live video stretch factor multiplied by 1000, eg 1000 == 1.0, 1300 == 1.3 */
    DWORD	dwMaxLiveVideoStretch;  /* maximum live video stretch factor multiplied by 1000, eg 1000 == 1.0, 1300 == 1.3 */
    DWORD	dwMinHwCodecStretch;    /* minimum hardware codec stretch factor multiplied by 1000, eg 1000 == 1.0, 1300 == 1.3 */
    DWORD	dwMaxHwCodecStretch;    /* maximum hardware codec stretch factor multiplied by 1000, eg 1000 == 1.0, 1300 == 1.3 */
    DWORD	dwReserved1;
    DWORD	dwReserved2;
    DWORD	dwReserved3;
    DWORD	dwSVBCaps;              /* driver specific capabilities for System->Vmem blts */
    DWORD	dwSVBCKeyCaps;          /* driver color key capabilities for System->Vmem blts */
    DWORD	dwSVBFXCaps;            /* driver FX capabilities for System->Vmem blts */
    DWORD	dwSVBRops[DD_ROP_SPACE];/* ROPS supported for System->Vmem blts */
    DWORD	dwVSBCaps;              /* driver specific capabilities for Vmem->System blts */
    DWORD	dwVSBCKeyCaps;          /* driver color key capabilities for Vmem->System blts */
    DWORD	dwVSBFXCaps;            /* driver FX capabilities for Vmem->System blts */
    DWORD	dwVSBRops[DD_ROP_SPACE];/* ROPS supported for Vmem->System blts */
    DWORD	dwSSBCaps;              /* driver specific capabilities for System->System blts */
    DWORD	dwSSBCKeyCaps;          /* driver color key capabilities for System->System blts */
    DWORD	dwSSBFXCaps;            /* driver FX capabilities for System->System blts */
    DWORD	dwSSBRops[DD_ROP_SPACE];/* ROPS supported for System->System blts */
    DWORD	dwReserved4;
    DWORD	dwReserved5;
    DWORD	dwReserved6;
} DDCAPS_DX3,*LPDDCAPS_DX3;

typedef struct _DDCAPS
{
/*  0*/ DWORD  dwSize;			/* size of the DDDRIVERCAPS structure */
/*  4*/ DWORD  dwCaps;			/* driver specific capabilities */
/*  8*/ DWORD  dwCaps2;			/* more driver specific capabilites */
/*  c*/ DWORD  dwCKeyCaps;		/* color key capabilities of the surface */
/* 10*/ DWORD  dwFXCaps;		/* driver specific stretching and effects capabilites */
/* 14*/ DWORD  dwFXAlphaCaps;		/* alpha driver specific capabilities */
/* 18*/ DWORD  dwPalCaps;		/* palette capabilities */
/* 1c*/ DWORD  dwSVCaps;		/* stereo vision capabilities */
/* 20*/ DWORD  dwAlphaBltConstBitDepths;	/* DDBD_2,4,8 */
/* 24*/ DWORD  dwAlphaBltPixelBitDepths;	/* DDBD_1,2,4,8 */
/* 28*/ DWORD  dwAlphaBltSurfaceBitDepths;	/* DDBD_1,2,4,8 */
/* 2c*/ DWORD  dwAlphaOverlayConstBitDepths;	/* DDBD_2,4,8 */
/* 30*/ DWORD  dwAlphaOverlayPixelBitDepths;	/* DDBD_1,2,4,8 */
/* 34*/ DWORD  dwAlphaOverlaySurfaceBitDepths;	/* DDBD_1,2,4,8 */
/* 38*/ DWORD  dwZBufferBitDepths;		/* DDBD_8,16,24,32 */
/* 3c*/ DWORD  dwVidMemTotal;		/* total amount of video memory */
/* 40*/ DWORD  dwVidMemFree;		/* amount of free video memory */
/* 44*/ DWORD  dwMaxVisibleOverlays;	/* maximum number of visible overlays */
/* 48*/ DWORD  dwCurrVisibleOverlays;	/* current number of visible overlays */
/* 4c*/ DWORD  dwNumFourCCCodes;	/* number of four cc codes */
/* 50*/ DWORD  dwAlignBoundarySrc;	/* source rectangle alignment */
/* 54*/ DWORD  dwAlignSizeSrc;		/* source rectangle byte size */
/* 58*/ DWORD  dwAlignBoundaryDest;	/* dest rectangle alignment */
/* 5c*/ DWORD  dwAlignSizeDest;		/* dest rectangle byte size */
/* 60*/ DWORD  dwAlignStrideAlign;	/* stride alignment */
/* 64*/ DWORD  dwRops[DD_ROP_SPACE];	/* ROPS supported */
/* 84*/ DDSCAPS ddsCaps;		/* DDSCAPS structure has all the general capabilities */
/* 88*/ DWORD  dwMinOverlayStretch;	/* minimum overlay stretch factor multiplied by 1000, eg 1000 == 1.0, 1300 == 1.3 */
/* 8c*/ DWORD  dwMaxOverlayStretch;	/* maximum overlay stretch factor multiplied by 1000, eg 1000 == 1.0, 1300 == 1.3 */
/* 90*/ DWORD  dwMinLiveVideoStretch;	/* minimum live video stretch factor multiplied by 1000, eg 1000 == 1.0, 1300 == 1.3 */
/* 94*/ DWORD  dwMaxLiveVideoStretch;	/* maximum live video stretch factor multiplied by 1000, eg 1000 == 1.0, 1300 == 1.3 */
/* 98*/ DWORD  dwMinHwCodecStretch;	/* minimum hardware codec stretch factor multiplied by 1000, eg 1000 == 1.0, 1300 == 1.3 */
/* 9c*/ DWORD  dwMaxHwCodecStretch;	/* maximum hardware codec stretch factor multiplied by 1000, eg 1000 == 1.0, 1300 == 1.3 */
/* a0*/ DWORD  dwReserved1;
/* a4*/ DWORD  dwReserved2;
/* a8*/ DWORD  dwReserved3;
/* ac*/ DWORD  dwSVBCaps;	/* driver specific capabilities for System->Vmem blts */
/* b0*/ DWORD  dwSVBCKeyCaps;	/* driver color key capabilities for System->Vmem blts */
/* b4*/ DWORD  dwSVBFXCaps;	/* driver FX capabilities for System->Vmem blts */
/* b8*/ DWORD  dwSVBRops[DD_ROP_SPACE];/* ROPS supported for System->Vmem blts */
/* d8*/ DWORD  dwVSBCaps;	/* driver specific capabilities for Vmem->System blts */
/* dc*/ DWORD  dwVSBCKeyCaps;	/* driver color key capabilities for Vmem->System blts */
/* e0*/ DWORD  dwVSBFXCaps;	/* driver FX capabilities for Vmem->System blts */
/* e4*/ DWORD  dwVSBRops[DD_ROP_SPACE];/* ROPS supported for Vmem->System blts */
/*104*/ DWORD  dwSSBCaps;	/* driver specific capabilities for System->System blts */
/*108*/ DWORD  dwSSBCKeyCaps;	/* driver color key capabilities for System->System blts */
/*10c*/ DWORD  dwSSBFXCaps;	/* driver FX capabilities for System->System blts */
/*110*/ DWORD  dwSSBRops[DD_ROP_SPACE];/* ROPS supported for System->System blts */
#if       DIRECTDRAW_VERSION >= 0x0500
/*130*/ DWORD  dwMaxVideoPorts;	/* maximum number of usable video ports */
/*134*/ DWORD  dwCurrVideoPorts;/* current number of video ports used */
/*138*/ DWORD  dwSVBCaps2;	/* more driver specific capabilities for System->Vmem blts */
/*13c*/ DWORD  dwNLVBCaps;	/* driver specific capabilities for non-local->local vidmem blts */
/*140*/ DWORD  dwNLVBCaps2;	/* more driver specific capabilities non-local->local vidmem blts */
/*144*/ DWORD  dwNLVBCKeyCaps;	/* driver color key capabilities for non-local->local vidmem blts */
/*148*/ DWORD  dwNLVBFXCaps;	/* driver FX capabilities for non-local->local blts */
/*14c*/ DWORD  dwNLVBRops[DD_ROP_SPACE];/* ROPS supported for non-local->local blts */
#else  /* DIRECTDRAW_VERSION >= 0x0500 */
/*130*/ DWORD  dwReserved4;
/*134*/ DWORD  dwReserved5;
/*138*/ DWORD  dwReserved6;
#endif /* DIRECTDRAW_VERSION >= 0x0500 */
} DDCAPS,*LPDDCAPS;


/* DDCAPS.dwCaps */
#define DDCAPS_3D			0x00000001
#define DDCAPS_ALIGNBOUNDARYDEST	0x00000002
#define DDCAPS_ALIGNSIZEDEST		0x00000004
#define DDCAPS_ALIGNBOUNDARYSRC		0x00000008
#define DDCAPS_ALIGNSIZESRC		0x00000010
#define DDCAPS_ALIGNSTRIDE		0x00000020
#define DDCAPS_BLT			0x00000040
#define DDCAPS_BLTQUEUE			0x00000080
#define DDCAPS_BLTFOURCC		0x00000100
#define DDCAPS_BLTSTRETCH		0x00000200
#define DDCAPS_GDI			0x00000400
#define DDCAPS_OVERLAY			0x00000800
#define DDCAPS_OVERLAYCANTCLIP		0x00001000
#define DDCAPS_OVERLAYFOURCC		0x00002000
#define DDCAPS_OVERLAYSTRETCH		0x00004000
#define DDCAPS_PALETTE			0x00008000
#define DDCAPS_PALETTEVSYNC		0x00010000
#define DDCAPS_READSCANLINE		0x00020000
#define DDCAPS_STEREOVIEW		0x00040000
#define DDCAPS_VBI			0x00080000
#define DDCAPS_ZBLTS			0x00100000
#define DDCAPS_ZOVERLAYS		0x00200000
#define DDCAPS_COLORKEY			0x00400000
#define DDCAPS_ALPHA			0x00800000
#define DDCAPS_COLORKEYHWASSIST		0x01000000
#define DDCAPS_NOHARDWARE		0x02000000
#define DDCAPS_BLTCOLORFILL		0x04000000
#define DDCAPS_BANKSWITCHED		0x08000000
#define DDCAPS_BLTDEPTHFILL		0x10000000
#define DDCAPS_CANCLIP			0x20000000
#define DDCAPS_CANCLIPSTRETCHED		0x40000000
#define DDCAPS_CANBLTSYSMEM		0x80000000

/* DDCAPS.dwCaps2 */
#define DDCAPS2_CERTIFIED		0x00000001
#define DDCAPS2_NO2DDURING3DSCENE       0x00000002
#define DDCAPS2_VIDEOPORT		0x00000004
#define DDCAPS2_AUTOFLIPOVERLAY		0x00000008
#define DDCAPS2_CANBOBINTERLEAVED	0x00000010
#define DDCAPS2_CANBOBNONINTERLEAVED	0x00000020
#define DDCAPS2_COLORCONTROLOVERLAY	0x00000040
#define DDCAPS2_COLORCONTROLPRIMARY	0x00000080
#define DDCAPS2_CANDROPZ16BIT		0x00000100
#define DDCAPS2_NONLOCALVIDMEM		0x00000200
#define DDCAPS2_NONLOCALVIDMEMCAPS	0x00000400
#define DDCAPS2_NOPAGELOCKREQUIRED	0x00000800
#define DDCAPS2_WIDESURFACES		0x00001000
#define DDCAPS2_CANFLIPODDEVEN		0x00002000
#define DDCAPS2_CANBOBHARDWARE		0x00004000


/* Set/Get Colour Key Flags */
#define DDCKEY_COLORSPACE  0x00000001  /* Struct is single colour space */
#define DDCKEY_DESTBLT     0x00000002  /* To be used as dest for blt */
#define DDCKEY_DESTOVERLAY 0x00000004  /* To be used as dest for CK overlays */
#define DDCKEY_SRCBLT      0x00000008  /* To be used as src for blt */
#define DDCKEY_SRCOVERLAY  0x00000010  /* To be used as src for CK overlays */

typedef struct _DDCOLORKEY
{
	DWORD	dwColorSpaceLowValue;/* low boundary of color space that is to
                                      * be treated as Color Key, inclusive
				      */
	DWORD	dwColorSpaceHighValue;/* high boundary of color space that is
                                       * to be treated as Color Key, inclusive
				       */
} DDCOLORKEY,*LPDDCOLORKEY;

/* ddCKEYCAPS bits */
#define DDCKEYCAPS_DESTBLT			0x00000001
#define DDCKEYCAPS_DESTBLTCLRSPACE		0x00000002
#define DDCKEYCAPS_DESTBLTCLRSPACEYUV		0x00000004
#define DDCKEYCAPS_DESTBLTYUV			0x00000008
#define DDCKEYCAPS_DESTOVERLAY			0x00000010
#define DDCKEYCAPS_DESTOVERLAYCLRSPACE		0x00000020
#define DDCKEYCAPS_DESTOVERLAYCLRSPACEYUV	0x00000040
#define DDCKEYCAPS_DESTOVERLAYONEACTIVE		0x00000080
#define DDCKEYCAPS_DESTOVERLAYYUV		0x00000100
#define DDCKEYCAPS_SRCBLT			0x00000200
#define DDCKEYCAPS_SRCBLTCLRSPACE		0x00000400
#define DDCKEYCAPS_SRCBLTCLRSPACEYUV		0x00000800
#define DDCKEYCAPS_SRCBLTYUV			0x00001000
#define DDCKEYCAPS_SRCOVERLAY			0x00002000
#define DDCKEYCAPS_SRCOVERLAYCLRSPACE		0x00004000
#define DDCKEYCAPS_SRCOVERLAYCLRSPACEYUV	0x00008000
#define DDCKEYCAPS_SRCOVERLAYONEACTIVE		0x00010000
#define DDCKEYCAPS_SRCOVERLAYYUV		0x00020000
#define DDCKEYCAPS_NOCOSTOVERLAY		0x00040000

typedef struct _DDPIXELFORMAT {
    DWORD	dwSize;                 /* 0: size of structure */
    DWORD	dwFlags;                /* 4: pixel format flags */
    DWORD	dwFourCC;               /* 8: (FOURCC code) */
    union {
	DWORD	dwRGBBitCount;          /* C: how many bits per pixel */
	DWORD	dwYUVBitCount;          /* C: how many bits per pixel */
	DWORD	dwZBufferBitDepth;      /* C: how many bits for z buffers */
	DWORD	dwAlphaBitDepth;        /* C: how many bits for alpha channels*/
    } x;
    union {
	DWORD	dwRBitMask;             /* 10: mask for red bit*/
	DWORD	dwYBitMask;             /* 10: mask for Y bits*/
    } y;
    union {
	DWORD	dwGBitMask;             /* 14: mask for green bits*/
	DWORD	dwUBitMask;             /* 14: mask for U bits*/
    } z;
    union {
	DWORD   dwBBitMask;             /* 18: mask for blue bits*/
	DWORD   dwVBitMask;             /* 18: mask for V bits*/
    } xx;
    union {
    	DWORD	dwRGBAlphaBitMask;	/* 1C: mask for alpha channel */
    	DWORD	dwYUVAlphaBitMask;	/* 1C: mask for alpha channel */
	DWORD	dwRGBZBitMask;		/* 1C: mask for Z channel */
	DWORD	dwYUVZBitMask;		/* 1C: mask for Z channel */
    } xy;
    					/* 20: next structure */
} DDPIXELFORMAT,*LPDDPIXELFORMAT;

/* DDCAPS.dwFXCaps */
#define DDFXCAPS_BLTARITHSTRETCHY	0x00000020
#define DDFXCAPS_BLTARITHSTRETCHYN	0x00000010
#define DDFXCAPS_BLTMIRRORLEFTRIGHT	0x00000040
#define DDFXCAPS_BLTMIRRORUPDOWN	0x00000080
#define DDFXCAPS_BLTROTATION		0x00000100
#define DDFXCAPS_BLTROTATION90		0x00000200
#define DDFXCAPS_BLTSHRINKX		0x00000400
#define DDFXCAPS_BLTSHRINKXN		0x00000800
#define DDFXCAPS_BLTSHRINKY		0x00001000
#define DDFXCAPS_BLTSHRINKYN		0x00002000
#define DDFXCAPS_BLTSTRETCHX		0x00004000
#define DDFXCAPS_BLTSTRETCHXN		0x00008000
#define DDFXCAPS_BLTSTRETCHY		0x00010000
#define DDFXCAPS_BLTSTRETCHYN		0x00020000
#define DDFXCAPS_OVERLAYARITHSTRETCHY	0x00040000
#define DDFXCAPS_OVERLAYARITHSTRETCHYN	0x00000008
#define DDFXCAPS_OVERLAYSHRINKX		0x00080000
#define DDFXCAPS_OVERLAYSHRINKXN	0x00100000
#define DDFXCAPS_OVERLAYSHRINKY		0x00200000
#define DDFXCAPS_OVERLAYSHRINKYN	0x00400000
#define DDFXCAPS_OVERLAYSTRETCHX	0x00800000
#define DDFXCAPS_OVERLAYSTRETCHXN	0x01000000
#define DDFXCAPS_OVERLAYSTRETCHY	0x02000000
#define DDFXCAPS_OVERLAYSTRETCHYN	0x04000000
#define DDFXCAPS_OVERLAYMIRRORLEFTRIGHT	0x08000000
#define DDFXCAPS_OVERLAYMIRRORUPDOWN	0x10000000

/* DDCAPS.dwFXAlphaCaps */
#define DDFXALPHACAPS_BLTALPHAEDGEBLEND		0x00000001
#define DDFXALPHACAPS_BLTALPHAPIXELS		0x00000002
#define DDFXALPHACAPS_BLTALPHAPIXELSNEG		0x00000004
#define DDFXALPHACAPS_BLTALPHASURFACES		0x00000008
#define DDFXALPHACAPS_BLTALPHASURFACESNEG	0x00000010
#define DDFXALPHACAPS_OVERLAYALPHAEDGEBLEND	0x00000020
#define DDFXALPHACAPS_OVERLAYALPHAPIXELS	0x00000040
#define DDFXALPHACAPS_OVERLAYALPHAPIXELSNEG	0x00000080
#define DDFXALPHACAPS_OVERLAYALPHASURFACES	0x00000100
#define DDFXALPHACAPS_OVERLAYALPHASURFACESNEG	0x00000200

/* DDCAPS.dwPalCaps */
#define DDPCAPS_4BIT			0x00000001
#define DDPCAPS_8BITENTRIES		0x00000002
#define DDPCAPS_8BIT			0x00000004
#define DDPCAPS_INITIALIZE		0x00000008
#define DDPCAPS_PRIMARYSURFACE		0x00000010
#define DDPCAPS_PRIMARYSURFACELEFT	0x00000020
#define DDPCAPS_ALLOW256		0x00000040
#define DDPCAPS_VSYNC			0x00000080
#define DDPCAPS_1BIT			0x00000100
#define DDPCAPS_2BIT			0x00000200
#define DDPCAPS_ALPHA                   0x00000400

/* DDCAPS.dwSVCaps */
#define DDSVCAPS_ENIGMA			0x00000001l
#define DDSVCAPS_FLICKER		0x00000002l
#define DDSVCAPS_REDBLUE		0x00000004l
#define DDSVCAPS_SPLIT			0x00000008l

/* BitDepths */
#define DDBD_1				0x00004000
#define DDBD_2				0x00002000
#define DDBD_4				0x00001000
#define DDBD_8				0x00000800
#define DDBD_16				0x00000400
#define DDBD_24				0x00000200
#define DDBD_32				0x00000100

/* DDOVERLAYFX.dwDDFX */
#define DDOVERFX_ARITHSTRETCHY		0x00000001
#define DDOVERFX_MIRRORLEFTRIGHT	0x00000002
#define DDOVERFX_MIRRORUPDOWN		0x00000004

/* DDCOLORKEY.dwFlags */
#define DDPF_ALPHAPIXELS		0x00000001
#define DDPF_ALPHA			0x00000002
#define DDPF_FOURCC			0x00000004
#define DDPF_PALETTEINDEXED4		0x00000008
#define DDPF_PALETTEINDEXEDTO8		0x00000010
#define DDPF_PALETTEINDEXED8		0x00000020
#define DDPF_RGB			0x00000040
#define DDPF_COMPRESSED			0x00000080
#define DDPF_RGBTOYUV			0x00000100
#define DDPF_YUV			0x00000200
#define DDPF_ZBUFFER			0x00000400
#define DDPF_PALETTEINDEXED1		0x00000800
#define DDPF_PALETTEINDEXED2		0x00001000
#define DDPF_ZPIXELS			0x00002000

/* SetCooperativeLevel dwFlags */
#define DDSCL_FULLSCREEN		0x00000001
#define DDSCL_ALLOWREBOOT		0x00000002
#define DDSCL_NOWINDOWCHANGES		0x00000004
#define DDSCL_NORMAL			0x00000008
#define DDSCL_EXCLUSIVE			0x00000010
#define DDSCL_ALLOWMODEX		0x00000040
#define DDSCL_SETFOCUSWINDOW		0x00000080
#define DDSCL_SETDEVICEWINDOW		0x00000100
#define DDSCL_CREATEDEVICEWINDOW	0x00000200


/* DDSURFACEDESC.dwFlags */
#define	DDSD_CAPS		0x00000001
#define	DDSD_HEIGHT		0x00000002
#define	DDSD_WIDTH		0x00000004
#define	DDSD_PITCH		0x00000008
#define	DDSD_BACKBUFFERCOUNT	0x00000020
#define	DDSD_ZBUFFERBITDEPTH	0x00000040
#define	DDSD_ALPHABITDEPTH	0x00000080
#define	DDSD_LPSURFACE		0x00000800
#define	DDSD_PIXELFORMAT	0x00001000
#define	DDSD_CKDESTOVERLAY	0x00002000
#define	DDSD_CKDESTBLT		0x00004000
#define	DDSD_CKSRCOVERLAY	0x00008000
#define	DDSD_CKSRCBLT		0x00010000
#define	DDSD_MIPMAPCOUNT	0x00020000
#define	DDSD_REFRESHRATE	0x00040000
#define	DDSD_LINEARSIZE		0x00080000
#define	DDSD_ALL		0x000ff9ee

/* EnumSurfaces flags */
#define DDENUMSURFACES_ALL          0x00000001
#define DDENUMSURFACES_MATCH        0x00000002
#define DDENUMSURFACES_NOMATCH      0x00000004
#define DDENUMSURFACES_CANBECREATED 0x00000008
#define DDENUMSURFACES_DOESEXIST    0x00000010

/* SetDisplayMode flags */
#define DDSDM_STANDARDVGAMODE	0x00000001

/* EnumDisplayModes flags */
#define DDEDM_REFRESHRATES	0x00000001
#define DDEDM_STANDARDVGAMODES	0x00000002


typedef struct _DDSURFACEDESC
{
	DWORD	dwSize;		/* 0: size of the DDSURFACEDESC structure*/
	DWORD	dwFlags;	/* 4: determines what fields are valid*/
	DWORD	dwHeight;	/* 8: height of surface to be created*/
	DWORD	dwWidth;	/* C: width of input surface*/
	LONG	lPitch;		/*10: distance to start of next line (return value only)*/
	DWORD	dwBackBufferCount;/* 14: number of back buffers requested*/
	union {
		DWORD	dwMipMapCount;/* 18:number of mip-map levels requested*/
		DWORD	dwZBufferBitDepth;/*18: depth of Z buffer requested*/
		DWORD	dwRefreshRate;/* 18:refresh rate (used when display mode is described)*/
	} x;		
	DWORD	dwAlphaBitDepth;/* 1C:depth of alpha buffer requested*/
	DWORD	dwReserved;	/* 20:reserved*/
	union {
		LPVOID	lpSurface;	/* 24:pointer to the associated surface memory*/
		DWORD	dwLinearSize;	/* 24:Formless late-allocated optimized surface size*/
	} y;
	DDCOLORKEY	ddckCKDestOverlay;/* 28: CK for dest overlay use*/
	DDCOLORKEY	ddckCKDestBlt;	/* 30: CK for destination blt use*/
	DDCOLORKEY	ddckCKSrcOverlay;/* 38: CK for source overlay use*/
	DDCOLORKEY	ddckCKSrcBlt;	/* 40: CK for source blt use*/
	DDPIXELFORMAT	ddpfPixelFormat;/* 48: pixel format description of the surface*/
	DDSCAPS		ddsCaps;	/* 68: direct draw surface caps */
} DDSURFACEDESC,*LPDDSURFACEDESC;

/* DDCOLORCONTROL.dwFlags */
#define DDCOLOR_BRIGHTNESS	0x00000001
#define DDCOLOR_CONTRAST	0x00000002
#define DDCOLOR_HUE		0x00000004
#define DDCOLOR_SATURATION	0x00000008
#define DDCOLOR_SHARPNESS	0x00000010
#define DDCOLOR_GAMMA		0x00000020
#define DDCOLOR_COLORENABLE	0x00000040

typedef struct {
	DWORD	dwSize;
	DWORD	dwFlags;
	LONG	lBrightness;
	LONG	lContrast;
	LONG	lHue;
	LONG	lSaturation;
	LONG	lSharpness;
	LONG	lGamma;
	LONG	lColorEnable;
	DWORD	dwReserved1;
} DDCOLORCONTROL,*LPDDCOLORCONTROL;

typedef BOOL (CALLBACK * LPDDENUMCALLBACKA)(GUID *, LPSTR, LPSTR, LPVOID);
typedef BOOL (CALLBACK * LPDDENUMCALLBACKW)(GUID *, LPWSTR, LPWSTR, LPVOID);
DECL_WINELIB_TYPE_AW(LPDDENUMCALLBACK)

typedef HRESULT (CALLBACK * LPDDENUMMODESCALLBACK)(LPDDSURFACEDESC, LPVOID);
typedef HRESULT (CALLBACK * LPDDENUMSURFACESCALLBACK)(LPDIRECTDRAWSURFACE, LPDDSURFACEDESC, LPVOID);

typedef BOOL (CALLBACK * LPDDENUMCALLBACKEXA)(GUID *, LPSTR, LPSTR, LPVOID, HMONITOR);
typedef BOOL (CALLBACK * LPDDENUMCALLBACKEXW)(GUID *, LPWSTR, LPWSTR, LPVOID, HMONITOR);
DECL_WINELIB_TYPE_AW(LPDDENUMCALLBACKEX)

HRESULT WINAPI DirectDrawEnumerateExA( LPDDENUMCALLBACKEXA lpCallback, LPVOID lpContext, DWORD dwFlags);
HRESULT WINAPI DirectDrawEnumerateExW( LPDDENUMCALLBACKEXW lpCallback, LPVOID lpContext, DWORD dwFlags);

/* flags for DirectDrawEnumerateEx */
#define DDENUM_ATTACHEDSECONDARYDEVICES	0x00000001
#define DDENUM_DETACHEDSECONDARYDEVICES	0x00000002
#define DDENUM_NONDISPLAYDEVICES	0x00000004

typedef struct _DDBLTFX
{
    DWORD       dwSize;                         /* size of structure */
    DWORD       dwDDFX;                         /* FX operations */
    DWORD       dwROP;                          /* Win32 raster operations */
    DWORD       dwDDROP;                        /* Raster operations new for DirectDraw */
    DWORD       dwRotationAngle;                /* Rotation angle for blt */
    DWORD       dwZBufferOpCode;                /* ZBuffer compares */
    DWORD       dwZBufferLow;                   /* Low limit of Z buffer */
    DWORD       dwZBufferHigh;                  /* High limit of Z buffer */
    DWORD       dwZBufferBaseDest;              /* Destination base value */
    DWORD       dwZDestConstBitDepth;           /* Bit depth used to specify Z constant for destination */
    union
    {
        DWORD   dwZDestConst;                   /* Constant to use as Z buffer for dest */
        LPDIRECTDRAWSURFACE lpDDSZBufferDest;   /* Surface to use as Z buffer for dest */
    } x;
    DWORD       dwZSrcConstBitDepth;            /* Bit depth used to specify Z constant for source */
    union
    {
        DWORD   dwZSrcConst;                    /* Constant to use as Z buffer for src */
        LPDIRECTDRAWSURFACE lpDDSZBufferSrc;    /* Surface to use as Z buffer for src */
    } y;
    DWORD       dwAlphaEdgeBlendBitDepth;       /* Bit depth used to specify constant for alpha edge blend */
    DWORD       dwAlphaEdgeBlend;               /* Alpha for edge blending */
    DWORD       dwReserved;
    DWORD       dwAlphaDestConstBitDepth;       /* Bit depth used to specify alpha constant for destination */
    union
    {
        DWORD   dwAlphaDestConst;               /* Constant to use as Alpha Channel */
        LPDIRECTDRAWSURFACE lpDDSAlphaDest;     /* Surface to use as Alpha Channel */
    } z;
    DWORD       dwAlphaSrcConstBitDepth;        /* Bit depth used to specify alpha constant for source */
    union
    {
        DWORD   dwAlphaSrcConst;                /* Constant to use as Alpha Channel */
        LPDIRECTDRAWSURFACE lpDDSAlphaSrc;      /* Surface to use as Alpha Channel */
    } a;
    union
    {
        DWORD   dwFillColor;                    /* color in RGB or Palettized */
        DWORD   dwFillDepth;                    /* depth value for z-buffer */
	DWORD   dwFillPixel;			/* pixel val for RGBA or RGBZ */
        LPDIRECTDRAWSURFACE lpDDSPattern;       /* Surface to use as pattern */
    } b;
    DDCOLORKEY  ddckDestColorkey;               /* DestColorkey override */
    DDCOLORKEY  ddckSrcColorkey;                /* SrcColorkey override */
} DDBLTFX,*LPDDBLTFX;

/* dwDDFX */
/* arithmetic stretching along y axis */
#define DDBLTFX_ARITHSTRETCHY			0x00000001
/* mirror on y axis */
#define DDBLTFX_MIRRORLEFTRIGHT			0x00000002
/* mirror on x axis */
#define DDBLTFX_MIRRORUPDOWN			0x00000004
/* do not tear */
#define DDBLTFX_NOTEARING			0x00000008
/* 180 degrees clockwise rotation */
#define DDBLTFX_ROTATE180			0x00000010
/* 270 degrees clockwise rotation */
#define DDBLTFX_ROTATE270			0x00000020
/* 90 degrees clockwise rotation */
#define DDBLTFX_ROTATE90			0x00000040
/* dwZBufferLow and dwZBufferHigh specify limits to the copied Z values */
#define DDBLTFX_ZBUFFERRANGE			0x00000080
/* add dwZBufferBaseDest to every source z value before compare */
#define DDBLTFX_ZBUFFERBASEDEST			0x00000100

typedef struct _DDOVERLAYFX
{
    DWORD       dwSize;                         /* size of structure */
    DWORD       dwAlphaEdgeBlendBitDepth;       /* Bit depth used to specify constant for alpha edge blend */
    DWORD       dwAlphaEdgeBlend;               /* Constant to use as alpha for edge blend */
    DWORD       dwReserved;
    DWORD       dwAlphaDestConstBitDepth;       /* Bit depth used to specify alpha constant for destination */
    union
    {
        DWORD   dwAlphaDestConst;               /* Constant to use as alpha channel for dest */
        LPDIRECTDRAWSURFACE lpDDSAlphaDest;     /* Surface to use as alpha channel for dest */
    } x;
    DWORD       dwAlphaSrcConstBitDepth;        /* Bit depth used to specify alpha constant for source */
    union
    {
        DWORD   dwAlphaSrcConst;                /* Constant to use as alpha channel for src */
        LPDIRECTDRAWSURFACE lpDDSAlphaSrc;      /* Surface to use as alpha channel for src */
    } y;
    DDCOLORKEY  dckDestColorkey;                /* DestColorkey override */
    DDCOLORKEY  dckSrcColorkey;                 /* DestColorkey override */
    DWORD       dwDDFX;                         /* Overlay FX */
    DWORD       dwFlags;                        /* flags */
} DDOVERLAYFX,*LPDDOVERLAYFX;

typedef struct _DDBLTBATCH
{
    LPRECT		lprDest;
    LPDIRECTDRAWSURFACE	lpDDSSrc;
    LPRECT		lprSrc;
    DWORD		dwFlags;
    LPDDBLTFX		lpDDBltFx;
} DDBLTBATCH,*LPDDBLTBATCH;

#define MAX_DDDEVICEID_STRING          512

typedef struct tagDDDEVICEIDENTIFIER {
  char    szDriver[MAX_DDDEVICEID_STRING];
  char    szDescription[MAX_DDDEVICEID_STRING];
  LARGE_INTEGER  liDriverVersion;
  DWORD   dwVendorId;
  DWORD   dwDeviceId;
  DWORD   dwSubSysId;
  DWORD   dwRevision;
  GUID    guidDeviceIdentifier;
} DDDEVICEIDENTIFIER, * LPDDDEVICEIDENTIFIER;


/*****************************************************************************
 * IDirectDrawPalette interface
 */
#define ICOM_INTERFACE IDirectDrawPalette
#define IDirectDrawPalette_METHODS \
    ICOM_METHOD1(HRESULT,GetCaps,    LPDWORD,lpdwCaps) \
    ICOM_METHOD4(HRESULT,GetEntries, DWORD,dwFlags, DWORD,dwBase, DWORD,dwNumEntries, LPPALETTEENTRY,lpEntries) \
    ICOM_METHOD3(HRESULT,Initialize, LPDIRECTDRAW,lpDD, DWORD,dwFlags, LPPALETTEENTRY,lpDDColorTable) \
    ICOM_METHOD4(HRESULT,SetEntries, DWORD,dwFlags, DWORD,dwStartingEntry, DWORD,dwCount, LPPALETTEENTRY,lpEntries)
#define IDirectDrawPalette_IMETHODS \
    IUnknown_IMETHODS \
    IDirectDrawPalette_METHODS
ICOM_DEFINE(IDirectDrawPalette,IUnknown)
#undef ICOM_INTERFACE

#ifdef ICOM_CINTERFACE
    /*** IUnknown methods ***/
#define IDirectDrawPalette_QueryInterface(p,a,b) ICOM_CALL2(QueryInterface,p,a,b)
#define IDirectDrawPalette_AddRef(p)             ICOM_CALL (AddRef,p)
#define IDirectDrawPalette_Release(p)            ICOM_CALL (Release,p)
    /*** IDirectDrawPalette methods ***/
#define IDirectDrawPalette_GetCaps(p,a)          ICOM_CALL1(GetCaps,p,a)
#define IDirectDrawPalette_GetEntries(p,a,b,c,d) ICOM_CALL4(GetEntries,p,a,b,c,d)
#define IDirectDrawPalette_Initialize(p,a,b,c)   ICOM_CALL3(Initialize,p,a,b,c)
#define IDirectDrawPalette_SetEntries(p,a,b,c,d) ICOM_CALL4(SetEntries,p,a,b,c,d)
#endif


/*****************************************************************************
 * IDirectDrawClipper interface
 */
#define ICOM_INTERFACE IDirectDrawClipper
#define IDirectDrawClipper_METHODS \
    ICOM_METHOD3(HRESULT,GetClipList,       LPRECT,lpRect, LPRGNDATA,lpClipList, LPDWORD,lpdwSize) \
    ICOM_METHOD1(HRESULT,GetHWnd,           HWND*,lphWnd) \
    ICOM_METHOD2(HRESULT,Initialize,        LPDIRECTDRAW,lpDD, DWORD,dwFlags) \
    ICOM_METHOD1(HRESULT,IsClipListChanged, BOOL*,lpbChanged) \
    ICOM_METHOD2(HRESULT,SetClipList,       LPRGNDATA,lpClipList, DWORD,dwFlags) \
    ICOM_METHOD2(HRESULT,SetHWnd,           DWORD,dwFlags, HWND,hWnd)
#define IDirectDrawClipper_IMETHODS \
    IUnknown_IMETHODS \
    IDirectDrawClipper_METHODS
ICOM_DEFINE(IDirectDrawClipper,IUnknown)
#undef ICOM_INTERFACE

#ifdef ICOM_CINTERFACE
    /*** IUnknown methods ***/
#define IDirectDrawClipper_QueryInterface(p,a,b) ICOM_CALL2(QueryInterface,p,a,b)
#define IDirectDrawClipper_AddRef(p)             ICOM_CALL (AddRef,p)
#define IDirectDrawClipper_Release(p)            ICOM_CALL (Release,p)
    /*** IDirectDrawClipper methods ***/
#define IDirectDrawClipper_GetClipList(p,a,b,c)   ICOM_CALL3(GetClipList,p,a,b,c)
#define IDirectDrawClipper_GetHWnd(p,a)           ICOM_CALL1(GetHWnd,p,a)
#define IDirectDrawClipper_Initialize(p,a,b)      ICOM_CALL2(Initialize,p,a,b)
#define IDirectDrawClipper_IsClipListChanged(p,a) ICOM_CALL1(IsClipListChanged,p,a)
#define IDirectDrawClipper_SetClipList(p,a,b)     ICOM_CALL2(SetClipList,p,a,b)
#define IDirectDrawClipper_SetHWnd(p,a,b)         ICOM_CALL2(SetHWnd,p,a,b)
#endif


/*****************************************************************************
 * IDirectDraw interface
 */
#define ICOM_INTERFACE IDirectDraw
#define IDirectDraw_METHODS \
    ICOM_METHOD (HRESULT,Compact) \
    ICOM_METHOD3(HRESULT,CreateClipper,          DWORD,dwFlags, LPDIRECTDRAWCLIPPER*,lplpDDClipper, IUnknown*,pUnkOuter) \
    ICOM_METHOD4(HRESULT,CreatePalette,          DWORD,dwFlags, LPPALETTEENTRY,lpColorTable, LPDIRECTDRAWPALETTE*,lplpDDPalette, IUnknown*,pUnkOuter) \
    ICOM_METHOD3(HRESULT,CreateSurface,          LPDDSURFACEDESC,lpDDSurfaceDesc, LPDIRECTDRAWSURFACE*,lplpDDSurface, IUnknown*,pUnkOuter) \
    ICOM_METHOD2(HRESULT,DuplicateSurface,       LPDIRECTDRAWSURFACE,lpDDSurface, LPDIRECTDRAWSURFACE*,lplpDupDDSurface) \
    ICOM_METHOD4(HRESULT,EnumDisplayModes,       DWORD,dwFlags, LPDDSURFACEDESC,lpDDSurfaceDesc, LPVOID,lpContext, LPDDENUMMODESCALLBACK,lpEnumModesCallback) \
    ICOM_METHOD4(HRESULT,EnumSurfaces,           DWORD,dwFlags, LPDDSURFACEDESC,lpDDSD, LPVOID,lpContext, LPDDENUMSURFACESCALLBACK,lpEnumSurfacesCallback) \
    ICOM_METHOD (HRESULT,FlipToGDISurface) \
    ICOM_METHOD2(HRESULT,GetCaps,                LPDDCAPS,lpDDDriverCaps, LPDDCAPS,lpDDHELCaps) \
    ICOM_METHOD1(HRESULT,GetDisplayMode,         LPDDSURFACEDESC,lpDDSurfaceDesc) \
    ICOM_METHOD2(HRESULT,GetFourCCCodes,         LPDWORD,lpNumCodes, LPDWORD,lpCodes) \
    ICOM_METHOD1(HRESULT,GetGDISurface,          LPDIRECTDRAWSURFACE*,lplpGDIDDSurface) \
    ICOM_METHOD1(HRESULT,GetMonitorFrequency,    LPDWORD,lpdwFrequency) \
    ICOM_METHOD1(HRESULT,GetScanLine,            LPDWORD,lpdwScanLine) \
    ICOM_METHOD1(HRESULT,GetVerticalBlankStatus, BOOL*,lpbIsInVB) \
    ICOM_METHOD1(HRESULT,Initialize,             GUID*,lpGUID) \
    ICOM_METHOD (HRESULT,RestoreDisplayMode) \
    ICOM_METHOD2(HRESULT,SetCooperativeLevel,    HWND,hWnd, DWORD,dwFlags) \
    ICOM_METHOD3(HRESULT,SetDisplayMode,         DWORD,, DWORD,, DWORD,) \
    ICOM_METHOD2(HRESULT,WaitForVerticalBlank,   DWORD,dwFlags, HANDLE,hEvent)
#define IDirectDraw_IMETHODS \
    IUnknown_IMETHODS \
    IDirectDraw_METHODS
ICOM_DEFINE(IDirectDraw,IUnknown)
#undef ICOM_INTERFACE

#ifdef ICOM_CINTERFACE
    /*** IUnknown methods ***/
#define IDirectDraw_QueryInterface(p,a,b) ICOM_CALL2(QueryInterface,p,a,b)
#define IDirectDraw_AddRef(p)             ICOM_CALL (AddRef,p)
#define IDirectDraw_Release(p)            ICOM_CALL (Release,p)
    /*** IDirectDraw methods ***/
#define IDirectDraw_Compact(p)                  ICOM_CALL (Compact,p)
#define IDirectDraw_CreateClipper(p,a,b,c)      ICOM_CALL3(CreateClipper,p,a,b,c)
#define IDirectDraw_CreatePalette(p,a,b,c,d)    ICOM_CALL4(CreatePalette,p,a,b,c,d)
#define IDirectDraw_CreateSurface(p,a,b,c)      ICOM_CALL3(CreateSurface,p,a,b,c)
#define IDirectDraw_DuplicateSurface(p,a,b)     ICOM_CALL2(DuplicateSurface,p,a,b)
#define IDirectDraw_EnumDisplayModes(p,a,b,c,d) ICOM_CALL4(EnumDisplayModes,p,a,b,c,d)
#define IDirectDraw_EnumSurfaces(p,a,b,c,d)     ICOM_CALL4(EnumSurfaces,p,a,b,c,d)
#define IDirectDraw_FlipToGDISurface(p)         ICOM_CALL (FlipToGDISurface,p)
#define IDirectDraw_GetCaps(p,a,b)              ICOM_CALL2(GetCaps,p,a,b)
#define IDirectDraw_GetDisplayMode(p,a)         ICOM_CALL1(GetDisplayMode,p,a)
#define IDirectDraw_GetFourCCCodes(p,a,b)       ICOM_CALL2(GetFourCCCodes,p,a,b)
#define IDirectDraw_GetGDISurface(p,a)          ICOM_CALL1(GetGDISurface,p,a)
#define IDirectDraw_GetMonitorFrequency(p,a)    ICOM_CALL1(GetMonitorFrequency,p,a)
#define IDirectDraw_GetScanLine(p,a)            ICOM_CALL1(GetScanLine,p,a)
#define IDirectDraw_GetVerticalBlankStatus(p,a) ICOM_CALL1(GetVerticalBlankStatus,p,a)
#define IDirectDraw_Initialize(p,a)             ICOM_CALL1(Initialize,p,a)
#define IDirectDraw_RestoreDisplayMode(p)       ICOM_CALL (RestoreDisplayMode,p)
#define IDirectDraw_SetCooperativeLevel(p,a,b)  ICOM_CALL2(SetCooperativeLevel,p,a,b)
#define IDirectDraw_SetDisplayMode(p,a,b,c)     ICOM_CALL3(SetDisplayMode,p,a,b,c)
#define IDirectDraw_WaitForVerticalBlank(p,a,b) ICOM_CALL2(WaitForVerticalBlank,p,a,b)
#endif


/* flags for Lock() */
#define DDLOCK_SURFACEMEMORYPTR	0x00000000
#define DDLOCK_WAIT		0x00000001
#define DDLOCK_EVENT		0x00000002
#define DDLOCK_READONLY		0x00000010
#define DDLOCK_WRITEONLY	0x00000020
#define DDLOCK_NOSYSLOCK	0x00000800


/*****************************************************************************
 * IDirectDraw2 interface
 */
/* Note: IDirectDraw2 cannot derive from IDirectDraw because the number of 
 * arguments of SetDisplayMode has changed !
 */
#define ICOM_INTERFACE IDirectDraw2
#define IDirectDraw2_METHODS \
    ICOM_METHOD (HRESULT,Compact) \
    ICOM_METHOD3(HRESULT,CreateClipper,          DWORD,dwFlags, LPDIRECTDRAWCLIPPER*,lplpDDClipper, IUnknown*,pUnkOuter) \
    ICOM_METHOD4(HRESULT,CreatePalette,          DWORD,dwFlags, LPPALETTEENTRY,lpColorTable, LPDIRECTDRAWPALETTE*,lplpDDPalette, IUnknown*,pUnkOuter) \
    ICOM_METHOD3(HRESULT,CreateSurface,          LPDDSURFACEDESC,lpDDSurfaceDesc, LPDIRECTDRAWSURFACE*,lplpDDSurface, IUnknown*,pUnkOuter) \
    ICOM_METHOD2(HRESULT,DuplicateSurface,       LPDIRECTDRAWSURFACE,lpDDSurface, LPDIRECTDRAWSURFACE*,lplpDupDDSurface) \
    ICOM_METHOD4(HRESULT,EnumDisplayModes,       DWORD,dwFlags, LPDDSURFACEDESC,lpDDSurfaceDesc, LPVOID,lpContext, LPDDENUMMODESCALLBACK,lpEnumModesCallback) \
    ICOM_METHOD4(HRESULT,EnumSurfaces,           DWORD,dwFlags, LPDDSURFACEDESC,lpDDSD, LPVOID,lpContext, LPDDENUMSURFACESCALLBACK,lpEnumSurfacesCallback) \
    ICOM_METHOD (HRESULT,FlipToGDISurface) \
    ICOM_METHOD2(HRESULT,GetCaps,                LPDDCAPS,lpDDDriverCaps, LPDDCAPS,lpDDHELCaps) \
    ICOM_METHOD1(HRESULT,GetDisplayMode,         LPDDSURFACEDESC,lpDDSurfaceDesc) \
    ICOM_METHOD2(HRESULT,GetFourCCCodes,         LPDWORD,lpNumCodes, LPDWORD,lpCodes) \
    ICOM_METHOD1(HRESULT,GetGDISurface,          LPDIRECTDRAWSURFACE*,lplpGDIDDSurface) \
    ICOM_METHOD1(HRESULT,GetMonitorFrequency,    LPDWORD,lpdwFrequency) \
    ICOM_METHOD1(HRESULT,GetScanLine,            LPDWORD,lpdwScanLine) \
    ICOM_METHOD1(HRESULT,GetVerticalBlankStatus, BOOL*,lpbIsInVB) \
    ICOM_METHOD1(HRESULT,Initialize,             GUID*,lpGUID) \
    ICOM_METHOD (HRESULT,RestoreDisplayMode) \
    ICOM_METHOD2(HRESULT,SetCooperativeLevel,    HWND,hWnd, DWORD,dwFlags) \
    ICOM_METHOD5(HRESULT,SetDisplayMode,         DWORD,dwWidth, DWORD,dwHeight, DWORD,dwBPP, DWORD,dwRefreshRate, DWORD,dwFlags) \
    ICOM_METHOD2(HRESULT,WaitForVerticalBlank,   DWORD,dwFlags, HANDLE,hEvent) \
    /* added in v2 */ \
    ICOM_METHOD3(HRESULT,GetAvailableVidMem, LPDDSCAPS,lpDDCaps, LPDWORD,lpdwTotal, LPDWORD,lpdwFree)
#define IDirectDraw2_IMETHODS \
    IUnknown_IMETHODS \
    IDirectDraw2_METHODS
ICOM_DEFINE(IDirectDraw2,IUnknown)
#undef ICOM_INTERFACE

#ifdef ICOM_CINTERFACE
    /*** IUnknown methods ***/
#define IDirectDraw2_QueryInterface(p,a,b) ICOM_CALL2(QueryInterface,p,a,b)
#define IDirectDraw2_AddRef(p)             ICOM_CALL (AddRef,p)
#define IDirectDraw2_Release(p)            ICOM_CALL (Release,p)
    /*** IDirectDraw methods ***/
#define IDirectDraw2_Compact(p)                  ICOM_CALL (Compact,p)
#define IDirectDraw2_CreateClipper(p,a,b,c)      ICOM_CALL3(CreateClipper,p,a,b,c)
#define IDirectDraw2_CreatePalette(p,a,b,c,d)    ICOM_CALL4(CreatePalette,p,a,b,c,d)
#define IDirectDraw2_CreateSurface(p,a,b,c)      ICOM_CALL3(CreateSurface,p,a,b,c)
#define IDirectDraw2_DuplicateSurface(p,a,b)     ICOM_CALL2(DuplicateSurface,p,a,b)
#define IDirectDraw2_EnumDisplayModes(p,a,b,c,d) ICOM_CALL4(EnumDisplayModes,p,a,b,c,d)
#define IDirectDraw2_EnumSurfaces(p,a,b,c,d)     ICOM_CALL4(EnumSurfaces,p,a,b,c,d)
#define IDirectDraw2_FlipToGDISurface(p)         ICOM_CALL (FlipToGDISurface,p)
#define IDirectDraw2_GetCaps(p,a,b)              ICOM_CALL2(GetCaps,p,a,b)
#define IDirectDraw2_GetDisplayMode(p,a)         ICOM_CALL1(GetDisplayMode,p,a)
#define IDirectDraw2_GetFourCCCodes(p,a,b)       ICOM_CALL2(GetFourCCCodes,p,a,b)
#define IDirectDraw2_GetGDISurface(p,a)          ICOM_CALL1(GetGDISurface,p,a)
#define IDirectDraw2_GetMonitorFrequency(p,a)    ICOM_CALL1(GetMonitorFrequency,p,a)
#define IDirectDraw2_GetScanLine(p,a)            ICOM_CALL1(GetScanLine,p,a)
#define IDirectDraw2_GetVerticalBlankStatus(p,a) ICOM_CALL1(GetVerticalBlankStatus,p,a)
#define IDirectDraw2_Initialize(p,a)             ICOM_CALL1(Initialize,p,a)
#define IDirectDraw2_RestoreDisplayMode(p)       ICOM_CALL (RestoreDisplayMode,p)
#define IDirectDraw2_SetCooperativeLevel(p,a,b)  ICOM_CALL2(SetCooperativeLevel,p,a,b)
#define IDirectDraw2_SetDisplayMode(p,a,b,c,d,e) ICOM_CALL5(SetDisplayMode,p,a,b,c,d,e)
#define IDirectDraw2_WaitForVerticalBlank(p,a,b) ICOM_CALL2(WaitForVerticalBlank,p,a,b)
/*** IDirectDraw2 methods ***/
#define IDirectDraw2_GetAvailableVidMem(p,a,b,c) ICOM_CALL3(GetAvailableVidMem,p,a,b,c)
#endif


/*****************************************************************************
 * IDirectDraw4 interface
 */
#define ICOM_INTERFACE IDirectDraw4
#define IDirectDraw4_METHODS \
    ICOM_METHOD2(HRESULT,GetSurfaceFromDC,    HDC,, LPDIRECTDRAWSURFACE*,) \
    ICOM_METHOD (HRESULT,RestoreAllSurfaces) \
    ICOM_METHOD (HRESULT,TestCooperativeLevel) \
    ICOM_METHOD2(HRESULT,GetDeviceIdentifier, LPDDDEVICEIDENTIFIER,, DWORD,)
#define IDirectDraw4_IMETHODS \
    IDirectDraw2_IMETHODS \
    IDirectDraw4_METHODS
ICOM_DEFINE(IDirectDraw4,IDirectDraw2)
#undef ICOM_INTERFACE

#ifdef ICOM_CINTERFACE
    /*** IUnknown methods ***/
#define IDirectDraw4_QueryInterface(p,a,b) ICOM_CALL2(QueryInterface,p,a,b)
#define IDirectDraw4_AddRef(p)             ICOM_CALL (AddRef,p)
#define IDirectDraw4_Release(p)            ICOM_CALL (Release,p)
    /*** IDirectDraw methods ***/
#define IDirectDraw4_Compact(p)                  ICOM_CALL (Compact,p)
#define IDirectDraw4_CreateClipper(p,a,b,c)      ICOM_CALL3(CreateClipper,p,a,b,c)
#define IDirectDraw4_CreatePalette(p,a,b,c,d)    ICOM_CALL4(CreatePalette,p,a,b,c,d)
#define IDirectDraw4_CreateSurface(p,a,b,c)      ICOM_CALL3(CreateSurface,p,a,b,c)
#define IDirectDraw4_DuplicateSurface(p,a,b)     ICOM_CALL2(DuplicateSurface,p,a,b)
#define IDirectDraw4_EnumDisplayModes(p,a,b,c,d) ICOM_CALL4(EnumDisplayModes,p,a,b,c,d)
#define IDirectDraw4_EnumSurfaces(p,a,b,c,d)     ICOM_CALL4(EnumSurfaces,p,a,b,c,d)
#define IDirectDraw4_FlipToGDISurface(p)         ICOM_CALL (FlipToGDISurface,p)
#define IDirectDraw4_GetCaps(p,a,b)              ICOM_CALL2(GetCaps,p,a,b)
#define IDirectDraw4_GetDisplayMode(p,a)         ICOM_CALL1(GetDisplayMode,p,a)
#define IDirectDraw4_GetFourCCCodes(p,a,b)       ICOM_CALL2(GetFourCCCodes,p,a,b)
#define IDirectDraw4_GetGDISurface(p,a)          ICOM_CALL1(GetGDISurface,p,a)
#define IDirectDraw4_GetMonitorFrequency(p,a)    ICOM_CALL1(GetMonitorFrequency,p,a)
#define IDirectDraw4_GetScanLine(p,a)            ICOM_CALL1(GetScanLine,p,a)
#define IDirectDraw4_GetVerticalBlankStatus(p,a) ICOM_CALL1(GetVerticalBlankStatus,p,a)
#define IDirectDraw4_Initialize(p,a)             ICOM_CALL1(Initialize,p,a)
#define IDirectDraw4_RestoreDisplayMode(p)       ICOM_CALL (RestoreDisplayMode,p)
#define IDirectDraw4_SetCooperativeLevel(p,a,b)  ICOM_CALL2(SetCooperativeLevel,p,a,b)
#define IDirectDraw4_SetDisplayMode(p,a,b,c)     ICOM_CALL3(SetDisplayMode,p,a,b,c)
#define IDirectDraw4_WaitForVerticalBlank(p,a,b) ICOM_CALL2(WaitForVerticalBlank,p,a,b)
/*** IDirectDraw2 methods ***/
#define IDirectDraw4_GetAvailableVidMem(p,a,b,c) ICOM_CALL3(GetAvailableVidMem,p,a,b,c)
/*** IDirectDraw4 methods ***/
#define IDirectDraw4_GetSurfaceFromDC(p,a,b)    ICOM_CALL2(GetSurfaceFromDC,p,a,b,c)
#define IDirectDraw4_RestoreAllSurfaces(pc)     ICOM_CALL (RestoreAllSurfaces,p)
#define IDirectDraw4_TestCooperativeLevel(p)    ICOM_CALL (TestCooperativeLevel,p)
#define IDirectDraw4_GetDeviceIdentifier(p,a,b) ICOM_CALL2(GetDeviceIdentifier,p,a,b)
#endif


/*****************************************************************************
 * IDirectDrawSurface interface
 */
#define ICOM_INTERFACE IDirectDrawSurface
#define IDirectDrawSurface_METHODS \
    ICOM_METHOD1(HRESULT,AddAttachedSurface,    LPDIRECTDRAWSURFACE,lpDDSAttachedSurface) \
    ICOM_METHOD1(HRESULT,AddOverlayDirtyRect,   LPRECT,lpRect) \
    ICOM_METHOD5(HRESULT,Blt,                   LPRECT,lpDestRect, LPDIRECTDRAWSURFACE,lpDDSrcSurface, LPRECT,lpSrcRect, DWORD,dwFlags, LPDDBLTFX,lpDDBltFx) \
    ICOM_METHOD3(HRESULT,BltBatch,              LPDDBLTBATCH,lpDDBltBatch, DWORD,dwCount, DWORD,dwFlags) \
    ICOM_METHOD5(HRESULT,BltFast,               DWORD,dwX, DWORD,dwY, LPDIRECTDRAWSURFACE,lpDDSrcSurface, LPRECT,lpSrcRect, DWORD,dwTrans) \
    ICOM_METHOD2(HRESULT,DeleteAttachedSurface, DWORD,dwFlags, LPDIRECTDRAWSURFACE,lpDDSAttachedSurface) \
    ICOM_METHOD2(HRESULT,EnumAttachedSurfaces,  LPVOID,lpContext, LPDDENUMSURFACESCALLBACK,lpEnumSurfacesCallback) \
    ICOM_METHOD3(HRESULT,EnumOverlayZOrders,    DWORD,dwFlags, LPVOID,lpContext, LPDDENUMSURFACESCALLBACK,lpfnCallback) \
    ICOM_METHOD2(HRESULT,Flip,                  LPDIRECTDRAWSURFACE,lpDDSurfaceTargetOverride, DWORD,dwFlags) \
    ICOM_METHOD2(HRESULT,GetAttachedSurface,    LPDDSCAPS,lpDDSCaps, LPDIRECTDRAWSURFACE*,lplpDDAttachedSurface) \
    ICOM_METHOD1(HRESULT,GetBltStatus,          DWORD,dwFlags) \
    ICOM_METHOD1(HRESULT,GetCaps,               LPDDSCAPS,lpDDSCaps) \
    ICOM_METHOD1(HRESULT,GetClipper,            LPDIRECTDRAWCLIPPER*,lplpDDClipper) \
    ICOM_METHOD2(HRESULT,GetColorKey,           DWORD,dwFlags, LPDDCOLORKEY,lpDDColorKey) \
    ICOM_METHOD1(HRESULT,GetDC,                 HDC*,lphDC) \
    ICOM_METHOD1(HRESULT,GetFlipStatus,         DWORD,dwFlags) \
    ICOM_METHOD2(HRESULT,GetOverlayPosition,    LPLONG,lplX, LPLONG,lplY) \
    ICOM_METHOD1(HRESULT,GetPalette,            LPDIRECTDRAWPALETTE*,lplpDDPalette) \
    ICOM_METHOD1(HRESULT,GetPixelFormat,        LPDDPIXELFORMAT,lpDDPixelFormat) \
    ICOM_METHOD1(HRESULT,GetSurfaceDesc,        LPDDSURFACEDESC,lpDDSurfaceDesc) \
    ICOM_METHOD2(HRESULT,Initialize,            LPDIRECTDRAW,lpDD, LPDDSURFACEDESC,lpDDSurfaceDesc) \
    ICOM_METHOD (HRESULT,IsLost) \
    ICOM_METHOD4(HRESULT,Lock,                  LPRECT,lpDestRect, LPDDSURFACEDESC,lpDDSurfaceDesc, DWORD,dwFlags, HANDLE,hEvent) \
    ICOM_METHOD1(HRESULT,ReleaseDC,             HDC,hDC) \
    ICOM_METHOD (HRESULT,Restore) \
    ICOM_METHOD1(HRESULT,SetClipper,            LPDIRECTDRAWCLIPPER,lpDDClipper) \
    ICOM_METHOD2(HRESULT,SetColorKey,           DWORD,dwFlags, LPDDCOLORKEY,lpDDColorKey) \
    ICOM_METHOD2(HRESULT,SetOverlayPosition,    LONG,lX, LONG,lY) \
    ICOM_METHOD1(HRESULT,SetPalette,            LPDIRECTDRAWPALETTE,lpDDPalette) \
    ICOM_METHOD1(HRESULT,Unlock,                LPVOID,lpSurfaceData) \
    ICOM_METHOD5(HRESULT,UpdateOverlay,         LPRECT,lpSrcRect, LPDIRECTDRAWSURFACE,lpDDDestSurface, LPRECT,lpDestRect, DWORD,dwFlags, LPDDOVERLAYFX,lpDDOverlayFx) \
    ICOM_METHOD1(HRESULT,UpdateOverlayDisplay,  DWORD,dwFlags) \
    ICOM_METHOD2(HRESULT,UpdateOverlayZOrder,   DWORD,dwFlags, LPDIRECTDRAWSURFACE,lpDDSReference)
#define IDirectDrawSurface_IMETHODS \
    IUnknown_IMETHODS \
    IDirectDrawSurface_METHODS
ICOM_DEFINE(IDirectDrawSurface,IUnknown)
#undef ICOM_INTERFACE

#ifdef ICOM_CINTERFACE
    /*** IUnknown methods ***/
#define IDirectDrawSurface_QueryInterface(p,a,b) ICOM_CALL2(QueryInterface,p,a,b)
#define IDirectDrawSurface_AddRef(p)             ICOM_CALL (AddRef,p)
#define IDirectDrawSurface_Release(p)            ICOM_CALL (Release,p)
    /*** IDirectDrawSurface methods ***/
#define IDirectDrawSurface_AddAttachedSurface(p,a)      ICOM_CALL1(AddAttachedSurface,p,a)
#define IDirectDrawSurface_AddOverlayDirtyRect(p,a)     ICOM_CALL1(AddOverlayDirtyRect,p,a)
#define IDirectDrawSurface_Blt(p,a,b,c,d,e)             ICOM_CALL5(Blt,p,a,b,c,d,e)
#define IDirectDrawSurface_BltBatch(p,a,b,c)            ICOM_CALL3(BltBatch,p,a,b,c)
#define IDirectDrawSurface_BltFast(p,a,b,c,d,e)         ICOM_CALL5(BltFast,p,a,b,c,d,e)
#define IDirectDrawSurface_DeleteAttachedSurface(p,a,b) ICOM_CALL2(DeleteAttachedSurface,p,a,b)
#define IDirectDrawSurface_EnumAttachedSurfaces(p,a,b)  ICOM_CALL2(EnumAttachedSurfaces,p,a,b)
#define IDirectDrawSurface_EnumOverlayZOrders(p,a,b,c)  ICOM_CALL3(EnumOverlayZOrders,p,a,b,c)
#define IDirectDrawSurface_Flip(p,a,b)                  ICOM_CALL2(Flip,p,a,b)
#define IDirectDrawSurface_GetAttachedSurface(p,a,b)    ICOM_CALL2(GetAttachedSurface,p,a,b)
#define IDirectDrawSurface_GetBltStatus(p,a)            ICOM_CALL1(GetBltStatus,p,a)
#define IDirectDrawSurface_GetCaps(p,a)                 ICOM_CALL1(GetCaps,p,a)
#define IDirectDrawSurface_GetClipper(p,a)              ICOM_CALL1(GetClipper,p,a)
#define IDirectDrawSurface_GetColorKey(p,a,b)           ICOM_CALL2(GetColorKey,p,a,b)
#define IDirectDrawSurface_GetDC(p,a)                   ICOM_CALL1(GetDC,p,a)
#define IDirectDrawSurface_GetFlipStatus(p,a)           ICOM_CALL1(GetFlipStatus,p,a)
#define IDirectDrawSurface_GetOverlayPosition(p,a,b)    ICOM_CALL2(GetOverlayPosition,p,a,b)
#define IDirectDrawSurface_GetPalette(p,a)              ICOM_CALL1(GetPalette,p,a)
#define IDirectDrawSurface_GetPixelFormat(p,a)          ICOM_CALL1(GetPixelFormat,p,a)
#define IDirectDrawSurface_GetSurfaceDesc(p,a)          ICOM_CALL1(GetSurfaceDesc,p,a)
#define IDirectDrawSurface_Initialize(p,a,b)            ICOM_CALL2(Initialize,p,a,b)
#define IDirectDrawSurface_IsLost(p)                    ICOM_CALL (IsLost,p)
#define IDirectDrawSurface_Lock(p,a,b,c,d)              ICOM_CALL4(Lock,p,a,b,c,d)
#define IDirectDrawSurface_ReleaseDC(p,a)               ICOM_CALL1(ReleaseDC,p,a)
#define IDirectDrawSurface_Restore(p,a,b)               ICOM_CALL (Restore,p,a,b)
#define IDirectDrawSurface_SetClipper(p,a,b)            ICOM_CALL1(SetClipper,p,a,b)
#define IDirectDrawSurface_SetColorKey(p,a,b)           ICOM_CALL2(SetColorKey,p,a,b)
#define IDirectDrawSurface_SetOverlayPosition(p,a,b)    ICOM_CALL2(SetOverlayPosition,p,a,b)
#define IDirectDrawSurface_SetPalette(p,a)              ICOM_CALL1(SetPalette,p,a)
#define IDirectDrawSurface_Unlock(p,a)                  ICOM_CALL1(Unlock,p,a)
#define IDirectDrawSurface_UpdateOverlay(p,a,b,c,d,e)   ICOM_CALL5(UpdateOverlay,p,a,b,c,d,e)
#define IDirectDrawSurface_UpdateOverlayDisplay(p,a)    ICOM_CALL1(UpdateOverlayDisplay,p,a)
#define IDirectDrawSurface_UpdateOverlayZOrder(p,a,b)   ICOM_CALL2(UpdateOverlayZOrder,p,a,b)
#endif


/*****************************************************************************
 * IDirectDrawSurface2 interface
 */
/* Cannot inherit from IDirectDrawSurface because the LPDIRECTDRAWSURFACE parameters 
 * have been converted to LPDIRECTDRAWSURFACE2.
 */
#define ICOM_INTERFACE IDirectDrawSurface2
#define IDirectDrawSurface2_METHODS \
    ICOM_METHOD1(HRESULT,AddAttachedSurface,    LPDIRECTDRAWSURFACE2,lpDDSAttachedSurface) \
    ICOM_METHOD1(HRESULT,AddOverlayDirtyRect,   LPRECT,lpRect) \
    ICOM_METHOD5(HRESULT,Blt,                   LPRECT,lpDestRect, LPDIRECTDRAWSURFACE2,lpDDSrcSurface, LPRECT,lpSrcRect, DWORD,dwFlags, LPDDBLTFX,lpDDBltFx) \
    ICOM_METHOD3(HRESULT,BltBatch,              LPDDBLTBATCH,lpDDBltBatch, DWORD,dwCount, DWORD,dwFlags) \
    ICOM_METHOD5(HRESULT,BltFast,               DWORD,dwX, DWORD,dwY, LPDIRECTDRAWSURFACE2,lpDDSrcSurface, LPRECT,lpSrcRect, DWORD,dwTrans) \
    ICOM_METHOD2(HRESULT,DeleteAttachedSurface, DWORD,dwFlags, LPDIRECTDRAWSURFACE2,lpDDSAttachedSurface) \
    ICOM_METHOD2(HRESULT,EnumAttachedSurfaces,  LPVOID,lpContext, LPDDENUMSURFACESCALLBACK,lpEnumSurfacesCallback) \
    ICOM_METHOD3(HRESULT,EnumOverlayZOrders,    DWORD,dwFlags, LPVOID,lpContext, LPDDENUMSURFACESCALLBACK,lpfnCallback) \
    ICOM_METHOD2(HRESULT,Flip,                  LPDIRECTDRAWSURFACE2,lpDDSurfaceTargetOverride, DWORD,dwFlags) \
    ICOM_METHOD2(HRESULT,GetAttachedSurface,    LPDDSCAPS,lpDDSCaps, LPDIRECTDRAWSURFACE2*,lplpDDAttachedSurface) \
    ICOM_METHOD1(HRESULT,GetBltStatus,          DWORD,dwFlags) \
    ICOM_METHOD1(HRESULT,GetCaps,               LPDDSCAPS,lpDDSCaps) \
    ICOM_METHOD1(HRESULT,GetClipper,            LPDIRECTDRAWCLIPPER*,lplpDDClipper) \
    ICOM_METHOD2(HRESULT,GetColorKey,           DWORD,dwFlags, LPDDCOLORKEY,lpDDColorKey) \
    ICOM_METHOD1(HRESULT,GetDC,                 HDC*,lphDC) \
    ICOM_METHOD1(HRESULT,GetFlipStatus,         DWORD,dwFlags) \
    ICOM_METHOD2(HRESULT,GetOverlayPosition,    LPLONG,lplX, LPLONG,lplY) \
    ICOM_METHOD1(HRESULT,GetPalette,            LPDIRECTDRAWPALETTE*,lplpDDPalette) \
    ICOM_METHOD1(HRESULT,GetPixelFormat,        LPDDPIXELFORMAT,lpDDPixelFormat) \
    ICOM_METHOD1(HRESULT,GetSurfaceDesc,        LPDDSURFACEDESC,lpDDSurfaceDesc) \
    ICOM_METHOD2(HRESULT,Initialize,            LPDIRECTDRAW,lpDD, LPDDSURFACEDESC,lpDDSurfaceDesc) \
    ICOM_METHOD (HRESULT,IsLost) \
    ICOM_METHOD4(HRESULT,Lock,                  LPRECT,lpDestRect, LPDDSURFACEDESC,lpDDSurfaceDesc, DWORD,dwFlags, HANDLE,hEvent) \
    ICOM_METHOD1(HRESULT,ReleaseDC,             HDC,hDC) \
    ICOM_METHOD (HRESULT,Restore) \
    ICOM_METHOD1(HRESULT,SetClipper,            LPDIRECTDRAWCLIPPER,lpDDClipper) \
    ICOM_METHOD2(HRESULT,SetColorKey,           DWORD,dwFlags, LPDDCOLORKEY,lpDDColorKey) \
    ICOM_METHOD2(HRESULT,SetOverlayPosition,    LONG,lX, LONG,lY) \
    ICOM_METHOD1(HRESULT,SetPalette,            LPDIRECTDRAWPALETTE,lpDDPalette) \
    ICOM_METHOD1(HRESULT,Unlock,                LPVOID,lpSurfaceData) \
    ICOM_METHOD5(HRESULT,UpdateOverlay,         LPRECT,lpSrcRect, LPDIRECTDRAWSURFACE2,lpDDDestSurface, LPRECT,lpDestRect, DWORD,dwFlags, LPDDOVERLAYFX,lpDDOverlayFx) \
    ICOM_METHOD1(HRESULT,UpdateOverlayDisplay,  DWORD,dwFlags) \
    ICOM_METHOD2(HRESULT,UpdateOverlayZOrder,   DWORD,dwFlags, LPDIRECTDRAWSURFACE2,lpDDSReference) \
    /* added in v2 */ \
    ICOM_METHOD1(HRESULT,GetDDInterface, LPVOID*,lplpDD) \
    ICOM_METHOD1(HRESULT,PageLock,       DWORD,dwFlags) \
    ICOM_METHOD1(HRESULT,PageUnlock,     DWORD,dwFlags)
#define IDirectDrawSurface2_IMETHODS \
    IUnknown_IMETHODS \
    IDirectDrawSurface2_METHODS
ICOM_DEFINE(IDirectDrawSurface2,IUnknown)
#undef ICOM_INTERFACE

#ifdef ICOM_CINTERFACE
    /*** IUnknown methods ***/
#define IDirectDrawSurface2_QueryInterface(p,a,b) ICOM_CALL2(QueryInterface,p,a,b)
#define IDirectDrawSurface2_AddRef(p)             ICOM_CALL (AddRef,p)
#define IDirectDrawSurface2_Release(p)            ICOM_CALL (Release,p)
/*** IDirectDrawSurface methods (almost) ***/
#define IDirectDrawSurface2_AddAttachedSurface(p,a)      ICOM_CALL1(AddAttachedSurface,p,a)
#define IDirectDrawSurface2_AddOverlayDirtyRect(p,a)     ICOM_CALL1(AddOverlayDirtyRect,p,a)
#define IDirectDrawSurface2_Blt(p,a,b,c,d,e)             ICOM_CALL5(Blt,p,a,b,c,d,e)
#define IDirectDrawSurface2_BltBatch(p,a,b,c)            ICOM_CALL3(BltBatch,p,a,b,c)
#define IDirectDrawSurface2_BltFast(p,a,b,c,d,e)         ICOM_CALL5(BltFast,p,a,b,c,d,e)
#define IDirectDrawSurface2_DeleteAttachedSurface(p,a,b) ICOM_CALL2(DeleteAttachedSurface,p,a,b)
#define IDirectDrawSurface2_EnumAttachedSurfaces(p,a,b)  ICOM_CALL2(EnumAttachedSurfaces,p,a,b)
#define IDirectDrawSurface2_EnumOverlayZOrders(p,a,b,c)  ICOM_CALL3(EnumOverlayZOrders,p,a,b,c)
#define IDirectDrawSurface2_Flip(p,a,b)                  ICOM_CALL2(Flip,p,a,b)
#define IDirectDrawSurface2_GetAttachedSurface(p,a,b)    ICOM_CALL2(GetAttachedSurface,p,a,b)
#define IDirectDrawSurface2_GetBltStatus(p,a)            ICOM_CALL1(GetBltStatus,p,a)
#define IDirectDrawSurface2_GetCaps(p,a)                 ICOM_CALL1(GetCaps,p,a)
#define IDirectDrawSurface2_GetClipper(p,a)              ICOM_CALL1(GetClipper,p,a)
#define IDirectDrawSurface2_GetColorKey(p,a,b)           ICOM_CALL2(GetColorKey,p,a,b)
#define IDirectDrawSurface2_GetDC(p,a)                   ICOM_CALL1(GetDC,p,a)
#define IDirectDrawSurface2_GetFlipStatus(p,a)           ICOM_CALL1(GetFlipStatus,p,a)
#define IDirectDrawSurface2_GetOverlayPosition(p,a,b)    ICOM_CALL2(GetOverlayPosition,p,a,b)
#define IDirectDrawSurface2_GetPalette(p,a)              ICOM_CALL1(GetPalette,p,a)
#define IDirectDrawSurface2_GetPixelFormat(p,a)          ICOM_CALL1(GetPixelFormat,p,a)
#define IDirectDrawSurface2_GetSurfaceDesc(p,a)          ICOM_CALL1(GetSurfaceDesc,p,a)
#define IDirectDrawSurface2_Initialize(p,a,b)            ICOM_CALL2(Initialize,p,a,b)
#define IDirectDrawSurface2_IsLost(p)                    ICOM_CALL (IsLost,p)
#define IDirectDrawSurface2_Lock(p,a,b,c,d)              ICOM_CALL4(Lock,p,a,b,c,d)
#define IDirectDrawSurface2_ReleaseDC(p,a)               ICOM_CALL1(ReleaseDC,p,a)
#define IDirectDrawSurface2_Restore(p,a,b)               ICOM_CALL (Restore,p,a,b)
#define IDirectDrawSurface2_SetClipper(p,a,b)            ICOM_CALL1(SetClipper,p,a,b)
#define IDirectDrawSurface2_SetColorKey(p,a,b)           ICOM_CALL2(SetColorKey,p,a,b)
#define IDirectDrawSurface2_SetOverlayPosition(p,a,b)    ICOM_CALL2(SetOverlayPosition,p,a,b)
#define IDirectDrawSurface2_SetPalette(p,a)              ICOM_CALL1(SetPalette,p,a)
#define IDirectDrawSurface2_Unlock(p,a)                  ICOM_CALL1(Unlock,p,a)
#define IDirectDrawSurface2_UpdateOverlay(p,a,b,c,d,e)   ICOM_CALL5(UpdateOverlay,p,a,b,c,d,e)
#define IDirectDrawSurface2_UpdateOverlayDisplay(p,a)    ICOM_CALL1(UpdateOverlayDisplay,p,a)
#define IDirectDrawSurface2_UpdateOverlayZOrder(p,a,b)   ICOM_CALL2(UpdateOverlayZOrder,p,a,b)
/*** IDirectDrawSurface2 methods ***/
#define IDirectDrawSurface2_GetDDInterface(p,a) ICOM_CALL1(GetDDInterface,p,a)
#define IDirectDrawSurface2_PageLock(p,a)       ICOM_CALL1(PageLock,p,a)
#define IDirectDrawSurface2_PageUnlock(p,a)     ICOM_CALL1(PageUnlock,p,a)
#endif


/*****************************************************************************
 * IDirectDrawSurface3 interface
 */
/* Cannot inherit from IDirectDrawSurface2 because the LPDIRECTDRAWSURFACE2 parameters 
 * have been converted to LPDIRECTDRAWSURFACE3.
 */
#define ICOM_INTERFACE IDirectDrawSurface3
#define IDirectDrawSurface3_METHODS \
    ICOM_METHOD1(HRESULT,AddAttachedSurface,    LPDIRECTDRAWSURFACE3,lpDDSAttachedSurface) \
    ICOM_METHOD1(HRESULT,AddOverlayDirtyRect,   LPRECT,lpRect) \
    ICOM_METHOD5(HRESULT,Blt,                   LPRECT,lpDestRect, LPDIRECTDRAWSURFACE3,lpDDSrcSurface, LPRECT,lpSrcRect, DWORD,dwFlags, LPDDBLTFX,lpDDBltFx) \
    ICOM_METHOD3(HRESULT,BltBatch,              LPDDBLTBATCH,lpDDBltBatch, DWORD,dwCount, DWORD,dwFlags) \
    ICOM_METHOD5(HRESULT,BltFast,               DWORD,dwX, DWORD,dwY, LPDIRECTDRAWSURFACE3,lpDDSrcSurface, LPRECT,lpSrcRect, DWORD,dwTrans) \
    ICOM_METHOD2(HRESULT,DeleteAttachedSurface, DWORD,dwFlags, LPDIRECTDRAWSURFACE3,lpDDSAttachedSurface) \
    ICOM_METHOD2(HRESULT,EnumAttachedSurfaces,  LPVOID,lpContext, LPDDENUMSURFACESCALLBACK,lpEnumSurfacesCallback) \
    ICOM_METHOD3(HRESULT,EnumOverlayZOrders,    DWORD,dwFlags, LPVOID,lpContext, LPDDENUMSURFACESCALLBACK,lpfnCallback) \
    ICOM_METHOD2(HRESULT,Flip,                  LPDIRECTDRAWSURFACE3,lpDDSurfaceTargetOverride, DWORD,dwFlags) \
    ICOM_METHOD2(HRESULT,GetAttachedSurface,    LPDDSCAPS,lpDDSCaps, LPDIRECTDRAWSURFACE3*,lplpDDAttachedSurface) \
    ICOM_METHOD1(HRESULT,GetBltStatus,          DWORD,dwFlags) \
    ICOM_METHOD1(HRESULT,GetCaps,               LPDDSCAPS,lpDDSCaps) \
    ICOM_METHOD1(HRESULT,GetClipper,            LPDIRECTDRAWCLIPPER*,lplpDDClipper) \
    ICOM_METHOD2(HRESULT,GetColorKey,           DWORD,dwFlags, LPDDCOLORKEY,lpDDColorKey) \
    ICOM_METHOD1(HRESULT,GetDC,                 HDC*,lphDC) \
    ICOM_METHOD1(HRESULT,GetFlipStatus,         DWORD,dwFlags) \
    ICOM_METHOD2(HRESULT,GetOverlayPosition,    LPLONG,lplX, LPLONG,lplY) \
    ICOM_METHOD1(HRESULT,GetPalette,            LPDIRECTDRAWPALETTE*,lplpDDPalette) \
    ICOM_METHOD1(HRESULT,GetPixelFormat,        LPDDPIXELFORMAT,lpDDPixelFormat) \
    ICOM_METHOD1(HRESULT,GetSurfaceDesc,        LPDDSURFACEDESC,lpDDSurfaceDesc) \
    ICOM_METHOD2(HRESULT,Initialize,            LPDIRECTDRAW,lpDD, LPDDSURFACEDESC,lpDDSurfaceDesc) \
    ICOM_METHOD (HRESULT,IsLost) \
    ICOM_METHOD4(HRESULT,Lock,                  LPRECT,lpDestRect, LPDDSURFACEDESC,lpDDSurfaceDesc, DWORD,dwFlags, HANDLE,hEvent) \
    ICOM_METHOD1(HRESULT,ReleaseDC,             HDC,hDC) \
    ICOM_METHOD (HRESULT,Restore) \
    ICOM_METHOD1(HRESULT,SetClipper,            LPDIRECTDRAWCLIPPER,lpDDClipper) \
    ICOM_METHOD2(HRESULT,SetColorKey,           DWORD,dwFlags, LPDDCOLORKEY,lpDDColorKey) \
    ICOM_METHOD2(HRESULT,SetOverlayPosition,    LONG,lX, LONG,lY) \
    ICOM_METHOD1(HRESULT,SetPalette,            LPDIRECTDRAWPALETTE,lpDDPalette) \
    ICOM_METHOD1(HRESULT,Unlock,                LPVOID,lpSurfaceData) \
    ICOM_METHOD5(HRESULT,UpdateOverlay,         LPRECT,lpSrcRect, LPDIRECTDRAWSURFACE3,lpDDDestSurface, LPRECT,lpDestRect, DWORD,dwFlags, LPDDOVERLAYFX,lpDDOverlayFx) \
    ICOM_METHOD1(HRESULT,UpdateOverlayDisplay,  DWORD,dwFlags) \
    ICOM_METHOD2(HRESULT,UpdateOverlayZOrder,   DWORD,dwFlags, LPDIRECTDRAWSURFACE3,lpDDSReference) \
    /* added in v2 */ \
    ICOM_METHOD1(HRESULT,GetDDInterface, LPVOID*,lplpDD) \
    ICOM_METHOD1(HRESULT,PageLock,       DWORD,dwFlags) \
    ICOM_METHOD1(HRESULT,PageUnlock,     DWORD,dwFlags) \
    /* added in v3 */ \
    ICOM_METHOD2(HRESULT,SetSurfaceDesc, LPDDSURFACEDESC,lpDDSD, DWORD,dwFlags)
#define IDirectDrawSurface3_IMETHODS \
    IUnknown_IMETHODS \
    IDirectDrawSurface3_METHODS
ICOM_DEFINE(IDirectDrawSurface3,IUnknown)
#undef ICOM_INTERFACE

#ifdef ICOM_CINTERFACE
    /*** IUnknown methods ***/
#define IDirectDrawSurface3_QueryInterface(p,a,b) ICOM_CALL2(QueryInterface,p,a,b)
#define IDirectDrawSurface3_AddRef(p)             ICOM_CALL (AddRef,p)
#define IDirectDrawSurface3_Release(p)            ICOM_CALL (Release,p)
/*** IDirectDrawSurface methods (almost) ***/
#define IDirectDrawSurface3_AddAttachedSurface(p,a)      ICOM_CALL1(AddAttachedSurface,p,a)
#define IDirectDrawSurface3_AddOverlayDirtyRect(p,a)     ICOM_CALL1(AddOverlayDirtyRect,p,a)
#define IDirectDrawSurface3_Blt(p,a,b,c,d,e)             ICOM_CALL5(Blt,p,a,b,c,d,e)
#define IDirectDrawSurface3_BltBatch(p,a,b,c)            ICOM_CALL3(BltBatch,p,a,b,c)
#define IDirectDrawSurface3_BltFast(p,a,b,c,d,e)         ICOM_CALL5(BltFast,p,a,b,c,d,e)
#define IDirectDrawSurface3_DeleteAttachedSurface(p,a,b) ICOM_CALL2(DeleteAttachedSurface,p,a,b)
#define IDirectDrawSurface3_EnumAttachedSurfaces(p,a,b)  ICOM_CALL2(EnumAttachedSurfaces,p,a,b)
#define IDirectDrawSurface3_EnumOverlayZOrders(p,a,b,c)  ICOM_CALL3(EnumOverlayZOrders,p,a,b,c)
#define IDirectDrawSurface3_Flip(p,a,b)                  ICOM_CALL2(Flip,p,a,b)
#define IDirectDrawSurface3_GetAttachedSurface(p,a,b)    ICOM_CALL2(GetAttachedSurface,p,a,b)
#define IDirectDrawSurface3_GetBltStatus(p,a)            ICOM_CALL1(GetBltStatus,p,a)
#define IDirectDrawSurface3_GetCaps(p,a)                 ICOM_CALL1(GetCaps,p,a)
#define IDirectDrawSurface3_GetClipper(p,a)              ICOM_CALL1(GetClipper,p,a)
#define IDirectDrawSurface3_GetColorKey(p,a,b)           ICOM_CALL2(GetColorKey,p,a,b)
#define IDirectDrawSurface3_GetDC(p,a)                   ICOM_CALL1(GetDC,p,a)
#define IDirectDrawSurface3_GetFlipStatus(p,a)           ICOM_CALL1(GetFlipStatus,p,a)
#define IDirectDrawSurface3_GetOverlayPosition(p,a,b)    ICOM_CALL2(GetOverlayPosition,p,a,b)
#define IDirectDrawSurface3_GetPalette(p,a)              ICOM_CALL1(GetPalette,p,a)
#define IDirectDrawSurface3_GetPixelFormat(p,a)          ICOM_CALL1(GetPixelFormat,p,a)
#define IDirectDrawSurface3_GetSurfaceDesc(p,a)          ICOM_CALL1(GetSurfaceDesc,p,a)
#define IDirectDrawSurface3_Initialize(p,a,b)            ICOM_CALL2(Initialize,p,a,b)
#define IDirectDrawSurface3_IsLost(p)                    ICOM_CALL (IsLost,p)
#define IDirectDrawSurface3_Lock(p,a,b,c,d)              ICOM_CALL4(Lock,p,a,b,c,d)
#define IDirectDrawSurface3_ReleaseDC(p,a)               ICOM_CALL1(ReleaseDC,p,a)
#define IDirectDrawSurface3_Restore(p,a,b)               ICOM_CALL (Restore,p,a,b)
#define IDirectDrawSurface3_SetClipper(p,a,b)            ICOM_CALL1(SetClipper,p,a,b)
#define IDirectDrawSurface3_SetColorKey(p,a,b)           ICOM_CALL2(SetColorKey,p,a,b)
#define IDirectDrawSurface3_SetOverlayPosition(p,a,b)    ICOM_CALL2(SetOverlayPosition,p,a,b)
#define IDirectDrawSurface3_SetPalette(p,a)              ICOM_CALL1(SetPalette,p,a)
#define IDirectDrawSurface3_Unlock(p,a)                  ICOM_CALL1(Unlock,p,a)
#define IDirectDrawSurface3_UpdateOverlay(p,a,b,c,d,e)   ICOM_CALL5(UpdateOverlay,p,a,b,c,d,e)
#define IDirectDrawSurface3_UpdateOverlayDisplay(p,a)    ICOM_CALL1(UpdateOverlayDisplay,p,a)
#define IDirectDrawSurface3_UpdateOverlayZOrder(p,a,b)   ICOM_CALL2(UpdateOverlayZOrder,p,a,b)
/*** IDirectDrawSurface2 methods ***/
#define IDirectDrawSurface3_GetDDInterface(p,a) ICOM_CALL1(GetDDInterface,p,a)
#define IDirectDrawSurface3_PageLock(p,a)       ICOM_CALL1(PageLock,p,a)
#define IDirectDrawSurface3_PageUnlock(p,a)     ICOM_CALL1(PageUnlock,p,a)
/*** IDirectDrawSurface3 methods ***/
#define IDirectDrawSurface3_SetSurfaceDesc(p,a,b) ICOM_CALL2(SetSurfaceDesc,p,a,b)
#endif


/*****************************************************************************
 * IDirectDrawSurface4 interface
 */
/* Cannot inherit from IDirectDrawSurface2 because the LPDIRECTDRAWSURFACE2 parameters 
 * have been converted to LPDIRECTDRAWSURFACE3.
 */
/* FIXME: I assumed IDirectDrawSurface4 does not inherit from IDirectDrawSurface3 but I 
 * have actually not proof of that. Can someone check and remove this comment or patch 
 * accordingly ?
 */
#define ICOM_INTERFACE IDirectDrawSurface4
#define IDirectDrawSurface4_METHODS \
    ICOM_METHOD1(HRESULT,AddAttachedSurface,    LPDIRECTDRAWSURFACE4,lpDDSAttachedSurface) \
    ICOM_METHOD1(HRESULT,AddOverlayDirtyRect,   LPRECT,lpRect) \
    ICOM_METHOD5(HRESULT,Blt,                   LPRECT,lpDestRect, LPDIRECTDRAWSURFACE4,lpDDSrcSurface, LPRECT,lpSrcRect, DWORD,dwFlags, LPDDBLTFX,lpDDBltFx) \
    ICOM_METHOD3(HRESULT,BltBatch,              LPDDBLTBATCH,lpDDBltBatch, DWORD,dwCount, DWORD,dwFlags) \
    ICOM_METHOD5(HRESULT,BltFast,               DWORD,dwX, DWORD,dwY, LPDIRECTDRAWSURFACE4,lpDDSrcSurface, LPRECT,lpSrcRect, DWORD,dwTrans) \
    ICOM_METHOD2(HRESULT,DeleteAttachedSurface, DWORD,dwFlags, LPDIRECTDRAWSURFACE4,lpDDSAttachedSurface) \
    ICOM_METHOD2(HRESULT,EnumAttachedSurfaces,  LPVOID,lpContext, LPDDENUMSURFACESCALLBACK,lpEnumSurfacesCallback) \
    ICOM_METHOD3(HRESULT,EnumOverlayZOrders,    DWORD,dwFlags, LPVOID,lpContext, LPDDENUMSURFACESCALLBACK,lpfnCallback) \
    ICOM_METHOD2(HRESULT,Flip,                  LPDIRECTDRAWSURFACE4,lpDDSurfaceTargetOverride, DWORD,dwFlags) \
    ICOM_METHOD2(HRESULT,GetAttachedSurface,    LPDDSCAPS,lpDDSCaps, LPDIRECTDRAWSURFACE4*,lplpDDAttachedSurface) \
    ICOM_METHOD1(HRESULT,GetBltStatus,          DWORD,dwFlags) \
    ICOM_METHOD1(HRESULT,GetCaps,               LPDDSCAPS,lpDDSCaps) \
    ICOM_METHOD1(HRESULT,GetClipper,            LPDIRECTDRAWCLIPPER*,lplpDDClipper) \
    ICOM_METHOD2(HRESULT,GetColorKey,           DWORD,dwFlags, LPDDCOLORKEY,lpDDColorKey) \
    ICOM_METHOD1(HRESULT,GetDC,                 HDC*,lphDC) \
    ICOM_METHOD1(HRESULT,GetFlipStatus,         DWORD,dwFlags) \
    ICOM_METHOD2(HRESULT,GetOverlayPosition,    LPLONG,lplX, LPLONG,lplY) \
    ICOM_METHOD1(HRESULT,GetPalette,            LPDIRECTDRAWPALETTE*,lplpDDPalette) \
    ICOM_METHOD1(HRESULT,GetPixelFormat,        LPDDPIXELFORMAT,lpDDPixelFormat) \
    ICOM_METHOD1(HRESULT,GetSurfaceDesc,        LPDDSURFACEDESC,lpDDSurfaceDesc) \
    ICOM_METHOD2(HRESULT,Initialize,            LPDIRECTDRAW,lpDD, LPDDSURFACEDESC,lpDDSurfaceDesc) \
    ICOM_METHOD (HRESULT,IsLost) \
    ICOM_METHOD4(HRESULT,Lock,                  LPRECT,lpDestRect, LPDDSURFACEDESC,lpDDSurfaceDesc, DWORD,dwFlags, HANDLE,hEvent) \
    ICOM_METHOD1(HRESULT,ReleaseDC,             HDC,hDC) \
    ICOM_METHOD (HRESULT,Restore) \
    ICOM_METHOD1(HRESULT,SetClipper,            LPDIRECTDRAWCLIPPER,lpDDClipper) \
    ICOM_METHOD2(HRESULT,SetColorKey,           DWORD,dwFlags, LPDDCOLORKEY,lpDDColorKey) \
    ICOM_METHOD2(HRESULT,SetOverlayPosition,    LONG,lX, LONG,lY) \
    ICOM_METHOD1(HRESULT,SetPalette,            LPDIRECTDRAWPALETTE,lpDDPalette) \
    ICOM_METHOD1(HRESULT,Unlock,                LPVOID,lpSurfaceData) \
    ICOM_METHOD5(HRESULT,UpdateOverlay,         LPRECT,lpSrcRect, LPDIRECTDRAWSURFACE4,lpDDDestSurface, LPRECT,lpDestRect, DWORD,dwFlags, LPDDOVERLAYFX,lpDDOverlayFx) \
    ICOM_METHOD1(HRESULT,UpdateOverlayDisplay,  DWORD,dwFlags) \
    ICOM_METHOD2(HRESULT,UpdateOverlayZOrder,   DWORD,dwFlags, LPDIRECTDRAWSURFACE4,lpDDSReference) \
    /* added in v2 */ \
    ICOM_METHOD1(HRESULT,GetDDInterface, LPVOID*,lplpDD) \
    ICOM_METHOD1(HRESULT,PageLock,       DWORD,dwFlags) \
    ICOM_METHOD1(HRESULT,PageUnlock,     DWORD,dwFlags) \
    /* added in v3 */ \
    ICOM_METHOD2(HRESULT,SetSurfaceDesc, LPDDSURFACEDESC,lpDDSD, DWORD,dwFlags) \
    /* added in v4 */ \
    ICOM_METHOD4(HRESULT,SetPrivateData,       REFGUID,, LPVOID,, DWORD,, DWORD,) \
    ICOM_METHOD3(HRESULT,GetPrivateData,       REFGUID,, LPVOID,, LPDWORD,) \
    ICOM_METHOD1(HRESULT,FreePrivateData,      REFGUID,) \
    ICOM_METHOD1(HRESULT,GetUniquenessValue,   LPDWORD,) \
    ICOM_METHOD (HRESULT,ChangeUniquenessValue)
#define IDirectDrawSurface4_IMETHODS \
    IUnknown_IMETHODS \
    IDirectDrawSurface4_METHODS
ICOM_DEFINE(IDirectDrawSurface4,IUnknown)
#undef ICOM_INTERFACE

#ifdef ICOM_CINTERFACE
    /*** IUnknown methods ***/
#define IDirectDrawSurface4_QueryInterface(p,a,b) ICOM_CALL2(QueryInterface,p,a,b)
#define IDirectDrawSurface4_AddRef(p)             ICOM_CALL (AddRef,p)
#define IDirectDrawSurface4_Release(p)            ICOM_CALL (Release,p)
/*** IDirectDrawSurface (almost) methods ***/
#define IDirectDrawSurface4_AddAttachedSurface(p,a)      ICOM_CALL1(AddAttachedSurface,p,a)
#define IDirectDrawSurface4_AddOverlayDirtyRect(p,a)     ICOM_CALL1(AddOverlayDirtyRect,p,a)
#define IDirectDrawSurface4_Blt(p,a,b,c,d,e)             ICOM_CALL5(Blt,p,a,b,c,d,e)
#define IDirectDrawSurface4_BltBatch(p,a,b,c)            ICOM_CALL3(BltBatch,p,a,b,c)
#define IDirectDrawSurface4_BltFast(p,a,b,c,d,e)         ICOM_CALL5(BltFast,p,a,b,c,d,e)
#define IDirectDrawSurface4_DeleteAttachedSurface(p,a,b) ICOM_CALL2(DeleteAttachedSurface,p,a,b)
#define IDirectDrawSurface4_EnumAttachedSurfaces(p,a,b)  ICOM_CALL2(EnumAttachedSurfaces,p,a,b)
#define IDirectDrawSurface4_EnumOverlayZOrders(p,a,b,c)  ICOM_CALL3(EnumOverlayZOrders,p,a,b,c)
#define IDirectDrawSurface4_Flip(p,a,b)                  ICOM_CALL2(Flip,p,a,b)
#define IDirectDrawSurface4_GetAttachedSurface(p,a,b)    ICOM_CALL2(GetAttachedSurface,p,a,b)
#define IDirectDrawSurface4_GetBltStatus(p,a)            ICOM_CALL1(GetBltStatus,p,a)
#define IDirectDrawSurface4_GetCaps(p,a)                 ICOM_CALL1(GetCaps,p,a)
#define IDirectDrawSurface4_GetClipper(p,a)              ICOM_CALL1(GetClipper,p,a)
#define IDirectDrawSurface4_GetColorKey(p,a,b)           ICOM_CALL2(GetColorKey,p,a,b)
#define IDirectDrawSurface4_GetDC(p,a)                   ICOM_CALL1(GetDC,p,a)
#define IDirectDrawSurface4_GetFlipStatus(p,a)           ICOM_CALL1(GetFlipStatus,p,a)
#define IDirectDrawSurface4_GetOverlayPosition(p,a,b)    ICOM_CALL2(GetOverlayPosition,p,a,b)
#define IDirectDrawSurface4_GetPalette(p,a)              ICOM_CALL1(GetPalette,p,a)
#define IDirectDrawSurface4_GetPixelFormat(p,a)          ICOM_CALL1(GetPixelFormat,p,a)
#define IDirectDrawSurface4_GetSurfaceDesc(p,a)          ICOM_CALL1(GetSurfaceDesc,p,a)
#define IDirectDrawSurface4_Initialize(p,a,b)            ICOM_CALL2(Initialize,p,a,b)
#define IDirectDrawSurface4_IsLost(p)                    ICOM_CALL (IsLost,p)
#define IDirectDrawSurface4_Lock(p,a,b,c,d)              ICOM_CALL4(Lock,p,a,b,c,d)
#define IDirectDrawSurface4_ReleaseDC(p,a)               ICOM_CALL1(ReleaseDC,p,a)
#define IDirectDrawSurface4_Restore(p,a,b)               ICOM_CALL (Restore,p,a,b)
#define IDirectDrawSurface4_SetClipper(p,a,b)            ICOM_CALL1(SetClipper,p,a,b)
#define IDirectDrawSurface4_SetColorKey(p,a,b)           ICOM_CALL2(SetColorKey,p,a,b)
#define IDirectDrawSurface4_SetOverlayPosition(p,a,b)    ICOM_CALL2(SetOverlayPosition,p,a,b)
#define IDirectDrawSurface4_SetPalette(p,a)              ICOM_CALL1(SetPalette,p,a)
#define IDirectDrawSurface4_Unlock(p,a)                  ICOM_CALL1(Unlock,p,a)
#define IDirectDrawSurface4_UpdateOverlay(p,a,b,c,d,e)   ICOM_CALL5(UpdateOverlay,p,a,b,c,d,e)
#define IDirectDrawSurface4_UpdateOverlayDisplay(p,a)    ICOM_CALL1(UpdateOverlayDisplay,p,a)
#define IDirectDrawSurface4_UpdateOverlayZOrder(p,a,b)   ICOM_CALL2(UpdateOverlayZOrder,p,a,b)
/*** IDirectDrawSurface2 methods ***/
#define IDirectDrawSurface4_GetDDInterface(p,a) ICOM_CALL1(GetDDInterface,p,a)
#define IDirectDrawSurface4_PageLock(p,a)       ICOM_CALL1(PageLock,p,a)
#define IDirectDrawSurface4_PageUnlock(p,a)     ICOM_CALL1(PageUnlock,p,a)
/*** IDirectDrawSurface3 methods ***/
#define IDirectDrawSurface4_SetSurfaceDesc(p,a) ICOM_CALL(SetSurfaceDesc,p,a)
/*** IDirectDrawSurface4 methods ***/
#define IDirectDrawSurface4_SetPrivateData(p,a,b,c,d) ICOM_CALL4(SetPrivateData,p,a,b,c,d)
#define IDirectDrawSurface4_GetPrivateData(p,a,b,c)   ICOM_CALL3(GetPrivateData,p,a,b,c)
#define IDirectDrawSurface4_FreePrivateData(p,a)      ICOM_CALL1(FreePrivateData,p,a)
#define IDirectDrawSurface4_GetUniquenessValue(p,a)   ICOM_CALL1(GetUniquenessValue,p,a)
#define IDirectDrawSurface4_ChangeUniquenessValue(p)  ICOM_CALL (ChangeUniquenessValue,p)
#endif


/*****************************************************************************
 * IDirectDrawColorControl interface
 */
#define ICOM_INTERFACE IDirectDrawColorControl
#define IDirectDrawColorControl_METHODS \
    ICOM_METHOD1(HRESULT,GetColorControls, LPDDCOLORCONTROL,lpColorControl) \
    ICOM_METHOD1(HRESULT,SetColorControls, LPDDCOLORCONTROL,lpColorControl)
#define IDirectDrawColorControl_IMETHODS \
    IUnknown_IMETHODS \
    IDirectDrawColorControl_METHODS
ICOM_DEFINE(IDirectDrawColorControl,IUnknown)
#undef ICOM_INTERFACE

#ifdef ICOM_CINTERFACE
	/*** IUnknown methods ***/
#define IDirectDrawColorControl_QueryInterface(p,a,b) ICOM_CALL2(QueryInterface,p,a,b)
#define IDirectDrawColorControl_AddRef(p)             ICOM_CALL (AddRef,p)
#define IDirectDrawColorControl_Release(p)            ICOM_CALL (Release,p)
	/*** IDirectDrawColorControl methods ***/
#define IDirectDrawColorControl_GetColorControls(p,a) ICOM_CALL(GetColorControls,p,a)
#define IDirectDrawColorControl_SetColorControls(p,a) ICOM_CALL(SetColorControls,p,a)
#endif


HRESULT WINAPI DirectDrawCreate(LPGUID,LPDIRECTDRAW*,LPUNKNOWN);
HRESULT WINAPI DirectDrawEnumerateA(LPDDENUMCALLBACKA,LPVOID);
HRESULT WINAPI DirectDrawEnumerateW(LPDDENUMCALLBACKW,LPVOID);
#define DirectDrawEnumerate WINELIB_NAME_AW(DirectDrawEnumerate)
HRESULT WINAPI DirectDrawCreateClipper(DWORD,LPDIRECTDRAWCLIPPER*,LPUNKNOWN);

#endif /* __WINE_DDRAW_H */
