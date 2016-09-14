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
#include "mc_ipc.h"
#include "mc_module.h"
#include "mc_wifi.h"
#if mxMC
#include <lwip/netdb.h>
#include <lwip/inet.h>
#include <lwip/dns.h>
#include <lwip/tcpip.h>
#else
#include "mc_compat.h"
#define LWIP_IPV6	1
#endif

#if !XS_ARCHIVE
#include "xm_socket.xs.c"
MC_MOD_DECL(socket);
#endif

#define MC_SOCKET_MAX_BUF	(8*1024)
#define MC_SOCKET_SBUF_SIZE	128
#ifndef MIN
#define MIN(a, b)	((a) < (b) ? (a) : (b))
#endif

struct mc_socket {
	int s;
	struct in_addr addr;
	int port;
	int type;
	struct sockaddr_in sin, peer;
	enum {SOCK_STATE_INIT = 0, SOCK_STATE_RESOLVING, SOCK_STATE_CONNECTING, SOCK_STATE_CONNECTED, SOCK_STATE_CLOSING} state;
	struct mc_sbuf {
		uint8_t buf[MC_SOCKET_SBUF_SIZE];
		uint8_t *bp;
		struct sockaddr_in sin;
		struct mc_sbuf *next;
	} *sbuf;
	xsMachine *the;
	xsSlot this;
};

extern void xs_socket_destructor(void *data);

static size_t
mc_socket_buflen(struct mc_socket *sock)
{
	struct mc_sbuf *sp;
	size_t ttl = 0;

	for (sp = sock->sbuf; sp != NULL; sp = sp->next)
		ttl += sp->bp - sp->buf;
	return ttl;
}

static int
socket_flush_queue(struct mc_socket *sock)
{
	while (sock->sbuf != NULL) {
		struct mc_sbuf *sp = sock->sbuf;
		size_t sz = sp->bp - sp->buf;
		int n = lwip_sendto(sock->s, sp->buf, sz, 0, (struct sockaddr *)&sp->sin, sizeof(sp->sin));
		if (n < 0) {
			if (errno == EAGAIN)
				break;
			mc_log_error("socket: sendto failed: %d\n", errno);
			return -1;
		}
		if ((size_t)n < sz) {
			memmove(sp->buf, sp->buf + n, sz - n);
			sp->bp -= n;
			break;
		}
		sock->sbuf = sp->next;
		mc_free(sp);
	}
	return 0;
}

static void
mc_socket_callback(int s, unsigned int flags, void *closure)
{
	struct mc_socket *sock = closure;

	// mc_log_debug("socket: mc_socket_callback STATE:%d\n", sock->state);
	xsBeginHost(sock->the);
	xsVars(1);
	if (flags & MC_SOCK_WRITE) {
		switch (sock->state) {
		case SOCK_STATE_CONNECTING: {
			int serror = 0;
			socklen_t len = sizeof(serror);
			// mc_log_debug("socket: mc_socket_callback SOCK_STATE_CONNECTING\n");

			if (lwip_getsockopt(s, SOL_SOCKET, SO_ERROR, &serror, &len) != 0 || serror != 0) {
				mc_log_debug("socket: getsockopt SO_ERROR: %d, %d\n", errno, serror);
				xsCall_noResult(sock->this, xsID("onError"), NULL);
			}
			else
				xsCall_noResult(sock->this, xsID("onConnect"), NULL);
			sock->state = SOCK_STATE_CONNECTED;
			break;
		}
		case SOCK_STATE_CONNECTED:
			if (socket_flush_queue(sock) != 0)
				xsCall_noResult(sock->this, xsID("onError"), NULL);
			if (sock->sbuf == NULL) {
				xsIndex id_onWritable = xsID("onWritable");
				mc_event_register(s, MC_SOCK_READ, mc_socket_callback, closure);	/* turn off the WRITE bit */
				if (xsHas(sock->this, id_onWritable)) {
					xsGet(xsVar(0), sock->this, id_onWritable);
					if (xsIsInstanceOf(xsVar(0), xsFunctionPrototype)) {
						xsSetInteger(xsVar(0), MC_SOCKET_MAX_BUF);
						xsCall_noResult(sock->this, id_onWritable, &xsVar(0), NULL);
					}
				}
			}
			break;
		case SOCK_STATE_RESOLVING:
			break;

		default:
			mc_fatal("socket: bad status: %d\n", sock->state);
			break;
		}
	}
	if (flags & MC_SOCK_READ) {
#if mxMC
		unsigned short n;
#else
		int n;
#endif
		if (lwip_ioctl(s, FIONREAD, &n) < 0) {
			mc_log_error("socket: lwip_ioctl failed: %d\n", errno);
			xsCall_noResult(sock->this, xsID("onError"), NULL);
		}
		else if (n == 0) {	/* only in the case of TCP */
			// mc_log_debug("FIONREAD: 0\n");
			xsCall_noResult(sock->this, xsID("onClose"), NULL);
		}
		else {
			// mc_log_debug("FIONREAD: %d, calling onMessage...\n", n);
			xsSetInteger(xsVar(0), n);
			xsCall_noResult(sock->this, xsID("onMessage"), &xsVar(0), NULL);
		}
	}
	if (flags == MC_SOCK_CLOSE) {
		xsCall_noResult(sock->this, xsID("onClose"), NULL);
	}
	xsEndHost(sock->the);
}

