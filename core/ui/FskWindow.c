/*
 *     Copyright (C) 2010-2016 Marvell International Ltd.
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
#define __FSKTHREAD_PRIV__

#define _WIN32_WINNT 0x0400

#define __FSKWINDOW_PRIV__
#define __FSKPORT_PRIV__
#define __FSKBITMAP_PRIV__
#define __FSKEVENT_PRIV__
#define __FSKECMASCRIPT_PRIV__
#include "FskWindow.h"
#include "FskDragDrop.h"
#include "FskECMAScript.h"
#include "FskEnvironment.h"
#include "FskFrameBuffer.h"
#include "FskRotate90.h"

#include <assert.h>

#if FSKBITMAP_OPENGL
	#define GL_GLEXT_PROTOTYPES
	#include "FskGLBlit.h"
	#ifdef __APPLE__
		#if TARGET_OS_IPHONE
			#if GLES_VERSION == 1
				#import <OpenGLES/ES1/gl.h>
			#else /* GLES_VERSION == 2 */
				#import <OpenGLES/ES2/gl.h>
			#endif /* GLES_VERSION == 2 */
		#else
			#include <OpenGL/gl.h>			// Header File For The OpenGL Library
		#endif
	#elif ANDROID || __linux__ || TARGET_OS_KPL || (defined(_MSC_VER) && (FSK_OPENGLES_ANGLE == 1))
		#ifndef GLES_VERSION
			#include "FskGLBlit.h"
		#endif /* GLES_VERSION */
		#if GLES_VERSION == 2
			#include <GLES2/gl2.h>		// Header File For the GLES 2 Library
			#include <GLES2/gl2ext.h>	// Header file for standard GLES 2 extensions.
		#else /* GLES_VERSION == 1 */
			#include <GLES/gl.h>		// Header File For the GLES 1 Library
			#include <GLES/glext.h>		// Header file for standard GLES 1 extensions.
		#endif /* GLES_VERSION */
	#elif _MSC_VER
		#include <gl/gl.h>				// Header File For The OpenGL32 Library
	#else
		#error unknown OS
	#endif /* OS */
#endif /* FSKBITMAP_OPENGL */

#if TARGET_OS_WIN32 || TARGET_OS_MAC || TARGET_OS_LINUX || TARGET_OS_KPL

#include "FskTextConvert.h"
#include "FskUtilities.h"
#include "FskPlatformImplementation.h"

static FskListMutex gWindowList;

#if TARGET_OS_WIN32
	#include "Windowsx.h"
	#include "resource.h"

	static long FAR PASCAL FskWindowWndProc(HWND hwnd, UINT msg, UINT wParam, LONG lParam);
	static long FAR PASCAL FskWindowWndProcNoHook(HWND hwnd, UINT msg, UINT wParam, LONG lParam);

	UINT gEventMessage;
	UINT gFskApplicationMessage;

	static HANDLE gResizeColumnCursor, gResizeRowCursor;
#elif TARGET_OS_MAC
    #if TARGET_OS_IPHONE
        #include "FskCocoaSupportPhone.h"
    #else /* !TARGET_OS_IPHONE */
        #include "FskCocoaSupport.h"
        #include <ApplicationServices/ApplicationServices.h>
    #endif /* !TARGET_OS_IPHONE */
#elif TARGET_OS_ANDROID
	#include "FskCursor.h"
	#include "FskHardware.h"
	#include "FskFrameBuffer.h"
#elif TARGET_OS_LINUX
	#include "FskCursor.h"
#elif TARGET_OS_KPL
	#include "FskEventKpl.h"
	#include "KplScreen.h"
	#include "KplUIEvents.h"
#if SUPPORT_LINUX_GTK
	#include "FskGtkWindow.h"
#endif
#endif /* TARGET_OS */

#if GL_DEBUG
	extern FskInstrumentedType gOpenGLTypeInstrumentation;
	#define			LOGD(...)	do { FskInstrumentedTypePrintfDebug  (&gOpenGLTypeInstrumentation, __VA_ARGS__); } while(0)
	#define			LOGI(...)	do { FskInstrumentedTypePrintfVerbose(&gOpenGLTypeInstrumentation, __VA_ARGS__); } while(0)
	#define			LOGE(...)	do { FskInstrumentedTypePrintfMinimal(&gOpenGLTypeInstrumentation, __VA_ARGS__); } while(0)
#else
	#define LOGD(...)
	#define LOGI(...)
	#define LOGE(...)
#endif /* GL_DEBUG */

static Boolean checkWindowBitmapSize(FskWindow win);
static void windowIncrementUseCount(FskWindow win);
static void windowDecrementUseCount(FskWindow win);
static void postProcessEventMsg(FskWindow win);
void rotateAndScalePoints(FskWindow win, FskPointAndTicks pts, UInt32 count);
void sendEventWindowUpdate(FskWindow win, Boolean redrawAll, Boolean skipBeforeUpdate, const FskTime updateTime);		//@@ redrawAll parameter actually should be a rectangle indicating the invalidated portion of the window
static void sendEventWindowSizeChanged(FskWindow win);
#if TARGET_OS_MAC || TARGET_OS_WIN32 || (TARGET_OS_KPL && SUPPORT_LINUX_GTK)
    static void sendEventWindowClose(FskWindow win);
#endif
static void stillDownEvent(struct FskTimeCallBackRecord *callback, const FskTime time, void *param);

static void windowUpdateCallback(struct FskTimeCallBackRecord *callback, const FskTime time, void *param);
static void scheduleWindowUpdateCallback(FskWindow win);

#if TARGET_OS_MAC || TARGET_OS_WIN32 || (TARGET_OS_KPL && SUPPORT_LINUX_GTK)
    static UInt32 getScreenPixelFormat(void);
#endif
#if TARGET_OS_MAC || TARGET_OS_WIN32 || TARGET_OS_KPL
    static void updateWindowRotation(FskWindow window);
#endif
#if FSKBITMAP_OPENGL
	static void setWindowAsSysContext(FskWindow w);
#else
	#define setWindowAsSysContext(w)
#endif
static void setWindowBitmap(FskWindow window, FskBitmap bits);

// static FskErr FskWindowGetScreenBitmap(FskWindow win, FskBitmap *screenBits);

#if SUPPORT_INSTRUMENTATION
	static Boolean doFormatMessageWindow(FskInstrumentedType dispatch, UInt32 msg, void *msgData, char *buffer, UInt32 bufferSize);

	static FskInstrumentedTypeRecord gWindowTypeInstrumentation = {
		NULL,
		sizeof(FskInstrumentedTypeRecord),
		"window",
		FskInstrumentationOffset(FskWindowRecord),
		NULL,
		0,
		NULL,
		doFormatMessageWindow
	};
#endif /* SUPPORT_INSTRUMENTATION */

#define imeActive(win) false

#if TARGET_OS_KPL
	static FskErr KplUIEventHandler(KplUIEvent kplEvent, void *refcon);
#endif /* TARGET_OS_KPL */

FskErr FskWindowNew(FskWindow *windowOut, UInt32 width, UInt32 height, UInt32 windowStyle, FskWindowEventHandler handler, void *refcon)
{
	//@@ need to define style for not-resizable, modal, full screen
	FskErr err = kFskErrNone;
	FskEvent initEvent;
	FskWindow win = NULL;
	char *updateIntervalStr;
	UInt32 updateInterval = 0;

	err = FskMemPtrNewClear(sizeof(FskWindowRecord), &win);
	BAIL_IF_ERR(err);

	win->thread = FskThreadGetCurrent();
	win->hasTitleBar = (windowStyle & (kFskWindowNoTitle | kFskWindowFullscreen)) == 0;
	win->useCount = 1;
	win->dragUseNativeProxy = true;
	if (windowStyle & kFskWindowCustom)
		win->isCustomWindow = true;

#if FSKBITMAP_OPENGL
	{
	const char *value = FskEnvironmentGet("useGL");
	win->usingGL = value && (0 == FskStrCompare("1", value));
	}
#endif /* FSKBITMAP_OPENGL */

	err = FskListMutexNew(&win->eventQueue, "windowEventQueue");
	BAIL_IF_ERR(err);

	err = FskPortNew(&win->port, NULL, NULL);
	BAIL_IF_ERR(err);

	win->windowScale = 1;
	win->scale = (win->windowScale * win->port->scale) >> 16;
	win->rotation = 0;

	win->isFullscreenWindow = ((windowStyle & kFskWindowFullscreen) != 0);

	FskInstrumentedItemNew(win, NULL, &gWindowTypeInstrumentation);
	FskInstrumentedItemSetOwner(win->port, win);

#if TARGET_OS_WIN32
	{
	static Boolean registeredWindowClass = false;
	RECT r;
	DWORD style, styleEx = 0;

	if (false == registeredWindowClass) {
		WNDCLASSEX wc;
		wc.cbSize = sizeof(WNDCLASSEX);
		wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
		wc.lpfnWndProc = FskWindowWndProc;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;
		wc.hInstance = hInst;
#if FSK_WINDOWS_DEFAULT_WINDOW_ICON_ID
		wc.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(FSK_WINDOWS_DEFAULT_WINDOW_ICON_ID));
#else /* !FSK_WINDOWS_DEFAULT_WINDOW_ICON_ID */
		wc.hIcon = NULL;
#endif /* !FSK_WINDOWS_DEFAULT_WINDOW_ICON_ID */
		wc.hIconSm = NULL;
		wc.hCursor = LoadCursor(NULL, IDC_ARROW);
		wc.hbrBackground = NULL;
		wc.lpszMenuName = NULL;
		wc.lpszClassName = kFskWindowClassName;
		RegisterClassEx(&wc);

		gEventMessage = RegisterWindowMessageW(L"FskWindowEventMessage");
		gFskApplicationMessage = RegisterWindowMessageW(L"FskApplicationMessage");

		registeredWindowClass = true;
	}

	if (windowStyle & kFskWindowCustom) {
		style = WS_OVERLAPPEDWINDOW | WS_SYSMENU;
	}
	else if (0 == (kFskWindowFullscreen & windowStyle)) {
		style = WS_OVERLAPPEDWINDOW;
		if (windowStyle & kFskWindowNoTitle)
			style = WS_POPUP;
		if (windowStyle & kFskWindowNoResize)
			style &= ~(WS_SIZEBOX | WS_MAXIMIZEBOX);
	}
	else {
		HWND hWndDesktop = GetDesktopWindow();
		RECT desktopBounds;
		style = WS_POPUP;
		GetClientRect(hWndDesktop, &desktopBounds);
		width = desktopBounds.right - desktopBounds.left;
		height = desktopBounds.bottom - desktopBounds.top;
	}

	r.top = 0;
	r.left = 0;
	r.right = width;
	r.bottom = height;
	AdjustWindowRectEx(&r, style, (0 == ((kFskWindowCustom | kFskWindowFullscreen) & windowStyle)), styleEx);

	win->hwnd = CreateWindowEx(styleEx, kFskWindowClassName, "window",
							style | WS_CLIPCHILDREN,
							CW_USEDEFAULT, CW_USEDEFAULT,
							r.right - r.left, r.bottom - r.top,
							NULL, NULL, hInst, NULL);
	SetWindowLongPtr(win->hwnd, GWL_USERDATA, (LONG_PTR)win);

	#if (1 == FSK_OPENGLES_ANGLE)
		FskGLSetNativeWindow(win->hwnd);
	#endif
	}
#elif TARGET_OS_MAC
    #if TARGET_OS_IPHONE
        if (!FskCocoaWindowCreate(win, (windowStyle & kFskWindowCustom), (windowStyle & kFskWindowFullscreen), width, height))
            BAIL(kFskErrMemFull);
		float scale;
		FskCocoaWindowGetScreenScale(win, &scale);
		if (1.0 < scale)
			FskPortScaleSet(win->port, FskRoundFloatToFixed(scale));
    #else /* !TARGET_OS_IPHONE */
        if (!FskCocoaWindowCreate(win, (windowStyle & kFskWindowCustom), width, height))
            BAIL(kFskErrMemFull);
    #endif /* !TARGET_OS_IPHONE */
#elif TARGET_OS_ANDROID
{
	FskRectangleRecord r;
	int densityDpi;

	win->isFullscreenWindow = true;
	(void)FskFrameBufferGetScreenBounds(&r);
	width = r.width;
	height = r.height;
	// FskInstrumentedItemPrintfDebug(win, "android - new window (FULLSCREEN): width: %d height %d", width, height);

	gAndroidCallbacks->getDPICB(NULL, NULL, &densityDpi);

	if (densityDpi > 420) {
		FskPortScaleSet(win->port, FskIntToFixed(3));
		win->scale = (win->port->scale * win->windowScale) >> 16;
		// FskInstrumentedItemPrintfDebug(win, "densityDpi: %d scale 3", densityDpi);
	}
	else if (densityDpi >= 320) {		// DENSITY_XHIGH
		FskPortScaleSet(win->port, FskIntToFixed(2));
		win->scale = (win->port->scale * win->windowScale) >> 16;
		// FskInstrumentedItemPrintfDebug(win, "densityDpi: %d scale 2", densityDpi);
	}
	else if ((densityDpi < 240) 	// DENSITY_LOW or _MEDIUM
			&& ((width <= 480) && (height <= 480))) {	// TABLET HACK
		FskPortScaleSet(win->port, FskIntToFixed(1));
		win->scale = (win->port->scale * win->windowScale) >> 16;
		// FskInstrumentedItemPrintfDebug(win, "densityDpi: %d scale 1", densityDpi);
	}
	else {					// DENSITY_HIGH
		FskPortScaleSet(win->port, FskIntToFixed(3) >> 1);
		win->scale = (win->port->scale * win->windowScale) >> 16;
		// FskInstrumentedItemPrintfDebug(win, "densityDpi: %d scale 1.5", densityDpi);
	}
}
#elif (TARGET_OS_KPL && SUPPORT_LINUX_GTK)
	FskGtkWindowCreate(win, windowStyle, width, height);
#endif /* TARGET_OS_ANDROID */

	checkWindowBitmapSize(win);

	win->port->window = win;
	if (NULL != win->bits) {
//		win->port->invalidArea = win->bits->bounds;
        FskWindowInvalidateContentRectangle(win, &win->bits->bounds);
	}

	FskWindowEventSetHandler(win, handler, refcon);

#if TARGET_OS_KPL
	#if !SUPPORT_LINUX_GTK
	KplUIEventSetCallback(KplUIEventHandler, win);
	#endif
#endif /* TARGET_OS_KPL */

	// post the initialize event
	err = FskEventNew(&initEvent, kFskEventWindowInitialize, NULL, kFskEventModifierNotSet);
	BAIL_IF_ERR(err);

	FskWindowEventQueue(win, initEvent);

	FskListMutexPrepend(gWindowList, win);

	// initialize frame rate
    win->retainsPixelsBetweenUpdates = !win->usingGL;

#if !TARGET_OS_MAC || !USE_DISPLAY_LINK	/* Mac & iOS uses DisplayLink to update the window */
	{
		Boolean get, set;
		UInt32 dataType;
        FskMediaPropertyValueRecord property;

		win->useFrameBufferUpdate = (kFskErrNone == FskFrameBufferHasProperty(kFskFrameBufferPropertyContinuousDrawing, &get, &set, &dataType));

		if (!win->useFrameBufferUpdate) {
			FskTimeCallbackUINew(&win->updateTimer);
			FskTimeGetNow(&win->nextUpdate);
			FskTimeCallbackSet(win->updateTimer, &win->nextUpdate, windowUpdateCallback, win);
		}

        if (!win->usingGL) {
            if (kFskErrNone == FskFrameBufferGetProperty(kFskFrameBufferPropertyRetainsPixelsBetweenUpdates, &property))
                win->retainsPixelsBetweenUpdates = property.value.b;
        }

#if TARGET_OS_ANDROID
		FskMutexNew(&win->drawPumpMutex, "Drawpump");
		win->drawPumpCnt = 0;
#endif
	}
#endif
	updateIntervalStr = FskEnvironmentGet("updateIntervalMS");
	if (NULL != updateIntervalStr)
		updateInterval = FskStrToNum(updateIntervalStr);
	FskWindowSetUpdates(win, NULL, NULL, &updateInterval);

bail:
	if (kFskErrNone != err) {
		FskWindowDispose(win);
		win = NULL;
	}

	if (NULL != windowOut)
		*windowOut = win;

	return err;
}

