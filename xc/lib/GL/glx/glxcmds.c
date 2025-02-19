/* $XFree86: xc/lib/GL/glx/glxcmds.c,v 1.13 2000/11/13 23:31:23 dawes Exp $ */
/*
** The contents of this file are subject to the GLX Public License Version 1.0
** (the "License"). You may not use this file except in compliance with the
** License. You may obtain a copy of the License at Silicon Graphics, Inc.,
** attn: Legal Services, 2011 N. Shoreline Blvd., Mountain View, CA 94043
** or at http://www.sgi.com/software/opensource/glx/license.html.
**
** Software distributed under the License is distributed on an "AS IS"
** basis. ALL WARRANTIES ARE DISCLAIMED, INCLUDING, WITHOUT LIMITATION, ANY
** IMPLIED WARRANTIES OF MERCHANTABILITY, OF FITNESS FOR A PARTICULAR
** PURPOSE OR OF NON- INFRINGEMENT. See the License for the specific
** language governing rights and limitations under the License.
**
** The Original Software is GLX version 1.2 source code, released February,
** 1999. The developer of the Original Software is Silicon Graphics, Inc.
** Those portions of the Subject Software created by Silicon Graphics, Inc.
** are Copyright (c) 1991-9 Silicon Graphics, Inc. All Rights Reserved.
**
** $SGI$
*/

/*
 * Direct rendering support added by Precision Insight, Inc.
 *
 * Authors:
 *   Kevin E. Martin <kevin@precisioninsight.com>
 *   Brian Paul <brian@precisioninsight.com>
 */

#include "packsingle.h"
#include "glxclient.h"
#include <extutil.h>
#include <Xext.h>
#include <string.h>
#include "glapi.h"
#ifdef GLX_DIRECT_RENDERING
#include "indirect_init.h"
#endif

static const char GL_ClientExtensions[] = 
                  "GL_EXT_abgr "
                  "GL_EXT_blend_color "
                  "GL_EXT_blend_minmax "
                  "GL_EXT_blend_subtract " ;

static const char GLXClientVendorName[] = "SGI";
static const char GLXClientVersion[] = "1.2";
static const char GLXClientExtensions[] = 
		    "GLX_EXT_visual_info "
		    "GLX_EXT_visual_rating "
		    "GLX_EXT_import_context "
		    "GLX_ARB_get_proc_address ";

/*
** Create a new context.
*/
static
GLXContext CreateContext(Display *dpy, XVisualInfo *vis,
				GLXContext shareList, 
				Bool allowDirect, GLXContextID contextID)
{
    xGLXCreateContextReq *req;
    GLXContext gc;
    int bufSize = XMaxRequestSize(dpy) * 4;
    CARD8 opcode;
#ifdef GLX_DIRECT_RENDERING
    __GLXdisplayPrivate *priv;
#endif

    opcode = __glXSetupForCommand(dpy);
    if (!opcode) {
	return NULL;
    }

    /* Allocate our context record */
    gc = (GLXContext) Xmalloc(sizeof(struct __GLXcontextRec));
    if (!gc) {
	/* Out of memory */
	return NULL;
    }
    memset(gc, 0, sizeof(struct __GLXcontextRec));

    /* Allocate transport buffer */
    gc->buf = (GLubyte *) Xmalloc(bufSize);
    if (!gc->buf) {
	Xfree(gc);
	return NULL;
    }
    gc->bufSize = bufSize;

    /* Fill in the new context */
    gc->renderMode = GL_RENDER;

    gc->state.storePack.alignment = 4;
    gc->state.storeUnpack.alignment = 4;

    __glXInitVertexArrayState(gc);

    gc->attributes.stackPointer = &gc->attributes.stack[0];

    /*
    ** PERFORMANCE NOTE: A mode dependent fill image can speed things up.
    ** Other code uses the fastImageUnpack bit, but it is never set
    ** to GL_TRUE.
    */
    gc->fastImageUnpack = GL_FALSE;
    gc->fillImage = __glFillImage;
    gc->isDirect = GL_FALSE;
    gc->pc = gc->buf;
    gc->bufEnd = gc->buf + bufSize;
    if (__glXDebug) {
	/*
	** Set limit register so that there will be one command per packet
	*/
	gc->limit = gc->buf;
    } else {
	gc->limit = gc->buf + bufSize - __GLX_BUFFER_LIMIT_SIZE;
    }
    gc->createDpy = dpy;
    gc->majorOpcode = opcode;

    /*
    ** Constrain the maximum drawing command size allowed to be
    ** transfered using the X_GLXRender protocol request.  First
    ** constrain by a software limit, then constrain by the protocl
    ** limit.
    */
    if (bufSize > __GLX_RENDER_CMD_SIZE_LIMIT) {
        bufSize = __GLX_RENDER_CMD_SIZE_LIMIT;
    }
    if (bufSize > __GLX_MAX_RENDER_CMD_SIZE) {
        bufSize = __GLX_MAX_RENDER_CMD_SIZE;
    }
    gc->maxSmallRenderCommandSize = bufSize;


    if (None == contextID) {
#ifdef GLX_DIRECT_RENDERING
	/*
	** Create the direct rendering context, if requested and
	** available.
	*/
	priv = __glXInitialize(dpy);
	if (allowDirect && priv->driDisplay.private) {
	    __GLXscreenConfigs *psc = &priv->screenConfigs[vis->screen];
	    if (psc && psc->driScreen.private) {
		void *shared = (shareList ?
				shareList->driContext.private : NULL);
		gc->driContext.private =
		    (*psc->driScreen.createContext)(dpy, vis, shared,
						    &gc->driContext);
		if (gc->driContext.private) {
		    gc->isDirect = GL_TRUE;
		    gc->screen = vis->screen;
		    gc->vid = vis->visualid;
		}
	    }
	}
#endif

	/* Send the glXCreateContext request */
	LockDisplay(dpy);
	GetReq(GLXCreateContext,req);
	req->reqType = gc->majorOpcode;
	req->glxCode = X_GLXCreateContext;
	req->context = gc->xid = XAllocID(dpy);
	req->visual = vis->visualid;
	req->screen = vis->screen;
	req->shareList = shareList ? shareList->xid : None;
	req->isDirect = gc->isDirect;
	UnlockDisplay(dpy);
	SyncHandle();
	gc->imported = GL_FALSE;
    }
    else {
	gc->xid = contextID;
	gc->imported = GL_TRUE;
    }

    return gc;
}

GLXContext glXCreateContext(Display *dpy, XVisualInfo *vis,
			    GLXContext shareList, Bool allowDirect)
{
    return CreateContext(dpy, vis, shareList, allowDirect, None);
}

void __glXFreeContext(__GLXcontext *gc)
{
    if (gc->vendor) XFree((char *) gc->vendor);
    if (gc->renderer) XFree((char *) gc->renderer);
    if (gc->version) XFree((char *) gc->version);
    if (gc->extensions) XFree((char *) gc->extensions);
    __glFreeAttributeState(gc);
    XFree((char *) gc->buf);
    XFree((char *) gc);
    
}

