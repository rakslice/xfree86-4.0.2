/* $XFree86: xc/programs/Xserver/hw/xfree86/common/xf86Cursor.c,v 3.28 2000/06/24 00:33:54 dawes Exp $ */
/* $XConsortium: xf86Cursor.c /main/10 1996/10/19 17:58:23 kaleb $ */

#define NEED_EVENTS
#include "X.h"
#include "Xmd.h"
#include "input.h"
#include "cursor.h"
#include "mipointer.h"
#include "scrnintstr.h"
#include "globals.h"

#include "compiler.h"

#include "xf86.h"
#include "xf86Priv.h"
#include "xf86_OSproc.h"

#ifdef XINPUT
#include "XIproto.h"
#include "xf86Xinput.h"
#endif

#ifdef XFreeXDGA
#include "dgaproc.h"
#endif

typedef struct _xf86EdgeRec {
   short screen;
   short start;
   short end;
   DDXPointRec offset;
   struct _xf86EdgeRec *next;
} xf86EdgeRec, *xf86EdgePtr;

typedef struct {
    xf86EdgePtr	left, right, up, down;
} xf86ScreenLayoutRec, *xf86ScreenLayoutPtr;

static Bool xf86CursorOffScreen(ScreenPtr *pScreen, int *x, int *y);
static void xf86CrossScreen(ScreenPtr pScreen, Bool entering);
static void xf86WarpCursor(ScreenPtr pScreen, int x, int y);

static void xf86PointerMoved(int scrnIndex, int x, int y);

static miPointerScreenFuncRec xf86PointerScreenFuncs = {
  xf86CursorOffScreen,
  xf86CrossScreen,
  xf86WarpCursor,
#ifdef XINPUT
  xf86eqEnqueue,
  xf86eqSwitchScreen
#else
  /* let miPointerInitialize take care of these */
  NULL,
  NULL
#endif
};

static xf86ScreenLayoutRec xf86ScreenLayout[MAXSCREENS];

static Bool HardEdges;

/*
 * xf86InitViewport --
 *      Initialize paning & zooming parameters, so that a driver must only
 *      check what resolutions are possible and whether the virtual area
 *      is valid if specified.
 */

void
xf86InitViewport(ScrnInfoPtr pScr)
{

  pScr->PointerMoved = xf86PointerMoved;

  /*
   * Compute the initial Viewport if necessary
   */
  if (pScr->frameX0 < 0)
    {
      pScr->frameX0 = (pScr->virtualX - pScr->modes->HDisplay) / 2;
      pScr->frameY0 = (pScr->virtualY - pScr->modes->VDisplay) / 2;
    }
  pScr->frameX1 = pScr->frameX0 + pScr->modes->HDisplay - 1;
  pScr->frameY1 = pScr->frameY0 + pScr->modes->VDisplay - 1;

  /*
   * Now adjust the initial Viewport, so it lies within the virtual area
   */
  if (pScr->frameX1 >= pScr->virtualX)
    {
	pScr->frameX0 = pScr->virtualX - pScr->modes->HDisplay;
	pScr->frameX1 = pScr->frameX0 + pScr->modes->HDisplay - 1;
    }

  if (pScr->frameY1 >= pScr->virtualY)
    {
	pScr->frameY0 = pScr->virtualY - pScr->modes->VDisplay;
	pScr->frameY1 = pScr->frameY0 + pScr->modes->VDisplay - 1;
    }
}


/*
 * xf86SetViewport --
 *      Scroll the visual part of the screen so the pointer is visible.
 */

void
xf86SetViewport(ScreenPtr pScreen, int x, int y)
{
  ScrnInfoPtr   pScr = XF86SCRNINFO(pScreen);

  (*pScr->PointerMoved)(pScreen->myNum, x, y);
}


