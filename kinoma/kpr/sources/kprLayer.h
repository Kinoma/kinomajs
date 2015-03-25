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
#ifndef __KPRLAYER__
#define __KPRLAYER__

#include "kpr.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct KprLayerStruct {
	KprSlotPart;
	KprContentPart;
	KprContainerPart;
	KprLayerPart;
	FskPort port;
	FskBitmap bitmap;
	UInt32 error;
};

FskAPI(FskErr) KprLayerNew(KprLayer *self, KprCoordinates coordinates, UInt32 flags);

FskAPI(void) KprLayerAttach(KprLayer self, KprContent content);
FskAPI(void) KprLayerBlit(void* it, FskPort port, FskBitmap srcBits, FskRectangle srcRect, FskRectangle dstRect);
FskAPI(FskErr) KprLayerCapture(KprLayer self, KprContent content, FskBitmap* bitmap);
FskAPI(void) KprLayerComputeMatrix(KprLayer self);
FskAPI(KprContent) KprLayerDetach(KprLayer self);
FskAPI(Boolean) KprLayerGLContextLost(KprLayer self);
FskAPI(void) KprLayerMatrixChanged(KprLayer self);
FskAPI(void) KprLayerSetEffect(KprLayer self, KprEffect effect);

extern void KprLayerInvalidated(void* it, FskRectangle area);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