/*
** Destroy the named context
*/
static void 
DestroyContext(Display *dpy, GLXContext gc)
{
    xGLXDestroyContextReq *req;
    GLXContextID xid;
    CARD8 opcode;
    GLboolean imported;

    opcode = __glXSetupForCommand(dpy);
    if (!opcode || !gc) {
	return;
    }

    __glXLock();
    xid = gc->xid;
    imported = gc->imported;
    gc->xid = None;

#ifdef GLX_DIRECT_RENDERING
    /* Destroy the direct rendering context */
    if (gc->isDirect) {
	if (gc->driContext.private) {
	    (*gc->driContext.destroyContext)(dpy, gc->screen,
					     gc->driContext.private);
	    gc->driContext.private = NULL;
	}
    }
#endif

    if (gc->currentDpy) {
	/* Have to free later cuz it's in use now */
	__glXUnlock();
    } else {
	/* Destroy the handle if not current to anybody */
	__glXUnlock();
	__glXFreeContext(gc);
    }

    if (!imported) {
	/* 
	** This dpy also created the server side part of the context.
	** Send the glXDestroyContext request.
	*/
	LockDisplay(dpy);
	GetReq(GLXDestroyContext,req);
	req->reqType = opcode;
	req->glxCode = X_GLXDestroyContext;
	req->context = xid;
	UnlockDisplay(dpy);
	SyncHandle();
    }
}
void glXDestroyContext(Display *dpy, GLXContext gc)
{
    DestroyContext(dpy, gc);
}

/*
** Return the major and minor version #s for the GLX extension
*/
Bool glXQueryVersion(Display *dpy, int *major, int *minor)
{
    __GLXdisplayPrivate *priv;

    /* Init the extension.  This fetches the major and minor version. */
    priv = __glXInitialize(dpy);
    if (!priv) return GL_FALSE;

    if (major) *major = priv->majorVersion;
    if (minor) *minor = priv->minorVersion;
    return GL_TRUE;
}

/*
** Query the existance of the GLX extension
*/
Bool glXQueryExtension(Display *dpy, int *errorBase, int *eventBase)
{
    int major_op, erb, evb;
    Bool rv;

    rv = XQueryExtension(dpy, GLX_EXTENSION_NAME, &major_op, &evb, &erb);
    if (rv) {
	if (errorBase) *errorBase = erb;
	if (eventBase) *eventBase = evb;
    }
    return rv;
}

/*
** Put a barrier in the token stream that forces the GL to finish its
** work before X can proceed.
*/
void glXWaitGL(void)
{
    xGLXWaitGLReq *req;
    GLXContext gc = __glXGetCurrentContext();
    Display *dpy = gc->currentDpy;

    if (!dpy) return;

    /* Flush any pending commands out */
    __glXFlushRenderBuffer(gc, gc->pc);

#ifdef GLX_DIRECT_RENDERING
    if (gc->isDirect) {
/* This bit of ugliness unwraps the glFinish function */
#ifdef glFinish
#undef glFinish
#endif
	glFinish();
	return;
    }
#endif

    /* Send the glXWaitGL request */
    LockDisplay(dpy);
    GetReq(GLXWaitGL,req);
    req->reqType = gc->majorOpcode;
    req->glxCode = X_GLXWaitGL;
    req->contextTag = gc->currentContextTag;
    UnlockDisplay(dpy);
    SyncHandle();
}

/*
** Put a barrier in the token stream that forces X to finish its
** work before GL can proceed.
*/
void glXWaitX(void)
{
    xGLXWaitXReq *req;
    GLXContext gc = __glXGetCurrentContext();
    Display *dpy = gc->currentDpy;

    if (!dpy) return;

    /* Flush any pending commands out */
    __glXFlushRenderBuffer(gc, gc->pc);

#ifdef GLX_DIRECT_RENDERING
    if (gc->isDirect) {
	XSync(dpy, False);
	return;
    }
#endif

    /*
    ** Send the glXWaitX request.
    */
    LockDisplay(dpy);
    GetReq(GLXWaitX,req);
    req->reqType = gc->majorOpcode;
    req->glxCode = X_GLXWaitX;
    req->contextTag = gc->currentContextTag;
    UnlockDisplay(dpy);
    SyncHandle();
}

void glXUseXFont(Font font, int first, int count, int listBase)
{
    xGLXUseXFontReq *req;
    GLXContext gc = __glXGetCurrentContext();
    Display *dpy = gc->currentDpy;

    if (!dpy) return;

    /* Flush any pending commands out */
    (void) __glXFlushRenderBuffer(gc, gc->pc);

#ifdef GLX_DIRECT_RENDERING
    if (gc->isDirect) {
      DRI_glXUseXFont(font, first, count, listBase);
      return;
    }
#endif

    /* Send the glXUseFont request */
    LockDisplay(dpy);
    GetReq(GLXUseXFont,req);
    req->reqType = gc->majorOpcode;
    req->glxCode = X_GLXUseXFont;
    req->contextTag = gc->currentContextTag;
    req->font = font;
    req->first = first;
    req->count = count;
    req->listBase = listBase;
    UnlockDisplay(dpy);
    SyncHandle();
}

/************************************************************************/

/*
** Copy the source context to the destination context using the
** attribute "mask".
*/
void glXCopyContext(Display *dpy, GLXContext source, GLXContext dest,
		    unsigned long mask)
{
    xGLXCopyContextReq *req;
    GLXContext gc = __glXGetCurrentContext();
    GLXContextTag tag;
    CARD8 opcode;

    opcode = __glXSetupForCommand(dpy);
    if (!opcode) {
	return;
    }

#ifdef GLX_DIRECT_RENDERING
    if (gc->isDirect) {
	/* NOT_DONE: This does not work yet */
    }
#endif

    /*
    ** If the source is the current context, send its tag so that the context
    ** can be flushed before the copy.
    */
    if (source == gc && dpy == gc->currentDpy) {
	tag = gc->currentContextTag;
    } else {
	tag = 0;
    }

    /* Send the glXCopyContext request */
    LockDisplay(dpy);
    GetReq(GLXCopyContext,req);
    req->reqType = opcode;
    req->glxCode = X_GLXCopyContext;
    req->source = source ? source->xid : None;
    req->dest = dest ? dest->xid : None;
    req->mask = mask;
    req->contextTag = tag;
    UnlockDisplay(dpy);
    SyncHandle();
}


/*
** Return GL_TRUE if the context is direct rendering or not.
*/
static Bool __glXIsDirect(Display *dpy, GLXContextID contextID)
{
    xGLXIsDirectReq *req;
    xGLXIsDirectReply reply;
    CARD8 opcode;

    opcode = __glXSetupForCommand(dpy);
    if (!opcode) {
	return GL_FALSE;
    }

    /* Send the glXIsDirect request */
    LockDisplay(dpy);
    GetReq(GLXIsDirect,req);
    req->reqType = opcode;
    req->glxCode = X_GLXIsDirect;
    req->context = contextID;
    _XReply(dpy, (xReply*) &reply, 0, False);
    UnlockDisplay(dpy);
    SyncHandle();

    return reply.isDirect;
}

