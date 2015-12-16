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
#import "Fsk.h"

#if TARGET_OS_IPHONE
#import <UIKit/UIKit.h>
#import <CoreText/CoreText.h>
#import <AVFoundation/AVFoundation.h>
#else /* !TARGET_OS_IPHONE */
#import <Cocoa/Cocoa.h>
#import <AppKit/NSApplication.h>
#endif /* TARGET_OS_IPHONE */
#import <AudioUnit/AudioUnit.h>
#import <AudioToolbox/AudioToolbox.h>

#define __FSKAUDIO_PRIV__
#define __FSKBITMAP_PRIV__
#define __FSKTEXT_PRIV__

#import "FskCocoaSupportCommon.h"
#import "FskEnvironment.h"
#import "FskMemory.h"
#import "FskString.h"
#import "FskTransferAlphaBitmap.h"
#if TARGET_OS_IPHONE
#import "FskCocoaSupportPhone.h"
#import "FskCocoaApplicationPhone.h"
#import "FskCocoaViewControllerPhone.h"
#endif /* TARGET_OS_IPHONE */


#if SUPPORT_INSTRUMENTATION

	#define LOG_PARAMETERS
	#define LOG_CACHE
	//#define LOG_VERTICES

	#define COCOA_DEBUG	1
	#define FskInstrumentedTypeEnabled(type, level)	FskInstrumentedTypeHasListenersForLevel(FskInstrumentedGlobalType(type), level)	/**< Whether instrumentation will print anything. */
	#define FskInstrumentedGlobalType(type)			(&(g##type##TypeInstrumentation))												/**< The data structure created with FskInstrumentedSimpleType(). */
#else /* !SUPPORT_INSTRUMENTATION */
	#define FskInstrumentedTypeEnabled(type, level)	(0)																				/**< Instrumentation will not print anything. */
#endif /* SUPPORT_INSTRUMENTATION */

FskInstrumentedSimpleType(CocoaCommon, cocoacommon);												/**< This declares the types needed for instrumentation. */

#if COCOA_DEBUG
	#define	LOGD(...)	FskCocoaCommonPrintfDebug(__VA_ARGS__)										/**< Print debugging logs. */
	#define	LOGI(...)	FskCocoaCommonPrintfVerbose(__VA_ARGS__)									/**< Print information logs. */
#endif	/* COCOA_DEBUG */
#define		LOGE(...)	FskCocoaCommonPrintfMinimal(__VA_ARGS__)									/**< Print error logs always, when instrumentation is on. */
#ifndef     LOGD
	#define LOGD(...)																				/**< Don't print debugging logs. */
#endif   		/* LOGD */
#ifndef     LOGI
	#define LOGI(...)																				/**< Don't print information logs. */
#endif   		/* LOGI */
#ifndef     LOGE
	#define LOGE(...)																				/**< Don't print error logs. */
#endif   	/* LOGE */
#define LOGD_ENABLED()	FskInstrumentedTypeEnabled(CocoaCommon, kFskInstrumentationLevelDebug)		/**< Whether LOGD() will print anything. */
#define LOGI_ENABLED()	FskInstrumentedTypeEnabled(CocoaCommon, kFskInstrumentationLevelVerbose)	/**< Whether LOGI() will print anything. */
#define LOGE_ENABLED()	FskInstrumentedTypeEnabled(CocoaCommon, kFskInstrumentationLevelMinimal)	/**< Whether LOGE() will print anything. */


#pragma mark --- defines ---

#define DEBUG_TEXT_BOX		0
#define DEBUG_TEXT_STYLE	0

#if USE_FRACTIONAL_TEXT_SIZE
#define TEXTSIZE_TO_FLOAT(size)	((size >= 0x8000) ? FskFixedToFloat(size) : (CGFloat)size)
#else
#define ROUND_TEXTSIZE(size) ((size >= 0x8000) ? FskRoundFixedToInt(size) : size)
#define TEXTSIZE_TO_FLOAT(size)	((CGFloat)ROUND_TEXTSIZE(size))
#endif

#pragma mark --- structures ---

#if USE_CORE_TEXT
#define MAXLINELIST 5000
#define LINEHASHSIZE	((MAXLINELIST * 3) / 2)

typedef struct SizeInfo {
	CGFloat ascent;
	CGFloat descent;
	CGFloat width;
	CGFloat height;
} SizeInfoRecord, *SizeInfo;

typedef struct BytesInfo {
	UInt32 charsLength;
	UInt32 bytesLength;
} BytesInfoRecored, *BytesInfo;

typedef struct LayoutInfo {
	Boolean useGlyph;
	UInt32 codesLen;
	UInt16 *codes;
	FskFixed *layout;
	CGFloat width;
} LayoutInfoRecord, *LayoutInfo;

typedef struct LineListRecord {
	struct LineListRecord *next;
	struct LineListRecord *prev;

	UInt32 textLength;
	char *text;
	FskColorRGBARecord colorRGB;
	UInt32 blendLevel;
	UInt32 textSize;
	UInt32 textStyle;
	const char *fontName;
	CGFloat constraintWidth;

	NSString *textString;
	UInt32 lineLength;
	CTLineRef line;
	SizeInfoRecord size;
	BytesInfoRecored bytes;
	LayoutInfoRecord layout;
	struct LineListRecord *hnext;
} LineListRecord, *LineList;

/*
  | desc | flag | code |
  |  4   |  1   |  11  |
 */
#define RUNFONTS_BITS	4
#define MAXRUNFONTS (1 << RUNFONTS_BITS)
#define DESC_SHIFT	(sizeof(UInt16) * 8 - RUNFONTS_BITS)
#define MAX_IMMEDIATE_CODE	(1 << (DESC_SHIFT - 1))

struct runFontDescriptor {
	CFStringRef name;
	UInt16 *glyphtab;
	struct runFontList {
		struct runFontList *next;
		CTFontRef fontRef;
		UInt32 fontSize;
		UInt32 fontStyle;
	} *list;
};

typedef struct FontListRecord {
	struct FontListRecord *next;
	CFStringRef name;
	CTFontRef fontRef;
	int numRunFonts;
	struct runFontDescriptor runFonts[MAXRUNFONTS];
} FontListRecord, *FontList;

struct FskTextFormatCacheRecord {
	UInt32 textSize;
	CGFloat ascent;
	CGFloat descent;
	CGFloat leading;
	CGFloat width;
	Boolean strikeThrough;
	FontList fontInfo;	// readonly
	CTFontRef fontRef;
};

struct FskTextEngineStateRecord {
	FskListDoubleRecord lineList;
	int lineListCount;
	LineList lineHashTable[LINEHASHSIZE];
	FontList fontList;
};

static FskTextEngineStateRecord gTextState;
#endif	/* USE_CORE_TEXT */

#pragma mark --- bitmap ---

Boolean FskCocoaBitmapUseGL()
{
	static int useGL = -1;

	if (useGL < 0)
	{
		// avoid Fsk has not been initialzied
		if (FskThreadGetCurrent() == NULL) {
			return true;
		}
		const char *env = FskEnvironmentGet("useGL");

		useGL = ((env != NULL) && (FskStrCompare(env, "1") == 0)) ? 1 : 0;
	}

	return (Boolean)useGL;
}

#pragma mark --- text ---
#if USE_CORE_TEXT

#if SUPPORT_INSTRUMENTATION
void FskCocoaTextFormatCacheInfoStringGet(FskTextFormatCache cache, char **pInfoStr)
{
	char *nameBuf = NULL;
	CFIndex length, maxSize;
	CTFontSymbolicTraits traits;
	char styleStr[10], *s;

	traits = CTFontGetSymbolicTraits(cache->fontInfo->fontRef);
	s = styleStr;
	if (traits & kCTFontItalicTrait)		*s++ = 'I';
	if (traits & kCTFontBoldTrait)			*s++ = 'B';
	if (traits & kCTFontExpandedTrait)		*s++ = 'E';
	if (traits & kCTFontCondensedTrait)		*s++ = 'C';
	if (traits & kCTFontMonoSpaceTrait)		*s++ = 'M';
	if (traits & kCTFontVerticalTrait)		*s++ = 'V';
	if (traits & kCTFontUIOptimizedTrait)	*s++ = 'U';
	if (cache->strikeThrough)				*s++ = 'S';
	*s = 0;
	if (styleStr[0] == 0)	{ styleStr[0] = 'P'; styleStr[1] = 0; }

	*pInfoStr = NULL;
	length  = CFStringGetLength(cache->fontInfo->name);
	maxSize = CFStringGetMaximumSizeForEncoding(length, kCFStringEncodingUTF8) + 1;
	if ((kFskErrNone != FskMemPtrNew(maxSize, &nameBuf)) || !CFStringGetCString(cache->fontInfo->name, nameBuf, maxSize, kCFStringEncodingUTF8))	goto bail;
	asprintf(pInfoStr, "<TextFormatCache fontName=\"%s\" textSize=%g traits=\"%s\" ascent=%g descent=%g leading=%g width=%g numRunFonts=%d/>",
		 nameBuf, cache->textSize/65536., styleStr, cache->ascent, cache->descent, cache->leading, cache->width, cache->fontInfo->numRunFonts);
bail:
	FskMemPtrDispose(nameBuf);
}
#endif /* SUPPORT_INSTRUMENTATION */

#if 0
static char *
debug_text(const char *text, UInt32 textLength)
{
	static char buf[1024];

	if (textLength >= 1024)
	{
		FskStrCopy(buf, "text is too long");
	}
	else
	{
		FskMemCopy(buf, text, textLength);
		buf[textLength] = '\0';
	}
	return buf;
}
#endif // 0

static void
freeLineList_(LineList lineList)
{
	if (lineList != NULL) {
		if (lineList->text != NULL)
			FskMemPtrDispose((FskMemPtr)lineList->text);
		if (lineList->fontName != NULL)
			FskMemPtrDispose((FskMemPtr)lineList->fontName);
		if (lineList->line != NULL)
			CFRelease(lineList->line);
		if (lineList->layout.codes != NULL)
			FskMemPtrDispose((FskMemPtr)lineList->layout.codes);
		if (lineList->layout.layout != NULL)
			FskMemPtrDispose((FskMemPtr)lineList->layout.layout);
		[lineList->textString release];
	}
}

static void
freeLineList(LineList lineList)
{
	freeLineList_(lineList);
	FskMemPtrDispose((FskMemPtr)lineList);
}

static FontList
registerBaseFont(FskTextEngineState state, CFStringRef fontName, UInt32 textSize)
{
	FontList fl;

	for (fl = state->fontList; fl != NULL; fl = fl->next) {
		if (CFStringCompare(fontName, fl->name, 0) == 0)
			break;
	}
	if (fl == NULL) {
		if (FskMemPtrNewClear(sizeof(FontListRecord), &fl) != kFskErrNone)
			return NULL;
		fl->name = CFStringCreateCopy(NULL, fontName);
		if ((fl->fontRef = CTFontCreateWithName(fontName, TEXTSIZE_TO_FLOAT(textSize), NULL)) == NULL) {
			CFRelease(fl->name);
			FskMemPtrDispose(fl);
			return NULL;
		}
		fl->next = state->fontList;
		state->fontList = fl;
	}
	return fl;
}

