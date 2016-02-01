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
/* IMPORTANT: Please define __FSKPLATFORM__ regardless of the platform to allow cross-platform testing of its inclusion */
#ifndef __FSKPLATFORM__
#define __FSKPLATFORM__

#if defined(_MSC_VER)
	#define _longjmp longjmp
#endif

#define SRC_24BGR				1
#define SRC_32BGRA				1
#define SRC_32ARGB				1
#define SRC_16RGB565LE			1
#define SRC_16RGB565BE			0
#define SRC_YUV420				1
#define SRC_YUV420i				0
#define SRC_8G					1
#define SRC_16AG				1
#define SRC_16RGBA4444LE		0

#define DST_24BGR				1
#define DST_32BGRA				1
#define DST_16RGB565LE			1
#define DST_YUV420				1
#define DST_8G					1
#define DST_32ABGR				1
#define DST_16RGBA4444LE		0

#define DST_UNITY_24BGR			1

#define kFskBitmapFormatDefaultRGB kFskBitmapFormat24BGR
#define kFskBitmapFormatDefaultRGBA kFskBitmapFormat32BGRA
#define kFskBitmapFormatDefault kFskBitmapFormatDefaultRGB

#define kFskTextDefaultEngine "Kpl/Win32/GDI"

#define TARGET_RT_FPU			1

#define SUPPORT_EDGEENHANCEDTEXT 1

#define USE_FRAMEBUFFER_VECTORS 1

#undef SUPPORT_INSTRUMENTATION
#define SUPPORT_INSTRUMENTATION 0

#undef SUPPORT_MEMORY_DEBUG
#define SUPPORT_MEMORY_DEBUG 0

#undef SUPPORT_SYNCHRONIZATION_DEBUG
#define SUPPORT_SYNCHRONIZATION_DEBUG 0

#ifndef FSKBITMAP_OPENGL
	#if (1 == FSK_OPENGLES_KPL)
		#define FSKBITMAP_OPENGL 1	///< Insert the glPort pointer into the FskBitmap data structure. This by itself does not enable GL acceleration.
	#else
		#define FSKBITMAP_OPENGL 0	///< Do not insert the glPort pointer into the FskBitmap data structure.
	#endif
#endif /* FSKBITMAP_OPENGL */

#endif /* __FSKPLATFORM__ */
