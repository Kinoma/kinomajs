/*
     Copyright (C) 2010-2015 Marvell International Ltd.
     Copyright (C) 2002-2010 Kinoma, Inc.

     Licensed under the Apache License, Version 2.0 (the "License");
     you may not use this file except in compliance with the License.
     You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

     Unless required by applicable law or agreed to in writing, software
     distributed under the License is distributed on an "AS IS" BASIS,
     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
     See the License for the specific language governing permissions and
     limitations under the License.
*/
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
#include <linux/netlink.h>
#include <linux/rtnetlink.h>

#include <netdb.h>
#include <net/if.h>
#include <netinet/in.h>
#ifdef ANDROID_PLATFORM
#include <dirent.h>
#else
#include <ifaddrs.h>
#endif
#include <net/if_arp.h>

#define __FSKTHREAD_PRIV__
#include "FskThread.h"
#include "FskNetUtils.h"
#include "FskList.h"
#include "FskUtilities.h"
#include "KplNetInterface.h"

#if ANDROID_PLATFORM
	#define IGNORE_NETINTERFACE	1
	#define NUM_IGNORE_NET 2
	char *ignoreInterfaces[NUM_IGNORE_NET] = { "usb0", "p2p0" };
#endif

static KplNetInterfaceChangedCallback gNetInterfaceChangedCallback;
static void *gNetInterfaceChangedCallbackRefcon;
static FskThreadDataHandler gInterfaceDataHandler = NULL;
static FskThreadDataSource gInterfaceDataSource = NULL;

void KplNetInterfaceDataHandlerCallback(FskThreadDataHandler handler, FskThreadDataSource source, void *refCon)
{
	int len;
	char buf[4096];
	struct iovec iov = { buf, sizeof(buf) };
	struct sockaddr_nl sa;
	struct msghdr msg = { (void *)&sa, sizeof(sa), &iov, 1, NULL, 0, 0 };
	struct nlmsghdr *nh;
	int fd;
	
	fd = source->dataNode;
	len = recvmsg(fd, &msg, 0);

	for (nh = (struct nlmsghdr *) buf; NLMSG_OK (nh, len); nh = NLMSG_NEXT (nh, len)) {
		/* The end of multipart message. */
		if (nh->nlmsg_type == NLMSG_DONE)
			break;
	}
	gNetInterfaceChangedCallback(gNetInterfaceChangedCallbackRefcon);
	return;
}

FskErr KplNetInterfaceInitialize(void)
{
	FskErr err = kFskErrNone;
	struct sockaddr_nl sa;
	int fd, ret;

	memset(&sa, 0, sizeof(sa));
	sa.nl_family = AF_NETLINK;
	sa.nl_groups = RTMGRP_LINK | RTMGRP_IPV4_IFADDR;

	fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
	if (fd < 0) {
		err = kFskErrSocketNotConnected;
		goto bail;
	}
	
	gInterfaceDataSource = FskThreadCreateDataSource(fd);
	
	ret = bind(fd, (struct sockaddr *) &sa, sizeof(sa));
	if (-1 == ret) {
		err = kFskErrSocketNotConnected;
		goto bail;
	}
	
	FskThreadAddDataHandler(&gInterfaceDataHandler, gInterfaceDataSource, KplNetInterfaceDataHandlerCallback, true, false, NULL);
bail:
	return err;
}

FskErr KplNetInterfaceTerminate(void)
{
	FskThreadRemoveDataHandler(&gInterfaceDataHandler);
	if (gInterfaceDataSource && (gInterfaceDataSource->dataNode >= 0))
		close(gInterfaceDataSource->dataNode);
	FskMemPtrDispose(gInterfaceDataSource);
	gInterfaceDataSource = NULL;
	
	return kFskErrNone;
}

