/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/ati/atimode.c,v 1.1 2000/10/11 22:52:56 tsi Exp $ */
/*
 * Copyright 2000 by Marc Aurele La France (TSI @ UQV), tsi@ualberta.ca
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting documentation, and
 * that the name of Marc Aurele La France not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  Marc Aurele La France makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as-is" without express or implied warranty.
 *
 * MARC AURELE LA FRANCE DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS.  IN NO
 * EVENT SHALL MARC AURELE LA FRANCE BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#include "ati.h"
#include "atiadapter.h"
#include "atichip.h"
#include "atidac.h"
#include "atidsp.h"
#include "atimach64.h"
#include "atimach64io.h"
#include "atimode.h"
#include "atiprint.h"
#include "ativga.h"
#include "atiwonder.h"
#include "atiwonderio.h"

#ifndef AVOID_CPIO

/*
 * ATICopyVGAMemory --
 *
 * This function is called to copy one or all banks of a VGA plane.
 */
static void
ATICopyVGAMemory
(
    ATIPtr   pATI,
    ATIHWPtr pATIHW,
    pointer  *saveptr,
    pointer  *from,
    pointer  *to
)
{
    unsigned int iBank;

    for (iBank = 0;  iBank < pATIHW->nBank;  iBank++)
    {
        (*pATIHW->SetBank)(pATI, iBank);
        (void)memcpy(*to, *from, 0x00010000U);
        *saveptr = (char *)(*saveptr) + 0x00010000U;
    }
}

/*
 * ATISwap --
 *
 * This function saves/restores video memory contents during video mode
 * switches.
 */
static void
ATISwap
(
    int      iScreen,
    ATIPtr   pATI,
    ATIHWPtr pATIHW,
    Bool     ToFB
)
{
    pointer save, *from, *to;
    unsigned int iPlane = 0, PlaneMask = 1;
    CARD8 seq2, seq4, gra1, gra3, gra4, gra5, gra6, gra8;

    /*
     * This is only done for non-accelerator modes.  If the video state on
     * server entry was an accelerator mode, the application that relinquished
     * the console had better do the Right Thing (tm) anyway by saving and
     * restoring its own video memory contents.
     */
    if (pATIHW->crtc != ATI_CRTC_VGA)
        return;

    if (ToFB)
    {
        if (!pATIHW->frame_buffer)
            return;

        from = &save;
        to = &pATI->pBank;
    }
    else
    {
        /* Allocate the memory */
        if (!pATIHW->frame_buffer)
        {
            pATIHW->frame_buffer =
                (pointer)xalloc(pATIHW->nBank * pATIHW->nPlane * 0x00010000U);
            if (!pATIHW->frame_buffer)
            {
                xf86DrvMsg(iScreen, X_WARNING,
                    "Temporary frame buffer could not be allocated.\n");
                return;
            }
        }

        from = &pATI->pBank;
        to = &save;
    }

    /* Turn off screen */
    ATIVGASaveScreen(pATI, SCREEN_SAVER_ON);

    /* Save register values to be modified */
    seq2 = GetReg(SEQX, 0x02U);
    seq4 = GetReg(SEQX, 0x04U);
    gra1 = GetReg(GRAX, 0x01U);
    gra3 = GetReg(GRAX, 0x03U);
    gra4 = GetReg(GRAX, 0x04U);
    gra5 = GetReg(GRAX, 0x05U);
    gra6 = GetReg(GRAX, 0x06U);
    gra8 = GetReg(GRAX, 0x08U);

    save = pATIHW->frame_buffer;

    /* Temporarily normalise the mode */
    if (gra1 != 0x00U)
        PutReg(GRAX, 0x01U, 0x00U);
    if (gra3 != 0x00U)
        PutReg(GRAX, 0x03U, 0x00U);
    if (gra6 != 0x05U)
        PutReg(GRAX, 0x06U, 0x05U);
    if (gra8 != 0xFFU)
        PutReg(GRAX, 0x08U, 0xFFU);

    if (seq4 & 0x08U)
    {
        /* Setup packed mode memory */
        if (seq2 != 0x0FU)
            PutReg(SEQX, 0x02U, 0x0FU);
        if (seq4 != 0x0AU)
            PutReg(SEQX, 0x04U, 0x0AU);
        if (pATI->Chip < ATI_CHIP_264CT)
        {
            if (gra5 != 0x00U)
                PutReg(GRAX, 0x05U, 0x00U);
        }
        else
        {
            if (gra5 != 0x40U)
                PutReg(GRAX, 0x05U, 0x40U);
        }

        ATICopyVGAMemory(pATI, pATIHW, &save, from, to);

        if (seq2 != 0x0FU)
            PutReg(SEQX, 0x02U, seq2);
        if (seq4 != 0x0AU)
            PutReg(SEQX, 0x04U, seq4);
        if (pATI->Chip < ATI_CHIP_264CT)
        {
            if (gra5 != 0x00U)
                PutReg(GRAX, 0x05U, gra5);
        }
        else
        {
            if (gra5 != 0x40U)
                PutReg(GRAX, 0x05U, gra5);
        }
    }
    else
    {
        gra4 = GetReg(GRAX, 0x04U);

        /* Setup planar mode memory */
        if (seq4 != 0x06U)
            PutReg(SEQX, 0x04U, 0x06U);
        if (gra5 != 0x00U)
            PutReg(GRAX, 0x05U, 0x00U);

        for (;  iPlane < pATIHW->nPlane;  iPlane++)
        {
            PutReg(SEQX, 0x02U, PlaneMask);
            PutReg(GRAX, 0x04U, iPlane);
            ATICopyVGAMemory(pATI, pATIHW, &save, from, to);
            PlaneMask <<= 1;
        }

        PutReg(SEQX, 0x02U, seq2);
        if (seq4 != 0x06U)
            PutReg(SEQX, 0x04U, seq4);
        PutReg(GRAX, 0x04U, gra4);
        if (gra5 != 0x00U)
            PutReg(GRAX, 0x05U, gra5);
    }

    /* Restore registers */
    if (gra1 != 0x00U)
        PutReg(GRAX, 0x01U, gra1);
    if (gra3 != 0x00U)
        PutReg(GRAX, 0x03U, gra3);
    if (gra6 != 0x05U)
        PutReg(GRAX, 0x06U, gra6);
    if (gra8 != 0xFFU)
        PutReg(GRAX, 0x08U, gra8);

    /* Back to bank 0 */
    (*pATIHW->SetBank)(pATI, 0);

    /*
     * If restoring video memory for a server video mode, free the frame buffer
     * save area.
     */
    if (ToFB && (pATIHW == &pATI->NewHW))
    {
        xfree(pATIHW->frame_buffer);
        pATIHW->frame_buffer = NULL;
    }
}

