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

static txSlot* fxNewGlobalInstance(txMachine* the);
static void fx_Iterator_iterator(txMachine* the);
static void fx_decodeURI(txMachine* the);
static void fx_decodeURIComponent(txMachine* the);
static void fx_encodeURI(txMachine* the);
static void fx_encodeURIComponent(txMachine* the);
static void fx_escape(txMachine* the);
static void fx_eval(txMachine* the);
static void fx_trace(txMachine* the);
static void fx_unescape(txMachine* the);

static const char gxURIEmptySet[128] = {
  /* 0 1 2 3 4 5 6 7 8 9 A B C D E F */
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 0x                    */
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 1x                    */
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 2x   !"#$%&'()*+,-./  */
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 3x  0123456789:;<=>?  */
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 4x  @ABCDEFGHIJKLMNO  */
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 5X  PQRSTUVWXYZ[\]^_  */
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 6x  `abcdefghijklmno  */
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 	/* 7X  pqrstuvwxyz{|}~   */
};

static const char gxURIReservedSet[128] = {
  /* 0 1 2 3 4 5 6 7 8 9 A B C D E F */
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 0x                    */
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 1x                    */
	 0,0,0,1,1,0,1,0,0,0,0,1,1,0,0,1,	/* 2x   !"#$%&'()*+,-./  */
	 0,0,0,0,0,0,0,0,0,0,1,1,0,1,0,1,	/* 3x  0123456789:;<=>?  */
	 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 4x  @ABCDEFGHIJKLMNO  */
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 5X  PQRSTUVWXYZ[\]^_  */
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 6x  `abcdefghijklmno  */
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 	/* 7X  pqrstuvwxyz{|}~   */
};

static const char gxURIUnescapedSet[128] = {
  /* 0 1 2 3 4 5 6 7 8 9 A B C D E F */
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 0x                    */
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 1x                    */
	 0,1,0,0,0,0,0,1,1,1,1,0,0,1,1,0,	/* 2x   !"#$%&'()*+,-./  */
	 1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,	/* 3x  0123456789:;<=>?  */
	 0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,	/* 4x  @ABCDEFGHIJKLMNO  */
	 1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,1,	/* 5X  PQRSTUVWXYZ[\]^_  */
	 0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,	/* 6x  `abcdefghijklmno  */
	 1,1,1,1,1,1,1,1,1,1,1,0,0,0,1,0 	/* 7X  pqrstuvwxyz{|}~   */
};

static const char gxURIReservedAndUnescapedSet[128] = {
  /* 0 1 2 3 4 5 6 7 8 9 A B C D E F */
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 0x                    */
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 1x                    */
	 0,1,0,1,1,0,1,1,1,1,1,1,1,1,1,1,	/* 2x   !"#$%&'()*+,-./  */
	 1,1,1,1,1,1,1,1,1,1,1,1,0,1,0,1,	/* 3x  0123456789:;<=>?  */
	 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,	/* 4x  @ABCDEFGHIJKLMNO  */
	 1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,1,	/* 5X  PQRSTUVWXYZ[\]^_  */
	 0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,	/* 6x  `abcdefghijklmno  */
	 1,1,1,1,1,1,1,1,1,1,1,0,0,0,1,0 	/* 7X  pqrstuvwxyz{|}~   */
};

static txS1* gxURIHexa = (txS1*)"0123456789ABCDEF";

#define mxURIXDigit(X) \
	((('0' <= (X)) && ((X) <= '9')) \
		? ((X) - '0') \
		: ((('a' <= (X)) && ((X) <= 'f')) \
			? (10 + (X) - 'a') \
			: (10 + (X) - 'A')))

void fxBuildGlobal(txMachine* the)
{
    static const txHostFunctionBuilder gx_global_builders[] = {
		{ fx_decodeURI, 1, _decodeURI },
		{ fx_decodeURIComponent, 1, _decodeURIComponent },
		{ fx_encodeURI, 1, _encodeURI },
		{ fx_encodeURIComponent, 1, _encodeURIComponent },
		{ fx_escape, 1, _escape },
		{ fx_eval, 1, _eval },
		{ fx_trace, 1, _trace },
		{ fx_unescape, 1, _unescape },
		{ C_NULL, 0, 0 },
    };
    const txHostFunctionBuilder* builder;
	txSlot* slot;
	
	fxNewGlobalInstance(the);
	mxPull(mxGlobal);

	fxNewInstance(the);
	mxPull(mxObjectPrototype);
	
	mxPush(mxObjectPrototype);
	fxNewFunctionInstance(the, XS_NO_ID);
	mxPull(mxFunctionPrototype);
	
	for (builder = gx_global_builders; builder->callback; builder++) {
		fxNewHostFunctionGlobal(the, builder->callback, builder->length, mxID(builder->id), XS_DONT_ENUM_FLAG);
		the->stack++;
	}
	slot = fxSetGlobalProperty(the, mxGlobal.value.reference, mxID(_undefined), C_NULL);
	slot->flag = XS_GET_ONLY;
	
	mxPush(mxObjectPrototype);
	slot = fxNewObjectInstance(the);
	slot = fxNextHostFunctionProperty(the, slot, fx_Iterator_iterator, 0, mxID(_Symbol_iterator), XS_DONT_ENUM_FLAG);
	mxPull(mxIteratorPrototype);
}

