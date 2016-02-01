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

#include "FskPin.h"
#include "FskMemory.h"

#include "mraa/pwm.h"

#define PWMDEFAULTPERIOD 20

static Boolean mraaPWMCanHandle(SInt32 number, const char *name, SInt32 *remappedNumber);
static FskErr mraaPWMNew(FskPinPWM *pin, SInt32 number, const char *name);
void mraaPWMDispose(FskPinPWM pin);
static FskErr mraaPWMSetDutyCycle(FskPinPWM pin, double value);
static FskErr mraaPWMGetDutyCycle(FskPinPWM pin, double *value);
static FskErr mraaPWMSetDutyCycleAndPeriod(FskPinPWM pin, UInt8 dutyCycle, UInt8 period);
static FskErr mraaPWMGetDutyCycleAndPeriod(FskPinPWM pin, UInt8 *dutyCycle, UInt8 *period);

FskPinPWMDispatchRecord gMRAAPWM = {
	mraaPWMCanHandle,
	mraaPWMNew,
	mraaPWMDispose,
	mraaPWMSetDutyCycle,
	mraaPWMGetDutyCycle,
	mraaPWMSetDutyCycleAndPeriod,
	mraaPWMGetDutyCycleAndPeriod
};

typedef struct {
	FskPinPWMRecord		pd;

	int		enabled;
	UInt8	period;
	UInt8	dutyCycle;

	mraa_pwm_context	context;
} mraaPWMRecord, *mraaPWM;

Boolean mraaPWMCanHandle(SInt32 number, const char *name, SInt32 *remappedNumber)
{
	return NULL == name;
}

FskErr mraaPWMNew(FskPinPWM *pin, SInt32 number, const char *name)
{
	FskErr err;
	mraaPWM mpwm;

	err = FskMemPtrNewClear(sizeof(mraaPWMRecord), &mpwm);
	if (err) return err;

	mpwm->context = mraa_pwm_init(number);
	if (!mpwm->context) {
		FskMemPtrDispose(mpwm);
		return kFskErrOperationFailed;
	}

	mraa_pwm_period_ms(mpwm->context, PWMDEFAULTPERIOD);
	mpwm->period = PWMDEFAULTPERIOD;
	mraa_pwm_write(mpwm->context, 0.0);

	*pin = (FskPinPWM)mpwm;

	return err;
}

void mraaPWMDispose(FskPinPWM pin)
{
	mraaPWM mpwm = (mraaPWM)pin;
	mraaPWMSetDutyCycle(pin, 0);
	mraa_pwm_enable(mpwm->context, 0);
	mraa_pwm_close(mpwm->context);
	FskMemPtrDispose(mpwm);
}

FskErr mraaPWMSetDutyCycle(FskPinPWM pin, double value)
{
	mraaPWM mpwm = (mraaPWM)pin;
	int result = mraa_pwm_write(mpwm->context, value);

	if (MRAA_SUCCESS != result)
		return kFskErrOperationFailed;

	if (!mpwm->enabled) {
		mraa_pwm_enable(mpwm->context, 1);
		mpwm->enabled = 1;
	}
	return  kFskErrNone;
}

FskErr mraaPWMGetDutyCycle(FskPinPWM pin, double *value)
{
	mraaPWM mpwm = (mraaPWM)pin;
	*value = mraa_pwm_read(mpwm->context);
	return kFskErrNone;
}

FskErr mraaPWMSetDutyCycleAndPeriod(FskPinPWM pin, UInt8 dutyCycle, UInt8 period)
{
	// dutyCycle and period aer in ms
	mraaPWM mpwm = (mraaPWM)pin;
	int result;
	float dc = (float)dutyCycle;

	result = mraa_pwm_config_ms(mpwm->context, period, dc);
	if (MRAA_SUCCESS != result) {
		return kFskErrOperationFailed;
	}

	mpwm->period = period;
	mpwm->dutyCycle = dutyCycle;
	if (!mpwm->enabled) {
		mraa_pwm_enable(mpwm->context, 1);
		mpwm->enabled = 1;
	}
	return  kFskErrNone;
}

FskErr mraaPWMGetDutyCycleAndPeriod(FskPinPWM pin, UInt8 *dutyCycle, UInt8 *period)
{
	mraaPWM mpwm = (mraaPWM)pin;
	float value, dc;

	value = mraa_pwm_read(mpwm->context);
	dc = mpwm->period * value;
	*dutyCycle = dc;
	*period = mpwm->period;

	return kFskErrNone;
}

/*
	Extension
*/

FskExport(FskErr) FskPinPWMMRAA_fskLoad(FskLibrary library)
{
	return FskExtensionInstall(kFskExtensionPinPWM, &gMRAAPWM);
}

FskExport(FskErr) FskPinPWMMRAA_fskUnload(FskLibrary library)
{
	return FskExtensionUninstall(kFskExtensionPinPWM, &gMRAAPWM);
}
