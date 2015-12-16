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

//#define mxTrace 1
//#define mxTraceCall 1

#define c_iszero(NUMBER) (FP_ZERO == c_fpclassify(NUMBER))
static txBoolean fxIsScopableSlot(txMachine* the, txSlot* instance, txInteger id);

extern void fxRemapIDs(txMachine* the, txByte* codeBuffer, txSize codeSize, txID* theIDs);
static void fxRunEval(txMachine* the);
static void fxRunInstance(txMachine* the, txSlot* instance);

#if defined(__GNUC__) && defined(__OPTIMIZE__)
	#if defined(mxFrequency)
		#define mxBreak \
			the->frequencies[byte]++; \
			goto *bytes[byte]
	#elif defined(mxTrace)
		#define mxBreak \
			if (gxDoTrace) fxTraceCode(the, byte); \
			goto *bytes[byte]
	#else
		#define mxBreak \
			goto *bytes[byte]
	#endif
	#define mxCase(OPCODE) OPCODE:
	#define mxSwitch(OPCODE) mxBreak;
#else
	#define mxBreak \
		break
	#define mxCase(OPCODE) \
		case OPCODE:
	#if defined(mxFrequency)
		#define mxSwitch(OPCODE) \
			the->frequencies[OPCODE]++; \
			switch(OPCODE)
	#else
		#define mxSwitch(OPCODE) \
			switch(OPCODE)
	#endif
#endif

#define mxCode code
#define mxFrame frame
#define mxScope scope
#define mxStack stack

#define mxFrameArgv(THE_INDEX) (mxFrame + 5 + ((mxFrame + 5)->value.integer) - (THE_INDEX))
#define mxFrameArgc ((mxFrame + 5)->value.integer)
#define mxFrameThis (mxFrame + 4)
#define mxFrameFunction (mxFrame + 3)
#define mxFrameTarget (mxFrame + 2)
#define mxFrameResult (mxFrame + 1)
#define mxFrameEnvironment (mxFrame - 1)

#define mxRestoreState { \
	txSlot** it = (txSlot**)the; \
	mxStack = *it++; \
	mxScope = *it++; \
	mxFrame = *it++; \
	mxCode = *((txByte**)it); \
}
#if defined(mxFrequency)
	#define mxSaveState { \
		the->exits[byte]++; \
		the->stack = mxStack; \
		the->scope = mxScope; \
		the->frame = mxFrame; \
		the->code = mxCode; \
	}
#else
	#define mxSaveState { \
		txSlot** it = (txSlot**)the; \
		*it++ = mxStack; \
		*it++ =	mxScope; \
		*it++ = mxFrame; \
		*it = (txSlot*)mxCode; \
	}
#endif

#ifdef mxDebug
#define mxOverflow(_COUNT) \
	if ((mxStack - _COUNT) < the->stackBottom) { \
		mxSaveState; \
		fxReport(the, "stack overflow (%ld)!\n", ((the->stack - _COUNT) - the->stackBottom)); \
		fxJump(the); \
	} \
	mxStack -= _COUNT
#else
#define mxOverflow(_COUNT) \
	mxStack -= _COUNT
#endif

#define mxIsCallable(THE_SLOT) \
	( (THE_SLOT) &&  ((THE_SLOT)->flag & XS_VALUE_FLAG) && (((THE_SLOT)->next->kind == XS_CALLBACK_KIND) || ((THE_SLOT)->next->kind == XS_CODE_KIND) || ((THE_SLOT)->next->kind == XS_CODE_X_KIND) || ((THE_SLOT)->next->kind == XS_PROXY_KIND)))
	
#define mxPushKind(_KIND) { \
	mxOverflow(1); \
	mxStack->next = C_NULL;  \
	mxStack->flag = XS_NO_FLAG;  \
	mxStack->kind = _KIND; \
}

#define mxRunDebug(_ERROR, ...) { \
	mxSaveState; \
	fxThrowMessage(the, NULL, 0, _ERROR, __VA_ARGS__); \
}

#define mxRunDebugID(_ERROR, _MESSAGE, _ID) { \
	mxSaveState; \
	fxIDToString(the, _ID, the->nameBuffer, sizeof(the->nameBuffer)); \
	fxThrowMessage(the, NULL, 0, _ERROR, _MESSAGE, the->nameBuffer); \
}

#define mxToBoolean(SLOT) \
	if (XS_BOOLEAN_KIND != (SLOT)->kind) { \
		if (XS_SYMBOL_KIND <= (SLOT)->kind) { \
			mxSaveState; \
			fxToBoolean(the, SLOT); \
			mxRestoreState; \
		} \
		else \
			fxToBoolean(the, SLOT); \
	}

#define mxToInteger(SLOT) \
	if (XS_INTEGER_KIND != (SLOT)->kind) { \
		if (XS_SYMBOL_KIND <= (SLOT)->kind) { \
			mxSaveState; \
			fxToInteger(the, SLOT); \
			mxRestoreState; \
		} \
		else \
			fxToInteger(the, SLOT); \
	}

#define mxToNumber(SLOT) \
	if (XS_NUMBER_KIND != (SLOT)->kind) { \
		if (XS_SYMBOL_KIND <= (SLOT)->kind) { \
			mxSaveState; \
			fxToNumber(the, SLOT); \
			mxRestoreState; \
		} \
		else \
			fxToNumber(the, SLOT); \
	}
	
#define mxToString(SLOT) \
	if ((XS_STRING_KIND != (SLOT)->kind) && (XS_STRING_X_KIND != (SLOT)->kind)) { \
		mxSaveState; \
		fxToString(the, SLOT); \
		mxRestoreState; \
	}
	
#define mxToInstance(SLOT) \
	if (XS_REFERENCE_KIND != (SLOT)->kind) { \
		mxSaveState; \
		fxToInstance(the, SLOT); \
		mxRestoreState; \
	}
		
#define mxNextCode(OFFSET) { \
	mxCode += OFFSET; \
	byte = *((txU1*)mxCode); \
}
#define mxSkipCode(OFFSET) { \
	mxCode += OFFSET; \
}
#define mxRunS1(OFFSET) ((txS1*)mxCode)[OFFSET+0]
#define mxRunS2(OFFSET) (((txS1*)mxCode)[OFFSET+0] << 8) | ((txU1*)mxCode)[OFFSET+1]
#define mxRunS4(OFFSET) (((txS1*)mxCode)[OFFSET+0] << 24) | (((txU1*)mxCode)[OFFSET+1] << 16) | (((txU1*)mxCode)[OFFSET+2] << 8) | ((txU1*)mxCode)[OFFSET+3]

#define mxRunU1(OFFSET) ((txU1*)mxCode)[OFFSET]
#define mxRunU2(OFFSET) (((txU1*)mxCode)[OFFSET+0] << 8) | ((txU1*)mxCode)[OFFSET+1]

#ifdef mxTrace
short gxDoTrace = 1;

static void fxTraceCode(txMachine* the, txU1 theCode) 
{
	if (((XS_NO_CODE < theCode) && (theCode < XS_CODE_COUNT)))
		fprintf(stderr, "\n%s", gxCodeNames[theCode]);
	else
		fprintf(stderr, "\n?");
}

static void fxTraceID(txMachine* the, txInteger id) 
{
	if (id >= 0)
		fprintf(stderr, " [%ld]", id);
	else {	
		txSlot* key = fxGetKey(the, id);
		if (key)
			fprintf(stderr, " [%s]", key->value.key.string);
		else
			fprintf(stderr, " [?]");
	}
}

static void fxTraceIndex(txMachine* the, txU2 theIndex) 
{
	fprintf(stderr, " %d", theIndex);
}

static void fxTraceInteger(txMachine* the, txInteger theInteger) 
{
	fprintf(stderr, " %ld", theInteger);
}

static void fxTraceNumber(txMachine* the, txNumber theNumber) 
{
	fprintf(stderr, " %lf", theNumber);
}

static void fxTraceReturn(txMachine* the) 
{
	fprintf(stderr, "\n");
}

static void fxTraceString(txMachine* the, txString theString) 
{
	fprintf(stderr, " \"%s\"", theString);
}
#endif

#ifdef mxTraceCall
int depth = 0;

static void fxTraceCallBegin(txMachine* the, txSlot* function)
{
	txSlot* slot = fxGetProperty(the, function->value.reference, mxID(_name));
	int i;
	for (i = 0; i < depth; i++)
		fprintf(stderr, "\t");
	if (slot && (slot->kind == XS_STRING_KIND) &&  (slot->kind == XS_STRING_X_KIND))
		fprintf(stderr, " [%s]\n", slot->value.string);
	else
		fprintf(stderr, " [?]\n");
	depth++;
}
static void fxTraceCallEnd(txMachine* the, txSlot* function)
{
	depth--;
}
#endif


