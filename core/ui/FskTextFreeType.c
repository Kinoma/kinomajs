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
#define __FSKBITMAP_PRIV__
#define __FSKTEXT_PRIV__
#define FSK_FREETYPE_KINOMA_ALLOCATOR 1

#include "FskMemory.h"
#include "FskText.h"

#include "FskAssociativeArray.h"
#include "FskEdgeEnhancedText.h"
#include "FskExtensions.h"
#include "FskPixelOps.h"
#include "FskTextConvert.h"
#include "FskTransferAlphaBitmap.h"
#include "FskUtilities.h"
#include "FskThread.h"


#include "FskEnvironment.h"
#include "FskFiles.h"
#include "expat.h"

#define SCAN_DEBUG 0
#if SCAN_DEBUG
	static void sFskFTFontScan(char *fontPath);
#endif

#define HEIGHT_ADJUST_PX	1

#define ROUND_TEXTSIZE_DOWN	0

#define USE_RECTS 0
#if USE_RECTS
int RECT_WIDTH = 24;
int RECT_HEIGHT = 24;
#endif

#if 1
#define mulFix	FskFixMul
#else
#define mulFix	FT_MulFix
#endif

#define ROUND_OFF_BITS(x, n)	(((x) + (1 << ((n) - 1))) >> (n))			/**< Downshift n bits, with rounding. */
#define CEIL_OFF_BITS(x, n)		(((x) + (1 << (n)) - 1) >> (n))				/**< Downshift n bits, with ceil. */
#define FT_FRACTIONAL_BITS		6											/**< Sixty-fourths of pixel resolution. */
#define FT_TRUNC_TO_INT(x)		((x) >> FT_FRACTIONAL_BITS)					/**< Shift out the extra FT_FRACTIONAL_BITS. */
#define FT_ROUND_TO_INT(x)		ROUND_OFF_BITS((x), FT_FRACTIONAL_BITS)		/**< Truncate after adding 1/2. */
#define FT_CEIL_TO_INT(x)		CEIL_OFF_BITS((x), FT_FRACTIONAL_BITS)		/**< Truncate after adding 1 - epsilon. */
#define FT_AA_AURA				(0x10 << (FT_FRACTIONAL_BITS - 4))			/**< Freetype antialiasing aura. We assume that it is 1.0 pixels. */
#define INT_TO_FT(x)			((x) << FT_FRACTIONAL_BITS)					/**< Convert an integer to 26.6 fixed point, as used by FreeType. */
#define OBLIQUE_SKEW			0x6000L										/**< 0.375 = 3/8 */
#define FIXED_HALF				(1 << (16 - 1))								/**< One-half in fixed point. */

static FskErr freeTypeNew(FskTextEngineState *state);
static FskErr freeTypeDispose(FskTextEngineState state);
static FskErr freeTypeFormatCacheNew(FskTextEngineState state, FskTextFormatCache *cache, FskBitmap bits, UInt32 textSize, UInt32 textStyle, const char *fontName);
static FskErr freeTypeFormatCacheDispose(FskTextEngineState state, FskTextFormatCache cache);
static FskErr freeTypeBox(FskTextEngineState state, FskBitmap bits, const char *text, UInt32 textLen, FskConstRectangle bounds, FskConstRectangle clipRect, FskConstColorRGBA color, UInt32 blendLevel, UInt32 textSize, UInt32 textStyle, UInt16 hAlign, UInt16 vAlign, const char *fontName, FskTextFormatCache cache);
static FskErr freeTypeGetBounds(FskTextEngineState state, FskBitmap bits, const char *text, UInt32 textLen, UInt32 textSize, UInt32 textStyle, const char *fontName, FskRectangle bounds, FskTextFormatCache cache);
static FskErr freeTypeGetFontInfo(FskTextEngineState state, FskTextFontInfo info, const char *fontName, UInt32 textSize, UInt32 textStyle, FskTextFormatCache formatCache);
static FskErr freeTypeFitWidth(FskTextEngineState state, FskBitmap bits, const char *text, UInt32 textLen, UInt32 textSize, UInt32 textStyle, const char *fontName, UInt32 width, UInt32 flags, UInt32 *fitBytes, UInt32 *fitChars, FskTextFormatCache cache);
static FskErr freeTypeAddFontFile(FskTextEngineState state, const char *path);
static FskErr freeTypeGetFontList(FskTextEngineState state, char **fontNames);
static FskErr freeTypeDefaultFontSet(FskTextEngineState state, const char *defaultFace);
static FskErr freeTypeSetFamilyMap(FskTextEngineState state, const char *map);
static FskErr freeTypeGetLayout(FskTextEngineState state, FskBitmap bits, const char *text, UInt32 textLen, UInt32 textSize, UInt32 textStyle, const char *fontName, UInt16 **unicodeText, UInt32 *unicodeLen, FskFixed **layout, FskTextFormatCache cache);

static FskErr freeTypeSetZoom(void *stateIn, void *track, UInt32 propertyID, FskMediaPropertyValue property);
static FskErr freeTypeGetFractionalTextSize(void *stateIn, void *track, UInt32 propertyID, FskMediaPropertyValue property);

FskMediaPropertyEntryRecord freeTypeProperties[] = {
	{kFskTextEnginePropertyZoom,                    kFskMediaPropertyTypeFloat,			NULL,                               freeTypeSetZoom},
	{kFskTextEnginePropertyFractionalTextSize,		kFskMediaPropertyTypeBoolean,		freeTypeGetFractionalTextSize,		NULL},
	{kFskMediaPropertyUndefined,                    kFskMediaPropertyTypeUndefined,		NULL,                               NULL}
};

static FskTextEngineDispatchRecord gFskTextFreeTypeDispatch = {
	"FreeType",
	freeTypeNew,
	freeTypeDispose,
	freeTypeFormatCacheNew,
	freeTypeFormatCacheDispose,
	freeTypeBox,
	freeTypeGetBounds,
	freeTypeGetFontInfo,
	freeTypeFitWidth,
	freeTypeGetLayout,
	freeTypeAddFontFile,
	freeTypeDefaultFontSet,
	freeTypeGetFontList,
	freeTypeSetFamilyMap,
	freeTypeProperties
};

FskErr FskTextFreeTypeInitialize(void);
void FskTextFreeTypeUninitialize(void);

#include <errno.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_CACHE_H

#include FT_CACHE_IMAGE_H
#include FT_CACHE_SMALL_BITMAPS_H
#include FT_CACHE_CHARMAP_H
#include FT_TRUETYPE_IDS_H

#if FSK_FREETYPE_KINOMA_ALLOCATOR
	#include FT_MODULE_H
	static FT_Memory ftmem;
#endif

#define gForceAntiAliasFont (true)
#define kBoldRepeatFactor	17

typedef struct  {
	FT_Fixed		zoom;
} FskTextEngineFreeTypeRecord, *FskTextEngineFreeType;

typedef struct FskFTParserDataStruct FskFTParserDataRecord, *FskFTParserData;
typedef struct FskFTMappingStruct FskFTMappingRecord, *FskFTMapping;
typedef struct FskFTMappingWeightStruct FskFTMappingWeightRecord, *FskFTMappingWeight;
typedef struct FskFTFamilyStruct FskFTFamilyRecord, *FskFTFamily;
typedef struct FskFTFaceStruct FskFTFaceRecord, *FskFTFace;
typedef struct FskFTAliasStruct FskFTAliasRecord, *FskFTAlias;
typedef struct FskFTSizeStruct FskFTSizeRecord, *FskFTSize;
typedef struct FskFTGlyphStruct FskFTGlyphRecord, *FskFTGlyph;

struct FskFTMappingWeightStruct {
	char* name;
	UInt32 weight;
};

struct FskFTParserDataStruct {
	FskFTMapping mapping;
	FskFTAlias family;
	FskFTAlias alias;
	FskFTAlias language;
	char* value;
};

struct FskFTMappingStruct {
	FskFTFamily family;
	FskFTAlias system;
	FskFTAlias alias;
	FskFTAlias fallback;
	FskFTAlias language;
	FskFTFace currentFace;
	FskFTFace fallbackFace;
	FskAssociativeArray familyCache;
};

struct FskFTFamilyStruct {
	FskFTFamily next;
	FskFTFace face;
	char* name;
};

struct FskFTFaceStruct {
	FskFTFace	next;
	UInt32		style;
	UInt32		weight;
	UInt32		faceIndex;	// a font file can have many faces
	Boolean		scalable;
	UInt16		numFixedSizes;
	FskFTSize	fixedSizes;
	FT_FaceRec 	*ftFace;
	int			CMAPIndex;
	UInt32		spaceIndex;
	FT_Pos		spaceWidth;	// width of a space (-1 if unknown)
	SInt32	 	spaceKern;
	FT_Size	 	ftSize;
	FTC_ScalerRec scaler;
	FTC_ImageTypeRec font;
	SInt32	 	size;
	char		path[1];
};

struct FskFTAliasStruct {
	FskFTAlias next;
	FskFTFamily family;
	char* name;
};

struct FskFTSizeStruct {
	UInt16 height;
	UInt16 width;
	UInt32 pointSize;		// 26.6 fixed point
};


struct FskFTGlyphStruct {
	int		index;
	int		chars;
	Boolean discard;
	Boolean fallback;
	Boolean missing;
	FT_Glyph	image;
	UInt32		uc;
	UInt32		advance;
	FskFTFace	face;
};

typedef struct {
	FskFTFace fFace;
	Boolean			haveEllipsisWidth;
	FskFTGlyph 			ellipsisStrike;
} FskTextFormatCacheFTRecord, *FskTextFormatCacheFT;

static FskErr FskFTMappingNew(FskFTMapping *it);
static void FskFTMappingDispose(FskFTMapping mapping);
static FskErr FskFTMappingAddFace(FskFTMapping mapping, const char* name, const char* style, FskFTFace face);
static FskErr FskFTMappingFromFile(FskFTMapping mapping, const char* path);
static FskErr FskFTMappingLookup(FskFTMapping mapping, FskFTGlyph glyph, FskFTFace face, UInt32 unicode);
static void FskFTMappingPrint(FskFTMapping mapping);
static FskErr FskFTMappingSetCurrentFace(FskFTMapping mapping, FskFTFace face);


static FskMutex gFTMutex;

static char *gFontFamilyMap = NULL;

static FT_Library      gFTLibrary;			// global
static FTC_Manager     gFTCacheManager = NULL;			// global
static FTC_ImageCache  gFTImageCache;
static FTC_SBitCache   gFTSbitsCache;
static FTC_CMapCache   gFTCmapCache;

static FskFTMapping gFskFTMapping = NULL;
static FskFTMappingWeightRecord gFskFTMappingWeight[] =
	{
		{	"Thin",			100	},
		{	"ExtraLight",	200	},
		{	"UltraLight",	200	},
		{	"Light",		300	},
		{	"Normal",		400	},
		{	"Regular",		400	},
		{	"Medium",		500	},
		{	"DemiBold",		600	},
		{	"SemiBold",		600	},
		{	"ExtraBold",	800	},
		{	"UltraBold",	800	},
		{	"Black",		900	},
		{	"Heavy",		900	},
		{	"ExtraBlack",	950	},
		{	"UltraBlack",	950	},
		{	"Bold",			700	},
		{	NULL,			0	}
	};

