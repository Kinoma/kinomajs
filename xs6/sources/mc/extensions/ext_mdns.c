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
#include "mc_stdio.h"
#include "mc_xs.h"
#if mxMC
#include <mdns.h>
#include <wm_net.h>
#include <lwip/sockets.h>
#include <lwip/netdb.h>
#include <lwip/inet.h>
#include <lwip/netifapi.h>
#else
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#if defined(__APPLE__)
#include <dns_sd.h>
#endif
#endif

#define MDNS_MAX_SERVICES	6

#if defined(__APPLE__)
enum {
	WM_SUCCESS = 0,
	MDNS_DISCOVERED, MDNS_UPDATED, MDNS_DISAPPEARED,
	UP, DOWN,
	REANNOUNCE,
};

struct mdns_service {
	char *servname;
	char *servtype;
	char *keyvals;
	enum {MDNS_PROTO_TCP, MDNS_PROTO_UDP} proto;
	char *domain;
	int port;
	unsigned long ipaddr;
#if defined(__APPLE__)
	DNSServiceRef ref;
	TXTRecordRef txt;
#endif
};

struct mdns_service_config {
	void *iface_handle;
	struct mdns_service **services;
};

typedef int (*mdns_query_callback_t)(void *data, const struct mdns_service *s, int status);

static char *mdns_domain = NULL, *mdns_hostname = NULL;
static struct mdns_service_config mdns_services[MDNS_MAX_SERVICES];

int
mdns_start(char *domain, char *hostname)
{
	mdns_domain = mc_strdup(domain);
	mdns_hostname = mc_strdup(hostname);
	return 0;
}

void
mdns_stop()
{
	if (mdns_domain != NULL) {
		mc_free(mdns_domain);
		mdns_domain = NULL;
	}
	if (mdns_hostname != NULL) {
		mc_free(mdns_hostname);
		mdns_hostname = NULL;
	}
}

int
mdns_query_monitor(char *fqst, mdns_query_callback_t query_callback, void *data)
{
	return 0;
}

void
mdns_query_unmonitor(char *fqst)
{
}

int
mdns_add_service_iface(struct mdns_service_config config)
{
	mdns_services[0] = config;
	return 0;
}

int
mdns_iface_state_change(void *iface_handle, int state)
{
	struct mdns_service **svcs = NULL;
	int i;
	int err;

	for (i = 0; i < MDNS_MAX_SERVICES; i++) {
		if (mdns_services[i].iface_handle == iface_handle) {
			svcs = mdns_services[i].services;
			break;
		}
	}
	if (svcs == NULL)
		return -1;
	switch (state) {
	case UP:
		for (i = 0; svcs[i] != NULL; i++) {
			struct mdns_service *svc = svcs[i];
			char *type = mc_malloc(strlen(svc->servtype) + 1 + 4 /* proto */ + 1);
			sprintf(type, "_%s.%s", svc->servtype, svc->proto == MDNS_PROTO_TCP ? "_tcp" : "_udp");
			err = DNSServiceRegister(&svc->ref, kDNSServiceInterfaceIndexAny, 0, svc->servname, type, svc->domain, mdns_hostname, htons(svc->port), TXTRecordGetLength(&svc->txt), TXTRecordGetBytesPtr(&svc->txt), NULL, NULL);
			if (err != 0)
				return -1;
		}
		break;
	case REANNOUNCE:
		for (i = 0; svcs[i] != NULL; i++) {
			struct mdns_service *svc = svcs[i];
			err = DNSServiceUpdateRecord(svc->ref, NULL, 0, TXTRecordGetLength(&svc->txt), TXTRecordGetBytesPtr(&svc->txt), 0);
			if (err != 0)
				return -1;
		}
		break;
	}
	return 0;
}

void
mdns_remove_service_iface(void *iface_handle)
{
	struct mdns_service **svcs = NULL;
	int i;

	for (i = 0; i < MDNS_MAX_SERVICES; i++) {
		if (mdns_services[i].iface_handle == iface_handle) {
			svcs = mdns_services[i].services;
			break;
		}
	}
	if (svcs != NULL) {
		for (i = 0; svcs[i] != NULL; i++) {
			struct mdns_service *svc = svcs[i];
			DNSServiceRefDeallocate(svc->ref);
		}
	}
}

