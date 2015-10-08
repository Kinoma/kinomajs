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
#ifdef _WIN32
	#if defined(_WIN32_WINNT)
		#undef _WIN32_WINNT
	#endif /* !defined(_WIN32_WINNT) */
	#define _WIN32_WINNT  0x0500	/* Peter SAYS that this will work */
#endif /* _WIN32 */

#include "Fsk.h"

#ifdef _WIN32
	#include <Windows.h>			/* 	#include <Wingdi.h> */
#endif /* _WIN32 */

#if TARGET_OS_MAC && USE_CORE_TEXT
	#include <CoreText/CoreText.h>
#endif /* USE_CORE_TEXT */

#include "FskGlyphPath.h"


//#define TEST_LINUX_ON_MAC
#ifdef TEST_LINUX_ON_MAC
	#undef TARGET_OS_MAC
	#undef TARGET_OS_LINUX
	#define TARGET_OS_MAC		0
	#define TARGET_OS_LINUX	1
#endif /* TEST_LINUX_ON_MAC */


#if SUPPORT_INSTRUMENTATION
	#define LOG_PARAMETERS				/**< Log the parameters of API calls. */
	//#define LOG_FONT_MATCH			/**< Log the process of matching fonts. */

	#define FskInstrumentedGlobalType(type)			(&(g##type##TypeInstrumentation))												/**< The data structure created with FskInstrumentedSimpleType(). */
	#define FskInstrumentedTypeEnabled(type, level)	FskInstrumentedTypeHasListenersForLevel(FskInstrumentedGlobalType(type), level)	/**< Whether instrumentation will print anything. */
#else /* !SUPPORT_INSTRUMENTATION */
	#define FskInstrumentedTypeEnabled(type, level)	(0)																				/**< Instrumentation will not print anything. */
#endif /* SUPPORT_INSTRUMENTATION */
#ifndef GLYPH_DEBUG
	#define GLYPH_DEBUG 0				/**< Turn off extra debugging logs. */
#endif /* GLYPH_DEBUG */
#if									\
	defined(LOG_PARAMETERS)			|| \
	defined(LOG_FONT_MATCH)
	#undef  GLYPH_DEBUG
	#define GLYPH_DEBUG 1
#endif /* LOG_PARAMETERS et al */

FskInstrumentedSimpleType(GlyphPath, glyphpath);												/**< This declares the types needed for instrumentation. */

#if GLYPH_DEBUG
	#define	LOGD(...)	FskGlyphPathPrintfDebug(__VA_ARGS__)									/**< Print debugging logs. */
	#define	LOGI(...)	FskGlyphPathPrintfVerbose(__VA_ARGS__)									/**< Print information logs. */
#endif /* GLYPH_DEBUG */
#define		LOGE(...)	FskGlyphPathPrintfMinimal(__VA_ARGS__)									/**< Print error logs always, when instrumentation is on. */
#ifndef     LOGD
	#define LOGD(...)	do {} while(0)															/**< Don't print debugging logs. */
#endif   	/* LOGD */
#ifndef     LOGI
	#define LOGI(...)	do {} while(0)															/**< Don't print information logs. */
#endif   	/* LOGI */
#ifndef     LOGE
	#define LOGE(...)	do {} while(0)															/**< Don't print error logs. */
#endif   	/* LOGE */
#define LOGD_ENABLED()	FskInstrumentedTypeEnabled(GlyphPath, kFskInstrumentationLevelDebug)		/**< Whether LOGD() will print anything. */
#define LOGI_ENABLED()	FskInstrumentedTypeEnabled(GlyphPath, kFskInstrumentationLevelVerbose)	/**< Whether LOGI() will print anything. */
#define LOGE_ENABLED()	FskInstrumentedTypeEnabled(GlyphPath, kFskInstrumentationLevelMinimal)	/**< Whether LOGE() will print anything. */



#define UNUSED(x)	((void)(x))

#define USE_UNHINTED_OUTLINES

#if TARGET_OS_MAC || TARGET_OS_WIN32
static const char	*gDefaultFontNames[] = {
	"Helvetica",
	"Arial",
	"Verdana",
	"Times",
	NULL
};
#endif


#if GLYPH_DEBUG
#include "FskTextConvert.h"

static void LogAttributes(const FskFontAttributes *a) {
	static const char *anchor[]		= { "start", "middle", "end" };
	static const char *style[]		= { "normal", "italic", "oblique" };
	static const char *stretch[]	= { NULL, "UltraCondensed" "ExtraCondensed" "Condensed" "SemiCondensed" "Normal" "SemiExpanded" "Expanded" "ExtraExpanded" "UltraExpanded" };

	if (!a)
		return;

	LOGD("\tAttributes(family=\"%s\" size=%g weight=%u style=%s anchor=%s stretch=%s decoration=%u variant=%u sizeadjust=%g)",
		a->family, a->size, a->weight, style[a->style], anchor[a->anchor], stretch[a->stretch], a->decoration, a->variant, a->sizeAdjust);
}

static void LogGrowablePathFromUnicodeStringNew(
	const UInt16			*uniChars,
	UInt32					numUniChars,
	const FskFontAttributes	*attributes,
	FskFixedPoint2D			*origin,
	FskGrowablePath			*path
) {
	char *utf8 = NULL;
	UInt32 utfBytes;
	(void)FskTextUnicode16NEToUTF8(uniChars, numUniChars*sizeof(*uniChars), &utf8, &utfBytes);
	LOGD("GrowablePathFromUnicodeStringNew(unichars=%*s numUniChars=%u attributes=%p origin=(%g,%g) path=%p",
		(int)utfBytes, utf8, numUniChars, attributes, origin->x/65536., origin->y/65536., path);
	FskMemPtrDispose(utf8);
	LogAttributes(attributes);
}

#endif /* GLYPH_DEBUG */


/********************************************************************************
 * GetNextTokenInCommaSeparatedList
 ********************************************************************************/

static int
GetNextTokenInCommaSeparatedList(const char **inStrP, char *outStr, int maxChars)
{
	const char *inStr	= *inStrP;
	int			quoted	= 0;
	int			gotOne	= 0;
	char		c;

	/* Skip initial spaces and look for quotes */
	for ( ;  ; inStr++) {
		c = *inStr;
		if ((c == ' ') || (c == '\t') || (c == '\n') || (c == '\r'))	continue;
		if ((c == '\'') || (c == '"'))									quoted = *inStr++;
		break;
	}

	/* Look to see if we gobbled everything up */
	if (*inStr == 0)	goto done;		/* End of list */

	/* The first character is now a non-space */
	gotOne = 1;
	for (maxChars-- ; (maxChars > 0) && ((*outStr = c = *inStr++) != 0); outStr++, maxChars--) {					/* Gather characters of token */
		if (quoted) {
			if (c != quoted)	continue;											/* Transfer all characters until a matching quote is found */

			/* End of quote */
			for ( ; ((c = *inStr) == ' ') || (c == '\t') || (c == '\n') || (c == '\r'); inStr++)	continue;		/* Gobble up spaces */
			if (c == ',')	inStr++;																				/* Gobble up the comma */
			break;																									/* End of token */
		}
		else {	/* Not quoted */
			#ifdef SPACES_TERMINATE_TOKENS	/* @@@ Why was this put in again??? */
			if ((c ==' ') || (c == '\t') || (c == '\n') || (c == '\r')) {											/* Spaces terminate a token */
				for ( ; ((c = *inStr) == ' ') || (c == '\t') || (c == '\n') || (c == '\r'); inStr++)	continue;	/* Gobble up additional spaces */
				if ((c = *inStr) == ',')	inStr++;																/* Gobble up a comma if it exists */
				break;																								/* End of token */
			}
			#endif /* SPACES_TERMINATE_TOKENS */
			if (c == ',')	{	inStr++;	break;	}																/* Gobble up the comma */
		}
	}

	/* Trim trailing spaces */
	while (((c = outStr[-1]) == ' ') || (c == '\t') || (c == '\n') || (c == '\r'))
		outStr--;

done:
	*outStr++ = 0;	/* Double-terminate for Freetype, because it expects a list of strings. */
	*outStr   = 0;
	*inStrP = inStr;
	return gotOne;
}


#if 0
#pragma mark -
#pragma mark Mac
#endif
/********************************************************************************
 ********************************************************************************
 ********************************************************************************
 *****	MAC -- MAC -- MAC -- MAC -- MAC -- MAC -- MAC -- MAC -- MAC -- MAC	*****
 ********************************************************************************
 ********************************************************************************
 ********************************************************************************/
#if TARGET_OS_MAC && USE_CORE_TEXT

struct FskTextContextRecord {
	FskGrowablePath	path;
	CTFontRef font;
	CGPoint origin;
	CGPoint currentPathAdvance;
	FskErr err;
};

#define XCOORD(xx)		(textContext->origin.x + textContext->currentPathAdvance.x + (xx))
#define YCOORD(yy)		(textContext->origin.y + textContext->currentPathAdvance.y - (yy))

static CTFontRef
GetFontWithCString(const char *name, const FskFontAttributes *at)
{
	CTFontRef font = NULL;
	CFStringRef nameString = NULL;
	Boolean genericFamilyName = false;
	CFStringRef familyName = NULL;
	CTFontSymbolicTraits symbolicTraits = 0;
	FskErr err = kFskErrNone;

	if (FskStrCompare(name, "sans-serif") == 0)
	{
#if TARGET_OS_IPHONE
		name = "Helvetica";
#else
		name = "Arial";
#endif
		genericFamilyName = true;
	}
	else if (FskStrCompare(name, "serif") == 0)
	{
		name = "Times";
		genericFamilyName = true;
	}
	else if (FskStrCompare(name, "cursive") == 0)
	{
		name = "Zapfino";	// ??
		genericFamilyName = true;
	}
	else if (FskStrCompare(name, "fantasy") == 0)
	{
		name = "Herculanum";	// ??
		genericFamilyName = true;
	}
	else if (FskStrCompare(name, "monospace") == 0)
	{
		name = "Courier New";
		genericFamilyName = true;
	}

	nameString = CFStringCreateWithCString(kCFAllocatorDefault, name, kCFStringEncodingUTF8);
	BAIL_IF_NULL(nameString, err, kFskErrOperationFailed);

	font = CTFontCreateWithName(nameString, at->size, NULL);
	BAIL_IF_NULL(font, err, kFskErrOperationFailed);

	if (!genericFamilyName)
	{
		familyName = CTFontCopyFamilyName(font);
		BAIL_IF_NULL(familyName, err, kFskErrOperationFailed);

		/* do not want font substitution if family name is specified. */
		BAIL_IF_FALSE((CFStringCompare(familyName, nameString, kCFCompareCaseInsensitive) == kCFCompareEqualTo), err, kFskErrOperationFailed);
	}

	if (at->weight >= ((kFskFontWeightBold + kFskFontWeightNormal)/2))				symbolicTraits |= kCTFontBoldTrait;
	if ((at->style == kFskFontStyleItalic) || (at->style == kFskFontStyleOblique))	symbolicTraits |= kCTFontItalicTrait;
	if (at->stretch < kFskFontStretchSemiCondensed)									symbolicTraits |= kCTFontCondensedTrait;
	if (at->stretch > kFskFontStretchSemiExpanded)									symbolicTraits |= kCTFontExpandedTrait;
//	if (at->decoration & kFskFontDecorationUnderline)	/* underline is not supported by font? */

	if (symbolicTraits != 0)
	{
		CTFontRef traitsFont;

		traitsFont = CTFontCreateCopyWithSymbolicTraits(font, at->size, NULL, symbolicTraits, symbolicTraits);
		if (traitsFont != NULL)
		{
			CFRelease(font);
			font = traitsFont;
		}
	}

bail:
	if (familyName) CFRelease(familyName);
	if (nameString) CFRelease(nameString);
	if (err)
	{
		if (font) CFRelease(font);
		font = NULL;
	}

	return font;
}

