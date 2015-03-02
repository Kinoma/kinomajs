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
#ifndef __FSKYUV422TO420__
#define __FSKYUV422TO420__


#ifndef __FSK__
	#include "Fsk.h"
#endif /* __FSK__ */


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */



		/********************************************************************************
		 * Nominal range, offset 128 chroma, u y v y
		 ********************************************************************************/

FskAPI(void)	FskUYVY422toYUV420(
			UInt32			width,
			UInt32			height,
			const UInt32	*src,				/* Cb' Y' Cr' Y' */
			SInt32			srcRowBytes,
			UInt8			*dstY,
			SInt32			dstYRowBytes,
			UInt8			*dstU,				/* Cb' */
			UInt8			*dstV,				/* Cr' */
			SInt32			dstUVRowBytes
		);


		/********************************************************************************
		 * Nominal range, offset 128 chroma, y u y v
		 ********************************************************************************/

FskAPI(void)	FskYUYV422toYUV420(
			UInt32			width,
			UInt32			height,
			const UInt32	*src,				/* Y' Cb' Y' Cr' */
			SInt32			srcRowBytes,
			UInt8			*dstY,
			SInt32			dstYRowBytes,
			UInt8			*dstU,				/* Cb' */
			UInt8			*dstV,				/* Cr' */
			SInt32			dstUVRowBytes
		);


		/********************************************************************************
		 * Nominal range, two's complement chroma, u y v y
		 ********************************************************************************/

FskAPI(void)	FskSignedUYVY422toYUV420(
			UInt32			width,
			UInt32			height,
			
			const UInt32	*src,				/* Cb' Y' Cr' Y' src */
			SInt32			srcRB,
			
			UInt8			*dstY0,				/* Y'  dst */
			SInt32			dstYRB,
			UInt8			*dstU,				/* Cb' dst */
			UInt8			*dstV,				/* Cr' dst */
			SInt32			dstUVRB
		);


		/********************************************************************************
		 * Nominal range, two's complement chroma, y u y v
		 ********************************************************************************/

FskAPI(void)	FskSignedYUYV422toYUV420(
			UInt32			width,
			UInt32			height,
			
			const UInt32	*src,				/* Y' Cb' Y' Cr' src */
			SInt32			srcRB,
			
			UInt8			*dstY0,				/* Y'  dst */
			SInt32			dstYRB,
			UInt8			*dstU,				/* Cb' dst */
			UInt8			*dstV,				/* Cr' dst */
			SInt32			dstUVRB
		);


		/********************************************************************************
		 * Full range, offset 128 chroma, u y v y
		 ********************************************************************************/

FskAPI(void)	FskFullUYVY422toYUV420(
			UInt32			width,
			UInt32			height,
			const UInt32	*src,				/* Cb' Y' Cr' Y' src */
			SInt32			srcRB,
			UInt8			*dstY0,				/* Y'  dst */
			SInt32			dstYRB,
			UInt8			*dstU,				/* Cb' dst */
			UInt8			*dstV,				/* Cr' dst */
			SInt32			dstUVRB
		);


		/********************************************************************************
		 * Full range, offset 128 chroma, y u y v
		 ********************************************************************************/

FskAPI(void)	FskFullYUYV422toYUV420(
			UInt32			width,
			UInt32			height,
			const UInt32	*src,				/* Y' Cb' Y' Cr' src */
			SInt32			srcRB,
			UInt8			*dstY0,				/* Y'  dst */
			SInt32			dstYRB,
			UInt8			*dstU,				/* Cb' dst */
			UInt8			*dstV,				/* Cr' dst */
			SInt32			dstUVRB
		);


		/********************************************************************************
		 * Full range, two's complement chroma, u y v y
		 ********************************************************************************/

FskAPI(void)	FskFullSignedUYVY422toYUV420(
			UInt32			width,
			UInt32			height,
			const UInt32	*src,				/* Cb' Y' Cr' Y' src */
			SInt32			srcRB,
			UInt8			*dstY0,				/* Y'  dst */
			SInt32			dstYRB,
			UInt8			*dstU,				/* Cb' dst */
			UInt8			*dstV,				/* Cr' dst */
			SInt32			dstUVRB
		);


		/********************************************************************************
		 * Full range, two's complement chroma, y u y v
		 ********************************************************************************/

