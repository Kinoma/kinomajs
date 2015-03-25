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

#ifndef mxBoundsCheck
	#define mxBoundsCheck 1
#endif

#define	XS_PROFILE_COUNT (256 * 1024)

#ifdef mxDebug
static void fxDebugC(txMachine* the, txString thePath, txInteger theLine);
#endif
static void fxBeginScan(txMachine* the, txFlag theFlag);
static void fxEndScan(txMachine* the);

/* Slot */

txKind fxTypeOf(txMachine* the, txSlot* theSlot)
{
	if (theSlot->kind == XS_ALIAS_KIND)
		return XS_REFERENCE_KIND;
	else
		return theSlot->kind;
}

/* Primitives */

void fxUndefined(txMachine* the, txSlot* theSlot)
{
#if !mxOptimize
	mxZeroSlot(theSlot);
	theSlot->kind = XS_UNDEFINED_KIND;
#else
	txSize* dst = (txSize*)(theSlot);
	*dst++ = 0;
	*dst++ = 0;
	theSlot->kind = XS_UNDEFINED_KIND;
#endif
}

void fxNull(txMachine* the, txSlot* theSlot)
{
#if !mxOptimize
	mxZeroSlot(theSlot);
	theSlot->kind = XS_NULL_KIND;
#else
	txSize* dst = (txSize*)(theSlot);
	*dst++ = 0;
	*dst++ = 0;
	theSlot->kind = XS_NULL_KIND;
#endif
}

void fxBoolean(txMachine* the, txSlot* theSlot, txS1 theValue)
{
#if !mxOptimize
	mxZeroSlot(theSlot);
	theSlot->value.boolean = theValue;
	theSlot->kind = XS_BOOLEAN_KIND;
#else
	txSize* dst = (txSize*)(theSlot);
	*dst++ = 0;
	*dst++ = 0;
	theSlot->kind = XS_BOOLEAN_KIND;
	theSlot->value.boolean = theValue;
#endif
}

txS1 fxToBoolean(txMachine* the, txSlot* theSlot)
{
	switch (theSlot->kind) {
	case XS_UNDEFINED_KIND:
	case XS_NULL_KIND:
		theSlot->kind = XS_BOOLEAN_KIND;
		theSlot->value.boolean = 0;
		break;
	case XS_BOOLEAN_KIND:
		break;
	case XS_INTEGER_KIND:
		theSlot->kind = XS_BOOLEAN_KIND;
		theSlot->value.boolean = (theSlot->value.integer == 0) ? 0 : 1;
		break;
	case XS_NUMBER_KIND:
		theSlot->kind = XS_BOOLEAN_KIND;
		switch (c_fpclassify(theSlot->value.number)) {
		case FP_NAN:
		case FP_ZERO:
			theSlot->value.boolean = 0;
			break;
		default:
			theSlot->value.boolean = 1;
			break;
		}
		break;
	case XS_STRING_KIND:
		theSlot->kind = XS_BOOLEAN_KIND;
		if (c_strlen(theSlot->value.string) == 0)
			theSlot->value.boolean = 0;
		else
			theSlot->value.boolean = 1;
		break;
	case XS_REFERENCE_KIND:
	case XS_ALIAS_KIND:
		theSlot->kind = XS_BOOLEAN_KIND;
		theSlot->value.boolean = 1;
		break;
	default:
		mxDebug0(the, XS_TYPE_ERROR, "Cannot coerce to boolean");
		break;
	}
	return (txS1)theSlot->value.boolean;
}

void fxInteger(txMachine* the, txSlot* theSlot, txInteger theValue)
{
#if !mxOptimize
	mxZeroSlot(theSlot);
	theSlot->value.integer = theValue;
	theSlot->kind = XS_INTEGER_KIND;
#else
	txSize* dst = (txSize*)(theSlot);
	*dst++ = 0;
	*dst++ = 0;
	theSlot->kind = XS_INTEGER_KIND;
	theSlot->value.integer = theValue;
#endif
}

txInteger fxToInteger(txMachine* the, txSlot* theSlot)
{
#if mxOptimize
	if (XS_INTEGER_KIND == theSlot->kind)
		return theSlot->value.integer;				// this is the case over 90% of the time, so avoid the switch
#endif

again:
	switch (theSlot->kind) {
	case XS_UNDEFINED_KIND:
	case XS_NULL_KIND:
		theSlot->kind = XS_INTEGER_KIND;
		theSlot->value.integer = 0;
		break;
	case XS_BOOLEAN_KIND:
		theSlot->kind = XS_INTEGER_KIND;
		if (theSlot->value.boolean == 0)
			theSlot->value.integer = 0;
		else
			theSlot->value.integer = 1;
		break;
	case XS_INTEGER_KIND:
		break;
	case XS_NUMBER_KIND:
		theSlot->kind = XS_INTEGER_KIND;
		switch (c_fpclassify(theSlot->value.number)) {
		case C_FP_INFINITE:
		case C_FP_NAN:
		case C_FP_ZERO:
			theSlot->value.integer = 0;
			break;
		default: {
			#define MODULO 4294967296.0
			txNumber aNumber = fmod(theSlot->value.number, MODULO);
			if (aNumber >= MODULO / 2)
				aNumber -= MODULO;
			else if (aNumber < -MODULO / 2)
				aNumber += MODULO;
			theSlot->value.integer = (txInteger)aNumber;
			} break;
		}
		break;
	case XS_STRING_KIND:
		theSlot->kind = XS_NUMBER_KIND;
		theSlot->value.number = fxStringToNumber(the, theSlot->value.string, 1);
		goto again;
	case XS_REFERENCE_KIND:
	case XS_ALIAS_KIND:
		fxToPrimitive(the, theSlot, XS_NUMBER_HINT);
		goto again;
	default:
		mxDebug0(the, XS_TYPE_ERROR, "Cannot coerce to integer");
		break;
	}
	return theSlot->value.integer;
}

void fxNumber(txMachine* the, txSlot* theSlot, txNumber theValue)
{
	mxZeroSlot(theSlot);
	theSlot->value.number = theValue;
	theSlot->kind = XS_NUMBER_KIND;
}

txNumber fxToNumber(txMachine* the, txSlot* theSlot)
{
again:
	switch (theSlot->kind) {
	case XS_UNDEFINED_KIND:
		theSlot->kind = XS_NUMBER_KIND;
		theSlot->value.number = C_NAN;
		break;
	case XS_NULL_KIND:
		theSlot->kind = XS_NUMBER_KIND;
		theSlot->value.number = 0;
		break;
	case XS_BOOLEAN_KIND:
		theSlot->kind = XS_NUMBER_KIND;
		if (theSlot->value.boolean == 0)
			theSlot->value.number = 0;
		else
			theSlot->value.number = 1;
		break;
	case XS_INTEGER_KIND:
		theSlot->kind = XS_NUMBER_KIND;
		theSlot->value.number = theSlot->value.integer;
		break;
	case XS_NUMBER_KIND:
		break;
	case XS_STRING_KIND:
		theSlot->kind = XS_NUMBER_KIND;
		theSlot->value.number = fxStringToNumber(the, theSlot->value.string, 1);
		break;
	case XS_REFERENCE_KIND:
	case XS_ALIAS_KIND:
		fxToPrimitive(the, theSlot, XS_NUMBER_HINT);
		goto again;
	default:
		mxDebug0(the, XS_TYPE_ERROR, "Cannot coerce to number");
		break;
	}
	return theSlot->value.number;
}

void fxString(txMachine* the, txSlot* theSlot, txString theValue)
{
	mxZeroSlot(theSlot);
	fxCopyStringC(the, theSlot, theValue);
}

void fxStringBuffer(txMachine* the, txSlot* theSlot, txString theValue, txSize theSize)
{
	mxZeroSlot(theSlot);
	theSlot->value.string = (txString)fxNewChunk(the, theSize + 1);
	c_memcpy(theSlot->value.string, theValue, theSize);
	theSlot->value.string[theSize] = 0;
	theSlot->kind = XS_STRING_KIND;
}

txString fxToString(txMachine* the, txSlot* theSlot)
{
	char aBuffer[256];
again:
	switch (theSlot->kind) {
	case XS_UNDEFINED_KIND:
		fxCopyStringC(the, theSlot, "undefined");
		break;
	case XS_NULL_KIND:
		fxCopyStringC(the, theSlot, "null");
		break;
	case XS_BOOLEAN_KIND:
		if (theSlot->value.boolean == 0)
			fxCopyStringC(the, theSlot, "false");
		else
			fxCopyStringC(the, theSlot, "true");
		break;
	case XS_INTEGER_KIND:
		fxCopyStringC(the, theSlot, fxIntegerToString(theSlot->value.integer, aBuffer, sizeof(aBuffer)));
		break;
	case XS_NUMBER_KIND:
		fxCopyStringC(the, theSlot, fxNumberToString(the, theSlot->value.number, aBuffer, sizeof(aBuffer), 0, 0));
		break;
	case XS_STRING_KIND:
		break;
	case XS_REFERENCE_KIND:
	case XS_ALIAS_KIND:
		fxToPrimitive(the, theSlot, XS_STRING_HINT);
		goto again;
	default:
		mxDebug0(the, XS_TYPE_ERROR, "Cannot coerce to string");
		break;
	}
	return theSlot->value.string;
}

txString fxToStringBuffer(txMachine* the, txSlot* theSlot, txString theBuffer, txSize theSize)
{
	char* aString;
	txSize aSize;

	aString = fxToString(the, theSlot);
	aSize = c_strlen(aString) + 1;
	if (aSize > theSize)
		mxDebug0(the, XS_RANGE_ERROR, "Cannot buffer string");
	c_memcpy(theBuffer, aString, aSize);
	return theBuffer;
}

void fxUnsigned(txMachine* the, txSlot* theSlot, txUnsigned theValue)
{
	mxZeroSlot(theSlot);
	if (((txInteger)theValue) >= 0) {
		theSlot->value.integer = theValue;
		theSlot->kind = XS_INTEGER_KIND;
	}
	else {
		theSlot->value.number = theValue;
		theSlot->kind = XS_NUMBER_KIND;
	}
}

