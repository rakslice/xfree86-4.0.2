/* $XFree86: xc/programs/Xserver/hw/xfree86/os-support/shared/libc_wrapper.c,v 1.71 2000/12/06 20:39:54 dawes Exp $ */
/*
 * Copyright 1997 by The XFree86 Project, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the names of Orest Zborowski and David Wexelblat 
 * not be used in advertising or publicity pertaining to distribution of 
 * the software without specific, written prior permission.  Orest Zborowski
 * and David Wexelblat make no representations about the suitability of this 
 * software for any purpose.  It is provided "as is" without express or 
 * implied warranty.
 *
 * THE XFREE86 PROJECT, INC. DISCLAIMS ALL WARRANTIES WITH REGARD 
 * TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND 
 * FITNESS, IN NO EVENT SHALL OREST ZBOROWSKI OR DAVID WEXELBLAT BE LIABLE 
 * FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES 
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN 
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF 
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */

#include <X.h>
#include <Xmd.h>
#include <sys/types.h>
#include <sys/stat.h>
#if defined(__bsdi__)
#undef _POSIX_SOURCE
#undef _ANSI_SOURCE
#endif
#include <sys/time.h>
#include <math.h>
#include <stdarg.h>
#include <fcntl.h>
#include "Xfuncproto.h"
#include "os.h"
#include <stdarg.h>
#include <ctype.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <sys/ioctl.h>
#ifdef __EMX__
#define NO_MMAP
#include <sys/param.h>
#endif
#ifdef HAS_SVR3_MMAPDRV
#define NO_MMAP
#ifdef SELF_CONTAINED_WRAPPER
#include <sys/at_ansi.h>
#include <sys/kd.h>
#include <sys/sysmacros.h>
#if !defined(_NEED_SYSI86)
# include <sys/immu.h>
# include <sys/region.h>
#endif
#include <sys/mmap.h>
struct kd_memloc MapDSC;
int mmapFd = -2;
#else
extern struct kd_memloc MapDSC;
extern int mmapFd;
#endif
#endif
#ifndef NO_MMAP
#include <sys/mman.h>
#ifndef MAP_FAILED
#define MAP_FAILED ((caddr_t)-1)
#endif
#endif
#if !defined(ISC)
#include <stdlib.h>
#endif

#ifdef __CYGWIN__
#define vsscanf sscanf
#define vfscanf fscanf
#endif

#define NEED_XF86_TYPES
#define NEED_XF86_PROTOTYPES
#define DONT_DEFINE_WRAPPERS
#include "xf86_ansic.h"

#ifndef SELF_CONTAINED_WRAPPER
#include "xf86.h"
#include "xf86Priv.h"
#define NO_OSLIB_PROTOTYPES
#define XF86_OS_PRIVS
#define HAVE_WRAPPER_DECLS
#include "xf86_OSlib.h"
#else
void xf86WrapperInit(void);
#endif


#ifndef X_NOT_POSIX
#include <dirent.h>
#else
#ifdef SYSV
#include <dirent.h>
#else
#ifdef USG
#include <dirent.h>
#else
#include <sys/dir.h>
#ifndef dirent
#define dirent direct
#endif
#endif
#endif
#endif
typedef struct dirent DIRENTRY;

#ifdef __EMX__
#define _POSIX_SOURCE
#endif
#ifdef ISC202
#include <sys/types.h>
#define WIFEXITED(a)  ((a & 0x00ff) == 0)  /* LSB will be 0 */
#define WEXITSTATUS(a) ((a & 0xff00) >> 8)
#define WIFSIGNALED(a) ((a & 0xff00) == 0) /* MSB will be 0 */
#define WTERMSIG(a) (a & 0x00ff)
#else
#if defined(ISC) && !defined(_POSIX_SOURCE)
#define _POSIX_SOURCE
#include <sys/types.h>
#include <sys/wait.h>
#undef _POSIX_SOURCE
#else
#if defined(MINIX) || defined(AMOEBA) || (defined(ISC) && defined(_POSIX_SOURCE)) || defined(Lynx) || (defined (__alpha__) && defined(linux))
#include <sys/types.h>
#endif
#include <sys/wait.h>
#endif
#endif
#ifdef Lynx
#if !defined(S_IFIFO) && defined(S_IFFIFO)
#define S_IFIFO S_IFFIFO
#endif
#endif

/* For xf86getpagesize() */
#if defined(linux)
#define HAS_SC_PAGESIZE
#define HAS_GETPAGESIZE
#elif defined(CSRG_BASED)
#define HAS_GETPAGESIZE
#elif defined(DGUX)
#define HAS_GETPAGESIZE
#elif defined(sun) && !defined(SVR4)
#define HAS_GETPAGESIZE
#endif
#ifdef XNO_SYSCONF
#undef _SC_PAGESIZE
#endif
#ifdef HAVE_SYSV_IPC
#include <sys/ipc.h>
#include <sys/shm.h>
#endif
#include <setjmp.h>

#if 0
#define SETBUF_RETURNS_INT
#endif

double xf86HUGE_VAL;

#ifndef SELF_CONTAINED_WRAPPERS
extern void xf86DisableIO(void);
#endif

/*
 * This file contains the XFree86 wrappers for libc functions that can be
 * called by loadable modules
 */

double
xf86hypot(double x, double y)
{
	return(hypot(x,y));
}

void
xf86qsort(void *base, xf86size_t nmemb, xf86size_t size,
	  int (*comp)(const void *, const void *))
{
	qsort(base, nmemb, size, comp);
}

/* string functions */

char*
xf86strcat(char* dest, const char* src)
{
	return(strcat(dest,src));
}

char*
xf86strchr(const char* s, int c)
{
	return strchr(s,c);
}

int
xf86strcmp(const char* s1, const char* s2)
{
	return strcmp(s1,s2);
}

