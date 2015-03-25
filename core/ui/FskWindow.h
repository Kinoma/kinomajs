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
#ifndef __FSKWINDOW__
#define __FSKWINDOW__

#include "FskPort.h"
#include "FskEvent.h"
#include "FskThread.h"

#if defined(__FSKWINDOW_PRIV__)
#if SUPPORT_EXTERNAL_SCREEN
FskAPI(void) FskExtScreenHandleConnected(int identifier, FskDimension size);
FskAPI(void) FskExtScreenHandleDisconnected(int identifier);
FskAPI(void) FskExtScreenHandleChanged(int identifier, FskDimension newSize);
#endif	/* SUPPORT_EXTERNAL_SCREEN */
#endif	/* __FSKWINDOW_PRIV__ */

#if defined(__FSKWINDOW_PRIV__) || SUPPORT_INSTRUMENTATION
	// implementation headers
	#if TARGET_OS_WIN32
		#include <Windows.h>
		#if TRY_WINCE_DD
			#include "ddraw.h"
		#endif /* TRY_WINCE_DD */
	#endif /* TARGET_OS_MAC */
#endif /* __FSKWINDOW_PRIV__ */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct FskWindowRecord FskWindowRecord;
typedef struct FskWindowRecord *FskWindow;
typedef const struct FskWindowRecord *FskConstWindow;

typedef Boolean (*FskWindowEventHandler)(FskEvent event, UInt32 eventCode,
							FskWindow window, void *refcon);
#if TARGET_OS_WIN32
	typedef long (*FskWindowHookProc)(HWND hwnd, UINT msg, UINT wParam, LONG lParam, void *refcon);
#endif /* TARGET_OS_WIN32 */

#if !defined(__FSKWINDOW_PRIV__) && !SUPPORT_INSTRUMENTATION
#else /* __FSKWINDOW_PRIV__ */
	#if TARGET_OS_WIN32
		typedef struct FskWindowHookRecord FskWindowHookRecord;
		typedef struct FskWindowHookRecord *FskWindowHook;

		struct FskWindowHookRecord {
			struct FskWindowHookRecord		*next;
			FskWindowHookProc				proc;
			UInt32							flags;
			void							*refcon;
		};
	#endif /* TARGET_OS_WIN32 */

	struct FskWindowRecord {
		struct FskWindowRecord		*next;

		FskPort						port;

		Boolean						resizable;
		Boolean						isDisposing;
		Boolean						hasTitleBar;
		Boolean						isDropTarget;
		Boolean						dragUseNativeProxy;
		Boolean						isCustomWindow;
		Boolean						doBeforeUpdate;
		Boolean						doAfterUpdate;

		SInt16						useCount;

		FskInstrumentedItemDeclaration

		SInt32						windowScale;		// scaling to occur in FskWindow
		SInt32						scale;				// combination of port & window scaling
		SInt32						rotation;			// 0, 90, 180, 270 - rotation applied by FskWindow
		SInt32						requestedRotation;	// 0, 90, 180, 270, kFskWindowRotationPortrait, kFskWindowRotationLandscape
		SInt32						rotationDisabled;	// counter. 0 means rotation may be applied

		FskThread					thread;				// thread window was created in

		Boolean						usingGL;
		Boolean						isFullscreenWindow;
        Boolean                     retainsPixelsBetweenUpdates;

	#if TARGET_OS_WIN32
		HWND						hwnd;
		HACCEL						haccel;
		Boolean						ignoreResize;
		Boolean						trackMouseEventInstalled;
		SInt16						dragUseCount;
		FskWindowHook				hooks;

		// drag & drop support
		void						*dropTarget;

		UInt32						updateSeed;
		Boolean						ignoreDeactivate;

		#if TRY_WINCE_DD
			LPDIRECTDRAW				lpDD;
			LPDIRECTDRAWSURFACE			lpFrontBuffer;
			LPDIRECTDRAWSURFACE			lpBackBuffer;
			DWORD						ddWidth;
			DWORD						ddHeight;
			FskRectangleRecord			previousInvalidArea;
			Boolean						bSingleBuffer;
		#endif /* TRY_WINCE_DD */

	#elif TARGET_OS_MAC
		#if TARGET_OS_IPHONE
			void						*uiWindow;
			void						*uiWindowController;
		#else
			void						*nsWindow;
			FskRectangleRecord			lastWindowFrame;
			FskPointRecord				dragResizeMousePoint;
		#endif

	#elif TARGET_OS_KPL
		void						*screenBitmap;
		#if SUPPORT_LINUX_GTK
			void						*gtkWin;
			FskThread					gtkThread;
		#endif
	#endif

	#if TARGET_OS_ANDROID
		FskRectangleRecord			previousInvalidArea;
		//TODO: this should be valid for all platforms except iphone?
		FskMutex					drawPumpMutex;
		UInt32						drawPumpCnt;	//redraw request in thread event Q
	#endif /* TARGET_OS_ANDROID */

		FskRectangleRecord			invalidArea;

		FskBitmap					bits;
		FskRectangleRecord			bounds;						// short-cut for window->bitmap->bounds

		// size constraints
		UInt32						minWidth;
		UInt32						minHeight;
		UInt32						maxWidth;
		UInt32						maxHeight;

		// events
		FskListMutex				eventQueue;

		FskWindowEventHandler		eventHandler;
		void						*eventHandlerRefcon;

		FskTimeCallBack				stillDownTimer;
		FskRectangleRecord			stillDownArea;
		UInt32						stillDownInterval;

		FskTimeCallBack				pressHoldTimer;
		UInt32						pressHoldState;

		// cursor
		UInt32						cursorShape;
		SInt32                      cursorHideCount;

		// deferred updates
		UInt32						updateInterval;			// in ms
		FskTimeCallBack				updateTimer;
		FskTimeRecord				nextUpdate;				// e.g. time that updateTimer is set to fire
		Boolean						updateSuspended;
		Boolean						useFrameBufferUpdate;
	};
