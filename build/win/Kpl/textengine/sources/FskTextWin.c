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
#define _WIN32_WINNT 0x0500
#define __FSKBITMAP_PRIV__
#define __FSKTEXT_PRIV__
#include "FskText.h"

#if SUPPORT_EDGEENHANCEDTEXT
	#include "FskEdgeEnhancedText.h"
#endif
#include "FskExtensions.h"
#include "FskFiles.h"
#include "FskPixelOps.h"
#include "FskTextConvert.h"
#include "FskTransferAlphaBitmap.h"
#include "FskUtilities.h"
#include "FskFrameBuffer.h"

struct FskTextEngineStateRecord {
	HDC					dc;

	LOGFONTW			logFont;
};

typedef struct {
	HFONT		font;
	Boolean		haveEllipsisWidth;
	DWORD		ellipsisWidth;
} FskTextFormatCacheGDIRecord, *FskTextFormatCacheGDI;

static FskErr FskBitmapNewWin32(SInt32 width, SInt32 height, UInt32 pixelFormat, FskBitmap *bitsOut);
static FskErr FskBitmapDisposeWin32(FskBitmap bits);

static FskErr winTextNew(FskTextEngineState *stateOut);
static FskErr winTextDispose(FskTextEngineState state);
static FskErr winTextFormatCacheNew(FskTextEngineState state, FskTextFormatCache *cache, FskBitmap bits, UInt16 textSize, UInt32 textStyle, const char *fontName);
static FskErr winTextFormatCacheDispose(FskTextEngineState state, FskTextFormatCache cache);
static FskErr winTextBox(FskTextEngineState state, FskBitmap bits, const char *text, UInt32 textLen, FskConstRectangle bounds, FskConstRectangle clipRect, FskConstColorRGBA color, UInt32 blendLevel, UInt16 textSize, UInt32 textStyle, UInt16 hAlign, UInt16 vAlign, const char *fontName, FskTextFormatCache cache);
static FskErr winTextGetBounds(FskTextEngineState state, FskBitmap bits, const char *text, UInt32 textLen, UInt16 textSize, UInt32 textStyle, const char *fontName, FskRectangle bounds, FskTextFormatCache cache);
static FskErr winTextGetFontInfo(FskTextEngineState state, FskTextFontInfo info, const char *fontName, UInt32 textSize, UInt32 textStyle, FskTextFormatCache formatCache);
static FskErr winTextFitWidth(FskTextEngineState state, FskBitmap bits, const char *text, UInt32 textLen, UInt16 textSize, UInt32 textStyle, const char *fontName, UInt32 width, UInt32 flags, UInt32 *fitBytes, UInt32 *fitChars, FskTextFormatCache cache);
static FskErr winTextAddFontFile(FskTextEngineState state, const char *path);
static FskErr winTextGetFontList(FskTextEngineState state, char **fontNames);
static FskErr winTextSetFamilyMap(FskTextEngineState state, const char *map);

static FskTextEngineDispatchRecord gFskTextWindowsDispatch = {
	"Kpl/Win32/GDI",
	winTextNew,
	winTextDispose,
	winTextFormatCacheNew,
	winTextFormatCacheDispose,
	winTextBox,
	winTextGetBounds,
	winTextGetFontInfo,
	winTextFitWidth,
	NULL,
	winTextAddFontFile,
	NULL,
	winTextGetFontList,
	winTextSetFamilyMap
};

static HGDIOBJ syncFont(UInt32 textSize, UInt32 textStyle, const char *fontName, HFONT *font, FskTextEngineState state);
static void remapForMicrosoft(UInt16 *encodedText, UInt32 count);

static char *gFontFamilyMap = NULL;

/*
	configuration
*/

FskErr FskTextWindowsInitialize(void);
void FskTextWindowsUninitialize(void);

FskErr FskTextWindowsInitialize(void)
{
	FskExtensionInstall(kFskExtensionTextEngine, &gFskTextWindowsDispatch);
	return kFskErrNone;
}

void FskTextWindowsUninitialize(void)
{
	FskExtensionUninstall(kFskExtensionTextEngine, &gFskTextWindowsDispatch);
	FskMemPtrDisposeAt((void **)&gFontFamilyMap);
}

FskErr winTextNew(FskTextEngineState *stateOut)
{
	FskErr err;
	FskTextEngineState state;

	err = FskMemPtrNewClear(sizeof(FskTextEngineStateRecord), (FskMemPtr *)&state);
	if (err) goto bail;

	*stateOut = state;

	state->dc = CreateCompatibleDC(NULL);

//	state->logFont.lfWidth = 0;
//	state->logFont.lfEscapement = 0;
//	state->logFont.lfOrientation = 0;
	state->logFont.lfCharSet = DEFAULT_CHARSET;
	state->logFont.lfOutPrecision = OUT_DEFAULT_PRECIS;
	state->logFont.lfClipPrecision = CLIP_DEFAULT_PRECIS;
	state->logFont.lfQuality = DEFAULT_QUALITY;
	state->logFont.lfPitchAndFamily = DEFAULT_PITCH;

bail:
	return err;
}

