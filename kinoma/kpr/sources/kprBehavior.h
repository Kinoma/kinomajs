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
#ifndef __KPRBEHAVIOR__
#define __KPRBEHAVIOR__

#include "kpr.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef void (*KprBehaviorCallbackProc)(void* it);
typedef void (*KprBehaviorActivatedCallbackProc)(void* it, Boolean activated);
typedef Boolean (*KprBehaviorKeyCallbackProc)(void* it, char* key, UInt32 modifiers, UInt32 count, double ticks);
typedef void (*KprBehaviorMarkCallbackProc)(void* it, xsMarkRoot markRoot);
typedef SInt32 (*KprBehaviorMeasureCallbackProc)(void* it, SInt32 size);
typedef void (*KprBehaviorMessageCallbackProc)(void* it, KprMessage message);
#if SUPPORT_REMOTE_NOTIFICATION
typedef void (*KprBehaviorRemoteNotificationRegisteredCallbackProc)(void* it, const char *deviceToken, const char *osType);
typedef void (*KprBehaviorRemoteNotificationCallbackProc)(void* it, const char *json);
#endif	/* SUPPORT_REMOTE_NOTIFICATION */
typedef Boolean (*KprBehaviorSensorCallbackProc)(void* it, UInt32 id, double ticks, UInt32 c, double *values);
typedef void (*KprBehaviorTouchCallbackProc)(void* it, UInt32 id, SInt32 x, SInt32 y, double ticks);

struct KprDelegateStruct {
	KprBehaviorMessageCallbackProc accept;
	KprBehaviorActivatedCallbackProc activated;
	KprBehaviorCallbackProc adapt;
	KprBehaviorMessageCallbackProc cancel;
	KprBehaviorMessageCallbackProc complete;
	KprBehaviorCallbackProc displayed;
	KprBehaviorCallbackProc displaying;
	KprBehaviorCallbackProc dispose;
	KprBehaviorCallbackProc finished;
	KprBehaviorCallbackProc focused;
	KprBehaviorMessageCallbackProc invoke;
	KprBehaviorKeyCallbackProc keyDown;
	KprBehaviorKeyCallbackProc keyUp;
	KprBehaviorCallbackProc launch;
	KprBehaviorCallbackProc loaded;
	KprBehaviorCallbackProc loading;
	KprBehaviorMarkCallbackProc mark;
	KprBehaviorMeasureCallbackProc measureHorizontally;
	KprBehaviorMeasureCallbackProc measureVertically;
	KprBehaviorCallbackProc metadataChanged;
	KprBehaviorCallbackProc quit;
#if SUPPORT_REMOTE_NOTIFICATION
	KprBehaviorRemoteNotificationRegisteredCallbackProc remoteNotificationRegistered;
	KprBehaviorRemoteNotificationCallbackProc remoteNotified;
#endif	/* SUPPORT_REMOTE_NOTIFICATION */
	KprBehaviorCallbackProc scrolled;
	KprBehaviorSensorCallbackProc sensorBegan;
	KprBehaviorSensorCallbackProc sensorChanged;
	KprBehaviorSensorCallbackProc sensorEnded;
	KprBehaviorCallbackProc stateChanged;
	KprBehaviorCallbackProc timeChanged;
	KprBehaviorTouchCallbackProc touchBegan;
	KprBehaviorTouchCallbackProc touchCancelled;
	KprBehaviorTouchCallbackProc touchEnded;
	KprBehaviorTouchCallbackProc touchMoved;
	KprBehaviorCallbackProc transitionBegan;
	KprBehaviorCallbackProc transitionEnded;
	KprBehaviorCallbackProc undisplayed;
	KprBehaviorCallbackProc unfocused;
};

struct KprBehaviorStruct {
	KprDelegate delegate;
	KprSlotPart;
};

