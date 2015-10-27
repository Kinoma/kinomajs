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
#include "xs6All.h"

#define mxFunctionReferenceTypeDispatch(REFERENCE) ((REFERENCE)->value.reference->next->next->next->next->next->next->next)

static txSlot* fxArgToInstance(txMachine* the, txInteger i);
static txBoolean fxCheckLength(txMachine* the, txSlot* slot, txInteger* index);

static txSlot* fxCheckArrayBufferInstance(txMachine* the, txSlot* slot);
static void fx_ArrayBuffer(txMachine* the);
static void fx_ArrayBuffer_fromString(txMachine* the);
static void fx_ArrayBuffer_isView(txMachine* the);
static void fx_ArrayBuffer_prototype_get_byteLength(txMachine* the);
static void fx_ArrayBuffer_prototype_concat(txMachine* the);
static void fx_ArrayBuffer_prototype_slice(txMachine* the);

static txSlot* fxCheckDataViewInstance(txMachine* the, txSlot* slot);
static void fx_DataView(txMachine* the);
static void fx_DataView_prototype_buffer_get(txMachine* the);
static void fx_DataView_prototype_byteLength_get(txMachine* the);
static void fx_DataView_prototype_byteOffset_get(txMachine* the);
static void fx_DataView_prototype_get(txMachine* the);
static void fx_DataView_prototype_set(txMachine* the);

static void fxCallTypedArrayItem(txMachine* the, txSlot* function, txSlot* dispatch, txSlot* view, txSlot* data, txInteger index, txSlot* item);
static txSlot* fxCheckTypedArrayInstance(txMachine* the, txSlot* slot);
static int fxCompareTypedArrayItem(txMachine* the, txSlot* function, txSlot* dispatch, txSlot* view, txSlot* data, txInteger index);
static void fxReduceTypedArrayItem(txMachine* the, txSlot* function, txSlot* dispatch, txSlot* view, txSlot* data, txInteger index);
static void fx_TypedArray(txMachine* the);
static void fx_TypedArray_from(txMachine* the);
static void fx_TypedArray_from_object(txMachine* the, txSlot* instance, txSlot* function, txSlot* _this);
static void fx_TypedArray_of(txMachine* the);
static void fx_TypedArray_prototype_buffer_get(txMachine* the);
static void fx_TypedArray_prototype_byteLength_get(txMachine* the);
static void fx_TypedArray_prototype_byteOffset_get(txMachine* the);
static void fx_TypedArray_prototype_copyWithin(txMachine* the);
static void fx_TypedArray_prototype_entries(txMachine* the);
static void fx_TypedArray_prototype_entries_next(txMachine* the);
static void fx_TypedArray_prototype_every(txMachine* the);
static void fx_TypedArray_prototype_fill(txMachine* the);
static void fx_TypedArray_prototype_filter(txMachine* the);
static void fx_TypedArray_prototype_find(txMachine* the);
static void fx_TypedArray_prototype_findIndex(txMachine* the);
static void fx_TypedArray_prototype_forEach(txMachine* the);
static void fx_TypedArray_prototype_indexOf(txMachine* the);
static void fx_TypedArray_prototype_join(txMachine* the);
static void fx_TypedArray_prototype_keys(txMachine* the);
static void fx_TypedArray_prototype_keys_next(txMachine* the);
static void fx_TypedArray_prototype_lastIndexOf(txMachine* the);
static void fx_TypedArray_prototype_length_get(txMachine* the);
static void fx_TypedArray_prototype_map(txMachine* the);
static void fx_TypedArray_prototype_reduce(txMachine* the);
static void fx_TypedArray_prototype_reduceRight(txMachine* the);
static void fx_TypedArray_prototype_reverse(txMachine* the);
static void fx_TypedArray_prototype_set(txMachine* the);
static void fx_TypedArray_prototype_slice(txMachine* the);
static void fx_TypedArray_prototype_some(txMachine* the);
static void fx_TypedArray_prototype_sort(txMachine* the);
static void fx_TypedArray_prototype_subarray(txMachine* the);
static void fx_TypedArray_prototype_toStringTag_get(txMachine* the);
static void fx_TypedArray_prototype_values(txMachine* the);
static void fx_TypedArray_prototype_values_next(txMachine* the);

static int fxFloat32Compare(const void* p, const void* q);
static void fxFloat32Getter(txMachine* the, txSlot* data, txInteger offset, txSlot* slot, int endian);
static void fxFloat32Setter(txMachine* the, txSlot* data, txInteger offset, txSlot* slot, int endian);
static int fxFloat64Compare(const void* p, const void* q);
static void fxFloat64Getter(txMachine* the, txSlot* data, txInteger offset, txSlot* slot, int endian);
static void fxFloat64Setter(txMachine* the, txSlot* data, txInteger offset, txSlot* slot, int endian);
static int fxInt8Compare(const void* p, const void* q);
static void fxInt8Getter(txMachine* the, txSlot* data, txInteger offset, txSlot* slot, int endian);
static void fxInt8Setter(txMachine* the, txSlot* data, txInteger offset, txSlot* slot, int endian);
static int fxInt16Compare(const void* p, const void* q);
static void fxInt16Getter(txMachine* the, txSlot* data, txInteger offset, txSlot* slot, int endian);
static void fxInt16Setter(txMachine* the, txSlot* data, txInteger offset, txSlot* slot, int endian);
static int fxInt32Compare(const void* p, const void* q);
static void fxInt32Getter(txMachine* the, txSlot* data, txInteger offset, txSlot* slot, int endian);
static void fxInt32Setter(txMachine* the, txSlot* data, txInteger offset, txSlot* slot, int endian);
static int fxUint8Compare(const void* p, const void* q);
static void fxUint8Getter(txMachine* the, txSlot* data, txInteger offset, txSlot* slot, int endian);
static void fxUint8Setter(txMachine* the, txSlot* data, txInteger offset, txSlot* slot, int endian);
static int fxUint16Compare(const void* p, const void* q);
static void fxUint16Getter(txMachine* the, txSlot* data, txInteger offset, txSlot* slot, int endian);
static void fxUint16Setter(txMachine* the, txSlot* data, txInteger offset, txSlot* slot, int endian);
static int fxUint32Compare(const void* p, const void* q);
static void fxUint32Getter(txMachine* the, txSlot* data, txInteger offset, txSlot* slot, int endian);
static void fxUint32Setter(txMachine* the, txSlot* data, txInteger offset, txSlot* slot, int endian);
static void fxUint8ClampedSetter(txMachine* the, txSlot* data, txInteger offset, txSlot* slot, int endian);

#define mxTypeDispatchCount 9
const txTypeDispatch gTypeDispatches[mxTypeDispatchCount] = {
	{ 4, fxFloat32Getter, fxFloat32Setter, fxFloat32Compare, _getFloat32, _setFloat32, _Float32Array },
	{ 8, fxFloat64Getter, fxFloat64Setter, fxFloat64Compare, _getFloat64, _setFloat64, _Float64Array },
	{ 1, fxInt8Getter, fxInt8Setter, fxInt8Compare, _getInt8, _setInt8, _Int8Array },
	{ 2, fxInt16Getter, fxInt16Setter, fxInt16Compare, _getInt16, _setInt16, _Int16Array },
	{ 4, fxInt32Getter, fxInt32Setter, fxInt32Compare, _getInt32, _setInt32, _Int32Array },
	{ 1, fxUint8Getter, fxUint8Setter, fxUint8Compare, _getUint8, _setUint8, _Uint8Array },
	{ 2, fxUint16Getter, fxUint16Setter, fxUint16Compare, _getUint16, _setUint16, _Uint16Array },
	{ 4, fxUint32Getter, fxUint32Setter, fxUint32Compare, _getUint32, _setUint32, _Uint32Array },
	{ 1, fxUint8Getter, fxUint8ClampedSetter, fxUint8Compare, _getUint8Clamped, _setUint8Clamped, _Uint8ClampedArray }
};

enum {
	EndianNative = 0,
	EndianLittle = 1,
	EndianBig = 2
};

void fxArrayBuffer(txMachine* the, txSlot* slot, void* data, txInteger byteLength)
{
	txSlot* instance;
	txSlot* arrayBuffer;
	if (byteLength < 0)
		mxRangeError("invalid byteLength %ld", byteLength);
	mxPush(mxArrayBufferPrototype);
	instance = fxNewArrayBufferInstance(the);
	arrayBuffer = instance->next;
	if (byteLength) {
		arrayBuffer->value.arrayBuffer.address = fxNewChunk(the, byteLength);
		arrayBuffer->value.arrayBuffer.length = byteLength;
		if (data != NULL)
			c_memcpy(arrayBuffer->value.arrayBuffer.address, data, byteLength);
	}
	mxPullSlot(slot);
}

void fxGetArrayBufferData(txMachine* the, txSlot* slot, txInteger byteOffset, void* data, txInteger byteLength)
{
	txSlot* instance = fxCheckArrayBufferInstance(the, slot);
	txSlot* arrayBuffer = instance->next;
	txInteger length = arrayBuffer->value.arrayBuffer.length;
	if ((byteOffset < 0) || (length < byteOffset))
		mxRangeError("out of range byteOffset %ld", byteOffset);
	if ((byteLength < 0) || (length < (byteOffset + byteLength)))
		mxRangeError("out of range byteLength %ld", byteLength);
	c_memcpy(data, arrayBuffer->value.arrayBuffer.address + byteOffset, byteLength);
}

txInteger fxGetArrayBufferLength(txMachine* the, txSlot* slot)
{
	txSlot* instance = fxCheckArrayBufferInstance(the, slot);
	txSlot* arrayBuffer = instance->next;
	return arrayBuffer->value.arrayBuffer.length;
}

void fxSetArrayBufferData(txMachine* the, txSlot* slot, txInteger byteOffset, void* data, txInteger byteLength)
{
	txSlot* instance = fxCheckArrayBufferInstance(the, slot);
	txSlot* arrayBuffer = instance->next;
	txInteger length = arrayBuffer->value.arrayBuffer.length;
	if ((byteOffset < 0) || (length < byteOffset))
		mxRangeError("out of range byteOffset %ld", byteOffset);
	if ((byteLength < 0) || (length < (byteOffset + byteLength)))
		mxRangeError("out of range byteLength %ld", byteLength);
	c_memcpy(arrayBuffer->value.arrayBuffer.address + byteOffset, data, byteLength);
}

