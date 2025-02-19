/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/i810/i810_dri.c,v 1.13 2000/12/01 14:28:56 dawes Exp $ */

#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86_ansic.h"
#include "xf86Priv.h"

#include "xf86PciInfo.h"
#include "xf86Pci.h"

#include "windowstr.h"

#include "GL/glxtokens.h"

#include "i810.h"
#include "i810_dri.h"

static char I810KernelDriverName[] = "i810";
static char I810ClientDriverName[] = "i810";

static Bool I810InitVisualConfigs(ScreenPtr pScreen);
static Bool I810CreateContext(ScreenPtr pScreen, VisualPtr visual, 
			      drmContext hwContext, void *pVisualConfigPriv,
			      DRIContextType contextStore);
static void I810DestroyContext(ScreenPtr pScreen, drmContext hwContext,
			       DRIContextType contextStore);
static void I810DRISwapContext(ScreenPtr pScreen, DRISyncType syncType, 
			       DRIContextType readContextType, 
			       void *readContextStore,
			       DRIContextType writeContextType, 
			       void *writeContextStore);
static void I810DRIInitBuffers(WindowPtr pWin, RegionPtr prgn, CARD32 index);
static void I810DRIMoveBuffers(WindowPtr pParent, DDXPointRec ptOldOrg, 
			       RegionPtr prgnSrc, CARD32 index);

extern void GlxSetVisualConfigs(int nconfigs,
				__GLXvisualConfig *configs,
				void **configprivs);

static int i810_pitches[] = {
   512,
   1024,
   2048,
   4096,
   0
};

static int i810_pitch_flags[] = {
    0x0,
    0x1,
    0x2,
    0x3,
    0
};

Bool I810CleanupDma(ScrnInfoPtr pScrn)
{
   I810Ptr pI810 = I810PTR(pScrn);
   Bool ret_val;
   
   ret_val = drmI810CleanupDma(pI810->drmSubFD);
   if (ret_val == FALSE)
      xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "I810 Dma Cleanup Failed\n");
   return ret_val;
}

Bool I810InitDma(ScrnInfoPtr pScrn)
{
   I810Ptr pI810 = I810PTR(pScrn);
   I810RingBuffer *ring = &(pI810->LpRing);
   drmI810Init info;
   Bool ret_val;
   
   info.start = ring->mem.Start;
   info.end = ring->mem.End; 
   info.size = ring->mem.Size;
   info.ring_map_idx = 6; 
   info.buffer_map_idx = 5; 
   info.sarea_off = sizeof(XF86DRISAREARec);

   info.front_offset = 0;
   info.back_offset = pI810->BackBuffer.Start;
   info.depth_offset = pI810->DepthBuffer.Start;
   info.w = pScrn->virtualX;
   info.h = pScrn->virtualY;
   info.pitch = pI810->auxPitch;
   info.pitch_bits = pI810->auxPitchBits;

   ret_val = drmI810InitDma(pI810->drmSubFD, &info);
   if(ret_val == FALSE) ErrorF("I810 Dma Initialization Failed\n");
   return ret_val;
}

static Bool
I810InitVisualConfigs(ScreenPtr pScreen)
{
   ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
   I810Ptr pI810 = I810PTR(pScrn);
   int numConfigs = 0;
   __GLXvisualConfig *pConfigs = 0;
   I810ConfigPrivPtr pI810Configs = 0;
   I810ConfigPrivPtr *pI810ConfigPtrs = 0;
   int accum, stencil, db, depth;
   int i;

   switch (pScrn->bitsPerPixel) {
   case 8:
   case 24:
   case 32:
      break;
   case 16:
      numConfigs = 8;

      pConfigs = (__GLXvisualConfig *) xnfcalloc(sizeof(__GLXvisualConfig), numConfigs);
      if (!pConfigs)
	 return FALSE;

      pI810Configs = (I810ConfigPrivPtr) xnfcalloc(sizeof(I810ConfigPrivRec), numConfigs);
      if (!pI810Configs) {
	 xfree(pConfigs);
	 return FALSE;
      }

      pI810ConfigPtrs = (I810ConfigPrivPtr *) xnfcalloc(sizeof(I810ConfigPrivPtr), numConfigs);
      if (!pI810ConfigPtrs) {
	 xfree(pConfigs);
	 xfree(pI810Configs);
	 return FALSE;
      }

      for (i=0; i<numConfigs; i++) 
	 pI810ConfigPtrs[i] = &pI810Configs[i];

      i = 0;
      depth = 1;
      for (accum = 0; accum <= 1; accum++) {
         for (stencil = 0; stencil <= 1; stencil++) {
            for (db = 1; db >= 0; db--) {
               pConfigs[i].vid = -1;
               pConfigs[i].class = -1;
               pConfigs[i].rgba = TRUE;
               pConfigs[i].redSize = 5;
               pConfigs[i].greenSize = 6;
               pConfigs[i].blueSize = 5;
               pConfigs[i].redMask = 0x0000F800;
               pConfigs[i].greenMask = 0x000007E0;
               pConfigs[i].blueMask = 0x0000001F;
               pConfigs[i].alphaMask = 0;
               if (accum) {
                  pConfigs[i].accumRedSize = 16;
                  pConfigs[i].accumGreenSize = 16;
                  pConfigs[i].accumBlueSize = 16;
                  pConfigs[i].accumAlphaSize = 16;
               }
               else {
                  pConfigs[i].accumRedSize = 0;
                  pConfigs[i].accumGreenSize = 0;
                  pConfigs[i].accumBlueSize = 0;
                  pConfigs[i].accumAlphaSize = 0;
               }
               pConfigs[i].doubleBuffer = db ? TRUE : FALSE;
               pConfigs[i].stereo = FALSE;
               pConfigs[i].bufferSize = 16;
               if (depth)
                  pConfigs[i].depthSize = 16;
               else
                  pConfigs[i].depthSize = 0;
               if (stencil)
                  pConfigs[i].stencilSize = 8;
               else 
                  pConfigs[i].stencilSize = 0;
               pConfigs[i].auxBuffers = 0;
               pConfigs[i].level = 0;
               if (stencil || accum)
                  pConfigs[i].visualRating = GLX_SLOW_VISUAL_EXT;
               else
                  pConfigs[i].visualRating = GLX_NONE_EXT;
               pConfigs[i].transparentPixel = 0;
               pConfigs[i].transparentRed = 0;
               pConfigs[i].transparentGreen = 0;
               pConfigs[i].transparentBlue = 0;
               pConfigs[i].transparentAlpha = 0;
               pConfigs[i].transparentIndex = 0;
               i++;
            }
         }
      }
      assert(i == numConfigs);
      break;
   }
   pI810->numVisualConfigs = numConfigs;
   pI810->pVisualConfigs = pConfigs;
   pI810->pVisualConfigsPriv = pI810Configs;
   GlxSetVisualConfigs(numConfigs, pConfigs, (void**)pI810ConfigPtrs);
   return TRUE;
}


