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
#include "FskFont.h"
#include "FskGrowableStorage.h"
#include "FskFixedMath.h"
#include <string.h>

typedef UInt16	FskUniChar;


/********************************************************************************
 * FskFontFaceRecord
 ********************************************************************************/

typedef struct FskFontFaceRecord {
	UInt8					*fontFamily;			/* Comma-separated UTF-8 list */
	UInt32					fontStyle;				/* OR of {normal, italic, oblique} */
	UInt32					fontVariant;			/* OR of {normal, small-caps} */
	UInt32					fontWeight;				/* OR of {all, normal, bold, 100, 200, 300, 400, 500, 600, 700, 800, 900} */
	UInt32					fontStretch;			/* OR of {all, normal, {ultra-,extra-,semi-,}{condensed,expanded}} */
	UInt8					*fontSize;				/*  */
	FskUnicodeRange			*unicodeRange;			/* default = [0, 0x10FFFF] */
	FskFixed				unitsPerEm;				/* default = 1000 */
	UInt32					panose1[10];			/* default = { 0,0,0,0,0,0,0,0,0,0 } */
	FskFixed				stemV;					/* default = 0 */
	FskFixed				stemH;					/* default = 0 */
	FskFixed				slope;					/* default = 0 */
	FskFixed				capHeight;				/* uppercase height */
	FskFixed				xHeight;				/* lowercase height */
	FskFixed				accentHeight;			/* default = ascent */
	FskFixed				ascent;					/* default = unitsPerEm - vertOriginY */
	FskFixed				descent;				/* default = vertOriginY */
	FskFixed				*widths;				/* ??? */
	FskFixedRectangleRecord	bbox;
	FskFixed				ideographicBaseline;
	FskFixed				alphabeticBaseline;
	FskFixed				mathematicalBaseline;
	FskFixed				hangingBaseline;
	FskFixed				underlinePosition;
	FskFixed				underlineThickness;
	FskFixed				strikethroughPosition;
	FskFixed				strikethroughThickness;
	FskFixed				overlinePosition;
	FskFixed				overlineThickness;
	FskFixed				vIdeographic;
	FskFixed				vAlphabetic;
	FskFixed				vMathematical;
	FskFixed				vHanging;
} FskFontFaceRecord;


/********************************************************************************
 * Gloc
 ********************************************************************************/

typedef struct Gloc	{	/* Glyph info locator */
	UInt32	offset;		/* From the beginning */
	UInt32	size;		/* These are all self-terminating, but we scope them with a size anyway */
} Gloc;


/********************************************************************************
 * FskGlyphRecord
 ********************************************************************************/

typedef struct FskGlyphRecord {
	/* These are stored in the directory */
	UInt8		numChars;		/* The number of characters used in the glyph */
	UInt8		orientation;	/* H or V */
	UInt8		arabicForm;		/* Initial, medial, terminal, isolated */
	UInt8		unused;
	FskFixed	horizAdvX;		/* The horizontal advance after rendering the character in the X direction */
#ifdef NOT_IN_SVG_TINY
	FskFixed	vertOriginX;	/* The X-coordinate in the font coordinate system of the origin of the glyph to be used when drawing vertically oriented text */
	FskFixed	vertOriginY;	/* The Y-coordinate in the font coordinate system of the origin of the glyph to be used when drawing vertically oriented text */
	FskFixed	vertAdvanceY;	/* The vertical advance after rendering a glyph in vertical orientation */
#endif /* NOT_IN_SVG_TINY */

	/* These are stored in the BLOB */
	Gloc		uniStrIndex;		/* Index to full UniCode string if more than 1 character */
	Gloc		pathIndex;			/* Path for the glyph */
	Gloc		nameIndex;			/* Index into a list of names */
	Gloc		langCodeIndex;		/* Index into a list of language codes */

//	FskPath		path;				/* Path describing the glyph (4-byte aligned) */
//	char		*multiCharString;	/* Some glyphs are for multiple characters (e.g. ligatures), recorded here, NULL otherwise */
//	char		*name;				/* Name of the glyph, in UTF-8 */
//	char		*langCode;			/* Comma-separated, NULL-terminated list of languages */
} FskGlyphRecord;


/********************************************************************************
 * FskFontRecord
 ********************************************************************************/

