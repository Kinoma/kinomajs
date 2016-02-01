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
#include "Kpl.h"
#include "KplGL.h"

#include "FskMemory.h"
#include "FskEvent.h"
#include "FskUtilities.h"
#include "KplUIEvents.h"

#include "windowsx.h"

#define GLES_VERSION	2
#define CONTEXT565		1
//#define CONTEXT8888		1

#define kWindowWidth	320
#define kWindowHeight	240

static HWND ghWnd = NULL;
static void createWindow(UInt32 width, UInt32 height);

/*******************************************************************************
 * KplGLInitialize
 *******************************************************************************/
FskErr KplGLInitialize(EGLDisplay *displayOut, EGLSurface *surfaceOut, EGLContext *contextOut, void **nativeWindowOut)
{
	static const EGLint configAttribs8888[] = {
		EGL_BUFFER_SIZE,		32,
		EGL_ALPHA_SIZE,			8,
		EGL_RED_SIZE,			8,
		EGL_GREEN_SIZE,			8,
		EGL_BLUE_SIZE,			8,
		EGL_DEPTH_SIZE,			0,
		EGL_STENCIL_SIZE,		0,
		EGL_SAMPLES,			0,
		EGL_COLOR_BUFFER_TYPE,	EGL_RGB_BUFFER,
#if GLES_VERSION == 1
		EGL_RENDERABLE_TYPE,	EGL_OPENGL_ES_BIT,
#else
		EGL_RENDERABLE_TYPE,	EGL_OPENGL_ES2_BIT,
#endif
		EGL_SURFACE_TYPE,		EGL_WINDOW_BIT,
		EGL_NONE,				EGL_NONE
	};
	static const EGLint configAttribs565[] = {
		EGL_BUFFER_SIZE,		16,
		EGL_ALPHA_SIZE,			0,
		EGL_RED_SIZE,			5,
		EGL_GREEN_SIZE,			6,
		EGL_BLUE_SIZE,			5,
		EGL_DEPTH_SIZE,			0,
		EGL_STENCIL_SIZE,		0,
		EGL_SAMPLES,			0,
		EGL_COLOR_BUFFER_TYPE,	EGL_RGB_BUFFER,
#if GLES_VERSION == 1
		EGL_RENDERABLE_TYPE,	EGL_OPENGL_ES_BIT,
#else
		EGL_RENDERABLE_TYPE,	EGL_OPENGL_ES2_BIT,
#endif
		EGL_SURFACE_TYPE,		EGL_WINDOW_BIT,
		EGL_NONE,				EGL_NONE
	};
	static const EGLint ctxAttr[] = {
		EGL_CONTEXT_CLIENT_VERSION,	GLES_VERSION,
		EGL_NONE,					EGL_NONE
	};
	FskErr		err			= kFskErrNone;
	EGLDisplay	display		= NULL;
	EGLSurface	surface		= NULL;
	EGLContext	context		= NULL;
	EGLConfig	*config		= NULL;
	EGLint		numConfigs;
	const EGLint *attribList;
#ifdef CONTEXT565
	attribList = configAttribs565;
#else
	attribList = configAttribs8888;
#endif	

	if (NULL == ghWnd)
		createWindow(kWindowWidth, kWindowHeight);
		
	BAIL_IF_FALSE(EGL_NO_DISPLAY != (display = eglGetDisplay((NativeDisplayType)EGL_DEFAULT_DISPLAY)), err, kFskErrGraphicsContext);
	BAIL_IF_FALSE(                  eglInitialize(display, NULL, NULL), err, kFskErrGraphicsContext);	/* (display, &major, &minor) */
	BAIL_IF_FALSE(eglChooseConfig(display, attribList, NULL, 0, &numConfigs), err, kFskErrGraphicsContext);
	BAIL_IF_ZERO(numConfigs, err, kFskErrMismatch);
	BAIL_IF_ERR(err = FskMemPtrNew(numConfigs * sizeof(*config), (FskMemPtr*)(void*)(&config)));
	BAIL_IF_FALSE(eglChooseConfig(display, attribList, config, numConfigs, &numConfigs), err, kFskErrGraphicsContext);

	BAIL_IF_FALSE(EGL_NO_SURFACE !=	(surface = eglCreateWindowSurface(display, config[0], ghWnd, NULL)), err, kFskErrGraphicsContext);
	BAIL_IF_FALSE(EGL_NO_CONTEXT != (context = eglCreateContext(display, config[0], EGL_NO_CONTEXT, ctxAttr)), err, kFskErrGraphicsContext);
	BAIL_IF_FALSE(EGL_FALSE      != eglMakeCurrent(display, surface, surface, context), err, kFskErrGraphicsContext);

bail:
	if (err) {
		if (display) {
			if (surface)
				eglDestroySurface(display, surface);
			if (context)
				eglDestroyContext(display, context);
			//eglTerminate(display);
		}
		display = NULL;
		surface = NULL;
		context = NULL;
	}

	*displayOut = display;
	*surfaceOut = surface;
	*contextOut = context;
	*nativeWindowOut = ghWnd;
	
	return err;
}