#endif /* AVOID_CPIO */

/*
 * ATIModePreInit --
 *
 * This function initialises an ATIHWRec with information common to all video
 * states generated by the driver.
 */
void
ATIModePreInit
(
    ScrnInfoPtr pScreenInfo,
    ATIPtr      pATI,
    ATIHWPtr    pATIHW
)
{
    CARD32 lcd_index;
    int HDisplay, VDisplay;

#ifndef AVOID_CPIO

    if (pATI->VGAAdapter != ATI_ADAPTER_NONE)
    {
        /* Fill in VGA data */
        ATIVGAPreInit(pATI, pATIHW);

        /* Fill in VGA Wonder data */
        if (pATI->CPIO_VGAWonder)
            ATIVGAWonderPreInit(pATI, pATIHW);
    }

    /* Fill in Mach64 data */
    if (pATI->Chip >= ATI_CHIP_88800GXC)

#endif /* AVOID_CPIO */

    {
        ATIMach64PreInit(pScreenInfo, pATI, pATIHW);
    }

    if (pATI->Chip >= ATI_CHIP_264CT)
    {
        /* Ensure proper VCLK source */
        pATIHW->pll_vclk_cntl = ATIGetMach64PLLReg(PLL_VCLK_CNTL) |
            (PLL_VCLK_SRC_SEL | PLL_VCLK_RESET);

        /* Set provisional values for other PLL registers */
        pATIHW->pll_vclk_post_div = ATIGetMach64PLLReg(PLL_VCLK_POST_DIV);
        pATIHW->pll_vclk0_fb_div = ATIGetMach64PLLReg(PLL_VCLK0_FB_DIV);
        pATIHW->pll_vclk1_fb_div = ATIGetMach64PLLReg(PLL_VCLK1_FB_DIV);
        pATIHW->pll_vclk2_fb_div = ATIGetMach64PLLReg(PLL_VCLK2_FB_DIV);
        pATIHW->pll_vclk3_fb_div = ATIGetMach64PLLReg(PLL_VCLK3_FB_DIV);
        pATIHW->pll_xclk_cntl = ATIGetMach64PLLReg(PLL_XCLK_CNTL);

        /* For now disable extended reference and feedback dividers */
        if (pATI->Chip >= ATI_CHIP_264LT)
            pATIHW->pll_ext_vpll_cntl = ATIGetMach64PLLReg(PLL_EXT_VPLL_CNTL) &
                ~(PLL_EXT_VPLL_EN | PLL_EXT_VPLL_VGA_EN | PLL_EXT_VPLL_INSYNC);
    }

    /* Initialise CRTC data for LCD panels */
    if (pATI->LCDPanelID >= 0)
    {
        if (pATI->Chip == ATI_CHIP_264LT)
        {
            pATIHW->horz_stretching = inr(HORZ_STRETCHING);
            pATIHW->vert_stretching = inr(VERT_STRETCHING);
            pATIHW->lcd_gen_ctrl = inr(LCD_GEN_CTRL);
        }
        else /* if ((pATI->Chip == ATI_CHIP_264LTPRO) ||
                    (pATI->Chip == ATI_CHIP_264XL) ||
                    (pATI->Chip == ATI_CHIP_MOBILITY)) */
        {
            lcd_index = inr(LCD_INDEX);
            pATIHW->lcd_index = (lcd_index &
                ~(LCD_REG_INDEX | LCD_DISPLAY_DIS | LCD_SRC_SEL)) |
                (LCD_SRC_SEL_CRTC1 | LCD_CRTC2_DISPLAY_DIS);
            pATIHW->config_panel =
                ATIGetMach64LCDReg(LCD_CONFIG_PANEL) | DONT_SHADOW_HEND;
            pATIHW->lcd_gen_ctrl = ATIGetMach64LCDReg(LCD_GEN_CNTL);
            pATIHW->horz_stretching = ATIGetMach64LCDReg(LCD_HORZ_STRETCHING);
            pATIHW->vert_stretching = ATIGetMach64LCDReg(LCD_VERT_STRETCHING);
            outr(LCD_INDEX, lcd_index);
        }

        pATIHW->lcd_gen_ctrl &=
            ~(HORZ_DIVBY2_EN | DISABLE_PCLK_RESET | DIS_HOR_CRT_DIVBY2 |
              VCLK_DAC_PM_EN | XTALIN_PM_EN | CRTC_RW_SELECT |
              USE_SHADOWED_VEND | USE_SHADOWED_ROWCUR |
              SHADOW_EN | SHADOW_RW_EN);
        pATIHW->lcd_gen_ctrl |= DONT_SHADOW_VPAR | LOCK_8DOT;

        if (pATI->OptionCRT)
        {
            /*
             * Use primary CRTC to drive the CRT.  Turn off panel interface.
             */
            pATIHW->lcd_gen_ctrl &= ~LCD_ON;
            pATIHW->lcd_gen_ctrl |= CRT_ON;
        }
        else
        {
            /* Use primary CRTC to drive the panel */
            pATIHW->lcd_gen_ctrl |= LCD_ON;

            /*
             * Determine porch data.  The following is inaccurate (but still
             * good enough) when BIOS initialisation has set things up so that
             * the registers read here are not the ones actually in use by the
             * panel.
             */

#ifndef AVOID_CPIO

            if (pATI->LockData.crtc_gen_cntl & CRTC_EXT_DISP_EN)

#endif /* AVOID_CPIO */

            {
                pATIHW->crtc_h_total_disp = inr(CRTC_H_TOTAL_DISP);
                pATIHW->crtc_h_sync_strt_wid = inr(CRTC_H_SYNC_STRT_WID);
                pATIHW->crtc_v_total_disp = inr(CRTC_V_TOTAL_DISP);
                pATIHW->crtc_v_sync_strt_wid = inr(CRTC_V_SYNC_STRT_WID);

                HDisplay = GetBits(pATIHW->crtc_h_total_disp, CRTC_H_DISP);
                VDisplay = GetBits(pATIHW->crtc_v_total_disp, CRTC_V_DISP);

                pATI->LCDHSyncStart =
                    (GetBits(pATIHW->crtc_h_sync_strt_wid,
                             CRTC_H_SYNC_STRT_HI) *
                     (MaxBits(CRTC_H_SYNC_STRT) + 1)) +
                    GetBits(pATIHW->crtc_h_sync_strt_wid, CRTC_H_SYNC_STRT) -
                    HDisplay;
                pATI->LCDHSyncWidth =
                    GetBits(pATIHW->crtc_h_sync_strt_wid, CRTC_H_SYNC_WID);
                pATI->LCDHBlankWidth =
                    GetBits(pATIHW->crtc_h_total_disp, CRTC_H_TOTAL) -
                    HDisplay;
                pATI->LCDVSyncStart =
                    GetBits(pATIHW->crtc_v_sync_strt_wid, CRTC_V_SYNC_STRT) -
                    VDisplay;
                pATI->LCDVSyncWidth =
                    GetBits(pATIHW->crtc_v_sync_strt_wid, CRTC_V_SYNC_WID);
                pATI->LCDVBlankWidth =
                    GetBits(pATIHW->crtc_v_total_disp, CRTC_V_TOTAL) -
                    VDisplay;
            }

#ifndef AVOID_CPIO

            else
            {
                pATIHW->crt[0] = GetReg(CRTX(pATI->CPIO_VGABase), 0x00U);
                pATIHW->crt[1] = GetReg(CRTX(pATI->CPIO_VGABase), 0x01U);
                pATIHW->crt[4] = GetReg(CRTX(pATI->CPIO_VGABase), 0x04U);
                pATIHW->crt[5] = GetReg(CRTX(pATI->CPIO_VGABase), 0x05U);
                pATIHW->crt[6] = GetReg(CRTX(pATI->CPIO_VGABase), 0x06U);
                pATIHW->crt[7] = GetReg(CRTX(pATI->CPIO_VGABase), 0x07U);
                pATIHW->crt[16] = GetReg(CRTX(pATI->CPIO_VGABase), 0x10U);
                pATIHW->crt[17] = GetReg(CRTX(pATI->CPIO_VGABase), 0x11U);
                pATIHW->crt[18] = GetReg(CRTX(pATI->CPIO_VGABase), 0x12U);

                HDisplay = pATIHW->crt[1] + 1;
                VDisplay = (((pATIHW->crt[7] << 3) & 0x0200U) |
                            ((pATIHW->crt[7] << 7) & 0x0100U) |
                            pATIHW->crt[18]) + 1;

                pATI->LCDHSyncStart = pATIHW->crt[4] - HDisplay;
                pATI->LCDHSyncWidth =
                    (pATIHW->crt[5] - pATIHW->crt[4]) & 0x1FU;
                pATI->LCDHBlankWidth = pATIHW->crt[0] + 5 - HDisplay;
                pATI->LCDVSyncStart = (((pATIHW->crt[7] << 2) & 0x0200U) |
                                       ((pATIHW->crt[7] << 6) & 0x0100U) |
                                       pATIHW->crt[16]) - VDisplay;
                pATI->LCDVSyncWidth =
                    (pATIHW->crt[17] - pATIHW->crt[16]) & 0x0FU;
                pATI->LCDVBlankWidth = (((pATIHW->crt[7] << 4) & 0x0200U) |
                                        ((pATIHW->crt[7] << 8) & 0x0100U) |
                                        pATIHW->crt[6]) + 2 - VDisplay;
            }

#endif /* AVOID_CPIO */

            HDisplay <<= 3;
            pATI->LCDHSyncStart <<= 3;
            pATI->LCDHSyncWidth <<= 3;
            pATI->LCDHBlankWidth <<= 3;

            /* If the mode on entry wasn't stretched, adjust timings */
            if (!(pATIHW->horz_stretching & HORZ_STRETCH_EN) &&
                ((HDisplay = pATI->LCDHorizontal - HDisplay) > 0))
            {
                if ((pATI->LCDHSyncStart -= HDisplay) < 0)
                    pATI->LCDHSyncStart = 0;
                pATI->LCDHBlankWidth -= HDisplay;
                HDisplay = pATI->LCDHSyncStart + pATI->LCDHSyncWidth;
                if (pATI->LCDHBlankWidth < HDisplay)
                    pATI->LCDHBlankWidth = HDisplay;
            }

            if (!(pATIHW->vert_stretching & VERT_STRETCH_EN) &&
                ((VDisplay = pATI->LCDVertical - VDisplay) > 0))
            {
                if ((pATI->LCDVSyncStart -= VDisplay) < 0)
                    pATI->LCDVSyncStart = 0;
                pATI->LCDVBlankWidth -= VDisplay;
                VDisplay = pATI->LCDVSyncStart + pATI->LCDVSyncWidth;
                if (pATI->LCDVBlankWidth < VDisplay)
                    pATI->LCDVBlankWidth = VDisplay;
            }
        }
    }

    /* Set RAMDAC data */
    ATIDACPreInit(pScreenInfo, pATI, pATIHW);
}

