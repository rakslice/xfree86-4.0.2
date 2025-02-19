/* $TOG: xwininfo.c /main/56 1998/02/09 14:21:06 kaleb $ */
/*

Copyright 1987, 1998  The Open Group

All Rights Reserved.

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall
not be used in advertising or otherwise to promote the sale, use or
other dealings in this Software without prior written authorization
from The Open Group.

*/
/* $XFree86: xc/programs/xwininfo/xwininfo.c,v 1.5 1999/03/02 11:50:04 dawes Exp $ */


/*
 * xwininfo.c	- MIT Project Athena, X Window system window
 *		  information utility.
 *
 *
 *	This program will report all relevant information
 *	about a specific window.
 *
 *  Author:	Mark Lillibridge, MIT Project Athena
 *		16-Jun-87
 */

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/Xos.h>
#include <X11/extensions/shape.h>
#include <X11/Xmu/WinUtil.h>
#include <stdio.h>

/* Include routines to handle parsing defaults */
#include "dsimple.h"

typedef struct {
	long code;
	char *name;
} binding;

#if NeedFunctionPrototypes
extern void scale_init(void);
extern char *nscale(int, int, int, char *);
extern char *xscale(int);
extern char *yscale(int);
extern char *bscale(int);
extern int bad_window_handler(Display *, XErrorEvent *);
extern int main(int, char **);
extern char *LookupL(long, binding *);
extern char *Lookup(int, binding *);
extern void Display_Window_Id(Window, int);
extern void Display_Stats_Info(Window);
extern void Display_Bits_Info(Window);
extern void Display_Event_Mask(long);
extern void Display_Events_Info(Window);
extern void Display_Tree_Info(Window, int);
extern void display_tree_info_1(Window, int, int);
extern void Display_Hints(XSizeHints *);
extern void Display_Size_Hints(Window);
extern void Display_Window_Shape(Window);
extern void Display_WM_Info(Window);
#endif

static char *window_id_format = "0x%lx";

/*
 * Report the syntax for calling xwininfo:
 */
void
usage()
{
    fprintf (stderr,
	"usage:  %s [-options ...]\n\n", program_name);
    fprintf (stderr,
	"where options include:\n");
    fprintf (stderr,
	"    -help                print this message\n");
    fprintf (stderr,
	"    -display host:dpy    X server to contact\n");
    fprintf (stderr,
	"    -root                use the root window\n");
    fprintf (stderr,
	"    -id windowid         use the window with the specified id\n");
    fprintf (stderr,
	"    -name windowname     use the window with the specified name\n");
    fprintf (stderr,
	"    -int                 print window id in decimal\n");
    fprintf (stderr,
	"    -children            print parent and child identifiers\n");
    fprintf (stderr,
	"    -tree                print children identifiers recursively\n");
    fprintf (stderr,
	"    -stats               print window geometry [DEFAULT]\n");
    fprintf (stderr,
	"    -bits                print window pixel information\n");
    fprintf (stderr,
	"    -events              print events selected for on window\n");
    fprintf (stderr,
	"    -size                print size hints\n");
    fprintf (stderr,
	"    -wm                  print window manager hints\n");
    fprintf (stderr,
	"    -shape               print shape extents\n");
    fprintf (stderr,
	"    -frame               don't ignore window manager frames\n");
    fprintf (stderr,
	"    -english             print sizes in english units\n");
    fprintf (stderr,
	"    -metric              print sizes in metric units\n");
    fprintf (stderr,
	"    -all                 -tree, -stats, -bits, -events, -wm, -size, -shape\n");
    fprintf (stderr,
	"\n");
    exit (1);
}

/*
 * pixel to inch, metric converter.
 * Hacked in by Mark W. Eichin <eichin@athena> [eichin:19880619.1509EST]
 *
 * Simply put: replace the old numbers with string print calls.
 * Returning a local string is ok, since we only ever get called to
 * print one x and one y, so as long as they don't collide, they're
 * fine. This is not meant to be a general purpose routine.
 *
 */

#define getdsp(var,fn) var = fn(dpy, DefaultScreen(dpy))
int xp=0, xmm=0;
int yp=0, ymm=0;
int bp=0, bmm=0;
int english = 0, metric = 0;

