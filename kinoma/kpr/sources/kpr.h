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
#ifndef __KPR__
#define __KPR__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define CHECK_UNACCELERATED_BITMAPS 0

#include "xs.h"
/* XS MACROS */
#define xsAssert(it) if (!(it)) xsThrow(xsString(#it))
#define xsBeginHostSandboxCode(_THE,_CODE) \
	do { \
		xsMachine* __HOST_THE__ = _THE; \
		xsJump __HOST_JUMP__; \
		__HOST_JUMP__.nextJump = (__HOST_THE__)->firstJump; \
		__HOST_JUMP__.stack = (__HOST_THE__)->stack; \
		__HOST_JUMP__.scope = (__HOST_THE__)->scope; \
		__HOST_JUMP__.frame = (__HOST_THE__)->frame; \
		__HOST_JUMP__.code = (__HOST_THE__)->code; \
		(__HOST_THE__)->firstJump = &__HOST_JUMP__; \
		if (_setjmp(__HOST_JUMP__.buffer) == 0) { \
			xsMachine* the = fxBeginHost(__HOST_THE__); \
			the->code = _CODE; \
			fxEnterSandbox(__HOST_THE__)
#define xsEndHostSandboxCode() \
			fxEndHost(__HOST_THE__); \
			(__HOST_THE__)->firstJump = __HOST_JUMP__.nextJump; \
		} \
		break; \
	} while(1)
#define xsFindBoolean(_THIS,_ID,_RESULT) \
	(*(--the->stack) = _THIS, ((fxHasOwnID(the, _ID) && (fxTypeOf(the, &(the->scratch)) != xsUndefinedType)) ? ((*(_RESULT) = fxToBoolean(the, &(the->scratch))), true) : false))
#define xsFindInteger(_THIS,_ID,_RESULT) \
	(*(--the->stack) = _THIS, ((fxHasOwnID(the, _ID) && (fxTypeOf(the, &(the->scratch)) != xsUndefinedType)) ? ((*(_RESULT) = fxToInteger(the, &(the->scratch))), true) : false))
#define xsFindNumber(_THIS,_ID,_RESULT) \
	(*(--the->stack) = _THIS, ((fxHasOwnID(the, _ID) && (fxTypeOf(the, &(the->scratch)) != xsUndefinedType)) ? ((*(_RESULT) = fxToNumber(the, &(the->scratch))), true) : false))
#define xsFindResult(_THIS,_ID) \
	(*(--the->stack) = _THIS, \
	(fxHasID(the, _ID) ? (xsResult = the->scratch, true) : false))
#define xsFindString(_THIS,_ID,_RESULT) \
	(*(--the->stack) = _THIS, (fxHasOwnID(the, _ID) ? ((*(_RESULT) = fxToString(the, &(the->scratch))), true) : false))
#define xsCallFunction0(_FUNCTION,_THIS) \
	(xsOverflow(-3), \
	fxInteger(the, --the->stack, 0), \
	*(--the->stack) = (_THIS), \
	*(--the->stack) = (_FUNCTION), \
	fxCall(the), \
	fxPop())
#define xsCallFunction1(_FUNCTION,_THIS,_SLOT0) \
	(xsOverflow(-4), \
	*(--the->stack) = (_SLOT0), \
	fxInteger(the, --the->stack, 1), \
	*(--the->stack) = (_THIS), \
	*(--the->stack) = (_FUNCTION), \
	fxCall(the), \
	fxPop())
#define xsCallFunction2(_FUNCTION,_THIS,_SLOT0,_SLOT1) \
	(xsOverflow(-5), \
	*(--the->stack) = (_SLOT0), \
	*(--the->stack) = (_SLOT1), \
	fxInteger(the, --the->stack, 2), \
	*(--the->stack) = (_THIS), \
	*(--the->stack) = (_FUNCTION), \
	fxCall(the), \
	fxPop())
#define xsCallFunction3(_FUNCTION,_THIS,_SLOT0,_SLOT1,_SLOT2) \
	(xsOverflow(-6), \
	*(--the->stack) = (_SLOT0), \
	*(--the->stack) = (_SLOT1), \
	*(--the->stack) = (_SLOT2), \
	fxInteger(the, --the->stack, 3), \
	*(--the->stack) = (_THIS), \
	*(--the->stack) = (_FUNCTION), \
	fxCall(the), \
	fxPop())
