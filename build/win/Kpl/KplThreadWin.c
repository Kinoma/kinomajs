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
#define _WIN32_WINNT 0x0400

#define __FSKUTILITIES_PRIV__
#define __FSKNETUTILS_PRIV__
#define __FSKTHREAD_PRIV__
#define __FSKWINDOW_PRIV__
#define __FSKTIME_PRIV__
#define __FSKPORT_PRIV__
#define __FSKEVENT_PRIV__
#define __FSKEXTENSIONS_PRIV__
#include "FskThread.h"
#include "FskExtensions.h"
#include "FskWindow.h"
#include "FskMain.h"

#include "FskList.h"
#include "FskMemory.h"
#include "FskPlatformImplementation.h"
#include "FskSynchronization.h"

#include "KplThreadWinPriv.h"
#include "KplTimeWinPriv.h"
#include "KplSocket.h"

#include "Windows.h"
#include "Windowsx.h"
#include <process.h>

#include "FskNetUtils.h"

static unsigned int __stdcall kplThreadProc(void *refcon);

FskListMutex gKplThreads = NULL;

UINT gThreadEventMessage;
DWORD gFskThreadTLSSlot;

extern UINT gAsyncSelectMessage;
extern HINSTANCE hInst;

static void makeThreadWindow(KplThread thread);
static long FAR PASCAL threadWndProc(HWND hwnd, UINT msg, UINT wParam, LONG lParam);
static void win32SocketEvent(UINT wParam, UINT event);

FskErr KplThreadCreate(KplThread *threadOut, KplThreadProc procedure, void *refcon, UInt32 flags, const char *name)
{
	FskErr			err;
	KplThread	thread = NULL;
	
	err = FskMemPtrNewClear(sizeof(KplThreadRecord), (FskMemPtr*)&thread);
	if (err) goto bail;
	
#if SUPPORT_TIMER_THREAD
	thread->nextCallbackTime.seconds = ~0;
	thread->waitableTimer = CreateEvent(NULL, false, false, NULL);
#endif

	FskListMutexPrepend(gKplThreads, thread);

	thread->flags = flags;
	thread->clientProc = procedure;
	thread->clientRefCon = refcon;

	thread->handle = (HANDLE)_beginthreadex(NULL, 0, kplThreadProc, thread, 0, &thread->id);
	if (0 == thread->handle) {
		err = kFskErrOperationFailed;
		goto bail;
	}
	
	if (thread->flags & kFskThreadFlagsHighPriority)
		SetThreadPriority(thread->handle, THREAD_PRIORITY_ABOVE_NORMAL);
	else if (thread->flags & kFskThreadFlagsLowPriority)
		SetThreadPriority(thread->handle, THREAD_PRIORITY_BELOW_NORMAL);

bail:
	if (err) {
		KplThreadJoin(thread);
		thread = NULL;
	}
	*threadOut = (KplThread)thread;
	
	return err;
}

FskErr KplThreadJoin(KplThread kplThread)
{
	KplThread thread = (KplThread)kplThread;
	
	if (!thread) return kFskErrNone;
	
#if SUPPORT_WAITABLE_TIMER || SUPPORT_TIMER_THREAD
	if (thread->waitableTimer)
		CloseHandle(thread->waitableTimer);
#endif
	if (thread->handle)
		CloseHandle(thread->handle);
	
	if (NULL != gKplThreads)
		FskListMutexRemove(gKplThreads, thread);
	
	FskMemPtrDispose((FskMemPtr)thread);
	
	return kFskErrNone;
}

KplThread KplThreadGetCurrent(void)
{
	return (KplThread)TlsGetValue(gFskThreadTLSSlot);
}

void KplThreadYield(void)
{
	Sleep(10);
}

void KplThreadWake(KplThread kplThread)
{
	KplThread thread = (KplThread)kplThread;
	PostMessage(thread->window, gThreadEventMessage, 0, 0);
}

void KplThreadPostEvent(KplThread kplThread, void *event)
{
	KplThread thread = (KplThread)kplThread;
	PostMessage(thread->window, gThreadEventMessage, 0, (LPARAM)event);
}

FskErr KplThreadCreateMain(KplThread *kplThread)
{
	KplThread thread = NULL;
	FskErr err;
	
	gFskThreadTLSSlot = TlsAlloc();
	gThreadEventMessage = RegisterWindowMessage("FskThreadEvent");
	
	err = FskMemPtrNewClear(sizeof(KplThreadRecord), (FskMemPtr*)&thread);
	if (err) goto bail;
	
	err = FskListMutexNew(&gKplThreads, "gKplThreads");
	if (err) return err;

#if SUPPORT_TIMER_THREAD
	thread->nextCallbackTime.seconds = ~0;
	thread->waitableTimer = CreateEvent(NULL, false, false, NULL);
	timeBeginPeriod(5);
#endif

	thread->id = GetCurrentThreadId();
	makeThreadWindow(thread);

	FskListMutexPrepend(gKplThreads, thread);

bail:
	if (0 != err) {
		FskMemPtrDispose(thread);
		thread = NULL;
	}
	
	*kplThread = (KplThread)thread;
	
	return err;
}