void
mdns_set_txt_rec(struct mdns_service *svc, char *keys, char separator)
{
	char *p, *tok1, *tok2;
	char ss[2];

	ss[0] = separator; ss[1] = '\0';
	TXTRecordCreate(&svc->txt, 0, NULL);
	for (; (p = strtok_r(keys, ss, &tok1)) != NULL; keys = NULL) {
		char *key = strtok_r(p, "=", &tok2);
		char *val = strtok_r(NULL, "=", &tok2);
		TXTRecordSetValue(&svc->txt, key, strlen(val), val);
	}
}

int
is_uap_started()
{
	return 0;
}

void *
net_get_uap_handle()
{
	return NULL;
}

void *
net_get_sta_handle()
{
	return NULL;
}
#endif

#if mxMC && WMSDK_VERSION <= 2013082
extern int dnameify(char *name, uint16_t kvlen, uint8_t sep, uint8_t * dest);
int mdns_set_txt_rec(struct mdns_service *s, char *recname)
{
	s->keyvals = recname;
	s->kvlen = strlen(recname) + 1;
	if (s->keyvals != NULL) {
		s->kvlen = dnameify(s->keyvals, s->kvlen, ':', NULL);
		if (s->kvlen > MDNS_MAX_KEYVAL_LEN) {
			// LOG("key/value exceeds MDNS_MAX_KEYVAL_LEN");
			return -WM_E_MDNS_TOOBIG;
		} else
			return WM_SUCCESS;
	} else
		return -WM_E_MDNS_INVAL;
}
#endif

typedef struct {
	struct mdns_service svc;
	char *txt;
	char *fqst;
} mc_mdns_service_t;

typedef struct {
	int started;
	int configured;
	char *domain;
	char *hostname;
	void *iface_handle;
#if WMSDK_VERSION < 3001016
	int num_services;
	struct mdns_service *services[MDNS_MAX_SERVICES + 1];
	mc_mdns_service_t *svcbuf[MDNS_MAX_SERVICES + 1];
#endif
	xsSlot this;
} mc_mdns_t;

static mc_mdns_t mc_mdns = {0};

void
xs_mdns_start(xsMachine *the)
{
	mc_mdns_t *mdns = &mc_mdns;
	int ac = xsToInteger(xsArgc);
	int err;

	if (mdns->started)
		return;
	mdns->domain = ac > 0 ? mc_strdup(xsToString(xsArg(0))) : NULL;
	mdns->hostname = ac > 1 ? mc_strdup(xsToString(xsArg(1))) : NULL;
	if ((err = mdns_start(mdns->domain, mdns->hostname)) != WM_SUCCESS) {
		mc_log_error("mdns_start failed: %d\n", err);
		goto bail;
	}
	mdns->iface_handle = is_uap_started() ? net_get_uap_handle() : net_get_sta_handle();
	mdns->this = xsThis;
	mdns->started = 1;
	mdns->configured = 0;
#if WMSDK_VERSION < 3001016
	mdns->num_services = 0;
	mdns->services[0] = NULL;
#endif
	xsRemember(mdns->this);
	return;
bail:
	if (mdns->domain != NULL)
		mc_free(mdns->domain);
	if (mdns->hostname != NULL)
		mc_free(mdns->hostname);
}

void
xs_mdns_stop(xsMachine *the)
{
	mc_mdns_t *mdns = &mc_mdns;

	if (!mdns->started)
		return;
	if (mdns->domain != NULL)
		mc_free(mdns->domain);
	if (mdns->hostname != NULL)
		mc_free(mdns->hostname);
#if WMSDK_VERSION < 3001016
	mdns_remove_service_iface(mdns->iface_handle);	/* this broadcasts "good bye" */
#endif
	mdns_stop();
	mdns->started = 0;
	xsForget(mdns->this);
}

void
xs_mdns_reannounce(xsMachine *the)
{
	mc_mdns_t *mdns = &mc_mdns;
	int err;

	if ((err = mdns_iface_state_change(mdns->iface_handle, mdns->configured ? REANNOUNCE : UP)) != WM_SUCCESS) {
		mc_log_error("mdns_iface_state_change failed:% d\n", err);
	}
	mdns->configured = 0;
}

