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
#ifndef __FSKGLYPHPATH__
#define __FSKGLYPHPATH__

#ifndef __FSKPATH__
# include "FskPath.h"
#endif /* __FSKPATH__ */



#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* Text anchor */
#define kFskTextAnchorStart				0
#define kFskTextAnchorMiddle			1
#define kFskTextAnchorEnd				2


/* Font Styles */
#define kFskFontStyleNormal				0
#define kFskFontStyleItalic				1
#define kFskFontStyleOblique			2


/* Font Weights = { kFskFontWeightNormal | kFskFontWeightBold | 100 | 200 | 300 | 400 | 500 | 600 | 700 | 800 | 900 } */
#define kFskFontWeightNormal			400
#define kFskFontWeightBold				700


/* Font Variant */
#define kFskFontVariantNormal			0
#define kFskFontVariantSmallCaps		1


/* Font Stretch */
#define kFskFontStretchUltraCondensed	1
#define kFskFontStretchExtraCondensed	2
#define kFskFontStretchCondensed		3
#define kFskFontStretchSemiCondensed	4
#define kFskFontStretchNormal			5
#define kFskFontStretchSemiExpanded		6
#define kFskFontStretchExpanded			7
#define kFskFontStretchExtraExpanded	8
#define kFskFontStretchUltraExpanded	9


/* Font Decoration */
#define kFskFontDecorationNone			0
#define kFskFontDecorationUnderline		(1 << 0)
#define kFskFontDecorationOverline		(1 << 1)
#define kFskFontDecorationLineThrough	(1 << 2)
#define kFskFontDecorationBlink			(1 << 3)


typedef struct FskFontAttributes {
	/* Tiny profile */
	char	*family;	/* Comma-separated list of font names */
	float	size;		/* baseline  to baseline, when set solid */
	UInt16	weight;		/* bold, ... */
	UInt8	style;		/* italic ... */
	UInt8	anchor;		/* start, middle, end */

	/* Additional attributes of the Basic profile */
	UInt8	stretch;	/* expanded, condensed, ... */
	UInt8	decoration;	/* underline, overline, lineThrough, blink */
	UInt8	variant;	/* small caps */
	float	sizeAdjust;
} FskFontAttributes;


/********************************************************************************
 * High-level routines
 ********************************************************************************/

				/* Create a new path from a Unicode text string */
FskAPI(FskErr)	FskPathFromUnicodeStringNew(
					const UInt16			*uniChars,
					UInt32					numUniChars,
					const FskFontAttributes	*attributes,
					FskFixedPoint2D			*origin,		/* On input, the beginning, on output the end */
					FskPath					*path
				);

/* Create a new growable path from a Unicode text string */
FskAPI(FskErr)	FskGrowablePathFromUnicodeStringNew(
					const UInt16			*uniChars,
					UInt32					numUniChars,
					const FskFontAttributes	*attributes,
					FskFixedPoint2D			*origin,		/* On input, the beginning, on output the end */
					FskGrowablePath			*path
				);

/* Get the width, in pixels, of a Unicode string. A negative result is an error. */
FskAPI(FskFixed)	FskUnicodeStringGetWidth(
						const UInt16			*uniChars,
						UInt32					numUniChars,
						const FskFontAttributes	*attributes
					);


/********************************************************************************
 * Low-level routines
 ********************************************************************************/

				/* Opaque text context for efficient operation of low level routines */
typedef struct	FskTextContextRecord *FskTextContext;
FskAPI(FskErr)	FskTextContextFromFontAttributesNew(const FskFontAttributes *attributes, FskTextContext *FskTextContext);
FskAPI(void)	FskTextContextDispose(FskTextContext textContext);
FskAPI(FskErr)	FskTextContextClone(FskTextContext textContext, FskTextContext *newTextContext);

				/* Append the path of the given glyph to the textContext */
typedef UInt16	FskGlyphID;
FskErr			FskTextContextAppendGlyphPath(FskGlyphID glyphID, FskFixed x0, FskFixed y0, FskTextContext textContext);

				/* Get a glyph string corresponding to the unicode string */
FskAPI(FskErr)	FskGlyphStringNewFromUnicodeString(const UInt16 *uniChars, UInt32 numUniChars, FskTextContext textContext, FskGlyphID **glyphs, FskFixedVector2D **offset, UInt32 *numGlyphs);
FskAPI(void)	FskGlyphStringDispose(FskGlyphID *glyphs, FskFixedVector2D *offset);

				/* Get a path from the context */
FskAPI(FskErr)	FskTextContextGetPath(FskTextContext textContext, FskConstPath *path);	/* The text context's path itself */
FskAPI(FskErr)	FskTextContextNewPath(FskTextContext textContext, FskPath *path);		/* A copy of the text context's path */

				/* Get the list of names of font faces available on this system */
FskAPI(FskErr)	FskGetSystemFontFaceNameList(FskConstGrowableArray *faceNames, void *sysContext);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __FSKGLYPHPATH__ */

