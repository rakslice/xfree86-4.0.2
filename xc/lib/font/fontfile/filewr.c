/* $TOG: filewr.c /main/5 1998/02/09 10:48:21 kaleb $ */

/*

Copyright 1991, 1998  The Open Group

All Rights Reserved.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from The Open Group.

*/
/* $XFree86: xc/lib/font/fontfile/filewr.c,v 3.4 2000/08/09 23:40:15 dawes Exp $ */

/*
 * Author:  Keith Packard, MIT X Consortium
 */

#include <fntfilio.h>
#include <X11/Xos.h>

FontFilePtr
FontFileOpenWrite (const char *name)
{
    int	fd;

#if defined(WIN32) || defined(__EMX__) || defined(__CYGWIN__)
    fd = open (name, O_CREAT|O_TRUNC|O_RDWR|O_BINARY, 0666);
#else
    fd = creat (name, 0666);
#endif
    if (fd < 0)
	return 0;
    return (FontFilePtr) BufFileOpenWrite (fd);
}

FontFilePtr
FontFileOpenWriteFd (int fd)
{
    return (FontFilePtr) BufFileOpenWrite (fd);
}

FontFilePtr
FontFileOpenFd (int fd)
{
    return (FontFilePtr) BufFileOpenRead (fd);
}
