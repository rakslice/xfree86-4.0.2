/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/ati/r128_accel.c,v 1.4 2000/12/04 19:21:52 dawes Exp $ */
/*
 * Copyright 1999, 2000 ATI Technologies Inc., Markham, Ontario,
 *                      Precision Insight, Inc., Cedar Park, Texas, and
 *                      VA Linux Systems Inc., Fremont, California.
 *
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation on the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NON-INFRINGEMENT.  IN NO EVENT SHALL ATI, PRECISION INSIGHT, VA LINUX
 * SYSTEMS AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * Authors:
 *   Rickard E. Faith <faith@valinux.com>
 *   Kevin E. Martin <martin@valinux.com>
 *   Alan Hourihane <ahourihane@valinux.com>
 *
 * Credits:
 *
 *   Thanks to Alan Hourihane <alanh@fairlite.demon..co.uk> and SuSE for
 *   providing source code to their 3.3.x Rage 128 driver.  Portions of
 *   this file are based on the acceleration code for that driver.
 *
 * References:
 *
 *   RAGE 128 VR/ RAGE 128 GL Register Reference Manual (Technical
 *   Reference Manual P/N RRG-G04100-C Rev. 0.04), ATI Technologies: April
 *   1999.
 *
 *   RAGE 128 Software Development Manual (Technical Reference Manual P/N
 *   SDK-G04000 Rev. 0.01), ATI Technologies: June 1999.
 *
 * Notes on unimplemented XAA optimizations:
 *
 *   SetClipping:   The Rage128 doesn't support the full 16bit registers needed
 *                  for XAA clip rect support.
 *   SolidFillTrap: This will probably work if we can compute the correct
 *                  Bresenham error values.
 *   TwoPointLine:  The Rage 128 supports Bresenham lines instead.
 *   DashedLine with non-power-of-two pattern length: Apparently, there is
 *                  no way to set the length of the pattern -- it is always
 *                  assumed to be 8 or 32 (or 1024?).
 *   ScreenToScreenColorExpandFill: See p. 4-17 of the Technical Reference
 *                  Manual where it states that monochrome expansion of frame
 *                  buffer data is not supported.
 *   CPUToScreenColorExpandFill, direct: The implementation here uses a hybrid
 *                  direct/indirect method.  If we had more data registers,
 *                  then we could do better.  If XAA supported a trigger write
 *                  address, the code would be simpler.
 * (Alan Hourihane) Update. We now use purely indirect and clip the full
 *                  rectangle. Seems as the direct method has some problems
 *                  with this, although this indirect method is much faster
 *                  than the old method of setting up the engine per scanline.
 *                  This code was the basis of the Radeon work we did.
 *   Color8x8PatternFill: Apparently, an 8x8 color brush cannot take an 8x8
 *                  pattern from frame buffer memory.
 *   ImageWrites:   See CPUToScreenColorExpandFill.
 *
 */

#define R128_IMAGEWRITE 0       /* Disable ImageWrites - faster in software */
#define R128_TRAPEZOIDS 0       /* Trapezoids don't work               */

				/* Driver data structures */
#include "r128.h"
#include "r128_reg.h"
#ifdef XF86DRI
#define _XF86DRI_SERVER_
#include "r128_dri.h"
#endif

				/* Line support */
#include "miline.h"

				/* X and server generic header files */
#include "xf86.h"

static struct {
    int rop;
    int pattern;
} R128_ROP[] = {
    { R128_ROP3_ZERO, R128_ROP3_ZERO }, /* GXclear        */
    { R128_ROP3_DSa,  R128_ROP3_DPa  }, /* Gxand          */
    { R128_ROP3_SDna, R128_ROP3_PDna }, /* GXandReverse   */
    { R128_ROP3_S,    R128_ROP3_P    }, /* GXcopy         */
    { R128_ROP3_DSna, R128_ROP3_DPna }, /* GXandInverted  */
    { R128_ROP3_D,    R128_ROP3_D    }, /* GXnoop         */
    { R128_ROP3_DSx,  R128_ROP3_DPx  }, /* GXxor          */
    { R128_ROP3_DSo,  R128_ROP3_DPo  }, /* GXor           */
    { R128_ROP3_DSon, R128_ROP3_DPon }, /* GXnor          */
    { R128_ROP3_DSxn, R128_ROP3_PDxn }, /* GXequiv        */
    { R128_ROP3_Dn,   R128_ROP3_Dn   }, /* GXinvert       */
    { R128_ROP3_SDno, R128_ROP3_PDno }, /* GXorReverse    */
    { R128_ROP3_Sn,   R128_ROP3_Pn   }, /* GXcopyInverted */
    { R128_ROP3_DSno, R128_ROP3_DPno }, /* GXorInverted   */
    { R128_ROP3_DSan, R128_ROP3_DPan }, /* GXnand         */
    { R128_ROP3_ONE,  R128_ROP3_ONE  }  /* GXset          */
};

/* Flush all dirty data in the Pixel Cache to memory. */
void R128EngineFlush(ScrnInfoPtr pScrn)
{
    R128InfoPtr   info      = R128PTR(pScrn);
    unsigned char *R128MMIO = info->MMIO;
    int           i;

    OUTREGP(R128_PC_NGUI_CTLSTAT, R128_PC_FLUSH_ALL, ~R128_PC_FLUSH_ALL);
    for (i = 0; i < R128_TIMEOUT; i++) {
	if (!(INREG(R128_PC_NGUI_CTLSTAT) & R128_PC_BUSY)) break;
    }
}

/* Reset graphics card to known state. */
void R128EngineReset(ScrnInfoPtr pScrn)
{
    R128InfoPtr   info      = R128PTR(pScrn);
    unsigned char *R128MMIO = info->MMIO;
    CARD32        clock_cntl_index;
    CARD32        mclk_cntl;
    CARD32        gen_reset_cntl;

    R128EngineFlush(pScrn);

    clock_cntl_index = INREG(R128_CLOCK_CNTL_INDEX);
    mclk_cntl        = INPLL(pScrn, R128_MCLK_CNTL);

    OUTPLL(R128_MCLK_CNTL, mclk_cntl | R128_FORCE_GCP | R128_FORCE_PIPE3D_CP);

    gen_reset_cntl   = INREG(R128_GEN_RESET_CNTL);

    OUTREG(R128_GEN_RESET_CNTL, gen_reset_cntl | R128_SOFT_RESET_GUI);
    INREG(R128_GEN_RESET_CNTL);
    OUTREG(R128_GEN_RESET_CNTL, gen_reset_cntl & ~R128_SOFT_RESET_GUI);
    INREG(R128_GEN_RESET_CNTL);

    OUTPLL(R128_MCLK_CNTL,        mclk_cntl);
    OUTREG(R128_CLOCK_CNTL_INDEX, clock_cntl_index);
    OUTREG(R128_GEN_RESET_CNTL,   gen_reset_cntl);
}

