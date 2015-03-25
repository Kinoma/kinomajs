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
#ifndef __FSKBLITDISPATCHDEF__
#define __FSKBLITDISPATCHDEF__

#ifndef __FSKPIXELOPS__
# include "FskPixelOps.h"
#endif /* __FSKPIXELOPS__ */

#ifndef __FSKBITMAP__
 #include "FskBitmap.h"
#endif /* __FSKBITMAP__ */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/*****************************************************************
 *****************************************************************
 **			We count the number of enabled pixel formats		**
 **					(order does not matter here)				**
 *****************************************************************
 *****************************************************************/

#define NUM_SRC_FORMATS	(	SRC_32ARGB			\
						+	SRC_32BGRA			\
						+	SRC_32RGBA			\
						+	SRC_32ABGR			\
						+	SRC_24BGR			\
						+	SRC_24RGB			\
						+	SRC_16RGB565BE		\
						+	SRC_16RGB565LE		\
						+	SRC_16RGB5515LE		\
						+	SRC_16RGB5515BE		\
						+	SRC_16BGR565LE		\
						+	SRC_16RGBA4444LE	\
						+	SRC_16RGBA4444BE	\
						+	SRC_16AG			\
						+	SRC_YUV420			\
						+	SRC_YUV422			\
						+	SRC_8A				\
						+	SRC_8G				\
						+	SRC_YUV420spuv		\
						+	SRC_UYVY			\
						+	SRC_YUV420i			\
						+	SRC_32A16RGB565LE	\
						+	SRC_YUV420spvu		\
						)
						/****************************************
						 *	^  ADD NEW PIXEL FORMATS HERE  ^	*
						 ****************************************/

#define NUM_DST_FORMATS	(	DST_32ARGB			\
						+	DST_32BGRA			\
						+	DST_32RGBA			\
						+	DST_32ABGR			\
						+	DST_24BGR			\
						+	DST_24RGB			\
						+	DST_16RGB565BE		\
						+	DST_16RGB565LE		\
						+	DST_16RGB5515LE		\
						+	DST_16RGB5515BE		\
						+	DST_16BGR565LE		\
						+	DST_16RGBA4444LE	\
						+	DST_16RGBA4444BE	\
						+	DST_16AG			\
						+	DST_YUV420			\
						+	DST_YUV422			\
						+	DST_8A				\
						+	DST_8G				\
						+	DST_YUV420spuv		\
						+	DST_UYVY			\
						+	DST_UYVY			\
						+	DST_32A16RGB565LE	\
						+	DST_YUV420spvu		\
						)
						/****************************************
						 *	^  ADD NEW PIXEL FORMATS HERE  ^	*
						 ****************************************/

#define NUM_DST_UNITY_FORMATS	(	DST_UNITY_32ARGB		\
								+	DST_UNITY_32BGRA		\
								+	DST_UNITY_32RGBA		\
								+	DST_UNITY_32ABGR		\
								+	DST_UNITY_24BGR			\
								+	DST_UNITY_24RGB			\
								+	DST_UNITY_16RGB565BE	\
								+	DST_UNITY_16RGB565LE	\
								+	DST_UNITY_16RGB5515LE	\
								+	DST_UNITY_16RGB5515BE	\
								+	DST_UNITY_16BGR565LE	\
								+	DST_UNITY_16RGBA4444LE	\
								+	DST_UNITY_16RGBA4444BE	\
								+	DST_UNITY_16AG			\
								+	DST_UNITY_YUV420		\
								+	DST_UNITY_YUV422		\
								+	DST_UNITY_8A			\
								+	DST_UNITY_8G			\
								+	DST_UNITY_YUV420spuv	\
								+	DST_UNITY_UYVY			\
								+	DST_UNITY_32A16RGB565LE	\
								+	DST_UNITY_YUV420spvu	\
								)
						/****************************************
						 *	^  ADD NEW PIXEL FORMATS HERE  ^	*
						 ****************************************/


/*****************************************************************
 *****************************************************************
 **		We assign indices to be used in the dispatch table		**
 **																**
 **		Order DOES matter here, though not critically so.		**
 **																**
 **		We usually put the most popular pixel formats first,	**
 **		where "popular" here refers to the number of places		**
 **		in this code where the formats are used.				**
 **		In particular, we can truncate some dispatch tables		**
 **		easily if the unused pixel formats are at the end.		**
 *****************************************************************
 *****************************************************************/


/********************************************************************************
 *								Source Kind Indices								*
 ********************************************************************************/
enum {
	SRC_INDEX_INTRODUCTION										= -1						/* kFskBitmapFormatUnknown */


	/* The enabled pixel formats will get a sequential index here */


	/* 32 ARGB */
	#if SRC_32ARGB
		#if   !defined(SRC_KIND_0)
			#define SRC_KIND_0				fsk32ARGBFormatKind	/* 0 */
		#else
			#error Unexpected predefinition of SRC_KIND_0
		#endif
		,SRC_INDEX_32ARGB
	#endif /* SRC_32ARGB */


	/* 32 BGRA */
	#if SRC_32BGRA
		#if   !defined(SRC_KIND_0)
			#   define SRC_KIND_0			fsk32BGRAFormatKind	/* 0 */
		#elif !defined(SRC_KIND_1)
			#   define SRC_KIND_1			fsk32BGRAFormatKind	/* 1 */
		#endif
		,SRC_INDEX_32BGRA
	#endif /* SRC_32BGRA */


	/* 32 RGBA */
	#if SRC_32RGBA
		#if   !defined(SRC_KIND_0)
			#   define SRC_KIND_0			fsk32RGBAFormatKind	/* 0 */
		#elif !defined(SRC_KIND_1)
			#   define SRC_KIND_1			fsk32RGBAFormatKind	/* 1 */
		#elif !defined(SRC_KIND_2)
			#   define SRC_KIND_2			fsk32RGBAFormatKind	/* 2 */
		#endif
		,SRC_INDEX_32RGBA
	#endif /* SRC_32RGBA */


	/* 32 ABGR */
	#if SRC_32ABGR
		#if   !defined(SRC_KIND_0)
			#   define SRC_KIND_0			fsk32ABGRFormatKind	/* 0 */
		#elif !defined(SRC_KIND_1)
			#   define SRC_KIND_1			fsk32ABGRFormatKind	/* 1 */
		#elif !defined(SRC_KIND_2)
			#   define SRC_KIND_2			fsk32ABGRFormatKind	/* 2 */
		#elif !defined(SRC_KIND_3)
			#   define SRC_KIND_3			fsk32ABGRFormatKind	/* 3 */
		#endif
		,SRC_INDEX_32ABGR
	#endif /* SRC_32RGBA */


	/* 24 BGR */
	#if SRC_24BGR
		#if   !defined(SRC_KIND_0)
			#   define SRC_KIND_0			fsk24BGRFormatKind	/* 0 */
		#elif !defined(SRC_KIND_1)
			#   define SRC_KIND_1			fsk24BGRFormatKind	/* 1 */
		#elif !defined(SRC_KIND_2)
			#   define SRC_KIND_2			fsk24BGRFormatKind	/* 2 */
		#elif !defined(SRC_KIND_3)
			#   define SRC_KIND_3			fsk24BGRFormatKind	/* 3 */
		#elif !defined(SRC_KIND_4)
			#   define SRC_KIND_4			fsk24BGRFormatKind	/* 4 */
		#endif
		,SRC_INDEX_24BGR
	#endif /* SRC_24BGR */


	/* 24 RGB */
	#if SRC_24RGB
		#if   !defined(SRC_KIND_0)
			#   define SRC_KIND_0			fsk24RGBFormatKind	/* 0 */
		#elif !defined(SRC_KIND_1)
			#   define SRC_KIND_1			fsk24RGBFormatKind	/* 1 */
		#elif !defined(SRC_KIND_2)
			#   define SRC_KIND_2			fsk24RGBFormatKind	/* 2 */
		#elif !defined(SRC_KIND_3)
			#   define SRC_KIND_3			fsk24RGBFormatKind	/* 3 */
		#elif !defined(SRC_KIND_4)
			#   define SRC_KIND_4			fsk24RGBFormatKind	/* 4 */
		#elif !defined(SRC_KIND_5)
			#   define SRC_KIND_5			fsk24RGBFormatKind	/* 5 */
		#endif
		,SRC_INDEX_24RGB
	#endif /* SRC_24RGB */


	/* 16 RGB 565 BE */
	#if SRC_16RGB565BE
		#if   !defined(SRC_KIND_0)
			#   define SRC_KIND_0			fsk16RGB565BEFormatKind	/* 0 */
		#elif !defined(SRC_KIND_1)
			#   define SRC_KIND_1			fsk16RGB565BEFormatKind	/* 1 */
		#elif !defined(SRC_KIND_2)
			#   define SRC_KIND_2			fsk16RGB565BEFormatKind	/* 2 */
		#elif !defined(SRC_KIND_3)
			#   define SRC_KIND_3			fsk16RGB565BEFormatKind	/* 3 */
		#elif !defined(SRC_KIND_4)
			#   define SRC_KIND_4			fsk16RGB565BEFormatKind	/* 4 */
		#elif !defined(SRC_KIND_5)
			#   define SRC_KIND_5			fsk16RGB565BEFormatKind	/* 5 */
		#elif !defined(SRC_KIND_6)
			#   define SRC_KIND_6			fsk16RGB565BEFormatKind	/* 6 */
		#endif
		,SRC_INDEX_16RGB565BE
	#endif /* SRC_16RGB565BE */


	/* 16 RGB 565 LE */
	#if SRC_16RGB565LE
		#if   !defined(SRC_KIND_0)
			#   define SRC_KIND_0			fsk16RGB565LEFormatKind	/* 0 */
		#elif !defined(SRC_KIND_1)
			#   define SRC_KIND_1			fsk16RGB565LEFormatKind	/* 1 */
		#elif !defined(SRC_KIND_2)
			#   define SRC_KIND_2			fsk16RGB565LEFormatKind	/* 2 */
		#elif !defined(SRC_KIND_3)
			#   define SRC_KIND_3			fsk16RGB565LEFormatKind	/* 3 */
		#elif !defined(SRC_KIND_4)
			#   define SRC_KIND_4			fsk16RGB565LEFormatKind	/* 4 */
		#elif !defined(SRC_KIND_5)
			#   define SRC_KIND_5			fsk16RGB565LEFormatKind	/* 5 */
		#elif !defined(SRC_KIND_6)
			#   define SRC_KIND_6			fsk16RGB565LEFormatKind	/* 6 */
		#elif !defined(SRC_KIND_7)
			#   define SRC_KIND_7			fsk16RGB565LEFormatKind	/* 7 */
		#endif
		,SRC_INDEX_16RGB565LE
	#endif /* SRC_16RGB565LE */


	/* 16 BGR 565 LE */
	#if SRC_16BGR565LE
		#if   !defined(SRC_KIND_0)
			#   define SRC_KIND_0			fsk16BGR565LEFormatKind	/* 0 */
		#elif !defined(SRC_KIND_1)
			#   define SRC_KIND_1			fsk16BGR565LEFormatKind	/* 1 */
		#elif !defined(SRC_KIND_2)
			#   define SRC_KIND_2			fsk16BGR565LEFormatKind	/* 2 */
		#elif !defined(SRC_KIND_3)
			#   define SRC_KIND_3			fsk16BGR565LEFormatKind	/* 3 */
		#elif !defined(SRC_KIND_4)
			#   define SRC_KIND_4			fsk16BGR565LEFormatKind	/* 4 */
		#elif !defined(SRC_KIND_5)
			#   define SRC_KIND_5			fsk16BGR565LEFormatKind	/* 5 */
		#elif !defined(SRC_KIND_6)
			#   define SRC_KIND_6			fsk16BGR565LEFormatKind	/* 6 */
		#elif !defined(SRC_KIND_7)
			#   define SRC_KIND_7			fsk16BGR565LEFormatKind	/* 7 */
		#elif !defined(SRC_KIND_8)
			#   define SRC_KIND_8			fsk16BGR565LEFormatKind	/* 8 */
		#endif
		,SRC_INDEX_16BGR565LE
	#endif /* SRC_16BGR565LE */


	/* 16 RGB 5515 LE */
	#if SRC_16RGB5515LE
		#if   !defined(SRC_KIND_0)
			#   define SRC_KIND_0			fsk16RGB5515LEFormatKind	/* 0 */
		#elif !defined(SRC_KIND_1)
			#   define SRC_KIND_1			fsk16RGB5515LEFormatKind	/* 1 */
		#elif !defined(SRC_KIND_2)
			#   define SRC_KIND_2			fsk16RGB5515LEFormatKind	/* 2 */
		#elif !defined(SRC_KIND_3)
			#   define SRC_KIND_3			fsk16RGB5515LEFormatKind	/* 3 */
		#elif !defined(SRC_KIND_4)
			#   define SRC_KIND_4			fsk16RGB5515LEFormatKind	/* 4 */
		#elif !defined(SRC_KIND_5)
			#   define SRC_KIND_5			fsk16RGB5515LEFormatKind	/* 5 */
		#elif !defined(SRC_KIND_6)
			#   define SRC_KIND_6			fsk16RGB5515LEFormatKind	/* 6 */
		#elif !defined(SRC_KIND_7)
			#   define SRC_KIND_7			fsk16RGB5515LEFormatKind	/* 7 */
		#elif !defined(SRC_KIND_8)
			#   define SRC_KIND_8			fsk16RGB5515LEFormatKind	/* 8 */
		#elif !defined(SRC_KIND_9)
			#   define SRC_KIND_9			fsk16RGB5515LEFormatKind	/* 9 */
		#endif
		,SRC_INDEX_16RGB5515LE
	#endif /* SRC_16RGB5515LE */


