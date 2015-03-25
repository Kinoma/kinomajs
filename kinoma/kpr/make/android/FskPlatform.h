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

#ifndef FSKBITMAP_OPENGL
	#define FSKBITMAP_OPENGL 1	///< Enable  generation of code to support OpenGL-accelerated graphics.
//	#define FSKBITMAP_OPENGL 0	///< Disable generation of code to support OpenGL-accelerated graphics.
#endif /* FSKBITMAP_OPENGL */

#if FSKBITMAP_OPENGL
#define SRC_32BGRA				0	/* Many GLES  implementations support this */
#define SRC_32RGBA				1	/* All GL(ES) implementations support this */
#define DST_32BGRA				0	/* Few GL implementations read into this format */
#define DST_32RGBA				1	/* All GL implementations read into this format */
#define kFskBitmapFormatDefaultRGB			kFskBitmapFormat32RGBA
#define kFskBitmapFormatDefaultRGBA			kFskBitmapFormat32RGBA
#else
#define SRC_32BGRA				1	/* Many GLES  implementations support this */
#define SRC_32RGBA				0	/* All GL(ES) implementations support this */
#define DST_32BGRA				1	/* Few GL implementations read into this format */
#define DST_32RGBA				0	/* All GL implementations read into this format */
#define kFskBitmapFormatDefaultRGB			kFskBitmapFormat16RGB565LE
#define kFskBitmapFormatDefaultRGBA			kFskBitmapFormat32BGRA
#define kFskBitmapFormatSourceDefaultRGBA	kFskBitmapFormat32A16RGB565LE
#endif

#define kFskBitmapFormatDefault     		kFskBitmapFormatDefaultRGB


#define SRC_16RGB565LE			1	/* Most GLES  implementations support this as a texture */
#define SRC_YUV420				1	/* We have a GL shader that supports  this */
#define SRC_YUV420i				1	/* Difficult for GL,  fastest for software */
#define SRC_32A16RGB565LE		1

#define DST_16RGB565LE			1	/* Most GLES implementations display  this */
#define DST_UNITY_16RGB565LE	1



#define kFskTextDefaultEngine "FreeType"

#define	SUPPORT_REMOTE_NOTIFICATION	0

#define TARGET_RT_FPU			0

#undef SUPPORT_INSTRUMENTATION
#define SUPPORT_INSTRUMENTATION 0

#undef SUPPORT_MEMORY_DEBUG
#define SUPPORT_MEMORY_DEBUG 0

#undef SUPPORT_SYNCHRONIZATION_DEBUG
#define SUPPORT_SYNCHRONIZATION_DEBUG 0

#endif /* __FSKPLATFORM__ */


