/*
     Copyright (C) 2010-2015 Marvell International Ltd.
     Copyright (C) 2002-2010 Kinoma, Inc.

     Licensed under the Apache License, Version 2.0 (the "License");
     you may not use this file except in compliance with the License.
     You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

     Unless required by applicable law or agreed to in writing, software
     distributed under the License is distributed on an "AS IS" BASIS,
     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
     See the License for the specific language governing permissions and
     limitations under the License.
*/
/**
	\file	FskBitmap.h
	\brief	Bitmap image representation and methods.
*/
#ifndef __FSKBITMAP__
#define __FSKBITMAP__

#include "Fsk.h"
#include "FskGraphics.h"
#include "FskRectangle.h"

#ifdef __FSKBITMAP_PRIV__
	// implementation headers
	#if TARGET_OS_WIN32
		#include <Windows.h>
	#endif
#endif /* __FSKBITMAP_PRIV__ */

#if TARGET_OS_LINUX
	#include "FskPlatformImplementation.h"
#endif /* TARGET_OS_LINUX */

#if TARGET_OS_WIN32
	#include <WTypes.h>
#endif /* TARGET_OS_WIN32 */

#ifndef FSKBITMAP_OPENGL
//	#define FSKBITMAP_OPENGL 1	///< Enable  generation of code to support OpenGL-accelerated graphics.
	#define FSKBITMAP_OPENGL 0	///< Disable generation of code to support OpenGL-accelerated graphics.
#endif /* FSKBITMAP_OPENGL */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


typedef enum {
    kFskBitmapFormatDefaultNoAlpha	= (-1),     ///< A pixel format without alpha which is preferred on the target platform.
    kFskBitmapFormatDefaultAlpha	= (-2),     ///< A pixel format  with   alpha which is preferred on the target platform.

    kFskBitmapFormatUnknown			= (0U),     ///< unknown pixel format
    kFskBitmapFormat24BGR			= (1U),     ///< windows 24 bit
    kFskBitmapFormat32ARGB			= (2U),     ///< mac 32 bit
    kFskBitmapFormat32BGRA			= (3U),     ///< win 32 bit
    kFskBitmapFormat32RGBA			= (4U),     ///< PNG 32 bit
    kFskBitmapFormat24RGB			= (5U),     ///< PNG 24 bit
    kFskBitmapFormat16RGB565BE		= (6U),     ///< 16 bit RGB 565 big endian
    kFskBitmapFormat16RGB565LE		= (7U),     ///< 16 bit RGB 565 little endian
    kFskBitmapFormat16RGB5515LE		= (8U),     ///< 16 bit RGB 5515 little endian
    kFskBitmapFormatYUV420			= (9U),     ///< YUV 4:2:0 planar FOURCC('I420')
    kFskBitmapFormatYUV422			= (10U),	///< YUV 422 of unspecified format
    kFskBitmapFormat8A				= (11U),	///< 8 bit alpha
    kFskBitmapFormat8G				= (12U),	///< 8 bit grayscale
    kFskBitmapFormatYUV420spuv		= (13U),	///< YUV 4:2:0 semi-planar FOURCC('NV12') (i.e. with Y in the first plane, and UV interleaved in the second plane)
    kFskBitmapFormat16BGR565LE		= (14U),	///< 16 bit BGR 565 little endian
    kFskBitmapFormat16AG			= (15U),	///< 16 bit alpha-gray format
    kFskBitmapFormat32ABGR			= (16U),	///< 32 bit ABGR 8-8-8-8
    kFskBitmapFormat16RGBA4444LE	= (17U),	///< 16 bit RGBA 4-4-4-4
    kFskBitmapFormatUYVY			= (18U),	///< YUV 422 interleaved - uyvyuyvy... FOURCC('UYVY')
    kFskBitmapFormatYUV420i			= (19U),	///< YUV 420 interleaved - uvyyyy
    kFskBitmapFormat32A16RGB565LE	= (20U),	///< 32 bits - 8 bit alpha in high bytes, one bye unused, 16RGB565LE in low two bytes.
    kFskBitmapFormatYUV420spvu		= (21U),	///< YUV 4:2:0 semi-planar FOURCC('NV21') (i.e. with Y in the first plane, and VU interleaved in the second plane)
    kFskBitmapFormatGLRGB			= (22U),	///< Open GL RGB  offscreen pixel format (usually 565).
    kFskBitmapFormatGLRGBA			= (23U) 	///< Open GL RGBA offscreen pixel format (usually 8888).
} FskBitmapFormatEnum;

