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

#include "mraa/gpio.h"

static Boolean mraaDigitalCanHandle(SInt32 number, const char *name, SInt32 *remappedNumber);
static FskErr mraaDigitalNew(FskPinDigital *pin, SInt32 number, const char *name, FskPinsDigitalDirection direction);
static void mraaDigitalDispose(FskPinDigital pin);
static FskErr mraaDigitalSetDirection(FskPinDigital pin, FskPinsDigitalDirection direction);
static FskErr mraaDigitalGetDirection(FskPinDigital pin, FskPinsDigitalDirection *direction);
static FskErr mraaDigitalRead(FskPinDigital pin, Boolean *value);
static FskErr mraaDigitalWrite(FskPinDigital pin, Boolean value);
static FskErr mraaDigitalRepeat(FskPinDigital pin, FskPinDigitalRepeatTriggerProc triggeredCallback, void *refCon);

static FskPinDigitalDispatchRecord gMRAAPinDigital = {
	mraaDigitalCanHandle,
	mraaDigitalNew,
	mraaDigitalDispose,
	mraaDigitalSetDirection,
	mraaDigitalGetDirection,
	mraaDigitalRead,
	mraaDigitalWrite,
	mraaDigitalRepeat
};

typedef struct {
	FskPinDigitalRecord				pd;

	mraa_gpio_context				context;
	FskPinsDigitalDirection			direction;

	FskPinDigitalRepeatTriggerProc	triggeredCallback;
	void							*refCon
} mraaDigitalRecord, *mraaDigital;

static void mraaDigitalCallback(void *refCon);

Boolean mraaDigitalCanHandle(SInt32 number, const char *name, SInt32 *remappedNumber)
{
	return NULL == name;
}

FskErr mraaDigitalNew(FskPinDigital *pin, SInt32 number, const char *name, FskPinsDigitalDirection direction)
{
	FskErr err;
	mraa_gpio_context context;
	mraaDigital md;

	context = mraa_gpio_init(number);
	if (!context) return kFskErrOperationFailed;

	err = FskMemPtrNewClear(sizeof(mraaDigitalRecord), (FskMemPtr *)&md);
	if (err) {
		mraa_gpio_close(context);
		return err;
	}

	md->context = context;

	*pin = (FskPinDigital)md;
	return mraaDigitalSetDirection(*pin, direction);
}

void mraaDigitalDispose(FskPinDigital pin)
{
	mraaDigital md = (mraaDigital)pin;

	if (md->context)
		mraa_gpio_close(md->context);

	FskMemPtrDispose(pin);
}

FskErr mraaDigitalSetDirection(FskPinDigital pin, FskPinsDigitalDirection direction)
{
	mraaDigital md = (mraaDigital)pin;
	mraa_result_t result = MRAA_SUCCESS;

	md->direction = direction;

	if (kFskPinDigitalDirectionOut == direction)
		result = mraa_gpio_dir(md->context, MRAA_GPIO_OUT);
	else if (kFskPinDigitalDirectionIn == direction)
		result = mraa_gpio_dir(md->context, MRAA_GPIO_IN);

	return (MRAA_SUCCESS  == result) ? kFskErrNone : kFskErrOperationFailed;
}

FskErr mraaDigitalGetDirection(FskPinDigital pin, FskPinsDigitalDirection *direction)
{
	mraaDigital md = (mraaDigital)pin;

	*direction = md->direction;

	return kFskErrNone;
}

FskErr mraaDigitalRead(FskPinDigital pin, Boolean *value)
{
	mraaDigital md = (mraaDigital)pin;
	int result = mraa_gpio_read(md->context);
	if (-1 == result) return kFskErrOperationFailed;
	*value = result != 0;
	return kFskErrNone;
}

FskErr mraaDigitalWrite(FskPinDigital pin, Boolean value)
{
	mraaDigital md = (mraaDigital)pin;
	mraa_result_t result = mraa_gpio_write(md->context, value ? 1 : 0);
	return (MRAA_SUCCESS  == result) ? kFskErrNone : kFskErrOperationFailed;
}

FskErr mraaDigitalRepeat(FskPinDigital pin, FskPinDigitalRepeatTriggerProc triggeredCallback, void *refCon)
{
	mraaDigital md = (mraaDigital)pin;
	mraa_result_t result = MRAA_SUCCESS;

	md->triggeredCallback = triggeredCallback;
	md->refCon = refCon;

	mraa_gpio_isr_exit(md->context);
	if (triggeredCallback)
		result = mraa_gpio_isr(md->context, MRAA_GPIO_EDGE_BOTH, mraaDigitalCallback, md);

	return (MRAA_SUCCESS  == result) ? kFskErrNone : kFskErrOperationFailed;
}

void mraaDigitalCallback(void *refCon)
{
	mraaDigital md = (mraaDigital)refCon;
	if (md->triggeredCallback)
		(md->triggeredCallback)((FskPinDigital)md, md->refCon);
}

/*
	Extension
*/

FskExport(FskErr) FskPinDigitalMRAA_fskLoad(FskLibrary library)
{
	return FskExtensionInstall(kFskExtensionPinDigital, &gMRAAPinDigital);
}

FskExport(FskErr) FskPinDigitalMRAA_fskUnload(FskLibrary library)
{
	return FskExtensionUninstall(kFskExtensionPinDigital, &gMRAAPinDigital);
}