void fxRunID(txMachine* the, txSlot* generator, txID id)
{
	register txSlot* stack = the->stack;
	register txSlot* scope = the->scope;
	register txSlot* frame = the->frame;
	register txByte* code = the->code;
	register txSlot* variable;
	register txSlot* slot;
	register txU1 byte = 0;
	register txU2 index;
	register txS4 offset;
#if defined(__GNUC__) && defined(__OPTIMIZE__)
	static void *const gxBytes[] = {
		&&XS_NO_CODE,
		&&XS_CODE_ADD,
		&&XS_CODE_ARGUMENT,
		&&XS_CODE_ARGUMENTS,
		&&XS_CODE_ARGUMENTS_SLOPPY,
		&&XS_CODE_ARGUMENTS_STRICT,
		&&XS_CODE_ARRAY,
		&&XS_CODE_ARROW,
		&&XS_CODE_AT,
		&&XS_CODE_BEGIN_SLOPPY,
		&&XS_CODE_BEGIN_STRICT,
		&&XS_CODE_BEGIN_STRICT_BASE,
		&&XS_CODE_BEGIN_STRICT_DERIVED,
		&&XS_CODE_BIT_AND,
		&&XS_CODE_BIT_NOT,
		&&XS_CODE_BIT_OR,
		&&XS_CODE_BIT_XOR,
		&&XS_CODE_BRANCH_1,
		&&XS_CODE_BRANCH_2,
		&&XS_CODE_BRANCH_4,
		&&XS_CODE_BRANCH_ELSE_1,
		&&XS_CODE_BRANCH_ELSE_2,
		&&XS_CODE_BRANCH_ELSE_4,
		&&XS_CODE_BRANCH_IF_1,
		&&XS_CODE_BRANCH_IF_2,
		&&XS_CODE_BRANCH_IF_4,
		&&XS_CODE_BRANCH_STATUS_1,
		&&XS_CODE_BRANCH_STATUS_2,
		&&XS_CODE_BRANCH_STATUS_4,
		&&XS_CODE_CALL,
		&&XS_CODE_CALL_TAIL,
		&&XS_CODE_CATCH_1,
		&&XS_CODE_CATCH_2,
		&&XS_CODE_CATCH_4,
		&&XS_CODE_CLASS,
		&&XS_CODE_CODE_1,
		&&XS_CODE_CODE_2,
		&&XS_CODE_CODE_4,
		&&XS_CODE_CODE_ARCHIVE_1,
		&&XS_CODE_CODE_ARCHIVE_2,
		&&XS_CODE_CODE_ARCHIVE_4,
		&&XS_CODE_CONST_CLOSURE_1,
		&&XS_CODE_CONST_CLOSURE_2,
		&&XS_CODE_CONST_GLOBAL,
		&&XS_CODE_CONST_LOCAL_1,
		&&XS_CODE_CONST_LOCAL_2,
		&&XS_CODE_CURRENT,
		&&XS_CODE_DEBUGGER,
		&&XS_CODE_DECREMENT,
		&&XS_CODE_DELETE_CLOSURE_1,
		&&XS_CODE_DELETE_CLOSURE_2,
		&&XS_CODE_DELETE_EVAL,
		&&XS_CODE_DELETE_GLOBAL,
		&&XS_CODE_DELETE_LOCAL_1,
		&&XS_CODE_DELETE_LOCAL_2,
		&&XS_CODE_DELETE_PROPERTY,
		&&XS_CODE_DELETE_PROPERTY_AT,
		&&XS_CODE_DELETE_SUPER,
		&&XS_CODE_DELETE_SUPER_AT,
		&&XS_CODE_DIVIDE,
		&&XS_CODE_DUB,
		&&XS_CODE_END,
		&&XS_CODE_EQUAL,
		&&XS_CODE_EVAL,
		&&XS_CODE_EXCEPTION,
		&&XS_CODE_EXTEND,
		&&XS_CODE_FALSE,
		&&XS_CODE_FILE,
		&&XS_CODE_FOR_IN,
		&&XS_CODE_FOR_OF,
		&&XS_CODE_FUNCTION,
		&&XS_CODE_GENERATOR,
		&&XS_CODE_GET_CLOSURE_1,
		&&XS_CODE_GET_CLOSURE_2,
		&&XS_CODE_GET_EVAL,
		&&XS_CODE_GET_GLOBAL,
		&&XS_CODE_GET_HOME,
		&&XS_CODE_GET_LOCAL_1,
		&&XS_CODE_GET_LOCAL_2,
		&&XS_CODE_GET_PROPERTY,
		&&XS_CODE_GET_PROPERTY_AT,
		&&XS_CODE_GET_SUPER,
		&&XS_CODE_GET_SUPER_AT,
		&&XS_CODE_GET_THIS,
		&&XS_CODE_HOST,
		&&XS_CODE_IN,
		&&XS_CODE_INCREMENT,
		&&XS_CODE_INSTANCEOF,
		&&XS_CODE_INSTANTIATE,
		&&XS_CODE_INTEGER_1,
		&&XS_CODE_INTEGER_2,
		&&XS_CODE_INTEGER_4,
		&&XS_CODE_LEFT_SHIFT,
		&&XS_CODE_LESS,
		&&XS_CODE_LESS_EQUAL,
		&&XS_CODE_LINE,
		&&XS_CODE_MINUS,
		&&XS_CODE_MODULE,
		&&XS_CODE_MODULO,
		&&XS_CODE_MORE,
		&&XS_CODE_MORE_EQUAL,
		&&XS_CODE_MULTIPLY,
		&&XS_CODE_NEW,
		&&XS_CODE_NEW_CLOSURE,
		&&XS_CODE_NEW_EVAL,
		&&XS_CODE_NEW_GLOBAL,
		&&XS_CODE_NEW_LOCAL,
		&&XS_CODE_NEW_PROPERTY,
		&&XS_CODE_NEW_TEMPORARY,
		&&XS_CODE_NOT,
		&&XS_CODE_NOT_EQUAL,
		&&XS_CODE_NULL,
		&&XS_CODE_NUMBER,
		&&XS_CODE_OBJECT,
		&&XS_CODE_PLUS,
		&&XS_CODE_POP,
		&&XS_CODE_PULL_CLOSURE_1,
		&&XS_CODE_PULL_CLOSURE_2,
		&&XS_CODE_PULL_LOCAL_1,
		&&XS_CODE_PULL_LOCAL_2,
		&&XS_CODE_RESULT,
		&&XS_CODE_RESERVE_1,
		&&XS_CODE_RESERVE_2,
		&&XS_CODE_RETHROW,
		&&XS_CODE_RETRIEVE_1,
		&&XS_CODE_RETRIEVE_2,
		&&XS_CODE_RETRIEVE_TARGET,
		&&XS_CODE_RETRIEVE_THIS,
		&&XS_CODE_RETURN,
		&&XS_CODE_SCOPE,
		&&XS_CODE_SET_CLOSURE_1,
		&&XS_CODE_SET_CLOSURE_2,
		&&XS_CODE_SET_EVAL,
		&&XS_CODE_SET_GLOBAL,
		&&XS_CODE_SET_HOME,
		&&XS_CODE_SET_LOCAL_1,
		&&XS_CODE_SET_LOCAL_2,
		&&XS_CODE_SET_PROPERTY,
		&&XS_CODE_SET_PROPERTY_AT,
		&&XS_CODE_SET_SUPER,
		&&XS_CODE_SET_SUPER_AT,
		&&XS_CODE_SET_THIS,
		&&XS_CODE_SIGNED_RIGHT_SHIFT,
		&&XS_CODE_START,
		&&XS_CODE_STORE_1,
		&&XS_CODE_STORE_2,
		&&XS_CODE_STORE_TARGET,
		&&XS_CODE_STORE_THIS,
		&&XS_CODE_STRICT_EQUAL,
		&&XS_CODE_STRICT_NOT_EQUAL,
		&&XS_CODE_STRING_1,
		&&XS_CODE_STRING_2,
		&&XS_CODE_STRING_ARCHIVE_1,
		&&XS_CODE_STRING_ARCHIVE_2,
		&&XS_CODE_SUBTRACT,
		&&XS_CODE_SUPER,
		&&XS_CODE_SWAP,
		&&XS_CODE_SYMBOL,
		&&XS_CODE_TARGET,
		&&XS_CODE_THROW,
		&&XS_CODE_TRANSFER,
		&&XS_CODE_TRUE,
		&&XS_CODE_TYPEOF,
		&&XS_CODE_UNCATCH,
		&&XS_CODE_UNDEFINED,
		&&XS_CODE_UNSIGNED_RIGHT_SHIFT,
		&&XS_CODE_UNWIND_1,
		&&XS_CODE_UNWIND_2,
		&&XS_CODE_VOID,
		&&XS_CODE_WITH,
		&&XS_CODE_WITHOUT,
		&&XS_CODE_YIELD
	};
	register void * const *bytes = gxBytes;
#endif
	txJump* jump = C_NULL;
	txSlot scratch;
	txSlot** address;
	txSlot* formerGenerator = C_NULL;
	txJump* formerJump = the->firstJump;
	
	if (generator) {
		slot = mxStack;
		variable = generator->next;
		offset = variable->value.array.length;
		mxOverflow(offset);
		c_memcpy(mxStack, variable->value.array.address, offset * sizeof(txSlot));
		mxCode = (mxStack++)->value.reference->next->value.code;
		offset = (mxStack++)->value.integer;
		while (offset) {
			jump = c_malloc(sizeof(txJump));
			if (jump) {
				jump->nextJump = the->firstJump;
				jump->stack = slot - (mxStack++)->value.integer;
				jump->frame = slot - (mxStack++)->value.integer;
				jump->scope = slot - (mxStack++)->value.integer;
				jump->code = mxCode + (mxStack++)->value.integer;
				jump->flag = 1;
                the->firstJump = jump;
				if (c_setjmp(jump->buffer) == 1) {
					jump = the->firstJump;
                    the->firstJump = jump->nextJump;
					mxStack = jump->stack;
					mxFrame = jump->frame;
					mxScope = jump->scope;
					mxCode = jump->code;
					c_free(jump);
                    goto XS_CODE_JUMP;
				}
			}
			else {
				mxSaveState;
				fxJump(the);
			}
			offset--;
		}
		variable = slot - (mxStack++)->value.integer;
		variable->next = mxFrame;
		variable->flag = XS_NO_FLAG;
#ifdef mxDebug
		if (mxFrame && (mxFrame->flag & XS_STEP_INTO_FLAG))
			variable->flag |= XS_STEP_INTO_FLAG | XS_STEP_OVER_FLAG;
#endif
		variable->value.frame.code = mxCode;
		variable->value.frame.scope = mxScope;
		mxFrame = variable;
		mxScope = slot - (mxStack++)->value.integer;
		mxCode = mxCode + (mxStack++)->value.integer;
		mxStack->kind = the->scratch.kind;
		mxStack->value = the->scratch.value;
		
		generator->next->next->value.reference->next->next->value.boolean = 1; // done
		mxFrameResult->kind = XS_UNDEFINED_KIND;
#ifdef mxTraceCall
		fxTraceCallBegin(the, mxFrameFunction);
#endif
#ifdef mxProfile
		fxBeginFunction(the, mxFrameFunction);
#endif
XS_CODE_JUMP:
		byte = *((txU1*)mxCode);
	}
	else {
		slot = (mxStack + 2)->value.reference;
		offset = XS_NO_ID;
		goto XS_CODE_CALL_ALL;
	}
	for (;;) {

#ifdef mxTrace
		if (gxDoTrace) fxTraceCode(the, byte);
#endif
		//fxCheckStack(the, mxStack);
		
		mxSwitch(byte) {
		mxCase(XS_NO_CODE)
			mxBreak;
			
		mxCase(XS_CODE_BEGIN_SLOPPY)
			index = mxFrameThis->kind;
			if (index < XS_REFERENCE_KIND) {
				if ((index == XS_UNDEFINED_KIND) || (index == XS_NULL_KIND))
					*mxFrameThis = mxGlobal;
				else {
					mxSaveState;
					fxToInstance(the, mxFrameThis);
					mxRestoreState;
				}
			}
			mxNextCode(2);
			mxBreak;
		mxCase(XS_CODE_BEGIN_STRICT)
			mxFrame->flag |= XS_STRICT_FLAG;
			mxNextCode(2);
			mxBreak;
		mxCase(XS_CODE_BEGIN_STRICT_BASE)
			mxFrame->flag |= XS_STRICT_FLAG;
			slot = mxFrameTarget;
            if (slot->kind == XS_UNDEFINED_KIND)
				mxRunDebug(XS_TYPE_ERROR, "call: class constructor");
			mxNextCode(2);
			mxBreak;
		mxCase(XS_CODE_BEGIN_STRICT_DERIVED)
			mxFrame->flag |= XS_STRICT_FLAG;
			slot = mxFrameTarget;
            if (slot->kind == XS_UNDEFINED_KIND)
				mxRunDebug(XS_TYPE_ERROR, "call: class constructor");
			mxNextCode(2);
			mxBreak;
			
		mxCase(XS_CODE_CALL)
			mxSkipCode(1);
			slot = fxGetInstance(the, mxStack);
            if (!mxIsCallable(slot))
				mxRunDebugID(XS_TYPE_ERROR, "call %s: no function", offset);
			/* TARGET */
			mxPushKind(XS_UNDEFINED_KIND);
			/* RESULT */
			mxPushKind(XS_UNDEFINED_KIND);
		XS_CODE_CALL_ALL:
			/* FRAME */
			mxOverflow(1);
			mxStack->kind = XS_FRAME_KIND;
			mxStack->next = mxFrame;
			mxStack->flag = XS_NO_FLAG;
#ifdef mxDebug
			if (mxFrame && (mxFrame->flag & XS_STEP_INTO_FLAG))
				mxStack->flag |= XS_STEP_INTO_FLAG | XS_STEP_OVER_FLAG;
#endif
			mxStack->value.frame.code = mxCode;
			mxStack->value.frame.scope = mxScope;
		XS_CODE_CALL_STACK:
			mxFrame = mxStack;
#ifdef mxTraceCall
		fxTraceCallBegin(the, mxFrameFunction);
#endif
#ifdef mxProfile
			fxBeginFunction(the, mxFrameFunction);
#endif
			if (slot->next->kind == XS_CALLBACK_KIND) {
				mxFrame->flag |= XS_C_FLAG;
				mxPushKind(XS_VAR_KIND);
				mxStack->value.integer = 0;
				mxScope = mxStack;
				mxCode = (txByte*)slot->next->value.callback.IDs;
				byte = XS_CODE_CALL;
				mxSaveState;
				(*(slot->next->value.callback.address))(the);
				mxRestoreState;
				goto XS_CODE_END_ALL;
			}
			if (slot->next->kind == XS_PROXY_KIND) {
				mxFrame->flag |= XS_C_FLAG;
				mxPushKind(XS_VAR_KIND);
				mxStack->value.integer = 0;
				mxScope = mxStack;
				mxCode = C_NULL;
				byte = XS_CODE_CALL;
				mxSaveState;
				fxRunInstance(the, slot);
				mxRestoreState;
				goto XS_CODE_END_ALL;
			}
			variable = mxFunctionInstanceClosures(slot);
			mxPushKind(variable->kind);
			mxStack->value = variable->value;
			mxScope = mxStack;
			mxCode = slot->next->value.code;
			byte = *((txU1*)mxCode);
			mxBreak;
		mxCase(XS_CODE_CALL_TAIL)
			mxSkipCode(1);
			slot = fxGetInstance(the, mxStack);
            if (!slot)
				mxRunDebugID(XS_TYPE_ERROR, "call %s: no function", offset);
			/* TARGET */
			mxPushKind(XS_UNDEFINED_KIND);
			/* RESULT */
			mxPushKind(XS_UNDEFINED_KIND);
			/* FRAME */
			mxOverflow(1);
			mxStack->kind = XS_FRAME_KIND;
			mxStack->next = mxFrame->next;
			mxStack->flag = XS_NO_FLAG;
#ifdef mxDebug
			if (mxFrame && (mxFrame->flag & XS_STEP_INTO_FLAG))
				mxStack->flag |= XS_STEP_INTO_FLAG | XS_STEP_OVER_FLAG;
#endif
			mxStack->value.frame.code = mxFrame->value.frame.code;
			mxStack->value.frame.scope = mxFrame->value.frame.scope;
#ifdef mxTraceCall
			fxTraceCallEnd(the, mxFrameFunction);
#endif
#ifdef mxProfile
			fxEndFunction(the, mxFrameFunction);
#endif
			offset = 6 + ((mxStack + 5)->value.integer);
			variable = mxFrame + 6 + ((mxFrame + 5)->value.integer) - offset;
			c_memmove(variable, mxStack, offset * sizeof(txSlot));
			mxStack = variable;
			goto XS_CODE_CALL_STACK;
		mxCase(XS_CODE_END)
		XS_CODE_END_ALL:
#ifdef mxTraceCall
			fxTraceCallEnd(the, mxFrameFunction);
#endif
#ifdef mxProfile
			fxEndFunction(the, mxFrameFunction);
#endif
			mxStack = mxFrameArgv(-1);
			mxScope = mxFrame->value.frame.scope;
			mxCode = mxFrame->value.frame.code;
			slot = mxFrameResult;
			mxOverflow(1);
			if (mxFrameTarget->kind) {
				if (slot->kind == XS_REFERENCE_KIND)
					*mxStack = *slot;
				else {
					*mxStack = *mxFrameThis;
					if (mxStack->kind < 0)
						mxRunDebug(XS_REFERENCE_ERROR, "this is not initialized");
				}
			}
			else
				*mxStack = *slot;
			mxFrame = mxFrame->next;
			if (!mxFrame || (mxFrame->flag & XS_C_FLAG)) {
#ifdef mxTrace
				if (gxDoTrace) fxTraceReturn(the);
#endif
				byte = XS_CODE_END;
				mxSaveState;
				return;	
			}
			byte = *((txU1*)mxCode);
			mxBreak;
		mxCase(XS_CODE_RETURN)
#ifdef mxProfile
			fxEndFunction(the, mxFrameFunction);
#endif
			mxStack = mxFrameArgv(-1);
			mxScope = mxFrame->value.frame.scope;
			mxCode = mxFrame->value.frame.code;
			mxOverflow(1);
			*mxStack = *mxFrameResult;
			mxFrame = mxFrame->next;
#ifdef mxTrace
			if (gxDoTrace)
				if (gxDoTrace) fxTraceReturn(the);
#endif
			mxSaveState;
			return;	
		mxCase(XS_CODE_START)
			formerGenerator = generator;
			formerJump = the->firstJump;
			slot = fxGetProperty(the, mxFrameFunction->value.reference, mxID(_prototype));
			mxPushKind(slot->kind);
			mxStack->value = slot->value;
			mxSaveState;
			generator = fxNewGeneratorInstance(the);
			mxRestoreState;
            if (mxFrameTarget->kind != XS_UNDEFINED_KIND)
            	mxFrameThis->kind = XS_UNDEFINED_KIND;
			/* continue */
		mxCase(XS_CODE_YIELD)
			mxSkipCode(1);
			*mxFrameResult = *mxStack;
			generator->next->next->value.reference->next->next->value.boolean = 0; // done
			
			slot = mxFrameArgv(-1);
			mxPushKind(XS_INTEGER_KIND);
			mxStack->value.integer = mxCode - mxFrameFunction->value.reference->next->value.code;;
			mxPushKind(XS_INTEGER_KIND);
			mxStack->value.integer = slot - mxScope;
			mxPushKind(XS_INTEGER_KIND);
			mxStack->value.integer = slot - mxFrame;
			jump = the->firstJump;
			offset = 0;
			while (jump != formerJump) {
				txJump* nextJump = jump->nextJump;
				mxPushKind(XS_INTEGER_KIND);
				mxStack->value.integer = jump->code - mxFrameFunction->value.reference->next->value.code;
				mxPushKind(XS_INTEGER_KIND);
				mxStack->value.integer = slot - jump->scope;
				mxPushKind(XS_INTEGER_KIND);
				mxStack->value.integer = slot - jump->frame;
				mxPushKind(XS_INTEGER_KIND);
				mxStack->value.integer = slot - jump->stack;
				c_free(jump);
				jump = nextJump;
				offset++;
			}
			the->firstJump = formerJump;
			mxPushKind(XS_INTEGER_KIND);
			mxStack->value.integer = offset;
			mxPushKind(mxFrameFunction->kind);
			mxStack->value = mxFrameFunction->value;
			offset = slot - mxStack;
			
			slot = generator->next;
			variable = slot->value.array.address;
			if (slot->value.array.length < offset) {
				mxSaveState;
				if (variable)
					variable = (txSlot *)fxRenewChunk(the, variable, offset * sizeof(txSlot));
				if (!variable)
					variable = (txSlot *)fxNewChunk(the, offset * sizeof(txSlot));
				mxRestoreState;
				slot->value.array.address = variable;
			}
			slot->value.array.length = offset;
			c_memcpy(variable, mxStack, offset * sizeof(txSlot));
			
			generator = formerGenerator;
			formerGenerator = C_NULL;
			goto XS_CODE_END_ALL;
			
	/* FRAMES */		
		mxCase(XS_CODE_ARGUMENT)
			offset = mxRunS1(1);
#ifdef mxTrace
			if (gxDoTrace) fxTraceInteger(the, offset);
#endif
			offset = mxFrameArgc - offset;
			if (0 < offset) {
				slot = mxFrame + 5 + offset;
				mxPushKind(slot->kind);
				mxStack->value = slot->value;
			}
			else {
				mxPushKind(XS_UNDEFINED_KIND);
			}
			mxNextCode(2);
			mxBreak;
		mxCase(XS_CODE_ARGUMENTS)
			offset = mxRunS1(1);
#ifdef mxTrace
			if (gxDoTrace) fxTraceInteger(the, offset);
#endif
			mxOverflow(1);
			*mxStack = mxArrayPrototype;
			mxSaveState;
			fxNewArrayInstance(the);
			mxRestoreState;
			offset = mxFrameArgc - offset;
			if (0 < offset) {
				variable = mxFrame + 5 + offset;
				for (id = 0; id < offset; id++) {
					slot = fxSetProperty(the, mxStack->value.reference, id, C_NULL);
					slot->kind = variable->kind;
					slot->value = variable->value;
					variable--;
				}
			}
			mxNextCode(2);
			mxBreak;
		mxCase(XS_CODE_ARGUMENTS_SLOPPY)
			offset = mxRunS1(1);
			mxOverflow(1);
			*mxStack = mxParametersPrototype;
			mxSaveState;
			fxNewParametersInstance(the, offset);
			mxRestoreState;
			mxNextCode(2);
			mxBreak;
		mxCase(XS_CODE_ARGUMENTS_STRICT)
			offset = mxRunS1(1);
			mxOverflow(1);
			*mxStack = mxArgumentsStrictPrototype;
			mxSaveState;
			fxNewArgumentsStrictInstance(the, offset);
			mxRestoreState;
			mxNextCode(2);
			mxBreak;
		mxCase(XS_CODE_CURRENT)
			mxOverflow(1);
			*mxStack = *mxFrameFunction;
			mxNextCode(1);
			mxBreak;
		mxCase(XS_CODE_RESULT)
			*mxFrameResult = *(mxStack++);
			mxNextCode(1);
			mxBreak;
		mxCase(XS_CODE_TARGET)
			mxOverflow(1);
			*mxStack = *mxFrameTarget;
			mxNextCode(1);
			mxBreak;
		mxCase(XS_CODE_GET_THIS)
			if (mxFrameThis->kind < 0)
				mxRunDebug(XS_REFERENCE_ERROR, "this is not initialized");
			mxOverflow(1);
			*mxStack = *mxFrameThis;
			mxNextCode(1);
			mxBreak;
		mxCase(XS_CODE_SET_THIS)
			*mxFrameThis = *mxStack;
			mxNextCode(1);
			mxBreak;
		
	/* EXCEPTIONS */	
		mxCase(XS_CODE_EXCEPTION)
			mxOverflow(1);
			*mxStack = mxException;
			mxNextCode(1);
			mxBreak;
		mxCase(XS_CODE_CATCH_1)
			offset = mxRunS1(1);
			mxNextCode(2);
			goto XS_CODE_CATCH;
		mxCase(XS_CODE_CATCH_2)
			offset = mxRunS2(1);
			mxNextCode(3);
			goto XS_CODE_CATCH;
		mxCase(XS_CODE_CATCH_4)
			offset = mxRunS4(1);
			mxNextCode(5);
		XS_CODE_CATCH:
			jump = c_malloc(sizeof(txJump));
			if (jump) {
				jump->nextJump = the->firstJump;
				jump->stack = mxStack;
				jump->scope = mxScope;
				jump->frame = mxFrame;
				jump->environment = mxFrameEnvironment->value.reference;
				jump->code = mxCode + offset;
				jump->flag = 1;
                the->firstJump = jump;
				if (c_setjmp(jump->buffer) != 0) {
					jump = the->firstJump;
                    the->firstJump = jump->nextJump;
					mxStack = jump->stack;
					mxScope = jump->scope;
					mxFrame = jump->frame;
					mxFrameEnvironment->value.reference = jump->environment;
					mxCode = jump->code;
					c_free(jump);
					byte = *((txU1*)mxCode);
				}
			}
			else {
				mxSaveState;
				fxJump(the);
			}
			mxBreak;
		mxCase(XS_CODE_RETHROW)
			mxSaveState;
			fxJump(the);
			mxBreak;
		mxCase(XS_CODE_THROW)
			mxException = *mxStack;
		#ifdef mxDebug
			mxSaveState;
			fxDebugThrow(the, C_NULL, 0, "throw");
			mxRestoreState;
		#endif
			mxSaveState;
			fxJump(the);
			mxBreak;
		mxCase(XS_CODE_UNCATCH)
			jump = the->firstJump;
			the->firstJump = jump->nextJump;
			c_free(jump);
			mxNextCode(1);
			mxBreak;
			
	/* BRANCHES */	
		mxCase(XS_CODE_BRANCH_1)
			offset = mxRunS1(1);
			mxNextCode(2 + offset);
			mxBreak;
		mxCase(XS_CODE_BRANCH_2)
			offset = mxRunS2(1);
			mxNextCode(3 + offset);
			mxBreak;
		mxCase(XS_CODE_BRANCH_4)
			offset = mxRunS4(1);
			mxNextCode(5 + offset);
			mxBreak;
		mxCase(XS_CODE_BRANCH_ELSE_1)
			offset = mxRunS1(1);
			index = 2;
			goto XS_CODE_BRANCH_ELSE;
		mxCase(XS_CODE_BRANCH_ELSE_2)
			offset = mxRunS2(1);
			index = 3;
			goto XS_CODE_BRANCH_ELSE;
		mxCase(XS_CODE_BRANCH_ELSE_4)
			offset = mxRunS4(1);
			index = 5;
		XS_CODE_BRANCH_ELSE:
			byte = mxStack->kind;
			if (XS_BOOLEAN_KIND == byte)
				mxNextCode(mxStack->value.boolean ? index : index + offset)
			else if (XS_INTEGER_KIND == byte)
				mxNextCode(mxStack->value.integer ? index : index + offset)
			else if (XS_NUMBER_KIND == byte)
				mxNextCode((c_isnan(mxStack->value.number) || c_iszero(mxStack->value.number)) ? index + offset : index)
			else if ((XS_STRING_KIND == byte) || (XS_STRING_X_KIND == byte))
				mxNextCode(c_strlen(mxStack->value.string) ? index : index + offset)
			else
				mxNextCode((XS_UNDEFINED_KIND == byte) || (XS_NULL_KIND == byte) ? index + offset : index)
			mxStack++;
			mxBreak;
		mxCase(XS_CODE_BRANCH_IF_1)
			offset = mxRunS1(1);
			index = 2;
			goto XS_CODE_BRANCH_IF;
		mxCase(XS_CODE_BRANCH_IF_2)
			offset = mxRunS2(1);
			index = 3;
			goto XS_CODE_BRANCH_IF;
		mxCase(XS_CODE_BRANCH_IF_4)
			offset = mxRunS4(1);
			index = 5;
		XS_CODE_BRANCH_IF:
			byte = mxStack->kind;
			if (XS_BOOLEAN_KIND == byte)
				mxNextCode(mxStack->value.boolean ? index + offset : index)
			else if (XS_INTEGER_KIND == byte)
				mxNextCode(mxStack->value.integer ? index + offset : index)
			else if (XS_NUMBER_KIND == byte)
				mxNextCode((c_isnan(mxStack->value.number) || c_iszero(mxStack->value.number)) ? index : index + offset)
			else if ((XS_STRING_KIND == byte) || (XS_STRING_X_KIND == byte))
				mxNextCode(c_strlen(mxStack->value.string) ? index + offset : index)
			else
				mxNextCode((XS_UNDEFINED_KIND == byte) || (XS_NULL_KIND == byte) ? index : index + offset)
			mxStack++;
			mxBreak;
		mxCase(XS_CODE_BRANCH_STATUS_1)
			offset = mxRunS1(1);
			index = 2;
			goto XS_CODE_BRANCH_STATUS;
		mxCase(XS_CODE_BRANCH_STATUS_2)
			offset = mxRunS2(1);
			index = 3;
			goto XS_CODE_BRANCH_STATUS;
		mxCase(XS_CODE_BRANCH_STATUS_4)
			offset = mxRunS4(1);
			index = 5;
		XS_CODE_BRANCH_STATUS:
			if (the->status & XS_THROW_STATUS) {
				mxException = *mxStack;
			#ifdef mxDebug
				mxSaveState;
				fxDebugThrow(the, C_NULL, 0, "throw");
				mxRestoreState;
			#endif
				mxSaveState;
				fxJump(the);
			}
			mxNextCode((the->status & XS_RETURN_STATUS) ? index : index + offset);
			mxBreak;
			
	/* STACK */	
		mxCase(XS_CODE_DUB)
			mxOverflow(1);
			*mxStack = *(mxStack + 1);
			mxNextCode(1);
			mxBreak;
		mxCase(XS_CODE_POP)
			mxStack++;
			mxNextCode(1);
			mxBreak;
		mxCase(XS_CODE_SWAP)
			scratch = *(mxStack);
			*(mxStack) = *(mxStack + 1);
			*(mxStack + 1) = scratch;
			mxNextCode(1);
			mxBreak;

	/* SCOPE */		
		mxCase(XS_CODE_CONST_CLOSURE_1)
			index = mxRunU1(1);
			mxNextCode(2);
			goto XS_CODE_CONST_CLOSURE;
		mxCase(XS_CODE_CONST_CLOSURE_2)
			index = mxRunU2(1);
			mxNextCode(3);
		XS_CODE_CONST_CLOSURE:
#ifdef mxTrace
			if (gxDoTrace) fxTraceIndex(the, index - 2);
#endif
			variable = (mxFrame - index)->value.closure;
			variable->flag |= XS_DONT_SET_FLAG;
			mxBreak;
		mxCase(XS_CODE_CONST_LOCAL_1)
			index = mxRunU1(1);
			mxNextCode(2);
			goto XS_CODE_CONST_LOCAL;
		mxCase(XS_CODE_CONST_LOCAL_2)
			index = mxRunU2(1);
			mxNextCode(3);
		XS_CODE_CONST_LOCAL:
#ifdef mxTrace
			if (gxDoTrace) fxTraceIndex(the, index - 2);
#endif
			variable = mxFrame - index;
			variable->flag |= XS_DONT_SET_FLAG;
			mxBreak;
		mxCase(XS_CODE_DELETE_CLOSURE_1)
			index = mxRunU1(1);
			mxNextCode(2);
			goto XS_CODE_DELETE_CLOSURE;
		mxCase(XS_CODE_DELETE_CLOSURE_2)
			index = mxRunU2(1);
			mxNextCode(3);
		XS_CODE_DELETE_CLOSURE:
#ifdef mxTrace
			if (gxDoTrace) fxTraceIndex(the, index - 2);
#endif
			variable = (mxFrame - index)->value.closure;
			offset = variable->ID;
		XS_CODE_DELETE_CLOSURE_ALL:
			if (mxFrame->flag & XS_STRICT_FLAG)
				mxRunDebugID(XS_REFERENCE_ERROR, "delete %s: no permission (strict mode)", variable->ID);
			mxPushKind(XS_BOOLEAN_KIND);
			mxStack->value.boolean = 0;
			mxBreak;
		mxCase(XS_CODE_DELETE_LOCAL_1)
			index = mxRunU1(1);
			mxNextCode(2);
			goto XS_CODE_DELETE_LOCAL;
		mxCase(XS_CODE_DELETE_LOCAL_2)
			index = mxRunU2(1);
			mxNextCode(3);
		XS_CODE_DELETE_LOCAL:
#ifdef mxTrace
			if (gxDoTrace) fxTraceIndex(the, index - 2);
#endif
			variable = mxFrame - index;
			if (mxFrame->flag & XS_STRICT_FLAG)
				mxRunDebugID(XS_REFERENCE_ERROR, "delete %s: no permission (strict mode)", variable->ID);
			mxPushKind(XS_BOOLEAN_KIND);
			mxStack->value.boolean = 0;
			mxBreak;
		mxCase(XS_CODE_GET_CLOSURE_1)
			index = mxRunU1(1);
			mxNextCode(2);
			goto XS_CODE_GET_CLOSURE;
		mxCase(XS_CODE_GET_CLOSURE_2)
			index = mxRunU2(1);
			mxNextCode(3);
		XS_CODE_GET_CLOSURE:
#ifdef mxTrace
			if (gxDoTrace) fxTraceIndex(the, index - 2);
#endif
			slot = mxFrame - index;
			variable = slot->value.closure;
			#ifdef mxDebug
				offset = slot->ID;
			#endif
		XS_CODE_GET_CLOSURE_ALL:
			if (variable->kind < 0)
				mxRunDebugID(XS_REFERENCE_ERROR, "get %s: not initialized yet", slot->ID);
			mxPushKind(variable->kind);
			mxStack->value = variable->value;
			mxBreak;
		mxCase(XS_CODE_GET_LOCAL_1)
			index = mxRunU1(1);
			mxNextCode(2);
			goto XS_CODE_GET_LOCAL;
		mxCase(XS_CODE_GET_LOCAL_2)
			index = mxRunU2(1);
			mxNextCode(3);
		XS_CODE_GET_LOCAL:
#ifdef mxTrace
			if (gxDoTrace) fxTraceIndex(the, index - 2);
#endif
			variable = mxFrame - index;
			#ifdef mxDebug
				offset = variable->ID;
			#endif
			mxPushKind(variable->kind);
			mxStack->value = variable->value;
			mxBreak;
		mxCase(XS_CODE_NEW_CLOSURE)
			offset = mxRunS2(1);
#ifdef mxTrace
			if (gxDoTrace) fxTraceID(the, offset);
#endif
			variable = --mxScope;
#ifdef mxTrace
			if (gxDoTrace) fxTraceIndex(the, mxFrame - mxScope - 2);
#endif
			variable->flag = XS_NO_FLAG;
			variable->ID = (txID)offset;
			mxSaveState;
			variable->value.closure = fxNewSlot(the);
			mxRestoreState;
			variable->value.closure->kind = XS_UNINITIALIZED_KIND;
			variable->kind = XS_CLOSURE_KIND;
			mxNextCode(3);
			mxBreak;
		mxCase(XS_CODE_NEW_LOCAL)
			offset = mxRunS2(1);
#ifdef mxTrace
			if (gxDoTrace) fxTraceID(the, offset);
#endif
			mxNextCode(3);
			variable = --mxScope;
#ifdef mxTrace
			if (gxDoTrace) fxTraceIndex(the, mxFrame - mxScope - 2);
#endif
			variable->flag = XS_NO_FLAG;
			variable->ID = (txID)offset;
			mxBreak;
		mxCase(XS_CODE_NEW_TEMPORARY)
			variable = --mxScope;
#ifdef mxTrace
			if (gxDoTrace) fxTraceIndex(the, mxFrame - mxScope - 2);
#endif
			variable->flag = XS_NO_FLAG;
			variable->ID = XS_NO_ID;
			mxNextCode(1);
			mxBreak;
		mxCase(XS_CODE_PULL_CLOSURE_1)
			index = mxRunU1(1);
			mxNextCode(2);
			goto XS_CODE_PULL_CLOSURE;
		mxCase(XS_CODE_PULL_CLOSURE_2)
			index = mxRunU2(1);
			mxNextCode(3);
		XS_CODE_PULL_CLOSURE:
#ifdef mxTrace
			if (gxDoTrace) fxTraceIndex(the, index - 2);
#endif
			variable = (mxFrame - index)->value.closure;
//		XS_CODE_PULL_CLOSURE_ALL:
			if (variable->flag & XS_DONT_SET_FLAG)
				mxRunDebugID(XS_TYPE_ERROR, "set %s: const", variable->ID);
			variable->kind = mxStack->kind;
			variable->value = mxStack->value;
			mxStack++;
			mxBreak;
		mxCase(XS_CODE_PULL_LOCAL_1)
			index = mxRunU1(1);
			mxNextCode(2);
			goto XS_CODE_PULL_LOCAL;
		mxCase(XS_CODE_PULL_LOCAL_2)
			index = mxRunU2(1);
			mxNextCode(3);
		XS_CODE_PULL_LOCAL:
#ifdef mxTrace
			if (gxDoTrace) fxTraceIndex(the, index - 2);
#endif
			variable = mxFrame - index;
			if (variable->flag & XS_DONT_SET_FLAG)
				mxRunDebugID(XS_TYPE_ERROR, "set %s: const", variable->ID);
			variable->kind = mxStack->kind;
			variable->value = mxStack->value;
			mxStack++;
			mxBreak;
		mxCase(XS_CODE_RESERVE_1)
			index = mxRunU1(1);
			mxNextCode(2);
			goto XS_CODE_RESERVE;
		mxCase(XS_CODE_RESERVE_2)
			index = mxRunU2(1);
			mxNextCode(3);
		XS_CODE_RESERVE:	
#ifdef mxTrace
			if (gxDoTrace) fxTraceIndex(the, index);
#endif
			mxOverflow(index);		
			c_memset(mxStack, 0, index * sizeof(txSlot));
			mxBreak;
		mxCase(XS_CODE_SET_CLOSURE_1)
			index = mxRunU1(1);
			mxNextCode(2);
			goto XS_CODE_SET_CLOSURE;
		mxCase(XS_CODE_SET_CLOSURE_2)
			index = mxRunU2(1);
			mxNextCode(3);
		XS_CODE_SET_CLOSURE:
#ifdef mxTrace
			if (gxDoTrace) fxTraceIndex(the, index - 2);
#endif
			variable = (mxFrame - index)->value.closure;
		XS_CODE_SET_CLOSURE_ALL:
			if (variable->flag & XS_DONT_SET_FLAG)
				mxRunDebugID(XS_TYPE_ERROR, "set %s: const", variable->ID);
			variable->kind = mxStack->kind;
			variable->value = mxStack->value;
			mxBreak;
			
		mxCase(XS_CODE_SET_LOCAL_1)
			index = mxRunU1(1);
			mxNextCode(2);
			goto XS_CODE_SET_LOCAL;
		mxCase(XS_CODE_SET_LOCAL_2)
			index = mxRunU2(1);
			mxNextCode(3);
		XS_CODE_SET_LOCAL:
#ifdef mxTrace
			if (gxDoTrace) fxTraceIndex(the, index - 2);
#endif
			variable = mxFrame - index;
			if (variable->flag & XS_DONT_SET_FLAG)
				mxRunDebugID(XS_TYPE_ERROR, "set %s: const", variable->ID);
			variable->kind = mxStack->kind;
			variable->value = mxStack->value;
			mxBreak;
		mxCase(XS_CODE_UNWIND_1)
			index = mxRunU1(1);
			mxNextCode(2);
			goto XS_CODE_UNWIND;
		mxCase(XS_CODE_UNWIND_2)
			index = mxRunU2(1);
			mxNextCode(3);
		XS_CODE_UNWIND:
#ifdef mxTrace
			if (gxDoTrace) fxTraceIndex(the, index);
#endif
			slot = mxScope;
			mxScope += index;
			while (slot < mxScope)
				(slot++)->kind = XS_UNDEFINED_KIND;
			mxBreak;
			
			
		mxCase(XS_CODE_EVAL)
			mxSaveState;
			fxRunEval(the);
			mxRestoreState;
			mxNextCode(1);
			mxBreak;
		mxCase(XS_CODE_WITH)
			variable = mxFrameEnvironment;
			mxSaveState;
			slot = fxNewInstance(the);
			if (variable->kind == XS_REFERENCE_KIND)
				slot->value.instance.prototype = variable->value.reference;
			slot = slot->next = fxNewSlot(the);
			slot->kind = XS_WITH_KIND;
			slot->value.reference = fxToInstance(the, the->stack + 1);
			mxRestoreState;
			variable->kind = XS_REFERENCE_KIND;
			variable->value.reference = mxStack->value.reference;
			mxStack++;
			mxStack->kind = XS_REFERENCE_KIND;
			mxStack->value.reference = variable->value.reference;
			mxNextCode(1);
			mxBreak;
		mxCase(XS_CODE_WITHOUT)
			variable = mxFrameEnvironment;
			slot = variable->value.reference->value.instance.prototype;
			if (slot) {
				variable->kind = XS_REFERENCE_KIND;
				variable->value.reference = slot;
			}
			else
				variable->kind = XS_NULL_KIND;
			mxNextCode(1);
			mxBreak;
		mxCase(XS_CODE_SCOPE)	
			variable = mxFrameEnvironment;
			mxSaveState;
			slot = fxNewInstance(the);
			if (variable->kind == XS_REFERENCE_KIND)
				slot->value.instance.prototype = variable->value.reference;
			slot = slot->next = fxNewSlot(the);
			slot->kind = XS_WITH_KIND;
			slot->value.reference = C_NULL;
			mxRestoreState;
			variable = mxFunctionInstanceClosures((mxStack + 1)->value.reference);
			variable->kind = XS_REFERENCE_KIND;
			variable->value.reference = mxStack->value.reference;
			mxNextCode(1);
			mxBreak;
			
		mxCase(XS_CODE_RETRIEVE_1)
			index = mxRunU1(1);
			mxNextCode(2);
			goto XS_CODE_RETRIEVE;
		mxCase(XS_CODE_RETRIEVE_2)
			index = mxRunU2(1);
			mxNextCode(3);
		XS_CODE_RETRIEVE:	
#ifdef mxTrace
			if (gxDoTrace) fxTraceIndex(the, index);
#endif
			slot = mxFrameEnvironment->value.reference->next->next;
			variable = mxScope;
			while (index) {
				--variable;
				offset = variable->ID = slot->ID;
				variable->kind = slot->kind;
				variable->value = slot->value;
#ifdef mxTrace
				if (gxDoTrace) fxTraceID(the, offset);
#endif
				index--;
				slot = slot->next;
			}
			mxScope = variable;
			mxBreak;
		mxCase(XS_CODE_RETRIEVE_TARGET)
			variable = mxFrameTarget;
			variable->kind = slot->kind;
			variable->value = slot->value;
			slot = slot->next;
			mxNextCode(1);
			mxBreak;
		mxCase(XS_CODE_RETRIEVE_THIS)
			variable = mxFrameThis;
			variable->kind = slot->kind;
			variable->value = slot->value;
			slot = slot->next;
			mxNextCode(1);
			mxBreak;
		mxCase(XS_CODE_STORE_1)
			index = mxRunU1(1);
			mxNextCode(2);
			goto XS_CODE_STORE;
		mxCase(XS_CODE_STORE_2)
			index = mxRunU2(1);
			mxNextCode(3);
		XS_CODE_STORE:	
#ifdef mxTrace
			if (gxDoTrace) fxTraceIndex(the, index - 2);
#endif
			address = &(mxStack->value.reference->next);
			while ((slot = *address)) {
				address = &slot->next;
			}
			mxSaveState;
			*address = slot = fxNewSlot(the);
			mxRestoreState;
			variable = mxFrame - index;
			offset = slot->ID = variable->ID;
			slot->kind = variable->kind;
			slot->value = variable->value;
#ifdef mxTrace
			if (gxDoTrace) fxTraceID(the, offset);
#endif
			mxBreak;
		mxCase(XS_CODE_STORE_TARGET)
			address = &(mxStack->value.reference->next);
			while ((slot = *address)) {
				address = &slot->next;
			}
			mxSaveState;
			*address = slot = fxNewSlot(the);
			mxRestoreState;
			variable = mxFrameTarget;
			slot->ID = mxID(_new_target);
			slot->kind = variable->kind;
			slot->value = variable->value;
			mxNextCode(1);
			mxBreak;
		mxCase(XS_CODE_STORE_THIS)
			address = &(mxStack->value.reference->next);
			while ((slot = *address)) {
				address = &slot->next;
			}
			mxSaveState;
			*address = slot = fxNewSlot(the);
			mxRestoreState;
			variable = mxFrameThis;
			slot->ID = mxID(_this);
			slot->kind = variable->kind;
			slot->value = variable->value;
			mxNextCode(1);
			mxBreak;
			
	/* GLOBALS */	
		mxCase(XS_CODE_CONST_GLOBAL)
			offset = mxRunS2(1);
			slot = mxGlobal.value.reference->next->value.table.address[offset & 0x00007FFF];
			//slot = fxGetGlobalProperty(the, mxGlobal.value.reference, offset);
			slot->flag |= XS_DONT_SET_FLAG;
			mxNextCode(3);
			mxBreak;
		mxCase(XS_CODE_DELETE_GLOBAL)
			offset = mxRunS2(1);
			mxNextCode(3);
		XS_CODE_DELETE_GLOBAL_ALL:
			mxPushKind(XS_BOOLEAN_KIND);
			mxStack->value.boolean = fxRemoveGlobalProperty(the, mxGlobal.value.reference, offset);
			mxBreak;
		mxCase(XS_CODE_GET_GLOBAL)
			offset = mxRunS2(1);
			mxNextCode(3);
		XS_CODE_GET_GLOBAL_ALL:
			mxOverflow(1);
			*mxStack = mxGlobal;
			slot = mxGlobal.value.reference->next->value.table.address[offset & 0x00007FFF];
			//slot = fxGetGlobalProperty(the, mxGlobal.value.reference, offset);
			if (!slot && (byte != XS_CODE_TYPEOF))
				mxRunDebugID(XS_REFERENCE_ERROR, "get %s: undefined variable", offset);
			goto XS_CODE_GET_ALL;
		mxCase(XS_CODE_NEW_GLOBAL)
			offset = mxRunS2(1);
			mxSaveState;
			slot = fxSetGlobalProperty(the, mxGlobal.value.reference, offset, C_NULL);
			mxRestoreState;
			slot->flag |= XS_DONT_DELETE_FLAG;
			mxNextCode(3);
			mxBreak;
		mxCase(XS_CODE_SET_GLOBAL)
			offset = mxRunS2(1);
			mxNextCode(3);
		XS_CODE_SET_GLOBAL_ALL:
			mxOverflow(1);
			*mxStack = mxGlobal;
			slot = mxGlobal.value.reference->next->value.table.address[offset & 0x00007FFF];
			//slot = fxGetGlobalProperty(the, mxGlobal.value.reference, offset);
			if (!slot) {
				if (mxFrame->flag & XS_STRICT_FLAG)
					mxRunDebugID(XS_REFERENCE_ERROR, "set %s: undefined variable (strict mode)", offset);
				mxSaveState;
				slot = fxSetGlobalProperty(the, mxStack->value.reference, offset, C_NULL);
				mxRestoreState;
			}
			goto XS_CODE_SET_ALL;
				
	/* PROPERTIES */		
		mxCase(XS_CODE_AT)
			if ((mxStack->kind == XS_INTEGER_KIND) && fxIntegerToIndex(the->dtoa, mxStack->value.integer, &(scratch.value.integer))) {
				offset = scratch.value.integer;
			}
			else if ((mxStack->kind == XS_NUMBER_KIND) && fxNumberToIndex(the->dtoa, mxStack->value.number, &(scratch.value.integer))) {
				offset = scratch.value.integer;
			}
			else if (mxStack->kind == XS_SYMBOL_KIND) {
				offset = mxStack->value.ID;
			}
			else {
				mxToString(mxStack);
				if (fxStringToIndex(the->dtoa, mxStack->value.string, &(scratch.value.integer))) {
					offset = scratch.value.integer;
				}
				else {
					mxSaveState;
					if (mxStack->kind == XS_STRING_X_KIND)
						slot = fxNewNameX(the, mxStack->value.string);
					else
						slot = fxNewName(the, mxStack);
					mxRestoreState;
					offset = slot->ID;
				}
			}
			mxStack++;
			mxNextCode(1);
			mxBreak;
		
		mxCase(XS_CODE_DELETE_PROPERTY)
		mxCase(XS_CODE_DELETE_SUPER)
			offset = mxRunS2(1);
			mxSkipCode(2);
			/* continue */
		mxCase(XS_CODE_DELETE_PROPERTY_AT)
		mxCase(XS_CODE_DELETE_SUPER_AT)
			mxToInstance(mxStack);
			mxSaveState;
			index = (txU2)fxRemoveProperty(the, mxStack->value.reference, offset);
			mxRestoreState;
			mxNextCode(1);
#ifdef mxTrace
			if (gxDoTrace) fxTraceID(the, offset);
#endif
		XS_CODE_DELETE_ALL:	
			if (!index && (mxFrame->flag & XS_STRICT_FLAG))
				mxRunDebugID(XS_TYPE_ERROR, "delete %s: no permission (strict mode)", offset);
			mxStack->kind = XS_BOOLEAN_KIND;
			mxStack->value.boolean = index;
			mxBreak;
			
		mxCase(XS_CODE_GET_HOME)
			slot = mxFunctionInstanceHome(mxStack->value.reference);
			mxStack->kind = slot->kind;
			mxStack->value = slot->value;
			mxNextCode(1);
			mxBreak;
		mxCase(XS_CODE_GET_SUPER)
			offset = mxRunS2(1);
			mxSkipCode(2);
			/* continue */
		mxCase(XS_CODE_GET_SUPER_AT)
			mxNextCode(1);
			slot = mxFunctionInstanceHome(mxFrameFunction->value.reference);
			if (slot->kind != XS_REFERENCE_KIND)
				mxRunDebugID(XS_TYPE_ERROR, "get super %s: no home", offset);
			slot = fxGetProperty(the, fxGetParent(the, fxGetInstance(the, slot)), offset);
			goto XS_CODE_GET_ALL;
		mxCase(XS_CODE_GET_PROPERTY)
			offset = mxRunS2(1);
			mxSkipCode(2);
			/* continue */
		mxCase(XS_CODE_GET_PROPERTY_AT)
			mxToInstance(mxStack);
			slot = fxGetProperty(the, mxStack->value.reference, offset);
			mxNextCode(1);
		XS_CODE_GET_ALL:	
#ifdef mxTrace
			if (gxDoTrace) fxTraceID(the, offset);
#endif
			if (!slot) {
				mxStack->kind = XS_UNDEFINED_KIND;
				mxBreak;
			}
			if (slot->kind == XS_ACCESSOR_KIND) {
				slot = slot->value.accessor.getter;
				if (mxIsFunction(slot)) {
					mxPushKind(XS_INTEGER_KIND);
					mxStack->value.integer = 0;
					/* SWAP THIS */
					scratch = *(mxStack);
					*(mxStack) = *(mxStack + 1);
					*(mxStack + 1) = scratch;
					/* FUNCTION */
					mxPushKind(XS_REFERENCE_KIND);
					mxStack->value.reference = slot;
					/* TARGET */
					mxPushKind(XS_UNDEFINED_KIND);
					/* RESULT */
					mxPushKind(XS_UNDEFINED_KIND);
					goto XS_CODE_CALL_ALL;
				}
				slot = &mxUndefined;
			}
			mxStack->kind = slot->kind;
			mxStack->value = slot->value;
			mxBreak;
			
		mxCase(XS_CODE_IN)
			if (!mxIsReference(mxStack))
				mxRunDebug(XS_TYPE_ERROR, "in: no reference");
			mxSaveState;
			index = (txU2)fxHasProperty(the, mxStack->value.reference, offset);
			mxRestoreState;
			mxStack->kind = XS_BOOLEAN_KIND;
			mxStack->value.boolean = index;
			mxNextCode(1);
			mxBreak;
			
		mxCase(XS_CODE_NEW_PROPERTY)
			byte = mxRunU1(1);
#ifdef mxTrace
			if (gxDoTrace) fxTraceID(the, offset);
#endif
			mxToInstance(mxStack);
			mxSaveState;
			fxNewProperty(the, fxToInstance(the, mxStack), offset, byte, mxStack + 1);
			mxRestoreState;
			mxStack++;
			mxNextCode(2);
			mxBreak;
		
		mxCase(XS_CODE_SET_HOME)
			slot = mxFunctionInstanceHome(mxStack->value.reference);
			mxStack++;
			slot->kind = mxStack->kind;
			slot->value = mxStack->value;
			mxNextCode(1);
			mxBreak;
		mxCase(XS_CODE_SET_SUPER)
			offset = mxRunS2(1);
			mxSkipCode(2);
			/* continue */
		mxCase(XS_CODE_SET_SUPER_AT)
			slot = mxFunctionInstanceHome(mxFrameFunction->value.reference);
			if (slot->kind != XS_REFERENCE_KIND)
				mxRunDebugID(XS_TYPE_ERROR, "set super %s: no home", offset);
			slot = fxGetProperty(the, fxGetParent(the, fxGetInstance(the, slot)), offset);
			if (!slot || (slot->kind != XS_ACCESSOR_KIND)) {
				mxSaveState;
				slot = fxSetProperty(the, fxToInstance(the, mxStack), offset, C_NULL);
				mxRestoreState;
			}
			mxNextCode(1);
			goto XS_CODE_SET_ALL;
		mxCase(XS_CODE_SET_PROPERTY)
			offset = mxRunS2(1);
			mxSkipCode(2);
			/* continue */
		mxCase(XS_CODE_SET_PROPERTY_AT)
			mxSaveState;
			slot = fxSetProperty(the, fxToInstance(the, mxStack), offset, C_NULL);
			mxRestoreState;
			mxNextCode(1);
		XS_CODE_SET_ALL:	
#ifdef mxTrace
			if (gxDoTrace) fxTraceID(the, offset);
#endif
			if (!slot)
				mxRunDebugID(XS_TYPE_ERROR, "set %s: not extensible", offset);
			if (slot->kind == XS_ACCESSOR_KIND) {
				slot = slot->value.accessor.setter;
				if (!mxIsFunction(slot)) {
					if (mxFrame->flag & XS_STRICT_FLAG)
						mxRunDebugID(XS_TYPE_ERROR, "set %s: no setter", offset);
					mxBreak;
				}
				mxPushKind(XS_INTEGER_KIND);
				mxStack->value.integer = 1;
				/* SWAP THIS */
				scratch = *(mxStack);
				*(mxStack) = *(mxStack + 1);
				*(mxStack + 1) = scratch;
				/* FUNCTION */
				mxPushKind(XS_REFERENCE_KIND);
				mxStack->value.reference = slot;
				/* TARGET */
				mxPushKind(XS_UNDEFINED_KIND);
				/* RESULT */
				mxStack--;
				*(mxStack) = *(mxStack + 5);
				goto XS_CODE_CALL_ALL;
			}
			mxStack++;
			if (slot->flag & XS_DONT_SET_FLAG) {
				if (mxFrame->flag & XS_STRICT_FLAG)
					mxRunDebugID(XS_TYPE_ERROR, "set %s: const", offset);
				mxBreak;
			}
			slot->kind = mxStack->kind;
			slot->value = mxStack->value;
			mxBreak;
			
	/* INSTANCES */	
		mxCase(XS_CODE_ARRAY)
			mxOverflow(1);
			*mxStack = mxArrayPrototype;
			mxSaveState;
			fxNewInstanceOf(the);
			mxRestoreState;
			mxNextCode(1);
			mxBreak;
		mxCase(XS_CODE_CLASS)
			slot = mxStack + 2;
			if (slot->kind != XS_NULL_KIND)
				mxStack->value.reference->value.instance.prototype = slot->value.reference;
			slot = mxStack + 1;
			variable = mxFunctionInstanceHome(mxStack->value.reference);
			variable->kind = slot->kind;
			variable->value = slot->value;

			mxSaveState;
			fxNewProperty(the, fxToInstance(the, mxStack), mxID(_prototype), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG, slot);
			//variable = fxSetProperty(the, mxStack->value.reference, mxID(_prototype), C_NULL);
			//variable->kind = slot->kind;
			//variable->value = slot->value;
			slot = fxSetProperty(the, slot->value.reference, mxID(_constructor), C_NULL);
			mxRestoreState;
			
			slot->flag |= XS_DONT_ENUM_FLAG;
			slot->kind = mxStack->kind;
			slot->value = mxStack->value;
			mxStack += 3;
            mxNextCode(1);
			mxBreak;
		mxCase(XS_CODE_EXTEND)
			if (mxStack->kind == XS_NULL_KIND) {
				mxSaveState;
				fxNewInstance(the);
				mxRestoreState;
			}
			else {
				slot = fxGetInstance(the, mxStack);
            	if (!slot)
					mxRunDebug(XS_TYPE_ERROR, "extends: no function");
				mxSaveState;
				fxCreateInstance(the, slot);
				mxRestoreState;
			}
            mxNextCode(1);
			mxBreak;
		mxCase(XS_CODE_HOST)
			index = mxRunU2(1);
#ifdef mxTrace
			if (gxDoTrace) fxTraceIndex(the, index);
#endif
			mxOverflow(1);
			*mxStack = *mxFrameThis;
			slot = fxGetProperty(the, mxStack->value.reference, mxID(0));
			mxStack->kind = slot->kind;
			mxStack->value = slot->value;
			slot = fxGetProperty(the, mxStack->value.reference, index);
			mxStack->kind = slot->kind;
			mxStack->value = slot->value;
			mxNextCode(3);
			mxBreak;
		mxCase(XS_CODE_INSTANTIATE)
			mxSaveState;
			fxNewInstanceOf(the);
			mxRestoreState;
			mxNextCode(1);
			mxBreak;
		mxCase(XS_CODE_NEW)
			mxSkipCode(1);
			slot = fxGetInstance(the, mxStack);
            if (!mxIsCallable(slot))
				mxRunDebugID(XS_TYPE_ERROR, "new %s: no function", offset);
			/* THIS */
			if (fxIsBaseFunctionInstance(the, slot)) {
				mxSaveState;
				fxCreateInstance(the, slot);
				mxRestoreState;
			}
			else
				mxPushKind(XS_UNINITIALIZED_KIND);
			/* FUNCTION */
			scratch = *(mxStack);
			*(mxStack) = *(mxStack + 1);
			*(mxStack + 1) = scratch;
			/* TARGET */
			mxOverflow(1);
			*mxStack = *(mxStack + 1);
			/* RESULT */
			mxOverflow(1);
			*mxStack = *(mxStack + 3);
			goto XS_CODE_CALL_ALL;
		mxCase(XS_CODE_OBJECT)
			mxOverflow(1);
			*mxStack = mxObjectPrototype;
			mxSaveState;
			fxNewInstanceOf(the);
			mxRestoreState;
			mxNextCode(1);
			mxBreak;
		mxCase(XS_CODE_SUPER)
			mxSkipCode(1);
			if (mxFrameTarget->kind == XS_UNDEFINED_KIND)
				mxRunDebug(XS_REFERENCE_ERROR, "super: no new.target");
			if (mxFrameThis->kind != XS_UNINITIALIZED_KIND)
				mxRunDebug(XS_REFERENCE_ERROR, "super: this is already initialized");
			slot = mxFunctionInstanceHome(mxFrameFunction->value.reference);
			slot = fxGetProperty(the, slot->value.reference, mxID(_constructor));
			slot = slot->value.reference->value.instance.prototype;
            if (!slot || !mxIsFunction(slot))
				mxRunDebug(XS_TYPE_ERROR, "super: no function");
			/* THIS */
			if (fxIsBaseFunctionInstance(the, slot)) {
				mxSaveState;
				fxCreateInstance(the, mxFrameTarget->value.reference);
				mxRestoreState;
			}
			else
				mxPushKind(XS_UNINITIALIZED_KIND);
			/* FUNCTION */
			mxPushKind(XS_REFERENCE_KIND);
			mxStack->value.reference = slot;
			/* TARGET */
			mxPushKind(mxFrameTarget->kind);
			mxStack->value = mxFrameTarget->value;
			/* RESULT */
			mxOverflow(1);
			*mxStack = *(mxStack + 3);
			goto XS_CODE_CALL_ALL;
			
	/* FUNCTIONS */		
		mxCase(XS_CODE_ARROW)
			offset = mxRunS2(1);
#ifdef mxTrace
			if (gxDoTrace) fxTraceID(the, offset);
#endif
			mxOverflow(1);
			*mxStack = mxFunctionPrototype;
			mxSaveState;
			fxNewFunctionInstance(the, (txID)offset);
			mxRestoreState;
			variable = mxFunctionInstanceHome(mxFrameFunction->value.reference);
			slot = mxFunctionInstanceHome(mxStack->value.reference);
			slot->kind = variable->kind;
			slot->value = variable->value;
			mxNextCode(3);
			mxBreak;
		mxCase(XS_CODE_FUNCTION)
			offset = mxRunS2(1);
#ifdef mxTrace
			if (gxDoTrace) fxTraceID(the, offset);
#endif
			mxOverflow(1);
			*mxStack = mxFunctionPrototype;
			mxSaveState;
			fxNewFunctionInstance(the, (txID)offset);
			mxRestoreState;
			mxNextCode(3);
			mxBreak;
		mxCase(XS_CODE_GENERATOR)
			offset = mxRunS2(1);
#ifdef mxTrace
			if (gxDoTrace) fxTraceID(the, offset);
#endif
			mxOverflow(1);
			*mxStack = mxGeneratorFunctionPrototype;
			mxSaveState;
			fxNewGeneratorFunctionInstance(the,(txID) offset);
			mxRestoreState;
			mxNextCode(3);
			mxBreak;
		mxCase(XS_CODE_CODE_1)
			offset = mxRunS1(1);
			mxSkipCode(2);
			goto XS_CODE_CODE;
		mxCase(XS_CODE_CODE_2)
			offset = mxRunS2(1);
			mxSkipCode(3);
			goto XS_CODE_CODE;
		mxCase(XS_CODE_CODE_4)
			offset = mxRunS4(1);
			mxSkipCode(5);
		XS_CODE_CODE:
			mxSaveState;
			scratch.value.code = (txByte*)fxNewChunk(the, offset);
			mxRestoreState;
			c_memcpy(scratch.value.code, mxCode, offset);
			variable = mxStack->value.reference;
			slot = mxFunctionInstanceCode(variable);
			slot->value.code = scratch.value.code;
			slot = mxFunctionInstanceLength(variable);
			slot->value.integer = *(scratch.value.code + 1);
			mxNextCode(offset);
			mxBreak;
		mxCase(XS_CODE_CODE_ARCHIVE_1)
			offset = mxRunS1(1);
			mxSkipCode(2);
			goto XS_CODE_CODE_ARCHIVE;
		mxCase(XS_CODE_CODE_ARCHIVE_2)
			offset = mxRunS2(1);
			mxSkipCode(3);
			goto XS_CODE_CODE_ARCHIVE;
		mxCase(XS_CODE_CODE_ARCHIVE_4)
			offset = mxRunS4(1);
			mxSkipCode(5);
		XS_CODE_CODE_ARCHIVE:
			variable = mxStack->value.reference;
			slot = mxFunctionInstanceCode(variable);
			slot->kind = XS_CODE_X_KIND;
			slot->value.code = mxCode;
			slot = mxFunctionInstanceLength(variable);
			slot->value.integer = *(mxCode + 1);
			mxNextCode(offset);
			mxBreak;

	/* VALUES */		
		mxCase(XS_CODE_UNDEFINED)
			mxPushKind(XS_UNDEFINED_KIND);
			mxNextCode(1);
			mxBreak; 
		mxCase(XS_CODE_NULL)
			mxPushKind(XS_NULL_KIND);
			mxNextCode(1);
			mxBreak; 
		mxCase(XS_CODE_FALSE)
			mxPushKind(XS_BOOLEAN_KIND);
			mxStack->value.boolean = 0;
			mxNextCode(1);
			mxBreak;
		mxCase(XS_CODE_TRUE)
			mxPushKind(XS_BOOLEAN_KIND);
			mxStack->value.boolean = 1;
			mxNextCode(1);
			mxBreak;
		mxCase(XS_CODE_INTEGER_1)
			mxPushKind(XS_INTEGER_KIND);
			mxStack->value.integer = mxRunS1(1);
			mxNextCode(2);
#ifdef mxTrace
			if (gxDoTrace) fxTraceInteger(the, mxStack->value.integer);
#endif
			mxBreak;
		mxCase(XS_CODE_INTEGER_2)
			mxPushKind(XS_INTEGER_KIND);
			mxStack->value.integer = mxRunS2(1);
			mxNextCode(3);
#ifdef mxTrace
			if (gxDoTrace) fxTraceInteger(the, mxStack->value.integer);
#endif
			mxBreak;
		mxCase(XS_CODE_INTEGER_4)
			mxPushKind(XS_INTEGER_KIND);
			mxStack->value.integer = mxRunS4(1);
			mxNextCode(5);
#ifdef mxTrace
			if (gxDoTrace) fxTraceInteger(the, mxStack->value.integer);
#endif
			mxBreak;
		mxCase(XS_CODE_NUMBER)
			mxPushKind(XS_NUMBER_KIND);
			mxCode++;
			mxDecode8(mxCode, mxStack->value.number);
			byte = *((txU1*)mxCode);
#ifdef mxTrace
			if (gxDoTrace) fxTraceNumber(the, mxStack->value.number);
#endif
			mxBreak;
			
		mxCase(XS_CODE_STRING_1)
			index = mxRunU1(1);
			mxSkipCode(2);
			goto XS_CODE_STRING;
		mxCase(XS_CODE_STRING_2)
			index = mxRunU2(1);
			mxSkipCode(3);
		XS_CODE_STRING:
			mxSaveState;
			scratch.value.string = (txString)fxNewChunk(the, index);
			mxRestoreState;
			c_memcpy(scratch.value.string, mxCode, index);
			mxPushKind(XS_STRING_KIND);
			mxStack->value.string = scratch.value.string;
			mxNextCode(index);
#ifdef mxTrace
			if (gxDoTrace) fxTraceString(the, mxStack->value.string);
#endif
			mxBreak;
		mxCase(XS_CODE_STRING_ARCHIVE_1)
			index = mxRunU1(1);
			mxSkipCode(2);
			goto XS_CODE_STRING_ARCHIVE;
		mxCase(XS_CODE_STRING_ARCHIVE_2)
			index = mxRunU2(1);
			mxSkipCode(3);
		XS_CODE_STRING_ARCHIVE:
			mxPushKind(XS_STRING_X_KIND);
			mxStack->value.string = (txString)mxCode;
			mxNextCode(index);
#ifdef mxTrace
			if (gxDoTrace) fxTraceString(the, mxStack->value.string);
#endif
			mxBreak;
			
		mxCase(XS_CODE_SYMBOL)
			mxPushKind(XS_SYMBOL_KIND);
			mxStack->value.ID = mxRunS2(1);
#ifdef mxTrace
			if (gxDoTrace) fxTraceID(the, mxStack->value.ID);
#endif
			mxNextCode(3);
			mxBreak;
			

	/* EXPRESSIONS */ 
		mxCase(XS_CODE_BIT_NOT)
			mxToInteger(mxStack);
			mxStack->value.integer = ~mxStack->value.integer;
			mxNextCode(1);
			mxBreak;
		mxCase(XS_CODE_BIT_AND)
			slot = mxStack + 1;
			mxToInteger(slot);
			mxToInteger(mxStack);
			slot->value.integer &= mxStack->value.integer;
			mxStack++;
			mxNextCode(1);
			mxBreak;
		mxCase(XS_CODE_BIT_OR)
			slot = mxStack + 1;
			mxToInteger(slot);
			mxToInteger(mxStack);
			slot->value.integer |= mxStack->value.integer;
			mxStack++;
			mxNextCode(1);
			mxBreak;
		mxCase(XS_CODE_BIT_XOR)
			slot = mxStack + 1;
			mxToInteger(slot);
			mxToInteger(mxStack);
			slot->value.integer ^= mxStack->value.integer;
			mxStack++;
			mxNextCode(1);
			mxBreak;
			
		mxCase(XS_CODE_LEFT_SHIFT)
			slot = mxStack + 1;
			mxToInteger(slot);
			mxToInteger(mxStack);
			slot->value.integer <<= mxStack->value.integer & 0x1f;
			mxStack++;
			mxNextCode(1);
			mxBreak;
		mxCase(XS_CODE_SIGNED_RIGHT_SHIFT)
			slot = mxStack + 1;
			mxToInteger(slot);
			mxToInteger(mxStack);
			slot->value.integer >>= mxStack->value.integer & 0x1f;
			mxStack++;
			mxNextCode(1);
			mxBreak;
		mxCase(XS_CODE_UNSIGNED_RIGHT_SHIFT)
			slot = mxStack + 1;
			mxSaveState;
			fxUnsigned(the, slot, fxToUnsigned(the, slot) >> (fxToUnsigned(the, mxStack) & 0x1F));
			mxRestoreState;
			mxStack++;
			mxNextCode(1);
			mxBreak;
				
		mxCase(XS_CODE_MINUS)
			if ((mxStack->kind == XS_INTEGER_KIND) && mxStack->value.integer)
				mxStack->value.integer = -mxStack->value.integer;
			else {
				mxToNumber(mxStack);
				mxStack->value.number = -mxStack->value.number;
			}
			mxNextCode(1);
			mxBreak;
		mxCase(XS_CODE_PLUS)
			if (mxStack->kind != XS_INTEGER_KIND) {
				mxToNumber(mxStack);
			}
			mxNextCode(1);
			mxBreak;
			
		mxCase(XS_CODE_DECREMENT)
			if (mxStack->kind == XS_INTEGER_KIND) {
				mxStack->value.integer--;
			}
			else {
				mxToNumber(mxStack);
				mxStack->value.number--;
			}
			mxNextCode(1);
			mxBreak;
		mxCase(XS_CODE_INCREMENT)
			if (mxStack->kind == XS_INTEGER_KIND)
				mxStack->value.integer++;
			else {
				mxToNumber(mxStack);
				mxStack->value.number++;
			}
			mxNextCode(1);
			mxBreak;
			
		mxCase(XS_CODE_ADD)
			slot = mxStack + 1;
			if ((slot->kind == XS_INTEGER_KIND) && (mxStack->kind == XS_INTEGER_KIND)) {
				txInteger a = slot->value.integer;
				txInteger b = mxStack->value.integer;
				txInteger c = a + b;
				if (((a ^ c) & (b ^ c)) < 0) {
					fxToNumber(the, slot);
					fxToNumber(the, mxStack);
					goto XS_CODE_ADD_OVERFLOW;
				}
				slot->value.integer = c;
			} 			
			else if ((slot->kind == XS_NUMBER_KIND) && (mxStack->kind == XS_NUMBER_KIND)) {
		XS_CODE_ADD_OVERFLOW:
				slot->value.number += mxStack->value.number;
			} 			
			else if ((slot->kind == XS_INTEGER_KIND) && (mxStack->kind == XS_NUMBER_KIND)) {
				mxToNumber(slot);
				slot->value.number += mxStack->value.number;
			} 			
			else if ((slot->kind == XS_NUMBER_KIND) && (mxStack->kind == XS_INTEGER_KIND)) {
				mxToNumber(mxStack);
				slot->value.number += mxStack->value.number;
			} 			
			else {
				mxSaveState;
				fxToPrimitive(the, slot, XS_NO_HINT);
				fxToPrimitive(the, mxStack, XS_NO_HINT);
				if ((slot->kind == XS_STRING_KIND) || (slot->kind == XS_STRING_X_KIND) || (mxStack->kind == XS_STRING_KIND) || (mxStack->kind == XS_STRING_X_KIND)) {
					fxToString(the, slot);
					fxToString(the, mxStack);
					fxConcatString(the, slot, mxStack);
				}
				else {
					mxToNumber(slot);
					mxToNumber(mxStack);
					slot->value.number += mxStack->value.number;
				}
				mxRestoreState;
			}
			mxStack++;
			mxNextCode(1);
			mxBreak;
		mxCase(XS_CODE_SUBTRACT)
			slot = mxStack + 1;
			if ((slot->kind == XS_INTEGER_KIND) && (mxStack->kind == XS_INTEGER_KIND)) {
				slot->value.integer -= mxStack->value.integer;
			} 
			else {
				mxToNumber(slot);
				mxToNumber(mxStack);
				slot->value.number -= mxStack->value.number;
			}
			mxStack++;
			mxNextCode(1);
			mxBreak;
			
		mxCase(XS_CODE_DIVIDE)
			slot = mxStack + 1;
			mxToNumber(slot);
			mxToNumber(mxStack);
			slot->value.number /= mxStack->value.number;
			mxStack++;
			mxNextCode(1);
			mxBreak;
		mxCase(XS_CODE_MULTIPLY)
			slot = mxStack + 1;
			mxToNumber(slot);
			mxToNumber(mxStack);
			slot->value.number *= mxStack->value.number;
			mxStack++;
			mxNextCode(1);
			mxBreak;
		mxCase(XS_CODE_MODULO)
			slot = mxStack + 1;
			if ((slot->kind == XS_INTEGER_KIND) && (mxStack->kind == XS_INTEGER_KIND) && (mxStack->value.integer != 0)) {
				slot->value.integer %= mxStack->value.integer;
			} 
			else {
				mxToNumber(slot);
				mxToNumber(mxStack);
				slot->value.number = c_fmod(slot->value.number, mxStack->value.number);
			}
			mxStack++;
			mxNextCode(1);
			mxBreak;
			
		mxCase(XS_CODE_NOT)
			mxToBoolean(mxStack);
			mxStack->value.boolean = !mxStack->value.boolean;
			mxNextCode(1);
			mxBreak;
		mxCase(XS_CODE_LESS)
			slot = mxStack + 1;
			if ((slot->kind == XS_INTEGER_KIND) && (mxStack->kind == XS_INTEGER_KIND))
				offset = slot->value.integer < mxStack->value.integer;
			else if ((slot->kind == XS_NUMBER_KIND) || (mxStack->kind == XS_NUMBER_KIND))
				goto XS_CODE_LESS_ALL;
			else {
				mxSaveState; 
				fxToPrimitive(the, slot, XS_NUMBER_HINT); 
				fxToPrimitive(the, mxStack, XS_NUMBER_HINT); 
				mxRestoreState;
				if (((slot->kind == XS_STRING_KIND) || (slot->kind == XS_STRING_X_KIND)) && ((mxStack->kind == XS_STRING_KIND) || (mxStack->kind == XS_STRING_X_KIND)))
					offset = c_strcmp(slot->value.string, mxStack->value.string) < 0;
				else {
			XS_CODE_LESS_ALL:
					mxToNumber(slot);
					mxToNumber(mxStack);
					offset = (!c_isnan(slot->value.number)) && (!c_isnan(mxStack->value.number)) && (slot->value.number < mxStack->value.number);
				}
			}
			slot->kind = XS_BOOLEAN_KIND;
			slot->value.boolean = offset;
			mxStack++;
			mxNextCode(1);
			mxBreak;
		mxCase(XS_CODE_LESS_EQUAL)
			slot = mxStack + 1;
			if ((slot->kind == XS_INTEGER_KIND) && (mxStack->kind == XS_INTEGER_KIND))
				offset = slot->value.integer <= mxStack->value.integer;
			else if ((slot->kind == XS_NUMBER_KIND) || (mxStack->kind == XS_NUMBER_KIND))
				goto XS_CODE_LESS_EQUAL_ALL;
			else {
				mxSaveState; 
				fxToPrimitive(the, slot, XS_NUMBER_HINT); 
				fxToPrimitive(the, mxStack, XS_NUMBER_HINT); 
				mxRestoreState;
				if (((slot->kind == XS_STRING_KIND) || (slot->kind == XS_STRING_X_KIND)) && ((mxStack->kind == XS_STRING_KIND) || (mxStack->kind == XS_STRING_X_KIND)))
					offset = c_strcmp(slot->value.string, mxStack->value.string) <= 0;
				else {
			XS_CODE_LESS_EQUAL_ALL:
					mxToNumber(slot);
					mxToNumber(mxStack);
					offset = (!c_isnan(slot->value.number)) && (!c_isnan(mxStack->value.number)) && (slot->value.number <= mxStack->value.number);
				}
			}
			slot->kind = XS_BOOLEAN_KIND;
			slot->value.boolean = offset;
			mxStack++;
			mxNextCode(1);
			mxBreak;
		mxCase(XS_CODE_MORE)
			slot = mxStack + 1;
			if ((slot->kind == XS_INTEGER_KIND) && (mxStack->kind == XS_INTEGER_KIND))
				offset = slot->value.integer > mxStack->value.integer;
			else if ((slot->kind == XS_NUMBER_KIND) || (mxStack->kind == XS_NUMBER_KIND))
				goto XS_CODE_MORE_ALL;
			else {
				mxSaveState; 
				fxToPrimitive(the, slot, XS_NUMBER_HINT); 
				fxToPrimitive(the, mxStack, XS_NUMBER_HINT); 
				mxRestoreState;
				if (((slot->kind == XS_STRING_KIND) || (slot->kind == XS_STRING_X_KIND)) && ((mxStack->kind == XS_STRING_KIND) || (mxStack->kind == XS_STRING_X_KIND)))
					offset = c_strcmp(slot->value.string, mxStack->value.string) > 0;
				else {
			XS_CODE_MORE_ALL:
					mxToNumber(slot);
					mxToNumber(mxStack);
					offset = (!c_isnan(slot->value.number)) && (!c_isnan(mxStack->value.number)) && (slot->value.number > mxStack->value.number);
				}
			}
			slot->kind = XS_BOOLEAN_KIND;
			slot->value.boolean = offset;
			mxStack++;
			mxNextCode(1);
			mxBreak;
		mxCase(XS_CODE_MORE_EQUAL)
			slot = mxStack + 1;
			if ((slot->kind == XS_INTEGER_KIND) && (mxStack->kind == XS_INTEGER_KIND))
				offset = slot->value.integer >= mxStack->value.integer;
			else if ((slot->kind == XS_NUMBER_KIND) || (mxStack->kind == XS_NUMBER_KIND))
				goto XS_CODE_MORE_EQUAL_ALL;
			else {
				mxSaveState; 
				fxToPrimitive(the, slot, XS_NUMBER_HINT); 
				fxToPrimitive(the, mxStack, XS_NUMBER_HINT); 
				mxRestoreState;
				if (((slot->kind == XS_STRING_KIND) || (slot->kind == XS_STRING_X_KIND)) && ((mxStack->kind == XS_STRING_KIND) || (mxStack->kind == XS_STRING_X_KIND)))
					offset = c_strcmp(slot->value.string, mxStack->value.string) >= 0;
				else {
			XS_CODE_MORE_EQUAL_ALL:
					mxToNumber(slot);
					mxToNumber(mxStack);
					offset = (!c_isnan(slot->value.number)) && (!c_isnan(mxStack->value.number)) && (slot->value.number >= mxStack->value.number);
				}
			}
			slot->kind = XS_BOOLEAN_KIND;
			slot->value.boolean = offset;
			mxStack++;
			mxNextCode(1);
			mxBreak;
			
		mxCase(XS_CODE_EQUAL)
		mxCase(XS_CODE_STRICT_EQUAL)
			slot = mxStack + 1;
		XS_CODE_EQUAL_AGAIN:
			if (slot->kind == mxStack->kind) {
				if ((XS_UNDEFINED_KIND == slot->kind) || (XS_NULL_KIND == slot->kind))
					offset = 1;
				else if (XS_BOOLEAN_KIND == slot->kind)
					offset = slot->value.boolean == stack->value.boolean;
				else if (XS_INTEGER_KIND == slot->kind)
					offset = slot->value.integer == stack->value.integer;
				else if (XS_NUMBER_KIND == slot->kind)
					offset = (!c_isnan(slot->value.number)) && (!c_isnan(mxStack->value.number)) && (slot->value.number == mxStack->value.number);
				else if ((XS_STRING_KIND == slot->kind) || (XS_STRING_X_KIND == slot->kind))
					offset = c_strcmp(slot->value.string, mxStack->value.string) == 0;
				else if (XS_SYMBOL_KIND == slot->kind)
					offset = slot->value.ID == mxStack->value.ID;
				else if (XS_REFERENCE_KIND == slot->kind)
					offset = slot->value.reference == mxStack->value.reference;
                else
                    offset = 0;
			}
			else if ((XS_INTEGER_KIND == slot->kind) && (XS_NUMBER_KIND == mxStack->kind))
				offset = (!c_isnan(mxStack->value.number)) && ((txNumber)(slot->value.integer) == stack->value.number);
			else if ((XS_NUMBER_KIND == slot->kind) && (XS_INTEGER_KIND == mxStack->kind))
				offset = (!c_isnan(slot->value.number)) && (slot->value.number == (txNumber)(mxStack->value.integer));
			else if ((XS_STRING_KIND == slot->kind) && (XS_STRING_X_KIND == mxStack->kind))
				offset = c_strcmp(slot->value.string, mxStack->value.string) == 0;
			else if ((XS_STRING_X_KIND == slot->kind) && (XS_STRING_KIND == mxStack->kind))
				offset = c_strcmp(slot->value.string, mxStack->value.string) == 0;
			else if (XS_CODE_EQUAL == byte) {
				if ((slot->kind == XS_UNDEFINED_KIND) && (mxStack->kind == XS_NULL_KIND))
					offset = 1;
				else if ((slot->kind == XS_NULL_KIND) && (mxStack->kind == XS_UNDEFINED_KIND))
					offset = 1;
				else if (((slot->kind == XS_STRING_KIND) || (slot->kind == XS_STRING_X_KIND)) && ((XS_INTEGER_KIND == mxStack->kind) || (XS_NUMBER_KIND == mxStack->kind))) {
					fxToNumber(the, slot);
					goto XS_CODE_EQUAL_AGAIN;
				}
				else if (((mxStack->kind == XS_STRING_KIND) || (mxStack->kind == XS_STRING_X_KIND)) && ((XS_INTEGER_KIND == slot->kind) || (XS_NUMBER_KIND == slot->kind))) {
					fxToNumber(the, mxStack); 
					goto XS_CODE_EQUAL_AGAIN;
				}
				else if (XS_BOOLEAN_KIND == slot->kind) {
					fxToNumber(the, slot);
					goto XS_CODE_EQUAL_AGAIN;
				}
				else if (XS_BOOLEAN_KIND == mxStack->kind) {
					fxToNumber(the, mxStack);
					goto XS_CODE_EQUAL_AGAIN;
				}
				else if (((slot->kind == XS_INTEGER_KIND) || (slot->kind == XS_NUMBER_KIND) || (slot->kind == XS_STRING_KIND) || (slot->kind == XS_STRING_X_KIND) || (slot->kind == XS_SYMBOL_KIND)) && mxIsReference(mxStack)) {
					mxSaveState;
					fxToPrimitive(the, mxStack, XS_NO_HINT);
					mxRestoreState;
					goto XS_CODE_EQUAL_AGAIN;
				}
				else if (((mxStack->kind == XS_INTEGER_KIND) || (mxStack->kind == XS_NUMBER_KIND) || (mxStack->kind == XS_STRING_KIND) || (mxStack->kind == XS_STRING_X_KIND) || (mxStack->kind == XS_SYMBOL_KIND)) && mxIsReference(slot)) {
					mxSaveState;
					fxToPrimitive(the, slot, XS_NO_HINT);
					mxRestoreState;
					goto XS_CODE_EQUAL_AGAIN;
				}
                else
                    offset = 0;
			}
			else 
				offset = 0;
			slot->kind = XS_BOOLEAN_KIND;
			slot->value.boolean = offset;
			mxStack++;
			mxNextCode(1);
			mxBreak;
		mxCase(XS_CODE_NOT_EQUAL)
		mxCase(XS_CODE_STRICT_NOT_EQUAL)
			slot = mxStack + 1;
		XS_CODE_NOT_EQUAL_AGAIN:
			if (slot->kind == mxStack->kind) {
				if ((XS_UNDEFINED_KIND == slot->kind) || (XS_NULL_KIND == slot->kind))
					offset = 0;
				else if (XS_BOOLEAN_KIND == slot->kind)
					offset = slot->value.boolean != stack->value.boolean;
				else if (XS_INTEGER_KIND == slot->kind)
					offset = slot->value.integer != stack->value.integer;
				else if (XS_NUMBER_KIND == slot->kind)
					offset = (c_isnan(slot->value.number)) || (c_isnan(mxStack->value.number)) || (slot->value.number != mxStack->value.number);
				else if ((XS_STRING_KIND == slot->kind) || (XS_STRING_X_KIND == slot->kind))
					offset = c_strcmp(slot->value.string, mxStack->value.string) != 0;
				else if (XS_SYMBOL_KIND == slot->kind)
					offset = slot->value.ID != mxStack->value.ID;
				else if (XS_REFERENCE_KIND == slot->kind)
					offset = slot->value.reference != mxStack->value.reference;
                else
                    offset = 1;
			}
			else if ((XS_INTEGER_KIND == slot->kind) && (XS_NUMBER_KIND == mxStack->kind))
				offset = (c_isnan(mxStack->value.number)) || ((txNumber)(slot->value.integer) != stack->value.number);
			else if ((XS_NUMBER_KIND == slot->kind) && (XS_INTEGER_KIND == mxStack->kind))
				offset = (c_isnan(slot->value.number)) || (slot->value.number != (txNumber)(mxStack->value.integer));
			else if ((XS_STRING_KIND == slot->kind) && (XS_STRING_X_KIND == mxStack->kind))
				offset = c_strcmp(slot->value.string, mxStack->value.string) != 0;
			else if ((XS_STRING_X_KIND == slot->kind) && (XS_STRING_KIND == mxStack->kind))
				offset = c_strcmp(slot->value.string, mxStack->value.string) != 0;
			else if (XS_CODE_NOT_EQUAL == byte) {
				if ((slot->kind == XS_UNDEFINED_KIND) && (mxStack->kind == XS_NULL_KIND))
					offset = 0;
				else if ((slot->kind == XS_NULL_KIND) && (mxStack->kind == XS_UNDEFINED_KIND))
					offset = 0;
				else if (((slot->kind == XS_STRING_KIND) || (slot->kind == XS_STRING_X_KIND)) && ((XS_INTEGER_KIND == mxStack->kind) || (XS_NUMBER_KIND == mxStack->kind))) {
					fxToNumber(the, slot);
					goto XS_CODE_NOT_EQUAL_AGAIN;
				}
				else if (((mxStack->kind == XS_STRING_KIND) || (mxStack->kind == XS_STRING_X_KIND)) && ((XS_INTEGER_KIND == slot->kind) || (XS_NUMBER_KIND == slot->kind))) {
					fxToNumber(the, mxStack); 
					goto XS_CODE_NOT_EQUAL_AGAIN;
				}
				else if (XS_BOOLEAN_KIND == slot->kind) {
					fxToNumber(the, slot);
					goto XS_CODE_NOT_EQUAL_AGAIN;
				}
				else if (XS_BOOLEAN_KIND == mxStack->kind) {
					fxToNumber(the, mxStack);
					goto XS_CODE_NOT_EQUAL_AGAIN;
				}
				else if (((slot->kind == XS_INTEGER_KIND) || (slot->kind == XS_NUMBER_KIND) || (slot->kind == XS_STRING_KIND) || (slot->kind == XS_STRING_X_KIND) || (slot->kind == XS_SYMBOL_KIND)) && mxIsReference(mxStack)) {
					mxSaveState;
					fxToPrimitive(the, mxStack, XS_NO_HINT);
					mxRestoreState;
					goto XS_CODE_NOT_EQUAL_AGAIN;
				}
				else if (((mxStack->kind == XS_INTEGER_KIND) || (mxStack->kind == XS_NUMBER_KIND) || (mxStack->kind == XS_STRING_KIND) || (mxStack->kind == XS_STRING_X_KIND) || (mxStack->kind == XS_SYMBOL_KIND)) && mxIsReference(slot)) {
					mxSaveState;
					fxToPrimitive(the, slot, XS_NO_HINT);
					mxRestoreState;
					goto XS_CODE_NOT_EQUAL_AGAIN;
				}
                else
                    offset = 1;
			}
			else 
				offset = 1;
			slot->kind = XS_BOOLEAN_KIND;
			slot->value.boolean = offset;
			mxStack++;
			mxNextCode(1);
			mxBreak;

		mxCase(XS_CODE_FOR_OF)
			mxSkipCode(1);
			offset = mxID(_Symbol_iterator);
			mxToInstance(mxStack);
			slot = fxGetProperty(the, mxStack->value.reference, offset);
			if (!slot)
				mxRunDebugID(XS_TYPE_ERROR, "get %s: no property", offset);
			mxPushKind(XS_INTEGER_KIND);
			mxStack->value.integer = 0;
			/* SWAP THIS */
			scratch = *(mxStack);
			*(mxStack) = *(mxStack + 1);
			*(mxStack + 1) = scratch;
			/* FUNCTION */
			mxPushKind(slot->kind);
			mxStack->value = slot->value;
			slot = fxGetInstance(the, mxStack);
			if (!slot || !mxIsFunction(slot))
				mxRunDebugID(XS_TYPE_ERROR, "call %s: no function", offset);
			/* TARGET */
			mxPushKind(XS_UNDEFINED_KIND);
			/* RESULT */
			mxPushKind(XS_UNDEFINED_KIND);
			goto XS_CODE_CALL_ALL;
		mxCase(XS_CODE_FOR_IN)
			mxSkipCode(1);
			mxPushKind(XS_INTEGER_KIND);
			mxStack->value.integer = 0;
			/* SWAP THIS */
			scratch = *(mxStack);
			*(mxStack) = *(mxStack + 1);
			*(mxStack + 1) = scratch;
			/* FUNCTION */
			mxOverflow(1);
			*mxStack = mxEnumeratorFunction;
			slot = fxGetInstance(the, mxStack);
			/* TARGET */
			mxPushKind(XS_UNDEFINED_KIND);
			/* RESULT */
			mxPushKind(XS_UNDEFINED_KIND);
			goto XS_CODE_CALL_ALL;
		mxCase(XS_CODE_INSTANCEOF)
			mxNextCode(1);
			offset = mxID(_Symbol_hasInstance);
			mxBreak;
		mxCase(XS_CODE_TYPEOF)
			byte = mxStack->kind;
			if (XS_UNDEFINED_KIND == byte)
				*mxStack = mxUndefinedString;
			else if (XS_NULL_KIND == byte)
				*mxStack = mxObjectString;
			else if (XS_BOOLEAN_KIND == byte)
				*mxStack = mxBooleanString;
			else if ((XS_INTEGER_KIND == byte) || (XS_NUMBER_KIND == byte))
				*mxStack = mxNumberString;
			else if ((XS_STRING_KIND == byte) || (XS_STRING_X_KIND == byte))
				*mxStack = mxStringString;
			else if (XS_SYMBOL_KIND == byte)
				*mxStack = mxSymbolString;
			else if (XS_REFERENCE_KIND == byte) {
				slot = fxGetInstance(the, mxStack);
				if (mxIsFunction(slot))
					*mxStack = mxFunctionString;
				else
					*mxStack = mxObjectString;
			}
			mxNextCode(1);
			mxBreak;
		mxCase(XS_CODE_VOID)
			mxNextCode(1);
			mxStack->kind = XS_UNDEFINED_KIND;
			mxBreak;

	/* DEBUG */		
		mxCase(XS_CODE_DEBUGGER)
			mxNextCode(1);
		#ifdef mxDebug
			mxSaveState;
			fxDebugLoop(the, C_NULL, 0, "debugger");
			mxRestoreState;
		#endif
			mxBreak;
		mxCase(XS_CODE_FILE)
		#ifdef mxDebug
			id = mxRunS2(1);
#ifdef mxTrace
			if (gxDoTrace) fxTraceID(the, id);
#endif
			slot = fxGetKey(the, id);
			if (!(slot->flag & XS_DEBUG_FLAG)) {
				mxSaveState;
				fxDebugFile(the, slot);
				mxRestoreState;
			}
			mxFrameEnvironment->next = slot;
		#endif
			mxNextCode(3);
			mxBreak;
		mxCase(XS_CODE_LINE)
		#ifdef mxDebug
			id = mxRunS2(1);
#ifdef mxTrace
			if (gxDoTrace) fxTraceInteger(the, id);
#endif
			mxFrameEnvironment->ID = id;
			if (fxIsReadable(the))
				fxDebugCommand(the);
			if ((mxFrame->flag & XS_STEP_OVER_FLAG) || mxBreakpoints.value.list.first) {
				mxSaveState;
				fxDebugLine(the);
				mxRestoreState;
			}
		#endif
			mxNextCode(3);
			mxBreak;

	/* MODULE */		
		mxCase(XS_CODE_TRANSFER)
			mxSkipCode(1);
			slot = mxTransferConstructor.value.reference;
			/* THIS */
			mxSaveState;
			fxCreateInstance(the, slot);
			mxRestoreState;
			/* FUNCTION */
			mxOverflow(1);
			*mxStack = mxTransferConstructor;
			/* TARGET */
			mxOverflow(1);
			*mxStack = mxTransferConstructor;
			/* RESULT */
			mxPushKind(XS_UNDEFINED_KIND);
			goto XS_CODE_CALL_ALL;
			mxBreak;
		mxCase(XS_CODE_MODULE)
			mxSkipCode(1);
			slot = mxModuleConstructor.value.reference;
			/* THIS */
			variable = mxFunctionInstanceModule(mxFrameFunction->value.reference);
			mxPushKind(variable->kind);
			mxStack->value = variable->value;
			/* FUNCTION */
			mxOverflow(1);
			*mxStack = mxModuleConstructor;
			/* TARGET */
			mxOverflow(1);
			*mxStack = mxModuleConstructor;
			/* RESULT */
			mxPushKind(XS_UNDEFINED_KIND);
			goto XS_CODE_CALL_ALL;
			mxBreak;

	/* EVAL & WITH */		
		mxCase(XS_CODE_DELETE_EVAL)
			offset = mxRunS2(1);
#ifdef mxTrace
			if (gxDoTrace) fxTraceID(the, offset);
#endif
			mxNextCode(3);
			variable = mxFrameEnvironment;
			if (variable->kind == XS_REFERENCE_KIND) {
				variable = variable->value.reference;
		XS_CODE_DELETE_EVAL_AGAIN:
				slot = variable->next->value.reference;
				if (slot) {
					mxPushKind(XS_REFERENCE_KIND);
					mxStack->value.reference = slot;
					mxSaveState;
					index = fxIsScopableSlot(the, slot, offset);
					mxRestoreState;
					if (index) {
						slot = fxGetProperty(the, slot, offset);
						if (slot) {
							index = (txU2)fxRemoveProperty(the, mxStack->value.reference, offset);
							goto XS_CODE_DELETE_ALL;
						}
					}
					mxStack++;
				}
				slot = fxGetOwnProperty(the, variable, offset);
				if (slot) {
					variable = slot->value.closure;
					goto XS_CODE_DELETE_CLOSURE_ALL;
				}
				variable = variable->value.instance.prototype;
				if (variable)
					goto XS_CODE_DELETE_EVAL_AGAIN;
			}
			goto XS_CODE_DELETE_GLOBAL_ALL;
			mxBreak;
		mxCase(XS_CODE_GET_EVAL)
			offset = mxRunS2(1);
			mxNextCode(3);
			variable = mxFrameEnvironment;
			if (variable->kind == XS_REFERENCE_KIND) {
				variable = variable->value.reference;
		XS_CODE_GET_EVAL_AGAIN:
				slot = variable->next->value.reference;
				if (slot) {
					mxPushKind(XS_REFERENCE_KIND);
					mxStack->value.reference = slot;
					mxSaveState;
					index = fxIsScopableSlot(the, slot, offset);
					mxRestoreState;
					if (index) {
						slot = fxGetProperty(the, slot, offset);
						if (slot) {
							if ((XS_CODE_CALL == byte) || (XS_CODE_CALL_TAIL == byte)) {
								(mxStack + 1)->kind = XS_REFERENCE_KIND;
								(mxStack + 1)->value.reference = mxStack->value.reference;
							}
							goto XS_CODE_GET_ALL;
						}
					}
					mxStack++;
				}
				slot = fxGetOwnProperty(the, variable, offset);
				if (slot) {
					variable = slot->value.closure;
				#ifdef mxTrace
					if (gxDoTrace) fxTraceID(the, offset);
				#endif
					goto XS_CODE_GET_CLOSURE_ALL;
				}
				variable = variable->value.instance.prototype;
				if (variable)
					goto XS_CODE_GET_EVAL_AGAIN;
			}
			goto XS_CODE_GET_GLOBAL_ALL;
		mxCase(XS_CODE_NEW_EVAL)
			offset = mxRunS2(1);
#ifdef mxTrace
			if (gxDoTrace) fxTraceID(the, offset);
#endif
			mxNextCode(3);
			variable = mxFrameEnvironment;
			if (variable->kind == XS_REFERENCE_KIND) {
                variable = variable->value.reference;
				slot = variable->next->value.reference;
				if (slot) {
					slot = fxGetOwnProperty(the, slot, offset);
					if (slot)
						mxBreak;
				}
				slot = fxGetOwnProperty(the, variable, offset);
				if (slot)
					mxBreak;
				mxSaveState;
				fxSetProperty(the, variable->next->value.reference, offset, C_NULL);
				mxRestoreState;
			}
			mxBreak;
		mxCase(XS_CODE_SET_EVAL)
			offset = mxRunS2(1);
#ifdef mxTrace
			if (gxDoTrace) fxTraceID(the, offset);
#endif
			mxNextCode(3);
			variable = mxFrameEnvironment;
			if (variable->kind == XS_REFERENCE_KIND) {
				variable = variable->value.reference;
		XS_CODE_SET_EVAL_AGAIN:
				slot = variable->next->value.reference;
				if (slot) {
					mxPushKind(XS_REFERENCE_KIND);
					mxStack->value.reference = slot;
					mxSaveState;
					index = fxIsScopableSlot(the, slot, offset);
					mxRestoreState;
					if (index) {
						slot = fxGetProperty(the, slot, offset);
						if (slot) {
							mxSaveState;
							slot = fxSetProperty(the, mxStack->value.reference, offset, C_NULL);
							mxRestoreState;
							goto XS_CODE_SET_ALL;
						}
					}
					mxStack++;
				}
				slot = fxGetOwnProperty(the, variable, offset);
				if (slot) {
					variable = slot->value.closure;
					goto XS_CODE_SET_CLOSURE_ALL;
				}
				variable = variable->value.instance.prototype;
				if (variable)
					goto XS_CODE_SET_EVAL_AGAIN;
			}
			goto XS_CODE_SET_GLOBAL_ALL;
		}
	}
}