FskErr winTextDispose(FskTextEngineState state)
{
	if (state) {
		DeleteDC(state->dc);
		FskMemPtrDispose(state);
	}

	return kFskErrNone;
}

FskErr winTextAddFontFile(FskTextEngineState state, const char *path)
{
	FskErr err;
	WCHAR *nativePath = NULL;

	err = FskFilePathToNative(path, (char **)&nativePath);
	if (err) goto bail;

	if (0 == AddFontResourceExW(nativePath, FR_PRIVATE, 0))
		err = kFskErrOperationFailed;

bail:
	FskMemPtrDispose(nativePath);

	return err;
}

/********************************************************************************
 * winTextBox
 ********************************************************************************/

FskErr winTextBox(FskTextEngineState state, FskBitmap bits, const char *text, UInt32 textLen, FskConstRectangle bounds, FskConstRectangle clipRect, FskConstColorRGBA color, UInt32 blendLevel, UInt16 textSize, UInt32 textStyle, UInt16 hAlign, UInt16 vAlign, const char *fontName, FskTextFormatCache formatCacheIn)
{
	FskTextFormatCacheGDI formatCache = (FskTextFormatCacheGDI)formatCacheIn;
	RECT r;
	UINT flags = DT_SINGLELINE | DT_NOPREFIX | ((kFskTextTruncateEnd & textStyle) ? DT_END_ELLIPSIS : 0);
	Boolean direct = false;
	FskBitmap scratchBits = NULL;
	HFONT font;
	HDC dc = direct ? bits->hdc : state->dc;
	HGDIOBJ saveFont;
	unsigned char scratchBuffer[256];

	if (direct) {
		if (clipRect)
			IntersectClipRect(dc, clipRect->x, clipRect->y, clipRect->x + clipRect->width, clipRect->y + clipRect->height);
		SetRect(&r, bounds->x, bounds->y, bounds->x + bounds->width, bounds->y + bounds->height);

		SetTextColor(dc, RGB(color->r, color->g, color->b));
	}
	else {
		FskErr err;
		const UInt32 kFskTextOffscreenFormat = kFskBitmapFormat24BGR;

		err = FskBitmapNewWin32(-bounds->width, bounds->height, kFskTextOffscreenFormat, &scratchBits);
		if (kFskErrNone == err)
			SetRect(&r, 0, 0, bounds->width, bounds->height);
		else {
			FskRectangleRecord b;

			err = winTextGetBounds(state, bits, text, textLen, textSize, textStyle, fontName, &b, formatCacheIn);
			if (err) return err;

			err = FskBitmapNew(-b.width, b.height, kFskTextOffscreenFormat, &scratchBits);
			if (err) return err;

			SetRect(&r, 0, 0, b.width, b.height);
		}

		dc = scratchBits->hdc;
		SetTextColor(dc, RGB(255, 255, 255));
	}

	if (NULL == formatCache)
		saveFont = syncFont(textSize, textStyle, fontName, &font, state);
	else
		saveFont = SelectObject(dc, formatCache->font);

#if 0
	switch (hAlign) {
		case kFskTextAlignLeft:
		default:
			flags |= DT_LEFT;
			break;

		case kFskTextAlignCenter:
			flags |= DT_CENTER;
			break;

		case kFskTextAlignRight:
			flags |= DT_RIGHT;
			break;
	}

	switch (vAlign) {
		case kFskTextAlignTop:
		default:
			flags |= DT_TOP;
			break;

		case kFskTextAlignCenter:
			flags |= DT_VCENTER;
			break;

		case kFskTextAlignBottom:
			flags |= DT_BOTTOM;
			break;
	}
#endif

	{
	SIZE sz;
	UINT align;
	SInt32 y;
	char *encodedText = (char *)scratchBuffer;
	UInt32 encodedTextLen = sizeof(scratchBuffer);
	if (kFskErrNone != FskTextUTF8ToUnicode16NENoAlloc(text, textLen, (UInt16 *)scratchBuffer, &encodedTextLen)) {
		if (kFskErrNone != FskMemPtrNew(encodedTextLen, &encodedText)) {
			FskBitmapDispose(scratchBits);
			goto done;
		}
		FskTextUTF8ToUnicode16NENoAlloc(text, textLen, (UInt16 *)encodedText, &encodedTextLen);
	}

	encodedTextLen >>= 1;
	remapForMicrosoft((UInt16 *)encodedText, encodedTextLen);

	if (kFskTextTruncateCenter & textStyle) {
		// work hard to truncate the center, since Windows doesn't support this directly
		SIZE size;
		int *widths;

		if (kFskErrNone == FskMemPtrNew(sizeof(int) * encodedTextLen, (FskMemPtr *)&widths)) {
			int maxC, i, fitWidth = (r.right - r.left);
			GetTextExtentExPointW(dc, (WCHAR *)encodedText, encodedTextLen, 32768, &maxC, widths, &size);
			if (size.cx > fitWidth) {
				SInt32 currentWidth = size.cx;
				SInt32 truncBegin, truncEnd;
				WCHAR ellipsis = 0x2026;
				SIZE ellipsisSize;
				UInt16 *uniChars = (UInt16 *)encodedText;

				for (i = encodedTextLen - 1; i > 0; i--)
					widths[i] -= widths[i - 1];

				GetTextExtentPoint32W(dc, (LPWSTR)&ellipsis, 1, &ellipsisSize);		//@@ could use ellipsisWidth in cache here
				fitWidth -= ellipsisSize.cx;

				if (fitWidth > 0) {
					Boolean phase = true;		// start towards the end
					truncBegin = truncEnd = encodedTextLen / 2;
					while ((currentWidth > fitWidth) && ((truncEnd - truncBegin) != encodedTextLen)) {
						if (phase) {
							if (truncEnd < (SInt32)encodedTextLen) {
								currentWidth -= widths[truncEnd];
								truncEnd += 1;
							}
						}
						else {
							if (0 != truncBegin) {
								truncBegin -= 1;
								currentWidth -= widths[truncBegin];
							}
						}
						phase = !phase;
					}
					FskMemMove(&uniChars[truncBegin + 1], &uniChars[truncEnd], (encodedTextLen - truncEnd) * 2);
					uniChars[truncBegin] = ellipsis;
					encodedTextLen -= (truncEnd - truncBegin);
					encodedTextLen += 1;
					flags &= ~DT_END_ELLIPSIS;
				}
			}

			FskMemPtrDispose(widths);
		}
	}

#if 0
	DrawTextW(dc, (LPWSTR)encodedText, encodedTextLen, &r, flags);
#else
	if (kFskTextTruncateEnd & textStyle) {
		int fitChars;
		int stackWidths[256], *widths, width = r.right - r.left;
		WCHAR ellipsis = 0x2026;
		SIZE ellipsisSz;

		if (encodedTextLen < 256)
			widths = stackWidths;
		else {
			if (kFskErrNone != FskMemPtrNew(sizeof(int) * encodedTextLen, (FskMemPtr *)&widths)) {
				widths = stackWidths;
				encodedTextLen = 256;
			}
		}

		GetTextExtentExPointW(dc, (WCHAR *)encodedText, encodedTextLen, width, &fitChars, widths, &sz);
		if ((UInt32)fitChars < encodedTextLen) {
			// remove trailing white space
			if (formatCache) {
				if (!formatCache->haveEllipsisWidth) {
					GetTextExtentExPointW(dc, (WCHAR *)&ellipsis, 1, 0, NULL, NULL, &ellipsisSz);
					formatCache->haveEllipsisWidth = true;
					formatCache->ellipsisWidth = ellipsisSz.cx;
				}
				else
					ellipsisSz.cx = formatCache->ellipsisWidth;
			}
			else
				GetTextExtentExPointW(dc, (WCHAR *)&ellipsis, 1, 0, NULL, NULL, &ellipsisSz);

			if (width > ellipsisSz.cx) {
				width -= ellipsisSz.cx;

				while (fitChars > 2) {
					UInt16 c = ((UInt16 *)encodedText)[fitChars - 2];

					if ((32 != c) && (9 != c) && (0x3000 != c))
						break;

					fitChars -= 1;
				}

				// truncate if needed to make room for the ellipsis
				while ((widths[fitChars - 1] > width) && (fitChars > 2))
					fitChars -= 1;

				// add ellipsis
				((UInt16 *)encodedText)[fitChars - 1] = 0x2026;		// ellipsis
				encodedTextLen = fitChars;
			}
			else
				encodedTextLen = 0;
		}

		if (widths != stackWidths)
			FskMemPtrDispose(widths);
	}
	else {
		if (kFskTextAlignCenter == vAlign)
			GetTextExtentExPointW(dc, (WCHAR *)encodedText, encodedTextLen, 0, NULL, NULL, &sz);
	}

	if (kFskTextAlignCenter == vAlign) {
		y = (r.top + r.bottom - sz.cy) >> 1;
		align = TA_TOP;
	}
	else
	if (kFskTextAlignTop == vAlign) {
		y = r.top;
		align = TA_TOP;
	}
	else {		// kFskTextAlignBottom
		y = r.bottom;
		align = TA_BOTTOM;
	}

	if (kFskTextAlignLeft == hAlign) {
		SetTextAlign(dc, TA_LEFT | align);
		ExtTextOutW(dc, r.left, y, ETO_CLIPPED, &r, (WCHAR *)encodedText, encodedTextLen, NULL);

	}

	else
	if (kFskTextAlignCenter == hAlign) {
		SetTextAlign(dc, TA_CENTER | align);
		ExtTextOutW(dc, (r.left + r.right) >> 1, y, ETO_CLIPPED, &r, (WCHAR *)encodedText, encodedTextLen, NULL);

	}

	else {// kFskTextAlignRight

		SetTextAlign(dc, TA_RIGHT | align);
		ExtTextOutW(dc, r.right, y, ETO_CLIPPED, &r, (WCHAR *)encodedText, encodedTextLen, NULL);

	}

#endif

	if ((void *)scratchBuffer != (void *)encodedText)
		FskMemPtrDispose(encodedText);
	}

	if (scratchBits) {
		FskPointRecord where = {bounds->x, bounds->y};
		FskGraphicsModeParametersRecord modeParams = {sizeof(FskGraphicsModeParametersRecord), blendLevel};
		int h;
		unsigned char *i = (unsigned char *)scratchBits->bits;

		// crush back to gray - use super-cheap luminence calculation to accomdate cleartype
		for (h=scratchBits->bounds.height; h>0; h--) {
			int w = scratchBits->bounds.width;
			unsigned char *o = i;
			while (w--) {
				*o++ = (i[0] + (i[1] << 1) + i[2]) >> 2;
				i += 3;
			}
			i += scratchBits->rowBytes - (3 * scratchBits->bounds.width);
		}
		scratchBits->pixelFormat = kFskBitmapFormat8A;
		scratchBits->depth = 8;

		if (0 == ((kFskTextOutline | kFskTextOutlineHeavy) & textStyle))
			FskTransferAlphaBitmap(scratchBits, NULL, bits, &where, clipRect, color, (blendLevel >= 255) ? NULL : &modeParams);
#if SUPPORT_EDGEENHANCEDTEXT
		else {
			FskColorRGBARecord edgeColor;
			edgeColor.r = ~color->r;
			edgeColor.g = ~color->g;
			edgeColor.b = ~color->b;
			edgeColor.a = color->a;

			if (0 == (kFskTextOutlineHeavy & textStyle))
				FskTransferEdgeEnhancedGrayscaleText(scratchBits, &scratchBits->bounds, bits, &where, clipRect, color, &edgeColor, blendLevel);
			else
				FskTransferEdgeEnhancedDoubleGrayscaleText(scratchBits, &scratchBits->bounds, bits, &where, clipRect, color, &edgeColor, blendLevel);
		}
#endif

		FskBitmapDisposeWin32(scratchBits);
	}
	else {
		SelectClipRgn(dc, NULL);		// for next time around
	}

done:
	if (NULL == formatCache) {
		SelectObject(dc, saveFont);
		DeleteObject(font);
	}

	return kFskErrNone;
}


