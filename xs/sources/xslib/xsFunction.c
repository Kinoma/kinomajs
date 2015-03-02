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

#include "xsAll.h"

#define mxCheckFunction(THE_SLOT) \
	if (!mxIsFunction(THE_SLOT)) \
		mxDebug0(the, XS_TYPE_ERROR, "this is no Function")

static void fx_Function(txMachine* the);
static void fx_Function_get_length(txMachine* the);
static void fx_Function_get_prototype(txMachine* the);
static void fx_Function_set_prototype(txMachine* the);
static void fx_Function_apply(txMachine* the);
static void fx_Function_bind(txMachine* the);
static void fx_Function_bound(txMachine* the);
static void fx_Function_call(txMachine* the);

void fxBuildFunction(txMachine* the)
{
	mxPush(mxGlobal);
			
	mxPush(mxObjectPrototype);
	fxNewFunctionInstance(the);
	fxNewHostFunction(the, fx_Function_get_length, 0);
	fxQueueID(the, the->lengthID, XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG | XS_GETTER_FLAG);
	fxNewHostFunction(the, fx_Function_get_prototype, 0);
	fxQueueID(the, the->prototypeID, XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_GETTER_FLAG);
	fxNewHostFunction(the, fx_Function_set_prototype, 1);
	fxQueueID(the, the->prototypeID, XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_SETTER_FLAG);
	fxNewHostFunction(the, fx_Function_apply, 2);
	fxQueueID(the, fxID(the, "apply"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_Function_bind, 1);
	fxQueueID(the, fxID(the, "bind"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_Function_call, 1);
	fxQueueID(the, fxID(the, "call"), XS_DONT_ENUM_FLAG);
	
	fxAliasInstance(the, the->stack);
	mxFunctionPrototype = *the->stack;
	fxNewHostConstructor(the, fx_Function, 1);
	the->stack->value.reference->next->next->next->flag |= XS_DONT_SET_FLAG;
	 *(--the->stack) = mxFunctionPrototype;
	fxPutID(the, fxID(the, "constructor"), XS_DONT_ENUM_FLAG, XS_DONT_ENUM_FLAG);
	fxQueueID(the, fxID(the, "Function"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	
	the->stack++;
}

txSlot* fxNewArgumentsInstance(txMachine* the)
{
	txSlot* anInstance;
	txSlot* aProperty;
	txID aCount, anID;
	txSlot* aFunction;
	txSlot* aSlot;

	mxPush(mxObjectPrototype);
	anInstance = fxNewObjectInstance(the);
	the->stack->ID = the->argumentsID;
	the->stack->flag = XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG;
	aProperty = anInstance->next = fxNewSlot(the);
	aProperty->flag = XS_DONT_ENUM_FLAG;
	aProperty->ID = the->lengthID;
	aProperty->kind = XS_INTEGER_KIND;
	aProperty->value.integer = mxArgc;
	aCount = (txID)(aProperty->value.integer);
	if (the->frame->flag & XS_STRICT_FLAG) {
		aFunction = mxThrowTypeErrorFunction.value.reference;
		aProperty = aProperty->next = fxNewSlot(the);
		aProperty->flag = XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG;
		aProperty->ID = the->calleeID;
		aProperty->kind = XS_ACCESSOR_KIND;
		aProperty->value.accessor.getter = aFunction;
		aProperty->value.accessor.setter = aFunction;
		aProperty = aProperty->next = fxNewSlot(the);
		aProperty->flag = XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG;
		aProperty->ID = the->callerID;
		aProperty->kind = XS_ACCESSOR_KIND;
		aProperty->value.accessor.getter = aFunction;
		aProperty->value.accessor.setter = aFunction;
		for (anID = 0; anID < aCount; anID++) {
			aSlot = the->frame + 4 + aCount - anID;
			aProperty = aProperty->next = fxNewSlot(the);
			aProperty->ID = anID;
			aProperty->kind = aSlot->kind;
			aProperty->value = aSlot->value;
		}
	}
	else {
		aProperty = aProperty->next = fxNewSlot(the);
		aProperty->flag = XS_DONT_ENUM_FLAG;
		aProperty->ID = the->calleeID;
		aProperty->kind = mxFunction->kind;
		aProperty->value = mxFunction->value;
		aProperty = aProperty->next = fxNewSlot(the);
		aProperty->flag = XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG;
		aProperty->ID = the->callerID;
		aProperty->kind = XS_NULL_KIND;
		aProperty->value.array.length = aCount;
		aProperty->value.array.address = the->frame;
		for (anID = 0; anID < aCount; anID++) {
			aProperty = aProperty->next = fxNewSlot(the);
			aProperty->ID = anID;
			aProperty->kind = XS_ACCESSOR_KIND;
			aProperty->value.accessor.getter = mxGetArgumentFunction.value.reference;
			aProperty->value.accessor.setter = mxSetArgumentFunction.value.reference;
		}
	}
	return anInstance;
}

txSlot* fxNewFunctionInstance(txMachine* the)
{
	txSlot* anInstance;
	txSlot* aProperty;
	txSlot* aPrototype;

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
	
	/* CODE */
	aProperty = anInstance->next = fxNewSlot(the);
	aProperty->flag = XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG;
	aProperty->kind = mxEmptyCode.kind;
	aProperty->value.code = mxEmptyCode.value.code;

	/* CLOSURE */
	aProperty = aProperty->next = fxNewSlot(the);
	aProperty->flag = XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG;
	aProperty->kind = XS_NULL_KIND;
	
	/* PROTOTYPE */
	aProperty = aProperty->next = fxNewSlot(the);
	aProperty->flag = XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG;
	aPrototype =  fxNewSlot(the);
	aPrototype->ID = mxObjectPrototype.value.alias;
	aPrototype->next = C_NULL;
	aPrototype->flag = XS_NO_FLAG;
	aPrototype->kind = XS_INSTANCE_KIND;
	aPrototype->value.instance.garbage = C_NULL;
	aPrototype->value.instance.prototype = C_NULL;
	
	aProperty->kind = XS_REFERENCE_KIND;
	aProperty->value.reference = aPrototype;
	
#ifdef mxProfile
	/* PROFILE */
	aProperty = aProperty->next = fxNewSlot(the);
	aProperty->flag = XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG;
	aProperty->kind = XS_INTEGER_KIND;
	aProperty->value.integer = the->profileID;
	the->profileID++;
#endif

	return anInstance;
}

void fxNewProgramInstance(txMachine* the)
{
	txSlot* anInstance;
	txSlot* aProperty;

	fxNewInstance(the);
	anInstance = the->stack->value.reference;
	
	/* CODE */
	aProperty = anInstance->next = fxNewSlot(the);
	*aProperty = *(the->stack + 1);
	aProperty->ID = XS_NO_ID;
	aProperty->flag = XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG;

	/* CLOSURE */
	aProperty = aProperty->next = fxNewSlot(the);
	aProperty->flag = XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG;
	aProperty->kind = XS_NULL_KIND;
	
	/* PROTOTYPE */
	aProperty = aProperty->next = fxNewSlot(the);
	aProperty->flag = XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG;
	
#ifdef mxProfile
	/* PROFILE */
	aProperty = aProperty->next = fxNewSlot(the);
	aProperty->flag = XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG;
	aProperty->kind = XS_INTEGER_KIND;
	aProperty->value.integer = the->profileID;
	the->profileID++;
#endif
	
	*(the->stack + 1) = *(the->stack);
	the->stack++;
}

void fx_Function(txMachine* the)
{	
	txSlot* aFunction;
	txInteger aCount, anIndex, aFileIndex, aLineIndex;
	txFlag aFlag;
	txSlot* aSlot;
	txStringStream aStream;
	txByte* aCode;
	txSlot* aProperty;
	
	if (mxResult->kind == XS_UNDEFINED_KIND) {
		mxPush(mxFunctionPrototype);
		aFunction = fxNewFunctionInstance(the);
		*mxResult = *(the->stack++);
	}
	else
		aFunction = fxGetOwnInstance(the, mxResult);
	mxCheckFunction(aFunction);

	aCount = mxArgc;
	anIndex = 0;
	aFileIndex = aCount - 2;
	aLineIndex = aCount - 1;
	aFlag = XS_NO_FLAG;
	if ((aCount >= 4) && (mxArgv(aLineIndex)->kind == XS_INTEGER_KIND)) {
		aCount -= 2;
		aFlag |= XS_DEBUG_FLAG;
	}
	aSlot = fxGetInstance(the, mxThis);
	if (aSlot && (aSlot->flag & XS_SANDBOX_FLAG))
		aFlag |= XS_SANDBOX_FLAG;
	else
		aFlag |= the->frame->next->flag & XS_SANDBOX_FLAG;
	
	mxZeroSlot(--the->stack);
	fxCopyStringC(the, the->stack, "(");
	while (aCount > 1) {
		fxToString(the, mxArgv(anIndex));
		fxConcatString(the, the->stack, mxArgv(anIndex));
		if (aCount > 2)
			fxConcatStringC(the, the->stack, ", ");
		aCount--;
		anIndex++;
	}
	fxConcatStringC(the, the->stack, "){");
	if (aCount > 0) {
		fxToString(the, mxArgv(anIndex));
		fxConcatString(the, the->stack, mxArgv(anIndex));
	}
	fxConcatStringC(the, the->stack, "}");
		
	aStream.slot = the->stack;
	aStream.offset = 0;
	aStream.size = c_strlen(the->stack->value.string);
#ifdef mxDebug
	if (aFlag & XS_DEBUG_FLAG) 
		aCode = fxParseScript(the, &aStream, fxStringGetter, 
				fxNewFile(the, mxArgv(aFileIndex)), mxArgv(aLineIndex)->value.integer, 
				aFlag, C_NULL);
	else
#endif
		aCode = fxParseScript(the, &aStream, fxStringGetter, C_NULL, 0, 
				aFlag, C_NULL);
	
	aFunction->next->value.code = aCode;
	
	aProperty = fxSetProperty(the, aFunction, the->bodyID, C_NULL);
	aProperty->flag = XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG;
	aProperty->value.string = mxArgv(anIndex)->value.string;
	aProperty->kind = mxArgv(anIndex)->kind;
	the->stack++;
}

void fx_Function_get_length(txMachine* the)
{	
	txSlot* aFunction;
	txSlot* aSlot;

	aFunction = fxGetInstance(the, mxThis);
	mxCheckFunction(aFunction);
	aSlot = aFunction->next;
	if (aSlot->kind == XS_CODE_KIND)
		mxResult->value.integer = *(aSlot->value.code + 4);
	else
		mxResult->value.integer = aSlot->value.callback.length;
	mxResult->kind = XS_INTEGER_KIND;
}

void fx_Function_get_prototype(txMachine* the)
{	
	txSlot* aFunction;
	txSlot* aSlot;

	aFunction = fxGetInstance(the, mxThis);
	mxCheckFunction(aFunction);
	aSlot = aFunction->next->next->next;
	mxResult->value = aSlot->value;
	mxResult->kind = aSlot->kind;
}

void fx_Function_set_prototype(txMachine* the)
{	
	txSlot* aFunction;
	txSlot* aSlot;

	aFunction = fxGetInstance(the, mxThis);
	mxCheckFunction(aFunction);
	aSlot = aFunction->next->next->next;
	if (aSlot->flag & XS_DONT_SET_FLAG)
		mxDebug0(the, XS_NO_ERROR, "set prototype: no permission");
	else {
		if (aFunction->flag & XS_SHARED_FLAG)
			mxDebug0(the, XS_TYPE_ERROR, "Function.set prototype: this is shared");
		fxToInstance(the, mxArgv(0));
		aSlot->value = mxArgv(0)->value;
		aSlot->kind = mxArgv(0)->kind;
	}
}

void fx_Function_apply(txMachine* the)
{
	txInteger aCount, anIndex;
	
	if ((mxArgc < 2) || (mxArgv(1)->kind == XS_UNDEFINED_KIND) || (mxArgv(1)->kind == XS_NULL_KIND))
		aCount = 0;
	else {
		fxToInstance(the, mxArgv(1));
		*(--the->stack) = *mxArgv(1);
		fxGetID(the, the->lengthID);
		aCount = fxToInteger(the, the->stack);
		the->stack++;
		for (anIndex = 0; anIndex < aCount; anIndex++) {
			*(--the->stack) = *mxArgv(1);
			fxGetID(the, (txID)anIndex);
		}
	}
	/* #PARAM */
	mxZeroSlot(--the->stack);
	the->stack->kind = XS_INTEGER_KIND;
	the->stack->value.integer = aCount;
	/* THIS */
	if (mxArgc < 1) {
		mxZeroSlot(--the->stack);
	}
	else
		*(--the->stack) = *mxArgv(0);
	/* FUNCTION */
	*(--the->stack) = *mxThis;
	/* RESULT */
	mxZeroSlot(--the->stack);
	fxRunID(the, XS_NO_ID);
	*mxResult = *(the->stack++);
}

void fx_Function_bind(txMachine* the)
{
	txSlot* aFunction;
	txSlot* aProperty;
	txSlot* anArray;
	txSlot* anItem;
	txSize aCount, anIndex;

	aFunction = fxGetInstance(the, mxThis);
	if (!mxIsFunction(aFunction))
		mxDebug0(the, XS_TYPE_ERROR, "this is no Function");
	if (mxArgc < 1)
		mxDebug0(the, XS_SYNTAX_ERROR, "Function.prototype.bind: no this parameter");
		
	aProperty = aFunction->next;
	if (aProperty->kind == XS_CODE_KIND)
		aCount = *(aProperty->value.code + 4);
	else
		aCount = aProperty->value.callback.length;
	aCount -= mxArgc - 1;
	if (aCount < 0)
		aCount = 0;
		
	mxPush(mxFunctionPrototype);
	aFunction = fxNewFunctionInstance(the);
	*mxResult = *(the->stack++);
	aProperty = aFunction->next;
	aProperty->kind = XS_CALLBACK_KIND;
	aProperty->value.callback.address = fx_Function_bound;
	aProperty->value.callback.length = aCount;
	
	aProperty = fxSetProperty(the, aFunction, fxID(the, "boundFunction"), C_NULL);
	aProperty->kind = mxThis->kind;
	aProperty->value = mxThis->value;
	aProperty = fxSetProperty(the, aFunction, fxID(the, "boundThis"), C_NULL);
	aProperty->kind = mxArgv(0)->kind;
	aProperty->value = mxArgv(0)->value;
	aProperty = fxSetProperty(the, aFunction, fxID(the, "boundArguments"), C_NULL);
	mxPush(mxArrayPrototype);
	anArray = fxNewArrayInstance(the);
	aProperty->kind = the->stack->kind;
	aProperty->value = the->stack->value;
	the->stack++;
	anItem = anArray->next;
	for (anIndex = 1; anIndex < mxArgc; anIndex++) {
		anItem->next = fxNewSlot(the);
		anItem = anItem->next;
		anItem->kind = mxArgv(anIndex)->kind;
		anItem->value = mxArgv(anIndex)->value;
	}
	anArray->next->value.array.length = mxArgc - 1;
	fxCacheArray(the, anArray);
}

void fx_Function_bound(txMachine* the)
{
	txSlot* boundArguments;
	txInteger aCount, anIndex;
	txSlot* aParameter;
	
	mxPush(*mxFunction);
	fxGetID(the, fxID(the, "boundArguments")); 
	boundArguments = fxGetInstance(the, the->stack);
	the->stack++;
	aCount = boundArguments->next->value.array.length;
	aParameter = boundArguments->next->value.array.address;
	for (anIndex = 0; anIndex < aCount; anIndex++) {
		mxZeroSlot(--the->stack);
		the->stack->kind = aParameter->kind;
		the->stack->value = aParameter->value;
		aParameter++;
	}
	for (anIndex = 0; anIndex < mxArgc; anIndex++) {
		mxZeroSlot(--the->stack);
		the->stack->kind = mxArgv(anIndex)->kind;
		the->stack->value = mxArgv(anIndex)->value;
	}
	/* #PARAM */
	mxZeroSlot(--the->stack);
	the->stack->kind = XS_INTEGER_KIND;
	the->stack->value.integer = aCount + mxArgc;
	/* THIS */
	mxPush(*mxFunction);
	fxGetID(the, fxID(the, "boundThis"));
	/* FUNCTION */
	mxPush(*mxFunction);
	fxGetID(the, fxID(the, "boundFunction"));
	/* RESULT */
	mxZeroSlot(--the->stack);
	fxRunID(the, XS_NO_ID);
	*mxResult = *(the->stack++);
}

void fx_Function_call(txMachine* the)
{	
	txInteger aCount, anIndex;

	aCount = mxArgc;
	anIndex = 1;
	while (anIndex < aCount) {
		*(--the->stack) = *mxArgv(anIndex);
		anIndex++;
	}
	/* #PARAMS */
	mxZeroSlot(--the->stack);
	the->stack->kind = XS_INTEGER_KIND;
	the->stack->value.integer = anIndex - 1;
	/* THIS */
	if (mxArgc < 1) {
		mxZeroSlot(--the->stack);
	}
	else
		*(--the->stack) = *mxArgv(0);
	/* FUNCTION */
	*(--the->stack) = *mxThis;
	/* RESULT */
	mxZeroSlot(--the->stack);
	fxRunID(the, XS_NO_ID);
	*mxResult = *(the->stack++);
}
