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
#include "mc_module.h"

#if !XS_ARCHIVE
#include "xm_timeinterval.xs.c"
MC_MOD_DECL(timeinterval);
#endif

typedef struct {
	struct mc_timeout *tc;
	xsSlot obj;
} mc_timeinterval_t;

static void
timeInterval_callback(xsMachine *the, struct mc_timeout *tc, void *closure)
{
	mc_timeinterval_t *ti = closure;

	xsBeginHost(the);
	xsCall_noResult(ti->obj, xsID("_callback"), NULL);
	xsEndHost(the);
}

void
xs_timeInterval_destructor(void *data)
{
	mc_timeinterval_t *ti = data;

	if (ti != NULL) {
		mc_interval_reset(ti->tc);
		mc_free(ti);
	}
}

void
xs_timeInterval_constructor(xsMachine *the)
{
	mc_timeinterval_t *ti;
	unsigned long interval;

	if ((ti = mc_malloc(sizeof(mc_timeinterval_t))) == NULL)
		mc_xs_throw(the, "TimeInteval: no mem");
	interval = xsToInteger(xsArg(1));
	xsSet(xsThis, xsID("_callback"), xsArg(0));
	ti->obj = xsThis;
	if ((ti->tc = mc_interval_set(interval, timeInterval_callback, ti)) == NULL) {
		xs_timeInterval_destructor(ti);
		mc_xs_throw(the, "TimeInterval: no time slot");
	}
	xsSetHostData(xsThis, ti);
}

void
xs_timeInterval_close(xsMachine *the)
{
	mc_timeinterval_t *ti = xsGetHostData(xsThis);

	if (ti != NULL) {
		mc_interval_reset(ti->tc);
		mc_free(ti);
		xsSetHostData(xsThis, NULL);
	}
}

void
xs_timeInterval_start(xsMachine *the)
{
	mc_timeinterval_t *ti = xsGetHostData(xsThis);

	if (ti == NULL)
		mc_xs_throw(the, "TimeInterval: no instance");
	mc_interval_start(ti->tc);
}

void
xs_timeInterval_stop(xsMachine *the)
{
	mc_timeinterval_t *ti = xsGetHostData(xsThis);

	if (ti == NULL)
		mc_xs_throw(the, "TimeInterval: no instance");
	mc_interval_stop(ti->tc);
}

void
xs_timeInterval_reschedule(xsMachine *the)
{
	mc_timeinterval_t *ti = xsGetHostData(xsThis);
	unsigned long newInterval = xsToInteger(xsArg(0));

	if (ti == NULL)
		mc_xs_throw(the, "TimeInterval: no instance");
	mc_interval_reschedule(ti->tc, newInterval);
}

void
xs_timeInterval_getInterval(xsMachine *the)
{
	mc_timeinterval_t *ti = xsGetHostData(xsThis);

	if (ti == NULL)
		mc_xs_throw(the, "TimeInterval: no instance");
	xsSetInteger(xsResult, mc_interval_get_interval(ti->tc));
}