static void
MyCGPathApplierFunction(void *info, const CGPathElement *element)
{
	FskTextContext textContext = (FskTextContext)info;
	CGPoint *points = element->points;
	CGPathElementType type = element->type;
	FskErr err = kFskErrNone;

	if (textContext->err)
	{
		return;
	}

	switch(type) {
		case kCGPathElementMoveToPoint:
			err = FskGrowablePathAppendSegmentFloatMoveTo(XCOORD(points[0].x), YCOORD(points[0].y), textContext->path);
			break;

		case kCGPathElementAddLineToPoint:
			err = FskGrowablePathAppendSegmentFloatLineTo(XCOORD(points[0].x), YCOORD(points[0].y), textContext->path);
			break;

		case kCGPathElementAddQuadCurveToPoint:
			err = FskGrowablePathAppendSegmentFloatQuadraticBezierTo(XCOORD(points[0].x), YCOORD(points[0].y),
																	 XCOORD(points[1].x), YCOORD(points[1].y),
																	 textContext->path);
			break;

		case kCGPathElementAddCurveToPoint:
			err = FskGrowablePathAppendSegmentFloatCubicBezierTo(XCOORD(points[0].x), YCOORD(points[0].y),
																 XCOORD(points[1].x), YCOORD(points[1].y),
																 XCOORD(points[2].x), YCOORD(points[2].y),
																 textContext->path);
			break;

		case kCGPathElementCloseSubpath:
			err = FskGrowablePathAppendSegmentClose(textContext->path);
			break;
	}

	textContext->err = err;
}

/********************************************************************************
 * FskTextContextFromFontAttributesNew
 ********************************************************************************/

FskErr
FskTextContextFromFontAttributesNew(const FskFontAttributes *at, FskTextContext *pTextContext)
{
	FskTextContext textContext;
	CTFontRef font = NULL;
	FskErr err;

	/* Allocate context and clear */
	BAIL_IF_ERR(err = FskMemPtrNewClear(sizeof(struct FskTextContextRecord), (FskMemPtr*)pTextContext));
	textContext = *pTextContext;

	/* Determine the font family, or one similar to it */
	if (at->family != NULL) {		/* Parse the font family */
		const char *fs;
		char fontName[256];
		for (fs = at->family; GetNextTokenInCommaSeparatedList(&fs, fontName, sizeof(fontName));) {
			if ((font = GetFontWithCString(fontName, at)) != NULL) {
				LOGD("Succeeded getting text context from \"%s\"", fontName);
				break;
			}
			LOGD("Failed    getting text context from \"%s\"", fontName);
		}
	}
	if (font == NULL) {		/* Couldn't find the fonts - choose one from our default list */
		const char **fl;
		for (fl = gDefaultFontNames; *fl != NULL; fl++) {
			if ((font = GetFontWithCString(*fl, at)) != NULL) {
				LOGD("Succeeded getting text context from default \"%s\"", *fl);
				break;
			}
			LOGD("Failed    getting text context from default \"%s\"", *fl);
		}
	}

	BAIL_IF_NULL(font, err, kFskErrNotFound);

	textContext->font = font;

	BAIL_IF_ERR(err = FskGrowablePathNew(1024, &textContext->path));

bail:
	if (err)
	{
		FskTextContextDispose(*pTextContext);
		*pTextContext = NULL;
	}
	return err;
}


/********************************************************************************
 * FskTextContextClone
 ********************************************************************************/

FskErr
FskTextContextClone(FskTextContext textContext, FskTextContext *newTextContext)
{
	FskErr err;

	*newTextContext = NULL;
	BAIL_IF_NULL(textContext, err, kFskErrNone);
	BAIL_IF_ERR(err = FskMemPtrNewClear(sizeof(struct FskTextContextRecord), (FskMemPtr*)newTextContext));
	**newTextContext = *textContext;
	if (textContext->font)
	{
		CFRetain((*newTextContext)->font);
	}
	if (textContext->path)
	{
		(*newTextContext)->path = NULL;
		BAIL_IF_ERR(err = FskGrowablePathClone(textContext->path, &(*newTextContext)->path));
	}

bail:
	if (err)
	{
		FskTextContextDispose(*newTextContext);
		*newTextContext = NULL;
	}
	return err;
}


/********************************************************************************
 * FskTextContextDispose
 ********************************************************************************/

void
FskTextContextDispose(FskTextContext textContext)
{
	if (textContext != NULL)
	{
		if (textContext->path != NULL) FskGrowablePathDispose(textContext->path);
		if (textContext->font != NULL) CFRelease(textContext->font);

		(void)FskMemPtrDispose(textContext);
	}
}


/********************************************************************************
 * FskTextContextAppendGlyphPath
 ********************************************************************************/

FskErr
FskTextContextAppendGlyphPath(FskGlyphID glyphID, FskFixed x0, FskFixed y0, FskTextContext textContext)
{
	CGPathRef glyphPath = NULL;
	FskErr err = kFskErrNone;

	glyphPath = CTFontCreatePathForGlyph(textContext->font, glyphID, NULL);
	if (glyphPath != NULL)	// printable?
	{
		textContext->currentPathAdvance.x = FskFixedToFloat(x0);
		textContext->currentPathAdvance.y = FskFixedToFloat(y0);
		textContext->err = kFskErrNone;
		CGPathApply(glyphPath, textContext, MyCGPathApplierFunction);
		err = textContext->err;
		if (!err)
			err = FskGrowablePathAppendSegmentEndGlyph(textContext->path);
		CFRelease(glyphPath);
	}

	return err;
}


/********************************************************************************
 * FskGrowablePathFromUnicodeStringNew
 ********************************************************************************/

FskErr
FskGrowablePathFromUnicodeStringNew(
									const UInt16			*uniChars,
									UInt32					numUniChars,
									const FskFontAttributes	*attributes,
									FskFixedPoint2D			*origin,
									FskGrowablePath			*path
									)
{
	FskTextContext textContext = NULL;
	CGGlyph *glyphs = NULL;
	CGSize *advances = NULL;
	CGPoint advance;
	double totalAdvance;
	FskFixedPoint2D	org;
	UInt32 i;
	FskErr err = kFskErrNone;

	if (origin != NULL)	{	org   = *origin;			}
	else				{	org.x = 0;		org.y = 0;	}

	*path = NULL;

	BAIL_IF_ERR(err = FskTextContextFromFontAttributesNew(attributes, &textContext));		/* Make context */

	textContext->origin.x = FskFixedToFloat(org.x);
	textContext->origin.y = FskFixedToFloat(org.y);

	err = FskMemPtrNewClear(numUniChars * sizeof(CGGlyph), &glyphs);
	BAIL_IF_ERR(err);

	err = FskMemPtrNewClear(numUniChars * sizeof(CGSize), &advances);
	BAIL_IF_ERR(err);

	BAIL_IF_FALSE(CTFontGetGlyphsForCharacters(textContext->font, uniChars, glyphs, numUniChars), err, kFskErrOperationFailed);
	totalAdvance = CTFontGetAdvancesForGlyphs(textContext->font, kCTFontHorizontalOrientation, glyphs, advances, numUniChars);

	advance = CGPointMake(0.0, 0.0);

	for (i = 0; i < numUniChars; i++)
	{
		err = FskTextContextAppendGlyphPath(glyphs[i], FskRoundFloatToFixed(advance.x), FskRoundFloatToFixed(advance.y), textContext);
		BAIL_IF_ERR(err);

		advance.x += advances[i].width;
	}

	*path = textContext->path;	/* Steal the growable path from the context */
	textContext->path = NULL;

	if (origin != NULL)
	{
		origin->x += FskRoundFloatToFixed(totalAdvance);
	}

bail:
	if (advances != NULL) FskMemPtrDispose(advances);
	if (glyphs != NULL) FskMemPtrDispose(glyphs);
	if (textContext != NULL) FskTextContextDispose(textContext);

	return err;
}


/********************************************************************************
 * FskUnicodeStringGetWidth
 * TODO: Implement a more lightweight version.
 ********************************************************************************/

double
FskUnicodeStringGetWidth(const UInt16 *uniChars, UInt32 numUniChars, const FskFontAttributes *attributes)
{
	FskTextContext textContext = NULL;
	CGGlyph *glyphs = NULL;
	double totalAdvance = 0.0;
	FskErr err = kFskErrNone;

	BAIL_IF_ERR(err = FskTextContextFromFontAttributesNew(attributes, &textContext));

	err = FskMemPtrNewClear(numUniChars * sizeof(CGGlyph), &glyphs);
	BAIL_IF_ERR(err);

	BAIL_IF_FALSE(CTFontGetGlyphsForCharacters(textContext->font, uniChars, glyphs, numUniChars), err, kFskErrOperationFailed);
	totalAdvance = CTFontGetAdvancesForGlyphs(textContext->font, kCTFontHorizontalOrientation, glyphs, NULL, numUniChars);

bail:
	if (glyphs != NULL) FskMemPtrDispose(glyphs);
	FskTextContextDispose(textContext);
	return totalAdvance;
}

#elif (TARGET_OS_MAC || TARGET_OS_MACOSX) && !TARGET_OS_IPHONE

	#include <ApplicationServices/ApplicationServices.h>


struct	FskTextContextRecord {
	FskGrowablePath				path;

	Float32Point				origin;
	float						windowHeight;
	Boolean						first;
	ATSUStyle					atsuStyle;

	ATSCurveType				curveType;

	ATSQuadraticNewPathUPP		newPathProc;
	ATSQuadraticLineUPP			lineProc;
	ATSQuadraticCurveUPP		curveProc;
	ATSQuadraticClosePathUPP	closeQuadraticPathProc;
	ATSCubicMoveToUPP			moveToProc;
	ATSCubicLineToUPP			lineToProc;
	ATSCubicCurveToUPP			curveToProc;
	ATSCubicClosePathUPP		closeCubicPathProc;
};


#define XCOORD(xx)		(textContext->origin.x + (xx))
#define YCOORD(yy)		(textContext->origin.y + (yy))


#define kFaceNameSize	32

/********************************************************************************
 * MyCompareFontNamesProc
 ********************************************************************************/

static int
MyCompareFontNamesProc(const void *s1, const void *s2)
{
	return FskStrCompareCaseInsensitiveWithLength((const char*)s1, (const char*)s2, (UInt32)kFaceNameSize);
}


/********************************************************************************
 * FskGetSystemFontFaceNameList
 ********************************************************************************/

FskErr
FskGetSystemFontFaceNameList(FskConstGrowableArray *faceNames, void *sysContext)
{
	static FskGrowableArray gFaceNames	= NULL;			/* Once allocated, this is never deallocated */
	FskErr					err			= kFskErrNone;
	CFStringRef				ffName		= NULL;
	ATSFontFamilyIterator	ffIt		= NULL;
	ATSFontFamilyRef		ffRef;
	char					strBuf[256];

	UNUSED(sysContext);

	if (gFaceNames == NULL) {
		BAIL_IF_ERR(err = FskGrowableArrayNew(kFaceNameSize, 256, &gFaceNames));
		BAIL_IF_ERR(err = ATSFontFamilyIteratorCreate(kATSFontContextLocal, NULL, gFaceNames, kATSOptionFlagsUnRestrictedScope, &ffIt));
		while ((err = ATSFontFamilyIteratorNext(ffIt, &ffRef)) == noErr) {
			BAIL_IF_ERR(err = ATSFontFamilyGetName(ffRef, kATSOptionFlagsDefault, &ffName));
			CFStringGetCString(ffName, strBuf, sizeof(strBuf), kCFStringEncodingUTF8);
			strBuf[kFaceNameSize-1] = 0;	/* Make sure there is a terminator */
			BAIL_IF_ERR(err = FskGrowableArrayAppendItem(gFaceNames, strBuf));
			CFRelease(ffName); ffName = NULL;
	  	}
		BAIL_IF_ERR(err = FskGrowableArraySortItems(gFaceNames, MyCompareFontNamesProc));
	}

bail:
	if (ffIt   != NULL)	ATSFontFamilyIteratorRelease(&ffIt);
	if (ffName != NULL)	CFRelease(ffName);
	*faceNames = gFaceNames;

	return err;
}


/********************************************************************************
 * MyQuadraticLineProc
 ********************************************************************************/