	/* 16 RGB 5515 BE */
	#if SRC_16RGB5515BE
		#if   !defined(SRC_KIND_0)
			#   define SRC_KIND_0			fsk16RGB5515BEFormatKind	/* 0 */
		#elif !defined(SRC_KIND_1)
			#   define SRC_KIND_1			fsk16RGB5515BEFormatKind	/* 1 */
		#elif !defined(SRC_KIND_2)
			#   define SRC_KIND_2			fsk16RGB5515BEFormatKind	/* 2 */
		#elif !defined(SRC_KIND_3)
			#   define SRC_KIND_3			fsk16RGB5515BEFormatKind	/* 3 */
		#elif !defined(SRC_KIND_4)
			#   define SRC_KIND_4			fsk16RGB5515BEFormatKind	/* 4 */
		#elif !defined(SRC_KIND_5)
			#   define SRC_KIND_5			fsk16RGB5515BEFormatKind	/* 5 */
		#elif !defined(SRC_KIND_6)
			#   define SRC_KIND_6			fsk16RGB5515BEFormatKind	/* 6 */
		#elif !defined(SRC_KIND_7)
			#   define SRC_KIND_7			fsk16RGB5515BEFormatKind	/* 7 */
		#elif !defined(SRC_KIND_8)
			#   define SRC_KIND_8			fsk16RGB5515BEFormatKind	/* 8 */
		#elif !defined(SRC_KIND_9)
			#   define SRC_KIND_9			fsk16RGB5515BEFormatKind	/* 9 */
		#elif !defined(SRC_KIND_10)
			#   define SRC_KIND_10			fsk16RGB5515BEFormatKind	/* 10 */
		#endif
		,SRC_INDEX_16RGB5515BE
	#endif /* SRC_16RGB5515BE */


	/* 16 RGBA 4444 LE */
	#if SRC_16RGBA4444LE
		#if   !defined(SRC_KIND_0)
			#   define SRC_KIND_0			fsk16RGBA4444LEFormatKind	/* 0 */
		#elif !defined(SRC_KIND_1)
			#   define SRC_KIND_1			fsk16RGBA4444LEFormatKind	/* 1 */
		#elif !defined(SRC_KIND_2)
			#   define SRC_KIND_2			fsk16RGBA4444LEFormatKind	/* 2 */
		#elif !defined(SRC_KIND_3)
			#   define SRC_KIND_3			fsk16RGBA4444LEFormatKind	/* 3 */
		#elif !defined(SRC_KIND_4)
			#   define SRC_KIND_4			fsk16RGBA4444LEFormatKind	/* 4 */
		#elif !defined(SRC_KIND_5)
			#   define SRC_KIND_5			fsk16RGBA4444LEFormatKind	/* 5 */
		#elif !defined(SRC_KIND_6)
			#   define SRC_KIND_6			fsk16RGBA4444LEFormatKind	/* 6 */
		#elif !defined(SRC_KIND_7)
			#   define SRC_KIND_7			fsk16RGBA4444LEFormatKind	/* 7 */
		#elif !defined(SRC_KIND_8)
			#   define SRC_KIND_8			fsk16RGBA4444LEFormatKind	/* 8 */
		#elif !defined(SRC_KIND_9)
			#   define SRC_KIND_9			fsk16RGBA4444LEFormatKind	/* 9 */
		#elif !defined(SRC_KIND_10)
			#   define SRC_KIND_10			fsk16RGBA4444LEFormatKind	/* 10 */
		#elif !defined(SRC_KIND_11)
			#   define SRC_KIND_11			fsk16RGBA4444LEFormatKind	/* 11 */
		#endif
		,SRC_INDEX_16RGBA4444LE
	#endif /* SRC_16RGBA4444LE */


	/* 16 RGBA 4444 BE */
	#if SRC_16RGBA4444BE
		#if   !defined(SRC_KIND_0)
			#   define SRC_KIND_0			fsk16RGBA4444BEFormatKind	/* 0 */
		#elif !defined(SRC_KIND_1)
			#   define SRC_KIND_1			fsk16RGBA4444BEFormatKind	/* 1 */
		#elif !defined(SRC_KIND_2)
			#   define SRC_KIND_2			fsk16RGBA4444BEFormatKind	/* 2 */
		#elif !defined(SRC_KIND_3)
			#   define SRC_KIND_3			fsk16RGBA4444BEFormatKind	/* 3 */
		#elif !defined(SRC_KIND_4)
			#   define SRC_KIND_4			fsk16RGBA4444BEFormatKind	/* 4 */
		#elif !defined(SRC_KIND_5)
			#   define SRC_KIND_5			fsk16RGBA4444BEFormatKind	/* 5 */
		#elif !defined(SRC_KIND_6)
			#   define SRC_KIND_6			fsk16RGBA4444BEFormatKind	/* 6 */
		#elif !defined(SRC_KIND_7)
			#   define SRC_KIND_7			fsk16RGBA4444BEFormatKind	/* 7 */
		#elif !defined(SRC_KIND_8)
			#   define SRC_KIND_8			fsk16RGBA4444BEFormatKind	/* 8 */
		#elif !defined(SRC_KIND_9)
			#   define SRC_KIND_9			fsk16RGBA4444BEFormatKind	/* 9 */
		#elif !defined(SRC_KIND_10)
			#   define SRC_KIND_10			fsk16RGBA4444BEFormatKind	/* 10 */
		#elif !defined(SRC_KIND_11)
			#   define SRC_KIND_11			fsk16RGBA4444BEFormatKind	/* 11 */
		#elif !defined(SRC_KIND_12)
			#   define SRC_KIND_12			fsk16RGBA4444BEFormatKind	/* 12 */
		#endif
		,SRC_INDEX_16RGBA4444BE
	#endif /* SRC_16RGBA4444BE */


	/* 16 AG */
	#if SRC_16AG
		#if   !defined(SRC_KIND_0)
			#   define SRC_KIND_0			fsk16AGFormatKind	/* 0 */
		#elif !defined(SRC_KIND_1)
			#   define SRC_KIND_1			fsk16AGFormatKind	/* 1 */
		#elif !defined(SRC_KIND_2)
			#   define SRC_KIND_2			fsk16AGFormatKind	/* 2 */
		#elif !defined(SRC_KIND_3)
			#   define SRC_KIND_3			fsk16AGFormatKind	/* 3 */
		#elif !defined(SRC_KIND_4)
			#   define SRC_KIND_4			fsk16AGFormatKind	/* 4 */
		#elif !defined(SRC_KIND_5)
			#   define SRC_KIND_5			fsk16AGFormatKind	/* 5 */
		#elif !defined(SRC_KIND_6)
			#   define SRC_KIND_6			fsk16AGFormatKind	/* 6 */
		#elif !defined(SRC_KIND_7)
			#   define SRC_KIND_7			fsk16AGFormatKind	/* 7 */
		#elif !defined(SRC_KIND_8)
			#   define SRC_KIND_8			fsk16AGFormatKind	/* 8 */
		#elif !defined(SRC_KIND_9)
			#   define SRC_KIND_9			fsk16AGFormatKind	/* 9 */
		#elif !defined(SRC_KIND_10)
			#   define SRC_KIND_10			fsk16AGFormatKind	/* 10 */
		#elif !defined(SRC_KIND_11)
			#   define SRC_KIND_11			fsk16AGFormatKind	/* 11 */
		#elif !defined(SRC_KIND_12)
			#   define SRC_KIND_12			fsk16AGFormatKind	/* 12 */
		#elif !defined(SRC_KIND_13)
			#   define SRC_KIND_13			fsk16AGFormatKind	/* 13 */
		#endif
		,SRC_INDEX_16AG
	#endif /* SRC_16AG */


	/* 8 A */
	#if SRC_8A
		#if   !defined(SRC_KIND_0)
			#   define SRC_KIND_0			fsk8AFormatKind	/* 0 */
		#elif !defined(SRC_KIND_1)
			#   define SRC_KIND_1			fsk8AFormatKind	/* 1 */
		#elif !defined(SRC_KIND_2)
			#   define SRC_KIND_2			fsk8AFormatKind	/* 2 */
		#elif !defined(SRC_KIND_3)
			#   define SRC_KIND_3			fsk8AFormatKind	/* 3 */
		#elif !defined(SRC_KIND_4)
			#   define SRC_KIND_4			fsk8AFormatKind	/* 4 */
		#elif !defined(SRC_KIND_5)
			#   define SRC_KIND_5			fsk8AFormatKind	/* 5 */
		#elif !defined(SRC_KIND_6)
			#   define SRC_KIND_6			fsk8AFormatKind	/* 6 */
		#elif !defined(SRC_KIND_7)
			#   define SRC_KIND_7			fsk8AFormatKind	/* 7 */
		#elif !defined(SRC_KIND_8)
			#   define SRC_KIND_8			fsk8AFormatKind	/* 8 */
		#elif !defined(SRC_KIND_9)
			#   define SRC_KIND_9			fsk8AFormatKind	/* 9 */
		#elif !defined(SRC_KIND_10)
			#   define SRC_KIND_10			fsk8AFormatKind	/* 10 */
		#elif !defined(SRC_KIND_11)
			#   define SRC_KIND_11			fsk8AFormatKind	/* 11 */
		#elif !defined(SRC_KIND_12)
			#   define SRC_KIND_12			fsk8AFormatKind	/* 12 */
		#elif !defined(SRC_KIND_13)
			#   define SRC_KIND_13			fsk8AFormatKind	/* 13 */
		#elif !defined(SRC_KIND_14)
			#   define SRC_KIND_14			fsk8AFormatKind	/* 14 */
		#endif
		,SRC_INDEX_8A
	#endif /* SRC_8A */


	/* 8 G */
	#if SRC_8G
		#if   !defined(SRC_KIND_0)
			#   define SRC_KIND_0			fsk8GFormatKind	/* 0 */
		#elif !defined(SRC_KIND_1)
			#   define SRC_KIND_1			fsk8GFormatKind	/* 1 */
		#elif !defined(SRC_KIND_2)
			#   define SRC_KIND_2			fsk8GFormatKind	/* 2 */
		#elif !defined(SRC_KIND_3)
			#   define SRC_KIND_3			fsk8GFormatKind	/* 3 */
		#elif !defined(SRC_KIND_4)
			#   define SRC_KIND_4			fsk8GFormatKind	/* 4 */
		#elif !defined(SRC_KIND_5)
			#   define SRC_KIND_5			fsk8GFormatKind	/* 5 */
		#elif !defined(SRC_KIND_6)
			#   define SRC_KIND_6			fsk8GFormatKind	/* 6 */
		#elif !defined(SRC_KIND_7)
			#   define SRC_KIND_7			fsk8GFormatKind	/* 7 */
		#elif !defined(SRC_KIND_8)
			#   define SRC_KIND_8			fsk8GFormatKind	/* 8 */
		#elif !defined(SRC_KIND_9)
			#   define SRC_KIND_9			fsk8GFormatKind	/* 9 */
		#elif !defined(SRC_KIND_10)
			#   define SRC_KIND_10			fsk8GFormatKind	/* 10 */
		#elif !defined(SRC_KIND_11)
			#   define SRC_KIND_11			fsk8GFormatKind	/* 11 */
		#elif !defined(SRC_KIND_12)
			#   define SRC_KIND_12			fsk8GFormatKind	/* 12 */
		#elif !defined(SRC_KIND_13)
			#   define SRC_KIND_13			fsk8GFormatKind	/* 13 */
		#elif !defined(SRC_KIND_14)
			#   define SRC_KIND_14			fsk8GFormatKind	/* 14 */
		#elif !defined(SRC_KIND_15)
			#   define SRC_KIND_15			fsk8GFormatKind	/* 15 */
		#endif
		,SRC_INDEX_8G
	#endif /* SRC_8G */