static CTFontRef
createFontWithAttributes(CTFontRef fref, UInt32 textSizeIn, UInt32 textStyle)
{
	CGFloat textSize = TEXTSIZE_TO_FLOAT(textSizeIn);
	CTFontSymbolicTraits iTraits = 0;
	CTFontRef aref = NULL;

	if ((textStyle & kFskTextBold) != 0)
		iTraits |= kCTFontBoldTrait;
	if ((textStyle & kFskTextItalic) != 0)
		iTraits |= kCTFontItalicTrait;
	if (iTraits != 0 && CTFontGetSymbolicTraits(fref) != iTraits) {	// if the font already has the given traits attr, it can be redundant and might cause an unexpected result...
		aref = CTFontCreateCopyWithSymbolicTraits(fref, textSize, NULL, iTraits, iTraits);
		if (aref == NULL) {
			// if fref has no symbolic traits, search for substitution.
			CTFontRef arefOrig;
			CTFontDescriptorRef descOrig, desc;
			NSSet *set;
			NSArray *descs;

			descOrig = CTFontCopyFontDescriptor(fref);
//			{
//				NSString *name = (NSString *)CTFontDescriptorCopyAttribute(descOrig, kCTFontNameAttribute);
//				NSLog(@"orig name %@", name);
//				[name release];
//			}
			set = [NSSet setWithObjects:(id)kCTFontTraitsAttribute, nil];
			descs = (NSArray *)CTFontDescriptorCreateMatchingFontDescriptors(descOrig, (CFSetRef)set);
			CFRelease(descOrig);
			for (NSUInteger i = 0; i < descs.count; i++) {
				desc = (CTFontDescriptorRef)[descs objectAtIndex:i];
				arefOrig = CTFontCreateWithFontDescriptor(desc, textSize, NULL);	// this font should be registered??
				aref = CTFontCreateCopyWithSymbolicTraits(arefOrig, textSize, NULL, iTraits, iTraits);
				CFRelease(arefOrig);
				if (aref != NULL) {
//					{
//						desc = CTFontCopyFontDescriptor(aref);
//						NSString *name = (NSString *)CTFontDescriptorCopyAttribute(desc, kCTFontNameAttribute);
//						NSLog(@"aref name %@", name);
//						[name release];
//						CFRelease(desc);
//					}
					break;
				}
			}
			[descs release];
		}
	}
	if (aref == NULL) {
		CTFontDescriptorRef fontDescriptor = NULL;
		if (textStyle & kFskTextCode) {
			const UniChar characters[2]={'W','.'};
			CGGlyph glyphs[2];
			CGSize advances[2];
			CTFontGetGlyphsForCharacters(fref, characters, glyphs, 2);
			CTFontGetAdvancesForGlyphs(fref, kCTFontHorizontalOrientation, glyphs, advances, 2);
			float advance = round(advances[0].width);
			CFTypeRef keys[1];
			CFTypeRef values[1];
			keys[0] = kCTFontFixedAdvanceAttribute;
			values[0] = CFNumberCreate(nil, kCFNumberFloatType, (void *)&advance);
			CFDictionaryRef fontAttributes = CFDictionaryCreate(NULL, (const void **)keys, (const void **)values, 1, NULL, NULL);
			fontDescriptor = CTFontDescriptorCreateWithAttributes(fontAttributes);
		}
		aref = CTFontCreateCopyWithAttributes(fref, textSize, NULL, fontDescriptor);
	}
//	{
//		CTFontDescriptorRef desc = CTFontCopyFontDescriptor(aref);
//		NSString *name = (NSString *)CTFontDescriptorCopyAttribute(desc, kCTFontNameAttribute);
//		NSLog(@"aref name %@", name);
//		[name release];
//		CFRelease(desc);
//	}
	return aref;
}

static int
registerRunFont(CTFontRef runFont, FskTextFormatCache cache)
{
	CFStringRef name = CTFontCopyPostScriptName(runFont);
	FontList fl = cache->fontInfo;

	if (name == NULL) {
		NSLog(@"no postscript font");
		return 0;
	}
	for (int i = 0; i < fl->numRunFonts; i++) {
		if (CFStringCompare(fl->runFonts[i].name, name, 0) == 0) {
			CFRelease(name);
			return i;
		}
	}
	if (fl->numRunFonts >= MAXRUNFONTS) {
		NSLog(@"too many run fonts");
		CFRelease(name);
		return 0;	// @@
	}
	struct runFontDescriptor *desc = &fl->runFonts[fl->numRunFonts];
	desc->name = name;
	desc->list = NULL;	// create the font ref on demand
	desc->glyphtab = NULL;
	return fl->numRunFonts++;
}

static UInt16
encodeGlyphCode(int d, UInt16 code, FskTextFormatCache cache)
{
	FontList fl = cache->fontInfo;
	struct runFontDescriptor *desc;

	if (d >= fl->numRunFonts)
		return 0;
	desc = &fl->runFonts[d];
top:
	if (code >= MAX_IMMEDIATE_CODE) {
		if (desc->glyphtab == NULL) {
			if (FskMemPtrNewClear(sizeof(UInt16) * MAX_IMMEDIATE_CODE, (FskMemPtr *)&desc->glyphtab) != kFskErrNone) {
				NSLog(@"no more core");
				return 0;	// @@
			}
		}
		int i;
		UInt16 *tab = desc->glyphtab;
		for (i = 0; i < MAX_IMMEDIATE_CODE; i++) {
			if (tab[i] == code || tab[i] == 0) {
				tab[i] = code;
				code = i | MAX_IMMEDIATE_CODE;
				break;
			}
		}
		if (i >= MAX_IMMEDIATE_CODE) {
			// table overflow
			if (fl->numRunFonts >= MAXRUNFONTS) {
				NSLog(@"no more descriptors!");
				return 0;	// @@ no more descriptors!
			}
			CFStringRef name = CFStringCreateCopy(NULL, desc->name);
			if (name == NULL) {
				NSLog(@"no more core");
				return 0;	// @@
			}
			d = fl->numRunFonts++;
			desc = &fl->runFonts[d];
			desc->name = name;
			desc->list = NULL;
			desc->glyphtab = NULL;
			goto top;
		}
	}
	return (d << DESC_SHIFT) | code;
}

static UInt16
decodeGlyphCode(UInt16 code, FskTextFormatCache cache)
{
	if (code & MAX_IMMEDIATE_CODE) {
		struct runFontDescriptor *desc = &cache->fontInfo->runFonts[code >> DESC_SHIFT];
		return desc->glyphtab[code & (MAX_IMMEDIATE_CODE - 1)];
	}
	else
		return code & (MAX_IMMEDIATE_CODE - 1);
}

static const char*
GetCStringFromCFString(CFStringRef cfString, char **tmpBuf)
{
	const char *nameCstr;
	if ((NULL == (nameCstr = CFStringGetCStringPtr(cfString, kCFStringEncodingMacRoman)))	&&	/* This seems to be most common */
		(NULL == (nameCstr = CFStringGetCStringPtr(cfString, kCFStringEncodingASCII)))		&&
		(NULL == (nameCstr = CFStringGetCStringPtr(cfString, kCFStringEncodingUTF8)))
	) {
		CFIndex z = CFStringGetLength(cfString) * 2 + 2;
		if ((kFskErrNone == FskMemPtrNew((UInt32)z, tmpBuf)) && CFStringGetCString(cfString, *tmpBuf, z, kCFStringEncodingUTF8))
			nameCstr = *tmpBuf;
	}
	return nameCstr;
}

static CTFontRef
getRunFont(FskTextEngineState state, UInt16 code, UInt32 textSize, UInt32 textStyle, const char *fontName, FskTextFormatCache cache)
{
	FontList fl = cache->fontInfo;
	int d = code >> DESC_SHIFT;
	struct runFontList *l;
	struct runFontDescriptor *desc;
	CTFontRef fref;
	char *tmpCstr = NULL;

	if (d >= fl->numRunFonts)
		return fl->fontRef;	// GL draws a "replacment code" (0) without asking the glyph code
	desc = &fl->runFonts[d];
	for (l = desc->list; l != NULL; l = l->next) {
		if (l->fontSize == textSize && l->fontStyle == textStyle)
			return l->fontRef;
	}
	if (FskMemPtrNew(sizeof(struct runFontList), (FskMemPtr *)&l) != kFskErrNone) {
		LOGE("no more core");
		return NULL;
	}
	if ((fref = CTFontCreateWithName(desc->name, TEXTSIZE_TO_FLOAT(textSize), NULL)) == NULL) {
		LOGE("Can't create a run font: \"%s\"", GetCStringFromCFString(desc->name, &tmpCstr));
		FskMemPtrDispose(l);
		FskMemPtrDispose(tmpCstr);
		return NULL;
	}
	if ((l->fontRef = createFontWithAttributes(fref, textSize, textStyle)) == NULL) {
		LOGE("Can't create a run font with attr: name = \"%s\", size = %d, style = 0x%lx", GetCStringFromCFString(desc->name, &tmpCstr), textSize, textStyle);
		FskMemPtrDispose(tmpCstr);
		l->fontRef = fref;
	}
	else
		CFRelease(fref);
	l->fontSize = textSize;
	l->fontStyle = textStyle;
	l->next = desc->list;
	desc->list = l;
	return l->fontRef;
}

static CGColorRef
createCGColor(FskConstColorRGBA fskColorRGB, UInt32 blendLevel)
{
	CGColorRef colorRef = NULL;

	if (fskColorRGB != NULL) {
		CGColorSpaceRef rgbColorSpace;

		rgbColorSpace = CGColorSpaceCreateDeviceRGB();
		if (rgbColorSpace != NULL) {
			CGFloat fontColor[4];

			fontColor[0] = (float)fskColorRGB->r / 255;
			fontColor[1] = (float)fskColorRGB->g / 255;
			fontColor[2] = (float)fskColorRGB->b / 255;
			fontColor[3] = (float)blendLevel / 255;
			colorRef = CGColorCreate(rgbColorSpace, fontColor);
			CGColorSpaceRelease(rgbColorSpace);
		}
	}
	return colorRef;
}

static NSMutableAttributedString *
createAttributedString(const char *text, UInt32 textLength, FskConstColorRGBA fskColorRGB, UInt32 blendLevel, UInt32 textSize, UInt32 textStyle, const char *fontName, FskTextFormatCache cache)
{
	NSString *textString;
	NSMutableAttributedString *attrString;
	NSRange range;
	CTFontRef ctFontRef = NULL;
	CTTextAlignment ctTextAlignment;
	CTParagraphStyleSetting ctParagraphStyleSetting;
	CTParagraphStyleRef ctParagraphStyleRef;
	NSUInteger textStringLength;
	SInt32 one = 1;

	if ((text == NULL) || (textLength == 0)) return nil;

	ctFontRef = cache->fontRef;

	textString = [[NSString alloc] initWithBytes:text length:textLength encoding:NSUTF8StringEncoding];
	if (textString == nil)
	{
		return nil;
	}
	textStringLength = [textString length];
	range = NSMakeRange(0, textStringLength);

	/* attributed string */
	attrString = [[NSMutableAttributedString alloc] initWithString:textString];
	[textString release];

	[attrString addAttribute:(NSString*)kCTFontAttributeName value:(id)ctFontRef range:range];

	/* underline */
#if DEBUG_TEXT_STYLE
	textStyle |= kFskTextUnderline;
	textStyle |= kFskTextStrike;
#endif /* DEBUG_TEXT_STYLE */
	if ((textStyle & kFskTextUnderline) != 0)
	{
		CFNumberRef numberOne = CFNumberCreate(NULL, kCFNumberSInt32Type, &one);
		CFAttributedStringSetAttribute((CFMutableAttributedStringRef)attrString, CFRangeMake(0, textStringLength), kCTUnderlineStyleAttributeName, numberOne);
	}
	/* strike */
	if ((textStyle & kFskTextStrike) != 0 && cache)
	{
		cache->strikeThrough = true;
	}
	/* color */
	if (fskColorRGB != NULL)
	{
		CGColorRef colorRef = createCGColor(fskColorRGB, blendLevel);

		[attrString addAttribute:(NSString*)kCTForegroundColorAttributeName value:(id)colorRef range:range];
		CGColorRelease(colorRef);
	}

#if 1
	ctTextAlignment = kCTLeftTextAlignment;
#else // 0
	/* horizontal alignment */
	switch (horizontalAlignment)
	{
		case kFskTextAlignCenter:
			ctTextAlignment = kCTCenterTextAlignment;
			break;
		case kFskTextAlignRight:
			ctTextAlignment = kCTRightTextAlignment;
			break;
		case kFskTextAlignLeft:
		default:
			ctTextAlignment = kCTLeftTextAlignment;
			break;
	}
#endif // 0

	ctParagraphStyleSetting.spec = kCTParagraphStyleSpecifierAlignment;
	ctParagraphStyleSetting.valueSize = sizeof(CTTextAlignment);
	ctParagraphStyleSetting.value = &ctTextAlignment;

	ctParagraphStyleRef = CTParagraphStyleCreate(&ctParagraphStyleSetting, 1);
	if (ctParagraphStyleRef == NULL)
	{
		[attrString release];
		return nil;
	}
	[attrString addAttribute:(NSString*)kCTParagraphStyleAttributeName value:(id)ctParagraphStyleRef range:range];
	CFRelease(ctParagraphStyleRef);

	return attrString;
}