static pascal OSStatus
MyQuadraticLineProc(const Float32Point *pt1, const Float32Point *pt2, void *userData)
{
	OSStatus		err;
	FskTextContext	textContext	= (FskTextContext)userData;

	if (textContext->first) {
		BAIL_IF_ERR(err = FskGrowablePathAppendSegmentFloatMoveTo(	XCOORD(pt1->x),		YCOORD(pt1->y),
			textContext->path
		));
		textContext->first = false;
	}

	err = FskGrowablePathAppendSegmentFloatLineTo(					XCOORD(pt2->x),		YCOORD(pt2->y),
		textContext->path
	);

bail:
	return err;
}


/********************************************************************************
 * MyQuadraticCurveProc
 ********************************************************************************/

static pascal OSStatus
MyQuadraticCurveProc(const Float32Point *pt1, const Float32Point *controlPt, const Float32Point *pt2, void *userData)
{
	OSStatus		err;
	FskTextContext	textContext	= (FskTextContext)userData;

	if (textContext->first) {
		BAIL_IF_ERR(err = FskGrowablePathAppendSegmentFloatMoveTo(	XCOORD(pt1->x),			YCOORD(pt1->y),
			textContext->path
		));
		textContext->first = false;
	}

	err = FskGrowablePathAppendSegmentFloatQuadraticBezierTo(		XCOORD(controlPt->x),	YCOORD(controlPt->y),
															XCOORD(pt2->x),			YCOORD(pt2->y),
		textContext->path
	);

bail:
	return err;
}


/********************************************************************************
 * MyQuadraticNewPathProc
 ********************************************************************************/

static pascal OSStatus
MyQuadraticNewPathProc(void *userData)
{
	FskTextContext	textContext	= (FskTextContext)userData;

	if (FskGrowablePathGetSegmentCount(textContext->path) > 0)	/* If not the first contour... */
		FskGrowablePathAppendSegmentClose(textContext->path);				/* ... close the previous one */
	textContext->first = true;

	return noErr;
}


/********************************************************************************
 * MyClosePathProc
 ********************************************************************************/

static pascal OSStatus
MyClosePathProc(void *userData)
{
	OSStatus		err			= noErr;
	FskTextContext	textContext	= (FskTextContext)userData;

	err = FskGrowablePathAppendSegmentClose(textContext->path);
	textContext->first = true;

	return err;
}


/********************************************************************************
 * MyCubicMoveToProc
 ********************************************************************************/

static pascal OSStatus
MyCubicMoveToProc(const Float32Point *pt, void *userData)
{
	OSStatus		err;
	FskTextContext	textContext	= (FskTextContext)userData;

	err = FskGrowablePathAppendSegmentFloatMoveTo(			XCOORD(pt->x),		YCOORD(pt->y),
		textContext->path
	);

	return err;
}


/********************************************************************************
 * MyCubicLineToProc
 ********************************************************************************/

static pascal OSStatus
MyCubicLineToProc(const Float32Point *pt, void *userData)
{
	OSStatus		err;
	FskTextContext	textContext	= (FskTextContext)userData;

	err = FskGrowablePathAppendSegmentFloatLineTo(			XCOORD(pt->x),		YCOORD(pt->y),
		textContext->path
	);

	return err;
}


/********************************************************************************
 * MyCubicCurveToProc
 ********************************************************************************/

static pascal OSStatus
MyCubicCurveToProc(const Float32Point *pt1, const Float32Point *pt2, const Float32Point *pt3, void *userData)
{
	OSStatus		err;
	FskTextContext	textContext	= (FskTextContext)userData;

	err = FskGrowablePathAppendSegmentFloatCubicBezierTo(	XCOORD(pt1->x),		YCOORD(pt1->y),
													XCOORD(pt2->x),		YCOORD(pt2->y),
													XCOORD(pt3->x),		YCOORD(pt3->y),
		textContext->path
	);

	return err;
}


/********************************************************************************
 * GetFMFontFamilyFromFontName
 ********************************************************************************/

static FMFontFamily
GetFMFontFamilyFromFontName(const char *fontFamilyName)
{
	CFStringRef				cfFontNameString;
	ATSFontFamilyRef		atsFontFamilyRef;
	FMFontFamily			fmFontFamily		= kInvalidFontFamily;

	if ((cfFontNameString = CFStringCreateWithCString(kCFAllocatorDefault, fontFamilyName, kCFStringEncodingUTF8)) != NULL) {
		if ((atsFontFamilyRef = ATSFontFamilyFindFromName(cfFontNameString, kATSOptionFlagsDefault)) != (ATSFontFamilyRef)(kInvalidFontFamily))
			fmFontFamily = FMGetFontFamilyFromATSFontFamilyRef(atsFontFamilyRef);
		CFRelease(cfFontNameString);
	}

	return fmFontFamily;
}


/********************************************************************************
 * NewStyleFromFontAttributes
 ********************************************************************************/

static OSStatus
NewStyleFromFontAttributes(const FskFontAttributes *attr, ATSUStyle *atsuStyle)
{
	OSStatus				err					= noErr;
	ATSUAttributeTag		theTags[]			= {
													kATSUFontTag,
													kATSUSizeTag,
													kATSUQDBoldfaceTag,
													kATSUQDItalicTag,
													kATSUQDUnderlineTag,
													kATSUQDCondensedTag,
													kATSUQDExtendedTag,
													kATSUStyleStrikeThroughTag
												};
	ByteCount       		theSizes[]			= {
													sizeof(ATSUFontID),	/* font */
													sizeof(Fixed),		/* size */
													sizeof(Boolean),	/* bold */
													sizeof(Boolean),	/* italic */
													sizeof(Boolean),	/* underline */
													sizeof(Boolean),	/* condensed */
													sizeof(Boolean),	/* extended */
													sizeof(Boolean)		/* strikethrough */
												};
	ATSUAttributeValuePtr	theValues[sizeof(theTags) / sizeof(ATSUAttributeTag)];
	FMFontFamily			fmFontFamily			= kInvalidFontFamily;
	FMFontStyle				fmStyle, fmIntrinsicStyle;
	ATSUFontID      		atsuFontID;
	Fixed					atsuFontSize;
	Boolean					isBold, isItalic, isUnderline, isCondensed, isExtended, isStrikeThrough;

	/* Create a style */
	BAIL_IF_NULL(atsuStyle, err, paramErr);
	BAIL_IF_ERR(err = ATSUCreateStyle(atsuStyle));
	//BAIL_IF_ERR(err = ATSUClearStyle(atsuStyle);	// Probably not necessary

	/* Determine the font family, or one similar to it */
	if (attr->family != NULL) {						/* Parse the font family */
		const char	*fs;
		char		fontName[256];
		for (fs = attr->family; GetNextTokenInCommaSeparatedList(&fs, fontName, sizeof(fontName)); ) {
			if ((fmFontFamily = GetFMFontFamilyFromFontName(fontName)) != kInvalidFontFamily) {
				LOGD("Succeeded getting text context from \"%s\"", fontName);
				break;
			}
			LOGD("Failed    getting text context from \"%s\"", fontName);
		}
	}
	if (fmFontFamily ==  kInvalidFontFamily) {		/* Couldn't find the fonts - choose one from our default list */
		const char **fl;
		for (fl = gDefaultFontNames; *fl != NULL; fl++) {
			if ((fmFontFamily = GetFMFontFamilyFromFontName(*fl)) != kInvalidFontFamily) {
				LOGD("Succeeded getting text context from default \"%s\"", *fl);
				break;
			}
			LOGD("Failed    getting text context from default \"%s\"", *fl);
		}
	}

	/* Set the style to select the closest member of the font family */
	fmStyle = 0;
	if (attr->weight >= ((kFskFontWeightBold + kFskFontWeightNormal)/2))				fmStyle |= bold;
	if ((attr->style == kFskFontStyleItalic) || (attr->style == kFskFontStyleOblique))	fmStyle |= italic;
	if (attr->stretch < kFskFontStretchSemiCondensed)									fmStyle |= condense;
	if (attr->stretch > kFskFontStretchSemiExpanded)									fmStyle |= extend;
	if (attr->decoration & kFskFontDecorationUnderline)									fmStyle |= underline;
	BAIL_IF_ERR(err = FMGetFontFromFontFamilyInstance(fmFontFamily, fmStyle, &atsuFontID, &fmIntrinsicStyle));

	/* Clear the style attributes that are intrinsic to the font */
	fmStyle &= ~fmIntrinsicStyle;

	/* Apply non-intrinsic styles to the font */
	isBold			= (fmStyle & bold)		!= 0;
	isItalic		= (fmStyle & italic)	!= 0;
	isUnderline		= (fmStyle & underline)	!= 0;
	isCondensed		= (fmStyle & condense)	!= 0;
	isExtended		= (fmStyle & extend)	!= 0;
	isStrikeThrough	= (attr->decoration & kFskFontDecorationLineThrough) != 0;
	atsuFontSize	= attr->size * 65536.0f + 0.5f;

	/* Set up attribute array */
	theValues[0] = &atsuFontID;
	theValues[1] = &atsuFontSize;
	theValues[2] = &isBold;
	theValues[3] = &isItalic;
	theValues[4] = &isUnderline;
	theValues[5] = &isCondensed;
	theValues[6] = &isExtended;
	theValues[7] = &isStrikeThrough;

	/* Set the attributes of the style */
	err = ATSUSetAttributes(*atsuStyle, sizeof(theTags) / sizeof(ATSUAttributeTag), theTags, theSizes, theValues);

bail:
	if (err != noErr) {
		if ((atsuStyle != NULL) && (*atsuStyle != NULL)) {
			(void)ATSUDisposeStyle(*atsuStyle);
			*atsuStyle = NULL;
		}
	}

	return err;
}


/********************************************************************************
 * FskTextContextFromFontAttributesNew
 ********************************************************************************/

FskErr
FskTextContextFromFontAttributesNew(const FskFontAttributes *attributes, FskTextContext *pTextContext)
{
	FskErr			err;
	FskTextContext	textContext;

	/* Allocate context and clear */
	BAIL_IF_ERR(err = FskMemPtrNewClear(sizeof(struct FskTextContextRecord), (FskMemPtr*)pTextContext));
	textContext = *pTextContext;

	/* Allocate style and initialize */
	BAIL_IF_ERR(NewStyleFromFontAttributes(attributes, &textContext->atsuStyle));

	/* Initialize UPP's */
#ifdef USE_NATIVE_CUBICS	/* MacOS always goes through quadratics, so it is a waste to use cubics */
	if ((err = ATSUGetNativeCurveType(textContext->atsuStyle, &textContext->curveType)) != noErr)
#endif /* USE_NATIVE_CUBICS */
	textContext->curveType = kATSQuadCurveType;
	if (textContext->curveType != kATSCubicCurveType) {						/* Quadratic path */
		textContext->newPathProc			= NewATSQuadraticNewPathUPP(	MyQuadraticNewPathProc);
		textContext->lineProc				= NewATSQuadraticLineUPP(		MyQuadraticLineProc);
		textContext->curveProc				= NewATSQuadraticCurveUPP(		MyQuadraticCurveProc);
		textContext->closeQuadraticPathProc	= NewATSQuadraticClosePathUPP(	MyClosePathProc);
	}
	else {																	/* Cubic path */
		textContext->moveToProc				= NewATSCubicMoveToUPP(			MyCubicMoveToProc);
		textContext->lineToProc				= NewATSCubicLineToUPP(			MyCubicLineToProc);
		textContext->curveToProc			= NewATSCubicCurveToUPP(		MyCubicCurveToProc);
		textContext->closeCubicPathProc		= NewATSCubicClosePathUPP(		MyClosePathProc);
	}

	BAIL_IF_ERR(err = FskGrowablePathNew(1024, &textContext->path));

	textContext->windowHeight	= 0;
	textContext->origin.x		= 0;
	textContext->origin.y		= 0;
	textContext->windowHeight	= 0;
	textContext->first			= true;

bail:
	return err;
}


/********************************************************************************
 * FskTextContextClone
 ********************************************************************************/

