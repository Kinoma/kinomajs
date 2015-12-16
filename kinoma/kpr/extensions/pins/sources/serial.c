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
#ifdef USESERIAL

#define __FSKTHREAD_PRIV__
#include "FskECMAScript.h"
#include "kprPins.h"
#include "FskMemory.h"
#include "FskTextConvert.h"
#include "FskString.h"
#include "FskManifest.xs.h"

#include "KplThread.h"
#include "KplThreadLinuxPriv.h"

#include <poll.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/eventfd.h>

#define KPR_NO_GRAMMAR 1

static void writeOne(xsMachine *the, FskSerialIO sio, xsSlot *slot);
static void serialThread(void *param);
static void serialDataReady(void *arg0, void *arg1, void *arg2, void *arg3);
static void disposePollerThread(FskSerialIO sio);

void xs_serial(void *data)
{
    FskSerialIO sio = data;
    if (sio) {
		disposePollerThread(sio);
        FskSerialIOPlatformDispose(sio);
		FskMutexDispose(sio->mutex);

        FskMemPtrDispose(sio);
    }
}

void xs_serial_init(xsMachine* the)
{
    FskErr err;
    FskSerialIO sio;
    SInt32 rxPin = 0, txPin = 0;
    SInt32 baud = xsToInteger(xsGet(xsThis, xsID("baud")));
    int ttyNumrx = -1, ttyNumtx = -1;
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

        if ((0 == rxPin) && (0 == txPin))
            xsThrowDiagnosticIfFskErr(kFskErrInvalidParameter, "No pin specified for rx or tx (rx pin %d, tx pin %d).", (int)rxPin, (int)txPin);

        if (rxPin) {
            ttyNumrx = FskHardwarePinsMux(rxPin, kFskHardwarePinUARTRX);
            if (ttyNumrx < 0)
                ttyNumrx = FskHardwarePinsMux(rxPin, kFskHardwarePinUARTTX);
        }
        if (txPin) {
            ttyNumtx = FskHardwarePinsMux(txPin, kFskHardwarePinUARTTX);
            if (ttyNumtx < 0) {
                ttyNumtx = FskHardwarePinsMux(txPin, kFskHardwarePinUARTRX);
            }
        }

        if (((rxPin && txPin) && (ttyNumrx != ttyNumtx)) || (rxPin && (ttyNumrx < 0)) || (txPin && (ttyNumtx < 0)))
            xsThrowDiagnosticIfFskErr(kFskErrInvalidParameter, "Invalid serial pins (rx pin %d, tx pin %d).", (int)rxPin, (int)txPin);
    }

    xsThrowIfFskErr(FskMemPtrNewClear(sizeof(FskSerialIORecord) + (path ? FskStrLen(path) : 0), (FskMemPtr *)&sio));
    sio->rxNum = rxPin;
    sio->txNum = txPin;
    if (path)
        FskStrCopy(sio->path, path);
    else if (-1 != ttyNumrx)
        sio->ttyNum = ttyNumrx;
    else if (-1 != ttyNumtx)
        sio->ttyNum = ttyNumtx;
    else
        xsThrowDiagnosticIfFskErr(kFskErrBadState, "Serial ttyNumtx and ttyNumrx are both uninitialized", 0);

	FskMutexNew(&sio->mutex, "serial pin");
	sio->pinsThread = FskThreadGetCurrent();

    err = FskSerialIOPlatformInit(sio, baud);
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
		if (sio->pollerThread)
			FskMutexAcquire(sio->mutex);

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
					char *d;
					int ds;

					if (sio->pollerThread) {
						if (maxCount)
							xsThrowDiagnosticIfFskErr(kFskErrUnimplemented, "maxCount not supported with Serial repeat", kFskErrUnimplemented);
						d = (char *)sio->data;
						ds = sio->dataCount;
						sio->dataCount = 0;
					}
					else {
						err = FskSerialIOPlatformRead(sio, &d, &ds, maxCount - dataSize);
						if (kFskErrNone == err)
							;
						else if (kFskErrNoData == err) {
							d = NULL;
							ds = 0;
						}
						else
							xsThrowDiagnosticIfFskErr(err, "Serial read failed with error %s (rx pin %d, tx pin %d).", FskInstrumentationGetErrorString(err), (int)sio->rxNum, (int)sio->txNum);
					}

					if (NULL == data) {
						data = (unsigned char *)d;
						dataSize = ds;
					}
					else {
						err = FskMemPtrRealloc(dataSize + ds, &data);
						if (err) {
							FskMemPtrDispose(data);
							xsError(err);
						}
						FskMemMove(data + dataSize, d, ds);
						dataSize += ds;
					}

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
//@@ this can't work.... when _repeat is active
				err = FskSerialIOPlatformRead(sio, (char **)&data, &dataSize, 1);
				if (kFskErrNone == err)
					xsResult = xsInteger(*(unsigned char *)data);
				 else {
					if (kFskErrNoData == err)
						xsResult = xsUndefined;
					else
						xsThrowDiagnosticIfFskErr(err, "Serial read failed with error %s (rx pin %d, tx pin %d).", FskInstrumentationGetErrorString(err), (int)sio->rxNum, (int)sio->txNum);
				}
				break;
		}
		if (data != sio->data)
			FskMemPtrDispose(data);
		if (sio->pollerThread)
			FskMutexRelease(sio->mutex);
	}
	xsCatch {
		if (data != sio->data)
			FskMemPtrDispose(data);
		if (sio->pollerThread)
			FskMutexRelease(sio->mutex);

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
            if ((value < 0) || (value > 255))
                    xsThrowDiagnosticIfFskErr(kFskErrInvalidParameter, "SerialIO Error: Invalid character value (rx pin %d, tx pin %d).", (int)sio->rxNum, (int)sio->txNum);
            err = FskSerialIOPlatformWriteChar(sio, (char)value);
            xsThrowDiagnosticIfFskErr(err, "Serial write failed with error %s (rx pin %d, tx pin %d).", FskInstrumentationGetErrorString(err), (int)sio->rxNum, (int)sio->txNum);
            }
            break;
        
        case xsStringType: {
            char *text = xsToString(*slot);
            err = FskSerialIOPlatformWriteString(sio, text, FskStrLen(text));
            xsThrowDiagnosticIfFskErr(err, "Serial write failed with error %s (rx pin %d, tx pin %d).", FskInstrumentationGetErrorString(err), (int)sio->rxNum, (int)sio->txNum);
            }
            break;

        case xsReferenceType:
            if (xsIsInstanceOf(*slot, xsArrayBufferPrototype)) {
                char *data = xsToArrayBuffer(*slot);
                SInt32 dataSize = xsGetArrayBufferLength(*slot);
                err = FskSerialIOPlatformWriteString(sio, data, dataSize);
                xsThrowDiagnosticIfFskErr(err, "Serial write failed with error %s (rx pin %d, tx pin %d).", FskInstrumentationGetErrorString(err), (int)sio->rxNum, (int)sio->txNum);
			}
			else if (xsIsInstanceOf(*slot, xsChunkPrototype)) {
                char *data = xsGetHostData(*slot);
                SInt32 dataSize = xsToInteger(xsGet(*slot, xsID("length")));
                err = FskSerialIOPlatformWriteString(sio, data, dataSize);
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

	disposePollerThread(sio);

    sio->poller = (xsTest(xsArg(0))) ? xsGetHostData(xsArg(0)) : NULL;

	if (sio->poller) {
		sio->eventfd = eventfd(0, EFD_NONBLOCK);
		if (sio->eventfd >= 0)
			FskThreadCreate(&sio->pollerThread, serialThread, kFskThreadFlagsDefault, sio, "serial poller");
	}
}