/*******************************************************************************
 * KplGLTerminate
 *******************************************************************************/
void KplGLTerminate(void)
{
}

/*******************************************************************************
 * Local functions
 *******************************************************************************/
#define kFskWindowClassName "FskWnd1"

extern HINSTANCE hInst;

static long FAR PASCAL FskWindowWndProc(HWND hwnd, UINT msg, UINT wParam, LONG lParam);

void createWindow(UInt32 width, UInt32 height)
{
	static Boolean registeredWindowClass = false;
	RECT rSize;
	
	if (!registeredWindowClass) {
		WNDCLASS wc = {0};
		wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
		wc.lpfnWndProc = FskWindowWndProc;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;
		wc.hInstance = hInst;
		wc.hIcon = NULL;
		wc.hCursor = LoadCursor(NULL, IDC_ARROW);
		wc.hbrBackground = NULL;
		wc.lpszMenuName = NULL;
		wc.lpszClassName = kFskWindowClassName;
		RegisterClass(&wc);
		registeredWindowClass = true;
	}
	
	rSize.top = 0;
	rSize.left = 0;
	rSize.right = width;
	rSize.bottom = height;
	AdjustWindowRectEx(&rSize, WS_CAPTION, false, 0);
	width = rSize.right - rSize.left;
	height = rSize.bottom - rSize.top;

	ghWnd = CreateWindowEx(0, kFskWindowClassName, "OpenGL KPL native window",
							WS_CAPTION | WS_CLIPCHILDREN,
							CW_USEDEFAULT, CW_USEDEFAULT,
							width, height,
							NULL, NULL, hInst, NULL);
	ShowWindow(ghWnd, SW_SHOW);
}

long FAR PASCAL FskWindowWndProc(HWND hwnd, UINT msg, UINT wParam, LONG lParam)
{
	UInt32 eventID;
	long result = 0;
	KplUIEvent event = NULL;
	
	switch (msg) {
		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
			if (WM_LBUTTONDOWN == msg) {
				eventID = kKplUIEventMouseDown;
				SetCapture(hwnd);
			}
			else {
				eventID = kKplUIEventMouseUp;
				ReleaseCapture();
			}
			if (kFskErrNone == KplUIEventNew(&event, eventID, NULL)) {
				event->mouse.x = GET_X_LPARAM(lParam);
				event->mouse.y = GET_Y_LPARAM(lParam);
			}
			break;
		case WM_ACTIVATE:
			KplUIEventNew(&event, (WA_INACTIVE == LOWORD(wParam)) ? kKplUIEventScreenInactive : kKplUIEventScreenActive, NULL);
			break;
		case WM_CHAR: {
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
			if (kFskErrNone == KplUIEventNew(&event, kKplUIEventKeyDown, NULL)) {
				event->key.keyCode = wParam;
				event->key.modifiers = modifiers;
			}
			}
			break;
		default:
			result = DefWindowProc(hwnd, msg, wParam, lParam);
			break;
	}

	if (NULL != event)
		KplUIEventSend(event);
		
	return result;
}
