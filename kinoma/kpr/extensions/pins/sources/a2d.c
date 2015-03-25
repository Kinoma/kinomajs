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
#ifdef USEA2D

#include "kprPins.h"

#include "FskECMAScript.h"
#include "FskMemory.h"
#include "FskManifest.xs.h"

#define KPR_NO_GRAMMAR 1

static FskErr FskA2DNew(FskA2D *a2dOut, SInt32 pin);

static FskErr FskA2DNew(FskA2D *a2dOut, SInt32 pin)
{
	FskErr err;
	FskA2D a2d = NULL;
	int mux;

	err = FskMemPtrNewClear(sizeof(FskA2DRecord), (FskMemPtr *)&a2d);
    BAIL_IF_ERR(err);

	a2d->pinNum = pin;

	mux = FskHardwarePinsMux(pin, kFskHardwarePinADC);
	if (mux < 0)
		BAIL(kFskErrInvalidParameter);

    a2d->realPin = mux;
    err = FskA2DPlatformInit(a2d);
    BAIL_IF_ERR(err);

bail:
    if (err) {
        xs_a2d(a2d);
        a2d = NULL;
    }

    *a2dOut = a2d;

    return err;
}

void xs_a2d(void *a2d)
{
    if (a2d) {
        FskA2DPlatformDispose((FskA2D)a2d);
        FskMemPtrDispose(a2d);
    }
}

void xs_a2d_init(xsMachine* the)
{
    FskErr err;
    FskA2D a2d;
    SInt32 pin = xsToInteger(xsGet(xsThis, xsID("pin")));

    if (xsGetHostData(xsThis))
        xsThrowDiagnosticIfFskErr(kFskErrBadState, "A2D pin %d already initialized.", (int)pin);

    err = FskA2DNew(&a2d, pin);
    xsThrowDiagnosticIfFskErr(err, "A2D init failed with error %s on pin %d.", FskInstrumentationGetErrorString(err), (int)pin);

    xsSetHostData(xsThis, a2d);
}

void xs_a2d_read(xsMachine* the)
{
    FskA2D a2d = xsGetHostData(xsThis);
    if (a2d) {
		double value = FskA2DPlatformRead(a2d);
        if (-1 == value)
            xsThrowDiagnosticIfFskErr(kFskErrOperationFailed, "A2D pin %d read error.", (int)a2d->pinNum);
        xsResult = xsNumber(value);
    }
}

void xs_a2d_close(xsMachine* the)
{
    xs_a2d(xsGetHostData(xsThis));
    xsSetHostData(xsThis, NULL);
}

#endif /* USEA2D */