/* Just like the BSD version.  It assumes that tolower() is ANSI-compliant */
int
xf86strcasecmp(const char* s1, const char* s2)
{
	const unsigned char *us1 = (const unsigned char *)s1;
	const unsigned char *us2 = (const unsigned char *)s2;

	while (tolower(*us1) == tolower(*us2++))
		if (*us1++ == '\0')
			return 0;

	return tolower(*us1) - tolower(*--us2);
}

char*
xf86strcpy(char* dest, const char* src)
{
	return strcpy(dest,src);
}

xf86size_t
xf86strcspn(const char* s1, const char* s2)
{
	return (xf86size_t)strcspn(s1,s2);
}

xf86size_t
xf86strlen(const char* s)
{
	return (xf86size_t)strlen(s);
}

char*
xf86strncat(char* dest, const char* src, xf86size_t n)
{
	return strncat(dest,src,(size_t)n);
}

int
xf86strncmp(const char* s1, const char* s2, xf86size_t n)
{
	return strncmp(s1,s2,(size_t)n);
}

/* Just like the BSD version.  It assumes that tolower() is ANSI-compliant */
int
xf86strncasecmp(const char* s1, const char* s2, xf86size_t n)
{
	if (n != 0) {
		const unsigned char *us1 = (const unsigned char *)s1;
		const unsigned char *us2 = (const unsigned char *)s2;

		do {
			if (tolower(*us1) != tolower(*us2++))
				return tolower(*us1) - tolower(*--us2);
			if (*us1++ == '\0')
				break;
		} while (--n != 0);
	}
	return 0;
}

char*
xf86strncpy(char* dest, const char* src, xf86size_t n)
{
	return strncpy(dest,src,(size_t)n);
}

char*
xf86strpbrk(const char* s1, const char* s2)
{
	return strpbrk(s1,s2);
}

char*
xf86strrchr(const char* s, int c)
{
	return strrchr(s,c);
}

xf86size_t
xf86strspn(const char* s1, const char* s2)
{
	return strspn(s1,s2);
}

char*
xf86strstr(const char* s1, const char* s2)
{
	return strstr(s1,s2);
}

char*
xf86strtok(char* s1, const char* s2)
{
	return strtok(s1,s2);
}

char*
xf86strdup(const char* s)
{
	return xstrdup(s);
}

int
xf86sprintf(char *s, const char *format, ...)
{
    int ret;
    va_list args;
    va_start(args, format);
    ret = vsprintf(s, format, args);
    va_end(args);
    return ret;
}

int
xf86snprintf(char *s, xf86size_t len, const char *format, ...)
{
    int ret;
    va_list args;
    va_start(args, format);
    ret = vsnprintf(s, (size_t)len, format, args);
    va_end(args);
    return ret;
}

void
xf86bzero(void* s, unsigned int n)
{
    memset(s, 0, n);
}
  
#ifdef HAVE_VSSCANF
int
xf86sscanf(char *s, const char *format, ...)
#else
int
xf86sscanf(char *s, const char *format, char *a0, char *a1, char *a2,
	   char *a3, char *a4, char *a5, char *a6, char *a7, char *a8,
	   char *a9) /* limit of ten args */
#endif
{
#ifdef HAVE_VSSCANF
	int ret;
	va_list args;
	va_start(args, format);

	ret = vsscanf(s,format,args);
	va_end(args);
	return ret;
#else
	return sscanf(s, format, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9);
#endif
}
  
/* Basic I/O */

int xf86errno;

/* XXX This is not complete */

static int
xfToOsOpenFlags(int xfflags)
{
    int flags = 0;

    /* XXX This assumes O_RDONLY is 0 */
    if (xfflags & XF86_O_WRONLY)
	flags |= O_WRONLY;
    if (xfflags & XF86_O_RDWR)
	flags |= O_RDWR;
    if (xfflags & XF86_O_CREAT)
	flags |= O_CREAT;

    return flags;
}

int 
xf86open(const char *path, int flags, ...)
{
    int fd;
    va_list ap;

    va_start(ap, flags);
    flags = xfToOsOpenFlags(flags);
    if (flags & O_CREAT) {
	mode_t mode = va_arg(ap, mode_t);
	fd = open(path, flags, mode);
    } else {
	fd = open(path, flags);
    }
    va_end(ap);
    xf86errno = xf86GetErrno();
    return fd;
}

int
xf86close(int fd)
{
    int status = close(fd);

    xf86errno = xf86GetErrno();
    return status;
}

long
xf86lseek(int fd, long offset, int whence)
{
	switch (whence) {
	case XF86_SEEK_SET:
		whence = SEEK_SET;
		break;
	case XF86_SEEK_CUR:
		whence = SEEK_CUR;
		break;
	case XF86_SEEK_END:
		whence = SEEK_END;
		break;
	}
	return (long)lseek(fd, (off_t)offset, whence);
}

int
xf86ioctl(int fd, unsigned long request, pointer argp)
{
    int status = ioctl(fd, request, argp);

    xf86errno = xf86GetErrno();
    return status;
}

xf86ssize_t
xf86read(int fd, void *buf, xf86size_t nbytes)
{
    xf86ssize_t n = read(fd, buf, (size_t)nbytes);

    xf86errno = xf86GetErrno();
    return n;
}

xf86ssize_t
xf86write(int fd, const void *buf, xf86size_t nbytes)
{
    xf86ssize_t n = write(fd, buf, (size_t)nbytes);

    xf86errno = xf86GetErrno();
    return n;
}

