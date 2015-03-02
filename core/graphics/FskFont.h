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
#ifndef __FSKFONT__
#define __FSKFONT__

#ifndef __FSKGLYPHPATH__
# include "FskGlyphPath.h"
#endif /* __FSKGLYPHPATH__ */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* Opaque Structure definitions */
typedef					FskGrowableArray		FskFontCollection;
typedef					FskConstGrowableArray	FskConstFontCollection;
typedef struct			FskFontRecord			*FskFont;
typedef const struct	FskFontRecord			*FskConstFont;


/********************************************************************************
 ********************************************************************************
 ********************************************************************************
 *****						Font Attribute Definitions						*****
 ********************************************************************************
 ********************************************************************************
 ********************************************************************************/


/* Font face styles -- OR these together */
#define kFskFontFaceStyleNormal			(1 << kFskFontStyleNormal)
#define kFskFontFaceStyleItalic			(1 << kFskFontStyleItalic)
#define kFskFontFaceStyleOblique		(1 << kFskFontStyleOblique)
#define kFskFontFaceStyleAll			(	kFskFontFaceStyleNormal		\
										|	kFskFontFaceStyleItalic		\
										|	kFskFontFaceStyleOblique	\
										)

/* Font face variant -- OR these together (no ALL is specified) */
#define kFskFaceFontVariantNormal		(1 << kFskFontVariantNormal)
#define kFskFaceFontVariantSmallCaps	(1 << kFskFontVariantSmallCaps)

/* Font face weights -- OR these together */
#define kFskFontFaceWeight(w)			(1 << ((UInt32)((w) - 50) / 100))	/* w={100,200,300,400,500,600,700,800,900} ==> {1,2,4,8,16,32,64,128,256} */
#define kFskFontFaceWeightNormal		kFskFontFaceWeight(kFskFontWeightNormal)
#define kFskFontFaceWeightBold			kFskFontFaceWeight(kFskFontWeightBold)
#define kFskFontFaceWeightAll			(	kFskFontFaceWeight(100)	\
										|	kFskFontFaceWeight(200)	\
										|	kFskFontFaceWeight(300)	\
										|	kFskFontFaceWeight(400)	\
										|	kFskFontFaceWeight(500)	\
										|	kFskFontFaceWeight(600)	\
										|	kFskFontFaceWeight(700)	\
										|	kFskFontFaceWeight(800)	\
										|	kFskFontFaceWeight(900)	\
										)

/* Font face stretch -- OR these together */
#define kFskFontFaceStretchUltraCondensed	(1 << (kFskFontStretchUltraCondensed - 1))
#define kFskFontFaceStretchExtraCondensed	(1 << (kFskFontStretchExtraCondensed - 1))
#define kFskFontFaceStretchCondensed		(1 << (kFskFontStretchCondensed      - 1))
#define kFskFontFaceStretchSemiCondensed	(1 << (kFskFontStretchSemiCondensed  - 1))
#define kFskFontFaceStretchNormal			(1 << (kFskFontStretchNormal         - 1))
#define kFskFontFaceStretchSemiExpanded		(1 << (kFskFontStretchSemiExpanded   - 1))
#define kFskFontFaceStretchExpanded			(1 << (kFskFontStretchExpanded       - 1))
#define kFskFontFaceStretchExtraExpanded	(1 << (kFskFontStretchExtraExpanded  - 1))
#define kFskFontFaceStretchUltraExpanded	(1 << (kFskFontStretchUltraExpanded  - 1))
#define kFskFontFaceStretchAll				(	kFskFontFaceStretchUltraCondensed	\
											|	kFskFontFaceStretchExtraCondensed	\
											|	kFskFontFaceStretchCondensed		\
											|	kFskFontFaceStretchSemiCondensed	\
											|	kFskFontFaceStretchNormal			\
											|	kFskFontFaceStretchSemiExpanded		\
											|	kFskFontFaceStretchExpanded			\
											|	kFskFontFaceStretchExtraExpanded	\
											|	kFskFontFaceStretchUltraExpanded	\
											)

/* Glyph orientation */
#define kFskFontGlyphOrientationHorizontal	1
#define kFskFontGlyphOrientationVertical	2
#define kFskFontGlyphOrientationAll			3	/* (or 0) */

/* Arabic form */
#define kFskFontGlyphArabicFormNone			0
#define kFskFontGlyphArabicFormIsolated		0
#define kFskFontGlyphArabicFormInitial		1
#define kFskFontGlyphArabicFormMedial		2
#define kFskFontGlyphArabicFormTerminal		3




/********************************************************************************
 ********************************************************************************
 ********************************************************************************
 *****							Font Data Structures						*****
 ********************************************************************************
 ********************************************************************************
 ********************************************************************************/



