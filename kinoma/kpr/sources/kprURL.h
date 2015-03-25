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
#ifndef __KPRURL__
#define __KPRURL__

#include "kpr.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct KprURLPartsStruct {
	char* scheme;
	UInt32 schemeLength;
	char* authority;
	UInt32 authorityLength;
	char* user;
	UInt32 userLength;
	char* password;
	UInt32 passwordLength;
	char* host;
	UInt32 hostLength;
	UInt32 port;
	char* path;
	UInt32 pathLength;
	char* name;
	UInt32 nameLength;
	char* query;
	UInt32 queryLength;
	char* fragment;
	UInt32 fragmentLength;
};

FskAPI(FskErr) KprURLJoin(KprURLParts parts, char** result);
FskAPI(FskErr) KprURLMerge(char* base, char* reference, char** result);
FskAPI(void) KprURLSplit(char* url, KprURLParts parts);
FskAPI(FskErr) KprURLToPath(char* url, char** result);
FskAPI(FskErr) KprPathToURL(char* path, char** result);

FskAPI(FskErr) KprQueryParse(char* query, FskAssociativeArray* array);
FskAPI(FskErr) KprQuerySerialize(FskAssociativeArray array, char** query);

FskAPI(FskErr) KprAuthorityReverse(char* id, char** di);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
