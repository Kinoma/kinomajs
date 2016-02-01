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

#define mxIsPromise(THE_SLOT) \
	((THE_SLOT) && ((THE_SLOT)->next) && ((THE_SLOT)->next->flag & XS_INTERNAL_FLAG) && ((THE_SLOT)->next->kind == XS_PROMISE_KIND))

#define mxPromiseStatus(INSTANCE) ((INSTANCE)->next)
#define mxPromiseThens(INSTANCE) ((INSTANCE)->next->next)
#define mxPromiseResult(INSTANCE) ((INSTANCE)->next->next->next)

enum {
	mxUndefinedStatus,
	mxPendingStatus,
	mxFulfilledStatus,
	mxRejectedStatus
};

static txSlot* fxNewPromiseAlready(txMachine* the);
static txSlot* fxNewPromiseFunction(txMachine* the, txSlot* already, txSlot* promise, txSlot* function);
static txSlot* fxNewPromiseFunctionAll(txMachine* the, txSlot* already, txSlot* array, txInteger index, txSlot* count, txSlot* promise, txSlot* function);
static void fxBuildPromiseCapability(txMachine* the);
static void fxCallPromise(txMachine* the);
static void fxCallPromiseAll(txMachine* the);
static void fxCheckPromiseCapability(txMachine* the, txSlot* capability, txSlot** resolveFunction, txSlot** rejectFunction);
static void fxOnRejectedPromise(txMachine* the);
static void fxOnResolvedPromise(txMachine* the);
static void fxRejectPromise(txMachine* the);
static void fxResolvePromise(txMachine* the);
static void fx_Promise(txMachine* the);
static void fx_Promise_all(txMachine* the);
static void fx_Promise_race(txMachine* the);
static void fx_Promise_reject(txMachine* the);
static void fx_Promise_resolve(txMachine* the);
static void fx_Promise_prototype_catch(txMachine* the);
static void fx_Promise_prototype_then(txMachine* the);
static void fxQueueJob(txMachine* the, txID id);