/********************************************************************************
 * winTextGetBounds
 ********************************************************************************/

FskErr winTextGetBounds(FskTextEngineState state, FskBitmap bits, const char *text, UInt32 textLen, UInt16 textSize, UInt32 textStyle, const char *fontName, FskRectangle bounds, FskTextFormatCache formatCacheIn)
{
	FskTextFormatCacheGDI formatCache = (FskTextFormatCacheGDI)formatCacheIn;
	SIZE sz;
	HFONT font;
	HDC dc = state->dc;
	HGDIOBJ saveFont;
	unsigned char scratchBuffer[256];

	if (NULL == formatCache)
		saveFont = syncFont(textSize, textStyle, fontName, &font, state);
	else
		saveFont = SelectObject(dc, formatCache->font);

	{
	char *encodedText = (char *)scratchBuffer;
	UInt32 encodedTextLen;
	encodedTextLen = sizeof(scratchBuffer);
	if (kFskErrNone != FskTextUTF8ToUnicode16NENoAlloc(text, textLen, (UInt16 *)scratchBuffer, &encodedTextLen)) {
		if (kFskErrNone != FskMemPtrNew(encodedTextLen, &encodedText))
			goto bail;
		FskTextUTF8ToUnicode16NENoAlloc(text, textLen, (UInt16 *)encodedText, &encodedTextLen);
	}

	encodedTextLen >>= 1;
	remapForMicrosoft((UInt16 *)encodedText, encodedTextLen);

	GetTextExtentPoint32W(dc, (LPWSTR)encodedText, encodedTextLen, &sz);
	FskRectangleSet(bounds, 0, 0, sz.cx, sz.cy);

	if ((void *)scratchBuffer != (void *)encodedText)
		FskMemPtrDispose(encodedText);
	}

	if ((kFskTextOutline | kFskTextOutlineHeavy) & textStyle) {
		SInt32 outlineWidth = 2;
		if (kFskTextOutlineHeavy & textStyle)
			outlineWidth = 4;
		bounds->width += outlineWidth;
		bounds->height += outlineWidth;
	}

bail:
	if (NULL == formatCache) {
		SelectObject(dc, saveFont);
		DeleteObject(font);
	}

	return kFskErrNone;
}


