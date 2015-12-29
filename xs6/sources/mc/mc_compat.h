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
#include "xs.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <sys/filio.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/utsname.h>
#if mxMacOSX
#include <ifaddrs.h>
#endif

#define lwip_ioctl(d, c, o)	ioctl(d, c, o)
#define lwip_fcntl(d, c, o)	fcntl(d, c, o)
#define lwip_socket(d, t, p)	socket(d, t, p)
#define lwip_gethostbyname(n)	gethostbyname(n)
#define lwip_gethostbyname_r(n, h, b, l, rp, hp)	gethostbyname_r(n)
#define lwip_getaddrinfo(n, s, h, r)	getaddrinfo(n, s, h, r)
#define lwip_freeaddrinfo(a)	freeaddrinfo(a)
#define lwip_close(d)	close(d)
#define lwip_connect(d, a, n)	connect(d, a, n)
#define lwip_sendto(d, b, n, o, a, l)	sendto(d, b, n, o, a, l)
#define lwip_send(d, b, n, o)	send(d, b, n, o)
#define lwip_recvfrom(d, b, n, o, a, l)	recvfrom(d, b, n, o, a, l)
#define lwip_recv(d, b, n, o)	recv(d, b, n, o)
#define lwip_accept(d, a, s)	accept(d, a, s)
#define lwip_bind(d, a, s)	bind(d, a, s)
#define lwip_listen(d, n)	listen(d, n)
#define lwip_read(d, b, n)	read(d, b, n)
#define lwip_write(d, b, n)	write(d, b, n)
#define lwip_select(n, r, w, e, t)	select(n, r, w, e, t)
#define lwip_setsockopt(d, p, c, b, s)	setsockopt(d, p, c, b, s)
#define lwip_getsockname(s, a, l)	getsockname(s, a, l)
#define lwip_getsockopt(s, l, n, o, p)	getsockopt(s, l, n, o, p)