static void 
xf86PointerMoved(int scrnIndex, int x, int y)
{
  Bool          frameChanged = FALSE;
  ScrnInfoPtr   pScr = xf86Screens[scrnIndex];

  /*
   * check wether (x,y) belongs to the visual part of the screen
   * if not, change the base of the displayed frame accoring
   */
  if ( pScr->frameX0 > x) { 
    pScr->frameX0 = x;
    pScr->frameX1 = x + pScr->currentMode->HDisplay - 1;
    frameChanged = TRUE ;
  }
  
  if ( pScr->frameX1 < x) { 
    pScr->frameX1 = x + 1;
    pScr->frameX0 = x - pScr->currentMode->HDisplay + 1;
    frameChanged = TRUE ;
  }
  
  if ( pScr->frameY0 > y) { 
    pScr->frameY0 = y;
    pScr->frameY1 = y + pScr->currentMode->VDisplay - 1;
    frameChanged = TRUE;
  }
  
  if ( pScr->frameY1 < y) { 
    pScr->frameY1 = y;
    pScr->frameY0 = y - pScr->currentMode->VDisplay + 1;
    frameChanged = TRUE; 
  }
  
  if (frameChanged && pScr->AdjustFrame != NULL)
    pScr->AdjustFrame(pScr->scrnIndex, pScr->frameX0, pScr->frameY0, 0);
}

/*
 * xf86LockZoom --
 *	Enable/disable ZoomViewport
 */

void
xf86LockZoom(ScreenPtr pScreen, Bool lock)
{
  XF86SCRNINFO(pScreen)->zoomLocked = lock;
}

Bool
xf86ZoomLocked(ScreenPtr pScreen)
{
  if (xf86Info.dontZoom || XF86SCRNINFO(pScreen)->zoomLocked)
    return TRUE;
  else
    return FALSE;
}
    
/*
 * xf86ZoomViewport --
 *      Reinitialize the visual part of the screen for another mode.
 */

void
xf86ZoomViewport (ScreenPtr pScreen, int zoom)
{
  ScrnInfoPtr   pScr = XF86SCRNINFO(pScreen);
  Bool tmp;
  int px, py;
  
  if (pScr->zoomLocked)
    return;

  if (pScr->SwitchMode != NULL &&
      pScr->currentMode != pScr->currentMode->next) {
    pScr->currentMode = zoom > 0 ? pScr->currentMode->next
				 : pScr->currentMode->prev;

    xf86EnterServerState(SETUP);
    tmp = pScr->SwitchMode(pScr->scrnIndex, pScr->currentMode, 0);
    xf86EnterServerState(OPERATING);
    if (tmp) {
      /* 
       * adjust new frame for the displaysize
       */
      pScr->frameX0 = (pScr->frameX1 + pScr->frameX0 -
		       pScr->currentMode->HDisplay) / 2;
      pScr->frameX1 = pScr->frameX0 + pScr->currentMode->HDisplay - 1;

      if (pScr->frameX0 < 0) {
	  pScr->frameX0 = 0;
	  pScr->frameX1 = pScr->frameX0 + pScr->currentMode->HDisplay - 1;
      } else if (pScr->frameX1 >= pScr->virtualX) {
	  pScr->frameX0 = pScr->virtualX - pScr->currentMode->HDisplay;
	  pScr->frameX1 = pScr->frameX0 + pScr->currentMode->HDisplay - 1;
      }
      
      pScr->frameY0 = (pScr->frameY1 + pScr->frameY0 -
		       pScr->currentMode->VDisplay) / 2;
      pScr->frameY1 = pScr->frameY0 + pScr->currentMode->VDisplay - 1;

      if (pScr->frameY0 < 0) {
	  pScr->frameY0 = 0;
	  pScr->frameY1 = pScr->frameY0 + pScr->currentMode->VDisplay - 1;
      } else if (pScr->frameY1 >= pScr->virtualY) {
	  pScr->frameY0 = pScr->virtualY - pScr->currentMode->VDisplay;
	  pScr->frameY1 = pScr->frameY0 + pScr->currentMode->VDisplay - 1;
      }
    }
    else /* switch failed, so go back to old mode */
      pScr->currentMode = zoom > 0 ? pScr->currentMode->prev
				   : pScr->currentMode->next;
  }

  if (pScr->AdjustFrame != NULL)
    (pScr->AdjustFrame)(pScr->scrnIndex, pScr->frameX0, pScr->frameY0, 0);

  if (pScreen == miPointerCurrentScreen()) {
    miPointerPosition(&px, &py);
    xf86WarpCursor(pScreen, px, py);
  }
}