typedef struct FskFontRecord {
	FskFixed				horizOriginX;	/* default = 0 */
	FskFixed				horizOriginY;	/* default = 0 */
	FskFixed				horizAdvX;

#ifdef NOT_IN_SVG_TINY
	FskFixed				vertOriginX;	/* The X-coordinate in the font coordinate system of the origin of the glyph to be used when drawing vertically oriented text */
	FskFixed				vertOriginY;	/* The Y-coordinate in the font coordinate system of the origin of the glyph to be used when drawing vertically oriented text */
	FskFixed				vertAdvanceY;	/* The vertical advance after rendering a glyph in vertical orientation */
	FskKernSpec				*vKern;
#endif /* NOT_IN_SVG_TINY */

	FskKernSpec				*hKern;

	FskFontFaceRecord		face;

	FskGrowableBlobArray	glyphs;
} FskFontRecord;


/********************************************************************************
 * FskFontSpecSetDefaults
 ********************************************************************************/

void
FskFontSpecSetDefaults(FskFontSpec *fs)
{
	FskMemSet(fs, 0, sizeof(FskFontSpec));

	fs->face.fontStyle		= kFskFontFaceStyleAll;
	fs->face.fontVariant	= kFskFaceFontVariantNormal;
	fs->face.fontWeight		= kFskFontFaceWeightAll;
	fs->face.fontStretch	= kFskFontFaceStretchAll;
}


/********************************************************************************
 * StringClone
 ********************************************************************************/

static FskErr
StringClone(const char *fr, char **to)
{
	FskErr	err	= kFskErrNone;

	*to = NULL;
	if (fr != NULL)
		err = FskMemPtrNewFromData(FskStrLen(fr) + 1, fr, (FskMemPtr*)to);

	return err;
}


/********************************************************************************
 * FskKernSpecDispose
 ********************************************************************************/

void
FskKernSpecDispose(FskKernSpec *ks)
{
	if (ks != NULL) {
		FskKernSpec	*kp;
		for (kp = ks; ks->k != 0; kp++) {
			FskMemPtrDispose((FskMemPtr)(kp->u1));
			FskMemPtrDispose((FskMemPtr)(kp->g1));
			FskMemPtrDispose((FskMemPtr)(kp->u2));
			FskMemPtrDispose((FskMemPtr)(kp->g2));
		}
		FskMemPtrDispose(ks);
	}
}


/********************************************************************************
 * KernSpecClone
 ********************************************************************************/

static FskErr
KernSpecClone(const FskKernSpec *fr, FskKernSpec **top)
{
	FskErr		err	= kFskErrNone;
	UInt32		n;
	FskKernSpec	*to;

	*top = NULL;
	if (fr != NULL)	 {
		for (n = 0; fr->k != 0; n++, fr++)
			continue;
		fr -= n;
		err = FskMemPtrNewClear((n + 1) * sizeof(FskKernSpec), (FskMemPtr*)top);
		for (to = *top; n--; fr++, to++) {
			BAIL_IF_ERR(err = StringClone(fr->u1, &to->u1));
			BAIL_IF_ERR(err = StringClone(fr->g1, &to->g1));
			BAIL_IF_ERR(err = StringClone(fr->u2, &to->u2));
			BAIL_IF_ERR(err = StringClone(fr->g2, &to->g2));
			to->k = fr->k;
		}
	}

bail:
	if (err != kFskErrNone)	{
		FskKernSpecDispose(*top);
		*top = NULL;
	}
	return err;
}


/********************************************************************************
 * UnicodeRangeClone
 ********************************************************************************/

static FskErr
UnicodeRangeClone(const FskUnicodeRange *fr, FskUnicodeRange **to)
{
	FskErr	err	= kFskErrNone;

	*to = NULL;
	if (fr != NULL) {
		UInt32 n = 0;
		for ( ; (fr->first != 0) && (fr->last >= fr->first); n++, fr++)
			continue;
		fr -= n;
		err = FskMemPtrNewFromData((n + 1) * sizeof(FskUnicodeRange), fr, (FskMemPtr*)to);
	}

	return err;
}


/********************************************************************************
 * SizeOfUnicodeRange
 ********************************************************************************/

static UInt32
SizeOfUnicodeRange(const FskUnicodeRange *rg)
{
	UInt32 n = 0;

	if (rg != NULL)
		for ( ; (rg->first != 0) && (rg->last >= rg->first); n++, rg++)
			n += rg->last - rg->first + 1;

	return n;
}


