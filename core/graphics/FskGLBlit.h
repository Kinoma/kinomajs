/*
 *     Copyright (C) 2010-2015 Marvell International Ltd.
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
	\file	FskGLBlit.h
	\brief	OpenGL and OpenGL-ES implementations of blits.
*/
#ifndef __FSKGLBLIT__
#define __FSKGLBLIT__

#include "FskGraphics.h"
#include "FskRectangle.h"
#include "FskBitmap.h"

//#define GL_TRACE

#if defined(GL_TRACE) && TARGET_OS_ANDROID
	#include "FskHardware.h"
	#include <android/log.h>																	// DEBUG turk
#endif /* defined(GL_TRACE) && TARGET_OS_ANDROID */


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct FskTextEngineRecord;
struct FskTextFormatCacheRecord;
struct FskScaleOffset;


/*********************************************************************************
 *********************************************************************************
 **									FskGLPort									**
 *********************************************************************************
 *********************************************************************************/

//struct FskGLPortRecord;										/* Opaque definition in FskBitmap.h */
//typedef struct FskGLPortRecord *FskGLPort;					/* Opaque definition in FskBitmap.h */
//typedef const struct FskGLPortRecord *FskConstGLPort;			/* Opaque definition in FskBitmap.h */
struct FskGLBlitContextRecord;									/**< Opaque definition of FskGLBlit Context Record. */
typedef struct FskGLBlitContextRecord FskGLBlitContextRecord;	/**< Type definition for FskGLBlit Context Record. */
typedef struct FskGLBlitContextRecord *FskGLBlitContext;		/**< Type definition for FskGLBlit Context Object. */


/** System-dependent procedure to call before doing GL drawing.
 *	\param[in]	sysContext	the system-dependent GL context.
 **/
typedef void (*FskGLFocusProc)(void *sysContext);


/**	System-dependent procedure to call after completing GL drawing.
 *	If drawing occurs in a back-buffer, this would swap buffers.
 *	\param[in]	sysContext	the system-dependent GL context.
 **/
typedef void (*FskGLDefocusProc)(void *sysContext);


/** Initialize GL for Fsk.
 *	This does nothing if it has already been initialized.
 *	\param[in]	v						system-specific data to get GL initialized.
 *										ANativeWindow *aWin - on android.
 *	\return		kFskErrNone				if the port was initialized either now or previously.
 *	\return		kFskErrMemFull			if there was not enough memory to allocate the port;
 *	\return		kFskErrInvalidParameter	if a bad parameter was supplied;
 *	\return		kFskErrGraphicsContext	if GL could not be initialized, perhaps because Open GL was not appropriately initialized.
 **/
FskAPI(FskErr)	FskGLInit(void *v);


/** Shut down GL for Fsk.
 *	\return		kFskErrNone		if there was no error.
 **/
FskAPI(FskErr)	FskGLShutdown(void);

#if TARGET_OS_ANDROID || TARGET_OS_LINUX || (TARGET_OS_KPL && (FSK_OPENGLES_KPL == 1)) || (defined(_MSC_VER) && (FSK_OPENGLES_ANGLE == 1))


/** Record the context generated outside of the APIs in this file.
 *	\param[in]	eglDisplay	the EGL display.
 *	\param[in]	eglSurface	the EGL surface.
 *	\param[in]	eglContext	the EGL context.
 *	\return		kFskErrNone	if the operation was successful.
 */
FskAPI(FskErr)		FskGLSetEGLContext(void *eglDisplay, void *eglSurface, void *eglContext);

/** Record the native window.
 *	\param[in]	aWin	the native window.
 *	\return		kFskErrNone	if the operation was successful.
 */
FskAPI(FskErr)		FskGLSetNativeWindow(void *aWin);

#endif /* TARGET_OS */

/** Record the native screen FBO.
 *	\param[in]	fbo	the default FBO.
 *	\return		kFskErrNone	if the operation was successful.
 */
FskAPI(FskErr)		FskGLSetDefaultFBO(unsigned fbo);

/** Allocate and initialize an Fsk GL port.
 * System specific setup needs to be already done and passed in as sysContext.
 *	\param[in]	width		The width of the port.
 *	\param[in]	height		The height of the port.
 *	\param[in]	sysContext	The system-dependent context for the port.
 *	\param[out]	port		The location into which a newly allocated and initialized GL port is stored.
 *	\return		kFskErrNone				if the port was allocated and initialized successfully;
 *	\return		kFskErrMemFull			if there was not enough memory to allocate the port;
 *	\return		kFskErrInvalidParameter	if a bad parameter was supplied;
 *	\return		kFskErrGraphicsContext	if GL was not yet initialized. Port is otherwise valid.
 **/
FskAPI(FskErr)		FskGLPortNew(UInt32 width, UInt32 height, void *sysContext, FskGLPort *port);


/** Deallocate the Fsk GL Port.
 *	\param[in]	port	the port to be deallocated.
 *	\return		kFskErrNone	if the port was deallocated successfully.
 **/
FskAPI(FskErr)		FskGLPortDispose(FskGLPort port);

/** Deallocate and nullify the Fsk GL Port.
 *	\param[in]	port	pointer to the port to be deallocated. This will be set to NULL upon return.
 *	\return		kFskErrNone	if the port was deallocated successfully.
 **/
FskAPI(FskErr)		FskGLPortDisposeAt(FskGLPort *port);


/** Get the current port (can be NULL).
 *	\return	the current port. Beware that it may be NULL.
 **/
FskAPI(FskGLPort)	FskGLPortGetCurrent(void);


/** Set the current port.
 *	\param[in]	port	the port to be used.
 **/
FskAPI(void)		FskGLPortSetCurrent(FskGLPort port);


/** Make the given port the current port, and return the previous port in its place.
 *	\param[in,out]	port	on input, the new port.
 *							on output, the old port.
 **/
FskAPI(void)		FskGLPortSwapCurrent(FskGLPort *port);


/** Swap buffers.
 *	\param[in]	port	the port.
 *	\return		kFskErrNone				if the operation completed successfully.
 				kFskErrOperationFailed	if the operation failed.
				kFskErrBadState			if the port was not properly initialized.

 **/
FskAPI(FskErr)	FskGLPortSwapBuffers(FskConstGLPort port);


/** Call this procedure when the OS window size changes.
 *	\param[in]	port	the port whose dimensions are to be set.
 *	\param[in]	width	the desired width  of the port.
 *	\param[in]	height	the desired height of the port.
 **/

FskAPI(void)	FskGLPortResize(FskGLPort port, UInt32 width, UInt32 height);


/** Get the current width and height of the given port.
 *	\param[in]	port	the port to be queried for its dimensions.
 *	\param[out]	width	the location to store the port's width.
 *	\param[out]	height	the location to store the port's height.
 **/
FskAPI(void)	FskGLPortGetSize(FskConstGLPort port, UInt32 *width, UInt32 *height);


/** Set the OpenGL sysContext.
 *	\param[in]	port		the port whose sysContext is to be set.
 *	\param[in]	sysContext	the sysContext to be associated with this port.
 *	\todo		Add			void (*focusProc)(void *sysContext),
 *							void (*defocusProc)(void *sysContext),
 *							void (*detach)(void *sysContext)
 **/
FskAPI(void)	FskGLPortSetSysContext(FskGLPort port, void *sysContext);


/** Get the OpenGL sysContext. We shouldn't need this, though.
 *	\param[in]	port	the port to be queried for its context.
 *	\return				the system context.
 **/
FskAPI(void*)	FskGLPortGetSysContext(FskConstGLPort port);