void
xs_mdns_newService(xsMachine *the)
{
	mc_mdns_t *mdns = &mc_mdns;
	mc_mdns_service_t *service = xsGetHostData(xsArg(0));
#if WMSDK_VERSION < 3001016
	struct mdns_service_config config;
#endif

#if WMSDK_VERSION < 3001016
	if (mdns->num_services >= MDNS_MAX_SERVICES)
		mc_xs_throw(the, "MDNS: too many services!");
	mdns->svcbuf[mdns->num_services] = service;
	mdns->services[mdns->num_services] = &service->svc;
	mdns->services[++mdns->num_services] = NULL;
	config.iface_handle = mdns->iface_handle;
	config.services = mdns->services;
	if (mdns_add_service_iface(config) != WM_SUCCESS)
		mc_xs_throw(the, "mdns_add_service_iface failed");
#else
	if (mdns_announce_service(&service->svc, mdns->iface_handle) != WM_SUCCESS)
		mc_xs_throw(the, "mdns_announce_service failed");
#endif
	mdns->configured = false;
}

void
xs_mdns_removeService(xsMachine *the)
{
#if WMSDK_VERSION >= 3001016
	mc_mdns_t *mdns = &mc_mdns;
	mc_mdns_service_t *service = xsGetHostData(xsArg(0));
	int err;

	/*
	mdns_iface_state_change(mdns->iface_handle, DOWN);
	mdns_iface_state_change(mdns->iface_handle, UP);
	*/
	if ((err = mdns_deannounce_service(&service->svc, mdns->iface_handle)) != WM_SUCCESS) {
		mc_log_error("mdns_deannounce_service failed: %d", err);
		mc_xs_throw(the, "mdns_deannounce_service failed");
	}
#endif
}

static void
free_service(mc_mdns_service_t *service)
{
	struct mdns_service *svc = &service->svc;

	if (svc->servname != NULL) mc_free((void *)svc->servname);
	if (svc->servtype != NULL) mc_free((void *)svc->servtype);
	if (svc->domain != NULL) mc_free((void *)svc->domain);
	if (service->fqst != NULL) mc_free(service->fqst);
	if (service->txt != NULL) mc_free(service->txt);
	mc_free(service);
}

void
xs_mdns_service_constructor(xsMachine *the)
{
	mc_mdns_service_t *service = NULL;
	char *fqst = NULL;
	int port;
	char *type, *proto, *domain;
	struct mdns_service *svc;

	if (xsToInteger(xsArgc) < 3)
		return;
	if ((service = mc_calloc(1, sizeof(mc_mdns_service_t))) == NULL)
		goto nomem;
	if ((fqst = mc_strdup(xsToString(xsArg(0)))) == NULL)
		goto nomem;
	port = xsToInteger(xsArg(1));
	service->fqst = mc_strdup(fqst);
	type = strtok(fqst, ".");
	proto = strtok(NULL, ".");
	domain = strtok(NULL, ".");
	if (type == NULL) type = fqst;
	if (proto == NULL) proto = "_tcp";
	if (domain == NULL) domain = "local";
	svc = &service->svc;
	svc->servname = mc_strdup(xsToString(xsArg(2)));
	svc->servtype = *type == '_' ? mc_strdup(type + 1) : mc_strdup(type);
	svc->proto = strcmp(proto, "_tcp") == 0 ? MDNS_PROTO_TCP : MDNS_PROTO_UDP;
	svc->domain = mc_strdup(domain);
	svc->port = port;
	service->txt = mc_strdup("");
	if (service->fqst == NULL || service->txt == NULL || svc->servname == NULL || svc->servtype == NULL || svc->domain == NULL)
		goto nomem;
	mdns_set_txt_rec(svc, service->txt, ':');
	xsSetHostData(xsThis, service);
	mc_free(fqst);
	return;

nomem:
	mc_free(fqst);
	free_service(service);
	mc_xs_throw(the, "MDNS: no mem");
}

void
xs_mdns_service_configure(xsMachine *the)
{
	int ac = xsToInteger(xsArgc);
	mc_mdns_service_t *service = xsGetHostData(xsThis);
	struct mdns_service *svc = &service->svc;
	char separator = ':';

	if (ac > 1) {
		char *s = xsToString(xsArg(1));
		separator = s[0];
	}
	if (service->txt != NULL)
		mc_free(service->txt);
	service->txt = mc_strdup(xsToString(xsArg(0)));
	if (service->txt == NULL)
		mc_xs_throw(the, "MDNS: nomem");
	mdns_set_txt_rec(svc, service->txt, separator);
}

