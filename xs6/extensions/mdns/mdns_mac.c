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

#include "mc_stdio.h"
#include "mc_event.h"
#include "mc_misc.h"
#include "mc_xs.h"
#include "mc_time.h"
#include "mc_memory.h"
#include "mc_ipc.h"
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <dns_sd.h>

typedef struct {
	DNSServiceRef ref, rref;
	TXTRecordRef txt;
	xsMachine *the;
	xsSlot this;
	char *name;
} mdns_t;

static void
mdns_set_txt_rec(xsMachine *the, mdns_t *mdns, xsSlot *keys)
{
	char key[256], val[256];
	xsIndex id_next = xsID("next");
	xsIndex id_done = xsID("done");
	xsIndex id_value = xsID("value");
	int base;

	TXTRecordCreate(&mdns->txt, 0, NULL);
	if (keys == NULL)
		return;
	base = xsVars(4);
	xsVar(base + 0) = xsEnumerate(*keys);
	do {
		xsCall(xsVar(base + 1), xsVar(base + 0), id_next, NULL);
		xsGet(xsVar(base + 2), xsVar(base + 1), id_done);
		if (xsTest(xsVar(base + 2)))
			break;
		xsGet(xsVar(base + 2), xsVar(base + 1), id_value);
		xsGetAt(xsVar(base + 3), *keys, xsVar(base + 2));
		xsToStringBuffer(xsVar(base + 2), key, sizeof(key));
		xsToStringBuffer(xsVar(base + 3), val, sizeof(val));
		TXTRecordSetValue(&mdns->txt, key, strlen(val), val);
	} while (1);
	xsVars(-4);
}

static void
resolv_reply(DNSServiceRef sdRef, DNSServiceFlags flags, uint32_t interfaceIndex, DNSServiceErrorType errorCode, const char *fullname, const char *hosttarget, uint16_t port, /* In network byte order */ uint16_t txtLen, const unsigned char *txtRecord, void *context)
{
	mdns_t *mdns = context;
	char key[257], value[257];
	uint8_t len;
	const void *ptr;
	int idx;

	xsBeginHost(mdns->the);
	xsVars(5);
	xsSetNewInstanceOf(xsVar(0), xsObjectPrototype);
	for (idx = 0; TXTRecordGetItemAtIndex(txtLen, txtRecord, idx, sizeof(key), key, &len, &ptr) == kDNSServiceErr_NoError; idx++) {
		memcpy(value, ptr, len);
		value[len] = '\0';
		xsSetString(xsVar(1), value);
		xsSet(xsVar(0), xsID(key), xsVar(1));
	}
	xsSetString(xsVar(1), (char *)fullname);
	xsSetString(xsVar(2), mdns->name);
	xsSetString(xsVar(3), (char *)hosttarget);
	xsSetInteger(xsVar(4), htons(port));
	xsCall_noResult(mdns->this, xsID("callback"), &xsVar(1), &xsVar(2), &xsVar(3), &xsVar(4), &xsVar(0), NULL);
	xsEndHost(mdns->the);
	mc_free(mdns->name); mdns->name = NULL;
	mc_event_unregister(DNSServiceRefSockFD(mdns->rref));
	DNSServiceRefDeallocate(mdns->rref);
	mdns->rref = NULL;
}

static void
mdns_resolv_callback(int s, unsigned int flags, void *closure)
{
	mdns_t *mdns = closure;

	if (flags & MC_SOCK_READ)
		DNSServiceProcessResult(mdns->rref);
}

static void
browse_reply(DNSServiceRef sdRef, DNSServiceFlags flags, uint32_t interfaceIndex, DNSServiceErrorType errorCode, const char *serviceName, const char *regtype, const char *replyDomain, void *context)
{
	mdns_t *mdns = context;
	int err;

	if (flags & kDNSServiceFlagsAdd) {
		int fd;
		mdns->name = mc_strdup(serviceName);
		err = DNSServiceResolve(&mdns->rref, kDNSServiceFlagsForceMulticast, interfaceIndex, serviceName, regtype, replyDomain, resolv_reply, context);
		if (err != 0) {
			mc_log_error("mdns: resolv error: %d\n", err);
			return;
		}
		fd = DNSServiceRefSockFD(mdns->rref);
		mc_event_register(fd, MC_SOCK_READ, mdns_resolv_callback, mdns);
	}
	else {
		xsBeginHost(mdns->the);
		xsVars(2);
		xsSetUndefined(xsVar(0));
		xsSetString(xsVar(1), (char *)serviceName);
		xsCall_noResult(mdns->this, xsID("callback"), &xsVar(0), &xsVar(1), NULL);
		xsEndHost(mdns->the);
	}
}

