/* $XFree86: xc/programs/Xserver/hw/xfree86/parser/scan.c,v 1.14 2000/11/02 19:58:20 anderson Exp $ */
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

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>

#if !defined(X_NOT_POSIX)
#if defined(_POSIX_SOURCE)
#include <limits.h>
#else
#define _POSIX_SOURCE
#include <limits.h>
#undef _POSIX_SOURCE
#endif /* _POSIX_SOURCE */
#endif /* !X_NOT_POSIX */
#if !defined(PATH_MAX)
#if defined(MAXPATHLEN)
#define PATH_MAX MAXPATHLEN
#else
#define PATH_MAX 1024
#endif /* MAXPATHLEN */
#endif /* !PATH_MAX */

#if !defined(MAXHOSTNAMELEN)
#define MAXHOSTNAMELEN 32
#endif /* !MAXHOSTNAMELEN */

#include "Configint.h"
#include "xf86tokens.h"

#define CONFIG_BUF_LEN     1024

static int StringToToken (char *, xf86ConfigSymTabRec *);

static FILE *configFile = NULL;
static int configStart = 0;		/* start of the current token */
static int configPos = 0;		/* current readers position */
static int configLineNo = 0;	/* linenumber */
static char *configBuf, *configRBuf;	/* buffer for lines */
static char *configPath;		/* path to config file */
static char *configSection;		/* name of current section being parsed */
static int pushToken = LOCK_TOKEN;
LexRec val;

#ifdef __EMX__
extern char *__XOS2RedirRoot(char *path);
#endif

/* 
 * xf86strToUL --
 *
 *  A portable, but restricted, version of strtoul().  It only understands
 *  hex, octal, and decimal.  But it's good enough for our needs.
 */
unsigned int
xf86strToUL (char *str)
{
	int base = 10;
	char *p = str;
	unsigned int tot = 0;

	if (*p == '0')
	{
		p++;
		if ((*p == 'x') || (*p == 'X'))
		{
			p++;
			base = 16;
		}
		else
			base = 8;
	}
	while (*p)
	{
		if ((*p >= '0') && (*p <= ((base == 8) ? '7' : '9')))
		{
			tot = tot * base + (*p - '0');
		}
		else if ((base == 16) && (*p >= 'a') && (*p <= 'f'))
		{
			tot = tot * base + 10 + (*p - 'a');
		}
		else if ((base == 16) && (*p >= 'A') && (*p <= 'F'))
		{
			tot = tot * base + 10 + (*p - 'A');
		}
		else
		{
			return (tot);
		}
		p++;
	}
	return (tot);
}

/* 
 * xf86getToken --
 *      Read next Token form the config file. Handle the global variable
 *      pushToken.
 */
