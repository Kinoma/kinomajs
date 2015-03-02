/*
	Copyright (c) 2002-2014 Kinoma Inc.

	All rights reserved.
*/

/* IMPORTANT: Please define __FSKPLATFORM__ regardless of the platform to allow cross-platform testing of its inclusion */
#ifndef __FSKPLATFORM__
#define __FSKPLATFORM__

#include <math.h>
#include <time.h>

#define SRC_16AG				1
#define SRC_16RGB565BE			0
#define SRC_16RGB565LE			1
#define SRC_16RGBA4444BE		0
#define SRC_16RGBA4444LE		0
#define SRC_24BGR				1
#define SRC_32ARGB				1
#define SRC_32BGRA				1
#define SRC_32RGBA				1
#define SRC_8G					1
#define SRC_YUV420				1

#define DST_16RGB5515BE			0
#define DST_16RGB5515LE			0
#define DST_16RGB565BE			1
#define DST_16RGB565LE			1
#define DST_16RGBA4444BE		0
#define DST_16RGBA4444LE		0
#define DST_24BGR				1
#define DST_32ABGR				1
#define DST_32ARGB				1
#define DST_32BGRA				1
#define DST_32RGBA				1
#define DST_8G					1
#define DST_YUV420				1

#define DST_UNITY_32ABGR		1
#define DST_UNITY_32ARGB		1

#define kFskBitmapFormatDefaultRGB kFskBitmapFormat32BGRA
#define kFskBitmapFormatDefaultRGBA kFskBitmapFormat32BGRA
#define kFskBitmapFormatDefault kFskBitmapFormatDefaultRGB

#define kFskTextDefaultEngine "FreeType"

#define TARGET_RT_FPU			0

#undef USE_FRAMEBUFFER_VECTORS
#define USE_FRAMEBUFFER_VECTORS 0

#undef SUPPORT_INSTRUMENTATION
#define SUPPORT_INSTRUMENTATION 0

#undef SUPPORT_MEMORY_DEBUG
#define SUPPORT_MEMORY_DEBUG	0

#undef SUPPORT_SYNCHRONIZATION_DEBUG
#define SUPPORT_SYNCHRONIZATION_DEBUG 0

#ifndef FSKBITMAP_OPENGL
	#if (1 == FSK_OPENGLES_KPL)
		#define FSKBITMAP_OPENGL 1	///< Insert the glPort pointer into the FskBitmap data structure. This by itself does not enable GL acceleration.
	#else
		#define FSKBITMAP_OPENGL 0	///< Do not insert the glPort pointer into the FskBitmap data structure.
	#endif
#endif

#endif /* __FSKPLATFORM__ */