/********************************************************************************
 * syncFont
 ********************************************************************************/

static Boolean setToFont(HDC dc, LOGFONTW *logFont, const char *fontName, UInt32 fontNameLen, HGDIOBJ *result, HFONT *font, const char *mapping, Boolean checkMatch);

HGDIOBJ syncFont(UInt32 textSize, UInt32 textStyle, const char *fontName, HFONT *font, FskTextEngineState state)
{
	LOGFONTW *logFont = &state->logFont;
	HGDIOBJ result = NULL;

	*font = NULL;

	logFont->lfHeight = -(SInt32)textSize;
	logFont->lfWeight = ((textStyle & kFskTextBold) ? FW_BOLD : FW_NORMAL);
	logFont->lfItalic = (0 != (textStyle & kFskTextItalic));
	logFont->lfUnderline = (0 != (textStyle & kFskTextUnderline));
	logFont->lfStrikeOut = (0 != (textStyle & kFskTextStrike));

	while (fontName && *fontName) {
		UInt32 fontNameLen = FskStrLen(fontName);
		const char *next = fontName + fontNameLen + 1;

		if (setToFont(state->dc, logFont, fontName, fontNameLen, &result, font, gFontFamilyMap, 0 != *next))
			break;

		fontName = next;
	}

	if (NULL == *font) {
		logFont->lfFaceName[0] = 0;
		*font = CreateFontIndirectW(logFont);
		result = SelectObject(state->dc, *font);
	}

	return result;
}