#endif /* __FSKWINDOW_PRIV__ */

enum {
	kFskWindowDefault = 0,
	kFskWindowNoTitle = 1L << 0,
	kFskWindowNoResize = 1L << 1,
	kFskWindowModal = 1L << 2,
	kFskWindowFullscreen = 1L << 3,
	kFskWindowCustom = 1L << 4
};

FskAPI(FskErr) FskWindowNew(FskWindow *window, UInt32 width, UInt32 height, UInt32 windowStyle, FskWindowEventHandler handler, void *refcon);
FskAPI(FskErr) FskWindowDispose(FskWindow window);

FskAPI(FskErr) FskWindowSetSize(FskWindow window, UInt32 width, UInt32 height);
FskAPI(FskErr) FskWindowGetSize(FskWindow window, UInt32 *width, UInt32 *height);

FskAPI(FskErr) FskWindowSetLocation(FskWindow window, SInt32 x, SInt32 y);
FskAPI(FskErr) FskWindowGetLocation(FskWindow window, SInt32 *x, SInt32 *y);

FskAPI(FskErr) FskWindowSetTitle(FskWindow window, const char *title);
FskAPI(FskErr) FskWindowGetTitle(FskWindow window, char **title);

FskAPI(FskErr) FskWindowSetVisible(FskWindow window, Boolean visible);
FskAPI(FskErr) FskWindowGetVisible(FskWindow window, Boolean *visible);

FskAPI(FskErr) FskWindowSetSizeConstraints(FskWindow window, UInt32 minWidth, UInt32 minHeight, UInt32 maxWidth, UInt32 maxHeight);
FskAPI(FskErr) FskWindowGetSizeConstraints(FskWindow window, UInt32 *minWidth, UInt32 *minHeight, UInt32 *maxWidth, UInt32 *maxHeight);

FskAPI(FskPort) FskWindowGetPort(FskWindow window);
FskAPI(FskErr) FskWindowGetCursor(FskWindow window, FskPoint pt);
#if TARGET_OS_LINUX
	FskAPI(FskErr) FskWindowSetCursor(FskWindow window, FskPoint pt);
#endif /* TARGET_OS_LINUX */
FskAPI(FskErr) FskWindowShowCursor(FskWindow window);
FskAPI(FskErr) FskWindowHideCursor(FskWindow window);
FskAPI(Boolean) FskWindowCursorIsVisible(FskWindow window);
FskAPI(FskErr) FskWindowMoveCursor(FskWindow window, SInt32 x, SInt32 y);

FskAPI(void) *FskWindowGetNativeWindow(FskWindow window);

