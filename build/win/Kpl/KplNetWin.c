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
#include "KplNet.h"
#include "FskMemory.h"
#include "FskString.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <WinDNS.h>

FskErr KplNetHostnameResolve(char *hostname, int *addr)
{
	struct sockaddr_in	tcp_srv_addr;
	UInt32				inaddr;
	struct hostent		*hp, tcp_host_info;

	FskMemSet((char*)&tcp_srv_addr, 0, sizeof(tcp_srv_addr));
	tcp_srv_addr.sin_family = AF_INET;
	tcp_srv_addr.sin_port = 0;

	if ((inaddr = inet_addr((const char*)hostname)) != INADDR_NONE) { // dotted decimal
		FskMemMove(&tcp_srv_addr.sin_addr, &inaddr, sizeof(inaddr));
		tcp_host_info.h_name = NULL;
	}
	else {
		if ((hp = gethostbyname((const char*)hostname)) == NULL) {
			return kFskErrNameLookupFailed;
		}
		
		tcp_host_info = *hp;	// found by name
		FskMemMove(&tcp_srv_addr.sin_addr, hp->h_addr, hp->h_length);
	}

	*addr = ntohl(tcp_srv_addr.sin_addr.s_addr);

	return kFskErrNone;
}

FskErr KplNetServerSelection(const char *dname, char **srvName, UInt16 *targetPort)
{
	DNS_STATUS status;
	PDNS_RECORD dnsRecord = NULL, p;
	DNS_SRV_DATA *sp, *target = NULL;
	FskErr err = kFskErrNameLookupFailed;

	status = DnsQuery(dname, DNS_TYPE_SRV, DNS_QUERY_STANDARD, NULL, &dnsRecord, NULL);
	if (status != NO_ERROR) {
		err = status == ERROR_DS_DNS_LOOKUP_FAILURE ? kFskErrNameLookupFailed : kFskErrOperationFailed;
		goto bail;
	}
	for (p = dnsRecord; p != NULL; p = p->pNext) {
		sp = &p->Data.SRV;
		if (target == NULL || sp->wPriority < target->wPriority || (sp->wPriority == target->wPriority && sp->wWeight > target->wWeight))
			target = sp;
	}
	if (target == NULL)
		goto bail;
	*srvName = FskStrDoCopy(target->pNameTarget);
	if (*srvName == NULL) {
		err = kFskErrMemFull;
		goto bail;
	}
	if (targetPort) *targetPort = target->wPort;
	err = kFskErrNone;
bail:
	DnsRecordListFree(dnsRecord, DnsFreeRecordList);
	return err;
}
