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
#ifndef __KPL_SCREEN_H__
#define __KPL_SCREEN_H__

#include "FskErrors.h"
#include "KplBitmap.h"
#include "KplRectangle.h"

#ifdef __cplusplus
extern "C" {
#endif

FskErr KplScreenGetBitmap(KplBitmap *bitmap);
FskErr KplScreenDisposeBitmap(KplBitmap bitmap);

FskErr KplScreenLockBitmap(KplBitmap bitmap);
FskErr KplScreenUnlockBitmap(KplBitmap bitmap);

FskErr KplScreenDrawBitmap(KplBitmap src, KplRectangle srcRect, KplBitmap screen, KplRectangle dstRect);

FskErr KplScreenGetAuxInfo(unsigned char **auxInfo, UInt32 *auxInfoSize);

void   KplScreenEventWakeUp(void);

#ifdef __cplusplus
}
#endif

#endif
