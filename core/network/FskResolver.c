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
#define __FSKNETUTILS_PRIV__

#include "Fsk.h"
#include "FskList.h"
#include "FskThread.h"
#include "FskPlatformImplementation.h"
#include "FskNetUtils.h"
#include "FskResolver.h"

#if TARGET_OS_LINUX || TARGET_OS_MAC
	#include <stdio.h>
	#include <stdlib.h>
	#include <string.h>
	#include <errno.h>

	#include <netinet/in.h>
	#include <netdb.h>

	#define closesocket		close

	#include <resolv.h>
	#include <arpa/inet.h>
	#include <arpa/nameser.h>
	#if TARGET_OS_MAC
		#if !TARGET_OS_IPHONE
			#include <net/if_types.h>
			#include <net/if_dl.h>
		#endif
		#include <CoreFoundation/CoreFoundation.h>
		#include <SystemConfiguration/SystemConfiguration.h>
	#elif TARGET_OS_LINUX
		#include <net/if_arp.h>
	#endif
#elif TARGET_OS_WIN32
	#include <WinDNS.h>
#endif

#if TARGET_OS_KPL
	#include "KplNet.h"
#endif

#if TARGET_OS_LINUX || TARGET_OS_MAC
/* from RFC1035 (DNS Message format) */
struct rr_header {
	UInt16 rh_id;
	union {
		struct {
#if TARGET_RT_BIG_ENDIAN
			UInt8 rf_qr: 1;
			UInt8 rf_opcode: 4;
			UInt8 rf_aa: 1;
			UInt8 rf_tc: 1;
			UInt8 rf_rd: 1;
			UInt8 rf_ra: 1;
			UInt8 rf_z: 3;
			UInt8 rf_rcode: 4;
#else
			UInt8 rf_rcode: 4;
			UInt8 rf_z: 3;
			UInt8 rf_ra: 1;
			UInt8 rf_rd: 1;
			UInt8 rf_tc: 1;
			UInt8 rf_aa: 1;
			UInt8 rf_opcode: 4;
			UInt8 rf_qr: 1;
#endif
		} ru_rf;
		UInt16 ru_flags;
	} rh_u;
	UInt16 rh_qdcount;
	UInt16 rh_ancount;
	UInt16 rh_nscount;
	UInt16 rh_arcount;
};

struct rr {
	char rr_name[NS_MAXDNAME];
	UInt16 rr_type;
	UInt16 rr_class;
	UInt32 rr_ttl;
	UInt16 rr_rdlength;
	void *rr_rd;
};

struct rr_srv {
	UInt16 rs_priority;
	UInt16 rs_weight;
	UInt16 rs_port;
	char rs_target[NS_MAXDNAME];
};
#endif /* TARGET_OS_LINUX || TARGET_OS_MAC */


void FskAsyncResolverInitialize(void);
void FskAsyncResolverTerminate(void);

static FskThread resolverThread = NULL;
static Boolean resolverThreadDone = true;

// ---------------------------------------------------------------------
FskErr FskNetHostnameResolveQT(char *hostname, long queryType, UInt32 *addr, UInt16 *port)
{
	extern Boolean		gQuitting;
#if !TARGET_OS_KPL
	struct sockaddr_in	tcp_srv_addr;
	UInt32				inaddr;
	struct hostent		*hp;
	char			*srvName = NULL;

	if (gQuitting)
		return kFskErrOutOfSequence;

	if (port != NULL) *port = 0;

	FskMemSet((char*)&tcp_srv_addr, 0, sizeof(tcp_srv_addr));
	tcp_srv_addr.sin_family = AF_INET;
	tcp_srv_addr.sin_port = 0;

	if ((inaddr = inet_addr((const char*)hostname)) != INADDR_NONE) { // dotted decimal
		FskMemMove(&tcp_srv_addr.sin_addr, &inaddr, sizeof(inaddr));
	}
	else {
		FskErr err;
		switch (queryType) {
		case kConnectFlagsServerSelection:
			err = FskNetServerSelection(hostname, &srvName, port);
			if (err) return err;
			hostname = srvName;
			break;
		default:	/* fall thru */
			break;
		}
		hp = gethostbyname((const char*)hostname);
		if (srvName != NULL) FskMemPtrDispose(srvName);
		if (hp == NULL) {
			if (NULL == gethostbyname("kinoma.com"))
				return kFskErrNoNetworkInterfaces;
			return kFskErrNameLookupFailed;
		}

		FskMemMove(&tcp_srv_addr.sin_addr, hp->h_addr, hp->h_length);
	}

	*addr = ntohl(tcp_srv_addr.sin_addr.s_addr);

	return kFskErrNone;
#else
	FskErr err = kFskErrNone;
	char *srvName = NULL;
	if (gQuitting)
		return kFskErrOutOfSequence;

	if (port != NULL) *port = 0;

	switch (queryType) {
	case kConnectFlagsServerSelection:
		err = KplNetServerSelection(hostname, &srvName, port);
		if (err) return err;
		hostname = srvName;
		break;
	default:	/* fall thru */
		break;
	}
	err = KplNetHostnameResolve(hostname, (int*)addr);
	if (srvName != NULL) FskMemPtrDispose(srvName);
	return err;
#endif
}

