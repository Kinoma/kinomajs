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
#include "mc_xs.h"
#include "mc_stdio.h"
#include "mc_module.h"

#if !XS_ARCHIVE
#include "ext_a2d.xs.c"
MC_MOD_DECL(A2D);
#endif

#if mxMC
#include <wm_os.h>
#include <mdev_adc.h>

#define MC_A2D_MAX_ID	1
#define MC_ADC_GAIN		ADC_GAIN_0P5
#define MC_VMAX_IN_mV	1800	/* Max input voltage in milliVolts */

static int mc_a2d_ids[MC_A2D_MAX_ID] = {0};

typedef struct {
	mdev_t *mdev;
	int id;
	int channel;
	int resolution;
	int refVoltage;
	double gain;
} mc_a2d_t;

void open_a2d(xsMachine *the, mc_a2d_t *a2d)
{
	if ((a2d->mdev = adc_drv_open(a2d->id, a2d->channel)) == NULL) {
		mc_free(a2d);
		mc_xs_throw(the, "a2d: adc_drv_open failed");
	}
}

void close_a2d(mc_a2d_t* a2d)
{
	adc_drv_close(a2d->mdev);
	a2d->mdev = NULL;
}

void
xs_a2d_constructor(xsMachine *the)
{
	int id = xsToInteger(xsArg(0));
	int channel = xsToInteger(xsArg(1));
	mc_a2d_t *a2d;
	ADC_CFG_Type config;
	int ret;

	if (id >= MC_A2D_MAX_ID)
		mc_xs_throw(the, "bad arg");
	
	if (!mc_a2d_ids[id]) {
		adc_drv_init(id);
		mc_a2d_ids[id]++;
	}

	if ((a2d = mc_malloc(sizeof(mc_a2d_t))) == NULL)
		mc_xs_throw(the, "a2d: no mem");
	a2d->mdev = NULL;
	a2d->id = id;
	a2d->channel = channel;


#if defined(CONFIG_CPU_MW300)
	a2d->mdev = adc_drv_open(a2d->id, a2d->channel);

	ret = adc_drv_selfcalib(a2d->mdev, vref_internal);
	if (ret != WM_SUCCESS)
		mc_log_debug("A2D: Calibration failed!\n");

	adc_drv_close(a2d->mdev);
	a2d->mdev = NULL;
#endif
	
	adc_modify_default_config(adcResolution, ADC_RESOLUTION_16BIT);
	adc_modify_default_config(adcVrefSource, ADC_VREF_18);
	adc_modify_default_config(adcGainSel, MC_ADC_GAIN);
	adc_get_config(&config);

	switch (config.adcResolution) {
	case ADC_RESOLUTION_12BIT:
		a2d->resolution = 1 << 11;
		break;
	case ADC_RESOLUTION_14BIT:
		a2d->resolution = 1 << 13;
		break;
	case ADC_RESOLUTION_16BIT:
		a2d->resolution = 1 << 15;
		break;
	default:
		goto unsupported;
	}
	switch (config.adcVrefSource) {
	case ADC_VREF_18:
		a2d->refVoltage = 1800;
		break;
	case ADC_VREF_12:
		a2d->refVoltage = 1200;
		break;
	default:
		goto unsupported;
	}
	switch (config.adcGainSel) {
	case ADC_GAIN_0P5:
		a2d->gain = 0.5;
		break;
	case ADC_GAIN_1:
		a2d->gain = 1.0;
		break;
	case ADC_GAIN_2:
		a2d->gain = 2.0;
		break;
	default:
		goto unsupported;
	}

	xsSetHostData(xsThis, a2d);
	return;
unsupported:
	mc_xs_throw(the, "unsupported");
}



void
xs_a2d_destructor(void *data)
{
	if (data != NULL)
		mc_free(data);
}

void
xs_a2d_close(xsMachine *the)
{
	mc_a2d_t *a2d = xsGetHostData(xsThis);

	if(a2d->mdev){
		close_a2d(a2d);
		a2d->mdev = NULL;
	}
		
}

void
xs_a2d_read(xsMachine *the)
{
	mc_a2d_t *a2d = xsGetHostData(xsThis);
	float res;
	uint16_t sample;

	if(a2d->mdev)
		close_a2d(a2d);
	open_a2d(the, a2d);
	sample = adc_drv_result(a2d->mdev);
	close_a2d(a2d);

	res = ((float)sample / (float)a2d->resolution) * a2d->refVoltage * ( (float)1/ a2d->gain);
	
	if(res > 3300) res = 1.0;	//max = 3.3v
	else res /= 3300.0;			//normalize value to [0, 3.3v]

	xsSetNumber(xsResult, res);
	return;

}

#else

void
xs_a2d_constructor(xsMachine *the)
{
}

void
xs_a2d_destructor(void *data)
{
}

void
xs_a2d_close(xsMachine *the)
{
}

void
xs_a2d_read(xsMachine *the)
{
}

#endif