static Boolean
makeGlyphLayout(NSString *textString, CTLineRef line, CFIndex lineLength, LayoutInfo layout, FskTextFormatCache cache)
{
#if IGNORE_TRAILING_SPACES
	CFIndex *indices = NULL;
#endif /* IGNORE_TRAILING_SPACES */
	CGPoint *positions = NULL;
	CGGlyph *glyphs = NULL;
	CGSize adv;
	CFIndex lineCount, lineIndex = 0;
	Boolean err = true;

	lineCount = CTLineGetGlyphCount(line);
	if ((layout->codes = (UInt16 *)FskMemPtrAlloc(sizeof(UInt16) * (UInt32)lineCount)) == NULL)
		goto bail;
	if ((layout->layout = (FskFixed *)FskMemPtrAlloc(sizeof(FskFixed) * (UInt32)lineCount)) == NULL)
		goto bail;
	CFArrayRef runs = CTLineGetGlyphRuns(line);
	CFIndex runCount = CFArrayGetCount(runs);
	for (CFIndex j = 0, i = 0; j < runCount; j++) {
		CTRunRef run = (CTRunRef)CFArrayGetValueAtIndex(runs, j);
		CFIndex glyphCount = CTRunGetGlyphCount(run);
		if ((positions = (CGPoint *)FskMemPtrAlloc(sizeof(CGPoint) * (UInt32)glyphCount)) == NULL)
			goto bail;
		CTRunGetPositions(run, CFRangeMake(0, 0), positions);
		CTRunGetAdvances(run, CFRangeMake(glyphCount - 1, 1), &adv);
		layout->width = positions[glyphCount - 1].x + adv.width;	// take the last run
#if IGNORE_TAILING_SPACES
		if ((indicies = (CFIndex *)FskMemPtrAlloc(sizeof(CFIndex) * glyphCount)) == NULL)
			goto bail;
		CTRunGetStringIndices(run, CFRangeMake(0, 0), indices);
#endif /* IGNORE_TRAILING_SPACES */
		if ((glyphs = (CGGlyph *)FskMemPtrAlloc(sizeof(CGGlyph) * (UInt32)glyphCount)) == NULL)
			goto bail;
		CTRunGetGlyphs(run, CFRangeMake(0, 0), glyphs);
		CTFontRef runFont = CFDictionaryGetValue(CTRunGetAttributes(run), kCTFontAttributeName);
		int d = registerRunFont(runFont, cache);
		for (int k = 0; k < glyphCount; k++) {
			layout->codes[i] = encodeGlyphCode(d, glyphs[k], cache);
			layout->layout[i++] = FskRoundFloatToFixed(positions[k].x);
#if IGNORE_TRAILING_SPACES
			unichar c = [textString characterAtIndex:indices[k]];
			if (!FskStrIsSpace(c))
				lineIndex = i;
#else /* !IGNORE_TRAILING_SPACES */
			lineIndex = i;
#endif /* !IGNORE_TRAILING_SPACES */
		}
		FskMemPtrDispose(glyphs); glyphs = NULL;
		FskMemPtrDispose(positions); positions = NULL;
#if IGNORE_TAILING_SPACES
		FskmemPtrDispose(indicies); indicies = NULL;
#endif /* IGNORE_TRAILING_SPACES */
	}
	layout->codesLen = lineIndex == 0 ? 1 : (UInt32)lineIndex;
	err = false;

bail:
	if (glyphs) FskMemPtrDispose(glyphs);
	if (positions) FskMemPtrDispose(positions);
#if IGNORE_TRAILING_SPACES
	if (indices) FskMemPtrDispose(indices);
#endif /* IGNORE_TRAILING_SPACES */
	return err;
}

static Boolean
makeTextLayout(NSString *textString, CTLineRef line, CFIndex lineLength, LayoutInfo layout, FskTextFormatCache cache)
{
	if ((NSUInteger)lineLength > textString.length) return true;

	if (FskMemPtrNewClear(sizeof(UInt16) * (UInt32)lineLength, (FskMemPtr*)&layout->codes) != kFskErrNone)
		return true;
	if (FskMemPtrNewClear(sizeof(FskFixed) * (UInt32)lineLength, (FskMemPtr*)&layout->layout) != kFskErrNone)
		return true;

#if IGNORE_TRAILING_SPACES
	for (;lineLength > 1; lineLength--) {
		unichar c = [textString characterAtIndex:lineLength - 1];
		if (!FskStrIsSpace(c))
			break;
	}
#endif /* IGNORE_TRAILING_SPACES */
	layout->codesLen = (UInt32)lineLength;

	for (CFIndex i = 0; i < lineLength; i++) {
		CGFloat offset = CTLineGetOffsetForStringIndex(line, i, NULL);
		layout->layout[i] = FskRoundFloatToFixed(offset);
		((UInt16 *)layout->codes)[i] = (UInt16)[textString characterAtIndex:i];
	}
	return false;
}

static void
makeLayoutInfo(NSString *textString, CTLineRef line, CFIndex lineLength, LayoutInfo layout, FskTextFormatCache cache)
{
	Boolean err = true;

	layout->codesLen = 0;
	layout->codes = NULL;
	layout->layout = NULL;

	if (layout->useGlyph)
		err = makeGlyphLayout(textString, line, lineLength, layout, cache);
	else
		err = makeTextLayout(textString, line, lineLength, layout, cache);

	if (err) {
		if (layout->codes) FskMemPtrDisposeAt((FskMemPtr *)&layout->codes);
		if (layout->layout) FskMemPtrDisposeAt((FskMemPtr *)&layout->layout);
		layout->codesLen = 0;
	}
}

static void
makeBytesOut(NSString *textString, CFIndex lineLength, BytesInfo bytes)
{
	NSString *subString;

	subString = [textString substringToIndex:lineLength];
	bytes->bytesLength = (UInt32)[subString lengthOfBytesUsingEncoding:NSUTF8StringEncoding];
	bytes->charsLength = (UInt32)lineLength;
}

static BOOL
sameColorRGB(FskConstColorRGBA rgb1, FskConstColorRGBA rgb2)
{
	if (rgb1 == NULL)
		return (rgb2 == NULL) ? YES : NO;
	if (rgb2 == NULL)
		return NO;
	if ((rgb1->r != rgb2->r) || (rgb1->g != rgb2->g) || (rgb1->b != rgb2->b))
		return NO;
	return YES;
}

static BOOL
isOneChar(const char *text, UInt32 textLength)
{
	if (textLength == 0)
		return NO;	// ??
	unsigned char c = (unsigned char)*text;
	return (((c & ~0x7f) == 0 && textLength == 1) ||
		((c & ~0x1f) == 0xc0 && textLength == 2) ||
		((c & ~0x0f) == 0xe0 && textLength == 3) ||
		((c & ~0x07) == 0xf0 && textLength == 4) ||
		((c & ~0x03) == 0xf8 && textLength == 5) ||
		((c & ~0x01) == 0xfc && textLength == 6));
}

static int
lineHash(const char *text, int textLength)
{
	unsigned int v = 0;

	while (--textLength >= 0)
		v = v * 2 + *text++;
	return v % LINEHASHSIZE;
}

static LineList
lineLookup(FskTextEngineState state, const char *text, UInt32 textLength, FskConstColorRGBA fskColorRGB, UInt32 blendLevel, UInt32 textSize, UInt32 textStyle, const char *fontName, CGFloat constraintWidth)
{
	int v = lineHash(text, textLength);

	for (LineList ll = state->lineHashTable[v]; ll != NULL; ll = ll->hnext) {
		if (textLength == ll->textLength &&
		    textSize == ll->textSize &&
		    textStyle == ll->textStyle &&
		    constraintWidth == ll->constraintWidth &&
		    blendLevel == ll->blendLevel &&
		    sameColorRGB(fskColorRGB, &ll->colorRGB) &&
		    FskStrCompareWithLength(text, ll->text, textLength) == 0 &&
		    FskStrCompare(fontName, ll->fontName) == 0)
			return ll;
	}
	return NULL;
}

static void
lineRegister(FskTextEngineState state, LineList ll)
{
	int v = lineHash(ll->text, ll->textLength);

	ll->hnext = state->lineHashTable[v];
	state->lineHashTable[v] = ll;
}

static void
lineDeregister(FskTextEngineState state, LineList ll)
{
	int v = lineHash(ll->text, ll->textLength);

	for (LineList *lp = &state->lineHashTable[v]; *lp != NULL; lp = &(*lp)->hnext) {
		if (*lp == ll) {
			*lp = ll->hnext;
			break;
		}
	}
}