/* Kerning specification is a list of FskKernSpec, terminate by a FskKernSpec populated with zeros */
typedef struct FskKernSpec {
	char		*u1;	/* e.g. "A"."ffl",&#x66;&#x66;&#x6c;,&#102;&#102;&#108;,U+20A7,U+215?,U+00??,U+E??,U+AC00-D7FF,U+11E00-121FF */
	char		*g1;	/* e.g. "contour integral", "dollar sign", "Latin capital letter T" */
	char		*u2;	/* Second character in kern pair, identified by Unicode */
	char		*g2;	/* Second character in kern pair, identified by glyph name */
	FskFixed	k;		/* Kerning adjustment, in points */
} FskKernSpec;

typedef struct FskUnicodeRange {
	UInt32	first;	/* We use 32 bit Unicode to accommodate the default ... */
	UInt32	last;	/* ... SVG Unicode range [0, 0x10FFFF], ... */
} FskUnicodeRange;	/* ... though we use 0 as the terminator */


typedef struct FskFontFaceSpec {
	char					*fontFamily;			/* Comma-separated UTF-8 list */
	UInt32					fontStyle;				/* OR of {normal, italic, obliqued}; default = all */
	UInt32					fontVariant;			/* OR of {normal, small-caps}; default = normal */
	UInt32					fontWeight;				/* OR of {all, normal, bold, 100, 200, 300, 400, 500, 600, 700, 800, 900}; default = all */
	UInt32					fontStretch;			/* OR of {all, normal, {ultra-,extra-,semi-,}{condensed,expanded}}; default = normal */
	char					*fontSize;				/* Strings describing sizes */
	FskUnicodeRange			*unicodeRange;			/* 0-terminated array. NULL ==> default = [0, 0x10FFFF] */
	UInt32					unitsPerEm;				/* This is ignored; we assume 65536 units per em; can be used to store original value */
	UInt8					panose1[10];			/* default = { 0,0,0,0,0,0,0,0,0,0 } */
	UInt8					unused[2];				/* pad */
	FskFixed				stemV;					/* width of vertical stems;   default = 0 ==> undefined */
	FskFixed				stemH;					/* width of horizontal stems; default = 0 ==> undefined */
	FskFixed				slope;					/* vertical stroke angle, in counterclockwise degrees; usually non-positive; default = 0 */
	FskFixed				capHeight;				/* uppercase height */
	FskFixed				xHeight;				/* lowercase height */
	FskFixed				accentHeight;			/* default = ascent */
	FskFixed				ascent;					/* default = unitsPerEm - vertOriginY */
	FskFixed				descent;				/* default = vertOriginY */
	char					*widths;				/* e.g. "U+4E00-4E1F 1736 1874 1692, U+1A?? 1490, U+215? 1473 1838 1927 1684 1356 1792 1815 1848 1870 1492 1715 1745 1584 1992 1978 1770 */
	FskFixedRectangleRecord	bbox;					/* Union of the bounding boxes of all glyphs */
	FskFixed				ideographicBaseline;	/* Ideographic baseline */
	FskFixed				alphabeticBaseline;		/* Bottom baseline for Latin, Greek, and Cyrillic; top baseline for Sanscrit */
	FskFixed				mathematicalBaseline;	/* Baseline for mathematical typesetting */
	FskFixed				hangingBaseline;		/* Hanging baseline */
	FskFixed				underlinePosition;
	FskFixed				underlineThickness;
	FskFixed				strikethroughPosition;
	FskFixed				strikethroughThickness;
	FskFixed				overlinePosition;
	FskFixed				overlineThickness;
#ifdef NOT_IN_SVG_TINY
	FskFixed				vIdeographic;			/* Ideographic baseline for vertically-oriented text */
	FskFixed				vAlphabetic;			/* Alphabetic baseline for vertically-oriented text */
	FskFixed				vMathematical;			/* Mathematical baseline for vertically-oriented text */
	FskFixed				vHanging;				/* Hanging baseline for vertically-oriented text */
#endif /* NOT_IN_SVG_TINY */
} FskFontFaceSpec;


/* Font initializer.
 * Note that the units here are in ems, not in the font design coordinate system specified by unitsPerEm;
 * conversion occurs by division by unitsPerEm.
 */