/* The FIFO has 64 slots.  This routines waits until at least `entries' of
   these slots are empty. */
void R128WaitForFifoFunction(ScrnInfoPtr pScrn, int entries)
{
    R128InfoPtr   info      = R128PTR(pScrn);
    unsigned char *R128MMIO = info->MMIO;
    int           i;

    for (;;) {
	for (i = 0; i < R128_TIMEOUT; i++) {
	    info->fifo_slots = INREG(R128_GUI_STAT) & R128_GUI_FIFOCNT_MASK;
	    if (info->fifo_slots >= entries) return;
	}
	R128TRACE(("FIFO timed out: %d entries, stat=0x%08x, probe=0x%08x\n",
		   INREG(R128_GUI_STAT) & R128_GUI_FIFOCNT_MASK,
		   INREG(R128_GUI_STAT),
		   INREG(R128_GUI_PROBE)));
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		   "FIFO timed out, resetting engine...\n");
	R128EngineReset(pScrn);
#ifdef XF86DRI
	R128CCE_RESET(pScrn, info);
	if (info->CCE2D) {
	    R128CCE_START(pScrn, info);
	}
#endif
    }
}

/* Wait for the graphics engine to be completely idle: the FIFO has
   drained, the Pixel Cache is flushed, and the engine is idle.  This is a
   standard "sync" function that will make the hardware "quiescent". */
void R128WaitForIdle(ScrnInfoPtr pScrn)
{
    R128InfoPtr   info      = R128PTR(pScrn);
    unsigned char *R128MMIO = info->MMIO;
    int           i;

    R128WaitForFifoFunction(pScrn, 64);

    for (;;) {
	for (i = 0; i < R128_TIMEOUT; i++) {
	    if (!(INREG(R128_GUI_STAT) & R128_GUI_ACTIVE)) {
		R128EngineFlush(pScrn);
		return;
	    }
	}
	R128TRACE(("Idle timed out: %d entries, stat=0x%08x, probe=0x%08x\n",
		   INREG(R128_GUI_STAT) & R128_GUI_FIFOCNT_MASK,
		   INREG(R128_GUI_STAT),
		   INREG(R128_GUI_PROBE)));
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		   "Idle timed out, resetting engine...\n");
	R128EngineReset(pScrn);
#ifdef XF86DRI
	R128CCE_RESET(pScrn, info);
	if (info->CCE2D) {
	    R128CCE_START(pScrn, info);
	}
#endif
    }
}

#ifdef XF86DRI
/* Wait until the CCE is completely idle: the FIFO has drained and the
 * CCE is idle.
 */
static void R128CCEWaitForIdle(ScrnInfoPtr pScrn)
{
    R128InfoPtr info = R128PTR(pScrn);
    int         ret;
    int         i    = 0;

    for (;;) {
	do {
	    ret = drmR128WaitForIdleCCE(info->drmFD);
	    if (ret && ret != -EBUSY) {
		xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
			   "%s: CCE idle %d\n", __FUNCTION__, ret);
	    }
	} while ((ret == -EBUSY) && (i++ < R128_TIMEOUT));

	if (ret == 0) return;

	xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		   "Idle timed out, resetting engine...\n");
	R128EngineReset(pScrn);

	/* Always restart the engine when doing CCE 2D acceleration */
	R128CCE_RESET(pScrn, info);
	R128CCE_START(pScrn, info);
    }
}
#endif

/* Setup for XAA SolidFill. */
static void R128SetupForSolidFill(ScrnInfoPtr pScrn,
				  int color, int rop, unsigned int planemask)
{
    R128InfoPtr   info      = R128PTR(pScrn);
    unsigned char *R128MMIO = info->MMIO;

#ifdef XF86DRI
    R128CCE_TO_MMIO(pScrn, info);
#endif

    R128WaitForFifo(pScrn, 4);
    OUTREG(R128_DP_GUI_MASTER_CNTL, (info->dp_gui_master_cntl
				     | R128_GMC_BRUSH_SOLID_COLOR
				     | R128_GMC_SRC_DATATYPE_COLOR
				     | R128_ROP[rop].pattern));
    OUTREG(R128_DP_BRUSH_FRGD_CLR,  color);
    OUTREG(R128_DP_WRITE_MASK,      planemask);
    OUTREG(R128_DP_CNTL,            (R128_DST_X_LEFT_TO_RIGHT
				     | R128_DST_Y_TOP_TO_BOTTOM));
}

/* Subsequent XAA SolidFillRect.

   Tests: xtest CH06/fllrctngl, xterm
*/
static void  R128SubsequentSolidFillRect(ScrnInfoPtr pScrn,
					 int x, int y, int w, int h)
{
    R128InfoPtr   info      = R128PTR(pScrn);
    unsigned char *R128MMIO = info->MMIO;

#ifdef XF86DRI
    R128CCE_TO_MMIO(pScrn, info);
#endif

    R128WaitForFifo(pScrn, 2);
    OUTREG(R128_DST_Y_X,          (y << 16) | x);
    OUTREG(R128_DST_WIDTH_HEIGHT, (w << 16) | h);
}

/* Setup for XAA solid lines. */
static void R128SetupForSolidLine(ScrnInfoPtr pScrn,
				  int color, int rop, unsigned int planemask)
{
    R128InfoPtr   info      = R128PTR(pScrn);
    unsigned char *R128MMIO = info->MMIO;

#ifdef XF86DRI
    R128CCE_TO_MMIO(pScrn, info);
#endif

    R128WaitForFifo(pScrn, 3);
    OUTREG(R128_DP_GUI_MASTER_CNTL, (info->dp_gui_master_cntl
				     | R128_GMC_BRUSH_SOLID_COLOR
				     | R128_GMC_SRC_DATATYPE_COLOR
				     | R128_ROP[rop].pattern));
    OUTREG(R128_DP_BRUSH_FRGD_CLR,  color);
    OUTREG(R128_DP_WRITE_MASK,      planemask);
}