static unsigned int mylog2(unsigned int n)
{
   unsigned int log2 = 1;
   while (n>1) n >>= 1, log2++;
   return log2;
}


Bool I810DRIScreenInit(ScreenPtr pScreen)
{
   ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
   I810Ptr pI810 = I810PTR(pScrn);
   DRIInfoPtr pDRIInfo;
   I810DRIPtr pI810DRI;
   unsigned long tom;
   unsigned long agpHandle;
   unsigned long dcacheHandle;
   int sysmem_size = 0;
   int back_size = 0;
   int pitch_idx = 0;
   int bufs;
   int width = pScrn->displayWidth * pI810->cpp;
   int i;
   
   /* Hardware 3D rendering only implemented for 16bpp */
   /* And it only works for 5:6:5 (Mark) */
   if (pScrn->depth != 16)
      return FALSE;

   /* Check that the GLX, DRI, and DRM modules have been loaded by testing
    * for known symbols in each module. */
   if (!xf86LoaderCheckSymbol("GlxSetVisualConfigs")) return FALSE;
   if (!xf86LoaderCheckSymbol("DRIScreenInit"))       return FALSE;
   if (!xf86LoaderCheckSymbol("drmAvailable"))        return FALSE;
   if (!xf86LoaderCheckSymbol("DRIQueryVersion")) {
      xf86DrvMsg(pScreen->myNum, X_ERROR,
                 "TDFXDRIScreenInit failed (libdri.a too old)\n");
      return FALSE;
   }
   
   /* Check the DRI version */
   {
      int major, minor, patch;
      DRIQueryVersion(&major, &minor, &patch);
      if (major != 3 || minor != 0 || patch < 0) {
         xf86DrvMsg(pScreen->myNum, X_ERROR,
                    "I810DRIScreenInit failed (DRI version = %d.%d.%d, expected 3.0.x).  Disabling DRI.\n",
                    major, minor, patch);
         return FALSE;
      }
   }

   pDRIInfo = DRICreateInfoRec();
   if (!pDRIInfo) {
      xf86DrvMsg(pScreen->myNum, X_ERROR, "DRICreateInfoRec failed\n");
      return FALSE;
   }


/*     pDRIInfo->wrap.ValidateTree = 0;    */
/*     pDRIInfo->wrap.PostValidateTree = 0;    */

   pI810->pDRIInfo = pDRIInfo;
   pI810->LockHeld = 0;

   pDRIInfo->drmDriverName = I810KernelDriverName;
   pDRIInfo->clientDriverName = I810ClientDriverName;
   pDRIInfo->busIdString = xalloc(64);

   sprintf(pDRIInfo->busIdString, "PCI:%d:%d:%d",
	   ((pciConfigPtr)pI810->PciInfo->thisCard)->busnum,
	   ((pciConfigPtr)pI810->PciInfo->thisCard)->devnum,
	   ((pciConfigPtr)pI810->PciInfo->thisCard)->funcnum);
   pDRIInfo->ddxDriverMajorVersion = I810_MAJOR_VERSION;
   pDRIInfo->ddxDriverMinorVersion = I810_MINOR_VERSION;
   pDRIInfo->ddxDriverPatchVersion = I810_PATCHLEVEL;
   pDRIInfo->frameBufferPhysicalAddress = pI810->LinearAddr;
   pDRIInfo->frameBufferSize = (((pScrn->displayWidth * 
				  pScrn->virtualY * pI810->cpp) + 
				 4096 - 1) / 4096) * 4096;

   pDRIInfo->frameBufferStride = pScrn->displayWidth*pI810->cpp;
   pDRIInfo->ddxDrawableTableEntry = I810_MAX_DRAWABLES;

   if (SAREA_MAX_DRAWABLES < I810_MAX_DRAWABLES)
      pDRIInfo->maxDrawableTableEntry = SAREA_MAX_DRAWABLES;
   else
      pDRIInfo->maxDrawableTableEntry = I810_MAX_DRAWABLES;

   /* For now the mapping works by using a fixed size defined
    * in the SAREA header
    */
   if (sizeof(XF86DRISAREARec)+sizeof(I810SAREARec)>SAREA_MAX) {
      xf86DrvMsg(pScreen->myNum, X_ERROR, "Data does not fit in SAREA\n");
      return FALSE;
   }
   pDRIInfo->SAREASize = SAREA_MAX;

   if (!(pI810DRI = (I810DRIPtr)xnfcalloc(sizeof(I810DRIRec),1))) {
      DRIDestroyInfoRec(pI810->pDRIInfo);
      pI810->pDRIInfo=0;
      return FALSE;
   }
   pDRIInfo->devPrivate = pI810DRI;
   pDRIInfo->devPrivateSize = sizeof(I810DRIRec);
   pDRIInfo->contextSize = sizeof(I810DRIContextRec);
   
   pDRIInfo->CreateContext = I810CreateContext;
   pDRIInfo->DestroyContext = I810DestroyContext;
   pDRIInfo->SwapContext = I810DRISwapContext;
   pDRIInfo->InitBuffers = I810DRIInitBuffers;
   pDRIInfo->MoveBuffers = I810DRIMoveBuffers;
   pDRIInfo->bufferRequests = DRI_ALL_WINDOWS;
   
   
   /* This adds the framebuffer as a drm map *before* we have asked agp
    * to allocate it.  Scary stuff, hold on...
    */
   if (!DRIScreenInit(pScreen, pDRIInfo, &pI810->drmSubFD)) {
      xf86DrvMsg(pScreen->myNum, X_ERROR, "DRIScreenInit failed\n");
      xfree(pDRIInfo->devPrivate);
      pDRIInfo->devPrivate=0;
      DRIDestroyInfoRec(pI810->pDRIInfo);
      pI810->pDRIInfo=0;
      return FALSE;
   }
   
   /* Check the i810 DRM version */
   {
      drmVersionPtr version = drmGetVersion(pI810->drmSubFD);
      if (version) {
         if (version->version_major != 1 ||
             version->version_minor != 1 ||
             version->version_patchlevel < 0) {
            /* incompatible drm version */
            xf86DrvMsg(pScreen->myNum, X_ERROR,
                       "I810DRIScreenInit failed (DRM version = %d.%d.%d, expected 1.0.x).  Disabling DRI.\n",
                       version->version_major,
                       version->version_minor,
                       version->version_patchlevel);
            I810DRICloseScreen(pScreen);
            drmFreeVersion(version);
            return FALSE;
         }
         drmFreeVersion(version);
      }
   }

   pI810DRI->regsSize=I810_REG_SIZE;
   if (drmAddMap(pI810->drmSubFD, (drmHandle)pI810->MMIOAddr, 
		 pI810DRI->regsSize, DRM_REGISTERS, 0, &pI810DRI->regs)<0) {
      xf86DrvMsg(pScreen->myNum, X_ERROR, "drmAddMap(regs) failed\n");
      DRICloseScreen(pScreen);
      return FALSE;
   }
   xf86DrvMsg(pScreen->myNum, X_INFO, "[drm] Registers = 0x%08lx\n",
	      pI810DRI->regs);
   
   pI810->backHandle = 0;
   pI810->zHandle = 0;
   pI810->cursorHandle = 0;
   pI810->sysmemHandle = 0;
   pI810->agpAcquired = FALSE;
   pI810->dcacheHandle = 0;
   
   /* Agp Support - Need this just to get the framebuffer.
    */
   if(drmAgpAcquire(pI810->drmSubFD) < 0) {
      xf86DrvMsg(pScreen->myNum, X_ERROR, "drmAgpAquire failed\n");
      DRICloseScreen(pScreen);
      return FALSE;
   }
   pI810->agpAcquired = TRUE;
   
   if (drmAgpEnable(pI810->drmSubFD, 0) < 0) {
      xf86DrvMsg(pScreen->myNum, X_ERROR, "drmAgpEnable failed\n");
      DRICloseScreen(pScreen);
      return FALSE;
   }

   memset (&pI810->DcacheMem, 0, sizeof(I810MemRange));
   memset (&pI810->BackBuffer, 0, sizeof(I810MemRange));
   memset (&pI810->DepthBuffer, 0, sizeof(I810MemRange));
   pI810->CursorPhysical = 0;
   
   /* Dcache - half the speed of normal ram, but has use as a Z buffer
    * under the DRI.  
    */

   drmAgpAlloc(pI810->drmSubFD, 4096 * 1024, 1, NULL, &dcacheHandle);
   pI810->dcacheHandle = dcacheHandle;
   xf86DrvMsg(pScreen->myNum, X_INFO, "dcacheHandle : %p\n", dcacheHandle);
   
#define Elements(x) sizeof(x)/sizeof(*x)
   for (pitch_idx = 0 ; pitch_idx < Elements(i810_pitches) ; pitch_idx++) 
      if (width <= i810_pitches[pitch_idx]) 
	 break;
   
   if (pitch_idx == Elements(i810_pitches)) {
      xf86DrvMsg(pScrn->scrnIndex, X_INFO, 
		 "Couldn't find depth/back buffer pitch");
      DRICloseScreen(pScreen);
      return FALSE;
   }
   else {
      back_size = i810_pitches[pitch_idx] * (pScrn->virtualY + 4);
      back_size = ((back_size + 4096 - 1) / 4096) * 4096;
   }
   
   sysmem_size = pScrn->videoRam * 1024;
   if (dcacheHandle != 0) {
      if (back_size > 4*1024*1024) {
	 xf86DrvMsg(pScreen->myNum, X_INFO, "Backsize is larger then 4 meg\n");
	 sysmem_size = sysmem_size - 2*back_size;
	 drmAgpFree(pI810->drmSubFD, dcacheHandle);
	 pI810->dcacheHandle = dcacheHandle = 0;
      }
      else {
	 sysmem_size = sysmem_size - back_size;
      }
   }
   else {
      sysmem_size = sysmem_size - 2*back_size;
   }

   if(sysmem_size > 48*1024*1024) {
      sysmem_size = 48*1024*1024;

      xf86DrvMsg(pScrn->scrnIndex, X_INFO, 
		 "User requested more memory then fits in the agp aperture\n"
		 "Truncating to %d bytes of memory\n",
		 sysmem_size);
   }

   sysmem_size -= 4096;		/* remove 4k for the hw cursor */

   pI810->SysMem.Start = 0;
   pI810->SysMem.Size = sysmem_size;
   pI810->SysMem.End = sysmem_size;
   tom = sysmem_size;

   pI810->SavedSysMem = pI810->SysMem;

   if (dcacheHandle != 0) {
      /* The Z buffer is always aligned to the 48 mb mark in the aperture */
      if(drmAgpBind(pI810->drmSubFD, dcacheHandle, 48*1024*1024) == 0) {
	 memset (&pI810->DcacheMem, 0, sizeof(I810MemRange));
	 xf86DrvMsg(pScrn->scrnIndex, X_INFO, 
		    "GART: Found 4096K Z buffer memory\n");
	 pI810->DcacheMem.Start = 48*1024*1024;
	 pI810->DcacheMem.Size = 1024 * 4096;
	 pI810->DcacheMem.End = pI810->DcacheMem.Start + pI810->DcacheMem.Size;
	 if (!I810AllocLow(&(pI810->DepthBuffer), 
			   &(pI810->DcacheMem),
			   back_size)) 
	 {
	    xf86DrvMsg(pScrn->scrnIndex, X_INFO, 
		       "Depth buffer allocation failed\n");
	    DRICloseScreen(pScreen);
	    return FALSE;
	 }
      } else {
	 xf86DrvMsg(pScrn->scrnIndex, X_INFO, "GART: dcache bind failed\n");
	 drmAgpFree(pI810->drmSubFD, dcacheHandle);
	 pI810->dcacheHandle = dcacheHandle = 0;
      }   
   } else {
      xf86DrvMsg(pScrn->scrnIndex, X_INFO, "GART: no dcache memory found\n");
   }

   drmAgpAlloc(pI810->drmSubFD, back_size, 0, NULL, &agpHandle);
   pI810->backHandle = agpHandle;
   
   if (agpHandle != 0) {
      /* The backbuffer is always aligned to the 56 mb mark in the aperture */
      if(drmAgpBind(pI810->drmSubFD, agpHandle, 56*1024*1024) == 0) {
	 xf86DrvMsg(pScrn->scrnIndex, X_INFO,
		    "Bound backbuffer memory\n");
	 
	 pI810->BackBuffer.Start = 56*1024*1024;
	 pI810->BackBuffer.Size = back_size;
	 pI810->BackBuffer.End = (pI810->BackBuffer.Start + 
				  pI810->BackBuffer.Size);
      } else {
	 xf86DrvMsg(pScrn->scrnIndex, X_INFO, "Unable to bind backbuffer\n");
	 DRICloseScreen(pScreen);
	 return FALSE;
      }
   } else {
      xf86DrvMsg(pScrn->scrnIndex, X_INFO,
		 "Unable to allocate backbuffer memory\n");
      DRICloseScreen(pScreen);
      return FALSE;
   }
      
   if(dcacheHandle == 0) {
      /* The Z buffer is always aligned to the 48 mb mark in the aperture */
      drmAgpAlloc(pI810->drmSubFD, back_size, 0,
		  NULL, &agpHandle);
      pI810->zHandle = agpHandle;

      if(agpHandle != 0) {
	 if(drmAgpBind(pI810->drmSubFD, agpHandle, 48*1024*1024) == 0) {
	    xf86DrvMsg(pScrn->scrnIndex, X_INFO, "Bound depthbuffer memory\n");
	    pI810->DepthBuffer.Start = 48*1024*1024;
	    pI810->DepthBuffer.Size = back_size;
	    pI810->DepthBuffer.End = (pI810->DepthBuffer.Start + 
				      pI810->DepthBuffer.Size);
	 } else {
	    xf86DrvMsg(pScrn->scrnIndex, X_INFO, 
		       "Unable to bind depthbuffer\n");
	    DRICloseScreen(pScreen);
	    return FALSE;
	 }
      } else {
	 xf86DrvMsg(pScrn->scrnIndex, X_INFO,
		    "Unable to allocate depthbuffer memory\n");
	 DRICloseScreen(pScreen);
	 return FALSE;
      }
   }	 
   
   /* Now allocate and bind the agp space.  This memory will include the
    * regular framebuffer as well as texture memory.
    */
   drmAgpAlloc(pI810->drmSubFD, sysmem_size, 0, NULL, &agpHandle);
   if (agpHandle == 0) {
      xf86DrvMsg(pScreen->myNum, X_ERROR, "drmAgpAlloc failed\n");
      DRICloseScreen(pScreen);
      return FALSE;
   }
   pI810->sysmemHandle = agpHandle;

   if (drmAgpBind(pI810->drmSubFD, agpHandle, 0) != 0) {
      xf86DrvMsg(pScreen->myNum, X_ERROR, "drmAgpBind failed\n");
      DRICloseScreen(pScreen);
      return FALSE;
   }
   
   drmAgpAlloc(pI810->drmSubFD, 4096, 2, 
	       (unsigned long *)&pI810->CursorPhysical, &agpHandle); 
   pI810->cursorHandle = agpHandle;

   if (agpHandle != 0) {
      tom = sysmem_size;

      if (drmAgpBind(pI810->drmSubFD, agpHandle, tom) == 0) { 
	 xf86DrvMsg(pScrn->scrnIndex, X_INFO, 
		    "GART: Allocated 4K for mouse cursor image\n");
	 pI810->CursorStart = tom;	 
	 tom += 4096;
      }
      else {
	 xf86DrvMsg(pScrn->scrnIndex, X_INFO, "GART: cursor bind failed\n");
	 pI810->CursorPhysical = 0;    
      } 
   }
   else {
      xf86DrvMsg(pScrn->scrnIndex, X_INFO, "GART: cursor alloc failed\n");
      pI810->CursorPhysical = 0;
   }

   /* Steal some of the excess cursor space for the overlay regs,
    * then allocate 202*2 pages for the overlay buffers.
    */
    pI810->OverlayPhysical = pI810->CursorPhysical + 1024;
    pI810->OverlayStart = pI810->CursorStart + 1024;

    /* drmAddMap happens later to preserve index order */



   I810SetTiledMemory(pScrn, 1,
		      pI810->DepthBuffer.Start,
		      i810_pitches[pitch_idx],
		      8*1024*1024);
   
   I810SetTiledMemory(pScrn, 2,
		      pI810->BackBuffer.Start,
		      i810_pitches[pitch_idx],
		      8*1024*1024);
   
   pI810->auxPitch = i810_pitches[pitch_idx];
   pI810->auxPitchBits = i810_pitch_flags[pitch_idx];
   pI810->SavedDcacheMem = pI810->DcacheMem;
   pI810DRI->backbufferSize = pI810->BackBuffer.Size;

   if (drmAddMap(pI810->drmSubFD, (drmHandle)pI810->BackBuffer.Start,
		 pI810->BackBuffer.Size, DRM_AGP, 0, 
		 &pI810DRI->backbuffer) < 0) {
      xf86DrvMsg(pScreen->myNum, X_ERROR, "drmAddMap(backbuffer) failed\n");
      DRICloseScreen(pScreen);
      return FALSE;
   }
   
   pI810DRI->depthbufferSize = pI810->DepthBuffer.Size;
   if (drmAddMap(pI810->drmSubFD, (drmHandle)pI810->DepthBuffer.Start,
		 pI810->DepthBuffer.Size, DRM_AGP, 0, 
		 &pI810DRI->depthbuffer) < 0) {
      xf86DrvMsg(pScreen->myNum, X_ERROR, "drmAddMap(depthbuffer) failed\n");
      DRICloseScreen(pScreen);
      return FALSE;
   }
   
   /* Allocate FrontBuffer etc. */
   if (!I810AllocateFront(pScrn)) {
      DRICloseScreen(pScreen);
      return FALSE;
   }

   /* Allocate buffer memory */
   I810AllocHigh( &(pI810->BufferMem), &(pI810->SysMem), 
		  I810_DMA_BUF_NR * I810_DMA_BUF_SZ);
   
   xf86DrvMsg(pScreen->myNum, X_INFO, "Buffer map : %lx\n",
              pI810->BufferMem.Start);
   
   if (pI810->BufferMem.Start == 0 || 
      pI810->BufferMem.End - pI810->BufferMem.Start > 
      I810_DMA_BUF_NR * I810_DMA_BUF_SZ) {
      xf86DrvMsg(pScreen->myNum, X_ERROR,
                 "Not enough memory for dma buffers\n");
      DRICloseScreen(pScreen);
      return FALSE;
   }
   if (drmAddMap(pI810->drmSubFD, (drmHandle)pI810->BufferMem.Start,
		pI810->BufferMem.Size, DRM_AGP, 0,
		&pI810->buffer_map) < 0) {
      xf86DrvMsg(pScreen->myNum, X_ERROR, "drmAddMap(buffer_map) failed\n");
      DRICloseScreen(pScreen);
      return FALSE;
   }

   pI810DRI->agp_buffers = pI810->buffer_map;
   pI810DRI->agp_buf_size = pI810->BufferMem.Size;

   if (drmAddMap(pI810->drmSubFD, (drmHandle)pI810->LpRing.mem.Start,
		 pI810->LpRing.mem.Size, DRM_AGP, 0,
		 &pI810->ring_map) < 0) {
      xf86DrvMsg(pScreen->myNum, X_ERROR, "drmAddMap(ring_map) failed\n");
      DRICloseScreen(pScreen);
      return FALSE;
   }
   
   /* Use the rest of memory for textures. */
   pI810DRI->textureSize = pI810->SysMem.Size;

   i = mylog2(pI810DRI->textureSize / I810_NR_TEX_REGIONS);

   if (i < I810_LOG_MIN_TEX_REGION_SIZE)
      i = I810_LOG_MIN_TEX_REGION_SIZE;

   pI810DRI->logTextureGranularity = i;
   pI810DRI->textureSize = (pI810DRI->textureSize >> i) << i; /* truncate */

   if(pI810DRI->textureSize < 512*1024) {
      ErrorF("Less then 512k for textures\n");
      DRICloseScreen(pScreen);
      return FALSE;
   }
   
   I810AllocLow( &(pI810->TexMem), &(pI810->SysMem),
		 pI810DRI->textureSize);
   
   if (drmAddMap(pI810->drmSubFD, (drmHandle)pI810->TexMem.Start,
		 pI810->TexMem.Size, DRM_AGP, 0,
		 &pI810DRI->textures) < 0) {
      xf86DrvMsg(pScreen->myNum, X_ERROR, "drmAddMap(textures) failed\n");
      DRICloseScreen(pScreen);
      return FALSE;
   }
   
   if((bufs = drmAddBufs(pI810->drmSubFD,
			 I810_DMA_BUF_NR,
			 I810_DMA_BUF_SZ,
			 DRM_AGP_BUFFER, pI810->BufferMem.Start)) <= 0) {
      xf86DrvMsg(pScrn->scrnIndex, X_INFO,
		 "[drm] failure adding %d %d byte DMA buffers\n",
		 I810_DMA_BUF_NR,
		 I810_DMA_BUF_SZ);
      DRICloseScreen(pScreen);
      return FALSE;
   }

   xf86DrvMsg(pScrn->scrnIndex, X_INFO, "[drm] added %d %d byte DMA buffers\n",
	      bufs, I810_DMA_BUF_SZ);


   I810InitDma(pScrn);
   
   /* Okay now initialize the dma engine */

   if (!pI810DRI->irq) {
      pI810DRI->irq = drmGetInterruptFromBusID(pI810->drmSubFD,
					       ((pciConfigPtr)pI810->PciInfo
						->thisCard)->busnum,
					       ((pciConfigPtr)pI810->PciInfo
						->thisCard)->devnum,
					       ((pciConfigPtr)pI810->PciInfo
						->thisCard)->funcnum);
      if((drmCtlInstHandler(pI810->drmSubFD, pI810DRI->irq)) != 0) {
	 xf86DrvMsg(pScrn->scrnIndex, X_INFO,
		    "[drm] failure adding irq handler, there is a device "
		    "already using that irq\n Consider rearranging your "
		    "PCI cards\n");
	 DRICloseScreen(pScreen);
	 return FALSE;
      }
   }

   xf86DrvMsg(pScrn->scrnIndex, X_INFO,
	      "[drm] dma control initialized, using IRQ %d\n",
	      pI810DRI->irq);
   
   pI810DRI=(I810DRIPtr)pI810->pDRIInfo->devPrivate;
   pI810DRI->deviceID=pI810->PciInfo->chipType;
   pI810DRI->width=pScrn->virtualX;
   pI810DRI->height=pScrn->virtualY;
   pI810DRI->mem=pScrn->videoRam*1024;
   pI810DRI->cpp=pI810->cpp;
   
   pI810DRI->fbOffset=pI810->FrontBuffer.Start;
   pI810DRI->fbStride=pI810->auxPitch;
   
   pI810DRI->bitsPerPixel = pScrn->bitsPerPixel;
   
   
   pI810DRI->textureOffset=pI810->TexMem.Start;
   
   pI810DRI->backOffset=pI810->BackBuffer.Start;
   pI810DRI->depthOffset=pI810->DepthBuffer.Start;
   
   pI810DRI->ringOffset=pI810->LpRing.mem.Start;
   pI810DRI->ringSize=pI810->LpRing.mem.Size;
   
   pI810DRI->auxPitch = pI810->auxPitch;
   pI810DRI->auxPitchBits = pI810->auxPitchBits;

   if (!(I810InitVisualConfigs(pScreen))) {
      xf86DrvMsg(pScreen->myNum, X_ERROR, "I810InitVisualConfigs failed\n");
      DRICloseScreen(pScreen);
      return FALSE;
   }

   xf86DrvMsg(pScrn->scrnIndex, X_INFO, "visual configs initialized\n" );
   pI810->pDRIInfo->driverSwapMethod = DRI_HIDE_X_CONTEXT;
      
   return TRUE;
}

