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
	\file	FskBlur.h
	\brief	Gaussian and iterated box filter blurring.
*/
#ifndef __FSKBLUR__
#define __FSKBLUR__

#ifndef __FSKBITMAP__
# include "FskBitmap.h"
#endif /* __FSKBITMAP__ */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */



/***************************************************************************//**
 * Blur an image with a Gaussian blur.
 *	\param[in]	srcBM	the source bitmap.
 *	\param[in]	srcRect	a sub-rectangle in the source to blur; NULL implies the whole bitmap.
 *	\param[out]	dstBM	the destination bitmap. Must be the same pixel format as the source bitmap.
 *						Can be the same as the source bitmap.
 *	\param[in]	dstPt	a place in the destination to place the blurred srcRect; NULL implies (0,0).
 *	\param[in]	sigmaX	the standard deviation of the Gaussian kernel used for blurring horizontally.
 *	\param[in]	sigmaY	the standard deviation of the Gaussian kernel used for blurring vertically.
 *	\return	kFskErrNone					if the operation completed successfully.
 *	\return	kFskErrMemFull				if a small amount of working memory was not able to be allocated.
 *	\return	kFskErrNothingRendered		if the effective source rect (after appropriate clipping) has zero area.
 *										This is not really an error, so err > 0.
 *	\return	kFskErrUnsupportedPixelType	if the pixel format is not accommodated; currently support:
 * 										8A, 8G,
 * 										16AG, 16RGB565BE, 16RGB565LE, 16BGR565LE,
 *										24BGR, 24RGB,
 *										32ARGB, 32BGRA, 32RGBA, 32ABGR
 *******************************************************************************/

FskAPI(FskErr) FskGaussianBlur(FskConstBitmap srcBM, FskConstRectangle srcRect, FskBitmap dstBM, FskConstPoint dstPt, float sigmaX, float sigmaY);


/***************************************************************************//**
 * Blur an image with a Box blur.
 *	\param[in]	srcBM	the source bitmap.
 *	\param[in]	srcRect	a sub-rectangle in the source to blur; NULL implies the whole bitmap.
 *	\param[out]	dstBM	the destination bitmap. Must be the same pixel format as the source bitmap.
 *						Can be the same as the source bitmap.
 *	\param[in]	dstLoc	a place in the destination to place the blurred srcRect; NULL implies (0,0).
 *	\param[in]	radiusX	the radius of the kernel used for blurring horizontally, incorporating 2*radiusX+1 pixels.
 *	\param[in]	radiusY	the radius of the kernel used for blurring vertically,   incorporating 2*radiusX+1 pixels.
 *	\return	kFskErrNone					if the operation completed successfully.
 *	\return	kFskErrMemFull				if a small amount of working memory was not able to be allocated.
 *	\return	kFskErrNothingRendered		if the effective source rect (after appropriate clipping) has zero area.
 *										This is not really an error, so err > 0.
 *	\return	kFskErrUnsupportedPixelType	if the pixel format is not accommodated; currently support:
 * 										8A, 8G,
 * 										16AG, 16RGB565BE, 16RGB565LE, 16BGR565LE,
 *										24BGR, 24RGB,
 *										32ARGB, 32BGRA, 32RGBA, 32ABGR
 *******************************************************************************/

FskAPI(FskErr) FskBoxBlur(FskConstBitmap srcBM, FskConstRectangle srcRect, FskBitmap dstBM, FskConstPoint dstLoc, UInt32 radiusX, UInt32 radiusY);


/***************************************************************************//**
 * Blur an image. The specified sigma is the standard deviation of a Gaussian.
 * If the sigma is large, the blur is implemented as an iterated box blur for speed;
 * however, unlike the Gaussian blur, the iterated box blur has a quantized sigma.
 *
 *	\param[in]	srcBM	the source bitmap.
 *	\param[in]	srcRect	a sub-rectangle in the source to blur; NULL implies the whole bitmap.
 *	\param[out]	dstBM	the destination bitmap. Must be the same pixel format as the source bitmap.
 *						Can be the same as the source bitmap.
 *	\param[in]	dstPt	a place in the destination to place the blurred srcRect; NULL implies (0,0).
 *	\param[in]	sigmaX	the standard deviation of the Gaussian kernel used for blurring horizontally.
 *	\param[in]	sigmaY	the standard deviation of the Gaussian kernel used for blurring vertically.
 *	\return	kFskErrNone					if the operation completed successfully.
 *	\return	kFskErrMemFull				if a small amount of working memory was not able to be allocated.
 *	\return	kFskErrNothingRendered		if the effective source rect (after appropriate clipping) has zero area.
 *										This is not really an error, so err > 0.
 *	\return	kFskErrUnsupportedPixelType	if the pixel format is not accommodated; currently support:
 * 										8A, 8G,
 * 										16AG, 16RGB565BE, 16RGB565LE, 16BGR565LE,
 *										24BGR, 24RGB,
 *										32ARGB, 32BGRA, 32RGBA, 32ABGR
 *******************************************************************************/

FskAPI(FskErr) FskBlur(FskConstBitmap srcBM, FskConstRectangle srcRect, FskBitmap dstBM, FskConstPoint dstPt, float sigmaX, float sigmaY);


/***************************************************************************//**
 * Blur an image with a box blur repeated 3 times to closely approximate a Gaussian blur.
 * This has a significant speed advantage over the typical Gaussian blur implementation especially at higher sigmas,
 * because this is O(1) and Gaussian blur is O(sigma), with a crossover point of about sigma=2.1 on ARM, 4 on x86.
 * At sigma=10, the iterated box blur is 5-7X faster on ARM, 2.3X faster on x86.
 * The effective sigma is quantized to multiples of 1.05, since the box blur implementation is quantized to integers,
 * so we recommend using it only for larger sigmas. When sigma >= 10, the results should be indistinguishable from a Gaussian blur.
 * However, if the blur radius is not that critical, sigma >= 2 will still look like a Gaussian blur, although with an apparently different sigma.
 * Box blur is especially suitable for SIMD implementation.
 *	\param[in]	srcBM	the source bitmap.
 *	\param[in]	srcRect	a sub-rectangle in the source to blur; NULL implies the whole bitmap.
 *	\param[out]	dstBM	the destination bitmap. Must be the same pixel format as the source bitmap.
 *						Can be the same as the source bitmap.
 *	\param[in]	dstPt	a place in the destination to place the blurred srcRect; NULL implies (0,0).
 *	\param[in]	sigmaX	the equivalent standard deviation of a Gaussian kernel used for blurring horizontally.
 *	\param[in]	sigmaY	the equivalent standard deviation of a Gaussian kernel used for blurring vertically.
 *	\return	kFskErrNone					if the operation completed successfully.
 *	\return	kFskErrMemFull				if a small amount of working memory was not able to be allocated.
 *	\return	kFskErrNothingRendered		if the effective source rect (after appropriate clipping) has zero area.
 *										This is not really an error, so err > 0.
 *	\return	kFskErrUnsupportedPixelType	if the pixel format is not accommodated; currently support:
 * 										8A, 8G,
 * 										16AG, 16RGB565BE, 16RGB565LE, 16BGR565LE,
 *										24BGR, 24RGB,
 *										32ARGB, 32BGRA, 32RGBA, 32ABGR
 *******************************************************************************/

FskAPI(FskErr) FskIteratedBoxBlur(FskConstBitmap srcBM, FskConstRectangle srcRect, FskBitmap dstBM, FskConstPoint dstPt, float sigmaX, float sigmaY);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __FSKBLUR__ */