	/* YUV 4:2:0 semi-planar */
	#if SRC_YUV420spuv
		#if   !defined(SRC_KIND_0)
			#   define SRC_KIND_0			fskYUV420spuvFormatKind	/* 0 */
		#elif !defined(SRC_KIND_1)
			#   define SRC_KIND_1			fskYUV420spuvFormatKind	/* 1 */
		#elif !defined(SRC_KIND_2)
			#   define SRC_KIND_2			fskYUV420spuvFormatKind	/* 2 */
		#elif !defined(SRC_KIND_3)
			#   define SRC_KIND_3			fskYUV420spuvFormatKind	/* 3 */
		#elif !defined(SRC_KIND_4)
			#   define SRC_KIND_4			fskYUV420spuvFormatKind	/* 4 */
		#elif !defined(SRC_KIND_5)
			#   define SRC_KIND_5			fskYUV420spuvFormatKind	/* 5 */
		#elif !defined(SRC_KIND_6)
			#   define SRC_KIND_6			fskYUV420spuvFormatKind	/* 6 */
		#elif !defined(SRC_KIND_7)
			#   define SRC_KIND_7			fskYUV420spuvFormatKind	/* 7 */
		#elif !defined(SRC_KIND_8)
			#   define SRC_KIND_8			fskYUV420spuvFormatKind	/* 8 */
		#elif !defined(SRC_KIND_9)
			#   define SRC_KIND_9			fskYUV420spuvFormatKind	/* 9 */
		#elif !defined(SRC_KIND_10)
			#   define SRC_KIND_10			fskYUV420spuvFormatKind	/* 10 */
		#elif !defined(SRC_KIND_11)
			#   define SRC_KIND_11			fskYUV420spuvFormatKind	/* 11 */
		#elif !defined(SRC_KIND_12)
			#   define SRC_KIND_12			fskYUV420spuvFormatKind	/* 12 */
		#elif !defined(SRC_KIND_13)
			#   define SRC_KIND_13			fskYUV420spuvFormatKind	/* 13 */
		#elif !defined(SRC_KIND_14)
			#   define SRC_KIND_14			fskYUV420spuvFormatKind	/* 14 */
		#elif !defined(SRC_KIND_15)
			#   define SRC_KIND_15			fskYUV420spuvFormatKind	/* 15 */
		#elif !defined(SRC_KIND_16)
			#   define SRC_KIND_16			fskYUV420spuvFormatKind	/* 16 */
		#endif
		,SRC_INDEX_YUV420spuv
	#endif /* SRC_YUV420sp */


	/* YUV 4:2:0 planar */
	#if SRC_YUV420
		#if   !defined(SRC_KIND_0)
			#   define SRC_KIND_0			fskYUV420FormatKind	/* 0 */
		#elif !defined(SRC_KIND_1)
			#   define SRC_KIND_1			fskYUV420FormatKind	/* 1 */
		#elif !defined(SRC_KIND_2)
			#   define SRC_KIND_2			fskYUV420FormatKind	/* 2 */
		#elif !defined(SRC_KIND_3)
			#   define SRC_KIND_3			fskYUV420FormatKind	/* 3 */
		#elif !defined(SRC_KIND_4)
			#   define SRC_KIND_4			fskYUV420FormatKind	/* 4 */
		#elif !defined(SRC_KIND_5)
			#   define SRC_KIND_5			fskYUV420FormatKind	/* 5 */
		#elif !defined(SRC_KIND_6)
			#   define SRC_KIND_6			fskYUV420FormatKind	/* 6 */
		#elif !defined(SRC_KIND_7)
			#   define SRC_KIND_7			fskYUV420FormatKind	/* 7 */
		#elif !defined(SRC_KIND_8)
			#   define SRC_KIND_8			fskYUV420FormatKind	/* 8 */
		#elif !defined(SRC_KIND_9)
			#   define SRC_KIND_9			fskYUV420FormatKind	/* 9 */
		#elif !defined(SRC_KIND_10)
			#   define SRC_KIND_10			fskYUV420FormatKind	/* 10 */
		#elif !defined(SRC_KIND_11)
			#   define SRC_KIND_11			fskYUV420FormatKind	/* 11 */
		#elif !defined(SRC_KIND_12)
			#   define SRC_KIND_12			fskYUV420FormatKind	/* 12 */
		#elif !defined(SRC_KIND_13)
			#   define SRC_KIND_13			fskYUV420FormatKind	/* 13 */
		#elif !defined(SRC_KIND_14)
			#   define SRC_KIND_14			fskYUV420FormatKind	/* 14 */
		#elif !defined(SRC_KIND_15)
			#   define SRC_KIND_15			fskYUV420FormatKind	/* 15 */
		#elif !defined(SRC_KIND_16)
			#   define SRC_KIND_16			fskYUV420FormatKind	/* 16 */
		#elif !defined(SRC_KIND_17)
			#   define SRC_KIND_17			fskYUV420FormatKind	/* 17 */
		#endif
		,SRC_INDEX_YUV420
	#endif /* SRC_YUV420 */

	/* YUV 4:2:2 planar */
	#if SRC_YUV422
		#if   !defined(SRC_KIND_0)
			#   define SRC_KIND_0			fskYUV422FormatKind	/* 0 */
		#elif !defined(SRC_KIND_1)
			#   define SRC_KIND_1			fskYUV422FormatKind	/* 1 */
		#elif !defined(SRC_KIND_2)
			#   define SRC_KIND_2			fskYUV422FormatKind	/* 2 */
		#elif !defined(SRC_KIND_3)
			#   define SRC_KIND_3			fskYUV422FormatKind	/* 3 */
		#elif !defined(SRC_KIND_4)
			#   define SRC_KIND_4			fskYUV422FormatKind	/* 4 */
		#elif !defined(SRC_KIND_5)
			#   define SRC_KIND_5			fskYUV422FormatKind	/* 5 */
		#elif !defined(SRC_KIND_6)
			#   define SRC_KIND_6			fskYUV422FormatKind	/* 6 */
		#elif !defined(SRC_KIND_7)
			#   define SRC_KIND_7			fskYUV422FormatKind	/* 7 */
		#elif !defined(SRC_KIND_8)
			#   define SRC_KIND_8			fskYUV422FormatKind	/* 8 */
		#elif !defined(SRC_KIND_9)
			#   define SRC_KIND_9			fskYUV422FormatKind	/* 9 */
		#elif !defined(SRC_KIND_10)
			#   define SRC_KIND_10			fskYUV422FormatKind	/* 10 */
		#elif !defined(SRC_KIND_11)
			#   define SRC_KIND_11			fskYUV422FormatKind	/* 11 */
		#elif !defined(SRC_KIND_12)
			#   define SRC_KIND_12			fskYUV422FormatKind	/* 12 */
		#elif !defined(SRC_KIND_13)
			#   define SRC_KIND_13			fskYUV422FormatKind	/* 13 */
		#elif !defined(SRC_KIND_14)
			#   define SRC_KIND_14			fskYUV422FormatKind	/* 14 */
		#elif !defined(SRC_KIND_15)
			#   define SRC_KIND_15			fskYUV422FormatKind	/* 15 */
		#elif !defined(SRC_KIND_16)
			#   define SRC_KIND_16			fskYUV422FormatKind	/* 16 */
		#elif !defined(SRC_KIND_17)
			#   define SRC_KIND_17			fskYUV422FormatKind	/* 17 */
		#elif !defined(SRC_KIND_18)
			#   define SRC_KIND_18			fskYUV422FormatKind	/* 18 */
		#endif
		,SRC_INDEX_YUV422
	#endif /* SRC_YUV422 */


	/* UYVY 4:2:2 chunky */
	#if SRC_UYVY
		#if   !defined(SRC_KIND_0)
			#   define SRC_KIND_0			fskUYVYFormatKind	/* 0 */
		#elif !defined(SRC_KIND_1)
			#   define SRC_KIND_1			fskUYVYFormatKind	/* 1 */
		#elif !defined(SRC_KIND_2)
			#   define SRC_KIND_2			fskUYVYFormatKind	/* 2 */
		#elif !defined(SRC_KIND_3)
			#   define SRC_KIND_3			fskUYVYFormatKind	/* 3 */
		#elif !defined(SRC_KIND_4)
			#   define SRC_KIND_4			fskUYVYFormatKind	/* 4 */
		#elif !defined(SRC_KIND_5)
			#   define SRC_KIND_5			fskUYVYFormatKind	/* 5 */
		#elif !defined(SRC_KIND_6)
			#   define SRC_KIND_6			fskUYVYFormatKind	/* 6 */
		#elif !defined(SRC_KIND_7)
			#   define SRC_KIND_7			fskUYVYFormatKind	/* 7 */
		#elif !defined(SRC_KIND_8)
			#   define SRC_KIND_8			fskUYVYFormatKind	/* 8 */
		#elif !defined(SRC_KIND_9)
			#   define SRC_KIND_9			fskUYVYFormatKind	/* 9 */
		#elif !defined(SRC_KIND_10)
			#   define SRC_KIND_10			fskUYVYFormatKind	/* 10 */
		#elif !defined(SRC_KIND_11)
			#   define SRC_KIND_11			fskUYVYFormatKind	/* 11 */
		#elif !defined(SRC_KIND_12)
			#   define SRC_KIND_12			fskUYVYFormatKind	/* 12 */
		#elif !defined(SRC_KIND_13)
			#   define SRC_KIND_13			fskUYVYFormatKind	/* 13 */
		#elif !defined(SRC_KIND_14)
			#   define SRC_KIND_14			fskUYVYFormatKind	/* 14 */
		#elif !defined(SRC_KIND_15)
			#   define SRC_KIND_15			fskUYVYFormatKind	/* 15 */
		#elif !defined(SRC_KIND_16)
			#   define SRC_KIND_16			fskUYVYFormatKind	/* 16 */
		#elif !defined(SRC_KIND_17)
			#   define SRC_KIND_17			fskUYVYFormatKind	/* 17 */
		#elif !defined(SRC_KIND_18)
			#   define SRC_KIND_18			fskUYVYFormatKind	/* 18 */
		#elif !defined(SRC_KIND_19)
			#   define SRC_KIND_19			fskUYVYFormatKind	/* 19 */
		#endif
		,SRC_INDEX_UYVY
	#endif /* SRC_UYVY */

	/* YUV 4:2:0 interleaved */
	#if SRC_YUV420i
		#if   !defined(SRC_KIND_0)
			#   define SRC_KIND_0			fskYUV420iFormatKind	/* 0 */
		#elif !defined(SRC_KIND_1)
			#   define SRC_KIND_1			fskYUV420iFormatKind	/* 1 */
		#elif !defined(SRC_KIND_2)
			#   define SRC_KIND_2			fskYUV420iFormatKind	/* 2 */
		#elif !defined(SRC_KIND_3)
			#   define SRC_KIND_3			fskYUV420iFormatKind	/* 3 */
		#elif !defined(SRC_KIND_4)
			#   define SRC_KIND_4			fskYUV420iFormatKind	/* 4 */
		#elif !defined(SRC_KIND_5)
			#   define SRC_KIND_5			fskYUV420iFormatKind	/* 5 */
		#elif !defined(SRC_KIND_6)
			#   define SRC_KIND_6			fskYUV420iFormatKind	/* 6 */
		#elif !defined(SRC_KIND_7)
			#   define SRC_KIND_7			fskYUV420iFormatKind	/* 7 */
		#elif !defined(SRC_KIND_8)
			#   define SRC_KIND_8			fskYUV420iFormatKind	/* 8 */
		#elif !defined(SRC_KIND_9)
			#   define SRC_KIND_9			fskYUV420iFormatKind	/* 9 */
		#elif !defined(SRC_KIND_10)
			#   define SRC_KIND_10			fskYUV420iFormatKind	/* 10 */
		#elif !defined(SRC_KIND_11)
			#   define SRC_KIND_11			fskYUV420iFormatKind	/* 11 */
		#elif !defined(SRC_KIND_12)
			#   define SRC_KIND_12			fskYUV420iFormatKind	/* 12 */
		#elif !defined(SRC_KIND_13)
			#   define SRC_KIND_13			fskYUV420iFormatKind	/* 13 */
		#elif !defined(SRC_KIND_14)
			#   define SRC_KIND_14			fskYUV420iFormatKind	/* 14 */
		#elif !defined(SRC_KIND_15)
			#   define SRC_KIND_15			fskYUV420iFormatKind	/* 15 */
		#elif !defined(SRC_KIND_16)
			#   define SRC_KIND_16			fskYUV420iFormatKind	/* 16 */
		#elif !defined(SRC_KIND_17)
			#   define SRC_KIND_17			fskYUV420iFormatKind	/* 17 */
		#endif
		,SRC_INDEX_YUV420i
	#endif /* SRC_YUV420i */

	/* 32A 16RGB565LE */
	#if SRC_32A16RGB565LE
		#if   !defined(SRC_KIND_0)
			#   define SRC_KIND_0			fsk32A16RGB565LEFormatKind	/* 0 */
		#elif !defined(SRC_KIND_1)
			#   define SRC_KIND_1			fsk32A16RGB565LEFormatKind	/* 1 */
		#elif !defined(SRC_KIND_2)
			#   define SRC_KIND_2			fsk32A16RGB565LEFormatKind	/* 2 */
		#elif !defined(SRC_KIND_3)
			#   define SRC_KIND_3			fsk32A16RGB565LEFormatKind	/* 3 */
		#elif !defined(SRC_KIND_4)
			#   define SRC_KIND_4			fsk32A16RGB565LEFormatKind	/* 4 */
		#elif !defined(SRC_KIND_5)
			#   define SRC_KIND_5			fsk32A16RGB565LEFormatKind	/* 5 */
		#elif !defined(SRC_KIND_6)
			#   define SRC_KIND_6			fsk32A16RGB565LEFormatKind	/* 6 */
		#elif !defined(SRC_KIND_7)
			#   define SRC_KIND_7			fsk32A16RGB565LEFormatKind	/* 7 */
		#elif !defined(SRC_KIND_8)
			#   define SRC_KIND_8			fsk32A16RGB565LEFormatKind	/* 8 */
		#elif !defined(SRC_KIND_9)
			#   define SRC_KIND_9			fsk32A16RGB565LEFormatKind	/* 9 */
		#elif !defined(SRC_KIND_10)
			#   define SRC_KIND_10			fsk32A16RGB565LEFormatKind	/* 10 */
		#elif !defined(SRC_KIND_11)
			#   define SRC_KIND_11			fsk32A16RGB565LEFormatKind	/* 11 */
		#elif !defined(SRC_KIND_12)
			#   define SRC_KIND_12			fsk32A16RGB565LEFormatKind	/* 12 */
		#elif !defined(SRC_KIND_13)
			#   define SRC_KIND_13			fsk32A16RGB565LEFormatKind	/* 13 */
		#elif !defined(SRC_KIND_14)
			#   define SRC_KIND_14			fsk32A16RGB565LEFormatKind	/* 14 */
		#elif !defined(SRC_KIND_15)
			#   define SRC_KIND_15			fsk32A16RGB565LEFormatKind	/* 15 */
		#elif !defined(SRC_KIND_16)
			#   define SRC_KIND_16			fsk32A16RGB565LEFormatKind	/* 16 */
		#elif !defined(SRC_KIND_17)
			#   define SRC_KIND_17			fsk32A16RGB565LEFormatKind	/* 17 */
		#elif !defined(SRC_KIND_18)
			#   define SRC_KIND_18			fsk32A16RGB565LEFormatKind	/* 18 */
		#endif
		,SRC_INDEX_32A16RGB565LE
	#endif /* SRC_32A16RGB565LE */