enum {
	kFskFreeTypeNoMaxWidth = 0xffffffff
};

static void sFTStrikeDispose(FskFTGlyph strike);

FT_CALLBACK_DEF(FT_Error) _FTFaceRequester(FTC_FaceID  theFaceID,
	FT_Library  theLibrary,
	FT_Pointer  theData,
	FT_Face*    theFace)
{
	FskErr err = kFskErrNone;
	FskFTFace face = (FskFTFace)theFaceID;
	FT_UNUSED(theData);

	BAIL_IF_ERR(err = FT_New_Face(theLibrary, face->path, face->faceIndex, theFace));
	BAIL_IF_ERR(err = FT_Select_Charmap(*theFace, FT_ENCODING_UNICODE));
	face->CMAPIndex = FT_Get_Charmap_Index((*theFace)->charmap);
bail:
	return err;
}

typedef struct xFTLibRec
{
    FT_Memory          memory;           /* library's memory manager */

    FT_Generic         generic;

    FT_Int             version_major;
    FT_Int             version_minor;
    FT_Int             version_patch;

    FT_UInt            num_modules;
} xFTLibRec, *xFTLib;


FskInstrumentedSimpleType(FreeType, freetype);

/********************************************************************************
 * sFskFTFindFont
 ********************************************************************************/

void sFskFTFaceUpdateScaler(FskFTFace fFace, UInt32 textSize) {
#if USE_RECTS
	return;
#else
	if (fFace && (fFace->size != textSize)) {
	    UInt32 integerTextSize = textSize;											/* Assume that the text size is given as an integer, ... */
	    if (textSize > FIXED_HALF)													/* ... unless the textSize is unbelievably huge, ... */
	        integerTextSize = FskRoundFixedToInt(integerTextSize);					/* ... when it is interpreted as 16.16 fixed-point instead */

		fFace->spaceWidth = -1;
        fFace->font.width  =
        fFace->font.height = (FT_UShort)integerTextSize;
        fFace->size = textSize;
        fFace->ftSize = NULL;

        if (textSize > FIXED_HALF) {												/* If the text size is specified in 16.16 fixed-point, ... */
            fFace->scaler.width =
            fFace->scaler.height = (FT_Int)FskFixMul(textSize, 64);
        }
        else {
            fFace->scaler.width =
            fFace->scaler.height = (FT_Int)(textSize * 64);
        }
	}
	return;
#endif
}

FT_Size sFskFTFaceGetSize(FskFTFace face) {
#if USE_RECTS
	return NULL;
#else
	FskErr err = kFskErrNone;
	FT_Size ftSize = NULL;
	BAIL_IF_ERR(err = FskFTMappingSetCurrentFace(gFskFTMapping, face));
	ftSize = face->ftSize;
bail:
	return ftSize;
#endif
}

static FskFTFace sFskFTFindFont(const char *familyName, UInt32 textStyle, UInt32 textSize, FskTextFormatCache formatCache) {
#if USE_RECTS
	RECT_WIDTH = textSize;
	RECT_HEIGHT = textSize;
	return NULL;
#else
	FskFTFamily family = NULL;
	FskFTFace face = NULL;
	FskFTFace closest = NULL;
	UInt32 weight, i;

#if ROUND_TEXTSIZE_DOWN
	UInt32 newTextSize;
	newTextSize = (textSize >> 1) << 1;
//	FskFreeTypePrintfDebug("textSize was %d now %d", textSize, newTextSize);
	textSize = newTextSize;
#endif
	FskMutexAcquire(gFTMutex);			// caller is responsible for calling release, if this call succeeds

	if (formatCache)
		closest = ((FskTextFormatCacheFT)formatCache)->fFace;
	else if (gFskFTMapping) {
		FskFTAlias alias;
		// look in family cache
		family = FskAssociativeArrayElementGetReference(gFskFTMapping->familyCache, familyName);
		if (family)
			goto gotFamily;
		// look for installed font
		for (family = gFskFTMapping->family; family; family = family->next) {
			if (!FskStrCompareCaseInsensitive(familyName, family->name))
				goto gotFamily;
		}
		// look for system font
		for (alias = gFskFTMapping->system; alias; alias = alias->next) {
			if (!FskStrCompareCaseInsensitive(familyName, alias->name)) {
				family = alias->family;
				if (family)
					goto gotFamily;
			}
		}
		// look for aliases font
		for (alias = gFskFTMapping->alias; alias; alias = alias->next) {
			if (!FskStrCompareCaseInsensitive(familyName, alias->name)) {
				family = alias->family;
				if (family)
					goto gotFamily;
			}
		}
		// then try matching through the family map table
		if (NULL != gFontFamilyMap) {
			char *p;

			for (p = gFontFamilyMap; 0 != *p; ) {
				// FskFreeTypePrintfDebug("Matching font family map \"%s\" with \"%s\"", p, familyName);
				if (FskStrCompareCaseInsensitive(p, familyName) == 0) {
					p += FskStrLen(p) + 1;
					for (family = gFskFTMapping->family; family; family = family->next) {
						// FskFreeTypePrintfDebug("Matching font map family \"%s\" with \"%s\"", family->familyName, p);
						if (!FskStrCompareCaseInsensitive(p, family->name))
							goto gotFamily;
					}
					return NULL;
				}
				p += FskStrLen(p) + 1;
				p += FskStrLen(p) + 1;
			}
		}
		// take first system font
		family = gFskFTMapping->system ? gFskFTMapping->system->family : NULL;
gotFamily:
		if (family) {
			// add to family cache
			FskAssociativeArrayElementSetReference(gFskFTMapping->familyCache, familyName, family);
			// get the closest face
			weight = textStyle & kFskTextBold ? 700 : 400;
			for (face = family->face; face; face = face->next) {
				if (!closest && face->scalable)
					closest = face;
				if (face->style == textStyle) {
					if (face->scalable) {
						if (face->weight == weight)
							goto gotFace;
						else
							closest = face;
					}
					else if (face->numFixedSizes) {
						closest = face;
						for (i = 0; i < face->numFixedSizes; i++) {
							if (textSize == FT_TRUNC_TO_INT(face->fixedSizes[i].pointSize)) {
								if (face->weight == weight)
									goto gotFace;
							}
						}
					}
				}
			}
		}
	}
	if (closest)
		face = closest;
	else
		FskMutexRelease(gFTMutex);
gotFace:
	if (face)
		sFskFTFaceUpdateScaler(face, textSize);
	return face;
#endif
}

/********************************************************************************
 * freeTypeAddFontFile
 ********************************************************************************/

FskErr freeTypeAddFontFile(FskTextEngineState state, const char *fontFullPath)
{
	FskErr err = kFskErrNone;
	SInt32 faceIter = 0;
	char *p;

	FskFreeTypePrintfDebug("freeTypeAddFontFile %s", fontFullPath);
	p = FskStrRChr(fontFullPath, '/');
	if (p) {
		p++;
		if (0 == FskStrCompare(p, "DroidSans_Subset.ttf")) {
			FskFreeTypePrintfDebug("Don't use DroidSans_Subset.ttf %s", p);
			return kFskErrNone;
		}
	}

	while (true) {
		FT_FaceRec *ftFace;

		err = FT_New_Face(gFTLibrary, fontFullPath, faceIter, &ftFace);
		if (!err) {
			FskFTFace fFace = NULL;

			FskFreeTypePrintfDebug(" -- family_name: %s, style: %x, style_name: %s\n", ftFace->family_name, ftFace->style_flags, ftFace->style_name);
			err = FskMemPtrNewClear(sizeof(FskFTFaceRecord) + FskStrLen(fontFullPath) + 1, &fFace);
			if (err)
				goto bailFont;

			// initialize face FTC_ImageTypeRec structure
			fFace->font.face_id = (FTC_FaceID)fFace;
            fFace->font.width = fFace->font.height = 0;
			fFace->font.flags = gForceAntiAliasFont ? FT_LOAD_DEFAULT : FT_LOAD_TARGET_MONO;
			fFace->font.flags |= FT_LOAD_NO_HINTING /* FT_LOAD_FORCE_AUTOHINT */;

			fFace->scaler.face_id = (FTC_FaceID)fFace;
			fFace->scaler.pixel = 0;
			fFace->scaler.x_res = fFace->scaler.y_res = 70;

			fFace->faceIndex = faceIter;
			fFace->style = (ftFace->style_flags & FT_STYLE_FLAG_BOLD) ? kFskTextBold : 0;
			fFace->style |= (ftFace->style_flags & FT_STYLE_FLAG_ITALIC) ? kFskTextItalic : 0;
			fFace->weight = fFace->style & kFskTextBold ? 700 : 400;
			if (NULL != ftFace->style_name) {
				UInt32 i = 0;
				FskFTMappingWeightRecord weightMap;
				for (weightMap = gFskFTMappingWeight[i++]; weightMap.name; weightMap = gFskFTMappingWeight[i++]) {
					if (FskStrStr(ftFace->style_name, weightMap.name)) {
						// mark face as not regular or normal
						fFace->weight = weightMap.weight;
						if (fFace->weight >= 700)
							fFace->style |= kFskTextBold;
						break;
					}
				}
				if (FskStrStr(ftFace->style_name, "Italic")) {
					// mark face as not regular or normal
					fFace->style |= kFskTextItalic;
				}
			}

			FskStrCopy(fFace->path, fontFullPath);
			fFace->scalable = ftFace->num_fixed_sizes == 0;
			fFace->numFixedSizes = ftFace->num_fixed_sizes;
			if (ftFace->num_fixed_sizes) {
				int j;
				err = FskMemPtrNew(ftFace->num_fixed_sizes * sizeof(FskFTSizeRecord), &fFace->fixedSizes);
				if (err)
					goto bailFont;
				for (j=0; j<ftFace->num_fixed_sizes; j++) {
					fFace->fixedSizes[j].height = ftFace->available_sizes[j].height;
					fFace->fixedSizes[j].width = ftFace->available_sizes[j].width;
					fFace->fixedSizes[j].pointSize = ftFace->available_sizes[j].size;
				}
			}

			FskFTMappingAddFace(gFskFTMapping, ftFace->family_name, ftFace->style_name, fFace);

			faceIter++;
			if (faceIter >= ftFace->num_faces)
				break;
			FT_Done_Face(ftFace);
			continue;		// to next face iteration for this file
bailFont:
			FT_Done_Face(ftFace);
			if (fFace) {
				FskMemPtrDispose(fFace->fixedSizes);
				FskMemPtrDispose(fFace);
			}
			// FskMemPtrDispose(fFamily);
			goto bail;
		}
		break;				// out of this font file
	}

bail:
	return err;
}

