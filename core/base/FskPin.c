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

#include "FskExtensions.h"
#include "FskMemory.h"

FskErr FskPinDigitalNew(FskPinDigital *pin, SInt32 number, const char *name, FskPinsDigitalDirection direction)
{
	FskErr err;
	FskPinDigitalDispatch dispatch = NULL;
	UInt32 i = 0;

	while (true) {
		FskPinDigitalDispatch aDispatch = FskExtensionGetByIndex(kFskExtensionPinDigital, i++);
		if (NULL == aDispatch)
			BAIL(kFskErrExtensionNotFound);

		if ((aDispatch->doCanHandle)(number, name, &number)) {
			dispatch = aDispatch;
			break;
		}
	}

	err = (dispatch->doNew)(pin, number, name, direction);
	BAIL_IF_ERR(err);

	(*pin)->dispatch = dispatch;

bail:
	return err;
}

FskErr FskPinAnalogNew(FskPinAnalog *pin, SInt32 number, const char *name)
{
	FskErr err;
	FskPinAnalogDispatch dispatch = NULL;
	UInt32 i = 0;

	while (true) {
		FskPinAnalogDispatch aDispatch = FskExtensionGetByIndex(kFskExtensionPinAnalog, i++);
		if (NULL == aDispatch)
			BAIL(kFskErrExtensionNotFound);

		if ((aDispatch->doCanHandle)(number, name, &number)) {
			dispatch = aDispatch;
			break;
		}
	}

	err = (dispatch->doNew)(pin, number, name);
	BAIL_IF_ERR(err);

	(*pin)->dispatch = dispatch;

bail:
	return err;
}

FskErr FskPinI2CNew(FskPinI2C *pin, SInt32 sda, SInt32 sclk, SInt32 bus)
{
	FskErr err;
	FskPinI2CDispatch dispatch = NULL;
	UInt32 i = 0;

	while (true) {
		SInt32 remappedBus = kFskPinI2CNoBus;
		FskPinI2CDispatch aDispatch = FskExtensionGetByIndex(kFskExtensionPinI2C, i++);
		if (NULL == aDispatch)
			BAIL(kFskErrExtensionNotFound);

		if ((aDispatch->doCanHandle)(sda, sclk, bus, &remappedBus)) {
			dispatch = aDispatch;
			break;
		}

		if (kFskPinI2CNoBus != remappedBus)
			bus = remappedBus;
	}

	err = (dispatch->doNew)(pin, sda, sclk, bus);
	BAIL_IF_ERR(err);

	(*pin)->dispatch = dispatch;

bail:
	return err;
}

FskErr FskPinPWMNew(FskPinPWM *pin, SInt32 number, const char *name)
{
	FskErr err;
	FskPinPWMDispatch dispatch = NULL;
	UInt32 i = 0;

	while (true) {
		FskPinPWMDispatch aDispatch = FskExtensionGetByIndex(kFskExtensionPinPWM, i++);
		if (NULL == aDispatch)
			BAIL(kFskErrExtensionNotFound);

		if ((aDispatch->doCanHandle)(number, name, &number)) {
			dispatch = aDispatch;
			break;
		}
	}

	err = (dispatch->doNew)(pin, number, name);
	BAIL_IF_ERR(err);

	(*pin)->dispatch = dispatch;

bail:
	return err;
}

FskErr FskPinSerialNew(FskPinSerial *pin, SInt32 rxNumber, SInt32 txNumber, const char *nameIn, SInt32 baud)
{
	FskErr err;
	const char *name = nameIn;
	FskPinSerialDispatch dispatch = NULL;
	UInt32 i = 0;

	while (true) {
		char *remappedName = NULL;
		FskPinSerialDispatch aDispatch = FskExtensionGetByIndex(kFskExtensionPinSerial, i++);
		if (NULL == aDispatch)
			BAIL(kFskErrExtensionNotFound);

		if ((aDispatch->doCanHandle)(rxNumber, txNumber, name, &remappedName)) {
			dispatch = aDispatch;
			break;
		}

		if (remappedName) {
			if (name != nameIn)
				FskMemPtrDispose((char *)name);
			name = remappedName;
		}
	}

	err = (dispatch->doNew)(pin, rxNumber, txNumber, name, baud);
	BAIL_IF_ERR(err);

	(*pin)->dispatch = dispatch;

bail:
	if (name != nameIn)
		FskMemPtrDispose((char *)name);

	return err;
}