/**	Set the procedures to be called before and after drawing a frame using GL commands.
 *	Note: this is a system function, not tied to a particular port, i.e.
 *	all ports share the same focus and defocus procs.
 *	Each port, however, has its own sysContext, which is passed as a parameter.
 *	\param	focus	the procedure to be called before drawing a frame using GL commands.
 *	\param	defocus	the procedure to be called after  drawing a frame using GL commands.
 **/
void		FskGLSetFocusDefocusProcs(FskGLFocusProc focus, FskGLDefocusProc defocus);


/**	Set the procedures to be called before and after drawing a frame using GL commands.
 *	\param	pFocus		location to store the procedure to be called before drawing a frame using GL commands.
 *	\param	pDefocus	location to store the procedure to be called after  drawing a frame using GL commands.
 **/
void		FskGLGetFocusDefocusProcs(FskGLFocusProc *pFocus, FskGLDefocusProc *pDefocus);


/** Focus on this port's GL sysContext.
 *	This needs to be called before calling any of the blits.
 *	\param[in]	port	the port to focused.
 **/
void		FskGLPortFocus(FskGLPort port);

/** Defocus this port's GL sysContext.
 *	This needs to be called after calling the blits.
 *	\param[in]	port	the port to be defocused.
 **/
void		FskGLPortDefocus(FskConstGLPort port);


/** Get the source texture.
 *	\param[in]	port	the port to be queried.
 *	\return		the GL source texture associated with this port.
 *	\return		0	if there is no source texture associated with this port.
 **/
FskAPI(unsigned)	FskGLPortSourceTexture(FskConstGLPort port);


/** Query whether the port represents a GL accelerated destination.
 *	\param[in]	port	the port to be queried.
 *	\return		true	if the port can be used as a GL accelerated destination.
 *	\return		false	otherwise.
 **/
Boolean			FskGLPortIsDestination(FskConstGLPort port);


/** Set whether the port is persistent, i.e. retains its value from frame to frame when used as a destination.
 *	\param[in]	port	the port to be set.
 *	\return		kfskErrNone	if the operation was completed successfully.
 **/
FskErr			FskGLPortSetPersistent(FskGLPort port, Boolean value);


/** Query whether the port is persistent, i.e. retains its value from frame to frame when used as a destination.
 *	\param[in]	port	the port to be queried.
 *	\return		true	if the port is persistent.
 *	\return		false	otherwise.
 **/
Boolean			FskGLPortIsPersistent(FskConstGLPort port);


/*********************************************************************************
 *********************************************************************************
 **							FskGLBlitContext									**
 *********************************************************************************
 *********************************************************************************/

struct FskGLContextRecord;


/** Initialize the Fsk GL Blit Context from the given GL Context.
 *	\param[in]	glContext		the GL Context.
 **/
FskAPI(void) FskGLBlitContextInitDefault(struct FskGLContextRecord *glContext);


/** Create a Blit Context from an existing GL Context.
 *	The blit context is used to minimize calls to Open GL by caching a local copy of the GL state.
 *	\param[in]	glContext		the GL Context.
 *	\param[out]	pBlitContext	pointer to a place to store the new Blit Context.
 *	\return		kFskErrNone		if the operation was successful.
 *	\note		the current context is not changed.
 **/
FskAPI(FskErr)	FskGLBlitContextNew(struct FskGLContextRecord *glContext, FskGLBlitContext *pBlitContext);


/** Create a Blit Context from the current native Context.
 *	This creates both a new GL context as well as a new Blit context.
 *	When disposed, the GL context is disposed as well.
 *	The blit context is used to minimize calls to Open GL by caching a local copy of the GL state.
 *	\param[out]	pBlitContext	pointer to a place to store the new Blit Context.
 *	\return		kFskErrNone		if the operation was successful.
 *	\note		the current context is not changed.
 **/
FskAPI(FskErr)	FskGLBlitContextNewFromCurrent(FskGLBlitContext *pBlitContext);


/** Create a Blit Context from scratch.
 *	The blit context is used to minimize calls to Open GL by caching a local copy of the GL state.
 *	\param[in]	width			the desired width  of the offscreen context.
 *	\param[in]	height			the desired height of the offscreen context.
 *	\param[in]	pixelFormat		the desired format of the offscreen context.
 *	\param[in]	share			another context, whose resources are to be shared; NULL implies resources are not shared.
 *	\param[out]	pBlitContext	pointer to a place to store the new Blit Context.
 *	\return		kFskErrNone		if the operation was successful.
 *	\note		the current context is not changed.
 **/
FskAPI(FskErr)	FskGLBlitOffscreenContextNew(UInt32 width, UInt32 height, FskBitmapFormatEnum pixelFormat, UInt32 version, FskGLBlitContext share, FskGLBlitContext *pBlitContext);


/** Delete a Blit Context.
 *	\param[in]	blitContext		the Blit Context to be disposed.
 *	\return		kFskErrNone		if the operation was successful.
 *	\note		This disposes the GL context if the blit context was created with FskGLBlitOffscreenContextNew, but not with FskGLBlitContextNew.
 *	\note		The default Blit context cannot be disposed; it is disposed instead with FskGLShutdown().
 *	\note		If the context-to-be-disposed is the current context, then the default context becomes current.
 **/
FskAPI(FskErr)	FskGLBlitContextDispose(FskGLBlitContext blitContext);


/** Make the specified Blit Context current.
 *	This also sets the GL context.
 *	\param[in]	blitContext		the Blit Context to be made current.
 *	\return		kFskErrNone		if the operation was successful.
 **/
FskAPI(FskErr)	FskGLBlitContextMakeCurrent(FskGLBlitContext blitContext);


/** Get the current Blit Context.
 *	\return		the current blit context.
 **/
FskAPI(FskGLBlitContext) FskGLBlitContextGetCurrent(void);


/** Swap the Blit context. This also swaps the GL context.
 *	\param[in,out]	pBlitContext	on  input, the pointer contains the new Blit Context;
 *									on output, the pointer contains the previous Blit Context.
 *	\return	kFskErrNone		if the operation was successful.
 *	\note	The sequence
 *				oldContext = newContext;
 *				FskGLBlitContextSwapCurrent(&oldContext);
 *			is equivalent to
 *				FskGLBlitContextGetCurrent(&oldContext);
 *				FskGLBlitContextMakeCurrent(newContext);
 **/
FskAPI(FskErr)	FskGLBlitContextSwapCurrent(FskGLBlitContext *pBlitContext);


/*********************************************************************************
 *********************************************************************************
 **							Utility Functions									**
 *********************************************************************************
 *********************************************************************************/


/** Get the corresponding GL format and type from the Fsk Pixel Format.
 *	\param[in]	format				the fsk pixel format.
 *	\param[out]	glFormat			the location to store the corresponding GL format (can be NULL).
 *	\param[out]	glType				the location to store the corresponding GL type (can be NULL).
 *	\param[out]	glInternalFormat	the location to store the corresponding GL internal format (can be NULL).
 **/
FskAPI(FskErr)		FskGLGetFormatAndTypeFromPixelFormat(FskBitmapFormatEnum format, unsigned *glFormat, unsigned *glType, int *glInternalFormat);


/** Given an error as returned from glGetError(), convert it into an FskErr code.
 *	\param[in]	glErr	the GL error code.
 *	return		the corresponding fsk error code.
 **/
FskAPI(FskErr)		FskErrorFromGLError(unsigned glErr);


/** Shortcut to set the GL viewport and orthographic camera matrix.
 *	\param[in]	port	the port, which contains its width and height.
 *	\return		kFskErrNone	if the view was set successfully.
 **/