void scale_init()
{
  getdsp(yp,  DisplayHeight);
  getdsp(ymm, DisplayHeightMM);
  getdsp(xp,  DisplayWidth);
  getdsp(xmm, DisplayWidthMM);
  bp  = xp  + yp;
  bmm = xmm + ymm;
}

#define MILE (5280*12)
#define YARD (3*12)
#define FOOT (12)

char *nscale(n, np, nmm, nbuf)
     int n, np, nmm;
     char *nbuf;
{
  sprintf(nbuf, "%d", n);
  if(metric||english) {
    sprintf(nbuf+strlen(nbuf), " (");
  }
  if(metric) {
    sprintf(nbuf+strlen(nbuf),"%.2f mm%s", ((double) n)*nmm/np, english?"; ":"");
  }
  if(english) {
    double inch_frac;
    Bool printed_anything = False;
    int mi, yar, ft, inr;

    inch_frac = ((double) n)*(nmm/25.4)/np;
    inr = (int)inch_frac;
    inch_frac -= (double)inr;
    if(inr>=MILE) {
      mi = inr/MILE;
      inr %= MILE;
      sprintf(nbuf+strlen(nbuf), "%d %s(?!?)",
	      mi, (mi==1)?"mile":"miles");
      printed_anything = True;
    }
    if(inr>=YARD) {
      yar = inr/YARD;
      inr %= YARD;
      if (printed_anything)
	  sprintf(nbuf+strlen(nbuf), ", ");
      sprintf(nbuf+strlen(nbuf), "%d %s",
	      yar, (yar==1)?"yard":"yards");
      printed_anything = True;
    }
    if(inr>=FOOT) {
      ft = inr/FOOT;
      inr  %= FOOT;
      if (printed_anything)
	  sprintf(nbuf+strlen(nbuf), ", ");
      sprintf(nbuf+strlen(nbuf), "%d %s",
	      ft, (ft==1)?"foot":"feet");
      printed_anything = True;
    }
    if (!printed_anything || inch_frac != 0.0 || inr != 0) {
      if (printed_anything)
	  sprintf(nbuf+strlen(nbuf), ", ");
      sprintf(nbuf+strlen(nbuf), "%.2f inches", inr+inch_frac);
    }
  }
  if (english || metric) strcat (nbuf, ")");
  return(nbuf);
}	  
  
char xbuf[BUFSIZ];
char *xscale(x)
     int x;
{
  if(!xp) {
    scale_init();
  }
  return(nscale(x, xp, xmm, xbuf));
}

char ybuf[BUFSIZ];
char *yscale(y)
     int y;
{
  if(!yp) {
    scale_init();
  }
  return(nscale(y, yp, ymm, ybuf));
}

char bbuf[BUFSIZ];
char *bscale(b)
     int b;
{
  if(!bp) {
    scale_init();
  }
  return(nscale(b, bp, bmm, bbuf));
}

/* end of pixel to inch, metric converter */

/* This handler is enabled when we are checking
   to see if the -id the user specified is valid. */

/* ARGSUSED */
int
bad_window_handler(disp, err)
    Display *disp;
    XErrorEvent *err;
{
    char badid[20];

    sprintf(badid, window_id_format, err->resourceid);
    Fatal_Error("No such window with id %s.", badid);
    exit (1);
    return 0;
}