#define xsCallFunction4(_FUNCTION,_THIS,_SLOT0,_SLOT1,_SLOT2,_SLOT3) \
	(xsOverflow(-7), \
	*(--the->stack) = (_SLOT0), \
	*(--the->stack) = (_SLOT1), \
	*(--the->stack) = (_SLOT2), \
	*(--the->stack) = (_SLOT3), \
	fxInteger(the, --the->stack, 4), \
	*(--the->stack) = (_THIS), \
	*(--the->stack) = (_FUNCTION), \
	fxCall(the), \
	fxPop())
#define xsCallFunction5(_FUNCTION,_THIS,_SLOT0,_SLOT1,_SLOT2,_SLOT3,_SLOT4) \
	(xsOverflow(-8), \
	*(--the->stack) = (_SLOT0), \
	*(--the->stack) = (_SLOT1), \
	*(--the->stack) = (_SLOT2), \
	*(--the->stack) = (_SLOT3), \
	*(--the->stack) = (_SLOT4), \
	fxInteger(the, --the->stack, 5), \
	*(--the->stack) = (_THIS), \
	*(--the->stack) = (_FUNCTION), \
	fxCall(the), \
	fxPop())
#define xsCallFunction6(_FUNCTION,_THIS,_SLOT0,_SLOT1,_SLOT2,_SLOT3,_SLOT4,_SLOT5) \
	(xsOverflow(-9), \
	*(--the->stack) = (_SLOT0), \
	*(--the->stack) = (_SLOT1), \
	*(--the->stack) = (_SLOT2), \
	*(--the->stack) = (_SLOT3), \
	*(--the->stack) = (_SLOT4), \
	*(--the->stack) = (_SLOT5), \
	fxInteger(the, --the->stack, 6), \
	*(--the->stack) = (_THIS), \
	*(--the->stack) = (_FUNCTION), \
	fxCall(the), \
	fxPop())
#define xsCallFunction7(_FUNCTION,_THIS,_SLOT0,_SLOT1,_SLOT2,_SLOT3,_SLOT4,_SLOT5,_SLOT6) \
	(xsOverflow(-10), \
	*(--the->stack) = (_SLOT0), \
	*(--the->stack) = (_SLOT1), \
	*(--the->stack) = (_SLOT2), \
	*(--the->stack) = (_SLOT3), \
	*(--the->stack) = (_SLOT4), \
	*(--the->stack) = (_SLOT5), \
	*(--the->stack) = (_SLOT6), \
	fxInteger(the, --the->stack, 7), \
	*(--the->stack) = (_THIS), \
	*(--the->stack) = (_FUNCTION), \
	fxCall(the), \
	fxPop())
#define xsCallFunction8(_FUNCTION,_THIS,_SLOT0,_SLOT1,_SLOT2,_SLOT3,_SLOT4,_SLOT5,_SLOT6,_SLOT7) \
	(xsOverflow(-11), \
	*(--the->stack) = (_SLOT0), \
	*(--the->stack) = (_SLOT1), \
	*(--the->stack) = (_SLOT2), \
	*(--the->stack) = (_SLOT3), \
	*(--the->stack) = (_SLOT4), \
	*(--the->stack) = (_SLOT5), \
	*(--the->stack) = (_SLOT6), \
	*(--the->stack) = (_SLOT7), \
	fxInteger(the, --the->stack, 8), \
	*(--the->stack) = (_THIS), \
	*(--the->stack) = (_FUNCTION), \
	fxCall(the), \
	fxPop())
#define xsZeroSlot(_SLOT) \
	_SLOT.data[0] = _SLOT.data[1] = _SLOT.data[2] = _SLOT.data[3] = NULL

