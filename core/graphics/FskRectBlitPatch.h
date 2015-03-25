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
#ifndef __FSKRECTBLITPATCH__
#define __FSKRECTBLITPATCH__

#ifndef __FSK__
# include "Fsk.h"				/* for UInt32 */
#endif /* __FSK__ */

#ifndef __FSKFIXEDMATH__
# include "FskFixedMath.h"		/* for FskFixed */
#endif /* __FSKFIXEDMATH__ */


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#define kFskRectBlitBits	18	/**< The number of fractional bits used for srcX0, srcY0, srcXInc, srcYInc. */

/****************************************************************************//**
 * FskRectBlitParams is the interface to the BLIT procs.
 *
 * It is not necessarily a public data structure. It is probably stable,
 * but it shouldn't be relied upon. Any patches introduced should be cleared
 * and registered with Kinoma, so that any changes in this data structure
 * can be propagated to the blit procs.
 ********************************************************************************/

typedef struct FskRectBlitParams {
	/* Source image */
	const void		*srcBaseAddr;		/**< Location of source pixel (0,0) */
	SInt32			srcRowBytes;		/**< Source's increment to the pixel below */
	UInt32			srcWidth;			/**< Never go outside [0, srcWidth  - 1] */
	UInt32			srcHeight;			/**< Never go outside [0, srcHeight - 1] */

	/* Destination image */
	void			*dstBaseAddr;		/**< Location of destination pixel (0,0). */
	SInt32			dstRowBytes;		/**< Destination's increment to the pixel below. */
	UInt32			dstWidth;			/**< The number of pixels to touch horizontally in the destination. */
	UInt32			dstHeight;			/**< The number of pixels to tough vertically in the destination. */

	/* Source coordinates corresponding to destination coordinates (0,0), pre-rounded */
	FskFixed		srcX0;				/**< The horizontal source coordinate corresponding to destination coordinate 0, with kFskRectBlitBits fractional bits. */
	FskFixed		srcY0;				/**< The  vertical  source coordinate corresponding to destination coordinate 0, with kFskRectBlitBits fractional bits. */

	/* Increments in the source as we advance to the next destination pixel in X or Y */
	FskFixed		srcXInc;			/**< The horizontal increment in source coordinate corresponding to +1 in destination coordinate, with kFskRectBlitBits fractional bits. */
	FskFixed		srcYInc;			/**< The horizontal increment in source coordinate corresponding to +1 in destination coordinate, with kFskRectBlitBits fractional bits. */

	Boolean			isPremul;			/**< Indicates that the source is premultiplied by its alpha. */

	/* Info for YUV: pointers to U and V components */
	const void		*srcUBaseAddr;		/**< The location of    source   pixel U(0,0). */
	const void		*srcVBaseAddr;		/**< The location of    source   pixel V(0,0). */
	const void		*dstUBaseAddr;		/**< The location of destination pixel U(0,0). */
	const void		*dstVBaseAddr;		/**< The location of destination pixel V(0,0). */

	/* Operation color */
	UInt8			alpha;				/**< Alpha component of the op color. */
	UInt8			red;				/**< Red   component of the op color. */
	UInt8			green;				/**< Green component of the op color. */
	UInt8			blue;				/**< Blue  component of the op color. */

	/* video adjustments */
	FskFixed		contrast;			/**< Contrast,   with 16 fractional bits, typically in [ 0, +2]. */
	FskFixed		brightness;			/**< Brightness, with 16 fractional bits, typically in [-1, +1]. */
	void			*sprites;			/**< FskVideoSpriteWorld. */
	FskBitmapFormatEnum	srcPixelFormat;		/**< The pixel format of the source. */
	FskBitmapFormatEnum	dstPixelFormat;		/**< The pixel format of the destination. */

	unsigned char   *blockPatternPtr;
	FskFixed		last_srcXInc;		/**< The last last_srcXInc hat was used. */
	FskFixed		last_srcYInc;		/**< The last last_srcYInc hat was used. */

	/* Platform-specific extension mechanism */
	void			*extension;			/**< Undefined, implementation-dependent, extension fields. */
} FskRectBlitParams;


/****************************************************************************//**
 * FskRectTransferProc is the API for specialized BLIT procs and patches.
 *	\param[in]	params	the common specifications for the BLIT procs.
 ********************************************************************************/

typedef void (*FskRectTransferProc)(const FskRectBlitParams *params);


/****************************************************************************//**
 * FskPatchRectBlitProc patches a specific BLIT proc.
 *
 * Replace the specified rect blit proc, returning the old proc, unless there is an error,
 * in which case the result is negative, with value consistent with FskErrors.h.
 * Note that NULL is not an error -- it only indicates that there was no blit proc
 * previously installed.
 *
 *	\param[in]	blitProc		the blit proc to be patched in.
 *	\param[in]	srcPixelFormat	the    source   pixel format implemented in this BLIT proc.
 *	\param[in]	dstPixelFormat	the destination pixel format implemented in this BLIT proc.
 *	\param[in]	mode			the transfer mode implemented in this BLIT proc.
 *	\param[in]	canBlend		Boolean to indicate whether this proc can blend.
 *	\return		the previous BLIT proc found in this slot.
 ********************************************************************************/

FskRectTransferProc		FskPatchRectBlitProc(
		FskRectTransferProc		blitProc,
		UInt32					srcPixelFormat,
		UInt32					dstPixelFormat,
		UInt32					mode,
		UInt32					canBlend
);


