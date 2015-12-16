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
#ifndef __FSKTEXT__
#define __FSKTEXT__

#include "FskRectangle.h"
#include "FskBitmap.h"
#include "FskMedia.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct FskTextEngineRecord FskTextEngineRecord;
typedef struct FskTextEngineRecord *FskTextEngine;

typedef struct FskTextFormatCacheRecord FskTextFormatCacheRecord;
typedef struct FskTextFormatCacheRecord *FskTextFormatCache;

/** Information about a font type face.
 *	This is specific to a face of a particular name, size and style.
 **/
typedef struct {
	UInt32		ascent;		/**< The distance from the alphabetic baseline to the highest ascender of the entire face. */
	UInt32		descent;	/**< The distance from the alphabetic baseline to the lowest descender of the entire face. */
	UInt32		leading;	/**< The extra spacing between one line and the next, beyond the ascent and the descent. */

	UInt32		height;		/**<  ascent + descent + leading - may not be the same as the total of those fields due to rounding when scaling */
	UInt32		width;		/**<  width of the widest glyph */

	UInt32		glyphHeight;		/**<  ascent + descent - may not be the same as the total of those fields due to rounding when scaling */
} FskTextFontInfoRecord, *FskTextFontInfo;

#ifdef __FSKTEXT_PRIV__

typedef struct FskTextEngineStateRecord FskTextEngineStateRecord;
typedef struct FskTextEngineStateRecord *FskTextEngineState;

typedef struct {
	const char *name;

	FskErr (*doNew)(FskTextEngineState *state);
	FskErr (*doDispose)(FskTextEngineState state);

	FskErr (*doFormatCacheNew)(FskTextEngineState state, FskTextFormatCache *cache, FskBitmap bits, UInt32 textSize, UInt32 textStyle, const char *fontName);
	FskErr (*doFormatCacheDispose)(FskTextEngineState state, FskTextFormatCache cache);
	FskErr (*doBox)(FskTextEngineState state, FskBitmap bits, const char *text, UInt32 textLen, FskConstRectangle dstRect, FskConstRectangleFloat dstRectFloat, FskConstRectangle clipRect, FskConstColorRGBA color, UInt32 blendLevel,
					UInt32 textSize, UInt32 textStyle, UInt16 hAlign, UInt16 vAlign, const char *fontName, FskTextFormatCache cache);
	FskErr (*doGetBounds)(FskTextEngineState state, FskBitmap bits, const char *text, UInt32 textLen, UInt32 textSize, UInt32 textStyle, const char *fontName, FskRectangle bounds, FskDimensionFloat dimensions, FskTextFormatCache cache);
	FskErr (*doGetFontInfo)(FskTextEngineState state, FskTextFontInfo info, const char *fontName, UInt32 textSize, UInt32 textStyle, FskTextFormatCache formatCache);
	FskErr (*doFitWidth)(FskTextEngineState state, FskBitmap bits, const char *text, UInt32 textLen, UInt32 textSize, UInt32 textStyle, const char *fontName, UInt32 width, UInt32 flags,
					UInt32 *fitBytes, UInt32 *fitChars, FskTextFormatCache cache);
	FskErr (*doGetLayout)(FskTextEngineState state, FskBitmap bits, const char *text, UInt32 textLen, UInt32 textSize, UInt32 textStyle, const char *fontName,
					UInt16 **unicodeText, UInt32 *unicodeLen, FskFixed **layout, FskTextFormatCache cache);

	// global functions
	FskErr (*doAddFontFile)(FskTextEngineState state, const char *path);
	FskErr (*doSetDefaultFont)(FskTextEngineState state, const char *defaultFont);
	FskErr (*doGetFontList)(FskTextEngineState state, char **fontNames);
	FskErr (*doSetFamilyMap)(FskTextEngineState state, const char *map);

	// instance properties
	FskMediaPropertyEntry		properties;
} FskTextEngineDispatchRecord, *FskTextEngineDispatch;