void
I810DRICloseScreen(ScreenPtr pScreen)
{
   ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
   I810Ptr pI810 = I810PTR(pScrn);

   I810CleanupDma(pScrn);

   if(pI810->dcacheHandle) drmAgpFree(pI810->drmSubFD, pI810->dcacheHandle);
   if(pI810->backHandle) drmAgpFree(pI810->drmSubFD, pI810->backHandle);
   if(pI810->zHandle) drmAgpFree(pI810->drmSubFD, pI810->zHandle);
   if(pI810->cursorHandle) drmAgpFree(pI810->drmSubFD, pI810->cursorHandle);
   if(pI810->sysmemHandle) drmAgpFree(pI810->drmSubFD, pI810->sysmemHandle);

   if(pI810->agpAcquired == TRUE) drmAgpRelease(pI810->drmSubFD);
   
   pI810->backHandle = 0;
   pI810->zHandle = 0;
   pI810->cursorHandle = 0;
   pI810->sysmemHandle = 0;
   pI810->agpAcquired = FALSE;
   pI810->dcacheHandle = 0;

   
   DRICloseScreen(pScreen);

   if (pI810->pDRIInfo) {
      if (pI810->pDRIInfo->devPrivate) {
	 xfree(pI810->pDRIInfo->devPrivate);
	 pI810->pDRIInfo->devPrivate=0;
      }
      DRIDestroyInfoRec(pI810->pDRIInfo);
      pI810->pDRIInfo=0;
   }
   if (pI810->pVisualConfigs) xfree(pI810->pVisualConfigs);
   if (pI810->pVisualConfigsPriv) xfree(pI810->pVisualConfigsPriv);
}

