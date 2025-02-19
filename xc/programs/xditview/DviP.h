/*
 * $XConsortium: DviP.h,v 1.10 92/02/11 01:27:15 keith Exp $
 */
/* $XFree86: xc/programs/xditview/DviP.h,v 1.3 2000/12/04 21:01:01 dawes Exp $ */

/* 
 * DviP.h - Private definitions for Dvi widget
 */

#ifndef _XtDviP_h
#define _XtDviP_h

#ifdef USE_XFT
#include <X11/Xft/Xft.h>
#endif

#include "Dvi.h"
#include <X11/Xaw/SimpleP.h>
#include "DviChar.h"

/***********************************************************************
 *
 * Dvi Widget Private Data
 *
 ***********************************************************************/

/************************************
 *
 *  Class structure
 *
 ***********************************/

/*
 * New fields for the Dvi widget class record
 */

typedef struct _DviClass {
	int		makes_compiler_happy;  /* not used */
} DviClassPart;

/*
 * Full class record declaration
 */

typedef struct _DviClassRec {
    CoreClassPart	core_class;
    SimpleClassPart	simple_class;
    DviClassPart	command_class;
} DviClassRec;

extern DviClassRec dviClassRec;

/***************************************
 *
 *  Instance (widget) structure 
 *
 **************************************/

/*
 * a list of fonts we've used for this widget
 */

typedef struct _dviFontSizeList {
	struct _dviFontSizeList	*next;
	int			size;
	char			*x_name;
#ifdef USE_XFT
	XftFont			*font;
	Bool			core;
#else
	XFontStruct		*font;
#endif
	int			doesnt_exist;
} DviFontSizeList;

typedef struct _dviFontList {
	struct _dviFontList	*next;
	char			*dvi_name;
	char			*x_name;
	int			dvi_number;
	Boolean			initialized;
	Boolean			scalable;
	DviFontSizeList		*sizes;
	DviCharNameMap		*char_map;
} DviFontList;

typedef struct _dviFontMap {
	struct _dviFontMap	*next;
	char			*dvi_name;
	char			*x_name;
} DviFontMap;

#define DVI_TEXT_CACHE_SIZE	256
#define DVI_CHAR_CACHE_SIZE	1024

#ifdef USE_XFT
typedef struct _dviTextItem {
	char		*chars;
	int		nchars;
	int		x;
	XftFont		*font;
} DviTextItem;
#endif

typedef struct _dviCharCache {
#ifdef USE_XFT
	DviTextItem	cache[DVI_TEXT_CACHE_SIZE];
#else
	XTextItem	cache[DVI_TEXT_CACHE_SIZE];
#endif
	char		char_cache[DVI_CHAR_CACHE_SIZE];
	int		index;
	int		max;
	int		char_index;
	int		font_size;
	int		font_number;
#ifdef USE_XFT
	XftFont		*font;
#else
	XFontStruct	*font;
#endif
	int		start_x, start_y;
	int		x, y;
} DviCharCache;

typedef struct _dviState {
	struct _dviState	*next;
	int			font_size;
	int			font_bound;
	int			font_number;
	int			line_style;
	int			line_width;
	int			x;
	int			y;
} DviState;

typedef struct _dviFileMap {
	struct _dviFileMap	*next;
	long			position;
	int			page_number;
} DviFileMap;

/*
 * New fields for the Dvi widget record
 */

typedef struct {
	/*
	 * resource specifiable items
	 */
	char		*font_map_string;
	unsigned long	foreground;
	int		requested_page;
	int		last_page;
	FILE		*file;
	Boolean		seek;		/* file is "seekable" */
#ifdef USE_XFT
	XftFont		*default_font;
#else
	XFontStruct	*default_font;
#endif
	int		backing_store;
	Boolean		noPolyText;
	int		screen_resolution;
	float		page_width;
	float		page_height;
	int		size_scale_set;
	/*
 	 * private state
 	 */
	FILE		*tmpFile;	/* used when reading stdin */
	char		readingTmp;	/* reading now from tmp */
	char		ungot;		/* have ungetc'd a char */
	GC		normal_GC;
#ifdef USE_XFT
	XftDraw		*draw;
	XftColor	black;
#endif
	DviFileMap	*file_map;
	DviFontList	*fonts;
	DviFontMap	*font_map;
	int		current_page;
	int		font_size;
	int		font_number;
	int		device_resolution;
	int		line_width;
	int		line_style;
	int		desired_width;
	int		desired_height;
	int		size_scale;	/* font size scale */
#ifdef USE_XFT
	XftFont		*font;
#else
	XFontStruct	*font;
#endif
	int		display_enable;
	double		scale;		/* device coordinates to pixels */
	struct ExposedExtents {
	    int x1, y1, x2, y2;
	}		extents;
	DviState	*state;
	DviCharCache	cache;
} DviPart;

#define DviGetIn(dw,cp)\
    (dw->dvi.tmpFile ? (\
	DviGetAndPut (dw, cp) \
    ) :\
	(*cp = getc (dw->dvi.file))\
)

#define DviGetC(dw, cp)\
    (dw->dvi.readingTmp ? (\
	((*cp = getc (dw->dvi.tmpFile)) == EOF) ? (\
	    fseek (dw->dvi.tmpFile, 0l, 2),\
	    (dw->dvi.readingTmp = 0),\
	    DviGetIn (dw,cp)\
	) : (\
	    *cp\
	)\
    ) : (\
	DviGetIn(dw,cp)\
    )\
)

#define DviUngetC(dw, c)\
    (dw->dvi.readingTmp ? (\
	ungetc (c, dw->dvi.tmpFile)\
    ) : ( \
	(dw->dvi.ungot = 1),\
	ungetc (c, dw->dvi.file)))

#define ToX(dw,device)		    ((int) ((device) * (dw)->dvi.scale + 0.5))
#define ToDevice(dw,x)		    ((int) ((x) / (dw)->dvi.scale + 0.5))
#define FontSizeInPixels(dw,size)   ((int) ((size) * (dw)->dvi.screen_resolution / ((dw)->dvi.size_scale * 72)))
#define FontSizeInDevice(dw,size)   ((int) ((size) * (dw)->dvi.device_resolution / ((dw)->dvi.size_scale * 72)))

/*
 * Full widget declaration
 */

typedef struct _DviRec {
    CorePart	core;
    SimplePart	simple;
    DviPart	dvi;
} DviRec;

#ifdef USE_XFT
extern XftFont		*QueryFont ();
#else
extern XFontStruct	*QueryFont ();
#endif

extern DviCharNameMap	*QueryFontMap ();
#endif /* _XtDviP_h */