extern void KprDefaultBehaviorAccept(void* it, KprMessage message);
extern void KprDefaultBehaviorActivated(void* it, Boolean activated);
extern void KprDefaultBehaviorAdapt(void* it);
extern void KprDefaultBehaviorCancel(void* it, KprMessage message);
extern void KprDefaultBehaviorComplete(void* it, KprMessage message);
extern void KprDefaultBehaviorDisplayed(void* it);
extern void KprDefaultBehaviorDisplaying(void* it);
extern void KprDefaultBehaviorDispose(void* it);
extern void KprDefaultBehaviorFinished(void* it);
extern void KprDefaultBehaviorFocused(void* it);
extern void KprDefaultBehaviorInvoke(void* it, KprMessage message);
extern Boolean KprDefaultBehaviorKeyDown(void* it, char* key, UInt32 modifiers, UInt32 count, double ticks);
extern Boolean KprDefaultBehaviorKeyUp(void* it, char* key, UInt32 modifiers, UInt32 count, double ticks);
extern void KprDefaultBehaviorLaunch(void* it);
extern void KprDefaultBehaviorLoaded(void* it);
extern void KprDefaultBehaviorLoading(void* it);
extern void KprDefaultBehaviorMark(void* it, xsMarkRoot markRoot);
extern SInt32 KprDefaultBehaviorMeasureHorizontally(void* it, SInt32 width);
extern SInt32 KprDefaultBehaviorMeasureVertically(void* it, SInt32 height);
extern void KprDefaultBehaviorMetadataChanged(void* it);
extern void KprDefaultBehaviorQuit(void* it);
#if SUPPORT_REMOTE_NOTIFICATION
extern void KprDefaultBehaviorRemoteNotificationRegistered(void* it, const char *deviceToken, const char *osType);
extern void KprDefaultBehaviorRemoteNotified(void* it, const char *json);
#endif	/* SUPPORT_REMOTE_NOTIFICATION */
extern void KprDefaultBehaviorScrolled(void* it);
extern Boolean KprDefaultBehaviorSensorBegan(void* it, UInt32 id, double ticks, UInt32 c, double *values);
extern Boolean KprDefaultBehaviorSensorChanged(void* it, UInt32 id, double ticks, UInt32 c, double *values);
extern Boolean KprDefaultBehaviorSensorEnded(void* it, UInt32 id, double ticks, UInt32 c, double *values);
extern void KprDefaultBehaviorStateChanged(void* it);
extern void KprDefaultBehaviorTimeChanged(void* it);
extern void KprDefaultBehaviorTouchBegan(void* it, UInt32 id, SInt32 x, SInt32 y, double ticks);
extern void KprDefaultBehaviorTouchCancelled(void* it, UInt32 id, SInt32 x, SInt32 y, double ticks);
extern void KprDefaultBehaviorTouchEnded(void* it, UInt32 id, SInt32 x, SInt32 y, double ticks);
extern void KprDefaultBehaviorTouchMoved(void* it, UInt32 id, SInt32 x, SInt32 y, double ticks);
extern void KprDefaultBehaviorTransitionBeginning(void* it);
extern void KprDefaultBehaviorTransitionEnded(void* it);
extern void KprDefaultBehaviorUndisplayed(void* it);
extern void KprDefaultBehaviorUnfocused(void* it);
FskAPI(FskErr) KprDelegateNew(KprDelegate* it);

FskAPI(FskErr) KprDragBarBehaviorNew(KprBehavior* it, KprContent content);
FskAPI(FskErr) KprGrowBoxBehaviorNew(KprBehavior* it, KprContent content);
FskAPI(FskErr) KprScriptBehaviorNew(KprBehavior* it, KprContent content, xsMachine* the, xsSlot* slot);