#ifndef KPR_NO_GRAMMAR
	#ifdef KPR_CONFIG
		#include "FskManifest.xs.h"
	#elif FSK_EXTENSION_EMBED
		#if TARGET_OS_WIN32
			#include "..\..\..\tmp\win32\fsk\debug\kprVM.xs.h"
		#elif TARGET_OS_UNITY
			#if SUPPORT_XS_DEBUG
				#include "../../../tmp/unity/fsk/debug/kprVM.xs.h"
			#else
				#include "../../../tmp/unity/fsk/release/kprVM.xs.h"
			#endif
		#elif TARGET_OS_KPL
			#if SUPPORT_XS_DEBUG
				#include "../../../tmp/linux/fsk/debug/kprVM.xs.h"
			#else
				#include "../../../tmp/linux/fsk/release/kprVM.xs.h"
			#endif
		#elif TARGET_OS_IPHONE
			#if defined(__i386__)
				#if SUPPORT_XS_DEBUG
					#include "../../../tmp/iphonesimulator/fsk/debug/kprVM.xs.h"
				#else
					#include "../../../tmp/iphonesimulator/fsk/release/kprVM.xs.h"
				#endif
			#else
				#if SUPPORT_XS_DEBUG
					#include "../../../tmp/iphoneos/fsk/debug/kprVM.xs.h"
				#else
					#include "../../../tmp/iphoneos/fsk/release/kprVM.xs.h"
				#endif
			#endif
		#elif TARGET_OS_MAC
			#if SUPPORT_XS_DEBUG
				#include "../../../tmp/mac/fsk/debug/kprVM.xs.h"
			#else
				#include "../../../tmp/mac/fsk/release/kprVM.xs.h"
			#endif
		#elif TARGET_OS_ANDROID
			#if SUPPORT_XS_DEBUG
				#include "../../../tmp/android/fsk/debug/kprVM.xs.h"
			#else
				#include "../../../tmp/android/fsk/release/kprVM.xs.h"
			#endif
		#endif
	#else
		#undef mxExport
		#define mxExport extern
		#include "kpr.xs.h"
	#endif
#endif

#if TARGET_OS_ANDROID
	#include "FskHardware.h"
#endif /* TARGET_OS */

#include "FskAssociativeArray.h"
#include "FskAudio.h"
#include "FskFiles.h"
#include "FskGrowableStorage.h"
#include "FskThread.h"
#include "FskWindow.h"
#include "FskImage.h"

#include "FskMatrix.h"
typedef float FskSCoordinate;
typedef double FskDCoordinate;

#if TARGET_OS_WIN32
    #define UNUSED
    #include <math.h>
    #include <sys/timeb.h>
    #include <sys/types.h>
    #include <time.h>
    #define M_PI 3.14159265358979323846
#elif TARGET_OS_IPHONE
    #define UNUSED __attribute__ ((unused))
#elif TARGET_OS_MAC
    #define UNUSED __attribute__ ((unused))
#else
    #define UNUSED
#endif

//@@ eventually we should eliminate ASSERT in favor of FskAssert
#define ASSERT(it) FskAssert(it)

#define bailIfError(X) { err = (X); if (err != kFskErrNone) goto bail; }
#define bailIfNULL(X) { if ((X) == NULL) { err = kFskErrMemFull; goto bail; } }

#if SUPPORT_XS_DEBUG
    #define kprTraceDiagnostic(the, _MSG, ...)    \
        do {                                    \
            xsBeginHost(the);             \
            xsTraceDiagnostic(_MSG, __VA_ARGS__);  \
            xsEndHost(the);               \
        } while (0)

    #define bailIfErrorWithDiagnostic(X, the, _MSG, ...) { err = (X); if (kFskErrNone != err) {kprTraceDiagnostic(the, _MSG, __VA_ARGS__); goto bail;} }
#else
    #define kprTraceDiagnostic(the, _MSG, ...)
    #define bailIfErrorWithDiagnostic(X, the, _MSG, ...) bailIfError(X)
#endif

#define kprDumpCache 0
#define kprDumpMemory 0
#define kprDumpMessage 0

enum {
	kprBackgroundTouch = 1 << 0,
	kprHost = 1 << 1,
	kprMessaging = 1 << 2,
	kprVisible = 1 << 3,
	kprActive = 1 << 4,
	kprClip = 1 << 5,
	kprExclusiveTouch = 1 << 6,
	kprMultipleTouch = 1 << 7,
	kprDisplaying = 1 << 8,
	kprHasOwnStyle = 1 << 9,
	kprContainer = 1 << 10,
	kprIdling = 1 << 11,
	kprPlaced = 1 << 12,
	kprXChanged = 1 << 13,
	kprYChanged = 1 << 14,
	kprWidthChanged = 1 << 15,
	kprHeightChanged = 1 << 16,
	kprLayer = 1 << 17,
	kprFocusable = 1 << 18,
	kprPort = 1 << 19,
	kprContentsPlaced = 1 << 20,
	kprContentsHorizontallyChanged = 1 << 21,
	kprContentsVerticallyChanged = 1 << 22,
	kprDisplayed = 1 << 23,

