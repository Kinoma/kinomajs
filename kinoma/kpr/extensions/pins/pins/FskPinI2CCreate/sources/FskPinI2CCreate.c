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

#include "FskPin.h"

#include <unistd.h>

static const uint8_t FFaddy1 = 0x20;
static const uint8_t FFaddy2 = 0x21;

static const uint8_t I2CDELAY = 50;
static const uint8_t I2CDATA = 0x26;

static const uint8_t SMBTYPE = 1;
static const uint8_t NOTSMBTYPE = 0;
static const uint8_t BLOCK = 1;
static const uint8_t NOTBLOCK = 0;
static const uint8_t READ = 1;
static const uint8_t WRITE = 0;

static const int LP = 101;
static const int RP = 102;

static Boolean createI2CCanHandle(SInt32 sda, SInt32 sclk, SInt32 bus, SInt32 *remappedBus);
static FskErr createI2CNew(FskPinI2C *pin, SInt32 sda, SInt32 sclk, SInt32 bus);
static void createI2CDispose(FskPinI2C pin);
static FskErr createI2CSetAddress(FskPinI2C pin, UInt8 address);
static FskErr createI2CReadByte(FskPinI2C pin, UInt8 *byte);
static FskErr createI2CReadBytes(FskPinI2C pin, SInt32 bufferSize, SInt32 *bytesRead, UInt8 *bytes);
static FskErr createI2CWriteByte(FskPinI2C pin, UInt8 byte);
static FskErr createI2CWriteBytes(FskPinI2C pin, SInt32 count, const UInt8 *bytes);
static FskErr createI2CReadDataByte(FskPinI2C pin, UInt8 command, UInt8 *byte);
static FskErr createI2CReadDataWord(FskPinI2C pin, UInt8 command, UInt16 *word);
static FskErr createI2CReadDataBytes(FskPinI2C pin, UInt8 command, SInt32 bufferSize, SInt32 *bytesRead, UInt8 *bytes);
static FskErr createI2CWriteDataByte(FskPinI2C pin, UInt8 command, UInt8 byte);
static FskErr createI2CWriteDataWord(FskPinI2C pin, UInt8 command, UInt16 word);
static FskErr createI2CWriteDataBytes(FskPinI2C pin, UInt8 command, SInt32 count, const UInt8 *bytes);

FskPinI2CDispatchRecord gCreatePinI2C = {
	createI2CCanHandle,
	createI2CNew,
	createI2CDispose,
	createI2CSetAddress,
	createI2CReadByte,
	createI2CReadBytes,
	createI2CWriteByte,
	createI2CWriteBytes,
	createI2CReadDataByte,
	createI2CReadDataWord,
	createI2CReadDataBytes,
	createI2CWriteDataByte,
	createI2CWriteDataWord,
	createI2CWriteDataBytes,
	NULL
};

typedef struct {
	FskPinI2CRecord					pd;
	int								address;
	int								bus;
	FskPinI2C						i2c;
} createI2CRecord, *createI2C;

Boolean createI2CCanHandle(SInt32 sda, SInt32 sclk, SInt32 bus, SInt32 *remappedBus)
{
	int sdaDev, clkDev;

	if ((kFskPinI2CNoBus != bus) && (bus < 100))
		return false;

	sdaDev = FskHardwarePinsMux(sda, kFskHardwarePinI2CData);
	clkDev = FskHardwarePinsMux(sclk, kFskHardwarePinI2CClock);

	if (sdaDev > 100) {
		int slaveAddy = 0;
		int pinNum = 0;
		sdaDev -= 100;

		if (sdaDev <= 7) {
			slaveAddy = FFaddy1;
			pinNum = sdaDev;
			sdaDev = LP;
		}else {
			slaveAddy = FFaddy2;
			pinNum = sdaDev - 8;
			sdaDev = RP;
		}

		FskHardwarePinsDoMux(pinNum, slaveAddy, 7);
	}

	if (clkDev > 100) {
		int slaveAddy = 0;
		int pinNum = 0;
		clkDev -= 100;

		if (clkDev <= 7) {
			slaveAddy = FFaddy1;
			pinNum = clkDev;
			clkDev = LP;
		} else {
			slaveAddy = FFaddy2;
			pinNum = clkDev - 8;
			clkDev = RP;
		}

		FskHardwarePinsDoMux(pinNum, slaveAddy, 6);
	}

	if (sdaDev < 0 || clkDev < 0)
		return false;

//@@ if sdaDev != clkDev - error??

	*remappedBus = sdaDev;

	if (sdaDev < 100)
		return false;

	return true;
}

