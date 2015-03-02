/*
     Copyright (C) 2010-2015 Marvell International Ltd.
     Copyright (C) 2002-2010 Kinoma, Inc.

     Licensed under the Apache License, Version 2.0 (the "License");
     you may not use this file except in compliance with the License.
     You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

     Unless required by applicable law or agreed to in writing, software
     distributed under the License is distributed on an "AS IS" BASIS,
     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
     See the License for the specific language governing permissions and
     limitations under the License.
*/
#ifdef USEI2C

#define __FSKECMASCRIPT_PRIV__
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include "kprPins.h"

#include "FskECMAScript.h"
#include "FskExtensions.h"

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
static int FFBUS = 0;

FskExport(FskErr) i2c_fskLoad(FskLibrary library)
{
	return kFskErrNone;
}

FskExport(FskErr) i2c_fskUnload(FskLibrary library)
{
	return kFskErrNone;
}

#if SUPPORT_INSTRUMENTATION
   FskInstrumentedSimpleType(I2C, "I2C");
   // gI2CTypeInstrumentation
   #define         DBG_I2C(...)   do { FskInstrumentedTypePrintfDebug (&gI2CTypeInstrumentation, __VA_ARGS__); } while(0);
#else
    #define         DBG_I2C(format, ...)
#endif  // SUPPORT_INSTRUMENTATION

#define bailIfError(X) { err = (X); if (err != kFskErrNone) goto bail; }
#define bailIfNoFile(X) { if ((X) == NULL) { err = kFskErrFileNotFound; goto bail; } }
#define bailIfNull(X) { if ((X) == NULL) { err = kFskErrMemFull; goto bail; } }

static int getFFAddress(int busNum) {
	if (busNum == LP) return FFaddy1;
	if (busNum == RP) return FFaddy2;
	return 0;
}

FskErr FskI2COpen(int busNum)
{
    FskErr err;

    if (busNum < 100) {
        err = FskI2CDevOpen(busNum);
        return err;
    }
  
    return kFskErrNone;
}

static FskErr FskI2CClose(int busNum)
{
    if (busNum < 100)
        return FskI2CDevClose(busNum);

    return kFskErrNone;
}

static int ffSlave = 0;

FskErr FskI2CSetSlave(int busNum, uint8_t address)
{
    if (busNum < 100)
        return FskI2CDevSetSlave(busNum, address);

    ffSlave = address << 1;

    return kFskErrNone;
}

static FskErr TriggerFrontI2C(int busNum, uint8_t readBit, uint8_t reg, uint8_t byteCount, uint8_t SMB, uint8_t block){
	FskErr err = kFskErrNone; //TODO actually check this
	int slave = getFFAddress(busNum);
	int address = ffSlave + readBit;
	uint8_t go = 1;
	int check = 1;

	go |= (SMB << 1) | (block << 2);

	printf("Triggering front i2c with readBit %u reg %u byteCount %u SMB %u block %u go %u\n", readBit, reg, byteCount, SMB, block, go);

	err = FskI2CDevSetSlave(FFBUS, slave);
	err = FskI2CDevWriteByteDataSMB(FFBUS, 0x11, address);
	err = FskI2CDevWriteByteDataSMB(FFBUS, 0x12, reg);
	err = FskI2CDevWriteByteDataSMB(FFBUS, 0x13, byteCount);
	err = FskI2CDevWriteByteDataSMB(FFBUS, 0x10, go);

	while ((check & 1) == 1) {
		check = FskI2CDevReadByteDataSMB(FFBUS, 0x10);
#if !(TARGET_OS_WIN32)
		usleep(I2CDELAY);
#endif
	}

	if (check & 0x80) return kFskErrUnknown;

	return err;
}

