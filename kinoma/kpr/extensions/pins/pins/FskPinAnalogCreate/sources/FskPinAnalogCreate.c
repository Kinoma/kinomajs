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

/*
	Kinoma Create - Back pins
		Remaps from Kinoma pin numbers to Linux pin numbers
*/

static Boolean createBackAnalogCanHandle(SInt32 number, const char *name, SInt32 *remappedNumber);
FskPinAnalogDispatchRecord gCreateBackPinAnalog = {
	createBackAnalogCanHandle,
	NULL,
	NULL,
	NULL
};

/*
	Kinoma Create - Front Pins
*/

static Boolean createFrontAnalogCanHandle(SInt32 number, const char *name, SInt32 *remappedNumber);
static FskErr createFrontAnalogNew(FskPinAnalog *pin, SInt32 number, const char *name);
static void createFrontAnalogDispose(FskPinAnalog pin);
static FskErr createFrontAnalogRead(FskPinAnalog pin, double *value);

FskPinAnalogDispatchRecord gCreateFrontPinAnalog = {
	createFrontAnalogCanHandle,
	createFrontAnalogNew,
	createFrontAnalogDispose,
	createFrontAnalogRead
};

typedef struct {
	FskPinAnalogRecord		pd;

	int						pin;
	int						address;
	FskPinI2C				i2c;
} createFrontAnalogRecord, *createFrontAnalog;

static const int I2CBUS = 0;			//@@ move from here
static const uint8_t a2dAddr1 = 0x20;	//@@ move from here
static const uint8_t a2dAddr2 = 0x21;	//@@ move from here
static const uint8_t a2dpins[8] = {0x16, 0x18, 0x1A, 0x1C, 0x1E, 0x20, 0x22, 0x24};

Boolean createFrontAnalogCanHandle(SInt32 number, const char *name, SInt32 *remappedNumber)
{
	return FskHardwarePinsMux(number, kFskHardwarePinADC) >= FRONTOFFSET;
}

FskErr createFrontAnalogNew(FskPinAnalog *pin, SInt32 number, const char *name)
{
	FskErr err;
	createFrontAnalog cfa = NULL;

	err = FskMemPtrNewClear(sizeof(createFrontAnalogRecord), &cfa);
	if (err) return err;

	err = FskPinI2CNew(&cfa->i2c, 0, 0, I2CBUS);
	if (err) {
		FskMemPtrDispose(cfa);
		return err;
	}

	cfa->pin = FskHardwarePinsMux(number, kFskHardwarePinADC) - FRONTOFFSET;
	cfa->address = (cfa->pin < 8) ? a2dAddr1 : a2dAddr2;
	cfa->pin &= 7;

	FskHardwarePinsDoMux(cfa->pin, cfa->address, 3);

	*pin = (FskPinAnalog)cfa;

	return err;
}

void createFrontAnalogDispose(FskPinAnalog pin)
{
	createFrontAnalog cfa = (createFrontAnalog)pin;
	FskPinI2CDispose(cfa->i2c);
	FskMemPtrDispose(cfa);
}

FskErr createFrontAnalogRead(FskPinAnalog pin, double *value)
{
	createFrontAnalog cfa = (createFrontAnalog)pin;
	FskErr err;
	UInt8 readValue;

	err = FskPinI2CSetAddress(cfa->i2c, cfa->address);
	if (err) return err;

	err = FskPinI2CReadDataByte(cfa->i2c, a2dpins[cfa->pin], &readValue);
	if (err) return err;

	*value = ((double)readValue) / 1023.0;

	return kFskErrNone;
}

Boolean createBackAnalogCanHandle(SInt32 number, const char *name, SInt32 *remappedNumber)
{
	switch (number) {
		case 38: *remappedNumber = 51; return true;
		case 37: *remappedNumber = 52; return true;
		case 40: *remappedNumber = 53; return true;
		case 39: *remappedNumber = 54; return true;
		case 44: *remappedNumber = 55; return true;
		case 43: *remappedNumber = 56; return true;
		case 48: *remappedNumber = 57; return true;
		case 47: *remappedNumber = 58; return true;
	}

	return true;
}

/*
	Extension
*/

FskExport(FskErr) FskPinAnalogCreate_fskLoad(FskLibrary library)
{
	FskExtensionInstall(kFskExtensionPinAnalog, &gCreateFrontPinAnalog);
	return FskExtensionInstall(kFskExtensionPinAnalog, &gCreateBackPinAnalog);
}

FskExport(FskErr) FskPinAnalogCreate_fskUnload(FskLibrary library)
{
	FskExtensionUninstall(kFskExtensionPinAnalog, &gCreateFrontPinAnalog);
	return FskExtensionUninstall(kFskExtensionPinAnalog, &gCreateBackPinAnalog);
}