FskAPI(void)	FskFullSignedYUYV422toYUV420(
			UInt32			width,
			UInt32			height,
			const UInt32	*src,				/* Y' Cb' Y' Cr' src */
			SInt32			srcRB,
			UInt8			*dstY0,				/* Y'  dst */
			SInt32			dstYRB,
			UInt8			*dstU,				/* Cb' dst */
			UInt8			*dstV,				/* Cr' dst */
			SInt32			dstUVRB
		);


		/********************************************************************************
		 * Nominal range, two's complement chroma, y u y v, to MPEG-4 4:2:0
		 ********************************************************************************/

FskAPI(void)	FskSignedYUYV422toMPEG4YUV420(
			UInt32			width,
			UInt32			height,
			
			const UInt32	*src,				/* Y' Cb' Y' Cr' src */
			SInt32			srcRB,
			
			UInt8			*dstY0,				/* Y'  dst */
			SInt32			dstYRB,
			UInt8			*dstU,				/* Cb' dst */
			UInt8			*dstV,				/* Cr' dst */
			SInt32			dstUVRB
		);


		/********************************************************************************
		 * Nominal range, two's complement chroma, u y v y, to MPEG-4 4:2:0
		 ********************************************************************************/

FskAPI(void)	FskSignedUYVY422toMPEG4YUV420(
			UInt32			width,
			UInt32			height,
			
			const UInt32	*src,				/* Y' Cb' Y' Cr' src */
			SInt32			srcRB,
			
			UInt8			*dstY0,				/* Y'  dst */
			SInt32			dstYRB,
			UInt8			*dstU,				/* Cb' dst */
			UInt8			*dstV,				/* Cr' dst */
			SInt32			dstUVRB
		);


		/********************************************************************************
		 * Wide range, two's complement chroma, y u y v, to MPEG-4 4:2:0
		 ********************************************************************************/

FskAPI(void)	FskWideSignedYUYV422toMPEG4YUV420(
			UInt32			width,
			UInt32			height,
			
			const UInt32	*src,				/* Y' Cb' Y' Cr' src */
			SInt32			srcRB,
			
			UInt8			*dstY0,				/* Y'  dst */
			SInt32			dstYRB,
			UInt8			*dstU,				/* Cb' dst */
			UInt8			*dstV,				/* Cr' dst */
			SInt32			dstUVRB
		);

		/********************************************************************************
		 * Wide range, two's complement chroma, u y v y, to MPEG-4 4:2:0
		 ********************************************************************************/

FskAPI(void)	FskWideSignedUYVY422toMPEG4YUV420(
			UInt32			width,
			UInt32			height,
			
			const UInt32	*src,				/* Y' Cb' Y' Cr' src */
			SInt32			srcRB,
			
			UInt8			*dstY0,				/* Y'  dst */
			SInt32			dstYRB,
			UInt8			*dstU,				/* Cb' dst */
			UInt8			*dstV,				/* Cr' dst */
			SInt32			dstUVRB
		);

		/********************************************************************************
		 * Video range, offset 128 chroma, u y v y, to MPEG-4 4:2:0
		 ********************************************************************************/

FskAPI(void)	FskYUYV422toMPEG4YUV420(
			UInt32			width,
			UInt32			height,
			
			const UInt32	*src,				/* Y' Cb' Y' Cr' src */
			SInt32			srcRB,
			
			UInt8			*dstY0,				/* Y'  dst */
			SInt32			dstYRB,
			UInt8			*dstU,				/* Cb' dst */
			UInt8			*dstV,				/* Cr' dst */
			SInt32			dstUVRB
		);

		/********************************************************************************
		 * Video range,  offset 128 chroma, u y v y, to MPEG-4 4:2:0
		 ********************************************************************************/

FskAPI(void)	FskUYVY422toMPEG4YUV420(
			UInt32			width,
			UInt32			height,
			
			const UInt32	*src,				/* Y' Cb' Y' Cr' src */
			SInt32			srcRB,
			
			UInt8			*dstY0,				/* Y'  dst */
			SInt32			dstYRB,
			UInt8			*dstU,				/* Cb' dst */
			UInt8			*dstV,				/* Cr' dst */
			SInt32			dstUVRB
		);



		/********************************************************************************
		 * Macros for formats with V before U
		 ********************************************************************************/

