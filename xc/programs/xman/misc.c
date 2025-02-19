/* $XConsortium: misc.c,v 1.31 94/12/16 21:36:53 gildea Exp $ */
/*

Copyright (c) 1987, 1988  X Consortium

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of the X Consortium shall
not be used in advertising or otherwise to promote the sale, use or
other dealings in this Software without prior written authorization
from the X Consortium.

*/
/* $XFree86: xc/programs/xman/misc.c,v 1.4 2000/06/13 23:15:53 dawes Exp $ */

/*
 * xman - X window system manual page display program.
 * Author:    Chris D. Peterson, MIT Project Athena
 * Created:   October 27, 1987
 */

#include "globals.h"
#include "vendor.h"
#include <X11/Xos.h> 		/* sys/types.h and unistd.h included in here */
#include <sys/stat.h>
#include <errno.h>
#include <X11/Xaw/Dialog.h>
#include <X11/Shell.h>

static FILE * Uncompress(ManpageGlobals * man_globals, char * filename);
static Boolean UncompressNamed(ManpageGlobals * man_globals, char * filename, char * output);
static Boolean UncompressUnformatted(ManpageGlobals * man_globals, char * entry, char * filename);

#if defined(ISC) || defined(SCO)
static char *uncompress_format = NULL;
static char *uncompress_formats[] =
      {  UNCOMPRESS_FORMAT_1,
         UNCOMPRESS_FORMAT_2,
         UNCOMPRESS_FORMAT_3
      };
#endif

/*	Function Name: PopupWarning
 *	Description: This function pops upa warning message.
 *	Arguments: string - the specific warning string.
 *	Returns: none
 */

extern Widget top;
static Widget warnShell, warnDialog;

static void 
PopdownWarning(Widget w, XtPointer client, XtPointer call)
{
  XtPopdown((Widget)client);
}

void
PopupWarning(ManpageGlobals * man_globals, char * string)
{
  int n;
  Arg wargs[3];
  Dimension topX, topY;
  char buffer[BUFSIZ];
  Widget positionto;
  Boolean hasPosition;

  sprintf( buffer, "Xman Warning: %s", string);
  hasPosition = FALSE;
  if (man_globals->This_Manpage)
    positionto = man_globals->This_Manpage;
  else
    positionto = top;
  if (top)
  {
    n=0;
    XtSetArg(wargs[n], XtNx, &topX); n++;
    XtSetArg(wargs[n], XtNy, &topY); n++;
    XtGetValues(top, wargs, n);
    hasPosition = TRUE;
  }

  if (man_globals != NULL) 
    ChangeLabel(man_globals->label, buffer);
  if (man_globals->label == NULL) {
    n=0;
    if (hasPosition)
    {
      XtSetArg(wargs[n], XtNx, topX); n++;
      XtSetArg(wargs[n], XtNy, topY); n++;
    }
    XtSetArg(wargs[n], XtNtransientFor, top); n++;
    warnShell = XtCreatePopupShell("warnShell", transientShellWidgetClass, 
				   initial_widget, wargs, n);
    XtSetArg(wargs[0], XtNlabel, buffer); 
    warnDialog = XtCreateManagedWidget("warnDialog", dialogWidgetClass, 
				       warnShell, wargs, 1);
    XawDialogAddButton(warnDialog, "dismiss", PopdownWarning, 
		       (XtPointer)warnShell);
    XtRealizeWidget(warnShell);
    Popup(warnShell, XtGrabNone);
  }
}

/*	Function Name: PrintError
 *	Description: This Function prints an error message and exits.
 *	Arguments: string - the specific message.
 *	Returns: none. - exits tho.
 */

void
PrintError(char * string)
{
  fprintf(stderr,"Xman Error: %s\n",string);
  exit(1);
}

/*	Function Name: OpenFile
 *	Description: Assignes a file to the manpage.
 *	Arguments: man_globals - global structure.
 *                 file        - the file pointer.
 *	Returns: none
 */

void
OpenFile(ManpageGlobals * man_globals, FILE * file)
{
  Arg arglist[1];
  Cardinal num_args = 0;

  XtSetArg(arglist[num_args], XtNfile, file); num_args++;
  XtSetValues(man_globals->manpagewidgets.manpage, arglist, num_args);
}


