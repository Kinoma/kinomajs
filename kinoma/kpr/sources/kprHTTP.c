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
#include "FskUUID.h"

#include "kpr.h"
#include "kprHTTPCache.h"
#include "kprHTTPClient.h"
#include "kprHTTPCookies.h"
#include "kprHTTPKeychain.h"
#include "kprUtilities.h"
#include "kprURL.h"

/* HTTP CACHE */

FskAPI(KprHTTPCache) KprHTTPClientCache();

void KPR_HTTP_Cache_clear(xsMachine *the UNUSED)
{
	KprHTTPCache self = KprHTTPClientCache();
	KprHTTPCacheClear(self);
}

/* HTTP COOKIES */

FskAPI(KprHTTPCookies) KprHTTPClientCookies();
FskAPI(void) KprHTTPClientCookiesClear();
FskAPI(FskErr) KprHTTPClientCookiesGet(char* url, char** it);
FskAPI(FskErr) KprHTTPClientCookiesPut(char* url, char* cookies);

void KPR_HTTP_Cookies_clear(xsMachine *the UNUSED)
{
	KprHTTPCookies self = KprHTTPClientCookies();
	KprHTTPCookiesClear(self);
}

void KPR_HTTP_Cookies_get(xsMachine *the)
{
	KprHTTPCookies self = KprHTTPClientCookies();
	char* url = xsToString(xsArg(0));
	char* cookies = NULL;
	KprHTTPCookiesGet(self, url, &cookies);
	if (cookies)
		xsResult = xsString(cookies);
	else
		xsResult = xsString("");
}

void KPR_HTTP_Cookies_set(xsMachine *the)
{
	KprHTTPCookies self = KprHTTPClientCookies();
	char* url = xsToString(xsArg(0));
	char* cookies = xsToString(xsArg(1));
	KprHTTPCookiesPut(self, url, cookies);
}

/* HTTP KEYCHAIN */

void KPR_HTTP_Keychain_clear(xsMachine *the UNUSED)
{
	KprHTTPKeychain self = KprHTTPClientKeychain();
	KprHTTPKeychainClear(self);
}

void KPR_HTTP_Keychain_get(xsMachine *the)
{
	KprHTTPKeychain self = KprHTTPClientKeychain();
	char* user = NULL;
	char* password = NULL;
	xsTry {
		xsStringValue host = xsToString(xsArg(0));
		xsStringValue realm = xsToString(xsArg(1));
		KprHTTPKeychainGet(self, host, realm, &user, &password);
		if (user && password) {
			xsResult = xsNewInstanceOf(xsObjectPrototype);
			xsEnterSandbox();
			xsSet(xsResult, xsID("user"), xsString(user));
			xsSet(xsResult, xsID("password"), xsString(password));
			xsLeaveSandbox();
			FskMemPtrDispose(user);
			FskMemPtrDispose(password);
		}
	}
	xsCatch {
		FskMemPtrDispose(user);
		FskMemPtrDispose(password);
		xsThrow(xsException);
	}
}

void KPR_HTTP_Keychain_remove(xsMachine *the)
{
	KprHTTPKeychain self = KprHTTPClientKeychain();
	xsStringValue host = xsToString(xsArg(0));
	xsStringValue realm = xsToString(xsArg(1));
	KprHTTPKeychainRemove(self, host, realm);
}

void KPR_HTTP_Keychain_set(xsMachine *the)
{
	KprHTTPKeychain self = KprHTTPClientKeychain();
	xsStringValue host = xsToString(xsArg(0));
	xsStringValue realm = xsToString(xsArg(1));
	xsStringValue user = xsToString(xsArg(2));
	xsStringValue password = xsToString(xsArg(3));
	xsThrowIfFskErr(KprHTTPKeychainPut(self, host, realm, user, password));
}


//--------------------------------------------------
//--------------------------------------------------
/* CRYPT */
//--------------------------------------------------
//--------------------------------------------------

void KPR_MD5(xsMachine *the)
{
	char buffer[33];	
	if (xsStringType == xsTypeOf(xsArg(0))) {
		char *str = xsToString(xsArg(0));
		KprCryptMD5((unsigned char *)str, FskStrLen(str), NULL, buffer);
	}
	else
		KprCryptMD5(xsGetHostData(xsArg(0)), xsToInteger(xsGet(xsArg(0), xsID("length"))), NULL, buffer);
	xsResult = xsString(buffer);
}