#define FskVYUY422toYUV420(w, h, s, srb, dy, dyrb, du, dv, duvrb)					FskUYVY422toYUV420(w, h, s, srb, dy, dyrb, dv, du, duvrb)					/* Cr' Y' Cb' Y' */
#define FskYVYU422toYUV420(w, h, s, srb, dy, dyrb, du, dv, duvrb)					FskYUYV422toYUV420(w, h, s, srb, dy, dyrb, dv, du, duvrb)					/* Y' Cr' Y' Cb' */

#define FskSignedVYUY422toYUV420(w, h, s, srb, dy, dyrb, du, dv, duvrb)				FskSignedUYVY422toYUV420(w, h, s, srb, dy, dyrb, dv, du, duvrb)				/* Cr' Y' Cb' Y' */
#define FskSignedYVYU422toYUV420(w, h, s, srb, dy, dyrb, du, dv, duvrb)				FskSignedYUYV422toYUV420(w, h, s, srb, dy, dyrb, dv, du, duvrb)				/* Y' Cr' Y' Cb' */

#define FskFullVYUY422toYUV420(w, h, s, srb, dy, dyrb, du, dv, duvrb)				FskFullUYVY422toYUV420(w, h, s, srb, dy, dyrb, dv, du, duvrb)				/* Cr' Y' Cb' Y' */
#define FskFullYVYU422toYUV420(w, h, s, srb, dy, dyrb, du, dv, duvrb)				FskFullYUYV422toYUV420(w, h, s, srb, dy, dyrb, dv, du, duvrb)				/* Y' Cr' Y' Cb' */

#define FskFullSignedVYUY422toYUV420(w, h, s, srb, dy, dyrb, du, dv, duvrb)			FskFullSignedUYVY422toYUV420(w, h, s, srb, dy, dyrb, dv, du, duvrb)			/* Cr' Y' Cb' Y' */
#define FskFullSignedYVYU422toYUV420(w, h, s, srb, dy, dyrb, du, dv, duvrb)			FskFullSignedYUYV422toYUV420(w, h, s, srb, dy, dyrb, dv, du, duvrb)			/* Y' Cr' Y' Cb' */

#define FskSignedVYUY422toMPEG4YUV420(w, h, s, srb, dy, dyrb, du, dv, duvrb)		FskSignedUYVY422toMPEG4YUV420(w, h, s, srb, dy, dyrb, dv, du, duvrb)		/* Cr' Y' Cb' Y' */
#define FskSignedYVYU422toMPEG4YUV420(w, h, s, srb, dy, dyrb, du, dv, duvrb)		FskSignedYUYV422toMPEG4YUV420(w, h, s, srb, dy, dyrb, dv, du, duvrb)		/* Y' Cr' Y' Cb' */

#define FskWideSignedYVYU422toMPEG4YUV420(w, h, s, srb, dy, dyrb, du, dv, duvrb)	FskWideSignedYUYV422toMPEG4YUV420(w, h, s, srb, dy, dyrb, dv, du, duvrb)	/* Cr' Y' Cb' Y' */
#define FskWideSignedVYUY422toMPEG4YUV420(w, h, s, srb, dy, dyrb, du, dv, duvrb)	FskWideSignedUYVY422toMPEG4YUV420(w, h, s, srb, dy, dyrb, dv, du, duvrb)	/* Y' Cr' Y' Cb' */

#define FskYVYU422toMPEG4YUV420(w, h, s, srb, dy, dyrb, du, dv, duvrb)				FskYUYV422toMPEG4YUV420(w, h, s, srb, dy, dyrb, dv, du, duvrb)				/* Cr' Y' Cb' Y' */
#define FskVYUY422toMPEG4YUV420(w, h, s, srb, dy, dyrb, du, dv, duvrb)				FskUYVY422toMPEG4YUV420(w, h, s, srb, dy, dyrb, dv, du, duvrb)				/* Y' Cr' Y' Cb' */




#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __FSKYUV422TO420__ */