static FskErr ReadBlockFromFrontI2C(int* count, uint8_t* values){
	uint8_t onAddress = I2CDATA;
	int i;

	if (*count <= 0){ //SMB block read -- need to read the length and report it back.
		*count = FskI2CDevReadByteDataSMB(FFBUS, onAddress);
		onAddress++;
	}

	if (*count <= 0) return kFskErrOperationFailed;

	for (i = 0; i < *count; i++){
		int test;
		test = FskI2CDevReadByteDataSMB(FFBUS, (onAddress + i));
		if (test < 0) return kFskErrOperationFailed;
		*(values + i) = test;
	}
	return kFskErrNone;
}

static FskErr WriteBlockToFrontI2C(int count, uint8_t* values){
	FskErr err = kFskErrNone;
	int i;

	if (count < 1) return kFskErrInvalidParameter;

	for (i = 0; i < count; i++){
		err = FskI2CDevWriteByteDataSMB(FFBUS, (I2CDATA + i), *(values+i));
		if (err != kFskErrNone) return kFskErrOperationFailed;
	}
	return err;
}

static int32_t FskI2CReadByte(int busNum, I2C_BusType i2cBusType)
{
	FskErr err = kFskErrNone;
	if (busNum < 100){
		return FskI2CDevReadByte(busNum, i2cBusType);
	}else{
		err = TriggerFrontI2C(busNum, READ, 0, 1, NOTSMBTYPE, NOTBLOCK);
		return FskI2CDevReadByteDataSMB(FFBUS, I2CDATA);
	}
}
static FskErr FskI2CWriteByte(int busNum, I2C_BusType i2cBusType, uint8_t data)
{
	FskErr err = kFskErrNone;
    if (busNum < 100){
        return FskI2CDevWriteByte(busNum, i2cBusType, data);
    }else{
    	err = FskI2CDevWriteByteDataSMB(FFBUS, I2CDATA, data);
    	return TriggerFrontI2C(busNum, WRITE, 0, 1, NOTSMBTYPE, NOTBLOCK);
    }
}

int32_t FskI2CReadByteDataSMB(int busNum, uint8_t reg) {
	FskErr err = kFskErrNone;
	if (busNum < 100) {
		return FskI2CDevReadByteDataSMB(busNum, reg);
	} else {
		err = TriggerFrontI2C(busNum, READ, reg, 1, SMBTYPE, NOTBLOCK);
		return FskI2CDevReadByteDataSMB(FFBUS, I2CDATA);
	}
}

FskErr FskI2CWriteByteDataSMB(int busNum, uint8_t reg, uint8_t data) {
  FskErr err = kFskErrNone;
  if (busNum < 100) {
	  return FskI2CDevWriteByteDataSMB(busNum, reg, data);
  } else {
	  err = FskI2CDevWriteByteDataSMB(FFBUS, I2CDATA, data);
	  return TriggerFrontI2C(busNum, WRITE, reg, 1, SMBTYPE, NOTBLOCK);
  }
}

static FskErr FskI2CWriteQuickSMB(int busNum, uint8_t value)
{
    if (busNum < 100)
        return FskI2CDevWriteQuickSMB(busNum, value);

    return kFskErrUnimplemented;
}

int32_t FskI2CReadWordDataSMB(int busNum, uint8_t reg){
	FskErr err = kFskErrNone; //TODO and this one
	if (busNum < 100) {
		return FskI2CDevReadWordDataSMB(busNum, reg);
	} else {
		int data = 0;
		err = TriggerFrontI2C(busNum, READ, reg, 2, SMBTYPE, NOTBLOCK);
		data = FskI2CDevReadByteDataSMB(FFBUS, I2CDATA + 1);
		data = data << 8;
		data |= FskI2CDevReadByteDataSMB(FFBUS, I2CDATA);
		return data;
	}
}

static FskErr FskI2CWriteWordDataSMB(int busNum, uint8_t reg, uint16_t value) {
	FskErr err = kFskErrNone;

	if (busNum < 100) {
		return FskI2CDevWriteWordDataSMB(busNum, reg, value);
	} else {
		err = FskI2CDevWriteWordDataSMB(FFBUS, 0x2C, value);
		err = TriggerFrontI2C(busNum, WRITE, reg, 2, SMBTYPE, NOTBLOCK);
		return err;
	}
}

