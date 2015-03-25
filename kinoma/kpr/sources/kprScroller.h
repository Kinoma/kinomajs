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
#ifndef __KPRSCROLLER__
#define __KPRSCROLLER__

#include "kpr.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct {
	KprSlotPart;
	KprContentPart;
	KprContainerPart;
	FskPointRecord delta;
} KprScrollerRecord, *KprScroller;

FskAPI(FskErr) KprScrollerNew(KprScroller *it, KprCoordinates coordinates, KprSkin skin, KprStyle style);
FskAPI(void) KprScrollerConstraint(KprScroller self, FskPoint delta);
FskAPI(void) KprScrollerLoop(KprScroller self, Boolean loopIt);
FskAPI(void) KprScrollerPredictBy(KprScroller self, SInt32 dx, SInt32 dy);
FskAPI(void) KprScrollerPredictTo(KprScroller self, SInt32 x, SInt32 y);
FskAPI(void) KprScrollerReveal(KprScroller self, FskRectangle bounds);
FskAPI(void) KprScrollerScrollBy(KprScroller self, SInt32 dx, SInt32 dy);
FskAPI(void) KprScrollerScrollTo(KprScroller self, SInt32 x, SInt32 y);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
