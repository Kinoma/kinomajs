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
#include "FskExtensions.h"

#define __FSKBITMAP_PRIV__
#define __FSKPORT_PRIV__

#include "kprBehavior.h"
#include "kprContent.h"
#include "kprEffect.h"
#include "kprBrowser.h"
#include "kprMessage.h"
#include "kprSkin.h"
#include "kprURL.h"
#include "kprShell.h"
#include "kprUtilities.h"

FskExport(FskErr) kprBrowser_fskLoad(FskLibrary library)
{
	return kFskErrNone;
}

FskExport(FskErr) kprBrowser_fskUnload(FskLibrary library)
{
	return kFskErrNone;
}

/* BROWSER */

static void KprBrowserLoading(FskBrowser browser, void* it);
static void KprBrowserLoaded(FskBrowser browser, void* it);
static Boolean KprBrowserShouldHandleURL(FskBrowser browser, char *url, void* it);

static void KprBrowserActivated(void* it, Boolean activateIt);
static void KprBrowserCascade(void* it, KprStyle style);
static void KprBrowserDispose(void* it);
static void KprBrowserPlaced(void* it);
static void KprBrowserSetWindow(void* it, KprShell shell, KprStyle style);

static void KprBrowserLayered(void* it, KprLayer layer, Boolean layerIt);
static void KprBrowserPlace(void* it);
static void KprBrowserPredict(void* it, FskRectangle area);
static void KprBrowserShowing(void* it, Boolean showIt);
static void KprBrowserShown(void* it, Boolean showIt);

#if SUPPORT_INSTRUMENTATION
static FskInstrumentedTypeRecord KprBrowserInstrumentation = { NULL, sizeof(FskInstrumentedTypeRecord), "KprBrowser", FskInstrumentationOffset(KprBrowserRecord), NULL, 0, NULL, KprInsrumentationFormatMessage, NULL, 0 };
#endif
static KprDispatchRecord KprBrowserDispatchRecord = {
	"browser",
	KprBrowserActivated,
	KprContentAdded,
	KprBrowserCascade,
	KprBrowserDispose,
	KprContentDraw,
	KprContentFitHorizontally,
	KprContentFitVertically,
	KprContentGetBitmap,
	KprContentHit,
	KprContentIdle,
	KprContentInvalidated,
	KprBrowserLayered,
	KprContentMark,
	KprContentMeasureHorizontally,
	KprContentMeasureVertically,
	KprBrowserPlace,
	KprBrowserPlaced,
	KprBrowserPredict,
	KprContentReflowing,
	KprContentRemoving,
	KprBrowserSetWindow,
	KprBrowserShowing,
	KprBrowserShown,
	KprContentUpdate
};

FskErr KprBrowserNew(KprBrowser* it,  KprCoordinates coordinates)
{
	FskErr err = kFskErrNone;
	KprBrowser self;

	bailIfError(FskMemPtrNewClear(sizeof(KprBrowserRecord), it));
	self = *it;
	FskInstrumentedItemNew(self, NULL, &KprBrowserInstrumentation);
	self->dispatch = &KprBrowserDispatchRecord;
	self->flags = kprVisible;
	KprContentInitialize((KprContent)self, coordinates, NULL, NULL);
	
	bailIfError(FskBrowserCreate(&self->_browser, self));
	
	FskBrowserSetDidStartLoadCallback(self->_browser, KprBrowserLoading);
	FskBrowserSetDidLoadCallback(self->_browser, KprBrowserLoaded);
	FskBrowserSetShouldHandleURLCallback(self->_browser, KprBrowserShouldHandleURL);
	
bail:
	if (err != kFskErrNone) KprBrowserDispose(*it);
	return err;
}

char *KprBrowserGetURL(KprBrowser self)
{
	return FskBrowserGetURL(self->_browser);
}

void KprBrowserSetURL(KprBrowser self, char* url)
{
	KprContentInvalidate((KprContent)self);
	
	FskBrowserSetURL(self->_browser, url);

	KprContentReflow((KprContent)self, kprSizeChanged);
}

void KprBrowserReload(KprBrowser self)
{
	FskBrowserReload(self->_browser);
	KprContentReflow((KprContent)self, kprSizeChanged);
}

void KprBrowserBack(KprBrowser self)
{
	FskBrowserBack(self->_browser);
	KprContentReflow((KprContent)self, kprSizeChanged);
}

void KprBrowserForward(KprBrowser self)
{
	FskBrowserForward(self->_browser);
	KprContentReflow((KprContent)self, kprSizeChanged);
}

Boolean KprBrowserCanBack(KprBrowser self)
{
	return FskBrowserCanBack(self->_browser);
}

Boolean KprBrowserCanForward(KprBrowser self)
{
	return FskBrowserCanForward(self->_browser);
}

char *KprBrowserEvaluateScript(KprBrowser self, char *script)
{
	return FskBrowserEvaluateScript(self->_browser, script);
}

void KprBrowserLoading(FskBrowser browser UNUSED, void* it)
{
	KprBrowser self = it;
	kprDelegateLoading(self);
}

void KprBrowserLoaded(FskBrowser browser UNUSED, void* it)
{
	KprBrowser self = it;
	kprDelegateLoaded(self);
}