FskErr
FskTextContextClone(FskTextContext textContext, FskTextContext *newTextContext)
{
	FskErr err;
	*newTextContext = NULL;
	BAIL_IF_NULL(textContext, err, kFskErrNone);
	BAIL_IF_ERR(err = FskMemPtrNewClear(sizeof(struct FskTextContextRecord), (FskMemPtr*)newTextContext));
	**newTextContext = *textContext;
	if (textContext->path) {
		(**newTextContext).path = NULL;
		BAIL_IF_ERR(err = FskGrowablePathClone(textContext->path, &(**newTextContext).path));
	}
	// TODO: clone all UPPs and ATSUStyle
	BAIL_IF_ERR(err = ATSUCreateAndCopyStyle(textContext->atsuStyle, &(**newTextContext).atsuStyle));


bail:
	return err;
}


/********************************************************************************
 * FskTextContextDispose
 ********************************************************************************/

void
FskTextContextDispose(FskTextContext textContext)
{
	if (textContext != NULL) {
		if (textContext->newPathProc			!= NULL)	DisposeATSQuadraticNewPathUPP(	textContext->newPathProc);
		if (textContext->lineProc				!= NULL)	DisposeATSQuadraticLineUPP(		textContext->lineProc);
		if (textContext->curveProc				!= NULL)	DisposeATSQuadraticCurveUPP(	textContext->curveProc);
		if (textContext->closeQuadraticPathProc	!= NULL)	DisposeATSQuadraticClosePathUPP(textContext->closeQuadraticPathProc);
		if (textContext->moveToProc				!= NULL)	DisposeATSCubicMoveToUPP(		textContext->moveToProc);
		if (textContext->lineToProc				!= NULL)	DisposeATSCubicLineToUPP(		textContext->lineToProc);
		if (textContext->curveToProc			!= NULL)	DisposeATSCubicCurveToUPP(		textContext->curveToProc);
		if (textContext->closeCubicPathProc		!= NULL)	DisposeATSCubicClosePathUPP(	textContext->closeCubicPathProc);

		if (textContext->atsuStyle	!= NULL)	(void)ATSUDisposeStyle(textContext->atsuStyle);
		if (textContext->path		!= NULL)	FskGrowablePathDispose(textContext->path);

		(void)FskMemPtrDispose(textContext);
	}
}


/********************************************************************************
 * FskTextContextAppendGlyphPath
 ********************************************************************************/

FskErr
FskTextContextAppendGlyphPath(FskGlyphID glyphID, FskFixed x0, FskFixed y0, FskTextContext textContext)
{
	OSStatus err, callBackResult;

	textContext->first		= true;
	textContext->origin.x	= x0 * (1.0f / 65536.0f);
	textContext->origin.y	= y0 * (1.0f / 65536.0f);
	if (textContext->curveType == kATSQuadCurveType)
		err = ATSUGlyphGetQuadraticPaths(
			textContext->atsuStyle,   glyphID,
			textContext->newPathProc, textContext->lineProc,   textContext->curveProc,   textContext->closeQuadraticPathProc,
			textContext, &callBackResult
		);
	else
		err = ATSUGlyphGetCubicPaths(
			textContext->atsuStyle,   glyphID,
			textContext->moveToProc,  textContext->lineToProc, textContext->curveToProc, textContext->closeCubicPathProc,
			textContext, &callBackResult
		);

	(void)FskGrowablePathAppendSegmentEndGlyph(textContext->path);

	if (err != noErr)
		err = callBackResult;
	return err;
}


/********************************************************************************
 * atsuGetOneLayoutControl
 * atsuGetLayoutLineDirection
 ********************************************************************************/

static OSStatus
atsuGetOneLayoutControl(ATSUTextLayout iLayout, ATSUAttributeTag iTag, ByteCount iExpectedValueSize, ATSUAttributeValuePtr oValue)
{
	OSStatus status = ATSUGetLayoutControl(iLayout, iTag, iExpectedValueSize, oValue, NULL);
	return (status == kATSUNotSetErr) ? noErr : status;
}

#define atsuGetLayoutLineDirection( iLayout, oDirectionPtr )	atsuGetOneLayoutControl(iLayout, kATSULineDirectionTag, sizeof(Boolean), oDirectionPtr)


/********************************************************************************
 * AppendLayoutPathToTextContext
 ********************************************************************************/

static FskErr
AppendLayoutPathToTextContext(
	ATSUTextLayout		iLayout,
	FskFixedPoint2D		*origin,
	FskTextContext		textContext
)
{
	OSStatus			err				= kFskErrNone;
	ATSLayoutRecord		*layoutRecords	= NULL;
	Fixed				*deltaYs		= NULL;
	ItemCount			numRecords, numDeltaYs, i;
	FskFixed			xorg, yorg;
	Boolean				lineDirection;
	ATSUCaret			theCaret;

	/* Get layout */
	ATSUDirectGetLayoutDataArrayPtrFromTextLayout(iLayout, 0, kATSUDirectDataLayoutRecordATSLayoutRecordCurrent, (void**)(void*)&layoutRecords, &numRecords);
	ATSUDirectGetLayoutDataArrayPtrFromTextLayout(iLayout, 0, kATSUDirectDataBaselineDeltaFixedArray, (void**)(void*)&deltaYs, &numDeltaYs);

	/* Append paths for each glyph in the layout */
	for (i = 0; i < numRecords; i++) {
		xorg = origin->x + layoutRecords[i].realPos;
		if (deltaYs == NULL)	yorg = origin->y;
		else					yorg = origin->y - deltaYs[i];
		textContext->first = true;
		if (layoutRecords[i].glyphID != kATSDeletedGlyphcode)
			if ((err = FskTextContextAppendGlyphPath(layoutRecords[i].glyphID, xorg, yorg, textContext)) != noErr)
				break;
	}

	/* Update origin */
    if (	(atsuGetLayoutLineDirection(iLayout, &lineDirection)	== noErr)
    	&&	(ATSUOffsetToPosition(iLayout, lineDirection ? 0 : numRecords-1, false, &theCaret, NULL, NULL) == noErr)
    )
		origin->x += theCaret.fX;

	//if (err == noErr) {
	//	err = FskGrowablePathAppendSegmentClose(&textContext.path);
	//}

	if (deltaYs != NULL)			ATSUDirectReleaseLayoutDataArrayPtr(NULL, kATSUDirectDataBaselineDeltaFixedArray, (void**)(void*)&deltaYs);
	ATSUDirectReleaseLayoutDataArrayPtr(NULL, kATSUDirectDataLayoutRecordATSLayoutRecordCurrent, (void**)(void*)&layoutRecords);

	return err;
}


/********************************************************************************
 * FskGrowablePathFromUnicodeStringNew
 ********************************************************************************/

FskErr
FskGrowablePathFromUnicodeStringNew(
	const UInt16			*uniChars,
	UInt32					numUniChars,
	const FskFontAttributes	*attributes,
	FskFixedPoint2D			*origin,
	FskGrowablePath			*path
)
{
	FskTextContext	textContext		= NULL;
	ATSUTextLayout	theLayout		= NULL;
	OSStatus		err				= noErr;
	ItemCount		numRecords;
	FskFixedPoint2D	org;

	#if defined(LOG_PARAMETERS)
		LogGrowablePathFromUnicodeStringNew(uniChars, numUniChars, attributes, origin, path);
	#endif /* LOG_PARAMETERS */

	if (origin != NULL)	{	org   = *origin;			}
	else				{	org.x = 0;		org.y = 0;	}

	*path = NULL;
	numRecords = numUniChars;
	BAIL_IF_ERR(err = FskTextContextFromFontAttributesNew(attributes, &textContext));		/* Make context */
	BAIL_IF_ERR(err = ATSUCreateTextLayoutWithTextPtr(uniChars, 0, numRecords, numRecords, 1, &numRecords, &textContext->atsuStyle, &theLayout));	/* make layout */
	BAIL_IF_ERR(err = ATSUSetTransientFontMatching(theLayout, true));						/* Do font substitution if necessary */
	BAIL_IF_ERR(err = AppendLayoutPathToTextContext(theLayout, &org, textContext));			/* Make path */
	*path = textContext->path;	textContext->path = NULL;									/* Steal the growable path from the context */
	if (origin != NULL)	*origin = org;														/* Update the origin */

bail:
	if (theLayout != NULL)		(void)ATSUDisposeTextLayout(theLayout);
	if (textContext != NULL)	FskTextContextDispose(textContext);

	return err;
}


/********************************************************************************
 * FskUnicodeStringGetWidth
 * TODO: Implement a more lightweight version.
 ********************************************************************************/

double
FskUnicodeStringGetWidth(const UInt16 *uniChars, UInt32 numUniChars, const FskFontAttributes *attributes)
{
	FskErr			err		= kFskErrNone;
	FskGrowablePath	path	= NULL;
	FskFixedPoint2D	origin;

	origin.x = 0;
	origin.y = 0;
	BAIL_IF_NEGATIVE(err = FskGrowablePathFromUnicodeStringNew(uniChars, numUniChars, attributes, &origin, &path), origin.x, err);

bail:
	FskGrowablePathDispose(path);
	return FskFixedToFloat(origin.x);
}


#if 0
#pragma mark -
#pragma mark Windows
#endif
/********************************************************************************
 ********************************************************************************
 ********************************************************************************
 *****	WINDOWS -- WINDOWS -- WINDOWS -- WINDOWS -- WINDOWS -- WINDOWS		*****
 ********************************************************************************
 ********************************************************************************
 ********************************************************************************/
#elif TARGET_OS_WIN32


#define XCOORD(xx)		(textContext->origin.x + (xx))
#define YCOORD(yy)		(textContext->origin.y - (yy))

struct FskTextContextRecord {
	FskGrowablePath	path;

	HWND			hWnd;	// == NULL I'm not sure yet whether we need to do this
	HDC				hdc;
	HFONT			hFont;
	HFONT			oldFont;
	FskFixedPoint2D	origin;
};

#define kFaceNameSize	(LF_FACESIZE * sizeof(TCHAR))


/********************************************************************************
 * MyEnumFontsProc
 ********************************************************************************/

static int CALLBACK
MyEnumFontsProc(CONST LOGFONT *lplf, CONST TEXTMETRIC *lptm, DWORD dwType, LPARAM lpData)
{
	UNUSED(lptm);
	UNUSED(dwType);
	FskGrowableArrayAppendItem((FskGrowableArray)lpData, lplf->lfFaceName);
	return 1;
}

/********************************************************************************
 * MyCompareFontNamesProc
 ********************************************************************************/

static int
MyCompareFontNamesProc(const void *s1, const void *s2)
{
	return FskStrCompareCaseInsensitiveWithLength((const char*)s1, (const char*)s2, kFaceNameSize);
}


/********************************************************************************
 * FskGetSystemFontFaceNameList
 ********************************************************************************/

FskErr
FskGetSystemFontFaceNameList(FskConstGrowableArray *faceNames, void *sysContext)
{
	static FskGrowableArray gFaceNames	= NULL;			/* Once allocated, this is never deallocated */
	FskErr					err			= kFskErrNone;
	DWORD					flags;
	LOGFONTA				logFont;
	logFont.lfCharSet        = DEFAULT_CHARSET;
	logFont.lfClipPrecision  = CLIP_DEFAULT_PRECIS;
	logFont.lfEscapement     = 0;
	logFont.lfFaceName[0]    = 0;
	logFont.lfHeight         = 0;
	logFont.lfItalic         = FALSE;
	logFont.lfOrientation    = 0;
	logFont.lfOutPrecision   = OUT_DEFAULT_PRECIS;
	logFont.lfPitchAndFamily = 0;
	logFont.lfQuality        = DEFAULT_QUALITY;
	logFont.lfStrikeOut      = FALSE;
	logFont.lfUnderline      = FALSE;
	logFont.lfWeight         = 0;
	logFont.lfWidth          = 0;
	flags                    = 0;
	if (gFaceNames == NULL) {
		BAIL_IF_ERR(err = FskGrowableArrayNew(kFaceNameSize, 256, &gFaceNames));
		BAIL_IF_FALSE(EnumFontFamiliesExA((HDC)sysContext, &logFont, (FONTENUMPROCA)MyEnumFontsProc, (LPARAM)gFaceNames, flags), err, GetLastError());
		BAIL_IF_ERR(err = FskGrowableArraySortItems(gFaceNames, MyCompareFontNamesProc));
	}

bail:
	*faceNames = gFaceNames;
	return err;
}


