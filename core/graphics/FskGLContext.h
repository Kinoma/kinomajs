/*
 *     Copyright (C) 2010-2016 Marvell International Ltd.
 *     Copyright (C) 2002-2010 Kinoma, Inc.
 *
 *     Licensed under the Apache License, Version 2.0 (the "License");
 *     you may not use this file except in compliance with the License.
 *     You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *     Unless required by applicable law or agreed to in writing, software
 *     distributed under the License is distributed on an "AS IS" BASIS,
 *     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *     See the License for the specific language governing permissions and
 *     limitations under the License.
 */
/**
	\file	FskGLContext.h
	\brief	Device-dependent Open GL Context creation.
*/
#ifndef __FSKGLCONTEXT__
#define __FSKGLCONTEXT__

#include "FskBitmap.h"


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */



struct FskGLContextRecord;										/**< Opaque    GL context data type. */
typedef struct FskGLContextRecord		FskGLContextRecord;		/**< Opaque    GL context data type. */
typedef struct FskGLContextRecord		*FskGLContext;			/**< Mutable   GL context object.    */
typedef const struct FskGLContextRecord	*FskConstGLContext;		/**< Immutable GL context object.    */

/** Storage for context. */
struct FskGLContextStorage {
	void *storage[6];											/**< Storage large enough to store the GL context on any platform. */
};
typedef struct FskGLContextStorage FskGLContextStorage;			/**< Storage large enough to store the GL context on any platform. */


/**	Create a new GL offscreen context.
 **	\param[in]	width		the desired offscreen width  (can be 0).
 **	\param[in]	height		the desired offscreen height (can be 0).
 **	\param[in]	pixelFormat	the desired pixel precision (coarse: RGB, RGBA 565).
 **	\param[in]	version		the desired Open GL version.
 **	\param[in]	share		share assets with this other context; if NULL, then no assets are shared.
 **	\param[out]	ctx			the resultant context.
 **	\return		kFskErrNone	if the operation succeeded.
 **	\note		When a context is created with dimension width=0 height=0, an FBO must be allocated if it is desired to do rendering.
 **/
FskAPI(FskErr)		FskGLOffscreenContextNew(UInt32 width, UInt32 height, FskBitmapFormatEnum pixelFormat, UInt32 version, FskGLContext share, FskGLContext *ctx);


/** Dispose of a GL Context.
 **	\param[in]	ctx			the context to be disposed. Can be NULL.
 **	\param[in]	terminateGL	if true, GL is terminated.
 **/
FskAPI(void)		FskGLContextDispose(FskGLContext ctx, Boolean terminateGL);


/** Dispose of the cross-platform storage for an Fsk GL Context.
 ** The OpenGL context is not disposed, though.
 ** This is useful to prevent memory leaks when the context is transferred
 ** and managed elsewhere, as in an FskGLBlitContext.
 **	\param[in]	ctx			the context to be disposed. Can be NULL.
 **	\return		kFskErrNone	when complete.
 **/
FskErr	FskGLContextDisposeStorage(FskGLContext ctx);
#define FskGLContextDisposeStorage(ctx)	FskMemPtrDispose(ctx)


/** Retrieve the frame buffer from the context.
 **	\param[in]	ctx		the context to be queried. NULL will always return 0.
 **	\return		the frame buffer associated with the context.
 **/
FskAPI(unsigned)	FskGLContextFrameBuffer(FskConstGLContext ctx);


/** Set the current context.
 **	\param[in]	ctx			the context to be make current. NULL implies removing the context or reverting to the default.
 **	\return		kFskErrNone	if the operation succeeded.
 **/
FskAPI(FskErr)		FskGLContextMakeCurrent(FskConstGLContext ctx);


/** Create an FskGLContext from the currently active Open GL context.
 **	\return		kFskErrNone	if the operation succeeded.
 **/
FskAPI(FskErr)		FskGLContextNewFromCurrentContext(FskGLContext *pCtx);


/**	Get the current GL context for use in a subsequent FskGLContextMakeCurrent() call.
 **	\param[in]	storage	a pointer to storage for the GL context.
 **	\param[out]	pCtx	a place to store the resultant context. This can be NULL, because we merely set *pCtx = (FskGLContext)storage;
 **	\return		kFskErrNone				if the operation completed successfully.
 **	\return		kFskErrNotAccelerated	if there is no currently active GL context. In this case, *pCtx is set to NULL.
 **	\note		Do not call FskGLContextDispose() with the value returned in pCtx; it is intended only for interfacing to FskGLContextMakeCurrent().
 **/
FskAPI(FskErr)		FskGLContextGetCurrentContext(FskGLContextStorage *storage, FskGLContext *pCtx);


/**	Swap the front and back buffers.
 ** This only has an effect if the context is double-buffered, and is a no-op when single-buffered.
 **	\param[in]	ctx			the GL context whose buffer is to be swapped.
 **	\return		kFskErrNone	if the operation completed successfully.
 **/
