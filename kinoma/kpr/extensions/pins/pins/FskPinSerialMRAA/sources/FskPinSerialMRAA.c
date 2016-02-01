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
#include "FskTime.h"

#include "mraa.h"

static Boolean mraaSerialCanHandle(SInt32 rxNumber, SInt32 txNumber, const char *name, char **remappedName);
static FskErr mraaSerialNew(FskPinSerial *pin, SInt32 rxNumber, SInt32 txNumber, const char *name, SInt32 baud);
static void mraaSerialDispose(FskPinSerial pin);
static FskErr mraaSerialWrite(FskPinSerial pin, SInt32 bufferSize, const UInt8 *bytes);
static FskErr mraaSerialRead(FskPinSerial pin, SInt32 bufferSize, SInt32 *bytesRead, UInt8 *bytes);
static FskErr mraaSerialDataReady(FskPinSerial pin, FskPinSerialRepeatTriggerProc triggered, void *refCon);

FskPinSerialDispatchRecord gMRAAPinSerial = {
	mraaSerialCanHandle,
	mraaSerialNew,
	mraaSerialDispose,
	mraaSerialWrite,
	mraaSerialRead,
	mraaSerialDataReady
};

typedef struct {
	FskPinSerialRecord				pd;

	mraa_uart_context				dev;

	FskPinSerialRepeatTriggerProc	triggered;
	void							*refCon;

	FskTimeCallBack					callback;
} mraaSerialRecord, *mraaSerial;

static void mraaCheckDataReady(FskTimeCallBack callback, const FskTime time, void *param);

Boolean mraaSerialCanHandle(SInt32 rxNumber, SInt32 txNumber, const char *name, char **remappedName)
{
	Boolean success = true;

	if (txNumber) {
		success = mraa_pin_mode_test(txNumber, MRAA_PIN_UART);
		if (!success)
			fprintf(stderr, "tx pin %d can not handle UART\n", txNumber);
	}
	if (success && rxNumber) {
		success = mraa_pin_mode_test(rxNumber, MRAA_PIN_UART);
		if (!success)
			fprintf(stderr, "rx pin %d can not handle UART\n", rxNumber);
	}

	return success;
}

FskErr mraaSerialNew(FskPinSerial *pin, SInt32 rxNumber, SInt32 txNumber, const char *name, SInt32 baud)
{
	FskErr err;
	mraaSerial ms;
	mraa_uart_context dev;

	if (mraa_get_platform_type() == MRAA_RASPBERRY_PI)
		dev = mraa_uart_init_raw("/dev/ttyAMA0");
	else
		dev = mraa_uart_init(0);
	if (!dev) {
		return kFskErrOperationFailed;
	}

	if (MRAA_SUCCESS != mraa_uart_set_baudrate(dev, baud)) {
		mraa_uart_stop(dev);
		return kFskErrOperationFailed;
	}

	err = FskMemPtrNewClear(sizeof(mraaSerialRecord), &ms);
	if (err) {
		mraa_uart_stop(dev);
		return err;
	}

	ms->dev = dev;

	*pin = (FskPinSerial)ms;
	return kFskErrNone;
}

void mraaSerialDispose(FskPinSerial pin)
{
	mraaSerial ms = (mraaSerial)pin;

	mraa_uart_stop(ms->dev);
	FskTimeCallbackDispose(ms->callback);
	FskMemPtrDispose(ms);
}

FskErr mraaSerialWrite(FskPinSerial pin, SInt32 bufferSize, const UInt8 *bytes)
{
	mraaSerial ms = (mraaSerial)pin;
	int result;

	result = mraa_uart_write(ms->dev, bytes, bufferSize);
	return (result < 0) ? kFskErrOperationFailed : kFskErrNone;
}

FskErr mraaSerialRead(FskPinSerial pin, SInt32 bufferSize, SInt32 *bytesReadOut, UInt8 *bytes)
{
	mraaSerial ms = (mraaSerial)pin;
	int result;

	if (!mraa_uart_data_available(ms->dev, 0)) {
		*bytesReadOut = 0;
		return kFskErrNoData;
	}

	result = mraa_uart_read(ms->dev, bytes, bufferSize);
	if (result <= 0)
		return kFskErrOperationFailed;

	*bytesReadOut = result;

	return kFskErrNone;
}

FskErr mraaSerialDataReady(FskPinSerial pin, FskPinSerialRepeatTriggerProc triggered, void *refCon)
{
	mraaSerial ms = (mraaSerial)pin;

	ms->triggered = triggered;
	ms->refCon = refCon;

	if (triggered) {
		if (!ms->callback) {
			FskTimeCallbackNew(&ms->callback);
			if (!ms->callback)
				return kFskErrOperationFailed;
			FskTimeCallbackScheduleFuture(ms->callback, 0, 50, mraaCheckDataReady, ms);
		}
	}
	else {
		if (ms->callback) {
			FskTimeCallbackDispose(ms->callback);
			ms->callback = NULL;
		}
	}

	return kFskErrNone;
}

void mraaCheckDataReady(FskTimeCallBack callback, const FskTime time, void *param)
{
	mraaSerial ms = (mraaSerial)param;

	if (mraa_uart_data_available(ms->dev, 0))
		(ms->triggered)((FskPinSerial)ms, ms->refCon);

	FskTimeCallbackScheduleFuture(ms->callback, 0, 50, mraaCheckDataReady, ms);
}

/*
	Extension
*/

FskExport(FskErr) FskPinSerialMRAA_fskLoad(FskLibrary library)
{
	mraa_result_t result;

	result = mraa_init();
	if ((result == MRAA_SUCCESS) || (result == MRAA_ERROR_PLATFORM_ALREADY_INITIALISED))
		return FskExtensionInstall(kFskExtensionPinSerial, &gMRAAPinSerial);
	else
		return kFskErrOperationFailed;
}

FskExport(FskErr) FskPinSerialMRAA_fskUnload(FskLibrary library)
{
	return FskExtensionUninstall(kFskExtensionPinSerial, &gMRAAPinSerial);
}