static int32_t FskI2CProcessCallSMB(int busNum, uint8_t reg, uint16_t value)
{
	if (busNum < 100)
		return FskI2CDevProcessCallSMB(busNum, reg, value);

    return kFskErrUnimplemented;
}

static int FskI2CReadBlockDataSMB(int busNum, uint8_t reg, uint8_t length, uint8_t *values)
{
	FskErr err = kFskErrNone;

    if (busNum < 100){
        return FskI2CDevReadBlockDataSMB(busNum, reg, length, values);
    }else{
    	int count = 0;
    	err = TriggerFrontI2C(busNum, READ, reg, length, SMBTYPE, BLOCK);
    	if (err != kFskErrNone) return 0;
    	err = ReadBlockFromFrontI2C(&count, values);
    	if (err != kFskErrNone) return 0;
    	return count;
    }
}

static FskErr FskI2CWriteBlock(int busNum, I2C_BusType i2cBusType, uint8_t length, uint8_t *values){
	FskErr err = kFskErrNone;
	if (busNum < 100){
		return FskI2CDevWriteBlock(busNum, i2cBusType, length, values);
	}else{
		err = WriteBlockToFrontI2C(length, values);
		if (err != kFskErrNone) return err;
		err = TriggerFrontI2C(busNum, WRITE, 0, length, NOTSMBTYPE, BLOCK);
		return err;
	}
}

static FskErr FskI2CWriteBlockDataSMB(int busNum, uint8_t reg, uint8_t length, uint8_t *values)
{
	FskErr err = kFskErrNone;
    if (busNum < 100){
        return FskI2CDevWriteBlockDataSMB(busNum, reg, length, values);
    }else{
    	err = WriteBlockToFrontI2C(length, values);
		if (err != kFskErrNone) return err;
		err = TriggerFrontI2C(busNum, WRITE, 0, length, NOTSMBTYPE, BLOCK);
		return err;
    }
}

typedef struct {
    int sdaPin;
    int clkPin;
    int sdaDev;
    int clkDev;
    int bus;
    uint8_t address;
#if SUPPORT_XS_DEBUG
    char diagnosticID[512];
#endif
} xsI2CRecord, *xsI2C;

static unsigned char *writeOne(xsMachine *the, xsI2C i2c, xsSlot *slot, unsigned char *bufPtr, unsigned char *bufEnd);

void xs_i2c(void *data)
{
    if (data) {
        xsI2C i2c = data;
        FskI2CClose(i2c->bus);
        FskMemPtrDispose(i2c);
    }
}

