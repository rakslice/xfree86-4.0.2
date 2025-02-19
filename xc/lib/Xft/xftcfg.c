/*
 * $XFree86: xc/lib/Xft/xftcfg.c,v 1.7 2000/12/17 09:11:37 keithp Exp $
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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "xftint.h"

static char	*XftConfigDefaultDirs[] = {
    "/usr/X11R6/lib/X11/fonts/Type1",
    0
};

char		**XftConfigDirs = XftConfigDefaultDirs;
static int	XftConfigNdirs;

static XftSubst	*XftSubsts;
/* #define XFT_DEBUG_EDIT */

Bool
XftConfigAddDir (char *d)
{
    char    **dirs;
    char    *dir;
    char    *h;

    if (*d == '~')
    {
	h = getenv ("HOME");
	if (!h)
	    return False;
	dir = (char *) malloc (strlen (h) + strlen (d));
	strcpy (dir, h);
	strcat (dir, d+1);
    }
    else
    {
	dir = (char *) malloc (strlen (d) + 1);
	if (!dir)
	    return False;
	strcpy (dir, d);
    }
    dirs = (char **) malloc ((XftConfigNdirs + 2) * sizeof (char *));
    if (!dirs)
    {
	free (dir);
	return False;
    }
    if (XftConfigNdirs)
    {
	memcpy (dirs, XftConfigDirs, XftConfigNdirs * sizeof (char *));
    }
    dirs[XftConfigNdirs] = dir;
    XftConfigNdirs++;
    dirs[XftConfigNdirs] = 0;
    if (XftConfigDirs != XftConfigDefaultDirs)
	free (XftConfigDirs);
    XftConfigDirs = dirs;
    return True;
}

static int XftSubstsMaxObjects;

Bool
XftConfigAddEdit (XftTest *test, XftEdit *edit)
{
    XftSubst	*subst, **prev;
    XftTest	*t;
    int		num;

    subst = (XftSubst *) malloc (sizeof (XftSubst));
    if (!subst)
	return False;
    for (prev = &XftSubsts; *prev; prev = &(*prev)->next);
    *prev = subst;
    subst->next = 0;
    subst->test = test;
    subst->edit = edit;
#ifdef XFT_DEBUG_EDIT
    printf ("Add Subst ");
    XftSubstPrint (subst);
#endif
    num = 0;
    for (t = test; t; t = t->next)
	num++;
    if (XftSubstsMaxObjects < num)
	XftSubstsMaxObjects = num;
    return True;
}

typedef struct _XftSubState {
    XftPatternElt   *elt;
    XftValueList    *value;
} XftSubState;

static XftValue
_XftConfigPromote (XftValue v)
{
    if (v.type == XftTypeInteger)
    {
	v.type = XftTypeDouble;
	v.u.d = (double) v.u.i;
    }
    return v;
}

Bool
_XftConfigCompareValue (XftValue    m,
			XftOp	    op,
			XftValue    v)
{
    Bool    ret;
    
    if (m.type == XftTypeVoid)
	return True;
    m = _XftConfigPromote (m);
    v = _XftConfigPromote (v);
    if (m.type == v.type) 
    {
	ret = False;
	switch (m.type) {
	case XftTypeDouble:
	    switch (op) {
	    case XftOpEqual:    
		ret = m.u.d == v.u.d;
		break;
	    case XftOpNotEqual:    
		ret = m.u.d != v.u.d;
		break;
	    case XftOpLess:    
		ret = m.u.d < v.u.d;
		break;
	    case XftOpLessEqual:    
		ret = m.u.d <= v.u.d;
		break;
	    case XftOpMore:    
		ret = m.u.d > v.u.d;
		break;
	    case XftOpMoreEqual:    
		ret = m.u.d >= v.u.d;
		break;
	    default:
		break;
	    }
	    break;
	case XftTypeBool:
	    switch (op) {
	    case XftOpEqual:    
		ret = m.u.b == v.u.b;
		break;
	    case XftOpNotEqual:    
		ret = m.u.b != v.u.b;
		break;
	    default:
		break;
	    }
	    break;
	case XftTypeString:
	    switch (op) {
	    case XftOpEqual:    
		ret = _XftStrCmpIgnoreCase (m.u.s, v.u.s) == 0;
		break;
	    case XftOpNotEqual:    
		ret = _XftStrCmpIgnoreCase (m.u.s, v.u.s) != 0;
		break;
	    default:
		break;
	    }
	    break;
	default:
	    break;
	}
    }
    else
    {
	if (op == XftOpNotEqual)
	    ret = True;
	else
	    ret = False;
    }
    return ret;
}

static XftValueList *
_XftConfigMatchValueList (XftTest	*t,
			  XftValueList  *v)
{
    XftValueList    *ret = 0;
    
    for (; v; v = v->next)
    {
	if (_XftConfigCompareValue (v->value, t->op, t->value))
	{
	    if (!ret)
		ret = v;
	}
	else
	{
	    if (t->qual == XftQualAll)
	    {
		ret = 0;
		break;
	    }
	}
    }
    return ret;
}

