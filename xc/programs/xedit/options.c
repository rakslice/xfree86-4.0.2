/*
 * Copyright (c) 1999 by The XFree86 Project, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *  
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE XFREE86 PROJECT BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Except as contained in this notice, the name of the XFree86 Project shall
 * not be used in advertising or otherwise to promote the sale, use or other
 * dealings in this Software without prior written authorization from the
 * XFree86 Project.
 *
 * Author: Paulo C�sar Pereira de Andrade
 */

/* $XFree86: xc/programs/xedit/options.c,v 1.8 2000/09/26 15:57:24 tsi Exp $ */

#include <stdio.h>
#ifndef X_NOT_STDC_ENV
#include <stdlib.h>
#endif
#include "xedit.h"

#include <X11/Xaw/SmeBSB.h>
#include <X11/Xaw/SimpleMenu.h>

/*
 * Types
 */
typedef struct _property_info {
    Boolean automatic;
    char *ext_res, *prop_res;
    char **extensions;
    int num_extensions;
    XawTextPropertyList *properties;
    void (*SetMode)(Widget);
    void (*UnsetMode)(Widget);
} property_info;

/*
 * Prototypes
 */
static void SetColumns(Widget, XEvent*, String*, Cardinal*);
static void ChangeField(Widget, XEvent*, String*, Cardinal*);
static void EditCallback(Widget, XtPointer, XtPointer);
static void ModeCallback(Widget, XtPointer, XtPointer);
static void PopupColumnsCallback(Widget, XtPointer, XtPointer);
static void CreateColumnsShell(void);
static void ProcessColumnsCallback(Widget, XtPointer, XtPointer);
static void DoSetTextProperties(xedit_flist_item*, property_info*);

/*
 * externs in c-mode.c
 */
extern void C_ModeStart(Widget);
extern void C_ModeEnd(Widget);

extern void _XawTextBuildLineTable(TextWidget, XawTextPosition, _XtBoolean);

/*
 * Initialization
 */
extern Widget texts[3];

static Widget edit_popup, wrap_popup, justify_popup, scroll_popup,
	      columns_shell, left_text, right_text, mode_popup;

static XFontStruct *fonts[3];
static Pixel foregrounds[3], backgrounds[3];

static XtActionsRec actions[] = {
    {"set-columns", SetColumns},
    {"change-field", ChangeField},
};

#define	C_MODE		0
static property_info property_list[] = {
    {
	/* C */
	False,		/* automatic */
	NULL,		/* ext_res */
	NULL,		/* prop_res */
	NULL,		/* extensions */
	0,		/* num_extensions */
	NULL,		/* properties */
	C_ModeStart,	/* SetMode */
	C_ModeEnd	/* UnsetMode */
    },
};

#define Offset(field) XtOffsetOf(struct _property_info, field)
static XtResource C_resources[] = {
    {"auto", "Auto", XtRBoolean, sizeof(Boolean),
	Offset(automatic), XtRImmediate, (XtPointer)True},
    {"extensions", "Extensions", XtRString, sizeof(char*),
	Offset(ext_res), XtRString, "c,h,cc,C"},
    {"properties", "Properties", XtRString, sizeof(char*),
	Offset(prop_res), XtRString, "error?background=black&foreground=white"},
};
#undef Offset

#define WRAP_NEVER	1
#define WRAP_LINE	2
#define WRAP_WORD	3
#define	AUTO_FILL	4
#define JUST_LEFT	5
#define JUST_RIGHT	6
#define JUST_CENTER	7
#define JUST_FULL	8
#define SCROLL_VERT	9
#define SCROLL_HORIZ	10

static Widget autoFill, wrapNever, wrapLine, wrapWord,
	      justifyLeft, justifyRight, justifyCenter, justifyFull,
	      breakColumns, scrollVert, scrollHoriz, modeNone, modeC;