txUnsigned fxToUnsigned(txMachine* the, txSlot* theSlot)
{
	txUnsigned result;
again:
	switch (theSlot->kind) {
	case XS_UNDEFINED_KIND:
	case XS_NULL_KIND:
		theSlot->kind = XS_INTEGER_KIND;
		result = theSlot->value.integer = 0;
		break;
	case XS_BOOLEAN_KIND:
		theSlot->kind = XS_INTEGER_KIND;
		if (theSlot->value.boolean == 0)
			result = theSlot->value.integer = 0;
		else
			result = theSlot->value.integer = 1;
		break;
	case XS_INTEGER_KIND:
		if (theSlot->value.integer < 0)
			theSlot->value.integer = -theSlot->value.integer;
		result = theSlot->value.integer;
		break;
	case XS_NUMBER_KIND:
		theSlot->kind = XS_INTEGER_KIND;
		switch (c_fpclassify(theSlot->value.number)) {
		case C_FP_INFINITE:
		case C_FP_NAN:
		case C_FP_ZERO:
			result = theSlot->value.integer = 0;
			break;
		default: {
			#define MODULO 4294967296.0
			txNumber aNumber = fmod(theSlot->value.number, MODULO);
			if (aNumber < 0)
				aNumber += MODULO;
			result = (txUnsigned)aNumber;
			fxUnsigned(the, theSlot, result);
			} break;
		}
		break;
	case XS_STRING_KIND:
		theSlot->kind = XS_NUMBER_KIND;
		theSlot->value.number = fxStringToNumber(the, theSlot->value.string, 1);
		goto again;
	case XS_REFERENCE_KIND:
	case XS_ALIAS_KIND:
		fxToPrimitive(the, theSlot, XS_NUMBER_HINT);
		goto again;
	default:
		result = theSlot->value.integer = 0;
		mxDebug0(the, XS_TYPE_ERROR, "Cannot coerce to unsigned");
		break;
	}
	return result;
}

/* Instances and Prototypes */

void fxNewInstanceOf(txMachine* the)
{
	txSlot* aParent;

	fxToInstance(the, the->stack);
	aParent = fxGetInstance(the, the->stack);
	if (aParent->flag & XS_VALUE_FLAG) {
		switch (aParent->next->kind) {
		case XS_CALLBACK_KIND:
		case XS_CODE_KIND:
			fxNewFunctionInstance(the);
			break;
		case XS_ARRAY_KIND:
			fxNewArrayInstance(the);
			break;
		case XS_STRING_KIND:
			fxNewStringInstance(the);
			break;
		case XS_BOOLEAN_KIND:
			fxNewBooleanInstance(the);
			break;
		case XS_NUMBER_KIND:
			fxNewNumberInstance(the);
			break;
		case XS_DATE_KIND:
			fxNewDateInstance(the);
			break;
		case XS_REGEXP_KIND:
			fxNewRegExpInstance(the);
			break;
		case XS_HOST_KIND:
			if ((aParent->next->next) && (aParent->next->next->ID == the->chunkID))
				fxNewChunkInstance(the);
			else
				fxNewHostInstance(the);
			break;
		default:
			mxDebug0(the, XS_SYNTAX_ERROR, "C: xsNewInstanceOf: invalid prototype");
			break;
		}
	}
	else
		fxNewObjectInstance(the);
}

txBoolean fxIsInstanceOf(txMachine* the)
{
	txBoolean result = 0;
	txSlot* theInstance = the->stack++;
	txSlot* thePrototype = the->stack++;

	if (mxIsReference(theInstance) && mxIsReference(thePrototype)) {
		theInstance	= fxGetInstance(the, theInstance);
		thePrototype = fxGetInstance(the, thePrototype);
		while (theInstance) {
			if (theInstance == thePrototype) {
				result = 1;
				break;
			}
			theInstance = fxGetParent(the, theInstance);
		}
	}
	return result;
}

/* Host Constructors, Functions and Objects */

void fxNewHostConstructor(txMachine* the, txCallback theCallback, txInteger theLength)
{
	txSlot* aStack;
	txSlot* anInstance;
	txSlot* aProperty;

	fxToInstance(the, the->stack);
	aStack = the->stack;

	mxZeroSlot(--the->stack);
	anInstance = fxNewSlot(the);
	anInstance->ID = mxFunctionPrototype.value.alias;
	anInstance->flag = XS_VALUE_FLAG;
	anInstance->kind = XS_INSTANCE_KIND;
	anInstance->value.instance.garbage = C_NULL;
	anInstance->value.instance.prototype = C_NULL;
	the->stack->value.reference = anInstance;
	the->stack->kind = XS_REFERENCE_KIND;

	/* CALLBACK */
	aProperty = anInstance->next = fxNewSlot(the);
	aProperty->ID = XS_NO_ID;
	aProperty->flag = XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG;
	aProperty->kind = XS_CALLBACK_KIND;
	aProperty->value.callback.address = theCallback;
	aProperty->value.callback.length = theLength;

	/* CLOSURE */
	aProperty = aProperty->next = fxNewSlot(the);
	aProperty->flag = XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG;
	aProperty->kind = XS_CLOSURE_KIND;
	aProperty->value.closure.reference = the->scope;
	aProperty->value.closure.symbolMap = the->code;

	/* PROTOTYPE */
	aProperty = aProperty->next = fxNewSlot(the);
	aProperty->flag = XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG;
	aProperty->kind = aStack->kind;
	aProperty->value = aStack->value;

	*aStack = *the->stack;
	the->stack++;

#ifdef mxProfile
	/* PROFILE */
	aProperty = aProperty->next = fxNewSlot(the);
	aProperty->flag = XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG;
	aProperty->kind = XS_INTEGER_KIND;
	aProperty->value.integer = the->profileID;
	the->profileID++;
#endif
}

void fxNewHostFunction(txMachine* the, txCallback theCallback, txInteger theLength)
{
	txSlot* anInstance;
	txSlot* aProperty;

	mxZeroSlot(--the->stack);
	anInstance = fxNewSlot(the);
	anInstance->ID = mxFunctionPrototype.value.alias;
	anInstance->flag = XS_VALUE_FLAG;
	anInstance->kind = XS_INSTANCE_KIND;
	anInstance->value.instance.garbage = C_NULL;
	anInstance->value.instance.prototype = C_NULL;
	the->stack->value.reference = anInstance;
	the->stack->kind = XS_REFERENCE_KIND;

	/* CALLBACK */
	aProperty = anInstance->next = fxNewSlot(the);
	aProperty->flag = XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG;
	aProperty->kind = XS_CALLBACK_KIND;
	aProperty->value.callback.address = theCallback;
	aProperty->value.callback.length = theLength;

	/* CLOSURE */
	aProperty = aProperty->next = fxNewSlot(the);
	aProperty->flag = XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG;
	aProperty->kind = XS_CLOSURE_KIND;
	if (the->scope && (the->scope->kind != XS_INSTANCE_KIND))
		fprintf(stderr, "oops\n");
	aProperty->value.closure.reference = the->scope;
	aProperty->value.closure.symbolMap = the->code;

	/* PROTOTYPE */
	aProperty = aProperty->next = fxNewSlot(the);
	aProperty->flag = XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG;
	aProperty->kind = XS_ALIAS_KIND;
	aProperty->value.alias = mxObjectPrototype.value.alias;

#ifdef mxProfile
	/* PROFILE */
	aProperty = aProperty->next = fxNewSlot(the);
	aProperty->flag = XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG;
	aProperty->kind = XS_INTEGER_KIND;
	aProperty->value.integer = the->profileID;
	the->profileID++;
#endif
}

void fxNewHostInstance(txMachine* the)
{
	txSlot* aPrototype;
	txSlot* anInstance;
	txSlot* aProperty;

	aPrototype = fxGetInstance(the, the->stack);

	anInstance = fxNewSlot(the);
	anInstance->flag = XS_VALUE_FLAG;
	anInstance->kind = XS_INSTANCE_KIND;
	anInstance->value.instance.garbage = C_NULL;
	anInstance->value.instance.prototype = C_NULL;
	if (the->stack->kind == XS_ALIAS_KIND)
		anInstance->ID = the->stack->value.alias;
	else
		anInstance->value.instance.prototype = aPrototype;
	the->stack->value.reference = anInstance;
	the->stack->kind = XS_REFERENCE_KIND;

	aProperty = anInstance->next = fxNewSlot(the);
	aProperty->ID = XS_NO_ID;
	aProperty->flag = aPrototype->next->flag;
	aProperty->kind = XS_HOST_KIND;
	aProperty->value.host.data = C_NULL;
	aProperty->value.host.variant.destructor = aPrototype->next->value.host.variant.destructor;
}

void fxNewHostObject(txMachine* the, txDestructor theDestructor)
{
	txSlot* anInstance;
	txSlot* aProperty;

	mxZeroSlot(--the->stack);

	anInstance = fxNewSlot(the);
	anInstance->ID = mxHostPrototype.value.alias;
	anInstance->flag = XS_VALUE_FLAG;
	anInstance->kind = XS_INSTANCE_KIND;
	anInstance->value.instance.garbage = C_NULL;
	anInstance->value.instance.prototype = C_NULL;
	the->stack->value.reference = anInstance;
	the->stack->kind = XS_REFERENCE_KIND;

	aProperty = anInstance->next = fxNewSlot(the);
	aProperty->ID = XS_NO_ID;
	aProperty->flag = XS_NO_FLAG;
	aProperty->kind = XS_HOST_KIND;
	aProperty->value.host.data = C_NULL;
	aProperty->value.host.variant.destructor = theDestructor;
}

void* fxGetHostData(txMachine* the, txSlot* theSlot)
{
#if !mxOptimize
	if (!mxIsReference(theSlot))
		mxDebug0(the, XS_SYNTAX_ERROR, "C: xsGetHostData: no reference");
	theSlot = fxGetInstance(the, theSlot);
	if (!mxIsHost(theSlot))
		mxDebug0(the, XS_SYNTAX_ERROR, "C: xsGetHostData: no host instance");
	return theSlot->next->value.host.data;
#else
	if (theSlot->kind == XS_REFERENCE_KIND)
		theSlot = theSlot->value.reference;
	else
	if (theSlot->kind == XS_ALIAS_KIND)
		theSlot = the->aliasArray[theSlot->value.alias];
	else {
		mxDebug0(the, XS_SYNTAX_ERROR, "C: xsGetHostData: no reference");
		return NULL;			// never happens
	}

	if (theSlot && (theSlot->flag & XS_VALUE_FLAG)) {
		theSlot = theSlot->next;
		if (theSlot->kind == XS_HOST_KIND)
			return theSlot->value.host.data;
	}
	mxDebug0(the, XS_SYNTAX_ERROR, "C: xsGetHostData: no host instance");
	return NULL;			// never happens
#endif
}