Bool glXIsDirect(Display *dpy, GLXContext gc)
{
    if (!gc) {
	return GL_FALSE;
#ifdef GLX_DIRECT_RENDERING
    } else if (gc->isDirect) {
	return GL_TRUE;
#endif
    }
    return __glXIsDirect(dpy, gc->xid);
}

GLXPixmap glXCreateGLXPixmap(Display *dpy, XVisualInfo *vis, Pixmap pixmap)
{
    xGLXCreateGLXPixmapReq *req;
    GLXPixmap xid;
    CARD8 opcode;

    opcode = __glXSetupForCommand(dpy);
    if (!opcode) {
	return None;
    }

    /* Send the glXCreateGLXPixmap request */
    LockDisplay(dpy);
    GetReq(GLXCreateGLXPixmap,req);
    req->reqType = opcode;
    req->glxCode = X_GLXCreateGLXPixmap;
    req->screen = vis->screen;
    req->visual = vis->visualid;
    req->pixmap = pixmap;
    req->glxpixmap = xid = XAllocID(dpy);
    UnlockDisplay(dpy);
    SyncHandle();
    return xid;
}

/*
** Destroy the named pixmap
*/
void glXDestroyGLXPixmap(Display *dpy, GLXPixmap glxpixmap)
{
    xGLXDestroyGLXPixmapReq *req;
    CARD8 opcode;

    opcode = __glXSetupForCommand(dpy);
    if (!opcode) {
	return;
    }
    
    /* Send the glXDestroyGLXPixmap request */
    LockDisplay(dpy);
    GetReq(GLXDestroyGLXPixmap,req);
    req->reqType = opcode;
    req->glxCode = X_GLXDestroyGLXPixmap;
    req->glxpixmap = glxpixmap;
    UnlockDisplay(dpy);
    SyncHandle();
}

void glXSwapBuffers(Display *dpy, GLXDrawable drawable)
{
    xGLXSwapBuffersReq *req;
    GLXContext gc = __glXGetCurrentContext();
    GLXContextTag tag;
    CARD8 opcode;
#ifdef GLX_DIRECT_RENDERING
    __GLXdisplayPrivate *priv;
    __DRIdrawable *pdraw;

#if defined(XTHREADS)
    xmutex_lock(&__glXSwapBuffersMutex);
#endif

    priv = __glXInitialize(dpy);
    if (priv->driDisplay.private) {
	__GLXscreenConfigs *psc = &priv->screenConfigs[gc->screen];
	if (psc && psc->driScreen.private) {
	    /*
	    ** getDrawable returning NULL implies that the drawable is
	    ** not bound to a direct rendering context.
	    */
	    pdraw = (*psc->driScreen.getDrawable)(dpy, drawable,
						  psc->driScreen.private);
	    if (pdraw) {
		(*pdraw->swapBuffers)(dpy, pdraw->private);
#if defined(XTHREADS)
                xmutex_unlock(&__glXSwapBuffersMutex);
#endif
		return;
	    }
	}
    }
#endif

    opcode = __glXSetupForCommand(dpy);
    if (!opcode) {
#if defined(GLX_DIRECT_RENDERING) && defined(XTHREADS)
        xmutex_unlock(&__glXSwapBuffersMutex);
#endif
	return;
    }

    /*
    ** The calling thread may or may not have a current context.  If it
    ** does, send the context tag so the server can do a flush.
    */
    if ((dpy == gc->currentDpy) && (drawable == gc->currentDrawable)) {
	tag = gc->currentContextTag;
    } else {
	tag = 0;
    }

    /* Send the glXSwapBuffers request */
    LockDisplay(dpy);
    GetReq(GLXSwapBuffers,req);
    req->reqType = opcode;
    req->glxCode = X_GLXSwapBuffers;
    req->drawable = drawable;
    req->contextTag = tag;
    UnlockDisplay(dpy);
    SyncHandle();
    XFlush(dpy);
#if defined(GLX_DIRECT_RENDERING) && defined(XTHREADS)
    xmutex_unlock(&__glXSwapBuffersMutex);
#endif
}

/*
** Return configuration information for the given display, screen and
** visual combination.
*/
int glXGetConfig(Display *dpy, XVisualInfo *vis, int attribute,
		 int *value_return)
{
    __GLXvisualConfig *pConfig;
    __GLXscreenConfigs *psc;
    __GLXdisplayPrivate *priv;
    GLint i;

    /* Initialize the extension, if needed */
    priv = __glXInitialize(dpy);
    if (!priv) {
	/* No extension */
	return GLX_NO_EXTENSION;
    }

    /* Check screen number to see if its valid */
    if ((vis->screen < 0) || (vis->screen >= ScreenCount(dpy))) {
	return GLX_BAD_SCREEN;
    }

    /* Check to see if the GL is supported on this screen */
    psc = &priv->screenConfigs[vis->screen];
    pConfig = psc->configs;
    if (!pConfig) {
	/* No support for GL on this screen regardless of visual */
	if (attribute == GLX_USE_GL) {
	    *value_return = GL_FALSE;
	    return Success;
	}
	return GLX_BAD_VISUAL;
    }

    /* Lookup attribute after first finding a match on the visual */
    for (i = psc->numConfigs; --i >= 0; pConfig++) {
	if (pConfig->vid == vis->visualid) {
	    switch (attribute) {
	      case GLX_USE_GL:
		*value_return = GL_TRUE;
		return Success;
	      case GLX_BUFFER_SIZE:
		*value_return =  pConfig->bufferSize;
		return Success;
	      case GLX_RGBA:
		*value_return = pConfig->rgba;
		return Success;
	      case GLX_RED_SIZE:
		*value_return =  pConfig->redSize;
		return Success;
	      case GLX_GREEN_SIZE:
		*value_return =  pConfig->greenSize;
		return Success;
	      case GLX_BLUE_SIZE:
		*value_return =  pConfig->blueSize;
		return Success;
	      case GLX_ALPHA_SIZE:
		*value_return =  pConfig->alphaSize;
		return Success;
	      case GLX_DOUBLEBUFFER:
		*value_return =  pConfig->doubleBuffer;
		return Success;
	      case GLX_STEREO:
		*value_return =  pConfig->stereo;
		return Success;
	      case GLX_AUX_BUFFERS:
		*value_return =  pConfig->auxBuffers;
		return Success;
	      case GLX_DEPTH_SIZE:
		*value_return =  pConfig->depthSize;
		return Success;
	      case GLX_STENCIL_SIZE:
		*value_return =  pConfig->stencilSize;
		return Success;
	      case GLX_ACCUM_RED_SIZE:
		*value_return =  pConfig->accumRedSize;
		return Success;
	      case GLX_ACCUM_GREEN_SIZE:
		*value_return =  pConfig->accumGreenSize;
		return Success;
	      case GLX_ACCUM_BLUE_SIZE:
		*value_return =  pConfig->accumBlueSize;
		return Success;
	      case GLX_ACCUM_ALPHA_SIZE:
		*value_return =  pConfig->accumAlphaSize;
		return Success;
	      case GLX_LEVEL:
		*value_return =  pConfig->level;
		return Success;
	      case GLX_TRANSPARENT_TYPE_EXT:
		*value_return = pConfig->transparentPixel;
		return Success;
	      case GLX_TRANSPARENT_RED_VALUE_EXT:
		*value_return = pConfig->transparentRed;
		return Success;
	      case GLX_TRANSPARENT_GREEN_VALUE_EXT:
		*value_return = pConfig->transparentGreen;
		return Success;
	      case GLX_TRANSPARENT_BLUE_VALUE_EXT:
		*value_return = pConfig->transparentBlue;
		return Success;
	      case GLX_TRANSPARENT_ALPHA_VALUE_EXT:
		*value_return = pConfig->transparentAlpha;
		return Success;
	      case GLX_TRANSPARENT_INDEX_VALUE_EXT:
		*value_return = pConfig->transparentIndex;
		return Success;
	      case GLX_X_VISUAL_TYPE_EXT:
		switch(pConfig->class) {
		    case TrueColor:    
		      *value_return = GLX_TRUE_COLOR_EXT;   break;
		    case DirectColor:  
		      *value_return = GLX_DIRECT_COLOR_EXT; break;
		    case PseudoColor:  
		      *value_return = GLX_PSEUDO_COLOR_EXT; break;
		    case StaticColor:  
		      *value_return = GLX_STATIC_COLOR_EXT; break;
		    case GrayScale:    
		      *value_return = GLX_GRAY_SCALE_EXT;   break;
		    case StaticGray:   
		      *value_return = GLX_STATIC_GRAY_EXT;  break;
		}
		return Success;
	      case GLX_VISUAL_CAVEAT_EXT:
		*value_return = pConfig->visualRating;
		return Success;
	      default:
		return GLX_BAD_ATTRIBUTE;
	    }
	}
    }

    /*
    ** If we can't find the config for this visual, this visual is not
    ** supported by the OpenGL implementation on the server.
    */
    if (attribute == GLX_USE_GL) {
	*value_return = GL_FALSE;
	return Success;
    }
    return GLX_BAD_VISUAL;
}