/*	Function Name: FindManualFile
 *	Description: Opens the manual page file given the entry information.
 *	Arguments: man_globals - the globals info for this manpage.
 *                 section_num - section number of the man page.
 *                 entry_num   - entry number of the man page.
 *	Returns: fp - the file pointer
 *
 * NOTES:
 *
 * If there is a uncompressed section it will look there for uncompresed 
 * manual pages first and then for individually comressed file in the 
 * uncompressed section.
 * 
 * If there is a compressed directory then it will also look there for 
 * the manual pages.
 *
 * If both of these fail then it will attempt to format the manual page.
 */

FILE *
FindManualFile(ManpageGlobals * man_globals, int section_num, int entry_num)
{
  FILE * file;
  char path[BUFSIZ], page[BUFSIZ], section[BUFSIZ], *temp;
  char filename[BUFSIZ];
  char * entry = manual[section_num].entries[entry_num];
  int len_cat = strlen(CAT);
#if defined(ISC) || defined(SCO)
  int i;
#endif

  temp = CreateManpageName(entry, 0, 0);
  sprintf(man_globals->manpage_title, "The current manual page is: %s.", temp);
  XtFree(temp);

  ParseEntry(entry, path, section, page);

/*
 * Look for uncompressed files first.
 */
#if defined(__OpenBSD__) || defined(__NetBSD__)
  /* look in machine subdir first */
  sprintf(filename, "%s/%s%s/%s/%s", path, CAT, 
	  section + len_cat, MACHINE, page);
  if ( (file = fopen(filename,"r")) != NULL)
    return(file);
#endif
  
  sprintf(filename, "%s/%s%s/%s", path, CAT, section + len_cat, page);
  if ( (file = fopen(filename,"r")) != NULL)
    return(file);

/*
 * Then for compressed files in an uncompressed directory.
 */

#if !defined(ISC) && !defined(SCO)
#if defined(__OpenBSD__) || defined(__NetBSD__)
  /* look in machine subdir first */
  sprintf(filename, "%s/%s%s/%s/%s.%s", path, CAT, 
	  section + len_cat, MACHINE, page, COMPRESSION_EXTENSION);
  if ( (file = Uncompress(man_globals, filename)) != NULL)
    return(file);
#endif
  sprintf(filename, "%s/%s%s/%s.%s", path, CAT, 
	  section + len_cat, page, COMPRESSION_EXTENSION);
  if ( (file = Uncompress(man_globals, filename)) != NULL) 
    return(file);
#ifdef GZIP_EXTENSION
  else {
#if defined(__OpenBSD__) || defined(__NetBSD__)
      /* look in machine subdir first */
      sprintf(filename, "%s/%s%s/%s/%s.%s", path, CAT, 
	      section + len_cat, MACHINE, page, GZIP_EXTENSION);
      if ( (file = Uncompress(man_globals, filename)) != NULL)
	  return(file);
#endif
    sprintf(filename, "%s/%s%s/%s.%s", path, CAT,
	    section + len_cat, page, GZIP_EXTENSION);
    if ( (file = Uncompress(man_globals, filename)) != NULL) 
      return(file);
  }
#endif
#else
  for(i = 0; i < strlen(COMPRESSION_EXTENSIONS); i++) {
      sprintf(filename, "%s/%s%s/%s.%c", path, CAT,
            section + len_cat, page, COMPRESSION_EXTENSIONS[i]);
      uncompress_format = uncompress_formats[i];
#ifdef DEBUG
      printf("Trying .%c ...\n", COMPRESSION_EXTENSIONS[i]);
#endif
      if ( (file = Uncompress(man_globals, filename)) != NULL) 
	return(file);
  }
#endif

/*
 * And lastly files in a compressed directory.
 *
 * The directory is not actually compressed it is just named man#.Z
 * and all files in it are compressed without the .Z extension.
 * HP does it this way (really :-).
 */

  sprintf(filename, "%s/%s%s.%s/%s", path, CAT, section + len_cat,
	  COMPRESSION_EXTENSION, page);
  if ( (file = Uncompress(man_globals, filename)) != NULL)
    return(file);
/*
 * We did not find any preformatted manual pages, try to format it.
 */

  return(Format(man_globals, entry));
}

/*	Function Namecompress
 *	Description: This function will attempt to find a compressed man
 *                   page and uncompress it.
 *	Arguments: man_globals - the psuedo global info.
 *                 filename - name of file to uncompress.
 *	Returns:; a pointer to the file or NULL.
 */

