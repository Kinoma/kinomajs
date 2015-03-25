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
#define __FSKWINDOW_PRIV__

#include "kprSkin.h"
#include "kprBehavior.h"
#include "kprContent.h"
#include "kprHandler.h"
#include "kprMessage.h"
#include "kprShell.h"

#if TARGET_OS_MAC
	#if TARGET_OS_IPHONE
		#include "FskCocoaSupportPhone.h"
	#else /* !TARGET_OS_IPHONE */
		#include "FskCocoaSupport.h"
	#endif /* !TARGET_OS_IPHONE */
#endif

void KprDefaultBehaviorAccept(void* it UNUSED, KprMessage message UNUSED)
{
}

void KprDefaultBehaviorActivated(void* it UNUSED, Boolean activated UNUSED)
{
}

void KprDefaultBehaviorAdapt(void* it UNUSED)
{
}

void KprDefaultBehaviorCancel(void* it UNUSED, KprMessage message UNUSED)
{
}

void KprDefaultBehaviorComplete(void* it UNUSED, KprMessage message UNUSED)
{
}

void KprDefaultBehaviorDisplayed(void* it UNUSED)
{
}

void KprDefaultBehaviorDisplaying(void* it UNUSED)
{
}

void KprDefaultBehaviorDispose(void* it UNUSED)
{
}

void KprDefaultBehaviorFinished(void* it UNUSED)
{
}

void KprDefaultBehaviorFocused(void* it UNUSED)
{
}

void KprDefaultBehaviorInvoke(void* it UNUSED, KprMessage message UNUSED)
{
}

Boolean KprDefaultBehaviorKeyDown(void* it UNUSED, char* key UNUSED, UInt32 modifiers UNUSED, UInt32 count UNUSED, double ticks UNUSED)
{
	return false;
}

Boolean KprDefaultBehaviorKeyUp(void* it UNUSED, char* key UNUSED, UInt32 modifiers UNUSED, UInt32 count UNUSED, double ticks UNUSED)
{
	return false;
}

void KprDefaultBehaviorLaunch(void* it UNUSED)
{
}

void KprDefaultBehaviorLoaded(void* it UNUSED)
{
}

void KprDefaultBehaviorLoading(void* it UNUSED)
{
}

void KprDefaultBehaviorMark(void* it UNUSED, xsMarkRoot markRoot UNUSED)
{
}

SInt32 KprDefaultBehaviorMeasureHorizontally(void* it UNUSED, SInt32 width)
{
	return width;
}

SInt32 KprDefaultBehaviorMeasureVertically(void* it UNUSED, SInt32 height)
{
	return height;
}

void KprDefaultBehaviorMetadataChanged(void* it UNUSED)
{
}

void KprDefaultBehaviorQuit(void* it UNUSED)
{
}

#if SUPPORT_REMOTE_NOTIFICATION
void KprDefaultBehaviorRemoteNotificationRegistered(void* it UNUSED, const char *deviceToken UNUSED, const char *osType UNUSED)
{
}

void KprDefaultBehaviorRemoteNotified(void* it UNUSED, const char *json UNUSED)
{
}
#endif	/* SUPPORT_REMOTE_NOTIFICATION */

void KprDefaultBehaviorScrolled(void* it UNUSED)
{
}

Boolean KprDefaultBehaviorSensorBegan(void* it UNUSED, UInt32 id UNUSED, double ticks UNUSED, UInt32 c UNUSED, double *values UNUSED)
{
	return false;
}

Boolean KprDefaultBehaviorSensorChanged(void* it UNUSED, UInt32 id UNUSED, double ticks UNUSED, UInt32 c UNUSED, double *values UNUSED)
{
	return false;
}

Boolean KprDefaultBehaviorSensorEnded(void* it UNUSED, UInt32 id UNUSED, double ticks UNUSED, UInt32 c UNUSED, double *values UNUSED)
{
	return false;
}

void KprDefaultBehaviorStateChanged(void* it UNUSED)
{
}

void KprDefaultBehaviorTimeChanged(void* it UNUSED)
{
}

void KprDefaultBehaviorTouchBegan(void* it UNUSED, UInt32 id UNUSED, SInt32 x UNUSED, SInt32 y UNUSED, double ticks UNUSED)
{
}

void KprDefaultBehaviorTouchCancelled(void* it UNUSED, UInt32 id UNUSED, SInt32 x UNUSED, SInt32 y UNUSED, double ticks UNUSED)
{
}

void KprDefaultBehaviorTouchEnded(void* it UNUSED, UInt32 id UNUSED, SInt32 x UNUSED, SInt32 y UNUSED, double ticks UNUSED)
{
}

void KprDefaultBehaviorTouchMoved(void* it UNUSED, UInt32 id UNUSED, SInt32 x UNUSED, SInt32 y UNUSED, double ticks UNUSED)
{
}

void KprDefaultBehaviorTransitionBeginning(void* it UNUSED)
{
}

void KprDefaultBehaviorTransitionEnded(void* it UNUSED)
{
}

void KprDefaultBehaviorUndisplayed(void* it UNUSED)
{
}

void KprDefaultBehaviorUnfocused(void* it UNUSED)
{
}