txDestructor fxGetHostDestructor(txMachine* the, txSlot* theSlot)
{
	if (!mxIsReference(theSlot))
		mxDebug0(the, XS_SYNTAX_ERROR, "C: xsGetHostDestructor: no reference");
	theSlot = fxGetInstance(the, theSlot);
	if (!mxIsHost(theSlot))
		mxDebug0(the, XS_SYNTAX_ERROR, "C: xsGetHostDestructor: no host instance");
	if (theSlot->next->flag & XS_HOST_HOOKS_FLAG)
		mxDebug0(the, XS_SYNTAX_ERROR, "C: xsGetHostDestructor: no host destructor");
	return theSlot->next->value.host.variant.destructor;
}

txHostHooks* fxGetHostHooks(txMachine* the, txSlot* theSlot)
{
	if (!mxIsReference(theSlot))
		mxDebug0(the, XS_SYNTAX_ERROR, "C: xsGetHostHooks: no reference");
	theSlot = fxGetInstance(the, theSlot);
	if (!mxIsHost(theSlot))
		mxDebug0(the, XS_SYNTAX_ERROR, "C: xsGetHostHooks: no host instance");
	if (!(theSlot->next->flag & XS_HOST_HOOKS_FLAG))
		mxDebug0(the, XS_SYNTAX_ERROR, "C: xsGetHostHooks: no host hooks");
	return theSlot->next->value.host.variant.hooks;
}

void fxSetHostData(txMachine* the, txSlot* theSlot, void* theData)
{
	if (!mxIsReference(theSlot))
		mxDebug0(the, XS_SYNTAX_ERROR, "C: xsSetHostData: no reference");
	theSlot = fxGetOwnInstance(the, theSlot);
	if (!mxIsHost(theSlot))
		mxDebug0(the, XS_SYNTAX_ERROR, "C: xsSetHostData: no host instance");
	theSlot->next->value.host.data = theData;
}

void fxSetHostDestructor(txMachine* the, txSlot* theSlot, txDestructor theDestructor)
{
	if (!mxIsReference(theSlot))
		mxDebug0(the, XS_SYNTAX_ERROR, "C: xsSetHostDestructor: no reference");
	theSlot = fxGetOwnInstance(the, theSlot);
	if (!mxIsHost(theSlot))
		mxDebug0(the, XS_SYNTAX_ERROR, "C: xsSetHostDestructor: no host instance");
	theSlot->next->flag &= ~XS_HOST_HOOKS_FLAG;
	theSlot->next->value.host.variant.destructor = theDestructor;
}

void fxSetHostHooks(txMachine* the, txSlot* theSlot, txHostHooks* theHooks)
{
	if (!mxIsReference(theSlot))
		mxDebug0(the, XS_SYNTAX_ERROR, "C: xsSetHostHooks: no reference");
	theSlot = fxGetOwnInstance(the, theSlot);
	if (!mxIsHost(theSlot))
		mxDebug0(the, XS_SYNTAX_ERROR, "C: xsSetHostHooks: no host instance");
	theSlot->next->flag |= XS_HOST_HOOKS_FLAG;
	theSlot->next->value.host.variant.hooks = theHooks;
}

/* Identifiers */

txID fxID(txMachine* the, txString theName)
{
	txSlot* aSymbol;

	aSymbol = fxNewSymbolC(the, theName);
	return aSymbol->ID;
}

txID fxFindID(txMachine* the, txString theName)
{
	txSlot* aSymbol = fxFindSymbol(the, theName);
	return aSymbol ? aSymbol->ID : 0;
}

txS1 fxIsID(txMachine* the, txString theName)
{
	return fxFindSymbol(the, theName) ? 1 : 0;
}

txString fxName(txMachine* the, txID theID)
{
	txSlot* aSymbol = fxGetSymbol(the, theID);
	if (aSymbol)
		return aSymbol->value.symbol.string;
	return C_NULL;
}

/* Properties */

txS1 fxHasID(txMachine* the, txID theID)
{
	txSlot* anInstance;
	txSlot* aProperty;

	fxToInstance(the, the->stack);
	anInstance = fxGetInstance(the, the->stack);
	aProperty = fxGetProperty(the, anInstance, theID);
	the->stack++;
	if (aProperty) {
		the->scratch.kind = aProperty->kind;
		the->scratch.value = aProperty->value;
		return 1;
	}
	fxUndefined(the, &the->scratch);
	return 0;
}

txS1 fxHasOwnID(txMachine* the, txID theID)
{
	txSlot* anInstance;
	txSlot* aProperty;

	fxToInstance(the, the->stack);
	anInstance = fxGetInstance(the, the->stack);
	aProperty = fxGetOwnProperty(the, anInstance, theID);
	the->stack++;
	if (aProperty) {
		the->scratch.kind = aProperty->kind;
		the->scratch.value = aProperty->value;
		return 1;
	}
	fxUndefined(the, &the->scratch);
	return 0;
}

void fxGet(txMachine* the, txSlot* theProperty, txID theID)
{
	txSlot* aFunction;

	if (!theProperty) {
		mxZeroSlot(the->stack);
		mxDebugID(the, XS_NO_ERROR, "C: xsGet %s: no property", theID);
	}
	else if (theProperty->kind == XS_ACCESSOR_KIND) {
		txSlot* anInstance = fxGetInstance(the, the->stack);
		mxZeroSlot(the->stack);
		the->stack->kind = XS_INTEGER_KIND;
		the->stack->value.integer = 0;
		/* THIS */
		mxZeroSlot(--the->stack);
		the->stack->value.reference = anInstance;
		the->stack->kind = XS_REFERENCE_KIND;
		/* FUNCTION */
		aFunction = theProperty->value.accessor.getter;
		if (!mxIsFunction(aFunction))
			mxDebugID(the, XS_TYPE_ERROR, "C: xsCall get %s: no function", theID);
		mxInitSlot(--the->stack, XS_REFERENCE_KIND);
		the->stack->value.reference = aFunction;
		/* RESULT */
		mxZeroSlot(--the->stack);
		fxRunID(the, theID);
	}
	else {
		mxZeroSlot(the->stack);
		the->stack->kind = theProperty->kind;
		the->stack->value = theProperty->value;
	}
}

void fxGetAt(txMachine* the)
{
	txSlot* anInstance;
	txSlot* aProperty;
	txID anID = XS_NO_ID;

	the->stack++;
	fxToInstance(the, the->stack);
	anInstance = fxGetInstance(the, the->stack);
	aProperty = fxGetPropertyAt(the, anInstance, the->stack - 1, &anID);
	fxGet(the, aProperty, anID);
}

void fxGetID(txMachine* the, txID theID)
{
	txSlot* anInstance;
	txSlot* aProperty;

	fxToInstance(the, the->stack);
	anInstance = fxGetInstance(the, the->stack);
	aProperty = fxGetProperty(the, anInstance, theID);
	fxGet(the, aProperty, theID);
}

void fxSet(txMachine* the, txSlot* theProperty, txID theID)
{
	txSlot* aFunction;

	the->stack++;
	if (!theProperty)
		mxDebugID(the, XS_TYPE_ERROR, "C: xsSet %s: no patch", theID);
	else if (theProperty->flag & XS_DONT_SET_FLAG)
		mxDebugID(the, XS_NO_ERROR, "C: xsSet %s: no permission", theID);
	else if (theProperty->kind == XS_ACCESSOR_KIND) {
		txSlot* anInstance = fxGetInstance(the, the->stack - 1);
		mxZeroSlot(--the->stack);
		the->stack->kind = XS_INTEGER_KIND;
		the->stack->value.integer = 1;
		/* THIS */
		mxZeroSlot(--the->stack);
		the->stack->value.reference = anInstance;
		the->stack->kind = XS_REFERENCE_KIND;
		/* FUNCTION */
		aFunction = theProperty->value.accessor.setter;
		if (!mxIsFunction(aFunction))
			mxDebugID(the, XS_TYPE_ERROR, "C: xsCall set %s: no function", theID);
		mxInitSlot(--the->stack, XS_REFERENCE_KIND);
		the->stack->value.reference = aFunction;
		/* RESULT */
		the->stack--;
		*(the->stack) = *(the->stack + 4);
		fxRunID(the, theID);
	}
	else {
		theProperty->kind = the->stack->kind;
		theProperty->value = the->stack->value;
	}
}

void fxSetAt(txMachine* the)
{
	txSlot* anInstance;
	txSlot* aProperty;
	txID anID = XS_NO_ID;

	fxToInstance(the, the->stack + 1);
	anInstance = fxGetOwnInstance(the, the->stack + 1);
	if (!anInstance)
		mxDebugID(the, XS_TYPE_ERROR, "set %s: shared instance", anID);
	aProperty = fxSetPropertyAt(the, anInstance, the->stack, &anID, C_NULL);
	the->stack++;
	fxSet(the, aProperty, anID);
}

void fxSetID(txMachine* the, txID theID)
{
	txSlot* anInstance;
	txSlot* aProperty;

	fxToInstance(the, the->stack);
	anInstance = fxGetOwnInstance(the, the->stack);
	if (!anInstance)
		mxDebugID(the, XS_TYPE_ERROR, "set %s: shared instance", theID);
	aProperty = fxSetProperty(the, anInstance, theID, C_NULL);
	fxSet(the, aProperty, theID);
}

void fxDeleteAt(txMachine* the)
{
	txSlot* anInstance;
	txID anID = XS_NO_ID;

	the->stack++;
	if ((the->stack->kind == XS_REFERENCE_KIND) || (the->stack->kind == XS_ALIAS_KIND)) {
		anInstance = fxGetOwnInstance(the, the->stack);
		if (!anInstance)
			mxDebugID(the, XS_TYPE_ERROR, "delete %s: shared instance", anID);
		fxRemovePropertyAt(the, anInstance, the->stack - 1, &anID);
	}
	else
		mxDebugID(the, XS_REFERENCE_ERROR, "delete %s: no instance", anID);
}

