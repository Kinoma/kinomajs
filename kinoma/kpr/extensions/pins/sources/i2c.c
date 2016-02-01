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

#ifdef USEI2C

#define __FSKECMASCRIPT_PRIV__

#if SUPPORT_INSTRUMENTATION
   FskInstrumentedSimpleType(I2C, "I2C");
   // gI2CTypeInstrumentation
   //#define         DBG_I2C(...)   do { FskInstrumentedTypePrintfDebug (&gI2CTypeInstrumentation, __VA_ARGS__); } while(0);
	//@BSF - fix me
    #define         DBG_I2C(format, ...)
#else
    #define         DBG_I2C(format, ...)
#endif  // SUPPORT_INSTRUMENTATION

typedef struct {
    FskPinI2C pin;
    int sdaPin;
    int clkPin;
    int sdaDev;
    int clkDev;
    int bus;
    UInt8 address;
#if SUPPORT_XS_DEBUG
    char diagnosticID[512];
#endif
} xsI2CRecord, *xsI2C;

static unsigned char *writeOne(xsMachine *the, xsI2C i2c, xsSlot *slot, unsigned char *bufPtr, unsigned char *bufEnd);

void xs_i2c(void *data)
{
    if (data) {
        xsI2C i2c = data;
        FskPinI2CDispose(i2c->pin);
        FskMemPtrDispose(i2c);
    }
}

void xs_i2c_init(xsMachine *the)
{
    xsI2C i2c = NULL;
    FskErr err;
    int address;
    int sdaPin = 0, clkPin = 0, sdaDev = 0, clkDev = 0;
	FskPinI2C pin = NULL;

    if (xsGetHostData(xsThis))
        xsThrowDiagnosticIfFskErr(kFskErrBadState, "I2C pin already initialized (SDA pin %d).", i2c->sdaPin);

    address = xsToInteger(xsGet(xsThis, xsID("address")));
    xsResult = xsGet(xsThis, xsID("bus"));
    if (xsUndefinedType == xsTypeOf(xsResult)) {
        sdaPin = xsToInteger(xsGet(xsThis, xsID("sda")));
        clkPin = xsToInteger(xsGet(xsThis, xsID("clock")));

		err = FskPinI2CNew(&pin, sdaPin, clkPin, kFskPinI2CNoBus);
    }
    else {
        sdaDev = xsToInteger(xsResult);
		err = FskPinI2CNew(&pin, 0, 0, sdaDev);
	}

    xsThrowDiagnosticIfFskErr(err, "I2C open failed %s (SDA pin %d, CLK pin %d).", FskInstrumentationGetErrorString(err), sdaPin, clkPin);

    err = FskMemPtrNewClear(sizeof(xsI2CRecord), &i2c);
    if (err) {
        FskPinI2CDispose(pin);
        xsError(err);
    }
    
    xsSetHostData(xsThis, i2c);

	i2c->pin = pin;
    i2c->sdaPin = sdaPin;
    i2c->clkPin = clkPin;
    i2c->sdaDev = sdaDev;
    i2c->clkDev = clkDev;
    i2c->address = (UInt8)address;
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
	FskErr err;
    UInt8 byte;

    DBG_I2C("xs_i2c_readByte\n");

	xsThrowIfNULL(i2c);

	FskPinI2CSetAddress(i2c->pin, i2c->address);

    err = FskPinI2CReadByte(i2c->pin, &byte);
	xsThrowDiagnosticIfFskErr(err, "I2C readByte failed %s.", i2c->diagnosticID);
    xsResult = xsInteger(byte);
}

void xs_i2c_writeByte(xsMachine *the)
{
    xsI2C i2c = xsGetHostData(xsThis);
    FskErr err;
    UInt8 byte = (UInt8)xsToInteger(xsArg(0));

    DBG_I2C("xs_i2c_writeByte: %#x\n", byte);

	xsThrowIfNULL(i2c);

    FskPinI2CSetAddress(i2c->pin, i2c->address);

    err = FskPinI2CWriteByte(i2c->pin, byte);
    xsThrowDiagnosticIfFskErr(err, "I2C writeByte failed with error %s %s.", FskInstrumentationGetErrorString(err), i2c->diagnosticID);
}

void xs_i2c_writeQuickSMB(xsMachine *the)
{
    xsI2C i2c = xsGetHostData(xsThis);
    FskErr err;
    uint8_t byte = (uint8_t)xsToInteger(xsArg(0));

	DBG_I2C("xs_i2c_writeQuickSMB Writing quick byte SMB %u\n", byte);

	xsThrowIfNULL(i2c);

    FskPinI2CSetAddress(i2c->pin, i2c->address);

//@@    err = FskI2CWriteQuickSMB(i2c->bus, byte);
err = kFskErrOperationFailed;		//@@
    xsThrowDiagnosticIfFskErr(err, "I2C writeQuickSMB failed with error %s %s.", FskInstrumentationGetErrorString(err), i2c->diagnosticID);
}

