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

#include "mraa/i2c.h"		//@@

static Boolean mraaI2CCanHandle(SInt32 sda, SInt32 sclk, SInt32 bus, SInt32 *remappedBus);
static FskErr mraaI2CNew(FskPinI2C *pin, SInt32 sda, SInt32 sclk, SInt32 bus);
static void mraaI2CDispose(FskPinI2C pin);
static FskErr mraaI2CSetAddress(FskPinI2C pin, UInt8 address);
static FskErr mraaI2CReadByte(FskPinI2C pin, UInt8 *byte);
static FskErr mraaI2CReadBytes(FskPinI2C pin, SInt32 bufferSize, SInt32 *bytesRead, UInt8 *bytes);
static FskErr mraaI2CWriteByte(FskPinI2C pin, UInt8 byte);
static FskErr mraaI2CWriteBytes(FskPinI2C pin, SInt32 count, const UInt8 *bytes);
static FskErr mraaI2CReadDataByte(FskPinI2C pin, UInt8 command, UInt8 *byte);
static FskErr mraaI2CReadDataWord(FskPinI2C pin, UInt8 command, UInt16 *word);
static FskErr mraaI2CReadDataBytes(FskPinI2C pin, UInt8 command, SInt32 bufferSize, SInt32 *bytesRead, UInt8 *bytes);
static FskErr mraaI2CWriteDataByte(FskPinI2C pin, UInt8 command, UInt8 byte);
static FskErr mraaI2CWriteDataWord(FskPinI2C pin, UInt8 command, UInt16 word);
static FskErr mraaI2CWriteDataBytes(FskPinI2C pin, UInt8 command, SInt32 count, const UInt8 *bytes);

FskPinI2CDispatchRecord gMRAAPinI2C = {
	mraaI2CCanHandle,
	mraaI2CNew,
	mraaI2CDispose,
	mraaI2CSetAddress,
	mraaI2CReadByte,
	mraaI2CReadBytes,
	mraaI2CWriteByte,
	mraaI2CWriteBytes,
	mraaI2CReadDataByte,
	mraaI2CReadDataWord,
	mraaI2CReadDataBytes,
	mraaI2CWriteDataByte,
	mraaI2CWriteDataWord,
	mraaI2CWriteDataBytes,
	NULL
};

typedef struct {
	FskPinI2CRecord					pd;

	mraa_i2c_context				dev;
} mraaI2CRecord, *mraaI2C;

Boolean mraaI2CCanHandle(SInt32 sda, SInt32 sclk, SInt32 bus, SInt32 *remappedBus)
{
	return kFskPinI2CNoBus != bus;
}

FskErr mraaI2CNew(FskPinI2C *pin, SInt32 sda, SInt32 sclk, SInt32 bus)
{
	mraaI2C mi2c;
	mraa_i2c_context dev;
	FskErr err;

	dev = mraa_i2c_init(bus);
	if (!dev) return kFskErrOperationFailed;

	err = FskMemPtrNewClear(sizeof(mraaI2CRecord), &mi2c);
	if (err) {
		mraa_i2c_stop(dev);
		return err;
	}

	mi2c->dev = dev;
	*pin = (FskPinI2C)mi2c;

	return kFskErrNone;
}

void mraaI2CDispose(FskPinI2C pin)
{
	mraaI2C mi2c = (mraaI2C)pin;
	mraa_i2c_stop(mi2c->dev);
	FskMemPtrDispose(mi2c);
}

FskErr mraaI2CSetAddress(FskPinI2C pin, UInt8 address)
{
	mraaI2C mi2c = (mraaI2C)pin;
	mraa_result_t result = mraa_i2c_address(mi2c->dev, address);
	return (MRAA_SUCCESS == result) ? kFskErrNone : kFskErrOperationFailed;
}

FskErr mraaI2CReadByte(FskPinI2C pin, UInt8 *byte)
{
	mraaI2C mi2c = (mraaI2C)pin;
	*byte = mraa_i2c_read_byte(mi2c->dev);
	return kFskErrNone;
}

