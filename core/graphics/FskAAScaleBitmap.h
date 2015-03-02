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
	\file	FskAAScaleBitmap.h
	\brief	Anti-aliased scaling of a bitmap.
*/
#ifndef __FSKAASCALEBITMAP__
#define __FSKAASCALEBITMAP__

#ifndef __FSKBITMAP__
	#include "FskBitmap.h"
#endif /* __FSKBITMAP__ */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */



#define kAAScaleTentKernelType		0
#define kAAScaleLanczosKernelType	1



/********************************************************************************
 ********************************************************************************
 ********************************************************************************
 *****					Simple API for scaling images						*****
 ********************************************************************************
 ********************************************************************************
 ********************************************************************************/


/** Scale a bitmap.
 *	\param[in]	kernelType	the filter kernel to use, either kAAScaleTentKernelType or kAAScaleLanczosKernelType.
 *	\param[in]	srcBM		the source bitmap.
 *	\param[in]	srcRect		the sub-rectangle to be used as the source. NULL implies srcBM->bounds.
 *	\param[in]	dstBM		the destination bitmap.
 *	\param[in]	dstRect		the sub-rectangle to be used as the destination. NULL implies dstBM->bounds.
 *	\return		kFskErrNone						if the operation completed successfully.
 *	\return		kFskErrUnsupportedPixelType		if the src/dst pixel format pair is not supported.
 *	\return		kFskErrInvalidParameter			if one of the parameters was not appropriate.
 *	\return		kFskErrMemFull					if the scaler was not able to be allocated.
 **/
FskAPI(FskErr) FskAAScaleBitmap(
	int					kernelType,
	FskBitmap			srcBM,
	FskConstRectangle	srcRect,	/* Map this rect... */
	FskBitmap			dstBM,
	FskConstRectangle	dstRect		/* ...to this rect */
);



/********************************************************************************
 ********************************************************************************
 ********************************************************************************
 *****    API for repeatedly scaling similar images, or for push scaling    *****
 ********************************************************************************
 ********************************************************************************
 ********************************************************************************/


/** A data structure used by the scaler. */
struct FskAAScalerRecord;
/** A data structure used by the scaler. */
typedef struct FskAAScalerRecord FskAAScalerRecord;
/** A data structure used by the scaler. */
typedef struct FskAAScalerRecord *FskAAScaler;


/** Procedure which fetches the next source scanline for the scaler.
 *	\param[in]	userData	pointer which is supplied to (*FskAAScalerGetNextSrcLine).
 *	\return					a pointer to the next source scanline.
 **/
typedef const void* (*FskAAScalerGetNextSrcLine)(void *userData);


/** Procedure which fetches the next destination scanline for the scaler.
 *	\param[in]	userData	pointer which is supplied to FskAAScalerGetNextDstLine.
 *	\param[in]	done		when false, indicates that another scanline is being requested.
 *							when true,  indicates that the last line has been processed and can be flushed.
 *	\return					a pointer to the next source scanline.
 **/
typedef void* (*FskAAScalerGetNextDstLine)(void *userData, Boolean done);


/** Allocate a new scaler.
 *	This can operate in either push or pull mode.
 *	In pull mode, supply both getNextSrcLine and getNextDstLine; when FskAAScalerNew returns, the image resizing is complete.
 *	In push mode, you need to call FskAAScalerNextLine() repeatedly, srcHeight times, then call FskAAScalerDispose().
 *	Push mode is currently unimplemented.
 *	\param[in]	kernelType		the filter kernel to use, either kAAScaleTentKernelType or kAAScaleLanczosKernelType.
 *	\param[in]	srcWidth		the number of pixels horizontally in the source image.
 *	\param[in]	srcHeight		the number of pixels  vertically  in the source image.
 *	\param[in]	srcPixelFormat	the format of the source pixels.
 *	\param[in]	getNextSrcLine	the procedure to call to get a pointer to the next source line.
 *	\param[in]	dstWidth		the number of pixels horizontally in the destination image.
 *	\param[in]	dstHeight		the number of pixels  vertically  in the destination image.
 *	\param[in]	dstPixelFormat	the format of the destination pixels.
 *	\param[in]	getNextDstLine	the procedure to call to get a pointer to the next destination line.
 *	\param[in]	hScale			the scale factor to applied horizontally. If zero, this is inferred by srcWidth  and dstWidth.
 *	\param[in]	vScale			the scale factor to applied vertically.   If zero, this is inferred by srcHeight and dstHeight.
 *	\param[in]	hOffset			the horizontal phase. Typically 0.
 *	\param[in]	vOffset			the  vertical  phase. Typically 0.
 *	\param[in]	*userData		pointer to user data, which is passed as a parameter to (*getNextSrcLine)() and (*getNextDstLine)().
 *	\param[out]	*scaler			scaler data structure, used in push mode. Can be NULL in pull mode, in which case the resizing is done immediately.
 *	\return		kFskErrNone						if the operation completed successfully.
 *	\return		kFskErrUnsupportedPixelType		if the src/dst pixel format pair is not supported.
 *	\return		kFskErrInvalidParameter			if one of the parameters was not appropriate.
 *	\return		kFskErrMemFull					if the scaler was not able to be allocated.
 **/