/********************************************************************************
 * FskFontSpecCleanup
 ********************************************************************************/

void
FskFontSpecCleanup(FskFontSpec *fs)
{
	if (fs != NULL) {
		FskKernSpecDispose(fs->hKern);
		FskPathDispose(fs->missingGlyphPath);
		FskMemPtrDispose((FskMemPtr)(fs->face.unicodeRange));
		FskMemPtrDispose((FskMemPtr)(fs->face.fontFamily));
		FskMemPtrDispose((FskMemPtr)(fs->face.fontSize));
		FskMemPtrDispose((FskMemPtr)(fs->face.widths));
		#ifdef NOT_IN_SVG_TINY
			FskKernSpecDispose(fs->vKern);
		#endif /* NOT_IN_SVG_TINY */
	}
}


/********************************************************************************
 * FskFontSpecDispose
 ********************************************************************************/

void
FskFontSpecDispose(FskFontSpec *fs)
{
	FskFontSpecCleanup(fs);
	FskMemPtrDispose((FskMemPtr)(fs));
}


/********************************************************************************
 * FskFontDispose
 ********************************************************************************/

void
FskFontDispose(FskFont ft)
{
	if (ft != NULL) {
		FskGrowableBlobArrayDispose(ft->glyphs);
		FskKernSpecDispose(ft->hKern);
		FskMemPtrDispose(ft->face.fontFamily);
		FskMemPtrDispose(ft->face.fontSize);
		FskMemPtrDispose(ft->face.unicodeRange);
		FskMemPtrDispose(ft->face.widths);
		#ifdef NOT_IN_SVG_TINY
			FskKernSpecDispose(ft->vKern);
		#endif /* NOT_IN_SVG_TINY */

		FskMemPtrDispose(ft);
	}
}


#define EXPECTED_GLYPH_BLOB_SIZE	128
#define EXPECTED_MAX_GLYPHS			16


/********************************************************************************
 * FskFontNew
 ********************************************************************************/

