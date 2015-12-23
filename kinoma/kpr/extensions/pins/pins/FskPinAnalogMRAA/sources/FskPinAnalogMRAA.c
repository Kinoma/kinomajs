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

#include "mraa/aio.h"

static Boolean mraaAnalogCanHandle(SInt32 number, const char *name, SInt32 *remappedNumber);
static FskErr mraaAnalogNew(FskPinAnalog *pin, SInt32 number, const char *name);
static void mraaAnalogDispose(FskPinAnalog pin);
static FskErr mraaAnalogRead(FskPinAnalog pin, double *value);

static FskPinAnalogDispatchRecord gMRAAPinAnalog = {
	mraaAnalogCanHandle,
	mraaAnalogNew,
	mraaAnalogDispose,
	mraaAnalogRead
};

typedef struct {
	FskPinAnalogRecord		pd;

	mraa_aio_context		context;
} mraaAnalogRecord, *mraaAnalog;

Boolean mraaAnalogCanHandle(SInt32 number, const char *name, SInt32 *remappedNumber)
{
	return NULL == name;
}

FskErr mraaAnalogNew(FskPinAnalog *pin, SInt32 number, const char *name)
{
	FskErr err;
	mraa_aio_context context;
	mraaAnalog ma;

	context = mraa_aio_init(number);
	if (!context) return kFskErrOperationFailed;

	err = FskMemPtrNewClear(sizeof(mraaAnalogRecord), (FskMemPtr *)&ma);
	if (err) {
		mraa_aio_close(context);
		return err;
	}

	ma->context = context;

	*pin = (FskPinAnalog)ma;
	return kFskErrNone;
}

void mraaAnalogDispose(FskPinAnalog pin)
{
	mraaAnalog ma = (mraaAnalog)pin;

	if (ma->context)
		mraa_aio_close(ma->context);

	FskMemPtrDispose(pin);
}

FskErr mraaAnalogRead(FskPinAnalog pin, double *value)
{
	mraaAnalog ma = (mraaAnalog)pin;

	*value = (double)mraa_aio_read_float(ma->context);

	return kFskErrNone;
}

/*
	Extension
*/

FskExport(FskErr) FskPinAnalogMRAA_fskLoad(FskLibrary library)
{
	return FskExtensionInstall(kFskExtensionPinAnalog, &gMRAAPinAnalog);
}

FskExport(FskErr) FskPinAnalogMRAA_fskUnload(FskLibrary library)
{
	return FskExtensionUninstall(kFskExtensionPinAnalog, &gMRAAPinAnalog);
}