void*
xf86mmap(void *start, xf86size_t length, int prot,
	 int flags, int fd, xf86size_t /* off_t */ offset)
{
#ifndef NO_MMAP
    int p=0, f=0;
    void *rc;

    if (flags & XF86_MAP_FIXED)		f |= MAP_FIXED;
    if (flags & XF86_MAP_SHARED)	f |= MAP_SHARED;
    if (flags & XF86_MAP_PRIVATE)	f |= MAP_PRIVATE;
    if (prot  & XF86_PROT_EXEC)		p |= PROT_EXEC;
    if (prot  & XF86_PROT_READ)		p |= PROT_READ;
    if (prot  & XF86_PROT_WRITE)	p |= PROT_WRITE;
    if (prot  & XF86_PROT_NONE)		p |= PROT_NONE;

    rc = mmap(start,(size_t)length,p,f,fd,(off_t)offset);

    xf86errno = xf86GetErrno();
    if (rc == MAP_FAILED)
	return XF86_MAP_FAILED;
    else
	return rc;
#else
#ifdef HAS_SVR3_MMAPDRV
    void *rc;
#ifdef SELF_CONTAINED_WRAPPER
    if(mmapFd < 0) {
      if ((mmapFd = open("/dev/mmap", O_RDWR)) == -1) {
          ErrorF("Warning: failed to open /dev/mmap \n");
          xf86errno = xf86_ENOSYS;
          return XF86_MAP_FAILED;
      }
    }
#endif
    MapDSC.vaddr    = (char *)start;
    MapDSC.physaddr = (char *)offset;
    MapDSC.length   = length;
    MapDSC.ioflg    = 1;

    rc = (pointer)ioctl(mmapFd, MAP, &MapDSC);
    xf86errno = xf86GetErrno();
    if (rc == NULL)
	return XF86_MAP_FAILED;
    else
	return rc;
#else
    ErrorF("Warning: mmap() is not supported on this platform\n");
    xf86errno = xf86_ENOSYS;
    return XF86_MAP_FAILED;
#endif
#endif
}

int
xf86munmap(void *start, xf86size_t length)
{
#ifndef NO_MMAP
    int rc = munmap(start,(size_t)length);

    xf86errno = xf86GetErrno();
    return rc;
#else
#ifdef HAS_SVR3_MMAPDRV
    int rc = ioctl(mmapFd, UNMAPRM , start);
 
    xf86errno = xf86GetErrno();
    return rc;
#else
    ErrorF("Warning: munmap() is not supported on this platform\n");
    xf86errno = xf86_ENOSYS;
    return -1;
#endif
#endif
}

int
xf86stat(const char *file_name, struct xf86stat *xfst)
{
    int         rc;
    struct stat st;

    rc            = stat(file_name, &st);
    xf86errno     = xf86GetErrno();
    xfst->st_rdev = st.st_rdev;	/* Not much is currently supported */
    return rc;
}

int
xf86fstat(int fd, struct xf86stat *xfst)
{
    int         rc;
    struct stat st;

    rc            = fstat(fd, &st);
    xf86errno     = xf86GetErrno();
    xfst->st_rdev = st.st_rdev;	/* Not much is currently supported */
    return rc;
}

static int
xfToOsAccessMode(int xfmode)
{
    switch(xfmode) {
    case XF86_R_OK: return R_OK;
    case XF86_W_OK: return W_OK;
    case XF86_X_OK: return X_OK;
    case XF86_F_OK: return F_OK;
    }
    return 0;
}

int
xf86access(const char *pathname, int mode)
{
    int rc;
    
    mode      = xfToOsAccessMode(mode);
    rc        = access(pathname, mode);
    xf86errno = xf86GetErrno();
    return rc;
}



/* limited stdio support */

#define XF86FILE_magic	0x58464856	/* "XFHV" */

typedef struct _xf86_file_ {
	INT32	fileno;
	INT32	magic;
	FILE*	filehnd;
	char*	fname;
} XF86FILE_priv;

XF86FILE_priv stdhnd[3] = {
	{ 0, XF86FILE_magic, NULL, "$stdinp$" },
	{ 0, XF86FILE_magic, NULL, "$stdout$" },
	{ 0, XF86FILE_magic, NULL, "$stderr$" }
};

XF86FILE* xf86stdin = (XF86FILE*)&stdhnd[0];
XF86FILE* xf86stdout = (XF86FILE*)&stdhnd[1];
XF86FILE* xf86stderr = (XF86FILE*)&stdhnd[2];

void
xf86WrapperInit()
{
    if (stdhnd[0].filehnd == NULL)
	stdhnd[0].filehnd = stdin;
    if (stdhnd[1].filehnd == NULL)
	stdhnd[1].filehnd = stdout;
    if (stdhnd[2].filehnd == NULL)
	stdhnd[2].filehnd = stderr;
    xf86HUGE_VAL = HUGE_VAL;
}

XF86FILE*
xf86fopen(const char* fn, const char* mode)
{
	XF86FILE_priv* fp;
	FILE *f = fopen(fn,mode);
	xf86errno = xf86GetErrno();
	if (!f) return 0;

	fp = xalloc(sizeof(XF86FILE_priv));
	fp->magic = XF86FILE_magic;
	fp->filehnd = f;
	fp->fileno = fileno(f);
	fp->fname = xf86strdup(fn);
#ifdef DEBUG
	ErrorF("xf86fopen(%s,%s) yields FILE %p XF86FILE %p\n",
		fn,mode,f,fp);
#endif
	return (XF86FILE*)fp;
}

static void _xf86checkhndl(XF86FILE_priv* f,const char *func)
{
	if (!f || f->magic != XF86FILE_magic ||
	    !f->filehnd || !f->fname) {
		FatalError("libc_wrapper error: passed invalid FILE handle to %s\n",
			func);
		exit(42);
	}
}

int
xf86fclose(XF86FILE* f) 
{
	XF86FILE_priv* fp = (XF86FILE_priv*)f;
	int ret;

	_xf86checkhndl(fp,"xf86fclose");

	/* somewhat bad check */
	if (fp->fileno < 3 && fp->fname[0]=='$') {
		/* assume this is stdin/out/err, don't dispose */
		ret = fclose(fp->filehnd);
	} else {
		ret = fclose(fp->filehnd);
		fp->magic = 0;	/* invalidate */
		xfree(fp->fname);
		xfree(fp);
	}
	return ret ? -1 : 0;
}