void fxDeleteID(txMachine* the, txID theID)
{
	txSlot* anInstance;

	if ((the->stack->kind == XS_REFERENCE_KIND) || (the->stack->kind == XS_ALIAS_KIND)) {
		anInstance = fxGetOwnInstance(the, the->stack);
		if (!anInstance)
			mxDebugID(the, XS_TYPE_ERROR, "delete %s: shared instance", theID);
		fxRemoveProperty(the, anInstance, theID);
	}
	else
		mxDebugID(the, XS_REFERENCE_ERROR, "delete %s: no instance", theID);
}

void fxCall(txMachine* the)
{
	txID anID;
	txSlot* aFunction;

	fxToInstance(the, the->stack + 1);
	anID = XS_NO_ID;
	aFunction = fxGetInstance(the, the->stack);
	if (!mxIsFunction(aFunction))
		mxDebugID(the, XS_TYPE_ERROR, "C: xsCall %s: no function", anID);
	/* RESULT */
	mxZeroSlot(--the->stack);
	fxRunID(the, anID);
}

void fxCallID(txMachine* the, txID theID)
{
	txSlot* anInstance;
	txSlot* aProperty;
	txSlot* aFunction;

	fxToInstance(the, the->stack);
	anInstance = fxGetInstance(the, the->stack);
	aProperty = fxGetProperty(the, anInstance, theID);
	if (!aProperty)
		mxDebugID(the, XS_TYPE_ERROR, "C: xsCall %s: no property", theID);
	if (!mxIsReference(aProperty))
		mxDebugID(the, XS_TYPE_ERROR, "C: xsCall %s: no instance", theID);
	aFunction = fxGetInstance(the, aProperty);
	if (!mxIsFunction(aFunction))
		mxDebugID(the, XS_TYPE_ERROR, "C: xsCall %s: no function", theID);
	/* FUNCTION */
	mxZeroSlot(--the->stack);
	the->stack->value.reference = aFunction;
	the->stack->kind = XS_REFERENCE_KIND;
	/* RESULT */
	mxZeroSlot(--the->stack);
	fxRunID(the, theID);
}

void fxNew(txMachine* the)
{
	txID anID;
	txSlot* aFunction;
	txSlot* aProperty;

	anID = XS_NO_ID;
	aFunction = fxGetInstance(the, the->stack);
	if (!mxIsFunction(aFunction))
		mxDebugID(the, XS_TYPE_ERROR, "C: xsNew %s: no constructor", anID);
	aProperty = aFunction->next->next->next;
	/* RESULT */
	mxZeroSlot(--the->stack);
	the->stack->value = aProperty->value;
	the->stack->kind = aProperty->kind;
	fxNewInstanceOf(the);
	*(the->stack + 2) = *(the->stack); // THIS
	fxRunID(the, anID);
}

void fxNewID(txMachine* the, txID theID)
{
	txSlot* anInstance;
	txSlot* aProperty;
	txSlot* aFunction;

	fxToInstance(the, the->stack);
	anInstance = fxGetInstance(the, the->stack);
	aProperty = fxGetProperty(the, anInstance, theID);
	if (!aProperty)
		mxDebugID(the, XS_TYPE_ERROR, "C: xsNew %s: no property", theID);
	if (!mxIsReference(aProperty))
		mxDebugID(the, XS_TYPE_ERROR, "C: xsNew %s: no instance", theID);
	aFunction = fxGetInstance(the, aProperty);
	if (!mxIsFunction(aFunction))
		mxDebugID(the, XS_TYPE_ERROR, "C: xsNew %s: no constructor", theID);
	aProperty = aFunction->next->next->next;
	the->stack->value = aProperty->value;
	the->stack->kind = aProperty->kind;
	fxNewInstanceOf(the);
	anInstance = the->stack->value.reference;
	/* FUNCTION */
	mxZeroSlot(--the->stack);
	the->stack->value.reference = aFunction;
	the->stack->kind = XS_REFERENCE_KIND;
	/* RESULT */
	mxZeroSlot(--the->stack);
	the->stack->value.reference = anInstance;
	the->stack->kind = XS_REFERENCE_KIND;
	fxRunID(the, theID);
}

/* Arguments and Variables */

void fxVars(txMachine* the, txInteger theCount)
{
	txSlot* aStack = the->frame - 1;
	if (aStack != the->stack)
		mxDebug0(the, XS_SYNTAX_ERROR, "C: xsVars: too late");
	fxOverflow(the, theCount, C_NULL, 0);
	aStack->value.integer = theCount;
	while (theCount) {
		mxZeroSlot(--the->stack);
		theCount--;
	}
}

txInteger fxCheckArg(txMachine* the, txInteger theIndex)
{
	txInteger aCount = mxArgc;
#if mxBoundsCheck
#if !mxOptimize
	if ((theIndex < 0) || (aCount <= theIndex))
		mxDebug1(the, XS_SYNTAX_ERROR, "C: xsArg(%ld): invalid index", theIndex);
	return aCount - 1 - theIndex;
#else
	aCount -= theIndex;
	if (aCount >= 0)
		return aCount - 1;

	mxDebug1(the, XS_SYNTAX_ERROR, "C: xsArg(%ld): invalid index", theIndex);
	return 0;			// never happens
#endif
#else
	return aCount - theIndex - 1;
#endif
}

txInteger fxCheckVar(txMachine* the, txInteger theIndex)
{
#if mxBoundsCheck
	if ((theIndex < 0) || (mxVarc <= theIndex))
		mxDebug1(the, XS_SYNTAX_ERROR, "C: xsVar(%ld): invalid index", theIndex);
#endif
	return theIndex;
}

#ifdef mxDebug
void fxDebugC(txMachine* the, txString thePath, txInteger theLine)
{
	if ((the->frame) && (the->frame->flag & XS_C_FLAG)) {
		(the->frame - 1)->next = (txSlot*)thePath;
		(the->frame - 1)->ID = (txID)theLine;
	}
}
#endif

void fxOverflow(txMachine* the, txInteger theCount, txString thePath, txInteger theLine)
{
#if mxBoundsCheck
	txSlot* aStack = the->stack + theCount;
#ifdef mxDebug
	fxDebugC(the, thePath, theLine);
	fxDebugLine(the);
#endif
	if (theCount < 0) {
		if (aStack < the->stackBottom)
			mxDebug1(the, XS_SYNTAX_ERROR, "stack overflow (%ld)", (the->stack - the->stackBottom) + theCount);
	}
	else if (theCount > 0) {
		if (aStack > the->stackTop)
			mxDebug1(the, XS_SYNTAX_ERROR, "stack overflow (%ld)", theCount - (the->stackTop - the->stack));
	}
#endif
}

/* Exceptions */

void fxThrow(txMachine* the, txString thePath, txInteger theLine)
{
#ifdef mxDebug
	fxDebugC(the, thePath, theLine);
	fxDebugThrow(the, "C: xsThrow");
#endif
	fxJump(the);
}

/* Errors */

void fxError(txMachine* the, txString thePath, txInteger theLine, txInteger theCode)
{
	char aBuffer[1024];

	fxErrorMessage(the, theCode, aBuffer, sizeof(aBuffer));
#ifdef mxDebug
	fxDebugC(the, thePath, theLine);
	fxDebugThrow(the, "C: xsError: %s", aBuffer);
#endif
	fxThrowMessage(the, XS_UNKNOWN_ERROR, aBuffer);
}

void fxErrorPrintf(txMachine* the, txString thePath, txInteger theLine, txString theMessage)
{
#ifdef mxDebug
	fxDebugC(the, thePath, theLine);
	fxDebugThrow(the, "C: xsError: %s", theMessage);
#endif
	fxThrowMessage(the, XS_UNKNOWN_ERROR, theMessage);
}

/* Debugger */


void fxDebugger(txMachine* the, txString thePath, txInteger theLine)
{
#ifdef mxDebug
	fxDebugC(the, thePath, theLine);
	fxDebugLoop(the, "C: xsDebugger");
#endif
}

void fxTrace(txMachine* the, txString theString)
{
#ifdef mxDebug
	fxReport(the, NULL, 0, "%s", theString);
#endif
}

/* Machine */

#if __FSK_LAYER__ && SUPPORT_INSTRUMENTATION

static Boolean doFormatMessageXS(FskInstrumentedType dispatch, UInt32 msg, void *msgData, char *buffer, UInt32 bufferSize);

static FskInstrumentedTypeRecord gXSTypeInstrumentation = {
	NULL,
	sizeof(FskInstrumentedTypeRecord),
	"xs",
	FskInstrumentationOffset(txMachine),
	NULL,
	0,
	NULL,
	doFormatMessageXS
};

Boolean doFormatMessageXS(FskInstrumentedType dispatch, UInt32 msg, void *msgData, char *buffer, UInt32 bufferSize)
{
	txMachine *the = (txMachine *)msgData;

	switch (msg) {
		case kFskXSInstrAllocateChunks:
			snprintf(buffer, bufferSize, "Chunk allocation: reserved=%ld, used=%ld, peak=%ld", the->maximumChunksSize, the->currentChunksSize, the->peakChunksSize);
			return true;

		case kFskXSInstrAllocateSlots:
			snprintf(buffer, bufferSize, "Slot allocation: reserved=%ld, used=%ld, peak=%ld",
				the->maximumHeapCount * sizeof(txSlot),
				the->currentHeapCount * sizeof(txSlot),
				the->peakHeapCount * sizeof(txSlot));
			return true;

		case kFskXSInstrBeginCollectSlots:
			snprintf(buffer, bufferSize, "Begin garbage collect slots");
			return true;

		case kFskXSInstrBeginCollectSlotsAndChunks:
			snprintf(buffer, bufferSize, "Begin garbage collect slots and chunks");
			return true;

		case kFskXSInstrEndCollectChunks:
			snprintf(buffer, bufferSize, "End garbage collect chunks: reserved=%ld, used=%ld, peak=%ld, free=%ld", the->maximumChunksSize, the->currentChunksSize, the->peakChunksSize, the->maximumChunksSize - the->currentChunksSize);
			return true;

		case kFskXSInstrEndCollectSlots:
			snprintf(buffer, bufferSize, "End garbage collect slots: reserved=%ld, used=%ld, peak=%ld, free=%ld",
				the->maximumHeapCount * sizeof(txSlot),
				the->currentHeapCount * sizeof(txSlot),
				the->peakHeapCount * sizeof(txSlot),
				(the->maximumHeapCount - the->currentHeapCount) * sizeof(txSlot));
			return true;

		case kFskXSInstrNewChunkWork:
			snprintf(buffer, bufferSize, "new chunk - doesn't fit, size=%ld", (long)msgData);
			return true;

		case kFskXSInstrSkipCollect:
			snprintf(buffer, bufferSize, "Skip garbage collect (disabled)");
			return true;

		case kFskXSInstrTrace:
			vsnprintf(buffer, bufferSize, (char *)((void **)msgData)[0], *(c_va_list *)((void **)msgData)[1]);
			return true;

		case kFskXSInstrReportWarning:
			FskStrCopy(buffer, "Warning! ");
			bufferSize -= FskStrLen(buffer);
			buffer += FskStrLen(buffer);
			vsnprintf(buffer, bufferSize, (char *)((void **)msgData)[0], *(c_va_list *)((void **)msgData)[1]);
			bufferSize -= FskStrLen(buffer);
			buffer += FskStrLen(buffer);
			snprintf(buffer, bufferSize, " in %s, line %d", ((char**)msgData)[2], (int)((void **)msgData)[3]);
			return true;

		case kFskXSInstrReportError:
			FskStrCopy(buffer, "Error! ");
			bufferSize -= FskStrLen(buffer);
			buffer += FskStrLen(buffer);
			vsnprintf(buffer, bufferSize, (char *)((void **)msgData)[0], *(c_va_list *)((void **)msgData)[1]);
			bufferSize -= FskStrLen(buffer);
			buffer += FskStrLen(buffer);
			snprintf(buffer, bufferSize, " in %s, line %d", ((char**)msgData)[2], (int)((void **)msgData)[3]);
			return true;

		case kFskXSInstrException:
			snprintf(buffer, bufferSize, "Exception thrown");
			return true;
	}

	return false;
}

