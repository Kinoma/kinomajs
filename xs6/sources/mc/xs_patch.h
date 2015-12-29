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
#ifndef __XS_PATCH_H__
#define __XS_PATCH_H__

#include "xs.h"

typedef struct {
	unsigned int maximum;
	unsigned int current;
	unsigned int peak;
	unsigned int nblocks;
} xsMemoryUse;
extern void xsReportMemoryUse(xsMachine *the, xsMemoryUse *slot, xsMemoryUse *chunk);
extern int xsStartDebug(xsMachine *the, const char *host);
extern void xsStopDebug(xsMachine *the);
extern xsBooleanValue xsGetCollectFlag(xsMachine *the);

#undef fxPop
#undef fxPush

static inline xsSlot _fxPop(xsMachine *the)
{
	return *the->stack++;
}

static inline void _fxPush(xsMachine *the, xsSlot *v)
{
	*--the->stack = *v;
}

#define fxPop()		_fxPop(the)
#define fxPush(v)	_fxPush(the, &v)

#undef xsTypeOf
#define xsTypeOf(_SLOT)	fxTypeOf(the, &_SLOT)

#undef xsToBoolean
#undef xsToInteger
#undef xsToNumber
#undef xsToString
#undef xsToStringBuffer
#define xsToBoolean(_SLOT)	fxToBoolean(the, &_SLOT)
#define xsToInteger(_SLOT)	fxToInteger(the, &_SLOT)
#define xsToNumber(_SLOT)	fxToNumber(the, &_SLOT)
#define xsToString(_SLOT)	fxToString(the, &_SLOT)
#define xsToStringBuffer(_SLOT,_BUFFER,_SIZE)	fxToStringBuffer(the, &_SLOT, _BUFFER ,_SIZE)

#define xsSetUndefined(_SLOT)	fxUndefined(the, &_SLOT)
#define xsSetNull(_SLOT)	fxNull(the, &_SLOT)
#define xsSetFalse(_SLOT)	fxBoolean(the, &_SLOT, 0)
#define xsSetTrue(_SLOT)	fxBoolean(the, &_SLOT, 1)
#define xsSetBoolean(_SLOT, _VALUE)	 fxBoolean(the, &_SLOT, _VALUE)
#define xsSetInteger(_SLOT, _VALUE)	fxInteger(the, &_SLOT, _VALUE)
#define xsSetNumber(_SLOT, _VALUE)	fxNumber(the, &_SLOT, _VALUE)
#define xsSetString(_SLOT, _VALUE)	fxString(the, &_SLOT, _VALUE)
#define xsSetStringBuffer(_SLOT, _BUFFER,_SIZE)	fxStringBuffer(the, &_SLOT, _BUFFER ,_SIZE)

#define xsSetArrayBuffer(_SLOT, _BUFFER, _SIZE)	fxArrayBuffer(the, &_SLOT, _BUFFER, _SIZE)
#undef xsGetArrayBufferData
#define xsGetArrayBufferData(_SLOT,_OFFSET,_BUFFER,_SIZE)	fxGetArrayBufferData(the, &_SLOT, _OFFSET, _BUFFER, _SIZE)
#undef xsSetArrayBufferData
#define xsSetArrayBufferData(_SLOT,_OFFSET,_BUFFER,_SIZE)	fxSetArrayBufferData(the, &_SLOT, _OFFSET, _BUFFER, _SIZE)
#undef xsToArrayBuffer
#define xsToArrayBuffer(_SLOT)	fxToArrayBuffer(the, &_SLOT)

#undef xsNewInstanceOf
#undef xsIsInstanceOf
extern void _xsNewInstanceOf(xsMachine *, xsSlot *, xsSlot *);
extern xsBooleanValue _xsIsInstanceOf(xsMachine *, xsSlot *, xsSlot *);
#define xsNewInstanceOf(_PROTOTYPE)	(_xsNewInstanceOf(the, &the->scratch, &_PROTOTYPE), the->scratch)
#define xsSetNewInstanceOf(_SLOT, _PROTOTYPE)	_xsNewInstanceOf(the, &_SLOT, &_PROTOTYPE)
#define xsIsInstanceOf(_SLOT,_PROTOTYPE)	_xsIsInstanceOf(the, &_SLOT, &_PROTOTYPE)

#undef xsHas
#undef xsHasOwn
#undef xsGet
#undef xsGetAt
#undef xsSet
#undef xsSetAt
#undef xsDelete
#undef xsDeleteAt
extern xsBooleanValue _xsHas(xsMachine *, xsSlot *, xsIndex);
#define xsHas(_THIS, _ID)	_xsHas(the, &_THIS, _ID)
extern xsBooleanValue _xsHasOwn(xsMachine *, xsSlot *, xsIndex);
#define xsHasOwn(_THIS, _ID)	_xsHasOwn(the, &_THIS, _ID)
extern void _xsGet(xsMachine *, xsSlot *, xsSlot *, xsIndex);
#define xsGet(_SLOT, _THIS, _ID)	_xsGet(the, &_SLOT, &_THIS, _ID)
extern void _xsGetAt(xsMachine *, xsSlot *, xsSlot *, xsSlot *);
#define xsGetAt(_SLOT, _THIS, _AT)	_xsGetAt(the, &_SLOT, &_THIS, &_AT)
extern void _xsSet(xsMachine *, xsSlot *, xsIndex, xsSlot *);
#define xsSet(_THIS, _ID, _SLOT)	_xsSet(the, &_THIS, _ID, &_SLOT)
extern void _xsSetAt(xsMachine *, xsSlot *, xsSlot *, xsSlot *);
#define xsSetAt(_THIS, _AT, _SLOT)	_xsSetAt(the, &_THIS, &_AT, &_SLOT)
extern void _xsDelete(xsMachine *, xsSlot *, xsIndex);
#define xsDelete(_THIS, _ID)	_xsDelete(the, &_THIS, _ID)
extern void _xsDeleteAt(xsMachine *, xsSlot *, xsSlot *);
#define xsDeleteAt(_THIS, _AT)	_xsDeleteAt(the, &_THIS, &_AT)
extern void _xsCall(xsMachine *, xsSlot *, xsSlot *, xsIndex, ...);
#define xsCall(_RES, _THIS, _ID, ...)		_xsCall(the, &_RES, &_THIS, _ID, __VA_ARGS__)
#define xsCall_noResult(_THIS, _ID, ...)	_xsCall(the, NULL, &_THIS, _ID, __VA_ARGS__)
extern void _xsNew(xsMachine *, xsSlot *, xsSlot *, xsIndex, ...);
#define xsNew(_RES, _THIS, _ID, ...)	_xsNew(the, &_RES, &_THIS, _ID, __VA_ARGS__)
extern xsBooleanValue _xsTest(xsMachine *, xsSlot *);
#undef xsTest
#define xsTest(_SLOT)	_xsTest(the, &_SLOT)

#undef xsGetHostData
#undef xsSetHostData
#define xsGetHostData(_SLOT)	fxGetHostData(the, &_SLOT)
#define xsSetHostData(_SLOT,_DATA)	fxSetHostData(the, &_SLOT, _DATA)

#undef xsVars
mxImport xsIntegerValue fxIncrementalVars(xsMachine*, xsIntegerValue);
#define xsVars(_COUNT) fxIncrementalVars(the, _COUNT)

#endif	/* __XS_PATCH_H__ */