KprDelegateRecord kprDefaultBehaviorDelegateRecord = {
	KprDefaultBehaviorAccept,
	KprDefaultBehaviorActivated,
	KprDefaultBehaviorAdapt,
	KprDefaultBehaviorCancel,
	KprDefaultBehaviorComplete,
	KprDefaultBehaviorDisplayed,
	KprDefaultBehaviorDisplaying,
	KprDefaultBehaviorDispose,
	KprDefaultBehaviorFinished,
	KprDefaultBehaviorFocused,
	KprDefaultBehaviorInvoke,
	KprDefaultBehaviorKeyDown,
	KprDefaultBehaviorKeyUp,
	KprDefaultBehaviorLaunch,
	KprDefaultBehaviorLoaded,
	KprDefaultBehaviorLoading,
	KprDefaultBehaviorMark,
	KprDefaultBehaviorMeasureHorizontally,
	KprDefaultBehaviorMeasureVertically,
	KprDefaultBehaviorMetadataChanged,
	KprDefaultBehaviorQuit,
#if SUPPORT_REMOTE_NOTIFICATION
	KprDefaultBehaviorRemoteNotificationRegistered,
	KprDefaultBehaviorRemoteNotified,
#endif	/* SUPPORT_REMOTE_NOTIFICATION */
	KprDefaultBehaviorScrolled,
	KprDefaultBehaviorSensorBegan,
	KprDefaultBehaviorSensorChanged,
	KprDefaultBehaviorSensorEnded,
	KprDefaultBehaviorStateChanged,
	KprDefaultBehaviorTimeChanged,
	KprDefaultBehaviorTouchBegan,
	KprDefaultBehaviorTouchCancelled,
	KprDefaultBehaviorTouchEnded,
	KprDefaultBehaviorTouchMoved,
	KprDefaultBehaviorTransitionBeginning,
	KprDefaultBehaviorTransitionEnded,
	KprDefaultBehaviorUndisplayed,
	KprDefaultBehaviorUnfocused
};

FskErr KprDelegateNew(KprDelegate* it)
{
	FskErr err = kFskErrNone;
	bailIfError(FskMemPtrNewClear(sizeof(KprDelegateRecord), it));
	FskMemCopy(*it, &kprDefaultBehaviorDelegateRecord, sizeof(KprDelegateRecord));
bail:
	return err;
}

#if TARGET_OS_MAC
extern void FskCocoaWindowDragMouseDown(FskWindow fskWindow);
extern void FskCocoaWindowDragMouseMoved(FskWindow fskWindow);
#endif
static void KprDragBarBehaviorDispose(void* it);
static void KprDragBarBehaviorTouchBegan(void* it, UInt32 id, SInt32 x, SInt32 y, double ticks);
static void KprDragBarBehaviorTouchEnded(void* it, UInt32 id, SInt32 x, SInt32 y, double ticks);
static void KprDragBarBehaviorTouchMoved(void* it, UInt32 id, SInt32 x, SInt32 y, double ticks);

typedef struct {
	KprDelegate delegate;
	KprSlotPart;
#if TARGET_OS_WIN32
	POINT ptAnchor;
	RECT rcAnchor;
#endif
} KprDragBarBehaviorRecord, *KprDragBarBehavior;

#if SUPPORT_INSTRUMENTATION
static FskInstrumentedTypeRecord KprDragBarBehaviorInstrumentation = { NULL, sizeof(FskInstrumentedTypeRecord), "KprDragBarBehavior", FskInstrumentationOffset(KprDragBarBehaviorRecord), NULL, 0, NULL, KprInsrumentationFormatMessage, NULL, 0 };
#endif
KprDelegate kprDragBarDelegate = NULL;

FskErr KprDragBarBehaviorNew(KprBehavior* it, KprContent content)
{
	FskErr err = kFskErrNone;
	KprDragBarBehavior self;
	
	if (!kprDragBarDelegate) {
		bailIfError(KprDelegateNew(&kprDragBarDelegate));
		kprDragBarDelegate->dispose = KprDragBarBehaviorDispose;
		kprDragBarDelegate->touchBegan = KprDragBarBehaviorTouchBegan;
		kprDragBarDelegate->touchEnded = KprDragBarBehaviorTouchEnded;
		kprDragBarDelegate->touchMoved = KprDragBarBehaviorTouchMoved;
	}
	bailIfError(FskMemPtrNewClear(sizeof(KprDragBarBehaviorRecord), it));
	self = *((KprDragBarBehavior*)it);
	FskInstrumentedItemNew(self, NULL, &KprDragBarBehaviorInstrumentation);
	FskInstrumentedItemSetOwner(self, content);
	self->delegate =kprDragBarDelegate;
bail:
	return err;
}

void KprDragBarBehaviorDispose(void* it)
{
	KprContent content = it;
	KprDragBarBehavior self = (KprDragBarBehavior)content->behavior;
	FskInstrumentedItemDispose(self);
	FskMemPtrDispose(self);
}

void KprDragBarBehaviorTouchBegan(void* it, UInt32 id UNUSED, SInt32 x UNUSED, SInt32 y UNUSED, double ticks UNUSED)
{
	{
	#if TARGET_OS_WIN32
		KprContent content = it;
		KprDragBarBehavior self = (KprDragBarBehavior)content->behavior;
		HWND hwnd = content->shell->window->hwnd;
		if (GetSystemMetrics(SM_CMONITORS) == 1) {
			RECT bounds;
			SystemParametersInfo(SPI_GETWORKAREA, 0, &bounds, 0);
			ClipCursor(&bounds);
		}
		GetCursorPos(&self->ptAnchor);
		GetWindowRect(hwnd, &self->rcAnchor);
	#elif TARGET_OS_MAC && !TARGET_OS_IPHONE
		KprContent content = it;
		FskCocoaWindowDragMouseDown(content->shell->window);
	#endif
	}
}