Boolean setToFont(HDC dc, LOGFONTW *logFont, const char *fontName, UInt32 fontNameLen, HGDIOBJ *result, HFONT *font, const char *map, Boolean checkMatch)
{
	Boolean match = false;
	WCHAR fontNameBuf[64];
	UInt32 bufLen;

	bufLen = sizeof(logFont->lfFaceName);
	if (kFskErrNone != FskTextUTF8ToUnicode16NENoAlloc(fontName, fontNameLen, (UInt16 *)logFont->lfFaceName, &bufLen))
		goto bail;

	logFont->lfFaceName[bufLen >> 1] = 0;

	if (*font)
		DeleteObject(*font);

	*font = CreateFontIndirectW(logFont);

	*result = SelectObject(dc, *font);

	if (!checkMatch && !map) {
		match = true;
		goto bail;
	}

	if (0 == GetTextFaceW(dc, sizeof(fontNameBuf) / 2, fontNameBuf))
		goto bail;

	match = 0 == wcscmp(fontNameBuf, logFont->lfFaceName);
	if (match) goto bail;

	// try the substitution map
	if (NULL != map) {
		while (*map) {
			if (0 == FskStrCompare(map, fontName)) {
				const char *t = map + FskStrLen(map) + 1;
				match = setToFont(dc, logFont, t, FskStrLen(t), result, font, NULL, true);
				goto bail;
			}
			map += FskStrLen(map) + 1;
			map += FskStrLen(map) + 1;
		}
	}

bail:
	return match;
}

typedef struct {
	char		*fonts;
	UInt32		lastFontOffset;
	UInt32		totalSize;
} GetFontState;


static int CALLBACK enumFont(const ENUMLOGFONTEXW *lpelfe, const NEWTEXTMETRICEXW *lpntme, DWORD FontType, LPARAM lParam);


/********************************************************************************
 * enumFont
 ********************************************************************************/

int CALLBACK enumFont(const ENUMLOGFONTEXW *lpelfe, const NEWTEXTMETRICEXW *lpntme, DWORD FontType, LPARAM lParam)
{
	GetFontState *state = (GetFontState *)lParam;
	UInt32 nameLen;
	char *fontName;

	if ('@' == lpelfe->elfLogFont.lfFaceName[0])
		return 1;

	if (kFskErrNone != FskTextUnicode16LEToUTF8((const UInt16 *)lpelfe->elfLogFont.lfFaceName, wcslen(lpelfe->elfLogFont.lfFaceName) * 2, &fontName, NULL))
		return 0;

	if (state->lastFontOffset && (0 == FskStrCompare(state->fonts + state->lastFontOffset, fontName))) {
		FskMemPtrDispose(fontName);
		return 1;
	}

	nameLen = FskStrLen(fontName);
	if (kFskErrNone != FskMemPtrRealloc(nameLen + state->totalSize + 2, (FskMemPtr *)&state->fonts)) {
		FskMemPtrDispose(fontName);
		return 0;
	}

	if (0 != state->totalSize)
		state->lastFontOffset += FskStrLen(&state->fonts[state->lastFontOffset]) + 1;
	state->totalSize += (2 + nameLen);
	FskStrCopy(&state->fonts[state->lastFontOffset], fontName);
	state->fonts[state->lastFontOffset + nameLen + 1] = 0;

	FskMemPtrDispose(fontName);

	return 1;
}

