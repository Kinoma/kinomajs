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

#include "kprPins.h"

static Boolean createPWMCanHandle(SInt32 number, const char *name, SInt32 *remappedNumber);
static FskErr createPWMNew(FskPinPWM *pin, SInt32 number, const char *name);
void createPWMDispose(FskPinPWM pin);
static FskErr createPWMSetDutyCycle(FskPinPWM pin, double value);
static FskErr createPWMGetDutyCycle(FskPinPWM pin, double *value);

FskPinPWMDispatchRecord gCreatePWM = {
	createPWMCanHandle,
	createPWMNew,
	createPWMDispose,
	createPWMSetDutyCycle,
	createPWMGetDutyCycle,
    NULL,
    NULL
};

typedef struct {
	FskPinPWMRecord		pd;

	FILE				*file;
} createPWMRecord, *createPWM;

Boolean createPWMCanHandle(SInt32 number, const char *name, SInt32 *remappedNumber)
{
    if (NULL == name){
        SInt32 pinNum = FskHardwarePinsMux(number, kFskHardwarePinPWM);
        if (pinNum >= 0 && pinNum < FRONTOFFSET) return true;
    }

	if (0 == FskStrCompare(name, "backlight"))
		return true;

	return false;
}

FskErr createPWMNew(FskPinPWM *pin, SInt32 number, const char *name)
{
	FskErr err;
	createPWM cpwm;
	int busNum = -1;
	char buffer[60];

	if (NULL == name)
		busNum = FskHardwarePinsMux(number, kFskHardwarePinPWM);
	else {
		if (0 == FskStrCompare(name, "backlight")) {
			number = 0;
			busNum = 0;
		}
			
	}

	if (busNum < 0)
		return kFskErrUnimplemented;

	err = FskMemPtrNewClear(sizeof(createPWMRecord), &cpwm);
	if (err) return err;

	snprintf(buffer, 60, "/sys/class/backlight/pwm-backlight.%d/brightness", busNum);
	cpwm->file = fopen(buffer, "w+");
	if (!cpwm->file) {
		FskMemPtrDispose(cpwm);
		return kFskErrFileNotFound;
	}
	setbuf(cpwm->file, NULL);

	*pin = (FskPinPWM)cpwm;

	return err;
}

void createPWMDispose(FskPinPWM pin)
{
	createPWM cpwm = (createPWM)pin;
	createPWMSetDutyCycle(pin, 0);
	fclose(cpwm->file);
	FskMemPtrDispose(cpwm);
}

FskErr createPWMSetDutyCycle(FskPinPWM pin, double value)
{
	createPWM cpwm = (createPWM)pin;
	int result = fprintf(cpwm->file, "%d\n", (int)(value * 1023.0));
	return (result < 0) ? kFskErrOperationFailed : kFskErrNone;
}

FskErr createPWMGetDutyCycle(FskPinPWM pin, double *value)
{
	createPWM cpwm = (createPWM)pin;
	int result;
	int intvalue;

	rewind(cpwm->file);
	result = fscanf(cpwm->file, "%d", &intvalue);
	if (result < 0) return kFskErrOperationFailed;

	*value = ((double)intvalue) / 1023.0;
	return kFskErrNone;
}

/*
	Extension
*/

FskExport(FskErr) FskPinPWMCreate_fskLoad(FskLibrary library)
{
	return FskExtensionInstall(kFskExtensionPinPWM, &gCreatePWM);
}

FskExport(FskErr) FskPinPWMCreate_fskUnload(FskLibrary library)
{
	return FskExtensionUninstall(kFskExtensionPinPWM, &gCreatePWM);
}