	/* YUV420spvu */
	#if SRC_YUV420spvu
		#if   !defined(SRC_KIND_0)
			#   define SRC_KIND_0			fskYUV420spvuFormatKind	/* 0 */
		#elif !defined(SRC_KIND_1)
			#   define SRC_KIND_1			fskYUV420spvuFormatKind	/* 1 */
		#elif !defined(SRC_KIND_2)
			#   define SRC_KIND_2			fskYUV420spvuFormatKind	/* 2 */
		#elif !defined(SRC_KIND_3)
			#   define SRC_KIND_3			fskYUV420spvuFormatKind	/* 3 */
		#elif !defined(SRC_KIND_4)
			#   define SRC_KIND_4			fskYUV420spvuFormatKind	/* 4 */
		#elif !defined(SRC_KIND_5)
			#   define SRC_KIND_5			fskYUV420spvuFormatKind	/* 5 */
		#elif !defined(SRC_KIND_6)
			#   define SRC_KIND_6			fskYUV420spvuFormatKind	/* 6 */
		#elif !defined(SRC_KIND_7)
			#   define SRC_KIND_7			fskYUV420spvuFormatKind	/* 7 */
		#elif !defined(SRC_KIND_8)
			#   define SRC_KIND_8			fskYUV420spvuFormatKind	/* 8 */
		#elif !defined(SRC_KIND_9)
			#   define SRC_KIND_9			fskYUV420spvuFormatKind	/* 9 */
		#elif !defined(SRC_KIND_10)
			#   define SRC_KIND_10			fskYUV420spvuFormatKind	/* 10 */
		#elif !defined(SRC_KIND_11)
			#   define SRC_KIND_11			fskYUV420spvuFormatKind	/* 11 */
		#elif !defined(SRC_KIND_12)
			#   define SRC_KIND_12			fskYUV420spvuFormatKind	/* 12 */
		#elif !defined(SRC_KIND_13)
			#   define SRC_KIND_13			fskYUV420spvuFormatKind	/* 13 */
		#elif !defined(SRC_KIND_14)
			#   define SRC_KIND_14			fskYUV420spvuFormatKind	/* 14 */
		#elif !defined(SRC_KIND_15)
			#   define SRC_KIND_15			fskYUV420spvuFormatKind	/* 15 */
		#elif !defined(SRC_KIND_16)
			#   define SRC_KIND_16			fskYUV420spvuFormatKind	/* 16 */
		#elif !defined(SRC_KIND_17)
			#   define SRC_KIND_17			fskYUV420spvuFormatKind	/* 17 */
		#elif !defined(SRC_KIND_18)
			#   define SRC_KIND_18			fskYUV420spvuFormatKind	/* 18 */
		#elif !defined(SRC_KIND_19)
			#   define SRC_KIND_19			fskYUV420spvuFormatKind	/* 19 */
		#endif
		,SRC_INDEX_32A16RGB565LE
	#endif /* SRC_32A16RGB565LE */
	/****************************************
	 *	^  ADD NEW PIXEL FORMATS HERE  ^	*
	 ****************************************/

	/* The disabled pixel formats will get an index of -1 here */
	#if !SRC_32ARGB
		,SRC_INDEX_32ARGB			= -1						/* 32 ARGB */
	#endif /* SRC_32ARGB */
	#if !SRC_32BGRA
		,SRC_INDEX_32BGRA			= -1						/* 32 BGRA */
	#endif /* SRC_32BGRA */
	#if !SRC_32RGBA
		,SRC_INDEX_32RGBA			= -1						/* 32 RGBA */
	#endif /* SRC_32RGBA */
	#if !SRC_32ABGR
		,SRC_INDEX_32ABGR			= -1						/* 32 ABGR */
	#endif /* SRC_32ABGR */
	#if !SRC_24BGR
		,SRC_INDEX_24BGR			= -1						/* 24 BGR */
	#endif /* SRC_24BGR */
	#if !SRC_24RGB
		,SRC_INDEX_24RGB			= -1						/* 24 RGB */
	#endif /* SRC_24RGB */
	#if !SRC_16RGB565BE
		,SRC_INDEX_16RGB565BE		= -1						/* 16 RGB 565 BE */
	#endif /* SRC_16RGB565BE */
	#if !SRC_16RGB565LE
		,SRC_INDEX_16RGB565LE		= -1						/* 16 RGB 565 LE */
	#endif /* SRC_16RGB565LE */
	#if !SRC_16BGR565LE
		,SRC_INDEX_16BGR565LE		= -1						/* 16 BGR 565 LE */
	#endif /* SRC_16BGR565LE */
	#if !SRC_16RGB5515LE
		,SRC_INDEX_16RGB5515LE		= -1						/* 16 RGB 5515 LE */
	#endif /* SRC_16RGB5515LE */
	#if !SRC_16RGB5515BE
		,SRC_INDEX_16RGB5515BE		= -1						/* 16 RGB 5515 BE */
	#endif /* SRC_16RGB5515BE */
	#if !SRC_16RGBA4444LE
		,SRC_INDEX_16RGBA4444LE		= -1						/* 16 RGBA 4444 LE */
	#endif /* SRC_16RGBA4444LE */
	#if !SRC_16RGBA4444BE
		,SRC_INDEX_16RGBA4444BE		= -1						/* 16 RGBA 4444 BE */
	#endif /* SRC_16RGBA4444BE */
	#if !SRC_16AG
		,SRC_INDEX_16AG				= -1						/* 16 AG */
	#endif /* SRC_16AG */
	#if !SRC_8A
		,SRC_INDEX_8A				= -1						/* 8 A */
	#endif /* SRC_8A */
	#if !SRC_8G
		,SRC_INDEX_8G				= -1						/* 8 G */
	#endif /* SRC_8G */
	#if !SRC_YUV420spuv
		,SRC_INDEX_YUV420spuv		= -1						/* YUV 4:2:0 semi-planar UV */
	#endif /* SRC_YUV420spuv */
	#if !SRC_YUV420
		,SRC_INDEX_YUV420			= -1						/* YUV 4:2:0 planar */
	#endif /* SRC_YUV420 */
	#if !SRC_YUV422
		,SRC_INDEX_YUV422			= -1						/* YUV 4:2:2 planar */
	#endif /* SRC_YUV422 */
	#if !SRC_UYVY
		,SRC_INDEX_UYVY				= -1						/* UYVY 4:2:2 chunky */
	#endif /* SRC_UYVY */
	#if !SRC_YUV420i
		,SRC_INDEX_YUV420i			= -1						/* YUV 4:2:0 interleaved */
	#endif /* SRC_YUV420i */
	#if !SRC_32A16RGB565LE
		,SRC_INDEX_32A16RGB565LE	= -1						/* 32A 16RGB565LE */
	#endif /* SRC_YUV420i */
	#if !SRC_YUV420spvu
		,SRC_INDEX_YUV420spvu		= -1						/* YUV 4:2:0 semi-planar VU */
	#endif /* SRC_YUV420spvu */
	/****************************************
	 *	^  ADD NEW PIXEL FORMATS HERE  ^	*
	 ****************************************/

};


/********************************************************************************
 *							Destination Kind Indices							*
 ********************************************************************************/
enum {
	DST_INDEX_INTRODUCTION											= -1						/* kFskBitmapFormatUnknown */


	/* The enabled pixel formats will get a sequential index here */


	/* 32 ARGB */
	#if DST_32ARGB
		#if   !defined(DST_KIND_0)
			#define DST_KIND_0				fsk32ARGBFormatKind	/* 0 */
		#else
			#error Unexpected predefinition of DST_KIND_0
		#endif
		,DST_INDEX_32ARGB
	#endif /* DST_32ARGB */


	/* 32 BGRA */
	#if DST_32BGRA
		#if   !defined(DST_KIND_0)
			#   define DST_KIND_0			fsk32BGRAFormatKind	/* 0 */
		#elif !defined(DST_KIND_1)
			#   define DST_KIND_1			fsk32BGRAFormatKind	/* 1 */
		#endif
		,DST_INDEX_32BGRA
	#endif /* DST_32BGRA */


	/* 32 RGBA */
	#if DST_32RGBA
		#if   !defined(DST_KIND_0)
			#   define DST_KIND_0			fsk32RGBAFormatKind	/* 0 */
		#elif !defined(DST_KIND_1)
			#   define DST_KIND_1			fsk32RGBAFormatKind	/* 1 */
		#elif !defined(DST_KIND_2)
			#   define DST_KIND_2			fsk32RGBAFormatKind	/* 2 */
		#endif
		,DST_INDEX_32RGBA
	#endif /* DST_32RGBA */


	/* 32 ABGR */
	#if DST_32ABGR
		#if   !defined(DST_KIND_0)
			#   define DST_KIND_0			fsk32ABGRFormatKind	/* 0 */
		#elif !defined(DST_KIND_1)
			#   define DST_KIND_1			fsk32ABGRFormatKind	/* 1 */
		#elif !defined(DST_KIND_2)
			#   define DST_KIND_2			fsk32ABGRFormatKind	/* 2 */
		#elif !defined(DST_KIND_3)
			#   define DST_KIND_3			fsk32ABGRFormatKind	/* 3 */
		#endif
		,DST_INDEX_32ABGR
	#endif /* DST_32ABGR */


	/* 24 BGR */
	#if DST_24BGR
		#if   !defined(DST_KIND_0)
			#   define DST_KIND_0			fsk24BGRFormatKind	/* 0 */
		#elif !defined(DST_KIND_1)
			#   define DST_KIND_1			fsk24BGRFormatKind	/* 1 */
		#elif !defined(DST_KIND_2)
			#   define DST_KIND_2			fsk24BGRFormatKind	/* 2 */
		#elif !defined(DST_KIND_3)
			#   define DST_KIND_3			fsk24BGRFormatKind	/* 3 */
		#elif !defined(DST_KIND_4)
			#   define DST_KIND_4			fsk24BGRFormatKind	/* 4 */
		#endif
		,DST_INDEX_24BGR
	#endif /* DST_24BGR */


	/* 24 RGB */
	#if DST_24RGB
		#if   !defined(DST_KIND_0)
			#   define DST_KIND_0			fsk24RGBFormatKind	/* 0 */
		#elif !defined(DST_KIND_1)
			#   define DST_KIND_1			fsk24RGBFormatKind	/* 1 */
		#elif !defined(DST_KIND_2)
			#   define DST_KIND_2			fsk24RGBFormatKind	/* 2 */
		#elif !defined(DST_KIND_3)
			#   define DST_KIND_3			fsk24RGBFormatKind	/* 3 */
		#elif !defined(DST_KIND_4)
			#   define DST_KIND_4			fsk24RGBFormatKind	/* 4 */
		#elif !defined(DST_KIND_5)
			#   define DST_KIND_5			fsk24RGBFormatKind	/* 5 */
		#endif
		,DST_INDEX_24RGB
	#endif /* DST_24RGB */


	/* 16 RGB 565 BE */
	#if DST_16RGB565BE
		#if   !defined(DST_KIND_0)
			#   define DST_KIND_0			fsk16RGB565BEFormatKind	/* 0 */
		#elif !defined(DST_KIND_1)
			#   define DST_KIND_1			fsk16RGB565BEFormatKind	/* 1 */
		#elif !defined(DST_KIND_2)
			#   define DST_KIND_2			fsk16RGB565BEFormatKind	/* 2 */
		#elif !defined(DST_KIND_3)
			#   define DST_KIND_3			fsk16RGB565BEFormatKind	/* 3 */
		#elif !defined(DST_KIND_4)
			#   define DST_KIND_4			fsk16RGB565BEFormatKind	/* 4 */
		#elif !defined(DST_KIND_5)
			#   define DST_KIND_5			fsk16RGB565BEFormatKind	/* 5 */
		#elif !defined(DST_KIND_6)
			#   define DST_KIND_6			fsk16RGB565BEFormatKind	/* 6 */
		#endif
		,DST_INDEX_16RGB565BE
	#endif /* DST_16RGB565BE */