void xs_i2c_init(xsMachine *the)
{
    xsI2C i2c = NULL;
    FskErr err;
    int address;
    int sdaPin = 0, clkPin = 0, sdaDev = 0, clkDev = 0;

    if (xsGetHostData(xsThis))
        xsThrowDiagnosticIfFskErr(kFskErrBadState, "I2C pin already initialized (SDA pin %d).", i2c->sdaPin);

    address = xsToInteger(xsGet(xsThis, xsID("address")));
    xsResult = xsGet(xsThis, xsID("bus"));
    if (xsUndefinedType == xsTypeOf(xsResult)) {
        sdaPin = xsToInteger(xsGet(xsThis, xsID("sda")));
        clkPin = xsToInteger(xsGet(xsThis, xsID("clock")));
        sdaDev = FskHardwarePinsMux(sdaPin, kFskHardwarePinI2CData);
        clkDev = FskHardwarePinsMux(clkPin, kFskHardwarePinI2CClock);

#if K4GEN2
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
#endif

    }
    else
        sdaDev = xsToInteger(xsResult);

	if (sdaDev < 0 || clkDev < 0)
        xsThrowDiagnosticIfFskErr(kFskErrInvalidParameter, "I2C invalid pins specified (SDA pin %d, CLK pin %d).", sdaPin, clkPin);

    DBG_I2C("Opening I2C at %d\n", sdaDev);
    err = FskI2COpen(sdaDev);
    xsThrowDiagnosticIfFskErr(err, "I2C open failed %s (SDA pin %d, CLK pin %d).", FskInstrumentationGetErrorString(err), sdaPin, clkPin);

    err = FskMemPtrNewClear(sizeof(xsI2CRecord), &i2c);
    if (err) {
        FskI2CClose(sdaDev);
        xsError(err);
    }
    
    xsSetHostData(xsThis, i2c);

    i2c->sdaPin = sdaPin;
    i2c->clkPin = clkPin;
    i2c->sdaDev = sdaDev;
    i2c->clkDev = clkDev;
    i2c->address = (uint8_t)address;
    i2c->bus = sdaDev;

#if SUPPORT_XS_DEBUG
    if (0 != sdaPin)
        snprintf(i2c->diagnosticID, sizeof(i2c->diagnosticID), "(Address 0x%x, SDA pin %d, CLK pin %d)", address, sdaPin, clkPin);
    else
        snprintf(i2c->diagnosticID, sizeof(i2c->diagnosticID), "(Address 0x%x, Bus %d)", address, sdaDev);
#endif
}

void xs_i2c_readByte(xsMachine *the)
{
    xsI2C i2c = xsGetHostData(xsThis);
    int32_t byte;

    DBG_I2C("xs_i2c_readByte\n");

    if (!i2c)
        xsError(kFskErrBadState);

    FskI2CSetSlave(i2c->bus, i2c->address);
  
    byte = FskI2CReadByte(i2c->bus, PLAIN);
    if (byte < 0)
        xsThrowDiagnosticIfFskErr(kFskErrOperationFailed, "I2C readByte failed %s.", i2c->diagnosticID);
    xsResult = xsInteger(byte);
}

void xs_i2c_writeByte(xsMachine *the)
{
    xsI2C i2c = xsGetHostData(xsThis);
    FskErr err;
    uint8_t byte = (uint8_t)xsToInteger(xsArg(0));

    DBG_I2C("xs_i2c_writeByte: %#x\n", byte);

    if (!i2c)
        xsError(kFskErrBadState);

    FskI2CSetSlave(i2c->bus, i2c->address);

    err = FskI2CWriteByte(i2c->bus, PLAIN, byte);
    xsThrowDiagnosticIfFskErr(err, "I2C writeByte failed with error %s %s.", FskInstrumentationGetErrorString(err), i2c->diagnosticID);
}

void xs_i2c_writeQuickSMB(xsMachine *the)
{
    xsI2C i2c = xsGetHostData(xsThis);
    FskErr err;
    uint8_t byte = (uint8_t)xsToInteger(xsArg(0));

	DBG_I2C("xs_i2c_writeQuickSMB Writing quick byte SMB %u\n", byte);

    if (!i2c)
        xsError(kFskErrBadState);

    FskI2CSetSlave(i2c->bus, i2c->address);

    err = FskI2CWriteQuickSMB(i2c->bus, byte);
    xsThrowDiagnosticIfFskErr(err, "I2C writeQuickSMB failed with error %s %s.", FskInstrumentationGetErrorString(err), i2c->diagnosticID);
}

void xs_i2c_readByteDataSMB(xsMachine *the)
{
    xsI2C i2c = xsGetHostData(xsThis);
	uint8_t reg = (uint8_t)xsToInteger(xsArg(0));
	int32_t val;

	DBG_I2C("xs_i2c_readByteDataSMB Reading byte from %u\n", reg);

    if (!i2c)
        xsError(kFskErrBadState);

    FskI2CSetSlave(i2c->bus, i2c->address);

	val = FskI2CReadByteDataSMB(i2c->bus, reg);
	if (val < 0)
        xsThrowDiagnosticIfFskErr(kFskErrOperationFailed, "I2C readByteDataSMB register %d failed %s.", (int)reg, i2c->diagnosticID);

    xsResult = xsInteger(val);
}