void
CreateEditPopup(void)
{
    Arg args[1];

    edit_popup		= XtCreatePopupShell("editMenu", simpleMenuWidgetClass,
					     topwindow, NULL, 0);
    XtRealizeWidget(edit_popup);

    wrap_popup		= XtCreatePopupShell("wrapMenu", simpleMenuWidgetClass,
					     edit_popup, NULL, 0);
    XtRealizeWidget(wrap_popup);

    XtSetArg(args[0], XtNmenuName, "wrapMenu");
    XtCreateManagedWidget("wrapMenuItem", smeBSBObjectClass, edit_popup, args, 1);

    wrapNever		= XtCreateManagedWidget("never", smeBSBObjectClass,
						wrap_popup, NULL, 0);
    XtAddCallback(wrapNever, XtNcallback, EditCallback, (XtPointer)WRAP_NEVER);
    wrapLine		= XtCreateManagedWidget("line", smeBSBObjectClass,
						wrap_popup, NULL, 0);
    XtAddCallback(wrapLine, XtNcallback, EditCallback, (XtPointer)WRAP_LINE);
    wrapWord		= XtCreateManagedWidget("word", smeBSBObjectClass,
						wrap_popup, NULL, 0);
    XtAddCallback(wrapWord, XtNcallback, EditCallback, (XtPointer)WRAP_WORD);

    autoFill		= XtCreateManagedWidget("autoFill", smeBSBObjectClass,
						edit_popup, NULL, 0);
    XtAddCallback(autoFill, XtNcallback, EditCallback, (XtPointer)AUTO_FILL);

    justify_popup	= XtCreatePopupShell("justifyMenu", simpleMenuWidgetClass,
					     edit_popup, NULL, 0);
    XtRealizeWidget(justify_popup);

    XtSetArg(args[0], XtNmenuName, "justifyMenu");
    XtCreateManagedWidget("justifyMenuItem", smeBSBObjectClass, edit_popup, args, 1);

    justifyLeft		= XtCreateManagedWidget("left", smeBSBObjectClass,
						justify_popup, NULL, 0);
    XtAddCallback(justifyLeft, XtNcallback, EditCallback, (XtPointer)JUST_LEFT);
    justifyRight	= XtCreateManagedWidget("right", smeBSBObjectClass,
						justify_popup, NULL, 0);
    XtAddCallback(justifyRight, XtNcallback, EditCallback, (XtPointer)JUST_RIGHT);
    justifyCenter	= XtCreateManagedWidget("center", smeBSBObjectClass,
						justify_popup, NULL, 0);
    XtAddCallback(justifyCenter, XtNcallback, EditCallback, (XtPointer)JUST_CENTER);
    justifyFull		= XtCreateManagedWidget("full", smeBSBObjectClass,
						justify_popup, NULL, 0);
    XtAddCallback(justifyFull, XtNcallback, EditCallback, (XtPointer)JUST_FULL);

    breakColumns	= XtCreateManagedWidget("breakColumns", smeBSBObjectClass,
						edit_popup, NULL, 0);
    XtAddCallback(breakColumns, XtNcallback, PopupColumnsCallback, NULL);

    scroll_popup	= XtCreatePopupShell("scrollMenu", simpleMenuWidgetClass,
					     edit_popup, NULL, 0);
    XtRealizeWidget(scroll_popup);

    XtSetArg(args[0], XtNmenuName, "scrollMenu");
    XtCreateManagedWidget("scrollMenuItem", smeBSBObjectClass, edit_popup, args, 1);

    scrollVert		= XtCreateManagedWidget("vertical", smeBSBObjectClass,
						scroll_popup, NULL, 0);
    XtAddCallback(scrollVert, XtNcallback, EditCallback, (XtPointer)SCROLL_VERT);
    scrollHoriz		= XtCreateManagedWidget("horizontal", smeBSBObjectClass,
						scroll_popup, NULL, 0);
    XtAddCallback(scrollHoriz, XtNcallback, EditCallback, (XtPointer)SCROLL_HORIZ);

    if (international == False) {
	char *list, *str;

	mode_popup	= XtCreatePopupShell("editModes", simpleMenuWidgetClass,
					     edit_popup, NULL, 0);
	XtRealizeWidget(mode_popup);

	XtSetArg(args[0], XtNmenuName, "editModes");
	XtCreateManagedWidget("modeMenuItem", smeBSBObjectClass, edit_popup, args, 1);

	modeNone	= XtCreateManagedWidget("none", smeBSBObjectClass,
						mode_popup, NULL, 0);
	XtAddCallback(modeNone, XtNcallback, ModeCallback, (XtPointer)NULL);
	modeC		= XtCreateManagedWidget("C", smeBSBObjectClass,
						mode_popup, NULL, 0);
	XtGetApplicationResources(modeC, (XtPointer)&property_list[C_MODE],
				  C_resources, XtNumber(C_resources), NULL, 0);
	property_list[C_MODE].properties =
	    XawTextSinkConvertPropertyList("C", property_list[C_MODE].prop_res,
					   topwindow->core.screen,
					   topwindow->core.colormap,
					   topwindow->core.depth);
	list = XtNewString(property_list[C_MODE].ext_res);
	for (str = strtok(list, " \t,"); str; str = strtok(NULL, " \t,")) {
	    property_list[C_MODE].extensions =
		(char**)XtRealloc((XtPointer)property_list[C_MODE].extensions,
				  (property_list[C_MODE].num_extensions + 1) *
				  sizeof(char*));
	    property_list[C_MODE].extensions
		[property_list[C_MODE].num_extensions++] = XtNewString(str);
	}
	XtFree(list);

	XtAddCallback(modeC, XtNcallback, ModeCallback, (XtPointer)&property_list[C_MODE]);
    }
}