#endif

txMachine* fxNewMachine(txAllocation* theAllocation, txGrammar* theGrammar, void* theContext)
{
	txMachine* the = (txMachine* )c_calloc(sizeof(txMachine), 1);
	if (the) {
		txJump aJump;

		aJump.nextJump = C_NULL;
		aJump.stack = C_NULL;
		aJump.scope = C_NULL;
		aJump.frame = C_NULL;
		aJump.code = C_NULL;
		the->firstJump = &aJump;
		if (c_setjmp(aJump.buffer) == 0) {
			txSlot* aSlot;

		#if __FSK_LAYER__
			FskInstrumentedItemNew(the, NULL, &gXSTypeInstrumentation);
		#endif

		#ifdef mxDebug
			the->echoSize = 1 * 1024;
			the->echoBuffer = (txString)c_malloc(the->echoSize);
			if (!the->echoBuffer)
				fxJump(the);
			//fxConnect(the);
			if (theGrammar)
				the->name = theGrammar->name;
			the->sorter = (txSlot**)c_malloc(theAllocation->symbolCount * sizeof(txSlot*));
			if (!the->sorter)
				fxJump(the);
			the->breakOnExceptionFlag = 1;
		#endif
		#ifdef mxProfile
			the->profileID = 1;
			the->profileBottom = c_malloc(XS_PROFILE_COUNT * sizeof(txProfileRecord));
			if (!the->profileBottom)
				fxJump(the);
			the->profileCurrent = the->profileBottom;
			the->profileTop = the->profileBottom + XS_PROFILE_COUNT;
		#endif

			the->dtoa = fxNew_dtoa();
			if (!the->dtoa)
				fxJump(the);
			fxAllocate(the, theAllocation);

			/* -1 */
			fxNewInstance(the);

			aSlot = the->stack->value.reference;
			aSlot->flag = XS_VALUE_FLAG;
			aSlot->next = fxNewSlot(the);
			aSlot = aSlot->next;
			aSlot->value.global.cache = (txSlot**)fxNewChunk(the, theAllocation->symbolCount * sizeof(txSlot*));
			aSlot->value.global.sandboxCache = (txSlot**)fxNewChunk(the, theAllocation->symbolCount * sizeof(txSlot*));
			aSlot->kind = XS_GLOBAL_KIND;
			c_memset(aSlot->value.global.cache, 0, theAllocation->symbolCount * sizeof(txSlot*));
			c_memset(aSlot->value.global.sandboxCache, 0, theAllocation->symbolCount * sizeof(txSlot*));

			the->scope = the->stack;
			/* -2 */
			mxZeroSlot(--the->stack);
			/* -3 */
			mxZeroSlot(--the->stack);
			/* -4 */
			mxZeroSlot(--the->stack);
			the->stack->kind = XS_LIST_KIND;
			/* -5 */
			mxZeroSlot(--the->stack);
			the->stack->kind = XS_LIST_KIND;

			the->frame = the->stack;
			the->scope = C_NULL;
			fxBuildGlobal(the);
			fxBuildObject(the);
			fxBuildFunction(the);
			fxBuildArray(the);
			fxBuildString(the);
			fxBuildBoolean(the);
			fxBuildNumber(the);
			fxBuildDate(the);
			fxBuildMath(the);
			fxBuildRegExp(the);
			fxBuildError(the);
			fxBuildGrammar(the);
			fxBuildJSON(the);
			fxBuildChunk(the);
			fxBuildInfoSet(the);
			the->frame = C_NULL;

			the->collectFlag = XS_COLLECTING_FLAG;
			the->context = theContext;

		#ifdef mxDebug
			if (fxGetAutomatic(the))
				fxLogin(the);
		#endif

			if (theGrammar) {
				fxLink(the, theGrammar);
				the->stack++;
			}

			the->firstJump = C_NULL;
		}
		else {
		#if __FSK_LAYER__
			FskInstrumentedItemDispose(the);
		#endif
			fxFree(the);
			c_free(the);
			the = NULL;
		}
	}
	return the;
}

void fxDeleteMachine(txMachine* the)
{
	txSlot* aSlot;
	txSlot* bSlot;
	txSlot* cSlot;

	if (!(the->shared)) {
	#ifdef mxProfile
		fxStopProfiling(the);
	#endif
	#ifdef mxFrequency
		fxReportFrequency(the);
	#endif
	#ifdef mxDebug
		fxLogout(the);
		//fxWriteBreakpoints(the);
	#endif
	}
	the->context = C_NULL;
	aSlot = the->cRoot;
	while (aSlot) {
		aSlot->flag |= XS_MARK_FLAG;
		aSlot = aSlot->next;
	}
	aSlot = the->firstHeap;
	while (aSlot) {
		bSlot = aSlot + 1;
		cSlot = aSlot->value.reference;
		while (bSlot < cSlot) {
			if ((bSlot->kind == XS_HOST_KIND) && (bSlot->value.host.variant.destructor)) {
				if (bSlot->flag & XS_HOST_HOOKS_FLAG) {
					if (bSlot->value.host.variant.hooks->destructor)
						(*(bSlot->value.host.variant.hooks->destructor))(bSlot->value.host.data);
				}
				else
					(*(bSlot->value.host.variant.destructor))(bSlot->value.host.data);
			}
			bSlot++;
		}
		aSlot = aSlot->next;
	}
	fxDelete_dtoa(the->dtoa);
	if (the->parser)
		c_free(the->parser);
	if (!(the->shared)) {
	#ifdef mxProfile
		if (the->profileBottom) {
			c_free(the->profileBottom);
			the->profileBottom = C_NULL;
			the->profileCurrent = C_NULL;
			the->profileTop = C_NULL;
		}
		if (the->profileDirectory) {
			c_free(the->profileDirectory);
		}
	#endif
	#if mxDebug
		//fxDisconnect(the);
		if (the->sorter) {
			c_free(the->sorter);
			the->sorter = C_NULL;
		}
		if (the->echoBuffer) {
			c_free(the->echoBuffer);
			the->echoBuffer = C_NULL;
		}
	#endif

	}
#if __FSK_LAYER__
	FskInstrumentedItemDispose(the);
#endif
	fxFree(the);
	c_free(the);
}