int
xf86getToken (xf86ConfigSymTabRec * tab)
{
	int c, i;

	/* 
	 * First check whether pushToken has a different value than LOCK_TOKEN.
	 * In this case rBuf[] contains a valid STRING/TOKEN/NUMBER. But in the
	 * oth * case the next token must be read from the input.
	 */
	if (pushToken == EOF_TOKEN)
		return (EOF_TOKEN);
	else if (pushToken == LOCK_TOKEN)
	{

		c = configBuf[configPos];

		/* 
		 * Get start of next Token. EOF is handled,
		 * whitespaces & comments are* skipped. 
		 */
		do
		{
			if (!c)
			{
				if (fgets (configBuf, CONFIG_BUF_LEN - 1, configFile) == NULL)
				{
					return (pushToken = EOF_TOKEN);
				}
				configLineNo++;
				configStart = configPos = 0;
			}
#ifndef __EMX__
			while (((c = configBuf[configPos++]) == ' ') || (c == '\t') || (c == '\n'));
#else
			while (((c = configBuf[configPos++]) == ' ') || (c == '\t') || (c == '\n')
				   || (c == '\r'));
#endif
			if (c == '#')
				c = '\0';
		}
		while (!c);

		/* GJA -- handle '-' and ','  * Be careful: "-hsync" is a keyword. */
		if ((c == ',') && !isalpha (configBuf[configPos]))
		{
			configStart = configPos;
			return COMMA;
		}
		else if ((c == '-') && !isalpha (configBuf[configPos]))
		{
			configStart = configPos;
			return DASH;
		}

		configStart = configPos;
		/* 
		 * Numbers are returned immediately ...
		 */
		if (isdigit (c))
		{
			int base;

			if (c == '0')
				if ((configBuf[configPos] == 'x') ||
					(configBuf[configPos] == 'X'))
					base = 16;
				else
					base = 8;
			else
				base = 10;

			configRBuf[0] = c;
			i = 1;
			while (isdigit (c = configBuf[configPos++]) ||
				   (c == '.') || (c == 'x') || (c == 'X') ||
				   ((base == 16) && (((c >= 'a') && (c <= 'f')) ||
									 ((c >= 'A') && (c <= 'F')))))
				configRBuf[i++] = c;
			configPos--;		/* GJA -- one too far */
			configRBuf[i] = '\0';
			val.num = xf86strToUL (configRBuf);
			val.realnum = atof (configRBuf);
			return (NUMBER);
		}

		/* 
		 * All Strings START with a \" ...
		 */
		else if (c == '\"')
		{
			i = -1;
			do
			{
				configRBuf[++i] = (c = configBuf[configPos++]);
#ifndef __EMX__
			}
			while ((c != '\"') && (c != '\n') && (c != '\0'));
#else
			}
			while ((c != '\"') && (c != '\n') && (c != '\r') && (c != '\0'));
#endif
			configRBuf[i] = '\0';
			val.str = xf86confmalloc (strlen (configRBuf) + 1);
			strcpy (val.str, configRBuf);	/* private copy ! */
			return (STRING);
		}

		/* 
		 * ... and now we MUST have a valid token.  The search is
		 * handled later along with the pushed tokens.
		 */
		else
		{
			configRBuf[0] = c;
			i = 0;
			do
			{
				configRBuf[++i] = (c = configBuf[configPos++]);;
#ifndef __EMX__
			}
			while ((c != ' ') && (c != '\t') && (c != '\n') && (c != '\0'));
#else
			}
			while ((c != ' ') && (c != '\t') && (c != '\n') && (c != '\r') && (c != '\0'));
#endif
			configRBuf[i] = '\0';
			i = 0;
		}

	}
	else
	{

		/* 
		 * Here we deal with pushed tokens. Reinitialize pushToken again. If
		 * the pushed token was NUMBER || STRING return them again ...
		 */
		int temp = pushToken;
		pushToken = LOCK_TOKEN;

		if (temp == COMMA || temp == DASH)
			return (temp);
		if (temp == NUMBER || temp == STRING)
			return (temp);
	}

	/* 
	 * Joop, at last we have to lookup the token ...
	 */
	if (tab)
	{
		i = 0;
		while (tab[i].token != -1)
			if (xf86nameCompare (configRBuf, tab[i].name) == 0)
				return (tab[i].token);
			else
				i++;
	}

	return (ERROR_TOKEN);		/* Error catcher */
}

void
xf86unGetToken (int token)
{
	pushToken = token;
}

char *
xf86tokenString (void)
{
	return configRBuf;
}

#if 1
int
xf86pathIsAbsolute(const char *path)
{
	if (path && path[0] == '/')
		return 1;
#ifdef __EMX__
	if (path && (path[0] == '\\' || (path[1] == ':')))
		return 0;
#endif
	return 0;
}

/* A path is "safe" if it is relative and if it contains no ".." elements. */
int
xf86pathIsSafe(const char *path)
{
	if (xf86pathIsAbsolute(path))
		return 0;

	/* Compare with ".." */
	if (!strcmp(path, ".."))
		return 0;

	/* Look for leading "../" */
	if (!strncmp(path, "../", 3))
		return 0;

	/* Look for trailing "/.." */
	if ((strlen(path) > 3) && !strcmp(path + strlen(path) - 3, "/.."))
		return 0;

	/* Look for "/../" */
	if (strstr(path, "/../"))
		return 0;

	return 1;
}

/*
 * This function substitutes the following escape sequences:
 *
 *    %A    cmdline argument as an absolute path (must be absolute to match)
 *    %R    cmdline argument as a relative path
 *    %S    cmdline argument as a "safe" path (relative, and no ".." elements)
 *    %X    default config file name ("XF86Config")
 *    %H    hostname
 *    %E    config file environment ($XF86CONFIG) as an absolute path
 *    %F    config file environment ($XF86CONFIG) as a relative path
 *    %G    config file environment ($XF86CONFIG) as a safe path
 *    %D    $HOME
 *    %P    projroot
 *    %M    major version number
 *    %%    %
 *    %&    EMX only: prepend X11ROOT env var
 */

