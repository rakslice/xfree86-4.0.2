/* $TOG: main.c /main/86 1998/03/25 08:17:50 kaleb $ */
/*

Copyright (c) 1993, 1994, 1998 The Open Group

All Rights Reserved.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
THE OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from The Open Group.

*/
/* $XFree86: xc/config/makedepend/main.c,v 3.17 2000/11/14 21:59:20 dawes Exp $ */

#include "def.h"
#ifdef hpux
#define sigvec sigvector
#endif /* hpux */

#ifdef X_POSIX_C_SOURCE
#define _POSIX_C_SOURCE X_POSIX_C_SOURCE
#include <signal.h>
#undef _POSIX_C_SOURCE
#else
#if defined(X_NOT_POSIX) || defined(_POSIX_SOURCE)
#include <signal.h>
#else
#define _POSIX_SOURCE
#include <signal.h>
#undef _POSIX_SOURCE
#endif
#endif

#if NeedVarargsPrototypes
#include <stdarg.h>
#endif

#ifdef MINIX
#define USE_CHMOD	1
#endif

#ifdef DEBUG
int	_debugmask;
#endif

char *ProgramName;

char	*directives[] = {
	"if",
	"ifdef",
	"ifndef",
	"else",
	"endif",
	"define",
	"undef",
	"include",
	"line",
	"pragma",
	"error",
	"ident",
	"sccs",
	"elif",
	"eject",
	"warning",
	NULL
};

#define MAKEDEPEND
#include "imakemdep.h"	/* from config sources */
#undef MAKEDEPEND

struct	inclist inclist[ MAXFILES ],
		*inclistp = inclist,
		maininclist;

char	*filelist[ MAXFILES ];
char	*includedirs[ MAXDIRS + 1 ];
char	*notdotdot[ MAXDIRS ];
char	*objprefix = "";
char	*objsuffix = OBJSUFFIX;
char	*startat = "# DO NOT DELETE";
int	width = 78;
boolean	append = FALSE;
boolean	printed = FALSE;
boolean	verbose = FALSE;
boolean	show_where_not = FALSE;
boolean warn_multiple = FALSE;	/* Warn on multiple includes of same file */

static void redirect(char *line, char *makefile);
#if !NeedVarargsPrototypes
void fatalerr();
void warning();
void warning1();
#endif

static
#ifdef SIGNALRETURNSINT
int
#else
void
#endif
catch (int sig)
{
	fflush (stdout);
	fatalerr ("got signal %d\n", sig);
}

#if defined(USG) || (defined(i386) && defined(SYSV)) || defined(WIN32) || defined(__EMX__) || defined(Lynx_22)
#define USGISH
#endif

#ifndef USGISH
#ifdef X_NOT_POSIX
#define sigaction sigvec
#define sa_handler sv_handler
#define sa_mask sv_mask
#define sa_flags sv_flags
#endif
struct sigaction sig_act;
#endif /* USGISH */

