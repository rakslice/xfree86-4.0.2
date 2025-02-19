/*
 * $XFree86: xc/lib/Xft/xftint.h,v 1.15 2000/12/15 17:12:53 keithp Exp $
 *
 * Copyright � 2000 Keith Packard, member of The XFree86 Project, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Keith Packard not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Keith Packard makes no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * KEITH PACKARD DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL KEITH PACKARD BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef _XFTINT_H_
#define _XFTINT_H_

#include <X11/Xlib.h>
#ifdef FREETYPE2
#include "XftFreetype.h"
#else
#include "Xft.h"
#endif

typedef struct _XftMatcher {
    char    *object;
    double  (*compare) (char *object, XftValue value1, XftValue value2);
} XftMatcher;

typedef struct _XftSymbolic {
    const char	*name;
    int		value;
} XftSymbolic;

struct _XftDraw {
    Display	    *dpy;
    Drawable	    drawable;
    Visual	    *visual;	/* NULL for bitmaps */
    Colormap	    colormap;
    Region	    clip;
    Bool	    core_set;
    Bool	    render_set;
    Bool	    render_able;
    struct {
	Picture		pict;
	Pixmap		fg_pix;
	Picture		fg_pict;
	XRenderColor	fg_color;
    } render;
    struct {
	GC		draw_gc;
	unsigned long	fg;
	Font		font;
    } core;
};

typedef struct _XftDisplayInfo {
    struct _XftDisplayInfo  *next;
    Display		    *display;
    XExtCodes		    *codes;
    XftPattern		    *defaults;
    XftFontSet		    *coreFonts;
    Bool		    hasRender;
} XftDisplayInfo;

extern XftFontSet	*_XftGlobalFontSet;
extern XftDisplayInfo	*_XftDisplayInfo;
extern char		**XftConfigDirs;
extern XftFontSet	*_XftFontSet;

#define XFT_NMISSING	256

#ifndef XFT_DEFAULT_PATH
#define XFT_DEFAULT_PATH "/usr/X11R6/lib/X11/XftConfig"
#endif

typedef enum _XftOp {
    XftOpInteger, XftOpDouble, XftOpString, XftOpBool, XftOpNil,
    XftOpField,
    XftOpAssign, XftOpPrepend, XftOpAppend,
    XftOpQuest,
    XftOpOr, XftOpAnd, XftOpEqual, XftOpNotEqual,
    XftOpLess, XftOpLessEqual, XftOpMore, XftOpMoreEqual,
    XftOpPlus, XftOpMinus, XftOpTimes, XftOpDivide,
    XftOpNot
} XftOp;

typedef struct _XftExpr {
    XftOp   op;
    union {
	int	ival;
	double	dval;
	char	*sval;
	Bool	bval;
	char	*field;
	struct {
	    struct _XftExpr *left, *right;
	} tree;
    } u;
} XftExpr;

typedef enum _XftQual {
    XftQualAny, XftQualAll
} XftQual;

typedef struct _XftTest {
    struct _XftTest	*next;
    XftQual		qual;
    char		*field;
    XftOp		op;
    XftValue		value;
} XftTest;

typedef struct _XftEdit {
    struct _XftEdit *next;
    const char	    *field;
    XftOp	    op;
    XftExpr	    *expr;
} XftEdit;

typedef struct _XftSubst {
    struct _XftSubst	*next;
    XftTest		*test;
    XftEdit		*edit;
} XftSubst;

/*
 * I tried this with functions that took va_list* arguments
 * but portability concerns made me change these functions
 * into macros (sigh).
 */

#define _XftPatternVapBuild(result, orig, va)			    \
{								    \
    XftPattern	*__p__ = (orig);				    \
    const char	*__o__;						    \
    XftValue	__v__;						    \
								    \
    if (!__p__)							    \
    {								    \
	__p__ = XftPatternCreate ();				    \
	if (!__p__)		    				    \
	    goto _XftPatternVapBuild_bail0;			    \
    }				    				    \
    for (;;)			    				    \
    {				    				    \
	__o__ = va_arg (va, const char *);			    \
	if (!__o__)		    				    \
	    break;		    				    \
	__v__.type = va_arg (va, XftType);			    \
	switch (__v__.type) {	    				    \
	case XftTypeVoid:					    \
	    goto _XftPatternVapBuild_bail1;       		    \
	case XftTypeInteger:	    				    \
	    __v__.u.i = va_arg (va, int);			    \
	    break;						    \
	case XftTypeDouble:					    \
	    __v__.u.d = va_arg (va, double);			    \
	    break;						    \
	case XftTypeString:					    \
	    __v__.u.s = va_arg (va, char *);			    \
	    break;						    \
	case XftTypeBool:					    \
	    __v__.u.b = va_arg (va, Bool);			    \
	    break;						    \
	}							    \
	if (!XftPatternAdd (__p__, __o__, __v__, True))		    \
	    goto _XftPatternVapBuild_bail1;			    \
    }								    \
    result = __p__;						    \
    goto _XftPatternVapBuild_return;				    \
								    \
_XftPatternVapBuild_bail1:					    \
    if (!orig)							    \
	XftPatternDestroy (__p__);				    \
_XftPatternVapBuild_bail0:					    \
    result = 0;							    \
								    \
_XftPatternVapBuild_return:					    \
    ;								    \
}