static BOOL
createLine_(void *stateIn, const char *text, UInt32 textLength, FskConstColorRGBA fskColorRGB, UInt32 blendLevel, UInt32 textSize, UInt32 textStyle, const char *fontName, FskTextFormatCache cache, CGFloat constraintWidth, CTLineRef *lineOut, SizeInfo sizeOut, BytesInfo bytesOut, LayoutInfo layoutOut, BOOL *cacheHit)
{
	FskTextEngineState state = stateIn;
	LineList ll = NULL;
	FskColorRGBARecord color;
	NSAttributedString *attrString = nil;
	FskTextFormatCache tmpCache = NULL;
	FskErr err = kFskErrUnknown;
	BOOL ret = NO;

	if (cache == NULL) {
		if (!FskCocoaTextFormatCacheNew(state, &tmpCache, NULL, textSize, textStyle, fontName))
			goto bail;
		cache = tmpCache;
	}
	if (fskColorRGB == NULL) {
		FskColorRGBASet(&color, 255, 255, 255, 255);
		fskColorRGB = &color;
		if (blendLevel == 0)
			blendLevel = 255;
	}
	if ((ll = lineLookup(state, text, textLength, fskColorRGB, blendLevel, textSize, textStyle, fontName, constraintWidth)) == NULL) {
		CTLineRef line;
		CFIndex lineLength;
		if ((attrString = createAttributedString(text, textLength, fskColorRGB, blendLevel, textSize, textStyle, fontName, cache)) == nil)
			goto bail;
		if (isOneChar(text, textLength)) {
			// optimize a little bit
			CFDictionaryRef attrDic = CFAttributedStringGetAttributes((CFAttributedStringRef)attrString, 0, NULL);
			CFStringRef tokenString = CFStringCreateWithBytes(NULL, (const UInt8 *)text, textLength, kCFStringEncodingUTF8, false);
			CFAttributedStringRef attrTokenString = CFAttributedStringCreate(NULL, tokenString, attrDic);
			line = CTLineCreateWithAttributedString(attrTokenString);
			CFRelease(tokenString);
			CFRelease(attrTokenString);
			lineLength = attrString.length;
		}
		else {
			UInt32 lineTruncation = textStyle & (kFskTextTruncateCenter | kFskTextTruncateEnd);
			CTTypesetterRef typesetter = CTTypesetterCreateWithAttributedString((CFAttributedStringRef)attrString);
			if (typesetter == NULL)
				goto bail;
			lineLength = CTTypesetterSuggestLineBreak(typesetter, 0, constraintWidth);
			if ((NSUInteger)lineLength < attrString.length && lineTruncation != 0) {
				UniChar token = 0x2026;
				CFStringRef tokenString = CFStringCreateWithCharacters(NULL, &token, 1);
				if (tokenString == NULL)
					goto bail;
				CFDictionaryRef attrDic = CFAttributedStringGetAttributes((CFAttributedStringRef)attrString, 0, NULL);
				CFAttributedStringRef attrTokenString = CFAttributedStringCreate(NULL, tokenString, attrDic);
				CFRelease(tokenString);
				if (attrTokenString == NULL)
					goto bail;
				CTLineRef truncationToken = CTLineCreateWithAttributedString(attrTokenString);
				CFRelease(attrTokenString);
				if (truncationToken == NULL)
					goto bail;
				if ((line = CTTypesetterCreateLine(typesetter, CFRangeMake(0, attrString.length))) == NULL)
					goto bail;
				CTLineTruncationType truncationType = lineTruncation & kFskTextTruncateCenter ? kCTLineTruncationMiddle : kCTLineTruncationEnd;
				CTLineRef truncatedLine = CTLineCreateTruncatedLine(line, constraintWidth, truncationType, truncationToken);
				CFRelease(line);
				CFRelease(truncationToken);
				line = truncatedLine;
				/* seems like there're some cases where lineLength < truncatedLine.length. */
			}
			else {
				line = CTTypesetterCreateLine(typesetter, CFRangeMake(0, lineLength));
			}
			CFRelease(typesetter);
		}
		if (line == NULL)
			goto bail;

		// make LineList
		if (state->lineListCount >= MAXLINELIST) {
			ll = (LineList)FskListDoubleRemoveLast(&state->lineList);
			lineDeregister(state, ll);
			freeLineList_(ll);	// reuse the earliest one
			FskMemSet(ll, 0, sizeof(LineListRecord));
			--state->lineListCount;
		}
		else {
			if ((err = FskMemPtrNewClear(sizeof(LineListRecord), (FskMemPtr *)&ll)) != kFskErrNone)
				goto bail;
		}
		if ((err = FskMemPtrNewFromData(textLength, text, (FskMemPtr *)&ll->text)) != kFskErrNone)
			goto bail;
		ll->textLength = textLength;
		ll->colorRGB = *fskColorRGB;
		ll->blendLevel = blendLevel;
		ll->textSize = textSize;
		ll->textStyle = textStyle;
		if ((ll->fontName = FskStrDoCopy(fontName)) == NULL) {
//			err = kFskErrMemFull;
			goto bail;
		}
		ll->constraintWidth = constraintWidth;
		ll->lineLength = (UInt32)lineLength;
		ll->line = line;
		ll->textString = [attrString.string retain];
		[attrString release];
		attrString = nil;
		ll->layout.useGlyph = true;
		makeLayoutInfo(ll->textString, ll->line, ll->lineLength, &ll->layout, cache);
		ll->size.width = ll->layout.width;
		ll->size.ascent = cache->ascent;
		ll->size.descent = cache->descent;
		ll->size.height = ll->size.ascent + ll->size.descent;
		if (textStyle & kFskTextItalic) /* Is there a way to determine whether the typeface has been obliqued rather than designed as italic? */
			ll->size.width += ll->size.ascent * 0.375f; /* If obliqued, the width is larger by some fraction of the height (can we use CGFloat CGFontGetItalicAngle(CGFontRef font)?) */

		state->lineListCount++;
		FskListDoublePrepend(&state->lineList, (FskListDoubleElement)ll);
		lineRegister(state, ll);
		*cacheHit = NO;
	}
	else
		*cacheHit = YES;

	if (bytesOut != NULL) {
		if (ll->bytes.bytesLength == 0)
			makeBytesOut(ll->textString, ll->lineLength, &ll->bytes);
		*bytesOut = ll->bytes;
	}
	if (layoutOut != NULL) {
		if (ll->layout.codesLen == 0 || layoutOut->useGlyph != ll->layout.useGlyph) {
			ll->layout.useGlyph = layoutOut->useGlyph;
			if (ll->layout.codes) FskMemPtrDisposeAt((FskMemPtr *)&ll->layout.codes);
			if (ll->layout.layout) FskMemPtrDisposeAt((FskMemPtr *)&ll->layout.layout);
			makeLayoutInfo(ll->textString, ll->line, ll->lineLength, &ll->layout, cache);
		}
		if (ll->layout.codesLen > 0) {
			if ((err = FskMemPtrNewFromData(sizeof(UInt16) * ll->layout.codesLen, ll->layout.codes, (FskMemPtr *)&layoutOut->codes)) != kFskErrNone)
				goto bail;
			if ((err = FskMemPtrNewFromData(sizeof(FskFixed) * ll->layout.codesLen, ll->layout.layout, (FskMemPtr *)&layoutOut->layout)) != kFskErrNone) {
				FskMemPtrDispose(layoutOut->codes);
				goto bail;
			}
		}
		layoutOut->codesLen = ll->layout.codesLen;
	}
	if (sizeOut != NULL)
		*sizeOut = ll->size;
	if (lineOut != NULL)
		// *lineOut = CFRetain(ll->line);	// @@ redundant
		*lineOut = ll->line;

	ll = NULL;
	ret = YES;

bail:
	if (attrString != nil)
		[attrString release];
	if (ll != NULL)
		freeLineList(ll);
	if (tmpCache != NULL)
		FskCocoaTextFormatCacheDispose(state, tmpCache);

	return ret;
}

static BOOL
createLine(void *stateIn, const char *text, UInt32 textLength, FskConstColorRGBA fskColorRGB, UInt32 blendLevel, UInt32 textSize, UInt32 textStyle, const char *fontName, FskTextFormatCache cache, CGFloat constraintWidth, CTLineRef *lineOut, SizeInfo sizeOut, BytesInfo bytesOut, LayoutInfo layoutOut)
{
	BOOL ret, cacheHit;
#if LOG_LINE_CACHE
	int i;
	struct timeval t1, t2;
	double d1, d2;

	gettimeofday(&t1, NULL);
#endif /* LOG_LINE_CACHE */
	ret = createLine_(stateIn, text, textLength, fskColorRGB, blendLevel, textSize, textStyle, fontName, cache, constraintWidth, lineOut, sizeOut, bytesOut, layoutOut, &cacheHit);
#if LOG_LINE_CACHE
	gettimeofday(&t2, NULL);
	d1 = t1.tv_usec + t1.tv_sec * 1000 * 1000;
	d2 = t2.tv_usec + t2.tv_sec * 1000 * 1000;

	fprintf(stderr, "\tcache %s t=%f, \"", cacheHit ? "hit" : "not hit", d2 - d1);
	for (i = 0; i < textLength; i++) {
		if (isprint(text[i]))
			fputc(text[i], stderr);
		else
			fprintf(stderr, "\\%02x", ((unsigned char *)text)[i]);
	}
	fprintf(stderr, "\"\n");
#endif /* LOG_LINE_CACHE */
	return ret;
}

#if TARGET_OS_IPHONE
void PurgeLineCache(void *refcon)
{
	FskTextEngineStateRecord *state = refcon;

	// purge 1/4 of the line cache for each call
	int n = state->lineListCount / 4;
	while (--n >= 0) {
		LineList ll = (LineList)FskListDoubleRemoveLast(&state->lineList);
		lineDeregister(state, ll);
		freeLineList(ll);
		--state->lineListCount;
	}
}
#endif	/* TARGET_OS_IPHONE */


/*
 * FskCocoaText APIs
 */
void FskCocoaTextInitialize()
{
	gTextState.lineListCount = 0;
	FskMemSet(gTextState.lineHashTable, 0, sizeof(gTextState.lineHashTable));
	gTextState.fontList = NULL;
#if TARGET_OS_IPHONE
	FskNotificationRegister(kFskNotificationLowMemory, PurgeLineCache, &gTextState);
#endif /* TARGET_OS_IPHONE */
}

void FskCocoaTextUninitialize()
{
#if TARGET_OS_IPHONE
	FskNotificationUnregister(kFskNotificationLowMemory, PurgeLineCache, &gTextState);
#endif /* TARGET_OS_IPHONE */
	LineList lineList;
	while ((lineList = (LineList)FskListDoubleRemoveFirst(&gTextState.lineList)) != NULL) {
		freeLineList(lineList);
	}
	FontList fl, next;
	for (fl = gTextState.fontList; fl != NULL; fl = next) {
		CFRelease(fl->name);
		CFRelease(fl->fontRef);
		for (int i = 0; i < fl->numRunFonts; i++) {
			struct runFontDescriptor *desc = &fl->runFonts[i];
			CFRelease(desc->name);
			if (desc->glyphtab != NULL)
				FskMemPtrDispose(desc->glyphtab);
			for (struct runFontList *l = desc->list, *next = NULL; l != NULL; l = next) {
				CFRelease(l->fontRef);
				next = l->next;
				FskMemPtrDispose(l);
			}
		}
		next = fl->next;
		FskMemPtrDispose(fl);
	}
}

FskErr FskCocoaTextNew(void **state)
{
	*state = &gTextState;
	return kFskErrNone;
}

FskErr FskCocoaTextDispose(void *stateIn)
{
	return kFskErrNone;
}

Boolean FskCocoaTextFormatCacheNew(void *state, FskTextFormatCache *cacheOut, FskBitmap bits, UInt32 textSize, UInt32 textStyle, const char *fontName)
{
	FskTextFormatCache cache;
	CTFontRef fref;

	if ((cache = (FskTextFormatCache)FskMemPtrCalloc(sizeof(FskTextFormatCacheRecord))) == NULL)
		goto bail;
	if ((cache->fontInfo = registerBaseFont(state, (CFStringRef)[NSString stringWithUTF8String:fontName], textSize)) == NULL)
		goto bail;
	if ((fref = createFontWithAttributes(cache->fontInfo->fontRef, textSize, textStyle)) == NULL)
		goto bail;
	cache->textSize = textSize;
	cache->ascent = CTFontGetAscent(fref);
	cache->descent = fabs(CTFontGetDescent(fref));
	cache->leading = CTFontGetLeading(fref);
	CGRect r = CTFontGetBoundingBox(fref);
	cache->width = r.size.width;
	cache->fontRef = fref;
	*cacheOut = cache;
	return true;

bail:
	FskCocoaTextFormatCacheDispose(state, cache);
	*cacheOut = NULL;
	return false;
}

void FskCocoaTextFormatCacheDispose(void *state, FskTextFormatCache cache)
{
	if (cache != NULL) {
		if (cache->fontRef != NULL)
			CFRelease(cache->fontRef);
		// fontInfo should not be disposed here
		FskMemPtrDispose(cache);
	}
}

void FskCocoaTextGetFontList(void *state, char **fontList)
{
	CFArrayRef	famArray = NULL;
	CFIndex		famCount, famStrLen, i, listLength;
	CFRange		range;
	CFStringRef	famName;
	FskErr		err;
	UInt8		*s;

	if (fontList == NULL) return;

	*fontList = NULL;
	#if !TARGET_OS_IPHONE
		famArray = CTFontManagerCopyAvailableFontFamilyNames();
	#else /* TARGET_OS_IPHONE */
		famArray = (CFArrayRef)[[UIFont familyNames] retain];
	#endif /* TARGET_OS_IPHONE */
	famCount = CFArrayGetCount(famArray);
	range.location = 0;

	for (i = 0, listLength = 1; i < famCount; ++i) {													/* Count the number of needed characters for the whole list */
		famName = CFArrayGetValueAtIndex(famArray, i);
		range.length = CFStringGetLength(famName);
		(void)CFStringGetBytes(famName, range, kCFStringEncodingUTF8, 0, false, NULL, 0, &famStrLen);	/* Ask how many bytes are needed for each family */
		listLength += famStrLen + 1;																	/* Tally them up, with an extra 0 terminator */
	}

	if (listLength < 2) listLength = 2;
	BAIL_IF_ERR(err = FskMemPtrNew(listLength, fontList));												/* Allocate the font list */
	s = (UInt8*)(*fontList);
	s[0] = 0; s[1] = 0;																					/* In case there are no fonts */

	for (i = 0; i < famCount; ++i) {
		famName = CFArrayGetValueAtIndex(famArray, i);
		range.length = CFStringGetLength(famName);
		(void)CFStringGetBytes(famName, range, kCFStringEncodingUTF8, 0, false, s, listLength, &famStrLen);	/* Copy font family name into list */
		s += famStrLen;																					/* Advance list pointer */
		*s++ = 0;																						/* Append terminating 0 for font name */
		listLength -= famStrLen + 1;
	}
	*s = 0;																								/* Append terminating 0 for font family list */
bail:
	CFRelease(famArray);

	return;
}

void FskCocoaTextGetBounds(void *state, const char *text, UInt32 textLength, UInt32 textSize, UInt32 textStyle, const char *fontName, FskRectangle bounds, FskTextFormatCache cache)
{
	SizeInfoRecord size;

	size.width = 0.0;
	size.height = 0.0;

	//NSLog(@"FskCocoaTextGetBounds: %s", debug_text(text, textLength));

	if ((text == NULL) || (textLength == 0)) goto bail;

	//	fprintf(stderr, "FskCocoaTextGetBounds:\n");
	createLine(state, text, textLength, NULL, 0, textSize, textStyle, fontName, cache, CGFLOAT_MAX, NULL, &size, NULL, NULL);

bail:
	FskRectangleSet(bounds, 0, 0, ceilf(size.width), roundf(size.height));
}