FskAPI(FskErr)		FskGLSetGLView(FskGLPort port);


/*********************************************************************************
 *********************************************************************************
 **									Textures									**
 *********************************************************************************
 *********************************************************************************/

/** Maximum texture dimension.
 *	\return	the maximum allowable texture dimension for either width or height.
 */
FskAPI(UInt32)		FskGLMaximumTextureDimension(void);


/** Mark the given bitmap for acceleration with OpenGL.
 *	No texture is actually loaded.
 *	\param[in,out]	bm				the bitmap.
 *	\param[in]		isAccelerated	if true, the bitmap is marked for acceleration.
 *									if false, the bitmap is not marked for acceleration.
 *	\return			kFskErrNone				if the operation was successful.
 *	\return			kFskErrTextureTooLarge	if the bitmap was too large, and was clipped.
 */
FskAPI(FskErr)		FskGLSetBitmapSourceIsAccelerated(FskBitmap bm, Boolean isAccelerated);


/** Swap the textures associated with each bitmap.
 *	Typically the fr bitmap has a port, and the to bitmap does not.
 *	In this case, the texture is transferred from one bitmap to another.
 *	\param[in,out]	fr			one bitmap.
 *	\param[in,out]	to			another.
 *	\param[in]		update		Upload the textures from their bitmaps.
 *	\return			kFskErrNone				if the operation was successful.
 *	\return			kFskErrInvalidParameter	if either bitmap was NULL.
 */
FskAPI(FskErr)		FskGLPortBitmapSwap(FskBitmap fr, FskBitmap to, Boolean update);


/** Accelerate the given bitmap with OpenGL.
 *	This will upload the image to a GL texture.
 *	\param[in,out]	bm			the bitmap.
 *	\return			kFskErrNone				if the operation was successful.
 *	\return			kFskErrTextureTooLarge	if the bitmap was too large, and was clipped.
 */
FskAPI(FskErr)		FskGLAccelerateBitmapSource(FskBitmap bm);


/** Deaccelerate the given bitmap.
 *	\param[in,out]	bm			the bitmap.
 *	\return			kFskErrNone	if the operation was successful.
 */
FskAPI(FskErr)		FskGLDeaccelerateBitmapSource(FskBitmap bm);


/** Update the GL Texture associated with the bitmap.
 *	\param[in,out]	bm			the bitmap.
 *	\return			kFskErrNone				if the operation was successful.
 *	\return			kFskErrTextureTooLarge	if the bitmap was too large, and was clipped.
 */
FskAPI(FskErr)		FskGLUpdateSource(FskConstBitmap bm);


/** Unload the texture from the GPU, but flag it to load when used.
 *	\param[in,out]	bm			the bitmap.
 *	\return			kFskErrNone				if the operation was successful.
 *	\return			kFskErrInvalidParameter	if the bitmap was bad
 *	\return			kFskErrNotAccelerated	if the bitmap is not accelerated.
 */
FskAPI(FskErr)		FskGLUnloadTexture(FskBitmap bm);


/** Update the texture of one bitmap with the of another bitmap.
 *	\param[out]		texBM		the bitmap whose texture is to be updated.
 *								This should be of the format kFskBitmapFormatGLRGBA.
 *	\param[in]		bm			initialize the texture from this bitmap.
 *	\return			kFskErrNone	if the operation was successful.
 *	\note			It is undefined what will happen if texBM is an accelerated bitmap
 *					of a format other than kFskBitmapFormatGLRGBA.
 */
FskAPI(FskErr)		FskGLUpdateTextureFromBitmap(FskBitmap texBM, FskConstBitmap bm);


/*********************************************************************************
 *********************************************************************************
 **										Queries									**
 *********************************************************************************
 *********************************************************************************/

/** Capabilities of OpenGL. **/
typedef struct FskGLCapabilitiesRecord {
	const char	*vendorStr;			/**< Vendor string,			e.g. "Marvell Technology Group Ltd"	*/
	const char	*versionStr;		/**< Version string,		e.g. "OpenGL ES-CM 1.1"				*/
	const char	*rendererStr;		/**< Renderer string,		e.g. "GC860 Graphics Engine"		*/
	const char	*glslVersion;		/**< GLSL version string,	e.g. NULL							*/
	UInt32		numCapabilities;	/**< The number of capabilities. */
	const char *capabilities[1];	/**< The capabilities.           */
} FskGLCapabilitiesRecord;
typedef       struct FskGLCapabilitiesRecord *FskGLCapabilities;		/**< FskGLCapabilities object */
typedef const struct FskGLCapabilitiesRecord *FskConstGLCapabilities;	/**< const FskGLCapabilities object */


/** Query the capabilities of GL.
 *	\param[out]	pCaps		a place to store the capabilities.
 *	\return		kFskErrNone				if there were no problems.
 *	\return		kFskErrGraphicsContext	if GL was not yet initialized.
 */
FskAPI(FskErr)	FskGLCapabilitiesGet(FskGLCapabilities *pCaps);

/** Deallocate storage allocated for the FskGLCapabilities.
 *	\param	caps		the item to deallocate.
 *	\return	kFskErrNone	if there were no errors.
 */
FskAPI(FskErr)	FskGLCapabilitiesDispose(FskGLCapabilities caps);
/** Deallocate storage allocated for the FskGLCapabilities.
 *	\param	caps		the item to deallocate.
 */
#define			FskGLCapabilitiesDispose(caps)	FskMemPtrDispose(caps)


/** Query as to whether GL has the requested capability.
 *	\param[in]	caps		the capabilities of this version of GL.
 *	\param[in]	queryCap	the capability that is being queried about.
 *	\return		true		if the capability is found in teh capabilities list.
 *	\return		false		if the capability is not foing in the list.
 */
FskAPI(Boolean)	FskGLCapabilitiesHas(FskConstGLCapabilities caps, const char *queryCap);


/** Returns the pixel formats that are supported as sources.
 *	In the arguments are returned a bitfield with each bit corresponding to the pixel format that is supported, e.g.
 *			1 << kFskBitmapFormatYUV420,
 *			1 << kFskBitmapFormatUYVY,
 *	\param[out]	formats[0]	the fastest formats,        supported directly in hardware.
 *	\param[out]	formats[1]	the second fastest formats, implemented as shaders;                 also includes formats[0].
 *	\param[out]	formats[2]	the third fastest formats,  utilizing lossless conversion in-place; also includes formats[1].
 *	\param[out]	formats[3]	the slowest formats,        requiring memory allocation;            also includes formats[2].
 */
FskAPI(void)	FskGLSourceTypes(UInt32 formats[4]);


/*********************************************************************************
 *********************************************************************************
 **									Export to sibling APIs						**
 *********************************************************************************
 *********************************************************************************/



/** Read pixels from the GL frame buffer.
 *	Depending on the characteristics of the destination bitmap, this goes either to a texture on the GPU or a bitmap buffer on the CPU.
 *
 * Transfer to a texture on the GPU:
 *		If the format of the dstBM is kFskBitmapFormatGLRGBA, then the screen data is transferred to its associated texture on the GPU.
 *		If the format is kFskBitmapFormat32RGBA, but is accelerated, the screen data is transferred to its associated texture on the GPU;
 *		this holds true for accelerated kFskBitmapFormat32BGRA, kFskBitmapFormat32ARGB, kFskBitmapFormat32ABGR as well.
 *		The kFskBitmapFormatGLRGB pixel format is not supported at all, and may go away.
 * Transfer to a bitmap bits buffer on the CPU:
 *		If the bitmap does not have a texture (i.e. is not accelerated), then the screen data is transferred to the bits buffer of the bitmap.
 *		kFskBitmapFormat32RGBA is always supported, and is fastest.
 *		kFskBitmapFormat32BGRA is sometimes supported and can be almost as fast as kFskBitmapFormat32RGBA.
 *		Other 32-bit formats are pretty fast but are slower than kFskBitmapFormat32RGBA and kFskBitmapFormat32BGRA.
 *		8, 16, and 24 bit formats need to allocate a temporary buffer, so take extra memory and time.
 *
 *	\param[in]	glPort		the GL port to be used as a source.
 *	\param[in]	backBuffer	whether the backbuffer or frontbuffer is to be retrieved (ignored)
 *	\param[in]	srcRect		the region to copy from the frame buffer
 *	\param[out]	dstBM		the bitmap where the data is to be stored.
 *	\return		kFskErrNone	if the operation executed successfully.
 */
