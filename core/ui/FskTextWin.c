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

#include "usp10.h"
#pragma comment(lib, "usp10.lib")

struct FskTextEngineStateRecord {
	HDC					dc;

	LOGFONTW			logFont;
};

typedef struct {
	HFONT		font;
	Boolean		haveEllipsisWidth;
	DWORD		ellipsisWidth;
} FskTextFormatCacheGDIRecord, *FskTextFormatCacheGDI;

static FskErr winTextNew(FskTextEngineState *stateOut);
static FskErr winTextDispose(FskTextEngineState state);
static FskErr winTextFormatCacheNew(FskTextEngineState state, FskTextFormatCache *cache, FskBitmap bits, UInt32 textSize, UInt32 textStyle, const char *fontName);
static FskErr winTextFormatCacheDispose(FskTextEngineState state, FskTextFormatCache cache);
static FskErr winTextBox(FskTextEngineState state, FskBitmap bits, const char *text, UInt32 textLen, FskConstRectangle bounds, FskConstRectangle clipRect, FskConstColorRGBA color, UInt32 blendLevel,
							UInt32 textSize, UInt32 textStyle, UInt16 hAlign, UInt16 vAlign, const char *fontName, FskTextFormatCache cache);
static FskErr winTextGetBounds(FskTextEngineState state, FskBitmap bits, const char *text, UInt32 textLen, UInt32 textSize, UInt32 textStyle, const char *fontName, FskRectangle bounds, FskTextFormatCache cache);
static FskErr winTextGetFontInfo(FskTextEngineState state, FskTextFontInfo info, const char *fontName, UInt32 textSize, UInt32 textStyle, FskTextFormatCache formatCache);
static FskErr winTextFitWidth(FskTextEngineState state, FskBitmap bits, const char *text, UInt32 textLen, UInt32 textSize, UInt32 textStyle, const char *fontName, UInt32 width, UInt32 flags,
								UInt32 *fitBytes, UInt32 *fitChars, FskTextFormatCache cache);
static FskErr winTextGetLayout(FskTextEngineState state, FskBitmap bits, const char *text, UInt32 textLen, UInt32 textSize, UInt32 textStyle, const char *fontName,
								UInt16 **unicodeText, UInt32 *unicodeLen, FskFixed **layout, FskTextFormatCache cache);
static FskErr winTextAddFontFile(FskTextEngineState state, const char *path);
static FskErr winTextGetFontList(FskTextEngineState state, char **fontNames);
static FskErr winTextSetFamilyMap(FskTextEngineState state, const char *map);
static FskErr winTextGetLayout(FskTextEngineState fte, FskBitmap bits, const char *text, UInt32 textLen, UInt32 textSize, UInt32 textStyle, const char *fontName,
								UInt16 **unicodeText, UInt32 *unicodeLen, FskFixed **layout, FskTextFormatCache cache);

static FskTextEngineDispatchRecord gFskTextWindowsDispatch = {
	"Win32/GDI",
	winTextNew,
	winTextDispose,
	winTextFormatCacheNew,
	winTextFormatCacheDispose,
	winTextBox,
	winTextGetBounds,
	winTextGetFontInfo,
	winTextFitWidth,
	winTextGetLayout,
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
	BAIL_IF_ERR(err);

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
	BAIL_IF_ERR(err);

	if (0 == AddFontResourceExW(nativePath, FR_PRIVATE, 0))
		BAIL(kFskErrOperationFailed);

bail:
	FskMemPtrDispose(nativePath);

	return err;
}


/********************************************************************************
 * winTextBox
 ********************************************************************************/