	kprPositionChanged = kprXChanged | kprYChanged,
	kprSizeChanged = kprWidthChanged | kprHeightChanged,
	kprHorizontallyChanged = kprXChanged | kprWidthChanged,
	kprVerticallyChanged = kprYChanged | kprHeightChanged,
	kprContentsChanged = kprContentsHorizontallyChanged | kprContentsVerticallyChanged,

	/* Table, Row, Column */
	kprHorizontal = 1 << 24,
	kprVertical = 1 << 25,

	/* Text */
	kprTextEditable = 1 << 24,
	kprTextHidden = 1 << 25,
	kprTextSelectable = 1 << 26,
	kprTextShowLast = 1 << 27,

	/* Layer */
	kprMatrixChanged = 1 << 24,
	kprBlocking = 1 << 25,
	kprFrozen = 1 << 26,
	kprNoAlpha = 1 << 27,
	kprDirty = 1 << 28,
	kprSubPixel = 1 << 29,
	kprNoAcceleration = 1 << 30,

	/* Scroller */
	kprLooping = 1 << 24,
	kprXScrolled = 1 << 25,
	kprYScrolled = 1 << 26,
	kprTracking = 1 << 27,
	
	kprScrolled = kprXScrolled | kprYScrolled,

	/* Image */
	/* kprMatrixChanged = 1 << 24, */
	kprImageFill = 1 << 25,
	kprImageFit = 1 << 26,

	/* Media */
	kprInsufficientBandwidth = 1 << 24,
	/* kprImageFill = 1 << 25, */
	/* kprImageFit = 1 << 26, */
	kprMediaReady = 1 << 27,
	kprMediaVideo = 1 << 28,

	/* Window */
	kprAdjusting = 1 << 24,
	kprAssetsChanged = 1 << 25,
	kprCollectGarbage = 1 << 26,
	kprWindowActive = 1 << 27,
	kprQuitting = 1 << 28,
	kprClosing = 1 << 30,
	
	/* Host */
	kprRotating = 1 << 24,
};

enum {
	kprCenter = 0,
	kprLeft = 1,
	kprRight = 2,
	kprLeftRight = 3,
	kprWidth = 4,
	kprLeftRightWidth = 7,
	kprMiddle = 0,
	kprTop = 1,
	kprBottom = 2,
	kprTopBottom = 3,
	kprHeight = 4,
	kprTopBottomHeight = 7,
	kprTemporary = 64
};

    
typedef struct {
	UInt16 horizontal;
	UInt16 vertical;
	SInt32 left;
	SInt32 width;
	SInt32 top;
	SInt32 right;
	SInt32 height;
	SInt32 bottom;
} KprCoordinatesRecord, *KprCoordinates;

typedef struct {
	SInt32 left;
	SInt32 top;
	SInt32 right;
	SInt32 bottom;
} KprMarginsRecord, *KprMargins;

typedef struct {
	UInt32 flags;
	SInt32 left;
	SInt32 top;
	SInt32 right;
	SInt32 bottom;
} KprTilesRecord, *KprTiles;


typedef void (*KprAssetDisposeProc)(void* it);
typedef struct KprAssetStruct KprAssetRecord, *KprAsset;
typedef struct KprSkinStruct KprSkinRecord, *KprSkin;
typedef struct KprColorSkinStruct KprColorSkinRecord, *KprColorSkin;
typedef struct KprSoundStruct KprSoundRecord, *KprSound;
typedef struct KprStyleStruct KprStyleRecord, *KprStyle;
typedef struct KprTextureStruct KprTextureRecord, *KprTexture;
typedef struct KprEffectStruct KprEffectRecord, *KprEffect;

typedef struct KprIdleLinkStruct KprIdleLinkRecord, *KprIdleLink;