Boolean KprBrowserShouldHandleURL(FskBrowser browser, char *url, void* it)
{
	FskErr err = kFskErrNone;
	KprBrowser self = it;
	KprMessage message = NULL;
	Boolean handle = false;
	
	bailIfError(KprMessageNew(&message, url));
	message->waiting = true;
	kprDelegateInvoke(self, message);
	handle = message->waiting;
bail:
	if (message) KprMessageDispose(message);
	return handle;
}

void KprBrowserActivated(void* it, Boolean activateIt)
{
	KprBrowser self = it;
	
	KprContentActivated(it, activateIt);
	
	FskBrowserActivated(self->_browser, activateIt);
}

void KprBrowserCascade(void* it, KprStyle style) 
{
	KprContentCascade(it, style);
	KprContentReflow(it, kprSizeChanged);
}


void KprBrowserDispose(void* it)
{
	KprBrowser self = it;
	
	FskBrowserDispose(&self->_browser);
	KprContentDispose(it);
}

void KprBrowserPlaced(void* it)
{
	KprBrowser self = it;
	FskRectangleRecord bounds;
	
	KprContentPlaced(it);
	
	bounds = self->bounds;
	KprContentToWindowCoordinates((KprContent)self, 0, 0, &bounds.x, &bounds.y);
	
	FskBrowserSetFrame(self->_browser, &bounds);
}

void KprBrowserSetWindow(void* it, KprShell shell, KprStyle style)
{
	KprBrowser self = it;
	
	if (!self->shell && shell)
		KprShellStartPlacing(shell, it);
	if (self->shell && !shell)
		KprShellStopPlacing(self->shell, it);
		
	if (self->_attached) {
		FskBrowserDetach(self->_browser);
		self->_attached = false;
	}
	
	KprContentSetWindow(it, shell, style);
	
	if (self->shell) {
		FskBrowserAttach(self->_browser, shell->window);
		self->_attached = true;
	}
}

void KprBrowserLayered(void* it UNUSED, KprLayer layer UNUSED, Boolean layerIt)
{
	KprBrowser self = it;
	
	self->_layered = layerIt;
	FskBrowserActivated(self->_browser, !self->_layered);
}

void KprBrowserPlace(void* it UNUSED)
{
}

void KprBrowserPredict(void* it UNUSED, FskRectangle area)
{
}

void KprBrowserShowing(void* it, Boolean showIt)
{
	KprContentInvalidate(it);
}

void KprBrowserShown(void* it, Boolean showIt)
{
	KprContentInvalidate(it);
}

/* BROWSER ECMASCRIPT */

void KPR_Browser(xsMachine *the)
{
	xsIntegerValue c = xsToInteger(xsArgc);
	KprCoordinatesRecord coordinates;
	KprBrowser self;
	xsSlotToKprCoordinates(the, &xsArg(0), &coordinates);
	xsThrowIfFskErr(KprBrowserNew(&self, &coordinates));
	kprContentConstructor(KPR_Browser);
	
	if (c > 1) {
		xsCall1_noResult(xsThis, xsID_load, xsArg(1));
	}
}


void KPR_browser_get_url(xsMachine *the)
{
	KprBrowser self = kprGetHostData(xsThis, this, browser);
	char* url = KprBrowserGetURL(self);
	if (url) {
		xsResult = xsString(url);
		FskMemPtrDispose(url);
	}
}

void KPR_browser_set_url(xsMachine *the)
{
	xsCall1_noResult(xsThis, xsID_load, xsArg(0));
}

void KPR_browser_load(xsMachine *the)
{
	xsIntegerValue c = xsToInteger(xsArgc);
	KprBrowser self = kprGetHostData(xsThis, this, browser);
	char* url = NULL;
	xsTry {
		if ((c > 0) && xsTest(xsArg(0))) {
			xsStringValue reference = xsToString(xsArg(0));
			xsStringValue base = xsToString(xsModuleURL);
			xsThrowIfFskErr(KprURLMerge(base, reference, &url));
		}
		KprBrowserSetURL(self, url);
		FskMemPtrDispose(url);
	}
	xsCatch {
		FskMemPtrDispose(url);
	}
}

void KPR_browser_reload(xsMachine *the)
{
	KprBrowser self = kprGetHostData(xsThis, this, browser);
	KprBrowserReload(self);
}

void KPR_browser_back(xsMachine *the)
{
	KprBrowser self = kprGetHostData(xsThis, this, browser);
	KprBrowserBack(self);
}

void KPR_browser_forward(xsMachine *the)
{
	KprBrowser self = kprGetHostData(xsThis, this, browser);
	KprBrowserForward(self);
}

void KPR_browser_get_canBack(xsMachine *the)
{
	KprBrowser self = kprGetHostData(xsThis, this, browser);
	xsResult = xsBoolean(KprBrowserCanBack(self));
}

void KPR_browser_get_canForward(xsMachine *the)
{
	KprBrowser self = kprGetHostData(xsThis, this, browser);
	xsResult = xsBoolean(KprBrowserCanForward(self));
}

void KPR_browser_evaluate(xsMachine *the)
{
	xsIntegerValue c = xsToInteger(xsArgc);
	KprBrowser self = kprGetHostData(xsThis, this, browser);
	char* script = NULL;
	if ((c > 0) && xsTest(xsArg(0))) {
		char *result;
		
		script = xsToString(xsArg(0));
		result = KprBrowserEvaluateScript(self, script);
		if (result) {
			xsResult = xsString(result);
			FskMemPtrDispose(result);
		}
	}
}

