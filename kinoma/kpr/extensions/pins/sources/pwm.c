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
#ifdef USEPWM

#include "FskExtensions.h"

#include "kprPins.h"

void xs_pwm(void *pwm)
{
	FskPinPWMDispose((FskPinPWM)pwm);
}

void xs_pwm_init(xsMachine* the)
{
    FskErr err;
    FskPinPWM pwm;
    SInt32 pin = 0;
    char *pinName = NULL;

    xsVars(1);

    pwm = xsGetHostData(xsThis);
    if (pwm)
        xsThrowDiagnosticIfFskErr(kFskErrBadState, "PWM pin %d already initialized.", (int)pin);

    xsVar(0) = xsGet(xsThis, xsID("pin"));
    if (xsStringType == xsTypeOf(xsVar(0)))
        pinName = xsToString(xsVar(0));
    else
        pin = xsToInteger(xsVar(0));

	err = FskPinPWMNew(&pwm, pin, pinName);
    xsThrowDiagnosticIfFskErr(err, "PWM initialization of pin %d failed with error %d.", (int)pin, FskInstrumentationGetErrorString(err));

    xsSetHostData(xsThis, pwm);
}

void xs_pwm_write(xsMachine* the)
{

	FskErr err;
	double value = xsToNumber(xsArg(0));
    FskPinPWM pwm = xsGetHostData(xsThis);
    if (!pwm) return;

	err = FskPinPWMSetDutyCycle(pwm, value);
	xsThrowDiagnosticIfFskErr(err, "PWM write of pin %d failed with error %d.", (int)-1, FskInstrumentationGetErrorString(err));
}

void xs_pwm_read(xsMachine* the)
{
	FskErr err;
	double value;
    FskPinPWM pwm = xsGetHostData(xsThis);
    if (!pwm) return;

	err = FskPinPWMGetDutyCycle(pwm, &value);
	xsThrowDiagnosticIfFskErr(err, "PWM read of pin %d failed with error %d.", (int)-1, FskInstrumentationGetErrorString(err));

	xsResult = xsNumber(value);
}

void xs_pwm_close(xsMachine* the)
{
    xs_pwm(xsGetHostData(xsThis));
    xsSetHostData(xsThis, NULL);
}

#endif /* USEPWM */