/* Subsequent XAA solid Bresenham line.

   Tests: xtest CH06/drwln, ico, Mark Vojkovich's linetest program

   [See http://www.xfree86.org/devel/archives/devel/1999-Jun/0102.shtml for
   Mark Vojkovich's linetest program, posted 2Jun99 to devel@xfree86.org.]

   x11perf -line500
                               1024x768@76Hz   1024x768@76Hz
                                        8bpp           32bpp
   not used:                     39700.0/sec     34100.0/sec
   used:                         47600.0/sec     36800.0/sec
*/
static void R128SubsequentSolidBresenhamLine(ScrnInfoPtr pScrn,
					     int x, int y,
					     int major, int minor,
					     int err, int len, int octant)
{
    R128InfoPtr   info      = R128PTR(pScrn);
    unsigned char *R128MMIO = info->MMIO;
    int           flags     = 0;

#ifdef XF86DRI
    R128CCE_TO_MMIO(pScrn, info);
#endif

    if (octant & YMAJOR)         flags |= R128_DST_Y_MAJOR;
    if (!(octant & XDECREASING)) flags |= R128_DST_X_DIR_LEFT_TO_RIGHT;
    if (!(octant & YDECREASING)) flags |= R128_DST_Y_DIR_TOP_TO_BOTTOM;

    R128WaitForFifo(pScrn, 6);
    OUTREG(R128_DP_CNTL_XDIR_YDIR_YMAJOR, flags);
    OUTREG(R128_DST_Y_X,                  (y << 16) | x);
    OUTREG(R128_DST_BRES_ERR,             err);
    OUTREG(R128_DST_BRES_INC,             minor);
    OUTREG(R128_DST_BRES_DEC,             -major);
    OUTREG(R128_DST_BRES_LNTH,            len);
}

/* Subsequent XAA solid horizontal and vertical lines

   1024x768@76Hz 8bpp
                             Without             With
   x11perf -hseg500      87600.0/sec     798000.0/sec
   x11perf -vseg500      38100.0/sec      38000.0/sec
*/
static void R128SubsequentSolidHorVertLine(ScrnInfoPtr pScrn,
					   int x, int y, int len, int dir )
{
    R128InfoPtr   info      = R128PTR(pScrn);
    unsigned char *R128MMIO = info->MMIO;

#ifdef XF86DRI
    R128CCE_TO_MMIO(pScrn, info);
#endif

    R128WaitForFifo(pScrn, 1);
    OUTREG(R128_DP_CNTL, (R128_DST_X_LEFT_TO_RIGHT
			  | R128_DST_Y_TOP_TO_BOTTOM));

    if (dir == DEGREES_0) {
	R128SubsequentSolidFillRect(pScrn, x, y, len, 1);
    } else {
	R128SubsequentSolidFillRect(pScrn, x, y, 1, len);
    }
}

/* Setup for XAA dashed lines.

   Tests: xtest CH05/stdshs, XFree86/drwln

   NOTE: Since we can only accelerate lines with power-of-2 patterns of
   length <= 32, these x11perf numbers are not representative of the
   speed-up on appropriately-sized patterns.

   1024x768@76Hz 8bpp
                             Without             With
   x11perf -dseg100     218000.0/sec     222000.0/sec
   x11perf -dline100    215000.0/sec     221000.0/sec
   x11perf -ddline100   178000.0/sec     180000.0/sec
*/
static void R128SetupForDashedLine(ScrnInfoPtr pScrn,
				   int fg, int bg,
				   int rop, unsigned int planemask,
				   int length, unsigned char *pattern)
{
    R128InfoPtr   info      = R128PTR(pScrn);
    unsigned char *R128MMIO = info->MMIO;
    CARD32        pat       = *(CARD32 *)pattern;

#ifdef XF86DRI
    R128CCE_TO_MMIO(pScrn, info);
#endif

    switch (length) {
    case  2: pat |= pat <<  2; /* fall through */
    case  4: pat |= pat <<  4; /* fall through */
    case  8: pat |= pat <<  8; /* fall through */
    case 16: pat |= pat << 16;
    }

    R128WaitForFifo(pScrn, 5);
    OUTREG(R128_DP_GUI_MASTER_CNTL, (info->dp_gui_master_cntl
				     | (bg == -1
					? R128_GMC_BRUSH_32x1_MONO_FG_LA
					: R128_GMC_BRUSH_32x1_MONO_FG_BG)
				     | R128_ROP[rop].pattern
				     | R128_GMC_BYTE_LSB_TO_MSB));
    OUTREG(R128_DP_WRITE_MASK,      planemask);
    OUTREG(R128_DP_BRUSH_FRGD_CLR,  fg);
    OUTREG(R128_DP_BRUSH_BKGD_CLR,  bg);
    OUTREG(R128_BRUSH_DATA0,        pat);
}

/* Subsequent XAA dashed line. */
static void R128SubsequentDashedBresenhamLine(ScrnInfoPtr pScrn,
					      int x, int y,
					      int major, int minor,
					      int err, int len, int octant,
					      int phase)
{
    R128InfoPtr   info      = R128PTR(pScrn);
    unsigned char *R128MMIO = info->MMIO;
    int           flags     = 0;

#ifdef XF86DRI
    R128CCE_TO_MMIO(pScrn, info);
#endif

    if (octant & YMAJOR)         flags |= R128_DST_Y_MAJOR;
    if (!(octant & XDECREASING)) flags |= R128_DST_X_DIR_LEFT_TO_RIGHT;
    if (!(octant & YDECREASING)) flags |= R128_DST_Y_DIR_TOP_TO_BOTTOM;

    R128WaitForFifo(pScrn, 7);
    OUTREG(R128_DP_CNTL_XDIR_YDIR_YMAJOR, flags);
    OUTREG(R128_DST_Y_X,                  (y << 16) | x);
    OUTREG(R128_BRUSH_Y_X,                (phase << 16) | phase);
    OUTREG(R128_DST_BRES_ERR,             err);
    OUTREG(R128_DST_BRES_INC,             minor);
    OUTREG(R128_DST_BRES_DEC,             -major);
    OUTREG(R128_DST_BRES_LNTH,            len);
}

#if R128_TRAPEZOIDS
				/* This doesn't work.  Except in the
				   lower-left quadrant, all of the pixel
				   errors appear to be because eL and eR
				   are not correct.  Drawing from right to
				   left doesn't help.  Be aware that the
				   non-_SUB registers set the sub-pixel
				   values to 0.5 (0x08), which isn't what
				   XAA wants. */
/* Subsequent XAA SolidFillTrap.  XAA always passes data that assumes we
   fill from top to bottom, so dyL and dyR are always non-negative. */