int
main(argc, argv)
     int argc;
     char **argv;
{
  register int i;
  int tree = 0, stats = 0, bits = 0, events = 0, wm = 0, size  = 0, shape = 0;
  int frame = 0, children = 0;
  Window window;

  INIT_NAME;

  /* Open display, handle command line arguments */
  Setup_Display_And_Screen(&argc, argv);

  /* Get window selected on command line, if any */
  window = Select_Window_Args(&argc, argv);

  /* Handle our command line arguments */
  for (i = 1; i < argc; i++) {
    if (!strcmp(argv[i], "-help"))
      usage();
    if (!strcmp(argv[i], "-int")) {
      window_id_format = "%ld";
      continue;
    }
    if (!strcmp(argv[i], "-children")) {
      children = 1;
      continue;
    }
    if (!strcmp(argv[i], "-tree")) {
      tree = 1;
      continue;
    }
    if (!strcmp(argv[i], "-stats")) {
      stats = 1;
      continue;
    }
    if (!strcmp(argv[i], "-bits")) {
      bits = 1;
      continue;
    }
    if (!strcmp(argv[i], "-events")) {
      events = 1;
      continue;
    }
    if (!strcmp(argv[i], "-wm")) {
      wm = 1;
      continue;
    }
    if (!strcmp(argv[i], "-frame")) {
      frame = 1;
      continue;
    }
    if (!strcmp(argv[i], "-size")) {
      size = 1;
      continue;
    }
    if (!strcmp(argv[i], "-shape")) {
      shape = 1;
      continue;
    }
    if (!strcmp(argv[i], "-english")) {
      english = 1;
      continue;
    }
    if (!strcmp(argv[i], "-metric")) {
      metric = 1;
      continue;
    }
    if (!strcmp(argv[i], "-all")) {
      tree = stats = bits = events = wm = size = shape = 1;
      continue;
    }
    usage();
  }

  /* If no window selected on command line, let user pick one the hard way */
  if (!window) {
	  printf("\n");
	  printf("xwininfo: Please select the window about which you\n");
	  printf("          would like information by clicking the\n");
	  printf("          mouse in that window.\n");
	  window = Select_Window(dpy);
	  if (window && !frame) {
	      Window root;
	      int dummyi;
	      unsigned int dummy;

	      if (XGetGeometry (dpy, window, &root, &dummyi, &dummyi,
				&dummy, &dummy, &dummy, &dummy) &&
		  window != root)
	        window = XmuClientWindow (dpy, window);
	  }
  }

  /*
   * Do the actual displaying as per parameters
   */
  if (!(children || tree || bits || events || wm || size))
    stats = 1;

  /*
   * make sure that the window is valid
   */
  {
    Window root;
    int x, y;
    unsigned width, height, bw, depth;
    XErrorHandler old_handler;

    old_handler = XSetErrorHandler(bad_window_handler);
    XGetGeometry (dpy, window, &root, &x, &y, &width, &height, &bw, &depth);
    XSync (dpy, False);
    (void) XSetErrorHandler(old_handler);
  }

  printf("\nxwininfo: Window id: ");
  Display_Window_Id(window, True);
  if (children || tree)
    Display_Tree_Info(window, tree);
  if (stats)
    Display_Stats_Info(window);
  if (bits)
    Display_Bits_Info(window);
  if (events)
    Display_Events_Info(window);
  if (wm)
    Display_WM_Info(window);
  if (size)
    Display_Size_Hints(window);
  if (shape)
    Display_Window_Shape(window);
  printf("\n");
  exit(0);
}


/*
 * Lookup: lookup a code in a table.
 */
static char _lookup_buffer[100];

char *LookupL(code, table)
    long code;
    binding *table;
{
	char *name;

	sprintf(_lookup_buffer, "unknown (code = %ld. = 0x%lx)", code, code);
	name = _lookup_buffer;

	while (table->name) {
		if (table->code == code) {
			name = table->name;
			break;
		}
		table++;
	}

	return(name);
}

char *Lookup(code, table)
    int code;
    binding *table;
{
    return LookupL((long)code, table);
}

/*
 * Routine to display a window id in dec/hex with name if window has one
 */

void
Display_Window_Id(window, newline_wanted)
    Window window;
    Bool newline_wanted;
{
    char *win_name;
    
    printf(window_id_format, window);         /* print id # in hex/dec */

    if (!window) {
	printf(" (none)");
    } else {
	if (window == RootWindow(dpy, screen)) {
	    printf(" (the root window)");
	}
	if (!XFetchName(dpy, window, &win_name)) { /* Get window name if any */
	    printf(" (has no name)");
	} else if (win_name) {
	    printf(" \"%s\"", win_name);
	    XFree(win_name);
	} else
	    printf(" (has no name)");
    }

    if (newline_wanted)
	printf("\n");

    return;
}


/*
 * Display Stats on window
 */
static binding _window_classes[] = {
	{ InputOutput, "InputOutput" },
	{ InputOnly, "InputOnly" },
        { 0, 0 } };

static binding _map_states[] = {
	{ IsUnmapped, "IsUnMapped" },
	{ IsUnviewable, "IsUnviewable" },
	{ IsViewable, "IsViewable" },
	{ 0, 0 } };