static FILE *
Uncompress(ManpageGlobals * man_globals, char * filename)
{
  char tmp_file[BUFSIZ], error_buf[BUFSIZ];
  FILE * file;

  if ( !UncompressNamed(man_globals, filename, tmp_file) )
    return(NULL);

  else if ((file = fopen(tmp_file, "r")) == NULL) {  
    sprintf(error_buf, "Something went wrong in retrieving the %s",
	    "uncompressed manual page try cleaning up /tmp.");
    PopupWarning(man_globals, error_buf);
  }

  unlink(tmp_file);		/* remove name in tree, it will remain
				   until we close the fd, however. */
  return(file);
}

/*	Function Name: UncompressNamed
 *	Description: This function will attempt to find a compressed man
 *                   page and uncompress it.
 *	Arguments: man_globals - the psuedo global info.
 *                 filename - name of file to uncompress.
 * RETURNED        output - the file name output (must be an allocated string).
 *	Returns:; TRUE if the file was found.
 */

static Boolean
UncompressNamed(ManpageGlobals * man_globals, char * filename, char * output)
{
  char tmp[BUFSIZ], cmdbuf[BUFSIZ], error_buf[BUFSIZ];
  struct stat junk;

  if (stat(filename, &junk) != 0) { /* Check for existance of the file. */
    if (errno != ENOENT) {
      sprintf(error_buf, "Error while stating file %s, errno = %d",
	      filename, errno);
      PopupWarning(man_globals, error_buf);
    }
    return(FALSE);
  }

/*
 * Using stdin is necessary to fool zcat since we cannot guarentee
 * the .Z extension.
 */

  strcpy(tmp, MANTEMP);		/* get a temp file. */
  (void) mktemp(tmp);
  strcpy(output, tmp);

#ifdef GZIP_EXTENSION
  if (streq(filename + strlen(filename) - strlen(GZIP_EXTENSION),
	    GZIP_EXTENSION))
    sprintf(cmdbuf, GUNZIP_FORMAT, filename, output);
  else
#endif
  sprintf(cmdbuf, UNCOMPRESS_FORMAT, filename, output);
  if(system(cmdbuf) == 0) 	/* execute search. */
    return(TRUE);

  sprintf(error_buf, "Error while uncompressing, command was: %s", cmdbuf);
  PopupWarning(man_globals, error_buf);
  return(FALSE);
}

/*	Function Name: Format
 *	Description: This funtion formats the manual pages and interfaces
 *                   with the user.
 *	Arguments: man_globals - the psuedo globals
 *                 file - the file pointer to use and return
 *                 entry - the current entry struct.
 *                 current_box - The current directory being displayed. 
 *	Returns: none.
 */

/* ARGSUSED */

