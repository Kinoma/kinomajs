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
/**
 *	\file	kprEffect.h
 *	\brief	Visual effects.
 *
 *			Effects are procedures that take an original bitmap and return a new bitmap.
 *			Firstly the new bitmap is created by copying the original bitmap.
 *			Here under the destination pixels are the pixels of the new bitmap.
 *			There are three kinds of effects.
 *			The first kind uses no mattes.
 *			The second kind computes a temporary matte.
 *			The third kind uses another bitmap as a matte.
 *
 *	\author	Patrick Soquet, 10/2011
 *
 *	\par	Copyright
 *			Copyright (C) 2011 Marvell Semiconductor, Inc.
 **/

#ifndef __KPREFFECT__
#define __KPREFFECT__

#include "kpr.h"
#include "FskEffects.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


struct KprEffectStruct {
	KprSlotPart;
	KprAssetPart;
	FskEffect compound;
};

FskAPI(FskErr) KprEffectNew(KprEffect *it, KprContext context);
FskAPI(void) KprEffectAdd(KprEffect self, FskEffect step);
FskAPI(FskErr) KprEffectApply(KprEffect self, FskBitmap src, FskPort port, FskBitmap *result);
FskAPI(void) KprEffectExtend(KprEffect self, FskRectangle bounds);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __KPREFFECT__ */