/* xftcfg.c */
Bool
XftConfigAddDir (char *d);

Bool
XftConfigAddEdit (XftTest *test, XftEdit *edit);

Bool
_XftConfigCompareValue (XftValue    m,
			XftOp	    op,
			XftValue    v);

/* xftcore.c */

#define XFT_CORE_N16LOCAL	256

XChar2b *
XftCoreConvert16 (XftChar16	    *string,
		  int		    len,
		  XChar2b	    xcloc[XFT_CORE_N16LOCAL]);

XChar2b *
XftCoreConvert32 (unsigned int	    *string,
		  int		    len,
		  XChar2b	    xcloc[XFT_CORE_N16LOCAL]);

void
XftCoreExtents8 (Display	*dpy,
		 XFontStruct	*fs,
		 XftChar8	*string, 
		 int		len,
		 XGlyphInfo	*extents);

void
XftCoreExtents16 (Display	    *dpy,
		  XFontStruct	    *fs,
		  XftChar16	    *string, 
		  int		    len,
		  XGlyphInfo	    *extents);

void
XftCoreExtents32 (Display	    *dpy,
		  XFontStruct	    *fs,
		  unsigned int	    *string, 
		  int		    len,
		  XGlyphInfo	    *extents);

Bool
XftCoreGlyphExists (Display	    *dpy,
		    XFontStruct	    *fs,
		    unsigned int    glyph);

/* xftdbg.c */
void
XftOpPrint (XftOp op);

void
XftTestPrint (XftTest *test);

void
XftExprPrint (XftExpr *expr);

void
XftEditPrint (XftEdit *edit);

void
XftSubstPrint (XftSubst *subst);

/* xftdir.c */
Bool
XftDirScan (XftFontSet *set, const char *dir);

/* xftdpy.c */
int
XftDefaultParseBool (char *v);

Bool
XftDefaultGetBool (Display *dpy, const char *object, int screen, Bool def);

int
XftDefaultGetInteger (Display *dpy, const char *object, int screen, int def);

double
XftDefaultGetDouble (Display *dpy, const char *object, int screen, double def);

XftFontSet *
XftDisplayGetFontSet (Display *dpy);

/* xftdraw.c */
Bool
XftDrawRenderPrepare (XftDraw	*draw,
		      XftColor	*color,
		      XftFont	*font);

Bool
XftDrawCorePrepare (XftDraw	*draw,
		    XftColor	*color,
		    XftFont	*font);

/* xftextent.c */
/* xftfont.c */
int
_XftFontDebug (void);
    
/* xftfreetype.c */
XftPattern *
XftFreeTypeQuery (const char *file, int id, int *count);

/* xftfs.c */
/* xftglyphs.c */
/* xftgram.y */
int
XftConfigparse (void);

int
XftConfigwrap (void);
    
void
XftConfigerror (char *fmt, ...);
    
char *
XftConfigSaveField (const char *field);

XftTest *
XftTestCreate (XftQual qual, const char *field, XftOp compare, XftValue value);

XftExpr *
XftExprCreateInteger (int i);

XftExpr *
XftExprCreateDouble (double d);

XftExpr *
XftExprCreateString (const char *s);

XftExpr *
XftExprCreateBool (Bool b);

XftExpr *
XftExprCreateNil (void);

XftExpr *
XftExprCreateField (const char *field);

XftExpr *
XftExprCreateOp (XftExpr *left, XftOp op, XftExpr *right);

void
XftExprDestroy (XftExpr *e);

XftEdit *
XftEditCreate (const char *field, XftOp op, XftExpr *expr);

void
XftEditDestroy (XftEdit *e);

/* xftinit.c */
Bool
XftInitFtLibrary (void);

/* xftlex.l */
extern int	XftConfigLineno;
extern char	*XftConfigFile;

int
XftConfiglex (void);

Bool
XftConfigLexFile(char *s);

Bool
XftConfigPushInput (char *s, Bool complain);

/* xftlist.c */
XftObjectSet *
_XftObjectSetVapBuild (const char *first, va_list *vap);

Bool
XftListValueCompare (XftValue	v1,
		     XftValue	v2);

Bool
XftListValueListCompare (XftValueList	*v1orig,
			 XftValueList	*v2orig,
			 XftQual	qual);

Bool
XftListMatch (XftPattern    *p,
	      XftPattern    *font,
	      XftQual	    qual);

Bool
XftListAppend (XftFontSet   *s,
	       XftPattern   *font,
	       XftObjectSet *os);


/* xftmatch.c */

/* xftname.c */
Bool
XftNameConstant (char *string, int *result);

/* xftpat.c */

/* xftrender.c */

/* xftstr.c */
char *
_XftSaveString (const char *s);

const char *
_XftGetInt(const char *ptr, int *val);

char *
_XftSplitStr (const char *field, char *save);

char *
_XftDownStr (const char *field, char *save);

const char *
_XftSplitField (const char *field, char *save);

const char *
_XftSplitValue (const char *field, char *save);

int
_XftMatchSymbolic (XftSymbolic *s, int n, const char *name, int def);

int
_XftStrCmpIgnoreCase (const char *s1, const char *s2);
    
/* xftxlfd.c */
Bool
XftCoreAddFonts (XftFontSet *set, Display *dpy, Bool ignore_scalable);

#endif /* _XFT_INT_H_ */