typedef struct KprDelegateStruct KprDelegateRecord, *KprDelegate;
typedef struct KprDispatchStruct KprDispatchRecord, *KprDispatch;

typedef struct KprBehaviorStruct KprBehaviorRecord, *KprBehavior;
typedef struct KprTransitionStruct KprTransitionRecord, *KprTransition;

typedef struct KprContentStruct KprContentRecord, *KprContent;
typedef struct KprContainerStruct KprContainerRecord, *KprContainer;
typedef struct KprLayerStruct KprLayerRecord, *KprLayer;
typedef struct KprLayoutStruct KprLayoutRecord, *KprLayout;
typedef struct KprShellStruct KprShellRecord, *KprShell;

typedef struct KprImageStruct KprImageRecord, *KprImage;
typedef struct KprImageCacheStruct KprImageCacheRecord, *KprImageCache;
typedef struct KprImageEntryStruct KprImageEntryRecord, *KprImageEntry;
typedef struct KprImageLinkStruct KprImageLinkRecord, *KprImageLink;
typedef struct KprImageTargetStruct KprImageTargetRecord, *KprImageTarget;
typedef struct KprPictureStruct KprPictureRecord, *KprPicture;
typedef struct KprThumbnailStruct KprThumbnailRecord, *KprThumbnail;

typedef struct KprBrowserStruct KprBrowserRecord, *KprBrowser;

typedef struct KprLabelStruct KprLabelRecord, *KprLabel;
typedef struct KprTextLinkStruct KprTextLinkRecord, *KprTextLink;
typedef struct KprTextStruct KprTextRecord, *KprText;

typedef struct KprURLPartsStruct KprURLPartsRecord, *KprURLParts;

typedef struct KprMessageStruct KprMessageRecord, *KprMessage;
typedef struct KprStreamStruct KprStreamRecord, *KprStream;
typedef struct KprStreamDispatchStruct KprStreamDispatchRecord, *KprStreamDispatch;

typedef struct KprContextStruct KprContextRecord, *KprContext;
typedef struct KprHandlerStruct KprHandlerRecord, *KprHandler;

typedef struct KprStorageStruct KprStorageRecord, *KprStorage;
typedef struct KprStorageEntryStruct KprStorageEntryRecord, *KprStorageEntry;
typedef struct KprStorageEntryDispatchStruct KprStorageEntryDispatchRecord, *KprStorageEntryDispatch;

typedef struct KprHTTPClientMessageStruct KprHTTPClientMessageRecord, *KprHTTPClientMessage;

typedef struct KprHTTPCacheStruct KprHTTPCacheRecord, *KprHTTPCache;
typedef struct KprHTTPCacheValueStruct KprHTTPCacheValueRecord, *KprHTTPCacheValue;
typedef struct KprHTTPCookiesStruct KprHTTPCookiesRecord, *KprHTTPCookies;
typedef struct KprHTTPCookieStruct KprHTTPCookieRecord, *KprHTTPCookie;
typedef struct KprHTTPKeychainStruct KprHTTPKeychainRecord, *KprHTTPKeychain;

typedef struct KprHTTPServerStruct KprHTTPServerRecord, *KprHTTPServer;
typedef struct KprHTTPServerMessageStruct KprHTTPServerMessageRecord, *KprHTTPServerMessage;
typedef struct KprHTTPServerReceiverDispatchStruct KprHTTPServerReceiverDispatchRecord, *KprHTTPServerReceiverDispatch;
typedef struct KprHTTPServerReceiverStruct KprHTTPServerReceiverRecord, *KprHTTPServerReceiver;
typedef struct KprHTTPServerSenderDispatchStruct KprHTTPServerSenderDispatchRecord, *KprHTTPServerSenderDispatch;
typedef struct KprHTTPServerSenderStruct KprHTTPServerSenderRecord, *KprHTTPServerSender;

#if SUPPORT_INSTRUMENTATION
#define KprSlotPart \
	xsMachine* the; \
	xsSlot slot; \
	FskInstrumentedItemRecord _instrumented
#else
#define KprSlotPart \
	xsMachine* the; \
	xsSlot slot
#endif

#define KprAssetPart \
	KprContext context; \
	KprAsset next; \
	KprAssetDisposeProc dispose; \
	UInt32 usage