static int
mc_socket_connect(struct mc_socket *sock)
{
	if (lwip_connect(sock->s, (struct sockaddr *)&sock->sin, sizeof(sock->sin)) == 0 || errno == EINPROGRESS) {
		sock->state = SOCK_STATE_CONNECTING;	/* even if the connection has been established, leave everything to the event callback */
		return 0;
	}
	else {
		mc_log_error("lwip_connect failed: %d\n", errno);
		return -1;
	}
}

void
xs_socket_constructor(xsMachine *the)
{
	int ac = xsToInteger(xsArgc);
	char *host, *proto;
	int port;
	int sock_type, sock_proto;
	int s = -1;
	int flags;
	struct in_addr iaddr;
	struct mc_socket *sock = NULL;
	xsIndex id_host = xsID("host");
	xsIndex id_port = xsID("port");
	xsIndex id_proto = xsID("proto");
	xsIndex id_multicast = xsID("multicast");
	xsIndex id_ttl = xsID("ttl");
	xsIndex id_loop = xsID("loop");
#define ERROR(...)	do {if (s >= 0) lwip_close(s); if (sock != NULL) mc_free(sock); mc_xs_throw(the, __VA_ARGS__);} while (0)

	xsVars(1);
	if (ac < 1 || !xsTest(xsArg(0)))
		return;		/* make an empty socket */
	if (!xsHas(xsArg(0), id_proto))
		goto badarg;
	xsGet(xsVar(0), xsArg(0), id_proto);
	proto = xsToString(xsVar(0));
	if (strcmp(proto, "tcp") == 0) {
		sock_type = SOCK_STREAM;
		sock_proto = IPPROTO_TCP;
	}
	else if (strcmp(proto, "udp") == 0) {
		sock_type = SOCK_DGRAM;
		sock_proto = IPPROTO_UDP;
	}
	else
		goto badarg;
	if (xsHas(xsArg(0), id_port)) {
		xsGet(xsVar(0), xsArg(0), id_port);
		port = xsToInteger(xsVar(0));
	}
	else
		port = -1;
	if (xsHas(xsArg(0), id_host)) {
		xsGet(xsVar(0), xsArg(0), id_host);
		host = xsToString(xsVar(0));
		xsSet(xsThis, id_host, xsVar(0));
	}
	else
		host = NULL;

	if ((s = lwip_socket(AF_INET, sock_type, sock_proto)) < 0)
		ERROR("socket: cannot create socket: %d", errno);
	/* set the socket properties */
	flags = lwip_fcntl(s, F_GETFL, 0);
	lwip_fcntl(s, F_SETFL, flags | O_NONBLOCK);
	iaddr.s_addr = 0;
	if (xsHas(xsArg(0), id_multicast)) {
		xsGet(xsVar(0), xsArg(0), id_multicast);
		if (!inet_aton(xsToString(xsVar(0)), &iaddr))
			ERROR("socket: illegal multicast address: %s", xsToString(xsVar(0)));
#if mxMC
		if (host != NULL) {
			struct in_addr addr;
			if (inet_aton(host, &addr))
				mc_wifi_add_mcast_filter(&addr);
		}
#endif
		if (lwip_setsockopt(s, IPPROTO_IP, IP_MULTICAST_IF, &iaddr, sizeof(struct in_addr)) != 0)
			ERROR("socket: IP_MULTICAST_IF failed: %d", errno);
	}
	if (xsHas(xsArg(0), id_ttl)) {
		int ttl;
		xsGet(xsVar(0), xsArg(0), id_ttl);
		ttl = xsToInteger(xsVar(0));
		if (lwip_setsockopt(s, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl)) != 0)
			ERROR("socket: IP_MULTICAST_TTL failed: %d", errno);
	}
	if (xsHas(xsArg(0), id_loop)) {
		unsigned char loop;
		xsGet(xsVar(0), xsArg(0), id_loop);
		loop = (unsigned char)xsToBoolean(xsVar(0));
		if (lwip_setsockopt(s, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop)) != 0)
			ERROR("socket: IP_MULTICAST_LOOP failed: %d", errno);
	}