static void R128SubsequentSolidFillTrap(ScrnInfoPtr pScrn, int y, int h,
					int left, int dxL, int dyL, int eL,
					int right, int dxR, int dyR, int eR)
{
    R128InfoPtr   info      = R128PTR(pScrn);
    unsigned char *R128MMIO = info->MMIO;
    int           flags     = 0;
    int           Lymajor   = 0;
    int           Rymajor   = 0;
    int           origdxL   = dxL;
    int           origdxR   = dxR;

#ifdef XF86DRI
    R128CCE_TO_MMIO(pScrn, info);
#endif

    R128TRACE(("Trap %d %d; L %d %d %d %d; R %d %d %d %d\n",
	       y, h,
	       left, dxL, dyL, eL,
	       right, dxR, dyR, eR));

    if (dxL < 0)    dxL = -dxL; else flags |= (1 << 0) /* | (1 << 8) */;
    if (dxR < 0)    dxR = -dxR; else flags |= (1 << 6);

    R128WaitForFifo(pScrn, 11);

#if 1
    OUTREG(R128_DP_CNTL,            flags | (1 << 1) | (1 << 7));
    OUTREG(R128_DST_Y_SUB,          ((y) << 4) | 0x0 );
    OUTREG(R128_DST_X_SUB,          ((left) << 4)|0x0);
    OUTREG(R128_TRAIL_BRES_ERR,     eR-dxR);
    OUTREG(R128_TRAIL_BRES_INC,     dxR);
    OUTREG(R128_TRAIL_BRES_DEC,     -dyR);
    OUTREG(R128_TRAIL_X_SUB,        ((right) << 4) | 0x0);
    OUTREG(R128_LEAD_BRES_ERR,      eL-dxL);
    OUTREG(R128_LEAD_BRES_INC,      dxL);
    OUTREG(R128_LEAD_BRES_DEC,      -dyL);
    OUTREG(R128_LEAD_BRES_LNTH_SUB, ((h) << 4) | 0x00);
#else
    OUTREG(R128_DP_CNTL,            flags | (1 << 1) );
    OUTREG(R128_DST_Y_SUB,          (y << 4));
    OUTREG(R128_DST_X_SUB,          (right << 4));
    OUTREG(R128_TRAIL_BRES_ERR,     eL);
    OUTREG(R128_TRAIL_BRES_INC,     dxL);
    OUTREG(R128_TRAIL_BRES_DEC,     -dyL);
    OUTREG(R128_TRAIL_X_SUB,        (left << 4) | 0);
    OUTREG(R128_LEAD_BRES_ERR,      eR);
    OUTREG(R128_LEAD_BRES_INC,      dxR);
    OUTREG(R128_LEAD_BRES_DEC,      -dyR);
    OUTREG(R128_LEAD_BRES_LNTH_SUB, h << 4);
#endif
}
#endif

/* Setup for XAA screen-to-screen copy.

   Tests: xtest CH06/fllrctngl (also tests transparency).
*/
static void R128SetupForScreenToScreenCopy(ScrnInfoPtr pScrn,
					   int xdir, int ydir, int rop,
					   unsigned int planemask,
					   int trans_color)
{
    R128InfoPtr   info      = R128PTR(pScrn);
    unsigned char *R128MMIO = info->MMIO;

#ifdef XF86DRI
    R128CCE_TO_MMIO(pScrn, info);
#endif

    info->xdir = xdir;
    info->ydir = ydir;
    R128WaitForFifo(pScrn, 3);
    OUTREG(R128_DP_GUI_MASTER_CNTL, (info->dp_gui_master_cntl
				     | R128_GMC_BRUSH_SOLID_COLOR
				     | R128_GMC_SRC_DATATYPE_COLOR
				     | R128_ROP[rop].rop
				     | R128_DP_SRC_SOURCE_MEMORY));
    OUTREG(R128_DP_WRITE_MASK,      planemask);
    OUTREG(R128_DP_CNTL,            ((xdir >= 0 ? R128_DST_X_LEFT_TO_RIGHT : 0)
				     | (ydir >= 0
					? R128_DST_Y_TOP_TO_BOTTOM
					: 0)));

    if (trans_color != -1) {
				/* Set up for transparency */
	R128WaitForFifo(pScrn, 3);
	OUTREG(R128_CLR_CMP_CLR_SRC, trans_color);
	OUTREG(R128_CLR_CMP_MASK,    R128_CLR_CMP_MSK);
	OUTREG(R128_CLR_CMP_CNTL,    (R128_SRC_CMP_NEQ_COLOR
				      | R128_CLR_CMP_SRC_SOURCE));
    }
}

/* Subsequent XAA screen-to-screen copy. */
static void R128SubsequentScreenToScreenCopy(ScrnInfoPtr pScrn,
					     int xa, int ya,
					     int xb, int yb,
					     int w, int h)
{
    R128InfoPtr   info      = R128PTR(pScrn);
    unsigned char *R128MMIO = info->MMIO;

#ifdef XF86DRI
    R128CCE_TO_MMIO(pScrn, info);
#endif

    if (info->xdir < 0) xa += w - 1, xb += w - 1;
    if (info->ydir < 0) ya += h - 1, yb += h - 1;

    R128WaitForFifo(pScrn, 3);
    OUTREG(R128_SRC_Y_X,          (ya << 16) | xa);
    OUTREG(R128_DST_Y_X,          (yb << 16) | xb);
    OUTREG(R128_DST_HEIGHT_WIDTH, (h << 16) | w);
}

/* Setup for XAA mono 8x8 pattern color expansion.  Patterns with
   transparency use `bg == -1'.  This routine is only used if the XAA
   pixmap cache is turned on.

   Tests: xtest XFree86/fllrctngl (no other test will test this routine with
                                   both transparency and non-transparency)

   1024x768@76Hz 8bpp
                             Without             With
   x11perf -srect100     38600.0/sec      85700.0/sec
   x11perf -osrect100    38600.0/sec      85700.0/sec
*/
static void R128SetupForMono8x8PatternFill(ScrnInfoPtr pScrn,
					   int patternx, int patterny,
					   int fg, int bg, int rop,
					   unsigned int planemask)
{
    R128InfoPtr   info      = R128PTR(pScrn);
    unsigned char *R128MMIO = info->MMIO;

#ifdef XF86DRI
    R128CCE_TO_MMIO(pScrn, info);
#endif

    R128WaitForFifo(pScrn, 6);
    OUTREG(R128_DP_GUI_MASTER_CNTL, (info->dp_gui_master_cntl
				     | (bg == -1
					? R128_GMC_BRUSH_8X8_MONO_FG_LA
					: R128_GMC_BRUSH_8X8_MONO_FG_BG)
				     | R128_ROP[rop].pattern
				     | R128_GMC_BYTE_LSB_TO_MSB));
    OUTREG(R128_DP_WRITE_MASK,      planemask);
    OUTREG(R128_DP_BRUSH_FRGD_CLR,  fg);
    OUTREG(R128_DP_BRUSH_BKGD_CLR,  bg);
    OUTREG(R128_BRUSH_DATA0,        patternx);
    OUTREG(R128_BRUSH_DATA1,        patterny);
}

