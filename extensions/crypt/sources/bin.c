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
#include "cryptTypes.h"
#include "common.h"


void
xs_bin_toRawString(xsMachine *the)
{
	int ac = xsToInteger(xsArgc);
	int start = 0, end, size;
	char *buffer;

	end = size = (UInt32)xsToInteger(xsGet(xsThis, xsID("length")));
	if (ac > 0) {
		start = xsToInteger(xsArg(0));
		if (start > size)
			start = size;
	}
	if (ac > 1) {
		end = xsToInteger(xsArg(1));
		if (end < 0) {
			if ((end = size + end) < start)
				end = start;
		}
		else if (end > size)
			end = size;
	}
	buffer = xsGetHostData(xsThis);
	xsResult = xsStringBuffer(&buffer[start], end - start);
}

void
xs_bin_setRawString(xsMachine *the)
{
	char *str = xsToStringCopy(xsArg(0));
	UInt32 size = FskStrLen(str);

	xsSet(xsThis, xsID("length"), xsInteger(size));
	FskMemCopy(xsGetHostData(xsThis), str, size);
	FskMemPtrDispose(str);
}

void
xs_bin_xor(xsMachine *the)
{
	UInt8 *c1, *c2, *c3;
	xsIntegerValue len1, len2, len3;

	getChunkData(the, &xsArg(0), (void **)&c1, &len1);
	getChunkData(the, &xsArg(1), (void **)&c2, &len2);
	if (len1 > len2)
		len1 = len2;
	if (xsToInteger(xsArgc) > 2 && xsIsInstanceOf(xsArg(2), xsChunkPrototype)) {
		len3 = xsToInteger(xsGet(xsArg(2), xsID("length")));
		if (len3 < len1)
			len1 = len3;
		xsResult = xsArg(2);
	}
	else
		xsResult = xsNew1(xsGlobal, xsID("Chunk"), xsInteger(len1));
	c3 = xsGetHostData(xsResult);
	while (--len1 >= 0)
		*c3++ = *c1++ ^ *c2++;
}

void
xs_bin_comp(xsMachine *the)
{
	void *c1, *c2;
	xsIntegerValue len1, len2;

	getChunkData(the, &xsThis, &c1, &len1);
	getChunkData(the, &xsArg(0), &c2, &len2);
	if (len1 != len2)
		xsResult = xsInteger(len2 - len1);
	else
		xsResult = xsInteger(FskMemCompare(c1, c2, (UInt32)len1));
}

void
xs_bin_ncomp(xsMachine *the)
{
	void *c1, *c2;
	xsIntegerValue len1, len2, n;

	getChunkData(the, &xsThis, &c1, &len1);
	getChunkData(the, &xsArg(0), &c2, &len2);
	n = xsToInteger(xsArg(1));
	if (n <= len1 && n <= len2)
		xsResult = xsInteger(FskMemCompare(c1, c2, (UInt32)n));
	else
		xsResult = xsInteger(len1 == len2 ? FskMemCompare(c1, c2, (UInt32)len1): len2 - len1);
}

void
xs_bin_peek32(xsMachine *the)
{
	UInt8 *p = xsGetHostData(xsThis);
	xsIntegerValue size = xsToInteger(xsGet(xsThis, xsID("length")));
	xsIntegerValue i = xsToInteger(xsArg(0));

	if (i + 3 < size)
		xsResult = xsInteger((p[i] << 24) | (p[i+1] << 16) | (p[i+2] << 8) | p[i+3]);
}

void
xs_bin_poke32(xsMachine *the)
{
	UInt8 *p = xsGetHostData(xsThis);
	xsIntegerValue size = xsToInteger(xsGet(xsThis, xsID("length")));
	xsIntegerValue i = xsToInteger(xsArg(0));
	UInt32 v = (UInt32)xsToInteger(xsArg(1));

	if (i + 3 < size) {
		p[i] = (UInt8)(v >> 24);
		p[i+1] = (UInt8)(v >> 16);
		p[i+2] = (UInt8)(v >> 8);
		p[i+3] = (UInt8)v;
	}
}

void
xs_bin_copy(xsMachine *the)
{
	UInt8 *data = xsGetHostData(xsThis);
	UInt32 size = xsToInteger(xsGet(xsThis, xsID("length")));
	UInt32 offset = (UInt32)xsToInteger(xsArg(0));
	UInt8 *srcdata = xsGetHostData(xsArg(1));
	UInt32 srcsize = xsToInteger(xsGet(xsArg(1), xsID("length")));

	if (offset + srcsize <= size)
		FskMemCopy(data + offset, srcdata, srcsize);
}