void xs_i2c_readByteDataSMB(xsMachine *the)
{
    xsI2C i2c = xsGetHostData(xsThis);
	FskErr err;
	UInt8 command = (UInt8)xsToInteger(xsArg(0));
	UInt8 byte;

	DBG_I2C("xs_i2c_readByteDataSMB Reading byte from %u\n", command);

	xsThrowIfNULL(i2c);

    FskPinI2CSetAddress(i2c->pin, i2c->address);

	err = FskPinI2CReadDataByte(i2c->pin, command, &byte);
	xsThrowDiagnosticIfFskErr(err, "I2C readByteDataSMB register %d failed %s.", (int)command, i2c->diagnosticID);

    xsResult = xsInteger(byte);
}

void xs_i2c_writeByteDataSMB(xsMachine *the)
{
    xsI2C i2c = xsGetHostData(xsThis);
    FskErr err;
    UInt8 command = (UInt8) xsToInteger(xsArg(0));
    UInt8 byte = (UInt8) xsToInteger(xsArg(1));

	DBG_I2C("xs_i2c_writeByteDataSMB write byte %u from %u\n", byte, command);

	xsThrowIfNULL(i2c);

    FskPinI2CSetAddress(i2c->pin, i2c->address);

    err = FskPinI2CWriteDataByte(i2c->pin, command, byte);
    xsThrowDiagnosticIfFskErr(err, "I2C writeByteDataSMB register %d failed with error %s %s.", (int)command, FskInstrumentationGetErrorString(err), i2c->diagnosticID);
}

void xs_i2c_writeWordDataSMB(xsMachine *the)
{
    xsI2C i2c = xsGetHostData(xsThis);
    FskErr err;
    UInt8 command = (UInt8)xsToInteger(xsArg(0));
    UInt16 word = (uint16_t)xsToInteger(xsArg(1));

	DBG_I2C("xs_i2c_writeWordDataSMB write word %u from %u\n", word, command);

	xsThrowIfNULL(i2c);

    FskPinI2CSetAddress(i2c->pin, i2c->address);

    err = FskPinI2CWriteDataWord(i2c->pin, command, word);
    xsThrowDiagnosticIfFskErr(err, "I2C writeWordDataSMB register %d failed with error %s %s.", (int)command, FskInstrumentationGetErrorString(err), i2c->diagnosticID);
}

void xs_i2c_processCallSMB(xsMachine *the)
{
    xsI2C i2c = xsGetHostData(xsThis);
	FskErr err;
	UInt8 command = (UInt8)xsToInteger(xsArg(0));
    UInt16 value = (UInt16)xsToInteger(xsArg(1));
    UInt16 result;

    DBG_I2C("xs_i2c_processCallSMB Process Call SMB to register %d, value %d\n", command, value);

	xsThrowIfNULL(i2c);

    FskPinI2CSetAddress(i2c->pin, i2c->address);

	err = FskPinI2CProcessCall(i2c->pin, command, value, &result);
	xsThrowDiagnosticIfFskErr(err, "I2C processCallSMB register %d failed %s.", (int)command, i2c->diagnosticID);

    xsResult = xsInteger(result);
}

void xs_i2c_readWordDataSMB(xsMachine *the)
{
    xsI2C i2c = xsGetHostData(xsThis);
    UInt8 command = (UInt8)xsToInteger(xsArg(0));
	FskErr err;
    UInt16 val;
  
    DBG_I2C("xs_i2c_readWordDataSMB Call SMB to register %d\n", command);

	xsThrowIfNULL(i2c);

    FskPinI2CSetAddress(i2c->pin, i2c->address);

    err = FskPinI2CReadDataWord(i2c->pin, command, &val);
//@@        if (! (i2c->address == 0x36) ) xsTraceDiagnostic("I2C readWordDataSMB register %d failed %s.", (int)reg, i2c->diagnosticID);
	xsThrowDiagnosticIfFskErr(err, "I2C readWordDataSMB register %d failed %s.", (int)command, i2c->diagnosticID);

    xsResult = xsInteger(val);
}