FskErr
FskFontNew(const FskFontSpec *fs, FskFont *ftp)
{
	FskErr	err;
	FskFont	ft		= NULL;
	UInt16	uc[2];

	*ftp = NULL;
	BAIL_IF_ERR(err = FskMemPtrNewClear(sizeof(FskFontRecord), (FskMemPtr*)(void*)(&ft)));

	ft->horizOriginX		= fs->horizOriginX;
	ft->horizOriginY		= fs->horizOriginY;
	ft->horizAdvX			= fs->horizAdvX;
	BAIL_IF_ERR(err = KernSpecClone(fs->hKern, &ft->hKern));

#ifdef NOT_IN_SVG_TINY
	ft->vertOriginX			= fs->vertOriginX;
	ft->vertOriginY			= fs->vertOriginY;
	ft->vertAdvanceY		= fs->vertAdvanceY;
	BAIL_IF_ERR(err = KernSpecClone(fs->vKern, &ft->vKern));
#endif /* NOT_IN_SVG_TINY */

	/* Font face */
	BAIL_IF_ERR(err = StringClone((const char*)(fs->face.fontFamily), (char**)(void*)(&ft->face.fontFamily)));
	BAIL_IF_ERR(err = UnicodeRangeClone(fs->face.unicodeRange, &ft->face.unicodeRange));
	BAIL_IF_ERR(err = StringClone((const char*)(fs->face.fontSize), (char**)(void*)(&ft->face.fontSize)));
	BAIL_IF_ERR(err = StringClone((const char*)(fs->face.widths), (char**)(void*)(&ft->face.widths)));
	FskMemCopy(ft->face.panose1, fs->face.panose1, 10);
	ft->face.unitsPerEm				= fs->face.unitsPerEm;	/* Original design space */
	ft->face.fontStyle				= fs->face.fontStyle;
	ft->face.fontVariant			= fs->face.fontVariant;
	ft->face.fontWeight				= fs->face.fontWeight;
	ft->face.fontStretch			= fs->face.fontStretch;
	ft->face.stemV					= fs->face.stemV;
	ft->face.stemH					= fs->face.stemH;
	ft->face.slope					= fs->face.slope;
	ft->face.capHeight				= fs->face.capHeight;
	ft->face.xHeight				= fs->face.xHeight;
	ft->face.accentHeight			= fs->face.accentHeight;
	ft->face.ascent					= fs->face.ascent;
	ft->face.descent				= fs->face.descent;
	ft->face.bbox					= fs->face.bbox;
	ft->face.ideographicBaseline	= fs->face.ideographicBaseline;
	ft->face.alphabeticBaseline		= fs->face.alphabeticBaseline;
	ft->face.mathematicalBaseline	= fs->face.mathematicalBaseline;
	ft->face.hangingBaseline		= fs->face.hangingBaseline;
	ft->face.underlinePosition		= fs->face.underlinePosition;
	ft->face.underlineThickness		= fs->face.underlineThickness;
	ft->face.strikethroughPosition	= fs->face.strikethroughPosition;
	ft->face.strikethroughThickness	= fs->face.strikethroughThickness;
	ft->face.overlinePosition		= fs->face.overlinePosition;
	ft->face.overlineThickness		= fs->face.overlineThickness;
#ifdef NOT_IN_SVG_TINY
	ft->face.vIdeographic			= fs->face.vIdeographic;
	ft->face.vAlphabetic			= fs->face.vAlphabetic;
	ft->face.vMathematical			= fs->face.vMathematical;
	ft->face.vHanging				= fs->face.vHanging;
#endif /* NOT_IN_SVG_TINY */

	err = FskGrowableBlobArrayNew(SizeOfUnicodeRange(ft->face.unicodeRange), EXPECTED_MAX_GLYPHS, sizeof(FskGlyphRecord), &(ft->glyphs));
    BAIL_IF_ERR(err);

	uc[0] = uc[1] = 0;						/* The zero character code point is used to store the missing glyph */
	err = FskFontAddGlyph(
		ft,									/* The font */
		uc,									/* Zero character code */
		(UInt8*)"missing glyph",			/* Glyph name */
		fs->missingGlyphPath,				/* The path of the missing glyph */
		NULL,								/* Language code */
		kFskFontGlyphOrientationAll,		/* H or V */
		kFskFontGlyphArabicFormIsolated,	/* Initial, medial, terminal, isolated */
		fs->missingGlyphHorizAdvanceX		/* The horizontal advance after rendering the character in the X direction */
	#ifdef NOT_IN_SVG_TINY
	,	fs->missingGlyphVertOriginX,		/* The X-coordinate in the font coordinate system of the origin of the glyph to be used when drawing vertically oriented text (not in tiny) */
		fs->missingGlyphVertOriginY,		/* The Y-coordinate in the font coordinate system of the origin of the glyph to be used when drawing vertically oriented text (not in tiny) */
		fs->missingGlyphVertAdvanceY		/* The vertical advance after rendering a glyph in vertical orientation (not in tiny) */
	#endif /* NOT_IN_SVG_TINY */
	);

	*ftp = ft;

bail:
	if (err != kFskErrNone) {
		FskFontDispose(ft);
	}

	return err;
}


/********************************************************************************
 * SetGloc
 ********************************************************************************/

static void
SetGloc(Gloc *gloc, UInt32 *offset, UInt32 size)
{
	gloc->offset	= *offset;			/* Store offset */
	gloc->size		= size;				/* Store size */
	size			= (size + 3) & ~3;	/* Bump size up to multiple of 4 bytes */
	*offset			+= size;			/* Update offset */
}


/********************************************************************************
 * HashCharString
 ********************************************************************************/

static UInt32
HashCharString(UInt16 *str, UInt32 strLen)
{
	UInt32	hash;

	if (strLen == 0)	strLen = FskUnicodeStrLen(str);
	hash = *str++;
	if (strLen > 1) {
		SInt32	diff;
		UInt32	shift;
		SInt32	ref		= hash;

		for (shift = 21, strLen--; strLen--; shift += 6) {	/* @@@ This doesn't work!!! */
			diff = (SInt32)(*str++) - ref;
			hash ^= diff << shift;
		}
	}

	return hash;
}


/********************************************************************************
 * FskFontAddGlyph
 ********************************************************************************/