#ifdef ANDROID_PLATFORM
FskErr KplNetInterfaceEnumerate(KplNetInterfaceRecord **interfaceList)
{
	FskErr err = kFskErrNone;
	KplNetInterfaceRecord *nir;
	int fd;
	struct ifreq ifr;
	struct sockaddr_in *sa;
	DIR *d;
	struct dirent *dir;

	*interfaceList = NULL;

	fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
	if (fd < 0) goto skip;

fprintf(stderr, "Kpl-ANDROID Enumerate Interfaces:\n");
	d = opendir("/sys/class/net");
	if (0 == d) {
		BAIL(kFskErrNetworkInterfaceError);
	}

	while ((dir = readdir(d))) {
#if IGNORE_NETINTERFACE
		Boolean ignore = false;
		int i;
#endif
		char *ifname;
		unsigned int theIP = 0, theNetmask = 0, theStatus = 0;

		if (dir->d_name[0] == '.')
			continue;
		ifname = dir->d_name;
fprintf(stderr, "  -- %s: ", ifname);

#if IGNORE_NETINTERFACE
		i = NUM_IGNORE_NET;
		while (i) {
			if (FskStrCompare(ignoreInterfaces[i-1], ifname) == 0) {
				fprintf(stderr, " IGNORED");
				ignore = true;
			}
			i--;
		}
		if (ignore)
			continue;
#endif /* IGNORE_NETINTERFACE */	

		memset (&ifr, 0, sizeof(struct ifreq));
		strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
		ifr.ifr_name[IFNAMSIZ - 1] = 0;

		if (ioctl(fd, SIOCGIFADDR, &ifr) >= 0) {
			sa = (struct sockaddr_in*)(void*)&(ifr.ifr_addr);
			theIP = ntohl(sa->sin_addr.s_addr);
		}

		if (ioctl(fd, SIOCGIFNETMASK, &ifr) >= 0) {
			sa = (struct sockaddr_in*)(void*)&(ifr.ifr_addr);
			theNetmask = ntohl(sa->sin_addr.s_addr);
		}

		if (ioctl(fd, SIOCGIFFLAGS, &ifr) >= 0) {
			if (ifr.ifr_flags & 1)
				theStatus = 1;
		}

		if (theIP == 0)
			theStatus = 0;

fprintf(stderr, "IP: %x, Netmask: %x [%s]", theIP, theNetmask, theStatus ? "UP" : "DOWN");

		if (ioctl(fd, SIOCGIFHWADDR, &ifr) >= 0) {
fprintf(stderr, " Got HWADDR ");
			if (ifr.ifr_hwaddr.sa_family == ARPHRD_ETHER) {
fprintf(stderr, "ETHER");
				FskMemPtrNewClear(sizeof(KplNetInterfaceRecord), (FskMemPtr*)(void*)&nir);
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
fprintf(stderr, " Family not ETHER -- huh?");
			}
		}
	}
	closedir(d);
skip:

	err = FskMemPtrNewClear(sizeof(KplNetInterfaceRecord), (FskMemPtr*)(void*)&nir);
	if (kFskErrNone == err) {
		nir->name = FskStrDoCopy("localhost");
		nir->ip = FskNetMakeIP(127, 0, 0, 1);
		nir->netmask = 0xff000000;
		nir->status = 1;
		FskListAppend((FskList*)interfaceList, nir);
	}

bail:
	if (fd >= 0)
		close(fd);

	return err;
}
#else

FskErr KplNetInterfaceEnumerate(KplNetInterfaceRecord **interfaceList)
{
	FskErr err = kFskErrNone;
	KplNetInterfaceRecord *nir;
	struct ifaddrs* ifaddr = NULL;
	struct ifaddrs* ifa;
	int fd = -1;
	struct ifreq  ifr;
    
	*interfaceList = NULL;

	if (getifaddrs(&ifaddr) == -1) {
		err = kFskErrNetworkInterfaceError;
		goto done;
	}
	
	fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
	if (fd < 0) goto done;
    
	for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
		if (ifa->ifa_addr == NULL)
			continue;

		if (ifa->ifa_addr->sa_family == AF_INET) {
			memset(&ifr, 0,  sizeof(struct ifreq));
			strncpy(ifr.ifr_name, ifa->ifa_name, IFNAMSIZ);
			ifr.ifr_name[IFNAMSIZ - 1] = 0;
            
			FskMemPtrNewClear(sizeof(KplNetInterfaceRecord), (FskMemPtr *)&nir);
			if (!nir) {
				err = kFskErrMemFull;
				goto done;
			}
			nir->name = FskStrDoCopy(ifa->ifa_name);
			nir->ip = ntohl(((struct sockaddr_in *)ifa->ifa_addr)->sin_addr.s_addr);
			nir->netmask = ntohl(((struct sockaddr_in *)ifa->ifa_netmask)->sin_addr.s_addr);
			nir->status = (ifa->ifa_flags & IFF_UP) ? 1 : 0;
			if (ioctl(fd, SIOCGIFHWADDR, &ifr) >= 0) {
				if (ifr.ifr_hwaddr.sa_family == ARPHRD_ETHER) {
					FskMemCopy(nir->MAC, ifr.ifr_hwaddr.sa_data, 6);
				}
			}
			FskListAppend((FskList*)interfaceList, nir);
		}
	}
    
done:
	if (fd >= 0)
		close(fd);
	if (ifaddr)
		freeifaddrs(ifaddr);
	return err;
}

#endif
FskErr KplNetInterfaceSetChangedCallback(KplNetInterfaceChangedCallback cb, void *refCon)
{
	gNetInterfaceChangedCallback = cb;
	gNetInterfaceChangedCallbackRefcon = refCon;
	return kFskErrNone;
}