FskAPI(FskErr) FskGLPortPixelsRead(FskGLPort glPort, Boolean backBuffer, FskConstRectangle srcRect, FskBitmap dstBM);


/** Query whether it is recommended to render directly to a texture.
 *	Some implementations are not trustworthy.
 *	\return			false if it is not recommended to render directly to a texture.
 */
FskAPI(Boolean) FskGLRecommendRenderingToTexture(void);


/** Set up a bitmap with a texture that can be used as a rendering destination.
 *	If the bitmap currently has no texture, one is created of the same size as the bitmap.
 *	No initialization is done.
 *	\param[in]	bm			The bitmap whose texture is to become the new framebuffer.
 *	\param[in]	renderable	If true,  assures that the bitmap has a texture and that it is renderable.
 *							If false, makes the bitmap texture unrenderable. No textures are deallocated.
 *	\return		kFskErrNone	if successful.
 */
FskAPI(FskErr) FskGLBitmapTextureSetRenderable(FskBitmap bm, Boolean renderable);


/** Render to a texture rather than the frame buffer.
 *	If the bitmap currently has no texture, one is created of the same size as the bitmap.
 *	No initialization is done.
 *	\param[in]	bm	The bitmap whose texture is to become the new framebuffer;
 *					if NULL, restores rendering to the normal frame buffer.
 *	\return			kFskErrNone	if successful.
 */
FskAPI(FskErr)	FskGLRenderToBitmapTexture(FskBitmap bm, FskConstColorRGBA clear);


/** Use the specified GPU program.
 *	\param[in]	id	the identification number for the chosen GPU program.
 *	\return		kFskErrNone	if the operation was successful.
 */
FskAPI(FskErr) FskGLUseProgram(unsigned int id);


/** Get a pointer to our global Open GL coordinate buffer.
 *	\param[out]		coordinates			pointer to a place to store the coordinate pointer.
 *	\param[in,out]	coordinatesBytes	pointer to a place to store the number of bytes allocated for the coordinate buffer.
 *										If coordinates==NULL and coordinatesBytes!=NULL, then FskGLGetCoordinatePointer verifies that
 *										at least *coordinatesBytes is allocated for the buffer, reallocating if necessary.
 *	\bug		There is no way to increase the number of bytes in the coordinate pointer
 *	\return		kFskErrNone	if the operation was successful.
 */
FskAPI(FskErr) FskGLGetCoordinatePointer(float **coordinates, UInt32 *coordinatesBytes);



/*********************************************************************************
 *********************************************************************************
 **										Blits									**
 *********************************************************************************
 *********************************************************************************/


/** Fill the rectangle with the given color.
 *	\param[in]	dstBM		a proxy destination bitmap.
 *	\param[in]	r			the rectangle to be filled.
 *	\param[in]	color		the color to which the rectangle should be cleared.
 *	\param[in]	mode		the composition mode.
 *	\param[in]	modeParams	the mode parameters (may be NULL).
 *	\return		kFskErrNone		if the rectangle was filled successfully.
 **/
FskAPI(FskErr)	FskGLRectangleFill(FskBitmap dstBM, FskConstRectangle r, FskConstColorRGBA color, UInt32 mode, FskConstGraphicsModeParameters modeParams);


/** Copy the specified rect of the src to the specified rect of the dst, stretching to do so.
 *	\param[in]	srcBM		the source bitmap to be drawn.
 *	\param[in]	srcRect		an optional subrectangle of the source to be copied (may be NULL).
 *	\param[in]	dstBM		the proxy destination bitmap (unused).
 *	\param[in]	dstRect		the rectangle in the destination where the source should be stretched and copied.
 *	\param[in]	dstClip		an optional rectangle that to restrict the changes (may be NULL).
 *	\param[in]	opColor		a color that is used when the mode specifies colorize.
 *	\param[in]	mode		the mode.  The kFskGraphicsModeBilinear flag may be ORed with either of
 *							{kFskGraphicsModeCopy, kFskGraphicsModeAlpha, or kFskGraphicsModeColorize}.
 *	\param[in]	modeParams	additional parameters, sometimes needed for some of the modes.
 *	\return		kFskErrNone	if the operation was successful.
 **/
FskAPI(FskErr)	FskGLBitmapDraw(
	FskConstBitmap					srcBM,
	FskConstRectangle				srcRect,	/* Map this rect... */
	FskBitmap						dstBM,
	FskConstRectangle				dstRect,	/* ...to this rect */
	FskConstRectangle				dstClip,	/* But clip thus-wise */
	FskConstColorRGBA				opColor,
	UInt32							mode,
	FskConstGraphicsModeParameters	modeParams	/* We get blend level from here */
);


/** Copy the specified rect of the src to the specified rect of the dst, stretching to do so.
 *	\param[in]	srcBM		the source bitmap to be drawn.
 *	\param[in]	srcRect		an optional subrectangle of the source to be copied (may be NULL).
 *	\param[in]	dstBM		the proxy destination bitmap (unused).
 *	\param[in]	dstClip		an optional rectangle that to restrict the changes (may be NULL).
 *	\param[in]	scaleOffset	the scale and offset applied to the image.
 *							Note that scale has kFskScaleBits fractional bits, offset has kFskOffsetBits==16 fractional bits.
 *	\param[in]	opColor		a color that is used when the mode specifies colorize.
 *	\param[in]	mode		the mode.  The kFskGraphicsModeBilinear flag may be ORed with either of
 *							{kFskGraphicsModeCopy, kFskGraphicsModeAlpha, or kFskGraphicsModeColorize}.
 *	\param[in]	modeParams	additional parameters, sometimes needed for some of the modes.
 *	\return		kFskErrNone	if the operation was successful.
 **/
FskAPI(FskErr)	FskGLScaleOffsetBitmap(
	FskConstBitmap					srcBM,
	FskConstRectangle				srcRect,
	FskBitmap						dstBM,
	FskConstRectangle				dstClip,
	const struct FskScaleOffset 	*scaleOffset,
	FskConstColorRGBA				opColor,
	UInt32							mode,
	FskConstGraphicsModeParameters	modeParams	/* blend level tint color */
);


/** Copy the specified rectangle of the source to the destination, tiling if the source is too small to fill the dstRect.
 *	\param[in]	srcBM			Source bitmap
 *	\param[in]	srcClip			Bounds of source bitmaps to be used.
 *	\param[out]	dstBM			Destination bitmap.
 *	\param[in]	dstRect			Bounds to tile in destination
 *	\param[in]	dstClip			Clip of destination, incorporating source clip.
 *	\param[in]	scale			source tile scale.
 *	\param[in]	opColor			Operation color used if needed for the given transfer mode.
 *	\param[in]	mode			Transfer mode, incorporating quality.
 *	\param[in]	modeParams		We get blend level from here.
 *	\return		kFskErrNone		if the operation was successful.
 *	\bug		This differs too much from the software version. It was necessary to reduce correlation down to 0.9 in order for tests to pass.
 *	\todo		Adjust scale factors so that\n
 *				(1) the texture is not interpolated out of bounds,\n
 *				(2) the scale matches as closely to that of the software as possible, and\n
 *				(3) there are no gaps in the tiling.
 **/