void FskCocoaTextGetBoundsSubpixel(void *state, const char *text, UInt32 textLength, UInt32 textSize, UInt32 textStyle, const char *fontName, FskDimensionFloat dimension, FskTextFormatCache cache)
{
	SizeInfoRecord size;

	size.width = 0.0;
	size.height = 0.0;

	if (text && textLength)
		createLine(state, text, textLength, NULL, 0, textSize, textStyle, fontName, cache, CGFLOAT_MAX, NULL, &size, NULL, NULL);

	dimension->width = size.width;
	dimension->height = size.height;
}

static void MyTextGetLayout(Boolean useGlyph, void *state, const char *text, UInt32 textLength, UInt32 textSize, UInt32 textStyle, const char *fontName, UInt16 **codePtr, UInt32 *codeLenPtr, FskFixed **layoutPtr, float *widthPtr, float *heightPtr, FskTextFormatCache cache)
{
	LayoutInfoRecord layout;
	SizeInfoRecord size;

	layout.useGlyph = useGlyph;
	layout.codesLen = 0;
	layout.codes	= NULL;
	layout.layout	= NULL;

	//NSLog(@"FskCocoaTextGetLayout: %s", debug_text(text, textLength));
	if ((text == NULL) || (textLength == 0)) goto bail;

	createLine(state, text, textLength, NULL, 0, textSize, textStyle, fontName, cache, CGFLOAT_MAX, NULL, &size, NULL, &layout);

	if (widthPtr != NULL)
		*widthPtr = size.width;
	if (heightPtr != NULL)
		*heightPtr = size.height;

bail:
	if (codeLenPtr != NULL)
		*codeLenPtr = layout.codesLen;
	if (codePtr != NULL)
		*codePtr = layout.codes;
	else
		FskMemPtrDispose((FskMemPtr)layout.codes);	/* Dispose of  codes  if not wanted */
	if (layoutPtr != NULL)
		*layoutPtr = layout.layout;
	else
		FskMemPtrDispose((FskMemPtr)layout.layout);	/* Dispose of laypout if not wanted */
}

void FskCocoaTextGetLayout(void *state, const char *text, UInt32 textLength, UInt32 textSize, UInt32 textStyle, const char *fontName, UInt16 **unicodeTextPtr, UInt32 *unicodeLenPtr, FskFixed **layoutPtr, FskTextFormatCache cache)
{
	FskTextFormatCache tmpCache = NULL;

	if (cache == NULL) {
		if (!FskCocoaTextFormatCacheNew(state, &cache, NULL, textSize, textStyle, fontName))
			return;
		tmpCache = cache;
	}

	MyTextGetLayout(false, state, text, textLength, textSize, textStyle, fontName, unicodeTextPtr, unicodeLenPtr, layoutPtr, NULL, NULL, cache);

	if (tmpCache != NULL)
		FskCocoaTextFormatCacheDispose(state, tmpCache);
}

void FskCocoaTextGetGlyphs(void *state, const char *text, UInt32 textLength, UInt32 textSize, UInt32 textStyle, const char *fontName, UInt16 **glyphsPtr, UInt32 *glyphsLenPtr, FskFixed **layoutPtr, float *widthPtr, float *heightPtr, FskTextFormatCache cache)
{
	MyTextGetLayout(true, state, text, textLength, textSize, textStyle, fontName, glyphsPtr, glyphsLenPtr, layoutPtr, widthPtr, heightPtr, cache);
}

Boolean FskCocoaTextFitWidth(void *state, const char *text, UInt32 textLength, UInt32 textSize, UInt32 textStyle, const char *fontName, UInt32 width, UInt32 flags, UInt32 *fitBytesOut, UInt32 *fitCharsOut, FskTextFormatCache cache)
{
	BytesInfoRecored bytes;
	Boolean ret = false;

	bytes.bytesLength = 0;
	bytes.charsLength = 0;

	if ((text == NULL) || (textLength == 0)) goto bail;

	//NSLog(@"FskCocoaTextFitWidth: %s", debug_text(text, textLength));

	//	fprintf(stderr, "FskCocoaTextFitWidth:\n");
	ret = createLine(state, text, textLength, NULL, 0, textSize, textStyle, fontName, cache, width, NULL, NULL, &bytes, NULL);

bail:
	if (fitBytesOut)
	{
		*fitBytesOut = bytes.bytesLength;
	}

	if (fitCharsOut)
	{
		*fitCharsOut = bytes.charsLength;
	}
	return ret;
}

void FskCocoaTextGetFontInfo(void *state, FskTextFontInfo info, const char *fontName, UInt32 textSize, UInt32 textStyle, FskTextFormatCache cache)
{
	FskTextFormatCache tmpCache = NULL;

	if (cache == NULL) {
		if (!FskCocoaTextFormatCacheNew(state, &cache, NULL, textSize, textStyle, fontName)) {
			info->ascent = info->descent = info->leading = info->width = info->height = 0;
			return;
		}
		tmpCache = cache;
	}
	info->ascent = (UInt32)ceilf(cache->ascent);
	info->descent = (UInt32)ceilf(cache->descent);
	info->leading = (UInt32)ceilf(cache->leading);
	info->width = (UInt32)ceilf(cache->width);
	info->height = (UInt32)ceilf(cache->ascent + cache->descent + cache->leading);
	info->glyphHeight = (UInt32)ceilf(cache->ascent + cache->descent);
	if (tmpCache != NULL)
		FskCocoaTextFormatCacheDispose(state, tmpCache);
}

Boolean FskCocoaTextDraw(void *state, FskBitmap fskBitmap, const char *text, UInt32 textLength, FskConstRectangle fskRect, FskConstRectangle clipFskRect, FskConstColorRGBA fskColorRGB, UInt32 blendLevel, UInt32 textSize, UInt32 textStyle, UInt16 horizontalAlignment, UInt16 verticalAlignment, const char *fontName, FskTextFormatCache cache)
{
	Boolean ret = false;
	CGContextRef context;
	CTLineRef line = NULL;
	CGRect drawRect;
	CGPoint position;
	SizeInfoRecord size;
	double penOffset;
	Boolean strikeThrough;
	FskTextFormatCache tmpCache = NULL;

	if ((fskBitmap == NULL) || (text == NULL) || (textLength == 0) || (fskRect == NULL)) return false;

	//NSLog(@"FskCocoaTextDraw: %s", debug_text(text, textLength));

	context = (CGContextRef)(fskBitmap->cgBitmapContext);
	if (NULL == context) {									/* If the bitmap is not an Apple-supported pixel type */
		FskBitmap tmp;
		FskColorRGBARecord color;
		FskGraphicsModeParametersRecord modeParams;

		/* TODO: Allocate a smaller bitmap and/or render to a smaller rect */
		if ((kFskErrNone != FskBitmapNew(fskBitmap->bounds.width, fskBitmap->bounds.height, kFskBitmapFormat8G, &tmp)) || NULL == tmp->cgBitmapContext)
			return false;
		FskColorRGBASet(&color, 0, 0, 0, 0);
		FskRectangleFill(tmp, fskRect, &color, kFskGraphicsModeCopy, NULL);
		FskColorRGBASet(&color, 255, 255, 255, 255);
		modeParams.dataSize = sizeof(modeParams);
		modeParams.blendLevel = blendLevel;
		strikeThrough = FskCocoaTextDraw(state, tmp, text, textLength, fskRect, clipFskRect, &color, 255, textSize, textStyle, horizontalAlignment, verticalAlignment, fontName, cache)
		&& (kFskErrNone == FskTransferAlphaBitmap(tmp, fskRect, fskBitmap, (FskConstPoint)fskRect, clipFskRect, fskColorRGB, &modeParams));
		FskBitmapDispose(tmp);
		return strikeThrough;
	}

	if (cache == NULL) {
		/* just for GLTest */
		if (!FskCocoaTextFormatCacheNew(state, &cache, NULL, textSize, textStyle, fontName))
			return false;
		tmpCache = cache;
	}

	drawRect = CGRectMake(fskRect->x, fskBitmap->bounds.height - fskRect->y - fskRect->height, fskRect->width, fskRect->height);

	//	fprintf(stderr, "FskCocoaTextDraw:\n");
	if (!(ret = createLine(state, text, textLength, fskColorRGB, blendLevel, textSize, textStyle, fontName, cache, fskRect->width, &line, &size, NULL, NULL)))
		goto bail;

#if 0	/* to align with GLText */
	CGFloat flush;
	switch (horizontalAlignment)
	{
		case kFskTextAlignCenter:
			flush = 0.5;
			break;
		case kFskTextAlignRight:
			flush = 1.0;
			break;
		case kFskTextAlignLeft:
		default:
			flush = 0.0;
			break;
	}

	penOffset = CTLineGetPenOffsetForFlush(line, flush, fskRect->width);
#else // 1
	switch (horizontalAlignment) {
	case kFskTextAlignCenter:
		penOffset = (fskRect->width - size.width) / 2;
		break;
	case kFskTextAlignRight:
		penOffset = fskRect->width - size.width;
		break;
	case kFskTextAlignLeft:
	default:
		penOffset = 0;
		break;
	}
#endif // 1

	position = drawRect.origin;
	position.x += penOffset;
	position.y += size.descent;

	switch (verticalAlignment)
	{
		case kFskTextAlignTop:
			position.y += fskRect->height - size.height;
			break;
		case kFskTextAlignBottom:
			break;
		case kFskTextAlignCenter:
		default:
			position.y += (fskRect->height - size.height) / 2;
			break;
	}

	position.x = roundf(position.x);	/* move to the nearest integer to align with GLText */

	strikeThrough = (cache != NULL) ? cache->strikeThrough : ((textStyle & kFskTextStrike) != 0);

	CGContextSaveGState(context);
	CGContextSetShouldAntialias(context, true);

	CGContextClipToRect(context, drawRect);
	if (clipFskRect)
		CGContextClipToRect(context, CGRectMake(clipFskRect->x, fskBitmap->bounds.height - clipFskRect->y - clipFskRect->height, clipFskRect->width, clipFskRect->height));

	CGContextSetTextPosition(context, position.x, position.y);
	CTLineDraw(line, context);

	if (strikeThrough)
	{
		CGFloat y = roundf(position.y - size.descent + size.height / 2);
		CGColorRef colorRef = createCGColor(fskColorRGB, blendLevel);

		CGContextBeginPath(context);
		CGContextSetLineWidth(context, 1.0);
		CGContextSetStrokeColorWithColor(context, colorRef);
		CGContextMoveToPoint(context, position.x, y);
		CGContextAddLineToPoint(context, position.x + size.width, y);
		CGContextStrokePath(context);
		CFRelease(colorRef);
	}

	CGContextRestoreGState(context);

#if DEBUG_TEXT_BOX
	{
		CGFloat color[4] = {0.0, 0.0, 0.0, 1.0};
		CGColorSpaceRef cs;
		CGColorRef strokeColor;

		switch (verticalAlignment)
		{
			case kFskTextAlignTop:
				color[0] = 1.0;
				break;
			case kFskTextAlignBottom:
				color[1] = 1.0;
				break;
			case kFskTextAlignCenter:
			default:
				color[2] = 1.0;
				break;
		}
		color[3] = 1.0;
		CGColorRef colorRef;

		cs = CGColorSpaceCreateDeviceRGB();
		strokeColor = CGColorCreate(cs, color);
		CGColorSpaceRelease(cs);
		CGContextSaveGState(context);
		CGContextSetLineWidth(context, 1.0);
		CGContextSetStrokeColorWithColor(context, strokeColor);
		CGColorRelease(strokeColor);
		CGContextStrokeRect(context, drawRect);
		CGContextRestoreGState(context);
	}
#endif /* DEBUG_TEXT_BOX */

bail:
	if (tmpCache != NULL)
		FskCocoaTextFormatCacheDispose(state, tmpCache);

	return ret;
}