void xs_i2c_writeByteDataSMB(xsMachine *the)
{
    xsI2C i2c = xsGetHostData(xsThis);
    FskErr err;
    uint8_t reg = (uint8_t) xsToInteger(xsArg(0));
    uint8_t byte = (uint8_t) xsToInteger(xsArg(1));

	DBG_I2C("xs_i2c_writeByteDataSMB write byte %u from %u\n", byte, reg);

    if (!i2c)
        xsError(kFskErrBadState);

    FskI2CSetSlave(i2c->bus, i2c->address);

    err = FskI2CWriteByteDataSMB(i2c->bus, reg, byte);
    xsThrowDiagnosticIfFskErr(err, "I2C writeByteDataSMB register %d failed with error %s %s.", (int)reg, FskInstrumentationGetErrorString(err), i2c->diagnosticID);
}

void xs_i2c_writeWordDataSMB(xsMachine *the)
{
    xsI2C i2c = xsGetHostData(xsThis);
    FskErr err;
    uint8_t reg = (uint8_t)xsToInteger(xsArg(0));
    uint16_t word = (uint16_t)xsToInteger(xsArg(1));

	DBG_I2C("xs_i2c_writeWordDataSMB write word %u from %u\n", word, reg);

    if (!i2c)
        xsError(kFskErrBadState);

    FskI2CSetSlave(i2c->bus, i2c->address);

    err = FskI2CWriteWordDataSMB(i2c->bus, reg, word);
    xsThrowDiagnosticIfFskErr(err, "I2C writeWordDataSMB register %d failed with error %s %s.", (int)reg, FskInstrumentationGetErrorString(err), i2c->diagnosticID);
}

void xs_i2c_processCallSMB(xsMachine *the)
{
    xsI2C i2c = xsGetHostData(xsThis);
	int32_t reg = xsToInteger(xsArg(0));
    int32_t value = xsToInteger(xsArg(1));
    int32_t retVal;

    DBG_I2C("xs_i2c_processCallSMB Process Call SMB to register %d, value %d\n", reg, value);

    if (!i2c)
        xsError(kFskErrBadState);

    FskI2CSetSlave(i2c->bus, i2c->address);

    retVal = FskI2CProcessCallSMB(i2c->bus, reg, value);
    if (-1 == retVal)
        xsThrowDiagnosticIfFskErr(kFskErrOperationFailed, "I2C processCallSMB register %d failed %s.", (int)reg, i2c->diagnosticID);

    xsResult = xsInteger(retVal);
}

void xs_i2c_readWordDataSMB(xsMachine *the)
{
    xsI2C i2c = xsGetHostData(xsThis);
    uint8_t reg = (uint8_t)xsToInteger(xsArg(0));
    int32_t val;
  
    DBG_I2C("xs_i2c_readWordDataSMB Process Call SMB to register %d\n", reg);

    if (!i2c)
        xsError(kFskErrBadState);
  
    FskI2CSetSlave(i2c->bus, i2c->address);

    val = FskI2CReadWordDataSMB(i2c->bus, reg);
    if (val < 0) {
        if (! (i2c->address == 0x36) ) xsTraceDiagnostic("I2C readWordDataSMB register %d failed %s.", (int)reg, i2c->diagnosticID);
        xsError(kFskErrOperationFailed);
    }

    xsResult = xsInteger(val);
}

