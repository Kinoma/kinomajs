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
#ifndef __FSKBROWSER__
#define __FSKBROWSER__

#include "FskWindow.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

	typedef struct FskBrowserRecord *FskBrowser;
	
	typedef void (*FskBrowserGeneralCallback)(FskBrowser browser, void *refcon);
	typedef Boolean (*FskBrowserShouldHandleURLCallback)(FskBrowser browser, char *url, void *refcon);
	
	// Webview
	struct FskBrowserRecord {
#if TARGET_OS_MAC
		void *native;
#elif TARGET_OS_ANDROID
		char *url;
#endif
		void *refcon;
		FskBrowserGeneralCallback didStartLoadCallback;
		FskBrowserGeneralCallback didLoadCallback;
		FskBrowserShouldHandleURLCallback shouldHandleURLCallback;
	};
	
	FskAPI(FskErr) FskBrowserCreate(FskBrowser *fskBrowser, void *refcon);
	FskAPI(void) FskBrowserDispose(FskBrowser *fskBrowser);

	FskAPI(void) FskBrowserActivated(FskBrowser browser, Boolean activeIt);
	FskAPI(void) FskBrowserSetFrame(FskBrowser browser, FskRectangle area);
	FskAPI(void) FskBrowserAttach(FskBrowser browser, FskWindow window);
	FskAPI(void) FskBrowserDetach(FskBrowser browser);
	
	FskAPI(char *) FskBrowserGetURL(FskBrowser browser);
	FskAPI(void) FskBrowserSetURL(FskBrowser browser, char *url);
	FskAPI(char *) FskBrowserEvaluateScript(FskBrowser browser, char *script);
	FskAPI(void) FskBrowserReload(FskBrowser browser);
	FskAPI(void) FskBrowserBack(FskBrowser browser);
	FskAPI(void) FskBrowserForward(FskBrowser browser);
	FskAPI(Boolean) FskBrowserCanBack(FskBrowser browser);
	FskAPI(Boolean) FskBrowserCanForward(FskBrowser browser);
	
	FskAPI(void) FskBrowserSetDidStartLoadCallback(FskBrowser browser, FskBrowserGeneralCallback callback);
	FskAPI(void) FskBrowserInvokeDidStartLoadCallback(FskBrowser browser);
	
	FskAPI(void) FskBrowserSetDidLoadCallback(FskBrowser browser, FskBrowserGeneralCallback callback);
	FskAPI(void) FskBrowserInvokeDidLoadCallback(FskBrowser browser);

	FskAPI(void) FskBrowserSetShouldHandleURLCallback(FskBrowser browser, FskBrowserShouldHandleURLCallback callback);
	FskAPI(Boolean) FskBrowserInvokeShouldHandleURLCallback(FskBrowser browser, char *url);
	
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __FSKBROWSER__ */