#if TARGET_OS_KPL
FskErr FskNetServerSelection(const char *dname, char **srvName, UInt16 *targetPort)
{
	return KplNetServerSelection(dname, srvName, targetPort);

}
#elif TARGET_OS_WIN32	/* including WINCE ... hopefully... */
FskErr FskNetServerSelection(const char *dname, char **srvName, UInt16 *targetPort)
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
    BAIL_IF_NULL(*srvName, err, kFskErrMemFull);
	if (targetPort) *targetPort = target->wPort;
	err = kFskErrNone;
bail:
	DnsRecordListFree(dnsRecord, DnsFreeRecordList);
	return err;
}
#elif TARGET_OS_LINUX || TARGET_OS_MAC
static const u_char *
dnGetResource(struct rr *rp, const u_char *mp, const u_char *msg, const u_char *eom)
{
	int n;
#define CHECK_EOM(n)	if (mp + n > eom) return NULL

	if ((n = dn_expand(msg, eom, mp, rp->rr_name, sizeof(rp->rr_name))) < 0)
		return NULL;
	mp += n;
	CHECK_EOM(2); rp->rr_type = ns_get16(mp); mp += 2;
	CHECK_EOM(2); rp->rr_class = ns_get16(mp); mp += 2;
	CHECK_EOM(4); rp->rr_ttl = ns_get32(mp); mp += 4;
	CHECK_EOM(2); rp->rr_rdlength = ns_get16(mp); mp += 2;
	CHECK_EOM(rp->rr_rdlength); rp->rr_rd = (void *)mp; mp += rp->rr_rdlength;
	return mp;
}

static const u_char *
dnGetService(struct rr_srv *sp, const struct rr *rp, const u_char *msg)
{
	int n;
	u_char *mp = rp->rr_rd;
	u_char *eom = rp->rr_rd + rp->rr_rdlength;
#undef CHECK_EOM
#define CHECK_EOM(n)	if (mp + n > eom) return NULL

	CHECK_EOM(2); sp->rs_priority = ns_get16(mp); mp += 2;
	CHECK_EOM(2); sp->rs_weight = ns_get16(mp); mp += 2;
	CHECK_EOM(2); sp->rs_port = ns_get16(mp); mp += 2;
	if ((n = dn_expand(msg, eom, mp, sp->rs_target, sizeof(sp->rs_target))) < 0)
		return NULL;
	mp += n;
	return mp;
}