#define kprDelegateAccept(IT, MESSAGE) (((IT)->behavior) ? (*(IT)->behavior->delegate->accept)(IT, MESSAGE) : 0)
#define kprDelegateActivated(IT, ACTIVATED) (((IT)->behavior) ? (*(IT)->behavior->delegate->activated)(IT, ACTIVATED) : 0)
#define kprDelegateAdapt(IT) (((IT)->behavior) ? (*(IT)->behavior->delegate->adapt)(IT) : 0)
#define kprDelegateCancel(IT, MESSAGE) (((IT)->behavior) ? (*(IT)->behavior->delegate->cancel)(IT, MESSAGE) : 0)
#define kprDelegateComplete(IT, MESSAGE) (((IT)->behavior) ? (*(IT)->behavior->delegate->complete)(IT, MESSAGE) : 0)
#define kprDelegateDisplayed(IT) (((IT)->behavior) ? (*(IT)->behavior->delegate->displayed)(IT) : 0)
#define kprDelegateDisplaying(IT) (((IT)->behavior) ? (*(IT)->behavior->delegate->displaying)(IT) : 0)
#define kprDelegateDispose(IT) (((IT)->behavior) ? (*(IT)->behavior->delegate->dispose)(IT) : 0)
#define kprDelegateFinished(IT) (((IT)->behavior) ? (*(IT)->behavior->delegate->finished)(IT) : 0)
#define kprDelegateFocused(IT) (((IT)->behavior) ? (*(IT)->behavior->delegate->focused)(IT) : 0)
#define kprDelegateInvoke(IT, MESSAGE) (((IT)->behavior) ? (*(IT)->behavior->delegate->invoke)(IT, MESSAGE) : 0)
#define kprDelegateKeyDown(IT, KEY, MODIFIERS, COUNT, TICKS) (((IT)->behavior) ? (*(IT)->behavior->delegate->keyDown)(IT, KEY, MODIFIERS, COUNT, TICKS) : 0)
#define kprDelegateKeyUp(IT, KEY, MODIFIERS, COUNT, TICKS) (((IT)->behavior) ? (*(IT)->behavior->delegate->keyUp)(IT, KEY, MODIFIERS, COUNT, TICKS) : 0)
#define kprDelegateLaunch(IT) (((IT)->behavior) ? (*(IT)->behavior->delegate->launch)(IT) : 0)
#define kprDelegateLoaded(IT) (((IT)->behavior) ? (*(IT)->behavior->delegate->loaded)(IT) : 0)
#define kprDelegateLoading(IT) (((IT)->behavior) ? (*(IT)->behavior->delegate->loading)(IT) : 0)
#define kprDelegateMark(IT, HOOK) (((IT)->behavior) ? (*(IT)->behavior->delegate->mark)(IT, HOOK) : 0)
#define kprDelegateMeasureHorizontally(IT, WIDTH) (((IT)->behavior) ? (*(IT)->behavior->delegate->measureHorizontally)(IT, WIDTH) : WIDTH)
#define kprDelegateMeasureVertically(IT, HEIGHT) (((IT)->behavior) ? (*(IT)->behavior->delegate->measureVertically)(IT, HEIGHT) : HEIGHT)
#define kprDelegateMetadataChanged(IT) (((IT)->behavior) ? (*(IT)->behavior->delegate->metadataChanged)(IT) : 0)
#define kprDelegateQuit(IT) (((IT)->behavior) ? (*(IT)->behavior->delegate->quit)(IT) : 0)
#if SUPPORT_REMOTE_NOTIFICATION
#define kprDelegateRemoteNotificationRegistered(IT, TOKEN, TYPE) (((IT)->behavior) ? (*(IT)->behavior->delegate->remoteNotificationRegistered)(IT, TOKEN, TYPE) : 0)
#define kprDelegateRemoteNotified(IT, JSON) (((IT)->behavior) ? (*(IT)->behavior->delegate->remoteNotified)(IT, JSON) : 0)
#endif	/* SUPPORT_REMOTE_NOTIFICATION */
#define kprDelegateScrolled(IT) (((IT)->behavior) ? (*(IT)->behavior->delegate->scrolled)(IT) : 0)
#define kprDelegateStateChanged(IT) (((IT)->behavior) ? (*(IT)->behavior->delegate->stateChanged)(IT) : 0)
#define kprDelegateTimeChanged(IT) (((IT)->behavior) ? (*(IT)->behavior->delegate->timeChanged)(IT) : 0)
#define kprDelegateSensorBegan(IT, ID, TICKS, C, VALUES) (((IT)->behavior) ? (*(IT)->behavior->delegate->sensorBegan)(IT, ID, TICKS, C, VALUES) : 0)
#define kprDelegateSensorChanged(IT, ID, TICKS, C, VALUES) (((IT)->behavior) ? (*(IT)->behavior->delegate->sensorChanged)(IT, ID, TICKS, C, VALUES) : 0)
#define kprDelegateSensorEnded(IT, ID, TICKS, C, VALUES) (((IT)->behavior) ? (*(IT)->behavior->delegate->sensorEnded)(IT, ID, TICKS, C, VALUES) : 0)
#define kprDelegateTouchBegan(IT, ID, X, Y, TICKS) (((IT)->behavior) ? (*(IT)->behavior->delegate->touchBegan)(IT, ID, X, Y, TICKS) : 0)
#define kprDelegateTouchCancelled(IT, ID, X, Y, TICKS) (((IT)->behavior) ? (*(IT)->behavior->delegate->touchCancelled)(IT, ID, X, Y, TICKS) : 0)
#define kprDelegateTouchEnded(IT, ID, X, Y, TICKS) (((IT)->behavior) ? (*(IT)->behavior->delegate->touchEnded)(IT, ID, X, Y, TICKS) : 0)
#define kprDelegateTouchMoved(IT, ID, X, Y, TICKS) (((IT)->behavior) ? (*(IT)->behavior->delegate->touchMoved)(IT, ID, X, Y, TICKS) : 0)
#define kprDelegateTransitionBeginning(IT) (((IT)->behavior) ? (*(IT)->behavior->delegate->transitionBegan)(IT) : 0)
#define kprDelegateTransitionEnded(IT) (((IT)->behavior) ? (*(IT)->behavior->delegate->transitionEnded)(IT) : 0)
#define kprDelegateUndisplayed(IT) (((IT)->behavior) ? (*(IT)->behavior->delegate->undisplayed)(IT) : 0)
#define kprDelegateUnfocused(IT) (((IT)->behavior) ? (*(IT)->behavior->delegate->unfocused)(IT) : 0)

typedef struct {
	KprDelegate delegate;
	KprSlotPart;
	xsIndex* code;
} KprScriptBehaviorRecord, *KprScriptBehavior;

FskAPI(KprScriptBehavior) KprContentGetScriptBehavior(void* it);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