/********************************************************************************
 * CreateFontFromFontName
 ********************************************************************************/

static int
CreateFontFromFontName(const FskFontAttributes *at, const char *fontName, FskTextContext textContext)
{
	textContext->hFont = CreateFontA(
		(int)(at->size + 0.5f),										// height of font
		0,															// average character width
		0,															// angle of escapement
		0,															// base-line orientation angle
		at->weight,													// font weight
		(at->style != 0),											// italic attribute option
		((at->decoration & kFskFontDecorationUnderline)   != 0),	// underline attribute option
		((at->decoration & kFskFontDecorationLineThrough) != 0),	// strikeout attribute option
		DEFAULT_CHARSET,											// character set identifier (ass opposed to ANSI_CHARSET, etc. )
		OUT_TT_ONLY_PRECIS,											// output precision, TrueType only (all outlines OUT_OUTLINE_PRECIS might not work)
		CLIP_DEFAULT_PRECIS,										// clipping precision
		ANTIALIASED_QUALITY,										// output quality
		DEFAULT_PITCH|FF_DONTCARE,									// pitch and family
		fontName													// typeface name
	);

	return (int)(textContext->hFont != NULL);
}


/********************************************************************************
 * FskTextContextFromFontAttributesNew
 ********************************************************************************/

FskErr
FskTextContextFromFontAttributesNew(const FskFontAttributes *at, FskTextContext *pTextContext)
{
	FskErr					err;
	FskTextContext			textContext;
	FskConstGrowableArray	faceNames;

	/* Allocate context and clear */
	BAIL_IF_ERR(err = FskMemPtrNewClear(sizeof(struct FskTextContextRecord), (FskMemPtr*)pTextContext));
	textContext = *pTextContext;

	/* Allocate a path */
	BAIL_IF_ERR(err = FskGrowablePathNew(1024, &textContext->path));

	/* Get a device context */
	textContext->hWnd = GetForegroundWindow();	//NULL;
	textContext->hdc = GetDC(textContext->hWnd);

	/* Make sure we have the list of font names, because Windows gives us a font even with a bogus name */
	err = FskGetSystemFontFaceNameList(&faceNames, textContext->hdc);

	/* Parse the font family, and see if we have one of the fonts */	textContext->hFont = NULL;
	if (at->family != NULL) {
		const char	*fs;
		char		fontName[256];
		char		*fn;
		for (fs = at->family; GetNextTokenInCommaSeparatedList(&fs, fontName, sizeof(fontName)); ) {
			if (	((fn = FskGrowableArrayBSearchItems(faceNames, fontName, MyCompareFontNamesProc)) != NULL)
				&&	CreateFontFromFontName(at, fn, textContext)
			) {
				LOGD("Succeeded getting text context from \"%s\"", fontName);
				break;
			}
			LOGD("Failed    getting text context from \"%s\"", fontName);
		}
	}

	/* Couldn't find any fonts: look through our default list */
	if (textContext->hFont == NULL) {
		const char **fl;
		for (fl = gDefaultFontNames; *fl != NULL; fl++) {
			if (CreateFontFromFontName(at, *fl, textContext)) {
				LOGD("Succeeded getting text context from \"%s\"", *fl);
				break;
			}
			LOGD("Failed    getting text context from default \"%s\"", *fl);
		}
	}
	BAIL_IF_NULL(textContext->hFont, err, kFskErrNotFound);

	textContext->oldFont = SelectObject(textContext->hdc, textContext->hFont);

bail:
	if (err) {
		FskTextContextDispose(textContext);
		*pTextContext = NULL;
	}
	return err;
}


/********************************************************************************
 * FskTextContextClone
 ********************************************************************************/

FskErr
FskTextContextClone(FskTextContext textContext, FskTextContext *newTextContext)
{
	FskErr err;
	*newTextContext = NULL;
	BAIL_IF_NULL(textContext, err, kFskErrNone);
	BAIL_IF_ERR(err = FskMemPtrNewClear(sizeof(struct FskTextContextRecord), (FskMemPtr*)newTextContext));
	**newTextContext = *textContext;
	if (textContext->path) {
		(**newTextContext).path = NULL;
		BAIL_IF_ERR(err = FskGrowablePathClone(textContext->path, &(**newTextContext).path));
	}
	(**newTextContext).oldFont = textContext->hFont;
	//TODO: implement and test
	//(**newTextContext).hFont   = CloneFont(textContext->hFont);
	//(**newTextContext).hdc     = CloneHDC(textContext->hdc);
	(**newTextContext).hFont = NULL;
	*(**newTextContext).hFont = *textContext->hFont;	// TODO: Deliberately cause a crash

bail:
	return err;
}


/********************************************************************************
 * FskTextContextDispose
 ********************************************************************************/

void
FskTextContextDispose(FskTextContext textContext)
{
	if (textContext != NULL) {
		if (textContext->path    != NULL)	FskGrowablePathDispose(textContext->path);
		if (textContext->oldFont != NULL)	(void)SelectObject(textContext->hdc, textContext->oldFont);
		if (textContext->hFont   != NULL)	(void)DeleteObject(textContext->hFont);
		if (textContext->hdc     != NULL)	(void)ReleaseDC(textContext->hWnd, textContext->hdc);
		(void)FskMemPtrDispose(textContext);
	}
}



/********************************************************************************
 * AppendPolyLine
 ********************************************************************************/

static FskErr
AppendPolyLine(UInt32 numPts, const FskFixedPoint2D *pt, FskTextContext textContext)
{
	FskErr err;

	for ( ; numPts--; pt++)
		if ((err = FskGrowablePathAppendSegmentLineTo(XCOORD(pt->x), YCOORD(pt->y), textContext->path)) != kFskErrNone)
			break;

	return err;
}


/********************************************************************************
 * AppendQuadraticBSpline
 ********************************************************************************/

static FskErr
AppendQuadraticBSpline(UInt32 numPts, const FskFixedPoint2D *pt, FskTextContext textContext)
{
	FskErr		err;

	BAIL_IF_FALSE((numPts > 0), err, kFskErrBadData);
	if (numPts == 1) {							/* Not enough for a quadratic segment -- append a line instead */
		err = AppendPolyLine(numPts, pt, textContext);
	}
	else {
		FskFixed	xm, ym;
		for (numPts -=2; numPts--; pt++) {		/* Convert from quadratic B-spline to quadratic Bezier */
			xm = (pt[1].x + pt[0].x) >> 1;
			ym = (pt[1].y + pt[0].y) >> 1;
			BAIL_IF_ERR(err = FskGrowablePathAppendSegmentQuadraticBezierTo(
				XCOORD(pt[0].x),	YCOORD(pt[0].y),
				XCOORD(xm),			YCOORD(ym),
				textContext->path
			));
		}
		err = FskGrowablePathAppendSegmentQuadraticBezierTo(
			XCOORD(pt[0].x),	YCOORD(pt[0].y),
			XCOORD(pt[1].x),	YCOORD(pt[1].y),
			textContext->path
		);
	}

bail:
	return err;
}


/********************************************************************************
 * AppendCubicBSpline
 ********************************************************************************/

static FskErr
AppendCubicBSpline(UInt32 numPts, const FskFixedPoint2D *pt, FskTextContext textContext)
{
	FskErr	err;

	if (numPts < 3) {		/* Not enough points for a cubic -- do a quadratic instead */
		err = AppendQuadraticBSpline(numPts, pt, textContext);
	}
	else {					/* We have at least 3 points for a cubic */
		FskFixedPoint2D q1, q2, q3, q4;
		q1 = *pt++;
		if (numPts >= 4) {	/* Convert from cubic B-spline to cubic Bezier */
			for (numPts -= 4; numPts--; pt++, q1 = q4) {
				q4.x = ((pt[0].x << 1) + pt[1].x + 1) / 3;	q4.y = ((pt[0].y << 1) + pt[1].y + 1) / 3;
				q2.x =     (q1.x       + pt[0].x) >> 1;		q2.y =     (q1.y       + pt[0].y) >> 1;
				q3.x =     (q2.x       +    q4.x) >> 1;		q3.y =     (q2.y       +    q4.y) >> 1;
				BAIL_IF_ERR(err = FskGrowablePathAppendSegmentCubicBezierTo(
					XCOORD(q1.x),	YCOORD(q1.y),
					XCOORD(q2.x),	YCOORD(q2.y),
					XCOORD(q3.x),	YCOORD(q3.y),
					textContext->path
				));
			}

			q4.x = (pt[0].x + pt[1].x) >> 1;		q4.y  = (pt[0].y + pt[1].y) >> 1;
			q2.x =    (q1.x + pt[0].x) >> 1;		q2.y =     (q1.y + pt[0].y) >> 1;
			q3.x =    (q2.x +    q4.x) >> 1;		q3.y =     (q2.y +    q4.y) >> 1;
			BAIL_IF_ERR(err = FskGrowablePathAppendSegmentCubicBezierTo(
				XCOORD(q1.x),	YCOORD(q1.y),
				XCOORD(q2.x),	YCOORD(q2.y),
				XCOORD(q3.x),	YCOORD(q3.y),
				textContext->path
			));
			pt++;
			q1 = q4;
		}

		err = FskGrowablePathAppendSegmentCubicBezierTo(
			XCOORD(q1.x),		YCOORD(q1.y),
			XCOORD(pt[0].x),	YCOORD(pt[0].y),
			XCOORD(pt[1].x),	YCOORD(pt[1].y),
			textContext->path
		);
	}

bail:
	return err;
}


/********************************************************************************
 * AppendOutlineToTextContext
 ********************************************************************************/

static FskErr
AppendOutlineToTextContext(LPTTPOLYGONHEADER lpHeader, DWORD size, FskTextContext textContext)
{
	LPTTPOLYGONHEADER	lpStart;	/* the start of the buffer */
	LPTTPOLYCURVE		lpCurve;	/* the current curve of a contour */
	FskErr				err;

	for (lpStart = lpHeader; (DWORD) lpHeader < (DWORD) (((LPSTR) lpStart) + size); lpHeader = (LPTTPOLYGONHEADER)(((LPSTR)lpHeader) + lpHeader->cb)) {
		BAIL_IF_FALSE(lpHeader->dwType == TT_POLYGON_TYPE, err, kFskErrBadData);

		/* Append each contour */
		BAIL_IF_ERR(err = FskGrowablePathAppendSegmentMoveTo(
			XCOORD(((FskFixedPoint2D*)(&lpHeader->pfxStart))->x),
			YCOORD(((FskFixedPoint2D*)(&lpHeader->pfxStart))->y),
			textContext->path
		));
		for (lpCurve = (LPTTPOLYCURVE)(lpHeader + 1); (DWORD) lpCurve < (DWORD)(((LPSTR)lpHeader) + lpHeader->cb); lpCurve = (LPTTPOLYCURVE)(lpCurve->apfx + lpCurve->cpfx)) {
			switch (lpCurve->wType) {
				case TT_PRIM_LINE:		err = AppendPolyLine(        lpCurve->cpfx, (FskFixedPoint2D*)(lpCurve->apfx), textContext);	break;
				case TT_PRIM_QSPLINE:	err = AppendQuadraticBSpline(lpCurve->cpfx, (FskFixedPoint2D*)(lpCurve->apfx), textContext);	break;
				case TT_PRIM_CSPLINE:	err = AppendCubicBSpline(    lpCurve->cpfx, (FskFixedPoint2D*)(lpCurve->apfx), textContext);	break;
				default:				err = kFskErrBadData;
			}
			BAIL_IF_ERR(err);
		}
		err = FskGrowablePathAppendSegmentClose(textContext->path);
	}

bail:
	return err;
}


/********************************************************************************
 * FskTextContextAppendGlyphPath
 ********************************************************************************/