FskAPI(FskErr)	FskGLTileBitmap(
	FskConstBitmap					srcBM,
	FskConstRectangle				srcClip,
	FskBitmap						dstBM,
	FskConstRectangle				dstRect,
	FskConstRectangle				dstClip,
	FskFixed						scale,
	FskConstColorRGBA				opColor,
	UInt32							mode,
	FskConstGraphicsModeParameters	modeParams
);


/** Use the given alpha map to modulate the color and apply to the destination.
 *	\param[in]	srcBM		the source bitmap. Must be pixel format 8G or 8A.
 *	\param[in]	srcRect		portion of the source bitmap to be transferred (may be NULL).
 *	\param[in]	dstBM		the proxy destination bitmap.
 *	\param[in]	dstLocation	the location where the src is to be copied
 *	\param[in]	dstClip		the destination clipping rectangle (may be NULL).
 *	\param[in]	fgColor		the color that is given to the source pixels with full value.
 *	\param[in]	modeParams	additional mode parameters.
 *	\return		kFskErrNone				if the operation was successful.
 *	\return		kFskErrNothingRendered	if the intersection of the various regions yields a NULL region.
**/
FskAPI(FskErr)	FskGLTransferAlphaBitmap(
	FskConstBitmap					srcBM,
	FskConstRectangle				srcRect,
	FskBitmap						dstBM,
	FskConstPoint					dstLocation,
	FskConstRectangle				dstClip,
	FskConstColorRGBA				fgColor,
	FskConstGraphicsModeParameters	modeParams
);


/** Draw the text in the specified color in the specified dstRect.
 *	\param[in]	fte			the font text engine.
 *	\param[in]	dstBM		the proxy destination bitmap.
 *	\param[in]	text		the text to be drawn (UTF-8?).
 *	\param[in]	textLen		the length of the text string.
 *	\param[in]	dstRect		the rectangle where the text is to be drawn.
 *	\param[in]	clipRect	a rectangle that can be used to clip some of the text.
 *	\param[in]	color		the desired color of the text.
 *	\param[in]	blendLevel	the opacity of the text (can also be specified with the alpha component of the color.
 *	\param[in]	textSize	the size of the text, in pixels.
 *	\param[in]	textStyle	the style of the text.
 *	\param[in]	hAlign		the horizontal alignment of the text.
 *	\param[in]	vAlign		the vertical  alignment of the text.
 *	\param[in]	fontName	the name of the font desired.
 *	\param[in]	cache		the cache. Can be NULL.
 *	\return		kFskErrNone	if the text were rendered without problems.
 **/
FskAPI(FskErr)	FskGLTextBox(
	struct FskTextEngineRecord	*fte,
	FskBitmap			dstBM,
	const char			*text,
	UInt32				textLen,
	FskConstRectangle	dstRect,
	FskConstRectangle	clipRect,
	FskConstColorRGBA	color,
	UInt32				blendLevel,
	UInt32				textSize,
	UInt32				textStyle,
	UInt16				hAlign,
	UInt16				vAlign,
	const char			*fontName,
	struct FskTextFormatCacheRecord	*cache
);


/** Assure that the glyphs of the given text have their strikes in the typeface bitmap.
 *	\param[in]	fte			the font text engine.
 *	\param[in]	text		the text to be drawn (UTF-8?).
 *	\param[in]	textLen		the length of the text string.
 *	\param[in]	textSize	the size of the text, in pixels.
 *	\param[in]	textStyle	the style of the text.
 *	\param[in]	fontName	the name of the font desired.
 *	\param[in]	cache		the cache. Can be NULL.
 *	\return		kFskErrNone	if the text glyphs were loaded without problems.
 **/
FskAPI(FskErr) FskGLTextGlyphsLoad(
	struct FskTextEngineRecord		*fte,
	const char						*text,
	UInt32							textLen,
	UInt32							textSize,
	UInt32							textStyle,
	const char						*fontName,
	struct FskTextFormatCacheRecord	*cache
);


/** Upload all text glyph bitmaps to their respective textures.
 *	\return	kFskErrNone	if the operation was successful.
 */
FskAPI(FskErr) FskGLTextGlyphsFlush(void);


/** Upload the glyph bitmap to the texture.
 *	\param[in]	fte			the font text engine.
 *	\param[in]	textSize	the size of the text, in pixels.
 *	\param[in]	textStyle	the style of the text.
 *	\param[in]	fontName	the name of the font desired.
 *	\param[in]	cache		the cache. Can be NULL.
 *	\return		kFskErrNone	if the text glyphs were uploaded to a texture without problems.
 **/
FskAPI(FskErr) FskGLTextGlyphsUpdateTexture(
	struct FskTextEngineRecord		*fte,
	UInt32							textSize,
	UInt32							textStyle,
	const char						*fontName,
	struct FskTextFormatCacheRecord	*cache
);


/** Draw the text in the specified color in the specified dstRect.
 *	\param[in]		fte			the font text engine.
 *	\param[in,out]	bits		scratch bitmap.
 *	\param[in]		text		the text to be drawn (UTF-8?).
 *	\param[in]		textLen		the length of the text string.
 *	\param[in]		textSize	the size of the text, in pixels.
 *	\param[in]		textStyle	the style of the text.
 *	\param[in]		fontName	the name of the font desired.
 *	\param[out]		bounds		the bounds of the text.
 *	\param[in]		cache		the cache. Can be NULL.
 *	\return			kFskErrNone	if the text were rendered without problems.
 **/
FskAPI(FskErr) FskGLTextGetBounds(
	struct FskTextEngineRecord	*fte,
	FskBitmap			bits,
	const char			*text,
	UInt32				textLen,
	UInt32				textSize,
	UInt32				textStyle,
	const char			*fontName,
	FskRectangle		bounds,
	struct FskTextFormatCacheRecord	*cache
);


/** Transform the image by the given perspective transformation.
 *	\param[in]	srcBM			the source to be transformed.
 *	\param[in]	numPoints		the number of points in the source polygon. If equal to zero, implies the four corners of the source.
 *	\param[in]	points			the points that make up the source polygon. If NULL, implies the four corners of the  source.
 *	\param[in]	dstBM			a proxy destination bitmap.
 *	\param[in]	dstClip			the destination clipping rectangle (may be NULL).
 *	\param[in]	M				the perspective matrix.
 *	\param[in]	opColor			Operation color used if needed for the given transfer mode.
 *	\param[in]	mode			Transfer mode, incorporating quality.
 *	\param[in]	modeParams		We get blend level from here.
 *	\return		kFskErrNone	if the rendering completed successively.
 */
FskAPI(FskErr)	FskGLPerspectiveTransformBitmap(
	FskConstBitmap					srcBM,
	UInt32							numPoints,
	const FskFixedPoint2D			*points,
	FskBitmap						dstBM,
	FskConstRectangle				dstClip,
	const float						M[3][3],
	FskConstColorRGBA				opColor,
	UInt32							mode,
	FskConstGraphicsModeParameters	modeParams
);


