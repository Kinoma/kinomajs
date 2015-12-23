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

#include "mraa/pwm.h"

static Boolean mraaPWMCanHandle(SInt32 number, const char *name, SInt32 *remappedNumber);
static FskErr mraaPWMNew(FskPinPWM *pin, SInt32 number, const char *name);
void mraaPWMDispose(FskPinPWM pin);
static FskErr mraaPWMSetDutyCycle(FskPinPWM pin, double value);
static FskErr mraaPWMGetDutyCycle(FskPinPWM pin, double *value);

FskPinPWMDispatchRecord gMRAAPWM = {
	mraaPWMCanHandle,
	mraaPWMNew,
	mraaPWMDispose,
	mraaPWMSetDutyCycle,
	mraaPWMGetDutyCycle
};

typedef struct {
	FskPinPWMRecord		pd;

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
	if (!mpwm->context)
		FskMemPtrDispose(mpwm);
		return kFskErrOperationFailed;
	}

	*pin = (FskPinPWM)mpwm;

	return err;
}

void mraaPWMDispose(FskPinPWM pin)
{
	mraaPWM mpwm = (mraaPWM)pin;
	mraaPWMSetDutyCycle(pin, 0);
	mraa_pwm_close(mpwm->context);
	FskMemPtrDispose(mpwm);
}

FskErr mraaPWMSetDutyCycle(FskPinPWM pin, double value)
{
	mraaPWM mpwm = (mraaPWM)pin;
	int result = mraa_pwm_write(mpwm->context, value);
	return (MRAA_SUCCESS == result) ? kFskErrNone : kFskErrOperationFailed;
}

FskErr mraaPWMGetDutyCycle(FskPinPWM pin, double *value)
{
	mraaPWM mpwm = (mraaPWM)pin;
	*value = mraa_pwm_read(npwm->context);
	return kFskErrNone;
}

/*
	Extension
*/

FskExport(FskErr) FskPinPWMCreate_fskLoad(FskLibrary library)
{
	return FskExtensionInstall(kFskExtensionPinPWM, &gMRAAPWM);
}

FskExport(FskErr) FskPinPWMCreate_fskUnload(FskLibrary library)
{
	return FskExtensionUninstall(kFskExtensionPinPWM, &gMRAAPWM);
}