FskErr
FskTextContextAppendGlyphPath(FskGlyphID glyphID, FskFixed x0, FskFixed y0, FskTextContext textContext)
{
	LPTTPOLYGONHEADER	outline		= NULL;
	SInt32				err, bytesNeeded;
	GLYPHMETRICS		glyphMetrics;

	MAT2				M;

	*((SInt32*)(&M.eM11)) = *((SInt32*)(&M.eM22)) = 1 << 16;
	*((SInt32*)(&M.eM12)) = *((SInt32*)(&M.eM21)) = 0;

	textContext->origin.x = x0;
	textContext->origin.y = y0;
#ifdef USE_UNHINTED_OUTLINES
	bytesNeeded = GetGlyphOutline(textContext->hdc, glyphID, GGO_NATIVE|GGO_GLYPH_INDEX|GGO_UNHINTED, &glyphMetrics, 0, NULL, &M);
#else /* USE_HINTED_OUTLINES */
	bytesNeeded = GetGlyphOutline(textContext->hdc, glyphID, GGO_NATIVE|GGO_GLYPH_INDEX,              &glyphMetrics, 0, NULL, &M);
#endif /* USE_HINTED_OUTLINES */
	BAIL_IF_NEGATIVE(bytesNeeded, err, GetLastError());	/* Error */
	BAIL_IF_ZERO(bytesNeeded, err, kFskErrNone);		/* Non-printing glyph */
	BAIL_IF_ERR(err = FskMemPtrNew(bytesNeeded, (FskMemPtr*)(&outline)));
#ifdef USE_UNHINTED_OUTLINES
	err = GetGlyphOutline(textContext->hdc, glyphID, GGO_NATIVE | GGO_GLYPH_INDEX | GGO_UNHINTED, &glyphMetrics, bytesNeeded, outline, &M);
#else /* USE_HINTED_OUTLINES */
	err = GetGlyphOutline(textContext->hdc, glyphID, GGO_NATIVE | GGO_GLYPH_INDEX,                &glyphMetrics, bytesNeeded, outline, &M);
#endif /* USE_HINTED_OUTLINES */
	if (err < 0) goto bail;
	err = AppendOutlineToTextContext(outline, bytesNeeded, textContext);
	(void)FskGrowablePathAppendSegmentEndGlyph(textContext->path);

bail:
	if (outline != NULL)	(void)FskMemPtrDispose(outline);
	return err;
}


/********************************************************************************
 * FskGetGlyphsFromUnicodeString
 ********************************************************************************/

FskErr
FskGetGlyphsFromUnicodeString(const UInt16 *uniChars, UInt32 numUniChars, FskTextContext textContext, FskGlyphID *glyphs, UInt32 *numGlyphs)
{
	HDC		hdc		= (HDC)textContext;
	FskErr	err;

	*numGlyphs = 0;
	if ((err = GetGlyphIndicesW(hdc, uniChars, numUniChars, glyphs, GGI_MARK_NONEXISTING_GLYPHS)) != GDI_ERROR) {
		*numGlyphs = err;
		err = kFskErrNone;
	}

	return err;
}


/********************************************************************************
 * FskGrowablePathFromUnicodeStringNew
 ********************************************************************************/

FskErr
FskGrowablePathFromUnicodeStringNew(
	const UInt16			*uniChars,
	UInt32					numUniChars,
	const FskFontAttributes	*attributes,
	FskFixedPoint2D			*origin,
	FskGrowablePath			*path
)
{
	FskErr			err;
	FskTextContext	textContext		= NULL;
	DWORD			placeResult;
	GCP_RESULTSW	placement;
	unsigned int	i;
	LPWSTR			gl;
	int				*dx;
	FskFixedPoint2D	org;

	#if defined(LOG_PARAMETERS)
		LogGrowablePathFromUnicodeStringNew(uniChars, numUniChars, attributes, origin, path);
	#endif /* LOG_PARAMETERS */

	*path					= NULL;
	placement.lpDx			= NULL;				/* We allocate memory for this later */
	placement.lpGlyphs		= NULL;				/* We allocate memory for this later */
	placement.lStructSize	= sizeof(placement);
	placement.lpOutString	= NULL;
	placement.lpOrder		= NULL;
	placement.lpCaretPos	= NULL;
	placement.lpClass		= NULL;
	placement.nMaxFit		= 0;
	placement.nGlyphs		= numUniChars + (numUniChars >> 1) + 1;	/* Expect just a few more glyphs */

	BAIL_IF_ERR(err = FskTextContextFromFontAttributesNew(attributes, &textContext));
	BAIL_IF_ERR(err = FskMemPtrNew(placement.nGlyphs * sizeof(int),   (FskMemPtr*)&placement.lpDx));		/* x-advance array */
	BAIL_IF_ERR(err = FskMemPtrNew(placement.nGlyphs * sizeof(WCHAR), (FskMemPtr*)&placement.lpGlyphs));	/* glyph array */
	BAIL_IF_ERR(err = FskMemPtrNew(numUniChars       * sizeof(CHAR),  (FskMemPtr*)&placement.lpClass));		/* character class array */
	for (i = 0; i < numUniChars; i++)
		placement.lpClass[i] = GCPCLASS_LATIN;
	placeResult = GetCharacterPlacementW(textContext->hdc, uniChars, numUniChars, 0, &placement, GCP_GLYPHSHAPE|GCP_LIGATE|GCP_USEKERNING);

	if (origin != NULL)	org = *origin;
	else				org.x = org.y = 0;

	for (i = placement.nGlyphs, dx = placement.lpDx, gl = placement.lpGlyphs; i--; gl++, org.x += *dx++ << 16) {
		BAIL_IF_ERR(err = FskTextContextAppendGlyphPath(*gl, org.x, org.y, textContext));
	}

	*path = textContext->path;	textContext->path = NULL;	/* Steal path from context */
	if (origin != NULL)	*origin = org;						/* Return current caret position */

bail:
	if (placement.lpClass != NULL)	FskMemPtrDispose(placement.lpClass);
	if (placement.lpGlyphs != NULL)	FskMemPtrDispose(placement.lpGlyphs);
	if (placement.lpDx != NULL)		FskMemPtrDispose(placement.lpDx);
	if (textContext != NULL)		FskTextContextDispose(textContext);

	return err;
}


/********************************************************************************
 * FskUnicodeStringGetWidth
 * TODO: Implement a more lightweight version.
 ********************************************************************************/

double
FskUnicodeStringGetWidth(const UInt16 *uniChars, UInt32 numUniChars, const FskFontAttributes *attributes)
{
	FskErr			err;
	FskTextContext	textContext		= NULL;
	DWORD			placeResult;
	GCP_RESULTSW	placement;
	unsigned int	i;
	int				*dx;
	FskFixed		width;

	placement.lpDx			= NULL;				/* We allocate memory for this later */
	placement.lpGlyphs		= NULL;				/* We allocate memory for this later */
	placement.lStructSize	= sizeof(placement);
	placement.lpOutString	= NULL;
	placement.lpOrder		= NULL;
	placement.lpCaretPos	= NULL;
	placement.lpClass		= NULL;
	placement.nMaxFit		= 0;
	placement.nGlyphs		= numUniChars + (numUniChars >> 1) + 1;	/* Expect just a few more glyphs */

	BAIL_IF_ERR(err = FskTextContextFromFontAttributesNew(attributes, &textContext));
	BAIL_IF_ERR(err = FskMemPtrNew(placement.nGlyphs * sizeof(int),   (FskMemPtr*)&placement.lpDx));		/* x-advance array */
	BAIL_IF_ERR(err = FskMemPtrNew(placement.nGlyphs * sizeof(WCHAR), (FskMemPtr*)&placement.lpGlyphs));	/* glyph array */
	BAIL_IF_ERR(err = FskMemPtrNew(numUniChars       * sizeof(CHAR),  (FskMemPtr*)&placement.lpClass));		/* character class array */
	for (i = 0; i < numUniChars; i++)
		placement.lpClass[i] = GCPCLASS_LATIN;
	placeResult = GetCharacterPlacementW(textContext->hdc, uniChars, numUniChars, 0, &placement, GCP_GLYPHSHAPE|GCP_LIGATE|GCP_USEKERNING);

	for (i = placement.nGlyphs, width = 0, dx = placement.lpDx; i--;)
		width += *dx++ << 16;

bail:
	if (placement.lpClass != NULL)	FskMemPtrDispose(placement.lpClass);
	if (placement.lpGlyphs != NULL)	FskMemPtrDispose(placement.lpGlyphs);
	if (placement.lpDx != NULL)		FskMemPtrDispose(placement.lpDx);
	if (textContext != NULL)		FskTextContextDispose(textContext);
	if (err)
		width = err;

	return FskFixedToFloat(width);
}


#if 0
#pragma mark -
#pragma mark WinCE
#endif
/********************************************************************************
 ********************************************************************************
 ********************************************************************************
 *****  IPHONE -- IPHONE -- IPHONE -- IPHONE -- IPHONE -- IPHONE -- IPHONE  *****
 ********************************************************************************
 ********************************************************************************
 ********************************************************************************/
#elif TARGET_OS_IPHONE

struct FskTextContextRecord {
	FskGrowablePath	path;
};

/********************************************************************************
 * FskTextContextFromFontAttributesNew
 ********************************************************************************/

FskErr
FskTextContextFromFontAttributesNew(const FskFontAttributes *at, FskTextContext *pTextContext)
{
	*pTextContext = NULL;
	return kFskErrUnimplemented;
}


/********************************************************************************
 * FskTextContextClone
 ********************************************************************************/

FskErr
FskTextContextClone(FskTextContext textContext, FskTextContext *newTextContext)
{
	*newTextContext = NULL;
	return kFskErrUnimplemented;
}


/********************************************************************************
 * FskTextContextDispose
 ********************************************************************************/

void
FskTextContextDispose(FskTextContext textContext)
{

}


/********************************************************************************
 * FskTextContextAppendGlyphPath
 ********************************************************************************/

FskErr
FskTextContextAppendGlyphPath(FskGlyphID glyphID, FskFixed x0, FskFixed y0, FskTextContext textContext)
{
	return kFskErrUnimplemented;
}


/********************************************************************************
 * FskGrowablePathFromUnicodeStringNew
 ********************************************************************************/

FskErr
FskGrowablePathFromUnicodeStringNew(
	const UInt16			*uniChars,
	UInt32					numUniChars,
	const FskFontAttributes	*attributes,
	FskFixedPoint2D			*origin,
	FskGrowablePath			*path
)
{
	*path = NULL;
	return kFskErrUnimplemented;
}


/********************************************************************************
 * FskUnicodeStringGetWidth
 * TODO: Implement a more lightweight version.
 ********************************************************************************/

double
FskUnicodeStringGetWidth(const UInt16 *uniChars, UInt32 numUniChars, const FskFontAttributes *attributes)
{
	FskErr			err		= kFskErrNone;
	FskGrowablePath	path	= NULL;
	FskFixedPoint2D	origin;

	#if defined(LOG_PARAMETERS)
		LogGrowablePathFromUnicodeStringNew(uniChars, numUniChars, attributes, origin, path);
	#endif /* LOG_PARAMETERS */

	origin.x = 0;
	origin.y = 0;
	BAIL_IF_NEGATIVE(err = FskGrowablePathFromUnicodeStringNew(uniChars, numUniChars, attributes, &origin, &path), origin.x, err);

bail:
	FskGrowablePathDispose(path);
	return FskFixedToFloat(origin.x);
}


#if 0
#pragma mark -
#pragma mark FreeType
#endif
/********************************************************************************
 ********************************************************************************
 ********************************************************************************
 *****		FREETYPE -- FREETYPE -- FREETYPE -- FREETYPE -- FREETYPE		*****
 ********************************************************************************
 ********************************************************************************
 ********************************************************************************/
#elif (FSK_TEXT_FREETYPE == 1)

#include "FskText.h"

#include <ft2build.h>		/* All the header and source files use <angle brackets>, so we do here as well */
#include FT_FREETYPE_H		/* Cute include mechanism, encouraged by FreeType */
#include FT_OUTLINE_H		/* For FT_Outline_Decompose() */

#define XCOORD(xx)		(textContext->origin.x + (xx))	/* The coordinates are already converted to 16.16 in olFuncs */
#define YCOORD(yy)		(textContext->origin.y - (yy))

#define FskFixed_FracBits	16
#define FT_FracBits			6
#define FTToFixedCoord(x)	((x) << (FskFixed_FracBits - FT_FracBits))	/* Convert from FreeType's 26.6 to FskFixed's 16.16 */

