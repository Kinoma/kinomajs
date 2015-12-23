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

#include "i2c-dev.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

static Boolean linuxI2CCanHandle(SInt32 sda, SInt32 sclk, SInt32 bus, SInt32 *remappedBus);
static FskErr linuxI2CNew(FskPinI2C *pin, SInt32 sda, SInt32 sclk, SInt32 bus);
static void linuxI2CDispose(FskPinI2C pin);
static FskErr linuxI2CSetAddress(FskPinI2C pin, UInt8 address);
static FskErr linuxI2CReadByte(FskPinI2C pin, UInt8 *byte);
static FskErr linuxI2CReadBytes(FskPinI2C pin, SInt32 bufferSize, SInt32 *bytesRead, UInt8 *bytes);
static FskErr linuxI2CWriteByte(FskPinI2C pin, UInt8 byte);
static FskErr linuxI2CWriteBytes(FskPinI2C pin, SInt32 count, const UInt8 *bytes);
static FskErr linuxI2CReadDataByte(FskPinI2C pin, UInt8 command, UInt8 *byte);
static FskErr linuxI2CReadDataWord(FskPinI2C pin, UInt8 command, UInt16 *word);
static FskErr linuxI2CReadDataBytes(FskPinI2C pin, UInt8 command, SInt32 bufferSize, SInt32 *bytesRead, UInt8 *bytes);
static FskErr linuxI2CWriteDataByte(FskPinI2C pin, UInt8 command, UInt8 byte);
static FskErr linuxI2CWriteDataWord(FskPinI2C pin, UInt8 command, UInt16 word);
static FskErr linuxI2CWriteDataBytes(FskPinI2C pin, UInt8 command, SInt32 count, const UInt8 *bytes);
static FskErr linuxI2CProcessCall(FskPinI2C pin, UInt8 command, UInt16 input, UInt16 *output);

FskPinI2CDispatchRecord gLinuxPinI2C = {
	linuxI2CCanHandle,
	linuxI2CNew,
	linuxI2CDispose,
	linuxI2CSetAddress,
	linuxI2CReadByte,
	linuxI2CReadBytes,
	linuxI2CWriteByte,
	linuxI2CWriteBytes,
	linuxI2CReadDataByte,
	linuxI2CReadDataWord,
	linuxI2CReadDataBytes,
	linuxI2CWriteDataByte,
	linuxI2CWriteDataWord,
	linuxI2CWriteDataBytes,
	linuxI2CProcessCall
};

typedef struct {
	FskPinI2CRecord					pd;
	int								bus;
} linuxI2CRecord, *linuxI2C;

#define kMaxI2cBus (10)
static int i2c_fd[kMaxI2cBus] = {0,0,0,0,0,0,0,0,0,0};
static int i2c_fd_count[kMaxI2cBus] = {0,0,0,0,0,0,0,0,0,0};
static int i2c_slave[kMaxI2cBus] = {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};

Boolean linuxI2CCanHandle(SInt32 sda, SInt32 sclk, SInt32 bus, SInt32 *remappedBus)
{
	return (kFskPinI2CNoBus != bus) && (bus < kMaxI2cBus);
}

FskErr linuxI2CNew(FskPinI2C *pin, SInt32 sda, SInt32 sclk, SInt32 bus)
{
	FskErr err;
	linuxI2C li2c;
    char i2cDev[80];

	err = FskMemPtrNewClear(sizeof(linuxI2CRecord), &li2c);
	if (err) return err;

	*pin = (FskPinI2C)li2c;
	li2c->bus = bus;

    if (i2c_fd[bus]) {
        i2c_fd_count[bus] += 1;
        return kFskErrNone;
    }

    sprintf(i2cDev, "%s%d", "/dev/i2c-", (int)bus);
	i2c_fd[bus] = open(i2cDev, O_RDWR);
    if (i2c_fd[bus] < 0)
        return kFskErrOperationFailed;

	i2c_slave[bus] = -1;
	i2c_fd_count[bus] = 1;

    return kFskErrNone;
}

void linuxI2CDispose(FskPinI2C pin)
{
	linuxI2C li2c = (linuxI2C)pin;

    i2c_fd_count[li2c->bus] -= 1;
    if (i2c_fd_count[li2c->bus] <= 0) {
		close(i2c_fd[li2c->bus]);
        i2c_fd[li2c->bus] = 0;
	}

	FskMemPtrDispose(li2c);
}

FskErr linuxI2CSetAddress(FskPinI2C pin, UInt8 address)
{
	linuxI2C li2c = (linuxI2C)pin;

    if (i2c_slave[li2c->bus] == address)
        return kFskErrNone;

     if ((ioctl(i2c_fd[li2c->bus], I2C_SLAVE, address)) < 0)
		return kFskErrOperationFailed;

	i2c_slave[li2c->bus] = address;

	return kFskErrNone;
}

typedef union i2c_smbus_data i2c_smbus_data;
struct i2c_smbus_ioctl_dataFSK {
    char                    read_write;
    __u8                    command;
    int                     size;
    void                    *data;      // i2c_smbus_data
};
typedef struct i2c_smbus_ioctl_dataFSK i2c_smbus_ioctl_dataFSK;

static __s32 doI2C(int file, char read_write, __u8 command, int size, void *data)
{
    i2c_smbus_ioctl_dataFSK parameters = {read_write, command, size, data};
    return ioctl(file, I2C_SMBUS, &parameters);
}