FskErr FskNetServerSelection(const char *dname, char **srvName, UInt16 *targetPort)
{
	int n, i;
	struct rr_header rh;
	struct rr *rp = NULL;
	struct rr_srv *sp = NULL, *target = NULL;
	u_char msg[NS_PACKETSZ];
	const u_char *mp, *eom;
	FskErr err = kFskErrNameLookupFailed;
#undef CHECK_EOM
#define CHECK_EOM(n)	if (mp + n > eom) goto bail

	if ((n = res_query(dname, ns_c_in, ns_t_srv, msg, sizeof(msg))) < 0)
		return kFskErrNameLookupFailed;
	mp = msg;
	eom = msg + n;
	CHECK_EOM(2); rh.rh_id = ns_get16(mp); mp += 2;
	CHECK_EOM(2); rh.rh_u.ru_flags = ns_get16(mp); mp += 2;
	CHECK_EOM(2); rh.rh_qdcount = ns_get16(mp); mp += 2;
	CHECK_EOM(2); rh.rh_ancount = ns_get16(mp); mp += 2;
	CHECK_EOM(2); rh.rh_nscount = ns_get16(mp); mp += 2;
	CHECK_EOM(2); rh.rh_arcount = ns_get16(mp); mp += 2;
	if (rh.rh_u.ru_rf.rf_rcode != ns_r_noerror)
		goto bail;

	/* skip the question section */
	for (i = 0; i < rh.rh_qdcount; i++) {
		/* qname / qtype / qclass */
		if ((n = dn_skipname(mp, eom)) < 0) goto bail;
		mp += n;
		CHECK_EOM(4); mp += 4;	// qtype + qclass
	}

	/* process resource records */
	if (FskMemPtrNew(sizeof(struct rr), &rp) != kFskErrNone)
        BAIL(kFskErrMemFull);
	for (i = 0; i < rh.rh_ancount; i++) {
		if (sp == NULL) {
			if (FskMemPtrNew(sizeof(struct rr_srv), &sp) != kFskErrNone)
                BAIL(kFskErrMemFull);
		}
		if ((mp = dnGetResource(rp, mp, msg, eom)) == NULL || dnGetService(sp, rp, msg) == NULL)
			goto bail;
		if (target == NULL || sp->rs_priority < target->rs_priority || (sp->rs_priority == target->rs_priority && sp->rs_weight > target->rs_weight)) {
			if (target != NULL) {
				struct rr_srv *t = target;
				target = sp;
				sp = t;
			}
			else {
				target = sp;
				sp = NULL;
			}
		}
	}
	if (target == NULL)
		goto bail;
	*srvName = FskStrDoCopy(target->rs_target);
    BAIL_IF_NULL(*srvName, err, kFskErrMemFull);
	if (targetPort) *targetPort = target->rs_port;
	err = kFskErrNone;

bail:
	if (target != NULL)
		FskMemPtrDispose(target);
	if (sp != NULL)
		FskMemPtrDispose(sp);
	if (rp != NULL)
		FskMemPtrDispose(rp);
	return err;
}
#endif /* TARGET_OS_LINUX || TARGET_OS_MAC */

static void resolverThreadProc(void  *refcon);
static void didResolve(void *a, void *b, void *c, void *d);
FskErr FskResolverDispose(FskResolver rr);

FskListMutex	resolverQueue = NULL;

#if SUPPORT_INSTRUMENTATION
static Boolean doFormatMessageFskResolver(FskInstrumentedType dispatch, UInt32 msg, void *msgData, char *buffer, UInt32 bufferSize);

static FskInstrumentedTypeRecord gFskResolverTypeInstrumentation = {
	NULL,
	sizeof(FskInstrumentedTypeRecord),
	"resolver",
	FskInstrumentationOffset(FskResolverRecord),
	NULL,
	0,
	NULL,
	doFormatMessageFskResolver
};
#endif

void FskAsyncResolverInitialize(void) {
	if (NULL == resolverQueue)
		FskListMutexNew(&resolverQueue, "resolverQueue");

	resolverThreadDone = false;
	FskThreadCreate(&resolverThread, resolverThreadProc, kFskThreadFlagsJoinable | kFskThreadFlagsWaitForInit, NULL, "resolver");
}

void FskAsyncResolverTerminate(void) {
	FskResolver rr;

	if (resolverQueue) {
		while (NULL != (rr = ((FskResolver)FskListMutexRemoveFirst(resolverQueue)))) {
			rr->err = kFskErrOperationCancelled;
			(rr->cb)(rr);
			FskResolverDispose(rr);
		}
		FskListMutexDispose(resolverQueue);
	}
	resolverThreadDone = true;
	FskThreadJoin(resolverThread);
}


FskErr FskNetHostnameResolveQTAsync(char *hostname, long queryType, FskResolverCallbackFn onResolved, void *ref, FskResolver *out) {
	FskResolver rr;
	FskErr err;
	SInt32 nameLen = FskStrLen(hostname) + 1;
	int i;

    BAIL_IF_ERR(err = FskMemPtrNewClear(sizeof(FskResolverRecord) + nameLen, (FskMemPtr*)(void*)&rr));

	FskStrCopy(rr->name, hostname);

    FskInstrumentedItemNew(rr, rr->name, &gFskResolverTypeInstrumentation);

	rr->queryType = queryType;
	rr->ref = ref;

	if (*rr->name && FskStrIsDottedQuad(hostname, &i)) {
		rr->resolvedIP = i;
		(*onResolved)(rr);
		err = FskResolverDispose(rr);
		goto done;
	}

	if (resolverThreadDone)
		FskAsyncResolverInitialize();

	rr->cb = onResolved;
	rr->callee = FskThreadGetCurrent();
	FskListMutexAppend(resolverQueue, rr);
	if (NULL != resolverThread)
		FskThreadWake(resolverThread);
	*out = rr;
	return kFskErrNone;

bail:
	FskResolverDispose(rr);
done:
	*out = NULL;
	return err;
}