FskAPI(FskWindow) FskWindowGetInd(UInt32 index, FskThread inThread);

FskAPI(FskErr) FskWindowGetDPI(FskWindow window, FskPoint pt);

FskAPI(FskErr) FskWindowCopyToBitmap(FskWindow window, const FskRectangle src, Boolean forExport, FskBitmap *bits);

#ifdef __FSKWINDOW_PRIV__
	FskErr FskWindowUpdateRectangle(FskWindow window, const FskRectangle updateArea, Boolean backBufferUnchanged);
	FskAPI(FskErr) FskWindowCopyBitsToWindow(FskWindow window, FskBitmap bits, const FskRectangle src, const FskRectangle dst, UInt32 mode, FskGraphicsModeParameters modeParams);		// mode not guaranteed.. pass kFskGraphicsModeCopy & NULL

	FskErr FskWindowResetInvalidContentRectangle(FskWindow window);
	FskErr FskWindowInvalidateContentRectangle(FskWindow window, const FskRectangle area);

	FskErr FskWindowBeginDrawing(FskWindow window, const FskRectangle bounds);
	FskErr FskWindowEndDrawing(FskWindow window, const FskRectangle bounds);

	#if TARGET_OS_ANDROID || TARGET_OS_WIN32 || TARGET_OS_MAC || (TARGET_OS_KPL && SUPPORT_LINUX_GTK)
		#define FskWindowScaleGet(w) ((w)->windowScale)
		#define FskWindowUIScaleGet(w) ((w)->scale)
		#define FskWindowRotateGet(w) ((w)->rotation)
	#else
		#define FskWindowScaleGet(w) (1)
		#define FskWindowUIScaleGet(w) (1)
		#define FskWindowRotateGet(w) (0)
	#endif
#endif /* __FSKWINDOW_PRIV__ */

FskAPI(FskErr) FskWindowEventSend(FskWindow window, FskEvent event);
FskAPI(FskErr) FskWindowEventQueue(FskWindow window, FskEvent event);
FskAPI(FskEvent) FskWindowEventPeek(FskWindow window, UInt32 eventCode);

FskAPI(FskErr) FskWindowEventSetHandler(FskWindow window, FskWindowEventHandler handler, void *refcon);
FskAPI(FskErr) FskWindowEventGetHandler(FskWindow window, FskWindowEventHandler *handler, void **refcon);

FskAPI(FskErr) FskWindowRequestStillDownEvents(FskWindow window, const FskRectangle r, UInt32 initialDelayMS, UInt32 intervalMS);
FskAPI(FskErr) FskWindowCancelStillDownEvents(FskWindow window);

FskAPI(FskErr) FskWindowRequestDragDrop(FskWindow window);
FskAPI(FskErr) FskWindowCancelDragDrop(FskWindow window);
FskAPI(void) FskWindowDragDropResult(FskWindow window, Boolean success);
FskAPI(FskErr) FskWindowSetUseDragDropNativeProxy(FskWindow window, Boolean useNativeProxy);
FskAPI(FskErr) FskWindowGetUseDragDropNativeProxy(FskWindow window, Boolean *useNativeProxy);

FskAPI(FskErr) FskWindowGetUpdateInterval(FskWindow window, UInt32 *intervalMS);
FskAPI(FskErr) FskWindowSetUpdates(FskWindow window, const Boolean *before, const Boolean *after, UInt32 *updateIntevalMS);

void FskWindowGetNextEventTime(FskThread thread, FskTime nextEventTime);


enum {
	kFskWindowRotationDisable = -1,
	kFskWindowRotationEnable = -2,
	kFskWindowRotationPortrait = -3,
	kFskWindowRotationLandscape = -4
};

FskAPI(FskErr) FskWindowGetRotation(FskWindow window, SInt32 *rotation, SInt32 *aggregate);
FskAPI(FskErr) FskWindowSetRotation(FskWindow window, SInt32 rotation);

#if TARGET_OS_ANDROID
	enum {
		kFskWindowKeyboardModeAlpha = 0,
		kFskWindowKeyboardModeNumeric,
		kFskWindowKeyboardModePassword
	};
#endif /* TARGET_OS_ANDROID */