void fxSetArrayBufferLength(txMachine* the, txSlot* slot, txInteger target)
{
	txSlot* instance = fxCheckArrayBufferInstance(the, slot);
	txSlot* arrayBuffer = instance->next;
	txIndex length = arrayBuffer->value.arrayBuffer.length;
	txByte* address = arrayBuffer->value.arrayBuffer.address;
	if (length != target) {
		if (address)
			address = (txByte*)fxRenewChunk(the, address, target);
		if (address) {
			if (length < target)
				c_memset(address + length, 0, target - length);
		}
		else {
			address = (txByte*)fxNewChunk(the, target);
			if (length < target) {
				c_memcpy(address, arrayBuffer->value.arrayBuffer.address, length);
				c_memset(address + length, 0, target - length);
			}
			else
				c_memcpy(address, arrayBuffer->value.arrayBuffer.address, target);
		}
		arrayBuffer->value.arrayBuffer.length = target;
		arrayBuffer->value.arrayBuffer.address = address;
	}
}

void* fxToArrayBuffer(txMachine* the, txSlot* slot)
{
	txSlot* instance = fxCheckArrayBufferInstance(the, slot);
	txSlot* arrayBuffer = instance->next;
	return arrayBuffer->value.arrayBuffer.address;
}

void fxBuildDataView(txMachine* the)
{
    static const txHostFunctionBuilder gx_ArrayBuffer_prototype_builders[] = {
		{ fx_ArrayBuffer_prototype_concat, 1, _concat },
		{ fx_ArrayBuffer_prototype_slice, 2, _slice },
		{ C_NULL, 0, 0 },
    };
    static const txHostFunctionBuilder gx_TypedArray_prototype_builders[] = {
		{ fx_TypedArray_prototype_copyWithin, 2, _copyWithin },
		{ fx_TypedArray_prototype_entries, 0, _entries },
		{ fx_TypedArray_prototype_every, 1, _every },
		{ fx_TypedArray_prototype_fill, 1, _fill },
		{ fx_TypedArray_prototype_filter, 1, _filter },
		{ fx_TypedArray_prototype_find, 1, _find },
		{ fx_TypedArray_prototype_findIndex, 1, _findIndex },
		{ fx_TypedArray_prototype_forEach, 1, _forEach },
		{ fx_TypedArray_prototype_indexOf, 1, _indexOf },
		{ fx_TypedArray_prototype_join, 1, _join },
		{ fx_TypedArray_prototype_keys, 0, _keys },
		{ fx_TypedArray_prototype_lastIndexOf, 1, _lastIndexOf },
		{ fx_TypedArray_prototype_map, 1, _map },
		{ fx_TypedArray_prototype_reduce, 1, _reduce },
		{ fx_TypedArray_prototype_reduceRight, 1, _reduceRight },
		{ fx_TypedArray_prototype_reverse, 0, _reverse },
		{ fx_TypedArray_prototype_set, 1, _set },
		{ fx_TypedArray_prototype_slice, 2, _slice },
		{ fx_TypedArray_prototype_some, 1, _some },
		{ fx_TypedArray_prototype_sort, 1, _sort },
		{ fx_TypedArray_prototype_subarray, 2, _subarray },
		{ fx_TypedArray_prototype_join, 0, _toLocaleString },
		{ fx_TypedArray_prototype_join, 0, _toString },
		{ fx_TypedArray_prototype_values, 0, _values },
		{ fx_TypedArray_prototype_values, 0, _Symbol_iterator },
		{ C_NULL, 0, 0 },
    };
    const txHostFunctionBuilder* builder;
    txSlot* slot;
	txInteger index;
	const txTypeDispatch *dispatch;
	
	mxPush(mxObjectPrototype);
	slot = fxLastProperty(the, fxNewArrayBufferInstance(the));
	slot = fxNextHostAccessorProperty(the, slot, fx_ArrayBuffer_prototype_get_byteLength, C_NULL, mxID(_byteLength), XS_DONT_ENUM_FLAG);
	for (builder = gx_ArrayBuffer_prototype_builders; builder->callback; builder++)
		slot = fxNextHostFunctionProperty(the, slot, builder->callback, builder->length, mxID(builder->id), XS_DONT_ENUM_FLAG);
	slot = fxNextStringProperty(the, slot, "ArrayBuffer", mxID(_Symbol_toStringTag), XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	mxArrayBufferPrototype = *the->stack;
	slot = fxLastProperty(the, fxNewHostConstructorGlobal(the, fx_ArrayBuffer, 1, mxID(_ArrayBuffer), XS_GET_ONLY));
	slot = fxNextHostFunctionProperty(the, slot, fx_ArrayBuffer_fromString, 1, mxID(_fromString), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, fx_ArrayBuffer_isView, 1, mxID(_isView), XS_DONT_ENUM_FLAG);
	slot = fxNextHostAccessorProperty(the, slot, fx_species_get, C_NULL, mxID(_Symbol_species), XS_DONT_ENUM_FLAG);
	the->stack++;

	mxPush(mxObjectPrototype);
	slot = fxLastProperty(the, fxNewDataViewInstance(the));
	slot = fxNextHostAccessorProperty(the, slot, fx_DataView_prototype_buffer_get, C_NULL, mxID(_buffer), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG);
	slot = fxNextHostAccessorProperty(the, slot, fx_DataView_prototype_byteLength_get, C_NULL, mxID(_byteLength), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG);
	slot = fxNextHostAccessorProperty(the, slot, fx_DataView_prototype_byteOffset_get, C_NULL, mxID(_byteOffset), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG);
	for (index = 0, dispatch = &gTypeDispatches[0]; index < mxTypeDispatchCount - 1; index++, dispatch++) {
		slot = fxNextHostFunctionProperty(the, slot, fx_DataView_prototype_get, 1, mxID(dispatch->getID), XS_DONT_ENUM_FLAG);
		fxNextTypeDispatchProperty(the, fxLastProperty(the, slot->value.reference), (txTypeDispatch *)dispatch, XS_NO_ID, XS_GET_ONLY);
		slot = fxNextHostFunctionProperty(the, slot, fx_DataView_prototype_set, 2, mxID(dispatch->setID), XS_DONT_ENUM_FLAG);
		fxNextTypeDispatchProperty(the, fxLastProperty(the, slot->value.reference), (txTypeDispatch *)dispatch, XS_NO_ID, XS_GET_ONLY);
	}
	slot = fxNextStringProperty(the, slot, "DataView", mxID(_Symbol_toStringTag), XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	mxDataViewPrototype = *the->stack;
	slot = fxLastProperty(the, fxNewHostConstructorGlobal(the, fx_DataView, 1, mxID(_DataView), XS_GET_ONLY));
	the->stack++;
	
	mxPush(mxObjectPrototype);
	slot = fxLastProperty(the, fxNewObjectInstance(the));
	slot = fxNextHostAccessorProperty(the, slot, fx_TypedArray_prototype_buffer_get, C_NULL, mxID(_buffer), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG);
	slot = fxNextHostAccessorProperty(the, slot, fx_TypedArray_prototype_byteLength_get, C_NULL, mxID(_byteLength), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG);
	slot = fxNextHostAccessorProperty(the, slot, fx_TypedArray_prototype_byteOffset_get, C_NULL, mxID(_byteOffset), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG);
	slot = fxNextHostAccessorProperty(the, slot, fx_TypedArray_prototype_length_get, C_NULL, mxID(_length), XS_DONT_ENUM_FLAG);
	slot = fxNextHostAccessorProperty(the, slot, fx_TypedArray_prototype_toStringTag_get, C_NULL, mxID(_Symbol_toStringTag), XS_DONT_ENUM_FLAG);
	for (builder = gx_TypedArray_prototype_builders; builder->callback; builder++)
		slot = fxNextHostFunctionProperty(the, slot, builder->callback, builder->length, mxID(builder->id), XS_DONT_ENUM_FLAG);
	for (index = 0, dispatch = &gTypeDispatches[0]; index < mxTypeDispatchCount; index++, dispatch++) {
		--the->stack;
		*(the->stack) = *(the->stack + 1);
		slot = fxLastProperty(the, fxNewTypedArrayInstance(the, (txTypeDispatch *)dispatch));
		slot = fxNextIntegerProperty(the, slot, dispatch->size, mxID(_BYTES_PER_ELEMENT), XS_GET_ONLY);
		slot = fxLastProperty(the, fxNewHostConstructorGlobal(the, fx_TypedArray, 1, mxID(dispatch->constructorID), XS_GET_ONLY));
		slot = fxNextIntegerProperty(the, slot, dispatch->size, mxID(_BYTES_PER_ELEMENT), XS_GET_ONLY);
		slot = fxNextHostFunctionProperty(the, slot, fx_TypedArray_from, 1, mxID(_from), XS_DONT_ENUM_FLAG);
		slot = fxNextHostFunctionProperty(the, slot, fx_TypedArray_of, 0, mxID(_of), XS_DONT_ENUM_FLAG);
		slot = fxNextHostAccessorProperty(the, slot, fx_species_get, C_NULL, mxID(_Symbol_species), XS_DONT_ENUM_FLAG);
		the->stack++;
	}
	the->stack++;

	mxPush(mxIteratorPrototype);
	slot = fxLastProperty(the, fxNewObjectInstance(the));
	slot = fxNextHostFunctionProperty(the, slot, fx_TypedArray_prototype_entries_next, 0, mxID(_next), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG);
	mxPull(mxTypedArrayEntriesIteratorPrototype);
	mxPush(mxIteratorPrototype);
	slot = fxLastProperty(the, fxNewObjectInstance(the));
	slot = fxNextHostFunctionProperty(the, slot, fx_TypedArray_prototype_keys_next, 0, mxID(_next), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG);
	mxPull(mxTypedArrayKeysIteratorPrototype);
	mxPush(mxIteratorPrototype);
	slot = fxLastProperty(the, fxNewObjectInstance(the));
	slot = fxNextHostFunctionProperty(the, slot, fx_TypedArray_prototype_values_next, 0, mxID(_next), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG);
	mxPull(mxTypedArrayValuesIteratorPrototype);
}

txSlot* fxArgToInstance(txMachine* the, txInteger i)
{
	if (mxArgc > i)
		return fxToInstance(the, mxArgv(i));
	mxTypeError("Cannot coerce undefined to object");
	return C_NULL;
}

txBoolean fxCheckLength(txMachine* the, txSlot* slot, txInteger* index)
{
	txNumber number = fxToNumber(the, slot), check;
	*index = fxToInteger(the, slot);
	check = *index;
	return (number == check) ? 1 : 0;
}


txSlot* fxCheckArrayBufferInstance(txMachine* the, txSlot* slot)
{
	if (slot->kind == XS_REFERENCE_KIND) {
		slot = slot->value.reference;
		if (slot->next->kind == XS_ARRAY_BUFFER_KIND)
			return slot;
	}
	mxTypeError("this is no ArrayBuffer instance");
	return C_NULL;
}

txSlot* fxNewArrayBufferInstance(txMachine* the)
{
	txSlot* instance;
	txSlot* property;
	instance = fxNewObjectInstance(the);
	instance->flag |= XS_VALUE_FLAG;
	property = instance->next = fxNewSlot(the);
	property->flag = XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG;
	property->kind = XS_ARRAY_BUFFER_KIND;
	property->value.arrayBuffer.address = C_NULL;
	property->value.arrayBuffer.length = 0;
	return instance;
}

void fx_ArrayBuffer(txMachine* the)
{
	txSlot* instance = fxCheckArrayBufferInstance(the, mxThis);
	txSlot* arrayBuffer = instance->next;
	if ((mxArgc < 1) || !fxCheckLength(the, mxArgv(0), &(arrayBuffer->value.arrayBuffer.length)))
		mxRangeError("invalid length");
	arrayBuffer->value.arrayBuffer.address = fxNewChunk(the, arrayBuffer->value.arrayBuffer.length);
}

void fx_ArrayBuffer_fromString(txMachine* the)
{
	txInteger length;
	if (mxArgc < 1)
		mxTypeError("no argument");
	length = c_strlen(fxToString(the, mxArgv(0)));
	mxPushInteger(length);
	mxPushInteger(1);
	mxPushSlot(mxThis);
	fxGetID(the, mxID(_Symbol_species));
	fxNew(the);
	mxPullSlot(mxResult);
	c_memcpy(mxResult->value.reference->next->value.arrayBuffer.address, mxArgv(0)->value.string, length);
}


void fx_ArrayBuffer_isView(txMachine* the)
{
	txSlot* slot;
	mxResult->kind = XS_BOOLEAN_KIND;
	mxResult->value.boolean = 0;
	if (mxArgc > 0) {
		slot = mxArgv(0);
		if (slot->kind == XS_REFERENCE_KIND) {
			slot = slot->value.reference;
			if (slot->next) {
				slot = slot->next;
				if ((slot->kind == XS_DATA_VIEW_KIND) || (slot->kind == XS_TYPED_ARRAY_KIND)) {
					mxResult->value.boolean = 1;
				}
			}
		}
	}
}

void fx_ArrayBuffer_prototype_get_byteLength(txMachine* the)
{
	txSlot* instance = fxCheckArrayBufferInstance(the, mxThis);
	txSlot* arrayBuffer = instance->next;
	mxResult->kind = XS_INTEGER_KIND;
	mxResult->value.integer = arrayBuffer->value.arrayBuffer.length;
}

void fx_ArrayBuffer_prototype_concat(txMachine* the)
{
	txSlot* instance = fxCheckArrayBufferInstance(the, mxThis);
	txSlot* arrayBuffer = instance->next;
	txInteger length = arrayBuffer->value.arrayBuffer.length;
	txInteger c = mxArgc, i = 0;
	txByte* address;
	txSlot* slot;
	while (i < c) {
		arrayBuffer = C_NULL;
		slot = mxArgv(i);
		if (slot->kind == XS_REFERENCE_KIND) {
			slot = slot->value.reference->next;
			if (slot && (slot->kind == XS_ARRAY_BUFFER_KIND))
				arrayBuffer = slot;
		}
		if (arrayBuffer)
			length += arrayBuffer->value.arrayBuffer.length;
		else
			mxTypeError("arguments[%ld] is no ArrayBuffer instance", i);
		i++;
	}
	mxPushInteger(length);
	mxPushInteger(1);
	mxPushSlot(mxThis);
	fxGetID(the, mxID(_constructor));
	fxGetID(the, mxID(_Symbol_species));
	fxNew(the);
	mxPullSlot(mxResult);
	address = mxResult->value.reference->next->value.arrayBuffer.address;
	arrayBuffer = instance->next;
	length = arrayBuffer->value.arrayBuffer.length;
	c_memcpy(address, arrayBuffer->value.arrayBuffer.address, length);
	address += length;
	i = 0;
	while (i < c) {
		arrayBuffer = mxArgv(i)->value.reference->next;
		length = arrayBuffer->value.arrayBuffer.length;
		c_memcpy(address, arrayBuffer->value.arrayBuffer.address, length);
		address += length;
		i++;
	}
}

void fx_ArrayBuffer_prototype_slice(txMachine* the)
{
	txSlot* instance = fxCheckArrayBufferInstance(the, mxThis);
	txSlot* arrayBuffer = instance->next;
	txInteger length = arrayBuffer->value.arrayBuffer.length;
	txInteger start = fxArgToIndex(the, 0, 0, length);
	txInteger stop = fxArgToIndex(the, 1, length, length);
	if (stop < start) 
		stop = start;
	mxPushInteger(stop - start);
	mxPushInteger(1);
	mxPushSlot(mxThis);
	fxGetID(the, mxID(_constructor));
	fxGetID(the, mxID(_Symbol_species));
	fxNew(the);
	mxPullSlot(mxResult);
	c_memcpy(mxResult->value.reference->next->value.arrayBuffer.address, arrayBuffer->value.arrayBuffer.address + start, stop - start);
}


txSlot* fxCheckDataViewInstance(txMachine* the, txSlot* slot)
{
	if (slot->kind == XS_REFERENCE_KIND) {
		slot = slot->value.reference;
		if (slot->next->kind == XS_DATA_VIEW_KIND)
			return slot;
	}
	mxTypeError("this is no DataView instance");
	return C_NULL;
}

txSlot* fxNewDataViewInstance(txMachine* the)
{
	txSlot* instance;
	txSlot* property;
	instance = fxNewObjectInstance(the);
	instance->flag |= XS_VALUE_FLAG;
	property = instance->next = fxNewSlot(the);
	property->flag = XS_GET_ONLY;
	property->kind = XS_DATA_VIEW_KIND;
	property->value.dataView.offset = 0;
	property->value.dataView.size = 0;
	property = fxNextNullProperty(the, property, XS_NO_ID, XS_GET_ONLY);
	return instance;
}

void fx_DataView(txMachine* the)
{
	txSlot* instance = fxCheckDataViewInstance(the, mxThis);
	txSlot* view = instance->next;
	txSlot* buffer = view->next;
	txSlot* slot;
	txSlot* arrayBuffer = C_NULL;
	txInteger limit, offset, size;
	if ((mxArgc > 0) && (mxArgv(0)->kind == XS_REFERENCE_KIND)) {
		slot = mxArgv(0)->value.reference->next;
		if (slot && (slot->kind == XS_ARRAY_BUFFER_KIND))
			arrayBuffer = slot;
	}
	if (arrayBuffer)
		limit = arrayBuffer->value.arrayBuffer.length;
	else
		mxTypeError("buffer is no ArrayBuffer instance");
	if (mxArgc > 1)
		offset = fxToInteger(the, mxArgv(1));
	else
		offset = 0;
	if (mxArgc > 2)
		size = fxToInteger(the, mxArgv(2));
	else
		size = limit - offset;
	if ((offset < 0) || (limit < offset))
		mxRangeError("out of range byteOffset %ld", offset);
	if ((size < 0) || (limit < (offset + size)))
		mxRangeError("out of range byteLength %ld", size);
	view->value.dataView.offset = offset;
	view->value.dataView.size = size;
	buffer->kind = XS_REFERENCE_KIND;
	buffer->value.reference = mxArgv(0)->value.reference;
}

void fx_DataView_prototype_buffer_get(txMachine* the)
{
	txSlot* instance = fxCheckDataViewInstance(the, mxThis);
	txSlot* view = instance->next;
	txSlot* buffer = view->next;
	mxResult->kind = buffer->kind;
	mxResult->value = buffer->value;
}

void fx_DataView_prototype_byteLength_get(txMachine* the)
{
	txSlot* instance = fxCheckDataViewInstance(the, mxThis);
	txSlot* view = instance->next;
	mxResult->kind = XS_INTEGER_KIND;
	mxResult->value.integer = view->value.dataView.size;
}

void fx_DataView_prototype_byteOffset_get(txMachine* the)
{
	txSlot* instance = fxCheckDataViewInstance(the, mxThis);
	txSlot* view = instance->next;
	mxResult->kind = XS_INTEGER_KIND;
	mxResult->value.integer = view->value.dataView.offset;
}

void fx_DataView_prototype_get(txMachine* the)
{
	txSlot* dispatch = mxFunctionReferenceTypeDispatch(mxFunction);
	txSlot* instance = fxCheckDataViewInstance(the, mxThis);
	txSlot* view = instance->next;
	txSlot* buffer = view->next;
	txInteger delta = dispatch->value.typedArray->size;
	txInteger offset;
	int endian = EndianBig;
	if ((mxArgc < 1) || !fxCheckLength(the, mxArgv(0), &offset))
		mxRangeError("invalid byteOffset");
	if (mxArgc >= 2 && fxToBoolean(the, mxArgv(1)))
		endian = EndianLittle;
	if ((offset < 0) || (view->value.dataView.size < (offset + delta)))
		mxRangeError("out of range byteOffset");
	offset += view->value.dataView.offset;
	(*dispatch->value.typedArray->getter)(the, buffer->value.reference->next, offset, mxResult, endian);
}

void fx_DataView_prototype_set(txMachine* the)
{
	txSlot* dispatch = mxFunctionReferenceTypeDispatch(mxFunction);
	txSlot* instance = fxCheckDataViewInstance(the, mxThis);
	txSlot* view = instance->next;
	txSlot* buffer = view->next;
	txInteger delta = dispatch->value.typedArray->size;
	txInteger offset;
	int endian = EndianBig;
	if ((mxArgc < 1) || !fxCheckLength(the, mxArgv(0), &offset))
		mxRangeError("invalid byteOffset");
	if (mxArgc >= 3 && fxToBoolean(the, mxArgv(2)))
		endian = EndianLittle;
	if ((offset < 0) || (view->value.dataView.size < (offset + delta)))
		mxRangeError("out of range byteOffset");
	offset += view->value.dataView.offset;
	(*dispatch->value.typedArray->setter)(the, buffer->value.reference->next, offset, (mxArgc < 2) ? &mxUndefined : mxArgv(1), endian);
}

#define mxTypedArrayDeclarations \
	txSlot* instance = fxCheckTypedArrayInstance(the, mxThis); \
	txSlot* dispatch = instance->next; \
	txSlot* view = dispatch->next; \
	txSlot* buffer = view->next; \
	txSlot* data = buffer->value.reference->next; \
	txInteger delta = dispatch->value.typedArray->size; \
	txInteger length = view->value.dataView.size / delta


void fxCallTypedArrayItem(txMachine* the, txSlot* function, txSlot* dispatch, txSlot* view, txSlot* data, txInteger index, txSlot* item)
{
	/* ARG0 */
	mxPushUndefined();
	(*dispatch->value.typedArray->getter)(the, data, view->value.dataView.offset + (index * dispatch->value.typedArray->size), the->stack, EndianNative);
	if (item) {
		item->kind = the->stack->kind;
		item->value = the->stack->value;
	}
	/* ARG1 */
	mxPushInteger(index);
	/* ARG2 */
	mxPushSlot(mxThis);
	/* ARGC */
	mxPushInteger(3);
	/* THIS */
	if (mxArgc > 1)
		mxPushSlot(mxArgv(1));
	else
		mxPushUndefined();
	/* FUNCTION */
	mxPushReference(function);
	fxCall(the);
}

int fxCompareTypedArrayItem(txMachine* the, txSlot* function, txSlot* dispatch, txSlot* view, txSlot* data, txInteger index)
{
	txSlot* slot = the->stack;
	int result;
	mxPushUndefined();
	(*dispatch->value.typedArray->getter)(the, data, view->value.dataView.offset + (index * dispatch->value.typedArray->size), the->stack, EndianNative);
	mxPushSlot(slot);
	/* ARGC */
	mxPushInteger(2);
	/* THIS */
	mxPushSlot(mxThis);
	/* FUNCTION */
	mxPushReference(function);
	fxCall(the);
	if (the->stack->kind == XS_INTEGER_KIND)
		result = the->stack->value.integer;
	else {
		txNumber number = fxToNumber(the, the->stack);
		result = (number < 0) ? -1 :  (number > 0) ? 1 : 0;
	}
	the->stack++;
	return result;
}

void fxReduceTypedArrayItem(txMachine* the, txSlot* function, txSlot* dispatch, txSlot* view, txSlot* data, txInteger index)
{
	/* ARG0 */
	mxPushSlot(mxResult);
	/* ARG1 */
	mxPushUndefined();
	(*dispatch->value.typedArray->getter)(the, data, view->value.dataView.offset + (index * dispatch->value.typedArray->size), the->stack, EndianNative);
	/* ARG2 */
	mxPushInteger(index);
	/* ARG3 */
	mxPushSlot(mxThis);
	/* ARGC */
	mxPushInteger(4);
	/* THIS */
	mxPushUndefined();
	/* FUNCTION */
	mxPushReference(function);
	fxCall(the);
}

txSlot* fxCheckTypedArrayInstance(txMachine* the, txSlot* slot)
{
	if (slot->kind == XS_REFERENCE_KIND) {
		slot = slot->value.reference;
		if (slot->next->kind == XS_TYPED_ARRAY_KIND)
			return slot;
	}
	mxTypeError("this is no TypedArray instance");
	return C_NULL;
}

txSlot* fxNewTypedArrayInstance(txMachine* the, txTypeDispatch* dispatch)
{
	txSlot* instance;
	txSlot* property;
	instance = fxNewObjectInstance(the);
	instance->flag |= XS_VALUE_FLAG;
	property = fxNextTypeDispatchProperty(the, instance, dispatch, XS_NO_ID, XS_GET_ONLY);
	property = property->next = fxNewSlot(the);
	property->flag = XS_GET_ONLY;
	property->kind = XS_DATA_VIEW_KIND;
	property->value.dataView.offset = 0;
	property->value.dataView.size = 0;
	property = fxNextNullProperty(the, property, XS_NO_ID, XS_GET_ONLY);
	return instance;
}

txSlot* fxGetTypedArrayProperty(txMachine* the, txSlot* instance, txInteger index)
{
	txSlot* dispatch = instance->next;
	txSlot* view = dispatch->next;
	txSlot* buffer = view->next;
	txInteger delta = dispatch->value.typedArray->size;
	index *= delta;
	if ((0 <= index) && ((index + delta) <= view->value.dataView.size))
		(*dispatch->value.typedArray->getter)(the, buffer->value.reference->next, view->value.dataView.offset + index, &(the->scratch), EndianNative);
	else
		the->scratch.kind = XS_UNDEFINED_KIND;
	return &the->scratch;
}

txSlot* fxSetTypedArrayProperty(txMachine* the, txSlot* instance, txInteger index)
{
	txSlot* dispatch = instance->next;
	txSlot* view = dispatch->next;
	txSlot* buffer = view->next;
	txInteger delta = dispatch->value.typedArray->size;
	index *= delta;
	if ((0 <= index) && ((index + delta) <= view->value.dataView.size))
		(*dispatch->value.typedArray->setter)(the, buffer->value.reference->next, view->value.dataView.offset + index, the->stack + 1, EndianNative);
	return &the->scratch;
}

void fx_TypedArray(txMachine* the)
{
	txSlot* instance = fxCheckTypedArrayInstance(the, mxThis);
	txSlot* dispatch = instance->next;
	txSlot* view = dispatch->next;
	txSlot* buffer = view->next;
	txSlot* data = C_NULL;
	txInteger delta = dispatch->value.typedArray->size;
	txSlot* slot;
	if ((mxArgc > 0) && (mxArgv(0)->kind == XS_REFERENCE_KIND)) {
		slot = mxArgv(0)->value.reference->next;
		if (slot && (slot->kind == XS_ARRAY_BUFFER_KIND)) {
			txInteger limit = slot->value.arrayBuffer.length;
			txInteger offset = fxArgToInteger(the, 1, 0);
			txInteger size = (mxArgc > 2) ? delta * fxToInteger(the, mxArgv(2)) : limit - offset;
			if (offset % delta)
				mxRangeError("invalid byteOffset %ld", offset);
			if ((offset < 0) || (limit <= offset))
				mxRangeError("out of range byteOffset %ld", offset);
			if (size % delta)
				mxRangeError("invalid byteLength %ld", size);
			if ((size < 0) || (limit < (offset + size)))
				mxRangeError("out of range byteLength %ld", size);
			view->value.dataView.offset = offset;
			view->value.dataView.size = size;
			buffer->kind = XS_REFERENCE_KIND;
			buffer->value.reference = mxArgv(0)->value.reference;
		}
		else if (slot && (slot->kind == XS_TYPED_ARRAY_KIND)) {
			txSlot* arrayDispatch = slot;
			txSlot* arrayView = arrayDispatch->next;
			txSlot* arrayData = arrayView->next->value.reference->next;
			txInteger arrayDelta = arrayDispatch->value.typedArray->size;
			txInteger arrayLength = arrayView->value.dataView.size / arrayDelta;
			txInteger arrayOffset = arrayView->value.dataView.offset;
			txInteger offset = 0;
			txInteger size = arrayLength * delta;
			mxPushInteger(size);
			mxPushInteger(1);
			mxPush(mxGlobal);
			fxNewID(the, mxID(_ArrayBuffer));
			mxPullSlot(buffer);
			data = buffer->value.reference->next;
			view->value.dataView.offset = offset;
			view->value.dataView.size = size;
			if (dispatch == arrayDispatch)
				c_memcpy(data->value.arrayBuffer.address + offset, arrayData->value.arrayBuffer.address + arrayOffset, size);
			else {
				mxPushUndefined();
				while (offset < size) {
					(*arrayDispatch->value.typedArray->getter)(the, arrayData, arrayOffset, the->stack, EndianNative);
					(*dispatch->value.typedArray->setter)(the, data, offset, the->stack, EndianNative);
					arrayOffset += arrayDelta;
					offset += delta;
				}
				the->stack++;
			}
		}
		else {
			fx_TypedArray_from_object(the, instance, C_NULL, C_NULL);
		}
	}
	else {
        txInteger length = fxArgToInteger(the, 0, 0);
        length *= delta;
		mxPushInteger(length);
		mxPushInteger(1);
		mxPush(mxGlobal);
		fxNewID(the, mxID(_ArrayBuffer));
		mxPullSlot(buffer);
        view->value.dataView.offset = 0;
        view->value.dataView.size = length;
	}
}

void fx_TypedArray_from(txMachine* the)
{
	txSlot* function = C_NULL;
	txSlot* _this = C_NULL;
	if (mxArgc > 1) {
		txSlot* slot = mxArgv(1);
		if (slot->kind == XS_REFERENCE_KIND) {
			slot = slot->value.reference;
			if ((slot->next->kind == XS_CODE_KIND) || (slot->next->kind == XS_CALLBACK_KIND))
				function = slot;
		}
	}
	if (mxArgc > 2)
		_this = mxArgv(2);
	mxPushSlot(mxThis);
	fxGetID(the, mxID(_prototype));
	fxNewInstanceOf(the);
	mxPullSlot(mxResult);
	fx_TypedArray_from_object(the, mxResult->value.reference, function, _this);
}

void fx_TypedArray_from_object(txMachine* the, txSlot* instance, txSlot* function, txSlot* _this)
{
	txSlot* dispatch = instance->next;
	txSlot* view = dispatch->next;
	txSlot* buffer = view->next;
	txInteger delta = dispatch->value.typedArray->size;
	txSlot* data = C_NULL;
	txSlot* list = fxNewInstance(the);
	txSlot* slot = list;
	txInteger count, index;
	slot = list;
	mxPushSlot(mxArgv(0));
	if (fxHasID(the, mxID(_Symbol_iterator))) {
		txSlot* iterator;
		txSlot* result;
		mxPushInteger(0);
		mxPushSlot(mxArgv(0));
		fxCallID(the, mxID(_Symbol_iterator));
		iterator = the->stack;
		count = 0;
		for(;;) {
			mxPushInteger(0);
			mxPushSlot(iterator);
			fxCallID(the, mxID(_next));
			result = the->stack;
			mxPushSlot(result);
			fxGetID(the, mxID(_done));
			if (fxToBoolean(the, the->stack++))
				break;
			fxGetID(the, mxID(_value));
			slot = fxNextSlotProperty(the, slot, the->stack, XS_NO_ID, XS_NO_FLAG);
			the->stack++;
			count++;
		}
		the->stack += 2;
	}
	else {
		mxPushSlot(mxArgv(0));
		fxGetID(the, mxID(_length));
		count = fxToInteger(the, the->stack++);
		index = 0;
		while (index < count) {
			mxPushSlot(mxArgv(0));
			fxGetID(the, index);
			slot = fxNextSlotProperty(the, slot, the->stack, XS_NO_ID, XS_NO_FLAG);
			the->stack++;
			index++;
		}	
	}
    view->value.dataView.offset = 0;
    view->value.dataView.size = count * delta;
	mxPushInteger(view->value.dataView.size);
	mxPushInteger(1);
	mxPush(mxGlobal);
	fxNewID(the, mxID(_ArrayBuffer));
	mxPullSlot(buffer);
	data = buffer->value.reference->next;
	slot = list->next;
	index = 0;
	while (slot) {
		/* ARG0 */
		mxPushSlot(slot);
		if (function) {
			/* ARG1 */
			mxPushInteger(index);
			/* ARGC */
			mxPushInteger(2);
			/* THIS */
			if (_this)
				mxPushSlot(_this);
			else
				mxPushUndefined();
			/* FUNCTION */
			mxPushReference(function);
			fxCall(the);
		}
		(*dispatch->value.typedArray->setter)(the, data, (index * delta), the->stack, EndianNative);
		the->stack++;
		slot = slot->next;
		index++;
	}
	the->stack++;
}

void fx_TypedArray_of(txMachine* the)
{
	txInteger count = mxArgc;
	txInteger index = 0;
	mxPushInteger(count);
	mxPushInteger(1);
	mxPushSlot(mxThis);
	fxNew(the);
	mxPullSlot(mxResult);
	{
		txSlot* resultDispatch = mxResult->value.reference->next;
		txSlot* resultView = resultDispatch->next;
		txSlot* resultData = resultView->next->value.reference->next;
		while (index < count) {
			(*resultDispatch->value.typedArray->setter)(the, resultData, resultView->value.dataView.offset + (index * resultDispatch->value.typedArray->size), mxArgv(index), EndianNative);
			index++;
		}
	}
}

void fx_TypedArray_prototype_buffer_get(txMachine* the)
{
	txSlot* instance = fxCheckTypedArrayInstance(the, mxThis);
	txSlot* dispatch = instance->next;
	txSlot* view = dispatch->next;
	txSlot* buffer = view->next;
	mxResult->kind = buffer->kind;
	mxResult->value = buffer->value;
}

void fx_TypedArray_prototype_byteLength_get(txMachine* the)
{
	txSlot* instance = fxCheckTypedArrayInstance(the, mxThis);
	txSlot* dispatch = instance->next;
	txSlot* view = dispatch->next;
	mxResult->kind = XS_INTEGER_KIND;
	mxResult->value.integer = view->value.dataView.size;
}

void fx_TypedArray_prototype_byteOffset_get(txMachine* the)
{
	txSlot* instance = fxCheckTypedArrayInstance(the, mxThis);
	txSlot* dispatch = instance->next;
	txSlot* view = dispatch->next;
	mxResult->kind = XS_INTEGER_KIND;
	mxResult->value.integer = view->value.dataView.offset;
}

void fx_TypedArray_prototype_copyWithin(txMachine* the)
{
	mxTypedArrayDeclarations;
	txByte* address = data->value.arrayBuffer.address + view->value.dataView.offset;
	txInteger target = fxArgToIndex(the, 0, 0, length);
	txInteger start = fxArgToIndex(the, 1, 0, length);
	txInteger end = fxArgToIndex(the, 2, length, length);
	txInteger count = end - start;
	if (count > length - target)
		count = length - target;
	c_memmove(address + (target * delta), address + (start * delta), count * delta);
	mxResult->kind = mxThis->kind;
	mxResult->value = mxThis->value;
}

void fx_TypedArray_prototype_entries(txMachine* the)
{
	fxCheckTypedArrayInstance(the, mxThis);
	mxPush(mxTypedArrayEntriesIteratorPrototype);
	fxNewIteratorInstance(the, mxThis);
	mxPullSlot(mxResult);
}

void fx_TypedArray_prototype_entries_next(txMachine* the)
{
	txSlot* iterator = fxGetInstance(the, mxThis);
	txSlot* result = iterator->next;
	txSlot* iterable = result->next;
	txSlot* index = iterable->next;
	txSlot* value = result->value.reference->next;
	txSlot* done = value->next;
	txSlot* instance = iterable->value.reference;
	txSlot* dispatch = instance->next;
	txSlot* view = dispatch->next;
	txSlot* buffer = view->next;
	txInteger offset = index->value.integer * dispatch->value.typedArray->size;
	if (offset < view->value.dataView.size) {
		mxPushSlot(index);
		mxPushUndefined();
		(*dispatch->value.typedArray->getter)(the, buffer->value.reference->next, view->value.dataView.offset + offset, the->stack, EndianNative);
		fxConstructArrayEntry(the, value);
		index->value.integer++;
	}
	else {
		value->kind = XS_UNDEFINED_KIND;
		done->value.boolean = 1;
	}
	mxResult->kind = result->kind;
	mxResult->value = result->value;
}

void fx_TypedArray_prototype_every(txMachine* the)
{
	mxTypedArrayDeclarations;
	txSlot* function = fxArgToCallback(the, 0);
	txInteger index = 0;
	mxResult->kind = XS_BOOLEAN_KIND;
	mxResult->value.boolean = 1;
	while (index < length) {
		fxCallTypedArrayItem(the, function, dispatch, view, data, index, C_NULL);
		mxResult->value.boolean = fxToBoolean(the, the->stack++);
		if (!mxResult->value.boolean)
			break;
		index++;
	}
}

void fx_TypedArray_prototype_fill(txMachine* the)
{
	mxTypedArrayDeclarations;
	txInteger start = fxArgToIndex(the, 1, 0, length);
	txInteger end = fxArgToIndex(the, 2, length, length);
	start *= delta;
	end *= delta;
	start += view->value.dataView.offset;
	end += view->value.dataView.offset;
	if (mxArgc > 0)
		mxPushSlot(mxArgv(0));
	else
		mxPushUndefined();
	while (start < end) {
		(*dispatch->value.typedArray->setter)(the, data, start, the->stack, EndianNative);
		start += delta;
	}
	the->stack++;
	mxResult->kind = mxThis->kind;
	mxResult->value = mxThis->value;
}

void fx_TypedArray_prototype_filter(txMachine* the)
{
	mxTypedArrayDeclarations;
	txSlot* function = fxArgToCallback(the, 0);
	txSlot* list = fxNewInstance(the);
	txSlot* slot = list;
	txInteger count = 0;
	txInteger index = 0;
	mxPushUndefined();
	while (index < length) {
		fxCallTypedArrayItem(the, function, dispatch, view, data, index, the->stack);
		if (fxToBoolean(the, the->stack++)) {
			count++;
			slot = fxNextSlotProperty(the, slot, the->stack, XS_NO_ID, XS_NO_FLAG);
		}
		index++;
	}
	the->stack++;
	mxPushInteger(count);
	mxPushInteger(1);
	mxPushSlot(mxThis);
	fxGetID(the, mxID(_constructor)); // @@ species
	fxNew(the);
	mxPullSlot(mxResult);
	{
		txSlot* resultDispatch = mxResult->value.reference->next;
		txSlot* resultView = resultDispatch->next;
		txSlot* resultData = resultView->next->value.reference->next;
		txInteger resultDelta = resultDispatch->value.typedArray->size;
		txInteger resultOffset = 0;
		slot = list->next;
		while (slot) {
			(*resultDispatch->value.typedArray->setter)(the, resultData, resultOffset, slot, EndianNative);
			resultOffset += resultDelta;
			slot = slot->next;
		}
	}
	the->stack++;
}

void fx_TypedArray_prototype_find(txMachine* the)
{
	mxTypedArrayDeclarations;
	txSlot* function = fxArgToCallback(the, 0);
	txInteger index = 0;
	mxPushUndefined();
	while (index < length) {
		fxCallTypedArrayItem(the, function, dispatch, view, data, index, the->stack);
		if (fxToBoolean(the, the->stack++)) {
			mxResult->kind = the->stack->kind;
			mxResult->value = the->stack->value;
			break;
		}
		index++;
	}
	the->stack++;
}

void fx_TypedArray_prototype_findIndex(txMachine* the)
{
	mxTypedArrayDeclarations;
	txSlot* function = fxArgToCallback(the, 0);
	txInteger index = 0;
	mxResult->kind = XS_INTEGER_KIND;
	mxResult->value.integer = -1;
	while (index < length) {
		fxCallTypedArrayItem(the, function, dispatch, view, data, index, C_NULL);
		if (fxToBoolean(the, the->stack)) {
			mxResult->value.integer = index;
			break;
		}
		the->stack++;
		index++;
	}
}

void fx_TypedArray_prototype_forEach(txMachine* the)
{
	mxTypedArrayDeclarations;
	txSlot* function = fxArgToCallback(the, 0);
	txInteger index = 0;
	while (index < length) {
		fxCallTypedArrayItem(the, function, dispatch, view, data, index, C_NULL);
		the->stack++;
		index++;
	}
}

void fx_TypedArray_prototype_indexOf(txMachine* the)
{
	mxTypedArrayDeclarations;
	txInteger index = fxArgToIndex(the, 1, 0, length);
	mxResult->kind = XS_INTEGER_KIND;
	mxResult->value.integer = -1;
	if (mxArgc > 0)
		mxPushSlot(mxArgv(0));
	else
		mxPushUndefined();
	mxPushUndefined();
	while (index < length) {
		(*dispatch->value.typedArray->getter)(the, data, view->value.dataView.offset + (index * delta), the->stack, EndianNative);
		if (fxIsSameSlot(the, the->stack, the->stack + 1)) {
			mxResult->value.integer = index;
			break;
		}
		index++;
	}
	the->stack += 2;
}

void fx_TypedArray_prototype_join(txMachine* the)
{
	mxTypedArrayDeclarations;
	if (length) {
		txInteger offset = view->value.dataView.offset;
		txInteger limit = offset + view->value.dataView.size;
		txString string;
		txBoolean comma = 0;
		txInteger size = 0;
		txSlot* list = fxNewInstance(the);
		txSlot* slot = list;
		if ((mxArgc > 0) && (mxArgv(0)->kind != XS_UNDEFINED_KIND)) {
			mxPushSlot(mxArgv(0));
			string = fxToString(the, the->stack);
			the->stack->kind = XS_KEY_KIND;
			the->stack->value.key.sum = c_strlen(string);
		}
		else {
			mxPushStringC(",");
			the->stack->kind = XS_KEY_KIND;
			the->stack->value.key.sum = 1;
		}
		mxPushUndefined();
		while (offset < limit) {
			if (comma) {
				slot = fxNextSlotProperty(the, slot, the->stack + 1, XS_NO_ID, XS_NO_FLAG);
				size += slot->value.key.sum;
			}
			else
				comma = 1;
			(*dispatch->value.typedArray->getter)(the, data, offset, the->stack, EndianNative);
			slot = fxNextSlotProperty(the, slot, the->stack, XS_NO_ID, XS_NO_FLAG);
			string = fxToString(the, slot);
			slot->kind = XS_KEY_KIND;
			slot->value.key.sum = c_strlen(string);
			size += slot->value.key.sum;
			offset += delta;
		}
		the->stack += 2;
		string = mxResult->value.string = fxNewChunk(the, size + 1);
		slot = list->next;
		while (slot) {
			c_memcpy(string, slot->value.key.string, slot->value.key.sum);
			string += slot->value.key.sum;
			slot = slot->next;
		}
		*string = 0;
		the->stack++;
		mxResult->kind = XS_STRING_KIND;
	}
	else {
		mxResult->value = mxEmptyString.value;
		mxResult->kind = mxEmptyString.kind;
	}
}

void fx_TypedArray_prototype_keys(txMachine* the)
{
	fxCheckTypedArrayInstance(the, mxThis);
	mxPush(mxTypedArrayKeysIteratorPrototype);
	fxNewIteratorInstance(the, mxThis);
	mxPullSlot(mxResult);
}

void fx_TypedArray_prototype_keys_next(txMachine* the)
{
	txSlot* iterator = fxGetInstance(the, mxThis);
	txSlot* result = iterator->next;
	txSlot* iterable = result->next;
	txSlot* index = iterable->next;
	txSlot* value = result->value.reference->next;
	txSlot* done = value->next;
	txSlot* instance = iterable->value.reference;
	txSlot* dispatch = instance->next;
	txSlot* view = dispatch->next;
	txInteger offset = index->value.integer * dispatch->value.typedArray->size;
	if (offset < view->value.dataView.size) {
		value->kind = XS_INTEGER_KIND;
		value->value.integer = index->value.integer;
		index->value.integer++;
	}
	else {
		value->kind = XS_UNDEFINED_KIND;
		done->value.boolean = 1;
	}
	mxResult->kind = result->kind;
	mxResult->value = result->value;
}

void fx_TypedArray_prototype_lastIndexOf(txMachine* the)
{
	mxTypedArrayDeclarations;
	txInteger index = fxArgToIndex(the, 1, length, length);
	mxResult->kind = XS_INTEGER_KIND;
	mxResult->value.integer = -1;
	if (mxArgc > 0)
		mxPushSlot(mxArgv(0));
	else
		mxPushUndefined();
	mxPushUndefined();
    if (index == length)
        index--;
	while (index >= 0) {
		(*dispatch->value.typedArray->getter)(the, data, view->value.dataView.offset + (index * delta), the->stack, EndianNative);
		if (fxIsSameSlot(the, the->stack, the->stack + 1)) {
			mxResult->value.integer = index;
			break;
		}
        index--;
	}
	the->stack += 2;
}

void fx_TypedArray_prototype_length_get(txMachine* the)
{
	txSlot* instance = fxCheckTypedArrayInstance(the, mxThis);
	txSlot* dispatch = instance->next;
	txSlot* view = dispatch->next;
	txInteger delta = dispatch->value.typedArray->size;
	txInteger length = view->value.dataView.size / delta;
	mxResult->kind = XS_INTEGER_KIND;
	mxResult->value.integer = length;
}

void fx_TypedArray_prototype_map(txMachine* the)
{
	mxTypedArrayDeclarations;
	txSlot* function = fxArgToCallback(the, 0);
	mxPushInteger(length);
	mxPushInteger(1);
	mxPushSlot(mxThis);
	fxGetID(the, mxID(_constructor)); // @@ species
	fxNew(the);
	mxPullSlot(mxResult);
	{
		txSlot* resultDispatch = mxResult->value.reference->next;
		txSlot* resultView = resultDispatch->next;
		txSlot* resultData = resultView->next->value.reference->next;
		txInteger index = 0;
		while (index < length) {
			fxCallTypedArrayItem(the, function, dispatch, view, data, index, C_NULL);
			(*dispatch->value.typedArray->setter)(the, resultData, resultView->value.dataView.offset + (index * resultDispatch->value.typedArray->size), the->stack, EndianNative);
			the->stack++;
			index++;
		}
	}
}

void fx_TypedArray_prototype_reduce(txMachine* the)
{
	mxTypedArrayDeclarations;
	txSlot* function = fxArgToCallback(the, 0);
	txInteger index = 0;
	if (mxArgc > 1)
		*mxResult = *mxArgv(1);
	else if (index < length) {
		(*dispatch->value.typedArray->getter)(the, data, view->value.dataView.offset, mxResult, EndianNative);
		index++;
	}
	while (index < length) {
		fxReduceTypedArrayItem(the, function, dispatch, view, data, index);
		mxPullSlot(mxResult);
		index++;
	}
}

void fx_TypedArray_prototype_reduceRight(txMachine* the)
{
	mxTypedArrayDeclarations;
	txSlot* function = fxArgToCallback(the, 0);
	txInteger index = length - 1;
	if (mxArgc > 1)
		*mxResult = *mxArgv(1);
	else if (index >= 0) {
		(*dispatch->value.typedArray->getter)(the, data, view->value.dataView.offset + (index * delta), mxResult, EndianNative);
		index--;
	}
	while (index >= 0) {
		fxReduceTypedArrayItem(the, function, dispatch, view, data, index);
		mxPullSlot(mxResult);
		index--;
	}
}

void fx_TypedArray_prototype_reverse(txMachine* the)
{
	mxTypedArrayDeclarations;
	if (length) {
		txByte buffer;
		txByte* first = data->value.arrayBuffer.address + view->value.dataView.offset;
		txByte* last = first + view->value.dataView.size - delta;
		txInteger offset;
		while (first < last) {
			for (offset = 0; offset < delta; offset++) {
				buffer = last[offset];
				last[offset] = first[offset];
				first[offset] = buffer;
			}
			first += delta;
			last -= delta;
		}
	}
	mxResult->kind = mxThis->kind;
	mxResult->value = mxThis->value;
}

void fx_TypedArray_prototype_set(txMachine* the)
{
	mxTypedArrayDeclarations;
	txSlot* array = fxArgToInstance(the, 0);
	txInteger target = fxArgToInteger(the, 1, 0);
	txInteger offset = view->value.dataView.offset + (target * delta);	
	if (array->next && (array->next->kind == XS_TYPED_ARRAY_KIND)) {
		txSlot* arrayDispatch = array->next;
		txSlot* arrayView = arrayDispatch->next;
		txSlot* arrayData = arrayView->next->value.reference->next;
		txInteger arrayDelta = arrayDispatch->value.typedArray->size;
		txInteger arrayLength = arrayView->value.dataView.size / arrayDelta;
		txInteger arrayOffset = arrayView->value.dataView.offset;	
		txInteger limit = offset + (arrayLength * delta);
		if ((target < 0) || (length - arrayLength < target))
			mxRangeError("invalid offset");
		if (data == arrayData) {
			txSlot* resultData;
			mxPushInteger(arrayLength * arrayDelta);
			mxPushInteger(1);
			mxPush(mxGlobal);
			fxNewID(the, mxID(_ArrayBuffer));
			resultData = the->stack->value.reference->next;
			c_memcpy(resultData->value.arrayBuffer.address, arrayData->value.arrayBuffer.address + arrayOffset, arrayLength * arrayDelta);
			arrayData = resultData;
			arrayOffset = 0;
		}
		else 
			mxPushUndefined();
		if (dispatch == arrayDispatch)
			c_memcpy(data->value.arrayBuffer.address + offset, arrayData->value.arrayBuffer.address + arrayOffset, limit - offset);
		else {
			mxPushUndefined();
			while (offset < limit) {
				(*arrayDispatch->value.typedArray->getter)(the, arrayData, arrayOffset, the->stack, EndianNative);
				(*dispatch->value.typedArray->setter)(the, data, offset, the->stack, EndianNative);
				arrayOffset += arrayDelta;
				offset += delta;
			}
			the->stack++;
		}
		the->stack++;
	}
	else {
		txInteger count, index;
		mxPushSlot(mxArgv(0));
		fxGetID(the, mxID(_length));
		count = fxToInteger(the, the->stack++);
		if ((target < 0) || (length - count < target))
			mxRangeError("invalid offset");
		index = 0;
		while (index < count) {
			mxPushSlot(mxArgv(0));
			fxGetID(the, index);
			(*dispatch->value.typedArray->setter)(the, data, offset, the->stack++, EndianNative);
			offset += delta;
			index++;
		}	
	}
}

void fx_TypedArray_prototype_slice(txMachine* the)
{
	mxTypedArrayDeclarations;
	txInteger start = fxArgToIndex(the, 0, 0, length);
	txInteger end = fxArgToIndex(the, 1, length, length);
	txInteger count = (end > start) ? end - start : 0;
	txInteger index = 0;
	mxPushInteger(count);
	mxPushInteger(1);
	mxPushSlot(mxThis);
	fxGetID(the, mxID(_constructor)); // @@ species
	fxNew(the);
	mxPullSlot(mxResult);
	mxPushUndefined();
	{
		txSlot* resultDispatch = mxResult->value.reference->next;
		txSlot* resultView = resultDispatch->next;
		txSlot* resultData = resultView->next->value.reference->next;
		while (start < end) {
			(*dispatch->value.typedArray->getter)(the, data, view->value.dataView.offset + (start * delta), the->stack, EndianNative);
			(*resultDispatch->value.typedArray->setter)(the, resultData, resultView->value.dataView.offset + (index * resultDispatch->value.typedArray->size), the->stack, EndianNative);
			start++;
			index++;
		}
	}
	the->stack++;
}

void fx_TypedArray_prototype_some(txMachine* the)
{
	mxTypedArrayDeclarations;
	txSlot* function = fxArgToCallback(the, 0);
	txInteger index = 0;
	mxResult->kind = XS_BOOLEAN_KIND;
	mxResult->value.boolean = 0;
	while (index < length) {
		fxCallTypedArrayItem(the, function, dispatch, view, data, index, C_NULL);
		mxResult->value.boolean = fxToBoolean(the, the->stack++);
		if (mxResult->value.boolean)
			break;
		index++;
	}
}

void fx_TypedArray_prototype_sort(txMachine* the)
{
	mxTypedArrayDeclarations;
	txSlot* function = C_NULL;
	if (mxArgc > 0) {
		txSlot* slot = mxArgv(0);
		if (slot->kind != XS_UNDEFINED_KIND) {
			slot = fxToInstance(the, slot);
			if ((slot->next->kind == XS_CODE_KIND) || (slot->next->kind == XS_CODE_X_KIND) || (slot->next->kind == XS_CALLBACK_KIND))
				function = slot;
			else
				mxTypeError("compare is no function");
		}
	}
	if (function) {
		/* like GCC qsort */
		#define COMPARE(INDEX) \
			fxCompareTypedArrayItem(the, function, dispatch, view, data, INDEX)
		#define MOVE(FROM,TO) \
			from = data->value.arrayBuffer.address + view->value.dataView.offset + ((FROM) * delta); \
			to = data->value.arrayBuffer.address + view->value.dataView.offset + ((TO) * delta); \
			for (k = 0; k < delta; k++) *to++ = *from++
		#define PUSH(INDEX) \
			mxPushUndefined(); \
			(*dispatch->value.typedArray->getter)(the, data, view->value.dataView.offset + ((INDEX) * delta), the->stack, EndianNative)
		#define PULL(INDEX) \
			(*dispatch->value.typedArray->setter)(the, data, view->value.dataView.offset + ((INDEX) * delta), the->stack++, EndianNative)
		if (length > 0) {
			txInteger i, j, k;
			txByte* from;
			txByte* to;
			if (length > mxSortThreshold) {
				txInteger lo = 0, hi = length - 1;
				txSortPartition stack[mxSortStackSize];
				txSortPartition *top = stack + 1;
				while (stack < top) {
					txIndex mid = lo + ((hi - lo) >> 1);
					PUSH(mid);
					if (COMPARE(lo) > 0) {
						MOVE(lo, mid);
						PULL(lo);
						PUSH(mid);
					}
					if (COMPARE(hi) < 0) {
						MOVE(hi, mid);
						PULL(hi);
						PUSH(mid);
						if (COMPARE(lo) > 0) {
							MOVE(lo, mid);
							PULL(lo);
							PUSH(mid);
						}
					}
					i = lo + 1;
					j = hi - 1;
					do {
						while (COMPARE(i) < 0) i++;
						while (COMPARE(j) > 0) j--;
						if (i < j) {
							PUSH(i);
							MOVE(j, i);
							PULL(j);
							i++;
							j--;
						}
						else if (i == j) {
							i++;
							j--;
							break;
						}
					} while (i <= j);
					if ((j - lo) <= mxSortThreshold) {
						if ((hi - i) <= mxSortThreshold) {
							top--;
							lo = top->lo; 
							hi = top->hi;
						}
						else {
							lo = i;
						}
					}
					else if ((hi - i) <= mxSortThreshold) {
						hi = j;
					}
					else if ((j - lo) > (hi - i)) {
						top->lo = lo;
						top->hi = j; 
						top++;
						lo = i;
					}
					else {
						top->lo = i;
						top->hi = hi; 
						top++;
						hi = j;
					}
				}
			}
			for (i = 1; i < length; i++) {
				PUSH(i);
				for (j = i; (j > 0) && (COMPARE(j - 1) > 0); j--) {
					MOVE(j - 1, j);
				}	
				PULL(j);
			}
		}
	}
	else
		c_qsort(data->value.arrayBuffer.address, length, delta, dispatch->value.typedArray->compare);
	mxResult->kind = mxThis->kind;
	mxResult->value = mxThis->value;
}

void fx_TypedArray_prototype_subarray(txMachine* the)
{
	txSlot* instance = fxCheckTypedArrayInstance(the, mxThis);
	txSlot* dispatch = instance->next;
	txSlot* view = dispatch->next;
	txSlot* buffer = view->next;
	txInteger delta = dispatch->value.typedArray->size;
	txInteger length = view->value.dataView.size / delta;
	txInteger start = fxArgToIndex(the, 0, 0, length);
	txInteger stop = fxArgToIndex(the, 1, length, length);
	if (stop < start) 
		stop = start;
	mxPushSlot(buffer);
	mxPushInteger(start * delta);
	mxPushInteger(stop - start);
	mxPushInteger(3);
	mxPush(mxGlobal);
	fxNewID(the, mxID(dispatch->value.typedArray->constructorID));
	mxPullSlot(mxResult);
}

void fx_TypedArray_prototype_toStringTag_get(txMachine* the)
{
	txSlot* instance = fxGetInstance(the, mxThis);
	txTypeDispatch *dispatch = instance->next->value.typedArray;
	txSlot* key = fxGetKey(the, mxID(dispatch->constructorID));
	if (key->kind == XS_KEY_X_KIND)
		mxResult->kind = XS_STRING_X_KIND;
	else
		mxResult->kind = XS_STRING_KIND;
	mxResult->value.string = key->value.key.string;
}

void fx_TypedArray_prototype_values(txMachine* the)
{
	fxCheckTypedArrayInstance(the, mxThis);
	mxPush(mxTypedArrayValuesIteratorPrototype);
	fxNewIteratorInstance(the, mxThis);
	mxPullSlot(mxResult);
}

void fx_TypedArray_prototype_values_next(txMachine* the)
{
	txSlot* iterator = fxGetInstance(the, mxThis);
	txSlot* result = iterator->next;
	txSlot* iterable = result->next;
	txSlot* index = iterable->next;
	txSlot* value = result->value.reference->next;
	txSlot* done = value->next;
	txSlot* instance = iterable->value.reference;
	txSlot* dispatch = instance->next;
	txSlot* view = dispatch->next;
	txSlot* buffer = view->next;
	txInteger offset = index->value.integer * dispatch->value.typedArray->size;
	if (offset < view->value.dataView.size) {
		(*dispatch->value.typedArray->getter)(the, buffer->value.reference->next, view->value.dataView.offset + offset, value, EndianNative);
		index->value.integer++;
	}
	else {
		value->kind = XS_UNDEFINED_KIND;
		done->value.boolean = 1;
	}
	mxResult->kind = result->kind;
	mxResult->value = result->value;
}

#if mxBigEndian
	#define mxEndianDouble_BtoN(a) (a)
	#define mxEndianFloat_BtoN(a) (a)
	#define mxEndianS32_BtoN(a) (a)
	#define mxEndianU32_BtoN(a) (a)
	#define mxEndianS16_BtoN(a) (a)
	#define mxEndianU16_BtoN(a) (a)

	#define mxEndianDouble_NtoB(a) (a)
	#define mxEndianFloat_NtoB(a) (a)
	#define mxEndianS32_NtoB(a) (a)
	#define mxEndianU32_NtoB(a) (a)
	#define mxEndianS16_NtoB(a) (a)
	#define mxEndianU16_NtoB(a) (a)
#else
	#define mxEndianDouble_LtoN(a) (a)
	#define mxEndianFloat_LtoN(a) (a)
	#define mxEndianS32_LtoN(a) (a)
	#define mxEndianU32_LtoN(a) (a)
	#define mxEndianS16_LtoN(a) (a)
	#define mxEndianU16_LtoN(a) (a)

	#define mxEndianDouble_NtoL(a) (a)
	#define mxEndianFloat_NtoL(a) (a)
	#define mxEndianS32_NtoL(a) (a)
	#define mxEndianU32_NtoL(a) (a)
	#define mxEndianS16_NtoL(a) (a)
	#define mxEndianU16_NtoL(a) (a)
#endif

#if mxLittleEndian
	#define mxEndianDouble_BtoN(a) (mxEndianDouble_Swap(a))
	#define mxEndianFloat_BtoN(a) (mxEndianFloat_Swap(a))
	#define mxEndianS32_BtoN(a) ((txS4) mxEndian32_Swap(a))
	#define mxEndianU32_BtoN(a) ((txU4) mxEndian32_Swap(a))
	#define mxEndianS16_BtoN(a) ((txS2) mxEndian16_Swap(a))
	#define mxEndianU16_BtoN(a) ((txU2) mxEndian16_Swap(a))

	#define mxEndianDouble_NtoB(a) (mxEndianDouble_Swap(a))
	#define mxEndianFloat_NtoB(a) (mxEndianFloat_Swap(a))
	#define mxEndianS32_NtoB(a) ((txS4) mxEndian32_Swap(a))
	#define mxEndianU32_NtoB(a) ((txU4) mxEndian32_Swap(a))
	#define mxEndianS16_NtoB(a) ((txS2) mxEndian16_Swap(a))
	#define mxEndianU16_NtoB(a) ((txU2) mxEndian16_Swap(a))
#else
	#define mxEndianDouble_LtoN(a) (mxEndianDouble_Swap(a))
	#define mxEndianFloat_LtoN(a) (mxEndianFloat_Swap(a))
	#define mxEndianS32_LtoN(a) ((txS4) mxEndian32_Swap(a))
	#define mxEndianU32_LtoN(a) ((txU4) mxEndian32_Swap(a))
	#define mxEndianS16_LtoN(a) ((txS2) mxEndian16_Swap(a))
	#define mxEndianU16_LtoN(a) ((txU2) mxEndian16_Swap(a))

	#define mxEndianDouble_NtoL(a) (mxEndianDouble_Swap(a))
	#define mxEndianFloat_NtoL(a) (mxEndianFloat_Swap(a))
	#define mxEndianS32_NtoL(a) ((txS4) mxEndian32_Swap(a))
	#define mxEndianU32_NtoL(a) ((txU4) mxEndian32_Swap(a))
	#define mxEndianS16_NtoL(a) ((txS2) mxEndian16_Swap(a))
	#define mxEndianU16_NtoL(a) ((txU2) mxEndian16_Swap(a))
#endif

static txU2 mxEndian16_Swap(txU2 a)
{
	txU2 b;
	txU1 *p1 = (txU1 *) &a, *p2 = (txU1 *) &b;
	int i;
	for (i = 0; i < 2; i++)
		p2[i] = p1[1 - i];
	return b;
}

static txU4 mxEndian32_Swap(txU4 a)
{
	txU4 b;
	txU1 *p1 = (txU1 *) &a, *p2 = (txU1 *) &b;
	int i;
	for (i = 0; i < 4; i++)
		p2[i] = p1[3 - i];
	return b;
}

static float mxEndianFloat_Swap(float a)
{
	float b;
	txU1 *p1 = (txU1 *) &a, *p2 = (txU1 *) &b;
	int i;
	for (i = 0; i < 4; i++)
		p2[i] = p1[3 - i];
	return b;
}

static double mxEndianDouble_Swap(double a)
{
	double b;
	txU1 *p1 = (txU1 *) &a, *p2 = (txU1 *) &b;
	int i;
	for (i = 0; i < 8; i++)
		p2[i] = p1[7 - i];
	return b;
}

#define toNative(size, endian) mxEndian##size##_##endian##toN
#define fromNative(size, endian) mxEndian##size##_Nto##endian
#define IMPORT(size) (endian == EndianBig ? toNative(size, B)(value) : endian == EndianLittle ? toNative(size, L)(value) : (value))
#define EXPORT(size) (endian == EndianBig ? fromNative(size, B)(value) : endian == EndianLittle ? toNative(size, L)(value) : (value))

int fxFloat32Compare(const void* p, const void* q)
{
	float a = *((float*)p);
	float b = *((float*)q);
	return (a < b) ? -1 : (a > b) ? 1 : 0;
}

void fxFloat32Getter(txMachine* the, txSlot* data, txInteger offset, txSlot* slot, int endian)
{
	float value;
	slot->kind = XS_NUMBER_KIND;
	value = *((float*)(data->value.arrayBuffer.address + offset));
	slot->value.number = IMPORT(Float);
}

void fxFloat32Setter(txMachine* the, txSlot* data, txInteger offset, txSlot* slot, int endian)
{
	float value = (float)fxToNumber(the, slot);
	*((float*)(data->value.arrayBuffer.address + offset)) = EXPORT(Float);
}

int fxFloat64Compare(const void* p, const void* q)
{
	double a = *((double*)p);
	double b = *((double*)q);
	return (a < b) ? -1 : (a > b) ? 1 : 0;
}

void fxFloat64Getter(txMachine* the, txSlot* data, txInteger offset, txSlot* slot, int endian)
{
	double value;
	slot->kind = XS_NUMBER_KIND;
	value = *((double*)(data->value.arrayBuffer.address + offset));
	slot->value.number = IMPORT(Double);
}

void fxFloat64Setter(txMachine* the, txSlot* data, txInteger offset, txSlot* slot, int endian)
{
	double value = (double)fxToNumber(the, slot);
	*((double*)(data->value.arrayBuffer.address + offset)) = EXPORT(Double);
}

int fxInt8Compare(const void* p, const void* q)
{
	txS1 a = *((txS1*)p);
	txS1 b = *((txS1*)q);
	return (a < b) ? -1 : (a > b) ? 1 : 0;
}

void fxInt8Getter(txMachine* the, txSlot* data, txInteger offset, txSlot* slot, int endian)
{
	slot->kind = XS_INTEGER_KIND;
	slot->value.integer = *((txS1*)(data->value.arrayBuffer.address + offset));
}

void fxInt8Setter(txMachine* the, txSlot* data, txInteger offset, txSlot* slot, int endian)
{
	*((txS1*)(data->value.arrayBuffer.address + offset)) = (txS1)fxToInteger(the, slot);
}

int fxInt16Compare(const void* p, const void* q)
{
	txS2 a = *((txS2*)p);
	txS2 b = *((txS2*)q);
	return (a < b) ? -1 : (a > b) ? 1 : 0;
}

void fxInt16Getter(txMachine* the, txSlot* data, txInteger offset, txSlot* slot, int endian)
{
	txS2 value;
	slot->kind = XS_INTEGER_KIND;
	value = *((txS2*)(data->value.arrayBuffer.address + offset));
	slot->value.integer = IMPORT(S16);
}

void fxInt16Setter(txMachine* the, txSlot* data, txInteger offset, txSlot* slot, int endian)
{
	txS2 value = (txS2)fxToInteger(the, slot);
	*((txS2*)(data->value.arrayBuffer.address + offset)) = EXPORT(S16);
}

int fxInt32Compare(const void* p, const void* q)
{
	txS4 a = *((txS4*)p);
	txS4 b = *((txS4*)q);
	return (a < b) ? -1 : (a > b) ? 1 : 0;
}

void fxInt32Getter(txMachine* the, txSlot* data, txInteger offset, txSlot* slot, int endian)
{
	txS4 value;
	slot->kind = XS_INTEGER_KIND;
	value = *((txS4*)(data->value.arrayBuffer.address + offset));
	slot->value.integer = IMPORT(S32);
}

void fxInt32Setter(txMachine* the, txSlot* data, txInteger offset, txSlot* slot, int endian)
{
	txS4 value = (txS4)fxToInteger(the, slot);
	*((txS4*)(data->value.arrayBuffer.address + offset)) = EXPORT(S32);
}

int fxUint8Compare(const void* p, const void* q)
{
	txU1 a = *((txU1*)p);
	txU1 b = *((txU1*)q);
	return (a < b) ? -1 : (a > b) ? 1 : 0;
}

void fxUint8Getter(txMachine* the, txSlot* data, txInteger offset, txSlot* slot, int endian)
{
	slot->kind = XS_INTEGER_KIND;
	slot->value.integer = *((txU1*)(data->value.arrayBuffer.address + offset));
}

void fxUint8Setter(txMachine* the, txSlot* data, txInteger offset, txSlot* slot, int endian)
{
	*((txU1*)(data->value.arrayBuffer.address + offset)) = (txU1)fxToUnsigned(the, slot);
}

int fxUint16Compare(const void* p, const void* q)
{
	txU2 a = *((txU2*)p);
	txU2 b = *((txU2*)q);
	return (a < b) ? -1 : (a > b) ? 1 : 0;
}

void fxUint16Getter(txMachine* the, txSlot* data, txInteger offset, txSlot* slot, int endian)
{
	txU2 value;
	slot->kind = XS_INTEGER_KIND;
	value = *((txU2*)(data->value.arrayBuffer.address + offset));
	slot->value.integer = IMPORT(U16);
}

void fxUint16Setter(txMachine* the, txSlot* data, txInteger offset, txSlot* slot, int endian)
{
	txU2 value = (txU2)fxToUnsigned(the, slot);
	*((txU2*)(data->value.arrayBuffer.address + offset)) = EXPORT(U16);
}

int fxUint32Compare(const void* p, const void* q)
{
	txU4 a = *((txU4*)p);
	txU4 b = *((txU4*)q);
	return (a < b) ? -1 : (a > b) ? 1 : 0;
}

void fxUint32Getter(txMachine* the, txSlot* data, txInteger offset, txSlot* slot, int endian)
{
	txUnsigned value = *((txU4*)(data->value.arrayBuffer.address + offset));
	value = IMPORT(U32);
	if (((txInteger)value) >= 0) {
		slot->kind = XS_INTEGER_KIND;
		slot->value.integer = value;
	}
	else {
		slot->kind = XS_NUMBER_KIND;
		slot->value.number = value;
	}
}

void fxUint32Setter(txMachine* the, txSlot* data, txInteger offset, txSlot* slot, int endian)
{
	txU4 value = (txU4)fxToUnsigned(the, slot);
	*((txU4*)(data->value.arrayBuffer.address + offset)) = EXPORT(U32);
}

void fxUint8ClampedSetter(txMachine* the, txSlot* data, txInteger offset, txSlot* slot, int endian)
{
	txInteger aValue = fxToInteger(the, slot);
	if (aValue < 0) aValue = 0;
	else if (aValue > 255) aValue = 255;
	*((txU1*)(data->value.arrayBuffer.address + offset)) = (txU1)aValue;
}

