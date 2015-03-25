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

#include "xsAll.h"

#define mxCheckChunk(THE_SLOT) \
	if (!mxIsChunk(THE_SLOT)) \
		mxDebug0(the, XS_TYPE_ERROR, "this is no Chunk")

typedef void (*txTypedArrayCallback)(txMachine*, void*, txSlot*);

typedef struct {
	char* constructorID;
	char* getID;
	char* setID;
	txInteger size;
	txTypedArrayCallback getter;
	txTypedArrayCallback setter;
} txTypedArrayDispatch;

static void fx_Chunk_destructor(void* theData);
static void fx_Chunk(txMachine* the);
static void fx_Chunk_aux(txMachine* the, txString theString, txU1** theData, txInteger* theLength);
static void fx_Chunk_get_length(txMachine* the);
static void fx_Chunk_set_length(txMachine* the);
static void fx_Chunk_append(txMachine* the);
static void fx_Chunk_free(txMachine* the);
static void fx_Chunk_peek(txMachine* the);
static void fx_Chunk_poke(txMachine* the);
static void fx_Chunk_slice(txMachine* the);
/* static */ void fx_Chunk_toString(txMachine* the);

static void fx_ArrayBuffer(txMachine* the);

static void fx_DataView(txMachine* the);
static void fx_DataView_get(txMachine* the);
static void fx_DataView_set(txMachine* the);

static void fx_TypedArray(txMachine* the);
static void fx_TypedArray_peek(txMachine* the);
static void fx_TypedArray_poke(txMachine* the);
static void fx_TypedArray_set(txMachine* the);
static void fx_TypedArray_subarray(txMachine* the);

static void fxFloat32Getter(txMachine* the, void* theData, txSlot* theSlot);
static void fxFloat32Setter(txMachine* the, void* theData, txSlot* theSlot);
static void fxFloat64Getter(txMachine* the, void* theData, txSlot* theSlot);
static void fxFloat64Setter(txMachine* the, void* theData, txSlot* theSlot);
static void fxInt8Getter(txMachine* the, void* theData, txSlot* theSlot);
static void fxInt8Setter(txMachine* the, void* theData, txSlot* theSlot);
static void fxInt16Getter(txMachine* the, void* theData, txSlot* theSlot);
static void fxInt16Setter(txMachine* the, void* theData, txSlot* theSlot);
static void fxInt32Getter(txMachine* the, void* theData, txSlot* theSlot);
static void fxInt32Setter(txMachine* the, void* theData, txSlot* theSlot);
static void fxUint8Getter(txMachine* the, void* theData, txSlot* theSlot);
static void fxUint8Setter(txMachine* the, void* theData, txSlot* theSlot);
static void fxUint16Getter(txMachine* the, void* theData, txSlot* theSlot);
static void fxUint16Setter(txMachine* the, void* theData, txSlot* theSlot);
static void fxUint32Getter(txMachine* the, void* theData, txSlot* theSlot);
static void fxUint32Setter(txMachine* the, void* theData, txSlot* theSlot);
static void fxUint8ClampedSetter(txMachine* the, void* theData, txSlot* theSlot);

#define mxTypedArrayDispatchCount 9
txTypedArrayDispatch gTypedArrayDispatches[mxTypedArrayDispatchCount] = {
	{ "Float32Array", "getFloat32", "setFloat32", 4, fxFloat32Getter, fxFloat32Setter },
	{ "Float64Array", "getFloat64", "setFloat64", 8, fxFloat64Getter, fxFloat64Setter },
	{ "Int8Array", "getInt8", "setInt8", 1, fxInt8Getter, fxInt8Setter },
	{ "Int16Array", "getInt16", "setInt16", 2, fxInt16Getter, fxInt16Setter },
	{ "Int32Array", "getInt32", "setInt32", 4, fxInt32Getter, fxInt32Setter },
	{ "Uint8Array", "getUint8", "setUint8", 1, fxUint8Getter, fxUint8Setter },
	{ "Uint16Array", "getUint16", "setUint16", 2, fxUint16Getter, fxUint16Setter },
	{ "Uint32Array", "getUint32", "setUint32", 4, fxUint32Getter, fxUint32Setter },
	{ "Uint8ClampedArray", "getUint8Clamped", "setUint8Clamped", 1, fxUint8Getter, fxUint8ClampedSetter }
};