FskErr FskWindowDispose(FskWindow win)
{
	if (NULL != win) {
		if (win->isDisposing)
			return kFskErrNone;

		win->useCount -= 1;
		if (win->useCount > 0) {
			win->eventHandler = NULL;		// no more callbacks should happen afte client has called dispose, even if window isn't quite disposed yet
			FskWindowSetVisible(win, false);
			return kFskErrNone;
		}

		win->isDisposing = true;

		FskTimeCallbackDispose(win->stillDownTimer);
		FskTimeCallbackDispose(win->pressHoldTimer);
		if (win->isDropTarget)
			FskWindowCancelDragDrop(win);

		FskTimeCallbackDispose(win->updateTimer);

		// note hooks are not removed in dispose any longer to avoid
		//  dispose order problems.

#if TARGET_OS_WIN32
		DestroyAcceleratorTable(win->haccel);

		SetWindowLongPtr(win->hwnd, GWL_USERDATA, (LONG_PTR)NULL);
		DestroyWindow(win->hwnd);
#elif TARGET_OS_MAC
    FskCocoaWindowDispose(win);
#elif (TARGET_OS_KPL && SUPPORT_LINUX_GTK)
	FskGtkWindowDispose(win);
#endif /* TARGET_OS */

		while (true) {
			FskEvent toRemove = (FskEvent)FskListMutexRemoveFirst(win->eventQueue);
			if (NULL == toRemove)
				break;
			FskEventDispose(toRemove);
		}

		FskListMutexDispose(win->eventQueue);

		FskPortDispose(win->port);
#if !TARGET_OS_KPL
		// @@ KplScreen owns win->bits and disposes the bitmap
		FskBitmapDispose(win->bits);
#endif
		FskBitmapDispose(win->rotationBits);
#if TARGET_OS_ANDROID
		FskMutexDispose(win->drawPumpMutex);
#endif
		FskInstrumentedItemDispose(win);

		FskListMutexRemove(gWindowList, win);

		FskMemPtrDispose(win);

	}

	return kFskErrNone;
}

FskErr FskWindowSetSize(FskWindow window, UInt32 width, UInt32 height)
{
#if TARGET_OS_WIN32 || TARGET_OS_MAC || (TARGET_OS_KPL && SUPPORT_LINUX_GTK)
	SInt32 scale = FskWindowUIScaleGet(window);
	SInt32 rotate = FskWindowRotateGet(window);
#endif /* TARGET_OS_WIN32, || TARGET_OS_MAC */
#if TARGET_OS_WIN32
	RECT r;
	WINDOWINFO wi;
    DWORD dwStyle;

	if (window->isFullscreenWindow)
		return kFskErrNone;

	width = FskPortUInt32Scale(window->port, width) * scale;
	height = FskPortUInt32Scale(window->port, height) * scale;
	if ((90 == rotate) || (270 == rotate)) {
		UInt32 temp = width;
		width = height;
		height = temp;
	}

	GetWindowInfo(window->hwnd, &wi);
	dwStyle = wi.dwStyle;

	r.top = 0;
	r.left = 0;
	r.right = width;
	r.bottom = height;
	if (!window->isCustomWindow)
		AdjustWindowRectEx(&r, dwStyle, false, GetWindowLong(window->hwnd, GWL_EXSTYLE));

	SetWindowPos(window->hwnd, NULL, 0, 0, r.right - r.left, r.bottom - r.top,
					SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER);
#elif TARGET_OS_MAC
    width *= scale;
    height *= scale;
    if ((90 == rotate) || (270 == rotate)) {
        UInt32 temp = width;
        width = height;
        height = temp;
    }

    FskCocoaWindowSetSize(window, FskPortUInt32Scale(window->port, width), FskPortUInt32Scale(window->port, height));
#elif (TARGET_OS_KPL && SUPPORT_LINUX_GTK)
    width *= scale;
    height *= scale;
    if ((90 == rotate) || (270 == rotate)) {
        UInt32 temp = width;
        width = height;
        height = temp;
    }

	FskGtkWindowSetSize(window->gtkWin, width, height);
#elif TARGET_OS_LINUX
	// deliberately fall thorugh so that window offscreen is resized to match frame buffer size
	if (window->isFullscreenWindow)
		return kFskErrNone;
#endif /* TARGET_OS */

	FskInstrumentedItemSendMessage(window, kFskWindowInstrSetSize, NULL);

	checkWindowBitmapSize(window);

	return kFskErrNone;
}

FskErr FskWindowGetUnscaledSize(FskWindow window, UInt32 *width, UInt32 *height)
{
	SInt32 rotate = FskWindowRotateGet(window);

#if USE_FRAMEBUFFER_VECTORS
	FskRectangleRecord r;

	if (kFskErrNone == FskFrameBufferGetScreenBounds(&r)) {
		*width = r.width;
		*height = r.height;
	}
	else {
		*width = 640;
		*height = 480;
	}
#elif TARGET_OS_WIN32
	RECT r;

	GetClientRect(window->hwnd, &r);
	*width = r.right - r.left;
	*height = r.bottom - r.top;
#elif TARGET_OS_MAC
    FskCocoaWindowGetSize(window, width, height);
#elif TARGET_OS_KPL
	#if !SUPPORT_LINUX_GTK
	{
	KplBitmap kplBitmap;
	KplScreenGetBitmap(&kplBitmap);
	*width = kplBitmap->width;
	*height = kplBitmap->height;
	}
	#else
	FskGtkWindowGetSize(window->gtkWin, width, height);
	#endif
#endif /* TARGET_OS */

	if ((90 == rotate) || (270 == rotate)) {
		UInt32 temp = *width;
		*width = *height;
		*height = temp;
	}

	return kFskErrNone;
}

FskErr FskWindowGetSize(FskWindow window, UInt32 *width, UInt32 *height)
{
	FskErr err = FskWindowGetUnscaledSize(window, width, height);
	if (kFskErrNone == err) {
		SInt32 scale = FskWindowScaleGet(window);
		*width = FskPortUInt32Unscale(window->port, *width) / scale;
		*height = FskPortUInt32Unscale(window->port, *height) / scale;
	}

	return err;
}

FskErr FskWindowSetLocation(FskWindow window, SInt32 x, SInt32 y)
{
	x = FskPortSInt32Scale(window->port, x);
	y = FskPortSInt32Scale(window->port, y);
#if TARGET_OS_WIN32
	SetWindowPos(window->hwnd, NULL, x, y, 0, 0, SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER);
#elif TARGET_OS_MAC
    FskCocoaWindowSetOrigin(window, x, y);
#elif TARGET_OS_KPL && SUPPORT_LINUX_GTK
	FskGtkWindowSetPos(window, x, y);
#endif /* TARGET_OS */

	FskInstrumentedItemSendMessage(window, kFskWindowInstrSetLocation, NULL);

	return kFskErrNone;
}

FskErr FskWindowGetLocation(FskWindow window, SInt32 *x, SInt32 *y)
{
#if TARGET_OS_WIN32
	RECT r;

	GetWindowRect(window->hwnd, &r);
	*x = r.left;
	*y = r.top;
#elif TARGET_OS_MAC
    FskCocoaWindowGetOrigin(window, x, y);
#elif TARGET_OS_KPL && SUPPORT_LINUX_GTK
	FskGtkWindowGetPos(window, x, y);
#else
	*x = 0;
	*y = 0;
#endif /* TARGET_OS */

	*x = FskPortSInt32Unscale(window->port, *x);
	*y = FskPortSInt32Unscale(window->port, *y);

	return kFskErrNone;
}

FskErr FskWindowSetTitle(FskWindow window, const char *titleIn)
{
	char *title = NULL;

#if TARGET_OS_WIN32
	FskTextToPlatform(titleIn, FskStrLen(titleIn), &title, NULL);
	SetWindowText(window->hwnd, title);
#elif TARGET_OS_MAC && !TARGET_OS_IPHONE
    FskCocoaWindowSetTitle(window, titleIn);
#elif (TARGET_OS_KPL && SUPPORT_LINUX_GTK)
	FskGtkWindowSetTitle(window, titleIn);
#endif /* TARGET_OS */
	FskMemPtrDispose(title);
//@@	FskInstrumentedItemSetName(window, titleIn);		//@@ need to make a copy
	FskInstrumentedItemSendMessage(window, kFskWindowInstrSetTitle, (void *)titleIn);
	return kFskErrNone;
}

FskErr FskWindowGetTitle(FskWindow window, char **titleOut)
{
	FskErr err = kFskErrNone;
	char *title = NULL;
#if USE_FRAMEBUFFER_VECTORS
	title = FskStrDoCopy ("Framebuffer");
#elif TARGET_OS_WIN32
	int titleLen = GetWindowTextLength(window->hwnd);

	err = FskMemPtrNewClear(titleLen + 1, (FskMemPtr *)&title);
	BAIL_IF_ERR(err);

	if (titleLen)
		GetWindowText(window->hwnd, title, titleLen + 1);
bail:
#elif TARGET_OS_MAC && !TARGET_OS_IPHONE
    FskCocoaWindowGetTitle(window, titleOut);
    return err;
#endif /* TARGET_OS */

	if (title) {
		char *titleTemp;
		FskTextToUTF8(title, FskStrLen(title), &titleTemp, NULL);
		FskMemPtrDispose(title);
		title = titleTemp;
	}
	*titleOut = title;

	return err;
}

FskErr FskWindowSetVisible(FskWindow window, Boolean visible)
{
	FskInstrumentedItemSendMessage(window, kFskWindowInstrSetVisible, (void *)(int)visible);

#if TARGET_OS_WIN32

	window->ignoreDeactivate = visible;			// we get a deactivate on show, of all the bizarre things
	window->ignoreResize += 1;

	ShowWindow(window->hwnd, visible ? SW_SHOW : SW_HIDE);

	window->ignoreDeactivate = false;
	window->ignoreResize -= 1;

#elif TARGET_OS_MAC
    FskCocoaWindowSetVisible(window, visible);
#elif TARGET_OS_KPL && SUPPORT_LINUX_GTK
	FskGtkWindowShow(window, visible);
#endif /* TARGET_OS */

	return kFskErrNone;
}

FskErr FskWindowGetVisible(FskWindow window, Boolean *visible)
{
#if TARGET_OS_WIN32
	*visible = IsWindowVisible(window->hwnd);
#elif TARGET_OS_MAC
    FskCocoaWindowGetVisible(window, visible);
#elif TARGET_OS_LINUX
	*visible = true;
#endif /* TARGET_OS */
	return kFskErrNone;
}

FskErr FskWindowSetSizeConstraints(FskWindow window, UInt32 minWidth, UInt32 minHeight, UInt32 maxWidth, UInt32 maxHeight)
{
	window->minWidth = minWidth;
	window->minHeight = minHeight;
	window->maxWidth = maxWidth;
	window->maxHeight = maxHeight;

	return kFskErrNone;
}

FskErr FskWindowGetSizeConstraints(FskWindow window, UInt32 *minWidth, UInt32 *minHeight, UInt32 *maxWidth, UInt32 *maxHeight)
{
	*minWidth = window->minWidth;
	*minHeight = window->minHeight;
	*maxWidth = window->maxWidth;
	*maxHeight = window->maxHeight;

	return kFskErrNone;
}

FskPort FskWindowGetPort(FskWindow window)
{
	return window->port;
}

FskErr FskWindowGetCursor(FskWindow window, FskPoint ptOut)
{
#if !TARGET_OS_ANDROID && !TARGET_OS_IPHONE
	FskPointAndTicksRecord pts;
#endif /* !TARGET_OS_ANDROID && !TARGET_OS_IPHONE */

	ptOut->x = 0;													/* Assure that the point is set to (0,0), even if an error occurs later */
	ptOut->y = 0;
	if (kFskErrNone == FskFrameBufferGetCursorLocation(ptOut))
		goto done;

	{
#if TARGET_OS_WIN32
	POINT pt;

	if (0 == GetCursorPos(&pt))
		return kFskErrOperationFailed;
	MapWindowPoints(NULL, window->hwnd, &pt, 1);
	ptOut->x = pt.x - window->port->origin.x;
	ptOut->y = pt.y - window->port->origin.y;
#elif TARGET_OS_MAC
    FskCocoaWindowGetMouseLocation(window, ptOut);
#else /* TARGET_OS_UNKNOWN */
	return kFskErrUnimplemented;
#endif /* TARGET_OS */
	}

done:
#if !TARGET_OS_ANDROID && !TARGET_OS_IPHONE
	pts.pt = *ptOut;
	rotateAndScalePoints(window, &pts, 1);
	*ptOut = pts.pt;
#endif /* !TARGET_OS_ANDROID && !TARGET_OS_IPHONE */

	return kFskErrNone;
}

FskErr FskWindowShowCursor(FskWindow window)
{
	if (--window->cursorHideCount != 0)
		return kFskErrNone;
#if TARGET_OS_MAC && !TARGET_OS_IPHONE
	CGAssociateMouseAndMouseCursorPosition(true);
	return CGDisplayShowCursor(kCGDirectMainDisplay) == kCGErrorSuccess ? kFskErrNone : kFskErrOperationFailed;
#else
	return kFskErrUnimplemented;
#endif
}

FskErr FskWindowHideCursor(FskWindow window)
{
	if (window->cursorHideCount++ != 0)
		return kFskErrNone;
#if TARGET_OS_MAC && !TARGET_OS_IPHONE
	CGAssociateMouseAndMouseCursorPosition(false);
	return CGDisplayHideCursor(kCGDirectMainDisplay) == kCGErrorSuccess ? kFskErrNone : kFskErrOperationFailed;
#else
	return kFskErrUnimplemented;
#endif
}

Boolean FskWindowCursorIsVisible(FskWindow window)
{
	return window->cursorHideCount <= 0;
}

FskErr FskWindowMoveCursor(FskWindow window, SInt32 x, SInt32 y)
{
#if TARGET_OS_MAC && !TARGET_OS_IPHONE
	CGPoint point;

	point.x = x;
	point.y = y;
	return CGDisplayMoveCursorToPoint(kCGDirectMainDisplay, point) == kCGErrorSuccess ? kFskErrNone : kFskErrOperationFailed;
#else
	return kFskErrUnimplemented;
#endif
}

void *FskWindowGetNativeWindow(FskWindow window) {
#if TARGET_OS_WIN32
	return window->hwnd;
#elif TARGET_OS_MAC && !TARGET_OS_IPHONE
    return FskCocoaWindowGetWindowRef(window);
#else /* TARGET_OS_UNKNOWN */
	return NULL;
#endif /* TARGET_OS */
}

FskWindow FskWindowGetInd(UInt32 index, FskThread inThread)
{
	FskWindow walker;

	FskMutexAcquire(gWindowList->mutex);

	for (walker = (FskWindow)gWindowList->list; NULL != walker; walker = walker->next) {
		if ((NULL == inThread) || (walker->thread == inThread)) {
			if (0 == index)
				break;
			index -= 1;
		}
	}

	FskMutexRelease(gWindowList->mutex);

	return walker;
}

#if TARGET_OS_ANDROID
FskErr FskWindowCopyBitsToWindow(FskWindow window, FskBitmap bits, const FskRectangle src, const FskRectangle dstIn, UInt32 mode, FskGraphicsModeParameters modeParams)
{
	return kFskErrUnimplemented;
}