/****************************************************************************//**
 * FskPatchUnityRectBlitProc patches a specific unity BLIT proc.
 *
 * Replace the specified unity rect blit proc, returning the old proc, unless there is an error,
 * in which case the result is negative, with value consistent with FskErrors.h.
 * Note that NULL is not an error -- it only indicates that there was no blit proc
 * previously installed.
 * Note that only kFskGraphicsModeCopy and kFskGraphicsModeAlpha have optimized
 * unity rect blit procs, and that other modes will return kFskErrInvalidParameter.
 *
 *	\param[in]	blitProc		the blit proc to be patched in.
 *	\param[in]	srcPixelFormat	the    source   pixel format implemented in this BLIT proc.
 *	\param[in]	dstPixelFormat	the destination pixel format implemented in this BLIT proc.
 *	\param[in]	mode			the transfer mode implemented in this BLIT proc.
 *	\param[in]	canBlend		Boolean to indicate whether this proc can blend.
 *	\return		the previous BLIT proc found in this slot.
 ********************************************************************************/

FskRectTransferProc		FskPatchUnityRectBlitProc(
	FskRectTransferProc		blitProc,
	UInt32					srcPixelFormat,
	UInt32					dstPixelFormat,
	UInt32					mode,
	UInt32					canBlend
);


/****************************************************************************//**
 * FskPatchAlphaBlitProc patches a specific alpha BLIT proc.
 *
 * Replace the specified alpha blit proc, returning the old proc, unless there is an error,
 * in which case the result is negative, with value consistent with FskErrors.h.
 * Note that NULL is not an error -- it only indicates that there was no blit proc
 * previously installed.
 *	\param[in]	blitProc		the blit proc to be patched in.
 *	\param[in]	dstPixelFormat	the destination pixel format implemented in this BLIT proc.
 *	\return		the previous BLIT proc found in this slot.
 ********************************************************************************/

FskRectTransferProc		FskPatchAlphaBlitProc(
	FskRectTransferProc		blitProc,
	UInt32					dstPixelFormat
);


void my_FskCopyYUV420i16RGB565SE_scale_bc_arm_v6(const FskRectBlitParams *p);
void my_FskCopyYUV420i16RGB565SE_unity_bc_arm_v5(const FskRectBlitParams *p);
void my_FskCopyYUV420i16RGB565SE_scale_bc_arm_v5(const FskRectBlitParams *p);
void my_FskCopyYUV420i16RGB565SE_unity_bc_arm_v4(const FskRectBlitParams *p);
void my_FskCopyYUV420i16RGB565SE_scale_bc_arm_v4(const FskRectBlitParams *p);

void my_FskCopyYUV420i32BGRA_scale_bc_arm_v6(    const FskRectBlitParams *p);
void my_FskCopyYUV420i32BGRA_unity_bc_arm_v5(    const FskRectBlitParams *p);
void my_FskCopyYUV420i32BGRA_scale_bc_arm_v5(    const FskRectBlitParams *p);
void my_FskCopyYUV420i32BGRA_unity_bc_arm_v4(    const FskRectBlitParams *p);
void my_FskCopyYUV420i32BGRA_scale_bc_arm_v4(    const FskRectBlitParams *p);

void my_FskCopyYUV420i16RGB565SE_unity_bc_c( const FskRectBlitParams *p);
void my_FskCopyYUV420i16RGB565SE_scale_bc_c( const FskRectBlitParams *p);
void my_FskCopyYUV420i16RGB565SE_unity_bc_c( const FskRectBlitParams *p);
void my_FskCopyYUV420i16RGB565SE_scale_bc_c( const FskRectBlitParams *p);
void my_FskCopyYUV420i32BGRA_unity_bc_c(	 const FskRectBlitParams *p);
void my_FskCopyYUV420i32BGRA_scale_bc_c(	 const FskRectBlitParams *p);
void my_FskCopyYUV420i32BGRA_unity_bc_c(	 const FskRectBlitParams *p);
void my_FskCopyYUV420i32BGRA_scale_bc_c(	 const FskRectBlitParams *p);

//#ifdef SUPPORT_NEON
void my_FskCopyYUV420i16RGB565SE_scale_bc_arm_v7(const FskRectBlitParams *p);
void my_FskCopyYUV420i16RGB565SE_scale_bc_neon1_c(  const FskRectBlitParams *p);

void my_FskCopyYUV420i32BGRA_scale_bc_arm_v7(    const FskRectBlitParams *p);
void my_FskCopyYUV420i32BGRA_scale_bc_neon1_c(		const FskRectBlitParams *p);
//#endif

//#ifdef SUPPORT_WMMX
void my_FskCopyYUV420i16RGB565SE_scale_bc_arm_wmmx(const FskRectBlitParams *p);
void my_FskCopyYUV420i32BGRA_scale_bc_arm_wmmx(    const FskRectBlitParams *p);
//#ifdef SUPPORT_YUV420_WMMX_OPT
void my_FskCopyYUV42016RGB565SE_scale_bc_arm_wmmx(const FskRectBlitParams *p);
void my_FskCopyYUV42032BGRA_scale_bc_arm_wmmx(    const FskRectBlitParams *p);
//#endif
//#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __FSKRECTBLITPATCH__ */

