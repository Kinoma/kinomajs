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
/********************************************************************************
 * Prior to invoking this file, it is necessary to set the preprocessor variables
 *	SRC_pixelFormat, DST_pixelFormat, and DST_UNITY_pixelFormat
 * for all pixel formats that will be accommodated on a particular platform.
 * An example for kFskBitmapFormat32ARGB follows:
 *	#define	SRC_32ARGB				1
 *	#define	DST_32ARGB				1
 *	#define	DST_UNITY_32ARGB		1
 * It is not necessary to set any undesired pixel format SRC and DST to 0,
 * because this will be done automatically.
 *
 * It is also necessry to
 *		#define GENERAL_BLIT_PROTO_FILE "MyGeneralBlitProto.c"
 * and/or
 *		#define UNITY_BLIT_PROTO_FILE   "MyUnityBlitProto.c"
 * to the appropriate prototype files before invoking this file.
 *
 * This file will then invoke those prototypes for each pair of SRC and DST
 * pixel formats.
 ********************************************************************************/


/********************************************************************************
 * The file FskBlitDispatchDef.h should have been included before,
 * but it's no big deal, it can be included here.  It will initialize
 * NUM_SRC_FORMATS, 		SRC_KIND_0,			SRC_KIND_1, ...
 * NUM_DST_FORMATS, 		DST_KIND_0,			DST_KIND_1, ...
 * NUM_DST_UNITY_FORMATS	DST_UNITY_KIND_0,	DST_UNITY_KIND_1, ...
 * static const signed char srcPixelFormatToPixelKindIndex[];
 * static const signed char dstPixelFormatToPixelKindIndex[];
 * static const signed char dstPixelKindUnityIndex[NUM_DST_FORMATS];
 ********************************************************************************/

#ifndef __FSKBLITDISPATCHDEF__
	#include "FskBlitDispatchDef.h"
#endif /* __FSKBLITDISPATCHDEF__ */



/********************************************************************************
 * Generate general blit procs
 ********************************************************************************/
#ifdef GENERAL_BLIT_PROTO_FILE
	#define BLIT_PROTO_FILE	GENERAL_BLIT_PROTO_FILE

	#if NUM_DST_FORMATS >= 1
		#define DST_KIND	DST_KIND_0
		#include "FskBlitSrc.h"
	#endif /* NUM_DST_FORMATS >= 1 */

	#if NUM_DST_FORMATS >= 2
		#define DST_KIND	DST_KIND_1
		#include "FskBlitSrc.h"
	#endif /* NUM_DST_FORMATS >= 2 */

	#if NUM_DST_FORMATS >= 3
		#define DST_KIND	DST_KIND_2
		#include "FskBlitSrc.h"
	#endif /* NUM_DST_FORMATS >= 3 */

	#if NUM_DST_FORMATS >= 4
		#define DST_KIND	DST_KIND_3
		#include "FskBlitSrc.h"
	#endif /* NUM_DST_FORMATS >= 4 */

	#if NUM_DST_FORMATS >= 5
		#define DST_KIND	DST_KIND_4
		#include "FskBlitSrc.h"
	#endif /* NUM_DST_FORMATS >= 5 */

	#if NUM_DST_FORMATS >= 6
		#define DST_KIND	DST_KIND_5
		#include "FskBlitSrc.h"
	#endif /* NUM_DST_FORMATS >= 6 */

	#if NUM_DST_FORMATS >= 7
		#define DST_KIND	DST_KIND_6
		#include "FskBlitSrc.h"
	#endif /* NUM_DST_FORMATS >= 7 */

	#if NUM_DST_FORMATS >= 8
		#define DST_KIND	DST_KIND_7
		#include "FskBlitSrc.h"
	#endif /* NUM_DST_FORMATS >= 8 */

	#if NUM_DST_FORMATS >= 9
		#define DST_KIND	DST_KIND_8
		#include "FskBlitSrc.h"
	#endif /* NUM_DST_FORMATS >= 9 */

	#if NUM_DST_FORMATS >= 10
		#define DST_KIND	DST_KIND_9
		#include "FskBlitSrc.h"
	#endif /* NUM_DST_FORMATS >= 10 */

	#if NUM_DST_FORMATS >= 11
		#define DST_KIND	DST_KIND_10
		#include "FskBlitSrc.h"
	#endif /* NUM_DST_FORMATS >= 11 */

	#if NUM_DST_FORMATS >= 12
		#define DST_KIND	DST_KIND_11
		#include "FskBlitSrc.h"
	#endif /* NUM_DST_FORMATS >= 12 */

	#if NUM_DST_FORMATS >= 13
		#define DST_KIND	DST_KIND_12
		#include "FskBlitSrc.h"
	#endif /* NUM_DST_FORMATS >= 13 */

	#if NUM_DST_FORMATS >= 14
		#define DST_KIND	DST_KIND_13
		#include "FskBlitSrc.h"
	#endif /* NUM_DST_FORMATS >= 14 */

	#if NUM_DST_FORMATS >= 15
		#define DST_KIND	DST_KIND_14
		#include "FskBlitSrc.h"
	#endif /* NUM_DST_FORMATS >= 15 */

	#if NUM_DST_FORMATS >= 16
		#define DST_KIND	DST_KIND_15
		#include "FskBlitSrc.h"
	#endif /* NUM_DST_FORMATS >= 16 */


	#if NUM_DST_FORMATS >= 17
		#error Unexpected large number of destination pixel formats
	#endif /* NUM_DST_FORMATS >= 10 */
	
	#undef BLIT_PROTO_FILE
	#undef GENERAL_BLIT_PROTO_FILE