#ifndef XCONFIGFILE
#define XCONFIGFILE	"XF86Config"
#endif
#ifndef PROJECTROOT
#define PROJECTROOT	"/usr/X11R6"
#endif
#ifndef XCONFENV
#define XCONFENV	"XF86CONFIG"
#endif
#ifndef XF86_VERSION_MAJOR
#ifdef XVERSION
#define XF86_VERSION_MAJOR	(XVERSION / 1000)
#else
#define XF86_VERSION_MAJOR	4
#endif
#endif

#define BAIL_OUT		do {									\
							xf86conffree(result);				\
							return NULL;						\
						} while (0)

#define CHECK_LENGTH	do {									\
							if (l > PATH_MAX) {					\
								BAIL_OUT;						\
							}									\
						} while (0)

#define APPEND_STR(s)	do {									\
							if (strlen(s) + l > PATH_MAX) {		\
								BAIL_OUT;						\
							} else {							\
								strcpy(result + l, s);			\
								l += strlen(s);					\
							}									\
						} while (0)

static char *
DoSubstitution(const char *template, const char *cmdline, const char *projroot,
				int *cmdlineUsed, int *envUsed)
{
	char *result;
	int i, l;
	static const char *env = NULL, *home = NULL;
	static char *hostname = NULL;
	static char majorvers[3] = "";
#ifdef __EMX__
	static char *x11root = NULL;
#endif

	if (!template)
		return NULL;

	if (cmdlineUsed)
		*cmdlineUsed = 0;
	if (envUsed)
		*envUsed = 0;

	result = xf86confmalloc(PATH_MAX + 1);
	l = 0;
	for (i = 0; template[i]; i++) {
		if (template[i] != '%') {
			result[l++] = template[i];
			CHECK_LENGTH;
		} else {
			switch (template[++i]) {
			case 'A':
				if (cmdline && xf86pathIsAbsolute(cmdline)) {
					APPEND_STR(cmdline);
					if (cmdlineUsed)
						*cmdlineUsed = 1;
				} else
					BAIL_OUT;
				break;
			case 'R':
				if (cmdline && !xf86pathIsAbsolute(cmdline)) {
					APPEND_STR(cmdline);
					if (cmdlineUsed)
						*cmdlineUsed = 1;
				} else 
					BAIL_OUT;
				break;
			case 'S':
				if (cmdline && xf86pathIsSafe(cmdline)) {
					APPEND_STR(cmdline);
					if (cmdlineUsed)
						*cmdlineUsed = 1;
				} else 
					BAIL_OUT;
				break;
			case 'X':
				APPEND_STR(XCONFIGFILE);
				break;
			case 'H':
				if (!hostname) {
					if ((hostname = xf86confmalloc(MAXHOSTNAMELEN + 1))) {
						if (gethostname(hostname, MAXHOSTNAMELEN) == 0) {
							hostname[MAXHOSTNAMELEN] = '\0';
						} else {
							xf86conffree(hostname);
							hostname = NULL;
						}
					}
				}
				if (hostname)
					APPEND_STR(hostname);
				break;
			case 'E':
				if (!env)
					env = getenv(XCONFENV);
				if (env && xf86pathIsAbsolute(env)) {
					APPEND_STR(env);
					if (envUsed)
						*envUsed = 1;
				} else
					BAIL_OUT;
				break;
			case 'F':
				if (!env)
					env = getenv(XCONFENV);
				if (env && !xf86pathIsAbsolute(env)) {
					APPEND_STR(env);
					if (envUsed)
						*envUsed = 1;
				} else
					BAIL_OUT;
				break;
			case 'G':
				if (!env)
					env = getenv(XCONFENV);
				if (env && xf86pathIsSafe(env)) {
					APPEND_STR(env);
					if (envUsed)
						*envUsed = 1;
				} else
					BAIL_OUT;
				break;
			case 'D':
				if (!home)
					home = getenv("HOME");
				if (home && xf86pathIsAbsolute(home))
					APPEND_STR(home);
				else
					BAIL_OUT;
				break;
			case 'P':
				if (projroot && xf86pathIsAbsolute(projroot))
					APPEND_STR(projroot);
				else
					BAIL_OUT;
				break;
			case 'M':
				if (!majorvers[0]) {
					if (XF86_VERSION_MAJOR < 0 || XF86_VERSION_MAJOR > 99) {
						fprintf(stderr, "XF86_VERSION_MAJOR is out of range\n");
						BAIL_OUT;
					} else
						sprintf(majorvers, "%d", XF86_VERSION_MAJOR);
				}
				APPEND_STR(majorvers);
				break;
			case '%':
				result[l++] = '%';
				CHECK_LENGTH;
				break;
#ifdef __EMX__
			case '&':
				if (!x11root)
					x11root = getenv("X11ROOT");
				if (x11root)
					APPEND_STR(x11root);
				else
					BAIL_OUT;
				break;
#endif
			default:
				fprintf(stderr, "invalid escape %%%c found in path template\n",
						template[i]);
				BAIL_OUT;
				break;
			}
		}
	}
#ifdef DEBUG
	fprintf(stderr, "Converted `%s' to `%s'\n", template, result);
#endif
	return result;
}

