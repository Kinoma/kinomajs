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
#ifndef __MC_EVENT_H__
#define __MC_EVENT_H__

#include "xs.h"

typedef void (*mc_event_callback_f)(int s, unsigned int flags, void *closure);
typedef struct {
	void (*f)(int status, void *closure);
	void *closure;
} mc_event_shutdown_callback_t;
#define MC_SOCK_READ	0x1
#define MC_SOCK_WRITE	0x2
#define MC_SOCK_CLOSE	0
#define MC_SOCK_ANY	0x4
extern void mc_event_register(int s, unsigned int flags, mc_event_callback_f callback, void *closure);
extern void mc_event_unregister(int s);
extern int mc_event_main(xsMachine *the, mc_event_shutdown_callback_t *cb);
extern void mc_event_shutdown();
extern void mc_event_exit(int status);
#define MC_CALL_CRITICAL	0x01
#define MC_CALL_ASYNC		0x02
typedef void (*mc_event_thread_callback_f)(xsMachine *, void *closure);
extern int mc_event_thread_call(mc_event_thread_callback_f callback, void *closure, uint32_t flags);

/* timer */
struct mc_timeout;
typedef void (*mc_timeout_callback_f)(xsMachine *, struct mc_timeout *tc, void *closure);
extern struct mc_timeout *mc_interval_set(unsigned long interval, mc_timeout_callback_f callback, void *closure);
extern void mc_interval_start(struct mc_timeout *tc);
extern void mc_interval_stop(struct mc_timeout *tc);
extern void mc_interval_reset(struct mc_timeout *tc);
extern void mc_interval_reschedule(struct mc_timeout *tc, unsigned long newInterval);
extern unsigned long mc_interval_get_interval(struct mc_timeout *tc);
extern void mc_event_set_time(const struct timeval *tv);
extern void mc_interval_stat();

#endif /* __MC_EVENT_H__ */
