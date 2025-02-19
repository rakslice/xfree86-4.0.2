/* $XFree86: xc/programs/Xserver/hw/xfree86/parser/Files.c,v 1.8 2000/10/20 14:59:02 alanh Exp $ */
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

#include "X11/Xos.h"
#include "xf86Parser.h"
#include "xf86tokens.h"
#include "Configint.h"

extern LexRec val;

static xf86ConfigSymTabRec FilesTab[] =
{
	{COMMENT, "###"},
	{ENDSECTION, "endsection"},
	{FONTPATH, "fontpath"},
	{RGBPATH, "rgbpath"},
	{MODULEPATH, "modulepath"},
	{LOGFILEPATH, "logfile"},
	{-1, ""},
};

static char *
prependRoot (char *pathname)
{
#ifndef __EMX__
	return pathname;
#else
	/* XXXX caveat: multiple path components in line */
	return (char *) __XOS2RedirRoot (pathname);
#endif
}

#define CLEANUP xf86freeFiles

XF86ConfFilesPtr
xf86parseFilesSection (void)
{
	int i, j;
	int k, l;
	char *str;
	parsePrologue (XF86ConfFilesPtr, XF86ConfFilesRec)

	while ((token = xf86getToken (FilesTab)) != ENDSECTION)
	{
		switch (token)
		{
		case COMMENT:
			if (xf86getToken (NULL) != STRING)
				Error (QUOTE_MSG, "###");
			ptr->file_comment = val.str;
			break;
		case FONTPATH:
			if (xf86getToken (NULL) != STRING)
				Error (QUOTE_MSG, "FontPath");
			j = FALSE;
			str = prependRoot (val.str);
			if (ptr->file_fontpath == NULL)
			{
				ptr->file_fontpath = xf86confmalloc (1);
				ptr->file_fontpath[0] = '\0';
				i = strlen (str) + 1;
			}
			else
			{
				i = strlen (ptr->file_fontpath) + strlen (str) + 1;
				if (ptr->file_fontpath[strlen (ptr->file_fontpath) - 1] != ',')
				{
					i++;
					j = TRUE;
				}
			}
			ptr->file_fontpath =
				xf86confrealloc (ptr->file_fontpath, i);
			if (j)
				strcat (ptr->file_fontpath, ",");

			strcat (ptr->file_fontpath, str);
			xf86conffree (val.str);
			break;
		case RGBPATH:
			if (xf86getToken (NULL) != STRING)
				Error (QUOTE_MSG, "RGBPath");
			ptr->file_rgbpath = val.str;
			break;
		case MODULEPATH:
			if (xf86getToken (NULL) != STRING)
				Error (QUOTE_MSG, "ModulePath");
			l = FALSE;
			str = prependRoot (val.str);
			if (ptr->file_modulepath == NULL)
			{
				ptr->file_modulepath = xf86confmalloc (1);
				ptr->file_modulepath[0] = '\0';
				k = strlen (str) + 1;
			}
			else
			{
				k = strlen (ptr->file_modulepath) + strlen (str) + 1;
				if (ptr->file_modulepath[strlen (ptr->file_modulepath) - 1] != ',')
				{
					k++;
					l = TRUE;
				}
			}
			ptr->file_modulepath = xf86confrealloc (ptr->file_modulepath, k);
			if (l)
				strcat (ptr->file_modulepath, ",");

			strcat (ptr->file_modulepath, str);
			xf86conffree (val.str);
			break;
		case LOGFILEPATH:
			if (xf86getToken (NULL) != STRING)
				Error (QUOTE_MSG, "LogFile");
			ptr->file_logfile = val.str;
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
	printf ("File section parsed\n");
#endif

	return ptr;
}

#undef CLEANUP

void
xf86printFileSection (FILE * cf, XF86ConfFilesPtr ptr)
{
	char *p, *s;

	if (ptr == NULL)
		return;

	if (ptr->file_comment)
		fprintf (cf, "\t###          \"%s\"\n", ptr->file_comment);
	if (ptr->file_logfile)
		fprintf (cf, "\tLogFile      \"%s\"\n", ptr->file_logfile);
	if (ptr->file_rgbpath)
		fprintf (cf, "\tRgbPath      \"%s\"\n", ptr->file_rgbpath);
	if (ptr->file_modulepath)
	{
		s = ptr->file_modulepath;
		p = index (s, ',');
		while (p)
		{
			*p = '\000';
			fprintf (cf, "\tModulePath   \"%s\"\n", s);
			*p = ',';
			s = p;
			s++;
			p = index (s, ',');
		}
		fprintf (cf, "\tModulePath   \"%s\"\n", s);
	}
	if (ptr->file_fontpath)
	{
		s = ptr->file_fontpath;
		p = index (s, ',');
		while (p)
		{
			*p = '\000';
			fprintf (cf, "\tFontPath     \"%s\"\n", s);
			*p = ',';
			s = p;
			s++;
			p = index (s, ',');
		}
		fprintf (cf, "\tFontPath     \"%s\"\n", s);
	}
}

void
xf86freeFiles (XF86ConfFilesPtr p)
{
	if (p == NULL)
		return;

	TestFree (p->file_logfile);
	TestFree (p->file_rgbpath);
	TestFree (p->file_modulepath);
	TestFree (p->file_fontpath);

	xf86conffree (p);
}