#if mxMacOSX
	{
		int set = 1;
		setsockopt(s, SOL_SOCKET, SO_NOSIGPIPE, (void *)&set, sizeof(int));
	}
#endif

	if ((sock = mc_calloc(1, sizeof(struct mc_socket))) == NULL)
		ERROR("socket: no mem");
	sock->s = s;
	sock->port = port;
	sock->type = sock_type;
	sock->addr = iaddr;
	sock->state = host != NULL && port >= 0 ? SOCK_STATE_RESOLVING : SOCK_STATE_INIT;
	sock->sbuf = NULL;
	sock->the = the;
	sock->this = xsThis;

	if (sock_type == SOCK_DGRAM) {
		if (sock->state == SOCK_STATE_RESOLVING) {
			mc_event_register(s, MC_SOCK_READ, mc_socket_callback, sock);
		} else {
			sock->state = SOCK_STATE_CONNECTING;
			mc_event_register(s, MC_SOCK_READ | MC_SOCK_WRITE, mc_socket_callback, sock);
		}
	} else {
		mc_event_register(s, MC_SOCK_READ | MC_SOCK_WRITE, mc_socket_callback, sock);
	}

	xsSetHostData(xsThis, sock);
	return;

badarg:
	ERROR("socket: bad arg");
}

static void
mc_socket_accept_callback(int s, unsigned int flags, void *closure)
{
	struct mc_socket *sock = closure;

	xsBeginHost(sock->the);
	if (flags & MC_SOCK_READ)
		xsCall_noResult(sock->this, xsID("onConnect"), NULL);
	else
		xsCall_noResult(sock->this, xsID("onClose"), NULL);
	xsEndHost(sock->the);
}

void
xs_socket_listeningSocket(xsMachine *the)
{
	int ac = xsToInteger(xsArgc);
	char *addr, *proto;
	int port;
	int sock_type, sock_proto;
	int s;
	struct sockaddr_in sin;
#if LWIP_IPV6
	struct sockaddr_in6 sin6;
#endif
	struct sockaddr *saddr;
	int saddrlen;
	struct mc_socket *sock;
	struct in_addr iaddr;
	int reuseaddr = 1;
	int ipv6 = 0;
	xsIndex id_addr = xsID("addr");
	xsIndex id_proto = xsID("proto");
	xsIndex id_port = xsID("port");
	xsIndex id_membership = xsID("membership");
	xsIndex id_ttl = xsID("ttl");
	xsIndex id_loop = xsID("loop");
#if LWIP_IPV6
	xsIndex id_ipv6 = xsID("ipv6");
#endif

	xsVars(1);
	if (ac < 1)
		goto badarg;
	if (!xsHas(xsArg(0), id_proto))
		goto badarg;
	xsGet(xsVar(0), xsArg(0), id_proto);
	proto = xsToString(xsVar(0));
	if (strcmp(proto, "tcp") == 0) {
		sock_type = SOCK_STREAM;
		sock_proto = IPPROTO_TCP;
	}
	else if (strcmp(proto, "udp") == 0) {
		sock_type = SOCK_DGRAM;
		sock_proto = IPPROTO_UDP;
	}
	else
		goto badarg;
	if (!xsHas(xsArg(0), id_port))
		goto badarg;
	xsGet(xsVar(0), xsArg(0), id_port);
	port = xsToInteger(xsVar(0));
#if LWIP_IPV6
	if (xsHas(xsArg(0), id_ipv6)) {
		xsGet(xsVar(0), xsArg(0), id_ipv6);
		ipv6 = xsTest(xsVar(0));
	}
#endif
	if ((s = lwip_socket(ipv6 ? AF_INET6 : AF_INET, sock_type, sock_proto)) < 0)
		mc_xs_throw(the, "socket: cannot create socket");
	(void)lwip_setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof(reuseaddr));