#else /* ! TARGET_OS_ANDROID */
FskErr FskWindowCopyBitsToWindow(FskWindow window, FskBitmap bits, const FskRectangle src, const FskRectangle dstIn, UInt32 mode, FskGraphicsModeParameters modeParams)
{
	FskErr err = kFskErrNone;
	FskRectangle dst = (FskRectangle)dstIn;
	FskRectangleRecord dstScaled;
	SInt32 scale = FskWindowScaleGet(window);
#if TARGET_OS_WIN32 || TARGET_OS_MAC
	SInt32 rotate = FskWindowRotateGet(window);
#endif /* TARGET_OS_WIN32 || TARGET_OS_MAC */

	dstScaled.x = dst->x * scale;
	dstScaled.y = dst->y * scale;
	dstScaled.width = dst->width * scale;
	dstScaled.height = dst->height * scale;
	dst = &dstScaled;

	if (window->usingGL)
		return kFskErrUnimplemented;

#if TARGET_OS_KPL
	#if !SUPPORT_LINUX_GTK
	{	/* begin TARGET_OS_KPL block */
		FskBitmap frameBuffer;

		(void)FskFrameBufferGetScreenBitmap(&frameBuffer);
		if (window->port->bits == frameBuffer) {
			if (bits != frameBuffer) {
				err = FskFrameBufferLockSurfaceArea(frameBuffer, dst, NULL, NULL);
				if (kFskErrNone == err) {
					err = FskBitmapDraw(bits, src, frameBuffer, dst, NULL, NULL, mode, modeParams);
					{	FskErr e = FskFrameBufferUnlockSurface(frameBuffer);
						if (kFskErrNone == err)	/* First error takes precedence */
							err = e;
					}
				}
			}
			else {
				// If the source bitmap is the frame buffer bitmap, we don't need to draw anything
				// because the bits have effectively already been copied to the window.  This is
				// because the window port's bitmap is the frame buffer, and the window port's bitmap
				// has already been drawn into before we get here.
			}
		}
	}	/* end TARGET_OS_KPL block */

	#else
		FskGtkWindowUpdateDa(window->gtkWin);
	#endif

#elif USE_FRAMEBUFFER_VECTORS
	{	/* begin USE_FRAMEBUFFER_VECTORS block */
		FskBitmap theFrameBuffer = NULL;

		FskInstrumentedItemSendMessageVerbose(window, kFskWindowInstrCopyBitsToScreen, src);

		(void)FskFrameBufferGetScreenBitmap(&theFrameBuffer);

		if (theFrameBuffer) {
			(void)FskFrameBufferHideCursor(dst);
			err = FskBitmapDraw(bits, src, theFrameBuffer, dst, NULL, NULL, mode, modeParams);
			(void)FskFrameBufferShowCursor(dst);
		}
		else {
			err = FskFrameBufferDisplayWindow(window, bits, src, dst, mode, modeParams);
		}
	}	/* end USE_FRAMEBUFFER_VECTORS block */


#elif TARGET_OS_WIN32
	{	/* begin TARGET_OS_WIN32 block */
		HDC hdc;
		FskBitmap tmp = NULL;

		FskInstrumentedItemSendMessageVerbose(window, kFskWindowInstrCopyBitsToScreen, src);

		hdc = GetDC(window->hwnd);

		rotate = (360 + rotate - bits->rotation) % 360;

		if ((90 == bits->rotation) || (270 == bits->rotation)) {
			SInt32 t = dst->width;
			dst->width = dst->height;
			dst->height = t;
		}

		if (90 == bits->rotation) {
			SInt32 t = dst->x;
			dst->x = window->bits->bounds.height - dst->y - dst->width;
			dst->y = t;
		}
		else if (270 == bits->rotation) {
			SInt32 t = dst->y;
			dst->y = window->bits->bounds.width - dst->x - dst->height;
			dst->x = t;
		}

		if ((NULL != bits->dibBits) && ((0 == rotate) || (180 == rotate)) && (1 == scale)) {
			BitBlt(hdc, dst->x, dst->y, dst->width, dst->height,
					(HDC)bits->hdc, src->x, src->y,
					SRCCOPY);
		}
		else {
			UInt32 pixelFormat = window->bits ? window->bits->pixelFormat : kFskBitmapFormatDefaultRGB;

			if ((0 == rotate) || (180 == rotate)) {
				if (kFskErrNone == FskBitmapNew(-dst->width, dst->height, pixelFormat, &tmp)){
					err = FskBitmapDraw(bits, src, tmp, &tmp->bounds, NULL, NULL, mode, modeParams);

					BitBlt(hdc, dst->x, dst->y, dst->width, dst->height,
							(HDC)tmp->hdc, 0, 0,
							SRCCOPY);

					FskBitmapDispose(tmp);
				}
			}
			else if (270 == rotate) {
				if (kFskErrNone == (err = FskBitmapNew(-dst->height, dst->width, pixelFormat, &tmp))) {
					FskScaleOffset so;

					so.scaleX = +scale << kFskScaleBits;
					so.scaleY = -scale << kFskScaleBits;
					so.offsetX = 0;
					so.offsetY = (dst->width - 1) << kFskOffsetBits;
					err = FskRotate90(bits, src, tmp, NULL, &so, 0);

					{
					DWORD x = dst->y;
					DWORD y = (window->bits->bounds.width * scale) - (dst->x + dst->width);
					BitBlt(hdc, x, y, dst->height, dst->width,
							(HDC)tmp->hdc, 0, 0,
							SRCCOPY);
					}

					FskBitmapDispose(tmp);
				}
			}
			else if (90 == rotate) {
				if (kFskErrNone == (err = FskBitmapNew(-dst->height, dst->width, pixelFormat, &tmp))) {
					FskScaleOffset so;

					so.scaleX = -scale << kFskScaleBits;
					so.scaleY = +scale << kFskScaleBits;
					so.offsetX = (dst->height - 1) << kFskOffsetBits;
					so.offsetY = 0;
					err = FskRotate90(bits, src, tmp, NULL, &so, 0);

					{
					DWORD x = (window->bits->bounds.height * scale) - (dst->y + dst->height);
					DWORD y = dst->x;
					BitBlt(hdc, x, y, dst->height, dst->width,
							(HDC)tmp->hdc, 0, 0,
							SRCCOPY);
					}

					FskBitmapDispose(tmp);
				}
			}
		}

		ReleaseDC(window->hwnd, hdc);

		if (err)
			return err;
	}	/* end TARGET_OS_WIN32 block */


#elif TARGET_OS_MAC
    {
    FskBitmap tmp;
    FskRectangleRecord dr;

    FskInstrumentedItemSendMessageVerbose(window, kFskWindowInstrCopyBitsToScreen, src);

    if (0 == rotate)
        FskCocoaWindowCopyBits(window, bits, src, dst);
    else if (90 == rotate) {
        if (kFskErrNone == (err = FskBitmapNew(dst->height, dst->width, kFskBitmapFormatDefaultRGB, &tmp))) {
            FskScaleOffset so;

            so.scaleX = -scale << kFskScaleBits;
            so.scaleY = +scale << kFskScaleBits;
            so.offsetX = (dst->height - 1) << kFskOffsetBits;
            so.offsetY = 0;
            err = FskRotate90(bits, src, tmp, NULL, &so, 0);

            FskRectangleSet(&dr, (window->bits->bounds.height * scale) - (dst->y + dst->height), dst->x, dst->height, dst->width);
            FskCocoaWindowCopyBits(window, tmp, &tmp->bounds, &dr);
            FskBitmapDispose(tmp);
        }
    }
    else if (270 == rotate) {
        if (kFskErrNone == FskBitmapNew(-dst->height, dst->width, kFskBitmapFormatDefaultRGB, &tmp)) {
            FskScaleOffset so;

            so.scaleX = +scale << kFskScaleBits;
            so.scaleY = -scale << kFskScaleBits;
            so.offsetX = 0;
            so.offsetY = (dst->width - 1) << kFskOffsetBits;
            err = FskRotate90(bits, src, tmp, NULL, &so, 0);
            FskRectangleSet(&dr, dst->y, (window->bits->bounds.width * scale) - (dst->x + dst->width), dst->height, dst->width);
            FskCocoaWindowCopyBits(window, tmp, &tmp->bounds, &dr);
            FskBitmapDispose(tmp);
        }
    }
    }
#endif /* TARGET_OS */

	return err;
}
#endif /* TARGET_OS */


FskErr FskWindowUpdateRectangle(FskWindow window, const FskRectangle updateArea, Boolean backBufferUnchanged)
{
	FskInstrumentedItemPrintfDebug(window, "FskWindowUpdateRectangle %p - update area %d, %d, %d, %d - backBufferUnchanged %d", window, (int)updateArea->x, (int)updateArea->y, (int)updateArea->width, (int)updateArea->height, backBufferUnchanged);
	FskInstrumentedItemPrintfDebug(window, " - window->bits is %p", window->bits);

	(void)FskFrameBufferLockSurfaceArea(window->bits, updateArea, NULL, NULL);

    if (!window->usingGL)
        FskWindowCopyBitsToWindow(window, window->bits, updateArea, updateArea, kFskGraphicsModeCopy, NULL);
    else
        FskInstrumentedItemPrintfDebug(window, "GL is NOT calling FskWindowCopyBitsToWindow() -- expect apparent nonresponsiveness");

	(void)FskFrameBufferUnlockSurface(window->bits);

	if ((false == backBufferUnchanged) && window->doAfterUpdate) {
		FskEvent e;

		if (kFskErrNone == FskEventNew(&e, kFskEventWindowAfterUpdate, NULL, 0)) {
			FskEventParameterAdd(e, kFskEventParameterUpdateRectangle, sizeof(FskRectangleRecord), updateArea);
			FskWindowEventQueue(window, e);
		}
	}

	if (window->updateSuspended)
		scheduleWindowUpdateCallback(window);

	return kFskErrNone;
}



FskErr FskWindowBeginDrawing(FskWindow window, const FskRectangle r)
{
	FskInstrumentedItemPrintfDebug(window, "FskWindowBeginDrawing %p [%d %d %d %d]", window->bits, (int)r->x, (int)r->y, (int)r->width, (int)r->height );

#if TARGET_OS_ANDROID
	if (!window->usingGL)
		(void)FskFrameBufferLockSurfaceArea(window->bits, r, NULL, NULL);
#endif

	return kFskErrNone;
}

FskErr FskWindowEndDrawing(FskWindow win, const FskRectangle bounds)
{
	FskErr err = kFskErrNone;

	FskInstrumentedItemPrintfDebug(win, "FskWindowEndDrawing %p [%d %d %d %d]", win->bits, (int)bounds->x, (int)bounds->y, (int)bounds->width, (int)bounds->height );

	if (win->rotationBits) {
		FskBitmap rotationBits = win->rotationBits;
		FskFixed saveScale = win->port->scale;
		float transform[3][3];
		extern FskErr FskPortBitmapProject(FskPort port, FskBitmap srcBits, FskRectangle srcRect, float transform[3][3]);

		win->rotationBits = NULL;

		FskPortSetBitmap(win->port, rotationBits);
		FskPortBeginDrawing(win->port, NULL);

		switch(win->rotation) {
			case 90:
				transform[0][0] =  0.f; transform[0][1] = 1.f;
				transform[1][0] = -1.f;	transform[1][1] = 0.f;
				transform[2][0] = (float)(win->bits->bounds.height - 1);	transform[2][1] = 0.f;
				break;
			case 180:
				transform[0][0] = -1.f;	transform[0][1] = 0.f;
				transform[1][0] = 0.f; 	transform[1][1] = -1.f;
				transform[2][0] = (float)(win->bits->bounds.width - 1);	transform[2][1] = (float)(win->bits->bounds.height - 1);
				break;
			case 270:
				transform[0][0] = 0.f;	transform[0][1] = -1.f;
				transform[1][0] = 1.f;	transform[1][1] = 0.f;
				transform[2][0] = 0.f;	transform[2][1] = (float)(win->bits->bounds.width - 1);
				break;
		}

		transform[0][2] = 0.f;
		transform[1][2] = 0.f;
		transform[2][2] = 1.f;

		FskPortSetClipRectangle(win->port, NULL);		//@@ save and restore??
		FskPortScaleSet(win->port, FskIntToFixed(1));
		FskPortBitmapProject(win->port, win->bits, &win->bits->bounds, transform);
		FskPortScaleSet(win->port, saveScale);

		FskPortEndDrawing(win->port);
		FskPortSetBitmap(win->port, win->bits);

		win->rotationBits = rotationBits;

		return kFskErrNone;
	}

#if FSKBITMAP_OPENGL
	if (win->usingGL) {
		FskGLPort glPort = win->port->bits->glPort;
	#if TARGET_OS_ANDROID
		LOGI("sendEventWindowUpdate FskPortEndDrawing");
	#endif /* TARGET_OS_ANDROID */
		if (glPort) {
	#if TARGET_OS_ANDROID || TARGET_OS_WIN32 || TARGET_OS_KPL
			LOGI("swapping buffers");
			FskGLPortSwapBuffers(glPort);
	#else /* !TARGET_OS_ANDROID */
			glFlush();
			//glSwapBuffers();
	#endif /* TARGET_OS_ANDROID */
		}
	}
    else
#endif /* FSKBITMAP_OPENGL */
#if TARGET_OS_KPL
		if (0 == win->rotation)
			err = FskWindowCopyBitsToWindow(win, win->bits, bounds, bounds, kFskGraphicsModeCopy, NULL);
#else
		err = FskWindowCopyBitsToWindow(win, win->bits, bounds, bounds, kFskGraphicsModeCopy, NULL);
#endif

#if TARGET_OS_ANDROID
	FskInstrumentedItemPrintfDebug(win, "FskWindowEndDrawing win->bits: %p - ", win->bits);

	if (!win->usingGL)
		(void)FskFrameBufferUnlockSurface(win->bits);
#endif
	return err;
}

FskErr FskWindowResetInvalidContentRectangle(FskWindow window)
{
#if TARGET_OS_WIN32
	ValidateRect(window->hwnd, NULL);
#elif TARGET_OS_MAC
    FskCocoaWindowSetNeedsDisplay(window, false);
#elif TARGET_OS_ANDROID
	FskRectangleSetEmpty(&window->port->invalidArea);
#endif /* TARGET_OS */
	return kFskErrNone;
}

FskErr FskWindowInvalidateContentRectangle(FskWindow window, const FskRectangle area)
{
    FskRectangleRecord scratch;

	FskInstrumentedItemSendMessageVerbose(window, kFskWindowInstrInvalidate, area);

	if (!window->updateTimer && !window->useFrameBufferUpdate)
		return kFskErrNone;

	if (!window->updateSuspended && FskListContains(&window->thread->timerCallbacks, window->updateTimer)) {
		// FskInstrumentedItemPrintfDebug(window, "FskWindowInvalidateContentRectangle - we have an updateTimer, and it's scheduled");
		return kFskErrNone;
	}

	if ((window->useFrameBufferUpdate || window->updateTimer) && window->doBeforeUpdate) //android drawing pump early return
        return kFskErrNone;

    // if OS already notified of invalid area, don't need to tell it again
    if (window->invalidArea.width && window->invalidArea.height && FskRectangleIntersect(area, &window->invalidArea, &scratch) &&
        (scratch.width == area->width) && (scratch.height == area->height))
        return kFskErrNone;
    else
        FskRectangleUnion(area, &window->invalidArea, &window->invalidArea);

	{
#if TARGET_OS_WIN32
	RECT r;
	SInt32 scale = FskWindowScaleGet(window);

	SetRect(&r, area->x * scale, area->y * scale, (area->x + area->width) * scale, (area->y + area->height) * scale);
	InvalidateRect(window->hwnd, &r, false);
#elif TARGET_OS_MAC
    FskCocoaWindowSetNeedsDisplayInRect(window, area);
#elif TARGET_OS_ANDROID
	FskRectangleUnion(&window->port->invalidArea, area, &window->port->invalidArea);
#elif (TARGET_OS_KPL && SUPPORT_LINUX_GTK)
	FskGtkWindowInvalidDaRect(window->gtkWin, area);
#endif /* TARGET_OS */
	}

	return kFskErrNone;
}

FskErr FskWindowEventSend(FskWindow window, FskEvent event)
{
	if (NULL == window || NULL == event)
		return kFskErrNone;

	if (window->thread == FskThreadGetCurrent()) {
		if (NULL != window->eventHandler) {
			FskInstrumentedItemSendMessageVerbose(window, kFskWindowInstrMsgDispatchEvent, event);
			(window->eventHandler)(event, event->eventCode,
					window, window->eventHandlerRefcon);
		}

		FskEventDispose(event);
	}
	else {
		FskWindowEventQueue(window, event);
	}

	return kFskErrNone;
}

FskEvent FskWindowEventPeek(FskWindow window, UInt32 eventCode)
{
	FskEvent q = NULL;

	if (window) {
		FskMutexAcquire(window->eventQueue->mutex);
		q = (FskEvent)window->eventQueue->list;

		while (q) {
			if (eventCode == q->eventCode)
				break;
			q = q->next;
		}
		FskMutexRelease(window->eventQueue->mutex);
	}
	return q;
}

FskErr FskWindowEventQueue(FskWindow window, FskEvent event)
{
	if (window) {
		FskThread thread = FskThreadGetCurrent();
		Boolean initiallyEmptyList = FskListMutexIsEmpty(window->eventQueue);

		FskInstrumentedItemSetOwner(event, window);

		FskInstrumentedItemSendMessageVerbose(window, kFskWindowInstrQueueEvent, event);

		FskListMutexAppend(window->eventQueue, event);
		if (initiallyEmptyList) {
			if (window->thread == thread) {
				postProcessEventMsg(window);
			}
			else {
#if TARGET_OS_WIN32
				PostMessage(window->hwnd, gEventMessage, 0, 0);	// post to native window, not to thread's native window
#elif TARGET_OS_MAC || TARGET_OS_LINUX || (TARGET_OS_KPL && SUPPORT_LINUX_GTK)
				FskThreadWake(window->thread);
#endif /* TARGET_OS */
			}
		}
	}
	return kFskErrNone;
}

FskWindow FskWindowGetActive(void)
{
#if TARGET_OS_WIN32
	HWND wnd = GetForegroundWindow();

	if (NULL != wnd) {
		FskWindow walker = NULL;
		while (true) {
			walker = (FskWindow)FskListMutexGetNext(gWindowList, walker);
			if (NULL == walker)
				break;
			if (walker->hwnd == wnd)
				return walker;
		}
	}

	return NULL;
#else /* !TARGET_OS_WIN32 */
	// note: this will sometimes be wrong in applications with multiple windows.
	//	the right thing is to find the system's concept of the active window
	//	and then post this event to that window (assuming it is also bound to Fsk)

	return FskListMutexGetNext(gWindowList, NULL);
#endif /* !TARGET_OS_WIN32 */
}


FskErr FskWindowEventSetHandler(FskWindow window, FskWindowEventHandler handler, void *refcon)
{
	window->eventHandler = handler;
	window->eventHandlerRefcon = refcon;
	return kFskErrNone;
}

FskErr FskWindowEventGetHandler(FskWindow window, FskWindowEventHandler *handler, void **refcon)
{
	*handler = window->eventHandler;
	*refcon = window->eventHandlerRefcon;
	return kFskErrNone;
}

FskErr FskWindowRequestStillDownEvents(FskWindow window, const FskRectangle r, UInt32 initialDelayMS, UInt32 intervalMS)
{
	if (r)
		window->stillDownArea = *r;
	else
		FskRectangleSetFull(&window->stillDownArea);

	window->stillDownInterval = intervalMS;

	if (!window->stillDownTimer)
		FskTimeCallbackNew(&window->stillDownTimer);

	FskTimeCallbackScheduleFuture(window->stillDownTimer, 0, initialDelayMS, stillDownEvent, window);

	return kFskErrNone;
}