int
xf86printf(const char *format, ...)
{
	int ret;
	va_list args;
	va_start(args, format);

	ret = printf(format,args);
	va_end(args);
	return ret;
}

int
xf86fprintf(XF86FILE* f, const char *format, ...)
{
	XF86FILE_priv* fp = (XF86FILE_priv*)f;

	int ret;
	va_list args;
	va_start(args, format);

#ifdef DEBUG
	ErrorF("xf86fprintf for XF86FILE %p\n", fp);
#endif
	_xf86checkhndl(fp,"xf86fprintf");

	ret = vfprintf(fp->filehnd,format,args);
	va_end(args);
	return ret;
}

int
xf86vfprintf(XF86FILE* f, const char *format, va_list ap)
{
	XF86FILE_priv* fp = (XF86FILE_priv*)f;

#ifdef DEBUG
	ErrorF("xf86vfprintf for XF86FILE %p\n", fp);
#endif
	_xf86checkhndl(fp,"xf86vfprintf");

	return vfprintf(fp->filehnd,format,ap);
}

int
xf86vsprintf(char *s, const char *format, va_list ap)
{
	return vsprintf(s, format, ap);
}

int
xf86vsnprintf(char *s, xf86size_t len, const char *format, va_list ap)
{
	return vsnprintf(s, (size_t)len, format, ap);
}

#ifdef HAVE_VFSCANF
int
xf86fscanf(XF86FILE* f, const char *format, ...)
#else
int
xf86fscanf(XF86FILE* f, const char *format, char *a0, char *a1, char *a2,
	   char *a3, char *a4, char *a5, char *a6, char *a7, char *a8,
	   char *a9) /* limit of ten args */
#endif
{
	XF86FILE_priv* fp = (XF86FILE_priv*)f;

#ifdef HAVE_VFSCANF
	int ret;
	va_list args;
	va_start(args, format);

	_xf86checkhndl(fp,"xf86fscanf");

	ret = vfscanf(fp->filehnd,format,args);
	va_end(args);
	return ret;
#else
	_xf86checkhndl(fp,"xf86fscanf");
	return fscanf(fp->filehnd, format, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9);
#endif
}

char *
xf86fgets(char *buf, INT32 n, XF86FILE* f)
{
	XF86FILE_priv* fp = (XF86FILE_priv*)f;

	_xf86checkhndl(fp,"xf86fgets");
	return fgets(buf,(int)n,fp->filehnd);
}

int
xf86fputs(const char *buf, XF86FILE* f)
{
	XF86FILE_priv* fp = (XF86FILE_priv*)f;

	_xf86checkhndl(fp,"xf86fputs");
	return fputs(buf,fp->filehnd);
}

int
xf86getc(XF86FILE* f)
{
	XF86FILE_priv* fp = (XF86FILE_priv*)f;

	_xf86checkhndl(fp,"xf86getc");
	return getc(fp->filehnd);
}

int
xf86fgetc(XF86FILE* f)
{
	XF86FILE_priv* fp = (XF86FILE_priv*)f;

	_xf86checkhndl(fp,"xf86fgetc");
	return fgetc(fp->filehnd);
}

int
xf86fputc(int c,XF86FILE* f)
{
	XF86FILE_priv* fp = (XF86FILE_priv*)f;

	_xf86checkhndl(fp,"xf86fputc");
	return fputc(c,fp->filehnd);
}

int
xf86fflush(XF86FILE* f)
{
	XF86FILE_priv* fp = (XF86FILE_priv*)f;

	_xf86checkhndl(fp,"xf86fflush");
	return fflush(fp->filehnd);
}

xf86size_t
xf86fread(void* buf, xf86size_t sz, xf86size_t cnt, XF86FILE* f)
{
	XF86FILE_priv* fp = (XF86FILE_priv*)f;

#ifdef DEBUG
	ErrorF("xf86fread for XF86FILE %p\n", fp);
#endif
	_xf86checkhndl(fp,"xf86fread");
	return fread(buf,(size_t)sz,(size_t)cnt,fp->filehnd);
}

xf86size_t
xf86fwrite(const void* buf, xf86size_t sz, xf86size_t cnt, XF86FILE* f)
{
	XF86FILE_priv* fp = (XF86FILE_priv*)f;

	_xf86checkhndl(fp,"xf86fwrite");
	return fwrite(buf,(size_t)sz,(size_t)cnt,fp->filehnd);
}

int
xf86fseek(XF86FILE* f, long offset, int whence)
{
	XF86FILE_priv* fp = (XF86FILE_priv*)f;

	_xf86checkhndl(fp,"xf86fseek");
	switch (whence) {
	case XF86_SEEK_SET:
		whence = SEEK_SET;
		break;
	case XF86_SEEK_CUR:
		whence = SEEK_CUR;
		break;
	case XF86_SEEK_END:
		whence = SEEK_END;
		break;
	}
	return fseek(fp->filehnd,offset,whence);
}

long
xf86ftell(XF86FILE* f)
{
	XF86FILE_priv* fp = (XF86FILE_priv*)f;

	_xf86checkhndl(fp,"xf86ftell");
	return ftell(fp->filehnd);
}

char*
xf86strerror(int n)
{
	return strerror(n);
}

/* required for portable fgetpos/fsetpos,
 * use as
 *	XF86fpos_t* pos = xalloc(xf86fpossize());
 */
long
xf86fpossize()
{
	return sizeof(fpos_t);
}