#if mxMacOSX
	{
		int set = 1;
		setsockopt(s, SOL_SOCKET, SO_NOSIGPIPE, (void *)&set, sizeof(int));
	}
#endif
	if (xsHas(xsArg(0), id_addr)) {
		xsGet(xsVar(0), xsArg(0), id_addr);
		addr = xsToString(xsVar(0));
	}
	else
		addr = NULL;
	if (ipv6) {
#if LWIP_IPV6
		memset(&sin6, 0, sizeof(sin6));
		sin6.sin6_family = AF_INET6;
		sin6.sin6_port = htons(port);
		if (addr == NULL)
			sin6.sin6_addr = in6addr_any;
		else
			inet_pton(AF_INET6, addr, &sin6.sin6_addr);
		saddr = (struct sockaddr *)&sin6;
		saddrlen = sizeof(sin6);
#endif
	}
	else {
		memset(&sin, 0, sizeof(sin));
		sin.sin_family = AF_INET;
		sin.sin_port = htons(port);
		if (addr == NULL)
			sin.sin_addr.s_addr = htonl(INADDR_ANY);
		else
			inet_aton(addr, &sin.sin_addr);
#if mxMC
		if (xsHas(xsArg(0), id_membership)) {	/* looks like LWIP accepts only IADDR_ANY for the broadcast addr */
			static struct sockaddr_in any;
			any = sin;
			any.sin_addr.s_addr = htonl(INADDR_ANY);
			saddr = (struct sockaddr *)&any;
		}
		else
#endif
			saddr = (struct sockaddr *)&sin;
		saddrlen = sizeof(sin);
	}
	if (lwip_bind(s, saddr, saddrlen) < 0) {
		lwip_close(s);
		mc_xs_throw(the, "socket: bind failed: %d", errno);
	}

	iaddr.s_addr = 0;
	if (sock_type == SOCK_STREAM) {
		xsIndex id_nlistner = xsID("nlistner");
		int nlistener = 1;
		if (xsHas(xsArg(0), id_nlistner)) {
			xsGet(xsVar(0), xsArg(0), id_nlistner);
			nlistener = xsToInteger(xsVar(0));
		}
		(void)lwip_listen(s, nlistener);
	}
	else {
		if (xsHas(xsArg(0), id_membership)) {
			struct ip_mreq maddr;
			xsGet(xsVar(0), xsArg(0), id_membership);
			maddr.imr_interface.s_addr = inet_addr(xsToString(xsVar(0)));
			maddr.imr_multiaddr.s_addr = sin.sin_addr.s_addr;
#if mxMC
			mc_wifi_add_mcast_filter(&maddr.imr_multiaddr);
#endif
			if (lwip_setsockopt(s, IPPROTO_IP, IP_ADD_MEMBERSHIP, &maddr, sizeof(struct ip_mreq)) != 0) {
				lwip_close(s);
				mc_xs_throw(the, "socket: IP_ADD_MEMBERSHIP failed: %d", errno);
			}
			if (lwip_setsockopt(s, IPPROTO_IP, IP_MULTICAST_IF, &maddr.imr_interface, sizeof(struct in_addr)) != 0) {
				lwip_close(s);
				mc_xs_throw(the, "socket: IP_MULTICAST_IF failed: %d", errno);
			}
			iaddr = maddr.imr_interface;
		}
		if (xsHas(xsArg(0), id_ttl)) {
			int ttl;
			xsGet(xsVar(0), xsArg(0), id_ttl);
			ttl = xsToInteger(xsVar(0));
			if (lwip_setsockopt(s, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl)) != 0) {
				lwip_close(s);
				mc_xs_throw(the, "socket: IP_MULTICAST_TTL failed: %d", errno);
			}
		}
		if (xsHas(xsArg(0), id_loop)) {
			unsigned char loop;
			xsGet(xsVar(0), xsArg(0), id_loop);
			loop = (unsigned char)xsToBoolean(xsVar(0));
			if (lwip_setsockopt(s, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop)) != 0) {
				lwip_close(s);
				mc_xs_throw(the, "socket: IP_MULTICAST_LOOP failed: %d", errno);
			}
		}
	}

	if ((sock = mc_malloc(sizeof(struct mc_socket))) == NULL) {
		lwip_close(s);
		mc_xs_throw(the, "socket: no mem");
	}
	sock->s = s;
	sock->sin = sin;
	sock->addr = iaddr;
	sock->peer = sin;
	sock->port = port;
	sock->state = SOCK_STATE_CONNECTED;
	sock->sbuf = NULL;
	sock->the = the;
	sock->this = xsThis;
	xsSetHostData(xsThis, sock);
	mc_event_register(s, MC_SOCK_READ, sock_type == SOCK_STREAM ? mc_socket_accept_callback : mc_socket_callback, sock);
	return;