void cancelStilldown(void *w, void *x, void *y, void *z)
{
	FskWindowCancelStillDownEvents((FskWindow)w);
}

FskErr FskWindowCancelStillDownEvents(FskWindow window)
{
	if (FskThreadGetCurrent() != window->thread)
		FskThreadPostCallback(window->thread, cancelStilldown, window, NULL, NULL, NULL);
	else {
		FskTimeCallbackDispose(window->stillDownTimer);
		window->stillDownTimer = NULL;
	}

	return kFskErrNone;
}

FskErr FskWindowRequestDragDrop(FskWindow window)
{
	FskErr err;

	err = FskDragDropTargetRegisterWindow(window);
	if (kFskErrNone == err) {
		window->isDropTarget = true;
		FskDragDropTargetUseNativeProxy(window, window->dragUseNativeProxy);
	}

	return err;
}

FskErr FskWindowCancelDragDrop(FskWindow window)
{
	FskErr err;

	err = FskDragDropTargetUnregisterWindow(window);
	if (kFskErrNone == err)
		window->isDropTarget = false;

	return err;
}

FskErr FskWindowSetUseDragDropNativeProxy(FskWindow window, Boolean useNativeProxy)
{
	FskErr err = kFskErrNone;

	window->dragUseNativeProxy = useNativeProxy;
	if (window->isDropTarget)
		err = FskDragDropTargetUseNativeProxy(window, window->dragUseNativeProxy);

	return err;
}

FskErr FskWindowGetUseDragDropNativeProxy(FskWindow window, Boolean *useNativeProxy)
{
	*useNativeProxy = window->dragUseNativeProxy;
	return kFskErrNone;
}

void FskWindowDragDropResult(FskWindow window, Boolean success)
{
	FskDragDropTargetDropResult(window, success);
}

FskErr FskWindowGetUpdateInterval(FskWindow window, UInt32 *intervalMS)
{
	*intervalMS = window->updateInterval;

	return kFskErrNone;
}

FskErr FskWindowGetRotation(FskWindow window, SInt32 *rotation, SInt32 *aggregate)
{
	if (rotation) {
#if TARGET_OS_WIN32 || TARGET_OS_MAC
		*rotation = window->rotation;
#else
		*rotation = 0;
#endif
	}

	if (aggregate) {
#if TARGET_OS_ANDROID
		*aggregate = (gFskPhoneHWInfo->orientation * 90) % 360;
#else
		*aggregate = 0;
#endif
	}

	return kFskErrNone;
}

FskErr FskWindowSetRotation(FskWindow window, SInt32 rotation)
{
#if TARGET_OS_WIN32 || TARGET_OS_MAC || TARGET_OS_KPL
	SInt32 currentRotation = window->rotation;

	if ((0 == rotation) ||(90 == rotation) || (180 == rotation) || (270 == rotation))
		window->requestedRotation = rotation;
	else if (kFskWindowRotationDisable == rotation)
		window->rotationDisabled += 1;
	else if (kFskWindowRotationEnable == rotation)
		window->rotationDisabled -= 1;
	else
		return kFskErrInvalidParameter;

	updateWindowRotation(window);

	if (currentRotation != window->rotation) {
		sendEventWindowSizeChanged(window);
		FskPortInvalidateRectangle(window->port, NULL);		// force invalidate. when changing between 90/270 or 0/180, the bitmap size doesn't change so it doesn't get reallocated, so it doesn't get invalidated
	}

	return kFskErrNone;
#else
	return kFskErrUnimplemented;
#endif
}

FskErr FskWindowSetUpdates(FskWindow window, const Boolean *before, const Boolean *after, UInt32 *updateIntervalMS)
{
	FskMediaPropertyValueRecord property;

	if (after)
		window->doAfterUpdate = *after;

	if (before && (*before != window->doBeforeUpdate)) {
		window->doBeforeUpdate = *before;

		if (window->useFrameBufferUpdate) {
			property.value.b = *before;
			property.type = kFskMediaPropertyTypeBoolean;
			FskFrameBufferSetProperty(kFskFrameBufferPropertyContinuousDrawing, &property);
		}
		else if (*before && window->updateSuspended && window->updateTimer) {
			// draw now, starting the drawing pump
			window->updateSuspended = false;
			FskTimeGetNow(&window->nextUpdate);
			FskTimeCallbackScheduleNextRun(window->updateTimer, windowUpdateCallback, window);
		}
#if TARGET_OS_MAC && USE_DISPLAY_LINK
		FskCocoaWindowSetUpdates(window);
#endif
	}

	if (updateIntervalMS) {
		window->updateInterval = *updateIntervalMS;
		if (window->useFrameBufferUpdate) {
			property.value.integer = window->updateInterval;
			property.type = kFskMediaPropertyTypeInteger;
			FskFrameBufferSetProperty(kFskFrameBufferPropertyUpdateInterval, &property);

			FskFrameBufferGetProperty(kFskFrameBufferPropertyUpdateInterval, &property);
			window->updateInterval = property.value.integer;
			FskInstrumentedItemPrintfDebug(window, "Setting Window Update Interval setting to %u, returned %u", (unsigned int)*updateIntervalMS, (unsigned int)window->updateInterval);
		}
		else if ((SInt32)window->updateInterval < 16)
			window->updateInterval = 16;
		else if (window->updateInterval > 500)
			window->updateInterval = 500;

	}

	return kFskErrNone;
}

FskErr FskWindowSetCursorShape(FskWindow win, UInt32 shape)
{
#if TARGET_OS_WIN32
	HMODULE module = NULL;
	LPCTSTR cursor = NULL;
	HANDLE hc = NULL;

	switch (shape) {
		case kFskCursorArrow:
			cursor = IDC_ARROW;
			break;

		case kFskCursorAliasArrow:
		case kFskCursorCopyArrow:
			cursor = IDC_ARROW;
			module = GetModuleHandle("ole32.dll");
			if (NULL != module)
				cursor = (LPCTSTR)((kFskCursorAliasArrow == shape) ? 2 : 3);
			break;

		case kFskCursorWait:
			cursor = IDC_WAIT;
			break;
		case kFskCursorIBeam:
			cursor = IDC_IBEAM;
			break;
		case kFskCursorNotAllowed:
			cursor = IDC_NO;
			break;
		case kFskCursorResizeAll:
			cursor = IDC_SIZEALL;
			break;
		case kFskCursorResizeLeftRight:
			cursor = IDC_SIZEWE;
			break;
		case kFskCursorResizeTopBottom:
			cursor = IDC_SIZENS;
			break;
		case kFskCursorResizeNESW:
			cursor = IDC_SIZENESW;
			break;
		case kFskCursorResizeNWSE:
			cursor = IDC_SIZENWSE;
			break;
		case kFskCursorLink:
			cursor = IDC_HAND;
			break;
		case kFskCursorResizeColumn:
		case kFskCursorResizeRow: {
			static Boolean firstTime = true;
			if (firstTime) {
				HINSTANCE hInst = LoadLibraryEx("comctl32.dll",NULL,LOAD_LIBRARY_AS_DATAFILE );
				if (NULL != hInst) {
					gResizeColumnCursor = LoadImage(hInst, MAKEINTRESOURCE(106), IMAGE_CURSOR, 0, 0, LR_SHARED);
					gResizeRowCursor = LoadImage(hInst, MAKEINTRESOURCE(135), IMAGE_CURSOR, 0, 0, LR_SHARED);
					FreeLibrary(hInst);
				}
				firstTime = false;
			}
			if ((kFskCursorResizeColumn == shape) && (NULL != gResizeColumnCursor))
				hc = gResizeColumnCursor;
			else
			if ((kFskCursorResizeRow == shape) && (NULL != gResizeRowCursor))
				hc = gResizeRowCursor;
			else
				cursor = IDC_SIZEALL;
			}
			break;
	}

	win->cursorShape = shape;

	if (cursor || hc) {
		if (NULL == hc)
			hc = LoadCursor(module, cursor);
		if (NULL != hc) {
			SetClassLong(win->hwnd, GCL_HCURSOR, (LONG)hc);
			SetCursor((HCURSOR)hc);
			return kFskErrNone;
		}
	}
	return kFskErrUnimplemented;
#elif TARGET_OS_MAC && !TARGET_OS_IPHONE
    FskCocoaCursorSet(win, shape);
    return kFskErrNone;
#elif TARGET_OS_LINUX
	win->cursorShape = shape;
	FskCursorSetShape(shape);
	return kFskErrNone;
#else /* TARGET_OS_UNKNOWN */
	return kFskErrUnimplemented;
#endif /* TARGET_OS */
}


void stillDownEvent(struct FskTimeCallBackRecord *callback, const FskTime time, void *param)
{
	FskWindow window = (FskWindow)param;
	FskPointRecord pos;

	FskWindowGetCursor(window, &pos);
	if (FskRectangleContainsPoint(&window->stillDownArea, &pos)) {
		FskEvent event;
		FskTime t = (FskTime)time;

#if TARGET_OS_WIN32
		FskTimeRecord mouseTimeR;
		UInt32 mouseTimeMS;
	#if TARGET_OS_WIN32
		mouseTimeMS = GetTickCount();
	#endif /* TARGET_OS */
		mouseTimeR.seconds = mouseTimeMS / 1000;
		mouseTimeR.useconds = (mouseTimeMS % 1000) * 1000;
		t = &mouseTimeR;
#elif TARGET_OS_ANDROID
		FskTimeRecord mouseTimeR;
		FskTimeGetNow(&mouseTimeR);
#endif /* TARGET_OS_WIN32 */

		if (kFskErrNone == FskEventNew(&event, kFskEventMouseStillDown, t, kFskEventModifierNotSet)) {
			FskEventParameterAdd(event, kFskEventParameterMouseLocation, sizeof(pos), &pos);
			FskWindowEventQueue(window, event);
		}
	}
	if (window->stillDownTimer)
		FskTimeCallbackScheduleFuture(callback, 0, window->stillDownInterval, stillDownEvent, param);
}

#if TARGET_OS_WIN32

static void windowEnterPressHold(struct FskTimeCallBackRecord *callback, const FskTime time, void *param);

long FAR PASCAL FskWindowWndProc(HWND hwnd, UINT msg, UINT wParam, LONG lParam)
{
	FskWindow win = (FskWindow)GetWindowLongPtr(hwnd, GWL_USERDATA);
	long result;
	FskWindowHook hook;

	if (NULL == win)
		return DefWindowProc(hwnd, msg, wParam, lParam);

	windowIncrementUseCount(win);

	for (hook = win->hooks; (NULL != hook) && (false == win->isDisposing); hook = hook->next) {
		if (kFskWindowHookCallBefore & hook->flags)
			(hook->proc)(hwnd, msg, wParam, lParam, hook->refcon);
	}

	result = FskWindowWndProcNoHook(hwnd, msg, wParam, lParam);

	for (hook = win->hooks; (NULL != hook) && (false == win->isDisposing); hook = hook->next) {
		if (kFskWindowHookCallAfter & hook->flags)
			(hook->proc)(hwnd, msg, wParam, lParam, hook->refcon);
	}

	if ((1 == win->useCount) && (false == win->isDisposing))
		FskWindowDispose(win);
	else
		windowDecrementUseCount(win);

	return result;
}


FskErr doOrQueue(FskWindow window, FskEvent event)
{
	if (FskListMutexIsEmpty(window->eventQueue))
		return FskWindowEventSend(window, event);
	else
		return FskWindowEventQueue(window, event);
}

