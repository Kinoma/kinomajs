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
#ifndef __KPRLABEL__
#define __KPRLABEL__

#include "kpr.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct KprLabelStruct {
	KprSlotPart;
	KprContentPart;
	char* text;
	UInt32 length;
	UInt32 from;
	UInt32 to;
	SInt32 left;
	SInt32 right;
	FskTimeCallBack showLastCallback;
};

FskAPI(FskErr) KprLabelNew(KprLabel *it, KprCoordinates coordinates, KprSkin skin, KprStyle style, char* text);
FskAPI(void) KprLabelFlagged(KprLabel self);
FskAPI(void) KprLabelGetSelectionBounds(KprLabel self, FskRectangle bounds);
UInt32 KprLabelHitOffset(KprLabel self, SInt32 x, SInt32 y);
FskAPI(FskErr) KprLabelInsertString(KprLabel self, char* string);
FskAPI(FskErr) KprLabelInsertStringWithLength(KprLabel self, char* text, UInt32 length);
FskAPI(FskErr) KprLabelSelect(KprLabel self, SInt32 selectionOffset, UInt32 selectionLength);
FskAPI(FskErr) KprLabelSetString(KprLabel self, char* string);
FskAPI(FskErr) KprLabelSetText(KprLabel self, char* text, UInt32 length);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