	/* 16 RGB 565 LE */
	#if DST_16RGB565LE
		#if   !defined(DST_KIND_0)
			#   define DST_KIND_0			fsk16RGB565LEFormatKind	/* 0 */
		#elif !defined(DST_KIND_1)
			#   define DST_KIND_1			fsk16RGB565LEFormatKind	/* 1 */
		#elif !defined(DST_KIND_2)
			#   define DST_KIND_2			fsk16RGB565LEFormatKind	/* 2 */
		#elif !defined(DST_KIND_3)
			#   define DST_KIND_3			fsk16RGB565LEFormatKind	/* 3 */
		#elif !defined(DST_KIND_4)
			#   define DST_KIND_4			fsk16RGB565LEFormatKind	/* 4 */
		#elif !defined(DST_KIND_5)
			#   define DST_KIND_5			fsk16RGB565LEFormatKind	/* 5 */
		#elif !defined(DST_KIND_6)
			#   define DST_KIND_6			fsk16RGB565LEFormatKind	/* 6 */
		#elif !defined(DST_KIND_7)
			#   define DST_KIND_7			fsk16RGB565LEFormatKind	/* 7 */
		#endif
		,DST_INDEX_16RGB565LE
	#endif /* DST_16RGB565LE */


	/* 16 BGR 565 LE */
	#if DST_16BGR565LE
		#if   !defined(DST_KIND_0)
			#   define DST_KIND_0			fsk16BGR565LEFormatKind	/* 0 */
		#elif !defined(DST_KIND_1)
			#   define DST_KIND_1			fsk16BGR565LEFormatKind	/* 1 */
		#elif !defined(DST_KIND_2)
			#   define DST_KIND_2			fsk16BGR565LEFormatKind	/* 2 */
		#elif !defined(DST_KIND_3)
			#   define DST_KIND_3			fsk16BGR565LEFormatKind	/* 3 */
		#elif !defined(DST_KIND_4)
			#   define DST_KIND_4			fsk16BGR565LEFormatKind	/* 4 */
		#elif !defined(DST_KIND_5)
			#   define DST_KIND_5			fsk16BGR565LEFormatKind	/* 5 */
		#elif !defined(DST_KIND_6)
			#   define DST_KIND_6			fsk16BGR565LEFormatKind	/* 6 */
		#elif !defined(DST_KIND_7)
			#   define DST_KIND_7			fsk16BGR565LEFormatKind	/* 7 */
		#elif !defined(DST_KIND_8)
			#   define DST_KIND_8			fsk16BGR565LEFormatKind	/* 8 */
		#endif
		,DST_INDEX_16BGR565LE
	#endif /* DST_16BGR565LE */


	/* 16 RGB 5515 LE */
	#if DST_16RGB5515LE
		#if   !defined(DST_KIND_0)
			#   define DST_KIND_0			fsk16RGB5515LEFormatKind	/* 0 */
		#elif !defined(DST_KIND_1)
			#   define DST_KIND_1			fsk16RGB5515LEFormatKind	/* 1 */
		#elif !defined(DST_KIND_2)
			#   define DST_KIND_2			fsk16RGB5515LEFormatKind	/* 2 */
		#elif !defined(DST_KIND_3)
			#   define DST_KIND_3			fsk16RGB5515LEFormatKind	/* 3 */
		#elif !defined(DST_KIND_4)
			#   define DST_KIND_4			fsk16RGB5515LEFormatKind	/* 4 */
		#elif !defined(DST_KIND_5)
			#   define DST_KIND_5			fsk16RGB5515LEFormatKind	/* 5 */
		#elif !defined(DST_KIND_6)
			#   define DST_KIND_6			fsk16RGB5515LEFormatKind	/* 6 */
		#elif !defined(DST_KIND_7)
			#   define DST_KIND_7			fsk16RGB5515LEFormatKind	/* 7 */
		#elif !defined(DST_KIND_8)
			#   define DST_KIND_8			fsk16RGB5515LEFormatKind	/* 8 */
		#elif !defined(DST_KIND_9)
			#   define DST_KIND_9			fsk16RGB5515LEFormatKind	/* 9 */
		#endif
		,DST_INDEX_16RGB5515LE
	#endif /* DST_16RGB5515LE */


	/* 16 RGB 5515 BE */
	#if DST_16RGB5515BE
		#if   !defined(DST_KIND_0)
			#   define DST_KIND_0			fsk16RGB5515BEFormatKind	/* 0 */
		#elif !defined(DST_KIND_1)
			#   define DST_KIND_1			fsk16RGB5515BEFormatKind	/* 1 */
		#elif !defined(DST_KIND_2)
			#   define DST_KIND_2			fsk16RGB5515BEFormatKind	/* 2 */
		#elif !defined(DST_KIND_3)
			#   define DST_KIND_3			fsk16RGB5515BEFormatKind	/* 3 */
		#elif !defined(DST_KIND_4)
			#   define DST_KIND_4			fsk16RGB5515BEFormatKind	/* 4 */
		#elif !defined(DST_KIND_5)
			#   define DST_KIND_5			fsk16RGB5515BEFormatKind	/* 5 */
		#elif !defined(DST_KIND_6)
			#   define DST_KIND_6			fsk16RGB5515BEFormatKind	/* 6 */
		#elif !defined(DST_KIND_7)
			#   define DST_KIND_7			fsk16RGB5515BEFormatKind	/* 7 */
		#elif !defined(DST_KIND_8)
			#   define DST_KIND_8			fsk16RGB5515BEFormatKind	/* 8 */
		#elif !defined(DST_KIND_9)
			#   define DST_KIND_9			fsk16RGB5515BEFormatKind	/* 9 */
		#elif !defined(DST_KIND_10)
			#   define DST_KIND_10			fsk16RGB5515BEFormatKind	/* 10 */
		#endif
		,DST_INDEX_16RGB5515BE
	#endif /* DST_16RGB5515BE */


	/* 16 RGBA 4444 LE */
	#if DST_16RGBA4444LE
		#if   !defined(DST_KIND_0)
			#   define DST_KIND_0			fsk16RGBA4444LEFormatKind	/* 0 */
		#elif !defined(DST_KIND_1)
			#   define DST_KIND_1			fsk16RGBA4444LEFormatKind	/* 1 */
		#elif !defined(DST_KIND_2)
			#   define DST_KIND_2			fsk16RGBA4444LEFormatKind	/* 2 */
		#elif !defined(DST_KIND_3)
			#   define DST_KIND_3			fsk16RGBA4444LEFormatKind	/* 3 */
		#elif !defined(DST_KIND_4)
			#   define DST_KIND_4			fsk16RGBA4444LEFormatKind	/* 4 */
		#elif !defined(DST_KIND_5)
			#   define DST_KIND_5			fsk16RGBA4444LEFormatKind	/* 5 */
		#elif !defined(DST_KIND_6)
			#   define DST_KIND_6			fsk16RGBA4444LEFormatKind	/* 6 */
		#elif !defined(DST_KIND_7)
			#   define DST_KIND_7			fsk16RGBA4444LEFormatKind	/* 7 */
		#elif !defined(DST_KIND_8)
			#   define DST_KIND_8			fsk16RGBA4444LEFormatKind	/* 8 */
		#elif !defined(DST_KIND_9)
			#   define DST_KIND_9			fsk16RGBA4444LEFormatKind	/* 9 */
		#elif !defined(DST_KIND_10)
			#   define DST_KIND_10			fsk16RGBA4444LEFormatKind	/* 10 */
		#elif !defined(DST_KIND_11)
			#   define DST_KIND_11			fsk16RGBA4444LEFormatKind	/* 11 */
		#endif
		,DST_INDEX_16RGBA4444LE
	#endif /* DST_16RGBA4444LE */


	/* 16 RGBA 4444 BE */
	#if DST_16RGBA4444BE
		#if   !defined(DST_KIND_0)
			#   define DST_KIND_0			fsk16RGBA4444BEFormatKind	/* 0 */
		#elif !defined(DST_KIND_1)
			#   define DST_KIND_1			fsk16RGBA4444BEFormatKind	/* 1 */
		#elif !defined(DST_KIND_2)
			#   define DST_KIND_2			fsk16RGBA4444BEFormatKind	/* 2 */
		#elif !defined(DST_KIND_3)
			#   define DST_KIND_3			fsk16RGBA4444BEFormatKind	/* 3 */
		#elif !defined(DST_KIND_4)
			#   define DST_KIND_4			fsk16RGBA4444BEFormatKind	/* 4 */
		#elif !defined(DST_KIND_5)
			#   define DST_KIND_5			fsk16RGBA4444BEFormatKind	/* 5 */
		#elif !defined(DST_KIND_6)
			#   define DST_KIND_6			fsk16RGBA4444BEFormatKind	/* 6 */
		#elif !defined(DST_KIND_7)
			#   define DST_KIND_7			fsk16RGBA4444BEFormatKind	/* 7 */
		#elif !defined(DST_KIND_8)
			#   define DST_KIND_8			fsk16RGBA4444BEFormatKind	/* 8 */
		#elif !defined(DST_KIND_9)
			#   define DST_KIND_9			fsk16RGBA4444BEFormatKind	/* 9 */
		#elif !defined(DST_KIND_10)
			#   define DST_KIND_10			fsk16RGBA4444BEFormatKind	/* 10 */
		#elif !defined(DST_KIND_11)
			#   define DST_KIND_11			fsk16RGBA4444BEFormatKind	/* 11 */
		#elif !defined(DST_KIND_12)
			#   define DST_KIND_12			fsk16RGBA4444BEFormatKind	/* 12 */
		#endif
		,DST_INDEX_16RGBA4444BE
	#endif /* DST_16RGBA4444BE */


	/* 16 AG */
	#if DST_16AG
		#if   !defined(DST_KIND_0)
			#   define DST_KIND_0			fsk16AGFormatKind	/* 0 */
		#elif !defined(DST_KIND_1)
			#   define DST_KIND_1			fsk16AGFormatKind	/* 1 */
		#elif !defined(DST_KIND_2)
			#   define DST_KIND_2			fsk16AGFormatKind	/* 2 */
		#elif !defined(DST_KIND_3)
			#   define DST_KIND_3			fsk16AGFormatKind	/* 3 */
		#elif !defined(DST_KIND_4)
			#   define DST_KIND_4			fsk16AGFormatKind	/* 4 */
		#elif !defined(DST_KIND_5)
			#   define DST_KIND_5			fsk16AGFormatKind	/* 5 */
		#elif !defined(DST_KIND_6)
			#   define DST_KIND_6			fsk16AGFormatKind	/* 6 */
		#elif !defined(DST_KIND_7)
			#   define DST_KIND_7			fsk16AGFormatKind	/* 7 */
		#elif !defined(DST_KIND_8)
			#   define DST_KIND_8			fsk16AGFormatKind	/* 8 */
		#elif !defined(DST_KIND_9)
			#   define DST_KIND_9			fsk16AGFormatKind	/* 9 */
		#elif !defined(DST_KIND_10)
			#   define DST_KIND_10			fsk16AGFormatKind	/* 10 */
		#elif !defined(DST_KIND_11)
			#   define DST_KIND_11			fsk16AGFormatKind	/* 11 */
		#elif !defined(DST_KIND_12)
			#   define DST_KIND_12			fsk16AGFormatKind	/* 12 */
		#elif !defined(DST_KIND_13)
			#   define DST_KIND_13			fsk16AGFormatKind	/* 13 */
		#endif
		,DST_INDEX_16AG
	#endif /* DST_16AG */


	/* 8 A */
	#if DST_8A
		#if   !defined(DST_KIND_0)
			#   define DST_KIND_0			fsk8AFormatKind	/* 0 */
		#elif !defined(DST_KIND_1)
			#   define DST_KIND_1			fsk8AFormatKind	/* 1 */
		#elif !defined(DST_KIND_2)
			#   define DST_KIND_2			fsk8AFormatKind	/* 2 */
		#elif !defined(DST_KIND_3)
			#   define DST_KIND_3			fsk8AFormatKind	/* 3 */
		#elif !defined(DST_KIND_4)
			#   define DST_KIND_4			fsk8AFormatKind	/* 4 */
		#elif !defined(DST_KIND_5)
			#   define DST_KIND_5			fsk8AFormatKind	/* 5 */
		#elif !defined(DST_KIND_6)
			#   define DST_KIND_6			fsk8AFormatKind	/* 6 */
		#elif !defined(DST_KIND_7)
			#   define DST_KIND_7			fsk8AFormatKind	/* 7 */
		#elif !defined(DST_KIND_8)
			#   define DST_KIND_8			fsk8AFormatKind	/* 8 */
		#elif !defined(DST_KIND_9)
			#   define DST_KIND_9			fsk8AFormatKind	/* 9 */
		#elif !defined(DST_KIND_10)
			#   define DST_KIND_10			fsk8AFormatKind	/* 10 */
		#elif !defined(DST_KIND_11)
			#   define DST_KIND_11			fsk8AFormatKind	/* 11 */
		#elif !defined(DST_KIND_12)
			#   define DST_KIND_12			fsk8AFormatKind	/* 12 */
		#elif !defined(DST_KIND_13)
			#   define DST_KIND_13			fsk8AFormatKind	/* 13 */
		#elif !defined(DST_KIND_14)
			#   define DST_KIND_14			fsk8AFormatKind	/* 14 */
		#endif
		,DST_INDEX_8A
	#endif /* DST_8A */