FskErr
FskFontAddGlyph(
	FskFont			font,
	UInt16			*uniChar,		/* Usually a single character, but may be a ligature (CANNOT be NULL) */
	UInt8			*name,			/* The name of the glyph (can be NULL) */
	FskConstPath	path,			/* The path of the glyph (CANNOT be NULL) */
	UInt8			*langCode,		/* The language codes, comma-separated (can be NULL) */
	UInt8			orientation,	/* H or V */
	UInt8			arabicForm,		/* Initial, medial, terminal, isolated */
	FskFixed		horizAdvX		/* The horizontal advance after rendering the character in the X direction */
#ifdef NOT_IN_SVG_TINY
,	FskFixed		vertOriginX,	/* The X-coordinate in the font coordinate system of the origin of the glyph to be used when drawing vertically oriented text (not in tiny) */
	FskFixed		vertOriginY,	/* The Y-coordinate in the font coordinate system of the origin of the glyph to be used when drawing vertically oriented text (not in tiny) */
	FskFixed		vertAdvanceY	/* The vertical advance after rendering a glyph in vertical orientation (not in tiny) */
#endif /* NOT_IN_SVG_TINY */
)
{
	FskErr				err;
	FskGlyphRecord		glyph;
	UInt32				offset, id;
	char				*blob;
	FskGlyphRecord		*dir;
	FskFixedMatrix3x2	M;

	glyph.numChars		= (UInt8)FskUnicodeStrLen(uniChar);	/* This is a bit redundant with glyph.uniStrIndex.size */
	glyph.orientation	= orientation;
	glyph.arabicForm	= arabicForm;
	glyph.horizAdvX		= horizAdvX;
	glyph.unused		= 0;
#ifdef NOT_IN_SVG_TINY
	glyph.vertOriginX	= vertOriginX;		/* not used in tiny */
	glyph.vertOriginY	= vertOriginY;		/* not used in tiny */
	glyph.vertAdvanceY	= vertAdvanceY;		/* not used in tiny */
#endif /* NOT_IN_SVG_TINY */

	offset = 0;
	SetGloc(&glyph.uniStrIndex,		&offset,	(glyph.numChars + 1) * sizeof(FskUniChar));							/* required */
	SetGloc(&glyph.pathIndex,		&offset,	FskPathSize(path));													/* required */
	SetGloc(&glyph.nameIndex,		&offset,	(name     && *name    ) ? (FskStrLen((char*)name)     + 1) : 0);	/* optional */
	SetGloc(&glyph.langCodeIndex,	&offset,	(langCode && *langCode) ? (FskStrLen((char*)langCode) + 1) : 0);	/* optional */

	id = HashCharString(uniChar, 0);
	BAIL_IF_ERR(err = FskGrowableBlobArrayGetPointerToNewEndItem(font->glyphs, offset, &id, (void**)(void*)(&blob), (void**)(void*)(&dir)));
	*dir = glyph;
	FskMemCopy(blob + glyph.uniStrIndex.offset, uniChar, glyph.uniStrIndex.size);
	FskMemCopy(blob + glyph.pathIndex.offset,   path,    glyph.pathIndex.size);
	M.M[1][1] = -(M.M[0][0] = FskFixDiv((1 << 16), font->face.unitsPerEm));
	M.M[0][1] = M.M[1][0] = M.M[2][0] = M.M[2][1] = 0;
	FskPathTransform((FskPath)(blob + glyph.pathIndex.offset), &M);


	if (glyph.nameIndex.size     > 0)	FskMemCopy(blob + glyph.nameIndex.offset, 		name,		glyph.nameIndex.size);
	if (glyph.langCodeIndex.size > 0)	FskMemCopy(blob + glyph.langCodeIndex.offset,	langCode,	glyph.langCodeIndex.size);

bail:
	return err;
}

/********************************************************************************
 * FskFontUnicodeToGlyphString
 ********************************************************************************/

FskErr
FskFontDoneEditing(FskFont font)
{
	return FskGrowableBlobArraySortItemsByID(font->glyphs);
}


/********************************************************************************
 * FskFontFindGlyph
 * returns the number of characters used by the returned glyph
 ********************************************************************************/