/********************************************************************************
 * convertToUnicode
 ********************************************************************************/

static ftUTF8Sequence gUTF8Sequences[] = {
    {1, 0x80, 0x00, 0*6, 0x0000007F, 0x00000000},
    {2, 0xE0, 0xC0, 1*6, 0x000007FF, 0x00000080},
    {3, 0xF0, 0xE0, 2*6, 0x0000FFFF, 0x00000800},
    {4, 0xF8, 0xF0, 3*6, 0x001FFFFF, 0x00010000},
    {5, 0xFC, 0xF8, 4*6, 0x03FFFFFF, 0x00200000},
    {6, 0xFE, 0xFC, 5*6, 0x7FFFFFFF, 0x04000000},
    {0, 0, 0, 0, 0, 0},
};

static UInt32 convertToUnicode(UInt32 *uc, char *in, UInt32 maxIn)
{
    ftUTF8Sequence *aSequence;
    unsigned char *aString = (unsigned char *)in;
    UInt32 aResult = *aString++;
    UInt32 aSize;

	if (0 == (0x80 & aResult)) {
		*uc = aResult;
		return 1;
	}

    for (aSequence = gUTF8Sequences; aSequence->size; aSequence++) {
        if ((aResult & aSequence->cmask) == aSequence->cval)
            break;
    }

	if (0 == aSequence->size)
		aResult = 0;
	else {
		aSize = aSequence->size - 1;
		while (aSize) {
			aSize--;
			aResult = (aResult << 6) | (*aString++ & 0x3F);
		}
		aResult &= aSequence->lmask;
	}
	*uc = aResult;
    return aSequence->size;
}

/********************************************************************************
 * sFTGetStrikeBBox
 ********************************************************************************/

void sFTGetStrikeBBox(FskTextEngineFreeType state, FskFTFace fFace, FskFTGlyph strike, FskRectangle bounds)
{
	FskFTGlyph glyph;
	FT_Size ftSize = NULL;

	ftSize = sFskFTFaceGetSize(fFace);

	bounds->x = 0;
	bounds->y = 0;
	bounds->width = 0;
	bounds->height = ftSize->metrics.ascender - ftSize->metrics.descender;

	for (glyph = strike; glyph->index >= 0; glyph++)
		bounds->width += glyph->advance;

	bounds->width = FT_ROUND_TO_INT(bounds->width);
	bounds->height = FT_ROUND_TO_INT(FT_MulFix(bounds->height, state->zoom)) + HEIGHT_ADJUST_PX;
}

/********************************************************************************
 * sFTStrikeGetGlyphBitmap
 ********************************************************************************/

FskErr sFTStrikeGetGlyphBitmap(FskFTGlyph theGlyph, FskPoint thePoint, FskBitmap theBitmap)
{
#if USE_RECTS
	return kFskErrNone;
#else
	FskErr err = kFskErrNone;
	long found = false;

	if (theGlyph->image == NULL) {

		FTC_SBit  aSBit = NULL;

		BAIL_IF_ERR(err = FTC_SBitCache_Lookup(gFTSbitsCache, &theGlyph->face->font, theGlyph->index, &aSBit, NULL));

		found = aSBit->buffer != 0;
		if (found) {

			found = aSBit->width && aSBit->height;
			thePoint->x += aSBit->left;
			thePoint->y -= aSBit->top;
			theBitmap->bounds.width = aSBit->width;
			theBitmap->bounds.height = aSBit->height;
			theBitmap->rowBytes = aSBit->pitch;
			theBitmap->bits = aSBit->buffer;
		}
	}
	else {
		FT_BitmapGlyph aBitmap;
		FT_Bitmap* aSource;

		aBitmap = (FT_BitmapGlyph)theGlyph->image;
		aSource = &aBitmap->bitmap;
		found = aSource->width && aSource->rows;
		thePoint->x += aBitmap->left;
		thePoint->y -= aBitmap->top;
		theBitmap->bounds.width = aSource->width;
		theBitmap->bounds.height = aSource->rows;
		theBitmap->rowBytes = aSource->pitch;
		theBitmap->bits = aSource->buffer;
	}
	return found;
bail:
	return false;
#endif
}

/********************************************************************************
 * sFTStrikeGetGlyphInfo
 ********************************************************************************/

FskErr sFTStrikeGetGlyphInfo(FskFTGlyph theGlyph, Boolean needImage, Boolean doOblique, FT_Fixed zoom)
{
#if USE_RECTS
	return kFskErrNone;
#else
	FskErr err = kFskErrNone;
	long found = false;
	Boolean zoomed = 0x010000L != zoom;
	FskFTFace fFace = theGlyph->face;

	theGlyph->image = NULL;
	if ((fFace->font.width  < 48) &&
		(fFace->font.height < 48) &&
		(false == doOblique) &&
		(false == zoomed)) {
		FTC_SBit  aSBit = NULL;

		BAIL_IF_ERR(err = FTC_SBitCache_Lookup(gFTSbitsCache, &fFace->font, theGlyph->index, &aSBit, NULL));

		found = aSBit->buffer != 0;
		if (found)
			theGlyph->advance = aSBit->xadvance << FT_FRACTIONAL_BITS;		// this works because apparently we aren't getting fractional metrics _ever_ from FreeType...
	}

	if (!found) {
		FT_Glyph aGlyph;

		// FskFreeTypePrintfDebug("prior to ImageCache Lookup");
		BAIL_IF_ERR(err = FTC_ImageCache_Lookup(gFTImageCache, &fFace->font, theGlyph->index, &aGlyph, NULL));

		if (false == zoomed)
			theGlyph->advance = ROUND_OFF_BITS(aGlyph->advance.x, 16-FT_FRACTIONAL_BITS);					/* 16 to 6 fracbits */
		else
			theGlyph->advance = ROUND_OFF_BITS(FT_MulFix(zoom, aGlyph->advance.x), 16-FT_FRACTIONAL_BITS);	/* 16 to 6 fracbits */

		if (needImage) {
			if (aGlyph->format != FT_GLYPH_FORMAT_BITMAP) {
				BAIL_IF_ERR(err = FT_Glyph_Copy(aGlyph, &aGlyph));

				if (doOblique || zoomed) {
					FT_Matrix transform;

					transform.xx = zoom;
					transform.yx = 0;
					transform.xy = doOblique ? OBLIQUE_SKEW : 0;						/* 0x6000 ~ 0.375 = 3/8 skew */
					transform.yy = zoom;
					FT_Glyph_Transform(aGlyph, &transform, NULL);						/* NOTE bbox is enlarged by height * 3/8 */
				}
				// FskFreeTypePrintfDebug("prior to Glyph To Bitmap");
				BAIL_IF_ERR(err = FT_Glyph_To_Bitmap(&aGlyph, FT_RENDER_MODE_NORMAL, NULL, 1));
				theGlyph->discard = true;
			}
			theGlyph->image = aGlyph;

			if (aGlyph->format != FT_GLYPH_FORMAT_BITMAP)
				BAIL(FT_Err_Invalid_Glyph_Format);
		}
	}
bail:
	if (err) {
		FskFreeTypePrintfDebug("sFTStrikeGetGlyphInfo - fail %d - index: %d uc: %u", theGlyph->index, err, (unsigned)theGlyph->uc);
	}
	return err;
#endif
}

/********************************************************************************
 * sFTStrikeNew
 ********************************************************************************/

FskErr sFTStrikeNew(FskTextEngineFreeType state, FskFTGlyph *strikeOut, FskFTFace fFace, const char *text, UInt32 textLen, UInt32 textStyle, Boolean needImage, UInt32 maxWidth)
{
#if USE_RECTS
	return kFskErrNone;
#else
	FskErr err = kFskErrNone;
	FT_Bool			use_kerning;
	FskFTGlyph			strike, glyph;
	int				horizRepeat = 1, aCount;
	UInt32			n;
	Boolean			doOblique = false;
	UInt32			totalWidth = 0;
	FT_Size ftSize = NULL;

	*strikeOut = NULL;
	if (kFskErrNone != FskMemPtrNewClear(sizeof(FskFTGlyphRecord) * (textLen + 1), &strike))
		return kFskErrMemFull;

	strike[textLen].index = -1;		// set sentinel

	if (textStyle & kFskTextBold) {
		if (!(fFace->style & kFskTextBold)) {
			horizRepeat = horizRepeat < 2 ? 2 : horizRepeat;		/* TODO: This fake emboldening enlarges the bonding box */
			FskFreeTypePrintfDebug("strikeNew synthetic Bold");
		}
	}

	doOblique = (textStyle & kFskTextItalic) && !(fFace->style & kFskTextItalic);	/* If we don't have italic but are asked for it we fake it by obliqueing */
	#if SUPPORT_INSTRUMENTATION
		if (doOblique)
			FskFreeTypePrintfDebug("strikeNew synthetic oblique instead of italic");
	#endif /* SUPPORT_INSTRUMENTATION */

	BAIL_IF_ERR(err = FskFTMappingSetCurrentFace(gFskFTMapping, fFace));

	use_kerning = (FT_Bool)FT_HAS_KERNING(fFace->ftFace);

	for (n = 0, glyph = strike, aCount = 0; (n < textLen) && (FT_TRUNC_TO_INT(totalWidth) <= maxWidth); aCount++) {
		int chars;
		UInt32 uc;

		glyph->index = -1;
		glyph->face = fFace;

		chars = convertToUnicode(&uc, (char *)(text + n), textLen - n);
		if (0 == chars) {
			BAIL(kFskErrBadData);
		}
		n += chars;
		glyph->fallback = false;
		glyph->chars = chars;
		glyph->uc = uc;
		if (uc <= 32) { // process control characters
			if (fFace->spaceWidth == -1) {
				glyph->index = FTC_CMapCache_Lookup(gFTCmapCache, (FTC_FaceID)fFace, fFace->CMAPIndex, 32);
				BAIL_IF_ERR(err = sFTStrikeGetGlyphInfo(glyph, needImage, doOblique, state->zoom));
				fFace->spaceIndex = glyph->index;
				fFace->spaceWidth = glyph->advance;
			}

			glyph->index = fFace->spaceIndex;
			glyph->image = NULL;
			glyph->discard = false;
			if (uc == 32)
				glyph->advance = fFace->spaceWidth;	// is this enough? or are there other variables that get changed as a side effect of cache lookup and get glyph info
			else if (uc == 9)
				glyph->advance = 4 * fFace->spaceWidth;	// 4 time the space width
			else if ((uc == 10) || (uc == 13))
				glyph->index = -1;
			else {
				glyph->advance = fFace->spaceWidth;
				glyph->missing = true;
			}
		}
		else {
			FskFTMappingLookup(gFskFTMapping, glyph, fFace, uc);
			BAIL_IF_ERR(err = sFTStrikeGetGlyphInfo(glyph, needImage, doOblique, state->zoom));
		}
		if (glyph->index >= 0) {
			glyph->advance += INT_TO_FT(horizRepeat - 1);

			totalWidth += glyph->advance;
				ftSize = sFskFTFaceGetSize(glyph->face);
				use_kerning = (FT_Bool)FT_HAS_KERNING(glyph->face->ftFace);
				// kerning not supported by FreeType Cache Sub-System
				// use_kerning is always false
				if (use_kerning && (glyph != strike) && glyph[-1].index) {
					if (glyph->face == glyph[-1].face) {
						FT_Vector delta;
						FT_Get_Kerning(glyph->face->ftFace, glyph[-1].index, glyph->index, ft_kerning_default, &delta);
						glyph[-1].advance += delta.x;
						totalWidth += fFace->spaceKern;
					}
				}
		}
		glyph++;
	}

	glyph->index = -1;
	*strikeOut = strike;

	return kFskErrNone;
bail:
	sFTStrikeDispose(strike);
	return err;

#endif
}