/************************************************************************/

/*
** Penalize for more auxiliary buffers than requested
*/
static int AuxScore(int minAux, int aux)
{
    return minAux - aux;
}

/*
** If color is desired, give increasing score for amount available.
** Scale this score by a multiplier to make color differences more
** important than other differences.  Otherwise give decreasing score for
** amount available.
*/
static int ColorScore(int minColor, int color)
{
    if (minColor)
	return 4 * (color - minColor);
    else
	return -color;
}

/*
** If accum buffer is desired, give increasing score for amount
** available.  Otherwise give decreasing score for amount available.
*/
static int AccumScore(int minAccum, int accum)
{
    if (minAccum)
	return accum - minAccum;
    else
	return -accum;
}

/*
** Penalize for indexes larger than requested
*/
static int IndexScore(int minIndex, int ix)
{
    return minIndex - ix;
}

/*
** If depth buffer is desired, give increasing score for amount
** available.  Scale this score by a multiplier to make depth differences
** more important than other non-color differences.  Otherwise give
** decreasing score for amount available.
*/
static int DepthScore(int minDepth, int depth)
{
    if (minDepth)
	return 2 * (depth - minDepth);
    else
	return -depth;
}

/*
** Penalize for stencil buffer larger than requested
*/
static int StencilScore(int minStencil, int stencil)
{
    return minStencil - stencil;
}

/* "Logical" xor - like && or ||; would be ^^ */
#define __GLX_XOR(a,b) (((a) && !(b)) || (!(a) && (b)))

/* Fetch a configuration value */
#define __GLX_GCONF(attrib)				    \
    if (glXGetConfig(dpy, thisVis, attrib, &val)) { 	    \
	XFree((char *)visualList);			    \
	return NULL;					    \
    }