enum {
	kFskCursorArrow = 0,
	kFskCursorAliasArrow,
	kFskCursorCopyArrow,
	kFskCursorWait,
	kFskCursorIBeam,
	kFskCursorNotAllowed,
	kFskCursorResizeAll,
	kFskCursorResizeLeftRight,
	kFskCursorResizeTopBottom,
	kFskCursorResizeNESW,
	kFskCursorResizeNWSE,
	kFskCursorLink,
	kFskCursorResizeColumn,
	kFskCursorResizeRow
};

FskAPI(FskErr) FskWindowSetCursorShape(FskWindow window, UInt32 cursor);

FskAPI(void) FskWindowUpdate(FskWindow window, const FskTime updateTime);

FskAPI(FskWindow) FskWindowGetActive(void);

FskErr FskWindowInitialize(void);
void FskWindowTerminate(void);

void FskWindowCheckEventQueue(FskWindow window);
void FskWindowCheckUpdate(void);
#if TARGET_OS_LINUX || TARGET_OS_MAC || TARGET_OS_KPL
	Boolean FskWindowCheckEvents(void);
#endif

#ifdef __FSKWINDOW_PRIV__
	#if TARGET_OS_MAC
		#define kEventClassFsk 'FSK '
		#define kEventFskProcessEvent 'FEVT'
		#define kEventParamWindow 'FWIN'

        #define kEventCloseWindowEvent 			'CWIN'
        #define kEventWakeupMainRunloopEvent	'WAKE'

        void FskWindowCocoaSizeChanged(FskWindow win);
        void FskWindowCocoaClose(FskWindow win);
	#endif /* TARGET_OS_MAC */
#endif /* __FSKWINDOW_PRIV__ */

#if TARGET_OS_WIN32
	enum {
		kFskWindowHookCallBefore = 1,
		kFskWindowHookCallAfter = 2
	};
	FskAPI(FskErr) FskWindowHookAdd(FskWindow window, FskWindowHookProc proc, UInt32 flags, void *refcon);
	FskAPI(FskErr) FskWindowHookRemove(FskWindow window, FskWindowHookProc proc, UInt32 flags, void *refcon);

	void FskWindowBlueToothAVCRP(UInt32 opID);
#endif /* TARGET_OS_WIN32 */

#if (TARGET_OS_KPL && SUPPORT_LINUX_GTK)
	void FskWindowGtkClose(FskWindow win);
	void FskWindowGtkSizeChanged(FskWindow win);
#endif
	
#if SUPPORT_INSTRUMENTATION
	enum {
		kFskWindowInstrMsgDispatchEvent = kFskInstrumentedItemFirstCustomMessage,
		kFskWindowInstrMsgDispatchEventDone,
		kFskWindowInstrSetSize,
		kFskWindowInstrSetLocation,
		kFskWindowInstrSetTitle,
		kFskWindowInstrSetVisible,
		kFskWindowInstrCopyBitsToScreen,
		kFskWindowInstrInvalidate,
		kFskWindowInstrWindowsMessage,
		kFskWindowInstrQueueEvent
	};
#endif /* SUPPORT_INSTRUMENTATION*/

#if TARGET_OS_ANDROID
void WindowUpdateCallback(void *win, void *unused1, void *unused2, void *unused3);
#endif

#if SUPPORT_EXTERNAL_SCREEN
	enum {
		kFskExtScreenStatusRemoved = 0,
		kFskExtScreenStatusNew,
		kFskExtScreenStatusChanged
	};

	FskDeclarePrivateType(FskExtScreen);
	typedef void (*FskExtScreenChangedCallback)(UInt32 status, int identifier, FskDimension size, void *param);

#ifndef __FSKWINDOW_PRIV__
	FskDeclarePrivateType(FskExtScreenNotifier)
#else
	typedef struct FskExtScreenNotifierRecord {
		struct FskExtScreenNotifierRecord *next;
		FskExtScreenChangedCallback callback;
		void *param;
		FskThread thread;

		FskInstrumentedItemDeclaration

		char name[1];		// must be last
	} FskExtScreenNotifierRecord, *FskExtScreenNotifier;
#endif

	FskAPI(FskExtScreenNotifier) FskExtScreenAddNotifier(FskExtScreenChangedCallback callback, void *param, char *name);
	FskAPI(void) FskExtScreenRemoveNotifier(FskExtScreenNotifier callbackRef);
#endif	/* SUPPORT_EXTERNAL_SCREEN */
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __FSKWINDOW__ */