#define qtRead32(p)	((p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3])
#define qtWrite32(p, v)	(p[0] = (UInt8)((v) >> 24), p[1] = (UInt8)((v) >> 16), p[2] = (UInt8)((v) >> 8), p[3] = (UInt8)(v))

void
xs_bin_getAtom(xsMachine *the)
{
	UInt8 *data = xsGetHostData(xsThis);
	UInt32 size = xsToInteger(xsGet(xsThis, xsID("length")));
	char *atomName = xsToString(xsArg(0));
	UInt32 targetAtom = qtRead32(atomName);
	UInt8 *endp = data + size;

	while (data + 8 < endp) {
		UInt8 *subData = data;
		UInt32 atomSize, atomType;
		atomSize = qtRead32(subData); subData += 4;
		atomType = qtRead32(subData); subData += 4;
		if (atomType == targetAtom) {
			size = atomSize - 8;
			xsResult = xsNew1(xsGlobal, xsID("Chunk"), xsInteger(size));
			data = xsGetHostData(xsResult);
			FskMemCopy(data, subData, size);
			break;
		}
		data += atomSize;
	}
}

void
xs_bin_putAtom(xsMachine *the)
{
	UInt8	*data;
	UInt32	size				= xsToInteger(xsGet(xsThis, xsID("length")));
	char	*atomName			= xsToString(xsArg(0));
	UInt32	atomType			= qtRead32(atomName);
	UInt8	*atomData			= xsGetHostData(xsArg(1));
	UInt32	atomDataSize		= xsToInteger(xsGet(xsArg(1), xsID("length")));
	UInt32	atomSize			= atomDataSize + 8;
	UInt32	version_and_flags	= 0;
	UInt8	*p;
	Boolean	fullatom;

	if ((fullatom = (xsToInteger(xsArgc) > 2))) {
		version_and_flags = xsToInteger(xsArg(2));
		atomSize += 4;
	}
	xsSet(xsThis, xsID("length"), xsInteger(size + atomSize));
	data = xsGetHostData(xsThis);
	p = data + size;
	qtWrite32(p, atomSize); p += 4;
	qtWrite32(p, atomType); p += 4;
	if (fullatom) {
		qtWrite32(p, version_and_flags); p += 4;
	}
	FskMemCopy(p, atomData, atomDataSize);
}


/*
 * atom iterator
 */

typedef struct {
	UInt8 *datap, *endp;
} atomIterator;

void
xs_bin_atomIterator(xsMachine *the)
{
	atomIterator *iter;
	FskErr err;

	if ((err = FskMemPtrNew(sizeof(atomIterator), (FskMemPtr *)&iter)) != kFskErrNone)
		cryptThrowFSK(err);
	iter->datap = xsGetHostData(xsArg(0));
	iter->endp = iter->datap + xsToInteger(xsGet(xsArg(0), xsID("length")));
	xsSetHostData(xsThis, iter);
	xsSet(xsThis, xsID("__data__"), xsArg(0));	/* avoid GC */
}

void
xs_bin_atomIterator_destructor(void *data)
{
	if (data)
		FskMemPtrDispose(data);
}

void
xs_bin_atomIterator_next(xsMachine *the)
{
	atomIterator *iter = xsGetHostData(xsThis);

	if (iter->datap + 8 < iter->endp) {
		UInt32 atomSize = qtRead32(iter->datap);
		iter->datap += atomSize;
		if (iter->datap < iter->endp)
			xsResult = xsThis;
		else
			xsResult = xsNull;
	}
	else
		xsResult = xsNull;
}

void
xs_bin_atomIterator_getName(xsMachine *the)
{
	atomIterator *iter = xsGetHostData(xsThis);

	if (iter->datap + 8 < iter->endp) {
		char buf[5];
		FskMemCopy(buf, iter->datap + 4, 4);
		buf[4] = '\0';
		xsResult = xsString(buf);
	}
	else
		xsResult = xsUndefined;
}

void
xs_bin_atomIterator_getValue(xsMachine *the)
{
	atomIterator *iter = xsGetHostData(xsThis);

	if (iter->datap + 8 < iter->endp) {
		UInt32 atomSize = qtRead32(iter->datap);
		xsResult = xsNew1(xsGlobal, xsID("Chunk"), xsInteger(atomSize - 8));
		FskMemCopy(xsGetHostData(xsResult), iter->datap + 8, atomSize - 8);
	}
	else
		xsResult = xsUndefined;
}
