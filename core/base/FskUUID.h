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
#ifndef __FSKUUID__
#define __FSKUUID__

#include "Fsk.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct FskUUIDRecord {
	UInt8 value[16];
} FskUUIDRecord, *FskUUID;

FskAPI(FskErr) FskUUIDCreate(FskUUID uuid);
FskAPI(char *) FskUUIDtoString(FskUUID uuid);
FskAPI(char *) FskUUIDtoString_844412(FskUUID uuid);

FskAPI(char *) FskUUIDGetForKey(const char *key);

#ifdef __FSKUUID_PRIV__
	void FskUUIDTerminate(void);
#endif
	
#ifdef __cplusplus
}
#endif

#endif