/* 
 * xf86openConfigFile --
 *
 * This function take a config file search path (optional), a command-line
 * specified file name (optional) and the ProjectRoot path (optional) and
 * locates and opens a config file based on that information.  If a
 * command-line file name is specified, then this function fails if none
 * of the located files.
 *
 * The return value is a pointer to the actual name of the file that was
 * opened.  When no file is found, the return value is NULL.
 *
 * The escape sequences allowed in the search path are defined above.
 *  
 */

#ifndef DEFAULT_CONF_PATH
#define DEFAULT_CONF_PATH	"/etc/X11/%S," \
							"%P/etc/X11/%S," \
							"/etc/X11/%G," \
							"%P/etc/X11/%G," \
							"/etc/X11/%X-%M," \
							"/etc/X11/%X," \
							"/etc/%X," \
							"%P/etc/X11/%X.%H," \
							"%P/etc/X11/%X-%M," \
							"%P/etc/X11/%X," \
							"%P/lib/X11/%X.%H," \
							"%P/lib/X11/%X-%M," \
							"%P/lib/X11/%X"
#endif

const char *
xf86openConfigFile(const char *path, const char *cmdline, const char *projroot)
{
	char *pathcopy;
	const char *template;
	int cmdlineUsed = 0;

	configFile = NULL;
	configStart = 0;		/* start of the current token */
	configPos = 0;		/* current readers position */
	configLineNo = 0;	/* linenumber */
	pushToken = LOCK_TOKEN;

	if (!path || !path[0])
		path = DEFAULT_CONF_PATH;
	pathcopy = xf86confmalloc(strlen(path) + 1);
	strcpy(pathcopy, path);
	if (!projroot || !projroot[0])
		projroot = PROJECTROOT;

	template = strtok(pathcopy, ",");

	/* First, search for a config file. */
	while (template && !configFile) {
		if ((configPath = DoSubstitution(template, cmdline, projroot,
										 &cmdlineUsed, NULL))) {
			if ((configFile = fopen(configPath, "r")) != 0) {
				if (cmdline && !cmdlineUsed) {
					fclose(configFile);
					configFile = NULL;
				}
			}
		}
		if (configPath && !configFile) {
			xf86conffree(configPath);
			configPath = NULL;
		}
		template = strtok(NULL, ",");
	}
	if (!configFile) {
		return NULL;
	}

	configBuf = xf86confmalloc (CONFIG_BUF_LEN);
	configRBuf = xf86confmalloc (CONFIG_BUF_LEN);
	configBuf[0] = '\0';		/* sanity ... */

	return configPath;
}
#else
/* 
 * xf86openConfigFile --
 *
 * Formerly findConfigFile(). This function take a pointer to a location
 * in which to place the actual name of the file that was opened.
 * This function uses the global character array xf86ConfigFile
 * This function returns the following results.
 *
 *  0   unable to open the config file
 *  1   file opened and ready to read
 *  
 */

