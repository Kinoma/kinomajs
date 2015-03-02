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
#ifndef __KPRHTTPKEYCHAIN__
#define __KPRHTTPKEYCHAIN__

#include "kpr.h"
#include "kprStorage.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct KprHTTPKeychainStruct {
	KprStorage table;
	char* path;
};

FskAPI(FskErr) KprHTTPKeychainNew(KprHTTPKeychain* it, UInt32 size, char* path);
FskAPI(void) KprHTTPKeychainDispose(KprHTTPKeychain self);

FskAPI(void) KprHTTPKeychainClear(KprHTTPKeychain self);
FskAPI(void) KprHTTPKeychainGet(KprHTTPKeychain self, char* host, char* realm, char** user, char** password);
FskAPI(FskErr) KprHTTPKeychainPut(KprHTTPKeychain self, char* host, char* realm, char* user, char* password);
FskAPI(void) KprHTTPKeychainRemove(KprHTTPKeychain self, char* host, char* realm);
FskAPI(void) KprHTTPKeychainRead(KprHTTPKeychain self);
FskAPI(FskErr) KprHTTPKeychainWrite(KprHTTPKeychain self);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
