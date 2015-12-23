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

#define __FSKTHREAD_PRIV__
#include "kprPins.h"

#ifdef USESERIAL

#include "FskECMAScript.h"
#include "FskMemory.h"
#include "FskTextConvert.h"
#include "FskString.h"
#include "FskManifest.xs.h"

#include "KplThread.h"
#include "KplThreadLinuxPriv.h"

static void writeOne(xsMachine *the, FskSerialIO sio, xsSlot *slot);

static void serialPinDataReady(FskPinSerial pin, void *refCon);
static void serialDataReady(void *arg0, void *arg1, void *arg2, void *arg3);

void xs_serial(void *data)
{
    FskSerialIO sio = data;
    if (sio) {
        FskPinSerialDispose(sio->pin);
        FskMemPtrDispose(sio);
    }
}

void xs_serial_init(xsMachine* the)
{
    FskErr err;
    FskSerialIO sio;
    SInt32 rxPin = 0, txPin = 0;
    SInt32 baud = xsToInteger(xsGet(xsThis, xsID("baud")));
    const char *path = NULL;

    sio = xsGetHostData(xsThis);
    if (sio)
        xsThrowDiagnosticIfFskErr(kFskErrBadState, "Serial already initialized (rx pin %d, tx pin %d).", (int)sio->rxNum, (int)sio->txNum);

    if (xsTest(xsGet(xsThis, xsID("path"))))
        path = xsToString(xsGet(xsThis, xsID("path")));
    else {
        if (xsHas(xsThis, xsID("rx")))
            rxPin = xsToInteger(xsGet(xsThis, xsID("rx")));
        if (xsHas(xsThis, xsID("tx")))
            txPin = xsToInteger(xsGet(xsThis, xsID("tx")));
    }

    xsThrowIfFskErr(FskMemPtrNewClear(sizeof(FskSerialIORecord) + (path ? FskStrLen(path) : 0), (FskMemPtr *)&sio));
    sio->rxNum = rxPin;
    sio->txNum = txPin;
    if (path)
        FskStrCopy(sio->path, path);

	sio->pinsThread = FskThreadGetCurrent();

	err = FskPinSerialNew(&sio->pin, rxPin, txPin, path, baud);
    if (err) {
        FskMemPtrDispose(sio);
        xsThrowDiagnosticIfFskErr(err, "Serial initialization failed with error %s (rx pin %d, tx pin %d).", FskInstrumentationGetErrorString(err), (int)sio->rxNum, (int)sio->txNum);
    }
    
    xsSetHostData(xsThis, sio);
}

