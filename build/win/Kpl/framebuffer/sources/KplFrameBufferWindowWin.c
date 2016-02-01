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
#define __FSKBITMAP_PRIV__
#define __FSKWINDOW_PRIV__
#define __FSKPORT_PRIV__
#include "FskWindow.h"
#include "FskPort.h"
#include "FskBitmap.h"
#include "FskFrameBuffer.h"
#include "KplScreen.h"
#include "KplUIEvents.h"

#include <windowsx.h>

#define kFskWindowClassName "FskWnd1"

extern HINSTANCE hInst;

static long FAR PASCAL FskWindowWndProc(HWND hwnd, UINT msg, UINT wParam, LONG lParam);

HWND createWindow(UInt32 width, UInt32 height)
{
	static Boolean registeredWindowClass = false;
	RECT rSize;
	HWND hWnd;
	
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

	hWnd = CreateWindowEx(0, kFskWindowClassName, "frame buffer",
							WS_CAPTION | WS_CLIPCHILDREN,
							CW_USEDEFAULT, CW_USEDEFAULT,
							width, height,
							NULL, NULL, hInst, NULL);
	ShowWindow(hWnd, SW_SHOW);
	return hWnd;
}

void copyBitsToWindow(HWND hWnd, FskBitmap screenBitmap)
{
	HDC	bitsDC, hdc = GetDC(hWnd);
	
	KplScreenGetAuxInfo((unsigned char**)&bitsDC, NULL);
	
	BitBlt(hdc, 0, 0, screenBitmap->bounds.width, screenBitmap->bounds.height,
		bitsDC, 0, 0,
		SRCCOPY);
		
	ReleaseDC(hWnd, hdc);
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
		case WM_PAINT: {
			PAINTSTRUCT ps;
			FskBitmap screenBitmap;
			
			BeginPaint(hwnd, &ps);
			
			// Here we repaint the window from the frame buffer by simply locking and unlocking
			FskFrameBufferGetScreenBitmap(&screenBitmap);
			FskFrameBufferLockSurface(screenBitmap, NULL, NULL);
			FskFrameBufferUnlockSurface(screenBitmap);
			
			EndPaint(hwnd, &ps);
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