badarg:
	mc_xs_throw(the, "socket: bad args");
}

static void
socket_free(struct mc_socket *sock)
{
	while (sock->sbuf != NULL) {
		struct mc_sbuf *sp = sock->sbuf;
		sock->sbuf = sp->next;
		mc_free(sp);
	}
	if (sock->s >= 0) {
		mc_event_unregister(sock->s);
		lwip_close(sock->s);
		sock->s = -1;
	}
}

void
xs_socket_destructor(void *data)
{
	struct mc_socket *sock = data;

	if (sock != NULL) {
		if (sock->state == SOCK_STATE_RESOLVING) {
			sock->state = SOCK_STATE_CLOSING;
			return;
		}
		socket_free(sock);
		mc_free(sock);
	}
}

static int
mc_socket_send_element(xsMachine *the, struct mc_socket *sock, xsSlot *slot, struct sockaddr_in *sin)
{
	size_t datasize;
	void *data;
	uint8_t intdata;
	int n;

	xsVars(1);
	switch (xsTypeOf(*slot)) {
	case xsIntegerType:
		datasize = 1;
		intdata = (uint8_t)xsToInteger(*slot);
		data = &intdata;
		break;
	case xsStringType:
		data = xsToString(*slot);
		datasize = strlen(data);
		break;
	case xsReferenceType:
		if (xsIsInstanceOf(*slot, xsArrayPrototype)) {
			xsGet(xsVar(0), *slot, xsID("length"));
			int len = xsToInteger(xsVar(0)), j;
			for (j = 0, n = 0; j < len; j++) {
				xsGet(xsVar(0), *slot, j);
				if ((n = mc_socket_send_element(the, sock, &xsVar(0), sin)) < 0) {
					if (j != 0)
						mc_xs_throw(the, "socket: partially written");
					/* else nothing has been written or buffered */
					return -1;
				}
			}
			return n;
		}
		else {	/* assume it's an ArrayBuffer */
			datasize = xsGetArrayBufferLength(*slot);
			data = xsToArrayBuffer(*slot);
		}
		break;
	default:
		mc_xs_throw(the, "socket: bad args");
		return -1;
	}
	n = 0;
	/* try to flush the queue anyway */
	if (socket_flush_queue(sock) != 0)
		xsCall_noResult(sock->this, xsID("onError"), NULL);
	if (sock->sbuf == NULL) {
		if (sin == NULL)
			sin = &sock->sin;
		if ((n = lwip_sendto(sock->s, data, datasize, 0, (struct sockaddr *)sin, sizeof(struct sockaddr_in))) < 0) {
			if (errno != EAGAIN) {
				mc_log_error("socket: sendto failed: %d\n", errno);
				xsCall_noResult(sock->this, xsID("onError"), NULL);
			}
			return -1;
		}
	}
	if ((size_t)n < datasize) {
		struct mc_sbuf *sp, **spp;
		size_t sz;
		datasize -= n;
		data += n;
		if (mc_socket_buflen(sock) + datasize > MC_SOCKET_MAX_BUF)
			goto nomem;
		for (spp = &sock->sbuf; *spp != NULL; spp = &(*spp)->next)
			;
		sp = *spp;
		if (sp != NULL && (sz = sp->buf + MC_SOCKET_SBUF_SIZE - sp->bp) > 0) {
			sz = MIN(datasize, sz);
			memcpy(sp->bp, data, sz);
			sp->bp += sz;
			datasize -= sz;
			data += sz;
		}
		while (datasize > 0) {
			if ((sp = mc_malloc(sizeof(struct mc_sbuf))) == NULL)
				goto nomem;
			sz = MIN(datasize, MC_SOCKET_SBUF_SIZE);
			memcpy(sp->buf, data, sz);
			sp->bp = sp->buf + sz;
			sp->next = NULL;
			sp->sin = sin != NULL ? *sin : sock->sin;
			datasize -= sz;
			data += sz;
			*spp = sp;
			spp = &sp->next;
		}
		return 0;
	}
	return 1;

nomem:
	if (n > 0)	/* partially written... can't take it back... */
		mc_xs_throw(the, "socket: no mem");
	return -1;
}