FskDeclarePrivateType(FskBitmap)
typedef const FskBitmapRecord *FskConstBitmap;			///< Pointer to a read-only Bitmap Record.

struct FskGLPortRecord;									///< Opaque declaration for a data structure to support Open GL.
typedef struct FskGLPortRecord *FskGLPort;				///< Pointer to a GL Port Record.
typedef const struct FskGLPortRecord *FskConstGLPort;	///< Pointer to a read-only GL Port Record.

#if defined(__FSKBITMAP_PRIV__) || SUPPORT_INSTRUMENTATION

	/// Data structure to store images represented as arrays of pixels.
	///
	struct FskBitmapRecord {
		FskRectangleRecord	bounds;					///< This range of pixels are stored at the bits field.
		UInt32				depth;					///< The number of bits per pixel, valid for chunky formats.
		FskBitmapFormatEnum	pixelFormat;			///< An enumeration symbolizing the components and ordering of components of the pixels.
		SInt32				rowBytes;				///< The y-stride, or bytes between pixels adjacent vertically.
		void				*bits;					///< The storage for the pixels in the given bounds.
		void				*bitsToDispose;			///< When FskBitmapDispose is called, this memory is disposed.

		SInt16				useCount;				///< 0 based: eligible to dispose when useCount goes negative
		SInt16				readLock;
		SInt16				writeLock;
		SInt16				writeSeed;
		Boolean				hasAlpha;				///< Indicates that there is storages for alpha and that it is meaningful.
		Boolean				alphaIsPremultiplied;	///< Indicates that the color components (i.e. R, G, B) have been premultiplied by alpha.

		FskInstrumentedItemDeclaration

		void				(*doDispose)(FskBitmap bmp, void *refcon);	///< Optional procedure to call to dispose the memory referenced in the bits field.
		void				*doDisposeRefcon;							///< This is passed to (*doDispose).

#if BG3CDP
        void				*physicalAddr;			///< The physical address for the pixels in the given bounds.
        void				*hBD;                   ///< The AMP buffer description
		void*				lastImageKhr;			///< The Khr image wrapper
		void*				lastInternalSurf;		///< gcoSURF
#endif /* BG3CDP */
		SInt32				rotation;				///< Rotation of N*90 degrees; not fully supported (deprecated?)
		void				*ext;					///< for frame buffer extensions

	#if FSKBITMAP_OPENGL
		FskGLPort			glPort;					///< Pointer to a data structure to support OpenGL accelerated graphics.
		SInt16				accelerateSeed;
		Boolean				accelerate;
	#endif /* FSKBITMAP_OPENGL */

	#if TARGET_OS_WIN32
		HBITMAP				hbmp;					///< Handle to a Windows bitmap backing up this image.
		void				*dibBits;				///< The pointer to the Windows device independent bitmap pixel storage.
		HDC					hdc;					///< Handle to a Windows device context.
		BITMAPINFO			bmpInfo;				///< Information about the Windows bitmap.
	#elif TARGET_OS_MAC
        void                *cgBitmapContext;		///< Macintosh core graphics context for the bitmap.
	#elif TARGET_OS_ANDROID
		int		surfaceLocked;
	#endif

		// WARNING: Nothing can come after these definitions
		// because on Windows, we extend the size of the FskBitmap structure
		//	to accomodate the color table as necessary.
	};

#endif /* __FSKBITMAP_PRIV__) || SUPPORT_INSTRUMENTATION */


/** Create a new bitmap.
 *	\param[in]	width		desired width of the bitmap.
 *							Note: this can be negative; it indicates that a native bitmap is desired.
 *							The absolute value of the width parameter is used as the actual width.
 *	\param[in]	height		desired height of the bitmap. THis cannot be negative.
 *	\param[in]	pixelFormat	desired pixel format of the bitmap.
 *	\param[out]	bits		pointer to a location to store the new bitmap.
 *	\return		kFskErrNone	if the bitmap was created successfully.
 */
FskAPI(FskErr) FskBitmapNew(SInt32 width, SInt32 height, FskBitmapFormatEnum pixelFormat, FskBitmap *bits);


/** Create a new bitmap, using an already existing pixel storage.
 *	\param[in]	width		width  of the bitmap.
 *	\param[in]	height		height of the bitmap.
 *	\param[in]	pixelFormat	pixel format of the bitmap.
 *	\param[in]	bitDepth	number of bits per pixel in the bitmap.
 *	\param[in]	baseAddr	pointer to the already existing storage for the pixels of the bitmap.
 *	\param[in]	rowBytes	the vertical byte stride.
 *	\param[out]	bitsOut		pointer to a location to store the new bitmap.
 *	\return		kFskErrNone	if the bitmap was created successfully.
 */