static UInt32
FskFontFindGlyph(FskConstFont font, const UInt16 *uniChars, UInt32 numUniChars, UInt16 *glyph, UInt32 *glyphsGend)
{
	FskErr	err				= kFskErrItemNotFound;
	UInt32	usedUniChars	= 1;
	UInt32	index			= 0;

	if (glyphsGend != NULL)	*glyphsGend = 1;	/* We will orginarily only generate 1 glyph */

	/* Look for ligatures */
#ifdef UNIMPLEMENTED
	if (err == kFskErrNone)
		goto done;
#endif /* UNIMPLEMENTED */

	/* No ligatures found, get a single character */
	if ((err = FskGrowableBlobArrayGetIndexFromIDOfItem(font->glyphs, uniChars[0], &index)) == kFskErrNone)
		goto done;

	/* No single character found, look for decomposition into multiple glyphs */
#ifdef UNIMPLEMENTED
	if (err == kFskErrNone)
		goto done;
#else /* UNIMPLEMENTED */
	(void)numUniChars;	/* Gets rid of unused warnings */
#endif /* UNIMPLEMENTED */

	/* No decomposition found: return the index of the missing glyph */
	index = 0;

done:
	if (glyph != NULL)	*glyph = (UInt16)index;
	return usedUniChars;
}


/********************************************************************************
 * FskFontNewGlyphStringFromUnicodeString
 ********************************************************************************/

FskErr
FskFontNewGlyphStringFromUnicodeString(FskConstFont font, const UInt16 *uniChars, UInt32 numUniChars, UInt16 **pGlyphs, UInt32 *pNumGlyphs)
{
	FskErr	err			= kFskErrNone;
	UInt16	*glyphs		= NULL;
	SInt32	numChars;
	UInt32	numGlyphs, glyphsGend, charsUsed;

	if (pNumGlyphs != NULL)	*pNumGlyphs	= 0;
	if (pGlyphs != NULL) {
		*pGlyphs	= NULL;
		BAIL_IF_ERR(err = FskMemPtrNew(numUniChars * sizeof(UInt16), (FskMemPtr*)pGlyphs));		/* Allocate glyph storage if asked for it */
		glyphs = *pGlyphs;
	}

	for (numChars = numUniChars, numGlyphs = 0; numChars > 0; ) {
		charsUsed = FskFontFindGlyph(font, uniChars, numChars, glyphs, &glyphsGend);	/* Look for the next glyph */
		numChars -= charsUsed;															/* Account for the number of characters used */
		uniChars += charsUsed;															/* Advance past the used characters */
		numGlyphs += glyphsGend;														/* Update the number of glyphs generated */
		if (glyphs != NULL)	glyphs += glyphsGend;										/* Advance past the glyphs generated */
	}

	if (pNumGlyphs != NULL)	*pNumGlyphs = numGlyphs;									/* Return the number of glyphs */

bail:
	return err;
}


/********************************************************************************
 * FskFontGetGlyphPath
 ********************************************************************************/

FskErr
FskFontGetGlyphPath(FskConstFont font, UInt16 glyph, FskConstPath *path, FskFixed *hAdvance)
{
	FskErr					err;
	const char				*blob;
	UInt32					blobSize;
	const FskGlyphRecord	*gRec;

	BAIL_IF_ERR(err = FskGrowableBlobArrayGetConstPointerToItem(font->glyphs, glyph, (const void**)(const void*)(&blob), &blobSize, (const void**)(const void*)(&gRec)));
	if (path     != NULL)	*path     = (FskConstPath)(blob + gRec->pathIndex.offset);
	if (hAdvance != NULL)	*hAdvance = gRec->horizAdvX;

bail:
	return err;
}


/********************************************************************************
 * FskFontAppendGlyphPathToGrowablePath
 ********************************************************************************/

FskErr
FskFontAppendGlyphPathToGrowablePath(FskConstFont font, UInt16 glyph, FskFixed size, FskFixedPoint2D *origin, FskGrowablePath path)
{
	FskErr				err;
	FskConstPath		glyphPath;
	FskFixed			hAdvance;
	FskFixedMatrix3x2	M;

	BAIL_IF_ERR(err = FskFontGetGlyphPath(font, glyph, &glyphPath, &hAdvance));
	M.M[0][0] = M.M[1][1] = size;
	M.M[0][1] = M.M[1][0] = 0;
	M.M[2][0] = origin->x;	M.M[2][1] = origin->y;
	BAIL_IF_ERR(err = FskGrowablePathAppendTransformedPath(glyphPath, &M, path));
	origin->x += FskFixMul(size, hAdvance);

bail:
	return err;
}