void
xs_socket_write(xsMachine *the)
{
	struct mc_socket *sock = xsGetHostData(xsThis);
	int ac = xsToInteger(xsArgc), i;

	for (i = 0; i < ac; i++) {
		if (mc_socket_send_element(the, sock, &xsArg(i), NULL) < 0)
			break;
	}
	xsSetInteger(xsResult, i);
	if (sock->s >= 0)
		mc_event_register(sock->s, MC_SOCK_READ | MC_SOCK_WRITE, mc_socket_callback, sock);
}

void
xs_socket_send(xsMachine *the)
{
	struct mc_socket *sock = xsGetHostData(xsThis);
	int ac = xsToInteger(xsArgc);
	struct sockaddr_in sin;
	int ret;


	if (ac < 1)
		return;
	else if ((ac > 1) && xsTest(xsArg(1))) {
		char buf[sizeof("xxx.xxx.xxx.xxx:65535")];
		char *addr = xsToStringBuffer(xsArg(1), buf, sizeof(buf)), *p = NULL;
		sin.sin_family = AF_INET;
		if ((p = strchr(addr, ':')) != NULL) {
			*p = '\0';
			sin.sin_port = atoi(p + 1);
			sin.sin_port = htons(sin.sin_port);
		}
		else
			sin.sin_port = sock->sin.sin_port;
		sin.sin_addr.s_addr = inet_addr(addr);
		if (p) *p = ':';
	}
	else
		sin = sock->sin;
	ret = mc_socket_send_element(the, sock, &xsArg(0), &sin) >= 0 ? 1 : 0;
	xsSetInteger(xsResult, ret);
	if (sock->s >= 0)
		mc_event_register(sock->s, MC_SOCK_READ | MC_SOCK_WRITE, mc_socket_callback, sock);
}

void
xs_socket_recv(xsMachine *the)
{
	struct mc_socket *sock = xsGetHostData(xsThis);
	int ac = xsToInteger(xsArgc);
	int sz, n;
	struct sockaddr_in sin;
	socklen_t slen = sizeof(sin);

	if (ac < 1)
		return;
	sz = xsToInteger(xsArg(0));
	if (sz <= 0)
		return;	// undefined
	if (ac > 1 && xsTypeOf(xsArg(1)) == xsReferenceType) {
		xsResult = xsArg(1);
		n = xsGetArrayBufferLength(xsResult);
		if (n < sz)
			sz = n;
	}
	else
		xsResult = xsArrayBuffer(NULL, sz);
	n = lwip_recvfrom(sock->s, xsToArrayBuffer(xsResult), sz, 0, (struct sockaddr *)&sin, &slen);
	if (n <= 0) {
		if (errno != EWOULDBLOCK && n < 0) {
			mc_log_error("socket: lwip_recvfrom error: %d\n", errno);
			xsCall_noResult(sock->this, xsID("onError"), NULL);
		}
		xsSetNull(xsResult);
	}
	else if (n != sz)
		xsSetArrayBufferLength(xsResult, n);
	if (slen >= sizeof(sin))
		sock->peer = sin;
	if (sock->s >= 0)
		mc_event_register(sock->s, MC_SOCK_READ | MC_SOCK_WRITE, mc_socket_callback, sock);
}

void
xs_socket_accept(xsMachine *the)
{
	struct mc_socket *sock = xsGetHostData(xsThis), *nsock;
	int ns;
	struct sockaddr_in sin;
	socklen_t slen = sizeof(sin);
	int flags;

	if ((ns = lwip_accept(sock->s, (struct sockaddr *)&sin, &slen)) < 0)
		return;
	if ((nsock = mc_malloc(sizeof(struct mc_socket))) == NULL) {
		lwip_close(ns);
		return;
	}
	flags = lwip_fcntl(ns, F_GETFL, 0);
	lwip_fcntl(ns, F_SETFL, flags | O_NONBLOCK);
	nsock->s = ns;
	nsock->addr = sock->addr;
	nsock->peer = sock->sin = sin;
	nsock->port = ntohs(sin.sin_port);
	nsock->state = SOCK_STATE_CONNECTED;
	nsock->sbuf = NULL;
	nsock->the = the;
	nsock->this = xsArg(0);
	xsSetHostData(xsArg(0), nsock);
	mc_event_register(ns, MC_SOCK_READ | MC_SOCK_WRITE, mc_socket_callback, nsock);
#if mxMacOSX
	{
		int set = 1;
		setsockopt(ns, SOL_SOCKET, SO_NOSIGPIPE, (void *)&set, sizeof(int));
	}
#endif
}