txSlot* fxNewGlobalInstance(txMachine* the)
{
	txSlot* instance;
	txSlot* property;
	instance = fxNewInstance(the);
	instance->flag |= XS_VALUE_FLAG;
	property = instance->next = fxNewSlot(the);
	property->value.table.address = (txSlot**)fxNewChunk(the, the->keyCount * sizeof(txSlot*));
	c_memset(property->value.table.address, 0, the->keyCount * sizeof(txSlot*));
	property->value.table.length = the->keyCount;
	property->kind = XS_GLOBAL_KIND;
	return instance;
}

txSlot* fxNewHostAccessorGlobal(txMachine* the, txCallback get, txCallback set, txID id, txFlag flag)
{
	txSlot *getter = NULL, *setter = NULL, *property;
	if (get)
		getter = fxNewHostFunction(the, get, 0, id);
	if (set)
		getter = fxNewHostFunction(the, set, 1, id);
	property = fxSetGlobalProperty(the, mxGlobal.value.reference, id, C_NULL);
	property->flag = flag;
	property->kind = XS_ACCESSOR_KIND;
	property->value.accessor.getter = getter;
	property->value.accessor.setter = setter;
	if (set)
		the->stack++;
	if (get)
		the->stack++;
	return property;
}

txSlot* fxNewHostConstructorGlobal(txMachine* the, txCallback call, txInteger length, txID id, txFlag flag)
{
	txSlot *function, *property;
	function = fxNewHostConstructor(the, call, length, id);
	property = fxSetGlobalProperty(the, mxGlobal.value.reference, id, C_NULL);
	property->flag = flag;
	property->kind = the->stack->kind;
	property->value = the->stack->value;
	return function;
}

txSlot* fxNewHostFunctionGlobal(txMachine* the, txCallback call, txInteger length, txID id, txFlag flag)
{
	txSlot *function, *property;
	function = fxNewHostFunction(the, call, length, id);
	property = fxSetGlobalProperty(the, mxGlobal.value.reference, id, C_NULL);
	property->flag = flag;
	property->kind = the->stack->kind;
	property->value = the->stack->value;
	return function;
}


txSlot* fxNewIteratorInstance(txMachine* the, txSlot* iterable) 
{
	txSlot* instance;
	txSlot* result;
	txSlot* property;
	instance = fxNewObjectInstance(the);
	mxPush(mxObjectPrototype);
	result = fxNewObjectInstance(the);
	property = fxNextUndefinedProperty(the, result, mxID(_value), XS_DONT_DELETE_FLAG | XS_DONT_SET_FLAG);
	property = fxNextBooleanProperty(the, property, 0, mxID(_done), XS_DONT_DELETE_FLAG | XS_DONT_SET_FLAG);
	property = fxNextSlotProperty(the, instance, the->stack, mxID(_result), XS_GET_ONLY);
	property = fxNextSlotProperty(the, property, iterable, mxID(_iterable), XS_GET_ONLY);
	property = fxNextIntegerProperty(the, property, 0, mxID(_index), XS_GET_ONLY);
    the->stack++;
	return instance;
}

void fx_Iterator_iterator(txMachine* the)
{
	*mxResult = *mxThis;
}

void fx_decodeURI(txMachine* the)
{
	if (mxArgc < 1)
		mxSyntaxError("no URI parameter");
	fxToString(the, mxArgv(0));
	fxDecodeURI(the, (txString)gxURIReservedSet);
}

void fx_decodeURIComponent(txMachine* the)
{
	if (mxArgc < 1)
		mxSyntaxError("no URI Component parameter");
	fxToString(the, mxArgv(0));
	fxDecodeURI(the, (txString)gxURIEmptySet);
}

void fx_encodeURI(txMachine* the)
{
	if (mxArgc < 1)
		mxSyntaxError("no URI parameter");
	fxEncodeURI(the, (txString)gxURIReservedAndUnescapedSet);
}

void fx_encodeURIComponent(txMachine* the)
{
	if (mxArgc < 1)
		mxSyntaxError("no URI Component parameter");
	fxEncodeURI(the, (txString)gxURIUnescapedSet);
}

