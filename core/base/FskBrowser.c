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
#include "FskBrowser.h"

#if TARGET_OS_MAC
#include "FskCocoaWebView.h"
#elif TARGET_OS_ANDROID
#include "FskHardware.h"
#endif

FskErr FskBrowserCreate(FskBrowser *browser, void *refcon)
{
	FskErr err = kFskErrNone;
	
	err = FskMemPtrNewClear(sizeof(struct FskBrowserRecord), browser);
	BAIL_IF_ERR(err);
	
#if TARGET_OS_MAC
	{
		err = FskCocoaWebViewCreate(*browser);
		BAIL_IF_ERR(err);
	}
#elif TARGET_OS_ANDROID
	{
		err = gAndroidCallbacks->webviewCreateCB(*browser);
		BAIL_IF_ERR(err);
	}
#endif
	(*browser)->refcon = refcon;
	return err;
bail:
	FskMemPtrDispose(*browser);
	*browser = NULL;
	return err;
}

void FskBrowserDispose(FskBrowser *browser)
{
#if TARGET_OS_MAC
	FskCocoaWebViewDispose((FskCocoaWebView)(*browser)->native);
#elif TARGET_OS_ANDROID
	gAndroidCallbacks->webviewDisposeCB(*browser);
#endif
	FskMemPtrDispose(*browser);
	*browser = NULL;
}

void FskBrowserActivated(FskBrowser browser, Boolean activeIt)
{
#if TARGET_OS_MAC
	FskCocoaWebView webview = (FskCocoaWebView)browser->native;
	FskCocoaWebViewActivated(webview, activeIt);
#elif TARGET_OS_ANDROID
	gAndroidCallbacks->webviewActivatedCB(browser, activeIt);
#endif
}

void FskBrowserSetFrame(FskBrowser browser, FskRectangle area)
{
#if TARGET_OS_MAC
	FskCocoaWebView webview = (FskCocoaWebView)browser->native;
	FskCocoaWebViewSetFrame(webview, area);
#elif TARGET_OS_ANDROID
	gAndroidCallbacks->webviewSetFrameCB(browser, area);
#endif
}

void FskBrowserAttach(FskBrowser browser, FskWindow window)
{
#if TARGET_OS_MAC
	FskCocoaWebView webview = (FskCocoaWebView)browser->native;
	FskCocoaWebViewAttach(webview, window);
#elif TARGET_OS_ANDROID
	gAndroidCallbacks->webviewAttachCB(browser, window);
#endif
}

void FskBrowserDetach(FskBrowser browser)
{
#if TARGET_OS_MAC
	FskCocoaWebView webview = (FskCocoaWebView)browser->native;
	FskCocoaWebViewDetach(webview);
#elif TARGET_OS_ANDROID
	gAndroidCallbacks->webviewDetachCB(browser);
#endif
}

char *FskBrowserGetURL(FskBrowser browser)
{
	char *url = NULL;
	
#if TARGET_OS_MAC
	FskCocoaWebView webview = (FskCocoaWebView)browser->native;
	url = FskCocoaWebViewGetURL(webview);
#elif TARGET_OS_ANDROID
	url = gAndroidCallbacks->webviewGetURLCB(browser);
#endif
	return url;
}

void FskBrowserSetURL(FskBrowser browser, char *url)
{
#if TARGET_OS_MAC
	FskCocoaWebView webview = (FskCocoaWebView)browser->native;
	FskCocoaWebViewSetURL(webview, url);
#elif TARGET_OS_ANDROID
	gAndroidCallbacks->webviewSetURLCB(browser, url);
#endif
}

char *FskBrowserEvaluateScript(FskBrowser browser, char *script)
{
	char *result = NULL;
	
#if TARGET_OS_MAC
	FskCocoaWebView webview = (FskCocoaWebView)browser->native;
	result = FskCocoaWebViewEvaluateScript(webview, script);
#elif TARGET_OS_ANDROID
	result = gAndroidCallbacks->webviewEvaluateScriptCB(browser, script);
#endif
	return result;
}


void FskBrowserReload(FskBrowser browser)
{
#if TARGET_OS_MAC
	FskCocoaWebView webview = (FskCocoaWebView)browser->native;
	FskCocoaWebViewReload(webview);
#elif TARGET_OS_ANDROID
	gAndroidCallbacks->webviewReloadCB(browser);
#endif
}

void FskBrowserBack(FskBrowser browser)
{
#if TARGET_OS_MAC
	FskCocoaWebView webview = (FskCocoaWebView)browser->native;
	FskCocoaWebViewBack(webview);
#elif TARGET_OS_ANDROID
	gAndroidCallbacks->webviewBackCB(browser);
#endif
}

void FskBrowserForward(FskBrowser browser)
{
#if TARGET_OS_MAC
	FskCocoaWebView webview = (FskCocoaWebView)browser->native;
	FskCocoaWebViewForward(webview);
#elif TARGET_OS_ANDROID
	gAndroidCallbacks->webviewForwardCB(browser);
#endif
}

Boolean FskBrowserCanBack(FskBrowser browser)
{
	Boolean result = false;
#if TARGET_OS_MAC
	FskCocoaWebView webview = (FskCocoaWebView)browser->native;
	result = FskCocoaWebViewCanBack(webview);
#elif TARGET_OS_ANDROID
	result = gAndroidCallbacks->webviewCanBackCB(browser);
#endif
	return result;
}

Boolean FskBrowserCanForward(FskBrowser browser){
	Boolean result = false;
#if TARGET_OS_MAC
	FskCocoaWebView webview = (FskCocoaWebView)browser->native;
	result = FskCocoaWebViewCanForward(webview);
#elif TARGET_OS_ANDROID
	result = gAndroidCallbacks->webviewCanForwardCB(browser);
#endif
	return result;
}

void FskBrowserSetDidStartLoadCallback(FskBrowser browser, FskBrowserGeneralCallback callback)
{
	browser->didStartLoadCallback = callback;
}

void FskBrowserInvokeDidStartLoadCallback(FskBrowser browser)
{
	if (browser->didStartLoadCallback) {
		browser->didStartLoadCallback(browser, browser->refcon);
	}
}

void FskBrowserSetDidLoadCallback(FskBrowser browser, FskBrowserGeneralCallback callback)
{
	browser->didLoadCallback = callback;
}

void FskBrowserInvokeDidLoadCallback(FskBrowser browser)
{
	if (browser->didLoadCallback) {
		browser->didLoadCallback(browser, browser->refcon);
	}
}

void FskBrowserSetShouldHandleURLCallback(FskBrowser browser, FskBrowserShouldHandleURLCallback callback)
{
	browser->shouldHandleURLCallback = callback;
}

Boolean FskBrowserInvokeShouldHandleURLCallback(FskBrowser browser, char *url)
{
	Boolean handle;
	if (browser->shouldHandleURLCallback) {
		handle = browser->shouldHandleURLCallback(browser, url, browser->refcon);
	} else {
		handle = true;
	}

	return handle;
}