/********************************************************************************
 * sFTStrikeDispose
 ********************************************************************************/

void sFTStrikeDispose(FskFTGlyph strike)
{
#if USE_RECTS
	return;
#else
	FskFTGlyph glyph;
	if (NULL == strike)
		return;

	for (glyph = strike; glyph->index >= 0; glyph++)
		if (glyph->image && glyph->discard) {
			FT_Done_Glyph(glyph->image);
		}

	FskMemPtrDispose(strike);
#endif
}

/********************************************************************************
 * FskFTFindFont
 ********************************************************************************/

FT_Face FskFTFindFont(const char *fontName, UInt32 textStyle, float textSize)
{
#if USE_RECTS
	RECT_WIDTH = textSize;
	RECT_HEIGHT = textSize;
	return NULL;
#else
	FskErr err = kFskErrNone;
	FskFTFace fFace;
	FT_Face ftFace = NULL;

#if ROUND_TEXTSIZE_DOWN
	UInt32 newTextSize, tsize;
	tsize = (UInt32)textSize;
	newTextSize = (tsize >> 1) << 1;
	textSize = (float)newTextSize;
#endif

	if ((fFace = sFskFTFindFont(fontName, textStyle, (UInt32)textSize, NULL)) != NULL) {
		sFskFTFaceUpdateScaler(fFace, textSize);
		BAIL_IF_ERR(err = FskFTMappingSetCurrentFace(gFskFTMapping, fFace));
		ftFace = fFace->ftFace;
		FskMutexRelease(gFTMutex);
	}
bail:
	return ftFace;
#endif
}


/********************************************************************************
 * FskTextBox
 ********************************************************************************/

FskErr freeTypeBox(FskTextEngineState stateIn, FskBitmap bits, const char *text, UInt32 textLen, FskConstRectangle r, FskConstRectangle clipRect, FskConstColorRGBA color, UInt32 blendLevel, UInt32 textSize, UInt32 textStyle, UInt16 hAlign, UInt16 vAlign, const char *fontName, FskTextFormatCache formatCacheIn)
{
	FskErr							err				= kFskErrNone;
	FskBitmap						freetypeBitmap	= NULL;
	FskFTFaceRecord					*fFace			= NULL;
	FskGraphicsModeParametersRecord	modeParams		= { sizeof(FskGraphicsModeParametersRecord), blendLevel};
	FskTextEngineFreeType			state			= (FskTextEngineFreeType)stateIn;
	FskTextFormatCacheFT			formatCache		= (FskTextFormatCacheFT)formatCacheIn;
	FskFTGlyph						strike			= NULL;
	SInt32							underlinestartx	= 0;
	int								horizRepeat		= 1;
	FskPointRecord		where;
	FskRectangleRecord	bounds, clip;
	FskFTGlyph			glyph;
	int					pen_x, pen_y, repeat, ascent, descent;
	FT_Size ftSize;

#if ROUND_TEXTSIZE_DOWN
	UInt32 newTextSize;
	newTextSize = (textSize >> 1) << 1;
	textSize = newTextSize;
#endif /* ROUND_TEXTSIZE_DOWN */

	BAIL_IF_ZERO(textLen, err, kFskErrNone);

	// combine bounds and clip to total clip
	if (NULL == clipRect)
		clip = *r;
	else
		BAIL_IF_FALSE(FskRectangleIntersect(clipRect, r, &clip), err, kFskErrNone/*kFskErrNothingRendered*/);
	BAIL_IF_FALSE(FskRectangleIntersect(&bits->bounds, &clip, &clip), err, kFskErrNone/*kFskErrNothingRendered*/);

#if !USE_RECTS
	fFace = sFskFTFindFont(fontName, textStyle, textSize, formatCacheIn);
	BAIL_IF_NULL(fFace, err, kFskErrNotFound/*was kFskErrParameterError*/);

	if (textStyle & kFskTextBold) {
		if (!(fFace->style & kFskTextBold)) {
			horizRepeat = 1;
			FskFreeTypePrintfDebug("freeTypeBox synthetic Bold");
		}
	}
#endif /* !USE_RECTS */

#if USE_RECTS
	{	int i;
		bounds.x = 0;
		bounds.y = 0;
		bounds.width = 0;
		bounds.height = RECT_HEIGHT;
		for (i = 0; i < textLen; i++) {
			bounds.width += RECT_WIDTH;		/* = ((RECT_WIDTH << FT_FRACTIONAL_BITS) + (1 << (FT_FRACTIONAL_BITS-1))) >> FT_FRACTIONAL_BITS */
		}
	}
#else /* !USE_RECTS */

	BAIL_IF_ERR(err = sFTStrikeNew((FskTextEngineFreeType)state, &strike, fFace, text, textLen, textStyle, true, kFskFreeTypeNoMaxWidth));
	sFTGetStrikeBBox(state, fFace, strike, &bounds);

	// truncate if necessary
	if (((kFskTextTruncateEnd | kFskTextTruncateCenter) & textStyle) && (bounds.width > r->width)) {
		FskFTGlyph tStrike, g;
		SInt32 width = INT_TO_FT(bounds.width);
		SInt32 targetWidth;

		if (!formatCache || (false == formatCache->haveEllipsisWidth)) {
			const char utf8Ellipsis[] = {0xe2, 0x80, 0xa6, 0};
			// make a strike containing the truncation character
			BAIL_IF_ERR(err = sFTStrikeNew((FskTextEngineFreeType)state, &tStrike, fFace, utf8Ellipsis, sizeof(utf8Ellipsis), textStyle, true, kFskFreeTypeNoMaxWidth));

			if (formatCache) {
				tStrike->discard = false;
				formatCache->ellipsisStrike = tStrike;
				formatCache->haveEllipsisWidth = true;
			}
		}

		if (formatCache) {
			tStrike = formatCache->ellipsisStrike;
		}

		// calculate available space including truncation character
		targetWidth = INT_TO_FT(r->width) - tStrike->advance;

		// find last glyph
		for (glyph = strike; glyph->index >= 0; glyph++)
			;

		if (!(kFskTextTruncateCenter & textStyle)) {
			// back-up until it is short enough
			do {
				glyph--;
				width -= glyph->advance;
				if (width <= targetWidth)
					break;
			} while (glyph != strike);

			// remove the unused characters and append the truncation character
			for (g = glyph; g->index >= 0; g++)
				if (g->image && g->discard)
					FT_Done_Glyph(g->image);
			glyph[0] = *tStrike;
			glyph[1].index = -1;
			bounds.width = FT_ROUND_TO_INT(width + glyph->advance);
		}
		else {
			Boolean phase = true;		// start towards the end
			SInt32 truncBegin, truncEnd, glyphCount = glyph - strike, i;

			truncBegin = truncEnd = glyphCount / 2;
			while ((width > targetWidth) && ((truncEnd - truncBegin) != glyphCount)) {
				if (phase) {
					if (truncEnd < glyphCount) {
						width -= strike[truncEnd].advance;
						truncEnd += 1;
					}
				}
				else {
					if (0 != truncBegin) {
						truncBegin -= 1;
						width -= strike[truncBegin].advance;
					}
				}
				phase = !phase;
			}

			// remove the unused characters and insert the truncation character
			for (i = truncBegin, g = &strike[truncBegin]; i < truncEnd; i++, g++)
				if (g->image && g->discard)
					FT_Done_Glyph(g->image);

			FskMemMove(&strike[truncBegin + 1], &strike[truncEnd], ((glyphCount - truncEnd) + 1) * sizeof(FskFTGlyphRecord));
			strike[truncBegin] = *tStrike;

			bounds.width = FT_ROUND_TO_INT(width + tStrike->advance);
		}

		if (!formatCache || !formatCache->haveEllipsisWidth)
			sFTStrikeDispose(tStrike);
	}
#endif /* !USE_RECTS */

	switch (hAlign) {
		case kFskTextAlignLeft:
		default:
			pen_x = 0;
			break;

		case kFskTextAlignCenter:
			pen_x = (r->width - bounds.width) >> 1;
			break;

		case kFskTextAlignRight:
			pen_x = r->width - bounds.width;
			break;
	}

#if USE_RECTS
	ascent = FT_CEIL_TO_INT(FT_MulFix(INT_TO_FT(RECT_HEIGHT), state->zoom) + FT_AA_AURA);
#else /* !USE_RECTS */
	ftSize = sFskFTFaceGetSize(fFace);
	ascent  = FT_CEIL_TO_INT(+FT_AA_AURA + FT_MulFix(ftSize->metrics.ascender,  state->zoom));
	descent = FT_CEIL_TO_INT(-FT_AA_AURA - FT_MulFix(ftSize->metrics.descender, state->zoom));
#endif /* !USE_RECTS */

	switch (vAlign) {
		case kFskTextAlignTop:
		default:
			pen_y = ascent;
			break;

		case kFskTextAlignCenter:
			pen_y = ((r->height - bounds.height) >> 1) + ascent;
			break;

		case kFskTextAlignBottom:
			pen_y = r->height - bounds.height + ascent;
			break;
	}

	/* Strike level clipping.  The available bounds are only approximate, and in no way guarantee
	 * that nothing is to be rendered outside of these bounds.
	 * We have determined that the vertical uncertainty is on the order of half of the text size (using 18 and 54 points).
	 * The horizontal bounds are much tighter, based on the glyph advances.
	 */
	bounds.x += pen_x + r->x;
	bounds.y += pen_y + r->y - ascent;
	bounds.y += descent;

	{	int boundsUncertainty = ((textSize > FIXED_HALF) ? (textSize >> 17) : (textSize>> 1)) + 2;
		FskRectangleRecord certainBounds;
		FskRectangleSet(&certainBounds, bounds.x, bounds.y - boundsUncertainty, bounds.width + 1, bounds.height + boundsUncertainty + (boundsUncertainty >> 3));
		if (false == FskRectanglesDoIntersect(&certainBounds, &clip))
		goto bail;
	}

#if USE_RECTS
{
	UInt32 advance;
	int i;
	FskRectangleRecord charRect;

	advance = INT_TO_FT(RECT_WIDTH );

	for (i = 0, pen_x <<= FT_FRACTIONAL_BITS; i < textLen; i++, pen_x += advance) {
		where.x = r->x + FT_TRUNC_TO_INT(pen_x);
		where.y = r->y + pen_y - RECT_HEIGHT;

		// glyph level horizontal clipping
		if ((where.x + (SInt32)(FT_TRUNC_TO_INT(advance) + 1)) <= clip.x)
			continue;   // before left edge
		if (where.x > (clip.x + clip.width))
			break;		// after right edge

		FskRectangleSet(&charRect, where.x, where.y, (RECT_WIDTH - 2), pen_y - 2);
		if (blendLevel)
			err = FskRectangleFill(bits, &charRect, color, kFskGraphicsModeAlpha, &modeParams);
		else
			err = FskRectangleFill(bits, &charRect, color, kFskGraphicsModeCopy, NULL);
		BAIL_IF_ERR(err);
	}
}
#else /* !USE_RECTS */

	// allocate the wrapper
	BAIL_IF_ERR(err = FskBitmapNewWrapper(2, 1, kFskBitmapFormat8A, 8, NULL, 2, &freetypeBitmap));

	for (glyph = strike, pen_x <<= FT_FRACTIONAL_BITS; glyph->index >= 0; pen_x += glyph->advance, glyph++) {
		where.x = r->x + FT_ROUND_TO_INT(pen_x);
		where.y = r->y + pen_y;
		// default value
		if (glyph == strike)
			underlinestartx = where.x;

		// glyph level horizontal clipping
		if ((where.x + (SInt32)(FT_TRUNC_TO_INT(glyph->advance) + 1)) <= clip.x)
			continue;   // before left edge
		if (where.x > (clip.x + clip.width))
			break;		// after right edge

		if (glyph->missing) {
			FskRectangleRecord r;
			r.x = where.x + 2, r.y = where.y + 1 ;
			r.width = FT_TRUNC_TO_INT(glyph->advance) - 4;
			r.height = bounds.height - 4;
            r.y = r.y - r.height + 2;
			if (FskRectangleIntersect(&clip, &r, &r))
				FskRectangleFill(bits, &r, color, kFskGraphicsModeCopy, NULL);
		}
		else if (32 == glyph->uc) {
			continue;
		}
		else if (sFTStrikeGetGlyphBitmap(glyph, &where, freetypeBitmap)) {
			UInt32 glyphAscent = 0, glyphDescent = 0;
			// updated value
			if (glyph == strike)
				underlinestartx = where.x;
#if USE_RECTS
#else /* !USE_RECTS */
			if (glyph->face != fFace) {
				ftSize = sFskFTFaceGetSize(fFace);
				glyphAscent  = FT_CEIL_TO_INT(+FT_AA_AURA + FT_MulFix(ftSize->metrics.ascender,  state->zoom));
				glyphDescent = FT_CEIL_TO_INT(-FT_AA_AURA - FT_MulFix(ftSize->metrics.descender, state->zoom));
				where.y += glyphAscent - glyphDescent - ascent + descent;
			}
#endif /* !USE_RECTS */
			for (repeat = 0; repeat < horizRepeat; repeat++) {
				if (!(kFskTextOutline & textStyle)) {
					err = FskTransferAlphaBitmap(freetypeBitmap, NULL, bits, &where, &clip, color, (blendLevel >= 255) ? NULL : &modeParams);
				}
				else {
					FskColorRGBARecord edgeColor;
					edgeColor.r = ~color->r;
					edgeColor.g = ~color->g;
					edgeColor.b = ~color->b;
					edgeColor.a = color->a;
					err = FskTransferEdgeEnhancedGrayscaleText(freetypeBitmap, &freetypeBitmap->bounds, bits, &where, &clip, color, &edgeColor, blendLevel);
				}
				if (err == kFskErrNothingRendered)
					err = kFskErrNone;
				BAIL_IF_ERR(err);
				where.x += 1;
				if (where.x >= (clip.x + clip.width))	/* If we are already past the right edge, ... */
					goto strikes;						/* ... we need not consider any more glyphs. TODO: not true with right-to-left scripts */
			}
#if USE_RECTS
#else /* !USE_RECTS */
			if (glyph->face != fFace) {
				where.y -= glyphAscent - glyphDescent - ascent + descent;
			}
#endif /* !USE_RECTS */
		}
	}
strikes:
	if (underlinestartx < clip.x)
		underlinestartx = clip.x;
	if (textStyle & kFskTextUnderline) {
		FskRectangleRecord ul;
		int underlineThickness;
		SInt32 underliney;
		SInt32 underlineendx = r->x + FT_ROUND_TO_INT(pen_x);

		ftSize = sFskFTFaceGetSize(fFace);
		if (fFace->ftFace->underline_position == 0)
			underliney = r->y + pen_y - (FT_MulFix(state->zoom, FT_MulFix(fFace->ftFace->descender, ftSize->metrics.y_scale)) >> 7);
		else
			underliney = r->y + pen_y - FT_TRUNC_TO_INT(FT_MulFix(state->zoom, FT_MulFix(fFace->ftFace->underline_position, ftSize->metrics.y_scale)));

		if (fFace->ftFace->underline_thickness == 0)
			underlineThickness = -(FT_MulFix(fFace->ftFace->descender, ftSize->metrics.y_scale) >> 8);
		else
			underlineThickness = FT_TRUNC_TO_INT(FT_MulFix(fFace->ftFace->underline_thickness, ftSize->metrics.y_scale));
		if (underlineThickness < 1)
			underlineThickness = 1;

		underliney -= underlineThickness / 2;
		FskRectangleSet(&ul, underlinestartx, underliney,
						underlineendx - underlinestartx,
						underlineThickness);
		if (blendLevel)
			err = FskRectangleFill(bits, &ul, color, kFskGraphicsModeAlpha, &modeParams);
		else
			err = FskRectangleFill(bits, &ul, color, kFskGraphicsModeCopy, NULL);
		BAIL_IF_ERR(err);
	}

	if (textStyle & kFskTextStrike) {
		FskRectangleRecord str;

		str.width = r->x + FT_ROUND_TO_INT(pen_x) - underlinestartx;
		if (fFace->ftFace->underline_thickness == 0)
			str.height = -(FT_MulFix(fFace->ftFace->descender, ftSize->metrics.y_scale) >> 8);
		else
			str.height = FT_TRUNC_TO_INT(FT_MulFix(fFace->ftFace->underline_thickness, ftSize->metrics.y_scale));
		if (str.height < 1)
			str.height = 1;
		str.x = underlinestartx;
		str.y = pen_y + r->y - (ascent / 3) - (str.height / 2);

		if (blendLevel)
			err = FskRectangleFill(bits, &str, color, kFskGraphicsModeAlpha, &modeParams);
		else
			err = FskRectangleFill(bits, &str, color, kFskGraphicsModeCopy, NULL);
		BAIL_IF_ERR(err);
	}

bail:
	FskBitmapDispose(freetypeBitmap);
	sFTStrikeDispose(strike);

	FskMutexRelease(gFTMutex);
#endif /* !USE_RECTS */

#if USE_RECTS
	bail:
#endif /* USE_RECTS */
	return err;
}