/*
 * ATIModeSave --
 *
 * This function saves the current video state.
 */
void
ATIModeSave
(
    ScrnInfoPtr pScreenInfo,
    ATIPtr      pATI,
    ATIHWPtr    pATIHW
)
{

#ifndef AVOID_CPIO

    int Index;

    /* Get bank to bank 0 */
    (*pATIHW->SetBank)(pATI, 0);

#endif /* AVOID_CPIO */

    /* Save clock data */
    ATIClockSave(pScreenInfo, pATI, pATIHW);

    if (pATI->Chip >= ATI_CHIP_264CT)
    {
        pATIHW->pll_vclk_cntl = ATIGetMach64PLLReg(PLL_VCLK_CNTL) |
            PLL_VCLK_RESET;
        pATIHW->pll_vclk_post_div = ATIGetMach64PLLReg(PLL_VCLK_POST_DIV);
        pATIHW->pll_vclk0_fb_div = ATIGetMach64PLLReg(PLL_VCLK0_FB_DIV);
        pATIHW->pll_vclk1_fb_div = ATIGetMach64PLLReg(PLL_VCLK1_FB_DIV);
        pATIHW->pll_vclk2_fb_div = ATIGetMach64PLLReg(PLL_VCLK2_FB_DIV);
        pATIHW->pll_vclk3_fb_div = ATIGetMach64PLLReg(PLL_VCLK3_FB_DIV);
        pATIHW->pll_xclk_cntl = ATIGetMach64PLLReg(PLL_XCLK_CNTL);
        if (pATI->Chip >= ATI_CHIP_264LT)
            pATIHW->pll_ext_vpll_cntl = ATIGetMach64PLLReg(PLL_EXT_VPLL_CNTL);
    }

    /* Save LCD registers */
    if (pATI->LCDPanelID >= 0)
    {
        if (pATI->Chip == ATI_CHIP_264LT)
        {
            pATIHW->horz_stretching = inr(HORZ_STRETCHING);
            pATIHW->vert_stretching = inr(VERT_STRETCHING);
            pATIHW->lcd_gen_ctrl = inr(LCD_GEN_CTRL);

            /* Set up to save non-shadow registers */
            outr(LCD_GEN_CTRL, pATIHW->lcd_gen_ctrl &
                ~(CRTC_RW_SELECT | SHADOW_EN | SHADOW_RW_EN));
        }
        else /* if ((pATI->Chip == ATI_CHIP_264LTPRO) ||
                    (pATI->Chip == ATI_CHIP_264XL) ||
                    (pATI->Chip == ATI_CHIP_MOBILITY)) */
        {
            pATIHW->lcd_index = inr(LCD_INDEX);
            pATIHW->config_panel = ATIGetMach64LCDReg(LCD_CONFIG_PANEL);
            pATIHW->lcd_gen_ctrl = ATIGetMach64LCDReg(LCD_GEN_CNTL);
            pATIHW->horz_stretching = ATIGetMach64LCDReg(LCD_HORZ_STRETCHING);
            pATIHW->vert_stretching = ATIGetMach64LCDReg(LCD_VERT_STRETCHING);
            pATIHW->ext_vert_stretch =
                ATIGetMach64LCDReg(LCD_EXT_VERT_STRETCH);

            /* Set up to save non-shadow registers */
            ATIPutMach64LCDReg(LCD_GEN_CNTL, pATIHW->lcd_gen_ctrl &
                ~(CRTC_RW_SELECT | SHADOW_EN | SHADOW_RW_EN));
        }
    }

#ifndef AVOID_CPIO

    if (pATI->VGAAdapter != ATI_ADAPTER_NONE)
    {
        /* Save VGA data */
        ATIVGASave(pATI, pATIHW);

        /* Save VGA Wonder data */
        if (pATI->CPIO_VGAWonder)
            ATIVGAWonderSave(pATI, pATIHW);
    }

    /* Save Mach64 data */
    if (pATI->Chip >= ATI_CHIP_88800GXC)

#endif /* AVOID_CPIO */

    {
        ATIMach64Save(pATI, pATIHW);
    }

    /* Save DSP data */
    if (pATI->Chip >= ATI_CHIP_264VTB)
        ATIDSPSave(pATI, pATIHW);

    if (pATI->LCDPanelID >= 0)
    {
        if (!pATI->OptionCRT)
        {
            /* Switch to shadow registers */
            if (pATI->Chip == ATI_CHIP_264LT)
                outr(LCD_GEN_CTRL, (pATIHW->lcd_gen_ctrl & ~CRTC_RW_SELECT) |
                    (SHADOW_EN | SHADOW_RW_EN));
            else /* if ((pATI->Chip == ATI_CHIP_264LTPRO) ||
                        (pATI->Chip == ATI_CHIP_264XL) ||
                        (pATI->Chip == ATI_CHIP_MOBILITY)) */
                ATIPutMach64LCDReg(LCD_GEN_CNTL,
                    (pATIHW->lcd_gen_ctrl & ~CRTC_RW_SELECT) |
                    (SHADOW_EN | SHADOW_RW_EN));

#ifndef AVOID_CPIO

            /* Save shadow VGA CRTC registers */
            for (Index = 0;  Index < NumberOf(pATIHW->shadow_vga);  Index++)
                pATIHW->shadow_vga[Index] =
                    GetReg(CRTX(pATI->CPIO_VGABase), Index);

#endif /* AVOID_CPIO */

            /* Save shadow Mach64 CRTC registers */
            pATIHW->shadow_h_total_disp = inr(CRTC_H_TOTAL_DISP);
            pATIHW->shadow_h_sync_strt_wid = inr(CRTC_H_SYNC_STRT_WID);
            pATIHW->shadow_v_total_disp = inr(CRTC_V_TOTAL_DISP);
            pATIHW->shadow_v_sync_strt_wid = inr(CRTC_V_SYNC_STRT_WID);
        }

        /* Restore CRTC selection and shadow state */
        if (pATI->Chip == ATI_CHIP_264LT)
            outr(LCD_GEN_CTRL, pATIHW->lcd_gen_ctrl);
        else /* if ((pATI->Chip == ATI_CHIP_264LTPRO) ||
                    (pATI->Chip == ATI_CHIP_264XL) ||
                    (pATI->Chip == ATI_CHIP_MOBILITY)) */
        {
            ATIPutMach64LCDReg(LCD_GEN_CNTL, pATIHW->lcd_gen_ctrl);
            outr(LCD_INDEX, pATIHW->lcd_index);
        }
    }

#ifndef AVOID_CPIO

    /*
     * For some unknown reason, CLKDIV2 needs to be turned off to save the
     * DAC's LUT reliably on VGA Wonder VLB adapters.
     */
    if ((pATI->Adapter == ATI_ADAPTER_NONISA) && (pATIHW->seq[1] & 0x08U))
        PutReg(SEQX, 0x01U, pATIHW->seq[1] & ~0x08U);

#endif /* AVOID_CPIO */

    /* Save RAMDAC state */
    ATIDACSave(pATI, pATIHW);

#ifndef AVOID_CPIO

    if ((pATI->Adapter == ATI_ADAPTER_NONISA) && (pATIHW->seq[1] & 0x08U))
        PutReg(SEQX, 0x01U, pATIHW->seq[1]);

#endif /* AVOID_CPIO */

    /*
     * The server has already saved video memory contents when switching out of
     * its virtual console, so don't do it again.
     */
    if (pATIHW != &pATI->NewHW)
    {
        pATIHW->FeedbackDivider = 0;    /* Don't programme clock */

#ifndef AVOID_CPIO

        /* Save video memory */
        ATISwap(pScreenInfo->scrnIndex, pATI, pATIHW, FALSE);

#endif /* AVOID_CPIO */

    }

#ifndef AVOID_CPIO

    if (pATI->VGAAdapter != ATI_ADAPTER_NONE)
        ATIVGASaveScreen(pATI, SCREEN_SAVER_OFF);       /* Turn on screen */

#endif /* AVOID_CPIO */

}