txMachine* fxAliasMachine(txAllocation* theAllocation, txMachine* theMachine, txString theName, void* theContext)
{
	txMachine* the = (txMachine *)c_calloc(sizeof(txMachine), 1);
	if (the) {
		txJump aJump;

		aJump.nextJump = C_NULL;
		aJump.stack = C_NULL;
		aJump.scope = C_NULL;
		aJump.frame = C_NULL;
		aJump.code = C_NULL;
		the->firstJump = &aJump;
		if (c_setjmp(aJump.buffer) == 0) {
			txInteger anIndex;
			txSlot* aSlot;
			txSlot** aSlotAddress;
			txSlot* aSharedSlot;
			txSlot** aSharedSlotAddress;
			txSlot** aSandboxSlotAddress;
			txSlot* aTemporarySlot;
			txID anID;

		#if __FSK_LAYER__
			FskInstrumentedItemNew(the, theName, &gXSTypeInstrumentation);
		#endif

		#ifdef mxDebug
			the->echoSize = 1 * 1024;
			the->echoBuffer = (txString)c_malloc(the->echoSize);
			if (!the->echoBuffer)
				fxJump(the);
			//fxConnect(the);
			the->name = theName;
			the->sorter = (txSlot**)c_malloc(theAllocation->symbolCount * sizeof(txSlot*));
			if (!the->sorter)
				fxJump(the);
			the->breakOnExceptionFlag = 1;
		#endif
		#ifdef mxProfile
			the->profileID = theMachine->profileID;
			the->profileBottom = c_malloc(XS_PROFILE_COUNT * sizeof(txProfileRecord));
			if (!the->profileBottom)
				fxJump(the);
			the->profileCurrent = the->profileBottom;
			the->profileTop = the->profileBottom + XS_PROFILE_COUNT;
		#endif

			the->dtoa = fxNew_dtoa();
			if (!the->dtoa)
				fxJump(the);
			theAllocation->symbolCount = theMachine->symbolCount;
			theAllocation->symbolModulo = theMachine->symbolModulo;

			fxAllocate(the, theAllocation);

			the->sharedMachine = theMachine;

#if !mxOptimize
			aSharedSlotAddress = theMachine->symbolArray;
			anIndex = theMachine->symbolIndex;
			while (anIndex) {
				if ((aSharedSlot = *aSharedSlotAddress)) {
					aSlot = fxNewSymbol(the, aSharedSlot); /* union */
					mxCheck(the, aSharedSlot->ID == aSlot->ID);
				}
				anIndex--;
				aSharedSlotAddress++;
			}
#else
			c_memmove(the->symbolArray, theMachine->symbolArray, the->symbolCount * sizeof(txSlot *));
			c_memmove(the->symbolTable, theMachine->symbolTable, the->symbolModulo * sizeof(txSlot *));
			the->symbolIndex = theMachine->symbolIndex;
			the->symbolOffset = the->symbolIndex;
#endif
			the->argumentsID = theMachine->argumentsID;
			the->bodyID = theMachine->bodyID;
			the->calleeID = theMachine->calleeID;
			the->callerID = theMachine->callerID;
			the->chunkID = theMachine->chunkID;
			the->configurableID = theMachine->configurableID;
			the->constructorID = theMachine->constructorID;
			the->enumerableID = theMachine->enumerableID;
			the->evalID = theMachine->evalID;
			the->execID = theMachine->execID;
			the->getID = theMachine->getID;
			the->globalID = theMachine->globalID;
			the->grammarID = theMachine->grammarID;
			the->grammarsID = theMachine->grammarsID;
			the->indexID = theMachine->indexID;
			the->inputID = theMachine->inputID;
			the->instanceID = theMachine->instanceID;
			the->instancesID = theMachine->instancesID;
			the->lengthID = theMachine->lengthID;
			the->lineID = theMachine->lineID;
			the->nameID = theMachine->nameID;
			the->namespaceID = theMachine->namespaceID;
			the->parentID = theMachine->parentID;
			the->parseID = theMachine->parseID;
			the->pathID = theMachine->pathID;
			the->patternsID = theMachine->patternsID;
			the->peekID = theMachine->peekID;
			the->pokeID = theMachine->pokeID;
			the->prefixID = theMachine->prefixID;
			the->prefixesID = theMachine->prefixesID;
			the->prototypeID = theMachine->prototypeID;
			the->rootsID = theMachine->rootsID;
			the->sandboxID = theMachine->sandboxID;
			the->serializeID = theMachine->serializeID;
			the->setID = theMachine->setID;
			the->toJSONID = theMachine->toJSONID;
			the->toStringID = theMachine->toStringID;
			the->valueID = theMachine->valueID;
			the->valueOfID = theMachine->valueOfID;
			the->writableID = theMachine->writableID;

			the->attributeID = theMachine->attributeID;
			the->cdataID = theMachine->cdataID;
			the->childrenID = theMachine->childrenID;
			the->commentID = theMachine->commentID;
			the->compareAttributesID = theMachine->compareAttributesID;
			the->elementID = theMachine->elementID;
			the->piID = theMachine->piID;
			the->pushID = theMachine->pushID;
			the->sortID = theMachine->sortID;
			the->xmlnsAttributesID = theMachine->xmlnsAttributesID;
			the->xmlnsNamespaceID = theMachine->xmlnsNamespaceID;
			the->xmlnsPrefixID = theMachine->xmlnsPrefixID;
			the->_attributesID = theMachine->_attributesID;
			the->__xs__infosetID = theMachine->__xs__infosetID;

			anIndex = the->aliasIndex = theMachine->aliasIndex;
			aSharedSlotAddress = theMachine->aliasArray;
			aSlotAddress = the->aliasArray;
			while (anIndex) {
				*aSlotAddress = *aSharedSlotAddress;
				anIndex--;
				aSharedSlotAddress++;
				aSlotAddress++;
			}

			/* -1 */
			fxNewInstance(the);
			aSlot = the->stack->value.reference;
			aSlot->flag = XS_VALUE_FLAG;
			aSlot->next = fxNewSlot(the);
			aSlot = aSlot->next;
			aSlot->value.global.cache = (txSlot**)fxNewChunk(the, theAllocation->symbolCount * sizeof(txSlot*));
			aSlot->value.global.sandboxCache = (txSlot**)fxNewChunk(the, theAllocation->symbolCount * sizeof(txSlot*));
			aSlot->kind = XS_GLOBAL_KIND;
			c_memset(aSlot->value.global.cache, 0, theAllocation->symbolCount * sizeof(txSlot*));
			c_memset(aSlot->value.global.sandboxCache, 0, theAllocation->symbolCount * sizeof(txSlot*));
			aSlotAddress = aSlot->value.global.cache ;
			aSandboxSlotAddress = aSlot->value.global.sandboxCache;
			aSharedSlot = theMachine->stackTop[-1].value.reference->next->next;
			while (aSharedSlot) {
				aSlot->next = aTemporarySlot = fxDuplicateSlot(the, aSharedSlot);
				anID = aTemporarySlot->ID & 0x7FFF;
				if (!(aTemporarySlot->flag & XS_SANDBOX_FLAG))
					aSlotAddress[anID] = aTemporarySlot;
				if (!(aTemporarySlot->flag & XS_DONT_SCRIPT_FLAG))
					aSandboxSlotAddress[anID] = aTemporarySlot;
				aSharedSlot = aSharedSlot->next;
				aSlot = aSlot->next;
			}

			the->scope = the->stack;
			/* -2 */
			mxZeroSlot(--the->stack);
			/* -3 */
			mxZeroSlot(--the->stack);
			/* -4 */
			mxZeroSlot(--the->stack);
			the->stack->kind = XS_LIST_KIND;
		#ifdef mxDebug
			aSharedSlot = theMachine->stackTop[-4].value.list.first;
			aSlotAddress = &(the->stack->value.list.first);
			while (aSharedSlot) {
				*aSlotAddress = fxDuplicateSlot(the, aSharedSlot);
				aSharedSlot = aSharedSlot->next;
				aSlotAddress = &((*aSlotAddress)->next);
			}
		#endif
			/* -5 */
			mxZeroSlot(--the->stack);
			the->stack->kind = XS_LIST_KIND;

			for (anIndex = -6; anIndex > -28; anIndex--)
				*(--the->stack) = theMachine->stackTop[anIndex];

			the->collectFlag = XS_COLLECTING_FLAG;
			the->context = theContext;

		#ifdef mxDebug
			if (fxGetAutomatic(the))
				fxLogin(the);
		#endif

			the->firstJump = C_NULL;

		}
		else {
		#if __FSK_LAYER__
			FskInstrumentedItemDispose(the);
		#endif

			fxFree(the);
			c_free(the);
			the = NULL;
		}
	}
	return the;
}

void fxShareMachine(txMachine* the)
{
	if (!(the->shared)) {
	#ifdef mxProfile
		fxStopProfiling(the);
	#endif
	#ifdef mxDebug
		fxLogout(the);
	#endif
		fxShare(the);
		the->shared = 1;
	#ifdef mxProfile
		if (the->profileBottom) {
			c_free(the->profileBottom);
			the->profileBottom = C_NULL;
			the->profileCurrent = C_NULL;
			the->profileTop = C_NULL;
		}
	#endif
	#if mxDebug
		//fxDisconnect(the);
		if (the->sorter) {
			c_free(the->sorter);
			the->sorter = C_NULL;
		}
		if (the->echoBuffer) {
			c_free(the->echoBuffer);
			the->echoBuffer = C_NULL;
		}
	#endif
	}
}

/* Garbage Collector */

void fxCollectGarbage(txMachine* the)
{
	fxCollect(the, 1);
}

void fxEnableGarbageCollection(txMachine* the, txBoolean enableIt)
{
	if (enableIt)
		the->collectFlag |= XS_COLLECTING_FLAG;
	else
		the->collectFlag &= ~XS_COLLECTING_FLAG;
}

void fxRemember(txMachine* the, txSlot* theSlot)
{
	txSlot* aHeap;
	txSlot* aLimit;
	if ((the->stack <= theSlot) && (theSlot < the->stackTop)) {
		return;
	}
	aHeap = the->firstHeap;
	while (aHeap) {
		aLimit = aHeap->value.reference;
		if ((aHeap < theSlot) && (theSlot < aLimit)) {
			return;
		}
		aHeap = aHeap->next;
	}
	fxForget(the, theSlot);
	theSlot->next = the->cRoot;
	the->cRoot = theSlot;
}

void fxForget(txMachine* the, txSlot* theSlot)
{
	if (!(theSlot->flag & XS_MARK_FLAG)) {
		txSlot* aSlot;
		txSlot** aSlotAddr = &(the->cRoot);
		while ((aSlot = *aSlotAddr)) {
			if (aSlot == theSlot) {
				*aSlotAddr = aSlot->next;
				return;
			}
			aSlotAddr = &(aSlot->next);
		}
	}
}
void fxAccess(txMachine* the, txSlot* theSlot)
{
	if (theSlot) {
		the->scratch.kind = theSlot->kind;
		the->scratch.value = theSlot->value;
	}
	else
		mxZeroSlot(&the->scratch);
}

/* Host */

txS1 fxBuildHost(txMachine* the, txCallback theCallback)
{
	txS1 aResult;

	mxTry(the) {
		/* ARGC */
		mxZeroSlot(--the->stack);
		the->stack->kind = XS_INTEGER_KIND;
		/* THIS */
		*(--the->stack) = mxGlobal;
		/* FUNCTION */
		mxZeroSlot(--the->stack);
		/* RESULT */
		mxZeroSlot(--the->stack);
		/* FRAME */
		mxZeroSlot(--the->stack);
		the->stack->next = the->frame;
		the->stack->ID = XS_NO_ID;
		the->stack->flag = XS_C_FLAG;
		the->stack->kind = XS_FRAME_KIND;
		the->stack->value.frame.code = the->code;
		the->stack->value.frame.scope = the->scope;
		the->frame = the->stack;
		the->scope = C_NULL;
		the->code = C_NULL;
		/* VARC */
		mxZeroSlot(--the->stack);
		the->stack->kind = XS_INTEGER_KIND;
		(*theCallback)(the);
		the->stack = the->frame + 5;
		the->scope = the->frame->value.frame.scope;
		the->code = the->frame->value.frame.code;
		the->frame = the->frame->next;
		aResult = 1;
	}
	mxCatch(the) {
		aResult = 0;
	}
	return aResult;
}

txMachine* fxBeginHost(txMachine* the)
{
	/* ARGC */
	mxZeroSlot(--the->stack);
	the->stack->kind = XS_INTEGER_KIND;
	/* THIS */
	*(--the->stack) = mxGlobal;
	/* FUNCTION */
	mxZeroSlot(--the->stack);
	/* RESULT */
	mxZeroSlot(--the->stack);
	/* FRAME */
	mxZeroSlot(--the->stack);
	the->stack->next = the->frame;
	the->stack->ID = XS_NO_ID;
	the->stack->flag = XS_C_FLAG;
	the->stack->kind = XS_FRAME_KIND;
	the->stack->value.frame.code = the->code;
	the->stack->value.frame.scope = the->scope;
	the->frame = the->stack;
	the->scope = C_NULL;
	the->code = C_NULL;
	/* VARC */
	mxZeroSlot(--the->stack);
	the->stack->kind = XS_INTEGER_KIND;
	return the;
}

