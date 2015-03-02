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
#ifndef __CRYPT_COMMON__
#define __CRYPT_COMMON__

#include "cryptTypes.h"

#if SUPPORT_XS_DEBUG && 0
#include "FskEnvironment.h"
#define CRYPT_XS_ENV	"CRYPT_XS_NAME"
#define XS_FNAME(xsname)	FskEnvironmentSet(CRYPT_XS_ENV, xsname)
#define GET_XS_FNAME()	FskEnvironmentGet(CRYPT_XS_ENV)
#else
#define XS_FNAME(xsname)
#define GET_XS_FNAME()	NULL
#endif

/*
 * error handling policy:
 *  - all xs functions may throw an error (which is an instance of Crypt.error)
 *  - sub functions in "arith" and "primitives" never throw errors
 *    if an xs function needs to call a function which may throw an error, make sure the xs function catches and re-throws it with its own code and message
 *  - high level functions (such as XML, WSSE -- typically implemented in script) which make use of the primitive and arith functions
 *    may throw an error, and the error object may have sub errors
 */

extern void cryptThrow_(xsMachine *the, char *code, char *message, xsSlot subError) FSK_FUNCTION_ANALYZER_NORETURN;
extern void cryptThrowFSK_(xsMachine *the, int fskcode) FSK_FUNCTION_ANALYZER_NORETURN;
#define cryptThrow(code)	cryptThrow_(the, code, GET_XS_FNAME(), xsUndefined)
#define cryptThrowFSK(fskcode)	cryptThrowFSK_(the, fskcode)

extern void getChunkData(xsMachine *the, xsSlot *chunk, void **datap, xsIntegerValue *sizep);
extern void getInputData(xsMachine *the, xsSlot *input, void **datap, xsIntegerValue *sizep);

#if SUPPORT_INSTRUMENTATION
extern Boolean cryptInstrumentFormat(FskInstrumentedType dispatch, UInt32 msg, void *msgData, char *buffer, UInt32 bufferSize);

enum {
	kCryptInstrMsg = kFskInstrumentedItemFirstCustomMessage,
};

#endif

#endif /* __CRYPT_COMMON__ */