#if SUPPORT_INSTRUMENTATION
#define KprStreamPart \
	KprStreamDispatch dispatch; \
	FskInstrumentedItemRecord _instrumented
#else
#define KprStreamPart \
	KprStreamDispatch dispatch
#endif

#define KprTimerPart \
	KprDispatch dispatch; \
	KprIdleLink idleLink; \
	double duration; \
	double interval; \
	double ticks; \
	double time

#define KprContentPart \
	KprTimerPart; \
	KprBehavior behavior; \
	KprShell shell; \
	KprContainer container; \
	KprContent previous; \
	KprContent next; \
	UInt32 flags; \
	FskRectangleRecord bounds; \
	KprCoordinatesRecord coordinates; \
	KprSkin skin; \
	KprStyle style; \
	double state; \
	SInt32 variant; \
	char* name

#define KprContainerPart \
	KprContent first; \
	KprContent last; \
	KprTransition transition; \
	FskRectangleRecord hole

#define KprContextPart \
	char* url; \
	char* id; \
	KprHandler firstHandler; \
	KprHandler lastHandler; \
	KprAsset firstSkin; \
	KprAsset firstSound; \
	KprAsset firstStyle; \
	KprAsset firstTexture; \
	KprAsset firstEffect \

#define KprImagePart \
	KprImageEntry entry; \
	char* url; \
	char* mime;  \
	FskRectangleRecord crop

#define KprPortPart \
	KprImageEntry entry; \
	char* url; \
	char* mime;  \
	FskRectangleRecord crop

/* sizeof(KprContainerPart) == sizeof(KprImagePart) */

#define KprLayerPart \
	FskDPoint2D corners[4]; \
	KprEffect effect; \
	FskDCoordinate opacity; \
	FskDPoint2D origin; \
	FskDCoordinate rotation; \
	FskDPoint2D scale; \
	FskDPoint2D skew; \
	FskDPoint2D translation; \
	FskDCoordinate matrix[3][3]; \
	FskDCoordinate *hitMatrix

extern void xsSlotToKprCoordinates(xsMachine *the, xsSlot* slot, KprCoordinates coordinates);
typedef struct KprContentLinkStruct KprContentLinkRecord, *KprContentLink;
typedef struct KprContentChainStruct KprContentChainRecord, *KprContentChain;

#define KprContentLinkPart \
	KprContentLink next; \
	KprContent content

struct KprContentLinkStruct {
	KprContentLinkPart;
};
struct KprContentChainStruct {
	KprContentLink first;
	KprContentLink next;
};

extern KprShell gShell;
extern KprImageCache gPictureImageCache;
extern KprImageCache gThumbnailImageCache;

#if SUPPORT_INSTRUMENTATION
#define kprVolatileConstructor(_CONSTRUCTOR) \
	self->the = the; \
	self->slot = xsThis; \
	xsSetHostData(xsThis, self); \
	(void)xsCall1(xsGet(xsGlobal, xsID_Object), xsID_seal, xsThis); \
	FskInstrumentedItemSendMessageNormal(self, kprInstrumentedVolatileConstruct, self)
#define kprContentConstructor(_CONSTRUCTOR) \
	self->the = the; \
	self->slot = xsThis; \
	xsSetHostData(xsThis, self); \
	(void)xsCall1(xsGet(xsGlobal, xsID_Object), xsID_seal, xsThis); \
	FskInstrumentedItemSendMessageNormal(self, kprInstrumentedVolatileConstruct, self)
#define kprVolatileDestructor(_DESTRUCTOR) \
	FskInstrumentedItemSendMessageNormal(self, kprInstrumentedVolatileDestruct, self); \
	self->the = NULL; \
	xsZeroSlot(self->slot)
#define kprVolatileGetter(_VOLATILE,_ID) \
	((_VOLATILE) \
		? (((_VOLATILE)->the) \
			? (_VOLATILE)->slot \
			: ((_VOLATILE)->the = the, \
			(_VOLATILE)->slot = xsNewInstanceOf(xsGet(xsGet(xsGlobal, xsID_KPR), _ID)), \
			xsSetHostData((_VOLATILE)->slot, _VOLATILE), \
			(void)xsCall1(xsGet(xsGlobal, xsID_Object), xsID_seal, (_VOLATILE)->slot), \
			(FskInstrumentedItemHasListenersForLevel(_VOLATILE, kFskInstrumentationLevelNormal)) ? FskInstrumentedItemSendMessageForLevel(_VOLATILE, kprInstrumentedVolatileReconstruct, _VOLATILE, kFskInstrumentationLevelNormal) : 0, \
			(_VOLATILE)->slot)) \
		: xsNull)