void fxEndHost(txMachine* the)
{
	the->stack = the->frame + 5;
	the->scope = the->frame->value.frame.scope;
	the->code = the->frame->value.frame.code;
	the->frame = the->frame->next;
}

void fxPutAt(txMachine* the, txFlag theFlag, txFlag theMask)
{
	txSlot* anInstance;
	txSlot* aProperty;
	txID anID = XS_NO_ID;
	txFlag aFlag = theFlag & ~XS_ACCESSOR_FLAG;
	txFlag aMask = theMask & ~XS_ACCESSOR_FLAG;

	fxToInstance(the, the->stack + 1);
	anInstance = fxGetOwnInstance(the, the->stack + 1);
	if (!anInstance)
		mxDebugID(the, XS_TYPE_ERROR, "set %s: shared instance", anID);
	aProperty = fxSetPropertyAt(the, anInstance, the->stack, &anID, &aFlag);
	the->stack += 2;
	aProperty->flag = (aProperty->flag & ~aMask) | aFlag;
	if (theFlag & XS_ACCESSOR_FLAG) {
		if (aProperty->kind != XS_ACCESSOR_KIND) {
			aProperty->kind = XS_ACCESSOR_KIND;
			aProperty->value.accessor.getter = C_NULL;
			aProperty->value.accessor.setter = C_NULL;
		}
		if (theFlag & XS_GETTER_FLAG)
			aProperty->value.accessor.getter = the->stack->value.reference;
		else
			aProperty->value.accessor.setter = the->stack->value.reference;
	}
	else {
		aProperty->kind = the->stack->kind;
		aProperty->value = the->stack->value;
	}
}

void fxPutID(txMachine* the, txID theID, txFlag theFlag, txFlag theMask)
{
	txSlot* anInstance;
	txSlot* aProperty;
	txFlag aFlag = theFlag & ~XS_ACCESSOR_FLAG;
	txFlag aMask = theMask & ~XS_ACCESSOR_FLAG;

	fxToInstance(the, the->stack);
	anInstance = fxGetOwnInstance(the, the->stack);
	if (!anInstance)
		mxDebugID(the, XS_TYPE_ERROR, "set %s: shared instance", theID);
	aProperty = fxSetProperty(the, anInstance, theID, &aFlag);
	the->stack++;
	aProperty->flag = (aProperty->flag & ~aMask) | aFlag;
	if (theFlag & XS_ACCESSOR_FLAG) {
		if (aProperty->kind != XS_ACCESSOR_KIND) {
			aProperty->kind = XS_ACCESSOR_KIND;
			aProperty->value.accessor.getter = C_NULL;
			aProperty->value.accessor.setter = C_NULL;
		}
		if (theFlag & XS_GETTER_FLAG)
			aProperty->value.accessor.getter = the->stack->value.reference;
		else
			aProperty->value.accessor.setter = the->stack->value.reference;
	}
	else {
		aProperty->kind = the->stack->kind;
		aProperty->value = the->stack->value;
	}
}

/* Script */

txBoolean fxExecute(txMachine* the, void* theStream, txGetter theGetter, txString thePath, long theLine)
{
	txBoolean aResult;

	fxBeginHost(the);
	{
		mxTry(the) {
			/* ARGC */
			mxZeroSlot(--the->stack);
			the->stack->kind = XS_INTEGER_KIND;
			/* THIS */
			*(--the->stack) = mxGlobal;
			/* FUNCTION */
			mxZeroSlot(--the->stack);
		#ifdef mxDebug
			if (thePath && theLine)
				the->stack->value.code = fxParseScript(the, theStream, theGetter,
						fxNewFileC(the, thePath), theLine,
						XS_PROGRAM_FLAG | XS_SANDBOX_FLAG | XS_DEBUG_FLAG, C_NULL);
			else
		#endif
				the->stack->value.code = fxParseScript(the, theStream, theGetter, C_NULL, 0,
						XS_PROGRAM_FLAG | XS_SANDBOX_FLAG, C_NULL);
			the->stack->kind = XS_CODE_KIND;
			fxNewProgramInstance(the);
			/* RESULT */
			mxZeroSlot(--the->stack);
			fxRunID(the, XS_NO_ID);
			the->stack++;
			aResult = 1;
		}
		mxCatch(the) {
			aResult = 0;
		}
	}
	fxEndHost(the);
	return aResult;
}

typedef struct {
	txString arguments;
	txInteger argumentsSize;
	txString body;
	txInteger bodySize;
	txInteger offset;
} txNewFunctionStream;

int fxNewFunctionStreamGetter(void* theStream)
{
	txNewFunctionStream* aStream = (txNewFunctionStream*)theStream;
	int result = C_EOF;
	if (aStream->offset < aStream->argumentsSize) {
		result = *(aStream->arguments + aStream->offset);
		aStream->offset++;
	}
	else if (aStream->offset == aStream->argumentsSize) {
		result = '{';
		aStream->offset++;
	}
	else {
		txInteger anOffset = aStream->offset - aStream->argumentsSize - 1;
		if (anOffset < aStream->bodySize) {
			result = *(aStream->body + anOffset);
			aStream->offset++;
		}
		else if (anOffset == aStream->bodySize) {
			result = '}';
			aStream->offset++;
		}
	}
	return result;
}

void fxNewFunction(txMachine* the, txString arguments, txInteger argumentsSize, txString body, txInteger bodySize, txFlag theFlag, txString thePath, txInteger theLine)
{
	txNewFunctionStream aStream;
	txSlot* aFunction;
	txByte* aCode;

	aStream.arguments = arguments;
	aStream.argumentsSize = argumentsSize;
	aStream.body = body;
	aStream.bodySize = bodySize;
	aStream.offset = 0;

	mxPush(mxFunctionPrototype);
	aFunction = fxNewFunctionInstance(the);
#ifdef mxDebug
	if (theFlag & XS_DEBUG_FLAG)
		aCode = fxParseScript(the, &aStream, fxNewFunctionStreamGetter,
				fxNewFileC(the, thePath), theLine,
				theFlag, C_NULL);
	else
#endif
		aCode = fxParseScript(the, &aStream, fxNewFunctionStreamGetter, C_NULL, 0,
				theFlag, C_NULL);
	aFunction->next->value.code = aCode;
}

/* XML */

void fxBeginScan(txMachine* the, txFlag theFlag)
{
	the->parseFlag = theFlag;
	/* THIS */
	if (the->frame)
		*(--the->stack) = *mxThis;
	else
		*(--the->stack) = mxGlobal;
	/* FUNCTION */
	mxZeroSlot(--the->stack);
	/* RESULT */
	mxZeroSlot(--the->stack);
	/* FRAME */
	mxZeroSlot(--the->stack);
	the->stack->next = the->frame;
	the->stack->ID = XS_NO_ID;
	the->stack->flag = XS_C_FLAG;
	the->stack->kind = XS_FRAME_KIND;
	the->stack->value.frame.code = the->code;
	the->stack->value.frame.scope = the->scope;
	the->frame = the->stack;
	/* VARC */
	mxZeroSlot(--the->stack);
	the->stack->kind = XS_INTEGER_KIND;
	the->stack->value.integer = 1;
	mxZeroSlot(--the->stack);
	the->code = C_NULL;
}

void fxEndScan(txMachine* the)
{
	the->stack = the->frame + 5 + mxArgc;
	the->scope = the->frame->value.frame.scope;
	the->code = the->frame->value.frame.code;
	*(--the->stack) = *mxResult;
	the->frame = the->frame->next;
}

void fxScan(txMachine* the, void* theStream, txGetter theGetter,
		txString thePath, long theLine, txMarkupCallbacks* theCallbacks)
{
	mxTry(the) {
		fxBeginScan(the, 0);
		fxParseMarkup(the, theStream, theGetter, fxNewFileC(the, thePath), theLine, theCallbacks);
		fxEndScan(the);
	}
	mxCatch(the) {
		mxZeroSlot(--the->stack);
	}
}

void fxScanBuffer(txMachine* the, void* theBuffer, txSize theSize,
		txString thePath, long theLine, txMarkupCallbacks* theCallbacks)
{
	mxTry(the) {
		fxBeginScan(the, 0);
		fxParseMarkupBuffer(the, theBuffer, theSize, fxNewFileC(the, thePath), theLine, theCallbacks);
		fxEndScan(the);
	}
	mxCatch(the) {
		mxZeroSlot(--the->stack);
	}
}

/* Grammars */

