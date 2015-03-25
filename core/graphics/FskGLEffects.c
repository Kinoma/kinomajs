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
	\file	FskGLEffects.c
	\brief	OpenGL and OpenGL-ES implementations of visual effects.
*/
#define __FSKBITMAP_PRIV__
#define __FSKGLBLIT_PRIV__
#define __FSKEFFECTS_PRIV__

#include <math.h>
#include <stdarg.h>
#include "FskEffects.h"
#include "FskGLBlit.h"
#include "FskMemory.h"
#include "FskPixelOps.h"
#include "FskString.h"
#if FSKBITMAP_OPENGL		/* This brackets the whole file */

#ifdef __APPLE__
	#if TARGET_OS_IPHONE
		#import <OpenGLES/ES2/gl.h>
	#else /* !TARGET_OS_IPHONE */
		#include <OpenGL/gl.h>
	#endif /* !TARGET_OS_IPHONE */
#elif TARGET_OS_ANDROID || TARGET_OS_KPL || defined(__linux__) || (FSK_OPENGLES_ANGLE == 1)
	#include <GLES2/gl2.h>			// Header File For the GLES 2 Library
	#include <GLES2/gl2ext.h>		// Header File For the GLES 2 extensionsLibrary
	#if (FSK_OPENGLES_ANGLE == 1)
		#undef GL_UNPACK_SKIP_ROWS	// The ANGLE library does not support calling glPixelStorei() with these values
		#undef GL_UNPACK_SKIP_PIXELS
		#undef GL_UNPACK_ROW_LENGTH
	#endif /* (FSK_OPENGLES_ANGLE == 1) */
#elif defined(_MSC_VER)
	#include <gl/gl.h>				// Header File For The OpenGL32 Library
#else
	#error unknown OS
#endif /* OS */

#ifndef TARGET_OS_WIN32
	#define TARGET_OS_WIN32 0
#endif /* TARGET_OS_WIN32 */

#define CHECK_GL_ERROR 0

#if SUPPORT_INSTRUMENTATION

	#define LOG_PARAMETERS
	#define LOG_CACHE
	//#define LOG_IMAGES
	//#define LOG_IMAGES_VERBOSE
	//#define LOG_VERTICES
	//#define LOG_GL_API			/**< Log the OpenGL API calls. */

	#define GLEFFECTS_DEBUG	1
	#define FskInstrumentedTypeEnabled(type, level)	FskInstrumentedTypeHasListenersForLevel(FskInstrumentedGlobalType(type), level)	/**< Whether instrumentation will print anything. */
	#define FskInstrumentedGlobalType(type)			(&(g##type##TypeInstrumentation))												/**< The data structure created with FskInstrumentedSimpleType(). */
#else /* !SUPPORT_INSTRUMENTATION */
	#define FskInstrumentedTypeEnabled(type, level)	(0)																				/**< Instrumentation will not print anything. */
#endif /* SUPPORT_INSTRUMENTATION */

FskInstrumentedSimpleType(GLEffects, gleffects);												/**< This declares the types needed for instrumentation. */

#ifdef GLEFFECTS_DEBUG
	#define	LOGD(...)	FskGLEffectsPrintfDebug(__VA_ARGS__)									/**< Print debugging logs. */
	#define	LOGI(...)	FskGLEffectsPrintfVerbose(__VA_ARGS__)									/**< Print information logs. */
#endif	/* GLEFFECTS_DEBUG */
#define		LOGE(...)	FskGLEffectsPrintfMinimal(__VA_ARGS__)									/**< Print error logs always, when instrumentation is on. */
#ifndef     LOGD
	#define LOGD(...)																			/**< Don't print debugging logs. */
#endif   	/* LOGD */
#ifndef     LOGI
	#define LOGI(...)																			/**< Don't print information logs. */
#endif   	/* LOGI */
#ifndef     LOGE
	#define LOGE(...)																			/**< Don't print error logs. */
#endif   /* LOGE */
#define LOGD_ENABLED()	FskInstrumentedTypeEnabled(GLEffects, kFskInstrumentationLevelDebug)	/**< Whether LOGD() will print anything. */
#define LOGI_ENABLED()	FskInstrumentedTypeEnabled(GLEffects, kFskInstrumentationLevelVerbose)	/**< Whether LOGI() will print anything. */
#define LOGE_ENABLED()	FskInstrumentedTypeEnabled(GLEffects, kFskInstrumentationLevelMinimal)	/**< Whether LOGE() will print anything. */

#ifdef LOG_GL_API
	#include "FskGLAPILog.h"
#endif /* LOG_GL_API */


#if GL_VERSION_2_0
 #define LOWP
 #define MEDIUMP
 #define HIGHP
#else /* GLES */
 #define LOWP				"lowp "		/**< Low    precision ( 8 bits) */
 #define MEDIUMP			"mediump "	/**< Medium precision (10 bits) */
 #define HIGHP				"highp "	/**< High   precision (32 bits) */
#endif /* < GL_VERSION_2_0 */

#define VERTEX_POSITION_INDEX	0		/**< The      vertex        (x,y) occupies the first  position in the vertex attributes. */
#define VERTEX_TEXCOORD_INDEX	1		/**< The texture coordinate (u,v) occupies the second position in the vertex attributes. */
#define VERTEX_END_INDEX		(-1)	/**< End the list of vertex attributes. */

#define TEXVERTEX_STRIDE		4		/**< 4 for xyuvxyuvxyuvxyuv... */
#define MTTVERTEX_STRIDE		6		/**< 6 for xyuvstxyuvstxyust...  */
#define TEXVERTEX_X				0		/**< The offset of the X coordinate in textured vertices. */
#define TEXVERTEX_Y				1		/**< The offset of the Y coordinate in textured vertices. */
#define TEXVERTEX_U				2		/**< The offset of the U coordinate in textured vertices. */
#define TEXVERTEX_V				3		/**< The offset of the V coordinate in textured vertices. */
#define TEXVERTEX_S				4		/**< The offset of the S coordinate in matte vertices. */
#define TEXVERTEX_T				5		/**< The offset of the T coordinate in matte vertices. */

typedef struct GLTexVertex  { float x, y, u, v; }       GLTexVertex;	/* dstCoord(x,y), texCoord(u,v) */
typedef struct GLMttVertex  { float x, y, u, v, s, t; } GLMttVertex;	/* dstCoord(x,y), texCoord(u,v), mtCoord(s,t) */
static FskErr FskGLEffectCopyApply(FskConstEffectCopy params, FskConstBitmap src, FskConstRectangle srcRect, FskBitmap dst, FskConstPoint dstPoint, FskEffectCache cache);



/********************************************************************************
 ********************************************************************************
 ********************************************************************************
 *****							Global IDs									*****
 ********************************************************************************
 ********************************************************************************
 ********************************************************************************/

/** Effects state variables.
 ** We encapsulate these together into one data structure for ease of management and resetting.
 **/
typedef struct EffectsGlobals {
	GLuint	gidCopyProgram;							/**< GL ID for the program. */
	GLint	gidCopyMatrix;							/**< GL ID for the matrix. */
	GLint	gidCopySrcBM;							/**< GL ID for the source bitmap. */
	float	CopyMtx[2];								/**< Current state of the shader matrix; only diagonals are pertinent. */

	GLuint	gidGaussianBlurProgram;					/**< GL ID for the program. */
	GLint	gidGaussianBlurMatrix;					/**< GL ID for the matrix. */
	GLint	gidGaussianBlurCoeff;					/**< GL ID for the blur coefficients. */
	GLint	gidGaussianBlurRadius;					/**< GL ID for the blur radius. */
	GLint	gidGaussianBlurSrcBM;					/**< GL ID for the source bitmap. */
	GLint	gidGaussianBlurSrcDelta;				/**< GL ID for the source delta. */
	float	GaussianBlurMtx[2];						/**< Current state of the shader matrix; only diagonals are pertinent. */
	float	GaussianSigma;							/**< Current state of the Gaussian sigma and dependent radius and coefficients. */

	GLuint	gidBoxBlurProgram;						/**< GL ID for the program. */
	GLint	gidBoxBlurMatrix;						/**< GL ID for the matrix. */
	GLint	gidBoxBlurRadius;						/**< GL ID for the blur radius. */
	GLint	gidBoxBlurSrcBM;						/**< GL ID for the source bitmap. */
	GLint	gidBoxBlurSrcDelta;						/**< GL ID for the source delta. */
	float	BoxBlurMtx[2];							/**< Current state of the shader matrix; only diagonals are pertinent. */
	int		BoxBlurRadius;							/**< Current state of the box blur radius. */

	GLuint	gidDilateProgram;						/**< GL ID for the program. */
	GLint	gidDilateMatrix;						/**< GL ID for the matrix. */
	GLint	gidDilateRadius;						/**< GL ID for the dilate radius. */
	GLint	gidDilateSrcBM;							/**< GL ID for the source bitmap. */
	GLint	gidDilateSrcDelta;						/**< GL ID for the source delta. */
	float	DilateMtx[2];							/**< Current state of the shader matrix; only diagonals are pertinent. */
	int		DilateRadius;							/**< Current state of the dilate radius. */

	GLuint	gidErodeProgram;						/**< GL ID for the program. */
	GLint	gidErodeMatrix;							/**< GL ID for the matrix. */
	GLint	gidErodeRadius;							/**< GL ID for the erode radius. */
	GLint	gidErodeSrcBM;							/**< GL ID for the source bitmap. */
	GLint	gidErodeSrcDelta;						/**< GL ID for the source delta. */
	float	ErodeMtx[2];							/**< Current state of the shader matrix; only diagonals are pertinent. */
	int		ErodeRadius;							/**< Current state of the erode radius. */

	GLuint	gidColorizeAlphaProgram;				/**< GL ID for the program. */
	GLint	gidColorizeAlphaMatrix;					/**< GL ID for the matrix. */
	GLint	gidColorizeAlphaColor0;					/**< GL ID for the color corresponding to alpha=0. */
	GLint	gidColorizeAlphaColor1;					/**< GL ID for the color corresponding to alpha=1. */
	GLint	gidColorizeAlphaSrcBM;					/**< GL ID for the source bitmap. */
	float	ColorizeAlphaMtx[2];					/**< Current state of the shader matrix; only diagonals are pertinent. */

	GLuint	gidMonochromeStraightProgram;			/**< GL ID for the program. */
	GLint	gidMonochromeStraightMatrix;			/**< GL ID for the matrix. */
	GLint	gidMonochromeStraightSrcBM;				/**< GL ID for the source bitmap. */
	GLint	gidMonochromeStraightColor0;			/**< GL ID for the color corresponding to black. */
	GLint	gidMonochromeStraightColor1;			/**< GL ID for the color corresponding to white. */
	float	MonochromeStraightMtx[2];				/**< Current state of the shader matrix; only diagonals are pertinent. */

	GLuint	gidMonochromePremulProgram;				/**< GL ID for the program. */
	GLint	gidMonochromePremulMatrix;				/**< GL ID for the matrix. */
	GLint	gidMonochromePremulSrcBM;				/**< GL ID for the source bitmap. */
	GLint	gidMonochromePremulColor0;				/**< GL ID for the color corresponding to black. */
	GLint	gidMonochromePremulColor1;				/**< GL ID for the color corresponding to white. */
	float	MonochromePremulMtx[2];					/**< Current state of the shader matrix; only diagonals are pertinent. */

	GLuint	gidColorizeInnerStraightProgram;		/**< GL ID for the program. */
	GLint	gidColorizeInnerStraightMatrix;			/**< GL ID for the matrix. */
	GLint	gidColorizeInnerStraightSrcBM;			/**< GL ID for the source bitmap. */
	GLint	gidColorizeInnerStraightMttBM;			/**< GL ID for the matte bitmap. */
	GLint	gidColorizeInnerStraightColor;			/**< GL ID for the color. */
	float	ColorizeInnerStraightMtx[2];			/**< Current state of the shader matrix; only diagonals are pertinent. */

	GLuint	gidColorizeInnerPremulProgram;			/**< GL ID for the program. */
	GLint	gidColorizeInnerPremulMatrix;			/**< GL ID for the matrix. */
	GLint	gidColorizeInnerPremulSrcBM;			/**< GL ID for the source bitmap. */
	GLint	gidColorizeInnerPremulMttBM;			/**< GL ID for the matte bitmap. */
	GLint	gidColorizeInnerPremulColor;			/**< GL ID for the color. */
	float	ColorizeInnerPremulMtx[2];				/**< Current state of the shader matrix; only diagonals are pertinent. */

	GLuint	gidColorizeOuterStraightProgram;		/**< GL ID for the program. */
	GLint	gidColorizeOuterStraightMatrix;			/**< GL ID for the matrix. */
	GLint	gidColorizeOuterStraightSrcBM;			/**< GL ID for the source bitmap. */
	GLint	gidColorizeOuterStraightMttBM;			/**< GL ID for the matte bitmap. */
	GLint	gidColorizeOuterStraightColor;			/**< GL ID for the color. */
	float	ColorizeOuterStraightMtx[2];			/**< Current state of the shader matrix; only diagonals are pertinent. */

	GLuint	gidColorizeOuterPremulProgram;			/**< GL ID for the program. */
	GLint	gidColorizeOuterPremulMatrix;			/**< GL ID for the matrix. */
	GLint	gidColorizeOuterPremulSrcBM;			/**< GL ID for the source bitmap. */
	GLint	gidColorizeOuterPremulMttBM;			/**< GL ID for the matte bitmap. */
	GLint	gidColorizeOuterPremulColor;			/**< GL ID for the color. */
	float	ColorizeOuterPremulMtx[2];				/**< Current state of the shader matrix; only diagonals are pertinent. */

	GLuint	gidColorizePremulProgram;				/**< GL ID for the program. */
	GLint	gidColorizePremulMatrix;				/**< GL ID for the matrix. */
	GLint	gidColorizePremulSrcBM;					/**< GL ID for the source bitmap. */
	GLint	gidColorizePremulColor;					/**< GL ID for the color. */
	float	ColorizePremulMtx[2];					/**< Current state of the shader matrix; only diagonals are pertinent. */

	GLuint	gidColorizeStraightProgram;				/**< GL ID for the program. */
	GLint	gidColorizeStraightMatrix;				/**< GL ID for the matrix. */
	GLint	gidColorizeStraightSrcBM;				/**< GL ID for the source bitmap. */
	GLint	gidColorizeStraightColor;				/**< GL ID for the color. */
	float	ColorizeStraightMtx[2];					/**< Current state of the shader matrix; only diagonals are pertinent. */

	GLuint	gidGelProgram;							/**< GL ID for the program. */
	GLint	gidGelMatrix;							/**< GL ID for the matrix. */
	GLint	gidGelSrcBM;							/**< GL ID for the source bitmap. */
	GLint	gidGelColor;							/**< GL ID for the color. */
	float	GelMtx[2];								/**< Current state of the shader matrix; only diagonals are pertinent. */

	GLuint gidStraightToPremultipliedProgram;		/**< GL ID for the program. */
	GLint  gidStraightToPremultipliedMatrix;		/**< GL ID for the matrix. */
	GLint  gidStraightToPremultipliedSrcBM;			/**< GL ID for the source bitmap. */
	float  StraightToPremultipliedMtx[2];			/**< Current state of the shader matrix; only diagonals are pertinent. */

	GLuint gidPremultipliedToStraightProgram;		/**< GL ID for the program. */
	GLint  gidPremultipliedToStraightMatrix;		/**< GL ID for the matrix. */
	GLint  gidPremultipliedToStraightSrcBM;			/**< GL ID for the source bitmap. */
	GLint  gidPremultipliedToStraightBgColor;		/**< GL ID for the background color. */
	float  PremultipliedToStraightMtx[2];			/**< Current state of the shader matrix; only diagonals are pertinent. */

	Boolean	canWrapNPOT;							/**< Indicates whether wrapping is available for non-power-of-two textures. */
	Boolean	rendererIsTiled;						/**< Indicates whether the renderer is tiled. */
	Boolean	hasHighPrecision;						/**< Indicates whether the fragment shader has high precision. */
	UInt8	mediumPrecisionBits;					/**< The number of significant bits in medium precision */
	SInt8	mediumPrecisionRange[2];				/* The exponent range of medium precision. */
} EffectsGlobals;

static EffectsGlobals gEffectsGlobals = { 0 };



/********************************************************************************
 ********************************************************************************
 ********************************************************************************
 *****							Support										*****
 ********************************************************************************
 ********************************************************************************
 ********************************************************************************/

#if CHECK_GL_ERROR

/********************************************************************************
 * GetFskGLError
 ********************************************************************************/

static FskErr GetFskGLError() {
	FskErr err = glGetError();
	if (err)
		err = FskErrorFromGLError(err);
	return err;
}


/********************************************************************************
 * Print Shader Log.
 ********************************************************************************/

static void PrintShaderLog(GLuint id, GLenum type, const char *shader) {
	char	*errMsg;
	GLsizei	msgLength;
	glGetShaderiv(id, GL_INFO_LOG_LENGTH, &msgLength);
	if (kFskErrNone == FskMemPtrNew(msgLength, &errMsg)) {
		glGetShaderInfoLog(id, msgLength, &msgLength, errMsg);
		LOGD("Shader Log:\n%sfor %s Shader:\n%s", errMsg, ((type == GL_VERTEX_SHADER) ? "Vertex" : "Fragment"), shader);
		FskMemPtrDispose(errMsg);
	}
}


/********************************************************************************
 * Print Program Log.
 ********************************************************************************/

static void PrintProgramLog(GLuint id) {
	char	*errMsg;
	GLsizei	msgLength;
	glGetProgramiv(id, GL_INFO_LOG_LENGTH, &msgLength);
	if (kFskErrNone == FskMemPtrNew(msgLength, &errMsg)) {
		glGetProgramInfoLog(id, msgLength, &msgLength, errMsg);
		LOGD("Program Log:\n%s", errMsg);
		FskMemPtrDispose(errMsg);
	}
}
#endif /* CHECK_GL_ERROR */

#ifdef GLEFFECTS_DEBUG
#if !defined(GL_BGRA) && defined(GL_BGRA_EXT)
	#define GL_BGRA GL_BGRA_EXT
#endif /* !GL_BGRA */


typedef struct LookupEntry {
	int			code;	/* Make sure that 0 is the last code */
	const char	*name;
} LookupEntry;
static const char* LookupNameFromCode(const LookupEntry *table, int code) {
	for (; table->name != NULL; ++table)
		if (table->code == code)
			break;
	return table->name ? table->name : "UNKNOWN";
}

static const char* GLInternalFormatNameFromCode(GLint code) {
	static const LookupEntry lookupTab[] = {
		{	GL_RGBA,						"RGBA"							},
		{	GL_RGB,							"RGB"							},
		{	GL_ALPHA,						"ALPHA"							},
		{	GL_LUMINANCE,					"LUMINANCE"						},
			#ifdef GL_BGRA
		{	GL_BGRA,						"BGRA"							},
			#endif /* GL_BGRA */
		{	0,								NULL							}
	};
	return LookupNameFromCode(lookupTab, code);
}
static const char* GLWrapModeNameFromCode(GLint code) {
	static const LookupEntry lookupTab[] = {
		{	GL_REPEAT,						"REPEAT"						},
		{	GL_CLAMP_TO_EDGE,				"CLAMP_TO_EDGE"					},
		{	GL_MIRRORED_REPEAT,				"MIRRORED_REPEAT"				},
		{	0,								NULL							}
	};
	return LookupNameFromCode(lookupTab, code);
}
static const char* GetEffectNameFromCode(int code) {
	static const LookupEntry lut[] = {
		{ kFskEffectBoxBlur,					"BoxBlur"					},
		{ kFskEffectColorize,					"Colorize"					},
		{ kFskEffectColorizeAlpha,				"ColorizeAlpha"				},
		{ kFskEffectColorizeInner,				"ColorizeInner"				},
		{ kFskEffectColorizeOuter,				"ColorizeOuter"				},
		{ kFskEffectCopy,						"Copy"						},
		{ kFskEffectCopyMirrorBorders,			"CopyMirrorBorders"			},
		{ kFskEffectDilate,						"Dilate"					},
		{ kFskEffectDirectionalBoxBlur,			"DirectionalBoxBlur"		},
		{ kFskEffectDirectionalDilate,			"DirectionalDilate"			},
		{ kFskEffectDirectionalErode,			"DirectionalErode"			},
		{ kFskEffectDirectionalGaussianBlur,	"DirectionalGaussianBlur"	},
		{ kFskEffectErode,						"Erode"						},
		{ kFskEffectGaussianBlur,				"GaussianBlur"				},
		{ kFskEffectInnerGlow,					"InnerGlow"					},
		{ kFskEffectInnerShadow,				"InnerShadow"				},
		{ kFskEffectMask,						"Mask"						},
		{ kFskEffectMonochrome,					"Monochrome"				},
		{ kFskEffectOuterGlow,					"OuterGlow"					},
		{ kFskEffectOuterShadow,				"OuterShadow"				},
		{ kFskEffectShade,						"Shade"						},
		{ -1,									NULL						}
	};
	return LookupNameFromCode(lut, code);
}

static const char* GetTopologyNameFromCode(int code) {
	static const LookupEntry lut[] = {
		{ kFskEffectCompoundTopologyPipeline,	"Pipeline"	},
		{ kFskEffectCompoundTopologyUnknown,	"Unknown"	},
		{ -1,									NULL		}
	};
	return LookupNameFromCode(lut, code);
}

static const char* ModeNameFromCode(UInt32 mode) {
	static const LookupEntry lookupTab[] = {
		{	kFskGraphicsModeBilinear|kFskGraphicsModeCopy,		"BilinearCopy"		},
		{	kFskGraphicsModeBilinear|kFskGraphicsModeAlpha,		"BilinearAlpha"		},
		{	kFskGraphicsModeBilinear|kFskGraphicsModeColorize,	"BilinearColorize"	},
		{	kFskGraphicsModeCopy,								"Copy"				},
		{	kFskGraphicsModeAlpha,								"Alpha"				},
		{	kFskGraphicsModeColorize,							"Colorize"			},
		{ -1,													NULL				}

	};
	return LookupNameFromCode(lookupTab, mode);
}

static void LogSrcBitmap(FskConstBitmap srcBM, const char *name) {
	if (!srcBM)
		return;
	if (!name)
		name = "SRCBM";
	if (srcBM->glPort) {
		FskRectangleRecord texRect;
		int glIntFormat;
		FskGLPortTexRectGet(srcBM->glPort, &texRect);
		FskGLPortTexFormatGet(srcBM->glPort, &glIntFormat);
		LOGD("\t%s: bounds(%ld %ld %ld %ld) depth=%lu format=%s rowBytes=%ld bits=%p alpha=%d premul=%d texture(#%u w=%ld h=%ld %s) useCount=%d",
			name, srcBM->bounds.x, srcBM->bounds.y, srcBM->bounds.width, srcBM->bounds.height, srcBM->depth,
			FskBitmapFormatName(srcBM->pixelFormat), srcBM->rowBytes, srcBM->bits, srcBM->hasAlpha, srcBM->alphaIsPremultiplied,
			FskGLPortSourceTexture(srcBM->glPort), texRect.width, texRect.height, GLInternalFormatNameFromCode(glIntFormat),
			srcBM->useCount);
	}
	else {
		LOGD("\t%s: bounds(%ld %ld %ld %ld), depth=%lu format=%s rowBytes=%ld bits=%p alpha=%d premul=%d useCount=%d",
			name, srcBM->bounds.x, srcBM->bounds.y, srcBM->bounds.width, srcBM->bounds.height, srcBM->depth,
			FskBitmapFormatName(srcBM->pixelFormat), srcBM->rowBytes, srcBM->bits, srcBM->hasAlpha, srcBM->alphaIsPremultiplied, srcBM->useCount);
	}
}

static void LogRect(FskConstRectangle r, const char *name) {
	if (!r)
		return;
	if (!name)
		name = "RECT";
	LOGD("\t%s(%ld %ld %ld %ld)", name, r->x, r->y, r->width, r->height);
}

static void LogPoint(FskConstPoint p, const char *name) {
	if (!p)
		return;
	if (!name)
		name = "POINT";
	LOGD("\t%s(%ld %ld)", name, p->x, p->y);
}

static void LogDstBitmap(FskBitmap dstBM, const char *name) {
	FskRectangleRecord	portRect;
	if (!dstBM)
		return;
	if (!name)
		name = "DSTBM";
	if (kFskErrNone == FskGLDstBMRectGet(dstBM, &portRect)) {
		unsigned	texName		= FskGLPortSourceTexture(dstBM->glPort);
		int			glIntFormat;
		if (texName)	FskGLPortTexFormatGet(dstBM->glPort, &glIntFormat);
		else			glIntFormat = 0;
		LOGD("\t%s: bounds(%ld %ld %ld %ld) depth=%lu format=%s rowBytes=%ld bits=%p alpha=%d premul=%d glDstPort(#%u w=%lu h=%lu %s), useCount=%d",
			name, dstBM->bounds.x, dstBM->bounds.y, dstBM->bounds.width, dstBM->bounds.height, dstBM->depth,
			FskBitmapFormatName(dstBM->pixelFormat), dstBM->rowBytes, dstBM->bits, dstBM->hasAlpha, dstBM->alphaIsPremultiplied,
			texName, portRect.width, portRect.height, GLInternalFormatNameFromCode(glIntFormat), dstBM->useCount);
	} else {
		LOGD("\t%s: bounds(%ld %ld %ld %ld) depth=%lu format=%s rowBytes=%ld bits=%p alpha=%d premul=%d, useCount=%d",
			name, dstBM->bounds.x, dstBM->bounds.y, dstBM->bounds.width, dstBM->bounds.height, dstBM->depth,
			FskBitmapFormatName(dstBM->pixelFormat), dstBM->rowBytes, dstBM->bits, dstBM->hasAlpha, dstBM->alphaIsPremultiplied, dstBM->useCount);
	}
}

static void LogEffectsParameters(const char *func, const void *params, FskConstBitmap src, FskConstRectangle srcRect, FskBitmap dst, FskConstPoint dstPoint, FskEffectCache cache) {
	LOGD("%s(params=%p src=%p srcRect=%p dst=%p dstPoint=%p cache=%p (%u))",
		func, params, src, srcRect, dst, dstPoint, cache, (cache ? cache->numBitmaps : 0));
	LogSrcBitmap(src, "src");
	LogRect(srcRect, "srcRect");
	LogDstBitmap(dst, "dst");
	LogPoint(dstPoint, "dstPoint");
}

static void LogColor(FskConstColorRGBA color, const char *name) {
	LOGD("\t%s(%3u %3u %3u %3u)", name, color->r, color->g, color->b, color->a);
}

#ifdef LOG_IMAGES
#include "FskEndian.h"
#include <stdio.h>
#include <stdarg.h>
#if TARGET_ALIGN_PACKPUSH
    #pragma pack(push, 2)
#elif TARGET_ALIGN_PACK
    #pragma pack(2)
#endif
typedef struct {
	UInt16		imageFileType;
	UInt32		fileSize;
	UInt16		reserved1;
	UInt16		reserved2;
	UInt32		imageDataOffset;

	// now comes the BITMAPINFOHEADER
	UInt32		biSize;
	SInt16		biWidth;
	SInt16		biHeight;
	UInt16		biPlanes;
	UInt16		biBitCount;
} FSK_PACK_STRUCT FskBMPOS221Record, *FskBMPOS221;
#if TARGET_ALIGN_PACKPUSH
    #pragma pack(pop)
#elif TARGET_ALIGN_PACK
    #pragma pack()