FskErr linuxI2CReadByte(FskPinI2C pin, UInt8 *byte)
{
	linuxI2C li2c = (linuxI2C)pin;
	int result = read(i2c_fd[li2c->bus], byte, 1);
	return (1 == result) ? kFskErrNone : kFskErrOperationFailed;
}

FskErr linuxI2CReadBytes(FskPinI2C pin, SInt32 bufferSize, SInt32 *bytesRead, UInt8 *bytes)
{
	linuxI2C li2c = (linuxI2C)pin;
	int result = read(i2c_fd[li2c->bus], bytes, bufferSize);
	if (result < 0)
		return kFskErrOperationFailed;

	*bytesRead = (SInt32)result;

	return kFskErrNone;
}

FskErr linuxI2CWriteByte(FskPinI2C pin, UInt8 byte)
{
	linuxI2C li2c = (linuxI2C)pin;
	int result = write(i2c_fd[li2c->bus], &byte, 1);
	return (1 == result) ? kFskErrNone : kFskErrOperationFailed;
}

FskErr linuxI2CWriteBytes(FskPinI2C pin, SInt32 count, const UInt8 *bytes)
{
	linuxI2C li2c = (linuxI2C)pin;
	int result = write(i2c_fd[li2c->bus], bytes, count);
	return (result == count) ? kFskErrNone : kFskErrOperationFailed;
}

FskErr linuxI2CReadDataByte(FskPinI2C pin, UInt8 command, UInt8 *byte)
{
	linuxI2C li2c = (linuxI2C)pin;
	union i2c_smbus_data data;
	int result = doI2C(i2c_fd[li2c->bus], I2C_SMBUS_READ, command, I2C_SMBUS_BYTE_DATA, &data);
	if (result)
		return kFskErrOperationFailed;

	*byte = data.byte & 0x0ff;

	return kFskErrNone;
}

FskErr linuxI2CReadDataWord(FskPinI2C pin, UInt8 command, UInt16 *word)
{
	linuxI2C li2c = (linuxI2C)pin;
    union i2c_smbus_data data;
    int result = doI2C(i2c_fd[li2c->bus], I2C_SMBUS_READ, command, I2C_SMBUS_WORD_DATA, &data);
    if (result)
		return kFskErrOperationFailed;

	*word = data.word & 0x0ffff;

	return kFskErrNone;
}

FskErr linuxI2CReadDataBytes(FskPinI2C pin, UInt8 command, SInt32 bufferSize, SInt32 *bytesRead, UInt8 *bytes)
{
	linuxI2C li2c = (linuxI2C)pin;
    union i2c_smbus_data data;
    int result;

	result = doI2C(i2c_fd[li2c->bus], I2C_SMBUS_READ, command, I2C_SMBUS_I2C_BLOCK_BROKEN, &data);
	if (result)
		return kFskErrOperationFailed;

	if (data.block[0] > bufferSize)
		data.block[0] = bufferSize;

	if (bytesRead)
		*bytesRead = data.block[0];

    memmove(bytes, &data.block[1], data.block[0]);

    return kFskErrNone;
}

FskErr linuxI2CWriteDataByte(FskPinI2C pin, UInt8 command, UInt8 byte)
{
	linuxI2C li2c = (linuxI2C)pin;
	union i2c_smbus_data data;
	int result;
	data.byte = byte;
	result = doI2C(i2c_fd[li2c->bus], I2C_SMBUS_WRITE, command, I2C_SMBUS_BYTE_DATA, &data);
	return (result < 0) ? kFskErrOperationFailed : kFskErrNone;
}

FskErr linuxI2CWriteDataWord(FskPinI2C pin, UInt8 command, UInt16 word)
{
	linuxI2C li2c = (linuxI2C)pin;
    union i2c_smbus_data data;
    int result;
   	data.word = word;
    result = doI2C(i2c_fd[li2c->bus], I2C_SMBUS_WRITE, command, I2C_SMBUS_WORD_DATA, &data);
	return (result < 0) ? kFskErrOperationFailed : kFskErrNone;
}

FskErr linuxI2CWriteDataBytes(FskPinI2C pin, UInt8 command, SInt32 count, const UInt8 *bytes)
{
	linuxI2C li2c = (linuxI2C)pin;
    union i2c_smbus_data data;
    int result;
	if (count > 32)
		return kFskErrRequestTooLarge;

    data.block[0] = count;
    memmove(&data.block[1], bytes, data.block[0]);

    result = doI2C(i2c_fd[li2c->bus], I2C_SMBUS_WRITE, command, I2C_SMBUS_I2C_BLOCK_BROKEN, &data);
	return (result < 0) ? kFskErrOperationFailed : kFskErrNone;
}

FskErr linuxI2CProcessCall(FskPinI2C pin, UInt8 command, UInt16 input, UInt16 *output)
{
	linuxI2C li2c = (linuxI2C)pin;
    union i2c_smbus_data data;
    int result;

    data.word = input;
    result = doI2C(i2c_fd[li2c->bus], I2C_SMBUS_WRITE, command, I2C_SMBUS_PROC_CALL, &data);
    if (result)
		return kFskErrOperationFailed;

    *output = data.word & 0x0ffff;

	return kFskErrNone;
}

/*
	Extension
*/

FskExport(FskErr) FskPinI2CLinux_fskLoad(FskLibrary library)
{
	return FskExtensionInstall(kFskExtensionPinI2C, &gLinuxPinI2C);
}

FskExport(FskErr) FskPinI2CLinux_fskUnload(FskLibrary library)
{
	return FskExtensionUninstall(kFskExtensionPinI2C, &gLinuxPinI2C);
}
