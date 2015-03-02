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
#ifndef __KPRSTYLE__
#define __KPRSTYLE__

#include "kpr.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

enum {
	kprColor0 = 1 << 0,
	kprColor1 = 1 << 1,
	kprColor2 = 1 << 2,
	kprColor3 = 1 << 3,
	kprHorizontalAlignment = 1 << 4,
	kprVerticalAlignment = 1 << 5,
	kprIndentation = 1 << 6,
	kprLineCount = 1 << 7,
	kprLineHeight = 1 << 8,
	kprMarginLeft = 1 << 9,
	kprMarginTop = 1 << 10,
	kprMarginRight = 1 << 11,
	kprMarginBottom = 1 << 12,
	kprTextFont = 1 << 13,
	kprTextSize = 1 << 14,
	kprTextStyle = 1 << 15,
	kprStyleInherited  = 1 << 30,
	kprStyleDirty = 1 << 31
};

struct KprStyleStruct {
	KprSlotPart;
	KprAssetPart;
	
	KprStyle father;
	KprStyle mother;
	KprStyle firstChild;
	KprStyle nextChild;
	
	UInt32 flags;
	FskColorRGBARecord colors[4];
	UInt16 horizontalAlignment;
	UInt16 verticalAlignment;
	UInt32 indentation;
	UInt32 lineCount;
	UInt32 lineHeight;
	KprMarginsRecord margins;
	char* textFont;
	SInt32 textSize;
	UInt32 textStyle;
	FskPortTextFormat textFormat;
	UInt16 ascent; 
	UInt16 descent;
};

FskAPI(FskErr) KprStyleNew(KprStyle *it, KprContext context, KprStyle father, KprStyle mother);
FskAPI(void) KprStyleApply(KprStyle self, FskPort port);
FskAPI(KprStyle) KprStyleCascade(KprStyle self, KprStyle father);
FskAPI(void) KprStyleClearHorizontalAlignment(KprStyle self);
FskAPI(void) KprStyleClearVerticalAlignment(KprStyle self);
FskAPI(void) KprStyleClearColor(KprStyle self, SInt32 which);
FskAPI(void) KprStyleClearIndentation(KprStyle self);
FskAPI(void) KprStyleClearLineCount(KprStyle self);
FskAPI(void) KprStyleClearLineHeight(KprStyle self);
FskAPI(void) KprStyleClearMarginLeft(KprStyle self);
FskAPI(void) KprStyleClearMarginTop(KprStyle self);
FskAPI(void) KprStyleClearMarginRight(KprStyle self);
FskAPI(void) KprStyleClearMarginBottom(KprStyle self);
FskAPI(void) KprStyleClearTextFont(KprStyle self);
FskAPI(void) KprStyleClearTextSize(KprStyle self);
FskAPI(void) KprStyleClearTextStyle(KprStyle self);
FskAPI(Boolean) KprStyleColorize(KprStyle self, FskPort port, double state);
FskAPI(void) KprStyleGetInfo(KprStyle self, UInt16* ascent, UInt16* descent);
FskAPI(void) KprStyleInherit(KprStyle self);
FskAPI(void) KprStyleMeasure(KprStyle self, char* string, FskRectangle bounds);
FskAPI(void) KprStyleSetHorizontalAlignment(KprStyle self, UInt16 alignment);
FskAPI(void) KprStyleSetVerticalAlignment(KprStyle self, UInt16 alignment);
FskAPI(void) KprStyleSetColor(KprStyle self, SInt32 which, FskColorRGBA color);
FskAPI(void) KprStyleSetIndentation(KprStyle self, UInt32 indentation);
FskAPI(void) KprStyleSetLineCount(KprStyle self, UInt32 lineCount);
FskAPI(void) KprStyleSetLineHeight(KprStyle self, UInt32 lineHeight);
FskAPI(void) KprStyleSetMarginLeft(KprStyle self, SInt32 marginLeft);
FskAPI(void) KprStyleSetMarginTop(KprStyle self, SInt32 marginTop);
FskAPI(void) KprStyleSetMarginRight(KprStyle self, SInt32 marginRight);
FskAPI(void) KprStyleSetMarginBottom(KprStyle self, SInt32 marginBottom);
FskAPI(void) KprStyleSetTextFont(KprStyle self, char* textFont);
FskAPI(void) KprStyleSetTextSize(KprStyle self, SInt32 textSize);
FskAPI(void) KprStyleSetTextStyle(KprStyle self, UInt32 textStyle);
FskAPI(KprStyle) KprStyleUncascade(KprStyle self);

FskAPI(FskErr) KprShellDefaultStyles(KprShell self);
FskAPI(void) KprShellUpdateStyles(KprShell self);

FskAPI(Boolean) KprParseHorizontalAlignment(char* s, UInt16 *alignment);
FskAPI(Boolean) KprParseVerticalAlignment(char* s, UInt16 *alignment);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