long FAR PASCAL FskWindowWndProcNoHook(HWND hwnd, UINT msg, UINT wParam, LONG lParam)
{
	long result = 0;
	FskWindow win = (FskWindow)GetWindowLongPtr(hwnd, GWL_USERDATA);
	FskEvent fskEvent;
	UInt32 eventCode;
	UInt32 modifiers;
	UInt32 clickNumber = 0;
	Boolean processAnEvent = false, checkUpdate = false;
	DWORD mouseTimeMS = 0;
	FskTimeRecord mouseTimeR;

#if SUPPORT_INSTRUMENTATION
	if (FskInstrumentedItemHasListenersForLevel(win, kFskInstrumentationLevelDebug)) {
		LONG temp[3];
		temp[0] = msg;
		temp[1] = wParam;
		temp[2] = lParam;
		FskInstrumentedItemSendMessageDebug(win, kFskWindowInstrWindowsMessage, temp);
	}
#endif /* SUPPORT_INSTRUMENTATION */

	switch (msg) {
		case WM_NCACTIVATE:
			if (0 == wParam)
				FskECMAScriptHibernate();

			if (win->isCustomWindow) {
				if ((false == win->ignoreDeactivate) || (0 != wParam)) {
					if (kFskErrNone == FskEventNew(&fskEvent, wParam ? kFskEventWindowActivated : kFskEventWindowDeactivated, NULL, kFskEventModifierNotSet))
						FskWindowEventSend(win, fskEvent);
					FskTimersSetUiActive(wParam ? true : false);
				}
				return TRUE;
			}
			return DefWindowProc(hwnd, msg, wParam, lParam);
		case WM_NCCALCSIZE:
			if (win->isCustomWindow) {
				if (wParam != FALSE){
					NCCALCSIZE_PARAMS *windowParams = (NCCALCSIZE_PARAMS*)lParam;
					windowParams->rgrc[0].left = windowParams->lppos->x;
					windowParams->rgrc[0].top = windowParams->lppos->y;
					windowParams->rgrc[0].right = windowParams->lppos->x + windowParams->lppos->cx;
					windowParams->rgrc[0].bottom = windowParams->lppos->y + windowParams->lppos->cy;
				}
				return 0;
			}
			return DefWindowProc(hwnd, msg, wParam, lParam);
		case WM_NCHITTEST:
			if (win->isCustomWindow)
				return HTCLIENT;
			return DefWindowProc(hwnd, msg, wParam, lParam);
		case WM_NCPAINT:
			if (win->isCustomWindow)
				return 0;
			return DefWindowProc(hwnd, msg, wParam, lParam);
		case WM_SYSKEYUP:
			if (win->isCustomWindow && ((wParam == VK_MENU) || (wParam == VK_F10))) {
				FskEvent fskEvent;
				if (kFskErrNone == FskEventNew(&fskEvent, kFskEventKeyDown, NULL, kFskEventModifierNotSet)) {
					char c[2] = {5, 0};
					FskEventParameterAdd(fskEvent, kFskEventParameterKeyUTF8, 2, c);
					FskWindowEventQueue(win, fskEvent);
				}
				return 0;
			}
			return DefWindowProc(hwnd, msg, wParam, lParam);
		case WM_SYSCHAR:
			if (win->isCustomWindow) {
				FskEvent fskEvent;
				if (kFskErrNone == FskEventNew(&fskEvent, kFskEventKeyDown, NULL, kFskEventModifierAlt)) {
					char c[6];
					c[0] = (char)wParam;
					if (wParam < 127) {
						c[1] = 0;
						FskEventParameterAdd(fskEvent, kFskEventParameterKeyUTF8, 2, c);
					}
					else {
						char *encodedText;
						UInt32 encodedTextLen;

						if (wParam & 0xff00) {
							c[0] = wParam >> 8;
							c[1] = wParam & 255;
							FskTextToUTF8(c, 2, &encodedText, &encodedTextLen);
						}
						else
							FskTextToUTF8(c, 1, &encodedText, &encodedTextLen);
						FskEventParameterAdd(fskEvent, kFskEventParameterKeyUTF8, encodedTextLen + 1, encodedText);
						FskMemPtrDispose(encodedText);
					}
					FskWindowEventQueue(win, fskEvent);
				}
				return 0;
			}
			return DefWindowProc(hwnd, msg, wParam, lParam);

		case WM_PAINT: {
			PAINTSTRUCT ps;
			FskRectangleRecord windowUpdateArea;
			SInt32 windowScale = FskWindowScaleGet(win);
			BeginPaint(win->hwnd, &ps);
				FskRectangleSet(&windowUpdateArea, ps.rcPaint.left / windowScale, ps.rcPaint.top / windowScale, (ps.rcPaint.right - ps.rcPaint.left) / windowScale, (ps.rcPaint.bottom - ps.rcPaint.top) / windowScale);
				sendEventWindowUpdate(win, !FskRectangleIsEqual(&windowUpdateArea, &win->port->invalidArea), false, NULL);		//@@ scaling!!
			EndPaint(win->hwnd, &ps);
			processAnEvent = true;
			}
			break;

		case WM_ERASEBKGND:
			return true;

		case WM_SIZE:
			if (wParam != SIZE_MINIMIZED) {
				if (win->isCustomWindow && !win->isFullscreenWindow) {
					RECT rc;
					HRGN rgn;
					GetWindowRect(hwnd, &rc);
					rgn = CreateRoundRectRgn(0, 0, rc.right - rc.left + 1, rc.bottom - rc.top + 1, 16, 16);
					SetWindowRgn(hwnd, rgn, TRUE);
				}
				if (!win->ignoreResize) {
					sendEventWindowSizeChanged(win);
					processAnEvent = true;
				}
			}
			break;

		case WM_CLOSE:
			sendEventWindowClose(win);
			return TRUE;		// sendEventWindowClose may result in window being destroyed, so we just return

		case FSK_WM_MOUSE_WITH_TIME: {
			MSG *tm = (MSG *)lParam;
			msg = tm->message;
			lParam = tm->lParam;
			wParam = tm->wParam;
			mouseTimeMS = tm->time;
			FskMemPtrDispose(tm);
			}
			// deliberate fall through

		case WM_LBUTTONDOWN:
		case WM_LBUTTONDBLCLK:
		case WM_LBUTTONUP:
		case WM_RBUTTONDOWN:
		case WM_RBUTTONUP:
		case WM_MOUSEMOVE:
		case FSK_WM_MOUSEMOVE:
		case WM_RBUTTONDBLCLK:
		case WM_MOUSELEAVE:
			switch (msg) {
				case WM_LBUTTONDOWN:
					eventCode = kFskEventMouseDown;
					SetCapture(win->hwnd);
					clickNumber = 1;
					break;
				case WM_LBUTTONDBLCLK:
					eventCode = kFskEventMouseDoubleClick;
					SetCapture(win->hwnd);
					clickNumber = 2;
					break;
				case WM_LBUTTONUP:
					eventCode = kFskEventMouseUp;
					ReleaseCapture();
					FskWindowCancelStillDownEvents(win);
					break;
				case WM_RBUTTONDOWN:
					eventCode = kFskEventRightMouseDown;
					SetCapture(win->hwnd);
					clickNumber = 1;
					break;
				case WM_RBUTTONDBLCLK:
					eventCode = kFskEventRightMouseDown;
					SetCapture(win->hwnd);
					clickNumber = 2;
					break;
				case WM_RBUTTONUP:
					eventCode = kFskEventRightMouseUp;
					ReleaseCapture();
					FskWindowCancelStillDownEvents(win);
					break;
				case FSK_WM_MOUSEMOVE: {
					UInt32 count = *(UInt32 *)lParam;
					mouseTimeMS = ((FskPointAndTicks)(lParam + sizeof(UInt32)))[count - 1].ticks;
					}
					// deliberate fall through
				case WM_MOUSEMOVE:
					eventCode = kFskEventMouseMoved;
					if (!win->trackMouseEventInstalled) {
						TRACKMOUSEEVENT eventTrack;

						eventTrack.cbSize = sizeof(eventTrack);
						eventTrack.dwFlags = TME_LEAVE;
						eventTrack.dwHoverTime = HOVER_DEFAULT;
						eventTrack.hwndTrack = win->hwnd;
						TrackMouseEvent(&eventTrack);

						win->trackMouseEventInstalled = true;
					}
					break;
				case WM_MOUSELEAVE:
					eventCode = kFskEventMouseLeave;
					win->trackMouseEventInstalled = false;
					break;
			}

			modifiers = 0;
			if (MK_CONTROL & wParam)
				modifiers |= kFskEventModifierControl;
			if (MK_SHIFT & wParam)
				modifiers |= kFskEventModifierShift;
			if (0x8000 & GetKeyState(VK_MENU))
				modifiers |= kFskEventModifierAlt;
			if (0x8000 & GetKeyState(VK_CAPITAL))
				modifiers |= kFskEventModifierCapsLock;
			if (MK_LBUTTON & wParam)
				modifiers |= kFskEventModifierMouseButton;

			if (0 == mouseTimeMS) mouseTimeMS = GetTickCount();
			mouseTimeR.seconds = mouseTimeMS / 1000;
			mouseTimeR.useconds = (mouseTimeMS % 1000) * 1000;
			if (kFskErrNone == FskEventNew(&fskEvent, eventCode, &mouseTimeR, modifiers)) {
				SInt32 rotate = FskWindowRotateGet(win);

				if (180 == rotate)
					rotate = 0;		// at 180, we render as 0

				if (FSK_WM_MOUSEMOVE == msg) {
					UInt32 count = *(UInt32 *)lParam;
					FskPointAndTicks pts = (FskPointAndTicks)(lParam + sizeof(UInt32));

					rotateAndScalePoints(win, pts, count);

					FskEventParameterAdd(fskEvent, kFskEventParameterMouseLocation,
								count * sizeof(FskPointAndTicksRecord), pts);
					FskMemPtrDispose((void *)lParam);
				}
				else
				if (kFskEventMouseLeave != eventCode) {
					FskPointAndTicksRecord pos;

					pos.ticks = mouseTimeMS;
					pos.index = 0;
					pos.pt.x = GET_X_LPARAM(lParam);
					pos.pt.y = GET_Y_LPARAM(lParam);
					pos.pt.x = FskPortUInt32Unscale(win->port, pos.pt.x);
					pos.pt.y = FskPortUInt32Unscale(win->port, pos.pt.y);
					if (0 != rotate) {
						double portScale = FskPortDoubleScale(win->port, 1.0);		//@@

						if (90 == rotate) {
							SInt32 temp = pos.pt.x;
							pos.pt.x = pos.pt.y;
							pos.pt.y = (SInt32)(((win->bits->bounds.height / portScale) - 1) - temp);
						}
						else if (270 == rotate) {
							SInt32 temp = pos.pt.x;
							pos.pt.x = (SInt32)(((win->bits->bounds.width / portScale) - 1) - pos.pt.y);
							pos.pt.y = temp;
						}
						else if (180 == rotate) {
							pos.pt.x = (SInt32)(((win->bits->bounds.width  / portScale) - 1) - pos.pt.x);
							pos.pt.y = (SInt32)(((win->bits->bounds.height / portScale) - 1) - pos.pt.y);
						}
					}

					pos.pt.x -= win->port->origin.x;
					pos.pt.y -= win->port->origin.y;

					FskEventParameterAdd(fskEvent, kFskEventParameterMouseLocation, sizeof(pos), &pos);
				}

				if (0 != clickNumber)
					FskEventParameterAdd(fskEvent, kFskEventParameterMouseClickNumber, sizeof(clickNumber), &clickNumber);

				doOrQueue(win, fskEvent);
				checkUpdate = true;
			}
			break;

		case WM_MOUSEWHEEL: {
			INT delta = GET_WHEEL_DELTA_WPARAM(wParam);
			float deltaF = (float)delta / (float)WHEEL_DELTA;

			if (kFskErrNone == FskEventNew(&fskEvent, kFskEventMouseWheel, NULL, kFskEventModifierNotSet)) {
				FskEventParameterAdd(fskEvent, kFskEventParameterMouseWheelDelta, sizeof(deltaF), &deltaF);
				doOrQueue(win, fskEvent);
				checkUpdate = true;
			}
			}
			break;

		case WM_KEYDOWN:
		case WM_KEYUP:
			{
			UInt32 evtCode;
			UInt32 functionKey = 0;
			SInt32 rotate = FskWindowRotateGet(win);

			if (msg == WM_KEYUP)
				evtCode = kFskEventKeyUp;
			else
				evtCode = kFskEventKeyDown;

			if (!imeActive(win) && (13 == wParam) /* || (32 == wParam) */) {
				if (msg == WM_KEYUP) {
					FskTimeCallbackDispose(win->pressHoldTimer);
					win->pressHoldTimer = NULL;

					if ((1 == win->pressHoldState) || (2 == win->pressHoldState)) {		// some phones send DOWN->CHAR->UP, others send CHAR->DOWN->UP
						char key[2];

						key[0] = (char)wParam;
						key[1] = 0;
						if (kFskErrNone == FskEventNew(&fskEvent, kFskEventKeyDown, NULL, 0)) {
							FskEventParameterAdd(fskEvent, kFskEventParameterKeyUTF8, 2, key);
							doOrQueue(win, fskEvent);
						}
						if (kFskErrNone == FskEventNew(&fskEvent, kFskEventKeyUp, NULL, 0)) {
							FskEventParameterAdd(fskEvent, kFskEventParameterKeyUTF8, 2, key);
							doOrQueue(win, fskEvent);
						}
					}

					win->pressHoldState = 0;
				}
				else {
					if (!win->pressHoldTimer) {
						FskTimeRecord t;

						FskTimeCallbackNew(&win->pressHoldTimer);

						FskTimeGetNow(&t);
						FskTimeAddMS(&t, 500);
						FskTimeCallbackSet(win->pressHoldTimer, &t, windowEnterPressHold, win);

						win->pressHoldState = 1;
					}
				}
				return 0;
			}

			if (180 == rotate)
				rotate = 0;		// at 180, we render as 0

			if (0 != rotate) {
				switch (wParam) {
					case VK_UP:
						if (90 == rotate)
							wParam = VK_LEFT;
						else if (270 == rotate)
							wParam = VK_RIGHT;
						else if (180 == rotate)
							wParam = VK_DOWN;
						break;

					case VK_DOWN:
						if (90 == rotate)
							wParam = VK_RIGHT;
						else if (270 == rotate)
							wParam = VK_LEFT;
						else if (180 == rotate)
							wParam = VK_UP;
						break;

					case VK_LEFT:
						if (90 == rotate)
							wParam = VK_DOWN;
						else if (270 == rotate)
							wParam = VK_UP;
						else if (180 == rotate)
							wParam = VK_RIGHT;
						break;

					case VK_RIGHT:
						if (90 == rotate)
							wParam = VK_UP;
						else if (270 == rotate)
							wParam = VK_DOWN;
						else if (180 == rotate)
							wParam = VK_LEFT;
						break;
				}
			}

			if ((VK_F1 <= wParam) && (wParam <= VK_F16)) {
				UInt32 modifiers = 0;

				if (0x8000 & GetKeyState(VK_CONTROL))
					modifiers |= kFskEventModifierControl;
				if (0x8000 & GetKeyState(VK_SHIFT))
					modifiers |= kFskEventModifierShift;
				if (0x8000 & GetKeyState(VK_MENU))
					modifiers |= kFskEventModifierAlt;
				if (0x8000 & GetKeyState(VK_CAPITAL))
					modifiers |= kFskEventModifierCapsLock;
				if (MK_LBUTTON & wParam)
					modifiers |= kFskEventModifierMouseButton;
				if (kFskErrNone == FskEventNew(&fskEvent, evtCode, NULL, modifiers)) {
					char zero = 0;

					functionKey = wParam - VK_F1 + 1;
					FskEventParameterAdd(fskEvent, kFskEventParameterKeyUTF8, 1, &zero);
					FskEventParameterAdd(fskEvent, kFskEventParameterFunctionKey, sizeof(functionKey), &functionKey);

					doOrQueue(win, fskEvent);
					result = false;		// An application should return zero if it processes this message.
				}
			}
			else if (lParam & 1 << 24) {
				// extended key
				switch (wParam) {
					case VK_UP:
						wParam = 30;
						break;

					case VK_DOWN:
						wParam = 31;
						break;

					case VK_LEFT:
						wParam = 28;
						break;

					case VK_RIGHT:
						wParam = 29;
						break;

					case VK_DELETE:
						wParam = 127;
						break;

					case VK_PRIOR:
						wParam = 11;
						break;

					case VK_NEXT:
						wParam = 12;
						break;

					case VK_END:
						wParam = 4;
						break;

					case VK_HOME:
						wParam = 1;
						break;

					case 0xb3:		//VK_MEDIA_PLAY_PAUSE
						wParam = 0;
						functionKey = kFskEventFunctionKeyTogglePlayPause;
						break;

					case 0xb1:		//VK_MEDIA_PREV_TRACK
						wParam = 0;
						functionKey = kFskEventFunctionKeyPreviousTrack;
						break;

					case 0xb0:		//VK_MEDIA_NEXT_TRACK
						wParam = 0;
						functionKey = kFskEventFunctionKeyNextTrack;
						break;

					case 0xb2:		// VK_MEDIA_STOP
						wParam = 0;
						functionKey = kFskEventFunctionKeyStop;
						break;

					case 0xac:		// VK_BROWSER_HOME
						wParam = 0;
						functionKey = kFskEventFunctionKeyHome;
						break;

					default:
						wParam = 0;
						break;
				}

				if (wParam || functionKey) {
					UInt32 modifiers = 0;
					if (0x8000 & GetKeyState(VK_CONTROL))
						modifiers |= kFskEventModifierControl;
					if (0x8000 & GetKeyState(VK_SHIFT))
						modifiers |= kFskEventModifierShift;
					if (0x8000 & GetKeyState(VK_MENU))
						modifiers |= kFskEventModifierAlt;
					if (0x8000 & GetKeyState(VK_CAPITAL))
						modifiers |= kFskEventModifierCapsLock;
					if (MK_LBUTTON & wParam)
						modifiers |= kFskEventModifierMouseButton;
					if (kFskErrNone == FskEventNew(&fskEvent, evtCode, NULL, modifiers)) {
						char c[2];
						c[0] = (char)wParam;
						c[1] = 0;
						FskEventParameterAdd(fskEvent, kFskEventParameterKeyUTF8, 2, c);
						if (functionKey)
							FskEventParameterAdd(fskEvent, kFskEventParameterFunctionKey, sizeof(functionKey), &functionKey);
						doOrQueue(win, fskEvent);
						result = false;
					}
				}
			}
			}
			break;

		case WM_IME_CHAR:
		case WM_CHAR: {
			UInt32 modifiers = 0;

			if (!imeActive(win) && (13 == wParam) /* || (32 == wParam) */) {
				if (1 == win->pressHoldState)
					win->pressHoldState = 2;		// we've ignored a char event, so if the hold isn't long enough, we need to generate the regular key event

				return 0;			// we handle converting space & CR from key to char
			}

			if (0x8000 & GetKeyState(VK_CONTROL))
				modifiers |= kFskEventModifierControl;
			if (0x8000 & GetKeyState(VK_SHIFT))
				modifiers |= kFskEventModifierShift;
			if (0x8000 & GetKeyState(VK_MENU))
				modifiers |= kFskEventModifierAlt;
			if (0x8000 & GetKeyState(VK_CAPITAL))
				modifiers |= kFskEventModifierCapsLock;
			if (MK_LBUTTON & wParam)
				modifiers |= kFskEventModifierMouseButton;

		if (wParam < 32) {
			// special cases for cut/copy/paste
			if (3 == wParam) {
				wParam = 'C';
				modifiers |= kFskEventModifierControl;
			}
			else
			if (22 == wParam) {
				wParam = 'V';
				modifiers |= kFskEventModifierControl;
			}
			else
			if (24 == wParam) {
				wParam = 'X';
				modifiers |= kFskEventModifierControl;
			}
		}

			if (kFskErrNone == FskEventNew(&fskEvent, kFskEventKeyDown, NULL, modifiers)) {
				char c[6];
				c[0] = (char)wParam;
				if (wParam < 127) {
					c[1] = 0;
					FskEventParameterAdd(fskEvent, kFskEventParameterKeyUTF8, 2, c);
				}
				else {
					char *encodedText;
					UInt32 encodedTextLen;

					if (wParam & 0xff00) {
						c[0] = wParam >> 8;
						c[1] = wParam & 255;
						FskTextToUTF8(c, 2, &encodedText, &encodedTextLen);
					}
					else
						FskTextToUTF8(c, 1, &encodedText, &encodedTextLen);
					FskEventParameterAdd(fskEvent, kFskEventParameterKeyUTF8, encodedTextLen + 1, encodedText);
					FskMemPtrDispose(encodedText);
				}

				doOrQueue(win, fskEvent);
				checkUpdate = true;
			}
			}
			break;

		case WM_COMMAND:
			// send unique ID of selected item to window
			if (kFskErrNone == FskEventNew(&fskEvent, kFskEventMenuCommand, NULL, kFskEventModifierNotSet)) {
				SInt32 command = LOWORD(wParam);
				FskEventParameterAdd(fskEvent, kFskEventParameterCommand, sizeof(command), &command);
				FskWindowEventQueue(win, fskEvent);
			}
			break;

		case WM_INITMENUPOPUP:
			{
				HMENU menu = (HMENU)wParam;
				int i;
				int c = GetMenuItemCount(menu);
				for (i = 0; i < c; i++) {
					UINT id = GetMenuItemID(menu, i);
					if (kFskErrNone == FskEventNew(&fskEvent, kFskEventMenuStatus, NULL, kFskEventModifierNotSet)) {
						FskEventParameterAdd(fskEvent, kFskEventParameterCommand, sizeof(id), &id);
						FskWindowEventSend(win, fskEvent);
					}
				}
			}
			break;

		case WM_GETMINMAXINFO: {
			MINMAXINFO *info = (MINMAXINFO *)lParam;
			RECT rMin, rMax;

			rMin.left = 0;
			rMin.top = 0;
			rMin.right = win->minWidth;
			rMin.bottom = win->minHeight;
			if (!win->isCustomWindow)
				AdjustWindowRectEx(&rMin, GetWindowLong(hwnd, GWL_STYLE), TRUE, GetWindowLong(hwnd, GWL_EXSTYLE));

			rMax.left = 0;
			rMax.top = 0;
			rMax.right = win->maxWidth;
			rMax.bottom = win->maxHeight;
			if (!win->isCustomWindow)
				AdjustWindowRectEx(&rMax, GetWindowLong(hwnd, GWL_STYLE), TRUE, GetWindowLong(hwnd, GWL_EXSTYLE));

			if (win->minWidth) info->ptMinTrackSize.x = rMin.right - rMin.left;
			if (win->minHeight) info->ptMinTrackSize.y = rMin.bottom - rMin.top;
			if (win->maxWidth) info->ptMaxTrackSize.x = rMax.right - rMax.left;
			if (win->maxHeight) info->ptMaxTrackSize.y = rMax.bottom - rMax.top;
			}
			break;
		default:
			if (msg == gFskApplicationMessage) {
				if (kFskErrNone == FskEventNew(&fskEvent, kFskEventApplication, NULL, kFskEventModifierNotSet)) {
					FskEventParameterAdd(fskEvent, kFskEventParameterApplicationParam1, sizeof(wParam), &wParam);
					FskEventParameterAdd(fskEvent, kFskEventParameterApplicationParam2, sizeof(lParam), &lParam);
					FskWindowEventQueue(win, fskEvent);
				}
			}
			else if (msg == gEventMessage)
				processAnEvent = true;	// used to give us more time to process our events
			else
				result = DefWindowProc(hwnd, msg, wParam, lParam);
			break;
	}

	if (processAnEvent)
		FskWindowCheckEventQueue(win);

//	if (checkUpdate)
//		UpdateWindow(hwnd);

	return result;
}