void
SetEditMenu(void)
{
    Arg args[7];
    Cardinal num_args;
    Boolean auto_fill;
    XawTextWrapMode wrap_mode;
    XawTextJustifyMode justify;
    XawTextScrollMode vscroll, hscroll;
    short left, right;
    XawTextPropertyList *prop;

    num_args = 0;
    XtSetArg(args[num_args], XtNwrap, &wrap_mode);		++num_args;
    XtSetArg(args[num_args], XtNautoFill, &auto_fill);		++num_args;
    XtSetArg(args[num_args], XtNjustifyMode, &justify);		++num_args;
    XtSetArg(args[num_args], XtNleftColumn, &left);		++num_args;
    XtSetArg(args[num_args], XtNrightColumn, &right);		++num_args;
    XtSetArg(args[num_args], XtNscrollVertical, &vscroll);	++num_args;
    XtSetArg(args[num_args], XtNscrollHorizontal, &hscroll);	++num_args;
    XtGetValues(textwindow, args, num_args);

    if (international == False) {
	XtSetArg(args[0], XawNtextProperties, &prop);
	XtGetValues(XawTextGetSink(textwindow), args, 1);
    }

    if (flist.pixmap) {
	XtSetArg(args[0], XtNleftBitmap, None);
	XtSetArg(args[1], XtNleftBitmap, flist.pixmap);
	if (!auto_fill)
	    XtSetValues(autoFill, &args[0], 1);
	else
	    XtSetValues(autoFill, &args[1], 1);
	switch (wrap_mode) {
	    case XawtextWrapNever:
		XtSetValues(wrapLine, &args[0], 1);
		XtSetValues(wrapWord, &args[0], 1);
		XtSetValues(wrapNever, &args[1], 1);
		break;
	    case XawtextWrapLine:
		XtSetValues(wrapNever, &args[0], 1);
		XtSetValues(wrapWord, &args[0], 1);
		XtSetValues(wrapLine, &args[1], 1);
		break;
	    case XawtextWrapWord:
		XtSetValues(wrapNever, &args[0], 1);
		XtSetValues(wrapLine, &args[0], 1);
		XtSetValues(wrapWord, &args[1], 1);
		break;
	}
	switch (justify) {
	    case XawjustifyLeft:
		XtSetValues(justifyRight, &args[0], 1);
		XtSetValues(justifyCenter, &args[0], 1);
		XtSetValues(justifyFull, &args[0], 1);
		XtSetValues(justifyLeft, &args[1], 1);
		break;
	    case XawjustifyRight:
		XtSetValues(justifyLeft, &args[0], 1);
		XtSetValues(justifyCenter, &args[0], 1);
		XtSetValues(justifyFull, &args[0], 1);
		XtSetValues(justifyRight, &args[1], 1);
		break;
	    case XawjustifyCenter:
		XtSetValues(justifyLeft, &args[0], 1);
		XtSetValues(justifyRight, &args[0], 1);
		XtSetValues(justifyFull, &args[0], 1);
		XtSetValues(justifyCenter, &args[1], 1);
		break;
	    case XawjustifyFull:
		XtSetValues(justifyLeft, &args[0], 1);
		XtSetValues(justifyRight, &args[0], 1);
		XtSetValues(justifyCenter, &args[0], 1);
		XtSetValues(justifyFull, &args[1], 1);
		break;
	}
	if (!vscroll)
	    XtSetValues(scrollVert, &args[0], 1);
	else
	    XtSetValues(scrollVert, &args[1], 1);
	if (!hscroll)
	    XtSetValues(scrollHoriz, &args[0], 1);
	else
	    XtSetValues(scrollHoriz, &args[1], 1);

	if (international == False) {
	    if (prop == NULL) {
		XtSetValues(modeNone, &args[1], 1);
		XtSetValues(modeC, &args[0], 1);
	    }
	    else if (prop == property_list[C_MODE].properties) {
		XtSetValues(modeNone, &args[0], 1);
		XtSetValues(modeC, &args[1], 1);
	    }
	}
    }
    if (!auto_fill) {
	XtSetSensitive(wrapNever, True);
	XtSetSensitive(wrapLine, True);
	XtSetSensitive(wrapWord, True);

	XtSetSensitive(justifyLeft, False);
	XtSetSensitive(justifyRight, False);
	XtSetSensitive(justifyCenter, False);
	XtSetSensitive(justifyFull, False);
	XtSetSensitive(breakColumns, False);
    }
    else {
	XtSetSensitive(wrapNever, False);
	XtSetSensitive(wrapLine, False);
	XtSetSensitive(wrapWord, False);

	XtSetSensitive(justifyLeft, left < right);
	XtSetSensitive(justifyRight, left < right);
	XtSetSensitive(justifyCenter, left < right);
	XtSetSensitive(justifyFull, left < right);
	XtSetSensitive(breakColumns, True);
    }
}