struct FskTextEngineRecord {
	FskTextEngineDispatch	dispatch;
	FskTextEngineState		state;
    Boolean                 fractionalTextSize;
};

#endif /* __FSKTEXT_PRIV__ */


/** Create a new text engine.
 *	\param[out]	fte			place to store a reference to the font text engine.
 *	\param		name		Specific text engine to use, or kFskTextDefaultEngine.
 *	\return		kFskErrNone	if successful.
 **/
FskAPI(FskErr) FskTextEngineNew(FskTextEngine *fte, const char *name);

/** Dispose a text engine.
 *	\param[in]	fte			reference to the font text engine.
 *	\param		propertyID	the ID of the property to be queried.
 *	\param		get			???
 *	\param		set			???
 *	\param		dataType	???
 *	\return		kFskErrNone	if successful.
 **/
FskAPI(FskErr) FskTextEngineDispose(FskTextEngine fte);

/** Create a structure embodying all of the properties of a typeface.
 *	This accelerates all subsequent text operations.
 *	\param[in]	fte			reference to the font text engine.
 *	\param[out]	cache		the resultant font cache.
 *	\param[in]	bits		a reference bitmap, which can be NULL but may be faster is one is supplied.
 *	\param[in]	textSize	the size of the typeface, in pixels.
 *	\param[in]	textStyle	the style of the text,composed of kFskTextPlain, kFskTextBold, kFskTextItalic, kFskTextUnderline, kFskTextOutline,
 *	\param[in]	fontName	the name of the font.
 *	\return		kFskErrNone	if the cache was created successfully.
 **/
FskAPI(FskErr) FskTextFormatCacheNew(FskTextEngine fte, FskTextFormatCache *cache, FskBitmap bits, UInt32 textSize, UInt32 textStyle, const char *fontName);

/** Dispose a text format cache, previously allocated with FskTextFormatCacheNew().
 *	This accelerates all subsequent text operations.
 *	\param[in]	fte			reference to the font text engine.
 *	\param[in]	cache		the font cache.
 *	\return		kFskErrNone	if completed successfully.
 **/
FskAPI(FskErr) FskTextFormatCacheDispose(FskTextEngine fte, FskTextFormatCache cache);

enum {
	kFskTextEnginePropertyZoom = 1,
	kFskTextEnginePropertyFractionalTextSize = 2,
};

/** Test whether the text engine has a given property.
 *	\param[in]	fte			reference to the font text engine.
 *	\param		propertyID	the ID of the property to be queried.
 *	\param		get			???
 *	\param		set			???
 *	\param		dataType	???
 *	\return		kFskErrNone	if successful.
 **/
FskAPI(FskErr) FskTextEngineHasProperty(FskTextEngine fte, UInt32 propertyID, Boolean *get, Boolean *set, UInt32 *dataType);

/** Set the value of a property of the text engine.
 *	\param[in]	fte			reference to the font text engine.
 *	\param		propertyID	the ID of the property to be set.
 *	\param		property	the value of the property to be set.
 *	\return		kFskErrNone	if successful.
 **/
FskAPI(FskErr) FskTextEngineSetProperty(FskTextEngine fte, UInt32 propertyID, FskMediaPropertyValue property);

/** Get the value of a property of the text engine.
 *	\param[in]	fte			reference to the font text engine.
 *	\param		propertyID	the ID of the property whose value is to be retrieved.
 *	\param		property	a place to store the value of the specified property.
 *	\return		kFskErrNone	if successful.
 **/
FskAPI(FskErr) FskTextEngineGetProperty(FskTextEngine fte, UInt32 propertyID, FskMediaPropertyValue property);