void windowEnterPressHold(struct FskTimeCallBackRecord *callback, const FskTime time, void *param)
{
	FskWindow win = param;
	FskEvent event;
	char zero = 0;
	UInt32 functionKey = 65540;

	windowIncrementUseCount(win);

	if (kFskErrNone == FskEventNew(&event, kFskEventKeyDown, NULL, 0)) {
		FskEventParameterAdd(event, kFskEventParameterKeyUTF8, 1, &zero);
		FskEventParameterAdd(event, kFskEventParameterFunctionKey, sizeof(functionKey), &functionKey);
		doOrQueue(win, event);
	}
	if (kFskErrNone == FskEventNew(&event, kFskEventKeyUp, NULL, 0)) {
		FskEventParameterAdd(event, kFskEventParameterKeyUTF8, 1, &zero);
		FskEventParameterAdd(event, kFskEventParameterFunctionKey, sizeof(functionKey), &functionKey);
		doOrQueue(win, event);
	}

	win->pressHoldState = 3;

	windowDecrementUseCount(win);
}

#endif

#if TARGET_OS_ANDROID
	static Boolean needMoreRedraw = 0;
#endif

void sendEventWindowUpdate(FskWindow win, Boolean redrawAll, Boolean skipBeforeUpdate, const FskTime updateTimeIn)
{
	FskEvent fskEvent;
    FskTimeRecord updateTime;

	#if TARGET_OS_ANDROID
        FskBitmap gFB = NULL;
		Boolean didLock = false;

		if (gAndroidCallbacks->getMidWindowResizeCB()) {
			// Don't do useless drawing if we're mid-size-change
			// This useless drawing also cycles the android framebuffers and
			// can cause misdrawing
			FskInstrumentedItemPrintfDebug(win, "***** Mid size change - bail");
			FskRectangleSetEmpty(&win->port->invalidArea);
			return;
		}

		if (win->usingGL && win->bits->glPort == NULL) {
//			FskInstrumentedItemPrintfDebug(win, "[%s] ***** about to sendEventWindowUpdate, but we're using GL and there is no glPort - bail.");
			return;
		}

	#endif /* TARGET_OS_ANDROID */


    if (updateTimeIn)
        updateTime = *updateTimeIn;
    else
        FskTimeGetNow(&updateTime);

    FskRectangleSetEmpty(&win->invalidArea);

	if (win->doBeforeUpdate && !skipBeforeUpdate) {
		FskInstrumentedItemPrintfDebug(win, "  --  send a BeforeUpdate");
		if (kFskErrNone == FskEventNew(&fskEvent, kFskEventWindowBeforeUpdate, &updateTime, kFskEventModifierNotSet)) {
			FskWindowEventSend(win, fskEvent);
            if (FskRectangleIsEmpty(&win->port->invalidArea))
                return;
        }
	}

	if (FskRectangleIsEmpty(&win->port->invalidArea))
	{
		FskInstrumentedItemPrintfDebug(win, " update from back-buffer - win %p, win->bits %p, win->port->bits %p", win, win->bits, win->port->bits);
		FskWindowUpdateRectangle(win, &win->bits->bounds, true);
	}
	else {
		FskInstrumentedItemPrintfDebug(win, " render");
		// let the client draw
		if (kFskErrNone == FskEventNew(&fskEvent, kFskEventWindowUpdate, &updateTime, kFskEventModifierNotSet)) {
			FskRectangleRecord invalidArea = win->retainsPixelsBetweenUpdates ? win->port->invalidArea : win->bits->bounds;

			// the YUV to RGB color converter needs to be given even x, width & height values. there is probably a better place to put this knowledge, but....
			// also 1.5x scaling needs this to avoid leaving behind garbage bits
			if (invalidArea.x & 1) {
				invalidArea.x -= 1;
				invalidArea.width += 1;
			}
			invalidArea.width += (invalidArea.width & 1);

			if (invalidArea.y & 1) {
				invalidArea.y -= 1;
				invalidArea.height += 1;
			}
			invalidArea.height += (invalidArea.height & 1);

			FskPortSetUpdateRectangle(win->port, &invalidArea);
			FskEventParameterAdd(fskEvent, kFskEventParameterUpdateRectangle, sizeof(invalidArea), &invalidArea);

#if TARGET_OS_ANDROID
            if (!win->usingGL) {
                didLock = true;
                FskPortRectScale(win->port, &invalidArea);
                (void)FskFrameBufferLockSurfaceArea(gFB, &invalidArea, NULL, NULL);
            }
#endif /* TARGET_OS_ANDROID */

			FskRectangleSetEmpty(&win->port->invalidArea);			// do this before beginning to draw so that if client invalidates during update, we won't blow them away

			FskPortBeginDrawing(win->port, NULL);
				FskWindowEventSend(win, fskEvent);
				if (redrawAll)
					FskRectangleUnion(&win->bits->bounds, &win->port->changedArea, &win->port->changedArea);		//@@ - SCALING!!
			FskPortEndDrawing(win->port);

			FskPortSetUpdateRectangle(win->port, NULL);
		} /* FskEventNew was successful */
	}

#if TARGET_OS_ANDROID
	if (didLock) {
		(void)FskFrameBufferUnlockSurface(gFB);
	}
#endif /* TARGET_OS_ANDROID */
	FskInstrumentedItemPrintfDebug(win, "***** sendEventWindowUpdate end *****");
} /* sendEventWindowUpdate */

void sendEventWindowSizeChanged(FskWindow win)
{
	FskEvent fskEvent;

	FskInstrumentedItemPrintfDebug(win, "sendEventWindowSizeChanged");
	if (false == checkWindowBitmapSize(win))
		return;

	if (win->eventHandler && (kFskErrNone == FskEventNew(&fskEvent, kFskEventWindowResize, NULL, kFskEventModifierNotSet))) {
#if TARGET_OS_MAC
		if (FskInstrumentedItemHasListeners(win))
			FskInstrumentedItemSendMessage(win, kFskWindowInstrMsgDispatchEvent, fskEvent);
		(win->eventHandler)(fskEvent, fskEvent->eventCode, win, win->eventHandlerRefcon);
        FskEventDispose(fskEvent);
#else /* TARGET_OTHERS */
		FskWindowEventQueue(win, fskEvent);
#endif /* TARGET_OTHERS */
	}

#if TARGET_OS_ANDROID
	if (kFskErrNone == FskEventNew(&fskEvent, kFskEventWindowAfterResize, NULL, kFskEventModifierNotSet)) {
		FskWindowEventQueue(win, fskEvent);
	}
#endif /* TARGET_OS_ANDROID */
}

#if TARGET_OS_MAC || TARGET_OS_WIN32 || (TARGET_OS_KPL && SUPPORT_LINUX_GTK)
void sendEventWindowClose(FskWindow win)
{
	// tell the event handler we're done, since it doesn't already know
	FskEvent fskEvent;

	if (kFskErrNone == FskEventNew(&fskEvent, kFskEventWindowClose, NULL, kFskEventModifierNotSet))
		FskWindowEventSend(win, fskEvent);
}
#endif

#if TARGET_OS_ANDROID
Boolean checkWindowBitmapSize(FskWindow win)
{
	FskBitmap bmp = win->bits;

	if (NULL == win->port)
		return false;

	(void)FskFrameBufferGetScreenBitmap(&bmp);

	if (NULL != bmp) {
		FskEvent fskEvent;

		if (kFskErrNone == FskEventNew(&fskEvent, kFskEventWindowBeforeResize, NULL, kFskEventModifierNotSet)) {
 			if (FskInstrumentedItemHasListeners(win))
				FskInstrumentedItemSendMessage(win, kFskWindowInstrMsgDispatchEvent, fskEvent);
			if (win->eventHandler)
				(win->eventHandler)(fskEvent, fskEvent->eventCode, win, win->eventHandlerRefcon);
//FskInstrumentedItemPrintfDebug(win, "checkWindowBitmapSize: returned from dispatching event");
			FskEventDispose(fskEvent);
		}

	}

	setWindowBitmap(win, bmp);
	setWindowAsSysContext(win);

	FskRectangleSetFull(&win->port->invalidArea);
	FskPortRectUnscale(win->port, &win->port->invalidArea);

	return true;
}

#elif TARGET_OS_KPL

#if !SUPPORT_LINUX_GTK
Boolean checkWindowBitmapSize(FskWindow win)
{
	FskBitmap bmp = win->bits;
	UInt32 newWidth, newHeight;
	SInt32 rotate = FskWindowRotateGet(win);

	if (NULL == win->port)
		return false;

	FskWindowGetUnscaledSize(win, &newWidth, &newHeight);
	FskInstrumentedItemPrintfDebug(win, "In checkWindowBitmapSize, FskWindowGetUnscaledSize returned width=%d height=%d================",newWidth,newHeight);
	
	if (NULL != bmp) {
		FskEvent fskEvent;
		FskBitmap screenBitmap;
		
		(void)FskFrameBufferGetScreenBitmap(&screenBitmap);

		if (((SInt32)newWidth != bmp->bounds.width) || ((SInt32)newHeight != bmp->bounds.height)) {
			if (kFskErrNone == FskEventNew(&fskEvent, kFskEventWindowBeforeResize, NULL, kFskEventModifierNotSet)) {
				if (FskInstrumentedItemHasListeners(win))
					FskInstrumentedItemSendMessage(win, kFskWindowInstrMsgDispatchEvent, fskEvent);
				if (win->eventHandler)
					(win->eventHandler)(fskEvent, fskEvent->eventCode, win, win->eventHandlerRefcon);
				FskEventDispose(fskEvent);
			}

			#if FSKBITMAP_OPENGL
				if (bmp->glPort == win->glPortForWindow) {
					bmp->glPort = NULL;
				}
			#endif /* FSKBITMAP_OPENGL */
			if (screenBitmap != win->bits)
				FskBitmapDispose(win->bits);
			win->port->bits = NULL;
			win->bits = NULL;
		}
	}

	if (win->rotationBits) {
#if FSKBITMAP_OPENGL
		if (win->rotationBits->glPort == win->glPortForWindow)
			win->rotationBits->glPort = NULL;
#endif
		win->rotationBits = NULL;
	}
	
	if (0 == rotate) {
		(void)FskFrameBufferGetScreenBitmap(&bmp);
	}
	else {
		(void)FskFrameBufferGetScreenBitmap(&win->rotationBits);
		FskBitmapNew(-(SInt32)newWidth, (SInt32)newHeight, win->rotationBits->pixelFormat, &bmp);
		FskInstrumentedItemPrintfDebug(win, "===============rotationBits width=%d rotationBits height=%d================",win->rotationBits->bounds.width,win->rotationBits->bounds.height);
		FskInstrumentedItemPrintfDebug(win, "===============bits width=%d bits height=%d================",bmp->bounds.width,bmp->bounds.height);
	}
	
	setWindowBitmap(win, bmp);
	if (win->bits) {
		win->port->invalidArea = win->bits->bounds;
		FskPortRectUnscale(win->port, &win->port->invalidArea);
	}
	
	setWindowAsSysContext(win);
	
	return true;
}
#else
Boolean checkWindowBitmapSize(FskWindow win)
{
	FskErr err;
	UInt32 newWidth, newHeight;
	FskBitmap bmp = win->bits;
	Boolean changed = false;
	#if FSKBITMAP_OPENGL
		FskGLPort glPort =  NULL;
	#endif /* FSKBITMAP_OPENGL */

	if (NULL == win->port)
		return false;

	FskWindowGetUnscaledSize(win, &newWidth, &newHeight);

	if (NULL != bmp) {
		if (!changed && ((SInt32)newWidth == bmp->bounds.width) && ((SInt32)newHeight == bmp->bounds.height))
			return false;

		#if FSKBITMAP_OPENGL
			glPort = bmp->glPort;						/* Save the glPort for the next bitmap */
			bmp->glPort = NULL;
		#endif /* FSKBITMAP_OPENGL */
		FskGtkWindowBitmapDispose(win->gtkWin);
		win->port->bits = NULL;
		win->bits = NULL;
		changed = true;
	}

	//err = FskBitmapNew(-(SInt32)newWidth, (SInt32)newHeight, getScreenPixelFormat(), &bmp);
	err = FskGtkWindowBitmapNew(win->gtkWin, newWidth, newHeight, &bmp);
	if (err) {
		#if FSKBITMAP_OPENGL
			FskGLPortDispose(glPort);					/* Otherwise it will leak */
		#endif /* FSKBITMAP_OPENGL */
		return false;		// bad scene
	}
	#if FSKBITMAP_OPENGL
		bmp->glPort = glPort;							/* Copy from the old */
		FskGLPortResize(glPort, newWidth, newHeight);	/* This must be done in the GL thread */
	#endif /* FSKBITMAP_OPENGL */

	FskInstrumentedItemSetOwner(bmp, win);

	LOGI("Calling setWindowBitmap with a new Bitmap");
	setWindowBitmap(win, bmp);
	setWindowAsSysContext(win);
	win->port->invalidArea = bmp->bounds;

	if(changed) {
		FskEvent fskEvent;
		if (kFskErrNone == FskEventNew(&fskEvent, kFskEventWindowResize, NULL, kFskEventModifierNotSet)) {
			(win->eventHandler)(fskEvent, fskEvent->eventCode, win, win->eventHandlerRefcon);
			FskEventDispose(fskEvent);
		}
	}
	return true;
}
#endif
#else /* TARGET_OTHERS */

Boolean checkWindowBitmapSize(FskWindow win)
{
	FskErr err;
	UInt32 newWidth, newHeight;
	FskBitmap bmp = win->bits;
	SInt32 rotate = FskWindowRotateGet(win);

	if (NULL == win->port)
		return false;

	FskWindowGetUnscaledSize(win, &newWidth, &newHeight);

	if (NULL != bmp) {
		if (((SInt32)newWidth != bmp->bounds.width) || ((SInt32)newHeight != bmp->bounds.height)) {
			FskEvent fskEvent;
			
			if (win->eventHandler && (kFskErrNone == FskEventNew(&fskEvent, kFskEventWindowBeforeResize, NULL, kFskEventModifierNotSet))) {
				if (FskInstrumentedItemHasListeners(win))
					FskInstrumentedItemSendMessage(win, kFskWindowInstrMsgDispatchEvent, fskEvent);
				(win->eventHandler)(fskEvent, fskEvent->eventCode, win, win->eventHandlerRefcon);
				FskEventDispose(fskEvent);
			}

			#if FSKBITMAP_OPENGL
				if (bmp->glPort == win->glPortForWindow)
					bmp->glPort = NULL;
			#endif /* FSKBITMAP_OPENGL */
			FskBitmapDispose(win->bits);
			win->port->bits = NULL;
			win->bits = NULL;
		}
	}

	err = FskBitmapNew(-(SInt32)newWidth, (SInt32)newHeight, getScreenPixelFormat(), &bmp);
	if (err) {
		#if SUPPORT_INSTRUMENTATION
			FskInstrumentedItemPrintfDebug(win, "ERROR: checkWindowBitmapSize failed to allocate a bitmap (%d x %d format: %d)",
										   -(int)newWidth, (int)newHeight, (unsigned int)getScreenPixelFormat());
		#endif /* SUPPORT_INSTRUMENTATION */
		return false;		// bad scene
	}

#if FSKBITMAP_OPENGL
	if (0 == rotate)
		bmp->glPort = win->glPortForWindow;
#endif
	
	FskInstrumentedItemSetOwner(bmp, win);

	LOGI("Calling setWindowBitmap with a new Bitmap");
	setWindowBitmap(win, bmp);
	win->port->invalidArea = bmp->bounds;

	if (win->rotationBits) {
#if FSKBITMAP_OPENGL
		if (win->rotationBits->glPort == win->glPortForWindow)
			win->rotationBits->glPort = NULL;
#endif
		FskBitmapDispose(win->rotationBits);
		win->rotationBits = NULL;
	}
	if ((90 == rotate) || (270 == rotate))
		FskBitmapNew(-(SInt32)newHeight, (SInt32)newWidth, getScreenPixelFormat(), &win->rotationBits);
	else if (180 == rotate)
		FskBitmapNew(-(SInt32)newWidth, (SInt32)newHeight, getScreenPixelFormat(), &win->rotationBits);

	setWindowAsSysContext(win);

#if TARGET_OS_WIN32
	InvalidateRect(win->hwnd, NULL, false);
#endif /* TARGET_OS_WIN32 */

	return true;
}
#endif /* TARGET_OTHERS */


