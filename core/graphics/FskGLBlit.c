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
	\file	FskGLBlit.c
	\brief	OpenGL and OpenGL-ES implementations of blits.
*/

#ifdef BG3CDP
	#define BG3CDP_GL	1
#endif /* BG3CDP */
#if 0 //BG3CDP_GL	/* Perhaps this will reduce memory usage on BG3CDP? */
	#define RGB_TEXTURES_WORK	1	/**< This indicates that RGB textures function on this platform. */
#else /* ! BG3CDP_GL */
	#define RGB_TEXTURES_WORK 	0	/**< This indicates that RGB textures do not function on this platform. */
#endif /* ! BG3CDP_GL */

#define __FSKBITMAP_PRIV__	/**< Gain access to the fields of the bitmap. */
#define __FSKTEXT_PRIV__	/**< Get access to the fields of text. */
#define __FSKGLBLIT_PRIV__	/**< Get access to GL private functions. */
#include "FskGLBlit.h"		/* No header come before this to assure that it is self-sufficient. */
#if FSKBITMAP_OPENGL		/* This brackets the whole file */

/************************************************ Debugging configuration ************************************************/
#if SUPPORT_INSTRUMENTATION
	//#define LOG_COLOR				/**< Log when changing color. */
	//#define LOG_CONTEXT			/**< Log context calls. */
	//#define LOG_EPHEMERAL			/**< Log ephemeral texture usage. */
	//#define LOG_GL_API			/**< Log the OpenGL API calls. */
	#define LOG_INIT				/**< Log calls related to initialization and shutdown. */
	#define LOG_PARAMETERS			/**< Log the parameters of API calls. */
	//#define LOG_MEMORY			/**< Log the amount of memory that we use. */
	//#define LOG_READPIXELS		/**< Log any call to read from the screen. */
	//#define LOG_RENDER_TO_TEXTURE	/**< Log when changing render target. */
	#define LOG_SET_TEXTURE		/**< Log texture uploads. */
	#define LOG_SHUTDOWN			/**< Log the results of shutting down. */
	//#define LOG_SWAPBUFFERS		/**< Log swapbuffer calls, along with timestamp. */
	//#define LOG_TEXT				/**< Log any text-related activity. */
	//#define LOG_TEXTURE_LIFE		/**< Log the creation and deletion of textures. */
	//#define LOG_VIEW				/**< Log calls that change view. */
	//#define LOG_YUV				/**< Log calls related to YUV. */
#endif /* SUPPORT_INSTRUMENTATION */
//#define DUMP_TYPEFACE				/**< Dump typefaces when disposing of them. */


//#define GL_DEBUG 1
#ifndef GL_DEBUG
	#define GL_DEBUG 0				/**< Turn off extra debugging logs. */
#endif /* GL_DEBUG */
#if									\
	defined(LOG_COLOR)				|| \
	defined(LOG_EPHEMERAL)			|| \
	defined(LOG_GL_API)				|| \
	defined(LOG_INIT)				|| \
	defined(LOG_MEMORY)				|| \
	defined(LOG_PARAMETERS)			|| \
	defined(LOG_READPIXELS)			|| \
	defined(LOG_RENDER_TO_TEXTURE)	|| \
	defined(LOG_SET_TEXTURE)		|| \
	defined(LOG_SHUTDOWN)			|| \
	defined(LOG_SWAPBUFFERS)		|| \
	defined(LOG_TEXT)				|| \
	defined(LOG_TEXTURE_LIFE)		|| \
	defined(LOG_VIEW)				|| \
	defined(LOG_YUV)
	#undef  GL_DEBUG
	#define GL_DEBUG 1
#endif /* LOG_PARAMETERS et al */

#define FSK_DEBUG				0	/**< Extra debugging messages. */
#define TEST_YUV_SHADERS		0	/**< Use YUV shaders even if there is hardware support for YUV. */
//#define FLUSH_OFTEN				/**< This changes GL from a fast asynchronous API to a slow synchronous one. */
#if 0//GLES_VERSION == 2
	#define DEBUG_ES2 1				/**< Extra debugging for GL ES 2. */
#endif /* GLES_VERSION == 2 */

#ifndef CHECK_GL_ERROR
	#if 0 //GL_DEBUG || FSK_DEBUG
		#define CHECK_GL_ERROR	1	/**< Make all calls synchronous, and check for errors after each one. */
	#else
		#define CHECK_GL_ERROR	0	/**< Do not check for errors, allowing GL processing to happen asynchronously. */
	#endif /* *_DEBUG */
#endif /* CHECK_GL_ERROR */
#if CHECK_GL_ERROR
	#undef GL_DEBUG
	#define GL_DEBUG 1
	static const char* GLErrorStringFromCode(int err);
#endif /* CHECK_GL_ERROR */

/* 	In order to enable debug output on Android, it is necessary to:
 *	(1) #define SUPPORT_INSTRUMENTATION 1 in FskPlatform.android.h
 *	(2) put this into manifest.xml:
 *			<instrument platform="android" androidlog="true" threads="true" trace="false">
 *				<kind name="opengl" messages="debug"/>
 *			</instrument>
 *			<instrument platform="mac" threads="true" trace="true">
 *				<kind name="opengl" messages="debug"/>
 *			</instrument>
 *		On android, you can see the output by executing "adb logcat" on Android,
 *		while on the Mac, the output will go to stdout, but
 *	(3) Perhaps #define LOG_PARAMETERS or GL_DEBUG
 *	(4) fskbuild --clean [ --run ] [ --test ]
 *	(5a) To view the output on Android, type "adb logcat" in the host (Mac or Linux) Terminal window.
 *	(5b) To view the output on the Mac, you can
 *	     - launch from XCode, and the output will be displayed in the console window.
 *	     - launch from Terminal with:
 *	     	cd ${F_HOME}/bin/mac/debug/Kinoma Simulator.app/Contents/MacOS"
 *	     	./fsk"
 *	       and you will be able to see the output in the Terminal window.
 */

#if SUPPORT_INSTRUMENTATION
	#ifndef GL_DEBUG
		#define GL_DEBUG 1
	#endif /* GL_DEBUG */
	#define FskInstrumentedGlobalType(type)			(&(g##type##TypeInstrumentation))												/**< The data structure created with FskInstrumentedSimpleType(). */
	#define FskInstrumentedTypeEnabled(type, level)	FskInstrumentedTypeHasListenersForLevel(FskInstrumentedGlobalType(type), level)	/**< Whether instrumentation will print anything. */
#else /* !SUPPORT_INSTRUMENTATION */
	#define FskInstrumentedTypeEnabled(type, level)	(0)																				/**< Instrumentation will not print anything. */
#endif /* SUPPORT_INSTRUMENTATION */

FskInstrumentedSimpleType(OpenGL, opengl);													/**< This declares the types needed for instrumentation. */

#if GL_DEBUG
	#define	LOGD(...)	FskOpenGLPrintfDebug(__VA_ARGS__)									/**< Print debugging logs. */
	#define	LOGI(...)	FskOpenGLPrintfVerbose(__VA_ARGS__)									/**< Print information logs. */
#endif /* GL_DEBUG */
#define		LOGE(...)	FskOpenGLPrintfMinimal(__VA_ARGS__)									/**< Print error logs always, when instrumentation is on. */
#ifndef     LOGD
	#define LOGD(...)	do {} while(0)														/**< Don't print debugging logs. */
#endif   /* LOGD */
#ifndef     LOGI
	#define LOGI(...)	do {} while(0)														/**< Don't print information logs. */
#endif   /* LOGI */
#ifndef     LOGE
	#define LOGE(...)	do {} while(0)														/**< Don't print error logs. */
#endif   /* LOGE */
#define LOGD_ENABLED()	FskInstrumentedTypeEnabled(OpenGL, kFskInstrumentationLevelDebug)	/**< Whether LOGD() will print anything. */
#define LOGI_ENABLED()	FskInstrumentedTypeEnabled(OpenGL, kFskInstrumentationLevelVerbose)	/**< Whether LOGI() will print anything. */
#define LOGE_ENABLED()	FskInstrumentedTypeEnabled(OpenGL, kFskInstrumentationLevelMinimal)	/**< Whether LOGE() will print anything. */

/************************************************ Headers, system first ************************************************/
#ifdef __APPLE__
	#if TARGET_OS_IPHONE
		#if GLES_VERSION == 1
			#import <OpenGLES/ES1/gl.h>
		#else /* GLES_VERSION == 2 */
			#import <OpenGLES/ES2/gl.h>
			#import <OpenGLES/ES2/glext.h>
		#endif /* GLES_VERSION == 2 */
	#else /* !TARGET_OS_IPHONE */
		#include <OpenGL/gl.h>			// Header File For The OpenGL Library
	#endif /* !TARGET_OS_IPHONE */
#elif TARGET_OS_ANDROID || TARGET_OS_KPL || defined(__linux__) || (FSK_OPENGLES_ANGLE == 1)
	#define GL_GLEXT_PROTOTYPES
	#ifndef GLES_VERSION
		#include "FskGLBlit.h"
	#endif /* GLES_VERSION */
	#if GLES_VERSION == 2
		#include <GLES2/gl2.h>		// Header File For the GLES 2 Library
		#include <GLES2/gl2ext.h>	// Header file for standard GLES 2 extensions.
	#else /* GLES_VERSION == 1 */
		#include <GLES/gl.h>		// Header File For the GLES 1 Library
		#include <GLES/glext.h>		// Header file for standard GLES 1 extensions.
	#endif /* GLES_VERSION */
	#define EGL_EGLEXT_PROTOTYPES
	#include <EGL/egl.h>
	#include <EGL/eglext.h>
	#include <EGL/eglplatform.h>
	#define __FSKWINDOW_PRIV__
	#include "FskWindow.h"
	#if TARGET_OS_ANDROID
		#include <android/native_window.h>
	#endif /* android */
	#if TARGET_OS_KPL
		#include "KplGL.h"
	#endif /* TARGET_OS_KPL */
#elif defined(_MSC_VER)
	#include <gl/gl.h>				// Header File For The OpenGL32 Library
#else
	#error unknown OS
#endif /* OS */
#if (FSK_OPENGLES_ANGLE == 1) || !(defined(GL_VERSION_1_1) || defined(GL_VERSION_2_0) )	/* If either ANGLE or GLES */
	#undef GL_UNPACK_SKIP_ROWS		/* Neither the ANGLE library nor GL ES supports calling glPixelStorei() with these values */
	#undef GL_UNPACK_SKIP_PIXELS	/* but some GLES header files define them anyway */
	#undef GL_UNPACK_ROW_LENGTH
#endif /* (FSK_OPENGLES_ANGLE == 1) */

#ifdef TRACK_TEXTURES
	#include "FskGrowableStorage.h"
#endif /* TRACK_TEXTURES */

#include <errno.h>
#include <math.h>
#include <stdarg.h>

#include "FskBitmap.h"
#include "FskBlit.h"
#include "FskErrors.h"
#include "FskGLContext.h"
#include "FskMemory.h"
#include "FskPixelOps.h"
#include "FskRectBlit.h"
#include "FskString.h"
#include "FskText.h"	/* This relies on USE_GLYPH */
#if USE_GLYPH
	#include "FskTextConvert.h"
#endif /* USE_GLYPH */
#if TARGET_OS_MAC
	#if TARGET_OS_IPHONE
		#include "FskCocoaSupportPhone.h"
	#else /* !TARGET_OS_IPHONE */
		#include "FskCocoaSupport.h"
	#endif /* !TARGET_OS_IPHONE */
	#include "FskYUV420ToYUV422.h"
#endif /* TARGET_OS_MAC */
#if USE_FRAMEBUFFER_VECTORS
	#include "FskFrameBuffer.h"
#endif /* USE_FRAMEBUFFER_VECTORS */

#ifdef BG3CDP_GL
#if 1
	#include "gc_hal_base.h"
#else /* For Ken to compile as android */
	#include "../../libraries/amp/gpu_hal_kernel_inc/gc_hal_base.h"
#endif
#endif /* BG3CDP_GL */

#ifdef LOG_GL_API
	#include "FskGLAPILog.h"
#endif /* LOG_GL_API */


/************************************************ System configuration, macros and typedefs ************************************************/
#define OPTIMIZE_STATE_CHANGES				1	/**< When true, eliminates redundant state changes; otherwise always set state changes. */
#define GL_RGB_422_APPLE_WORKS_ON_IPHONE	0	/* At this time, GL_RGB_422_APPLE does not work on the iPhone. */

#define TEXTURE_MIN_DIM						16	/**< No texture will be allocated with dimension smaller than this.   */
#define	TEXTURE_BLOCK_HORIZONTAL			4	/**< All textures will have  widths that are multiples of this value. */
#define	TEXTURE_BLOCK_VERTICAL				4	/**< All textures will have heights that are multiples of this value. */
#define ALPHA_BLOCK_VERTICAL				32	/**< Alpha textures need to be a multiple of 32 to avoids crashing on Qualcomm Adreno */
#define SAVE_TEX_DIM						256	/**< Dispose of textures larger than this in any dimension. */

#define EPHEMERAL_TEXTURE_CACHE_SIZE		0	/**< This is the number of ephemeral texture objects kept around. 6 allows ping pong {Y,U,V}. */
#define USE_PORT_POOL						1	/**< Saves disposed glPorts in a pool. */
#define MAX_PORT_POOL_SIZE					16	/**< The maximum number of ports to keep in the port pool. */


#if GL_VERSION_2_0
 #define LOWP
 #define MEDIUMP
 #define HIGHP
#else /* < GL_VERSION_2_0 */
 #define LOWP				"lowp "		/**< Low    precision ( 8 bits) */
 #define MEDIUMP			"mediump "	/**< Medium precision (10 bits) */
 #define HIGHP				"highp "	/**< High   precision (32 bits) */
#endif /* < GL_VERSION_2_0 */

#define TEXVERTEX_STRIDE		4		/**< 4 for xyuvxyuvxyuvxyuv... is the only one supported. */
#define TEXVERTEX_X				0		/**< The offset of the X coordinate in textured vertices. */
#define TEXVERTEX_Y				1		/**< The offset of the Y coordinate in textured vertices. */
#define TEXVERTEX_U				2		/**< The offset of the U coordinate in textured vertices. */
#define TEXVERTEX_V				3		/**< The offset of the V coordinate in textured vertices. */
#define VERTEX_POSITION_INDEX	0		/**< The      vertex        (x,y) occupies the first  position in the vertex attributes. */
#define VERTEX_TEXCOORD_INDEX	1		/**< The texture coordinate (u,v) occupies the second position in the vertex attributes. */
#define VERTEX_END_INDEX		(-1)	/**< End the list of vertex attributes. */
#define UNICODE_ELLIPSIS		0x2026	/**< The unicode code point for ellipsis. */
#if USE_GLYPH
	#define REPLACEMENT_CODE	0		/**< The strike of the character to use when no strike has been found. */
#else /* !USE_GLYPH */
	#define REPLACEMENT_CODE	0xFFFD	/**< The strike of the character to use when no strike has been found. */
#endif /* !USE_GLYPH */

//#define MESH_WITH_TRIANGLES

#define UCHAR_TO_FLOAT_COLOR(u)	((u) * (1.f / 255.f))		/**< Convert from a color represented as an unsigned char to a float. */
#define CEIL_MULTIPLE(x, b)	(((x) + (b) - 1) & ~((b) - 1))	/**< Bump x up to the next multiple of b. This only works when b is a power of two */

#define BLOCKIFY(i, blockSize)		(((i) + ((blockSize) - 1)) & ~((blockSize) - 1))	/**< Bump up to the next higher multiple of blockSize, which must be a power of 2. */

#if !defined(GL_BGRA) && defined(GL_BGRA_EXT)
	#define GL_BGRA GL_BGRA_EXT
#endif /* !defined(GL_BGRA) && defined(GL_BGRA_EXT) */
#if !defined(GL_BGRA) && defined(GL_BGRA_IMG)
	#define GL_BGRA GL_BGRA_IMG
#endif /* !defined(GL_BGRA) && defined(GL_BGRA_IMG) */
#if !defined(GL_UNSIGNED_SHORT_4_4_4_4_REV) && defined(GL_UNSIGNED_SHORT_4_4_4_4_REV_EXT)
	#define GL_UNSIGNED_SHORT_4_4_4_4_REV GL_UNSIGNED_SHORT_4_4_4_4_REV_EXT
#endif /* !defined(GL_UNSIGNED_SHORT_4_4_4_4_REV) && defined(GL_UNSIGNED_SHORT_4_4_4_4_REV_EXT) */
#if !defined(GL_UNSIGNED_SHORT_1_5_5_5_REV) && defined(GL_UNSIGNED_SHORT_1_5_5_5_REV_EXT)
	#define GL_UNSIGNED_SHORT_1_5_5_5_REV GL_UNSIGNED_SHORT_1_5_5_5_REV_EXT
#endif /* !defined(GL_UNSIGNED_SHORT_1_5_5_5_REV) && defined(GL_UNSIGNED_SHORT_1_5_5_5_REV_EXT) */

#define ROUND_FIXED_TO_INT(x)	(((x) + 0x8000) >> 16)		/**< Round a  fixed   point number to the nearest integer. \param[in] x the  fixed   point number; \return the integer. */
#define ROUND_FLOAT_TO_INT(x)	(int)((x) + .5f)			/**< Round a floating point number to the nearest integer. \param[in] x the floating point number; \return the integer. */

#define BLEND_HASH_FACTOR									(47U)	/**< Choose from 22, 26, 27, 28, 29, 30, 36, 37, 38, 39, 40, 41, 42, 43, 44, 46, 47, 48, 49, 50. */
#define BLEND_SEPARATE_HASH(sc,dc,sa,da)					((((sc)*BLEND_HASH_FACTOR+(dc))*BLEND_HASH_FACTOR+(sa))*BLEND_HASH_FACTOR+(da))	/**< Crunch 4x16 bits to 32 bits. */
#define BLEND_HASH(s,d)										BLEND_SEPARATE_HASH(s,d,s,d)	/**< 32 bit hash compatible with BLEND_SEPARATE_HASH(). */
#if OPTIMIZE_STATE_CHANGES
	#define BLEND_FUNC_SEPARATE_WILL_CHANGE(sc,dc,sa,da)	(BLEND_SEPARATE_HASH(sc,dc,sa,da) != gGLGlobalAssets.blitContext->blendHash)	/**< Query whether blend function will change. */
	#define BLEND_FUNC_WILL_CHANGE(s,d)						(BLEND_HASH(s,d)                  != gGLGlobalAssets.blitContext->blendHash)	/**< Query whether blend function will change. */
	#define BLEND_ENABLE_WILL_CHANGE(doBlend)				((doBlend)   ? !gGLGlobalAssets.blitContext->glBlend       : gGLGlobalAssets.blitContext->glBlend)			/**< Query whether blend enable will change. */
	#define SCISSOR_ENABLE_WILL_CHANGE(doScissor)			((doScissor) ? !gGLGlobalAssets.blitContext->glScissorTest : gGLGlobalAssets.blitContext->glScissorTest)	/**< Query whether blend function will change. */
    #define SCISSOR_WILL_CHANGE(x,y,width,height)			((((GLint)(x)        - gGLGlobalAssets.blitContext->scissorX)		| \
    														  ((GLint)(y)        - gGLGlobalAssets.blitContext->scissorY)		| \
    														  ((GLsizei)(width)  - gGLGlobalAssets.blitContext->scissorWidth)	| \
    														  ((GLsizei)(height) - gGLGlobalAssets.blitContext->scissorHeight)) != 0)		/**< Query whether scissor rect will change. */
	#define SHADER_MATRIX_WILL_CHANGE(shader)				(gGLGlobalAssets.matrixSeed != gGLGlobalAssets.blitContext->shader.matrixSeed)	/**< Query whether shader matrix will change. */
	#define TEXTURE_WILL_CHANGE(texName)					((texName) != gGLGlobalAssets.blitContext->lastTexture)							/**< Query whether texture will change. */
#else /* OPTIMIZE_STATE_CHANGES */
	#define BLEND_FUNC_SEPARATE_WILL_CHANGE(sc,dc,sa,da)	1																				/**< Query whether blend function will change. */
	#define BLEND_FUNC_WILL_CHANGE(s,d)						1																				/**< Query whether blend function will change. */
	#define BLEND_ENABLE_WILL_CHANGE(doBlend)				1																				/**< Query whether blend enable will change. */
	#define SCISSOR_ENABLE_WILL_CHANGE(doScissor)			1																				/**< Query whether blend function will change. */
    #define SCISSOR_WILL_CHANGE(x,y,width,height)			1																				/**< Query whether scissor rect will change. */
	#define SHADER_MATRIX_WILL_CHANGE(shader)				1																				/**< Query whether shader matrix will change. */
	#define TEXTURE_WILL_CHANGE(texName)					1																				/**< Query whether texture will change. */
#endif /* OPTIMIZE_STATE_CHANGES */


#if GLES_VERSION < 2
	#define SET_GLOBAL_BLEND_SEPARATE_FUNC(sc,dc,sa,da)		do { (*gGLGlobalAssets.blendFunctionSeparateProc)(sc,dc,sa,da);					\
																gGLGlobalAssets.blitContext->blendHash = BLEND_SEPARATE_HASH(sc,dc,sa,da);	\
																SET_GLOBAL_BLEND_ENABLE(true); } while(0)														/**< Set blend function. */
#else /* GLES_VERSION == 2 */
	#define SET_GLOBAL_BLEND_SEPARATE_FUNC(sc,dc,sa,da)		do { glBlendFuncSeparate(sc,dc,sa,da);											\
																gGLGlobalAssets.blitContext->blendHash = BLEND_SEPARATE_HASH(sc,dc,sa,da);} while(0)			/**< Set blend function. */
#endif /* GLES_VERSION == 2 */
#define SET_GLOBAL_BLEND_FUNC(s,d)							do { glBlendFunc(s,d);															\
																gGLGlobalAssets.blitContext->blendHash = BLEND_HASH(s,d);  } while(0)							/**< Set blend function. */
#define SET_GLOBAL_BLEND_ENABLE(doBlend)					do { if (doBlend)	{  glEnable(GL_BLEND); gGLGlobalAssets.blitContext->glBlend = 1; }	\
																else			{ glDisable(GL_BLEND); gGLGlobalAssets.blitContext->glBlend = 0; }} while(0)	/**< Set blend enable. */
#define SET_GLOBAL_SCISSOR_ENABLE(doScissor)				do { if (doScissor)	{  glEnable(GL_SCISSOR_TEST); gGLGlobalAssets.blitContext->glScissorTest = 1;	} \
																else			{ glDisable(GL_SCISSOR_TEST); gGLGlobalAssets.blitContext->glScissorTest = 0; }} while(0)	/**< Set scissor enable. */
#define SET_GLOBAL_SCISSOR(x,y,width,height)				do {gGLGlobalAssets.blitContext->scissorX = x;	gGLGlobalAssets.blitContext->scissorWidth  = width;		\
																gGLGlobalAssets.blitContext->scissorY = y;	gGLGlobalAssets.blitContext->scissorHeight = height;	\
																glScissor(x,y,width,height);	} while(0)														/**< Set scissor. */
#define SET_SHADER_MATRIX(shader)							do { glUniformMatrix3fv(gGLGlobalAssets.shader.matrix, 1, GL_FALSE, gGLGlobalAssets.matrix[0]); \
																gGLGlobalAssets.blitContext->shader.matrixSeed = gGLGlobalAssets.matrixSeed; } while(0)			/**< Set shader matrix. */
#define SET_TEXTURE(texName)								do { glBindTexture(GL_TEXTURE_2D, (texName)); gGLGlobalAssets.blitContext->lastTexture = (texName); } while(0)	/**< Set texture. */
#define FORGET_TEXTURE(texName)                             do { if(gGLGlobalAssets.blitContext->lastTexture == (texName)) gGLGlobalAssets.blitContext->lastTexture = 0; } while (0)

#define CHANGE_BLEND_FUNC_SEPARATE(sc,dc,sa,da)				do { if (BLEND_FUNC_SEPARATE_WILL_CHANGE(sc,dc,sa,da))	SET_GLOBAL_BLEND_SEPARATE_FUNC(sc,dc,sa,da);	CHANGE_BLEND_ENABLE(true); } while(0)	/**< Set blend function if it would change. */
#define CHANGE_BLEND_FUNC(s,d)								do { if          (BLEND_FUNC_WILL_CHANGE(s,d))			SET_GLOBAL_BLEND_FUNC         (s,d);			CHANGE_BLEND_ENABLE(true); } while(0)	/**< Set blend function if it would change. */
#define CHANGE_BLEND_ENABLE(doBlend)						do { if        (BLEND_ENABLE_WILL_CHANGE(doBlend))		SET_GLOBAL_BLEND_ENABLE       (doBlend);	} while(0)	/**< Set blend enable if it would change. */
#define CHANGE_SCISSOR_ENABLE(doScissor)					do { if      (SCISSOR_ENABLE_WILL_CHANGE(doScissor))	SET_GLOBAL_SCISSOR_ENABLE     (doScissor);	} while(0)	/**< Set scissor enable if it would change. */
#define CHANGE_SHADER_MATRIX(shader)						do { if       (SHADER_MATRIX_WILL_CHANGE(shader))		SET_SHADER_MATRIX             (shader);		} while(0)	/**< Set shader matrix if it would change. */
#define CHANGE_TEXTURE(texName)								do { if             (TEXTURE_WILL_CHANGE(texName))		SET_TEXTURE                   (texName);	} while(0)	/**< Set texture if it would change. */

static void CHANGE_SCISSOR(FskGLPort glPort, SInt32 x, SInt32 y, SInt32 width, SInt32 height);		/* Needs to be defined after gGLGlobalAssets */

typedef GLint GLInternalFormat;		/**< The type used for internal format in glTex*Image2D(). All other GL APIs use GLenum. */
typedef GLenum GLFormat;			/**< The type used for external format in GL. */
typedef GLenum GLType;				/**< The type used for external type in GL. */

#if 0
#pragma mark ======== FskGLPort ========
#endif
/********************************************************************************
 ********************************************************************************
 ********************************************************************************
 *****								FskGLPort								*****
 ********************************************************************************
 ********************************************************************************
 ********************************************************************************/


/****************************************************************************//**
 * Auxiliary data to be stored with each GL texture.
 ********************************************************************************/

typedef struct GLTextureRecord {
	FskRectangleRecord		bounds;								/**< The bounds of the texture. */
	GLInternalFormat		glIntFormat;						/**< The internal GL format used by this texture. */
	GLuint					name;								/**< The handle by which GL refers this texture. */
	GLuint					nameU;								/**< The handle by which GL refers the U component of this texture. */
	GLuint					nameV;								/**< The handle by which GL refers the V component of this texture. */
	GLint					filter;								/**< The filter mode { GL_LINEAR, GL_NEAREST } used for GL_TEXTURE_{MIN,MAX}_FILTER */
	GLint					wrapMode;							/**< The wrap mode { GL_CLAMP_TO_EDGE, GL_REPEAT, GL_MIRRORED_REPEAT } used on both texture coordinates. */
	FskConstBitmap			srcBM;								/**< The bitmap source of the texture. */
	Boolean					flipY;								/**< Indicates that the image in the texture is flipped vertically from the usual. */
	Boolean					fboInited;							/**< Indicates that the texture has been cleared at least once for use as a frame buffer texture. */
	Boolean					persistent;							/**< Save the value when switching textures in FBO. */
	UInt8					viewRotation;						/**< Rotate when rendering (only 0 and 180 degrees). */
} GLTextureRecord,
 *GLTexture;											/**< Object that contains auxiliary data to be stored with each GL texture. */
typedef const struct GLTextureRecord *ConstGLTexture;	/**< Read-only object that contains auxiliary data to be stored with each GL texture. */

#define GL_TEXTURE_UNLOADED ((GLuint)(-1))				/**< name* with this value indicates that the texture should be cached on the GPU but has been unloaded. */


/****************************************************************************//**
 * Information about a GL port.
 ********************************************************************************/

struct FskGLPortRecord {
	/* Properties of an accelerated source. */
	GLTextureRecord		texture;				/**< The texture associated with this bitmap as a source. */

	/* Properties of an accelerated destination. */
	void				*sysContext;			/**< A pointer to a system-specific context */
	UInt32				portWidth;				/**< The width  of the port. */
	UInt32				portHeight;				/**< The height of the port. */

	FskGLPort			next;					/**< Linked list of ports */
};
typedef struct FskGLPortRecord FskGLPortRecord;	/**< GL port record. */
static FskGLPort gCurrentGLPort	= NULL;			/**< The current GL port. */


/** Glyph strike descriptor.
 **/
struct FskGlyphStrikeRecord {	/* 16-20 bytes */
	UInt16	codePoint;	/**< Unicode code point. */
	UInt16	x;			/**< Horizontal location of the glyph. */
	UInt16	y;			/**< Vertical   location of the glyph. */
	UInt16	width;		/**< Bounding width  of the glyph. */
	UInt16	height;		/**< Bounding height of the glyph. */
	UInt32	lastUsed;	/**< Last time used. */
#if USE_GLYPH			/* 2 bytes more */
	float	xoffset;
#endif /* USE_GLYPH */
};
typedef struct FskGlyphStrikeRecord FskGlyphStrikeRecord;		/**< Strike record type. */
typedef struct FskGlyphStrikeRecord *FskGlyphStrike;			/**< Strike pointer type. */
typedef const struct FskGlyphStrikeRecord *FskConstGlyphStrike;	/**< Const strike pointer type. */


/** Typeface encapsulation.
 **	TODO: move to FskText.h.
 **/
struct FskGLTypeFaceRecord {
	FskGLTypeFace	next;					/**< The next typeface. */
	char			*fontName;				/**< The name of the font. */
	UInt32			textStyle;				/**< Style variations of the typeface. */
	UInt32			textSize;				/**< The size of the typeface, in pixels. */
	UInt16			cellWidth;				/**< The width  of each glyph cell. */
	UInt16			cellHeight;				/**< The height of each glyph cell. */
	UInt16			lastStrikeIndex;		/**< The last strike index used, or -1 if virgin. */
	UInt16			lastStaticStrike;		/**< Keep all strikes prior to this. */
	UInt32			numStrikes;				/**< The number of strikes currently allocated. */
	FskGlyphStrike	strikes;				/**< The strike info table. Strike[0] stores a placeholder glyph */
	UInt16			*glyphTab;				/**< The table to translate unicode code points to glyphs. */
	UInt32			glyphTabSize;			/**< The current number of entries in glyphTab. */
	UInt32			lastTextTime;			/**< The last time that text was generated. */
	FskBitmap		bm;						/**< The bitmap that stores the glyphs. */
	struct FskTextEngineRecord		*fte;	/**< Text engine record. */
	struct FskTextFormatCacheRecord	*cache;	/**< Font cache. */
	Boolean			dirty;					/**< If true, the GL texture has not yet been uploaded with the current value of the bitmap. */
#if USE_GLYPH
	UInt16			ellipsisCode;			/**< The glyph code for the ellipsis character. */
#endif /* USE_GLYPH */
	/* char fontNameStorage[length]; cones here */
};


/** Color & blendlevel, state variable. */
typedef struct ColorLevel {
	UInt32	color;	/**< ENCODE_LAST_OP_COLOR(r, g, b, a) */
	UInt32	level;	/**< { blendLevel, blendLevel | PREMULTIPLY_FLAG, FULL_LEVEL_FLAG } */
} ColorLevel;


#define ENCODE_LAST_OP_COLOR(r, g, b, a)	(((r)<<24) | ((g)<<16) | ((b)<<8) | ((a)<<0))	/**< Used to encode ColorLevel.color. */
#define PREMULTIPLY_FLAG	(0x0100)														/**< Indicates that ColorLevel.color has been premultiplied by alpha and blendLevel. */
#define FULL_LEVEL_FLAG		(0x0200)														/**< Indicates that ColorLevel.level is opaque. */
#define ENCODE_LAST_OP_FCOLOR(r, g, b, a)	((UInt32)((r) * 16777216.f + (g) * 65536.f + (b) * 256.f + (a)))	/**< Used to encode ColorLevel.color when supplied as floating-point. */


/****************************************************************************//**
 * ID handles for accessing program members.
 * VERY IMPORTANT: We rely on the { matrix and matrixSeed } of each data structure to be in the same position.
 ********************************************************************************/

/** The IDs used for filling. */
typedef struct IdFill {
	GLuint		program;	/**< program ID */
	GLint		matrix;		/**< matrix  ID */
	GLint		opColor;	/**< opColor ID */
} IdFill;

/** The IDs used for drawing bitmaps. */
typedef struct IdDrawBitmap {
	GLuint		program;	/**< program ID */
	GLint		matrix;		/**< matrix  ID */
	GLint		opColor;	/**< opColor ID */
	GLint		srcBM;		/**< srcBM   ID */
} IdDrawBitmap;
typedef struct IdDrawBitmap	IdTransferAlpha;	/**< The IDs used for transferring an alpha image. */
typedef struct IdDrawBitmap	IdGenericShader;	/**< The IDs used to represent a generic shader. */

/** The IDs used for drawing brightness/contrast adjusted bitmaps. */
typedef struct IdDrawBCBitmap {
	GLuint		program;	/**< program  ID */
	GLint		matrix;		/**< matrix   ID */
	GLint		opColor;	/**< opColor  ID */
	GLint		srcBM;		/**< srcBM    ID */
	GLint		offColor;	/**< offColor ID */
} IdDrawBCBitmap;

/** The IDs used for tiling bitmaps. */
typedef struct IdTileBitmap {
	GLuint		program;	/**< program   ID */
	GLint		matrix;		/**< matrix    ID */
	GLint		opColor;	/**< opColor   ID */
	GLint		srcBM;		/**< srcBM     ID */
	GLint		tileSpace;	/**< tileSpace ID */
} IdTileBitmap;

/** The IDs used for drawing bitmaps in perspective. */
typedef struct IdPerspective {
	GLuint		program;	/**< program ID */
	GLint		matrix;		/**< matrix  ID */
	GLint		opColor;	/**< opColor ID */
	GLint		srcBM;		/**< srcBM   ID */
} IdPerspective;

/** The IDs used for drawing YUV 4:2:0 bitmaps. */
typedef struct IdYUV420 {
	GLuint		program;	/**< program  ID */
	GLint		matrix;		/**< matrix   ID */
	GLint		colorMtx;	/**< colorMtx ID */
	GLint		yTex;		/**< yTex     ID */
	GLint		uTex;		/**< uTex     ID */
	GLint		vTex;		/**< vTex     ID */
} IdYUV420;

/** The IDs used for drawing YUV 4:2:0 bitmaps. */
typedef struct IdYUV420sp {
	GLuint		program;	/**< program  ID */
	GLint		matrix;		/**< matrix   ID */
	GLint		colorMtx;	/**< colorMtx ID */
	GLint		yTex;		/**< yTex     ID */
	GLint		uvTex;		/**< uvTex    ID */
} IdYUV420sp;

/** The IDs used for drawing YUV 4:2:2 bitmaps. */
typedef struct IdYUV422 {
	GLuint		program;	/**< program  ID */
	GLint		matrix;		/**< matrix   ID */
	GLint		colorMtx;	/**< colorMtx ID */
	GLint		srcBM;		/**< srcBM    ID */
	GLint		texDim;		/**< texDim   ID */
} IdYUV422;

/** The IDs used for drawing YUV 4:4:4 bitmaps. */
typedef struct IdYUV444 {
	GLuint		program;	/**< program  ID */
	GLint		matrix;		/**< matrix   ID */
	GLint		colorMtx;	/**< colorMtx ID, assuming YCrCr ordering */
	GLint		srcBM;		/**< srcBM    ID */
} IdYUV444;

/** The IDs used for drawing brightness/contrast adjusted BG3CDP UYVY bitmaps. */
#if defined(EGL_VERSION) && defined(BG3CDP_GL)
typedef struct IdDrawBG3CDPBitmap {
	GLuint		program;	/**< program  ID */
	GLint		matrix;		/**< matrix   ID */
	GLint		opColor;	/**< opColor  ID */
	GLint		srcBM;		/**< srcBM    ID */
	GLint		offColor;	/**< offColor ID */
} IdDrawBG3CDPBitmap;
#endif /* defined(EGL_VERSION) && defined(BG3CDP_GL) */


/****************************************************************************//**
 * State caches for shaders.
 * VERY IMPORTANT: We rely on the { matrix and matrixSeed } of each data structure to be in the same position.
 ********************************************************************************/

typedef struct ShaderState {
	UInt32		matrixSeed;	/**< the sequence number of the last matrix used in this shader */
	ColorLevel	lastParams;	/**< The last color and blend level used in this shader */
} ShaderState;


#if TARGET_CPU_ARM
/** API for Marvell's YUV upload interface.
 * Two pixels formats are accommodated:
 *		kFskBitmapFormatYUV420	FOURCC('I420')
 *		kFskBitmapFormatUYVY	FOURCC('UYVY')
 */
typedef void (GL_APIENTRYP MarvellTexImageVideoProc) (GLenum target, GLsizei width, GLsizei height, GLenum Format, const void *pixels);
#define GL_UYVY_MRVL	0x9100	/**< YUV 4:2:2 UYVY */
#define GL_I420_MRVL	0x9101	/**< YUV 4:2:0 8 bit Y plane followed by 8 bit 2x2 subsampled U and V planes. */
#define GL_YV12_MRVL	0x9102	/**< YUV 4:2:0 8 bit Y plane followed by 8 bit 2x2 subsampled V and U planes. */
#define GL_YUY2_MRVL	0x9103	/**< YUV 4:2:2 YUYV */
#endif /* TARGET_CPU_ARM */

/** Typedef for a function that uploads a particular kind of texture.
 *	\param[in]	srcBM	the source bitmap to be uploaded.
 *	\param[in]	tx		the texture descriptor.
 *	\return		kFskErrNone	if the operation was successful.
 */
typedef FskErr (*FskUploadTextureProc)(FskConstBitmap srcBM, GLTexture tx);

#define FskGLHardwareSupportsPixelFormat(pixelFormat)		(gGLGlobalAssets.srcFormats[0] & (1 << (UInt32)(pixelFormat)))	/**< Query whether the hardware supports the given pixel format. */
#define FskGLShaderSupportsPixelFormat(pixelFormat)			(gGLGlobalAssets.srcFormats[1] & (1 << (UInt32)(pixelFormat)))	/**< Query whether the shader or hardware supports the given pixel format. */
#define FskGLEasyConversionSupportsPixelFormat(pixelFormat)	(gGLGlobalAssets.srcFormats[2] & (1 << (UInt32)(pixelFormat)))	/**< Query whether the given pixel format is easy to accommodate. */
#define FskGLConversionSupportsPixelFormat(pixelFormat)		(gGLGlobalAssets.srcFormats[3] & (1 << (UInt32)(pixelFormat)))	/**< Query whether we support the given pixel format, even if it needs to be converted. */
#define FskGLHardwareSupportsYUVPixelFormat(pixelFormat)	(gGLGlobalAssets.hasImageExternal && ((1 << (UInt32)(pixelFormat)) & gGLGlobalAssets.srcFormats[0] &	\
															((1 << (UInt32)kFskBitmapFormatUYVY) | (1 << (UInt32)kFskBitmapFormatYUV420) | (1 << (UInt32)kFskBitmapFormatYUV420spuv) |		\
															(1 << (UInt32)kFskBitmapFormatYUV420spvu))))	/**< Query whether the hardware supports the given YUV pixel format. */


/****************************************************************************//**
 * Global assets.
 ********************************************************************************/

struct FskGLBlitContextRecord {							/**< State per blit context. */
		FskGLBlitContext	next;						/**< The  next blit context. */

		FskGLContext		glContext;					/**< The GL context. */
		Boolean				glContextMine;				/**< Determines whether the GL context belongs to this blit context. */
		Boolean				unused;						/**< Padding: unused. */

		Boolean				glBlend;					/**< This indicates that    GL_BLEND     has been enabled. */
		Boolean				glScissorTest;				/**< This indicates that GL_SCISSOR_TEST has been enabled. */
		GLint				scissorX;					/**< Left edge of the scissor rectangle. */
		GLint				scissorY;					/**< Top  edge of the scissor rectangle. */
		GLsizei				scissorWidth;				/**< Width     of the scissor rectangle. */
		GLsizei				scissorHeight;				/**< Height    of the scissor rectangle. */
		GLuint				lastTexture;				/**< This is the name of the last texture used in Texture unit 0. */
		UInt32				blendHash;					/**< This is the hash for the curent blend function. */
		GLuint				frameBufferObject;			/**< A frame buffer object to be used for rendering off screen. */
		GLuint				fboTexture;					/**< This is the texture that is currently attached to the frameBufferObject. */
		GLuint				defaultFBO;					/**< A frame buffer object to be used for rendering on screen. */
	#if GLES_VERSION == 2
		GLuint				lastProgram;				/**< The last program used. */
		ShaderState			fill;						/**< State for the fill          program. */
		ShaderState			drawBitmap;					/**< State for the drawBitmap    program. */
		ShaderState			transferAlpha;				/**< State for the transferAlpha program. */
		ShaderState			drawBCBitmap;				/**< State for the drawBCBitmap  program. */
		ShaderState			tileBitmap;					/**< State for the tileBitmap    program. */
		ShaderState			perspective;				/**< State for the perspective   program. */
		ShaderState			yuv420;						/**< State for the yuv420        program. */
		ShaderState			yuv420sp;					/**< State for the yuv420sp      program. */
		ShaderState			yuv422;						/**< State for the yuv422        program. */
		ShaderState			yuv444;						/**< State for the yuv444        program. */
		#if defined(EGL_VERSION) && defined(BG3CDP_GL)
			ShaderState		drawBG3CDPBitmap;			/**< State for the drawBG3CDPBitmap program. */
		#endif /* defined(EGL_VERSION) && defined(BG3CDP_GL) */
	#endif /* GLES_VERSION */

	#ifdef EGL_VERSION
		void				*nativeWindow;				/**< A handle to the native window. */
		EGLDisplay			eglDisplay;					/**< EGL display. */
		EGLSurface			eglSurface;					/**< EGL surface. */
		EGLContext			eglContext;					/**< EGL context. */
	#endif /* EGL_VERSION */
};														/**< State per blit context */


typedef struct FskGLGlobalAssetsRecord {			/* Members set once, at initialization. */
	SInt32			isInited;						/**< The global assets have been inited. */

	/* Capabilities */
	#if GLES_VERSION == 2
		Boolean		hasHighPrecisionShaderFloat;	/**< Indicates that the fragment shader supports high precision computations. */
		Boolean		hasAppleRGB422;					/**< Indicates that the fragment shader supports Apple's APPLE_rgb_422 extension. */
	#endif /* GLES_VERSION == 2 */
	#if defined(EGL_VERSION) && defined(BG3CDP_GL)
		Boolean	hasImageExternal;					/**< Accommodates the OES_EGL_image_external extension */
	#endif /* EGL_VERSION */
	Boolean			allowNearestBitmap;				/**< Indicates whether textures can use GL_NEAREST, or whether it always uses GL_LINEAR. */
	Boolean			hasNPOT;						/**< Indicates whether the GPU can receive non-power-of-two (NPOT) textures. */
	Boolean			hasRepeatNPOT;					/**< Indicates whether the GPU can REPEAT or MIRRORED_REPEAT non-power-of-two (NPOT) textures. */
	Boolean			hasFBO;							/**< Indicates whether the GPU has a reliable frame buffer object implementation. */
	Boolean			rendererIsTiled;				/**< Indicates that the renderer draws one tile completely before moving on to the next, rather than the whole screen at once. */
	Boolean			registeredLowMemoryWarning;		/**< Indicates that we have registered with purgeGL, NULL */
	GLint			maxTextureSize;					/**< The maximum texture size in each dimension. */
	UInt32			srcFormats[4];					/**< Supported formats bitfield, with the first being the fastest. Latter ones include the previous ones. */
	UInt32			readFormats;					/**< Pixels formats supported in hardware for glReadPixels. */

	/* Run-time proc dispatch */
	void (*blendFunctionSeparateProc)(GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha);	/**< blend function separate. */
	void (*platformYUVUploadProc)(void);			/**< Platform-dependent function to be called to aid in uploading YUV textures. */
	FskUploadTextureProc	texImageYUV420;			/**< Function to upload a YUV 4:2:0 texture in planar format [Y] [U] [V]. */
	FskUploadTextureProc	texImageUYVY;			/**< Function to upload a YUV 4:2:2 texture in chunky format UYVY. */

	/* Set and restore GL context */
	FskGLFocusProc	focus;							/**< System-dependent procedure to call before doing any GL drawing. */
	FskGLFocusProc	defocus;						/**< System-dependent procedure to call after completing GL drawing for this frame. */

	/* Caches */
	FskGLPort		activePorts;					/**< A list of active ports. */
	FskGLTypeFace	typeFaces;						/**< The list of cached typefaces. */
	float			*coordinates;					/**< A global coordinate array, used to upload vertices and other varying parameters to the GPU. */
	UInt32			coordinatesBytes;				/**< The number of bytes currently allocated for the coordinates. */
	#if USE_PORT_POOL
		FskGLPort	freePorts;						/**< A list of free ports, with their textures. */
	#endif /* USE_PORT_POOL */
	#if EPHEMERAL_TEXTURE_CACHE_SIZE > 0			/* This FIFO is used for ephemeral objects, to minimize the possibility of updating a live texture */
		GLuint		textureObjectCache[EPHEMERAL_TEXTURE_CACHE_SIZE];	/**< Cache of texture objects. */
		GLuint		nextTextureObjectIndex;			/**< The index of the next available texture object */
	#endif /* EPHEMERAL_TEXTURE_CACHE_SIZE */
	#ifdef TRACK_TEXTURES
		FskGrowableArray	glTextures;
	#endif /* TRACK_TEXTURES */

	#if GLES_VERSION == 2
		/* Shader handles */
		IdFill			fill;						/**< IDs for the program to be used for filling regions with a solid color. */
		IdDrawBitmap	drawBitmap;					/**< IDs for the program to be used for transferring color images. */
		IdTileBitmap	tileBitmap;					/**< IDs for the program to be used for    tiling    color images. */
		IdDrawBCBitmap	drawBCBitmap;				/**< IDs for the program to be used for transferring brightness/contrast adjusted color images. */
		IdTransferAlpha	transferAlpha;				/**< IDs for the program to be used for transferring alpha images. */
		IdYUV420		yuv420;						/**< IDs for the program to be used for transferring YUV 420 images. */
		IdYUV420sp		yuv420sp;					/**< IDs for the program to be used for transferring YUV 420 semi-planar images. */
		IdYUV422		yuv422;						/**< IDs for the program to be used for transferring YUV 422 images. */
		IdYUV444		yuv444;						/**< IDs for the program to be used for transferring YUV 444 images. */
	#if defined(EGL_VERSION) && defined(BG3CDP_GL)
		IdDrawBG3CDPBitmap	drawBG3CDPBitmap;
	#endif /* defined(EGL_VERSION) && defined(BG3CDP) */

		/* View matrix */
		UInt32			matrixSeed;					/**< A sequence number that is incremented when the view matrix is changed. */
		float			matrix[3][3];				/**< The combined model/view/clip matrix. */
	#endif /* GLES_VERSION */

	FskGLBlitContextRecord	defaultContext;			/**< The default FskGLBlit context */
	FskGLBlitContextRecord	*blitContext;			/**< The current FskGLBlit context */

} FskGLGlobalAssetsRecord,							/**< GL Global Assets Record. */
 *FskGLGlobalAssets;								/**< GL Global Assets Object. */


static FskGLGlobalAssetsRecord gGLGlobalAssets = {
	0												/* Everything is initialized to zero */
};

static void purgeGL(void *refcon);


#ifdef TRACK_TEXTURES
#define TRACK_PERIOD 1
static int gTrackCount = 0;
static int CompareTextures(const void *v1, const void *v2) {
	const GLuint *t1 = (const GLuint*)v1;
	const GLuint *t2 = (const GLuint*)v2;
	if (*t1 < *t2)	return -1;
	if (*t1 > *t2)	return +1;
	return 0;
}
static void PrintTrackedTextures(const char *msg) {
	FskGrowableStorage	str		= NULL;
	FskErr				err;
	UInt32				n;
	const GLuint		*txp;

	BAIL_IF_ERR(err = FskGrowableArraySortItems(gGLGlobalAssets.glTextures, CompareTextures));
	BAIL_IF_ERR(err = FskGrowableArrayGetConstPointerToItem(gGLGlobalAssets.glTextures, 0, (const void**)(&txp)));
	n = FskGrowableArrayGetItemCount(gGLGlobalAssets.glTextures);
	(void)FskGrowableStorageNew(32 + n * 4, &str);
	if (!msg) msg = "";
	(void)FskGrowableStorageAppendF(str, "%s TrackedTextures[%u]:", msg, (unsigned)n);
	for (; n--; ++txp)
		(void)FskGrowableStorageAppendF(str, " %u", (unsigned)(*txp));
	LOGD("%s", FskGrowableStorageGetPointerToCString(str));

bail:
	FskGrowableStorageDispose(str);
	return;
}
static void TrackTextures(unsigned n, const GLuint *tx) {
	FskErr err;
	BAIL_IF_ERR(err = FskGrowableArrayAppendItems(gGLGlobalAssets.glTextures, tx, n));
	if (((gTrackCount += n) % TRACK_PERIOD) < n) PrintTrackedTextures(NULL);
bail:
	return;
}
static void TrackTexture(GLuint tx) {
	return TrackTextures(1, &tx);
}
static void UntrackTexture(GLuint tx) {
	FskErr			err;
	UInt32			i, n;
	const GLuint	*txp;

	BAIL_IF_ERR(err = FskGrowableArrayGetConstPointerToItem(gGLGlobalAssets.glTextures, 0, (const void**)(&txp)));
	n = FskGrowableArrayGetItemCount(gGLGlobalAssets.glTextures);
	for (i = 0; i < n; ++i) {
		if (txp[i] == tx) {
			FskGrowableArrayRemoveItem(gGLGlobalAssets.glTextures, i);
			if ((++gTrackCount % TRACK_PERIOD) == 0) PrintTrackedTextures(NULL);
			return;
		}
	}
	LOGD("UntrackTexture(%u) fails", tx);
bail:
	return;
}
static void UntrackTextures(unsigned n, const GLuint *txp) {
	for (; n--; ++txp)
		UntrackTexture(*txp);
}
#endif /* TRACK_TEXTURES */


/****************************************************************************//**
 * Change the GL scissor box and enable scissoring.
 *	\param[in]	glPort	the GL port.
 *	\param[in]	x		the left edge of the clip rectangle.
 *	\param[in]	y		the top  edge of the clip rectangle. We convert this to bottom edge for GL.
 *	\param[in]	width	the width  of the clip rectangle.
 *	\param[in]	height	the height of the clip rectangle.
 ********************************************************************************/

static void CHANGE_SCISSOR(FskGLPort glPort, SInt32 x, SInt32 y, SInt32 width, SInt32 height) {
	if (glPort) {																				/* If there is a port, ... */
		if (width == (SInt32)(glPort->portWidth) && height == (SInt32)(glPort->portHeight) && x == 0 && y == 0) {	/* ... and the clip rect is the port rect, ... */
			CHANGE_SCISSOR_ENABLE(false);														/* ... turn clipping off because it is faster. */
			return;
		}
		if (!glPort->texture.name || glPort->texture.flipY)
			y = glPort->portHeight - y - height;
	}
	if (SCISSOR_WILL_CHANGE(x, y, width, height)) {
		SET_GLOBAL_SCISSOR(x, y, width, height);												/* When we change the scissor box, ... */
		SET_GLOBAL_SCISSOR_ENABLE(true);														/* ... always follow it by a scissor enable, to work properly on Adreno */
	}
	else {
		CHANGE_SCISSOR_ENABLE(true);
	}
}


#if 0
#pragma mark ======== Shaders ========
#endif
/********************************************************************************
 ********************************************************************************
 ********************************************************************************
 *****								Shaders									*****
 ********************************************************************************
 ********************************************************************************
 ********************************************************************************/
#if GLES_VERSION == 2


/**	Vertex shader computes clipping space coordinates for the vertex positions.
 */
#if 0
#pragma mark gFillVertexShader
#endif
static const char gFillVertexShader[] = {
	"uniform mat3 matrix;\n"
	"attribute vec2 vPosition;\n"
	"void main() {\n"
	"	gl_Position.xyw = matrix * vec3(vPosition, 1.);\n"
	"	gl_Position.z = 0.;\n"
	"}\n"
};


/**	Fragment shader sets the pixel to a constant color. */
#if 0
#pragma mark gFillShader
#endif
static const char gFillShader[] = {
	"uniform " LOWP "vec4 opColor;\n"
	"void main() {\n"
	"	gl_FragColor = opColor;\n"
	"}\n"
};


/**	Vertex shader computes clipping space and texture coordinates for each vertex. */
#if 0
#pragma mark gDrawBitmapVertexShader
#endif
static const char gDrawBitmapVertexShader[] = {
	"uniform mat3 matrix;\n"
	"attribute vec2 vPosition;\n"
	"attribute vec2 vTexCoord;\n"
	"varying vec2 srcCoord;\n"
	"void main() {\n"
	"	gl_Position.xyw = matrix * vec3(vPosition, 1.);\n"
	"	gl_Position.z = 0.;\n"
	"	srcCoord = vTexCoord;\n"
	"}\n"
};


/**	Fragment shader computes the texture pixel and modulates it with the opColor. */
#if 0
#pragma mark gDrawBitmapShader
#endif
static const char gDrawBitmapShader[] = {
	"uniform " LOWP "vec4 opColor;\n"
	"uniform " LOWP "sampler2D srcBM;\n"
	"varying " HIGHP "vec2 srcCoord;\n"
	"void main() {\n"
	"	gl_FragColor = texture2D(srcBM, srcCoord) * opColor;\n"
	"}\n"
};


/**	Fragment shader computes the texture pixel, modulates it by the opColor, and adds a constant color.
 *	Typically used for brightness and contrast adjustment, though it is more general.
 */
#if 0
#pragma mark gDrawBCBitmapShader
#endif
static const char gDrawBCBitmapShader[] = {
	"uniform " LOWP "vec4 opColor;\n"
	"uniform " LOWP "vec4 offColor;\n"
	"uniform " LOWP "sampler2D srcBM;\n"
	"varying " HIGHP "vec2 srcCoord;\n"
	"void main() {\n"
	"	gl_FragColor = texture2D(srcBM, srcCoord) * opColor + offColor;\n"
	"}\n"
};


/**	Fragment shader computes a tiled texture pixel and modulates it with the opColor. */
#if 0
#pragma mark gTileBitmapShader
#endif
static const char gTileBitmapShader[] = {
	"uniform " LOWP "vec4 opColor;\n"
	"uniform " LOWP "sampler2D srcBM;\n"
	"uniform " HIGHP "vec4 tileSpace;\n"	/* tileSpace .x=tileLeft, .y=tileTop, .z=tileWidth, .w=tileHeight */
	"varying " HIGHP "vec2 srcCoord;\n"
	"void main() {\n"
	"	gl_FragColor = texture2D(srcBM, fract(srcCoord) * tileSpace.zw + tileSpace.xy) * opColor;\n"
	"}\n"
};


/**	Fragment shader computes the alpha texture value for this pixel and modulates the opColor.
 *	It is typically used for rendering text.
 */
#if 0
#pragma mark gTransferAlphaShader
#endif
static const char gTransferAlphaShader[] = {
	"uniform " LOWP "sampler2D srcBM;\n"
	"uniform " LOWP "vec4 opColor;\n"
	"varying " HIGHP "vec2 srcCoord;\n"
	"void main() {\n"
	"	gl_FragColor = opColor;\n"
	"	gl_FragColor.a *= texture2D(srcBM, srcCoord).a;\n"
	"}\n"
};


/** This shader implements the conversion of planar 4:2:0 YCbCr or actually any tristimulus color space to RGB.
 *	A generic 4x4 transformation is used. This can accommodate, for example:
 *	- an arbitrary bias, not just Y-16, U-128, V-128;
 *	- an arbitrary transformation to RGB;
 *	- an arbitrary RGB color correction;
 *	- brightness and contrast adjustment.
 */
#if 0
#pragma mark gYUV420Shader
#endif
static const char gYUV420Shader[] = {
	"uniform " LOWP    "sampler2D yTex;\n"					/* Stored as alpha */
	"uniform " LOWP    "sampler2D uTex;\n"					/* Stored as alpha */
	"uniform " LOWP    "sampler2D vTex;\n"					/* Stored as alpha */
	"uniform " MEDIUMP "mat4 colorMtx;\n"
	"varying " HIGHP   "vec2 srcCoord;\n"
	"void main() {\n"
	"	"LOWP "vec4 yuv;\n"
	"	yuv[0] = texture2D(yTex, srcCoord).a;\n"
	"	yuv[1] = texture2D(uTex, srcCoord).a;\n"
	"	yuv[2] = texture2D(vTex, srcCoord).a;\n"
	"	yuv[3] = 1.;\n"
	"	gl_FragColor = colorMtx * yuv;\n"
	"}\n"
};


/** This shader implements the conversion of planar 4:2:0sp YCbCr or actually any tristimulus color space to RGB.
 *	A generic 4x4 transformation is used. This can accommodate, for example:
 *	- an arbitrary bias, not just Y-16, U-128, V-128;
 *	- an arbitrary transformation to RGB;
 *	- an arbitrary RGB color correction;
 *	- brightness and contrast adjustment.
 */
#if 0
#pragma mark gYUV420spShader
#endif
static const char gYUV420spShader[] = {
	"uniform " LOWP    "sampler2D yTex;\n"			/* Stored as alpha */
	"uniform " LOWP    "sampler2D uvTex;\n"			/* Stored as luminance-alpha */
	"uniform " MEDIUMP "mat4 colorMtx;\n"
	"varying " HIGHP   "vec2 srcCoord;\n"
	"void main() {\n"
	"	"LOWP "vec4 yuv;\n"
	"	yuv.x  = texture2D(yTex,  srcCoord).a;\n"	/* Y comes from the alpha component of yTex */
	"	yuv.yz = texture2D(uvTex, srcCoord).ga;\n"	/* U comes from either (r, g, or b) component of uvTex; V comes from its alpha component */
	"	yuv.w = 1.;\n"
	"	gl_FragColor = colorMtx * yuv;\n"
	"}\n"
};


/** This shader implements the conversion of chunky 4:2:2 YCbCr or actually any tristimulus color space to RGB.
 *	A generic 4x4 transformation is used. This can accommodate, for example:
 *	- an arbitrary bias, not just Y-16, U-128, V-128;
 *	- an arbitrary transformation to RGB;
 *	- an arbitrary RGB color correction;
 *	- brightness and contrast adjustment.
 */
#if 0	/* We  cannot use this shader because bilinear samplers are not predictable */
#pragma mark gYUV422ShaderA
static const char gYUV422ShaderA[] = {
	"uniform " LOWP    "sampler2D srcBM;\n"
	"uniform " MEDIUMP "mat4 colorMtx;\n"
	"uniform " MEDIUMP "vec2 texDim;\n"				/* Chrominance dimensions */
	"varying " HIGHP   "vec2 srcCoord;\n"			/* Chrominance resolution */
	"void main() {\n"
	"	"LOWP    "vec4 uyvy0;\n"					/* Sample for chrominance */
	"	"LOWP    "vec4 uyvy1;\n"					/* Sample offset 1 luminance pixel from chrominance */
	"	"LOWP    "vec4 yuv;\n"
	"	"MEDIUMP "float fr;\n"
	"	uyvy0 = texture2D(srcBM, srcCoord);\n"
	"	fr = fract(srcCoord.x * texDim.x) * 2.0;\n"	/* [0, 2) */
	"	uyvy1 = texture2D(srcBM, vec2((srcCoord.x + ((0.5 - floor(fr)) / texDim.x)), srcCoord.y));\n"	/* Offset by +/-0.5/texWidth */
	"	fr -= floor(fr) * 2.0;\n"					/* fr -= (fr < 1) ? 0 : 2; */
	"	yuv[3] = 1.;\n"
		#ifndef YUV422_LUMINANCE_FIRST /* i.e. UYVY or VYUY */
	"		yuv[2] = uyvy0[2];\n"
	"		yuv[1] = uyvy0[0];\n"
	"		yuv[0] = ((uyvy0[3] + uyvy1[1] - uyvy0[1] - uyvy1[3]) * fr + uyvy0[3] - uyvy1[3] + sign(fr) * (uyvy1[3] - uyvy1[1])) * fr + uyvy0[1];\n"
		#else /* YUV422_LUMINANCE_FIRST , i.e. YUYV or YVYU */
	"		yuv[2] = uyvy0[3];\n"
	"		yuv[1] = uyvy0[1];\n"
	"		yuv[0] = ((uyvy0[2] + uyvy1[0] - uyvy0[0] - uyvy1[2]) * fr + uyvy0[2] - uyvy1[2]  + sign(fr) * (uyvy1[2] - uyvy1[0])) * fr + uyvy0[0];\n"
		#endif /* YUV422_LUMINANCE_FIRST */
	"	gl_FragColor = colorMtx * yuv;\n"
	"}\n"
};
#endif /* 0 */


/**	Vertex shader computes clipping space and texture coordinates for each vertex. */
#if 0
#pragma mark gDraw422VertexShader
#endif
static const char gDraw422VertexShader[] = {
	"uniform mat3 matrix;\n"
	"uniform " MEDIUMP "vec4 texDim;\n"				/* Chrominance width & height, inverse width and inverse height */
	"attribute vec2 vPosition;\n"
	"attribute vec2 vTexCoord;\n"
	"varying vec4 srcCoord;\n"
	"varying vec2 phase;\n"
	"void main() {\n"
	"	gl_Position.xyw = matrix * vec3(vPosition, 1.);\n"
	"	gl_Position.z = 0.;\n"
	"	srcCoord.xy = vTexCoord;\n"					/* texture coordinate */
	"	srcCoord.zw = vTexCoord + texDim.zw;\n"		/* texture coordinate plus one pixel diagonally */
	"	phase = srcCoord.xy * texDim.xy;\n"			/* phase is computed at high precision */
	"}\n"
};


/** This shader implements the conversion of chunky 4:2:2 YCbCr or actually any tristimulus color space to RGB.
 *	A generic 4x4 transformation is used. This can accommodate, for example:
 *	- an arbitrary bias, not just Y-16, U-128, V-128;
 *	- an arbitrary transformation to RGB;
 *	- an arbitrary RGB color correction;
 *	- brightness and contrast adjustment.
 */
#if 0
#pragma mark gYUV422Shader
#endif
static const char gYUV422Shader[] = {
	"uniform " LOWP    "sampler2D srcBM;\n"
	"uniform " MEDIUMP "mat4 colorMtx;\n"
	"uniform " MEDIUMP "vec4 texDim;\n"							/* Chrominance width & height, inverse width and inverse height */
	"varying " HIGHP   "vec4 srcCoord;\n"						/* Chrominance resolution */
	"varying " HIGHP   "vec2 phase;\n"							/* Pixel phase */
	"void main() {\n"
	"	"MEDIUMP "vec4 p00, p01, p10, p11;\n"
	"	"MEDIUMP "float fr;\n"
	"	p00 = texture2D(srcBM, srcCoord.xy);\n"					/* p(0,0) */
	"	p10 = texture2D(srcBM, srcCoord.zy);\n"					/* p(1,0) */
	"	p01 = texture2D(srcBM, srcCoord.xw);\n"					/* p(0,1) */
	"	p11 = texture2D(srcBM, srcCoord.zw);\n"					/* p(1,1) */
	"	fr = fract(phase.y);\n"									/* fy */
	"	p00 = mix(p00, p01, fr);\n"								/* p(0,fy) */
	"	p10 = mix(p10, p11, fr);\n"								/* p(1,fy) */
	"	fr = fract(phase.x);\n"									/* cx */
	"	p11.gb = mix(p00.rb, p10.rb, fr);\n"					/* uv(cx,fy) */
	"	fr = fr * 2.0;\n"										/* lx */
	"	p01.rg = mix(p00.ga, vec2(p00.a, p10.g), fract(fr));\n"	/* l(lx), l(1+lx) */
	"	p11.r = mix(p01.r, p01.g, floor(fr));\n"
	"	p11.a = 1.0;\n"
	"	gl_FragColor = colorMtx * p11;\n"
	"}\n"
};


/* Shader for APPLE_rgb_422.
 * From http://www.opengl.org/registry/specs/APPLE/rgb_422.txt, we know that
 * UNSIGNED_SHORT_8_8_APPLE:
 *        15  14  13  12  11  10  9   8   7   6   5   4   3   2   1   0
 *      +-------------------------------+-------------------------------+
 *   0  |               Y0              |               Cb              |
 *      +-------------------------------+-------------------------------+
 *   1  |               Y1              |               Cr              |
 *      +-------------------------------+-------------------------------+
 *
 * UNSIGNED_SHORT_8_8_REV_APPLE:
 *        15  14  13  12  11  10  9   8   7   6   5   4   3   2   1   0
 *      +-------------------------------+-------------------------------+
 *   0  |               Cb              |               Y0              |
 *      +-------------------------------+-------------------------------+
 *   1  |               Cr              |               Y1              |
 *      +-------------------------------+-------------------------------+
 * gets mapped into {Cr,Y0,Cb}, {Cr,Y1,Cb},
 * i.e. adjacent RGB pixels actually have luminance in their green channels,
 * and the chrominance samples are duplicated into adjacent pixels,
 * with Cr into red and Cb into blue, with the effect of shifting the chrominance phase 1/2 pixel to the right.
 * UYVY uses UNSIGNED_SHORT_8_8_APPLE on little endian and UNSIGNED_SHORT_8_8_REV_APPLE on big endian machines, and
 * YUYV uses UNSIGNED_SHORT_8_8_REV_APPLE on little endian and UNSIGNED_SHORT_8_8_APPLE on big endian machines.
 * This allows the shader to use either CCIR-601 or CCIR-709 conversions to RGB.
 */


#ifdef GL_RGB_422_APPLE
/**	This shader takes 4:4:4 CrYCb pixels and applies the color transformation matrix to convert to RGB.
 *	The matrix allows the possibility to apply brightness and contrast modification for free.
 */
#if 0
#pragma mark gYUV444Shader
#endif
static const char gYUV444Shader[] = {
	"uniform " MEDIUMP	"mat4 colorMtx;\n"
	"uniform " LOWP		"sampler2D srcBM;\n"
	"varying " HIGHP	"vec2 srcCoord;\n"
	"void main() {\n"
	"	gl_FragColor = colorMtx * texture2D(srcBM, srcCoord).gbra;\n"
	"}\n"
};
#endif /* GL_RGB_422_APPLE */


#if 0
#pragma mark gDrawBG3CDPBitmapShader
#endif
#if defined(BG3CDP_GL)

static const char gDrawBG3CDPBitmapShader[] = {
	"#extension GL_OES_EGL_image_external : require\n"
	"uniform " LOWP "vec4 opColor;\n"
	"uniform " LOWP "vec4 offColor;\n"
	"uniform " LOWP "samplerExternalOES srcBM;\n"
	"varying " HIGHP "vec2 srcCoord;\n"
	"void main() {\n"
	"	gl_FragColor = texture2D(srcBM, srcCoord) * opColor + offColor;\n"
	"}\n"
};
#endif /* defined(BG3CDP_GL) */

#endif /* GLES_VERSION == 2 */


#if 0
#pragma mark ======== Utilities ========
#endif
/********************************************************************************
 ********************************************************************************
 ********************************************************************************
 *****								Utilities								*****
 ********************************************************************************
 ********************************************************************************
 ********************************************************************************/


//#define TEXELS_ARE_POINTS		/**< If defined, texels are considered to be points as opposed to squares. Better matching as squares, except edge crud in perspective. */
//#define PIXELS_ARE_POINTS		/**< If defined, pixels are considered to be points as opposed to squares. */
#if TARGET_OS_MAC || TARGET_OS_WIN32 || TARGET_OS_ANDROID || TARGET_OS_LINUX || TARGET_OS_KPL
	typedef float											GLCoordinateType;
	#define GL_COORD_TYPE_ENUM								GL_FLOAT
	#define NormalizedCoordinate(coord, max)				((float)( coord          ) / (float)(max))
	#define NormalizedCoordinateBiased(coord, max, bias)	((float)((coord) + (bias)) / (float)(max))
	#define UInt8ColorToGLCoord(u)							((u) * (1.f / 255.f))
	#define IntToGLCoord(i)									((float)(i))
	#define FixedToGLCoord(x)								((x) * (1.f / 65536.f))
#elif 0
	typedef GLfixed											GLCoordinateType;
	#define GL_COORD_TYPE_ENUM								GL_FIXED
	#define NormalizedCoordinate(coord, max)				FskFixDiv((FskFixed)(coord),  (FskFixed)(max))
	#define NormalizedCoordinateBiased(coord, max, bias)	FskFixDiv(((FskFixed)(coord) << 16) + (FskFixed)((bias) * 65536), (FskFixed)(max) << 16)
	#define UInt8ColorToGLCoord(u)							(((u) * 0x010101U + 0x80) >> 8)
	#define IntToGLCoord(i)									((i) << 16)
	#define FixedToGLCoord(x)								(x)
#else
	#error unknown OS
#endif /* TARGET */

/** The vertex data structure used for drawing with textures.
 **/
typedef struct GLTexVertex  { GLCoordinateType x, y, u, v; } GLTexVertex;

/** The vertex data structure used for drawing without textures.
 **/
typedef struct GLFillVertex { GLCoordinateType x, y;       } GLFillVertex;

/****************************************************************************//**
 *	Check to see if there have been any Open GL errors, and if so, return an equivalent FskError.
 *	\return	kFskErrNone					if there was no error.
 ********************************************************************************/

static FskErr GetFskGLError() {
	FskErr err = glGetError();
	if (err)
		err = FskErrorFromGLError(err);
	return err;
}


#ifdef EGL_VERSION

/****************************************************************************//**
 * Convert the given EGL error code to a Fsk error code.
 *	\param[in]	eglErr		the EGL error code.
 *	\return		kFskErrNone	if there was no error.
 *							otherwise, returns the equivalent Fsk error code.
 ********************************************************************************/
static FskErr FskErrorFromEGLError(EGLint eglErr) {
	struct ELU { EGLint eglErr; FskErr fskErr; };
	static const struct ELU lookupTab[] = {
		{ EGL_SUCCESS,				kFskErrNone					},	/* Function succeeded. */
		{ EGL_NOT_INITIALIZED,		kFskErrEGLNotInitialized	},	/* EGL is not initialized, or could not be initialized, for the specified display. */
		{ EGL_BAD_ACCESS,			kFskErrEGLBadAccess			},	/* EGL cannot access a requested resource (for example, a context is bound in another thread). */
		{ EGL_BAD_ALLOC,			kFskErrEGLBadAlloc			},	/* EGL failed to allocate resources for the requested operation. */
		{ EGL_BAD_ATTRIBUTE,		kFskErrEGLBadAttribute		},	/* An unrecognized attribute or attribute value was passed in an attribute list. */
		{ EGL_BAD_CONFIG,			kFskErrEGLBadConfig			},	/* An EGLConfig argument does not name a valid EGLConfig.  */
		{ EGL_BAD_CONTEXT,			kFskErrEGLBadContext		},	/* An EGLContext argument does not name a valid EGLContext. */
		{ EGL_BAD_CURRENT_SURFACE,	kFskErrEGLCurrentSurface	},	/* The current surface of the calling thread is a window, pbuffer, or pixmap that is no longer valid. */
		{ EGL_BAD_DISPLAY,			kFskErrEGLBadDisplay		},	/* An EGLDisplay argument does not name a valid EGLDisplay. */
		{ EGL_BAD_MATCH,			kFskErrEGLBadMatch			},	/* Arguments are inconsistent; for example, an otherwise valid context requires buffers (e.g. depth or stencil) not allocated by an otherwise valid surface. */
		{ EGL_BAD_NATIVE_PIXMAP,	kFskErrEGLBadNativePixmap	},	/* An EGLNativePixmapType argument does not refer to a valid native pixmap. */
		{ EGL_BAD_NATIVE_WINDOW,	kFskErrEGLBadNativeWindow	},	/* An EGLNativeWindowType argument does not refer to a valid native window. */
		{ EGL_BAD_PARAMETER,		kFskErrEGLBadParameter		},	/* One or more argument values are invalid. */
		{ EGL_BAD_SURFACE,			kFskErrEGLBadSurface		},	/* An EGLSurface argument does not name a valid surface (window, pbuffer, or pixmap) configured for rendering. */
		{ EGL_CONTEXT_LOST,			kFskErrEGLContextLost		},	/* A power management event has occurred. The application must destroy all contexts and reinitialize client API state and objects to continue rendering. */
		{ -1,						kFskErrUnknown				}
	};
	const struct ELU *p;
	for (p = lookupTab; p != &p[sizeof(lookupTab)/sizeof(lookupTab[0])-1]; ++p)
		if (eglErr == p->eglErr)
			break;
	return p->fskErr;
}


/****************************************************************************//**
 * Check for an EGL error, and convert error codes to Fsk error codes.
 *	\return	kFskErrNone	if there was no error.
 *						otherwise, returns the equivalent Fsk error code.
 ********************************************************************************/
static FskErr GetFskEGLError() {
	return FskErrorFromEGLError(eglGetError());
}
#endif /* TARGET_OS_ANDROID || TARGET_OS_LINUX */


#if 0 && GLES_VERSION == 2
	#define USE_GLCLEAR
#endif /* GLES_VERSION == 2 */
#ifdef USE_GLCLEAR
/****************************************************************************//**
 * Set the GL clear color from an Fsk Color RGBA.
 *	\param[in]	color	the desired clear color.
 ********************************************************************************/

static void glClearColorFskColorRGBA(FskConstColorRGBA color) {
	int i;
	const UInt8 *s = &color->r;
	GLCoordinateType gc[4], *d = gc;
	for (i = 4; i--; )
		*d++ = UInt8ColorToGLCoord(*s++);
	#if GL_COORD_TYPE_ENUM == GL_FLOAT
		glClearColor(gc[0], gc[1], gc[2], gc[3]);
	#else /* GL_COORD_TYPE_ENUM == GL_FIXED */
		glClearColorx(gc[0], gc[1], gc[2], gc[3]);
	#endif /* GL_COORD_TYPE_ENUM == GL_FIXED */
}
#endif /* USE_GLCLEAR */


/****************************************************************************//**
 * SmallestContainingPowerOfTwo:
 * determine the smallest power of two number that is no less than the given number.
 * SmallestContainingPowerOfTwo(0) = 0.
 *	\param[in]	x	the input number.
 *	\return		the smallest power of two equal or greater than the given number.
 ********************************************************************************/

static UInt32 SmallestContainingPowerOfTwo(UInt32 x) {
	--x;				/* One less */
	x |= (x >>  1);		/* Propagate ones to the right */
	x |= (x >>  2);
	x |= (x >>  4);
	x |= (x >>  8);
	x |= (x >> 16);
	++x;				/* Add one to create a pure power of two */
	return x;

}


#if GLES_VERSION == 2


/****************************************************************************//**
 * FskGLUseProgram.
 * We use this rather than glUseProgram() in order to remember the last shader,
 * to avoid loading in a new program.
 *	\param[in]	id	the ID of the program.
 *	\return		kFskErrNone	if there was no error.
 ********************************************************************************/

FskErr FskGLUseProgram(unsigned int id) {
	FskErr err = kFskErrNone;
	if (gGLGlobalAssets.blitContext->lastProgram != id) {
		gGLGlobalAssets.blitContext->lastProgram  = id;
		if (!id)
			err = kFskErrGLProgram;
		else
			glUseProgram(id);
		#if CHECK_GL_ERROR
			if (!err) {
				err = glGetError();
				if (err) {
					LOGE("FskGLUseProgram(%u): GL returns (%d): %s", (unsigned)id, (int)err, GLErrorStringFromCode(err));
					err = FskErrorFromGLError(err);
				}
			}
		#endif /* CHECK_GL_ERROR */
	}
	return err;
}


#ifdef LOG_COLOR
static void LogChangeColor(ColorLevel *lastParams, FskConstColorRGBA opColor, UInt32 newOpColor, UInt32 newLevel) {
	const char			*prog, *change;
	FskColorRGBARecord	white = { 255, 255, 255, 255 };

	if (!opColor)	opColor = &white;
	if      (lastParams == &gGLGlobalAssets.blitContext->fill.lastParams)			prog = "fill";
	else if (lastParams == &gGLGlobalAssets.blitContext->drawBitmap.lastParams)		prog = "drawBitmap";
	else if (lastParams == &gGLGlobalAssets.blitContext->transferAlpha.lastParams)	prog = "transferAlpha";
	else if (lastParams == &gGLGlobalAssets.blitContext->drawBCBitmap.lastParams)	prog = "drawBCBitmap";
	else if (lastParams == &gGLGlobalAssets.blitContext->tileBitmap.lastParams)		prog = "tileBitmap";
	else if (lastParams == &gGLGlobalAssets.blitContext->perspective.lastParams)	prog = "perspective";
	else if (lastParams == &gGLGlobalAssets.blitContext->yuv420.lastParams)			prog = "yuv420";
	else if (lastParams == &gGLGlobalAssets.blitContext->yuv420sp.lastParams)		prog = "yuv420sp";
	else if (lastParams == &gGLGlobalAssets.blitContext->yuv422.lastParams)			prog = "yuv422";
	else if (lastParams == &gGLGlobalAssets.blitContext->yuv444.lastParams)			prog = "yuv444";
	else																			prog = "unknown";
	if ((lastParams->color != newOpColor) || (lastParams->level != newLevel))		change = "changing  ";
	else																			change = "preserving";
	LOGD("%s rgba(%02x%02x%02x%02x) color(%08x) level(%08x) from color(%08x) level(%08x) context(%p) program(%s)",
		change, opColor->r, opColor->g, opColor->b, opColor->a, newOpColor, newLevel, lastParams->color, lastParams->level, gGLGlobalAssets.blitContext, prog);
}
static void LogFColor(const float *color, const char *name);
#endif /* LOG_COLOR */


/****************************************************************************//**
 * FskSetProgramOpColor.
 * This converts the color from UInt8x4 to Float32x4, and scales by the blend level.
 *	\param[in]	id			the id of the program's opColor.
 *	\param[in]	opColor		the desired opColor (NULL yields opaque white).
 *	\param[in]	modeParams	contains the blend level, which is used here only to scale the color.
 ********************************************************************************/


static void FskSetProgramOpColor(GLint id, ColorLevel *lastParams, FskConstColorRGBA opColor, Boolean premultiply, FskConstGraphicsModeParameters modeParams) {
	float fOpColor[4];
	UInt32 lastOpColor, lastLevel;

	/* Compute floating point color */
	if (opColor) {
		lastOpColor = ENCODE_LAST_OP_COLOR(opColor->r, opColor->g, opColor->b, opColor->a);
		fOpColor[0] = UCHAR_TO_FLOAT_COLOR(opColor->r);
		fOpColor[1] = UCHAR_TO_FLOAT_COLOR(opColor->g);
		fOpColor[2] = UCHAR_TO_FLOAT_COLOR(opColor->b);
		fOpColor[3] = UCHAR_TO_FLOAT_COLOR(opColor->a);
	}
	else {
		lastOpColor = ENCODE_LAST_OP_COLOR(255, 255, 255, 255);
		fOpColor[0] = 1.f;
		fOpColor[1] = 1.f;
		fOpColor[2] = 1.f;
		fOpColor[3] = 1.f;
	}

	/* Modulate by the blend level */
	if (modeParams && ((UInt32)(modeParams->blendLevel) < 255)) {		/* Won't pass if blendLevel < 0 || blendLevel >= 255 */
		float blendLevel = UCHAR_TO_FLOAT_COLOR(modeParams->blendLevel);
		lastLevel = modeParams->blendLevel;
		fOpColor[3] *= blendLevel;
		if (premultiply) {
			fOpColor[0] *= blendLevel;
			fOpColor[1] *= blendLevel;
			fOpColor[2] *= blendLevel;
			lastLevel |= PREMULTIPLY_FLAG;
		}
	}
	else {
		lastLevel = FULL_LEVEL_FLAG;
	}

	#ifdef LOG_COLOR
		LogChangeColor(lastParams, opColor, lastOpColor, lastLevel);
	#endif /* LOG_COLOR */
	if (!OPTIMIZE_STATE_CHANGES || (lastParams->color != lastOpColor) || (lastParams->level != lastLevel)) {
		/*id = glGetUniformLocation(progIDs->program, "opColor"); */
		glUniform4fv(id, 1, fOpColor);
		lastParams->color = lastOpColor;
		lastParams->level = lastLevel;
	}
}


/****************************************************************************//**
 * SetBitmapProgramOpColor.
 * This converts the color from UInt8x4 to Float32x4, and scales by the blend level.
 *	\param[in]	id			the id of the program's opColor.
 *	\param[in]	premultiply	if true, indicates that the source is premultiplied.
 *	\param[in]	opColor		the desired opColor (NULL yields opaque white).
 *	\param[in]	modeParams	contains the blend level, which is used here only to scale the color.
 *	\return		the address of the shader state structure, if successful.
 *	\return		NULL, if unsuccessful.
 ********************************************************************************/

static ShaderState* SetBitmapProgramOpColor(FskConstColorRGBA opColor, Boolean premultiply, UInt32 mode, FskConstGraphicsModeParameters modeParams) {
	FskErr			err		= kFskErrNone;
	float			fOpColor[4], fOffColor[4];
	UInt32			lastOpColor, lastLevel;
	ShaderState		*shaderState;

	/* The starting opColor is opaque white, unless the mode is Colorize and a color was supplied */
	if (((mode & kFskGraphicsModeMask) == kFskGraphicsModeColorize) && opColor) {
		lastOpColor = ENCODE_LAST_OP_COLOR(opColor->r, opColor->g, opColor->b, opColor->a);
		fOpColor[0] = UCHAR_TO_FLOAT_COLOR(opColor->r);
		fOpColor[1] = UCHAR_TO_FLOAT_COLOR(opColor->g);
		fOpColor[2] = UCHAR_TO_FLOAT_COLOR(opColor->b);
		fOpColor[3] = UCHAR_TO_FLOAT_COLOR(opColor->a);
	}
	else {
		lastOpColor = ENCODE_LAST_OP_COLOR(255, 255, 255, 255);
		fOpColor[0] = 1.f;
		fOpColor[1] = 1.f;
		fOpColor[2] = 1.f;
		fOpColor[3] = 1.f;
	}

	/* If brightness and contrast was supplied, we need to use a special brightness/contrast program.
	 * We also modulate the opColor by the contrast, and compute an offset color.
	 * Otherwise, we use the basic bitmap program.
	 */
	if (modeParams && (modeParams->dataSize >= sizeof(FskGraphicsModeParametersVideoRecord)) && (((FskGraphicsModeParametersVideo)modeParams)->kind =='cbcb')) {
		float brightness = ((FskGraphicsModeParametersVideo)modeParams)->brightness * (1.f/65536.f);
		float contrast   = ((FskGraphicsModeParametersVideo)modeParams)->contrast   * (1.f/65536.f) + 1.f;
		shaderState = &gGLGlobalAssets.blitContext->drawBCBitmap;									/* Use brightness/contrast program */
		fOffColor[0] =
		fOffColor[1] =
		fOffColor[2] = (1.f - contrast) * .5f + brightness;
		fOffColor[3] = 0.f;
		fOpColor[0] *= contrast;
		fOpColor[1] *= contrast;
		fOpColor[2] *= contrast;
	}
	else {
		shaderState = &gGLGlobalAssets.blitContext->drawBitmap;										/* Use the normal program */
	}

	/* If blending, modulate alpha by the blend factor. If also premultiplied, the color is modulated as well. */
	if (modeParams && ((UInt32)(modeParams->blendLevel) < 255)) {									/* Won't pass if blendLevel < 0 || blendLevel >= 255 */
		float blendLevel = (kFskGraphicsModeCopy == (mode & kFskGraphicsModeMask)) ? 1.f : UCHAR_TO_FLOAT_COLOR(modeParams->blendLevel);
		lastLevel = modeParams->blendLevel;
		fOpColor[3] *= blendLevel;
		if (premultiply) {
			fOpColor[0] *= blendLevel;
			fOpColor[1] *= blendLevel;
			fOpColor[2] *= blendLevel;
			lastLevel |= PREMULTIPLY_FLAG;
		}
	}
	else {
		lastLevel = FULL_LEVEL_FLAG;
	}

	/* Set the computed opColor */
	if (&gGLGlobalAssets.blitContext->drawBCBitmap == shaderState) {
		BAIL_IF_ERR(err = FskGLUseProgram(gGLGlobalAssets.drawBCBitmap.program));					/* Use brightness/contrast program */
		CHANGE_SHADER_MATRIX(drawBCBitmap);
		#ifdef LOG_COLOR
			LogChangeColor(&gGLGlobalAssets.blitContext->drawBCBitmap.lastParams, opColor, lastOpColor, lastLevel);
			LogFColor(fOpColor,  "ShaderGainColor");
			LogFColor(fOffColor, "ShaderBrightnessColor");
		#endif /* LOG_COLOR */
		if (1																			||
			!OPTIMIZE_STATE_CHANGES														||
			(gGLGlobalAssets.blitContext->drawBCBitmap.lastParams.color != lastOpColor)	||
			(gGLGlobalAssets.blitContext->drawBCBitmap.lastParams.level != lastLevel)
		) {
			glUniform4fv(gGLGlobalAssets.drawBCBitmap.opColor,  1,  fOpColor);						/* Set   op   color -- always set this -- ignore state */
			glUniform4fv(gGLGlobalAssets.drawBCBitmap.offColor, 1, fOffColor);						/* Set offset color -- always set this -- ignore state */
			gGLGlobalAssets.blitContext->drawBCBitmap.lastParams.color = lastOpColor;
			gGLGlobalAssets.blitContext->drawBCBitmap.lastParams.level = lastLevel;
			#ifdef LOG_COLOR
				LOGD("Shader Brightness and Contrast Colors have been changed");
			#endif /* LOG_COLOR */
		}
	}
	else {
		BAIL_IF_ERR(err = FskGLUseProgram(gGLGlobalAssets.drawBitmap.program));						/* Use the simple bitmap program */
		#ifdef LOG_COLOR
			LogChangeColor(&gGLGlobalAssets.blitContext->drawBitmap.lastParams, opColor, lastOpColor, lastLevel);
		#endif /* LOG_COLOR */
		CHANGE_SHADER_MATRIX(drawBitmap);
		if (!OPTIMIZE_STATE_CHANGES														||
			(gGLGlobalAssets.blitContext->drawBitmap.lastParams.color != lastOpColor)	||
			(gGLGlobalAssets.blitContext->drawBitmap.lastParams.level != lastLevel)
		) {
			glUniform4fv(gGLGlobalAssets.drawBitmap.opColor, 1, fOpColor);
			gGLGlobalAssets.blitContext->drawBitmap.lastParams.color = lastOpColor;
			gGLGlobalAssets.blitContext->drawBitmap.lastParams.level = lastLevel;
		}
	}

	/* If there were any errors, we use no program */
bail:
	if (err)
		shaderState = NULL;

	return shaderState;
}


/****************************************************************************//**
 * Print Shader Log.
 *	\param[in]	id		the ID of the shader.
 *	\param[in]	type	either GL_VERTEX_SHADER or GL_FRAGMENT_SHADER.
 *	\param[in]	shader	the shader source code.
 ********************************************************************************/

static void PrintShaderLog(GLuint id, GLenum type, const char *shader) {
	char	*errMsg;
	GLsizei	msgLength;
	glGetShaderiv(id, GL_INFO_LOG_LENGTH, &msgLength);
	if (kFskErrNone == FskMemPtrNew(msgLength, &errMsg)) {
		glGetShaderInfoLog(id, msgLength, &msgLength, errMsg);
		fprintf(stderr, "\nShader Log:\n%sfor %s Shader:\n%s\n", errMsg, ((type == GL_VERTEX_SHADER) ? "Vertex" : "Fragment"), shader);

		FskMemPtrDispose(errMsg);
	}
}


/****************************************************************************//**
 * Print Program Log.
 *	\param[in]	id	the id of the program.
 ********************************************************************************/

static void PrintProgramLog(GLuint id) {
	char	*errMsg;
	GLsizei	msgLength;
	glGetProgramiv(id, GL_INFO_LOG_LENGTH, &msgLength);
	if (kFskErrNone == FskMemPtrNew(msgLength+1, &errMsg)) {
		glGetProgramInfoLog(id, msgLength, &msgLength, errMsg);
		errMsg[msgLength] = 0;									/* Make sure that it is terminated */
		fprintf(stderr, "\nProgram Log:\n%s\n", errMsg);
		FskMemPtrDispose(errMsg);
	}
}


//#define DEBUG_ES2
#ifdef DEBUG_ES2
/********************************************************************************
 * PrintProgramUniformsInfo
 ********************************************************************************/

static void PrintProgramUniformsInfo(GLuint id) {
	char	*name = NULL;
	GLint	num, maxLen, index, size, location;
	GLenum	type;

	glGetProgramiv(id, GL_ACTIVE_UNIFORMS,           &num);
	glGetProgramiv(id, GL_ACTIVE_UNIFORM_MAX_LENGTH, &maxLen);
	if (kFskErrNone != FskMemPtrNew(maxLen, &name))
		return;

	fprintf(stderr, "Program %u has %d uniforms:\n", id, num);
	for (index = 0; index < num; ++index) {
		glGetActiveUniform(id, index, maxLen, NULL, &size, &type, name);
		location = glGetUniformLocation(id, name);
		printf("%*s: ", maxLen+4, name);
		switch (type) {
			case GL_FLOAT:			printf("float");		break;
			case GL_FLOAT_VEC2:		printf("float2");		break;
			case GL_FLOAT_VEC3:		printf("float3");		break;
			case GL_FLOAT_VEC4:		printf("float4");		break;
			case GL_INT:			printf("int");			break;
			case GL_INT_VEC2:		printf("int2");			break;
			case GL_INT_VEC3:		printf("int3");			break;
			case GL_INT_VEC4:		printf("int4");			break;
			case GL_BOOL:			printf("bool");			break;
			case GL_BOOL_VEC2:		printf("bool2");		break;
			case GL_BOOL_VEC3:		printf("bool3");		break;
			case GL_BOOL_VEC4:		printf("bool4");		break;
			case GL_FLOAT_MAT2:		printf("matrix2x2");	break;
			case GL_FLOAT_MAT3:		printf("matrix3x3");	break;
			case GL_FLOAT_MAT4:		printf("matrix4x4");	break;
			case GL_SAMPLER_2D:		printf("sampler2D");	break;
			case GL_SAMPLER_CUBE:	printf("samplerCube");	break;
		}
		if (size > 1)
			printf("[%d]", size);
		printf(" @ %d\n", location);
	}
	FskMemPtrDispose(name);
}


/********************************************************************************
 * PrintProgramAttributesInfo
 ********************************************************************************/

static void PrintProgramAttributesInfo(GLuint id) {
	char	*name = NULL;
	GLint	num, maxLen, index, size, location;
	GLenum	type;

	glGetProgramiv(id, GL_ACTIVE_ATTRIBUTES,           &num);
	glGetProgramiv(id, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, &maxLen);
	if (kFskErrNone != FskMemPtrNew(maxLen, &name))
		return;

	fprintf(stderr, "Program %u has %d attributes:\n", id, num);
	for (index = 0; index < num; ++index) {
		glGetActiveAttrib(id, index, maxLen, NULL, &size, &type, name);
		location = glGetAttribLocation(id, name);
		printf("%*s: ", maxLen+4, name);
		switch (type) {
			case GL_FLOAT:			printf("float");		break;
			case GL_FLOAT_VEC2:		printf("float2");		break;
			case GL_FLOAT_VEC3:		printf("float3");		break;
			case GL_FLOAT_VEC4:		printf("float4");		break;
			case GL_FLOAT_MAT2:		printf("matrix2x2");	break;
			case GL_FLOAT_MAT3:		printf("matrix3x3");	break;
			case GL_FLOAT_MAT4:		printf("matrix4x4");	break;
		}
		if (size > 1)
			printf("[%d]", size);
		printf(" @ %d\n", location);
	}
	FskMemPtrDispose(name);
}


/********************************************************************************
 * PrintTransformedVertices
 ********************************************************************************/

static void PrintTransformedVertices(const float *vertices, const FskGLPort glPort, const float *matrix, const float *texCoords) {
	float xVert[4], viewVert[3];
	typedef float Tfloat3[3];
	Tfloat3 *M;
	int i;
	const float *uv = texCoords;

	M = matrix ? (Tfloat3*)matrix : gGLGlobalAssets.matrix;
	for (i = 0; i < 4; ++i, vertices += 2, uv += 2) {
		xVert[0] = M[0][0] * vertices[0] + M[1][0] * vertices[1] + M[2][0];
		xVert[1] = M[0][1] * vertices[0] + M[1][1] * vertices[1] + M[2][1];
		xVert[3] = M[0][2] * vertices[0] + M[1][2] * vertices[1] + M[2][2];
		xVert[2] = 0;
		viewVert[0] = xVert[0] / xVert[3];
		viewVert[1] = xVert[1] / xVert[3];
		viewVert[2] = xVert[2] / xVert[3];
		viewVert[0] = (viewVert[0] + 1.f) * (glPort->portWidth  * 0.5f);
		viewVert[1] = (viewVert[1] + 1.f) * (glPort->portHeight * 0.5f);
		viewVert[2] += 0.5;
		if (!texCoords) {
			printf("%d: (%7.3f, %7.3f) --> (%7.3f, %7.3f, %7.3f, %7.3f) --> (%7.3f, %7.3f, %7.3f)\n",
				i,
				vertices[0], vertices[1],
				xVert[0], xVert[1], xVert[2], xVert[3],
				viewVert[0], viewVert[1], viewVert[2]
			);
		}
		else {
			float uvw[3], euv[2];
			uvw[0] = uv[0] / xVert[3];
			uvw[1] = uv[1] / xVert[3];
			uvw[2] = 1.0f  / xVert[3];
			euv[0] = uvw[0] / uvw[2];
			euv[1] = uvw[1] / uvw[2];
			/*       i      x       y       u      v             x      y      z      w        u      v      q             x      y      z        u      v */
			printf("%d: {(%7.3f, %7.3f), (%5.3f, %5.3f)} --> {(%6.3f, %6.3f, %6.3f, %6.3f), (%5.3f, %5.3f, %5.3f)} --> {(%8.3f, %8.3f, %5.3f), (%5.3f, %5.3f)}\n",
				i,
				vertices[0], vertices[1], uv[0], uv[1],
				xVert[0], xVert[1], xVert[2], xVert[3], uvw[0], uvw[1], uvw[2],
				viewVert[0], viewVert[1], viewVert[2], euv[0], euv[1]
			);
		}
	}
}
#endif /* DEBUG_ES2 */


/****************************************************************************//**
 * FskGLNewShader
 ********************************************************************************/

#undef FskGLNewShader
FskErr FskGLNewShader(const char *shaderStr, GLenum type, GLuint *shaderID) {
	GLuint	id;
	GLint	result;

	*shaderID = 0;
	id = glCreateShader(type);
	glShaderSource(id, 1, &shaderStr, NULL);
	glCompileShader(id);
	glGetShaderiv(id, GL_COMPILE_STATUS, &result);
	if (result) {
		*shaderID = id;
		return kFskErrNone;
	}
	else {
		PrintShaderLog(id, type, shaderStr);
		glDeleteShader(id);
		return kFskErrGLShader;
	}
}


/****************************************************************************//**
 * FskGLNewProgram
 ********************************************************************************/

#undef FskGLNewProgram
FskErr FskGLNewProgram(GLuint vertexShader, GLuint fragmentShader, GLuint *progID, ...) {
	GLint		result, index;
	GLuint		id;
	va_list		ap;
	const char	*name;

	va_start(ap, progID);
	*progID = 0;

	id = glCreateProgram();
	glAttachShader(id, vertexShader);
	glAttachShader(id, fragmentShader);

	while ((index = va_arg(ap, GLint)) >= 0) {
		name = va_arg(ap, const char*);
		glBindAttribLocation(id, index, name);
	}
	va_end(ap);

	glLinkProgram(id);
	glGetProgramiv(id, GL_LINK_STATUS, &result);
	if (result) {
		*progID = id;
		return kFskErrNone;
	}
	else {
		PrintProgramLog(id);
		glDeleteProgram(id);
		return kFskErrGLProgram;
	}
}


#ifdef UNUSED
/****************************************************************************//**
 * Validate Program.
 ********************************************************************************/

static FskErr ValidateProgram(GLuint program) {
	GLint	result;
	glValidateProgram(program);
	glGetProgramiv(program, GL_VALIDATE_STATUS, &result);
	if (result) {
		return kFskErrNone;
	}
	else {
		PrintProgramLog(program);
		return kFskErrGLProgram;
	}
}
#endif /* UNUSED */


#endif /* GLES_VERSION == 2 */


#if defined(GL_TRACE) && TARGET_OS_ANDROID
	void FskGLTrace(const char *func, const char *file, int line) {
		LOGD("Tracing call to %s from %s, line %d", func, file, line);
	}
#endif /* GL_TRACE */

#if GL_DEBUG

/********************************************************************************
 * PRINT_IF_ERROR
 ********************************************************************************/

static void PRINT_IF_ERROR(FskErr err, int line, const char *msg, ...) {
	if (err) {
		if (!msg) {
			#if SUPPORT_INSTRUMENTATION
				LOGE("FskGLBlit.c, line %4d: err = %d: %s", line, (int)err, FskInstrumentationGetErrorString(err));
			#else /* !SUPPORT_INSTRUMENTATION */
			LOGE("FskGLBlit.c, line %4d: err = %d", line, (int)err);
			#endif /* !SUPPORT_INSTRUMENTATION */
		}
		else {
			char	buf[1024];
			va_list	ap;
			va_start(ap, msg);
			vsnprintf(buf, sizeof(buf), msg, ap);
			va_end(ap);
			#if SUPPORT_INSTRUMENTATION
				LOGE("FskGLBlit.c, line %4d: err = %d, @ %s: %s", line, (int)err, buf, FskInstrumentationGetErrorString(err));
			#else /* !SUPPORT_INSTRUMENTATION */
			LOGE("FskGLBlit.c, line %4d: err = %d, @ %s", line, (int)err, buf);
			#endif /* !SUPPORT_INSTRUMENTATION */
		}
	}
}


#if 0
/********************************************************************************
 * PrintTextureIfError
 ********************************************************************************/

static void PrintTextureIfError(FskErr err, int line, const char *msg, Boolean ephemeral, GLuint name) {
	if (err)
		LOGE("FskGLBlit.c, line %4d: err = %d, %s%s, %s texture #%u",
			line, (int)err, (msg ? " @ " : ""), (msg ? msg : ""), (ephemeral ? "ephemeral" : "cached"), name
		);
}
#endif /* 0 */


/********************************************************************************
 * Lookups for debugging.
 ********************************************************************************/

typedef struct LookupEntry {
	int			code;	/* Make sure that 0 is the last code */
	const char	*name;
} LookupEntry;
static const char* LookupCodeToName(const LookupEntry *table, int code) {
	for (; table->code != 0; ++table)
		if (table->code == code)
			break;
	return table->name;
}
#ifdef LOG_SET_TEXTURE
static const char* ConversionFormatNameFromCode(UInt32 format) {
	static const LookupEntry lookupTab[] = {
		{	0xFFFFFFFFU,					""								},
		{	kFskBitmapFormat32BGRA,			"32BGRA"						},
		{	kFskBitmapFormat32RGBA,			"32RGBA"						},
		{	kFskBitmapFormat24RGB,			"24RGB"							},
		{	0,								"UNKNOWN"						}
	};
	return LookupCodeToName(lookupTab, format);
}
#endif /* LOG_SET_TEXTURE */
static const char* GLFormatNameFromCode(GLFormat format) {
	static const LookupEntry lookupTab[] = {
		{	GL_RGBA,						"RGBA"							},
		{	GL_RGB,							"RGB"							},
		{	GL_ALPHA,						"ALPHA"							},
		{	GL_LUMINANCE,					"LUMINANCE"						},
	#ifdef    GL_BGRA
		{	  GL_BGRA,						"BGRA"							},
	#endif /* GL_BGRA */
	#ifdef    GL_YCBCR_422_APPLE
		{	  GL_YCBCR_422_APPLE,			"YCBCR_422_APPLE"				},
	#endif /* GL_YCBCR_422_APPLE */
	#ifdef	  GL_RGB_422_APPLE
		{	  GL_RGB_422_APPLE,				"RGB_422_APPLE"					},
	#endif /* GL_RGB_422_APPLE */
		{	0,								"UNKNOWN"						}
	};
	return LookupCodeToName(lookupTab, format);
}
static const char* GLTypeNameFromCode(GLType code) {
	static const LookupEntry lookupTab[] = {
		{	GL_UNSIGNED_BYTE,				"UNSIGNED_BYTE"					},
		{	GL_UNSIGNED_SHORT_5_6_5,		"UNSIGNED_SHORT_5_6_5"			},
		{	GL_UNSIGNED_SHORT_4_4_4_4,		"UNSIGNED_SHORT_4_4_4_4"		},
	#ifdef    GL_UNSIGNED_INT_8_8_8_8
		{	  GL_UNSIGNED_INT_8_8_8_8,		"UNSIGNED_INT_8_8_8_8"			},
	#endif /* GL_UNSIGNED_INT_8_8_8_8 */
	#ifdef    GL_UNSIGNED_INT_8_8_8_8_REV
		{	  GL_UNSIGNED_INT_8_8_8_8_REV,	"UNSIGNED_INT_8_8_8_8_REV"		},
	#endif /* GL_UNSIGNED_INT_8_8_8_8_REV */
	#ifdef    GL_UNSIGNED_SHORT_5_6_5_REV
		{     GL_UNSIGNED_SHORT_5_6_5_REV,	"UNSIGNED_SHORT_5_6_5_REV"		},
	#endif /* GL_UNSIGNED_SHORT_5_6_5_REV */
	#ifdef    GL_UNSIGNED_SHORT_4_4_4_4_REV
		{     GL_UNSIGNED_SHORT_4_4_4_4_REV,"UNSIGNED_SHORT_4_4_4_4_REV"	},
	#endif /* GL_UNSIGNED_SHORT_4_4_4_4_REV */
		{	0,								"UNKNOWN"						}
	};
	return LookupCodeToName(lookupTab, code);
}
static const char* GLInternalFormatNameFromCode(GLInternalFormat format) {
	static const LookupEntry lookupTab[] = {
		{	GL_RGBA,						"RGBA"							},
		{	GL_RGB,							"RGB"							},
		{	GL_ALPHA,						"ALPHA"							},
		{	GL_LUMINANCE,					"LUMINANCE"						},
	#ifdef    GL_BGRA
		{     GL_BGRA,						"BGRA"							},
	#endif /* GL_BGRA */
	#ifdef	  GL_RGB_422_APPLE
		{	  GL_RGB_422_APPLE,				"RGB_422_APPLE"					},
	#endif /* GL_RGB_422_APPLE */
		{	0,								"UNKNOWN"						}
	};
	return LookupCodeToName(lookupTab, format);
}
static double FloatTextSize(UInt32 textSize) { return (textSize < 65536U) ? (double)textSize : textSize * (1./65536.); }
#if GLES_VERSION == 2
static const char* GLQualityNameFromCode(int quality) {
	static const LookupEntry lookupTab[] = {
		{	GL_NEAREST,						"NEAREST"						},
		{	GL_LINEAR,						"LINEAR"						},
		{	0,								"UNKNOWN"						}
	};
	return LookupCodeToName(lookupTab, quality);
}
static const char* GLFrameBufferStatusStringFromCode(int status) {
	static const LookupEntry lookupTab[] = {
		#if   defined(GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT)
			{	GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT,				"GL FRAMEBUFFER: INCOMPLETE ATTACHMENT"			},
		#elif defined(GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT)
			{	GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT,			"GL FRAMEBUFFER: INCOMPLETE ATTACHMENT"			},
		#endif     /* GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT */
		#if   defined(GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT)
			{	GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT,		"GL FRAMEBUFFER: INCOMPLETE MISSING ATTACHMENT"	},
		#elif defined(GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT)
			{	GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT,	"GL FRAMEBUFFER: INCOMPLETE MISSING ATTACHMENT"	},
		#endif     /* GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT */
		#if   defined(GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS)
			{	GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS,				"GL FRAMEBUFFER: INCOMPLETE DIMENSIONS"			},
		#elif defined(GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT)
			{	GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT,			"GL FRAMEBUFFER: INCOMPLETE DIMENSIONS"			},
		#endif     /* GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS */
		#if   defined(GL_FRAMEBUFFER_INCOMPLETE_FORMATS)
			{	GL_FRAMEBUFFER_INCOMPLETE_FORMATS,					"GL FRAMEBUFFER: INCOMPLETE FORMATS"			},
		#elif defined(GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT)
			{	GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT,				"GL FRAMEBUFFER: INCOMPLETE FORMATS"			},
		#endif     /* GL_FRAMEBUFFER_INCOMPLETE_FORMATS */
		#if   defined(GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT)
			{	GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT,			"GL FRAMEBUFFER: INCOMPLETE DRAW BUFFER"		},
		#endif     /* GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT */
		#if   defined(GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT)
			{	GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT,			"GL FRAMEBUFFER: INCOMPLETE READ BUFFER"		},
		#endif     /* GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT */
		#if   defined(GL_FRAMEBUFFER_UNSUPPORTED)
			{	GL_FRAMEBUFFER_UNSUPPORTED,							"GL FRAMEBUFFER: UNSUPPORTED"					},
		#elif defined(GL_FRAMEBUFFER_UNSUPPORTED_EXT)
			{	GL_FRAMEBUFFER_UNSUPPORTED,							"GL FRAMEBUFFER: UNSUPPORTED"					},
		#endif     /* GL_FRAMEBUFFER_UNSUPPORTED */
		#if   defined(GL_INVALID_FRAMEBUFFER_OPERATION)
			{	GL_INVALID_FRAMEBUFFER_OPERATION,					"GL FRAMEBUFFER: INVALID OPERATION"				},
		#elif defined(GL_INVALID_FRAMEBUFFER_OPERATION_EXT)
			{	GL_INVALID_FRAMEBUFFER_OPERATION_EXT,				"GL FRAMEBUFFER: INVALID OPERATION"				},
		#endif     /* GL_INVALID_FRAMEBUFFER_OPERATION */
		#if defined(GL_FRAMEBUFFER_COMPLETE)
			{	GL_FRAMEBUFFER_COMPLETE,							"GL FRAMEBUFFER: COMPLETE"						},
		#endif   /* GL_FRAMEBUFFER_COMPLETE */
			{	0,													"GL FRAMEBUFFER: NOT ACCELERATED"				}
	};
	return LookupCodeToName(lookupTab, status);
}
#endif /* GLES_VERSION == 2 */
static const char* GLErrorStringFromCode(int err) {
	static const LookupEntry lookupTab[] = {
		{ GL_INVALID_ENUM,									"An unacceptable value is specified for an enumerated argument."														},
		{ GL_INVALID_VALUE,									"A numeric argument is out of range. "																					},
		{ GL_INVALID_OPERATION,								"The specified operation is not allowed in the current state. "															},
		{ GL_OUT_OF_MEMORY,									"There is not enough memory left to execute the command. The state of the GL is undefined."								},
	#ifdef GL_INVALID_FRAMEBUFFER_OPERATION
		{ GL_INVALID_FRAMEBUFFER_OPERATION,					"The command is trying to render to or read from the framebuffer while the currently bound framebuffer "
															"is not framebuffer complete (i.e. the return value from glCheckFramebufferStatus is not GL_FRAMEBUFFER_COMPLETE)."		},
	#endif /* GL_INVALID_FRAMEBUFFER_OPERATION */
	#ifdef GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT
		{  GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT,			"Not all framebuffer attachment points are framebuffer attachment complete. "
															"This means that at least one attachment point with a renderbuffer or texture attached has its attached object "
															"no longer in existence or has an attached image with a width or height of zero, "
															"or the color attachment point has a non-color-renderable image attached, "
															"or the depth attachment point has a non-depth-renderable image attached, "
															"or the stencil attachment point has a non-stencil-renderable image attached."											},
	#endif     /* GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT */
	#ifdef GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT
		{  GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT,	"No images are attached to the framebuffer."																			},
	#endif     /* GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT */
	#ifdef GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS
		{  GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS,			"Not all framebuffer attached images have the same width and height."													},
	#endif     /* GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS */
	#ifdef GL_FRAMEBUFFER_INCOMPLETE_FORMATS
		{  GL_FRAMEBUFFER_INCOMPLETE_FORMATS,				"GL framebuffer has incomplete formats"																					},
	#endif     /* GL_FRAMEBUFFER_INCOMPLETE_FORMATS */
	#ifdef GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT
		{  GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT,		"GL framebuffer has an incomplete draw buffer"																			},
	#endif     /* GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT */
	#ifdef GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT
		{  GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT,		"GL framebuffer has an incomplete read buffer"																			},
	#endif     /* GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT */
	#ifdef GL_FRAMEBUFFER_UNSUPPORTED
		{  GL_FRAMEBUFFER_UNSUPPORTED,						"The combination of internal formats of the attached images violates an implementation-dependent set of restrictions."	},
	#endif     /* GL_FRAMEBUFFER_UNSUPPORTED */
	#ifdef GL_INVALID_FRAMEBUFFER_OPERATION
		{  GL_INVALID_FRAMEBUFFER_OPERATION,				"GL framebuffer executing an invalid operation"																			},
	#endif     /* GL_INVALID_FRAMEBUFFER_OPERATION */
	#ifdef GL_STACK_OVERFLOW
		{  GL_STACK_OVERFLOW,								"Command would cause a stack overflow."																					},
	#endif /* GL_STACK_OVERFLOW */
	#ifdef GL_STACK_UNDERFLOW
		{  GL_STACK_UNDERFLOW,								"Command would cause a stack underflow."																				},
	#endif /* GL_STACK_UNDERFLOW */
	#ifdef GL_FRAMEBUFFER_COMPLETE
		{  GL_FRAMEBUFFER_COMPLETE,							"No error: the framebuffer is complete."																				},
	#endif /* GL_FRAMEBUFFER_COMPLETE */
		{ GL_NO_ERROR,										NULL																													}
	};
	return LookupCodeToName(lookupTab, err);
}


static const char* ModeNameFromCode(UInt32 mode) {
	static const LookupEntry lookupTab[] = {
		{	kFskGraphicsModeBilinear|kFskGraphicsModeCopy,		"BilinearCopy"		},
		{	kFskGraphicsModeBilinear|kFskGraphicsModeAlpha,		"BilinearAlpha"		},
		{	kFskGraphicsModeBilinear|kFskGraphicsModeColorize,	"BilinearColorize"	},
		{	kFskGraphicsModeCopy,								"Copy"				},
		{	kFskGraphicsModeAlpha,								"Alpha"				},
		{	kFskGraphicsModeColorize,							"Colorize"			},
		{	0,													"UNKNOWN"			}
	};
	return LookupCodeToName(lookupTab, mode);
}

static void LogSrcBitmap(FskConstBitmap srcBM, const char *name) {
	if (!srcBM)
		return;
	if (!name)
		name = "SRCBM";
	if (srcBM->glPort) {
		if (0 == (srcBM->glPort->texture.nameU | srcBM->glPort->texture.nameV))	/* Neither a U nor a V texture */
			LOGD("\t%s: bounds(%d, %d, %d, %d), depth=%u, format=%s, rowBytes=%d, bits=%p, alpha=%d, premul=%d, texture(#%u, w=%d, h=%d, %s, backRef=%s), useCount=%d",
				name, (int)srcBM->bounds.x, (int)srcBM->bounds.y, (int)srcBM->bounds.width, (int)srcBM->bounds.height, (unsigned)srcBM->depth,
				FskBitmapFormatName(srcBM->pixelFormat), (int)srcBM->rowBytes, srcBM->bits, srcBM->hasAlpha, srcBM->alphaIsPremultiplied,
				srcBM->glPort->texture.name, (int)srcBM->glPort->texture.bounds.width, (int)srcBM->glPort->texture.bounds.height,
				GLInternalFormatNameFromCode(srcBM->glPort->texture.glIntFormat),
				srcBM->glPort->texture.srcBM == NULL ? "NULL" : srcBM->glPort->texture.srcBM == srcBM ? "self" : "another", srcBM->useCount);
		else if (0 == srcBM->glPort->texture.nameV)								/* No V texture, but there us a UV teture */
			LOGD("\t%s: bounds(%d, %d, %d, %d), depth=%u, format=%s, rowBytes=%d, bits=%p, alpha=%d, premul=%d, texture(Y=#%u, UV=#%u, w=%d, h=%d, %s, backRef=%s, useCount=%d)",
				name, (int)srcBM->bounds.x, (int)srcBM->bounds.y, (int)srcBM->bounds.width, (int)srcBM->bounds.height, (unsigned)srcBM->depth,
				FskBitmapFormatName(srcBM->pixelFormat), srcBM->rowBytes, srcBM->bits, srcBM->hasAlpha, srcBM->alphaIsPremultiplied,
				srcBM->glPort->texture.name, srcBM->glPort->texture.nameU,
				(int)srcBM->glPort->texture.bounds.width, (int)srcBM->glPort->texture.bounds.height,
				GLInternalFormatNameFromCode(srcBM->glPort->texture.glIntFormat),
				srcBM->glPort->texture.srcBM == NULL ? "NULL" : srcBM->glPort->texture.srcBM == srcBM ? "self" : "another", srcBM->useCount);
		else																	/* Both a U and a V texture */
			LOGD("\t%s: bounds(%d, %d, %d, %d), depth=%u, format=%s, rowBytes=%d, bits=%p, alpha=%d, premul=%d, texture(Y=#%u, U=#%u, V=#%u, w=%d, h=%d, %s, backRef=%s, useCount=%d)",
				name, (int)srcBM->bounds.x, (int)srcBM->bounds.y, (int)srcBM->bounds.width, (int)srcBM->bounds.height, (unsigned)srcBM->depth,
				FskBitmapFormatName(srcBM->pixelFormat), srcBM->rowBytes, srcBM->bits, srcBM->hasAlpha, srcBM->alphaIsPremultiplied,
				srcBM->glPort->texture.name, srcBM->glPort->texture.nameU, srcBM->glPort->texture.nameV,
				(int)srcBM->glPort->texture.bounds.width, (int)srcBM->glPort->texture.bounds.height,
				GLInternalFormatNameFromCode(srcBM->glPort->texture.glIntFormat),
				srcBM->glPort->texture.srcBM == NULL ? "NULL" : srcBM->glPort->texture.srcBM == srcBM ? "self" : "another", srcBM->useCount);
	}
	else {
		LOGD("\t%s: bounds(%d, %d, %d, %d), depth=%u, format=%s, rowBytes=%d, bits=%p, alpha=%d, premul=%d, useCount=%d",
			name, (int)srcBM->bounds.x, (int)srcBM->bounds.y, (int)srcBM->bounds.width, (int)srcBM->bounds.height, (unsigned)srcBM->depth,
			FskBitmapFormatName(srcBM->pixelFormat), (int)srcBM->rowBytes, srcBM->bits, srcBM->hasAlpha, srcBM->alphaIsPremultiplied, srcBM->useCount);
	}
}

static void LogRect(FskConstRectangle r, const char *name) {
	if (!r)
		return;
	if (!name)
		name = "RECT";
	LOGD("\t%s(%d, %d, %d, %d)", name, (int)r->x, (int)r->y, (int)r->width, (int)r->height);
}

static void LogDstBitmap(FskConstBitmap dstBM, const char *name) {
	if (!dstBM)
		return;
	if (!name)
		name = "DSTBM";
	if (dstBM->glPort && dstBM->glPort->portWidth && dstBM->glPort->portHeight) {
		if (dstBM->glPort->texture.name)
			LOGD("\t%s: bounds(%d, %d, %d, %d), name, depth=%u, format=%s, rowBytes=%d, bits=%p, alpha=%d, premul=%d, glDstPort(#%u, %u, %u), useCount=%d",
				name, (int)dstBM->bounds.x, (int)dstBM->bounds.y, (int)dstBM->bounds.width, (int)dstBM->bounds.height, (unsigned)dstBM->depth,
				FskBitmapFormatName(dstBM->pixelFormat), (int)dstBM->rowBytes, dstBM->bits, dstBM->hasAlpha, dstBM->alphaIsPremultiplied,
				dstBM->glPort->texture.name, (int)dstBM->glPort->portWidth, (int)dstBM->glPort->portHeight, dstBM->useCount);
		else
			LOGD("\t%s: bounds(%d, %d, %d, %d), name, depth=%u, format=%s, rowBytes=%d, bits=%p, alpha=%d, premul=%d, glDstPort(%u, %u), useCount=%d",
				name, (int)dstBM->bounds.x, (int)dstBM->bounds.y, (int)dstBM->bounds.width, (int)dstBM->bounds.height, (unsigned)dstBM->depth,
				FskBitmapFormatName(dstBM->pixelFormat), (int)dstBM->rowBytes, dstBM->bits, dstBM->hasAlpha, dstBM->alphaIsPremultiplied,
				(int)dstBM->glPort->portWidth, (int)dstBM->glPort->portHeight, dstBM->useCount);
	}
	else
		LOGD("\t%s: bounds(%d, %d, %d, %d), name, depth=%u, format=%s, rowBytes=%d, bits=%p, alpha=%d, premul=%d, useCount=%d",
			name, (int)dstBM->bounds.x, (int)dstBM->bounds.y, (int)dstBM->bounds.width, (int)dstBM->bounds.height, (unsigned)dstBM->depth,
			FskBitmapFormatName(dstBM->pixelFormat), (int)dstBM->rowBytes, dstBM->bits, dstBM->hasAlpha, dstBM->alphaIsPremultiplied, dstBM->useCount);
}

static void LogColor(FskConstColorRGBA color, const char *name) {
	if (!color)
		return;
	if (!name)
		name = "COLOR";
	LOGD("\t%s(%d, %d, %d, %d)", name, color->r, color->g, color->b, color->a);
}

static void LogFColor(const float *color, const char *name) {
	if (!color)
		return;
	if (!name)
		name = "COLOR";
	LOGD("%s(%5.3f, %5.3f, %5.3f, %5.3f)", name, color[0], color[1], color[2], color[3]);
}

static void LogModeParams(FskConstGraphicsModeParameters modeParams) {
	if (!modeParams)
		return;
	if (modeParams->dataSize <= sizeof(FskGraphicsModeParametersRecord)) {
		LOGD("\tmodeParams: dataSize=%u, blendLevel=%d", (unsigned)modeParams->dataSize, (int)modeParams->blendLevel);
	} else {
		FskGraphicsModeParametersVideo videoParams = (FskGraphicsModeParametersVideo)modeParams;
		LOGD("\tmodeParams: dataSize=%u, blendLevel=%d, kind='%4s, contrast=%f, brightness=%f, sprites=%p",
			(unsigned)videoParams->header.dataSize, (int)videoParams->header.blendLevel, (char*)(&videoParams->kind),
			videoParams->contrast * (1.f/65536.f) + 1.f, videoParams->brightness * (1.f/65536.f), videoParams->sprites);
	}
}

static void LogGLPort(FskConstGLPort glPort) {
	if (!glPort)
		return;
	if (glPort->portWidth != 0 &&glPort->portHeight) {	/* Dst */
		LOGD("\tdstPort: width=%u, height=%u, sysContext=%p", (unsigned)glPort->portWidth, (unsigned)glPort->portHeight, glPort->sysContext);
	}
	else {												/* Src */
		LOGD("\tsrcPort: width = %u, height = %u, glIntFormat=%s, name=%d, srcBM=%p",
			(unsigned)glPort->texture.bounds.width, (unsigned)glPort->texture.bounds.height, GLInternalFormatNameFromCode(glPort->texture.glIntFormat),
			glPort->texture.name, glPort->texture.srcBM);
	}
}

static void LogMatrix(const float M[3][3], const char *name) {
	if (!name)
		name = "MATRIX";
	LOGD("\t%s: { [%.3g %.3g %.3g] [%.3g %.3g %.3g] [%.3g %.3g %.3g] }",
		name,
		M[0][0], M[0][1], M[0][2],
		M[1][0], M[1][1], M[1][2],
		M[2][0], M[2][1], M[2][2]
	);
}

static void LogTextFormatCache(FskTextFormatCache cache, const char *name) {
	#if defined(SUPPORT_TEXT_FORMAT_CACHE_DEBUG) && SUPPORT_TEXT_FORMAT_CACHE_DEBUG
		if (LOGD_ENABLED() && cache) {
			char	*infoStr	= NULL;
			#if     TARGET_OS_IPHONE
			#elif   TARGET_OS_MAC || TARGET_OS_MACOSX
				void FskCocoaTextFormatCacheInfoStringGet(FskTextFormatCache cache, char **pInfoStr);
				FskCocoaTextFormatCacheInfoStringGet(cache, &infoStr);
			#elif   TARGET_OS_WIN32
			#elif   TARGET_OS_ANDROID
			#elif   TARGET_OS_LINUX
			#elif   TARGET_OS_KPL
			#else /*TARGET_OS_UNKNOWN*/
			#endif/*TARGET_OS*/
			if (infoStr) {
				if (!name) name = "cache";
				LOGD("\t%s: %s", name, infoStr);
				FskMemPtrDispose(infoStr);
			}
		}
	#else /* SUPPORT_TEXT_FORMAT_CACHE_DEBUG */
		if (cache || name){}
	#endif /* SUPPORT_TEXT_FORMAT_CACHE_DEBUG */
}

static void LogGLRectangleFill(FskConstBitmap dstBM, FskConstRectangle r, FskConstColorRGBA color, UInt32 mode, FskConstGraphicsModeParameters modeParams) {
	LOGD("GLRectangleFill(dstBM=%p, r=%p, color=%p, mode=%s, modeParams=%p)", dstBM, r, color, ModeNameFromCode(mode), modeParams);
	LogDstBitmap(dstBM, "dstBM");
	LogRect(r, "r");
	LogColor(color, "color");
	LogModeParams(modeParams);
}

static void LogGLBitmapDraw(
	FskConstBitmap					srcBM,
	FskConstRectangle				srcRect,	/* Map this rect... */
	FskConstBitmap					dstBM,
	FskConstRectangle				dstRect,	/* ...to this rect */
	FskConstRectangle				dstClip,	/* But clip thuswise */
	FskConstColorRGBA				opColor,
	UInt32							mode,
	FskConstGraphicsModeParameters	modeParams	/* We get blend level and tint color from here */
) {
	LOGD("GLBitmapDraw(srcBM=%p, srcRect=%p, dstBM=%p, dstRect=%p, dstClip=%p, opColor=%p, mode=%s, modeParams=%p)",
		srcBM, srcRect, dstBM, dstRect, dstClip, opColor, ModeNameFromCode(mode), modeParams);
	LogSrcBitmap(srcBM, "srcBM");
	LogRect(srcRect, "srcRect");
	LogDstBitmap(dstBM, "dstBM");
	LogRect(dstRect, "dstRect");
	LogRect(dstClip, "dstClip");
	LogColor(opColor, "opColor");
	LogModeParams(modeParams);
}

static void LogScaleOffset(const FskScaleOffset *scaleOffset) {
	if (scaleOffset)
		LOGD("\tscale(%.5f, %.5f), offset(%.5f, %.5f)",
			scaleOffset->scaleX  * (1.f/((float)(1 << kFskScaleBits))),
			scaleOffset->scaleY  * (1.f/((float)(1 << kFskScaleBits))),
			scaleOffset->offsetX * (1.f/((float)(1 << kFskOffsetBits))),
			scaleOffset->offsetY * (1.f/((float)(1 << kFskOffsetBits))));
}

static void LogGLScaleOffsetBitmap(
	FskConstBitmap					srcBM,
	FskConstRectangle				srcRect,	/* Map this rect... */
	FskConstBitmap					dstBM,
	FskConstRectangle				dstClip,	/* But clip thuswise */
	const FskScaleOffset			*scaleOffset,
	FskConstColorRGBA				opColor,
	UInt32							mode,
	FskConstGraphicsModeParameters	modeParams	/* We get blend level and tint color from here */
) {
	LOGD("GLScaleOffsetBitmap(srcBM=%p, srcRect=%p, dstBM=%p, dstClip=%p, scaleOffset=%p, opColor=%p, mode=%s, modeParams=%p)",
		srcBM, srcRect, dstBM, dstClip, scaleOffset, opColor, ModeNameFromCode(mode), modeParams);
	LogSrcBitmap(srcBM, "srcBM");
	LogRect(srcRect, "srcRect");
	LogDstBitmap(dstBM, "dstBM");
	LogRect(dstClip, "dstClip");
	LogScaleOffset(scaleOffset);
	LogColor(opColor, "opColor");
	LogModeParams(modeParams);
}

static void LogGLTileBitmap(
	FskConstBitmap					srcBM,				/* Source bitmap */
	FskConstRectangle				srcRect,			/* Bounds of source bitmaps to be used */
	FskConstBitmap					dstBM,				/* Destination bitmap */
	FskConstRectangle				dstRect,			/* Bounds to tile in destination*/
	FskConstRectangle				dstClip,			/* Clip of destination, incorporating source clip */
	FskFixed						scale,				/* source tile scale */
	FskConstColorRGBA				opColor,			/* Operation color used if needed for the given transfer mode */
	UInt32							mode,				/* Transfer mode, incorporating quality */
	FskConstGraphicsModeParameters	modeParams			/* We get blend level and tint color from here */
) {
	LOGD("GLTileBitmap(srcBM=%p, srcRect=%p, dstBM=%p, dstRect=%p, dstClip=%p, scale=%.6g opColor=%p, mode=%s, modeParams=%p)",
		srcBM, srcRect, dstBM, dstRect, dstClip, scale * (1./65536.), opColor, ModeNameFromCode(mode), modeParams);
	LogSrcBitmap(srcBM, "srcBM");
	LogRect(srcRect, "srcRect");
	LogDstBitmap(dstBM, "dstBM");
	LogRect(dstRect, "dstRect");
	LogRect(dstClip, "dstClip");
	LogColor(opColor, "opColor");
	LogModeParams(modeParams);
}

static void LogGLTransferAlphaBitmap(
	FskConstBitmap					srcBM,
	FskConstRectangle				srcRect,	/* Map this rect... */
	FskConstBitmap					dstBM,
	FskConstPoint					dstLoc,		/* ...to this location */
	FskConstRectangle				dstClip,	/* But clip thuswise */
	FskConstColorRGBA				fgColor,
	FskConstGraphicsModeParameters	modeParams	/* We get blend level and tint color from here */
) {
	LOGD("GLTransferAlphaBitmap(srcBM=%p, srcRect=%p, dstBM=%p, dstLoc=%p, dstClip=%p, fgColor=%p, modeParams=%p)",
		srcBM, srcRect, dstBM, dstLoc, dstClip, fgColor, modeParams);
	LogSrcBitmap(srcBM, "srcBM");
	LogRect(srcRect, "srcRect");
	LogDstBitmap(dstBM, "dstBM");
	if (dstLoc) LOGD("\tdstLoc(%d, %d)", (int)dstLoc->x, (int)dstLoc->y);
	LogRect(dstClip, "dstClip");
	LogColor(fgColor, "fgColor");
	LogModeParams(modeParams);
}

static void LogGLTextBox(
	struct FskTextEngineRecord		*fte,
	FskConstBitmap					dstBM,
	const char						*text,
	UInt32							textLen,
	FskConstRectangle				dstRect,
	FskConstRectangle				dstClip,
	FskConstColorRGBA				color,
	UInt32							blendLevel,
	UInt32							textSize,
	UInt32							textStyle,
	UInt16							hAlign,
	UInt16							vAlign,
	const char						*fontName,
	struct FskTextFormatCacheRecord	*cache
) {
	LOGD("GLTextBox(fte=%p, dstBM=%p, text=\"%.*s\", textLen=%u, dstRect=%p, dstClip=%p, color=%p, blendLevel=%u, textSize=%g, textStyle=$%03X, hAlign=%u, vAlign=%u, fontName=\"%s\", cache=%p)",
		fte, dstBM, textLen, text, (unsigned)textLen, dstRect, dstClip, color, (unsigned)blendLevel, FloatTextSize(textSize), (unsigned)textStyle, (unsigned)hAlign, (unsigned)vAlign, fontName, cache);
	LogDstBitmap(dstBM, "dstBM");
	LogRect(dstRect, "dstRect");
	LogRect(dstClip, "dstClip");
	LogColor(color, "color");
	LogTextFormatCache(cache, "textFormatCache");
}

static void LogPortPixelsRead(FskConstGLPort glPort, Boolean backBuffer, FskConstRectangle srcRect, FskBitmap dstBM) {
	LOGD("GLPortPixelsRead(glPort=%p, backbuffer=%s, srcRect=%p, dstBM=%p)", glPort, backBuffer ? "true" : "false", srcRect, dstBM);
	LogGLPort(glPort);
	LogRect(srcRect, "srcRect");
	LogSrcBitmap(dstBM, "dstBM");
}

static void LogProjectBitmap(
	FskConstBitmap					srcBM,
	FskConstRectangle				srcRect,
	FskConstBitmap					dstBM,
	FskConstRectangle				dstClip,
	const float						M[3][3],
	FskConstColorRGBA				opColor,
	UInt32							mode,
	FskConstGraphicsModeParameters	modeParams
) {
	LOGD("GLProjectBitmap(srcBM=%p, srcRect=%p, dstBM=%p, dstClip=%p, M=%p, opColor=%p, mode=%s, modeParams=%p)",
		srcBM, srcRect, dstBM, dstClip, M, opColor, ModeNameFromCode(mode), modeParams);
	LogSrcBitmap(srcBM, "srcBM");
	LogRect(srcRect, "srcRect");
	LogDstBitmap(dstBM, "dstBM");
	LogRect(dstClip, "dstClip");
	LogMatrix(M, "M");
	LogColor(opColor, "opColor");
	LogModeParams(modeParams);
}

#if USE_GLYPH
static void LogTextGetGlyphs(const char *text, UInt32 textLen, UInt32 textSize, UInt32 textStyle, const char *fontName, const UInt16 *glyphs, UInt32 glyphsLen) {
	FskErr	err		= kFskErrNone;
	char	*str	= NULL;
	UInt16	*uniStr	= NULL;
	UInt32	i, uniCount;

	if (!LOGD_ENABLED())
		return;

	BAIL_IF_NULL(text, err, kFskErrInvalidParameter);
	BAIL_IF_NULL(fontName, err, kFskErrInvalidParameter);
	BAIL_IF_NULL(glyphs, err, kFskErrInvalidParameter);
	BAIL_IF_ERR(err = FskTextUTF8ToUnicode16NE((const UInt8*)text, textLen, &uniStr, &uniCount));
	BAIL_IF_NULL(uniStr, err, kFskErrMemFull);
	uniCount /= 2;	/* Convert from byte count to codepoint count */
	i = (uniCount > glyphsLen) ? uniCount : glyphsLen;
	BAIL_IF_ERR(err = FskMemPtrNew(i * 5 * sizeof(*str) + 1, &str));
	BAIL_IF_NULL(str, err, kFskErrMemFull);
	LOGD("TextGetGlyphs(text=%p textLen=%u textSize=%g textStyle=$%03X fontName=\"%s\" glyphs=%p glyphsLen=%u)", text, textLen, FloatTextSize(textSize), textStyle, fontName, glyphs, glyphsLen);

	for (i = 0; i < uniCount; ++i)
		sprintf(str + 5 * i, "%04.4X ", uniStr[i]);
	str[5 * uniCount - 1] = 0;
	LOGD("\ttext  ={%s} \"%.*s\"", str, textLen, text);
	for (i = 0; i < glyphsLen; ++i)
		sprintf(str + 5 * i, "%04.4X ", glyphs[i]);
	str[5 * glyphsLen - 1] = 0;
	LOGD("\tglyphs={%s}", str);
bail:
	FskMemPtrDispose(str);
	FskMemPtrDispose(uniStr);
}
#endif /* USE_GLYPH */

#ifdef LOG_TEXTURE_LIFE
static unsigned CountAllocatedTextures(void) {
	unsigned count = 0;
	FskGLPort p;
	for (p = gGLGlobalAssets.activePorts; p != NULL; p = p->next) {
		if (p->texture.name  && GL_TEXTURE_UNLOADED != p->texture.name)		++count;
		if (p->texture.nameU && GL_TEXTURE_UNLOADED != p->texture.nameU)	++count;
		if (p->texture.nameV && GL_TEXTURE_UNLOADED != p->texture.nameV)	++count;
	}
	return count;
}
#endif /* LOG_TEXTURE_LIFE */

#else /* !GL_DEBUG */

#define PRINT_IF_ERROR(err, line, ...)	do {} while(0)	/**< Macro to do nothing if GL_DEBUG is false. */

#endif /* GL_DEBUG */


#ifdef LOG_MEMORY
	#if defined(_WIN32)
		#include <Windows.h>
		#include <psapi.h>
	#elif defined(__unix__) || defined(__unix) || defined(unix) || (defined(__APPLE__) && defined(__MACH__))
		#include <unistd.h>
		#include <sys/param.h>
		#include <sys/resource.h>
		#include <sys/types.h>
		#if defined(BSD)
			#include <sys/sysctl.h>
		#endif /* BSD */
		#if defined(__APPLE__) && defined(__MACH__)
			#include <mach/mach.h>
		#elif (defined(_AIX) || defined(__TOS__AIX__)) || (defined(__sun__) || defined(__sun) || defined(sun) && (defined(__SVR4) || defined(__svr4__)))
			#include <fcntl.h>
			#include <procfs.h>
		#elif defined(__linux__) || defined(__linux) || defined(linux) || defined(__gnu_linux__)
			#include <stdio.h>
		#endif
	#else /* Unknown OS */
		#error "Unable to define getMemorySize(), getPeakRSS() or getCurrentRSS() for an unknown OS."
	#endif /* OS */


/****************************************************************************//**
 * Print a 64-bit number, with commas, to a string.
 *	\param[in]	x	the number to be printed.
 *	\param[out]	buf	a buffer to hold the string result.
 *	\return		the buffer, buf.
 ********************************************************************************/

static const char* CommaPrintS64(FskInt64 x, char buf[32]) {
	char *s0, *s1;
	int i;

	s0 = buf + (i = sprintf(buf, "%lld", x));	/* i is the string length > 0; s1 is set to the null terminator. */
	s1 = s0 + (i = (i - 1) / 3);				/* i is the number of commas that need to be inserted; s1 is set to the new null terminator. */
	*s1-- = *s0--;								/* Copy the terminating null character */
	while (i--) {								/* Copy from the end, until all commas are inserted */
		*s1-- = *s0--;
		*s1-- = *s0--;
		*s1-- = *s0--;
		*s1-- = ',';							/* Insert a comma every 3 digits */
	}
	return buf;
}


/****************************************************************************//**
 * Get the peak resident set size (i.e. peak memory usage).
 *	\return		the maximum number of bytes used by this process.
 ********************************************************************************/

static FskInt64 GetPeakRSS(void) {
	#if defined(_WIN32)																							/* Windows -------------------------------------------------- */
		PROCESS_MEMORY_COUNTERS info;
		GetProcessMemoryInfo(GetCurrentProcess(), &info, sizeof(info));
		return (FskInt64)info.PeakWorkingSetSize;
	#elif (defined(_AIX) || defined(__TOS__AIX__)) || (defined(__sun__) || defined(__sun) || defined(sun) && (defined(__SVR4) || defined(__svr4__)))	/* AIX and Solaris ------------------------------------------ */
		struct psinfo psinfo;
		int fd = -1;
		if ((fd = open("/proc/self/psinfo", O_RDONLY)) == -1)
			return 0LL;														/* Can't open? */
		if (read(fd, &psinfo, sizeof(psinfo)) != sizeof(psinfo)) {
			close(fd);
			return 0LL;														/* Can't read? */
		}
		close(fd);
		return psinfo.pr_rssize * 1024LL;
	#elif defined(__unix__) || defined(__unix) || defined(unix) || (defined(__APPLE__) && defined(__MACH__))	/* BSD, Linux, and OSX -------------------------------------- */
		struct rusage rusage;
		getrusage(RUSAGE_SELF, &rusage);
		#if defined(__APPLE__) && defined(__MACH__)
			return (FskInt64)rusage.ru_maxrss;
		#else
			return rusage.ru_maxrss * 1024LL;
		#endif
	#else																										/* Unknown OS ----------------------------------------------- */
		return 0LL;															/* Unsupported. */
	#endif
}


/****************************************************************************//**
 * Get the current resident set size (i.e. current memory usage).
 *	\return		the current number of bytes used by this process.
 ********************************************************************************/

static FskInt64 GetCurrentRSS(void) {
	#if defined(_WIN32)																				/* Windows -------------------------------------------------- */
		PROCESS_MEMORY_COUNTERS info;
		GetProcessMemoryInfo(GetCurrentProcess(), &info, sizeof(info));
		return (FskInt64)info.WorkingSetSize;
	#elif defined(__APPLE__) && defined(__MACH__)													/* OSX ------------------------------------------------------ */
		struct mach_task_basic_info info;
		mach_msg_type_number_t infoCount = MACH_TASK_BASIC_INFO_COUNT;
		if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO, (task_info_t)&info, &infoCount) != KERN_SUCCESS)
			return 0LL;													/* Can't access? */
		return (FskInt64)info.resident_size;
	#elif defined(__linux__) || defined(__linux) || defined(linux) || defined(__gnu_linux__)		/* Linux ---------------------------------------------------- */
		long rss = 0L;
		FILE* fp = NULL;
		if ((fp = fopen("/proc/self/statm", "r")) == NULL)
			return (FskInt64)0L;										/* Can't open? */
		if (fscanf(fp, "%*s%ld", &rss) != 1) {
			fclose(fp);
			return 0LL;													/* Can't read? */
		}
		fclose(fp);
		return (FskInt64)rss * (FskInt64)sysconf(_SC_PAGESIZE);
	#else																							/* AIX, BSD, Solaris, and Unknown OS ------------------------ */
		return 0LL;														/* Unsupported. */
	#endif
}

#endif /* LOG_MEMORY */


/****************************************************************************//**
 * Initialize textures to a happy state.
 *	\param[in]		n	the number of textures to be initialized.
 *	\param[in,out]	the textures to be initialized.
 *	\return			kFskErrNone				if successful.
 *	\return			kFskErrGraphicsContext	if an illegal texture was submitted.
 ********************************************************************************/

static FskErr InitializeTextures(GLsizei n, GLuint *textures) {
	FskErr err = kFskErrNone;

	#ifdef LOG_TEXTURE_LIFE
		LOGD("InitializeTextures(%d): %u textures allocated", n, CountAllocatedTextures());
		#if 0
		if (LOGD_ENABLED()) {										/* Print out all textures allocated */
			FskGLPort p;
			for (p = gGLGlobalAssets.activePorts; p != NULL; p = p->next) {
				if (p->texture.name && GL_TEXTURE_UNLOADED  != p->texture.name) {
					GLuint nameU = (p->texture.nameU && GL_TEXTURE_UNLOADED != p->texture.nameU) ? p->texture.nameU : 0;
					GLuint nameV = (p->texture.nameV && GL_TEXTURE_UNLOADED != p->texture.nameV) ? p->texture.nameV : 0;
					FskBitmapFormatEnum bmf = p->texture.srcBM ? p->texture.srcBM->pixelFormat : kFskBitmapFormatUnknown;
					LOGD("\t{#%u #%u #%u} %d x %d %s (<--%s)", p->texture.name, nameU, nameV, (int)(p->texture.bounds.width),
						(int)(p->texture.bounds.height), GLInternalFormatNameFromCode(p->texture.glIntFormat), FskBitmapFormatName(bmf)
					);
				}
			}
		}
		#endif
	#endif /* LOG_TEXTURE_LIFE */

	for (; n--; ++textures) {
		if (0 == *textures || GL_TEXTURE_UNLOADED == *textures) {		/* Do not try to initialize non-textures */
			err = kFskErrGraphicsContext;
			continue;
		}
		CHANGE_TEXTURE(*textures);
		#ifdef GL_UNPACK_SKIP_ROWS
			glPixelStorei(GL_UNPACK_SKIP_ROWS,   0);
		#endif /* GL_UNPACK_SKIP_ROWS */
		#ifdef GL_UNPACK_SKIP_PIXELS
			glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
		#endif /* GL_UNPACK_SKIP_PIXELS */
		#ifdef GL_UNPACK_ROW_LENGTH
			glPixelStorei(GL_UNPACK_ROW_LENGTH,  0);
		#endif /* GL_UNPACK_ROW_LENGTH */
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		#if GLES_VERSION == 1
			glEnableClientState(GL_VERTEX_ARRAY);
			glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		#endif /* GLES_VERSION == 1 */

		/*
			GL_TEXTURE_MIN_FILTER defaults to GL_NEAREST_MIPMAP_LINEAR.
			Changing that to GL_LINEAR during the render
			loop can be very expensive. Changing between GL_LINEAR and GL_NEAREST
			during the rendering loops appears to be inexpensive, at least
			when GL_LINEAR was active when the texture was uploaded.

			GL_TEXTURE_MAG_FILTER defaults to GL_LINEAR already.
		*/
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
//		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		/*
			GL_TEXTURE_WRAP_S and GL_TEXTURE_WRAP_T default to GL_REPEAT
		*/
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}
	return err;
}


/****************************************************************************//**
 * Generate a GL texture and initialize to a known state.
 ********************************************************************************/

static FskErr FskGenGLTexturesAndInit(GLsizei numTextures, GLuint *textures) {
	GLsizei i = numTextures;													/* The number of textures that we need to allocate */
	FskErr err;
	FskMemSet(textures, 0xFF, numTextures * sizeof(*textures));					/* Initialize all textures to GL_TEXTURE_UNLOADED, in case allocation fails */

	/* Allocate textures, first from the pool, then from GL */
	if (i > 0) {																/* If we need more, but have exhausted the pool ... */
		glGenTextures(i, textures);												/* ... allocate a few more from GL */
		#ifdef TRACK_TEXTURES
			TrackTextures(i, textures);
		#endif /* TRACK_TEXTURES */

	#if CHECK_GL_ERROR
		err = glGetError();
		if (err) {
			LOGD("glGenTextures(%u) err = %#04x", i, err);
			for (; i--; ++textures)
				if (0 == *textures)
					*textures = GL_TEXTURE_UNLOADED;
			err = FskErrorFromGLError(err);
			goto bail;
		}
	#endif /* CHECK_GL_ERROR */
	}
	textures -= (numTextures - i);												/* Reset to the original texture array pointer */

	err = InitializeTextures(numTextures, textures);							/* Set textures to a known state */
	#if GL_DEBUG																/* GL Better not give us a texture equal to 0xFFFFFFFF */
		for (i = numTextures; i--; ++textures) {
			if (GL_TEXTURE_UNLOADED == *textures)
				LOGE("GL supplied a texture GL_TEXTURE_UNLOADED = %u", *textures);
			else if (0 == *textures)
				LOGE("GL supplied a texture 0");
		}
	#endif /* GL_DEBUG */
	#if CHECK_GL_ERROR
		if (!err)	err = GetFskGLError();	/* If we didn't generate an error, see if GL did */
		else		(void)GetFskGLError();	/* If we did generate an error, return that and toss any GL errors */
bail:
	#endif /* CHECK_GL_ERROR */
	PRINT_IF_ERROR(err, __LINE__, "FskGenGLTexturesAndInit");
	return err;
}


/****************************************************************************//**
 * Dispose the Global GL Assets.
 ********************************************************************************/

static void FskDeleteGLTextures(GLsizei numTextures, GLuint *textures) {
	for (; numTextures; --numTextures, ++textures) {									/* For all of the given textures, ... */
		FskGLBlitContext ctx;
		if (0 != *textures && GL_TEXTURE_UNLOADED != *textures) {						/* ... that is, all that are really textures, ... */
			for (ctx = &gGLGlobalAssets.defaultContext; ctx != NULL; ctx = ctx->next)	/* ... look through each context, ... */
				if (ctx->lastTexture == *textures)										/* ... and if its last texture is this texture ... */
					ctx->lastTexture = 0;												/* ... obliterate it from the context's memory. */
			#ifdef TRACK_TEXTURES
				UntrackTexture(*textures);
			#endif /* TRACK_TEXTURES */
			glDeleteTextures(1, textures);												/* Also actually delete the texture, ... */
			*textures = 0;																/* ... and nullify its reference. */
		}
	}
}


/****************************************************************************//**
 * GLPortListDeletePort - delete a port from a linked list.
 *	\param[in,out]	list	The list to be modified.
 *	\param[in]		port	The port to be deleted from the list.
 *	\return			kFskErrNone		if the port was found and deleted from the list;
 *	\return			kFskErrNotFound	if the port was not found in the list.
 ********************************************************************************/

static FskErr GLPortListDeletePort(FskGLPort *list, FskGLPort port) {
	for ( ; *list != NULL; list = &((**list).next)) {
		if (*list == port) {
			*list = port->next;
			port->next = NULL;
			return kFskErrNone;
		}
	}
	return kFskErrNotFound;
}


#ifdef UNUSED
/****************************************************************************//**
 * GLPortListGetLastNext - Get the last next field in a linked list.
 *	\param[in,out]	list	The list to be traversed.
 *	\return			a pointer to the last next field.
 ********************************************************************************/

static FskGLPort* GLPortListGetLastNext(FskGLPort *list) {
	while (*list != NULL)			/* Advance to the last next field, which is NULL */
		list = &((**list).next);
	return list;
}


/****************************************************************************//**
 * GLPortListAppendPort - append a port to the end of a linked list.
 *	\param[in,out]	list	The list to be modified.
 *	\param[in]		port	The port to be appended to the list.
 *	\return			kFskErrNone		if successful.
 ********************************************************************************/

static void GLPortListAppendPort(FskGLPort *list, FskGLPort port) {
	port->next = NULL;						/* Terminate this port, to make it a one element list */
	*GLPortListGetLastNext(list) = port;
}
#endif /* UNUSED */


/****************************************************************************//**
 * GLPortListInsertPort - insert a port into the beginning of a linked list.
 *	\param[in,out]	list	The list to be modified.
 *	\param[in]		port	The port to be inserted into the list.
 *	\return			kFskErrNone		if successful.
 ********************************************************************************/

static void GLPortListInsertPort(FskGLPort *list, FskGLPort port) {
	port->next = *list;
	*list = port;
}


/****************************************************************************//**
 * Remove the given texture object from all contexts' lastTexture state.
 *	\param[in]	texID	the ID of the texture object.
 ********************************************************************************/

static void ForgetGivenTextureObjectInAllContexts(GLuint texID) {
	FskGLBlitContext ctx;

	for (ctx = &gGLGlobalAssets.defaultContext; ctx; ctx = ctx->next) {
		if (ctx->lastTexture && (ctx->lastTexture == texID))
			ctx->lastTexture = 0;
	}
}


/****************************************************************************//**
 * Remove all of the texture objects in the given texture from all contexts' states.
 *	\param[in]	tx the texture to be forgotten.
 ********************************************************************************/

static void ForgetGivenTexturesInAllContexts(GLTexture tx) {
	FskGLBlitContext ctx;

	for (ctx = &gGLGlobalAssets.defaultContext; ctx; ctx = ctx->next) {
		if (ctx->lastTexture &&(ctx->lastTexture == tx->name	||
								ctx->lastTexture == tx->nameU	||
								ctx->lastTexture == tx->nameV)
		)	ctx->lastTexture = 0;
	}

	if	(gGLGlobalAssets.blitContext) {																/* There is no blit context at shutdown */
		for (ctx = &gGLGlobalAssets.defaultContext; ctx; ctx = ctx->next) {
			if (ctx->fboTexture && (ctx->fboTexture == tx->name		||
									ctx->fboTexture == tx->nameU	||
									ctx->fboTexture == tx->nameV)
			)	ctx->fboTexture = GL_TEXTURE_UNLOADED;
		}
	}
}


/****************************************************************************//**
 * GLPortReallyDeletePortTextures
 *	\param[in,out]	port		the port whose textures are to be deleted.
 *	\return			kFskErrNone	if successful.
 ********************************************************************************/

static FskErr GLPortReallyDeletePortTextures(FskGLPort port) {
	if (port == NULL)
		return kFskErrNone;

	#ifdef LOG_TEXTURE_LIFE
		LOGD("GLPortReallyDeletePortTextures: disposing %ux%u %s texture {#%u,#%u,#%u}", (unsigned)port->texture.bounds.width, (unsigned)port->texture.bounds.height,
			GLInternalFormatNameFromCode(port->texture.glIntFormat), port->texture.name, port->texture.nameU, port->texture.nameV);
	#endif /* LOG_TEXTURE_LIFE */

	ForgetGivenTexturesInAllContexts(&port->texture);

	if (port->texture.name  && GL_TEXTURE_UNLOADED != port->texture.name) {
		#ifdef TRACK_TEXTURES
			UntrackTexture(port->texture.name);
		#endif /* TRACK_TEXTURES */
		glDeleteTextures(1, &port->texture.name);
	}
	if (port->texture.nameU && GL_TEXTURE_UNLOADED != port->texture.nameU) {
		#ifdef TRACK_TEXTURES
			UntrackTexture(port->texture.nameU);
		#endif /* TRACK_TEXTURES */
		glDeleteTextures(1, &port->texture.nameU);
	}
	if (port->texture.nameV && GL_TEXTURE_UNLOADED != port->texture.nameV) {
		#ifdef TRACK_TEXTURES
			UntrackTexture(port->texture.nameV);
		#endif /* TRACK_TEXTURES */
		glDeleteTextures(1, &port->texture.nameV);
	}
	port->texture.name  = 0;
	port->texture.nameU = 0;
	port->texture.nameV = 0;
	#if CHECK_GL_ERROR
		return GetFskGLError();
	#else /*! CHECK_GL_ERROR */
		return kFskErrNone;
	#endif /* CHECK_GL_ERROR */
}

/********************************************************************************
 * GLPortReallyDispose
 ********************************************************************************/

static FskErr GLPortReallyDispose(FskGLPort port) {
	if (port == NULL)
		return kFskErrNone;
	(void)GLPortReallyDeletePortTextures(port);
	(void)GLPortListDeletePort(&(gGLGlobalAssets.activePorts), port);
	if (port == gCurrentGLPort)
		gCurrentGLPort = NULL;
	return FskMemPtrDispose(port);
}


#if GLES_VERSION == 2
/********************************************************************************
 * DisposeProgram
 ********************************************************************************/

static void DisposeProgram(GLuint *program) {
	if (0 == *program)
		return;
	glDeleteProgram(*program);
	*program = 0;
}
#endif /* GLES_VERSION == 2 */


/****************************************************************************//**
 * Dispose GL objects:
 *          texture objects
 *          program objects
 *      framebuffer objects
 *    vertex shader objects
 *  fragment shader objects
 * Also dispose of many data structures attached to these objects: freePorts, typefaces.
 ********************************************************************************/

static void DisposeGLObjects() {
	FskGLPort p;
	FskGLBlitContext ctx;

	#if GLES_VERSION == 2

		/* Dispose of programs */
		#ifdef LOG_SHUTDOWN
			LOGD("Disposing Programs");
		#endif /* LOG_SHUTDOWN */
		DisposeProgram(&gGLGlobalAssets.yuv422.program);
		DisposeProgram(&gGLGlobalAssets.yuv420sp.program);
		DisposeProgram(&gGLGlobalAssets.yuv420.program);
		DisposeProgram(&gGLGlobalAssets.transferAlpha.program);
		DisposeProgram(&gGLGlobalAssets.drawBCBitmap.program);
		DisposeProgram(&gGLGlobalAssets.tileBitmap.program);
		DisposeProgram(&gGLGlobalAssets.drawBitmap.program);
		DisposeProgram(&gGLGlobalAssets.fill.program);
		#ifdef GL_RGB_422_APPLE
			DisposeProgram(&gGLGlobalAssets.yuv444.program);
		#endif /* GL_RGB_422_APPLE */
		#if defined(EGL_VERSION) && defined(BG3CDP_GL)
			DisposeProgram(&gGLGlobalAssets.drawBG3CDPBitmap.program);
		#endif /* defined(EGL_VERSION) && defined(BG3CDP_GL) */

		gGLGlobalAssets.isInited = 0;
	#endif /* GLES_VERSION */

	/* Dispose typefaces */
	if (gGLGlobalAssets.typeFaces) {
		FskGLTypeFace typeFace;
		while (NULL != (typeFace = gGLGlobalAssets.typeFaces)) {
			#ifdef LOG_SHUTDOWN
				LOGD("Disposing typeface(\"%s\" size=%g style=$%03X)", typeFace->fontName, FloatTextSize(typeFace->textSize), typeFace->textStyle);
			#endif /* LOG_SHUTDOWN */
			gGLGlobalAssets.typeFaces = gGLGlobalAssets.typeFaces->next;
			FskGLTypeFaceDispose(typeFace);
		}
	}

	/* Dispose of textures in the texture object cache, and zero out the cache. */
	#if EPHEMERAL_TEXTURE_CACHE_SIZE > 0
	{	int i, mustDelete;
		for (i = EPHEMERAL_TEXTURE_CACHE_SIZE, mustDelete = 0; i--;)
			if ((0 != gGLGlobalAssets.textureObjectCache[i]) && (GL_TEXTURE_UNLOADED != gGLGlobalAssets.textureObjectCache[i]))
				mustDelete = 1;
		if (mustDelete) {
			#ifdef LOG_SHUTDOWN
				LOGD("Deleting %u ephemeral textures #%u - #%u", EPHEMERAL_TEXTURE_CACHE_SIZE,
					gGLGlobalAssets.textureObjectCache[0], gGLGlobalAssets.textureObjectCache[EPHEMERAL_TEXTURE_CACHE_SIZE-1]);
			#endif /* LOG_SHUTDOWN */
			#ifdef TRACK_TEXTURES
				UntrackTextures(EPHEMERAL_TEXTURE_CACHE_SIZE, gGLGlobalAssets.textureObjectCache);
			#endif /* TRACK_TEXTURES */
			glDeleteTextures(EPHEMERAL_TEXTURE_CACHE_SIZE, gGLGlobalAssets.textureObjectCache);
			FskMemSet(gGLGlobalAssets.textureObjectCache, 0, sizeof(gGLGlobalAssets.textureObjectCache));
		}
		gGLGlobalAssets.isInited = 0;
	}
	#endif /* EPHEMERAL_TEXTURE_CACHE_SIZE */

	/* Dispose of all of the free ports and their textures */
	#if USE_PORT_POOL
		while (NULL != (p = gGLGlobalAssets.freePorts)) {
			#ifdef LOG_SHUTDOWN
				LOGD("Disposing freePort({#%u #%u #%u} %ux%u %s)",
					(unsigned)p->texture.name, (unsigned)p->texture.nameU, (unsigned)p->texture.nameV,
					(unsigned)p->texture.bounds.width, (unsigned)p->texture.bounds.height, GLInternalFormatNameFromCode(p->texture.glIntFormat));
			#endif /* LOG_SHUTDOWN */
			gGLGlobalAssets.freePorts = gGLGlobalAssets.freePorts->next;
			GLPortReallyDispose(p);
		}
	#endif /* USE_PORT_POOL */

	/* For each active port, delete its texture object, and instead indicate that it is unloaded.
	 * This provides an indication that it should be re-accelerated at some later point.
	 * The ports are still attached to a bitmap, so we still retain the ports.
	 */
	for (p = gGLGlobalAssets.activePorts; p != NULL; p = p->next) {
		if (p->texture.name && GL_TEXTURE_UNLOADED != p->texture.name) {
			FskBitmap srcBM = (FskBitmap)(p->texture.srcBM);				/* Cast away const-ness */
			#ifdef LOG_SHUTDOWN
				LOGD("Unloaded bitmap %p (useCount=%d) delete texture {#%u, #%u, #%u}",
					srcBM, srcBM->useCount, (unsigned)p->texture.name, (unsigned)p->texture.nameU, (unsigned)p->texture.nameV);
			#endif /* LOG_SHUTDOWN */
			GLPortReallyDeletePortTextures(p);								/* Unload all textures */
			if (srcBM)														/* This should always be true */
				srcBM->accelerateSeed = 0;									/* Indicate that it has not yet been accelerated */
			FskMemSet(&p->texture, 0, sizeof(p->texture));					/* Clear the entire texture data structure */
			p->texture.srcBM = srcBM;										/* but restore the srcBM ... */
			p->texture.name = GL_TEXTURE_UNLOADED;							/* ... and indicate that the texture has been UNLOADED */
			#if GL_DEBUG
				if (!srcBM)
					LOGE("ERROR: glPort=%p has no srcBM", p);
				else if (srcBM->glPort != p)	/* Assure that back-pointers to srcBM point back to us */
					LOGE("INCONSISTENT bidirectional references between glPort=%p and srcBM=%p ->glPort=%p", p, srcBM, srcBM->glPort);
			#endif /* GL_DEBUG */
		}
	}

	while (NULL != (ctx = gGLGlobalAssets.defaultContext.next)) {		/* Get rid of all secondary contexts */
		gGLGlobalAssets.defaultContext.next = ctx->next;				/* Remove this context from the list */
		FskGLBlitContextMakeCurrent(ctx);								/* Make it current */
		#if GLES_VERSION == 2
			if (ctx->frameBufferObject) {								/* Dispose of any frame buffer object */
				glBindFramebuffer(GL_FRAMEBUFFER, 0);
				glDeleteFramebuffers(1, &ctx->frameBufferObject);
			}
		#endif /* GLES_VERSION == 2 */
		FskGLBlitContextDispose(ctx);

	}
	FskGLBlitContextMakeCurrent(&gGLGlobalAssets.defaultContext);		/* Make the default context current */
 	FskMemPtrDisposeAt(&gGLGlobalAssets.defaultContext.glContext);		/* Don't dispose the main context -- just our memory of it */
}


#ifdef EGL_VERSION
static FskErr FskEGLWindowContextShutdown(void);
#endif /* EGL_VERSION */


/****************************************************************************//**
 * Dispose the Global GL Assets.
 * This tosses away everything, and resets the whole global asset state to 0.
 ********************************************************************************/

static void DisposeGlobalGLAssets(void) {
	FskGLPort p;

	#ifdef TRACK_TEXTURES
		PrintTrackedTextures("DisposeGlobalGLAssets0:");
	#endif /* TRACK_TEXTURES */

	#ifdef LOG_SHUTDOWN
		LOGD("DisposeGlobalGLAssets: calling DisposeGLObjects");
	#endif /* LOG_SHUTDOWN */
	DisposeGLObjects();

	#ifdef TRACK_TEXTURES
		PrintTrackedTextures("DisposeGlobalGLAssets1:");
	#endif /* TRACK_TEXTURES */

	if (gGLGlobalAssets.registeredLowMemoryWarning)
		FskNotificationUnregister(kFskNotificationLowMemory, purgeGL, NULL);

	#ifdef EGL_VERSION
		#ifdef LOG_SHUTDOWN
			LOGD("DisposeGlobalGLAssets: calling FskEGLWindowContextShutdown");
		#endif /* LOG_SHUTDOWN */
		FskEGLWindowContextShutdown();
	#endif /* EGL_VERSION */

	FskMemPtrDispose(gGLGlobalAssets.coordinates);
	/* gGLGlobalAssets.coordinates = NULL;		not needed with subsequent FskMemSet() */

	p = gGLGlobalAssets.activePorts;
	FskMemSet(&gGLGlobalAssets, 0, sizeof(gGLGlobalAssets));
	gGLGlobalAssets.activePorts = p;

	/* gGLGlobalAssets.isInited = 0;			not needed with preceding  FskMemSet() */
}


#ifdef EGL_VERSION

typedef struct EGLAttributeEntry {
	EGLint		code;
	SInt32		norm;	/* Divide by this number to normalize */
} EGLAttributeEntry;

static const EGLAttributeEntry gAttrib[] = {
	{	EGL_BUFFER_SIZE,				1		},
	{	EGL_ALPHA_SIZE,					1		},
	{	EGL_BLUE_SIZE,					1		},
	{	EGL_GREEN_SIZE,					1		},
	{	EGL_RED_SIZE,					1		},
	{	EGL_DEPTH_SIZE,					4		},
	{	EGL_STENCIL_SIZE,				4		},
	{	EGL_CONFIG_CAVEAT,				0		},
	{	EGL_CONFIG_ID,					0		},
	{	EGL_LEVEL,						0		},
	{	EGL_MAX_PBUFFER_WIDTH,			0		},
	{	EGL_MAX_PBUFFER_HEIGHT,			0		},
	{	EGL_MAX_PBUFFER_PIXELS,			0		},
	{	EGL_NATIVE_RENDERABLE,			0		},
	{	EGL_NATIVE_VISUAL_ID,			0		},
	{	EGL_NATIVE_VISUAL_TYPE,			0		},
	{	EGL_SAMPLES,					8		},
	{	EGL_SAMPLE_BUFFERS,				0		},
	{	EGL_SURFACE_TYPE,				1		},
	{	EGL_TRANSPARENT_TYPE,			0		},
	{	EGL_TRANSPARENT_BLUE_VALUE,		0		},
	{	EGL_TRANSPARENT_GREEN_VALUE,	0		},
	{	EGL_TRANSPARENT_RED_VALUE,		0		},
//	{	EGL_NONE,						0		},
	{	EGL_BIND_TO_TEXTURE_RGB,		1		},
	{	EGL_BIND_TO_TEXTURE_RGBA,		1		},
	{	EGL_MIN_SWAP_INTERVAL,			1		},
	{	EGL_MAX_SWAP_INTERVAL,			1		},
	{	EGL_LUMINANCE_SIZE,				1		},
	{	EGL_ALPHA_MASK_SIZE,			1		},
	{	EGL_COLOR_BUFFER_TYPE,			1		},
	{	EGL_RENDERABLE_TYPE,			1		},
	{	EGL_MATCH_NATIVE_PIXMAP,		0		},
	{	EGL_CONFORMANT,					1		}
};


static const EGLAttributeEntry* FindAttributeEntry(EGLint code) {
	const EGLAttributeEntry *p;
	for (p = &gAttrib[0]; p < &gAttrib[sizeof(gAttrib)/sizeof(gAttrib[0])]; ++p)
		if (code == p->code)
			return p;
	return NULL;
}


static SInt32 EGLAttributeNormFromCode(EGLint code) {
	const EGLAttributeEntry *p = FindAttributeEntry(code);
	return p ? p->norm : 0;
}

/*******************************************************************************
 * ConfigDistance
 * CompareConfigs
 *******************************************************************************/

static const EGLint *gTargetAttribList;
static EGLDisplay gDisplay;


static FskFixed ConfigDistance(const EGLConfig config) {
	EGLint		value, d, norm, at;
	FskFixed	distance = 0;

	if (config == NULL)
		return 0x7FFFFFFF;

	for (at = 0; gTargetAttribList[at] != EGL_NONE; at += 2) {
		EGLint atCode	= gTargetAttribList[at+0];
		EGLint atVal	= gTargetAttribList[at+1];

		if (eglGetConfigAttrib(gDisplay, config, atCode, &value)) {
			switch (atCode) {
				case EGL_RENDERABLE_TYPE:	/* Bit fields */
				case EGL_SURFACE_TYPE:
					d = ((atVal & value) == atVal) ? 0 : 1;
					break;
				case EGL_SAMPLES:
					if (atVal == 0)
						atVal = 1;		/* 0 and 1 compare the same */
					if (value == 0)
						value = 1;
					/* Fall through to compute the value of d */
				default:
					d = value - atVal;
					if (d < 0)
						d = 0;
					break;
			}
			if ((norm = EGLAttributeNormFromCode(atCode)) != 0)
				distance += FskFixDiv(d, norm);
		}
	}	/* attributes */

	return distance;
}


static int CompareConfigs(const void *config0, const void *config1) {
	FskFixed d0 = ConfigDistance(*((const EGLConfig*)config0));
	FskFixed d1 = ConfigDistance(*((const EGLConfig*)config1));
	if (d0 < d1)	return -1;
	if (d0 > d1)	return +1;
	return 0;
}


/*******************************************************************************
 * FskChooseBestEGLConfig
 *******************************************************************************/

EGLConfig FskChooseBestEGLConfig(EGLDisplay dpy, const EGLint *attribList) {
	FskErr		err		= kFskErrNone;
	EGLConfig	best	= NULL;
	EGLConfig	*config	= NULL;
	EGLBoolean	ok;
	EGLint		numConfigs, value, cf, at;

	BAIL_IF_FALSE(eglChooseConfig(dpy, &attribList[0], NULL, 0, &numConfigs), err, kFskErrGraphicsContext);
	BAIL_IF_ZERO(numConfigs, err, kFskErrMismatch);
	BAIL_IF_ERR(err = FskMemPtrNew(numConfigs * sizeof(*config), (FskMemPtr*)(void*)(&config)));
	BAIL_IF_FALSE(eglChooseConfig(dpy, &attribList[0], &config[0], numConfigs, &numConfigs), err, kFskErrGraphicsContext);

	/* Look for an exact match */
	for (cf = 0; cf < numConfigs; ++cf) {
		if ((EGL_TRUE == eglGetConfigAttrib(dpy, config[cf], EGL_CONFIG_CAVEAT, &value)) && (EGL_SLOW_CONFIG == value))
			goto nextConfig;											/* Don't consider any configurations with slow caveats */
		for (at = 0; attribList[at] != EGL_NONE; at += 2) {
			const EGLint atCode	= attribList[at+0];
			const EGLint atVal	= attribList[at+1];

			ok = config[cf] && eglGetConfigAttrib(dpy, config[cf], atCode, &value);
			if (!ok)
				goto nextConfig;
			switch (atCode) {
				case EGL_RENDERABLE_TYPE:								/* Bit fields */
				case EGL_SURFACE_TYPE:
					if ((atVal & value) != atVal)						/* AND of the bits */
						goto nextConfig;
					break;
				case EGL_SAMPLES:
					if (atVal != value && (atVal > 1 || value > 1))		/* 0 and 1 compare the same */
						goto nextConfig;
					break;
				default:
					if (atVal != value)
						goto nextConfig;
					break;
			}
		}	/* attributes */

		if (EGL_TRUE == (ok = eglGetConfigAttrib(dpy, config[cf], EGL_CONFIG_CAVEAT, &value))) {
			switch (value) {
				case EGL_NONE:
					best = config[cf];									/* We prefer a configuration that has no caveats */
					goto bail;											/* Since we got one that matches our request, plus has no caveats, we are happy with it */
				case EGL_NON_CONFORMANT_CONFIG:
					if (NULL == best)
						best = config[cf];								/* Choose the first caveat-laden configuration that is not slow */
					break;
				default:
					break;
			}
		}

	nextConfig:
		continue;
	} /* configs */
	if (best)															/* We found a match, but it has caveats */
		goto bail;

	/* No exact match. Sort and take the closest. */
	gTargetAttribList = attribList;
	gDisplay = dpy;
	FskQSort(&config[0], numConfigs, sizeof(config[0]), &CompareConfigs);
	best = config[0];
	//printf("Sorted\n");
	//PrintEGLConfigAttributes(dpy, numConfigs, &config[0]);

bail:
	FskMemPtrDispose(config);
	if (err) best = NULL;
	return best;
}

/****************************************************************************//**
 * Initialize EGL display, surface, and context.
 * This needs to be executed in the GL thread.
 * \param[in]	colorQuant	the color quantization: { 565, 888, 8888 }.
 * \param[in]	aWin	the native window. This can be released after FskEGLWindowContextInitialize() returns.
 * \return		kFskErrNone	if the platform GL was initialized successfully.
 ********************************************************************************/

static FskErr FskEGLWindowContextInitialize(UInt32 colorQuant, void *aWin)
{
	struct ConfigAttribs {
		EGLint	bufferSizeTag,		bufferSize,
				alphaSizeTag,		alphaSize,
				redSizeTag,			redSize,
				greenSizeTag,		greenSize,
				blueSizeTag,		blueSize,
				depthSizeTag,		depthSize,
				stencilSizeTag,		stencilSize,
				samplesTag,			samples,
				colorBufferTag,		colorBuffer,
				renderableTag,		renderable,
				surfaceTypeTag,		surfaceType,
				noneTag,			none;
	};
	struct ConfigAttribs configAttribs;
	static const EGLint ctxAttr[] = {
		EGL_CONTEXT_CLIENT_VERSION,	GLES_VERSION,			/* Either GLES 1 or 2 */
		EGL_NONE,					EGL_NONE
	};
	FskErr		err			= kFskErrNone;
	FskGLPort	glPort		= NULL;
	EGLDisplay	display		= NULL;
	EGLSurface	surface		= NULL;
	EGLContext	context		= NULL;
	EGLConfig	config;
	FskWindow	fWin;
	EGLint		surfWidth, surfHeight;

	#if GL_DEBUG
		FskThread thread = FskThreadGetCurrent();
		const char *threadName = FskThreadName(thread);
		LOGD("[%s, thread=%p] FskEGLWindowContextInitialize", threadName, thread);
	#endif /* GL_DEBUG */

//	if (!gGLGlobalAssets.blitContext && aWin)
//{
//fprintf(stderr, "no GLGlobalAssets.blitContext %x && aWin: %x setNativeWindow\n", gGLGlobalAssets.blitContext, aWin);
//		FskGLSetNativeWindow(aWin);
//}
	if (gGLGlobalAssets.blitContext->eglContext) {
		LOGD("GLWindowContextInitialize has already been successfully completed");
//		if (gGLGlobalAssets.blitContext->eglContext == eglGetCurrentContext())		/* All is OK: our notion of the context is the same as GL's */
			goto bail;
	#ifdef LOG_INIT
		LOGE((EGL_NO_CONTEXT == eglGetCurrentContext())				? "FskEGLWindowContextInitialize: we think there is a context but there is none"
																	: "FskEGLWindowContextInitialize: current context is not ours");
		if (gGLGlobalAssets.blitContext->eglSurface != eglGetCurrentSurface(EGL_DRAW)) {
			LOGE((EGL_NO_SURFACE == eglGetCurrentSurface(EGL_DRAW))	? "FskEGLWindowContextInitialize: we think there is a surface but there is none"
																	: "FskEGLWindowContextInitialize: current surface is not ours");
		}
		if (gGLGlobalAssets.blitContext->eglDisplay != eglGetCurrentDisplay()) {
			LOGE((EGL_NO_SURFACE == eglGetCurrentDisplay())			? "FskEGLWindowContextInitialize: we think there is a display but there is none"
																	: "FskEGLWindowContextInitialize: current display is not ours");
		}
	#endif /* LOG_INIT */
	}

	if (aWin) {																					/* We have been given a native window */
		gGLGlobalAssets.blitContext->nativeWindow = aWin;										/* Save it in case initialization fails */
		LOGD("GLWindowContextInitialize: aWin supplied = %p", aWin);
	}
	else {
		#if GL_DEBUG
			if (gGLGlobalAssets.blitContext->nativeWindow == NULL)
				LOGD("[%s] FskEGLWindowContextInitialize CANNOT COMPLETE because it has no native window", threadName);
		#endif /* GL_DEBUG */
		BAIL_IF_NULL(gGLGlobalAssets.blitContext->nativeWindow, err, kFskErrGraphicsContext);	/* No hope for initialization; bail */
		aWin = gGLGlobalAssets.blitContext->nativeWindow;										/* Use the saved native window */
		LOGD("GLWindowContextInitialize: aWin set from global nativeWindow = %p", aWin);
	}

	#if GL_DEBUG
		if (strcmp("main", threadName) == 0)
			LOGD("[%s] FskEGLWindowContextInitialize CANNOT COMPLETE because it is being called from the main thread", threadName);
	#endif /* GL_DEBUG */


	configAttribs.depthSizeTag		= EGL_DEPTH_SIZE;			configAttribs.depthSize		= 0;
	configAttribs.stencilSizeTag	= EGL_STENCIL_SIZE;			configAttribs.stencilSize	= 0;
	configAttribs.samplesTag		= EGL_SAMPLES;				configAttribs.samples		= 0;
	configAttribs.colorBufferTag	= EGL_COLOR_BUFFER_TYPE;	configAttribs.colorBuffer	= EGL_RGB_BUFFER;
	configAttribs.renderableTag		= EGL_RENDERABLE_TYPE;		configAttribs.renderable	= EGL_OPENGL_ES2_BIT;
	configAttribs.noneTag			= EGL_NONE;					configAttribs.none			= 0;
	configAttribs.surfaceTypeTag	= EGL_SURFACE_TYPE;			configAttribs.surfaceType	= EGL_WINDOW_BIT;
	configAttribs.bufferSizeTag		= EGL_BUFFER_SIZE;
	configAttribs.alphaSizeTag		= EGL_ALPHA_SIZE;
	configAttribs.redSizeTag		= EGL_RED_SIZE;
	configAttribs.greenSizeTag		= EGL_GREEN_SIZE;
	configAttribs.blueSizeTag		= EGL_BLUE_SIZE;

	switch (colorQuant) {
		case 565:
			configAttribs.bufferSize	= 16;
			configAttribs.alphaSize		= 0;
			configAttribs.redSize		= 5;
			configAttribs.greenSize		= 6;
			configAttribs.blueSize		= 5;
			break;
		case 888:
			configAttribs.bufferSize	= 24;
			configAttribs.alphaSize		= 0;
			configAttribs.redSize		= 8;
			configAttribs.greenSize		= 8;
			configAttribs.blueSize		= 8;
			break;
		case 8888:
		default:
			configAttribs.bufferSize	= 32;
			configAttribs.alphaSize		= 8;
			configAttribs.redSize		= 8;
			configAttribs.greenSize		= 8;
			configAttribs.blueSize		= 8;
			break;
	}

	BAIL_IF_FALSE(EGL_NO_DISPLAY != (display = eglGetDisplay((NativeDisplayType)EGL_DEFAULT_DISPLAY)),			err, kFskErrGraphicsContext);
	LOGD("GLWindowContextInitialize: eglGetDisplay = %p", display);
	BAIL_IF_FALSE(                  eglInitialize(display, NULL, NULL),											err, kFskErrGraphicsContext);	/* (display, &major, &minor) */
	LOGD("GLWindowContextInitialize: eglInitialize successful");
	BAIL_IF_NULL(                   (config  = FskChooseBestEGLConfig(display, (void*)&configAttribs)),			err, kFskErrGraphicsContext);
	LOGD("GLWindowContextInitialize: FskChooseBestEGLConfig = %p", config);
//	BAIL_IF_FALSE					(eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &format),		err, kFskErrGraphicsContext);
//	LOGD("GLWindowContextInitialize: eglGetConfigAttrib format = %d", format);
//	BAIL_IF_NEGATIVE(				ANativeWindow_setBuffersGeometry(aWin, 0, 0, format),						err, kFskErrGraphicsContext);	/* buffer size == window size, good format */
//	LOGD("GLWindowContextInitialize: ANativeWindow_setBuffersGeometry successful");
	BAIL_IF_FALSE(EGL_NO_SURFACE !=	(surface = eglCreateWindowSurface(display, config, aWin, NULL)),			err, kFskErrGraphicsContext);
	LOGD("GLWindowContextInitialize: eglCreateWindowSurface = %p", surface);
	BAIL_IF_FALSE(EGL_NO_CONTEXT != (context = eglCreateContext(display, config, EGL_NO_CONTEXT, ctxAttr)),		err, kFskErrGraphicsContext);
	LOGD("GLWindowContextInitialize: eglCreateContext = %p", context);
	BAIL_IF_FALSE(EGL_FALSE      != eglMakeCurrent(display, surface, surface, context),							err, kFskErrGraphicsContext);
	LOGD("GLWindowContextInitialize: eglMakeCurrent successful");

	err = FskGLSetEGLContext(display, surface, context);

	eglQuerySurface(display, surface, EGL_WIDTH,  &surfWidth);
	eglQuerySurface(display, surface, EGL_HEIGHT, &surfHeight);

//	BAIL_IF_ERR(err = GetFskEGLError());

	fWin = FskWindowGetActive();
	LOGD("GLWindowContextInitialize: fWin = %p", fWin);
	//if (fWin) {
	//	UInt32		winWidth,  winHeight;
	//	err = FskWindowGetSize(fWin, &winWidth, &winHeight);
	//	/* Duke it out! */
	//	if (surfWidth  != winWidth)		LOGI( "surfWidth=%d != winWidth=%d",  (int)surfWidth,  (int)winWidth);		// DEBUG only
	//	if (surfHeight != winHeight)	LOGI("surfHeight=%d != winHeight=%d", (int)surfHeight, (int)winHeight);		// DEBUG only
	//}

	if (NULL == (glPort = FskGLPortGetCurrent())) {
		BAIL_IF_ERR(err = FskGLPortNew(surfWidth, surfHeight, fWin, &glPort));
		LOGD("GLWindowContextInitialize: FskGLPortNew(%d, %d) = %p", surfWidth, surfHeight, glPort);
	}
	else {
		LOGD("GLWindowContextInitialize: FskGLPortGetCurrent = %p; Resizing port from (%u, %u) to (%d, %d)",
			glPort, (unsigned)glPort->portWidth, (unsigned)glPort->portHeight, surfWidth, surfHeight);
		glPort->portWidth  = surfWidth;		/* Can't call FskGLPortResize(glPort, surfWidth, surfHeight) yet */
		glPort->portHeight = surfHeight;
		FskGLSetGLView(glPort);				/* We toss away the error code, because GL may not be fully initialized. */
		FskGLPortSetSysContext(glPort, fWin);
	}
	LOGD("GLWindowContextInitialize: done");

bail:
	if (err) {
		if (display) {
			#if GL_DEBUG
				LOGD("[%s] FskEGLWindowContextInitialize: FskEGLWindowContextInitialize FAILED, returning %d", threadName, (int)err);
			#else /* GL_DEBUG */
				LOGD("GLWindowContextInitialize: FAILED, returning %d", (int)err);
			#endif /* GL_DEBUG */
			//eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
			if (surface)	eglDestroySurface(display, surface);
			if (context)	eglDestroyContext(display, context);
			//eglTerminate(display);
		}
	}
	else {
		#if GL_DEBUG
			if (eglGetCurrentContext()			!= context)	LOGE(    "EGLCurrentContext=%p != %p", eglGetCurrentContext(),         context);
			if (eglGetCurrentSurface(EGL_DRAW)	!= surface)	LOGE("EGLCurrentDrawSurface=%p != %p", eglGetCurrentSurface(EGL_DRAW), surface);
			if (eglGetCurrentSurface(EGL_READ)	!= surface)	LOGE("EGLCurrentReadSurface=%p != %p", eglGetCurrentSurface(EGL_READ), surface);
			if (eglGetCurrentDisplay()			!= display)	LOGE(    "EGLCurrentDisplay=%p != %p", eglGetCurrentDisplay(),         display);
		#endif /* GL_DEBUG */
		LOGD("GLWindowContextInitialize: successful");
	}
	return err;
}


/****************************************************************************//**
 * Set GL native window.
 * This needs to be executed in the GL thread.
 * \param[in]	aWin	the native window. This can be released after FskGLSetNativeWindow() returns.
 * \return		kFskErrNone
 ********************************************************************************/

#undef FskGLSetNativeWindow
FskErr FskGLSetNativeWindow(void *nativeWindow)
{
	if (!gGLGlobalAssets.blitContext)
		gGLGlobalAssets.blitContext = &gGLGlobalAssets.defaultContext;

//	if (NULL == nativeWindow) {
//		return kFskErrInvalidParameter;
//	}

	#if GL_DEBUG
		LOGD("[%s] FskGLSetNativeWindow(%p (was %p))", FskThreadName(FskThreadGetCurrent()), nativeWindow, gGLGlobalAssets.blitContext->nativeWindow);
	#endif /* GL_DEBUG */
	gGLGlobalAssets.blitContext->nativeWindow = nativeWindow;
	return kFskErrNone;
}


/****************************************************************************//**
 * Shut down GL display, surface, and context.
 * This needs to be executed in the GL thread.
 * \return		kFskErrNone	if GL was shut down successfully.
 ********************************************************************************/

static FskErr FskEGLWindowContextShutdown(void)
{
	FskErr err	= kFskErrNone;

	#ifdef _MSC_VER
		return err;			/* Why does it crash on Windows? */
	#endif /* _MSC_VER */

	DisposeGLObjects();

	/* Shut down the GL surface, context, and display. */
	if (gGLGlobalAssets.blitContext) {					/* If there is a current Fsk GL Blit Context */
		if (gGLGlobalAssets.blitContext->eglDisplay) {
			LOGD("Calling eglMakeCurrent(%p, %p, %p, %p)", gGLGlobalAssets.blitContext->eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
			eglMakeCurrent(gGLGlobalAssets.blitContext->eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
			LOGD("Calling eglDestroyContext(%p, %p) if non-null", gGLGlobalAssets.blitContext->eglDisplay, gGLGlobalAssets.blitContext->eglContext);
			if (gGLGlobalAssets.blitContext->eglContext)
				eglDestroyContext(gGLGlobalAssets.blitContext->eglDisplay, gGLGlobalAssets.blitContext->eglContext);
			LOGD("Calling eglDestroySurface(%p, %p) if non-null", gGLGlobalAssets.blitContext->eglDisplay, gGLGlobalAssets.blitContext->eglSurface);
			if (gGLGlobalAssets.blitContext->eglSurface)
				eglDestroySurface(gGLGlobalAssets.blitContext->eglDisplay, gGLGlobalAssets.blitContext->eglSurface);
			LOGD("Calling eglTerminate(%p)", gGLGlobalAssets.blitContext->eglDisplay);
			eglTerminate(gGLGlobalAssets.blitContext->eglDisplay);
			err = GetFskGLError();
		}
		LOGD("Setting egl{Display, Surface, Context} to NULL");
		gGLGlobalAssets.blitContext->eglDisplay = NULL;
		gGLGlobalAssets.blitContext->eglSurface = NULL;
		gGLGlobalAssets.blitContext->eglContext = NULL;

		LOGD("Setting nativeWindow to NULL, isInited and lastProgram to 0");
		gGLGlobalAssets.blitContext->nativeWindow = NULL;
		#if GLES_VERSION == 2
			gGLGlobalAssets.blitContext->lastProgram = 0;
		#endif /* GLES_VERSION == 2 */
		// TODO: Should we dispose of the FskGLBlitContext?
	}
	gGLGlobalAssets.isInited = 0;

	return err;
}


/****************************************************************************//**
 * Set the EGL display, surface, and context.
 * \return		kFskErrNone
 ********************************************************************************/

#undef FskGLSetEGLContext
FskErr FskGLSetEGLContext(void *eglDisplay, void *eglSurface, void *eglContext)
{
	if (!gGLGlobalAssets.blitContext)
		gGLGlobalAssets.blitContext = &gGLGlobalAssets.defaultContext;
	gGLGlobalAssets.blitContext->eglDisplay = eglDisplay;
	gGLGlobalAssets.blitContext->eglSurface = eglSurface;
	gGLGlobalAssets.blitContext->eglContext = eglContext;
	if (eglDisplay == NULL || eglSurface == NULL || eglContext == NULL) {
		gGLGlobalAssets.isInited = 0;	/* Requires re-initializations */
		gGLGlobalAssets.blitContext->nativeWindow = NULL;
		LOGD("de-initializing global assets");
	}
	return kFskErrNone;
}
#endif /* EGL_VERSION */

/****************************************************************************//**
* Set GL default FBO to be used when rendering to the screen.
* This needs to be executed in the GL thread.
* \param[in]	fbo     the default FBO
* \return		kFskErrNone
********************************************************************************/

FskErr FskGLSetDefaultFBO(unsigned fbo)
{
	if (gGLGlobalAssets.blitContext == NULL)
		gGLGlobalAssets.blitContext = &gGLGlobalAssets.defaultContext;
	#if defined(LOG_PARAMETERS) || defined(LOG_INIT)
		LOGD("SetDefaultFBO(%u), was %u", fbo, gGLGlobalAssets.blitContext->defaultFBO);
	#endif /* defined(LOG_PARAMETERS) || defined(LOG_TEXT) */
	gGLGlobalAssets.blitContext->defaultFBO = fbo;
	return kFskErrNone;
}

#if TARGET_OS_ANDROID
/****************************************************************************//**
 * Load all unloaded textures
 ********************************************************************************/
static FskErr SetGLTexture(FskConstBitmap srcBM, FskConstRectangle texRect, GLTexture tx);

#ifdef UNUSED

#include "FskUtilities.h"
static int CompareTextureNames(const void *va, const void *vb) {
	GLuint	a = *((const GLuint*)va),
			b = *((const GLuint*)vb);
	if (a < b)	return -1;
	if (a > b)	return +1;
	return 0;
}

static void LoadAllUnloadedTextures() {
	FskGLPort p;

	#if GL_DEBUG
		int u, n;
		for (p = gGLGlobalAssets.activePorts, u = 0, n = 0; p != NULL; p = p->next) {
			if (p->texture.name) {
				++n;												/* Count the total number of textures */
				if (GL_TEXTURE_UNLOADED == p->texture.name)
					++u;											/* Count the total number of unloaded textures */
			}
		}
		LOGD("Deferred loading of %d/%d textures", u, n);
		for (p = gGLGlobalAssets.activePorts, u = 0, n = 0; p != NULL; p = p->next) {
			if (p->texture.name) {
				if (!p->texture.srcBM)
					LOGE("ERROR: glPort=%p has no srcBM");
				else if (p->texture.srcBM->glPort != p)	/* Assure that back-pointers to srcBM point back to us */
					LOGE("INCONSISTENT bidirectional references between glPort=%p and srcBM=%p ->glPort=%p", p, p->texture.srcBM, p->texture.srcBM->glPort);
			}
		}
		if (gGLGlobalAssets.freePorts) {
			for (p = gGLGlobalAssets.activePorts, n = 0; p != NULL; p = p->next)
				++n;
			LOGD("Unexpected %d non-NULL freePorts", n);
		}
	#endif /* GL_DEBUG */

	for (p = gGLGlobalAssets.activePorts; p != NULL; p = p->next) {
		FskErr err;

		if (GL_TEXTURE_UNLOADED != p->texture.name)
			continue;

		if (kFskErrNone != FskGenGLTexturesAndInit(1, &p->texture.name))
			continue;

		err = SetGLTexture(p->texture.srcBM, NULL, &p->texture);
		if (kFskErrNone == err) {
			p->texture.wrapMode  = GL_CLAMP_TO_EDGE;
			p->texture.filter    = GL_LINEAR;
			p->texture.fboInited = false;
		}
		else if (kFskErrTextureTooLarge != err) {
			GLPortReallyDeletePortTextures(p);
			p->texture.name          = GL_TEXTURE_UNLOADED;
			p->texture.bounds.width  = 0;
			p->texture.bounds.height = 0;
		}
	}

	#if GL_DEBUG
		for (p = gGLGlobalAssets.activePorts, u = 0, n = 0; p != NULL; p = p->next) {
			if (p->texture.name) {
				++n;															/* Count the total number of textures */
				if (GL_TEXTURE_UNLOADED == p->texture.name)
					++u;														/* Count the total number of unloaded textures */
				else
					LOGD("Loaded bitmap %p texture {#%u, #%u, #%u}", p->texture.srcBM, p->texture.name, p->texture.nameU, p->texture.nameV);
			}
		}
		LOGD("%u textures loaded, %u textures not loaded, %u total", n - u, u, n);

		{	UInt32	numTex = 0;
			#define NUM_TMP_TEX 360
			GLuint	texNames[NUM_TMP_TEX];
			char	texStr[NUM_TMP_TEX * 4];
			for (p = gGLGlobalAssets.activePorts; p != NULL; p = p->next)		/* Collect textures from active ports */
				if (p->texture.name && numTex < NUM_TMP_TEX)
					texNames[numTex++] = p->texture.name;
			#if USE_PORT_POOL
				for (p = gGLGlobalAssets.freePorts; p != NULL; p = p->next)		/* Collect textures from free ports */
					if (p->texture.name && numTex < NUM_TMP_TEX)
						texNames[numTex++] = p->texture.name;
			#endif /* USE_PORT_POOL */
			#if EPHEMERAL_TEXTURE_CACHE_SIZE > 0
				for (n = 0; n < EPHEMERAL_TEXTURE_CACHE_SIZE; ++n)				/* Collect textures from the texture object cache */
					if (numTex < NUM_TMP_TEX)
						texNames[numTex++] = gGLGlobalAssets.textureObjectCache[n];
			#endif /* EPHEMERAL_TEXTURE_CACHE_SIZE */
			FskQSort(texNames, numTex, sizeof(GLuint), CompareTextureNames);	/* Sort all textures */
			texStr[0] = 0;
			for (n = 0; n < numTex; ++n) {
				u = FskStrLen(texStr);
				if (u < sizeof(texStr) - 4)
					snprintf(texStr + u, sizeof(texStr) - u, " %d", (int)texNames[n]);
			}
			LOGD("%u tex:%s", numTex, texStr);
		}

	#endif /* GL_DEBUG */
}
#endif /* UNUSED */
#endif /* TARGET_OS_ANDROID */


/****************************************************************************//**
 * Prepare YUV Texture for uploading.
 *	\param[in]		yuvBM	the source bitmap.
 *	\param[in,out]	tx		the texture record, which will be updated if necessary.
 *	\param[out]		yuv		pointer to the pixels, as returned by FskBitmapReadBegin().
 *	\return			kFskErrNone					if the operation was successful.
 *	\return			kFskErrUnsupportedPixelType	if the pixel type is not supported.
 ********************************************************************************/

static FskErr PrepareYUVTexture(FskConstBitmap yuvBM, GLTexture tx, const UInt8 **yuv) {
	FskErr	err	= kFskErrNone;
	SInt32	width, height;
	GLint	glQuality = (tx->filter == GL_NEAREST) ? GL_NEAREST : GL_LINEAR;

	/* Resize the textures if necessary */
	switch (yuvBM->pixelFormat) {		/* The width of the texture includes all rowBytes */
		case kFskBitmapFormatYUV420:														/* FOURCC('I420'): YYYY...UU...VV... */
		case kFskBitmapFormatYUV420spuv:													/* FOURCC("NV12') */
		case kFskBitmapFormatYUV420spvu:	width = yuvBM->rowBytes;			break;		/* FOURCC("NV21') */
		case kFskBitmapFormatUYVY:			width = yuvBM->rowBytes >> 1;		break;		/* FOURCC('UYVY'): UYVYUYVY... */
		default:							err = kFskErrUnsupportedPixelType;	goto bail;
	}
	height = yuvBM->bounds.height;
	SET_TEXTURE(tx->name);

	/* Don't know if there are any restrictions on texture sizes. */
	if (width != tx->bounds.width || height != tx->bounds.height) {							/* (re)allocate texture storage */
		/* Initialize the texture unit, if the textures needed to be resized. */
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, glQuality);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, glQuality);

		if (width  & 1)	{ LOGE("PrepareYUVTexture: odd  width=%d", (int)width ); ++width;  }
		if (height & 1)	{ LOGE("PrepareYUVTexture: odd height=%d", (int)height); ++height; }
		tx->bounds.x		= 0;
		tx->bounds.y		= 0;
		tx->bounds.width	= width;
		tx->bounds.height	= height;
		tx->glIntFormat		= 0;
		tx->wrapMode		= GL_CLAMP_TO_EDGE;
		tx->filter			= glQuality;
		tx->fboInited		= false;
	}

	err = FskBitmapReadBegin((FskBitmap)yuvBM, (const void**)(const void*)(yuv), NULL, NULL);	/* Get pixels */

bail:
	return err;
}


#if TARGET_CPU_ARM && !TARGET_OS_IPHONE
/********************************************************************************
 * SetMarvellTextureYUV420
 ********************************************************************************/

static FskErr SetMarvellTextureYUV420(FskConstBitmap yuvBM, GLTexture tx) {
	FskErr		err		= kFskErrNone;
	const UInt8	*yuv;

	BAIL_IF_ERR(err = PrepareYUVTexture(yuvBM, tx, &yuv));		/* This calls FskBitmapReadBegin() */
	(*((MarvellTexImageVideoProc)(gGLGlobalAssets.platformYUVUploadProc)))(GL_TEXTURE_2D, tx->bounds.width, tx->bounds.height, GL_I420_MRVL, yuv);
	FskBitmapReadEnd((FskBitmap)yuvBM);

bail:
	return err;
}


/********************************************************************************
 * SetMarvellTextureUYVY
 ********************************************************************************/

static FskErr SetMarvellTextureUYVY(FskConstBitmap yuvBM, GLTexture tx) {
	FskErr		err		= kFskErrNone;
	const UInt8	*yuv;

	BAIL_IF_ERR(err = PrepareYUVTexture(yuvBM, tx, &yuv));		/* This calls FskBitmapReadBegin() */
	(*((MarvellTexImageVideoProc)(gGLGlobalAssets.platformYUVUploadProc)))(GL_TEXTURE_2D, tx->bounds.width, tx->bounds.height, GL_UYVY_MRVL, yuv);
	FskBitmapReadEnd((FskBitmap)yuvBM);

bail:
	return err;
}
#endif /* TARGET_CPU_ARM && !TARGET_OS_IPHONE */


#if TARGET_OS_MAC && !TARGET_OS_IPHONE
/********************************************************************************
 * SetMacTextureUYVY
 ********************************************************************************/

static FskErr SetMacTextureUYVY(FskConstBitmap yuvBM, GLTexture tx) {
	FskErr		err	= kFskErrNone;
	const UInt8	*yuv;

	BAIL_IF_ERR(err = PrepareYUVTexture(yuvBM, tx, &yuv));		/* This calls FskBitmapReadBegin() */
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tx->bounds.width, tx->bounds.height, 0, GL_YCBCR_422_APPLE, GL_UNSIGNED_SHORT_8_8_APPLE, yuv);	/* Reshape the texture accordingly */
	FskBitmapReadEnd((FskBitmap)yuvBM);

bail:
	return err;
}
#endif /* TARGET_OS_MAC */


/********************************************************************************
 * HACKed versions of missing functions.
 ********************************************************************************/

static void glBlendFuncSeparateHACK(GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha) { if (srcAlpha || dstAlpha) {}; glBlendFunc(srcRGB, dstRGB); }
#define GL_GLOBAL_ASSETS_ARE_INITIALIZED()	gGLGlobalAssets.isInited	/**< Returns true if the GL global assets have already been initialized. */


/********************************************************************************
 * FskKplGLContextInitialize.
 ********************************************************************************/

#if TARGET_OS_KPL
FskErr FskKplGLContextInitialize() {
	FskErr		err = kFskErrNone;
	EGLDisplay	display;
	EGLSurface	surface;
	EGLContext	context;
	EGLint		surfWidth, surfHeight;
	void		*nativeWindow;
	FskWindow	fWin;
	FskGLPort	glPort;

	if (NULL == gGLGlobalAssets.blitContext)
		gGLGlobalAssets.blitContext = &gGLGlobalAssets.defaultContext;
	if (gGLGlobalAssets.blitContext->eglContext) {
		LOGD("FskKplGLContextInitialize has already been successfully completed");
		goto bail;
	}
	if (NULL != gGLGlobalAssets.blitContext->nativeWindow)
		goto bail;

	fWin = FskWindowGetActive();
	if (NULL == fWin)
		fWin = FskWindowGetInd(0, NULL);
	//if (NULL == fWin) {
		//err = kFskErrOutOfSequence;
		//goto bail;
	//}

#if USE_FRAMEBUFFER_VECTORS
	err = FskFrameBufferGetEGLContext((void**)&display, (void**)&surface, (void**)&context, (void**)&nativeWindow);
	if (err)
#endif /* USE_FRAMEBUFFER_VECTORS */
		err = KplGLInitialize(&display, &surface, &context, &nativeWindow);
	BAIL_IF_ERR(err);

	FskGLSetNativeWindow(nativeWindow);
	FskGLSetEGLContext(display, surface, context);

	eglQuerySurface(display, surface, EGL_WIDTH,  &surfWidth);
	eglQuerySurface(display, surface, EGL_HEIGHT, &surfHeight);

	if (NULL == (glPort = FskGLPortGetCurrent())) {
		BAIL_IF_ERR(err = FskGLPortNew(surfWidth, surfHeight, fWin, &glPort));
		LOGD("FskKplGLContextInitialize: FskGLPortNew(%d, %d) = %p", surfWidth, surfHeight, glPort);
	}
	else {
		LOGD("FskKplGLContextInitialize: FskGLPortGetCurrent = %p; Resizing port from (%u, %u) to (%d, %d)",
			glPort, (unsigned)glPort->portWidth, (unsigned)glPort->portHeight, surfWidth, surfHeight);
		glPort->portWidth  = surfWidth;		/* Can't call FskGLPortResize(glPort, surfWidth, surfHeight) yet */
		glPort->portHeight = surfHeight;
		FskGLSetGLView(glPort);				/* We toss away the error code, because GL may not be fully initialized. */
		FskGLPortSetSysContext(glPort, fWin);
	}

bail:
	return err;
}
#endif /* TARGET_OS_KPL */


#if TARGET_OS_ANDROID
/****************************************************************************//**
 * Look up Renderer Info.
 * Black list for GL functionality.
 ********************************************************************************/

typedef struct RendererInfo {
	const char	*name;	/**< The renderer name. */
	Boolean		hasFBO;	/**< Has a trustworthy FBO implementation. */
} RendererInfo;

static const RendererInfo rendererInfoTable[] = {
	/*	name				FBO	*/
	{	"Adreno (TM) 220",	1	},
	{	"Adreno (TM) 225",	1	},
	{	"Adreno 205",		1	},
	{	"GC1000 core",		1	},
	{	"GC530 core",		1	},	/* It doesn't work */
	{	"GC860 core",		1	},	/* It doesn't work on the Vizio */
	{	"Mali-400 MP",		1	},	/* It doesn't work on some Mali drivers */
	{	"NVIDIA Tegra 3",	1	},
	{	"NVIDIA Tegra",		1	},
	{	"PowerVR SGX 531",	1	},	/* It doesn't work */
	{	"PowerVR SGX 540",	1	},	/* It doesn't work on the Samsung Infuse */
	{	"PowerVR SGX 543",	1	},	/* This was found on the iPhone */
	{	NULL,				1	}
};
static const RendererInfo* LookupRendererInfo(const char *rendererName) {
	const RendererInfo *p;
	for (p = rendererInfoTable; p->name != NULL; ++p)
		if (0 == strcmp(rendererName, p->name))
			break;
	return p;
}
#endif /* TARGET_OS_ANDROID */


#if GLES_VERSION == 2
/****************************************************************************//**
 * SetDefaultShaderStates for the current context.
 ********************************************************************************/

static void SetDefaultShaderStates(void) {
	const float white[4] = { 1.f, 1.f, 1.f, 1.f };																/* Default opColor to opaque white */

	/* Fill shader */
	if (kFskErrNone == FskGLUseProgram(gGLGlobalAssets.fill.program)) {
		glUniform4fv(gGLGlobalAssets.fill.opColor, 1, white);													/* Set fill color to opaque white ... */
		gGLGlobalAssets.blitContext->fill.lastParams.color = ENCODE_LAST_OP_COLOR(255,255,255,255);				/* ... and remember it */
		gGLGlobalAssets.blitContext->fill.lastParams.level = FULL_LEVEL_FLAG;									/* Set the blend level to full */
	}

	/* DrawBitmap shader */
	if (kFskErrNone == FskGLUseProgram(gGLGlobalAssets.drawBitmap.program)) {
		glUniform4fv(gGLGlobalAssets.drawBitmap.opColor, 1, white);												/* Set opColor to opaque white ... */
		gGLGlobalAssets.blitContext->drawBitmap.lastParams.color = ENCODE_LAST_OP_COLOR(255,255,255,255);		/* ... and remember it */
		gGLGlobalAssets.blitContext->drawBitmap.lastParams.level = FULL_LEVEL_FLAG;								/* Set the blend level to full */
		glUniform1i(gGLGlobalAssets.drawBitmap.srcBM, 0);														/* Set the texture unit */
	}

	/* TileBitmap shader */
	if (kFskErrNone == FskGLUseProgram(gGLGlobalAssets.tileBitmap.program)) {
		glUniform4fv(gGLGlobalAssets.tileBitmap.opColor, 1, white);												/* Set opColor to opaque white ... */
		gGLGlobalAssets.blitContext->tileBitmap.lastParams.color = ENCODE_LAST_OP_COLOR(255,255,255,255);		/* ... and remember it */
		gGLGlobalAssets.blitContext->tileBitmap.lastParams.level = FULL_LEVEL_FLAG;								/* Set the blend level to full */
		glUniform4f(gGLGlobalAssets.tileBitmap.tileSpace, 0.f, 0.f, 1.f, 1.f);									/* Initialize tile space to (0, 0, 1, 1) */
		glUniform1i(gGLGlobalAssets.tileBitmap.srcBM, 0);														/* Set the texture unit */
	}

	/* Draw Brightness Contrast Bitmap shader */
	if (kFskErrNone == FskGLUseProgram(gGLGlobalAssets.drawBCBitmap.program)) {
		glUniform4fv(gGLGlobalAssets.drawBCBitmap.opColor, 1, white);											/* Set opColor to opaque white ... */
		gGLGlobalAssets.blitContext->drawBCBitmap.lastParams.color = ENCODE_LAST_OP_COLOR(255,255,255,255);		/* ... and remember it */
		gGLGlobalAssets.blitContext->drawBCBitmap.lastParams.level = FULL_LEVEL_FLAG;							/* Set the blend level to full */
		glUniform4f(gGLGlobalAssets.drawBCBitmap.offColor, 0.f, 0.f, 0.f, 0.f);									/* Set offset color to (0, 0, 0, 0) */
		glUniform1i(gGLGlobalAssets.drawBCBitmap.srcBM, 0);														/* Set the texture unit */
	}

	/* Transfer Alpha shader */
	if (kFskErrNone == FskGLUseProgram(gGLGlobalAssets.transferAlpha.program)) {
		glUniform4fv(gGLGlobalAssets.transferAlpha.opColor, 1, white);											/* Set opColor to opaque white ... */
		gGLGlobalAssets.blitContext->transferAlpha.lastParams.color = ENCODE_LAST_OP_COLOR(255,255,255,255);	/* ... and remember it */
		gGLGlobalAssets.blitContext->transferAlpha.lastParams.level = FULL_LEVEL_FLAG;							/* Set the blend level to full */
		glUniform1i(gGLGlobalAssets.transferAlpha.srcBM, 0);													/* Set the texture unit */
	}

	/* YUV 4:2:0 shader */
	if (kFskErrNone == FskGLUseProgram(gGLGlobalAssets.yuv420.program)) {
		glUniform1i(gGLGlobalAssets.yuv420.yTex, 0);															/* Set Y texture unit */
		glUniform1i(gGLGlobalAssets.yuv420.uTex, 1);															/* Set U texture unit */
		glUniform1i(gGLGlobalAssets.yuv420.vTex, 2);															/* Set V texture unit */
	}

	/* YUV 4:2:0 SP shader */
	if (kFskErrNone == FskGLUseProgram(gGLGlobalAssets.yuv420sp.program)) {
		glUniform1i(gGLGlobalAssets.yuv420sp.yTex, 0);															/* Set Y texture unit */
		glUniform1i(gGLGlobalAssets.yuv420sp.uvTex, 1);															/* Set UV texture unit */
	}

	/* YUV 4:2:2 shader */

#ifdef GL_RGB_422_APPLE
	/* YUV 4:4:4 shader */
	if (kFskErrNone == FskGLUseProgram(gGLGlobalAssets.yuv444.program)) {
		glUniform1i(gGLGlobalAssets.yuv444.srcBM, 0);															/* Set the texture unit */
	}
#endif /* GL_RGB_422_APPLE */

#if defined(EGL_VERSION) && defined(BG3CDP_GL)
	/* BG3CDP shader */
	if (kFskErrNone == FskGLUseProgram(gGLGlobalAssets.drawBG3CDPBitmap.program)) {
		glUniform4fv(gGLGlobalAssets.drawBG3CDPBitmap.opColor, 1, white);										/* Set opColor to opaque white ... */
		gGLGlobalAssets.blitContext->drawBG3CDPBitmap.lastParams.color = ENCODE_LAST_OP_COLOR(255,255,255,255);	/* ... and remember it */
		gGLGlobalAssets.blitContext->drawBG3CDPBitmap.lastParams.level = FULL_LEVEL_FLAG;						/* Set the blend level to full */
		glUniform4f(gGLGlobalAssets.drawBG3CDPBitmap.offColor, 0.f, 0.f, 0.f, 0.f);								/* Set offset color to (0, 0, 0, 0) */
		glUniform1i(gGLGlobalAssets.drawBG3CDPBitmap.srcBM, 0);													/* Set the texture unit */
	}
#endif /* defined(EGL_VERSION) && defined(BG3CDP_GL) */
}
#endif /* GLES_VERSION == 2 */


/****************************************************************************//**
 * Init The Context State.
 ********************************************************************************/

static void InitContextState(void) {
	SET_GLOBAL_BLEND_ENABLE(false);
	SET_GLOBAL_BLEND_FUNC(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	SET_GLOBAL_SCISSOR_ENABLE(false);
	SET_GLOBAL_SCISSOR(0, 0, 0, 0);
	glActiveTexture(GL_TEXTURE0);
	SET_TEXTURE(0);																/* Initialize the current texture */
	#if GLES_VERSION == 2
		gGLGlobalAssets.blitContext->lastProgram = 0;
		glVertexAttribPointer(VERTEX_POSITION_INDEX, 2, GL_COORD_TYPE_ENUM, GL_FALSE, sizeof(GLTexVertex), gGLGlobalAssets.coordinates + TEXVERTEX_X);
		glVertexAttribPointer(VERTEX_TEXCOORD_INDEX, 2, GL_COORD_TYPE_ENUM, GL_FALSE, sizeof(GLTexVertex), gGLGlobalAssets.coordinates + TEXVERTEX_U);
		glEnableVertexAttribArray(VERTEX_POSITION_INDEX);
		glEnableVertexAttribArray(VERTEX_TEXCOORD_INDEX);
		SetDefaultShaderStates();
	#else /* GLES_VERSION == 1 */
		glVertexPointer  (2, GL_COORD_TYPE_ENUM, sizeof(GLTexVertex), gGLGlobalAssets.coordinates + TEXVERTEX_X);
		glTexCoordPointer(2, GL_COORD_TYPE_ENUM, sizeof(GLTexVertex), gGLGlobalAssets.coordinates + TEXVERTEX_U);
	#endif /* GLES_VERSION == 1 */

	glClearColor(.5f, .5f, .5f, 1.f);									/* We clear the buffer to prevent crashing in libEGL_adreno200 */
	glClear(GL_COLOR_BUFFER_BIT);
}


/****************************************************************************//**
 * Initialize the Global GL Assets.
 *	\return	kFskErrNone				if the global GL assets were initialized successfully.
 *	\return kFskErrGraphicsContext	if GL has not yet been initialized.
 ********************************************************************************/

#undef InitGlobalGLAssets
static FskErr InitGlobalGLAssets() {
	FskErr				err		= kFskErrNone;
	Boolean				inited	= false;
	FskGLCapabilities	caps	= NULL;

	if (GL_GLOBAL_ASSETS_ARE_INITIALIZED()) {
		#ifdef LOG_INIT
			LOGD("[%s] InitGlobalGLAssets  -- already initialized", FskThreadName(FskThreadGetCurrent()));
		#endif /* LOG_INIT */
		return kFskErrNone;
	}

	#ifdef LOG_INIT
		LOGD("[%s] InitGlobalGLAssets  -- trying to initialize", FskThreadName(FskThreadGetCurrent()));
	#endif /* LOG_INIT */


	gGLGlobalAssets.blitContext = &gGLGlobalAssets.defaultContext;						/* Initialize to the default context */


	/*
	 * System-specific initialization
	 */

	#if TARGET_OS_MAC
		FskGLSetFocusDefocusProcs((FskGLFocusProc)(&FskCocoaWindowBeginDraw), (FskGLDefocusProc)(&FskCocoaWindowEndDraw));
	#elif TARGET_OS_ANDROID
		#ifdef LOG_INIT
			LOGD("InitGlobalGLAssets: Calling FskEGLWindowContextInitialize");
		#endif /* LOG_INIT */
		BAIL_IF_ERR(err = FskEGLWindowContextInitialize(565, NULL));	/* We do this in case initialization is deferred */
		#ifdef LOG_INIT
			LOGD("InitGlobalGLAssets: FskEGLWindowContextInitialize succeeded; checking FskGLCapabilitiesGet");
		#endif /* LOG_INIT */
	#elif TARGET_OS_WIN32 && (FSK_OPENGLES_ANGLE == 1)
		BAIL_IF_ERR(err = FskEGLWindowContextInitialize(565, NULL));
	#elif TARGET_OS_KPL
		BAIL_IF_ERR(err = FskKplGLContextInitialize());
	#endif /* TARGET_OS */

	FskNotificationRegister(kFskNotificationLowMemory, purgeGL, NULL);
	gGLGlobalAssets.registeredLowMemoryWarning = true;

	/*
	 * Init proc pointers
	 */

	gGLGlobalAssets.blendFunctionSeparateProc = glBlendFuncSeparateHACK;	/* Make sure this field is initialized. Incorrect for alpha, but available everywhere. */
	err = FskGLCapabilitiesGet(&caps);										/* Will fail if GL has not yet been initialized; but we trudge on, regardless */
	PRINT_IF_ERROR(err, __LINE__, "FskGLCapabilitiesGet");
	BAIL_IF_ERR(err);
	#ifdef LOG_INIT
		LOGD("InitGlobalGLAssets: FskGLCapabilitiesGet succeeded: VEND=\"%s\", VERS=\"%s\", REND=\"%s\", GLSL=\"%s\"", caps->vendorStr, caps->versionStr, caps->rendererStr, caps->glslVersion);
	#endif /* LOG_INIT */

	/* Set "blend separate" proc */
	#if		defined(GL_VERSION_1_1) || defined(GL_VERSION_2_0) || defined(GL_ES_VERSION_2_0)
		gGLGlobalAssets.blendFunctionSeparateProc = (void (*)(GLenum, GLenum, GLenum, GLenum))glBlendFuncSeparate;
	#elif	defined(GL_VERSION_ES_CM_1_0)
		#if defined(GL_OES_blend_func_separate) && GL_OES_blend_func_separate
			if (FskGLCapabilitiesHas(caps, "GL_OES_blend_func_separate"))
				gGLGlobalAssets.blendFunctionSeparateProc = glBlendFuncSeparateOES;
		#endif /* GL_OES_blend_func_separate */
	#else /* GL_VERSION_UNKNOWN */
		#error "What version of GL?"
	#endif /* GL_VERSION */

	/*
	 * Determine hardware-supported pixel formats
	 */

	/* These formats we know about a priori */
	#if (defined(GL_VERSION_ES_CM_1_0) || defined(GL_ES_VERSION_2_0)) && TARGET_RT_LITTLE_ENDIAN
		gGLGlobalAssets.srcFormats[0]	= (1 << (UInt32)kFskBitmapFormat32RGBA)
										| (1 << (UInt32)kFskBitmapFormat24RGB)
										| (1 << (UInt32)kFskBitmapFormat16RGBA4444LE)
										| (1 << (UInt32)kFskBitmapFormat16RGB565LE)
										| (1 << (UInt32)kFskBitmapFormat16AG)
										| (1 << (UInt32)kFskBitmapFormat8G)
										| (1 << (UInt32)kFskBitmapFormat8A)
										| (1 << kFskBitmapFormatGLRGBA)
										| (1 << kFskBitmapFormatGLRGB)
										;
	#elif (defined(GL_VERSION_ES_CM_1_0) || defined(GL_ES_VERSION_2_0)) && TARGET_RT_BIG_ENDIAN
		gGLGlobalAssets.srcFormats[0]	= (1 << (UInt32)kFskBitmapFormat32RGBA)
										| (1 << (UInt32)kFskBitmapFormat24RGB)
										| (1 << (UInt32)kFskBitmapFormat16RGB565BE)
										| (1 << (UInt32)kFskBitmapFormat8G)
										| (1 << (UInt32)kFskBitmapFormat8A)
										| (1 << (UInt32)kFskBitmapFormatGLRGBA)
										| (1 << (UInt32)kFskBitmapFormatGLRGB)
										;
	#elif TARGET_OS_MAC && !TARGET_OS_IPHONE
		gGLGlobalAssets.srcFormats[0]	= (1 << (UInt32)kFskBitmapFormat32RGBA)
										| (1 << (UInt32)kFskBitmapFormat32BGRA)
										| (1 << (UInt32)kFskBitmapFormat32ARGB)
										| (1 << (UInt32)kFskBitmapFormat32ABGR)
										| (1 << (UInt32)kFskBitmapFormat24BGR)
										| (1 << (UInt32)kFskBitmapFormat24RGB)
										| (1 << (UInt32)kFskBitmapFormat16RGB565BE)
										| (1 << (UInt32)kFskBitmapFormat16RGB565LE)
										| (1 << (UInt32)kFskBitmapFormat16RGBA4444LE)
										| (1 << (UInt32)kFskBitmapFormat16AG)
										| (1 << (UInt32)kFskBitmapFormat8G)
										| (1 << (UInt32)kFskBitmapFormat8A)
										| (1 << (UInt32)kFskBitmapFormatGLRGBA)
										| (1 << (UInt32)kFskBitmapFormatGLRGB)
										;
	#else /* TARGET_OS_UNKNOWN */
		#error What OS?
	#endif /* TARGET_OS */

	/* Determine BGRA support */
	if (0 != (	FskGLCapabilitiesHas(caps, "GL_EXT_texture_format_BGRA8888") |
				FskGLCapabilitiesHas(caps, "GL_IMG_texture_format_BGRA8888") |
				FskGLCapabilitiesHas(caps, "GL_APPLE_texture_format_BGRA8888")
	))
		gGLGlobalAssets.srcFormats[0] |= 1 << (UInt32)kFskBitmapFormat32BGRA;

	/* Determine YUV support */
	#if !TEST_YUV_SHADERS
		#if TARGET_CPU_ARM && defined(EGL_VERSION)	/* We get non-NULL for production Vizio tablets, but no video comes through. Disabled until we get a better test. */
			/* Get Marvell's special video upload proc pointer */
			if ((NULL != (gGLGlobalAssets.platformYUVUploadProc = eglGetProcAddress("glTexImageVideo")))	&&
				((0 == FskStrCompareWithLength(caps->vendorStr, "Marvell", 7))
				#ifdef GC1000_GLTEXIMAGEVIDEO_IS_NOT_BROKEN
					||
				 (0 == FskStrCompareWithLength(caps->vendorStr, "Vivante", 7))
				#endif /* GC1000_GLTEXIMAGEVIDEO_IS_NOT_BROKEN */
				)
			) {
				gGLGlobalAssets.texImageYUV420 = SetMarvellTextureYUV420;
				gGLGlobalAssets.texImageUYVY   = SetMarvellTextureUYVY;
				gGLGlobalAssets.srcFormats[0]	|=	(1 << (UInt32)kFskBitmapFormatYUV420)
												|	(1 << (UInt32)kFskBitmapFormatUYVY);
				#ifdef LOG_YUV
					LOGD("Vendor string \"%s\" and glTexImageVideo=%p enabled Marvell YUV420 and UYVY", caps->vendorStr, gGLGlobalAssets.platformYUVUploadProc);
				#endif /* LOG_YUV */
			}
		#endif /* TARGET_CPU_ARM */

		#if defined(EGL_VERSION) && defined(BG3CDP_GL)
			gGLGlobalAssets.hasImageExternal = 1;//FskGLCapabilitiesHas(caps, "OES_EGL_image_external");
			if (gGLGlobalAssets.hasImageExternal
				// && ((0 == FskStrCompareWithLength(caps->vendorStr, "Marvell", 7)) || (0 == FskStrCompareWithLength(caps->vendorStr, "Vivante", 7)))
			) {
				#ifdef LOG_YUV
					LOGD("OES_EGL_image_external enabled");
				#endif /* LOG_YUV */
				gGLGlobalAssets.srcFormats[0] |= 1 << (UInt32)kFskBitmapFormatUYVY;
			}
			else LOGD("No OES_EGL_image_external found in %s", (const char*)glGetString(GL_EXTENSIONS));
		#endif /* BG3CDP_GL */

		/* Determine whether it supports YUV 4:2:2 UYVY */
		if (0 != FskStrCompareWithLength(caps->vendorStr, "Intel ", 6) && FskGLCapabilitiesHas(caps, "GL_APPLE_ycbcr_422")) {
			gGLGlobalAssets.srcFormats[0] |= (1 << (UInt32)kFskBitmapFormatUYVY);
			#if TARGET_OS_MAC && !TARGET_OS_IPHONE && !TEST_YUV_SHADERS
				gGLGlobalAssets.texImageUYVY = SetMacTextureUYVY;
			#endif /* TARGET_OS_MAC && !TARGET_OS_IPHONE && !TEST_YUV_SHADERS */
			#ifdef LOG_YUV
				LOGD("GL_APPLE_ycbcr_422 enabled");
			#endif /* LOG_YUV */
		}


	#endif /* TEST_YUV_SHADERS */

	/* Apple has a format that makes it easier to use YUV 4:2:2 */
	#if GLES_VERSION == 2 && GL_RGB_422_APPLE && (!TARGET_OS_IPHONE || GL_RGB_422_APPLE_WORKS_ON_IPHONE)	/* iPhone lies about its capabilities */
	if (FskGLCapabilitiesHas(caps, "GL_APPLE_rgb_422")) {
		gGLGlobalAssets.hasAppleRGB422 = true;
		gGLGlobalAssets.srcFormats[1] |= (1 << (UInt32)kFskBitmapFormatUYVY);
		#ifdef LOG_YUV
			LOGD("GL_APPLE_rgb_422 enabled");
		#endif /* LOG_YUV */
	}
	#endif /* GL_RGB_422_APPLE */

	/*
	 * Determine shader-supported pixel formats
	 */

	gGLGlobalAssets.srcFormats[1] |= gGLGlobalAssets.srcFormats[0];									/* We start off with the hardware formats */

	/* Check whether we can use the 4:2:0 shader */
	#if GLES_VERSION == 2
		gGLGlobalAssets.srcFormats[1] |= (1 << (UInt32)kFskBitmapFormatYUV420);						/* Our 4:2:0    planar   shader seems to work on all GPUs */
		gGLGlobalAssets.srcFormats[1] |= (1 << (UInt32)kFskBitmapFormatYUV420spuv);					/* Our 4:2:0 semi-planar shader seems to work on all GPUs */
		gGLGlobalAssets.srcFormats[1] |= (1 << (UInt32)kFskBitmapFormatYUV420spvu);					/* Our 4:2:0 semi-planar shader seems to work on all GPUs */
	#endif /* GLES_VERSION == 2 */

	/* Check whether we can use the UYVY 4:2:2 shader */
	#if defined(GL_VERSION_2_0) && (GLES_VERSION == 2)												/* GL 2 always has high precision in fragment shaders ... */
		gGLGlobalAssets.srcFormats[1] |= (1 << (UInt32)kFskBitmapFormatUYVY);						/* ... to support our UYVY shader */
		gGLGlobalAssets.hasHighPrecisionShaderFloat = true;
	#elif defined(GL_ES_VERSION_2_0)
		gGLGlobalAssets.hasHighPrecisionShaderFloat = FskGLCapabilitiesHas(caps, "GL_OES_fragment_precision_high");	/* If we have high precision in fragment shaders, ... */
		if (!gGLGlobalAssets.hasHighPrecisionShaderFloat) {											/* Extension is not listed - query the fragment precision directly */
			#ifdef GL_HIGH_FLOAT
				GLint highFragmentFloatRange[2]  = { 0, 0 };
				GLint highFragmentFloatPrecision = 0;
				glGetShaderPrecisionFormat(GL_FRAGMENT_SHADER, GL_HIGH_FLOAT, highFragmentFloatRange, &highFragmentFloatPrecision);
				if (highFragmentFloatPrecision >= 23)
					gGLGlobalAssets.hasHighPrecisionShaderFloat = true;
			#endif /* GL_HIGH_FLOAT */
		}
		if (gGLGlobalAssets.hasHighPrecisionShaderFloat)											/* If the fragment shader precision is high enough ... */
			gGLGlobalAssets.srcFormats[1] |= (1 << (UInt32)kFskBitmapFormatUYVY);					/* ... then our UYVY shader will work */
	#endif /* GL_VERSION */
	#ifdef LOG_YUV
	{	static const char *abled[2] = { "disabled", "enabled" };
		LOGD("Shaders: YUV420 %s, YUV420spuv %s, YUV420spvu %s, UYVY %s",
			((gGLGlobalAssets.srcFormats[1] & (1 << (UInt32)kFskBitmapFormatYUV420))     ? abled[1] : abled[0]),
			((gGLGlobalAssets.srcFormats[1] & (1 << (UInt32)kFskBitmapFormatYUV420spuv)) ? abled[1] : abled[0]),
			((gGLGlobalAssets.srcFormats[1] & (1 << (UInt32)kFskBitmapFormatYUV420spvu)) ? abled[1] : abled[0]),
			((gGLGlobalAssets.srcFormats[1] & (1 << (UInt32)kFskBitmapFormatUYVY))       ? abled[1] : abled[0])
		);
	}
	#endif /* LOG_YUV */


	/*
	 * Determine those formats that we can accommodate by losslessly converting in place.
	 */

	/* 32 bit formats */
	gGLGlobalAssets.srcFormats[2] |= gGLGlobalAssets.srcFormats[1];									/* We start off with hardware and shader formats */
	if (gGLGlobalAssets.srcFormats[2] & ( (1 << (UInt32)kFskBitmapFormat32RGBA)						/* If we can handle any 8888 pixel format, ... */
										| (1 << (UInt32)kFskBitmapFormat32BGRA)
										| (1 << (UInt32)kFskBitmapFormat32ARGB)
										| (1 << (UInt32)kFskBitmapFormat32ABGR))
	)
		gGLGlobalAssets.srcFormats[2]  |= (1 << (UInt32)kFskBitmapFormat32RGBA)						/* ... we can losslessly convert and handle any other 8888 ... */
										| (1 << (UInt32)kFskBitmapFormat32BGRA)
										| (1 << (UInt32)kFskBitmapFormat32ARGB)
										| (1 << (UInt32)kFskBitmapFormat32ABGR)
										| (1 << (UInt32)kFskBitmapFormat32A16RGB565LE);				/* ... or 32A16RGB565LE */

	/* 24 bit formats */
	if (gGLGlobalAssets.srcFormats[2] & ( (1 << (UInt32)kFskBitmapFormat24RGB)						/* If we can handle any 888 pixel format, ... */
										| (1 << (UInt32)kFskBitmapFormat24BGR))
	) {
		gGLGlobalAssets.srcFormats[2]	|= (1 << (UInt32)kFskBitmapFormat24RGB)						/* If we can handle any 888 pixel format, ... */
										|  (1 << (UInt32)kFskBitmapFormat24BGR);					/* ... we can losslessly convert and handle any other 888 */
	}


	/*
	 * Determine those formats that we can accommodate by allocating a temporary buffer.
	 */
	gGLGlobalAssets.srcFormats[3] |= gGLGlobalAssets.srcFormats[2];									/* Start off with hardware, shader, and in-place-converted formats */
	#if GLES_VERSION == 2
		if (gGLGlobalAssets.srcFormats[3] & (1 << (UInt32)kFskBitmapFormatYUV420))
			gGLGlobalAssets.srcFormats[3] |= 1 << (UInt32)kFskBitmapFormatYUV420i;
	#endif /* GLES_VERSION == 2 */


	#ifdef LOG_INIT
	{	UInt32 i;
		for (i = kFskBitmapFormat24BGR; i <= kFskBitmapFormat32A16RGB565LE; ++i)
			if (FskGLHardwareSupportsPixelFormat(i))
				LOGD("InitGlobalGLAssets: has hardware %s", FskBitmapFormatName(i));
	}
	#endif /* LOG_INIT */

	/*
	 * Determine other capabilities
	 */

	/* Determine whether it supports non-power-of-two textures with the repeat and mirrored repeat wrap modes */
	gGLGlobalAssets.hasRepeatNPOT =	FskGLCapabilitiesHas(caps, "GL_OES_texture_npot")				/* NPOT with mipmap and clamp and repeat and mirrored repeat */
								  | FskGLCapabilitiesHas(caps, "GL_ARB_texture_non_power_of_two")	/* NPOT with mipmap and clamp and repeat and mirrored repeat */
								  ;
	if (gGLGlobalAssets.hasRepeatNPOT && FskStrCompare(caps->rendererStr, "Adreno 200"))
		gGLGlobalAssets.hasRepeatNPOT = 0;	/* Adreno 200 lies about being able to do REPEAT and MIRROR_REPEAT texture wrapping */
	/* Determine whether it supports non-power-of-two textures at all */
	#if GLES_VERSION == 2
		gGLGlobalAssets.hasNPOT = true;
	#else /* GLES_VERSION == 1 */
		gGLGlobalAssets.hasNPOT = gGLGlobalAssets.hasRepeatNPOT										/* NPOT with mipmap and clamp and repeat and mirrored repeat */
								| FskGLCapabilitiesHas(caps, "GL_IMG_texture_npot")					/* NPOT with mipmap and clamp */
								| FskGLCapabilitiesHas(caps, "GL_APPLE_texture_2D_limited_npot")	/* NPOT with clamp */
								;
	#endif /* GLES_VERSION == 1 */
	#ifdef LOG_INIT
		LOGD("InitGlobalGLAssets: %s NPOT", gGLGlobalAssets.hasNPOT ? "has" : "doesn't have");
		LOGD("InitGlobalGLAssets: %s Repeat NPOT", gGLGlobalAssets.hasRepeatNPOT ? "has" : "doesn't have");
	#endif /* LOG_INIT */

	/* Determine alternate pixel read format */
	gGLGlobalAssets.readFormats = (1 << (UInt32)kFskBitmapFormat32RGBA);							/* Everyone is required to do RGBA */
	{	GLint readFormat = 0, readType = 0;
		#if       defined(GL_IMPLEMENTATION_COLOR_READ_FORMAT)
			glGetIntegerv(GL_IMPLEMENTATION_COLOR_READ_FORMAT,     &readFormat);
		#elif     defined(GL_IMPLEMENTATION_COLOR_READ_FORMAT_OES)
			glGetIntegerv(GL_IMPLEMENTATION_COLOR_READ_FORMAT_OES, &readFormat);
		#endif /* GL_IMPLEMENTATION_COLOR_READ_FORMAT */
		#if       defined(GL_IMPLEMENTATION_COLOR_READ_TYPE)
			glGetIntegerv(GL_IMPLEMENTATION_COLOR_READ_TYPE,       &readType);
		#elif     defined(GL_IMPLEMENTATION_COLOR_READ_TYPE_OES)
			glGetIntegerv(GL_IMPLEMENTATION_COLOR_READ_TYPE_OES,   &readType);
		#endif /* GL_IMPLEMENTATION_COLOR_READ_TYPE */
		switch (readType) {
			case GL_UNSIGNED_BYTE:			switch (readFormat) {
				case GL_RGBA:		gGLGlobalAssets.readFormats |= 1 << (UInt32)kFskBitmapFormat32RGBA;					break;
				#ifdef GL_BGRA
					case GL_BGRA:	gGLGlobalAssets.readFormats |= 1 << (UInt32)kFskBitmapFormat32BGRA;					break;
				#endif /* GL_BGRA */
					default:		LOGD("Unexpected read format=%d, type=GL_UNSIGNED_BYTE", readFormat);				break;
				} break;
			case GL_UNSIGNED_SHORT_5_6_5:	switch (readFormat) {
				case GL_RGB:
					#if TARGET_CPU_ARM
						if (0 == FskStrCompareWithLength(caps->vendorStr, "Marvell", 7))
							break;	/* Marvell lies about being able to read 16RGB565 */
					#endif /* TARGET_CPU_ARM */
					#if TARGET_RT_LITTLE_ENDIAN
									gGLGlobalAssets.readFormats |= 1 << (UInt32)kFskBitmapFormat16RGB565LE;				break;
					#else /* TARGET_RT_BIG_ENDIAN */
									gGLGlobalAssets.readFormats |= 1 << (UInt32)kFskBitmapFormat16RGB565BE;				break;
					#endif /* TARGET_RT_XXX_ENDIAN */
				default:			LOGD("Unexpected read format=%d, type=GL_UNSIGNED_SHORT_5_6_5", readFormat);		break;
				} break;
			default:	if (readFormat && readType)	LOGD("Unexpected read format=%d, type=%d", readFormat, readType);	break;
		}
	}
	if (FskGLCapabilitiesHas(caps, "GL_EXT_read_format_bgra")	||	/* 32BGRA, 16ARGB4444SE, 16ARGB1555SE */
		FskGLCapabilitiesHas(caps, "GL_IMG_read_format")			/* 32BGRA, 16ARGB4444SE               */
	) {
		if (FskGLHardwareSupportsPixelFormat(kFskBitmapFormat32BGRA))
			gGLGlobalAssets.readFormats |= 1 << (UInt32)kFskBitmapFormat32BGRA;
	}

	/* Get the maximum texture size */
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &gGLGlobalAssets.maxTextureSize);
	if (gGLGlobalAssets.maxTextureSize > 4096)	/* Don't believe it */
		gGLGlobalAssets.maxTextureSize = 4096;


	/* Determine whether the GPU has a reliable FrameBufferObject implementation */
	#if TARGET_OS_ANDROID
		gGLGlobalAssets.hasFBO = LookupRendererInfo(caps->rendererStr)->hasFBO;
		#if GL_DEBUG
			if (NULL == LookupRendererInfo(caps->rendererStr)->name)
				LOGE("Renderer %s is not in the renderer table", caps->rendererStr);
		#endif /* GL_DEBUG */
	#elif TARGET_OS_IPHONE || TARGET_OS_MAC									/* Verified */
		gGLGlobalAssets.hasFBO = true;
	#elif TARGET_OS_KPL || defined(__linux__) || (FSK_OPENGLES_ANGLE == 1)	/* Unverified */
		gGLGlobalAssets.hasFBO = true;
	#else
		#error no renderer info for this platform.
	#endif /* TARGET_OS_ANDROID */
	#if GLES_VERSION == 2
		if (0 && gGLGlobalAssets.hasFBO) {														/* If we suspect that FBOs are available ...	*/
			glGetError();																		/* Clear prior errors */
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);	/* ... try executing ...						*/
			glBindFramebuffer(GL_FRAMEBUFFER, gGLGlobalAssets.defaultContext.defaultFBO);		/* ... some framebuffer calls ...				*/
			err = glGetError();
			if (err) {																			/* ... and if there was an error...				*/
				LOGD("InitGlobalGLAssets: %s is in the FBO white list, but glFramebufferTexture2D failed = %#x", caps->rendererStr, (int)err);
				gGLGlobalAssets.hasFBO = false;													/* ... void FBOs because they are not supported	*/
			}
		}
		#ifdef LOG_INIT
			LOGD("InitGlobalGLAssets: %s FBO", gGLGlobalAssets.hasFBO ? "has" : "doesn't have");
		#endif /* LOG_INIT */
	#endif /* GLES_VERSION == 2 */


	/* Determine whether the GPU renders the whole frame at once, or does it in tiles. */
	gGLGlobalAssets.rendererIsTiled = FskGLCapabilitiesHas(caps, "GL_QCOM_tiled_rendering");


	#ifdef TRACK_TEXTURES
		if (kFskErrNone != FskGrowableArrayNew(sizeof(GLuint), 100, &gGLGlobalAssets.glTextures))
			gGLGlobalAssets.glTextures = NULL;
	#endif /* TRACK_TEXTURES */

	/*
	 * Initialize texture object cache.
	 */


	#if EPHEMERAL_TEXTURE_CACHE_SIZE > 0
		#ifdef LOG_INIT
			LOGD("InitGlobalGLAssets: generating textures for texture object cache");
		#endif /* LOG_INIT */

		err = GetFskGLError();
		PRINT_IF_ERROR(err, __LINE__, "Just before glGenTextures");
		glGenTextures(EPHEMERAL_TEXTURE_CACHE_SIZE, gGLGlobalAssets.textureObjectCache);
		#ifdef TRACK_TEXTURES
			TrackTextures(EPHEMERAL_TEXTURE_CACHE_SIZE, gGLGlobalAssets.glTextures);
		#endif /* TRACK_TEXTURES */

		err = GetFskGLError();
		#ifdef LOG_INIT
			PRINT_IF_ERROR(err, __LINE__, "glGenTextures");
			{	int i;
				for (i = 0; i < EPHEMERAL_TEXTURE_CACHE_SIZE; ++i)
					LOGD("Initializing ephemeral texture object cache #%u", gGLGlobalAssets.textureObjectCache[i]);
			}
		#endif /* LOG_INIT */
		if (err)
			LOGE("glGenTextures failed (%d)", (int)err);
		BAIL_IF_ERR(err);
		/* Initialize textures */
		err = InitializeTextures(EPHEMERAL_TEXTURE_CACHE_SIZE, gGLGlobalAssets.textureObjectCache);
		if (err)
			LOGE("InitializeTextures failed (%d)", (int)err);
		BAIL_IF_ERR(err);
	#endif /* EPHEMERAL_TEXTURE_CACHE_SIZE */


	/*
	 * Initialize shaders
	 */


	#ifdef LOG_INIT
		LOGD("InitGlobalGLAssets: Starting to initialize shaders");
	#endif /* LOG_INIT */

	#if GLES_VERSION == 2
	{	static const char	strOpColor[]	= "opColor";
		static const char	strMatrix[]		= "matrix";
		static const char	strSrcBM[]		= "srcBM";
		static const char	strVPosition[]	= "vPosition";
		static const char	strVTexCoord[]	= "vTexCoord";
		static const char	strYtex[]		= "yTex";
		static const char	strUtex[]		= "uTex";
		static const char	strVtex[]		= "vTex";
		static const char	strUVtex[]		= "uvTex";
		static const char	strColorMtx[]	= "colorMtx";
		static const char	strTexDim[]		= "texDim";
		static const char	strOffColor[]	= "offColor";
		static const char	strTileSpace[]	= "tileSpace";
		GLuint				vertexShader, fragmentShader;

		#ifdef LOG_INIT
			LOGD("InitGlobalGLAssets: Compiling shaders, linking programs, getting uniform location, and initializing uniform variables");
		#endif /* LOG_INIT */

		/* Fill shader */
		BAIL_IF_ERR(err = FskGLNewShader(gFillVertexShader,	GL_VERTEX_SHADER,   &vertexShader));
		BAIL_IF_ERR(err = FskGLNewShader(gFillShader,		GL_FRAGMENT_SHADER, &fragmentShader));
		BAIL_IF_ERR(err = FskGLNewProgram(vertexShader, fragmentShader, &gGLGlobalAssets.fill.program, VERTEX_POSITION_INDEX, strVPosition, VERTEX_END_INDEX));
		glDeleteShader(fragmentShader);
		glDeleteShader(vertexShader);
		BAIL_IF_NEGATIVE(gGLGlobalAssets.fill.opColor	= glGetUniformLocation(gGLGlobalAssets.fill.program, strOpColor  ), err, kFskErrGLProgram);
		BAIL_IF_NEGATIVE(gGLGlobalAssets.fill.matrix	= glGetUniformLocation(gGLGlobalAssets.fill.program, strMatrix   ), err, kFskErrGLProgram);

		BAIL_IF_ERR(err = FskGLNewShader(gDrawBitmapVertexShader,  GL_VERTEX_SHADER,   &vertexShader));

		/* DrawBitmap shader */
		BAIL_IF_ERR(err = FskGLNewShader(gDrawBitmapShader,	GL_FRAGMENT_SHADER, &fragmentShader));
		BAIL_IF_ERR(err = FskGLNewProgram(vertexShader, fragmentShader, &gGLGlobalAssets.drawBitmap.program, VERTEX_POSITION_INDEX, strVPosition, VERTEX_TEXCOORD_INDEX, strVTexCoord, VERTEX_END_INDEX));
		glDeleteShader(fragmentShader);
		BAIL_IF_NEGATIVE(gGLGlobalAssets.drawBitmap.opColor	= glGetUniformLocation(gGLGlobalAssets.drawBitmap.program, strOpColor  ), err, kFskErrGLProgram);
		BAIL_IF_NEGATIVE(gGLGlobalAssets.drawBitmap.matrix	= glGetUniformLocation(gGLGlobalAssets.drawBitmap.program, strMatrix   ), err, kFskErrGLProgram);
		BAIL_IF_NEGATIVE(gGLGlobalAssets.drawBitmap.srcBM	= glGetUniformLocation(gGLGlobalAssets.drawBitmap.program, strSrcBM    ), err, kFskErrGLProgram);

		/* TileBitmap shader */
		BAIL_IF_ERR(err = FskGLNewShader(gTileBitmapShader,		GL_FRAGMENT_SHADER, &fragmentShader));
		BAIL_IF_ERR(err = FskGLNewProgram(vertexShader, fragmentShader, &gGLGlobalAssets.tileBitmap.program, VERTEX_POSITION_INDEX, strVPosition, VERTEX_TEXCOORD_INDEX, strVTexCoord, VERTEX_END_INDEX));
		glDeleteShader(fragmentShader);
		BAIL_IF_NEGATIVE(gGLGlobalAssets.tileBitmap.opColor		= glGetUniformLocation(gGLGlobalAssets.tileBitmap.program, strOpColor  ), err, kFskErrGLProgram);
		BAIL_IF_NEGATIVE(gGLGlobalAssets.tileBitmap.matrix		= glGetUniformLocation(gGLGlobalAssets.tileBitmap.program, strMatrix   ), err, kFskErrGLProgram);
		BAIL_IF_NEGATIVE(gGLGlobalAssets.tileBitmap.tileSpace	= glGetUniformLocation(gGLGlobalAssets.tileBitmap.program, strTileSpace), err, kFskErrGLProgram);
		BAIL_IF_NEGATIVE(gGLGlobalAssets.tileBitmap.srcBM		= glGetUniformLocation(gGLGlobalAssets.tileBitmap.program, strSrcBM    ), err, kFskErrGLProgram);

		/* Draw Brightness Contrast Bitmap shader */
		BAIL_IF_ERR(err = FskGLNewShader(gDrawBCBitmapShader,	GL_FRAGMENT_SHADER, &fragmentShader));
		BAIL_IF_ERR(err = FskGLNewProgram(vertexShader, fragmentShader, &gGLGlobalAssets.drawBCBitmap.program, VERTEX_POSITION_INDEX, strVPosition, VERTEX_TEXCOORD_INDEX, strVTexCoord, VERTEX_END_INDEX));
		glDeleteShader(fragmentShader);
		BAIL_IF_NEGATIVE(gGLGlobalAssets.drawBCBitmap.opColor	= glGetUniformLocation(gGLGlobalAssets.drawBCBitmap.program,	strOpColor  ), err, kFskErrGLProgram);
		BAIL_IF_NEGATIVE(gGLGlobalAssets.drawBCBitmap.offColor	= glGetUniformLocation(gGLGlobalAssets.drawBCBitmap.program,	strOffColor ), err, kFskErrGLProgram);
		BAIL_IF_NEGATIVE(gGLGlobalAssets.drawBCBitmap.matrix	= glGetUniformLocation(gGLGlobalAssets.drawBCBitmap.program,	strMatrix   ), err, kFskErrGLProgram);
		BAIL_IF_NEGATIVE(gGLGlobalAssets.drawBCBitmap.srcBM		= glGetUniformLocation(gGLGlobalAssets.drawBCBitmap.program,	strSrcBM    ), err, kFskErrGLProgram);

		/* Transfer Alpha shader */
		BAIL_IF_ERR(err = FskGLNewShader(gTransferAlphaShader,	GL_FRAGMENT_SHADER, &fragmentShader));
		BAIL_IF_ERR(err = FskGLNewProgram(vertexShader, fragmentShader, &gGLGlobalAssets.transferAlpha.program, VERTEX_POSITION_INDEX, strVPosition, VERTEX_TEXCOORD_INDEX, strVTexCoord, VERTEX_END_INDEX));
		glDeleteShader(fragmentShader);
		BAIL_IF_NEGATIVE(gGLGlobalAssets.transferAlpha.opColor	= glGetUniformLocation(gGLGlobalAssets.transferAlpha.program,	strOpColor  ), err, kFskErrGLProgram);
		BAIL_IF_NEGATIVE(gGLGlobalAssets.transferAlpha.matrix	= glGetUniformLocation(gGLGlobalAssets.transferAlpha.program,	strMatrix   ), err, kFskErrGLProgram);
		BAIL_IF_NEGATIVE(gGLGlobalAssets.transferAlpha.srcBM	= glGetUniformLocation(gGLGlobalAssets.transferAlpha.program,	strSrcBM    ), err, kFskErrGLProgram);

		/* YUV 4:2:0 shader */
		BAIL_IF_ERR(err = FskGLNewShader(gYUV420Shader,		GL_FRAGMENT_SHADER, &fragmentShader));
		BAIL_IF_ERR(err = FskGLNewProgram(vertexShader, fragmentShader, &gGLGlobalAssets.yuv420.program, VERTEX_POSITION_INDEX, strVPosition, VERTEX_TEXCOORD_INDEX, strVTexCoord, VERTEX_END_INDEX));
		glDeleteShader(fragmentShader);
		BAIL_IF_NEGATIVE(gGLGlobalAssets.yuv420.matrix		= glGetUniformLocation(gGLGlobalAssets.yuv420.program, strMatrix   ), err, kFskErrGLProgram);
		BAIL_IF_NEGATIVE(gGLGlobalAssets.yuv420.yTex		= glGetUniformLocation(gGLGlobalAssets.yuv420.program, strYtex     ), err, kFskErrGLProgram);
		BAIL_IF_NEGATIVE(gGLGlobalAssets.yuv420.uTex		= glGetUniformLocation(gGLGlobalAssets.yuv420.program, strUtex     ), err, kFskErrGLProgram);
		BAIL_IF_NEGATIVE(gGLGlobalAssets.yuv420.vTex		= glGetUniformLocation(gGLGlobalAssets.yuv420.program, strVtex     ), err, kFskErrGLProgram);
		BAIL_IF_NEGATIVE(gGLGlobalAssets.yuv420.colorMtx	= glGetUniformLocation(gGLGlobalAssets.yuv420.program, strColorMtx ), err, kFskErrGLProgram);

		/* YUV 4:2:0 SP shader */
		BAIL_IF_ERR(err = FskGLNewShader(gYUV420spShader,	GL_FRAGMENT_SHADER, &fragmentShader));
		BAIL_IF_ERR(err = FskGLNewProgram(vertexShader, fragmentShader, &gGLGlobalAssets.yuv420sp.program, VERTEX_POSITION_INDEX, strVPosition, VERTEX_TEXCOORD_INDEX, strVTexCoord, VERTEX_END_INDEX));
		glDeleteShader(fragmentShader);
		BAIL_IF_NEGATIVE(gGLGlobalAssets.yuv420sp.matrix	= glGetUniformLocation(gGLGlobalAssets.yuv420sp.program, strMatrix   ), err, kFskErrGLProgram);
		BAIL_IF_NEGATIVE(gGLGlobalAssets.yuv420sp.yTex		= glGetUniformLocation(gGLGlobalAssets.yuv420sp.program, strYtex     ), err, kFskErrGLProgram);
		BAIL_IF_NEGATIVE(gGLGlobalAssets.yuv420sp.uvTex		= glGetUniformLocation(gGLGlobalAssets.yuv420sp.program, strUVtex    ), err, kFskErrGLProgram);
		BAIL_IF_NEGATIVE(gGLGlobalAssets.yuv420sp.colorMtx	= glGetUniformLocation(gGLGlobalAssets.yuv420sp.program, strColorMtx ), err, kFskErrGLProgram);

		/* YUV 4:4:4 shader */
#ifdef GL_RGB_422_APPLE
		BAIL_IF_ERR(err = FskGLNewShader(gYUV444Shader,		GL_FRAGMENT_SHADER, &fragmentShader));
		BAIL_IF_ERR(err = FskGLNewProgram(vertexShader, fragmentShader, &gGLGlobalAssets.yuv444.program, VERTEX_POSITION_INDEX, strVPosition, VERTEX_TEXCOORD_INDEX, strVTexCoord, VERTEX_END_INDEX));
		glDeleteShader(fragmentShader);
		BAIL_IF_NEGATIVE(gGLGlobalAssets.yuv444.matrix		= glGetUniformLocation(gGLGlobalAssets.yuv444.program, strMatrix   ), err, kFskErrGLProgram);
		BAIL_IF_NEGATIVE(gGLGlobalAssets.yuv444.srcBM		= glGetUniformLocation(gGLGlobalAssets.yuv444.program, strSrcBM    ), err, kFskErrGLProgram);
		BAIL_IF_NEGATIVE(gGLGlobalAssets.yuv444.colorMtx	= glGetUniformLocation(gGLGlobalAssets.yuv444.program, strColorMtx ), err, kFskErrGLProgram);
#endif /* GL_RGB_422_APPLE */


#if defined(EGL_VERSION) && defined(BG3CDP_GL)
		if (gGLGlobalAssets.hasImageExternal) {
			do {
				/* Draw Brightness Contrast Bitmap for BG3CDP shader */
				if (kFskErrNone != (err = FskGLNewShader(gDrawBG3CDPBitmapShader, GL_FRAGMENT_SHADER, &fragmentShader))) break;
				err = FskGLNewProgram(vertexShader, fragmentShader, &gGLGlobalAssets.drawBG3CDPBitmap.program, VERTEX_POSITION_INDEX, strVPosition, VERTEX_TEXCOORD_INDEX, strVTexCoord, VERTEX_END_INDEX);
				glDeleteShader(fragmentShader);
				if (kFskErrNone != err) break;
				gGLGlobalAssets.drawBG3CDPBitmap.opColor	= glGetUniformLocation(gGLGlobalAssets.drawBG3CDPBitmap.program, strOpColor  );
				gGLGlobalAssets.drawBG3CDPBitmap.offColor	= glGetUniformLocation(gGLGlobalAssets.drawBG3CDPBitmap.program, strOffColor );
				gGLGlobalAssets.drawBG3CDPBitmap.matrix		= glGetUniformLocation(gGLGlobalAssets.drawBG3CDPBitmap.program, strMatrix   );
				gGLGlobalAssets.drawBG3CDPBitmap.srcBM		= glGetUniformLocation(gGLGlobalAssets.drawBG3CDPBitmap.program, strSrcBM    );
				//LOGD("InitGlobalGLAssets: Successfully initialized BG3CDP shader");
			} while(0);
			if (err												||
				(gGLGlobalAssets.drawBG3CDPBitmap.opColor  < 0)	||
				(gGLGlobalAssets.drawBG3CDPBitmap.offColor < 0)	||
				(gGLGlobalAssets.drawBG3CDPBitmap.matrix   < 0)	||
				(gGLGlobalAssets.drawBG3CDPBitmap.srcBM    < 0)
			) {
				DisposeProgram(&gGLGlobalAssets.drawBG3CDPBitmap.program);
				gGLGlobalAssets.hasImageExternal = false;
				//LOGD("InitGlobalGLAssets: Failed to initialize BG3CDP shader: %s", GLErrorStringFromCode(err));
				err = kFskErrNone;
			}
		}
#endif /* defined(EGL_VERSION) && defined(BG3CDP_GL) */

		glDeleteShader(vertexShader);

		BAIL_IF_ERR(err = FskGLNewShader(gDraw422VertexShader, GL_VERTEX_SHADER,   &vertexShader));	/* Use a special vertex shader */
		BAIL_IF_ERR(err = FskGLNewShader(gYUV422Shader, GL_FRAGMENT_SHADER, &fragmentShader));
		BAIL_IF_ERR(err = FskGLNewProgram(vertexShader, fragmentShader, &gGLGlobalAssets.yuv422.program, VERTEX_POSITION_INDEX, strVPosition, VERTEX_TEXCOORD_INDEX, strVTexCoord, VERTEX_END_INDEX));
		BAIL_IF_NEGATIVE(gGLGlobalAssets.yuv422.matrix		= glGetUniformLocation(gGLGlobalAssets.yuv422.program, strMatrix   ), err, kFskErrGLProgram);
		BAIL_IF_NEGATIVE(gGLGlobalAssets.yuv422.srcBM		= glGetUniformLocation(gGLGlobalAssets.yuv422.program, strSrcBM    ), err, kFskErrGLProgram);
		BAIL_IF_NEGATIVE(gGLGlobalAssets.yuv422.texDim		= glGetUniformLocation(gGLGlobalAssets.yuv422.program, strTexDim   ), err, kFskErrGLProgram);
		BAIL_IF_NEGATIVE(gGLGlobalAssets.yuv422.colorMtx	= glGetUniformLocation(gGLGlobalAssets.yuv422.program, strColorMtx ), err, kFskErrGLProgram);
		glDeleteShader(fragmentShader);
		glDeleteShader(vertexShader);

		FskMemSet(gGLGlobalAssets.matrix, 0, sizeof(gGLGlobalAssets.matrix));

		#ifdef LOG_INIT
			LOGD("InitGlobalGLAssets: GPU Program initialization complete");
		#endif /* LOG_INIT */
	}
	#endif /* GLES_VERSION == 2 */

	/* Allocate global coordinate array */
	if (gGLGlobalAssets.coordinates == NULL) {
		gGLGlobalAssets.coordinatesBytes = sizeof(GLCoordinateType) * 4096;
		BAIL_IF_ERR(err = FskMemPtrNew(gGLGlobalAssets.coordinatesBytes, &gGLGlobalAssets.coordinates));
	}

	InitContextState();

	err = FskGLContextNewFromCurrentContext(&gGLGlobalAssets.defaultContext.glContext);	/* Record the current context */
	PRINT_IF_ERROR(err, __LINE__, "FskGLContextNewFromCurrentContext");
	BAIL_IF_ERR(err);
	gGLGlobalAssets.defaultContext.glContextMine = true;								/* Remember to deallocate this context when shutting down */


	inited = true;
	#ifdef LOG_INIT
		LOGD("InitGlobalGLAssets  -- initialization successful");
	#endif /* LOG_INIT */

	#if GL_DEBUG
		if (gGLGlobalAssets.activePorts != NULL) {
			FskGLPort p;
			LOGD("InitGlobalGLAssets  -- activePorts NOT NULL");
			for (p = gGLGlobalAssets.activePorts; p != NULL; p = p->next) {
				LOGD("ActiveGLPort(%p): (%u,%u) tex(w=%u h=%u if=%u name={%u %u %u} bm=%p ft=%#x wr=%d fl=%d)",
					p,
					(unsigned)p->portWidth,
					(unsigned)p->portHeight,
					(unsigned)p->texture.bounds.width,
					(unsigned)p->texture.bounds.height,
					p->texture.glIntFormat,
					p->texture.name,
					p->texture.nameU,
					p->texture.nameV,
					p->texture.srcBM,
					p->texture.filter,
					p->texture.wrapMode,
					p->texture.flipY
				);
			}
		}
	#endif /* GL_DEBUG */

	#if 0 && TARGET_OS_ANDROID
		LoadAllUnloadedTextures();	/* Load deferred and unloaded textures */
	#endif /* TARGET_OS */

	(void)glGetError();	/* Clear sticky error */

bail:
	FskMemPtrDispose(caps);
	if (inited) {
		gGLGlobalAssets.isInited = 1;			/* Indicate that the it has been initialized */
#if 0
		if (0 != atexit(DisposeGlobalGLAssets))	/* Register cleanup proc, to be called at exit. */
			err = errno;
#endif
	}
	else {										/* If anything failed ... */
		DisposeGLObjects();						/* ... go back to square one */
	}

	#ifdef LOG_INIT
		if (err)
			LOGD("[%s] InitGlobalGLAssets  -- initialization FAILED (%d)", FskThreadName(FskThreadGetCurrent()), (int)err);
	#endif /* LOG_INIT */

	return err;
}
#ifdef GL_TRACE
	#define InitGlobalGLAssets(v)	(GL_TRACE_LOG("InitGlobalGLAssets"),	InitGlobalGLAssets(v))
#endif /* GL_TRACE */


/****************************************************************************//**
 * FskGLResetAllState
 *	\param[in]	port		the GL port to be set.
 *	\return		kFskErrNone	if all of the state was set successfully.
 ********************************************************************************/

FskErr FskGLResetAllState(FskConstGLPort port) {
	FskErr	err = kFskErrNone;

	/* Set clear color */
	#if GL_COORD_TYPE_ENUM == GL_FLOAT
		glClearColor(0.f, 0.f, 0.f, 0.f);																		/* Reset clear color */
	#else /* GL_COORD_TYPE_ENUM == GL_FIXED */
		glClearColorx(0, 0, 0, 0);
	#endif /* GL_COORD_TYPE_ENUM == GL_FIXED */

	#if defined(GL_VERSION_1_1) || defined(GL_VERSION_2_0)
		glDrawBuffer(GL_FRONT);
	#endif /* GL */
	(void)glGetError();																							/* Clear errors */
	#ifdef GL_LIGHTING
		glDisable(GL_LIGHTING);
	#endif /* GL_LIGHTING */
	#ifdef GL_FLAT
		glShadeModel(GL_FLAT);
	#endif /* GL_FLAT */
	#if GLES_VERSION < 2
		glEnable(GL_TEXTURE_2D);
	#endif /* GL 1 */
	SET_GLOBAL_SCISSOR_ENABLE(false);																			/* Reset scissoring */
	SET_GLOBAL_BLEND_ENABLE(false);	gGLGlobalAssets.blitContext->blendHash   = 0;								/* Reset blending */
	glActiveTexture(GL_TEXTURE0);	gGLGlobalAssets.blitContext->lastTexture = 0;								/* Reset texture */
	BAIL_IF_ERR(err = GetFskGLError());
	#if GLES_VERSION == 2
		gGLGlobalAssets.blitContext->lastProgram = 0;															/* Reset program */
//		gGLGlobalAssets.matrixSeed++;	// This is done in FskGLSetGLView() below
	#endif /* GLES_VERSION == 2 */

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);	/* Row alignment for transferring images to the GPU. */

	if (gGLGlobalAssets.coordinates) {
		#if GLES_VERSION == 2
			glVertexAttribPointer(VERTEX_POSITION_INDEX, 2, GL_COORD_TYPE_ENUM, GL_FALSE, sizeof(GLTexVertex), gGLGlobalAssets.coordinates + TEXVERTEX_X);
			glVertexAttribPointer(VERTEX_TEXCOORD_INDEX, 2, GL_COORD_TYPE_ENUM, GL_FALSE, sizeof(GLTexVertex), gGLGlobalAssets.coordinates + TEXVERTEX_U);
		#else /* GLES_VERSION == 1 */
			glVertexPointer  (2, GL_COORD_TYPE_ENUM, sizeof(GLTexVertex), gGLGlobalAssets.coordinates + TEXVERTEX_X);
			glTexCoordPointer(2, GL_COORD_TYPE_ENUM, sizeof(GLTexVertex), gGLGlobalAssets.coordinates + TEXVERTEX_U);
		#endif /* GLES_VERSION == 1 */
	}

	BAIL_IF_ERR(err = FskGLSetGLView((FskGLPort)port));															/* Reset view: this does ++gGLGlobalAssets.matrixSeed */

bail:
	#if GL_DEBUG
		if (err)
			LOGD("[%s] FskGLResetAllState FAILED (%d)", FskThreadName(FskThreadGetCurrent()), (int)err);
	#endif /* GL_DEBUG */
	return err;
}


static const UInt8 alTab[8] = { 8, 1, 2, 1, 4, 1, 2, 1 };	/* Alignment table, used by ComputeAlignment() below. */

/****************************************************************************//**
 * Compute alignment of an image.
 * This returns either 1, 2, 4, or 8.
 *	\param[in]		baseAddr	the base address of the bitmap.
 *	\param[in]		rowBytes	y-stride.
 ********************************************************************************/
#define ComputeAlignment(baseAddr, rowBytes)	alTab[((unsigned)(baseAddr) | (rowBytes)) & 7]


/****************************************************************************//**
 * Make vertices for a polygon corresponding to the given rectangle.
 *	\param[in]		r			the rectangle.
 *	\param[in,out]	vertices	array of 8 GLCoordinateType numbers, to store four 2D points.
 ********************************************************************************/

static void MakeTexRectVertices(FskConstRectangle r)
{
	GLTexVertex			*vertex = (GLTexVertex*)(gGLGlobalAssets.coordinates);
	GLCoordinateType	x0		= IntToGLCoord(r->x),
						x1		= IntToGLCoord(r->x + r->width),
						y0		= IntToGLCoord(r->y),
						y1		= IntToGLCoord(r->y + r->height);
	vertex[0].x = x0;		vertex[0].y = y0;
	vertex[1].x = x0;		vertex[1].y = y1;
	vertex[2].x = x1;		vertex[2].y = y1;
	vertex[3].x = x1;		vertex[3].y = y0;
}


/****************************************************************************//**
 * Scale a rectangle, producing vertices.
 *	\param[in]	scaleOffset	scale and offset for the transformation.
 *							Note that scale is represented as 24.8, whereas offset is represented as 16.16.
 *	\param[in]	r			the bounds of the source texture.
 *	\return		kFskErrNone	if the operation completed successfully.
 ********************************************************************************/

static void ScaleOffsetRectVertices(const FskScaleOffset *scaleOffset, FskConstRectangle r)
{
	GLTexVertex			*vertex = (GLTexVertex*)(gGLGlobalAssets.coordinates);

	#if GL_COORD_TYPE_ENUM == GL_FLOAT
		float	x0, x1, y0, y1;
		x1 = scaleOffset->scaleX   * (1.f/((float)(1 << kFskScaleBits)));	/* x1 here holds scaleX in floating point */
		y1 = scaleOffset->scaleY   * (1.f/((float)(1 << kFskScaleBits)));	/* y1 here holds scaleY in floating point */
		x0 = scaleOffset->offsetX  * (1.f/((float)(1 << kFskOffsetBits)));	/* x0 here holds offsetX in floating point */
		y0 = scaleOffset->offsetY  * (1.f/((float)(1 << kFskOffsetBits)));	/* y0 here holds offsetY in floating point */
		x0 += x1 * r->x;													/* Left   coordinate */
		y0 -= y1 * r->y;													/* Bottom coordinate */
		x1 = x0 + x1 * (r->width  - 1) + ((x1 < 0) ? -1 : 1);				/* Right  coordinate */
		y1 = y0 + y1 * (r->height - 1) - ((y1 < 0) ? -1 : 1);				/* Bottom coordinate */
		if (x0 < x1)	x1 += 1.f;
		else			x0 += 1.f;
		if (y0 < y1)	y1 += 1.f;
		else			y0 += 1.f;
	#else /* GL_COORD_TYPE_ENUM == GL_FIXED */
		GLfixed	x0	=        (scaleOffset->offsetX >> (kFskOffsetBits - 16)) + FskFixedNMul(scaleOffset->scaleX, r->x            , kFskScaleBits - 16),
				x1	=        (scaleOffset->offsetX >> (kFskOffsetBits - 16)) + FskFixedNMul(scaleOffset->scaleX, r->x + r->width , kFskScaleBits - 16),
				y0	=        (scaleOffset->offsetY >> (kFskOffsetBits - 16)) + FskFixedNMul(scaleOffset->scaleY, r->y            , kFskScaleBits - 16),
				y1	=        (scaleOffset->offsetY >> (kFskOffsetBits - 16)) + FskFixedNMul(scaleOffset->scaleY, r->y + r->height, kFskScaleBits - 16);
		x1 += (scaleOffset->offsetX < 0) ? -0x10000 : 0x10000;
		y1 += (scaleOffset->offsetY < 0) ? -0x10000 : 0x10000;
		if (x0 < x1)	x1 += 0x10000;
		else			x0 += 0x10000;
		if (y0 < y1)	y1 += 0x10000;
		else			y0 += 0x10000;
	#endif /* GL_COORD_TYPE_ENUM == GL_FIXED */

	vertex[0].x = x0;		vertex[0].y = y0;
	vertex[1].x = x0;		vertex[1].y = y1;
	vertex[2].x = x1;		vertex[2].y = y1;
	vertex[3].x = x1;		vertex[3].y = y0;
}


/****************************************************************************//**
 * Make texture coordinates corresponding to the given rectangle.
 *	\param[in]	r			the rectangle.
 *	\param[in]	texRect		the bounds of the texture.
 *	\param[in]	tx			the texture record
 *	\param[out]	texCoords	the generated texture coordinates, four 2D points.
 ********************************************************************************/

static void MakeRectTexCoords(FskConstRectangle r, FskConstRectangle texRect, ConstGLTexture tx)
{
	GLTexVertex			*vertex = (GLTexVertex*)(gGLGlobalAssets.coordinates);
	GLCoordinateType	u0, u1, v0, v1;
	SInt32				m, o;

	m = tx->bounds.width;
	o = r->x - texRect->x;
	u0 = NormalizedCoordinateBiased(o,             m, +.125);
	u1 = NormalizedCoordinateBiased(o + r->width,  m, -.375);

	m = tx->bounds.height;
	o = r->y - texRect->y;
	if (!tx->flipY) {
		v0 = NormalizedCoordinateBiased(o,             m, +.125);
		v1 = NormalizedCoordinateBiased(o + r->height, m, -.375);
	}
	else {
		o = texRect->height - o;
		v0 = NormalizedCoordinateBiased(o,             m, +.125);
		v1 = NormalizedCoordinateBiased(o - r->height, m, -.375);
	}

	vertex[0].u = u0;		vertex[0].v = v0;
	vertex[1].u = u0;		vertex[1].v = v1;
	vertex[2].u = u1;		vertex[2].v = v1;
	vertex[3].u = u1;		vertex[3].v = v0;
}


#if GLES_VERSION == 2
/****************************************************************************//**
 * Make texture coordinates for YUV422 corresponding to the given rectangle.
 *	\param[in]	r			the rectangle.
 *	\param[in]	texRect		the bounds of the texture.
 *	\param[in]	tx			the texture record
 *	\param[out]	texCoords	the generated texture coordinates, four 2D points.
 ********************************************************************************/

static void MakeRectTexYUV422Coords(FskConstRectangle r, FskConstRectangle texRect, ConstGLTexture tx)
{
	GLTexVertex			*vertex = (GLTexVertex*)(gGLGlobalAssets.coordinates);
	GLCoordinateType	u0, u1, v0, v1;
	SInt32				m, o;

	m = tx->bounds.width << 1;
	o = r->x - texRect->x;
	u0 = NormalizedCoordinateBiased(o,                 m, +.0);
	u1 = NormalizedCoordinateBiased(o + r->width - 1,  m, -.0);

	m = tx->bounds.height;
	o = r->y - texRect->y;
	if (!tx->flipY) {
		v0 = NormalizedCoordinateBiased(o,                 m, +.0);
		v1 = NormalizedCoordinateBiased(o + r->height - 1, m, -.0);
	}
	else {
		o = texRect->height - o;
		v0 = NormalizedCoordinateBiased(o,                 m, +.0);
		v1 = NormalizedCoordinateBiased(o - r->height + 1, m, -.0);
	}

	vertex[0].u = u0;		vertex[0].v = v0;
	vertex[1].u = u0;		vertex[1].v = v1;
	vertex[2].u = u1;		vertex[2].v = v1;
	vertex[3].u = u1;		vertex[3].v = v0;
}
#endif /* GLES_VERSION == 2 */


/****************************************************************************//**
 * Make texture coordinates (u,v) corresponding to the model coordinates (x,y).
 * The model and texture coordinates are passed in the global array gGLGlobalAssets.coordinates.
 * On input,  gGLGlobalAssets.coordinates has	{x0,y0,--,--,x1,y1,--,--,...,xm,ym,--,--};
 * on output, gGLGlobalAssets.coordinates has	{x0,y0,u0,v0,x0,y1,u1,v1,...,xm,ym,um,vm},
 * where m = numPts - 1.
 *	\param[in]	numPts		the number of points, starting at &gGLGlobalAssets.coordinates[0], with stride TEXVERTEX_STRIDE.
 *	\param[in]	texRect		the bounds of the texture.
 *	\param[in]	tx			the texture record.
 ********************************************************************************/

static void MakeTexCoords(UInt32 numPts, FskConstRectangle texRect, GLTexture tx) {
	GLTexVertex *vertex = (GLTexVertex*)(gGLGlobalAssets.coordinates);

	#if GL_COORD_TYPE_ENUM == GL_FLOAT
		#ifdef PIXELS_ARE_POINTS
			const float	flip = (float)(texRect->height - 1);
			#ifdef TEXELS_ARE_POINTS
				const float iw = 1.f / (tx->bounds.width - 0), ih = 1.f / (tx->bounds.height - 0);
			#else /* TEXELS_ARE_SQUARES */
				const float iw = 1.f / (tx->bounds.width - 1), ih = 1.f / (tx->bounds.height - 1);
			#endif /* TEXELS_ARE_SQUARES */
		#else /* PIXELS_ARE_SQUARES */
			const float	flip = (float)(texRect->height);
			#ifdef TEXELS_ARE_POINTS
				const float iw = 1.f / (tx->bounds.width + 1), ih = 1.f / (tx->bounds.height + 1);
			#else /* TEXELS_ARE_SQUARES */
				const float iw = 1.f / (tx->bounds.width - 0), ih = 1.f / (tx->bounds.height - 0);
			#endif /* TEXELS_ARE_SQUARES */
		#endif /* PIXELS */
		if (!tx->flipY) for (; numPts--; ++vertex) {		/* Yes, this seems to be negative logic, because vertexY has already been flipped */
			vertex->u = iw *         vertex->x;
			vertex->v = ih * (flip - vertex->y);
		}
		else            for (; numPts--; ++vertex) {
			vertex->u = iw *         vertex->x;
			vertex->v = ih *         vertex->y;
		}
	#elif GL_COORD_TYPE_ENUM == GL_FIXED
		#ifdef PIXELS_ARE_POINTS
			const FskFract	flip = (texRect->height - 1) << 16;
			#ifdef TEXELS_ARE_POINTS
				const FskFract	iw = FskFracDiv(1, tx->bounds.width),     ih = FskFracDiv(1, tx->bounds.height);
			#else /* TEXELS_ARE_SQUARES */
				const FskFract	iw = FskFracDiv(1, tx->bounds.width - 1), ih = FskFracDiv(1, tx->bounds.height - 1);
			#endif /* TEXELS_ARE_SQUARES */
		#else /* PIXELS_ARE_SQUARES */
			const FskFract	flip = texRect->height << 16;
			#ifdef TEXELS_ARE_POINTS
				const FskFract	iw = FskFracDiv(1, tx->bounds.width + 1), ih = FskFracDiv(1, tx->bounds.height + 1);
			#else /* TEXELS_ARE_SQUARES */
				const FskFract	iw = FskFracDiv(1, tx->bounds.width),     ih = FskFracDiv(1, tx->bounds.height);
			#endif /* TEXELS_ARE_SQUARES */
		#endif /* PIXELS */
		if (!tx->flipY) for (; numPts--; ++vertex) {		/* Yes, this seems to be negative logic, because vertexY has already been flipped */
			vertex->u = FskFracMul(iw,         vertex->x);
			vertex->v = FskFracMul(ih, (flip - vertex->y));
		}
		else for (; numPts--; ++vertex) {
			vertex->u = FskFracMul(iw,         vertex->x);
			vertex->v = FskFracMul(ih,         vertex->y);
		}
	#else /* GL_COORD_TYPE_ENUM == unknown */
		for (; numPts--; ++vertex) {
			vertex->u = NormalizedCoordinate(vertex->x /*- texRect->x*/, tx->bounds.width);
			vertex->v = NormalizedCoordinate(vertex->y /*- texRect->y*/, tx->bounds.height);
		}
	#endif /* GL_COORD_TYPE_ENUM */
}


/********************************************************************************
 * ConvertImage32Format - in place.
 * We use this on "const" bitmaps twice to leave the source unchanged, so we can convert to a format that GL likes.
 ********************************************************************************/

static FskErr ConvertImage32Format(UInt32 *p, SInt32 wd, SInt32 ht, SInt32 rb, UInt32 srcFmt, UInt32 dstFmt)
{
	FskErr		err		= kFskErrNone;
	UInt32		pix;
	SInt32		bump, w, h;

	if (srcFmt == dstFmt)
		return kFskErrNone;

	bump = (rb - wd * sizeof(*p)) / sizeof(*p);

	switch (srcFmt) {
		case FskName2(kFskBitmapFormat,fsk32ARGBKindFormat):	switch (dstFmt) {
			case kFskBitmapFormat32A16RGB565LE:						for (h=ht; h--; p+=bump) for (w=wd; w--; ++p) { pix=*p; FskName2(fskConvert32ARGB,fsk32A16RGB565LEFormatKind)(pix); *p=pix; } break;
			case FskName2(kFskBitmapFormat,fsk32ABGRKindFormat):	for (h=ht; h--; p+=bump) for (w=wd; w--; ++p) { pix=*p; fskConvert32ARGB32ABGR(pix); *p=pix; } break;
			case FskName2(kFskBitmapFormat,fsk32BGRAKindFormat):	for (h=ht; h--; p+=bump) for (w=wd; w--; ++p) { pix=*p; fskConvert32ARGB32BGRA(pix); *p=pix; } break;
			case FskName2(kFskBitmapFormat,fsk32RGBAKindFormat):	for (h=ht; h--; p+=bump) for (w=wd; w--; ++p) { pix=*p; fskConvert32ARGB32RGBA(pix); *p=pix; } break;
			default: err = kFskErrUnsupportedPixelType;
		} break;
		case FskName2(kFskBitmapFormat,fsk32BGRAKindFormat):	switch (dstFmt) {
			case kFskBitmapFormat32A16RGB565LE:						for (h=ht; h--; p+=bump) for (w=wd; w--; ++p) { pix=*p; FskName2(fskConvert32BGRA,fsk32A16RGB565LEFormatKind)(pix); *p=pix; } break;
			case FskName2(kFskBitmapFormat,fsk32ABGRKindFormat):	for (h=ht; h--; p+=bump) for (w=wd; w--; ++p) { pix=*p; fskConvert32BGRA32ABGR(pix); *p=pix; } break;
			case FskName2(kFskBitmapFormat,fsk32ARGBKindFormat):	for (h=ht; h--; p+=bump) for (w=wd; w--; ++p) { pix=*p; fskConvert32BGRA32ARGB(pix); *p=pix; } break;
			case FskName2(kFskBitmapFormat,fsk32RGBAKindFormat):	for (h=ht; h--; p+=bump) for (w=wd; w--; ++p) { pix=*p; fskConvert32BGRA32RGBA(pix); *p=pix; } break;
			default: err = kFskErrUnsupportedPixelType;
		} break;
		case FskName2(kFskBitmapFormat,fsk32RGBAKindFormat):	switch (dstFmt) {
			case kFskBitmapFormat32A16RGB565LE:						for (h=ht; h--; p+=bump) for (w=wd; w--; ++p) { pix=*p; FskName2(fskConvert32RGBA,fsk32A16RGB565LEFormatKind)(pix); *p=pix; } break;
			case FskName2(kFskBitmapFormat,fsk32ABGRKindFormat):	for (h=ht; h--; p+=bump) for (w=wd; w--; ++p) { pix=*p; fskConvert32RGBA32ABGR(pix); *p=pix; } break;
			case FskName2(kFskBitmapFormat,fsk32ARGBKindFormat):	for (h=ht; h--; p+=bump) for (w=wd; w--; ++p) { pix=*p; fskConvert32RGBA32ARGB(pix); *p=pix; } break;
			case FskName2(kFskBitmapFormat,fsk32BGRAKindFormat):	for (h=ht; h--; p+=bump) for (w=wd; w--; ++p) { pix=*p; fskConvert32RGBA32BGRA(pix); *p=pix; } break;
			default: err = kFskErrUnsupportedPixelType;
		} break;
		case FskName2(kFskBitmapFormat,fsk32ABGRKindFormat):	switch (dstFmt) {
			case kFskBitmapFormat32A16RGB565LE:						for (h=ht; h--; p+=bump) for (w=wd; w--; ++p) { pix=*p; FskName2(fskConvert32ABGR,fsk32A16RGB565LEFormatKind)(pix); *p=pix; } break;
			case FskName2(kFskBitmapFormat,fsk32ARGBKindFormat):	for (h=ht; h--; p+=bump) for (w=wd; w--; ++p) { pix=*p; fskConvert32ABGR32ARGB(pix); *p=pix; } break;
			case FskName2(kFskBitmapFormat,fsk32BGRAKindFormat):	for (h=ht; h--; p+=bump) for (w=wd; w--; ++p) { pix=*p; fskConvert32ABGR32BGRA(pix); *p=pix; } break;
			case FskName2(kFskBitmapFormat,fsk32RGBAKindFormat):	for (h=ht; h--; p+=bump) for (w=wd; w--; ++p) { pix=*p; fskConvert32ABGR32RGBA(pix); *p=pix; } break;
			default: err = kFskErrUnsupportedPixelType;
		} break;
		case kFskBitmapFormat32A16RGB565LE:						switch (dstFmt) {
			case FskName2(kFskBitmapFormat,fsk32ABGRKindFormat):	for (h=ht; h--; p+=bump) for (w=wd; w--; ++p) { pix=*p; FskName3(fskConvert,fsk32A16RGB565LEFormatKind,32ABGR)(pix); *p=pix; } break;
			case FskName2(kFskBitmapFormat,fsk32ARGBKindFormat):	for (h=ht; h--; p+=bump) for (w=wd; w--; ++p) { pix=*p; FskName3(fskConvert,fsk32A16RGB565LEFormatKind,32ARGB)(pix); *p=pix; } break;
			case FskName2(kFskBitmapFormat,fsk32BGRAKindFormat):	for (h=ht; h--; p+=bump) for (w=wd; w--; ++p) { pix=*p; FskName3(fskConvert,fsk32A16RGB565LEFormatKind,32BGRA)(pix); *p=pix; } break;
			case FskName2(kFskBitmapFormat,fsk32RGBAKindFormat):	for (h=ht; h--; p+=bump) for (w=wd; w--; ++p) { pix=*p; FskName3(fskConvert,fsk32A16RGB565LEFormatKind,32RGBA)(pix); *p=pix; } break;
			default: err = kFskErrUnsupportedPixelType;
		} break;
		default: err = kFskErrUnsupportedPixelType;
	}
	return err;
}


/********************************************************************************
 * ConvertImage24Format - in place
 ********************************************************************************/

static FskErr ConvertImage24Format(UInt8 *p, SInt32 wd, SInt32 ht, SInt32 rb, UInt32 srcFmt, UInt32 dstFmt)
{
	FskErr	err		= kFskErrNone;
	SInt32	bump, w, h;
	UInt8	temp;

	if (srcFmt == dstFmt)
		return kFskErrNone;

	bump = rb - wd * 3;
	for (h = ht; h--; p += bump) for (w = wd; w--; p += 3) {
		temp = p[0];
		p[0] = p[2];
		p[2] = temp;
	}
	return err;
}


/********************************************************************************
 * ReflectImageVertically
 ********************************************************************************/

static void ReflectImageVertically(void *pix, SInt32 width, SInt32 height, SInt32 pixBytes, SInt32 rowBytes) {
	UInt8	*r0 = (UInt8*)pix;
	UInt8	*r1 = (UInt8*)pix + (height - 1) * rowBytes;
	UInt32	w;

	switch (pixBytes) {
		case 3:
			width *= 3;
		case 1: {
			typedef UInt8 PixType;
			PixType *p0, *p1, t;
			for (height >>= 1; height--; r0 += rowBytes, r1 -= rowBytes) {
				for (w = width, p0 = (PixType*)r0, p1 = (PixType*)r1; w--; ++p0, ++p1) {
					t = *p0; *p0 = *p1; *p1 = t;
				}
			}

		}	break;
		case 2: {
			typedef UInt16 PixType;
			PixType *p0, *p1, t;
			for (height >>= 1; height--; r0 += rowBytes, r1 -= rowBytes) {
				for (w = width, p0 = (PixType*)r0, p1 = (PixType*)r1; w--; ++p0, ++p1) {
					t = *p0; *p0 = *p1; *p1 = t;
				}
			}

		}	break;
		case 4: {
			typedef UInt32 PixType;
			PixType *p0, *p1, t;
			for (height >>= 1; height--; r0 += rowBytes, r1 -= rowBytes) {
				for (w = width, p0 = (PixType*)r0, p1 = (PixType*)r1; w--; ++p0, ++p1) {
					t = *p0; *p0 = *p1; *p1 = t;
				}
			}

		}	break;
		default:
			break;
	}
}


/********************************************************************************
 * ChangeRowBytes
 ********************************************************************************/

#if !defined(GL_PACK_ROW_LENGTH) || defined(DUMP_TYPEFACE)
static void ChangeRowBytes(void *pix, SInt32 width, SInt32 height, SInt32 pixBytes, SInt32 srcRowBytes, SInt32 dstRowBytes) {
	UInt32	rowBytes	= width * pixBytes;
	char	*s			= (char*)pix,
			*d			= (char*)pix;

	/* TODO: Make this logic work when rowBytes is negative */
	if (dstRowBytes == srcRowBytes)
		return;
	if (dstRowBytes > srcRowBytes) {
		s += (height - 1) * srcRowBytes;
		d += (height - 1) * dstRowBytes;
		srcRowBytes = -srcRowBytes;
		dstRowBytes = -dstRowBytes;
	}
	for (; height--; s += srcRowBytes, d += dstRowBytes)
		FskMemMove(d, s, rowBytes);
}
#endif /* GL_PACK_ROW_LENGTH */


#define kFskBitmapFormat24GL	kFskBitmapFormat24RGB	/**< This is the format that is compatible with GL_RGB.  */
#define kFskBitmapFormat32GL	kFskBitmapFormat32RGBA	/**< This is the format that is compatible with GL_RGBA. */


#if GLES_VERSION == 2

/********************************************************************************
 * ReshapeYUV420Textures.
 * This just initializes the textures, but does not upload anything.
 ********************************************************************************/

static FskErr ReshapeYUV420Textures(GLsizei width, GLsizei height, GLTexture tx, GLint glQuality) {
	FskErr		err			= kFskErrNone;
	#if defined(LOG_TEXTURE_LIFE) || defined(LOG_YUV)
		GLsizei inWidth		= width,
				inHeight	= height;
	#endif /* LOG_TEXTURE_LIFE */

	/* (re)allocate texture storage */
	if (gGLGlobalAssets.hasNPOT) {
		width  = BLOCKIFY(width,  TEXTURE_BLOCK_HORIZONTAL/*2*/);												/* Should the block should be twice as big so U and V will be properly aligned? */
		height = BLOCKIFY(height,     ALPHA_BLOCK_VERTICAL/*2*/);
	}
	else {
		width  = SmallestContainingPowerOfTwo(width);
		height = SmallestContainingPowerOfTwo(height);
	}
	if (width  < TEXTURE_MIN_DIM)
		width  = TEXTURE_MIN_DIM;
	if (height < TEXTURE_MIN_DIM)
		height = TEXTURE_MIN_DIM;

	if (tx->glIntFormat == GL_ALPHA && tx->bounds.width == width && tx->bounds.height == height && tx->nameU && tx->nameV) {	/* No need to check tx->name if tx->with and tx->bounds.height are set */
		#if defined(LOG_TEXTURE_LIFE) || defined(LOG_YUV)
			LOGD("ReshapeYUV420Textures({#%u, #%u, #%u} from %s %ux%u -- already ALPHA Q%d %dx%d->%dx%d)", (unsigned)tx->name, (unsigned)tx->nameU, (unsigned)tx->nameV,
				GLInternalFormatNameFromCode(tx->glIntFormat), (unsigned)tx->bounds.width, (unsigned)tx->bounds.height, glQuality==GL_LINEAR, inWidth, inHeight, width, height);
		#endif /* LOG_TEXTURE_LIFE */
		goto bail;																								/* Already the right size and shape -- done */
	}
	#if defined(LOG_TEXTURE_LIFE) || defined(LOG_YUV)
			LOGD("ReshapeYUV420Textures({#%u, #%u, #%u} from %s %ux%u to ALPHA Q%d %dx%d->%dx%d)", (unsigned)tx->name, (unsigned)tx->nameU, (unsigned)tx->nameV,
				GLInternalFormatNameFromCode(tx->glIntFormat), (unsigned)tx->bounds.width, (unsigned)tx->bounds.height, glQuality==GL_LINEAR, inWidth, inHeight, width, height);
	#endif /* LOG_TEXTURE_LIFE */

	if (!gGLGlobalAssets.allowNearestBitmap)																	/* If low quality is disallowed, */
		glQuality = GL_LINEAR;																					/* always bilinearly interpolate */

	tx->bounds.x		= 0;
	tx->bounds.y		= 0;
	tx->bounds.width	= width;
	tx->bounds.height	= height;
	tx->wrapMode		= GL_CLAMP_TO_EDGE;
	tx->filter			= glQuality;
	tx->glIntFormat		= GL_ALPHA;																				/* It has an internal alpha format */
	tx->fboInited		= false;

	/*
	 * Initialize the texture units, if the textures needed to be resized.
	 */
	if (0 == tx->name  || GL_TEXTURE_UNLOADED == tx->name)		FskGenGLTexturesAndInit(1, &tx->name);			/* Allocate texture for Y */
	if (0 == tx->nameU || GL_TEXTURE_UNLOADED == tx->nameU)		FskGenGLTexturesAndInit(1, &tx->nameU);			/* Allocate texture for U */
	if (0 == tx->nameV || GL_TEXTURE_UNLOADED == tx->nameV)		FskGenGLTexturesAndInit(1, &tx->nameV);			/* Allocate texture for V */
	BAIL_IF_FALSE((	0 != tx->name  && GL_TEXTURE_UNLOADED != tx->name  &&
					0 != tx->nameU && GL_TEXTURE_UNLOADED != tx->nameU &&
					0 != tx->nameV && GL_TEXTURE_UNLOADED != tx->nameV
				), err, kFskErrGraphicsContext);

	glActiveTexture(GL_TEXTURE2);																				/* Texture unit 2 ... */
	SET_TEXTURE(tx->nameV);																						/* ... has the V texture */
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	#ifdef GL_UNPACK_ROW_LENGTH
		glPixelStorei(GL_UNPACK_ROW_LENGTH,  0);
	#endif /* GL_UNPACK_ROW_LENGTH */
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, glQuality);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, glQuality);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, tx->bounds.width>>1, tx->bounds.height>>1, 0, GL_ALPHA, GL_UNSIGNED_BYTE, NULL);	/* Resize V texture; half-res */
	#if CHECK_GL_ERROR
		BAIL_IF_ERR(err = GetFskGLError());
	#endif /* CHECK_GL_ERROR */

	glActiveTexture(GL_TEXTURE1);																				/* Texture unit 1 ... */
	SET_TEXTURE(tx->nameU);																						/* ... has the U texture */
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	#ifdef GL_UNPACK_ROW_LENGTH
		glPixelStorei(GL_UNPACK_ROW_LENGTH,  0);
	#endif /* GL_UNPACK_ROW_LENGTH */
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, glQuality);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, glQuality);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, tx->bounds.width>>1, tx->bounds.height>>1, 0, GL_ALPHA, GL_UNSIGNED_BYTE, NULL);	/* Resize U texture; half-res */
	#if CHECK_GL_ERROR
		BAIL_IF_ERR(err = GetFskGLError());
	#endif /* CHECK_GL_ERROR */

	glActiveTexture(GL_TEXTURE0);																				/* Texture unit 0 ... */
	SET_TEXTURE(tx->name);																						/* ... has the Y texture */
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	#ifdef GL_UNPACK_ROW_LENGTH
		glPixelStorei(GL_UNPACK_ROW_LENGTH,  0);
	#endif /* GL_UNPACK_ROW_LENGTH */
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, glQuality);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, glQuality);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, tx->bounds.width, tx->bounds.height, 0, GL_ALPHA, GL_UNSIGNED_BYTE, NULL);		/* Resize Y texture */
	#if CHECK_GL_ERROR
		BAIL_IF_ERR(err = GetFskGLError());
	#endif /* CHECK_GL_ERROR */

	#if defined(LOG_TEXTURE_LIFE) || defined(LOG_YUV)
		LOGD("ReshapeYUV420Textures: {#%u, #%u, #%u} is now %s %dx%d", (unsigned)tx->name, (unsigned)tx->nameU, (unsigned)tx->nameV, GLInternalFormatNameFromCode(tx->glIntFormat), tx->bounds.width, tx->bounds.height);
	#endif /* LOG_TEXTURE_LIFE */

bail:
	PRINT_IF_ERROR(err, __LINE__, "ReshapeYUV420Textures");
	return err;
}


/********************************************************************************
 * ReshapeYUV420spTextures.
 ********************************************************************************/

static FskErr ReshapeYUV420spTextures(GLsizei width, GLsizei height, GLTexture tx, GLint glQuality) {
	FskErr		err			= kFskErrNone;
	#if defined(LOG_TEXTURE_LIFE) || defined(LOG_YUV)
		GLsizei inWidth		= width,
				inHeight	= height;
	#endif /* LOG_TEXTURE_LIFE */

	if (gGLGlobalAssets.hasNPOT) {
		width  = BLOCKIFY(width,  TEXTURE_BLOCK_HORIZONTAL/*2*/);												/* Should the block should be twice as big so U and V will be properly aligned? */
		height = BLOCKIFY(height,     ALPHA_BLOCK_VERTICAL/*2*/);
	}
	else {
		width  = SmallestContainingPowerOfTwo(width);
		height = SmallestContainingPowerOfTwo(height);
	}
	if (width  < TEXTURE_MIN_DIM)
		width  = TEXTURE_MIN_DIM;
	if (height < TEXTURE_MIN_DIM)
		height = TEXTURE_MIN_DIM;

	if (tx->glIntFormat == GL_ALPHA && tx->bounds.width == width && tx->bounds.height == height && tx->nameU && !tx->nameV) {	/* No need to check tx->name if tx->with and tx->bounds.height are set */
		#if defined(LOG_TEXTURE_LIFE) || defined(LOG_YUV)
			LOGD("ReshapeYUV420spTextures({#%u, #%u} from %s %ux%u -- already ALPHA Q%d %dx%d->%dx%d)", (unsigned)tx->name, (unsigned)tx->nameU,
				GLInternalFormatNameFromCode(tx->glIntFormat), (unsigned)tx->bounds.width, (unsigned)tx->bounds.height, glQuality==GL_LINEAR, inWidth, inHeight, width, height);
		#endif /* LOG_TEXTURE_LIFE */
		goto bail;																								/* Already the right size and shape -- done */
	}
	#if defined(LOG_TEXTURE_LIFE) || defined(LOG_YUV)
			LOGD("ReshapeYUV420spTextures({#%u, #%u} from %s %ux%u to ALPHA Q%d %dx%d->%dx%d)", (unsigned)tx->name, (unsigned)tx->nameU,
				GLInternalFormatNameFromCode(tx->glIntFormat), (unsigned)tx->bounds.width, (unsigned)tx->bounds.height, glQuality==GL_LINEAR, inWidth, inHeight, width, height);
	#endif /* LOG_TEXTURE_LIFE */

	if (!gGLGlobalAssets.allowNearestBitmap)																	/* If low quality is disallowed, */
		glQuality = GL_LINEAR;																					/* always bilinearly interpolate */

	tx->bounds.x		= 0;
	tx->bounds.y		= 0;
	tx->bounds.width	= width;
	tx->bounds.height	= height;
	tx->wrapMode		= GL_CLAMP_TO_EDGE;
	tx->filter			= glQuality;
	tx->glIntFormat		= GL_ALPHA;																				/* It has an internal alpha format for Y and luminance-alpha for UV */
	tx->fboInited		= false;

	/*
	 * Initialize the texture units, if the textures needed to be resized.
	 */
	if (0 == tx->name  ||  GL_TEXTURE_UNLOADED == tx->name)		FskGenGLTexturesAndInit(1, &tx->name);			/* Allocate texture for Y  */
	if (0 == tx->nameU ||  GL_TEXTURE_UNLOADED == tx->nameU)	FskGenGLTexturesAndInit(1, &tx->nameU);			/* Allocate texture for UV */
	if (0 != tx->nameV) { FskDeleteGLTextures(1, &tx->nameV);	tx->nameV = 0; }
	BAIL_IF_FALSE((	0 != tx->name  && GL_TEXTURE_UNLOADED != tx->name  &&
					0 != tx->nameU && GL_TEXTURE_UNLOADED != tx->nameU
				), err, kFskErrGraphicsContext);

	glActiveTexture(GL_TEXTURE1);																				/* Texture unit 1 ... */
	SET_TEXTURE(tx->nameU);																						/* ... has the UV texture */
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	#ifdef GL_UNPACK_ROW_LENGTH
		glPixelStorei(GL_UNPACK_ROW_LENGTH,  0);
	#endif /* GL_UNPACK_ROW_LENGTH */
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, glQuality);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, glQuality);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE_ALPHA, tx->bounds.width>>1, tx->bounds.height>>1, 0, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, NULL);	/* Resize UV texture; half-res */
	#if CHECK_GL_ERROR
		BAIL_IF_ERR(err = GetFskGLError());
	#endif /* CHECK_GL_ERROR */

	glActiveTexture(GL_TEXTURE0);																				/* Texture unit 0 ... */
	SET_TEXTURE(tx->name);																						/* ... has the Y texture */
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	#ifdef GL_UNPACK_ROW_LENGTH
		glPixelStorei(GL_UNPACK_ROW_LENGTH,  0);
	#endif /* GL_UNPACK_ROW_LENGTH */
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, glQuality);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, glQuality);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, tx->bounds.width, tx->bounds.height, 0, GL_ALPHA, GL_UNSIGNED_BYTE, NULL);		/* Resize Y texture */
	#if CHECK_GL_ERROR
		BAIL_IF_ERR(err = GetFskGLError());
	#endif /* CHECK_GL_ERROR */

	#if defined(LOG_TEXTURE_LIFE) || defined(LOG_YUV)
		LOGD("ReshapeYUV420spTextures: {#%u, #%u} is now %s %dx%d", (unsigned)tx->name, (unsigned)tx->nameU, GLInternalFormatNameFromCode(tx->glIntFormat), tx->bounds.width, tx->bounds.height);
	#endif /* LOG_TEXTURE_LIFE */

bail:
	PRINT_IF_ERROR(err, __LINE__, "ReshapeYUV420spTextures");
	return err;
}


/********************************************************************************
 * ReshapeUYVYTexture.
 ********************************************************************************/

static FskErr ReshapeUYVYTexture(GLsizei width, GLsizei height, GLTexture tx, GLint glQuality) {
	FskErr				err			= kFskErrNone;
	GLInternalFormat	glIntFmt;
	GLFormat			glFormat;
	GLType				glType;
	#if defined(LOG_TEXTURE_LIFE) || defined(LOG_YUV)
		GLsizei inWidth		= (width + 1) >> 1,
				inHeight	= height;
	#endif /* LOG_TEXTURE_LIFE */

	width = (width + 1) >> 1;																						/* Chrominance width */

	BAIL_IF_ERR(err = FskGLGetFormatAndTypeFromPixelFormat(kFskBitmapFormat32RGBA, &glFormat, &glType, &glIntFmt));

	if (gGLGlobalAssets.hasNPOT) {
		width  = BLOCKIFY(width,  TEXTURE_BLOCK_HORIZONTAL);
		height = BLOCKIFY(height, TEXTURE_BLOCK_VERTICAL);
	}
	else {
		width  = SmallestContainingPowerOfTwo(width);
		height = SmallestContainingPowerOfTwo(height);
	}
	if (width  < TEXTURE_MIN_DIM)
		width  = TEXTURE_MIN_DIM;
	if (height < TEXTURE_MIN_DIM)
		height = TEXTURE_MIN_DIM;

	if (tx->glIntFormat == glIntFmt && tx->bounds.width == width && tx->bounds.height == height && !tx->nameU && !tx->nameV) {	/* No need to check tx->name if tx->with and tx->bounds.height are set */
		#if defined(LOG_TEXTURE_LIFE) || defined(LOG_YUV)
			LOGD("ReshapeUYVYTexture(#%u from %s %ux%u -- already %s %dx%d->%dx%d)", (unsigned)tx->name, GLInternalFormatNameFromCode(tx->glIntFormat),
				(unsigned)tx->bounds.width, (unsigned)tx->bounds.height, GLInternalFormatNameFromCode(GL_ALPHA), inWidth, inHeight, width, height);
		#endif /* LOG_TEXTURE_LIFE || LOG_YUV */
		goto bail;																							/* Already the right size and shape -- done */
	}
	#if defined(LOG_TEXTURE_LIFE) || defined(LOG_YUV)
		LOGD("ReshapeUYVYTexture(#%u from %s %ux%u to %s %dx%d->%dx%d)", (unsigned)tx->name, GLInternalFormatNameFromCode(tx->glIntFormat),
			(unsigned)tx->bounds.width, (unsigned)tx->bounds.height, GLInternalFormatNameFromCode(GL_ALPHA), inWidth, inHeight, width, height);
	#endif /* LOG_TEXTURE_LIFE || LOG_YUV */

	tx->bounds.x		= 0;
	tx->bounds.y		= 0;
	tx->bounds.width	= width;
	tx->bounds.height	= height;
	tx->srcBM			= NULL;
	tx->fboInited		= false;

	if (0 == tx->name ||   GL_TEXTURE_UNLOADED == tx->name)		FskGenGLTexturesAndInit(1, &tx->name);			/* Allocate texture for Y  */
	if (0 != tx->nameU) { FskDeleteGLTextures(1, &tx->nameU);	tx->nameU = 0; }
	if (0 != tx->nameV) { FskDeleteGLTextures(1, &tx->nameV);	tx->nameV = 0; }
	BAIL_IF_FALSE((0 != tx->name && GL_TEXTURE_UNLOADED != tx->name), err, kFskErrGraphicsContext);

	SET_TEXTURE(tx->name);
	tx->glIntFormat = glIntFmt;

	tx->filter = glQuality;
	if (glQuality) {																							/* If quality is set to 0, defer its setting */
		#ifdef GL_RGB_422_APPLE
			if (!gGLGlobalAssets.hasAppleRGB422)																/* Apple RGB 422 shader accommodates quality */
		#endif		/* GL_RGB_422_APPLE */
		glQuality = GL_NEAREST;																					/* We do interpolation in the shader */
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, glQuality);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, glQuality);
	}

	tx->wrapMode = GL_CLAMP_TO_EDGE;
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
	glTexImage2D(GL_TEXTURE_2D, 0, glIntFmt, tx->bounds.width, tx->bounds.height, 0, glFormat, glType, NULL);	/* Chrominance width */
	#if CHECK_GL_ERROR
		BAIL_IF_ERR(err = GetFskGLError());
	#endif /* CHECK_GL_ERROR */

	#if defined(LOG_TEXTURE_LIFE) || defined(LOG_YUV)
		LOGD("ReshapeUYVYTexture: #%u is now %s %dx%d", (unsigned)tx->name, GLInternalFormatNameFromCode(tx->glIntFormat), tx->bounds.width, tx->bounds.height);
	#endif /* LOG_TEXTURE_LIFE || LOG_YUV */

bail:
	PRINT_IF_ERROR(err, __LINE__, "ReshapeUYVYTexture");
	return err;
}
#endif /* GLES_VERSION == 2 */


/****************************************************************************//**
 * Enlarge the texture to fit the rect of the bitmap, if necessary.
 *	\param[in]	width				the width  of the portion of the bitmap to be uploaded.
 *	\param[in]	height				the height of the portion of the bitmap to be uploaded.
 *	\param[in]	srcGLFormat			the GL format of the source.
 *	\param[in]	srcGLType			the GL  type  of the source.
 *	\param[in]	internalGLFormat	the internal GL format of the texture.
 *	\param[in]	tx					the GL texture record.
 *	\return		kFskErrNone	if the texture was set successfully.
 ********************************************************************************/

static FskErr ReshapeOffscreenTexture(GLInternalFormat internalGLFormat, GLsizei width, GLsizei height, GLFormat srcGLFormat, GLType srcGLType, GLTexture tx) {
	FskErr	err		= kFskErrNone,
			sizeErr	= kFskErrNone;

	#ifdef LOG_TEXTURE_LIFE
	if (gGLGlobalAssets.blitContext->lastTexture) {
		GLint curTex;
		glGetIntegerv(GL_TEXTURE_BINDING_2D, &curTex);
		if (curTex != (GLint)gGLGlobalAssets.blitContext->lastTexture)	LOGE("ERROR: curTex(%d) != gGLGlobalAssets.blitContext->lastTexture(%d)", curTex, gGLGlobalAssets.blitContext->lastTexture);
	}
	#endif /* LOG_TEXTURE_LIFE */

	tx->fboInited = false;

	/* If the given texture has the same format and size, no resizing is necessary */
	if (tx->glIntFormat == internalGLFormat && width == tx->bounds.width && height == tx->bounds.height && !tx->nameU && !tx->nameV) {
		#ifdef LOG_TEXTURE_LIFE
			LOGD("ReshapeOffscreenTexture(#%u from %s %ux%u to %s %dx%d -- already the right shape)", (unsigned)tx->name,
				GLInternalFormatNameFromCode(tx->glIntFormat), (unsigned)tx->bounds.width, (unsigned)tx->bounds.height, GLInternalFormatNameFromCode(internalGLFormat), width, height);
		#endif /* LOG_TEXTURE_LIFE */
		return kFskErrNone;
	}
	#ifdef LOG_TEXTURE_LIFE
		LOGD("ReshapeOffscreenTexture(#%u from %s %ux%u to %s %dx%d)", (unsigned)tx->name,
			GLInternalFormatNameFromCode(tx->glIntFormat), (unsigned)tx->bounds.width, (unsigned)tx->bounds.height, GLInternalFormatNameFromCode(internalGLFormat), width, height);
	#endif /* LOG_TEXTURE_LIFE */

	/* (re)allocate texture storage */
	if (width  > gGLGlobalAssets.maxTextureSize) {				/* If the texture is too wide, ... */
		width  = gGLGlobalAssets.maxTextureSize;				/* ... silently truncate it, so it might work for the left part of the image ... */
		sizeErr = kFskErrTextureTooLarge;						/* ... and indicate to the caller that we have done this. */
	}
	if (height > gGLGlobalAssets.maxTextureSize) {				/* If the texture is too tall, ... */
		height = gGLGlobalAssets.maxTextureSize;				/* ... silently truncate it, so it might work for the top part of the image. */
		sizeErr = kFskErrTextureTooLarge;						/* ... and indicate to the caller that we have done this. */
	}

	if (0 == tx->name ||   GL_TEXTURE_UNLOADED == tx->name)
		FskGenGLTexturesAndInit(1, &tx->name);
	else
		CHANGE_TEXTURE(tx->name);
	if (0 != tx->nameU)	{ FskDeleteGLTextures(1, &tx->nameU);	tx->nameU = 0; }
	if (0 != tx->nameV) { FskDeleteGLTextures(1, &tx->nameV);	tx->nameV = 0; }
	BAIL_IF_FALSE((0 != tx->name && GL_TEXTURE_UNLOADED != tx->name), err, kFskErrGraphicsContext);

	glTexImage2D(GL_TEXTURE_2D, 0, internalGLFormat, width, height, 0, srcGLFormat, srcGLType, NULL);	/* Reshape the texture accordingly */
	#if CHECK_GL_ERROR
		err = GetFskGLError();
		if (err) {
			LOGD("Couldn't resize {#%u %dx%d %s}: glTexImage2D(GL_TEXTURE_2D, 0, %s, %d, %d, 0, %s, %s, NULL) returns %s",
				tx->name, tx->bounds.width, tx->bounds.height, GLInternalFormatNameFromCode(tx->glIntFormat), GLInternalFormatNameFromCode(internalGLFormat), width, height,
				GLFormatNameFromCode(srcGLFormat), GLTypeNameFromCode(srcGLFormat), FskInstrumentationGetErrorString(err));
			SET_TEXTURE(tx->name);
			glTexImage2D(GL_TEXTURE_2D, 0, internalGLFormat, width, height, 0, srcGLFormat, srcGLType, NULL);	/* Reshape the texture accordingly */
			err = GetFskGLError();
			LOGD("Second attempt: {#%u %dx%d %s}: glTexImage2D(GL_TEXTURE_2D, 0, %s, %d, %d, 0, %s, %s, NULL) returns %s",
				tx->name, tx->bounds.width, tx->bounds.height, GLInternalFormatNameFromCode(tx->glIntFormat), GLInternalFormatNameFromCode(internalGLFormat), width, height,
				GLFormatNameFromCode(srcGLFormat), GLTypeNameFromCode(srcGLFormat), err ? FskInstrumentationGetErrorString(err) : "kFskErrNone");
		}
	#endif /* CHECK_GL_ERROR */
	if (!err) {
		tx->bounds.x		= 0;
		tx->bounds.y		= 0;
		tx->bounds.width	= width;
		tx->bounds.height	= height;
		tx->glIntFormat		= internalGLFormat;					/* Record the format to facilitate reuse */
		tx->fboInited		= false;
		err = sizeErr;											/* ... report a too large texture error if it occurred */
	}
	#ifdef LOG_MEMORY
		if (LOGD_ENABLED()) {
			char currStr[32], peakStr[32], temsStr[32];
			FskInt64 tems;
			FskGLPort p;
			for (p = gGLGlobalAssets.activePorts, tems=0; p != NULL; p = p->next)
				tems += p->texture.bounds.height * p->texture.bounds.width * ((p->texture.glIntFormat == GL_ALPHA) ? 1 : 4);
			LOGD("     currRSS:%11s     peakRSS:%11s     texMem:%11s", CommaPrintS64(GetCurrentRSS(), currStr),
					CommaPrintS64(GetPeakRSS(), peakStr), CommaPrintS64(tems, temsStr));
		}
	#endif /* LOG_MEMORY */


bail:
	PRINT_IF_ERROR(err, __LINE__, "ReshapeOffscreenTexture");
	return err;
}


/****************************************************************************//**
 * Enlarge the texture to fit the rect of the bitmap, if necessary.
 *	\param[in]	width				the width  of the portion of the bitmap to be uploaded.
 *	\param[in]	height				the height of the portion of the bitmap to be uploaded.
 *	\param[in]	srcGLFormat			the GL format of the source.
 *	\param[in]	srcGLType			the GL  type  of the source.
 *	\param[in]	internalGLFormat	the internal GL format of the texture.
 *	\param[in]	tx					the GL texture record.
 *	\return		kFskErrNone	if the texture was set successfully.
 ********************************************************************************/

static FskErr ReshapeRGBTexture(GLInternalFormat internalGLFormat, GLsizei width, GLsizei height, GLFormat srcGLFormat, GLType srcGLType, GLTexture tx) {
	FskErr err = kFskErrNone;

	if (tx->glIntFormat == internalGLFormat										/* If the given texture has the same format ... */
		&& width  <= tx->bounds.width  && tx->bounds.width  < (width <<1)		/* ...and is large enough ... */
		&& height <= tx->bounds.height && tx->bounds.height < (height<<1)		/* ... but not twice as large, ... */
		&& !tx->nameU && !tx->nameV
	) {
		#ifdef LOG_TEXTURE_LIFE
			LOGD("ReshapeRGBTexture(#%u from %s %ux%u to %s %dx%d -- already %s)", (unsigned)tx->name,
				GLInternalFormatNameFromCode(tx->glIntFormat), (unsigned)tx->bounds.width, (unsigned)tx->bounds.height, GLInternalFormatNameFromCode(internalGLFormat), width, height,
				((width == tx->bounds.width && height == tx->bounds.height) ? "the right shape" : "close enough")
			);
		#endif /* LOG_TEXTURE_LIFE */
		return kFskErrNone;										/* ... no resizing is necessary */
	}
	#ifdef LOG_TEXTURE_LIFE
		LOGD("ReshapeRGBTexture(#%u from %s %ux%u to %s %dx%d)", (unsigned)tx->name,
			GLInternalFormatNameFromCode(tx->glIntFormat), (unsigned)tx->bounds.width, (unsigned)tx->bounds.height, GLInternalFormatNameFromCode(internalGLFormat), width, height);
	#endif /* LOG_TEXTURE_LIFE */

	/* (re)allocate texture storage */
	if (gGLGlobalAssets.hasNPOT) {								/* If the GPU can handle non-power-of-two textures, ... */
		width  = BLOCKIFY(width,  TEXTURE_BLOCK_HORIZONTAL);	/* ... we allocate a texture that has a certain block size for speed of uploading */
		height = BLOCKIFY(height, TEXTURE_BLOCK_VERTICAL);
	}
	else {														/* If the GPU cannot handle non-power-of-two textures, ... */
		width  = SmallestContainingPowerOfTwo(width);			/* ... we allocate a texture that is the next power of two larger */
		height = SmallestContainingPowerOfTwo(height);
	}
	if (width  < TEXTURE_MIN_DIM)								/* For reusability, ... */
		width  = TEXTURE_MIN_DIM;								/* ... we only allocate ... */
	if (height < TEXTURE_MIN_DIM)								/* ... a minimum size ... */
		height = TEXTURE_MIN_DIM;								/* ... of texture. */

	BAIL_IF_ERR(err = ReshapeOffscreenTexture(internalGLFormat, width, height, srcGLFormat, srcGLType, tx));	/* This resizes a scalar texture to exactly (width, height) */

bail:
	PRINT_IF_ERROR(err, __LINE__, "ReshapeRGBTexture");
	return err;
}


/****************************************************************************//**
 * Determine whether the video dimensions meet hardware upload requirements.
 * Since we coerce all video to have an even height, we accept (height & 3) == 3 or 0.
 *	\param[in]	bm		the bitmap
 *	\return		true	if the bitmap meets the requirements;
 *				false	otherwise.
 ********************************************************************************/

#define CanDoHardwareVideo(bm)	(0 == (((bm)->rowBytes & (16-1)) | (((bm)->bounds.height + 1) & 2)))


/****************************************************************************//**
 *	Reshape a texture.
 ********************************************************************************/

static FskErr ReshapeTexture(FskConstBitmap bm, GLTexture tx) {
	FskErr				err			= kFskErrNone;
	UInt32				width		= bm->depth ? (UInt32)(bm->rowBytes / (bm->depth >> 3)) : (UInt32)bm->bounds.width;
	UInt32				height		= bm->bounds.height;
	FskBitmapFormatEnum	pixelFormat = bm->pixelFormat;
	GLInternalFormat	glIntFormat;
	GLFormat			glFormat;
	GLType				glType;

	switch (pixelFormat) {
		case kFskBitmapFormatYUV420:
			if (gGLGlobalAssets.texImageYUV420 && CanDoHardwareVideo(bm)) {
				glIntFormat = glFormat = GL_RGB;
				glType = GL_UNSIGNED_BYTE;
				goto reshapeDefault;
			}
			#if GLES_VERSION == 2
				BAIL_IF_ERR(err = ReshapeYUV420Textures(width, height, tx, GL_LINEAR));
			#else /* GLES_VERSION == 2 */
				err = kFskErrUnsupportedPixelType;
			#endif /* GLES_VERSION == 2 */
			break;
		case kFskBitmapFormatYUV420spuv:
		case kFskBitmapFormatYUV420spvu:
			#if GLES_VERSION == 2
				BAIL_IF_ERR(err = ReshapeYUV420spTextures(width, height, tx, GL_LINEAR));
			#else /* GLES_VERSION == 2 */
				err = kFskErrUnsupportedPixelType;
			#endif /* GLES_VERSION == 2 */
			break;
		case kFskBitmapFormatUYVY:
			#if defined(EGL_VERSION) && defined(BG3CDP_GL)
				if (gGLGlobalAssets.hasImageExternal)
					return kFskErrNone;
			#endif /* #if defined(EGL_VERSION) && defined(BG3CDP_GL) */
			if (gGLGlobalAssets.texImageUYVY && CanDoHardwareVideo(bm)) {
				glIntFormat = glFormat = GL_RGB;
				glType = GL_UNSIGNED_BYTE;
				goto reshapeDefault;
			}
			#if GLES_VERSION == 2
				BAIL_IF_ERR(err = ReshapeUYVYTexture(width, height, tx, 0));			/* Luminance width and height */
			#else /* GLES_VERSION == 2 */
				err = kFskErrUnsupportedPixelType;
			#endif /* GLES_VERSION == 2 */
			break;
		case kFskBitmapFormatGLRGB:
		case kFskBitmapFormatGLRGBA:
			(void)FskGLGetFormatAndTypeFromPixelFormat(pixelFormat, &glFormat, &glType, &glIntFormat);
			BAIL_IF_ERR(err = ReshapeOffscreenTexture(glIntFormat, width, height, glFormat, glType, tx));
			break;
		default:
			if (!FskGLHardwareSupportsPixelFormat(pixelFormat)) switch (bm->depth) {	/* If GL does not support this pixel type, we can quickly convert to another format */
				case 32:	pixelFormat = kFskBitmapFormat32RGBA;		break;
				case 24:	pixelFormat = kFskBitmapFormat24RGB;		break;
				case 16:	pixelFormat = kFskBitmapFormat16RGB565LE;	break;
				default:	BAIL(kFskErrUnsupportedPixelType);
			}
			err = FskGLGetFormatAndTypeFromPixelFormat(pixelFormat, &glFormat, &glType, &glIntFormat);
			BAIL_IF_ERR(err);
		reshapeDefault:
			BAIL_IF_ERR(err = ReshapeRGBTexture(glIntFormat, width, height, glFormat, glType, tx));
			break;
	}

bail:
	PRINT_IF_ERROR(err, __LINE__, "ReshapeTexture");
	return err;
}


/****************************************************************************//**
 * Set the given GL texture from a bitmap.
 * We allocate a smaller bitmap if the bitmap is too large.
 *	\param[in]	internalGLFormat	the internal format used by GL
 *	\param[in]	width				the width of the image
 *	\param[in]	height				the height of the image
 *	\param[in]	rowPix				the number of pixels from one line to the next (rowPix >= width)
 *	\param[in]	srcGLFormat			the format of the source pixels
 *	\param[in]	srcGLType			the type of the source pixels
 *	\param[in]	tx					the texture descriptor
 *	\param[in]	topLeft				pointer to the top left pixel of the image
 *	\return		kFskErrNone	if the texture was set successfully.
 ********************************************************************************/

static FskErr
ShapeAndLoadTexture(GLInternalFormat internalGLFormat, GLsizei width, GLsizei height, GLint rowPix, GLFormat srcGLFormat, GLType srcGLType, GLTexture tx, const GLvoid *srcTopLeft)
{
	FskErr		err		= kFskErrNone;
	FskBitmap	srcBM	= NULL,
				dstBM	= NULL;
	GLsizei		glWidth;

	#if defined(LOG_SET_TEXTURE) || defined(LOG_TEXTURE_LIFE)
		LOGD("ShapeAndLoadTexture(iglFmt=%s width=%d height=%d rowPix=%d glFormat=%s, glType=%s tx=%p, topLeft=%p)",
			GLInternalFormatNameFromCode(internalGLFormat), width, height, rowPix, GLFormatNameFromCode(srcGLFormat), GLTypeNameFromCode(srcGLType), tx, srcTopLeft
		);
	#endif /*  defined(LOG_SET_TEXTURE) || defined(LOG_TEXTURE_LIFE) */

	#ifdef GL_UNPACK_ROW_LENGTH
		#ifdef LOG_SET_TEXTURE
			LOGD("GL_UNPACK_ROW_LENGTH");
		#endif /* LOG_SET_TEXTURE */
		glPixelStorei(GL_UNPACK_ROW_LENGTH, rowPix);
		glWidth = width;
	#else /* !GL_UNPACK_ROW_LENGTH */
		glWidth = rowPix;
	#endif /* GL_UNPACK_ROW_LENGTH */

	err = ReshapeRGBTexture(internalGLFormat, glWidth, height, srcGLFormat, srcGLType, tx);
	PRINT_IF_ERROR(err, __LINE__, "ReshapeRGBTexture");

	if (!err) {
		#ifdef LOG_SET_TEXTURE
			LOGD("Calling #%u glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, glWidth=%d, height=%d, srcGLFormat=%s, srcGLType=%s, srcTopLeft=%p; embed(%d, %d) in texture(%u, %u))",
				tx->name, glWidth, height, GLFormatNameFromCode(srcGLFormat), GLTypeNameFromCode(srcGLType), srcTopLeft, width, height, (unsigned)tx->bounds.width, (unsigned)tx->bounds.height
			);
		#endif /* LOG_SET_TEXTURE */
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, glWidth, height, srcGLFormat, srcGLType, srcTopLeft);

		#if CHECK_GL_ERROR
			err = GetFskGLError();
			if (err) {
				glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, glWidth, height, srcGLFormat, srcGLType, srcTopLeft);
				err = GetFskGLError();
				LOGE("glTexSubImage2D failed. Second attempt %s", (err ? "failed" : "succeeded"));
			}
			if ((kFskErrInvalidParameter == err) && ((rowPix > tx->bounds.width) || (height > tx->bounds.height)))
				err = kFskErrTextureTooLarge;
			#if GL_DEBUG
				PRINT_IF_ERROR(err, __LINE__, "glTexSubImage2D");
			#endif /* GL_DEBUG */
			BAIL_IF_ERR(err);
		#endif /* CHECK_GL_ERROR */
	}

	if (err && ((glWidth > tx->bounds.width) || (height > tx->bounds.height))) {
		SInt32				pixBytes, rowBytes;
		FskBitmapFormatEnum	pixelFormat;
		FskRectangleRecord	r;
		FskColorRGBARecord	color;

		LOGE("The supplied texture was too big and ReshapeRGBTexture FAILED");

		switch (srcGLType) {
			case GL_UNSIGNED_BYTE:
				switch (srcGLFormat) {
					case GL_RGBA:					pixBytes = 4;	break;
					case GL_RGB:					pixBytes = 3;	break;
					case GL_LUMINANCE:				pixBytes = 1;	break;
					case GL_ALPHA:					pixBytes = 1;	break;
					#ifdef    GL_BGRA
						case  GL_BGRA:				pixBytes = 4;	break;
					#endif /* GL_BGRA */
					#ifdef    GL_BGR
						case  GL_BGR:				pixBytes = 3;	break;
					#endif /* GL_BGR */
					default:						goto bail;
				}
				break;
			#ifdef    GL_UNSIGNED_INT_8_8_8_8
				case  GL_UNSIGNED_INT_8_8_8_8:		pixBytes = 4;	break;
			#endif /* GL_UNSIGNED_INT_8_8_8_8 */
			#ifdef    GL_UNSIGNED_INT_8_8_8_8_REV
				case  GL_UNSIGNED_INT_8_8_8_8_REV:	pixBytes = 4;	break;
			#endif /* GL_UNSIGNED_INT_8_8_8_8_REV */
			#ifdef    GL_UNSIGNED_SHORT_4_4_4_4
				case  GL_UNSIGNED_SHORT_4_4_4_4:	pixBytes = 2;	break;
			#endif /* GL_UNSIGNED_SHORT_4_4_4_4 */
			#ifdef    GL_UNSIGNED_SHORT_5_6_5
				case  GL_UNSIGNED_SHORT_5_6_5:		pixBytes = 2;	break;
			#endif /* GL_UNSIGNED_SHORT_5_6_5 */
			#ifdef    GL_UNSIGNED_SHORT_5_6_5_REV
				case  GL_UNSIGNED_SHORT_5_6_5_REV:	pixBytes = 2;	break;
			#endif /* GL_UNSIGNED_SHORT_5_6_5_REV */
			#ifdef    GL_UNSIGNED_SHORT_8_8_APPLE
				case  GL_UNSIGNED_SHORT_8_8_APPLE:	pixBytes = 2;	break;
			#endif /* GL_UNSIGNED_SHORT_8_8_APPLE */
			default:								goto bail;
		}
		rowBytes = rowPix * pixBytes;
		switch (pixBytes) {
			default:
			case 1:	pixelFormat = kFskBitmapFormat8G;			break;
			case 2:	pixelFormat = kFskBitmapFormat16RGB565LE;	break;
			case 3:	pixelFormat = kFskBitmapFormat24BGR;		break;
			case 4:	pixelFormat = kFskBitmapFormatDefaultRGBA;	break;
		}
		BAIL_IF_ERR(err = FskBitmapNewWrapper(width, height, pixelFormat, pixBytes<<3, (void*)srcTopLeft, rowBytes, &srcBM));
		if (width  > tx->bounds.width)
			width  = tx->bounds.width;
		if (height > tx->bounds.height)
			height = tx->bounds.height;
		BAIL_IF_ERR(err = FskBitmapNew(width, height, pixelFormat, &dstBM));
		BAIL_IF_ERR(err = FskBitmapDraw(srcBM, &srcBM->bounds, dstBM, &srcBM->bounds, NULL, NULL, kFskGraphicsModeCopy, NULL));
		LOGD("Clipping large image from %dx%d to %dx%d", (int)srcBM->bounds.width, (int)srcBM->bounds.height, (int)dstBM->bounds.width, (int)dstBM->bounds.height);
		FskColorRGBASet(&color, 0, 0, 0, 0);
		if (srcBM->bounds.width > dstBM->bounds.width) {							/* If the image is too wide ...   */
			FskRectangleSet(&r, dstBM->bounds.width-1, 0, 1, dstBM->bounds.height);	/* ... clear the right column ... */
			FskRectangleFill(dstBM, &r, &color, kFskGraphicsModeCopy, NULL);		/* ... to black */
		}
		if (srcBM->bounds.height > dstBM->bounds.height) {							/* If the image is too tall ... */
			FskRectangleSet(&r, 0, dstBM->bounds.height-1, dstBM->bounds.width, 1);	/* ... clear the bottom row ... */
			FskRectangleFill(dstBM, &r, &color, kFskGraphicsModeCopy, NULL);		/* ... to black */
		}
		FskBitmapReadBegin(dstBM, &srcTopLeft, &rowBytes, NULL);
		err = ShapeAndLoadTexture(internalGLFormat, width, height, rowBytes/pixBytes, srcGLFormat, srcGLType, tx, srcTopLeft);
		FskBitmapReadEnd(dstBM);
		if (!err)
			err = kFskErrTextureTooLarge;
	}

bail:
	PRINT_IF_ERROR(err, __LINE__, "ShapeAndLoadTexture");
	if (srcBM) FskBitmapDispose(srcBM);
	if (dstBM) FskBitmapDispose(dstBM);
	#ifdef GL_UNPACK_ROW_LENGTH
		glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	#endif /* GL_UNPACK_ROW_LENGTH */
	return err;
}


#if GLES_VERSION == 2

/********************************************************************************
 * SetGLYUV420Textures.
 * This is used for our cross-platform YUV420 shader.
 ********************************************************************************/

static FskErr SetGLYUV420Textures(FskConstBitmap yuvBM, GLTexture tx, GLint glQuality) {
	FskErr	err	= kFskErrNone;
	UInt32	widthLuma, widthChroma, heightLuma, heightChroma;
	SInt32	rb;
	UInt8	*yuv[3];

	#ifdef LOG_YUV
		LOGD("SetGLYUV420Textures(%p, %p, %s)", yuvBM, tx, GLQualityNameFromCode(glQuality));
	#endif /* LOG_YUV */

	/* Get pointers to Y, U, V */
	if (kFskErrNone != (err = FskBitmapReadBegin((FskBitmap)yuvBM, (const void**)(const void*)(&yuv[0]), &rb, NULL)))
		return err;
	widthLuma    = rb;																						/* The width of the texture includes all rowBytes */
	widthChroma  = rb >> 1;
	heightLuma   =  yuvBM->bounds.height;
	heightChroma = (yuvBM->bounds.height + 1) >> 1;
	yuv[1] = yuv[0] + widthLuma   * heightLuma;																/* U follows Y */
	yuv[2] = yuv[1] + widthChroma * heightChroma;															/* V follows U (note we guard against odd height) */

	BAIL_IF_ERR(err = ReshapeYUV420Textures(widthLuma, heightLuma, tx, glQuality));							/* Resize the textures if necessary */


	/*
	 * Upload textures
	 */

	/* V */
	glActiveTexture(GL_TEXTURE2);
	SET_TEXTURE(tx->nameV);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, widthChroma, heightChroma, GL_ALPHA, GL_UNSIGNED_BYTE, yuv[2]);	/* Upload V texture */
	#if CHECK_GL_ERROR
		BAIL_IF_ERR(err = GetFskGLError());
	#endif /* CHECK_GL_ERROR */

	/* U */
	glActiveTexture(GL_TEXTURE1);
	SET_TEXTURE(tx->nameU);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, widthChroma, heightChroma, GL_ALPHA, GL_UNSIGNED_BYTE, yuv[1]);	/* Upload U texture */
	#if CHECK_GL_ERROR
		BAIL_IF_ERR(err = GetFskGLError());
	#endif /* CHECK_GL_ERROR */

	/* Y */
	glActiveTexture(GL_TEXTURE0);																			/* Always leave GL_TEXTURE0 as the active texture */
	SET_TEXTURE(tx->name);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, widthLuma, heightLuma, GL_ALPHA, GL_UNSIGNED_BYTE, yuv[0]);		/* Upload Y texture */
	#if CHECK_GL_ERROR
		BAIL_IF_ERR(err = GetFskGLError());
	#endif /* CHECK_GL_ERROR */

bail:
	FskBitmapReadEnd((FskBitmap)yuvBM);
	return err;
}


/********************************************************************************
 * SetGLYUV420spTextures.
 * This is used for our cross-platform YUV420sp shader.
 ********************************************************************************/

static FskErr SetGLYUV420spTextures(FskConstBitmap yuvBM, GLTexture tx, GLint glQuality) {
	FskErr	err	= kFskErrNone;
	UInt32	widthLuma, widthChroma, heightLuma, heightChroma;
	SInt32	rb;
	UInt8	*yuv[2];

	#ifdef LOG_YUV
		LOGD("SetGLYUV420spTextures(%p, %p, %s)", yuvBM, tx, GLQualityNameFromCode(glQuality));
	#endif /* LOG_YUV */

	/* Get pointers to Y, UV */
	if (kFskErrNone != (err = FskBitmapReadBegin((FskBitmap)yuvBM, (const void**)(const void*)(&yuv[0]), &rb, NULL)))
		return err;
	widthLuma    = rb;																						/* The width of the texture includes all rowBytes */
	widthChroma  = rb >> 1;
	heightLuma   =  yuvBM->bounds.height;
	heightChroma = (yuvBM->bounds.height + 1) >> 1;
	yuv[1] = yuv[0] + widthLuma * heightLuma;																/* UV follows Y */

	BAIL_IF_ERR(err = ReshapeYUV420spTextures(widthLuma, heightLuma, tx, glQuality));						/* Resize the textures if necessary */


	/*
	 * Upload textures
	 */

	/* UV */
	glActiveTexture(GL_TEXTURE1);
	SET_TEXTURE(tx->nameU);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, widthChroma, heightChroma, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, yuv[1]);	/* Upload UV texture */
	#if CHECK_GL_ERROR
		BAIL_IF_ERR(err = GetFskGLError());
	#endif /* CHECK_GL_ERROR */

	/* Y */
	glActiveTexture(GL_TEXTURE0);																			/* Always leave GL_TEXTURE0 as the active texture */
	SET_TEXTURE(tx->name);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, widthLuma, heightLuma, GL_ALPHA, GL_UNSIGNED_BYTE, yuv[0]);		/* Upload Y texture */
	#if CHECK_GL_ERROR
		BAIL_IF_ERR(err = GetFskGLError());
	#endif /* CHECK_GL_ERROR */

bail:
	FskBitmapReadEnd((FskBitmap)yuvBM);
	return err;
}

#ifndef   TARGET_RT_BIG_ENDIAN
#define   TARGET_RT_BIG_ENDIAN 0
#endif /* TARGET_RT_BIG_ENDIAN */
#ifndef   TARGET_RT_LITTLE_ENDIAN
#define   TARGET_RT_LITTLE_ENDIAN 0
#endif /* TARGET_RT_LITTLE_ENDIAN */

/** Upload a UYVY texture for use by the 422 shader. Do not use this for hardware UYVY upload.
 *  UYVY is 2 bytes per pixel, but we upload 4 bytes per pixel at half the width.
 *	\param[in]		srcBM	the source bitmap
 *	\param[in,out]	tx		the texture structure pointer.
 *	\param[in]		glQuality	the desired quality: GL_NEAREST, GL_LINEAR.
 *	\return			kFskErrNone	if successful.
 */

static FskErr SetGLUYVYTexture(FskConstBitmap srcBM, GLTexture tx, GLint glQuality) {
	FskErr				err		= kFskErrNone;
	UInt8				*yuv;
	SInt32				rb;
	UInt32				widthLuma, widthChroma, height;
	GLInternalFormat	glIntFmt;
	GLFormat			glFormat;
	GLType				glType;

	#ifdef LOG_YUV
		LOGD("SetGLUYVYTexture(%p, %p, %s)", srcBM, tx, GLQualityNameFromCode(glQuality));
	#endif /* LOG_YUV */

	/* Get pointers to UYVY, as well as width, height and rowBytes */
	BAIL_IF_ERR(err = FskBitmapReadBegin((FskBitmap)srcBM, (const void**)(const void*)&yuv, &rb, NULL));
	widthLuma   = rb >> 1;																			/* Luminance   width, including all rowbytes. */
	widthChroma = rb >> 2;																			/* Chrominance width, including all rowbytes. */
	height = srcBM->bounds.height;

	CHANGE_TEXTURE(tx->name);																		/* The last texture bound to TEXTURE0 */

	if (GL_CLAMP_TO_EDGE != tx->wrapMode) {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		tx->wrapMode = GL_CLAMP_TO_EDGE;
	}
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);															/* UYVY */

	#ifdef GL_RGB_422_APPLE
		if (gGLGlobalAssets.hasAppleRGB422) {
			const int	cbIndex		= 0;	/* 0: UYVY, 1:YUYV, 2: VYUY, 3:YVYU */
			GLType		glType;
			if (tx->filter != glQuality) {
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, glQuality);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, glQuality);
				tx->filter = glQuality;
			}

			/* Upload texture */
			if      ((TARGET_RT_LITTLE_ENDIAN && cbIndex == 0) || (TARGET_RT_BIG_ENDIAN && cbIndex == 1))	glType = GL_UNSIGNED_SHORT_8_8_APPLE;
			else if ((TARGET_RT_LITTLE_ENDIAN && cbIndex == 1) || (TARGET_RT_BIG_ENDIAN && cbIndex == 0))	glType = GL_UNSIGNED_SHORT_8_8_REV_APPLE;
			else																							BAIL(kFskErrUnsupportedPixelType);
			if ((unsigned)tx->bounds.width == widthLuma && (unsigned)tx->bounds.height == height && tx->glIntFormat == GL_RGB) {	/* glTexSubImage2D is sometimes faster */
				glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, tx->bounds.width, tx->bounds.height, GL_RGB_422_APPLE, glType, yuv);
			}
			else {
				tx->bounds.width  = widthLuma;
				tx->bounds.height = height;
				#if !TARGET_OS_IPHONE	/* Mac likes GL_RGB and not GL_RGB_422_APPLE for the internal format */
					tx->glIntFormat = GL_RGB;
				#else /* TARGET_OS_IPHONE  IOS likes GL_RGB_422_APPLE and not GL_RGB for the internal format -- not yet proven */
					tx->glIntFormat = GL_RGB_422_APPLE;
				#endif /* TARGET_OS_IPHONE */
				glTexImage2D(GL_TEXTURE_2D, 0, tx->glIntFormat, tx->bounds.width, tx->bounds.height, 0, GL_RGB_422_APPLE, glType, yuv);
			}
		} else
	#endif /* GL_RGB_422_APPLE */
	{
		BAIL_IF_ERR(err = FskGLGetFormatAndTypeFromPixelFormat(kFskBitmapFormat32RGBA, &glFormat, &glType, &glIntFmt));	/* UYVY */
		BAIL_IF_ERR(ReshapeUYVYTexture(widthLuma, height, tx, glQuality));							/* Resize the texture is necessary (this takes luminance width) */
		if (tx->filter != GL_NEAREST) {																/* UYVY requires point sampling, because interpolation is done in the shader */
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);						/* We do interpolation in the shader */
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);						/* We do interpolation in the shader */
			tx->filter = GL_NEAREST;																/* Always set filtering to NEAREST */
		}

		/* Upload texture */
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, widthChroma, height, glFormat, glType, yuv);		/* We use chrominance width here */
	}

	#if CHECK_GL_ERROR
		err = GetFskGLError();
		PRINT_IF_ERROR(err, __LINE__, "glTexImage2D");
	#endif /* CHECK_GL_ERROR */

bail:
	FskBitmapReadEnd((FskBitmap)srcBM);
	return err;
}

#endif /* GLES_VERSION == 2 */


#if defined(EGL_VERSION) && defined(BG3CDP_GL)
/********************************************************************************
 * BG3CDPBitmapDispose
 ********************************************************************************/

static void BG3CDPBitmapDispose(FskBitmap bm, void *refcon) {
	if (bm->lastImageKhr)
		eglDestroyImageKHR(gGLGlobalAssets.blitContext->eglDisplay, bm->lastImageKhr);
	if (bm->lastInternalSurf)
		gcoSURF_Destroy(bm->lastInternalSurf);
	bm->doDispose = NULL;
	(void)FskBitmapDispose(bm);
}


/****************************************************************************//**
 * Allocate a new GC surface.
 * This was derived from alloc_surface() in gc_surf.c.
 *	\param[in]		width		the width of the surface.
 *	\param[in]		height		the height of the surface.
 *	\param[in]		rowBytes	the number of bytes between pixels vertically.
 *	\param[in]		format		the format of the pixels.
 *	\param[in,out]	pPhys		pointer to a location that has the physical address.
 *	\param[in,out]	pVirt		pointer to a location that has the virtual address.
 *								If either *phys==NULL or *virt== NULL, this will allocate a buffer;
 *								otherwise, it uses the provided physical and virtual addresses.
 *	\param[out]		pSurf		pointer to a location to store the new surface.
 *	\return			kFskErrNone				if the surface was allocated successfully.
 *	\return			kFskErrEGLBadSurface	if the surface was not able to be created.
 ********************************************************************************/

static FskErr GCSurfaceNew(
    gctINT32		width,
    gctINT32		height,
    gctINT32		rowBytes,
    gceSURF_FORMAT	format,
    void			**pPhys,
    void			**pVirt,
    gcoSURF			*pSurf
) {
	FskErr		err		= kFskErrNone;
    gcePOOL		pool;
    gcoHAL		hal;
    gcoOS		os;
    gcoSURF		surface;
    gctPOINTER	memory[3];
    gctUINT		address[3];

	*pSurf = NULL;
	BAIL_IF_NEGATIVE(gcoOS_Construct(NULL, &os), err, kFskErrEGLBadSurface);
	BAIL_IF_NEGATIVE(gcoHAL_Construct(NULL, os, &hal), err, kFskErrEGLBadSurface);
	pool = (*pVirt && *pPhys) ? gcvPOOL_USER : gcvPOOL_DEFAULT;
    BAIL_IF_NEGATIVE(gcoSURF_Construct(NULL, width, height, 1, gcvSURF_BITMAP, format, pool, &surface), err, kFskErrEGLBadSurface);
    if (pool == gcvPOOL_USER) {
        BAIL_IF_NEGATIVE(gcoSURF_SetBuffer(surface, gcvSURF_BITMAP, format, rowBytes, *pVirt, (gctUINT32)(*pPhys)), err, kFskErrEGLBadSurface);
        BAIL_IF_NEGATIVE(gcoSURF_SetWindow(surface, 0, 0, width, height), err, kFskErrEGLBadSurface);
    }
    BAIL_IF_NEGATIVE(gcoSURF_Lock(surface, address, memory), err, kFskErrEGLBadSurface);

    *pSurf = surface;
    *pPhys = (void*)address[0];
    *pVirt = memory[0];

bail:
	return err;
}


/********************************************************************************
 * SetEXTTexture
 ********************************************************************************/

static FskErr SetEXTTexture(FskConstBitmap srcBM, FskConstRectangle texRect, GLTexture tx) {
	FskErr			err		= kFskErrNone;
	FskBitmap		SRCBM	= (FskBitmap)srcBM;												/* Remove the const */
	EGLClientBuffer	clientBuffer;
	gceSURF_FORMAT	yuvTexFormat;

	if (NULL == SRCBM->physicalAddr)
		BAIL(kFskErrUnimplemented);

	switch (SRCBM->pixelFormat) {
		case kFskBitmapFormatUYVY:			yuvTexFormat = gcvSURF_UYVY;	break;
		default:							BAIL(kFskErrUnimplemented);
	}

	/* Allocate a proprietary gco surface */
	if (NULL == SRCBM->lastInternalSurf) {
		#if defined(LOG_YUV)
			LOGD("GCSurfaceNew(width=%d, height=%d, rowBytes=%d, yuvTexFormat=%d, phys=%p, virt=%pm, pSurf=%p)",
				(int)SRCBM->bounds.width, (int)SRCBM->bounds.height, (int)SRCBM->rowBytes, yuvTexFormat, &SRCBM->physicalAddr, &SRCBM->bits, &SRCBM->lastInternalSurf);
		#endif /* LOG_YUV */
		BAIL_IF_ERR(err = GCSurfaceNew(SRCBM->bounds.width, SRCBM->bounds.height, SRCBM->rowBytes, yuvTexFormat, &SRCBM->physicalAddr, &SRCBM->bits, ((gcoSURF*)(&SRCBM->lastInternalSurf))));
		SRCBM->doDispose = &BG3CDPBitmapDispose;										/* Make sure to dispose the lastInternalSurf when the bitmap is disposed */
	}
	clientBuffer = (EGLClientBuffer)(SRCBM->lastInternalSurf);


	/* Create a KHR image from the gco surface */
	if (!gGLGlobalAssets.blitContext)													/* Make sure that we have a blit context */
		gGLGlobalAssets.blitContext = &gGLGlobalAssets.defaultContext;

	if (NULL == SRCBM->lastImageKhr) {
		#if defined(LOG_YUV)
			LOGD("eglCreateImageKHR(display=%p, context=%p, target=%d, buffer=%p, attrib_list=%p)",
				gGLGlobalAssets.blitContext->eglDisplay, EGL_NO_CONTEXT, EGL_NATIVE_BUFFER_ANDROID, clientBuffer, NULL);
		#endif /* LOG_YUV */
		SRCBM->lastImageKhr = eglCreateImageKHR(gGLGlobalAssets.blitContext->eglDisplay, EGL_NO_CONTEXT, EGL_NATIVE_BUFFER_ANDROID, clientBuffer, NULL);
		SRCBM->doDispose = &BG3CDPBitmapDispose;										/* Make sure to dispose the lastImageKhr when the bitmap is disposed */
		#if CHECK_GL_ERROR
			err = GetFskEGLError();
			PRINT_IF_ERROR(err, __LINE__, "eglCreateImageKHR(%p, %p, %d, %p, %p)",
				gGLGlobalAssets.blitContext->eglDisplay, EGL_NO_CONTEXT, EGL_NATIVE_BUFFER_ANDROID, clientBuffer, 0);
		#endif /* CHECK_GL_ERROR */
		BAIL_IF_FALSE(EGL_NO_IMAGE_KHR != SRCBM->lastImageKhr, err, kFskErrEGLBadNativePixmap);
	}


	/* Allocate the texture, if not already allocated. */
	if (0 == tx->name || GL_TEXTURE_UNLOADED == tx->name)
		BAIL_IF_ERR(err = FskGenGLTexturesAndInit(1, &tx->name));
	FskRectangleSet(&tx->bounds, 0, 0, SRCBM->bounds.width, SRCBM->bounds.height);
	tx->glIntFormat	= yuvTexFormat;
	tx->srcBM		= srcBM;

	/* Bind the texture */
	CHANGE_TEXTURE(tx->name);
	gGLGlobalAssets.blitContext->lastTexture = err ? 0 : tx->name;							/* These texture loaders do not set lastTexture */
	#if CHECK_GL_ERROR
		err = GetFskGLError();
		PRINT_IF_ERROR(err, __LINE__, "glBindTexture(%d)", tx->name);
	#endif /* CHECK_GL_ERROR */

	/* Upload the texture */
	#if defined(LOG_YUV)
		LOGD("glEGLImageTargetTexture2DOES(%d, %p)", GL_TEXTURE_EXTERNAL_OES, SRCBM->lastImageKhr);
	#endif /* LOG_YUV */
	glEGLImageTargetTexture2DOES(GL_TEXTURE_EXTERNAL_OES, (GLeglImageOES)(SRCBM->lastImageKhr));
	#if CHECK_GL_ERROR
		err = GetFskGLError();
		PRINT_IF_ERROR(err, __LINE__, "glEGLImageTargetTexture2DOES(%d, %p)", GL_TEXTURE_EXTERNAL_OES, SRCBM->lastImageKhr);
	#endif /* CHECK_GL_ERROR */

	/* Set texture properties */
	if (tx->filter == 0) {	/* If filter mode has not been set, initialize it to GL_NEAREST; this may be overridden in the blit later */
		glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		tx->filter = GL_NEAREST;
		#if CHECK_GL_ERROR
			err = GetFskGLError();
			PRINT_IF_ERROR(err, __LINE__, "glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_***_FILTER)");
		#endif /* CHECK_GL_ERROR */
	}
	if (tx->wrapMode == 0) {
		glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		tx->wrapMode = GL_CLAMP_TO_EDGE;
		#if CHECK_GL_ERROR
			err = GetFskGLError();
			PRINT_IF_ERROR(err, __LINE__, "glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_*)");
		#endif /* CHECK_GL_ERROR */
	}

bail:
	return err;
}
#endif /* BG3CDP_GL */


/****************************************************************************//**
 * Set the given GL texture from a bitmap.
 *	\param[in]	srcBM	the source bitmap.
 *	\param[in]	texRect	the bounds of the texture.
 *	\param[in]	tx		the GL texture record.
 *	\return		kFskErrNone	if the texture was set successfully.
 ********************************************************************************/

static FskErr SetGLTexture(FskConstBitmap srcBM, FskConstRectangle texRect, GLTexture tx)
{
	FskErr				err			= kFskErrNone;
	FskBitmapFormatEnum	cnvFormat	= (FskBitmapFormatEnum)(0xFFFFFFFFU);
	const UInt8			*srcTopLeft;				/* The top left of the rect that we are uploading */
	FskBitmapFormatEnum	srcPixelFormat;				/* The Fsk pixel format */
	SInt32				srcRowPix, srcRowBytes;		/* The number of pixels and bytes per row, respectively */
	GLenum				srcGLFormat, srcGLType;		/* The GL external format an type of the texture to be uploaded */
	GLint				internalGLFormat;			/* The internal GL format of the texture */
	FskRectangleRecord	srcRect;					/* The rectangle to be uploaded */

	#if FSK_DEBUG
		if (srcBM == NULL || tx == NULL)
			return kFskErrInvalidParameter;
	#endif /* FSK_DEBUG */

	tx->flipY = false;	/* Indicate that we loaded the texture, as opposed to being loaded from the screen with the opposite vertical polarity */
	/* Use special YUV loaders if they exist */
	switch (srcBM->pixelFormat) {
		case kFskBitmapFormatYUV420:											/* Everyone accommodates YUV420 */
			if (gGLGlobalAssets.texImageYUV420 && CanDoHardwareVideo(srcBM)) {
				#if defined(LOG_SET_TEXTURE) || defined(LOG_YUV)
					LOGD("SetGLTexture loading %dx%d YUV420 native texture #%u from %p", srcBM->bounds.width, srcBM->bounds.height, tx->name, srcBM);
				#endif /* LOG_SET_TEXTURE */
				if (0 == tx->name || GL_TEXTURE_UNLOADED == tx->name)
					err = FskGenGLTexturesAndInit(1, &tx->name);
				if (!err)
					err = gGLGlobalAssets.texImageYUV420(srcBM, tx);			/* System specific YUV420 loader */
				gGLGlobalAssets.blitContext->lastTexture = err ? 0 : tx->name;	/* These texture loaders do not set lastTexture */
				return err;
			}
			#if GLES_VERSION == 2
				#if defined(LOG_SET_TEXTURE) || defined(LOG_YUV)
					LOGD("SetGLYUV420Textures loading %dx%d YUV420 shader texture {#%u, #%u, #%u} from %p", srcBM->bounds.width, srcBM->bounds.height, tx->name, tx->nameU, tx->nameV, srcBM);
				#endif /* LOG_SET_TEXTURE */
				return SetGLYUV420Textures(srcBM, tx, GL_LINEAR);				/* Shader-based YUV420 loader; this sets lastTexture */
			#endif /* GLES_VERSION == 2 */
			break;
		case kFskBitmapFormatYUV420spuv:
		case kFskBitmapFormatYUV420spvu:
			#if GLES_VERSION == 2
				#if defined(LOG_SET_TEXTURE) || defined(LOG_YUV)
					LOGD("SetGLYUV420spTextures loading %dx%d YUV420sp shader texture {#%u, #%u} from %p", srcBM->bounds.width, srcBM->bounds.height, tx->name, tx->nameU, srcBM);
				#endif /* LOG_SET_TEXTURE */
				return SetGLYUV420spTextures(srcBM, tx, GL_LINEAR);				/* Shader-based YUV420sp loader; this sets lastTexture */
			#endif /* GLES_VERSION == 2 */
			break;
		case kFskBitmapFormatUYVY:
#if defined(EGL_VERSION) && defined(BG3CDP_GL)
#ifdef EXTRA_CRISPY
			if (FskGLHardwareSupportsYUVPixelFormat(kFskBitmapFormatUYVY) && (kFskErrNone == (err = SetEXTTexture(srcBM, texRect, tx)))) return err;
#else /* ORIGINAL */
			if (gGLGlobalAssets.hasImageExternal && srcBM->physicalAddr) {
				FskBitmap SRCBM = (FskBitmap)srcBM;													/* Remove the const */
				const gceSURF_FORMAT yuvTexFormat = gcvSURF_UYVY;									/* TODO: Accommodate other image formats */

				/* Make sure that we have a blit context */
				if (!gGLGlobalAssets.blitContext)
					gGLGlobalAssets.blitContext = &gGLGlobalAssets.defaultContext;

				/* Allocate a proprietary gco surface */
				if (NULL == SRCBM->lastInternalSurf) {
					#if defined(LOG_YUV)
						LOGD("GCSurfaceNew(width=%d, height=%d, rowBytes=%d, yuvTexFormat=%d, phys=%p, virt=%pm, pSurf=%p)",
							(int)SRCBM->bounds.width, (int)SRCBM->bounds.height, (int)SRCBM->rowBytes, yuvTexFormat, &SRCBM->physicalAddr, &SRCBM->bits, &SRCBM->lastInternalSurf);
					#endif /* LOG_YUV */
					BAIL_IF_ERR(err = GCSurfaceNew(SRCBM->bounds.width, SRCBM->bounds.height, SRCBM->rowBytes, yuvTexFormat, &SRCBM->physicalAddr, &SRCBM->bits, &SRCBM->lastInternalSurf));
					SRCBM->doDispose = &BG3CDPBitmapDispose;										/* Make sure to dispose the lastInternalSurf when the bitmap is disposed */
				}

				/* Create a KHR image from the gco surface */
				if (NULL == SRCBM->lastImageKhr) {
					EGLClientBuffer	clientBuffer = (EGLClientBuffer)(SRCBM->lastInternalSurf);
					#if defined(LOG_YUV)
						LOGD("eglCreateImageKHR(display=%p, context=%p, target=%d, buffer=%p, attrib_list=%p)",
							gGLGlobalAssets.blitContext->eglDisplay, EGL_NO_CONTEXT, EGL_NATIVE_BUFFER_ANDROID, clientBuffer, NULL);
					#endif /* LOG_YUV */
					SRCBM->lastImageKhr = eglCreateImageKHR(gGLGlobalAssets.blitContext->eglDisplay, EGL_NO_CONTEXT, EGL_NATIVE_BUFFER_ANDROID, clientBuffer, NULL);
					SRCBM->doDispose = &BG3CDPBitmapDispose;										/* Make sure to dispose the lastImageKhr when the bitmap is disposed */
					#if CHECK_GL_ERROR
						err = GetFskEGLError();
						PRINT_IF_ERROR(err, __LINE__, "eglCreateImageKHR(%p, %p, %d, %p, %p)",
							gGLGlobalAssets.blitContext->eglDisplay, EGL_NO_CONTEXT, EGL_NATIVE_BUFFER_ANDROID, clientBuffer, 0);
					#endif /* CHECK_GL_ERROR */
					BAIL_IF_FALSE(EGL_NO_IMAGE_KHR != SRCBM->lastImageKhr, err, kFskErrEGLBadNativePixmap);
				}

				/* Allocate the texture, if not already allocated. */
				if (0 == tx->name || GL_TEXTURE_UNLOADED == tx->name)
					BAIL_IF_ERR(err = FskGenGLTexturesAndInit(1, &tx->name));
				FskRectangleSet(&tx->bounds, 0, 0, SRCBM->bounds.width, SRCBM->bounds.height);
				tx->glIntFormat	= yuvTexFormat;
				tx->srcBM		= srcBM;

				/* Bind the texture */
				CHANGE_TEXTURE(tx->name);
				gGLGlobalAssets.blitContext->lastTexture = err ? 0 : tx->name;					/* These texture loaders do not set lastTexture */
				#if CHECK_GL_ERROR
					err = GetFskGLError();
					PRINT_IF_ERROR(err, __LINE__, "glBindTexture(%d)", tx->name);
				#endif /* CHECK_GL_ERROR */

				/* Upload the texture */
				#if defined(LOG_YUV)
					LOGD("glEGLImageTargetTexture2DOES(%d, %p)", GL_TEXTURE_EXTERNAL_OES, SRCBM->lastImageKhr);
				#endif /* LOG_YUV */
				glEGLImageTargetTexture2DOES(GL_TEXTURE_EXTERNAL_OES, (GLeglImageOES)(SRCBM->lastImageKhr));
				#if CHECK_GL_ERROR
					err = GetFskGLError();
					PRINT_IF_ERROR(err, __LINE__, "glEGLImageTargetTexture2DOES(%d, %p)", GL_TEXTURE_EXTERNAL_OES, SRCBM->lastImageKhr);
				#endif /* CHECK_GL_ERROR */

				/* Set texture properties */
				if (tx->filter == 0) {	/* If filter mode has not been set, initialize it to GL_NEAREST; this may be overridden in the blit later */
					glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
					glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
					tx->filter = GL_NEAREST;
					#if CHECK_GL_ERROR
						err = GetFskGLError();
						PRINT_IF_ERROR(err, __LINE__, "glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_***_FILTER)");
					#endif /* CHECK_GL_ERROR */
				}
				if (tx->wrapMode == 0) {
					glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
					glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
					tx->wrapMode = GL_CLAMP_TO_EDGE;
					#if CHECK_GL_ERROR
						err = GetFskGLError();
						PRINT_IF_ERROR(err, __LINE__, "glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_*)");
					#endif /* CHECK_GL_ERROR */
				}

				return kFskErrNone;
			}
#endif /* ORIGINAL */
#endif /* defined(EGL_VERSION) && defined(BG3CDP_GL) */
			if (gGLGlobalAssets.texImageUYVY && CanDoHardwareVideo(srcBM)) {
				#if defined(LOG_SET_TEXTURE) || defined(LOG_YUV)
					LOGD("SetGLTexture loading %dx%d UYVY native texture #%u from %p", srcBM->bounds.width, srcBM->bounds.height, tx->name, srcBM);
				#endif /* LOG_SET_TEXTURE */
				if (0 == tx->name || GL_TEXTURE_UNLOADED == tx->name)
					err = FskGenGLTexturesAndInit(1, &tx->name);
				if (!err)
					err = gGLGlobalAssets.texImageUYVY(srcBM, tx);				/* System specific UYVY loader */
				gGLGlobalAssets.blitContext->lastTexture = err ? 0 : tx->name;	/* These texture loaders do not set lastTexture */
				return err;
			}
			#if GLES_VERSION == 2
				#if defined(LOG_SET_TEXTURE) || defined(LOG_YUV)
					LOGD("SetGLUYVYTexture loading %dx%d UYVY shader texture #%u from %p", srcBM->bounds.width, srcBM->bounds.height, tx->name, srcBM);
				#endif /* LOG_SET_TEXTURE */
				return SetGLUYVYTexture(srcBM, tx, GL_LINEAR);					/* Shader-based UYVY loader; this sets lastTexture */
			#endif /* GLES_VERSION == 2 */
			break;
		case kFskBitmapFormatGLRGB:
			err = ReshapeOffscreenTexture(GL_RGB, srcBM->bounds.width, srcBM->bounds.height, GL_RGB, GL_UNSIGNED_BYTE, tx);
			#if 1||CHECK_GL_ERROR
				PRINT_IF_ERROR(err, __LINE__, "ReshapeOffscreenTexture(GL_RGB)");
			#endif /* CHECK_GL_ERROR */
			if (!err)
				return err;
		case kFskBitmapFormatGLRGBA:
			err = ReshapeOffscreenTexture(GL_RGBA, srcBM->bounds.width, srcBM->bounds.height, GL_RGBA, GL_UNSIGNED_BYTE, tx);
			#if 1||CHECK_GL_ERROR
				PRINT_IF_ERROR(err, __LINE__, "ReshapeOffscreenTexture(GL_RGBA)");
			#endif /* CHECK_GL_ERROR */
			return err;
		default:
			break;
	}

	if (srcBM->depth < 8)
		return kFskErrUnsupportedPixelType;

	BAIL_IF_ERR(err = FskBitmapReadBegin((FskBitmap)srcBM, (const void**)(const void*)&srcTopLeft, &srcRowBytes, &srcPixelFormat));		// Note: in the format conversion case, we are writing... but we really don't want this operation to change the write seed

	if ((srcRowPix = srcRowBytes) < 0)
		srcRowPix = -srcRowPix;
	srcRowPix /= (srcBM->depth >> 3);

	if (texRect)	FskRectangleIntersect(texRect, &srcBM->bounds, &srcRect);
	else			srcRect = srcBM->bounds;

	srcTopLeft += (srcRect.y - srcBM->bounds.y) *  srcRowBytes
				+ (srcRect.x - srcBM->bounds.x) * (srcBM->depth >> 3);
	if (srcRowBytes < 0)
		srcTopLeft += (srcRect.height - 1) * srcRowBytes;	// TODO: This points to the correct pixels, but they are upside down. Workaround: use positive rowBytes.

	err = FskGLGetFormatAndTypeFromPixelFormat(srcPixelFormat, &srcGLFormat, &srcGLType, &internalGLFormat);
	if (err) {
		#define DONT_CONVERT_32A16RGB565LE_BACK		/**< If 32A16RGB565LE is supplied as a source, convert it to a format supported by GL (GL_RGBA or GL_BGRA), and don't convert it back. */
		switch (srcBM->depth) {
			case 32:
				cnvFormat = ((kFskBitmapFormatDefaultRGBA == kFskBitmapFormat32BGRA) && FskGLHardwareSupportsPixelFormat(kFskBitmapFormat32BGRA)) ? kFskBitmapFormat32BGRA : kFskBitmapFormat32GL;
				#ifdef LOG_SET_TEXTURE
					LOGD("SetGLTexture: Temporarily convert from 32-bit pixel type %s to %s", FskBitmapFormatName(srcPixelFormat), ConversionFormatNameFromCode(cnvFormat));
				#endif /* LOG_SET_TEXTURE */
				#ifdef DONT_CONVERT_32A16RGB565LE_BACK						/* Convert the whole image ... */
					if (srcPixelFormat == kFskBitmapFormat32A16RGB565LE)	/* ... if 32A16RGB565LE */
						err = ConvertImage32Format((UInt32*)srcBM->bits, srcBM->bounds.width, srcBM->bounds.height, srcRowBytes, srcPixelFormat, cnvFormat);
					else													/* Only convert the part that we will use right now */
				#endif /* DONT_CONVERT_32A16RGB565LE_BACK */
				err = ConvertImage32Format((UInt32*)srcTopLeft, srcRect.width, srcRect.height, srcRowBytes, srcPixelFormat, cnvFormat);
				if (kFskErrNone != err)
					cnvFormat = (FskBitmapFormatEnum)(0xFFFFFFFFU);
				PRINT_IF_ERROR(err, __LINE__, "ConvertImage32Format(%d, %d)", srcPixelFormat, cnvFormat);
			break;
			case 24:
				#ifdef LOG_SET_TEXTURE
					LOGD("SetGLTexture: Temporarily convert from 24-bit pixel type %s to %s", FskBitmapFormatName(srcPixelFormat), ConversionFormatNameFromCode(kFskBitmapFormat24GL));
				#endif /* LOG_SET_TEXTURE */
				if (kFskErrNone == (err = ConvertImage24Format((UInt8*)srcTopLeft, srcRect.width, srcRect.height, srcRowBytes, srcPixelFormat, kFskBitmapFormat24GL)))
					cnvFormat = kFskBitmapFormat24GL;	/* Successful temporary in-place conversion to a format that GL likes */
				PRINT_IF_ERROR(err, __LINE__, "ConvertImage24Format(%d, %d)", srcPixelFormat, cnvFormat);
				break;
		}
		BAIL_IF_ERR(err);
		/* In-place conversion was successful, so let's get the GL format and type. */
		err = FskGLGetFormatAndTypeFromPixelFormat(cnvFormat, &srcGLFormat, &srcGLType, &internalGLFormat);
		PRINT_IF_ERROR(err, __LINE__, "FskGLGetFormatAndTypeFromPixelFormat");
		BAIL_IF_ERR(err);
	}

	if (srcGLFormat == GL_LUMINANCE)				/* We never use luminance as a texture, ... */
		srcGLFormat = internalGLFormat = GL_ALPHA;	/* ... so we reinterpret it as alpha. */

	#ifdef LOG_SET_TEXTURE
		if (cnvFormat == (FskBitmapFormatEnum)(0xFFFFFFFFU))
			LOGD("BITMAP(%s) -> GL(%s, %s) -> TEXTURE(%s)", FskBitmapFormatName(srcPixelFormat), GLFormatNameFromCode(srcGLFormat),
				GLTypeNameFromCode(srcGLType), GLInternalFormatNameFromCode(internalGLFormat));
		else
			LOGD("BITMAP(%s) -> CONVERTED_TO(%s) -> GL(%s, %s) -> TEXTURE(%s)",	FskBitmapFormatName(srcPixelFormat), ConversionFormatNameFromCode(cnvFormat),
				GLFormatNameFromCode(srcGLFormat), GLTypeNameFromCode(srcGLType), GLInternalFormatNameFromCode(internalGLFormat));
	#endif /* LOG_SET_TEXTURE */

	#ifdef DONT_CONVERT_32A16RGB565LE_BACK
		if (srcPixelFormat == kFskBitmapFormat32A16RGB565LE) {				/* If 32A16RGB565LE, ... */
			#ifdef LOG_SET_TEXTURE
				LOGD("SetGLTexture changed its mind: the conversion from pixel type %s to %s is permanent", FskBitmapFormatName(srcPixelFormat), ConversionFormatNameFromCode(cnvFormat));
			#endif /* LOG_SET_TEXTURE */
			((FskBitmap)srcBM)->pixelFormat = srcPixelFormat = cnvFormat;	/* ... convert to the blessed R8G8B8A8 format... */
			cnvFormat = (FskBitmapFormatEnum)(0xFFFFFFFFU);					/* ... and never convert back. */
		}
	#endif /* DONT_CONVERT_32A16RGB565LE_BACK */

	CHANGE_TEXTURE(tx->name);
	#if CHECK_GL_ERROR
		err = GetFskGLError();
		PRINT_IF_ERROR(err, __LINE__, "glBindTexture");
		BAIL_IF_ERR(err);
	#endif /* CHECK_GL_ERROR */
	glPixelStorei(GL_UNPACK_ALIGNMENT, ComputeAlignment(srcTopLeft, srcRowBytes));

	err = ShapeAndLoadTexture(internalGLFormat, srcRect.width, srcRect.height, srcRowPix, srcGLFormat, srcGLType, tx, srcTopLeft);

	#ifdef LOG_SET_TEXTURE
		switch (err) {
			case kFskErrNone:				LOGD("SetGLTexture SUCCEEDS");													break;
			case kFskErrTextureTooLarge:	LOGD("SetGLTexture succeeded, but clipped the texture because it was too big");	break;
			default:																										break;	/* Handled below */
		}
	#endif /* LOG_SET_TEXTURE */

bail:
	PRINT_IF_ERROR((err != kFskErrNone && err != kFskErrTextureTooLarge), __LINE__, "SetGLTexture FAILED");
	#ifdef FLUSH_OFTEN
		glFlush();
	#endif /* FLUSH_OFTEN */

	if (cnvFormat != (FskBitmapFormatEnum)(0xFFFFFFFFU)) switch (srcBM->depth) {
		case 32: ConvertImage32Format((UInt32*)srcTopLeft, srcRect.width, srcRect.height, srcRowBytes, cnvFormat, srcPixelFormat);	break;
		case 24: ConvertImage24Format((UInt8 *)srcTopLeft, srcRect.width, srcRect.height, srcRowBytes, cnvFormat, srcPixelFormat);	break;
	}

	#ifdef GL_UNPACK_ROW_LENGTH
		glPixelStorei(GL_UNPACK_ROW_LENGTH,  0);
	#endif /* GL_UNPACK_ROW_LENGTH */
	FskBitmapReadEnd((FskBitmap)srcBM);

	PRINT_IF_ERROR(err, __LINE__, "SetGLTexture");
	return err;
}


/****************************************************************************//**
 * Set the given GL wrapping texture from a bitmap.
 *	\param[in]	srcBM	the source bitmap.
 *	\param[in]	texRect	the bounds of the texture.
 *	\param[in]	tx		the GL texture record.
 *	\return		kFskErrNone	if the texture was set successfully.
 ********************************************************************************/

static FskErr SetGLWrapture(FskConstBitmap srcBM, FskConstRectangle texRect, GLTexture tx)
{
	FskErr				err			= kFskErrNone;
	FskBitmapFormatEnum	cnvFormat	= (FskBitmapFormatEnum)(0xFFFFFFFFU);
	const UInt8			*srcTopLeft;				/* The top left of the rect that we are uploading */
	FskBitmapFormatEnum	srcPixelFormat;				/* The Fsk pixel format */
	SInt32				srcRowPix, srcRowBytes;		/* The number of pixels and byter per row, respectively */
	GLenum				srcGLFormat, srcGLType;		/* The GL external format an type of the texture to be uploaded */
	GLint				internalGLFormat;			/* The internal GL format of the texture */
	FskRectangleRecord	srcRect;					/* The rectangle to be uploaded */

	#if FSK_DEBUG
		if (srcBM == NULL || tx == NULL)
			return kFskErrInvalidParameter;
	#endif /* FSK_DEBUG */

	tx->flipY = false;	/* Indicate that we loaded the texture, as opposed to being loaded from the screen with the opposite vertical polarity */

	BAIL_IF_ERR(err = FskBitmapReadBegin((FskBitmap)srcBM, (const void**)(const void*)&srcTopLeft, &srcRowBytes, &srcPixelFormat));		// Note: in the format conversion case, we are writing... but we really don't want this operation to change the write seed

	if ((srcRowPix = srcRowBytes) < 0)
		srcRowPix = -srcRowPix;
	srcRowPix /= (srcBM->depth >> 3);

	if (texRect)	FskRectangleIntersect(texRect, &srcBM->bounds, &srcRect);
	else			srcRect = srcBM->bounds;

	srcTopLeft += (srcRect.y - srcBM->bounds.y) *  srcRowBytes
				+ (srcRect.x - srcBM->bounds.x) * (srcBM->depth >> 3);
	if (srcRowBytes < 0)
		srcTopLeft += (srcRect.height - 1) * srcRowBytes;	// TODO: This points to the correct pixels, but they are upside down. Workaround: use positive rowBytes.

	err = FskGLGetFormatAndTypeFromPixelFormat(srcPixelFormat, &srcGLFormat, &srcGLType, &internalGLFormat);
	if (err) {
		switch (srcBM->depth) {
			case 32:
				cnvFormat = ((kFskBitmapFormatDefaultRGBA == kFskBitmapFormat32BGRA) && FskGLHardwareSupportsPixelFormat(kFskBitmapFormat32BGRA)) ? kFskBitmapFormat32BGRA : kFskBitmapFormat32GL;
				if (kFskErrNone != (err = ConvertImage32Format((UInt32*)srcBM->bits, srcBM->bounds.width, srcBM->bounds.height, srcRowBytes, srcPixelFormat, cnvFormat))) {
					PRINT_IF_ERROR(err, __LINE__, "ConvertImage32Format(%d --> %d)",  srcPixelFormat, cnvFormat);
					cnvFormat = (FskBitmapFormatEnum)(0xFFFFFFFFU);
				}
				break;
			case 24:
				if (kFskErrNone == (err = ConvertImage24Format((UInt8*)srcTopLeft, srcRect.width, srcRect.height, srcRowBytes, srcPixelFormat, kFskBitmapFormat24GL))) {
					cnvFormat = kFskBitmapFormat24GL;	/* Successful temporary in-place conversion to a format that GL likes */
				} else {
					PRINT_IF_ERROR(err, __LINE__, "ConvertImage32Format(%d --> %d)",  srcPixelFormat, "24RGB");
				}
				break;
		}
		BAIL_IF_ERR(err);
		/* In-place conversion was successful, so let's get the GL format and type. */
		err = FskGLGetFormatAndTypeFromPixelFormat(cnvFormat, &srcGLFormat, &srcGLType, &internalGLFormat);
		PRINT_IF_ERROR(err, __LINE__, "FskGLGetFormatAndTypeFromPixelFormat");
		BAIL_IF_ERR(err);
	}

	#ifdef DONT_CONVERT_32A16RGB565LE_BACK
		if (srcPixelFormat == kFskBitmapFormat32A16RGB565LE) {				/* If 32A16RGB565LE, ... */
			#if GL_DEBUG
				LOGD("SetGLWrapture changed its mind: the conversion from pixel type %u to %u is permanent", (unsigned)srcPixelFormat, (unsigned)cnvFormat);
			#endif /* GL_DEBUG */
			((FskBitmap)srcBM)->pixelFormat = srcPixelFormat = cnvFormat;	/* ... convert to the blessed R8G8B8A8 format... */
			cnvFormat = (FskBitmapFormatEnum)(0xFFFFFFFFU);					/* ... and never convert back. */
		}
	#endif /* DONT_CONVERT_32A16RGB565LE_BACK */

	CHANGE_TEXTURE(tx->name);
	#if CHECK_GL_ERROR
		err = GetFskGLError();
		PRINT_IF_ERROR(err, __LINE__, "glBindTexture");
		BAIL_IF_ERR(err);
	#endif /* CHECK_GL_ERROR */

	/* Pass the texture to GL. The pixels need to be contiguous, so we need to pass all pixels in each row.
	 * We need to set extra edge pixels, so that wraparound interpolation will work correctly.
	 * We assume that the texture is part of a continuous texture, so the adjacent pixels on the right have good values.
	 * We also upload an additional row on the bottom so that it will wraparound interpolate correctly there.
	 */
	glPixelStorei(GL_UNPACK_ALIGNMENT, ComputeAlignment(srcTopLeft, srcRowBytes));
	#ifdef GL_UNPACK_ROW_LENGTH /* We only need to upload the pixels that we want */
		glPixelStorei(GL_UNPACK_ROW_LENGTH,  srcRowPix);
		err = ReshapeRGBTexture(internalGLFormat, srcRect.width + 1, srcRect.height + 1, srcGLFormat, srcGLType, tx);
		BAIL_IF_ERR(err);
		/*              target     level  xoffset        yoffset         width           height          format       type       data       */
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0,             0,              srcRect.width,  srcRect.height, srcGLFormat, srcGLType, srcTopLeft);	/* Upper left rect */
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0,             srcRect.height, srcRect.width,  1,              srcGLFormat, srcGLType, srcTopLeft);	/* Bottom row */
	//	These may not be needed, because the already uploaded pixels may be useful.
		glTexSubImage2D(GL_TEXTURE_2D, 0, srcRect.width, 0,              1,              srcRect.height, srcGLFormat, srcGLType, srcTopLeft);	/* Right column */
		glTexSubImage2D(GL_TEXTURE_2D, 0, srcRect.width, srcRect.height, 1,              1,              srcGLFormat, srcGLType, srcTopLeft);	/* Bottom right corner */
	#else /* !GL_UNPACK_ROW_LENGTH: We need to load all of the rowBytes pixels! */
		err = ReshapeRGBTexture(internalGLFormat, srcRowPix + 1, srcRect.height + 1, srcGLFormat, srcGLType, tx);
		BAIL_IF_ERR(err);
		/*              target     level  xoffset        yoffset         width       height          format       type       data       */
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0,             0,              srcRowPix,  srcRect.height, srcGLFormat, srcGLType, srcTopLeft);	/* Upper left rect */
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0,             srcRect.height, srcRowPix,  1,              srcGLFormat, srcGLType, srcTopLeft);	/* Bottom row */
	//	These may not be needed, because the already uploaded pixels may be useful.
//		glTexSubImage2D(GL_TEXTURE_2D, 0, srcRect.width, 0,              srcRowPix,  srcRect.height, srcGLFormat, srcGLType, srcTopLeft);	/* Right column */
//		glTexSubImage2D(GL_TEXTURE_2D, 0, srcRect.width, srcRect.height, 1,          1,              srcGLFormat, srcGLType, srcTopLeft);	/* Bottom right corner */
	#endif /* GL_UNPACK_ROW_LENGTH */

	#if CHECK_GL_ERROR
		err = GetFskGLError();
		PRINT_IF_ERROR(err, __LINE__, "glTexSubImage2D");
		BAIL_IF_ERR(err);
	#endif /* CHECK_GL_ERROR */

bail:
	if (cnvFormat != (FskBitmapFormatEnum)(0xFFFFFFFFU)) switch (srcBM->depth) {
#ifdef FLUSH_OFTEN
	glFlush();
#endif /* FLUSH_OFTEN */
		case 32: ConvertImage32Format((UInt32*)srcTopLeft, srcRect.width, srcRect.height, srcRowBytes, cnvFormat, srcPixelFormat);	break;
		case 24: ConvertImage24Format((UInt8 *)srcTopLeft, srcRect.width, srcRect.height, srcRowBytes, cnvFormat, srcPixelFormat);	break;
	}

	#ifdef GL_UNPACK_ROW_LENGTH
		glPixelStorei(GL_UNPACK_ROW_LENGTH,  0);
	#endif /* GL_UNPACK_ROW_LENGTH */
	FskBitmapReadEnd((FskBitmap)srcBM);

	PRINT_IF_ERROR(err, __LINE__, "SetGLWrapture");
	return err;
}


/****************************************************************************//**
 * Transform a textured rectangle to a quadrangle.
 *	\param[in]	srcBM	the source bitmap. If NULL, it uses the bitmap of the texture.
 *	\param[in]	srcRect	the source rectangle.
 *	\param[in]	tx		the texture containing the source image.
 *	\return		kFskErrNone	if the operation completed successfully.
 ********************************************************************************/

static FskErr TransformTexturedRectangle(FskConstBitmap srcBM, FskConstRectangle srcRect, GLTexture tx) {
	FskErr				err				= kFskErrNone;
	FskRectangleRecord	texRect;

	if (GL_CLAMP_TO_EDGE != tx->wrapMode) {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);				/* Set texture parameters */
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		tx->wrapMode = GL_CLAMP_TO_EDGE;
	}
	#if GLES_VERSION < 2
		glEnable(GL_TEXTURE_2D);
	#endif /* GLES_VERSION < 2 */

	if (srcBM != NULL) {
		err = SetGLTexture(srcBM, srcRect, tx);
		PRINT_IF_ERROR(err, __LINE__, "SetGLTexture");
		if (err != kFskErrNone && err != kFskErrTextureTooLarge)
			goto bail;
		texRect = *srcRect;
	}
	else {
		FskRectangleSet(&texRect, 0, 0, tx->bounds.width, tx->bounds.height);
	}

	MakeRectTexCoords(srcRect, &texRect, tx);												/* Set texture coordinates */
	#if 0 && FSK_DEBUG
	{	GLTexVertex *vertex = (GLTexVertex*)(gGLGlobalAssets.coordinates);
		LOGD("{");
		for (err = 0; err < 4; ++err)
			LOGD("\t(%.1f, %.1f, %.3f, %.3f)", vertex[0].x, vertex[0].y, vertex[0].u, vertex[0].v);
		LOGD("}");
	}
	#endif /* FSK_DEBUG */

	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);													/* Draw quad */
	#if CHECK_GL_ERROR
		err = GetFskGLError();
		PRINT_IF_ERROR(err, __LINE__, "glDrawArrays");
		BAIL_IF_ERR(err);
	#endif /* CHECK_GL_ERROR */

#ifdef FLUSH_OFTEN
	glFlush();
#endif /* FLUSH_OFTEN */

bail:
	#if GLES_VERSION < 2
		glDisable(GL_TEXTURE_2D);
	#endif /* GLES_VERSION < 2 */
	PRINT_IF_ERROR(err, __LINE__, "TransformTexturedRectangle");
	return err;
}


static Boolean USE_MESH = 1;


/****************************************************************************//**
 * Set the x-component of two triangles that make up a quad, in several rows.
 *	\param[in]		x0			the lesser  x value of the quad.
 *	\param[in]		x1			the greater x value of the quad.
 *	\param[in]		rowStride	the stride between rows of quads.
 *	\param[in]		numRows		the length of the vector.
 *	\param[in,out]	x			the x-vector.
 ********************************************************************************/

static void SetQuadX(float x0, float x1, int rowStride, unsigned numRows, float *x) {
	rowStride -= 5 * TEXVERTEX_STRIDE;
	for (; numRows--; x += rowStride) {
	#ifdef MESH_WITH_TRIANGLES
		/* The triangles are scanned as { (0,0), (0,1), (1,1) }, { (1,0), (0,0), (1,1) }. */
		*x = x0;	x += TEXVERTEX_STRIDE;
		*x = x0;	x += TEXVERTEX_STRIDE;
		*x = x1;	x += TEXVERTEX_STRIDE;
		*x = x1;	x += TEXVERTEX_STRIDE;
		*x = x0;	x += TEXVERTEX_STRIDE;
		*x = x1;
	#else /* MESH_WITH_TRIANGLE_STRIP */
		/* The strip is scanned as { (0,0), (0,0), (1,0), (0,1), (1,1), (1,1) }
		 * Note the degenerate triangle at the beginning and the end to act as punctuation.
		 */
		*x = x0;	x += TEXVERTEX_STRIDE;
		*x = x0;	x += TEXVERTEX_STRIDE;
		*x = x1;	x += TEXVERTEX_STRIDE;
		*x = x0;	x += TEXVERTEX_STRIDE;
		*x = x1;	x += TEXVERTEX_STRIDE;
		*x = x1;
	#endif /* MESH_WITH_TRIANGLE_STRIP */
	}
}


/****************************************************************************//**
 * Set the y-component of two triangles that make up a quad, in a row.
 *	\param[in]		y0			the lesser  y value  of the quad.
 *	\param[in]		y1			the greater y value  of the quad.
 *	\param[in]		stride		the stride between adjacent y coordinates.
 *	\param[in]		numQuads	the length of the vector.
 *	\param[in,out]	x			the x-vector.
 ********************************************************************************/

static void SetQuadY(float y0, float y1, unsigned numQuads, float *y) {
	for (; numQuads--;) {
	#ifdef MESH_WITH_TRIANGLES
		/* The triangles are scanned as { (0,0), (0,1), (1,1) }, { (1,0), (0,0), (1,1) }. */
		*y = y0;	y += TEXVERTEX_STRIDE;
		*y = y1;	y += TEXVERTEX_STRIDE;
		*y = y1;	y += TEXVERTEX_STRIDE;
		*y = y0;	y += TEXVERTEX_STRIDE;
		*y = y0;	y += TEXVERTEX_STRIDE;
		*y = y1;	y += TEXVERTEX_STRIDE;
	#else /* MESH_WITH_TRIANGLE_STRIP */
		/* The strip is scanned as { (0,0), (0,0), (1,0), (0,1), (1,1), (1,1) }
		 * Note the degenerate triangle at the beginning and the end to act as punctuation.
		 */
		*y = y0;	y += TEXVERTEX_STRIDE;
		*y = y0;	y += TEXVERTEX_STRIDE;
		*y = y0;	y += TEXVERTEX_STRIDE;
		*y = y1;	y += TEXVERTEX_STRIDE;
		*y = y1;	y += TEXVERTEX_STRIDE;
		*y = y1;	y += TEXVERTEX_STRIDE;
	#endif /* MESH_WITH_TRIANGLE_STRIP */
	}
}


/****************************************************************************//**
 * Set the uv-components of two triangles that make up a quad, in a vector.
 *	\param[in]		u0			the lesser  u value  of the quad.
 *	\param[in]		u1			the greater u value  of the quad.
 *	\param[in]		ue			the last    u value  of the quad.
 *	\param[in]		v0			the lesser  v value  of the quad.
 *	\param[in]		v1			the greater v value  of the quad.
 *	\param[in]		ve			the last    v value  of the quad.
 *	\param[in]		numQX		the number of quads in each row.
 *	\param[in]		numQY		the number of rows of quads.
 *	\param[in,out]	uv			the uv-vector.
 ********************************************************************************/

static void SetQuadUV(float u0, float u1, float ue, float v0, float v1, float ve, unsigned numQX, unsigned numQY, float *uv) {
	unsigned w;

	#ifdef MESH_WITH_TRIANGLES
		/* The triangles are scanned as { (0,0), (0,1), (1,1) }, { (1,0), (0,0), (1,1) }. */
		for (; --numQY;) {
			for (w = numQX; --w;) {
				uv[0] = u0;	uv[1] = v0;	uv += TEXVERTEX_STRIDE;
				uv[0] = u0;	uv[1] = v1;	uv += TEXVERTEX_STRIDE;
				uv[0] = u1;	uv[1] = v1;	uv += TEXVERTEX_STRIDE;
				uv[0] = u1;	uv[1] = v0;	uv += TEXVERTEX_STRIDE;
				uv[0] = u0;	uv[1] = v0;	uv += TEXVERTEX_STRIDE;
				uv[0] = u1;	uv[1] = v1;	uv += TEXVERTEX_STRIDE;
			}
			uv[0] = u0;	uv[1] = v0;	uv += TEXVERTEX_STRIDE;
			uv[0] = u0;	uv[1] = v1;	uv += TEXVERTEX_STRIDE;
			uv[0] = ue;	uv[1] = v1;	uv += TEXVERTEX_STRIDE;
			uv[0] = ue;	uv[1] = v0;	uv += TEXVERTEX_STRIDE;
			uv[0] = u0;	uv[1] = v0;	uv += TEXVERTEX_STRIDE;
			uv[0] = ue;	uv[1] = v1;	uv += TEXVERTEX_STRIDE;
		}
		for (w = numQX; --w;) {
			uv[0] = u0;	uv[1] = v0;	uv += TEXVERTEX_STRIDE;
			uv[0] = u0;	uv[1] = ve;	uv += TEXVERTEX_STRIDE;
			uv[0] = u1;	uv[1] = ve;	uv += TEXVERTEX_STRIDE;
			uv[0] = u1;	uv[1] = v0;	uv += TEXVERTEX_STRIDE;
			uv[0] = u0;	uv[1] = v0;	uv += TEXVERTEX_STRIDE;
			uv[0] = u1;	uv[1] = ve;	uv += TEXVERTEX_STRIDE;
		}
		uv[0] = u0;	uv[1] = v0;	uv += TEXVERTEX_STRIDE;
		uv[0] = u0;	uv[1] = ve;	uv += TEXVERTEX_STRIDE;
		uv[0] = ue;	uv[1] = ve;	uv += TEXVERTEX_STRIDE;
		uv[0] = ue;	uv[1] = v0;	uv += TEXVERTEX_STRIDE;
		uv[0] = u0;	uv[1] = v0;	uv += TEXVERTEX_STRIDE;
		uv[0] = ue;	uv[1] = ve;	uv += TEXVERTEX_STRIDE;
	#else /* MESH_WITH_TRIANGLE_STRIP */
		/* The strip is scanned as { (0,0), (0,0), (1,0), (0,1), (1,1), (1,1) }
		 * Note the degenerate triangle at the beginning and the end to act as punctuation.
		 */
		for (; --numQY;) {
			for (w = numQX; --w;) {
				uv[0] = u0;	uv[1] = v0;	uv += TEXVERTEX_STRIDE;
				uv[0] = u0;	uv[1] = v0;	uv += TEXVERTEX_STRIDE;
				uv[0] = u1;	uv[1] = v0;	uv += TEXVERTEX_STRIDE;
				uv[0] = u0;	uv[1] = v1;	uv += TEXVERTEX_STRIDE;
				uv[0] = u1;	uv[1] = v1;	uv += TEXVERTEX_STRIDE;
				uv[0] = u1;	uv[1] = v1;	uv += TEXVERTEX_STRIDE;
			}
			uv[0] = u0;	uv[1] = v0;	uv += TEXVERTEX_STRIDE;
			uv[0] = u0;	uv[1] = v0;	uv += TEXVERTEX_STRIDE;
			uv[0] = ue;	uv[1] = v0;	uv += TEXVERTEX_STRIDE;
			uv[0] = u0;	uv[1] = v1;	uv += TEXVERTEX_STRIDE;
			uv[0] = ue;	uv[1] = v1;	uv += TEXVERTEX_STRIDE;
			uv[0] = ue;	uv[1] = v1;	uv += TEXVERTEX_STRIDE;
		}
		for (w = numQX; --w;) {
			uv[0] = u0;	uv[1] = v0;	uv += TEXVERTEX_STRIDE;
			uv[0] = u0;	uv[1] = v0;	uv += TEXVERTEX_STRIDE;
			uv[0] = u1;	uv[1] = v0;	uv += TEXVERTEX_STRIDE;
			uv[0] = u0;	uv[1] = ve;	uv += TEXVERTEX_STRIDE;
			uv[0] = u1;	uv[1] = ve;	uv += TEXVERTEX_STRIDE;
			uv[0] = u1;	uv[1] = ve;	uv += TEXVERTEX_STRIDE;
		}
		uv[0] = u0;	uv[1] = v0;	uv += TEXVERTEX_STRIDE;
		uv[0] = u0;	uv[1] = v0;	uv += TEXVERTEX_STRIDE;
		uv[0] = ue;	uv[1] = v0;	uv += TEXVERTEX_STRIDE;
		uv[0] = u0;	uv[1] = ve;	uv += TEXVERTEX_STRIDE;
		uv[0] = ue;	uv[1] = ve;	uv += TEXVERTEX_STRIDE;
		uv[0] = ue;	uv[1] = ve;	/* uv += TEXVERTEX_STRIDE */;
	#endif /* MESH_WITH_TRIANGLE_STRIP */
}


/****************************************************************************//**
 * Allocate a tile mesh for temporary use.
 *	\param[in]		needBytes		the number of bytes needed.
 *	\param[out]		mesh			the allocated mesh.
 *	\return			kFskErrNone		if there were no errors.
 ********************************************************************************/

static FskErr AllocTempTileMesh(UInt32 needBytes, float **mesh) {
	FskErr err = kFskErrNone;

	*mesh = NULL;
	if ((UInt32)needBytes > gGLGlobalAssets.coordinatesBytes) {
		BAIL_IF_ERR(err = FskMemPtrRealloc(needBytes, &gGLGlobalAssets.coordinates));
		gGLGlobalAssets.coordinatesBytes = needBytes;

		#if GLES_VERSION == 2
			glVertexAttribPointer(VERTEX_POSITION_INDEX, 2, GL_COORD_TYPE_ENUM, GL_FALSE, sizeof(GLTexVertex), gGLGlobalAssets.coordinates + TEXVERTEX_X);
			glVertexAttribPointer(VERTEX_TEXCOORD_INDEX, 2, GL_COORD_TYPE_ENUM, GL_FALSE, sizeof(GLTexVertex), gGLGlobalAssets.coordinates + TEXVERTEX_U);
		#else /* GLES_VERSION == 1 */
			glVertexPointer  (2, GL_COORD_TYPE_ENUM, sizeof(GLTexVertex), gGLGlobalAssets.coordinates + TEXVERTEX_X);
			glTexCoordPointer(2, GL_COORD_TYPE_ENUM, sizeof(GLTexVertex), gGLGlobalAssets.coordinates + TEXVERTEX_U);
		#endif /* GLES_VERSION == 1 */
	}
	*mesh = gGLGlobalAssets.coordinates;

bail:
	PRINT_IF_ERROR(err, __LINE__, "AllocTempTileMesh");
	return err;
}


/****************************************************************************//**
 * Make a tile mesh.
 *	\param[in]	x0			the minimum x value of the mesh.
 *	\param[in]	x1			the maximum x value of the mesh.
 *	\param[in]	dx			the x delta between mesh nodes.
 *	\param[in]	y0			the minimum y value of the mesh.
 *	\param[in]	y1			the maximum y value of the mesh.
 *	\param[in]	dy			the y delta between mesh nodes.
 *	\param[in]	u0			the minimum u value of the mesh, corresponding to x0.
 *	\param[in]	du			the u delta between mesh nodes.
 *	\param[in]	v0			the minimum v value of the mesh, corresponding to v0.
 *	\param[in]	dv			the v delta between mesh nodes.
 *	\param[out]	numPtsPtr	returns the number of points generated.
 *	\param[out]	xyPtr		returns a pointer to the xy vector with the specified stride.
 *	\param[out]	uvPtr		returns a pointer to the uv vector with the specified stride.
 ********************************************************************************/

static void NewTileMesh(
	float x0, float x1, float dx,
	float y0, float y1, float dy,
	float u0, float u1,
	float v0, float v1,
	UInt32 *numPtsPtr, float **xyPtr, float **uvPtr
) {
	FskErr	err;
	int		numQX, numQY, numQ, numT, numPts, rowStride, quadStride, i;
	UInt32	numBytes;
	float	*xy, *uv, xa, xb, ya, yb, ue, ve;

	numQX      = (int)(ceil((x1 - x0) / dx));				/* The number of quads in X */
	numQY      = (int)(ceil((y1 - y0) / dy));				/* The number of quads in Y */
	numQ       = numQX * numQY;								/* The total number of quads */
	numT       = 2 * numQ;									/* The total number of triangles */
	numPts     = 3 * numT;									/* The total number of points */
	numBytes   = numPts * 4 * sizeof(float);				/* Each point has {x, y, u, v} */
	quadStride = 2 * 3 * TEXVERTEX_STRIDE;					/* The stride from one quad to the next */
	rowStride  = numQX * quadStride;						/* The number of floats from one row to the next */
	BAIL_IF_ERR(err = AllocTempTileMesh(numBytes, &xy));
	uv = xy + 2;
	*numPtsPtr = numPts;
	*xyPtr = xy;
	*uvPtr = uv;

	/* Set UV texture coordinates */
	ue = ((x1 - x0) / dx - numQX + 1) * (u1 - u0) + u0;		/* The last u and v ... */
	ve = ((y1 - y0) / dy - numQY + 1) * (v1 - v0) + v0;		/* ... might not be a complete cycle. */
	SetQuadUV(u0, u1, ue, v0, v1, ve, numQX, numQY, uv);

	/* Set X polygon coordinates */
	for (i = numQX, xy = *xyPtr, xa = x0, xb = x0 + dx; --i; xa = xb, xb += dx, xy += quadStride)
		SetQuadX(xa, xb, rowStride, numQY, xy);
	SetQuadX(xa, x1, rowStride, numQY, xy);					/* Rightmost partial tiles */

	/* Set Y polygon coordinates */
	for (i = numQY, xy = *xyPtr + 1, ya = y0, yb = y0 + dy; --i; ya = yb, yb += dy, xy += rowStride)
		SetQuadY(ya, yb, numQX, xy);
	SetQuadY(ya, y1, numQX, xy);	/* Bottom partial tiles */

	return;


bail:
	*numPtsPtr	= 0;
	*xyPtr		= NULL;
	*uvPtr		= NULL;
}


/****************************************************************************//**
 * Tile a texture to fill a rectangle.
 *	\param[in]	scale		the scale factor to be applied to the source, typically { 1.0, 1.5 2.0 }.
 *	\param[in]	srcRect		the source rectangle.
 *	\param[in]	texRect		the bounds of the source texture.
 *	\param[in]	dstRect		the rectangle to be modified.
 *	\param[in]	tx			the texture containing the source image.
 *	\param[in]	glPort		the GL port of the destination.
 *	\return		kFskErrNone	if the operation completed successfully.
 ********************************************************************************/

static FskErr TileTexturedRectangle(FskFixed scale, FskConstRectangle srcRect, FskConstRectangle texRect, FskConstRectangle dstRect, GLTexture tx, FskGLPort glPort) {
	FskErr		err		= kFskErrNone;
	float		fScale	= scale * (1.f / 65536.f);
	GLTexVertex *vertex = (GLTexVertex*)(gGLGlobalAssets.coordinates);

	#if CHECK_GL_ERROR
		err = GetFskGLError();
		PRINT_IF_ERROR(err, __LINE__, "before TileTexturedRectangle");
		BAIL_IF_ERR(err);
	#endif /* CHECK_GL_ERROR */

	#if GLES_VERSION < 2
		glEnable(GL_TEXTURE_2D);
	#endif /* GL 1 */

	#if GLES_VERSION == 2
		if (gGLGlobalAssets.hasHighPrecisionShaderFloat) {	/* High enough precision to use the tile shader */
			float fudge = .5f - .5f/fScale;					/* This converts from pixels as points to pixels as squares */
			float iw = 1.f / tx->bounds.width;				/* Normalizers */
			float ih = 1.f / tx->bounds.height;
			float x0 = (srcRect->x      +   fudge) * iw;	/* Normalized coordinates, moved in by fudge factor ... */
			float y0 = (srcRect->y      +   fudge) * ih;
			float dx = (srcRect->width  - 2*fudge) * iw;	/* ... on either side  */
			float dy = (srcRect->height - 2*fudge) * ih;

			if (GL_CLAMP_TO_EDGE != tx->wrapMode) {
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
				tx->wrapMode = GL_CLAMP_TO_EDGE;
			}

			/* Set tile space */
			glUniform4f(gGLGlobalAssets.tileBitmap.tileSpace, x0, y0, dx, dy);

			/* Set texture tile coordinates */
			vertex[0].u = vertex[1].u = 0;
			vertex[0].v = vertex[3].v = 0;
			vertex[2].u = vertex[3].u = dstRect->width  / (srcRect->width  * fScale);
			vertex[1].v = vertex[2].v = dstRect->height / (srcRect->height * fScale);

			/* Make polygon and draw */
			MakeTexRectVertices(dstRect);
			glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
		} else
	#endif /* GLES_VERSION == 2 */

	if (	srcRect->width	== tx->bounds.width  &&
			srcRect->height	== tx->bounds.height &&
			srcRect->x		== texRect->x        &&
			srcRect->y		== texRect->y
	) {
		if (GL_REPEAT != tx->wrapMode) {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			tx->wrapMode = GL_REPEAT;
		}

		vertex[0].u = vertex[1].u = vertex[0].v = vertex[3].v = 0;	/* Set texture coordinates */
		vertex[2].u = vertex[3].u = dstRect->width  / (tx->bounds.width  * fScale);
		vertex[1].v = vertex[2].v = dstRect->height / (tx->bounds.height * fScale);

		/* Make polygon and draw */
		MakeTexRectVertices(dstRect);
		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	}

	else if (USE_MESH) {
		float fudge = .5f - .5f/fScale;	/* This converts from pixels as points to pixels as squares */
		UInt32 numPts;
		float *xy, *uv;
		if (GL_CLAMP_TO_EDGE != tx->wrapMode) {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			tx->wrapMode = GL_CLAMP_TO_EDGE;
		}
		NewTileMesh((float)(dstRect->x), (float)(dstRect->x + dstRect->width),  srcRect->width  * fScale,
					(float)(dstRect->y), (float)(dstRect->y + dstRect->height), srcRect->height * fScale,
					(srcRect->x - texRect->x + fudge) / tx->bounds.width,  (srcRect->x - texRect->x + srcRect->width  - fudge) / tx->bounds.width,
					(srcRect->y - texRect->y + fudge) / tx->bounds.height, (srcRect->y - texRect->y + srcRect->height - fudge) / tx->bounds.height,
					&numPts, &xy, &uv
		);
		BAIL_IF_ZERO(numPts, err, kFskErrMemFull);
		#if 0
			if (numPts > 100*6)	/* If using more than 100 quads, it is pretty inefficient. */
				LOGI("Using %u quads to mesh a %dx%d tile, when you could be using only 1 quad by using a %ux%u or %ux%u tile.",
					(unsigned)(numPts / 6,)
					(int)srcRect->width, (int)srcRect->height,
					(unsigned)SmallestContainingPowerOfTwo(srcRect->width),   (unsigned)SmallestContainingPowerOfTwo(srcRect->height),
					(unsigned)SmallestContainingPowerOfTwo(srcRect->width)/2, (unsigned)SmallestContainingPowerOfTwo(srcRect->height)/2
				);
		#endif /* 0 */
		#ifdef MESH_WITH_TRIANGLES
			glDrawArrays(GL_TRIANGLES, 0, numPts);
		#else /* MESH_WITH_TRIANGLE_STRIP */
			glDrawArrays(GL_TRIANGLE_STRIP, 0, numPts);
		#endif /* MESH_WITH_TRIANGLE_STRIP */
	}

	else {
		float x0, y0, x1, y1, xMax, yMax, dx, dy;

		/* Make texture coordinates */
		if (GL_CLAMP_TO_EDGE != tx->wrapMode) {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			tx->wrapMode = GL_CLAMP_TO_EDGE;
		}

		x0 = (float)(srcRect->x - texRect->x);
		y0 = (float)(srcRect->y - texRect->y);
		{
			float fudge = .5f - .5f/fScale;	/* This converts from pixels as points to pixels as squares */
			vertex[0].u = vertex[1].u = (x0 + fudge                  ) / tx->bounds.width;		/* Set texture coordinates */
			vertex[0].v = vertex[3].v = (y0 + fudge                  ) / tx->bounds.height;
			vertex[2].u = vertex[3].u = (x0 + srcRect->width  - fudge) / tx->bounds.width;
			vertex[1].v = vertex[2].v = (y0 + srcRect->height - fudge) / tx->bounds.height;
		}

		/* Set vertices and draw polygons */
		#if CHECK_GL_ERROR
			err = GetFskGLError();
			PRINT_IF_ERROR(err, __LINE__, "glTexParameteri");
			BAIL_IF_ERR(err);
		#endif /* CHECK_GL_ERROR */
		dx = srcRect->width  * fScale;
		dy = srcRect->height * fScale;
		xMax = (float)(dstRect->x + dstRect->width);
		yMax = (float)(dstRect->y + dstRect->height);
		for (y1 = (y0 = (float)(dstRect->y)) + dy; y0 < yMax; y0 = y1, y1 += dy) {
			vertex[0].y = vertex[3].y = glPort->portHeight - y0;
			vertex[1].y = vertex[2].y = glPort->portHeight - y1;
			for (x1 = (x0 = (float)(dstRect->x)) + dx; x0 < xMax; x0 = x1, x1 += dx) {
				vertex[0].x = vertex[1].x = x0;
				vertex[2].x = vertex[3].x = x1;
				glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
			}
		}
	}
	#if CHECK_GL_ERROR
		err = GetFskGLError();
		PRINT_IF_ERROR(err, __LINE__, "glDrawArrays");
	#endif /* CHECK_GL_ERROR */

	#ifdef FLUSH_OFTEN
		glFlush();
	#endif /* FLUSH_OFTEN */

bail:
	PRINT_IF_ERROR(err, __LINE__, "TileTexturedRectangle");
	#if GLES_VERSION < 2
		glDisable(GL_TEXTURE_2D);
	#endif /* GL 1 */
	return err;
}


/****************************************************************************//**
 * Tile a bitmap to fill a rectangle.
 *	\param[in]	srcBM	the source bitmap.
 *	\param[in]	scale	the scale factor to be applied to the source, typically { 1.0, 1.5 2.0 }.
 *	\param[in]	srcRect	the region of the source bitmap to be used to fill the destination rectangle.
 *	\param[in]	dstRect	the destination rectangle.
 *	\param[in]	glPort	the GL port of the destination.
 *	\return		kFskErrNone		if the operation completed successfully.
 ********************************************************************************/

static FskErr TileBitmapRect(FskFixed scale, FskConstBitmap srcBM, FskConstRectangle srcRect, FskConstRectangle dstRect, GLTexture tx, FskGLPort glPort) {
	FskErr				err		= kFskErrNone;
	FskRectangleRecord	texRect;													/* Bounds of the uploaded texture */

	if (srcBM) {
		texRect = srcBM->bounds;													/* Upload the entire texture */
		err = SetGLWrapture(srcBM, &texRect, tx);
		BAIL_IF_ERR(err);
	}
	else {
		FskRectangleSet(&texRect, 0, 0, tx->bounds.width, tx->bounds.height);		/* The entire texture */
	}

	err = TileTexturedRectangle(scale, srcRect, &texRect, dstRect, tx, glPort);

bail:
	PRINT_IF_ERROR(err, __LINE__, "TileBitmapRect");
	return err;
}


/****************************************************************************//**
 * Make a polygon that corresponds to the given rectangle dimensions.
 *	\param[in]	width	the width  of the rectangle.
 *	\param[in]	height	the height of the rectangle.
 *	\param[out]	rectPts	an array of 4 points where the resultant polygon points are to be stored.
 ********************************************************************************/

static void MakeRectPolygon(int width, int height) {
	GLTexVertex *vertex = (GLTexVertex*)(gGLGlobalAssets.coordinates);
	GLCoordinateType w, h;
	#ifdef PIXELS_ARE_POINTS
		#if GL_COORD_TYPE_ENUM == GL_FLOAT
			w = (GLCoordinateType)(width  - 1);
			h = (GLCoordinateType)(height - 1);
		#else /* GL_COORD_TYPE_ENUM == GL_FIXED */
			w = (GLCoordinateType)((width  - 1) << 16);
			h = (GLCoordinateType)((height - 1) << 16);
		#endif /* GL_COORD_TYPE_ENUM */
	#else /* PIXELS_ARE_SQUARES */
		#if GL_COORD_TYPE_ENUM == GL_FLOAT
			w = (GLCoordinateType)width;
			h = (GLCoordinateType)height;
		#else /* GL_COORD_TYPE_ENUM == GL_FIXED */
			w = (GLCoordinateType)(width  << 16);
			h = (GLCoordinateType)(height << 16);
		#endif /* GL_COORD_TYPE_ENUM */
	#endif /* PIXELS_ARE_SQUARES */
	vertex[0].x = 0;	vertex[0].y = 0;
	vertex[1].x = 0;	vertex[1].y = h;
	vertex[2].x = w;	vertex[2].y = h;
	vertex[3].x = w;	vertex[3].y = 0;
}


/****************************************************************************//**
 * Perform a linear transformation, i.e. matrix multiplication.
 * All matrices {L, R, P} must be distinct.
 *	\param[in]	L		the left  matrix, of size nRows X lCols.
 *	\param[in]	R		the right matrix, of size lCols X rCols.
 *	\param[out]	P		the resultant product matrix, of size NRows x rCols.
 *	\param[in]	nRows	the number of rows in L and P.
 *	\param[in]	lCols	the number of columns in L and the number of rows in R.
 *	\param[in]	rCols	the number of columns in R and P.
 *	\par		Multiplication of two 3x3 matrices:
 *				SLinearTransform(L, R, P, 3, 3, 3);
 *	\par		Transformation of N 4D row-vectors by a 4x4 matrix:
 *				SLinearTransform(Pin, M, Pout, N, 4, 4);
 *	\par		Transformation of N 3D column-vectors by a 4x3 matrix:
 *				SLinearTransform(M, Pin, Pout, 4, 3, N);
 ********************************************************************************/

static void SLinearTransform(
	const    float	*L,		/* The left  matrix */
	const    float	*R,		/* The right matrix */
	register float	*P,		/* The resultant matrix */
	int				nRows,	/* The number of rows of the left and resultant matrices */
	int				lCols,	/* The number of columns in the left matrix */
	int				rCols	/* The number of columns in the resultant matrix */
) {
	register const float	*lp;									/* Left  matrix pointer for dot product */
	register const char		*rp;									/* Right matrix pointer for dot product */
	register int			k;										/* Loop counter */
	register double			sum;									/* Extended precision for intermediate results */
	register int			j, i;									/* Loop counters */
	const char				*lb;
	register int			rRowBytes	= rCols * sizeof(float);
	register int			lRowBytes	= lCols * sizeof(float);

	for (i = nRows, lb = (const char*)L; i--; lb += lRowBytes) {	/* Each row in L */
		for (j = 0; j < rCols; j++) {								/* Each column in R */
			lp = (const float *)lb;									/* Left of ith row of L */
			rp = (const char *)(R + j);								/* Top of jth column of R */
			sum = 0;
			for (k = lCols; k--; rp += rRowBytes)
				sum += *lp++ * (*((const float*)rp));				/* *P += L[i'][k'] * R[k'][j] */
			*P++ = (float)sum;
		}
	}
}


/****************************************************************************//**
 * Test whether the bitmap is premultiplied.
 *	\param[in]		bm		the bitmap to be tested.
 *	\return			true	if the bitmap has alpha and is premultiplied;
 *	\return			false	otherwise.
 ********************************************************************************/

static Boolean IsBitmapPremultiplied(FskConstBitmap bm) {
	return bm && bm->hasAlpha && bm->alphaIsPremultiplied;
}


/****************************************************************************//**
 * Test whether the child rectangle is fully contained within the mother rectangle.
 *	\param[in]	srcBounds	the bounds of the src (typically &srcBM->bounds).
 *	\param[in]	srcRect		the subRect of the source to display.
 *	\param[in]	dstBM		the destination proxy bitmap (can be NULL).
 *	\param[in]	dstRect		the subRect of the source to display.
 *	\param[out]	sRect		the computed rect of the src.
 *	\param[out]	dRect		the computed rect of the dst.
 *	\return		kFsrErrNone				if successful;
 *	\return		kFskErrNothingRendered	if everything was clipped out.
 ********************************************************************************/

static FskErr ComputeTransformationSrcDstRects(
	FskGLPort			glPort,
	FskConstRectangle	srcBounds,
	FskConstRectangle	srcRect,
	FskConstBitmap		dstBM,
	FskConstRectangle	dstRect,
	FskRectangle		sRect,
	FskRectangle		dRect
) {
	FskErr err = kFskErrNone;

	if    (dstRect)	*dRect = *dstRect;																/* If a dstRect was given, use it */
	else if (dstBM)	*dRect = dstBM->bounds;															/* Otherwise if a dstBM was given, use its bounds */
	else			FskRectangleSet(dRect, 0, 0, glPort->portWidth, glPort->portHeight);			/* Otherwise just use the GL port's dimensions */

	if (!srcRect) {																					/* If no srcRect is given ... */
		*sRect = *srcBounds;																		/* ... use the srcBounds */
	}
	else if (FskRectangleContainsRectangle(srcBounds, srcRect)) {									/* If the srcRect is contained within the srcBounds ... */
		*sRect = *srcRect;																			/* ... use it directly */
	}
	else {																							/* Otherwise srcRect and dstRect need to be clipped */
		float s, t;
		BAIL_IF_FALSE(FskRectangleIntersect(srcRect, srcBounds, sRect), err, kFskErrNothingRendered);
		s = (float)(dRect->width) / (float)(srcRect->width);										/* Horizontal scale */
		t = (sRect->x - srcRect->x) * s + dRect->x;													/* Left edge in floating-point */
		dRect->x     = ROUND_FLOAT_TO_INT(t);														/* TODO: use a float rect */
		dRect->width = ROUND_FLOAT_TO_INT(t + sRect->width * s) - dRect->x;
		s = (float)(dRect->height) / (float)(srcRect->height);										/* Vertical scale */
		t = (sRect->y - srcRect->y) * s + dRect->y;													/* Top edge in floating-point */
		dRect->y     = ROUND_FLOAT_TO_INT(t);
		dRect->height = ROUND_FLOAT_TO_INT(t + sRect->height * s) - dRect->y;
	}

bail:
	return err;
}


/****************************************************************************//**
 * Set the GL draw state for Textured fills.
 *	\param[in]		srcBounds	the bounds of the source texture.
 *	\param[in]		srcRect		the subregion of the source texture to be used in the drawing operation.
 *	\param[in]		dstBM		the destination bitmap proxy.
 *	\param[in]		dstRect		the subregion of the destination that is to be used to determine scaling.
 *	\param[in]		dstClip		the subregion of the destination that is to be modified.
 *	\param[in]		glPort		the glPort of the destination.
 *	\param[in]		opColor		the color to be used if the colorize mode is used.
 *								if NULL, white is used.
 *	\param[in]		mode		the transfer mode to be used, one of {kFskGraphicsModeCopy, kFskGraphicsModeAlpha, kFskGraphicsModeColorize},
 *								possible ORed with kFskGraphicsModeBilinear.
 *	\param[in]		modeParams	the mode parameters.
 *								if NULL, then blendLevel is assumed to be 255.
 *	\param[in]		srcIsPremul	the source colors are premultiplied by their alpha.
 *	\param[out]		sRect		the resultant source rectangle.
 *	\param[out]		dRect		the resultant destination rectangle, which will be changed.
 *	\return			kFskErrNone	if the operation was successful.
 ********************************************************************************/

static FskErr SetTexturedDrawState(
	GLTexture						txSrc,
	FskConstColorRGBA				opColor,
	UInt32							mode,
	FskConstGraphicsModeParameters	modeParams,	/* We get blend level and tint color from here */
	Boolean							srcIsPremul
) {
	FskErr				err				= kFskErrNone;
	UInt8				blendLevel		= (UInt8)((modeParams && ((UInt32)(modeParams->blendLevel) < 255)) ? (UInt8)modeParams->blendLevel : 255U);
	GLint				iParam;

	#if GLES_VERSION < 2
		FskColorRGBARecord	defaultOpColor	= { 255, 255, 255, 255 };
		if (opColor == NULL)
			opColor = &defaultOpColor;
	#else /* GLES_VERSION == 2 */
		if (opColor) {}	/* Avoid unused parameter messages */
	#endif /* GLES_VERSION < 2 */

	#if CHECK_GL_ERROR
		err = GetFskGLError();
		PRINT_IF_ERROR(err, __LINE__, "initial SetTexturedDrawState");
	#endif /* CHECK_GL_ERROR */


	/* Quality */
	iParam = (gGLGlobalAssets.allowNearestBitmap && !(mode & kFskGraphicsModeBilinear)) ? GL_NEAREST : GL_LINEAR;
	if (txSrc->filter != iParam) {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, iParam);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, iParam);
		txSrc->filter = iParam;
	}

	/* Transfer mode */
	if (blendLevel == 255) {
		switch (mode & kFskGraphicsModeMask) {
			case kFskGraphicsModeCopy:
				CHANGE_BLEND_ENABLE(false);
				#if GLES_VERSION < 2
					glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
				#endif /* GLES_VERSION < 2 */
				break;
			case kFskGraphicsModeAlpha:
				#if GLES_VERSION < 2
					glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
					if (!srcIsPremul)	CHANGE_BLEND_FUNC_SEPARATE(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
					else				CHANGE_BLEND_FUNC(                                               GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
				#else /* GLES_VERSION == 2 */
					if (!srcIsPremul)	CHANGE_BLEND_FUNC_SEPARATE(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
					else				CHANGE_BLEND_FUNC(                                               GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
				#endif /* GLES_VERSION < 2 */
				break;
			case kFskGraphicsModeColorize:
				CHANGE_BLEND_ENABLE(false);
				#if GLES_VERSION < 2
					glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
					glColor4ub(opColor->r, opColor->g, opColor->b, opColor->a);
				#endif /* GLES_VERSION < 2 */
				break;
		}
	}
	else {
		#if GLES_VERSION < 2
			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
			if (!srcIsPremul)	CHANGE_BLEND_FUNC_SEPARATE(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);	// TODO: appropriate for alpha, but copy & colorize?
			else				CHANGE_BLEND_FUNC(                                               GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
			switch (mode & kFskGraphicsModeMask) {
				case kFskGraphicsModeCopy:
					glColor4ub(255U, 255U, 255U, blendLevel);
					break;
				case kFskGraphicsModeColorize:
					glColor4ub(opColor->r, opColor->g, opColor->b, FskAlphaMul(opColor->a, blendLevel));
					break;
				case kFskGraphicsModeAlpha:
					glColor4ub(255U, 255U, 255U, blendLevel);
					break;
			}
		#else /* GLES_VERSION == 2 */
			if (!srcIsPremul)	CHANGE_BLEND_FUNC_SEPARATE(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
			else				CHANGE_BLEND_FUNC(                                               GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
		#endif /* GLES_VERSION == 2 */
	}
	#if CHECK_GL_ERROR
		err = GetFskGLError();
	#endif /* CHECK_GL_ERROR */

	PRINT_IF_ERROR(err, __LINE__, "FAILED SetTexturedDrawState");
	return err;
}


/****************************************************************************//**
 * Allocate a texture object that will be disposed shortly.
 *	\param[out]	texID	place to store the texture object.
 *	\return		kFskErrNone	if the texture object was allocated successfully.
 ********************************************************************************/

static FskErr AllocateEphemeralTextureObject(GLuint *texID) {
	#if EPHEMERAL_TEXTURE_CACHE_SIZE > 0
		*texID = gGLGlobalAssets.textureObjectCache[gGLGlobalAssets.nextTextureObjectIndex];
		if (++(gGLGlobalAssets.nextTextureObjectIndex) >= EPHEMERAL_TEXTURE_CACHE_SIZE)
			gGLGlobalAssets.nextTextureObjectIndex = 0;
		#ifdef LOG_TEXTURE_LIFE
			LOGD("Using texture #%u from the ephemeral cache", *texID);
		#endif /* LOG_TEXTURE_LIFE */
		return kFskErrNone;
	#else /* EPHEMERAL_TEXTURE_CACHE_SIZE <= 0 */
		FskErr err =  FskGenGLTexturesAndInit(1, texID);
		#ifdef LOG_TEXTURE_LIFE
			LOGD("Generated ephemeral texture #%u", *texID);
		#endif /* LOG_TEXTURE_LIFE */
		return err;
	#endif /* EPHEMERAL_TEXTURE_CACHE_SIZE <= 0 */
}


/****************************************************************************//**
 * Free an ephemeral texture object.
 *	\param[in]	texID	the ID of the texture to be freed.
 ********************************************************************************/

static void FreeEphemeralTextureObject(GLuint texID) {
	if (texID) {
		ForgetGivenTextureObjectInAllContexts(texID);
		#if EPHEMERAL_TEXTURE_CACHE_SIZE <= 0
			#if defined(LOG_EPHEMERAL) || defined(LOG_TEXTURE_LIFE)
				LOGE("WARNING: PERFORMANCE REDUCED while flushing before freeing ephemeral texture #%u", texID);
			#endif /* defined(LOG_EPHEMERAL) || defined(LOG_TEXTURE_LIFE) */
			glFlush();																	/* This is slow, but we need to wait until all the rendering is done ... */
			#ifdef TRACK_TEXTURES
				UntrackTexture(texID);
			#endif /* TRACK_TEXTURES */
			glDeleteTextures(1, &texID);												/* ... before disposing of an ephemeral texture. */
		#elif defined(LOG_TEXTURE_LIFE) /* && EPHEMERAL_TEXTURE_CACHE_SIZE > 0 */
			LOGD("Returning texture #%u to the ephemeral pool", texID);
		#else /* !LOG_TEXTURE_LIFE && EPHEMERAL_TEXTURE_CACHE_SIZE > 0 */
		#endif /* EPHEMERAL_TEXTURE_CACHE_SIZE */
	}
}


/****************************************************************************//**
 * Free all ephemeral texture objects from the texture.
 *	\param[in]	tx	the texture descriptor, which may contain up to 3 textures.
 *	\return		kFskErrNone	if the port was retrieved successfully.
 ********************************************************************************/

static void FreeEphemeralTextureObjects(GLTexture tx) {
	if (tx->name)	{
		ForgetGivenTexturesInAllContexts(tx);
		#if EPHEMERAL_TEXTURE_CACHE_SIZE <= 0
			#if defined(LOG_EPHEMERAL) || defined(LOG_TEXTURE_LIFE)
				LOGE("WARNING: PERFORMANCE REDUCED while flushing before freeing ephemeral textures {#%u, #%u, #%u}", tx->name, tx->nameU, tx->nameV);
			#endif /* defined(LOG_EPHEMERAL) || defined(LOG_TEXTURE_LIFE) */
			glFlush();																	/* This is slow, but we need to wait until all the rendering is done ... */
			#ifdef TRACK_TEXTURES
				UntrackTexture(tx->name);
				if (tx->nameU)	UntrackTexture(tx->nameU);
				if (tx->nameV)	UntrackTexture(tx->nameV);
			#endif /* TRACK_TEXTURES */
			glDeleteTextures(1, &tx->name);												/* ... before disposing of the ephemeral textures. */
			if (tx->nameU)
				glDeleteTextures(1, &tx->nameU);
			if (tx->nameV)
				glDeleteTextures(1, &tx->nameV);
		#else /* EPHEMERAL_TEXTURE_CACHE_SIZE > 0 */
			#ifdef LOG_TEXTURE_LIFE
				LOGD("Returning textures {#%u, #%u, #%u} of size %ux%u to the ephemeral pool", tx->name, tx->nameU, tx->nameV, tx->bounds.width, tx->bounds.height);
			#endif /* LOG_TEXTURE_LIFE */
		#endif /* EPHEMERAL_TEXTURE_CACHE_SIZE > 0 */
	}
}


/********************************************************************************
 * GetBitmapTexture
 ********************************************************************************/

static GLTextureRecord* GetBitmapTexture(FskConstBitmap bm) {
	FskGLPort port;

	if (NULL != (port = bm->glPort) && 0 != port->texture.name) {	/* If the bitmap is accelerated */
		if (GL_TEXTURE_UNLOADED == port->texture.name) {			/* but currently unloaded */
			FskErr err;
			port->texture.name = 0;
			err = FskGLAccelerateBitmapSource((FskBitmap)bm);		/* load the texture into the GPU */
			if (err) {
				LOGE("GetBitmapTexture: bitmap %p FAILED TO LOAD, err = %d", bm, (int)err);
				return NULL;
			}
			#ifdef LOG_TEXTURE_LIFE
				LOGD("GetBitmapTexture: bitmap %p accelerated to #%u, err = %d", bm, port->texture.name, (int)err);
			#endif /* LOG_TEXTURE_LIFE */
		}
		return &port->texture;
	}

	return NULL;
}


/****************************************************************************//**
 * Set up a blit with an RGB or RGBA source.
 * This is independent of the actual shader used.
 *	\param[in]		srcBM			the source.
 *	\param[in]		srcRect			the source rectangle; NULL inplies srcBM->bounds.
 *	\param[in]		opColor			the operation color.
 *	\param[in]		mode			the compositing mode.
 *	\param[in]		modeParams		the mode parameters.
 *	\param[in]		pTexRec			a texture record, supplied by the caller, to be used for JIT texture, if necessary.
 *	\param[out]		pTex			upon return, this is set to either the texture part of yuvBM, or pTexRec if JIT textures were needed.
 *	\param[out]		texRect			the coordinate frame within which coordinates are specified.
 *	\return			kFskErrNone		if the operation was successful.
 ********************************************************************************/

static FskErr SetupRGBBlit(
	FskConstBitmap					srcBM,
	FskConstRectangle				srcRect,
	FskConstColorRGBA				opColor,
	UInt32							mode,
	FskConstGraphicsModeParameters	modeParams,
	GLTexture						pTexRec,
	GLTexture						*pTex,
	FskRectangle					texRect
) {
	FskErr				err				= kFskErrNone;
	Boolean				srcIsPremul		= IsBitmapPremultiplied(srcBM);
	SInt32				blendLevel		= (modeParams && ((UInt32)modeParams->blendLevel <= 255U)) ? modeParams->blendLevel : 255;
	GLTexture			tx;
	GLint				iParam;

	#if GLES_VERSION < 2
		FskColorRGBARecord	defaultOpColor	= { 255, 255, 255, 255 };
		if (opColor == NULL)
			opColor = &defaultOpColor;
		glEnable(GL_TEXTURE_2D);
	#endif /* GLES_VERSION < 2 */

	if (NULL != (tx = GetBitmapTexture(srcBM))) {
		CHANGE_TEXTURE(tx->name);
		if (texRect)	FskRectangleSet(texRect, 0, 0, tx->bounds.width, tx->bounds.height);
	}
	else {
		tx = pTexRec;
		FskMemSet(tx, 0, sizeof(*tx));														/* This forces a texture resize */
		AllocateEphemeralTextureObject(&tx->name);
		#ifdef LOG_TEXTURE_LIFE
			LOGD("Allocated ephemeral texture RGB #%u", tx->name);
		#endif /* LOG_TEXTURE_LIFE */
		SET_TEXTURE(tx->name);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		tx->filter = GL_LINEAR;
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);				/* Set texture parameters */
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		tx->wrapMode = GL_CLAMP_TO_EDGE;
		err = SetGLTexture(srcBM, srcRect, tx);												/* This calls glBindTexture() */
		if (texRect)	*texRect = *srcRect;
		PRINT_IF_ERROR(err, __LINE__, "SetGLTexture");
		if (err != kFskErrNone && err != kFskErrTextureTooLarge)
			goto bail;
	}
	*pTex = tx;

	if (GL_CLAMP_TO_EDGE != tx->wrapMode) {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);				/* Set texture parameters */
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		tx->wrapMode = GL_CLAMP_TO_EDGE;
	}

	/* Quality */
	iParam = (gGLGlobalAssets.allowNearestBitmap && !(mode & kFskGraphicsModeBilinear)) ? GL_NEAREST : GL_LINEAR;
	if (tx->filter != iParam) {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, iParam);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, iParam);
		tx->filter = iParam;
	}

	/* Transfer mode */
	mode &= kFskGraphicsModeMask;															/* Toss away quality bit, since we just set the quality */

	#if GLES_VERSION < 2

		if (blendLevel == 255) {															/* No blending */
			switch (mode) {
				case kFskGraphicsModeCopy:
					CHANGE_BLEND_ENABLE(false);
					glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
					break;
				case kFskGraphicsModeAlpha:
					glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
					if (!srcIsPremul)	CHANGE_BLEND_FUNC_SEPARATE(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
					else				CHANGE_BLEND_FUNC(                                               GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
					break;
				case kFskGraphicsModeColorize:
					CHANGE_BLEND_ENABLE(false);
					glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
					if (!srcIsPremul)	glColor4ub(opColor->r, opColor->g, opColor->b, opColor->a);
					else				glColor4ub(FskAlphaMul(opColor->a, opColor->r), FskAlphaMul(opColor->a, opColor->g), FskAlphaMul(opColor->a, opColor->b), opColor->a);
					break;
			}
		}
		else {
			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
			if (!srcIsPremul)	CHANGE_BLEND_FUNC_SEPARATE(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);	// TODO: appropriate for alpha, but copy & colorize?
			else				CHANGE_BLEND_FUNC(                                               GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
			switch (mode) {
				case kFskGraphicsModeCopy:
					if (!srcIsPremul)	glColor4ub(255U,       255U,       255U,       blendLevel);
					else				glColor4ub(blendLevel, blendLevel, blendLevel, blendLevel);
					break;
				case kFskGraphicsModeColorize:
					blendLevel = FskAlphaMul(opColor->a, blendLevel);
					if (!srcIsPremul)	glColor4ub(opColor->r, opColor->g, opColor->b, blendLevel);
					else				glColor4ub(FskAlphaMul(blendLevel, opColor->r), FskAlphaMul(blendLevel, opColor->g), FskAlphaMul(blendLevel, opColor->b), blendLevel);
					break;
				case kFskGraphicsModeAlpha:
					if (!srcIsPremul)	glColor4ub(255U, 255U, 255U, blendLevel);
					else				glColor4ub(blendLevel, blendLevel, blendLevel, blendLevel);
					break;
			}
		}

	#else /* GLES_VERSION == 2 */

		switch (mode) {
			case kFskGraphicsModeCopy:	default:
				if (blendLevel == 255) {
					CHANGE_BLEND_ENABLE(false);
					break;
				}
				glBlendColor(0.f, 0.f, 0.f, blendLevel * (1.f / 255.f));
				CHANGE_BLEND_FUNC(GL_CONSTANT_ALPHA, GL_ONE_MINUS_CONSTANT_ALPHA);
				break;
			case kFskGraphicsModeColorize:
				if ((255 == blendLevel) && !srcBM->hasAlpha && (255 == opColor->a)) {
					CHANGE_BLEND_ENABLE(false);
					break;
				}
				/* else fall through */
			case kFskGraphicsModeAlpha:
				if (!srcIsPremul)	CHANGE_BLEND_FUNC_SEPARATE(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
				else				CHANGE_BLEND_FUNC(                                               GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
				break;
		}

	#endif /* GLES_VERSION == 2 */

	#if CHECK_GL_ERROR
		err = GetFskGLError();
	#endif /* CHECK_GL_ERROR */

bail:
	PRINT_IF_ERROR(err, __LINE__, "FAILED SetupRGBBlit");
	return err;
}


#if GLES_VERSION == 2


/***************************************************************************//**
 * Make a YUV --> RGB transform matrix.
 *	\param[in]	ccir				Which CCIR standard to use, either 601 or 709.
 *	\param[in]	colorCorrection		Matrix to provide additional color correction after conversion to RGB (can be NULL).
 *	\param[in]	brightness			Offset (typically 0).
 *	\param[in]	contrast			Gain   (typically 1).
 *	\param[out]	colorMtx			Resultant color matrix to be fed into FskGLYUV420BitmapDraw().
 *	\return		kFskErrNone				if the operation was successful.
 *	\return		kFskErrInvalidParameter	if ccir is other than 601 or 709.
 *******************************************************************************/

static FskErr YUVColorMatrixMake(int ccir, const float *colorCorrection/*[4][4]*/, float brightness, float contrast, float *colorMtx/*[4][4]*/) {
	float M[3][4][4];	/* M[0] removes YUV bias, M[1] converts YUV-->RGB, M[2] applies brightness and contrast */
	float P[4][4];

	FskMemSet(&M, 0, sizeof(M));													/* Zero 3 matrices at once */

	/* Set M[1] to convert YUV-->RGB */
	switch (ccir) {
		case 601:
			M[1][2][0] =  1.596027344f;												/* Cr      coefficient  for red   */
			M[1][1][1] = -0.391761719f;	M[1][2][1] = -0.81296875f;					/* Cr & Cb coefficients for green */
			M[1][1][2] =  2.017230469f;												/* Cb      coefficient  for blue  */
			break;
		case 709:
			M[1][2][0] =  1.792742188f;												/* Cr      coefficient  for red   */
			M[1][1][1] = -0.21325f;		M[1][2][1] = -0.532910156f;					/* Cr & Cb coefficients for green */
			M[1][1][2] =  2.112402344f;												/* Cb      coefficient  for blue  */
			break;
		default:
			return kFskErrInvalidParameter;
	}
	M[1][0][0] = M[1][0][1] = M[1][0][2] = 1.164382813f;							/* Luminance scale */
	M[1][3][3] = 1.f;																/* Homogeneous parameter */

	/* Set M[0] to remove bias */
	M[0][0][0] = M[0][1][1] = M[0][2][2] = M[0][3][3] = 1.f;						/* No scaling */
	M[0][3][0] =               -16.f / 255.f;										/* Luminance bias   */
	M[0][3][1] = M[0][3][2] = -128.f / 255.f;										/* Chrominance bias */

	/* Set M[2] to apply brightness and contrast */
	M[2][0][0] = M[2][1][1] = M[2][2][2] = contrast;								/* Isotropic gain */
	M[2][3][0] = M[2][3][1] = M[2][3][2] = brightness + (1.f - contrast) * 0.5f;	/* Isotropic bias */
	M[2][3][3] = 1.f;																/* Homogeneous parameter */


	SLinearTransform(M[0][0], M[1][0], P[0], 4, 4, 4);								/* Remove YUV bias, convert from YUV to RGB */
	if (colorCorrection == NULL) {
		SLinearTransform(P[0], M[2][0], colorMtx, 4, 4, 4);							/* Apply brightness and contrast */
	}
	else {
		SLinearTransform(P[0],    colorCorrection, M[0][0],  4, 4, 4);				/* Apply color correction */
		SLinearTransform(M[0][0], M[2][0],         colorMtx, 4, 4, 4);				/* Apply brightness and contrast */
	}

	return kFskErrNone;
}


/***************************************************************************//**
 * SwapUVColorMatrix
 *	\params[in,out]	colorMtx	The resultant color matrix.
 *******************************************************************************/

static void SwapUVColorMatrix(float *colorMtx/*[4][4]*/) {
	float t;
	t = colorMtx[4*1+0];	colorMtx[4*1+0] = colorMtx[4*2+0];	colorMtx[4*2+0] = t;
	t = colorMtx[4*1+1];	colorMtx[4*1+1] = colorMtx[4*2+1];	colorMtx[4*2+1] = t;
	t = colorMtx[4*1+2];	colorMtx[4*1+2] = colorMtx[4*2+2];	colorMtx[4*2+2] = t;
	t = colorMtx[4*1+3];	colorMtx[4*1+3] = colorMtx[4*2+3];	colorMtx[4*2+3] = t;
}


/***************************************************************************//**
 * MakeYUVColorBlendShaderMatrix
 *	\param[in]		opColor		An operation color, used if the mode is Colorize. NULL implies a white opColor.
 *	\param[in]		mode		The transfer mode, one of {kFskGraphicsModeCopy, kFskGraphicsModeAlpha, kFskGraphicsModeColorize} optionally ORed with kFskGraphicsModeBilinear.
 *	\param[in]		modeParams	{ blendLevel, brightness and contrast}. NULL, implies { 255, 0, 255 }.
 *	\params[out]	colorMtx	The resultant color matrix.
 *	\return			kFskErrNone	if the operation completed successfully.
 *******************************************************************************/

static FskErr MakeYUVColorBlendShaderMatrix(FskConstColorRGBA opColor, UInt32 mode, FskConstGraphicsModeParameters modeParams, float *colorMtx/*[4][4]*/) {
	FskErr	err		= kFskErrNone;
	UInt32	mode0	= mode & kFskGraphicsModeMask;
	float	colCor[4][4], brightness, contrast;

	FskMemSet(&colCor[0][0], 0, sizeof(colCor));										/* Zero the color correction matrix */
	if ((mode0 == kFskGraphicsModeColorize) && opColor) {
		colCor[0][0] = UCHAR_TO_FLOAT_COLOR(opColor->r);								/* Set the diagonals for colorizing */
		colCor[1][1] = UCHAR_TO_FLOAT_COLOR(opColor->g);
		colCor[2][2] = UCHAR_TO_FLOAT_COLOR(opColor->b);
		colCor[3][3] = UCHAR_TO_FLOAT_COLOR(opColor->a);
	}
	else {
		colCor[0][0] = 1.f;																/* Set the diagonals to identity */
		colCor[1][1] = 1.f;
		colCor[2][2] = 1.f;
		colCor[3][3] = 1.f;
	}
	brightness = 0.f;																	/* Default brightness and contrast */
	contrast   = 1.f;
	if (modeParams) {
		if (modeParams->dataSize == sizeof(FskGraphicsModeParametersVideoRecord)) {		/* Special video parameters */
			FskGraphicsModeParametersVideo videoParams = (FskGraphicsModeParametersVideo)modeParams;
			if (videoParams->kind == 'cbcb') {
				brightness = videoParams->brightness * (1.f/65536.f);					/* Custom brightness and contrast */
				contrast   = videoParams->contrast   * (1.f/65536.f) + 1.f;
			}
		}
		if ((UInt32)(modeParams->blendLevel) < 255) {
			colCor[3][3] *= UCHAR_TO_FLOAT_COLOR(modeParams->blendLevel);				/* Modulate alpha by the blend level */
			CHANGE_BLEND_FUNC_SEPARATE(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
		}
		else {
			CHANGE_BLEND_ENABLE(false);													/* ... disable blending because we know that YUV has no alpha */
		}
	}
	else {
		if (colCor[3][3] == 1.f)														/* If color and blend level are both opaque, ... */
			CHANGE_BLEND_ENABLE(false);													/* ... disable blending because we know that YUV has no alpha */
		else																			/* Otherwise, ... */
			CHANGE_BLEND_FUNC_SEPARATE(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);	/* ... make sure to blend */
	}
	err = YUVColorMatrixMake(601, colCor[0], brightness, contrast, colorMtx);			/* Color matrix converts from YUV to RGB, with colorizing, brightness and contrast */
	return err;
}


/****************************************************************************//**
 * Set up a blit with a YUV 4:2:0 planar source.
 *	\param[in]		yuvBM			the YUV 4:2:0 planar source.
 *	\param[in]		opColor			a color that is used when the mode specifies colorize.
 *	\param[in]		mode			the compositing mode.
 *	\param[in]		modeParams		the mode parameters.
 *	\param[in]		pTexRec			a texture record, supplied by the caller, to be used for JIT texture, if necessary.
 *	\param[out]		pTex			upon return, this is set to either the texture part of yuvBM, or pTexRec if JIT textures were needed.
 *	\return			kFskErrNone		if the operation was successful.
 ********************************************************************************/

static FskErr SetupYUV420Blit(
	FskConstBitmap					yuvBM,
	FskConstColorRGBA				opColor,
	UInt32							mode,
	FskConstGraphicsModeParameters	modeParams,
	GLTexture						pTexRec,
	GLTexture						*pTex
) {
	FskErr				err			= kFskErrNone;
	GLint				glQuality	= (gGLGlobalAssets.allowNearestBitmap && !(mode & kFskGraphicsModeBilinear)) ? GL_NEAREST : GL_LINEAR;
	GLTexture			tx;
	GLCoordinateType	cMtx[4*4];

	#ifdef LOG_YUV
		LOGD("SetupYUV420Blit");
	#endif /* LOG_YUV */

	BAIL_IF_ERR(err = FskGLUseProgram(gGLGlobalAssets.yuv420.program));							/* Set program */

	if (yuvBM->glPort && (tx = &yuvBM->glPort->texture)->name && tx->nameU && tx->nameV) {		/* Cached texture */
		/* Nothing more to do here */
	}
	else {
		tx = pTexRec;
		AllocateEphemeralTextureObject(&tx->name);										/* Luminance texture */
		AllocateEphemeralTextureObject(&tx->nameU);										/* Chrominance (Cb, U) texture */
		AllocateEphemeralTextureObject(&tx->nameV);										/* Chrominance (Cr, V) texture */
		#ifdef LOG_TEXTURE_LIFE
			LOGD("Allocated ephemeral textures YUV420 {#%u, #%u, #%u}", tx->name, tx->nameU, tx->nameV);
		#endif /* LOG_TEXTURE_LIFE */
		tx->bounds.width	= 0;														/* This forces a texture resize */
		tx->bounds.height	= 0;														/* This forces a texture resize */
		tx->glIntFormat		= 0;														/* This forces a texture resize */
		tx->srcBM			= NULL;
		tx->filter			= 0;														/* Assume that 0 != GL_NEAREST && 0 != GL_LINEAR */
		tx->wrapMode		= 0;														/* This will force clamping to be set */
		BAIL_IF_ERR(err = SetGLYUV420Textures(yuvBM, tx, glQuality));					/* Upload the textures */
	}
	*pTex = tx;

	glActiveTexture(GL_TEXTURE2);
	SET_TEXTURE(tx->nameV);																/* Set texture to be used for V */
	if (tx->filter != glQuality) {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, glQuality);				/* Set quality for V */
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, glQuality);
	}

	glActiveTexture(GL_TEXTURE1);
	SET_TEXTURE(tx->nameU);																/* Set texture to be used to U */
	if (tx->filter != glQuality) {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, glQuality);				/* Set quality for U */
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, glQuality);
	}

	glActiveTexture(GL_TEXTURE0);														/* We always leave GL_TEXTURE0 as the active texture */
	SET_TEXTURE(tx->name);																/* Set texture to be used for Y */
	if (tx->filter != glQuality) {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, glQuality);				/* Set quality for Y */
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, glQuality);
		tx->filter = glQuality;
	}

	CHANGE_SHADER_MATRIX(yuv420);

	err = MakeYUVColorBlendShaderMatrix(opColor, mode, modeParams, cMtx);				/* This also enables or disables blending */
	glUniformMatrix4fv(gGLGlobalAssets.yuv420.colorMtx, 1, GL_FALSE, cMtx);				/* Set color matrix */


	#if CHECK_GL_ERROR
		err = GetFskGLError();
		PRINT_IF_ERROR(err, __LINE__, "setting FskGLYUV420BitmapDrawState ");
		BAIL_IF_ERR(err);
	#endif /* CHECK_GL_ERROR */

bail:
	return err;
}


/****************************************************************************//**
 * Set up a blit with a YUV 4:2:0 semi-planar source.
 *	\param[in]		yuvBM			the YUV 4:2:0 semi-planar source.
 *	\param[in]		opColor			a color that is used when the mode specifies colorize.
 *	\param[in]		mode			the compositing mode.
 *	\param[in]		modeParams		the mode parameters.
 *	\param[in]		pTexRec			a texture record, supplied by the caller, to be used for JIT texture, if necessary.
 *	\param[out]		pTex			upon return, this is set to either the texture part of yuvBM, or pTexRec if JIT textures were needed.
 *	\return			kFskErrNone		if the operation was successful.
 ********************************************************************************/

static FskErr SetupYUV420spBlit(
	FskConstBitmap					yuvBM,
	FskConstColorRGBA				opColor,
	UInt32							mode,
	FskConstGraphicsModeParameters	modeParams,
	GLTexture						pTexRec,
	GLTexture						*pTex
) {
	FskErr				err			= kFskErrNone;
	GLint				glQuality	= (gGLGlobalAssets.allowNearestBitmap && !(mode & kFskGraphicsModeBilinear)) ? GL_NEAREST : GL_LINEAR;
	GLTexture			tx;
	GLCoordinateType	cMtx[4*4];

	#ifdef LOG_YUV
		LOGD("SetupYUV420spBlit");
	#endif /* LOG_YUV */

	BAIL_IF_ERR(err = FskGLUseProgram(gGLGlobalAssets.yuv420sp.program));				/* Set program */

	if (yuvBM->glPort && (tx = &yuvBM->glPort->texture)->name && tx->nameU) {			/* Cached texture */
		/* Nothing more to do here */
	}
	else {
		tx = pTexRec;
		AllocateEphemeralTextureObject(&tx->name);										/* Luminance texture */
		AllocateEphemeralTextureObject(&tx->nameU);										/* Chrominance (Cb, Cr) texture */
		#ifdef LOG_TEXTURE_LIFE
			LOGD("Allocated ephemeral textures YUV420sp {#%u, #%u}", tx->name, tx->nameU);
		#endif /* LOG_TEXTURE_LIFE */
		tx->bounds.width	= 0;														/* This forces a texture resize */
		tx->bounds.height	= 0;														/* This forces a texture resize */
		tx->glIntFormat		= 0;														/* This forces a texture resize */
		tx->srcBM			= NULL;
		tx->filter			= 0;														/* Assume that 0 != GL_NEAREST && 0 != GL_LINEAR */
		tx->wrapMode		= 0;														/* This will force clamping to be set */
		BAIL_IF_ERR(err = SetGLYUV420spTextures(yuvBM, tx, glQuality));					/* Upload the textures */
	}
	*pTex = tx;

	glActiveTexture(GL_TEXTURE1);
	SET_TEXTURE(tx->nameU);																/* Set texture to be used to U */
	if (tx->filter != glQuality) {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, glQuality);				/* Set quality for U */
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, glQuality);
	}

	glActiveTexture(GL_TEXTURE0);														/* We always leave GL_TEXTURE0 as the active texture */
	SET_TEXTURE(tx->name);																/* Set texture to be used for Y */
	if (tx->filter != glQuality) {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, glQuality);				/* Set quality for Y */
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, glQuality);
		tx->filter = glQuality;
	}

	CHANGE_SHADER_MATRIX(yuv420sp);

	err = MakeYUVColorBlendShaderMatrix(opColor, mode, modeParams, cMtx);				/* This also enables or disables blending */
	if (yuvBM->pixelFormat == kFskBitmapFormatYUV420spvu)
		SwapUVColorMatrix(cMtx);
	glUniformMatrix4fv(gGLGlobalAssets.yuv420sp.colorMtx, 1, GL_FALSE, cMtx);			/* Set color matrix */


	#if CHECK_GL_ERROR
		err = GetFskGLError();
		PRINT_IF_ERROR(err, __LINE__, "setting FskGLYUV420spBitmapDrawState ");
		BAIL_IF_ERR(err);
	#endif /* CHECK_GL_ERROR */

bail:
	return err;
}


/****************************************************************************//**
 * Set up a blit with a YUV 4:2:2 chunky source.
 *	\param[in]		srcBM			the YUV 4:2:2 source.
 *	\param[in]		srcUIndex		the index of the U (Cb) component, e.g. 0 for UYVY, 1 for YUYV, 2 for VYUY, 3 for YVYU.
 *	\param[in]		opColor			a color that is used when the mode specifies colorize.
 *	\param[in]		mode			the compositing mode.
 *	\param[in]		modeParams		the mode parameters.
 *	\param[in]		pTexRec			a texture record, supplied by the caller, to be used for JIT texture, if necessary.
 *	\param[out]		pTex			upon return, this is set to either the texture part of yuvBM, or pTexRec if JIT textures were needed.
 *	\return			kFskErrNone		if the operation was successful.
 ********************************************************************************/

static FskErr SetupYUV422Blit(
	FskConstBitmap					srcBM,
	UInt32							srcUIndex,			/* Only { UYVY or VYUY } if !GL_RGB_422_APPLE, or { UYVY or YVYU } if GL_RGB_422_APPLE */
	FskConstColorRGBA				opColor,
	UInt32							mode,
	FskConstGraphicsModeParameters	modeParams,
	GLTexture						pTexRec,
	GLTexture						*pTex
) {
	FskErr				err		= kFskErrNone;
	GLTexture			tx;
	GLCoordinateType	cMtx[4*4];
	GLint				glQuality;

	err = MakeYUVColorBlendShaderMatrix(opColor, mode, modeParams, cMtx);							/* This also enables or disables blending */
	if (srcUIndex) {}	/* Avoid unused parameter warnings */										/* We only accommodate UYVY unless we look at srcIndex */
	/* Choose the program to use, depending on availability */
	#ifdef GL_RGB_422_APPLE
		if (gGLGlobalAssets.hasAppleRGB422) {
			#ifdef LOG_YUV
				LOGD("SetupYUV422Blit(GL_RGB_422_APPLE)");
			#endif /* LOG_YUV */
			BAIL_IF_ERR(err = FskGLUseProgram(gGLGlobalAssets.yuv444.program));						/* Set program */
			CHANGE_SHADER_MATRIX(yuv444);															/* Set matrix */
			glUniform1i(gGLGlobalAssets.yuv444.srcBM, 0);											/* Set src texture */
			glUniformMatrix4fv(gGLGlobalAssets.yuv444.colorMtx, 1, GL_FALSE, cMtx);					/* Set color matrix */
			glQuality = (gGLGlobalAssets.allowNearestBitmap && !(mode & kFskGraphicsModeBilinear)) ? GL_NEAREST : GL_LINEAR;
		}
		else
	#endif /* GL_RGB_422_APPLE */
	{
		#ifdef LOG_YUV
			LOGD("SetupYUV422Blit");
		#endif	/* LOG_YUV */
		BAIL_IF_ERR(err = FskGLUseProgram(gGLGlobalAssets.yuv422.program));							/* Set program */
		CHANGE_SHADER_MATRIX(yuv422);																/* Set matrix */
		glUniform1i(gGLGlobalAssets.yuv422.srcBM, 0);												/* Set src texture */
		glUniformMatrix4fv(gGLGlobalAssets.yuv422.colorMtx, 1, GL_FALSE, cMtx);						/* Set color matrix */
		glQuality = GL_NEAREST;																		/* The 422 shader requires GL_NEAREST, regardless of whether it is allowed */
	}

	/* Set the texture */
	if (srcBM->glPort && (tx = &srcBM->glPort->texture)->name) {									/* Cached texture */
		CHANGE_TEXTURE(tx->name);
		if (tx->filter != glQuality) {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, glQuality);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, glQuality);
			tx->filter = glQuality;
		}
	}
	else {
		tx = pTexRec;																				/* The caller provides the storage for an ephemeral texture */
		AllocateEphemeralTextureObject(&tx->name);
		#ifdef LOG_TEXTURE_LIFE
			LOGD("Allocated ephemeral texture YUV422 #%u", tx->name);
		#endif /* LOG_TEXTURE_LIFE */
		tx->bounds.width	= 0;																	/* This forces a texture resize */
		tx->bounds.height	= 0;																	/* This forces a texture resize */
		tx->glIntFormat		= 0;																	/* This forces a texture resize */
		tx->srcBM			= NULL;
		tx->filter			= 0;																	/* Assume that 0 != GL_NEAREST && 0 != GL_LINEAR */
		tx->wrapMode		= 0;																	/* This will force clamping to be set */
		BAIL_IF_ERR(err = SetGLUYVYTexture(srcBM, tx, glQuality));
	}
	*pTex = tx;

	if (GL_CLAMP_TO_EDGE != tx->wrapMode) {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		tx->wrapMode = GL_CLAMP_TO_EDGE;
	}

	#ifdef GL_RGB_422_APPLE
		if (!gGLGlobalAssets.hasAppleRGB422)
	#endif /* GL_RGB_422_APPLE */
		glUniform4f(gGLGlobalAssets.yuv422.texDim, (GLfloat)(tx->bounds.width),
					(GLfloat)(tx->bounds.height), 1.f/tx->bounds.width, 1.f/tx->bounds.height);		/* Set texture dimensions */

	#if CHECK_GL_ERROR
		err = GetFskGLError();
		PRINT_IF_ERROR(err, __LINE__, "setting FskGLYUV422BitmapDrawState");
		BAIL_IF_ERR(err);
	#endif /* CHECK_GL_ERROR */

bail:
	return err;
}

#if defined(BG3CDP_GL)

/****************************************************************************//**
 * Set up a blit with a BG3CDP source.
 *	\param[in]		srcBM			the source.
 *	\param[in]		opColor			a color that is used when the mode specifies colorize.
 *	\param[in]		mode			the compositing mode.
 *	\param[in]		modeParams		the mode parameters.
 *	\param[in]		pTexRec			a texture record, supplied by the caller, to be used for JIT texture, if necessary.
 *	\param[out]		pTex			upon return, this is set to either the texture part of yuvBM, or pTexRec if JIT textures were needed.
 *	\return			kFskErrNone		if the operation was successful.
 ********************************************************************************/

static FskErr SetupBG3CDPBlit(
	FskConstBitmap					srcBM,
	FskConstColorRGBA				opColor,
	UInt32							mode,
	FskConstGraphicsModeParameters	modeParams,
	GLTexture						pTexRec,
	GLTexture						*pTex
) {
	FskErr		err		= kFskErrNone;
	GLTexture	tx;
	GLint		glQuality;
	float		fOpColor[4], fOffColor[4];
	UInt32		lastOpColor, lastLevel;

	#ifdef LOG_YUV
		LOGD("SetupBG3CDPBlit()");
	#endif /* LOG_YUV */

	if (!srcBM->physicalAddr)
		BAIL(kFskErrUnimplemented);

	/* Abort early if the shader did not compile */
	BAIL_IF_ERR(err = FskGLUseProgram(gGLGlobalAssets.drawBG3CDPBitmap.program));					/* Set program */

	/* The starting opColor is opaque white, unless the mode is Colorize and a color was supplied */
	if (((mode & kFskGraphicsModeMask) == kFskGraphicsModeColorize) && opColor) {
		lastOpColor = ENCODE_LAST_OP_COLOR(opColor->r, opColor->g, opColor->b, opColor->a);
		fOpColor[0] = UCHAR_TO_FLOAT_COLOR(opColor->r);
		fOpColor[1] = UCHAR_TO_FLOAT_COLOR(opColor->g);
		fOpColor[2] = UCHAR_TO_FLOAT_COLOR(opColor->b);
		fOpColor[3] = UCHAR_TO_FLOAT_COLOR(opColor->a);
	}
	else {
		lastOpColor = ENCODE_LAST_OP_COLOR(255, 255, 255, 255);
		fOpColor[0] = 1.f;
		fOpColor[1] = 1.f;
		fOpColor[2] = 1.f;
		fOpColor[3] = 1.f;
	}

	/* If brightness and contrast was supplied, we need to use a special brightness/contrast program.
	 * We also modulate the opColor by the contrast, and compute an offset color.
	 * Otherwise, we use the basic bitmap program.
	 */
	if (modeParams && (modeParams->dataSize >= sizeof(FskGraphicsModeParametersVideoRecord)) && (((FskGraphicsModeParametersVideo)modeParams)->kind =='cbcb')) {
		float brightness = ((FskGraphicsModeParametersVideo)modeParams)->brightness * (1.f/65536.f);
		float contrast   = ((FskGraphicsModeParametersVideo)modeParams)->contrast   * (1.f/65536.f) + 1.f;
		fOffColor[0] =
		fOffColor[1] =
		fOffColor[2] = (1.f - contrast) * .5f + brightness;
		fOffColor[3] = 0.f;
		fOpColor[0] *= contrast;
		fOpColor[1] *= contrast;
		fOpColor[2] *= contrast;
	}
	else {
		fOffColor[0] = 0.f;
		fOffColor[1] = 0.f;
		fOffColor[2] = 0.f;
		fOffColor[3] = 0.f;
	}

	/* If blending, modulate alpha by the blend factor. If also premultiplied, the color is modulated as well. */
	if (modeParams && ((UInt32)(modeParams->blendLevel) < 255)) {									/* Won't pass if blendLevel < 0 || blendLevel >= 255 */
		float blendLevel = (kFskGraphicsModeCopy == (mode & kFskGraphicsModeMask)) ? 1.f : UCHAR_TO_FLOAT_COLOR(modeParams->blendLevel);
		lastLevel = modeParams->blendLevel;
		fOpColor[3] *= blendLevel;
		if (IsBitmapPremultiplied(srcBM)) {
			fOpColor[0] *= blendLevel;
			fOpColor[1] *= blendLevel;
			fOpColor[2] *= blendLevel;
			lastLevel |= PREMULTIPLY_FLAG;
		}
	}
	else {
		lastLevel = FULL_LEVEL_FLAG;
	}

	CHANGE_SHADER_MATRIX(drawBG3CDPBitmap);
	glUniform1i(gGLGlobalAssets.drawBG3CDPBitmap.srcBM, 0);											/* Set src texture */
	#ifdef LOG_COLOR
		LogFColor(fOpColor,  "ShaderGainColor");
		LogFColor(fOffColor, "ShaderBrightnessColor");
	#endif /* LOG_COLOR */
	if (1																	||
		!OPTIMIZE_STATE_CHANGES												||
		(gGLGlobalAssets.blitContext->drawBG3CDPBitmap.lastParams.color != lastOpColor)	||
		(gGLGlobalAssets.blitContext->drawBG3CDPBitmap.lastParams.level != lastLevel)
	) {
		glUniform4fv(gGLGlobalAssets.drawBG3CDPBitmap.opColor,  1,  fOpColor);						/* Set   op   color -- always set this -- ignore state */
		glUniform4fv(gGLGlobalAssets.drawBG3CDPBitmap.offColor, 1, fOffColor);						/* Set offset color -- always set this -- ignore state */
		gGLGlobalAssets.blitContext->drawBG3CDPBitmap.lastParams.color = lastOpColor;
		gGLGlobalAssets.blitContext->drawBG3CDPBitmap.lastParams.level = lastLevel;
		#ifdef LOG_COLOR
			LOGD("Shader Brightness and Contrast Colors have been changed");
		#endif /* LOG_COLOR */
	}

	glQuality = (gGLGlobalAssets.allowNearestBitmap && !(mode & kFskGraphicsModeBilinear)) ? GL_NEAREST : GL_LINEAR;

	/* Set the texture */
	if (srcBM->glPort && (tx = &srcBM->glPort->texture)->name) {									/* Cached texture */
		CHANGE_TEXTURE(tx->name);
		if (tx->filter != glQuality) {
			glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MIN_FILTER, glQuality);
			glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MAG_FILTER, glQuality);
			tx->filter = glQuality;
		}
	}
	else {
		tx = pTexRec;																				/* The caller provides the storage for an ephemeral texture */
		AllocateEphemeralTextureObject(&tx->name);
		#ifdef LOG_TEXTURE_LIFE
			LOGD("Allocated ephemeral texture BG3CDP #%u", tx->name);
		#endif /* LOG_TEXTURE_LIFE */
		tx->bounds.width	= 0;																	/* This forces a texture resize */
		tx->bounds.height	= 0;																	/* This forces a texture resize */
		tx->glIntFormat		= 0;																	/* This forces a texture resize */
		tx->srcBM			= NULL;
		tx->filter			= 0;																	/* Assume that 0 != GL_NEAREST && 0 != GL_LINEAR */
		tx->wrapMode		= 0;																	/* This will force clamping to be set */
		BAIL_IF_ERR(err = SetGLTexture(srcBM, &srcBM->bounds, tx));

	}
	*pTex = tx;

	if (GL_CLAMP_TO_EDGE != tx->wrapMode) {
		glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		tx->wrapMode = GL_CLAMP_TO_EDGE;
	}

	#if CHECK_GL_ERROR
		err = GetFskGLError();
		PRINT_IF_ERROR(err, __LINE__, "SetupBG3CDPBlit");
		BAIL_IF_ERR(err);
	#endif /* CHECK_GL_ERROR */

bail:
	return err;
}
#endif /* BG3CDP_GL */



#endif /* GLES_VERSION == 2 */


/****************************************************************************//**
 * Setup Blit.
 *	\param[in]		srcBM		the source bitmap to be drawn.
 *	\param[in]		srcRect		an optional subrectangle of the source to be copied (may be NULL).
 *	\param[in]		glPort		the GL port.
 *	\param[in]		dstClip		an optional rectangle that to restrict the changes (may be NULL).
 *	\param[in]		opColor		a color that is used when the mode specifies colorize.
 *	\param[in]		mode		the mode.  The kFskGraphicsModeBilinear flag may be ORed with either of
 *								{kFskGraphicsModeCopy, kFskGraphicsModeAlpha, or kFskGraphicsModeColorize}.
 *	\param[in]		modeParams	additional parameters, sometimes needed for some of the modes.
 *	\param[in,out]	jitTex		a pointer to a just-in-time texture. If unused, jitTex->name == 0.
 *								If jitTex->name !+ 0, appropriate disposal needs to be made to jitTex->name, jitTex->nameU, and jitTex->nameV.
 *	\param[out]		shaderState	location to hold a pointer to the shader state structure. Can be NULL.
 *	\return			kFskErrNone	if the operation was successful.
 ********************************************************************************/

static FskErr SetupBlit(
	FskConstBitmap					srcBM,
	FskConstRectangle				srcRect,
	FskGLPort						glPort,
	FskConstRectangle				dstClip,
	FskConstColorRGBA				opColor,
	UInt32							mode,
	FskConstGraphicsModeParameters	modeParams,
	GLTexture						jitTex,
	ShaderState						**shaderState
) {
	FskErr				err		= kFskErrNone;
	GLTextureRecord		*tx;
	FskRectangleRecord	texRect;

	if (!srcRect)
		srcRect = &srcBM->bounds;
	if (!srcBM->hasAlpha && (kFskGraphicsModeAlpha == (mode & kFskGraphicsModeMask)))
		mode = kFskGraphicsModeCopy | (mode & ~kFskGraphicsModeMask);
	if (shaderState)
		*shaderState = NULL;
	FskMemSet(jitTex, 0, sizeof(*jitTex));

	#if GLES_VERSION < 2

		BAIL_IF_ERR(err = SetupRGBBlit(srcBM, srcRect, opColor, mode, modeParams, jitTex, &tx, &texRect));
		MakeRectTexCoords(srcRect, &texRect, tx);																			/* Set texture coordinates */

	#else /* GLES_VERSION == 2 */

		#ifdef LOG_YUV
			tx = (srcBM && srcBM->glPort && srcBM->glPort->texture.name) ? &srcBM->glPort->texture : NULL;
		#endif /* LOG_YUV */
		switch (srcBM->pixelFormat) {
			case kFskBitmapFormatYUV420:
				#ifdef LOG_YUV
					if (tx)	LOGD("Blitting YUV420 from %p's texture {#%u,#%u,#%u} (%d/%d) using %s", srcBM, tx->name, tx->nameU, tx->nameV, srcBM->accelerateSeed, srcBM->writeSeed,
								((gGLGlobalAssets.texImageYUV420 && CanDoHardwareVideo(srcBM)) ? "hardware" : "shader")
							);
					else	LOGD("Blitting YUV420 from %p's bitmap using %s", srcBM,
								((gGLGlobalAssets.texImageYUV420 && CanDoHardwareVideo(srcBM)) ? "hardware" : "shader")
							);
				#endif /* LOG_YUV */
				if (gGLGlobalAssets.texImageYUV420 && CanDoHardwareVideo(srcBM)) goto defaultShader;						/* Hardware YUV420 upload is available */
				BAIL_IF_ERR(err = SetupYUV420Blit(srcBM, opColor, mode, modeParams, jitTex, &tx));							/* Set program, color matrix, view matrix */
				MakeRectTexCoords(srcRect, &srcBM->bounds, tx);																/* Set texture coordinates */
				if (shaderState) *shaderState = &gGLGlobalAssets.blitContext->yuv420;
				break;
			case kFskBitmapFormatYUV420spuv:
			case kFskBitmapFormatYUV420spvu:
				#ifdef LOG_YUV
					if (tx)	LOGD("Blitting YUV420sp from %p's texture {#%u,#%u} (%d/%d) using shader", srcBM, tx->name, tx->nameU, srcBM->accelerateSeed, srcBM->writeSeed,
								((gGLGlobalAssets.texImageYUV420 && CanDoHardwareVideo(srcBM)) ? "hardware" : "shader")
							);
					else	LOGD("Blitting YUV420sp from %p's bitmap using shader", srcBM,
								((gGLGlobalAssets.texImageYUV420 && CanDoHardwareVideo(srcBM)) ? "hardware" : "shader")
							);
				#endif /* LOG_YUV */
				BAIL_IF_ERR(err = SetupYUV420spBlit(srcBM, opColor, mode, modeParams, jitTex, &tx));						/* Set program, color matrix, view matrix */
				MakeRectTexCoords(srcRect, &srcBM->bounds, tx);																/* Set texture coordinates */
				if (shaderState) *shaderState = &gGLGlobalAssets.blitContext->yuv420sp;
				break;
			case kFskBitmapFormatUYVY:
				#ifdef LOG_YUV
				{	const char *how = (	gGLGlobalAssets.texImageUYVY && CanDoHardwareVideo(srcBM))	?	"hardware"		:
						#ifdef GL_RGB_422_APPLE
										gGLGlobalAssets.hasAppleRGB422								?	"AppleShader"	:
						#endif /* GL_RGB_422_APPLE */
						"shader";
					if (tx)	LOGD("Blitting UYVY from %p's texture #%u (%d/%d) using %s",	srcBM, tx->name, srcBM->accelerateSeed, srcBM->writeSeed,	how);
					else	LOGD("Blitting UYVY from %p's bitmap "           "using %s",	srcBM,														how);
				}
				#endif /* LOG_YUV */
				#if defined(EGL_VERSION) && defined(BG3CDP_GL)
				#ifdef EXTRA_CRISPY
					if (FskGLHardwareSupportsYUVPixelFormat(kFskBitmapFormatUYVY) && (kFskErrNone == (err = SetupBG3CDPBlit(srcBM, opColor, mode, modeParams, jitTex, &tx)))) {
						if (shaderState) *shaderState = &gGLGlobalAssets.blitContext->drawBG3CDPBitmap;
						MakeRectTexCoords(srcRect, &srcBM->bounds, tx);
						break;
					}
				#else /* ORIGINAL */
					if (gGLGlobalAssets.hasImageExternal && srcBM->physicalAddr) {
						BAIL_IF_ERR(err = SetupBG3CDPBlit(srcBM, opColor, mode, modeParams, jitTex, &tx));
						if (shaderState) *shaderState = &gGLGlobalAssets.blitContext->drawBG3CDPBitmap;
						MakeRectTexCoords(srcRect, &srcBM->bounds, tx);
						break;
					}
				#endif /* ORIGINAL */
				#endif /* defined(EGL_VERSION) && defined(BG3CDP_GL) */
				if (gGLGlobalAssets.texImageUYVY && CanDoHardwareVideo(srcBM)) goto defaultShader;							/* Hardware UYVY upload is available */
				BAIL_IF_ERR(err = SetupYUV422Blit(srcBM, /*srcUIndex=*/0, opColor, mode, modeParams, jitTex, &tx));			/* Set program, color matrix, view matrix */
				#ifdef GL_RGB_422_APPLE
					if (!gGLGlobalAssets.hasAppleRGB422)
				#endif /* GL_RGB_422_APPLE */
				{	if (shaderState) *shaderState = &gGLGlobalAssets.blitContext->yuv422;
					MakeRectTexYUV422Coords(srcRect, &srcBM->bounds, tx);
				}
				#ifdef GL_RGB_422_APPLE
					else {
						if (shaderState) *shaderState = &gGLGlobalAssets.blitContext->yuv444;
						MakeRectTexCoords(srcRect, &srcBM->bounds, tx);
					}
				#endif /* GL_RGB_422_APPLE */

				break;
			defaultShader:
			default:
				BAIL_IF_ERR(err = SetupRGBBlit(srcBM, srcRect, opColor, mode, modeParams, jitTex, &tx, &texRect));
				{	ShaderState *sid = SetBitmapProgramOpColor(opColor, IsBitmapPremultiplied(srcBM), mode, modeParams);	/* Sets program, matrix, opColor */
					if (shaderState)	*shaderState = sid;
					BAIL_IF_NULL(sid, err, kFskErrGLProgram);
				}
				MakeRectTexCoords(srcRect, &texRect, tx);														/* Set texture coordinates */
				break;
		}

	#endif /* GLES_VERSION == 2 */

	/* Set clip rect */
	if (dstClip)	CHANGE_SCISSOR(glPort, dstClip->x, dstClip->y, dstClip->width, dstClip->height);			/* This also enables scissoring */
	else			CHANGE_SCISSOR_ENABLE(false);

bail:
	return err;
}


/****************************************************************************//**
 * Get the destination GL port.
 * This should only be called from the rendering thread.
 * It can come from the bitmap, or the global current GL port.
 *	\param[in]	bm	the destination bitmap proxy.
 *	\param[out]	glPort	the GL port of the destination.
 *	\return		kFskErrNone	if the port was retrieved successfully.
 ********************************************************************************/

FskErr FskGLDstPort(FskBitmap bm, FskGLPort *glPortPtr) {
	FskErr		err		= kFskErrNone;
	FskGLPort	glPort	= NULL;

	#if GL_DEBUG
		BAIL_IF_NULL(bm, err, kFskErrInvalidParameter);			/* We don't normally test this, because this is an internal call */
	#endif /* GL_DEBUG */

	#if TARGET_OS_ANDROID || (TARGET_OS_KPL && BG3CDP)
		if (!GL_GLOBAL_ASSETS_ARE_INITIALIZED()) {	/* Deferred initialization */
			err = InitGlobalGLAssets();
			#if GL_DEBUG
				if (err)	LOGD("Deferred GL Port initialization in FskGLDstPort fails with code %d", (int)err);
				else		LOGD("Deferred GL Port initialization in FskGLDstPort succeeds");
			#endif /* GL_DEBUG */
		}
	#endif /* TARGET_OS_ANDROID */

#define AUTO_SET_FRAMEBUFFER_OBJECT		/**< Look at the port, and if it has a renderable texture, attach it automatically to the frame buffer object. */
#ifndef AUTO_SET_FRAMEBUFFER_OBJECT
	glPort = (bm && bm->glPort) ? bm->glPort : FskGLPortGetCurrent();
#else /* AUTO_SET_FRAMEBUFFER_OBJECT */
	if (bm == NULL || bm->glPort == NULL) {
		glPort = FskGLPortGetCurrent();														/* Render to current port (screen) */
	}
	else {
		glPort = bm->glPort;
		if (glPort->texture.name != gGLGlobalAssets.blitContext->fboTexture) {				/* If we are changing frame buffers */
			if (glPort->texture.name)	err = FskGLRenderToBitmapTexture(bm, NULL);			/* Render to texture */
			else						err = FskGLRenderToBitmapTexture(NULL, NULL);		/* Render to screen */
			BAIL_IF_ERR(err);
		}
	}
#endif /* AUTO_SET_FRAMEBUFFER_OBJECT */
	BAIL_IF_FALSE(glPort->portWidth > 0 && glPort->portHeight > 0, err, kFskErrNotAccelerated);

bail:
	PRINT_IF_ERROR(err, __LINE__, "FskGLDstPort");
	if (glPortPtr)	*glPortPtr = err ? NULL : glPort;
	return err;
}


/****************************************************************************//**
 * Convert a unicode character to a UTF-8 string.
 * The current largest Unicode point assigned is 0x10FFFF; this can code up to 0x1FFFFF.
 *	\param[in]		unicode			the unicode character.
 *	\param[out]		encodedText		the equivalent UTF-8 string.
 *	\param[out]		numBytes		the number of bytes in the string.
 ********************************************************************************/

static void UnicodeCharToUTF8(UInt16 unicode, char *utf8, UInt32 *pNumBytes) {
	UInt32 numBytes;
	if (0 == (unicode & ~0x007F)) {							/* 0x00 - 0x7F: 7 bit character */
		*utf8++ = (char)unicode;
		numBytes = 1;
	}
	else if (0 == (unicode & ~0x07FF)) {					/* 0x080 - 0x7FF: 11 bit character  */
		*utf8++ = (char)(0xC0 | (unicode >> 6));			/* 3 bit length code + 5 bits data  */
		*utf8++ = (char)(0x80 | (unicode & 0x3F));			/* 2 bit continuation code + 6 bits */
		numBytes = 2;
	}
	else /*if (0 == (unicode & ~0xFFFF))*/ {				/* 0x0800 - 0xFFFF: 16 bit character */
		*utf8++ = (char)(0xE0 | ( unicode >> 12));			/* 4 bit length code + 4 bits data   */
		*utf8++ = (char)(0x80 | ((unicode >>  6) & 0x3F));	/* 2 bit continuation code + 6 bits  */
		*utf8++ = (char)(0x80 | ( unicode        & 0x3F));	/* 2 bit continuation code + 6 bits  */
		numBytes = 3;
	}
	*utf8 = 0;
	if (pNumBytes)
		*pNumBytes = numBytes;
}


#if !USE_GLYPH
/****************************************************************************//**
 * Convert a UTF-8 sequence to a single unicode character.
 * The current largest Unicode point assigned is 0x10FFFF; this can code up to 0x1FFFFF.
 *	\param[in]		unicode			the unicode character.
 *	\param[out]		encodedText		the equivalent UTF-8 string.
 *	\param[out]		numBytes		the number of bytes in the string.
 ********************************************************************************/

static UInt16 UTF8ToUnicodeChar(const char **utf8, UInt32 *bytesUsed)
{
	const char	*text	= *utf8;
	UInt16		uc		= *text++;

	if (0x80 & uc) {							/* Non-ASCII */
		int i, size;
		for (size = 0, i = 6; i >= 0; --i)
			if ((uc & (1 << i)) == 0)			/* Look for the most significant zero */
				break;
		size = 6 - i;							/* Compute the number of additional bytes */
		uc &= ((1 << i) -1);					/* Clear off the size bits */
		while (size--)
			uc = (uc << 6) | (*text++ & 0x3F);	/* Append more bits */
	}
	if (bytesUsed)								/* If the bytesUsed pointer is non-NULL... */
		*bytesUsed = text - *utf8;				/* ... return the number of bytes used. */
	*utf8 = text;								/* Update the utf8 pointer */
	return uc;									/* Return the unicode character */
}
#endif /* !USE_GLYPH */

#if defined(_MSC_VER)		// Our Win32 text engine does not support the FskTextGetLayout function
	#define USE_LAYOUT 0	/**< Do not use the layout function to typeset the characters in a string; just use the character widths. */
#else /* !_MSC_VER */
	#define USE_LAYOUT 1	/**< Use the layout function to typeset the characters in a string, yielding more aesthetic placement. */
#endif /* !_MSC_VER */

#if !USE_LAYOUT
/****************************************************************************//**
 * Compute the number of Unicode characters in a UTF-8 string.
 *	\param[in]	utf8			the UTF-8 string.
 *	\param[in]	byteLength		the number of bytes in the UTF-8 string.
 *	\return		the number of unicode characters in the string.
 ********************************************************************************/

static UInt32 NumberOfUnicodeCharacters(const char *utf8, UInt32 byteLength)
{
	UInt32		n;
	const char	*end;
	for (n = 0, end	= utf8 + byteLength; utf8 != end; ++utf8)
		if ((*utf8 & 0xC0) != 0x80)
			++n;
	return n;
}
#endif /* USE_LAYOUT */


#if !USE_GLYPH
/****************************************************************************//**
 * Get the bounds for the strikes of a range of Unicode characters.
 *	\param[in]	typeFace			the typeface being queried.
 *	\param[in]	firstCodePoint		the first code point of the range.
 *	\param[in]	lastCodePoint		the last  code point of the range.
 *	\param[out]	bounds				the bounds. Only width and height are significant, because x and y are set to 0.
 ********************************************************************************/

static void FskTextGetUnicodeRangeStrikeBounds(FskConstGLTypeFace typeFace, UInt16 firstCodePoint, UInt16 lastCodePoint, FskRectangle bounds) {
	char	encodedText[8];
	UInt32	textBytes;
	UInt16	cp;
	FskRectangleRecord	r;

	for (cp = firstCodePoint; ; cp++) {
		UnicodeCharToUTF8(cp, encodedText, &textBytes);
		FskTextGetBounds(typeFace->fte, NULL, encodedText, textBytes, typeFace->textSize, typeFace->textStyle, typeFace->fontName, &r, NULL, typeFace->cache);
		if (bounds->width  < r.width)
			bounds->width  = r.width;
		if (bounds->height < r.height)
			bounds->height = r.height;
		if (cp == lastCodePoint)
			break;
	}
}
#endif /* !USE_GLYPH */

/****************************************************************************//**
 * Get the strike for a glyph.
 *	\param[in]		typeFace	the typeface and related state.
 *	\param[in,out]	strike		the information about the glyph strike.
 *								inputs:  codePoint, x, y
 *								outputs: width, height.
 *	\param[out]		bm			the bitmap to store the strike.
 *	\return			kFskErrNone	if the strike was retrieved successfully.
 *	\return			kFskErrRequestTooLarge	if the glyph was too large for the cell width or height.
 *											In this case, the strike width and height are the dimensions of the glyph.
 ********************************************************************************/

static FskErr StrikeGlyph(FskConstGLTypeFace typeFace, FskGlyphStrike strike, FskBitmap bm) {
	FskErr	err				= kFskErrNone;
#if !USE_GLYPH
	char	encodedText[4];
	UInt32	textBytes;
#endif /* !USE_GLYPH */
	FskRectangleRecord bounds;
	FskColorRGBARecord color;

#if !USE_GLYPH
	UnicodeCharToUTF8(strike->codePoint, encodedText, &textBytes);
	err = FskTextGetBounds(typeFace->fte, NULL, encodedText, textBytes, typeFace->textSize, typeFace->textStyle, typeFace->fontName, &bounds, NULL, typeFace->cache);
#else /* USE_GLYPH */
	err = FskTextGlyphGetBounds(typeFace->fte, NULL, &strike->codePoint, 1, typeFace->textSize, typeFace->textStyle, typeFace->fontName, &bounds, typeFace->cache);
	strike->xoffset = FskFixedToFloat(bounds.x);
	bounds.width = (SInt32)ceilf(FskFixedToFloat(bounds.width) - (strike->xoffset < 0 ? strike->xoffset : 0));
	bounds.height = (SInt32)ceilf(FskFixedToFloat(bounds.height));
#if TARGET_OS_MAC && USE_GLYPH && GLYPH_HAS_EDGE
//	bounds.width += 2 * GLYPH_HAS_EDGE;
	bounds.height += 2 * GLYPH_HAS_EDGE;
#endif
#endif /* USE_GLYPH */
	strike->width  = (UInt16)(bounds.width);
	strike->height = (UInt16)(bounds.height);
	if ((strike->width  > typeFace->cellWidth) || (strike->height > typeFace->cellHeight)) {
		#ifdef LOG_TEXT
			LOGI("Codepoint %04X is too large [%d %d] > [%d %d], signaling to resize cell size",
				strike->codePoint, (int)bounds.width, (int)bounds.height, typeFace->cellWidth, typeFace->cellHeight);
		#endif /* LOG_TEXT*/
		err = kFskErrRequestTooLarge;	/* This signals the caller to resize the cell size of the strike bitmap */
		FskColorRGBASet(&color,	64,	64,	64,	255);												/* Gray background signals an error */
	}
	else {
		FskColorRGBASet(&color,	0,	0,	0,	255);												/* Opaque background is normal */
	}
	FskRectangleSet(&bounds, strike->x, strike->y, typeFace->cellWidth, typeFace->cellHeight);
	FskRectangleFill(bm, &bounds, &color, kFskGraphicsModeCopy, NULL);
	BAIL_IF_ERR(err);

	FskColorRGBASet(&color, 255, 255, 255, 255);												/* Foreground is opaque */

#if USE_GLYPH
	err = FskTextGlyphBox(typeFace->fte, bm, &strike->codePoint, 1, &bounds, NULL, &color, 255, typeFace->textSize, typeFace->textStyle,
					kFskTextAlignLeft, kFskTextAlignTop, typeFace->fontName, typeFace->cache);	/* Strike glyph into cell */
#else /* !USE_GLYPH */
	err = FskTextBox(typeFace->fte, bm, encodedText, textBytes, &bounds, NULL, NULL, &color, 255, typeFace->textSize, typeFace->textStyle,
					kFskTextAlignLeft, kFskTextAlignTop, typeFace->fontName, typeFace->cache);	/* Strike glyph into cell */
#endif /* !USE_GLYPH */
bail:
	#if GL_DEBUG
		if (kFskErrRequestTooLarge == err)
			#ifdef LOG_TEXT
				LOGI("FskGLBlit.c, line %4d: err = kFskErrRequestTooLarge, @ StrikeGlyph", __LINE__);	/* A signal to resize the cell size */
			#else /* !LOG_TEXT */
				{}
			#endif /* !LOG_TEXT */
		else
			PRINT_IF_ERROR(err, __LINE__, "StrikeGlyph");
	#endif /* GL_DEBUG */
	return err;
}


/****************************************************************************//**
 * Resize the glyph table associated with a typeface.
 *	\param[in,out]		typeFace	the typeface and related glyph table.
 *	\param[in]			numGlyphs	the desired number of glyphs.
 *	\return				kFskErrNone	if the glyph table was enlarged successfully.
 ********************************************************************************/

static FskErr ResizeTypeFaceGlyphTable(FskGLTypeFace typeFace, UInt32 numGlyphs) {
	FskErr	err	= kFskErrNone;

	if (typeFace->glyphTabSize >= numGlyphs)
		goto bail;																														/* Already big enough */
	#ifdef LOG_TEXT
		LOGD("ResizeTypeFaceGlyphTable(%u)", (unsigned)numGlyphs);
	#endif /* LOG_TEXT */
	BAIL_IF_ERR(err = FskMemPtrRealloc(numGlyphs * sizeof(typeFace->glyphTab[0]), &typeFace->glyphTab));								/* Enlarge */
	FskMemSet(typeFace->glyphTab + typeFace->glyphTabSize, 0, (numGlyphs - typeFace->glyphTabSize) * sizeof(typeFace->glyphTab[0]));	/* Zero new entries */
	typeFace->glyphTabSize = numGlyphs;																									/* Update table size */

bail:
	PRINT_IF_ERROR(err, __LINE__, "ResizeTypeFaceGlyphTable");
	return err;
}


/****************************************************************************//**
 * Allocate a new bitmap for the type face glyph strikes.
 * Also resize or allocate the strike directory.
 * It is necessary to save the old bitmap, cell width, and cell height, because these are modified herein.
 * Also
 *	\param[in,out]	typeFace	the typeface. This holds:<br>
 *								- cellWidth[out]	- the width  of the cells used to hold a glyph.<br>
 *								- cellHeight[out]	- the height of the cells used to hold a glyph.<br>
 *								- numStrikes[out]	- the desired number of strikes; may be increased after return<br>
 *								- bm[out]			- the bitmap to be returned. Should be NULL on entry.
 *	\param[in]		numStrikes	- the number of strikes desired to be stored in the new bitmap.
 *	\param[in]		cellWidth	- the width of the cell for each strike.
 *	\param[in]		cellHeight	- the height of the cell for each strike.
 *	\return	kFskErrNone			if the bitmap was allocated successfully.
 *	\return	kFskErrAddressInUse	if typeFace->bm was not NULL.
 ********************************************************************************/

static FskErr NewGlyphStrikeBitmap(FskGLTypeFace typeFace, UInt32 numStrikes, UInt32 cellWidth, UInt32 cellHeight) {
	FskErr		err		= kFskErrNone;
	FskBitmap	bm		= NULL;
	unsigned	w, h, newNumStrikes;

	BAIL_IF_FALSE(typeFace->bm == NULL, err, kFskErrAddressInUse);
	BAIL_IF_FALSE(cellWidth > 0 && cellHeight > 0, err, kFskErrEmpty);

#if TARGET_OS_MAC && USE_GLYPH && GLYPH_HAS_EDGE
//	cellWidth += 2 * GLYPH_HAS_EDGE;
	cellHeight += 2 * GLYPH_HAS_EDGE;
#endif

	if (gGLGlobalAssets.maxTextureSize <= 0)		/* Even though gGLGlobalAssets initialization may have failed, ... */
		gGLGlobalAssets.maxTextureSize = 2048;		/* ... set globals to some amount of sanity. */

	/* Compute the size of a bitmap able to accommodate the specified number of strikes and cell size */
	w = (unsigned)sqrt((double)numStrikes * cellWidth * cellHeight);	/* Try to get a roughly square texture. */
	if (w > (unsigned)(gGLGlobalAssets.maxTextureSize))
		w = gGLGlobalAssets.maxTextureSize;
	else
		w = SmallestContainingPowerOfTwo(w);
	h = w / cellWidth;													/* Number of cells per line */
	h = (numStrikes + h - 1) / h;										/* Number of rows of cells */
	h *= cellHeight;													/* Number of pixels */
	h = gGLGlobalAssets.hasNPOT ? BLOCKIFY(h, ALPHA_BLOCK_VERTICAL)		/* Qualcomm Adreno crashes if not 32-blocked */
								: SmallestContainingPowerOfTwo(h);		/* Bump up to the next power of two, if cannot handle NPOT */
	if (h > (unsigned)(gGLGlobalAssets.maxTextureSize))
		h = gGLGlobalAssets.maxTextureSize;
	newNumStrikes = (w / cellWidth) * (h / cellHeight);					/* Capacity of bitmap in terms of the number of strikes. */

	/* Allocate the bitmap */
	#ifdef LOG_TEXT
	LOGD("NewGlyphStrikeBitmap(font={%s, sz=%g, st=$%03X}, cell=%ux%u, numStrikes=%u->%u->%u): allocating bitmap=%ux%u",
		typeFace->fontName, FloatTextSize(typeFace->textSize), typeFace->textStyle, (unsigned)cellWidth, (unsigned)cellHeight, (unsigned)typeFace->numStrikes, (unsigned)numStrikes, newNumStrikes, w, h);
	#endif /*LOG_TEXT */
	BAIL_IF_ERR(err = FskBitmapNew(w, h, kFskBitmapFormat8G, &bm));		/* Strike bitmap */
	FskBitmapWriteBegin(bm, NULL, NULL, NULL);
	FskMemSet(bm->bits, 0, bm->rowBytes * bm->bounds.height);			/* Clear bitmap to transparent */
	FskBitmapWriteEnd(bm);

	/* Resize the strike directory to be consistent with newNumStrikes */
	if (!typeFace->strikes)	{ BAIL_IF_ERR(err = FskMemPtrNewClear(newNumStrikes * sizeof(typeFace->strikes[0]), &typeFace->strikes));							/* First alloc */
	} else					{ BAIL_IF_ERR(err = FskMemPtrRealloc (newNumStrikes * sizeof(typeFace->strikes[0]), &typeFace->strikes));							/* Enlarge */
							  if (newNumStrikes > typeFace->numStrikes) FskMemSet(typeFace->strikes + typeFace->numStrikes, 0, (newNumStrikes - typeFace->numStrikes) * sizeof(typeFace->strikes[0]));	/* Zero new entries */
	}

	/* All went well: update the new values */
	typeFace->bm         = bm;
	typeFace->numStrikes = (UInt16)newNumStrikes;
	typeFace->cellWidth  = (UInt16)cellWidth;
	typeFace->cellHeight = (UInt16)cellHeight;

bail:
	if (err) {
		FskBitmapDispose(bm);
	}
	else {
		if (numStrikes > newNumStrikes)		/* Reached limits: cannot allocate any bigger */
			err = kFskErrTooMany;			/* Too many glyphs requested */
	}
	PRINT_IF_ERROR(err, __LINE__, "NewGlyphStrikeBitmap");
	return err;
}


/****************************************************************************//**
 * Copy a rectangle of bytes.
 * Necessary because FskBitmapDraw doesn't always accommodate kFskBitmapFormat8G.
 *	\param[in]	src	pointer to the the source pixel(0,0)
 *	\param[in]	srcRect		the upper left corner of the source. Width and height are ignored,
 *							instead using dstRect->width & dstRect->height.
 *	\param[in]	srcRowBytes	the byte stride from one source scanline to the next.
 *	\param[in]	dst	pointer to the the destination pixel(0,0)
 *	\param[in]	dstRect		the rectangle of the destination to be modified.
 *	\param[in]	dstRowBytes	the byte stride from one destination scanline to the next.
 ********************************************************************************/

static void CopyByteRect(const	UInt8 *src, FskConstRectangle srcRect, SInt32 srcRowBytes,
								UInt8 *dst, FskConstRectangle dstRect, SInt32 dstRowBytes
) {
	int w, h;
	src += srcRect->y * srcRowBytes + srcRect->x;
	dst += dstRect->y * dstRowBytes + dstRect->x;
	srcRowBytes -= dstRect->width;
	dstRowBytes -= dstRect->width;
	for (h = dstRect->height; h--; src += srcRowBytes, dst += dstRowBytes)
		for (w = dstRect->width; w--;)
			*dst++ = *src++;
}


/****************************************************************************//**
 * Transfer the glyph strikes from the src bitmap to the dst.
 *	\param[in,out]	typeFace	the typeface, whose strikes will be changed.
 *	\param[in]		srcBM		the source of the glyph strikes.
 *	\param[out]		dstBM		the destination of the src glyph strikes.
 ********************************************************************************/

static void TransferGlyphStrikes(FskGLTypeFace typeFace, Boolean canEasy, FskConstBitmap srcBM, FskBitmap dstBM) {
	const UInt8	*src;
	UInt8		*dst;
	SInt32		srcRowBytes, dstRowBytes;

	#ifdef LOG_TEXT
		LOGD("TransferGlyphStrikes %d from %dx%d to %dx%d, canEasy=%d", typeFace->lastStrikeIndex+1,
			(int)srcBM->bounds.width, (int)srcBM->bounds.height, (int)dstBM->bounds.width, (int)dstBM->bounds.height, canEasy);
	#endif /* TransferGlyphStrikes */

	FskBitmapReadBegin((FskBitmap)srcBM, (const void**)(const void*)&src, &srcRowBytes, NULL);
	FskBitmapWriteBegin(          dstBM,             (void**)(void*)&dst, &dstRowBytes, NULL);

	if (canEasy && dstBM->bounds.width == srcBM->bounds.width) {					/* Easy: same width. Strikes remain the same. */
		#if GL_DEBUG
			if (!((dstBM->bounds.width >= srcBM->bounds.width) && (dstBM->bounds.height >= srcBM->bounds.height))) {
				LOGE("TransferGlyphStrikes: DST IS NOT LARGER THAN SRC");
				return;
			}
		#endif /* GL_DEBUG */
		CopyByteRect(src, &srcBM->bounds, srcRowBytes, dst, &srcBM->bounds, dstRowBytes);
	}
	else {																			/* Strikes are rearranged into a wide bitmap. */
		FskRectangleRecord	srcRect, dstRect;
		unsigned		dstX		= 0,
						dstY		= 0,
						lastX		= dstBM->bounds.width  - typeFace->cellWidth,	/* Last horizontal and vertical positions. */
						lastY		= dstBM->bounds.height - typeFace->cellHeight;
		FskGlyphStrike	strike		= typeFace->strikes,
						endStrike	= typeFace->strikes + typeFace->lastStrikeIndex + 1;
		for (; strike < endStrike; ++strike) {
			srcRect.x = strike->x;			srcRect.y = strike->y;					/* Get the location of the strike in the srcBM */
			dstRect.x = strike->x = dstX;	dstRect.y = strike->y = dstY;			/* Set the new location of the strike in the dstBM */
			srcRect.width  = dstRect.width  = strike->width;
			srcRect.height = dstRect.height = strike->height;
			#if GL_DEBUG
				if (((dstRect.x + dstRect.width) > dstBM->bounds.width)	||
					((dstRect.y + dstRect.height) > dstBM->bounds.height)) {
					LOGE("TransferGlyphStrikes: CopyByteRect [%d,%d,%d,%d] > [0,0,%d,%d] OUT OF BOUNDS",
						(int)dstRect.x, (int)dstRect.y, (int)dstRect.width, (int)dstRect.height, (int)dstBM->bounds.width, (int)dstBM->bounds.height);
					goto bail;
				}
			#endif /* GL_DEBUG */
			CopyByteRect(src, &srcRect, srcRowBytes, dst, &dstRect, dstRowBytes);	/* Copy the strike */
			if ((dstX += typeFace->cellWidth) > lastX) {							/* Advance to the next horizontal... */
				dstX = 0;
				dstY += typeFace->cellHeight;										/* ... or vertical position. */
				if (dstY > lastY) {
					LOGE("TransferGlyphStrikes: dstY > lastY, OUT OF BOUNDS");
					goto bail;
				}
			}
		}
	}

bail:
	FskBitmapReadEnd((FskBitmap)srcBM);
	FskBitmapWriteEnd(           dstBM);
}


/****************************************************************************//**
 * Compare strikes.
 *	Note that this will order the strikes in order of *decreasing* last used time.
 *	\param[in]		v0	one strike.
 *	\param[in]		v1	another strike.
 *	\return			-1	if v0 was more recently used than v1.
 *	\return			+1	if v1 was more recently used than v0.
 *	\return			0	if v0 and v1 were most recently used at the same time.
 ********************************************************************************/

static int StrikeComparator(const void *v0, const void *v1) {
	FskConstGlyphStrike	*st0 = (FskConstGlyphStrike*)v0;
	FskConstGlyphStrike	*st1 = (FskConstGlyphStrike*)v1;
	int result;
	if      ((**st0).lastUsed < (**st1).lastUsed)	result = +1;	/* st1 is more recent, so should come first */
	else if ((**st0).lastUsed > (**st1).lastUsed)	result = -1;	/* st0 is more recent, so should come first */
	else											result =  0;	/* They were last accessed at the same time */
	return result;
}


/****************************************************************************//**
 * Remove the given percentage of floating strikes.
 *	Some of the strikes are considered to be fixed - these are not disturbed.
 *	The remainder are considered to be floating or in flux depending on storage needs.
 *	They are sorted by the time last used, and the least recently used ones are disposed.
 *	Note that the ones that remain are not necessarily in order of recentness.
 *	\param[in,out]	typeFace	the typeface and related strike table and bitmap.
 *	\param[in]		percent		the desired percentage [0..100] of strikes to remove.
 *								It is OK to remove 100% of the floating strikes.
 *	\return			kFskErrNone	if the strike table and bitmap was enlarged successfully.
 ********************************************************************************/

static void TossStrikes(FskGLTypeFace typeFace, int percent) {
	FskGlyphStrike			floatingStrike		= typeFace->strikes + typeFace->lastStaticStrike + 1;		/* Pointer to the first floating strike */
	const FskGlyphStrike	endStrike			= typeFace->strikes + typeFace->lastStrikeIndex  + 1;		/* Pointer beyond the last floating strike */
	const int				numFloatingStrikes	= typeFace->lastStrikeIndex - typeFace->lastStaticStrike;	/* These are strikes that we can dispose, maximun */
	FskGlyphStrike			*sortedStrikes		= NULL;														/* Array of pointers to glyph strikes */
	int						numToKeep, i;
	FskGlyphStrike			strike;
	UInt8					*strikePix;
	SInt32					strikeRowBytes;
	FskRectangleRecord		srcRect, dstRect;

	if (!(0 < percent && percent <= 100))	/* Sanity check */
		percent = 100;
	numToKeep = (numFloatingStrikes * (100 - percent)) / 100;

	LOGD("TossStrikes: tossing out %d%% floating=%d/%d static=%d", percent, numFloatingStrikes-numToKeep, numFloatingStrikes, typeFace->lastStaticStrike+1);

	/* Toss out the specified percentage of the least recently used glyph strikes, setting their code points to 0 */
	if ((0 == numToKeep) || (kFskErrNone != FskMemPtrNew(numFloatingStrikes * sizeof(FskGlyphStrike), &sortedStrikes))) {	/* Allocate array of strike pointers */
		/* Toss out 100% of the floating strikes */
		LOGD("TossStrikes: tossing out all %d floating strikes", numFloatingStrikes);
		numToKeep = 0;																					/* Set numToKeep if FskMemPtrNew() failed */
		for (strike = floatingStrike; strike < endStrike; ++strike) {
			typeFace->glyphTab[strike->codePoint] = 0;
			strike->codePoint = 0;
		}
	}
	else {																								/* Toss out less than 100% of the floating strikes. */
		/*  Sort the strikes to keep the most recently used. */
		for (i = numFloatingStrikes, strike = floatingStrike; i--; ++strike, ++sortedStrikes)
			*sortedStrikes = strike;																	/* Initialize them to their natural order */
		sortedStrikes -= numFloatingStrikes;															/* Restore pointer back to beginning of the array */
		FskQSort(sortedStrikes, numFloatingStrikes, sizeof(FskGlyphStrike), StrikeComparator);			/* Sort the strikes in order of progressively least recently used */
		for (i = numToKeep; i < numFloatingStrikes; ++i) {												/* Keep the first numToKeep strikes, toss the others */
			strike = sortedStrikes[i];
			typeFace->glyphTab[strike->codePoint] = 0;													/* Remove strike from the glyph table */
			strike->codePoint = 0;																		/* Deactivate the strike */
		}

		/* Rearrange the strike pixels and transfer the strike information to its new location */
		srcRect.width  = dstRect.width  = typeFace->cellWidth;											/* We will transfer the whole cell, ... */
		srcRect.height = dstRect.height = typeFace->cellHeight;											/* ... regardless of how much is being used, to maintain whitespace */
		FskBitmapWriteBegin(typeFace->bm, (void**)(void*)&strikePix, &strikeRowBytes, NULL);			/* Get access to the bitmap so we can rearrange the strikes */
		for (floatingStrike += numToKeep; floatingStrike < endStrike; ++floatingStrike) {				/* Squeeze out holes */
			if (0 == floatingStrike->codePoint) {														/* Found a hole */
				for (strike = floatingStrike + 1; strike < endStrike; ++strike)
					if (0 != strike->codePoint)															/* Found a strike */
						break;
				if (strike == endStrike)																/* No more strikes to move */
					break;																				/* Done */
				srcRect.x =         strike->x;			srcRect.y =         strike->y;					/* Old location of the strike pixels */
				dstRect.x = floatingStrike->x;			dstRect.y = floatingStrike->y;					/* New location of the strike pixels */
				#if GL_DEBUG
					if (((dstRect.x + dstRect.width ) > typeFace->bm->bounds.width )	||
						((dstRect.y + dstRect.height) > typeFace->bm->bounds.height)) {
						LOGE("TossStrikes: CopyByteRect [%d,%d,%d,%d] > [0,0,%d,%d] OUT OF BOUNDS",
							(int)dstRect.x, (int)dstRect.y, (int)dstRect.width, (int)dstRect.height, (int)typeFace->bm->bounds.width, (int)typeFace->bm->bounds.height);
						return;
					}
				#endif /* GL_DEBUG */
				CopyByteRect(strikePix, &srcRect, strikeRowBytes, strikePix, &dstRect, strikeRowBytes);	/* Transfer strike pixels from old location to new */
				*floatingStrike = *strike;																/* Fast, secure, maintainable copy, but wipes out x and y */
				floatingStrike->x = (UInt16)(dstRect.x);	floatingStrike->y = (UInt16)(dstRect.y);	/* Restore x and y of the strike */
				typeFace->glyphTab[strike->codePoint] = floatingStrike - typeFace->strikes;				/* Redirect the glyph strike index to the new, closer location */
				strike->codePoint = 0;																	/* Empty the old strike codepoint */
			}
		}
		FskBitmapWriteEnd(typeFace->bm);
		FskMemPtrDispose(sortedStrikes);
	}
	typeFace->lastStrikeIndex = typeFace->lastStaticStrike + numToKeep;
}


/****************************************************************************//**
 * Resize the glyph strike table and bitmap.
 *	\param[in,out]		typeFace	the typeface and related strike table and bitmap.
 *	\param[in]			numStrikes	the desired number of strikes.
 *	\return				kFskErrNone	if the strike table and bitmap was enlarged successfully.
 ********************************************************************************/

static FskErr ResizeStrikeMap(FskGLTypeFace typeFace, UInt32 numStrikes) {
	FskErr			err		= kFskErrNone;
	const FskBitmap	oldBM	= typeFace->bm;															/* Save the previous bitmap for later copying */
	FskGLPort		glPort	= oldBM->glPort;
	FskBitmap		newBM;

	if (typeFace->numStrikes >= numStrikes)															/* Already big enough */
		goto bail;																					/* Done */

	/* Resize the strike bitmap and determine its capacity in terms of strike size */
	#ifdef LOG_TEXT
		LOGD("ResizeStrikeMap: from %d to %d", (int)typeFace->numStrikes, (int)numStrikes);
	#endif /* LOG_TEXT */
	/* Resize glyph strike bitmap */
	typeFace->bm = NULL;																			/* NewGlyphStrikeBitmap now insists on this being NULL */
	err = NewGlyphStrikeBitmap(typeFace, numStrikes, typeFace->cellWidth, typeFace->cellHeight);	/* NewGlyphStrikeBitmap updates bm, numStrikes, strikes, cellWidth, cellHeight */
	if (kFskErrTooMany == err) {																	/* Reached the limit of the size font cache */
		TossStrikes(typeFace, 75);																	/* Toss 75% of the least recently used glyph strikes */
		err = kFskErrNone;																			/* We recovered from the bitmap allocation error */
		goto bail;																					/* All done */
	} else {
		BAIL_IF_ERR(err);
	}

	newBM = typeFace->bm;																			/* Just so we know which is new and which is old */
	newBM->glPort = glPort;																			/* Transfer the glPort to the new bitmap ... */
	oldBM->glPort = NULL;																			/* ... from the old */
	if (glPort) {
		glPort->texture.srcBM         = newBM;														/* Update bidirectional link for the port */
		glPort->texture.bounds.width  = 0;															/* It shouldn't be necessary to zero these ... */
		glPort->texture.bounds.height = 0;															/* ... to resize a texture, but is sufficient to guarantee */
	}
	typeFace->dirty = true;																			/* Need to reload texture */

	TransferGlyphStrikes(typeFace, true, oldBM, newBM);												/* Transfer the strikes from the old bitmap to the new */
	FskBitmapDispose(oldBM);

	if (glPort) {
		glPort->texture.bounds.width  = 0;															/* Invalidate texture ... */
		glPort->texture.bounds.height = 0;															/* ... dimensions */
	}
	if (glPort && glPort->texture.name != 0 && glPort->texture.name != GL_TEXTURE_UNLOADED)			/* If there is already a texture assigned, ... */
		err = ReshapeRGBTexture(GL_ALPHA, newBM->rowBytes, newBM->bounds.height, GL_ALPHA, GL_UNSIGNED_BYTE, &glPort->texture);	/* ... just resize the texture */
	else
		err = FskGLAccelerateBitmapSource(newBM);													/* Allocate a texture, though don't need to upload the texture.  */

bail:
	PRINT_IF_ERROR(err, __LINE__, "ResizeStrikeMap");
	return err;
}


/****************************************************************************//**
 * Resize the cell size of a type face.
 *	\param[in,out]		typeFace	the typeface and related strike table and bitmap.
 *	\param[in]			numStrikes	the desired number of strikes.
 *	\return				kFskErrNone	if the strike table and bitmap was enlarged successfully.
 ********************************************************************************/

static FskErr ResizeTypeFaceCellSize(FskGLTypeFace typeFace, UInt16 cellWidth, UInt16 cellHeight) {
	FskErr			err				= kFskErrNone;
	const FskBitmap	oldBM			= typeFace->bm;										/* Save the old bitmap so we can transfer the glyph strikes later */
	FskGLPort		glPort			= oldBM->glPort;									/* Save the old bitmap's glPort */

	#if defined(LOG_PARAMETERS) || defined(LOG_TEXT)
	LOGD("ResizeTypeFaceCellSize({%s %g $%03X} from %dx%d to %dx%d",
		typeFace->fontName, FloatTextSize(typeFace->textSize), (int)typeFace->textStyle,
		typeFace->cellWidth, typeFace->cellHeight, cellWidth, cellHeight);
	#endif /* defined(LOG_PARAMETERS) || defined(LOG_TEXT) */

	typeFace->bm = NULL;																/* NewGlyphStrikeBitmap now requires this, so we don't inadvertently lose a bitmap */
	err = NewGlyphStrikeBitmap(typeFace, typeFace->numStrikes, cellWidth, cellHeight);	/* NewGlyphStrikeBitmap updates bm, numStrikes, strikes, cellWidth, cellHeight */
	if (kFskErrTooMany == err)
		FskGLTypeFaceDispose(typeFace);

	BAIL_IF_ERR(err);

	typeFace->dirty        = true;														/* Need to reload texture */
	typeFace->bm->glPort   = glPort;													/* Transfer glPort to the new bitmap ... */
	oldBM->glPort          = NULL;														/* ... from the old */

	TransferGlyphStrikes(typeFace, false, oldBM, typeFace->bm);							/* Transfer the strikes from the old bitmap to the new */
	FskBitmapDispose(oldBM);

	if (glPort) {
		glPort->texture.srcBM         = typeFace->bm;									/* Update bidirectional link, if a texture was already allocated */
		glPort->texture.bounds.width  = 0;												/* Invalidate texture ... */
		glPort->texture.bounds.height = 0;												/* ... dimensions */
		if (glPort->texture.name != 0 && glPort->texture.name != GL_TEXTURE_UNLOADED)	/* If there is already a texture assigned, ... */
			err = ReshapeRGBTexture(GL_ALPHA, typeFace->bm->rowBytes, typeFace->bm->bounds.height, GL_ALPHA, GL_UNSIGNED_BYTE, &glPort->texture);	/* ... just resize the texture */
		else
			err = FskGLAccelerateBitmapSource(typeFace->bm);							/* Allocate a texture, though don't need to upload the texture.  */
	}

bail:
	PRINT_IF_ERROR(err, __LINE__, "ResizeTypeFaceCellSize");
	if (err) {
		LOGE("ResizeTypeFaceCellSize FAILED");
	}
	return err;
}


/********************************************************************************
 * FskGLTextStrikeGlyphRange
 ********************************************************************************/

static FskErr FskGLTextStrikeGlyphRange_(UInt16 firstCodePoint, UInt16 lastCodePoint, FskGLTypeFace typeFace) {
	FskErr			err			= kFskErrNone,
					stickyErr	= kFskErrNone;
	FskGlyphStrike	strike;
	unsigned		cellWidth	= typeFace->cellWidth,
					cellHeight	= typeFace->cellHeight;
	unsigned		x, y, lastX, lastY;
	UInt32			codePoint;

	#if TARGET_OS_WIN32 || TARGET_OS_MAC
		FskTextFormatCache tmpCache = NULL;	/* Windows and Mac require that we allocate a cache */
		if (NULL == typeFace->cache) {
			BAIL_IF_ERR(err = FskTextFormatCacheNew(typeFace->fte, &tmpCache, NULL, typeFace->textSize, typeFace->textStyle, typeFace->fontName));
			typeFace->cache = tmpCache;
		}
	#endif /* TARGET_OS_WIN32 || TARGET_OS_MAC */

	BAIL_IF_NULL(typeFace->bm, err, kFskErrBadState);				/* Make sure that we have a bitmap. */

	#if defined(LOG_PARAMETERS) || defined(LOG_TEXT)
		LOGD("GLTextStrikeGlyphRange([%04X, %04X](%d) {%s %g $%03X})", firstCodePoint, lastCodePoint, lastCodePoint-firstCodePoint+1, typeFace->fontName, FloatTextSize(typeFace->textSize), (int)typeFace->textStyle);
	#endif /* defined(LOG_PARAMETERS) || defined(LOG_TEXT) */

	/* Make sure that our codePoint-to-glyph table is large enough */
	if (lastCodePoint > typeFace->glyphTabSize) {					/* If the last code point doesn't fit into the table, ... */
		#if defined(LOG_TEXT)
			LOGD("GLTextStrikeGlyphRange: increasing glyphTabSize to %u", (unsigned)SmallestContainingPowerOfTwo(lastCodePoint));
		#endif /* LOG_TEXT */
		BAIL_IF_ERR(err = ResizeTypeFaceGlyphTable(typeFace, SmallestContainingPowerOfTwo(lastCodePoint)));	/* ... enlarge it. */
	}

	/* Make sure that our strike table and strike maps are big enough */
	codePoint = typeFace->lastStrikeIndex + 2;						/* UInt16 arithmetic is critical here because of our special use of -1 (codePoint is just temporary here) */
	x = (unsigned)codePoint + lastCodePoint - firstCodePoint + 1;	/* How large a table do we need? */
	if (x > typeFace->numStrikes) {									/* We need to allocate more strikes */
		if (x < (y = typeFace->numStrikes + 256))					/* If we don't need 256 more strikes ... */
			x = y;													/* ... allocate 256 extra anyway. */
		if (x > 65536)												/* But we never need ... */
			x = 65536;												/* ... more than 65536 */
		BAIL_IF_ERR(err = ResizeStrikeMap(typeFace, x));			/* This will probably increase the strike size, the strike table, and the strike bitmap, ... */
	}																/* ... but it may just toss out some least recently used glyphs */

restart:
	lastX = typeFace->bm->bounds.width  - cellWidth;				/* Last horizontal and vertical positions. */
	lastY = typeFace->bm->bounds.height - cellHeight;

	if ((UInt16)(-1) == typeFace->lastStrikeIndex) {				/* Virgin strikes. */
		strike = typeFace->strikes;									/* The first strike, */
		x = 0;														/* at the upper left. */
		y = 0;
	}
	else {
		strike = typeFace->strikes + typeFace->lastStrikeIndex;		/* Access the previous strike. */
		x = strike->x;												/* Initialize position to the previous position. */
		y = strike->y;
		if ((x += cellWidth) > lastX) {								/* Advance to the next horizontal... */
			x = 0;
			y += cellHeight;										/* ... or vertical position. */
			BAIL_IF_FALSE(y <= lastY, err, kFskErrTooMany);
		}
		++strike;													/* Advance to the next free strike. */
	}

	for (codePoint = firstCodePoint; codePoint <= lastCodePoint; ++codePoint) {
		if (codePoint == REPLACEMENT_CODE) {						/* Replacement character */
			typeFace->glyphTab[REPLACEMENT_CODE] = 0;				/* We standardize on placing the replacement character at glyph 0 */
			continue;
		}
#if USE_GLYPH
		else if (codePoint == 65535) {
			/* this seems to be the "undefined" glyph code */
			continue;
		}
#endif /* USE_GLYPH */
		if (typeFace->glyphTab[codePoint] != 0)						/* If already have a strike for this codePoint ... */
			continue;												/* ... move on to the next codePoint */
		strike->codePoint = (UInt16)codePoint;
		strike->x = x;
		strike->y = y;
		++(typeFace->lastStrikeIndex);								/* Update strike index to the last successful one. */
		if (typeFace->lastStrikeIndex >= typeFace->numStrikes) {
			/* Signal the caller to reallocate the strike directory and bitmap */
			err = kFskErrTooMany;
			--(typeFace->lastStrikeIndex);
			goto bail;
		}
		typeFace->glyphTab[codePoint] = typeFace->lastStrikeIndex;
		err = StrikeGlyph(typeFace, strike, typeFace->bm);
		if (kFskErrRequestTooLarge == err) {						/* The glyph is bigger than the cell size */
			--(typeFace->lastStrikeIndex);							/* Abort this strike */
			typeFace->glyphTab[codePoint] = 0;						/* Remove the glyph table entry for this character */
			if (cellWidth  < strike->width)
				cellWidth  = strike->width;
			if (cellHeight < strike->height)
				cellHeight = strike->height;
			strike->width = strike->height = 0;
			err = ResizeTypeFaceCellSize(typeFace, cellWidth, cellHeight);	/* TODO: use variable width cells. */
			if (stickyErr == kFskErrNone)
				stickyErr = err;
			BAIL_IF_ERR(err);
			firstCodePoint = (UInt16)codePoint;
			goto restart;
		}
		else if (kFskErrNone != err) {
			if (stickyErr == kFskErrNone)
				stickyErr = err;
		}

		if ((x += cellWidth) > lastX) {								/* Advance to the next horizontal... */
			x = 0;
			y += cellHeight;										/* ... or vertical position. */
			BAIL_IF_FALSE(y <= lastY, err, kFskErrTooMany);
		}
		++strike;													/* Advance to the next free strike. */
	}

	typeFace->dirty = true;

bail:
	PRINT_IF_ERROR(err, __LINE__, "FskGLTextStrikeGlyphRange");
	#if TARGET_OS_WIN32
		if (NULL != tmpCache) {
			FskTextFormatCacheDispose(typeFace->fte, tmpCache);
			typeFace->cache = NULL;
		}
	#endif /* TARGET_OS_WIN32 */
	return stickyErr;
}

#undef FskGLTextStrikeGlyphRange
FskErr FskGLTextStrikeGlyphRange(UInt16 firstCodePoint, UInt16 lastCodePoint, FskGLTypeFace typeFace) {
#if USE_GLYPH
	char	encodedText[8];
	UInt32	textBytes;
	UInt16  *glyphs = NULL;
	UInt32	numGlyphs, i;
	int codePoint;
	FskErr	err = kFskErrNone;

	for (codePoint = firstCodePoint; codePoint <= lastCodePoint; codePoint++) {
		UnicodeCharToUTF8(codePoint, encodedText, &textBytes);
		if ((err = FskTextGetGlyphs(typeFace->fte, NULL, encodedText, textBytes, typeFace->textSize, typeFace->textStyle, typeFace->fontName, &glyphs, &numGlyphs, NULL, NULL, NULL, typeFace->cache)) != kFskErrNone)
			return err;
		#ifdef LOG_TEXT
			LogTextGetGlyphs(encodedText, textBytes, typeFace->textSize, typeFace->textStyle, typeFace->fontName, glyphs, numGlyphs);
		#endif /* LOG_TEXT */
		/* must be 1 glyph but just in case... */
		for (i = 0; i < numGlyphs; i++) {
			if ((err = FskGLTextStrikeGlyphRange_(glyphs[i], glyphs[i], typeFace)) != kFskErrNone) {
				FskMemPtrDispose(glyphs);
				return err;
			}
		}
		if (codePoint == UNICODE_ELLIPSIS && numGlyphs > 0)
			typeFace->ellipsisCode = glyphs[0];
		FskMemPtrDispose(glyphs);
	}
	return err;
#else /* !USE_GLYPH */
	return FskGLTextStrikeGlyphRange_(firstCodePoint, lastCodePoint, typeFace);
#endif /* !USE_GLYPH */
}


/********************************************************************************
 * FskGLTypeFaceDispose
 ********************************************************************************/

#undef FskGLTypeFaceDispose
void FskGLTypeFaceDispose(FskGLTypeFace typeFace) {
	if (typeFace) {
		FskGLTypeFace *tf;
		#if defined(LOG_PARAMETERS) || defined(LOG_TEXT)
			LOGD("GLTypeFaceDispose(\"%s\" %g $%03X) %dx%d @ %p", typeFace->fontName, FloatTextSize(typeFace->textSize), (int)typeFace->textStyle,
				(int)typeFace->bm->bounds.width, (int)typeFace->bm->bounds.height, typeFace->bm);
		#endif /* LOG_TEXT */
		#if defined(DUMP_TYPEFACE)
		{	FILE *fd;
			char fName[256];
			snprintf(fName, sizeof(fName), "%s%g$%03X.%dx%dx8", typeFace->fontName, FloatTextSize(typeFace->textSize), (unsigned)typeFace->textStyle, (int)typeFace->bm->bounds.width, (int)typeFace->bm->bounds.height);
			if ((fd = fopen(fName, "wb")) != NULL) {
				if (typeFace->bm->rowBytes != typeFace->bm->bounds.width)
					ChangeRowBytes(typeFace->bm->bits, typeFace->bm->bounds.width, typeFace->bm->bounds.height,  typeFace->bm->depth>>3, typeFace->bm->rowBytes, typeFace->bm->bounds.width);
				fwrite(typeFace->bm->bits, typeFace->bm->bounds.width, typeFace->bm->bounds.height, fd);
				fclose(fd);
			}
		}
		#endif /* DUMP_TYPEFACE) */
		FskMemPtrDispose(typeFace->strikes);
		FskMemPtrDispose(typeFace->glyphTab);
		FskBitmapDispose(typeFace->bm);

		for (tf = &gGLGlobalAssets.typeFaces; *tf != NULL; tf = &((**tf).next)) {
			if (*tf == typeFace) {
				*tf = typeFace->next;
				typeFace->next = NULL;
				break;
			}
		}

		FskMemPtrDispose(typeFace);
	}
}


/********************************************************************************
 * FskGLTypeFaceNew
 ********************************************************************************/

#undef FskGLTypeFaceNew
FskErr FskGLTypeFaceNew(const char *fontName, UInt32 textSize, UInt32 textStyle, struct FskTextEngineRecord *fte, struct FskTextFormatCacheRecord *cache, FskGLTypeFace *pTypeFace) {
	FskErr				err				= kFskErrNone;
	FskGLTypeFace		typeFace;
	FskRectangleRecord	bounds;
	UInt32				z, integerTextSize;

	#if defined(LOG_PARAMETERS) || defined(LOG_TEXT)
		LOGD("GLTypeFaceNew(fontName=\"%s\" textSize=%g textStyle=$%03X fte=%p cache=%p)", fontName, FloatTextSize(textSize), (int)textStyle, fte, cache);
	#endif /* LOG_TEXT */

	BAIL_IF_ZERO(textSize, err, kFskErrSingular);

	textStyle &=	kFskTextPlain			|	/* Strip off truncation bits */
					kFskTextBold			|
					kFskTextItalic			|
					kFskTextUnderline		|
					kFskTextOutline			|
					kFskTextStrike			|
					kFskTextOutlineHeavy;

	/* Allocate the typeface data structure */
	*pTypeFace = NULL;
	z = FskStrLen(fontName) + 1;
	BAIL_IF_ERR(err = FskMemPtrNewClear(sizeof(**pTypeFace) + z, pTypeFace));			/* The data structure has the font name storage at the end */
	typeFace = *pTypeFace;
	typeFace->fontName			= (char*)(typeFace + 1); FskMemCopy(typeFace->fontName, fontName, z);	/* Copy the font name to its storage at the end of the typeface structure */
	typeFace->textSize			= textSize;
	typeFace->textStyle			= textStyle;
	typeFace->lastStrikeIndex	= (UInt16)(-1);
	typeFace->fte				= fte;
	typeFace->cache				= cache;
	typeFace->glyphTabSize		= 16384;												/* Initial size of codepoint to glyph table: allocated below */

    if ((integerTextSize = textSize) > 0x8000)											/* If the text size is too large to be considered an integer, ... */
    	integerTextSize = ROUND_FIXED_TO_INT(integerTextSize);							/* ... interpret it as a 16.16 fixed point number */
    typeFace->numStrikes = SmallestContainingPowerOfTwo(3072 / integerTextSize);		/* Initial size of strike directory, which determines the initial size of the bitmap */
    if (typeFace->numStrikes > 256)
		typeFace->numStrikes = 256;

	typeFace->dirty				= 1;
	//typeFace->lastTextTime	= 0;													/* Initialize LRU count */
	//typeFace->bm				= NULL;													/* NewGlyphStrikeBitmap insists on bm being NULL; it will allocate a new one */
	//typeFace->strikes			= NULL;													/* Strikes will be allocated by NewGlyphStrikeBitmap */
#if USE_GLYPH
	typeFace->ellipsisCode		= 0;
#endif /* USE_GLYPH */
#if TARGET_OS_MAC
	if (typeFace->cache == NULL) {
		BAIL_IF_ERR(err = FskTextFormatCacheNew(typeFace->fte, &typeFace->cache, NULL, typeFace->textSize, typeFace->textStyle, typeFace->fontName));
	}
#endif /* TARGET_OS_MAC */

	/* Determine the proper cell size for this font. TODO: is there an API for this? */
#if !USE_GLYPH
	FskRectangleSetEmpty(&bounds);
	FskTextGetUnicodeRangeStrikeBounds(typeFace,     0x0020,           0x007E,       &bounds);	/* Basic Latin				(95 chars) */
	FskTextGetUnicodeRangeStrikeBounds(typeFace,     0x00A0,           0x00FF,       &bounds);	/* Latin-1 supplement		(96 chars) */
	FskTextGetUnicodeRangeStrikeBounds(typeFace,     0xFFFD,           0xFFFD,       &bounds);	/* Replacement character	( 1 char)  */
	FskTextGetUnicodeRangeStrikeBounds(typeFace, UNICODE_ELLIPSIS, UNICODE_ELLIPSIS, &bounds);	/* ellipsis					( 1 char)  */
#else /* USE_GLYPH */
	{
		FskTextFontInfoRecord fontInfo;
		BAIL_IF_ERR(err = FskTextGetFontInfo(fte, &fontInfo, fontName, textSize, textStyle, typeFace->cache));
		FskRectangleSet(&bounds, 0, 0, fontInfo.width, fontInfo.height);	/* The width is usually much larger than we want, because of ligatures */
	}
#endif /* USE_GLYPH */
	#ifdef LOG_TEXT
		LOGD("%dx%d Initial cell rect for %s%g$%03X", bounds.width, bounds.height, fontName, FloatTextSize(textSize), (unsigned)textStyle);
	#endif /* LOG_TEXT */

	/* Allocate the strike bitmap, the strike directory, and the glyph-to-strike translation table */
	err = NewGlyphStrikeBitmap(typeFace, typeFace->numStrikes, bounds.width, bounds.height);	/* NewGlyphStrikeBitmap updates bm, numStrikes, strikes, cellWidth, cellHeight */
	if (kFskErrTooMany == err)
		FskGLTypeFaceDispose(typeFace);
	BAIL_IF_ERR(err);
	BAIL_IF_ERR(err = FskMemPtrNewClear(typeFace->glyphTabSize * sizeof(typeFace->glyphTab[0]), &(typeFace->glyphTab)));	/* CodePoint to glyph translator */

	typeFace->bm->pixelFormat = kFskBitmapFormat8A;
	FskBitmapSetOpenGLSourceAccelerated(typeFace->bm, true);
	typeFace->bm->pixelFormat = kFskBitmapFormat8G;

	/* Initialize it with a replacement character at glyph code 0 */
	typeFace->strikes[0].codePoint	= REPLACEMENT_CODE;									/* Replacement character */
	typeFace->strikes[0].x			= 0;
	typeFace->strikes[0].y			= 0;
	typeFace->lastStrikeIndex		= 0;
	BAIL_IF_ERR(err = StrikeGlyph(typeFace, typeFace->strikes, typeFace->bm));			/* Replacement character strike is now in the upper left of the bitmap */

	/* Also initialize the glyph strikes for Basic Latin. NB: start with the highest code point range to resize the glyph table only once. */
#if !USE_GLYPH	/* On iOS, making the initial bitmap took up most of the startup time, and performance did not seem to be impacted. */
	if (typeFace->numStrikes >= 32) {													/* If there is enough space, preload common glyphs */
		err = FskGLTextStrikeGlyphRange(UNICODE_ELLIPSIS, UNICODE_ELLIPSIS, typeFace);	/* ellipsis (…) */
		err = FskGLTextStrikeGlyphRange(0x0020, 0x0020, typeFace);						/* space */
		//err = FskGLTextStrikeGlyphRange(0x0020, 0x007E, typeFace);					/* space ... ~ (Basic Latin) */
	}
#endif /* !USE_GLYPH */
	typeFace->lastStaticStrike = typeFace->lastStrikeIndex;

	typeFace->next = gGLGlobalAssets.typeFaces;											/* Make linked list of typefaces. */
	gGLGlobalAssets.typeFaces = typeFace;												/* TODO: Typefaces should be purged every once in a while. */

bail:
	PRINT_IF_ERROR(err, __LINE__, "FskGLTypeFaceNew");
	return err;
}


/****************************************************************************//**
 * Get info about a typeface.
 *	\param[in]	typeFace	the typeface.
 *	\param[out]	pFontName	pointer to a location to store a pointer to the typeface's font name (can be NULL).
 *	\param[out]	pTextSize	pointer to a location to store the typeface's text size              (can be NULL).
 *	\param[out]	pTextStyle	pointer to a location to store the typeface's text style             (can be NULL).
 *	\return		kFskErrNone				if another typeface was found.
 *	\return		kFskErrIteratorComplete	if no more typefaces were found.
 ********************************************************************************/

static FskErr FskGLTypeFaceInfo(FskConstGLTypeFace typeFace, const char **pFontName, UInt32 *pTextSize, UInt32 *pTextStyle) {
	if (typeFace) {
		if (pFontName)	*pFontName  = typeFace->fontName;
		if (pTextSize)	*pTextSize  = typeFace->textSize;
		if (pTextStyle)	*pTextStyle = typeFace->textStyle;
		return kFskErrNone;
	}
	if (pFontName)	*pFontName  = NULL;
	if (pTextSize)	*pTextSize  = 0;
	if (pTextStyle)	*pTextStyle = 0;
	return kFskErrIteratorComplete;
}


/********************************************************************************
 * FskGLTypeFaceFirst
 ********************************************************************************/

#undef FskGLTypeFaceFirst
FskErr FskGLTypeFaceFirst(FskConstGLTypeFace *pTypeFace, const char **pFontName, UInt32 *pTextSize, UInt32 *pTextStyle) {
	return FskGLTypeFaceInfo(*pTypeFace = gGLGlobalAssets.typeFaces, pFontName, pTextSize, pTextStyle);
}


/********************************************************************************
 * FskGLTypeFaceNext
 ********************************************************************************/

#undef FskGLTypeFaceNext
FskErr FskGLTypeFaceNext(FskConstGLTypeFace *pTypeFace, const char **pFontName, UInt32 *pTextSize, UInt32 *pTextStyle) {
	if (*pTypeFace) *pTypeFace = (**pTypeFace).next;
	return FskGLTypeFaceInfo(*pTypeFace, pFontName, pTextSize, pTextStyle);
}


/****************************************************************************//**
 * Mesh a glyph
 *	\param[in]		strike	the glyph strike information.
 *	\param[in,out]	loc		the location of the strike, updated afterward by the width of the strike.
 *	\param[in]		stride	the stride between one xy pair and the next, or one uv pair and the next.
 *	\param[in,out]	pxy		pointer to an xy array, updated afterward by 6 * stride.
 *							The contents of these 6 locations are set to polygons of the destination.
 *	\param[in,out]	puv		pointer to a  uv array, updated afterward by 6 * stride.
 *							The contents of these 6 locations are set to polygons of the source.
 ********************************************************************************/

static void MeshAGlyph(FskConstGlyphStrike strike, const float texNorm[2], FskPointFloat loc, int stride, float **pxy, float **puv) {
	float	x0 = (float)(loc->x),
			x1 = (float)(loc->x + strike->width),
			y0 = (float)(loc->y),
			y1 = (float)(loc->y + strike->height),	/* Here we convert y1 from Y-down to Y-up; y0 has already been converted */
			u0 = texNorm[0] * (strike->x),
			u1 = texNorm[0] * (strike->x + strike->width),
			v0 = texNorm[1] * (strike->y),
			v1 = texNorm[1] * (strike->y + strike->height);
	float	*xy	= *pxy,
			*uv	= *puv;

	#ifdef MESH_WITH_TRIANGLES
		/* The triangles are scanned as { (0,0), (0,1), (1,1) }, { (1,0), (0,0), (1,1) }. */
		xy[0] = x0;		xy[1] = y0;		xy += stride;		uv[0] = u0;		uv[1] = v0;		uv += stride;
		xy[0] = x0;		xy[1] = y1;		xy += stride;		uv[0] = u0;		uv[1] = v1;		uv += stride;
		xy[0] = x1;		xy[1] = y1;		xy += stride;		uv[0] = u1;		uv[1] = v1;		uv += stride;
		xy[0] = x1;		xy[1] = y0;		xy += stride;		uv[0] = u1;		uv[1] = v0;		uv += stride;
		xy[0] = x0;		xy[1] = y0;		xy += stride;		uv[0] = u0;		uv[1] = v0;		uv += stride;
		xy[0] = x1;		xy[1] = y1;		xy += stride;		uv[0] = u1;		uv[1] = v1;		uv += stride;
	#else /* MESH_WITH_TRIANGLE_STRIP */
		/* The strip is scanned as { (0,0), (0,0), (1,0), (0,1), (1,1), (1,1) }
		 * Note the degenerate triangle at the beginning and the end to act as punctuation.
		 */
		xy[0] = x0;		xy[1] = y0;		xy += stride;		uv[0] = u0;		uv[1] = v0;		uv += stride;
		xy[0] = x0;		xy[1] = y0;		xy += stride;		uv[0] = u0;		uv[1] = v0;		uv += stride;
		xy[0] = x1;		xy[1] = y0;		xy += stride;		uv[0] = u1;		uv[1] = v0;		uv += stride;
		xy[0] = x0;		xy[1] = y1;		xy += stride;		uv[0] = u0;		uv[1] = v1;		uv += stride;
		xy[0] = x1;		xy[1] = y1;		xy += stride;		uv[0] = u1;		uv[1] = v1;		uv += stride;
		xy[0] = x1;		xy[1] = y1;		xy += stride;		uv[0] = u1;		uv[1] = v1;		uv += stride;
	#endif /* MESH_WITH_TRIANGLE_STRIP */

	loc->x += strike->width;	/* Advance  location  past this glyph */
	*pxy = xy;					/* Advance xy pointer past this glyph */
	*puv = uv;					/* Advance uv pointer past this glyph */
}


#define MESHGLYPH_LEFT(i)	xy[(i) * glyphStride + 0 * stride]	/**< The left  coordinate of the ith glyph (offsets 0, 1, or 4 would do). */
#define MESHGLYPH_RIGHT(i)	xy[(i) * glyphStride + 2 * stride]	/**< The right coordinate of the ith glyph (offsets 2, 3, or 5 would do). */
#define POINTS_PER_GLYPH	6									/**< Each glyph is a quad represented as two triangles */

/****************************************************************************//**
 * Truncate a text mesh
 *	\param[in]		truncateStyle	one of kFskTextTruncateEnd or kFskTextTruncateCenter.
 *	\param[in]		typeFace		the typeface being used.
 *	\param[in]		dstRect			the destination rectangle (only width and height are used).
 *	\param[in]		stride			the stride between one xy and the next, or uv and the next.
 *	\param[in]		texNorm			two normalizing factors for converting from pixel coordinates to texture coordinates.
 *	\param[in,out]	numPtsPtr		a pointer to the number of points. This is updated afterward.
 *	\param[in,out]	xy				a pointer to the xy vector, with stride given above.
 *	\param[in,out]	uv				a pointer to the uv vector, with stride given above.
 *	\return			the last X-coordinate.
 ********************************************************************************/

static int TruncateTextMesh(UInt32 truncateStyle, FskGLTypeFace typeFace, float dstWidth,
	int stride, const float texNorm[2], UInt32 *numPtsPtr, float *xy, float *uv
) {
	const int			glyphStride	= stride     * POINTS_PER_GLYPH;						/* The stride from one glyph to the next. */
	int					numGlyphs	= *numPtsPtr / POINTS_PER_GLYPH;						/* The number of glyphs represented by this mesh */
	float				right		= 0;
	int					i;
	FskGlyphStrike		strike;
	FskPointFloatRecord	loc;

	if (numGlyphs == 0)																	/* If there is no text ... */
		goto bail;																		/* ... we are done */
	right = MESHGLYPH_RIGHT(numGlyphs - 1);												/* This is the rightmost point in the mesh */
	if (right <= dstWidth)																/* If the string fits in the box ... */
		goto bail;																		/* ... we are done */

#if USE_GLYPH
	if (typeFace->ellipsisCode == 0)
		(void)FskGLTextStrikeGlyphRange(UNICODE_ELLIPSIS, UNICODE_ELLIPSIS, typeFace);
	if (typeFace->ellipsisCode >= typeFace->glyphTabSize)
		strike = &typeFace->strikes[0];
	else
		strike = &typeFace->strikes[typeFace->glyphTab[typeFace->ellipsisCode]];
#else /* !USE_GLYPH */
	if (UNICODE_ELLIPSIS >= typeFace->glyphTabSize								||		/* FskGLTypefaceNew guarantees that the glyph table is large enough for the ellipsis ...  */
		NULL == (strike = &typeFace->strikes[typeFace->glyphTab[UNICODE_ELLIPSIS]])		/* ... and that the ellipsis strike is loaded into the table ... */
	)
		strike = &typeFace->strikes[0];													/* ... so we should never get here, and we replace the ellipsis glyph with a replacement char */
#endif /* !USE_GLYPH */

	if (truncateStyle & kFskTextTruncateEnd) {
		int lastRight = (int)(MESHGLYPH_LEFT(0) + dstWidth - strike->width);			/* There will be enough room for the ellipsis if the previous character is no further than this */
		for (i = 0; i < numGlyphs; ++i)
			if (MESHGLYPH_RIGHT(i) > lastRight)
				break;
		if (i == numGlyphs)
			goto bail;
		xy += i * glyphStride;															/* Advance the xy pointer to the location of the ellipsis glyph */
		uv += i * glyphStride;															/* Advance the uv pointer to the location of the ellipsis glyph */
		loc.x = xy[0];																	/* Set the location where the glyph is to be placed */
		loc.y = xy[1];
		MeshAGlyph(strike, texNorm, &loc, stride, &xy, &uv);							/* Insert ellipsis */
		*numPtsPtr = (i + 1) * POINTS_PER_GLYPH;										/* Update the number of points in the mesh */
		right = MESHGLYPH_RIGHT(-1);													/* Get the right side of the mesh */
		#ifdef LOG_TEXT
			LOGD("Truncating text at end");
		#endif /* LOG_TEXT */
	}
	else if (truncateStyle & kFskTextTruncateCenter) {
		float	mid;
		int		midLeft, midRight, n;
		mid		= MESHGLYPH_LEFT(0) + (dstWidth - strike->width) * .5f;					/* Middle of the dstRect */
		for (midLeft = 0; MESHGLYPH_RIGHT(midLeft) <= mid;)								/* Find ... */
			++midLeft;																	/* ... the ... */
		--midLeft;																		/* ... last glyph before the ellipsis */
		mid = right - (dstWidth - strike->width) * .5f;									/* Half of the dstRect left of the right of the mesh */
		for (midRight = numGlyphs -1; MESHGLYPH_LEFT(midRight) >= mid;)					/* Find ... */
			--midRight;																	/* ... the ... */
		++midRight;																		/* ... first glyph after the ellipsis */
		i = (midLeft + 1) * glyphStride;												/* Index of the ellipsis */
		xy += i;																		/* Advance the xy pointer to the location of the ellipsis glyph */
		uv += i;																		/* Advance the uv pointer to the location of the ellipsis glyph */
		loc.x = xy[0];																	/* Get the position .. */
		loc.y = xy[1];																	/* ... where the ellipsis is to be placed */
		MeshAGlyph(strike, texNorm, &loc, stride, &xy, &uv);							/* Insert ellipsis */
		n = midRight - midLeft - 2;														/* The number of glyphs that we need to skip over */
		n *= glyphStride;																/* The number of floats that we need to skip over */
		mid = xy[n] - loc.x;															/* Delta x to bring the rightmost glyphs next to the ellipsis */
		for (i = (numGlyphs - midRight) * POINTS_PER_GLYPH; i-- > 0; xy += stride, uv += stride) {
			xy[0] = xy[0+n] - mid;														/* Copy x coordinate with a shift */
			xy[1] = xy[1+n];															/* Copy y coordinate */
			uv[0] = uv[0+n];															/* Copy u coordinate */
			uv[1] = uv[1+n];															/* Copy v coordinate */
		}
		numGlyphs = numGlyphs + midLeft - midRight + 2;									/* The current number of glyphs */
		*numPtsPtr = numGlyphs * POINTS_PER_GLYPH;										/* Update the number of points in the mesh */
		right = MESHGLYPH_RIGHT(-1);													/* Get the right side of the mesh */
		#ifdef LOG_TEXT
			LOGD("Truncating text in the center");
		#endif /* LOG_TEXT */
	}

bail:
	return (int)right;
}


/****************************************************************************//**
 * Assure that the necessary type face glyph strikes are loaded.
 ********************************************************************************/

static FskErr CheckTypeFaceGlyphStrikes(const char *text, UInt32 textLen, FskGLTypeFace typeFace) {
	FskErr		err			= kFskErrNone;
#if USE_GLYPH
	UInt16 *glyphs;
	UInt32 numGlyphs, i;
	BAIL_IF_ERR(err = FskTextGetGlyphs(typeFace->fte, NULL, text, textLen, typeFace->textSize, typeFace->textStyle, typeFace->fontName, &glyphs, &numGlyphs, NULL, NULL, NULL, typeFace->cache));
	#ifdef LOG_TEXT
		LogTextGetGlyphs(text, textLen, typeFace->textSize, typeFace->textStyle, typeFace->fontName, glyphs, numGlyphs);
	#endif /* LOG_TEXT */
	for (i = 0; i < numGlyphs; i++) {
		UInt16 gc = glyphs[i];
		if (gc >= typeFace->glyphTabSize || typeFace->glyphTab[gc] == 0) {
			BAIL_IF_ERR(err = FskGLTextStrikeGlyphRange_(glyphs[i], glyphs[i], typeFace));
		}
	}
#else /* !USE_GLYPH */
	const char	*endText	= text + textLen;

	while (text < endText) {
		UInt16 uc = UTF8ToUnicodeChar(&text, NULL);						/* This automatically advances */
		if (uc >= typeFace->glyphTabSize || 0 == typeFace->glyphTab[uc]) {
			BAIL_IF_ERR(err = FskGLTextStrikeGlyphRange(uc, uc, typeFace));
			typeFace->dirty = true;
		}
	}
#endif /* !USE_GLYPH */

bail:
#if USE_GLYPH
	FskMemPtrDispose(glyphs);
#endif /* !USE_GLYPH */
	PRINT_IF_ERROR(err, __LINE__, "CheckTypeFaceGlyphStrikes");
	return err;
}


/****************************************************************************//**
 * Make a text mesh.
 *	\param[in]	text			the text to be rendered, in UTF-8 format.
 *	\param[in]	textLen			the number of bytes used to represent the text.
 *	\param[in]	typeFace		the typeface.
 *	\param[in]	glyphTexture	the texture where the glyph strikes are located.
 *	\param[in]	dstRect			the location where the text is to be drawn.
 *	\param[in]	portHeight		the height of the port; used to invert Y from downscan to upscan.
 *	\param[in]	textHeight		the maximum height of the text, as determined with FskGLTextGetBounds().
 *	\param[in]	stride			the stride from one xy pair or uv pair to another, typically
 *								2 for planar: xyxyxy...uvuvuv..., or
 *								4 for chunky: xyuvxyuvxyuv...
 *	\param[in]	hAlign			the horizontal alignment of the text.
 *	\param[in]	vAlign			the  vertical  alignment of the text.
 *	\param[out]	numPtsPtr		returns the number of points generated.
 *	\param[out]	xyPtr			returns a pointer to the xy vector with the specified stride.
 *	\param[out]	uvPtr			returns a pointer to the uv vector with the specified stride.
 ********************************************************************************/

static FskErr NewTextMesh(
	const char			*text,
	UInt32				textLen,
	FskGLTypeFace		typeFace,
	GLTextureRecord		*glyphTexture,
	float				dstX,
	float				dstY,
	float				dstWidth,
	float				dstHeight,
	UInt32              textHeight,
	int					stride,
	UInt16				hAlign,
	UInt16				vAlign,
	UInt32				style,
	UInt32				*numPtsPtr,
	float				**xyPtr,
	float				**uvPtr
) {
	FskErr		err				= kFskErrNone;
	float		*xy				= 0,
				*uv				= 0;
	UInt32					numQuads, numPts, numBytes, i;
	FskPointFloatRecord		loc;
#if USE_GLYPH
	FskRectangleFloatRecord	textBounds;
#else
	FskRectangleRecord		textBounds;
#endif
	float					texNorm[2];

	#if USE_GLYPH
		UInt16		*glyphs = NULL;
		FskFixed	*layout			= NULL;
		BAIL_IF_ERR(err = FskTextGetGlyphs(typeFace->fte, NULL, text, textLen, typeFace->textSize, typeFace->textStyle, typeFace->fontName, &glyphs, &numQuads, &layout, &textBounds.width, &textBounds.height, typeFace->cache));
		#ifdef LOG_TEXT
			LogTextGetGlyphs(text, textLen, typeFace->textSize, typeFace->textStyle, typeFace->fontName, glyphs, numQuads);
		#endif /* LOG_TEXT */
	#elif USE_LAYOUT
		UInt16		*unicodeText	= NULL;
		FskFixed	*layout			= NULL;
		BAIL_IF_ERR(err = FskTextGetLayout(typeFace->fte, NULL, text, textLen, typeFace->textSize, typeFace->textStyle, typeFace->fontName, &unicodeText, &numQuads, &layout, typeFace->cache));
	#else /* !USE_LAYOUT */
		numQuads = NumberOfUnicodeCharacters(text, textLen);
	#endif /* USE_LAYOUT */
	numPts		= POINTS_PER_GLYPH * numQuads;				/* Two triangles per quad */
	numBytes	= numPts * 4 * sizeof(float);				/* (x,y,u,v) per point */
	BAIL_IF_ERR(err = AllocTempTileMesh(numBytes, &xy));
	switch (stride) {
		case 2:	uv = xy + 2 * numPts;	break;				/* Planar */
		case 4:	uv = xy + 2;			break;				/* Chunky */
		default:						goto bail;
	}
	*numPtsPtr = numPts;
	*xyPtr = xy;
	*uvPtr = uv;


	loc.x = 0;	/* Render in local coordinate system, in order to ...	*/	/* global coord: loc.x = dstRect->x;				*/
	loc.y = 0;	/* ... accommodate huge coordinates like +/-1073741824	*/	/*               loc.y = portHeight - dstRect->y;	*/
#if TARGET_OS_MAC && USE_GLYPH && GLYPH_HAS_EDGE
	loc.y -= GLYPH_HAS_EDGE;
#endif
	texNorm[0] = 1.f / glyphTexture->bounds.width;
	texNorm[1] = 1.f / glyphTexture->bounds.height;

	typeFace->lastTextTime++;													/* Augment the LRU count */

	/* Compose text using a triangle mesh textured with strikes from the strike cache. */
	for (i = 0; i < numQuads; ++i) {
		#if USE_GLYPH
			UInt16		gl = glyphs[i];
		#elif USE_LAYOUT
			UInt16		gl = unicodeText[i];
		#else /* !USE_LAYOUT */
			UInt32		utfBytes;
			UInt16		gl = UTF8ToUnicodeChar(&text, &utfBytes);				/* This automatically advances */
		#endif /* USE_LAYOUT */
		FskGlyphStrike	st;

		if (gl >= typeFace->glyphTabSize)	gl = 0;								/* Use the replacement glyph if we don't have the glyph installed. */
		else								gl = typeFace->glyphTab[gl];		/* Convert from unicode code point to glyph index */
		/* TODO: Load strikes for glyphs that have not yet been loaded.
		 * We need to distinguish between strikes that don't exist in this typeface and strikes that have not yet been loaded.
		 */
		st = &typeFace->strikes[gl];
		st->lastUsed = typeFace->lastTextTime;									/* Record the last time this glyph was used, to assist with strike cache management */
		#if USE_GLYPH
			loc.x = FskFixedToFloat(layout[i]) + (st->xoffset < 0 ? st->xoffset : 0);		/* layout[i] is already rounded up in the case of USE_GLYPH */
		#elif USE_LAYOUT
			loc.x = ROUND_FIXED_TO_INT(layout[i]);
		#endif /* USE_LAYOUT */
//#if TARGET_OS_MAC && USE_GLYPH && GLYPH_HAS_EDGE
//		loc.x -= GLYPH_HAS_EDGE;
//#endif
		MeshAGlyph(&typeFace->strikes[gl], texNorm, &loc, stride, &xy, &uv);	/* This automatically increments xy and uv */
	}

#if !USE_GLYPH
	textBounds.height = textHeight;
	textBounds.width = (SInt32)loc.x;	/* global coord: textBounds.width  = loc.x - dstRect->x; */
#else  /* USE_GLYPH */
	if (textHeight) {}	/* Remove unused parameter messages */
#endif /* USE_GLYPH */
	/* Insert an ellipsis if one of the truncate methods is requested, and the text is too wide; otherwise clip pixelwise */
	if ((0 != (style & (kFskTextTruncateEnd | kFskTextTruncateCenter))) && ceilf(textBounds.width) > dstWidth) {
		loc.x  = (float)TruncateTextMesh(style, typeFace, dstWidth, stride, texNorm, numPtsPtr, *xyPtr, *uvPtr);
		numPts = *numPtsPtr;
		textBounds.width = (SInt32)loc.x;
	}

	/* Determine the upper left of the text based on the specified alignment */
	textBounds.x = dstX;
	textBounds.y = dstY;
	switch (hAlign) {
		default:
	#if TARGET_OS_WIN32
		case kFskTextAlignCenter:	textBounds.x += (dstWidth - ((textBounds.width + 1) & ~1)) >> 1;		break;
		case kFskTextAlignRight:	textBounds.x +=  dstWidth -   textBounds.width;							break;
		case kFskTextAlignLeft:		textBounds.x +=  0;														break;
	#else
		case kFskTextAlignCenter:	textBounds.x +=    (dstWidth - textBounds.width) / 2;					break;
		case kFskTextAlignRight:	textBounds.x +=     dstWidth - textBounds.width;						break;
		case kFskTextAlignLeft:		textBounds.x +=  0;														break;
	#endif
	}
	switch (vAlign) {
		default:
		case kFskTextAlignTop:		textBounds.y +=  0;														break;
		case kFskTextAlignCenter:	textBounds.y +=      (dstHeight - textBounds.height) / 2;				break;
		case kFskTextAlignBottom:	textBounds.y +=       dstHeight - textBounds.height;					break;
	}

	/* Translate mesh coordinates to the desired location */
	{
#if USE_GLYPH
//@@jph
//@@		float dx = (float)roundf(textBounds.x);		/* Move to the nearest integer to align with the native API */
		float dx = textBounds.x;
#else
		float dx = (float)textBounds.x;
#endif
		float dy = (float)textBounds.y;
		for (i = numPts, xy = *xyPtr; i--; xy += stride) {
			xy[0] += dx;
			xy[1] += dy;
		}
	}

	if (typeFace->dirty) {									/* We assume that NewTextMesh is called in order to render text, ... */
		BAIL_IF_ERR(err = FskGLUpdateSource(typeFace->bm));	/* ... so we make sure that the typeFace texture is up-to-date */
		typeFace->dirty = false;
	}

bail:
	#if USE_GLYPH
		FskMemPtrDispose(glyphs);
		FskMemPtrDispose(layout);
	#elif USE_LAYOUT
		FskMemPtrDispose(unicodeText);
		FskMemPtrDispose(layout);
	#endif /* USE_LAYOUT */
	PRINT_IF_ERROR(err, __LINE__, "NewTextMesh");
	return err;
}


/****************************************************************************//**
 * Look for typeface.
 *	\param[in]	text		the text to be rendered, in UTF-8 format.
 *	\param[in]	textLen		the number of bytes used to represent the text.
 *	\param[in]	typeFace	the typeface.
 *	\param[in]	dstRect		the location where the text is to be drawn.
 *	\param[in]	portHeight	the height of the port; used to invert Y from downscan to upscan.
 *	\param[in]	stride		the stride from one xy pair or uv pair to another, typically
 *							2 for planar: xyxyxy...uvuvuv..., or
 *							4 for chunky: xyuvxyuvxyuv...
 *	\param[out]	numPtsPtr	returns the number of points generated.
 *	\param[out]	xyPtr		returns a pointer to the xy vector with the specified stride.
 *	\param[out]	uvPtr		returns a pointer to the uv vector with the specified stride.
 ********************************************************************************/

static FskGLTypeFace LookForTypeface(
	const char						*fontName,
	UInt32							textSize,
	UInt32							textStyle,
	struct FskTextEngineRecord		*fte,
	struct FskTextFormatCacheRecord	*cache
) {
	FskGLTypeFace face;

	textStyle &=	kFskTextPlain			|
					kFskTextBold			|
					kFskTextItalic			|
					kFskTextUnderline		|
					kFskTextOutline			|
					kFskTextStrike			|
					kFskTextOutlineHeavy;

	for (face = gGLGlobalAssets.typeFaces; face != NULL; face = face->next)
		if ((textSize  == face->textSize)					&&
			(textStyle == face->textStyle)                  &&
			(0 == FskStrCompare(fontName, face->fontName))
		)
			break;

	if (face) {
		face->fte   = fte;
		if (cache != NULL && face->cache != cache) {
			#if defined(LOG_TEXT)
				LOGD("LookForTypeface: replacing cache %p with %p", face->cache, cache);
				LogTextFormatCache(face->cache, "old cache");
				LogTextFormatCache(      cache, "new cache");
			#endif /* LOG_TEXT */
			face->cache = cache;
		}
	}

	return face;
}


//#define SUPPORT_YUV420i	/**< We support YUV 420i, by converting to YUV 420. */
#ifdef SUPPORT_YUV420i
/*******************************************************************************
 * Convert YUV420i to YUV420.
 *******************************************************************************/

static FskErr ConvertYUV420iYUV420(FskConstBitmap src, FskBitmap dst) {
	FskErr		err		= kFskErrNone;
	const UInt8	*s		= NULL;
	UInt8		*y		= NULL;
	UInt8		*u, *v;
	SInt32		yrb, srb, yBump, cBump, sBump;
	UInt32		width, h, w;

	BAIL_IF_ERR(err = FskBitmapReadBegin((FskBitmap)src, (const void**)(const void*)(&s), &srb, NULL));
	BAIL_IF_ERR(err = FskBitmapWriteBegin(dst,                       (void**)(void*)(&y), &yrb, NULL));

	width = dst->bounds.width  / 2;						/* Chroma width */
	h     = dst->bounds.height / 2;						/* Chroma height */
	u = y + dst->bounds.height * yrb;					/* Start of U plane */
	v = u + h                  * yrb / 2;				/* Start of V plane */
	yBump = 2 * yrb - dst->bounds.width * sizeof(*y);	/* Process 2 luma lines at a time */
	cBump = yrb / 2 - width * sizeof(*u);				/* or 1 chroma line */
	sBump = srb - width * 6;

	for (; h--; s += sBump, y += yBump, u += cBump, v += cBump) {
		for (w = width; w--; s += 6, y += 2, ++u, ++v) {
			u[0]     = s[0];
			v[0]     = s[1];
			y[0]     = s[2];
			y[1]     = s[3];
			y[0+yrb] = s[4];
			y[1+yrb] = s[5];
		}
	}

bail:
	if (s != NULL) FskBitmapReadEnd((FskBitmap)src);
	if (y != NULL) FskBitmapWriteEnd(dst);
	return err;
}
#endif /* SUPPORT_YUV420i */


/********************************************************************************
 * FskGLTextStrikeBitmap
 ********************************************************************************/

#undef FskGLTextStrikeBitmap
FskConstBitmap FskGLTextStrikeBitmap(FskConstGLTypeFace typeFace, FskPoint cellSize) {
	if (cellSize) {
		cellSize->x = typeFace->cellWidth;
		cellSize->y = typeFace->cellHeight;
	}
	return typeFace->bm;
}


/********************************************************************************
 * FskGLAllowNearestBitmap
 ********************************************************************************/

Boolean FskGLAllowNearestBitmap(void) {
	return gGLGlobalAssets.allowNearestBitmap;
}


/********************************************************************************
 * FskGLSetAllowNearestBitmap
 ********************************************************************************/

void FskGLSetAllowNearestBitmap(Boolean allow) {
	gGLGlobalAssets.allowNearestBitmap = allow;
}


#if GLES_VERSION == 2
/********************************************************************************
 * PerturbFloat
 * Make an infinitesimal change in the number so that equality comparisons will fail,
 * but the number would be essentially the same.
 ********************************************************************************/

static void PerturbFloat(float *f) {
	UInt32 *u = (UInt32*)(void*)f;
	(*u) ^= 1;
}
#endif /* GLES_VERSION == 2 */


/********************************************************************************
 * FindGLBlitContext
 ********************************************************************************/

static FskGLBlitContext* FindGLBlitContext(FskGLBlitContext ctx) {
	FskGLBlitContext *p;
	for (p = &gGLGlobalAssets.defaultContext.next; *p != NULL; p = &((**p).next))
		if (*p == ctx)
			return p;
	return NULL;
}


/********************************************************************************
 * RemoveGLBlitContext
 ********************************************************************************/

static FskErr RemoveGLBlitContext(FskGLBlitContext ctx) {
	FskGLBlitContext *p = FindGLBlitContext(ctx);
	if (NULL != p) {
		*p = ctx->next;
		return kFskErrNone;
	}
	return kFskErrNotFound;
}


#if 0
#pragma mark ======== GL DEBUG ========
#endif
#if defined(GL_DEBUG) && GL_DEBUG


#define TAG_WIDTH	24	/**< The amount of space allocated for a tag; should be the max tag width. */


/** A structure containing useful GL state.
 **/

typedef struct	FskGLState {
	GLboolean	blend;						/**< Whether or not to blend. */
	GLboolean	doubleBuffer;				/**< Whether or not to double buffer. */
	GLboolean	scissorTest;				/**< Whether or not to use a scissors test (clip). */
	GLboolean	textureEnabled;				/**< Whether or not texture mapping is enabled. */
	GLfloat		blendColor[4];				/**< The color to be used for blending. */
	GLfloat		clearColor[4];				/**< The color to be used to clear the RGB[A] buffer */
	GLfloat		modelViewMatrix[4][4];		/**< The value of the model view matrix. */
	GLfloat		projectionMatrix[4][4];		/**< The value of the projection matrix. */
	GLfloat		scissorBox[4];				/**< The scissor box (i.e. dst clip). */
	GLint		blendSrcRGB;				/**< The source of the source RGB. */
	GLint		blendSrcAlpha;				/**< The source of the source alpha. */
	GLint		blendDstRGB;				/**< The source of the destination RGB. */
	GLint		blendDstAlpha;				/**< The source of the destination alpha. */
	GLint		blendEquationRGB;			/**< The equation to be used for blending RGB. */
	GLint		blendEquationAlpha;			/**< The equation to be used for blending alpha. */
	GLint		drawBuffer;					/**< The buffer to be drawn to. */
	GLint		packAlignment;				/**< The alignment of data passed from the GPU to client space. */
	GLint		unpackAlignment;			/**< The alignment of data passed from client space to the GPU. */
	GLint		packRowLength;				/**< The Y-stride, in pixels, of pixels received from the GPU. */
	GLint		unpackRowLength;			/**< The Y-stride, in pixels, of pixels  passed   to  the GPU. */
	GLint		matrixMode;					/**< The current matrix mode. */
	GLint		modelViewStackDepth;		/**< The maximum depth available for the model view matrices. */
	GLint		projectionStackDepth;		/**< The maximum depth available for the model view matrices. */
	GLint		boundTexture;				/**< The "name" of the currently bound texture. */
	GLfloat		viewPort[4];				/**< The view port (x, y, width, height). */
} FskGLState;


/****************************************************************************//**
 * Print a GL boolean in table format.
 *	\param[in]	name	the name of the variable.
 *	\param[in]	val		pointer to the variable.
 *	\param[in]	size	the number of elements in the variable array.
 ********************************************************************************/

static void PrintGLBoolean(const char *name, const GLboolean *val, unsigned size) {
	const GLboolean undef = -1;
	char buf[16];
	printf("%*s", TAG_WIDTH, name);
	printf(" {");
	for (; size--; ++val)
		printf(" %14s",	*val == GL_FALSE	? "false"	:
						*val == GL_TRUE		? "true"	:
						*val == undef		? "undef"	:
						(snprintf(buf, sizeof(buf), "true%02X", *val), buf)
		);
	printf(" }\n");
}


/****************************************************************************//**
 * Print a GL integer in table format.
 *	\param[in]	name	the name of the variable.
 *	\param[in]	val		pointer to the variable.
 *	\param[in]	size	the number of elements in the variable array.
 ********************************************************************************/

static void PrintGLInteger(const char *name, const GLint *val, unsigned size) {
	printf("%*s", TAG_WIDTH, name);
	printf(" {");
	for (; size--; ++val)
		printf(" %14d", *val);
	printf(" }\n");
}


/****************************************************************************//**
 * Print a GL single precision floating point number in table format.
 *	\param[in]	name	the name of the variable.
 *	\param[in]	val		pointer to the variable.
 *	\param[in]	size	the number of elements in the variable array.
 ********************************************************************************/

static void PrintGLFloat(const char *name, const GLfloat *val, unsigned size) {
	printf("%*s", TAG_WIDTH, name);
	printf(" {");
	for (; size--; ++val)
		printf(" %14.7g", *val);
	printf(" }\n");
}


#if defined(GL_MODELVIEW_MATRIX) || defined(GL_PROJECTION_MATRIX)
/****************************************************************************//**
 * Print a GL single precision floating point matrix in table format.
 *	\param[in]	name	the name of the matrix.
 *	\param[in]	val		pointer to the matrix.
 *	\param[in]	nRows	the number of  rows   in the matrix.
 *	\param[in]	nCols	the number of columns in the matrix.
 ********************************************************************************/

static void PrintGLMatrix(const char *name, const GLfloat *val, unsigned nRows, unsigned nCols) {
	unsigned c;
	printf("%*s", TAG_WIDTH, name);
	printf(" {");
	for (; nRows--;) {
		for (c = nCols; c--; ++val)
			printf(" %14.7g", *val);
		printf("\n");
		if (nRows)
			printf("%*c", TAG_WIDTH + 2, ' ');
	}
	printf("%*c}\n", TAG_WIDTH + 1, ' ');
}
#endif /* defined(GL_MODELVIEW_MATRIX) || defined(GL_PROJECTION_MATRIX) */


/****************************************************************************//**
 * Print the given GL state.
 *	\param[in]	st	the GL state.
 ********************************************************************************/

static void FskGLStatePrint(const FskGLState *st) {
	#ifdef GL_COLOR_CLEAR_VALUE
		PrintGLFloat(  "clear color",				st->clearColor,				4);
	#endif /* GL_COLOR_CLEAR_VALUE */

	#ifdef GL_TEXTURE_2D
		PrintGLBoolean("texture mapping",			&st->textureEnabled,		1);
	#endif /* GL_TEXTURE_2D */

	#ifdef GL_TEXTURE_BINDING_2D
		PrintGLInteger("bound texture",				&st->boundTexture,			1);
	#endif /* GL_TEXTURE_BINDING_2D */

	#ifdef GL_BLEND
		PrintGLBoolean("blend",						&st->blend,					1);
	#endif /* GL_BLEND */

	#ifdef GL_BLEND_COLOR
		PrintGLFloat(  "blend color",				st->blendColor,				4);
	#endif /* GL_BLEND_COLOR */

	#if defined(GL_BLEND_EQUATION_RGB) || defined(GL_BLEND_EQUATION_RGB_OES)
		PrintGLInteger("blend equation RGB",		&st->blendEquationRGB,		1);
	#endif /* defined(GL_BLEND_EQUATION_RGB) || defined(GL_BLEND_EQUATION_RGB_OES) */

	#if defined(GL_BLEND_EQUATION_ALPHA) || defined(GL_BLEND_EQUATION_ALPHA_OES)
		PrintGLInteger("blend equationAlpha",		&st->blendEquationAlpha,	1);
	#endif /* defined(GL_BLEND_EQUATION_ALPHA) || defined(GL_BLEND_EQUATION_ALPHA_OES) */

	#if defined(GL_BLEND_SRC_RGB) || defined(GL_BLEND_SRC_RGB_OES)
		PrintGLInteger("blend src RGB",				&st->blendSrcRGB,			1);
	#endif /* defined(GL_BLEND_SRC_RGB) || defined(GL_BLEND_SRC_RGB_OES) */

	#if defined(GL_BLEND_SRC_ALPHA) \
	|| defined(GL_BLEND_SRC_RGB_OES)
		PrintGLInteger("blend src Alpha",			&st->blendSrcAlpha,			1);
	#endif /* defined(GL_BLEND_SRC_ALPHA) || defined(GL_BLEND_SRC_RGB_OES) */

	#if defined(GL_BLEND_DST_RGB) \
	||  defined(GL_BLEND_DST_RGB_OES)
		PrintGLInteger("blend dst RGB",				&st->blendDstRGB,			1);
	#endif /* defined(GL_BLEND_DST_RGB) || defined(GL_BLEND_DST_RGB_OES) */

	#if defined(GL_BLEND_DST_ALPHA) \
	||  defined(GL_BLEND_DST_ALPHA_OES)
		PrintGLInteger("blend dst Alpha",			&st->blendDstAlpha,			1);
	#endif /* defined(GL_BLEND_DST_ALPHA) || defined(GL_BLEND_DST_ALPHA_OES) */

	#ifdef GL_MATRIX_MODE
		PrintGLInteger("matrix mode",				&st->matrixMode,			1);
	#endif /* GL_MATRIX_MODE */

	#ifdef GL_MAX_MODELVIEW_STACK_DEPTH
		PrintGLInteger("modelview stack depth",		&st->modelViewStackDepth,	1);
	#endif /* GL_MAX_MODELVIEW_STACK_DEPTH */

	#ifdef GL_MAX_PROJECTION_STACK_DEPTH
		PrintGLInteger("projection stack depth",	&st->projectionStackDepth,	1);
	#endif /* GL_MAX_PROJECTION_STACK_DEPTH */

	#ifdef GL_MODELVIEW_MATRIX
		PrintGLMatrix( "modelview matrix",			st->modelViewMatrix[0],		4,	4);
	#endif /* GL_MODELVIEW_MATRIX */

	#ifdef GL_PROJECTION_MATRIX
		PrintGLMatrix( "projection matrix",			st->projectionMatrix[0],	4,	4);
	#endif /* GL_PROJECTION_MATRIX */

	#ifdef GL_PACK_ALIGNMENT
		PrintGLInteger("pack alignment",			&st->packAlignment,			1);
	#endif /* GL_PACK_ALIGNMENT */

	#ifdef GL_UNPACK_ALIGNMENT
		PrintGLInteger("unpack alignment",			&st->unpackAlignment,		1);
	#endif /* GL_UNPACK_ALIGNMENT */

	#if defined(GL_PACK_ROW_LENGTH)
		PrintGLInteger("pack row length",			&st->packRowLength,			1);
	#endif /* GL_PACK_ROW_LENGTH */

	#ifdef GL_UNPACK_ROW_LENGTH
		PrintGLInteger("unpack row length",			&st->unpackRowLength,		1);
	#endif /* GL_UNPACK_ROW_LENGTH */

	#ifdef GL_VIEWPORT
		PrintGLFloat(  "view port",					st->viewPort,				4);
	#endif /* GL_VIEWPORT */

	#ifdef GL_DOUBLEBUFFER
		PrintGLBoolean("double buffer",				&st->doubleBuffer,			1);
	#endif /* GL_DOUBLEBUFFER */

	#ifdef GL_DRAW_BUFFER
		PrintGLInteger("draw buffer",				&st->drawBuffer,			1);
	#endif /* GL_DRAW_BUFFER */

	#ifdef GL_SCISSOR_TEST
		PrintGLBoolean("scissor test",				&st->scissorTest,			1);
	#endif /* GL_SCISSOR_TEST */

	#ifdef GL_SCISSOR_BOX
		PrintGLFloat(  "scissor box",				st->scissorBox,				4);
	#endif /* GL_SCISSOR_BOX */
}


/********************************************************************************
 * Get the current GL state.
 *	\param[in]	st	a place to store the current GL state.
 *	\return		kFskErrNone	if the state was retrieved successfully.
 ********************************************************************************/

static FskErr FskGLStateGet(FskGLState *st) {
	FskMemSet(st, (char)(-1), sizeof(*st));			/* Initialize to -1 or NaN */

	#ifdef GL_BLEND
		glGetBooleanv(GL_BLEND,							&st->blend);
	#endif /* GL_BLEND */

	#ifdef GL_TEXTURE_2D
		glGetBooleanv(GL_TEXTURE_2D,					&st->textureEnabled);
	#endif /* GL_TEXTURE_2D */

	#ifdef GL_BLEND_COLOR
		glGetFloatv(  GL_BLEND_COLOR,					st->blendColor);
	#endif /* GL_BLEND_COLOR */

	#ifdef GL_TEXTURE_BINDING_2D
		glGetIntegerv(GL_TEXTURE_BINDING_2D,			&st->boundTexture);
	#endif /* GL_TEXTURE_BINDING_2D */

	#ifdef GL_BLEND_EQUATION_RGB
		glGetIntegerv(GL_BLEND_EQUATION_RGB,			&st->blendEquationRGB);
	#elif GL_BLEND_EQUATION_RGB_OES
		glGetIntegerv(GL_BLEND_EQUATION_RGB_OES,		&st->blendEquationRGB);
	#endif /* !GL_BLEND_EQUATION_RGB */

	#ifdef GL_BLEND_EQUATION_ALPHA
		glGetIntegerv(GL_BLEND_EQUATION_ALPHA,			&st->blendEquationAlpha);
	#elif GL_BLEND_EQUATION_ALPHA_OES
		glGetIntegerv(GL_BLEND_EQUATION_ALPHA_OES,		&st->blendEquationAlpha);
	#endif /* GL_BLEND_EQUATION_ALPHA */

	#ifdef GL_BLEND_SRC_RGB
		glGetIntegerv(GL_BLEND_SRC_RGB,					&st->blendSrcRGB);
	#elif GL_BLEND_SRC_RGB_OES
		glGetIntegerv(GL_BLEND_SRC_RGB_OES,				&st->blendSrcRGB);
	#endif /* !GL_BLEND_SRC_RGB */

	#ifdef GL_BLEND_SRC_ALPHA
		glGetIntegerv(GL_BLEND_SRC_ALPHA,				&st->blendSrcAlpha);
	#elif GL_BLEND_SRC_ALPHA_OES
		glGetIntegerv(GL_BLEND_SRC_ALPHA_OES,			&st->blendSrcAlpha);
	#endif /* !GL_BLEND_SRC_ALPHA */

	#ifdef GL_BLEND_DST_RGB
		glGetIntegerv(GL_BLEND_DST_RGB,					&st->blendDstRGB);
	#elif GL_BLEND_DST_RGB_OES
		glGetIntegerv(GL_BLEND_DST_RGB_OES,				&st->blendDstRGB);
	#endif /* GL_BLEND_DST_RGB */

	#ifdef GL_BLEND_DST_ALPHA
		glGetIntegerv(GL_BLEND_DST_ALPHA,				&st->blendDstAlpha);
	#elif GL_BLEND_DST_ALPHA_OES
		glGetIntegerv(GL_BLEND_DST_ALPHA_OES,			&st->blendDstAlpha);
	#endif /* GL_BLEND_DST_ALPHA_OES */

	#ifdef GL_COLOR_CLEAR_VALUE
		glGetFloatv(  GL_COLOR_CLEAR_VALUE,				st->clearColor);
	#endif /* GL_COLOR_CLEAR_VALUE */

	#ifdef GL_DOUBLEBUFFER
		glGetBooleanv(GL_DOUBLEBUFFER,					&st->doubleBuffer);
	#endif /* GL_DOUBLEBUFFER */

	#ifdef GL_DRAW_BUFFER
		glGetIntegerv(GL_DRAW_BUFFER,					&st->drawBuffer);
	#endif /* GL_DRAW_BUFFER */

	#ifdef GL_MATRIX_MODE
		glGetIntegerv(GL_MATRIX_MODE,					&st->matrixMode);
	#endif /* GL_MATRIX_MODE */

	#ifdef GL_MAX_MODELVIEW_STACK_DEPTH
		glGetIntegerv(GL_MAX_MODELVIEW_STACK_DEPTH,		&st->modelViewStackDepth);
	#endif /* GL_MAX_MODELVIEW_STACK_DEPTH */

	#ifdef GL_MAX_PROJECTION_STACK_DEPTH
		glGetIntegerv(GL_MAX_PROJECTION_STACK_DEPTH,	&st->projectionStackDepth);
	#endif /* GL_MAX_PROJECTION_STACK_DEPTH */

	#ifdef GL_PACK_ALIGNMENT
		glGetIntegerv(GL_PACK_ALIGNMENT,				&st->packAlignment);
	#endif /* GL_PACK_ALIGNMENT */

	#ifdef GL_UNPACK_ALIGNMENT
		glGetIntegerv(GL_UNPACK_ALIGNMENT,				&st->unpackAlignment);
	#endif /* GL_UNPACK_ALIGNMENT */

	#if defined(GL_PACK_ROW_LENGTH)
		glGetIntegerv(GL_PACK_ROW_LENGTH,				&st->packRowLength);
	#endif /* GL_PACK_ROW_LENGTH */

	#ifdef GL_UNPACK_ROW_LENGTH
		glGetIntegerv(GL_UNPACK_ROW_LENGTH,				&st->unpackRowLength);
	#endif /* GL_UNPACK_ROW_LENGTH */

	#ifdef GL_MODELVIEW_MATRIX
		glGetFloatv(  GL_MODELVIEW_MATRIX,				st->modelViewMatrix[0]);
	#endif /* GL_MODELVIEW_MATRIX */

	#ifdef GL_PROJECTION_MATRIX
		glGetFloatv(  GL_PROJECTION_MATRIX,				st->projectionMatrix[0]);
	#endif /* GL_PROJECTION_MATRIX */

	#ifdef GL_SCISSOR_BOX
		glGetFloatv(  GL_SCISSOR_BOX,					st->scissorBox);
	#endif /* GL_SCISSOR_BOX */

	#ifdef GL_SCISSOR_TEST
		glGetBooleanv(GL_SCISSOR_TEST,					&st->scissorTest);
	#endif /* GL_SCISSOR_TEST */

	#ifdef GL_VIEWPORT
		glGetFloatv(  GL_VIEWPORT,						st->viewPort);
	#endif /* GL_VIEWPORT */

	return GetFskGLError();
}


/********************************************************************************
 * FskPrintGLState
 ********************************************************************************/

void FskPrintGLState(void) {
	FskGLState st;
	FskGLStateGet(&st);
	FskGLStatePrint(&st);
}


#endif /* GL_DEBUG */


/********************************************************************************
 ********************************************************************************
 ********************************************************************************
 *****								Private API								*****
 ********************************************************************************
 ********************************************************************************
 ********************************************************************************/


/****************************************************************************//**
 * FskGLGetViewMatrix - gets a pointer to the current view matrix.
 *	\param[in]	matrixSeed	A place to store the current matrix seed. Can be NULL.
 *	\return	a pointer to the current view matrix.
 ********************************************************************************/

#undef FskGLGetViewMatrix
const float* FskGLGetViewMatrix(UInt32 *matrixSeed) {
	#if GLES_VERSION == 2
		if (matrixSeed) *matrixSeed = gGLGlobalAssets.matrixSeed;
		return gGLGlobalAssets.matrix[0];
	#else /* GLES_VERSION == 1 */
		return NULL;
	#endif /* GLES_VERSION == 1 */
}


#if 0
#pragma mark ======== API ========
#endif
/********************************************************************************
 ********************************************************************************
 ********************************************************************************
 *****									API									*****
 ********************************************************************************
 ********************************************************************************
 ********************************************************************************/


/********************************************************************************
 * FskGLInit
 ********************************************************************************/

#undef FskGLInit
FskErr FskGLInit(void *v) {
	FskErr	err	= kFskErrNone;

	#ifdef LOG_INIT
		LOGD("[%s] FskGLInit", FskThreadName(FskThreadGetCurrent()));
	#endif /* LOG_INIT */

	#if TARGET_OS_ANDROID
		/* This needs to be done:
		 * (1) in the same thread of the virtual machine.
		 * (2) with a pointer to the native window, passed as the parameter v.
		 */
		BAIL_IF_ERR(err = FskEGLWindowContextInitialize(565, v));
	#else /* !TARGET_OS_ANDROID */
		if (v) {}	/* Get rid of "unused" messages. */
	#endif /* TARGET_OS_ANDROID */

	BAIL_IF_ERR(err = InitGlobalGLAssets());

bail:
	return err;
}



/********************************************************************************
 * FskGLShutdown
 ********************************************************************************/

#undef FskGLShutdown
FskErr FskGLShutdown(void) {
	FskErr	err	= kFskErrNone;
	#if GL_DEBUG
		LOGD("[%s] FskGLShutdown", FskThreadName(FskThreadGetCurrent()));
	#endif /* GL_DEBUG */
	DisposeGlobalGLAssets();
//	#if TARGET_OS_ANDROID || TARGET_OS_WIN32 || TARGET_OS_KPL
//		err = FskEGLWindowContextShutdown();
//	#endif /* TARGET_OS_ANDROID */
	return err;
}


/********************************************************************************
 * FskGLCapabilitiesGet
 ********************************************************************************/

#undef FskGLCapabilitiesGet
FskErr FskGLCapabilitiesGet(FskGLCapabilities *pCaps) {
	FskErr		err				= kFskErrNone;
	const char	*vendor			= NULL;
	const char	*renderer		= NULL;
	const char	*version		= NULL;
	const char	*GLSL_version	= NULL;
	const char	*extensions		= NULL;
	FskGLCapabilities	caps	= NULL;
	UInt32		n, size;
	const char	*s, **c;
	char		*d;

	*pCaps = NULL;

#ifdef GL_VENDOR
	vendor = (const char*)glGetString(GL_VENDOR);
#endif /* GL_VENDOR */
#ifdef GL_RENDERER
	renderer = (const char*)glGetString(GL_RENDERER);
#endif /* GL_RENDERER */
#ifdef GL_VERSION
	version = (const char*)glGetString(GL_VERSION);
#endif /* GL_VENDOR */
#ifdef GL_SHADING_LANGUAGE_VERSION
	GLSL_version = (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION);
#endif /* GL_SHADING_LANGUAGE_VERSION */
#ifdef GL_EXTENSIONS
	extensions = (const char*)glGetString(GL_EXTENSIONS);
#endif /* GL_EXTENSIONS */

	BAIL_IF_FALSE((renderer != NULL && version != NULL), err, kFskErrGraphicsContext);	/* Fails if GL has not been inited */

	/* Determine storage needs and allocate */
	size = sizeof(FskGLCapabilitiesRecord);
	if (NULL != vendor)			size += 1 + strlen(vendor);
	if (NULL != renderer)		size += 1 + strlen(renderer);
	if (NULL != version)		size += 1 + strlen(version);
	if (NULL != GLSL_version)	size += 1 + strlen(GLSL_version);
	if (NULL != extensions)		size += 1 + strlen(extensions);
	n = 0;								/* Initialize the count of capabilities */
	if (NULL != extensions) {
		s = extensions;
		if (0 != *s) {					/* If there is at least one character ... */
			++n;						/* ... we count the first string. */
			do {
				if (*s == ' ')			/* A space indicates the start of a new capability ... */
					++n;				/* ... so we increment the count of capabilities. */
			} while (*++s);
		}
	}
	size += (n + 1) * sizeof(caps->capabilities[0]);	/* Allocate one extra capability pointer for a NULL terminator. */
	BAIL_IF_ERR(err = FskMemPtrNewClear(size, &caps));

	/* Initialize capabilities list */
	caps->numCapabilities = n;
	c = caps->capabilities;
	d = (char*)(c + (n + 1));			/* Place to copy the capabilities strings */
	if (NULL != extensions) {
		n = 0;
		s = extensions;
		*c = d;							/* Set the first pointer to the beginning of the string */
		while ((*d++ = *s++) != 0) {	/* Copy all characters from source to destination */
			if (d[-1] == ' ') {			/* If the character was a space, ... */
				d[-1] = 0;				/* ... replace the space by a NULL, because spaces in the source separate identifiers */
				if (c[0][0] != 0) {		/* If the current string was non-trivial ... */
					*++c  = d;			/* Initialize next capability pointer to the next potential string. */
					++n;				/* Increment the count of non-trivial capabilities */
				}
				else {
					*c = d;				/* String was NULL; skip over it */
				}
			}
		}
		if (c[0][0] != 0) {				/* If the last capability was non-trivial, ... */
			++c;						/* ... increment the capabilities pointer ... */
			++n;						/* ... and increment the number of non-trivial capabilities */
		}
		caps->numCapabilities = n;		/* record the actual number of non-trivial capabilities */
	}
	*c = NULL;							/* NULL termination for list */

	/* Initialize other strings */
	if (NULL != vendor)			for (caps->vendorStr   = d, s = vendor;       (*d++ = *s++) != 0;) {}	/* Initialize vendor */
	if (NULL != renderer)		for (caps->rendererStr = d, s = renderer;     (*d++ = *s++) != 0;) {}	/* Initialize renderer */
	if (NULL != version)		for (caps->versionStr  = d, s = version;      (*d++ = *s++) != 0;) {}	/* Initialize version */
	if (NULL != GLSL_version)	for (caps->glslVersion = d, s = GLSL_version; (*d++ = *s++) != 0;) {}	/* Initialize GLSL_version */

	/* Verify that we haven't written outside the buffer */
	n = (UInt32)(d - (char*)caps);
	if (n > size) {
		err = kFskErrBufferOverflow;
		goto bail;
	}

	*pCaps = caps;

bail:
	PRINT_IF_ERROR(err, __LINE__, "FskGLCapabilitiesGet");
	if (err)	FskGLCapabilitiesDispose(caps);
	return err;
}


/********************************************************************************
 * FskGLCapabilitiesHas
 ********************************************************************************/

#undef  FskGLCapabilitiesHas
Boolean FskGLCapabilitiesHas(FskConstGLCapabilities caps, const char *queryCap) {
	if (caps) {
		unsigned			i	= caps->numCapabilities;
		char const*const*	p	= caps->capabilities;
		for (; i--; ++p)
			if (0 == FskStrCompare(queryCap, *p))
				return true;
	}
	return false;
}


/********************************************************************************
 * FskGLSourceTypes
 ********************************************************************************/

#undef FskGLSourceTypes
void FskGLSourceTypes(UInt32 formats[4]) {
	if (formats)
		FskMemCopy(formats, gGLGlobalAssets.srcFormats, sizeof(gGLGlobalAssets.srcFormats));
}


/********************************************************************************
 * FskGLSetFocusDefocusProcs
 ********************************************************************************/

#undef FskGLSetFocusDefocusProcs
void   FskGLSetFocusDefocusProcs(FskGLFocusProc focus, FskGLDefocusProc defocus) {
	gGLGlobalAssets.focus	= focus;
	gGLGlobalAssets.defocus	= defocus;
}


/********************************************************************************
 * FskGLGetFocusDefocusProcs
 ********************************************************************************/

#undef FskGLGetFocusDefocusProcs
void   FskGLGetFocusDefocusProcs(FskGLFocusProc *pFocus, FskGLDefocusProc *pDefocus) {
	if (pFocus)		*pFocus   = gGLGlobalAssets.focus;
	if (pDefocus)	*pDefocus = gGLGlobalAssets.defocus;
}


/********************************************************************************
 * FskGLPortNew
 ********************************************************************************/

#undef FskGLPortNew
FskErr FskGLPortNew(UInt32 width, UInt32 height, void *sysContext, FskGLPort *pPort) {
	FskErr		err		= kFskErrNone;
	FskGLPort	port	= NULL;

	/* Allocate port */
	*pPort = NULL;
	BAIL_IF_ERR(err = FskMemPtrNewClear(sizeof(FskGLPortRecord), pPort));
	port = *pPort;
	port->sysContext = sysContext;
	port->portWidth  = width;								/* Setting width and height to nonzero makes this port a destination. */
	port->portHeight = height;
	GLPortListInsertPort(&gGLGlobalAssets.activePorts, port);	/* Insert this port into the active list */

	/* Initialize GL to a known state */
	if (FskGLPortIsDestination(port)) {
		LOGD("[%s, thread=%p] FskGLPortNew(%u, %u; is dst)", "---"/*FskThreadName(FskThreadGetCurrent())*/, FskThreadGetCurrent(), (unsigned)width, (unsigned)height);
		(void)InitGlobalGLAssets();							/* This will fail first time through, but does enough setup to allow FskGLPortFocus() */
		FskGLPortFocus(port);								/* Focus, so GL calls have an appropriate context. */
		if (gCurrentGLPort == NULL)
			gCurrentGLPort = port;
		if (kFskErrNone == (err = InitGlobalGLAssets())) {	/* This will fail if GL has not yet been initialized */
			err = FskGLResetAllState(port);					/* If this succeeds the next calls will succeed */
			glClearColor(0., 0., 0., 0.);					/* Initialize it to transparent black */
			glClear(GL_COLOR_BUFFER_BIT);
			#if CHECK_GL_ERROR
				if (kFskErrNone == err)
					err = GetFskGLError();
			#endif /* CHECK_GL_ERROR */
		}

		FskGLPortDefocus(port);
	}

	PRINT_IF_ERROR(err, __LINE__, "in FskGLPortNew");

bail:
	return err;
}


/********************************************************************************
 * FskGLPortDispose
 ********************************************************************************/

#undef FskGLPortDispose
FskErr FskGLPortDispose(FskGLPort port) {
#if !USE_PORT_POOL
	return GLPortReallyDispose(port);
#else /* USE_PORT_POOL */
	FskErr	err;
	int		freePortCount;

	if (port == NULL)
		return kFskErrNone;																			/* We always allow NULL as a valid argument to Dispose procs */

	if (0 == port->texture.name || GL_TEXTURE_UNLOADED != port->texture.name ||						/* If there is no texture attached, ... */
		port->texture.bounds.width > SAVE_TEX_DIM || port->texture.bounds.height > SAVE_TEX_DIM		/* ... or the port is too big to save, ... */
	) {
		#ifdef LOG_TEXTURE_LIFE
			if (port->texture.bounds.width > SAVE_TEX_DIM || port->texture.bounds.height > SAVE_TEX_DIM)
				LOGD("FskGLPortDispose: %ux%u %s textures {#%u,#%u,#%u} too big to save -- disposing",
					(unsigned)port->texture.bounds.width, (unsigned)port->texture.bounds.height,
					GLInternalFormatNameFromCode(port->texture.glIntFormat), port->texture.name, port->texture.nameU, port->texture.nameV);
		#endif /* LOG_TEXTURE_LIFE */
		return GLPortReallyDispose(port);															/* ... get rid of the port immediately */
	}

	err = GLPortListDeletePort(&(gGLGlobalAssets.activePorts), port);								/* Delete the port from the active port list */
	if (err) {																						/* If it wasn't found -- get rid of the cursed port immediately */
		#if !GL_DEBUG
			LOGE("FskGLPortDispose: %ux%u %d texture #%u NOT FOUND IN ACTIVE PORT LIST -- yikes!",
				(unsigned)port->texture.bounds.width, (unsigned)port->texture.bounds.height, port->texture.glIntFormat, port->texture.name);
		#else /* GL_DEBUG */
			LOGE("FskGLPortDispose: %ux%u %s texture #%u NOT FOUND IN ACTIVE PORT LIST -- yikes!",
				(unsigned)port->texture.bounds.width, (unsigned)port->texture.bounds.height, GLInternalFormatNameFromCode(port->texture.glIntFormat), port->texture.name);
		#endif /* GL_DEBUG */

		#ifdef LOG_TEXTURE_LIFE
		{	FskGLPort p;
			for (p = port; p != NULL; p = p->next) {
				LOGE("\tportChain %ux%u %s texture {#%u,#%u,#%u} ",
					(unsigned)p->texture.bounds.width, (unsigned)p->texture.bounds.height, GLInternalFormatNameFromCode(p->texture.glIntFormat),
					p->texture.name, p->texture.nameU, p->texture.nameV);
			}
		}
		#endif /* LOG_TEXTURE_LIFE */
		(void)GLPortReallyDispose(port);
		return err;
	}

	ForgetGivenTexturesInAllContexts(&port->texture);												/* Obliterate memory of this texture */
	if (port == gCurrentGLPort)																		/* If this port was the current port ... */
		gCurrentGLPort = NULL;																		/* ... nullify the current port. */
	port->portWidth  = 0;																			/* Just in case this port was used to render to a texture, ... */
	port->portHeight = 0;																			/* ... reset it. */

	#ifdef LOG_TEXTURE_LIFE
		LOGD("GLPortDispose saving %ux%u %s textures {#%u,#%u,#%u}", (unsigned)port->texture.bounds.width, (unsigned)port->texture.bounds.height,
			GLInternalFormatNameFromCode(port->texture.glIntFormat), port->texture.name, port->texture.nameU, port->texture.nameV);
	#endif /* LOG_TEXTURE_LIFE */
	port->texture.srcBM = NULL;															/* Disassociate it from its bitmap */
	GLPortListInsertPort(&gGLGlobalAssets.freePorts, port);								/* Link it into the freePort linked list */

	/* Count free ports and dispose the lot if it gets too big */
	for (port = gGLGlobalAssets.freePorts, freePortCount = 0; port != NULL; port = port->next) {
		if (++freePortCount > MAX_PORT_POOL_SIZE) {
			while (NULL != (port = gGLGlobalAssets.freePorts)) {
				gGLGlobalAssets.freePorts = gGLGlobalAssets.freePorts->next;
				GLPortReallyDispose(port);
			}
			break;
		}
	}

	return kFskErrNone;
#endif /* USE_PORT_POOL */
}


/********************************************************************************
 * FskGLPortDisposeAt
 ********************************************************************************/

#undef FskGLPortDisposeAt
FskErr FskGLPortDisposeAt(FskGLPort *port) {
	FskErr err;
	if (port) {
		err = FskGLPortDispose(*port);
		*port = NULL;
	}
	else {
		err = kFskErrInvalidParameter;
	}
	return err;
}


/********************************************************************************
 * FskGLPortBitmapSwap
 ********************************************************************************/

#undef    FskGLPortBitmapSwap
FskErr FskGLPortBitmapSwap(FskBitmap fr, FskBitmap to, Boolean update) {
	FskErr		err		= kFskErrNone;
	FskGLPort	fp, tp;
	BAIL_IF_FALSE(fr && to, err, kFskErrInvalidParameter);
	if (NULL != (tp = fr->glPort))
		tp->texture.srcBM = to;
	if (NULL != (fp = to->glPort))
		fp->texture.srcBM = fr;
	fr->glPort = fp;
	to->glPort = tp;
	if (update) {
		FskErr	err2;
		if (fr->glPort)
			err  = FskGLUpdateSource(fr);
		if (to->glPort)
			if (kFskErrNone != (err2 = FskGLUpdateSource(to)))
				err = err2;
	}
bail:
	return err;
}


/********************************************************************************
 * FskGLPortGetCurrent
 ********************************************************************************/

#undef    FskGLPortGetCurrent
FskGLPort FskGLPortGetCurrent(void) {
	return gCurrentGLPort;
}


/********************************************************************************
 * FskGLPortSetCurrent
 ********************************************************************************/

#undef FskGLPortSetCurrent
void   FskGLPortSetCurrent(FskGLPort port) {
	gCurrentGLPort = port;
}


/********************************************************************************
 * FskGLPortSwapCurrent
 ********************************************************************************/

#undef FskGLPortSwapCurrent
void   FskGLPortSwapCurrent(FskGLPort *port) {
	FskGLPort c = gCurrentGLPort;
	gCurrentGLPort = *port;
	*port = c;
}


/********************************************************************************
 * FskGLPortResize
 ********************************************************************************/

#undef FskGLPortResize
void   FskGLPortResize(FskGLPort port, UInt32 width, UInt32 height) {
	FskGLPortRecord tmpPort;

	#if defined(LOG_PARAMETERS) || defined(LOG_VIEW)
		LOGD("GLPortResize(%u, %u)", (unsigned)width, (unsigned)height);
	#endif /* LOG_PARAMETERS */

	if (port) {
		if (port->portWidth == width && port->portHeight == height)	/* Nothing has changed */
			return;													/* Don't call GL unless we need to. */
		port->portWidth  = width;
		port->portHeight = height;
	}
	else {															/* Set GL with a fake port. */
		FskMemSet(&tmpPort, 0, sizeof(tmpPort));	/* @@ don't know what's supposed to do but at least sysContext has to be cleared as FskGLPortFocus is called right after */
		tmpPort.portWidth  = width;
		tmpPort.portHeight = height;
		port = &tmpPort;
	}
	if (GL_GLOBAL_ASSETS_ARE_INITIALIZED()) {
		FskGLPortFocus(port);										/* Ripple through port size changes to GL */
		FskGLPortDefocus(port);
	}
}


/********************************************************************************
 * FskGLPortGetSize
 ********************************************************************************/

#undef FskGLPortGetSize
void   FskGLPortGetSize(FskConstGLPort port, UInt32 *width, UInt32 *height) {
	if (port == NULL) {
		*width  = 0;
		*height = 0;
		return;
	}
	*width  = port->portWidth;
	*height = port->portHeight;
}


/********************************************************************************
 * FskGLPortSetSysContext
 ********************************************************************************/

#undef FskGLPortSetSysContext
void   FskGLPortSetSysContext(FskGLPort port, void *sysContext) {
	if (port)
		port->sysContext = sysContext;
}


/********************************************************************************
 * FskGLPortGetSysContext
 ********************************************************************************/

#undef FskGLPortGetSysContext
void*  FskGLPortGetSysContext(FskConstGLPort port) {
	return (port != NULL) ? port->sysContext : NULL;
}


/********************************************************************************
 * FskGLPortFocus
 ********************************************************************************/

#undef FskGLPortFocus
void   FskGLPortFocus(FskGLPort port) {
	#ifdef LOG_PARAMETERS
		LOGD("GLPortFocus(%p)", port);
	#endif /* LOG_PARAMETERS */
	#if TARGET_OS_ANDROID || TARGET_OS_IPHONE || TARGET_OS_KPL
		if (!GL_GLOBAL_ASSETS_ARE_INITIALIZED()) {	/* Deferred initialization */
			#if GL_DEBUG
				FskErr err =
			#endif /* GL_DEBUG */
			InitGlobalGLAssets();
			#if GL_DEBUG
				if (err)	LOGD("Deferred GL Port initialization fails in FskGLPortFocus with code %d", (int)err);
				else		LOGD("Deferred GL Port initialization succeeds in FskGLPortFocus");
			#endif /* GL_DEBUG */
		}
	#endif /* TARGET_OS_ANDROID */
	if (port && port->portWidth && port->portHeight) {	/* if (FskGLPortIsDestination(port) */
		if (port->sysContext && gGLGlobalAssets.focus)
			(*gGLGlobalAssets.focus)(port->sysContext);
		(void)FskGLSetGLView(port);
	}
	#if CHECK_GL_ERROR
	{	FskErr err = GetFskGLError();
		PRINT_IF_ERROR(err, __LINE__, "FskGLPortFocus");
	}
	#endif /* CHECK_GL_ERROR */
}


/********************************************************************************
 * FskGLPortDefocus
 ********************************************************************************/

#undef FskGLPortDefocus
void   FskGLPortDefocus(FskConstGLPort port) {
	#ifdef LOG_PARAMETERS
		LOGD("GLPortDefocus(%p)", port);
	#endif /* LOG_PARAMETERS */
	if (port && port->sysContext && gGLGlobalAssets.defocus)
		(*gGLGlobalAssets.defocus)(port->sysContext);
	#if CHECK_GL_ERROR
		{	FskErr err = GetFskGLError();
			PRINT_IF_ERROR(err, __LINE__, "GLPortDefocus 2");
		}
	#endif /* CHECK_GL_ERROR */
}

#define DEFAULT_CONTEXT_EXISTS()	(NULL != gGLGlobalAssets.blitContext)


/********************************************************************************
 * SetGLContextOfBlitContext
 ********************************************************************************/

static void SetGLContextOfBlitContext(FskGLContext glContext, FskGLBlitContext blitContext) {
	blitContext->glContext  = glContext;
	blitContext->defaultFBO = FskGLContextFrameBuffer(glContext);

	#if defined(EGL_VERSION)
		FskGLContextGetEGL(glContext,
			(void**)(&blitContext->eglDisplay),
			(void**)(&blitContext->eglSurface),
			(void**)(&blitContext->eglContext)
		);
	#elif TARGET_OS_IPHONE
	#elif TARGET_OS_MAC
	#else
		#error Unknown GL context
	#endif /* TARGET */
}


/********************************************************************************
 * FskGLBlitContextInitDefault
 ********************************************************************************/

void FskGLBlitContextInitDefault(FskGLContext glContext) {
	#if defined(LOG_PARAMETERS) || defined(LOG_CONTEXT)
		LOGD("FskGLBlitContextInitDefault(%p)", glContext);
	#endif /* LOG_PARAMETERS */

	if (!gGLGlobalAssets.blitContext)
		gGLGlobalAssets.blitContext = &gGLGlobalAssets.defaultContext;
	SetGLContextOfBlitContext(glContext, &gGLGlobalAssets.defaultContext);
}


/********************************************************************************
 * FskGLBlitContextNew
 ********************************************************************************/

FskErr FskGLBlitContextNew(FskGLContext glContext, FskGLBlitContext *pBlitContext) {
	FskErr err = kFskErrNone;

	#if defined(LOG_PARAMETERS) || defined(LOG_CONTEXT)
		LOGD("FskGLBlitContextNew(glContext=%p, pBlitContext=%p)", glContext, pBlitContext);
	#endif /* LOG_PARAMETERS */

	BAIL_IF_NULL(pBlitContext, err, kFskErrInvalidParameter);
	*pBlitContext = NULL;

	if (DEFAULT_CONTEXT_EXISTS()) {														/* We already have established a default context, so this one is secondary */
		FskGLBlitContext ctx;
		BAIL_IF_ERR(err = FskMemPtrNewClear(sizeof(**pBlitContext), pBlitContext));
		ctx = *pBlitContext;
		ctx->next = gGLGlobalAssets.defaultContext.next;								/* Maintain a linked list of contexts, ... */
		gGLGlobalAssets.defaultContext.next = ctx;										/* ... so we can manage their lifetimes. */
		SetGLContextOfBlitContext(glContext, ctx);
		{	FskGLBlitContext saveContext = FskGLBlitContextGetCurrent();
			BAIL_IF_ERR(err = FskGLBlitContextMakeCurrent(ctx));
			InitContextState();
			BAIL_IF_ERR(err = FskGLBlitContextMakeCurrent(saveContext));
		}
	}
	else {																				/* No default context yet: allocate the default */
		SetGLContextOfBlitContext(glContext, (*pBlitContext = gGLGlobalAssets.blitContext = &gGLGlobalAssets.defaultContext));
	}

bail:
	return err;
}


/********************************************************************************
 * FskGLBlitContextNewFromCurrent
 ********************************************************************************/

FskAPI(FskErr)	FskGLBlitContextNewFromCurrent(FskGLBlitContext *pBlitContext) {
	FskGLContext	glContext;
	FskErr			err;

	#if defined(LOG_PARAMETERS) || defined(LOG_CONTEXT)
		LOGD("FskGLBlitContextNewFromCurrent(pBlitContext=%p)", pBlitContext);
	#endif /* LOG_PARAMETERS */

	*pBlitContext = NULL;
	BAIL_IF_ERR(err = FskGLContextNewFromCurrentContext(&glContext));
	BAIL_IF_ERR(err = FskGLBlitContextNew(glContext, pBlitContext));
	(**pBlitContext).glContextMine = true;

bail:
	return err;
}


/********************************************************************************
 * FskGLBlitOffscreenContextNew
 ********************************************************************************/

FskErr FskGLBlitOffscreenContextNew(UInt32 width, UInt32 height, FskBitmapFormatEnum pixelFormat, UInt32 version, FskGLBlitContext share, FskGLBlitContext *pBlitContext) {
	FskErr			err;
	FskGLContext	glContext;

	#if defined(LOG_PARAMETERS) || defined(LOG_CONTEXT)
		LOGD("FskGLBlitOffscreenContextNew(width=%u height=%u pixelFormat=%s version=%u share=%p pBlitContext=%p)",
				width, height, FskBitmapFormatName(pixelFormat), version, share, pBlitContext);
	#endif /* LOG_PARAMETERS */

	*pBlitContext = NULL;
	BAIL_IF_ERR(err = FskGLOffscreenContextNew(width, height, pixelFormat, version, (share ? share->glContext : NULL), &glContext));
	BAIL_IF_ERR(err = FskGLBlitContextNew(glContext, pBlitContext));
	(**pBlitContext).glContextMine = true;

bail:
	return err;
}


/********************************************************************************
 * FskGLBlitContextDispose
 *	TODO: Do we need to specify terminateGL?
 ********************************************************************************/

FskErr FskGLBlitContextDispose(FskGLBlitContext blitContext) {
	FskGLBlitContext	curCtx;
	FskErr				err;

	#if defined(LOG_PARAMETERS) || defined(LOG_CONTEXT)
		LOGD("FskGLBlitContextDispose(blitContext=%p)", blitContext);
	#endif /* LOG_PARAMETERS */

	BAIL_IF_NULL(blitContext, err, kFskErrNone);
	BAIL_IF_FALSE(blitContext != &gGLGlobalAssets.defaultContext, err, kFskErrGLStackUnderflow);	/* We cannot dispose of the default context */
	if ((curCtx = gGLGlobalAssets.blitContext) == blitContext)										/* If this is the current context, ... */
		curCtx = &gGLGlobalAssets.defaultContext;													/* ... restore later to the default context */

	FskGLBlitContextMakeCurrent(blitContext);														/* Set to a this context */
	if (blitContext->glContextMine)																	/* If we created the GL context, ... */
		FskGLContextDispose(blitContext->glContext, false);											/* ... we dispose of it. Current context is unknown. */
	err = RemoveGLBlitContext(blitContext);															/* Remove this blit context from our linked list */
	#if defined(LOG_CONTEXT)
		if (err)
			LOGD("FskGLBlitContextDispose::RemoveGLBlitContext() returns err=%s", FskInstrumentationGetErrorString(err));
	#endif
	FskMemPtrDispose(blitContext);																	/* Dispose of our memory allocation */
	FskGLBlitContextMakeCurrent(curCtx);															/* Set to a valid context */

bail:
	return err;
}


/********************************************************************************
 * FskGLBlitContextMakeCurrent
 ********************************************************************************/

FskErr FskGLBlitContextMakeCurrent(FskGLBlitContext blitContext) {
	FskErr err = kFskErrNone;
	FskGLBlitContext ocx;

	if ((ocx = gGLGlobalAssets.blitContext) == blitContext) {										/* If the context won't change, ... */
		#if defined(LOG_PARAMETERS) || defined(LOG_CONTEXT)
			LOGD("FskGLBlitContextMakeCurrent(blitContext=%p), already current", blitContext);
		#endif /* LOG_PARAMETERS */
		return kFskErrNone;																			/* ... we are done */
	}

	#if defined(LOG_PARAMETERS) || defined(LOG_CONTEXT)
		LOGD("FskGLBlitContextMakeCurrent(blitContext=%p[glContext=%p]), was %p", blitContext, blitContext->glContext, ocx);
	#endif /* LOG_PARAMETERS */

	#if GLES_VERSION == 2
		if (ocx)																					/* If there was an old context, ... */
			err = FskGLBitmapTextureTargetSet(NULL);												/* ... render to the screen in the old context to free any bound textures */
		#if CHECK_GL_ERROR || defined(LOG_CONTEXT)
			PRINT_IF_ERROR(err, __LINE__, "FskGLBlitContextMakeCurrent::FskGLBitmapTextureTargetSet");
		#endif /* CHECK_GL_ERROR */
	#endif /* GLES_VERSION == 2 */
	gGLGlobalAssets.blitContext = blitContext;														/* Remember that we changed the context */
	err = FskGLContextMakeCurrent(blitContext->glContext);											/* Actually change the GL context */
	#if defined(LOG_CONTEXT)
		PRINT_IF_ERROR(err, __LINE__, "FskGLBlitContextMakeCurrent::FskGLContextMakeCurrent");
	#endif /* LOG_CONTEXT */
	#if GLES_VERSION == 2
		(void)FskGLUseProgram(0);																	/* Force to set the program next time */
		gGLGlobalAssets.blitContext->fill.lastParams.level			= 0;							/* Force to change the opColor next time -- TODO: This shouldn't be necessary */
		gGLGlobalAssets.blitContext->drawBitmap.lastParams.level	= 0;
		gGLGlobalAssets.blitContext->transferAlpha.lastParams.level	= 0;
		gGLGlobalAssets.blitContext->drawBCBitmap.lastParams.level	= 0;
		gGLGlobalAssets.blitContext->tileBitmap.lastParams.level	= 0;
		gGLGlobalAssets.blitContext->perspective.lastParams.level	= 0;
		gGLGlobalAssets.blitContext->yuv420.lastParams.level		= 0;
		gGLGlobalAssets.blitContext->yuv420sp.lastParams.level		= 0;
		gGLGlobalAssets.blitContext->yuv422.lastParams.level		= 0;
		gGLGlobalAssets.blitContext->yuv444.lastParams.level		= 0;
		PerturbFloat(&gGLGlobalAssets.matrix[0][0]);												/* Force the view to be recomputed, and cause FskGLSetGLView() to clear to opaque black */
	#endif /* GLES_VERSION == 2 */
	return err;
}


/********************************************************************************
 * FskGLBlitContextGetCurrent
 ********************************************************************************/

FskGLBlitContext FskGLBlitContextGetCurrent(void) {
	if (!gGLGlobalAssets.blitContext)
		InitGlobalGLAssets();

	#if defined(LOG_PARAMETERS) || defined(LOG_CONTEXT)
		LOGD("FskGLBlitContextGetCurrent() returns %p[glContext=%p]", gGLGlobalAssets.blitContext, gGLGlobalAssets.blitContext->glContext);
	#endif /* LOG_PARAMETERS */

	return gGLGlobalAssets.blitContext;
}


/********************************************************************************
 * FskGLBlitContextSwapCurrent
 ********************************************************************************/

FskErr FskGLBlitContextSwapCurrent(FskGLBlitContext *pBlitContext) {
	FskGLBlitContext	ctx	= FskGLBlitContextGetCurrent();
	FskErr				err	= FskGLBlitContextMakeCurrent(*pBlitContext);

	#if defined(LOG_PARAMETERS) || defined(LOG_CONTEXT)
		LOGD("FskGLBlitContextSwapCurrent(pBlitContext=%p) old=%p new=%p", pBlitContext, ctx, *pBlitContext);
	#endif /* LOG_PARAMETERS */

	*pBlitContext = ctx;
	return err;
}


/********************************************************************************
 * FskGLPortSourceTexture
 ********************************************************************************/

#undef   FskGLPortSourceTexture
unsigned FskGLPortSourceTexture(FskConstGLPort port) {
	return port ? port->texture.name : 0;
}


/********************************************************************************
 * FskGLPortIsDestination
 ********************************************************************************/

#undef  FskGLPortIsDestination
Boolean FskGLPortIsDestination(FskConstGLPort port) {
	return (port && port->portWidth && port->portHeight) ? 1 : 0;
}


/********************************************************************************
 * FskGLPortSetPersistent
 ********************************************************************************/

#undef  FskGLPortSetPersistent
FskErr FskGLPortSetPersistent(FskGLPort port, Boolean value) {
	FskErr err = kFskErrNone;

	#if defined(LOG_PARAMETERS)
		LOGD("FskGLPortSetPersistent(port=%p value=%d)", port, value);
	#endif /* LOG_PARAMETERS */

	if (!port) goto bail;
	port->texture.persistent = value;
	if (port->texture.persistent) {													/* If persistent, check to see that we have a second texture */
		if (!port->texture.nameU) {
			BAIL_IF_ERR(err = FskGenGLTexturesAndInit(1, &port->texture.nameU));	/* Allocate a new texture */
			CHANGE_TEXTURE(port->texture.nameU);									/* Make it current */
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, port->portWidth, port->portHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);	/* Reshape it accordingly */
		}
	}
	else {																			/* If not persistent, we only need one texture */
		if (!port->texture.nameU) {
			FskDeleteGLTextures(1, &port->texture.nameU);
			port->texture.nameU = 0;
		}
	}
bail:
	return err;
}


/********************************************************************************
 * FskGLPortIsPersistent
 ********************************************************************************/

#undef  FskGLPortIsPersistent
Boolean FskGLPortIsPersistent(FskConstGLPort port) {
	return port ? port->texture.persistent : true;
}


/********************************************************************************
 * FskGLPortSetRotation
 ********************************************************************************/

FskErr FskGLPortSetRotation(FskGLPort port, UInt32 rotation) {
	FskErr	err = kFskErrNone;

	#if defined(LOG_PARAMETERS)
		LOGD("FskGLPortSetRotation(port=%p rotation=%u)", port, (unsigned)rotation);
	#endif /* LOG_PARAMETERS */

	BAIL_IF_FALSE(port && (rotation == 0 || rotation == 180), err, kFskErrInvalidParameter);
	port->texture.viewRotation = (UInt8)rotation;
	err = FskGLSetGLView(port);

bail:
	return err;
}


/********************************************************************************
 * FskGLPortGetRotation
 ********************************************************************************/

UInt32 FskGLPortGetRotation(FskConstGLPort port) {
	return port ? port->texture.viewRotation : 0;
}


/********************************************************************************
 * FskGLSetGLView
 ********************************************************************************/

#undef FskGLSetGLView
FskErr FskGLSetGLView(FskGLPort port) {
	if (!port || !port->portWidth || !port->portHeight) {
		#if GL_DEBUG
			if (port)	LOGD("[%s] GLSetGLView(%p; %d, %d): BAD VIEW: returning early", FskThreadName(FskThreadGetCurrent()), port, (int)port->portWidth, (int)port->portHeight);
			else		LOGD("[%s] GLSetGLView(port==NULL): BAD VIEW: returning early", FskThreadName(FskThreadGetCurrent()));
		#endif /* GL_DEBUG */
		return kFskErrInvalidParameter;
	}
	#if defined(LOG_PARAMETERS) || defined(LOG_VIEW)
		LOGD("[%s] GLSetGLView(%p; %d,%d)", FskThreadName(FskThreadGetCurrent()), port, (int)port->portWidth, (int)port->portHeight);
	#endif /* LOG_PARAMETERS */

	#ifdef LOG_VIEW
	{	GLint viewPort[4];
		glGetIntegerv(GL_VIEWPORT, viewPort);
		if (viewPort[0] != 0				||
			viewPort[1] != 0				||
			viewPort[2] != (GLint)port->portWidth	||
			viewPort[3] != (GLint)port->portHeight
		)
			LOGD("GLSetGLView(%p) viewPort changed (%d,%d,%d,%d) --> (0,0,%d,%d)", port, (int)viewPort[0], (int)viewPort[1], (int)viewPort[2], (int)viewPort[3], (int)port->portWidth, (int)port->portHeight);
	}
	#endif /* LOG_VIEW */

	#if GLES_VERSION < 2
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		#if GL_COORD_TYPE_ENUM != GL_FIXED && !TARGET_OS_ANDROID && !TARGET_OS_IPHONE
			glOrtho(0, port->portWidth, port->portHeight, 0, -10, 10);
		#elif GL_COORD_TYPE_ENUM != GL_FIXED && (TARGET_OS_ANDROID || TARGET_OS_IPHONE)
			glOrthof(0, port->portWidth, port->portHeight, 0, -10, 10);
		#else /* GL_COORD_TYPE_ENUM == GL_FIXED */
			glOrthox(0<<16, port->portWidth<<16, port->portHeight<<16, 0<<16, -10<<16, 10<<16);
		#endif /* GL_COORD_TYPE_ENUM == GL_FIXED */

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glViewport(0, 0, port->portWidth, port->portHeight);
	#else /* GLES_VERSION == 2 */
	{	float	wi = (float)(2.f / port->portWidth),
				hi = (float)(2.f / port->portHeight);
		float	x0, y0;
		if (!gGLGlobalAssets.blitContext->fboTexture)
			hi = -hi;
		if (port->texture.viewRotation) {
			wi = -wi;
			hi = -hi;
		}
		x0 = (wi < 0.f) ? +1.f : -1.f;
		y0 = (hi < 0.f) ? +1.f : -1.f;
		#ifdef LOG_VIEW
			if (!(	gGLGlobalAssets.matrix[0][0] == wi		&&
					gGLGlobalAssets.matrix[1][1] == hi)
			)
				LOGD("GLSetGLView(%p) matrix was { [%.4g %.4g %.4g] [%.4g %.4g %.4g] [%.4g %.4g %.4g] }, new { [%.4g 0 0] [0 %.4g 0] [%g %g 1] }",
					port,
					gGLGlobalAssets.matrix[0][0], gGLGlobalAssets.matrix[0][1], gGLGlobalAssets.matrix[0][2],
					gGLGlobalAssets.matrix[1][0], gGLGlobalAssets.matrix[1][1], gGLGlobalAssets.matrix[1][2],
					gGLGlobalAssets.matrix[2][0], gGLGlobalAssets.matrix[2][1], gGLGlobalAssets.matrix[2][2],
					wi, hi, x0, y0
				);
			#if 0
				else
					LOGD("GLSetGLView(%p) matrix set but unchanged { [%.4g 0 0] [0 %.4g 0] [%g %g  1] }", port, wi, hi, x0, y0);
			#endif /* LOG_VIEW */
		#endif /* LOG_VIEW */
		if (!(	gGLGlobalAssets.matrix[0][0] == wi		&&
				gGLGlobalAssets.matrix[1][1] == hi)
		) {
			glViewport(0, 0, port->portWidth, port->portHeight);
			gGLGlobalAssets.matrixSeed++;
			CHANGE_SCISSOR_ENABLE(false);                           /* Remove any previous clipping */
#if 1
			// client is responsible for covering all pixels, no need to help them here.
			port->texture.fboInited = true;
#else
			if (port->texture.name && !port->texture.fboInited) {
				#if defined(LOG_RENDER_TO_TEXTURE) || defined(LOG_VIEW)
					LOGD("GLSetGLView clearing FBO texture #%d", port->texture.name);
				#endif /* LOG_RENDER_TO_TEXTURE || LOG_VIEW */
				glClearColor(0., 0., 0., 1.);						/* Initialize it to ... */
				glClear(GL_COLOR_BUFFER_BIT);						/* ...  opaque black */
				port->texture.fboInited = true;
			}
#endif
		}
		else if (!OPTIMIZE_STATE_CHANGES) {							/* If not optimizing state changes, ... */
			glViewport(0, 0, port->portWidth, port->portHeight);	/* ... set the viewport ... */
			gGLGlobalAssets.matrixSeed++;							/* ... and the matrix every time */
		}
		gGLGlobalAssets.matrix[0][0] = wi;	gGLGlobalAssets.matrix[0][1] = 0.f;	gGLGlobalAssets.matrix[0][2] = 0.f;
		gGLGlobalAssets.matrix[1][0] = 0.f;	gGLGlobalAssets.matrix[1][1] = hi;	gGLGlobalAssets.matrix[1][2] = 0.f;
		gGLGlobalAssets.matrix[2][0] = x0;	gGLGlobalAssets.matrix[2][1] = y0;	gGLGlobalAssets.matrix[2][2] = 1.f;
	}
	#endif /* GLES_VERSION == 2 */

	#if CHECK_GL_ERROR
		return GetFskGLError();
	#else /* !CHECK_GL_ERROR */
		return kFskErrNone;
	#endif /* !CHECK_GL_ERROR */
}


/********************************************************************************
 * FskErrorFromGLError
 ********************************************************************************/

#undef FskErrorFromGLError
FskErr FskErrorFromGLError(GLenum glErr) {
	struct FskGLErrorEntry { GLenum gl;	FskErr fsk; };
	static const struct FskGLErrorEntry errTab[] = {
		{		GL_INVALID_ENUM,									kFskErrGLInvalidEnum							},
		{		GL_INVALID_VALUE,									kFskErrGLInvalidValue							},
		{		GL_INVALID_OPERATION,								kFskErrGLInvalidOperation						},
		{		GL_OUT_OF_MEMORY,									kFskErrGLOutOfMemory							},
	#ifdef		GL_STACK_OVERFLOW
		{		GL_STACK_OVERFLOW,									kFskErrGLStackOverflow							},
	#endif /*	GL_STACK_OVERFLOW */
	#ifdef		GL_STACK_UNDERFLOW
		{		GL_STACK_UNDERFLOW,									kFskErrGLStackUnderflow							},
	#endif /*	GL_STACK_UNDERFLOW */
	#ifdef		GL_TABLE_TOO_LARGE
		{		GL_TABLE_TOO_LARGE,									kFskErrGLTableTooLarge							},
	#endif /*	GL_TABLE_TOO_LARGE */
	#ifdef		GL_INVALID_FRAMEBUFFER_OPERATION
		{		GL_INVALID_FRAMEBUFFER_OPERATION,					kFskErrGLInvalidFramebufferOperation			},
	#endif /*	GL_INVALID_FRAMEBUFFER_OPERATION */
	#ifdef		GL_INVALID_FRAMEBUFFER_OPERATION_EXT
		{		GL_INVALID_FRAMEBUFFER_OPERATION_EXT,				kFskErrGLInvalidFramebufferOperation			},
	#endif /*	GL_INVALID_FRAMEBUFFER_OPERATION_EXT */
	#ifdef		GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT
		{		GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT,				kFskErrGLFramebufferIncompleteAttachment		},
	#endif /*	GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT */
	#ifdef		GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT
		{		GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT,			kFskErrGLFramebufferIncompleteAttachment		},
	#endif /*	GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT */
	#ifdef		GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT
		{		GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT,		kFskErrGLFramebufferIncompleteMissingAttachment	},
	#endif /*	GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT */
	#ifdef		GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT
		{		GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT,	kFskErrGLFramebufferIncompleteMissingAttachment	},
	#endif /*	GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT */
	#ifdef		GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS
		{		GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS,				kFskErrGLFramebufferIncompleteDimensions		},
	#endif /*	GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS */
	#ifdef		GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT
		{		GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT,			kFskErrGLFramebufferIncompleteDimensions		},
	#endif /*	GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT */
	#ifdef		GL_FRAMEBUFFER_INCOMPLETE_FORMATS
		{		GL_FRAMEBUFFER_INCOMPLETE_FORMATS,					kFskErrGLFramebufferIncompleteFormats			},
	#endif /*	GL_FRAMEBUFFER_INCOMPLETE_FORMATS */
	#ifdef		GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT
		{		GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT,				kFskErrGLFramebufferIncompleteFormats			},
	#endif /*	GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT */
	#ifdef		GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER
		{		GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER,				kFskErrGLFramebufferIncompleteDrawBuffer		},
	#endif /*	GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER */
	#ifdef		GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT
		{		GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT,			kFskErrGLFramebufferIncompleteDrawBuffer		},
	#endif /*	GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT */
	#ifdef		GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER
		{		GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER,				kFskErrGLFramebufferIncompleteReadBuffer		},
	#endif /*	GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER */
	#ifdef		GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT
		{		GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT,			kFskErrGLFramebufferIncompleteReadBuffer		},
	#endif /*	GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT */
	#ifdef		GL_FRAMEBUFFER_UNSUPPORTED
		{		GL_FRAMEBUFFER_UNSUPPORTED,							kFskErrGLFramebufferUnsupported					},
	#endif /*	GL_FRAMEBUFFER_UNSUPPORTED */
	#ifdef		GL_FRAMEBUFFER_UNSUPPORTED_EXT
		{		GL_FRAMEBUFFER_UNSUPPORTED_EXT,						kFskErrGLFramebufferUnsupported					},
	#endif /*	GL_FRAMEBUFFER_UNSUPPORTED_EXT */
	#ifdef		GL_FRAMEBUFFER_UNDEFINED
		{		GL_FRAMEBUFFER_UNDEFINED,							kFskErrGLFramebufferUndefined					},
	#endif /*	GL_FRAMEBUFFER_UNDEFINED */
	#ifdef		GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS
		{		GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS,			kFskErrGLFramebufferIncompleteLayerTargets		},
	#endif /*	GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS */
	#ifdef		GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE
		{		GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE,				kFskErrGLFramebufferIncompleteMultisample		},
	#endif /*	GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE */
	#ifdef		GL_FRAMEBUFFER_COMPLETE
		{		GL_FRAMEBUFFER_COMPLETE,							kFskErrGLFramebufferComplete,					},
	#endif /*	GL_FRAMEBUFFER_COMPLETE */
		{		GL_NO_ERROR,										kFskErrNone										}
	};
	FskErr err = glErr;
	const struct FskGLErrorEntry *p;
	for (p = errTab; p->gl != GL_NO_ERROR; ++p) {
		if (p->gl == glErr) {
			err = p->fsk;
			break;
		}
	}
	return err;
}


/********************************************************************************
 * FskGLGetFormatAndTypeFromPixelFormat
 * TODO: Make this table-driven, and perhaps suggest an alternate supported format.
 ********************************************************************************/

#undef FskGLGetFormatAndTypeFromPixelFormat
FskErr FskGLGetFormatAndTypeFromPixelFormat(FskBitmapFormatEnum format, unsigned *pglFormat, unsigned *pglType, int *pglIntFmt) {
	FskErr				err			= kFskErrNone;
	GLFormat			glFormat	= -1;
	GLType				glType		= -1;
	GLInternalFormat	glIntFmt	= -1;

	switch (format) {
	/* Every implementation can do alpha, luminance, and rgb textures in a standard way. */
		case kFskBitmapFormat24RGB:			glFormat = GL_RGB;				glType = GL_UNSIGNED_BYTE;					glIntFmt = GL_RGB;			break;
		case kFskBitmapFormat8A:			glFormat = GL_ALPHA;			glType = GL_UNSIGNED_BYTE;					glIntFmt = GL_ALPHA;		break;
		case kFskBitmapFormat8G:			glFormat = GL_LUMINANCE;		glType = GL_UNSIGNED_BYTE;					glIntFmt = GL_LUMINANCE;	break;

	#if TARGET_OS_WIN32	/* This is just a guess */
		case kFskBitmapFormat32ABGR:		glFormat = GL_RGBA;				glType = GL_UNSIGNED_BYTE;					glIntFmt = GL_RGBA;			break;
		#ifdef GL_BGRA
		case kFskBitmapFormat32BGRA:		glFormat = GL_BGRA;				glType = GL_UNSIGNED_BYTE;					glIntFmt = GL_BGRA;	if (!FskGLHardwareSupportsPixelFormat(kFskBitmapFormat32BGRA)) err = kFskErrUnsupportedPixelType; break;
		#endif /* GL_BGRA */
		case kFskBitmapFormat32RGBA:		glFormat = GL_RGBA;				glType = GL_UNSIGNED_BYTE;					glIntFmt = GL_RGBA;			break;

	#elif TARGET_OS_MAC && !TARGET_OS_IPHONE
		case kFskBitmapFormat16RGB565BE:	glFormat = GL_RGB;				glType = GL_UNSIGNED_SHORT_5_6_5_REV;		glIntFmt = GL_RGB;			break;	/* This has implications of an alignment of 2 */
		case kFskBitmapFormat16RGB565LE:	glFormat = GL_RGB;				glType = GL_UNSIGNED_SHORT_5_6_5;			glIntFmt = GL_RGB;			break;	/* This has implications of an alignment of 2 */
		case kFskBitmapFormat16RGBA4444LE:	glFormat = GL_RGBA;				glType = GL_UNSIGNED_SHORT_4_4_4_4;			glIntFmt = GL_RGBA;			break;	/* This has implications of an alignment of 2 */
		case kFskBitmapFormat24BGR:			glFormat = GL_BGR;				glType = GL_UNSIGNED_BYTE;					glIntFmt = GL_RGB;			break;
		case kFskBitmapFormat32ABGR:		glFormat = GL_RGBA;				glType = GL_UNSIGNED_INT_8_8_8_8;			glIntFmt = GL_RGBA;			break;	/* This has implications of an alignment of 4 */
		case kFskBitmapFormat32ARGB:		glFormat = GL_BGRA;				glType = GL_UNSIGNED_INT_8_8_8_8;			glIntFmt = GL_RGBA;			break;	/* This has implications of an alignment of 4 */
		case kFskBitmapFormat32BGRA:		glFormat = GL_BGRA;				glType = GL_UNSIGNED_INT_8_8_8_8_REV;		glIntFmt = GL_RGBA;			break;	/* This has implications of an alignment of 4 */
		case kFskBitmapFormat32RGBA:		glFormat = GL_RGBA;				glType = GL_UNSIGNED_INT_8_8_8_8_REV;		glIntFmt = GL_RGBA;			break;	/* This has implications of an alignment of 4 */
		case kFskBitmapFormatUYVY:		if (FskGLHardwareSupportsPixelFormat(kFskBitmapFormatUYVY)) {
											glFormat = GL_YCBCR_422_APPLE;	glType = GL_UNSIGNED_SHORT_8_8_APPLE;		glIntFmt = GL_RGBA;			break;
										}

										#if GL_RGB_422_APPLE && GLES_VERSION == 2
										if (gGLGlobalAssets.hasAppleRGB422) {
										#if   TARGET_RT_LITTLE_ENDIAN
											glFormat = GL_RGB_422_APPLE;	glType = GL_UNSIGNED_SHORT_8_8_APPLE;		glIntFmt = GL_RGBA;			break;
										#elif TARGET_RT_BIG_ENDIAN
											glFormat = GL_RGB_422_APPLE;	glType = GL_UNSIGNED_SHORT_8_8_REV_APPLE;	glIntFmt = GL_RGBA;			break;
										#endif /* TARGET_RT_ENDIAN */
										}
										#endif /* GL_RGB_422_APPLE */
										else {								err = kFskErrUnsupportedPixelType;										break;	}
	#elif TARGET_OS_ANDROID || TARGET_OS_IPHONE || TARGET_OS_LINUX || TARGET_OS_KPL /* This is probably valid for all little-endian GL ES machines */
		case kFskBitmapFormat16RGB565LE:	glFormat = GL_RGB;				glType = GL_UNSIGNED_SHORT_5_6_5;			glIntFmt = GL_RGB;			break;	/* This has implications of an alignment of 2 */
		case kFskBitmapFormat16RGBA4444LE:	glFormat = GL_RGBA;				glType = GL_UNSIGNED_BYTE;					glIntFmt = GL_RGBA;			break;
		case kFskBitmapFormat32ABGR:		glFormat = GL_RGBA;				glType = GL_UNSIGNED_BYTE;					glIntFmt = GL_RGBA;			break;
		#ifdef GL_BGRA
		case kFskBitmapFormat32BGRA:		glFormat = GL_BGRA;				glType = GL_UNSIGNED_BYTE;					glIntFmt = GL_BGRA;	if (!FskGLHardwareSupportsPixelFormat(kFskBitmapFormat32BGRA)) err = kFskErrUnsupportedPixelType; break;
		#endif /* GL_BGRA */
		case kFskBitmapFormat32RGBA:		glFormat = GL_RGBA;				glType = GL_UNSIGNED_BYTE;					glIntFmt = GL_RGBA;			break;
		case kFskBitmapFormatUYVY:
										#ifdef GL_RGB_422_APPLE
										if (gGLGlobalAssets.hasAppleRGB422) {
										#if   TARGET_RT_LITTLE_ENDIAN
											glFormat = GL_RGB_422_APPLE;	glType = GL_UNSIGNED_SHORT_8_8_APPLE;		glIntFmt = GL_RGB_422_APPLE;	break;
										#elif TARGET_RT_BIG_ENDIAN
											glFormat = GL_RGB_422_APPLE;	glType = GL_UNSIGNED_SHORT_8_8_REV_APPLE;	glIntFmt = GL_RGB_422_APPLE;	break;
										#endif /* TARGET_RT_ENDIAN */
										} else
										#endif /* GL_RGB_422_APPLE */
										{									err = kFskErrUnsupportedPixelType;										break;	}

	#elif TARGET_RT_LITTLE_ENDIAN			/* This is just a guess */
		case kFskBitmapFormat16RGB565BE:	glFormat = GL_RGB;				glType = GL_UNSIGNED_SHORT_5_6_5_REV;		glIntFmt = GL_RGB;			break;
		case kFskBitmapFormat16RGB565LE:	glFormat = GL_RGB;				glType = GL_UNSIGNED_SHORT_5_6_5;			glIntFmt = GL_RGB;			break;
		case kFskBitmapFormat16RGBA4444LE:	glFormat = GL_RGBA;				glType = GL_UNSIGNED_SHORT_4_4_4_4;			glIntFmt = GL_RGBA;			break;
		case kFskBitmapFormat32ABGR:		glFormat = GL_RGBA;				glType = GL_UNSIGNED_INT_8_8_8_8;			glIntFmt = GL_RGBA;			break;
		case kFskBitmapFormat32ARGB:		glFormat = GL_BGRA;				glType = GL_UNSIGNED_INT_8_8_8_8;			glIntFmt = GL_RGBA;			break;
		case kFskBitmapFormat32BGRA:		glFormat = GL_BGRA;				glType = GL_UNSIGNED_INT_8_8_8_8_REV;		glIntFmt = GL_RGBA;			break;
		case kFskBitmapFormat32RGBA:		glFormat = GL_RGBA;				glType = GL_UNSIGNED_INT_8_8_8_8_REV;		glIntFmt = GL_RGBA;			break;

	#else /* TARGET_RT_BIG_ENDIAN */		/* This is just a guess */
		case kFskBitmapFormat16RGB565BE:	glFormat = GL_RGB;				glType = GL_UNSIGNED_SHORT_5_6_5;			glIntFmt = GL_RGB;			break;
		case kFskBitmapFormat16RGB565LE:	glFormat = GL_RGB;				glType = GL_UNSIGNED_SHORT_5_6_5_REV;		glIntFmt = GL_RGB;			break;
		case kFskBitmapFormat16RGBA4444LE:	glFormat = GL_RGBA;				glType = GL_UNSIGNED_SHORT_4_4_4_4_REV;		glIntFmt = GL_RGBA;			break;
		case kFskBitmapFormat32ABGR:		glFormat = GL_RGBA;				glType = GL_UNSIGNED_INT_8_8_8_8_REV;		glIntFmt = GL_RGBA;			break;
		case kFskBitmapFormat32ARGB:		glFormat = GL_BGRA;				glType = GL_UNSIGNED_INT_8_8_8_8_REV;		glIntFmt = GL_RGBA;			break;
		case kFskBitmapFormat32BGRA:		glFormat = GL_BGRA;				glType = GL_UNSIGNED_INT_8_8_8_8;			glIntFmt = GL_RGBA;			break;
		case kFskBitmapFormat32RGBA:		glFormat = GL_RGBA;				glType = GL_UNSIGNED_INT_8_8_8_8;			glIntFmt = GL_RGBA;			break;
	#endif /* TARGET_RT_BIG_ENDIAN */

		case kFskBitmapFormatGLRGB:			glFormat = GL_RGB;				glType = GL_UNSIGNED_BYTE;					glIntFmt = GL_RGB;			break;
		case kFskBitmapFormatGLRGBA:		glFormat = GL_RGBA;				glType = GL_UNSIGNED_BYTE;					glIntFmt = GL_RGBA;			break;

		/* case kFskBitmapFormat16AG:			GL supports LUMINANCE_ALPHA, not ALPHA_LUMINANCE */
		/* case kFskBitmapFormat16BGR565LE:		GL supports RGB565, not BGR565 */
		/* case kFskBitmapFormat16RGB5515LE:	GL supports RGB5551, not RGB5515 */
		/* case kFskBitmapFormat32A16RGB565LE:	We handle this elsewhere by converting this over to 32RGBA or 32BGRA */
		/* case kFskBitmapFormatYUV420:			We handle planar YUV 4:2:0 elsewhere as a special case with a shader with three textures */
		/* case kFskBitmapFormatYUV420spuv:		We handle semi-planar YUV 4:2:0 elsewhere as a special case with a shader with two textures */
		/* case kFskBitmapFormatYUV420spvu:		We handle semi-planar YUV 4:2:0 elsewhere as a special case with a shader with two textures */
		/* case kFskBitmapFormatYUV420i:		UVYYYY is difficult to handle even with a shader */
		/* case kFskBitmapFormatYUV422:			This format is not well-defined */
		default:
			if (FskGLHardwareSupportsPixelFormat(format)) switch (format) {
			#if TARGET_OS_WIN32
				case kFskBitmapFormat16RGB565LE:	glFormat = GL_RGB;		glType = GL_UNSIGNED_SHORT_5_6_5;			glIntFmt = GL_RGB;			break;	/* This has implications of an alignment of 2 */
			#endif /* TARGET_OS_WIN32 */
				default:					err = kFskErrUnsupportedPixelType;	break;
			}
			else {
											err = kFskErrUnsupportedPixelType;	break;
			}
	}

	if (pglFormat)	*pglFormat	= glFormat;
	if (pglFormat)	*pglType	= glType;
	if (pglIntFmt)	*pglIntFmt	= glIntFmt;

	#ifdef LOG_SET_TEXTURE
		if (err)
			LOGD("GLGetFormatAndTypeFromPixelFormat: Unsupported pixel type = %s", FskBitmapFormatName(format));
	#endif /* LOG_SET_TEXTURE */

	return err;
}


/********************************************************************************
 * FskGLMaximumTextureDimension
 ********************************************************************************/

#undef FskGLMaximumTextureDimension
UInt32 FskGLMaximumTextureDimension(void) {
	return gGLGlobalAssets.maxTextureSize;
}


#if USE_PORT_POOL
#define CANNOT_SWITCH_SPECIES
/****************************************************************************//**
 *	Texture matching metric.
 *	Note that kTexPerfectMatchScore is the score for a perfect match.
 *	\param[in]	glPort
 *	\param[in]	width
 *	\param[in]	height
 *	\param[in]	ifmt
 *	\return		the texture matching score.
 ********************************************************************************/

static int TexMatchScore(FskGLPort glPort, SInt32 width, SInt32 height, GLInternalFormat ifmt) {
	int score = 0;

	if      (glPort->texture.bounds.width  == width)	score += 2;
	else if (glPort->texture.bounds.width  >  width)	score += 1;
	if      (glPort->texture.bounds.height == height)	score += 2;
	else if (glPort->texture.bounds.height >  height)	score += 1;
	if      (glPort->texture.glIntFormat   == ifmt)		score += 4;
	else {
		#ifndef CANNOT_SWITCH_SPECIES
			switch (ifmt) {
		case GL_BGRA:	if (glPort->texture.glIntFormat == GL_RGBA)	score += 1;	break;
		case GL_RGBA:	if (glPort->texture.glIntFormat == GL_BGRA)	score += 1;	break;
		default:		break;
	}
		#else /* CANNOT_SWITCH_SPECIES */
			score = 0;
		#endif /* CANNOT_SWITCH_SPECIES */
	}
	return score;
}
#define kTexPerfectMatchScore	6


/****************************************************************************//**
 *	Try to find a texture that is compatible with the given bitmap.
 *	Some textures are triple (for YUV420), some double (YUV420spuv and YUV420spvu), and others are single.
 *	\param[in]	bm		the bitmap.
 *	\param[out]	newPort	location to store a new port.
 *	\return				a compatible GL port/texture.
 ********************************************************************************/

static FskErr GetCompatibleFreePortTexture(FskConstBitmap bm, FskGLPort *newPort) {
	FskErr				err				= kFskErrNone;
	FskGLPort			glPort, ssPort;
	GLint				width, height;
	GLInternalFormat	glIntFormat;
	GLFormat			glFormat;
	GLType				glType;
	int					ssScore, k;

	*newPort = NULL;
	if (gGLGlobalAssets.freePorts == NULL) {
		err = kFskErrItemNotFound;
		goto bail;
	}

	width  = bm->rowBytes ? (GLint)(bm->rowBytes / (bm->depth >> 3)) : (GLint)bm->bounds.width;
	height = bm->bounds.height;
	if (gGLGlobalAssets.hasNPOT) {
		width  = BLOCKIFY(width,  TEXTURE_BLOCK_HORIZONTAL);
		height = BLOCKIFY(height, TEXTURE_BLOCK_VERTICAL);
	}
	else {
		width  = SmallestContainingPowerOfTwo(width);
		height = SmallestContainingPowerOfTwo(height);
	}

	/* Look for an exact match (glPort), but also reserve a same-species candidate (ssPort), in case we don't find one */
	#ifdef LOG_TEXTURE_LIFE
	{	unsigned numFreePorts;
		for (glPort = gGLGlobalAssets.freePorts, numFreePorts = 0; glPort != NULL; glPort = glPort->next)
			++numFreePorts;
		FskGLGetFormatAndTypeFromPixelFormat(bm->pixelFormat, &glFormat, &glType, &glIntFormat);
		LOGD("Looking through %u freePorts for a %dx%d %s texture", numFreePorts, width, height, GLInternalFormatNameFromCode(glIntFormat));
		for (glPort = gGLGlobalAssets.freePorts; glPort != NULL; glPort = glPort->next) {
			LOGD("%-9s %4dx%-4d {#%03d, #%03d, #%03d}",
				GLInternalFormatNameFromCode(glPort->texture.glIntFormat),
				(int)glPort->texture.bounds.width, (int)glPort->texture.bounds.height,
				glPort->texture.name, glPort->texture.nameU, glPort->texture.nameV
			);
		}
	}
	#endif /* LOG_TEXTURE_LIFE */
	ssPort	= NULL;
	ssScore = 0;
	switch (bm->pixelFormat) {
		hardwareVideo:
		default:
			if (kFskErrNone != FskGLGetFormatAndTypeFromPixelFormat(bm->pixelFormat, &glFormat, &glType, &glIntFormat))
				glIntFormat = bm->hasAlpha ? GL_RGBA : GL_RGB;									/* This is typically for YUV */
			for (glPort = gGLGlobalAssets.freePorts; glPort != NULL; glPort = glPort->next) {
				if (glPort->texture.nameU == 0) {												/* Not a double or triple texture */
					k = TexMatchScore(glPort, width, height, glIntFormat);
					if (k == kTexPerfectMatchScore) {											/* A matching port */
						#ifdef LOG_TEXTURE_LIFE
							LOGD("GetCompatibleFreePortTexture: Reusing %ux%u %s texture #%u", (unsigned)glPort->texture.bounds.width, (unsigned)glPort->texture.bounds.height,
								GLInternalFormatNameFromCode(glPort->texture.glIntFormat), glPort->texture.name);
						#endif /* LOG_TEXTURE_LIFE */
						break;
					}
					else if (k > ssScore) {
						ssPort = glPort;														/* Set it as the best inexact candidate so far */
						ssScore = k;
					}
				}
			}
			break;

		case kFskBitmapFormatUYVY:
			if (gGLGlobalAssets.texImageUYVY && CanDoHardwareVideo(bm)) goto hardwareVideo;		/* Hardware UYVY only requires a single texture */
			BAIL_IF_ERR(err = FskGLGetFormatAndTypeFromPixelFormat(kFskBitmapFormat32RGBA, &glFormat, &glType, &glIntFormat));
			width  = bm->rowBytes / ((bm->depth >> 3) << 1);									/* Chrominance width */
			if (gGLGlobalAssets.hasNPOT) {
				width  = BLOCKIFY(width,  TEXTURE_BLOCK_HORIZONTAL);
				height = BLOCKIFY(height, TEXTURE_BLOCK_VERTICAL);
			}
			else {
				width  = SmallestContainingPowerOfTwo(width);
				height = SmallestContainingPowerOfTwo(height);
			}
			goto hardwareVideo;																	/* Shader UYVY only requires a single texture, but of half the width */

		case kFskBitmapFormatYUV420spuv:
		case kFskBitmapFormatYUV420spvu:
			glFormat = glIntFormat = GL_ALPHA;
			glType   = GL_UNSIGNED_BYTE;
			for (glPort = gGLGlobalAssets.freePorts; glPort != NULL; glPort = glPort->next) {
				if (glPort->texture.nameU != 0 && glPort->texture.nameV == 0) {					/* A double texture */
					if (glPort->texture.bounds.width  == width		&&
						glPort->texture.bounds.height == height		&&
						glPort->texture.glIntFormat   == glIntFormat
					) {																			/* A matching port */
						#if defined(LOG_TEXTURE_LIFE) || defined(LOG_YUV)
							LOGD("GetCompatibleFreePortTexture: Reusing %ux%u %s texture {%d,%d}", (unsigned)glPort->texture.bounds.width, (unsigned)glPort->texture.bounds.height,
								GLInternalFormatNameFromCode(glPort->texture.glIntFormat), glPort->texture.name, glPort->texture.nameU);
						#endif /* LOG_TEXTURE_LIFE */
						break;
					}
					else if (ssPort == NULL)
						ssPort = glPort;														/* Use the first port as the best inexact port */
				}
			}
			break;

		case kFskBitmapFormatYUV420:
			if (gGLGlobalAssets.texImageYUV420 && CanDoHardwareVideo(bm)) goto hardwareVideo;	/* Hardware YUV420 only requires a single texture */
			glFormat = glIntFormat = GL_ALPHA;
			glType   = GL_UNSIGNED_BYTE;
			for (glPort = gGLGlobalAssets.freePorts; glPort != NULL; glPort = glPort->next) {
				if (glPort->texture.nameU != 0 && glPort->texture.nameV != 0) {					/* A triple texture */
					if (glPort->texture.bounds.width  == width		&&
						glPort->texture.bounds.height == height		&&
						glPort->texture.glIntFormat   == glIntFormat
					) {																			/* A matching port */
						#if defined(LOG_TEXTURE_LIFE) || defined(LOG_YUV)
							LOGD("GetCompatibleFreePortTexture: Reusing %ux%u %s texture {%d,%d,%d}", (unsigned)glPort->texture.bounds.width, (unsigned)glPort->texture.bounds.height,
								GLInternalFormatNameFromCode(glPort->texture.glIntFormat), glPort->texture.name, glPort->texture.nameU, glPort->texture.nameV);
						#endif /* LOG_TEXTURE_LIFE */
						break;
					}
					else if (ssPort == NULL)
						ssPort = glPort;														/* Use the first port as the best inexact port */
				}
			}
			break;
	}

	if (glPort)																					/* We have found an exact match */
		goto gotPort;

	#ifdef CANNOT_SWITCH_SPECIES																/* It may not be possible to reformat existing textures */
		if (NULL == ssPort) {
			err = kFskErrItemNotFound;															/* ... on some devices */
			goto bail;
		}
	#endif /* CANNOT_SWITCH_SPECIES */

	if ((glPort = ssPort) == NULL)																/* No exact match, no same species candidate */
		glPort = gGLGlobalAssets.freePorts;		/* There is at least one texture in the free port list, but it is not of the right species. Try surgery. */
	#ifdef LOG_TEXTURE_LIFE
		else switch (bm->pixelFormat) {
			case kFskBitmapFormatYUV420:
				LOGD("GetCompatibleFreePortTexture: Reusing same species %s texture {#%u,#%u,#%u}",
					GLInternalFormatNameFromCode(glPort->texture.glIntFormat),
					glPort->texture.name, glPort->texture.nameU, glPort->texture.nameV);
				break;
			case kFskBitmapFormatYUV420spuv:
			case kFskBitmapFormatYUV420spvu:
				LOGD("GetCompatibleFreePortTexture: Reusing same species %s texture {#%u,#%u}",
					GLInternalFormatNameFromCode(glPort->texture.glIntFormat),
					glPort->texture.name, glPort->texture.nameU);
				break;
			default:
				LOGD("GetCompatibleFreePortTexture: Reusing same species %s texture #%u",
					GLInternalFormatNameFromCode(glPort->texture.glIntFormat),
					glPort->texture.name);
					break;
		}
	#endif /* LOG_TEXTURE_LIFE */

	glPort->texture.bounds.width  = 0;															/* This forces full initialization */
	glPort->texture.bounds.height = 0;
	glPort->texture.glIntFormat   = 0;
	glPort->texture.srcBM         = NULL;
	err = ReshapeTexture(bm, &glPort->texture);													/* glPort is guaranteed to be nonzero at this point */
	if (kFskErrNone != err && kFskErrTextureTooLarge != err)									/* Too large texture is not a serious error, ... */
		goto bail;																				/* ... but other are */

gotPort:
	glPort->texture.bounds.x	= 0;															/* Assure that the bounds start out appropriately initialized */
	glPort->texture.bounds.y	= 0;
	glPort->texture.filter		= 0;
	glPort->texture.wrapMode	= 0;
	glPort->texture.fboInited	= false;
	GLPortListDeletePort(&gGLGlobalAssets.freePorts,   glPort);									/* Remove this port from the free port list ... */
	GLPortListInsertPort(&gGLGlobalAssets.activePorts, glPort);									/* ... and insert into the active port list */
	*newPort = glPort;																			/* Return the new port only if successful; otherwise NULL */

bail:
	return err;
}

#endif /* USE_PORT_POOL */


/****************************************************************************//**
 *	Try to find a texture that is compatible with the given bitmap.
 *	\param[in]	bm	the bitmap.
 *	\param[out]	newPort	location to store a new port.
 *	\return				a compatible GL port/texture.
 ********************************************************************************/

static FskErr GetCompatiblePortTexture(FskConstBitmap bm, FskGLPort *newPort) {
	FskErr		err		= kFskErrNone;

	#if USE_PORT_POOL
		err = GetCompatibleFreePortTexture(bm, newPort);
		if (kFskErrNone != err && kFskErrTextureTooLarge != err)								/* If we don't find a port texture in the pool ... */
	#endif /* USE_PORT_POOL */
	{	FskGLPort glPort;
		*newPort = NULL;
		BAIL_IF_ERR(err = FskGLPortNew(0, 0, NULL, &glPort));									/* ... allocate a new port ... */
		BAIL_IF_ERR(err = FskGenGLTexturesAndInit(1, &glPort->texture.name));					/* ... with one texture */
		err = ReshapeTexture(bm, &glPort->texture);												/* This will add new textures if needed */
		#ifdef LOG_TEXTURE_LIFE
			LOGD("GetCompatiblePortTexture: ReshapeTexture allocated texture %-9s %4dx%-4d {#%u, #%u, #%u}, err=%d",
				GLInternalFormatNameFromCode(glPort->texture.glIntFormat),
				(int)glPort->texture.bounds.width, (int)glPort->texture.bounds.height,
				glPort->texture.name, glPort->texture.nameU, glPort->texture.nameV, (int)err
			);
		#endif /* LOG_TEXTURE_LIFE */
		if (kFskErrNone != err && kFskErrTextureTooLarge != err)								/* Too large texture is not a serious error, ... */
			goto bail;																			/* ... but other are */
		*newPort = glPort;																		/* Return the new port only if successful; otherwise NULL */
	}
	if ((**newPort).texture.fboInited) {
		LOGE("Texture fboInited, but shouldn't be");
	}
bail:
	PRINT_IF_ERROR(err, __LINE__, "GetCompatiblePortTexture");
	return err;
}


/********************************************************************************
 * FskGLSetBitmapSourceIsAccelerated
 ********************************************************************************/

#undef FskGLSetBitmapSourceIsAccelerated
FskErr FskGLSetBitmapSourceIsAccelerated(FskBitmap bm, Boolean isAccelerated) {
	FskErr		err		= kFskErrNone;

	#if defined(LOG_PARAMETERS) || defined(LOG_TEXTURE_LIFE)
		LOGD("FskGLSetBitmapSourceIsAccelerated(bm=%p, isAccelerated=%d)", bm, isAccelerated);
		LogSrcBitmap(bm, "bm");
	#endif /* LOG_PARAMETERS */

	if (bm->glPort != NULL && !isAccelerated) {													/* If it is accelerated and not desired to be accelerated */
		err = FskGLDeaccelerateBitmapSource(bm);
	}
	else if (bm->glPort == NULL && isAccelerated) {												/* If it is not accelerated and is desired to be accelerated */
		err = GetCompatiblePortTexture(bm, &bm->glPort);										/* Get a port texture that matches; NULL if hard error */
		if (kFskErrNone != err && kFskErrTextureTooLarge != err)
			goto bail;																			/* We are done */
		bm->accelerate = true;
	}

bail:
	return err;
}
#ifdef GL_TRACE
	#define FskGLSetBitmapSourceIsAccelerated(bm, isAccelerated) (GL_TRACE_LOG("FskGLSetBitmapSourceIsAccelerated"), FskGLSetBitmapSourceIsAccelerated(bm, isAccelerated))
#endif /* GL_TRACE */


/********************************************************************************
 * FskGLAccelerateBitmapSource
 ********************************************************************************/

#undef FskGLAccelerateBitmapSource
FskErr FskGLAccelerateBitmapSource(FskBitmap bm) {
	FskErr		err		= kFskErrNone;
	FskGLPort	glPort;

	#if defined(LOG_PARAMETERS) || defined(LOG_TEXTURE_LIFE) || defined(LOG_SET_TEXTURE)
		LOGD("GLAccelerateBitmapSource(bm={%p, writeSeed=%d, accelSeed=%d})", bm, bm->writeSeed, bm->accelerateSeed);
		LogSrcBitmap(bm, "bm");
	#endif /* LOG_PARAMETERS */

	BAIL_IF_NULL(bm, err, kFskErrInvalidParameter);

	if ((glPort = bm->glPort) == NULL) {														/* If there is no glPort, allocate one */
		err = GetCompatiblePortTexture(bm, &bm->glPort);										/* Get a port texture that matches; NULL if hard error */
		if (kFskErrNone != err && kFskErrTextureTooLarge != err)								/* Too large texture is not a serious error */
			goto bail;																			/* Abort if there is a serious error */
		glPort = bm->glPort;
	}
	glPort->texture.srcBM = bm;																	/* Save srcBM so that we can restore after pause/resume */
	if ((0 == glPort->texture.name) || (GL_TEXTURE_UNLOADED == glPort->texture.name)) {
		#ifdef LOG_TEXTURE_LIFE
			LOGD("GLAccelerateBitmapSource: Generating texture for %s bitmap %p", glPort->texture.name ? "unloaded" : "new", bm);
		#endif /* LOG_TEXTURE_LIFE */
		glPort->texture.nameU = 0;
		glPort->texture.nameV = 0;
		err = FskGenGLTexturesAndInit(1, &glPort->texture.name);
		PRINT_IF_ERROR(err, __LINE__, "FskGenGLTexturesAndInit");
		if (err || 0 == glPort->texture.name || GL_TEXTURE_UNLOADED == glPort->texture.name) {
			#ifdef LOG_TEXTURE_LIFE
				LOGD("GLAccelerateBitmapSource: Texture acceleration being deferred because there is no context.");
			#endif /* LOG_TEXTURE_LIFE */
			glPort->texture.name = GL_TEXTURE_UNLOADED;
			if (!err) {
				err = kFskErrGraphicsContext;
				#ifdef LOG_TEXTURE_LIFE
					if (err)
						LOGD("GLAccelerateBitmapSource: Texture allocation failed.");
				#endif /* LOG_TEXTURE_LIFE */
			}
			goto bail;
		}
		glPort->texture.wrapMode	= 0;
		glPort->texture.filter      = 0;
		if (kFskBitmapFormatYUV420 == bm->pixelFormat && !(gGLGlobalAssets.texImageYUV420 && CanDoHardwareVideo(bm))) {	/* YUV420 needs a U and a V texture as well as a Y texture */
			err = FskGenGLTexturesAndInit(2, &glPort->texture.nameU);
			#if defined(LOG_TEXTURE_LIFE) || defined(LOG_YUV)
				if (err)
					LOGD("GLAccelerateBitmapSource: {U,V} Texture allocation failed.");
			#endif /* LOG_TEXTURE_LIFE */
			/* TODO: Should check for errors */
		}
		else if (kFskBitmapFormatYUV420spuv == bm->pixelFormat || kFskBitmapFormatYUV420spvu == bm->pixelFormat) {	/* YUV420sp needs a UV texture as well as a Y texture */
			err = FskGenGLTexturesAndInit(1, &glPort->texture.nameU);
			#if defined(LOG_TEXTURE_LIFE) || defined(LOG_YUV)
				if (err)
					LOGD("GLAccelerateBitmapSource: UV Texture allocation failed.");
			#endif /* LOG_TEXTURE_LIFE */
		}
	}
	else {
		(void)InitializeTextures(3, &glPort->texture.name);	/* This initializes all textures that are not 0 or GL_TEXTURE_UNLOADED */
	}
	#ifdef LOG_SET_TEXTURE
		LOGD("GLAccelerateBitmapSource: Calling SetGLTexture(bm=%p -> tx=#%u)", bm, glPort->texture.name);
	#endif /* LOG_SET_TEXTURE */
	err = SetGLTexture(bm, NULL, &glPort->texture);
	PRINT_IF_ERROR(err, __LINE__, "SetGLTexture");
	if (err != kFskErrNone && err != kFskErrTextureTooLarge) {	/* This texture is bad news */
		LOGE("FskGLAccelerateBitmapSource FAILED to accelerate bitmap %p, %dx%d", bm, (int)bm->bounds.width, (int)bm->bounds.height);
		GLPortReallyDeletePortTextures(glPort);					/* Toss it */
		glPort->texture.name          = GL_TEXTURE_UNLOADED;	/* Indicate that the texture is unloaded */
		glPort->texture.bounds.width  = 0;						/* Force it to reallocate */
		glPort->texture.bounds.height = 0;
	}

bail:
	PRINT_IF_ERROR(err, __LINE__, "FskGLAccelerateBitmapSource");
	return err;
}
#ifdef GL_TRACE
	#define FskGLAccelerateBitmapSource(bm) (GL_TRACE_LOG("FskGLAccelerateBitmapSource"), FskGLAccelerateBitmapSource(bm))
#endif /* GL_TRACE */


/********************************************************************************
 * FskGLDeaccelerateBitmapSource
 ********************************************************************************/

#undef FskGLDeaccelerateBitmapSource
FskErr FskGLDeaccelerateBitmapSource(FskBitmap bm) {
	FskErr	err	= kFskErrNone;

	#if defined(LOG_PARAMETERS) || defined(LOG_TEXTURE_LIFE)
		LOGD("GLDeaccelerateBitmapSource(bm=%p)", bm);
		LogSrcBitmap(bm, "bm");
	#endif /* LOG_PARAMETERS */

	BAIL_IF_FALSE(bm && bm->glPort && bm->glPort->texture.name, err, kFskErrNotAccelerated);

	FskGLPortDispose(bm->glPort);
	bm->glPort = NULL;

bail:
	return err;
}
#ifdef GL_TRACE
	#define FskGLDeaccelerateBitmapSource(bm) (GL_TRACE_LOG("FskGLDeaccelerateBitmapSource"), FskGLDeaccelerateBitmapSource(bm))
#endif /* GL_TRACE */


/********************************************************************************
 * FskGLUpdateSource
 ********************************************************************************/

#undef FskGLUpdateSource
FskErr FskGLUpdateSource(FskConstBitmap bm) {
	FskErr	err	= kFskErrNone;

	#if defined(LOG_PARAMETERS) || defined(LOG_SET_TEXTURE)
		LOGD("GLUpdateSource(bm=%p)", bm);
		LogSrcBitmap(bm, "bm");
	#endif /* LOG_PARAMETERS */

	BAIL_IF_FALSE(bm && bm->glPort && bm->glPort->texture.name, err, kFskErrNotAccelerated);
	if (GL_TEXTURE_UNLOADED != bm->glPort->texture.name) {
		err = SetGLTexture(bm, NULL, &bm->glPort->texture);
		bm->glPort->texture.srcBM = bm;
		PRINT_IF_ERROR(err, __LINE__, "SetGLTexture");
	}
	else {
		#if defined(LOG_SET_TEXTURE)
			LOGD("GLUpdateSource found an UNLOADED texture: calling FskGLAccelerateBitmapSource", bm);
		#endif /* LOG_PARAMETERS */
		err = FskGLAccelerateBitmapSource((FskBitmap)bm);
	}

bail:
	PRINT_IF_ERROR(err, __LINE__, "FskGLUpdateSource(%p) #%u", bm, ((bm && bm->glPort) ? bm->glPort->texture.name : 0));
	return err;
}


/********************************************************************************
 * FskGLUnloadTexture
 ********************************************************************************/

#undef FskGLUnloadTexture
FskErr FskGLUnloadTexture(FskBitmap bm) {
	FskErr		err		= kFskErrNone;
	GLTexture	tx;

	#if defined(LOG_PARAMETERS) || defined(LOG_TEXTURE_LIFE)
		LOGD("FskGLUnloadTexture(bm=%p)", bm);
		LogSrcBitmap(bm, "bm");
	#endif /* LOG_PARAMETERS */

	BAIL_IF_NULL(bm, err, kFskErrInvalidParameter);
	BAIL_IF_FALSE((NULL != bm->glPort) && (0 != (tx = &bm->glPort->texture)->name), err, kFskErrNotAccelerated);

	if (GL_TEXTURE_UNLOADED != tx->name) {
		#ifdef LOG_TEXTURE_LIFE
			LOGD("Unloaded bitmap %p texture {#%u,#%u,#%u}", bm, tx->name, tx->nameU, tx->nameU);
		#endif /* LOG_TEXTURE_LIFE */
		FskDeleteGLTextures(1, &tx->name);				/* Unload texture */
		tx->name = GL_TEXTURE_UNLOADED;
		if (tx->nameU) { FskDeleteGLTextures(1, &tx->nameU); tx->nameU = GL_TEXTURE_UNLOADED; }
		if (tx->nameV) { FskDeleteGLTextures(1, &tx->nameV); tx->nameV = GL_TEXTURE_UNLOADED; }
		tx->bounds.width  = 0;
		tx->bounds.height = 0;
	}

bail:
	return err;
}


/********************************************************************************
 * FskGLUpdateTextureFromBitmap
 ********************************************************************************/

FskErr FskGLUpdateTextureFromBitmap(FskBitmap texBM, FskConstBitmap bm) {
	FskGLPort	glPort;
	FskErr		err;

	BAIL_IF_NULL(bm, err, kFskErrInvalidParameter);
	BAIL_IF_NULL((glPort = texBM->glPort), err, kFskErrNotAccelerated);
	BAIL_IF_ERR(err = SetGLTexture(bm, NULL, &glPort->texture));
	texBM->bounds = bm->bounds;
bail:
	return err;
}


/********************************************************************************
 * FskGLPortPixelsRead
 ********************************************************************************/

#undef FskGLPortPixelsRead
FskErr FskGLPortPixelsRead(FskGLPort glPort, Boolean backBuffer, FskConstRectangle srcRect, FskBitmap dstBM) {
	FskErr				err		= kFskErrNone;
	FskBitmap			tm		= NULL;
	SInt32				x1, y1, dstRowBytes, dstPixBytes;
	UInt32				portWidth, portHeight;
	GLFormat			glFormat;
	GLType				glType;
	void				*dst0;
	FskRectangleRecord	sRect;
	FskBitmapFormatEnum	desiredPixelFormat;

	#if defined(LOG_PARAMETERS) || defined(LOG_READPIXELS)
		LogPortPixelsRead(glPort, backBuffer, srcRect, dstBM);
	#endif /* LOG_PARAMETERS */

	/* Trim srcRect to the size of the destination buffer */
	if (srcRect) {
		sRect = *srcRect;
		if (sRect.width  > dstBM->bounds.width)		sRect.width  = dstBM->bounds.width;
		if (sRect.height > dstBM->bounds.height)	sRect.height = dstBM->bounds.height;
	}
	else {
		FskRectangleSet(&sRect, 0, 0, dstBM->bounds.width, dstBM->bounds.height);
	}

	/* Trim against port size */
	FskGLPortFocus(glPort);																			/* Make sure to set parameters to the right GL port */
	FskGLPortGetSize(glPort, &portWidth, &portHeight);
	if ((x1 = sRect.x + sRect.width)  > (SInt32)portWidth)	{ sRect.width  = (x1 = portWidth)  - sRect.x; }
	if ((y1 = sRect.y + sRect.height) > (SInt32)portHeight)	{ sRect.height = (y1 = portHeight) - sRect.y; }
	CHANGE_SCISSOR_ENABLE(false);																	/* Assure no clipping makes Adreno happy */

	/* Reverse the sense of y, because we scan from top to bottom and GL scans from bottom to top */
	sRect.y = portHeight - sRect.y - sRect.height;

	if (!gGLGlobalAssets.blitContext->frameBufferObject || !gGLGlobalAssets.blitContext->fboTexture) {	/* If rendering to the screen rather than a frame buffer object ... */
		#if defined(GL_VERSION_1_1) || defined(GL_VERSION_2_0)
			glReadBuffer(backBuffer ? GL_BACK : GL_FRONT);											/* ... choose either the front or back buffer */
			(void)glGetError();		/* Clear errors */
		#else /* GL_VERSION */
			if (backBuffer) {}																		/* GLES doesn't offer the option; avoid warnings */
		#endif /* GL_VERSION */
	}

	FskBitmapWriteBegin(dstBM, &dst0, &dstRowBytes, NULL);											/* Get base address, row bytes, ... */
	glFinish();																						/* Make sure the rendering has completed */


	/*
	 * Read directly into a texture
	 */
	if (dstBM->glPort && dstBM->glPort->texture.name) {
		GLTexture	tx = &dstBM->glPort->texture;
		#ifdef LOG_READPIXELS
			LOGD("Transferring screen %ux%u to texture #%u", (unsigned)portWidth, (unsigned)portHeight, tx->name);
		#endif /* LOG_READPIXELS */
		if (GL_TEXTURE_UNLOADED == tx->name) {														/* If currently unloaded... */
			err = FskGenGLTexturesAndInit(1, &tx->name);											/* ... generate a texture */
			BAIL_IF_ERR(err);
		}
		SET_TEXTURE(tx->name);
		err = ReshapeRGBTexture(GL_RGB, sRect.width, sRect.height, GL_RGB, GL_UNSIGNED_BYTE, tx);
		BAIL_IF_ERR(err);
		dstBM->glPort->texture.flipY = gGLGlobalAssets.blitContext->fboTexture ? false : true;		/* Indicate that we loaded the texture from the screen with atypical vertical polarity */
		dstBM->accelerateSeed = dstBM->writeSeed;													/* Indicate that the texture is up-to-date */
		glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, sRect.x, sRect.y, sRect.width, sRect.height, 0);
		err = glGetError();
		#ifdef LOG_READPIXELS
			LOGD("Call to glCopyTexImage2D(iglFmt=%s, sRect.x=%d, sRect.y=%d, sRect.width=%d, sRect.height=%d) into texture(#%u, %ux%u) returns glError=%d",
				GLInternalFormatNameFromCode(GL_RGB), (int)sRect.x, (int)sRect.y, (int)sRect.width, (int)sRect.height, tx->name, (unsigned)tx->bounds.width, (unsigned)tx->bounds.height, err);
		#endif /* LOG_READPIXELS */
		if (err) {
			glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, sRect.x, sRect.y, sRect.width, sRect.height);
			err = glGetError();
			#ifdef LOG_READPIXELS
				LOGD("Call to glCopyTexSubImage2D(...) returns glError=%d", err);
			#endif /* LOG_READPIXELS */
			PRINT_IF_ERROR(err, __LINE__, "glCopyTexSubImage2D");
			if (err)
				err = FskErrorFromGLError(err);
		}
		goto bail;																					/* We  are done reading into a texture. */
	}


	/*
	 * Read into a bitmap
	 */

	/* Set GL parameters for the read */
	#ifdef LOG_READPIXELS
		LOGD("Transferring screen %ux%u to bitmap %p", (unsigned)portWidth, (unsigned)portHeight, dstBM);
	#endif /* LOG_READPIXELS */
	err = GetFskGLError();																			/* Clear any previous errors */
	dstPixBytes = dstBM->depth >> 3;																/* ... and bytes per pixel. */
	#ifdef GL_PACK_ROW_LENGTH
		glPixelStorei(GL_PACK_ROW_LENGTH, dstRowBytes / dstPixBytes);								/* Tell GL about the rowbytes of the destination. */
	#endif /* GL_PACK_ROW_LENGTH */
	glPixelStorei(GL_PACK_ALIGNMENT, 1);															/* Don't assume any special alignment. */

	desiredPixelFormat = dstBM->pixelFormat;
	if (0 == (gGLGlobalAssets.readFormats & (1 << desiredPixelFormat))) {							/* Check whether we can read  directly into the desired pixel format */
		dstBM->pixelFormat = kFskBitmapFormatUnknown;												/* We cannot read the texture directly into the desired pixel format */
		switch (desiredPixelFormat) {
			case kFskBitmapFormat32A16RGB565LE:
			case kFskBitmapFormat32ARGB:
			case kFskBitmapFormat32BGRA:
			case kFskBitmapFormat32ABGR:	dstBM->pixelFormat = kFskBitmapFormat32RGBA;			/* We can always read into 32RGBA */
											break;
			case kFskBitmapFormat24BGR:		if (gGLGlobalAssets.readFormats & (1 << (UInt32)kFskBitmapFormat24RGB))	dstBM->pixelFormat = kFskBitmapFormat24RGB;
											break;
			default:						break;
		}
	}
	if (kFskBitmapFormatUnknown != dstBM->pixelFormat) {											/* We can read the texture directly into a related pixel format */
	tryRGBAdst:
		err = FskGLGetFormatAndTypeFromPixelFormat(dstBM->pixelFormat, &glFormat, &glType, NULL);
		if (!err) {
			if (sRect.width  > dstBM->bounds.width)													/* Assure that the specified area can fit into the supplied bitmap, ... */
				sRect.width  = dstBM->bounds.width;													/* ... and trim the are if not */
			if (sRect.height > dstBM->bounds.height)
				sRect.height = dstBM->bounds.height;
			glReadPixels(sRect.x, sRect.y, sRect.width, sRect.height, glFormat, glType, dst0);		/* Get the pixels from the GPU into the bitmap. */
			err = glGetError();
			#ifdef LOG_READPIXELS
				LOGD("glReadPixels(x=%d y=%d w=%d h=%d fmt=%s typ=%s pix=%p) returns %#04x",
					(int)sRect.x, (int)sRect.y, (int)sRect.width, (int)sRect.height, GLFormatNameFromCode(glFormat), GLTypeNameFromCode(glType), dst0, (int)err);
				PRINT_IF_ERROR(err, __LINE__, "glReadPixels(requestedFormat=%s)", FskBitmapFormatName(dstBM->pixelFormat));
			#endif /* LOG_READPIXELS */
			if (err && kFskBitmapFormat32RGBA != dstBM->pixelFormat && 32 == dstBM->depth) {
				dstBM->pixelFormat = kFskBitmapFormat32RGBA;
				goto tryRGBAdst;
			}
			err = FskErrorFromGLError(err);
		}
		if (err) {																					/* This shouldn't happen, but something went wrong */
			dstBM->pixelFormat = kFskBitmapFormatUnknown;											/* Read into a format that we know works, and convert */
		}
		#ifndef GL_PACK_ROW_LENGTH
			else {
				#ifdef LOG_READPIXELS
					LOGD("ChangeRowBytes(pix=%p w=%d h=%d pb=%d orb=%d nrb=%d", dst0, (int)sRect.width, (int)sRect.height, (int)dstPixBytes, (int)sRect.width * (int)dstPixBytes, (int)dstRowBytes);
				#endif /* LOG_READPIXELS */
				ChangeRowBytes(dst0, sRect.width, sRect.height, dstPixBytes, sRect.width * dstPixBytes, dstRowBytes);	/* Make the image have the correct rowBytes */
			}
		#endif /* !GL_PACK_ROW_LENGTH */
	}
	if (kFskBitmapFormatUnknown == dstBM->pixelFormat) {											/* We cannot read the texture directly into a related pixel format */
		void *tmp0;
		FskBitmapFormatEnum tmpPixelFormat;
		dstBM->pixelFormat = desiredPixelFormat;													/* Restore the format, leaving incoming bitmap intact. */
		if (kFskBitmapFormatDefaultRGBA == kFskBitmapFormat32BGRA && (gGLGlobalAssets.readFormats & (1 << (UInt32)kFskBitmapFormat32BGRA)))
			tmpPixelFormat = kFskBitmapFormat32BGRA;
		else
			tmpPixelFormat = kFskBitmapFormat32RGBA;
		err = FskBitmapNew(sRect.width, sRect.height, tmpPixelFormat, &tm);							/* Allocate a temporary 32RGBA/32BGRA bitmap. */
		#ifdef LOG_READPIXELS
			PRINT_IF_ERROR(err, __LINE__, "FskBitmapNew(%s)", FskBitmapFormatName(tmpPixelFormat));
		#endif /* LOG_READPIXELS */
		BAIL_IF_ERR(err);
		FskBitmapWriteBegin(tm, &tmp0, NULL, NULL);													/* Get temporary bitmap base address */
		#ifdef GL_PACK_ROW_LENGTH
			glPixelStorei(GL_PACK_ROW_LENGTH, tm->rowBytes / (tm->depth >> 3));						/* Pixels per row */
		#else /* GL_PACK_ROW_LENGTH */
			tm->rowBytes = sRect.width * (tm->depth>>3);											/* Remove the gap in the tmp rowbytes (naughty!) */
		#endif /* GL_PACK_ROW_LENGTH */
		tryRGBAtm:
		err = FskGLGetFormatAndTypeFromPixelFormat(tm->pixelFormat, &glFormat, &glType, NULL);
		PRINT_IF_ERROR(err, __LINE__, "FskGLGetFormatAndTypeFromPixelFormat");
		BAIL_IF_ERR(err);
		#ifdef LOG_READPIXELS
			LOGD("Trying glReadPixels(sRect.x=%d, sRect.y=%d, sRect.width=%d, sRect.height=%d, glFormat=%s, glType=%s, dst0=%p), allocated %s",
				(int)sRect.x, (int)sRect.y, (int)sRect.width, (int)sRect.height, GLFormatNameFromCode(glFormat), GLTypeNameFromCode(glType), tmp0, FskBitmapFormatName(tmpPixelFormat));
		#endif /* LOG_READPIXELS */
		glReadPixels(sRect.x, sRect.y, sRect.width, sRect.height, glFormat, glType, tmp0);			/* Get the pixels from the GPU into the bitmap. */
		err = glGetError();
		if (err && kFskBitmapFormat32RGBA != tm->pixelFormat) {
			tm->pixelFormat = kFskBitmapFormat32RGBA;
			goto tryRGBAtm;
		}
		#ifdef LOG_READPIXELS
			PRINT_IF_ERROR(err, __LINE__, "glReadPixels(fallbackFormat=%s err=%#04x)", FskBitmapFormatName(tm->pixelFormat), (int)err);
		#endif /* LOG_READPIXELS */
		err = FskErrorFromGLError(err);
		BAIL_IF_ERR(err);
		#if !defined(SRC_32RGBA) || !SRC_32RGBA														/* If fsk does not support RGBA, convert to BGRA. */
			if (kFskBitmapFormat32RGBA == tm->pixelFormat) {
				ConvertImage32Format(tmp0, sRect.width, sRect.height, tm->rowBytes, tm->pixelFormat, kFskBitmapFormat32BGRA);
				tm->pixelFormat = kFskBitmapFormat32BGRA;
			}
		#endif /* !defined(SRC_32RGBA) || !SRC_32RGBA */
		FskBitmapWriteEnd(tm);
		#ifdef LOG_READPIXELS
			LOGD("FskBitmapDraw(%s --> %s)", FskBitmapFormatName(tm->pixelFormat), FskBitmapFormatName(dstBM->pixelFormat));
		#endif /* LOG_READPIXELS */
		err = FskBitmapDraw(tm, &tm->bounds, dstBM, &tm->bounds, NULL, NULL, kFskGraphicsModeCopy, NULL);	/* dstBM has the correct rowBytes */
		PRINT_IF_ERROR(err, __LINE__, "FskBitmapDraw");
		BAIL_IF_ERR(err);
	}

	/* Reformat as necessary */
	if (!gGLGlobalAssets.blitContext->fboTexture)
		ReflectImageVertically(dst0, sRect.width, sRect.height, dstPixBytes, dstRowBytes);			/* Convert from Y-up to Y-down. */
	if (desiredPixelFormat != dstBM->pixelFormat) {													/* Convert to desired format. */
		#ifdef LOG_READPIXELS
			LOGD("Converting %s --> %s in place", FskBitmapFormatName(dstBM->pixelFormat), FskBitmapFormatName(desiredPixelFormat));
		#endif /* LOG_READPIXELS */
		if      (dstBM->depth == 24)	ConvertImage24Format((UInt8 *)dst0, sRect.width, sRect.height, dstRowBytes, dstBM->pixelFormat, desiredPixelFormat);
		else if (dstBM->depth == 32)	ConvertImage32Format((UInt32*)dst0, sRect.width, sRect.height, dstRowBytes, dstBM->pixelFormat, desiredPixelFormat);
		dstBM->pixelFormat = desiredPixelFormat;
	}

bail:
	PRINT_IF_ERROR(err, __LINE__, "FskGLPortPixelsRead");
	#ifdef GL_PACK_ROW_LENGTH
		glPixelStorei(GL_PACK_ROW_LENGTH, 0);														/* Reset assumed rowBytes */
	#endif /* GL_PACK_ROW_LENGTH */
	FskBitmapDispose(tm);
	FskBitmapWriteEnd(dstBM);
	FskGLPortDefocus(glPort);																		/* Restore port focus when all GL commands are done */
	gGLGlobalAssets.blitContext->lastTexture = 0;
	return err;
}


/********************************************************************************
 * FskGLPortSwapBuffers
 ********************************************************************************/

#undef FskGLPortSwapBuffers
FskErr FskGLPortSwapBuffers(FskConstGLPort port) {
#ifdef EGL_VERSION
	#if defined(LOG_SWAPBUFFERS)
		static const char *glend[2] = { "glFlush", "glFinish" };
		FskTimeRecord time;
		FskTimeGetNow(&time);
	#endif /* LOG_SWAPBUFFERS */
	if (gGLGlobalAssets.rendererIsTiled)	glFinish();	/* Tiled renderers crash if you only flush; they need to be be completely finished before swapping buffers. */
	else									glFlush();	/* Documentation says that flush is not necessary, but experience shows that it crashes otherwise. */
	if (port) {											/* TODO: We don't really use the port. */
		EGLBoolean success = eglSwapBuffers(gGLGlobalAssets.blitContext->eglDisplay, gGLGlobalAssets.blitContext->eglSurface);
		#if defined(LOG_SWAPBUFFERS)
			LOGD("GLPortSwapBuffers%10d.%06d: %s, eglSwapBuffers returns %s", time.seconds, time.useconds, glend[gGLGlobalAssets.rendererIsTiled], success ? "success" : "failure");
		#endif /* GL_DEBUG */
		return success? kFskErrNone : kFskErrOperationFailed;
	}
	#if defined(LOG_SWAPBUFFERS)
		else
			LOGD("GLPortSwapBuffers%10d.%06d: %s", time.seconds, time.useconds, glend[gGLGlobalAssets.rendererIsTiled]);
	#endif /* LOG_SWAPBUFFERS */
	return kFskErrNone;
#else /* !EGL_VERSION */
	//glutSwapBuffers();
	//return kFskErrNone;
	if (port) {}	/* Remove unused parameter messages */
	return kFskErrUnimplemented;
#endif /* EGL_VERSION */
}


/********************************************************************************
 * FskGLBitmapTextureSetRenderable
 ********************************************************************************/

#undef FskGLBitmapTextureSetRenderable
FskErr FskGLBitmapTextureSetRenderable(FskBitmap bm, Boolean renderable) {
#if GLES_VERSION == 2
	FskErr		err	= kFskErrNone;
	FskGLPort	glPort;
	GLFormat	format;

	#if defined(LOG_PARAMETERS) || defined(LOG_RENDER_TO_TEXTURE)
		LOGD("FskGLBitmapTextureSetRenderable(bm=%p, renderable=%d)", bm, (int)renderable);
		LogDstBitmap(bm, "bm");
	#endif /* LOG_PARAMETERS */

	if (!bm)
		goto bail;

	#if TARGET_OS_ANDROID
		if (!GL_GLOBAL_ASSETS_ARE_INITIALIZED()) {	/* Deferred initialization */
			err = InitGlobalGLAssets();
			#if GL_DEBUG
				if (err)	LOGD("Deferred GL Port initialization in FskGLBitmapTextureSetRenderable fails with code %d", (int)err);
				else		LOGD("Deferred GL Port initialization in FskGLBitmapTextureSetRenderable succeeds");
			#endif /* GL_DEBUG */
		}
	#endif /* TARGET_OS_ANDROID */

	if (renderable) {
		if (NULL == bm->glPort) {															/* If the bitmap has not been accelerated, ... */
			BAIL_IF_ERR(err = FskGLSetBitmapSourceIsAccelerated(bm, true));					/* ... accelerate it */
			#ifdef LOG_RENDER_TO_TEXTURE
				LogDstBitmap(bm, "bm FskGLBitmapTextureSetRenderable");
			#endif /* LOG_RENDER_TO_TEXTURE */
		}
		glPort = bm->glPort;
		if (0 == glPort->texture.name || GL_TEXTURE_UNLOADED == glPort->texture.name) {		/* If there is no current texture ... */
			BAIL_IF_ERR(err = FskGenGLTexturesAndInit(1, &glPort->texture.name));			/* ... generate one */
			if (GL_TEXTURE_UNLOADED == glPort->texture.nameU)								/* If there is a missing persistent texture ... */
				BAIL_IF_ERR(err = FskGenGLTexturesAndInit(1, &glPort->texture.nameU));		/* ... generate one */
			#ifdef LOG_RENDER_TO_TEXTURE
				LogDstBitmap(bm, "bm FskGLBitmapTextureSetRenderable");
			#endif /* LOG_RENDER_TO_TEXTURE */
		}
		#if RGB_TEXTURES_WORK
			format = (bm->depth >= 32 || bm->pixelFormat == kFskBitmapFormatGLRGBA) ? GL_RGBA : GL_RGB;
		#else
			format = GL_RGBA;	/* Only RGBA is supported cross-platform, so don't do (bm->depth >= 32 || bm->pixelFormat == kFskBitmapFormatGLRGBA) ? GL_RGBA : GL_RGB; */
		#endif
		CHANGE_TEXTURE(glPort->texture.name);
		BAIL_IF_ERR(err = ReshapeOffscreenTexture(format, bm->bounds.width, bm->bounds.height, format, GL_UNSIGNED_BYTE, &glPort->texture));
		bm->accelerate = true;																/* Indicate that the texture is accelerated, ... */
		bm->accelerateSeed = bm->writeSeed;													/* ... and up-to-date */
		glPort->texture.srcBM = bm;
		glPort->texture.flipY = false;
		glPort->texture.fboInited = false;
		glPort->portWidth  = bm->bounds.width;												/* Set up this port ... */
		glPort->portHeight = bm->bounds.height;												/* ... as a destination */
		#if CHECK_GL_ERROR
			err = GetFskGLError();
		#endif /* CHECK_GL_ERROR */
	}
	else {
		if (NULL != (glPort = bm->glPort)) {
			glPort->portWidth  = 0;
			glPort->portHeight = 0;
		}
	}

bail:
	return err;

#else /* GLES_VERSION == 1 */

	if (bm || renderable) {}
	return kFskErrUnimplemented;

#endif /* GLES_VERSION == 1 */
}
#ifdef GL_TRACE
	#define FskGLBitmapTextureSetRenderable(bm) (GL_TRACE_LOG("FskGLBitmapTextureSetRenderable"), FskGLBitmapTextureSetRenderable(bm))
#endif /* GL_TRACE */


#if GLES_VERSION == 2
/********************************************************************************
 * FskGLBitmapTextureTargetSet
 ********************************************************************************/

#undef FskGLBitmapTextureTargetSet
FskErr FskGLBitmapTextureTargetSet(FskBitmap bm) {
	FskErr		err		= kFskErrNone;
	FskGLPort	glPort	= bm ? bm->glPort : NULL;

	#if defined(LOG_PARAMETERS) || defined(LOG_RENDER_TO_TEXTURE)
		LOGD("FskGLBitmapTextureTargetSet(bm=%p)", bm);
		LogSrcBitmap(bm, "bm");
	#endif /* LOG_PARAMETERS */

	if (glPort && glPort->texture.name) {														/* Render to a texture */
		if (!gGLGlobalAssets.blitContext->frameBufferObject) {									/* If we don't already have a frame buffer object, ... */
			glGenFramebuffers(1, &gGLGlobalAssets.blitContext->frameBufferObject);				/* ... allocate one */
			#ifdef LOG_RENDER_TO_TEXTURE
				LOGD("Allocated FBO #%d", gGLGlobalAssets.blitContext->frameBufferObject);
			#endif /* LOG_RENDER_TO_TEXTURE */
			#if CHECK_GL_ERROR
				BAIL_IF_ERR(err = GetFskGLError());
			#endif /* CHECK_GL_ERROR */
		}
		if (gGLGlobalAssets.blitContext->fboTexture != glPort->texture.name) {					/* The rendered-to texture is changing */
			if (gGLGlobalAssets.blitContext->fboTexture)
				FskGLPortSwapBuffers(NULL);														/* This causes tiled renderers to complete before swapping */
			glBindFramebuffer(GL_FRAMEBUFFER, gGLGlobalAssets.blitContext->frameBufferObject);	/* Render to this frame buffer object */
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, glPort->texture.name, 0);		/* With this texture */
			#ifdef LOG_RENDER_TO_TEXTURE
				LOGD("Rendering to texture #%d", glPort->texture.name);
			#endif /* LOG_RENDER_TO_TEXTURE */
			gGLGlobalAssets.blitContext->fboTexture = glPort->texture.name;						/* Remember the texture used for an FBO */
			CHANGE_TEXTURE(0);																	/* Make sure that a new texture is chosen */
			PerturbFloat(&gGLGlobalAssets.matrix[0][0]);										/* Force the view to be recomputed, and cause FskGLSetGLView() to clear to opaque black */
			glPort->portWidth  = glPort->texture.bounds.width;									/* Set up this port ... */
			glPort->portHeight = glPort->texture.bounds.height;									/* ... as a destination */
			FskGLSetGLView(glPort);
			#if GL_DEBUG
				err = FskErrorFromGLError(glCheckFramebufferStatus(GL_FRAMEBUFFER));			/* Check the frame buffer status */
				if (kFskErrGLFramebufferComplete == err) {
					err = kFskErrNone;
				}
				else {																			/* Error: incomplete frame buffer */
					LOGD("FskGLBitmapTextureTargetSet: %s: %s", GLFrameBufferStatusStringFromCode(err), GLInternalFormatNameFromCode(glPort->texture.glIntFormat));    /* Print the reason why */
					err = kFskErrNotAccelerated;
					goto bail;
				}
			#endif /* GL_DEBUG */
		}
		/* else: the render-to texture remains the same */
	}
	else if (gGLGlobalAssets.blitContext->fboTexture) {											/* render to the screen */
		#ifdef LOG_RENDER_TO_TEXTURE
			GLint curFB;
			LOGD("Rendering to screen");
			glGetIntegerv(GL_FRAMEBUFFER_BINDING, &curFB);
			if (curFB == 0)
				LOGD("PANIC: curFB is %d, whereas fbo=%d and fboTex=%d", curFB, gGLGlobalAssets.blitContext->frameBufferObject, gGLGlobalAssets.blitContext->fboTexture);
			if (!gGLGlobalAssets.blitContext->frameBufferObject)
				LOGD("PANIC: fboTexture=%d while frameBufferObject=%d", gGLGlobalAssets.blitContext->fboTexture, gGLGlobalAssets.blitContext->frameBufferObject);
		#endif /* LOG_RENDER_TO_TEXTURE */

		//@@jph this call to glFramebufferTexture2D seems unnecessary
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);		/* No texture on framebuffer */

		glBindFramebuffer(GL_FRAMEBUFFER, gGLGlobalAssets.blitContext->defaultFBO);				/* Render to the screen */
		gGLGlobalAssets.blitContext->fboTexture = 0;
		PerturbFloat(&gGLGlobalAssets.matrix[0][0]);											/* Force the view to change, and cause FskGLSetGLView() to clear to opaque black */
		FskGLSetGLView(gCurrentGLPort);
	}

#if CHECK_GL_ERROR || GL_DEBUG
	if (!err)					/* Don't overwrite the first error */
		err = GetFskGLError();
bail:
#endif /* CHECK_GL_ERROR || GL_DEBUG */

	return err;
}
#ifdef GL_TRACE
	#define FskGLBitmapTextureTargetSet(bm) (GL_TRACE_LOG("FskGLBitmapTextureTargetSet"), FskGLBitmapTextureTargetSet(bm))
#endif /* GL_TRACE */
#endif /* GLES_VERSION == 2 */


/****************************************************************************//**
 * Copy between two textures.
 * The textures must be the same size.
 *	\param[in]		src		the source texture.
 *	\param[in,out]	dst		the destination texture.
 *	\param[in]		glPort	the glPort, associated with either the src or dst, to specify bounds.
 *	\return			kFskErrNone	if the operation was completed successfully.
 ********************************************************************************/

static void CopyTexture(GLuint src, GLuint dst, FskGLPort glPort) {
#if GLES_VERSION == 2
	if (gGLGlobalAssets.blitContext->fboTexture != dst) {
		CHANGE_TEXTURE(0);																		/* Disengage any source texture */
		if (gGLGlobalAssets.blitContext->fboTexture)
			FskGLPortSwapBuffers(NULL);															/* This causes tiled renderers to complete before swapping */
		if (!gGLGlobalAssets.blitContext->frameBufferObject)									/* If we don't already have a frame buffer object, ... */
			glGenFramebuffers(1, &gGLGlobalAssets.blitContext->frameBufferObject);				/* ... allocate one */
		glBindFramebuffer(GL_FRAMEBUFFER, gGLGlobalAssets.blitContext->frameBufferObject);		/* Render to this frame buffer object */
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, dst, 0);	/* With this texture */
		gGLGlobalAssets.blitContext->fboTexture = dst;											/* Remember the texture used for an FBO */
		CHANGE_SCISSOR_ENABLE(false);															/* No scissoring */
		glClearColor(0., 0., 0., 1.);
		glClear(GL_COLOR_BUFFER_BIT);															/* Initialize frame buffer */
		#if CHECK_GL_ERROR
		{	FskErr err;
			if ((int)(err = glGetError()) != 0) LOGE("GL error (%d): %s, %s:%d", (int)err, GLErrorStringFromCode(err), __FILE__, __LINE__);
		}
		#endif /* CHECK_GL_ERROR */
	}

	FskGLSetGLView(glPort);
	(void)SetBitmapProgramOpColor(NULL, false, kFskGraphicsModeCopy, NULL);					/* Sets program, matrix, opColor */
	CHANGE_TEXTURE(src);																	/* Set the source texture */
	CHANGE_BLEND_ENABLE(false);																/* No blending */
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);					/* Set texture parameters */
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	MakeTexRectVertices(&glPort->texture.bounds);											/* Set (x,y) */
	MakeRectTexCoords(&glPort->texture.bounds, &glPort->texture.bounds, &glPort->texture);	/* Set (u,v) */
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);													/* Draw quad */
	CHANGE_TEXTURE(0);
#endif /* GLES_VERSION == 2 */
}


/****************************************************************************//**
 * Restore the contents of a GL Port Texture.
 *	\param[in,out]	glPort	the GL port, whose texture contents are to be restored, from nameU to name.
 *	\return			kFskErrNone	if the operation was completed successfully.
 ********************************************************************************/

static void RestoreGLPortTexture(FskGLPort glPort, Boolean doBlend) {
#if GLES_VERSION == 2
	CopyTexture(glPort->texture.nameU, glPort->texture.name, glPort);						/* Sets to GL_CLAMP_TO_EDGE, GL_NEAREST */
	CHANGE_BLEND_ENABLE(doBlend);
	glPort->texture.wrapMode = GL_CLAMP_TO_EDGE;											/* Remember ... */
	glPort->texture.filter   = GL_NEAREST;													/* ... those changes */
#endif /* GLES_VERSION == 2 */
}


/********************************************************************************
 * FskGLRenderToBitmapTexture
 ********************************************************************************/

#undef FskGLRenderToBitmapTexture
FskErr FskGLRenderToBitmapTexture(FskBitmap bm, FskConstColorRGBA clear) {
#if GLES_VERSION == 2

	FskErr		err				= kFskErrNone;
	Boolean		doBlend			= gGLGlobalAssets.blitContext->glBlend,
				doPersistent	= false;

	#if defined(LOG_PARAMETERS) || defined(LOG_RENDER_TO_TEXTURE)
		LOGD("FskGLRenderToBitmapTexture(bm=%p, clear=%p)", bm, clear);
		LogSrcBitmap(bm, "bm");
		LogColor(clear, "clear");
	#endif /* LOG_PARAMETERS */

	if (bm && bm->glPort) {	/* If the texture is persistent, swap textures to save the previous frame */
		GLTexture tx = &bm->glPort->texture;
		#if TARGET_OS_ANDROID
			if (tx->name == GL_TEXTURE_UNLOADED) {
				err = FskGLBitmapTextureSetRenderable(bm, true);
				#if CHECK_GL_ERROR
					BAIL_IF_ERR(err);
				#endif /* CHECK_GL_ERROR */
			}
		#endif /* TARGET_OS_ANDROID */
		if (tx->persistent && tx->fboInited && tx->name && tx->name != gGLGlobalAssets.blitContext->fboTexture) {
			GLuint swapTex;
			swapTex   = tx->name;
			tx->name  = tx->nameU;
			tx->nameU = swapTex;
			doPersistent = true;
			#if defined(LOG_RENDER_TO_TEXTURE)
				LOGD("FskGLRenderToBitmapTexture swaps textures #%u and #%u", tx->nameU, tx->name);
			#endif /* LOG_RENDER_TO_TEXTURE */
		}
	}

	err = FskGLBitmapTextureTargetSet(bm);
	#if CHECK_GL_ERROR
		BAIL_IF_ERR(err);
	#endif /* CHECK_GL_ERROR */

	/* This clear makes tiled renderers happy */
	CHANGE_SCISSOR_ENABLE(false);
	if (clear)
		glClearColor(UInt8ColorToGLCoord(clear->r), UInt8ColorToGLCoord(clear->g), UInt8ColorToGLCoord(clear->b), UInt8ColorToGLCoord(clear->a));
	else
	#ifdef BG3CDP_GL
		glClearColor(0., 0., 0., 0.);																/* The default color is transparent black */
	#else /* !BG3CDP_GL */
		glClearColor(0., 0., 0., 1.);																/* The default color is opaque black */
	#endif /* !BG3CDP_GL */
	glClear(GL_COLOR_BUFFER_BIT);
	#if defined(LOG_RENDER_TO_TEXTURE)
		if (clear)	LOGD("glClear COLOR(%.3f,%.3f,%.3f,%.3f)", UInt8ColorToGLCoord(clear->r), UInt8ColorToGLCoord(clear->g), UInt8ColorToGLCoord(clear->b), UInt8ColorToGLCoord(clear->a));
		else		LOGD("glClear COLOR(%.3f,%.3f,%.3f,%.3f)", 0.f, 0.f, 0.f, 1.f);
	#endif /* LOG_RENDER_TO_TEXTURE */
	if (bm && bm->glPort)
		bm->glPort->texture.fboInited = true;														/* True means it has been cleared at least once */

	if (doPersistent) {
		RestoreGLPortTexture(bm->glPort, doBlend);
		#if defined(LOG_RENDER_TO_TEXTURE)
			LOGD("FskGLRenderToBitmapTexture restores #%u to persistent texture #%u %c= #%u", bm->glPort->texture.nameU, gGLGlobalAssets.blitContext->fboTexture,
				((gGLGlobalAssets.blitContext->fboTexture == bm->glPort->texture.name) ? '=' : '!'), bm->glPort->texture.name);
		#endif /* LOG_RENDER_TO_TEXTURE */
	}

	#if CHECK_GL_ERROR
		err = GetFskGLError();
bail:
	#endif /* CHECK_GL_ERROR */
	return err;

#else /* GLES_VERSION == 1 */

	if (bm || clear) {}
	return kFskErrUnimplemented;

#endif /* GLES_VERSION == 1 */
}
#ifdef GL_TRACE
	#define FskGLRenderToBitmapTexture(bm) (GL_TRACE_LOG("FskGLRenderToBitmapTexture"), FskGLRenderToBitmapTexture(bm, clear))
#endif /* GL_TRACE */


/********************************************************************************
 * FskGLRecommendRenderingToTexture
 ********************************************************************************/

Boolean FskGLRecommendRenderingToTexture(void) {
	return gGLGlobalAssets.hasFBO;
}


/********************************************************************************
 * FskGLRectangleFill
 ********************************************************************************/

#undef FskGLRectangleFill
FskErr FskGLRectangleFill(FskBitmap dstBM, FskConstRectangle r, FskConstColorRGBA color, UInt32 mode, FskConstGraphicsModeParameters modeParams) {
	FskErr		err			= kFskErrNone;
	FskGLPort	glPort		= NULL;
	SInt32		blendLevel	= (modeParams && ((UInt32)(modeParams->blendLevel) < 255)) ? modeParams->blendLevel : 255;

	#if defined(LOG_PARAMETERS)
		LogGLRectangleFill(dstBM, r, color, mode, modeParams);
	#endif /* LOG_PARAMETERS */

	BAIL_IF_ERR(err = FskGLDstPort(dstBM, &glPort));

	mode &= kFskGraphicsModeMask;	/* Clear out kFskGraphicsModeBilinear bit */

	#ifdef USE_GLCLEAR	/* 16 pixels wide by 8 pixels high is highly efficient; otherwise, use a shader */
		if ((mode != kFskGraphicsModeColorize) && (blendLevel >= 255)) {							/* Opaque fill */
			SInt32 y;
			//GLCoordinateType saveColor[4];
			/* Save the clear color */
			//#if GL_COORD_TYPE_ENUM == GL_FLOAT
			//	glGetFloatv(GL_COLOR_CLEAR_VALUE, saveColor);
			//#else /* GL_COORD_TYPE_ENUM == GL_FIXED */
			//	glGetFixedv(GL_COLOR_CLEAR_VALUE, saveColor);
			//#endif /* GL_COORD_TYPE_ENUM == GL_FIXED */

			/* Convert from Y-down coordinate system to y-up */
			y = glPort->portHeight - r->y - r->height;

			/* Clear the specified rectangle */
			CHANGE_SCISSOR(glPort, r->x, y, r->width, r->height);									/* This also enables scissoring */
			glClearColorFskColorRGBA(color);
			glClear(GL_COLOR_BUFFER_BIT);

			/* Restore the clear color */
			//#if GL_COORD_TYPE_ENUM == GL_FLOAT
			//	glClearColor(saveColor[0], saveColor[1], saveColor[2], saveColor[3]);
			//#else /* GL_COORD_TYPE_ENUM == GL_FIXED */
			//	glClearColorx(saveColor[0], saveColor[1], saveColor[2], saveColor[3]);
			//#endif /* GL_COORD_TYPE_ENUM == GL_FIXED */
		} else
	#endif /* USE_GLCLEAR */
	{
		#if GLES_VERSION < 2
			glDisable(GL_TEXTURE_2D);
		#else /* GLES_VERSION == 2 */
			BAIL_IF_ERR(err = FskGLUseProgram(gGLGlobalAssets.fill.program));
			FskSetProgramOpColor(gGLGlobalAssets.fill.opColor, &gGLGlobalAssets.blitContext->fill.lastParams, color, false, NULL);
			CHANGE_SHADER_MATRIX(fill);
		#endif /* GLES_VERSION == 2 */
		CHANGE_SCISSOR_ENABLE(false);
		#if CHECK_GL_ERROR
			err = GetFskGLError();
			PRINT_IF_ERROR(err, __LINE__, "setting FskGLRectangleFill state");
			BAIL_IF_ERR(err);
		#endif /* CHECK_GL_ERROR */

		MakeTexRectVertices(r);

		if ((mode != kFskGraphicsModeColorize) && (blendLevel >= 255)) {
			#if GLES_VERSION < 2
				glColor4ub(color->r, color->g, color->b, color->a);
			#endif /* GLES_VERSION < 2 */
			CHANGE_BLEND_ENABLE(false);
			glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
		} else {
			if (mode != kFskGraphicsModeColorize) {					/* Blend */
				#if defined(GL_VERSION_1_1) || defined(GL_VERSION_2_0) || defined(GL_ES_VERSION_2_0)
					float blendFactor = UCHAR_TO_FLOAT_COLOR(blendLevel);
					#if GLES_VERSION < 2
						glColor4ub(color->r, color->g, color->b, color->a);
					#endif /* GLES_VERSION < 2 */
					glBlendColor(blendFactor, blendFactor, blendFactor, blendFactor);
					CHANGE_BLEND_FUNC(GL_CONSTANT_ALPHA, GL_ONE_MINUS_CONSTANT_ALPHA);
				#else /* !GL_VERSION_ES_CM_1_0 */
					#warning Blending will not be correct for alpha using GL1.
					#if GLES_VERSION < 2
						glColor4ub(color->r, color->g, color->b, blendLevel);
					#endif /* GLES_VERSION < 2 */
					CHANGE_BLEND_FUNC(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				#endif /* GL_VERSION_ES_CM_1_0 */
			}
			else {													/* Colorize */
				#if GLES_VERSION < 2
					glColor4ub(color->r, color->g, color->b, color->a);
				#endif /* GLES_VERSION < 2 */
				CHANGE_BLEND_FUNC(GL_ZERO, GL_SRC_COLOR);
			}
			glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
		}
	}

	#if CHECK_GL_ERROR
		err = GetFskGLError();
		PRINT_IF_ERROR(err, __LINE__, "glDrawArrays");
	#endif /* CHECK_GL_ERROR */
	#ifdef FLUSH_OFTEN
		glFlush();
	#endif /* FLUSH_OFTEN */

bail:
	PRINT_IF_ERROR(err, __LINE__, "FskGLRectangleFill");
	return err;
}


/********************************************************************************
 * FskGLBitmapDraw
 ********************************************************************************/

#undef FskGLBitmapDraw
FskErr FskGLBitmapDraw(
	FskConstBitmap					srcBM,
	FskConstRectangle				srcRect,	/* Map this rect... */
	FskBitmap						dstBM,
	FskConstRectangle				dstRect,	/* ...to this rect */
	FskConstRectangle				dstClip,	/* But clip thuswise */
	FskConstColorRGBA				opColor,
	UInt32							mode,
	FskConstGraphicsModeParameters	modeParams	/* We get blend level and tint color from here */
) {
	FskErr				err				= kFskErrNone;
	FskGLPort			glPort			= NULL;
	GLTextureRecord		jitTexRec;
	FskRectangleRecord	sRect, dRect;

	#ifdef LOG_PARAMETERS
		LogGLBitmapDraw(srcBM, srcRect, dstBM, dstRect, dstClip, opColor, mode, modeParams);
	#endif /* LOG_PARAMETERS */

	#ifdef SUPPORT_YUV420i
		if (srcBM->pixelFormat == kFskBitmapFormatYUV420i) {
			FskBitmap iBM = NULL;
			LOGE("GL was given YUV420i, but can only handle YUV420 or UYVY - converting to YUV420");
			if (kFskErrNone == (err = FskBitmapNew(srcBM->bounds.width, srcBM->bounds.height, kFskBitmapFormatYUV420, &iBM))) {
				if (kFskErrNone == (err = ConvertYUV420iYUV420(srcBM, iBM)))
					err = FskGLBitmapDraw(iBM, srcRect, dstBM, dstRect, dstClip, opColor, mode, modeParams);
				FskBitmapDispose(iBM);
			}
			return err;
		}
	#endif /* SUPPORT_YUV420i */

	FskMemSet(&jitTexRec, 0, sizeof(jitTexRec));
	BAIL_IF_ERR(err = FskGLDstPort(dstBM, &glPort));
	if (!srcRect)
		srcRect = &srcBM->bounds;
	BAIL_IF_ERR(err = ComputeTransformationSrcDstRects(glPort, &srcBM->bounds, srcRect, dstBM, dstRect, &sRect, &dRect));
	if (dstClip && FskRectangleContainsRectangle(dstClip, &dRect))
		dstClip = NULL;
	BAIL_IF_ERR(err = SetupBlit(srcBM, &sRect, glPort, dstClip, opColor, mode, modeParams, &jitTexRec, NULL));

	MakeTexRectVertices(&dRect);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);													/* Draw quad */

	#if 0
	{	GLTexVertex *vertex = (GLTexVertex*)(gGLGlobalAssets.coordinates);
		LOGD("{");
		for (err = 0; err < 4; ++err)
			LOGD("\t(%.1f, %.1f, %.3f, %.3f)", vertex[0].x, vertex[0].y, vertex[0].u, vertex[0].v);
		LOGD("}");
	}
	#endif /* 0 */
	#if CHECK_GL_ERROR
		err = GetFskGLError();
	#endif /* CHECK_GL_ERROR */
	#ifdef FLUSH_OFTEN
		glFlush();
	#endif /* FLUSH_OFTEN */

bail:
	FreeEphemeralTextureObjects(&jitTexRec);
	if (err == kFskErrNothingRendered)
		err = kFskErrNone;
	PRINT_IF_ERROR(err, __LINE__, "FskGLBitmapDraw");
	return err;
}


/********************************************************************************
 * FskGLScaleOffsetBitmap
 ********************************************************************************/

#undef FskGLScaleOffsetBitmap
FskErr FskGLScaleOffsetBitmap(
	FskConstBitmap					srcBM,
	FskConstRectangle				srcRect,
	FskBitmap						dstBM,
	FskConstRectangle				dstClip,
	const struct FskScaleOffset		*scaleOffset,
	FskConstColorRGBA				opColor,
	UInt32							mode,
	FskConstGraphicsModeParameters	modeParams
) {
	FskErr				err				= kFskErrNone;
	FskGLPort			glPort			= NULL;
	GLTextureRecord		jitTexRec;

	#ifdef LOG_PARAMETERS
		LogGLScaleOffsetBitmap(srcBM, srcRect, dstBM, dstClip, scaleOffset, opColor, mode, modeParams);
	#endif /* LOG_PARAMETERS */

	FskMemSet(&jitTexRec, 0, sizeof(jitTexRec));
	BAIL_IF_ERR(err = FskGLDstPort(dstBM, &glPort));
	if (!srcRect)
		srcRect = &srcBM->bounds;
	BAIL_IF_ERR(err = SetupBlit(srcBM, srcRect, glPort, dstClip, opColor, mode, modeParams, &jitTexRec, NULL));

{	FskRectangleRecord	subRect;
	FskRectangleSet(&subRect, 0, 0, srcRect->width, srcRect->height);
	ScaleOffsetRectVertices(scaleOffset, &subRect);					/* Set the quad vertices relative to srcRect */
}
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);							/* Draw quad */

	#if CHECK_GL_ERROR
		err = GetFskGLError();
	#endif /* CHECK_GL_ERROR */
	#ifdef FLUSH_OFTEN
		glFlush();
	#endif /* FLUSH_OFTEN */

bail:
	FreeEphemeralTextureObjects(&jitTexRec);
	if (err == kFskErrNothingRendered)
		err = kFskErrNone;
	PRINT_IF_ERROR(err, __LINE__, "FskGLScaleOffsetBitmap");
	return err;
}


/********************************************************************************
 * FskGLTileBitmap
 *
 * Ideally, we would use a repeating texture. Unfortunately, this has the restriction
 * that it be powers of two in each dimension, which we cannot guarantee in general.
 *
 * Another option is to use a triangle strip and set the texture coordinates for each vertex.
 *
 * In the general case, we have wraparound. So it is necessary to augment the source texture with an extra row and column,
 * to allow interpolation to occur.
 ********************************************************************************/

#undef FskGLTileBitmap
FskErr FskGLTileBitmap(
	FskConstBitmap					srcBM,
	FskConstRectangle				srcRect,
	FskBitmap						dstBM,
	FskConstRectangle				dstRect,
	FskConstRectangle				dstClip,
	FskFixed						scale,
	FskConstColorRGBA				opColor,
	UInt32							mode,
	FskConstGraphicsModeParameters	modeParams
) {
	FskErr				err			= kFskErrNone;
	FskGLPort			glPort		= NULL;
	GLTextureRecord		jitTexRec;
	GLTextureRecord		*tx;
	FskRectangleRecord	sRect, dRect, texRect, srcBounds;
	Boolean				srcIsPremultiplied;

	FskMemSet(&jitTexRec, 0, sizeof(jitTexRec));
	BAIL_IF_ERR(err = FskGLDstPort(dstBM, &glPort));

	#ifdef LOG_PARAMETERS
		LogGLTileBitmap(srcBM, srcRect, dstBM, dstRect, dstClip, scale, opColor, mode, modeParams);
	#endif /* LOG_PARAMETERS */

	if (!srcRect)
		srcRect = &srcBM->bounds;

	if (!srcBM->hasAlpha && (kFskGraphicsModeAlpha == (mode & kFskGraphicsModeMask)))
		mode = kFskGraphicsModeCopy | (mode & ~kFskGraphicsModeMask);

	srcIsPremultiplied = IsBitmapPremultiplied(srcBM);
	srcBounds = srcBM->bounds;
	if (NULL != (tx = GetBitmapTexture(srcBM))) {					/* Use the cached texture if available, ... */
		srcBM = NULL;												/* ... not the bitmap */
		FskRectangleSet(&texRect, 0, 0, tx->bounds.width, tx->bounds.height);		/* Coordinate frame is that of the texture */
	}
	else {															/* Uncached texture */
		AllocateEphemeralTextureObject(&jitTexRec.name);
		#ifdef LOG_TEXTURE_LIFE
			LOGD("Allocated ephemeral texture tile #%u", jitTexRec.name);
		#endif /* LOG_TEXTURE_LIFE */
		tx = &jitTexRec;
		texRect = srcBM->bounds;									/* Coordinate frame is that of the bitmap */
	}


	#if GLES_VERSION == 2
		if ((mode & kFskGraphicsModeMask) != kFskGraphicsModeColorize)
			opColor = NULL;
		if (gGLGlobalAssets.hasHighPrecisionShaderFloat) {			/* High enough precision to use the tile shader */
			BAIL_IF_ERR(err = FskGLUseProgram(gGLGlobalAssets.tileBitmap.program));
			FskSetProgramOpColor(gGLGlobalAssets.tileBitmap.opColor, &gGLGlobalAssets.blitContext->tileBitmap.lastParams, opColor, srcIsPremultiplied, modeParams);
			CHANGE_SHADER_MATRIX(tileBitmap);
		}
		else {														/* Mesh the tiles */
			BAIL_IF_ERR(err = FskGLUseProgram(gGLGlobalAssets.drawBitmap.program));
			FskSetProgramOpColor(gGLGlobalAssets.drawBitmap.opColor, &gGLGlobalAssets.blitContext->drawBitmap.lastParams, opColor, srcIsPremultiplied, modeParams);
			CHANGE_SHADER_MATRIX(drawBitmap);
		}
	#endif /* GLES_VERSION == 2 */

	CHANGE_TEXTURE(tx->name);										/* Make sure that all of the state is set for this texture */
	BAIL_IF_ERR(err = ComputeTransformationSrcDstRects(glPort, &srcBounds, srcRect, dstBM, dstRect, &sRect, &dRect));

	/* Set clip rect */
	if (dstClip && !FskRectangleContainsRectangle(dstClip, dstRect))
		CHANGE_SCISSOR(glPort, dstClip->x, dstClip->y, dstClip->width, dstClip->height);			/* This also enables scissoring */
	else
		CHANGE_SCISSOR_ENABLE(false);
	err = SetTexturedDrawState(tx, opColor, mode, modeParams, srcIsPremultiplied);
	#if GL_DEBUG
		PRINT_IF_ERROR(err, __LINE__, "SetTexturedDrawState");
//		PrintTextureIfError(err, __LINE__, "SetTexturedDrawState", jitTexRec.name, tx->name);
	#endif /* GL_DEBUG */
	BAIL_IF_ERR(err);

	err = TileBitmapRect(scale, srcBM, &sRect, &dRect, tx, glPort);

bail:
	FreeEphemeralTextureObject(jitTexRec.name);
	if (err == kFskErrNothingRendered)
		err = kFskErrNone;
	PRINT_IF_ERROR(err, __LINE__, "FskGLTileBitmap");
	return err;
}


/********************************************************************************
 * FskGLTransferAlphaBitmap
 ********************************************************************************/

#undef FskGLTransferAlphaBitmap
FskErr FskGLTransferAlphaBitmap(
	FskConstBitmap					srcBM,
	FskConstRectangle				srcRect,
	FskBitmap						dstBM,
	FskConstPoint					dstLocation,
	FskConstRectangle				dstClip,
	FskConstColorRGBA				fgColor,
	FskConstGraphicsModeParameters	modeParams
) {
	FskErr				err			= kFskErrNone;
	FskGLPort			glPort		= NULL;
	GLTextureRecord		jitTexRec;
	GLTextureRecord		*tx;
	FskRectangleRecord	sRect, dRect;

	#ifdef LOG_PARAMETERS
		LogGLTransferAlphaBitmap(srcBM, srcRect, dstBM, dstLocation, dstClip, fgColor, modeParams);
	#endif /* LOG_PARAMETERS */

	FskMemSet(&jitTexRec, 0, sizeof(jitTexRec));											/* Assure that jitTexRec.name is set, in case we bail */

	BAIL_IF_ERR(err = FskGLDstPort(dstBM, &glPort));

	if (dstLocation)	{	dRect.x =  dstLocation->x;	dRect.y =  dstLocation->y;	}
	else				{	dRect.x = srcBM->bounds.x;	dRect.y = srcBM->bounds.y;	}
	if (srcRect) {												/* If a srcRect was given, make sure it lies within the srcBM->bounds */
		SInt32 i;
		sRect = *srcRect;
		if ((i = srcBM->bounds.x - sRect.x) > 0) { sRect.x += i; sRect.width  -= i; dRect.x += i; }			/* Special left clip */
		if ((i = srcBM->bounds.y - sRect.y) > 0) { sRect.y += i; sRect.height -= i; dRect.y += i; }			/* Special top  clip */
		BAIL_IF_FALSE(FskRectangleIntersect(&srcBM->bounds, &sRect, &sRect), err, kFskErrNothingRendered);	/* Right and bottom clip */
	}
	else {																					/* Otherwise, use the bounds of the source bitmap */
		sRect = srcBM->bounds;
	}
	dRect.width  = sRect.width;
	dRect.height = sRect.height;

	if (dstClip && !FskRectangleContainsRectangle(dstClip, &dRect))							/* Use the hardware to do the clipping */
		CHANGE_SCISSOR(glPort, dstClip->x, dstClip->y, dstClip->width, dstClip->height);	/* This also enables scissoring */
	else
		CHANGE_SCISSOR_ENABLE(false);

	if (NULL != (tx = GetBitmapTexture(srcBM))) {											/* Use the cached texture if available */
		srcBM = NULL;
	}
	else {
		AllocateEphemeralTextureObject(&jitTexRec.name);
		#ifdef LOG_TEXTURE_LIFE
			LOGD("Allocated ephemeral texture alpha #%u", jitTexRec.name);
		#endif /* LOG_TEXTURE_LIFE */
		tx = &jitTexRec;
	}

	#if GLES_VERSION < 2
		glColor4ub(fgColor->r, fgColor->g, fgColor->b, (modeParams ? (modeParams->blendLevel * fgColor->a + 127) / 255: fgColor->a));
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	#else /* GLES_VERSION == 2 */
		BAIL_IF_ERR(err = FskGLUseProgram(gGLGlobalAssets.transferAlpha.program));
		FskSetProgramOpColor(gGLGlobalAssets.transferAlpha.opColor, &gGLGlobalAssets.blitContext->transferAlpha.lastParams, fgColor, false, modeParams);
		#if 0 /* GL_DEBUG */
			if (fgColor->a < 255U)
				LOGD("GLTransferAlphaBitmap: non-opaque color: [%2x %2x %2x %2x]", fgColor->r, fgColor->g, fgColor->b, fgColor->a);
		#endif /* GL_DEBUG */

		CHANGE_SHADER_MATRIX(transferAlpha);
	#endif /* GLES_VERSION == 2 */
	SET_TEXTURE(tx->name);														/* Make sure that all of the state is set for this texture */

	if (GL_CLAMP_TO_EDGE != tx->wrapMode) {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		tx->wrapMode = GL_CLAMP_TO_EDGE;
	}
	if (gGLGlobalAssets.allowNearestBitmap) {
		if (GL_NEAREST != tx->filter) {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			tx->filter = GL_NEAREST;
		}
	}
	CHANGE_BLEND_FUNC_SEPARATE(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA, GL_ONE);

	MakeTexRectVertices(&dRect);
	err = TransformTexturedRectangle(srcBM, &sRect, tx);
	PRINT_IF_ERROR(err, __LINE__, "TransformTexturedRectangle");

bail:
	FreeEphemeralTextureObject(jitTexRec.name);
	#if GLES_VERSION < 2
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
	#endif /* GLES_VERSION */
	if (err == kFskErrNothingRendered)
		err = kFskErrNone;
	PRINT_IF_ERROR(err, __LINE__, "FskGLTransferAlphaBitmap");
	return err;
}
#ifdef GL_TRACE
//	#define FskGLTransferAlphaBitmap(srcBM,srcRect,dstBM,dstLocation,dstClip,fgColor,modeParams) (GL_TRACE_LOG("FskGLTransferAlphaBitmap"), FskGLTransferAlphaBitmap(srcBM,srcRect,dstBM,dstLocation,dstClip,fgColor,modeParams))
#endif /* GL_TRACE */


/********************************************************************************
 * FskGLTextGlyphsLoad
 ********************************************************************************/

#undef FskGLTextGlyphsLoad
FskErr FskGLTextGlyphsLoad(
	struct FskTextEngineRecord		*fte,
	const char						*text,
	UInt32							textLen,
	UInt32							textSize,
	UInt32							textStyle,
	const char						*fontName,
	struct FskTextFormatCacheRecord	*cache
) {
	FskErr				err			= kFskErrNone;
	FskGLTypeFace		typeFace	= NULL;

	BAIL_IF_NULL(fte, err, kFskErrInvalidParameter);

	typeFace = LookForTypeface(fontName, textSize, textStyle, fte, cache);
	if (NULL == typeFace) {
		err = FskGLTypeFaceNew(fontName, textSize, textStyle, fte, cache, &typeFace);
		BAIL_IF_ERR(err);
		BAIL_IF_NULL(typeFace, err, kFskErrNotFound);
	}

	BAIL_IF_ERR(err = CheckTypeFaceGlyphStrikes(text, textLen, typeFace));
bail:
	return err;
}
#ifdef GL_TRACE
	#define FskGLTextGlyphsLoad(fte,text,textLen,textSize,textStyle,fontName,cache) (GL_TRACE_LOG("FskGLTextGlyphsLoad"), FskGLTextGlyphsLoad(fte,text,textLen,textSize,textStyle,fontName,cache))
#endif /* GL_TRACE */


/********************************************************************************
 * FskGLTextGlyphsFlush
 ********************************************************************************/

FskErr FskGLTextGlyphsFlush(void)
{
	FskGLTypeFace face;

	for (face = gGLGlobalAssets.typeFaces; face != NULL; face = face->next) {
		if (face->dirty) {
			if (face->bm->glPort)
				FskGLUpdateSource(face->bm);
			else
				FskGLAccelerateBitmapSource(face->bm);
			face->dirty = false;
		}
	}

	return kFskErrNone;
}


/********************************************************************************
 * FskGLTextGlyphsUpdateTexture
 ********************************************************************************/

#undef FskGLTextGlyphsUpdateTexture
FskErr FskGLTextGlyphsUpdateTexture(
	struct FskTextEngineRecord		*fte,
	UInt32							textSize,
	UInt32							textStyle,
	const char						*fontName,
	struct FskTextFormatCacheRecord	*cache
) {
	FskErr				err			= kFskErrNone;
	FskGLTypeFace		typeFace	= NULL;

	BAIL_IF_NULL(fte, err, kFskErrInvalidParameter);

	BAIL_IF_NULL(typeFace = LookForTypeface(fontName, textSize, textStyle, fte, cache), err, kFskErrNameLookupFailed);

	if (typeFace->dirty) {
		BAIL_IF_ERR(err = FskGLUpdateSource(typeFace->bm));
		typeFace->dirty = false;
	}

bail:
	return err;
}
#ifdef GL_TRACE
#define FskGLTextGlyphsUpdateTexture(fte, textSize, textStyle, fontName, cache) (GL_TRACE_LOG("FskGLTextGlyphsUpdateTexture"), FskGLTextGlyphsUpdateTexture(fte, textSize, textStyle, fontName, cache))
#endif /* GL_TRACE */



/********************************************************************************
 * FskGLTextBox
 ********************************************************************************/

#undef FskGLTextBox
FskErr FskGLTextBox(
	struct FskTextEngineRecord		*fte,
	FskBitmap						dstBM,
	const char						*text,
	UInt32							textLen,
	FskConstRectangle				dstRect,
	FskConstRectangleFloat			dstRectFloat,
	FskConstRectangle				dstClip,
	FskConstColorRGBA				color,
	UInt32							blendLevel,
	UInt32							textSize,
	UInt32							textStyle,
	UInt16							hAlign,
	UInt16							vAlign,
	const char						*fontName,
	struct FskTextFormatCacheRecord	*cache
) {
	FskErr				err			= kFskErrNone;
	float				*xy			= NULL,
						*uv			= NULL;
	FskGLPort			glPort		= NULL;
	FskGLTypeFace		typeFace	= NULL;
	UInt32				numPts		= 0;
	GLTextureRecord		*tx;
	FskGraphicsModeParametersRecord modeParams;
	FskRectangleRecord bounds;

	#if defined(LOG_PARAMETERS) || defined(LOG_TEXT)
		LogGLTextBox(fte, dstBM, text, textLen, dstRect, dstClip, color, blendLevel, textSize, textStyle, hAlign, vAlign, fontName, cache);
	#endif /* LOG_PARAMETERS */

	BAIL_IF_NULL(fte, err, kFskErrInvalidParameter);

	typeFace = LookForTypeface(fontName, textSize, textStyle, fte, cache);
	if (NULL == typeFace)
		err = FskGLTypeFaceNew(fontName, textSize, textStyle, fte, cache, &typeFace);
	BAIL_IF_ERR(err);
	BAIL_IF_NULL(typeFace, err, kFskErrNotFound);

	BAIL_IF_ERR(err = FskGLTextGetBounds(fte, NULL, text, textLen, textSize, textStyle, fontName, &bounds, cache));

	BAIL_IF_ERR(err = FskGLDstPort(dstBM, &glPort));
	tx = GetBitmapTexture(typeFace->bm);
	#ifdef LOG_TEXT
		if (tx == NULL)
			LOGD("FskGLTextBox: typeface texture is NULL");
		else if (0 == tx->name || GL_TEXTURE_UNLOADED == tx->name)
			LOGD("FskGLTextBox: typeface tx->name=%d", tx->name);
	#endif /* LOG_TEXT */
	if (NULL == tx) {
		//@@ we get here sometimes, and we never should...
		#ifdef LOG_TEXT
			LOGD("FskGLTextBox: glPort=%p, glPort->texture.name=%d", dstBM->glPort, dstBM->glPort ? dstBM->glPort->texture.name : 0);
		#endif /* LOG_TEXT */
		BAIL_IF_ERR(err = FskGLAccelerateBitmapSource(typeFace->bm));
		tx = GetBitmapTexture(typeFace->bm);
		#ifdef LOG_TEXT
			if (tx == NULL)
				LOGD("FskGLTextBox second attempt: typeface texture is NULL");
			else if (0 == tx->name || GL_TEXTURE_UNLOADED == tx->name)
				LOGD("FskGLTextBox second attempt: typeface tx->name=%d", tx->name);
		#endif /* LOG_TEXT */
	}
	BAIL_IF_NULL(tx, err, kFskErrNotAccelerated);

	CHANGE_TEXTURE(tx->name);											/* Make sure that all of the state is set for this texture */

// Glyphs already loaded in call to FskGLTextGetBounds above
//	#define LOAD_MISSING_GLYPHS											/**< Make sure all glyph strikes are present before typesetting the string. */
	#ifdef LOAD_MISSING_GLYPHS
		BAIL_IF_ERR(err = CheckTypeFaceGlyphStrikes(text, textLen, typeFace));
	#endif /* LOAD_MISSING_GLYPHS */

	if (dstClip && !FskRectangleContainsRectangle(dstClip, dstRect))
		CHANGE_SCISSOR(glPort, dstClip->x, dstClip->y, dstClip->width, dstClip->height);			/* This also enables scissoring */
	else
		CHANGE_SCISSOR_ENABLE(false);

	modeParams.dataSize = sizeof(modeParams);
	modeParams.blendLevel = blendLevel;

	#if GLES_VERSION < 2
		//glColor4ub(color->r, color->g, color->b, (modeParams ? (modeParams->blendLevel * color->a + 127) / 255: color->a));
		glColor4ub(color->r, color->g, color->b, (modeParams.blendLevel * color->a + 127) / 255);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		glEnable(GL_TEXTURE_2D);
	#else /* GLES_VERSION == 2 */
		BAIL_IF_ERR(err = FskGLUseProgram(gGLGlobalAssets.transferAlpha.program));
		FskSetProgramOpColor(gGLGlobalAssets.transferAlpha.opColor, &gGLGlobalAssets.blitContext->transferAlpha.lastParams, color, false, &modeParams);
		#if GL_DEBUG
			if (color->a < 255U)
				LOGD("GLTextBox: non-opaque color: [%2x %2x %2x %2x]", color->r, color->g, color->b, color->a);
		#endif /* GL_DEBUG */

		CHANGE_SHADER_MATRIX(transferAlpha);
	#endif /* GLES_VERSION == 2 */

	if (GL_CLAMP_TO_EDGE != tx->wrapMode) {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		tx->wrapMode = GL_CLAMP_TO_EDGE;
	}
	if (gGLGlobalAssets.allowNearestBitmap) {
		if (GL_NEAREST != tx->filter) {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			tx->filter = GL_NEAREST;
		}
	}
	CHANGE_BLEND_FUNC_SEPARATE(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA, GL_ONE);
	modeParams.blendLevel = blendLevel;

	if (dstRectFloat) {
		BAIL_IF_ERR(err = NewTextMesh(text, textLen, typeFace, tx, dstRectFloat->x, dstRectFloat->y, dstRectFloat->width, dstRectFloat->height, bounds.height, TEXVERTEX_STRIDE, hAlign, vAlign, textStyle, &numPts, &xy, &uv));
	}
	else {
		BAIL_IF_ERR(err = NewTextMesh(text, textLen, typeFace, tx, (float)dstRect->x, (float)dstRect->y, (float)dstRect->width, (float)dstRect->height, bounds.height, TEXVERTEX_STRIDE, hAlign, vAlign, textStyle, &numPts, &xy, &uv));
	}

	BAIL_IF_ZERO(numPts, err, kFskErrNothingRendered);
	#ifdef MESH_WITH_TRIANGLES
		glDrawArrays(GL_TRIANGLES, 0, numPts);
	#else /* MESH_WITH_TRIANGLE_STRIP */
		glDrawArrays(GL_TRIANGLE_STRIP, 0, numPts);
	#endif /* MESH_WITH_TRIANGLE_STRIP */
	#if CHECK_GL_ERROR
		err = GetFskGLError();
	#endif /* CHECK_GL_ERROR */

bail:
	#if GLES_VERSION < 2
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
		glDisable(GL_TEXTURE_2D);
	#endif /* GLES_VERSION */
	if (err == kFskErrNothingRendered)
		err = kFskErrNone;
	PRINT_IF_ERROR(err, __LINE__, "FskGLTextBox");
	return err;
}


/********************************************************************************
 * FskGLTextGetBounds
 ********************************************************************************/

#undef FskGLTextGetBounds
FskErr FskGLTextGetBounds(
	struct FskTextEngineRecord	*fte,
	FskBitmap			bits,
	const char			*text,
	UInt32				textLen,
	UInt32				textSize,
	UInt32				textStyle,
	const char			*fontName,
	FskRectangle		bounds,
	struct FskTextFormatCacheRecord	*cache
) {
	FskErr			err			= kFskErrNone;
#if !USE_GLYPH
	const char		*endText	= text + textLen;
#endif /* !USE_GLYPH */
	FskGLTypeFace	typeFace;
	FskRectangleRecord	r;

	FskRectangleSetEmpty(&r);
	if (bits) {}
	BAIL_IF_NULL(typeFace = LookForTypeface(fontName, textSize, textStyle, fte, cache), err, kFskErrNotFound);
	BAIL_IF_ERR(err = CheckTypeFaceGlyphStrikes(text, textLen, typeFace));

#if USE_GLYPH
	/* no use of the bounding rect */
#else /* !USE_GLYPH */
	while (text < endText) {
		UInt16			uniCode	= UTF8ToUnicodeChar(&text, NULL);						/* This automatically advances */
		UInt16			glyph	= (uniCode < typeFace->glyphTabSize) ? typeFace->glyphTab[uniCode] : 0;
		FskGlyphStrike	strike	= &typeFace->strikes[glyph];
		r.width += strike->width;
		if (r.height < strike->height)
			r.height = strike->height;
	}
#endif /* !USE_GLYPH */
	*bounds = r;

bail:
	PRINT_IF_ERROR(err, __LINE__, "FskGLTextGetBounds");
	return err;
}


/********************************************************************************
 * FskGLPerspectiveTransformBitmap
 ********************************************************************************/

#define maxTexCoords	16	/**< The maximum number of points or texture coordinates allowed in FskGLPerspectiveTransformBitmap. */

#undef FskGLPerspectiveTransformBitmap
FskErr FskGLPerspectiveTransformBitmap(
	FskConstBitmap					srcBM,
	UInt32							numPoints,
	const FskFixedPoint2D			*points,
	FskBitmap						dstBM,
	FskConstRectangle				dstClip,
	const float						M[3][3],
	FskConstColorRGBA				opColor,
	UInt32							mode,
	FskConstGraphicsModeParameters	modeParams
) {
	FskErr				err			= kFskErrNone;
	FskGLPort			glPort		= NULL;
	GLTextureRecord		jitTexRec;
	GLTexVertex			*vertices	= (GLTexVertex*)(gGLGlobalAssets.coordinates);
	ShaderState			*shaderState	= NULL;
	GLTextureRecord		*tx;
	float				P[3][3], N[3][3];
	UInt32				i;

	#if GLES_VERSION == 2
		float			viewMtx[3][3];
		FskMemCopy(viewMtx[0], gGLGlobalAssets.matrix[0], sizeof(viewMtx));							/* Save the current matrix, so we can restore it afterward */
	#endif /* GLES_VERSION == 2 */

	FskMemSet(&jitTexRec, 0, sizeof(jitTexRec));
	BAIL_IF_ERR(err = FskGLDstPort(dstBM, &glPort));

	/* Set model matrix */
	P[0][0] =  1.f;		P[2][2] = 1.f;
	P[0][1] =  0.f;		P[0][2] = 0.f;		P[1][0] = 0.f;		P[1][2] = 0.f;		P[2][0] = 0.f;
	P[1][1] = -1.f;																					/* Flip Y in src */
	#ifdef PIXELS_ARE_POINTS
		P[2][1] = (float)(srcBM->bounds.height-1);
	#else /* PIXELS_ARE_SQUARES */
		P[2][1] = (float)(srcBM->bounds.height);
	#endif /* PIXELS_ARE_SQUARES */

	SLinearTransform(P[0], M[0], N[0], 3, 3, 3);
	#if GLES_VERSION < 2
	{	float G[4][4];
		glMatrixMode(GL_MODELVIEW);
		G[0][0] = N[0][0];	G[0][1] = N[0][1];	G[0][2] = 0;	G[0][3] = N[0][2];
		G[1][0] = N[1][0];	G[1][1] = N[1][1];	G[1][2] = 0;	G[1][3] = N[1][2];
		G[2][0] = 0;		G[2][1] = 0;		G[2][2] = 0;	G[2][3] = 0;
		G[3][0] = N[2][0];	G[3][1] = N[2][1];	G[3][2] = 0;	G[3][3] = N[2][2];
		glLoadMatrixf(G[0]);
		glEnable(GL_TEXTURE_2D);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);									/* Transfer mode is copy */
	}
	#else /* GLES_VERSION == 2 */
	{	/* Concatenate view matrix */
		SLinearTransform(N[0], viewMtx[0], gGLGlobalAssets.matrix[0], 3, 3, 3);
		++gGLGlobalAssets.matrixSeed;																/* Set to a different seed to force updating of the matrix */
	}
	#endif /* GLES_VERSION == 2 */

	BAIL_IF_ERR(err = SetupBlit(srcBM, NULL, glPort, dstClip, opColor, mode, modeParams, &jitTexRec, &shaderState));

	/* Set the polygon to be rendered */
	BAIL_IF_FALSE(numPoints <= maxTexCoords, err, kFskErrTooMany);
	if ((numPoints > 0) && (points != NULL)) {														/* Polygon is specified */
		#if GL_COORD_TYPE_ENUM == GL_FLOAT
			float			nrm = 1.f / 65536.f;
			const FskFixed	*px;
			GLTexVertex		*pf;
			for (i = numPoints, pf = vertices, px = &points->x; i--; px += 2, ++pf) {
				pf->x = px[0] * nrm;
				pf->y = px[1] * nrm;
			}
		#elif GL_COORD_TYPE_ENUM == GL_FIXED
			const FskFixed	*pfr;
			GLTexVertex		*pto;
			for (i = numPoints, pto = vertices, pfr = &points->x; i--; pfr += 2, ++pto) {
				pto->x = pfr[0];
				pto->y = pfr[1];
			}
		#else /* GL_COORD_TYPE_ENUM */
			#error Unknown GL Coordinate typedef
		#endif /* GL_COORD_TYPE_ENUM */
	}

	else {		/* No points -- use the whole rectangle */
		MakeRectPolygon(srcBM->bounds.width, srcBM->bounds.height);
		numPoints = 4;
	}

	tx = (jitTexRec.name != 0) ? &jitTexRec : &srcBM->glPort->texture;
	MakeTexCoords(numPoints, &srcBM->bounds, tx);
	#if GLES_VERSION == 2
		if (shaderState == &gGLGlobalAssets.blitContext->yuv422) {				/* If we are using the 422 shader, use chrominance coordinates */
			GLTexVertex *vertex = (GLTexVertex*)(gGLGlobalAssets.coordinates);
			#if GL_COORD_TYPE_ENUM == GL_FLOAT
				float ru = .5f / srcBM->bounds.width;							/* Half a pixel offset between this shader's coordinates and GL's */
				float rv = .5f / srcBM->bounds.height;
			#else	/* GL_COORD_TYPE_ENUM == GL_FIXED */
				FskFixed ru = (1 << 15) / srcBM->bounds.width;					/* Half a pixel offset between this shader's coordinates and GL's */
				FskFixed rv = (1 << 15) / srcBM->bounds.height;
			#endif	/* GL_COORD_TYPE_ENUM == GL_FIXED */
			for (i = numPoints; i--; vertex++) {
				#if GL_COORD_TYPE_ENUM == GL_FLOAT
					vertex->u *= .5f;											/* Horizontal 4:2:2 chrominance coordinate is half that of luminance */
				#else				/* GL_COORD_TYPE_ENUM == GL_FIXED */
					vertex->u >>= 1;											/* Horizontal 4:2:2 chrominance coordinate is half that of luminance */
				#endif /* GL_COORD_TYPE_ENUM == GL_FIXED */
				vertex->u -= ru;
				vertex->v -= rv;
			}
		}
	#endif /* GLES_VERSION == 2 */


	/* Set the polygon to be rendered (prior to transformation). */
	glDrawArrays(GL_TRIANGLE_FAN, 0, numPoints);
	#if CHECK_GL_ERROR
		err = GetFskGLError();
		PRINT_IF_ERROR(err, __LINE__, "glDrawArrays");
		BAIL_IF_ERR(err);
	#endif /* CHECK_GL_ERROR */

	#ifdef FLUSH_OFTEN
		glFlush();
	#endif /* FLUSH_OFTEN */

bail:
	PRINT_IF_ERROR(err, __LINE__, "FskGLPerspectiveTransformBitmap");
	FreeEphemeralTextureObjects(&jitTexRec);
	#if GLES_VERSION < 2
		glDisable(GL_TEXTURE_2D);
		glLoadIdentity();
	#else /* GLES_VERSION == 2 */
		FskMemCopy(gGLGlobalAssets.matrix[0], viewMtx[0], sizeof(viewMtx));						/* Restore the current matrix */
		--gGLGlobalAssets.matrixSeed;															/* Restore the matrix seed */
		if (shaderState)
			shaderState->matrixSeed = gGLGlobalAssets.matrixSeed | 0x80000000;					/* Set to a maximally different seed to force updating of the matrix */
	#endif /* GL 1 */
	return err;
}


/********************************************************************************
 * FskGLProjectBitmap
 ********************************************************************************/

#undef FskGLProjectBitmap
FskErr FskGLProjectBitmap(
	FskConstBitmap					srcBM,
	FskConstRectangle				srcRect,
	FskBitmap						dstBM,
	FskConstRectangle				dstClip,
	const float						M[3][3],
	FskConstColorRGBA				opColor,
	UInt32							mode,
	FskConstGraphicsModeParameters	modeParams
) {
	FskFixedPoint2D		srcPts[4];
	FskErr				err;

	#if defined(LOG_PARAMETERS)
		LogProjectBitmap(srcBM, srcRect, dstBM, dstClip, M, opColor, mode, modeParams);
	#endif /* LOG_PARAMETERS */

	if (!srcRect) {
		err = FskGLPerspectiveTransformBitmap(srcBM, 0, NULL, dstBM, dstClip, M, opColor, mode, modeParams);
	}
	else {
		FskRectangleRecord	srcBounds;
		if (srcRect)	FskRectangleIntersect(srcRect, &srcBM->bounds, &srcBounds);
		srcPts[0].x = srcPts[3].x = FskIntToFixed(srcBounds.x);
		srcPts[0].y = srcPts[1].y = FskIntToFixed(srcBounds.y);
		srcPts[1].x = srcPts[2].x = FskIntToFixed(srcBounds.x + srcBounds.width  - 1);
		srcPts[2].y = srcPts[3].y = FskIntToFixed(srcBounds.y + srcBounds.height - 1);
		err = FskGLPerspectiveTransformBitmap(srcBM, 4, srcPts, dstBM, dstClip, M, opColor, mode, modeParams);
	}
	PRINT_IF_ERROR(err, __LINE__, "FskGLPerspectiveTransformBitmap");
	return err;
}


/********************************************************************************
 * FskGLGetCoordinatePointer
 ********************************************************************************/

FskErr FskGLGetCoordinatePointer(float **coordinates, UInt32 *coordinatesBytes) {
	FskErr err = kFskErrNone;

	BAIL_IF_FALSE(gGLGlobalAssets.isInited && gGLGlobalAssets.coordinates && gGLGlobalAssets.coordinatesBytes >= 16* sizeof(float), err, kFskErrGraphicsContext);
	if (coordinates) {
		*coordinates = gGLGlobalAssets.coordinates;
		if (coordinatesBytes)
			*coordinatesBytes	= gGLGlobalAssets.coordinatesBytes;
	} else if (coordinatesBytes) {
		if (*coordinatesBytes > gGLGlobalAssets.coordinatesBytes) {
			gGLGlobalAssets.coordinatesBytes = *coordinatesBytes = BLOCKIFY(*coordinatesBytes, 512);
			BAIL_IF_ERR(err = FskMemPtrRealloc(gGLGlobalAssets.coordinatesBytes, &gGLGlobalAssets.coordinates));
			#if GLES_VERSION == 2
				glVertexAttribPointer(VERTEX_POSITION_INDEX, 2, GL_COORD_TYPE_ENUM, GL_FALSE, sizeof(GLTexVertex), gGLGlobalAssets.coordinates + TEXVERTEX_X);
				glVertexAttribPointer(VERTEX_TEXCOORD_INDEX, 2, GL_COORD_TYPE_ENUM, GL_FALSE, sizeof(GLTexVertex), gGLGlobalAssets.coordinates + TEXVERTEX_U);
			#else /* GLES_VERSION == 1 */
				glVertexPointer  (2, GL_COORD_TYPE_ENUM, sizeof(GLTexVertex), gGLGlobalAssets.coordinates + TEXVERTEX_X);
				glTexCoordPointer(2, GL_COORD_TYPE_ENUM, sizeof(GLTexVertex), gGLGlobalAssets.coordinates + TEXVERTEX_U);
			#endif /* GLES_VERSION == 1 */
		}
	}
bail:
	return err;
}


/****************************************************************************//**
 * FskGLPortTexRectGet - get the rectangle of the texture.
 *	\param[in]	glPort	the glPort containing the texture descriptor.
 *	\param[out]	texRect	the requested rectangle.
 *	\return		kFskErrNone	if the operation was successful.
 ********************************************************************************/

#undef FskGLPortTexRectGet
FskErr FskGLPortTexRectGet(FskGLPort glPort, FskRectangle texRect) {
	FskErr err = kFskErrNone;

	if (glPort == NULL) {
		FskRectangleSetEmpty(texRect);
		BAIL(kFskErrInvalidParameter);
	}
	*texRect = glPort->texture.bounds;

bail:
	return err;
}


/****************************************************************************//**
 * FskGLDstBMRectGet - get the rectangle of the destination port.
 *	\param[in]	bm	the bitmap proxy for the GL destination.
 *	\param[out]	r	the requested rectangle.
 *	\return		kFskErrNone	if the operation was successful.
 ********************************************************************************/

#undef FskGLDstBMRectGet
FskErr FskGLDstBMRectGet(FskBitmap bm, FskRectangle r) {
	FskErr		err	= kFskErrNone;
	FskGLPort	glPort;

	if ((glPort = bm->glPort) == NULL)
		glPort = gCurrentGLPort;
	BAIL_IF_FALSE(glPort->portWidth && glPort->portHeight, err, kFskErrNotAccelerated);
	FskRectangleSet(r, 0, 0, glPort->portWidth, glPort->portHeight);
bail:
	return err;
}


/****************************************************************************//**
 * FskGLBindBMTexture - state-aware glBindTexture.
 *	\param[in]	bm		the bitmap containing the texture of interest.
 *	\param[in]	wrap	the desired warp mode { 0, GL_CLAMP_TO_EDGE, GL_REPEAT }. Set if nonzero.
 *	\param[in]	filter	the desired filtering { 0, GL_LINEAR, GL_NEAREST }. Set if nonzero.
 *	\return		kFskErrNone	if the operation was successful.
 ********************************************************************************/

#undef FskGLBindBMTexture
FskErr FskGLBindBMTexture(FskConstBitmap bm, int wrap, int filter) {
	FskGLPort	glPort;

	if ((NULL == bm) || (NULL == (glPort = bm->glPort)) || (0 == glPort->texture.name)) {
		CHANGE_TEXTURE(0);
		return kFskErrNotAccelerated;
	}

	CHANGE_TEXTURE(glPort->texture.name);
	if (wrap && (wrap != glPort->texture.wrapMode)) {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap);
	}
	glPort->texture.wrapMode = wrap;
	if (filter && (filter != glPort->texture.filter)) {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
	}
	glPort->texture.filter = filter;

	return kFskErrNone;
}


/****************************************************************************//**
 * FskGLUnbindBMTexture
 *	\param[in]	bm	the bitmap containing the texture of interest.
 *	\return		kFskErrNone	if the operation was successful.
 ********************************************************************************/

#undef FskGLUnbindBMTexture
void FskGLUnbindBMTexture(FskConstBitmap bm) {
	GLuint	texName	= (bm && bm->glPort) ? bm->glPort->texture.name : 0U;
	if (texName)
		FORGET_TEXTURE(texName);
}


/****************************************************************************//**
 * FskGLSetBlend - state aware way to set the blending function and enable.
 *	\param[in]	doBlend		if true,   enables blending, and sets the blend function.
 *							if false, disables blending. The blend function is not changed.
 *	\param[in]	srcRGB		the code used for the coefficient of the src RGB.
 *	\param[in]	dstRGB		the code used for the coefficient of the dst RGB.
 *	\param[in]	srcAlpha	the code used for the coefficient of the src alpha.
 *	\param[in]	dstAlpha	the code used for the coefficient of the src alpha.
 ********************************************************************************/

#undef FskGLSetBlend
void FskGLSetBlend(Boolean doBlend, unsigned int srcRGB, unsigned int dstRGB, unsigned int srcAlpha, unsigned int dstAlpha) {

	if (doBlend)
		CHANGE_BLEND_FUNC_SEPARATE(srcRGB, dstRGB, srcAlpha, dstAlpha);	/* This also calls CHANGE_BLEND_ENABLE() */
	else
		CHANGE_BLEND_ENABLE(doBlend);
}


/****************************************************************************//**
 * FskGLPortSetClip - state aware way to set the clipping.
 *	\param[in]	glPort		the GL port of the destination. If NULL, selects the current port.
 *	\param[in]	clipRect	the clipping rectangle. NULL turns off clipping.
 ********************************************************************************/

#undef FskGLPortSetClip
void FskGLPortSetClip(FskGLPort glPort, FskConstRectangle clipRect) {
	#if defined(LOG_PARAMETERS)
		LOGD("FskGLPortSetClip(glPort=%p clipRect=%p) while scissor(%s; %d %d %d %d)", glPort, clipRect,
			(gGLGlobalAssets.blitContext->glScissorTest ? "true" : "false"),
			gGLGlobalAssets.blitContext->scissorX, gGLGlobalAssets.blitContext->scissorY, gGLGlobalAssets.blitContext->scissorWidth, gGLGlobalAssets.blitContext->scissorHeight
			);
	#endif /* LOG_PARAMETERS */
	if (clipRect) {
		#if defined(LOG_PARAMETERS)
			LogRect(clipRect, "clipRect");
		#endif /* LOG_PARAMETERS */
		if (!glPort)	glPort = gCurrentGLPort;
		CHANGE_SCISSOR(glPort, clipRect->x, clipRect->y, clipRect->width, clipRect->height);	/* This also enables scissoring */
	}
	else {
		#if defined(LOG_PARAMETERS)
			LOGD("\tclipping disabled");
		#endif /* LOG_PARAMETERS */
		CHANGE_SCISSOR_ENABLE(false);
	}
}


/****************************************************************************//**
 * FskGLPortTexFormatGet - get the internal format of the glPort's texture.
 *	\param[in]	glPort	the port containing a reference to the texture.
 *	\param[out]	format	the GL code for the internal format of the specified texture.
 *	\return		kFskErrNone	if the operation was successful.
 ********************************************************************************/

#undef FskGLPortTexFormatGet
FskErr FskGLPortTexFormatGet(FskGLPort glPort, int *format) {
	FskErr err = kFskErrNone;
	BAIL_IF_FALSE(glPort && glPort->texture.name && glPort->texture.name != (unsigned)(-1) && glPort->texture.glIntFormat, err, kFskErrNotAccelerated);
	*format = glPort->texture.glIntFormat;
bail:
	return err;
}


/****************************************************************************//**
 * FskGLResizeBitmapTexture
 *	\param[in,out]	glPort		the port containing a reference to the texture.
 *	\param[in]		glFormat	the desired internal GL format for the texture.
 *	\param[in]		width		the desired width  the texture.
 *	\param[in]		height		the desired height the texture.
 *	\return			kFskErrNone	if the operation was successful.
 ********************************************************************************/

#undef FskGLPortResizeTexture
FskErr FskGLPortResizeTexture(FskGLPort glPort, int glFormat, int width, int height) {
	return ReshapeOffscreenTexture(glFormat, width, height, GL_RGBA, GL_UNSIGNED_BYTE, &glPort->texture);
}


/****************************************************************************//**
 * FskGLFBOIsInited
 *	\param[in]	glPort	the destination port.
 *	\return		true	if the the fbo has been inited.
 ********************************************************************************/
Boolean FskGLFBOIsInited(FskGLPort glPort) {
	return !glPort || glPort->texture.fboInited;
}


/****************************************************************************//**
 * FskGLFBOInit - cause the FBO to be initialized before doing anything.
 *	\param[in]	glPort	the destination port.
 ********************************************************************************/
void FskGLFBOInit(FskGLPort glPort) {
	glPort->texture.fboInited = false;
}


/********************************************************************************
 * FskGLEstimateTextureMemoryUsage
 ********************************************************************************/

UInt32 FskGLEstimateTextureMemoryUsage(void) {
	UInt32 mems = 0;
	FskGLPort p;
	for (p = gGLGlobalAssets.activePorts; p != NULL; p = p->next)
		mems += (UInt32)(p->texture.bounds.height) * (UInt32)(p->texture.bounds.width) * ((p->texture.glIntFormat == GL_ALPHA) ? 1U : 4U);
	return mems;
}


/********************************************************************************
 * FskGLReleaseResources
 ********************************************************************************/

FskErr FskGLReleaseResources(UInt32 which) {
	if (which & kFskGLResourceTypefaces) {															/* Dispose typefaces */
		FskGLTypeFace typeFace;
		LOGD("Disposing typefaces");
		while (NULL != (typeFace = gGLGlobalAssets.typeFaces))
			FskGLTypeFaceDispose(typeFace);
	}

	if (which & (kFskGLResourceBackedTextures | kFskGLResourceUnbackedTextures)) {					/* Unload textures */
		FskGLPort p;
		LOGD("Unloading textures with%s backing bitmap", (which & kFskGLResourceUnbackedTextures) ? " or without" : "");
		for (p = gGLGlobalAssets.activePorts; p != NULL; p = p->next) {
			if (p->texture.name && (GL_TEXTURE_UNLOADED != p->texture.name) && (p->texture.srcBM || (which & kFskGLResourceUnbackedTextures))) {
				GLPortReallyDeletePortTextures(p);													/* Toss it */
				p->texture.name = GL_TEXTURE_UNLOADED;
				if (p->texture.nameU) p->texture.nameU = GL_TEXTURE_UNLOADED;
				if (p->texture.nameV) p->texture.nameV = GL_TEXTURE_UNLOADED;
			}
		}
	}

	return FskErrorFromGLError(glGetError());
}


/****************************************************************************//**
 * purgeGL - operating system reports memory is low, empty caches
 *	\param[in,out]	refcon	reserved for future use; currently ignored.
 ********************************************************************************/
void purgeGL(void *refcon)
{
	LOGD("Disposing typefaces");
	if (refcon) {}
	(void)FskGLReleaseResources(kFskGLResourceTypefaces);
}


#if GLES_VERSION == 1	/* We need to define some functions when compiling for GL1, so things will link */
#ifndef __FSKEFFECTS__
	typedef struct FskEffectCacheRecord *FskEffectCache;
	typedef const struct FskEffectRecord *FskConstEffect;
#endif /* __FSKEFFECTS__ */
FskErr FskGLEffectCacheNew(void);
FskErr FskGLEffectCacheNew(void)
																								{ return kFskErrUnimplemented; }
void FskGLEffectCacheDispose(void);
void FskGLEffectCacheDispose(void)
																								{}
SInt32 FskGLEffectCacheCountDown(void);
SInt32 FskGLEffectCacheCountDown(void)
																								{ return 0; }
void FskGLEffectCacheDisposeAllBitmaps(void);
void FskGLEffectCacheDisposeAllBitmaps(void)
																								{}
FskErr FskGLEffectCacheGetBitmap(unsigned width, unsigned height, int flags, FskBitmap *bmp);
FskErr FskGLEffectCacheGetBitmap(unsigned width, unsigned height, int flags, FskBitmap *bmp)
																								{ if(width||height||flags||bmp){} return kFskErrUnimplemented; }
FskErr FskGLEffectCacheReleaseBitmap(FskBitmap bm);
FskErr FskGLEffectCacheReleaseBitmap(FskBitmap bm)
																								{ if(bm){} return kFskErrUnimplemented; }
FskErr FskGLEffectApply(FskConstEffect effect, FskConstBitmap src, FskConstRectangle srcRect, FskBitmap dst, FskConstPoint dstPoint);
FskErr FskGLEffectApply(FskConstEffect effect, FskConstBitmap src, FskConstRectangle srcRect, FskBitmap dst, FskConstPoint dstPoint)
																								{ if(effect||src||srcRect||dst||dstPoint){} return kFskErrUnimplemented; }
FskErr FskGLEffectApplyMode(FskConstEffect effect, FskConstBitmap src, FskConstRectangle srcRect, FskBitmap dst, FskConstPoint dstPoint, FskConstRectangle clipRect, UInt32 mode, FskConstGraphicsModeParameters modeParams);
FskErr FskGLEffectApplyMode(FskConstEffect effect, FskConstBitmap src, FskConstRectangle srcRect, FskBitmap dst, FskConstPoint dstPoint, FskConstRectangle clipRect, UInt32 mode, FskConstGraphicsModeParameters modeParams)
																								{ if(effect||src||srcRect||dst||dstPoint||clipRect||mode||modeParams){}	return kFskErrUnimplemented; }
void FskGLEffectsShutdown(void);
void FskGLEffectsShutdown(void)
																								{}
#endif /* GLES_VERSION == 1 */

#endif /* FSKBITMAP_OPENGL */