static binding _backing_store_states[] = {
	{ NotUseful, "NotUseful" },
	{ WhenMapped, "WhenMapped" },
	{ Always, "Always" },
	{ 0, 0 } };

static binding _bit_gravity_states[] = {
	{ ForgetGravity, "ForgetGravity" },
	{ NorthWestGravity, "NorthWestGravity" },
	{ NorthGravity, "NorthGravity" },
	{ NorthEastGravity, "NorthEastGravity" },
	{ WestGravity, "WestGravity" },
	{ CenterGravity, "CenterGravity" },
	{ EastGravity, "EastGravity" },
	{ SouthWestGravity, "SouthWestGravity" },
	{ SouthGravity, "SouthGravity" },
	{ SouthEastGravity, "SouthEastGravity" },
	{ StaticGravity, "StaticGravity" },
	{ 0, 0 }};

static binding _window_gravity_states[] = {
	{ UnmapGravity, "UnmapGravity" },
	{ NorthWestGravity, "NorthWestGravity" },
	{ NorthGravity, "NorthGravity" },
	{ NorthEastGravity, "NorthEastGravity" },
	{ WestGravity, "WestGravity" },
	{ CenterGravity, "CenterGravity" },
	{ EastGravity, "EastGravity" },
	{ SouthWestGravity, "SouthWestGravity" },
	{ SouthGravity, "SouthGravity" },
	{ SouthEastGravity, "SouthEastGravity" },
	{ StaticGravity, "StaticGravity" },
	{ 0, 0 }};

static binding _visual_classes[] = {
	{ StaticGray, "StaticGray" },
	{ GrayScale, "GrayScale" },
	{ StaticColor, "StaticColor" },
	{ PseudoColor, "PseudoColor" },
	{ TrueColor, "TrueColor" },
	{ DirectColor, "DirectColor" },
	{ 0, 0 }};