void fxRunEval(txMachine* the)
{	
	txStringStream aStream;
	txUnsigned flags;
	if (the->stack->value.integer == 0)
		the->stack->kind = XS_UNDEFINED_KIND;
	else {
		the->stack += the->stack->value.integer;
		if ((the->stack->kind == XS_STRING_KIND) || (the->stack->kind == XS_STRING_X_KIND)) {
			aStream.slot = the->stack;
			aStream.offset = 0;
			aStream.size = c_strlen(fxToString(the, aStream.slot));
			flags = mxProgramFlag | mxEvalFlag;
			if (the->frame->flag & XS_STRICT_FLAG)
				flags |= mxStrictFlag;
			fxRunScript(the, fxParseScript(the, &aStream, fxStringGetter, flags), mxThis, the->frame - 1, mxFunctionInstanceHome(mxFunction->value.reference), C_NULL);
			aStream.slot->kind = the->stack->kind;
			aStream.slot->value = the->stack->value;
			the->stack++;
		}
	}
}

txBoolean fxIsSameSlot(txMachine* the, txSlot* a, txSlot* b)
{	
	txBoolean result = 0;
	if (a->kind == b->kind) {
		if ((XS_UNDEFINED_KIND == a->kind) || (XS_NULL_KIND == a->kind))
			result = 1;
		else if (XS_BOOLEAN_KIND == a->kind)
			result = a->value.boolean == b->value.boolean;
		else if (XS_INTEGER_KIND == a->kind)
			result = a->value.integer == b->value.integer;
        else if (XS_NUMBER_KIND == a->kind)
			result = (!c_isnan(a->value.number)) && (!c_isnan(b->value.number)) && (a->value.number == b->value.number);
		else if ((XS_STRING_KIND == a->kind) || (XS_STRING_X_KIND == a->kind))
			result = c_strcmp(a->value.string, b->value.string) == 0;
		else if (XS_SYMBOL_KIND == a->kind)
			result = a->value.ID == b->value.ID;
		else if (XS_REFERENCE_KIND == a->kind)
			result = a->value.reference == b->value.reference;
	}
	else if ((XS_INTEGER_KIND == a->kind) && (XS_NUMBER_KIND == b->kind))
		result = (!c_isnan(b->value.number)) && ((txNumber)(a->value.integer) == b->value.number);
	else if ((XS_NUMBER_KIND == a->kind) && (XS_INTEGER_KIND == b->kind))
		result = (!c_isnan(a->value.number)) && (a->value.number == (txNumber)(b->value.integer));
	else if ((XS_STRING_KIND == a->kind) && (XS_STRING_X_KIND == b->kind))
		result = c_strcmp(a->value.string, b->value.string) == 0;
	else if ((XS_STRING_X_KIND == a->kind) && (XS_STRING_KIND == b->kind))
		result = c_strcmp(a->value.string, b->value.string) == 0;
	return result;
}