/* Subsequent XAA 8x8 pattern color expansion.  Because they are used in
   the setup function, `patternx' and `patterny' are not used here. */
static void R128SubsequentMono8x8PatternFillRect(ScrnInfoPtr pScrn,
						 int patternx, int patterny,
						 int x, int y, int w, int h)
{
    R128InfoPtr   info      = R128PTR(pScrn);
    unsigned char *R128MMIO = info->MMIO;

#ifdef XF86DRI
    R128CCE_TO_MMIO(pScrn, info);
#endif

    R128WaitForFifo(pScrn, 3);
    OUTREG(R128_BRUSH_Y_X,        (patterny << 8) | patternx);
    OUTREG(R128_DST_Y_X,          (y << 16) | x);
    OUTREG(R128_DST_HEIGHT_WIDTH, (h << 16) | w);
}

#if 0
/* Setup for XAA color 8x8 pattern fill.

   Tests: xtest XFree86/fllrctngl (with Mono8x8PatternFill off)
*/
static void R128SetupForColor8x8PatternFill(ScrnInfoPtr pScrn,
					    int patx, int paty,
					    int rop, unsigned int planemask,
					    int trans_color)
{
    R128InfoPtr   info      = R128PTR(pScrn);
    unsigned char *R128MMIO = info->MMIO;

#ifdef XF86DRI
    R128CCE_TO_MMIO(pScrn, info);
#endif

    R128TRACE(("Color8x8 %d %d %d\n", trans_color, patx, paty));

    R128WaitForFifo(pScrn, 2);
    OUTREG(R128_DP_GUI_MASTER_CNTL, (info->dp_gui_master_cntl
				     | R128_GMC_BRUSH_8x8_COLOR
				     | R128_GMC_SRC_DATATYPE_COLOR
				     | R128_ROP[rop].rop
				     | R128_DP_SRC_SOURCE_MEMORY));
    OUTREG(R128_DP_WRITE_MASK,      planemask);

    if (trans_color != -1) {
				/* Set up for transparency */
	R128WaitForFifo(pScrn, 3);
	OUTREG(R128_CLR_CMP_CLR_SRC, trans_color);
	OUTREG(R128_CLR_CMP_MASK,    R128_CLR_CMP_MSK);
	OUTREG(R128_CLR_CMP_CNTL,    (R128_SRC_CMP_NEQ_COLOR
				      | R128_CLR_CMP_SRC_SOURCE));
    }
}

/* Subsequent XAA 8x8 pattern color expansion. */
static void R128SubsequentColor8x8PatternFillRect( ScrnInfoPtr pScrn,
						   int patx, int paty,
						   int x, int y, int w, int h)
{
    R128InfoPtr   info      = R128PTR(pScrn);
    unsigned char *R128MMIO = info->MMIO;

#ifdef XF86DRI
    R128CCE_TO_MMIO(pScrn, info);
#endif

    R128TRACE(("Color8x8 %d,%d %d,%d %d %d\n", patx, paty, x, y, w, h));
    R128WaitForFifo(pScrn, 3);
    OUTREG(R128_SRC_Y_X, (paty << 16) | patx);
    OUTREG(R128_DST_Y_X, (y << 16) | x);
    OUTREG(R128_DST_HEIGHT_WIDTH, (h << 16) | w);
}
#endif

/* Setup for XAA indirect CPU-to-screen color expansion (indirect).
   Because of how the scratch buffer is initialized, this is really a
   mainstore-to-screen color expansion.  Transparency is supported when `bg
   == -1'.

   x11perf -ftext (pure indirect):
                               1024x768@76Hz   1024x768@76Hz
                                        8bpp           32bpp
   not used:                    685000.0/sec    794000.0/sec
   used:                       1070000.0/sec   1080000.0/sec

   We could improve this indirect routine by about 10% if the hardware
   could accept DWORD padded scanlines, or if XAA could provide bit-packed
   data.  We might also be able to move to a direct routine if there were
   more HOST_DATA registers.

   Implementing the hybrid indirect/direct scheme improved performance in a
   few areas:

   1024x768@76 8bpp
                                   Indirect          Hybrid
   x11perf -oddsrect10          50100.0/sec     71700.0/sec
   x11perf -oddsrect100          4240.0/sec      6660.0/sec
   x11perf -bigsrect10          50300.0/sec     71100.0/sec
   x11perf -bigsrect100          4190.0/sec      6800.0/sec
   x11perf -polytext           584000.0/sec    714000.0/sec
   x11perf -polytext16         154000.0/sec    172000.0/sec
   x11perf -seg1              1780000.0/sec   1880000.0/sec
   x11perf -copyplane10         42900.0/sec     58300.0/sec
   x11perf -copyplane100         4400.0/sec      6710.0/sec
   x11perf -putimagexy10         5090.0/sec      6670.0/sec
   x11perf -putimagexy100         424.0/sec       575.0/sec

   1024x768@76 -depth 24 -fbbpp 32
                                   Indirect          Hybrid
   x11perf -oddsrect100          4240.0/sec      6670.0/sec
   x11perf -bigsrect100          4190.0/sec      6800.0/sec
   x11perf -polytext           585000.0/sec    719000.0/sec
   x11perf -seg1              2960000.0/sec   2990000.0/sec
   x11perf -copyplane100         4400.0/sec      6700.0/sec
   x11perf -putimagexy100         138.0/sec       191.0/sec

*/
static void R128SetupForScanlineCPUToScreenColorExpandFill(ScrnInfoPtr pScrn,
							   int fg, int bg,
							   int rop,
							   unsigned int
							   planemask)
{
    R128InfoPtr   info      = R128PTR(pScrn);
    unsigned char *R128MMIO = info->MMIO;

#ifdef XF86DRI
    R128CCE_TO_MMIO(pScrn, info);
#endif

    R128WaitForFifo(pScrn, 4);
    OUTREG(R128_DP_GUI_MASTER_CNTL, (info->dp_gui_master_cntl
				     | R128_GMC_DST_CLIPPING
				     | R128_GMC_BRUSH_NONE
				     | (bg == -1
					? R128_GMC_SRC_DATATYPE_MONO_FG_LA
					: R128_GMC_SRC_DATATYPE_MONO_FG_BG)
				     | R128_ROP[rop].rop
				     | R128_GMC_BYTE_LSB_TO_MSB
				     | R128_DP_SRC_SOURCE_HOST_DATA));
    OUTREG(R128_DP_WRITE_MASK,      planemask);
    OUTREG(R128_DP_SRC_FRGD_CLR,    fg);
    OUTREG(R128_DP_SRC_BKGD_CLR,    bg);
}