void fxBuildPromise(txMachine* the)
{
    static const txHostFunctionBuilder gx_Promise_prototype_builders[] = {
		{ fx_Promise_prototype_catch, 1, _catch },
		{ fx_Promise_prototype_then, 2, _then },
		{ C_NULL, 0, 0 },
    };
    static const txHostFunctionBuilder gx_Promise_builders[] = {
		{ fx_Promise_all, 1, _all },
		{ fx_Promise_race, 1, _race },
		{ fx_Promise_reject, 1, _reject },
		{ fx_Promise_resolve, 1, _resolve },
		{ C_NULL, 0, 0 },
    };
    const txHostFunctionBuilder* builder;
	txSlot* slot;
	mxPush(mxObjectPrototype);
	slot = fxLastProperty(the, fxNewPromiseInstance(the));
	for (builder = gx_Promise_prototype_builders; builder->callback; builder++)
		slot = fxNextHostFunctionProperty(the, slot, builder->callback, builder->length, mxID(builder->id), XS_DONT_ENUM_FLAG);
	slot = fxNextStringXProperty(the, slot, "Promise", mxID(_Symbol_toStringTag), XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	mxPromisePrototype = *the->stack;
	slot = fxNewHostConstructorGlobal(the, fx_Promise, 1, mxID(_Promise), XS_DONT_ENUM_FLAG);
	mxPromiseConstructor = *the->stack;
	slot = fxLastProperty(the, slot);
	for (builder = gx_Promise_builders; builder->callback; builder++)
		slot = fxNextHostFunctionProperty(the, slot, builder->callback, builder->length, mxID(builder->id), XS_DONT_ENUM_FLAG);
	slot = fxNextHostAccessorProperty(the, slot, fx_species_get, C_NULL, mxID(_Symbol_species), XS_DONT_ENUM_FLAG);
	the->stack++;
	fxNewHostFunction(the, fxOnRejectedPromise, 1, XS_NO_ID);
	mxOnRejectedPromiseFunction = *the->stack;
	the->stack++;
	fxNewHostFunction(the, fxOnResolvedPromise, 1, XS_NO_ID);
	mxOnResolvedPromiseFunction = *the->stack;
	the->stack++;
	fxNewHostFunction(the, fxRejectPromise, 1, XS_NO_ID);
	mxRejectPromiseFunction = *the->stack;
	the->stack++;
	fxNewHostFunction(the, fxResolvePromise, 1, XS_NO_ID);
	mxResolvePromiseFunction = *the->stack;
	the->stack++;
}

void fxPushSpeciesConstructor(txMachine* the, txSlot* constructor)
{
	mxPushSlot(mxThis);
	fxGetID(the, mxID(_constructor));
	if (mxIsUndefined(the->stack)) {
		mxPop();
		mxPushSlot(constructor);
		return;
	}
	if (!mxIsReference(the->stack)) {
		mxTypeError("no constructor");
	}
	fxGetID(the, mxID(_Symbol_species));
	if (mxIsUndefined(the->stack) || mxIsNull(the->stack)) {
		mxPop();
		mxPushSlot(constructor);
		return;
	}
	if (!mxIsReference(the->stack)) {
		mxTypeError("no constructor");
	}
}

txSlot* fxNewPromiseInstance(txMachine* the)
{
	//static txID gID = -2;
	txSlot* promise;
	txSlot* slot;
	txSlot* instance;
	promise = fxNewSlot(the);
	promise->kind = XS_INSTANCE_KIND;
	promise->value.instance.garbage = C_NULL;
	promise->value.instance.prototype = the->stack->value.reference;
	the->stack->kind = XS_REFERENCE_KIND;
	the->stack->value.reference = promise;
	/* STATUS */
	slot = promise->next = fxNewSlot(the);
	slot->flag = XS_INTERNAL_FLAG | XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG;
	//slot->ID = gID++;
	slot->kind = XS_PROMISE_KIND;
	slot->value.integer = mxUndefinedStatus;
	/* THENS */
	slot = slot->next = fxNewSlot(the);
	slot->flag = XS_INTERNAL_FLAG | XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG;
	slot->value.reference = instance = fxNewSlot(the);
    slot->kind = XS_REFERENCE_KIND;
	instance->kind = XS_INSTANCE_KIND;
	instance->value.instance.garbage = C_NULL;
	instance->value.instance.prototype = C_NULL;
	/* RESULT */
	slot = slot->next = fxNewSlot(the);
	slot->flag = XS_INTERNAL_FLAG | XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG;
	//fprintf(stderr, "fxNewPromiseInstance %d\n", promise->next->ID);
	return promise;
}

txSlot* fxNewPromiseAlready(txMachine* the)
{
	txSlot* result;
	mxPushUndefined();
	result = the->stack->value.closure = fxNewSlot(the);
	the->stack->kind = XS_CLOSURE_KIND;
	result->kind = XS_BOOLEAN_KIND;
	result->value.boolean = 0;
	return result;
}

txSlot* fxNewPromiseFunction(txMachine* the, txSlot* already, txSlot* promise, txSlot* function)
{
	txSlot* result;
	txSlot* closures;
	txSlot* slot;
	result = fxNewHostFunction(the, fxCallPromise, 1, XS_NO_ID);
	closures = fxNewInstance(the);
	slot = closures->next = fxNewSlot(the);
	slot->kind = XS_CLOSURE_KIND;
	slot->value.closure = already;
	slot = slot->next = fxNewSlot(the);
	slot->kind = XS_REFERENCE_KIND;
	slot->value.reference = promise;
	slot = slot->next = fxNewSlot(the);
	slot->kind = XS_REFERENCE_KIND;
	slot->value.reference = function;
	slot = mxFunctionInstanceClosures(result);
	slot->kind = XS_REFERENCE_KIND;
	slot->value.reference = closures;
	the->stack++;
	return result;
}

txSlot* fxNewPromiseFunctionAll(txMachine* the, txSlot* already, txSlot* array, txInteger index, txSlot* count, txSlot* promise, txSlot* function)
{
	txSlot* result;
	txSlot* closures;
	txSlot* slot;
	result = fxNewHostFunction(the, fxCallPromiseAll, 1, XS_NO_ID);
	closures = fxNewInstance(the);
	slot = closures->next = fxNewSlot(the);
	slot->kind = XS_CLOSURE_KIND;
	slot->value.closure = already;
	slot = slot->next = fxNewSlot(the);
	slot->kind = XS_CLOSURE_KIND;
	slot->value.closure = array;
	slot = slot->next = fxNewSlot(the);
	slot->kind = XS_INTEGER_KIND;
	slot->value.integer = index;
	slot = slot->next = fxNewSlot(the);
	slot->kind = XS_CLOSURE_KIND;
	slot->value.closure = count;
	slot = slot->next = fxNewSlot(the);
	slot->kind = XS_REFERENCE_KIND;
	slot->value.reference = promise;
	slot = slot->next = fxNewSlot(the);
	slot->kind = XS_REFERENCE_KIND;
	slot->value.reference = function;
	slot = mxFunctionInstanceClosures(result);
	slot->kind = XS_REFERENCE_KIND;
	slot->value.reference = closures;
	the->stack++;
	return result;
}

void fxBuildPromiseCapability(txMachine* the)
{
	txSlot* closures = mxFunctionInstanceClosures(mxFunction->value.reference);
	txSlot* instance;
	txSlot* resolveFunction;
	txSlot* rejectFunction;
	if (mxIsReference(closures)) {
		instance = closures->value.reference;
		resolveFunction = instance->next;
		rejectFunction = resolveFunction->next;
		if (!mxIsUndefined(resolveFunction) || !mxIsUndefined(rejectFunction))
			mxTypeError("executor already called");
	}
	else {
		instance = fxNewInstance(the);
		resolveFunction = instance->next = fxNewSlot(the);
		rejectFunction = resolveFunction->next = fxNewSlot(the);
		mxPullSlot(closures);
	}
	if (mxArgc > 0) {
		resolveFunction->kind = mxArgv(0)->kind;
		resolveFunction->value = mxArgv(0)->value;
	}
	if (mxArgc > 1) {
		rejectFunction->kind = mxArgv(1)->kind;
		rejectFunction->value = mxArgv(1)->value;
	}
}

void fxCallPromise(txMachine* the)
{
	txSlot* slot;
	slot = mxFunctionInstanceClosures(mxFunction->value.reference)->value.reference->next;
	if (slot->value.closure->value.boolean)
		return;
	slot->value.closure->value.boolean = 1;
	if (mxArgc > 0)
		mxPushSlot(mxArgv(0));
	else
		mxPushUndefined();
	/* COUNT */
	mxPushInteger(1);
	/* THIS */
	slot = slot->next;
	mxPushSlot(slot);
	/* FUNCTION */
	slot = slot->next;
	mxPushSlot(slot);
	fxCall(the);
	mxPullSlot(mxResult);
}

void fxCallPromiseAll(txMachine* the)
{
	txSlot* slot;
	txSlot* array;
	txSlot* count;
	slot = mxFunctionInstanceClosures(mxFunction->value.reference)->value.reference->next;
	if (slot->value.closure->value.boolean)
		return;
	slot->value.closure->value.boolean = 1;
    if (mxArgc > 0)
		mxPushSlot(mxArgv(0));
    else
        mxPushUndefined();
	slot = slot->next;
	array = slot->value.closure;
	mxPushSlot(array);
	slot = slot->next;
	mxPushSlot(slot);
	fxSetAt(the);
	the->stack++;
	slot = slot->next;
	count = slot->value.closure;
	count->value.integer--;
	if (count->value.integer == 0) {
		mxPushSlot(array);
		/* COUNT */
		mxPushInteger(1);
		/* THIS */
		slot = slot->next;
		mxPushSlot(slot);
		/* FUNCTION */
		slot = slot->next;
		mxPushSlot(slot);
		fxCall(the);
		mxPullSlot(mxResult);
	}
}

void fxCheckPromiseCapability(txMachine* the, txSlot* capability, txSlot** resolveFunction, txSlot** rejectFunction)
{
	txSlot* slot = mxFunctionInstanceClosures(capability);
	txSlot* closures;
	if (!mxIsReference(slot))
		mxTypeError("executor not called");
	closures = slot->value.reference;	
	slot = closures->next;
	if (!mxIsReference(slot))
		mxTypeError("resolve is no object");
	slot = slot->value.reference;	
	if (!mxIsFunction(slot))
		mxTypeError("resolve is no function");
	*resolveFunction = slot;
	slot = closures->next->next;
	if (!mxIsReference(slot))
		mxTypeError("reject is no object");
	slot = slot->value.reference;	
	if (!mxIsFunction(slot))
		mxTypeError("reject is no function");
	*rejectFunction = slot;
}

void fxOnRejectedPromise(txMachine* the)
{
	txSlot* reaction = mxThis->value.reference;
	txSlot* resolveFunction = reaction->next;
	txSlot* rejectFunction = resolveFunction->next;
	txSlot* resolveHandler = rejectFunction->next;
	txSlot* rejectHandler = resolveHandler->next;
	txSlot* argument = mxArgv(0);
	txSlot* function = rejectFunction;
	if (rejectHandler->kind == XS_REFERENCE_KIND) {
		mxTry(the) {
			mxPushSlot(argument);
			/* COUNT */
			mxPushInteger(1);
			/* THIS */
			mxPushUndefined();
			/* FUNCTION */
			mxPushSlot(rejectHandler);
			fxCall(the);
			mxPullSlot(argument);
			function = resolveFunction;
		}
		mxCatch(the) {
			*argument = mxException;
		}
	}
	mxPushSlot(argument);
	/* COUNT */
	mxPushInteger(1);
	/* THIS */
	mxPushUndefined();
	/* FUNCTION */
	mxPushSlot(function);
	fxCall(the);
	the->stack++;
}

void fxOnResolvedPromise(txMachine* the)
{
	txSlot* reaction = mxThis->value.reference;
	txSlot* resolveFunction = reaction->next;
	txSlot* rejectFunction = resolveFunction->next;
	txSlot* resolveHandler = rejectFunction->next;
	txSlot* argument = mxArgv(0);
	txSlot* function = resolveFunction;
	if (resolveHandler->kind == XS_REFERENCE_KIND) {
		mxTry(the) {
			mxPushSlot(argument);
			/* COUNT */
			mxPushInteger(1);
			/* THIS */
			mxPushUndefined();
			/* FUNCTION */
			mxPushSlot(resolveHandler);
			fxCall(the);
			mxPullSlot(argument);
		}
		mxCatch(the) {
			*argument = mxException;
			function = rejectFunction;
		}
	}
	mxPushSlot(argument);
	/* COUNT */
	mxPushInteger(1);
	/* THIS */
	mxPushUndefined();
	/* FUNCTION */
	mxPushSlot(function);
	fxCall(the);
	the->stack++;
}

void fxRejectPromise(txMachine* the)
{
	txSlot* promise = mxThis->value.reference;
	txSlot* argument = mxArgv(0);
	txSlot* result;
	txSlot* slot;
	//fprintf(stderr, "fxRejectPromise %d\n", promise->next->ID);
	result = mxPromiseResult(promise);
	result->kind = argument->kind;
	result->value = argument->value;
	slot = mxPromiseThens(promise)->value.reference->next;
	while (slot) {
		mxPushSlot(argument);
		/* COUNT */
		mxPushInteger(1);
		/* THIS */
		mxPushReference(slot->value.reference);
		/* FUNCTION */
		mxPush(mxOnRejectedPromiseFunction);
		/* TARGET */
		mxPushUndefined();
		fxQueueJob(the, XS_NO_ID);
		slot = slot->next;
	}
	slot = mxPromiseStatus(promise);
	slot->value.integer = mxRejectedStatus;
}

void fxResolvePromise(txMachine* the)
{
	txSlot* promise = mxThis->value.reference;
	txSlot* argument = mxArgv(0);
	txSlot* slot;
	txSlot* function;
	txSlot* already;
	txSlot* result;
	//fprintf(stderr, "fxResolvePromise %d\n", promise->next->ID);
	mxTry(the) {
		if (mxIsReference(argument)) {
			if (argument->value.reference == promise)
				mxTypeError("promise resolves itself");
			mxPushSlot(argument);
			fxGetID(the, mxID(_then));
			slot = the->stack;
			if (mxIsReference(slot)) {
				function = slot->value.reference;
				if (mxIsFunction(function)) {
					already = fxNewPromiseAlready(the);
					fxNewPromiseFunction(the, already, promise, mxResolvePromiseFunction.value.reference);
					fxNewPromiseFunction(the, already, promise, mxRejectPromiseFunction.value.reference);
					/* COUNT */
					mxPushInteger(2);
					/* THIS */
					mxPushSlot(argument);
					/* FUNCTION */
					mxPushReference(function);
					/* TARGET */
					mxPushUndefined();
					fxQueueJob(the, XS_NO_ID);
					mxPop();
					mxPop();
					goto bail;
				}
			}
			mxPop();
		}
		result = mxPromiseResult(promise);
		result->kind = argument->kind;
		result->value = argument->value;
		slot = mxPromiseThens(promise)->value.reference->next;
		while (slot) {
			mxPushSlot(result);
			/* COUNT */
			mxPushInteger(1);
			/* THIS */
			mxPushReference(slot->value.reference);
			/* FUNCTION */
			mxPush(mxOnResolvedPromiseFunction);
			/* TARGET */
			mxPushUndefined();
			fxQueueJob(the, XS_NO_ID);
			slot = slot->next;
		}
		slot = mxPromiseStatus(promise);
		slot->value.integer = mxFulfilledStatus;
	}
bail:
	mxCatch(the) {
		result = mxPromiseResult(promise);
		result->kind = mxException.kind;
		result->value = mxException.value;
		slot = mxPromiseThens(promise)->value.reference->next;
		while (slot) {
			mxPushSlot(result);
			/* COUNT */
			mxPushInteger(1);
			/* THIS */
			mxPushReference(slot->value.reference);
			/* FUNCTION */
			mxPush(mxOnRejectedPromiseFunction);
			/* TARGET */
			mxPushUndefined();
			fxQueueJob(the, XS_NO_ID);
			slot = slot->next;
		}
		slot = mxPromiseStatus(promise);
		slot->value.integer = mxRejectedStatus;
	}
}

void fx_Promise(txMachine* the)
{
	txSlot* stack = the->stack;
	txSlot* promise;
	txSlot* function;
	txSlot* status;
	txSlot* already;
	txSlot* rejectFunction;
	if (mxTarget->kind == XS_UNDEFINED_KIND)
		mxTypeError("target is undefined");
	if (!mxIsReference(mxThis))
		mxTypeError("this is no object");
	promise = mxThis->value.reference;
	if (!mxIsPromise(promise))
		mxTypeError("this is no promise");
	if (mxArgc < 1)
		mxSyntaxError("no executor parameter");
	if (!mxIsReference(mxArgv(0)))
		mxTypeError("executor is no object");
	function = mxArgv(0)->value.reference;
	if (!mxIsFunction(function))
		mxTypeError("executor is no function");
	status = mxPromiseStatus(promise);
	status->value.integer = mxPendingStatus;
	already = fxNewPromiseAlready(the);
	fxNewPromiseFunction(the, already, promise, mxResolvePromiseFunction.value.reference);
	rejectFunction = fxNewPromiseFunction(the, already, promise, mxRejectPromiseFunction.value.reference);
	{
		mxTry(the) {
			/* COUNT */
			mxPushInteger(2);
			/* THIS */
			mxPushUndefined();
			/* FUNCTION */
			mxPushReference(function);
			fxCall(the);
		}
		mxCatch(the) {
			mxPush(mxException);
			/* COUNT */
			mxPushInteger(1);
			/* THIS */
			mxPushUndefined();
			/* FUNCTION */
			mxPushReference(rejectFunction);
			fxCall(the);
		}
	}
	the->stack = stack;
}

void fx_Promise_all(txMachine* the)
{
	txSlot* stack = the->stack;
	txSlot* capability;
	txSlot* promise;
	txSlot* resolveFunction;
	txSlot* rejectFunction;
	txSlot* array;
	txSlot* count;
	txSlot* iterator;
	txInteger index;
	txSlot* result;
	txSlot* argument;
	txSlot* already;
	
	if (!mxIsReference(mxThis))
		mxTypeError("this is no object");
	capability = fxNewHostFunction(the, fxBuildPromiseCapability, 2, XS_NO_ID);
	mxPushReference(capability);
	mxPushInteger(1);
	mxPushSlot(mxThis);
	fxNew(the);
	mxPullSlot(mxResult);
    promise = mxResult->value.reference;
	fxCheckPromiseCapability(the, capability, &resolveFunction, &rejectFunction);
	{
		mxTry(the) {
			mxPush(mxArrayPrototype);
			fxNewArrayInstance(the);
			mxPushUndefined();
			array = the->stack->value.closure = fxNewSlot(the);
			the->stack->kind = XS_CLOSURE_KIND;
			array->kind = XS_REFERENCE_KIND;
			array->value.reference = (the->stack + 1)->value.reference;
			
			mxPushUndefined();
			count = the->stack->value.closure = fxNewSlot(the);
			the->stack->kind = XS_CLOSURE_KIND;
			count->kind = XS_INTEGER_KIND;
			count->value.integer = 0;
			
			if (!mxIsReference(mxArgv(0)))
				mxTypeError("iterable is no object");
			mxPushInteger(0);
			mxPushSlot(mxArgv(0));
			fxCallID(the, mxID(_Symbol_iterator));
			iterator = the->stack;
			index = 0;
			{
				mxTry(the) {
					for(;;) {
						mxPushInteger(0);
						mxPushSlot(iterator);
						fxCallID(the, mxID(_next));
						result = the->stack;
						mxPushSlot(result);
						fxGetID(the, mxID(_done));	
						if (fxToBoolean(the, the->stack))
							break;
						mxPushSlot(result);
						fxGetID(the, mxID(_value));	
						mxPushInteger(1);
						mxPushSlot(mxThis);
						fxCallID(the, mxID(_resolve));
						argument = the->stack;
						already = fxNewPromiseAlready(the);
						fxNewPromiseFunctionAll(the, already, array, index, count, promise, resolveFunction);
						mxPushReference(rejectFunction);
						mxPushInteger(2);
						mxPushSlot(argument);
						fxCallID(the, mxID(_then));
						the->stack = iterator;
						index++;
					}
				}
				mxCatch(the) {
					fxCloseIterator(the, iterator);
					fxJump(the);
				}
			}
			count->value.integer += index;
			if (count->value.integer == 0) { // no elements or all elements resolved
				mxPushSlot(array);
				/* COUNT */
				mxPushInteger(1);
				/* THIS */
				mxPushUndefined();
				/* FUNCTION */
				mxPushReference(resolveFunction);
				fxCall(the);
			}
		}
		mxCatch(the) {
			mxPush(mxException);
			/* COUNT */
			mxPushInteger(1);
			/* THIS */
			mxPushUndefined();
			/* FUNCTION */
			mxPushReference(rejectFunction);
			fxCall(the);
		}
	}
	the->stack = stack;
}

void fx_Promise_race(txMachine* the)
{
	txSlot* stack = the->stack;
	txSlot* capability;
	txSlot* resolveFunction;
	txSlot* rejectFunction;
	txSlot* iterator;
	txInteger index;
	txSlot* result;
	txSlot* argument;

	if (!mxIsReference(mxThis))
		mxTypeError("this is no object");
	capability = fxNewHostFunction(the, fxBuildPromiseCapability, 2, XS_NO_ID);
	mxPushReference(capability);
	mxPushInteger(1);
	mxPushSlot(mxThis);
	fxNew(the);
	mxPullSlot(mxResult);
	fxCheckPromiseCapability(the, capability, &resolveFunction, &rejectFunction);
	{
		mxTry(the) {
			if (!mxIsReference(mxArgv(0)))
				mxTypeError("iterable is no object");
			mxPushInteger(0);
			mxPushSlot(mxArgv(0));
			fxCallID(the, mxID(_Symbol_iterator));
			iterator = the->stack;
			index = 0;
			{
				mxTry(the) {
					for(;;) {
						mxPushInteger(0);
						mxPushSlot(iterator);
						fxCallID(the, mxID(_next));
						result = the->stack;
						mxPushSlot(result);
						fxGetID(the, mxID(_done));	
						if (fxToBoolean(the, the->stack))
							break;
						mxPushSlot(result);
						fxGetID(the, mxID(_value));	
						mxPushInteger(1);
						mxPushSlot(mxThis);
						fxCallID(the, mxID(_resolve));
						argument = the->stack;
						mxPushReference(resolveFunction);
						mxPushReference(rejectFunction);
						mxPushInteger(2);
						mxPushSlot(argument);
						fxCallID(the, mxID(_then));
						the->stack = iterator;
						index++;
					}
				}
				mxCatch(the) {
					fxCloseIterator(the, iterator);
					fxJump(the);
				}
			}
			if (index == 0) {
				mxPushUndefined();
				/* COUNT */
				mxPushInteger(1);
				/* THIS */
				mxPushUndefined();
				/* FUNCTION */
				mxPushReference(resolveFunction);
				fxCall(the);
			}
		}
		mxCatch(the) {
			mxPush(mxException);
			/* COUNT */
			mxPushInteger(1);
			/* THIS */
			mxPushUndefined();
			/* FUNCTION */
			mxPushReference(rejectFunction);
			fxCall(the);
		}
	}
	the->stack = stack;
}

void fx_Promise_reject(txMachine* the)
{
	txSlot* capability;
	txSlot* resolveFunction;
	txSlot* rejectFunction;

	if (!mxIsReference(mxThis))
		mxTypeError("this is no object");
	capability = fxNewHostFunction(the, fxBuildPromiseCapability, 2, XS_NO_ID);
	mxPushReference(capability);
	mxPushInteger(1);
	mxPushSlot(mxThis);
	fxNew(the);
	mxPullSlot(mxResult);
	fxCheckPromiseCapability(the, capability, &resolveFunction, &rejectFunction);
	if (mxArgc > 0)
		mxPushSlot(mxArgv(0));
	else
		mxPushUndefined();
	/* COUNT */
	mxPushInteger(1);
	/* THIS */
	mxPushUndefined();
	/* FUNCTION */
	mxPushReference(rejectFunction);
	fxCall(the);
	mxPop();
	mxPop(); // capability
}

void fx_Promise_resolve(txMachine* the)
{
	txSlot* capability;
	txSlot* resolveFunction;
	txSlot* rejectFunction;

	if (!mxIsReference(mxThis))
		mxTypeError("this is no object");
	if ((mxArgc > 0) && mxIsReference(mxArgv(0))) {
		txSlot* promise = mxArgv(0)->value.reference;
		if (mxIsPromise(promise)) {
			mxPushReference(promise);
			fxGetID(the, mxID(_constructor));
			if (fxIsSameValue(the, mxThis, the->stack)) {
				*mxResult = *mxArgv(0);
				return;
			}
			mxPop();
		}
	}
	capability = fxNewHostFunction(the, fxBuildPromiseCapability, 2, XS_NO_ID);
	mxPushReference(capability);
	mxPushInteger(1);
	mxPushSlot(mxThis);
	fxNew(the);
	mxPullSlot(mxResult);
	fxCheckPromiseCapability(the, capability, &resolveFunction, &rejectFunction);
	if (mxArgc > 0)
		mxPushSlot(mxArgv(0));
	else
		mxPushUndefined();
	/* COUNT */
	mxPushInteger(1);
	/* THIS */
	mxPushUndefined();
	/* FUNCTION */
	mxPushReference(resolveFunction);
	fxCall(the);
	mxPop();
	mxPop(); // capability
}

void fx_Promise_prototype_catch(txMachine* the)
{
	mxPushUndefined();
	if (mxArgc > 0) 
		mxPushSlot(mxArgv(0));
	else
		mxPushUndefined();
	mxPushInteger(2);
	mxPushSlot(mxThis);
	fxCallID(the, mxID(_then));
	mxPullSlot(mxResult);
}

void fx_Promise_prototype_dumpAux(txMachine* the, txSlot* promise, txInteger c)
{
	txInteger i;
	txSlot* reference;
	for (i = 0; i < c; i++)
		fprintf(stderr, "\t");
	fprintf(stderr, "promise %d\n", promise->next->ID);
	reference = mxPromiseThens(promise)->value.reference->next;
    c++;
	while (reference) {
		fx_Promise_prototype_dumpAux(the, reference->value.reference, c);
		reference = reference->next;
	}
}

void fx_Promise_prototype_then(txMachine* the)
{
	txSlot* promise;
	txSlot* capability;
	txSlot* resolveFunction;
	txSlot* rejectFunction;
	txSlot* reaction;
	txSlot* slot;
	txSlot* status;

	if (!mxIsReference(mxThis))
		mxTypeError("this is no object");
	promise = mxThis->value.reference;
	if (!mxIsPromise(promise))
		mxTypeError("this is no promise");
		
	capability = fxNewHostFunction(the, fxBuildPromiseCapability, 2, XS_NO_ID);
	mxPushReference(capability);
	mxPushInteger(1);
	fxPushSpeciesConstructor(the, &mxPromiseConstructor);
	fxNew(the);
	mxPullSlot(mxResult);
	fxCheckPromiseCapability(the, capability, &resolveFunction, &rejectFunction);
		
	reaction = fxNewInstance(the);
	slot = reaction->next = fxNewSlot(the);
	slot->kind = XS_REFERENCE_KIND;
	slot->value.reference = resolveFunction;
	slot = slot->next = fxNewSlot(the);
	slot->kind = XS_REFERENCE_KIND;
	slot->value.reference = rejectFunction;
	slot = slot->next = fxNewSlot(the);
	if ((mxArgc > 0) && mxIsReference(mxArgv(0))) {
		slot->kind = XS_REFERENCE_KIND;
		slot->value.reference = mxArgv(0)->value.reference;
	}
	slot = slot->next = fxNewSlot(the);
	if ((mxArgc > 1) && mxIsReference(mxArgv(1))) {
		slot->kind = XS_REFERENCE_KIND;
		slot->value.reference = mxArgv(1)->value.reference;
	}
		
	status = mxPromiseStatus(promise);
	if (status->value.integer == mxPendingStatus) {
		txSlot** address = &(mxPromiseThens(promise)->value.reference->next);
		while ((slot = *address)) 
			address = &(slot->next);
		slot = *address = fxNewSlot(the);
		slot->kind = XS_REFERENCE_KIND;
		slot->value.reference = reaction;
	}
	else {
		slot = mxPromiseResult(promise);
		mxPushSlot(slot);
		/* COUNT */
		mxPushInteger(1);
		/* THIS */
		mxPushReference(reaction);
		/* FUNCTION */
		if (status->value.integer == mxFulfilledStatus)
			mxPush(mxOnResolvedPromiseFunction);
		else
			mxPush(mxOnRejectedPromiseFunction);
        /* TARGET */
		mxPushUndefined();
		fxQueueJob(the, XS_NO_ID);
	}
	mxPop(); // reaction
	mxPop(); // capability
}

void fxQueueJob(txMachine* the, txID id)
{
	txInteger count, index;
	txSlot* job;
	txSlot* stack;
	txSlot* slot;
	txSlot** address;
	
	if (mxPendingJobs.value.reference->next == NULL) {
		fxQueuePromiseJobs(the);
	}
	job = fxNewInstance(the);
	stack = the->stack + 4;
	slot = job->next = fxNewSlot(the);
	slot->ID = id;
	slot->kind = XS_INTEGER_KIND;
	count = slot->value.integer = stack->value.integer;
	stack += count;
	for (index = 0; index < count; index++) {
		slot = slot->next = fxNewSlot(the);
		slot->kind = stack->kind;
		slot->value = stack->value;
		stack--;
	}
	slot = slot->next = fxNewSlot(the);
	slot->kind = stack->kind;
	slot->value = stack->value;
	stack--;
	slot = slot->next = fxNewSlot(the);
	slot->kind = stack->kind;
	slot->value = stack->value;
	stack--;
	slot = slot->next = fxNewSlot(the);
	slot->kind = stack->kind;
	slot->value = stack->value;
	stack--;
	slot = slot->next = fxNewSlot(the);
	slot->kind = stack->kind;
	slot->value = stack->value;
	
	address = &(mxPendingJobs.value.reference->next);
	while ((slot = *address)) 
		address = &(slot->next);
	slot = *address = fxNewSlot(the);	
	slot->kind = XS_REFERENCE_KIND;
	slot->value.reference = job;
	the->stack += 5 + count;
}

void fxRunPromiseJobs(txMachine* the)
{
	txInteger count, index;
	txSlot* job;
	txSlot* slot;
	txID id;
	
	job = mxRunningJobs.value.reference->next = mxPendingJobs.value.reference->next;
	mxPendingJobs.value.reference->next = C_NULL;
	while (job) {
		mxTry(the) {
			slot = job->value.reference->next;
			id = slot->ID;
			count = slot->value.integer;
			for (index = 0; index < count; index++) {
				slot = slot->next;
				mxPushSlot(slot);
			}
			/* COUNT */
			slot = slot->next;
			mxPushSlot(slot);
			/* THIS */
			slot = slot->next;
			mxPushSlot(slot);
			/* FUNCTION */
			slot = slot->next;
			mxPushSlot(slot);
			/* TARGET */
			slot = slot->next;
			mxPushSlot(slot);
			/* RESULT */
			mxPushUndefined();
			fxRunID(the, C_NULL, id);
			the->stack++;
		}
		mxCatch(the) {
		}
		job = job->next;
	}
	mxRunningJobs.value.reference->next = C_NULL;
}