FskErr FskTextGlyphGetBounds(FskTextEngine fte, FskBitmap bits, const UInt16 *codes, UInt32 codesLen, UInt32 textSize, UInt32 textStyle, const char *fontName, FskRectangle bounds, FskTextFormatCache cache)
{
	CGFloat x = 0, width = 0;

	if (cache == NULL)
		return kFskErrInvalidParameter;

#if !USE_FRACTIONAL_TEXT_SIZE
	textSize = ROUND_TEXTSIZE(textSize);
#endif

	FskRectangleSet(bounds, 0, 0, 0, 0);
	for (UInt32 i = 0; i < codesLen; i++) {
		CGGlyph glyph = decodeGlyphCode(codes[i], cache);
		CTFontRef runFont = getRunFont(fte->state, codes[i], textSize, textStyle, fontName, cache);
		if (runFont == NULL)
			continue;
		CGRect rect = CTFontGetBoundingRectsForGlyphs(runFont, kCTFontDefaultOrientation, &glyph, NULL, 1);
		// sanity check -- when the system causes an error like "FT_Load_Glyph failed: error 6", the rect seems to have (inf, inf, 0, 0)
		if (rect.origin.x == INFINITY || rect.origin.y == INFINITY)
			continue;
		if (i == 0) {
			CGFloat ascent = CTFontGetAscent(runFont);
			bounds->y = FskRoundFloatToFixed(ascent);
			bounds->x = FskRoundFloatToFixed(rect.origin.x);
		}
		width = x + rect.origin.x + rect.size.width;
		if (i + 1 < codesLen)
			x += CTFontGetAdvancesForGlyphs(runFont, kCTFontDefaultOrientation, &glyph, NULL, 1);
	}
	bounds->width = FskRoundFloatToFixed(width);
	bounds->height = FskRoundFloatToFixed(cache->ascent + cache->descent);
	return kFskErrNone;
}

FskErr FskTextGlyphBox(FskTextEngine fte, FskBitmap bits, const UInt16 *codes, UInt32 codesLen, FskConstRectangle r, FskConstRectangle clipRect, FskConstColorRGBA color, UInt32 blendLevel, UInt32 textSize, UInt32 textStyle, UInt16 hAlign, UInt16 vAlign, const char *fontName, FskTextFormatCache cache)
{
	CGContextRef context = bits->cgBitmapContext;
	BOOL strike = (textStyle & (kFskTextStrike | kFskTextUnderline)) != 0;
	CGFloat ascent, descent;
	CGPoint origin, point;

	if (cache == NULL)
		return kFskErrInvalidParameter;

#if !USE_FRACTIONAL_TEXT_SIZE
	textSize = ROUND_TEXTSIZE(textSize);
#endif

	ascent = cache->ascent;
	descent = cache->descent;
	origin.x = r->x;
	origin.y = bits->bounds.height - r->y;

	switch (hAlign) {
		case kFskTextAlignCenter:
		case kFskTextAlignRight: {
			FskRectangleRecord bounds;
			FskTextGlyphGetBounds(fte, bits, codes, codesLen, textSize, textStyle, fontName, &bounds, cache);
			CGFloat width = ceilf(FskRoundFloatToFixed(bounds.width));
			origin.x += hAlign == kFskTextAlignCenter ? (r->width - width) / 2 : r->width - width;
			break;
		}
		case kFskTextAlignLeft:
		default:
			break;
	}

	// adjust origin.y with the font ascent later
	switch (vAlign) {
		case kFskTextAlignTop:
			break;
		case kFskTextAlignBottom:
			origin.y -= r->height - (ascent + descent);
			break;
		case kFskTextAlignCenter:
		default:
			origin.y -= r->height - ((ascent + descent) / 2);
			break;
	}

	CGContextSaveGState(context);
	CGContextSetShouldAntialias(context, true);
	if (color != NULL)
		CGContextSetRGBFillColor(context, (CGFloat)color->r / 255, (CGFloat)color->g / 255, (CGFloat)color->b / 255, (CGFloat)blendLevel / 255);
	if (clipRect)
		CGContextClipToRect(context, CGRectMake(clipRect->x, bits->bounds.height - clipRect->y - clipRect->height, clipRect->width, clipRect->height));
	for (UInt32 i = 0; i < codesLen; i++) {
		CGGlyph glyph = decodeGlyphCode(codes[i], cache);
		CTFontRef runFont = getRunFont(fte->state, codes[i], textSize, textStyle, fontName, cache);
		if (runFont == NULL)
			continue;
		if (i == 0) {
			CGRect rect = CTFontGetBoundingRectsForGlyphs(runFont, kCTFontDefaultOrientation, &glyph, NULL, 1);
			point.x = origin.x - (rect.origin.x < 0 ? rect.origin.x : 0);	// shift the glyph to right if it has a negative offset
		}
		point.y = origin.y - ascent;
#if USE_GLYPH && GLYPH_HAS_EDGE
//		point.x += GLYPH_HAS_EDGE;
		point.y -= GLYPH_HAS_EDGE;
#endif
#if defined(__MAC_OS_X_VERSION_MAX_ALLOWED) && __MAC_OS_X_VERSION_MAX_ALLOWED <= 1060
		CGFontRef cgFont = CTFontCopyGraphicsFont(runFont, NULL);
		CGContextSetFont(context, cgFont);
		CGContextSetFontSize(context, CTFontGetSize(runFont));
		CGContextShowGlyphsAtPositions(context, &glyph, &point, 1);
		CFRelease(cgFont);
#else	/* OS X 10.7 and later or iOS */
		CTFontDrawGlyphs(runFont, &glyph, &point, 1, context);
#endif	/* OS X 10.7 and later or iOS */
		if (strike || i + 1 < codesLen)
			point.x += CTFontGetAdvancesForGlyphs(runFont, kCTFontDefaultOrientation, &glyph, NULL, 1);
	}
	if (strike) {
		CGColorRef colorRef = createCGColor(color, blendLevel);
		CGContextSetStrokeColorWithColor(context, colorRef);
		origin.y = bits->bounds.height - r->y - ascent;
		point.x = ceilf(point.x);
		CGContextBeginPath(context);
		if (textStyle & kFskTextUnderline) {
			point.y = origin.y + CTFontGetUnderlinePosition(cache->fontRef);
#if USE_GLYPH && GLYPH_HAS_EDGE
//			point.x += GLYPH_HAS_EDGE;
			point.y -= GLYPH_HAS_EDGE;
#endif
			CGContextSetLineWidth(context, CTFontGetUnderlineThickness(cache->fontRef));
			CGContextMoveToPoint(context, origin.x, point.y);
			CGContextAddLineToPoint(context, point.x, point.y);
		}
		if (textStyle & kFskTextStrike) {
			point.y = origin.y - descent + (ascent + descent) / 2;
#if USE_GLYPH && GLYPH_HAS_EDGE
//			point.x += GLYPH_HAS_EDGE;
			point.y -= GLYPH_HAS_EDGE;
#endif
			CGContextSetLineWidth(context, 1.0);
			CGContextMoveToPoint(context, origin.x, point.y);
			CGContextAddLineToPoint(context, point.x, point.y);
		}
		CGContextStrokePath(context);
		CFRelease(colorRef);
	}
	CGContextRestoreGState(context);
	return kFskErrNone;
}

FskErr FskTextGetGlyphs(FskTextEngine fte, FskBitmap bits, const char *text, UInt32 textLen, UInt32 textSize, UInt32 textStyle, const char *fontName, UInt16 **glyphsPtr, UInt32 *glyphsLenPtr, FskFixed **layout, float *widthPtr, float *heightPtr, FskTextFormatCache cache)
{
#if !USE_FRACTIONAL_TEXT_SIZE
	textSize = ROUND_TEXTSIZE(textSize);
#endif

	FskCocoaTextGetGlyphs(fte->state, text, textLen, textSize, textStyle, fontName, glyphsPtr, glyphsLenPtr, layout, widthPtr, heightPtr, cache);
	return kFskErrNone;
}
#endif /* USE_CORE_TEXT */

#pragma mark --- audio ---

#if USE_AUDIO_QUEUE
#define kFskAudioQueueBufferCount (8)

typedef struct {
	AudioQueueBufferRef buffer;
	Boolean bufferFilled;
	FskAudioOutBlock audioOutBlock;
} MyAudioQueueBufferRecord, *MyAudioQueueBuffer;

typedef struct {
	AudioQueueRef queue;
	MyAudioQueueBufferRecord buffers[kFskAudioQueueBufferCount];
	AudioStreamBasicDescription basicDesc;
	AudioStreamPacketDescription *packetDesc;
	UInt32 packetDescSize;
	enum FskAudioOutCategory category;
	UInt32 queuedSamples;
	UInt32 targetQueuedSamples;
} MyAudioQueueRecord, *MyAudioQueue;

Boolean FskCocoaAudioInitialize(FskAudioOut fskAudioOut)
{
	//NSLog(@"FskCocoaAudioInitialize");
	Boolean success = false;
	FskErr err;

	err = FskMemPtrNewClear(sizeof(MyAudioQueueRecord), (FskMemPtr *)&fskAudioOut->audioQueue);
	BAIL_IF_ERR(err);

	success = true;

bail:
	return success;
}

void FskCocoaAudioTerminate(FskAudioOut fskAudioOut)
{
	//NSLog(@"FskCocoaAudioTerminate");
	if (fskAudioOut->audioQueue)
	{
		MyAudioQueue queue = (MyAudioQueue)fskAudioOut->audioQueue;
		if (queue->queue != NULL)
		{
			MyAudioQueueBuffer buffer;
			UInt32 i;

			AudioQueueDispose(queue->queue, true);
			queue->queue = NULL;
			for (i = 0; i < kFskAudioQueueBufferCount; i++)
			{
				buffer = &queue->buffers[i];
				buffer->buffer = NULL;
				buffer->bufferFilled = false;
			}
		}
		if (queue->packetDesc != NULL)
		{
			FskMemPtrDispose(queue->packetDesc);
			queue->packetDesc = NULL;
			queue->packetDescSize = 0;
		}
		FskMemPtrDispose(queue);
		fskAudioOut->audioQueue = NULL;
	}
}

static Boolean
getBasicDescFromFskFormat(UInt32 *fskFormat, UInt16 numChannels, double sampleRate, AudioStreamBasicDescription *desc)
{
	UInt32 size;
	FskErr err;
	BOOL needsGetProperty = YES;

	size = sizeof(AudioStreamBasicDescription);
	memset(desc, 0, size);

	desc->mChannelsPerFrame = numChannels;
	desc->mSampleRate = sampleRate;

	switch (*fskFormat) {
		case kFskAudioFormatPCM16BitBigEndian:
			desc->mFormatFlags |= kAudioFormatFlagIsBigEndian;
		case kFskAudioFormatPCM16BitLittleEndian:
		default:	/* force default LPCM output to use codec if available */
			*fskFormat = (*fskFormat == kFskAudioFormatPCM16BitBigEndian) ? kFskAudioFormatPCM16BitBigEndian : kFskAudioFormatPCM16BitLittleEndian;
			desc->mFormatFlags |= kAudioFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked;
			desc->mFormatID = kAudioFormatLinearPCM;
			desc->mFramesPerPacket = 1;
			desc->mBytesPerFrame = desc->mBytesPerPacket = 2 * desc->mChannelsPerFrame;
			desc->mBitsPerChannel = 16;
			needsGetProperty = NO;
			break;
		case kFskAudioFormatAAC:
			desc->mFormatID = kAudioFormatMPEG4AAC;
			break;
		case kFskAudioFormatMP3:
			desc->mFormatID = kAudioFormatMPEGLayer3;
			break;
			//		  default:
			//			  return false;
	}

	if (needsGetProperty)
	{
		err = AudioFormatGetProperty(kAudioFormatProperty_FormatInfo, 0, NULL, &size, desc);
		if (err != noErr) return false;
	}

	return true;
}