int
xf86openConfigFile (char *filename)
{
#define MAXPTRIES   6
	char *home = NULL;
	char *xconfig = NULL;
	char *xwinhome = NULL;
	char *configPaths[MAXPTRIES];
	int pcount = 0, idx;

/* 
 * First open if necessary the config file.
 * If the -xf86config flag was used, use the name supplied there (root only).
 * If $XF86CONFIG is a pathname, use it as the name of the config file (root)
 * If $XF86CONFIG is set but doesn't contain a '/', append it to 'XF86Config'
 *   and search the standard places (root only).
 * If $XF86CONFIG is not set, just search the standard places.
 */
	configFile = NULL;
	configStart = 0;		/* start of the current token */
	configPos = 0;		/* current readers position */
	configLineNo = 0;	/* linenumber */
	pushToken = LOCK_TOKEN;
	while (!configFile)
	{

		/* 
		 * configPaths[0]   is used as a buffer for -xf86config
		 *                  and $XF86CONFIG if it contains a path
		 * configPaths[1...MAXPTRIES-1] is used to store the paths of each of
		 *                  the other attempts
		 */
		for (pcount = idx = 0; idx < MAXPTRIES; idx++)
			configPaths[idx] = NULL;

		/* 
		 * First check if the -xf86config option was used.
		 */
		configPaths[pcount] = xf86confmalloc (PATH_MAX);
		if (xf86ConfigFile[0])
		{
			strcpy (configPaths[pcount], xf86ConfigFile);
			if ((configFile = fopen (configPaths[pcount], "r")) != 0)
				break;
			else
				return 0;
		}
		/* 
		 * Check if XF86CONFIG is set.
		 */
#ifndef __EMX__
		if (getuid () == 0
			&& (xconfig = getenv ("XF86CONFIG")) != 0
			&& strchr (xconfig, '/'))
#else
		/* no root available, and filenames start with drive letter */
		if ((xconfig = getenv ("XF86CONFIG")) != 0
			&& isalpha (xconfig[0])
			&& xconfig[1] == ':')
#endif
		{
			strcpy (configPaths[pcount], xconfig);
			if ((configFile = fopen (configPaths[pcount], "r")) != 0)
				break;
			else
				return 0;
		}

#ifndef __EMX__
		/* 
		 * ~/XF86Config ...
		 */
		if (getuid () == 0 && (home = getenv ("HOME")) != NULL)
		{
			configPaths[++pcount] = xf86confmalloc (PATH_MAX);
			strcpy (configPaths[pcount], home);
			strcat (configPaths[pcount], "/" XCONFIGFILE);
			if (xconfig)
				strcat (configPaths[pcount], xconfig);
			if ((configFile = fopen (configPaths[pcount], "r")) != 0)
				break;
		}

		/* 
		 * /etc/XF86Config
		 */
		configPaths[++pcount] = xf86confmalloc (PATH_MAX);
		strcpy (configPaths[pcount], "/etc/" XCONFIGFILE);
		if (xconfig)
			strcat (configPaths[pcount], xconfig);
		if ((configFile = fopen (configPaths[pcount], "r")) != 0)
			break;

		/* 
		 * $(XCONFIGDIR)/XF86Config.<hostname>
		 */

		configPaths[++pcount] = xf86confmalloc (PATH_MAX);
		if (getuid () == 0 && (xwinhome = getenv ("XWINHOME")) != NULL)
			sprintf (configPaths[pcount], "%s/lib/X11/" XCONFIGFILE, xwinhome);
		else
			strcpy (configPaths[pcount], XCONFIGDIR "/" XCONFIGFILE);
		if (getuid () == 0 && xconfig)
			strcat (configPaths[pcount], xconfig);
		strcat (configPaths[pcount], ".");
#ifdef AMOEBA
		{
			extern char *XServerHostName;

			strcat (configPaths[pcount], XServerHostName);
		}
#else
		gethostname (configPaths[pcount] + strlen (configPaths[pcount]),
					 MAXHOSTNAMELEN);
#endif
		if ((configFile = fopen (configPaths[pcount], "r")) != 0)
			break;
#endif /* !__EMX__  */

		/* 
		 * $(XCONFIGDIR)/XF86Config
		 */
		configPaths[++pcount] = xf86confmalloc (PATH_MAX);
#ifndef __EMX__
		if (getuid () == 0 && xwinhome)
			sprintf (configPaths[pcount], "%s/lib/X11/" XCONFIGFILE, xwinhome);
		else
			strcpy (configPaths[pcount], XCONFIGDIR "/" XCONFIGFILE);
		if (getuid () == 0 && xconfig)
			strcat (configPaths[pcount], xconfig);
#else
		/*
		 * we explicitly forbid numerous config files everywhere for OS/2;
		 * users should consider them lucky to have one in a standard place
		 * and another one with the -xf86config option
		 */
		xwinhome = getenv ("X11ROOT");	/* get drive letter */
		if (!xwinhome) {
			fprintf (stderr,"X11ROOT environment variable not set\n");
			exit(2);
		}
		strcpy (configPaths[pcount], __XOS2RedirRoot ("/XFree86/lib/X11/XConfig"));
#endif

		if ((configFile = fopen (configPaths[pcount], "r")) != 0)
			break;

		return 0;
	}
	configBuf = xf86confmalloc (CONFIG_BUF_LEN);
	configRBuf = xf86confmalloc (CONFIG_BUF_LEN);
	configPath = xf86confmalloc (PATH_MAX);

	strcpy (configPath, configPaths[pcount]);

	if (filename)
		strcpy (filename, configPaths[pcount]);
	for (idx = 0; idx <= pcount; idx++)
		if (configPaths[idx] != NULL)
			xf86conffree (configPaths[idx]);

	configBuf[0] = '\0';		/* sanity ... */

	return 1;
}
#endif