FskAPI(FskErr)	FskAAScalerNew(
	int		kernelType,
	UInt32	srcWidth,	UInt32	srcHeight,	FskBitmapFormatEnum	srcPixelFormat,	FskAAScalerGetNextSrcLine	getNextSrcLine,
	UInt32	dstWidth,	UInt32	dstHeight,	FskBitmapFormatEnum	dstPixelFormat,	FskAAScalerGetNextDstLine	getNextDstLine,
	double	hScale,		double	vScale,		double	hOffset,                    double                      vOffset,
	void	*userData,
	FskAAScaler	*scaler
);


/** Process another image, with parameters initialized by FskAAScalerNew.
 *	When it returns, the image is scaled.
 *	\param[in]	scaler		the scaler, as allocated by FskAAScalerNew().
 **/
FskAPI(void)	FskAAScalerRun(FskAAScaler scaler);


/** Supply another source scanline to the scaler in push mode.
 *	\param[in]	scaler		the scaler, as allocated by FskAAScalerNew().
 *	\param[in]	srcLine		the next scanline.
 *	\return		kFskErrNone	if the operation completed successfully.
 **/
FskAPI(FskErr)	FskAAScalerNextLine(FskAAScaler scaler, const void *srcLine);


/** Reset the scaler engine.
 *	This would only be used if the scaler would be allocated once and used to resize a set of identically-sized images.
 *	\param[in]	scaler		the scaler, as allocated by FskAAScalerNew().
 *	\return		kFskErrNone	if the operation completed successfully.
 **/
FskAPI(void)	 FskAAScalerReset(FskAAScaler scaler);


/** Dispose a scaler.
 *	\param[in]	scaler	the scaler to be disposed.
 *	\return		kFskErrNone		if the operation completed successfully.
 **/
FskAPI(FskErr)	FskAAScalerDispose(FskAAScaler scaler);



/********************************************************************************
 ********************************************************************************
 ********************************************************************************
 *****				API for decimating an image by integral amounts			*****
 ********************************************************************************
 ********************************************************************************
 ********************************************************************************/


/** Decimate an image by integral amounts using a box filter.
 *	The src and dst regions must be distinct, either by using different src and dst, or by using nonoverlapping rects.
 *	\param[in]	src		The source bitmap.
 *	\param[in]	srcRect	A region of the src to be decimated. NULL implies src->bounds,
 *	\param[in]	dst		The destination bitmap.
 *	\param[in]	dstRect	A region of the dst to be overwritten. NULL implies dst->bounds.
 *	\return		kFskErrNone					if the operation was completed successfully.
 *	\return		kFskErrMismatch				if the src and dst bitmaps are not the same pixel format.
 *	\return		kFskErrUnsupportedPixelType	if the pixel format is not supported.
 *	\return		kFskErrParameterError		if srcRect->width/dstRect->width and srcRect->height/dstRect->height are not both integers.
 **/
FskAPI(FskErr) FskBoxDecimate(FskConstBitmap src, FskConstRectangle srcRect, FskBitmap dst, FskConstRectangle dstRect);



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __FSKAASCALEBITMAP__ */