typedef struct FskFontSpec {
	FskFixed				horizOriginX;				/* default = 0 */
	FskFixed				horizOriginY;				/* default = 0 */
	FskFixed				horizAdvX;
	FskKernSpec				*hKern;						/* Array terminated with k=0 */

	FskPath					missingGlyphPath;			/* Graphics to draw when the glyph is missing */
	FskFixed				missingGlyphHorizAdvanceX;	/* The horizontal advance after rendering the character in the X direction */

#ifdef NOT_IN_SVG_TINY
	FskFixed				missingGlyphVertOriginX;	/* The X-coordinate in the font coordinate system of the origin of the glyph to be used when drawing vertically oriented text (not in tiny) */
	FskFixed				missingGlyphVertOriginY;	/* The Y-coordinate in the font coordinate system of the origin of the glyph to be used when drawing vertically oriented text (not in tiny) */
	FskFixed				missingGlyphVertAdvanceY;	/* The vertical advance after rendering a glyph in vertical orientation (not in tiny) */
	FskFixed				vertOriginX;				/* The X-coordinate in the font coordinate system of the origin of the glyph to be used when drawing vertically oriented text */
	FskFixed				vertOriginY;				/* The Y-coordinate in the font coordinate system of the origin of the glyph to be used when drawing vertically oriented text */
	FskFixed				vertAdvanceY;				/* The vertical advance after rendering a glyph in vertical orientation */
	FskKernSpec				*vKern;		`				/* Array terminated with k=0 */
#endif /* NOT_IN_SVG_TINY */

	FskFontFaceSpec			face;

} FskFontSpec;



/********************************************************************************
 ********************************************************************************
 ********************************************************************************
 *****							Font Collection API							*****
 ********************************************************************************
 ********************************************************************************
 ********************************************************************************/


						/* Constructor */
FskAPI(FskErr)			FskFontCollectionNew(FskFontCollection *fc);

						/* Destructor */
FskAPI(void)			FskFontCollectionDispose(FskFontCollection fc);

						/* Get a particular font from the collection */
FskAPI(FskConstFont)	FskFskFontCollectionFindFont(FskConstFontCollection fc, const FskFontAttributes *attributes);

						/* Generate a path for a given character string and font specification */
FskAPI(FskErr)			FskFontCollectionNewGrowablePathFromUnicodeString(
							FskConstFontCollection	fc,
							const UInt16			*uniChars,
							UInt32					numUniChars,
							const FskFontAttributes	*attributes,
							FskFixedPoint2D			*origin,
							FskGrowablePath			*path
						);



/********************************************************************************
 ********************************************************************************
 ********************************************************************************
 *****								Font  API								*****
 ********************************************************************************
 ********************************************************************************
 ********************************************************************************/


					/* Call this before setting parameters in FskFontSpec */
FskAPI(void)		FskFontSpecSetDefaults(FskFontSpec *fs);

					/* Call this to create a new font and manage it in our collection */
FskAPI(FskErr)		FskFontCollectionAddNewFont(const FskFontSpec *fs, FskFontCollection fc, FskFont *ft);

					/* Don't call this directly; it is called from FskFontCollectionAddNewFont() */
FskErr				FskFontNew(const FskFontSpec *fs, FskFont *ft);

					/* Don't call this directly; it is called from FskFontCollectionDispose() */
void				FskFontDispose(FskFont ft);

					/* Add a glyph to a given font */
FskAPI(FskErr)		FskFontAddGlyph(
						FskFont			font,
						UInt16			*uniChar,		/* Usually a single character, but may be a ligature (CANNOT be NULL) */
						UInt8			*name,			/* The name of the glyph (can be NULL) */
						FskConstPath	path,			/* The path of the glyph, with 1.0 units per em (CANNOT be NULL) */
						UInt8			*langCode,		/* The language codes, comma-separated (can be NULL) */
						UInt8			orientation,	/* H or V */
						UInt8			arabicForm,		/* Initial, medial, terminal, isolated */
						FskFixed		horizAdvX		/* The horizontal advance after rendering the character in the X direction */
					#ifdef NOT_IN_SVG_TINY
					,	FskFixed		vertOriginX,	/* The X-coordinate in the font coordinate system of the origin of the glyph to be used when drawing vertically oriented text (not in tiny) */
						FskFixed		vertOriginY,	/* The Y-coordinate in the font coordinate system of the origin of the glyph to be used when drawing vertically oriented text (not in tiny) */
						FskFixed		vertAdvanceY	/* The vertical advance after rendering a glyph in vertical orientation (not in tiny) */
					#endif /* NOT_IN_SVG_TINY */
					);

					/* Call this after all of the glyphs have been added, in order to optimize the font */
FskAPI(FskErr)		FskFontDoneEditing(FskFont font);

					/* Convert a Unicode string into a glyph string */
FskAPI(FskErr)		FskFontNewGlyphStringFromUnicodeString(FskConstFont font, const UInt16 *uniChars, UInt32 numUniChars, UInt16 **pGlyphs, UInt32 *pNumGlyphs);

					/* Get the path for a given glyph */
FskAPI(FskErr)		FskFontGetGlyphPath(FskConstFont font, UInt16 glyph, FskConstPath *path, FskFixed *hAdvance);

					/* Append the scaled path of the glyph to a growable path, and update the origin */
FskAPI(FskErr)		FskFontAppendGlyphPathToGrowablePath(FskConstFont font, UInt16 glyph, FskFixed size, FskFixedPoint2D *origin, FskGrowablePath path);


/* Convenience functions */
void				FskKernSpecDispose(FskKernSpec *ks);
void				FskFontSpecCleanup(FskFontSpec *fs);
void				FskFontSpecDispose(FskFontSpec *fs);



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __FSKFONT__ */