/** Transform the image by the given perspective transformation.
 *	\param[in]	srcBM			the source to be transformed.
 *	\param[in]	srcRect			The subrect of the source to be transformed. If NULL, implies the full  source.
 *	\param[in]	dstBM			a proxy destination bitmap.
 *	\param[in]	dstClip			the destination clipping rectangle (may be NULL).
 *	\param[in]	M				the perspective matrix.
 *	\param[in]	opColor			Operation color used if needed for the given transfer mode.
 *	\param[in]	mode			Transfer mode, incorporating quality.
 *	\param[in]	modeParams		We get blend level from here.
 *	\return		kFskErrNone	if the rendering completed successively.
 */
FskAPI(FskErr)	FskGLProjectBitmap(
	FskConstBitmap					srcBM,
	FskConstRectangle				srcRect,
	FskBitmap						dstBM,
	FskConstRectangle				dstClip,
	const float						M[3][3],
	FskConstColorRGBA				opColor,
	UInt32							mode,
	FskConstGraphicsModeParameters	modeParams
);


/** A record to hold typeface and cache information. */
struct FskGLTypeFaceRecord;
typedef struct FskGLTypeFaceRecord FskGLTypeFaceRecord;			/**< TypeFace record type. */
typedef struct FskGLTypeFaceRecord *FskGLTypeFace;				/**< TypeFace pointer type. */
typedef const struct FskGLTypeFaceRecord *FskConstGLTypeFace;	/**< const TypeFace pointer type. */


/** Allocate a new GL typeface cache.
 *	\param[in]	fontName	the name of the font.
 *	\param[in]	textSize	the size of the text, in pixels.
 *	\param[in]	textStyle	text style.
 *	\param[in]	fte			text engine.
 *	\param[in]	cache		text cache.
 *	\param[out]	pTypeFace	pointer to a location to store the typeface.
 *	\return		kFskErrNone	if the allocation and initialization was successful.
 */
FskAPI(FskErr) FskGLTypeFaceNew(const char *fontName, UInt32 textSize, UInt32 textStyle, struct FskTextEngineRecord *fte, struct FskTextFormatCacheRecord *cache, FskGLTypeFace *pTypeFace);


/** Dispose a GL typeface cache.
 *	\param[in]	typeFace	the typeface cache to be disposed.
 */
FskAPI(void) FskGLTypeFaceDispose(FskGLTypeFace typeFace);


/** Get the first typeface.
 *	\param[out]		pTypeFace	pointer to a location to store a typeface iterator.              Must be non-NULL.
 *								This is set to the first typeface, or NULL if there are no typefaces.
 *	\param[out]		pFontName	pointer to a location to store a pointer to the typeface's font name (can be NULL).
 *	\param[out]		pTextSize	pointer to a location to store the typeface's text size              (can be NULL).
 *	\param[out]		pTextStyle	pointer to a location to store the typeface's text style             (can be NULL).
 *	\return			kFskErrNone				if there was at least one typeface.
 *	\return			kFskErrIteratorComplete	if no typefaces were found.
 */
FskAPI(FskErr)	FskGLTypeFaceFirst(FskConstGLTypeFace *pTypeFace, const char **pFontName, UInt32 *pTextSize, UInt32 *pTextStyle);


/** Iterate through the typefaces.
 *	This and FskGLTypeFaceFirst() can be used to manage the typeface cache size.
 *	\param[in,out]	pTypeFace	pointer to a location to store a typeface iterator.              Must be non-NULL.
 *								This is updated to the next typeface, or NULL if there are no more typefaces.
 *	\param[out]		pFontName	pointer to a location to store a pointer to the typeface's font name (can be NULL).
 *	\param[out]		pTextSize	pointer to a location to store the typeface's text size              (can be NULL).
 *	\param[out]		pTextStyle	pointer to a location to store the typeface's text style             (can be NULL).
 *	\return			kFskErrNone				if another typeface  was  found.
 *	\return			kFskErrIteratorComplete	if no more typefaces were found.
 */
FskAPI(FskErr)	FskGLTypeFaceNext(FskConstGLTypeFace *pTypeFace, const char **pFontName, UInt32 *pTextSize, UInt32 *pTextStyle);


/** Add a range of glyph strikes to the GL typeface cache.
 *	\param[in]		firstCodePoint	the first unicode codepoint.
 *	\param[in]		lastCodePoint	the last unicode codepoint.
 *	\param[in,out]	typeFace		the typeface and related state.
 *	\return			kFskErrNone		if the strike was added successfully.
 **/
FskAPI(FskErr) FskGLTextStrikeGlyphRange(UInt16 firstCodePoint, UInt16 lastCodePoint, FskGLTypeFace typeFace);


/** Get the text strike bitmap.
 *	\param[in]	typeFace	the GL typeface to be queried.
 *	\param[out]	cellSize	the size of the cells used in the typeface.
 *	\return		the text strike bitmap, if successful;
 *	\return		NULL, otherwise.
 */
FskAPI(FskConstBitmap) FskGLTextStrikeBitmap(FskConstGLTypeFace typeFace, FskPoint cellSize);


/** Query whether GL_NEAREST is allowed for filtering.
 *	\return		true	if requests for GL_NEAREST is respected;
 *	\return		false	if requests for GL_NEAREST is ignored, and GL_LINEAR is used everywhere.
 */
FskAPI(Boolean) FskGLAllowNearestBitmap(void);


/** Set whether GL_NEAREST is allowed for filtering.
 *	\param		allow	if true, allows GL_NEAREST; otherwise GL_LINEAR is always used.
 */
FskAPI(void) FskGLSetAllowNearestBitmap(Boolean allow);


/** Print the OpenGL State.
 *	Useful for debugging.
 **/
void FskPrintGLState(void);


/** Release GL resources.
 *	\param[in]	which	OR of { kFskGLResourceTypefaces, kFskGLResourceBackedTextures, kFskGLResourceUnbackedTextures, kFskGLResourceAll }.
 *	\return		the last GL error, converted to an FskError. This may be caused by a GL call in the remote past, since it is a sticky error.
 **/
FskAPI(FskErr) FskGLReleaseResources(UInt32 which);

#define kFskGLResourceTypefaces			(1 << 0)		/**< Typeface textures and bitmaps. */
#define kFskGLResourceBackedTextures	(1 << 1)		/**< Textures that have a backing bitmap. */
#define kFskGLResourceUnbackedTextures	(1 << 2)		/**< All textures, including those that do not have a backing bitmap. */
#define kFskGLResourceAll				((UInt32)(-1))	/**< All resources. */


/** Estimate the amount of memory used by textures that we track.
 *	This is a lower bound, since we assume that 1 byte is allocated for each component.
 *	\return		the estimate.
 **/
FskAPI(UInt32) FskGLEstimateTextureMemoryUsage(void);