void fxLink(txMachine* the, txGrammar* theGrammar)
{
	txString aString;
	txInteger aCount, anIndex, aLength;
	txSlot* aSymbol;

	if (the->shared)
		mxDebug0(the, XS_UNKNOWN_ERROR, "C: xsLink: shared machine");

	aString = (txString)theGrammar->symbols;
	aCount = *((unsigned char*)aString);
	aString++;
	aCount <<= 8;
	aCount += *((unsigned char*)aString);
	aString++;
	mxZeroSlot(--the->stack);
	if (0 < aCount) {
		#if mxUseApHash
		txSize aSum;
		#endif
		the->stack->value.code = fxNewChunk(the, aCount * sizeof(txID));
		the->stack->kind = XS_CODE_KIND;
		c_memset(the->stack->value.code, 0, aCount * sizeof(txID));
		for (anIndex = 0; anIndex < aCount; anIndex++) {
			aLength = c_strlen(aString);
		#ifdef mxDebug
			if (((aLength >= 3) && (c_strcmp(aString + aLength - 3, ".xs") == 0))
					|| ((aLength >= 4) && (c_strcmp(aString + aLength - 4, ".xml") == 0)))
			{	
				aSymbol = fxNewFileC(the, aString);
			}
			else
		#endif
		#if mxUseApHash 
			{
				txByte *p = (txByte *)aString + aLength + 1;
				mxDecode4(p,aSum);
				aSymbol = fxNewSymbolLink(the, aString, aSum);
			}
		#else
			aSymbol = fxNewSymbolC(the, aString);
		#endif
			aString += aLength + symbolEncodeLength + 1;
			((txID*)the->stack->value.code)[anIndex] = aSymbol->ID;
		}
		fxRemapIDs(the, theGrammar->code, (txID*)the->stack->value.code);
	}
	if (theGrammar->callback) {
		/* ARGC */
		mxInitSlot(--the->stack, XS_INTEGER_KIND);
		the->stack->value.integer = 0;
		/* THIS */
		*(--the->stack) = mxGlobal;
		/* FUNCTION */
		mxZeroSlot(--the->stack);
		/* RESULT */
		mxZeroSlot(--the->stack);
		/* FRAME */
		mxZeroSlot(--the->stack);
		the->stack->next = the->frame;
		the->stack->ID = XS_NO_ID;
		the->stack->flag = XS_C_FLAG;
		the->stack->kind = XS_FRAME_KIND;
		the->stack->value.frame.code = the->code;
		the->stack->value.frame.scope = the->scope;
		the->frame = the->stack;
		the->scope = C_NULL;
		the->code = (the->frame + 5)->value.code;
		/* VARC */
		mxZeroSlot(--the->stack);
		the->stack->kind = XS_INTEGER_KIND;
		(*(theGrammar->callback))(the);
		the->stack = the->frame + 5;
		the->scope = the->frame->value.frame.scope;
		the->code = the->frame->value.frame.code;
		the->frame = the->frame->next;
	}
	the->stack++;
	/* ARGC */
	mxZeroSlot(--the->stack);
	the->stack->kind = XS_INTEGER_KIND;
	/* THIS */
	*(--the->stack) = mxGlobal;
	/* FUNCTION */
	mxZeroSlot(--the->stack);
	the->stack->value.code = theGrammar->code;
	the->stack->kind = XS_HOST_KIND;
	fxNewProgramInstance(the);
	/* RESULT */
	mxZeroSlot(--the->stack);
	fxRunID(the, XS_NO_ID);
	//the->stack++;
}

void fxParse(txMachine* the, void* theStream, txGetter theGetter,
		txString thePath, long theLine, txFlag theFlag)
{
	fxBeginScan(the, theFlag);
	{
		mxTry(the) {
			fxProcessRoots(the);
			fxParseMarkup(the, theStream, theGetter, fxNewFileC(the, thePath), theLine, &gxGrammarMarkupCallbacks);
		}
		mxCatch(the) {
			if (theFlag & XS_SNIFF_FLAG)
				break;
			fxJump(the);
		}
	}
	fxEndScan(the);
	if (theFlag & XS_SNIFF_FLAG) {
		txSlot* aSlot = fxGetInstance(the, the->stack);
		if (aSlot && (aSlot->kind == XS_INSTANCE_KIND) && (aSlot->ID >= 0)) {
			the->stack->value.alias = aSlot->ID;
			the->stack->kind = XS_ALIAS_KIND;
		}
		else
			mxZeroSlot(the->stack);
	}
}

void fxParseBuffer(txMachine* the, void* theBuffer, txSize theSize,
		txString thePath, long theLine, txFlag theFlag)
{
	fxBeginScan(the, theFlag);
	{
		mxTry(the) {
			fxProcessRoots(the);
			fxParseMarkupBuffer(the, theBuffer, theSize, fxNewFileC(the, thePath), theLine, &gxGrammarMarkupCallbacks);
		}
		mxCatch(the) {
			if (theFlag & XS_SNIFF_FLAG)
				break;
			fxJump(the);
		}
	}
	fxEndScan(the);
	if (theFlag & XS_SNIFF_FLAG) {
		txSlot* aSlot = fxGetInstance(the, the->stack);
		if (aSlot && (aSlot->kind == XS_INSTANCE_KIND) && (aSlot->ID >= 0)) {
			the->stack->value.alias = aSlot->ID;
			the->stack->kind = XS_ALIAS_KIND;
		}
		else
			mxZeroSlot(the->stack);
	}
}

void fxSerialize(txMachine* the, void* theStream, txPutter thePutter)
{
	txSerializer aSerializer;

	c_memset(&aSerializer, 0, sizeof(aSerializer));
	aSerializer.stack = the->stack;
	aSerializer.stream = theStream;
	aSerializer.putter = thePutter;
	fxSerializeRoot(the, &aSerializer);
}

void fxSerializeBuffer(txMachine* the, void* theBuffer, txSize theSize)
{
	txSerializer aSerializer;

	c_memset(&aSerializer, 0, sizeof(aSerializer));
	aSerializer.stack = the->stack;
	aSerializer.buffer = (txString)theBuffer;
	aSerializer.size = theSize;
	fxSerializeRoot(the, &aSerializer);
	the->stack->value.integer = aSerializer.offset;
	the->stack->kind = XS_INTEGER_KIND;
}

void fxEnterSandbox(txMachine* the)
{
	the->frame->flag |= XS_SANDBOX_FLAG;
}

void fxLeaveSandbox(txMachine* the)
{
	the->frame->flag &= ~XS_SANDBOX_FLAG;
}

void fxSandbox(txMachine* the)
{
	fxGetID(the, the->sandboxID);
}

txInteger fxScript(txMachine* the)
{
	txSlot* aFrame = the->frame;
	txInteger aResult = 0;
	while (aFrame) {
		if (aFrame->flag & XS_SANDBOX_FLAG)
			return aResult;
		aFrame = aFrame->next;
		aResult++;
	}
	return 0;
}

txString fxCurrentFile(txMachine* the)
{
	txSlot* aFrame = the->frame->next;
	txSlot* aRoute = aFrame - 1;
	if (aRoute->next)
		return aRoute->next->value.symbol.string;
	return C_NULL;
}

static txModule* gxFirstModule = NULL;

txCallback fxGetHostModule(txString theName)
{
	txModule* aModule = gxFirstModule;
	while (aModule) {
		if (!c_strcmp(aModule->name, theName))
			return aModule->callback;
		aModule = aModule->next;
	}
	return NULL;
}

void fxSetHostModule(txString theName, txCallback theCallback)
{
	txModule* aModule = c_malloc(sizeof(txModule));
	aModule->next = gxFirstModule;
	aModule->name = theName;
	aModule->callback = theCallback;
	gxFirstModule = aModule;
}

void fxModuleURL(txMachine* the)
{
	txID requireID = (txID)fxID(the, "require");
	txID uriID = (txID)fxID(the, "uri");
	txSlot* aFrame = the->frame;
	mxZeroSlot(--the->stack);
	while (aFrame) {
		txSlot* aScope = aFrame->value.frame.scope;
		while (aScope) {
			if (mxIsReference(aScope)) {
				txSlot* anInstance = fxGetInstance(the, aScope);
				txSlot* aProperty = fxGetProperty(the, anInstance, requireID);
				if (aProperty) {
					anInstance = fxGetInstance(the, aProperty);
					aProperty = fxGetProperty(the, anInstance, uriID);
					if (aProperty) {
						the->stack->kind = aProperty->kind;
						the->stack->value = aProperty->value;
						return;
					}
				}
			}
			aScope = aScope->next;
		}
		aFrame = aFrame->next;
	}
}

void fxHostScope(txMachine* the, txIndex theDepth)
{
	txSlot* aScope = the->scope->next;
	while (theDepth) {
		aScope = aScope->next;
		theDepth--;
	}
	*(--the->stack) = *aScope;
}

void fxNewHostClosure(txMachine* the, txBoolean theFlag)
{
	txSlot* aSlot;
	txSlot* bSlot;
	fxNewInstance(the);
	the->scope = bSlot = the->stack->value.reference;
	bSlot = bSlot->next = fxNewSlot(the);
	if (theFlag) {
		fxNewInstance(the);
		bSlot->kind = the->stack->kind;
		bSlot->value = the->stack->value;
		the->stack++;
	}
	else
		bSlot->kind = XS_NULL_KIND;
	aSlot = the->frame[2].value.reference->next->next; // function > instance > closure
	if (aSlot->kind == XS_CLOSURE_KIND) {
		aSlot = aSlot->value.closure.reference; // closure > instance
		if (aSlot) {
			aSlot = aSlot->next;
			while (aSlot) {
				bSlot = bSlot->next = fxNewSlot(the);
				bSlot->kind = aSlot->kind;
				bSlot->value = aSlot->value;
				aSlot = aSlot->next;
			}
		}
	}
}

void fxNewHostSymbolMap(txMachine* the, txInteger theCount, txString* theStrings)
{
	txSlot* aSymbol;
	txInteger anIndex;
	mxZeroSlot(--the->stack);
	the->code = the->stack->value.code = fxNewChunk(the, theCount * sizeof(txID));
	the->stack->kind = XS_CLOSURE_KIND;
	for (anIndex = 0; anIndex < theCount; anIndex++) {
		aSymbol = fxNewSymbolC(the, theStrings[anIndex]);
		((txID*)the->stack->value.code)[anIndex] = aSymbol->ID;
	}
}

void fxCopyObject(txMachine* the)
{
	txSlot* toInstance;
	txSlot* fromInstance;
	txSlot* toProperty;
	txSlot* fromProperty;
	txSlot** firstAddress;
	txSlot** lastAddress;

	fxToInstance(the, the->stack + 1);
	toInstance = fxGetInstance(the, the->stack + 1);
	fxToInstance(the, the->stack);
	fromInstance = fxGetInstance(the, the->stack);
	firstAddress = &toInstance->next;
	while ((toProperty = *firstAddress))
		firstAddress = &toProperty->next;
	lastAddress = firstAddress;
	fromProperty = fromInstance->next;
	while (fromProperty) {
		if (fromProperty->ID != XS_NO_ID) {
			toProperty = toInstance->next;
			while (toProperty != *firstAddress) {
				if (toProperty->ID == fromProperty->ID)
					break;
				toProperty = toProperty->next;
			}
			if (toProperty != *firstAddress) {
				toProperty->kind = fromProperty->kind;
				toProperty->value = fromProperty->value;
			}
			else {
				*lastAddress = toProperty = fxNewSlot(the);
				toProperty->ID = fromProperty->ID;
				toProperty->flag = fromProperty->flag & (~(XS_SANDBOX_FLAG | XS_DONT_SCRIPT_FLAG));
				toProperty->kind = fromProperty->kind;
				toProperty->value = fromProperty->value;
				lastAddress = &toProperty->next;
			}
		}
		fromProperty = fromProperty->next;
	}	
	the->stack++;
}