void
xf86closeConfigFile (void)
{
	xf86conffree (configPath);
	xf86conffree (configRBuf);
	xf86conffree (configBuf);

	fclose (configFile);
}

void
xf86parseError (char *format,...)
{
	va_list ap;

#if 0
	fprintf (stderr, "Parse error on line %d of section %s in file %s\n",
			 configLineNo, configSection, configPath);
	fprintf (stderr, "\t");
	va_start (ap, format);
	vfprintf (stderr, format, ap);
	va_end (ap);

	fprintf (stderr, "\n");
#else
	ErrorF ("Parse error on line %d of section %s in file %s\n\t",
		 configLineNo, configSection, configPath);
	va_start (ap, format);
	VErrorF (format, ap);
	va_end (ap);

	ErrorF ("\n");
#endif

}

void
xf86parseWarning (char *format,...)
{
	va_list ap;

#if 0
	fprintf (stderr, "Parse warning on line %d of section %s in file %s\n",
			 configLineNo, configSection, configPath);
	fprintf (stderr, "\t");
	va_start (ap, format);
	vfprintf (stderr, format, ap);
	va_end (ap);

	fprintf (stderr, "\n");
#else
	ErrorF ("Parse warning on line %d of section %s in file %s\n\t",
		 configLineNo, configSection, configPath);
	va_start (ap, format);
	VErrorF (format, ap);
	va_end (ap);

	ErrorF ("\n");
#endif
}

void
xf86validationError (char *format,...)
{
	va_list ap;

#if 0
	fprintf (stderr, "Data incomplete in file %s\n",
			 configPath);
	fprintf (stderr, "\t");
	va_start (ap, format);
	vfprintf (stderr, format, ap);
	va_end (ap);

	fprintf (stderr, "\n");
#else
	ErrorF ("Data incomplete in file %s\n\t", configPath);
	va_start (ap, format);
	VErrorF (format, ap);
	va_end (ap);

	ErrorF ("\n");
#endif
}

void
xf86setSection (char *section)
{
	configSection = section;
}

/* 
 * xf86getToken --
 *  Lookup a string if it is actually a token in disguise.
 */
int
xf86getStringToken (xf86ConfigSymTabRec * tab)
{
	return StringToToken (val.str, tab);
}

static int
StringToToken (char *str, xf86ConfigSymTabRec * tab)
{
	int i;

	for (i = 0; tab[i].token != -1; i++)
	{
		if (!xf86nameCompare (tab[i].name, str))
			return tab[i].token;
	}
	return (ERROR_TOKEN);
}


/* 
 * Compare two names.  The characters '_', ' ', and '\t' are ignored
 * in the comparison.
 */
int
xf86nameCompare (const char *s1, const char *s2)
{
	char c1, c2;

	if (!s1 || *s1 == 0) {
		if (!s2 || *s2 == 0)
			return (0);
		else
			return (1);
		}

	while (*s1 == '_' || *s1 == ' ' || *s1 == '\t')
		s1++;
	while (*s2 == '_' || *s2 == ' ' || *s2 == '\t')
		s2++;
	c1 = (isupper (*s1) ? tolower (*s1) : *s1);
	c2 = (isupper (*s2) ? tolower (*s2) : *s2);
	while (c1 == c2)
	{
		if (c1 == '\0')
			return (0);
		s1++;
		s2++;
		while (*s1 == '_' || *s1 == ' ' || *s1 == '\t')
			s1++;
		while (*s2 == '_' || *s2 == ' ' || *s2 == '\t')
			s2++;
		c1 = (isupper (*s1) ? tolower (*s1) : *s1);
		c2 = (isupper (*s2) ? tolower (*s2) : *s2);
	}
	return (c1 - c2);
}