static xf86EdgePtr
FindEdge(xf86EdgePtr edge, int val)
{
    while(edge && (edge->end <= val))
	edge = edge->next;

    if(edge && (edge->start <= val))
	return edge;

    return NULL;
}

/*
 * xf86CursorOffScreen --
 *      Check whether it is necessary to switch to another screen
 */

static Bool
xf86CursorOffScreen(ScreenPtr *pScreen, int *x, int *y)
{
    xf86EdgePtr edge;
    int tmp;

    if(screenInfo.numScreens == 1)
	return FALSE;

    if(*x < 0) {
        tmp = *y;
	if(tmp < 0) tmp = 0;
	if(tmp >= (*pScreen)->height) tmp = (*pScreen)->height - 1;

	if((edge = xf86ScreenLayout[(*pScreen)->myNum].left))
	   edge = FindEdge(edge, tmp);

	if(!edge) *x = 0;
	else {
	    *x += edge->offset.x;
	    *y += edge->offset.y;
	    *pScreen = xf86Screens[edge->screen]->pScreen;
	}
    }

    if(*x >= (*pScreen)->width) {
        tmp = *y;
	if(tmp < 0) tmp = 0;
	if(tmp >= (*pScreen)->height) tmp = (*pScreen)->height - 1;

	if((edge = xf86ScreenLayout[(*pScreen)->myNum].right))
	   edge = FindEdge(edge, tmp);

	if(!edge) *x = (*pScreen)->width - 1;
	else {
	    *x += edge->offset.x;
	    *y += edge->offset.y;
	    *pScreen = xf86Screens[edge->screen]->pScreen;
	}
    }

    if(*y < 0) {
        tmp = *x;
	if(tmp < 0) tmp = 0;
	if(tmp >= (*pScreen)->width) tmp = (*pScreen)->width - 1;

	if((edge = xf86ScreenLayout[(*pScreen)->myNum].up))
	   edge = FindEdge(edge, tmp);

	if(!edge) *y = 0;
	else {
	    *x += edge->offset.x;
	    *y += edge->offset.y;
	    *pScreen = xf86Screens[edge->screen]->pScreen;
	}
    }

    if(*y >= (*pScreen)->height) {
        tmp = *x;
	if(tmp < 0) tmp = 0;
	if(tmp >= (*pScreen)->width) tmp = (*pScreen)->width - 1;

	if((edge = xf86ScreenLayout[(*pScreen)->myNum].down))
	   edge = FindEdge(edge, tmp);

	if(!edge) *y = (*pScreen)->height - 1;
	else {
	    *x += edge->offset.x;
	    *y += edge->offset.y;
	    (*pScreen) = xf86Screens[edge->screen]->pScreen;
	}
    }


#if 0
    /* This presents problems for overlapping screens when
 	HardEdges is used.  Have to think about the logic more */
    if((*x < 0) || (*x >= (*pScreen)->width) || 
       (*y < 0) || (*y >= (*pScreen)->height)) {
	/* We may have crossed more than one screen */
	xf86CursorOffScreen(pScreen, x, y);
    }
#endif

    return TRUE;
}



/*
 * xf86CrossScreen --
 *      Switch to another screen
 */