/*
 * ATIModeCalculate --
 *
 * This function fills in an ATIHWRec with all register values needed to enable
 * a video state.  It's important that this be done without modifying the
 * current video state.
 */
Bool
ATIModeCalculate
(
    int            iScreen,
    ATIPtr         pATI,
    ATIHWPtr       pATIHW,
    DisplayModePtr pMode
)
{
    CARD32 lcd_index;
    int Index, ECPClock;

    /* Clobber mode timings */
    if ((pATI->LCDPanelID >= 0) && !pATI->OptionCRT &&
        !pMode->CrtcHAdjusted && !pMode->CrtcVAdjusted &&
        (!pATI->OptionSync || (pMode->type & M_T_BUILTIN)))
    {
        int VScan;

        pMode->Clock = pATI->LCDClock;
        pMode->Flags &= ~(V_DBLSCAN | V_INTERLACE | V_CLKDIV2);

        /*
         * Use doublescanning or multiscanning to get around vertical blending
         * limitations.
         */
        VScan = pATI->LCDVertical / pMode->VDisplay;
        switch (pATIHW->crtc)
        {

#ifndef AVOID_CPIO

            case ATI_CRTC_VGA:
                if (VScan > 64)
                    VScan = 64;
                pMode->VScan = VScan;
                break;

#endif /* AVOID_CPIO */

            case ATI_CRTC_MACH64:
                pMode->VScan = 0;
                if (VScan <= 1)
                    break;
                VScan = 2;
                pMode->Flags |= V_DBLSCAN;
                break;

            default:
                break;
        }

        pMode->HSyncStart = pMode->HDisplay + pATI->LCDHSyncStart;
        pMode->HSyncEnd = pMode->HSyncStart + pATI->LCDHSyncWidth;
        pMode->HTotal = pMode->HDisplay + pATI->LCDHBlankWidth;

        pMode->VSyncStart = pMode->VDisplay +
            ATIDivide(pATI->LCDVSyncStart, VScan, 0, 0);
        pMode->VSyncEnd = pMode->VSyncStart +
            ATIDivide(pATI->LCDVSyncWidth, VScan, 0, 1);
        pMode->VTotal = pMode->VDisplay +
            ATIDivide(pATI->LCDVBlankWidth, VScan, 0, 0);
    }

    switch (pATIHW->crtc)
    {

#ifndef AVOID_CPIO

        case ATI_CRTC_VGA:
            /* Fill in VGA data */
            ATIVGACalculate(pATI, pATIHW, pMode);

            /* Fill in VGA Wonder data */
            if (pATI->CPIO_VGAWonder)
                ATIVGAWonderCalculate(pATI, pATIHW, pMode);

            if (pATI->Chip >= ATI_CHIP_88800GXC)
            {
                if (pATI->Chip >= ATI_CHIP_264CT)
                {
                    /*
                     * Selected bits of accelerator & VGA CRTC registers are
                     * actually copies of each other.
                     */
                    pATIHW->crtc_h_total_disp =
                        SetBits(pMode->CrtcHTotal, CRTC_H_TOTAL) |
                            SetBits(pMode->CrtcHDisplay, CRTC_H_DISP);
                    pATIHW->crtc_h_sync_strt_wid =
                        SetBits(pMode->CrtcHSyncStart, CRTC_H_SYNC_STRT) |
                            SetBits(pMode->CrtcHSkew, CRTC_H_SYNC_DLY) | /* ? */
                            SetBits(GetBits(pMode->CrtcHSyncStart, 0x0100U),
                                CRTC_H_SYNC_STRT_HI) |
                            SetBits(pMode->CrtcHSyncEnd, CRTC_H_SYNC_WID);
                    if (pMode->Flags & V_NHSYNC)
                        pATIHW->crtc_h_sync_strt_wid |= CRTC_H_SYNC_POL;

                    pATIHW->crtc_v_total_disp =
                        SetBits(pMode->CrtcVTotal, CRTC_V_TOTAL) |
                            SetBits(pMode->CrtcVDisplay, CRTC_V_DISP);
                    pATIHW->crtc_v_sync_strt_wid =
                        SetBits(pMode->CrtcVSyncStart, CRTC_V_SYNC_STRT) |
                            SetBits(pMode->CrtcVSyncEnd, CRTC_V_SYNC_WID);
                    if (pMode->Flags & V_NVSYNC)
                        pATIHW->crtc_v_sync_strt_wid |= CRTC_V_SYNC_POL;
                }

                pATIHW->crtc_gen_cntl = inr(CRTC_GEN_CNTL) &
                    ~(CRTC_DBL_SCAN_EN | CRTC_INTERLACE_EN |
                      CRTC_HSYNC_DIS | CRTC_VSYNC_DIS | CRTC_CSYNC_EN |
                      CRTC_PIX_BY_2_EN | CRTC_DISPLAY_DIS |
                      CRTC_VGA_XOVERSCAN | CRTC_VGA_128KAP_PAGING |
                      CRTC_VFC_SYNC_TRISTATE |
                      CRTC_LOCK_REGS |  /* Already off, but ... */
                      CRTC_SYNC_TRISTATE | CRTC_EXT_DISP_EN |
                      CRTC_DISP_REQ_EN | CRTC_VGA_LINEAR | CRTC_VGA_TEXT_132 |
                      CRTC_CUR_B_TEST);
#if 0           /* This isn't needed, but is kept for reference */
                if (pMode->Flags & V_DBLSCAN)
                    pATIHW->crtc_gen_cntl |= CRTC_DBL_SCAN_EN;
#endif
                if (pMode->Flags & V_INTERLACE)
                    pATIHW->crtc_gen_cntl |= CRTC_INTERLACE_EN;
                if ((pMode->Flags & (V_CSYNC | V_PCSYNC)) || pATI->OptionCSync)
                    pATIHW->crtc_gen_cntl |= CRTC_CSYNC_EN;
                if (pATI->depth <= 4)
                    pATIHW->crtc_gen_cntl |= CRTC_EN | CRTC_CNT_EN;
                else
                    pATIHW->crtc_gen_cntl |=
                        CRTC_EN | CRTC_VGA_LINEAR | CRTC_CNT_EN;
            }
            break;

#endif /* AVOID_CPIO */

        case ATI_CRTC_MACH64:
            /* Fill in Mach64 data */
            ATIMach64Calculate(pATI, pATIHW, pMode);
            break;

        default:
            break;
    }

    /* Set up LCD register values */
    if (pATI->LCDPanelID >= 0)
    {
        int VDisplay = pMode->VDisplay;

        if (pMode->Flags & V_DBLSCAN)
            VDisplay <<= 1;
        if (pMode->VScan > 1)
            VDisplay *= pMode->VScan;
        if (pMode->Flags & V_INTERLACE)
            VDisplay >>= 1;

        if (pATI->Chip == ATI_CHIP_264LT)
            pATIHW->horz_stretching = inr(HORZ_STRETCHING);
        else /* if ((pATI->Chip == ATI_CHIP_264LTPRO) ||
                    (pATI->Chip == ATI_CHIP_264XL) ||
                    (pATI->Chip == ATI_CHIP_MOBILITY)) */
        {
            lcd_index = inr(LCD_INDEX);
            pATIHW->horz_stretching = ATIGetMach64LCDReg(LCD_HORZ_STRETCHING);
            pATIHW->ext_vert_stretch =
                ATIGetMach64LCDReg(LCD_EXT_VERT_STRETCH) &
                ~(AUTO_VERT_RATIO | VERT_STRETCH_MODE);

            /*
             * Don't use vertical blending if the mode is too wide or not
             * vertically stretched.
             */
            if (!pATI->OptionCRT &&
                (pMode->HDisplay <= pATI->LCDVBlendFIFOSize) &&
                (VDisplay < pATI->LCDVertical))
                pATIHW->ext_vert_stretch |= VERT_STRETCH_MODE;

            outr(LCD_INDEX, lcd_index);
        }

        pATIHW->horz_stretching &=
            ~(HORZ_STRETCH_RATIO | HORZ_STRETCH_LOOP | AUTO_HORZ_RATIO |
              HORZ_STRETCH_MODE | HORZ_STRETCH_EN);
        if (!pATI->OptionCRT && (pMode->HDisplay < pATI->LCDHorizontal))
            pATIHW->horz_stretching |= (HORZ_STRETCH_MODE | HORZ_STRETCH_EN) |
                SetBits(((pMode->HDisplay & ~7) *
                         (MaxBits(HORZ_STRETCH_BLEND) + 1)) /
                        pATI->LCDHorizontal, HORZ_STRETCH_BLEND);

        if (pATI->OptionCRT || (VDisplay >= pATI->LCDVertical))
            pATIHW->vert_stretching = 0;
        else
        {
            pATIHW->vert_stretching = (VERT_STRETCH_USE0 | VERT_STRETCH_EN) |
                SetBits((VDisplay * (MaxBits(VERT_STRETCH_RATIO0) + 1)) /
                        pATI->LCDVertical, VERT_STRETCH_RATIO0);
        }

        if (!pATI->OptionCRT)
        {

#ifndef AVOID_CPIO

            /* Copy non-shadow CRTC register values to the shadow set */
            for (Index = 0;  Index < NumberOf(pATIHW->shadow_vga);  Index++)
                pATIHW->shadow_vga[Index] = pATIHW->crt[Index];

#endif /* AVOID_CPIO */

            pATIHW->shadow_h_total_disp = pATIHW->crtc_h_total_disp;
            pATIHW->shadow_h_sync_strt_wid = pATIHW->crtc_h_sync_strt_wid;
            pATIHW->shadow_v_total_disp = pATIHW->crtc_v_total_disp;
            pATIHW->shadow_v_sync_strt_wid = pATIHW->crtc_v_sync_strt_wid;
        }
    }

    /* Fill in clock data */
    if (!ATIClockCalculate(iScreen, pATI, pATIHW, pMode))
        return FALSE;

    /* Setup ECP clock divider */
    if (pATI->Chip >= ATI_CHIP_264VT)
    {
        pATIHW->pll_vclk_cntl &= ~PLL_ECP_DIV;
        /* XXX Don't do this for TVOut! */
        ECPClock = pMode->SynthClock;
        for (Index = 0;  (ECPClock > 125000) && (Index < 2);  Index++)
            ECPClock >>= 1;
        pATIHW->pll_vclk_cntl |= SetBits(Index, PLL_ECP_DIV);
    }

    return TRUE;
}

