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
#include "mc_ipc.h"
#include "mc_time.h"
#include "mc_event.h"
#if mxMC
#include <lwip/sockets.h>
#else
#include "mc_compat.h"
#include <sys/time.h>
#include <semaphore.h>
#endif

static struct {
	mc_event_callback_f callback;
	void *closure;
	int fd;		/* only for any */
} mc_event_callbacks[FD_SETSIZE], mc_event_callback_any;	/* assume FD_SETSIZE is small */
static int maxfd = -1;
static fd_set reads, writes;

#define MC_MAX_TIMEOUTS	24
static struct mc_timeout {
	enum {MC_TIMEOUT_UNUSED, MC_TIMEOUT_RUNNING, MC_TIMEOUT_STOP} status;
	unsigned long interval;
	mc_timeout_callback_f callback;
	void *closure;
	uint64_t timeout;
	int pri;
} mc_timeout_callbacks[MC_MAX_TIMEOUTS] = {{0}};

static mc_event_shutdown_callback_t *mc_shutdown_callback = NULL;

static int g_status = -1;
static int g_exit_status = 0;

static uint64_t
mc_event_get_time()
{
	struct timeval tv;

	mc_gettimeofday(&tv, NULL);
	return (uint64_t)tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

void
mc_event_set_time(const struct timeval *tv)
{
	/* adjust timer */
	int i;
	uint64_t ct = mc_event_get_time();
	uint64_t nt = (uint64_t)tv->tv_sec * 1000 + tv->tv_usec / 1000;
	uint64_t diff = nt - ct;

	for (i = 0; i < MC_MAX_TIMEOUTS; i++) {
		struct mc_timeout *tc = &mc_timeout_callbacks[i];
		if (tc->status != MC_TIMEOUT_UNUSED)
			tc->timeout += diff;
	}
}

void
mc_event_shutdown()
{
	int i;

	mc_log_debug("event: shutting down\n");
	if (mc_shutdown_callback != NULL)
		(*mc_shutdown_callback->f)(g_exit_status, mc_shutdown_callback->closure);
	for (i = 0; i < FD_SETSIZE; i++) {
		if (mc_event_callbacks[i].callback != NULL)
			(*mc_event_callbacks[i].callback)(i, MC_SOCK_CLOSE, mc_event_callbacks[i].closure);
	}
}

void
mc_event_exit(int status)
{
	mc_event_shutdown();
	g_exit_status = status;
	if (status < 0)
		g_status++;	/* force quit */
}

void
mc_event_register(int s, unsigned int flags, mc_event_callback_f callback, void *closure)
{
	if (flags == MC_SOCK_ANY) {
		mc_event_callback_any.callback = callback;
		mc_event_callback_any.closure = closure;
		mc_event_callback_any.fd = s;
		return;
	}
	if (s < 0 || s >= FD_SETSIZE)
		mc_fatal("PANIC! bad fd! %d\n", s);
	if (flags & MC_SOCK_READ)
		FD_SET(s, &reads);
	else
		FD_CLR(s, &reads);
	if (flags & MC_SOCK_WRITE)
		FD_SET(s, &writes);
	else
		FD_CLR(s, &writes);
	mc_event_callbacks[s].callback = callback;
	mc_event_callbacks[s].closure = closure;
	if (s > maxfd)
		maxfd = s;
}

void
mc_event_unregister(int s)
{
	if (s < 0 || s >= FD_SETSIZE) {
		if (mc_event_callback_any.fd == s) {
			mc_event_callback_any.callback = NULL;
			mc_event_callback_any.closure = NULL;
			mc_event_callback_any.fd = 0;
		}
		return;
	}
	FD_CLR(s, &reads);
	FD_CLR(s, &writes);
	mc_event_callbacks[s].callback = NULL;
	mc_event_callbacks[s].closure = NULL;
	if (s == maxfd) {
		while (--maxfd >= 0 && mc_event_callbacks[maxfd].callback == NULL)
			;
	}
}

static uint64_t
mc_event_process_timeout(xsMachine *the)
{
	int n;
	uint64_t t_current, t_min, t;
	struct mc_timeout *tc, *tcmin;

	for (n = 0; n < MC_MAX_TIMEOUTS; n++)
		mc_timeout_callbacks[n].pri = 0;
	t_current = mc_event_get_time();
again:
	t_min = ~0U;
	for (n = MC_MAX_TIMEOUTS, tc = mc_timeout_callbacks, tcmin = NULL; --n >= 0; tc++) {
		if (tc->status == MC_TIMEOUT_RUNNING) {
			if (tc->timeout <= t_current) {
				if (tcmin == NULL || tc->timeout < tcmin->timeout || tc->pri < tcmin->pri)
					tcmin = tc;
			}
			else if ((t = tc->timeout - t_current) < t_min)
				t_min = t;
		}
	}
	if (tcmin != NULL) {
		(*tcmin->callback)(the, tcmin, tcmin->closure);
		tcmin->pri++;
		if (tcmin->status == MC_TIMEOUT_RUNNING)
			tcmin->timeout = t_current + tcmin->interval;
		goto again;
	}
	return t_min != ~0U ? t_min : 0U;
}

struct mc_timeout *
mc_interval_set(unsigned long interval, mc_timeout_callback_f callback, void *closure)
{
	int i;

	for (i = 0; i < MC_MAX_TIMEOUTS; i++) {
		if (mc_timeout_callbacks[i].status == MC_TIMEOUT_UNUSED) {
			struct mc_timeout *tc = &mc_timeout_callbacks[i];
			tc->interval = interval;
			tc->status = MC_TIMEOUT_STOP;
			tc->callback = callback;
			tc->closure = closure;
			return tc;
		}
	}
	mc_log_error("# Error: timeout: no slot!\n");
	return NULL;
}

void
mc_interval_start(struct mc_timeout *tc)
{
	tc->status = MC_TIMEOUT_RUNNING;
	tc->timeout = mc_event_get_time() + tc->interval;
}

void
mc_interval_stop(struct mc_timeout *tc)
{
	tc->status = MC_TIMEOUT_STOP;
}

void
mc_interval_reschedule(struct mc_timeout *tc, unsigned long newInterval)
{
	tc->interval = newInterval;
	mc_interval_start(tc);
}

void
mc_interval_reset(struct mc_timeout *tc)
{
	tc->status = MC_TIMEOUT_UNUSED;
}

unsigned long
mc_interval_get_interval(struct mc_timeout *tc)
{
	return tc->interval;
}

void
mc_interval_stat()
{
	int i;

	mc_log_notice("current time = %lld\n", mc_event_get_time());
	for (i = 0; i < MC_MAX_TIMEOUTS; i++) {
		struct mc_timeout *tc = &mc_timeout_callbacks[i];
		if (tc->status != MC_TIMEOUT_UNUSED)
			mc_log_notice("[%d] interval = %ld, timeout = %lld\n", i, tc->interval, tc->timeout);
	}
}

typedef struct {
	mc_event_thread_callback_f callback;
	void *closure;
	mc_semaphore_t sem;
	uint32_t flags;
} mc_local_event_t;
#define MC_LOCAL_EVENT_PORT	9999

#define USE_SEMAPHORE	0

#if USE_SEMAPHORE
static void
mc_task_init(mc_local_event_t *ev)
{
	mc_semaphore_create(&ev->sem);
}

static void
mc_task_sleep(mc_local_event_t *ev)
{
	mc_semaphore_wait(&ev->sem);
}

static void
mc_task_wake(mc_local_event_t *ev)
{
	mc_semaphore_post(&ev->sem);
}

static void
mc_task_fin(mc_local_event_t *ev)
{
	mc_semaphore_delete(&ev->sem);
}
#endif

static void
mc_event_thread_call_local(mc_event_thread_callback_f callback, void *closure, uint32_t flags)
{
	int s;
	struct sockaddr_in sin;
	mc_local_event_t ev;
	int fl;

	if ((s = lwip_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
		return;
	fl = lwip_fcntl(s, F_GETFL, 0);
	lwip_fcntl(s, F_SETFL, fl | O_NONBLOCK);
	sin.sin_family = AF_INET;
	sin.sin_port = htons(MC_LOCAL_EVENT_PORT);
	sin.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	ev.callback = callback;
	ev.closure = closure;
	ev.flags = flags;
#if USE_SEMAPHORE
	mc_task_init(&ev);
#endif
	lwip_sendto(s, &ev, sizeof(ev), 0, (struct sockaddr *)&sin, sizeof(sin));
#if !USE_SEMAPHORE
	if (!(flags & MC_CALL_ASYNC))
		lwip_recv(s, &ev, sizeof(ev), 0);
#endif
	lwip_close(s);
#if USE_SEMAPHORE
	if (!(flags & MC_CALL_ASYNC)) {
		mc_task_sleep(&ev);
		mc_task_fin(&ev);
	}
#endif
}

static void
mc_event_local_callback(int s, unsigned int flags, void *closure)
{
	xsMachine *the = closure;
	mc_local_event_t ev;
	struct sockaddr_in sin;
	socklen_t slen = sizeof(sin);
	int n;

	if (flags & MC_SOCK_READ) {
		n = lwip_recvfrom(s, &ev, sizeof(ev), 0, (struct sockaddr *)&sin, &slen);
		if (n == sizeof(ev) && ev.callback != NULL)
			(*ev.callback)(the, ev.closure);
#if !USE_SEMAPHORE
		if (!(ev.flags & MC_CALL_ASYNC))
			lwip_sendto(s, &ev, sizeof(ev), 0, (struct sockaddr *)&sin, sizeof(sin));
#endif
	}
	else {
		mc_event_unregister(s);
		lwip_close(s);
	}
#if USE_SEMAPHORE
	if (!(ev.flags & MC_CALL_ASYNC))
		mc_task_wake(&ev);
#endif
}

typedef struct {
	mc_event_thread_callback_f callback;
	void *closure;
	uint32_t flags;
} mc_delegation_event_t;

static mc_queue_t mc_delegation_queue;

static void
mc_delegation_main(void *data)
{
	mc_delegation_event_t ev = {NULL, NULL};

	do {
		if (mc_queue_recv(&mc_delegation_queue, &ev, sizeof(ev)) == sizeof(ev) && ev.callback != NULL)
			mc_event_thread_call_local(ev.callback, ev.closure, ev.flags);
	} while (ev.callback != NULL);
	mc_queue_delete(&mc_delegation_queue);
}

static int
mc_event_delegation_init()
{
	if (mc_queue_create(&mc_delegation_queue, sizeof(mc_delegation_event_t)) != 0)
		return -1;
	if (mc_thread_create(mc_delegation_main, NULL, -1) != 0) {
		mc_queue_delete(&mc_delegation_queue);
		return -1;
	}
	return 0;
}

static void
mc_event_delegation_call(mc_event_thread_callback_f callback, void *closure, uint32_t flags)
{
	mc_delegation_event_t ev;

	ev.callback = callback;
	ev.closure = closure;
	ev.flags = flags;
	mc_queue_send(&mc_delegation_queue, &ev, sizeof(ev));
}

static void
mc_event_delegation_fin()
{
	mc_event_delegation_call(NULL, NULL, 0);	/* terminate the thread */
}

int
mc_event_thread_call(mc_event_thread_callback_f callback, void *closure, uint32_t flags)
{
	if (g_status != 0)
		return -1;

	if (flags & MC_CALL_CRITICAL)
		mc_event_delegation_call(callback, closure, flags);
	else
		mc_event_thread_call_local(callback, closure, flags);
	return 0;
}

int
mc_event_main(xsMachine *the, mc_event_shutdown_callback_t *cb)
{
	struct timeval tv;
	fd_set rs, ws;
	int n = -1, i;
	unsigned int flags;
	uint64_t timeInterval;
	struct sockaddr_in sin;
	int localevent_sock;

	mc_shutdown_callback = cb;
	g_status = -1;	/* not ready yet */
	if ((localevent_sock = lwip_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
		return -1;
	sin.sin_family = AF_INET;
	sin.sin_port = htons(MC_LOCAL_EVENT_PORT);
	sin.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	lwip_bind(localevent_sock, (struct sockaddr *)&sin, sizeof(sin));
	mc_event_register(localevent_sock, MC_SOCK_READ, mc_event_local_callback, the);
	mc_event_delegation_init();
	g_status = 0;	/* running */
	while (!g_status) {
		timeInterval = mc_event_process_timeout(the);
		if (timeInterval != 0) {
			tv.tv_sec = timeInterval / 1000;
			tv.tv_usec = (timeInterval % 1000) * 1000;
		}
		if (maxfd < 0) {	/* @@ what if only timer is running?? */
			mc_log_debug("event: no one is waiting!\n");
			break;	/* no one is waiting for anything! will be blocked forever unless break */
		}
		rs = reads;
		ws = writes;
		n = lwip_select(maxfd + 1, &rs, &ws, NULL, timeInterval > 0 ? &tv : NULL);
		if (mc_event_callback_any.callback != NULL)
			(*mc_event_callback_any.callback)(mc_event_callback_any.fd, (unsigned int)n, mc_event_callback_any.closure);
		if (n > 0) {
			for (i = 0; i <= maxfd; i++) {
				flags = 0;
				if (FD_ISSET(i, &rs))
					flags |= MC_SOCK_READ;
				if (FD_ISSET(i, &ws))
					flags |= MC_SOCK_WRITE;
				if (flags) {
					if (mc_event_callbacks[i].callback == NULL) {
						mc_log_error("event: %s ready for closed socket(%d)!?\n", flags & MC_SOCK_WRITE ? "write" : "read", i);
						continue;
					}
					(*mc_event_callbacks[i].callback)(i, flags, mc_event_callbacks[i].closure);
				}
			}
		}
	}
	mc_event_delegation_fin();
	lwip_close(localevent_sock);
	return g_exit_status;
}