void
Display_Stats_Info(window)
     Window window;
{
  XWindowAttributes win_attributes;
  XVisualInfo vistemplate, *vinfo;
  XSizeHints hints;
  int dw = DisplayWidth (dpy, screen), dh = DisplayHeight (dpy, screen);
  int rx, ry, xright, ybelow;
  int showright = 0, showbelow = 0;
  Status status;
  Window wmframe;
  int junk;
  long longjunk;
  Window junkwin;

  if (!XGetWindowAttributes(dpy, window, &win_attributes))
    Fatal_Error("Can't get window attributes.");
  vistemplate.visualid = XVisualIDFromVisual(win_attributes.visual);
  vinfo = XGetVisualInfo(dpy, VisualIDMask, &vistemplate, &junk);

  (void) XTranslateCoordinates (dpy, window, win_attributes.root, 
				-win_attributes.border_width,
				-win_attributes.border_width,
				&rx, &ry, &junkwin);
				
  xright = (dw - rx - win_attributes.border_width * 2 -
	    win_attributes.width);
  ybelow = (dh - ry - win_attributes.border_width * 2 -
	    win_attributes.height);

  printf("\n");
  printf("  Absolute upper-left X:  %s\n", xscale(rx));
  printf("  Absolute upper-left Y:  %s\n", yscale(ry));
  printf("  Relative upper-left X:  %s\n", xscale(win_attributes.x));
  printf("  Relative upper-left Y:  %s\n", yscale(win_attributes.y));
  printf("  Width: %s\n", xscale(win_attributes.width));
  printf("  Height: %s\n", yscale(win_attributes.height));
  printf("  Depth: %d\n", win_attributes.depth);
  printf("  Visual Class: %s\n", Lookup(vinfo->class, _visual_classes));
  printf("  Border width: %s\n", bscale(win_attributes.border_width));
  printf("  Class: %s\n",
  	 Lookup(win_attributes.class, _window_classes));
  printf("  Colormap: 0x%lx (%sinstalled)\n", 
	 win_attributes.colormap, win_attributes.map_installed ? "" : "not ");
  printf("  Bit Gravity State: %s\n",
  	 Lookup(win_attributes.bit_gravity, _bit_gravity_states));
  printf("  Window Gravity State: %s\n",
  	 Lookup(win_attributes.win_gravity, _window_gravity_states));
  printf("  Backing Store State: %s\n",
  	 Lookup(win_attributes.backing_store, _backing_store_states));
  printf("  Save Under State: %s\n",
  	 win_attributes.save_under ? "yes" : "no");
  printf("  Map State: %s\n",
	 Lookup(win_attributes.map_state, _map_states));
  printf("  Override Redirect State: %s\n",
  	 win_attributes.override_redirect ? "yes" : "no");
  printf("  Corners:  +%d+%d  -%d+%d  -%d-%d  +%d-%d\n",
	 rx, ry, xright, ry, xright, ybelow, rx, ybelow);

  /*
   * compute geometry string that would recreate window
   */
  printf("  -geometry ");

  /* compute size in appropriate units */
  status = XGetWMNormalHints(dpy, window, &hints, &longjunk);
  if (status  &&  hints.flags & PResizeInc  &&
              hints.width_inc != 0  &&  hints.height_inc != 0) {
      if (hints.flags & (PBaseSize|PMinSize)) {
	  if (hints.flags & PBaseSize) {
	      win_attributes.width -= hints.base_width;
	      win_attributes.height -= hints.base_height;
	  } else {
	      /* ICCCM says MinSize is default for BaseSize */
	      win_attributes.width -= hints.min_width;
	      win_attributes.height -= hints.min_height;
	  }
      }
      printf("%dx%d", win_attributes.width/hints.width_inc,
	     win_attributes.height/hints.height_inc);
  } else
      printf("%dx%d", win_attributes.width, win_attributes.height);

  if (!(hints.flags&PWinGravity))
      hints.win_gravity = NorthWestGravity; /* per ICCCM */
  /* find our window manager frame, if any */
  wmframe = window;
  while (True) {
      Window root, parent;
      Window *childlist;
      unsigned int ujunk;

      status = XQueryTree(dpy, wmframe, &root, &parent, &childlist, &ujunk);
      if (parent == root || !parent || !status)
	  break;
      wmframe = parent;
      if (status && childlist)
	  XFree((char *)childlist);
  }
  if (wmframe != window) {
      /* WM reparented, so find edges of the frame */
      /* Only works for ICCCM-compliant WMs, and then only if the
         window has corner gravity.  We would need to know the original width
	 of the window to correctly handle the other gravities. */

      XWindowAttributes frame_attr;

      if (!XGetWindowAttributes(dpy, wmframe, &frame_attr))
	  Fatal_Error("Can't get frame attributes.");
      switch (hints.win_gravity) {
	case NorthWestGravity: case SouthWestGravity:
	case NorthEastGravity: case SouthEastGravity:
	case WestGravity:
	  rx = frame_attr.x;
      }
      switch (hints.win_gravity) {
	case NorthWestGravity: case SouthWestGravity:
	case NorthEastGravity: case SouthEastGravity:
	case EastGravity:
	  xright = dw - frame_attr.x - frame_attr.width -
	      2*frame_attr.border_width;
      }
      switch (hints.win_gravity) {
	case NorthWestGravity: case SouthWestGravity:
	case NorthEastGravity: case SouthEastGravity:
	case NorthGravity:
	  ry = frame_attr.y;
      }
      switch (hints.win_gravity) {
	case NorthWestGravity: case SouthWestGravity:
	case NorthEastGravity: case SouthEastGravity:
	case SouthGravity:
	  ybelow = dh - frame_attr.y - frame_attr.height -
	      2*frame_attr.border_width;
      }
  }
  /* If edge gravity, offer a corner on that edge (because the application
     programmer cares about that edge), otherwise offer upper left unless
     some other corner is close to an edge of the screen.
     (For corner gravity, assume gravity was set by XWMGeometry.
     For CenterGravity, it doesn't matter.) */
  if (hints.win_gravity == EastGravity  ||
      (abs(xright) <= 100  &&  abs(xright) < abs(rx)
        &&  hints.win_gravity != WestGravity))
      showright = 1;
  if (hints.win_gravity == SouthGravity  ||
      (abs(ybelow) <= 100  &&  abs(ybelow) < abs(ry)
        &&  hints.win_gravity != NorthGravity))
      showbelow = 1;
  
  if (showright)
      printf("-%d", xright);
  else
      printf("+%d", rx);
  if (showbelow)
      printf("-%d", ybelow);
  else
      printf("+%d", ry);
  printf("\n");
}


/*
 * Display bits info:
 */