#endif /* GENERAL_BLIT_PROTO_FILE */




/********************************************************************************
 * Generate unity blit procs
 ********************************************************************************/

#ifdef UNITY_BLIT_PROTO_FILE
	#define BLIT_PROTO_FILE	UNITY_BLIT_PROTO_FILE

	#if NUM_DST_FORMATS >= 1
		#define DST_KIND	DST_UNITY_KIND_0
		#include "FskBlitSrc.h"
	#endif /* NUM_DST_UNITY_FORMATS >= 1 */

	#if NUM_DST_UNITY_FORMATS >= 2
		#define DST_KIND	DST_UNITY_KIND_1
		#include "FskBlitSrc.h"
	#endif /* NUM_DST_UNITY_FORMATS >= 2 */

	#if NUM_DST_UNITY_FORMATS >= 3
		#define DST_KIND	DST_UNITY_KIND_2
		#include "FskBlitSrc.h"
	#endif /* NUM_DST_UNITY_FORMATS >= 3 */

	#if NUM_DST_UNITY_FORMATS >= 4
		#define DST_KIND	DST_UNITY_KIND_3
		#include "FskBlitSrc.h"
	#endif /* NUM_DST_UNITY_FORMATS >= 4 */

	#if NUM_DST_UNITY_FORMATS >= 5
		#define DST_KIND	DST_UNITY_KIND_4
		#include "FskBlitSrc.h"
	#endif /* NUM_DST_UNITY_FORMATS >= 5 */

	#if NUM_DST_UNITY_FORMATS >= 6
		#define DST_KIND	DST_UNITY_KIND_5
		#include "FskBlitSrc.h"
	#endif /* NUM_DST_UNITY_FORMATS >= 6 */

	#if NUM_DST_UNITY_FORMATS >= 7
		#define DST_KIND	DST_UNITY_KIND_6
		#include "FskBlitSrc.h"
	#endif /* NUM_DST_UNITY_FORMATS >= 7 */

	#if NUM_DST_UNITY_FORMATS >= 8
		#define DST_KIND	DST_UNITY_KIND_7
		#include "FskBlitSrc.h"
	#endif /* NUM_DST_UNITY_FORMATS >= 8 */

	#if NUM_DST_UNITY_FORMATS >= 9
		#define DST_KIND	DST_UNITY_KIND_8
		#include "FskBlitSrc.h"
	#endif /* NUM_DST_UNITY_FORMATS >= 9 */

	#if NUM_DST_UNITY_FORMATS >= 10
		#define DST_KIND	DST_UNITY_KIND_9
		#include "FskBlitSrc.h"
	#endif /* NUM_DST_UNITY_FORMATS >= 10 */

	#if NUM_DST_UNITY_FORMATS >= 11
		#define DST_KIND	DST_UNITY_KIND_10
		#include "FskBlitSrc.h"
	#endif /* NUM_DST_UNITY_FORMATS >= 11 */

	#if NUM_DST_UNITY_FORMATS >= 12
		#define DST_KIND	DST_UNITY_KIND_11
		#include "FskBlitSrc.h"
	#endif /* NUM_DST_UNITY_FORMATS >= 12 */

	#if NUM_DST_UNITY_FORMATS >= 13
		#define DST_KIND	DST_UNITY_KIND_12
		#include "FskBlitSrc.h"
	#endif /* NUM_DST_UNITY_FORMATS >= 13 */

	#if NUM_DST_UNITY_FORMATS >= 14
		#define DST_KIND	DST_UNITY_KIND_13
		#include "FskBlitSrc.h"
	#endif /* NUM_DST_UNITY_FORMATS >= 14 */

	#if NUM_DST_UNITY_FORMATS >= 15
		#define DST_KIND	DST_UNITY_KIND_14
		#include "FskBlitSrc.h"
	#endif /* NUM_DST_UNITY_FORMATS >= 15 */

	#if NUM_DST_UNITY_FORMATS >= 16
		#define DST_KIND	DST_UNITY_KIND_15
		#include "FskBlitSrc.h"
	#endif /* NUM_DST_UNITY_FORMATS >= 16 */

	#if NUM_DST_UNITY_FORMATS >= 17
		#error Unexpected large number of destination pixel formats
	#endif /* NUM_DST_UNITY_FORMATS >= 17 */
	
	#undef BLIT_PROTO_FILE
	#undef UNITY_BLIT_PROTO_FILE

#endif /* UNITY_BLIT_PROTO_FILE */