/********************************************************************************
 * FskTextGetBounds
 ********************************************************************************/

FskErr freeTypeGetBounds(FskTextEngineState stateIn, FskBitmap bits, const char *text, UInt32 textLen, UInt32 textSize, UInt32 textStyle, const char *fontName, FskRectangle bounds, FskTextFormatCache formatCache)
{
#if USE_RECTS
	int i;
	bounds->x = 0;
	bounds->y = 0;
	bounds->width = 0;
	bounds->height = RECT_HEIGHT;
	for (i = 0; i < textLen; i++) {
		bounds->width += RECT_WIDTH;	/* ((RECT_WIDTH << FT_FRACTIONAL_BITS) + (1 << (FT_FRACTIONAL_BITS-1))) >> FT_FRACTIONAL_BITS */
	}
	return kFskErrNone;
#else
	FskTextEngineFreeType state = (FskTextEngineFreeType)stateIn;
	FskErr			err;
	FskFTFace		fFace;
	FskFTGlyph strike;

#if ROUND_TEXTSIZE_DOWN
	UInt32 newTextSize;
	newTextSize = (textSize >> 1) << 1;
	textSize = newTextSize;
#endif

	bounds->x = 0;
	bounds->y = 0;
	bounds->width = 0;
	bounds->height = 0;

	if (textLen == 0) {
		return kFskErrNone;
	}

	fFace = sFskFTFindFont(fontName, textStyle, textSize, formatCache);

	if (NULL == fFace)
		return kFskErrParameterError;

	BAIL_IF_ERR(err = sFTStrikeNew((FskTextEngineFreeType)state, &strike, fFace, text, textLen, textStyle, false, kFskFreeTypeNoMaxWidth));
	sFTGetStrikeBBox(state, fFace, strike, bounds);

	if ((textStyle & kFskTextItalic) && !(fFace->style & kFskTextItalic)) {	/* If we had to oblique a plain font, we need to adjust the width */
		FskFreeTypePrintfDebug("freeTypeGetBounds adjusting bbox from %dx%d to %dx%d due to obliquing",
			bounds->width, bounds->height, bounds->width + ((bounds->height * (OBLIQUE_SKEW >> (16 - 3))) >> 3), bounds->height);
		bounds->width += (bounds->height * (OBLIQUE_SKEW >> (16 - 3))) >> 3;		/* Add the fraction of the height that translates to skew, quantized to eighths (3/8) */
	}
	if (textStyle & kFskTextUnderline) {
		FT_Size ftSize = sFskFTFaceGetSize(fFace);
		FskFixed scale = FT_MulFix(state->zoom, ftSize->metrics.y_scale);
		SInt32 underlineThickness, underliney, descender;

		/* Compute underline thickness */
		if (0 == (underlineThickness = fFace->ftFace->underline_thickness))								/* If no underline thickness is specified, ... */
			underlineThickness = (-fFace->ftFace->descender + (1 << (2-1))) >> 2;						/* ... use one quarter descender distance */
		underlineThickness = FT_TRUNC_TO_INT(FT_MulFix(underlineThickness, scale));						/* Scale the thickness, and convert to an integer */
		if (underlineThickness < 1)
			underlineThickness = 1;

		/* Compute the location of the underline */
		if (0 == (underliney = fFace->ftFace->underline_position))										/* If no underline position is specified, ... */
			underliney = (fFace->ftFace->descender + (1 << (1-1))) >> 1;								/* ... use one half descender distance */
		underliney = FT_MulFix(underliney, scale);

		/* Compute the descender */
		descender = FT_MulFix(ftSize->metrics.descender, state->zoom);

		/* Compute adjustment to the bounding box */
		descender  = -FT_TRUNC_TO_INT(descender);														/* Convert descender  to positive integer */
		underliney = -FT_TRUNC_TO_INT(underliney);														/* Convert underliney to positive integer */
		underliney -= underlineThickness >> 1;															/* Push underliney up by half the underline thickness */
		underliney += underlineThickness;																/* Pull underliney down by the underline thickness */
		underliney -= descender;																		/* See whether the underline exceeds the descender */
		if (underliney > 0)																				/* If it does, ... */
			bounds->height += underliney;																/* ... increase the height */
	}


bail:
	sFTStrikeDispose(strike);

	FskMutexRelease(gFTMutex);

	return err;
#endif
}