FskErr FskResolverDispose(FskResolver rr) {
    FskInstrumentedItemDispose(rr);

	FskMemPtrDispose(rr);
	return kFskErrNone;
}

FskErr FskResolverCancel(FskResolver rr) {
	FskErr err = kFskErrNone;
	FskMutexAcquire(resolverQueue->mutex);
	if (FskListContains(resolverQueue->list, rr)) {
		rr->cancelled = true;
   		FskInstrumentedItemSendMessage(rr, kFskResolverInstrMsgCancelRequest, rr);
	}
	else
		err = kFskErrItemNotFound;
	FskMutexRelease(resolverQueue->mutex);
	return err;
}

FskErr FskResolverCancelByRef(void *ref) {
	FskResolver rr = NULL;

	FskMutexAcquire(resolverQueue->mutex);
	while (NULL != (rr = (FskResolver)FskListGetNext(resolverQueue->list, rr))) {
		if (ref == rr->ref) {
			rr->cancelled = true;
    		FskInstrumentedItemSendMessage(rr, kFskResolverInstrMsgCanceled, rr);

		}
	}
	FskMutexRelease(resolverQueue->mutex);
		return kFskErrNone;
	}



// didResolve(hostname, address, callback, reference *)
static void didResolve(void *a, void *b, void *c, void *d) {
	FskResolver rr = (FskResolver)a;

    FskInstrumentedItemSendMessage(rr, kFskResolverInstrMsgResolved, rr);
	if (!rr->cancelled)
		(*rr->cb)(rr);
	FskResolverDispose(rr);
}

/* caller thread context above. */
/* resolver thread context below */

static void processResolution() {
	FskResolver rr;

	while (NULL != (rr = (FskResolver)FskListMutexRemoveFirst(resolverQueue))) {
		if (rr->cancelled) {
			FskResolverDispose(rr);
			continue;
		}
		FskInstrumentedItemSendMessage(rr, kFskResolverInstrMsgResolutionRequest, rr);
		rr->err = FskNetHostnameResolveQT(rr->name, rr->queryType, &rr->resolvedIP, &rr->targetPort);
		if (rr->cancelled)
			FskResolverDispose(rr);
		else
			FskThreadPostCallback(rr->callee, didResolve, rr, NULL, NULL, NULL);
	}
}

static void resolverThreadProc(void *refcon)
{
	FskThreadAddEventHandler(kFskEventSystemQuitRequest, (FskThreadEventHandlerRoutine)FskThreadDefaultQuitHandler, &resolverThreadDone);
//	sethostent(1);		// this can be used on Linux to keep a connection to the resolver open
	FskThreadInitializationComplete(FskThreadGetCurrent());

	while (!resolverThreadDone) {
		processResolution();
		FskThreadRunloopCycle(-1);
	}

//	endhostent();
}

#if SUPPORT_INSTRUMENTATION


static Boolean doFormatMessageFskResolver(FskInstrumentedType dispatch, UInt32 msg, void *msgData, char *buffer, UInt32 bufferSize) {
	FskResolver rr = (FskResolver)msgData;
	switch (msg) {
		case kFskResolverInstrMsgResolutionRequest:
			snprintf(buffer, bufferSize, "name resolution request for %s", rr->name);
			return true;
		case kFskResolverInstrMsgCancelRequest:
			snprintf(buffer, bufferSize, "requesting resolver cancelation for %s", rr->name);
			return true;
		case kFskResolverInstrMsgCanceled:
			snprintf(buffer, bufferSize, "resolver canceled for %s", rr->name);
			return true;
		case kFskResolverInstrMsgResolved:
			snprintf(buffer, bufferSize, "name resolved %s - %ld.%ld.%ld.%ld", rr->name,
				(rr->resolvedIP & 0xff000000) >> 24,
				(rr->resolvedIP & 0x00ff0000) >> 16,
				(rr->resolvedIP & 0x0000ff00) >> 8,
				(rr->resolvedIP & 0x000000ff)
			);
			return true;
	}
	return false;
}
#endif