static binding _gravities[] = {
	{ UnmapGravity, "UnMapGravity" },      /* WARNING: both of these have*/
	{ ForgetGravity, "ForgetGravity" },    /* the same value - see code */
	{ NorthWestGravity, "NorthWestGravity" },
	{ NorthGravity, "NorthGravity" },
	{ NorthEastGravity, "NorthEastGravity" },
	{ WestGravity, "WestGravity" },
	{ CenterGravity, "CenterGravity" },
	{ EastGravity, "EastGravity" },
	{ SouthWestGravity, "SouthWestGravity" },
	{ SouthGravity, "SouthGravity" },
	{ SouthEastGravity, "SouthEastGravity" },
	{ StaticGravity, "StaticGravity" },
	{ 0, 0 } };

static binding _backing_store_hint[] = {
	{ NotUseful, "NotUseful" },
	{ WhenMapped, "WhenMapped" },
	{ Always, "Always" },
	{ 0, 0 } };

static binding _bool[] = {
	{ 0, "No" },
	{ 1, "Yes" },
	{ 0, 0 } };

void
Display_Bits_Info(window)
     Window window;
{
  XWindowAttributes win_attributes;

  if (!XGetWindowAttributes(dpy, window, &win_attributes))
    Fatal_Error("Can't get window attributes.");

  printf("\n");
  printf("  Bit gravity: %s\n",
	 Lookup(win_attributes.bit_gravity, _gravities+1));
  printf("  Window gravity: %s\n",
	 Lookup(win_attributes.win_gravity, _gravities));
  printf("  Backing-store hint: %s\n",
	 Lookup(win_attributes.backing_store, _backing_store_hint));
  printf("  Backing-planes to be preserved: 0x%lx\n",
	 win_attributes.backing_planes);
  printf("  Backing pixel: %ld\n", win_attributes.backing_pixel);
  printf("  Save-unders: %s\n",
	 Lookup(win_attributes.save_under, _bool));
}


/*
 * Routine to display all events in an event mask
 */
static binding _event_mask_names[] = {
	{ KeyPressMask, "KeyPress" },
	{ KeyReleaseMask, "KeyRelease" },
	{ ButtonPressMask, "ButtonPress" },
	{ ButtonReleaseMask, "ButtonRelease" },
	{ EnterWindowMask, "EnterWindow" },
	{ LeaveWindowMask, "LeaveWindow" },
	{ PointerMotionMask, "PointerMotion" },
	{ PointerMotionHintMask, "PointerMotionHint" },
	{ Button1MotionMask, "Button1Motion" },
	{ Button2MotionMask, "Button2Motion" },
	{ Button3MotionMask, "Button3Motion" },
	{ Button4MotionMask, "Button4Motion" },
	{ Button5MotionMask, "Button5Motion" },
	{ ButtonMotionMask, "ButtonMotion" },
	{ KeymapStateMask, "KeymapState" },
	{ ExposureMask, "Exposure" },
	{ VisibilityChangeMask, "VisibilityChange" },
	{ StructureNotifyMask, "StructureNotify" },
	{ ResizeRedirectMask, "ResizeRedirect" },
	{ SubstructureNotifyMask, "SubstructureNotify" },
	{ SubstructureRedirectMask, "SubstructureRedirect" },
	{ FocusChangeMask, "FocusChange" },
	{ PropertyChangeMask, "PropertyChange" },
	{ ColormapChangeMask, "ColormapChange" },
	{ OwnerGrabButtonMask, "OwnerGrabButton" },
	{ 0, 0 } };

void
Display_Event_Mask(mask)
     long mask;
{
  long bit, bit_mask;

  for (bit=0, bit_mask=1; bit<sizeof(long)*8; bit++, bit_mask <<= 1)
    if (mask & bit_mask)
      printf("      %s\n",
	     LookupL(bit_mask, _event_mask_names));
}


/*
 * Display info on events
 */
void
Display_Events_Info(window)
     Window window;
{
  XWindowAttributes win_attributes;

  if (!XGetWindowAttributes(dpy, window, &win_attributes))
    Fatal_Error("Can't get window attributes.");

  printf("\n");
  printf("  Someone wants these events:\n");
  Display_Event_Mask(win_attributes.all_event_masks);

  printf("  Do not propagate these events:\n");
  Display_Event_Mask(win_attributes.do_not_propagate_mask);

  printf("  Override redirection?: %s\n",
	 Lookup(win_attributes.override_redirect, _bool));
}


  /* left out visual stuff */
  /* left out colormap */
  /* left out map_installed */