/********************************************************************************
 * FskTextGetLayout
 ********************************************************************************/

FskErr freeTypeGetLayout(FskTextEngineState stateIn, FskBitmap bits, const char *text, UInt32 textLen, UInt32 textSize, UInt32 textStyle, const char *fontName,
							UInt16 **unicodeTextPtr, UInt32 *unicodeLenPtr, FskFixed **layoutPtr, FskTextFormatCache cache)
{
	FskTextEngineFreeType state = (FskTextEngineFreeType)stateIn;
	FskFTGlyph			strike			= NULL;
	UInt16			*unicodeText	= NULL;
	FskFixed		*layout			= NULL;
	FskFTGlyph			glyph;
	FskErr			err;
	FskFTFace		fFace;
	FskFixed		x;
	UInt32			numGlyphs, i;

#if ROUND_TEXTSIZE_DOWN
	textSize = (textSize >> 1) << 1;
#endif /* ROUND_TEXTSIZE_DOWN */

	fFace = sFskFTFindFont(fontName, textStyle, textSize, cache);
	if (NULL == fFace)
		return kFskErrParameterError;

	BAIL_IF_ERR(err = sFTStrikeNew((FskTextEngineFreeType)state, &strike, fFace, text, textLen, textStyle, false, kFskFreeTypeNoMaxWidth));

	for (glyph = strike, numGlyphs = 0; glyph->index >= 0; ++glyph)
		++numGlyphs;
	BAIL_IF_ERR(err = FskMemPtrNew(sizeof(*unicodeText) * numGlyphs, &unicodeText));
	BAIL_IF_ERR(err = FskMemPtrNew(sizeof(*layout)      * numGlyphs, &layout));
	for (i = 0, glyph = strike, x = 0; i < numGlyphs; ++i, x += glyph->advance, ++glyph) {
		layout[i] = x << (16-FT_FRACTIONAL_BITS);	/* Convert from 6 to 16 fractional bits */
		unicodeText[i] = glyph->uc;
	}

	if (unicodeLenPtr)
		*unicodeLenPtr = numGlyphs;
	if (unicodeTextPtr) {
		*unicodeTextPtr = unicodeText;
		unicodeText = NULL;
	}
	if (layoutPtr) {
		*layoutPtr = layout;
		layout = NULL;
	}

bail:
	sFTStrikeDispose(strike);
	FskMutexRelease(gFTMutex);
	FskMemPtrDispose(layout);
	FskMemPtrDispose(unicodeText);
	if (err) {
		if (unicodeLenPtr)	*unicodeLenPtr	= 0;
		if (unicodeTextPtr)	*unicodeTextPtr	= NULL;
		if (layoutPtr)		*layoutPtr		= NULL;
	}
	return err;
}

/********************************************************************************
 * FskTextGetFontList
 ********************************************************************************/

FskErr freeTypeGetFontList(FskTextEngineState state, char **fontNames)
{
	FskFTFamily family;
	FskMemPtr fontNameRet = NULL;
	int entrySize, retSize, retLoc;
	FskErr err;

	if (!gFskFTMapping)
		return kFskErrOperationFailed;
	retSize = 0;
	retLoc = 0;
	for (family = gFskFTMapping->family; family; family = family->next) {
		entrySize = FskStrLen(family->name);
		retSize += entrySize + 1;
		err = FskMemPtrRealloc(retSize + 1, (FskMemPtr*)&fontNameRet);
		if (err) {
			FskMemPtrDisposeAt(&fontNameRet);
			goto bail;
		}
		FskStrCopy((char*)(&fontNameRet[retLoc]), family->name);
		FskFreeTypePrintfDebug("FONT - %s", family->name);
		retLoc += entrySize + 1;
	}
	fontNameRet[retLoc] = '\0';	// terminate the list

bail:
	*fontNames = (char*)fontNameRet;

	return err;
}

/********************************************************************************
 * FskFTFontConfigure
 ********************************************************************************/

FskErr freeTypeDefaultFontSet(FskTextEngineState state, const char *defaultFace)
{
	FskErr err = kFskErrNone;
	FskFTFamily family;
	FskFTAlias alias = NULL;

	if (gFskFTMapping && !gFskFTMapping->system) {
		for (family = gFskFTMapping->family; family; family = family->next) {
			if (!FskStrCompareCaseInsensitive(defaultFace, family->name)) {
				BAIL_IF_ERR(err = FskMemPtrNewClear(sizeof(FskFTAliasRecord), &alias));
				BAIL_IF_NULL(alias->name = FskStrDoCopy(defaultFace), err, kFskErrMemFull);
				alias->family = family;
				FskFreeTypePrintfDebug("freeTypeDefaultFontSet to %s", family->name);
				FskListPrepend(&gFskFTMapping->system, alias);
				break;
			}
		}
	}
bail:
	FskFTMappingPrint(gFskFTMapping);
	return kFskErrNone;
}

#ifdef DEBUG_DEFAULT_FONT
/*****************************************************************************//*
 * Get the Freetype default font.
 *	\param[in]	state	the font text engine state (fte->state).
 *	\param[out]	pList	a location to store a string list of the default fonts; FskMemPtrDispose() should be called when done.
 *						NULL is a valid parameter, because the return value indicates whether a default font was found at all.
 *	\return		kFskErrNone		if a default font was found.
 *	\return		kFskErrNotFound	if a default font was not found.
 ********************************************************************************/

FskAPI(FskErr) freeTypeDefaultFontGet(FskTextEngineState state, char **pList);
FskErr freeTypeDefaultFontGet(FskTextEngineState state, char **pList)
{
	FskErr err = kFskErrNone;

	BAIL_IF_FALSE(gFskFTMapping && gFskFTMapping->system && gFskFTMapping->system->family && gFskFTMapping->system->family->name, err, kFskErrNotFound);

	if (pList) {																		/* If a list is requested (rather than merely querying its existence) */
		UInt32		length	= 1;														/* Account for the terminating null character */
		char		*s;
		FskFTAlias	al;

		*pList = NULL;
		for (al = gFskFTMapping->system; al; al = al->next)								/* Count the total length ... */
			length += FskStrLen(al->family->name) + 1;									/* ... of all default font names */
		BAIL_IF_ERR(FskMemPtrNew(length, pList));										/* Allocate a font name list */
		for (al = gFskFTMapping->system, s = *pList; al; al = al->next, s += length)	/* For each default font name, ... */
			FskMemCopy(s, al->family->name, length = FskStrLen(al->family->name) + 1);	/* ... copy it to the list */
		*s = 0;																			/* Add a null list terminator */
	}

bail:
	return err;
}
#endif /* DEBUG_DEFAULT_FONT */

/********************************************************************************
 * freeTypeSetFamilyMap
 ********************************************************************************/

FskErr freeTypeSetFamilyMap(FskTextEngineState state, const char *map)
{
	FskMemPtrDisposeAt(&gFontFamilyMap);
	return FskStrListDoCopy(map, &gFontFamilyMap);
}

/********************************************************************************
 * FskTextFreeTypeInitialize
 ********************************************************************************/

void *ftMalloc(FT_Memory memory, long size) {
	return FskMemPtrAlloc(size);
}

void ftFree(FT_Memory memory, void *block) {
	FskMemPtrDispose((FskMemPtr)block);
}

void *ftRealloc(FT_Memory memory, long cur_size, long new_size, void *block) {
	FT_Pointer ret;
	ret = block;
	if (kFskErrNone == FskMemPtrRealloc(new_size, &ret))
		return ret;
	else
		return NULL;
}

void FskTextFreeTypePurge(void *refcon)
{
	if (gFTCacheManager)
		FTC_Manager_Reset(gFTCacheManager);
}


FskErr FskTextFreeTypeInitialize(void)
{
	FskErr err = kFskErrNone;

	BAIL_IF_ERR(FskMutexNew(&gFTMutex, "gFTMutex"));

#if FSK_FREETYPE_KINOMA_ALLOCATOR
	BAIL_IF_ERR(FskMemPtrNew(sizeof(struct FT_MemoryRec_), &ftmem));
	ftmem->user = NULL;
	ftmem->alloc = ftMalloc;
	ftmem->free = ftFree;
	ftmem->realloc = ftRealloc;

	BAIL_IF_ERR(FT_New_Library(ftmem, &gFTLibrary));
	FT_Add_Default_Modules(gFTLibrary);
#else
	BAIL_IF_ERR(FT_Init_FreeType(&gFTLibrary));
#endif

//	BAIL_IF_ERR(FTC_Manager_New(gFTLibrary, 0, 0, 0, _FTFaceRequester, 0, &gFTCacheManager));
	BAIL_IF_ERR(FTC_Manager_New(gFTLibrary, 8, 16, 400000, _FTFaceRequester, 0, &gFTCacheManager));
	BAIL_IF_ERR(FTC_SBitCache_New(gFTCacheManager, &gFTSbitsCache));
	BAIL_IF_ERR(FTC_ImageCache_New(gFTCacheManager, &gFTImageCache));
	BAIL_IF_ERR(FTC_CMapCache_New(gFTCacheManager, &gFTCmapCache));

	FskExtensionInstall(kFskExtensionTextEngine, &gFskTextFreeTypeDispatch);

    FskNotificationRegister(kFskNotificationLowMemory, FskTextFreeTypePurge, NULL);

	BAIL_IF_ERR(FskFTMappingNew(&gFskFTMapping));
	return err;
bail:
	return kFskErrOperationFailed;
}

/********************************************************************************
 * FskTextFreeTypeUninitialize
 ********************************************************************************/

void FskTextFreeTypeUninitialize(void)
{

	FskNotificationUnregister(kFskNotificationLowMemory, FskTextFreeTypePurge, NULL);

//	sFskFTFontScanDispose();

	FTC_Manager_Done(gFTCacheManager);
	FT_Done_FreeType(gFTLibrary);

	FskMemPtrDispose(gFontFamilyMap);

	FskMutexDispose(gFTMutex);

	FskFTMappingDispose(gFskFTMapping);

	FskExtensionUninstall(kFskExtensionTextEngine, &gFskTextFreeTypeDispatch);
}