#ifdef __FSKGLBLIT_PRIV__
	FskAPI(FskErr)			FskGLBindBMTexture(FskConstBitmap bm, int wrap, int filter);
	FskAPI(FskErr)			FskGLBitmapTextureTargetSet(FskBitmap bm);
	FskAPI(FskErr)			FskGLDstBMRectGet(FskBitmap bm, FskRectangle r);
	FskAPI(FskErr)			FskGLDstPort(FskBitmap bm, FskGLPort *glPortPtr);
	FskAPI(void)			FskGLFBOInit(FskGLPort glPort);
	FskAPI(Boolean)			FskGLFBOIsInited(FskGLPort glPort);
	FskAPI(const float*)	FskGLGetViewMatrix(void);
	FskAPI(FskErr)			FskGLNewProgram(unsigned int vertexShader, unsigned int fragmentShader, unsigned int *progID, ...);
	FskAPI(FskErr)			FskGLNewShader(const char *shaderStr, unsigned int type, unsigned int *shaderID);
	FskAPI(FskErr)			FskGLPortResizeTexture(FskGLPort glPort, int glFormat, int width, int height);
	FskAPI(void)			FskGLPortSetClip(FskGLPort glPort, FskConstRectangle clipRect);
	FskAPI(FskErr)			FskGLPortTexFormatGet(FskGLPort glPort, int *format);
	FskAPI(FskErr)			FskGLPortTexRectGet(FskGLPort glPort, FskRectangle texRect);
	FskAPI(FskErr)			FskGLResetAllState(FskConstGLPort port);
	FskAPI(void)			FskGLSetBlend(Boolean doBlend, unsigned int srcRGB, unsigned int dstRGB, unsigned int srcAlpha, unsigned int dstAlpha);
	FskAPI(void)			FskGLUnbindBMTexture(FskConstBitmap bm);
	#if TARGET_OS_ANDROID || TARGET_OS_KPL || defined(__linux__) || (FSK_OPENGLES_ANGLE == 1)
		#ifndef EGL_VERSION
			#include <EGL/egl.h>
		#endif /* EGL_VERSION */
		FskAPI(EGLConfig)	FskChooseBestEGLConfig(EGLDisplay dpy, const EGLint *attribList);
	#endif /* EGL_VERSION */
#endif /* __FSKGLBLIT_PRIV__ */


#ifndef GLES_VERSION
	#define GLES_VERSION	2	/**< This specifies the version of GL to use: either 1 or 2. */
#endif /* GLES_VERSION */