/* Subsequent XAA indirect CPU-to-screen color expansion.  This is only
   called once for each rectangle. */
static void R128SubsequentScanlineCPUToScreenColorExpandFill(ScrnInfoPtr pScrn,
							     int x, int y,
							     int w, int h,
							     int skipleft)
{
    R128InfoPtr   info      = R128PTR(pScrn);
    unsigned char *R128MMIO = info->MMIO;
    int x1clip = x+skipleft;
    int x2clip = x+w;

#ifdef XF86DRI
    R128CCE_TO_MMIO(pScrn, info);
#endif

    info->scanline_h      = h;
    info->scanline_words  = (w + 31) >> 5;

#if 0
    /* Seems as though the Rage128's doesn't like blitting directly
     * as we must be overwriting something too quickly, therefore we
     * render to the buffer first and then blit */
    if ((info->scanline_words * h) <= 9) {
	/* Turn on direct for less than 9 dword colour expansion */
	info->scratch_buffer[0]
	    = (unsigned char *)(ADDRREG(R128_HOST_DATA_LAST)
				- (info->scanline_words - 1));
	info->scanline_direct = 1;
    } else
#endif
    {
	/* Use indirect for anything else */
	info->scratch_buffer[0] = info->scratch_save;
	info->scanline_direct   = 0;
    }

    if (pScrn->bitsPerPixel == 24) {
	x1clip *= 3;
	x2clip *= 3;
    }

    R128WaitForFifo(pScrn, 4 + (info->scanline_direct ?
					(info->scanline_words * h) : 0) );
    OUTREG(R128_SC_TOP_LEFT,     (y << 16)       | (x1clip & 0xffff));
    OUTREG(R128_SC_BOTTOM_RIGHT, ((y+h-1) << 16) | ((x2clip-1) & 0xffff));
    OUTREG(R128_DST_Y_X,         (y << 16)       | (x & 0xffff));
    /* Have to pad the width here and use clipping engine */
    OUTREG(R128_DST_HEIGHT_WIDTH, (h << 16)      | ((w + 31) & ~31));
}

/* Subsequent XAA indirect CPU-to-screen color expandion.  This is called
   once for each scanline. */
static void R128SubsequentColorExpandScanline(ScrnInfoPtr pScrn, int bufno)
{
    R128InfoPtr     info      = R128PTR(pScrn);
    unsigned char   *R128MMIO = info->MMIO;
    CARD32          *p        = (CARD32 *)info->scratch_buffer[bufno];
    int             i;
    int             left      = info->scanline_words;
    volatile CARD32 *d;

#ifdef XF86DRI
    R128CCE_TO_MMIO(pScrn, info);
#endif

    if (info->scanline_direct) return;
    --info->scanline_h;
    while (left) {
	if (left <= 8) {
	  /* Last scanline - finish write to DATA_LAST */
	  if (info->scanline_h == 0) {
	    R128WaitForFifo(pScrn, left);
				/* Unrolling doesn't improve performance */
	    for (d = ADDRREG(R128_HOST_DATA_LAST) - (left - 1); left; --left)
		*d++ = *p++;
	    return;
	  } else {
	    R128WaitForFifo(pScrn, left);
				/* Unrolling doesn't improve performance */
	    for (d = ADDRREG(R128_HOST_DATA7) - (left - 1); left; --left)
		*d++ = *p++;
	  }
	} else {
	    R128WaitForFifo(pScrn, 8);
				/* Unrolling doesn't improve performance */
	    for (d = ADDRREG(R128_HOST_DATA0), i = 0; i < 8; i++)
		*d++ = *p++;
	    left -= 8;
	}
    }
}

/* Setup for XAA indirect image write.

   1024x768@76Hz 8bpp
                             Without             With
   x11perf -putimage10   37500.0/sec      39300.0/sec
   x11perf -putimage100   2150.0/sec       1170.0/sec
   x11perf -putimage500    108.0/sec         49.8/sec
 */
#if R128_IMAGEWRITE
static void R128SetupForScanlineImageWrite(ScrnInfoPtr pScrn,
					   int rop,
					   unsigned int planemask,
					   int trans_color,
					   int bpp,
					   int depth)
{
    R128InfoPtr   info      = R128PTR(pScrn);
    unsigned char *R128MMIO = info->MMIO;

#ifdef XF86DRI
    R128CCE_TO_MMIO(pScrn, info);
#endif

    info->scanline_bpp = bpp;

    R128WaitForFifo(pScrn, 2);
    OUTREG(R128_DP_GUI_MASTER_CNTL, (info->dp_gui_master_cntl
				     | R128_GMC_DST_CLIPPING
				     | R128_GMC_BRUSH_1X8_COLOR
				     | R128_GMC_SRC_DATATYPE_COLOR
				     | R128_ROP[rop].rop
				     | R128_GMC_BYTE_LSB_TO_MSB
				     | R128_DP_SRC_SOURCE_HOST_DATA));
    OUTREG(R128_DP_WRITE_MASK,      planemask);

    if (trans_color != -1) {
				/* Set up for transparency */
	R128WaitForFifo(pScrn, 3);
	OUTREG(R128_CLR_CMP_CLR_SRC, trans_color);
	OUTREG(R128_CLR_CMP_MASK,    R128_CLR_CMP_MSK);
	OUTREG(R128_CLR_CMP_CNTL,    (R128_SRC_CMP_NEQ_COLOR
				      | R128_CLR_CMP_SRC_SOURCE));
    }
}

/* Subsequent XAA indirect image write. This is only called once for each
   rectangle. */