FskErr createI2CNew(FskPinI2C *pin, SInt32 sda, SInt32 sclk, SInt32 bus)
{
	FskErr err;
	createI2C ci2c;

	if (kFskPinI2CNoBus == bus) {
		if (!createI2CCanHandle(sda, sclk, bus, &bus))
			return kFskErrUnimplemented;
		if (bus < 100)
			return kFskErrUnimplemented;
	}

	err = FskMemPtrNewClear(sizeof(createI2CRecord), &ci2c);
	if (err) return err;

	*pin = (FskPinI2C)ci2c;
	ci2c->bus = bus;

	return FskPinI2CNew(&ci2c->i2c, 0, 0, 0);
}

void createI2CDispose(FskPinI2C pin)
{
	createI2C ci2c = (createI2C)pin;

	FskPinI2CDispose(ci2c->i2c);
}

FskErr createI2CSetAddress(FskPinI2C pin, UInt8 address)
{
	createI2C ci2c = (createI2C)pin;

	ci2c->address = address;

	return kFskErrNone;
}

static int getFFAddress(int busNum) {
	if (busNum == LP) return FFaddy1;
	if (busNum == RP) return FFaddy2;
	return 0;
}

static FskErr triggerFrontI2C(createI2C ci2c, uint8_t readBit, uint8_t command, uint8_t byteCount, uint8_t SMB, uint8_t block)
{
	FskErr err;
	int slave = getFFAddress(ci2c->bus);
	int address = (ci2c->address << 1) + readBit;
	uint8_t go = 1;
	UInt8 check = 1;

	go |= (SMB << 1) | (block << 2);

	err = FskPinI2CSetAddress(ci2c->i2c, slave);
	if (err) return err;
	err = FskPinI2CWriteDataByte(ci2c->i2c, 0x11, address);
	if (err) return err;
	err = FskPinI2CWriteDataByte(ci2c->i2c, 0x12, command);
	if (err) return err;
	err = FskPinI2CWriteDataByte(ci2c->i2c, 0x13, byteCount);
	if (err) return err;
	err = FskPinI2CWriteDataByte(ci2c->i2c, 0x10, go);
	if (err) return err;

	while (true) {
		err = FskPinI2CReadDataByte(ci2c->i2c, 0x10, &check);
		if (err) return err;

		if (!(check & 1))
			break;

#if !TARGET_OS_WIN32
		usleep(I2CDELAY);
#endif
	}

	return (check & 0x80) ? kFskErrUnknown : kFskErrNone;
}

static FskErr readBlockFromFrontI2C(createI2C ci2c, SInt32 bufferSize, SInt32 *count, UInt8 *values)
{
	UInt8 onAddress = I2CDATA;
	int i;
	UInt8 byte;
	FskErr err;

	if (*count <= 0) { //SMB block read -- need to read the length and report it back.
		err = FskPinI2CReadDataByte(ci2c->i2c, onAddress++, &byte);
		if (err) return err;
		*count = byte;
	}

	if (*count <= 0) return kFskErrOperationFailed;

	for (i = 0; i < *count; i++) {
		err = FskPinI2CReadDataByte(ci2c->i2c, onAddress + i, &byte);
		if (err) return err;

		if (i < bufferSize)
			values[i] = byte;
	}

	return kFskErrNone;
}

static FskErr writeBlockToFrontI2C(createI2C ci2c, int count, const UInt8 *values)
{
	int i;

	if (count < 1) return kFskErrInvalidParameter;

	for (i = 0; i < count; i++){
		FskErr err = FskPinI2CWriteDataByte(ci2c->i2c, (I2CDATA + i), *(values+i));
		if (err) return err;
	}

	return kFskErrNone;
}

FskErr createI2CReadByte(FskPinI2C pin, UInt8 *byte)
{
	createI2C ci2c = (createI2C)pin;
	FskErr err;

	err = triggerFrontI2C(ci2c, READ, 0, 1, NOTSMBTYPE, NOTBLOCK);
	if (err) return err;

	return FskPinI2CReadDataByte(ci2c->i2c, I2CDATA, byte);
}