FskAPI(FskErr) FskBitmapNewWrapper(SInt32 width, SInt32 height, FskBitmapFormatEnum pixelFormat, UInt32 bitDepth, void *baseAddr, SInt32 rowBytes, FskBitmap *bitsOut);


//#if ESCHER
//FskAPI(FskErr) FskBitmapNewWrapperYUVPlanar(SInt32 width, SInt32 height, UInt32 pixelFormat, UInt32 bitDepth, void *Y, void *U, void *V, SInt32 rowBytes, FskBitmap *bitsOut);
//#endif


/** Dispose the given bitmap.
 *	\param[in]	bits		the bitmap to dispose.
 *	\return		kFskErrNone	if the bitmap was disposed successfully.
 */
FskAPI(FskErr) FskBitmapDispose(FskBitmap bits);


/** Increment the use count of the bitmap.
 *	This is only decremented with FskBitmapReleaseBits().
 *	Do not use naively.
 *	\param[in]	bmp		the bitmap.
 *	\return		kFskErrNone	if the operation succeeded.
 */
FskAPI(FskErr) FskBitmapUse(FskBitmap bmp);


/** Secure read access to a bitmap.
 *	This may lock pointers or otherwise stall other access,
 *	and must be balanced by a FskBitmapReadEnd() to release access to the bitmap.
 *	This guarantees that the fields of the private bitmap data structure are valid.
 *	\param[in]	bmp			the bitmap.
 *	\param[out]	bits		location to store the pointer to pixel(0,0) (can be NULL).
 *	\param[out]	rowBytes	location to store the rowBytes (can be NULL).
 *	\param[out]	pixelFormat	location to store the pixel format (can be NULL).
 *	\return		kFskErrNone	if the operation succeeded.
 */
FskAPI(FskErr) FskBitmapReadBegin(FskBitmap bmp, const void **bits, SInt32 *rowBytes, FskBitmapFormatEnum *pixelFormat);

/** Release read access to a bitmap.
 *	The fields of the private bitmap data structure may not be valid once access is relinquished with this call.
 *	If FskBitmapReadEnd() is called more than FskBitmapReadBegin(), the bitmap may be disposed and all references to it will be invalid.
 *	\return		kFskErrNone	if the operation succeeded.
 */
FskAPI(FskErr) FskBitmapReadEnd(FskBitmap bmp);

/** Secure write access to a bitmap.
 *	This may lock pointers or otherwise stall other access,
 *	and must be balanced by a FskBitmapReadEnd() to release access to the bitmap.
 *	This guarantees that the fields of the private bitmap data structure are valid.
 *	The writeSeed is changed to indicate that the contents may have changed.
 *	\param[in]	bmp			the bitmap.
 *	\param[out]	bits		location to store the pointer to pixel(0,0) (can be NULL).
 *	\param[out]	rowBytes	location to store the rowBytes (can be NULL).
 *	\param[out]	pixelFormat	location to store the pixel format (can be NULL).
 *	\return		kFskErrNone	if the operation succeeded.
 */
FskAPI(FskErr) FskBitmapWriteBegin(FskBitmap bmp, void **bits, SInt32 *rowBytes, FskBitmapFormatEnum *pixelFormat);

/** Release write access to a bitmap.
 *	The fields of the private bitmap data structure may not be valid once access is relinquished with this call.
 *	If FskBitmapWriteEnd() is called more than FskBitmapWriteBegin(), the bitmap may be disposed and all references to it will be invalid.
 *	\return		kFskErrNone	if the operation succeeded.
 */
FskAPI(FskErr) FskBitmapWriteEnd(FskBitmap bmp);


/** Get the number of bits per pixel in the bitmap.
 *	\param[in]	bmp			the bitmap.
 *	\param[out]	depth		location to store the number of bits per pixel.
 *	\return		kFskErrNone	if the call succeeded.
 */
FskAPI(FskErr) FskBitmapGetDepth(FskConstBitmap bmp, UInt32 *depth);


/** Get the bounds of the pixels in the bitmap.
 *	\param[in]	bmp			the bitmap.
 *	\param[out]	bounds		location to store the bounds of the pixels in the bitmap.
 *	\return		kFskErrNone	if the call succeeded.
 */
FskAPI(FskErr) FskBitmapGetBounds(FskConstBitmap bmp, FskRectangle bounds);