/********************************************************************************
 * FskFontCollectionNew
 ********************************************************************************/

FskErr
FskFontCollectionNew(FskFontCollection *fc)
{
	return FskGrowableArrayNew(sizeof(FskFont), 12, fc);
}


/********************************************************************************
 * FskFontCollectionDispose
 ********************************************************************************/

void
FskFontCollectionDispose(FskFontCollection fc)
{
	UInt32	numFonts;
	FskFont	*ft;
	FskErr	err;

	if (fc != NULL) {
		if (	((numFonts = FskGrowableArrayGetItemCount(fc)) > 0)
			&&	((err = FskGrowableArrayGetPointerToItem(fc, 0, (void**)(void*)(&ft))) == kFskErrNone)
		)
			for ( ; numFonts--; ft++)
				FskFontDispose(*ft);
		FskGrowableArrayDispose(fc);
	}
}


/********************************************************************************
 * FskFontCollectionAddNewFont
 ********************************************************************************/

FskErr
FskFontCollectionAddNewFont(const FskFontSpec *fs, FskFontCollection fc, FskFont *ftp)
{
	FskErr	err		= kFskErrNone;
	FskFont ft		= NULL;

	*ftp = NULL;
	BAIL_IF_ERR(err = FskFontNew(fs, &ft));
	BAIL_IF_ERR(err = FskGrowableArrayAppendItem(fc, &ft));
	*ftp = ft;

bail:
	if (err != kFskErrNone) {		/* If unsuccessful ... */
		FskFontDispose(ft);			/* ... dispose of memory allocated here */
	}

	return err;
}


/********************************************************************************
 * SameFontFamily
 ********************************************************************************/

static Boolean
SameFontFamily(const UInt8 *face, const char *attr)
{
	if (FskStrCompareCaseInsensitive((const char*)face, attr) == 0)		return true;

	/* We might want a more extensive comparison */

	return false;
}


/********************************************************************************
 * SameFontStyle
 ********************************************************************************/

static Boolean
SameFontStyle(UInt32 face, UInt32 attr)
{
	attr = (1 << attr);						/* Convert from {0,1,2} to {1,2,4} */
	return ((face & attr) != 0);
}


/********************************************************************************
 * SameFontWeight
 ********************************************************************************/

static Boolean
SameFontWeight(UInt32 face, UInt32 attr)
{
	attr = (attr - 50) / 100;				/* Convert from {100, 200, 300, ...} to {0, 1, 2, 3, ...} */
	attr = (1 << attr);						/* Convert from {0, 1, 2, 3, ...}    to {1<<0, 1<<1, ...} bit mask */
	return ((face & attr) != 0);
}


/********************************************************************************
 * GetGlyphBlobData, GetGlyphCharacterString, GetGlyphPath, GetGlyphName, GetGlyphLanguageCode
 * Accessors to glyph blob data
 ********************************************************************************/

#ifdef UNUSED
static char*	GetGlyphBlobData(       Gloc           *gloc,     char *blob)	{	return (gloc->size > 0) ? (blob + gloc->offset) : NULL;				}
static UInt8*	GetGlyphCharacterString(FskGlyphRecord *glyphDir, char *blob)	{	return ( UInt8*)GetGlyphBlobData(&glyphDir->uniStrIndex,   blob);	}
static FskPath	GetGlyphPath(           FskGlyphRecord *glyphDir, char *blob)	{	return (FskPath)GetGlyphBlobData(&glyphDir->pathIndex,     blob);	}
static UInt8*	GetGlyphName(           FskGlyphRecord *glyphDir, char *blob)	{	return ( UInt8*)GetGlyphBlobData(&glyphDir->nameIndex,     blob);	}
static UInt8*	GetGlyphLanguageCode(   FskGlyphRecord *glyphDir, char *blob)	{	return ( UInt8*)GetGlyphBlobData(&glyphDir->langCodeIndex, blob);	}
#endif /* UNUSED */


/********************************************************************************
 * SameFontStretch
 ********************************************************************************/

static Boolean
SameFontStretch(UInt32 face, UInt32 attr)
{
	attr = (1 << (attr - 1));				/* Convert from {1,2,3,...} to {1<<0, 1<<1, 1<<2, ...} */
	return ((face & attr) != 0);
}


