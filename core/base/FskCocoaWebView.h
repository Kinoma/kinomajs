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
#ifndef __FSKCOCOAWEBVIEW__
#define __FSKCOCOAWEBVIEW__

#include "FskWindow.h"
#include "FskBrowser.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

	// Webview
	struct FskCocoaWebViewRecord;
	typedef struct FskCocoaWebViewRecord *FskCocoaWebView;
	
	FskAPI(FskErr) FskCocoaWebViewCreate(FskBrowser browser);
	FskAPI(void) FskCocoaWebViewDispose(FskCocoaWebView fskBrowser);
	
	FskAPI(void) FskCocoaWebViewActivated(FskCocoaWebView fskWebView, Boolean activeIt);
	FskAPI(void) FskCocoaWebViewSetFrame(FskCocoaWebView fskWebView, FskRectangle area);
	FskAPI(void) FskCocoaWebViewAttach(FskCocoaWebView fskWebView, FskWindow fskWindow);
	FskAPI(void) FskCocoaWebViewDetach(FskCocoaWebView fskWebView);
	
	FskAPI(char *) FskCocoaWebViewGetURL(FskCocoaWebView fskWebView);
	FskAPI(void) FskCocoaWebViewSetURL(FskCocoaWebView fskWebView, char *url);
	FskAPI(char *) FskCocoaWebViewEvaluateScript(FskCocoaWebView fskWebView, char *script);
	FskAPI(void) FskCocoaWebViewReload(FskCocoaWebView browser);
	FskAPI(void) FskCocoaWebViewBack(FskCocoaWebView browser);
	FskAPI(void) FskCocoaWebViewForward(FskCocoaWebView browser);
	FskAPI(Boolean) FskCocoaWebViewCanBack(FskCocoaWebView browser);
	FskAPI(Boolean) FskCocoaWebViewCanForward(FskCocoaWebView browser);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __FSKCOCOAWEBVIEW__ */