static void
dispose_service(mc_mdns_service_t *service)
{
#if WMSDK_VERSION < 3001016
	mc_mdns_t *mdns = &mc_mdns;
	int i;

	for (i = 0; i < mdns->num_services; i++) {
		if (service == mdns->svcbuf[i]) {
			for (; i < mdns->num_services; i++) {
				mdns->svcbuf[i] = mdns->svcbuf[i + 1];
				mdns->services[i] = mdns->services[i + 1];
			}
			--mdns->num_services;
			break;
		}
	}
#endif
	free_service(service);
}

void
xs_mdns_service_close(xsMachine *the)
{
	dispose_service(xsGetHostData(xsThis));
	xsSetHostData(xsThis, NULL);
}

void
xs_mdns_service_destructor(void *data)
{
	if (data != NULL)
		dispose_service(data);
}

#ifdef CONFIG_MDNS_QUERY

typedef struct {
	xsMachine *the;
	xsSlot this;
} mc_mdns_query_t;

static mc_mdns_query_t wm_query = {0};

static int
query_callback(void *data, const struct mdns_service *s, int status)
{
	mc_mdns_query_t *query = data;
	char *fqst;
	int len;
	struct in_addr in;
	char ports[6];
	char *id;

	switch (status) {
	case MDNS_DISCOVERED:
	case MDNS_UPDATED:
		id = "onFound";
		break;
	case MDNS_DISAPPEARED:
		id = "onLost";
		break;
	default:
		wm_log_debug("MDNS: query_callback: status = %d\n", status);
		return WM_SUCCESS;
	}
	len = strlen(s->servtype) + 1 + sizeof("_tcp") + 1 + strlen(s->domain);
	if ((fqst = mc_malloc(len)) == NULL) {
		mc_log_error("MDNS: query_callback: no mem\n");
		return WM_SUCCESS;
	}
	snprintf(fqst, len, "%s.%s.%s", s->servtype, s->proto == MDNS_PROTO_TCP ? "_tcp" : "_udp", s->domain);
	in.s_addr = s->ipaddr;
	snprintf(ports, sizeof(ports), "%d", s->port);
	xsBeginHost(query->the);
	xsTry {
		xsVars(5);
		xsSetString(xsVar(0), fqst);
		mc_free(fqst);
		xsSetString(xsVar(1), inet_ntoa(in));
		xsSetString(xsVar(2), ports);
		if (s->keyvals != NULL)
			xsSetString(xsVar(3), s->keyvals);
		else
			xsSetUndefined(xsVar(3));
		xsSetString(xsVar(4), (xsStringValue)s->servname);
		xsCall_noResult(query->this, xsID(id), &xsVar(0), &xsVar(1), &xsVar(2), &xsVar(3), &xsVar(4), NULL);
	} xsCatch {
		mc_log_error("MDNS: query_callback: cought an error sigal\n");
	}
	xsEndHost(query->the);
	return WM_SUCCESS;	/* return SUCCESS to keep monitoring the service */
}

void
xs_mdns_query(xsMachine *the)
{
	char *fqst = xsToString(xsArg(0));
	Boolean stop = xsToInteger(xsArgc) > 1 && xsTest(xsArg(1));
	int err;

	if (stop) {
		mdns_query_unmonitor(fqst);
		xsSetTrue(xsResult);
	}
	else {
		wm_query.the = the;
		wm_query.this = xsThis;
		xsRemember(wm_query.this);
		if ((err = mdns_query_monitor(fqst, query_callback, &wm_query)) != WM_SUCCESS) {
			wm_log_error("mdns_query_monitor failed: %d\n", err);
			xsSetFalse(xsResult);
		}
		else
			xsSetTrue(xsResult);
	}
}

void
xs_mdns_close(xsMachine *the)
{
	xsForget(wm_query.this);
}

#else /* CONFIG_MDNS_QUERY */
void
xs_mdns_query(xsMachine *the)
{
}

void
xs_mdns_close(xsMachine *the)
{
}
#endif /* CONFIG_MDNS_QUERY */
