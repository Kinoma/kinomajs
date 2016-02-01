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
#ifndef __FSKPORT__
#define __FSKPORT__

#include "FskBitmap.h"
#include "FskFixedMath.h"
#include "FskGraphics.h"
#include "FskText.h"

#ifdef __cplusplus
extern "C" {
#endif

FskDeclarePrivateType(FskPort)
FskDeclarePrivateType(FskPortTextFormat)

struct FskScaleOffset;
struct FskEffectRecord;


typedef FskErr (*FskPortPicPrepareItem)(FskPort port, void *params, UInt32 paramsSize);
typedef FskErr (*FskPortPicRenderItem)(FskPort port, void *params, UInt32 paramsSize);

typedef FskErr (*vFskPortBeginDrawing)(FskPort port, FskConstColorRGBA background);
typedef FskErr (*vFskPortEndDrawing)(FskPort port);
typedef void (*vFskPortRectangleFill)(FskPort port, FskConstRectangle rect);
typedef void (*vFskPortRectangleFrame)(FskPort port, FskConstRectangle rect);
typedef void (*vFskPortBitmapDraw)(FskPort port, FskBitmap bits, FskConstRectangle srcRect, FskConstRectangle dstRect);
typedef void (*vFskPortBitmapScaleOffset)(FskPort port, FskBitmap bits, FskConstRectangle srcRect, const struct FskScaleOffset *scaleOffset);
typedef void (*vFskPortBitmapDrawSubpixel)(FskPort port, FskBitmap bits, FskConstRectangle srcRect, double x, double y, double width, double height);
typedef void (*vFskPortBitmapTile)(FskPort port, FskBitmap srcBits, FskConstRectangle srcRect, FskConstRectangle dstRect, FskFixed scale);
typedef void (*vFskPortTextDraw)(FskPort port, const char *text, UInt32 textLen, FskConstRectangle bounds);
typedef void (*vFskPortTextDrawSubpixel)(FskPort port, const char *text, UInt32 textLen, double x, double y, double width, double height);
typedef FskErr (*vFskPortPicSaveAdd)(FskPort port, FskPortPicRenderItem draw, FskPortPicPrepareItem prepare, void *params, UInt32 paramsSize);
typedef void (*vFskPortEffectApply)(FskPort port, FskBitmap src, FskConstRectangle srcRect, FskConstPoint dstPoint, const struct FskEffectRecord* effect);
typedef FskErr (*vFskPortSetOrigin)(FskPort port, SInt32 x, SInt32 y);
typedef FskErr (*vFskPortSetClipRectangle)(FskPort port, FskConstRectangle clip);
typedef FskErr (*vFskPortSetPenColor)(FskPort port, FskConstColorRGBA color);
typedef FskErr (*vFskPortTextFormatApply)(FskPort port, FskPortTextFormat textFormat);
typedef FskErr (*vFskPortSetTextAlignment)(FskPort port, UInt16 hAlign, UInt16 vAlign);
typedef FskErr (*vFskPortSetTextFont)(FskPort port, const char *fontName);
typedef FskErr (*vFskPortSetTextSize)(FskPort port, UInt32 size);
typedef FskErr (*vFskPortSetTextStyle)(FskPort port, UInt32 style);
typedef FskErr (*vFskPortSetTextExtra)(FskPort port, FskFixed extra);
typedef FskErr (*vFskPortSetGraphicsMode)(FskPort port, UInt32 graphicsMode, FskConstGraphicsModeParameters graphicsModeParameters);
typedef void (*vFskPortScaleSet)(FskPort port, FskFixed scale);
typedef FskErr (*vFskPortGetBitmap)(FskPort port, FskBitmap *bits);

struct FskPortVectorsRecord {
    vFskPortBeginDrawing        doBeginDrawing;
    vFskPortEndDrawing          doEndDrawing;
    vFskPortRectangleFill       doRectangleFill;
    vFskPortRectangleFrame      doRectangleFrame;
    vFskPortBitmapDraw          doBitmapDraw;
    vFskPortBitmapScaleOffset   doBitmapScaleOffset;
    vFskPortBitmapDrawSubpixel  doBitmapDrawSubpixel;
    vFskPortBitmapTile          doBitmapTile;
    vFskPortTextDraw            doTextDraw;
    vFskPortTextDrawSubpixel	doTextDrawSubpixel;
    vFskPortPicSaveAdd          doPicSaveAdd;
    vFskPortEffectApply         doEffectApply;
    vFskPortSetOrigin           doSetOrigin;
    vFskPortSetClipRectangle    doSetClipRectangle;
    vFskPortSetPenColor         doSetPenColor;
    vFskPortTextFormatApply     doTextFormatApply;
    vFskPortSetTextAlignment    doSetTextAlignment;
    vFskPortSetTextFont         doSetTextFont;
    vFskPortSetTextSize         doSetTextSize;
    vFskPortSetTextStyle        doSetTextStyle;
    vFskPortSetTextExtra        doSetTextExtra;
    vFskPortSetGraphicsMode     doSetGraphicsMode;
    vFskPortScaleSet            doScaleSet;
    vFskPortGetBitmap           doGetBitmap;
};

typedef struct FskPortVectorsRecord FskPortVectorsRecord;
typedef struct FskPortVectorsRecord *FskPortVectors;

#define FSKPORT_DECLARE_VECTOR_DRAW_FUNCTIONS(prefix) \
    static FskErr prefix##BeginDrawing(FskPort port, FskConstColorRGBA background);    \
    static FskErr prefix##EndDrawing(FskPort port);    \
    static void prefix##RectangleFill(FskPort port, FskConstRectangle rect);    \
    static void prefix##RectangleFrame(FskPort port, FskConstRectangle rect);    \
    static void prefix##BitmapDraw(FskPort port, FskBitmap bits, FskConstRectangle srcRect, FskConstRectangle dstRect);    \
    static void prefix##BitmapScaleOffset(FskPort port, FskBitmap bits, FskConstRectangle srcRect, const struct FskScaleOffset *scaleOffset);    \
    static void prefix##BitmapDrawSubpixel(FskPort port, FskBitmap bits, FskConstRectangle srcRect, double x, double y, double width, double height);    \
    static void prefix##BitmapTile(FskPort port, FskBitmap srcBits, FskConstRectangle srcRect, FskConstRectangle dstRect, FskFixed scale);    \
    static void prefix##TextDraw(FskPort port, const char *text, UInt32 textLen, FskConstRectangle bounds);    \
    static void prefix##TextDrawSubpixel(FskPort port, const char *text, UInt32 textLen, double x, double y, double width, double height);    \
    static FskErr prefix##PicSaveAdd(FskPort port, FskPortPicRenderItem draw, FskPortPicPrepareItem prepare, void *params, UInt32 paramsSize);    \
    static void prefix##EffectApply(FskPort port, FskBitmap src, FskConstRectangle srcRect, FskConstPoint dstPoint, const struct FskEffectRecord* effect);

#define FSKPORT_DECLARE_VECTOR_STATE_FUNCTIONS(prefix) \
    static FskErr prefix##SetOrigin(FskPort port, SInt32 x, SInt32 y);    \
    static FskErr prefix##SetClipRectangle(FskPort port, FskConstRectangle clip);    \
    static FskErr prefix##SetPenColor(FskPort port, FskConstColorRGBA color);    \
    static FskErr prefix##TextFormatApply(FskPort port, FskPortTextFormat textFormat);    \
    static FskErr prefix##SetTextAlignment(FskPort port, UInt16 hAlign, UInt16 vAlign);    \
    static FskErr prefix##SetTextFont(FskPort port, const char *textFont);    \
    static FskErr prefix##SetTextSize(FskPort port, UInt32 size);    \
    static FskErr prefix##SetTextStyle(FskPort port, UInt32 style);    \
    static FskErr prefix##SetTextExtra(FskPort port, FskFixed extra);    \
    static FskErr prefix##SetGraphicsMode(FskPort port, UInt32 graphicsMode, FskConstGraphicsModeParameters graphicsModeParameters);    \
    static void prefix##ScaleSet(FskPort port, FskFixed scale); \
    static FskErr prefix##GetBitmap(FskPort port, FskBitmap *bits);

#define FSKPORT_DECLARE_VECTOR_FUNCTIONS(prefix) \
    FSKPORT_DECLARE_VECTOR_STATE_FUNCTIONS(prefix) \
    FSKPORT_DECLARE_VECTOR_DRAW_FUNCTIONS(prefix)

#define FSKPORT_DECLARE_VECTOR_RECORD(prefix) \
     {  \
        prefix##BeginDrawing,  \
        prefix##EndDrawing,  \
        prefix##RectangleFill,  \
        prefix##RectangleFrame,  \
        prefix##BitmapDraw,  \
        prefix##BitmapScaleOffset,  \
        prefix##BitmapDrawSubpixel,  \
        prefix##BitmapTile,  \
        prefix##TextDraw,  \
        prefix##TextDrawSubpixel,  \
        prefix##PicSaveAdd,  \
        prefix##EffectApply,  \
        prefix##SetOrigin,  \
        prefix##SetClipRectangle,  \
        prefix##SetPenColor,  \
        prefix##TextFormatApply,  \
        prefix##SetTextAlignment,  \
        prefix##SetTextFont,  \
        prefix##SetTextSize,  \
        prefix##SetTextStyle,  \
        prefix##SetTextExtra,  \
        prefix##SetGraphicsMode,  \
        prefix##ScaleSet,  \
        prefix##GetBitmap  \
    }

#if defined(__FSKPORT_PRIV__) || SUPPORT_INSTRUMENTATION
	#include "FskEffects.h"
	#include "FskText.h"

	typedef FskErr (*FskBitmapDrawProc)(FskConstBitmap src, FskConstRectangle srcRect, FskBitmap dst, FskConstRectangle dstRect, FskConstRectangle dstClip, FskConstColorRGBA opColor, UInt32 mode, FskConstGraphicsModeParameters modeParams);
	typedef void (*FskPortScaleRectProc)(FskPort port, FskRectangle rect);
	typedef UInt32 (*FskPortScaleUInt32Proc)(FskPort port, UInt32 value);
	typedef SInt32 (*FskPortScaleSInt32Proc)(FskPort port, SInt32 value);
	typedef double (*FskPortScaleDoubleProc)(FskPort port, double value);

	struct FskPortRecord {
		void						*picSave;
		void						*picSaveNext;
        void                        *picSaveLastSwapBits;

		FskBitmap					bits;
		FskPointRecord				origin;
		FskColorRGBARecord			penColor;
		UInt32						graphicsMode;
		FskGraphicsModeParameters	graphicsModeParameters;
		FskRectangleRecord			clipRect;			// not relative to origin
		FskRectangleRecord			updateRect;			// not relative to origin

		struct FskWindowRecord		*window;
		FskRectangleRecord			changedArea;		// not relative to origin
		FskRectangleRecord			invalidArea;		// not relative to origin
		FskRectangleRecord			aggregateClip;		// changedArea intersected with invalidArea (NOT relative to origin)
        FskRectangleRecord          aggregateClipScaled;// aggregateClip with port scale applied
		UInt16						drawingDepth;		// how deeply nested begin/end is

		FskTextEngine				textEngine;
		char						*fontName;
		UInt32						textSize;
		UInt16						textHAlign;
		UInt16						textVAlign;
		UInt32						textStyle;
		FskFixed					textExtra;
		FskTextFormatCache			textFormatCache;
		Boolean						textFromFormat;

        Boolean                     lowMemoryWarningRegistered;

		FskPortTextFormat			textFormats;
		SInt32						useCount;

        FskPortVectorsRecord        vector;

        FskPortVectorsRecord        externalVector;

		FskFixed					scale;
		FskPortScaleRectProc		doScaleRect;
		FskPortScaleRectProc		doUnscaleRect;
		FskPortScaleUInt32Proc		doScaleUInt32;
		FskPortScaleUInt32Proc		doUnscaleUInt32;
		FskPortScaleSInt32Proc		doScaleSInt32;
		FskPortScaleSInt32Proc		doUnscaleSInt32;
		FskPortScaleDoubleProc		doScaleDouble;
		FskPortScaleDoubleProc		doUnscaleDouble;

        Boolean                     deferredOrigin;
        Boolean                     deferredPenColor;
        Boolean                     deferredTextStyle;
        Boolean                     deferredUnused;				/**< Padding to assure that graphicsModeCache is on a 4 byte boundary. */

		UInt8						graphicsModeCache[16];		/**< Copy of the variable-sized graphics mode. THIS MUST BE ON A 4-BYTE BOUNDARY. */
		char						fontNameCache[64];			/**< Cache for the font name. */

		FskInstrumentedItemDeclaration
	};

	struct FskPortTextFormatRecord {
		FskPortTextFormat				next;
		FskPort							port;
		FskTextFormatCache				textFormatCache;

		SInt16							useCount;
		UInt32							textSize;
		UInt32							textStyle;

		char							fontName[1];
	};

	struct FskPortPicEffectParametersRecord {
		FskBitmap				src;
		FskRectangleRecord		srcRect;
		FskPointRecord			dstPoint;
		FskEffectRecord			effect;
	};

	FskAPI(FskErr) FskPortPicSaveAdd(FskPort port, FskPortPicRenderItem draw, FskPortPicPrepareItem prepare, void *params, UInt32 paramsSize);

	/* These are in FskPortEfects.c */
	FskErr FskPortEffectGetSrcCompatibleGLCacheBitmap(FskPort port, FskConstBitmap src, FskConstRectangle srcRect, int flags, FskBitmap *bmp);
	FskErr FskPortEffectQueue(FskPort port, FskBitmap src, FskConstRectangle srcRect, FskConstPoint dstPoint, const struct FskEffectRecord *effect);
	void FskPortEffectCheckSourcesAreAccelerated(FskBitmap src, const struct FskEffectRecord *e);
	void FskPortEffectScale(FskPort port, struct FskPortPicEffectParametersRecord *portEffect);
	void FskPortLogEffect(FskPort port, void *params, const char *name);

#endif


FskAPI(FskErr) FskPortNew(FskPort *port, FskBitmap bits, const char *textEngine);
FskAPI(FskErr) FskPortDispose(FskPort port);

FskAPI(FskErr) FskPortSetOrigin(FskPort port, SInt32 x, SInt32 y);
FskAPI(FskErr) FskPortGetOrigin(FskPort port, SInt32 *x, SInt32 *y);
FskAPI(FskErr) FskPortOffsetOrigin(FskPort port, SInt32 x, SInt32 y);

FskAPI(FskErr) FskPortSetGraphicsMode(FskPort port, UInt32 graphicsMode, FskConstGraphicsModeParameters graphicsModeParameters);
FskAPI(FskErr) FskPortGetGraphicsMode(FskPort port, UInt32 *graphicsMode, FskGraphicsModeParameters *graphicsModeParameters);

FskAPI(FskErr) FskPortSetUpdateRectangle(FskPort port, FskConstRectangle updateRect);
FskAPI(FskErr) FskPortGetUpdateRectangle(FskPort port, FskRectangle updateRect);

FskAPI(FskErr) FskPortSetClipRectangle(FskPort port, FskConstRectangle clip);
FskAPI(FskErr) FskPortGetClipRectangle(FskPort port, FskRectangle clip);

FskAPI(FskErr) FskPortSetPenColor(FskPort port, FskConstColorRGBA color);
FskAPI(FskErr) FskPortGetPenColor(FskPort port, FskColorRGBA color);

FskAPI(FskErr) FskPortSetTextAlignment(FskPort port, UInt16 hAlign, UInt16 vAlign);
FskAPI(FskErr) FskPortGetTextAlignment(FskPort port, UInt16 *hAlign, UInt16 *vAlign);

FskAPI(FskErr) FskPortSetTextSize(FskPort port, UInt32 size);
FskAPI(FskErr) FskPortGetTextSize(FskPort port, UInt32 *size);

FskAPI(FskErr) FskPortSetTextStyle(FskPort port, UInt32 style);
FskAPI(FskErr) FskPortGetTextStyle(FskPort port, UInt32 *style);

FskAPI(FskErr) FskPortSetTextFont(FskPort port, const char *fontName);
FskAPI(FskErr) FskPortGetTextFont(FskPort port, char **fontName);

FskAPI(FskErr) FskPortSetTextExtra(FskPort port, FskFixed extra);
FskAPI(FskErr) FskPortGetTextExtra(FskPort port, FskFixed *extra);

FskAPI(FskErr) FskPortResetInvalidRectangle(FskPort port);
FskAPI(FskErr) FskPortInvalidateRectangle(FskPort port, FskConstRectangle area);
FskAPI(FskErr) FskPortGetInvalidRectangle(FskPort port, FskRectangle area);

FskAPI(FskErr) FskPortSetBitmap(FskPort port, FskBitmap bits);
FskAPI(FskErr) FskPortGetBitmap(FskPort port, FskBitmap *bits);

FskAPI(FskErr) FskPortGetBounds(FskPort port, FskRectangle bounds);

FskAPI(FskErr) FskPortBeginDrawing(FskPort port, FskConstColorRGBA color);
FskAPI(FskErr) FskPortEndDrawing(FskPort port);

FskAPI(Boolean) FskPortAccumulateChange(FskPort port, FskConstRectangle area);

FskAPI(void) FskPortRectangleFill(FskPort port, FskConstRectangle rect);
FskAPI(void) FskPortRectangleFrame(FskPort port, FskConstRectangle rect);
FskAPI(void) FskPortBitmapDraw(FskPort port, FskBitmap bits, FskConstRectangle srcRect, FskConstRectangle dstRect);
FskAPI(void) FskPortBitmapDrawSubpixel(FskPort port, FskBitmap bits, FskConstRectangle srcRect, double x, double y, double width, double height);
FskAPI(void) FskPortBitmapScaleOffset(FskPort port, FskBitmap bits, FskConstRectangle srcRect, const struct FskScaleOffset *scaleOffset);
FskAPI(void) FskPortBitmapTile(FskPort port, FskBitmap srcBits, FskConstRectangle srcRect, FskConstRectangle dstRect, FskFixed scale);
FskAPI(void) FskPortTextDraw(FskPort port, const char *text, UInt32 textLen, FskConstRectangle bounds);
FskAPI(void) FskPortTextDrawSubpixel(FskPort port, const char *text, UInt32 textLen, double x, double y, double width, double height);
FskAPI(void) FskPortStringDraw(FskPort port, const char *text, FskConstRectangle bounds);

FskAPI(void) FskPortTextGetBounds(FskPort port, const char *text, UInt32 textLen, FskRectangle bounds);
FskAPI(void) FskPortTextGetBoundsSubPixel(FskPort port, const char *text, UInt32 textLen, double *width, double *height);
FskAPI(void) FskPortStringGetBounds(FskPort port, const char *text, FskRectangle bounds);

FskAPI(FskErr) FskPortTextFitWidth(FskPort port, const char *text, UInt32 textLen, UInt32 width, UInt32 flags, UInt32 *fitBytes, UInt32 *fitChars);
FskAPI(FskErr) FskPortGetFontInfo(FskPort port, FskTextFontInfo info);

FskAPI(FskErr) FskPortTextFormatNew(FskPortTextFormat *textFormat, FskPort port, UInt32 textSize, UInt32 textStyle, const char *fontName);
FskAPI(FskErr) FskPortTextFormatDispose(FskPortTextFormat textFormat);

FskAPI(FskErr) FskPortTextFormatApply(FskPort port, FskPortTextFormat textFormat);

FskAPI(FskErr) FskPortTextGetEngine(FskPort port, char **engine);

FskAPI(FskErr) FskPortPreferredYUVFormatsGet(FskPort port, FskBitmapFormatEnum **yuvFormats);
#define        FskPreferredYUVFormatsDispose(yuvFormats)	FskMemPtrDispose(yuvFormats)

FskAPI(FskErr) FskPortPreferredRGBFormatsGet(FskPort port, FskBitmapFormatEnum *noAlpha, FskBitmapFormatEnum *withAlpha);

#ifdef __FSKPORT_PRIV__
    #define FskPortRectScale(port, r) (port->doScaleRect(port, r))
    #define FskPortRectUnscale(port, r) (port->doUnscaleRect(port, r))
    #define FskPortUInt32Scale(port, v) (port->doScaleUInt32(port, v))
    #define FskPortUInt32Unscale(port, v) (port->doUnscaleUInt32(port, v))
    #define FskPortSInt32Scale(port, v) (port->doScaleSInt32(port, v))
    #define FskPortSInt32Unscale(port, v) (port->doUnscaleSInt32(port, v))
    #define FskPortDoubleScale(port, v) (port->doScaleDouble(port, v))
    #define FskPortDoubleUnscale(port, v) (port->doUnscaleDouble(port, v))
#endif

FskAPI(FskFixed) FskPortScaleGet(FskPort port);
FskAPI(void) FskPortScaleSet(FskPort port, FskFixed scale);

FskAPI(void) FskPortSetVectors(FskPort port, FskPortVectors externalVector);

struct FskEffectRecord;	/**< Forward declaration, defined in defined in FskEffects.h, where the API uses FskConstEffect = const struct FskEffectRecord* */

/** Apply an effect.
 **	\param[in]	effect		The parameters for the effect, including its name.
 **	\param[in]	src			The source bitmap.
 **	\param[in]	srcRect		A rectangle specifying a sunset of the src bitmap. NULL implies the whole bitmap.
 **	\param[out]	dst			The destination bitmap.
 **	\param[in]	dstPoint	The location in the destination where the result is to be placed.
 **	\return		kFskErrNone	if the operation was successful.
 **/
FskAPI(void) FskPortEffectApply(FskPort port, FskBitmap src, FskConstRectangle srcRect, FskConstPoint dstPoint, const struct FskEffectRecord* effect);


/** Get a temporary bitmap for use in effects.
 ** If the port is GL accelerated, it will come from a texture cache; otherwise an FskBitmap will be allocated.
 ** In any event, the pixel format will be appropriately chosen as kFskBitmapFormatGLRGBA or kFskBitmapFormatDefaultRGBA,
 ** depending on whether the port is OpenGL accelerated or not, to provide a unified simpler interface to the caller.
 ** When done, this can be released with FskPortReleaseTempEffectBitmap() to return it to the cache for reuse,
 ** or disposed with FskBitmapDispose().
 **	\param[in]	port		The port used as  destination for the effects.
 **	\param[in]	width		The desired width  of the temporary bitmap.
 **	\param[in]	height		The desired height of the temporary bitmap.
 **	\param[in]	pixelFormat	The desired format of the temporary bitmap.
 **	\param[in]	bmp			A place to store the temporary bitmap.
 **	\return		kFskErrNone	if the bitmap was successfully allocated.
 **/
FskAPI(FskErr)	FskPortGetTempEffectBitmap(FskPort port, SInt32 width, SInt32 height, FskBitmapFormatEnum pixelFormat, FskBitmap *bmp);


/** Release the temporary bitmap used for effects.
 ** If the port is GL accelerated, it will be saved to a texture cache; otherwise it will be disposed.
 **	\note	This will fail with an error code if the use count is not 0, because this indicates that some other code is still using it,
 **			and FskBitmapDispose() will be called instead.
 **	\param[in]	port		The port used as  destination for the effects.
 **	\param[in]	width		The desired width  of the temporary bitmap.
 **	\param[in]	height		The desired height of the temporary bitmap.
 **	\param[in]	pixelFormat	The desired format of the temporary bitmap.
 **	\param[in]	bmp			A place to store the temporary bitmap.
 **	\return		kFskErrNone		if the bitmap was released successfully.
 **	\return		kFskErrIsBusy	if the use count is not zero.
 **/
FskAPI(void)	FskPortReleaseTempEffectBitmap(FskPort port, FskBitmap bm);


/** Release the OpenGL resources that the port is keeping.
 ** If the port is GL accelerated, it will be saved to a texture cache; otherwise it will be disposed.
 **	\param[in]	port	The port.
 **/
//@@ NO ONE EVER CALLS THIS
FskAPI(void)	FskPortReleaseGLResources(FskPort port);

#if SUPPORT_INSTRUMENTATION

enum {
	kFskPortInstrMsgSetGraphicsMode = kFskInstrumentedItemFirstCustomMessage,
	kFskPortInstrMsgSetClipRectangle,
	kFskPortInstrMsgSetPenColor,
	kFskPortInstrMsgSetTextAlignment,
	kFskPortInstrMsgSetTextSize,
	kFskPortInstrMsgSetTextStyle,
	kFskPortInstrMsgSetTextFont,
	kFskPortInstrMsgSetTextExtra,
	kFskPortInstrMsgInvalidateRectangle,
	kFskPortInstrMsgSetBitmap,
	kFskPortInstrMsgBeginDrawing,
	kFskPortInstrMsgEndDrawing,
	kFskPortInstrMsgRectangleFill,
	kFskPortInstrMsgRectangleFrame,
	kFskPortInstrMsgApplyMaskAndValue,
	kFskPortInstrMsgBitmapDraw,
	kFskPortInstrMsgTextDraw,
	kFskPortInstrMsgTextDrawSubpixel,
	kFskPortInstrMsgTextGetBounds,
	kFskPortInstrMsgTextFitWidth,
	kFskPortInstrMsgGetFontInfo,
	kFskPortInstrMsgTextFormatApply,
	kFskPortInstrMsgTextFlushCache,
	kFskPortInstrMsgScaleSet,
	kFskPortInstrMsgBitmapScaleOffset,
    kFskPortInstrMsgSetOrigin,
    kFskPortInstrMsgOffsetOrigin,
    kFskPortInstrMsgEffect,
    kFskPortInstrMsgGetBitmap,
    kFskPortInstrMsgBitmapTile,
    kFskPortInstrMsgBitmapSubpixel,
    kFskPortInstrMsgPicSaveAdd
};

#endif

#ifdef __cplusplus
}
#endif

#endif
