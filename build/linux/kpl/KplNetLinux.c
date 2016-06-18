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
#include "Fsk.h"
#include "FskMemory.h"
#include "FskString.h"

#include "KplNet.h"
#include "KplSocket.h"

#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <arpa/nameser.h>
#include <resolv.h>

#if ANDRO
//
#elif ANDROID_PLATFORM
#include "arpa_nameser.h"
#include "resolv_private.h"
#endif


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

#ifdef ANDROID_PLATFORM
#define NS_MAXDNAME 1025
#endif
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

FskErr KplNetHostnameResolve(char *hostname, int *addr)
{
	extern Boolean		gQuitting;
	struct sockaddr_in	tcp_srv_addr;
	UInt32				inaddr;
	struct hostent		*hp, tcp_host_info;

	if (gQuitting)
		return kFskErrOutOfSequence;

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

FskErr KplNetServerSelection(const char *dname, char **srvName, UInt16 *targetPort)
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
	if (FskMemPtrNew(sizeof(struct rr), &rp) != kFskErrNone) {
		err = kFskErrMemFull;
		goto bail;
	}
	for (i = 0; i < rh.rh_ancount; i++) {
		if (sp == NULL) {
			if (FskMemPtrNew(sizeof(struct rr_srv), &sp) != kFskErrNone) {
				err = kFskErrMemFull;
				goto bail;
			}
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
	if (*srvName == NULL) {
		err = kFskErrMemFull;
		goto bail;
	}
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