/** Render text into a rectangle.
 *	\param[in]	fte			reference to the font text engine.
 *	\param[in]	bits		a reference bitmap, which can be NULL but may be faster is one is supplied.
 *	\param[in]	text		the string of UTF-8 text.
 *	\param[in]	textLen		the number of bytes in the UTF-8 text.
 *	\param[in]	dstRect		the rectangle into which the text is to be rendered.
 *	\param[in]	dstRectFloa	the rectangle into which the text is to be rendered. May be null.
 *	\param[in]	clipRect	a rectangular clipping region (may be NULL).
 *	\param[in]	color		the desired color of the text.
 *	\param[in]	blendLevel	transparency of the text (255 is opaque, 0 is totally transparent).
 *	\param[in]	textSize	the size of the typeface, in pixels.
 *	\param[in]	textStyle	the style of the text,composed of kFskTextPlain, kFskTextBold, kFskTextItalic, kFskTextUnderline, kFskTextOutline,
 *	\param[in]	hAlign		horizontal alignment, one of kFskTextAlignCenter, kFskTextAlignLeft, kFskTextAlignRight,  from FskGraphics.h.
 *	\param[in]	vAlign		vertical   alignment, one of kFskTextAlignCenter, kFskTextAlignTop,  kFskTextAlignBottom, from FskGraphics.h.
 *	\param[in]	fontName	the name of the font.
 *	\param[in]	cache		the font cache.
 *	\return		kFskErrNone	if the text was rendered successfully.
 **/
FskAPI(FskErr) FskTextBox(FskTextEngine fte, FskBitmap bits, const char *text, UInt32 textLen, FskConstRectangle dstRect, FskConstRectangleFloat dstRectFloat, FskConstRectangle clipRect, FskConstColorRGBA color,
							UInt32 blendLevel, UInt32 textSize, UInt32 textStyle, UInt16 hAlign, UInt16 vAlign, const char *fontName, FskTextFormatCache cache);

/** Get the rough bounds of a text string.
 *	There is no guarantee whatsoever that no text would be rendered outside of these bounds.
 *	\param[in]	fte			reference to the font text engine.
 *	\param[in]	bits		a reference bitmap, which can be NULL but may be faster is one is supplied.
 *	\param[in]	text		the string of UTF-8 text.
 *	\param[in]	textLen		the number of bytes in the UTF-8 text.
 *	\param[in]	textSize	the size of the typeface, in pixels.
 *	\param[in]	textStyle	the style of the text,composed of kFskTextPlain, kFskTextBold, kFskTextItalic, kFskTextUnderline, kFskTextOutline,
 * 							kFskTextStrike, kFskTextOutlineHeavy, kFskTextTruncateEnd, kFskTextTruncateCenter, from FskGraphics.h.
 *	\param[in]	fontName	the name of the font.
 *	\param[out]	bounds		the bounds of the text.
 *	\param[out]	dimensions	the bounds of the text - optional.
 *	\param[in]	cache		the font cache.
 *	\return		kFskErrNone	if the bounds were retrieved successfully.
 **/
FskAPI(FskErr) FskTextGetBounds(FskTextEngine fte, FskBitmap bits, const char *text, UInt32 textLen, UInt32 textSize, UInt32 textStyle, const char *fontName,
								FskRectangle bounds, FskDimensionFloat dimensions, FskTextFormatCache cache);

/** Get the list of fonts available on this system.
 *	\param[in]	fte			reference to the font text engine.
 *	\param[out]	fontNames	address of a pointer that receives the font names in the form of a string list, where each font name ends
 *							with a null character, and the last name is followed by an additional null character, two altogether.
 *							Dispose with FskMemPtrDispose().
 *	\return		kFskErrNone	if the font list was retrieved successfully.
 **/
FskAPI(FskErr) FskTextGetFontList(FskTextEngine fte, char **fontNames);