int
main(int argc, char *argv[])
{
	char	**fp = filelist;
	char	**incp = includedirs;
	char	*p;
	struct inclist	*ip;
	char	*makefile = NULL;
	struct filepointer	*filecontent;
	struct symtab *psymp = predefs;
	char *endmarker = NULL;
	char *defincdir = NULL;
	char **undeflist = NULL;
	int numundefs = 0, i;

	ProgramName = argv[0];

	while (psymp->s_name)
	{
	    define2(psymp->s_name, psymp->s_value, &maininclist);
	    psymp++;
	}
	if (argc == 2 && argv[1][0] == '@') {
	    struct stat ast;
	    int afd;
	    char *args;
	    char **nargv;
	    int nargc;
	    char quotechar = '\0';

	    nargc = 1;
	    if ((afd = open(argv[1]+1, O_RDONLY)) < 0)
		fatalerr("cannot open \"%s\"\n", argv[1]+1);
	    fstat(afd, &ast);
	    args = (char *)malloc(ast.st_size + 1);
	    if ((ast.st_size = read(afd, args, ast.st_size)) < 0)
		fatalerr("failed to read %s\n", argv[1]+1);
	    args[ast.st_size] = '\0';
	    close(afd);
	    for (p = args; *p; p++) {
		if (quotechar) {
		    if (quotechar == '\\' ||
			(*p == quotechar && p[-1] != '\\'))
			quotechar = '\0';
		    continue;
		}
		switch (*p) {
		case '\\':
		case '"':
		case '\'':
		    quotechar = *p;
		    break;
		case ' ':
		case '\n':
		    *p = '\0';
		    if (p > args && p[-1])
			nargc++;
		    break;
		}
	    }
	    if (p[-1])
		nargc++;
	    nargv = (char **)malloc(nargc * sizeof(char *));
	    nargv[0] = argv[0];
	    argc = 1;
	    for (p = args; argc < nargc; p += strlen(p) + 1)
		if (*p) nargv[argc++] = p;
	    argv = nargv;
	}
	for(argc--, argv++; argc; argc--, argv++) {
	    	/* if looking for endmarker then check before parsing */
		if (endmarker && strcmp (endmarker, *argv) == 0) {
		    endmarker = NULL;
		    continue;
		}
		if (**argv != '-') {
			/* treat +thing as an option for C++ */
			if (endmarker && **argv == '+')
				continue;
			*fp++ = argv[0];
			continue;
		}
		switch(argv[0][1]) {
		case '-':
			endmarker = &argv[0][2];
			if (endmarker[0] == '\0') endmarker = "--";
			break;
		case 'D':
			if (argv[0][2] == '\0') {
				argv++;
				argc--;
			}
			for (p=argv[0] + 2; *p ; p++)
				if (*p == '=') {
					*p = ' ';
					break;
				}
			define(argv[0] + 2, &maininclist);
			break;
		case 'I':
			if (incp >= includedirs + MAXDIRS)
			    fatalerr("Too many -I flags.\n");
			*incp++ = argv[0]+2;
			if (**(incp-1) == '\0') {
				*(incp-1) = *(++argv);
				argc--;
			}
			break;
		case 'U':
			/* Undef's override all -D's so save them up */
			numundefs++;
			if (numundefs == 1)
			    undeflist = malloc(sizeof(char *));
			else
			    undeflist = realloc(undeflist,
						numundefs * sizeof(char *));
			if (argv[0][2] == '\0') {
				argv++;
				argc--;
			}
			undeflist[numundefs - 1] = argv[0] + 2;
			break;
		case 'Y':
			defincdir = argv[0]+2;
			break;
		/* do not use if endmarker processing */
		case 'a':
			if (endmarker) break;
			append = TRUE;
			break;
		case 'w':
			if (endmarker) break;
			if (argv[0][2] == '\0') {
				argv++;
				argc--;
				width = atoi(argv[0]);
			} else
				width = atoi(argv[0]+2);
			break;
		case 'o':
			if (endmarker) break;
			if (argv[0][2] == '\0') {
				argv++;
				argc--;
				objsuffix = argv[0];
			} else
				objsuffix = argv[0]+2;
			break;
		case 'p':
			if (endmarker) break;
			if (argv[0][2] == '\0') {
				argv++;
				argc--;
				objprefix = argv[0];
			} else
				objprefix = argv[0]+2;
			break;
		case 'v':
			if (endmarker) break;
			verbose = TRUE;
#ifdef DEBUG
			if (argv[0][2])
				_debugmask = atoi(argv[0]+2);
#endif
			break;
		case 's':
			if (endmarker) break;
			startat = argv[0]+2;
			if (*startat == '\0') {
				startat = *(++argv);
				argc--;
			}
			if (*startat != '#')
				fatalerr("-s flag's value should start %s\n",
					"with '#'.");
			break;
		case 'f':
			if (endmarker) break;
			makefile = argv[0]+2;
			if (*makefile == '\0') {
				makefile = *(++argv);
				argc--;
			}
			break;

		case 'm':
			warn_multiple = TRUE;
			break;
			
		/* Ignore -O, -g so we can just pass ${CFLAGS} to
		   makedepend
		 */
		case 'O':
		case 'g':
			break;
		default:
			if (endmarker) break;
	/*		fatalerr("unknown opt = %s\n", argv[0]); */
			warning("ignoring option %s\n", argv[0]);
		}
	}
	/* Now do the undefs from the command line */
	for (i = 0; i < numundefs; i++)
	    undefine(undeflist[i], &maininclist);
	if (numundefs > 0)
	    free(undeflist);

	if (!defincdir) {
#ifdef PREINCDIR
	    if (incp >= includedirs + MAXDIRS)
		fatalerr("Too many -I flags.\n");
	    *incp++ = PREINCDIR;
#endif
#ifdef __EMX__
	    {
		char *emxinc = getenv("C_INCLUDE_PATH");
		/* can have more than one component */
		if (emxinc) {
		    char *beg, *end;
		    beg= (char*)strdup(emxinc);
		    for (;;) {
			end = (char*)strchr(beg,';');
			if (end) *end = 0;
		    	if (incp >= includedirs + MAXDIRS)
				fatalerr("Too many include dirs\n");
			*incp++ = beg;
			if (!end) break;
			beg = end+1;
		    }
		}
	    }
#else /* !__EMX__, does not use INCLUDEDIR at all */
	    if (incp >= includedirs + MAXDIRS)
		fatalerr("Too many -I flags.\n");
	    *incp++ = INCLUDEDIR;
#endif

#ifdef POSTINCDIR
	    if (incp >= includedirs + MAXDIRS)
		fatalerr("Too many -I flags.\n");
	    *incp++ = POSTINCDIR;
#endif
	} else if (*defincdir) {
	    if (incp >= includedirs + MAXDIRS)
		fatalerr("Too many -I flags.\n");
	    *incp++ = defincdir;
	}

	redirect(startat, makefile);

	/*
	 * catch signals.
	 */
#ifdef USGISH
/*  should really reset SIGINT to SIG_IGN if it was.  */
#ifdef SIGHUP
	signal (SIGHUP, catch);
#endif
	signal (SIGINT, catch);
#ifdef SIGQUIT
	signal (SIGQUIT, catch);
#endif
	signal (SIGILL, catch);
#ifdef SIGBUS
	signal (SIGBUS, catch);
#endif
	signal (SIGSEGV, catch);
#ifdef SIGSYS
	signal (SIGSYS, catch);
#endif
#else
	sig_act.sa_handler = catch;
#if defined(_POSIX_SOURCE) || !defined(X_NOT_POSIX)
	sigemptyset(&sig_act.sa_mask);
	sigaddset(&sig_act.sa_mask, SIGINT);
	sigaddset(&sig_act.sa_mask, SIGQUIT);
#ifdef SIGBUS
	sigaddset(&sig_act.sa_mask, SIGBUS);
#endif
	sigaddset(&sig_act.sa_mask, SIGILL);
	sigaddset(&sig_act.sa_mask, SIGSEGV);
	sigaddset(&sig_act.sa_mask, SIGHUP);
	sigaddset(&sig_act.sa_mask, SIGPIPE);
#ifdef SIGSYS
	sigaddset(&sig_act.sa_mask, SIGSYS);
#endif
#else
	sig_act.sa_mask = ((1<<(SIGINT -1))
			   |(1<<(SIGQUIT-1))
#ifdef SIGBUS
			   |(1<<(SIGBUS-1))
#endif
			   |(1<<(SIGILL-1))
			   |(1<<(SIGSEGV-1))
			   |(1<<(SIGHUP-1))
			   |(1<<(SIGPIPE-1))
#ifdef SIGSYS
			   |(1<<(SIGSYS-1))
#endif
			   );
#endif /* _POSIX_SOURCE */
	sig_act.sa_flags = 0;
	sigaction(SIGHUP, &sig_act, (struct sigaction *)0);
	sigaction(SIGINT, &sig_act, (struct sigaction *)0);
	sigaction(SIGQUIT, &sig_act, (struct sigaction *)0);
	sigaction(SIGILL, &sig_act, (struct sigaction *)0);
#ifdef SIGBUS
	sigaction(SIGBUS, &sig_act, (struct sigaction *)0);
#endif
	sigaction(SIGSEGV, &sig_act, (struct sigaction *)0);
#ifdef SIGSYS
	sigaction(SIGSYS, &sig_act, (struct sigaction *)0);
#endif
#endif /* USGISH */

	/*
	 * now peruse through the list of files.
	 */
	for(fp=filelist; *fp; fp++) {
		filecontent = getfile(*fp);
		ip = newinclude(*fp, (char *)NULL);

		find_includes(filecontent, ip, ip, 0, FALSE);
		freefile(filecontent);
		recursive_pr_include(ip, ip->i_file, base_name(*fp));
		inc_clean();
	}
	if (printed)
		printf("\n");
	return 0;
}