FskErr KplThreadTerminateMain(void)
{
	TlsSetValue(gFskThreadTLSSlot, 0);
	
	FskMutexAcquire(gKplThreads->mutex);
	FskListMutexDispose(gKplThreads);
	gKplThreads = NULL;
	
	return kFskErrNone;
}

void KplThreadNotifyPendingSocketData(void *socket, Boolean pendingReadable, Boolean pendingWritable)
{
	KplThread thread = KplThreadGetCurrent();
	
	PostMessage(thread->window, gAsyncSelectMessage, (WPARAM)socket,
		(pendingReadable ? FD_READ : 0) | (pendingWritable ? FD_WRITE : 0));
}

void KplThreadNotifyClientComplete(KplThread kplThread)
{
}

void *KplThreadGetRefcon(KplThread kplThread)
{
	return (NULL != kplThread) ? kplThread->clientRefCon : NULL;
}

#define FSK_WM_MOUSEMOVE (WM_APP + 0x1123)
#define FSK_WM_MOUSE_WITH_TIME (WM_APP + 0x1124)

FskErr KplThreadRunloopCycle(SInt32 msTimeout)
{
	MSG msg;
	FskWindow win;
	DWORD anchorTicks, ticks;

	KplThread thread = (KplThread)KplThreadGetCurrent();
	const int kFskMWMOFlags = MWMO_ALERTABLE | MWMO_INPUTAVAILABLE;

	Boolean flushMouse = false, timedOut = false;

	if (!thread)
		return kFskErrBadState;

	// we need to wait in an alertable state so APCs can call immediately (e.g. ReadDirectoryChangesW callback)
	while (true) {

#if SUPPORT_WAITABLE_TIMER || SUPPORT_TIMER_THREAD
		if (thread->waitableTimer) {
			DWORD result;
			DWORD waitMS;
			if (0 != thread->nextTicks) {
				DWORD now = GetTickCount();
				if (now >= thread->nextTicks)
					waitMS = 0;
				else
					waitMS = thread->nextTicks - now;
			}
			else
				waitMS = INFINITE;
			result = MsgWaitForMultipleObjectsEx(1, &thread->waitableTimer, waitMS, QS_ALLINPUT, kFskMWMOFlags);
			if ((WAIT_OBJECT_0 == result) || (WAIT_IO_COMPLETION == result))
				KplTimeCallbackService(thread);
			else {
				timedOut = WAIT_TIMEOUT == result;
				break;
			}
		}
		else
#endif
		{
			HANDLE h;
			DWORD result = MsgWaitForMultipleObjectsEx(0, &h, INFINITE, QS_ALLINPUT, kFskMWMOFlags);

			timedOut = WAIT_TIMEOUT == result;
			if ((WAIT_OBJECT_0 == result) || timedOut)
				break;
		}
	}

	anchorTicks = GetTickCount();
	flushMouse = timedOut && (0 != thread->nextTicks);
	thread->nextTicks = 0;
	while (true) {
		Boolean haveMessage = PeekMessage(&msg, NULL, 0, 0, true);
		Boolean adjustMouseMessages;

		if (thread->haveMouseMove) {
			if (!haveMessage && !flushMouse) {
				DWORD now = GetTickCount();
				DWORD delta = now - thread->points[1].ticks;
				UInt32 kThreshold;
				win = FskWindowGetActive();
				kThreshold = win ? win->updateInterval : 30;
				if (0 != thread->nextTicks)
					break;
				if (delta < kThreshold) {
					thread->nextTicks = ((now + kThreshold) / kThreshold) * kThreshold;
					break;
				}
			}
			if ((WM_LBUTTONDOWN == msg.message) || (WM_LBUTTONUP == msg.message) || (WM_LBUTTONDBLCLK == msg.message) || (thread->haveMouseMove >= 48) || !haveMessage || flushMouse) {
				thread->points[0].ticks = thread->haveMouseMove;
				if (kFskErrNone == FskMemPtrNewFromData((sizeof(FskPointAndTicksRecord) * thread->haveMouseMove) + sizeof(UInt32), &thread->points[0].ticks, &thread->mouseMove.lParam)) {
					thread->mouseMove.message = FSK_WM_MOUSEMOVE;

					TranslateMessage(&thread->mouseMove);
					DispatchMessage(&thread->mouseMove);
				}
				thread->haveMouseMove = 0;
			}
		}

		if (!haveMessage)
			break;

		flushMouse = false;

		win = FskWindowGetActive();
		adjustMouseMessages = (NULL != win);

		// @@ Let the KPL frame buffer window handle mounse messages
		adjustMouseMessages = false;
		
		if (adjustMouseMessages) {
			if ((WM_LBUTTONDOWN == msg.message) || (WM_LBUTTONDBLCLK == msg.message) || (WM_LBUTTONUP == msg.message) ||
				(WM_RBUTTONDOWN == msg.message) || (WM_RBUTTONUP == msg.message) || (WM_RBUTTONDBLCLK == msg.message) || (WM_MOUSELEAVE == msg.message)) {
				void *tMsg;

				if (kFskErrNone == FskMemPtrNewFromData(sizeof(msg), &msg, &tMsg)) {
					msg.message = FSK_WM_MOUSE_WITH_TIME;
					msg.lParam = (long)tMsg;
					TranslateMessage(&msg);
					DispatchMessage(&msg);
					continue;
				}
			}

			if (WM_MOUSEMOVE == msg.message) {
				thread->haveMouseMove += 1;
				thread->points[thread->haveMouseMove].pt.x = GET_X_LPARAM(msg.lParam);
				thread->points[thread->haveMouseMove].pt.y = GET_Y_LPARAM(msg.lParam);
				thread->points[thread->haveMouseMove].ticks = msg.time;

				thread->mouseMove = msg;

				ticks = GetTickCount();
				if ((anchorTicks + 80) < ticks) {
					// every 80 ms allow an event through, so mouse moved don't starve callbacks (which can include drawing)
					KplTimeCallbackService(thread);
					anchorTicks = ticks;
				}

				continue;
			}
		}

#if 0	// @@ The "haccel" field isn't available on TARGET_OS_KPL
		if (win && win->haccel) 
			if (TranslateAccelerator(win->hwnd, win->haccel, &msg))
				continue;
#endif
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return 0;
}

HWND KplThreadGetCurrentHWND()
{
	HWND hWnd = NULL;
	KplThread kplThread;
	
	kplThread = (KplThread)KplThreadGetCurrent();
	if (kplThread)
		hWnd = kplThread->window;
		
	return hWnd;
}

void makeThreadWindow(KplThread thread)
{
	WNDCLASS wc;

	// we need a window to be able to receive messages
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = threadWndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInst;
	wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = "kinoma-thread";
	RegisterClass(&wc);
	thread->window = CreateWindow("kinoma-thread", NULL,
		WS_DISABLED, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, hInst, NULL);

	TlsSetValue(gFskThreadTLSSlot, thread);
}

long FAR PASCAL threadWndProc(HWND hwnd, UINT msg, UINT wParam, LONG lParam)
{
	if (gQuitting) {
		return 1;
	}
	else if (gThreadEventMessage == msg) {
		FskEvent event = (FskEvent)lParam;
		FskHandleThreadEvent(event);

		return 1;
	}
	else if (gAsyncSelectMessage == msg) {
		int event;

		event = WSAGETSELECTEVENT(lParam);
		
		win32SocketEvent(wParam, event);
		
		FskThreadNotifySocketData(wParam);

		return 1;
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}

void win32SocketEvent(UINT skt, UINT event)
{
	UInt32 eventType = -1;

	switch(event) {
		case FD_READ: eventType = kKplSocketHostEventRead; break;
		case FD_ACCEPT: eventType = kKplSocketHostEventAccept; break;
		case FD_WRITE: eventType = kKplSocketHostEventWrite; break;
		case FD_CONNECT: eventType = kKplSocketHostEventConnect; break;
		case FD_CLOSE: eventType = kKplSocketHostEventClose; break;
	}

	if (-1 != eventType)		
		FskKplSocketHostEvent((KplSocket)skt, eventType);
}

#if _DEBUG
typedef struct tagTHREADNAME_INFO {
	DWORD dwType; // must be 0x1000
	LPCSTR szName; // pointer to name (in user addr space)
	DWORD dwThreadID; // thread ID (-1=caller thread)
	DWORD dwFlags; // reserved for future use, must be zero
} THREADNAME_INFO;
#endif

unsigned int __stdcall kplThreadProc(void *refcon)
{
	KplThread thread = (KplThread)refcon;

	// Do per-thread initialization
	CoInitialize(NULL);

	timeBeginPeriod(5);

#if _DEBUG
	if (IsDebuggerPresent() && (NULL != thread->name)) {
		THREADNAME_INFO info;
		info.dwType = 0x1000;
		info.szName = thread->name;
		info.dwThreadID = -1;
		info.dwFlags = 0;

		RaiseException(0x406D1388, 0, sizeof(info)/sizeof(DWORD), (DWORD*)&info);		// yes, this really comes from the microsoft documentation
	}
#endif

	makeThreadWindow(thread);

	// Run the client
	(thread->clientProc)(thread->clientRefCon);

	// Do per-thread clean-up
	DestroyWindow(thread->window);
	TlsSetValue(gFskThreadTLSSlot, 0);

	CoUninitialize();

	return 0;
}