void serialThread(void *param)
{
    FskSerialIO sio = param;
	unsigned char buffer[1024];
	struct pollfd fds[2];

	fds[0].fd = sio->eventfd;
	fds[0].events = POLLIN | POLLERR | POLLHUP;

	fds[1].fd = FskSerialIOPlatformGetFD(sio);
	fds[1].events = POLLIN | POLLERR;

//	FskThreadInitializationComplete(FskThreadGetCurrent());
	while (!sio->killed) {
		UInt32 bytesRead;
		int nfds;
		int i;
		Boolean hasData = false;

		poll(fds, 2, -1);

		if (!(fds[1].revents & POLLIN))
			continue;

		if (kFskErrNone != FskSerialIOPlatformReadBlocking(sio, sizeof(buffer), buffer, &bytesRead))
			break;

		FskMutexAcquire(sio->mutex);

		if ((bytesRead + sio->dataCount) > sio->dataAllocated) {
			UInt32 newSize = (bytesRead + sio->dataCount) * 2;
			if (newSize < 8192) newSize = 8192;
			if (kFskErrNone != FskMemPtrRealloc(newSize, &sio->data)) {
				FskMutexRelease(sio->mutex);
				break;
			}
			sio->dataAllocated = newSize;
		}

		FskMemMove(sio->data + sio->dataCount, buffer, bytesRead);
		sio->dataCount += bytesRead;

		FskMutexRelease(sio->mutex);

		FskThreadPostCallback(sio->pinsThread, serialDataReady, sio, NULL, NULL, NULL);
	}
}

void serialDataReady(void *arg0, void *arg1, void *arg2, void *arg3)
{
	FskSerialIO sio = arg0;
	if (sio->dataCount)
		KprPinsPollerRun(sio->poller);
}

void disposePollerThread(FskSerialIO sio)
{
	if (sio->pollerThread) {
		uint64_t one = 1;
		sio->killed = true;
		if (sio->eventfd >= 0)
			write(sio->eventfd, &one, sizeof(one));
		FskThreadJoin(sio->pollerThread);
		if (sio->eventfd >= 0)
			close(sio->eventfd);

		FskMutexAcquire(sio->mutex);
			FskMemPtrDisposeAt(&sio->data);
			sio->dataAllocated = 0;
			sio->dataCount = 0;
			sio->pollerThread = NULL;
			sio->killed = false;
			sio->eventfd = -1;
		FskMutexRelease(sio->mutex);
	}
}


#endif /* USESERIAL */