/** Get the format of the pixels in the bitmap.
 *	\param[in]	bmp			the bitmap.
 *	\param[out]	pixelFormat	location to store the format of the pixels in the bitmap.
 *	\return		kFskErrNone	if the call succeeded.
 */
FskAPI(FskErr) FskBitmapGetPixelFormat(FskConstBitmap bmp, FskBitmapFormatEnum *pixelFormat);


/** Indicate that the bitmap has a meaningful alpha component.
 *	\param[in]	bmp			the bitmap.
 *	\param[in]	hasAlpha	if true, indicates that the bitmap has a meaningful alpha.
 *	\return		kFskErrNone	if the call succeeded.
 */
FskAPI(FskErr) FskBitmapSetHasAlpha(FskBitmap bmp, Boolean hasAlpha);


/** Query whether the bitmap has an alpha component.
 *	\param[in]	bmp			the bitmap.
 *	\param[out]	hasAlpha	location to store the result of the query.
 *	\return		kFskErrNone	if the call succeeded.
 */
FskAPI(FskErr) FskBitmapGetHasAlpha(FskConstBitmap bmp, Boolean *hasAlpha);


/** Indicate that the bitmap has pixels where the color components have been premultiplied by the alpha component.
 *	When premultiplied, no color component should have a value greater than the respective alpha component.
 *	The result of any operation is undefined when a color component exceeds alpha in a premultiplied bitmap.
 *	\param[in]	bmp				the bitmap.
 *	\param[in]	isPremultiplied	if true, indicates that the bitmap has pixels where the color components have been premultiplied by alpha.
 *	\return		kFskErrNone		if the call succeeded.
 */
FskAPI(FskErr) FskBitmapSetAlphaIsPremultiplied(FskBitmap bmp, Boolean isPremultiplied);


/** Query whether the bitmap has pixels where the color components have been premultiplied by the alpha component.
 *	\param[in]	bmp				the bitmap.
 *	\param[out]	isPremultiplied	location to store the result of the query.
 *								Returns false if the bitmap does not have alpha.
 *	\return		kFskErrNone		if the call succeeded.
 */
FskAPI(FskErr) FskBitmapGetAlphaIsPremultiplied(FskConstBitmap bmp, Boolean *isPremultiplied);


/** Retrieve the pixel at the given location in the bitmap.
 *	\param[in]	bmp				the bitmap.
 *	\param[in]	x				the x-coordinate of the pixel.
 *	\param[in]	y				the y-coordinate of the pixel.
 *	\param[out]	pixel			location to store the value of the pixel at the specified location in the bitmap.
 *	\return		kFskErrNone		if the call succeeded.
 */
FskAPI(FskErr) FskBitmapGetPixel(FskConstBitmap bmp, SInt32 x, SInt32 y, FskColorRGBA pixel);


/** Retrieve the native representation of the bitmap.
 *	\param[in]	bmp	the bitmap.
 *	\return		the native representation of the bitmap, or NULL if no such representation exists.
 */
FskAPI(void) *FskBitmapGetNativeBitmap(FskBitmap bmp);

#if BG3CDP
/** Set bitmap physical address
 *	\param[in]	bmp				the bitmap.
 *	\param[in]	addr            physical address
 *	\return		kFskErrNone		if the call succeeded.
 */
FskAPI(FskErr) FskBitmapSetPhysicalAddress(FskBitmap bmp, void *addr);


/** Get bitmap physical address
 *	\param[in]	bmp				the bitmap.
 *	\return		physical address of the bitmap
 */
FskAPI(void) *FskBitmapGetPhysicalAddress(FskConstBitmap bmp);
#endif


#if TARGET_OS_WIN32
	/** Create a bitmap from a Windows bitmap.
	 *	\param[in]	hBitmap		the Windows bitmap.
	 *	\param[out]	bitsOut		the new FskBitmap, wrapping  the Windows bitmap.
	 *	\return		kFskErrNone	if the call succeeded.
	 */
	FskErr FskBitmapNewFromWindowsBitmap(FskBitmap *bitsOut, HBITMAP hBitmap);
#endif /* TARGET_OS_WIN32 */