/* NEED TO CHECK THIS */
/* ARGSUSED */
static void
xf86CrossScreen (ScreenPtr pScreen, Bool entering)
{
#if 0
  if (xf86Info.sharedMonitor)
    (XF86SCRNINFO(pScreen)->EnterLeaveMonitor)(entering);
  (XF86SCRNINFO(pScreen)->EnterLeaveCursor)(entering);
#endif
}


/*
 * xf86WarpCursor --
 *      Warp possible to another screen
 */

/* ARGSUSED */
static void
xf86WarpCursor (ScreenPtr pScreen, int x, int y)
{
    int    sigstate;
    sigstate = xf86BlockSIGIO ();
  miPointerWarpCursor(pScreen,x,y);

  xf86Info.currentScreen = pScreen;
    xf86UnblockSIGIO (sigstate);
}


void *
xf86GetPointerScreenFuncs(void)
{
    return (void *)&xf86PointerScreenFuncs;
}


static xf86EdgePtr
AddEdge(
   xf86EdgePtr edge, 
   short min, 
   short max,
   short dx,
   short dy,
   short screen
){
   xf86EdgePtr pEdge = edge, pPrev = NULL, pNew;

   while(1) {
	while(pEdge && (min >= pEdge->end)) {
	    pPrev = pEdge;
	    pEdge = pEdge->next;
	}  

	if(!pEdge) {
	    if(!(pNew = xalloc(sizeof(xf86EdgeRec))))
		break;

	    pNew->screen = screen;
	    pNew->start = min;  
	    pNew->end = max;   
	    pNew->offset.x = dx;
	    pNew->offset.y = dy;
	    pNew->next = NULL;

	    if(pPrev)
		pPrev->next = pNew;
	    else
		edge = pNew;
	    
	    break;
	} else if (min < pEdge->start) {
	    if(!(pNew = xalloc(sizeof(xf86EdgeRec))))
		break;

	    pNew->screen = screen;
	    pNew->start = min;  
	    pNew->offset.x = dx;
	    pNew->offset.y = dy;
	    pNew->next = pEdge;

	    if(pPrev) pPrev->next = pNew;
	    else edge = pNew;

	    if(max <= pEdge->start) {
		pNew->end = max;   
		break;
	    } else {
		pNew->end = pEdge->start;
		min = pEdge->end;
	    }
	} else
	    min = pEdge->end;

	pPrev = pEdge;
	pEdge = pEdge->next;

	if(max <= min) break;
   }
	
   return edge;
}

static void
FillOutEdge(xf86EdgePtr pEdge, int limit)
{
    xf86EdgePtr pNext;
    int diff;

    if(pEdge->start > 0) pEdge->start = 0;

    while((pNext = pEdge->next)) {
	diff = pNext->start - pEdge->end;
	if(diff > 0) {	
	    pEdge->end += diff >> 1;
	    pNext->start -= diff - (diff >> 1);
	}
	pEdge = pNext;
    }

    if(pEdge->end < limit)
	pEdge->end = limit;    
}

/*
 * xf86InitOrigins() can deal with a maximum of 32 screens
 * on 32 bit architectures, 64 on 64 bit architectures.
 */