void xs_serial_read(xsMachine* the)
{
    FskSerialIO sio = xsGetHostData(xsThis);
	char* formatString;
	FskErr err;
	unsigned char *data = NULL;
	int dataSize = 0;
	int format = 0, i, maxCount = 0;
    SInt32 endTime = 0, argc;

	if (!sio)
		xsError(kFskErrBadState);

    if (!sio->path && (0 == sio->rxNum))
        xsThrowDiagnosticIfFskErr(kFskErrBadState, "Cannot read from transmit only serial connection (rx pin %d, tx pin %d).", (int)sio->rxNum, (int)sio->txNum);

    formatString = xsToString(xsArg(0));
    if (!FskStrCompare(formatString, "ArrayBuffer")) format = 4;
    else if (!FskStrCompare(formatString, "Chunk")) format = 0;
    else if (!FskStrCompare(formatString, "Array")) format = 1;
    else if (!FskStrCompare(formatString, "String")) format = 2;
    else if (!FskStrCompare(formatString, "charCode")) format = 3;

	{
	xsTry {
		switch (format) {
			case 0: // Chunk
			case 1: // Array
			case 2: // String
			case 4: // ArrayBuffer
				argc = xsToInteger(xsArgc);
				if (argc >= 2) {
					maxCount = xsToInteger(xsArg(1));

					if (argc) {
						SInt32 duration = xsToInteger(xsArg(2));
						FskTimeRecord now;
						FskTimeGetNow(&now);
						endTime = FskTimeInMS(&now) + duration;
					}
				}

				while (true) {
					FskTimeRecord now;
					unsigned char buffer[1024];
					SInt32 ds;
					SInt32 bytesToRead = ((0 == maxCount) || (maxCount > sizeof(buffer))) ? sizeof(buffer) : maxCount;

					err = FskPinSerialRead(sio->pin, bytesToRead, &ds, buffer);
					if (err) {
						if (kFskErrNoData == err)
							ds = 0;
						else
							xsThrowDiagnosticIfFskErr(err, "Serial read failed with error %s (rx pin %d, tx pin %d).", FskInstrumentationGetErrorString(err), (int)sio->rxNum, (int)sio->txNum);
					}

					err = FskMemPtrRealloc(dataSize + ds, &data);
					if (err) {
						FskMemPtrDispose(data);
						xsError(err);
					}
					FskMemMove(data + dataSize, buffer, ds);
					dataSize += ds;

					if (!endTime || (dataSize >= maxCount))
						break;

					FskTimeGetNow(&now);
					if (FskTimeInMS(&now) >= endTime)
						break;
					
					FskDelay(2);
				}

				if (4 == format) {
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
				else if (2 == format) {
					if (!FskTextUTF8IsValid((const unsigned char *)data, dataSize)) {
						xsThrowDiagnosticIfFskErr(kFskErrBadData, "SerialIO Error: string read but not UTF-8 clean data (rx pin %d, tx pin %d).", (int)sio->rxNum, (int)sio->txNum);
					}
					xsResult = xsStringBuffer((xsStringValue)data, dataSize);
				}
				break;

			case 3: // charCode
				{
				UInt8 c;
				err = FskPinSerialRead(sio->pin, 1, &dataSize, &c);
				if (kFskErrNone == err)
					xsResult = xsInteger(c);
				 else {
					if (kFskErrNoData == err)
						xsResult = xsUndefined;
					else
						xsThrowDiagnosticIfFskErr(err, "Serial read failed with error %s (rx pin %d, tx pin %d).", FskInstrumentationGetErrorString(err), (int)sio->rxNum, (int)sio->txNum);
				}
				}
				break;
		}

		FskMemPtrDispose(data);
	}
	xsCatch {
		FskMemPtrDispose(data);

		xsThrow(xsException);
	}
	}
}

void writeOne(xsMachine *the, FskSerialIO sio, xsSlot *slot)
{
    xsType argType = xsTypeOf(*slot);
    FskErr err;

    switch (argType) {
        case xsIntegerType:
        case xsNumberType: {
            SInt32 value = xsToInteger(*slot);
			UInt8 cv = (UInt8)value;
            if ((value < 0) || (value > 255))
                    xsThrowDiagnosticIfFskErr(kFskErrInvalidParameter, "SerialIO Error: Invalid character value (rx pin %d, tx pin %d).", (int)sio->rxNum, (int)sio->txNum);
            err = FskPinSerialWrite(sio->pin, 1, &cv);
            xsThrowDiagnosticIfFskErr(err, "Serial write failed with error %s (rx pin %d, tx pin %d).", FskInstrumentationGetErrorString(err), (int)sio->rxNum, (int)sio->txNum);
            }
            break;
        
        case xsStringType: {
            char *text = xsToString(*slot);
            err = FskPinSerialWrite(sio->pin, FskStrLen(text), (UInt8 *)text);
            xsThrowDiagnosticIfFskErr(err, "Serial write failed with error %s (rx pin %d, tx pin %d).", FskInstrumentationGetErrorString(err), (int)sio->rxNum, (int)sio->txNum);
            }
            break;

        case xsReferenceType:
            if (xsIsInstanceOf(*slot, xsArrayBufferPrototype)) {
                char *data = xsToArrayBuffer(*slot);
                SInt32 dataSize = xsGetArrayBufferLength(*slot);
                err = FskPinSerialWrite(sio->pin, dataSize, (UInt8 *)data);
                xsThrowDiagnosticIfFskErr(err, "Serial write failed with error %s (rx pin %d, tx pin %d).", FskInstrumentationGetErrorString(err), (int)sio->rxNum, (int)sio->txNum);
			}
			else if (xsIsInstanceOf(*slot, xsChunkPrototype)) {
                char *data = xsGetHostData(*slot);
                SInt32 dataSize = xsToInteger(xsGet(*slot, xsID("length")));
                err = FskPinSerialWrite(sio->pin, dataSize, (UInt8 *)data);
                xsThrowDiagnosticIfFskErr(err, "Serial write failed with error %s (rx pin %d, tx pin %d).", FskInstrumentationGetErrorString(err), (int)sio->rxNum, (int)sio->txNum);
            }
            else if (xsIsInstanceOf(*slot, xsArrayPrototype)) {
                SInt32 length = xsToInteger(xsGet(*slot, xsID("length"))), j;
                for (j = 0; j < length; j++) {
                    xsSlot item = xsGet(*slot, j);
                    writeOne(the, sio, &item);
                }
            }
            else
                xsThrowDiagnosticIfFskErr(kFskErrUnimplemented, "Serial IO Error: unsupported argument type passed to write (rx pin %d, tx pin %d).", (int)sio->rxNum, (int)sio->txNum);
            break;
        
        default:
            xsThrowDiagnosticIfFskErr(kFskErrUnimplemented, "Serial IO Error: unsupported argument type passed to write (rx pin %d, tx pin %d).", (int)sio->rxNum, (int)sio->txNum);
            break;
    }
}

void xs_serial_write(xsMachine* the)
{
	FskSerialIO sio = xsGetHostData(xsThis);
    int argc = xsToInteger(xsArgc), i;

	if (!sio)
		xsError(kFskErrBadState);

    if (!sio->path && (0 == sio->txNum))
        xsThrowDiagnosticIfFskErr(kFskErrBadState, "Cannot write to receive only serial connection (rx pin %d, tx pin %d).", (int)sio->rxNum, (int)sio->txNum);

    for (i = 0; i < argc; i++)
        writeOne(the, sio, &xsArg(i));
}

void xs_serial_close(xsMachine *the)
{
    xs_serial(xsGetHostData(xsThis));
    xsSetHostData(xsThis, NULL);
}

void xs_serial_repeat(xsMachine *the)
{
	FskSerialIO sio = xsGetHostData(xsThis);

	if (!sio)
		return;

    if (!sio->path && (0 == sio->rxNum))
        xsThrowDiagnosticIfFskErr(kFskErrBadState, "Cannot poll from transmit only serial connection (rx pin %d, tx pin %d).", (int)sio->rxNum, (int)sio->txNum);

    sio->poller = (xsTest(xsArg(0))) ? xsGetHostData(xsArg(0)) : NULL;

	FskPinSerialDataReady(sio->pin, sio->poller ? serialPinDataReady : NULL, sio);
}

void serialPinDataReady(FskPinSerial pin, void *refCon)
{
	FskSerialIO sio = refCon;
	if (!sio->notifyPending) {
		sio->notifyPending = true;
		FskThreadPostCallback(sio->pinsThread, serialDataReady, sio, NULL, NULL, NULL);
	}
}

void serialDataReady(void *arg0, void *arg1, void *arg2, void *arg3)
{
	FskSerialIO sio = arg0;
	sio->notifyPending = false;
	KprPinsPollerRun(sio->poller);
}

#endif /* USESERIAL */