#ifdef __EMX__
/*
 * eliminate \r chars from file
 */
static int 
elim_cr(char *buf, int sz)
{
	int i,wp;
	for (i= wp = 0; i<sz; i++) {
		if (buf[i] != '\r')
			buf[wp++] = buf[i];
	}
	return wp;
}
#endif

struct filepointer *
getfile(char *file)
{
	int	fd;
	struct filepointer	*content;
	struct stat	st;

	content = (struct filepointer *)malloc(sizeof(struct filepointer));
	if ((fd = open(file, O_RDONLY)) < 0) {
		warning("cannot open \"%s\"\n", file);
		content->f_p = content->f_base = content->f_end = (char *)malloc(1);
		*content->f_p = '\0';
		return(content);
	}
	fstat(fd, &st);
	content->f_base = (char *)malloc(st.st_size+1);
	if (content->f_base == NULL)
		fatalerr("cannot allocate mem\n");
	if ((st.st_size = read(fd, content->f_base, st.st_size)) < 0)
		fatalerr("failed to read %s\n", file);
#ifdef __EMX__
	st.st_size = elim_cr(content->f_base,st.st_size);
#endif
	close(fd);
	content->f_len = st.st_size+1;
	content->f_p = content->f_base;
	content->f_end = content->f_base + st.st_size;
	*content->f_end = '\0';
	content->f_line = 0;
	return(content);
}