int
xf86fgetpos(XF86FILE* f,XF86fpos_t* pos)
{
	XF86FILE_priv* fp = (XF86FILE_priv*)f;
	fpos_t *ppos = (fpos_t*)pos;

	_xf86checkhndl(fp,"xf86fgetpos");
#ifndef ISC
	return fgetpos(fp->filehnd,ppos);
#else
	*ppos = ftell(fp->filehnd);
	if (*ppos < 0L)
		return(-1);
	return(0);
#endif
}

int
xf86fsetpos(XF86FILE* f,const XF86fpos_t* pos)
{
	XF86FILE_priv* fp = (XF86FILE_priv*)f;
	fpos_t *ppos = (fpos_t*)pos;

	/* XXX need to handle xf86errno here */
	_xf86checkhndl(fp,"xf86fsetpos");
#ifndef ISC
	return fsetpos(fp->filehnd,ppos);
#else
	if (ppos == NULL)
	{
		errno = EINVAL;
		return EOF;
	}
	return fseek(fp->filehnd, *ppos, SEEK_SET);
#endif
}

void
xf86perror(const char *s)
{
	perror(s);
}

int
xf86remove(const char *s)
{
#ifdef _POSIX_SOURCE
	return remove(s);
#else
	return unlink(s);
#endif
}

int
xf86rename(const char *old, const char *new)
{
#ifdef _POSIX_SOURCE
	return rename(old,new);
#else
	int ret = link(old,new);
	if (!ret) {
		ret = unlink(old);
		if (ret) unlink(new);
	} else
		ret = unlink(new);
	return ret;
#endif
}

void
xf86rewind(XF86FILE* f)
{
	XF86FILE_priv* fp = (XF86FILE_priv*)f;

	_xf86checkhndl(fp,"xf86fsetpos");
	rewind(fp->filehnd);
}

void
xf86clearerr(XF86FILE* f)
{
	XF86FILE_priv* fp = (XF86FILE_priv*)f;

	_xf86checkhndl(fp,"xf86clearerr");
	clearerr(fp->filehnd);
}

int
xf86feof(XF86FILE* f)
{
	XF86FILE_priv* fp = (XF86FILE_priv*)f;

	_xf86checkhndl(fp,"xf86feof");
	return feof(fp->filehnd);
}

int
xf86ferror(XF86FILE* f)
{
	XF86FILE_priv* fp = (XF86FILE_priv*)f;

	_xf86checkhndl(fp,"xf86ferror");
	return ferror(fp->filehnd);
}

XF86FILE*
xf86freopen(const char* fname,const char* mode,XF86FILE* fold)
{
	XF86FILE_priv* fp = (XF86FILE_priv*)fold;
	FILE *fnew;

	_xf86checkhndl(fp,"xf86freopen");
	fnew = freopen(fname,mode,fp->filehnd);
	xf86errno = xf86GetErrno();
	if (!fnew) {
		xf86fclose(fold);	/* discard old XF86FILE structure */
		return 0;
	}
	/* recycle the old XF86FILE structure */
	fp->magic = XF86FILE_magic;
	fp->filehnd = fnew;
	fp->fileno = fileno(fnew);
	fp->fname = xf86strdup(fname);
#ifdef DEBUG
	ErrorF("xf86freopen(%s,%s,%p) yields FILE %p XF86FILE %p\n",
		fname,mode,fold,fnew,fp);
#endif
	return (XF86FILE*)fp;
}

int
xf86setbuf(XF86FILE* f, char *buf)
{
	XF86FILE_priv* fp = (XF86FILE_priv*)f;

	_xf86checkhndl(fp,"xf86fsetbuf");
#ifdef SETBUF_RETURNS_INT
	return setbuf(fp->filehnd, buf);
#else
	setbuf(fp->filehnd, buf);
	return 0;
#endif
}

int
xf86setvbuf(XF86FILE* f, char *buf, int mode, xf86size_t size)
{
	XF86FILE_priv* fp = (XF86FILE_priv*)f;
	int vbufmode;

	_xf86checkhndl(fp,"xf86fsetvbuf");

	switch (mode) {
	case XF86_IONBF:
		vbufmode = _IONBF;
		break;
	case XF86_IOLBF:
		vbufmode = _IOFBF;
		break;
	case XF86_IOFBF:
		vbufmode = _IOLBF;
		break;
	default:
		FatalError("libc_wrapper error: mode in setvbuf incorrect\n");
		exit(42);
	}

	return setvbuf(fp->filehnd,buf,vbufmode,(size_t)size);
}

XF86FILE*
xf86tmpfile(void)
{
#ifdef NEED_TMPFILE
	return xf86fopen(tmpnam((char*)0),"w+");
#else
	XF86FILE_priv* fp;
	FILE *f = tmpfile();
	xf86errno = xf86GetErrno();
	if (!f) return 0;

	fp = xalloc(sizeof(XF86FILE_priv));
	fp->magic = XF86FILE_magic;
	fp->filehnd = f;
	fp->fileno = fileno(f);
	fp->fname = xf86strdup("*tmpfile*"); /* so that it can be xfree()'d */
#ifdef DEBUG
	ErrorF("xf86tmpfile() yields FILE %p XF86FILE %p\n",f,fp);
#endif
	return (XF86FILE*)fp;
}
#endif /* HAS_TMPFILE */


int
xf86ungetc(int c,XF86FILE* f)
{
	XF86FILE_priv* fp = (XF86FILE_priv*)f;

	_xf86checkhndl(fp,"xf86ungetc");
	return ungetc(c,fp->filehnd);
}

/* Misc functions. Some are ANSI C, some are not. */

void
xf86usleep(usec)
    unsigned long usec;
{
#if (defined(SYSV) || defined(SVR4)) && !defined(sun)
    syscall(3112, (usec) / 1000 + 1);
#else
    usleep(usec);
#endif
}

void
xf86getsecs(long * secs, long * usecs)
{
	struct timeval tv;

	gettimeofday(&tv, NULL);
	*secs = tv.tv_sec;
	*usecs= tv.tv_usec;

	return;
}