FskErr freeTypeFitWidth(FskTextEngineState state, FskBitmap bits, const char *text, UInt32 textLen, UInt32 textSize, UInt32 textStyle, const char *fontName, UInt32 width, UInt32 flags, UInt32 *fitBytesOut, UInt32 *fitCharsOut, FskTextFormatCache formatCache)
{
	FskErr err = kFskErrNone;
	UInt32 aSize, aBytes, aWidth;
	FskFTGlyph strike = NULL, glyph;
	Boolean doRelease = false;
	FskFTFace		fFace;

#if ROUND_TEXTSIZE_DOWN
	UInt32 newTextSize;
	newTextSize = (textSize >> 1) << 1;
	textSize = newTextSize;
#endif

	aSize = 0;
	aBytes = 0;

	if (0 == textLen)
		goto bail;

	fFace = sFskFTFindFont(fontName, textStyle, textSize, formatCache);

	if (NULL == fFace)
		goto bail;

	doRelease = true;

	BAIL_IF_ERR(err = sFTStrikeNew((FskTextEngineFreeType)state, &strike, fFace, text, textLen, textStyle, false, width + 10));

	width <<= FT_FRACTIONAL_BITS;
	aWidth = 0;
	for (glyph = strike; glyph->index >= 0; glyph++) {
		aWidth += glyph->advance;
		if (width < aWidth)
			break;
		aSize++;
		aBytes += glyph->chars;
	}
	if ((kFskTextFitFlagBreak & flags) && (glyph->index >= 0)) {
		const char *p = text + aBytes;
		UInt32 truncSize = aSize;
		while (glyph > strike) {
			glyph--;
			if (FskTextUTF8IsWhitespace((const UInt8*)(&p[-glyph->chars]), glyph->chars, NULL)) {	// if we break on a space, we want to return that space as part of the string
				aBytes = p - text;
				aSize = truncSize;
				break;
			}
			truncSize--;
			p -= glyph->chars;
		}
	}

bail:
	sFTStrikeDispose(strike);

	if (doRelease)
		FskMutexRelease(gFTMutex);

	if (err) {
		aSize = 0;
		aBytes = 0;
	}

	if (fitCharsOut)
		*fitCharsOut = aSize;

	if (fitBytesOut)
		*fitBytesOut = aBytes;

	return err;
}

FskErr freeTypeGetFontInfo(FskTextEngineState stateIn, FskTextFontInfo info, const char *fontName, UInt32 textSize, UInt32 textStyle, FskTextFormatCache formatCache)
{
	FskTextEngineFreeType	state = (FskTextEngineFreeType)stateIn;
	FskFTFace				fFace;
	SInt32					leading;
	FT_Size ftSize;

#if ROUND_TEXTSIZE_DOWN
	UInt32 newTextSize;
	newTextSize = (textSize >> 1) << 1;
	textSize = newTextSize;
#endif /* ROUND_TEXTSIZE_DOWN */

	fFace = sFskFTFindFont(fontName, textStyle, (UInt32)textSize, formatCache);
	if (NULL == fFace)
		return kFskErrParameterError;       //@@ muxtex not released??

	ftSize = sFskFTFaceGetSize(fFace);

	info->width   = FT_ROUND_TO_INT(FskFixMul(ftSize->face->bbox.xMax - ftSize->face->bbox.xMin, FskFixMul(state->zoom, ftSize->metrics.x_scale)));
	info->height  = FT_ROUND_TO_INT(FskFixMul(ftSize->metrics.height,     state->zoom)) + HEIGHT_ADJUST_PX;
	info->glyphHeight  = FT_ROUND_TO_INT(FskFixMul(ftSize->metrics.ascender + -ftSize->metrics.descender,     state->zoom)) + HEIGHT_ADJUST_PX;
	info->ascent  = FT_ROUND_TO_INT(FskFixMul(ftSize->metrics.ascender,   state->zoom));
	info->descent = FT_ROUND_TO_INT(FskFixMul(-ftSize->metrics.descender, state->zoom)) + HEIGHT_ADJUST_PX;	/* Convert from negative descent to positive */
	leading = ftSize->metrics.height - ftSize->metrics.ascender + ftSize->metrics.descender;					/* Note: height >= 0, ascender >= 0, descender <= 0 */
	info->leading = (leading >= 0) ? FT_ROUND_TO_INT(FskFixMul(leading, state->zoom)) + HEIGHT_ADJUST_PX : 0;	/* We want leading to be non-negative */

//	FskFreeTypePrintfDebug("fontName: %s Ascent: x%x Desc: x%x Height: x%x", fontName, ftSize->metrics.ascender, ftSize->metrics.descender, ftSize->metrics.height);
//	FskFreeTypePrintfDebug("  ascent: %d - descent: %d - leading: %d", info->ascent, info->descent, info->leading);

	FskMutexRelease(gFTMutex);

	return kFskErrNone;
}

FskErr freeTypeFormatCacheNew(FskTextEngineState state, FskTextFormatCache *cache, FskBitmap bits, UInt32 textSize, UInt32 textStyle, const char *fontName)
{
	FskTextFormatCacheFT formatCache;
#if ROUND_TEXTSIZE_DOWN
	UInt32 newTextSize;
	newTextSize = (textSize >> 1) << 1;
	textSize = newTextSize;
#endif

	FskMemPtrNewClear(sizeof(FskTextFormatCacheFTRecord), &formatCache);
	if (NULL == formatCache)
		return kFskErrOperationFailed;

	formatCache->fFace = sFskFTFindFont(fontName, textStyle, textSize, NULL);

	FskMutexRelease(gFTMutex);

	*cache = (FskTextFormatCache)formatCache;

	return kFskErrNone;
}

FskErr freeTypeFormatCacheDispose(FskTextEngineState state, FskTextFormatCache cache)
{
	if (NULL != cache) {
		if (((FskTextFormatCacheFT)cache)->haveEllipsisWidth) {
			FskFTGlyph glyph = ((FskTextFormatCacheFT)cache)->ellipsisStrike;
			glyph->discard = true;
			sFTStrikeDispose(glyph);
			// FskMemPtrDispose(((FskTextFormatCacheFT)cache)->ellipsisStrike);
		}
		FskMemPtrDispose(cache);
	}
	return kFskErrNone;
}

FskErr freeTypeSetZoom(void *stateIn, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
	FskTextEngineFreeType state = stateIn;

	state->zoom = (FT_Fixed)(property->value.number * 65536.0);
	if (state->zoom <= 0)
		state->zoom = 0x010000;

	return kFskErrNone;
}

FskErr freeTypeGetFractionalTextSize(void *stateIn, void *track, UInt32 propertyID, FskMediaPropertyValue property)
{
    property->type = kFskMediaPropertyTypeBoolean;
    property->value.b = true;

    return kFskErrNone;
}

FskErr freeTypeNew(FskTextEngineState *stateOut)
{
	FskErr err;
	FskTextEngineFreeType state;

	err = FskMemPtrNewClear(sizeof(FskTextEngineFreeTypeRecord), &state);
	BAIL_IF_ERR(err);

	state->zoom = 0x00010000L;

bail:
	*stateOut = (FskTextEngineState)state;


	return err;
}

FskErr freeTypeDispose(FskTextEngineState state)
{
	FskMemPtrDispose(state);
	return kFskErrNone;
}

/********************************************************************************
 * FskFTMapping
 ********************************************************************************/

FskErr FskFTMappingNew(FskFTMapping *it)
{
	FskErr err = kFskErrNone;

	FskFTMapping mapping = NULL;
	BAIL_IF_ERR(err = FskMemPtrNewClear(sizeof(FskFTMappingRecord), &mapping));
	mapping->familyCache = FskAssociativeArrayNew();
	*it = mapping;
bail:
	return err;
}

void FskFTMappingDispose(FskFTMapping mapping)
{
	FskFTFamily family, nextFamily;
	FskFTFace face, nextFace;
	FskFTAlias alias, nextAlias;

	for (family = mapping->family; family; family = nextFamily) {
		nextFamily = family->next;
		FskMemPtrDispose(family->name);
		for (face = family->face; face; face = nextFace) {
			nextFace = face->next;
			FskMemPtrDispose(face->fixedSizes);
			FskMemPtrDispose(face);
		}
		FskMemPtrDispose(family);
	}
	for (alias = mapping->system; alias; alias = nextAlias) {
		nextAlias = alias->next;
		FskMemPtrDispose(alias->name);
		FskMemPtrDispose(alias);
	}
	for (alias = mapping->alias; alias; alias = nextAlias) {
		nextAlias = alias->next;
		FskMemPtrDispose(alias->name);
		FskMemPtrDispose(alias);
	}
	for (alias = mapping->language; alias; alias = nextAlias) {
		nextAlias = alias->next;
		FskMemPtrDispose(alias->name);
		FskMemPtrDispose(alias);
	}
	for (alias = mapping->fallback; alias; alias = nextAlias) {
		nextAlias = alias->next;
		FskMemPtrDispose(alias->name);
		FskMemPtrDispose(alias);
	}
	FskAssociativeArrayDispose(mapping->familyCache);
	FskMemPtrDispose(mapping);
}

FskErr FskFTMappingAddFace(FskFTMapping mapping, const char* name, const char* style, FskFTFace face)
{
	FskErr err = kFskErrNone;
	FskFTFamily family = NULL;
	FskFTFace* facePtr = NULL;
	char* filename = FskStrRChr(face->path, '/');
	char* family_name = FskStrDoCopy(name);
	if (family_name && mapping && filename++) {
		// patch for Samsung
		char* p = FskStrRChr(family_name, '-');
		if (p) {
			if (!FskStrCompareCaseInsensitive(p + 1, style))
				*p = 0;
		}
		// add face to the families
		for (family = mapping->family; family; family = family->next) {
			if (!FskStrCompareCaseInsensitive(family_name, family->name))
				break;
		}
		if (!family) {
			BAIL_IF_ERR(err = FskMemPtrNewClear(sizeof(FskFTFamilyRecord), &family));
			BAIL_IF_NULL(family->name = FskStrDoCopy(family_name), err, kFskErrMemFull);
			FskListAppend(&mapping->family, family);
		}
		for (facePtr = &family->face; *facePtr; facePtr = &(*facePtr)->next) {
			if (*facePtr && ((*facePtr)->weight > face->weight)) {
				face->next = *facePtr;
				*facePtr = face;
				break;
			}
		}
		if (!*facePtr)
			FskListAppend(&family->face, face);
	}
bail:
	if (err) {
		if (family) {
			FskMemPtrDispose(family->name);
			FskMemPtrDispose(family);
		}
	}
	FskMemPtrDispose(family_name);
	return err;
}

void FskFTMappingXMLDefault(void *data, const char *text, int size)
{
	FskErr err = kFskErrNone;
	FskFTParserData parserData = data;
	int i;
	UInt32 length;

	if (parserData->value) {
		for (i = 0; i < size; i++)
			if (!isspace(text[i])) break;
		size -= i;
		if (size) {
			length = FskStrLen(parserData->value);
			BAIL_IF_ERR(err = FskMemPtrRealloc(length + size + 1, &parserData->value));
			FskMemCopy(parserData->value + length, text + i, size);
			parserData->value[length + size] = 0;
		}
	}
bail:
	return;
}