/*
	Will this work? - @@ wasn't previously implemented for Front Pins
*/
FskErr createI2CReadBytes(FskPinI2C pin, SInt32 bufferSize, SInt32 *bytesRead, UInt8 *bytes)
{
	createI2C ci2c = (createI2C)pin;
	FskErr err = triggerFrontI2C(ci2c, READ, 0, bufferSize, NOTSMBTYPE, BLOCK);
	if (err) return err;
	*bytesRead = 0;
	return readBlockFromFrontI2C(ci2c, bufferSize, bytesRead, bytes);
}

FskErr createI2CWriteByte(FskPinI2C pin, UInt8 byte)
{
	createI2C ci2c = (createI2C)pin;
	FskErr err = FskPinI2CWriteDataByte(ci2c->i2c, I2CDATA, byte);
	if (err) return err;

	return triggerFrontI2C(ci2c, WRITE, 0, 1, NOTSMBTYPE, NOTBLOCK);
}

FskErr createI2CWriteBytes(FskPinI2C pin, SInt32 count, const UInt8 *bytes)
{
	createI2C ci2c = (createI2C)pin;
	FskErr err = writeBlockToFrontI2C(ci2c, count, bytes);
	if (err) return err;
	return triggerFrontI2C(ci2c, WRITE, 0, count, NOTSMBTYPE, BLOCK);
}

FskErr createI2CReadDataByte(FskPinI2C pin, UInt8 command, UInt8 *byte)
{
	createI2C ci2c = (createI2C)pin;
	FskErr err = triggerFrontI2C(ci2c, READ, command, 1, SMBTYPE, NOTBLOCK);
	if (err) return err;
	return FskPinI2CReadDataByte(ci2c->i2c, I2CDATA, byte);
}

FskErr createI2CReadDataWord(FskPinI2C pin, UInt8 command, UInt16 *word)
{
	createI2C ci2c = (createI2C)pin;
	UInt8 byte;
	FskErr err = triggerFrontI2C(ci2c, READ, command, 2, SMBTYPE, NOTBLOCK);
	if (err) return err;
	err = FskPinI2CReadDataByte(ci2c->i2c, I2CDATA + 1, &byte);
	if (err) return err;
	*word = byte << 8;
	err = FskPinI2CReadDataByte(ci2c->i2c, I2CDATA, &byte);
	if (err) return err;
	*word |= byte;
	return kFskErrNone;
}

FskErr createI2CReadDataBytes(FskPinI2C pin, UInt8 command, SInt32 bufferSize, SInt32 *bytesRead, UInt8 *bytes)
{
	createI2C ci2c = (createI2C)pin;
	FskErr err = triggerFrontI2C(ci2c, READ, command, bufferSize, SMBTYPE, BLOCK);
	if (err) return err;
	*bytesRead = 0;
	return readBlockFromFrontI2C(ci2c, bufferSize, bytesRead, bytes);
}

FskErr createI2CWriteDataByte(FskPinI2C pin, UInt8 command, UInt8 byte)
{
	createI2C ci2c = (createI2C)pin;
	FskErr err = FskPinI2CWriteDataByte(ci2c->i2c, I2CDATA, byte);
	if (err) return err;
	return triggerFrontI2C(ci2c, WRITE, command, 1, SMBTYPE, NOTBLOCK);
}

FskErr createI2CWriteDataWord(FskPinI2C pin, UInt8 command, UInt16 word)
{
	createI2C ci2c = (createI2C)pin;
	FskErr err = FskPinI2CWriteDataWord(ci2c->i2c, 0x2C, word);		//@@ no constant? why is this not I2CDATA?
	if (err) return err;
	return triggerFrontI2C(ci2c, WRITE, command, 2, SMBTYPE, NOTBLOCK);
}

FskErr createI2CWriteDataBytes(FskPinI2C pin, UInt8 command, SInt32 count, const UInt8 *bytes)
{
	createI2C ci2c = (createI2C)pin;
	FskErr err = writeBlockToFrontI2C(ci2c, count, bytes);
	if (err) return err;
	return triggerFrontI2C(ci2c, WRITE, command, count, SMBTYPE, BLOCK);
}

/*
	Extension
*/

FskExport(FskErr) FskPinI2CCreate_fskLoad(FskLibrary library)
{
	return FskExtensionInstall(kFskExtensionPinI2C, &gCreatePinI2C);
}

FskExport(FskErr) FskPinI2CCreate_fskUnload(FskLibrary library)
{
	return FskExtensionUninstall(kFskExtensionPinI2C, &gCreatePinI2C);
}