/*
** Return the visual that best matches the template.  Return None if no
** visual matches the template.
*/
XVisualInfo *glXChooseVisual(Display *dpy, int screen, int *attribList)
{
    XVisualInfo visualTemplate;
    XVisualInfo *visualList;
    XVisualInfo *thisVis;
    int count, i, maxscore = 0, maxi, score, val, thisVisRating, maxRating = 0;

    /*
    ** Declare and initialize template variables
    */
    int bufferSize = 0;
    int level = 0;
    int rgba = 0;
    int doublebuffer = 0;
    int stereo = 0;
    int auxBuffers = 0;
    int redSize = 0;
    int greenSize = 0;
    int blueSize = 0;
    int alphaSize = 0;
    int depthSize = 0;
    int stencilSize = 0;
    int accumRedSize = 0;
    int accumGreenSize = 0;
    int accumBlueSize = 0;
    int accumAlphaSize = 0;
    /* for visual_info extension */
    int visualType = 0;		
    int visualTypeValue = 0;
    int transparentPixel = 0;
    int transparentPixelValue = GLX_NONE_EXT; 
    int transparentIndex = 0;
    int transparentIndexValue = 0; 
    int transparentRed = 0;	
    int transparentRedValue = 0;
    int transparentGreen = 0;
    int transparentGreenValue = 0; 
    int transparentBlue = 0;
    int transparentBlueValue = 0; 
    int transparentAlpha = 0;	
    int transparentAlphaValue = 0; 
    /* for visual_rating extension */
    int visualRating = 0; 
    int visualRatingValue = GLX_NONE_EXT; 

    /*
    ** Get a list of all visuals, return if list is empty
    */
    visualTemplate.screen = screen;
    visualList = XGetVisualInfo(dpy,VisualScreenMask,&visualTemplate,&count);
    if (visualList == NULL)
	return None;

    /*
    ** Build a template from the defaults and the attribute list
    ** Free visual list and return if an unexpected token is encountered
    */
    while (*attribList != None) {
	switch (*attribList++) {
	  case GLX_USE_GL:
	    break;
	  case GLX_BUFFER_SIZE:
	    bufferSize = *attribList++;
	    break;
	  case GLX_LEVEL:
	    level = *attribList++;
	    break;
	  case GLX_RGBA:
	    rgba = 1;
	    break;
	  case GLX_DOUBLEBUFFER:
	    doublebuffer = 1;
	    break;
	  case GLX_STEREO:
	    stereo = 1;
	    break;
	  case GLX_AUX_BUFFERS:
	    auxBuffers = *attribList++;
	    break;
	  case GLX_RED_SIZE:
	    redSize = *attribList++;
	    break;
	  case GLX_GREEN_SIZE:
	    greenSize = *attribList++;
	    break;
	  case GLX_BLUE_SIZE:
	    blueSize = *attribList++;
	    break;
	  case GLX_ALPHA_SIZE:
	    alphaSize = *attribList++;
	    break;
	  case GLX_DEPTH_SIZE:
	    depthSize = *attribList++;
	    break;
	  case GLX_STENCIL_SIZE:
	    stencilSize = *attribList++;
	    break;
	  case GLX_ACCUM_RED_SIZE:
	    accumRedSize = *attribList++;
	    break;
	  case GLX_ACCUM_GREEN_SIZE:
	    accumGreenSize = *attribList++;
	    break;
	  case GLX_ACCUM_BLUE_SIZE:
	    accumBlueSize = *attribList++;
	    break;
	  case GLX_ACCUM_ALPHA_SIZE:
	    accumAlphaSize = *attribList++;
	    break;
	  case GLX_X_VISUAL_TYPE_EXT:
	    visualType = 1;
	    visualTypeValue = *attribList++;
	    break;
	  case GLX_TRANSPARENT_TYPE_EXT:
	    transparentPixel = 1;
	    transparentPixelValue = *attribList++;
	    break;
	  case GLX_TRANSPARENT_INDEX_VALUE_EXT:
	    transparentIndex= 1;
	    transparentIndexValue = *attribList++;
	    break;
	  case GLX_TRANSPARENT_RED_VALUE_EXT:
	    transparentRed = 1;
	    transparentRedValue = *attribList++;
	    break;
	  case GLX_TRANSPARENT_GREEN_VALUE_EXT:
	    transparentGreen = 1;
	    transparentGreenValue = *attribList++;
	    break;
	  case GLX_TRANSPARENT_BLUE_VALUE_EXT:
	    transparentBlue = 1;
	    transparentBlueValue = *attribList++;
	    break;
	  case GLX_TRANSPARENT_ALPHA_VALUE_EXT:
	    transparentAlpha = 1;
	    transparentAlphaValue = *attribList++;
	    break;
	  case GLX_VISUAL_CAVEAT_EXT:
	    visualRating = 1;
	    visualRatingValue = *attribList++;
	    break;
	  default:
	    XFree((char *)visualList);
	    return None;
	}
    }

    /*
    ** Eliminate visuals that don't meet minimum requirements
    ** Compute a score for those that do
    ** Remember which visual, if any, got the highest score
    */
    maxi = -1;
    for (i = 0; i < count; i++) {
	score = 0;
	thisVis = &visualList[i];	/* NOTE: used by __GLX_GCONF */

	if (thisVis->class == TrueColor || thisVis->class == PseudoColor) {
	    /* Bump score by one for TrueColor and PseudoColor visuals. */
	    score++;
	}
	
	__GLX_GCONF(GLX_USE_GL);
	if (! val)
	    continue;
	__GLX_GCONF(GLX_LEVEL);
	if (level != val)
	    continue;
	__GLX_GCONF(GLX_RGBA);
	if (__GLX_XOR(rgba, val))
	    continue;
	__GLX_GCONF(GLX_DOUBLEBUFFER);
	if (__GLX_XOR(doublebuffer, val))
	    continue;
	__GLX_GCONF(GLX_STEREO);
	if (__GLX_XOR(stereo, val))
	    continue;
	__GLX_GCONF(GLX_AUX_BUFFERS);
	if (auxBuffers > val)
	    continue;
	else
	    score += AuxScore(auxBuffers, val);
	if (transparentPixel) {
	    if (transparentPixelValue != val)
		continue;
	    if (transparentPixelValue == GLX_TRANSPARENT_TYPE_EXT) {
		if (rgba) {
		    __GLX_GCONF(GLX_TRANSPARENT_RGB_EXT);
		    if (transparentRed) {
			__GLX_GCONF(GLX_TRANSPARENT_RED_VALUE_EXT);
			if (transparentRedValue != val)
			    continue;
		    }
		    if (transparentGreen) {
			__GLX_GCONF(GLX_TRANSPARENT_GREEN_VALUE_EXT);
			if (transparentGreenValue != val)
			    continue;
		    }
		    if (transparentBlue) {
			__GLX_GCONF(GLX_TRANSPARENT_BLUE_VALUE_EXT);
			if (transparentBlueValue != val)
		    continue;
		    }
		    /* Transparent Alpha ignored for now */
		} else {
		    __GLX_GCONF(GLX_TRANSPARENT_INDEX_EXT);
		    if (transparentIndex) {
			__GLX_GCONF(GLX_TRANSPARENT_INDEX_VALUE_EXT);
			if (transparentIndexValue != val)
			    continue;
		    }
		}
	    }
	}
	if (visualType) {
	    __GLX_GCONF(GLX_X_VISUAL_TYPE_EXT);
	    if (visualTypeValue != val)
		continue;
	} else if (rgba) {
	    /* If the extension isn't specified then insure that rgba
	    ** and ci return the usual visual types.
	    */
	    if (!(thisVis->class == TrueColor || thisVis->class == DirectColor))
		continue;
	} else {
	    if (!(thisVis->class == PseudoColor 
			|| thisVis->class == StaticColor))
		continue;
	}
	    
	__GLX_GCONF(GLX_VISUAL_CAVEAT_EXT);
	/** 
	** Unrated visuals are given rating GLX_NONE.
	*/
	thisVisRating = val ? val : GLX_NONE_EXT;
	if (visualRating && (visualRatingValue != val))
	    continue;
	if (rgba) {
	    __GLX_GCONF(GLX_RED_SIZE);
	    if (redSize > val)
		continue;
	    else 
		score += ColorScore(redSize,val);
	    __GLX_GCONF(GLX_GREEN_SIZE);
	    if (greenSize > val)
		continue;
	    else 
		score += ColorScore(greenSize, val);
	    __GLX_GCONF(GLX_BLUE_SIZE);
	    if (blueSize > val)
		continue;
	    else 
		score += ColorScore(blueSize, val);
	    __GLX_GCONF(GLX_ALPHA_SIZE);
	    if (alphaSize > val)
		continue;
	    else 
		score += ColorScore(alphaSize, val);
	    __GLX_GCONF(GLX_ACCUM_RED_SIZE);
	    if (accumRedSize > val)
		continue;
	    else 
		score += AccumScore(accumRedSize, val);
	    __GLX_GCONF(GLX_ACCUM_GREEN_SIZE);
	    if (accumGreenSize > val)
		continue;
	    else 
		score += AccumScore(accumGreenSize, val);
	    __GLX_GCONF(GLX_ACCUM_BLUE_SIZE);
	    if (accumBlueSize > val)
		continue;
	    else 
		score += AccumScore(accumBlueSize, val);
	    __GLX_GCONF(GLX_ACCUM_ALPHA_SIZE);
	    if (accumAlphaSize > val)
		continue;
	    else 
		score += AccumScore(accumAlphaSize, val);
	} else {
	    __GLX_GCONF(GLX_BUFFER_SIZE);
	    if (bufferSize > val)
		continue;
	    else
		score += IndexScore(bufferSize, val);
	}
	__GLX_GCONF(GLX_DEPTH_SIZE);
	if (depthSize > val)
	    continue;
	else
	    score += DepthScore(depthSize, val);
	__GLX_GCONF(GLX_STENCIL_SIZE);
	if (stencilSize > val)
	    continue;
	else
	    score += StencilScore(stencilSize, val);

	/*
	** The visual_rating extension indicates that a NONE visual
	** is always returned in preference to a SLOW one.
	** Note that enum values are in increasing order (NONE < SLOW).
	*/
	if (maxi < 0 || maxRating > thisVisRating) {
	    maxi = i;
	    maxscore = score;
	    maxRating = thisVisRating;
	} else {
	    if (score > maxscore) {
		maxi = i;
		maxscore = score;
	    }
	}
    }

    /*
    ** If no visual is acceptable, return None
    ** Otherwise, create an XVisualInfo list with just the selected X visual
    **   and return this after freeing the original list
    */
    if (maxi < 0) {
	XFree((char *)visualList);
	return None;
    } else {
	visualTemplate.visualid = visualList[maxi].visualid;
	XFree((char *)visualList);
	visualList = XGetVisualInfo(dpy,VisualScreenMask|VisualIDMask,&visualTemplate,&count);
	return visualList;
    }
}

