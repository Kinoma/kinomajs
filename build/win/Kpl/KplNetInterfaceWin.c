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
#include <WinSock2.h>
#include <ws2tcpip.h>
#include <MSWSock.h>
#include <iptypes.h>
#include <iphlpapi.h>
#include <windows.h>

#include "KplNetInterface.h"
#include "FskNetUtils.h"
#include "FskList.h"
#include "FskHTTPClient.h"

static KplNetInterfaceChangedCallback gNetInterfaceChangedCallback;
static void *gNetInterfaceChangedCallbackRefcon;

static void installWindowsNetworkInterfaceChangeTracker(void); 
static void removeWindowsNetworkInterfaceChangeTracker(void);
static UINT gNetworkInterfaceChangedMessage;

FskErr KplNetInterfaceInitialize(void)
{
	installWindowsNetworkInterfaceChangeTracker();
	return kFskErrNone;
}

FskErr KplNetInterfaceTerminate(void)
{
	removeWindowsNetworkInterfaceChangeTracker();
	return kFskErrNone;
}

FskErr KplNetInterfaceEnumerate(KplNetInterfaceRecord **interfaceList)
{
	DWORD	bytes, result;
	KplNetInterfaceRecord *nir;
	IP_ADAPTER_INFO	*adapters = NULL, *pAdapter;
	FskErr err = kFskErrNone;

	*interfaceList = NULL;

	result = GetAdaptersInfo(NULL, &bytes);
	if (result == ERROR_NO_DATA)
		return kFskErrNetworkInterfaceError;
	if (( ERROR_SUCCESS != result ) && (ERROR_BUFFER_OVERFLOW != result))
		return kFskErrNetworkInterfaceError;
	FskMemPtrNew(bytes, (FskMemPtr*)&adapters);

	result = GetAdaptersInfo(adapters, &bytes);
	if ( ERROR_SUCCESS != result ) {
		err = kFskErrNetworkInterfaceError;
		goto bail;
	}

	for (pAdapter = adapters ; NULL != pAdapter ; pAdapter = pAdapter->Next){
		const IP_ADDR_STRING* ip = &pAdapter->IpAddressList;
		if (pAdapter->Type == IF_TYPE_PPP)
			continue;
		if (FskStrCompare("0.0.0.0", ip->IpAddress.String) == 0)
			continue;

		FskMemPtrNewClear(sizeof(KplNetInterfaceRecord), (FskMemPtr*)&nir);
		if (nir) {
			UInt32 j;
			nir->name = FskStrDoCopy(pAdapter->AdapterName);
			if (6 == pAdapter->AddressLength) {
				for(j = 0; j < pAdapter->AddressLength; j++)
					nir->MAC[j] = pAdapter->Address[j];
			}
			FskNetStringToIPandPort(ip->IpAddress.String, &nir->ip, NULL);
			FskNetStringToIPandPort(ip->IpMask.String, &nir->netmask, NULL);
			nir->status = 1;
			FskListAppend((FskList *)interfaceList, nir);
		}
	}

	// add loop back I/F to total number of IP addresses  
	FskMemPtrNewClear(sizeof(KplNetInterfaceRecord), (FskMemPtr*)&nir);
	if (nir) {
		int j;
		nir->name = FskStrDoCopy("localhost");
		for(j = 0; j < 6; j++)
			nir->MAC[j] = 0;
		nir->ip = FskNetMakeIP(127,0,0,1);
		nir->netmask = 0xff000000;
		nir->status = 1;
		FskListAppend((FskList *)interfaceList, nir);
	}

bail:
	FskMemPtrDispose(adapters);

	return err;
}

FskErr KplNetInterfaceSetChangedCallback(KplNetInterfaceChangedCallback cb, void *refCon)
{
	gNetInterfaceChangedCallback = cb;
	gNetInterfaceChangedCallbackRefcon = refCon;
	return kFskErrNone;
}

static HANDLE networkInterfaceChangeThread = NULL;
static HWND networkInterfaceTrackerWindow = NULL;
static HANDLE networkInterfaceChangeQuitEvent = NULL;

DWORD WINAPI networkInterfaceChangeThreadFunc( LPVOID lpParam ) 
{
	static OVERLAPPED overlap;
	DWORD ret;

	HANDLE hand = NULL;
	HANDLE watch[2];
	overlap.hEvent = WSACreateEvent();

	while (1) {
		ret = NotifyAddrChange(&hand, &overlap);

		if (ret != NO_ERROR) {
			if (WSAGetLastError() != WSA_IO_PENDING) {
//				printf("NotifyAddrChange error...%d\n", WSAGetLastError());			
				break;
			}
		}
		watch[0] = overlap.hEvent;
		watch[1] = networkInterfaceChangeQuitEvent;

		ret = WaitForMultipleObjects(2, watch, FALSE, INFINITE);
		if (ret == WAIT_OBJECT_0) {
			PostMessage(networkInterfaceTrackerWindow, gNetworkInterfaceChangedMessage, 0, 0);
//			printf("IP Address table changed..\n");
			Sleep(100);
		}
		else
			break;
	}
	CloseHandle(overlap.hEvent);
	CloseHandle(networkInterfaceChangeQuitEvent);
	DestroyWindow(networkInterfaceTrackerWindow);
	networkInterfaceChangeQuitEvent = NULL;
	networkInterfaceTrackerWindow = NULL;
	networkInterfaceChangeThread = NULL;

	return 0;
}

long FAR PASCAL interfaceTrackerProc(HWND hwnd, UINT msg, UINT wParam, LONG lParam)
{
	if (WM_SETTINGCHANGE == msg) {
		LPCTSTR foo = (LPCTSTR)lParam;
		if (0 == FskStrCompare("Software\\Microsoft\\Internet Explorer", (const char *)foo)) {
			FskHTTPSyncSystemProxy();
		}
	}
	if (gNetworkInterfaceChangedMessage == msg) {
		gNetInterfaceChangedCallback(gNetInterfaceChangedCallbackRefcon);
		return 1;
	}
	return DefWindowProc(hwnd, msg, wParam, lParam);
}

void makeInterfaceTrackerWindow()
{
	WNDCLASS wc;

	// we need a window to be able to receive messages
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = interfaceTrackerProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = NULL;
	wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = "interfaceTrackerWindow";
	RegisterClass(&wc);
	networkInterfaceTrackerWindow = CreateWindow("interfaceTrackerWindow", NULL,
		WS_DISABLED, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, NULL, NULL);
}

void installWindowsNetworkInterfaceChangeTracker(void) 
{ 
    DWORD dwThreadId, dwThrdParam = 1; 

	networkInterfaceChangeQuitEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	gNetworkInterfaceChangedMessage = RegisterWindowMessage("FskNetUtilsInterfaceChanged");
	makeInterfaceTrackerWindow();

	networkInterfaceChangeThread = CreateThread( 
        NULL,                        // default security attributes 
        0,                           // use default stack size  
        networkInterfaceChangeThreadFunc,                  // thread function 
        &dwThrdParam,                // argument to thread function 
        0,                           // use default creation flags 
        &dwThreadId);                // returns the thread identifier 

	if (networkInterfaceChangeThread == NULL) 
	{
		CloseHandle(networkInterfaceChangeQuitEvent);
		networkInterfaceChangeQuitEvent = NULL;
		DestroyWindow(networkInterfaceTrackerWindow);
	}
}

void removeWindowsNetworkInterfaceChangeTracker(void)
{
	CloseHandle(networkInterfaceChangeThread);
	SetEvent(networkInterfaceChangeQuitEvent);
}