void fxBuildChunk(txMachine* the)
{
	txInteger anIndex;
    txSlot* aProperty;
	txTypedArrayDispatch *aDispatch;
	
	mxPush(mxGlobal);
			
	mxPush(mxObjectPrototype);
	fxNewChunkInstance(the);
	fxNewHostFunction(the, fx_Chunk_get_length, 0);
	fxQueueID(the, the->lengthID, XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_GETTER_FLAG);
	fxNewHostFunction(the, fx_Chunk_set_length, 1);
	fxQueueID(the, the->lengthID, XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_SETTER_FLAG);
	fxNewHostFunction(the, fx_Chunk_append, 1);
	fxQueueID(the, fxID(the, "append"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_Chunk_free, 0);
	fxQueueID(the, fxID(the, "free"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_Chunk_peek, 1);
	fxQueueID(the, the->peekID, XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_Chunk_poke, 2);
	fxQueueID(the, the->pokeID, XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_Chunk_slice, 2);
	fxQueueID(the, fxID(the, "slice"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_Chunk_toString, 0);
	fxQueueID(the, fxID(the, "toString"), XS_DONT_ENUM_FLAG);
	fxAliasInstance(the, the->stack);
	mxChunkPrototype = *the->stack;
	fxNewHostConstructor(the, fx_Chunk, 1);
	the->stack->value.reference->next->next->next->flag |= XS_DONT_SET_FLAG;
	 *(--the->stack) = mxChunkPrototype;
	fxPutID(the, fxID(the, "constructor"), XS_DONT_ENUM_FLAG, XS_DONT_ENUM_FLAG);
	fxQueueID(the, fxID(the, "Chunk"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	
	mxPush(mxObjectPrototype);
	fxNewChunkInstance(the);
	fxNewHostFunction(the, fx_Chunk_get_length, 0);
	fxQueueID(the, fxID(the, "byteLength"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_GETTER_FLAG);
	fxNewHostFunction(the, fx_Chunk_slice, 1);
	fxQueueID(the, fxID(the, "slice"), XS_DONT_ENUM_FLAG);
	fxAliasInstance(the, the->stack);
	the->scratch = *the->stack;	
	fxNewHostConstructor(the, fx_ArrayBuffer, 1);
	the->stack->value.reference->next->next->next->flag |= XS_DONT_SET_FLAG;
	 *(--the->stack) = the->scratch;
	fxPutID(the, fxID(the, "constructor"), XS_DONT_ENUM_FLAG, XS_DONT_ENUM_FLAG);
	fxQueueID(the, fxID(the, "ArrayBuffer"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	
	mxPush(mxObjectPrototype);
	fxNewObjectInstance(the);
	mxPushUndefined();
	fxQueueID(the, fxID(the, "buffer"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_GETTER_FLAG);
	mxPushInteger(0);
	fxQueueID(the, fxID(the, "byteLength"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_GETTER_FLAG);
	mxPushInteger(0);
	fxQueueID(the, fxID(the, "byteOffset"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_GETTER_FLAG);
	for (anIndex = 0, aDispatch = &gTypedArrayDispatches[0]; anIndex < mxTypedArrayDispatchCount - 1; anIndex++, aDispatch++) {
		fxNewHostFunction(the, fx_DataView_get, 1);
		aProperty = the->stack->value.reference->next->next->next;
		aProperty->flag = XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG;
		aProperty->kind = XS_INTEGER_KIND;
		aProperty->value.integer = anIndex;
		fxQueueID(the, fxID(the, aDispatch->getID), XS_DONT_ENUM_FLAG);
		fxNewHostFunction(the, fx_DataView_set, 2);
		aProperty = the->stack->value.reference->next->next->next;
		aProperty->flag = XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG;
		aProperty->kind = XS_INTEGER_KIND;
		aProperty->value.integer = anIndex;
		fxQueueID(the, fxID(the, aDispatch->setID), XS_DONT_ENUM_FLAG);
	}
	the->scratch = *the->stack;	
	fxNewHostConstructor(the, fx_DataView, 1);
	 *(--the->stack) = the->scratch;
	fxPutID(the, fxID(the, "constructor"), XS_DONT_ENUM_FLAG, XS_DONT_ENUM_FLAG);
	fxQueueID(the, fxID(the, "DataView"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	
	for (anIndex = 0, aDispatch = &gTypedArrayDispatches[0]; anIndex < mxTypedArrayDispatchCount; anIndex++, aDispatch++) {
		fxNewHostObject(the, C_NULL);
		the->stack->value.reference->next->value.host.data = aDispatch;
		mxPushUndefined();
		fxQueueID(the, fxID(the, "buffer"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
		mxPushInteger(0);
		fxQueueID(the, fxID(the, "byteLength"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
		mxPushInteger(0);
		fxQueueID(the, fxID(the, "byteOffset"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
		mxPushInteger(0);
		fxQueueID(the, the->lengthID, XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
		fxNewHostFunction(the, fx_TypedArray_peek, 1);
		fxQueueID(the, the->peekID, XS_DONT_ENUM_FLAG);
		fxNewHostFunction(the, fx_TypedArray_poke, 2);
		fxQueueID(the, the->pokeID, XS_DONT_ENUM_FLAG);
		fxNewHostFunction(the, fx_TypedArray_set, 2);
		fxQueueID(the, fxID(the, "set"), XS_DONT_ENUM_FLAG);
		fxNewHostFunction(the, fx_TypedArray_subarray, 0);
		fxQueueID(the, fxID(the, "subarray"), XS_DONT_ENUM_FLAG);
		the->scratch = *the->stack;	
		fxNewHostConstructor(the, fx_TypedArray, 1);
		 *(--the->stack) = the->scratch;
		fxPutID(the, fxID(the, "constructor"), XS_DONT_ENUM_FLAG, XS_DONT_ENUM_FLAG);
		mxPushInteger(aDispatch->size);
		fxQueueID(the, fxID(the, "BYTES_PER_ELEMENT"), XS_DONT_ENUM_FLAG);
		fxQueueID(the, fxID(the, aDispatch->constructorID), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	}
	
	the->stack++;
}

txSlot* fxNewChunkInstance(txMachine* the)
{
	txSlot* anInstance;
	txSlot* aProperty;

	anInstance = fxNewSlot(the);
	anInstance->flag = XS_VALUE_FLAG;
	anInstance->kind = XS_INSTANCE_KIND;
	anInstance->value.instance.garbage = C_NULL;
	anInstance->value.instance.prototype = C_NULL;
	if (the->stack->kind == XS_ALIAS_KIND)
		anInstance->ID = the->stack->value.alias;
	else
		anInstance->value.instance.prototype = the->stack->value.reference;
	the->stack->value.reference = anInstance;
	the->stack->kind = XS_REFERENCE_KIND;

	aProperty = anInstance->next = fxNewSlot(the);
	aProperty->ID = XS_NO_ID;
	aProperty->flag = XS_NO_FLAG;
	aProperty->kind = XS_HOST_KIND;
	aProperty->value.host.data = C_NULL;
	aProperty->value.host.variant.destructor = fx_Chunk_destructor;
	
	aProperty = aProperty->next = fxNewSlot(the);
	aProperty->ID = the->chunkID;
	aProperty->flag = XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG;
	aProperty->kind = XS_INTEGER_KIND;
	aProperty->value.integer = 0;
	
	return anInstance;
}

void fx_Chunk_destructor(void* theData)
{
	if (theData)
		c_free(theData);
}

void fx_Chunk(txMachine* the)
{
	txSlot* aChunk;
	txInteger aLength;
	txString aString;
	txU1* theData;
	txInteger theOffset;
	txInteger theLength;
	txSlot* aSlot;
	txU1* aData;
//@@
	if ((mxThis->kind == XS_REFERENCE_KIND) && ((mxThis->value.reference->flag & XS_SHARED_FLAG) == 0)) {
		aChunk = mxThis->value.reference;
        if (!mxIsChunk(aChunk)) {
            mxPush(mxChunkPrototype);
            aChunk = fxNewChunkInstance(the);
            *mxResult = *(the->stack++);
        }
        else
            *mxResult = *mxThis;
    }
	else
		*mxResult = *mxThis;
	aString = C_NULL;
	theData = C_NULL;
	theOffset = 0;
	if (mxArgc > 0) {
		aLength = -1;
		aSlot = mxArgv(0);
		if ((aSlot->kind == XS_INTEGER_KIND) || (aSlot->kind == XS_NUMBER_KIND))
			aLength = fxToInteger(the, aSlot);
		else if (aSlot->kind == XS_STRING_KIND)
			aString = fxToString(the, aSlot);
		else if (mxIsReference(aSlot)) {
			aSlot = fxGetInstance(the, aSlot);
			if (mxIsNumber(aSlot))
				aLength = fxToInteger(the, aSlot);
			else if (mxIsString(aSlot))
				aString = fxToString(the, aSlot);
			else if (mxIsChunk(aSlot)) {
				theData = (txU1 *)aSlot->next->value.host.data;
				aLength = aSlot->next->next->value.integer;
				if (mxArgc > 1) {
					theOffset = fxToInteger(the, mxArgv(1));
					aLength -= theOffset;
					if (mxArgc > 2) {
						theLength = fxToInteger(the, mxArgv(2));
						if (aLength > theLength)
							aLength = theLength;
					}
				}
			}
		}
	}
	else
		aLength = 0;
	
	if (aString) {
		fx_Chunk_aux(the, aString, &aData, &aLength);
		aChunk->next->value.host.data = aData;
		aChunk->next->value.host.variant.destructor = fx_Chunk_destructor;
		aChunk->next->next->value.integer = aLength;
	}
	else if (aLength >= 0) {
		if (aLength) {
			aData = (txU1 *)c_malloc(aLength);
			if (!aData)
				fxJump(the);
			if (theData)
				c_memcpy(aData, theData + theOffset, aLength);
			else
				c_memset(aData, 0, aLength);
		}
		else
			aData = C_NULL;
		aChunk->next->value.host.data = aData;
		aChunk->next->value.host.variant.destructor = fx_Chunk_destructor;
		aChunk->next->next->value.integer = aLength;
	}
	else
		mxDebug0(the, XS_RANGE_ERROR, "new Chunk: invalid parameters");
}

void fx_Chunk_aux(txMachine* the, txString theString, txU1** theData, txInteger* theLength)
{
	txInteger aLength;
	txU1* src;
	txInteger srcIndex;
	txU1* dst;
	txInteger dstIndex;
	txFlag aFlag;
	txU1 aByte;
	txU1 aBuffer[4];

	aLength = 0;
	src = (txU1*)theString;
	srcIndex = 0;
	dstIndex = 3;
	aFlag = 0;
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
			aLength += dstIndex;
			srcIndex = 0;
		}
		if (aFlag)
			break;
	}
	
	*theLength = aLength;
	if (!aLength) {
		*theData = C_NULL;
		return;
	}
	*theData = (txU1 *)c_malloc(aLength);
	if (!*theData)
		fxJump(the);
	
	src = (txU1*)theString;
	srcIndex = 0;
	dst = *theData;
	dstIndex = 3;
	aFlag = 0;
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

void fx_Chunk_get_length(txMachine* the)
{
	txSlot* aSlot;
	
	aSlot = fxGetInstance(the, mxThis);
	mxCheckChunk(aSlot);
	mxResult->value.integer = aSlot->next->next->value.integer;
	mxResult->kind = XS_INTEGER_KIND;
}

void fx_Chunk_set_length(txMachine* the)
{
	txSlot* aSlot;
	txInteger theLength;
	txU1* aData;
	txDestructor aDestructor;
	txInteger aLength;
	
	aSlot = fxGetOwnInstance(the, mxThis);
	mxCheckChunk(aSlot);
	if (mxArgc == 0)
		mxDebug0(the, XS_SYNTAX_ERROR, "Chunk.set length: no parameter");
	theLength = fxToInteger(the, mxArgv(0));
	if (theLength < 0)
		mxDebug1(the, XS_RANGE_ERROR, "Chunk.set length: not in range %ld", theLength);
	aSlot = aSlot->next;
	aData = (txU1 *)aSlot->value.host.data;
	aDestructor = aSlot->value.host.variant.destructor;
	aLength = aSlot->next->value.integer;
	if (aLength != theLength) {
		if (aDestructor == fx_Chunk_destructor) {
			if (theLength) {
				if (aData) {
					aData = (txU1 *)c_realloc(aData, theLength);
					if (!aData)
						mxDebug1(the, XS_RANGE_ERROR, "Chunk.set length: not enough memory %ld", theLength);
					if (aLength < theLength)
						c_memset(aData + aLength, 0, theLength - aLength);
				}
				else {
					aData = (txU1 *)c_malloc(theLength);
					if (!aData)
						mxDebug1(the, XS_RANGE_ERROR, "Chunk.set length: not enough memory %ld", theLength);
					c_memset(aData, 0, theLength);
				}
			}
			else if (aData) {
				c_free(aData);
				aData = C_NULL;
			}
			aSlot->value.host.data = aData;
		}
		aSlot->next->value.integer = theLength;
	}
}

void fx_Chunk_append(txMachine* the)
{
	txSlot* aSlot;
	txU1* aData;
	txDestructor aDestructor;
	txInteger aLength;
	txSlot* bSlot;
	txU1* bData;
	txInteger bLength;
	txInteger cLength;
	txU1* cData;
	
	aSlot = fxGetOwnInstance(the, mxThis);
	mxCheckChunk(aSlot);
	if (mxArgc < 1)
		mxDebug0(the, XS_SYNTAX_ERROR, "Chunk.append: not enough parameters");
	bSlot = fxGetInstance(the, mxArgv(0));
	if (!mxIsChunk(bSlot))
		mxDebug0(the, XS_TYPE_ERROR, "the parameter is no Chunk");
	aSlot = aSlot->next;
	aData = (txU1 *)aSlot->value.host.data;
	aDestructor = aSlot->value.host.variant.destructor;
	aLength = aSlot->next->value.integer;
	bSlot = bSlot->next;
	bData = (txU1 *)bSlot->value.host.data;
	bLength = bSlot->next->value.integer;
	cLength = aLength + bLength;
	if (cLength) {
		cData = (txU1 *)c_malloc(cLength);
		if (!cData)
			fxJump(the);
		if (aData && aLength)
			c_memcpy(cData, aData, aLength);
		if (bData && bLength)
			c_memcpy(cData + aLength, bData, bLength);
	}
	else
		cData = C_NULL;
	if (aDestructor)
		(*aDestructor)(aData);
	aSlot->value.host.data = cData;
	aSlot->value.host.variant.destructor = fx_Chunk_destructor;
	aSlot->next->value.integer = cLength;
}

void fx_Chunk_free(txMachine* the)
{
	txSlot* aSlot;
	void* aData;
	txDestructor aDestructor;
	
	aSlot = fxGetOwnInstance(the, mxThis);
	mxCheckChunk(aSlot);
	aSlot = aSlot->next;
	aData = aSlot->value.host.data;
	aDestructor = aSlot->value.host.variant.destructor;
	if (aDestructor)
		(*aDestructor)(aData);
	aSlot->value.host.data = C_NULL;
	aSlot->next->value.integer = 0;
}

void fx_Chunk_peek(txMachine* the)
{
	txSlot* aSlot;
	txU1* aData;
	txInteger aLength;
	txInteger anIndex;
	
	aSlot = fxGetOwnInstance(the, mxThis);
	mxCheckChunk(aSlot);
	if (mxArgc < 1)
		mxDebug0(the, XS_SYNTAX_ERROR, "Chunk.peek: not enough parameters");
	aSlot = aSlot->next;
	aData = (txU1*)aSlot->value.host.data;
	aLength = aSlot->next->value.integer;
	anIndex = fxToInteger(the, mxArgv(0));
	if ((anIndex < 0) || (aLength <= anIndex))
		mxDebug1(the, XS_RANGE_ERROR, "Chunk.peek: index not in range %ld", anIndex);
	mxResult->value.integer = aData[anIndex];
	mxResult->kind = XS_INTEGER_KIND;
}

void fx_Chunk_poke(txMachine* the)
{
	txSlot* aSlot;
	txU1* aData;
	txInteger aLength;
	txInteger anIndex;
	txInteger aValue;
	
	aSlot = fxGetOwnInstance(the, mxThis);
	mxCheckChunk(aSlot);
	if (mxArgc < 2)
		mxDebug0(the, XS_SYNTAX_ERROR, "Chunk.poke: not enough parameters");
	aSlot = aSlot->next;
	aData = (txU1*)aSlot->value.host.data;
	aLength = aSlot->next->value.integer;
	anIndex = fxToInteger(the, mxArgv(0));
	if ((anIndex < 0) || (aLength <= anIndex))
		mxDebug1(the, XS_RANGE_ERROR, "Chunk.poke: index not in range %ld", anIndex);
	aValue = fxToInteger(the, mxArgv(1));
	if ((aValue < 0) || (256 <= aValue))
		mxDebug1(the, XS_RANGE_ERROR, "Chunk.poke: value not in range %ld", aValue);
	aData[anIndex] = (txU1)aValue;
}

void fx_Chunk_slice(txMachine* the)
{
	txSlot* aSlot;
	txU1* aData;
	txInteger aLength;
	txSlot* bSlot;
	txInteger aStart, aStop;
	txU1* bData;
	
	aSlot = fxGetInstance(the, mxThis);
	mxCheckChunk(aSlot);
	mxZeroSlot(--the->stack);
	if (aSlot->ID >= 0) {
		the->stack->kind = XS_ALIAS_KIND;
		the->stack->value.alias = aSlot->ID;
	}
	else {
		the->stack->kind = XS_REFERENCE_KIND;
		the->stack->value.reference = aSlot->value.instance.prototype;
	}
	bSlot = fxNewChunkInstance(the);
 	*mxResult = *(the->stack++);           
	aSlot = aSlot->next;
	aData = (txU1 *)aSlot->value.host.data;
	aLength = aSlot->next->value.integer;
	bSlot = bSlot->next;
	aStart = 0;
	aStop = aLength;
	if ((mxArgc > 0) && (mxArgv(0)->kind != XS_UNDEFINED_KIND)) {
		fxToInteger(the, mxArgv(0));
		aStart = mxArgv(0)->value.integer;
		if (aStart < 0) {
			aStart = aLength + aStart;
			if (aStart < 0)
				aStart = 0;
		}
		else if (aStart > aLength)
			aStart = aLength;
	}
	if ((mxArgc > 1) && (mxArgv(1)->kind != XS_UNDEFINED_KIND)) {
		fxToInteger(the, mxArgv(1));
		aStop = mxArgv(1)->value.integer;
		if (aStop < 0) {
			aStop = aLength + aStop;
			if (aStop < 0)
				aStop = 0;
		}
		else if (aStop > aLength)
			aStop = aLength;
	}
	if (aStart < aStop) {
		aLength = aStop - aStart;
		bData = (txU1 *)c_malloc(aLength);
		if (!bData) 
			fxJump(the);
		c_memcpy(bData, aData + aStart, aLength);
		bSlot->value.host.data = bData;
		bSlot->next->value.integer = aLength;
	}
}

void fx_Chunk_toString(txMachine* the)
{
	static char base64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	txSlot* aSlot;
	txInteger aLength;
	txU1* src;
	txU1* dst;
	txU1 byte0, byte1, byte2;
	
	aSlot = fxGetInstance(the, mxThis);
	mxCheckChunk(aSlot);
	aSlot = aSlot->next;
	src = (txU1 *)aSlot->value.host.data;
	aSlot = aSlot->next;
	aLength = aSlot->value.integer;
	dst = (txU1 *)fxNewChunk(the, (((aLength + 2) / 3) * 4) + 1);
	mxResult->value.string = (txString)dst;
	mxResult->kind = XS_STRING_KIND;
	while (aLength > 2) {
		byte0 = *src++;
		byte1 = *src++;
		byte2 = *src++;
		*dst++ = base64[((byte0 & 0xFC) >> 2)];
		*dst++ = base64[(((byte0 & 0x03) << 4) | ((byte1 & 0xF0) >> 4))];
		*dst++ = base64[(((byte1 & 0x0F) << 2) | ((byte2 & 0xC0) >> 6))];
		*dst++ = base64[(byte2 & 0x3F)];
		aLength -= 3;
	}
	if (aLength == 2) {
		byte0 = *src++;
		byte1 = *src++;
		*dst++ = base64[((byte0 & 0xFC) >> 2)];
		*dst++ = base64[(((byte0 & 0x03) << 4) | ((byte1 & 0xF0) >> 4))];
		*dst++ = base64[((byte1 & 0x0F) << 2)];
		*dst++ = '=';
	}
	else if (aLength == 1) {
		byte0 = *src++;
		*dst++ = base64[((byte0 & 0xFC) >> 2)];
		*dst++ = base64[((byte0 & 0x03) << 4)];
		*dst++ = '=';
		*dst++ = '=';
	}
	*dst = 0;
}

void fx_ArrayBuffer(txMachine* the)
{
	txInteger aLength;
	txU1* aData;
	txSlot* anInstance;
	if (mxArgc < 1)
		mxDebug0(the, XS_SYNTAX_ERROR, "DataView: not enough parameters");
	aLength = fxToInteger(the, mxArgv(0));
	aData = (txU1 *)c_malloc(aLength);
	if (!aData)
		fxJump(the);
	c_memset(aData, 0, aLength);
	anInstance = fxGetOwnInstance(the, mxThis);
	anInstance->next->value.host.data = aData;
	anInstance->next->next->value.integer = aLength;
}

void fx_DataView(txMachine* the)
{
	txSlot* aChunk;
	txInteger aLimit, anOffset, aLength;
	if (mxArgc < 1)
		mxDebug0(the, XS_SYNTAX_ERROR, "DataView: not enough parameters");
	aChunk = fxGetInstance(the, mxArgv(0));
	mxCheckChunk(aChunk);
	aLimit = aChunk->next->next->value.integer;
	if (mxArgc > 1)
		anOffset = fxToInteger(the, mxArgv(1));
	else
		anOffset = 0;
	if (mxArgc > 2)
		aLength = fxToInteger(the, mxArgv(2));
	else
		aLength = aLimit - anOffset;
	if ((anOffset < 0) || (aLimit <= anOffset))
		mxDebug1(the, XS_RANGE_ERROR, "DataView: offset not in range %ld", anOffset);
	if (aLimit < (anOffset + aLength))
		mxDebug0(the, XS_RANGE_ERROR, "DataView: buffer overflow");
	mxPush(*mxThis);
	mxPush(*mxArgv(0));
	fxQueueID(the, fxID(the, "buffer"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	mxPushInteger(aLength);
	fxQueueID(the, fxID(the, "byteLength"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	mxPushInteger(anOffset);
	fxQueueID(the, fxID(the, "byteOffset"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	the->stack++;
}

void fx_DataView_get(txMachine* the)
{
	txSlot* anInstance = fxGetInstance(the, mxThis);
	txSlot* aChunk = fxGetInstance(the, anInstance->next);
	txU1* aData = (txU1*)aChunk->next->value.host.data;
	txTypedArrayDispatch *aDispatch = gTypedArrayDispatches;
	txInteger anOffset, aLength;
	if (mxArgc < 1)
		mxDebug0(the, XS_SYNTAX_ERROR, "DataView.prototype.set: not enough parameters");
	aDispatch += mxFunction->value.reference->next->next->next->value.integer;
	anOffset = fxToInteger(the, mxArgv(0));
	aLength = anInstance->next->next->value.integer - aDispatch->size;
	if ((anOffset < 0) || (aLength < anOffset))
		mxDebug1(the, XS_RANGE_ERROR, "DataView.prototype.get: offset not in range %ld", anOffset);
	aLength = aChunk->next->next->value.integer - aDispatch->size;
	anOffset += anInstance->next->next->next->value.integer;
	if (aLength < anOffset)
		mxDebug0(the, XS_RANGE_ERROR, "DataView.prototype.get: buffer overflow");
	aDispatch->getter(the, aData + anOffset, mxResult);
}

void fx_DataView_set(txMachine* the)
{
	txSlot* anInstance = fxGetInstance(the, mxThis);
	txSlot* aChunk = fxGetInstance(the, anInstance->next);
	txU1* aData = (txU1*)aChunk->next->value.host.data;
	txTypedArrayDispatch *aDispatch = gTypedArrayDispatches;
	txInteger anOffset, aLength;
	if (mxArgc < 2)
		mxDebug0(the, XS_SYNTAX_ERROR, "DataView.prototype.get: not enough parameters");
	aDispatch += mxFunction->value.reference->next->next->next->value.integer;
	anOffset = fxToInteger(the, mxArgv(0));
	aLength = anInstance->next->next->value.integer - aDispatch->size;
	if ((anOffset < 0) || (aLength < anOffset))
		mxDebug1(the, XS_RANGE_ERROR, "DataView.prototype.get: offset not in range %ld", anOffset);
	aLength = aChunk->next->next->value.integer - aDispatch->size;
	anOffset += anInstance->next->next->next->value.integer;
	if (aLength < anOffset)
		mxDebug0(the, XS_RANGE_ERROR, "DataView.prototype.get: buffer overflow");
	aDispatch->setter(the, aData + anOffset, mxArgv(1));
}

void fx_TypedArray(txMachine* the)
{
	txSlot* anInstance;
	txTypedArrayDispatch *aDispatch;
	txInteger aLength = 0;
	txInteger anOffset = 0;
	txSlot* aChunk = C_NULL;
	txSlot* aSequence = C_NULL;
	txSlot* aSlot;
	txInteger anIndex;
	anInstance = fxGetOwnInstance(the, mxThis);
	aSlot = anInstance->value.instance.prototype;
	aDispatch = anInstance->next->value.host.data = aSlot->next->value.host.data;
	if (mxArgc > 0) {
		aSlot = mxArgv(0);
		if ((aSlot->kind == XS_INTEGER_KIND) || (aSlot->kind == XS_NUMBER_KIND)) {
			aLength = fxToInteger(the, aSlot);
		}
		else if (mxIsReference(aSlot)) {
			anInstance = fxGetInstance(the, aSlot);
			if (mxIsNumber(anInstance)) {
				aLength = fxToInteger(the, anInstance);
			}
			else if (mxIsChunk(anInstance)) {
				aChunk = anInstance;
				aLength = aChunk->next->next->value.integer / aDispatch->size;
				if (mxArgc > 1) {
					anOffset = fxToInteger(the, mxArgv(1));
					if (mxArgc > 2) {
						aLength = fxToInteger(the, mxArgv(2));
					}
				}
				if (anOffset % aDispatch->size)
					mxDebug0(the, XS_RANGE_ERROR, "TypedArray: byteOffset is no multiple of element size");
				if ((anOffset + (aLength * aDispatch->size)) > aChunk->next->next->value.integer)
					mxDebug0(the, XS_RANGE_ERROR, "TypedArray: buffer overflow");
			}
			else {
				aSequence = aSlot;
				mxPush(*aSlot);
				fxGetID(the, the->lengthID);
				aLength = fxToInteger(the, the->stack);
				the->stack++;
			}
		}
	}
	mxPush(*mxThis);
	if (aChunk) {
		mxPush(*aSlot);
	}
	else {
		mxPushInteger(aLength * aDispatch->size);
		mxPushInteger(1);
		mxPush(mxGlobal);
		fxNewID(the, fxID(the, "ArrayBuffer"));
	}
	fxQueueID(the, fxID(the, "buffer"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	mxPushInteger(aLength * aDispatch->size);
	fxQueueID(the, fxID(the, "byteLength"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	mxPushInteger(anOffset);
	fxQueueID(the, fxID(the, "byteOffset"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	mxPushInteger(aLength);
	fxQueueID(the, the->lengthID, XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	if (aSequence) {
		for (anIndex = 0; anIndex < aLength; anIndex++) {
			mxPush(*aSequence);
			mxPushInteger(anIndex);
			fxGetAt(the);
			mxPush(*mxThis);
			mxPushInteger(anIndex);
			fxSetAt(the);
			the->stack--;
		}
	}
}

void fx_TypedArray_peek(txMachine* the)
{
	txSlot* anInstance = fxGetInstance(the, mxThis);
	txTypedArrayDispatch *aDispatch = anInstance->next->value.host.data;
	txSlot* aChunk = fxGetInstance(the, anInstance->next->next);
	txU1* aData = (txU1*)aChunk->next->value.host.data;
	txInteger anOffset = anInstance->next->next->next->next->value.integer;
	txInteger aLength = anInstance->next->next->next->next->next->value.integer;
	txInteger anIndex = fxToInteger(the, mxArgv(0));
	if ((anIndex < 0) || (aLength <= anIndex))
		mxDebug1(the, XS_RANGE_ERROR, "TypedArray.prototype.peek: index not in range %ld", anIndex);
	aLength = aChunk->next->next->value.integer - aDispatch->size;
	anOffset += anIndex * aDispatch->size;
	if (aLength < anOffset)
		mxDebug0(the, XS_RANGE_ERROR, "TypedArray.prototype.peek: buffer overflow");
	aDispatch->getter(the, aData + anOffset, mxResult);
}

void fx_TypedArray_poke(txMachine* the)
{
	txSlot* anInstance = fxGetOwnInstance(the, mxThis);
	txTypedArrayDispatch *aDispatch = anInstance->next->value.host.data;
	txSlot* aChunk = fxGetInstance(the, anInstance->next->next);
	txU1* aData = (txU1*)aChunk->next->value.host.data;
	txInteger anOffset = anInstance->next->next->next->next->value.integer;
	txInteger aLength = anInstance->next->next->next->next->next->value.integer;
	txInteger anIndex = fxToInteger(the, mxArgv(0));
	if ((anIndex < 0) || (aLength <= anIndex))
		mxDebug1(the, XS_RANGE_ERROR, "TypedArray.prototype.poke: index not in range %ld", anIndex);
	aLength = aChunk->next->next->value.integer - aDispatch->size;
	anOffset += anIndex * aDispatch->size;
	if (aLength < anOffset)
		mxDebug0(the, XS_RANGE_ERROR, "TypedArray.prototype.poke: buffer overflow");
	aDispatch->setter(the, aData + anOffset, mxArgv(1));
}

void fx_TypedArray_set(txMachine* the)
{
	txSlot* aSlot = fxGetOwnInstance(the, mxThis);
	txTypedArrayDispatch *aDispatch = aSlot->next->value.host.data;
	txInteger aLength = aSlot->next->next->next->next->next->value.integer;
	txSlot* bSlot;
	txSlot* aChunk;
	txU1* aData;
	txU1* bData;
	txInteger aStart, aStop, anOffset, aLimit, bLimit;
	if (mxArgc < 1)
		mxDebug0(the, XS_SYNTAX_ERROR, "TypedArray.prototype.set: not enough parameters");
	bSlot = fxGetInstance(the, mxArgv(0));
	if (mxIsHost(bSlot) && (bSlot->next->value.host.data == aDispatch)) {
		aStop = bSlot->next->next->next->next->next->value.integer;
		if (mxArgc > 1)
			aStart = fxToInteger(the, mxArgv(1));
		else
			aStart = 0;
		aStop += aStart;
		if ((aStart < 0) || (aLength <= aStart) || (aLength < aStop))
			mxDebug1(the, XS_RANGE_ERROR, "TypedArray.prototype.set: offset not in range %ld", aStart);
		
		aStart *= aDispatch->size;
		aStart += aSlot->next->next->next->next->value.integer;
		aLength = bSlot->next->next->next->value.integer;
		anOffset = bSlot->next->next->next->next->value.integer;
		aChunk = fxGetOwnInstance(the, aSlot->next->next);
		aData = (txU1*)aChunk->next->value.host.data;
		aLimit = aChunk->next->next->value.integer - aLength;
		if (aLimit < aStart)
			mxDebug0(the, XS_RANGE_ERROR, "TypedArray.prototype.set: buffer overflow");
		aChunk = fxGetInstance(the, bSlot->next->next);
		bData = (txU1*)aChunk->next->value.host.data;
		bLimit = aChunk->next->next->value.integer - aLength;
		if (bLimit < anOffset)
			mxDebug0(the, XS_RANGE_ERROR, "TypedArray.prototype.set: buffer overflow");
			
		c_memmove(aData + aStart, bData + anOffset, aLength);
	}
	else {
		txSlot* aSequence = mxArgv(0);
		mxPush(*aSequence);
		fxGetID(the, the->lengthID);
		aStop = fxToInteger(the, the->stack);
		the->stack++;
		if (mxArgc > 1)
			aStart = fxToInteger(the, mxArgv(1));
		else
			aStart = 0;
		aStop += aStart;
		if ((aStart < 0) || (aLength <= aStart) || (aLength < aStop))
			mxDebug1(the, XS_RANGE_ERROR, "TypedArray.prototype.set: offset not in range %ld", aStart);
		for (anOffset = aStart; anOffset < aStop; anOffset++) {
			mxPush(*aSequence);
			mxPushInteger(anOffset - aStart);
			fxGetAt(the);
			mxPush(*mxThis);
			mxPushInteger(anOffset);
			fxSetAt(the);
			the->stack--;
		}
	}
}

void fx_TypedArray_subarray(txMachine* the)
{
	txSlot* anInstance = fxGetInstance(the, mxThis);
	txTypedArrayDispatch *aDispatch = anInstance->next->value.host.data;
	txSlot* aReference = anInstance->next->next;
	txInteger aLength = anInstance->next->next->next->next->next->value.integer;
	txInteger aStart = 0;
	txInteger aStop = aLength;
	if ((mxArgc > 0) && (mxArgv(0)->kind != XS_UNDEFINED_KIND)) {
		fxToInteger(the, mxArgv(0));
		aStart = mxArgv(0)->value.integer;
		if (aStart < 0) {
			aStart = aLength + aStart;
			if (aStart < 0)
				aStart = 0;
		}
		else if (aStart > aLength)
			aStart = aLength;
	}
	if ((mxArgc > 1) && (mxArgv(1)->kind != XS_UNDEFINED_KIND)) {
		fxToInteger(the, mxArgv(1));
		aStop = mxArgv(1)->value.integer;
		if (aStop < 0) {
			aStop = aLength + aStop;
			if (aStop < 0)
				aStop = 0;
		}
		else if (aStop > aLength)
			aStop = aLength;
	}
	if (aStart >= aStop) 
		mxDebug0(the, XS_RANGE_ERROR, "TypedArray.prototype.subarray: begin >= end");
	mxZeroSlot(--the->stack);
	the->stack->kind = XS_REFERENCE_KIND;
	the->stack->value.reference = anInstance->value.instance.prototype;
	fxNewHostInstance(the);
	the->stack->value.reference->next->value.host.data = aDispatch;
	mxPush(*aReference);
	fxQueueID(the, fxID(the, "buffer"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	mxPushInteger((aStop - aStart) * aDispatch->size);
	fxQueueID(the, fxID(the, "byteLength"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	mxPushInteger(aStart * aDispatch->size);
	fxQueueID(the, fxID(the, "byteOffset"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	mxPushInteger(aStop - aStart);
	fxQueueID(the, the->lengthID, XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	*mxResult = *(the->stack++);
}

void fxFloat32Getter(txMachine* the, void* theData, txSlot* theSlot)
{
	fxNumber(the, theSlot, *((float*)theData));
}

void fxFloat32Setter(txMachine* the, void* theData, txSlot* theSlot)
{
	*((float*)theData) = (float)fxToNumber(the, theSlot);
}

void fxFloat64Getter(txMachine* the, void* theData, txSlot* theSlot)
{
	fxNumber(the, theSlot, *((double*)theData));
}

void fxFloat64Setter(txMachine* the, void* theData, txSlot* theSlot)
{
	*((double*)theData) = (double)fxToNumber(the, theSlot);
}

void fxInt8Getter(txMachine* the, void* theData, txSlot* theSlot)
{
	fxInteger(the, theSlot, *((txS1*)theData));
}

void fxInt8Setter(txMachine* the, void* theData, txSlot* theSlot)
{
	*((txS1*)theData) = (txS1)fxToInteger(the, theSlot);
}

void fxInt16Getter(txMachine* the, void* theData, txSlot* theSlot)
{
	fxInteger(the, theSlot, *((txS2*)theData));
}

void fxInt16Setter(txMachine* the, void* theData, txSlot* theSlot)
{
	*((txS2*)theData) = (txS2)fxToInteger(the, theSlot);
}

void fxInt32Getter(txMachine* the, void* theData, txSlot* theSlot)
{
	fxInteger(the, theSlot, *((txS4*)theData));
}

void fxInt32Setter(txMachine* the, void* theData, txSlot* theSlot)
{
	*((txS4*)theData) = (txS4)fxToInteger(the, theSlot);
}

void fxUint8Getter(txMachine* the, void* theData, txSlot* theSlot)
{
	fxUnsigned(the, theSlot, *((txU1*)theData));
}

void fxUint8Setter(txMachine* the, void* theData, txSlot* theSlot)
{
	*((txU1*)theData) = (txU1)fxToUnsigned(the, theSlot);
}

void fxUint16Getter(txMachine* the, void* theData, txSlot* theSlot)
{
	fxUnsigned(the, theSlot, *((txU2*)theData));
}

void fxUint16Setter(txMachine* the, void* theData, txSlot* theSlot)
{
	*((txU2*)theData) = (txU2)fxToUnsigned(the, theSlot);
}

void fxUint32Getter(txMachine* the, void* theData, txSlot* theSlot)
{
	fxUnsigned(the, theSlot, *((txU4*)theData));
}

void fxUint32Setter(txMachine* the, void* theData, txSlot* theSlot)
{
	*((txU4*)theData) = (txU4)fxToUnsigned(the, theSlot);
}

void fxUint8ClampedSetter(txMachine* the, void* theData, txSlot* theSlot)
{
	txInteger aValue = fxToInteger(the, theSlot);
	if (aValue < 0) aValue = 0;
	else if (aValue > 255) aValue = 255;
	*((txU1*)theData) = (txU1)aValue;
}


