LIBRARY libOSmesa
VERSION LIBRARY_VERSION
EXPORTS
	OSMesaCreateContext
	OSMesaDestroyContext
	OSMesaGetColorBuffer
	OSMesaGetCurrentContext
	OSMesaGetDepthBuffer
	OSMesaGetIntegerv
	OSMesaMakeCurrent
	OSMesaPixelStore
	_glapi_Context 
	_glapi_noop_enable_warnings
	_glapi_add_entrypoint
	_glapi_get_dispatch_table_size
	_glapi_set_dispatch
	_glapi_check_multithread
	_glapi_set_context
	glTexCoordPointer
	glColorPointer
	glNormalPointer
	glVertexPointer
	glDrawElements

/* $XFree86: xc/lib/GL/mesa/src/OSmesa/OSmesa-def.cpp,v 1.1 2000/08/09 23:40:12 dawes Exp $ */