/********************************************************************************
 * winTextGetFontList
 ********************************************************************************/

FskErr winTextGetFontList(FskTextEngineState stateIn, char **fontNames)
{
	LOGFONTW lf;
	GetFontState state;

	FskMemSet(&state, 0, sizeof(state));
	FskMemSet(&lf, 0, sizeof(lf));
	lf.lfCharSet = DEFAULT_CHARSET;
	EnumFontFamiliesExW(stateIn->dc, &lf, (FONTENUMPROCW)enumFont, (LPARAM)&state, 0);

	*fontNames = state.fonts;

	return kFskErrNone;
}

FskErr winTextSetFamilyMap(FskTextEngineState state, const char *map)
{
	FskMemPtrDisposeAt((void **)&gFontFamilyMap);
	return FskStrListDoCopy(map, &gFontFamilyMap);
}

FskErr winTextFitWidth(FskTextEngineState state, FskBitmap bits, const char *text, UInt32 textLen, UInt16 textSize, UInt32 textStyle, const char *fontName, UInt32 width, UInt32 flags, UInt32 *fitBytesOut, UInt32 *fitCharsOut, FskTextFormatCache formatCacheIn)
{
	FskTextFormatCacheGDI formatCache = (FskTextFormatCacheGDI)formatCacheIn;
	FskErr err = kFskErrNone;
	HFONT font;
	HDC dc = state->dc;
	UInt16 *encodedText = NULL;
	UInt32 encodedTextLen, unicodeCharCount;
	SIZE sz;
	int fitChars;
	HGDIOBJ saveFont;
	unsigned char scratchBuffer[256];

	if (NULL == formatCache)
		saveFont = syncFont(textSize, textStyle, fontName, &font, state);
	else
		saveFont = SelectObject(dc, formatCache->font);

	encodedText = (UInt16 *)scratchBuffer;
	encodedTextLen = sizeof(scratchBuffer);
	if (kFskErrNone != FskTextUTF8ToUnicode16NENoAlloc(text, textLen, (UInt16 *)encodedText, &encodedTextLen)) {
		if (kFskErrNone != FskMemPtrNew(encodedTextLen, &encodedText))
			goto bail;
		FskTextUTF8ToUnicode16NENoAlloc(text, textLen, (UInt16 *)encodedText, &encodedTextLen);
	}

	encodedTextLen >>= 1;
	remapForMicrosoft(encodedText, encodedTextLen);

	unicodeCharCount = encodedTextLen;

	if (0 == GetTextExtentExPointW(dc, (WCHAR *)encodedText, unicodeCharCount, width, &fitChars, NULL, &sz))
		fitChars = 0;

	if (kFskTextFitFlagBreak & flags) {
		if ((UInt32)fitChars < unicodeCharCount) {
			// find last whitespace
			UInt16 *uc = &encodedText[fitChars - 1];

			while (uc >= encodedText) {
				UInt16 c = *uc;
				if ((32 == c) || (9 == c) || (0x3000 == c)) {
					fitChars = (uc + 1) - encodedText;
					break;
				}
				uc -= 1;
			}
		}
	}

	if (fitCharsOut)
		*fitCharsOut = fitChars;

	if (NULL != fitBytesOut) {
		const char *p = text;
		while (0 != fitChars) {
			unsigned char c = *p++;
			fitChars--;
			if (c & 0x80) {
				c = *p++;
				while (c & 0x80) {
					if ((c & 0xc0) == 0xc0)
						break;
					c = *p++;
				}
				p--;
			}
		}
		*fitBytesOut = p - text;
	}

bail:
	if (scratchBuffer != (void *)encodedText)
		FskMemPtrDispose(encodedText);

	if (NULL == formatCache) {
		SelectObject(dc, saveFont);
		DeleteObject(font);
	}

	return err;
}