FILE *
Format(ManpageGlobals * man_globals, char * entry)
{
  FILE * file;
  Widget manpage = man_globals->manpagewidgets.manpage;
  char cmdbuf[BUFSIZ], tmp[BUFSIZ], filename[BUFSIZ], error_buf[BUFSIZ];
  char path[BUFSIZ];
  XEvent event;
  Position x,y;			/* location to pop up the
				   "would you like to save" widget. */

  if ( !UncompressUnformatted(man_globals, entry, filename) ) {
    /* We Really could not find it, this should never happen, yea right. */
    sprintf(error_buf, "Could not open manual page, %s", entry);
    PopupWarning(man_globals, error_buf);
    XtPopdown( XtParent(man_globals->standby) );
    return(NULL);
  }

  if ((file = fopen(filename, "r")) != NULL) {
    char line[BUFSIZ];

    if (fgets(line, sizeof(line), file) != NULL) {
	if (strncmp(line, ".so ", 4) == 0) {
	  line[strlen(line) - 1] = '\0';
	  fclose(file);
	  unlink(filename);
	  if (line[4] != '/') {
	    char *ptr = NULL;

	    strcpy(tmp, entry);
	    if ((ptr = rindex(tmp, '/')) != NULL) {
	      *ptr = '\0';
	      if ((ptr = rindex(tmp, '/')) != NULL)
		ptr[1] = '\0';
	    }
	  }
	  else
	    *tmp = '\0';
	  sprintf(filename, "%s%s", tmp, line + 4);

	  return (Format(man_globals, filename));
	}
    }
    fclose(file);
  }

  Popup(XtParent(man_globals->standby), XtGrabExclusive);
  while ( !XCheckTypedWindowEvent(XtDisplay(man_globals->standby), 
				  XtWindow(man_globals->standby), 
				  Expose, &event) );
  XtDispatchEvent( &event );
  XFlush(XtDisplay(man_globals->standby));

  strcpy(tmp,MANTEMP);		          /* Get a temp file. */
  (void) mktemp(tmp);
  strcpy(man_globals->tempfile, tmp);

  ParseEntry(entry, path, NULL, NULL);

  sprintf(cmdbuf,"cd %s ; %s %s %s > %s %s", path, TBL,
	  filename, FORMAT, man_globals->tempfile, "2> /dev/null");

  if(system(cmdbuf) != 0) {	/* execute search. */
    sprintf(error_buf,
	    "Something went wrong trying to run the command: %s", cmdbuf);
    PopupWarning(man_globals, error_buf);
    file = NULL;
  }
  else {
    if ((file = fopen(man_globals->tempfile,"r")) == NULL) {  
      sprintf(error_buf, "Something went wrong in retrieving the %s",
	      "temp file, try cleaning up /tmp");
      PopupWarning(man_globals, error_buf);
    }
    else {

      XtPopdown( XtParent(man_globals->standby) );
  
      if ( (man_globals->save == NULL) ||
	   (man_globals->manpagewidgets.manpage == NULL) ) 
	unlink(man_globals->tempfile);
      else {
	char * ptr, catdir[BUFSIZ];

	/*
	 * If the catdir is writeable then ask the user if he/she wants to
	 * write the man page to it. 
	 */

	strcpy(catdir, man_globals->save_file);
	if ( (ptr = rindex(catdir, '/')) != NULL) {
	  *ptr = '\0';

	  if ( access(catdir, W_OK) != 0 )
	    unlink(man_globals->tempfile);
	  else {
	    x = (Position) Width(man_globals->manpagewidgets.manpage)/2;
	    y = (Position) Height(man_globals->manpagewidgets.manpage)/2;
	    XtTranslateCoords(manpage, x, y, &x, &y);
	    PositionCenter( man_globals->save, (int) x, (int) y, 0, 0, 0, 0);
	    XtPopup( man_globals->save, XtGrabExclusive);
	  }
	}
	else 
	  unlink(man_globals->tempfile);
      }
    }
  }

  if (man_globals->compress || man_globals->gzip)    /* If the original
							was compressed
				   			then this is a tempory
							file. */
    unlink(filename);
  
  return(file);
}

/*	Function Name: UncompressUnformatted
 *	Description: Finds an uncompressed unformatted manual page.
 *	Arguments: man_globals - the psuedo global structure.
 *                 entry - the manual page entry.
 * RETURNED        filename - location to put the name of the file.
 *	Returns: TRUE if the file was found.
 */

static Boolean
UncompressUnformatted(ManpageGlobals * man_globals, char * entry, char * filename)
{
  char path[BUFSIZ], page[BUFSIZ], section[BUFSIZ], input[BUFSIZ];
  int len_cat = strlen(CAT), len_man = strlen(MAN);

  ParseEntry(entry, path, section, page);

#if defined(__OpenBSD__) || defined(__NetBSD__)
  /* 
   * look for uncomressed file in machine subdir first 
   */
  sprintf(filename, "%s/%s%s/%s/%s", path, MAN, 
	  section + len_cat, MACHINE, page);
  if ( access( filename, R_OK ) == 0 ) {
    man_globals->compress = FALSE;
    man_globals->gzip = FALSE;
    sprintf(man_globals->save_file, "%s/%s%s/%s/%s", path,
	    CAT, section + len_cat, MACHINE, page);
    return(TRUE);
  }
 /*
  * Then for compressed files in an uncompressed directory.
  */
  sprintf(input, "%s.%s", filename, COMPRESSION_EXTENSION);
  if ( UncompressNamed(man_globals, input, filename) ) {
    man_globals->compress = TRUE;
    sprintf(man_globals->save_file, "%s/%s%s/%s.%s", path,
	    CAT, section + len_cat, page, COMPRESSION_EXTENSION);
    return(TRUE);
  }
#ifdef GZIP_EXTENSION
  else {
    sprintf(input, "%s.%s", filename, GZIP_EXTENSION);
    if ( UncompressNamed(man_globals, input, filename) ) {
      man_globals->compress = TRUE;
      man_globals->gzip = TRUE;
      sprintf(man_globals->save_file, "%s/%s%s/%s.%s", path,
	      CAT, section + len_cat, page, GZIP_EXTENSION);
      return(TRUE);
    }
  }
#endif /* GZIP_EXTENSION */
#endif /* __OpenBSD__ || __NetBSD__ */
/*
 * Look for uncompressed file first.
 */

  sprintf(filename, "%s/%s%s/%s", path, MAN, section + len_man, page);
  if ( access( filename, R_OK ) == 0 ) {
    man_globals->compress = FALSE;
    man_globals->gzip = FALSE;
    sprintf(man_globals->save_file, "%s/%s%s/%s", path,
	    CAT, section + len_cat, page);
    return(TRUE);
  }

/*
 * Then for compressed files in an uncompressed directory.
 */

  sprintf(input, "%s.%s", filename, COMPRESSION_EXTENSION);
  if ( UncompressNamed(man_globals, input, filename) ) {
    man_globals->compress = TRUE;
    sprintf(man_globals->save_file, "%s/%s%s/%s.%s", path,
	    CAT, section + len_cat, page, COMPRESSION_EXTENSION);
    return(TRUE);
  }
#ifdef GZIP_EXTENSION
  else {
    sprintf(input, "%s.%s", filename, GZIP_EXTENSION);
    if ( UncompressNamed(man_globals, input, filename) ) {
      man_globals->compress = TRUE;
      man_globals->gzip = TRUE;
      sprintf(man_globals->save_file, "%s/%s%s/%s.%s", path,
	      CAT, section + len_cat, page, GZIP_EXTENSION);
      return(TRUE);
    }
  }
#endif
/*
 * And lastly files in a compressed directory.
 */

  sprintf(input, "%s/%s%s.%s/%s", path, 
	  MAN, section + len_man, COMPRESSION_EXTENSION, page);
  if ( UncompressNamed(man_globals, input, filename) ) {
    man_globals->compress = TRUE;
    sprintf(man_globals->save_file, "%s/%s%s.%s/%s", path, 
	    CAT, section + len_cat, COMPRESSION_EXTENSION, page);
    return(TRUE);
  }
  return(FALSE);
}
  