void
freefile(struct filepointer *fp)
{
	free(fp->f_base);
	free(fp);
}

char *copy(char *str)
{
	char	*p = (char *)malloc(strlen(str) + 1);

	strcpy(p, str);
	return(p);
}

int
match(char *str, char **list)
{
	int	i;

	for (i=0; *list; i++, list++)
		if (strcmp(str, *list) == 0)
			return(i);
	return(-1);
}

/*
 * Get the next line.  We only return lines beginning with '#' since that
 * is all this program is ever interested in.
 */
char *getnextline(struct filepointer *filep)
{
	char	*p,	/* walking pointer */
		*eof,	/* end of file pointer */
		*bol;	/* beginning of line pointer */
	int	lineno;	/* line number */

	p = filep->f_p;
	eof = filep->f_end;
	if (p >= eof)
		return((char *)NULL);
	lineno = filep->f_line;

	for(bol = p--; ++p < eof; ) {
		if (*p == '/' && *(p+1) == '*') { /* consume comments */
			*p++ = ' ', *p++ = ' ';
			while (*p) {
				if (*p == '*' && *(p+1) == '/') {
					*p++ = ' ', *p = ' ';
					break;
				}
				else if (*p == '\n')
					lineno++;
				*p++ = ' ';
			}
			continue;
		}
		else if (*p == '/' && *(p+1) == '/') { /* consume comments */
			*p++ = ' ', *p++ = ' ';
			while (*p && *p != '\n')
				*p++ = ' ';
			if (*p == '\n') {
				lineno++;
				*p++ = ' ';
			}
			continue;
		}
		else if (*p == '\\') {
			if (*(p+1) == '\n') {
				*p = ' ';
				*(p+1) = ' ';
				lineno++;
			}
		}
		else if (*p == '\n') {
			lineno++;
			if (*bol == '#') {
				char *cp;

				*p++ = '\0';
				/* punt lines with just # (yacc generated) */
				for (cp = bol+1; 
				     *cp && (*cp == ' ' || *cp == '\t'); cp++);
				if (*cp) goto done;
			}
			bol = p+1;
		}
	}
	if (*bol != '#')
		bol = NULL;
done:
	filep->f_p = p;
	filep->f_line = lineno;
	return(bol);
}

/*
 * Strip the file name down to what we want to see in the Makefile.
 * It will have objprefix and objsuffix around it.
 */
char *base_name(char *file)
{
	char	*p;

	file = copy(file);
	for(p=file+strlen(file); p>file && *p != '.'; p--) ;

	if (*p == '.')
		*p = '\0';
	return(file);
}