static Boolean
refillBuffer(FskAudioOut fskAudioOut, UInt32 bufferIndex)
{
	MyAudioQueue myQueue = (MyAudioQueue)fskAudioOut->audioQueue;
	AudioQueueRef audioQueue = myQueue->queue;
	Boolean refilled = false;

	// remove unused from queue
	FskAudioOutRemoveUnusedFromQueue(fskAudioOut);

	FskMutexAcquire(fskAudioOut->mutex);

	// get the data
	if (fskAudioOut->playing)
	{
		FskAudioOutBlock fskAudioOutBlock;
		MyAudioQueue queue;
		MyAudioQueueBuffer buffer;
		AudioQueueBufferRef audioQueueBuffer;
		AudioStreamPacketDescription *desc;
		SInt32 offset;
		OSStatus err;

		// get the next block
		for (fskAudioOutBlock = fskAudioOut->blocks; fskAudioOutBlock; fskAudioOutBlock = fskAudioOutBlock->next) {
			if (fskAudioOutBlock->silence || ((fskAudioOutBlock->frameCount == 0) && (fskAudioOutBlock->dataSize == 0)))
			{
				fskAudioOutBlock->done = true;
			}
			if (!fskAudioOutBlock->done && !fskAudioOutBlock->onUsing) break;
		}

		if (fskAudioOutBlock == NULL)
		{
			//NSLog(@"No Out Blocks!!");
			goto bail;
		}

		queue = (MyAudioQueue)fskAudioOut->audioQueue;
		buffer = &queue->buffers[bufferIndex];
		audioQueueBuffer = buffer->buffer;

		if (fskAudioOutBlock->frameCount > queue->packetDescSize)
		{
			if (FskMemPtrRealloc(fskAudioOutBlock->frameCount * sizeof(AudioStreamPacketDescription), (FskMemPtr*)&queue->packetDesc) != kFskErrNone)
			{
				//NSLog(@"Queue Buffer Overflow!!");
				goto bail;
			}
			queue->packetDescSize = fskAudioOutBlock->frameCount;
		}

		if (fskAudioOutBlock->dataSize > audioQueueBuffer->mAudioDataBytesCapacity)
		{
			AudioQueueFreeBuffer(audioQueue, audioQueueBuffer);
			buffer->buffer = NULL;

			err = AudioQueueAllocateBuffer(audioQueue, fskAudioOutBlock->dataSize , &audioQueueBuffer);
			if (err != noErr)
			{
				//NSLog(@"Queue Buffer Overflow!!");
				goto bail;
			}
			buffer->buffer = audioQueueBuffer;
		}

		desc = queue->packetDesc;
		offset = 0;

		audioQueueBuffer->mAudioDataByteSize = fskAudioOutBlock->dataSize;
		FskMemCopy(audioQueueBuffer->mAudioData, fskAudioOutBlock->data, fskAudioOutBlock->dataSize);

		if (fskAudioOutBlock->frameSizes != NULL)
		{
			UInt32 *frameSizes = fskAudioOutBlock->frameSizes;
			UInt32 i;

			for (i = 0; i < fskAudioOutBlock->frameCount; i++)
			{
				desc[i].mStartOffset = offset;
				desc[i].mVariableFramesInPacket = 0;	// maybe always constant number of frames in every packet
				desc[i].mDataByteSize = frameSizes[i];
				offset += frameSizes[i];
			}
		}
		else
		{
			UInt32 bytesPerPacket = (queue->basicDesc.mBytesPerPacket > 0) ? queue->basicDesc.mBytesPerPacket : fskAudioOutBlock->dataSize / fskAudioOutBlock->frameCount;
			UInt32 i;

			for (i = 0; i < fskAudioOutBlock->frameCount; i++)
			{
				desc[i].mStartOffset = offset;
				desc[i].mVariableFramesInPacket = 0;	// maybe always constant numeber of frames in every packet
				desc[i].mDataByteSize = bytesPerPacket;
				offset += bytesPerPacket;
			}
		}

		buffer->bufferFilled = true;
		buffer->audioOutBlock = fskAudioOutBlock;
		queue->queuedSamples += fskAudioOutBlock->frameCount;

		//NSLog(@"refilled %d", (int)bufferIndex);
		(void)AudioQueueEnqueueBuffer(audioQueue, audioQueueBuffer, fskAudioOutBlock->frameCount, desc);
		//NSLog(@"AudioQueueEnqueueBuffer %d", (int)fskAudioOutBlock->frameCount);

		fskAudioOutBlock->onUsing = true;
		refilled = true;
	}

bail:
	FskMutexRelease(fskAudioOut->mutex);

	return refilled;
}

static Boolean
refillBuffers(FskAudioOut fskAudioOut)
{
	MyAudioQueue queue = (MyAudioQueue)fskAudioOut->audioQueue;
	UInt32 i;
	Boolean refilled = false;

	if (queue->queuedSamples >= queue->targetQueuedSamples)
		return false;

	for (i = 0; i < kFskAudioQueueBufferCount; i++)
	{
		if (!queue->buffers[i].bufferFilled)
		{
			if (!refillBuffer(fskAudioOut, i))
			{
				break;
			}
			refilled = true;
		}
	}
	return refilled;
}

static void
audioQueueOutputCallback(void* userData, AudioQueueRef audioQueue, AudioQueueBufferRef audioQueueBuffer)
{
	FskAudioOut fskAudioOut = (FskAudioOut)userData;
	FskAudioOutBlock block;
	MyAudioQueue queue = (MyAudioQueue)fskAudioOut->audioQueue;
	MyAudioQueueBuffer buffer;
	UInt32 i;

	if (audioQueue != queue->queue)
	{
		return;
	}

	for (i = 0; i < kFskAudioQueueBufferCount; i++)
	{
		buffer = &queue->buffers[i];
		if (buffer->buffer == audioQueueBuffer)
		{
			block = buffer->audioOutBlock;
			buffer->bufferFilled = false;
			queue->queuedSamples -= block->frameCount;
			block->onUsing = false;
			block->done = true;
			buffer->audioOutBlock = NULL;
			//NSLog(@"audioQueueOutputCallback %d", (int)i);
			break;
		}
	}

	if (refillBuffers(fskAudioOut))
	{
		//NSLog(@"audioQueueOutputCallback: refilled");
	}
}

Boolean FskCocoaAudioEnqueueBuffers(FskAudioOut fskAudioOut)
{
	return refillBuffers(fskAudioOut);
}

Boolean FskCocoaAudioSetFormat(FskAudioOut audioOut, UInt32 format, UInt16 numChannels, double sampleRate, unsigned char *formatInfo, UInt32 formatInfoSize)
{
	//NSLog(@"FskCocoaAudioOutSetFormat");
	OSStatus err = noErr;
	MyAudioQueue myQueue = (MyAudioQueue)audioOut->audioQueue;
	MyAudioQueueBuffer myBuffer;
	UInt32 size, i;

	if (myQueue == NULL) return false;

	if (myQueue->queue != NULL)
	{
		if ((audioOut->format != format) || (audioOut->sampleRate != sampleRate) || (audioOut->numChannels != numChannels))
		{
			AudioQueueDispose(myQueue->queue, true);
			myQueue->queue = NULL;
			for (i = 0; i < kFskAudioQueueBufferCount; i++)
			{
				myBuffer = &myQueue->buffers[i];
				myBuffer->buffer = NULL;
				myBuffer->bufferFilled = false;
			}
		}
	}
	if (myQueue->queue == NULL)
	{
		UInt32 packetDescSize;

		// basic description
		if (!getBasicDescFromFskFormat(&format, numChannels, sampleRate, &myQueue->basicDesc)) goto bail;

		// make a queue
		err = AudioQueueNewOutput(&myQueue->basicDesc, audioQueueOutputCallback, audioOut,
								  CFRunLoopGetCurrent(), kCFRunLoopCommonModes, 0, (AudioQueueRef*)&myQueue->queue);
		if (err != noErr) goto bail;

		audioOut->chunkRequestSize = ceil(sampleRate / 2);		// for half second
		packetDescSize = audioOut->chunkRequestSize;   // if every packet has only one frame.
		if (myQueue->basicDesc.mFramesPerPacket > 0)
		{
			packetDescSize /= myQueue->basicDesc.mFramesPerPacket;
			audioOut->chunkRequestSize = packetDescSize * myQueue->basicDesc.mFramesPerPacket;
		}
		size = audioOut->chunkRequestSize * numChannels * 2;	// enough for uncommpressed audio
		for (i = 0; i < kFskAudioQueueBufferCount; i++)
		{
			err = AudioQueueAllocateBuffer(myQueue->queue, size, &myQueue->buffers[i].buffer);
			if (err != noErr) goto bail;
		}

		if (myQueue->packetDesc == NULL)
		{
			if (FskMemPtrNewClear(packetDescSize * sizeof(AudioStreamPacketDescription), (FskMemPtr*)&myQueue->packetDesc) != kFskErrNone) goto bail;
			myQueue->packetDescSize = packetDescSize;
		}
		else if (packetDescSize > myQueue->packetDescSize)
		{
			if (FskMemPtrRealloc(packetDescSize * sizeof(AudioStreamPacketDescription), (FskMemPtr*)&myQueue->packetDesc) != kFskErrNone) goto bail;
			myQueue->packetDescSize = packetDescSize;
		}

		audioOut->sampleRate = sampleRate;
		audioOut->format = format;
		audioOut->numChannels = numChannels;
	}

bail:
	return (err == noErr) ? true : false;
}

void FskCocoaAudioGetVolume(FskAudioOut fskAudioOut, UInt16 *leftVolume, UInt16 *rightVolume)
{
	//NSLog(@"FskCocoaAudioGetVolume");
	OSStatus err = noErr;
	MyAudioQueue queue;
	AudioQueueParameterValue volume = 0;
	UInt16 fskVolume = 0;

	if ((fskAudioOut == NULL) || (fskAudioOut->audioQueue == NULL)) goto bail;

	queue = (MyAudioQueue)fskAudioOut->audioQueue;

	if (queue->queue == NULL) goto bail;

	err = AudioQueueGetParameter(queue->queue, kAudioQueueParam_Volume, &volume);
	BAIL_IF_ERR(err);

	fskVolume = volume * 256;
bail:
	if (fskAudioOut)
	{
		fskAudioOut->leftVolume = fskVolume;
		fskAudioOut->rightVolume = fskVolume;
	}

	if (leftVolume)
		*leftVolume = fskVolume;

	if (rightVolume)
		*rightVolume = fskVolume;
}

void FskCocoaAudioSetVolume(FskAudioOut fskAudioOut, UInt16 leftVolume, UInt16 rightVolume)
{
	//NSLog(@"FskCocoaAudioSetVolume");
	UInt16 maxVolume = MAX(leftVolume, rightVolume);
	MyAudioQueue queue;

	if ((fskAudioOut == NULL) || (fskAudioOut->audioQueue == NULL)) return;

	queue = (MyAudioQueue)fskAudioOut->audioQueue;

	if (queue->queue == NULL) return;

	// set the volume
	fskAudioOut->leftVolume = leftVolume;
	fskAudioOut->rightVolume = rightVolume;
	(void)AudioQueueSetParameter(queue->queue, kAudioQueueParam_Volume, (double)maxVolume / 256);
#if TARGET_OS_IPHONE || (defined(MAC_OS_X_VERSION_10_7) && (MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_7))
	// SDK 10.6 does not support kAudioQueueParam_Pan
	if (leftVolume != rightVolume)
	{
		UInt16 minVolume = MIN(leftVolume, rightVolume);
		double pan = (double)(maxVolume - minVolume) / maxVolume;

		if (leftVolume > rightVolume)
		{
			pan = (- pan);
		}
		AudioQueueSetParameter(queue->queue, kAudioQueueParam_Pan, pan);
	}
	else
	{
		AudioQueueSetParameter(queue->queue, kAudioQueueParam_Pan, 0.0);
	}
#endif /* TARGET_OS_IPHONE  || MAC_OS_X_VERSION_MAX_ALLOWED */
}

Boolean FskCocoaAudioStart(FskAudioOut fskAudioOut)
{
	//NSLog(@"FskCocoaAudioStart");
	OSStatus err = noErr;
	MyAudioQueue queue;
#if defined(QUEUE_GET_CURRENT_TIME)
	AudioTimeStamp timeStamp;
#endif /* QUEUE_GET_CURRENT_TIME */

	if ((fskAudioOut == NULL) || (fskAudioOut->audioQueue == NULL))
	{
		return false;
	}

	queue = (MyAudioQueue)fskAudioOut->audioQueue;

#if TARGET_OS_IPHONE
	enum FskAudioOutCategory category = queue ? queue->category : kFskAudioOutCategoryPlayback;
	if (!FskCocoaAudioSessionSetupPlaying(category)) return false;
#endif /* TARGET_OS_IPHONE */

	if (queue->queue == NULL)
	{
		return false;
	}

	FskAudioOutGetSamplesQueued(fskAudioOut, NULL, &queue->targetQueuedSamples);
	queue->queuedSamples = 0;

	refillBuffers(fskAudioOut);

	err = AudioQueueStart(queue->queue, NULL);
	if (err)
	{
		return false;
	}

#if defined(QUEUE_GET_CURRENT_TIME)
	AudioQueueGetCurrentTime(queue->queue, NULL, &timeStamp, NULL);
	fskAudioOut->currentSampleTime = timeStamp.mSampleTime;
#else /* !QUEUE_GET_CURRENT_TIME */
	FskTimeGetNow(&fskAudioOut->startHostTime);
#endif /* !QUEUE_GET_CURRENT_TIME */

	return true;
}