void xs_i2c_readBlockDataSMB(xsMachine* the)
{
    xsI2C i2c = xsGetHostData(xsThis);
	char* formatString;
	int format = 0, i, dataSize = 0;
    uint8_t reg = (uint8_t)xsToInteger(xsArg(0));
    uint8_t data[34]; //needs to be 34 because we're using I2C_SMBUS_I2C_BLOCK_BROKEN in i2cdev.c
    SInt32 length = (SInt32)xsToInteger(xsArg(1));

	if (!i2c)
		xsError(kFskErrBadState);

    if ((length < 0) || (length > 32))
        xsThrowDiagnosticIfFskErr(kFskErrInvalidParameter, "I2C readBlockDataSMB bad length %d %s.", length, i2c->diagnosticID);

	FskI2CSetSlave(i2c->bus, i2c->address);

    formatString = xsToString(xsArg(2));
    if (!FskStrCompare(formatString, "Chunk")) format = 0;
    else if (!FskStrCompare(formatString, "Array")) format = 1;

    dataSize = FskI2CReadBlockDataSMB(i2c->bus, reg, length, data);
    if (dataSize <= 0)
        xsThrowDiagnosticIfFskErr(kFskErrBadData, "I2C readBlockDataSMB register %d failed %s.", (int)reg, i2c->diagnosticID);

    if (0 == format) {
        xsResult = xsNew1(xsGlobal, xsID("Chunk"), xsInteger(dataSize));
        FskMemMove(xsGetHostData(xsResult), data, dataSize);
    }
    else if (1 == format) {
        xsResult = xsNew1(xsGlobal, xsID("Array"), xsInteger(dataSize));
        for (i = 0; i < dataSize; i++)
            xsSet(xsResult, i, xsInteger(data[i]));
    }
}

void xs_i2c_readBlock(xsMachine *the)
{
	FskErr err;
	xsI2C i2c = xsGetHostData(xsThis);
	int argc = xsToInteger(xsArgc), i;
	int format = 0;
    SInt32 dataSize = xsToInteger(xsArg(0));
    unsigned char *data = NULL;
    
    err = FskMemPtrNew(dataSize, &data);
    xsThrowDiagnosticIfFskErr(err, "I2C readBlock failed %d", err);

	FskI2CSetSlave(i2c->bus, i2c->address);
	err = FskI2CDevReadBlock(i2c->bus, PLAIN, &dataSize, data);
    if (err) {
        xsTraceDiagnostic("I2C readBlock failed with error %s %s.", FskInstrumentationGetErrorString(err), i2c->diagnosticID);
        goto bail;
    }

    if (argc > 1) {
        int t = xsTypeOf(xsArg(1));
        if ((xsNumberType == t) || (t == xsIntegerType))
            format = xsToInteger(xsArg(1));
        else {
            char *formatString = xsToString(xsArg(1));
            if (0 == FskStrCompare(formatString, "Chunk"))
                format = 0;
            else if (0 == FskStrCompare(formatString, "Array"))
                format = 1;
        }
    }
    
    if (0 == format) {
        xsResult = xsNew1(xsGlobal, xsID("Chunk"), xsInteger(dataSize));
        FskMemMove(xsGetHostData(xsResult), data, dataSize);
    }
    else if (1 == format) {
        xsResult = xsNew1(xsGlobal, xsID("Array"), xsInteger(dataSize));
        for (i = 0; i < dataSize; i++)
            xsSet(xsResult, i, xsInteger(data[i]));
    }
    
bail:
    FskMemPtrDispose(data);
    if (err)
		xsError(err);
}

void xs_i2c_writeBlock(xsMachine* the)
{
	FskErr err;
	xsI2C i2c = xsGetHostData(xsThis);
	int argc = xsToInteger(xsArgc), i;

	unsigned char buffer[32], *bufPtr = buffer;
	if (!i2c)
		xsError(kFskErrBadState);

	for (i = 0; i < argc; i++)
		bufPtr = writeOne(the, i2c, &xsArg(i), bufPtr, buffer + sizeof(buffer));

	FskI2CSetSlave(i2c->bus, i2c->address);

	err = FskI2CWriteBlock(i2c->bus, PLAIN, bufPtr - buffer, buffer);
    xsThrowDiagnosticIfFskErr(err, "I2C FskI2CWriteBlock failed with error %s %s.", FskInstrumentationGetErrorString(err), i2c->diagnosticID);
}