/*
 * ATIModeSet --
 *
 * This function sets a video mode.  It writes out all video state data that
 * has been previously calculated or saved.
 */
void
ATIModeSet
(
    ScrnInfoPtr pScreenInfo,
    ATIPtr      pATI,
    ATIHWPtr    pATIHW
)
{

#ifndef AVOID_CPIO

    int Index;

    /* Get back to bank 0 */
    (*pATIHW->SetBank)(pATI, 0);

#endif /* AVOID_CPIO */

    if (pATI->Chip >= ATI_CHIP_264CT)
    {
        ATIPutMach64PLLReg(PLL_VCLK_CNTL, pATIHW->pll_vclk_cntl);
        ATIPutMach64PLLReg(PLL_VCLK_POST_DIV, pATIHW->pll_vclk_post_div);
        ATIPutMach64PLLReg(PLL_VCLK0_FB_DIV, pATIHW->pll_vclk0_fb_div);
        ATIPutMach64PLLReg(PLL_VCLK1_FB_DIV, pATIHW->pll_vclk1_fb_div);
        ATIPutMach64PLLReg(PLL_VCLK2_FB_DIV, pATIHW->pll_vclk2_fb_div);
        ATIPutMach64PLLReg(PLL_VCLK3_FB_DIV, pATIHW->pll_vclk3_fb_div);
        ATIPutMach64PLLReg(PLL_XCLK_CNTL, pATIHW->pll_xclk_cntl);
        if (pATI->Chip >= ATI_CHIP_264LT)
            ATIPutMach64PLLReg(PLL_EXT_VPLL_CNTL, pATIHW->pll_ext_vpll_cntl);
        ATIPutMach64PLLReg(PLL_VCLK_CNTL,
            pATIHW->pll_vclk_cntl & ~PLL_VCLK_RESET);
    }

    /* Load LCD registers */
    if (pATI->LCDPanelID >= 0)
    {
        /* Stop CRTC */
        outr(CRTC_GEN_CNTL, pATIHW->crtc_gen_cntl &
            ~(CRTC_EXT_DISP_EN | CRTC_EN));

        if (pATI->Chip == ATI_CHIP_264LT)
        {
            /* Update non-shadow registers first */
            outr(LCD_GEN_CTRL, pATIHW->lcd_gen_ctrl &
                ~(DISABLE_PCLK_RESET | CRTC_RW_SELECT |
                  SHADOW_EN | SHADOW_RW_EN));

            /* Temporarily disable stretching */
            outr(HORZ_STRETCHING, pATIHW->horz_stretching &
                ~(HORZ_STRETCH_MODE | HORZ_STRETCH_EN));
            outr(VERT_STRETCHING, pATIHW->vert_stretching &
                ~(VERT_STRETCH_RATIO1 | VERT_STRETCH_RATIO2 |
                  VERT_STRETCH_USE0 | VERT_STRETCH_EN));
        }
        else /* if ((pATI->Chip == ATI_CHIP_264LTPRO) ||
                    (pATI->Chip == ATI_CHIP_264XL) ||
                    (pATI->Chip == ATI_CHIP_MOBILITY)) */
        {
            /* Update non-shadow registers first */
            ATIPutMach64LCDReg(LCD_CONFIG_PANEL, pATIHW->config_panel);
            ATIPutMach64LCDReg(LCD_GEN_CNTL, pATIHW->lcd_gen_ctrl &
                ~(DISABLE_PCLK_RESET | CRTC_RW_SELECT |
                  SHADOW_EN | SHADOW_RW_EN));

            /* Temporarily disable stretching */
            ATIPutMach64LCDReg(LCD_HORZ_STRETCHING, pATIHW->horz_stretching &
                ~(HORZ_STRETCH_MODE | HORZ_STRETCH_EN));
            ATIPutMach64LCDReg(LCD_VERT_STRETCHING, pATIHW->vert_stretching &
                ~(VERT_STRETCH_RATIO1 | VERT_STRETCH_RATIO2 |
                  VERT_STRETCH_USE0 | VERT_STRETCH_EN));
        }
    }

    switch (pATIHW->crtc)
    {

#ifndef AVOID_CPIO

        case ATI_CRTC_VGA:
            /* Stop CRTC */
            if (pATI->Chip >= ATI_CHIP_88800GXC)
                outr(CRTC_GEN_CNTL, pATIHW->crtc_gen_cntl & ~CRTC_EN);

            /* Start sequencer reset */
            PutReg(SEQX, 0x00U, 0x00U);

            /* Set pixel clock */
            if ((pATIHW->FeedbackDivider > 0) &&
                (pATI->ProgrammableClock > ATI_CLOCK_FIXED))
                ATIClockSet(pATI, pATIHW);

            /* Load VGA Wonder */
            if (pATI->CPIO_VGAWonder)
                ATIVGAWonderSet(pATI, pATIHW);

            /* Load VGA device */
            ATIVGASet(pATI, pATIHW);

            /* Load Mach64 registers */
            if (pATI->Chip >= ATI_CHIP_88800GXC)
            {
                outr(CRTC_GEN_CNTL, pATIHW->crtc_gen_cntl);
                outr(MEM_VGA_WP_SEL, pATIHW->mem_vga_wp_sel);
                outr(MEM_VGA_RP_SEL, pATIHW->mem_vga_rp_sel);
                outr(CONFIG_CNTL, pATIHW->config_cntl);
                if (pATI->Chip >= ATI_CHIP_264CT)
                {
                    outr(CRTC_H_TOTAL_DISP, pATIHW->crtc_h_total_disp);
                    outr(CRTC_H_SYNC_STRT_WID, pATIHW->crtc_h_sync_strt_wid);
                    outr(CRTC_V_TOTAL_DISP, pATIHW->crtc_v_total_disp);
                    outr(CRTC_V_SYNC_STRT_WID, pATIHW->crtc_v_sync_strt_wid);
                    outr(CRTC_OFF_PITCH, pATIHW->crtc_off_pitch);
                    outr(BUS_CNTL, pATIHW->bus_cntl);
                    outr(DAC_CNTL, pATIHW->dac_cntl);
                }
            }

            break;

#endif /* AVOID_CPIO */

        case ATI_CRTC_MACH64:
            /* Load Mach64 CRTC registers */
            ATIMach64Set(pATI, pATIHW);

#ifndef AVOID_CPIO

            if (pATI->UseSmallApertures)
            {
                /* Oddly enough, these need to be set also, maybe others */
                PutReg(SEQX, 0x02U, pATIHW->seq[2]);
                PutReg(SEQX, 0x04U, pATIHW->seq[4]);
                PutReg(GRAX, 0x06U, pATIHW->gra[6]);
                if (pATI->CPIO_VGAWonder)
                    ATIModifyExtReg(pATI, 0xB6U, -1, 0x00U, pATIHW->b6);
            }

#endif /* AVOID_CPIO */

            break;

        default:
            break;
    }

    if (pATI->LCDPanelID >= 0)
    {
        if (!pATI->OptionCRT &&
            (!pATI->OptionDevel || (pATIHW == &pATI->OldHW)))
        {
            /* Switch to shadow registers */
            if (pATI->Chip == ATI_CHIP_264LT)
                outr(LCD_GEN_CTRL, (pATIHW->lcd_gen_ctrl &
                     ~(DISABLE_PCLK_RESET | CRTC_RW_SELECT)) |
                    (SHADOW_EN | SHADOW_RW_EN));
            else /* if ((pATI->Chip == ATI_CHIP_264LTPRO) ||
                        (pATI->Chip == ATI_CHIP_264XL) ||
                        (pATI->Chip == ATI_CHIP_MOBILITY)) */
                ATIPutMach64LCDReg(LCD_GEN_CNTL,
                    (pATIHW->lcd_gen_ctrl &
                     ~(DISABLE_PCLK_RESET | CRTC_RW_SELECT)) |
                    (SHADOW_EN | SHADOW_RW_EN));

            /* Restore shadow registers */
            switch (pATIHW->crtc)
            {

#ifndef AVOID_CPIO

                case ATI_CRTC_VGA:
                    for (Index = 0;
                         Index < NumberOf(pATIHW->shadow_vga);
                         Index++)
                        PutReg(CRTX(pATI->CPIO_VGABase), Index,
                            pATIHW->shadow_vga[Index]);
                    /* Fall through */

#endif /* AVOID_CPIO */

                case ATI_CRTC_MACH64:
                    outr(CRTC_H_TOTAL_DISP, pATIHW->shadow_h_total_disp);
                    outr(CRTC_H_SYNC_STRT_WID, pATIHW->shadow_h_sync_strt_wid);
                    outr(CRTC_V_TOTAL_DISP, pATIHW->shadow_v_total_disp);
                    outr(CRTC_V_SYNC_STRT_WID, pATIHW->shadow_v_sync_strt_wid);
                    break;

                default:
                    break;
            }
        }

        /* Restore CRTC selection & shadow state and enable stretching */
        if (pATI->Chip == ATI_CHIP_264LT)
        {
            outr(LCD_GEN_CTRL, pATIHW->lcd_gen_ctrl);
            outr(HORZ_STRETCHING, pATIHW->horz_stretching);
            outr(VERT_STRETCHING, pATIHW->vert_stretching);
        }
        else /* if ((pATI->Chip == ATI_CHIP_264LTPRO) ||
                    (pATI->Chip == ATI_CHIP_264XL) ||
                    (pATI->Chip == ATI_CHIP_MOBILITY)) */
        {
            ATIPutMach64LCDReg(LCD_GEN_CNTL, pATIHW->lcd_gen_ctrl);
            ATIPutMach64LCDReg(LCD_HORZ_STRETCHING, pATIHW->horz_stretching);
            ATIPutMach64LCDReg(LCD_VERT_STRETCHING, pATIHW->vert_stretching);
            ATIPutMach64LCDReg(LCD_EXT_VERT_STRETCH, pATIHW->ext_vert_stretch);
            outr(LCD_INDEX, pATIHW->lcd_index);
        }
    }

    /*
     * Set DSP registers.  Note that, for some reason, sequencer resets clear
     * the DSP_CONFIG register on early integrated controllers.
     */
    if (pATI->Chip >= ATI_CHIP_264VTB)
        ATIDSPSet(pATI, pATIHW);

    /* Load RAMDAC */
    ATIDACSet(pATI, pATIHW);

#ifndef AVOID_CPIO

    /* Restore video memory */
    ATISwap(pScreenInfo->scrnIndex, pATI, pATIHW, TRUE);

    if (pATI->VGAAdapter != ATI_ADAPTER_NONE)
        ATIVGASaveScreen(pATI, SCREEN_SAVER_OFF);       /* Turn on screen */

#endif /* AVOID_CPIO */

    if ((xf86GetVerbosity() > 3) && (pATIHW == &pATI->NewHW))
    {
        xf86ErrorFVerb(4, "\n After setting mode \"%s\":\n\n",
            pScreenInfo->currentMode->name);
        ATIPrintMode(pScreenInfo->currentMode);
        ATIPrintRegisters(pATI);
    }
}