typedef enum {
	SEGNONE		= 0,
	SEGMOVE,
	SEGLINE,
	SEGQUAD,
	SEGCUBIC,
	SEGCLOSE,
	SEGGLYPH
} SegType;

struct FskTextContextRecord {
	FskGrowablePath		path;

	FT_Face				face;
	FT_Outline_Funcs	olFuncs;
	FskFixedPoint2D		origin;
	Boolean				hasKerning;
	SegType				lastSegment;
};

extern FT_Library				ftLib;


/********************************************************************************
 * ClosePreviousContour
 ********************************************************************************/

static Boolean
ClosePreviousContour(FskTextContext textContext) {
	switch (textContext->lastSegment) {
		case SEGLINE:
		case SEGQUAD:
		case SEGCUBIC:
			FskGrowablePathAppendSegmentClose(textContext->path);
			textContext->lastSegment = SEGCLOSE;
			return true;
		default:
			break;
	}
	return false;
}


/********************************************************************************
 * MyMoveTo
 ********************************************************************************/

static int
MyMoveTo(const FT_Vector *to, void *userData)
{
	FskTextContext	textContext		= (FskTextContext)userData;

	(void)ClosePreviousContour(textContext);
	textContext->lastSegment = SEGMOVE;
	return	FskGrowablePathAppendSegmentMoveTo(
				XCOORD(to->x),		YCOORD(to->y),
				textContext->path
			);
}


/********************************************************************************
 * MyLineTo
 ********************************************************************************/

static int
MyLineTo(const FT_Vector *to, void *userData)
{
	FskTextContext	textContext		= (FskTextContext)userData;

	textContext->lastSegment = SEGLINE;
	return	FskGrowablePathAppendSegmentLineTo(
				XCOORD(to->x),		YCOORD(to->y),
				textContext->path
			);
}


/********************************************************************************
 * MyQuadTo
 ********************************************************************************/

static int
MyQuadTo(const FT_Vector *control, const FT_Vector *to, void *userData)
{
	FskTextContext	textContext		= (FskTextContext)userData;

	textContext->lastSegment = SEGQUAD;
	return	FskGrowablePathAppendSegmentQuadraticBezierTo(
				XCOORD(control->x),	YCOORD(control->y),
				XCOORD(to->x),		YCOORD(to->y),
				textContext->path
			);
}


/********************************************************************************
 * MyCubicTo
 ********************************************************************************/

static int
MyCubicTo(const FT_Vector *control1, const FT_Vector *control2, const FT_Vector *to, void *userData)
{
	FskTextContext	textContext		= (FskTextContext)userData;

	textContext->lastSegment = SEGCUBIC;
	return	FskGrowablePathAppendSegmentCubicBezierTo(
				XCOORD(control1->x),	YCOORD(control1->y),
				XCOORD(control2->x),	YCOORD(control2->y),
				XCOORD(to->x),			YCOORD(to->y),
				textContext->path
			);
}


/********************************************************************************
 * FskTextContextFromFontAttributesNew
 ********************************************************************************/

FskErr
FskTextContextFromFontAttributesNew(const FskFontAttributes *attributes, FskTextContext *pTextContext)
{
	FskErr			err;
	FskTextContext	textContext;
	UInt32			textStyle;

	/* Allocate context and clear */
	*pTextContext = NULL;
	BAIL_IF_ERR(err = FskMemPtrNewClear(sizeof(struct FskTextContextRecord), (FskMemPtr*)pTextContext));
	textContext = *pTextContext;

	BAIL_IF_ERR(err = FskGrowablePathNew(1024, &textContext->path));

	textContext->olFuncs.move_to	= (FT_Outline_MoveToFunc )(&MyMoveTo);	/* ftimage.h is inconsistent ... */
	textContext->olFuncs.line_to	= (FT_Outline_LineToFunc )(&MyLineTo);	/* ... as to the const-ness of its arguments, ... */
	textContext->olFuncs.conic_to	= (FT_Outline_ConicToFunc)(&MyQuadTo);	/* ... so we cast to overcome that. */
	textContext->olFuncs.cubic_to	= (FT_Outline_CubicToFunc)(&MyCubicTo);
	textContext->olFuncs.shift		= FskFixed_FracBits - FT_FracBits;		/* Convert from 6 to 16 fractional bits */
	textContext->olFuncs.delta		= 0;

	/* Get the face */
	textStyle = kFskTextPlain;
	if (attributes->weight >= ((kFskFontWeightBold + kFskFontWeightNormal)/2))
		textStyle |= kFskTextBold;
	if ((attributes->style == kFskFontStyleItalic) || (attributes->style == kFskFontStyleOblique))
		textStyle |= kFskTextItalic;
	if (attributes->decoration & kFskFontDecorationUnderline)
		textStyle |= kFskTextUnderline;

	textContext->face = NULL;
	/* Determine the font family, or one similar to it */
	if (attributes->family != NULL) {						/* Parse the font family */
		const char	*fs;
		char		fontName[256];
		for (fs = attributes->family; GetNextTokenInCommaSeparatedList(&fs, fontName, sizeof(fontName)); ) {
			if ((textContext->face = FskFTFindFont(fontName, textStyle, attributes->size)) != NULL) {
				LOGD("Succeeded getting text context from \"%s\"", fontName);
				break;
			}
			LOGD("Failed    getting text context from \"%s\"", fontName);
		}
	}
	if (textContext->face == NULL) {		/* Couldn't find the fonts - choose one from our default list */
		BAIL_IF_NULL((textContext->face = FskFTFindFont(NULL, textStyle, attributes->size)), err, kFskErrNotFound);
	}
	textContext->hasKerning = FT_HAS_KERNING(textContext->face);

bail:
	if (err != kFskErrNone) {
		FskTextContextDispose(*pTextContext);
		*pTextContext = NULL;
	}

	return err;
}


/********************************************************************************
 * FskTextContextClone
 ********************************************************************************/

FskErr
FskTextContextClone(FskTextContext textContext, FskTextContext *newTextContext)
{
	FskErr err;
	*newTextContext = NULL;
	BAIL_IF_NULL(textContext, err, kFskErrNone);
	BAIL_IF_ERR(err = FskMemPtrNewClear(sizeof(struct FskTextContextRecord), (FskMemPtr*)newTextContext));
	**newTextContext = *textContext;
	if (textContext->path) {
		(**newTextContext).path = NULL;
		BAIL_IF_ERR(err = FskGrowablePathClone(textContext->path, &(**newTextContext).path));
	}
bail:
	return err;
}


/********************************************************************************
 * FskTextContextDispose
 ********************************************************************************/

void
FskTextContextDispose(FskTextContext textContext)
{
	if (textContext != NULL) {
		if (textContext->path != NULL)
			FskGrowablePathDispose(textContext->path);
		(void)FskMemPtrDispose(textContext);
	}
}


/********************************************************************************
 * FskTextContextAppendGlyphPath
 ********************************************************************************/

FskErr
FskTextContextAppendGlyphPath(FskGlyphID glyphID, FskFixed x0, FskFixed y0, FskTextContext textContext)
{
	FskErr				err;

	BAIL_IF_ERR(err = FT_Load_Glyph(textContext->face, glyphID, FT_LOAD_NO_BITMAP));
	BAIL_IF_FALSE((textContext->face->glyph->format == FT_GLYPH_FORMAT_OUTLINE), err, kFskErrBadData);
	textContext->origin.x = x0;
	textContext->origin.y = y0;
	err = FT_Outline_Decompose(&textContext->face->glyph->outline, &textContext->olFuncs, textContext);
	if (ClosePreviousContour(textContext)) {									/* Don't punctuate spaces */
		(void)FskGrowablePathAppendSegmentEndGlyph(textContext->path);
		textContext->lastSegment = SEGGLYPH;
	}

bail:
	return err;
}


/********************************************************************************
 * FskGetGlyphStringFromUnicodeString
 * It seems as if FreeType only supports 1:1 mapping between characters and glyphs,
 * i.e. it doesn't accommodate many-to-one (e.g. ligatures)
 * or one-to-many (e.g. diacriticals).
 ********************************************************************************/

FskErr
FskNewGlyphStringFromUnicodeString(const UInt16 *uniChars, UInt32 numUniChars, FskTextContext textContext, FskGlyphID **pGlyphs, FskFixedVector2D **pOffsets, UInt32 *numGlyphs)
{
	FskErr				err			= kFskErrNone;
	FskGlyphID			*glyphs		= NULL;
	FskFixedVector2D	*offsets	= NULL;
	FskFixed			x			= 0;

	if (pGlyphs)	*pGlyphs	= NULL;																/* Initialize optional return values to NULL in case of error */
	if (pOffsets)	*pOffsets	= NULL;

	BAIL_IF_NULL(numGlyphs, err, kFskErrInvalidParameter);											/* We require a place to return the number of glyphs */
	*numGlyphs = numUniChars;																		/* There seems to be a 1:1 mapping of characters to glyphs in FreeType */

	BAIL_IF_ERR(err = FskMemPtrNew(*numGlyphs * sizeof(*glyphs),  (FskMemPtr*)(void*)(&glyphs)));	/* Allocate glyphs */
	BAIL_IF_ERR(err = FskMemPtrNew(*numGlyphs * sizeof(*offsets), (FskMemPtr*)(void*)(&offsets)));	/* Allocate offsets */

	for ( ; numUniChars--; glyphs++, offsets++) {
		*glyphs = FT_Get_Char_Index(textContext->face, *uniChars++);								/* Get the glyph corresponding to each character */
		offsets->x = x;																				/* Set the horizontal offsets for each character in the string */
		offsets->y = 0;
		x += FTToFixedCoord(textContext->face->glyph->advance.x);									/* Advance the horizontal offset by the glyph advance */
	}

		/* No errors: transfer supplementary results, if desired */
	if (pGlyphs)	{ *pGlyphs  = glyphs;	glyphs  = NULL;	}										/* If the glyphs  were desired, transfer them from glyphs  to *pGlyphs  */
	if (pOffsets)	{ *pOffsets = offsets;	offsets = NULL;	}										/* If the offsets were desired, transfer them from offsets to *pOffsets */

bail:
	(void)FskMemPtrDispose(glyphs);																	/* Dispose of the glyphs  if they weren't otherwise returned */
	(void)FskMemPtrDispose(offsets);																/* Dispose of the offsets if they weren't otherwise returned */

	return err;
}


/********************************************************************************
 * FskUnicodeStringGetWidth
 ********************************************************************************/

double
FskUnicodeStringGetWidth(
   const UInt16				*uniChars,
   UInt32					numUniChars,
   const FskFontAttributes	*attributes
)
{
#if 0	/* This might work, but needs to be tested */
	FskGlyphID			glyph;
	FskFixed			x		= 0;
	FskTextContext textContext	= NULL;

	BAIL_IF_ERR(err = FskTextContextFromFontAttributesNew(attributes, &textContext));

	for ( ; numUniChars--; uniChars++) {
		glyph = FT_Get_Char_Index(textContext->face, *uniChars++);
		x += FTToFixedCoord(textContext->face->glyph->advance.x);
	}

bail:

	return FskFixedToFloat(x);
#else	/* This will work, but is at overkill */
	FskErr			err		= kFskErrNone;
	FskGrowablePath	path	= NULL;
	FskFixedPoint2D	origin;

	origin.x = 0;
	origin.y = 0;
	BAIL_IF_NEGATIVE(err = FskGrowablePathFromUnicodeStringNew(uniChars, numUniChars, attributes, &origin, &path), origin.x, err);

bail:
	FskGrowablePathDispose(path);
	return FskFixedToFloat(origin.x);
#endif
}


/********************************************************************************
 * FskGrowablePathFromUnicodeStringNew
 ********************************************************************************/