#if defined(USG) && !defined(CRAY) && !defined(SVR4) && !defined(__EMX__) && !defined(clipper) && !defined(__clipper__)
int rename (char *from, char *to)
{
    (void) unlink (to);
    if (link (from, to) == 0) {
	unlink (from);
	return 0;
    } else {
	return -1;
    }
}
#endif /* USGISH */

void
redirect(char *line, char *makefile)
{
	struct stat	st;
	FILE	*fdin, *fdout;
	char	backup[ BUFSIZ ],
		buf[ BUFSIZ ];
	boolean	found = FALSE;
	int	len;

	/*
	 * if makefile is "-" then let it pour onto stdout.
	 */
	if (makefile && *makefile == '-' && *(makefile+1) == '\0') {
		puts(line);
		return;
	}

	/*
	 * use a default makefile is not specified.
	 */
	if (!makefile) {
		if (stat("Makefile", &st) == 0)
			makefile = "Makefile";
		else if (stat("makefile", &st) == 0)
			makefile = "makefile";
		else
			fatalerr("[mM]akefile is not present\n");
	}
	else
	    stat(makefile, &st);
	if ((fdin = fopen(makefile, "r")) == NULL)
		fatalerr("cannot open \"%s\"\n", makefile);
	sprintf(backup, "%s.bak", makefile);
	unlink(backup);
#if defined(WIN32) || defined(__EMX__)
	fclose(fdin);
#endif
	if (rename(makefile, backup) < 0)
		fatalerr("cannot rename %s to %s\n", makefile, backup);
#if defined(WIN32) || defined(__EMX__)
	if ((fdin = fopen(backup, "r")) == NULL)
		fatalerr("cannot open \"%s\"\n", backup);
#endif
	if ((fdout = freopen(makefile, "w", stdout)) == NULL)
		fatalerr("cannot open \"%s\"\n", backup);
	len = strlen(line);
	while (!found && fgets(buf, BUFSIZ, fdin)) {
		if (*buf == '#' && strncmp(line, buf, len) == 0)
			found = TRUE;
		fputs(buf, fdout);
	}
	if (!found) {
		if (verbose)
		warning("Adding new delimiting line \"%s\" and dependencies...\n",
			line);
		puts(line); /* same as fputs(fdout); but with newline */
	} else if (append) {
	    while (fgets(buf, BUFSIZ, fdin)) {
		fputs(buf, fdout);
	    }
	}
	fflush(fdout);
#if defined(USGISH) || defined(_SEQUENT_) || defined(USE_CHMOD)
	chmod(makefile, st.st_mode);
#else
        fchmod(fileno(fdout), st.st_mode);
#endif /* USGISH */
}

void
#if NeedVarargsPrototypes
fatalerr(char *msg, ...)
#else
/*VARARGS*/
fatalerr(msg,x1,x2,x3,x4,x5,x6,x7,x8,x9)
    char *msg;
#endif
{
#if NeedVarargsPrototypes
	va_list args;
#endif
	fprintf(stderr, "%s: error:  ", ProgramName);
#if NeedVarargsPrototypes
	va_start(args, msg);
	vfprintf(stderr, msg, args);
	va_end(args);
#else
	fprintf(stderr, msg,x1,x2,x3,x4,x5,x6,x7,x8,x9);
#endif
	exit (1);
}

void
#if NeedVarargsPrototypes
warning(char *msg, ...)
#else
/*VARARGS0*/
warning(msg,x1,x2,x3,x4,x5,x6,x7,x8,x9)
    char *msg;
#endif
{
#if NeedVarargsPrototypes
	va_list args;
#endif
	fprintf(stderr, "%s: warning:  ", ProgramName);
#if NeedVarargsPrototypes
	va_start(args, msg);
	vfprintf(stderr, msg, args);
	va_end(args);
#else
	fprintf(stderr, msg,x1,x2,x3,x4,x5,x6,x7,x8,x9);
#endif
}

void
#if NeedVarargsPrototypes
warning1(char *msg, ...)
#else
/*VARARGS0*/
warning1(msg,x1,x2,x3,x4,x5,x6,x7,x8,x9)
    char *msg;
#endif
{
#if NeedVarargsPrototypes
	va_list args;
	va_start(args, msg);
	vfprintf(stderr, msg, args);
	va_end(args);
#else
	fprintf(stderr, msg,x1,x2,x3,x4,x5,x6,x7,x8,x9);
#endif
}