/*
 * ATIModeAccelInit --
 *
 * This function is called to initialise XAA on a screen.
 */
Bool
ATIModeAccelInit
(
    ScrnInfoPtr pScreenInfo,
    ScreenPtr   pScreen,
    ATIPtr      pATI
)
{
    BoxRec ScreenArea;

    if (!pATI->OptionAccel)
        return TRUE;

    if (!(pATI->pXAAInfo = XAACreateInfoRec()))
        return FALSE;

    switch (pATI->Adapter)
    {
        case ATI_ADAPTER_MACH64:
            if (ATIMach64AccelInit(pATI, pATI->pXAAInfo))
                break;

        default:
            XAADestroyInfoRec(pATI->pXAAInfo);
            pATI->pXAAInfo = NULL;
            return FALSE;
    }

    ScreenArea.x1 = ScreenArea.y1 = 0;
    ScreenArea.x2 = pATI->displayWidth;
    ScreenArea.y2 = pScreenInfo->videoRam * 1024 * 8 / pATI->displayWidth /
        pATI->bitsPerPixel;
    if ((unsigned)ScreenArea.y2 > ATIMach64MaxY)
        ScreenArea.y2 = ATIMach64MaxY;
    xf86InitFBManager(pScreen, &ScreenArea);

    if (XAAInit(pScreen, pATI->pXAAInfo))
        return TRUE;

    XAADestroyInfoRec(pATI->pXAAInfo);
    pATI->pXAAInfo = NULL;
    return FALSE;
}