#define kprContentGetter(_VOLATILE) \
	((_VOLATILE) \
		? (((_VOLATILE)->the) \
			? (_VOLATILE)->slot \
			: ((_VOLATILE)->the = the, \
			(_VOLATILE)->slot = xsNewInstanceOf(xsGet(xsGet(xsGlobal, xsID_KPR), xsID((_VOLATILE)->dispatch->type))), \
			xsSetHostData((_VOLATILE)->slot, _VOLATILE), \
			(void)xsCall1(xsGet(xsGlobal, xsID_Object), xsID_seal, (_VOLATILE)->slot), \
			(FskInstrumentedItemHasListenersForLevel(_VOLATILE, kFskInstrumentationLevelNormal)) ? FskInstrumentedItemSendMessageForLevel(_VOLATILE, kprInstrumentedVolatileReconstruct, _VOLATILE, kFskInstrumentationLevelNormal) : 0, \
			(_VOLATILE)->slot)) \
		: xsNull)
#define FskInstrumentedItemClearOwner(obj) if (obj) FskInstrumentedItemSetOwner_(&(obj)->_instrumented, NULL)
enum {
	kprInstrumentedModuleRequire = kFskInstrumentedItemFirstCustomMessage,
	kprInstrumentedVolatileConstruct,
	kprInstrumentedVolatileDestruct,
	kprInstrumentedVolatileReconstruct,
	kprInstrumentedContentBeginCollect,
	kprInstrumentedContentBeginPurge,
	kprInstrumentedContentClose,
	kprInstrumentedContentCreateMachine,
	kprInstrumentedContentDeleteMachine,
	kprInstrumentedContentEndCollect,
	kprInstrumentedContentEndPurge,
	kprInstrumentedContentPutBehavior,
	kprInstrumentedContentRemoveBehavior,
	kprInstrumentedAssetBind,
	kprInstrumentedAssetUnbind,
	kprInstrumentedImageCacheGet,
	kprInstrumentedImageCachePut,
	kprInstrumentedImageCacheRemove,
	kprInstrumentedImageEntryAdd,
	kprInstrumentedImageEntryRemove,
	kprInstrumentedTransitionBind,
	kprInstrumentedTransitionUnbind,
	kprInstrumentedContentCallBehavior,
	kprInstrumentedTextDumpBlock,
	kprInstrumentedTextDumpLine,
	kprInstrumentedTextDumpRun,
	kprInstrumentedWindowInvalidated,
	kprInstrumentedMessageCancel,
	kprInstrumentedMessageComplete,
	kprInstrumentedMessageConstruct,
	kprInstrumentedMessageDestruct,
	kprInstrumentedMessageInvoke,
	kprInstrumentedMessageNotify,
	kprInstrumentedMessageRedirect,
	kprInstrumentedMessageResume,
	kprInstrumentedMessageSuspend,
	kprInstrumentedMessageHTTPBegin,
	kprInstrumentedMessageHTTPContinue,
	kprInstrumentedMessageHTTPEnd,
	kprInstrumentedMessageLibraryBegin,
	kprInstrumentedMessageLibraryEnd,
	kprInstrumentedHTTPConnectionAvailable,
	kprInstrumentedHTTPConnectionCandidate,
	kprInstrumentedHTTPConnectionClose,
	kprInstrumentedHTTPConnectionComplete,
	kprInstrumentedHTTPConnectionFinished,
	kprInstrumentedHTTPConnectionHeaders,
	kprInstrumentedHTTPConnectionOpen,
	kprInstrumentedHTTPConnectionProcess,
	kprInstrumentedHTTPConnectionReceiving,
	kprInstrumentedHTTPConnectionSending,
	kprInstrumentedHTTPCacheValueDisposeData,
	kprInstrumentedHTTPCacheValueLock,
	kprInstrumentedHTTPCacheValueRemove,
	kprInstrumentedHTTPCacheValueUnlock,
	kprInstrumentedTextureLoad,
	kprInstrumentedTextureUnload,
	kprInstrumentedContainerReflowing,
	kprInstrumentedLayerUpdate,
	kprInstrumentedStyleFormat,
	kprInstrumentedTransitionLink,
	kprInstrumentedTransitionUnlink,
	kprInstrumentedLayerBitmapNew,
	kprInstrumentedLayerBitmapDispose,
	kprInstrumentedLayerBitmapRelease
};
extern Boolean KprInsrumentationFormatMessage(FskInstrumentedType dispatch, UInt32 msg, void *msgData, char *buffer, UInt32 bufferSize);
#else
#define kprVolatileConstructor(_CONSTRUCTOR) \
	self->the = the; \
	self->slot = xsThis; \
	xsSetHostData(xsThis, self); \
	(void)xsCall1(xsGet(xsGlobal, xsID_Object), xsID_seal, xsThis)