void
xs_socket_connect(xsMachine *the)
{
	struct mc_socket *sock = xsGetHostData(xsThis);

	if (sock->state == SOCK_STATE_CLOSING) {
		xs_socket_destructor(sock);
		return;
	}
	if (sock->s < 0)	/* already closed */
		return;	/* can't do anything here */
	if (xsToInteger(xsArgc) < 1 || !xsTest(xsArg(0)) || inet_aton(xsToString(xsArg(0)), &sock->sin.sin_addr) == 0) {
		/* couldn't resolve */
		xsCall_noResult(sock->this, xsID("onError"), NULL);
		return;
	}
	sock->sin.sin_family = AF_INET;
	sock->sin.sin_port = htons(sock->port);
	if (sock->type == SOCK_STREAM) {
		if (mc_socket_connect(sock) != 0) {
			xsCall_noResult(sock->this, xsID("onError"), NULL);
			return;
		}
	}
	else {
		sock->state = SOCK_STATE_CONNECTING;
		mc_event_register(sock->s, MC_SOCK_READ | MC_SOCK_WRITE, mc_socket_callback, sock);
	}
}

void
xs_socket_close(xsMachine *the)
{
	struct mc_socket *sock = xsGetHostData(xsThis);

	socket_free(sock);
}

void
xs_socket_flush(xsMachine *the)
{
	/* do nothing */
}

void
xs_socket_getAddr(xsMachine *the)
{
	struct mc_socket *sock = xsGetHostData(xsThis);

	xsSetString(xsResult, inet_ntoa(sock->addr));
}

void
xs_socket_getPort(xsMachine *the)
{
	struct mc_socket *sock = xsGetHostData(xsThis);

	xsSetInteger(xsResult, sock->port);
}

void
xs_socket_getPeerAddr(xsMachine *the)
{
	struct mc_socket *sock = xsGetHostData(xsThis);

	xsSetString(xsResult, inet_ntoa(sock->peer.sin_addr));
}

void
xs_socket_getPeerPort(xsMachine *the)
{
	struct mc_socket *sock = xsGetHostData(xsThis);

	xsSetInteger(xsResult, ntohs(sock->peer.sin_port));
}

void
xs_socket_getBytesAvailable(xsMachine *the)
{
	struct mc_socket *sock = xsGetHostData(xsThis);
#if mxMC
	unsigned short n;
#else
	int n;
#endif
	if (lwip_ioctl(sock->s, FIONREAD, &n) < 0) {
		mc_log_error("socket: lwip_ioctl failed: %d\n", errno);
		n = 0;
	}
	xsSetInteger(xsResult, n);
}

void
xs_socket_getBytesWritable(xsMachine *the)
{
	struct mc_socket *sock = xsGetHostData(xsThis);
	int n;

	n = MC_SOCKET_MAX_BUF - mc_socket_buflen(sock);
	xsSetInteger(xsResult, n < 0 ? 0 : n);
}

void
xs_socket_getNativeSocket(xsMachine *the)
{
	struct mc_socket *sock = xsGetHostData(xsThis);

	xsSetInteger(xsResult, sock->s);
}

void
xs_socket_aton(xsMachine *the)
{
	struct in_addr addr;

	if (inet_aton(xsToString(xsArg(0)), &addr))
		xsResult = xsArrayBuffer(&addr, sizeof(addr));
}

void
xs_socket_ntoa(xsMachine *the)
{
	void *data = xsToArrayBuffer(xsArg(0));
	size_t size = xsGetArrayBufferLength(xsArg(0));
	struct in_addr addr;

	if (size < sizeof(struct in_addr))
		return;	// undefined
	memcpy(&addr, data, sizeof(addr));
	xsSetString(xsResult, inet_ntoa(addr));
}