void KprDragBarBehaviorTouchEnded(void* it UNUSED, UInt32 id UNUSED, SInt32 x UNUSED, SInt32 y UNUSED, double ticks UNUSED)
{
#if TARGET_OS_WIN32
	ClipCursor(NULL);
#endif
}

void KprDragBarBehaviorTouchMoved(void* it, UInt32 id UNUSED, SInt32 x UNUSED, SInt32 y UNUSED, double ticks UNUSED)
{
#if TARGET_OS_WIN32
	KprContent content = it;
	KprDragBarBehavior self = (KprDragBarBehavior)content->behavior;
	HWND hwnd = content->shell->window->hwnd;
	if (!IsZoomed(hwnd)) {
		POINT pt;
		RECT rc;
		GetCursorPos(&pt);
		rc = self->rcAnchor;
		rc.left += pt.x - self->ptAnchor.x;
		rc.right += pt.x - self->ptAnchor.x;
		rc.top += pt.y - self->ptAnchor.y;
		rc.bottom += pt.y - self->ptAnchor.y;
		MoveWindow(hwnd, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, TRUE);
	}
#elif TARGET_OS_MAC && !TARGET_OS_IPHONE
	KprContent content = it;
	FskCocoaWindowDragMouseMoved(content->shell->window);
#endif
}

static void KprGrowBoxBehaviorDispose(void* it);
static void KprGrowBoxBehaviorTouchBegan(void* it, UInt32 id, SInt32 x, SInt32 y, double ticks);
static void KprGrowBoxBehaviorTouchEnded(void* it, UInt32 id, SInt32 x, SInt32 y, double ticks);
static void KprGrowBoxBehaviorTouchMoved(void* it, UInt32 id, SInt32 x, SInt32 y, double ticks);

typedef struct {
	KprDelegate delegate;
	KprSlotPart;
#if TARGET_OS_WIN32
	POINT ptAnchor;
	RECT rcAnchor;
#endif
} KprGrowBoxBehaviorRecord, *KprGrowBoxBehavior;

#if SUPPORT_INSTRUMENTATION
static FskInstrumentedTypeRecord KprGrowBoxBehaviorInstrumentation = { NULL, sizeof(FskInstrumentedTypeRecord), "KprGrowBoxBehavior", FskInstrumentationOffset(KprGrowBoxBehaviorRecord), NULL, 0, NULL, KprInsrumentationFormatMessage, NULL, 0 };
#endif
KprDelegate kprGrowBoxDelegate = NULL;

FskErr KprGrowBoxBehaviorNew(KprBehavior* it, KprContent content)
{
	FskErr err = kFskErrNone;
	KprGrowBoxBehavior self;
	if (!kprGrowBoxDelegate) {
		bailIfError(KprDelegateNew(&kprGrowBoxDelegate));
		kprGrowBoxDelegate->dispose = KprGrowBoxBehaviorDispose;
		kprGrowBoxDelegate->touchBegan = KprGrowBoxBehaviorTouchBegan;
		kprGrowBoxDelegate->touchEnded = KprGrowBoxBehaviorTouchEnded;
		kprGrowBoxDelegate->touchMoved = KprGrowBoxBehaviorTouchMoved;
	}
	bailIfError(FskMemPtrNewClear(sizeof(KprGrowBoxBehaviorRecord), it));
	self = *((KprGrowBoxBehavior*)it);
	FskInstrumentedItemNew(self, NULL, &KprGrowBoxBehaviorInstrumentation);
	FskInstrumentedItemSetOwner(self, content);
	self->delegate = kprGrowBoxDelegate;
bail:
	return err;
}

void KprGrowBoxBehaviorDispose(void* it)
{
	KprContent content = it;
	KprGrowBoxBehavior self = (KprGrowBoxBehavior)content->behavior;
	FskInstrumentedItemDispose(self);
	FskMemPtrDispose(self);
}

void KprGrowBoxBehaviorTouchBegan(void* it, UInt32 id UNUSED, SInt32 x UNUSED, SInt32 y UNUSED, double ticks UNUSED)
{
#if TARGET_OS_WIN32
	KprContent content = it;
	KprGrowBoxBehavior self = (KprGrowBoxBehavior)content->behavior;
	KprShell shell = content->shell;
	HWND hwnd = shell->window->hwnd;
	if (GetSystemMetrics(SM_CMONITORS) == 1) {
		RECT bounds;
		SystemParametersInfo(SPI_GETWORKAREA, 0, &bounds, 0);
		ClipCursor(&bounds);
	}
	GetCursorPos(&self->ptAnchor);
	GetWindowRect(hwnd, &self->rcAnchor);
#elif TARGET_OS_MAC && !TARGET_OS_IPHONE
	KprContent content = it;
	FskCocoaWindowResizeMouseDown(content->shell->window);
#endif
}

void KprGrowBoxBehaviorTouchEnded(void* it UNUSED, UInt32 id UNUSED, SInt32 x UNUSED, SInt32 y UNUSED, double ticks UNUSED)
{
#if TARGET_OS_WIN32
	ClipCursor(NULL);
#endif
}