FskErr winTextGetFontInfo(FskTextEngineState state, FskTextFontInfo info, const char *fontName, UInt32 textSize, UInt32 textStyle, FskTextFormatCache formatCacheIn)
{
	FskTextFormatCacheGDI formatCache = (FskTextFormatCacheGDI)formatCacheIn;
	HFONT font, saveFont;
	HDC dc = state->dc;
	TEXTMETRIC metrics;

	if (NULL == formatCache)
		saveFont = (HFONT)syncFont(textSize, textStyle, fontName, &font, state);
	else
		saveFont = (HFONT)SelectObject(dc, formatCache->font);

	GetTextMetrics(dc, &metrics);
	info->ascent = metrics.tmAscent;
	info->descent = metrics.tmDescent;
	info->leading = metrics.tmExternalLeading;

	if ((kFskTextOutline | kFskTextOutlineHeavy) & textStyle) {
		SInt32 outlineWidth = 1;
		if (kFskTextOutlineHeavy & textStyle)
			outlineWidth = 2;
		info->ascent += outlineWidth;
		info->descent += outlineWidth;
	}

	if (NULL == formatCache) {
		SelectObject(dc, saveFont);
		DeleteObject(font);
	}

	return kFskErrNone;
}

FskErr winTextFormatCacheNew(FskTextEngineState state, FskTextFormatCache *cacheOut, FskBitmap bits, UInt16 textSize, UInt32 textStyle, const char *fontName)
{
	FskErr err = kFskErrNone;
	HFONT font, saveFont;

	saveFont = (HFONT)syncFont(textSize, textStyle, fontName, &font, state);
	if (NULL == saveFont) {
		err = kFskErrOperationFailed;
		goto bail;
	}

	FskMemPtrNewClear(sizeof(FskTextFormatCacheGDIRecord), cacheOut);

	((FskTextFormatCacheGDI)*cacheOut)->font = font;

bail:
	return err;
}

FskErr winTextFormatCacheDispose(FskTextEngineState state, FskTextFormatCache cacheIn)
{
	if (NULL != cacheIn) {
		FskTextFormatCacheGDI cache = (FskTextFormatCacheGDI)cacheIn;

		DeleteObject(cache->font);
		FskMemPtrDispose(cache);
	}

	return kFskErrNone;
}

void remapForMicrosoft(UInt16 *p, UInt32 count)
{
	while (count--) {
		UInt16 c = *p++;
		if (c < 0x2016)
			continue;

		if (0x2016 == c)
			p[-1] = 0x2225;
		else
		if (0x301c == c)
			p[-1] = 0xFF5E;
	}
}

