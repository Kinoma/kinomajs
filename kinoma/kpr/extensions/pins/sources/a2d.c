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
#include "kprPins.h"

#ifdef USEA2D

#include "FskECMAScript.h"
#include "FskMemory.h"
#include "FskManifest.xs.h"

#define KPR_NO_GRAMMAR 1

void xs_a2d(void *a2d)
{
	FskPinAnalogDispose((FskPinAnalog)a2d);
}

void xs_a2d_init(xsMachine* the)
{
    FskErr err;
    FskPinAnalog a2d;
    SInt32 pin = xsToInteger(xsGet(xsThis, xsID("pin")));

    if (xsGetHostData(xsThis))
        xsThrowDiagnosticIfFskErr(kFskErrBadState, "Analog pin %d already initialized.", (int)pin);

    err = FskPinAnalogNew(&a2d, pin, NULL);
    xsThrowDiagnosticIfFskErr(err, "Analog init failed with error %s on pin %d.", FskInstrumentationGetErrorString(err), (int)pin);

    xsSetHostData(xsThis, a2d);
}

void xs_a2d_read(xsMachine* the)
{
	FskErr err;
    FskPinAnalog a2d = xsGetHostData(xsThis);
	double value;

	xsThrowIfNULL(a2d);

	err = FskPinAnalogRead(a2d, &value);
	xsThrowDiagnosticIfFskErr(err, "Analog pin read %d error.", -1);

	xsResult = xsNumber(value);
}

void xs_a2d_close(xsMachine* the)
{
    xs_a2d(xsGetHostData(xsThis));
    xsSetHostData(xsThis, NULL);
}

#endif /* USEA2D */
