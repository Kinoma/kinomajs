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
#include "mc_xs.h"
#include "mc_stdio.h"
#include "mc_module.h"

#if !XS_ARCHIVE
#include "ext_pwm.xs.c"
MC_MOD_DECL(PWM);
#endif

#if mxMC
#include <wm_os.h>
#include <mdev_gpt.h>

#define MC_GPT_MAX_ID	4

static int mc_gpt_ids[MC_GPT_MAX_ID] = {0};

typedef struct {
	int id;
	int channel;
	int running;
} mc_gpt_t;

void
xs_gpt_constructor(xsMachine *the)
{
	int id = xsToInteger(xsArg(0));
	int channel = xsToInteger(xsArg(1));
	mc_gpt_t *gpt;

	if (id >= MC_GPT_MAX_ID)
		mc_xs_throw(the, "gpt: bad arg");
	if (!mc_gpt_ids[id]) {
		gpt_drv_init(id);
		mc_gpt_ids[id]++;
	}
	if ((gpt = mc_malloc(sizeof(mc_gpt_t))) == NULL)
		mc_xs_throw(the, "gpt: no mem");
	gpt->id = id;
	gpt->channel = channel;
	gpt->running = 0;
	xsSetHostData(xsThis, gpt);
}

void
xs_gpt_destructor(void *data)
{
	mc_gpt_t *gpt = data;

	if (data != NULL) {
		if (gpt->running) {
			mdev_t *mdev = gpt_drv_open(gpt->id);
			if (mdev != NULL) {
				gpt_drv_stop(mdev);
				gpt_drv_close(mdev);
			}
		}
		mc_free(gpt);
	}
}

void
xs_gpt_close(xsMachine *the)
{
	mc_gpt_t *gpt = xsGetHostData(xsThis);

	if (gpt->running) {
		mdev_t *mdev = gpt_drv_open(gpt->id);
		if (mdev != NULL) {
			gpt_drv_stop(mdev);
			gpt_drv_close(mdev);
			gpt->running = 0;
		}
	}
}

void
xs_gpt_start(xsMachine *the)
{
	mc_gpt_t *gpt = xsGetHostData(xsThis);
	uint32_t interval = xsToInteger(xsArg(0));
	mdev_t *mdev;

	if ((mdev = gpt_drv_open(gpt->id)) == NULL)
		return;
	gpt_drv_set(mdev, interval);
	gpt_drv_start(mdev);
	gpt_drv_close(mdev);
	gpt->running++;
	xsSetTrue(xsResult);
}

void
xs_gpt_stop(xsMachine *the)
{
	mc_gpt_t *gpt = xsGetHostData(xsThis);
	mdev_t *mdev;

	if ((mdev = gpt_drv_open(gpt->id)) == NULL)
		return;
	gpt_drv_stop(mdev);
	gpt_drv_close(mdev);
	gpt->running = 0;
	xsSetTrue(xsResult);
}

void
xs_gpt_pwm(xsMachine *the)
{
	mc_gpt_t *gpt = xsGetHostData(xsThis);
	double on = xsToNumber(xsArg(0));	/* in ms */
	double off = xsToNumber(xsArg(1));	/* ditto */
	mdev_t *mdev;

	if ((mdev = gpt_drv_open(gpt->id)) == NULL)
		return;
	gpt_drv_set(mdev, 0xffffffff/50);
	gpt_drv_pwm(mdev, gpt->channel, (uint32_t)(on * 50000), (uint32_t)(off * 50000));	/* @ 50MHz? */
	gpt_drv_start(mdev);
	gpt_drv_close(mdev);
	gpt->running++;
	xsSetTrue(xsResult);
}

#else

void
xs_gpt_constructor(xsMachine *the)
{
}

void
xs_gpt_destructor(void *data)
{
}

void
xs_gpt_close(xsMachine *the)
{
}

void
xs_gpt_start(xsMachine *the)
{
}

void
xs_gpt_stop(xsMachine *the)
{
}

void
xs_gpt_pwm(xsMachine *the)
{
}

#endif