#define kprContentConstructor(_CONSTRUCTOR) \
	self->the = the; \
	self->slot = xsThis; \
	xsSetHostData(xsThis, self); \
	(void)xsCall1(xsGet(xsGlobal, xsID_Object), xsID_seal, xsThis)
#define kprVolatileDestructor(_DESTRUCTOR) \
	self->the = NULL; \
	xsZeroSlot(self->slot)
#define kprVolatileGetter(_VOLATILE,_ID) \
	((_VOLATILE) \
		? (((_VOLATILE)->the) \
			? (_VOLATILE)->slot \
			: ((_VOLATILE)->the = the, \
			(_VOLATILE)->slot = xsNewInstanceOf(xsGet(xsGet(xsGlobal, xsID_KPR), _ID)), \
			xsSetHostData((_VOLATILE)->slot, _VOLATILE), \
			(void)xsCall1(xsGet(xsGlobal, xsID_Object), xsID_seal, (_VOLATILE)->slot), \
			(_VOLATILE)->slot)) \
		: xsNull)
#define kprContentGetter(_VOLATILE) \
	((_VOLATILE) \
		? (((_VOLATILE)->the) \
			? (_VOLATILE)->slot \
			: ((_VOLATILE)->the = the, \
			(_VOLATILE)->slot = xsNewInstanceOf(xsGet(xsGet(xsGlobal, xsID_KPR), xsID((_VOLATILE)->dispatch->type))), \
			xsSetHostData((_VOLATILE)->slot, _VOLATILE), \
			(void)xsCall1(xsGet(xsGlobal, xsID_Object), xsID_seal, (_VOLATILE)->slot), \
			(_VOLATILE)->slot)) \
		: xsNull)
#define FskInstrumentedItemClearOwner(obj)
#endif

#ifdef mxDebug
extern void* KprGetHostData(xsMachine* the, xsSlot* slot, xsIndex index, char* param, char* id);
extern void* KprGetHostData2(xsMachine* the, xsSlot* slot, xsIndex index, xsIndex index2, char* param, char* id, char* id2);
#define kprGetHostData(_SLOT, _PARAM, _ID) KprGetHostData(the, &(_SLOT), xsID_##_ID, #_PARAM, #_ID)
#define kprGetHostData2(_SLOT, _PARAM, _ID, _ID2) KprGetHostData2(the, &(_SLOT), xsID_##_ID, xsID_##_ID2, #_PARAM, #_ID, #_ID2)
#else
#define kprGetHostData(_SLOT, _PARAM, _ID) xsGetHostData(_SLOT)
#define kprGetHostData2(_SLOT, _PARAM, _ID, _ID2) xsGetHostData(_SLOT)
#endif

#if SUPPORT_XS_DEBUG
    FskAPI(Boolean) KprParseColor(xsMachine *the, char* s, FskColorRGBA color);
#else
    FskAPI(Boolean) KprParseColor_(char* s, FskColorRGBA color);
    #define KprParseColor(the, s, color) KprParseColor_(s, color)
#endif
extern UInt32 KprEnvironmentGetUInt32(char* key, UInt32 it);
extern FskErr KprModulesBasesSetup(char* url, char* path);
extern void KprModulesBasesCleanup(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