void FskCocoaAudioStop(FskAudioOut fskAudioOut)
{
	//NSLog(@"FskCocoaAudioStop");
	MyAudioQueue queue;
#if defined(QUEUE_GET_CURRENT_TIME)
	AudioTimeStamp timeStamp;
#endif  /* QUEUE_GET_CURRENT_TIME */
	UInt32 i;

	if ((fskAudioOut == NULL) || (fskAudioOut->audioQueue == NULL)) return;

	queue = (MyAudioQueue)fskAudioOut->audioQueue;

	if (queue->queue == NULL) return;

#if defined(QUEUE_GET_CURRENT_TIME)
	AudioQueueGetCurrentTime(queue->queue, NULL, &timeStamp, NULL);
	fskAudioOut->currentSampleTime = timeStamp.mSampleTime;
#else /* !QUEUE_GET_CURRENT_TIME */
	FskTimeGetNow(&fskAudioOut->currentHostTime);
#endif /* !QUEUE_GET_CURRENT_TIME */

	(void)AudioQueueFlush(queue->queue);
	(void)AudioQueueStop(queue->queue, YES);

	for (i = 0; i < kFskAudioQueueBufferCount; i++)
	{
		queue->buffers[i].bufferFilled = false;
		queue->buffers[i].audioOutBlock = NULL;
	}
#if TARGET_OS_IPHONE
	FskCocoaAudioSessionTearDown();
#endif /* TARGET_OS_IPHONE */

	queue->targetQueuedSamples = 0;
	queue->queuedSamples = 0;
}

void FskCocoaAudioGetSamplePosition(FskAudioOut fskAudioOut, FskSampleTime *fskSampleTime)
{
#if defined(QUEUE_GET_CURRENT_TIME)
	Float64 sampleTime = fskAudioOut->currentSampleTime;

	if (fskAudioOut->playing)
	{
		MyAudioQueue myQueue = (MyAudioQueue)fskAudioOut->audioQueue;
		AudioTimeStamp timeStamp;

		AudioQueueGetCurrentTime(myQueue->queue, NULL, &timeStamp, NULL);
		sampleTime += timeStamp.mSampleTime;
	}
	*fskSampleTime = fskAudioOut->zeroTime + sampleTime;
#else /* !QUEUE_GET_CURRENT_TIME */
	FskTimeRecord currentHostTime;
	SInt32 msec;

	if (fskAudioOut->playing)
	{
		FskTimeGetNow(&currentHostTime);
	}
	else
	{
		FskTimeCopy(&currentHostTime, &fskAudioOut->currentHostTime);
	}
	FskTimeSub(&fskAudioOut->startHostTime, &currentHostTime);
	msec = FskTimeInMS(&currentHostTime);
	*fskSampleTime = fskAudioOut->zeroTime + ((FskSampleTime)msec * fskAudioOut->sampleRate) / 1000;
#endif /* !QUEUE_GET_CURRENT_TIME */
}

void FskCocoaAudioHasProperty(FskAudioOut audioOut, UInt32 propertyID, Boolean *get, Boolean *set, UInt32 *dataType)
{
	if (propertyID == kFskAudioOutPropertyCategory) {
		*get = *set = true;
		*dataType = kFskMediaPropertyTypeInteger;
	}
	else {
		*get = *set = false;
		*dataType = kFskMediaPropertyTypeUndefined;
	}
}

Boolean FskCocoaAudioSetProperty(FskAudioOut audioOut, UInt32 propertyID, FskMediaPropertyValue property)
{
	MyAudioQueue queue;

	if (audioOut == NULL || (queue = audioOut->audioQueue) == NULL)
		return false;
	if (propertyID == kFskAudioOutPropertyCategory && property->type == kFskMediaPropertyTypeInteger) {
		queue->category = property->value.integer;
		return true;
	}
	else
		return false;
}

Boolean FskCocoaAudioGetProperty(FskAudioOut audioOut, UInt32 propertyID, FskMediaPropertyValue property)
{
	MyAudioQueue queue;

	if (audioOut == NULL || (queue = audioOut->audioQueue) == NULL)
		return false;
	if (propertyID == kFskAudioOutPropertyCategory) {
		property->type = kFskMediaPropertyTypeInteger;
		property->value.integer = queue->category;
		return true;
	}
	else
		return false;
}

#if TARGET_OS_IPHONE
@interface FskCocoaAudioSession : NSObject
{
	SInt32 activeChannels;
	AVAudioSession *audioSession;
}
@end

@implementation FskCocoaAudioSession
+ (id) sharedInstance
{
	static FskCocoaAudioSession *instance = nil;
	static dispatch_once_t onceToken;

	dispatch_once(&onceToken, ^{
			instance = [[self alloc] init];
		});
	return instance;
}

- (id) init
{
	self = [super init];
	activeChannels = 0;
	audioSession = [AVAudioSession sharedInstance];
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(interruptionListener:)
		name:AVAudioSessionInterruptionNotification object:audioSession];
	return self;
}

- (void) dealloc
{
	[super dealloc];
}

- (void) interruptionListener: (NSNotification *)notification
{
	AVAudioSessionInterruptionType type = [[[notification userInfo] objectForKey:AVAudioSessionInterruptionTypeKey] intValue];
	FskWindow fskWindow;
	UInt32 fkey = 0;

	switch (type) {
	case AVAudioSessionInterruptionTypeBegan:
		activeChannels = 0;	// not necessary but just in case
		fkey = kFskEventFunctionKeyBeginInterruption;
		break;
	case AVAudioSessionInterruptionTypeEnded:
		if ([audioSession setActive:YES error:nil]) {
			AVAudioSessionInterruptionOptions options = [[[notification userInfo] objectForKey:AVAudioSessionInterruptionOptionKey] intValue];
			fkey = options == AVAudioSessionInterruptionOptionShouldResume ? kFskEventFunctionKeyResumeFromInterruption : kFskEventFunctionKeyEndInterruption;
		}
		break;
	}
	if (fkey != 0) {
		if ((fskWindow = [FskCocoaApplication sharedApplication].mainViewController.fskWindow) != NULL) {
			FskEvent fskEvent;
			char chars[2] = {0, 0};
			FskEventCodeEnum ev[2] = {kFskEventKeyDown, kFskEventKeyUp};
			for (unsigned int i = 0; i < sizeof(ev) / sizeof(ev[0]); i++) {
				if (FskEventNew(&fskEvent, ev[i], NULL, 0) == kFskErrNone) {
					FskEventParameterAdd(fskEvent, kFskEventParameterKeyUTF8, sizeof(chars), chars);
					FskEventParameterAdd(fskEvent, kFskEventParameterFunctionKey, sizeof(fkey), &fkey);
					FskWindowEventQueue(fskWindow, fskEvent);
				}
			}
		}
	}
}

- (BOOL) setRecording:(BOOL) startRecord
{
	if (!audioSession.inputAvailable)
		return NO;
	activeChannels = 0;
	if (audioSession.category == AVAudioSessionCategoryRecord) {
		if (!startRecord) {	// stop recording
			[audioSession setCategory:AVAudioSessionCategoryAmbient error:nil];	// set to default
			[audioSession setActive:NO withOptions:AVAudioSessionSetActiveOptionNotifyOthersOnDeactivation error:nil];
		}
		return YES;
	}
	else
		return [audioSession setCategory:AVAudioSessionCategoryRecord error:nil] && [audioSession setActive:YES error:nil];
}

- (BOOL) setPlaying:(BOOL) startPlaying category:(enum FskAudioOutCategory) fskCategory
{
	NSString *category = fskCategory == kFskAudioOutCategoryAmbient ? AVAudioSessionCategoryAmbient : AVAudioSessionCategoryPlayback;
	if (audioSession.category == AVAudioSessionCategoryPlayback ||
	    ([audioSession setCategory:category error:nil] && [audioSession setActive:YES error:nil])) {
		activeChannels++;
		return YES;
	}
	else
		return NO;
}

- (void) deactive
{
	if (activeChannels > 0)
		if (--activeChannels == 0)
			[audioSession setActive:NO withOptions:AVAudioSessionSetActiveOptionNotifyOthersOnDeactivation error:nil];
}
@end

Boolean FskCocoaAudioSessionSetupRecording(Boolean startRecord)
{
	return [[FskCocoaAudioSession sharedInstance] setRecording:startRecord];
}

Boolean FskCocoaAudioSessionSetupPlaying(enum FskAudioOutCategory category)
{
	return [[FskCocoaAudioSession sharedInstance] setPlaying:YES category:category];
}

Boolean FskCocoaAudioSessionSetupFakePlaying()
{
	enum FskAudioOutCategory category = kFskAudioOutCategoryPlayback;
	return FskCocoaAudioSessionSetupPlaying(category);
}

void FskCocoaAudioSessionTearDown()
{
	[[FskCocoaAudioSession sharedInstance] deactive];
}

#endif /* TARGET_OS_IPHONE */

#endif /* USE_AUDIO_QUEUE */

#pragma mark --- file ---
#import "FskFiles.h"
char *FskCocoaGetSpecialPath(UInt32 type, const Boolean create)
{
	NSFileManager *filemgr = [NSFileManager defaultManager];
	NSSearchPathDirectory directory;
	char *path;
	NSURL *url;
	NSString *str;

	switch (type) {
	case kFskDirectorySpecialTypeDocument: directory = NSDocumentDirectory; goto next;
	case kFskDirectorySpecialTypePhoto: directory = NSPicturesDirectory; goto next;
	case kFskDirectorySpecialTypeMusic: directory = NSMusicDirectory; goto next;
	case kFskDirectorySpecialTypeVideo: directory = NSMoviesDirectory; goto next;
	case kFskDirectorySpecialTypeTV: directory = NSMoviesDirectory; goto next;	// just to pass the test...
	case kFskDirectorySpecialTypeDownload: directory = NSDownloadsDirectory; goto next;
	case kFskDirectorySpecialTypeCache: directory = NSCachesDirectory; goto next;
	next:
		url = [filemgr URLForDirectory:directory inDomain:NSUserDomainMask appropriateForURL:nil create:create error:nil];
#if 0	/* 10.9 or later */
		path = url != nil ? FskStrDoCopy([url fileSystemRepresentation]) : NULL;
#else // 1
		path = url != nil ? FskStrDoCopy([[[url path] stringByAppendingString:@"/"] UTF8String]) : NULL;
#endif // 1
		break;
	case kFskDirectorySpecialTypeTemporary:
		str = NSTemporaryDirectory();
		path = str != nil ? FskStrDoCopy([str UTF8String]) : NULL;
		break;
	case kFskDirectorySpecialTypeApplicationPreference:
	case kFskDirectorySpecialTypeApplicationPreferenceRoot:
		if ((url = [filemgr URLForDirectory:NSLibraryDirectory inDomain:NSUserDomainMask appropriateForURL:nil create:NO error:nil]) != nil) {
			url = [url URLByAppendingPathComponent:@"Preferences" isDirectory:YES];
			if (type == kFskDirectorySpecialTypeApplicationPreference)
				url = [url URLByAppendingPathComponent:@"/fsk/1" isDirectory:YES];
#if 0	/* 10.9 or later */
			path = FskStrDoCopy([url fileSystemRepresentation]);
#else // 1
			path = FskStrDoCopy([[[url path] stringByAppendingString:@"/"] UTF8String]);
#endif //1
		}
		else
			path = NULL;
		break;
	case kFskDirectorySpecialTypeStartMenu:
	case kFskDirectorySpecialTypeMusicSync:
	case kFskDirectorySpecialTypeVideoSync:
	case kFskDirectorySpecialTypePhotoSync:
	case kFskDirectorySpecialTypePlaylistSync:
	default:
		path = NULL;
		break;
	}
	return path;
}

const char *FskCocoaDisplayNameAtPath(const char *path)
{
	NSString *name = [[NSFileManager defaultManager] displayNameAtPath:[NSString stringWithUTF8String:path]];
	return name != NULL ? [name UTF8String] : NULL;
}