static XftValue
_XftConfigEvaluate (XftPattern *p, XftExpr *e)
{
    XftValue	v, vl, vr;
    XftResult	r;
    
    switch (e->op) {
    case XftOpInteger:
	v.type = XftTypeInteger;
	v.u.i = e->u.ival;
	break;
    case XftOpDouble:
	v.type = XftTypeDouble;
	v.u.d = e->u.dval;
	break;
    case XftOpString:
	v.type = XftTypeString;
	v.u.s = e->u.sval;
	break;
    case XftOpBool:
	v.type = XftTypeBool;
	v.u.b = e->u.bval;
	break;
    case XftOpField:
	r = XftPatternGet (p, e->u.field, 0, &v);
	if (r != XftResultMatch)
	    v.type = XftTypeVoid;
	break;
    case XftOpQuest:
	vl = _XftConfigEvaluate (p, e->u.tree.left);
	if (vl.type == XftTypeBool)
	{
	    if (vl.u.b)
		v = _XftConfigEvaluate (p, e->u.tree.right->u.tree.left);
	    else
		v = _XftConfigEvaluate (p, e->u.tree.right->u.tree.right);
	}
	else
	    v.type = XftTypeVoid;
	break;
    case XftOpOr:
    case XftOpAnd:
    case XftOpEqual:
    case XftOpNotEqual:
    case XftOpLess:
    case XftOpLessEqual:
    case XftOpMore:
    case XftOpMoreEqual:
    case XftOpPlus:
    case XftOpMinus:
    case XftOpTimes:
    case XftOpDivide:
	vl = _XftConfigEvaluate (p, e->u.tree.left);
	vr = _XftConfigEvaluate (p, e->u.tree.right);
	vl = _XftConfigPromote (vl);
	vr = _XftConfigPromote (vr);
	if (vl.type == vr.type)
	{
	    switch (vl.type) {
	    case XftTypeDouble:
		switch (e->op) {
		case XftOpPlus:	   
		    v.type = XftTypeDouble;
		    v.u.d = vl.u.d + vr.u.d; 
		    break;
		case XftOpMinus:
		    v.type = XftTypeDouble;
		    v.u.d = vl.u.d - vr.u.d; 
		    break;
		case XftOpTimes:
		    v.type = XftTypeDouble;
		    v.u.d = vl.u.d * vr.u.d; 
		    break;
		case XftOpDivide:
		    v.type = XftTypeDouble;
		    v.u.d = vl.u.d / vr.u.d; 
		    break;
		case XftOpEqual:    
		    v.type = XftTypeBool; 
		    v.u.b = vl.u.d == vr.u.d;
		    break;
		case XftOpNotEqual:    
		    v.type = XftTypeBool; 
		    v.u.b = vl.u.d != vr.u.d;
		    break;
		case XftOpLess:    
		    v.type = XftTypeBool; 
		    v.u.b = vl.u.d < vr.u.d;
		    break;
		case XftOpLessEqual:    
		    v.type = XftTypeBool; 
		    v.u.b = vl.u.d <= vr.u.d;
		    break;
		case XftOpMore:    
		    v.type = XftTypeBool; 
		    v.u.b = vl.u.d > vr.u.d;
		    break;
		case XftOpMoreEqual:    
		    v.type = XftTypeBool; 
		    v.u.b = vl.u.d >= vr.u.d;
		    break;
		default:
		    v.type = XftTypeVoid; 
		    break;
		}
		if (v.type == XftTypeDouble &&
		    v.u.d == (double) (int) v.u.d)
		{
		    v.type = XftTypeInteger;
		    v.u.i = (int) v.u.d;
		}
		break;
	    case XftTypeBool:
		switch (e->op) {
		case XftOpOr:
		    v.type = XftTypeBool;
		    v.u.b = vl.u.b || vr.u.b;
		    break;
		case XftOpAnd:
		    v.type = XftTypeBool;
		    v.u.b = vl.u.b && vr.u.b;
		    break;
		case XftOpEqual:
		    v.type = XftTypeBool;
		    v.u.b = vl.u.b == vr.u.b;
		    break;
		case XftOpNotEqual:
		    v.type = XftTypeBool;
		    v.u.b = vl.u.b != vr.u.b;
		    break;
		default:
		    v.type = XftTypeVoid; 
		    break;
		}
		break;
	    case XftTypeString:
		switch (e->op) {
		case XftOpEqual:
		    v.type = XftTypeBool;
		    v.u.b = _XftStrCmpIgnoreCase (vl.u.s, vr.u.s) == 0;
		    break;
		case XftOpNotEqual:
		    v.type = XftTypeBool;
		    v.u.b = _XftStrCmpIgnoreCase (vl.u.s, vr.u.s) != 0;
		    break;
		case XftOpPlus:
		    v.type = XftTypeString;
		    v.u.s = malloc (strlen (vl.u.s) + strlen (vr.u.s) + 1);
		    if (v.u.s)
		    {
			strcpy (v.u.s, vl.u.s);
			strcat (v.u.s, vr.u.s);
		    }
		    else
			v.type = XftTypeVoid;
		    break;
		default:
		    v.type = XftTypeVoid;
		    break;
		}
	    default:
		v.type = XftTypeVoid;
		break;
	    }
	}
	else
	    v.type = XftTypeVoid;
	break;
    case XftOpNot:
	vl = _XftConfigEvaluate (p, e->u.tree.left);
	switch (vl.type) {
	case XftTypeBool:
	    v.type = XftTypeBool;
	    v.u.b = !vl.u.b;
	    break;
	default:
	    v.type = XftTypeVoid;
	    break;
	}
	break;
    default:
	v.type = XftTypeVoid;
	break;
    }
    return v;
}