/*	Function Name: AddCursor
 *	Description: This function adds the cursor to the window.
 *	Arguments: w - the widget to add the cursor to.
 *                 cursor - the cursor to add to this widget.
 *	Returns: none
 */

void
AddCursor(Widget w, Cursor cursor)
{
  XColor colors[2];
  Arg args[10];
  Cardinal num_args = 0;
  Colormap c_map;
  
  if (!XtIsRealized(w)) {
    PopupWarning(NULL, "Widget is not realized, no cursor added.\n");
    return;
  }

  XtSetArg( args[num_args], XtNcolormap, &c_map); num_args++;
  XtGetValues( w, args, num_args);

  colors[0].pixel = resources.cursors.fg_color;
  colors[1].pixel = resources.cursors.bg_color;

  XQueryColors (XtDisplay(w), c_map, colors, 2);
  XRecolorCursor(XtDisplay(w), cursor, colors, colors+1);
  XDefineCursor(XtDisplay(w),XtWindow(w),cursor);
}

/*	Function Name: ChangeLabel
 *	Description: This function changes the label field of the
 *                   given widget to the string in str.
 *	Arguments: w - the widget.
 *                 str - the string to change the label to.
 *	Returns: none
 */

void
ChangeLabel(Widget w, char * str)
{
  Arg arglist[3];		/* An argument list. */

  if (w == NULL) return;

  XtSetArg(arglist[0], XtNlabel, str);

/* shouldn't really have to do this. */
  XtSetArg(arglist[1], XtNwidth, 0);
  XtSetArg(arglist[2], XtNheight, 0);

  XtSetValues(w, arglist, (Cardinal) 1);
}

/*
 * In an ideal world this would be part of the XToolkit, and I would not
 * have to do it, but such is life sometimes.  Perhaps in X11R3.
 */

/*	Function Name: PositionCenter
 *	Description: This fuction positions the given widgets center
 *                   in the following location.
 *	Arguments: widget - the widget widget to postion
 *                 x,y - The location for the center of the widget
 *                 above - number of pixels above center to locate this widget
 *                 left - number of pixels left of center to locate this widget
 *                 h_space, v_space - how close to get to the edges of the
 *                                    parent window.
 *	Returns: none
 *      Note:  This should only be used with a popup widget that has override
 *             redirect set.
 */