/** Get the layout for a string of UTF-8 text.
 *	\param[in]	fte			reference to the font text engine.
 *	\param[in]	bits		a reference bitmap, which can be NULL but may be faster is one is supplied.
 *	\param[in]	text		the string of UTF-8 text.
 *	\param[in]	textLen		the number of bytes in the UTF-8 text.
 *	\param[in]	textSize	the size of the typeface, in pixels.
 *	\param[in]	textStyle	the style of the text,composed of kFskTextPlain, kFskTextBold, kFskTextItalic, kFskTextUnderline, kFskTextOutline,
 * 							kFskTextStrike, kFskTextOutlineHeavy, kFskTextTruncateEnd, kFskTextTruncateCenter, from FskGraphics.h.
 *	\param[in]	fontName	the name of the font.
 *	\param[out]	unicodeText	the address of a pointer where the unicode representation of the text is to be stored; dispose with FskMemPtrDispose() (can be NULL).
 *	\param[out]	unicodeLen	the number of unicode characters; also the number of layout points.
 *	\param[out]	layout		the location of each of the unicode characters; dispose with FskMemPtrDispose() (can be NULL).
 *	\param[in]	cache		the font cache.
 *	\return		kFskErrNone	if the layout was retrieved successfully.
 **/
FskAPI(FskErr) FskTextGetLayout(FskTextEngine fte, FskBitmap bits, const char *text, UInt32 textLen, UInt32 textSize, UInt32 textStyle, const char *fontName,
								UInt16 **unicodeText, UInt32 *unicodeLen, FskFixed **layout, FskTextFormatCache cache);

/** Get information about a font: ascent, descent, leading.
 *	\param[in]	fte			reference to the font text engine.
 *	\param[out]	info		pointer to a place to store the font information.
 *	\param[in]	fontName	the name of the font.
 *	\param[in]	textSize	the size of the typeface, in pixels.
 *	\param[in]	textStyle	the style of the text,composed of kFskTextPlain, kFskTextBold, kFskTextItalic, kFskTextUnderline, kFskTextOutline,
 * 							kFskTextStrike, kFskTextOutlineHeavy, kFskTextTruncateEnd, kFskTextTruncateCenter, from FskGraphics.h.
 *	\param[in]	cache		the font cache.
 *	\return		kFskErrNone	if the information was retrieved successfully.
 **/
FskAPI(FskErr) FskTextGetFontInfo(FskTextEngine fte, FskTextFontInfo info, const char *fontName, UInt32 textSize, UInt32 textStyle, FskTextFormatCache formatCache);

/** Options for the flags parameter of FskTextFitWidth(). **/
enum {
	kFskTextFitFlagNone		= 0,		/**< No special fitting instructions. */
	kFskTextFitFlagMidpoint	= 1L << 1,	/**< ??? */
	kFskTextFitFlagBreak	= 1L << 2,	/**< ??? */
};

/** Fit text.
 *	\param[in]	fte			reference to the font text engine.
 *	\param[in]	bits		a reference bitmap, which can be NULL but may be faster is one is supplied.
 *	\param[in]	text		the string of UTF-8 text.
 *	\param[in]	textLen		the number of bytes in the UTF-8 text.
 *	\param[in]	textSize	the size of the typeface, in pixels.
 *	\param[in]	textStyle	the style of the text,composed of kFskTextPlain, kFskTextBold, kFskTextItalic, kFskTextUnderline, kFskTextOutline,
 * 							kFskTextStrike, kFskTextOutlineHeavy, kFskTextTruncateEnd, kFskTextTruncateCenter, from FskGraphics.h.
 *	\param[in]	fontName	the name of the font.
 *	\param[in]	width		???
 *	\param[in]	flags		???
 *	\param		fitBytes	???
 *	\param		fitChars	???
 *	\param[in]	cache		the font cache.
 *	\return		kFskErrNone	if successful.
 **/
FskAPI(FskErr) FskTextFitWidth(FskTextEngine fte, FskBitmap bits, const char *text, UInt32 textLen, UInt32 textSize, UInt32 textStyle, const char *fontName, UInt32 width, UInt32 flags, UInt32 *fitBytes, UInt32 *fitChars, FskTextFormatCache cache);