#endif
static FskErr BMPEncode(FskBitmap bits, const void **dataOut, UInt32 *dataSizeOut)
{
	FskErr err;
	FskRectangleRecord bounds;
	unsigned char *image = NULL, *p;
	SInt32 imageSize = 0, x, y, outputRowBytes;
	FskBMPOS221Record bmps;
	FskColorRGBARecord rgba;
	Boolean hasAlpha;
	UInt32 *srcPtr;
	SInt32 srcRowBytes;
	FskBitmapFormatEnum srcFormat;

	FskBitmapReadBegin(bits, (const void**)(const void*)&srcPtr, &srcRowBytes, &srcFormat);
	BAIL_IF_ERR(err = FskBitmapGetPixel(bits, 0, 0, &rgba));	// verify that this pixel format is supported
	BAIL_IF_FALSE(srcFormat != kFskBitmapFormatYUV420	&&		// Our simple conversion attempt below doesn't work for YUV
				  srcFormat != kFskBitmapFormatYUV422	&&
				  srcFormat != kFskBitmapFormatUYVY		&&
				  srcFormat != kFskBitmapFormatYUV420i, err, kFskErrUnsupportedPixelType);
	FskBitmapGetBounds(bits, &bounds);

	err = FskBitmapGetHasAlpha(bits, &hasAlpha);
	if (err) goto bail;

	outputRowBytes = bounds.width * (hasAlpha ? 4 : 3);
	if (outputRowBytes % 4)
		outputRowBytes += 4 - (outputRowBytes % 4);

	imageSize = sizeof(FskBMPOS221Record) + (bounds.height * outputRowBytes);
	err = FskMemPtrNewClear(imageSize, (FskMemPtr *)&image);
	if (err) goto bail;

	bmps.imageFileType = (UInt16)(('B'<<8) | 'M');
	bmps.imageFileType = FskEndianU16_NtoB(bmps.imageFileType);
	bmps.fileSize = sizeof(FskBMPOS221Record) + (bounds.height * outputRowBytes);
	bmps.fileSize = FskEndianU32_NtoL(bmps.fileSize);
	bmps.reserved1 = 0;
	bmps.reserved2 = 0;
	bmps.imageDataOffset = FskEndianU32_NtoL(sizeof(bmps));
	bmps.biSize = FskEndianU32_NtoL(12);
	bmps.biWidth = (SInt16)FskEndianU16_NtoL(bounds.width);
	bmps.biHeight = (SInt16)FskEndianU16_NtoL(bounds.height);
	bmps.biPlanes = FskEndianU16_NtoL(1);
	bmps.biBitCount = hasAlpha ? FskEndianU16_NtoL(32) : FskEndianU16_NtoL(24);

	p = image;
	FskMemMove(p, &bmps, sizeof(bmps));
	p += sizeof(bmps);
	if (false == hasAlpha) {
		for (y = bounds.height - 1; y >= 0; y--) {
			for (x = 0; x < bounds.width; x++) {
				FskBitmapGetPixel(bits, x, y, &rgba);		// not the fastest, but the most general purpose
				*p++ = rgba.b;
				*p++ = rgba.g;
				*p++ = rgba.r;
			}
			p -= bounds.width * 3;
			p += outputRowBytes;
		}
	}
	else {
		if (kFskBitmapFormat32BGRA != srcFormat) {
			for (y = bounds.height - 1; y >= 0; y--) {
				for (x = 0; x < bounds.width; x++) {
					FskBitmapGetPixel(bits, x, y, &rgba);		// not the fastest, but the most general purpose
					*p++ = rgba.b;
					*p++ = rgba.g;
					*p++ = rgba.r;
					*p++ = rgba.a;
				}
				p -= bounds.width * 4;
				p += outputRowBytes;
			}
		}
		else {
			srcPtr = (UInt32*)((UInt32)(srcRowBytes * (bounds.height - 1)) + (char *)srcPtr);
			for (y = bounds.height - 1; y >= 0; y--) {
				for (x = 0; x < bounds.width; x++) {
					FskMisaligned32_PutN(srcPtr, p);
					++srcPtr;
					p += 4;
				}
				p -= bounds.width * 4;
				p += outputRowBytes;
				srcPtr = (UInt32 *)(-srcRowBytes - (bounds.width * 4) + (char *)srcPtr);
			}
		}
	}

bail:
	FskBitmapReadEnd(bits);

	if (err) {
		FskMemPtrDisposeAt(&image);
		imageSize = 0;
	}

	*dataOut = image;
	*dataSizeOut = imageSize;

	return err;
}
static FskErr WriteBMPToPath(FskConstBitmap bits, const char *path) {
	const void	*data = NULL;
	UInt32	size = 0;
	FskErr	err;
	BAIL_IF_ERR(err = BMPEncode((FskBitmap)bits, &data, &size));
	FILE *fd = fopen(path, "wb");
	if (fd) {
		#ifdef LOG_IMAGES_VERBOSE
			LOGD("Writing \"%s\"", path);
		#endif /* LOG_IMAGES_VERBOSE */
		fwrite(data, size, 1, fd);
		fclose(fd);
	}
	else {
		LOGD("Cannot open \"%s\"\n", path);
	}
bail:
	FskMemPtrDispose((void*)data);
	if (err) LOGD("BMP encode error on %s \"%s\": %d", FskBitmapFormatName(bits->pixelFormat), path, err);
	return err;
}
static FskErr WriteBMPToVPath(FskConstBitmap bm, const char *fmt, ...) {
	va_list ap;
	char	path[1024];
	va_start(ap, fmt);
	vsnprintf(path, sizeof(path), fmt, ap);
	va_end(ap);
	return WriteBMPToPath(bm, path);
}
static FskErr WriteGLBMPToVPath(FskConstBitmap bm, const char *fmt, ...) {
	FskBitmap tmp = NULL;
	FskErr err;
	Boolean b;
	va_list ap;
	char	path[1024];
	err = FskBitmapNew(bm->bounds.width, bm->bounds.height, kFskBitmapFormat32BGRA, &tmp);	/* BMP writes BGRA */
	FskBitmapGetHasAlpha(bm, &b);
	FskBitmapSetHasAlpha(tmp, true);
	err = FskGLPortPixelsRead(bm->glPort, 0, NULL, tmp);
	va_start(ap, fmt);
	vsnprintf(path, sizeof(path), fmt, ap);
	va_end(ap);
	err = WriteBMPToPath(tmp, path);
	FskBitmapDispose(tmp);
	return err;
}
#endif /* LOG_IMAGES */

#endif /* GLEFFECTS_DEBUG */


/*******************************************************************************
 * SetEffectVertices
 *******************************************************************************/

static FskErr SetEffectVertices(FskConstRectangle srcRect, FskConstRectangle texRect, FskConstRectangle dstRect) {
	FskErr err = kFskErrNone;
	GLTexVertex	*vertices;

	#ifdef LOG_VERTICES
		LOGD("srcRect(%3d, %3d, %3d, %3d), texRect(%3d, %3d, %3d, %3d), dstRect(%3d, %3d, %3d, %3d)",
			(int)srcRect->x,	(int)srcRect->y,	(int)srcRect->width,	(int)srcRect->height,
			(int)texRect->x,	(int)texRect->y,	(int)texRect->width,	(int)texRect->height,
			(int)dstRect->x,	(int)dstRect->y,	(int)dstRect->width,	(int)dstRect->height
		);
	#endif /* LOG_VERTICES */

	BAIL_IF_ERR(err = FskGLGetCoordinatePointer((float**)(void*)(&vertices), NULL));
	vertices[0].x = vertices[1].x = (float)(dstRect->x);
	vertices[2].x = vertices[3].x = (float)(dstRect->x + dstRect->width);
	vertices[0].y = vertices[3].y = (float)(dstRect->y);
	vertices[1].y = vertices[2].y = (float)(dstRect->y + dstRect->height);
	vertices[0].u = vertices[1].u = (srcRect->x - texRect->x)                   / (float)(texRect->width);
	vertices[2].u = vertices[3].u = (srcRect->x - texRect->x + srcRect->width)  / (float)(texRect->width);
	vertices[0].v = vertices[3].v = (srcRect->y - texRect->y)                   / (float)(texRect->height);
	vertices[1].v = vertices[2].v = (srcRect->y - texRect->y + srcRect->height) / (float)(texRect->height);

	#ifdef LOG_VERTICES
	{	int i;
		for (i = 0; i < 4; ++i)
			LOGD("vertex[%d]: x=%7.2f y=%7.2f u=%7.5f v=%7.5f", i, vertices[i].x, vertices[i].y, vertices[i].u, vertices[i].v);
	}
	#endif /* LOG_VERTICES */

bail:
	return err;
}


/*******************************************************************************
 * ResetMatteEffectVertices
 *******************************************************************************/

static FskErr ResetMatteEffectVertices(void) {
	FskErr err = kFskErrNone;
	GLTexVertex	*vertices;

	BAIL_IF_ERR(err = FskGLGetCoordinatePointer((float**)(void*)(&vertices), NULL));
	glVertexAttribPointer(VERTEX_POSITION_INDEX, 2, GL_FLOAT, GL_FALSE, sizeof(GLTexVertex), &vertices->x);
	glVertexAttribPointer(VERTEX_TEXCOORD_INDEX, 2, GL_FLOAT, GL_FALSE, sizeof(GLTexVertex), &vertices->u);

bail:
	return err;
}


/*******************************************************************************
 * SetMatteEffectVertices
 *******************************************************************************/

static FskErr SetMatteEffectVertices(FskConstRectangle srcRect, FskConstRectangle stxRect, FskConstRectangle mttRect, FskConstRectangle mtxRect, FskConstRectangle dstRect) {
	FskErr err = kFskErrNone;
	GLMttVertex	*vertices;

	#ifdef LOG_VERTICES
		LOGD("srcRect(%3d, %3d, %3d, %3d), stxRect(%3d, %3d, %3d, %3d), mttRect(%3d, %3d, %3d, %3d), mtxRect(%3d, %3d, %3d, %3d), dstRect(%3d, %3d, %3d, %3d)",
			(int)srcRect->x,	(int)srcRect->y,	(int)srcRect->width,	(int)srcRect->height,
			(int)stxRect->x,	(int)stxRect->y,	(int)stxRect->width,	(int)stxRect->height,
			(int)mttRect->x,	(int)mttRect->y,	(int)mttRect->width,	(int)mttRect->height,
			(int)mtxRect->x,	(int)mtxRect->y,	(int)mtxRect->width,	(int)mtxRect->height,
			(int)dstRect->x,	(int)dstRect->y,	(int)dstRect->width,	(int)dstRect->height
		);
	#endif /* LOG_VERTICES */

	BAIL_IF_ERR(err = FskGLGetCoordinatePointer((float**)(void*)(&vertices), NULL));
	glVertexAttribPointer(VERTEX_POSITION_INDEX, 2, GL_FLOAT, GL_FALSE, sizeof(GLMttVertex), &vertices->x);
	glVertexAttribPointer(VERTEX_TEXCOORD_INDEX, 4, GL_FLOAT, GL_FALSE, sizeof(GLMttVertex), &vertices->u);

	vertices[0].x = vertices[1].x = (float)(dstRect->x);													/* destination coordinates */
	vertices[2].x = vertices[3].x = (float)(dstRect->x + dstRect->width);
	vertices[0].y = vertices[3].y = (float)(dstRect->y);
	vertices[1].y = vertices[2].y = (float)(dstRect->y + dstRect->height);

	vertices[0].u = vertices[1].u = (srcRect->x - stxRect->x)                   / (float)(stxRect->width);	/* source coordinates */
	vertices[2].u = vertices[3].u = (srcRect->x - stxRect->x + srcRect->width)  / (float)(stxRect->width);
	vertices[0].v = vertices[3].v = (srcRect->y - stxRect->y)                   / (float)(stxRect->height);
	vertices[1].v = vertices[2].v = (srcRect->y - stxRect->y + srcRect->height) / (float)(stxRect->height);

	vertices[0].s = vertices[1].s = (mttRect->x - mtxRect->x)                   / (float)(mtxRect->width);	/* matte coordinates */
	vertices[2].s = vertices[3].s = (mttRect->x - mtxRect->x + mttRect->width)  / (float)(mtxRect->width);
	vertices[0].t = vertices[3].t = (mttRect->y - mtxRect->y)                   / (float)(mtxRect->height);
	vertices[1].t = vertices[2].t = (mttRect->y - mtxRect->y + mttRect->height) / (float)(mtxRect->height);

	#ifdef LOG_VERTICES
	{	int i;
		for (i = 0; i < 4; ++i)
			LOGD("vertex[%d]: x=%7.2f y=%7.2f u=%7.5f v=%7.5f s=%7.5f t=%7.5f", i, vertices[i].x, vertices[i].y, vertices[i].u, vertices[i].v, vertices[i].s, vertices[i].t);
	}
	#endif /* LOG_VERTICES */

bail:
	return err;
}


#ifdef LOG_VERTICES
/*******************************************************************************
 * AreaOfTriangle
 *******************************************************************************/

static float AreaOfTriangle(float x0, float y0, float x1, float y1, float x2, float y2) {
	double	x3 = (double)x1 - (double)x0,
			y3 = (double)y1 - (double)y0,
			x4 = (double)x2 - (double)x0,
			y4 = (double)y2 - (double)y0;
	return (float)(fabs(x3 * y4 - y3 * x4) * .5);
}
#endif /* LOG_VERTICES */


/*******************************************************************************
 * SetReflectBorderVertices
 *    x0  x1  x2  x3            [0]			[1]			[2]			[3]
 *  y0 +---+---+---+ v0			0			2			4			6		[0]
 *     |   |   |   |
 *  y1 +---+---+---+ v1			1, 13		3, 11		5, 9		7		[1]
 *     |   |   |   |
 *  y2 +---+---+---+ v2			14			12, 16		10, 18		8, 20	[2]
 *     |   |   |   |
 *  y3 +---+---+---+ v3			15			17			19			21		[3]
 *    u0  u1  u2  u3
 *
 *	0=(0,0)		1=(0,1)		2=(1,0)		3=(1,1)		4=(2,0)		5=(2,1)		6=(3,0)		7=(3,1)
 *	8=(3,2)		9=(2,1)		10=(3,1)	11=(1,1)	12=(1,2)	13=(0,1)	14=(0,2)	15=(0,3)
 *	16=(1,2)	17=(1,3)	18=(2,2)	19=(2,3)	20=(3,2)	21=(3,3)
 *******************************************************************************/
#define NUM_REFLECTED_VERTICES	22

static FskErr SetReflectBorderVertices(int border, FskConstRectangle srcRect, FskConstRectangle texRect, FskConstRectangle dstRect) {
	FskErr		err		= kFskErrNone;
	const float	xybias	= (float)border,
				iw		= 1.f / (float)(texRect->width),
				ih		= 1.f / (float)(texRect->height);
	struct DU8 { UInt8 x, y; };
	static const struct DU8 scan[] = {	/* Triangle strip scan */
		{ 0, 0 },	/*  0 */
		{ 0, 1 },	/*  1 */
		{ 1, 0 },	/*  2 */
		{ 1, 1 },	/*  3 */
		{ 2, 0 },	/*  4 */
		{ 2, 1 },	/*  5 */
		{ 3, 0 },	/*  6 */
		{ 3, 1 },	/*  7 */
		{ 3, 2 },	/*  8 */
		{ 2, 1 },	/*  9 */
		{ 2, 2 },	/* 10 */
		{ 1, 1 },	/* 11 */
		{ 1, 2 },	/* 12 */
		{ 0, 1 },	/* 13 */
		{ 0, 2 },	/* 14 */
		{ 0, 3 },	/* 15 */
		{ 1, 2 },	/* 16 */
		{ 1, 3 },	/* 17 */
		{ 2, 2 },	/* 18 */
		{ 2, 3 },	/* 19 */
		{ 3, 2 },	/* 20 */
		{ 3, 3 }	/* 21 */
	};
	const unsigned numVertices = sizeof(scan) / sizeof(scan[0]);
	const struct DU8 *q;
	GLTexVertex	*vertices, *p;
	GLTexVertex	samp[4];
	unsigned	i;

	BAIL_IF_FALSE(numVertices == NUM_REFLECTED_VERTICES, err, kFskErrMismatch);

	#ifdef LOG_VERTICES
		LOGD("SetReflectBorderVertices(%d, %p, %p, %p)", border, srcRect, texRect, dstRect);
		LogRect(srcRect, "srcRect");
		LogRect(texRect, "texRect");
		LogRect(dstRect, "dstRect");
	#endif /* LOG_VERTICES */

	/* Initialize samples */
	samp[1].x = dstRect->x + xybias;				samp[2].x = samp[1].x + dstRect->width;		samp[0].x = samp[1].x - border;		samp[3].x = samp[2].x + border;
	samp[1].y = dstRect->y + xybias;				samp[2].y = samp[1].y + dstRect->height;	samp[0].y = samp[1].y - border;		samp[3].y = samp[2].y + border;
	samp[1].u = (float)(srcRect->x - texRect->x);	samp[2].u = samp[1].u + srcRect->width;		samp[0].u = samp[1].u + border;		samp[3].u = samp[2].u - border;
	samp[1].v = (float)(srcRect->y - texRect->y);	samp[2].v = samp[1].v + srcRect->height;	samp[0].v = samp[1].v + border;		samp[3].v = samp[2].v - border;
	for (i = 4, p = samp; i--; ++p) {
		p->u *= iw;																		/* Normalize ... */
		p->v *= ih;																		/* ... texture coordinates */
	}

	BAIL_IF_ERR(err = FskGLGetCoordinatePointer((float**)(void*)(&vertices), NULL));
	for (i = numVertices, p = vertices, q = scan; i--; ++p, ++q) {
		const GLTexVertex *r;
		r = &samp[q->x];
		p->x = r->x;
		p->u = r->u;
		r = &samp[q->y];
		p->y = r->y;
		p->v = r->v;
	}

	#ifdef LOG_VERTICES
		for (i = 0; i < numVertices; ++i)
			LOGD("vertex[%2d]: x=%7.2f y=%7.2f u=%7.5f v=%7.5f", i, vertices[i].x, vertices[i].y, vertices[i].u, vertices[i].v);
		for (i = 2; i < numVertices; ++i)
			LOGD("area[%2d,%2d, %2d]: xy=%6.0f uv=%6.5f", i-2, i-1, i,
				AreaOfTriangle(vertices[i-2].x, vertices[i-2].y, vertices[i-1].x, vertices[i-1].y, vertices[i-0].x, vertices[i-0].y),
				AreaOfTriangle(vertices[i-2].u, vertices[i-2].v, vertices[i-1].u, vertices[i-1].v, vertices[i-0].u, vertices[i-0].v)
			);
	#endif /* LOG_VERTICES */

bail:
	return err;
}


/*******************************************************************************
 * MakeSrcDstRects
 *******************************************************************************/

static FskErr MakeSrcDstRects(FskConstBitmap src, FskConstRectangle srcRect, FskConstRectangle dstBounds, FskConstPoint dstPoint, FskRectangle sRect, FskRectangle dRect) {
	FskErr			err		= kFskErrNone;
#if 0
	FskPointRecord	delta;

	if (!dstPoint)																							/* If the destination point is not specified ... */
		dstPoint = (FskConstPoint)dstBounds;																/* ... it is the upper left of the destination */
	delta.x = dstPoint->x;																					/* Compute motion vector */
	delta.y = dstPoint->y;
	if (srcRect) {
		delta.x -= srcRect->x;																				/* Compute motion vector from src rect to dst */
		delta.y -= srcRect->y;
		BAIL_IF_FALSE(FskRectangleIntersect(srcRect, &src->bounds, sRect), err, kFskErrNothingRendered);	/* Intersect src rect with bounds */
	}
	else {
		delta.x -= src->bounds.x;																			/* Compute motion vector from src bounds to dst */
		delta.y -= src->bounds.y;
		*sRect = src->bounds;																				/* Src rect is the bounds */
	}
	FskRectangleSet(dRect, sRect->x + delta.x, sRect->y + delta.y, sRect->width, sRect->height);			/* Offset src rect to dst space */
	BAIL_IF_FALSE(FskRectangleIntersect(dRect, dstBounds, dRect), err, kFskErrNothingRendered);				/* Intersect dst rect with bounds */
	FskRectangleSet(sRect, dRect->x - delta.x, dRect->y - delta.y, dRect->width, dRect->height);			/* Translate dst rect back to src space */
bail:
#else
	*sRect = srcRect ? *srcRect : src->bounds;
	if (dstPoint)	{ dRect->x =  dstPoint->x; dRect->y =  dstPoint->y; }
	else			{ dRect->x = dstBounds->x; dRect->y = dstBounds->y; }
	dRect->width  = sRect->width;
	dRect->height = sRect->height;
#endif
	return err;
}


#define ALWAYS_CLEAR_DST_TEXTURE 0

static const FskColorRGBARecord blank = { 0, 0, 0, 0 };


/********************************************************************************
 * SetEffectSrcDst
 ********************************************************************************/

static FskErr SetEffectSrcDst(FskConstBitmap src, FskConstRectangle srcRect, int srcWrapMode, int srcFilter, FskBitmap dst, FskConstPoint dstPoint) {
	FskErr				err		= kFskErrNone;
	FskRectangleRecord	sRect, dRect, texRect, dstPortRect;

	BAIL_IF_FALSE(src != dst, err, kFskErrDuplicateElement);										/* Assure that the src and dst are different */
	#if !ALWAYS_CLEAR_DST_TEXTURE
		if (FskGLFBOIsInited(dst->glPort))
			err = FskGLBitmapTextureTargetSet(dst);													/* Set dst as FBO but don't clear */
		else
	#endif /* ALWAYS_CLEAR_DST_TEXTURE */
	err = FskGLRenderToBitmapTexture(dst, &blank);													/* Set dst as FBO and clear */
	BAIL_IF_ERR(err);
	BAIL_IF_ERR(err = FskGLDstBMRectGet(dst, &dstPortRect));										/* Get dst glPort bounds */
	BAIL_IF_ERR(err = FskGLBindBMTexture(src, srcWrapMode, srcFilter));								/* Set src texture */
	BAIL_IF_ERR(err = FskGLPortTexRectGet(src->glPort, &texRect));									/* Get src texture bounds */
	BAIL_IF_ERR(err = MakeSrcDstRects(src, srcRect, &dstPortRect, dstPoint, &sRect, &dRect));		/* Clip and determine rects */
	BAIL_IF_ERR(err = SetEffectVertices(&sRect, &texRect, &dRect));									/* Set the vertices */

bail:
	return err;
}


/********************************************************************************
 * SetEffectMirrorRepeatSrcDst
 ********************************************************************************/

static FskErr SetEffectMirrorRepeatSrcDst(int border, FskConstBitmap src, FskConstRectangle srcRect, FskBitmap dst, FskConstPoint dstPoint) {
	FskErr				err		= kFskErrNone;
	FskRectangleRecord	sRect, dRect, texRect, dstPortRect;

	BAIL_IF_FALSE(src != dst, err, kFskErrDuplicateElement);										/* Assure that the src and dst are different */
	#if !ALWAYS_CLEAR_DST_TEXTURE
		if (FskGLFBOIsInited(dst->glPort))
			err = FskGLBitmapTextureTargetSet(dst);													/* Set dst as FBO but don't clear */
		else
	#endif /* ALWAYS_CLEAR_DST_TEXTURE */
	err = FskGLRenderToBitmapTexture(dst, &blank);													/* Set dst as FBO and clear */
	BAIL_IF_ERR(err = FskGLDstBMRectGet(dst, &dstPortRect));										/* Get dst glPort bounds */
	BAIL_IF_ERR(err = FskGLBindBMTexture(src, GL_CLAMP_TO_EDGE, GL_NEAREST));						/* Set src texture */
	BAIL_IF_ERR(err = FskGLPortTexRectGet(src->glPort, &texRect));									/* Get src texture bounds */
	BAIL_IF_ERR(err = MakeSrcDstRects(src, srcRect, &dstPortRect, dstPoint, &sRect, &dRect));		/* Clip and determine rects */
	BAIL_IF_ERR(err = SetReflectBorderVertices(border, &sRect, &texRect, &dRect));					/* Set the vertices */

bail:
	return err;
}


/********************************************************************************
 * SetMatteEffectSrcDst
 ********************************************************************************/

static FskErr SetMatteEffectSrcDst(FskConstBitmap src, FskConstRectangle srcRect, FskConstBitmap mtt, FskConstPoint mttPt, FskBitmap dst, FskConstPoint dstPt) {
	FskErr				err		= kFskErrNone;
	FskRectangleRecord	sRect, dRect, mRect, texRect, mtxRect;
	GLint				mattQual;

	BAIL_IF_ERR(err = FskGLBitmapTextureTargetSet(dst));											/* Set dst as FBO */
	sRect = srcRect ? *srcRect : src->bounds;														/* Determine rects */
	if (mttPt)	FskRectangleSet(&mRect, mttPt->x, mttPt->y, sRect.width, sRect.height);
	else		mRect = mtt->bounds;
	if (dstPt)	{ dRect.x =      dstPt->x; dRect.y =      dstPt->y; }
	else		{ dRect.x = dst->bounds.x; dRect.y = dst->bounds.y; }
	dRect.width  = sRect.width;
	dRect.height = sRect.height;

	glActiveTexture(GL_TEXTURE1);
	mattQual = (sRect.width == mRect.width && sRect.height == mRect.height) ? GL_NEAREST : GL_LINEAR;
	BAIL_IF_ERR(err = FskGLBindBMTexture(mtt, GL_CLAMP_TO_EDGE, mattQual));							/* Set matte texture */

	glActiveTexture(GL_TEXTURE0);
	BAIL_IF_ERR(err = FskGLBindBMTexture(src, GL_CLAMP_TO_EDGE, GL_NEAREST));						/* Set src texture */

	BAIL_IF_ERR(err = FskGLPortTexRectGet(src->glPort, &texRect));									/* Get src texture bounds */
	BAIL_IF_ERR(err = FskGLPortTexRectGet(mtt->glPort, &mtxRect));									/* Get mtt texture bounds */

	BAIL_IF_ERR(err = SetMatteEffectVertices(&sRect, &texRect, &mRect, &mtxRect, &dRect));			/* Set the vertices */

bail:
	return err;
}


/********************************************************************************
 * SetUniformColor
 ********************************************************************************/

static void SetUniformColor(GLint location, FskConstColorRGBA color) {
	float f[4];
	f[0] = color->r * (1.f / 255.f);
	f[1] = color->g * (1.f / 255.f);
	f[2] = color->b * (1.f / 255.f);
	f[3] = color->a * (1.f / 255.f);
	glUniform4fv(location, 1, f);
}


/********************************************************************************
 * SetPremultipliedUniformColor
 ********************************************************************************/

static void SetPremultipliedUniformColor(GLint location, FskConstColorRGBA color) {
	float f[4];
	float preNorm = color->a / (255.f * 255.f);
	f[0] = color->r * preNorm;
	f[1] = color->g * preNorm;
	f[2] = color->b * preNorm;
	f[3] = color->a * (1.f / 255.f);
	glUniform4fv(location, 1, f);
}


/********************************************************************************
 * BothBitmapsHaveAlpha
 ********************************************************************************/

static int BothBitmapsHaveAlpha(FskConstBitmap bm1, FskConstBitmap bm2) {
	Boolean hasAlpha;
	FskBitmapGetHasAlpha(bm1, &hasAlpha);
	if (hasAlpha)
		FskBitmapGetHasAlpha(bm2, &hasAlpha);
	return hasAlpha ? kFskGLEffectCacheBitmapWithAlpha : 0;
}


/*******************************************************************************
 * TruncateToMidFloat
 *******************************************************************************/

static float TruncateToMidFloat(float f, unsigned bits) {
	typedef union TFlint {
		float	f;
		int		i;
	} TFlint;
	TFlint x;
	x.f = f;
	if (bits < 24)	x.i &= -1 << (24 - bits);
	return x.f;
}


/*******************************************************************************
 * GaussianCoeffRadFromSigma
 *******************************************************************************/

#define kSigmaCutoff		2.5f
#define MAX_LOOP_RADIUS_VAL	32
#define MAX_LOOP_RADIUS		"32"


static void GaussianCoeffRadFromSigma(float sigma, float coeff[3], GLint *radius) {
	float	acc;
	int		i;
	float	scale	= expf(-.5f / (sigma * sigma));													/* Compute the Gaussian attenuation factor */
	*radius = (GLint)(sigma * kSigmaCutoff);														/* We need to go out this far so that the truncation is negligible */
	//if (*radius > MAX_LOOP_RADIUS_VAL)															// TODO: We only want to limit this on devices that need to have loops unrolled.
	//	*radius = MAX_LOOP_RADIUS_VAL;
	coeff[0] = 1.f;
	if (gEffectsGlobals.hasHighPrecision) {
		coeff[1] = scale;
		coeff[2] = scale * scale;
		for (i = *radius, acc = .5f; i--;) {
			coeff[0] *= coeff[1];
			coeff[1] *= coeff[2];
			acc      += coeff[0];																	/* Compute normalization */
		}
		coeff[0] = .5f / acc;																		/* Normalize coefficients so that they sum to 1.0 */
	}
	else {
		coeff[2] = TruncateToMidFloat(scale * scale, gEffectsGlobals.mediumPrecisionBits);
		coeff[1] = scale = TruncateToMidFloat(scale, gEffectsGlobals.mediumPrecisionBits);
		for (i = *radius, acc = .5f; i--;) {
			coeff[0] = TruncateToMidFloat(coeff[0] * coeff[1], gEffectsGlobals.mediumPrecisionBits);
			coeff[1] = TruncateToMidFloat(coeff[1] * coeff[2], gEffectsGlobals.mediumPrecisionBits);
			acc = TruncateToMidFloat(acc +  coeff[0], gEffectsGlobals.mediumPrecisionBits);			/* Compute normalization */
		}

		coeff[0] = .5f / acc;																		/* Normalize coefficients so that they sum to 1.0 */
		coeff[0] *= 1.003f;																			/* Compensate for truncation rather than rounding */
		coeff[0] = TruncateToMidFloat(coeff[0], gEffectsGlobals.mediumPrecisionBits);
	}
	coeff[1] = scale;																				/* Restore the Gaussian attenuation factor */
}