/********************************************************************************
 * FskFskFontCollectionFindFont
 ********************************************************************************/

FskConstFont
FskFskFontCollectionFindFont(FskConstFontCollection fc, const FskFontAttributes *attributes)
{
	FskConstFont	font		= NULL;
	FskConstFont	fp, *fa;
	UInt32			numFonts, i;

	numFonts = FskGrowableArrayGetItemCount(fc);
	(void)FskGrowableArrayGetConstPointerToItem(fc, 0, (const void**)(const void*)(&fa));

	/* Look for exact match */
	for (i = numFonts ; i--; fa++) {
		fp = *fa;
		if (	(SameFontFamily(	fp->face.fontFamily,	attributes->family))	/* e.g. "Helvetica" */
			&&	(SameFontStyle(		fp->face.fontStyle,		attributes->style))		/* normal, italic, oblique */
			&&	(SameFontWeight(	fp->face.fontWeight,	attributes->weight))	/* normal, bold, 100, 200, ... 900 */
			&&	(SameFontStretch(	fp->face.fontStretch,	attributes->stretch))	/* normal, {ultra-,extra-,semi-,}{condensed,expanded}} */
			&&	(					fp->face.fontVariant ==	attributes->variant)	/* normal, small-caps */
		) {
			font = fp;
			break;
		}
	}

#if 0
	/* Look for approximate match */
	if (font == NULL) for (i = numFonts, fp -= numFonts; i--; fp++) {
		if (	(AlmostSameFontFamily(	fp->face.fontFamily,	attributes->family))	/* e.g. "Helvetica" */
			&&	(AlmostSameFontStyle(	fp->face.fontStyle,		attributes->style))		/* normal, italic, oblique */
			&&	(AlmostSameFontWeight(	fp->face.fontWeight,	attributes->weight))	/* normal, bold, 100, 200, ... 900 */
			&&	(AlmostSameFontStretch(	fp->face.fontStretch,	attributes->stretch))	/* normal, {ultra-,extra-,semi-,}{condensed,expanded}} */
			&&	(						fp->face.fontVariant ==	attributes->variant)	/* normal, small-caps */
		) {
			font = fp;
			break;
		}
	}
#endif

	return font;
}


/********************************************************************************
 * FskFskFontCollectionNewGrowablePathFromUnicodeString
 ********************************************************************************/

#define EXPECTED_BYTES_PER_SEGMENT			40
#define EXPECTED_SEGMENTS_PER_GLYPH_PATH	32
#define EXPECTED_BYTES_PER_GLYPH_PATH		(EXPECTED_SEGMENTS_PER_GLYPH_PATH * EXPECTED_BYTES_PER_SEGMENT)

FskErr
FskFontCollectionNewGrowablePathFromUnicodeString(
	FskConstFontCollection	fc,
	const UInt16			*uniChars,
	UInt32					numUniChars,
	const FskFontAttributes	*attributes,
	FskFixedPoint2D			*origin,
	FskGrowablePath			*path
)
{
	FskErr			err				= kFskErrNone;
	UInt16			*glyphs			= NULL;
	FskFixed		size;
	FskFixedPoint2D	org;
	FskConstFont	font;
	UInt32			numGlyphs, i;

	*path = NULL;
	BAIL_IF_NULL((font = FskFskFontCollectionFindFont(fc, attributes)), err, kFskErrItemNotFound);
	BAIL_IF_ERR(err = FskGrowablePathNew(numUniChars * EXPECTED_BYTES_PER_GLYPH_PATH, path));
	BAIL_IF_ERR(err = FskFontNewGlyphStringFromUnicodeString(font, uniChars, numUniChars, &glyphs, &numGlyphs));

	if (origin != NULL)	{	org   = *origin;			}
	else				{	org.x = 0;		org.y = 0;	}
	size = FskRoundAndSaturateFloatToFixed(attributes->size);
	for (i = 0; i < numGlyphs; i++) {
		BAIL_IF_ERR(err = FskFontAppendGlyphPathToGrowablePath(font, glyphs[i], size, &org, *path));
	}
	if (origin != NULL)	*origin = org;														/* Update the origin */

bail:
	FskMemPtrDispose(glyphs);
	if (err != kFskErrNone) {
		FskGrowablePathDispose(*path);
		*path = NULL;
	}
	return err;
}