	/* 8 G */
	#if DST_8G
		#if   !defined(DST_KIND_0)
			#   define DST_KIND_0			fsk8GFormatKind	/* 0 */
		#elif !defined(DST_KIND_1)
			#   define DST_KIND_1			fsk8GFormatKind	/* 1 */
		#elif !defined(DST_KIND_2)
			#   define DST_KIND_2			fsk8GFormatKind	/* 2 */
		#elif !defined(DST_KIND_3)
			#   define DST_KIND_3			fsk8GFormatKind	/* 3 */
		#elif !defined(DST_KIND_4)
			#   define DST_KIND_4			fsk8GFormatKind	/* 4 */
		#elif !defined(DST_KIND_5)
			#   define DST_KIND_5			fsk8GFormatKind	/* 5 */
		#elif !defined(DST_KIND_6)
			#   define DST_KIND_6			fsk8GFormatKind	/* 6 */
		#elif !defined(DST_KIND_7)
			#   define DST_KIND_7			fsk8GFormatKind	/* 7 */
		#elif !defined(DST_KIND_8)
			#   define DST_KIND_8			fsk8GFormatKind	/* 8 */
		#elif !defined(DST_KIND_9)
			#   define DST_KIND_9			fsk8GFormatKind	/* 9 */
		#elif !defined(DST_KIND_10)
			#   define DST_KIND_10			fsk8GFormatKind	/* 10 */
		#elif !defined(DST_KIND_11)
			#   define DST_KIND_11			fsk8GFormatKind	/* 11 */
		#elif !defined(DST_KIND_12)
			#   define DST_KIND_12			fsk8GFormatKind	/* 12 */
		#elif !defined(DST_KIND_13)
			#   define DST_KIND_13			fsk8GFormatKind	/* 13 */
		#elif !defined(DST_KIND_14)
			#   define DST_KIND_14			fsk8GFormatKind	/* 14 */
		#elif !defined(DST_KIND_15)
			#   define DST_KIND_15			fsk8GFormatKind	/* 15 */
		#endif
		,DST_INDEX_8G
	#endif /* DST_8G */


	/* YUV 4:2:0 semi-planar UV */
	#if DST_YUV420spuv
		#if   !defined(DST_KIND_0)
			#   define DST_KIND_0			fskYUV420spuvFormatKind	/* 0 */
		#elif !defined(DST_KIND_1)
			#   define DST_KIND_1			fskYUV420spuvFormatKind	/* 1 */
		#elif !defined(DST_KIND_2)
			#   define DST_KIND_2			fskYUV420spuvFormatKind	/* 2 */
		#elif !defined(DST_KIND_3)
			#   define DST_KIND_3			fskYUV420spuvFormatKind	/* 3 */
		#elif !defined(DST_KIND_4)
			#   define DST_KIND_4			fskYUV420spuvFormatKind	/* 4 */
		#elif !defined(DST_KIND_5)
			#   define DST_KIND_5			fskYUV420spuvFormatKind	/* 5 */
		#elif !defined(DST_KIND_6)
			#   define DST_KIND_6			fskYUV420spuvFormatKind	/* 6 */
		#elif !defined(DST_KIND_7)
			#   define DST_KIND_7			fskYUV420spuvFormatKind	/* 7 */
		#elif !defined(DST_KIND_8)
			#   define DST_KIND_8			fskYUV420spuvFormatKind	/* 8 */
		#elif !defined(DST_KIND_9)
			#   define DST_KIND_9			fskYUV420spuvFormatKind	/* 9 */
		#elif !defined(DST_KIND_10)
			#   define DST_KIND_10			fskYUV420spuvFormatKind	/* 10 */
		#elif !defined(DST_KIND_11)
			#   define DST_KIND_11			fskYUV420spuvFormatKind	/* 11 */
		#elif !defined(DST_KIND_12)
			#   define DST_KIND_12			fskYUV420spuvFormatKind	/* 12 */
		#elif !defined(DST_KIND_13)
			#   define DST_KIND_13			fskYUV420spuvFormatKind	/* 13 */
		#elif !defined(DST_KIND_14)
			#   define DST_KIND_14			fskYUV420spuvFormatKind	/* 14 */
		#elif !defined(DST_KIND_15)
			#   define DST_KIND_15			fskYUV420spuvFormatKind	/* 15 */
		#elif !defined(DST_KIND_16)
			#   define DST_KIND_16			fskYUV420spuvFormatKind	/* 16 */
		#endif
		,DST_INDEX_YUV420spuv
	#endif /* DST_YUV420spuv */


	/* YUV 4:2:0 planar */
	#if DST_YUV420
		#if   !defined(DST_KIND_0)
			#   define DST_KIND_0			fskYUV420FormatKind	/* 0 */
		#elif !defined(DST_KIND_1)
			#   define DST_KIND_1			fskYUV420FormatKind	/* 1 */
		#elif !defined(DST_KIND_2)
			#   define DST_KIND_2			fskYUV420FormatKind	/* 2 */
		#elif !defined(DST_KIND_3)
			#   define DST_KIND_3			fskYUV420FormatKind	/* 3 */
		#elif !defined(DST_KIND_4)
			#   define DST_KIND_4			fskYUV420FormatKind	/* 4 */
		#elif !defined(DST_KIND_5)
			#   define DST_KIND_5			fskYUV420FormatKind	/* 5 */
		#elif !defined(DST_KIND_6)
			#   define DST_KIND_6			fskYUV420FormatKind	/* 6 */
		#elif !defined(DST_KIND_7)
			#   define DST_KIND_7			fskYUV420FormatKind	/* 7 */
		#elif !defined(DST_KIND_8)
			#   define DST_KIND_8			fskYUV420FormatKind	/* 8 */
		#elif !defined(DST_KIND_9)
			#   define DST_KIND_9			fskYUV420FormatKind	/* 9 */
		#elif !defined(DST_KIND_10)
			#   define DST_KIND_10			fskYUV420FormatKind	/* 10 */
		#elif !defined(DST_KIND_11)
			#   define DST_KIND_11			fskYUV420FormatKind	/* 11 */
		#elif !defined(DST_KIND_12)
			#   define DST_KIND_12			fskYUV420FormatKind	/* 12 */
		#elif !defined(DST_KIND_13)
			#   define DST_KIND_13			fskYUV420FormatKind	/* 13 */
		#elif !defined(DST_KIND_14)
			#   define DST_KIND_14			fskYUV420FormatKind	/* 14 */
		#elif !defined(DST_KIND_15)
			#   define DST_KIND_15			fskYUV420FormatKind	/* 15 */
		#elif !defined(DST_KIND_16)
			#   define DST_KIND_16			fskYUV420FormatKind	/* 16 */
		#elif !defined(DST_KIND_17)
			#   define DST_KIND_17			fskYUV420FormatKind	/* 17 */
		#endif
		,DST_INDEX_YUV420
	#endif /* DST_YUV420 */


	/* YUV 4:2:2 planar */
	#if DST_YUV422
		#if   !defined(DST_KIND_0)
			#   define DST_KIND_0			fskYUV422FormatKind	/* 0 */
		#elif !defined(DST_KIND_1)
			#   define DST_KIND_1			fskYUV422FormatKind	/* 1 */
		#elif !defined(DST_KIND_2)
			#   define DST_KIND_2			fskYUV422FormatKind	/* 2 */
		#elif !defined(DST_KIND_3)
			#   define DST_KIND_3			fskYUV422FormatKind	/* 3 */
		#elif !defined(DST_KIND_4)
			#   define DST_KIND_4			fskYUV422FormatKind	/* 4 */
		#elif !defined(DST_KIND_5)
			#   define DST_KIND_5			fskYUV422FormatKind	/* 5 */
		#elif !defined(DST_KIND_6)
			#   define DST_KIND_6			fskYUV422FormatKind	/* 6 */
		#elif !defined(DST_KIND_7)
			#   define DST_KIND_7			fskYUV422FormatKind	/* 7 */
		#elif !defined(DST_KIND_8)
			#   define DST_KIND_8			fskYUV422FormatKind	/* 8 */
		#elif !defined(DST_KIND_9)
			#   define DST_KIND_9			fskYUV422FormatKind	/* 9 */
		#elif !defined(DST_KIND_10)
			#   define DST_KIND_10			fskYUV422FormatKind	/* 10 */
		#elif !defined(DST_KIND_11)
			#   define DST_KIND_11			fskYUV422FormatKind	/* 11 */
		#elif !defined(DST_KIND_12)
			#   define DST_KIND_12			fskYUV422FormatKind	/* 12 */
		#elif !defined(DST_KIND_13)
			#   define DST_KIND_13			fskYUV422FormatKind	/* 13 */
		#elif !defined(DST_KIND_14)
			#   define DST_KIND_14			fskYUV422FormatKind	/* 14 */
		#elif !defined(DST_KIND_15)
			#   define DST_KIND_15			fskYUV422FormatKind	/* 15 */
		#elif !defined(DST_KIND_16)
			#   define DST_KIND_16			fskYUV422FormatKind	/* 16 */
		#elif !defined(DST_KIND_17)
			#   define DST_KIND_17			fskYUV422FormatKind	/* 17 */
		#elif !defined(DST_KIND_18)
			#   define DST_KIND_18			fskYUV422FormatKind	/* 18 */
		#endif
		,DST_INDEX_YUV422
	#endif /* DST_YUV422 */


	/* UYVY 4:2:2 chunky */
	#if DST_UYVY
		#if   !defined(DST_KIND_0)
			#   define DST_KIND_0			fskUYVYFormatKind	/* 0 */
		#elif !defined(DST_KIND_1)
			#   define DST_KIND_1			fskUYVYFormatKind	/* 1 */
		#elif !defined(DST_KIND_2)
			#   define DST_KIND_2			fskUYVYFormatKind	/* 2 */
		#elif !defined(DST_KIND_3)
			#   define DST_KIND_3			fskUYVYFormatKind	/* 3 */
		#elif !defined(DST_KIND_4)
			#   define DST_KIND_4			fskUYVYFormatKind	/* 4 */
		#elif !defined(DST_KIND_5)
			#   define DST_KIND_5			fskUYVYFormatKind	/* 5 */
		#elif !defined(DST_KIND_6)
			#   define DST_KIND_6			fskUYVYFormatKind	/* 6 */
		#elif !defined(DST_KIND_7)
			#   define DST_KIND_7			fskUYVYFormatKind	/* 7 */
		#elif !defined(DST_KIND_8)
			#   define DST_KIND_8			fskUYVYFormatKind	/* 8 */
		#elif !defined(DST_KIND_9)
			#   define DST_KIND_9			fskUYVYFormatKind	/* 9 */
		#elif !defined(DST_KIND_10)
			#   define DST_KIND_10			fskUYVYFormatKind	/* 10 */
		#elif !defined(DST_KIND_11)
			#   define DST_KIND_11			fskUYVYFormatKind	/* 11 */
		#elif !defined(DST_KIND_12)
			#   define DST_KIND_12			fskUYVYFormatKind	/* 12 */
		#elif !defined(DST_KIND_13)
			#   define DST_KIND_13			fskUYVYFormatKind	/* 13 */
		#elif !defined(DST_KIND_14)
			#   define DST_KIND_14			fskUYVYFormatKind	/* 14 */
		#elif !defined(DST_KIND_15)
			#   define DST_KIND_15			fskUYVYFormatKind	/* 15 */
		#elif !defined(DST_KIND_16)
			#   define DST_KIND_16			fskUYVYFormatKind	/* 16 */
		#elif !defined(DST_KIND_17)
			#   define DST_KIND_17			fskUYVYFormatKind	/* 17 */
		#elif !defined(DST_KIND_18)
			#   define DST_KIND_18			fskUYVYFormatKind	/* 18 */
		#elif !defined(DST_KIND_19)
			#   define DST_KIND_19			fskUYVYFormatKind	/* 19 */
		#endif
		,DST_INDEX_UYVY
	#endif /* DST_UYVY */

