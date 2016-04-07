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

/* IMPORTANT: Please define __FSKPLATFORM__ regardless of the platform to allow cross-platform testing of its inclusion */
#ifndef __FSKPLATFORM__
#define __FSKPLATFORM__


#include <math.h>
#include <time.h>

#define SRC_16RGB565LE			1
#define SRC_32BGRA				1
#define SRC_32A16RGB565LE		1

#define SRC_YUV420i				0
#define SRC_YUV420				1

#define SRC_8G					   0
#define SRC_16AG				   0

#define DST_16RGB565LE			1
#define DST_YUV420				0
//	#define DST_YUV422		   1
#define DST_8G					   0
#define DST_32BGRA				1

#define DST_UNITY_16RGB565LE	1

#define kFskBitmapFormatDefaultRGB  kFskBitmapFormat16RGB565LE
#define kFskBitmapFormatDefaultRGBA kFskBitmapFormat32BGRA
#define kFskBitmapFormatSourceDefaultRGBA	kFskBitmapFormat32A16RGB565LE
#define kFskBitmapFormatDefault     kFskBitmapFormatDefaultRGB

#define kFskBitmapFormatDefaultRGB  kFskBitmapFormat16RGB565LE
#define kFskBitmapFormatDefaultRGBA kFskBitmapFormat32BGRA
#define kFskBitmapFormatSourceDefaultRGBA	kFskBitmapFormat32A16RGB565LE
#define kFskBitmapFormatDefault     kFskBitmapFormatDefaultRGB

#undef USE_FRAMEBUFFER_VECTORS
#define USE_FRAMEBUFFER_VECTORS 1

#define kFskTextDefaultEngine "FreeType"

#define TARGET_RT_FPU			0

#undef SUPPORT_INSTRUMENTATION
#define SUPPORT_INSTRUMENTATION 0

#undef SUPPORT_MEMORY_DEBUG
#define SUPPORT_MEMORY_DEBUG	0

#undef SUPPORT_SYNCHRONIZATION_DEBUG
#define SUPPORT_SYNCHRONIZATION_DEBUG 0

#ifndef FSKBITMAP_OPENGL
	#if (1 == FSK_OPENGLES_KPL)
		#define FSKBITMAP_OPENGL 0	///< Insert the glPort pointer into the FskBitmap data structure. This by itself does not enable GL acceleration.
	#else
		#define FSKBITMAP_OPENGL 0	///< Do not insert the glPort pointer into the FskBitmap data structure.
	#endif
#endif

#endif /* __FSKPLATFORM__ */