void
xf86InitOrigins(void)
{
    unsigned long screensLeft, prevScreensLeft, mask;
    screenLayoutPtr screen;
    ScreenPtr pScreen;
    int x1, x2, y1, y2, left, right, top, bottom;
    int i, j, ref, minX, minY, min, max;
    xf86ScreenLayoutPtr pLayout;
    Bool OldStyleConfig = FALSE;

    /* need to have this set up with a config file option */
    HardEdges = FALSE;

    bzero(xf86ScreenLayout, MAXSCREENS * sizeof(xf86ScreenLayoutRec));
	
    screensLeft = prevScreensLeft = (1 << xf86NumScreens) - 1;

    while(1) {
	for(mask = screensLeft, i = 0; mask; mask >>= 1, i++) {
	    if(!(mask & 1L)) continue;

	    screen = &xf86ConfigLayout.screens[i];

	    switch(screen->where) {
	    case PosObsolete:
		OldStyleConfig = TRUE;
		pLayout = &xf86ScreenLayout[i];
		/* force edge lists */
		if(screen->left) {
		    ref = screen->left->screennum;
		    pLayout->left = AddEdge(pLayout->left, 
			0, xf86Screens[i]->pScreen->height,
			xf86Screens[ref]->pScreen->width, 0, ref);
		}
		if(screen->right) {
		    ref = screen->right->screennum;
		    pScreen = xf86Screens[i]->pScreen;
		    pLayout->right = AddEdge(pLayout->right, 
			0, pScreen->height, -pScreen->width, 0, ref);
		}
		if(screen->top) {
		    ref = screen->top->screennum;
		    pLayout->up = AddEdge(pLayout->up, 
			0, xf86Screens[i]->pScreen->width,
			0, xf86Screens[ref]->pScreen->height, ref);
		}
		if(screen->bottom) {
		    ref = screen->bottom->screennum;
		    pScreen = xf86Screens[i]->pScreen;
		    pLayout->down = AddEdge(pLayout->down, 
			0, pScreen->width, 0, -pScreen->height, ref);
		}
	        /* we could also try to place it based on those
		   relative locations if we wanted to */
		screen->x = screen->y = 0;
		/* FALLTHROUGH */
	    case PosAbsolute:
		dixScreenOrigins[i].x = screen->x;
		dixScreenOrigins[i].y = screen->y;
		screensLeft &= ~(1 << i);
		break;
	    case PosRelative:
		ref = screen->refscreen->screennum;
		if(screensLeft & (1 << ref)) break;
		dixScreenOrigins[i].x = dixScreenOrigins[ref].x + screen->x;
		dixScreenOrigins[i].y = dixScreenOrigins[ref].y + screen->y;
		screensLeft &= ~(1 << i);
		break;
	    case PosRightOf:
		ref = screen->refscreen->screennum;
		if(screensLeft & (1 << ref)) break;
		pScreen = xf86Screens[ref]->pScreen;
		dixScreenOrigins[i].x = 
			dixScreenOrigins[ref].x + pScreen->width;
		dixScreenOrigins[i].y = dixScreenOrigins[ref].y;
		screensLeft &= ~(1 << i);
		break;
	    case PosLeftOf:
		ref = screen->refscreen->screennum;
		if(screensLeft & (1 << ref)) break;
		pScreen = xf86Screens[i]->pScreen;
		dixScreenOrigins[i].x = 
			dixScreenOrigins[ref].x - pScreen->width;
		dixScreenOrigins[i].y = dixScreenOrigins[ref].y;
		screensLeft &= ~(1 << i);
		break;
	    case PosBelow:
		ref = screen->refscreen->screennum;
		if(screensLeft & (1 << ref)) break;
		pScreen = xf86Screens[ref]->pScreen;
		dixScreenOrigins[i].x = dixScreenOrigins[ref].x;
		dixScreenOrigins[i].y = 
			dixScreenOrigins[ref].y + pScreen->height;
		screensLeft &= ~(1 << i);
		break;
	    case PosAbove:
		ref = screen->refscreen->screennum;
		if(screensLeft & (1 << ref)) break;
		pScreen = xf86Screens[i]->pScreen;
		dixScreenOrigins[i].x = dixScreenOrigins[ref].x;
		dixScreenOrigins[i].y = 
			dixScreenOrigins[ref].y - pScreen->height;
		screensLeft &= ~(1 << i);
		break;
	    default:
		ErrorF("Illegal placement keyword in Layout!\n");
		break;
	    }

	}

	if(!screensLeft) break;

	if(screensLeft == prevScreensLeft) {
	/* All the remaining screens are referencing each other.
	   Assign a value to one of them and go through again */
	    i = 0;
	    while(!((1 << i) & screensLeft)){ i++; }

	    ref = xf86ConfigLayout.screens[i].refscreen->screennum;
	    dixScreenOrigins[ref].x = dixScreenOrigins[ref].y = 0;
	    screensLeft &= ~(1 << ref);
	}

	prevScreensLeft = screensLeft;
    }

    /* justify the topmost and leftmost to (0,0) */
    minX = dixScreenOrigins[0].x;
    minY = dixScreenOrigins[0].y;

    for(i = 1; i < xf86NumScreens; i++) {
	if(dixScreenOrigins[i].x < minX)
	  minX = dixScreenOrigins[i].x;
	if(dixScreenOrigins[i].y < minY)
	  minY = dixScreenOrigins[i].y;
    }

    if (minX | minY) {
	for(i = 0; i < xf86NumScreens; i++) {
	   dixScreenOrigins[i].x -= minX;
	   dixScreenOrigins[i].y -= minY;
	}
    }


    /* Create the edge lists */

    if(!OldStyleConfig) {
      for(i = 0; i < xf86NumScreens; i++) {
	pLayout = &xf86ScreenLayout[i];

	pScreen = xf86Screens[i]->pScreen;

	left = dixScreenOrigins[i].x;
	right = left + pScreen->width;
	top = dixScreenOrigins[i].y;
	bottom = top + pScreen->height;

	for(j = 0; j < xf86NumScreens; j++) {
	    if(i == j) continue;

	    x1 = dixScreenOrigins[j].x;
	    x2 = x1 + xf86Screens[j]->pScreen->width;
	    y1 = dixScreenOrigins[j].y;
	    y2 = y1 + xf86Screens[j]->pScreen->height;

	    if((bottom > y1) && (top < y2)) {
		min = y1 - top;
		if(min < 0) min = 0;
		max = pScreen->height - (bottom - y2);
		if(max > pScreen->height) max = pScreen->height;

		if(((left - 1) >= x1) && ((left - 1) < x2))
		    pLayout->left = AddEdge(pLayout->left, min, max,
			dixScreenOrigins[i].x - dixScreenOrigins[j].x,
			dixScreenOrigins[i].y - dixScreenOrigins[j].y, j);

		if((right >= x1) && (right < x2))	
		    pLayout->right = AddEdge(pLayout->right, min, max,
			dixScreenOrigins[i].x - dixScreenOrigins[j].x,
			dixScreenOrigins[i].y - dixScreenOrigins[j].y, j);
	    }


	    if((left < x2) && (right > x1)) {
		min = x1 - left;
		if(min < 0) min = 0;
		max = pScreen->width - (right - x2);
		if(max > pScreen->width) max = pScreen->width;

		if(((top - 1) >= y1) && ((top - 1) < y2))
		    pLayout->up = AddEdge(pLayout->up, min, max,
			dixScreenOrigins[i].x - dixScreenOrigins[j].x,
			dixScreenOrigins[i].y - dixScreenOrigins[j].y, j);

		if((bottom >= y1) && (bottom < y2))
		    pLayout->down = AddEdge(pLayout->down, min, max,
			dixScreenOrigins[i].x - dixScreenOrigins[j].x,
			dixScreenOrigins[i].y - dixScreenOrigins[j].y, j);
	    }
	}
      }
    }

    if(!HardEdges && !OldStyleConfig) {
	for(i = 0; i < xf86NumScreens; i++) {
	    pLayout = &xf86ScreenLayout[i];
	    pScreen = xf86Screens[i]->pScreen;
	    if(pLayout->left) 
		FillOutEdge(pLayout->left, pScreen->height);
	    if(pLayout->right) 
		FillOutEdge(pLayout->right, pScreen->height);
	    if(pLayout->up) 
		FillOutEdge(pLayout->up, pScreen->width);
	    if(pLayout->down) 
		FillOutEdge(pLayout->down, pScreen->width);
	}
    }
}
