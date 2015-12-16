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
#define __FSKTHREAD_PRIV__
/*
*/
#define __FSKNETUTILS_PRIV__
#include "Fsk.h"


#ifndef IGNORE_NETINTERFACE
#define IGNORE_NETINTERFACE		1
#endif /* IGNORE_NETINTERFACE */

#if IGNORE_NETINTERFACE
	#if TARGET_OS_ANDROID
		#define NUM_IGNORE_NET	2
		char *ignoreInterfaces[NUM_IGNORE_NET] = { "usb0", "p2p0" };
	#elif TARGET_OS_LINUX || TARGET_OS_MAC
		#define NUM_IGNORE_NET	5
		char *ignoreInterfaces[NUM_IGNORE_NET] = { "usb0", "vnic0", "vnic1", "vboxnet0", "p2p0" };
	#endif
#endif

#if TARGET_OS_WIN32
	#include <WinSock2.h>
	#include <ws2tcpip.h>

	#include <MSWSock.h>

	#include <Windows.h>
	#include <stdio.h>
	#include <stdlib.h>
	#include <string.h>
	#include <time.h>

	#include <fcntl.h>
	#include <errno.h>
	#include <sys/types.h>
#elif TARGET_OS_LINUX || TARGET_OS_MAC
	#include <stdio.h>
	#include <stdlib.h>
	#include <string.h>
	#include <time.h>
	#include <unistd.h>
	#include <fcntl.h>
	#include <errno.h>

	#include <sys/types.h>
	#include <sys/ioctl.h>
	#include <sys/socket.h>

	#include <netinet/in.h>
	#include <netdb.h>
	#include <net/if.h>
	#include <arpa/inet.h>

	#include <dirent.h>

	#define closesocket		close
#if TARGET_OS_LINUX
	#include <resolv.h>
	#include <net/if_arp.h>
#endif
#if TARGET_OS_MAC
#if !TARGET_OS_IPHONE
	#include <net/if_types.h>
	#include <net/if_dl.h>
#endif /* TARGET_OS_IPHONE */
	#include <CoreFoundation/CoreFoundation.h>
	#include <SystemConfiguration/SystemConfiguration.h>
	#include <ifaddrs.h>
	#include <net/if.h>
#endif /* TARGET_OS_IPHONE */

#endif /* TARGET_OS_MAC */

#if TARGET_OS_WIN32
	#include "FskHTTPClient.h"
#endif

#include "FskPlatformImplementation.h"
#include "FskNetUtils.h"
#include "FskList.h"

#include "FskThread.h"
#include "FskNetInterface.h"

#if TARGET_OS_KPL
#include "KplNetInterface.h"
#endif /* TARGET_OS_KPL */

static FskListMutex	interfaceChangeCBList = NULL;
FskListMutex	gNetworkInterfaceList = NULL;
static FskErr sFskNetInterfaceEnumerate(FskNetInterfaceRecord **interfaceList);
#if TARGET_OS_WIN32 || TARGET_OS_LINUX || TARGET_OS_MAC || TARGET_OS_KPL
	static void sFskHandleNetworkInterfacesChanged(void);
#endif /* TARGET_OS_WIN32 || TARGET_OS_LINUX || TARGET_OS_MAC || TARGET_OS_KPL */

#if TARGET_OS_KPL
static void KplNetInterfaceChanged(void *refCon);
#endif /* TARGET_OS_KPL */

// ---------------------------------------------------------------------
#if TARGET_OS_WIN32
	UINT gNetworkInterfaceChangedMessage;
#endif /* TARGET_OS_WIN32 */

#if SUPPORT_INSTRUMENTATION
static Boolean doFormatMessageFskNetInterfaceNotifier(FskInstrumentedType dispatch, UInt32 msg, void *msgData, char *buffer, UInt32 bufferSize);

	static FskInstrumentedTypeRecord gNetInterfaceNotifierTypeInstrumentation = {
		NULL,
		sizeof(FskInstrumentedTypeRecord),
		"netinterfacenotifier",
		FskInstrumentationOffset(FskNetInterfaceNotifierRec),
		NULL,
		0,
		NULL,
		doFormatMessageFskNetInterfaceNotifier
	};
#endif /* SUPPORT_INSTRUMENTATION */