int
xf86ffs(int mask)
{
	int n;
	if (mask == 0) return 0;
	for (n = 1; (mask & 1)==0; n++)
		mask >>= 1;
	return n;
}

char *
xf86getenv(const char * a)
{
	/* Only allow this when the real and effective uids are the same */
	if (getuid() != geteuid())
		return NULL;
	else
		return(getenv(a));
}

void *
xf86bsearch(const void *key, const void *base, xf86size_t nmemb,
	    xf86size_t size, int (*compar)(const void *, const void *))
{
	return bsearch(key, base, (size_t)nmemb, (size_t)size, compar);
}

/*VARARGS1*/
int
xf86execl(const char *pathname, const char *arg, ...)
{
#ifndef __EMX__
    int i;
    pid_t pid;
#ifdef MACH386
    union wait exit_status;
#else
    int exit_status;
#endif
    char *arglist[5];
    va_list args;
    va_start(args, arg);
    arglist[0] = (char*)&args;
    i = 1;
    while (i < 5 && (arglist[i++] = va_arg(args, char *)) != NULL)
	;
    va_end(args);

    if ((pid = fork()) < 0) {
        ErrorF("Fork failed (%s)\n", strerror(errno));
        return -1;
    } else if (pid == 0) { /* child */
	/* 
	 * Make sure that the child doesn't inherit any I/O permissions it
	 * shouldn't have.  It's better to put constraints on the development
	 * of a clock program than to give I/O permissions to a bogus program
	 * in someone's XF86Config file
	 */
#ifndef SELF_CONTAINED_WRAPPER
	xf86DisableIO();
#endif
        setuid(getuid());
#if !defined(SELF_CONTAINED_WRAPPER) && !defined(AMOEBA) && !defined(MINIX)
        /* set stdin, stdout to the consoleFD, and leave stderr alone */
        for (i = 0; i < 2; i++)
        {
          if (xf86Info.consoleFd != i)
          {
            close(i);
            dup(xf86Info.consoleFd);
          }
        }
#endif

	execv(pathname, arglist);
	ErrorF("Exec failed for command \"%s\" (%s)\n",
	       pathname, strerror(errno));
	exit(255);
    }

    /* parent */
    wait(&exit_status);
    if (WIFEXITED(exit_status))
    {
	switch (WEXITSTATUS(exit_status))
	    {
	    case 0:     /* OK */
		return 0;
	    case 255:   /* exec() failed */
		return(255);
	    default:    /* bad exit status */
		ErrorF("Program \"%s\" had bad exit status %d\n",
		       pathname, WEXITSTATUS(exit_status));
		return(WEXITSTATUS(exit_status));
	    }
    }
    else if (WIFSIGNALED(exit_status))
    {
	ErrorF("Program \"%s\" died on signal %d\n",
	       pathname, WTERMSIG(exit_status));
	return(WTERMSIG(exit_status));
    }
#ifdef WIFSTOPPED
    else if (WIFSTOPPED(exit_status))
    {
	ErrorF("Program \"%s\" stopped by signal %d\n",
	       pathname, WSTOPSIG(exit_status));
	return(WSTOPSIG(exit_status));
    }
#endif
    else /* should never get to this point */
    {
	ErrorF("Program \"%s\" has unknown exit condition\n",
	       pathname);
	return(1);
    }
#else
    return(1);
#endif /* __EMX__ Disable this crazy business for now */
}

void
xf86abort(void)
{
	ErrorF("Module called abort() function\n");
	abort();
}

void
xf86exit(int ex)
{
	ErrorF("Module called exit() function with value=%d\n",ex);
	exit(ex);
}

/* directory handling functions */
#define XF86DIR_magic	0x78666876	/* "xfhv" */

typedef struct _xf86_dir_ {
	DIR		*dir;
	INT32		magic;
	XF86DIRENT	*dirent;
} XF86DIR_priv;

static void
_xf86checkdirhndl(XF86DIR_priv* f,const char *func)
{
	if (!f || f->magic != XF86DIR_magic || !f->dir || !f->dirent) {
		FatalError("libc_wrapper error: passed invalid DIR handle to %s\n",
			func);
		exit(42);
	}
}

XF86DIR *
xf86opendir(const char *name)
{
	XF86DIR_priv *dp;
	DIR *dirp;

	dirp = opendir(name);
	if (!dirp)
		return (XF86DIR*)0;

	dp = xalloc(sizeof(XF86DIR_priv));
	dp->magic = XF86DIR_magic; /* This time I have this, Dirk! :-) */
	dp->dir = dirp;
	dp->dirent = xalloc(sizeof(struct _xf86dirent));

	return (XF86DIR*)dp;
}

XF86DIRENT*
xf86readdir(XF86DIR* dirp)
{
	XF86DIR_priv* dp = (XF86DIR_priv*)dirp;
	DIRENTRY *de;
	XF86DIRENT* xde;
	int sz;

	_xf86checkdirhndl(dp,"xf86readdir");

	de = readdir(dp->dir);
	if (!de)
		return (XF86DIRENT*)0;
	xde = dp->dirent;
	sz = strlen(de->d_name);
	strncpy(xde->d_name,de->d_name, sz>_XF86NAMELEN ? (_XF86NAMELEN+1) : (sz+1));
	xde->d_name[_XF86NAMELEN] = '\0';	/* be sure to have a 0 byte */
	return xde;
}

void
xf86rewinddir(XF86DIR* dirp)
{
	XF86DIR_priv* dp = (XF86DIR_priv*)dirp;

	_xf86checkdirhndl(dp,"xf86readdir");
	rewinddir(dp->dir);
}

int
xf86closedir(XF86DIR* dir)
{
	XF86DIR_priv* dp = (XF86DIR_priv*)dir;
	int n;

	_xf86checkdirhndl(dp,"xf86readdir");

	n = closedir(dp->dir);
	dp->magic = 0;
	xfree(dp->dirent);
	xfree(dp);

	return n;
}