static void R128SubsequentScanlineImageWriteRect(ScrnInfoPtr pScrn,
						 int x, int y,
						 int w, int h,
						 int skipleft)
{
    R128InfoPtr   info      = R128PTR(pScrn);
    unsigned char *R128MMIO = info->MMIO;
    int x1clip = x+skipleft;
    int x2clip = x+w;

    int shift = 0; /* 32bpp */

#ifdef XF86DRI
    R128CCE_TO_MMIO(pScrn, info);
#endif

    if (pScrn->bitsPerPixel == 8) shift = 3;
    else if (pScrn->bitsPerPixel == 16) shift = 1;

    info->scanline_h      = h;
    info->scanline_words  = (w * info->scanline_bpp + 31) >> 5;

#if 0
    /* Seeing as the CPUToScreen doesn't like this, I've done this
     * here too, as it uses pretty much the same path. */
    if ((info->scanline_words * h) <= 9) {
	/* Turn on direct for less than 9 dword colour expansion */
	info->scratch_buffer[0]
	    = (unsigned char *)(ADDRREG(R128_HOST_DATA_LAST)
				- (info->scanline_words - 1));
	info->scanline_direct = 1;
    } else
#endif
    {
	/* Use indirect for anything else */
	info->scratch_buffer[0] = info->scratch_save;
	info->scanline_direct   = 0;
    }

    if (pScrn->bitsPerPixel == 24) {
	x1clip *= 3;
	x2clip *= 3;
    }

    R128WaitForFifo(pScrn, 4 + (info->scanline_direct ?
					(info->scanline_words * h) : 0) );
    OUTREG(R128_SC_TOP_LEFT,      (y << 16)       | (x1clip & 0xffff));
    OUTREG(R128_SC_BOTTOM_RIGHT,  ((y+h-1) << 16) | ((x2clip-1) & 0xffff));
    OUTREG(R128_DST_Y_X,          (y << 16)       | (x & 0xffff));
    /* Have to pad the width here and use clipping engine */
    OUTREG(R128_DST_HEIGHT_WIDTH, (h << 16)       | ((w + shift) & ~shift));
}

/* Subsequent XAA indirect iamge write.  This is called once for each
   scanline. */
static void R128SubsequentImageWriteScanline(ScrnInfoPtr pScrn, int bufno)
{
    R128InfoPtr     info      = R128PTR(pScrn);
    unsigned char   *R128MMIO = info->MMIO;
    CARD32          *p        = (CARD32 *)info->scratch_buffer[bufno];
    int             i;
    int             left      = info->scanline_words;
    volatile CARD32 *d;

#ifdef XF86DRI
    R128CCE_TO_MMIO(pScrn, info);
#endif

    if (info->scanline_direct) return;
    --info->scanline_h;
    while (left) {
	if (left <= 8) {
	  /* Last scanline - finish write to DATA_LAST */
	  if (info->scanline_h == 0) {
	    R128WaitForFifo(pScrn, left);
				/* Unrolling doesn't improve performance */
	    for (d = ADDRREG(R128_HOST_DATA_LAST) - (left - 1); left; --left)
		*d++ = *p++;
	    return;
	  } else {
	    R128WaitForFifo(pScrn, left);
				/* Unrolling doesn't improve performance */
	    for (d = ADDRREG(R128_HOST_DATA7) - (left - 1); left; --left)
		*d++ = *p++;
	  }
	} else {
	    R128WaitForFifo(pScrn, 8);
				/* Unrolling doesn't improve performance */
	    for (d = ADDRREG(R128_HOST_DATA0), i = 0; i < 8; i++)
		*d++ = *p++;
	    left -= 8;
	}
    }
}
#endif

/* Initialize the acceleration hardware. */
void R128EngineInit(ScrnInfoPtr pScrn)
{
    R128InfoPtr   info      = R128PTR(pScrn);
    unsigned char *R128MMIO = info->MMIO;

    R128TRACE(("EngineInit (%d/%d)\n", info->CurrentLayout.pixel_code, info->CurrentLayout.bitsPerPixel));

    OUTREG(R128_SCALE_3D_CNTL, 0);
    R128EngineReset(pScrn);

    switch (info->CurrentLayout.pixel_code) {
    case 8:  info->datatype = 2; break;
    case 15: info->datatype = 3; break;
    case 16: info->datatype = 4; break;
    case 24: info->datatype = 5; break;
    case 32: info->datatype = 6; break;
    default:
	R128TRACE(("Unknown depth/bpp = %d/%d (code = %d)\n",
		   info->CurrentLayout.depth, info->CurrentLayout.bitsPerPixel,
		   info->CurrentLayout.pixel_code));
    }
    info->pitch = (info->CurrentLayout.displayWidth / 8) * (info->CurrentLayout.pixel_bytes == 3 ? 3 : 1);

    R128TRACE(("Pitch for acceleration = %d\n", info->pitch));

    R128WaitForFifo(pScrn, 2);
    OUTREG(R128_DEFAULT_OFFSET, 0);
    OUTREG(R128_DEFAULT_PITCH,  info->pitch);

    R128WaitForFifo(pScrn, 4);
    OUTREG(R128_AUX_SC_CNTL,             0);
    OUTREG(R128_DEFAULT_SC_BOTTOM_RIGHT, (R128_DEFAULT_SC_RIGHT_MAX
					  | R128_DEFAULT_SC_BOTTOM_MAX));
    OUTREG(R128_SC_TOP_LEFT,             0);
    OUTREG(R128_SC_BOTTOM_RIGHT,         (R128_DEFAULT_SC_RIGHT_MAX
					  | R128_DEFAULT_SC_BOTTOM_MAX));

    info->dp_gui_master_cntl = ((info->datatype << R128_GMC_DST_DATATYPE_SHIFT)
				| R128_GMC_CLR_CMP_CNTL_DIS
				| R128_GMC_AUX_CLIP_DIS);
    R128WaitForFifo(pScrn, 1);
    OUTREG(R128_DP_GUI_MASTER_CNTL, (info->dp_gui_master_cntl
				     | R128_GMC_BRUSH_SOLID_COLOR
				     | R128_GMC_SRC_DATATYPE_COLOR));

    R128WaitForFifo(pScrn, 8);
    OUTREG(R128_DST_BRES_ERR,      0);
    OUTREG(R128_DST_BRES_INC,      0);
    OUTREG(R128_DST_BRES_DEC,      0);
    OUTREG(R128_DP_BRUSH_FRGD_CLR, 0xffffffff);
    OUTREG(R128_DP_BRUSH_BKGD_CLR, 0x00000000);
    OUTREG(R128_DP_SRC_FRGD_CLR,   0xffffffff);
    OUTREG(R128_DP_SRC_BKGD_CLR,   0x00000000);
    OUTREG(R128_DP_WRITE_MASK,     0xffffffff);

    R128WaitForFifo(pScrn, 1);
#if X_BYTE_ORDER == X_BIG_ENDIAN
    OUTREGP(R128_DP_DATATYPE,
	    R128_HOST_BIG_ENDIAN_EN, ~R128_HOST_BIG_ENDIAN_EN);
#else
    OUTREGP(R128_DP_DATATYPE, 0, ~R128_HOST_BIG_ENDIAN_EN);
#endif

    R128WaitForIdle(pScrn);
}