void xs_i2c_writeBlockDataSMB(xsMachine* the)
{
    FskErr err;
    xsI2C i2c = xsGetHostData(xsThis);
    int argc = xsToInteger(xsArgc), i;
    uint8_t reg = (uint8_t)xsToInteger(xsArg(0));
    unsigned char buffer[32], *bufPtr = buffer;

	if (!i2c)
		xsError(kFskErrBadState);

	FskI2CSetSlave(i2c->bus, i2c->address);

    for (i = 1; i < argc; i++)
        bufPtr = writeOne(the, i2c, &xsArg(i), bufPtr, buffer + sizeof(buffer));

    err = FskI2CWriteBlockDataSMB(i2c->bus, reg, bufPtr - buffer, buffer);
    xsThrowDiagnosticIfFskErr(err, "I2C writeBlockDataSMB register %d failed with error %s %s.", (int)reg, FskInstrumentationGetErrorString(err), i2c->diagnosticID);
}

void xs_i2c_close(xsMachine *the)
{
    xs_i2c(xsGetHostData(xsThis));
    xsSetHostData(xsThis, NULL);
}

unsigned char *writeOne(xsMachine *the, xsI2C i2c, xsSlot *slot, unsigned char *bufPtr, unsigned char *bufEnd)
{
    xsType argType = xsTypeOf(*slot);

    switch (argType) {
        case xsIntegerType:
        case xsNumberType: {
            SInt32 value = xsToInteger(*slot);
            if ((value < 0) || (value > 255))
                xsThrowDiagnosticIfFskErr(kFskErrInvalidParameter, "I2C write invalid character value %s.", i2c->diagnosticID);
            if ((bufEnd - bufPtr) < 1)
                xsThrowDiagnosticIfFskErr(kFskErrInvalidParameter, "I2C write 32 byte write max %s.", i2c->diagnosticID);
            *bufPtr++ = (char)value;
            }
            break;
        
        case xsStringType: {
            char *text = xsToString(*slot);
            SInt32 dataSize = FskStrLen(text);
            if ((bufEnd - bufPtr) < dataSize)
                xsThrowDiagnosticIfFskErr(kFskErrInvalidParameter, "I2C write 32 byte write max %s.", i2c->diagnosticID);
            FskMemMove(bufPtr, text, dataSize);
            bufPtr += dataSize;
            }
            break;

        case xsReferenceType:
            if (xsIsInstanceOf(*slot, xsChunkPrototype)) {
                char *data = xsGetHostData(*slot);
                SInt32 dataSize = xsToInteger(xsGet(*slot, xsID("length")));
                if ((bufEnd - bufPtr) < dataSize)
                    xsThrowDiagnosticIfFskErr(kFskErrInvalidParameter, "I2C write 32 byte write max %s.", i2c->diagnosticID);
                FskMemMove(bufPtr, data, dataSize);
                bufPtr += dataSize;
            }
            else if (xsIsInstanceOf(*slot, xsArrayPrototype)) {
                SInt32 length = xsToInteger(xsGet(*slot, xsID("length"))), j;
                if ((bufEnd - bufPtr) < length)
                    xsThrowDiagnosticIfFskErr(kFskErrInvalidParameter, "I2C write 32 byte write max %s.", i2c->diagnosticID);
                for (j = 0; j < length; j++) {
                    xsSlot item = xsGet(*slot, j);
                    bufPtr = writeOne(the, i2c, &item, bufPtr, bufEnd);
                }
            }
            else
                xsThrowDiagnosticIfFskErr(kFskErrInvalidParameter, "I2C unsupported argument type passed to write %s.", i2c->diagnosticID);
            break;
        
        default:
            xsThrowDiagnosticIfFskErr(kFskErrInvalidParameter, "I2C unsupported argument type passed to write %s.", i2c->diagnosticID);
            break;
    }
    
    return bufPtr;
}

#endif /* USEI2C */