	/* 32 A 16 565 LE */
	#if DST_32A16RGB565LE
		#if   !defined(DST_KIND_0)
		#   define DST_KIND_0				fsk32A16RGB565LEFormatKind	/* 0 */
		#elif !defined(DST_KIND_1)
			#   define DST_KIND_1			fsk32A16RGB565LEFormatKind	/* 1 */
		#elif !defined(DST_KIND_2)
			#   define DST_KIND_2			fsk32A16RGB565LEFormatKind	/* 2 */
		#elif !defined(DST_KIND_3)
			#   define DST_KIND_3			fsk32A16RGB565LEFormatKind	/* 3 */
		#elif !defined(DST_KIND_4)
			#   define DST_KIND_4			fsk32A16RGB565LEFormatKind	/* 4 */
		#elif !defined(DST_KIND_5)
			#   define DST_KIND_5			fsk32A16RGB565LEFormatKind	/* 5 */
		#elif !defined(DST_KIND_6)
			#   define DST_KIND_6			fsk32A16RGB565LEFormatKind	/* 6 */
		#elif !defined(DST_KIND_7)
			#   define DST_KIND_7			fsk32A16RGB565LEFormatKind	/* 7 */
		#elif !defined(DST_KIND_8)
			#   define DST_KIND_8			fsk32A16RGB565LEFormatKind	/* 8 */
		#elif !defined(DST_KIND_9)
			#   define DST_KIND_9			fsk32A16RGB565LEFormatKind	/* 9 */
		#elif !defined(DST_KIND_10)
			#   define DST_KIND_10			fsk32A16RGB565LEFormatKind	/* 10 */
		#elif !defined(DST_KIND_11)
			#   define DST_KIND_11			fsk32A16RGB565LEFormatKind	/* 11 */
		#elif !defined(DST_KIND_12)
			#   define DST_KIND_12			fsk32A16RGB565LEFormatKind	/* 12 */
		#elif !defined(DST_KIND_13)
			#   define DST_KIND_13			fsk32A16RGB565LEFormatKind	/* 13 */
		#elif !defined(DST_KIND_14)
			#   define DST_KIND_14			fsk32A16RGB565LEFormatKind	/* 14 */
		#elif !defined(DST_KIND_15)
			#   define DST_KIND_15			fsk32A16RGB565LEFormatKind	/* 15 */
		#elif !defined(DST_KIND_16)
			#   define DST_KIND_16			fsk32A16RGB565LEFormatKind	/* 16 */
		#elif !defined(DST_KIND_17)
			#   define DST_KIND_17			fsk32A16RGB565LEFormatKind	/* 17 */
		#elif !defined(DST_KIND_18)
			#   define DST_KIND_18			fsk32A16RGB565LEFormatKind	/* 18 */
		#elif !defined(DST_KIND_19)
			#   define DST_KIND_19			fsk32A16RGB565LEFormatKind	/* 19 */
		#elif !defined(DST_KIND_20)
			#   define DST_KIND_20			fsk32A16RGB565LEFormatKind	/* 20 */
		#endif
		,DST_INDEX_32A16RGB565LE
	#endif /* DST_32A16RGB565LE */

	/* YUV 4:2:0 semi-planar VU */
	#if DST_YUV420spvu
		#if   !defined(DST_KIND_0)
		#   define DST_KIND_0				fskYUV420spvuFormatKind	/* 0 */
		#elif !defined(DST_KIND_1)
			#   define DST_KIND_1			fskYUV420spvuFormatKind	/* 1 */
		#elif !defined(DST_KIND_2)
			#   define DST_KIND_2			fskYUV420spvuFormatKind	/* 2 */
		#elif !defined(DST_KIND_3)
			#   define DST_KIND_3			fskYUV420spvuFormatKind	/* 3 */
		#elif !defined(DST_KIND_4)
			#   define DST_KIND_4			fskYUV420spvuFormatKind	/* 4 */
		#elif !defined(DST_KIND_5)
			#   define DST_KIND_5			fskYUV420spvuFormatKind	/* 5 */
		#elif !defined(DST_KIND_6)
			#   define DST_KIND_6			fskYUV420spvuFormatKind	/* 6 */
		#elif !defined(DST_KIND_7)
			#   define DST_KIND_7			fskYUV420spvuFormatKind	/* 7 */
		#elif !defined(DST_KIND_8)
			#   define DST_KIND_8			fskYUV420spvuFormatKind	/* 8 */
		#elif !defined(DST_KIND_9)
			#   define DST_KIND_9			fskYUV420spvuFormatKind	/* 9 */
		#elif !defined(DST_KIND_10)
			#   define DST_KIND_10			fskYUV420spvuFormatKind	/* 10 */
		#elif !defined(DST_KIND_11)
			#   define DST_KIND_11			fskYUV420spvuFormatKind	/* 11 */
		#elif !defined(DST_KIND_12)
			#   define DST_KIND_12			fskYUV420spvuFormatKind	/* 12 */
		#elif !defined(DST_KIND_13)
			#   define DST_KIND_13			fskYUV420spvuFormatKind	/* 13 */
		#elif !defined(DST_KIND_14)
			#   define DST_KIND_14			fskYUV420spvuFormatKind	/* 14 */
		#elif !defined(DST_KIND_15)
			#   define DST_KIND_15			fskYUV420spvuFormatKind	/* 15 */
		#elif !defined(DST_KIND_16)
			#   define DST_KIND_16			fskYUV420spvuFormatKind	/* 16 */
		#elif !defined(DST_KIND_17)
			#   define DST_KIND_17			fskYUV420spvuFormatKind	/* 17 */
		#elif !defined(DST_KIND_18)
			#   define DST_KIND_18			fskYUV420spvuFormatKind	/* 18 */
		#elif !defined(DST_KIND_19)
			#   define DST_KIND_19			fskYUV420spvuFormatKind	/* 19 */
		#elif !defined(DST_KIND_20)
			#   define DST_KIND_20			fskYUV420spvuFormatKind	/* 20 */
		#elif !defined(DST_KIND_21)
			#   define DST_KIND_21			fskYUV420spvuFormatKind	/* 21 */
		#endif
		,DST_INDEX_32A16RGB565LE
	#endif /* DST_32A16RGB565LE */
	/****************************************
	 *	^  ADD NEW PIXEL FORMATS HERE  ^	*
	 ****************************************/

	/* The disabled pixel formats will get an index of -1 here */
	#if !DST_32ARGB
		,DST_INDEX_32ARGB			= -1						/* 32 ARGB */
	#endif /* DST_32ARGB */
	#if !DST_32BGRA
		,DST_INDEX_32BGRA			= -1						/* 32 BGRA */
	#endif /* DST_32BGRA */
	#if !DST_32RGBA
		,DST_INDEX_32RGBA			= -1						/* 32 RGBA */
	#endif /* DST_32RGBA */
	#if !DST_32ABGR
		,DST_INDEX_32ABGR			= -1						/* 32 ABGR */
	#endif /* DST_32ABGR */
	#if !DST_24BGR
		,DST_INDEX_24BGR			= -1						/* 24 BGR */
	#endif /* DST_24BGR */
	#if !DST_24RGB
		,DST_INDEX_24RGB			= -1						/* 24 RGB */
	#endif /* DST_24RGB */
	#if !DST_16RGB565BE
		,DST_INDEX_16RGB565BE		= -1						/* 16 RGB 565 BE */
	#endif /* DST_16RGB565BE */
	#if !DST_16RGB565LE
		,DST_INDEX_16RGB565LE		= -1						/* 16 RGB 565 LE */
	#endif /* DST_16RGB565LE */
	#if !DST_16BGR565LE
		,DST_INDEX_16BGR565LE		= -1						/* 16 BGR 565 LE */
	#endif /* DST_16BGR565LE */
	#if !DST_16RGB5515LE
		,DST_INDEX_16RGB5515LE		= -1						/* 16 RGB 5515 LE */
	#endif /* DST_16RGB5515LE */
	#if !DST_16RGB5515BE
		,DST_INDEX_16RGB5515BE		= -1						/* 16 RGB 5515 BE */
	#endif /* DST_16RGB5515BE */
	#if !DST_16RGBA4444LE
		,DST_INDEX_16RGBA4444LE		= -1						/* 16 RGBA 4444 LE */
	#endif /* DST_16RGBA4444LE */
	#if !DST_16RGBA4444BE
		,DST_INDEX_16RGBA4444BE		= -1						/* 16 RGBA 4444 BE */
	#endif /* DST_16RGBA4444BE */
	#if !DST_16AG
		,DST_INDEX_16AG				= -1						/* 16 AG */
	#endif /* DST_16AG */
	#if !DST_8A
		,DST_INDEX_8A				= -1						/* 8 A */
	#endif /* DST_8A */
	#if !DST_8G
		,DST_INDEX_8G				= -1						/* 8 G */
	#endif /* DST_8G */
	#if !DST_YUV420spuv
		,DST_INDEX_YUV420spuv		= -1						/* YUB 4:2:0 sp UV */
	#endif /* DST_YUV420spuv */
	#if !DST_YUV420
		,DST_INDEX_YUV420			= -1						/* YUV 4:2:0 planar */
	#endif /* DST_YUV420 */
	#if !DST_YUV422
		,DST_INDEX_YUV422			= -1						/* YUV 4:2:2 planar */
	#endif /* DST_YUV422 */
	#if !DST_UYVY
		,DST_INDEX_UYVY				= -1						/* UYVY 4:2:2 planar */
	#endif /* DST_UYVY */
	#if !DST_YUV420i
		,DST_INDEX_YUV420i			= -1						/* YUV 4:2:0 interleaved */
	#endif /* DST_YUV420i */
	#if !DST_32A16RGB565LE
		,DST_INDEX_32A16RGB565LE	= -1						/* 32A 16RGB565LE */
	#endif /* DST_32A16RGB565LE */
	#if !DST_YUV420spvu
		,DST_INDEX_YUV420spvu		= -1						/* YUB 4:2:0 sp VU */
	#endif /* DST_YUV420spvu */
	/****************************************
	 *	^  ADD NEW PIXEL FORMATS HERE  ^	*
	 ****************************************/

};



/********************************************************************************
 *						Unity Destination Kind Indices							*
 ********************************************************************************/

#if (NUM_DST_FORMATS > 0)
	#if FskName2(DST_UNITY_,FskName3(fsk,DST_KIND_0,KindFormat))
		#define DST_UNITY_INDEX_0	0
		#define DST_UNITY_KIND_0	DST_KIND_0
	#else
		#define DST_UNITY_INDEX_0	(-1)
	#endif
#endif

#if (NUM_DST_FORMATS > 1)
	#if FskName2(DST_UNITY_,FskName3(fsk,DST_KIND_1,KindFormat))
		#if   !defined(DST_UNITY_KIND_0)
			#define DST_UNITY_INDEX_1	0
			#define DST_UNITY_KIND_0	DST_KIND_1
		#elif !defined(DST_UNITY_KIND_1)
			#define DST_UNITY_INDEX_1	1
			#define DST_UNITY_KIND_1	DST_KIND_1
		#endif
	#else
		#define DST_UNITY_INDEX_1	(-1)
	#endif
#endif

#if (NUM_DST_FORMATS > 2)
	#if FskName2(DST_UNITY_,FskName3(fsk,DST_KIND_2,KindFormat))
		#if   !defined(DST_UNITY_KIND_0)
			#define DST_UNITY_INDEX_2	0
			#define DST_UNITY_KIND_0	DST_KIND_2
		#elif !defined(DST_UNITY_KIND_1)
			#define DST_UNITY_INDEX_2	1
			#define DST_UNITY_KIND_1	DST_KIND_2
		#elif !defined(DST_UNITY_KIND_2)
			#define DST_UNITY_INDEX_2	2
			#define DST_UNITY_KIND_2	DST_KIND_2
		#endif
	#else
		#define DST_UNITY_INDEX_2	(-1)
	#endif
#endif

#if (NUM_DST_FORMATS > 3)
	#if FskName2(DST_UNITY_,FskName3(fsk,DST_KIND_3,KindFormat))
		#if   !defined(DST_UNITY_KIND_0)
			#define DST_UNITY_INDEX_3	0
			#define DST_UNITY_KIND_0	DST_KIND_3
		#elif !defined(DST_UNITY_KIND_1)
			#define DST_UNITY_INDEX_3	1
			#define DST_UNITY_KIND_1	DST_KIND_3
		#elif !defined(DST_UNITY_KIND_2)
			#define DST_UNITY_INDEX_3	2
			#define DST_UNITY_KIND_2	DST_KIND_3
		#elif !defined(DST_UNITY_KIND_3)
			#define DST_UNITY_INDEX_3	3
			#define DST_UNITY_KIND_3	DST_KIND_3
		#endif
	#else
		#define DST_UNITY_INDEX_3	(-1)
	#endif
#endif

#if (NUM_DST_FORMATS > 4)
	#if FskName2(DST_UNITY_,FskName3(fsk,DST_KIND_4,KindFormat))
		#if   !defined(DST_UNITY_KIND_0)
			#define DST_UNITY_INDEX_4	0
			#define DST_UNITY_KIND_0	DST_KIND_4
		#elif !defined(DST_UNITY_KIND_1)
			#define DST_UNITY_INDEX_4	1
			#define DST_UNITY_KIND_1	DST_KIND_4
		#elif !defined(DST_UNITY_KIND_2)
			#define DST_UNITY_INDEX_4	2
			#define DST_UNITY_KIND_2	DST_KIND_4
		#elif !defined(DST_UNITY_KIND_3)
			#define DST_UNITY_INDEX_4	3
			#define DST_UNITY_KIND_3	DST_KIND_4
		#elif !defined(DST_UNITY_KIND_4)
			#define DST_UNITY_INDEX_4	4
			#define DST_UNITY_KIND_4	DST_KIND_4
		#endif
	#else
		#define DST_UNITY_INDEX_4	(-1)
	#endif
#endif