#define PETERS_HACK
void FskWindowUpdate(FskWindow win, const FskTime time)
{
#if TARGET_OS_WIN32
	FskRectangle area = &win->port->invalidArea;
	RECT r;

	SetRect(&r, area->x, area->y, area->x + area->width, area->y + area->height);
	ValidateRect(win->hwnd, &r);
#endif /* TARGET_OS_WIN32 */
#if TARGET_OS_ANDROID && defined(PETERS_HACK)
	// service pending events, because they may include window resize events
	FskInstrumentedItemPrintfDebug(win, "FskWindowUpdate(%p) while LinuxHandleThreadEvents", win);
	while (LinuxHandleThreadEvents())
       ;
#endif

	sendEventWindowUpdate(win, false, false, time);
}

FskErr FskWindowCopyToBitmap(FskWindow window, const FskRectangle srcIn, Boolean forExport, FskBitmap *bitsOut)
{
	FskErr err;
	FskBitmap bits = window->bits, copy = NULL;
	Boolean hasAlpha;
	FskBitmapFormatEnum pixelFormat = 0;
	FskRectangle src;
    FskMediaPropertyValueRecord property;

    if ((NULL == srcIn) && (kFskErrNone == FskFrameBufferGetProperty(kFskFrameBufferPropertyDisplayCopy, &property))) {
        err = kFskErrNone;
        copy = property.value.bitmap;
        goto bail;
    }

#if TARGET_OS_ANDROID
	FskBitmap screenBitmap;
	Boolean didLock = false;
	(void)FskFrameBufferGetScreenBitmap(&screenBitmap);
	FskInstrumentedItemPrintfDebug(window, "FskWindowCopyToBitmap - window: %p, windowsBits: %p frameBuffer: %p window->usingGL %d, bits->glPort %p" , window, bits, screenBitmap , window->usingGL, bits->glPort);

	if (screenBitmap == bits && ! bits->glPort) {
//		FskInstrumentedItemPrintfDebug(window, "[%s] FskWindowCopyToBitmap - Source is not OpenGL - use FrameBufferLockForReading", threadTag(FskThreadGetCurrent()));
		(void)FskFrameBufferLockSurfaceForReading(&screenBitmap);
		didLock = true;
		bits = screenBitmap;
	}
#endif /* TARGET_OS_ANDROID */

	src = srcIn ? srcIn : &bits->bounds;
	FskBitmapGetPixelFormat(bits, &pixelFormat);
	FskBitmapGetHasAlpha(bits, &hasAlpha);

#if FSKBITMAP_OPENGL
	if (bits->glPort) {
		if (!forExport)
			pixelFormat = kFskBitmapFormatGLRGBA;
		err = FskBitmapNew(src->width, src->height, pixelFormat, &copy);
		BAIL_IF_ERR(err);
		FskBitmapSetHasAlpha(copy, hasAlpha);
		err = FskGLPortPixelsRead(bits->glPort, 0, src, copy);
		BAIL_IF_ERR(err);
	}
	else
#endif /* FSKBITMAP_OPENGL */
	{
		err = FskBitmapNew(src->width, src->height, pixelFormat, &copy);
		BAIL_IF_ERR(err);
		FskBitmapSetHasAlpha(copy, hasAlpha);
		FskInstrumentedItemPrintfDebug(window, " -- copy (screen) %d %d %d %d from %p to copy %p", (int)src->x, (int)src->y, (int)src->width, (int)src->height, bits, copy);
		err = FskBitmapDraw(bits, src, copy, &copy->bounds, NULL, NULL, kFskGraphicsModeCopy, NULL);
		BAIL_IF_ERR(err);
	}

bail:
	if (err) {
		FskInstrumentedItemPrintfDebug(window, " -- copy (screen) failed with err: %d", err);
		FskBitmapDispose(copy);
		copy = NULL;
	}

	*bitsOut = copy;

#if TARGET_OS_ANDROID
	if (didLock) {
		(void)FskFrameBufferUnlockSurfaceForReading(screenBitmap);
	}
#endif /* TARGET_OS_ANDROID */

	return err;
}

void windowIncrementUseCount(FskWindow win)
{
	win->useCount += 1;
    assert(win->useCount > 1);         //@@ try to appease coverity
}

void windowDecrementUseCount(FskWindow win)
{
	win->useCount -= 1;
	if (win->useCount <= 0)
		FskWindowDispose(win);
    else
        assert(win->useCount >= 1);     //@@ try to appease coverity
}

#if TARGET_OS_WIN32

FskErr FskWindowHookAdd(FskWindow window, FskWindowHookProc proc, UInt32 flags, void *refcon)
{
	FskErr err;
	FskWindowHook hook;

	err = FskMemPtrNewClear(sizeof(FskWindowHookRecord), (FskMemPtr *)&hook);
	if (err) return err;

	hook->proc = proc;
	hook->flags = flags;
	hook->refcon = refcon;
	FskListPrepend((FskList *)&window->hooks, hook);

	window->useCount += 1;

	return kFskErrNone;
}

FskErr FskWindowHookRemove(FskWindow window, FskWindowHookProc proc, UInt32 flags, void *refcon)
{
	FskWindowHook hook;

	for (hook = window->hooks; NULL != hook; hook = hook->next) {
		if ((hook->proc == proc) && (hook->flags == flags) && (hook->refcon == refcon)) {
			FskListRemove((FskList *)&window->hooks, hook);
			FskMemPtrDispose(hook);
			windowDecrementUseCount(window);
			return kFskErrNone;
		}
	}
	return kFskErrInvalidParameter;
}

#endif /* TARGET_OS_WIN32 */

void FskWindowCheckEventQueue(FskWindow win)
{
	// this routine is to run in the window's thread.
	// process the event at the front of our queue
	FskEvent fskEvent;

	if (win && (fskEvent = (FskEvent)FskListMutexRemoveFirst(win->eventQueue))) {
		windowIncrementUseCount(win);

		FskWindowEventSend(win, fskEvent);

		if (!FskListMutexIsEmpty(win->eventQueue))
			postProcessEventMsg(win);

		windowDecrementUseCount(win);
	}
}

void postProcessEventMsg(FskWindow win)
{
	// call us back soon so we can work on our event queue
#if TARGET_OS_WIN32
	PostMessage(win->hwnd, gEventMessage, 0, 0);
#elif TARGET_OS_MAC
    FskCocoaEventSend(win, kEventClassFsk, kEventFskProcessEvent);
#elif TARGET_OS_LINUX || (TARGET_OS_KPL && SUPPORT_LINUX_GTK)
	//@@ what here? Wake?
	// Maybe nothing. We check the Windows' event queues in the runloop to see if something needs to be done
	if (FskThreadGetCurrent() != win->thread) {
		FskInstrumentedItemPrintfDebug(win, "[%s] PostProcessEvent - window on other thread %s - wake", threadTag(FskThreadGetCurrent()), win->thread->name);
		FskThreadWake(win->thread);
	}
	else {
//		win->thread->wakePending++;
//		FskInstrumentedItemPrintfDebug(win, "[%s] PostProcessEvent - set wakePending on %s", threadTag(FskThreadGetCurrent()), win->thread->name);
	}
#endif /* TARGET_OS */
}

// FskErr FskWindowGetScreenBitmap(FskWindow win, FskBitmap *screenBits)
// {
// 	return kFskErrUnimplemented;
// }

void rotateAndScalePoints(FskWindow win, FskPointAndTicks pts, UInt32 count)
{
	SInt32 rotate = FskWindowRotateGet(win);
	SInt32 scale = FskWindowUIScaleGet(win);
	SInt32 windowScale = FskWindowScaleGet(win);
	UInt32 i;

	if ((FskIntToFixed(1) == scale) && (0 == rotate))
		return;

	for (i = 0; i < count; i++) {
		double portScale = FskPortDoubleScale(win->port, 1.0);

		pts[i].pt.x = FskPortUInt32Unscale(win->port, pts[i].pt.x) / windowScale;
		pts[i].pt.y = FskPortUInt32Unscale(win->port, pts[i].pt.y) / windowScale;

		if (90 == rotate) {
			SInt32 temp = pts[i].pt.x;
			pts[i].pt.x = pts[i].pt.y;
			pts[i].pt.y = (SInt32)(((win->bits->bounds.height / portScale)- 1) - temp);
		}
		else if (270 == rotate) {
			SInt32 temp = pts[i].pt.x;
			pts[i].pt.x = (SInt32)(((win->bits->bounds.width / portScale) - 1) - pts[i].pt.y);
			pts[i].pt.y = temp;
		}
		else if (180 == rotate) {
			pts[i].pt.x = (SInt32)(((win->bits->bounds.width  / portScale) - 1) - pts[i].pt.x);
			pts[i].pt.y = (SInt32)(((win->bits->bounds.height / portScale) - 1) - pts[i].pt.y);
		}
	}
}

#else /* !TARGET_OS_WIN32 && !!TARGET_OS_MAC && !!TARGET_OS_LINUX */

// stubs for non-GUI platforms

FskErr FskWindowNew(FskWindow *window, UInt32 width, UInt32 height, UInt32 windowStyle, FskWindowEventHandler handler, void *refcon) { return kFskErrUnimplemented; }
FskErr FskWindowDispose(FskWindow window) { return kFskErrUnimplemented; }

FskErr FskWindowSetSize(FskWindow window, UInt32 width, UInt32 height) { return kFskErrUnimplemented; }
FskErr FskWindowGetSize(FskWindow window, UInt32 *width, UInt32 *height) { return kFskErrUnimplemented; }

FskErr FskWindowSetLocation(FskWindow window, SInt32 x, SInt32 y) { return kFskErrUnimplemented; }
FskErr FskWindowGetLocation(FskWindow window, SInt32 *x, SInt32 *y) { return kFskErrUnimplemented; }

FskErr FskWindowSetTitle(FskWindow window, const char *title) { return kFskErrUnimplemented; }
FskErr FskWindowGetTitle(FskWindow window, char **title) { return kFskErrUnimplemented; }

FskErr FskWindowSetVisible(FskWindow window, Boolean visible) { return kFskErrUnimplemented; }
FskErr FskWindowGetVisible(FskWindow window, Boolean *visible) { return kFskErrUnimplemented; }

FskPort FskWindowGetPort(FskWindow window) { return NULL; }
FskErr FskWindowGetCursor(FskWindow window, FskPoint pt) { return kFskErrUnimplemented; }

void *FskWindowGetNativeWindow(FskWindow window) { return NULL; }

FskErr FskWindowUpdateRectangle(FskWindow window, const FskRectangle updateArea, Boolean backBufferUnchanged) { return kFskErrUnimplemented; }

FskErr FskWindowResetInvalidContentRectangle(FskWindow window) { return kFskErrUnimplemented; }
FskErr FskWindowInvalidateContentRectangle(FskWindow window, const FskRectangle area) { return kFskErrUnimplemented; }

FskErr FskWindowEventSend(FskWindow window, FskEvent event) { return kFskErrUnimplemented; }
FskErr FskWindowEventQueue(FskWindow window, FskEvent event) { return kFskErrUnimplemented; }

FskErr FskWindowEventSetHandler(FskWindow window, FskWindowEventHandler handler, void *refcon) { return kFskErrUnimplemented; }
FskErr FskWindowEventGetHandler(FskWindow window, FskWindowEventHandler *handler, void **refcon) { return kFskErrUnimplemented; }

FskErr FskWindowRequestStillDownEvents(FskWindow window, const FskRectangle r, UInt32 initialDelayMS, UInt32 intervalMS) { return kFskErrUnimplemented; }
FskErr FskWindowCancelStillDownEvents(FskWindow window) { return kFskErrUnimplemented; }

#endif /* !TARGET_OS_WIN32 && !!TARGET_OS_MAC && !!TARGET_OS_LINUX */

#if SUPPORT_EXTERNAL_SCREEN
static FskListMutex gExtScreenList = NULL;
static FskListMutex	gExtScreenChangeCBList = NULL;

struct FskExtScreenRecord {
	struct FskExtScreenRecord *next;
	int identifier;
	FskDimensionRecord size;
	FskInstrumentedItemDeclaration
};

#if SUPPORT_INSTRUMENTATION

static FskInstrumentedTypeRecord gExtScreenNotifierInstrumentation = {
	NULL,
	sizeof(FskInstrumentedTypeRecord),
	"externalScreen",
	FskInstrumentationOffset(FskExtScreenNotifierRecord),
	NULL,
	0,
	NULL,
	NULL
};
#endif /* SUPPORT_INSTRUMENTATION */
FskInstrumentedTypePrintfsDefine(ExtScreen, gExtScreenNotifierInstrumentation);

FskExtScreenNotifier FskExtScreenAddNotifier(FskExtScreenChangedCallback callback, void *param, char *debugName)
{
	FskExtScreenNotifier notif = NULL;
	FskThread thread = FskThreadGetCurrent();
	UInt32 nameLen = debugName ? FskStrLen(debugName) + 1 : 0;

	if (kFskErrNone == FskMemPtrNewClear(sizeof(FskExtScreenNotifierRecord) + nameLen, &notif)) {
		FskExtScreenPrintfDebug("ExtScreenNotifier NEW -- %x", notif);
		notif->callback = callback;
		notif->param = param;

		notif->thread = thread;
		if (nameLen)
			FskMemMove(notif->name, debugName, nameLen);

		FskListMutexPrepend(gExtScreenChangeCBList, notif);
		FskInstrumentedItemNew(notif, notif->name, &gExtScreenNotifierInstrumentation);
	}
	return notif;
}

void FskExtScreenRemoveNotifier(FskExtScreenNotifier callback)
{
	FskExtScreenPrintfDebug("ExtScreenNotifier REMOVE -- %x", callback);
	if (NULL != callback) {
		FskListMutexRemove(gExtScreenChangeCBList, callback);
		FskInstrumentedItemDispose(callback);
		FskMemPtrDispose(callback);
	}
	FskExtScreenPrintfDebug("ExtScreenNotifier REMOVE -- %x done", callback);
}

void FskExtScreenHandleConnected(int identifier, FskDimension size) {
#if SUPPORT_INSTRUMENTATION
	FskExtScreenPrintfDebug("ExtScreenNotifier Connected -- identifier is: %d", identifier);
#endif /* SUPPORT_INSTRUMENTATION */

	FskExtScreen screen = NULL;

	if (kFskErrNone == FskMemPtrNewClear(sizeof(FskExtScreenRecord), &screen)) {
		screen->identifier = identifier;
		screen->size = *size;

		FskListMutexPrepend(gExtScreenList, screen);

		FskExtScreenNotifier notif = (FskExtScreenNotifier)gExtScreenChangeCBList->list;
		while (notif) {
			(*notif->callback)(kFskExtScreenStatusNew, screen->identifier, &screen->size, notif->param);
			notif = notif->next;
		}
	}
}

void FskExtScreenHandleDisconnected(int identifier) {
#if SUPPORT_INSTRUMENTATION
	FskExtScreenPrintfDebug("ExtScreenNotifier Disconnected -- identifier is: %d", identifier);
#endif /* SUPPORT_INSTRUMENTATION */

	FskExtScreen screen = NULL;

	screen = (FskExtScreen)gExtScreenList->list;
	while (screen) {
		if (screen->identifier == identifier) {
			break;
		}
		screen = screen->next;
	}

	if (screen) {
		FskExtScreenNotifier notif = (FskExtScreenNotifier)gExtScreenChangeCBList->list;
		while (notif) {
			(*notif->callback)(kFskExtScreenStatusRemoved, screen->identifier, NULL, notif->param);
			notif = notif->next;
		}
	}
}

void FskExtScreenHandleChanged(int identifier, FskDimension newSize) {
#if SUPPORT_INSTRUMENTATION
	FskExtScreenPrintfDebug("ExtScreenNotifier Changed -- identifier is: %d", identifier);
#endif /* SUPPORT_INSTRUMENTATION */

	FskExtScreen screen = NULL;

	screen = (FskExtScreen)gExtScreenList->list;
	while (screen) {
		if (screen->identifier == identifier) {
			break;
		}
		screen = screen->next;
	}

	if (screen) {
		screen->size = *newSize;

		FskExtScreenNotifier notif = (FskExtScreenNotifier)gExtScreenChangeCBList->list;
		while (notif) {
			(*notif->callback)(kFskExtScreenStatusChanged, screen->identifier, &screen->size, notif->param);
			notif = notif->next;
		}
	}
}
#endif	/* SUPPORT_EXTERNAL_SCREEN */

