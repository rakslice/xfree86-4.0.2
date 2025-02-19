/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/sis/sis_vb.h,v 1.3 2000/12/02 01:16:18 dawes Exp $ */

/* CR30 VBInfo = CR31:CR30 */
#define	SET_SIMU_SCAN_MODE	0x0001
#define	SWITCH_TO_CRT2		0x0002
#define	SET_CRT2_TO_AVIDEO	0x0004		/* Composite */
#define	SET_CRT2_TO_SVIDEO	0x0008
#define	SET_CRT2_TO_SCART	0x0010
#define	SET_CRT2_TO_LCD		0x0020
#define	SET_CRT2_TO_RAMDAC	0x0040
#define	SET_CRT2_TO_HIVISION_TV	0x0080
#define	SET_CRT2_TO_TV		(SET_CRT2_TO_AVIDEO | SET_CRT2_TO_SVIDEO | \
				SET_CRT2_TO_SCART | SET_CRT2_TO_HIVISION_TV)
/* CR31 */
#define	SET_PAL_TV		0x0100
#define	SET_IN_SLAVE_MODE	0x0200
#define	SET_NO_SIMU_ON_LOCK	0x0400
#define	SET_NO_SIMU_TV_ON_LOCK			SET_NO_SIMU_ON_LOCK
#define	DISABLE_LOAD_CRT2DAC	0x1000
#define	DISABLE_CRT2_DISPLAY	0x2000
#define	DRIVER_MODE		0x4000  

Bool    SiSSetMode(ScrnInfoPtr pScrn, DisplayModePtr mode);
void	SetBlock(CARD16 port, CARD8 from, CARD8 to, CARD8 *DataPtr);

extern void   EnableBridge(unsigned short);
extern void   DisableBridge(unsigned short);
extern void   LockCRT2(unsigned short);
extern void   UnLockCRT2(unsigned short);
extern void   SetCH7005(unsigned short);
extern unsigned short GetCH7005(unsigned short);

typedef	struct	_SiS301Reg  {
	CARD8	*VBPart1;
	CARD8	*VBPart2;
	CARD8	*VBPart3;
	CARD8	*VBPart4;
} SiS301RegRec, SiS301RegPtr;