FskAPI(FskErr)		FskGLContextSwapBuffers(FskGLContext ctx);


#if TARGET_OS_ANDROID || TARGET_OS_KPL || defined(__linux__) || (FSK_OPENGLES_ANGLE == 1) || defined(EGL_VERSION)

	/**	Create a new FskGLContext from an EGL {display,surface,context}.
	 ** This is typically used when creating contexts that are not available through the FskGLOffscreenContextNew() API,
	 ** such as a multisample context or a window context.
	 **	\param[in]	display		the EGL display.
	 **	\param[in]	surface		the EGL surface.
	 **	\param[in]	context		the EGL context.
	 **	\param[out]	ctx			the resultant context.
	 **	\return		kFskErrNone	if the operation succeeded.
	 **/
	FskAPI(FskErr)	FskGLContextNewFromEGL(void* display, void* surface, void* context, FskGLContext *pCtx);


	/**	Get the EGL parameters from an FskGLContext.
	 ** This returns the parameters that were give to FskGLContextNewFromEGL().
	 **	\param[in]	ctx			the Fsk GL context.
	 **	\param[out]	pDisplay	a place to store the EGL display (can be NULL).
	 **	\param[out]	pSurface	a place to store the EGL surface (can be NULL).
	 **	\param[out]	pContext	a place to store the EGL context (can be NULL).
	 **/
	FskAPI(void)	FskGLContextGetEGL(FskGLContext ctx, void** pDisplay, void** pSurface, void** pContext);


	/**	Create a new GL window context.
	 **	\param[in]	nativeWindow	a pointer to the native window to be associated with this context.
	 **	\param[in]	height			the desired offscreen height (can be 0).
	 **	\param[in]	pixelFormat		the desired pixel precision (coarse: RGB, RGBA 565).
	 **	\param[in]	version			the desired Open GL version.
	 **	\param[in]	share			share assets with this other context; if NULL, then no assets are shared.
	 **	\param[out]	ctx				the resultant context.
	 **	\return		kFskErrNone		if the operation succeeded.
	 **	\todo		Make this cross-platform.
	 **/
	FskAPI(FskErr)	FskGLWindowContextNew(void *nativeWindow, FskBitmapFormatEnum pixelFormat, UInt32 version, FskGLContext share, FskGLContext *pCtx);
#endif /* EGL_VERSION */


#if TARGET_OS_MAC && !TARGET_OS_IPHONE
	#ifndef _CGLTYPES_H
		#include <OpenGL/CGLTypes.h>
	#endif /*_CGLTYPES_H */

	/**	Create a new FskGLContext from a CGL Context.
	 ** This is typically used when creating contexts that are not available through the FskGLOffscreenContextNew() API,
	 ** such as a multisample context or a window context.
	 **	\param[in]	context		the CGL context.
	 **	\param[out]	ctx			the resultant context.
	 **	\return		kFskErrNone	if the operation succeeded.
	 **/
	FskAPI(FskErr) FskGLContextNewFromCGLContext(CGLContextObj context, FskGLContext *pCtx);

	/**	Get the CGL Context object from the FskGLContext.
	 ** This returns the parameter given to FskGLContextNewFromCGLContext().
	 **	\param[in]	ctx			the Fsk GL context.
	 **	\param[out]	pContext	a place to store the CGL context.
	 **/
	FskAPI(void) FskGLContexGetCGL(FskGLContext ctx, CGLContextObj *pContext);
#endif /* TARGET_OS_MAC && !TARGET_OS_IPHONE */


#if TARGET_OS_IPHONE
	#ifdef __OBJC__
		#ifndef _EAGL_H_
			#include <OpenGLES/EAGL.h>
		#endif /* _EAGL_H_ */
	#elif !defined(__FSKCOCOASUPPORTPHONE__) /* !__OBJC__ */
		struct EAGLContext;
		typedef struct EAGLContext EAGLContext;
	#endif /* __OBJC__ */
	/**	Create a new FskGLContext from an EAGL Context.
	 ** This is typically used when creating contexts that are not available through the FskGLOffscreenContextNew() API,
	 ** such as a multisample context or a window context.
	 **	\param[in]	context		the EAGL context.
	 **	\param[out]	ctx			the resultant context.
	 **	\return		kFskErrNone	if the operation succeeded.
	 **/
	FskAPI(FskErr) FskGLContextNewFromEAGLContext(EAGLContext *context, FskGLContext *pCtx);

	/**	Get the EAGL Context from an Fsk GL context.
	 ** This returns the parameter given to FskGLContextNewFromEAGLContext().
	 **	\param[in]	ctx			the Fsk GL context.
	 **	\param[out]	pContext	a place to store the EAGL context.
	 **/
	FskAPI(void) FskGLContextGetEAGL(FskGLContext ctx, EAGLContext **pContext);
#endif /* TARGET_OS_IPHONE */



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __FSKGLCONTEXT__ */