/*
** Query the Server GLX string and cache it in the display private.
** This routine will allocate the necessay space for the string.
*/
static char *QueryServerString( Display *dpy, int opcode,
                                        int screen, int name )
{
    xGLXQueryServerStringReq *req;
    xGLXQueryServerStringReply reply;
    int length, numbytes, slop;
    char *buf;

    /* Send the glXQueryServerString request */
    LockDisplay(dpy);
    GetReq(GLXQueryServerString,req);
    req->reqType = opcode;
    req->glxCode = X_GLXQueryServerString;
    req->screen = screen;
    req->name = name;
    _XReply(dpy, (xReply*) &reply, 0, False);

    length = reply.length;
    numbytes = reply.n;
    slop = numbytes * __GLX_SIZE_INT8 & 3;
    buf = (char *)Xmalloc(numbytes);
    if (!buf) {
        /* Throw data on the floor */
        _XEatData(dpy, length);
    } else {
        _XRead(dpy, (char *)buf, numbytes);
        if (slop) _XEatData(dpy,4-slop);
    }
    UnlockDisplay(dpy);
    SyncHandle();
    return buf;
}

#define SEPARATOR " "

static char *combine_strings( const char *cext_string, const char *sext_string )
{
   int clen, slen;
   char *combo_string, *token, *s1;
   const char *s2, *end;

   /*
   ** String can't be longer than min(cstring, sstring)
   ** pull tokens out of shortest string
   ** include space in combo_string for final separator and null terminator
   */
   if ( (clen = strlen( cext_string)) > (slen = strlen( sext_string)) ) {
        combo_string = (char *) Xmalloc( slen + 2 );
	s1 = (char *) malloc( slen + 2 ); strcpy( s1, sext_string );
        s2 = cext_string;
   } else {
        combo_string = (char *) Xmalloc( clen + 2 );
	s1 = (char *) Xmalloc( clen + 2 ); strcpy( s1, cext_string);
        s2 = sext_string;
   }
   if (!combo_string || !s1) {
	if (combo_string) Xfree(combo_string);
	if (s1) Xfree(s1);
        return NULL;
   }
   combo_string[0] = '\0';

   /* Get first extension token */
   token = strtok( s1, SEPARATOR);
   while ( token != NULL ) {

	/*
	** if token in second string then save it
	** beware of extension names which are prefixes of other extension names
	*/
	const char *p = s2;
	end = p + strlen(p);
	while (p < end) {
	    int n = strcspn(p, SEPARATOR);
	    if ((strlen(token) == n) && (strncmp(token, p, n) == 0)) {
		combo_string = strcat( combo_string, token);
		combo_string = strcat( combo_string, SEPARATOR);
	    }
	    p += (n + 1);
	}

        /* Get next extension token */
        token = strtok( NULL, SEPARATOR);
   }
   Xfree(s1);
   return combo_string;
}

const char *glXQueryExtensionsString( Display *dpy, int screen )
{
    __GLXvisualConfig *pConfig;
    __GLXscreenConfigs *psc;
    __GLXdisplayPrivate *priv;

    /* Initialize the extension, if needed .  This has the added value
       of initializing/allocating the display private */
    priv = __glXInitialize(dpy);
    if (!priv) {
	return NULL;
    }

    /* Check screen number to see if its valid */
    if ((screen < 0) || (screen >= ScreenCount(dpy))) {
	return NULL;
    }

    /* Check to see if the GL is supported on this screen */
    psc = &priv->screenConfigs[screen];
    pConfig = psc->configs;
    if (!pConfig) {
	/* No support for GL on this screen regardless of visual */
	return NULL;
    }
   
    if (!psc->effectiveGLXexts) {
        if (!psc->serverGLXexts) {
	    psc->serverGLXexts = QueryServerString(dpy, priv->majorOpcode,
					  	   screen, GLX_EXTENSIONS);
	}
	psc->effectiveGLXexts = combine_strings(GLXClientExtensions,
						psc->serverGLXexts);
    }

    return psc->effectiveGLXexts;
}

const char *glXGetClientString( Display *dpy, int name )
{
    switch(name) {
	case GLX_VENDOR:
	    return (GLXClientVendorName);
	case GLX_VERSION:
	    return (GLXClientVersion);
	case GLX_EXTENSIONS:
	    return (GLXClientExtensions);
	default:
	    return NULL;
    }
}

const char *glXQueryServerString( Display *dpy, int screen, int name )
{
    __GLXvisualConfig *pConfig;
    __GLXscreenConfigs *psc;
    __GLXdisplayPrivate *priv;

    /* Initialize the extension, if needed .  This has the added value
       of initializing/allocating the display private */
    priv = __glXInitialize(dpy);
    if (!priv) {
	/* No extension */
	return NULL;
    }

    /* Check screen number to see if its valid */
    if ((screen < 0) || (screen >= ScreenCount(dpy))) {
	return NULL;
    }

    /* Check to see if the GL is supported on this screen */
    psc = &priv->screenConfigs[screen];
    pConfig = psc->configs;
    if (!pConfig) {
	/* No support for GL on this screen regardless of visual */
	return NULL;
    }
 
    switch(name) {
	case GLX_VENDOR:
	    if (!priv->serverGLXvendor) {
	 	priv->serverGLXvendor = 
			QueryServerString(dpy, priv->majorOpcode,
					  screen, GLX_VENDOR);
	    }
	    return(priv->serverGLXvendor);
	case GLX_VERSION:
	    if (!priv->serverGLXversion) {
	 	priv->serverGLXversion = 
			QueryServerString(dpy, priv->majorOpcode,
					  screen, GLX_VERSION);
	    }
	    return(priv->serverGLXversion);
	case GLX_EXTENSIONS:
	    if (!psc->serverGLXexts) {
	 	psc->serverGLXexts = 
			QueryServerString(dpy, priv->majorOpcode,
					  screen, GLX_EXTENSIONS);
	    }
	    return(psc->serverGLXexts);
	default:
	    return NULL;
    }
}