/*
 * Display root, parent, and (recursively) children information
 */
void
Display_Tree_Info(window, recurse)
     Window window;
     int recurse;		/* true if should show children's children */
{
    display_tree_info_1(window, recurse, 0);
}

void
display_tree_info_1(window, recurse, level)
     Window window;
     int recurse;
     int level;			/* recursion level */
{
  int i, j;
  int rel_x, rel_y, abs_x, abs_y;
  unsigned int width, height, border, depth;
  Window root_win, parent_win;
  unsigned int num_children;
  Window *child_list;
  XClassHint classhint;

  if (!XQueryTree(dpy, window, &root_win, &parent_win, &child_list,
		  &num_children))
    Fatal_Error("Can't query window tree.");

  if (level == 0) {
    printf("\n");
    printf("  Root window id: ");
    Display_Window_Id(root_win, True);
    printf("  Parent window id: ");
    Display_Window_Id(parent_win, True);
  }

  if (level == 0  ||  num_children > 0) {
    printf("     ");
    for (j=0; j<level; j++) printf("   ");
    printf("%d child%s%s\n", num_children, num_children == 1 ? "" : "ren",
	   num_children ? ":" : ".");
  }

  for (i = (int)num_children - 1; i >= 0; i--) {
    printf("     ");
    for (j=0; j<level; j++) printf("   ");
    Display_Window_Id(child_list[i], False);
    printf(": (");
    if(XGetClassHint(dpy, child_list[i], &classhint)) {
	if(classhint.res_name) {
	    printf("\"%s\" ", classhint.res_name);
	    XFree(classhint.res_name);
	} else
	    printf("(none) ");
	if(classhint.res_class) {
	    printf("\"%s\") ", classhint.res_class);
	    XFree(classhint.res_class);
	} else
	    printf("(none)) ");
    } else
	printf(") ");

    if (XGetGeometry(dpy, child_list[i], &root_win,
		     &rel_x, &rel_y, &width, &height, &border, &depth)) {
	Window child;

	printf (" %ux%u+%d+%d", width, height, rel_x, rel_y);
	if (XTranslateCoordinates (dpy, child_list[i], root_win,
				   0 ,0, &abs_x, &abs_y, &child)) {
	    printf ("  +%d+%d", abs_x - border, abs_y - border);
	}
    }
    printf("\n");
    
    if (recurse)
	display_tree_info_1(child_list[i], 1, level+1);
  }

  if (child_list) XFree((char *)child_list);
}


/*
 * Display a set of size hints
 */
void
Display_Hints(hints)
     XSizeHints *hints;
{
	long flags;

	flags = hints->flags;
	
	if (flags & USPosition)
	  printf("      User supplied location: %s, %s\n",
		 xscale(hints->x), yscale(hints->y));

	if (flags & PPosition)
	  printf("      Program supplied location: %s, %s\n",
		 xscale(hints->x), yscale(hints->y));

	if (flags & USSize) {
	  printf("      User supplied size: %s by %s\n",
		 xscale(hints->width), yscale(hints->height));
	}

	if (flags & PSize)
	  printf("      Program supplied size: %s by %s\n",
		 xscale(hints->width), yscale(hints->height));

	if (flags & PMinSize)
	  printf("      Program supplied minimum size: %s by %s\n",
		 xscale(hints->min_width), yscale(hints->min_height));

	if (flags & PMaxSize)
	  printf("      Program supplied maximum size: %s by %s\n",
		 xscale(hints->max_width), yscale(hints->max_height));

	if (flags & PBaseSize) {
	  printf("      Program supplied base size: %s by %s\n",
		 xscale(hints->base_width), yscale(hints->base_height));
	}

	if (flags & PResizeInc) {
	  printf("      Program supplied x resize increment: %s\n",
		 xscale(hints->width_inc));
	  printf("      Program supplied y resize increment: %s\n",
		 yscale(hints->height_inc));
	  if (hints->width_inc != 0 && hints->height_inc != 0) {
	      if (flags & USSize)
		  printf("      User supplied size in resize increments:  %s by %s\n",
			 (xscale(hints->width / hints->width_inc)), 
			 (yscale(hints->height / hints->height_inc)));
	      if (flags & PSize)
		  printf("      Program supplied size in resize increments:  %s by %s\n",
			 (xscale(hints->width / hints->width_inc)), 
			 (yscale(hints->height / hints->height_inc)));
	      if (flags & PMinSize)
		  printf("      Program supplied minimum size in resize increments: %s by %s\n",
			 xscale(hints->min_width / hints->width_inc), yscale(hints->min_height / hints->height_inc));
	      if (flags & PBaseSize)
		  printf("      Program supplied base size in resize increments:  %s by %s\n",
			 (xscale(hints->base_width / hints->width_inc)), 
			 (yscale(hints->base_height / hints->height_inc)));
	  }
        }

	if (flags & PAspect) {
	  printf("      Program supplied min aspect ratio: %s/%s\n",
		 xscale(hints->min_aspect.x), yscale(hints->min_aspect.y));
	  printf("      Program supplied max aspect ratio: %s/%s\n",
		 xscale(hints->max_aspect.x), yscale(hints->max_aspect.y));
        }

	if (flags & PWinGravity) {
	  printf("      Program supplied window gravity: %s\n",
		 Lookup(hints->win_gravity, _gravities));
	}
}