txBoolean fxIsScopableSlot(txMachine* the, txSlot* instance, txInteger id)
{	
	txBoolean result = 1;
	fxBeginHost(the);
	mxPushReference(instance);
	fxGetID(the, mxID(_Symbol_unscopables));
	if (mxIsReference(the->stack)) {
		fxGetID(the, id);
		result = fxToBoolean(the, the->stack) ? 0 : 1;
	}
	the->stack++;
	fxEndHost(the);
	return result;

	/*
	txSlot* unscopables = fxGetProperty(the, instance, mxID(_Symbol_unscopables));
	if (unscopables && (unscopables->kind == XS_REFERENCE_KIND)) {
		txSlot* unscopable = fxGetProperty(the, unscopables->value.reference, id);
		if (unscopable)
			return fxToBoolean(the, unscopable) ? 0 : 1;
	}
	return 1;
	*/
}

void fxRemapIDs(txMachine* the, txByte* codeBuffer, txSize codeSize, txID* theIDs)
{
	register const txS1* bytes = gxCodeSizes;
	register txByte* p = codeBuffer;
	register txByte* q = codeBuffer + codeSize;
	register txS1 offset;
	txU2 index;
	txID id;
	while (p < q) {
		//fprintf(stderr, "%s", gxCodeNames[*((txU1*)p)]);
		offset = bytes[*((txU1*)p)];
		if (0 < offset)
			p += offset;
		else if (0 == offset) {
			p++;
			mxDecode2(p, id);
			if (id != XS_NO_ID) {
				id &= 0x7FFF;
				id = theIDs[id];
				p -= 2;
				mxEncode2(p, id);
			}
		}
		else if (-1 == offset) {
			p++;
			index = *((txU1*)p);
			p += 1 + index;
		}
		else if (-2 == offset) {
			p++;
			mxDecode2(p, index);
			p += index;
		}
		//fprintf(stderr, "\n");
	}
}