#if 0
#pragma mark -
#endif /* 0 */
/********************************************************************************
 ********************************************************************************
 ********************************************************************************
 *****							Shaders										*****
 ********************************************************************************
 ********************************************************************************
 ********************************************************************************/

#if 0
#pragma mark gEffectsVertexShader
#endif
static const char gEffectsVertexShader[] = {
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
#if 0
#pragma mark gMatteVertexShader
#endif
static const char gMatteVertexShader[] = {
	"uniform   mat3 matrix;\n"
	"attribute vec2 vPosition;\n"
	"attribute vec4 vTexCoord;\n"
	"varying   vec4 srcCoord;\n"
	"void main() {\n"
	"	gl_Position.xyw = matrix * vec3(vPosition, 1.);\n"
	"	gl_Position.z = 0.;\n"
	"	srcCoord = vTexCoord;\n"
	"}\n"
	};

#if 0
#pragma mark gCopyFragmentShader
#endif
static const char gCopyFragmentShader[] = {
	"uniform " LOWP "sampler2D srcBM;\n"
	"varying " HIGHP "vec2 srcCoord;\n"
	"void main() {\n"
	"	gl_FragColor = texture2D(srcBM, srcCoord);\n"
	"}\n"
};

#if 0
#pragma mark gGelFragmentShader
#endif
static const char gGelFragmentShader[] = {
	"uniform " LOWP "sampler2D srcBM;\n"
	"uniform " LOWP "vec4 color;\n"
	"varying " HIGHP "vec2 srcCoord;\n"
	"void main() {\n"
	"	gl_FragColor = texture2D(srcBM, srcCoord) * color;\n"
	"}\n"
};

#if 0
#pragma mark gGaussianBlurFragmentShader
#endif
#if !TARGET_OS_WIN32
static const char gGaussianBlurFragmentShaderU[] = {										/* Shader for GPUs without looping, such as the NVidia Tegra */
	"uniform	"MEDIUMP"	vec3		coeff;\n"											/* g0 = 1/(sigma*sqrt(2*pi)), g1=exp(-1/(2*sigma^2)), g2=exp(-1/(sigma^2)) */
	"uniform	"LOWP"		int			rad;\n"												/* Radius of filter extent */
	"uniform	"LOWP"		sampler2D	srcBM;\n"											/* Source texture */
	"uniform	"HIGHP"		vec2		srcDelta;\n"										/* Delta between successive samples. {dx, dy} */
	"varying	"HIGHP"		vec2		srcCoord;\n"										/* Central source coordinate. {x, y} */
	"void main() {\n"
	"	"MEDIUMP"	vec3	g	= coeff;\n"													/* Simulations indicate that the computations are stable even at low precision */
	"	"MEDIUMP"	vec4	acc	= texture2D(srcBM, srcCoord) * g.x;\n"						/* Central value scaled by central coefficient */
	"	for ("LOWP" int r = 1; r <= "MAX_LOOP_RADIUS" && r <= rad; ++r) {\n"				/* Unrollable syntax needed for Nvidia Tegra */
	"		g.xy *= g.yz;\n"																/* Generate the next coefficient */
	"		acc  += texture2D(srcBM, srcCoord + float(r) * srcDelta) * g.x;\n"
	"		acc  += texture2D(srcBM, srcCoord - float(r) * srcDelta) * g.x;\n"
	"	}\n"
	"	gl_FragColor = acc;\n"
	"}\n"
};
static const char gGaussianBlurFragmentShaderV[] = {										/* Shader for GPUs without looping, such as the NVidia Tegra */
	"uniform	"MEDIUMP"	vec3		coeff;\n"											/* g0 = 1/(sigma*sqrt(2*pi)), g1=exp(-1/(2*sigma^2)), g2=exp(-1/(sigma^2)) */
	"uniform	"LOWP"		int			rad;\n"												/* Radius of filter extent */
	"uniform	"LOWP"		sampler2D	srcBM;\n"											/* Source texture */
	"uniform	"HIGHP"		vec2		srcDelta;\n"										/* Delta between successive samples. {dx, dy} */
	"varying	"HIGHP"		vec2		srcCoord;\n"										/* Central source coordinate. {x, y} */
	"void main() {\n"
	"	"HIGHP"	vec3	g	= coeff;\n"														/* Simulations indicate that the computations are stable even at low precision */
	"	"HIGHP"	vec4	acc	= texture2D(srcBM, srcCoord) * g.x;\n"							/* Central value scaled by central coefficient */
	"	for ("LOWP" int r = 1; r <= "MAX_LOOP_RADIUS" && r <= rad; ++r) {\n"				/* Unrollable syntax needed for Nvidia Tegra */
	"		g.xy *= g.yz;\n"																/* Generate the next coefficient */
	"		acc  += texture2D(srcBM, srcCoord + float(r) * srcDelta) * g.x;\n"
	"		acc  += texture2D(srcBM, srcCoord - float(r) * srcDelta) * g.x;\n"
	"	}\n"
	"	gl_FragColor = acc;\n"
	"}\n"
};
static const char gGaussianBlurFragmentShaderH[] = {										/* For Adreno 320, which has a bug with mediump */
	"uniform	"HIGHP"		vec3		coeff;\n"											/* g0 = 1/(sigma*sqrt(2*pi)), g1=exp(-1/(2*sigma^2)), g2=exp(-1/(sigma^2)) */
	"uniform	"LOWP"		int			rad;\n"												/* Radius of filter extent */
	"uniform	"LOWP"		sampler2D	srcBM;\n"											/* Source texture */
	"uniform	"HIGHP"		vec2		srcDelta;\n"										/* Delta between successive samples. {dx, dy} */
	"varying	"HIGHP"		vec2		srcCoord;\n"										/* Central source coordinate. {x, y} */
	"void main() {\n"
	"	"HIGHP"	vec3	g	= coeff;\n"														/* Tests indicate that mediump float is actually computed as fixed point. */
	"	"HIGHP"	vec4	acc	= texture2D(srcBM, srcCoord) * g.x;\n"							/* Central value scaled by central coefficient */
	"	for ("LOWP" int r = 1; r <= rad; r++) {\n"
	"		g.xy *= g.yz;\n"																/* Generate the next coefficient */
	"		acc  += texture2D(srcBM, srcCoord + float(r) * srcDelta) * g.x;\n"
	"		acc  += texture2D(srcBM, srcCoord - float(r) * srcDelta) * g.x;\n"
	"	}\n"
	"	gl_FragColor = acc;\n"
	"}\n"
};
static const char gGaussianBlurFragmentShaderL[] = {
	"uniform	"MEDIUMP"	vec3		coeff;\n"											/* g0 = 1/(sigma*sqrt(2*pi)), g1=exp(-1/(2*sigma^2)), g2=exp(-1/(sigma^2)) */
	"uniform	"LOWP"		int			rad;\n"												/* Radius of filter extent */
	"uniform	"LOWP"		sampler2D	srcBM;\n"											/* Source texture */
	"uniform	"HIGHP"		vec2		srcDelta;\n"										/* Delta between successive samples. {dx, dy} */
	"varying	"HIGHP"		vec2		srcCoord;\n"										/* Central source coordinate. {x, y} */
	"void main() {\n"
	"	"MEDIUMP"	vec3	g	= coeff;\n"													/* Simulations indicate that the computations are stable even at low precision */
	"	"MEDIUMP"	vec4	acc	= texture2D(srcBM, srcCoord) * g.x;\n"						/* Central value scaled by central coefficient */
	"	for ("LOWP" int r = 1; r <= rad; ++r) {\n"
	"		g.xy *= g.yz;\n"																/* Generate the next coefficient */
	"		acc  += texture2D(srcBM, srcCoord + float(r) * srcDelta) * g.x;\n"
	"		acc  += texture2D(srcBM, srcCoord - float(r) * srcDelta) * g.x;\n"
	"	}\n"
	"	gl_FragColor = acc;\n"
	"}\n"
};
#else /* TARGET_OS_WIN32 */
static const char gGaussianBlurFragmentShaderW[] = {										/* Shader for ANGLE on Windows */
	"uniform	"MEDIUMP"	vec3		coeff;\n"											/* g0 = 1/(sigma*sqrt(2*pi)), g1=exp(-1/(2*sigma^2)), g2=exp(-1/(sigma^2)) */
	"uniform	"LOWP"		int			rad;\n"												/* Radius of filter extent */
	"uniform	"LOWP"		sampler2D	srcBM;\n"											/* Source texture */
	"uniform	"HIGHP"		vec2		srcDelta;\n"										/* Delta between successive samples. {dx, dy} */
	"varying	"HIGHP"		vec2		srcCoord;\n"										/* Central source coordinate. {x, y} */
	"void main() {\n"
	"	"MEDIUMP"	vec3	g	= coeff;\n"													/* Simulations indicate that the computations are stable even at low precision */
	"	"MEDIUMP"	vec4	acc	= texture2D(srcBM, srcCoord) * g.x;\n"						/* Central value scaled by central coefficient */
	"	"LOWP"		int		rmx	= rad <= "MAX_LOOP_RADIUS" ? rad : "MAX_LOOP_RADIUS";\n"	/* Specify a max value in order to unroll */
	"	for ("LOWP" int r = 1; r <= rmx; ++r) {\n"											/* Unrollable syntax needed for ANGLE on Windows */
	"		g.xy *= g.yz;\n"																/* Generate the next coefficient */
	"		acc  += texture2D(srcBM, srcCoord + float(r) * srcDelta) * g.x;\n"
	"		acc  += texture2D(srcBM, srcCoord - float(r) * srcDelta) * g.x;\n"
	"	}\n"
	"	gl_FragColor = acc;\n"
	"}\n"
};
#endif /* TARGET_OS_WIN32 */

#if 0
#pragma mark gDilateFragmentShader
#endif
#if !TARGET_OS_WIN32
static const char gDilateFragmentShaderU[] = {												/* Shader for GPUs without looping, such as the NVidia Tegra */
	"uniform	"LOWP"		int			rad;\n"												/* Radius of effect */
	"uniform	"LOWP"		sampler2D	srcBM;\n"											/* Source texture */
	"varying	"HIGHP"		vec2		srcCoord;\n"										/* Central source coordinate. {x, y} */
	"uniform	"HIGHP"		vec2		srcDelta;\n"										/* Delta between successive samples. {dx, dy} */
	"void main() {\n"
	"	gl_FragColor = texture2D(srcBM, srcCoord);\n"
	"	for ("LOWP" int r = 1; r <= "MAX_LOOP_RADIUS" && r <= rad; ++r) {\n"				/* Unrollable syntax needed for Nvidia Tegra */
	"		gl_FragColor = max(gl_FragColor, texture2D(srcBM, srcCoord + float(r) * srcDelta));\n"	/* Next source pixel in the positive direction */
	"		gl_FragColor = max(gl_FragColor, texture2D(srcBM, srcCoord - float(r) * srcDelta));\n"	/* Next source pixel in the negative direction */
	"	}\n"
	"}\n"
};
static const char gDilateFragmentShaderL[] = {
	"uniform	"LOWP"		int			rad;\n"												/* Radius of effect */
	"uniform	"LOWP"		sampler2D	srcBM;\n"											/* Source texture */
	"varying	"HIGHP"		vec2		srcCoord;\n"										/* Central source coordinate. {x, y} */
	"uniform	"HIGHP"		vec2		srcDelta;\n"										/* Delta between successive samples. {dx, dy} */
	"void main() {\n"
	"	gl_FragColor = texture2D(srcBM, srcCoord);\n"
	"	for ("LOWP" int r = 1; r <= rad; ++r) {\n"
	"		gl_FragColor = max(gl_FragColor, texture2D(srcBM, srcCoord + float(r) * srcDelta));\n"	/* Next source pixel in the positive direction */
	"		gl_FragColor = max(gl_FragColor, texture2D(srcBM, srcCoord - float(r) * srcDelta));\n"	/* Next source pixel in the negative direction */
	"	}\n"
	"}\n"
};
#else /* TARGET_OS_WIN32 */
static const char gDilateFragmentShaderW[] = {												/* Shader for ANGLE on Windows */
	"uniform	"LOWP"		int			rad;\n"												/* Radius of effect */
	"uniform	"LOWP"		sampler2D	srcBM;\n"											/* Source texture */
	"varying	"HIGHP"		vec2		srcCoord;\n"										/* Central source coordinate. {x, y} */
	"uniform	"HIGHP"		vec2		srcDelta;\n"										/* Delta between successive samples. {dx, dy} */
	"void main() {\n"
	"	"LOWP" int rmx	= rad <= "MAX_LOOP_RADIUS" ? rad : "MAX_LOOP_RADIUS";\n"			/* Specify a max value in order to unroll */
	"	gl_FragColor = texture2D(srcBM, srcCoord);\n"
	"	for ("LOWP" int r = 1; r <= rmx; ++r) {\n"											/* Unrollable syntax needed for ANGLE on Windows */
	"		gl_FragColor = max(gl_FragColor, texture2D(srcBM, srcCoord + float(r) * srcDelta));\n"	/* Next source pixel in the positive direction */
	"		gl_FragColor = max(gl_FragColor, texture2D(srcBM, srcCoord - float(r) * srcDelta));\n"	/* Next source pixel in the negative direction */
	"	}\n"
	"}\n"
};
#endif /* TARGET_OS_WIN32 */

#if 0
#pragma mark gErodeFragmentShader
#endif
#if !TARGET_OS_WIN32
static const char gErodeFragmentShaderU[] = {												/* Shader for GPUs without looping, such as the NVidia Tegra */
	"uniform	"LOWP"		int			rad;\n"												/* Radius of effect */
	"uniform	"LOWP"		sampler2D	srcBM;\n"											/* Source texture */
	"varying	"HIGHP"		vec2		srcCoord;\n"										/* Central source coordinate. {x, y} */
	"uniform	"HIGHP"		vec2		srcDelta;\n"										/* Delta between successive samples. {dx, dy} */
	"void main() {\n"
	"	gl_FragColor = texture2D(srcBM, srcCoord);\n"
	"	for ("LOWP" int r = 1; r <= "MAX_LOOP_RADIUS" && r <= rad; ++r) {\n"				/* Unrollable syntax needed for Nvidia Tegra */
	"		gl_FragColor = min(gl_FragColor, texture2D(srcBM, srcCoord + float(r) * srcDelta));\n"	/* Next source pixel in the positive direction */
	"		gl_FragColor = min(gl_FragColor, texture2D(srcBM, srcCoord - float(r) * srcDelta));\n"	/* Next source pixel in the negative direction */
	"	}\n"
	"}\n"
};
static const char gErodeFragmentShaderL[] = {
	"uniform	"LOWP"		int			rad;\n"												/* Radius of effect */
	"uniform	"LOWP"		sampler2D	srcBM;\n"											/* Source texture */
	"varying	"HIGHP"		vec2		srcCoord;\n"										/* Central source coordinate. {x, y} */
	"uniform	"HIGHP"		vec2		srcDelta;\n"										/* Delta between successive samples. {dx, dy} */
	"void main() {\n"
	"	gl_FragColor = texture2D(srcBM, srcCoord);\n"
	"	for ("LOWP" int r = 1; r <= rad; ++r) {\n"
	"		gl_FragColor = min(gl_FragColor, texture2D(srcBM, srcCoord + float(r) * srcDelta));\n"	/* Next source pixel in the positive direction */
	"		gl_FragColor = min(gl_FragColor, texture2D(srcBM, srcCoord - float(r) * srcDelta));\n"	/* Next source pixel in the negative direction */
	"	}\n"
	"}\n"
};
#else /* TARGET_OS_WIN32 */
static const char gErodeFragmentShaderW[] = {												/* Shader for ANGLE on Windows */
	"uniform	"LOWP"		int			rad;\n"												/* Radius of effect */
	"uniform	"LOWP"		sampler2D	srcBM;\n"											/* Source texture */
	"varying	"HIGHP"		vec2		srcCoord;\n"										/* Central source coordinate. {x, y} */
	"uniform	"HIGHP"		vec2		srcDelta;\n"										/* Delta between successive samples. {dx, dy} */
	"void main() {\n"
	"	"LOWP" int rmx	= rad <= "MAX_LOOP_RADIUS" ? rad : "MAX_LOOP_RADIUS";\n"			/* Specify a max value in order to unroll */
	"	gl_FragColor = texture2D(srcBM, srcCoord);\n"
	"	for ("LOWP" int r = 1; r <= rmx; ++r) {\n"											/* Unrollable syntax needed for ANGLE on Windows */
	"		gl_FragColor = min(gl_FragColor, texture2D(srcBM, srcCoord + float(r) * srcDelta));\n"	/* Next source pixel in the positive direction */
	"		gl_FragColor = min(gl_FragColor, texture2D(srcBM, srcCoord - float(r) * srcDelta));\n"	/* Next source pixel in the negative direction */
	"	}\n"
	"}\n"
};
#endif /* TARGET_OS_WIN32 */

#if 0
#pragma mark gBoxBlurFragmentShader
#endif
#if !TARGET_OS_WIN32
static const char gBoxBlurFragmentShaderU[] = {												/* Shader for GPUs without looping, such as the NVidia Tegra */
	"uniform	"LOWP"		int			rad;\n"												/* Radius of filter extent */
	"uniform	"LOWP"		sampler2D	srcBM;\n"											/* Source texture */
	"uniform	"HIGHP"		vec3		srcDelta;\n"										/* Delta between successive samples, plus norm. {dx, dy, norm} */
	"varying	"HIGHP"		vec2		srcCoord;\n"										/* Central source coordinate. {x, y} */
	"void main() {\n"
	"	"MEDIUMP"	vec4	acc	= texture2D(srcBM, srcCoord);\n"							/* Central value */
	"	for ("LOWP" int i = 1; i <= "MAX_LOOP_RADIUS" && i <= rad; ++i) {\n"				/* Unrollable syntax needed for Nvidia Tegra */
	"		acc += texture2D(srcBM, srcCoord + float(i) * srcDelta.xy);\n"
	"		acc += texture2D(srcBM, srcCoord - float(i) * srcDelta.xy);\n"
	"	}\n"
	"	gl_FragColor = acc * srcDelta.z;\n"
	"}\n"
};
static const char gBoxBlurFragmentShaderL[] = {
	"uniform	"LOWP"		int			rad;\n"												/* Radius of filter extent */
	"uniform	"LOWP"		sampler2D	srcBM;\n"											/* Source texture */
	"uniform	"HIGHP"		vec3		srcDelta;\n"										/* Delta between successive samples, plus norm. {dx, dy, norm} */
	"varying	"HIGHP"		vec2		srcCoord;\n"										/* Central source coordinate. {x, y} */
	"void main() {\n"
	"	"MEDIUMP"	vec4	acc	= texture2D(srcBM, srcCoord);\n"							/* Central value */
	"	for ("LOWP" int r = 1; r <= rad; ++r) {\n"
	"		acc += texture2D(srcBM, srcCoord + float(r) * srcDelta.xy);\n"
	"		acc += texture2D(srcBM, srcCoord - float(r) * srcDelta.xy);\n"
	"	}\n"
	"	gl_FragColor = acc * srcDelta.z;\n"
	"}\n"
};
#else /* TARGET_OS_WIN32 */
static const char gBoxBlurFragmentShaderW[] = {												/* Shader for ANGLE on Windows */
	"uniform	"LOWP"		int			rad;\n"												/* Radius of filter extent */
	"uniform	"LOWP"		sampler2D	srcBM;\n"											/* Source texture */
	"uniform	"HIGHP"		vec3		srcDelta;\n"										/* Delta between successive samples, plus norm. {dx, dy, norm} */
	"varying	"HIGHP"		vec2		srcCoord;\n"										/* Central source coordinate. {x, y} */
	"void main() {\n"
	"	"MEDIUMP"	vec4	acc	= texture2D(srcBM, srcCoord);\n"							/* Central value */
	"	"LOWP"		int		rmx	= rad <= "MAX_LOOP_RADIUS" ? rad : "MAX_LOOP_RADIUS";\n"	/* Specify a max value in order to unroll */
	"	for ("LOWP" int i = 1; i <= rmx; ++i) {\n"											/* Unrollable syntax needed for Angle on Windows */
	"		acc += texture2D(srcBM, srcCoord + float(i) * srcDelta.xy);\n"
	"		acc += texture2D(srcBM, srcCoord - float(i) * srcDelta.xy);\n"
	"	}\n"
	"	gl_FragColor = acc * srcDelta.z;\n"
	"}\n"
};
#endif /* TARGET_OS_WIN32 */

#if 0
#pragma mark gColorizeAlphaFragmentShader
#endif
static const char gColorizeAlphaFragmentShader[] = {
	"uniform	"LOWP"		sampler2D	srcBM;\n"							/* Source texture */
	"uniform	"LOWP"		vec4		color0;\n"							/* Color corresponding to alpha=0 */
	"uniform	"LOWP"		vec4		color1;\n"							/* Color corresponding to alpha=1 */
	"varying	"HIGHP"		vec2		srcCoord;\n"						/* Source coordinate. {x, y} */
	"void main() {\n"
	"	gl_FragColor = mix(color0, color1, texture2D(srcBM, srcCoord).a);\n"
	"}\n"
};

/* LUMA601 or LUMA709 is defined in FskPixelOps.h */
#ifdef LUMA601			/* CCIR 601 & JPEG/JFIF Luminance */
	#define R_TO_Y		"0.299"
	#define G_TO_Y		"0.587"
	#define B_TO_Y		"0.114"
#elif defined(LUMA709)	/* 709 */
	#define R_TO_Y		"0.2126"
	#define G_TO_Y		"0.7152"
	#define B_TO_Y		"0.0722"
#else
	#error Please choose a standard for luminance.
#endif

#if 0
#pragma mark gMonochromeStraightFragmentShader
#endif
static const char gMonochromeStraightFragmentShader[] = {
	"uniform	"LOWP"		vec4		color0;\n"							/* Color corresponding to gray=0 */
	"uniform	"LOWP"		vec4		color1;\n"							/* Color corresponding to gray=1 */
	"uniform	"LOWP"		sampler2D	srcBM;\n"							/* Source texture */
	"varying	"HIGHP"		vec2		srcCoord;\n"						/* Central source coordinate. {x, y} */
	"void main() {\n"
	"	"LOWP" float alpha;\n"
	"	gl_FragColor = texture2D(srcBM, srcCoord);\n"
	"	alpha = gl_FragColor.a;\n"
	"	gl_FragColor = mix(color0, color1, dot(gl_FragColor.rgb, vec3("R_TO_Y", "G_TO_Y", "B_TO_Y")));\n"
	"	gl_FragColor.a *= alpha;\n"
	"}\n"
};

#if 0
#pragma mark gMonochromePremulFragmentShader
#endif
static const char gMonochromePremulFragmentShader[] = {
	"uniform	"LOWP"		vec4		color0;\n"							/* Color corresponding to gray=0 */
	"uniform	"LOWP"		vec4		color1;\n"							/* Color corresponding to gray=1 */
	"uniform	"LOWP"		sampler2D	srcBM;\n"							/* Source texture */
	"varying	"HIGHP"		vec2		srcCoord;\n"						/* Central source coordinate. {x, y} */
	"void main() {\n"
	"	gl_FragColor = texture2D(srcBM, srcCoord);\n"
	"	if (gl_FragColor.a != 0.0) {\n"										/* If alpha is zero, the whole pixel is zero */
	"		gl_FragColor = mix(color0, color1, clamp(dot(gl_FragColor.rgb, vec3("R_TO_Y", "G_TO_Y", "B_TO_Y")) / gl_FragColor.a, 0.0, 1.0)) * gl_FragColor.a;\n"
	"	}\n"
	"}\n"
};

#if 0
#pragma mark gColorizeInnerStraightFragmentShader
#endif
static const char gColorizeInnerStraightFragmentShader[] = {
	"uniform	"LOWP"		sampler2D	srcBM;\n"							/* Source texture */
	"uniform	"LOWP"		sampler2D	mttBM;\n"							/* Matte texture */
	"uniform	"LOWP"		vec4		color;\n"							/* Color */
	"varying	"HIGHP"		vec4		srcCoord;\n"						/* Source coordinate {x, y}. Matte  coordinate {z, w} */
	"void main() {\n"
	"	"LOWP" vec4 src = texture2D(srcBM, srcCoord.xy);\n"
	"	gl_FragColor.a = src.a;\n"
	"	gl_FragColor.rgb = mix(src.rgb, color.rgb, (1.0 - texture2D(mttBM, srcCoord.zw).a) * color.a);\n"
	"}\n"
};

#if 0
#pragma mark gColorizeInnerPremulFragmentShader
#endif
static const char gColorizeInnerPremulFragmentShader[] = {
	"uniform	"LOWP"		sampler2D	srcBM;\n"							/* Source texture */
	"uniform	"LOWP"		sampler2D	mttBM;\n"							/* Matte texture */
	"uniform	"LOWP"		vec4		color;\n"							/* Color */
	"varying	"HIGHP"		vec4		srcCoord;\n"						/* Source coordinate {x, y}. Matte  coordinate {z, w} */
	"void main() {\n"
	"	"LOWP" vec4 src = texture2D(srcBM, srcCoord.xy);\n"
	"	gl_FragColor.a = src.a;\n"
	"	gl_FragColor.rgb = mix(src.rgb, src.a * color.rgb, (1.0 - texture2D(mttBM, srcCoord.zw).a) * color.a);\n"
	"}\n"
};

#if 0
#pragma mark gColorizeOuterStraightFragmentShader
#endif
static const char gColorizeOuterStraightFragmentShader[] = {
	"uniform	"LOWP"		sampler2D	srcBM;\n"							/* Source texture */
	"uniform	"LOWP"		sampler2D	mttBM;\n"							/* Matte texture */
	"uniform	"LOWP"		vec4		color;\n"							/* Color */
	"varying	"HIGHP"		vec4		srcCoord;\n"						/* Source coordinate {x, y}. Matte  coordinate {z, w} */
	"void main() {\n"
	"	"LOWP" vec4  src = texture2D(srcBM, srcCoord.xy);\n"
	"	gl_FragColor.a   = (texture2D(mttBM, srcCoord.zw).a * (1.0 - src.a)) * color.a + src.a;\n"
	"	if (gl_FragColor.a != 0.0) src.a /= gl_FragColor.a;\n"
	"	gl_FragColor.rgb = mix(color.rgb, src.rgb, src.a);\n"
	"}\n"
};

#if 0
#pragma mark gColorizeOuterPremulFragmentShader
#endif
static const char gColorizeOuterPremulFragmentShader[] = {
	"uniform	"LOWP"		sampler2D	srcBM;\n"							/* Source texture */
	"uniform	"LOWP"		sampler2D	mttBM;\n"							/* Matte texture */
	"uniform	"LOWP"		vec4		color;\n"							/* Color */
	"varying	"HIGHP"		vec4		srcCoord;\n"						/* Source coordinate {x, y}. Matte  coordinate {z, w} */
	"void main() {\n"
	"	"LOWP" vec4 src = texture2D(srcBM, srcCoord.xy);\n"
	"	gl_FragColor = (texture2D(mttBM, srcCoord.zw).a * (1.0 - src.a)) * color + src;\n"
	"}\n"
};

#if 0
#pragma mark gColorizeStraightFragmentShader
#endif
static const char gColorizeStraightFragmentShader[] = {
	"uniform	"LOWP"		sampler2D	srcBM;\n"							/* Source texture */
	"uniform	"LOWP"		vec4		color;\n"							/* Color, straight */
	"varying	"HIGHP"		vec2		srcCoord;\n"						/* Source coordinate {x, y}. Matte  coordinate {z, w} */
	"void main() {\n"
	"	gl_FragColor = texture2D(srcBM, srcCoord);\n"
	"	gl_FragColor.rgb = mix(gl_FragColor.rgb, color.rgb, color.a);\n"
	"}\n"
};

#if 0
#pragma mark gColorizePremulFragmentShader
#endif
static const char gColorizePremulFragmentShader[] = {
	"uniform	"LOWP"		sampler2D	srcBM;\n"							/* Source texture */
	"uniform	"LOWP"		vec4		color;\n"							/* Color, straight */
	"varying	"HIGHP"		vec2		srcCoord;\n"						/* Source coordinate {x, y}. Matte  coordinate {z, w} */
	"void main() {\n"
	"	gl_FragColor = texture2D(srcBM, srcCoord);\n"
	"	gl_FragColor.rgb = mix(gl_FragColor.rgb, color.rgb * gl_FragColor.a, color.a);\n"
	"}\n"
};

#if 0
#pragma mark gStraightToPremultipliedFragmentShader
#endif
static const char gStraightToPremultipliedFragmentShader[] = {
	"uniform	"LOWP"		sampler2D	srcBM;\n"							/* Source texture */
	"varying	"HIGHP"		vec2		srcCoord;\n"						/* Central source coordinate. {x, y} */
	"void main() {\n"
	"	gl_FragColor = texture2D(srcBM, srcCoord);\n"
	"	gl_FragColor.rgb *= gl_FragColor.a;\n"								/* Scale RGB by alpha */
	"}\n"
};

#if 0
#pragma mark gPremultipliedToStraightFragmentShader
#endif
static const char gPremultipliedToStraightFragmentShader[] = {
	"uniform	"LOWP"		vec3		color;\n"							/* Background color */
	"uniform	"LOWP"		sampler2D	srcBM;\n"							/* Source texture */
	"varying	"HIGHP"		vec2		srcCoord;\n"						/* Central source coordinate. {x, y} */
	"void main() {\n"
	"	gl_FragColor = texture2D(srcBM, srcCoord);\n"
	"	if (gl_FragColor.a != 0.0)	gl_FragColor.rgb /= gl_FragColor.a;\n"	/* If alpha is nonzero, divide by it */
	"	else						gl_FragColor.rgb  = color;\n"			/* Otherwise, assign the background color */
	"}\n"
};


#if 0
#pragma mark -
#endif /* 0 */
/********************************************************************************
 ********************************************************************************
 ********************************************************************************
 *****									API									*****
 ********************************************************************************
 ********************************************************************************
 ********************************************************************************/


/********************************************************************************
 * FskGLEffectCacheDispose
 ********************************************************************************/

FskErr FskGLEffectCacheDispose(FskEffectCache cache) {
	FskErr		err			= kFskErrNone;
	unsigned	i;

	if (cache) {
		#ifdef LOG_PARAMETERS
			LOGD("FskGLEffectCacheDispose(%p): disposing %u bitmaps", cache, (unsigned)cache->numBitmaps);
		#endif /* LOG_PARAMETERS */

		for (i = cache->numBitmaps; i--;) {
			#ifdef LOG_CACHE
				LOGD("\t%u: BitmapDispose(%p)", i, cache->bitmaps[i]);
			#endif /* LOG_CACHE */
			FskBitmapDispose(cache->bitmaps[i]);
		}
		FskMemPtrDispose(cache);
	}
	#ifdef LOG_PARAMETERS
		else LOGD("FskGLEffectCacheDispose(%p)", cache);
	#endif /* LOG_PARAMETERS */
	return err;
}


/********************************************************************************
 * FskGLEffectCacheNew
 ********************************************************************************/

FskErr FskGLEffectCacheNew(FskEffectCache *pCache) {
	FskErr		err			= kFskErrNone;

	#ifdef LOG_PARAMETERS
		LOGD("FskGLEffectCacheNew()");
	#endif /* LOG_PARAMETERS */

	BAIL_IF_ERR(err = FskGLInit(NULL));	/* Make sure that GL is initialized */

	*pCache = NULL;
	BAIL_IF_ERR(err = FskMemPtrNewClear(sizeof(FskEffectCacheRecord), pCache));			/* Sets numBitmaps = 0 */
	BAIL_IF_ERR(err = FskMemPtrNewClear(4*sizeof(FskBitmap), &(**pCache).bitmaps));		/* Allocate 4 up front */

bail:
	return err;
}


/********************************************************************************
 * FskGLEffectCacheGetBitmap
 ********************************************************************************/

FskErr FskGLEffectCacheGetBitmap(FskEffectCache cache, unsigned width, unsigned height, int flags, FskBitmap *bmp) {
	FskErr				err			= kFskErrNone;
	FskBitmap			bm;

	#if defined(LOG_PARAMETERS) || defined(LOG_CACHE)
		LOGD("FskGLEffectCacheGetBitmap(%p, %u, %u, with%sAlpha%s)", cache, (unsigned)width, (unsigned)height,
			 ((flags & kFskGLEffectCacheBitmapWithAlpha) ? "" : "out"),
			 ((flags & kFskGLEffectCacheBitmapInit) ? " | init" : "")
		);
	#endif /* LOG_PARAMETERS || LOG_CACHE */

	*bmp = NULL;

	if (cache && cache->numBitmaps) {														/* We have at least one bitmap in the cache */
		bm = cache->bitmaps[--(cache->numBitmaps)];											/* TODO: If there is one that is already the desired size, use it. */
		#ifdef LOG_CACHE
		{	FskRectangleRecord texRect;
			FskGLPortTexRectGet(bm->glPort, &texRect);
			LOGD("\tFskGLEffectCacheGetBitmap grabs bitmap %p (#%u) from cache, leaving %d, resizing from %dx%d --> %ux%u",
				bm, FskGLPortSourceTexture(bm->glPort), cache->numBitmaps, texRect.width, texRect.height, width, height);
		}
		#endif /* LOG_CACHE */
		if ((unsigned)bm->bounds.width != width || (unsigned)bm->bounds.height != height) {
			bm->bounds.width	= width;
			bm->bounds.height	= height;
			bm->pixelFormat		= kFskBitmapFormatGLRGBA;
			BAIL_IF_ERR(err = FskGLPortResizeTexture(bm->glPort, GL_RGBA, width, height));	/* We hard-wire GL_RGBA because GL_RGB doesn't work cross platform */
			#if SUPPORT_INSTRUMENTATION
				if (FskGLFBOIsInited(bm->glPort))
					LOGE("\tERROR: FskGLPortResizeTexture returns an FBO-inited texture");
			#endif /* SUPPORT_INSTRUMENTATION */
		}
	}
	else {																					/* No bitmaps or no cache */
		BAIL_IF_ERR(err = FskBitmapNew(width, height, kFskBitmapFormatGLRGBA, &bm));		/* Allocate and store at that address */
		#ifdef LOG_CACHE
			LOGD("\tFskGLEffectCacheGetBitmap allocates new %ux%u RGBA bitmap %p (#%u)", (unsigned)width, (unsigned)height, bm, FskGLPortSourceTexture(bm->glPort));
		#endif /* LOG_CACHE */
		#if SUPPORT_INSTRUMENTATION
			if (FskGLFBOIsInited(bm->glPort))
				LOGE("\tERROR: FskBitmapNew returns an FBO-inited texture");
		#endif /* SUPPORT_INSTRUMENTATION */
	}
	FskBitmapSetHasAlpha(bm, ((flags & kFskGLEffectCacheBitmapWithAlpha) ? true : false));
	if (flags & kFskGLEffectCacheBitmapInit)
		FskGLFBOInit(bm->glPort);
	*bmp = bm;

bail:
	#ifdef LOG_CACHE
		if (err)	LOGD("\tFskGLEffectCacheGetBitmap() returns err=%d", (int)err);
	#endif /* LOG_CACHE */
	return err;
}


/********************************************************************************
 * GLEffectCacheGetSrcCompatibleBitmap
 ********************************************************************************/

static FskErr GLEffectCacheGetSrcCompatibleBitmap(FskEffectCache cache, FskConstBitmap src, unsigned width, unsigned height, FskBitmap *bmp) {
	FskErr err;
	BAIL_IF_ERR(err = FskGLEffectCacheGetBitmap(cache, width, height, (src->hasAlpha ? kFskGLEffectCacheBitmapWithAlpha : 0), bmp));
	(**bmp).alphaIsPremultiplied = src->alphaIsPremultiplied;
bail:
	return err;
}


/********************************************************************************
 * FskGLEffectCacheReleaseBitmap
 ********************************************************************************/

FskErr FskGLEffectCacheReleaseBitmap(FskEffectCache cache, FskBitmap bm) {
	FskErr err = kFskErrNone;

	if (!bm)
		goto bail;

	if (cache) {																							/* There is a cache */
		#if GLEFFECTS_DEBUG
			for (err = cache->numBitmaps; err--;) if (cache->bitmaps[err] == bm) {
				LOGE("FskGLEffectCacheReleaseBitmap: bitmap %p has already been released to the cache", bm);
				LogSrcBitmap(bm, NULL);
				bm = NULL;	/* Don't dispose bm in bail */
				BAIL(kFskErrDuplicateElement);
			}
			if (bm->useCount)
				LOGE("FskGLEffectCacheReleaseBitmap: bitmap %p has useCount=%d", bm, (int)(bm->useCount));
		#endif /* GLEFFECTS_DEBUG */
		BAIL_IF_NONZERO(bm->useCount, err, kFskErrIsBusy);													/* Someone else is using it, so we cannot put it into the cache */
		BAIL_IF_FALSE(FskBitmapIsOpenGLDestinationAccelerated(bm), err, kFskErrUnsupportedPixelType);		/* We only cache GL bitmaps */
		BAIL_IF_ERR(err = FskMemPtrRealloc(++(cache->numBitmaps) * sizeof(FskBitmap), &cache->bitmaps));
		cache->bitmaps[cache->numBitmaps - 1] = bm;
		FskGLUnbindBMTexture(bm);
		#if defined(LOG_PARAMETERS) || defined(LOG_CACHE)
			LOGD("FskGLEffectCacheReleaseBitmap(%p, %p), %u cached", cache, bm, cache->numBitmaps);
			LogSrcBitmap(bm, "cache");
		#endif /* LOG_PARAMETERS || LOG_CACHE */
	}
	else {																									/* There is no cache ... */
		#if defined(LOG_PARAMETERS) || defined(LOG_CACHE)
			LOGD("FskGLEffectCacheReleaseBitmap(%p, %p)", cache, bm);
			LogSrcBitmap(bm, "dispose");
		#endif /* LOG_PARAMETERS || LOG_CACHE */
		(void)FskBitmapDispose(bm);																			/* ... so we just dispose it */
	}

bail:
	if (err)	(void)FskBitmapDispose(bm);																	/* If we weren't able to cache it, dispose it */
	return err;
}


/****************************************************************************//**
 * Assure isolated pixels, so that appropriate boundary conditions can be applied.
 *	\param[in,out]	src			pointer to a place where the source bitmap pointer is stored. This may be replaced by a new temporary bitmap, tmpBM.
 *	\param[in,out]	srcRect		pointer to a place where the src rect is stored. Cannot be NULL, but *srcRect can be NULL. This may be replaced by the temp bounds.
 *	\param[in]		dst			the dst bitmap pointer.
 *	\param[in,out]	cache		pointer to a place where the cache pointer is stored. If *cache==NULL, then a new cache is allocated if needed, and returned in both tmpCache and cache.
 *	\param[out]		texRect		place to store the rect of the texture.
 *	\param[out]		tmpBM		place to store a temporary bitmap, if one is needed.
 *	\param[out]		tmpCache	place to store a temporary cache, if one is needed. Can be NULL if cache is non-NULL.
 ********************************************************************************/

static FskErr AssureIsolatedPixels(FskConstBitmap *src, FskConstRectangle *srcRect, FskConstBitmap dst, FskEffectCache *cache, FskRectangle texRect, FskBitmap *tmpBM, FskEffectCache *tmpCache) {
	FskErr				err			= kFskErrNone;

	*tmpBM    = NULL;											/* This signals whether or not we needed to use a temporary bitmap */
	if (tmpCache)	*tmpCache = NULL;							/* This might be NULL, if the caller supplies a non-NULL cache */
	if (!*srcRect)	*srcRect = &(**src).bounds;					/* If a srcRect was not given, assume the whole src BM bounds */
	FskGLPortTexRectGet((**src).glPort, texRect);				/* Get the texRect of the src BM */
	if (!FskRectangleIsEqual(texRect, *srcRect)) {				/* If the texture isn't tight enough, copy to a tight one */
		if (NULL == *cache) {									/* If there is no cache, ... */
			BAIL_IF_NULL(tmpCache, err, kFskErrInvalidParameter);
			BAIL_IF_ERR(err = FskGLEffectCacheNew(tmpCache));	/* ... allocate one */
			*cache = *tmpCache;
		}
		BAIL_IF_ERR(err = GLEffectCacheGetSrcCompatibleBitmap(*cache, *src, (**srcRect).width, (**srcRect).height, tmpBM));	/* Allocate a tight texture */
		BAIL_IF_ERR(err = FskGLEffectCopyApply(NULL, *src, *srcRect, *tmpBM, NULL, *cache));								/* Copy  to a tight texture */
		*src = *tmpBM;											/* Replace the src with the tmp BM */
		*srcRect = &(**tmpBM).bounds;							/* Update the src bounds with the tmp bounds */
		FskGLPortTexRectGet((**src).glPort, texRect);			/* Update the texRect with the tmp texRect */
	}

bail:
	return err;
}


/********************************************************************************
 * SetEffectMatrix
 ********************************************************************************/

static void SetEffectMatrix(GLint id, float *state) {
	const float *M = FskGLGetViewMatrix();
	if (M[0] != state[0] || M[4] != state[1]) {	/* Only the scale coefficients change */
		glUniformMatrix3fv(id, 1, GL_FALSE, M);
		state[0] = M[0];
		state[1] = M[4];
	}
}


#if 0
#pragma mark -
#endif /* 0 */
/********************************************************************************
 ********************************************************************************
 ********************************************************************************
 *****							Simple Effects								*****
 ********************************************************************************
 ********************************************************************************
 ********************************************************************************/


/********************************************************************************
 * FskGLEffectCopyApply
 ********************************************************************************/

static FskErr FskGLEffectCopyApply(FskConstEffectCopy params, FskConstBitmap src, FskConstRectangle srcRect, FskBitmap dst, FskConstPoint dstPoint, FskEffectCache cache) {
	FskErr	err		= kFskErrNone;

	#if defined(LOG_PARAMETERS)
		LogEffectsParameters(__FUNCTION__, params, src, srcRect, dst, dstPoint, cache);
	#endif /* LOG_PARAMETERS */

	BAIL_IF_ERR(err = FskGLUseProgram(gEffectsGlobals.gidCopyProgram));
	BAIL_IF_ERR(err = SetEffectSrcDst(src, srcRect, GL_CLAMP_TO_EDGE, GL_NEAREST, dst, dstPoint));	/* Compute rects, setup dst, bind src to texture#0 */
	SetEffectMatrix(gEffectsGlobals.gidCopyMatrix, gEffectsGlobals.CopyMtx);						/* Set matrix, only if changed */

	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	#if CHECK_GL_ERROR
		err = GetFskGLError();
	#endif /* CHECK_GL_ERROR */

bail:
	return err;
}


/********************************************************************************
 * EffectStretchApply -- internally used for Mask and Shade only.
 ********************************************************************************/

static FskErr EffectStretchApply(FskConstEffectCopy params, FskConstBitmap src, FskConstRectangle srcRect, FskBitmap dst, FskConstRectangle dstRect, FskEffectCache cache) {
	FskErr				err		= kFskErrNone;
	FskRectangleRecord	texRect;

	BAIL_IF_ERR(err = FskGLUseProgram(gEffectsGlobals.gidCopyProgram));
	BAIL_IF_ERR(err = FskGLBitmapTextureTargetSet(dst));											/* Set dst as FBO */
	BAIL_IF_ERR(err = FskGLBindBMTexture(src, GL_CLAMP_TO_EDGE, GL_LINEAR));						/* Set src texture */
	BAIL_IF_ERR(err = FskGLPortTexRectGet(src->glPort, &texRect));									/* Get src texture bounds */
	if (!srcRect)	srcRect = &src->bounds;
	if (!dstRect)	dstRect = &dst->bounds;
	BAIL_IF_ERR(err = SetEffectVertices(srcRect, &texRect, dstRect));								/* Set the vertices */
	SetEffectMatrix(gEffectsGlobals.gidCopyMatrix, gEffectsGlobals.CopyMtx);						/* Set matrix, only if changed */

	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	#if CHECK_GL_ERROR
		err = GetFskGLError();
	#endif /* CHECK_GL_ERROR */

bail:
	return err;
}


/********************************************************************************
 * EffectStretchGelApply -- internally used for Shade only.
 ********************************************************************************/

static FskErr EffectStretchGelApply(FskConstEffectGel params, FskConstBitmap src, FskConstRectangle srcRect, FskBitmap dst, FskConstRectangle dstRect, FskEffectCache cache) {
	FskErr				err		= kFskErrNone;
	FskRectangleRecord	texRect;

	BAIL_IF_ERR(err = FskGLUseProgram(gEffectsGlobals.gidGelProgram));
	BAIL_IF_ERR(err = FskGLBitmapTextureTargetSet(dst));											/* Set dst as FBO */
	BAIL_IF_ERR(err = FskGLBindBMTexture(src, GL_CLAMP_TO_EDGE, GL_LINEAR));						/* Set src texture */
	BAIL_IF_ERR(err = FskGLPortTexRectGet(src->glPort, &texRect));									/* Get src texture bounds */
	if (!srcRect)	srcRect = &src->bounds;
	if (!dstRect)	dstRect = &dst->bounds;
	BAIL_IF_ERR(err = SetEffectVertices(srcRect, &texRect, dstRect));								/* Set the vertices */
	SetEffectMatrix(gEffectsGlobals.gidGelMatrix, gEffectsGlobals.GelMtx);							/* Set matrix, only if changed */
	SetUniformColor(gEffectsGlobals.gidGelColor, &params->color);									/* Set color */

	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	#if CHECK_GL_ERROR
		err = GetFskGLError();
	#endif /* CHECK_GL_ERROR */

bail:
	return err;
}


/********************************************************************************
 * FskGLEffectCopyMirrorBordersApply
 ********************************************************************************/

static FskErr FskGLEffectCopyMirrorBordersApply(FskConstEffectCopyMirrorBorders params, FskConstBitmap src, FskConstRectangle srcRect, FskBitmap dst, FskConstPoint dstPoint, FskEffectCache cache) {
	FskErr	err		= kFskErrNone;

	#if defined(LOG_PARAMETERS)
		LogEffectsParameters(__FUNCTION__, params, src, srcRect, dst, dstPoint, cache);
	#endif /* LOG_PARAMETERS */

	BAIL_IF_ERR(err = FskGLUseProgram(gEffectsGlobals.gidCopyProgram));
	BAIL_IF_ERR(err = SetEffectMirrorRepeatSrcDst(params->border, src, srcRect, dst, dstPoint));	/* Compute rects, setup dst, bind src to texture#0 */
	SetEffectMatrix(gEffectsGlobals.gidCopyMatrix, gEffectsGlobals.CopyMtx);						/* Set matrix, only if changed */

	glDrawArrays(GL_TRIANGLE_STRIP, 0, NUM_REFLECTED_VERTICES);
	#if CHECK_GL_ERROR
		err = GetFskGLError();
	#endif /* CHECK_GL_ERROR */

bail:
	return err;
}


/********************************************************************************
 * FskGLEffectColorizeAlphaApply
 ********************************************************************************/

static FskErr FskGLEffectColorizeAlphaApply(FskConstEffectColorizeAlpha params, FskConstBitmap src, FskConstRectangle srcRect, FskBitmap dst, FskConstPoint dstPoint, FskEffectCache cache) {
	FskErr	err	= kFskErrNone;

	#if defined(LOG_PARAMETERS)
		LogEffectsParameters(__FUNCTION__, params, src, srcRect, dst, dstPoint, cache);
		LogColor(&params->color0, "color0");
		LogColor(&params->color1, "color1");
	#endif /* LOG_PARAMETERS */

	BAIL_IF_ERR(err = FskGLUseProgram(gEffectsGlobals.gidColorizeAlphaProgram));
	BAIL_IF_ERR(err = SetEffectSrcDst(src, srcRect, GL_CLAMP_TO_EDGE, GL_NEAREST, dst, dstPoint));	/* Compute rects, setup dst, bind src to texture#0 */
	SetEffectMatrix(gEffectsGlobals.gidColorizeAlphaMatrix, gEffectsGlobals.ColorizeAlphaMtx);		/* Set matrix, only if changed */
	SetUniformColor(gEffectsGlobals.gidColorizeAlphaColor0, &params->color0);						/* Set color0 */
	SetUniformColor(gEffectsGlobals.gidColorizeAlphaColor1, &params->color1);						/* Set color1 */

	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	#if CHECK_GL_ERROR
		err = GetFskGLError();
	#endif /* CHECK_GL_ERROR */

bail:
	return err;
}


/********************************************************************************
 * FskGLEffectColorizeInnerApply
 ********************************************************************************/

static FskErr FskGLEffectColorizeInnerApply(FskConstEffectColorizeInner params, FskConstBitmap src, FskConstRectangle srcRect, FskBitmap dst, FskConstPoint dstPoint, FskEffectCache cache) {
	FskErr	err	= kFskErrNone;

	#if defined(LOG_PARAMETERS)
		LogEffectsParameters(__FUNCTION__, params, src, srcRect, dst, dstPoint, cache);
		LogSrcBitmap(params->matte, "mat");
		LogColor(&params->color, "color");
	#endif /* LOG_PARAMETERS */

	if (!src->alphaIsPremultiplied) {
		BAIL_IF_ERR(err = FskGLUseProgram(gEffectsGlobals.gidColorizeInnerStraightProgram));
		BAIL_IF_ERR(err = SetMatteEffectSrcDst(src, srcRect, params->matte, NULL, dst, dstPoint));					/* Set up src, matte, dst and vertices */
		SetEffectMatrix(gEffectsGlobals.gidColorizeInnerStraightMatrix, gEffectsGlobals.ColorizeInnerStraightMtx);	/* Set matrix, only if changed */
		SetUniformColor(gEffectsGlobals.gidColorizeInnerStraightColor, &params->color);								/* color */
	}
	else {
		BAIL_IF_ERR(err = FskGLUseProgram(gEffectsGlobals.gidColorizeInnerPremulProgram));
		BAIL_IF_ERR(err = SetMatteEffectSrcDst(src, srcRect, params->matte, NULL, dst, dstPoint));					/* Set up src, matte, dst and vertices */
		SetEffectMatrix(gEffectsGlobals.gidColorizeInnerPremulMatrix, gEffectsGlobals.ColorizeInnerPremulMtx);		/* Set matrix, only if changed */
		SetUniformColor(gEffectsGlobals.gidColorizeInnerPremulColor, &params->color);								/* color */
	}

	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	ResetMatteEffectVertices();
	#if CHECK_GL_ERROR
		err = GetFskGLError();
	#endif /* CHECK_GL_ERROR */

bail:
	return err;
}


/********************************************************************************
 * FskGLEffectColorizeOuterApply
 ********************************************************************************/

static FskErr FskGLEffectColorizeOuterApply(FskConstEffectColorizeOuter params, FskConstBitmap src, FskConstRectangle srcRect, FskBitmap dst, FskConstPoint dstPoint, FskEffectCache cache) {
	FskErr	err	= kFskErrNone;

	#if defined(LOG_PARAMETERS)
		LogEffectsParameters(__FUNCTION__, params, src, srcRect, dst, dstPoint, cache);
		LogSrcBitmap(params->matte, "mat");
		LogColor(&params->color, "color");
	#endif /* LOG_PARAMETERS */

	if (!src->alphaIsPremultiplied) {
		BAIL_IF_ERR(err = FskGLUseProgram(gEffectsGlobals.gidColorizeOuterStraightProgram));
		BAIL_IF_ERR(err = SetMatteEffectSrcDst(src, srcRect, params->matte, NULL, dst, dstPoint));					/* Set up src, matte, dst and vertices */
		SetEffectMatrix(gEffectsGlobals.gidColorizeOuterStraightMatrix, gEffectsGlobals.ColorizeOuterStraightMtx);	/* Set matrix, only if changed */
		SetUniformColor(gEffectsGlobals.gidColorizeOuterStraightColor, &params->color);								/* color */
	}
	else {
		BAIL_IF_ERR(err = FskGLUseProgram(gEffectsGlobals.gidColorizeOuterPremulProgram));
		BAIL_IF_ERR(err = SetMatteEffectSrcDst(src, srcRect, params->matte, NULL, dst, dstPoint));					/* Set up src, matte, dst and vertices */
		SetEffectMatrix(gEffectsGlobals.gidColorizeOuterPremulMatrix, gEffectsGlobals.ColorizeOuterPremulMtx);		/* Set matrix, only if changed */
		SetPremultipliedUniformColor(gEffectsGlobals.gidColorizeOuterPremulColor, &params->color);					/* color */
	}

	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	ResetMatteEffectVertices();
	#if CHECK_GL_ERROR
		err = GetFskGLError();
	#endif /* CHECK_GL_ERROR */

	#ifdef LOG_IMAGES
	{	static int index = 0;

		WriteGLBMPToVPath(dst, "ColorizeOuter%04dDst.bmp", index);
		if (FskBitmapIsOpenGLDestinationAccelerated(src))			WriteGLBMPToVPath(src, "ColorizeOuter%04dSrcGL.bmp", index);
		else														WriteBMPToVPath(  src, "ColorizeOuter%04dSrcBM.bmp", index);
		if (FskBitmapIsOpenGLDestinationAccelerated(params->matte))	WriteGLBMPToVPath(params->matte, "ColorizeOuter%04dMatGL.bmp", index);
		else														WriteBMPToVPath(  params->matte, "ColorizeOuter%04dMatBM.bmp", index);
		++index;
	}
	#endif /* LOG_IMAGES */

bail:
	return err;
}


/********************************************************************************
 * FskGLEffectColorizeApply
 ********************************************************************************/

static FskErr FskGLEffectColorizeApply(FskConstEffectColorize params, FskConstBitmap src, FskConstRectangle srcRect, FskBitmap dst, FskConstPoint dstPoint, FskEffectCache cache) {
	FskErr		err			= kFskErrNone;
	#if defined(LOG_PARAMETERS)
		LogEffectsParameters(__FUNCTION__, params, src, srcRect, dst, dstPoint, cache);
		LogColor(&params->color, "color");
	#endif /* LOG_PARAMETERS */

	if (src->alphaIsPremultiplied) {
		BAIL_IF_ERR(err = FskGLUseProgram(gEffectsGlobals.gidColorizePremulProgram));
		BAIL_IF_ERR(err = SetEffectSrcDst(src, srcRect, GL_CLAMP_TO_EDGE, GL_NEAREST, dst, dstPoint));		/* Compute rects, setup dst, bind src to texture#0 */
		SetEffectMatrix(gEffectsGlobals.gidColorizePremulMatrix, gEffectsGlobals.ColorizePremulMtx);		/* Set matrix, only if changed */
		SetUniformColor(gEffectsGlobals.gidColorizePremulColor, &params->color);							/* color */
	} else {
		BAIL_IF_ERR(err = FskGLUseProgram(gEffectsGlobals.gidColorizeStraightProgram));
		BAIL_IF_ERR(err = SetEffectSrcDst(src, srcRect, GL_CLAMP_TO_EDGE, GL_NEAREST, dst, dstPoint));		/* Compute rects, setup dst, bind src to texture#0 */
		SetEffectMatrix(gEffectsGlobals.gidColorizeStraightMatrix, gEffectsGlobals.ColorizeStraightMtx);	/* Set matrix, only if changed */
		SetUniformColor(gEffectsGlobals.gidColorizeStraightColor, &params->color);							/* color */
	}

	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

	#if CHECK_GL_ERROR
		err = GetFskGLError();
	#endif /* CHECK_GL_ERROR */

bail:
	return err;
}


/********************************************************************************
 * FskGLEffectMonochromeApply
 ********************************************************************************/

static FskErr FskGLEffectMonochromeApply(FskConstEffectMonochrome params, FskConstBitmap src, FskConstRectangle srcRect, FskBitmap dst, FskConstPoint dstPoint, FskEffectCache cache) {
	FskErr				err			= kFskErrNone;

	#if defined(LOG_PARAMETERS)
		LogEffectsParameters(__FUNCTION__, params, src, srcRect, dst, dstPoint, cache);
		LogColor(&params->color0, "color0");
		LogColor(&params->color1, "color1");
	#endif /* LOG_PARAMETERS */

	BAIL_IF_ERR(err = SetEffectSrcDst(src, srcRect, GL_CLAMP_TO_EDGE, GL_NEAREST, dst, dstPoint));				/* Compute rects, setup dst, bind src to texture#0 */

	if (!src->alphaIsPremultiplied) {
		BAIL_IF_ERR(err = FskGLUseProgram(gEffectsGlobals.gidMonochromeStraightProgram));
		SetEffectMatrix(gEffectsGlobals.gidMonochromeStraightMatrix, gEffectsGlobals.MonochromeStraightMtx);	/* Set matrix, only if changed */
		SetUniformColor(gEffectsGlobals.gidMonochromeStraightColor0, &params->color0);							/* color0 */
		SetUniformColor(gEffectsGlobals.gidMonochromeStraightColor1, &params->color1);							/* color1 */
	}
	else {
		BAIL_IF_ERR(err = FskGLUseProgram(gEffectsGlobals.gidMonochromePremulProgram));
		SetEffectMatrix(gEffectsGlobals.gidMonochromePremulMatrix, gEffectsGlobals.MonochromePremulMtx);		/* Set matrix, only if changed */
		SetPremultipliedUniformColor(gEffectsGlobals.gidMonochromePremulColor0, &params->color0);				/* color0 */
		SetPremultipliedUniformColor(gEffectsGlobals.gidMonochromePremulColor1, &params->color1);				/* color1 */
	}


	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	#if CHECK_GL_ERROR
		err = GetFskGLError();
	#endif /* CHECK_GL_ERROR */

bail:
	return err;
}


/********************************************************************************
 * FskGLEffectMaskApply
 ********************************************************************************/

static FskErr FskGLEffectMaskApply(FskConstEffectMask params, FskConstBitmap src, FskConstRectangle srcRect, FskBitmap dst, FskConstPoint dstPoint, FskEffectCache cache) {
	FskErr				err			= kFskErrNone;
	FskEffectCache		myCache		= NULL;
	FskBitmap			tmp			= NULL;
	FskRectangleRecord	dstRect;

	#if defined(LOG_PARAMETERS)
		LogEffectsParameters(__FUNCTION__, params, src, srcRect, dst, dstPoint, cache);
		LOGD("\tparams: mask=%p", params->mask);
		LogSrcBitmap(params->mask, "mask");
		LogRect(&params->maskRect, "maskRect");
	#endif /* LOG_PARAMETERS */

	/* src is already accelerated; check mask */
	//FskBitmapSetOpenGLSourceAccelerated(params->mask, true);
	BAIL_IF_ERR(err = FskBitmapCheckGLSourceAccelerated((FskBitmap)params->mask));

	/* We may need a cache */
	if (!cache) {
		BAIL_IF_ERR(err = FskGLEffectCacheNew(&myCache));
		cache = myCache;
	}
	if (!srcRect)	srcRect  = &src->bounds;

	/* Copy src to dst */
	if (src != dst) {
		BAIL_IF_ERR(err = FskGLEffectCopyApply(NULL, src, srcRect, dst, dstPoint, cache));					/* src --> dst */
	} else /*if (srcRect || dstPoint)*/ {																	/* src and dst are the same, and a region is being specified */
		BAIL_IF_ERR(err = GLEffectCacheGetSrcCompatibleBitmap(cache, src, srcRect->width, srcRect->height, &tmp));
		BAIL_IF_ERR(err = FskGLEffectCopyApply(NULL, src,  srcRect, tmp, NULL,    cache));					/* src  --> tmp */
		BAIL_IF_ERR(err = FskGLEffectCopyApply(NULL, tmp, NULL,    dst,  dstPoint, cache));					/* tmp --> dst  */
	}
	/* else: src and dst are identical */

	dst->hasAlpha = 1;
	if (src->alphaIsPremultiplied)	FskGLSetBlend(true, GL_ZERO, GL_SRC_ALPHA, GL_ZERO, GL_SRC_ALPHA);		/* { dstRGB * srcAlpha, dstAlpha * srcAlpha } */
	else							FskGLSetBlend(true, GL_ZERO, GL_ONE,       GL_ONE, GL_ZERO);			/* { dstRGB,            srcAlpha            } */
	if (srcRect)	{ dstRect.width = srcRect->width;		dstRect.height = srcRect->height;		}
	else			{ dstRect.width = src->bounds.width;	dstRect.height = src->bounds.height;	}
	if (dstPoint)	{ dstRect.x     = dstPoint->x;			dstRect.y      = dstPoint->y;			}
	else			{ dstRect.x     = dst->bounds.x;		dstRect.y      = dst->bounds.y;			}
	BAIL_IF_ERR(err = EffectStretchApply(NULL, params->mask, &params->maskRect, dst, &dstRect, cache));		/* Stretch mask and replace alpha in the dst */

bail:
	FskGLEffectCacheReleaseBitmap(cache, tmp);
	FskGLEffectCacheDispose(myCache);
	return err;
}


/********************************************************************************
 * FskGLEffectShadeApply
 ********************************************************************************/

static FskErr FskGLEffectShadeApply(FskConstEffectShade params, FskConstBitmap src, FskConstRectangle srcRect, FskBitmap dst, FskConstPoint dstPoint, FskEffectCache cache) {
	FskErr				err			= kFskErrNone;
	FskEffectCache		myCache		= NULL;
	FskBitmap			tmp			= NULL;
	FskRectangleRecord	dstRect;
	FskEffectGelRecord	gel;

	#if defined(LOG_PARAMETERS)
		LogEffectsParameters(__FUNCTION__, params, src, srcRect, dst, dstPoint, cache);
		LOGD("\tparams: shadow=%p", params->shadow);
		LogSrcBitmap(params->shadow, "shadow");
		LogRect(&params->shadowRect, "shadowRect");
	#endif /* LOG_PARAMETERS */

	/* src is already accelerated; check mask */
	//FskBitmapSetOpenGLSourceAccelerated(params->shadow, true);
	BAIL_IF_ERR(err = FskBitmapCheckGLSourceAccelerated((FskBitmap)params->shadow));

	/* We may need a cache */
	if (!cache) {
		BAIL_IF_ERR(err = FskGLEffectCacheNew(&myCache));
		cache = myCache;
	}
	if (!srcRect)	srcRect  = &src->bounds;

	/* Copy src to dst */
	if (src != dst) {
		BAIL_IF_ERR(err = FskGLEffectCopyApply(NULL, src, srcRect, dst, dstPoint, cache));							/* src --> dst */
	} else /*if (srcRect || dstPoint)*/ {																			/* src and dst are the same, and a region is being specified */
		BAIL_IF_ERR(err = GLEffectCacheGetSrcCompatibleBitmap(cache, src, srcRect->width, srcRect->height, &tmp));
		BAIL_IF_ERR(err = FskGLEffectCopyApply(NULL, src,  srcRect, tmp, NULL,    cache));							/* src  --> tmp */
		BAIL_IF_ERR(err = FskGLEffectCopyApply(NULL, tmp, NULL,    dst,  dstPoint, cache));							/* tmp --> dst  */
	}
	/* else: src and dst are identical */

	/* TODO: Accommodate opacity */
	if (src->alphaIsPremultiplied) {
		FskColorRGBASet(&gel.color, params->opacity, params->opacity, params->opacity, params->opacity);
		FskGLSetBlend(true, GL_DST_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ZERO, GL_ONE);
	}
	else {
		FskColorRGBASet(&gel.color, 255, 255, 255, params->opacity);
		FskGLSetBlend(true, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ZERO, GL_ONE);
	}
	if (srcRect)	{ dstRect.width = srcRect->width;		dstRect.height = srcRect->height;		}
	else			{ dstRect.width = src->bounds.width;	dstRect.height = src->bounds.height;	}
	if (dstPoint)	{ dstRect.x     = dstPoint->x;			dstRect.y      = dstPoint->y;			}
	else			{ dstRect.x     = dst->bounds.x;		dstRect.y      = dst->bounds.y;			}
	BAIL_IF_ERR(err = EffectStretchGelApply(&gel, params->shadow, &params->shadowRect, dst, &dstRect, cache));		/* Stretch mask and composite alpha in the dst */

bail:
	FskGLEffectCacheReleaseBitmap(cache, tmp);
	FskGLEffectCacheDispose(myCache);
	return err;
}





/********************************************************************************
 ********************************************************************************
 ********************************************************************************
 *****							Elementary Effects							*****
 ********************************************************************************
 ********************************************************************************
 ********************************************************************************/


/********************************************************************************
 * FskGLEffectDirectionalGaussianBlurApply
 * We set the srcDelta every time, because it typically alternates between horizontal and vertical.
 ********************************************************************************/

static FskErr FskGLEffectDirectionalGaussianBlurApply(FskConstEffectDirectionalGaussianBlur params, FskConstBitmap src, FskConstRectangle srcRect, FskBitmap dst, FskConstPoint dstPoint, FskEffectCache cache) {
	FskErr				err			= kFskErrNone;
	GLint				wrapMode	= gEffectsGlobals.canWrapNPOT ? GL_MIRRORED_REPEAT : GL_CLAMP_TO_EDGE;
	FskRectangleRecord	texRect;
	float				coeff[3];
	int					radius;

	#if defined(LOG_PARAMETERS)
		LogEffectsParameters(__FUNCTION__, params, src, srcRect, dst, dstPoint, NULL);
		LOGD("\tparams: sigma=%.3g, direction=(%+3.1f, %+3.1f), wrapMode=%s", params->sigma, params->direction[0], params->direction[1], GLWrapModeNameFromCode(wrapMode));
	#endif /* LOG_PARAMETERS */

	if (cache) {}	/* unused */
	BAIL_IF_ERR(err = FskGLUseProgram(gEffectsGlobals.gidGaussianBlurProgram));
	BAIL_IF_ERR(err = SetEffectSrcDst(src, srcRect, wrapMode, GL_NEAREST, dst, dstPoint));			/* Compute rects, setup dst, bind src to texture#0 */
	FskGLPortTexRectGet(src->glPort, &texRect);

	SetEffectMatrix(gEffectsGlobals.gidGaussianBlurMatrix, gEffectsGlobals.GaussianBlurMtx);											/* Set matrix, only if changed */
	if ((gEffectsGlobals.GaussianSigma != params->sigma) || (0.f == params->sigma)) {													/* If sigma has changed, ... */
		gEffectsGlobals.GaussianSigma = params->sigma;																					/* cache updated sigma */
		GaussianCoeffRadFromSigma(params->sigma, coeff, &radius);																		/* compute coefficients and radius from sigma */
		glUniform3fv(gEffectsGlobals.gidGaussianBlurCoeff, 1, coeff);																	/* set coeff */
		glUniform1i(gEffectsGlobals.gidGaussianBlurRadius, radius);																		/* set radius */
	}
	glUniform2f(gEffectsGlobals.gidGaussianBlurSrcDelta, params->direction[0] / texRect.width, params->direction[1] / texRect.height);	/* srcDelta is always set */
	#if CHECK_GL_ERROR
		BAIL_IF_ERR(err = GetFskGLError());
	#endif /* CHECK_GL_ERROR */

	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	#if CHECK_GL_ERROR
		err = GetFskGLError();
	#endif /* CHECK_GL_ERROR */

bail:
	return err;
}


/********************************************************************************
 * FskGLEffectDirectionalBoxBlurApply
 * We set the srcDelta every time, because it typically alternates between horizontal and vertical.
 ********************************************************************************/

static FskErr FskGLEffectDirectionalBoxBlurApply(FskConstEffectDirectionalBoxBlur params, FskConstBitmap src, FskConstRectangle srcRect, FskBitmap dst, FskConstPoint dstPoint, FskEffectCache cache) {
	FskErr				err			= kFskErrNone;
	GLint				wrapMode	= gEffectsGlobals.canWrapNPOT ? GL_MIRRORED_REPEAT : GL_CLAMP_TO_EDGE;
	FskRectangleRecord	texRect;

	#if defined(LOG_PARAMETERS)
		LogEffectsParameters(__FUNCTION__, params, src, srcRect, dst, dstPoint, NULL);
		LOGD("\tparams: radius=%d, direction=(%+3.1f, %+3.1f), wrapMode=%s", params->radius, params->direction[0], params->direction[1], GLWrapModeNameFromCode(wrapMode));
	#endif /* LOG_PARAMETERS */

	if (cache) {}	/* unused */
	BAIL_IF_ERR(err = FskGLUseProgram(gEffectsGlobals.gidBoxBlurProgram));
	BAIL_IF_ERR(err = SetEffectSrcDst(src, srcRect, wrapMode, GL_NEAREST, dst, dstPoint));			/* Compute rects, setup dst, bind src to texture#0 */
	FskGLPortTexRectGet(src->glPort, &texRect);

	SetEffectMatrix(gEffectsGlobals.gidBoxBlurMatrix, gEffectsGlobals.BoxBlurMtx);					/* Set matrix, only if changed */
	if ((gEffectsGlobals.BoxBlurRadius != params->radius) || (0 == params->radius)) {				/* If the blur radius has changed, ... */
		gEffectsGlobals.BoxBlurRadius = params->radius;												/* cache radius */
		glUniform1i(gEffectsGlobals.gidBoxBlurRadius, params->radius);								/* set radius */
	}
	glUniform3f(gEffectsGlobals.gidBoxBlurSrcDelta, params->direction[0] / texRect.width,			/* srcDelta.x */
													params->direction[1] / texRect.height,			/* srcDelta.y */
													1.f / (2 * params->radius + 1));				/* norm */
	#if CHECK_GL_ERROR
		BAIL_IF_ERR(err = GetFskGLError());
	#endif /* CHECK_GL_ERROR */

	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	#if CHECK_GL_ERROR
		err = GetFskGLError();
	#endif /* CHECK_GL_ERROR */

bail:
	return err;
}


/********************************************************************************
 * FskGLEffectDirectionalDilateApply
 * We set the srcDelta every time, because it typically alternates between horizontal and vertical.
 ********************************************************************************/

static FskErr FskGLEffectDirectionalDilateApply(FskConstEffectDirectionalDilate params, FskConstBitmap src, FskConstRectangle srcRect, FskBitmap dst, FskConstPoint dstPoint, FskEffectCache cache) {
	FskErr				err			= kFskErrNone;
	FskRectangleRecord	texRect;

	#if defined(LOG_PARAMETERS)
		LogEffectsParameters(__FUNCTION__, params, src, srcRect, dst, dstPoint, cache);
		LOGD("\tparams: radius=%d, direction=(%+3.1f, %+3.1f)", params->radius, params->direction[0], params->direction[1]);
	#endif /* LOG_PARAMETERS */

	if (cache) {}	/* unused */
	BAIL_IF_ERR(err = FskGLUseProgram(gEffectsGlobals.gidDilateProgram));
	BAIL_IF_ERR(err = SetEffectSrcDst(src, srcRect, GL_CLAMP_TO_EDGE, GL_NEAREST, dst, dstPoint));									/* Compute rects, setup dst, bind src to texture#0 */
	FskGLPortTexRectGet(src->glPort, &texRect);

	SetEffectMatrix(gEffectsGlobals.gidDilateMatrix, gEffectsGlobals.DilateMtx);													/* set matrix, if changed */
	if ((gEffectsGlobals.DilateRadius != params->radius) || (0 == params->radius)) {												/* If the dilate radius has changed, ... */
		gEffectsGlobals.DilateRadius = params->radius;																				/* cache radius */
		glUniform1i(gEffectsGlobals.gidDilateRadius, params->radius);																/* set radius */
	}
	glUniform2f(gEffectsGlobals.gidDilateSrcDelta, params->direction[0] / texRect.width, params->direction[1] / texRect.height);	/* srcDelta is set every time */
	#if CHECK_GL_ERROR
		BAIL_IF_ERR(err = GetFskGLError());
	#endif /* CHECK_GL_ERROR */

	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	#if CHECK_GL_ERROR
		err = GetFskGLError();
	#endif /* CHECK_GL_ERROR */

bail:
	return err;
}


/********************************************************************************
 * FskGLEffectDirectionalErodeApply
 * We set the srcDelta every time, because it typically alternates between horizontal and vertical.
 ********************************************************************************/

static FskErr FskGLEffectDirectionalErodeApply(FskConstEffectDirectionalErode params, FskConstBitmap src, FskConstRectangle srcRect, FskBitmap dst, FskConstPoint dstPoint, FskEffectCache cache) {
	FskErr				err			= kFskErrNone;
	FskRectangleRecord	texRect;

	#if defined(LOG_PARAMETERS)
		LogEffectsParameters(__FUNCTION__, params, src, srcRect, dst, dstPoint, cache);
		LOGD("\tparams: radius=%d, direction=(%+3.1f, %+3.1f)", params->radius, params->direction[0], params->direction[1]);
	#endif /* LOG_PARAMETERS */

	if (cache) {}	/* unused */
	BAIL_IF_ERR(err = FskGLUseProgram(gEffectsGlobals.gidErodeProgram));
	BAIL_IF_ERR(err = SetEffectSrcDst(src, srcRect, GL_CLAMP_TO_EDGE, GL_NEAREST, dst, dstPoint));								/* Compute rects, setup dst, bind src to texture#0 */
	FskGLPortTexRectGet(src->glPort, &texRect);

	SetEffectMatrix(gEffectsGlobals.gidErodeMatrix, gEffectsGlobals.ErodeMtx);													/* Set matrix, only if changed */
	if ((gEffectsGlobals.ErodeRadius != params->radius) || (0 == params->radius)) {												/* If the erode radius has changed, ... */
		gEffectsGlobals.ErodeRadius = params->radius;																			/* cache radius */
		glUniform1i(gEffectsGlobals.gidErodeRadius, params->radius);															/* set radius */
	}
	glUniform2f(gEffectsGlobals.gidErodeSrcDelta, params->direction[0] / texRect.width, params->direction[1] / texRect.height);	/* srcDelta */
	#if CHECK_GL_ERROR
		BAIL_IF_ERR(err = GetFskGLError());
	#endif /* CHECK_GL_ERROR */

	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	#if CHECK_GL_ERROR
		err = GetFskGLError();
	#endif /* CHECK_GL_ERROR */

bail:
	return err;
}


/********************************************************************************
 * FskGLEffectPremultiplyAlphaApply
 ********************************************************************************/

static FskErr FskGLEffectPremultiplyAlphaApply(FskConstEffectPremultiplyAlpha params, FskConstBitmap src, FskConstRectangle srcRect, FskBitmap dst, FskConstPoint dstPoint, FskEffectCache cache) {
	FskErr				err			= kFskErrNone;
	FskRectangleRecord	texRect;

	#if defined(LOG_PARAMETERS)
		LogEffectsParameters(__FUNCTION__, params, src, srcRect, dst, dstPoint, cache);
	#endif /* LOG_PARAMETERS */

	if (cache) {}	/* unused */
	BAIL_IF_ERR(err = FskGLUseProgram(gEffectsGlobals.gidStraightToPremultipliedProgram));
	BAIL_IF_ERR(err = SetEffectSrcDst(src, srcRect, GL_CLAMP_TO_EDGE, GL_NEAREST, dst, dstPoint));					/* Compute rects, setup dst, bind src to texture#0 */
	FskGLPortTexRectGet(src->glPort, &texRect);

	SetEffectMatrix(gEffectsGlobals.gidStraightToPremultipliedMatrix, gEffectsGlobals.StraightToPremultipliedMtx);	/* Set matrix, only if changed */
	#if CHECK_GL_ERROR
		BAIL_IF_ERR(err = GetFskGLError());
	#endif /* CHECK_GL_ERROR */

	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	#if CHECK_GL_ERROR
		err = GetFskGLError();
	#endif /* CHECK_GL_ERROR */

	dst->hasAlpha = true;
	dst->alphaIsPremultiplied = true;

bail:
	return err;
}


/********************************************************************************
 * FskGLEffectStraightAlphaApply
 ********************************************************************************/

static FskErr FskGLEffectStraightAlphaApply(FskConstEffectStraightAlpha params, FskConstBitmap src, FskConstRectangle srcRect, FskBitmap dst, FskConstPoint dstPoint, FskEffectCache cache) {
	FskErr				err			= kFskErrNone;
	FskRectangleRecord	texRect;

	#if defined(LOG_PARAMETERS)
		LogEffectsParameters(__FUNCTION__, params, src, srcRect, dst, dstPoint, cache);
		LOGD("\tbgColor(%3u %3u %3u)", params->color.r, params->color.g, params->color.b);
	#endif /* LOG_PARAMETERS */

	if (cache) {}	/* unused */
	BAIL_IF_ERR(err = FskGLUseProgram(gEffectsGlobals.gidPremultipliedToStraightProgram));
	BAIL_IF_ERR(err = SetEffectSrcDst(src, srcRect, GL_CLAMP_TO_EDGE, GL_NEAREST, dst, dstPoint));					/* Compute rects, setup dst, bind src to texture#0 */
	FskGLPortTexRectGet(src->glPort, &texRect);

	SetEffectMatrix(gEffectsGlobals.gidPremultipliedToStraightMatrix, gEffectsGlobals.PremultipliedToStraightMtx);	/* Set matrix, only if changed */
	glUniform3f(gEffectsGlobals.gidPremultipliedToStraightBgColor, params->color.r, params->color.g, params->color.b);
	#if CHECK_GL_ERROR
		BAIL_IF_ERR(err = GetFskGLError());
	#endif /* CHECK_GL_ERROR */

	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	#if CHECK_GL_ERROR
		err = GetFskGLError();
	#endif /* CHECK_GL_ERROR */

	dst->hasAlpha = true;
	dst->alphaIsPremultiplied = false;
bail:
	return err;
}




#if 0
#pragma mark -
#endif /* 0 */
/********************************************************************************
 ********************************************************************************
 ********************************************************************************
 *****							Compound Effects							*****
 ********************************************************************************
 ********************************************************************************
 ********************************************************************************/


/********************************************************************************
 * FskGLEffectDilateApply
 ********************************************************************************/

static FskErr FskGLEffectDilateApply(FskConstEffectDilate params, FskConstBitmap src, FskConstRectangle srcRect, FskBitmap dst, FskConstPoint dstPoint, FskEffectCache cache) {
	FskErr				err			= kFskErrNone;
	FskEffectCache		myCache		= NULL;
	FskBitmap			mid1		= NULL;
	FskBitmap			mid2		= NULL;
	FskRectangleRecord	texRect;
	FskEffectParametersRecord	p;

	#if defined(LOG_PARAMETERS)
		LogEffectsParameters(__FUNCTION__, params, src, srcRect, dst, dstPoint, cache);
		LOGD("\tparams: radius=%d", params->radius);
	#endif /* LOG_PARAMETERS */

	/* We need a cache */
	if (!cache) {
		BAIL_IF_ERR(err = FskGLEffectCacheNew(&myCache));
		cache = myCache;
	}
	if (!srcRect)	srcRect  = &src->bounds;

	/* Allocate a mid buffer */
	BAIL_IF_ERR(err = AssureIsolatedPixels(&src, &srcRect, dst, &cache, &texRect, &mid2, NULL));					/* This might allocate mid2 */
	BAIL_IF_ERR(err = GLEffectCacheGetSrcCompatibleBitmap(cache, src, srcRect->width, srcRect->height, &mid1));

	/* H dilate */
	p.directionalDilate.radius       = params->radius;
	p.directionalDilate.direction[0] = 1.f;
	p.directionalDilate.direction[1] = 0.f;
	BAIL_IF_ERR(err = FskGLEffectDirectionalDilateApply(&p.directionalDilate, src, srcRect, mid1, NULL, cache));	/* H dilate src --> mid1 */

	/* V dilate */
	p.directionalDilate.direction[0] = 0.f;
	p.directionalDilate.direction[1] = 1.f;
	BAIL_IF_ERR(err = FskGLEffectDirectionalDilateApply(&p.directionalDilate, mid1, NULL, dst, NULL, cache));		/* V dilate mid1 --> dst */

bail:
	FskGLEffectCacheReleaseBitmap(cache, mid2);
	FskGLEffectCacheReleaseBitmap(cache, mid1);
	FskGLEffectCacheDispose(myCache);
	return err;
}


/********************************************************************************
 * FskGLEffectErodeApply
 ********************************************************************************/

static FskErr FskGLEffectErodeApply(FskConstEffectErode params, FskConstBitmap src, FskConstRectangle srcRect, FskBitmap dst, FskConstPoint dstPoint, FskEffectCache cache) {
	FskErr				err			= kFskErrNone;
	FskEffectCache		myCache		= NULL;
	FskBitmap			mid1		= NULL;
	FskBitmap			mid2		= NULL;
	FskRectangleRecord	texRect;
	FskEffectParametersRecord	p;

	#if defined(LOG_PARAMETERS)
		LogEffectsParameters(__FUNCTION__, params, src, srcRect, dst, dstPoint, cache);
		LOGD("\tparams: radius=%d", params->radius);
	#endif /* LOG_PARAMETERS */

	/* We need a cache */
	if (!cache) {
		BAIL_IF_ERR(err = FskGLEffectCacheNew(&myCache));
		cache = myCache;
	}
	if (!srcRect)	srcRect  = &src->bounds;

	/* Allocate a mid buffer */
	BAIL_IF_ERR(err = AssureIsolatedPixels(&src, &srcRect, dst, &cache, &texRect, &mid2, NULL));					/* This might allocate mid2 */
	BAIL_IF_ERR(err = GLEffectCacheGetSrcCompatibleBitmap(cache, src, srcRect->width, srcRect->height, &mid1));

	/* H erode */
	p.directionalErode.radius       = params->radius;
	p.directionalErode.direction[0] = 1.f;
	p.directionalErode.direction[1] = 0.f;
	BAIL_IF_ERR(err = FskGLEffectDirectionalErodeApply(&p.directionalErode, src, srcRect, mid1, NULL, cache));		/* H erode src --> mid1 */

	/* V erode */
	p.directionalErode.direction[0] = 0.f;
	p.directionalErode.direction[1] = 1.f;
	BAIL_IF_ERR(err = FskGLEffectDirectionalErodeApply(&p.directionalErode, mid1, NULL, dst, NULL, cache));			/* V erode mid1 --> dst */


bail:
	FskGLEffectCacheReleaseBitmap(cache, mid2);
	FskGLEffectCacheReleaseBitmap(cache, mid1);
	FskGLEffectCacheDispose(myCache);
	return err;
}


/********************************************************************************
 * FskGLEffectGaussianBlurApply
 ********************************************************************************/

static FskErr FskGLEffectGaussianBlurApply(FskConstEffectGaussianBlur params, FskConstBitmap src, FskConstRectangle srcRect, FskBitmap dst, FskConstPoint dstPoint, FskEffectCache cache) {
	FskErr				err			= kFskErrNone;
	FskEffectCache		myCache		= NULL;
	FskBitmap			mid1		= NULL,
						mid2		= NULL;
	FskRectangleRecord	texRect;
	FskEffectDirectionalGaussianBlurRecord	dirams;
	#if !ALWAYS_CLEAR_DST_TEXTURE
		Boolean			doClear;
	#endif /* !ALWAYS_CLEAR_DST_TEXTURE */

	#if defined(LOG_PARAMETERS)
		LogEffectsParameters(__FUNCTION__, params, src, srcRect, dst, dstPoint, cache);
		LOGD("\tparams: sigma=(%.3g, %.3g)", params->sigmaX, params->sigmaY);
	#endif /* LOG_PARAMETERS */

	/* We need a cache */
	if (!cache) {
		BAIL_IF_ERR(err = FskGLEffectCacheNew(&myCache));
		cache = myCache;
	}

	if (!srcRect)	srcRect  = &src->bounds;
	#if !ALWAYS_CLEAR_DST_TEXTURE
		doClear = (dstPoint && (dstPoint->x || dstPoint->y)) || (srcRect->width != dst->bounds.width || srcRect->height != dst->bounds.height);
	#endif /* !ALWAYS_CLEAR_DST_TEXTURE */

	if (gEffectsGlobals.canWrapNPOT) {
		BAIL_IF_ERR(err = AssureIsolatedPixels(&src, &srcRect, dst, &cache, &texRect, &mid1, NULL));				/* Copy src to mid1 if necessary to get the proper border */
		BAIL_IF_ERR(err = GLEffectCacheGetSrcCompatibleBitmap(cache, src, srcRect->width, srcRect->height, &mid2));	/* Allocate mid2 buffer */

		#if !ALWAYS_CLEAR_DST_TEXTURE
			if (doClear)
				FskGLRenderToBitmapTexture(mid2, &blank);															/* Clear mid2 texture, especially outside of offset region */
		#endif	/* !ALWAYS_CLEAR_DST_TEXTURE */
		dirams.sigma = params->sigmaX;	dirams.direction[0] = 1.f;	dirams.direction[1] = 0.f;						/* H blur src --> mid2 */
		err = FskGLEffectDirectionalGaussianBlurApply(&dirams, src, srcRect, mid2, NULL, cache);

		dirams.sigma = params->sigmaY;	dirams.direction[0] = 0.f;	dirams.direction[1] = 1.f;						/* V blur mid2 --> dst */
		err = FskGLEffectDirectionalGaussianBlurApply(&dirams, mid2, NULL, dst, dstPoint, cache);
	}
	else {
		FskRectangleRecord sRect;
		FskEffectCopyMirrorBordersRecord mir;

		mir.border = (UInt32)(((params->sigmaX > params->sigmaY) ? params->sigmaX : params->sigmaY) * kSigmaCutoff);	/* Allocate intermediate buffers */
		sRect.width  = srcRect->width  + 2 * mir.border;																/* ... with a border all around */
		sRect.height = srcRect->height + 2 * mir.border;
		BAIL_IF_ERR(err = GLEffectCacheGetSrcCompatibleBitmap(cache, src, sRect.width, sRect.height, &mid1));
		BAIL_IF_ERR(err = GLEffectCacheGetSrcCompatibleBitmap(cache, src, sRect.width, sRect.height, &mid2));

		BAIL_IF_ERR(err = FskGLEffectCopyMirrorBordersApply(&mir, src, srcRect, mid1, NULL, cache));					/* Surround with mirrored borders src --> mid1 */

		#if !ALWAYS_CLEAR_DST_TEXTURE
			if (doClear)
				FskGLRenderToBitmapTexture(mid2, &blank);																/* Clear mid2 texture, especially outside of offset region */
		#endif /* !ALWAYS_CLEAR_DST_TEXTURE */

		FskRectangleSet(&sRect, mir.border, 0, srcRect->width, srcRect->height + 2 * mir.border);						/* Horizontal blur mid1 --> mid2 */
		dirams.sigma = params->sigmaX;	dirams.direction[0] = 1.f;	dirams.direction[1] = 0.f;
		BAIL_IF_ERR(err = FskGLEffectDirectionalGaussianBlurApply(&dirams, mid1, &sRect, mid2, NULL, cache));

		FskRectangleSet(&sRect, 0, mir.border, srcRect->width, srcRect->height);										/* Vertical blur mid2 --> dst */
		dirams.sigma = params->sigmaY;	dirams.direction[0] = 0.f;	dirams.direction[1] = 1.f;
		BAIL_IF_ERR(err = FskGLEffectDirectionalGaussianBlurApply(&dirams, mid2, &sRect, dst, dstPoint, cache));
	}

bail:
	FskGLEffectCacheReleaseBitmap(cache, mid1);
	FskGLEffectCacheReleaseBitmap(cache, mid2);
	FskGLEffectCacheDispose(myCache);
	return err;
}


/********************************************************************************
 * FskGLEffectBoxBlurApply
 ********************************************************************************/

static FskErr FskGLEffectBoxBlurApply(FskConstEffectBoxBlur params, FskConstBitmap src, FskConstRectangle srcRect, FskBitmap dst, FskConstPoint dstPoint, FskEffectCache cache) {
	FskErr				err			= kFskErrNone;
	FskEffectCache		myCache		= NULL;
	FskBitmap			mid1		= NULL,
						mid2		= NULL;
	FskRectangleRecord	texRect;
	FskEffectDirectionalBoxBlurRecord	dirams;
	#if !ALWAYS_CLEAR_DST_TEXTURE
		Boolean			doClear;
	#endif /* !ALWAYS_CLEAR_DST_TEXTURE */

	#if defined(LOG_PARAMETERS)
		LogEffectsParameters(__FUNCTION__, params, src, srcRect, dst, dstPoint, cache);
		LOGD("\tparams: radius=(%.3g, %.3g)", params->radiusX, params->radiusY);
	#endif /* LOG_PARAMETERS */

	/* We need a cache */
	if (!cache) {
		BAIL_IF_ERR(err = FskGLEffectCacheNew(&myCache));
		cache = myCache;
	}

	if (!srcRect)	srcRect  = &src->bounds;
	#if !ALWAYS_CLEAR_DST_TEXTURE
		doClear = (dstPoint && (dstPoint->x || dstPoint->y)) || (srcRect->width != dst->bounds.width || srcRect->height != dst->bounds.height);
	#endif /* !ALWAYS_CLEAR_DST_TEXTURE */

	if (gEffectsGlobals.canWrapNPOT) {
		BAIL_IF_ERR(err = AssureIsolatedPixels(&src, &srcRect, dst, &cache, &texRect, &mid1, NULL));				/* Copy src to mid1 if necessary to get the proper border */
		BAIL_IF_ERR(err = GLEffectCacheGetSrcCompatibleBitmap(cache, src, srcRect->width, srcRect->height, &mid2));	/* Allocate mid2 buffer */

		#if !ALWAYS_CLEAR_DST_TEXTURE
			if (doClear)
				FskGLRenderToBitmapTexture(mid2, &blank);															/* Clear mid2 texture, especially outside of offset region */
		#endif	/* !ALWAYS_CLEAR_DST_TEXTURE */
		dirams.radius = params->radiusX;	dirams.direction[0] = 1.f;	dirams.direction[1] = 0.f;					/* H blur src --> mid2 */
		err = FskGLEffectDirectionalBoxBlurApply(&dirams, src, srcRect, mid2, NULL, cache);

		dirams.radius = params->radiusY;	dirams.direction[0] = 0.f;	dirams.direction[1] = 1.f;					/* V blur mid2 --> dst */
		err = FskGLEffectDirectionalBoxBlurApply(&dirams, mid2, NULL, dst, dstPoint, cache);
	}
	else {
		FskRectangleRecord sRect;
		FskEffectCopyMirrorBordersRecord mir;

		mir.border = (params->radiusX > params->radiusY) ? params->radiusX : params->radiusY;						/* Allocate intermediate buffers */
		sRect.width  = srcRect->width  + 2 * mir.border;															/* ... with a border all around */
		sRect.height = srcRect->height + 2 * mir.border;
		BAIL_IF_ERR(err = GLEffectCacheGetSrcCompatibleBitmap(cache, src, sRect.width, sRect.height, &mid1));
		BAIL_IF_ERR(err = GLEffectCacheGetSrcCompatibleBitmap(cache, src, sRect.width, sRect.height, &mid2));

		BAIL_IF_ERR(err = FskGLEffectCopyMirrorBordersApply(&mir, src, srcRect, mid1, NULL, cache));				/* Surround with mirrored borders src --> mid1 */

		#if !ALWAYS_CLEAR_DST_TEXTURE
			if (doClear)
				FskGLRenderToBitmapTexture(mid2, &blank);															/* Clear mid2 texture, especially outside of offset region */
		#endif	/* !ALWAYS_CLEAR_DST_TEXTURE */

		FskRectangleSet(&sRect, mir.border, 0, srcRect->width, srcRect->height + 2 * mir.border);					/* Horizontal blur mid1 --> mid2 */
		dirams.radius = params->radiusX;	dirams.direction[0] = 1.f;	dirams.direction[1] = 0.f;
		BAIL_IF_ERR(err = FskGLEffectDirectionalBoxBlurApply(&dirams, mid1, &sRect, mid2, NULL, cache));

		FskRectangleSet(&sRect, 0, mir.border, srcRect->width, srcRect->height);									/* Vertical blur mid2 --> dst */
		dirams.radius = params->radiusY;	dirams.direction[0] = 0.f;	dirams.direction[1] = 1.f;
		BAIL_IF_ERR(err = FskGLEffectDirectionalBoxBlurApply(&dirams, mid2, &sRect, dst, dstPoint, cache));
	}

bail:
	FskGLEffectCacheReleaseBitmap(cache, mid1);
	FskGLEffectCacheReleaseBitmap(cache, mid2);
	FskGLEffectCacheDispose(myCache);
	return err;
}


/********************************************************************************
 * FskGLEffectInnerGlowApply
 ********************************************************************************/

static FskErr FskGLEffectInnerGlowApply(FskConstEffectInnerGlow params, FskConstBitmap src, FskConstRectangle srcRect, FskBitmap dst, FskConstPoint dstPoint, FskEffectCache cache) {
	FskErr				err			= kFskErrNone;
	FskEffectCache		myCache		= NULL;
	FskBitmap			mid1		= NULL;
	FskBitmap			mid2		= NULL;
	FskRectangleRecord	texRect;
	FskConstBitmap		mat;
	FskConstRectangle	matRect;
	FskEffectParametersRecord	p;

	#if defined(LOG_PARAMETERS)
		LogEffectsParameters(__FUNCTION__, params, src, srcRect, dst, dstPoint, cache);
		LOGD("\tparams: radius=%d, blurSigma=%.3g", params->radius, params->blurSigma);
		LogColor(&params->color, "color");
	#endif /* LOG_PARAMETERS */

	/* We need a cache */
	if (!cache) {
		BAIL_IF_ERR(err = FskGLEffectCacheNew(&myCache));
		cache = myCache;
	}
	if (!srcRect)	srcRect  = &src->bounds;

	/* Allocate two mid buffers */
	mat     = src;		/* The src might be copied to a tmp if the pixels are not appropriately isolated */
	matRect = srcRect;
	BAIL_IF_ERR(err = AssureIsolatedPixels(&mat, &matRect, dst, &cache, &texRect, &mid2, NULL));							/* This might allocate mid2 */
	BAIL_IF_ERR(err = GLEffectCacheGetSrcCompatibleBitmap(cache, src, matRect->width, matRect->height, &mid1));
	if (!mid2)																												/* If mid2 wasn't allocated by AssureIsolatedPixels, ... */
		BAIL_IF_ERR(err = GLEffectCacheGetSrcCompatibleBitmap(cache, src, matRect->width, matRect->height, &mid2));			/* ... allocate one here */

	/* H erode */
	p.directionalErode.radius       = params->radius;
	p.directionalErode.direction[0] = 1.f;
	p.directionalErode.direction[1] = 0.f;
	BAIL_IF_ERR(err = FskGLEffectDirectionalErodeApply(&p.directionalErode, mat, matRect, mid1, NULL, cache));				/* H erode src --> mid1 */

	/* V erode */
	p.directionalErode.direction[0] = 0.f;
	p.directionalErode.direction[1] = 1.f;
	BAIL_IF_ERR(err = FskGLEffectDirectionalErodeApply(&p.directionalErode, mid1, NULL, mid2, NULL, cache));				/* V erode mid1 --> mid2 */

	/* H blur */
	p.directionalGaussianBlur.sigma        = params->blurSigma;
	p.directionalGaussianBlur.direction[0] = 1.f;
	p.directionalGaussianBlur.direction[1] = 0.f;
	BAIL_IF_ERR(err = FskGLEffectDirectionalGaussianBlurApply(&p.directionalGaussianBlur, mid2, NULL, mid1, NULL, cache));	/* H blur mid2 --> mid1 */

	/* V blur */
	p.directionalGaussianBlur.direction[0] = 0.f;
	p.directionalGaussianBlur.direction[1] = 1.f;
	BAIL_IF_ERR(err = FskGLEffectDirectionalGaussianBlurApply(&p.directionalGaussianBlur, mid1, NULL, mid2, NULL, cache));	/* V blur mid1 --> mid2 */

	/* Colorize alpha and blend */
	p.colorizeInner.color     = params->color;
	p.colorizeInner.matte     = mid2;
	BAIL_IF_ERR(err = FskGLEffectColorizeInnerApply(&p.colorizeInner, src, srcRect, dst, dstPoint, cache));					/* colorize mid2 blended with dst */

bail:
	FskGLEffectCacheReleaseBitmap(cache, mid2);
	FskGLEffectCacheReleaseBitmap(cache, mid1);
	FskGLEffectCacheDispose(myCache);
	return err;
}


/********************************************************************************
 * FskGLEffectInnerShadowApply
 ********************************************************************************/

static FskErr FskGLEffectInnerShadowApply(FskConstEffectInnerShadow params, FskConstBitmap src, FskConstRectangle srcRect, FskBitmap dst, FskConstPoint dstPoint, FskEffectCache cache) {
	FskErr				err			= kFskErrNone;
	FskEffectCache		myCache		= NULL;
	FskBitmap			mid1		= NULL;
	FskBitmap			mid2		= NULL;
	FskRectangleRecord	texRect;
	FskPointRecord		mpt;
	FskConstBitmap		mat;
	FskConstRectangle	matRect;
	FskEffectParametersRecord	p;

	#if defined(LOG_PARAMETERS)
		LogEffectsParameters(__FUNCTION__, params, src, srcRect, dst, dstPoint, cache);
		LOGD("\tparams: offset=(%d, %d), blurSigma=%.3g", params->offset.x, params->offset.y, params->blurSigma);
		LogColor(&params->color, "color");
	#endif /* LOG_PARAMETERS */

	/* We need a cache */
	if (!cache) {
		BAIL_IF_ERR(err = FskGLEffectCacheNew(&myCache));
		cache = myCache;
	}
	if (!srcRect)	srcRect  = &src->bounds;


	/* Allocate two mid buffers */
	mat     = src;		/* The src might be copied to a tmp if the pixels are not appropriately isolated */
	matRect = srcRect;
	BAIL_IF_ERR(err = AssureIsolatedPixels(&mat, &matRect, dst, &cache, &texRect, &mid2, NULL));								/* This might allocate mid2 */
	BAIL_IF_ERR(err = GLEffectCacheGetSrcCompatibleBitmap(cache, src, matRect->width, matRect->height, &mid1));
	if (!mid2)																													/* If mid2 wasn't allocated by AssureIsolatedPixels, ... */
		BAIL_IF_ERR(err = GLEffectCacheGetSrcCompatibleBitmap(cache, src, matRect->width, matRect->height, &mid2));				/* ... allocate one here */

	/* H blur */
	p.directionalGaussianBlur.sigma        = params->blurSigma;
	p.directionalGaussianBlur.direction[0] = 1.f;
	p.directionalGaussianBlur.direction[1] = 0.f;
	mpt.x = params->offset.x;																									/* Offset in X while blurring horizontally */
	mpt.y = 0;
	#if !ALWAYS_CLEAR_DST_TEXTURE
		FskGLRenderToBitmapTexture(mid1, &blank);																				/* Clear mid1 texture, especially outside of offset region */
	#endif /* !ALWAYS_CLEAR_DST_TEXTURE */
	BAIL_IF_ERR(err = FskGLEffectDirectionalGaussianBlurApply(&p.directionalGaussianBlur, mat, matRect, mid1, &mpt, cache));	/* H blur src --> mid1 */

	/* V blur */
	p.directionalGaussianBlur.direction[0] = 0.f;
	p.directionalGaussianBlur.direction[1] = 1.f;
	mpt.x = 0;
	mpt.y = params->offset.y;																									/* Offset in Y while blurring vertically. */
	#if !ALWAYS_CLEAR_DST_TEXTURE
		FskGLRenderToBitmapTexture(mid2, &blank);																				/* Clear mid2 texture, especially outside of offset region */
	#endif /* !ALWAYS_CLEAR_DST_TEXTURE */
	BAIL_IF_ERR(err = FskGLEffectDirectionalGaussianBlurApply(&p.directionalGaussianBlur, mid1, NULL, mid2, &mpt, cache));		/* V blur mid1 --> mid2 */

	/* Colorize alpha and blend */
	p.colorizeInner.color     = params->color;
	p.colorizeInner.matte     = mid2;
	BAIL_IF_ERR(err = FskGLEffectColorizeInnerApply(&p.colorizeInner, src, srcRect, dst, dstPoint, cache));						/* colorize mid2 blended with dst */

bail:
	FskGLEffectCacheReleaseBitmap(cache, mid2);
	FskGLEffectCacheReleaseBitmap(cache, mid1);
	FskGLEffectCacheDispose(myCache);
	return err;
}


/********************************************************************************
 * FskGLEffectOuterGlowApply
 ********************************************************************************/

static FskErr FskGLEffectOuterGlowApply(FskConstEffectOuterGlow params, FskConstBitmap src, FskConstRectangle srcRect, FskBitmap dst, FskConstPoint dstPoint, FskEffectCache cache) {
	FskErr				err			= kFskErrNone;
	FskEffectCache		myCache		= NULL;
	FskBitmap			mid1		= NULL;
	FskBitmap			mid2		= NULL;
	FskRectangleRecord	texRect;
	FskConstBitmap		mat;
	FskConstRectangle	matRect;
	FskEffectParametersRecord	p;

	#if defined(LOG_PARAMETERS)
		LogEffectsParameters(__FUNCTION__, params, src, srcRect, dst, dstPoint, cache);
		LOGD("\tparams: radius=%d, blurSigma=%.3g", params->radius, params->blurSigma);
		LogColor(&params->color, "color");
	#endif /* LOG_PARAMETERS */

	/* We need a cache */
	if (!cache) {
		BAIL_IF_ERR(err = FskGLEffectCacheNew(&myCache));
		cache = myCache;
	}
	if (!srcRect)	srcRect  = &src->bounds;

	/* Allocate two mid buffers */
	mat     = src;		/* The src might be copied to a tmp if the pixels are not appropriately isolated */
	matRect = srcRect;
	BAIL_IF_ERR(err = AssureIsolatedPixels(&mat, &matRect, dst, &cache, &texRect, &mid2, NULL));							/* This might allocate mid2 */
	BAIL_IF_ERR(err = GLEffectCacheGetSrcCompatibleBitmap(cache, src, matRect->width, matRect->height, &mid1));
	if (!mid2)																												/* If mid2 wasn't allocated by AssureIsolatedPixels, ... */
		BAIL_IF_ERR(err = GLEffectCacheGetSrcCompatibleBitmap(cache, src, matRect->width, matRect->height, &mid2));			/* ... allocate one here */

	/* H dilate */
	p.directionalDilate.radius       = params->radius;
	p.directionalDilate.direction[0] = 1.f;
	p.directionalDilate.direction[1] = 0.f;
	BAIL_IF_ERR(err = FskGLEffectDirectionalDilateApply(&p.directionalDilate, mat, matRect, mid1, NULL, cache));			/* H dilate src --> mid1 */

	/* V dilate */
	p.directionalDilate.direction[0] = 0.f;
	p.directionalDilate.direction[1] = 1.f;
	BAIL_IF_ERR(err = FskGLEffectDirectionalDilateApply(&p.directionalDilate, mid1, NULL, mid2, NULL, cache));				/* V dilate mid1 --> mid2 */

	/* H blur */
	p.directionalGaussianBlur.sigma        = params->blurSigma;
	p.directionalGaussianBlur.direction[0] = 1.f;
	p.directionalGaussianBlur.direction[1] = 0.f;
	BAIL_IF_ERR(err = FskGLEffectDirectionalGaussianBlurApply(&p.directionalGaussianBlur, mid2, NULL, mid1, NULL, cache));	/* H blur mid2 --> mid1 */

	/* V blur */
	p.directionalGaussianBlur.direction[0] = 0.f;
	p.directionalGaussianBlur.direction[1] = 1.f;
	BAIL_IF_ERR(err = FskGLEffectDirectionalGaussianBlurApply(&p.directionalGaussianBlur, mid1, NULL, mid2, NULL, cache));	/* V blur mid1 --> mid2 */

	p.colorizeOuter.color     = params->color;
	p.colorizeOuter.matte     = mid2;
	BAIL_IF_ERR(err = FskGLEffectColorizeOuterApply(&p.colorizeOuter, src, srcRect, dst, dstPoint, cache));					/* colorize mid2 blended with dst */

bail:
	FskGLEffectCacheReleaseBitmap(cache, mid2);
	FskGLEffectCacheReleaseBitmap(cache, mid1);
	FskGLEffectCacheDispose(myCache);
	return err;
}


/********************************************************************************
 * FskGLEffectOuterShadowApply
 ********************************************************************************/

static FskErr FskGLEffectOuterShadowApply(FskConstEffectOuterShadow params, FskConstBitmap src, FskConstRectangle srcRect, FskBitmap dst, FskConstPoint dstPoint, FskEffectCache cache) {
	FskErr				err			= kFskErrNone;
	FskEffectCache		myCache		= NULL;
	FskBitmap			mid1		= NULL;
	FskBitmap			mid2		= NULL;
	FskRectangleRecord	texRect;
	FskPointRecord		mpt;
	FskConstBitmap		mat;
	FskConstRectangle	matRect;
	FskEffectParametersRecord	p;

	#if defined(LOG_PARAMETERS)
		LogEffectsParameters(__FUNCTION__, params, src, srcRect, dst, dstPoint, cache);
		LOGD("\tparams: offset=(%d, %d), blurSigma=%.3g", params->offset.x, params->offset.y, params->blurSigma);
		LogColor(&params->color, "color");
	#endif /* LOG_PARAMETERS */

	/* We need a cache */
	if (!cache) {
		BAIL_IF_ERR(err = FskGLEffectCacheNew(&myCache));
		cache = myCache;
	}
	if (!srcRect)	srcRect  = &src->bounds;

	/* Allocate two mid buffers */
	mat     = src;		/* The src might be copied to a tmp if the pixels are not appropriately isolated */
	matRect = srcRect;
	BAIL_IF_ERR(err = AssureIsolatedPixels(&mat, &matRect, dst, &cache, &texRect, &mid2, NULL));							/* This might allocate mid2 */
	BAIL_IF_ERR(err = GLEffectCacheGetSrcCompatibleBitmap(cache, src, matRect->width, matRect->height, &mid1));
	if (!mid2)																												/* If mid2 wasn't allocated by AssureIsolatedPixels, ... */
		BAIL_IF_ERR(err = GLEffectCacheGetSrcCompatibleBitmap(cache, src, matRect->width, matRect->height, &mid2));			/* ... allocate one here */

	/* H blur */
	p.directionalGaussianBlur.sigma        = params->blurSigma;
	p.directionalGaussianBlur.direction[0] = 1.f;
	p.directionalGaussianBlur.direction[1] = 0.f;
	mpt.x = params->offset.x;																								/* Offset in X while blurring horizontally */
	mpt.y = 0;
	#if !ALWAYS_CLEAR_DST_TEXTURE
		FskGLRenderToBitmapTexture(mid1, &blank);																			/* Clear mid1 texture, especially outside of offset region */
	#endif /* !ALWAYS_CLEAR_DST_TEXTURE */
	BAIL_IF_ERR(err = FskGLEffectDirectionalGaussianBlurApply(&p.directionalGaussianBlur, mat, NULL, mid1, &mpt, cache));	/* H blur src --> mid1 */

	/* V blur */
	p.directionalGaussianBlur.direction[0] = 0.f;
	p.directionalGaussianBlur.direction[1] = 1.f;
	mpt.x = 0;
	mpt.y = params->offset.y;																								/* Offset in Y while blurring vertically. */
	#if !ALWAYS_CLEAR_DST_TEXTURE
		FskGLRenderToBitmapTexture(mid2, &blank);																			/* Clear mid2 texture, especially outside of offset region */
	#endif /* !ALWAYS_CLEAR_DST_TEXTURE */
	BAIL_IF_ERR(err = FskGLEffectDirectionalGaussianBlurApply(&p.directionalGaussianBlur, mid1, NULL, mid2, &mpt, cache));	/* V blur mid1 --> mid2 */

	/* Colorize alpha and blend. */
	p.colorizeOuter.color     = params->color;
	p.colorizeOuter.matte     = mid2;
	BAIL_IF_ERR(err = FskGLEffectColorizeOuterApply(&p.colorizeOuter, src, srcRect, dst, dstPoint, cache));					/* colorize mid2 blended with dst */

bail:
	FskGLEffectCacheReleaseBitmap(cache, mid2);
	FskGLEffectCacheReleaseBitmap(cache, mid1);
	FskGLEffectCacheDispose(myCache);
	return err;
}


/********************************************************************************
 * FskGLEffectCompoundApply
 ********************************************************************************/

static FskErr FskGLEffectCompoundApply(FskConstEffect effect, FskConstBitmap src, FskConstRectangle srcRect, FskBitmap dst, FskConstPoint dstPoint, FskEffectCache cache) {
	FskErr			err		= kFskErrNone;
	FskEffectCache	myCache	= NULL;
	FskBitmap		mid1	= NULL,
					mid2	= NULL;
	FskBitmap		dbm;
	int				numStages;

	#if defined(LOG_PARAMETERS)
		LOGD("%s(effect=%p, src=%p, srcRect=%p, dst=%p, dstPoint=%p)", __FUNCTION__, effect, src, srcRect, dst, dstPoint);
		LogSrcBitmap(src, "src");
		LogRect(srcRect, "srcRect");
		LogDstBitmap(dst, "dst");
		LogPoint(dstPoint, "dstPoint");
		LOGD("\teffectID=%d (%s) topology=%d (%s), numStages=%d", effect->effectID, ((effect->effectID == kFskEffectCompound) ? "compound" : "???"),
			effect->params.compound.topology, GetTopologyNameFromCode(effect->params.compound.topology), effect->params.compound.numStages);
	#endif /* LOG_PARAMETERS */

	BAIL_IF_NONPOSITIVE(numStages = effect->params.compound.numStages, err, kFskErrEmpty);

	if (1 == numStages)	return FskGLEffectApply(effect + 1, src, srcRect, dst, dstPoint, cache);					/* No intermediate buffers needed for a single stage */

	if (srcRect == NULL)	srcRect = &src->bounds;																	/* Make sure that we know the bounds */
	if (!cache) {																									/* We need a cache */
		BAIL_IF_ERR(err = FskGLEffectCacheNew(&myCache));
		cache = myCache;
	}
	BAIL_IF_ERR(err = GLEffectCacheGetSrcCompatibleBitmap(cache, src, srcRect->width, srcRect->height, &mid1));		/* Allocate mid1 buffer */
	if (numStages > 2)
		BAIL_IF_ERR(err = GLEffectCacheGetSrcCompatibleBitmap(cache, src, srcRect->width, srcRect->height, &mid2));	/* Allocate mid2 buffer */
	dbm = mid1;																										/* mid1 is the first destination bitmap */

	switch (effect->params.compound.topology) {

		case kFskEffectCompoundTopologyPipeline:
			for (++effect; numStages-- > 1; ++effect, src = dbm, dbm = ((dbm == mid1) ? mid2 : mid1), srcRect = NULL)
				BAIL_IF_ERR(err = FskGLEffectApply(effect, src, srcRect, dbm, NULL, cache));
			BAIL_IF_ERR(err = FskGLEffectApply(effect, src, NULL, dst, dstPoint, cache));
			break;

		default:
			BAIL(kFskErrNetworkInterfaceError);
	}

bail:
	FskGLEffectCacheReleaseBitmap(cache, mid2);
	FskGLEffectCacheReleaseBitmap(cache, mid1);
	FskGLEffectCacheDispose(myCache);
	return err;
}


#if 0
#pragma mark -
#endif /* 0 */
/********************************************************************************
 ********************************************************************************
 ********************************************************************************
 *****									API									*****
 ********************************************************************************
 ********************************************************************************
 ********************************************************************************/


/********************************************************************************
 * FskGLEffectApplyMode
 ********************************************************************************/

FskErr FskGLEffectApplyMode(FskConstEffect effect, FskConstBitmap src, FskConstRectangle srcRect, FskBitmap dst, FskConstPoint dstPoint, FskEffectCache cache,
							FskConstRectangle clipRect, UInt32 mode, FskConstGraphicsModeParameters modeParams) {
	FskErr err;

	#if defined(LOG_PARAMETERS)
		LOGD("%s(effect=%p, src=%p, srcRect=%p, dst=%p, dstPoint=%p, cache=%p, clipRect=%p, mode=%s, modeParams=%p)",
			__FUNCTION__, effect, src, srcRect, dst, dstPoint, cache, clipRect, ModeNameFromCode(mode), modeParams);
		LogSrcBitmap(src, "src");
		LogRect(srcRect, "srcRect");
		LogDstBitmap(dst, "dst");
		LogPoint(dstPoint, "dstPoint");
		LogRect(clipRect, "clipRect");
		if (modeParams)
			LOGD("\tmodeParams(dataSize=%u blendLevel=%d)", modeParams->dataSize, modeParams->blendLevel);
	#endif /* LOG_PARAMETERS */

	if (!gEffectsGlobals.gidCopyProgram)	/* Make sure that the shaders are initialized */
		FskGLEffectsInit();

	if (modeParams && (modeParams->blendLevel != 255))
		err = kFskErrUnimplemented;																	/* We don't do blending; we do copy instead */

	switch (mode & kFskGraphicsModeMask) {
		case kFskGraphicsModeColorize:
		default:
			err =  kFskErrUnimplemented;															/* We don't colorize */
			goto useCopyMode;

		case kFskGraphicsModeAlpha:
			if (src->hasAlpha) {
				#if GLES_VERSION < 2
					glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
				#endif /* GLES_VERSION < 2 */
				if (!src->alphaIsPremultiplied)	FskGLSetBlend(true, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
				else							FskGLSetBlend(true, GL_ONE,       GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
				break;
			} /* If it doesn't have alpha, use copy mode */

		case kFskGraphicsModeCopy:
		useCopyMode:
			FskGLSetBlend(false, GL_ONE, GL_ZERO, GL_ONE, GL_ZERO);									/* Turn blending off at this level in the API */
			break;
	}

	FskGLPortSetClip(dst->glPort, clipRect);

	switch (effect->effectID) {
		case kFskEffectBoxBlur:					err = FskGLEffectBoxBlurApply(					&effect->params.boxBlur,					src, srcRect, dst, dstPoint, cache); break;
		case kFskEffectColorize:				err = FskGLEffectColorizeApply(					&effect->params.colorize,					src, srcRect, dst, dstPoint, cache); break;
		case kFskEffectColorizeAlpha:			err = FskGLEffectColorizeAlphaApply(			&effect->params.colorizeAlpha,				src, srcRect, dst, dstPoint, cache); break;
		case kFskEffectColorizeInner:			err = FskGLEffectColorizeInnerApply(			&effect->params.colorizeInner,				src, srcRect, dst, dstPoint, cache); break;
		case kFskEffectColorizeOuter:			err = FskGLEffectColorizeOuterApply(			&effect->params.colorizeOuter,				src, srcRect, dst, dstPoint, cache); break;
		case kFskEffectCompound:				err = FskGLEffectCompoundApply(					effect,										src, srcRect, dst, dstPoint, cache); break;
		case kFskEffectCopy:					err = FskGLEffectCopyApply(						&effect->params.copy,						src, srcRect, dst, dstPoint, cache); break;
		case kFskEffectCopyMirrorBorders:		err = FskGLEffectCopyMirrorBordersApply(		&effect->params.copyMirrorBorders,			src, srcRect, dst, dstPoint, cache); break;
		case kFskEffectDilate:					err = FskGLEffectDilateApply(					&effect->params.dilate,						src, srcRect, dst, dstPoint, cache); break;
		case kFskEffectDirectionalBoxBlur:		err = FskGLEffectDirectionalBoxBlurApply(		&effect->params.directionalBoxBlur,			src, srcRect, dst, dstPoint, cache); break;
		case kFskEffectDirectionalDilate:		err = FskGLEffectDirectionalDilateApply(		&effect->params.directionalDilate,			src, srcRect, dst, dstPoint, cache); break;
		case kFskEffectDirectionalErode:		err = FskGLEffectDirectionalErodeApply(			&effect->params.directionalErode,			src, srcRect, dst, dstPoint, cache); break;
		case kFskEffectDirectionalGaussianBlur:	err = FskGLEffectDirectionalGaussianBlurApply(	&effect->params.directionalGaussianBlur,	src, srcRect, dst, dstPoint, cache); break;
		case kFskEffectErode:					err = FskGLEffectErodeApply(					&effect->params.erode,						src, srcRect, dst, dstPoint, cache); break;
		case kFskEffectGaussianBlur:			err = FskGLEffectGaussianBlurApply(				&effect->params.gaussianBlur,				src, srcRect, dst, dstPoint, cache); break;
		case kFskEffectInnerGlow:				err = FskGLEffectInnerGlowApply(				&effect->params.innerGlow,					src, srcRect, dst, dstPoint, cache); break;
		case kFskEffectInnerShadow:				err = FskGLEffectInnerShadowApply(				&effect->params.innerShadow,				src, srcRect, dst, dstPoint, cache); break;
		case kFskEffectMask:					err = FskGLEffectMaskApply(						&effect->params.mask,						src, srcRect, dst, dstPoint, cache); break;
		case kFskEffectMonochrome:				err = FskGLEffectMonochromeApply(				&effect->params.monochrome,					src, srcRect, dst, dstPoint, cache); break;
		case kFskEffectOuterGlow:				err = FskGLEffectOuterGlowApply(				&effect->params.outerGlow,					src, srcRect, dst, dstPoint, cache); break;
		case kFskEffectOuterShadow:				err = FskGLEffectOuterShadowApply(				&effect->params.outerShadow,				src, srcRect, dst, dstPoint, cache); break;
		case kFskEffectShade:					err = FskGLEffectShadeApply(					&effect->params.shade,						src, srcRect, dst, dstPoint, cache); break;
		case kFskEffectPremultiplyAlpha:		err = FskGLEffectPremultiplyAlphaApply(			&effect->params.premultiplyAlpha,			src, srcRect, dst, dstPoint, cache); break;
		case kFskEffectStraightAlpha:			err = FskGLEffectStraightAlphaApply(			&effect->params.straightAlpha,				src, srcRect, dst, dstPoint, cache); break;

		default:								err = kFskErrUnimplemented; break;
	}

	return err;
}


/********************************************************************************
 * FskGLEffectApply
 ********************************************************************************/

FskErr FskGLEffectApply(FskConstEffect effect, FskConstBitmap src, FskConstRectangle srcRect, FskBitmap dst, FskConstPoint dstPoint, FskEffectCache cache) {
	return FskGLEffectApplyMode(effect, src, srcRect, dst, dstPoint, cache, NULL, kFskGraphicsModeCopy, NULL);
}


static const char	vPosition_str[] = "vPosition",
					vTexCoord_str[]	= "vTexCoord";


/********************************************************************************
 * InitFragmentShaderProgram
 ********************************************************************************/

static FskErr InitFragmentShaderProgram(GLuint vertexShader, GLuint fragmentShader, GLuint *pgidProg, ...) {
	GLint		result		= 0;
	GLuint		id;
	const char	*shader;
	va_list		ap;
	#if CHECK_GL_ERROR
		const char *lastShader;
	#endif /* CHECK_GL_ERROR */

	*pgidProg = id = 0;
	va_start(ap, pgidProg);
	while (NULL != (shader = va_arg(ap, const char*))) {						/* Try shaders in order */
		#if CHECK_GL_ERROR
			lastShader = shader;
		#endif /* CHECK_GL_ERROR */
		glShaderSource(fragmentShader, 1, &shader, NULL);
		glCompileShader(fragmentShader);
		glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &result);
		if (result) {															/* The shader compiles */
			id = glCreateProgram();
			glAttachShader(id, vertexShader);
			glAttachShader(id, fragmentShader);
			glBindAttribLocation(id, VERTEX_POSITION_INDEX, vPosition_str);
			glBindAttribLocation(id, VERTEX_TEXCOORD_INDEX, vTexCoord_str);
			glLinkProgram(id);
			glGetProgramiv(id, GL_LINK_STATUS, &result);
			if (result)															/* The program links */
				break;
		}
	}
	va_end(ap);

	if (result) {
		*pgidProg = id;
		//LOGD("Using Shader:\n%s", shader);
		return kFskErrNone;
	}
	#if CHECK_GL_ERROR
		else {
			if (id == 0)	PrintShaderLog(fragmentShader, GL_FRAGMENT_SHADER, lastShader);
			else			PrintProgramLog(id);

		}
	#endif /* CHECK_GL_ERROR */
	return kFskErrGLShader;
}


/********************************************************************************
 * FskGLEffectsInit
 ********************************************************************************/

FskErr FskGLEffectsInit(void) {
	FskErr				err				= kFskErrNone;
	static const char	matrix_str[]	= "matrix",
						coeff_str[]		= "coeff",
						rad_str[]		= "rad",
						srcBM_str[]		= "srcBM",
						mttBM_str[]		= "mttBM",
						srcDelta_str[]	= "srcDelta",
						color_str[]		= "color",
						color0_str[]	= "color0",
						color1_str[]	= "color1";
	const char			*extensions		= (const char*)glGetString(GL_EXTENSIONS);
	const GLfloat		grayColor[4]	= { .5f, .5f, .5f, 1.f };
	const GLfloat		identity[3*3]	= { 1.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 1.f };
	GLTexVertex			*vertices;
	GLuint				vertexShader, fragmentShader;
	#if CHECK_GL_ERROR
		GLint			result;
	#endif /* CHECK_GL_ERROR */


	#if defined(LOG_PARAMETERS)
		LOGE("FskGLEffectsInit");
	#endif /* LOG_PARAMETERS */

	FskMemSet(&gEffectsGlobals, 0, sizeof(gEffectsGlobals));


	/* Determine capabilities */
	gEffectsGlobals.canWrapNPOT =	FskStrStr(extensions, "GL_OES_texture_npot")
								||	FskStrStr(extensions, "GL_ARB_texture_non_power_of_two");
	gEffectsGlobals.rendererIsTiled = FskStrStr(extensions, "GL_QCOM_tiled_rendering") != NULL;
	#if defined(GL_VERSION_2_0)																		/* GL always has high precision */
		gEffectsGlobals.hasHighPrecision = true;
	#else																							/* GL ES */
		gEffectsGlobals.hasHighPrecision = (FskStrStr(extensions, "GL_OES_fragment_precision_high") != NULL);
	#endif /* GL_VERSION */

	gEffectsGlobals.mediumPrecisionBits		=  24;													/* GL always has high precision */
	gEffectsGlobals.mediumPrecisionRange[0]	= 128;
	gEffectsGlobals.mediumPrecisionRange[1]	= 128;
	#if !defined(GL_VERSION_2_0)																	/* GL ES might only have medium precision */
	{	GLint floatRange[2]  = { 0, 0 };
		GLint floatPrecision = 0;
		#ifdef GL_MEDIUM_FLOAT
			glGetShaderPrecisionFormat(GL_FRAGMENT_SHADER, GL_MEDIUM_FLOAT, floatRange, &floatPrecision);	/* Precision doesn't count the hidden bit */
			if (floatPrecision)	gEffectsGlobals.mediumPrecisionBits		= floatPrecision + 1;				/* Including the hidden bit */
			if (floatRange[0])	gEffectsGlobals.mediumPrecisionRange[0]	= floatRange[0];
			if (floatRange[1])	gEffectsGlobals.mediumPrecisionRange[1]	= floatRange[1];
		#endif /* GL_MEDIUM_FLOAT */
	}
	#endif /* GL_VERSION */


	/* Initialize Vertex Attribute Pointers */
	BAIL_IF_ERR(err = FskGLGetCoordinatePointer((float**)(void*)(&vertices), NULL));
	glVertexAttribPointer(VERTEX_POSITION_INDEX, 2, GL_FLOAT, GL_FALSE, sizeof(GLTexVertex), &vertices->x);
	glVertexAttribPointer(VERTEX_TEXCOORD_INDEX, 2, GL_FLOAT, GL_FALSE, sizeof(GLTexVertex), &vertices->u);
	glEnableVertexAttribArray(VERTEX_POSITION_INDEX);
	glEnableVertexAttribArray(VERTEX_TEXCOORD_INDEX);


	/*
	 * Initialize 1 source programs
	 */


	/* Initialize vertex shaders */
	BAIL_IF_ERR(err = FskGLNewShader(gEffectsVertexShader, GL_VERTEX_SHADER, &vertexShader));

	/* Initialize Copy shader */
	BAIL_IF_ERR(err = FskGLNewShader(gCopyFragmentShader, GL_FRAGMENT_SHADER, &fragmentShader));
	BAIL_IF_ERR(err = FskGLNewProgram(vertexShader, fragmentShader, &gEffectsGlobals.gidCopyProgram,
						 VERTEX_POSITION_INDEX, vPosition_str,
						 VERTEX_TEXCOORD_INDEX, vTexCoord_str,
						 VERTEX_END_INDEX));
	glDeleteShader(fragmentShader);
	BAIL_IF_ZERO(gEffectsGlobals.gidCopyProgram, err, kFskErrGLShader);
	BAIL_IF_NEGATIVE(gEffectsGlobals.gidCopyMatrix   = glGetUniformLocation(gEffectsGlobals.gidCopyProgram,	matrix_str),	err, kFskErrGLShader);
	BAIL_IF_NEGATIVE(gEffectsGlobals.gidCopySrcBM    = glGetUniformLocation(gEffectsGlobals.gidCopyProgram,	srcBM_str),		err, kFskErrGLShader);
	BAIL_IF_ERR(err = FskGLUseProgram(gEffectsGlobals.gidCopyProgram));
	glUniformMatrix3fv(gEffectsGlobals.gidCopyMatrix, 1, GL_FALSE, identity);						/* matrix */
	glUniform1i(gEffectsGlobals.gidCopySrcBM, 0);													/* srcBM is in texture #0 */
	#if CHECK_GL_ERROR
		glValidateProgram(gEffectsGlobals.gidCopyProgram);
		glGetProgramiv(gEffectsGlobals.gidCopyProgram, GL_VALIDATE_STATUS, &result);
		BAIL_IF_ZERO(result, err, kFskErrGLShader);
	#endif /* CHECK_GL_ERROR */

	/* Initialize Gaussian blur shader, program and attributes */
	fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	#if !TARGET_OS_WIN32
		if (!gEffectsGlobals.hasHighPrecision)	err = InitFragmentShaderProgram(vertexShader, fragmentShader, &gEffectsGlobals.gidGaussianBlurProgram,
																				gGaussianBlurFragmentShaderL, gGaussianBlurFragmentShaderU, NULL);
		else									err = InitFragmentShaderProgram(vertexShader, fragmentShader, &gEffectsGlobals.gidGaussianBlurProgram,
																				gGaussianBlurFragmentShaderH, gGaussianBlurFragmentShaderV, NULL);
	#else /* TARGET_OS_WIN32 */
		err = InitFragmentShaderProgram(vertexShader, fragmentShader, &gEffectsGlobals.gidGaussianBlurProgram,
										gGaussianBlurFragmentShaderW, NULL);
	#endif /* TARGET_OS_WIN32 */
	glDeleteShader(fragmentShader);
	BAIL_IF_ERR(err);
	BAIL_IF_ZERO(gEffectsGlobals.gidGaussianBlurProgram, err, kFskErrGLShader);
	BAIL_IF_NEGATIVE(gEffectsGlobals.gidGaussianBlurMatrix   = glGetUniformLocation(gEffectsGlobals.gidGaussianBlurProgram,	matrix_str),	err, kFskErrGLShader);
	BAIL_IF_NEGATIVE(gEffectsGlobals.gidGaussianBlurCoeff    = glGetUniformLocation(gEffectsGlobals.gidGaussianBlurProgram,	coeff_str),		err, kFskErrGLShader);
	BAIL_IF_NEGATIVE(gEffectsGlobals.gidGaussianBlurRadius   = glGetUniformLocation(gEffectsGlobals.gidGaussianBlurProgram,	rad_str),		err, kFskErrGLShader);
	BAIL_IF_NEGATIVE(gEffectsGlobals.gidGaussianBlurSrcBM    = glGetUniformLocation(gEffectsGlobals.gidGaussianBlurProgram,	srcBM_str),		err, kFskErrGLShader);
	BAIL_IF_NEGATIVE(gEffectsGlobals.gidGaussianBlurSrcDelta = glGetUniformLocation(gEffectsGlobals.gidGaussianBlurProgram,	srcDelta_str),	err, kFskErrGLShader);
	BAIL_IF_ERR(err = FskGLUseProgram(gEffectsGlobals.gidGaussianBlurProgram));
	glUniformMatrix3fv(gEffectsGlobals.gidGaussianBlurMatrix, 1, GL_FALSE, identity);				/* matrix */
	glUniform3fv(gEffectsGlobals.gidGaussianBlurCoeff, 1, grayColor);								/* coeff */
	glUniform1i(gEffectsGlobals.gidGaussianBlurRadius, 1);											/* radius */
	glUniform2f(gEffectsGlobals.gidGaussianBlurSrcDelta, .01f, 0.f);								/* srcDelta */
	glUniform1i(gEffectsGlobals.gidGaussianBlurSrcBM, 0);											/* srcBM is in texture #0 */
	#if CHECK_GL_ERROR
		glValidateProgram(gEffectsGlobals.gidGaussianBlurProgram);
		glGetProgramiv(gEffectsGlobals.gidGaussianBlurProgram, GL_VALIDATE_STATUS, &result);
		BAIL_IF_ZERO(result, err, kFskErrGLShader);
	#endif /* CHECK_GL_ERROR */

	/* Initialize Box blur shader, program and attributes */
	fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	#if !TARGET_OS_WIN32
		err = InitFragmentShaderProgram(vertexShader, fragmentShader, &gEffectsGlobals.gidBoxBlurProgram,
										gBoxBlurFragmentShaderL, gBoxBlurFragmentShaderU, NULL);
	#else /* TARGET_OS_WIN32 */
		err = InitFragmentShaderProgram(vertexShader, fragmentShader, &gEffectsGlobals.gidBoxBlurProgram,
										gBoxBlurFragmentShaderW, NULL);
	#endif /* TARGET_OS_WIN32 */
	BAIL_IF_ERR(err);
	glDeleteShader(fragmentShader);
	BAIL_IF_ZERO(gEffectsGlobals.gidBoxBlurProgram, err, kFskErrGLShader);
	BAIL_IF_NEGATIVE(gEffectsGlobals.gidBoxBlurMatrix   = glGetUniformLocation(gEffectsGlobals.gidBoxBlurProgram,	matrix_str),	err, kFskErrGLShader);
	BAIL_IF_NEGATIVE(gEffectsGlobals.gidBoxBlurRadius   = glGetUniformLocation(gEffectsGlobals.gidBoxBlurProgram,	rad_str),		err, kFskErrGLShader);
	BAIL_IF_NEGATIVE(gEffectsGlobals.gidBoxBlurSrcBM    = glGetUniformLocation(gEffectsGlobals.gidBoxBlurProgram,	srcBM_str),		err, kFskErrGLShader);
	BAIL_IF_NEGATIVE(gEffectsGlobals.gidBoxBlurSrcDelta = glGetUniformLocation(gEffectsGlobals.gidBoxBlurProgram,	srcDelta_str),	err, kFskErrGLShader);
	BAIL_IF_ERR(err = FskGLUseProgram(gEffectsGlobals.gidBoxBlurProgram));
	glUniformMatrix3fv(gEffectsGlobals.gidBoxBlurMatrix, 1, GL_FALSE, identity);					/* matrix */
	glUniform1i(gEffectsGlobals.gidBoxBlurRadius, 1);												/* radius */
	glUniform3f(gEffectsGlobals.gidBoxBlurSrcDelta, .01f, 0.f, 1.f);								/* srcDelta(x,y), norm */
	glUniform1i(gEffectsGlobals.gidBoxBlurSrcBM, 0);												/* srcBM is in texture #0 */
	#if CHECK_GL_ERROR
		glValidateProgram(gEffectsGlobals.gidBoxBlurProgram);
		glGetProgramiv(gEffectsGlobals.gidBoxBlurProgram, GL_VALIDATE_STATUS, &result);
		BAIL_IF_ZERO(result, err, kFskErrGLShader);
	#endif /* CHECK_GL_ERROR */

	/* Initialize Dilate shader, program and attributes */
	fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	#if !TARGET_OS_WIN32
		err = InitFragmentShaderProgram(vertexShader, fragmentShader, &gEffectsGlobals.gidDilateProgram, gDilateFragmentShaderL, gDilateFragmentShaderU, NULL);
	#else /* TARGET_OS_WIN32 */
		err = InitFragmentShaderProgram(vertexShader, fragmentShader, &gEffectsGlobals.gidDilateProgram, gDilateFragmentShaderW, NULL);
	#endif /* TARGET_OS_WIN32 */
	BAIL_IF_ERR(err);
	glDeleteShader(fragmentShader);
	BAIL_IF_ZERO(gEffectsGlobals.gidDilateProgram, err, kFskErrGLShader);
	BAIL_IF_NEGATIVE(gEffectsGlobals.gidDilateMatrix   = glGetUniformLocation(gEffectsGlobals.gidDilateProgram, matrix_str),	err, kFskErrGLShader);
	BAIL_IF_NEGATIVE(gEffectsGlobals.gidDilateRadius   = glGetUniformLocation(gEffectsGlobals.gidDilateProgram, rad_str),		err, kFskErrGLShader);
	BAIL_IF_NEGATIVE(gEffectsGlobals.gidDilateSrcBM    = glGetUniformLocation(gEffectsGlobals.gidDilateProgram, srcBM_str),		err, kFskErrGLShader);
	BAIL_IF_NEGATIVE(gEffectsGlobals.gidDilateSrcDelta = glGetUniformLocation(gEffectsGlobals.gidDilateProgram, srcDelta_str),	err, kFskErrGLShader);
	BAIL_IF_ERR(err = FskGLUseProgram(gEffectsGlobals.gidDilateProgram));
	glUniformMatrix3fv(gEffectsGlobals.gidDilateMatrix, 1, GL_FALSE, identity);						/* matrix  */
	glUniform1i(gEffectsGlobals.gidDilateRadius, 1);												/* radius */
	glUniform2f(gEffectsGlobals.gidDilateSrcDelta, .01f, 0.f);										/* srcDelta */
	glUniform1i(gEffectsGlobals.gidDilateSrcBM, 0);													/* srcBM is in texture #0 */
	#if CHECK_GL_ERROR
		glValidateProgram(gEffectsGlobals.gidDilateProgram);
		glGetProgramiv(gEffectsGlobals.gidDilateProgram, GL_VALIDATE_STATUS, &result);
		BAIL_IF_ZERO(result, err, kFskErrGLShader);
	#endif /* CHECK_GL_ERROR */

	/* Initialize Erode shader, program and attributes */
	fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	#if !TARGET_OS_WIN32
		err = InitFragmentShaderProgram(vertexShader, fragmentShader, &gEffectsGlobals.gidErodeProgram, gErodeFragmentShaderL, gErodeFragmentShaderU, NULL);
	#else /* TARGET_OS_WIN32 */
		err = InitFragmentShaderProgram(vertexShader, fragmentShader, &gEffectsGlobals.gidErodeProgram, gErodeFragmentShaderW, NULL);
	#endif /* TARGET_OS_WIN32 */
	BAIL_IF_ERR(err);
	glDeleteShader(fragmentShader);
	BAIL_IF_ZERO(gEffectsGlobals.gidErodeProgram, err, kFskErrGLShader);
	BAIL_IF_NEGATIVE(gEffectsGlobals.gidErodeMatrix   = glGetUniformLocation(gEffectsGlobals.gidErodeProgram, matrix_str),		err, kFskErrGLShader);
	BAIL_IF_NEGATIVE(gEffectsGlobals.gidErodeRadius   = glGetUniformLocation(gEffectsGlobals.gidErodeProgram, rad_str),			err, kFskErrGLShader);
	BAIL_IF_NEGATIVE(gEffectsGlobals.gidErodeSrcBM    = glGetUniformLocation(gEffectsGlobals.gidErodeProgram, srcBM_str),		err, kFskErrGLShader);
	BAIL_IF_NEGATIVE(gEffectsGlobals.gidErodeSrcDelta = glGetUniformLocation(gEffectsGlobals.gidErodeProgram, srcDelta_str),	err, kFskErrGLShader);
	BAIL_IF_ERR(err = FskGLUseProgram(gEffectsGlobals.gidErodeProgram));
	glUniformMatrix3fv(gEffectsGlobals.gidErodeMatrix, 1, GL_FALSE, identity);						/* matrix */
	glUniform1i(gEffectsGlobals.gidErodeRadius, 1);													/* radius */
	glUniform2f(gEffectsGlobals.gidErodeSrcDelta, .01f, 0.f);										/* srcDelta */
	glUniform1i(gEffectsGlobals.gidErodeSrcBM, 0);													/* srcBM is in texture #0 */
	#if CHECK_GL_ERROR
		glValidateProgram(gEffectsGlobals.gidErodeProgram);
		glGetProgramiv(gEffectsGlobals.gidErodeProgram, GL_VALIDATE_STATUS, &result);
		BAIL_IF_ZERO(result, err, kFskErrGLShader);
	#endif /* CHECK_GL_ERROR */

	/* Initialize Colorize Alpha shader, program and attributes */
	BAIL_IF_ERR(err = FskGLNewShader(gColorizeAlphaFragmentShader, GL_FRAGMENT_SHADER, &fragmentShader));
	BAIL_IF_ERR(err = FskGLNewProgram(vertexShader, fragmentShader, &gEffectsGlobals.gidColorizeAlphaProgram,
					 VERTEX_POSITION_INDEX, vPosition_str,
					 VERTEX_TEXCOORD_INDEX, vTexCoord_str,
					 VERTEX_END_INDEX));
	glDeleteShader(fragmentShader);
	BAIL_IF_ZERO(gEffectsGlobals.gidColorizeAlphaProgram, err, kFskErrGLShader);
	BAIL_IF_NEGATIVE(gEffectsGlobals.gidColorizeAlphaMatrix = glGetUniformLocation(gEffectsGlobals.gidColorizeAlphaProgram, matrix_str),	err, kFskErrGLShader);
	BAIL_IF_NEGATIVE(gEffectsGlobals.gidColorizeAlphaColor0 = glGetUniformLocation(gEffectsGlobals.gidColorizeAlphaProgram, color0_str),	err, kFskErrGLShader);
	BAIL_IF_NEGATIVE(gEffectsGlobals.gidColorizeAlphaColor1 = glGetUniformLocation(gEffectsGlobals.gidColorizeAlphaProgram, color1_str),	err, kFskErrGLShader);
	BAIL_IF_NEGATIVE(gEffectsGlobals.gidColorizeAlphaSrcBM  = glGetUniformLocation(gEffectsGlobals.gidColorizeAlphaProgram, srcBM_str),		err, kFskErrGLShader);
	BAIL_IF_ERR(err = FskGLUseProgram(gEffectsGlobals.gidColorizeAlphaProgram));
	glUniform1i(gEffectsGlobals.gidColorizeAlphaSrcBM, 0);											/* srcBM is in texture #0 */
	glUniformMatrix3fv(gEffectsGlobals.gidColorizeAlphaMatrix, 1, GL_FALSE, identity);				/* matrix */
	glUniform4fv(gEffectsGlobals.gidColorizeAlphaColor0, 1, grayColor);								/* color0 */
	glUniform4fv(gEffectsGlobals.gidColorizeAlphaColor1, 1, grayColor);								/* color1 */
	#if CHECK_GL_ERROR
		glValidateProgram(gEffectsGlobals.gidColorizeAlphaProgram);
		glGetProgramiv(gEffectsGlobals.gidColorizeAlphaProgram, GL_VALIDATE_STATUS, &result);
		BAIL_IF_ZERO(result, err, kFskErrGLShader);
	#endif /* CHECK_GL_ERROR */

	/* Initialize Monochrome Straight shader, program and attributes */
	BAIL_IF_ERR(err = FskGLNewShader(gMonochromeStraightFragmentShader, GL_FRAGMENT_SHADER, &fragmentShader));
	BAIL_IF_ERR(err = FskGLNewProgram(vertexShader, fragmentShader, &gEffectsGlobals.gidMonochromeStraightProgram,
					 VERTEX_POSITION_INDEX, vPosition_str,
					 VERTEX_TEXCOORD_INDEX, vTexCoord_str,
					 VERTEX_END_INDEX));
	glDeleteShader(fragmentShader);
	BAIL_IF_ZERO(gEffectsGlobals.gidMonochromeStraightProgram, err, kFskErrGLShader);
	BAIL_IF_NEGATIVE(gEffectsGlobals.gidMonochromeStraightMatrix = glGetUniformLocation(gEffectsGlobals.gidMonochromeStraightProgram, matrix_str),	err, kFskErrGLShader);
	BAIL_IF_NEGATIVE(gEffectsGlobals.gidMonochromeStraightSrcBM  = glGetUniformLocation(gEffectsGlobals.gidMonochromeStraightProgram, srcBM_str),	err, kFskErrGLShader);
	BAIL_IF_NEGATIVE(gEffectsGlobals.gidMonochromeStraightColor0 = glGetUniformLocation(gEffectsGlobals.gidMonochromeStraightProgram, color0_str),	err, kFskErrGLShader);
	BAIL_IF_NEGATIVE(gEffectsGlobals.gidMonochromeStraightColor1 = glGetUniformLocation(gEffectsGlobals.gidMonochromeStraightProgram, color1_str),	err, kFskErrGLShader);
	BAIL_IF_ERR(err = FskGLUseProgram(gEffectsGlobals.gidMonochromeStraightProgram));
	glUniform1i(gEffectsGlobals.gidMonochromeStraightSrcBM, 0);										/* srcBM */
	glUniformMatrix3fv(gEffectsGlobals.gidMonochromeStraightMatrix, 1, GL_FALSE, identity);			/* matrix */
	glUniform4fv(gEffectsGlobals.gidMonochromeStraightColor0, 1, grayColor);						/* color0 */
	glUniform4fv(gEffectsGlobals.gidMonochromeStraightColor1, 1, grayColor);						/* color1 */
	#if CHECK_GL_ERROR
		glValidateProgram(gEffectsGlobals.gidMonochromeStraightProgram);
		glGetProgramiv(gEffectsGlobals.gidMonochromeStraightProgram, GL_VALIDATE_STATUS, &result);
		BAIL_IF_ZERO(result, err, kFskErrGLShader);
	#endif /* CHECK_GL_ERROR */

	/* Initialize Monochrome Premul shader, program and attributes */
	BAIL_IF_ERR(err = FskGLNewShader(gMonochromePremulFragmentShader, GL_FRAGMENT_SHADER, &fragmentShader));
	BAIL_IF_ERR(err = FskGLNewProgram(vertexShader, fragmentShader, &gEffectsGlobals.gidMonochromePremulProgram,
					 VERTEX_POSITION_INDEX, vPosition_str,
					 VERTEX_TEXCOORD_INDEX, vTexCoord_str,
					 VERTEX_END_INDEX));
	glDeleteShader(fragmentShader);
	BAIL_IF_ZERO(gEffectsGlobals.gidMonochromePremulProgram, err, kFskErrGLShader);
	BAIL_IF_NEGATIVE(gEffectsGlobals.gidMonochromePremulMatrix = glGetUniformLocation(gEffectsGlobals.gidMonochromePremulProgram, matrix_str),	err, kFskErrGLShader);
	BAIL_IF_NEGATIVE(gEffectsGlobals.gidMonochromePremulSrcBM  = glGetUniformLocation(gEffectsGlobals.gidMonochromePremulProgram, srcBM_str),	err, kFskErrGLShader);
	BAIL_IF_NEGATIVE(gEffectsGlobals.gidMonochromePremulColor0 = glGetUniformLocation(gEffectsGlobals.gidMonochromePremulProgram, color0_str),	err, kFskErrGLShader);
	BAIL_IF_NEGATIVE(gEffectsGlobals.gidMonochromePremulColor1 = glGetUniformLocation(gEffectsGlobals.gidMonochromePremulProgram, color1_str),	err, kFskErrGLShader);
	BAIL_IF_ERR(err = FskGLUseProgram(gEffectsGlobals.gidMonochromePremulProgram));
	glUniform1i(gEffectsGlobals.gidMonochromePremulSrcBM, 0);										/* srcBM */
	glUniformMatrix3fv(gEffectsGlobals.gidMonochromePremulMatrix, 1, GL_FALSE, identity);			/* matrix */
	glUniform4fv(gEffectsGlobals.gidMonochromePremulColor0, 1, grayColor);							/* color0 */
	glUniform4fv(gEffectsGlobals.gidMonochromePremulColor1, 1, grayColor);							/* color1 */
	#if CHECK_GL_ERROR
		glValidateProgram(gEffectsGlobals.gidMonochromePremulProgram);
		glGetProgramiv(gEffectsGlobals.gidMonochromePremulProgram, GL_VALIDATE_STATUS, &result);
		BAIL_IF_ZERO(result, err, kFskErrGLShader);
	#endif /* CHECK_GL_ERROR */

	BAIL_IF_ERR(err = FskGLNewShader(gColorizePremulFragmentShader, GL_FRAGMENT_SHADER, &fragmentShader));
	BAIL_IF_ERR(err = FskGLNewProgram(vertexShader, fragmentShader, &gEffectsGlobals.gidColorizePremulProgram,
					 VERTEX_POSITION_INDEX, vPosition_str,
					 VERTEX_TEXCOORD_INDEX, vTexCoord_str,
					 VERTEX_END_INDEX));
	glDeleteShader(fragmentShader);
	BAIL_IF_ZERO(gEffectsGlobals.gidColorizePremulProgram, err, kFskErrGLShader);
	BAIL_IF_NEGATIVE(gEffectsGlobals.gidColorizePremulMatrix = glGetUniformLocation(gEffectsGlobals.gidColorizePremulProgram, matrix_str), err, kFskErrGLShader);
	BAIL_IF_NEGATIVE(gEffectsGlobals.gidColorizePremulSrcBM  = glGetUniformLocation(gEffectsGlobals.gidColorizePremulProgram, srcBM_str),  err, kFskErrGLShader);
	BAIL_IF_NEGATIVE(gEffectsGlobals.gidColorizePremulColor  = glGetUniformLocation(gEffectsGlobals.gidColorizePremulProgram, color_str),  err, kFskErrGLShader);
	BAIL_IF_ERR(err = FskGLUseProgram(gEffectsGlobals.gidColorizePremulProgram));
	glUniform1i(gEffectsGlobals.gidColorizePremulSrcBM, 0);											/* srcBM is in texture #0 */
	glUniformMatrix3fv(gEffectsGlobals.gidColorizePremulMatrix, 1, GL_FALSE, identity);				/* matrix */
	glUniform4fv(gEffectsGlobals.gidColorizePremulColor, 1, grayColor);								/* color */
	#if CHECK_GL_ERROR
		glValidateProgram(gEffectsGlobals.gidColorizePremulProgram);
		glGetProgramiv(gEffectsGlobals.gidColorizePremulProgram, GL_VALIDATE_STATUS, &result);
		BAIL_IF_ZERO(result, err, kFskErrGLShader);
	#endif /* CHECK_GL_ERROR */

	BAIL_IF_ERR(err = FskGLNewShader(gColorizeStraightFragmentShader, GL_FRAGMENT_SHADER, &fragmentShader));
	BAIL_IF_ERR(err = FskGLNewProgram(vertexShader, fragmentShader, &gEffectsGlobals.gidColorizeStraightProgram,
					 VERTEX_POSITION_INDEX, vPosition_str,
					 VERTEX_TEXCOORD_INDEX, vTexCoord_str,
					 VERTEX_END_INDEX));
	glDeleteShader(fragmentShader);
	BAIL_IF_ZERO(gEffectsGlobals.gidColorizeStraightProgram, err, kFskErrGLShader);
	BAIL_IF_NEGATIVE(gEffectsGlobals.gidColorizeStraightMatrix = glGetUniformLocation(gEffectsGlobals.gidColorizeStraightProgram, matrix_str), err, kFskErrGLShader);
	BAIL_IF_NEGATIVE(gEffectsGlobals.gidColorizeStraightSrcBM  = glGetUniformLocation(gEffectsGlobals.gidColorizeStraightProgram, srcBM_str),  err, kFskErrGLShader);
	BAIL_IF_NEGATIVE(gEffectsGlobals.gidColorizeStraightColor  = glGetUniformLocation(gEffectsGlobals.gidColorizeStraightProgram, color_str),  err, kFskErrGLShader);
	BAIL_IF_ERR(err = FskGLUseProgram(gEffectsGlobals.gidColorizeStraightProgram));
	glUniform1i(gEffectsGlobals.gidColorizeStraightSrcBM, 0);										/* srcBM is in texture #0 */
	glUniformMatrix3fv(gEffectsGlobals.gidColorizeStraightMatrix, 1, GL_FALSE, identity);			/* matrix */
	glUniform4fv(gEffectsGlobals.gidColorizeStraightColor, 1, grayColor);							/* color */
	#if CHECK_GL_ERROR
		glValidateProgram(gEffectsGlobals.gidColorizeStraightProgram);
		glGetProgramiv(gEffectsGlobals.gidColorizeStraightProgram, GL_VALIDATE_STATUS, &result);
		BAIL_IF_ZERO(result, err, kFskErrGLShader);
	#endif /* CHECK_GL_ERROR */

	BAIL_IF_ERR(err = FskGLNewShader(gGelFragmentShader, GL_FRAGMENT_SHADER, &fragmentShader));
	BAIL_IF_ERR(err = FskGLNewProgram(vertexShader, fragmentShader, &gEffectsGlobals.gidGelProgram,
					 VERTEX_POSITION_INDEX, vPosition_str,
					 VERTEX_TEXCOORD_INDEX, vTexCoord_str,
					 VERTEX_END_INDEX));
	glDeleteShader(fragmentShader);
	BAIL_IF_ZERO(gEffectsGlobals.gidGelProgram, err, kFskErrGLShader);
	BAIL_IF_NEGATIVE(gEffectsGlobals.gidGelMatrix = glGetUniformLocation(gEffectsGlobals.gidGelProgram, matrix_str), err, kFskErrGLShader);
	BAIL_IF_NEGATIVE(gEffectsGlobals.gidGelSrcBM  = glGetUniformLocation(gEffectsGlobals.gidGelProgram, srcBM_str),  err, kFskErrGLShader);
	BAIL_IF_NEGATIVE(gEffectsGlobals.gidGelColor  = glGetUniformLocation(gEffectsGlobals.gidGelProgram, color_str),  err, kFskErrGLShader);
	BAIL_IF_ERR(err = FskGLUseProgram(gEffectsGlobals.gidGelProgram));
	glUniform1i(gEffectsGlobals.gidGelSrcBM, 0);									/* srcBM is in texture #0 */
	glUniformMatrix3fv(gEffectsGlobals.gidGelMatrix, 1, GL_FALSE, identity);		/* matrix */
	glUniform4fv(gEffectsGlobals.gidGelColor, 1, grayColor);						/* color */
	#if CHECK_GL_ERROR
		glValidateProgram(gEffectsGlobals.gidGelProgram);
		glGetProgramiv(gEffectsGlobals.gidGelProgram, GL_VALIDATE_STATUS, &result);
		BAIL_IF_ZERO(result, err, kFskErrGLShader);
	#endif /* CHECK_GL_ERROR */

	BAIL_IF_ERR(err = FskGLNewShader(gStraightToPremultipliedFragmentShader, GL_FRAGMENT_SHADER, &fragmentShader));
	BAIL_IF_ERR(err = FskGLNewProgram(vertexShader, fragmentShader, &gEffectsGlobals.gidStraightToPremultipliedProgram,
					 VERTEX_POSITION_INDEX, vPosition_str,
					 VERTEX_TEXCOORD_INDEX, vTexCoord_str,
					 VERTEX_END_INDEX));
	glDeleteShader(fragmentShader);
	BAIL_IF_ZERO(gEffectsGlobals.gidStraightToPremultipliedProgram, err, kFskErrGLShader);
	BAIL_IF_NEGATIVE(gEffectsGlobals.gidStraightToPremultipliedMatrix = glGetUniformLocation(gEffectsGlobals.gidStraightToPremultipliedProgram, matrix_str), err, kFskErrGLShader);
	BAIL_IF_NEGATIVE(gEffectsGlobals.gidStraightToPremultipliedSrcBM  = glGetUniformLocation(gEffectsGlobals.gidStraightToPremultipliedProgram, srcBM_str),  err, kFskErrGLShader);
	BAIL_IF_ERR(err = FskGLUseProgram(gEffectsGlobals.gidStraightToPremultipliedProgram));
	glUniform1i(gEffectsGlobals.gidStraightToPremultipliedSrcBM, 0);									/* srcBM is in texture #0 */
	glUniformMatrix3fv(gEffectsGlobals.gidStraightToPremultipliedMatrix, 1, GL_FALSE, identity);		/* matrix */
	#if CHECK_GL_ERROR
		glValidateProgram(gEffectsGlobals.gidStraightToPremultipliedProgram);
		glGetProgramiv(gEffectsGlobals.gidStraightToPremultipliedProgram, GL_VALIDATE_STATUS, &result);
		BAIL_IF_ZERO(result, err, kFskErrGLShader);
	#endif /* CHECK_GL_ERROR */

	BAIL_IF_ERR(err = FskGLNewShader(gPremultipliedToStraightFragmentShader, GL_FRAGMENT_SHADER, &fragmentShader));
	BAIL_IF_ERR(err = FskGLNewProgram(vertexShader, fragmentShader, &gEffectsGlobals.gidPremultipliedToStraightProgram,
					 VERTEX_POSITION_INDEX, vPosition_str,
					 VERTEX_TEXCOORD_INDEX, vTexCoord_str,
					 VERTEX_END_INDEX));
	glDeleteShader(fragmentShader);
	BAIL_IF_ZERO(gEffectsGlobals.gidPremultipliedToStraightProgram, err, kFskErrGLShader);
	BAIL_IF_NEGATIVE(gEffectsGlobals.gidPremultipliedToStraightMatrix = glGetUniformLocation(gEffectsGlobals.gidPremultipliedToStraightProgram, matrix_str), err, kFskErrGLShader);
	BAIL_IF_NEGATIVE(gEffectsGlobals.gidPremultipliedToStraightSrcBM  = glGetUniformLocation(gEffectsGlobals.gidPremultipliedToStraightProgram, srcBM_str),  err, kFskErrGLShader);
	BAIL_IF_NEGATIVE(gEffectsGlobals.gidPremultipliedToStraightBgColor= glGetUniformLocation(gEffectsGlobals.gidPremultipliedToStraightProgram, color_str),  err, kFskErrGLShader);
	BAIL_IF_ERR(err = FskGLUseProgram(gEffectsGlobals.gidPremultipliedToStraightProgram));
	glUniform1i(gEffectsGlobals.gidPremultipliedToStraightSrcBM, 0);									/* srcBM is in texture #0 */
	glUniformMatrix3fv(gEffectsGlobals.gidPremultipliedToStraightMatrix, 1, GL_FALSE, identity);		/* matrix */
	glUniform3fv(gEffectsGlobals.gidPremultipliedToStraightBgColor, 1, grayColor);						/* color */
	#if CHECK_GL_ERROR
		glValidateProgram(gEffectsGlobals.gidPremultipliedToStraightProgram);
		glGetProgramiv(gEffectsGlobals.gidPremultipliedToStraightProgram, GL_VALIDATE_STATUS, &result);
		BAIL_IF_ZERO(result, err, kFskErrGLShader);
	#endif /* CHECK_GL_ERROR */

	glDeleteShader(vertexShader);


	/*
	 * Initialize 2 source programs
	 */

	BAIL_IF_ERR(err = FskGLNewShader(gMatteVertexShader,   GL_VERTEX_SHADER, &vertexShader));

	BAIL_IF_ERR(err = FskGLNewShader(gColorizeInnerStraightFragmentShader, GL_FRAGMENT_SHADER, &fragmentShader));
	BAIL_IF_ERR(err = FskGLNewProgram(vertexShader, fragmentShader, &gEffectsGlobals.gidColorizeInnerStraightProgram,
					 VERTEX_POSITION_INDEX, vPosition_str,
					 VERTEX_TEXCOORD_INDEX, vTexCoord_str,
					 VERTEX_END_INDEX));
	glDeleteShader(fragmentShader);
	BAIL_IF_ZERO(gEffectsGlobals.gidColorizeInnerStraightProgram, err, kFskErrGLShader);
	BAIL_IF_NEGATIVE(gEffectsGlobals.gidColorizeInnerStraightMatrix = glGetUniformLocation(gEffectsGlobals.gidColorizeInnerStraightProgram, matrix_str), err, kFskErrGLShader);
	BAIL_IF_NEGATIVE(gEffectsGlobals.gidColorizeInnerStraightSrcBM  = glGetUniformLocation(gEffectsGlobals.gidColorizeInnerStraightProgram, srcBM_str),  err, kFskErrGLShader);
	BAIL_IF_NEGATIVE(gEffectsGlobals.gidColorizeInnerStraightMttBM  = glGetUniformLocation(gEffectsGlobals.gidColorizeInnerStraightProgram, mttBM_str),  err, kFskErrGLShader);
	BAIL_IF_NEGATIVE(gEffectsGlobals.gidColorizeInnerStraightColor  = glGetUniformLocation(gEffectsGlobals.gidColorizeInnerStraightProgram, color_str),  err, kFskErrGLShader);
	BAIL_IF_ERR(err = FskGLUseProgram(gEffectsGlobals.gidColorizeInnerStraightProgram));
	glUniform1i(gEffectsGlobals.gidColorizeInnerStraightSrcBM, 0);									/* srcBM is in texture #0 */
	glUniform1i(gEffectsGlobals.gidColorizeInnerStraightMttBM, 1);									/* srcBM is in texture #1 */
	glUniformMatrix3fv(gEffectsGlobals.gidColorizeInnerStraightMatrix, 1, GL_FALSE, identity);		/* matrix */
	glUniform4fv(gEffectsGlobals.gidColorizeInnerStraightColor, 1, grayColor);						/* color */
	#if CHECK_GL_ERROR
		glValidateProgram(gEffectsGlobals.gidColorizeInnerStraightProgram);
		glGetProgramiv(gEffectsGlobals.gidColorizeInnerStraightProgram, GL_VALIDATE_STATUS, &result);
		BAIL_IF_ZERO(result, err, kFskErrGLShader);
	#endif /* CHECK_GL_ERROR */

	BAIL_IF_ERR(err = FskGLNewShader(gColorizeInnerPremulFragmentShader,   GL_FRAGMENT_SHADER, &fragmentShader));
	BAIL_IF_ERR(err = FskGLNewProgram(vertexShader, fragmentShader, &gEffectsGlobals.gidColorizeInnerPremulProgram,
					 VERTEX_POSITION_INDEX, vPosition_str,
					 VERTEX_TEXCOORD_INDEX, vTexCoord_str,
					 VERTEX_END_INDEX));
	glDeleteShader(fragmentShader);
	BAIL_IF_ZERO(gEffectsGlobals.gidColorizeInnerPremulProgram, err, kFskErrGLShader);
	BAIL_IF_NEGATIVE(gEffectsGlobals.gidColorizeInnerPremulMatrix = glGetUniformLocation(gEffectsGlobals.gidColorizeInnerPremulProgram, matrix_str), err, kFskErrGLShader);
	BAIL_IF_NEGATIVE(gEffectsGlobals.gidColorizeInnerPremulSrcBM  = glGetUniformLocation(gEffectsGlobals.gidColorizeInnerPremulProgram, srcBM_str),  err, kFskErrGLShader);
	BAIL_IF_NEGATIVE(gEffectsGlobals.gidColorizeInnerPremulMttBM  = glGetUniformLocation(gEffectsGlobals.gidColorizeInnerPremulProgram, mttBM_str),  err, kFskErrGLShader);
	BAIL_IF_NEGATIVE(gEffectsGlobals.gidColorizeInnerPremulColor  = glGetUniformLocation(gEffectsGlobals.gidColorizeInnerPremulProgram, color_str),  err, kFskErrGLShader);
	BAIL_IF_ERR(err = FskGLUseProgram(gEffectsGlobals.gidColorizeInnerPremulProgram));
	glUniform1i(gEffectsGlobals.gidColorizeInnerPremulSrcBM, 0);									/* srcBM is in texture #0 */
	glUniform1i(gEffectsGlobals.gidColorizeInnerPremulMttBM, 1);									/* srcBM is in texture #1 */
	glUniformMatrix3fv(gEffectsGlobals.gidColorizeInnerPremulMatrix, 1, GL_FALSE, identity);		/* matrix */
	glUniform4fv(gEffectsGlobals.gidColorizeInnerPremulColor, 1, grayColor);						/* color */
	#if CHECK_GL_ERROR
		glValidateProgram(gEffectsGlobals.gidColorizeInnerPremulProgram);
		glGetProgramiv(gEffectsGlobals.gidColorizeInnerPremulProgram, GL_VALIDATE_STATUS, &result);
		BAIL_IF_ZERO(result, err, kFskErrGLShader);
	#endif /* CHECK_GL_ERROR */

	BAIL_IF_ERR(err = FskGLNewShader(gColorizeOuterStraightFragmentShader, GL_FRAGMENT_SHADER, &fragmentShader));
	BAIL_IF_ERR(err = FskGLNewProgram(vertexShader, fragmentShader, &gEffectsGlobals.gidColorizeOuterStraightProgram,
					 VERTEX_POSITION_INDEX, vPosition_str,
					 VERTEX_TEXCOORD_INDEX, vTexCoord_str,
					 VERTEX_END_INDEX));
	glDeleteShader(fragmentShader);
	BAIL_IF_ZERO(gEffectsGlobals.gidColorizeOuterStraightProgram, err, kFskErrGLShader);
	BAIL_IF_NEGATIVE(gEffectsGlobals.gidColorizeOuterStraightMatrix = glGetUniformLocation(gEffectsGlobals.gidColorizeOuterStraightProgram, matrix_str), err, kFskErrGLShader);
	BAIL_IF_NEGATIVE(gEffectsGlobals.gidColorizeOuterStraightSrcBM  = glGetUniformLocation(gEffectsGlobals.gidColorizeOuterStraightProgram, srcBM_str),  err, kFskErrGLShader);
	BAIL_IF_NEGATIVE(gEffectsGlobals.gidColorizeOuterStraightMttBM  = glGetUniformLocation(gEffectsGlobals.gidColorizeOuterStraightProgram, mttBM_str),  err, kFskErrGLShader);
	BAIL_IF_NEGATIVE(gEffectsGlobals.gidColorizeOuterStraightColor  = glGetUniformLocation(gEffectsGlobals.gidColorizeOuterStraightProgram, color_str),  err, kFskErrGLShader);
	BAIL_IF_ERR(err = FskGLUseProgram(gEffectsGlobals.gidColorizeOuterStraightProgram));
	glUniform1i(gEffectsGlobals.gidColorizeOuterStraightSrcBM, 0);									/* srcBM is in texture #0 */
	glUniform1i(gEffectsGlobals.gidColorizeOuterStraightMttBM, 1);									/* srcBM is in texture #1 */
	glUniformMatrix3fv(gEffectsGlobals.gidColorizeOuterStraightMatrix, 1, GL_FALSE, identity);		/* matrix */
	glUniform4fv(gEffectsGlobals.gidColorizeOuterStraightColor, 1, grayColor);						/* color */
	#if CHECK_GL_ERROR
		glValidateProgram(gEffectsGlobals.gidColorizeOuterStraightProgram);
		glGetProgramiv(gEffectsGlobals.gidColorizeOuterStraightProgram, GL_VALIDATE_STATUS, &result);
		BAIL_IF_ZERO(result, err, kFskErrGLShader);
	#endif /* CHECK_GL_ERROR */

	BAIL_IF_ERR(err = FskGLNewShader(gColorizeOuterPremulFragmentShader,   GL_FRAGMENT_SHADER, &fragmentShader));
	BAIL_IF_ERR(err = FskGLNewProgram(vertexShader, fragmentShader, &gEffectsGlobals.gidColorizeOuterPremulProgram,
					 VERTEX_POSITION_INDEX, vPosition_str,
					 VERTEX_TEXCOORD_INDEX, vTexCoord_str,
					 VERTEX_END_INDEX));
	glDeleteShader(fragmentShader);
	BAIL_IF_ZERO(gEffectsGlobals.gidColorizeOuterPremulProgram, err, kFskErrGLShader);
	BAIL_IF_NEGATIVE(gEffectsGlobals.gidColorizeOuterPremulMatrix = glGetUniformLocation(gEffectsGlobals.gidColorizeOuterPremulProgram, matrix_str), err, kFskErrGLShader);
	BAIL_IF_NEGATIVE(gEffectsGlobals.gidColorizeOuterPremulSrcBM  = glGetUniformLocation(gEffectsGlobals.gidColorizeOuterPremulProgram, srcBM_str),  err, kFskErrGLShader);
	BAIL_IF_NEGATIVE(gEffectsGlobals.gidColorizeOuterPremulMttBM  = glGetUniformLocation(gEffectsGlobals.gidColorizeOuterPremulProgram, mttBM_str),  err, kFskErrGLShader);
	BAIL_IF_NEGATIVE(gEffectsGlobals.gidColorizeOuterPremulColor  = glGetUniformLocation(gEffectsGlobals.gidColorizeOuterPremulProgram, color_str),  err, kFskErrGLShader);
	BAIL_IF_ERR(err = FskGLUseProgram(gEffectsGlobals.gidColorizeOuterPremulProgram));
	glUniform1i(gEffectsGlobals.gidColorizeOuterPremulSrcBM, 0);									/* srcBM is in texture #0 */
	glUniform1i(gEffectsGlobals.gidColorizeOuterPremulMttBM, 1);									/* srcBM is in texture #1 */
	glUniformMatrix3fv(gEffectsGlobals.gidColorizeOuterPremulMatrix, 1, GL_FALSE, identity);		/* matrix */
	glUniform4fv(gEffectsGlobals.gidColorizeOuterPremulColor, 1, grayColor);						/* color */
	#if CHECK_GL_ERROR
		glValidateProgram(gEffectsGlobals.gidColorizeOuterPremulProgram);
		glGetProgramiv(gEffectsGlobals.gidColorizeOuterPremulProgram, GL_VALIDATE_STATUS, &result);
		BAIL_IF_ZERO(result, err, kFskErrGLShader);
	#endif /* CHECK_GL_ERROR */


	glDeleteShader(vertexShader);

	(void)FskGLBindBMTexture(0, 0, 0);
	(void)FskGLUseProgram(0);
	glActiveTexture(GL_TEXTURE0);

bail:

	#if CHECK_GL_ERROR
		if (err) LOGE("Error %d initializing effects", (int)err);
	#endif /* CHECK_GL_ERROR */

	return err;
}


/********************************************************************************
 * FskGLEffectsShutdown
 ********************************************************************************/

void FskGLEffectsShutdown(void) {

	#if defined(LOG_PARAMETERS)
		LOGD("FskGLEffectsShutdown");
	#endif /* LOG_PARAMETERS */

	if (gEffectsGlobals.gidGelProgram)							glDeleteProgram(gEffectsGlobals.gidGelProgram);
	if (gEffectsGlobals.gidColorizePremulProgram)				glDeleteProgram(gEffectsGlobals.gidColorizePremulProgram);
	if (gEffectsGlobals.gidColorizeStraightProgram)				glDeleteProgram(gEffectsGlobals.gidColorizeStraightProgram);
	if (gEffectsGlobals.gidColorizeOuterPremulProgram)			glDeleteProgram(gEffectsGlobals.gidColorizeOuterPremulProgram);
	if (gEffectsGlobals.gidColorizeOuterStraightProgram)		glDeleteProgram(gEffectsGlobals.gidColorizeOuterStraightProgram);
	if (gEffectsGlobals.gidColorizeInnerPremulProgram)			glDeleteProgram(gEffectsGlobals.gidColorizeInnerPremulProgram);
	if (gEffectsGlobals.gidColorizeInnerStraightProgram)		glDeleteProgram(gEffectsGlobals.gidColorizeInnerStraightProgram);

	if (gEffectsGlobals.gidMonochromePremulProgram)				glDeleteProgram(gEffectsGlobals.gidMonochromePremulProgram);
	if (gEffectsGlobals.gidMonochromeStraightProgram)			glDeleteProgram(gEffectsGlobals.gidMonochromeStraightProgram);
	if (gEffectsGlobals.gidColorizeAlphaProgram)				glDeleteProgram(gEffectsGlobals.gidColorizeAlphaProgram);
	if (gEffectsGlobals.gidErodeProgram)						glDeleteProgram(gEffectsGlobals.gidErodeProgram);
	if (gEffectsGlobals.gidDilateProgram)						glDeleteProgram(gEffectsGlobals.gidDilateProgram);
	if (gEffectsGlobals.gidGaussianBlurProgram)					glDeleteProgram(gEffectsGlobals.gidGaussianBlurProgram);
	if (gEffectsGlobals.gidBoxBlurProgram)						glDeleteProgram(gEffectsGlobals.gidBoxBlurProgram);
	if (gEffectsGlobals.gidCopyProgram)							glDeleteProgram(gEffectsGlobals.gidCopyProgram);

	FskMemSet(&gEffectsGlobals, 0, sizeof(gEffectsGlobals));
}

#endif /* FSKBITMAP_OPENGL */