#ifdef GL_TRACE
	#if TARGET_OS_ANDROID
		FskAPI(void) FskGLTrace(const char *func, const char *file, int line);
		#define GL_TRACE_LOG(func)	FskGLTrace(func, __FILE__, __LINE__)
	#else
		#define GL_TRACE_LOG(func)	fprintf(stderr, "Kinoma GL\tTracing call to %s from %s, line %d\n", func, __FILE__, __LINE__)
	#endif /* TARGET_OS */

	#define FskGLAccelerateBitmapSource(bm)															(GL_TRACE_LOG("FskGLAccelerateBitmapSource"),		FskGLAccelerateBitmapSource(bm))
	#define FskGLBindBMTexture(bm, wrap, filter)													(GL_TRACE_LOG("FskGLBindBMTexture"),				FskGLBindBMTexture(bm, wrap, filter))
	#define FskGLBitmapDraw(srcBM,srcRect,dstBM,dstRect,dstClip,opColor,mode,modeParams)			(GL_TRACE_LOG("FskGLBitmapDraw"),					FskGLBitmapDraw(srcBM,srcRect,dstBM,dstRect,dstClip,opColor,mode,modeParams))
	#define FskGLBitmapTextureTargetSet(bm)															(GL_TRACE_LOG("FskGLBitmapTextureTargetSet"),		FskGLBitmapTextureTargetSet(bm))
	#define FskGLCapabilitiesGet(pCaps)																(GL_TRACE_LOG("FskGLCapabilitiesGet"),				FskGLCapabilitiesGet(pCaps))
	#define FskGLCapabilitiesHas(caps,queryCap)														(GL_TRACE_LOG("FskGLCapabilitiesHas"),				FskGLCapabilitiesHas(caps,queryCap))
	#define FskGLDeaccelerateBitmapSource(bm)														(GL_TRACE_LOG("FskGLDeaccelerateBitmapSource"),		FskGLDeaccelerateBitmapSource(bm))
	#define FskGLDstBMRectGet(bm, r)																(GL_TRACE_LOG("FskGLDstBMRectGet"),					FskGLDstBMRectGet(bm, r))
	#define FskGLFBOIsInited(port)																	(GL_TRACE_LOG("FskGLFBOIsInited"),					FskGLFBOIsInited(port))
	#define FskGLGetFocusDefocusProcs(focus,defocus)												(GL_TRACE_LOG("FskGLGetFocusDefocusProcs"),			FskGLGetFocusDefocusProcs(focus,defocus))
	#define FskGLGetFormatAndTypeFromPixelFormat(fmt,glfmt,gltyp,glintfmt)							(GL_TRACE_LOG("FskGLGetFormatAndTypeFromPixelFormat"),	FskGLGetFormatAndTypeFromPixelFormat(fmt,glfmt,gltyp,glintfmt))
	#define FskGLGetViewMatrix()																	(GL_TRACE_LOG("FskGLGetViewMatrix"),				FskGLGetViewMatrix())
	#define FskGLInit(v)																			(GL_TRACE_LOG("FskGLInit"),							FskGLInit(v))
	#define FskGLMaximumTextureDimension()															(GL_TRACE_LOG("FskGLMaximumTextureDimension"),		FskGLMaximumTextureDimension())
	#define FskGLNewProgram(...)																	(GL_TRACE_LOG("FskGLNewProgram"),					FskGLNewProgram(__VA_ARGS__))
	#define FskGLNewShader(shaderStr,type,shaderID)													(GL_TRACE_LOG("FskGLNewShader"),					FskGLNewShader(shaderStr,type,shaderID))
	#define FskGLPerspectiveTransformBitmap(srcBM,numPoints,points,dstBM,dstClip,M,quality)			(GL_TRACE_LOG("FskGLPerspectiveTransformBitmap"),	FskGLPerspectiveTransformBitmap(srcBM,numPoints,points,dstBM,dstClip,M,quality))
	#define FskGLPortBitmapSwap(fr, to, update)														(GL_TRACE_LOG("FskGLPortBitmapSwap"),				FskGLPortBitmapSwap(fr, to, update))
	#define FskGLPortDefocus(port)																	(GL_TRACE_LOG("FskGLPortDefocus"),					FskGLPortDefocus(port))
	#define FskGLPortDispose(port)																	(GL_TRACE_LOG("FskGLPortDispose"),					FskGLPortDispose(port))
	#define FskGLPortFocus(port)																	(GL_TRACE_LOG("FskGLPortFocus"),					FskGLPortFocus(port))
	#define FskGLPortGetCurrent()																	(GL_TRACE_LOG("FskGLPortGetCurrent"),				FskGLPortGetCurrent())
	#define FskGLPortGetSize(port,width,height)														(GL_TRACE_LOG("FskGLPortGetSize"),					FskGLPortGetSize(port,width,height))
	#define FskGLPortGetSysContext(port)															(GL_TRACE_LOG("FskGLPortGetSysContext"),			FskGLPortGetSysContext(port))
	#define FskGLPortIsDestination(port)															(GL_TRACE_LOG("FskGLPortIsDestination"),			FskGLPortIsDestination(port))
	#define FskGLPortIsPersistent(p)																(GL_TRACE_LOG("FskGLPortIsPersistent"),				FskGLPortIsPersistent(p))
	#define FskGLPortNew(width,height,sysContext,port)												(GL_TRACE_LOG("FskGLPortNew"),						FskGLPortNew(width,height,sysContext,port))
	#define FskGLPortPixelsRead(glPort,backBuffer,FsrcRect,dstBM)									(GL_TRACE_LOG("FskGLPortPixelsRead"),				FskGLPortPixelsRead(glPort,backBuffer,FsrcRect,dstBM))
	#define FskGLPortResize(port,width,height)														(GL_TRACE_LOG("FskGLPortResize"),					FskGLPortResize(port,width,height))
	#define FskGLPortResizeTexture(glPort,glFormat,width,height)									(GL_TRACE_LOG("FskGLPortResizeTexture"),			FskGLPortResizeTexture(glPort,glFormat,width,height))
	#define FskGLPortSetCurrent(p)																	(GL_TRACE_LOG("FskGLPortSetCurrent"),				FskGLPortSetCurrent(p))
	#define FskGLPortSetPersistent(p,v)																(GL_TRACE_LOG("FskGLPortSetPersistent"),			FskGLPortSetPersistent(p,v))
	#define FskGLPortSetSysContext(port,sysContext)													(GL_TRACE_LOG("FskGLPortSetSysContext"),			FskGLPortSetSysContext(port,sysContext))
	#define FskGLPortSourceTexture(port)															(GL_TRACE_LOG("FskGLPortSourceTexture"),			FskGLPortSourceTexture(port))
	#define FskGLPortSwapBuffers(port)																(GL_TRACE_LOG("FskGLPortSwapBuffers"),				FskGLPortSwapBuffers(port))
	#define FskGLPortSwapCurrent(p)																	(GL_TRACE_LOG("FskGLPortSwapCurrent"),				FskGLPortSwapCurrent(p))
	#define FskGLPortTexFormatGet(glPort, format)													(GL_TRACE_LOG("FskGLPortTexFormatGet"),				FskGLPortTexFormatGet(glPort, format))
	#define FskGLPortTexRectGet(glPort, texRect)													(GL_TRACE_LOG("FskGLPortTexRectGet"),				FskGLPortTexRectGet(glPort, texRect))
	#define FskGLProjectBitmap(srcBM,srcRect,dstBM,dstClip,M,opColor,mode,modeParams)				(GL_TRACE_LOG("FskGLProjectBitmap"),				FskGLProjectBitmap(srcBM,srcRect,dstBM,dstClip,M,opColor,mode,modeParams))
	#define FskGLRectangleFill(dstBM,r,color,mode,modeParams)										(GL_TRACE_LOG("FskGLRectangleFill"),				FskGLRectangleFill(dstBM,r,color,mode,modeParams))
	#define FskGLRenderToBitmapTexture(bm, clear)													(GL_TRACE_LOG("FskGLRenderToBitmapTexture"),		FskGLRenderToBitmapTexture(bm, clear))
	#define FskGLScaleOffsetBitmap(srcBM,srcRect,dstBM,dstClip,scaleOffset,opColor,mode,modeParams)	(GL_TRACE_LOG("FskGLScaleOffsetBitmap"),			FskGLScaleOffsetBitmap(srcBM,srcRect,dstBM,dstClip,scaleOffset,opColor,mode,modeParams))
	#define FskGLSetBitmapSourceIsAccelerated(bm, isAccelerated)									(GL_TRACE_LOG("FskGLSetBitmapSourceIsAccelerated"),	FskGLSetBitmapSourceIsAccelerated(bm, isAccelerated))
	#define FskGLSetBlend(doBlend, srcRGB, dstRGB, srcAlpha, dstAlpha)								(GL_TRACE_LOG("FskGLSetBlend"),						FskGLSetBlend(doBlend, srcRGB, dstRGB, srcAlpha, dstAlpha))
	#define FskGLSetFocusDefocusProcs(focus,defocus)												(GL_TRACE_LOG("FskGLSetFocusDefocusProcs"),			FskGLSetFocusDefocusProcs(focus,defocus))
	#define FskGLSetGLView(port)																	(GL_TRACE_LOG("FskGLSetGLView"),					FskGLSetGLView(port))
	#define FskGLShutdown()																			(GL_TRACE_LOG("FskGLShutdown"),						FskGLShutdown())
	#define FskGLSourceTypes(formats)																(GL_TRACE_LOG("FskGLSourceTypes"),					FskGLSourceTypes(formats))
	#define FskGLTextBox(fte,dstBM,text,textLen,dstRect,clipRect,color,blendLevel,textSize,textStyle,hAlign,vAlign,fontName,cache)	(GL_TRACE_LOG("FskGLTextBox"),FskGLTextBox(fte,dstBM,text,textLen,dstRect,clipRect,color,blendLevel,textSize,textStyle,hAlign,vAlign,fontName,cache))
	#define FskGLTextGetBounds(f,b,t,l,z,y,n,d,c)													(GL_TRACE_LOG("FskGLTextGetBounds"),				FskGLTextGetBounds(f,b,t,l,z,y,n,d,c))
	#define FskGLTextGlyphsLoad(fte,text,textLen,textSize,textStyle,fontName,cache)					(GL_TRACE_LOG("FskGLTextGlyphsLoad"),				FskGLTextGlyphsLoad(fte,text,textLen,textSize,textStyle,fontName,cache))
	#define FskGLTextGlyphsUpdateTexture(fte,textSize,textStyle,fontName,cache)						(GL_TRACE_LOG("FskGLTextGlyphsUpdateTexture"),		FskGLTextGlyphsUpdateTexture(fte,textSize,textStyle,fontName,cache))
	#define FskGLTextStrikeBitmap(f,c)																(GL_TRACE_LOG("FskGLTextStrikeBitmap"),				FskGLTextStrikeBitmap(f,c))
	#define FskGLTextStrikeGlyphRange(c0,c1,f)														(GL_TRACE_LOG("FskGLTextStrikeGlyphRange"),			FskGLTextStrikeGlyphRange(c0,c1,f))
	#define FskGLTileBitmap(srcBM,srcClip,dstBM,dstRect,dstClip,scale,opColor,mode,modeParams)		(GL_TRACE_LOG("FskGLTileBitmap"),					FskGLTileBitmap(srcBM,srcClip,dstBM,dstRect,dstClip,scale,opColor,mode,modeParams))
	#define FskGLTransferAlphaBitmap(srcBM,srcRect,dstBM,dstLocation,dstClip,fgColor,modeParams)	(GL_TRACE_LOG("FskGLTransferAlphaBitmap"),			FskGLTransferAlphaBitmap(srcBM,srcRect,dstBM,dstLocation,dstClip,fgColor,modeParams))
	#define FskGLTypeFaceDispose(f)																	(GL_TRACE_LOG("FskGLTypeFaceDispose"),				FskGLTypeFaceDispose(f))
	#define FskGLTypeFaceFirst(pTypeFace, FontName, pTextSize, pTextStyle)							(GL_TRACE_LOG("FskGLTypeFaceFirst"),				FskGLTypeFaceFirst(pTypeFace, FontName, pTextSize, pTextStyle))
	#define FskGLTypeFaceNew(n,z,t,e,c,f)															(GL_TRACE_LOG("FskGLTypeFaceNew"),					FskGLTypeFaceNew(n,z,t,e,c,f))
	#define FskGLTypeFaceNext(pTypeFace, pFontName, pTextSize, pTextStyle)							(GL_TRACE_LOG("FskGLTypeFaceNext"),					FskGLTypeFaceNext(pTypeFace, pFontName, pTextSize, pTextStyle))
	#define FskGLUnbindBMTexture(bm)																(GL_TRACE_LOG("FskGLUnbindBMTexture"),				FskGLUnbindBMTexture(bm))
	#define FskGLUnloadTexture(bm)																	(GL_TRACE_LOG("FskGLUnloadTexture"),				FskGLUnloadTexture(bm))
	#define FskGLUpdateSource(bm)																	(GL_TRACE_LOG("FskGLUpdateSource"),					FskGLUpdateSource(bm))

	#define FskGLSetEGLContext(dsp,srf,ctx)															(GL_TRACE_LOG("FskGLSetEGLContext"),				FskGLSetEGLContext(dsp,srf,ctx))
	#define FskGLSetNativeWindow(win)																(GL_TRACE_LOG("FskGLSetNativeWindow"),				FskGLSetNativeWindow(win))


#endif /* GL_TRACE */



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __FSKGLBLIT__ */