FskErr mraaI2CReadBytes(FskPinI2C pin, SInt32 bufferSize, SInt32 *bytesRead, UInt8 *bytes)
{
	mraaI2C mi2c = (mraaI2C)pin;
	*bytesRead = mraa_i2c_read(mi2c->dev, bytes, bufferSize);
	return (0 == *bytesRead) ? kFskErrOperationFailed : kFskErrNone;
}

FskErr mraaI2CWriteByte(FskPinI2C pin, UInt8 byte)
{
	mraaI2C mi2c = (mraaI2C)pin;
	mraa_result_t result = mraa_i2c_write_byte(mi2c->dev, byte);
	return (MRAA_SUCCESS == result) ? kFskErrNone : kFskErrOperationFailed;
}

FskErr mraaI2CWriteBytes(FskPinI2C pin, SInt32 count, const UInt8 *bytes)
{
	mraaI2C mi2c = (mraaI2C)pin;
	mraa_result_t result = mraa_i2c_write(mi2c->dev, bytes, count);
	return (MRAA_SUCCESS == result) ? kFskErrNone : kFskErrOperationFailed;
}

FskErr mraaI2CReadDataByte(FskPinI2C pin, UInt8 command, UInt8 *byte)
{
	mraaI2C mi2c = (mraaI2C)pin;
	*byte = mraa_i2c_read_byte_data(mi2c->dev, command);
	return kFskErrNone;
}

FskErr mraaI2CReadDataWord(FskPinI2C pin, UInt8 command, UInt16 *word)
{
	mraaI2C mi2c = (mraaI2C)pin;
	*word = mraa_i2c_read_word_data(mi2c->dev, command);
	return kFskErrNone;
}

FskErr mraaI2CReadDataBytes(FskPinI2C pin, UInt8 command, SInt32 bufferSize, SInt32 *bytesRead, UInt8 *bytes)
{
	mraaI2C mi2c = (mraaI2C)pin;
	*bytesRead = mraa_i2c_read_bytes_data(mi2c->dev, command, bytes, bufferSize);
	return (0 == *bytesRead) ? kFskErrOperationFailed : kFskErrNone;
}

FskErr mraaI2CWriteDataByte(FskPinI2C pin, UInt8 command, UInt8 byte)
{
	mraaI2C mi2c = (mraaI2C)pin;
	mraa_result_t result = mraa_i2c_write_byte_data(mi2c->dev, byte, command);
	return (MRAA_SUCCESS == result) ? kFskErrNone : kFskErrOperationFailed;
}

FskErr mraaI2CWriteDataWord(FskPinI2C pin, UInt8 command, UInt16 word)
{
	mraaI2C mi2c = (mraaI2C)pin;
	mraa_result_t result = mraa_i2c_write_word_data(mi2c->dev, word, command);
	return (MRAA_SUCCESS == result) ? kFskErrNone : kFskErrOperationFailed;
}

FskErr mraaI2CWriteDataBytes(FskPinI2C pin, UInt8 command, SInt32 count, const UInt8 *bytes)
{
	mraaI2C mi2c = (mraaI2C)pin;
	mraa_result_t result = mraa_i2c_write(mi2c->dev, bytes, count);
	return (MRAA_SUCCESS == result) ? kFskErrNone : kFskErrOperationFailed;
}

/*
	Extension
*/

FskExport(FskErr) FskPinI2CMRAA_fskLoad(FskLibrary library)
{
	mraa_result_t result;

	result = mraa_init();
	if ((result == MRAA_SUCCESS) || (result == MRAA_ERROR_PLATFORM_ALREADY_INITIALISED))
		return FskExtensionInstall(kFskExtensionPinI2C, &gMRAAPinI2C);
	else
		return kFskErrOperationFailed;
}

FskExport(FskErr) FskPinI2CMRAA_fskUnload(FskLibrary library)
{
	return FskExtensionUninstall(kFskExtensionPinI2C, &gMRAAPinI2C);
}