FskErr
FskGrowablePathFromUnicodeStringNew(
	const UInt16			*uniChars,
	UInt32					numUniChars,
	const FskFontAttributes	*attributes,
	FskFixedPoint2D			*origin,
	FskGrowablePath			*path
)
{
	FskErr			err;
	FskGlyphID		glyphID, lastGlyphID;
	FskFixedPoint2D	org;
	FskTextContext	textContext			= NULL;
	#ifdef USE_MISSING_GLYPH_GLYPH
		FskGlyphID	missingCharGlyphID	= 0;
	#endif /* USE_MISSING_GLYPH_GLYPH */

	#if defined(LOG_PARAMETERS)
		LogGrowablePathFromUnicodeStringNew(uniChars, numUniChars, attributes, origin, path);
	#endif /* LOG_PARAMETERS */

	*path = NULL;
	BAIL_IF_ERR(err = FskTextContextFromFontAttributesNew(attributes, &textContext));

	if (origin != NULL)	org = *origin;
	else				org.x = org.y = 0;

	for (lastGlyphID = 0 ; numUniChars--; uniChars++, lastGlyphID = glyphID) {
		glyphID = FT_Get_Char_Index(textContext->face, *uniChars);									/* Character is missing if the glyphID is 0 */
		if (textContext->hasKerning && (lastGlyphID != 0) && (glyphID != 0)) {
			FT_Vector delta;
			if ((err = FT_Get_Kerning(textContext->face, lastGlyphID, glyphID, ft_kerning_default, &delta)) == kFskErrNone)
				org.x += (delta.x << (16 - 6));
		}
		if (glyphID != 0) {		/* Glyph is present */
			BAIL_IF_ERR(err = FskTextContextAppendGlyphPath(glyphID, org.x, org.y, textContext));	/* This has the side effect of loading the glyph ... */
			org.x += FTToFixedCoord(textContext->face->glyph->advance.x);							/* ... so we can get the advance width */
		}
		#ifdef USE_MISSING_GLYPH_GLYPH
		else {					/* Glyph is missing */
			BAIL_IF_ERR(err = FskTextContextAppendGlyphPath(missingCharGlyphID, org.x, org.y, textContext));
			org.x += FTToFixedCoord(textContext->face->glyph->advance.x);
		}
		#endif /* USE_MISSING_GLYPH_GLYPH */
	}

	*path = textContext->path;	textContext->path = NULL;											/* Steal growable path */
	if (origin != NULL)	*origin = org;																/* Return current caret position */


bail:
	if (textContext != NULL)	FskTextContextDispose(textContext);

	return err;
}

#if 0
#pragma mark -
#pragma mark UFST
#endif

/********************************************************************************
 ********************************************************************************
 ********************************************************************************
 *****		UFST -- UFST -- UFST -- UFST -- UFST -- UFST -- UFST -- UFST    *****
 ********************************************************************************
 ********************************************************************************
 ********************************************************************************/

#elif (FSK_TEXT_UFST == 1)

// Stubs for UFST
struct FskTextContextRecord {
	FskGrowablePath	path;
};

/********************************************************************************
 * FskTextContextFromFontAttributesNew
 ********************************************************************************/

FskErr
FskTextContextFromFontAttributesNew(const FskFontAttributes *attributes, FskTextContext *pTextContext)
{
	pTextContext = NULL;
	return kFskErrUnimplemented;
}


/********************************************************************************
 * FskTextContextClone
 ********************************************************************************/

FskErr
FskTextContextClone(FskTextContext textContext, FskTextContext *newTextContext)
{
	newTextContext = NULL;
	return kFskErrUnimplemented;
}


/********************************************************************************
 * FskTextContextDispose
 ********************************************************************************/

void
FskTextContextDispose(FskTextContext textContext)
{
}


/********************************************************************************
 * FskTextContextAppendGlyphPath
 ********************************************************************************/

FskErr
FskTextContextAppendGlyphPath(FskGlyphID glyphID, FskFixed x0, FskFixed y0, FskTextContext textContext)
{
	return kFskErrUnimplemented;
}


/********************************************************************************
 * FskGetGlyphStringFromUnicodeString
 * It seems as if FreeType only supports 1:1 mapping between characters and glyphs,
 * i.e. it doesn't accommodate many-to-one (e.g. ligatures)
 * or one-to-many (e.g. diacriticals).
 ********************************************************************************/

FskErr
FskNewGlyphStringFromUnicodeString(const UInt16 *uniChars, UInt32 numUniChars, FskTextContext textContext, FskGlyphID **pGlyphs, FskFixedVector2D **pOffsets, UInt32 *numGlyphs)
{
	*pGlyphs   = NULL:
	*pOffsets  = NULL;
	*numGlyphs = 0;
	return kFskErrUnimplemented;
}


/********************************************************************************
 * FskUnicodeStringGetWidth
 ********************************************************************************/

double
FskUnicodeStringGetWidth(
   const UInt16				*uniChars,
   UInt32					numUniChars,
   const FskFontAttributes	*attributes
)
{
	return 0;
}


/********************************************************************************
 * FskGrowablePathFromUnicodeStringNew
 ********************************************************************************/

FskErr
FskGrowablePathFromUnicodeStringNew(
	const UInt16			*uniChars,
	UInt32					numUniChars,
	const FskFontAttributes	*attributes,
	FskFixedPoint2D			*origin,
	FskGrowablePath			*path
)
{
	*path = NULL;

	#if defined(LOG_PARAMETERS)
		LogGrowablePathFromUnicodeStringNew(uniChars, numUniChars, attributes, origin, path);
	#endif /* LOG_PARAMETERS */

	return kFskErrUnimplemented;
}


/********************************************************************************
 ********************************************************************************
 ********************************************************************************
 *****	XXX -- XXX -- XXX -- XXX -- XXX -- XXX -- XXX -- XXX -- XXX -- XXX	*****
 ********************************************************************************
 ********************************************************************************
 ********************************************************************************/

#else
	#error Unimplemented for this platform
#endif




#if 0
#pragma mark -
#pragma mark Common
#endif
/********************************************************************************
 ********************************************************************************
 ********************************************************************************
 *****	COMMON -- COMMON -- COMMON -- COMMON -- COMMON -- COMMON -- COMMON	*****
 ********************************************************************************
 ********************************************************************************
 ********************************************************************************/



/********************************************************************************
 * FskTextContextGetPath
 ********************************************************************************/

FskErr
FskTextContextGetPath(FskTextContext textContext, FskConstPath *path)
{
	FskPath	myPath;
	FskErr err;
	err = FskGrowablePathNewPath(textContext->path, &myPath);
	*path = myPath;
	return err;
}


/********************************************************************************
 * FskGlyphStringDispose
 ********************************************************************************/

void
FskGlyphStringDispose(FskGlyphID *glyphs, FskFixedVector2D *offset)
{
	if (glyphs != NULL)	(void)FskMemPtrDispose(glyphs);
	if (offset != NULL)	(void)FskMemPtrDispose(offset);
}


/********************************************************************************
 * FskPathFromUnicodeStringNew
 ********************************************************************************/

FskErr
FskPathFromUnicodeStringNew(
	const UInt16			*uniChars,
	UInt32					numUniChars,
	const FskFontAttributes	*attributes,
	FskFixedPoint2D			*origin,
	FskPath					*path
)
{
	FskGrowablePath	grPath	= NULL;
	FskErr			err;

	BAIL_IF_ZERO(numUniChars, err, kFskErrNothingRendered);
	BAIL_IF_ERR(err = FskGrowablePathFromUnicodeStringNew(uniChars, numUniChars, attributes, origin, &grPath));
	err = FskGrowablePathNewPath(grPath, path);

bail:
	if (grPath != NULL)			FskGrowablePathDispose(grPath);

	return err;
}




/********************************************************************************
 ********************************************************************************
 *****					Normative SVG Specifications						*****
 ********************************************************************************
 ********************************************************************************
 *
 ****************************** Tiny SVG Attributes *****************************
 * <!-- module: svg-basic-text.mod ........................ -->
 * <!ENTITY % FontFamilyValue.datatype "CDATA" >
 * <!ENTITY % FontSizeValue.datatype "CDATA" >
 * <!ENTITY % SVG.TextContent.extra.attrib "" >
 * <!ENTITY % SVG.TextContent.attrib "
 *		text-anchor	( start | middle | end | inherit ) #IMPLIED
 * 		%SVG.TextContent.extra.attrib;
 * ">
 * <!ENTITY % SVG.Font.extra.attrib "" >
 * <!ENTITY % SVG.Font.attrib "
 *		font-family	%FontFamilyValue.datatype; #IMPLIED
 *		font-size	%FontSizeValue.datatype; #IMPLIED
 *		font-style	( normal | italic | oblique | inherit ) #IMPLIED
 *		font-weight	( normal | bold | bolder | lighter | 100 | 200 | 300 | 400 | 500 | 600 | 700 | 800 | 900 | inherit ) #IMPLIED
 *		%SVG.Font.extra.attrib;
 * ">
 ****************************** Basic SVG Attributes ****************************
 * <!-- module: svg-text.mod .............................. -->
 * <!ENTITY % BaselineShiftValue.datatype	"CDATA" >
 * <!ENTITY % FontFamilyValue.datatype		"CDATA" >
 * <!ENTITY % FontSizeValue.datatype		"CDATA" >
 * <!ENTITY % FontSizeAdjustValue.datatype	"CDATA" >
 * <!ENTITY % GlyphOrientationHorizontalValue.datatype	"CDATA" >
 * <!ENTITY % GlyphOrientationVerticalValue.datatype	"CDATA" >
 * <!ENTITY % KerningValue.datatype			"CDATA" >
 * <!ENTITY % SpacingValue.datatype			"CDATA" >
 * <!ENTITY % TextDecorationValue.datatype	"CDATA" >
 * <!ENTITY % SVG.Text.extra.attrib			"" >
 * <!ENTITY % SVG.Text.attrib "
 *		writing-mode ( lr-tb | rl-tb | tb-rl | lr | rl | tb | inherit ) #IMPLIED
 *		%SVG.Text.extra.attrib;
 * ">
 * <!ENTITY % SVG.TextContent.extra.attrib "" >
 * <!ENTITY % SVG.TextContent.attrib "
 *		alignment-baseline	( auto | baseline | before-edge | text-before-edge | middle | central | after-edge
 *							| text-after-edge | ideographic | alphabetic | hanging | mathematical | inherit ) #IMPLIED
 *		baseline-shift		%BaselineShiftValue.datatype; #IMPLIED
 *		direction			( ltr | rtl | inherit ) #IMPLIED
 *		dominant-baseline	( auto | use-script | no-change | reset-size | ideographic | alphabetic | hanging
 *							| mathematical | central | middle | text-after-edge | text-before-edge | inherit ) #IMPLIED
 *		glyph-orientation-horizontal	%GlyphOrientationHorizontalValue.datatype; #IMPLIED
 *		glyph-orientation-vertical		%GlyphOrientationVerticalValue.datatype; #IMPLIED
 *		kerning				%KerningValue.datatype; #IMPLIED
 *		letter-spacing		%SpacingValue.datatype; #IMPLIED
 *		text-anchor			( start | middle | end | inherit ) #IMPLIED
 *		text-decoration		( none | [ underline || overline || line-through || blink ] | inherit ) #IMPLIED
 *		unicode-bidi		( normal | embed | bidi-override | inherit ) #IMPLIED
 *		word-spacing		%SpacingValue.datatype; #IMPLIED
 *		%SVG.TextContent.extra.attrib;
 * ">
 * <!ENTITY % SVG.Font.extra.attrib "" >
 * <!ENTITY % SVG.Font.attrib "
 *		font-family			%FontFamilyValue.datatype; #IMPLIED
 *		font-size			%FontSizeValue.datatype; #IMPLIED
 *		font-size-adjust	%FontSizeAdjustValue.datatype; #IMPLIED
 *		font-stretch		( normal | wider | narrower | ultra-condensed | extra-condensed | condensed | semi-condensed
 *							| semi-expanded | expanded | extra-expanded | ultra-expanded | inherit ) #IMPLIED
 *		font-style			( normal | italic | oblique | inherit ) #IMPLIED
 *		font-variant		( normal | small-caps | inherit ) #IMPLIED
 *		font-weight			( normal | bold | bolder | lighter | 100 | 200 | 300 | 400 | 500 | 600 | 700 | 800 | 900 | inherit ) #IMPLIED
 *		%SVG.Font.extra.attrib;
 * ">
 ********************************************************************************
 */