static mode_t
xfToOsChmodMode(xf86mode_t xfmode)
{
    mode_t mode = 0;

    if (xfmode & XF86_S_ISUID) mode |= S_ISUID;
    if (xfmode & XF86_S_ISGID) mode |= S_ISGID;
#ifndef __EMX__
    if (xfmode & XF86_S_ISVTX) mode |= S_ISVTX;
#endif
    if (xfmode & XF86_S_IRUSR) mode |= S_IRUSR;
    if (xfmode & XF86_S_IWUSR) mode |= S_IWUSR;
    if (xfmode & XF86_S_IXUSR) mode |= S_IXUSR;
    if (xfmode & XF86_S_IRGRP) mode |= S_IRGRP;
    if (xfmode & XF86_S_IWGRP) mode |= S_IWGRP;
    if (xfmode & XF86_S_IXGRP) mode |= S_IXGRP;
    if (xfmode & XF86_S_IROTH) mode |= S_IROTH;
    if (xfmode & XF86_S_IWOTH) mode |= S_IWOTH;
    if (xfmode & XF86_S_IXOTH) mode |= S_IXOTH;

    return mode;
}

int
xf86chmod(const char *path, xf86mode_t xfmode)
{
    mode_t mode = xfToOsChmodMode(xfmode);
    int    rc   = chmod(path, mode);
    
    xf86errno   = xf86GetErrno();
    return rc;
}

int
xf86chown(const char *path, xf86uid_t owner, xf86gid_t group)
{
#ifndef __EMX__
    int rc = chown(path, owner, group);
#else
    int rc = 0;
#endif
    xf86errno = xf86GetErrno();
    return rc;
}

xf86uid_t
xf86geteuid(void)
{
    return geteuid();
}

xf86gid_t
xf86getegid(void)
{
    return getegid();
}

int
xf86getpid(void)
{
    return getpid();
}

static mode_t
xfToOsMknodMode(xf86mode_t xfmode)
{
    mode_t mode = xfToOsChmodMode(xfmode);

    if (xfmode & XF86_S_IFREG) mode |= S_IFREG;
    if (xfmode & XF86_S_IFCHR) mode |= S_IFCHR;
#ifndef __EMX__
    if (xfmode & XF86_S_IFBLK) mode |= S_IFBLK;
#endif
    if (xfmode & XF86_S_IFIFO) mode |= S_IFIFO;

    return mode;
}

int xf86mknod(const char *pathname, xf86mode_t xfmode, xf86dev_t dev)
{
    mode_t mode = xfToOsMknodMode(xfmode);
#ifndef __EMX__
    int rc      = mknod(pathname, mode, dev);
#else
    int rc = 0;
#endif    
    xf86errno   = xf86GetErrno();
    return rc;
}

unsigned int xf86sleep(unsigned int seconds)
{
    return sleep(seconds);
}

int xf86mkdir(const char *pathname, xf86mode_t xfmode)
{
    mode_t mode = xfToOsChmodMode(xfmode);
    int    rc   = mkdir(pathname, mode);
    
    xf86errno   = xf86GetErrno();
    return rc;
}


/* Several math functions */

int
xf86abs(int x)
{
	return abs(x);
}

double
xf86acos(double x)
{
	return acos(x);
}

double
xf86asin(double x)
{
	return asin(x);
}

double
xf86atan(double x)
{
	return atan(x);
}

double
xf86atan2(double x,double y)
{
	return atan2(x,y);
}

double
xf86atof(const char* s)
{
	return atof(s);
}

int
xf86atoi(const char* s)
{
	return atoi(s);
}

long
xf86atol(const char* s)
{
	return atol(s);
}

double
xf86ceil(double x)
{
	return ceil(x);
}

double
xf86cos(double x)
{
	return(cos(x));
}

double
xf86exp(double x)
{
	return(exp(x));
}

double
xf86fabs(double x)
{
        return(fabs(x));
}

double
xf86floor(double x)
{
	return floor(x);
}

double
xf86fmod(double x,double y)
{
	return fmod(x,y);
}

long
xf86labs(long x)
{
	return labs(x);
}

double
xf86ldexp(double x, int exp)
{
	return ldexp(x, exp);
}

double
xf86log(double x)
{
	return(log(x));
}

double
xf86log10(double x)
{
	return(log10(x));
}

double
xf86modf(double x,double* y)
{
	return modf(x,y);
}

double
xf86pow(double x, double y)
{
	return(pow(x,y));
}

double
xf86sin(double x)
{
	return sin(x);
}

double
xf86sqrt(double x)
{
	return(sqrt(x));
}

double
xf86strtod(const char *s, char **end)
{
	return strtod(s,end);
}

long
xf86strtol(const char *s, char **end, int radix)
{
	return strtol(s,end,radix);
}

unsigned long
xf86strtoul(const char *s, char **end,int radix)
{
	return strtoul(s,end,radix);
}

double
xf86tan(double x)
{
	return tan(x);
}

/* memory functions */
void*
xf86memchr(const void* s, int c, xf86size_t n)
{
	return memchr(s,c,(size_t)n);
}

int
xf86memcmp(const void* s1, const void* s2, xf86size_t n)
{
	return(memcmp(s1,s2,(size_t)n));
}

void*
xf86memcpy(void* dest, const void* src, xf86size_t n)
{
	return(memcpy(dest,src,(size_t)n));
}

void*
xf86memmove(void* dest, const void* src, xf86size_t n)
{
	return(memmove(dest,src,(size_t)n));
}

void*
xf86memset(void* s, int c, xf86size_t n)
{
	return(memset(s,c,(size_t)n));
}

/* ctype functions */

int
xf86isalnum(int c)
{
	return isalnum(c) ? 1 : 0;
}

