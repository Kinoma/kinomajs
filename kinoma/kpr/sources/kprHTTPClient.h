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
#ifndef __KPRHTTPCLIENT__
#define __KPRHTTPCLIENT__

#include "kpr.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define kprHTTPAcceptLanguage "Accept-Language"
#define kprHTTPHeaderAge "Age"
#define kprHTTPHeaderContentEncoding "Content-Encoding"
#define kprHTTPHeaderContentLanguage "Content-Language"
#define kprHTTPHeaderContentRange "Content-Range"
#define kprHTTPHeaderCookie "Cookie"
#define kprHTTPHeaderDate "Date"
#define kprHTTPHeaderETag "ETag"
#define kprHTTPHeaderIfNoneMatch "If-None-Match"
#define kprHTTPHeaderIfModifiedSince "If-Modified-Since"
#define kprHTTPHeaderLastModified "Last-Modified"
#define kprHTTPHeaderSetCookie "Set-Cookie"
#define kprHTTPHeaderVary "Vary"

#define kprHTTPMessageDefaultSize 32768

/* HTTP CLIENT */

FskAPI(FskThread) KprHTTPGetThread();
FskAPI(FskErr) KprHTTPStart(xsMachine* root);
FskAPI(void) KprHTTPStop();
FskAPI(KprHTTPCache) KprHTTPClientCache();
FskAPI(KprHTTPCookies) KprHTTPClientCookies();
FskAPI(FskErr) KprHTTPClientCookiesGet(char* url, char** it);
FskAPI(KprHTTPKeychain) KprHTTPClientKeychain();
FskAPI(xsMachine*) KprHTTPClientMachine();

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
