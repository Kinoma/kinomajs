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

static void fx_Error(txMachine* the);
static void fx_Error_aux(txMachine* the);
static void fx_Error_toString(txMachine* the);
static void fx_EvalError(txMachine* the);
static void fx_RangeError(txMachine* the);
static void fx_ReferenceError(txMachine* the);
static void fx_SyntaxError(txMachine* the);
static void fx_TypeError(txMachine* the);
static void fx_URIError(txMachine* the);

void fxBuildError(txMachine* the)
{
	mxPush(mxGlobal);
	
	mxPush(mxObjectPrototype);
	fxNewObjectInstance(the);
	mxPushStringC("Error");
	fxQueueID(the, fxID(the, "name"), XS_DONT_ENUM_FLAG);
	mxPush(mxEmptyString);
	fxQueueID(the, fxID(the, "message"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_Error_toString, 0);
	fxQueueID(the, fxID(the, "toLocaleString"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_Error_toString, 0);
	fxQueueID(the, fxID(the, "toString"), XS_DONT_ENUM_FLAG);
	fxAliasInstance(the, the->stack);
	mxErrorPrototype = *the->stack;
	fxNewHostConstructor(the, fx_Error, 1);
	the->stack->value.reference->next->next->next->flag |= XS_DONT_SET_FLAG;
	 *(--the->stack) = mxErrorPrototype;
	fxPutID(the, fxID(the, "constructor"), XS_DONT_ENUM_FLAG, XS_DONT_ENUM_FLAG);
	fxQueueID(the, fxID(the, "Error"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	
	mxPush(mxErrorPrototype);
	fxNewObjectInstance(the);
	mxPushStringC("EvalError");
	fxQueueID(the, fxID(the, "name"), XS_DONT_ENUM_FLAG);
	mxPush(mxEmptyString);
	fxQueueID(the, fxID(the, "message"), XS_DONT_ENUM_FLAG);
	fxAliasInstance(the, the->stack);
	mxEvalErrorPrototype = *the->stack;
	fxNewHostConstructor(the, fx_EvalError, 1);
	the->stack->value.reference->next->next->next->flag |= XS_DONT_SET_FLAG;
	 *(--the->stack) = mxEvalErrorPrototype;
	fxPutID(the, fxID(the, "constructor"), XS_DONT_ENUM_FLAG, XS_DONT_ENUM_FLAG);
	fxQueueID(the, fxID(the, "EvalError"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	
	mxPush(mxErrorPrototype);
	fxNewObjectInstance(the);
	mxPushStringC("RangeError");
	fxQueueID(the, fxID(the, "name"), XS_DONT_ENUM_FLAG);
	mxPush(mxEmptyString);
	fxQueueID(the, fxID(the, "message"), XS_DONT_ENUM_FLAG);
	fxAliasInstance(the, the->stack);
	mxRangeErrorPrototype = *the->stack;
	fxNewHostConstructor(the, fx_RangeError, 1);
	the->stack->value.reference->next->next->next->flag |= XS_DONT_SET_FLAG;
	 *(--the->stack) = mxRangeErrorPrototype;
	fxPutID(the, fxID(the, "constructor"), XS_DONT_ENUM_FLAG, XS_DONT_ENUM_FLAG);
	fxQueueID(the, fxID(the, "RangeError"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	
	mxPush(mxErrorPrototype);
	fxNewObjectInstance(the);
	mxPushStringC("ReferenceError");
	fxQueueID(the, fxID(the, "name"), XS_DONT_ENUM_FLAG);
	mxPush(mxEmptyString);
	fxQueueID(the, fxID(the, "message"), XS_DONT_ENUM_FLAG);
	fxAliasInstance(the, the->stack);
	mxReferenceErrorPrototype = *the->stack;
	fxNewHostConstructor(the, fx_ReferenceError, 1);
	the->stack->value.reference->next->next->next->flag |= XS_DONT_SET_FLAG;
	 *(--the->stack) = mxReferenceErrorPrototype;
	fxPutID(the, fxID(the, "constructor"), XS_DONT_ENUM_FLAG, XS_DONT_ENUM_FLAG);
	fxQueueID(the, fxID(the, "ReferenceError"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);

	mxPush(mxErrorPrototype);
	fxNewObjectInstance(the);
	mxPushStringC("SyntaxError");
	fxQueueID(the, fxID(the, "name"), XS_DONT_ENUM_FLAG);
	mxPush(mxEmptyString);
	fxQueueID(the, fxID(the, "message"), XS_DONT_ENUM_FLAG);
	fxAliasInstance(the, the->stack);
	mxSyntaxErrorPrototype = *the->stack;
	fxNewHostConstructor(the, fx_SyntaxError, 1);
	the->stack->value.reference->next->next->next->flag |= XS_DONT_SET_FLAG;
	 *(--the->stack) = mxSyntaxErrorPrototype;
	fxPutID(the, fxID(the, "constructor"), XS_DONT_ENUM_FLAG, XS_DONT_ENUM_FLAG);
	fxQueueID(the, fxID(the, "SyntaxError"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	
	mxPush(mxErrorPrototype);
	fxNewObjectInstance(the);
	mxPushStringC("TypeError");
	fxQueueID(the, fxID(the, "name"), XS_DONT_ENUM_FLAG);
	mxPush(mxEmptyString);
	fxQueueID(the, fxID(the, "message"), XS_DONT_ENUM_FLAG);
	fxAliasInstance(the, the->stack);
	mxTypeErrorPrototype = *the->stack;
	fxNewHostConstructor(the, fx_TypeError, 1);
	the->stack->value.reference->next->next->next->flag |= XS_DONT_SET_FLAG;
	 *(--the->stack) = mxTypeErrorPrototype;
	fxPutID(the, fxID(the, "constructor"), XS_DONT_ENUM_FLAG, XS_DONT_ENUM_FLAG);
	fxQueueID(the, fxID(the, "TypeError"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	
	mxPush(mxErrorPrototype);
	fxNewObjectInstance(the);
	mxPushStringC("URIError");
	fxQueueID(the, fxID(the, "name"), XS_DONT_ENUM_FLAG);
	mxPush(mxEmptyString);
	fxQueueID(the, fxID(the, "message"), XS_DONT_ENUM_FLAG);
	fxAliasInstance(the, the->stack);
	mxURIErrorPrototype = *the->stack;
	fxNewHostConstructor(the, fx_URIError, 1);
	the->stack->value.reference->next->next->next->flag |= XS_DONT_SET_FLAG;
	 *(--the->stack) = mxURIErrorPrototype;
	fxPutID(the, fxID(the, "constructor"), XS_DONT_ENUM_FLAG, XS_DONT_ENUM_FLAG);
	fxQueueID(the, fxID(the, "URIError"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);

	the->stack++;
}

void fxThrowError(txMachine* the, txError theError)
{
	if ((theError <= XS_NO_ERROR) || (XS_ERROR_COUNT <= theError))
		theError = XS_UNKNOWN_ERROR;
	mxPush(mxErrorPrototypes(theError));	
	fxNewObjectInstance(the);
	mxException = *(the->stack++);
	fxJump(the);
}

void fxThrowMessage(txMachine* the, txError theError, txString theMessage)
{
	if ((theError <= XS_NO_ERROR) || (XS_ERROR_COUNT <= theError))
		theError = XS_UNKNOWN_ERROR;
	mxPush(mxErrorPrototypes(theError));	
	fxNewObjectInstance(the);
	mxPushStringC(theMessage);
	fxQueueID(the, fxID(the, "message"), XS_DONT_ENUM_FLAG);
	mxException = *(the->stack++);
	fxJump(the);
}

void fx_Error(txMachine* the)
{
	if (mxResult->kind == XS_UNDEFINED_KIND) {
		mxPush(mxErrorPrototype);
		fxNewObjectInstance(the);
		*mxResult = *(the->stack++);
	}
	fx_Error_aux(the);
}

void fx_Error_aux(txMachine* the)
{
	txSlot* aProperty;

	if ((mxArgc > 0) && (mxArgv(0)->kind != XS_UNDEFINED_KIND)) {
		aProperty = fxSetProperty(the, fxGetOwnInstance(the, mxResult), fxID(the, "message"), C_NULL);
		aProperty->value.string = fxToString(the, mxArgv(0));
		aProperty->kind = mxArgv(0)->kind;
	}
}

void fx_Error_toString(txMachine* the)
{
	txSlot* anError;
	txSlot* aProperty;
	txInteger aLength;
	
	anError = fxGetInstance(the, mxThis);
	aProperty = fxGetProperty(the, anError, fxID(the, "name"));
	--the->stack;
	the->stack->kind = aProperty->kind;
	the->stack->value = aProperty->value;
	if (the->stack->kind == XS_UNDEFINED_KIND) 
		fxCopyStringC(the, the->stack, "Error");
	else	
		fxToString(the, the->stack);
	aProperty = fxGetProperty(the, anError, fxID(the, "message"));
	--the->stack;
	the->stack->kind = aProperty->kind;
	the->stack->value = aProperty->value;
	if (the->stack->kind == XS_UNDEFINED_KIND) 
		fxCopyStringC(the, the->stack, "");
	else	
		fxToString(the, the->stack);
	aLength = c_strlen(the->stack->value.string);
	if (aLength) {
		aLength += c_strlen((the->stack + 1)->value.string) + 2;
		mxResult->value.string = (txString)fxNewChunk(the, aLength + 1);
		mxResult->kind = XS_STRING_KIND;
		c_strcpy(mxResult->value.string, (the->stack + 1)->value.string);
		c_strcat(mxResult->value.string, ": ");
		c_strcat(mxResult->value.string, the->stack->value.string);
		the->stack++;
		the->stack++;
	}
	else {
		the->stack++;
		*mxResult = *(the->stack++);
	}
}

void fx_EvalError(txMachine* the)
{
	if (mxResult->kind == XS_UNDEFINED_KIND) {
		mxPush(mxEvalErrorPrototype);
		fxNewObjectInstance(the);
		*mxResult = *(the->stack++);
	}
	fx_Error_aux(the);
}

void fx_RangeError(txMachine* the)
{
	if (mxResult->kind == XS_UNDEFINED_KIND) {
		mxPush(mxRangeErrorPrototype);
		fxNewObjectInstance(the);
		*mxResult = *(the->stack++);
	}
	fx_Error_aux(the);
}

void fx_ReferenceError(txMachine* the)
{
	if (mxResult->kind == XS_UNDEFINED_KIND) {
		mxPush(mxReferenceErrorPrototype);
		fxNewObjectInstance(the);
		*mxResult = *(the->stack++);
	}
	fx_Error_aux(the);
}

void fx_SyntaxError(txMachine* the)
{
	if (mxResult->kind == XS_UNDEFINED_KIND) {
		mxPush(mxSyntaxErrorPrototype);
		fxNewObjectInstance(the);
		*mxResult = *(the->stack++);
	}
	fx_Error_aux(the);
}

void fx_TypeError(txMachine* the)
{
	if (mxResult->kind == XS_UNDEFINED_KIND) {
		mxPush(mxTypeErrorPrototype);
		fxNewObjectInstance(the);
		*mxResult = *(the->stack++);
	}
	fx_Error_aux(the);
}

void fx_URIError(txMachine* the)
{
	if (mxResult->kind == XS_UNDEFINED_KIND) {
		mxPush(mxURIErrorPrototype);
		fxNewObjectInstance(the);
		*mxResult = *(the->stack++);
	}
	fx_Error_aux(the);
}

