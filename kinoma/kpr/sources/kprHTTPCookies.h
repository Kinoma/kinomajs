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
#ifndef __KPRHTTPCOOKIES__
#define __KPRHTTPCOOKIES__

#include "kpr.h"
#include "kprStorage.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct KprHTTPCookiesStruct {
	KprStorage table;
	char* path;
};

FskAPI(FskErr) KprHTTPCookiesNew(KprHTTPCookies* it, UInt32 size, char* path);
FskAPI(void) KprHTTPCookiesDispose(KprHTTPCookies self);

FskAPI(void) KprHTTPCookiesCleanup(KprHTTPCookies self, Boolean session);
FskAPI(void) KprHTTPCookiesClear(KprHTTPCookies self);
FskAPI(FskErr) KprHTTPCookiesGet(KprHTTPCookies self, char* url, char** it);
FskAPI(FskErr) KprHTTPCookiesPut(KprHTTPCookies self, char* url, char* cookies);
FskAPI(void) KprHTTPCookiesRemove(KprHTTPCookies self, char* url);
FskAPI(void) KprHTTPCookiesRead(KprHTTPCookies self);
FskAPI(FskErr) KprHTTPCookiesWrite(KprHTTPCookies self);

struct KprHTTPCookieStruct {
	char* name;
	char* value;
	char* path;
	UInt32 expire;
	Boolean secure;
	Boolean httpOnly;
	FskInstrumentedItemDeclaration
};

FskAPI(FskErr) KprHTTPCookieNew(KprHTTPCookie* it);
FskAPI(void) KprHTTPCookieDispose(KprHTTPCookie self);

FskAPI(FskErr) KprHTTPCookieParseString(KprHTTPCookie* it, char* url, char* cookie, char** key);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