static Bool
_XftConfigAdd (XftValueList  **head,
	      XftValueList  *position,
	      Bool	    append,
	      XftValue	    value)
{
    XftValueList    *new, **prev;
    
    new = (XftValueList *) malloc (sizeof (XftValueList));
    if (!new)
	goto bail0;
    
    if (value.type == XftTypeString)
    {
	value.u.s = _XftSaveString (value.u.s);
	if (!value.u.s)
	    goto bail1;
	
    }
    new->value = value;
    new->next = 0;

    if (append)
    {
	prev = &position->next;
    }
    else
    {
	for (prev = head; *prev; prev = &(*prev)->next)
	{
	    if (*prev == position)
		break;
	}
#ifdef XFT_DEBUG
	if (!*prev)
	    printf ("position not on list\n");
#endif
    }

#ifdef XFT_DEBUG_EDIT
    printf ("%s list before ", append ? "Append" : "Prepend");
    XftValueListPrint (*head);
    printf ("\n");
#endif
    
    new->next = *prev;
    *prev = new;
    
#ifdef XFT_DEBUG_EDIT
    printf ("%s list after ", append ? "Append" : "Prepend");
    XftValueListPrint (*head);
    printf ("\n");
#endif
    
    return True;
    
bail1:
    free (new);
bail0:
    return False;
}

static void
_XftConfigDel (XftValueList	**head,
	      XftValueList	*position)
{
    XftValueList    **prev;

    for (prev = head; *prev; prev = &(*prev)->next)
    {
	if (*prev == position)
	{
	    *prev = position->next;
	    position->next = 0;
	    XftValueListDestroy (position);
	    break;
	}
    }
}

Bool
XftConfigSubstitute (XftPattern *p)
{
    XftSubst	    *s;
    XftSubState	    *st;
    int		    i;
    XftTest	    *t;
    XftEdit	    *e;
    XftValue	    v;

    st = (XftSubState *) malloc (XftSubstsMaxObjects * sizeof (XftSubState));
    if (!st && XftSubstsMaxObjects)
	return False;

#ifdef XFT_DEBUG_EDIT
    printf ("XftConfigSubstitute ");
    XftPatternPrint (p);
#endif
    for (s = XftSubsts; s; s = s->next)
    {
	for (t = s->test, i = 0; t; t = t->next, i++)
	{
#ifdef XFT_DEBUG_EDIT
	    printf ("XftConfigSubstitute test ");
	    XftTestPrint (t);
#endif
	    st[i].elt = XftPatternFind (p, t->field, False);
	    if (!st[i].elt)
	    {
		if (t->qual == XftQualAll)
		    continue;
		else
		    break;
	    }
	    st[i].value = _XftConfigMatchValueList (t, st[i].elt->values);
	    if (!st[i].value)
		break;
	}
	if (t)
	{
#ifdef XFT_DEBUG_EDIT
	    printf ("No match\n");
#endif
	    continue;
	}
#ifdef XFT_DEBUG_EDIT
	printf ("Substitute ");
	XftSubstPrint (s);
#endif
	for (e = s->edit; e; e = e->next)
	{
	    v = _XftConfigEvaluate (p, e->expr);
	    if (v.type == XftTypeVoid)
		continue;
	    for (t = s->test, i = 0; t; t = t->next, i++)
		if (!_XftStrCmpIgnoreCase (t->field, e->field))
		    break;
	    switch (e->op) {
	    case XftOpAssign:
		if (t)
		{
		    _XftConfigAdd (&st[i].elt->values, st[i].value, True, v);
		    _XftConfigDel (&st[i].elt->values, st[i].value);
		}
		else
		{
		    XftPatternDel (p, e->field);
		    XftPatternAdd (p, e->field, v, True);
		}
		break;
	    case XftOpPrepend:
		if (t)
		    _XftConfigAdd (&st[i].elt->values, st[i].value, False, v);
		else
		    XftPatternAdd (p, e->field, v, False);
		break;
	    case XftOpAppend:
		if (t)
		    _XftConfigAdd (&st[i].elt->values, st[i].value, True, v);
		else
		    XftPatternAdd (p, e->field, v, True);
		break;
	    default:
		break;
	    }
	}
#ifdef XFT_DEBUG_EDIT
	printf ("XftConfigSubstitute edit");
	XftPatternPrint (p);
#endif
    }
    free (st);
#ifdef XFT_DEBUG_EDIT
    printf ("XftConfigSubstitute done");
    XftPatternPrint (p);
#endif
    return True;
}
