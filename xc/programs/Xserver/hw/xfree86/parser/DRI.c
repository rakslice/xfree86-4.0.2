/* DRI.c -- DRI Section in XF86Config file
 * Created: Fri Mar 19 08:40:22 1999 by faith@precisioninsight.com
 * Revised: Thu Jun 17 16:08:05 1999 by faith@precisioninsight.com
 *
 * Copyright 1999 Precision Insight, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 * 
 * $XFree86: xc/programs/Xserver/hw/xfree86/parser/DRI.c,v 1.8 2000/11/30 20:45:33 paulo Exp $
 * 
 */

#include "xf86Parser.h"
#include "xf86tokens.h"
#include "Configint.h"

extern LexRec val;

static xf86ConfigSymTabRec DRITab[] =
{
    {ENDSECTION, "endsection"},
    {GROUP,      "group"},
    {BUFFERS,    "buffers"},
    {MODE,       "mode"},
    {-1,         ""},
};

#define CLEANUP xf86freeBuffersList

XF86ConfBuffersPtr
xf86parseBuffers (void)
{
    parsePrologue (XF86ConfBuffersPtr, XF86ConfBuffersRec)

    if (xf86getToken (NULL) != NUMBER)
	Error ("Buffers count expected", NULL);
    ptr->buf_count = val.num;

    if (xf86getToken (NULL) != NUMBER)
	Error ("Buffers size expected", NULL);
    ptr->buf_size = val.num;

    if ((token = xf86getToken (NULL)) == STRING) {
	ptr->buf_flags = val.str;
    } else {
	ptr->buf_flags = NULL;
	xf86unGetToken (token);
    }
    
#ifdef DEBUG
    printf ("Buffers parsed\n");
#endif

    return ptr;
}

#undef CLEANUP
	
#define CLEANUP xf86freeDRI

XF86ConfDRIPtr
xf86parseDRISection (void)
{
    parsePrologue (XF86ConfDRIPtr, XF86ConfDRIRec);

    /* Zero is a valid value for this. */
    ptr->dri_group = -1;
    while ((token = xf86getToken (DRITab)) != ENDSECTION) {
	switch (token)
	    {
	    case GROUP:
		if ((token = xf86getToken (NULL)) == STRING)
		    ptr->dri_group_name = val.str;
		else if (token == NUMBER)
		    ptr->dri_group = val.num;
		else
		    Error (GROUP_MSG, NULL);
		break;
	    case MODE:
		if (xf86getToken (NULL) != NUMBER)
		    Error (NUMBER_MSG, "Mode");
		ptr->dri_mode = val.num;
		break;
	    case BUFFERS:
		HANDLE_LIST (dri_buffers_lst, xf86parseBuffers,
			     XF86ConfBuffersPtr);
		break;
	    case EOF_TOKEN:
		Error (UNEXPECTED_EOF_MSG, NULL);
		break;
	    default:
		Error (INVALID_KEYWORD_MSG, xf86tokenString ());
		break;
	    }
    }
    
#ifdef DEBUG
    ErrorF("DRI section parsed\n");
#endif
    
    return ptr;
}

#undef CLEANUP

void
xf86printDRISection (FILE * cf, XF86ConfDRIPtr ptr)
{
    XF86ConfBuffersPtr bufs;
    
    if (ptr == NULL)
	return;
    
    if (ptr->dri_group_name)
	fprintf (cf, "\tGroup        \"%s\"\n", ptr->dri_group_name);
    else if (ptr->dri_group >= 0)
	fprintf (cf, "\tGroup        %d\n", ptr->dri_group);
    if (ptr->dri_mode)
	fprintf (cf, "\tMode         0%o\n", ptr->dri_mode);
    for (bufs = ptr->dri_buffers_lst; bufs; bufs = bufs->list.next) {
	fprintf (cf, "\tBuffers      %d %d",
		 bufs->buf_count, bufs->buf_size);
	if (bufs->buf_flags) fprintf (cf, " \"%s\"", bufs->buf_flags);
	fprintf (cf, "\n");
    }
}

void
xf86freeDRI (XF86ConfDRIPtr ptr)
{
    if (ptr == NULL)
	return;
    
    xf86freeBuffersList (ptr->dri_buffers_lst);
    xf86conffree (ptr);
}

void
xf86freeBuffersList (XF86ConfBuffersPtr ptr)
{
    XF86ConfBuffersPtr prev;

    while (ptr) {
	TestFree (ptr->buf_flags);
	prev = ptr;
	ptr  = ptr->list.next;
	xf86conffree (prev);
    }
}