/*ARGSUSED*/
static void
EditCallback(Widget sme, XtPointer client_data, XtPointer call_data)
{
    Arg args[1];
    Boolean auto_fill;
    XawTextScrollMode scroll;

    switch ((long)client_data) {
	case WRAP_NEVER:
	    XtSetArg(args[0], XtNwrap, XawtextWrapNever);
	    break;
	case WRAP_LINE:
	    XtSetArg(args[0], XtNwrap, XawtextWrapLine);
	    break;
	case WRAP_WORD:
	    XtSetArg(args[0], XtNwrap, XawtextWrapWord);
	    break;
	case AUTO_FILL:
	    XtSetArg(args[0], XtNautoFill, &auto_fill);
	    XtGetValues(textwindow, args, 1);
	    XtSetArg(args[0], XtNautoFill, !auto_fill);
	    break;
	case JUST_LEFT:
	    XtSetArg(args[0], XtNjustifyMode, XawjustifyLeft);
	    break;
	case JUST_RIGHT:
	    XtSetArg(args[0], XtNjustifyMode, XawjustifyRight);
	    break;
	case JUST_CENTER:
	    XtSetArg(args[0], XtNjustifyMode, XawjustifyCenter);
	    break;
	case JUST_FULL:
	    XtSetArg(args[0], XtNjustifyMode, XawjustifyFull);
	    break;
	case SCROLL_VERT:
	    XtSetArg(args[0], XtNscrollVertical, &scroll);
	    XtGetValues(textwindow, args, 1);
	    XtSetArg(args[0], XtNscrollVertical, scroll == XawtextScrollNever ?
		     XawtextScrollAlways : XawtextScrollNever);
	    break;
	case SCROLL_HORIZ:
	    XtSetArg(args[0], XtNscrollHorizontal, &scroll);
	    XtGetValues(textwindow, args, 1);
	    XtSetArg(args[0], XtNscrollHorizontal, scroll == XawtextScrollNever ?
		     XawtextScrollAlways : XawtextScrollNever);
	    break;
    }

    XtSetValues(textwindow, args, 1);
}