/** Transfer one bitmap to another with scaling, format conversion, and transfer operation.
 *	This procedure specifies size transformations with the srcRect and dstRect, though allows neither reflections nor subpixel positioning.
 *	\param[in]		src			The source bitmap.
 *	\param[in]		srcRect		The rectangle within the source that is to be transferred to the destination.
 *								This can be NULL, in which case the src->bounds is used as the srcRect.
 *	\param[in,out]	dst			The destination bitmap.
 *	\param[in]		dstRect		The rectangle to which the srcRect is to be mapped. This implies a scaling transformation.
 *								This can be NULL, in which case the dst->bounds is used as the dstRect.
 *	\param[in]		dstClip		This restricts changes to the destination to this clipping rectangle.
 *								This can be NULL, in which case the dst->bounds is used as the clipping rectangle.
 *	\param[in]		opColor		The color to be used in the kFskGraphicsModeColorize mode.
 *								This can be NULL, in which case opaque white RGBA={255,255,255,255} is used.
 *	\param[in]		mode		The desired transfer mode, one of kFskGraphicsModeCopy, kFskGraphicsModeAlpha, or kFskGraphicsModeColorize,
 *								optionally ORed with kFskGraphicsModeBilinear.
 *	\param[in]		modeParams	Extra parameters needed by the selected transfer mode, currently only blendLevel.
 *								This can be NULL, in which case a blendLevel of 255 is used.
 *	\return			kFskErrNone					if successful.
 *	\return			kFskErrUnalignedYUV			if the source or destination was YUV and the location or width was not an even number.
 *	\return			kFskErrUnsupportedPixelType	if either the source or destination is not supported by the implementation on this platform.
 *	\return			kFskErrInvalidParameter		if the transfer mode is not recognized.
 */
FskAPI(FskErr) FskBitmapDraw(FskConstBitmap src, FskConstRectangle srcRect, FskBitmap dst, FskConstRectangle dstRect, FskConstRectangle dstClip, FskConstColorRGBA opColor, UInt32 mode, FskConstGraphicsModeParameters modeParams);


/** Fill the given rectangle.
 *	\param[in,out]	dst			the bitmap to be modified.
 *	\param[in]		r			the region in the bitmap to be modified. This cannot be NULL.
 *	\param[in]		color		the fill color.
 *	\param[in]		mode		the operation to be applied when filling the rectangle; one of
 *								{ kFskGraphicsModeCopy, kFskGraphicsModeColorize}.
 *	\param[in]		modeParams	Parameters to be used during the pixel transfer, primarily the blendLevel.
 *								Can be NULL, in which case a blendLevel of 255 is assumed.
 */
FskAPI(FskErr) FskRectangleFill(FskBitmap dst, FskConstRectangle r, FskConstColorRGBA color, UInt32 mode, FskConstGraphicsModeParameters modeParams);

/** Set the sysContext field in the OpenGL context of the bitmap.
 *	\param[in,out]	bm			the bitmap.
 *	\param[in]		sysContext	the sysContext.
 *	\return			kFskErrNone	if the call succeeded.
 */
FskAPI(FskErr)	FskBitmapSetOpenGLSysContext(FskBitmap bm, void *sysContext);


/** Get the sysContext field in the OpenGL context of the bitmap.
 *	\param[in]	bm			the bitmap.
 *	\return		the sysContext, or NULL if not accessible.
 */
FskAPI(void*)	FskBitmapGetOpenGLSysContext(FskBitmap bm);


/** Maximum texture dimension.
 *	\return	the maximum allowable OpenGL texture dimension for either width or height.
 */
FskAPI(SInt32) FskBitmapMaximumOpenGLDimension(void);


/** Set whether a bitmap is to be accelerated as a source for OpenGL calls.
 *	This does not actually upload the texture, it just sets a bit in the bitmap.
 *	It needs to be followed by FskBitmapCheckGLSourceAccelerated() to actually upload the texture.
 *	\param[in]	bm			the bitmap.
 *	\param[in]	accelerated	if true, turns acceleration on; otherwise turns acceleration off.
 *	\return		kFskErrNone						if the acceleration was able to be changed as requested.
 *	\return		kFskErrExtensionNotFound		if GL is not being used.
 */
FskAPI(FskErr)	FskBitmapSetOpenGLSourceAccelerated(FskBitmap bm, Boolean accelerated);


/** Actually upload the texture from the bitmap storage.
 *	It checks the accelerate bit and compares seeds to see if anything has changed, and if so, uploads the texture.
 *	\param[in]	bits			the bitmap.
 *	\return		kFskErrNone						if the bitmap was accelerated as requested.
 *	\return		kFskErrUnsupportedPixelType		if the pixel type cannot be accelerated.
 *	\return		kFskErrTextureTooLarge			if the bitmap was too large, and was clipped.
 *	\return		kFskErrInvalidParameter			if the bits parameter was not a bitmap.
 *	\return		kFskErrGraphicsContext			if GL was not appropriately initialized.
 *	\return		kFskErrExtensionNotFound		if GL is not being used.
 *	\return		kFskErrUnimplemented			if acceleration is not available on the target platform.
 */
