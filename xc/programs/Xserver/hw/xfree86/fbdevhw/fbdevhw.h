/* $XFree86: xc/programs/Xserver/hw/xfree86/fbdevhw/fbdevhw.h,v 1.9 2000/11/18 19:37:14 tsi Exp $ */

#ifndef _FBDEVHW_H_
#define _FBDEVHW_H_

#include "xf86str.h"
#include "colormapst.h"

#define FBDEVHW_PACKED_PIXELS		0	/* Packed Pixels	*/
#define FBDEVHW_PLANES			1	/* Non interleaved planes */
#define FBDEVHW_INTERLEAVED_PLANES	2	/* Interleaved planes	*/
#define FBDEVHW_TEXT			3	/* Text/attributes	*/
#define FBDEVHW_VGA_PLANES		4	/* EGA/VGA planes       */

Bool  fbdevHWGetRec(ScrnInfoPtr pScrn);
void  fbdevHWFreeRec(ScrnInfoPtr pScrn);

Bool  fbdevHWProbe(pciVideoPtr pPci, char *device, char **namep);
Bool  fbdevHWInit(ScrnInfoPtr pScrn, pciVideoPtr pPci, char *device);

char* fbdevHWGetName(ScrnInfoPtr pScrn);
int   fbdevHWGetDepth(ScrnInfoPtr pScrn);
int   fbdevHWGetLineLength(ScrnInfoPtr pScrn);
int   fbdevHWGetType(ScrnInfoPtr pScrn);
int   fbdevHWGetVidmem(ScrnInfoPtr pScrn);

void* fbdevHWMapVidmem(ScrnInfoPtr pScrn);
int   fbdevHWLinearOffset(ScrnInfoPtr pScrn);
Bool  fbdevHWUnmapVidmem(ScrnInfoPtr pScrn);
void* fbdevHWMapMMIO(ScrnInfoPtr pScrn);
Bool  fbdevHWUnmapMMIO(ScrnInfoPtr pScrn);

void  fbdevHWSetVideoModes(ScrnInfoPtr pScrn);
DisplayModePtr fbdevHWGetBuildinMode(ScrnInfoPtr pScrn);
void  fbdevHWUseBuildinMode(ScrnInfoPtr pScrn);
Bool  fbdevHWModeInit(ScrnInfoPtr pScrn, DisplayModePtr mode);
void  fbdevHWSave(ScrnInfoPtr pScrn);
void  fbdevHWRestore(ScrnInfoPtr pScrn);

void  fbdevHWLoadPalette(ScrnInfoPtr pScrn, int numColors, int *indices,
		 LOCO *colors, VisualPtr pVisual);

int   fbdevHWValidMode(int scrnIndex, DisplayModePtr mode, Bool verbose, int flags);
Bool  fbdevHWSwitchMode(int scrnIndex, DisplayModePtr mode, int flags);
void  fbdevHWAdjustFrame(int scrnIndex, int x, int y, int flags);
Bool  fbdevHWEnterVT(int scrnIndex, int flags);
void  fbdevHWLeaveVT(int scrnIndex, int flags);
void  fbdevHWDPMSSet(ScrnInfoPtr pScrn, int mode, int flags);

#endif