static void
CreateColumnsShell(void)
{
    Atom delete_window;
    Widget form, ok, cancel;

    if (columns_shell)
	return;

    XtAppAddActions(XtWidgetToApplicationContext(topwindow),
		    actions, XtNumber(actions));

    columns_shell	= XtCreatePopupShell("columns", transientShellWidgetClass,
					     topwindow, NULL, 0);
    form		= XtCreateManagedWidget("form", formWidgetClass,
						columns_shell, NULL, 0);
    XtCreateManagedWidget("leftLabel", labelWidgetClass, form, NULL, 0);
    left_text		= XtVaCreateManagedWidget("left", asciiTextWidgetClass,
						  form, XtNeditType, XawtextEdit,
						  NULL, 0);
    XtCreateManagedWidget("rightLabel", labelWidgetClass, form, NULL, 0);
    right_text		= XtVaCreateManagedWidget("right", asciiTextWidgetClass,
						  form, XtNeditType, XawtextEdit,
						  NULL, 0);
    ok			= XtCreateManagedWidget("ok", commandWidgetClass,
						form, NULL, 0);
    XtAddCallback(ok, XtNcallback, ProcessColumnsCallback, (XtPointer)True);
    cancel		= XtCreateManagedWidget("cancel", commandWidgetClass,
						form, NULL, 0);
    XtAddCallback(cancel, XtNcallback, ProcessColumnsCallback, (XtPointer)False);

    XtRealizeWidget(columns_shell);
    delete_window = XInternAtom(XtDisplay(columns_shell), "WM_DELETE_WINDOW", False);
    XSetWMProtocols(XtDisplay(columns_shell), XtWindow(columns_shell), &delete_window, 1);

    XtSetKeyboardFocus(columns_shell, left_text);
}

/*ARGSUSED*/
static void
PopupColumnsCallback(Widget w, XtPointer client_data, XtPointer call_data)
{
    Arg args[3];
    char sleft[6], sright[6];
    short left, right;
    Dimension width, height, b_width;
    Window r, c;
    int x, y, wx, wy, max_x, max_y;
    unsigned mask;

    CreateColumnsShell();

    XQueryPointer(XtDisplay(columns_shell), XtWindow(columns_shell),
		  &r, &c, &x, &y, &wx, &wy, &mask);

    XtSetArg(args[0], XtNwidth, &width);
    XtSetArg(args[1], XtNheight, &height);
    XtSetArg(args[2], XtNborderWidth, &b_width);
    XtGetValues(columns_shell, args, 3);

    width += b_width << 1;
    height += b_width << 1;

    x -= (Position)(width >> 1);
    if (x < 0)
	x = 0;
    if (x > (max_x = (Position)(XtScreen(columns_shell)->width - width)))
	x = max_x;

    y -= (Position)(height >> 1);
    if (y < 0)
	y = 0;
    if (y > (max_y = (Position)(XtScreen(columns_shell)->height - height)))
	y = max_y;

    XtSetArg(args[0], XtNx, x);
    XtSetArg(args[1], XtNy, y);
    XtSetValues(columns_shell, args, 2);

    XtSetArg(args[0], XtNleftColumn, &left);
    XtSetArg(args[1], XtNrightColumn, &right);
    XtGetValues(textwindow, args, 2);
    XmuSnprintf(sleft, sizeof(sleft), "%d", left);
    XmuSnprintf(sright, sizeof(sright), "%d", right);
    XtSetArg(args[0], XtNstring, sleft);
    XtSetValues(left_text, args, 1);
    XtSetArg(args[0], XtNstring, sright);
    XtSetValues(right_text, args, 1);
    XtPopup(columns_shell, XtGrabExclusive);
}

/*ARGSUSED*/
static void
ProcessColumnsCallback(Widget w, XtPointer client_data, XtPointer call_data)
{
    if (client_data) {
	Arg args[2];
	char *left, *right;
	short leftc, rightc;

	left = GetString(left_text);
	right = GetString(right_text);

	leftc = atoi(left);
	rightc = atoi(right);
	XtSetArg(args[0], XtNleftColumn, leftc);
	XtSetArg(args[1], XtNrightColumn, rightc);

	XtSetValues(textwindow, args, 2);
    }

    XtPopdown(columns_shell);
}

/*ARGSUSED*/
static void
SetColumns(Widget w, XEvent *event, String *params, Cardinal *num_params)
{
    Bool ok = False;

    if (*num_params &&
	(params[0][0] == 'o' || params[0][0] == 'O'))
	ok = True;

    ProcessColumnsCallback(w, (XtPointer)(long)ok, NULL);
}