static Bool
I810CreateContext(ScreenPtr pScreen, VisualPtr visual, 
		  drmContext hwContext, void *pVisualConfigPriv,
		  DRIContextType contextStore)
{
   return TRUE;
}

static void
I810DestroyContext(ScreenPtr pScreen, drmContext hwContext, 
		   DRIContextType contextStore)
{
}


Bool
I810DRIFinishScreenInit(ScreenPtr pScreen)
{
   I810SAREARec *sPriv = (I810SAREARec *)DRIGetSAREAPrivate(pScreen);
   memset( sPriv, 0, sizeof(sPriv) );
   return DRIFinishScreenInit(pScreen);
}

void
I810DRISwapContext(ScreenPtr pScreen, DRISyncType syncType, 
		   DRIContextType oldContextType, void *oldContext,
		   DRIContextType newContextType, void *newContext)
{
   ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
   I810Ptr pI810 = I810PTR(pScrn);

   if (syncType == DRI_3D_SYNC && 
       oldContextType == DRI_2D_CONTEXT &&
       newContextType == DRI_2D_CONTEXT) 
   { 
      ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];

      if (I810_DEBUG & DEBUG_VERBOSE_DRI)
	 ErrorF("I810DRISwapContext (in)\n");
   
      pI810->LockHeld = 1;
      I810RefreshRing( pScrn );
   }
   else if (syncType == DRI_2D_SYNC && 
	    oldContextType == DRI_NO_CONTEXT &&
	    newContextType == DRI_2D_CONTEXT) 
   { 
      pI810->LockHeld = 0;
      if (I810_DEBUG & DEBUG_VERBOSE_DRI)
	 ErrorF("I810DRISwapContext (out)\n");
   }
   else if (I810_DEBUG & DEBUG_VERBOSE_DRI)
      ErrorF("I810DRISwapContext (other)\n");
}