#ifdef __FSKTEXT_PRIV__
	/** Initialize the text system.
	 *	\return	kFskErrNone	if successful.
	 **/
	FskErr FskTextInitialize(void);

	/** Shut down the text system.
	 *	\return	kFskErrNone	if successful.
	 **/
	void FskTextTerminate(void);

	/** Set the default face for this text engine.
	 *	\param[in]	fte			reference to the font text engine.
	 *	\param[in]	defaultFace	the type face to be used if not otherwise specified.
	 *	\return		kFskErrNone	if successful.
	 **/
	FskErr FskTextDefaultFontSet(FskTextEngine fte, const char *defaultFace);

	/** Set the default font family map for this text engine.
	 *	\param[in]	fte			reference to the font text engine.
	 *	\param[in]	map			the font family map.
	 *	\return		kFskErrNone	if successful.
	 **/
	FskErr FskTextSetFamilyMap(FskTextEngine fte, const char *map);
#endif /* __FSKTEXT_PRIV__ */

/** Make the font text engine aware of a font located in a file.
 *	\param[in]	fte			reference to the font text engine.
 *	\param[in]	path		full file name path to the font file.
 *	\return		kFskErrNone	if successful.
 **/
FskAPI(FskErr) FskTextAddFontFile(FskTextEngine fte, const char *path);

#if FSK_TEXT_FREETYPE
	struct FT_FaceRec_;

	/** Find the Freetype Face Record corresponding to a given type face.
	 *	\param[in]	fontName	the name of the font.
	 *	\param[in]	textStyle	the style of the text,composed of kFskTextPlain, kFskTextBold, kFskTextItalic, kFskTextUnderline, kFskTextOutline,
	 * 							kFskTextStrike, kFskTextOutlineHeavy, kFskTextTruncateEnd, kFskTextTruncateCenter, from FskGraphics.h.
	 *	\param[in]	textSize	the size of the typeface, in pixels.
	 *	\return		the Freetype Face Record	if successful,
	 *	\return		NULL						otherwise.
	 **/
	FskAPI(struct FT_FaceRec_*)	FskFTFindFont(const char *fontName, UInt32 textStyle, float textSize);
	FskAPI(FskErr)	FskFTAddMapping(const char *path);
#endif /* FSK_TEXT_FREETYPE */

#if USE_GLYPH	/* define APIs only for now -- no disptach functions are defined yet */
       FskAPI(FskErr) FskTextGlyphGetBounds(FskTextEngine fte, FskBitmap bits, const UInt16 *codes, UInt32 codesLen, UInt32 textSize, UInt32 textStyle, const char *fontName, FskRectangle bounds, FskTextFormatCache cache);
      FskAPI(FskErr) FskTextGlyphBox(FskTextEngine fte, FskBitmap bits, const UInt16 *codes, UInt32 codesLen, FskConstRectangle r, FskConstRectangle clipRect, FskConstColorRGBA color, UInt32 blendLevel, UInt32 textSize, UInt32 textStyle, UInt16 hAlign, UInt16 vAlign, const char *fontName, FskTextFormatCache cache);
     FskAPI(FskErr) FskTextGetGlyphs(FskTextEngine fte, FskBitmap bits, const char *text, UInt32 textLen, UInt32 textSize, UInt32 textStyle, const char *fontName, UInt16 **glyphsPtr, UInt32 *glyphsLenPtr, FskFixed **layout, float *widthPtr, float *heightPtr, FskTextFormatCache cache);
#endif

#if SUPPORT_INSTRUMENTATION	/* TODO: Put these into the dispatch table. Make available outside of instrumentation for debugging? */
FskAPI(void) FskCocoaTextFormatCacheInfoStringGet(FskTextFormatCache cache, char **pInfoStr);
FskAPI(void) FskFreeTypeTextFormatCacheInfoStringGet(FskTextFormatCache cache, char **pInfoStr);
#endif /* SUPPORT_INSTRUMENTATION */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __FSKTEXT__ */