FskErr FskWindowInitialize(void)
{
	FskErr err;
	err = FskListMutexNew(&gWindowList, "windowList");
	BAIL_IF_ERR(err);

#if SUPPORT_EXTERNAL_SCREEN
	err = FskListMutexNew(&gExtScreenList, "screenList");
	BAIL_IF_ERR(err);

	err = FskListMutexNew(&gExtScreenChangeCBList, "screenChangeCBList");
	BAIL_IF_ERR(err);
#endif	/* SUPPORT_EXTERNAL_SCREEN */

bail:
	return kFskErrNone;
}

void FskWindowTerminate(void)
{
	while (gWindowList) {
		FskWindow w = (FskWindow)FskListMutexRemoveFirst(gWindowList);
		if (NULL == w) {
			FskListMutexDispose(gWindowList);
			gWindowList = NULL;
			break;
		}
		FskWindowDispose(w);
	}

#if SUPPORT_EXTERNAL_SCREEN
	FskListMutexDispose(gExtScreenChangeCBList);
	FskListMutexDispose(gExtScreenList);
#endif	/* SUPPORT_EXTERNAL_SCREEN */

}


#if TARGET_OS_LINUX || TARGET_OS_MAC || TARGET_OS_KPL

Boolean FskWindowCheckEvents() {
	FskWindow	win = NULL;
	Boolean		ret = false;
	FskThread	thread;

	thread = FskThreadGetCurrent();

//tryAgain:
	while (NULL != (win = FskListMutexGetNext(gWindowList, win))) {
		if (thread != win->thread)
			continue;

#if TARGET_OS_ANDROID
		if (gAndroidCallbacks->noWindowDontDrawCB()) {
		    FskPortResetInvalidRectangle(win->port);
		}
#endif

		if (!FskListMutexIsEmpty(win->eventQueue)) {

			FskWindowCheckEventQueue(win);
		}

		if (!FskListMutexContains(gWindowList, win)) {
			win = NULL;
			continue;
		}

		if ((!FskRectangleIsEmpty(&win->port->invalidArea))) {
#ifdef PETERS_HACK
			FskInstrumentedItemPrintfDebug(win, "FskWindowCheckEvents: near test for updateTimer");
#endif /* PETERS_HACK */
#if TARGET_OS_MAC && USE_DISPLAY_LINK
			if (FskCocoaWindowDisplayPaused(win))
				sendEventWindowUpdate(win, false, false, NULL);
#else
			if (!win->updateTimer && !win->useFrameBufferUpdate)
				;		// we will update the window in a different way
			else if (win->useFrameBufferUpdate && win->doBeforeUpdate && !win->updateSuspended)
				;       // The continues drawing is on and running, wait it to update
			else if (!win->updateSuspended && FskListContains(&thread->timerCallbacks, win->updateTimer))
				;		// we will draw soon. wait to do that so the period remains consistent.
			else {
				sendEventWindowUpdate(win, false, false, NULL);
			}
#endif
		}
		if (!FskListMutexIsEmpty(win->eventQueue)) {
			ret = true;
		}
	}

	return ret;
}

#endif

#if TARGET_OS_ANDROID || TARGET_OS_KPL
void FskWindowGetNextEventTime(FskThread thread, FskTime nextEventTime)
{
	FskWindow w;
	FskEvent ev = NULL;

	FskTimeClear(nextEventTime);

    FskListMutexAcquireMutex(gWindowList);
    w = gWindowList->list;
	while (w) {
		if ((NULL == thread) || (w->thread == thread)) {
			if (NULL != (ev = (FskEvent)FskListMutexGetNext(w->eventQueue, ev))) {
//FskInstrumentedItemPrintfDebug(w, "[%s] FskWindowGetNextEventTime got an event for time %d.%03d [%d:%s]", threadTag(thread), ev->atTime.seconds, ev->atTime.useconds / 1000, ev->eventCode, nameFromEventCode(ev->eventCode));
				if (1 == FskTimeCompare(nextEventTime, &ev->atTime))
{
					FskTimeCopy(nextEventTime, &ev->atTime);
//FskInstrumentedItemPrintfDebug(w, "[%s] FskWindowGetNextEventTime returns %d.%03d", threadTag(thread), nextEventTime->seconds, nextEventTime->useconds / 1000);
}
			}
		}
		w = w->next;
	}
    FskListMutexReleaseMutex(gWindowList);
	if (nextEventTime->seconds == 0 && nextEventTime->useconds == 0)
		FskTimeGetNow(nextEventTime);
}
#endif /* TARGET_OS_ANDROID || TARGET_OS_KPL */

void FskWindowCheckUpdate(void)
{
	FskWindow walker = NULL;

	while (true) {
		walker = (FskWindow)FskListMutexGetNext(gWindowList, walker);
		if (NULL == walker)
			break;
		if ( (false == FskRectangleIsEmpty(&walker->port->invalidArea)))
			FskWindowUpdate(walker, NULL);
	}
}

void windowUpdateCallback(struct FskTimeCallBackRecord *callback, const FskTime time, void *param)
{
	FskWindow win = (FskWindow)param;
	FskEvent fskEvent;

//    FskFrameBufferGrabScreenForDrawing();		// 103253 - hangs Yammer login

	windowIncrementUseCount(win);

#if TARGET_OS_ANDROID && defined(PETERS_HACK)
    // service pending events, because they may include window resize events
    while (LinuxHandleThreadEvents())
        ;
#endif

	if (win->doBeforeUpdate && (kFskErrNone == FskEventNew(&fskEvent, kFskEventWindowBeforeUpdate, NULL, kFskEventModifierNotSet)))
		FskWindowEventSend(win, fskEvent);

	if (FskRectangleIsEmpty(&win->port->invalidArea)) {
		if (!win->doBeforeUpdate) {
			win->updateSuspended = true;
			goto done;
		}
	}
	else
		sendEventWindowUpdate(win, false, true, NULL);

	scheduleWindowUpdateCallback(win);

done:
	windowDecrementUseCount(win);

//    FskFrameBufferReleaseScreenForDrawing(); // 103253 - hangs Yammer login
}

void scheduleWindowUpdateCallback(FskWindow win)
{
	if (!win->updateInterval || !win->updateTimer)
		return;

	if (win->updateSuspended) {
		// after an immediate draw, so schedule one interval ahead
		win->updateSuspended = false;
		FskTimeGetNow(&win->nextUpdate);
	}

	FskTimeAddMS(&win->nextUpdate, win->updateInterval);

	FskTimeCallbackSet(win->updateTimer, &win->nextUpdate, windowUpdateCallback, win);

#if SUPPORT_INSTRUMENTATION && 0
	{
		FskTimeRecord now, delta = win->nextUpdate;

		FskTimeGetNow(&now);
		FskTimeSub(&now, &delta);
		FskInstrumentedItemPrintfDebug(win, "next update in %d ms", (int)FskTimeInMS(&delta));
	}
#endif
}

#if TARGET_OS_MAC || TARGET_OS_WIN32 || TARGET_OS_KPL

void updateWindowRotation(FskWindow window)
{
	if (window->rotationDisabled > 0)
		window->rotation = 0;
	else {
		if (window->requestedRotation >= 0)
			window->rotation = window->requestedRotation;
	}
}

#endif

void setWindowBitmap(FskWindow w, FskBitmap bits)
{
	w->bits = bits;
	FskPortSetBitmap(w->port, bits);

	if (w->eventHandler) {
		FskEvent e;

		if (kFskErrNone == FskEventNew(&e, kFskEventWindowBitmapChanged, NULL, kFskEventModifierNotSet)) {
			if (FskInstrumentedItemHasListeners(w))
				FskInstrumentedItemSendMessage(w, kFskWindowInstrMsgDispatchEvent, e);
			(w->eventHandler)(e, e->eventCode, w, w->eventHandlerRefcon);
			FskEventDispose(e);
		}
	}
}

#if FSKBITMAP_OPENGL

void setWindowAsSysContext(FskWindow w)
{
	FskBitmap bits;

	if (!w->usingGL)
		return;

	bits = w->rotationBits ? w->rotationBits : w->bits;
	if (!bits) return;

	if (w->glPortForWindow) {
		if (bits->glPort && (bits->glPort != w->glPortForWindow)) {
			FskGLPortDispose(bits->glPort);
			bits->glPort = w->glPortForWindow;
			FskGLPortResize(bits->glPort, bits->bounds.width, bits->bounds.height);	/* This must be done in the GL thread */
		}
	}

	if (bits->glPort) {
		#if TARGET_OS_ANDROID
			LOGD("[%s] setWindowAsGLSysContext(%p, %p) setting OpenGLSysContext with FskGLPortResize", FskThreadName(FskThreadGetCurrent()), w, bits);
		#endif /* TARGET_OS_ANDROID */
		FskBitmapSetOpenGLSysContext(bits, w);
		FskGLPortResize(bits->glPort, bits->bounds.width, bits->bounds.height);
	}
	else {
		#if TARGET_OS_ANDROID
			if (NULL != (bits->glPort = FskGLPortGetCurrent())) {
				FskBitmapSetOpenGLSysContext(bits, w);
				FskGLPortResize(bits->glPort, bits->bounds.width, bits->bounds.height);
				LOGD("[%s] setWindowAsGLSysContext(%p, %p) setting glPort (%p) to current", FskThreadName(FskThreadGetCurrent()), w, bits, bits->glPort);
			}
			else {
				FskGLPortNew(bits->bounds.width, bits->bounds.height, w, &bits->glPort);
				LOGD("[%s] setWindowAsGLSysContext(%p, %p) setting new GLPort (%p) OpenGLSysContext", FskThreadName(FskThreadGetCurrent()), w, bits, bits->glPort);
			}
		#else /* !TARGET_OS_ANDROID */
			FskGLPortNew(bits->bounds.width, bits->bounds.height, w, &bits->glPort);
			#if TARGET_OS_KPL
				FskGLPortSetSysContext(bits->glPort, w);
			#endif /* TARGET_OS_KPL */
		#endif /* !TARGET_OS_ANDROID */
	}

	if (!w->glPortForWindow)
		w->glPortForWindow = bits->glPort;
}

#endif

#if TARGET_OS_WIN32

BOOL CALLBACK enumMonitor(HMONITOR hMonitor, HDC hdc, LPRECT lpRMonitor, LPARAM dwData)
{
	MONITORINFOEX mi;

	memset(&mi, 0, sizeof(MONITORINFOEX));
	mi.cbSize = sizeof(MONITORINFOEX);
	if (GetMonitorInfo(hMonitor, (MONITORINFO *)&mi)) {
		HDC hDC = CreateDC("DISPLAY", mi.szDevice, NULL, NULL);
		if (hDC) {
			UInt32 depth = GetDeviceCaps(hDC, BITSPIXEL) * GetDeviceCaps(hDC, PLANES);
			UInt32 *outDepth = (UInt32 *)dwData;

			if (depth > *outDepth)
				*outDepth = depth;

			DeleteDC(hDC);
		}
	}
	return true;
}

UInt32 getScreenPixelFormat(void)
{
	UInt32 pixelFormat = 0;

	#if FSKBITMAP_OPENGL
		const char *value = FskEnvironmentGet("useGL");
		if (value && (0 == FskStrCompare("1", value)) && (kFskErrNone == FskGLInit(NULL)))
			return kFskBitmapFormatGLRGBA;
	#endif /* FSKBITMAP_OPENGL */

	EnumDisplayMonitors(NULL, NULL, enumMonitor, (LPARAM)&pixelFormat);
	switch (pixelFormat) {
		case 16:
			pixelFormat = kFskBitmapFormat16RGB565LE;
			break;

		case 24:
			pixelFormat = kFskBitmapFormat24BGR;
			break;

		case 32:
			pixelFormat = kFskBitmapFormat32BGRA;
			break;

		default:
			pixelFormat = kFskBitmapFormat24BGR;
			break;
	}

	return pixelFormat;
}

#elif TARGET_OS_MAC

UInt32 getScreenPixelFormat(void)
{
	#if FSKBITMAP_OPENGL
		const char *value = FskEnvironmentGet("useGL");
		if (value && (0 == FskStrCompare("1", value)) && (kFskErrNone == FskGLInit(NULL)))
			return kFskBitmapFormatGLRGBA;
	#endif /* FSKBITMAP_OPENGL */
	return kFskBitmapFormatDefault;
}

#elif (TARGET_OS_KPL && SUPPORT_LINUX_GTK)
UInt32 getScreenPixelFormat(void)
{
	#if FSKBITMAP_OPENGL
		const char *value = FskEnvironmentGet("useGL");
		if (value && (0 == FskStrCompare("1", value)) && (kFskErrNone == FskGLInit(NULL)))
			return kFskBitmapFormatGLRGBA;
	#endif /* FSKBITMAP_OPENGL */
	//return FskGtkWindowGetPixelFormat();
	return kFskBitmapFormat32RGBA;
}


#endif /* !TARGET_OS_WIN32 */

//@@jph what is this about
//this is a connection from the android libraries to allow us to grab the old frame prior to the hardware rotating the frame buffer so that we can use it as a source for our own rotation animation
#if TARGET_OS_ANDROID
void FskWindowAndroidSizeChanged(int win)
{
	needMoreRedraw = 1;
	sendEventWindowSizeChanged((FskWindow)win);  //TODO: fix this error for 64bits system!!!
}
#endif /* TARGET_OS_ANDROID */

#if TARGET_OS_MAC
void FskWindowCocoaSizeChanged(FskWindow win)
{
	sendEventWindowSizeChanged(win);

	if (!FskRectangleIsEmpty(&win->port->invalidArea))
		sendEventWindowUpdate(win, false, false, NULL);
}

void FskWindowCocoaClose(FskWindow win)
{
	sendEventWindowClose(win);
}

#endif /* TARGET_OS_MAC */


#if TARGET_OS_KPL
FskErr KplUIEventHandler(KplUIEvent kplEvent, void *refcon)
{
	FskWindow win = (FskWindow)refcon;
	FskErr err = kFskErrNone;

	if (NULL != kplEvent) {
		FskEvent fskEvent;
		err = KplUIEventToFskEvent(kplEvent, &fskEvent);
		if (0 == err)
			FskWindowEventQueue(win, fskEvent);
		KplUIEventDispose(kplEvent);
	}

	// Return "kFskErrNeedMoreTime" to the caller if there are more events to process, so that the caller
	// knows to get back here ASAP.
	if (FskWindowCheckEvents())
		err = kFskErrNeedMoreTime;

	return err;
}

#if SUPPORT_LINUX_GTK
void FskWindowGtkClose(FskWindow win)
{
	sendEventWindowClose(win);
}

void FskWindowGtkSizeChanged(FskWindow win)
{
	sendEventWindowSizeChanged(win);
}
#endif
#endif /* TARGET_OS_KPL */

#if SUPPORT_INSTRUMENTATION

Boolean doFormatMessageWindow(FskInstrumentedType dispatch, UInt32 msg, void *msgData, char *buffer, UInt32 bufferSize)
{
	switch (msg) {
		case kFskWindowInstrMsgDispatchEvent: {
			FskEvent event = (FskEvent)msgData;
			const char *eventName = FskInstrumentedItemGetName(event);

			if (eventName)
				snprintf(buffer, bufferSize, "received event %s (%p)", eventName, event);
			else
				snprintf(buffer, bufferSize, "received event %d (%p)", (int)event->eventCode, event);
			}
			return true;

		case kFskWindowInstrQueueEvent: {
			FskEvent event = (FskEvent)msgData;
			const char *eventName = FskInstrumentedItemGetName(event);

			if (eventName)
				snprintf(buffer, bufferSize, "queue event %s (%p)", eventName, event);
			else
				snprintf(buffer, bufferSize, "queue event %d (%p)", (int)event->eventCode, event);
			}
			return true;


		case kFskWindowInstrSetSize:
			snprintf(buffer, bufferSize, "setsize");
			return true;

		case kFskWindowInstrSetLocation:
			snprintf(buffer, bufferSize, "setlocation");
			return true;

		case kFskWindowInstrSetTitle:
			snprintf(buffer, bufferSize, "settitle to %s", (char*)msgData);
			return true;

		case kFskWindowInstrSetVisible:
			snprintf(buffer, bufferSize, msgData ? "setvisible true" : "setvisible false");
			return true;

		case kFskWindowInstrCopyBitsToScreen: {
			FskRectangle r = (FskRectangle)msgData;
			snprintf(buffer, bufferSize, "copybits to screen, x=%d, y=%d, w=%d, h=%d", (int)r->x, (int)r->y, (int)r->width, (int)r->height);
			}
			return true;

		case kFskWindowInstrInvalidate: {
			FskRectangle r = (FskRectangle)msgData;
			snprintf(buffer, bufferSize, "invalidate, x=%d, y=%d, w=%d, h=%d", (int)r->x, (int)r->y, (int)r->width, (int)r->height);
			}
			return true;
#if TARGET_OS_WIN32
		case kFskWindowInstrWindowsMessage: {
			LONG *longs = (LONG *)msgData;
			snprintf(buffer, bufferSize, "Win32 message, msg=%#x, wParam=%#x, lParam=%#x", longs[0], longs[1], longs[2]);
			}
			return true;
#endif /* TARGET_OS_WIN32 */
	}

	return false;
}

#endif /* SUPPORT_INSTRUMENTATION */