FskAPI(FskErr)	FskBitmapCheckGLSourceAccelerated(FskBitmap bits);


/** Query whether the bitmap is designated for use as an accelerated source using OpenGL.
 *	To assure that the texture is uploaded before executing any GL rendering operation, call FskBitmapCheckGLSourceAccelerated();
 *	this checks to see whether the texture reflects the current state of the bitmap, and only uploads the texture in that case.
 *	To check whether the bitmap has a texture, check FskGLPortSourceTexture(bm->glPort) for nonzero; note, though, that
 *	the contents of the texture may not reflect the current value of the bitmap.
 *	\param[in]	bm	the bitmap to be queried.
 *	\return		true if the bitmap is or will be accelerated for use as a source in OpenGL operations; false otherwise.
 */
FskAPI(Boolean)	FskBitmapIsOpenGLSourceAccelerated(FskConstBitmap bm);


/** Update the associated Open GL texture with the current contents of the bitmap.
 *	\param[in]	bm			the bitmap.
 *	\return		kFskErrNone	if the update was successful.
 */
FskAPI(FskErr)	FskBitmapUpdateGLSource(FskConstBitmap bm);


/** Query whether the bitmap is accelerated for use as a destination using OpenGL.
 *	\param[in]	bm	the bitmap to be queried.
 *	\return		true if the bitmap is accelerated for use as a destination in OpenGL operations; false otherwise.
 */
FskAPI(Boolean)	FskBitmapIsOpenGLDestinationAccelerated(FskConstBitmap bm);


/*****************************************************************
 *****************************************************************
 **		We make sure that all src and dst pixel formats			**
 **		are explicitly disabled if not enabled.					**
 **			(order does not matter here)						**
 *****************************************************************
 *****************************************************************/

#ifndef		SRC_32ARGB
# define	SRC_32ARGB				0
#endif /*	SRC_32ARGB */
#ifndef		DST_32ARGB
# define	DST_32ARGB				0
#endif /*	DST_32ARGB */
#ifndef		DST_UNITY_32ARGB
# define	DST_UNITY_32ARGB		0
#endif /*	DST_UNITY_32ARGB */

#ifndef		SRC_32BGRA
# define	SRC_32BGRA				0
#endif /*	SRC_32BGRA */
#ifndef		DST_32BGRA
# define	DST_32BGRA				0
#endif /*	DST_32BGRA */
#ifndef		DST_UNITY_32BGRA
# define	DST_UNITY_32BGRA		0
#endif /*	DST_UNITY_32BGRA */

#ifndef		SRC_32RGBA
# define	SRC_32RGBA				0
#endif /*	SRC_32RGBA */
#ifndef		DST_32RGBA
# define	DST_32RGBA				0
#endif /*	DST_32RGBA */
#ifndef		DST_UNITY_32RGBA
# define	DST_UNITY_32RGBA		0
#endif /*	DST_UNITY_32RGBA */

#ifndef		SRC_32ABGR
# define	SRC_32ABGR				0
#endif /*	SRC_32ABGR */
#ifndef		DST_32ABGR
# define	DST_32ABGR				0
#endif /*	DST_32ABGR */
#ifndef		DST_UNITY_32ABGR
# define	DST_UNITY_32ABGR		0
#endif /*	DST_UNITY_32ABGR */

#ifndef		SRC_24BGR
# define	SRC_24BGR				0
#endif /*	SRC_24BGR */
#ifndef		DST_24BGR
# define	DST_24BGR				0
#endif /*	DST_24BGR */
#ifndef		DST_UNITY_24BGR
# define	DST_UNITY_24BGR			0
#endif /*	DST_UNITY_24BGR */

#ifndef		SRC_24RGB
# define	SRC_24RGB				0
#endif /*	SRC_24RGB */
#ifndef		DST_24RGB
# define	DST_24RGB				0
#endif /*	DST_24RGB */
#ifndef		DST_UNITY_24RGB
# define	DST_UNITY_24RGB			0
#endif /*	DST_UNITY_24RGB */

#ifndef		SRC_16RGB565BE
# define	SRC_16RGB565BE			0
#endif /*	SRC_16RGB565BE */
#ifndef		DST_16RGB565BE
# define	DST_16RGB565BE			0
#endif /*	DST_16RGB565BE */
#ifndef		DST_UNITY_16RGB565BE
# define	DST_UNITY_16RGB565BE	0
#endif /*	DST_UNITY_16RGB565BE */

