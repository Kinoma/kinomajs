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
#include "xs6Platform.h"
#undef mxImport
#define mxImport extern
#include "expat.h"
#include "xs.h"

#ifdef KPR_CONFIG
#include "FskExtensions.h"
#include "FskManifest.xs.h"

FskExport(FskErr) xsChunk_fskLoad(FskLibrary it)
{
	return kFskErrNone;
}

FskExport(FskErr) xsChunk_fskUnload(FskLibrary it)
{
	return kFskErrNone;
}
#endif

void xs_chunk(void* it)
{
	if (it)
		c_free(it);
}

void xs_Chunk(xsMachine* the)
{
	xsIntegerValue c = xsToInteger(xsArgc);
	xsIntegerValue length = 0, offset = 0;
	void* data = C_NULL;
	xsVars(2);
	if (c > 0) {
		xsType type = xsTypeOf(xsArg(0));
		if ((type == xsIntegerType) || (type == xsNumberType))
			length = xsToInteger(xsArg(0));
		else if ((type == xsStringType) || (type == xsStringXType))
			xsVar(0) = xsArg(0);
		else if (type == xsReferenceType) {
			if (xsIsInstanceOf(xsArg(0), xsNumberPrototype))
				length = xsToInteger(xsArg(0));
			else if (xsIsInstanceOf(xsArg(0), xsStringPrototype))
				xsVar(0) = xsArg(0);
			else if (xsIsInstanceOf(xsArg(0), xsChunkPrototype)) {
				length = xsToInteger(xsGet(xsArg(0), xsID("(chunk)")));
				if (c > 1) {
					offset = xsToInteger(xsArg(1));
					if ((offset < 0) || (length < offset))
						xsRangeError("out of range offset %ld", offset);
					if (c > 2) {
						xsIntegerValue size = xsToInteger(xsArg(2));
						if ((size < 0) || (length < (offset + size)))
							xsRangeError("out of range size %ld", size);
						length = size;
					}
					else
						length -= offset;
				}
				xsVar(1) = xsArg(0);
			}
			else
				xsRangeError("invalid parameter");
		}
		else
			xsRangeError("invalid parameter");
	}
	if (xsTest(xsVar(0))) {
		txU1* src = (txU1*)xsToString(xsVar(0));
		txU1 aByte;
		xsIntegerValue srcIndex = 0;
		xsIntegerValue dstIndex = 3;
		xsBooleanValue aFlag = 0;
		while ((aByte = *src++)) {
			if (('A' <= aByte) && (aByte <= 'Z'))
				srcIndex++;
			else if (('a' <= aByte) && (aByte <= 'z'))
				srcIndex++;
			else if (('0' <= aByte) && (aByte <= '9'))
				srcIndex++;
			else if (aByte == '+')
				srcIndex++;
			else if (aByte == '/')
				srcIndex++;
			else if (aByte == '=') {
				if (srcIndex == 2) {
					if (*src == '=') {
						srcIndex++;
						dstIndex = 1;
						srcIndex++;
						aFlag = 1;
					}
					else
						continue;
				}
				else if (srcIndex == 3) {
					dstIndex = 2;
					srcIndex++;
					aFlag = 1;
				}
				else
					continue;
			}
			else
				continue; 
			if (srcIndex == 4) {
				length += dstIndex;
				srcIndex = 0;
			}
			if (aFlag)
				break;
		}
	}
	if (length >= 0) {
		data = c_malloc(length);
		if (!data)
			xsRangeError("not enough memory %ld", length);
		if (xsTest(xsVar(0))) {
			txU1 aBuffer[4];
			txU1* src = (txU1*)xsToString(xsVar(0));
			txU1* dst = data;
			txU1 aByte;
			xsIntegerValue srcIndex = 0;
			xsIntegerValue dstIndex = 3;
			xsBooleanValue aFlag = 0;
			while ((aByte = *src++)) {
				if (('A' <= aByte) && (aByte <= 'Z'))
					aByte = aByte - 'A';
				else if (('a' <= aByte) && (aByte <= 'z'))
					aByte = aByte - 'a' + 26;
				else if (('0' <= aByte) && (aByte <= '9'))
					aByte = aByte - '0' + 52;
				else if (aByte == '+')
					aByte = 62;
				else if (aByte == '/')
					aByte = 63;
				else if (aByte == '=') {
					if (srcIndex == 2) {
						if (*src == '=') {
							aBuffer[srcIndex++] = 0;
							dstIndex = 1;
							aByte = 0;
							aFlag = 1;
						}
						else
							continue;
					}
					else if (srcIndex == 3) {
						dstIndex = 2;
						aByte = 0;
						aFlag = 1;
					}
					else
						continue;
				}
				else
					continue; 
				aBuffer[srcIndex++] = aByte;
				if (srcIndex == 4) {
					*dst++ = (aBuffer[0] << 2) | ((aBuffer[1] & 0x30) >> 4);
					if (dstIndex > 1)
						*dst++ = ((aBuffer[1] & 0x0F) << 4) | ((aBuffer[2] & 0x3C) >> 2);
					if (dstIndex > 2)
						*dst++ = ((aBuffer[2] & 0x03) << 6) | (aBuffer[3] & 0x3F);
					srcIndex = 0;
				}
				if (aFlag)
					break;
			}
		}
		else if (xsTest(xsVar(1)))
			c_memcpy(data, ((txU1*)xsGetHostData(xsVar(1))) + offset, length);
		else
			c_memset(data, 0, length);
	}
	xsSetHostDestructor(xsThis, xs_chunk);
	xsSetHostData(xsThis, data);
	xsSet(xsThis, xsID("(chunk)"), xsInteger(length));
}