FskErr winTextBox(FskTextEngineState state, FskBitmap bits, const char *text, UInt32 textLen, FskConstRectangle bounds, FskConstRectangle clipRect, FskConstColorRGBA color, UInt32 blendLevel, UInt32 textSize, UInt32 textStyle, UInt16 hAlign, UInt16 vAlign, const char *fontName, FskTextFormatCache formatCacheIn)
{
	FskTextFormatCacheGDI formatCache = (FskTextFormatCacheGDI)formatCacheIn;
	RECT r;
	UINT flags = DT_SINGLELINE | DT_NOPREFIX | ((kFskTextTruncateEnd & textStyle) ? DT_END_ELLIPSIS : 0);
	Boolean direct = (blendLevel >= 255) && (NULL != bits->hbmp) &&
			!((kFskTextOutline | kFskTextOutlineHeavy) & textStyle) && !bits->hasAlpha;
	FskBitmap scratchBits = NULL;
	HFONT font;
	HDC dc = direct ? bits->hdc : state->dc;
	HGDIOBJ saveFont;
	FskRectangleRecord clip;
	unsigned char scratchBuffer[256];

	// combine bounds and clip to total clip
	if (NULL == clipRect) {
		clip = *bounds;
	}
	else {
		if (false == FskRectangleIntersect(clipRect, bounds, &clip))
			return kFskErrNone;
	}

	if (direct) {
		if (clipRect)
			IntersectClipRect(dc, clipRect->x, clipRect->y, clipRect->x + clipRect->width, clipRect->y + clipRect->height);
		SetRect(&r, bounds->x, bounds->y, bounds->x + bounds->width, bounds->y + bounds->height);

		SetTextColor(dc, RGB(color->r, color->g, color->b));
	}
	else {
		FskErr err;
		const UInt32 kFskTextOffscreenFormat = kFskBitmapFormat24BGR;

		/* Negative width below indicates that we want a native bitmap */
		err = FskBitmapNew(-bounds->width, bounds->height, kFskTextOffscreenFormat, &scratchBits);
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
				FskTransferEdgeEnhancedGrayscaleText(scratchBits, &scratchBits->bounds, bits, &where, &clip, color, &edgeColor, blendLevel);
			else
				FskTransferEdgeEnhancedDoubleGrayscaleText(scratchBits, &scratchBits->bounds, bits, &where, &clip, color, &edgeColor, blendLevel);
		}
#endif

		FskBitmapDispose(scratchBits);
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

FskErr winTextGetBounds(FskTextEngineState state, FskBitmap bits, const char *text, UInt32 textLen, UInt32 textSize, UInt32 textStyle, const char *fontName, FskRectangle bounds, FskTextFormatCache formatCacheIn)
{
	FskTextFormatCacheGDI formatCache = (FskTextFormatCacheGDI)formatCacheIn;
	SIZE sz;
	HFONT font;
	HDC dc = (bits && bits->hdc) ? bits->hdc : state->dc;
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


#ifdef USE_CHARACTER_PLACEMENT

FskErr winTextGetLayout(FskTextEngineState state, FskBitmap bits, const char *text, UInt32 textLen, UInt32 textSize, UInt32 textStyle, const char *fontName,
						UInt16 **unicodeText, UInt32 *unicodeLen, FskFixed **layout, FskTextFormatCache formatCacheIn)
{
	FskErr					err			= kFskErrNone;
	HDC						dc			= (bits && bits->hdc) ? bits->hdc : state->dc;
	FskTextFormatCacheGDI	formatCache = (FskTextFormatCacheGDI)formatCacheIn;
	UInt16					*uniText	= NULL;
	UInt32					uniTextLen;
	GCP_RESULTSW				results;
	HFONT					font;
	DWORD					heightWidth;
	HGDIOBJ					saveFont;

	/* Initialize return values for the error case */
	FskMemSet(&results, 0, sizeof(GCP_RESULTS));
	if (unicodeText)	*unicodeText	= NULL;
	if (unicodeLen)		*unicodeLen		= 0;
	if (layout)			*layout			= NULL;

	/* Push the current font onto the stack and set to the desired font */
	saveFont = (NULL == formatCache)	? syncFont(textSize, textStyle, fontName, &font, state)
										: SelectObject(dc, formatCache->font);

	/* Generate the unicode representation of the text */
	FskTextUTF8ToUnicode16NENoAlloc(text, textLen, NULL, &uniTextLen);		/* uniTextLen is set to the number of bytes needed */
	BAIL_IF_ERR(err = FskMemPtrNew(uniTextLen, &uniText));
	FskTextUTF8ToUnicode16NENoAlloc(text, textLen, uniText, &uniTextLen);	/* uniTextLen is set to the number of bytes needed */
	uniTextLen /= sizeof(*uniText);											/* Convert from number of bytes to number of characters */

	/* Query the layout (TODO: replace deprecated API with appropriate Uniscribe API) */
	results.lStructSize	= sizeof(GCP_RESULTS);
	results.nGlyphs	= uniTextLen;
	results.nMaxFit	= 0;
//	BAIL_IF_ERR(err = FskMemPtrNew(uniTextLen * sizeof(*results.lpOutString),	&results.lpOutString));	// Unneeded
	BAIL_IF_ERR(err = FskMemPtrNew(uniTextLen * sizeof(*results.lpOrder),		&results.lpOrder));
	BAIL_IF_ERR(err = FskMemPtrNew(uniTextLen * sizeof(*results.lpDx),			&results.lpDx));
	BAIL_IF_ERR(err = FskMemPtrNew(uniTextLen * sizeof(*results.lpCaretPos),	&results.lpCaretPos));
	BAIL_IF_ERR(err = FskMemPtrNew(uniTextLen * sizeof(*results.lpClass),		&results.lpClass));
	BAIL_IF_ERR(err = FskMemPtrNew(uniTextLen * sizeof(*results.lpGlyphs),		&results.lpGlyphs));
	heightWidth = GetCharacterPlacementW(dc, uniText, uniTextLen, 0, &results, 0);
	BAIL_IF_ZERO(heightWidth, err, kFskErrOperationFailed);

	if (layout) {
		unsigned i;
		BAIL_IF_ERR(err = FskMemPtrNew(uniTextLen * sizeof(**layout), layout));
		for (i = 0; i < uniTextLen; ++i) {
			layout[0][i] = results.lpCaretPos[i] << 16;
			switch (results.lpClass[i]) {
				//case GCPCLASS_ARABIC:	/* right to left */
				case GCPCLASS_HEBREW:
					layout[0][i] -= results.lpDx[results.lpOrder[i]] << 16;
				default:				/* left to right */
					break;
			}
		}
	}
	if (unicodeText) {
		*unicodeText = uniText;
		uniText = NULL;
	}
	if (unicodeLen)
		*unicodeLen = uniTextLen;

bail:
	FskMemPtrDispose(results.lpGlyphs);
	FskMemPtrDispose(results.lpCaretPos);
	FskMemPtrDispose(results.lpDx);
	FskMemPtrDispose(results.lpOrder);
	FskMemPtrDispose(results.lpOutString);
	FskMemPtrDispose(uniText);
	if (NULL == formatCache) {
		SelectObject(dc, saveFont);
		DeleteObject(font);
	}

	return err;
}


#else /* USE_UNISCRIBE */


FskErr winTextGetLayout(FskTextEngineState state, FskBitmap bits, const char *text, UInt32 textLen, UInt32 textSize, UInt32 textStyle, const char *fontName,
						UInt16 **unicodeText, UInt32 *unicodeLen, FskFixed **layout, FskTextFormatCache formatCacheIn)
{
	FskErr					err			= kFskErrNone;
	HDC						dc			= (bits && bits->hdc) ? bits->hdc : state->dc;
	FskTextFormatCacheGDI	formatCache = (FskTextFormatCacheGDI)formatCacheIn;
	UInt16					*uniText	= NULL;
	UInt32					uniTextLen;
	UInt32					cmax_items;
	HFONT					font;
	HGDIOBJ					saveFont;
	UInt16					*textItem;

	SInt32					scriptItemCount;
	SCRIPT_ITEM				*scriptItems = NULL;
	SCRIPT_ITEM				*scriptItem = NULL;
	SCRIPT_VISATTR			*glyphVisattrs = NULL;
	WORD					*charCluster = NULL;

	UInt32					glyphCount = 0;
	WORD					*glyphs = NULL;
	SCRIPT_CACHE			scriptCache;
	int						*glyphAdvances = NULL;
	GOFFSET					*glyphOffsets = NULL;
	Boolean					resizeGlyphData;
	int						availableGlyphCount;
	HDC						dcParameter;
	ABC						runAbc;

	FskFixed				*item_layout;
	int						item_layout_begin = 0;
	int						item_width;
	int						start, length;
	HRESULT					hr;
	SInt32					i, j, k;

	/* Initialize return values for the error case */
	if (unicodeText)	*unicodeText	= NULL;
	if (unicodeLen)		*unicodeLen		= 0;
	if (layout)			*layout			= NULL;

	/* Push the current font onto the stack and set to the desired font */
	saveFont = (NULL == formatCache)	? syncFont(textSize, textStyle, fontName, &font, state)
										: SelectObject(dc, formatCache->font);

	/* Generate the unicode representation of the text */
	FskTextUTF8ToUnicode16NENoAlloc(text, textLen, NULL, &uniTextLen);		/* uniTextLen is set to the number of bytes needed */
	BAIL_IF_ERR(err = FskMemPtrNew(uniTextLen, &uniText));
	FskTextUTF8ToUnicode16NENoAlloc(text, textLen, uniText, &uniTextLen);	/* uniTextLen is set to the number of bytes needed */
	uniTextLen /= sizeof(*uniText);											/* Convert from number of bytes to number of characters */

	if(layout) {
		BAIL_IF_ERR(err = FskMemPtrNew(uniTextLen * sizeof(**layout), layout));
	}

	//allocate a pre-evaluated scriptItem memory
	cmax_items = uniTextLen + 2;
	BAIL_IF_ERR(err = FskMemPtrNew(cmax_items * sizeof(SCRIPT_ITEM), &scriptItems));

	//get the scriptItems from input string
	hr = ScriptItemize(uniText, uniTextLen, (cmax_items - 1), NULL, NULL, &scriptItems[0], &scriptItemCount);

	BAIL_IF_NONZERO(hr, err, kFskErrOperationFailed);

	//Resize the scriptItem array
	BAIL_IF_ERR(err = FskMemPtrRealloc((scriptItemCount + 1) * sizeof(SCRIPT_ITEM), &scriptItems));

	for(i = 0; i < scriptItemCount; i++) {
		scriptItem = &scriptItems[i];
		scriptCache = 0;
		start = scriptItem[0].iCharPos;
		length = scriptItem[1].iCharPos - scriptItem[0].iCharPos;

		textItem = uniText + start;
		//int currentStart = start;

		//Maybe have to add producing run here
		//while(currentStart < start + length) {
		//firstly allocate len * 1.5 + 16 for glyphs
		glyphCount = (UInt32)(length * 1.5 + 16);
		resizeGlyphData = true;

		BAIL_IF_ERR(err = FskMemPtrRealloc(glyphCount * sizeof(WORD), &glyphs));
		BAIL_IF_ERR(err = FskMemPtrRealloc(glyphCount * sizeof(SCRIPT_VISATTR), &glyphVisattrs));
		BAIL_IF_ERR(err = FskMemPtrRealloc(glyphCount * sizeof(WORD), &charCluster));
		BAIL_IF_ERR(err = FskMemPtrRealloc(glyphCount * sizeof(GOFFSET), &glyphOffsets));

		availableGlyphCount = 0;
		dcParameter = 0;
		if(scriptCache) {
			ScriptFreeCache(&scriptCache);
			scriptCache = 0;
		}

		while(true) {
			hr = ScriptShape((dcParameter ? dcParameter : NULL), &scriptCache, textItem, length, glyphCount, &scriptItem->a, &glyphs[0], &charCluster[0], &glyphVisattrs[0], &availableGlyphCount);

			//Resize the glyphs
			if(hr == 0) {
				glyphCount = availableGlyphCount;
				break;
			} else if (hr == E_PENDING) {
				dcParameter = dc;
			} else if (hr == E_OUTOFMEMORY) {
				if(resizeGlyphData)
					glyphCount += length;
				else
					goto bail;
			} else {
				goto bail;
			}
		}

		if(resizeGlyphData) {
			BAIL_IF_ERR(err = FskMemPtrRealloc(glyphCount * sizeof(WORD), &glyphs));
			BAIL_IF_ERR(err = FskMemPtrRealloc(glyphCount * sizeof(SCRIPT_VISATTR), &glyphVisattrs));
			BAIL_IF_ERR(err = FskMemPtrRealloc(length * sizeof(WORD), &charCluster));
			BAIL_IF_ERR(err = FskMemPtrRealloc(glyphCount * sizeof(int), &glyphAdvances));
			BAIL_IF_ERR(err = FskMemPtrRealloc(glyphCount * sizeof(GOFFSET), &glyphOffsets));
		}

		//generate the place information
		dcParameter = 0;
		while(true) {
			hr = ScriptPlace((dcParameter ? dcParameter : NULL), &scriptCache, &glyphs[0], glyphCount, &glyphVisattrs[0], &scriptItem->a, &glyphAdvances[0], &glyphOffsets[0], &runAbc);

			if(hr == 0) {
				break;
			} else if(hr == E_PENDING) {
				dcParameter = dc;
			} else {
				goto bail;
			}
		}
		//}

		item_layout = (FskFixed *)&layout[0][start];

		//Set the text's glyphs
		//if LTR script or RTL script
		if(scriptItem->a.fRTL == 0) {
			//the first text
			item_layout[0] = (item_layout_begin << 16);

			for(j = 1; j < length; j++) {
				item_width = 0;
				for(k = charCluster[j-1]; k < charCluster[j]; k++) {
					//Should have the dv to impress the Y-offset of combination glyph
					item_width += (glyphAdvances[k] + glyphOffsets[k].du) << 16;
				}
				item_layout[j] = item_layout[j-1] + item_width;
			}

			/*
			//the last text
			if(charCluster[j] != charCluster[j-1]) {
				item_width = 0;
				for(k = charCluster[j]; k < glyphCount; k++) {
					item_width += (glyphAdvances[k] + glyphOffsets[k].du) << 16;
				}
			} else {
			}
			*/

			item_layout_begin += runAbc.abcA + runAbc.abcB + runAbc.abcC;
		} else if(scriptItem->a.fRTL == 1) {
			//RTL script
			item_layout_begin = runAbc.abcA + runAbc.abcB + runAbc.abcC;
			item_width = 0;

			for(k = charCluster[1]; k < charCluster[0]; k++) {
				item_width += (glyphAdvances[k] + glyphOffsets[k].du);
			}
			item_layout[0] = (item_layout_begin - item_width) << 16;

			for(j = 1; j < length; j++) {
				item_width = 0;
				for(k = charCluster[j+1]; k < charCluster[j]; k++) {
					item_width += (glyphAdvances[k] + glyphOffsets[k].du);
				}
				item_layout[j] = item_layout[j-1] - (item_width << 16);
			}
		} else {
			//Error direction value
			goto bail;
		}
	}

	if (unicodeText) {
		*unicodeText = uniText;
		uniText = NULL;
	}
	if (unicodeLen)
		*unicodeLen = uniTextLen;

bail:
	FskMemPtrDispose(uniText);
	FskMemPtrDispose(scriptItems);
	FskMemPtrDispose(glyphVisattrs);
	FskMemPtrDispose(glyphs);
	FskMemPtrDispose(charCluster);
	FskMemPtrDispose(glyphOffsets);
	if (NULL == formatCache) {
		SelectObject(dc, saveFont);
		DeleteObject(font);
	}

	return err;
}

#endif /* USE_UNISCRIBE */


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

FskErr winTextFitWidth(FskTextEngineState state, FskBitmap bits, const char *text, UInt32 textLen, UInt32 textSize, UInt32 textStyle, const char *fontName, UInt32 width, UInt32 flags, UInt32 *fitBytesOut, UInt32 *fitCharsOut, FskTextFormatCache formatCacheIn)
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
	info->ascent  = metrics.tmAscent;
	info->descent = metrics.tmDescent;
	info->leading = metrics.tmExternalLeading;
	info->width   = metrics.tmMaxCharWidth;

	if ((kFskTextOutline | kFskTextOutlineHeavy) & textStyle) {
		SInt32 outlineWidth = 1;
		if (kFskTextOutlineHeavy & textStyle)
			outlineWidth = 2;
		info->ascent  += outlineWidth;
		info->descent += outlineWidth;
		info->width   += outlineWidth;
	}
	info->height = info->ascent + info->descent + info->leading;

	if (NULL == formatCache) {
		SelectObject(dc, saveFont);
		DeleteObject(font);
	}

	return kFskErrNone;
}

FskErr winTextFormatCacheNew(FskTextEngineState state, FskTextFormatCache *cacheOut, FskBitmap bits, UInt32 textSize, UInt32 textStyle, const char *fontName)
{
	FskErr err = kFskErrNone;
	HFONT font, saveFont;

	saveFont = (HFONT)syncFont(textSize, textStyle, fontName, &font, state);
	if (NULL == saveFont) {
		BAIL(kFskErrOperationFailed);
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