#ifndef		SRC_16RGB565LE
# define	SRC_16RGB565LE			0
#endif /*	SRC_16RGB565LE */
#ifndef		DST_16RGB565LE
# define	DST_16RGB565LE			0
#endif /*	DST_16RGB565LE */
#ifndef		DST_UNITY_16RGB565LE
# define	DST_UNITY_16RGB565LE	0
#endif /*	DST_UNITY_16RGB565LE */

#ifndef		SRC_16RGB5515LE
# define	SRC_16RGB5515LE			0
#endif /*	SRC_16RGB5515LE */
#ifndef		DST_16RGB5515LE
# define	DST_16RGB5515LE			0
#endif /*	DST_16RGB5515LE */
#ifndef		DST_UNITY_16RGB5515LE
# define	DST_UNITY_16RGB5515LE	0
#endif /*	DST_UNITY_16RGB5515LE */

#ifndef		SRC_16RGB5515BE
# define	SRC_16RGB5515BE			0
#endif /*	SRC_16RGB5515BE */
#ifndef		DST_16RGB5515BE
# define	DST_16RGB5515BE			0
#endif /*	DST_16RGB5515BE */
#ifndef		DST_UNITY_16RGB5515BE
# define	DST_UNITY_16RGB5515BE	0
#endif /*	DST_UNITY_16RGB5515BE */

#ifndef		SRC_16BGR565LE
# define	SRC_16BGR565LE			0
#endif /*	SRC_16BGR565LE */
#ifndef		DST_16BGR565LE
# define	DST_16BGR565LE			0
#endif /*	DST_16BGR565LE */
#ifndef		DST_UNITY_16BGR565LE
# define	DST_UNITY_16BGR565LE	0
#endif /*	DST_UNITY_16BGR565LE */

#ifndef		SRC_16AG
# define	SRC_16AG				0
#endif /*	SRC_16AG */
#ifndef		DST_16AG
# define	DST_16AG				0
#endif /*	DST_16AG */
#ifndef		DST_UNITY_16AG
# define	DST_UNITY_16AG			0
#endif /*	DST_UNITY_16AG */

#ifndef		SRC_16GA
# define	SRC_16GA				0
#endif /*	SRC_16GA */
#ifndef		DST_16GA
# define	DST_16GA				0
#endif /*	DST_16GA */
#ifndef		DST_UNITY_16GA
# define	DST_UNITY_16GA			0
#endif /*	DST_UNITY_16GA */

#ifndef		SRC_16RGBA4444LE
# define	SRC_16RGBA4444LE		0
#endif /*	SRC_16RGBA4444LE */
#ifndef		DST_16RGBA4444LE
# define	DST_16RGBA4444LE		0
#endif /*	DST_16RGBA4444LE */
#ifndef		DST_UNITY_16RGBA4444LE
# define	DST_UNITY_16RGBA4444LE	0
#endif /*	DST_UNITY_16RGBA4444LE */

#ifndef		SRC_16RGBA4444BE
# define	SRC_16RGBA4444BE		0
#endif /*	SRC_16RGBA4444BE */
#ifndef		DST_16RGBA4444BE
# define	DST_16RGBA4444BE		0
#endif /*	DST_16RGBA4444BE */
#ifndef		DST_UNITY_16RGBA4444BE
# define	DST_UNITY_16RGBA4444BE	0
#endif /*	DST_UNITY_16RGBA4444BE */

#ifndef		SRC_YUV420
# define	SRC_YUV420				0
#endif /*	SRC_YUV420 */
#ifndef		DST_YUV420
# define	DST_YUV420				0
#endif /*	DST_YUV420 */
#ifndef		DST_UNITY_YUV420
# define	DST_UNITY_YUV420		0
#endif /*	DST_UNITY_YUV420 */

#ifndef		SRC_YUV420i
# define	SRC_YUV420i				0
#endif /*	SRC_YUV420i */
#ifndef		DST_YUV420i
# define	DST_YUV420i				0
#endif /*	DST_YUV420i */
#ifndef		DST_UNITY_YUV420i
# define	DST_UNITY_YUV420i		0
#endif /*	DST_UNITY_YUV420i */

#ifndef		SRC_YUV422
# define	SRC_YUV422				0
#endif /*	SRC_YUV422 */
#ifndef		DST_YUV422
# define	DST_YUV422				0
#endif /*	DST_YUV422 */
#ifndef		DST_UNITY_YUV422
# define	DST_UNITY_YUV422		0
#endif /*	DST_UNITY_YUV422 */