void
PositionCenter(Widget widget, int x, int y, int above, int left, int v_space, int h_space)
{
  Arg wargs[2];
  int x_temp,y_temp;		/* location of the new window. */
  int parent_height,parent_width; /* Height and width of the parent widget or
				   the root window if it has no parent. */

  x_temp = x - left - Width(widget) / 2 + BorderWidth(widget);
  y_temp = y - above -  Height(widget) / 2 + BorderWidth(widget);

  parent_height = HeightOfScreen(XtScreen(widget));
  parent_width = WidthOfScreen(XtScreen(widget));

/*
 * Check to make sure that all edges are within the viewable part of the
 * root window, and if not then force them to be.
 */

  if (x_temp < h_space) 
    x_temp = v_space;
  if (y_temp < v_space)
    (y_temp = 2);

  if ( y_temp + Height(widget) + v_space > parent_height )
      y_temp = parent_height - Height(widget) - v_space; 

  if ( x_temp + Width(widget) + h_space > parent_width )
      x_temp = parent_width - Width(widget) - h_space; 

  XtSetArg(wargs[0], XtNx, x_temp); 
  XtSetArg(wargs[1], XtNy, y_temp); 
  XtSetValues(widget, wargs, 2);
}  

/*	Function Name: ParseEntry(entry, path, sect, page)
 *	Description: Parses the manual pages entry filenames.
 *	Arguments: str - the full path name.
 *                 path - the path name.      RETURNED
 *                 sect - the section name.   RETURNED
 *                 page - the page name.      RETURNED
 *	Returns: none.
 */

void
ParseEntry(char *entry, char *path, char *sect, char *page) 
{
  char *c, temp[BUFSIZ];

  strcpy(temp, entry);

  c = rindex(temp, '/');
  if (c == NULL) 
    PrintError("index failure in ParseEntry.");
  *c++ = '\0';
  if (page != NULL)
    strcpy(page, c);

  c = rindex(temp, '/');
  if (c == NULL) 
    PrintError("index failure in ParseEntry.");
  *c++ = '\0';
#if defined(__OpenBSD__) || defined(__NetBSD__)
  /* Skip machine subdirectory if present */
  if (strcmp(c, MACHINE) == 0) {
      c = rindex(temp, '/');
      if (c == NULL) 
	  PrintError("index failure in ParseEntry.");
      *c++ = '\0';
  }
#endif 
  if (sect != NULL)
    strcpy(sect, c);

  if (path != NULL)
    strcpy(path, temp);
}

/*      Function Name: GetGlobals
 *      Description: Gets the psuedo globals associated with the
 *                   manpage associated with this widget.
 *      Arguments: w - a widget in the manpage.
 *      Returns: the psuedo globals.
 *      Notes: initial_widget is a globals variable.
 *             manglobals_context is a global variable.
 */

ManpageGlobals *
GetGlobals(Widget w)
{
  Widget temp;
  caddr_t data;

  while ( (temp = XtParent(w)) != initial_widget && (temp != NULL))
    w = temp;

  if (temp == NULL) 
    XtAppError(XtWidgetToApplicationContext(w), 
	       "Xman: Could not locate widget in tree, exiting");

  if (XFindContext(XtDisplay(w), XtWindow(w),
		   manglobals_context, &data) != XCSUCCESS)
    XtAppError(XtWidgetToApplicationContext(w), 
	       "Xman: Could not find global data, exiting");

  return( (ManpageGlobals *) data);
}
  
/*      Function Name: SaveGlobals
 *      Description: Saves the psuedo globals on the widget passed
 *                   to this function, although GetGlobals assumes that
 *                   the data is associated with the popup child of topBox.
 *      Arguments: w - the widget to associate the data with.
 *                 globals - data to associate with this widget.
 *      Returns: none.
 *      Notes: WIDGET MUST BE REALIZED.
 *             manglobals_context is a global variable.
 */

void
SaveGlobals(Widget w, ManpageGlobals * globals)
{
  if (XSaveContext(XtDisplay(w), XtWindow(w), manglobals_context,
		   (caddr_t) globals) != XCSUCCESS)
    XtAppError(XtWidgetToApplicationContext(w), 
	       "Xman: Could not save global data, are you out of memory?");
}

/*      Function Name: RemoveGlobals
 *      Description: Removes the psuedo globals from the widget passed
 *                   to this function.
 *      Arguments: w - the widget to remove the data from.
 *      Returns: none.
 *      Notes: WIDGET MUST BE REALIZED.
 *             manglobals_context is a global variable.
 */

void
RemoveGlobals(Widget w)
{
  if (XDeleteContext(XtDisplay(w), XtWindow(w), 
		     manglobals_context) != XCSUCCESS)
    XtAppError(XtWidgetToApplicationContext(w), 
	       "Xman: Could not remove global data?");
}