static void
I810DRIInitBuffers(WindowPtr pWin, RegionPtr prgn, CARD32 index)
{
   ScreenPtr pScreen = pWin->drawable.pScreen;
   ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
   I810Ptr pI810 = I810PTR(pScrn);
   BoxPtr pbox = REGION_RECTS(prgn);
   int nbox = REGION_NUM_RECTS(prgn);

   if (I810_DEBUG & DEBUG_VERBOSE_DRI)
      ErrorF( "I810DRIInitBuffers\n");

   I810SetupForSolidFill(pScrn, 0, GXcopy, -1);
   while (nbox--) {
      I810SelectBuffer(pScrn, I810_BACK);
      I810SubsequentSolidFillRect(pScrn, pbox->x1, pbox->y1, 
				  pbox->x2-pbox->x1, pbox->y2-pbox->y1);
      pbox++;
   }

   /* Clear the depth buffer - uses 0xffff rather than 0.
    */
   pbox = REGION_RECTS(prgn);
   nbox = REGION_NUM_RECTS(prgn);
   I810SelectBuffer(pScrn, I810_DEPTH);
   I810SetupForSolidFill(pScrn, 0xffff, GXcopy, -1);
   while (nbox--) {  
      I810SubsequentSolidFillRect(pScrn, pbox->x1, pbox->y1, 
				  pbox->x2-pbox->x1, pbox->y2-pbox->y1);
      pbox++;
   }
   I810SelectBuffer(pScrn, I810_FRONT);
   pI810->AccelInfoRec->NeedToSync = TRUE;
}

