/* $TOG: pm.h /main/2 1998/02/10 18:17:58 kaleb $ */

/*
Copyright 1996, 1998  The Open Group

All Rights Reserved.

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall
not be used in advertising or otherwise to promote the sale, use or
other dealings in this Software without prior written authorization
from The Open Group.
*/

#include <X11/ICE/ICElib.h>

extern IceConn PM_iceConn;
extern int proxy_manager_fd;
extern Bool proxyMngr;

extern Bool CheckForProxyManager (
#if NeedFunctionPrototypes
    void
#endif
);

extern void ConnectToProxyManager (
#if NeedFunctionPrototypes
    void
#endif
);

extern void SendGetProxyAddrReply (
#if NeedFunctionPrototypes
    IceConn /*requestor_iceConn*/,
    int /*status*/,
    char * /*addr*/,
    char * /*error*/
#endif
);

extern void HandleProxyManagerConnection (
#if NeedFunctionPrototypes
    void
#endif
);