void FskFTMappingXMLStartTag(void *data, const char *name, const char **attributes)
{
	FskErr err = kFskErrNone;
	FskFTParserData parserData = data;
	FskFTMapping mapping = parserData->mapping;
	FskFTAlias family = NULL;
	FskFTAlias language = NULL;
	FskFTAlias alias = NULL;
	char** ptr = (char**)attributes;
	char* key;
	char* value;

	if (!FskStrCompare(name, "family")) {
		BAIL_IF_ERR(err = FskMemPtrNewClear(sizeof(FskFTAliasRecord), &family));
		while (*ptr) {
			key = *ptr++;
			value = *ptr++;
			if (!FskStrCompare(key, "name")) {
				BAIL_IF_NULL(family->name = FskStrDoCopy(value), err, kFskErrMemFull);
			}
			else if (!FskStrCompare(key, "lang")) {
				BAIL_IF_ERR(err = FskMemPtrNewClear(sizeof(FskFTAliasRecord), &language));
				BAIL_IF_NULL(language->name = FskStrDoCopy(value), err, kFskErrMemFull);
				parserData->language = language;
			}
		}
		parserData->family = family;
	}
	else if (!FskStrCompare(name, "font") || !FskStrCompare(name, "file")) {
		BAIL_IF_ERR(err = FskMemPtrNewClear(1, &parserData->value));
		while (*ptr) {
			key = *ptr++;
			value = *ptr++;
			if (!parserData->language && !FskStrCompare(key, "lang")) {
				BAIL_IF_ERR(err = FskMemPtrNewClear(sizeof(FskFTAliasRecord), &language));
				BAIL_IF_NULL(language->name = FskStrDoCopy(value), err, kFskErrMemFull);
				parserData->language = language;
			}
		}
	}
	else if (!FskStrCompare(name, "alias")) {
		BAIL_IF_ERR(err = FskMemPtrNewClear(sizeof(FskFTAliasRecord), &alias));
		while (*ptr) {
			key = *ptr++;
			value = *ptr++;
			if (!FskStrCompare(key, "name")) {
				BAIL_IF_NULL(alias->name = FskStrDoCopy(value), err, kFskErrMemFull);
			}
			else if (!FskStrCompare(key, "to")) {
				FskFTAlias system;
				for (system = mapping->system; system; system = system->next) {
					if (!FskStrCompareCaseInsensitive(value, system->name)) {
						FskListAppend(&mapping->alias, alias);
						alias->family = system->family;
						break;
					}
				}
			}
		}
	}
	else if (!FskStrCompare(name, "name")) {
		BAIL_IF_ERR(err = FskMemPtrNewClear(sizeof(FskFTAliasRecord), &alias));
		BAIL_IF_ERR(err = FskMemPtrNewClear(1, &parserData->value));
		parserData->alias = alias;
	}
	return;
bail:
	if (alias) {
		FskMemPtrDispose(alias->name);
		FskMemPtrDispose(alias);
	}
	if (family) {
		FskMemPtrDispose(family->name);
		FskMemPtrDispose(family);
	}
	if (language) {
		FskMemPtrDispose(language->name);
		FskMemPtrDispose(language);
	}
	return;
}

void FskFTMappingXMLStopTag(void *data, const char *name)
{
	FskFTParserData parserData = data;
	FskFTMapping mapping = parserData->mapping;
	FskFTFace face = NULL;
	FskFTAlias alias = parserData->alias;
	if (parserData->family && !FskStrCompare(name, "family")) {
		FskFTAlias family = parserData->family;
		if (family->family) {
			if (family->name) {
				FskListAppend(&mapping->system, family);
				for (alias = mapping->alias; alias; alias = alias->next) {
					if (alias->family == (FskFTFamily)family)
						alias->family = family->family;
				}
			}
			else {
					FskFTAlias language = parserData->language;
					FskListAppend(&mapping->fallback, family);
					if (language) {
						language->family = family->family;
						FskListAppend(&mapping->language, language);
						parserData->language = NULL;
					}
				}
			}
		else {
			FskMemPtrDispose(family->name);
			FskMemPtrDispose(family);
		}
		if (parserData->language) {
			FskMemPtrDispose(parserData->language->name);
			FskMemPtrDispose(parserData->language);
		}
		parserData->family = NULL;
	}
	else if (parserData->value && parserData->family && (!FskStrCompare(name, "font") || !FskStrCompare(name, "file"))) {
		// map alias
		FskFTFamily family;
		if (!parserData->family->family) {
			for (family = mapping->family; family; family = family->next) {
				for (face = family->face; face; face = face->next) {
					char* filename = FskStrRChr(face->path, '/');
					if (filename++) {
						if (!FskStrCompareCaseInsensitive(parserData->value, filename)) {
							parserData->family->family = family;
						}
					}
				}
			}
		}
		FskMemPtrDisposeAt(&parserData->value);
	}
	else if (parserData->family && parserData->alias && !FskStrCompare(name, "name")) {
		FskFTAlias family = parserData->family;
		FskFTAlias alias = parserData->alias;
		if (family->name) {
			alias->family = (FskFTFamily)family;
			FskListAppend(&mapping->alias, alias);
			alias->name = parserData->value;
		}
		else {
			family->name = parserData->value;
			FskMemPtrDispose(alias);
		}
		parserData->alias = NULL;
	}
	return;
}

FskErr FskFTMappingFromFile(FskFTMapping mapping, const char* path)
{
	FskErr err = kFskErrNone;
	FskInt64 size;
	unsigned char *data;
	FskFileMapping map = NULL;
	XML_Parser expat = NULL;
	FskFTParserDataRecord record;

	BAIL_IF_ERR(err = FskFileMap(path, &data, &size, 0, &map));
	BAIL_IF_NULL(expat = XML_ParserCreate(NULL), err, kFskErrOperationFailed);

	FskMemSet(&record, 0, sizeof(FskFTParserDataRecord));
	record.mapping = mapping;
	XML_SetUserData(expat, &record);
	XML_SetElementHandler(expat, FskFTMappingXMLStartTag, FskFTMappingXMLStopTag);
	XML_SetCharacterDataHandler(expat, FskFTMappingXMLDefault);

	if (!XML_Parse(expat, (const char*)data, (int)size, true)) {
		FskFreeTypePrintfDebug("EXPAT ERROR: %s - %d %d", XML_ErrorString(XML_GetErrorCode(expat)), (int)XML_GetCurrentLineNumber(expat), (int)XML_GetCurrentColumnNumber(expat));
		BAIL(kFskErrBadData);
	}
bail:
	if (expat)
		XML_ParserFree(expat);

	FskFileDisposeMap(map);
	return err;
}

FskErr FskFTMappingLookup(FskFTMapping mapping, FskFTGlyph glyph, FskFTFace face, UInt32 unicode)
{
	FskErr err = kFskErrNone;
	UInt32 index = 0;
	FskFTFace fallback = NULL;
	FskFTAlias alias;

	index = FTC_CMapCache_Lookup(gFTCmapCache, (FTC_FaceID)face, face->CMAPIndex, unicode);
	if (index)
		goto bail;
	if (mapping) {
		fallback = mapping->fallbackFace;
		if (fallback) {
			index = FTC_CMapCache_Lookup(gFTCmapCache, (FTC_FaceID)fallback, fallback->CMAPIndex, unicode);
			if (index) {
				sFskFTFaceUpdateScaler(fallback, face->size);
				face = fallback;
				glyph->fallback = true;
				goto bail;
			}
		}
		for (alias = mapping->fallback, fallback = NULL; alias; alias = alias->next, fallback = NULL) {
			FskFTFace closest = NULL;;
			for (closest = fallback = alias->family->face; fallback; fallback = fallback->next) {
				if (fallback->style == face->style)
					break;
			}
			if (!fallback)
				fallback = closest;
			if (fallback) {
				index = FTC_CMapCache_Lookup(gFTCmapCache, (FTC_FaceID)fallback, fallback->CMAPIndex, unicode);
				if (index) {
					sFskFTFaceUpdateScaler(fallback, face->size);
					face = mapping->fallbackFace = fallback;
					glyph->fallback = true;
					goto bail;
				}
			}
		}
	}
	glyph->missing = true;
	glyph->uc = 32;
	gFskFTMapping->currentFace = NULL;
	err = kFskErrNotFound;
bail:
	glyph->face = face;
	if (gFskFTMapping->currentFace != face)
		gFskFTMapping->currentFace = NULL;
	glyph->index = index;
	return err;
}

void FskFTMappingPrint(FskFTMapping mapping)
{
	FskFTFamily family;
	FskFTAlias alias;
	FskFTFace face;

	FskFreeTypePrintfDebug("- FskFTMappingPrint: %p", mapping);
	if (mapping) {
		FskFreeTypePrintfDebug(" - family:");
		for (family = mapping->family; family; family = family->next) {
			FskFreeTypePrintfDebug("  - %s:", family->name);
			for (face = family->face; face; face = face->next) {
				FskFreeTypePrintfDebug("   + %d %d < %s", face->weight, face->style, face->path);
			}
		}
		FskFreeTypePrintfDebug(" - system:");
		for (alias = mapping->system; alias; alias = alias->next) {
			FskFreeTypePrintfDebug("  - %s -> %s", alias->name, alias->family ? alias->family->name : "none");
		}
		FskFreeTypePrintfDebug(" - alias:");
		for (alias = mapping->alias; alias; alias = alias->next) {
			FskFreeTypePrintfDebug("  - %s -> %s", alias->name, alias->family ? alias->family->name : "none");
		}
		FskFreeTypePrintfDebug(" - language:");
		for (alias = mapping->language; alias; alias = alias->next) {
			FskFreeTypePrintfDebug("  - %s -> %s", alias->name, alias->family ? alias->family->name : "none");
		}
		FskFreeTypePrintfDebug(" - fallback:");
		for (alias = mapping->fallback; alias; alias = alias->next) {
			FskFreeTypePrintfDebug("  -> %s:", alias->family ? alias->family->name : "none");
		}
	}
}

FskErr FskFTMappingSetCurrentFace(FskFTMapping mapping, FskFTFace face)
{
	FskErr err = kFskErrNone;
#if USE_RECTS
#else
	if ((mapping->currentFace != face) || (face->ftSize == NULL)) {
		BAIL_IF_ERR(err = FTC_Manager_LookupSize(gFTCacheManager, &face->scaler, &face->ftSize));
		face->ftFace = face->ftSize->face;
		mapping->currentFace = face;
	}
bail:
#endif
	return err;
}

FskErr FskFTAddMapping(const char* path)
{
	FskErr err = kFskErrNone;
	FskFTMapping mapping = gFskFTMapping;
	if (path)
		BAIL_IF_ERR(err = FskFTMappingFromFile(mapping, path));
	else {
		FskFTFamily family;
		FskFTAlias alias = NULL;
		for (family = mapping->family; family; family = family->next) {
			if (FskStrStr(family->name, "Fallback")) {
				BAIL_IF_ERR(err = FskMemPtrNewClear(sizeof(FskFTAliasRecord), &alias));
				alias->family = family;
				alias->name = FskStrDoCopy(family->name);
				FskListAppend(&mapping->fallback, alias);
				break;;
			}
		}
	}
bail:
	FskFTMappingPrint(gFskFTMapping);
	return err;
}