void fx_escape(txMachine* the)
{
	static char gxSet[128] = {
	  /* 0 1 2 3 4 5 6 7 8 9 A B C D E F */
		 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 0x                    */
		 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 1x                    */
		 0,0,0,0,0,0,0,0,0,0,1,1,0,1,1,1,	/* 2x   !"#$%&'()*+,-./  */
		 1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,	/* 3x  0123456789:;<=>?  */
		 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,	/* 4x  @ABCDEFGHIJKLMNO  */
		 1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,1,	/* 5X  PQRSTUVWXYZ[\]^_  */
		 0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,	/* 6x  `abcdefghijklmno  */
		 1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0 	/* 7X  pqrstuvwxyz{|}~   */
	};
	txString string;
	txU1 *src;
	txInteger length;
	txU4 c;
	const txUTF8Sequence *sequence;
	txInteger size;
	txString result;
	txU1 *dst;
	
	if (mxArgc < 1)
		string = mxUndefinedString.value.string;
	else {
		string = fxToString(the, mxArgv(0));
	}
	src = (txU1*)string;
	length = 0;
	while ((c = *src++)) {
		for (sequence = gxUTF8Sequences; sequence->size; sequence++) {
			if ((c & sequence->cmask) == sequence->cval)
				break;
		}
		size = sequence->size - 1;
		while (size > 0) {
			size--;
			c = (c << 6) | (*src++ & 0x3F);
		}
		c &= sequence->lmask;
		if ((c < 128) && (gxSet[(int)c]))
			length += 1;
		else if (c < 256)
			length += 3;
		else
			length += 6;
	}
	length += 1;
	if (length == (src - (txU1*)string)) {
		mxResult->value.string = string;
		mxResult->kind = XS_STRING_KIND;
		return;
	}
	result = fxNewChunk(the, length);
	src = (txU1*)string;
	dst = (txU1*)result;
	while ((c = *src++)) {
		for (sequence = gxUTF8Sequences; sequence->size; sequence++) {
			if ((c & sequence->cmask) == sequence->cval)
				break;
		}
		size = sequence->size - 1;
		while (size > 0) {
			size--;
			c = (c << 6) | (*src++ & 0x3F);
		}
		c &= sequence->lmask;
		if ((c < 128) && (gxSet[(int)c]))
			*dst++ = (txU1)c;
		else if (c < 256) {
			*dst++ = '%';
			*dst++ = gxURIHexa[c >> 4];
			*dst++ = gxURIHexa[c & 15];
		}
		else {
			*dst++ = '%';
			*dst++ = 'u';
			*dst++ = gxURIHexa[c >> 12];
			*dst++ = gxURIHexa[(c >> 8) & 15];
			*dst++ = gxURIHexa[(c >> 4) & 15];
			*dst++ = gxURIHexa[c & 15];
		}
	}
	*dst = 0;
	mxResult->value.string = (char *)result;
	mxResult->kind = XS_STRING_KIND;
}

void fx_eval(txMachine* the)
{
	txStringStream aStream;

	if (mxArgc < 1)
		return;
	if (!mxIsStringPrimitive(mxArgv(0))) {
		*mxResult = *mxArgv(0);
		return;
	}
	aStream.slot = mxArgv(0);
	aStream.offset = 0;
	aStream.size = c_strlen(fxToString(the, mxArgv(0)));
	fxRunScript(the, fxParseScript(the, &aStream, fxStringGetter, mxProgramFlag), &mxGlobal, C_NULL, C_NULL, C_NULL);
	mxPullSlot(mxResult);
}

void fx_trace(txMachine* the)
{
#if mxDebug
	if (mxArgc > 0) {
		fxToString(the, mxArgv(0));
		fxReport(the, "%s", mxArgv(0)->value.string);
	}
#endif
}