int
xf86isalpha(int c)
{
	return isalpha(c) ? 1 : 0;
}

int
xf86iscntrl(int c)
{
	return iscntrl(c) ? 1 : 0;
}

int
xf86isdigit(int c)
{
	return isdigit(c) ? 1 : 0;
}

int
xf86isgraph(int c)
{
	return isgraph(c) ? 1 : 0;
}

int
xf86islower(int c)
{
	return islower(c) ? 1 : 0;
}

int
xf86isprint(int c)
{
	return isprint(c) ? 1 : 0;
}

int
xf86ispunct(int c)
{
	return ispunct(c) ? 1 : 0;
}

int
xf86isspace(int c)
{
	return isspace(c) ? 1 : 0;
}

int
xf86isupper(int c)
{
	return isupper(c) ? 1 : 0;
}

int
xf86isxdigit(int c)
{
	return isxdigit(c) ? 1 : 0;
}

int
xf86tolower(int c)
{
	return tolower(c);
}

int
xf86toupper(int c)
{
	return toupper(c);
}

/* memory allocation functions */
void*
xf86calloc(xf86size_t sz,xf86size_t n)
{
	return xcalloc(sz, n);
}

void
xf86free(void* p)
{
	xfree(p);
}

double
xf86frexp(double x, int *exp)
{
        return frexp(x, exp);
}

void*
xf86malloc(xf86size_t n)
{
	return xalloc(n);
}

void*
xf86realloc(void* p, xf86size_t n)
{
	return xrealloc(p,n);
}

/*
 * XXX This probably doesn't belong here.
 */
int
xf86getpagesize()
{
	static int pagesize = -1;

	if (pagesize != -1)
		return pagesize;

#if defined(_SC_PAGESIZE) || defined(HAS_SC_PAGESIZE)
	pagesize = sysconf(_SC_PAGESIZE);
#endif
#ifdef _SC_PAGE_SIZE
	if (pagesize == -1)
		pagesize = sysconf(_SC_PAGE_SIZE);
#endif
#ifdef HAS_GETPAGESIZE
	if (pagesize == -1)
		pagesize = getpagesize();
#endif
#ifdef PAGE_SIZE
	if (pagesize == -1)
		pagesize = PAGE_SIZE;
#endif
	if (pagesize == -1)
		FatalError("xf86getpagesize: Cannot determine page size\n");

	return pagesize;
}


#define mapnum(e) case (e): return (xf86_##e)

int
xf86GetErrno ()
{
	switch (errno)
	{
		case 0: return 0;
		mapnum (EACCES);
		mapnum (EAGAIN);
		mapnum (EBADF);
		mapnum (EEXIST);
		mapnum (EFAULT);
		mapnum (EINTR);
		mapnum (EINVAL);
		mapnum (EISDIR);
		mapnum (ELOOP);		/* not POSIX 1 */
		mapnum (EMFILE);
		mapnum (ENAMETOOLONG);
		mapnum (ENFILE);
		mapnum (ENOENT);
		mapnum (ENOMEM);
		mapnum (ENOSPC);
		mapnum (ENOTDIR);
		mapnum (EPIPE);
		mapnum (EROFS);
#ifndef __EMX__
		mapnum (ETXTBSY);	/* not POSIX 1 */
#endif
		mapnum (ENOTTY);
		mapnum (EBUSY);
		mapnum (ENODEV);
		mapnum (EIO);

		default:
			return (xf86_UNKNOWN);
	}
}

#undef mapnum



#ifdef NEED_SNPRINTF
#include "snprintf.c"
#endif

#ifdef HAVE_SYSV_IPC

int
xf86shmget(xf86key_t key, int size, int xf86shmflg)
{
    int shmflg = xf86shmflg & 0777;

    if (key == XF86IPC_PRIVATE) key = IPC_PRIVATE;
    

    if (xf86shmflg & XF86SHM_R) shmflg |= SHM_R;
    if (xf86shmflg & XF86SHM_W) shmflg |= SHM_W;
    if (xf86shmflg & XF86IPC_CREAT) shmflg |= IPC_CREAT;
    if (xf86shmflg & XF86IPC_EXCL) shmflg |= IPC_EXCL;
    if (xf86shmflg & XF86IPC_NOWAIT) shmflg |= IPC_NOWAIT;
    return shmget((key_t) key, size, shmflg);
}

char *
xf86shmat(int id, char *addr, int xf86shmflg)
{
    int shmflg = 0;
    
    if (xf86shmflg & XF86SHM_RDONLY) shmflg |= SHM_RDONLY;
    if (xf86shmflg & XF86SHM_RND) shmflg    |= SHM_RND;
    if (xf86shmflg & XF86SHM_REMAP) shmflg  |= SHM_REMAP;

    return shmat(id,addr,shmflg);
}

int
xf86shmdt(char *addr)
{
    return shmdt(addr);
}

/*
 * for now only implement the rmid command.
 */
int
xf86shmctl(int id, int xf86cmd, pointer *buf)
{
    int cmd;

    switch (xf86cmd) {
    case XF86IPC_RMID:
	cmd = IPC_RMID;
	break;
    default:
	return 0;
    }
    
    return shmctl(id, cmd, NULL);
}
#else

int
xf86shmget(xf86key_t key, int size, int xf86shmflg)
{
    return -1;
    
}

char *
xf86shmat(int id, char *addr, int xf86shmflg)
{
    return (char *)-1;
}

int
xf86shmctl(int id, int xf86cmd, pointer *buf)
{
    return -1;
}

int
xf86shmdt(char *addr)
{
    return -1;
}
#endif /* HAVE_SYSV_IPC */

int
xf86setjmp(xf86jmp_buf xf86env)
{
    return setjmp((void *)xf86env);
}

void
xf86longjmp(xf86jmp_buf xf86env, int val)
{
    longjmp((void *)xf86env, val);
}