// ---------------------------------------------------------------------
#if TARGET_OS_WIN32
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
				printf("NotifyAddrChange error...%d\n", WSAGetLastError());
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
		sFskHandleNetworkInterfacesChanged();
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

   // Check the return value for success.

	if (networkInterfaceChangeThread == NULL)
	{
      // deal with failure
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

#elif TARGET_OS_IPHONE
static SCNetworkReachabilityRef reachability = nil;

static void ReachabilityCallback(SCNetworkReachabilityRef target, SCNetworkReachabilityFlags flags, void* info)
{
	sFskHandleNetworkInterfacesChanged();
}

static void installIOSNetworkInterfaceChangeTracker()
{
	struct sockaddr_in localWifiAddress;

	bzero(&localWifiAddress, sizeof(localWifiAddress));
	localWifiAddress.sin_len = sizeof(localWifiAddress);
	localWifiAddress.sin_family = AF_INET;
	// IN_LINKLOCALNETNUM is defined in <netinet/in.h> as 169.254.0.0.
	localWifiAddress.sin_addr.s_addr = htonl(IN_LINKLOCALNETNUM);
	reachability = SCNetworkReachabilityCreateWithAddress(kCFAllocatorDefault, (const struct sockaddr *)&localWifiAddress);
	if (SCNetworkReachabilitySetCallback(reachability, ReachabilityCallback, NULL))
		(void)SCNetworkReachabilityScheduleWithRunLoop(reachability, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
}

static void removeIOSNetworkInterfaceChangeTracker()
{
	if (reachability != nil) {
		(void)SCNetworkReachabilityUnscheduleFromRunLoop(reachability, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
		CFRelease(reachability);
		reachability = nil;
	}
}
#elif TARGET_OS_MAC

void MacInterfacesChangedCallback(SCDynamicStoreRef store, CFArrayRef changedKeys, void *info);

static OSStatus CreateIPAddressListChangeCallbackSCF(SCDynamicStoreCallBack callback,
	void *contextPtr, SCDynamicStoreRef *storeRef, CFRunLoopSourceRef *sourceRef)
    // Create a SCF dynamic store reference and a
    // corresponding CFRunLoop source.  If you add the
    // run loop source to your run loop then the supplied
    // callback function will be called when local IP
    // address list changes.
{
    OSStatus                err = 0;
    SCDynamicStoreContext   context = {0, NULL, NULL, NULL, NULL};
    SCDynamicStoreRef       ref;
    CFStringRef             pattern;
    CFArrayRef              patternList;
    CFRunLoopSourceRef      rls;

    ref = NULL;
    pattern = NULL;
    patternList = NULL;
    rls = NULL;

    // Create a connection to the dynamic store, then create
    // a search pattern that finds all IPv4 entities.
    // The pattern is "State:/Network/Service/[^/]+/IPv4".

    context.info = contextPtr;
    ref = SCDynamicStoreCreate( NULL, CFSTR("KinomaIPAddressChangeCB-mk13"),
					callback, &context);
    if (ref != NULL) {
        pattern = SCDynamicStoreKeyCreateNetworkServiceEntity(NULL, kSCDynamicStoreDomainState,
					kSCCompAnyRegex, kSCEntNetIPv4);
		if (pattern == NULL)
			err = kFskErrOperationFailed;
    }

    // Create a pattern list containing just one pattern,
    // then tell SCF that we want to watch changes in keys
    // that match that pattern list, then create our run loop
    // source.

    if (err == kFskErrNone) {
        patternList = CFArrayCreate(NULL,  (const void **) &pattern, 1,
               &kCFTypeArrayCallBacks);
		if (patternList == NULL)
			err = kFskErrOperationFailed;
    }
    if (err == noErr) {
        if (!SCDynamicStoreSetNotificationKeys(ref, NULL,patternList))
			err = kFskErrOperationFailed;
    }
    if (err == noErr) {
        rls = SCDynamicStoreCreateRunLoopSource(NULL, ref, 0);
		if (rls == NULL)
			err = kFskErrOperationFailed;
    }

    // Clean up.

	if (pattern) CFRelease(pattern);
	if (patternList) CFRelease(patternList);
    if (err != noErr) {
        if (ref) CFRelease(ref);
        ref = NULL;
    }
    *storeRef = ref;
    *sourceRef = rls;

    return err;
}

static SCDynamicStoreRef storeRef;
static CFRunLoopSourceRef interfaceChangeRunLoopSourceRef = NULL;

void installMacNetworkInterfaceChangeTracker() {
    CreateIPAddressListChangeCallbackSCF(MacInterfacesChangedCallback,
		NULL, &storeRef, &interfaceChangeRunLoopSourceRef);
	CFRunLoopAddSource(CFRunLoopGetCurrent(), interfaceChangeRunLoopSourceRef, kCFRunLoopCommonModes);
}

void removeMacNetworkInterfaceChangeTracker(void) {
	if (NULL == interfaceChangeRunLoopSourceRef)
		CFRunLoopRemoveSource(CFRunLoopGetCurrent(), interfaceChangeRunLoopSourceRef, kCFRunLoopCommonModes);
	if (storeRef) CFRelease(storeRef);
	if (interfaceChangeRunLoopSourceRef) CFRelease(interfaceChangeRunLoopSourceRef);
}

void MacInterfacesChangedCallback(SCDynamicStoreRef store, CFArrayRef changedKeys, void *info) {
	FskInstrumentedTypePrintfDebug(&gNetInterfaceNotifierTypeInstrumentation,"MacInterfacesChangedCallback");
	sFskHandleNetworkInterfacesChanged();
}
#endif


#if TARGET_OS_WIN32
// ---------------------------------------------------------------------
static FskErr sFskNetInterfaceEnumerate(FskNetInterfaceRecord **interfaceList)
{
	DWORD	bytes, result;
	FskNetInterfaceRecord *nir;
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
	if ( ERROR_SUCCESS != result )
        BAIL(kFskErrNetworkInterfaceError);

	for (pAdapter = adapters ; NULL != pAdapter ; pAdapter = pAdapter->Next){
		const IP_ADDR_STRING* ip = &pAdapter->IpAddressList;
		// NOTE: there may be more than one IP per interface - this does not handle that case
		FskInstrumentedTypePrintfDebug(&gNetInterfaceNotifierTypeInstrumentation,"Network Adapter %s", ip->IpAddress.String);
		// check for ppp, 0.0.0.0, etc.
		if (pAdapter->Type == IF_TYPE_PPP)
				continue;
		if (FskStrCompare("0.0.0.0", ip->IpAddress.String) == 0)
				continue;

		FskMemPtrNewClear(sizeof(FskNetInterfaceRecord), (FskMemPtr*)&nir);
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
	FskMemPtrNewClear(sizeof(FskNetInterfaceRecord), (FskMemPtr*)&nir);
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
#elif TARGET_OS_LINUX || TARGET_OS_MAC

static FskErr sFskNetInterfaceEnumerate(FskNetInterfaceRecord **interfaceList)
{
	FskErr	err = kFskErrNone;
	FskNetInterfaceRecord *nir;
	int fd;
	struct ifreq  ifr;
	struct sockaddr_in *sa;
#if TARGET_OS_MAC	// BSD
	struct ifreq ibuf[32];
#endif /* TARGET_OS_MAC */
#if TARGET_OS_LINUX
#if TARGET_OS_ANDROID
	DIR *d;
	struct dirent *dir;
	*interfaceList = NULL;

	fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
	if (fd < 0) goto skip;

	FskInstrumentedTypePrintfDebug(&gNetInterfaceNotifierTypeInstrumentation, "Enumerate Interfaces:");

	d = opendir("/sys/class/net");
	if (0 == d)  {
		BAIL(kFskErrNetworkInterfaceError);
	}

	while ((dir = readdir(d))) {
#if IGNORE_NETINTERFACE
		Boolean ignore = false;
		int i;
#endif
		char *ifname;
		unsigned theIP = 0, theNetmask = 0, theStatus = 0;

		if (dir->d_name[0] == '.')
			continue;

		ifname = dir->d_name;

		FskInstrumentedTypePrintfDebug(&gNetInterfaceNotifierTypeInstrumentation,"%s: ", ifname);

#if IGNORE_NETINTERFACE
		i = NUM_IGNORE_NET;
		while (i) {
			if (FskStrCompare(ignoreInterfaces[i-1], ifname) == 0) {
				FskInstrumentedTypePrintfDebug(&gNetInterfaceNotifierTypeInstrumentation, "IGNORED");
				ignore = true;
			}
			i--;
		}
		if (ignore)
			continue;
#endif /* IGNORE_NETINTERFACE */

		memset(&ifr, 0,  sizeof(struct ifreq));
		strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
		ifr.ifr_name[IFNAMSIZ - 1] = 0;

		if (ioctl(fd, SIOCGIFADDR, &ifr) >= 0) {
			sa = (struct sockaddr_in*)(void*)&(ifr.ifr_addr);
			theIP = ntohl( sa->sin_addr.s_addr);
		}

		if (ioctl(fd, SIOCGIFNETMASK, &ifr) >= 0) {
			sa = (struct sockaddr_in*)(void*)&(ifr.ifr_addr);
			theNetmask = ntohl( sa->sin_addr.s_addr);
		}

		if (ioctl(fd, SIOCGIFFLAGS, &ifr) >= 0) {
			if (ifr.ifr_flags & 1)
				theStatus = 1;
		}

		if (theIP == 0)
			theStatus = 0;

		FskInstrumentedTypePrintfDebug(&gNetInterfaceNotifierTypeInstrumentation,"IP: %x, Netmask: %x [%s]", theIP, theNetmask, theStatus ? "UP " : "DOWN ");

		if (ioctl(fd, SIOCGIFHWADDR, &ifr) >= 0) {
			FskInstrumentedTypePrintfDebug(&gNetInterfaceNotifierTypeInstrumentation, "Got HWADDR ");

			if (ifr.ifr_hwaddr.sa_family == ARPHRD_ETHER) {
				FskInstrumentedTypePrintfDebug(&gNetInterfaceNotifierTypeInstrumentation, " ETHER");
				FskMemPtrNewClear(sizeof(FskNetInterfaceRecord), (FskMemPtr*)(void*)&nir);
				if (!nir) {
					closedir(d);
					BAIL(kFskErrMemFull);
				}
				FskMemCopy(nir->MAC, ifr.ifr_hwaddr.sa_data, 6);

				nir->name = FskStrDoCopy(ifname);
				nir->ip = theIP;
				nir->netmask = theNetmask;
				nir->status = theStatus;

				FskListAppend((FskList*)interfaceList, nir);
			}
			else {
				FskInstrumentedTypePrintfDebug(&gNetInterfaceNotifierTypeInstrumentation, " Family not ETHER Huh?");
			}
		}
	}
	closedir(d);
skip:

#else /* !TARGET_OS_ANDROID */
	FILE *fp;
	char buf[256];
#if IGNORE_NETINTERFACE
	Boolean ignore = false;
	int i;
#endif

	*interfaceList = NULL;
	fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
	if (fd < 0) goto skip;

	fp = fopen("/proc/net/dev", "r");
	if (!fp) {
		BAIL(kFskErrNetworkInterfaceError);
	}
	// ignore two lines
	fgets(buf, sizeof(buf), fp);
	fgets(buf, sizeof(buf), fp);
	while (fgets(buf, sizeof(buf), fp)) {
		char *ifname = strtok(buf, " :");
		if (!ifname) continue;

		FskInstrumentedTypePrintfDebug(&gNetInterfaceNotifierTypeInstrumentation,"%s: ", ifname);

#if IGNORE_NETINTERFACE
		i = NUM_IGNORE_NET;
		while (i) {
			if (FskStrCompare(ignoreInterfaces[i-1], ifname) == 0) {
				FskInstrumentedTypePrintfDebug(&gNetInterfaceNotifierTypeInstrumentation, "IGNORED");
				ignore = true;
			}
			i--;
		}
		if (ignore)
			continue;
#endif /* IGNORE_NETINTERFACE */
		strcpy(ifr.ifr_name, ifname);
		if ((ioctl(fd, SIOCGIFHWADDR, &ifr) != -1) && (ifr.ifr_hwaddr.sa_family == ARPHRD_ETHER)) {
			FskMemPtrNewClear(sizeof(FskNetInterfaceRecord), (FskMemPtr *)&nir);
			if (!nir) {
				err = kFskErrMemFull;
				fclose(fp);
				goto bail;
			}
			FskMemCopy(nir->MAC, ifr.ifr_hwaddr.sa_data, 6);
			nir->name = FskStrDoCopy(ifname);

			nir->ip = 0;
			if (ioctl(fd, SIOCGIFADDR, &ifr) != -1) {
				sa = (struct sockaddr_in*)(void*)&(ifr.ifr_addr);
				nir->ip = ntohl( sa->sin_addr.s_addr);
			}

			if (ioctl(fd, SIOCGIFNETMASK, &ifr) != -1) {
				sa = (struct sockaddr_in*)(void*)&(ifr.ifr_addr);
				nir->netmask = ntohl( sa->sin_addr.s_addr);
			}

			if (ioctl(fd, SIOCGIFFLAGS, &ifr) != -1) {
				if (ifr.ifr_flags & IFF_UP) {
					nir->status = 1;
				} else {
					nir->status = 0;
				}
			}

			if (nir->ip == 0)
				nir->status = 0;

			FskInstrumentedTypePrintfDebug(&gNetInterfaceNotifierTypeInstrumentation,"IP: %x, Netmask: %x [%s] HWADDR [%02x:%02x:%02x:%02x:%02x:%02x]", theIP, theNetmask, theStatus ? "UP " : "DOWN ", nir->MAC[0], nir->MAC[1], nir->MAC[2], nir->MAC[3], nir->MAC[4], nir->MAC[5]);
			FskListAppend((FskList*)interfaceList, nir);
		}
		else {
			FskInstrumentedTypePrintfDebug(&gNetInterfaceNotifierTypeInstrumentation, " Family not ETHER or no HWADDR");
		}
	}
	fclose(fp);
skip:
#endif /* !TARGET_OS_ANDROID */

#elif TARGET_OS_MAC && !TARGET_OS_IPHONE
	struct ifreq *ifrp, *ifend;
	unsigned int r;
	struct ifconf ifc;
#if IGNORE_NETINTERFACE
	Boolean ignore = false;
	int i;
#endif

	*interfaceList = NULL;
	fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
	ifc.ifc_len = sizeof(ibuf);
	ifc.ifc_buf = (caddr_t)ibuf;
	if (ioctl(fd, SIOCGIFCONF, &ifc) == -1 ||
		ifc.ifc_len < (int)sizeof(struct ifreq)) {
		BAIL(kFskErrNetworkInterfaceError);
	}

	ifrp = ibuf;
	ifend = (struct ifreq*)((char*)ibuf + ifc.ifc_len);
	while (ifrp < ifend) {
		if (ifrp->ifr_addr.sa_family == AF_LINK &&
			((struct sockaddr_dl *)&ifrp->ifr_addr)->sdl_type == IFT_ETHER) {
			err = FskMemPtrNewClear(sizeof(FskNetInterfaceRecord), (FskMemPtr *)&nir);
			BAIL_IF_ERR(err);
			nir->name = FskStrDoCopy(ifrp->ifr_name);
			FskMemCopy((char*)nir->MAC, (char*)LLADDR((struct sockaddr_dl *)&ifrp->ifr_addr), 6);

			FskInstrumentedTypePrintfDebug(&gNetInterfaceNotifierTypeInstrumentation,"%s: ", nir->name);

#if IGNORE_NETINTERFACE
			i = NUM_IGNORE_NET;
			ignore = false;
			while (i) {
				if (FskStrCompare(ignoreInterfaces[i-1], nir->name) == 0) {
					FskInstrumentedTypePrintfDebug(&gNetInterfaceNotifierTypeInstrumentation, "IGNORED");
					ignore = true;
				}
				i--;
			}
			if (ignore) {
				FskMemPtrDispose(nir->name);
				FskMemPtrDisposeAt(&nir);
				goto nextOne;
			}
#endif /* IGNORE_NETINTERFACE */

			strcpy(ifr.ifr_name, nir->name);		//@@ bounds check needed?
			if (ioctl(fd, SIOCGIFADDR, &ifr) != -1) {
				sa = (struct sockaddr_in*)(void*)&(ifr.ifr_addr);
				nir->ip = ntohl( sa->sin_addr.s_addr);
			}
			if (nir->ip) {
				if (ioctl(fd, SIOCGIFNETMASK, &ifr) != -1) {
					sa = (struct sockaddr_in*)(void*)&(ifr.ifr_addr);
					nir->netmask = ntohl( sa->sin_addr.s_addr);
				}

				if (ioctl(fd, SIOCGIFFLAGS, &ifr) != -1) {
					if (ifr.ifr_flags & IFF_UP)
						nir->status = 1;
				}

				FskInstrumentedTypePrintfDebug(&gNetInterfaceNotifierTypeInstrumentation,"IP: %x, Netmask: %x [%s] HWADDR [%02x:%02x:%02x:%02x:%02x:%02x]", nir->ip, nir->netmask, nir->status ? "UP " : "DOWN ", nir->MAC[0], nir->MAC[1], nir->MAC[2], nir->MAC[3], nir->MAC[4], nir->MAC[5]);

				FskListAppend(interfaceList, nir);
			}
			else {
				FskMemPtrDispose(nir->name);
				FskMemPtrDisposeAt(&nir);
			}
		}

nextOne:
		r = ifrp->ifr_addr.sa_len + sizeof(ifrp->ifr_name);
		if (r < sizeof(*ifrp))
			r = sizeof(*ifrp);
		ifrp = (struct ifreq*)((char*)ifrp+r);
	}

#elif TARGET_OS_IPHONE
	struct ifaddrs *iflist;
	if (getifaddrs(&iflist) != 0)
		return kFskErrNetworkInterfaceError;
	*interfaceList = NULL;
	for (struct ifaddrs *ifa = iflist; ifa != NULL; ifa = ifa->ifa_next) {
		if (ifa->ifa_name == NULL || ifa->ifa_addr == NULL || ifa->ifa_netmask == NULL || ((struct sockaddr_in*)(void*)ifa->ifa_addr)->sin_addr.s_addr == 0)
			continue;
		for (FskNetInterface ni = *interfaceList; ni != NULL; ni = ni->next) {
			if (FskStrCompare(ni->name, ifa->ifa_name) == 0)
				goto next;
		}
		if ((err = FskMemPtrNewClear(sizeof(FskNetInterfaceRecord), (FskMemPtr *)&nir)) != kFskErrNone)
			return err;
		nir->name = FskStrDoCopy(ifa->ifa_name);
		nir->ip = ntohl(((struct sockaddr_in*)(void*)ifa->ifa_addr)->sin_addr.s_addr);
		nir->netmask = ntohl(((struct sockaddr_in*)(void*)ifa->ifa_netmask)->sin_addr.s_addr);
		nir->status = ifa->ifa_flags & IFF_UP ? 1 : 0;
		FskListAppend((FskList *)interfaceList, nir);
	next:;
	}
	freeifaddrs(iflist);
	return kFskErrNone;
#endif /* TARGET_OS_IPHONE */

	// add loop back I/F to total number of IP addresses
	err = FskMemPtrNewClear(sizeof(FskNetInterfaceRecord), (FskMemPtr*)(void*)&nir);
	if (kFskErrNone == err) {
		nir->name = FskStrDoCopy("localhost");
		nir->ip = FskNetMakeIP(127,0,0,1);
		nir->netmask = 0xff000000;
		nir->status = 1;
		FskListAppend((FskList*)interfaceList, nir);
	}

bail:
	if (fd >= 0)		// coverity 10587
		close (fd);
	return err;
}

#elif TARGET_OS_KPL

static FskErr sFskNetInterfaceEnumerate(FskNetInterfaceRecord **interfaceList)
{
	FskErr err;
	KplNetInterfaceRecord *kplInterfaceList;

	*interfaceList = NULL;

	err = KplNetInterfaceEnumerate(&kplInterfaceList);
	BAIL_IF_ERR(err);

	while (kplInterfaceList) {
		FskNetInterfaceRecord *nir;
		KplNetInterfaceRecord *next = kplInterfaceList->next;

		err = FskMemPtrNew(sizeof(FskNetInterfaceRecord), (FskMemPtr*)&nir);
		BAIL_IF_ERR(err);

		nir->name = FskStrDoCopy(kplInterfaceList->name);
		nir->ip = kplInterfaceList->ip;
		FskMemMove(nir->MAC, kplInterfaceList->MAC, sizeof(nir->MAC));
		nir->status = kplInterfaceList->status;
		nir->netmask = kplInterfaceList->netmask;

		FskListAppend((FskList*)interfaceList, nir);

		FskMemPtrDispose(kplInterfaceList->name);
		FskMemPtrDispose(kplInterfaceList);

		kplInterfaceList = next;
	}

bail:
	return err;
}

#endif /* TARGET_OS_KPL */

// ---------------------------------------------------------------------
UInt32 FskNetInterfaceEnumerate() {
	return FskListMutexCount(gNetworkInterfaceList);
}

#if 0
void printInterfaces() {
	FskNetInterfaceRecord *nir;
	char str[32], mac[32];
	nir = gNetworkInterfaces;
	while (nir) {
		FskNetIPandPortToString(nir->ip, 0, str);
		FskStrNumToHex(nir->MAC[0], mac, 2);
		mac[2] = ':';
		FskStrNumToHex(nir->MAC[1], &mac[3], 2);
		mac[5] = ':';
		FskStrNumToHex(nir->MAC[2], &mac[6], 2);
		mac[8] = ':';
		FskStrNumToHex(nir->MAC[3], &mac[9], 2);
		mac[11] = ':';
		FskStrNumToHex(nir->MAC[4], &mac[12], 2);
		mac[14] = ':';
		FskStrNumToHex(nir->MAC[5], &mac[15], 2);
		mac[17] = '\0';

		FskInstrumentedTypePrintfDebug(&gNetInterfaceNotifierTypeInstrumentation, "IFC: %s -- %s -- %s", nir->name, str, mac);
		nir = nir->next;
	}
}
#endif // 0

#if TARGET_OS_WIN32 || TARGET_OS_LINUX || TARGET_OS_MAC || TARGET_OS_KPL

// ---------------------------------------------------------------------
static int sCompareInterfaces(FskNetInterfaceRecord *a, FskNetInterfaceRecord *b)
{
	int	ret = -1;

	if (a->status != b->status)
		return ret;

	if (a->ip == b->ip) {
		if (FskMemCompare(a->MAC, b->MAC, 6) == 0) {
			if (FskStrCompare(a->name, b->name) == 0) {
				ret = 0;
			}
		}
	}
	return ret;
}

#endif /* TARGET_OS_WIN32 || TARGET_OS_LINUX || TARGET_OS_MAC || TARGET_OS_KPL */

// ---------------------------------------------------------------------
static void sDisposeInterfaceList(FskNetInterfaceRecord **listHead)
{
	FskNetInterfaceRecord	*list = *listHead;

	while (list) {
		FskNetInterfaceRecord	*next = list->next;
		FskMemPtrDispose(list->name);
		FskMemPtrDispose(list);
		list = next;
	}

	*listHead = NULL;
}

// ---------------------------------------------------------------------
FskErr FskNetCleanupNetworkEnumeration()
{
	FskMutexAcquire(gNetworkInterfaceList->mutex);
	sDisposeInterfaceList((FskNetInterfaceRecord**)(void*)&gNetworkInterfaceList->list);
	FskListMutexDispose(gNetworkInterfaceList);
	gNetworkInterfaceList = NULL;
	return kFskErrNone;
}

// ---------------------------------------------------------------------
#if TARGET_OS_WIN32 || TARGET_OS_LINUX || TARGET_OS_MAC || TARGET_OS_KPL

static Boolean sHaveNetworkInterfacesChanged()
{
	FskErr	err;
	Boolean retVal = false;
	FskNetInterfaceRecord *existing, *newer, *check;

	FskMutexAcquire(gNetworkInterfaceList->mutex);
	err = sFskNetInterfaceEnumerate(&newer);
	if (err) {
		retVal = true;
		goto done;
	}

	if (FskListCount(newer) != FskListCount(gNetworkInterfaceList->list)) {
		retVal = true;
		goto done;
	}

	check = newer;
	while (check) {
		Boolean gotOne = false;
		existing = (FskNetInterfaceRecord *)gNetworkInterfaceList->list;
		while (existing) {
			if (sCompareInterfaces(check, existing) == 0) {
				gotOne = true;
				break;
			}
			existing = existing->next;
		}
		if (!gotOne) {
			retVal = true;
			goto done;
		}
		check = check->next;
	}

	retVal = false;

done:
	FskMutexRelease(gNetworkInterfaceList->mutex);
	sDisposeInterfaceList(&newer);
	return retVal;
}

#endif /* TARGET_OS_WIN32 || TARGET_OS_LINUX || TARGET_OS_MAC || TARGET_OS_KPL */

// ---------------------------------------------------------------------
static FskNetInterfaceRecord *sFskNetInterfaceFindByName(char *name, FskNetInterfaceRecord *list)
{
	FskNetInterfaceRecord *cur = list;

	while (cur) {
		if (FskStrCompare(cur->name, name) == 0)
			return cur;
		cur = cur->next;
	}
	return NULL;
}

FskErr FskNetInterfaceDescriptionDispose(FskNetInterfaceRecord *nir) {
	if (nir) {
		FskMemPtrDispose(nir->name);
		FskMemPtrDispose(nir);
	}
	return kFskErrNone;
}

// ---------------------------------------------------------------------
FskNetInterfaceRecord *FskNetInterfaceFindByName(char *name)
{
	FskNetInterfaceRecord	*ret = NULL, *found;

	FskInstrumentedTypePrintfDebug(&gNetInterfaceNotifierTypeInstrumentation, "FskNetInterfaceFindByName: %s", (name == NULL) ? "NULL" : name);
	FskMutexAcquire(gNetworkInterfaceList->mutex);
	found = (FskNetInterfaceRecord *)sFskNetInterfaceFindByName(name, (FskNetInterfaceRecord  *)gNetworkInterfaceList->list);
	if (found) {
		if (kFskErrNone == FskMemPtrNewFromData(sizeof(FskNetInterfaceRecord), found, &ret)) {
			ret->next = NULL;
			ret->name = FskStrDoCopy(found->name);
		}
	}
	FskMutexRelease(gNetworkInterfaceList->mutex);

	return ret;
}

// ---------------------------------------------------------------------
void doNetIfcCallback(void *a, void *b, void *c, void *d) {
	FskNetInterfaceChangedCallback callback = (FskNetInterfaceChangedCallback)a;
	FskNetInterfaceRecord *iface = (FskNetInterfaceRecord *)b;
	UInt32 status = (UInt32)c;
	void *param = (void*)d;

	callback(iface, status, param);
	FskInstrumentedTypePrintfDebug(&gNetInterfaceNotifierTypeInstrumentation, "doNetIfcCallback - about to dispose description");
	FskNetInterfaceDescriptionDispose(iface);
	FskInstrumentedTypePrintfDebug(&gNetInterfaceNotifierTypeInstrumentation, "doNetIfcCallback - after disposing description");
}

// ---------------------------------------------------------------------
void sNotifyInterfaceNotifiers(FskNetInterfaceRecord *iface, UInt32 status) {
	FskNetInterfaceNotifier callback = NULL, next;
	FskThread thread = FskThreadGetCurrent();

	callback = (FskNetInterfaceNotifier)FskListMutexGetNext(interfaceChangeCBList, NULL);
	while (callback) {
		next = (FskNetInterfaceNotifier)FskListMutexGetNext(interfaceChangeCBList, callback);
#if 0 && SUPPORT_INSTRUMENTATION
		if (FskInstrumentedItemHasListeners(callback)) {
			FskNetInterfaceRecord *iface = (FskNetInterfaceRecord *)FskNetInterfaceFindByName(ifcName);
			FskInterfaceInstrData data;
			data.notf = callback;
			data.ifc = iface;
			data.status = status;
			FskInstrumentedItemSendMessage(callback, kFskNetInstrMsgInterfaceNotify, &data);
		}
#endif /* SUPPORT_INSTRUMENTATION */
		if (callback->thread != thread) {
			FskNetInterfaceRecord *interFace;
			if (kFskErrNone == FskMemPtrNewFromData(sizeof(FskNetInterfaceRecord), iface, &interFace)) {
				interFace->next = NULL;
				interFace->name = FskStrDoCopy(iface->name);

				// POST notify
				FskInstrumentedTypePrintfDebug(&gNetInterfaceNotifierTypeInstrumentation, " - posting callback to thread %s for interface %s", callback->thread->name, iface->name);
				FskThreadPostCallback(callback->thread, doNetIfcCallback, (void*)callback->callback, (void*)interFace, (void*)status, (void*)callback->param);
			}
		}
		else {
			FskInstrumentedTypePrintfDebug(&gNetInterfaceNotifierTypeInstrumentation, "callback->thread %s - call", callback->thread->name);
			(callback->callback)(iface, status, callback->param);
		}
		callback = next;
	}
}

#if TARGET_OS_WIN32 || TARGET_OS_LINUX || TARGET_OS_MAC || TARGET_OS_KPL

// ---------------------------------------------------------------------
static Boolean sNotifyNetworkInterfacesChanged()
{
	FskErr	err;
	Boolean retVal = false;
	FskNetInterfaceRecord *existing, *newer, *check, *old;

#if SUPPORT_INSTRUMENTATION
	FskThread thread = FskThreadGetCurrent();
	FskInstrumentedTypePrintfDebug(&gNetInterfaceNotifierTypeInstrumentation, "NotifyNetworkInterfacesChanged -- thread is: %p - %s", thread, thread->name);
#endif /* SUPPORT_INSTRUMENTATION */

	// If we fail to enumerate interfaces, assume there are no interfaces available now and
	// notify our clients that their interfaces have been removed.
	err = sFskNetInterfaceEnumerate(&newer);
	if (err) {
		FskInstrumentedTypePrintfDebug(&gNetInterfaceNotifierTypeInstrumentation, "NotifyNetworkInterfacesChanged -- can't enumerate, Down the interfaces");
		FskMutexAcquire(gNetworkInterfaceList->mutex);
		existing = (FskNetInterfaceRecord *)gNetworkInterfaceList->list;
		while (existing) {
			sNotifyInterfaceNotifiers(existing, kFskNetInterfaceStatusRemoved);
			existing = existing->next;
		}
		sDisposeInterfaceList((FskNetInterfaceRecord**)(void*)&gNetworkInterfaceList->list);
		FskMutexRelease(gNetworkInterfaceList->mutex);
		return true;
	}

	FskMutexAcquire(gNetworkInterfaceList->mutex);
	old = (FskNetInterfaceRecord *)gNetworkInterfaceList->list;
	existing = (FskNetInterfaceRecord *)gNetworkInterfaceList->list;
	gNetworkInterfaceList->list = newer;
	while (existing) {
		FskInstrumentedTypePrintfDebug(&gNetInterfaceNotifierTypeInstrumentation, "NotifyNetworkInterfacesChanged -- looking for %s in the newer list", existing->name);
		check = sFskNetInterfaceFindByName(existing->name, newer);

		if (check) {
			FskInstrumentedTypePrintfDebug(&gNetInterfaceNotifierTypeInstrumentation, "NotifyNetworkInterfacesChanged -- found %s in the newer list", existing->name);
			if (sCompareInterfaces(check, existing) != 0) {
				FskInstrumentedTypePrintfDebug(&gNetInterfaceNotifierTypeInstrumentation, "NotifyNetworkInterfacesChanged -- %s changed - NOTIFY", existing->name);
				sNotifyInterfaceNotifiers(check, kFskNetInterfaceStatusChanged);
			}
			else {
				FskInstrumentedTypePrintfDebug(&gNetInterfaceNotifierTypeInstrumentation, "NotifyNetworkInterfacesChanged -- %s didn't change - no notify", existing->name);
			}
		}
		else {
			FskInstrumentedTypePrintfDebug(&gNetInterfaceNotifierTypeInstrumentation, "NotifyNetworkInterfacesChanged -- not found %s in the newer list - remove it.", existing->name);
			sNotifyInterfaceNotifiers(existing, kFskNetInterfaceStatusRemoved);
		}
		existing = existing->next;
	}

	existing = newer;
	while (existing) {
		check = sFskNetInterfaceFindByName(existing->name, old);
		if (!check) {
			FskInstrumentedTypePrintfDebug(&gNetInterfaceNotifierTypeInstrumentation, "NotifyNetworkInterfacesChanged -- new interface %s not in the older list - ADD it.", existing->name);
			sNotifyInterfaceNotifiers(existing, kFskNetInterfaceStatusNew);
		}
		existing = existing->next;
	}

	FskInstrumentedTypePrintfDebug(&gNetInterfaceNotifierTypeInstrumentation, "NotifyNetworkInterfacesChanged -- toss old list.");
	sDisposeInterfaceList(&old);


	existing = gNetworkInterfaceList->list;
	while (existing) {
		FskInstrumentedTypePrintfDebug(&gNetInterfaceNotifierTypeInstrumentation, " -- %s - %08x %08x %d", existing->name, existing->ip, existing->netmask, existing->status);
		existing = existing->next;
	}

	FskMutexRelease(gNetworkInterfaceList->mutex);
	return retVal;
}
#endif /* TARGET_OS_WIN32 || TARGET_OS_LINUX || TARGET_OS_MAC || TARGET_OS_KPL */

// ---------------------------------------------------------------------
FskErr FskNetInterfaceDescribe(int idx, FskNetInterfaceRecord **iface)
{
	int i = 0;
	FskErr ret = kFskErrNone;
	FskNetInterfaceRecord *nir;

	FskMutexAcquire(gNetworkInterfaceList->mutex);
	nir = (FskNetInterfaceRecord *)gNetworkInterfaceList->list;

	while (nir) {
		if (i == idx)
			break;
		nir = nir->next;
		i++;
	}

	if (nir) {
		FskNetInterfaceRecord *dup;
		if (kFskErrNone == FskMemPtrNewFromData(sizeof(FskNetInterfaceRecord), nir, &dup)) {
			dup->next = NULL;
			dup->name = FskStrDoCopy(nir->name);
		}
		*iface = dup;
	}
	else {
		*iface = NULL;
		ret = kFskErrNetworkInterfaceNotFound;
	}

	FskMutexRelease(gNetworkInterfaceList->mutex);
	return ret;
}


FskNetInterfaceNotifier FskNetInterfaceAddNotifier(FskNetInterfaceChangedCallback callback, void *param, char *debugName)
{
	FskNetInterfaceNotifier notRef = NULL;
	FskThread thread = FskThreadGetCurrent();
	UInt32 nameLen = debugName ? FskStrLen(debugName) + 1 : 0;

	if (kFskErrNone == FskMemPtrNewClear(sizeof(FskNetInterfaceNotifierRec) + nameLen, &notRef)) {
		FskInstrumentedTypePrintfDebug(&gNetInterfaceNotifierTypeInstrumentation, "NetInterfaceNotifier NEW -- %p", notRef);
		notRef->callback = callback;
		notRef->param = param;

		notRef->thread = thread;
		if (nameLen)
			FskMemMove(notRef->name, debugName, nameLen);

		FskListMutexPrepend(interfaceChangeCBList, notRef);
		FskInstrumentedItemNew(notRef, notRef->name, &gNetInterfaceNotifierTypeInstrumentation);
	}
	return notRef;
}

void FskNetInterfaceRemoveNotifier(FskNetInterfaceNotifier callbackRef)
{
//	FskThread thread = FskThreadGetCurrent();

//	FskInstrumentedTypePrintfDebug(&gNetInterfaceNotifierTypeInstrumentation, "NetInterfaceNotifier REMOVE -- %x", callbackRef);
	if (NULL != callbackRef) {
		FskInstrumentedTypePrintfDebug(&gNetInterfaceNotifierTypeInstrumentation, "NetInterface REMOVE -- %p %s", callbackRef, callbackRef->name);
		FskListMutexRemove(interfaceChangeCBList, callbackRef);
		FskInstrumentedItemDispose(callbackRef);
		FskMemPtrDispose(callbackRef);
	}
//	FskInstrumentedTypePrintfDebug(&gNetInterfaceNotifierTypeInstrumentation, "NetInterface REMOVE -- %x done", callbackRef);
}

#if TARGET_OS_WIN32 || TARGET_OS_LINUX || TARGET_OS_MAC || TARGET_OS_KPL

static void sFskHandleNetworkInterfacesChanged() {
#if SUPPORT_INSTRUMENTATION
	FskThread thread = FskThreadGetCurrent();
	FskInstrumentedTypePrintfDebug(&gNetInterfaceNotifierTypeInstrumentation, "Interfaces changed -- thread is: %p - %s", thread, thread->name);
#endif /* SUPPORT_INSTRUMENTATION */

	if (sHaveNetworkInterfacesChanged()) {
		FskInstrumentedTypePrintfDebug(&gNetInterfaceNotifierTypeInstrumentation, "Interfaces have changed - about to call notify");
		sNotifyNetworkInterfacesChanged();
		FskInstrumentedTypePrintfDebug(&gNetInterfaceNotifierTypeInstrumentation, "Interfaces have changed - after calling notify");
	}
}

#endif /* TARGET_OS_WIN32 || TARGET_OS_LINUX || TARGET_OS_MAC || TARGET_OS_KPL */

Boolean FskNetIsLocalAddress(int addr) {
	FskNetInterfaceRecord *nir;
	Boolean ret = false;

	if (0x7f000001 == addr)
		return true;

	FskMutexAcquire(gNetworkInterfaceList->mutex);
	nir = (FskNetInterfaceRecord *)gNetworkInterfaceList->list;

	while (nir) {
		if (nir->ip == addr) {
			ret = true;
			goto done;
		}
		nir = nir->next;
	}

done:
	FskMutexRelease(gNetworkInterfaceList->mutex);
	return ret;
}

Boolean FskNetIsLocalNetwork(int addr) {
	FskNetInterfaceRecord *nir;
	Boolean ret = false;

	FskMutexAcquire(gNetworkInterfaceList->mutex);
	nir = (FskNetInterfaceRecord *)gNetworkInterfaceList->list;

	while (nir) {
		if ((0 != nir->ip) && ((nir->ip & nir->netmask) == (addr & nir->netmask))) {
			ret = true;
			goto done;
		}
		nir = nir->next;
	}

done:
	FskMutexRelease(gNetworkInterfaceList->mutex);
	return ret;
}

#if TARGET_OS_IPHONE
void FskNetInterfacesChanged() {
	sFskHandleNetworkInterfacesChanged();
}
#endif /* TARGET_OS_IPHONE */

#if TARGET_OS_LINUX
void LinuxInterfacesChanged() {
#if !defined(ANDROID)
	_res.options &= ~RES_INIT;		// reinitialize nameserver to catch changes
#endif /* ! ANDROID */
	sFskHandleNetworkInterfacesChanged();
}
#endif /* TARGET_OS_LINUX */

FskErr FskNetInterfaceInitialize(void) {
	FskErr err;

	err = FskListMutexNew(&interfaceChangeCBList, "interfaceChangeCBLIst");
	if (err) return err;

#if TARGET_OS_WIN32
	installWindowsNetworkInterfaceChangeTracker();
#elif TARGET_OS_IPHONE
	installIOSNetworkInterfaceChangeTracker();
#elif TARGET_OS_MAC
	installMacNetworkInterfaceChangeTracker();
#endif /* TARGET_OS */

#if TARGET_OS_KPL
	KplNetInterfaceInitialize();
	KplNetInterfaceSetChangedCallback(KplNetInterfaceChanged, 0L);
#endif /* TARGET_OS_KPL */

	err = FskListMutexNew(&gNetworkInterfaceList, "gNetworkInterfaceList");
	if (err) return err;
	FskMutexAcquire(gNetworkInterfaceList->mutex);
	sFskNetInterfaceEnumerate((FskNetInterfaceRecord**)(void*)&gNetworkInterfaceList->list);
	FskMutexRelease(gNetworkInterfaceList->mutex);

	return err;
}

void FskNetInterfaceTerminate(void) {
	while (interfaceChangeCBList->list)
		FskNetInterfaceRemoveNotifier((FskNetInterfaceNotifier)interfaceChangeCBList->list);

	FskNetCleanupNetworkEnumeration();
#if TARGET_OS_WIN32
	removeWindowsNetworkInterfaceChangeTracker();
#elif TARGET_OS_IPHONE
	removeIOSNetworkInterfaceChangeTracker();
#elif TARGET_OS_MAC
	removeMacNetworkInterfaceChangeTracker();
#endif

#if TARGET_OS_KPL
	KplNetInterfaceTerminate();
#endif /* TARGET_OS_KPL */

	FskListMutexDispose(interfaceChangeCBList);
}

#if TARGET_OS_KPL
void KplNetInterfaceChanged(void *refCon)
{
	sFskHandleNetworkInterfacesChanged();
}
#endif /* TARGET_OS_KPL */

#if SUPPORT_INSTRUMENTATION
static Boolean doFormatMessageFskNetInterfaceNotifier(FskInstrumentedType dispatch, UInt32 msg, void *msgData, char *buffer, UInt32 bufferSize)
{
	FskInterfaceInstrData *data = (FskInterfaceInstrData *)msgData;

	switch (msg) {
		case kFskNetInstrMsgInterfaceNotify:
			{
			char *action;
			if (kFskNetInterfaceStatusNew == data->status)
				action = "added";
			else if (kFskNetInterfaceStatusRemoved == data->status)
				action = "removed";
			else if (kFskNetInterfaceStatusChanged == data->status)
				action = "changed";
			else
				action = "unknown";
			snprintf(buffer, bufferSize, "Notify (%s) interface [%s] [%s, %d.%d.%d.%d %d.%d.%d.%d (%s)]",
				data->notf->name, action, data->ifc->name,
				intAs4Bytes(data->ifc->ip), intAs4Bytes(data->ifc->netmask), data->ifc->status ? "up" : "down");
			return true;
			}
	}

	return false;
}
#endif /* SUPPORT_INSTRUMENTATION */
