/* $XFree86: xc/programs/Xserver/hw/xfree86/parser/Screen.c,v 1.16 2000/12/01 16:10:01 paulo Exp $ */
/* 
 * 
 * Copyright (c) 1997  Metro Link Incorporated
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
 * THE X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 * 
 * Except as contained in this notice, the name of the Metro Link shall not be
 * used in advertising or otherwise to promote the sale, use or other dealings
 * in this Software without prior written authorization from Metro Link.
 * 
 */

/* View/edit this file with tab stops set to 4 */

#include "xf86Parser.h"
#include "xf86tokens.h"
#include "Configint.h"

extern LexRec val;

static xf86ConfigSymTabRec DisplayTab[] =
{
	{COMMENT, "###"},
	{ENDSUBSECTION, "endsubsection"},
	{MODES, "modes"},
	{VIEWPORT, "viewport"},
	{VIRTUAL, "virtual"},
	{VISUAL, "visual"},
	{BLACK_TOK, "black"},
	{WHITE_TOK, "white"},
	{DEPTH, "depth"},
	{BPP, "fbbpp"},
	{WEIGHT, "weight"},
	{OPTION, "option"},
	{-1, ""},
};

#define CLEANUP xf86freeDisplayList

XF86ConfDisplayPtr
xf86parseDisplaySubSection (void)
{
	parsePrologue (XF86ConfDisplayPtr, XF86ConfDisplayRec)

	ptr->disp_black.red = ptr->disp_black.green = ptr->disp_black.blue = -1;
	ptr->disp_white.red = ptr->disp_white.green = ptr->disp_white.blue = -1;
	while ((token = xf86getToken (DisplayTab)) != ENDSUBSECTION)
	{
		switch (token)
		{
		case COMMENT:
			if (xf86getToken (NULL) != STRING)
				Error (QUOTE_MSG, "###");
			ptr->disp_comment = val.str;
			break;
		case VIEWPORT:
			if (xf86getToken (NULL) != NUMBER)
				Error (VIEWPORT_MSG, NULL);
			ptr->disp_frameX0 = val.num;
			if (xf86getToken (NULL) != NUMBER)
				Error (VIEWPORT_MSG, NULL);
			ptr->disp_frameY0 = val.num;
			break;
		case VIRTUAL:
			if (xf86getToken (NULL) != NUMBER)
				Error (VIRTUAL_MSG, NULL);
			ptr->disp_virtualX = val.num;
			if (xf86getToken (NULL) != NUMBER)
				Error (VIRTUAL_MSG, NULL);
			ptr->disp_virtualY = val.num;
			break;
		case DEPTH:
			if (xf86getToken (NULL) != NUMBER)
				Error (NUMBER_MSG, "Display");
			ptr->disp_depth = val.num;
			break;
		case BPP:
			if (xf86getToken (NULL) != NUMBER)
				Error (NUMBER_MSG, "Display");
			ptr->disp_bpp = val.num;
			break;
		case VISUAL:
			if (xf86getToken (NULL) != STRING)
				Error (QUOTE_MSG, "Display");
			ptr->disp_visual = val.str;
			break;
		case WEIGHT:
			if (xf86getToken (NULL) != NUMBER)
				Error (WEIGHT_MSG, NULL);
			ptr->disp_weight.red = val.num;
			if (xf86getToken (NULL) != NUMBER)
				Error (WEIGHT_MSG, NULL);
			ptr->disp_weight.green = val.num;
			if (xf86getToken (NULL) != NUMBER)
				Error (WEIGHT_MSG, NULL);
			ptr->disp_weight.blue = val.num;
			break;
		case BLACK_TOK:
			if (xf86getToken (NULL) != NUMBER)
				Error (BLACK_MSG, NULL);
			ptr->disp_black.red = val.num;
			if (xf86getToken (NULL) != NUMBER)
				Error (BLACK_MSG, NULL);
			ptr->disp_black.green = val.num;
			if (xf86getToken (NULL) != NUMBER)
				Error (BLACK_MSG, NULL);
			ptr->disp_black.blue = val.num;
			break;
		case WHITE_TOK:
			if (xf86getToken (NULL) != NUMBER)
				Error (WHITE_MSG, NULL);
			ptr->disp_white.red = val.num;
			if (xf86getToken (NULL) != NUMBER)
				Error (WHITE_MSG, NULL);
			ptr->disp_white.green = val.num;
			if (xf86getToken (NULL) != NUMBER)
				Error (WHITE_MSG, NULL);
			ptr->disp_white.blue = val.num;
			break;
		case MODES:
			{
				XF86ModePtr mptr;

				while ((token = xf86getToken (DisplayTab)) == STRING)
				{
					mptr = xf86confcalloc (1, sizeof (XF86ModeRec));
					mptr->mode_name = val.str;
					mptr->list.next = NULL;
					ptr->disp_mode_lst = (XF86ModePtr)
						xf86addListItem ((glp) ptr->disp_mode_lst, (glp) mptr);
				}
				xf86unGetToken (token);
			}
			break;
		case OPTION:
			{
				char *name;
				if ((token = xf86getToken (NULL)) != STRING)
					Error (BAD_OPTION_MSG, NULL);
				name = val.str;
				if ((token = xf86getToken (NULL)) == STRING)
				{
					ptr->disp_option_lst =
					    xf86addNewOption (ptr->disp_option_lst,
							  name, val.str);
				}
				else
				{
					ptr->disp_option_lst =
					    xf86addNewOption (ptr->disp_option_lst,
							  name, NULL);
					xf86unGetToken (token);
				}
			}
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
	printf ("Display subsection parsed\n");
#endif

	return ptr;
}

#undef CLEANUP

static xf86ConfigSymTabRec ScreenTab[] =
{
	{COMMENT, "###"},
	{ENDSECTION, "endsection"},
	{IDENTIFIER, "identifier"},
        {OBSDRIVER, "driver"},
	{MDEVICE, "device"},
	{MONITOR, "monitor"},
	{VIDEOADAPTOR, "videoadaptor"},
	{SCREENNO, "screenno"},
	{SUBSECTION, "subsection"},
	{DEFAULTDEPTH, "defaultcolordepth"},
	{DEFAULTDEPTH, "defaultdepth"},
	{DEFAULTBPP, "defaultbpp"},
	{DEFAULTFBBPP, "defaultfbbpp"},
	{OPTION, "option"},
	{-1, ""},
};

#define CLEANUP xf86freeScreenList
XF86ConfScreenPtr
xf86parseScreenSection (void)
{
	int has_ident = FALSE;
        int has_driver= FALSE;

	parsePrologue (XF86ConfScreenPtr, XF86ConfScreenRec)

		while ((token = xf86getToken (ScreenTab)) != ENDSECTION)
	{
		switch (token)
		{
		case COMMENT:
			if (xf86getToken (NULL) != STRING)
				Error (QUOTE_MSG, "###");
			ptr->scrn_comment = val.str;
			break;
		case IDENTIFIER:
			if (xf86getToken (NULL) != STRING)
				Error (QUOTE_MSG, "Identifier");
			ptr->scrn_identifier = val.str;
                        if (has_ident || has_driver)
                                Error (ONLY_ONE_MSG,"Identifier or Driver");
                        has_ident = TRUE;
			break;
                case OBSDRIVER:
			if (xf86getToken (NULL) != STRING)
				Error (QUOTE_MSG, "Driver");
			ptr->scrn_obso_driver = val.str;
                        if (has_ident || has_driver)
                                Error (ONLY_ONE_MSG,"Identifier or Driver");
                        has_driver = TRUE;
			break;
		case DEFAULTDEPTH:
			if (xf86getToken (NULL) != NUMBER)
				Error (NUMBER_MSG, "DefaultDepth");
			ptr->scrn_defaultdepth = val.num;
			break;
		case DEFAULTBPP:
			if (xf86getToken (NULL) != NUMBER)
				Error (NUMBER_MSG, "DefaultBPP");
			ptr->scrn_defaultbpp = val.num;
			break;
		case DEFAULTFBBPP:
			if (xf86getToken (NULL) != NUMBER)
				Error (NUMBER_MSG, "DefaultFbBPP");
			ptr->scrn_defaultfbbpp = val.num;
			break;
		case MDEVICE:
			if (xf86getToken (NULL) != STRING)
				Error (QUOTE_MSG, "Device");
			ptr->scrn_device_str = val.str;
			break;
		case MONITOR:
			if (xf86getToken (NULL) != STRING)
				Error (QUOTE_MSG, "Monitor");
			ptr->scrn_monitor_str = val.str;
			break;
		case VIDEOADAPTOR:
			{
				XF86ConfAdaptorLinkPtr aptr;

				if (xf86getToken (NULL) != STRING)
					Error (QUOTE_MSG, "VideoAdaptor");

				/* Don't allow duplicates */
				for (aptr = ptr->scrn_adaptor_lst; aptr; 
					aptr = (XF86ConfAdaptorLinkPtr) aptr->list.next)
					if (xf86nameCompare (val.str, aptr->al_adaptor_str) == 0)
						break;

				if (aptr == NULL)
				{
					aptr = xf86confcalloc (1, sizeof (XF86ConfAdaptorLinkRec));
					aptr->list.next = NULL;
					aptr->al_adaptor_str = val.str;
					ptr->scrn_adaptor_lst = (XF86ConfAdaptorLinkPtr)
						xf86addListItem ((glp) ptr->scrn_adaptor_lst, (glp) aptr);
				}
			}
			break;
		case OPTION:
			{
				char *name;
				if ((token = xf86getToken (NULL)) != STRING)
					Error (BAD_OPTION_MSG, NULL);
				name = val.str;
				if ((token = xf86getToken (NULL)) == STRING)
				{
					ptr->scrn_option_lst =
					    xf86addNewOption (ptr->scrn_option_lst,
							  name, val.str);
				}
				else
				{
					ptr->scrn_option_lst =
					    xf86addNewOption (ptr->scrn_option_lst,
							  name, NULL);
					xf86unGetToken (token);
				}
			}
			break;
		case SUBSECTION:
			if (xf86getToken (NULL) != STRING)
				Error (QUOTE_MSG, "SubSection");
			{
				HANDLE_LIST (scrn_display_lst, xf86parseDisplaySubSection,
							 XF86ConfDisplayPtr);
			}
			break;
		case EOF_TOKEN:
			Error (UNEXPECTED_EOF_MSG, NULL);
			break;
		default:
			Error (INVALID_KEYWORD_MSG, xf86tokenString ());
			break;
		}
	}

	if (!has_ident && !has_driver)
		Error (NO_IDENT_MSG, NULL);

#ifdef DEBUG
	printf ("Screen section parsed\n");
#endif

	return ptr;
}

void
xf86printScreenSection (FILE * cf, XF86ConfScreenPtr ptr)
{
	XF86ConfAdaptorLinkPtr aptr;
	XF86ConfDisplayPtr dptr;
	XF86ModePtr mptr;
	XF86OptionPtr optr;

	while (ptr)
	{
		fprintf (cf, "Section \"Screen\"\n");
		if (ptr->scrn_comment)
			fprintf (cf, "\t###        \"%s\"\n", ptr->scrn_comment);
		if (ptr->scrn_identifier)
			fprintf (cf, "\tIdentifier \"%s\"\n", ptr->scrn_identifier);
		if (ptr->scrn_obso_driver)
			fprintf (cf, "\tDriver     \"%s\"\n", ptr->scrn_obso_driver);
		if (ptr->scrn_device_str)
			fprintf (cf, "\tDevice     \"%s\"\n", ptr->scrn_device_str);
		if (ptr->scrn_monitor_str)
			fprintf (cf, "\tMonitor    \"%s\"\n", ptr->scrn_monitor_str);
		if (ptr->scrn_defaultdepth)
			fprintf (cf, "\tDefaultDepth     %d\n",
					 ptr->scrn_defaultdepth);
		if (ptr->scrn_defaultbpp)
			fprintf (cf, "\tDefaultBPP     %d\n",
					 ptr->scrn_defaultbpp);
		if (ptr->scrn_defaultfbbpp)
			fprintf (cf, "\tDefaultFbBPP     %d\n",
					 ptr->scrn_defaultfbbpp);
		for (optr = ptr->scrn_option_lst; optr; optr = optr->list.next)
		{
			fprintf (cf, "\tOption      \"%s\"", optr->opt_name);
			if (optr->opt_val)
				fprintf (cf, " \"%s\"", optr->opt_val);
			fprintf (cf, "\n");
		}
		for (aptr = ptr->scrn_adaptor_lst; aptr; aptr = aptr->list.next)
		{
			fprintf (cf, "\tVideoAdaptor \"%s\"\n", aptr->al_adaptor_str);
		}
		for (dptr = ptr->scrn_display_lst; dptr; dptr = dptr->list.next)
		{
			fprintf (cf, "\tSubSection \"Display\"\n");
			if (dptr->disp_comment)
				fprintf (cf, "\t\t###        %s\n",
						 dptr->disp_comment);
			if (dptr->disp_frameX0 != 0 || dptr->disp_frameY0 != 0)
			{
				fprintf (cf, "\t\tViewport   %d %d\n",
						 dptr->disp_frameX0, dptr->disp_frameY0);
			}
			if (dptr->disp_virtualX != 0 || dptr->disp_virtualY != 0)
			{
				fprintf (cf, "\t\tVirtual   %d %d\n",
						 dptr->disp_virtualX, dptr->disp_virtualY);
			}
			if (dptr->disp_depth)
			{
				fprintf (cf, "\t\tDepth     %d\n", dptr->disp_depth);
			}
			if (dptr->disp_bpp)
			{
				fprintf (cf, "\t\tFbBPP     %d\n", dptr->disp_bpp);
			}
			if (dptr->disp_visual)
			{
				fprintf (cf, "\t\tVisual    \"%s\"\n", dptr->disp_visual);
			}
			if (dptr->disp_weight.red != 0)
			{
				fprintf (cf, "\t\tWeight    %d %d %d\n",
					 dptr->disp_weight.red, dptr->disp_weight.green, dptr->disp_weight.blue);
			}
			if (dptr->disp_black.red != -1)
			{
				fprintf (cf, "\t\tBlack     0x%04x 0x%04x 0x%04x\n",
					  dptr->disp_black.red, dptr->disp_black.green, dptr->disp_black.blue);
			}
			if (dptr->disp_white.red != -1)
			{
				fprintf (cf, "\t\tWhite     0x%04x 0x%04x 0x%04x\n",
					  dptr->disp_white.red, dptr->disp_white.green, dptr->disp_white.blue);
			}
			if (dptr->disp_mode_lst)
			{
				fprintf (cf, "\t\tModes   ");
			}
			for (mptr = dptr->disp_mode_lst; mptr; mptr = mptr->list.next)
			{
				fprintf (cf, " \"%s\"", mptr->mode_name);
			}
			if (dptr->disp_mode_lst)
			{
				fprintf (cf, "\n");
			}
			for (optr = dptr->disp_option_lst; optr; optr = optr->list.next)
			{
				fprintf (cf, "\tOption      \"%s\"", optr->opt_name);
				if (optr->opt_val)
					fprintf (cf, " \"%s\"", optr->opt_val);
				fprintf (cf, "\n");
			}
			fprintf (cf, "\tEndSubSection\n");
		}
		fprintf (cf, "EndSection\n\n");
		ptr = ptr->list.next;
	}

}

void
xf86freeScreenList (XF86ConfScreenPtr ptr)
{
	XF86ConfScreenPtr prev;

	while (ptr)
	{
		TestFree (ptr->scrn_identifier);
		TestFree (ptr->scrn_monitor_str);
		TestFree (ptr->scrn_device_str);
		xf86optionListFree (ptr->scrn_option_lst);
		xf86freeAdaptorLinkList (ptr->scrn_adaptor_lst);
		xf86freeDisplayList (ptr->scrn_display_lst);
		prev = ptr;
		ptr = ptr->list.next;
		xf86conffree (prev);
	}
}

void
xf86freeAdaptorLinkList (XF86ConfAdaptorLinkPtr ptr)
{
	XF86ConfAdaptorLinkPtr prev;

	while (ptr)
	{
		TestFree (ptr->al_adaptor_str);
		prev = ptr;
		ptr = ptr->list.next;
		xf86conffree (prev);
	}
}

void
xf86freeDisplayList (XF86ConfDisplayPtr ptr)
{
	XF86ConfDisplayPtr prev;

	while (ptr)
	{
		xf86freeModeList (ptr->disp_mode_lst);
		xf86optionListFree (ptr->disp_option_lst);
		prev = ptr;
		ptr = ptr->list.next;
		xf86conffree (prev);
	}
}

void
xf86freeModeList (XF86ModePtr ptr)
{
	XF86ModePtr prev;

	while (ptr)
	{
		TestFree (ptr->mode_name);
		prev = ptr;
		ptr = ptr->list.next;
		xf86conffree (prev);
	}
}

int
xf86validateScreen (XF86ConfigPtr p)
{
	XF86ConfScreenPtr screen = p->conf_screen_lst;
	XF86ConfMonitorPtr monitor;
	XF86ConfDevicePtr device;
	XF86ConfAdaptorLinkPtr adaptor;

	if (!screen)
	{
		xf86validationError ("At least one Screen section is required.");
		return (FALSE);
	}

	while (screen)
	{
                if (screen->scrn_obso_driver && !screen->scrn_identifier)
                        screen->scrn_identifier = screen->scrn_obso_driver;
                
                monitor = xf86findMonitor (screen->scrn_monitor_str, p->conf_monitor_lst);
		if (!monitor)
		{
			xf86validationError (UNDEFINED_MONITOR_MSG,
						 screen->scrn_monitor_str, screen->scrn_identifier);
			return (FALSE);
		}
		else
		{
			screen->scrn_monitor = monitor;
			if (!xf86validateMonitor(p, screen))
				return (FALSE);
		}

		device = xf86findDevice (screen->scrn_device_str, p->conf_device_lst);
		if (!device)
		{
			xf86validationError (UNDEFINED_DEVICE_MSG,
						  screen->scrn_device_str, screen->scrn_identifier);
			return (FALSE);
		}
		else
			screen->scrn_device = device;

		adaptor = screen->scrn_adaptor_lst;
		while (adaptor)
		{
			adaptor->al_adaptor = xf86findVideoAdaptor (adaptor->al_adaptor_str, p->conf_videoadaptor_lst);
			if (!adaptor->al_adaptor)
			{
				xf86validationError (UNDEFINED_ADAPTOR_MSG, adaptor->al_adaptor_str, screen->scrn_identifier);
				return (FALSE);
			}
			else if (adaptor->al_adaptor->va_fwdref)
			{
				xf86validationError (ADAPTOR_REF_TWICE_MSG, adaptor->al_adaptor_str,
						     adaptor->al_adaptor->va_fwdref);
				return (FALSE);
			}

			adaptor->al_adaptor->va_fwdref = xf86configStrdup(screen->scrn_identifier);
			adaptor = adaptor->list.next;
		}

		screen = screen->list.next;
	}

	return (TRUE);
}

XF86ConfScreenPtr
xf86findScreen (const char *ident, XF86ConfScreenPtr p)
{
	while (p)
	{
		if (xf86nameCompare (ident, p->scrn_identifier) == 0)
			return (p);

		p = p->list.next;
	}
	return (NULL);
}

XF86ConfDisplayPtr
xf86findDisplay (int depth, XF86ConfDisplayPtr p)
{
	while (p)
	{
		if (depth == p->disp_depth)
			return (p);

		p = p->list.next;
	}
	return (NULL);
}