void KprGrowBoxBehaviorTouchMoved(void* it, UInt32 id UNUSED, SInt32 x UNUSED, SInt32 y UNUSED, double ticks UNUSED)
{
#if TARGET_OS_WIN32
	KprContent content = it;
	KprGrowBoxBehavior self = (KprGrowBoxBehavior)content->behavior;
	KprShell shell = content->shell;
	HWND hwnd = shell->window->hwnd;
	if (!IsZoomed(hwnd)) {
		POINT pt;
		RECT rc;
		GetCursorPos(&pt);
		rc = self->rcAnchor;
		rc.right += pt.x - self->ptAnchor.x;
		rc.bottom += pt.y - self->ptAnchor.y;
		MoveWindow(hwnd, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, TRUE);
	}
#elif TARGET_OS_MAC && !TARGET_OS_IPHONE
	KprContent content = it;
	FskCocoaWindowResizeMouseMoved(content->shell->window);
#endif
}

static void KprScriptBehaviorAccept(void* it, KprMessage message);
static void KprScriptBehaviorActivated(void* it, Boolean activated);
static void KprScriptBehaviorAdapt(void* it);
static void KprScriptBehaviorCancel(void* it, KprMessage message);
static void KprScriptBehaviorComplete(void* it, KprMessage message);
static void KprScriptBehaviorDisplayed(void* it);
static void KprScriptBehaviorDisplaying(void* it);
static void KprScriptBehaviorDispose(void* it);
static void KprScriptBehaviorFinished(void* it);
static void KprScriptBehaviorFocused(void* it);
static void KprScriptBehaviorInvoke(void* it, KprMessage message);
static Boolean KprScriptBehaviorKeyDown(void* it, char* key, UInt32 modifiers, UInt32 count, double ticks);
static Boolean KprScriptBehaviorKeyUp(void* it, char* key, UInt32 modifiers, UInt32 count, double ticks);
static void KprScriptBehaviorLaunch(void* it);
static void KprScriptBehaviorLoaded(void* it);
static void KprScriptBehaviorLoading(void* it);
static void KprScriptBehaviorMark(void* it, xsMarkRoot markRoot);
static SInt32 KprScriptBehaviorMeasureHorizontally(void* it, SInt32 width);
static SInt32 KprScriptBehaviorMeasureVertically(void* it, SInt32 height);
static void KprScriptBehaviorMetadataChanged(void* it);
static void KprScriptBehaviorQuit(void* it);
#if SUPPORT_REMOTE_NOTIFICATION
static void KprScriptBehaviorRemoteNotificationRegistered(void* it, const char *token, const char *osType);
static void KprScriptBehaviorRemoteNotified(void* it, const char *json);
#endif	/* SUPPORT_REMOTE_NOTIFICATION */
static void KprScriptBehaviorScrolled(void* it);
static Boolean KprScriptBehaviorSensorBegan(void* it, UInt32 id, double ticks, UInt32 c, double *values);
static Boolean KprScriptBehaviorSensorChanged(void* it, UInt32 id, double ticks, UInt32 c, double *values);
static Boolean KprScriptBehaviorSensorEnded(void* it, UInt32 id, double ticks, UInt32 c, double *values);
static void KprScriptBehaviorStateChanged(void* it);
static void KprScriptBehaviorTimeChanged(void* it);
static void KprScriptBehaviorTouchBegan(void* it, UInt32 id, SInt32 x, SInt32 y, double ticks);
static void KprScriptBehaviorTouchCancelled(void* it, UInt32 id, SInt32 x, SInt32 y, double ticks);
static void KprScriptBehaviorTouchEnded(void* it, UInt32 id, SInt32 x, SInt32 y, double ticks);
static void KprScriptBehaviorTouchMoved(void* it, UInt32 id, SInt32 x, SInt32 y, double ticks);
static void KprScriptBehaviorTransitionBeginning(void* it);
static void KprScriptBehaviorTransitionEnded(void* it);
static void KprScriptBehaviorUndisplayed(void* it);
static void KprScriptBehaviorUnfocused(void* it);

#if SUPPORT_INSTRUMENTATION
static FskInstrumentedTypeRecord KprScriptBehaviorInstrumentation = { NULL, sizeof(FskInstrumentedTypeRecord), "KprScriptBehavior", FskInstrumentationOffset(KprScriptBehaviorRecord), NULL, 0, NULL, KprInsrumentationFormatMessage, NULL, 0 };
#endif
static KprDelegateRecord KprScriptBehaviorDelegateRecord = {
	KprScriptBehaviorAccept,
	KprScriptBehaviorActivated,
	KprScriptBehaviorAdapt,
	KprScriptBehaviorCancel,
	KprScriptBehaviorComplete,
	KprScriptBehaviorDisplayed,
	KprScriptBehaviorDisplaying,
	KprScriptBehaviorDispose,
	KprScriptBehaviorFinished,
	KprScriptBehaviorFocused,
	KprScriptBehaviorInvoke,
	KprScriptBehaviorKeyDown,
	KprScriptBehaviorKeyUp,
	KprScriptBehaviorLaunch,
	KprScriptBehaviorLoaded,
	KprScriptBehaviorLoading,
	KprScriptBehaviorMark,
	KprScriptBehaviorMeasureHorizontally,
	KprScriptBehaviorMeasureVertically,
	KprScriptBehaviorMetadataChanged,
	KprScriptBehaviorQuit,
#if SUPPORT_REMOTE_NOTIFICATION
	KprScriptBehaviorRemoteNotificationRegistered,
	KprScriptBehaviorRemoteNotified,
#endif	/* SUPPORT_REMOTE_NOTIFICATION */
	KprScriptBehaviorScrolled,
	KprScriptBehaviorSensorBegan,
	KprScriptBehaviorSensorChanged,
	KprScriptBehaviorSensorEnded,
	KprScriptBehaviorStateChanged,
	KprScriptBehaviorTimeChanged,
	KprScriptBehaviorTouchBegan,
	KprScriptBehaviorTouchCancelled,
	KprScriptBehaviorTouchEnded,
	KprScriptBehaviorTouchMoved,
	KprScriptBehaviorTransitionBeginning,
	KprScriptBehaviorTransitionEnded,
	KprScriptBehaviorUndisplayed,
	KprScriptBehaviorUnfocused
};