#ifndef		SRC_8A
# define	SRC_8A					0
#endif /*	SRC_8A */
#ifndef		DST_8A
# define	DST_8A					0
#endif /*	DST_8A */
#ifndef		DST_UNITY_8A
# define	DST_UNITY_8A			0
#endif /*	DST_UNITY_8A */

#ifndef		SRC_8G
# define	SRC_8G					0
#endif /*	SRC_8G */
#ifndef		DST_8G
# define	DST_8G					0
#endif /*	DST_8G */
#ifndef		DST_UNITY_8G
# define	DST_UNITY_8G			0
#endif /*	DST_UNITY_8G */

#ifndef		SRC_YUV420spuv
# define	SRC_YUV420spuv			0
#endif /*	SRC_YUV420spuv */
#ifndef		DST_YUV420spuv
# define	DST_YUV420spuv			0
#endif /*	DST_YUV420spuv */
#ifndef		DST_UNITY_YUV420spuv
# define	DST_UNITY_YUV420spuv	0
#endif /*	DST_UNITY_YUV420spuv */

#ifndef		SRC_UYVY
# define	SRC_UYVY				0
#endif /*	SRC_UYVY */
#ifndef		DST_UYVY
# define	DST_UYVY				0
#endif /*	DST_UYVY */
#ifndef		DST_UNITY_UYVY
# define	DST_UNITY_UYVY			0
#endif /*	DST_UNITY_UYVY */

#ifndef		SRC_32A16RGB565LE
# define	SRC_32A16RGB565LE		0
#endif /*	SRC_32A16RGB565LE */
#ifndef		DST_32A16RGB565LE
# define	DST_32A16RGB565LE		0
#endif /*	DST_32A16RGB565LE */
#ifndef		DST_UNITY_32A16RGB565LE
# define	DST_UNITY_32A16RGB565LE	0
#endif /*	DST_UNITY_32A16RGB565LE */

#ifndef		SRC_YUV420spvu
# define	SRC_YUV420spvu			0
#endif /*	SRC_YUV420spvu */
#ifndef		DST_YUV420spvu
# define	DST_YUV420spvu			0
#endif /*	DST_YUV420spvu */
#ifndef		DST_UNITY_YUV420spvu
# define	DST_UNITY_YUV420spvu	0
#endif /*	DST_UNITY_YUV420spvu */
/****************************************
 *	^  ADD NEW PIXEL FORMATS HERE  ^	*
 ****************************************/

/********************************************************************************
 * Here we determine the characteristics of the platform:
 * not only the CPU, but the accompanying peripheral hardware as well.
 * The peripheral hardware is described in a file called FskPlatform.h
 *
 * Makefiles should have a rule
 *
 *		$(TMP_DIR)/FskPlatform.h: $(F_HOME)/tinyhttp/XXX/FskPlatform.XXX.h
 *			cp $(F_HOME)/tinyhttp/XXX/FskPlatform.XXX.h $(TMP_DIR)/FskPlatform.h
 *
 * where FskPlatform.XXX.h contains pixel formats for the platform
 *
 *		#define SRC_32BGRA				1	// These are the supported source types
 *		#define SRC_16RGB565LE			1
 *		#define SRC_YUV420				1
 *
 *		#define DST_16RGB565LE			1	// These are the supported destination types
 *
 *		#define DST_UNITY_16RGB565LE	1	// These are the popular screen pixel formats
 *
 * The notion of "platform" in FskPlatform.h is different than in the subsequent section,
 * because the former is unknown to the compiler, and the latter is.
 ********************************************************************************/

#if SUPPORT_INSTRUMENTATION

/** Enumeration of message type to be sent when instrumentation is turned on.
 */
enum {
	kFskBitmapInstrMsgGet = kFskInstrumentedItemFirstCustomMessage,	///< FskBitmapGetBits() was called.
	kFskBitmapInstrMsgRelease,										///< FskBitmapReleaseBits was called.
	kFskBitmapInstrMsgDisposeLocked,								///< The bitmap was disposed while still locked.
	kFskBitmapInstrMsgTooManyUnlocks,								///< More FskBitmapReleaseBits() than FskBitmapGetBits() calls have been made.
	kFskBitmapInstrMsgInitialize,									///< FskBitmapNew() has completed, and the results are reported.
	kFskBitmapInstrMsgInitializeWrapper,							///< FskBitmapNewWrapper() has completed, and the results are reported.
    kFskBitmapInstrMsgSetAccelerated
};

#endif /* SUPPORT_INSTRUMENTATION */



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __FSKBITMAP__ */