void fxRunInstance(txMachine* the, txSlot* instance)
{
	txSlot* array;
	txInteger c = mxArgc, i;
	txSlot* address;
	mxPush(mxArrayPrototype);
	array = fxLastProperty(the, fxNewArrayInstance(the));
	fxSetArrayLength(the, array, c);
	i = 0;
	address = array->value.array.address;
	while (i < c) {
		txSlot* property = mxArgv(i);
		address->ID = XS_NO_ID;
		address->kind = property->kind;
		address->value = property->value;
		i++;
		address++;
	}
	if (mxTarget->kind == XS_UNDEFINED_KIND)
		fxCallInstance(the, instance, mxThis, the->stack);
	else
		fxConstructInstance(the, instance, the->stack, mxTarget);
	the->stack++;
}


void fxRunScript(txMachine* the, txScript* script, txSlot* _this, txSlot* environment, txSlot* home, txSlot* module)
{
	if (script) {
		mxTry(the) {
			txSlot* instance;
			txSlot* property;
			if (script->symbolsBuffer) {
				txByte* p = script->symbolsBuffer;
				txInteger c = *((txU1*)p), i;
				p++;
				c <<= 8;
				c += *((txU1*)p);
				p++;
				mxPushUndefined();
				the->stack->value.code = fxNewChunk(the, c * sizeof(txID));
				the->stack->kind = XS_CODE_KIND;
				for (i = 0; i < c; i++) {
					txSlot* key = fxNewNameC(the, (txString)p);
					((txID*)the->stack->value.code)[i] = key->ID;
					p += c_strlen((char*)p) + 1;
				}
				fxRemapIDs(the, script->codeBuffer, script->codeSize, (txID*)the->stack->value.code);
			}	
			else {
				mxPush(mxIDs);
			}
			if (script->callback) {
				property = the->stack;
				/* ARGC */
				mxPushInteger(0);
				/* THIS */
				if (module)
					mxPushSlot(module);
				else
					mxPush(mxGlobal);
				the->stack->ID = XS_NO_ID;
				/* FUNCTION */
				mxPush(mxFunctionPrototype);
				instance = fxNewFunctionInstance(the, environment ? mxID(_eval) : XS_NO_ID);
				instance->next->kind = XS_CALLBACK_KIND;
				instance->next->value.callback.address = script->callback;
				instance->next->value.callback.IDs = (txID*)property->value.code;
				if (module) {
					property = mxFunctionInstanceModule(instance);
					property->kind = module->kind;
					property->value = module->value;
				}
				/* TARGET */
				mxPushUndefined();
				/* RESULT */
				mxPushUndefined();
				fxRunID(the, C_NULL, XS_NO_ID);
			}
			else {
				mxPush(mxHosts);
			}
			if (!module)
				property = fxSetGlobalProperty(the, mxGlobal.value.reference, mxID(0), C_NULL);
			else
				property = fxSetProperty(the, module->value.reference, mxID(0), C_NULL);
			mxPullSlot(property);
			the->stack++;

			/* ARGC */
			mxPushInteger(0);
			/* THIS */
			if (_this)
				mxPushSlot(_this);
			else
				mxPushUndefined();
			the->stack->ID = XS_NO_ID;
			/* FUNCTION */
			mxPush(mxFunctionPrototype);
			instance = fxNewFunctionInstance(the, XS_NO_ID);
			instance->next->kind = XS_CODE_X_KIND;
			instance->next->value.code = script->codeBuffer;
			if (environment) {
				property = mxFunctionInstanceClosures(instance);
				property->kind = environment->kind;
				property->value = environment->value;
			}
			if (home) {
				property = mxFunctionInstanceHome(instance);
				property->kind = home->kind;
				property->value = home->value;
			}
			if (module) {
				property = mxFunctionInstanceModule(instance);
				property->kind = module->kind;
				property->value = module->value;
			}
			/* TARGET */
			mxPushUndefined();
			/* RESULT */
			mxPushUndefined();
			fxRunID(the, C_NULL, XS_NO_ID);

			if (!module)
				fxRemoveGlobalProperty(the, mxGlobal.value.reference, mxID(0));		
			if (script->symbolsBuffer)
				fxDeleteScript(script);
		}
		mxCatch(the) {
			if (!module)
				fxRemoveGlobalProperty(the, mxGlobal.value.reference, mxID(0));		
			if (script->symbolsBuffer)
				fxDeleteScript(script);
			fxJump(the);
		}
	}
	else {
		mxSyntaxError("invalid script");
	}
}