void xs_chunk_get_length(xsMachine* the)
{
	xsResult = xsGet(xsThis, xsID("(chunk)"));
}

void xs_chunk_set_length(xsMachine* the)
{
	void* destructor = xsGetHostDestructor(xsThis);
	void* fromData = xsGetHostData(xsThis);
	void* toData = C_NULL;
	xsIntegerValue fromLength = xsToInteger(xsGet(xsThis, xsID("(chunk)")));
	xsIntegerValue toLength = xsToInteger(xsArg(0));
	if (toLength < 0)
		xsRangeError("not in range %ld", toLength);
	if (fromLength != toLength) {
		if (destructor == xs_chunk) {
			if (toLength) {
				if (fromData)
					toData = c_realloc(fromData, toLength);
				if (toData) {
					if (fromLength < toLength)
						c_memset(((txU1*)toData) + fromLength, 0, toLength - fromLength);
				}
				else {
					toData = c_malloc(toLength);
					if (!toData)
						xsRangeError("not enough memory %ld", toLength);
					if (fromLength < toLength) {
						if (fromLength)
							c_memcpy(toData, fromData, fromLength);
						c_memset(((txU1*)toData) + fromLength, 0, toLength - fromLength);
					}
					else
						c_memcpy(toData, fromData, toLength);
				}
			}
			else {
				if (fromData)
					c_free(fromData);
			}
			xsSetHostData(xsThis, toData);
		}
		xsSet(xsThis, xsID("(chunk)"), xsInteger(toLength));
	}
}

void xs_chunk_append(xsMachine* the)
{
	xsDestructor destructor = xsGetHostDestructor(xsThis);
	void* data = xsGetHostData(xsThis);
	xsIntegerValue length = xsToInteger(xsGet(xsThis, xsID("length")));
	void* paramData;
	xsIntegerValue paramLength;
	void* resultData;
	xsIntegerValue resultLength;
	if (xsToInteger(xsArgc) < 1)
		xsSyntaxError("not enough parameters");
	paramData = xsGetHostData(xsArg(0));
	paramLength = xsToInteger(xsGet(xsArg(0), xsID("length")));
	resultLength = length + paramLength;
	if (resultLength) {
		resultData = c_malloc(resultLength);
		if (!resultData)
			xsRangeError("not enough memory %ld", resultLength);
		if (data && length)
			c_memcpy(resultData, data, length);
		if (paramData && paramLength)
			c_memcpy(((txU1*)resultData) + length, paramData, paramLength);
	}
	else
		resultData = C_NULL;
	if (destructor)
		(*destructor)(data);
	xsSetHostDestructor(xsThis, xs_chunk);
	xsSetHostData(xsThis, resultData);
	xsSet(xsThis, xsID("(chunk)"), xsInteger(resultLength));
}