void xs_i2c_readBlockDataSMB(xsMachine* the)
{
    xsI2C i2c = xsGetHostData(xsThis);
	FskErr err;
	char* formatString;
	int format = 2, i;
	SInt32 dataSize = 0;
    UInt8 command = (UInt8)xsToInteger(xsArg(0));
    UInt8 data[34]; //needs to be 34 because we're using I2C_SMBUS_I2C_BLOCK_BROKEN in i2cdev.c		//@@ WTF - not at this layer, at least
    SInt32 length = (SInt32)xsToInteger(xsArg(1));

    DBG_I2C("xs_i2c_readBlockDataSMB\n");

	xsThrowIfNULL(i2c);

    if ((length < 0) || (length > 32))
        xsThrowDiagnosticIfFskErr(kFskErrInvalidParameter, "I2C readBlockDataSMB bad length %d %s.", length, i2c->diagnosticID);

	FskPinI2CSetAddress(i2c->pin, i2c->address);

    formatString = xsToString(xsArg(2));
    if (!FskStrCompare(formatString, "Buffer")) format = 2;
    else if (!FskStrCompare(formatString, "Chunk")) format = 0;
    else if (!FskStrCompare(formatString, "Array")) format = 1;

	err = FskPinI2CReadDataBytes(i2c->pin, command, length, &dataSize, data);
	xsThrowDiagnosticIfFskErr(err, "I2C readBlockDataSMB register %d failed %s.", (int)command, i2c->diagnosticID);

    if (2 == format) {
		xsResult = xsArrayBuffer(data, dataSize);
	}
    else if (0 == format) {
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
	int format = 2;
    SInt32 dataSize = xsToInteger(xsArg(0)), readCount;
	UInt8 data[32];

	xsThrowIfNULL(i2c);

    DBG_I2C("xs_i2c_readBlock\n");

	if ((dataSize > 32) || (dataSize <= 0))
		xsThrowDiagnosticIfFskErr(kFskErrInvalidParameter, "I2C readBlock invalid size %d. %s", (int)dataSize, i2c->diagnosticID);

	FskPinI2CSetAddress(i2c->pin, i2c->address);
	err = FskPinI2CReadBytes(i2c->pin, dataSize, &readCount, data);
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
            if (0 == FskStrCompare(formatString, "Buffer"))
                format = 2;
            else if (0 == FskStrCompare(formatString, "Chunk"))
                format = 0;
            else if (0 == FskStrCompare(formatString, "Array"))
                format = 1;
        }
    }
    
    if (2 == format) {
        xsResult = xsArrayBuffer(data, readCount);
    }
    else if (0 == format) {
        xsResult = xsNew1(xsGlobal, xsID("Chunk"), xsInteger(readCount));
        FskMemMove(xsGetHostData(xsResult), data, readCount);
    }
    else if (1 == format) {
        xsResult = xsNew1(xsGlobal, xsID("Array"), xsInteger(readCount));
        for (i = 0; i < readCount; i++)
            xsSet(xsResult, i, xsInteger(data[i]));
    }
    
bail:
    if (err)
		xsError(err);
}

void xs_i2c_writeBlock(xsMachine* the)
{
	FskErr err;
	xsI2C i2c = xsGetHostData(xsThis);
	int argc = xsToInteger(xsArgc), i;
	UInt8 buffer[32], *bufPtr = buffer;

	xsThrowIfNULL(i2c);

    DBG_I2C("xs_i2c_writeBlock\n");

	for (i = 0; i < argc; i++)
		bufPtr = writeOne(the, i2c, &xsArg(i), bufPtr, buffer + sizeof(buffer));

	FskPinI2CSetAddress(i2c->pin, i2c->address);

	err = FskPinI2CWriteBytes(i2c->pin, bufPtr - buffer, buffer);
    xsThrowDiagnosticIfFskErr(err, "I2C FskI2CWriteBlock failed with error %s %s.", FskInstrumentationGetErrorString(err), i2c->diagnosticID);
}

void xs_i2c_writeBlockDataSMB(xsMachine* the)
{
    FskErr err;
    xsI2C i2c = xsGetHostData(xsThis);
    int argc = xsToInteger(xsArgc), i;
    UInt8 command = (UInt8)xsToInteger(xsArg(0));
    unsigned char buffer[32], *bufPtr = buffer;

	xsThrowIfNULL(i2c);

    DBG_I2C("xs_i2c_writeBlockDataSMB\n");

	FskPinI2CSetAddress(i2c->pin, i2c->address);

    for (i = 1; i < argc; i++)
        bufPtr = writeOne(the, i2c, &xsArg(i), bufPtr, buffer + sizeof(buffer));

    err = FskPinI2CWriteDataBytes(i2c->pin, command, (SInt32)(bufPtr - buffer), buffer);
    xsThrowDiagnosticIfFskErr(err, "I2C writeBlockDataSMB register %d failed with error %s %s.", (int)command, FskInstrumentationGetErrorString(err), i2c->diagnosticID);
}

void xs_i2c_close(xsMachine *the)
{
    DBG_I2C("xs_i2c_close\n");

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
            if (xsIsInstanceOf(*slot, xsArrayBufferPrototype)) {
                char *data = xsToArrayBuffer(*slot);
                SInt32 dataSize = xsGetArrayBufferLength(*slot);
                if ((bufEnd - bufPtr) < dataSize)
                    xsThrowDiagnosticIfFskErr(kFskErrInvalidParameter, "I2C write 32 byte write max %s.", i2c->diagnosticID);
                FskMemMove(bufPtr, data, dataSize);
                bufPtr += dataSize;
            }
            else if (xsIsInstanceOf(*slot, xsChunkPrototype)) {
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