void fx_unescape(txMachine* the)
{
	txString string;
	txU1 *src;
	txInteger length;
	txU4 c, d;
	txString result;
	txU1 *dst;

	if (mxArgc < 1)
		string = mxUndefinedString.value.string;
	else {
		string = fxToString(the, mxArgv(0));
	}
	src = (txU1*)string;
	length = 0;
	while ((c = *src++)) {
		if (c == '%') {
			c = *src++;
			if (c == 'u') {
				c = *src++;
				if (c == 0)
					mxURIError("invalid URI");
				d = mxURIXDigit(c) << 12;
				c = *src++;
				if (c == 0)
					mxURIError("invalid URI");
				d += mxURIXDigit(c) << 8;
				c = *src++;
			}
			else 
				d = 0;
			if (c == 0)
				mxURIError("invalid URI");
			d += mxURIXDigit(c) << 4;
			c = *src++;
			if (c == 0)
				mxURIError("invalid URI");
			d += mxURIXDigit(c);
			if (d < 0x80) {
				length += 1;
			}
			else if (d < 0x800) {
				length += 2;
			}
			else if (d < 0x10000) {
				length += 3;
			}
			else if (d < 0x200000) {
				length += 4;
			}
		}
		else
			length += 1;
	}		
	length += 1;
	if (length == (src - (txU1*)string)) {
		mxResult->value.string = string;
		mxResult->kind = XS_STRING_KIND;
		return;
	}
	result = fxNewChunk(the, length);
	src = (txU1*)string;
	dst = (txU1*)result;
	while ((c = *src++)) {
		if (c == '%') {
			c = *src++;
			if (c == 'u') {
				c = *src++;
				d = mxURIXDigit(c) << 12;
				c = *src++;
				d += mxURIXDigit(c) << 8;
				c = *src++;
			}
			else 
				d = 0;
			d += mxURIXDigit(c) << 4;
			c = *src++;
			d += mxURIXDigit(c);
			if (d < 0x80) {
				*dst++ = (txU1)d;
			}
			else if (d < 0x800) {
				*dst++ = (txU1)(0xC0 | (d >> 6));
				*dst++ = (txU1)(0x80 | (d & 0x3F));
			}
			else if (d < 0x10000) {
				*dst++ = (txU1)(0xE0 | (d >> 12));
				*dst++ = (txU1)(0x80 | ((d >> 6) & 0x3F));
				*dst++ = (txU1)(0x80 | (d & 0x3F));
			}
			else if (d < 0x200000) {
				*dst++ = (txU1)(0xF0 | (d >> 18));
				*dst++ = (txU1)(0x80 | ((d >> 12) & 0x3F));
				*dst++ = (txU1)(0x80 | ((d >> 6) & 0x3F));
				*dst++ = (txU1)(0x80 | (d  & 0x3F));
			}
		}
		else
			*dst++ = (txU1)c;
	}
	*dst = 0;
	mxResult->value.string = (char *)result;
	mxResult->kind = XS_STRING_KIND;
}
			
void fxDecodeURI(txMachine* the, txString theSet)
{
	txU1* src;
	txS4 size;
	txU1 c, d;
	txS1 *result;
	txU1* dst;
	
	src = (txU1*)mxArgv(0)->value.string;
	size = 0;
	while ((c = *src++)) {
		if (c == '%') {
			c = *src++;
			if (c == 0)
				mxURIError("invalid URI");
			d = mxURIXDigit(c) << 4;
			c = *src++;
			if (c == 0)
				mxURIError("invalid URI");
			d += mxURIXDigit(c);
			if ((d < 128) && (theSet[(int)d]))
				size += 3;
			else
				size += 1;
		}
		else
			size += 1;
	}		
	size += 1;

	if (size == (src - (txU1*)mxArgv(0)->value.string)) {
		mxResult->value.string = mxArgv(0)->value.string;
		mxResult->kind = XS_STRING_KIND;
		return;
	}

	result = fxNewChunk(the, size);
	
	src = (txU1*)mxArgv(0)->value.string;
	dst = (txU1*)result;
	while ((c = *src++)) {
		if (c == '%') {
			c = *src++;
			d = mxURIXDigit(c) << 4;
			c = *src++;
			d += mxURIXDigit(c);
			if ((d < 128) && (theSet[(int)d])) {
				*dst++ = *(src - 3);
				*dst++ = *(src - 2);
				*dst++ = *(src - 1);
			}
			else
				*dst++ = d;
		}
		else
			*dst++ = c;
	}
	*dst = 0;
	
	mxResult->value.string = (char *)result;
	mxResult->kind = XS_STRING_KIND;
}

void fxEncodeURI(txMachine* the, txString theSet)
{
	txU1* src;
	txS4 size;
	txU1 c;
	txS1 *result;
	txU1* dst;

	fxToString(the, mxArgv(0));

	src = (txU1*)mxArgv(0)->value.string;
	size = 0;
	while ((c = *src++)) {
		if ((c < 128) && (theSet[(int)c]))
			size += 1;
		else
			size += 3;
	}
	size += 1;

	if (size == (src - (txU1*)mxArgv(0)->value.string)) {
		mxResult->value.string = mxArgv(0)->value.string;
		mxResult->kind = XS_STRING_KIND;
		return;
	}

	result = fxNewChunk(the, size);

	src = (txU1*)mxArgv(0)->value.string;
	dst = (txU1*)result;
	while ((c = *src++)) {
		if ((c < 128) && (theSet[(int)c]))
			*dst++ = c;
		else {
			*dst++ = '%';
			*dst++ = gxURIHexa[c >> 4];
			*dst++ = gxURIHexa[c & 15];
		}
	}
	*dst = 0;
	
	mxResult->value.string = (char *)result;
	mxResult->kind = XS_STRING_KIND;
}