/*ARGSUSED*/
static void
ChangeField(Widget w, XEvent *event, String *params, Cardinal *num_params)
{
    Widget focus = XtGetKeyboardFocusWidget(columns_shell);

    XtSetKeyboardFocus(columns_shell,
		       focus == left_text ? right_text : left_text);
}

/*ARGSUSED*/
static void
ModeCallback(Widget sme, XtPointer client_data, XtPointer call_data)
{
    DoSetTextProperties(FindTextSource(XawTextGetSource(textwindow), NULL),
			(property_info*)client_data);
}

void
SetTextProperties(xedit_flist_item *item, Bool force)
{
    int i, j;
    char *ext = strrchr(item->name, '.');
    property_info *info = NULL;

    if (!ext || !*ext) {
	DoSetTextProperties(item, NULL);
	return;
    }

    ++ext;

    for (i = 0; i < sizeof(property_list) / sizeof(property_list[0]); i++) {
	info = &property_list[i];
	for (j = 0; j < info->num_extensions; j++)
	    if (strcmp(info->extensions[j], ext) == 0)
		break;
	if (j < info->num_extensions)
	    break;
    }

    if (i >= sizeof(property_list) / sizeof(property_list[0]) ||
	(!force && info->automatic == False))
	info = NULL;
    DoSetTextProperties(item, info);
}

void
UpdateTextProperties(void)
{
    Arg args[4];
    Cardinal num_args;
    xedit_flist_item *item;
    XawTextPropertyList *prop;
    Widget text, source, sink;
    int i;

    /* save default information */
    if (fonts[0] == NULL) {
	for (i = 0; i < 3; i++) {
	    num_args = 0;
	    XtSetArg(args[num_args], XtNfont, &fonts[i]);	      ++num_args;
	    XtSetArg(args[num_args], XtNforeground, &foregrounds[i]); ++num_args;
	    XtSetArg(args[num_args], XtNbackground, &backgrounds[i]); ++num_args;
	    XtGetValues(XawTextGetSink(texts[i]), args, num_args);
	}
    }

    for (i = 0; i < 3; i++) {
	text = texts[i];
	source = XawTextGetSource(text);
	sink = XawTextGetSink(text);
	item = FindTextSource(source, NULL);

	XtSetArg(args[0], XawNtextProperties, &prop);
	XtGetValues(sink, args, 1);

	if (item == NULL || prop == item->properties)
	    continue;

	XtSetArg(args[0], XawNtextProperties, item->properties);
	num_args = 1;
	if (item->properties == NULL) {
	    XtSetArg(args[num_args], XtNfont, fonts[i]);	     ++num_args;
	    XtSetArg(args[num_args], XtNforeground, foregrounds[i]); ++num_args;
	    XtSetArg(args[num_args], XtNbackground, backgrounds[i]); ++num_args;
	}
	XtSetValues(sink, args, num_args);

	_XawTextBuildLineTable((TextWidget)text,
			       XawTextTopPosition(text), True);
	XawTextDisplay(text);
    }
}

static void
DoSetTextProperties(xedit_flist_item *item, property_info *info)
{
    XawTextPropertyList *prop;
    Widget source;
    Arg args[1];
    int idx, i;

    for (idx = 0; idx < 3; idx++)
	if (texts[idx] == textwindow)
	    break;

    source = item->source;

    XtSetArg(args[0], XawNtextProperties, &prop);
    XtGetValues(XawTextGetSink(texts[idx]), args, 1);

    XawTextSourceClearEntities(source, 0,
			       XawTextSourceScan(source, 0, XawstAll,
						 XawsdRight, 1, True));

    if (prop) {
	for (i = 0; i < sizeof(property_list) / sizeof(property_list[0]); i++)
	    if (property_list[i].properties == prop) {
		(property_list[i].UnsetMode)(source);
		break;
	    }
    }

    item->properties = info ? info->properties : NULL;

    XtSetArg(args[0], XawNtextProperties, item->properties);
    XtSetValues(XawTextGetSink(textwindow), args, 1);
    if (info)
	(info->SetMode)(source);
    XtSetArg(args[0], XawNtextProperties, prop);
    XtSetValues(XawTextGetSink(textwindow), args, 1);

    UpdateTextProperties();
}