static void
mdns_browse_callback(int s, unsigned int flags, void *closure)
{
	mdns_t *mdns = closure;

	if (flags & MC_SOCK_READ)
		DNSServiceProcessResult(mdns->ref);
}

void
xs_mdns_constructor(xsMachine *the)
{
	int err;
	mdns_t *mdns;
	char *type = NULL, *name = NULL, *domain = NULL;
	int port = 0;
	xsIndex id_type = xsID("type");
	xsIndex id_name = xsID("name");
	xsIndex id_port = xsID("port");
	xsIndex id_key = xsID("key");
	xsIndex id_query = xsID("query");

	xsVars(1);
	if (xsToInteger(xsArgc) < 1)
		return;
	if ((mdns = mc_malloc(sizeof(mdns_t))) == NULL)
		mc_xs_throw(the, "mdns: no mem");
	if (xsHas(xsArg(0), id_type)) {
		xsGet(xsVar(0), xsArg(0), id_type);
		type = mc_strdup(xsToString(xsVar(0)));
	}
	if (xsHas(xsArg(0), id_name)) {
		xsGet(xsVar(0), xsArg(0), id_name);
		name = mc_strdup(xsToString(xsVar(0)));
	}
	if (xsHas(xsArg(0), id_port)) {
		xsGet(xsVar(0), xsArg(0), id_port);
		port = xsToInteger(xsVar(0));
	}
	mdns_set_txt_rec(the, mdns, NULL);
	if (xsHas(xsArg(0), id_key)) {
		xsGet(xsVar(0), xsArg(0), id_key);
		if (xsTest(xsVar(0)))
			mdns_set_txt_rec(the, mdns, &xsVar(0));
	}
	if (xsHas(xsArg(0), id_query)) {
		int fd;
		mdns->the = the;
		mdns->this = xsThis;
		err = DNSServiceBrowse(&mdns->ref, 0, 0, type, domain, browse_reply, mdns);
		if (type != NULL) mc_free(type);
		if (err != 0) {
			mc_free(mdns);
			mc_xs_throw(the, "mdns: browsing error: %d", err);
		}
		fd = DNSServiceRefSockFD(mdns->ref);
		mc_event_register(fd, MC_SOCK_READ, mdns_browse_callback, mdns);
	}
	else {
		err = DNSServiceRegister(&mdns->ref, kDNSServiceInterfaceIndexAny, 0, name, type, domain, NULL, htons(port), TXTRecordGetLength(&mdns->txt), TXTRecordGetBytesPtr(&mdns->txt), NULL, NULL);
		if (type != NULL) mc_free(type);
		if (name != NULL) mc_free(name);
		if (err != 0) {
			mc_free(mdns);
			mc_xs_throw(the, "mdns: register error: %d", err);
		}
	}
	mdns->rref = NULL;
	xsSetHostData(xsThis, mdns);
}

void
xs_mdns_destructor(void *data)
{
	if (data != NULL) {
		mdns_t *mdns = data;
		if (mdns->rref != NULL) {
			mc_event_unregister(DNSServiceRefSockFD(mdns->rref));
			DNSServiceRefDeallocate(mdns->rref);
		}
		mc_event_unregister(DNSServiceRefSockFD(mdns->ref));
		DNSServiceRefDeallocate(mdns->ref);
		mc_free(mdns);
	}
}

void
xs_mdns_update(xsMachine *the)
{
	mdns_t *mdns = xsGetHostData(xsThis);
	int err;

	mdns_set_txt_rec(the, mdns, &xsArg(0));
	err = DNSServiceUpdateRecord(mdns->ref, NULL, 0, TXTRecordGetLength(&mdns->txt), TXTRecordGetBytesPtr(&mdns->txt), 0);
	if (err != 0)
		mc_log_error("mdns: update failed: %d\n", err);
}

void
xs_mdns_close(xsMachine *the)
{
	mdns_t *mdns = xsGetHostData(xsThis);

	xs_mdns_destructor(mdns);
	xsSetHostData(xsThis, NULL);
}
