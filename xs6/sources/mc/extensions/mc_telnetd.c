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
#include "mc_event.h"
#include "mc_xs.h"
#include "mc_telnetd.h"
#if !mxMC
#include "mc_compat.h"
#endif

#ifndef MAX_MSG_LEN
#define MAX_MSG_LEN	128
#endif

typedef struct telnet {
	int sock;
	int echo, eof;
	int command;
	unsigned char buf[MAX_MSG_LEN], *bp, *bufend;
	xsMachine *the;
	xsSlot this;
} telnet_t;

static int telnet_sock = -1;

#define CTRL(c)	(c - 'A' + 1)

static void
telnet_process_command(telnet_t *telnet, int c)
{
	switch (telnet->command) {
	case 250:	/* subnegotiation */
		break;	/* just ignore for now */
	case 240:	/* end of subnegotiation */
		telnet->command = 0;
		break;
	case 251:	/* WILL DO */
		switch (c) {
		case 34:	/* linemode */
			telnet->echo = 0;
			break;
		default:
			break;
		}
		telnet->command = 0;
		break;
	case 254:	/* DON'T */
		switch (c) {
		case 1:		/* echo */
			telnet->echo = 0;
			break;
		default:
			break;
		}
		telnet->command = 0;
		break;
	case 236:	/* EOF in the linemode */
		telnet->eof++;
		telnet->command = 0;
		break;
	default:
		telnet->command = 0;
		break;
	}
}

static int
telnet_getter(telnet_t *telnet)
{
	unsigned char c = '\0';
#if mxMC
	unsigned short n, i;
#else
	int n, i;
#endif

	if (lwip_ioctl(telnet->sock, FIONREAD, &n) < 0)
		return EOF;
	for (i = 0; i < n; i++) {
		if (lwip_read(telnet->sock, &c, 1) != 1)
			break;
		if (c == 255)
			telnet->command = -1;
		else if (telnet->command) {
			if (telnet->command == -1)
				telnet->command = c;
			else
				telnet_process_command(telnet, c);
		}
		else {
			if (c == '\b') {
				if (telnet->bp > telnet->buf)
					--telnet->bp;
			}
			else if (c == CTRL('D'))
				telnet->eof++;
			else if (c == '\r' || c == '\n')
				break;
			if (telnet->bp < telnet->bufend) {
				*telnet->bp++ = c;
				if (telnet->echo)
					lwip_write(telnet->sock, &c, 1);
			}
		}
	}
	return telnet->eof ? EOF : c;
}

static void
telnet_close(telnet_t *telnet)
{
	xsBeginHost(telnet->the);
	{
		xsIndex id = xsID("onClose");
		if (xsHas(telnet->this, id))
			xsCall_noResult(telnet->this, id, NULL);
		xsSetHostData(telnet->this, NULL);
	}
	xsEndHost(telnet->the);
	telnetd_close(telnet);
}

static void
telnet_rep(int sock, unsigned int flags, void *closure)
{
	telnet_t *telnet = closure;
	int c;

	if (!(flags & MC_SOCK_READ)) {
		telnet_close(telnet);
		return;
	}

	if ((c = telnet_getter(telnet)) == EOF) {
		telnet_close(telnet);
		return;
	}
	else if (c == '\n' || c == '\r') {
		if (telnet->bp < telnet->bufend)
			*telnet->bp = '\0';
		else
			telnet->bp[-1] = '\0';
		xsBeginHost(telnet->the);
		xsIndex id = xsID("onExecute");
		if (xsHas(telnet->this, id)) {
			xsVars(1);
			xsSetString(xsVar(0), (char *)telnet->buf);
			xsCall_noResult(telnet->this, id, &xsVar(0), NULL);
		}
		xsEndHost(telnet->the);
		telnet->bp = telnet->buf;
	}
}

static int
telnet_putter(const char *str)
{
	int n = 0;

	if (telnet_sock >= 0) {
		if ((n = lwip_write(telnet_sock, str, strlen(str))) <= 0)
			mc_stdio_unregister(telnet_putter);
	}
	return n;
}

void *
telnetd_connect(int ns, xsMachine *the, xsSlot *this)
{
	telnet_t *telnet;

	if ((telnet = mc_malloc(sizeof(telnet_t))) == NULL)
		return NULL;
	telnet->sock = ns;
	telnet->echo = 0;
	telnet->eof = 0;
	telnet->command = 0;
	telnet->the = the;
	telnet->this = *this;
	telnet->bp = telnet->buf;
	telnet->bufend = telnet->bp + sizeof(telnet->buf);
	telnet_sock = ns;
	mc_stdio_register(telnet_putter);
	mc_event_register(ns, MC_SOCK_READ, telnet_rep, telnet);
	return telnet;
}

void
telnetd_close(void *data)
{
	telnet_t *telnet = data;

	mc_log_notice("telnetd: closing the connection\n");
	mc_stdio_unregister(telnet_putter);
	mc_event_unregister(telnet->sock);
	lwip_close(telnet->sock);
	telnet_sock = -1;
	mc_free(telnet);
}

#ifdef TELNETD_STANDALONE

static void *telnet_instance = NULL;
static int listening_socket = -1;

static void telnet_connect_callback(int s, unsigned int flags, void *closure)
{
	xsMachine *the = closure;
	int ns;
	struct sockaddr_in sin;
	socklen_t slen = sizeof(sin);

	if (flags & MC_SOCK_READ) {
		if ((ns = lwip_accept(s, (struct sockaddr *)&sin, &slen)) >= 0) {
			mc_log_notice("telnetd: accepted from %s\n", inet_ntoa(sin.sin_addr));
			telnet_instance = telnetd_connect(ns, the, NULL);
		}
	}
	else
		telnetd_stop();
}

int telnetd_start(xsMachine *the)
{
	int s;
	int reuseaddr = 1;
	struct sockaddr_in sin;

	if ((s = lwip_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
		mc_log_error("telnetd: lwip_socket failed\n");
		return -1;
	}
	if (lwip_setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof(reuseaddr)) != 0) {
		mc_log_error("telnetd: lwip_setsockopt failed\n");
		/* fall thru */
	}
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(23);
	sin.sin_addr.s_addr = htonl(INADDR_ANY);
	if (lwip_bind(s, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
		mc_log_error("telnetd: lwip_bind failed\n");
		return -1;
	}
	if (lwip_listen(s, 1) < 0) {	/* only one connection at once */
		mc_log_error("telnetd: lwip_listen failed\n");
		return -1;
	}
	mc_event_register(s, MC_SOCK_READ, telnet_connect_callback, the);
	listening_socket = s;
	return 0;
}

void telnetd_stop()
{
	if (stream.sock >= 0) {
		mc_event_unregister(stream.sock);
		lwip_close(stream.sock);
		stream.sock = -1;
	}
	if (listening_socket >= 0) {
		mc_event_unregister(listening_socket);
		lwip_close(listening_socket);
		listening_socket = -1;
	}
	if (telnet_instance != NULL) {
		telnetd_close(telnet_instance);
		telnet_instance = NULL;
	}
}
#endif	/* TELNETD_STANDALONE */