void __glXClientInfo (  Display *dpy, int opcode  )
{
    xGLXClientInfoReq *req;
    int size;

    /* Send the glXClientInfo request */
    LockDisplay(dpy);
    GetReq(GLXClientInfo,req);
    req->reqType = opcode;
    req->glxCode = X_GLXClientInfo;
    req->major = GLX_MAJOR_VERSION;
    req->minor = GLX_MINOR_VERSION;

    size = strlen(GL_ClientExtensions) + 1;
    req->length += (size + 3) >> 2;
    req->numbytes = size;
    Data(dpy, GL_ClientExtensions, size);

    UnlockDisplay(dpy);
    SyncHandle();
}

/************************************************************************/
/*
** EXT_import_context entry points
*/
/************************************************************************/

Display *glXGetCurrentDisplay(void)
{
    GLXContext gc = __glXGetCurrentContext();
    if (NULL == gc) return NULL;
    return gc->currentDpy;
}


static int __glXQueryContextInfo(Display *dpy, GLXContext ctx)
{
    xGLXVendorPrivateReq *vpreq;
    xGLXQueryContextInfoEXTReq *req;
    xGLXQueryContextInfoEXTReply reply;
    CARD8 opcode;
    GLuint numValues;

    if (ctx == NULL) {
	return GLX_BAD_CONTEXT;
    }
    opcode = __glXSetupForCommand(dpy);
    if (!opcode) {
	return 0;
    }

    /* Send the glXQueryContextInfoEXT request */
    LockDisplay(dpy);
    GetReqExtra(GLXVendorPrivate,
	sz_xGLXQueryContextInfoEXTReq-sz_xGLXVendorPrivateReq,vpreq);
    req = (xGLXQueryContextInfoEXTReq *)vpreq;
    req->reqType = opcode;
    req->glxCode = X_GLXVendorPrivateWithReply;
    req->vendorCode = X_GLXvop_QueryContextInfoEXT;
    req->context = (unsigned int)(ctx->xid);
    _XReply(dpy, (xReply*) &reply, 0, False);
    UnlockDisplay(dpy);

    numValues = reply.n;
    if (numValues == 0) return Success;
    if (numValues > __GLX_MAX_CONTEXT_PROPS) return 0;

    {
	int *propList, *pProp;
	int nPropListBytes;
	int i;

	nPropListBytes = numValues << 3;
	propList = (int *) Xmalloc(nPropListBytes);
	if (NULL == propList) {
	    return 0;
	}
	_XRead(dpy, (char *)propList, nPropListBytes);
	pProp = propList;
	for (i=0; i < numValues; i++) {
	    switch (*pProp++) {
	    case GLX_SHARE_CONTEXT_EXT:
		ctx->share_xid = *pProp++;
		break;
	    case GLX_VISUAL_ID_EXT:
		ctx->vid = *pProp++;
		break;
	    case GLX_SCREEN_EXT:
		ctx->screen = *pProp++;
		break;
	    default:
		pProp++;
		continue;
	    }
	}
	Xfree((char *)propList);
    }
    SyncHandle();
    return Success;
}

int glXQueryContextInfoEXT(Display *dpy, GLXContext ctx, 
				int attribute, int *value)
{
    int retVal;

    /* get the information from the server if we don't have it already */
    if (!ctx->isDirect && (ctx->vid == None)) {
	retVal = __glXQueryContextInfo(dpy, ctx);
	if (Success != retVal) return retVal;
    }
    switch (attribute) {
    case GLX_SHARE_CONTEXT_EXT:
	*value = (int)(ctx->share_xid);
	break;
    case GLX_VISUAL_ID_EXT:
	*value = (int)(ctx->vid);
	break;
    case GLX_SCREEN_EXT:
	*value = (int)(ctx->screen);
	break;
    default:
	return GLX_BAD_ATTRIBUTE;
    }
    return Success;
}

GLXContextID glXGetContextIDEXT(const GLXContext ctx)
{
    return ctx->xid;
}

GLXContext glXImportContextEXT(Display *dpy, GLXContextID contextID)
{
    GLXContext ctx;

    if (contextID == None) {
	return NULL;
    }
    if (__glXIsDirect(dpy, contextID)) {
	return NULL;
    }

    ctx = CreateContext(dpy, NULL, NULL, GL_FALSE, contextID);
    if (NULL != ctx) {
	if (Success != __glXQueryContextInfo(dpy, ctx)) {
	   return NULL;
	}
    }
    return ctx;
}

void glXFreeContextEXT(Display *dpy, GLXContext ctx)
{
    DestroyContext(dpy, ctx);
}



/*
 * GLX 1.3 functions - these are just stubs for now!
 */

GLXFBConfig *glXChooseFBConfig(Display *dpy, int screen, const int *attribList, int *nitems)
{
    (void) dpy;
    (void) screen;
    (void) attribList;
    (void) nitems;
    return 0;
}


GLXContext glXCreateNewContext(Display *dpy, GLXFBConfig config, int renderType, GLXContext shareList, Bool direct)
{
    (void) dpy;
    (void) config;
    (void) renderType;
    (void) shareList;
    (void) direct;
    return 0;
}


GLXPbuffer glXCreatePbuffer(Display *dpy, GLXFBConfig config, const int *attribList)
{
    (void) dpy;
    (void) config;
    (void) attribList;
    return 0;
}


GLXPixmap glXCreatePixmap(Display *dpy, GLXFBConfig config, Pixmap pixmap, const int *attribList)
{
    (void) dpy;
    (void) config;
    (void) pixmap;
    (void) attribList;
    return 0;
}


GLXWindow glXCreateWindow(Display *dpy, GLXFBConfig config, Window win, const int *attribList)
{
    (void) dpy;
    (void) config;
    (void) win;
    (void) attribList;
    return 0;
}


void glXDestroyPbuffer(Display *dpy, GLXPbuffer pbuf)
{
    (void) dpy;
    (void) pbuf;
}


void glXDestroyPixmap(Display *dpy, GLXPixmap pixmap)
{
    (void) dpy;
    (void) pixmap;
}


void glXDestroyWindow(Display *dpy, GLXWindow window)
{
    (void) dpy;
    (void) window;
}


GLXDrawable glXGetCurrentReadDrawable(void)
{
    return 0;
}


GLXFBConfig *glXGetFBConfigs(Display *dpy, int screen, int *nelements)
{
   (void) dpy;
   (void) screen;
   (void) nelements;
   return 0;
}


int glXGetFBConfigAttrib(Display *dpy, GLXFBConfig config, int attribute, int *value)
{
    (void) dpy;
    (void) config;
    (void) attribute;
    (void) value;
    return 0;
}


