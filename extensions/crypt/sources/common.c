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
#include "cryptTypes.h"
#include "common.h"

void cryptThrow_(xsMachine *the, char *code, char *message, xsSlot subError)
{
	xsThrow(xsNew3(xsGet(xsGlobal, xsID("Crypt")), xsID("Error"), xsGet(xsGet(xsGet(xsGlobal, xsID("Crypt")), xsID("error")), xsID(code)), message ? xsString(message): xsUndefined, subError));
}

void cryptThrowFSK_(xsMachine *the, int fskcode)
{
	cryptThrow_(the, "kCryptSystemError", GET_XS_FNAME(), xsNew1(xsGet(xsGet(xsGlobal, xsID("Fsk")), xsID("Error")), xsID("Native"), xsInteger(fskcode)));
}

void
getChunkData(xsMachine *the, xsSlot *chunk, void **datap, xsIntegerValue *sizep)
{
	if (!xsIsInstanceOf(*chunk, xsChunkPrototype))
		cryptThrow("kCryptTypeError");
	*datap = xsGetHostData(*chunk);
	*sizep = xsToInteger(xsGet(*chunk, xsID("length")));
}

void
getInputData(xsMachine *the, xsSlot *input, void **datap, xsIntegerValue *sizep)
{
	if (xsIsInstanceOf(*input, xsChunkPrototype)) {
		*datap = xsGetHostData(*input);
		*sizep = xsToInteger(xsGet(*input, xsID("length")));
	}
	else if (xsTypeOf(*input) == xsStringType) {
		*datap = xsToString(*input);
		*sizep = FskStrLen(*datap);
	}
	else
		cryptThrow("kCryptTypeError");
}

void
xs_crypt_exit(xsMachine *the)
{
	FskExit(xsToInteger(xsArgc) > 0 ? xsToInteger(xsArg(0)): 0);
}

#if SUPPORT_INSTRUMENTATION
Boolean cryptInstrumentFormat(FskInstrumentedType dispatch, UInt32 msg, void *msgData, char *buffer, UInt32 bufferSize)
{
	char *name;
	SInt32 len;

	switch (msg) {
	case kCryptInstrMsg:
		/* this would not work when the formatter function is called asynchronously... */
		if ((name = GET_XS_FNAME()) != NULL) {
			FskStrNCopy(buffer, name, bufferSize);
			buffer[bufferSize - 1] = '\0';
			len = FskStrLen(buffer);
			bufferSize -= len;
			buffer += len;
		}
		if (msgData != NULL && bufferSize > 0) {
			/* msgData should contain an extra message */
			FskStrNCopy(buffer, msgData, bufferSize);
			buffer[bufferSize - 1] = '\0';
		}
		return true;
	default:
		/* unknown message */
		return false;
	}
}
#endif
