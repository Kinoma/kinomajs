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
#ifndef __KPRTABLE__
#define __KPRTABLE__

#include "kpr.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct {
	KprSlotPart;
	KprContentPart;
	KprContainerPart;
	SInt32 portion;
	SInt32 sum;
} KprColumnRecord, *KprColumn;

FskAPI(FskErr) KprColumnNew(KprColumn *it, KprCoordinates coordinates, KprSkin skin, KprStyle style);

typedef struct {
	KprSlotPart;
	KprContentPart;
	KprContainerPart;
	SInt32 portion;
	SInt32 sum;
} KprLineRecord, *KprLine;

FskAPI(FskErr) KprLineNew(KprLine *it, KprCoordinates coordinates, KprSkin skin, KprStyle style);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