FskErr FskBitmapNewWin32(SInt32 width, SInt32 height, UInt32 pixelFormat, FskBitmap *bitsOut)
{
	FskErr err = kFskErrNone;
	UInt32 depth = 0;
	FskBitmap bits = NULL;
	UInt32 colorPad = 0;
	Boolean wantsNativeBitmap = false;

	if (width < 0) {
		width = -width;
		wantsNativeBitmap = true;
	}

	if (wantsNativeBitmap) {
		switch (pixelFormat) {
			case kFskBitmapFormat16RGB565LE:
				depth = 16;
				colorPad = 20;
				break;

			case kFskBitmapFormat24BGR:
				depth = 24;
				break;

			case kFskBitmapFormat32ARGB:
			case kFskBitmapFormat32BGRA:
				depth = 32;
				break;

			case kFskBitmapFormat8A:
				depth = 8;
				colorPad = 256 * 4;
				break;
		}
	}

	if (0 != depth) {
		err = FskMemPtrNewClear(sizeof(FskBitmapRecord) + colorPad, (FskMemPtr *)&bits);
		if (err) goto bail;

		bits->rowBytes = (width * depth) / 8;
		if (bits->rowBytes % 4)
			bits->rowBytes += (4 - (bits->rowBytes % 4));
//		bits->rowBytes = bits->rowBytes;

		bits->bmpInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		bits->bmpInfo.bmiHeader.biWidth = width;
		bits->bmpInfo.bmiHeader.biHeight = (SInt32)-height;	// positive to get an upside down DIB (Windows deals with it better...)
		bits->bmpInfo.bmiHeader.biPlanes = 1;
		bits->bmpInfo.bmiHeader.biBitCount = (WORD)depth;
		if (16 == depth) {
			DWORD *colors;
			bits->bmpInfo.bmiHeader.biCompression = BI_BITFIELDS;
			colors = (DWORD *)&bits->bmpInfo.bmiColors;
			colors[0] = 0x1f << 11;
			colors[1] = 0x3f << 5;
			colors[2] = 0x1f;
		}
		else {
			bits->bmpInfo.bmiHeader.biCompression = BI_RGB;

			if (kFskBitmapFormat8A == pixelFormat) {
				RGBQUAD *color = bits->bmpInfo.bmiColors;
				UInt32 i;
				bits->bmpInfo.bmiHeader.biClrUsed = 255;
				for (i=0; i<256; i++, color++) {
					color->rgbRed =
					color->rgbGreen =
					color->rgbBlue = (BYTE)i;
					color->rgbReserved = 0;
				}
			}
		}

		if ((0 != bits->bmpInfo.bmiHeader.biWidth) && (0 != bits->bmpInfo.bmiHeader.biHeight)) {
			bits->hdc = CreateCompatibleDC(NULL);
			bits->hbmp = CreateDIBSection(bits->hdc, &bits->bmpInfo, DIB_RGB_COLORS,
								(void**)&bits->dibBits, 0, 0);
			if (NULL == bits->hbmp) {
				err = kFskErrMemFull;
				goto bail;
			}
//			bits->bits = (void *)((-bits->rowBytes * (height - 1)) + (unsigned char *)bits->dibBits);
			bits->bits = (void *)bits->dibBits;

			bits->ext = (void *)SelectObject(bits->hdc, bits->hbmp);
			SetBkMode(bits->hdc, TRANSPARENT);
		}
	}

	if (0 == depth) {
		// fall back case - instantiates bitmap even though host platform does not support the pixel format
		UInt32 pixelsSize = 0;
		SInt32 rowBytes = 0;

		switch (pixelFormat) {
			case kFskBitmapFormat24BGR:
			case kFskBitmapFormat24RGB:
				depth = 24;
				break;

			case kFskBitmapFormat32BGRA:
			case kFskBitmapFormat32ARGB:
			case kFskBitmapFormat32ABGR:
			case kFskBitmapFormat32RGBA:
			case kFskBitmapFormat32A16RGB565LE:
				depth = 32;
				break;

			case kFskBitmapFormat16RGB565BE:
			case kFskBitmapFormat16RGB565LE:
			case kFskBitmapFormat16RGB5515LE:
			case kFskBitmapFormat16RGBA4444LE:
			case kFskBitmapFormat16AG:
			case kFskBitmapFormatYUV422:						// Chunky: UYVY, YUYV, YVYU, or VYUY
				depth = 16;
				break;

			case kFskBitmapFormat8G:
				depth = 8;
				break;

			case kFskBitmapFormatYUV420:						// Planar: Y[4n], U[n], V[n]
				depth = 8;										// of Y
				rowBytes = width;								// Row bytes of Y plane
				rowBytes = (rowBytes + 3) & ~3;					// Assure that rowbytes is a multiple of 4.
				pixelsSize = (rowBytes * height);				// Bytes of Y plane
				pixelsSize += (rowBytes * (height + (height & 1))) / 2;	// U & V planes
				break;

			case kFskBitmapFormatYUV420i:						// Chunky: UVYYYY UVYYYY ...
				depth = 8;										// of Y
				rowBytes = width * 3;							// 2 scan lines (1.5 bytes per pixel)
				rowBytes = (rowBytes + 3) & ~3;					// Assure that rowbytes is a multiple of 4.
				pixelsSize = rowBytes * ((height + 1) >> 1);	// Since each row contains 2 scanlines, halve the [evenized] height
				break;

			default:
				err = kFskErrInvalidParameter;
				goto bail;
		}

		if (0 == rowBytes) {
			rowBytes = (depth * width) / 8;
			rowBytes = (rowBytes + 3) & ~3;						// Bump up rowbytes to the next multiple of 4.
		}

		if (0 == pixelsSize)
			pixelsSize = height * rowBytes;

		if (NULL == bits) {
			UInt32 alignedBitmapSize = (sizeof(FskBitmapRecord) + 31) & ~31;

			err = FskMemPtrNew(pixelsSize + alignedBitmapSize, &bits);
			if (err) goto bail;

			FskMemSet(bits, 0, sizeof(FskBitmapRecord));

			bits->bits = alignedBitmapSize + (char *)bits;
		}

		if (!bits->rowBytes)
			bits->rowBytes = rowBytes;
		bits->alphaIsPremultiplied = false;
	}

	FskRectangleSet(&bits->bounds, 0, 0, width, height);
	bits->pixelFormat = pixelFormat;
	bits->depth = depth;

	FskInstrumentedItemSendMessageNormal(bits, kFskBitmapInstrMsgInitialize, bits);

bail:
	if (kFskErrNone != err) {
		FskBitmapDispose(bits);
		bits = NULL;
	}

	*bitsOut = bits;

	return err;
}

FskErr FskBitmapDisposeWin32(FskBitmap bits)
{
	if (NULL != bits) {
		bits->useCount -= 1;
		if (bits->useCount >= 0)
			return kFskErrNone;

		if (bits->doDispose) {
			(bits->doDispose)(bits, bits->doDisposeRefcon);
			return kFskErrNone;
		}

		if (kFskErrNone != FskFrameBufferBitmapDispose(bits)) {
			if (bits->hbmp && bits->hdc)
				SelectObject(bits->hdc, bits->ext);
			if (NULL != bits->hbmp)
				DeleteObject(bits->hbmp);
			if (NULL != bits->hdc)
				DeleteDC(bits->hdc);
			FskMemPtrDispose(bits->bitsToDispose);
			FskMemPtrDispose(bits);
		}
	}

	return kFskErrNone;
}