#ifdef XF86DRI
    /* FIXME: When direct rendering is enabled, we should use the CCE to
       draw 2D commands */
static void R128CCEAccelInit(XAAInfoRecPtr a)
{
    a->Flags                            = 0;

				/* Sync */
    a->Sync                             = R128CCEWaitForIdle;
}
#endif

static void R128MMIOAccelInit(ScrnInfoPtr pScrn, XAAInfoRecPtr a)
{
    R128InfoPtr info = R128PTR(pScrn);

    a->Flags                            = (PIXMAP_CACHE
					   | OFFSCREEN_PIXMAPS
					   | LINEAR_FRAMEBUFFER);

				/* Sync */
    a->Sync                             = R128WaitForIdle;

				/* Solid Filled Rectangle */
    a->PolyFillRectSolidFlags           = 0;
    a->SetupForSolidFill                = R128SetupForSolidFill;
    a->SubsequentSolidFillRect          = R128SubsequentSolidFillRect;

				/* Screen-to-screen Copy */
				/* Transparency uses the wrong colors for
				   24 bpp mode -- the transparent part is
				   correct, but the opaque color is wrong.
				   This can be seen with netscape's I-bar
				   cursor when editing in the URL location
				   box. */
    a->ScreenToScreenCopyFlags          = ((pScrn->bitsPerPixel == 24)
					   ? NO_TRANSPARENCY
					   : 0);
    a->SetupForScreenToScreenCopy       = R128SetupForScreenToScreenCopy;
    a->SubsequentScreenToScreenCopy     = R128SubsequentScreenToScreenCopy;

				/* Mono 8x8 Pattern Fill (Color Expand) */
    a->SetupForMono8x8PatternFill       = R128SetupForMono8x8PatternFill;
    a->SubsequentMono8x8PatternFillRect = R128SubsequentMono8x8PatternFillRect;
    a->Mono8x8PatternFillFlags          = (HARDWARE_PATTERN_PROGRAMMED_BITS
					   | HARDWARE_PATTERN_PROGRAMMED_ORIGIN
					   | HARDWARE_PATTERN_SCREEN_ORIGIN
					   | BIT_ORDER_IN_BYTE_LSBFIRST);

				/* Indirect CPU-To-Screen Color Expand */
#if X_BYTE_ORDER == X_LITTLE_ENDIAN
    a->ScanlineCPUToScreenColorExpandFillFlags = LEFT_EDGE_CLIPPING
					       | LEFT_EDGE_CLIPPING_NEGATIVE_X;
#else
    a->ScanlineCPUToScreenColorExpandFillFlags = BIT_ORDER_IN_BYTE_MSBFIRST
					       | LEFT_EDGE_CLIPPING
					       | LEFT_EDGE_CLIPPING_NEGATIVE_X;
#endif
    a->NumScanlineColorExpandBuffers   = 1;
    a->ScanlineColorExpandBuffers      = info->scratch_buffer;
    info->scratch_save                 = xalloc(((pScrn->virtualX+31)/32*4)
					    + (pScrn->virtualX
					    * info->CurrentLayout.pixel_bytes));
    info->scratch_buffer[0]            = info->scratch_save;
    a->SetupForScanlineCPUToScreenColorExpandFill
	= R128SetupForScanlineCPUToScreenColorExpandFill;
    a->SubsequentScanlineCPUToScreenColorExpandFill
	= R128SubsequentScanlineCPUToScreenColorExpandFill;
    a->SubsequentColorExpandScanline   = R128SubsequentColorExpandScanline;

				/* Bresenham Solid Lines */
    a->SetupForSolidLine               = R128SetupForSolidLine;
    a->SubsequentSolidBresenhamLine    = R128SubsequentSolidBresenhamLine;
    a->SubsequentSolidHorVertLine      = R128SubsequentSolidHorVertLine;

				/* Bresenham Dashed Lines*/
    a->SetupForDashedLine              = R128SetupForDashedLine;
    a->SubsequentDashedBresenhamLine   = R128SubsequentDashedBresenhamLine;
    a->DashPatternMaxLength            = 32;
    a->DashedLineFlags                 = (LINE_PATTERN_LSBFIRST_LSBJUSTIFIED
					  | LINE_PATTERN_POWER_OF_2_ONLY);

				/* ImageWrite */
#if R128_IMAGEWRITE
    a->NumScanlineImageWriteBuffers    = 1;
    a->ScanlineImageWriteBuffers       = info->scratch_buffer;
    info->scratch_buffer[0]            = info->scratch_save;
    a->SetupForScanlineImageWrite      = R128SetupForScanlineImageWrite;
    a->SubsequentScanlineImageWriteRect= R128SubsequentScanlineImageWriteRect;
    a->SubsequentImageWriteScanline    = R128SubsequentImageWriteScanline;
    a->ScanlineImageWriteFlags         = CPU_TRANSFER_PAD_DWORD
		/* Performance tests show that we shouldn't use GXcopy for
		 * uploads as a memcpy is faster */
					  | NO_GXCOPY
					  | LEFT_EDGE_CLIPPING
					  | LEFT_EDGE_CLIPPING_NEGATIVE_X
					  | SCANLINE_PAD_DWORD;
#endif
}

/* Initialize XAA for supported acceleration and also initialize the
   graphics hardware for acceleration. */
Bool R128AccelInit(ScreenPtr pScreen)
{
    ScrnInfoPtr   pScrn = xf86Screens[pScreen->myNum];
    R128InfoPtr   info  = R128PTR(pScrn);
    XAAInfoRecPtr a;

    if (!(a = info->accel = XAACreateInfoRec())) return FALSE;

#ifdef XF86DRI
    /* FIXME: When direct rendering is enabled, we should use the CCE to
       draw 2D commands */
    if (info->CCE2D) R128CCEAccelInit(a);
    else
#endif
	R128MMIOAccelInit(pScrn, a);

    R128EngineInit(pScrn);
    return XAAInit(pScreen, a);
}
