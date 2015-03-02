/*
     Copyright (C) 2010-2015 Marvell International Ltd.
     Copyright (C) 2002-2010 Kinoma, Inc.

     Licensed under the Apache License, Version 2.0 (the "License");
     you may not use this file except in compliance with the License.
     You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

     Unless required by applicable law or agreed to in writing, software
     distributed under the License is distributed on an "AS IS" BASIS,
     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
     See the License for the specific language governing permissions and
     limitations under the License.
*/
#ifdef USEPWM

#include "FskECMAScript.h"
#include "FskExtensions.h"
#include "KplUIEvents.h"
#include "kprPins.h"
#include "FskMemory.h"

#define KPR_NO_GRAMMAR 1

static FskErr FskPWMNew(FskPWM *pwmOut, int pinNum, char *pinName)
{
	FskErr err;
	int busNum = -1;
    FskPWM pwm = NULL;

    if (NULL == pinName)
        busNum = FskHardwarePinsMux(pinNum, kFskHardwarePinPWM);
    else {
        if (0 == FskStrCompare(pinName, "backlight")) {     //@@ this is K4 specific name mapping. should be done in hardware specific file.
            pinNum = 0;
            busNum = 0;
        }
            
    }
	if (busNum < 0) return kFskErrInvalidParameter;

    err = FskMemPtrNewClear(sizeof(FskPWMRecord), (FskMemPtr *)&pwm);
	BAIL_IF_ERR(err);

	pwm->pinNum = pinNum;
	pwm->busNum = busNum;

    err = FskPWMPlatformInit(pwm);
	BAIL_IF_ERR(err);
    
bail:
    if (err)
        FskMemPtrDisposeAt(&pwm);

    *pwmOut = pwm;

	return err;
}

void xs_pwm(void *pwm)
{
    if (pwm) {
        FskPWMPlatformDispose((FskPWM)pwm);
        FskMemPtrDispose(pwm);
    }
}

static FskErr FskPWMReadPin(FskPWM pwm, double* value){
	if (pwm == NULL) return kFskErrFileNotOpen;
	return FskPWMPlatformGetDutyCycle(pwm, value);
}

void xs_pwm_init(xsMachine* the)
{
    FskErr err;
    FskPWM pwm;
    SInt32 pin = 0;
    char *pinName = NULL;

    xsVars(1);

    pwm = xsGetHostData(xsThis);
    if (pwm)
        xsThrowDiagnosticIfFskErr(kFskErrBadState, "PWM pin %d already initialized.", (int)pwm->pinNum);

    xsVar(0) = xsGet(xsThis, xsID("pin"));
    if (xsStringType == xsTypeOf(xsVar(0)))
        pinName = xsToString(xsVar(0));
    else
        pin = xsToInteger(xsVar(0));

    err = FskPWMNew(&pwm, pin, pinName);
    xsThrowDiagnosticIfFskErr(err, "PWM initialization of pin %d failed with error %d.", (int)pin, FskInstrumentationGetErrorString(err));

    xsSetHostData(xsThis, pwm);
}

void xs_pwm_write(xsMachine* the)
{
    FskPWM pwm = xsGetHostData(xsThis);
    if (pwm) {
		double value = xsToNumber(xsArg(0));
        FskErr err = FskPWMPlatformSetDutyCycle(pwm, value);
        xsThrowDiagnosticIfFskErr(err, "PWM write of pin %d failed with error %d.", (int)pwm->pinNum, FskInstrumentationGetErrorString(err));
    }
}

void xs_pwm_read(xsMachine* the)
{
    FskPWM pwm = xsGetHostData(xsThis);
    if (pwm) {
    	double value;
    	FskErr err = FskPWMReadPin(pwm, &value);
        xsThrowDiagnosticIfFskErr(err, "PWM wread of pin %d failed with error %d.", (int)pwm->pinNum, FskInstrumentationGetErrorString(err));
        xsResult = xsNumber(value);
    }
}

void xs_pwm_close(xsMachine* the)
{
    xs_pwm(xsGetHostData(xsThis));
    xsSetHostData(xsThis, NULL);
}

#endif /* USEPWM */