void xs_chunk_free(xsMachine* the)
{
	xsDestructor destructor = xsGetHostDestructor(xsThis);
	void* data = xsGetHostData(xsThis);
	if (destructor)
		(*destructor)(data);
	xsSetHostData(xsThis, C_NULL);
	xsSet(xsThis, xsID("(chunk)"), xsInteger(0));
}

void xs_chunk_peek(xsMachine* the)
{
	txU1* data = xsGetHostData(xsThis);
	xsIntegerValue length = xsToInteger(xsGet(xsThis, xsID("length")));
	xsIntegerValue index;
	if (xsToInteger(xsArgc) < 1)
		xsSyntaxError("not enough parameters");
	index = xsToInteger(xsArg(0));
	if ((index < 0) || (length <= index))
		xsRangeError("index not in range %ld", index);
	xsResult = xsInteger(data[index]);
}

void xs_chunk_poke(xsMachine* the)
{
	txU1* data = xsGetHostData(xsThis);
	xsIntegerValue length = xsToInteger(xsGet(xsThis, xsID("length")));
	xsIntegerValue index, value;
	if (xsToInteger(xsArgc) < 2)
		xsSyntaxError("not enough parameters");
	index = xsToInteger(xsArg(0));
	if ((index < 0) || (length <= index))
		xsRangeError("index not in range %ld", index);
	value = xsToInteger(xsArg(1));
	if ((value < 0) || (256 <= value))
		xsRangeError("value not in range %ld", value);
	data[index] = (txU1)value;
}

void xs_chunk_slice(xsMachine* the)
{
	xsIntegerValue c = xsToInteger(xsArgc);
	xsIntegerValue length = xsToInteger(xsGet(xsThis, xsID("length")));
	xsIntegerValue start = 0;
	xsIntegerValue stop = length;
	if (c > 0) {
		start = xsToInteger(xsArg(0));
		if (start < 0) {
			start = length + start;
			if (start < 0)
				start = 0;
		}
		else if (start > length)
			start = length;
	}
	if (c > 1) {
		stop = xsToInteger(xsArg(1));
		if (stop < 0) {
			stop = length + stop;
			if (stop < 0)
				stop = 0;
		}
		else if (stop > length)
			stop = length;
	}
	if (start < stop)
		length = stop - start;
	else
		length = 0;
	xsResult = xsNew1(xsGlobal, xsID("Chunk"), xsInteger(length));
	if (length) {
		void* fromData = xsGetHostData(xsThis);
		void* toData = xsGetHostData(xsResult);
		c_memcpy(toData, ((txU1*)fromData) + start, length);
	}
}

void xs_chunk_toString(xsMachine* the)
{
	static char base64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	xsIntegerValue length = xsToInteger(xsGet(xsThis, xsID("length")));
	txU1* src = xsGetHostData(xsThis);
	char* dst;
	txU1 byte0, byte1, byte2;
	xsResult = xsStringBuffer(C_NULL, (((length + 2) / 3) * 4) + 1);
	dst = xsToString(xsResult);
	while (length > 2) {
		byte0 = *src++;
		byte1 = *src++;
		byte2 = *src++;
		*dst++ = base64[((byte0 & 0xFC) >> 2)];
		*dst++ = base64[(((byte0 & 0x03) << 4) | ((byte1 & 0xF0) >> 4))];
		*dst++ = base64[(((byte1 & 0x0F) << 2) | ((byte2 & 0xC0) >> 6))];
		*dst++ = base64[(byte2 & 0x3F)];
		length -= 3;
	}
	if (length == 2) {
		byte0 = *src++;
		byte1 = *src++;
		*dst++ = base64[((byte0 & 0xFC) >> 2)];
		*dst++ = base64[(((byte0 & 0x03) << 4) | ((byte1 & 0xF0) >> 4))];
		*dst++ = base64[((byte1 & 0x0F) << 2)];
		*dst++ = '=';
	}
	else if (length == 1) {
		byte0 = *src++;
		*dst++ = base64[((byte0 & 0xFC) >> 2)];
		*dst++ = base64[((byte0 & 0x03) << 4)];
		*dst++ = '=';
		*dst++ = '=';
	}
	*dst = 0;
}