FskErr KprScriptBehaviorNew(KprBehavior* it, KprContent content, xsMachine* the, xsSlot* slot)
{
	FskErr err = kFskErrNone;
	KprScriptBehavior self;
	bailIfError(FskMemPtrNewClear(sizeof(KprScriptBehaviorRecord), it));
	self = *((KprScriptBehavior*)it);
	FskInstrumentedItemNew(self, NULL, &KprScriptBehaviorInstrumentation);
	FskInstrumentedItemSetOwner(self, content);
	self->delegate = &KprScriptBehaviorDelegateRecord;
	self->the = the;
	self->slot = *slot;
	self->code = the->code;
	FskInstrumentedItemSendMessageNormal(content, kprInstrumentedContentPutBehavior, self);
bail:
	return err;
}

#if SUPPORT_INSTRUMENTATION
#define KprScriptBehaviorTraceCallback(NAME) \
	void* params[2] = { self, NAME }; \
	FskInstrumentedItemSendMessageDebug(content, kprInstrumentedContentCallBehavior, params)
#else
#define KprScriptBehaviorTraceCallback(NAME)
#endif

#define KprScriptBehaviorCallback(ID) \
	KprContent content = it; \
	KprScriptBehavior self = (KprScriptBehavior)content->behavior; \
	KprScriptBehaviorTraceCallback(#ID); \
	xsBeginHostSandboxCode(self->the, self->code); \
	xsVars(2); \
	xsVar(0) = xsAccess(self->slot); \
	if (xsFindResult(xsVar(0), ID)) { \
		xsVar(1) = kprContentGetter(content); \
		(void)xsCallFunction1(xsResult, xsVar(0), xsVar(1)); \
	} \
	xsEndHostSandboxCode()

	
#define KprScriptBehaviorKeyCallback(ID) \
	KprContent content = it; \
	KprScriptBehavior self = (KprScriptBehavior)content->behavior; \
	Boolean result = false; \
	KprScriptBehaviorTraceCallback(#ID); \
	xsBeginHostSandboxCode(self->the, self->code); \
	xsVars(2); \
	xsVar(0) = xsAccess(self->slot); \
	if (xsFindResult(xsVar(0), ID)) { \
		xsVar(1) = kprContentGetter(content); \
		xsResult = xsCallFunction5(xsResult, xsVar(0), xsVar(1), xsString(key), xsInteger(modifiers), xsInteger(count), xsNumber(ticks)); \
		result = xsToBoolean(xsResult); \
	} \
	xsEndHostSandboxCode(); \
	return result

#define KprScriptBehaviorMeasureCallback(ID) \
	KprContent content = it; \
	KprScriptBehavior self = (KprScriptBehavior)content->behavior; \
	SInt32 result = size; \
	KprScriptBehaviorTraceCallback(#ID); \
	xsBeginHostSandboxCode(self->the, self->code); \
	xsVars(2); \
	xsVar(0) = xsAccess(self->slot); \
	if (xsFindResult(xsVar(0), ID)) { \
		xsVar(1) = kprContentGetter(content); \
		xsResult = xsCallFunction2(xsResult, xsVar(0), xsVar(1), xsInteger(size)); \
		result = xsToInteger(xsResult); \
	} \
	xsEndHostSandboxCode(); \
	return result
	
#if SUPPORT_REMOTE_NOTIFICATION
#define KprScriptBehaviorRemoteNotificationRegisteredCallback(ID, TOKEN, TYPE) \
	KprContent content = it; \
	KprScriptBehavior self = (KprScriptBehavior)content->behavior; \
	KprScriptBehaviorTraceCallback(#ID); \
	xsBeginHostSandboxCode(self->the, self->code); \
	xsVars(4); \
	xsVar(0) = xsAccess(self->slot); \
	if (xsFindResult(xsVar(0), ID)) { \
		xsVar(1) = kprContentGetter(content); \
		xsVar(2) = (TOKEN) ? xsString((char *)TOKEN) : xsNull; \
		xsVar(3) = xsString((char *)TYPE); \
		xsResult = xsCallFunction3(xsResult, xsVar(0), xsVar(1), xsVar(2), xsVar(3)); \
	} \
	xsEndHostSandboxCode();

#define KprScriptBehaviorRemoteNotificationCallback(ID, JSON_STRING) \
	KprContent content = it; \
	KprScriptBehavior self = (KprScriptBehavior)content->behavior; \
	KprScriptBehaviorTraceCallback(#ID); \
	xsBeginHostSandboxCode(self->the, self->code); \
	xsVars(3); \
	xsVar(0) = xsAccess(self->slot); \
	if (xsFindResult(xsVar(0), ID)) { \
		xsVar(1) = kprContentGetter(content); \
		xsVar(2) = xsGet(xsGlobal, xsID_JSON); \
		xsVar(2) = xsCall1(xsVar(2), xsID_parse, xsString((char *)JSON_STRING)); \
		xsResult = xsCallFunction2(xsResult, xsVar(0), xsVar(1), xsVar(2)); \
	} \
	xsEndHostSandboxCode();
#endif	/* SUPPORT_REMOTE_NOTIFICATION */

#define KprScriptBehaviorSensorCallback(ID) \
	KprContent content = it; \
	KprScriptBehavior self = (KprScriptBehavior)content->behavior; \
	Boolean result = false; \
	UInt32 i; \
	KprScriptBehaviorTraceCallback(#ID); \
	xsBeginHostSandboxCode(self->the, self->code); \
	xsVars(2); \
	xsVar(0) = xsAccess(self->slot); \
	if (xsFindResult(xsVar(0), ID)) { \
		xsVar(1) = kprContentGetter(content); \
		xsOverflow(-6 - c); \
		*(--the->stack) = xsVar(1); \
		fxInteger(the, --the->stack, id); \
		fxNumber(the, --the->stack, ticks); \
		for (i = 0; i < c; i++) \
			fxNumber(the, --the->stack, values[i]); \
		fxInteger(the, --the->stack, 3 + c); \
		*(--the->stack) = xsVar(0); \
		*(--the->stack) = xsResult; \
		fxCall(the); \
		xsResult = fxPop(); \
		result = xsToBoolean(xsResult); \
	} \
	xsEndHostSandboxCode(); \
	return result
	
#define KprScriptBehaviorTouchCallback(ID) \
	KprContent content = it; \
	KprScriptBehavior self = (KprScriptBehavior)content->behavior; \
	KprScriptBehaviorTraceCallback(#ID); \
	xsBeginHostSandboxCode(self->the, self->code); \
	xsVars(2); \
	xsVar(0) = xsAccess(self->slot); \
	if (xsFindResult(xsVar(0), ID)) { \
		xsVar(1) = kprContentGetter(content); \
		(void)xsCallFunction5(xsResult, xsVar(0), xsVar(1), xsInteger(id), xsInteger(x), xsInteger(y), xsNumber(ticks)); \
	} \
	xsEndHostSandboxCode()

void KprScriptBehaviorAccept(void* it, KprMessage message)
{
	KprContent content = it;
	KprScriptBehavior self = (KprScriptBehavior)content->behavior;
#if SUPPORT_INSTRUMENTATION
	void* params[2] = { self, "xsID_onAccept" };
	FskInstrumentedItemSendMessageDebug(content, kprInstrumentedContentCallBehavior, params);
#endif
	xsBeginHostSandboxCode(self->the, self->code);
	{
		xsVars(3);
		{
			xsTry {
				xsVar(0) = xsAccess(self->slot);
				if (xsFindResult(xsVar(0), xsID_onAccept)) {
					xsVar(1) = kprContentGetter(content);
					xsVar(2) = xsNewInstanceOf(xsGet(xsGet(xsGlobal, xsID_KPR), xsID_message));
					xsSetHostData(xsVar(2), message);
					FskInstrumentedItemSendMessageDebug(message, kprInstrumentedMessageConstruct, message);
					message->usage++; // host
					xsResult = xsCallFunction2(xsResult, xsVar(0), xsVar(1), xsVar(2));
				}
				if (!xsTest(xsResult))
					message->status = 404;
			}
			xsCatch {
			}
		}
	}
	xsEndHostSandboxCode();
}

void KprScriptBehaviorActivated(void* it, Boolean activated)
{
	KprContent content = it;
	KprScriptBehavior self = (KprScriptBehavior)content->behavior;
#if SUPPORT_INSTRUMENTATION
	void* params[2] = { self, "xsID_onActivated" };
	FskInstrumentedItemSendMessageDebug(content, kprInstrumentedContentCallBehavior, params);
#endif
    if (self) {
        xsBeginHostSandboxCode(self->the, self->code);
        {
            xsVars(1);
            if (xsFindResult(self->slot, xsID_onActivated)) {
                xsVar(0) = kprContentGetter(content);
                (void)xsCallFunction2(xsResult, self->slot, xsVar(0), xsBoolean(activated));
            }
        }
        xsEndHostSandboxCode();
    }
}

void KprScriptBehaviorAdapt(void* it)
{
	KprScriptBehaviorCallback(xsID_onAdapt);
}

void KprScriptBehaviorCancel(void* it, KprMessage message)
{
	KprContent content = it;
	KprScriptBehavior self = (KprScriptBehavior)content->behavior;
#if SUPPORT_INSTRUMENTATION
	void* params[2] = { self, "xsID_onCancel" };
	FskInstrumentedItemSendMessageDebug(content, kprInstrumentedContentCallBehavior, params);
#endif
	xsBeginHostSandboxCode(self->the, self->code);
	xsVars(3);
	xsVar(0) = xsAccess(self->slot);
	if (xsFindResult(xsVar(0), xsID_onCancel)) {
		xsVar(1) = kprContentGetter(content);
		if (message) {
			xsVar(2) = xsNewInstanceOf(xsGet(xsGet(xsGlobal, xsID_KPR), xsID_message));
			xsSetHostData(xsVar(2), message);
			FskInstrumentedItemSendMessageDebug(message, kprInstrumentedMessageConstruct, message);
			message->usage++; // host
		}
		(void)xsCallFunction2(xsResult, xsVar(0), xsVar(1), xsVar(2));
	}
	xsEndHostSandboxCode();
}

void KprScriptBehaviorComplete(void* it, KprMessage message)
{
	KprContent content = it;
	KprScriptBehavior self = (KprScriptBehavior)content->behavior;
#if SUPPORT_INSTRUMENTATION
	void* params[2] = { self, "xsID_onComplete" };
	FskInstrumentedItemSendMessageDebug(content, kprInstrumentedContentCallBehavior, params);
#endif
	xsBeginHostSandboxCode(self->the, self->code);
	xsVars(4);
	xsVar(0) = xsAccess(self->slot);
	if (xsFindResult(xsVar(0), xsID_onComplete)) {
		xsVar(1) = kprContentGetter(content);
		if (message) {
			xsVar(2) = xsNewInstanceOf(xsGet(xsGet(xsGlobal, xsID_KPR), xsID_message));
			xsSetHostData(xsVar(2), message);
			FskInstrumentedItemSendMessageDebug(message, kprInstrumentedMessageConstruct, message);
			message->usage++; // host
			KprMessageScriptTargetGet(message, the, &xsVar(3));
		}
		(void)xsCallFunction3(xsResult, xsVar(0), xsVar(1), xsVar(2), xsVar(3));
	}
	xsEndHostSandboxCode();
}

void KprScriptBehaviorDisplayed(void* it)
{
	KprScriptBehaviorCallback(xsID_onDisplayed);
}

void KprScriptBehaviorDisplaying(void* it)
{
	KprScriptBehaviorCallback(xsID_onDisplaying);
}

void KprScriptBehaviorDispose(void* it)
{
	KprContent content = it;
	KprScriptBehavior self = (KprScriptBehavior)content->behavior;
	FskInstrumentedItemSendMessageNormal(content, kprInstrumentedContentRemoveBehavior, self);
	FskInstrumentedItemDispose(self);
	FskMemPtrDispose(self);
}

void KprScriptBehaviorFinished(void* it)
{
	KprScriptBehaviorCallback(xsID_onFinished);
}

void KprScriptBehaviorFocused(void* it)
{
	KprScriptBehaviorCallback(xsID_onFocused);
}

void KprScriptBehaviorInvoke(void* it, KprMessage message)
{
	KprContent content = it;
	KprScriptBehavior self = (KprScriptBehavior)content->behavior;
#if SUPPORT_INSTRUMENTATION
	void* params[2] = { self, "xsID_onInvoke" };
	FskInstrumentedItemSendMessageDebug(content, kprInstrumentedContentCallBehavior, params);
#endif
	xsBeginHostSandboxCode(self->the, self->code);
	{
		xsVars(3);
		{
			xsTry {
				xsVar(0) = xsAccess(self->slot);
				if (xsFindResult(xsVar(0), xsID_onInvoke)) {
					xsVar(1) = kprContentGetter(content);
					xsVar(2) = xsNewInstanceOf(xsGet(xsGet(xsGlobal, xsID_KPR), xsID_message));
					xsSetHostData(xsVar(2), message);
					FskInstrumentedItemSendMessageDebug(message, kprInstrumentedMessageConstruct, message);
					message->usage++; // host
					(void)xsCallFunction2(xsResult, xsVar(0), xsVar(1), xsVar(2));
				}
				else
					message->status = 404;
			}
			xsCatch {
			}
		}
	}
	xsEndHostSandboxCode();
}

Boolean KprScriptBehaviorKeyDown(void* it, char* key, UInt32 modifiers, UInt32 count, double ticks)
{
	KprScriptBehaviorKeyCallback(xsID_onKeyDown);
}

Boolean KprScriptBehaviorKeyUp(void* it, char* key, UInt32 modifiers, UInt32 count, double ticks)
{
	KprScriptBehaviorKeyCallback(xsID_onKeyUp);
}

void KprScriptBehaviorLaunch(void* it)
{
	KprScriptBehaviorCallback(xsID_onLaunch);
}

void KprScriptBehaviorLoaded(void* it)
{
	KprScriptBehaviorCallback(xsID_onLoaded);
}

void KprScriptBehaviorMark(void* it, xsMarkRoot markRoot)
{
	KprContent content = it;
	KprScriptBehavior self = (KprScriptBehavior)content->behavior;
	(*markRoot)(self->the, &self->slot);
}

void KprScriptBehaviorLoading(void* it)
{
	KprScriptBehaviorCallback(xsID_onLoading);
}

SInt32 KprScriptBehaviorMeasureHorizontally(void* it, SInt32 size)
{
	KprScriptBehaviorMeasureCallback(xsID_onMeasureHorizontally);
}

SInt32 KprScriptBehaviorMeasureVertically(void* it, SInt32 size)
{
	KprScriptBehaviorMeasureCallback(xsID_onMeasureVertically);
}

void KprScriptBehaviorMetadataChanged(void* it)
{
	KprScriptBehaviorCallback(xsID_onMetadataChanged);
}

void KprScriptBehaviorQuit(void* it)
{
	KprScriptBehaviorCallback(xsID_onQuit);
}

#if SUPPORT_REMOTE_NOTIFICATION
void KprScriptBehaviorRemoteNotificationRegistered(void* it, const char *deviceToken, const char* osType)
{
	KprScriptBehaviorRemoteNotificationRegisteredCallback(xsID_onRemoteNotificationRegistered, deviceToken, osType);
}

void KprScriptBehaviorRemoteNotified(void* it, const char* json)
{
	KprScriptBehaviorRemoteNotificationCallback(xsID_onRemoteNotification, json);
}
#endif	/* SUPPORT_REMOTE_NOTIFICATION */

void KprScriptBehaviorScrolled(void* it)
{
	KprScriptBehaviorCallback(xsID_onScrolled);
}

Boolean KprScriptBehaviorSensorBegan(void* it, UInt32 id, double ticks, UInt32 c, double *values)
{
	KprScriptBehaviorSensorCallback(xsID_onSensorBegan);
}

Boolean KprScriptBehaviorSensorChanged(void* it, UInt32 id, double ticks, UInt32 c, double *values)
{
	KprScriptBehaviorSensorCallback(xsID_onSensorChanged);
}

Boolean KprScriptBehaviorSensorEnded(void* it, UInt32 id, double ticks, UInt32 c, double *values)
{
	KprScriptBehaviorSensorCallback(xsID_onSensorEnded);
}

void KprScriptBehaviorStateChanged(void* it)
{
	KprScriptBehaviorCallback(xsID_onStateChanged);
}

void KprScriptBehaviorTimeChanged(void* it)
{
	KprScriptBehaviorCallback(xsID_onTimeChanged);
}

void KprScriptBehaviorTouchBegan(void* it, UInt32 id, SInt32 x, SInt32 y, double ticks)
{
	KprScriptBehaviorTouchCallback(xsID_onTouchBegan);
}

void KprScriptBehaviorTouchCancelled(void* it, UInt32 id, SInt32 x, SInt32 y, double ticks)
{
	KprScriptBehaviorTouchCallback(xsID_onTouchCancelled);
}

void KprScriptBehaviorTouchEnded(void* it, UInt32 id, SInt32 x, SInt32 y, double ticks)
{
	KprScriptBehaviorTouchCallback(xsID_onTouchEnded);
}

void KprScriptBehaviorTouchMoved(void* it, UInt32 id, SInt32 x, SInt32 y, double ticks)
{
	KprScriptBehaviorTouchCallback(xsID_onTouchMoved);
}

void KprScriptBehaviorTransitionBeginning(void* it)
{
	KprScriptBehaviorCallback(xsID_onTransitionBeginning);
}

void KprScriptBehaviorTransitionEnded(void* it)
{
	KprScriptBehaviorCallback(xsID_onTransitionEnded);
}

void KprScriptBehaviorUndisplayed(void* it)
{
	KprScriptBehaviorCallback(xsID_onUndisplayed);
}

void KprScriptBehaviorUnfocused(void* it)
{
	KprScriptBehaviorCallback(xsID_onUnfocused);
}

KprScriptBehavior KprContentGetScriptBehavior(void* it)
{
	KprContent content = it;
	KprScriptBehavior self = (KprScriptBehavior)content->behavior;
	return (self && (self->delegate == &KprScriptBehaviorDelegateRecord)) ? self : NULL;
}

void KPR_Event_patch(xsMachine* the)
{
	xsResult = xsGet(xsGlobal, xsID("Event"));
	xsNewHostProperty(xsResult, xsID("FunctionKeyPlay"), xsInteger(kFskEventFunctionKeyPlay), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID("FunctionKeyPause"), xsInteger(kFskEventFunctionKeyPause), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID("FunctionKeyStop"), xsInteger(kFskEventFunctionKeyStop), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID("FunctionKeyTogglePlayPause"), xsInteger(kFskEventFunctionKeyTogglePlayPause), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID("FunctionKeyPreviousTrack"), xsInteger(kFskEventFunctionKeyPreviousTrack), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID("FunctionKeyNextTrack"), xsInteger(kFskEventFunctionKeyNextTrack), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID("FunctionKeyBeginSeekingBackward"), xsInteger(kFskEventFunctionKeyBeginSeekingBackward), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID("FunctionKeyEndSeekingBackward"), xsInteger(kFskEventFunctionKeyEndSeekingBackward), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID("FunctionKeyBeginSeekingForward"), xsInteger(kFskEventFunctionKeyBeginSeekingForward), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID("FunctionKeyEndSeekingForward"), xsInteger(kFskEventFunctionKeyEndSeekingForward), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID("FunctionKeyHome"), xsInteger(kFskEventFunctionKeyHome), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID("FunctionKeySearch"), xsInteger(kFskEventFunctionKeySearch), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID("FunctionKeyPower"), xsInteger(kFskEventFunctionKeyPower), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID("FunctionKeyMenu"), xsInteger(kFskEventFunctionKeyMenu), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID("FunctionKeyVolumeUp"), xsInteger(kFskEventFunctionKeyVolumeUp), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID("FunctionKeyVolumeDown"), xsInteger(kFskEventFunctionKeyVolumeDown), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID("FunctionKeyMute"), xsInteger(kFskEventFunctionKeyMute), xsDefault, xsDontScript);
}
