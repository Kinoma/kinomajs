/*
 *     Copyright (C) 2010-2016 Marvell International Ltd.
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

#include "FskMemory.h"

static const int pinGo = 0x03;

static int pinConfigRegs[8] = { 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B };


static Boolean createBackDigitalCanHandle(SInt32 number, const char *name, SInt32 *remappedNumber);
FskPinDigitalDispatchRecord gCreateBackPinDigital = {
	createBackDigitalCanHandle,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

static Boolean createFrontDigitalCanHandle(SInt32 number, const char *name, SInt32 *remappedNumber);
static FskErr createFrontDigitalNew(FskPinDigital *pin, SInt32 number, const char *name, FskPinsDigitalDirection direction);
static void createFrontDigitalDispose(FskPinDigital pin);
static FskErr createFrontDigitalSetDirection(FskPinDigital pin, FskPinsDigitalDirection direction);
static FskErr createFrontDigitalGetDirection(FskPinDigital pin, FskPinsDigitalDirection *direction);
static FskErr createFrontDigitalRead(FskPinDigital pin, Boolean *value);
static FskErr createFrontDigitalWrite(FskPinDigital pin, Boolean value);

FskPinDigitalDispatchRecord gCreateFrontPinDigital = {
	createFrontDigitalCanHandle,
	createFrontDigitalNew,
	createFrontDigitalDispose,
	createFrontDigitalSetDirection,
	createFrontDigitalGetDirection,
	createFrontDigitalRead,
	createFrontDigitalWrite,
	NULL
};

Boolean createBackDigitalCanHandle(SInt32 number, const char *name, SInt32 *remappedNumber)
{
	int pin = FskHardwarePinsMux(number, kFskHardwarePinGPIO);
	if (pin >= 0)
		*remappedNumber = (SInt32)pin;

	return false;
}


typedef struct {
	FskPinDigitalRecord		pd;

	int						pinNumber;
	FskPinsDigitalDirection	direction;
	FskPinI2C				i2c;
	UInt8					address;
} createFrontDigitalRecord, *createFrontDigital;

static const int I2CBUS = 0;			//@@ move from here
static const int FFDIGITALIN = 0x14;	//@@ move from here
static const int FFDIGITALOUT = 0x15;	//@@ move from here

Boolean createFrontDigitalCanHandle(SInt32 number, const char *name, SInt32 *remappedNumber)
{
	return FskHardwarePinsMux(number, kFskHardwarePinGPIO) >= FRONTOFFSET;
}

FskErr createFrontDigitalNew(FskPinDigital *pin, SInt32 number, const char *name, FskPinsDigitalDirection direction)
{
	FskErr err;
	createFrontDigital cfd = NULL;
	int pinNumber;

	err = FskMemPtrNewClear(sizeof(createFrontDigitalRecord), &cfd);
	BAIL_IF_ERR(err);

	pinNumber = FskHardwarePinsMux(number, kFskHardwarePinGPIO) - FRONTOFFSET;
	cfd->pinNumber = pinNumber & 0x07;
	cfd->address = (pinNumber < 8) ? 0x20 : 0x21;

	err = FskPinI2CNew(&cfd->i2c, 0, 0, I2CBUS);
	if (err) {
		FskMemPtrDispose(cfd);
		return err;
	}

	err = createFrontDigitalSetDirection((FskPinDigital)cfd, direction);

bail:
	*pin = (FskPinDigital)cfd;

	return err;
}

void createFrontDigitalDispose(FskPinDigital pin)
{
	createFrontDigital cfd = (createFrontDigital)pin;
	FskPinI2CDispose(cfd->i2c);
	FskMemPtrDispose(cfd);
}

FskErr createFrontDigitalSetDirection(FskPinDigital pin, FskPinsDigitalDirection direction)
{
	FskErr err;
	UInt8 check = 1;
	createFrontDigital cfd = (createFrontDigital)pin;

	err = FskPinI2CSetAddress(cfd->i2c, cfd->address);
	if (err) return err;

	if (kFskPinDigitalDirectionIn == direction)
		err = FskPinI2CWriteDataByte(cfd->i2c, pinConfigRegs[cfd->pinNumber], 4);
	else if (kFskPinDigitalDirectionOut == direction)
		err = FskPinI2CWriteDataByte(cfd->i2c, pinConfigRegs[cfd->pinNumber], 5);
	if (err) return err;

	err = FskPinI2CWriteDataByte(cfd->i2c, pinGo, 1);
	if (err) return err;

	while (check > 0){
		err = FskPinI2CReadDataByte(cfd->i2c, pinGo, &check);
		if (err) return err;
	}

	cfd->direction = direction;

	return kFskErrNone;
}

FskErr createFrontDigitalGetDirection(FskPinDigital pin, FskPinsDigitalDirection *direction)
{
	createFrontDigital cfd = (createFrontDigital)pin;

	*direction = cfd->direction;

	return kFskErrNone;
}

FskErr createFrontDigitalRead(FskPinDigital pin, Boolean *value)
{
	createFrontDigital cfd = (createFrontDigital)pin;
	FskErr err;
	int readFrom;
	UInt8 b;

	if (cfd->direction == kFskPinDigitalDirectionOut)
		readFrom = FFDIGITALOUT;
	else
		readFrom = FFDIGITALIN;

	err = FskPinI2CSetAddress(cfd->i2c, cfd->address);
	if (err) return err;

	err = FskPinI2CReadDataByte(cfd->i2c, readFrom, &b);
	if (err) return err;

	*value = (b & (1 << cfd->pinNumber)) ? true : false;

	return kFskErrNone;
}

FskErr createFrontDigitalWrite(FskPinDigital pin, Boolean value)
{
	createFrontDigital cfd = (createFrontDigital)pin;
	FskErr err;
	UInt8 b;

	err = FskPinI2CSetAddress(cfd->i2c, cfd->address);
	if (err) return err;

	err = FskPinI2CReadDataByte(cfd->i2c, FFDIGITALIN, &b);
	if (err) return err;

	if (value)
		b |= (1 << cfd->pinNumber);
	else
		b &= ~(1 << cfd->pinNumber);

	return FskPinI2CWriteDataByte(cfd->i2c, FFDIGITALOUT, b);
}

/*
	Extension
*/

FskExport(FskErr) FskPinDigitalCreate_fskLoad(FskLibrary library)
{
	FskExtensionInstall(kFskExtensionPinDigital, &gCreateBackPinDigital);
	return FskExtensionInstall(kFskExtensionPinDigital, &gCreateFrontPinDigital);
}

FskExport(FskErr) FskPinDigitalCreate_fskUnload(FskLibrary library)
{
	FskExtensionUninstall(kFskExtensionPinDigital, &gCreateBackPinDigital);
	return FskExtensionUninstall(kFskExtensionPinDigital, &gCreateFrontPinDigital);
}