void glXGetSelectedEvent(Display *dpy, GLXDrawable drawable, unsigned long *mask)
{
    (void) dpy;
    (void) drawable;
    (void) mask;
}


XVisualInfo *glXGetVisualFromFBConfig(Display *dpy, GLXFBConfig config)
{
    (void) dpy;
    (void) config;
    return 0;
}


Bool glXMakeContextCurrent(Display *dpy, GLXDrawable draw, GLXDrawable read, GLXContext ctx)
{
    (void) dpy;
    (void) draw;
    (void) read;
    (void) ctx;
    return 0;
}


int glXQueryContext(Display *dpy, GLXContext ctx, int attribute, int *value)
{
    (void) dpy;
    (void) ctx;
    (void) attribute;
    (void) value;
    return 0;
}


void glXQueryDrawable(Display *dpy, GLXDrawable draw, int attribute, unsigned int *value)
{
    (void) dpy;
    (void) draw;
    (void) attribute;
    (void) value;
}


void glXSelectEvent(Display *dpy, GLXDrawable drawable, unsigned long mask)
{
    (void) dpy;
    (void) drawable;
    (void) mask;
}



/*
** Mesa extension stubs.  These will help reduce portability problems.
*/

void glXReleaseBuffersMESA( Display *dpy, GLXDrawable d )
{
   (void) dpy;
   (void) d;
   /* no-op stub */
}


GLXPixmap glXCreateGLXPixmapMESA( Display *dpy, XVisualInfo *visual,
                                  Pixmap pixmap, Colormap cmap )
{
   (void) dpy;
   (void) visual;
   (void) pixmap;
   (void) cmap;
   return 0;
}


void glXCopySubBufferMESA( Display *dpy, GLXDrawable drawable,
                           int x, int y, int width, int height )
{
   (void) dpy;
   (void) drawable;
   (void) x;
   (void) y;
   (void) width;
   (void) height;
}


GLboolean glXSet3DfxModeMESA( GLint mode )
{
   (void) mode;
   return GL_FALSE;
}


/*
** glXGetProcAddress support
*/

struct name_address_pair {
   const char *Name;
   GLvoid *Address;
};

static struct name_address_pair GLX_functions[] = {
   { "glXChooseVisual", (GLvoid *) glXChooseVisual },
   { "glXCopyContext", (GLvoid *) glXCopyContext },
   { "glXCreateContext", (GLvoid *) glXCreateContext },
   { "glXCreateGLXPixmap", (GLvoid *) glXCreateGLXPixmap },
   { "glXDestroyContext", (GLvoid *) glXDestroyContext },
   { "glXDestroyGLXPixmap", (GLvoid *) glXDestroyGLXPixmap },
   { "glXGetConfig", (GLvoid *) glXGetConfig },
   { "glXGetCurrentContext", (GLvoid *) glXGetCurrentContext },
   { "glXGetCurrentDrawable", (GLvoid *) glXGetCurrentDrawable },
   { "glXIsDirect", (GLvoid *) glXIsDirect },
   { "glXMakeCurrent", (GLvoid *) glXMakeCurrent },
   { "glXQueryExtension", (GLvoid *) glXQueryExtension },
   { "glXQueryVersion", (GLvoid *) glXQueryVersion },
   { "glXSwapBuffers", (GLvoid *) glXSwapBuffers },
   { "glXUseXFont", (GLvoid *) glXUseXFont },
   { "glXWaitGL", (GLvoid *) glXWaitGL },
   { "glXWaitX", (GLvoid *) glXWaitX },

   { "glXGetClientString", (GLvoid *) glXGetClientString },
   { "glXQueryExtensionsString", (GLvoid *) glXQueryExtensionsString },
   { "glXQueryServerString", (GLvoid *) glXQueryServerString },

   { "glXGetCurrentDisplay", (GLvoid *) glXGetCurrentDisplay },

   { "glXChooseFBConfig", (GLvoid *) glXChooseFBConfig },
   { "glXCreateNewContext", (GLvoid *) glXCreateNewContext },
   { "glXCreatePbuffer", (GLvoid *) glXCreatePbuffer },
   { "glXCreatePixmap", (GLvoid *) glXCreatePixmap },
   { "glXCreateWindow", (GLvoid *) glXCreateWindow },
   { "glXDestroyPbuffer", (GLvoid *) glXDestroyPbuffer },
   { "glXDestroyPixmap", (GLvoid *) glXDestroyPixmap },
   { "glXDestroyWindow", (GLvoid *) glXDestroyWindow },
   { "glXGetCurrentReadDrawable", (GLvoid *) glXGetCurrentReadDrawable },
   { "glXGetFBConfigAttrib", (GLvoid *) glXGetFBConfigAttrib },
   { "glXGetFBConfigs", (GLvoid *) glXGetFBConfigs },
   { "glXGetSelectedEvent", (GLvoid *) glXGetSelectedEvent },
   { "glXGetVisualFromFBConfig", (GLvoid *) glXGetVisualFromFBConfig },
   { "glXMakeContextCurrent", (GLvoid *) glXMakeContextCurrent },
   { "glXQueryContext", (GLvoid *) glXQueryContext },
   { "glXQueryDrawable", (GLvoid *) glXQueryDrawable },
   { "glXSelectEvent", (GLvoid *) glXSelectEvent },

   /* extension functions */
   { "glXGetContextIDEXT", (GLvoid *) glXGetContextIDEXT },
   { "glXGetCurrentDrawableEXT", (GLvoid *) glXGetCurrentDrawableEXT },
   { "glXImportContextEXT", (GLvoid *) glXImportContextEXT },
   { "glXFreeContextEXT", (GLvoid *) glXFreeContextEXT },
   { "glXQueryContextInfoEXT", (GLvoid *) glXQueryContextInfoEXT },
   { "glXGetProcAddressARB", (GLvoid *) glXGetProcAddressARB },

   /* Mesa extensions */
   { "glXReleaseBuffersMESA", (GLvoid *) glXReleaseBuffersMESA },
   { "glXCreateGLXPixmapMESA", (GLvoid *) glXCreateGLXPixmapMESA },
   { "glXCopySubBufferMESA", (GLvoid *) glXCopySubBufferMESA },
   { "glXSet3DfxModeMESA", (GLvoid *) glXSet3DfxModeMESA },

   { NULL, NULL }   /* end of list */
};


static const GLvoid *
get_glx_proc_address(const char *funcName)
{
   GLuint i;
   for (i = 0; GLX_functions[i].Name; i++) {
      if (strcmp(GLX_functions[i].Name, funcName) == 0)
         return GLX_functions[i].Address;
   }
   return NULL;
}


void (*glXGetProcAddressARB(const GLubyte *procName))()
{
   typedef void (*gl_function)();
   gl_function f;

#if defined(GLX_DIRECT_RENDERING)
   __glXRegisterExtensions();
#endif

   f = (gl_function) get_glx_proc_address((const char *) procName);
   if (f) {
      return f;
   }

   f = (gl_function) _glapi_get_proc_address((const char *) procName);
   return f;
}