/* This routine is a modified form of XAADoBitBlt with the calls to
 * ScreenToScreenBitBlt built in. My routine has the prgnSrc as source
 * instead of destination. My origin is upside down so the ydir cases
 * are reversed. 
 *
 * KW: can you believe that this is called even when a 2d window moves?
 */
static void
I810DRIMoveBuffers(WindowPtr pParent, DDXPointRec ptOldOrg, 
		   RegionPtr prgnSrc, CARD32 index)
{
   ScreenPtr pScreen = pParent->drawable.pScreen;
   ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
   I810Ptr pI810 = I810PTR(pScrn);
   BoxPtr pboxTmp, pboxNext, pboxBase;
   DDXPointPtr pptTmp, pptNew2;
   int xdir, ydir;

   int screenwidth = pScrn->virtualX;
   int screenheight = pScrn->virtualY;

   BoxPtr pbox = REGION_RECTS(prgnSrc);
   int nbox = REGION_NUM_RECTS(prgnSrc);

   BoxPtr pboxNew1 = 0;
   BoxPtr pboxNew2 = 0;
   DDXPointPtr pptNew1 = 0;
   DDXPointPtr pptSrc = &ptOldOrg;

   int dx = pParent->drawable.x - ptOldOrg.x;
   int dy = pParent->drawable.y - ptOldOrg.y;

   /* If the copy will overlap in Y, reverse the order */
   if (dy>0) {
      ydir = -1;

      if (nbox>1) {
	 /* Keep ordering in each band, reverse order of bands */
	 pboxNew1 = (BoxPtr)ALLOCATE_LOCAL(sizeof(BoxRec)*nbox);
	 if (!pboxNew1) return;
	 pptNew1 = (DDXPointPtr)ALLOCATE_LOCAL(sizeof(DDXPointRec)*nbox);
	 if (!pptNew1) {
	    DEALLOCATE_LOCAL(pboxNew1);
	    return;
	 }
	 pboxBase = pboxNext = pbox+nbox-1;
	 while (pboxBase >= pbox) {
	    while ((pboxNext >= pbox) && (pboxBase->y1 == pboxNext->y1))
	       pboxNext--;
	    pboxTmp = pboxNext+1;
	    pptTmp = pptSrc + (pboxTmp - pbox);
	    while (pboxTmp <= pboxBase) {
	       *pboxNew1++ = *pboxTmp++;
	       *pptNew1++ = *pptTmp++;
	    }
	    pboxBase = pboxNext;
	 }
	 pboxNew1 -= nbox;
	 pbox = pboxNew1;
	 pptNew1 -= nbox;
	 pptSrc = pptNew1;
      }
   } else {
      /* No changes required */
      ydir = 1;
   }

   /* If the regions will overlap in X, reverse the order */
   if (dx>0) {
      xdir = -1;

      if (nbox > 1) {
	 /*reverse orderof rects in each band */
	 pboxNew2 = (BoxPtr)ALLOCATE_LOCAL(sizeof(BoxRec)*nbox);
	 pptNew2 = (DDXPointPtr)ALLOCATE_LOCAL(sizeof(DDXPointRec)*nbox);
	 if (!pboxNew2 || !pptNew2) {
	    if (pptNew2) DEALLOCATE_LOCAL(pptNew2);
	    if (pboxNew2) DEALLOCATE_LOCAL(pboxNew2);
	    if (pboxNew1) {
	       DEALLOCATE_LOCAL(pptNew1);
	       DEALLOCATE_LOCAL(pboxNew1);
	    }
	    return;
	 }
	 pboxBase = pboxNext = pbox;
	 while (pboxBase < pbox+nbox) {
	    while ((pboxNext < pbox+nbox) && (pboxNext->y1 == pboxBase->y1))
	       pboxNext++;
	    pboxTmp = pboxNext;
	    pptTmp = pptSrc + (pboxTmp - pbox);
	    while (pboxTmp != pboxBase) {
	       *pboxNew2++ = *--pboxTmp;
	       *pptNew2++ = *--pptTmp;
	    }
	    pboxBase = pboxNext;
	 }
	 pboxNew2 -= nbox;
	 pbox = pboxNew2;
	 pptNew2 -= nbox;
	 pptSrc = pptNew2;
      }
   } else {
      /* No changes are needed */
      xdir = 1;
   }

   /* SelectBuffer isn't really a good concept for the i810.
    */
   I810EmitFlush(pScrn);
   I810SetupForScreenToScreenCopy(pScrn, xdir, ydir, GXcopy, -1, -1);
   for ( ; nbox-- ; pbox++ ) {
      
      int x1 = pbox->x1;
      int y1 = pbox->y1;
      int destx = x1 + dx;
      int desty = y1 + dy;
      int w = pbox->x2 - x1 + 1;
      int h = pbox->y2 - y1 + 1;
      
      if ( destx < 0 ) x1 -= destx, w += destx, destx = 0; 
      if ( desty < 0 ) y1 -= desty, h += desty, desty = 0;
      if ( destx + w > screenwidth ) w = screenwidth - destx;
      if ( desty + h > screenheight ) h = screenheight - desty;
      if ( w <= 0 ) continue;
      if ( h <= 0 ) continue;
      

      if (I810_DEBUG & DEBUG_VERBOSE_DRI)
	 ErrorF( "MoveBuffers %d,%d %dx%d dx: %d dy: %d\n",
		 x1, y1, w, h, dx, dy);

      I810SelectBuffer(pScrn, I810_BACK);
      I810SubsequentScreenToScreenCopy(pScrn, x1, y1, destx, desty, w, h);
      I810SelectBuffer(pScrn, I810_DEPTH);
      I810SubsequentScreenToScreenCopy(pScrn, x1, y1, destx, desty, w, h);
   }
   I810SelectBuffer(pScrn, I810_FRONT);
   I810EmitFlush(pScrn);

   if (pboxNew2) {
      DEALLOCATE_LOCAL(pptNew2);
      DEALLOCATE_LOCAL(pboxNew2);
   }
   if (pboxNew1) {
      DEALLOCATE_LOCAL(pptNew1);
      DEALLOCATE_LOCAL(pboxNew1);
   }

   pI810->AccelInfoRec->NeedToSync = TRUE;
}