#if (NUM_DST_FORMATS > 5)
	#if FskName2(DST_UNITY_,FskName3(fsk,DST_KIND_5,KindFormat))
		#if   !defined(DST_UNITY_KIND_0)
			#define DST_UNITY_INDEX_5	0
			#define DST_UNITY_KIND_0	DST_KIND_5
		#elif !defined(DST_UNITY_KIND_1)
			#define DST_UNITY_INDEX_5	1
			#define DST_UNITY_KIND_1	DST_KIND_5
		#elif !defined(DST_UNITY_KIND_2)
			#define DST_UNITY_INDEX_5	2
			#define DST_UNITY_KIND_2	DST_KIND_5
		#elif !defined(DST_UNITY_KIND_3)
			#define DST_UNITY_INDEX_5	3
			#define DST_UNITY_KIND_3	DST_KIND_5
		#elif !defined(DST_UNITY_KIND_4)
			#define DST_UNITY_INDEX_5	4
			#define DST_UNITY_KIND_4	DST_KIND_5
		#elif !defined(DST_UNITY_KIND_5)
			#define DST_UNITY_INDEX_5	5
			#define DST_UNITY_KIND_5	DST_KIND_5
		#endif
	#else
		#define DST_UNITY_INDEX_5	(-1)
	#endif
#endif

#if (NUM_DST_FORMATS > 6)
	#if FskName2(DST_UNITY_,FskName3(fsk,DST_KIND_6,KindFormat))
		#if   !defined(DST_UNITY_KIND_0)
			#define DST_UNITY_INDEX_6	0
			#define DST_UNITY_KIND_0	DST_KIND_6
		#elif !defined(DST_UNITY_KIND_1)
			#define DST_UNITY_INDEX_6	1
			#define DST_UNITY_KIND_1	DST_KIND_6
		#elif !defined(DST_UNITY_KIND_2)
			#define DST_UNITY_INDEX_6	2
			#define DST_UNITY_KIND_2	DST_KIND_6
		#elif !defined(DST_UNITY_KIND_3)
			#define DST_UNITY_INDEX_6	3
			#define DST_UNITY_KIND_3	DST_KIND_6
		#elif !defined(DST_UNITY_KIND_4)
			#define DST_UNITY_INDEX_6	4
			#define DST_UNITY_KIND_4	DST_KIND_6
		#elif !defined(DST_UNITY_KIND_5)
			#define DST_UNITY_INDEX_6	5
			#define DST_UNITY_KIND_5	DST_KIND_6
		#elif !defined(DST_UNITY_KIND_6)
			#define DST_UNITY_INDEX_6	6
			#define DST_UNITY_KIND_6	DST_KIND_6
		#endif
	#else
		#define DST_UNITY_INDEX_6	(-1)
	#endif
#endif

#if (NUM_DST_FORMATS > 7)
	#if FskName2(DST_UNITY_,FskName3(fsk,DST_KIND_7,KindFormat))
		#if   !defined(DST_UNITY_KIND_0)
			#define DST_UNITY_INDEX_7	0
			#define DST_UNITY_KIND_0	DST_KIND_7
		#elif !defined(DST_UNITY_KIND_1)
			#define DST_UNITY_INDEX_7	1
			#define DST_UNITY_KIND_1	DST_KIND_7
		#elif !defined(DST_UNITY_KIND_2)
			#define DST_UNITY_INDEX_7	2
			#define DST_UNITY_KIND_2	DST_KIND_7
		#elif !defined(DST_UNITY_KIND_3)
			#define DST_UNITY_INDEX_7	3
			#define DST_UNITY_KIND_3	DST_KIND_7
		#elif !defined(DST_UNITY_KIND_4)
			#define DST_UNITY_INDEX_7	4
			#define DST_UNITY_KIND_4	DST_KIND_7
		#elif !defined(DST_UNITY_KIND_5)
			#define DST_UNITY_INDEX_7	5
			#define DST_UNITY_KIND_5	DST_KIND_7
		#elif !defined(DST_UNITY_KIND_6)
			#define DST_UNITY_INDEX_7	6
			#define DST_UNITY_KIND_6	DST_KIND_7
		#elif !defined(DST_UNITY_KIND_7)
			#define DST_UNITY_INDEX_7	7
			#define DST_UNITY_KIND_7	DST_KIND_7
		#endif
	#else
		#define DST_UNITY_INDEX_7	(-1)
	#endif
#endif

#if (NUM_DST_FORMATS > 8)
	#if FskName2(DST_UNITY_,FskName3(fsk,DST_KIND_8,KindFormat))
		#if   !defined(DST_UNITY_KIND_0)
			#define DST_UNITY_INDEX_8	0
			#define DST_UNITY_KIND_0	DST_KIND_8
		#elif !defined(DST_UNITY_KIND_1)
			#define DST_UNITY_INDEX_8	1
			#define DST_UNITY_KIND_1	DST_KIND_8
		#elif !defined(DST_UNITY_KIND_2)
			#define DST_UNITY_INDEX_8	2
			#define DST_UNITY_KIND_2	DST_KIND_8
		#elif !defined(DST_UNITY_KIND_3)
			#define DST_UNITY_INDEX_8	3
			#define DST_UNITY_KIND_3	DST_KIND_8
		#elif !defined(DST_UNITY_KIND_4)
			#define DST_UNITY_INDEX_8	4
			#define DST_UNITY_KIND_4	DST_KIND_8
		#elif !defined(DST_UNITY_KIND_5)
			#define DST_UNITY_INDEX_8	5
			#define DST_UNITY_KIND_5	DST_KIND_8
		#elif !defined(DST_UNITY_KIND_6)
			#define DST_UNITY_INDEX_8	6
			#define DST_UNITY_KIND_6	DST_KIND_8
		#elif !defined(DST_UNITY_KIND_7)
			#define DST_UNITY_INDEX_8	7
			#define DST_UNITY_KIND_7	DST_KIND_8
		#elif !defined(DST_UNITY_KIND_7)
			#define DST_UNITY_INDEX_8	8
			#define DST_UNITY_KIND_8	DST_KIND_8
		#endif
	#else
		#define DST_UNITY_INDEX_8	(-1)
	#endif
#endif

#if (NUM_DST_FORMATS > 9)
	#if FskName2(DST_UNITY_,FskName3(fsk,DST_KIND_9,KindFormat))
		#if   !defined(DST_UNITY_KIND_0)
			#define DST_UNITY_INDEX_9	0
			#define DST_UNITY_KIND_0	DST_KIND_9
		#elif !defined(DST_UNITY_KIND_1)
			#define DST_UNITY_INDEX_9	1
			#define DST_UNITY_KIND_1	DST_KIND_9
		#elif !defined(DST_UNITY_KIND_2)
			#define DST_UNITY_INDEX_9	2
			#define DST_UNITY_KIND_2	DST_KIND_9
		#elif !defined(DST_UNITY_KIND_3)
			#define DST_UNITY_INDEX_9	3
			#define DST_UNITY_KIND_3	DST_KIND_9
		#elif !defined(DST_UNITY_KIND_4)
			#define DST_UNITY_INDEX_9	4
			#define DST_UNITY_KIND_4	DST_KIND_9
		#elif !defined(DST_UNITY_KIND_5)
			#define DST_UNITY_INDEX_9	5
			#define DST_UNITY_KIND_5	DST_KIND_9
		#elif !defined(DST_UNITY_KIND_6)
			#define DST_UNITY_INDEX_9	6
			#define DST_UNITY_KIND_6	DST_KIND_9
		#elif !defined(DST_UNITY_KIND_7)
			#define DST_UNITY_INDEX_9	7
			#define DST_UNITY_KIND_7	DST_KIND_9
		#elif !defined(DST_UNITY_KIND_8)
			#define DST_UNITY_INDEX_9	8
			#define DST_UNITY_KIND_8	DST_KIND_9
		#elif !defined(DST_UNITY_KIND_9)
			#define DST_UNITY_INDEX_9	9
			#define DST_UNITY_KIND_9	DST_KIND_9
		#endif
	#else
		#define DST_UNITY_INDEX_9	(-1)
	#endif
#endif




/*****************************************************************
 *****************************************************************
 **		We verify that the FskBitmap.h pixel format codes		**
 **			have not changed, and abort if they have			**
 *****************************************************************
 *****************************************************************/

#define DISPATCH_FORMATS_OUT_OF_SYNC			\
		(kFskBitmapFormat24BGR 			!=	1)	\
	||	(kFskBitmapFormat32ARGB			!=	2)	\
	||	(kFskBitmapFormat32BGRA			!=	3)	\
	||	(kFskBitmapFormat32RGBA			!=	4)	\
	||	(kFskBitmapFormat24RGB			!=	5)	\
	||	(kFskBitmapFormat16RGB565BE		!=	6)	\
	||	(kFskBitmapFormat16RGB565LE		!=	7)	\
	||	(kFskBitmapFormat16RGB5515LE	!=	8)	\
	||	(kFskBitmapFormatYUV420			!=	9)	\
	||	(kFskBitmapFormatYUV422			!=	10)	\
	||	(kFskBitmapFormat8A				!=	11)	\
	||	(kFskBitmapFormat8G				!=	12)	\
	||	(kFskBitmapFormatYUV420spuv		!=	13)	\
	||	(kFskBitmapFormat16BGR565LE		!=	14)	\
	||	(kFskBitmapFormat16AG			!=	15)	\
	||	(kFskBitmapFormat32ABGR			!=	16)	\
	||	(kFskBitmapFormat16RGBA4444LE	!=	17)	\
	||	(kFskBitmapFormatUYVY			!=	18) \
	||	(kFskBitmapFormatYUV420i		!=	19) \
	||	(kFskBitmapFormat32A16RGB565LE	!=	20) \
	||	(kFskBitmapFormatYUV420spvu		!=	21) \
		/****************************************
		 *	^  ADD NEW PIXEL FORMATS HERE  ^	*
		 ****************************************/


/*****************************************************************
 *****************************************************************
 **		We build the mapping table from							**
 **		pixel format codes to pixel kind indices.				**
 **																**
 **		ORDER IS CRITICAL HERE									**
 **																**
 **		The order must be identical to that in FskBitmap.h.		**
 *****************************************************************
 *****************************************************************/

#if NUM_SRC_FORMATS > 0
static const signed char srcPixelFormatToPixelKindIndex[] = {
	SRC_INDEX_INTRODUCTION,
	SRC_INDEX_24BGR,
	SRC_INDEX_32ARGB,
	SRC_INDEX_32BGRA,
	SRC_INDEX_32RGBA,
	SRC_INDEX_24RGB,
	SRC_INDEX_16RGB565BE,
	SRC_INDEX_16RGB565LE,
	SRC_INDEX_16RGB5515LE,
	SRC_INDEX_YUV420,
	SRC_INDEX_YUV422,
	SRC_INDEX_8A,
	SRC_INDEX_8G,
	SRC_INDEX_YUV420spuv,
	SRC_INDEX_16BGR565LE,
	SRC_INDEX_16AG,
	SRC_INDEX_32ABGR,
	SRC_INDEX_16RGBA4444LE,
	SRC_INDEX_UYVY,
	SRC_INDEX_YUV420i,
	SRC_INDEX_32A16RGB565LE,
	SRC_INDEX_YUV420spvu
	/****************************************
	 *	^  ADD NEW PIXEL FORMATS HERE  ^	*
	 ****************************************/
};
#endif /* NUM_SRC_FORMATS */



#if NUM_DST_FORMATS > 0
static const signed char dstPixelFormatToPixelKindIndex[] = {
	DST_INDEX_INTRODUCTION,
	DST_INDEX_24BGR,
	DST_INDEX_32ARGB,
	DST_INDEX_32BGRA,
	DST_INDEX_32RGBA,
	DST_INDEX_24RGB,
	DST_INDEX_16RGB565BE,
	DST_INDEX_16RGB565LE,
	DST_INDEX_16RGB5515LE,
	DST_INDEX_YUV420,
	DST_INDEX_YUV422,
	DST_INDEX_8A,
	DST_INDEX_8G,
	DST_INDEX_YUV420spuv,
	DST_INDEX_16BGR565LE,
	DST_INDEX_16AG,
	DST_INDEX_32ABGR,
	DST_INDEX_16RGBA4444LE,
	DST_INDEX_UYVY,
	DST_INDEX_YUV420i,
	DST_INDEX_32A16RGB565LE,
	DST_INDEX_YUV420spvu
	/****************************************
	 *	^  ADD NEW PIXEL FORMATS HERE  ^	*
	 ****************************************/
};
#endif /* NUM_DST_FORMATS */



#if NUM_DST_UNITY_FORMATS > 0
static const signed char dstPixelKindUnityIndex[NUM_DST_FORMATS] = {
			DST_UNITY_INDEX_0
	#if NUM_DST_FORMATS > 1
		,	DST_UNITY_INDEX_1
	#endif
	#if NUM_DST_FORMATS > 2
		,	DST_UNITY_INDEX_2
	#endif
	#if NUM_DST_FORMATS > 3
		,	DST_UNITY_INDEX_3
	#endif
	#if NUM_DST_FORMATS > 4
		,	DST_UNITY_INDEX_4
	#endif
	#if NUM_DST_FORMATS > 5
		,	DST_UNITY_INDEX_5
	#endif
	#if NUM_DST_FORMATS > 6
		,	DST_UNITY_INDEX_6
	#endif
	#if NUM_DST_FORMATS > 7
		,	DST_UNITY_INDEX_7
	#endif
	#if NUM_DST_FORMATS > 8
		,	DST_UNITY_INDEX_8
	#endif
	#if NUM_DST_FORMATS > 9
		,	DST_UNITY_INDEX_9
	#endif

};
#endif


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __FSKBLITDISPATCHDEF__ */