/*
 * Display Size Hints info
 */
void
Display_Size_Hints(window)
     Window window;
{
	XSizeHints *hints = XAllocSizeHints();
	long supplied;

	printf("\n");
	if (!XGetWMNormalHints(dpy, window, hints, &supplied))
	    printf("  No normal window size hints defined\n");
	else {
	    printf("  Normal window size hints:\n");
	    hints->flags &= supplied;
	    Display_Hints(hints);
	}

	if (!XGetWMSizeHints(dpy, window, hints, &supplied, XA_WM_ZOOM_HINTS))
	    printf("  No zoom window size hints defined\n");
	else {
	    printf("  Zoom window size hints:\n");
	    hints->flags &= supplied;
	    Display_Hints(hints);
	}
	XFree((char *)hints);
}


void
Display_Window_Shape (window)
    Window  window;
{
    Bool    ws, bs;
    int	    xws, yws, xbs, ybs;
    unsigned int wws, hws, wbs, hbs;

    if (!XShapeQueryExtension (dpy, &bs, &ws))
	return;

    printf("\n");
    XShapeQueryExtents (dpy, window, &ws, &xws, &yws, &wws, &hws,
				     &bs, &xbs, &ybs, &wbs, &hbs);
    if (!ws)
	  printf("  No window shape defined\n");
    else {
	  printf("  Window shape extents:  %sx%s",
		 xscale(wws), yscale(hws));
	  printf("+%s+%s\n", xscale(xws), yscale(yws));
    }
    if (!bs)
	  printf("  No border shape defined\n");
    else {
	  printf("  Border shape extents:  %sx%s",
		 xscale(wbs), yscale(hbs));
	  printf("+%s+%s\n", xscale(xbs), yscale(ybs));
    }
}

/*
 * Display Window Manager Info
 */
static binding _state_hints[] = {
	{ DontCareState, "Don't Care State" },
	{ NormalState, "Normal State" },
	{ ZoomState, "Zoomed State" },
	{ IconicState, "Iconic State" },
	{ InactiveState, "Inactive State" },
	{ 0, 0 } };

void
Display_WM_Info(window)
     Window window;
{
        XWMHints *wmhints;
	long flags;

	wmhints = XGetWMHints(dpy, window);
	printf("\n");
	if (!wmhints) {
		printf("  No window manager hints defined\n");
		return;
	}
	flags = wmhints->flags;

	printf("  Window manager hints:\n");

	if (flags & InputHint)
	  printf("      Client accepts input or input focus: %s\n",
		 Lookup(wmhints->input, _bool));

	if (flags & IconWindowHint) {
		printf("      Icon window id: ");
		Display_Window_Id(wmhints->icon_window, True);
	}

	if (flags & IconPositionHint)
	  printf("      Initial icon position: %s, %s\n",
		 xscale(wmhints->icon_x), yscale(wmhints->icon_y));

	if (flags & StateHint)
	  printf("      Initial state is %s\n",
		 Lookup(wmhints->initial_state, _state_hints));
}
