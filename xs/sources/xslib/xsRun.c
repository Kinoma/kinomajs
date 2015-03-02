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
#if SUPPORT_NEON
#include "FskArch.h"
extern int FskHardwareGetARMCPU_All();
#endif


//#define mxTrace 1

#define mxAccelerator 1

static txSlot* fxRunAt(txMachine* the, txByte theIndex, txID* theID);
static void fxRunEnum(txMachine* the, txSlot* theProperty, txSlot* theLimit);
static void fxRemapID(txMachine* the, txByte* theCode, txID* theIDs);

static txByte* fxDefaultRemapIDsHook(txMachine* the, txByte* theCode, txID* theIDs);
static txByte* fxDefaultRunLoopHook(txMachine* the, txByte* theCode, txSlot** theRoute, txJump* theJump);

static txRemapIDsHook gxRemapIDsHook = fxDefaultRemapIDsHook;
static txRunLoopHook gxRunLoopHook = fxDefaultRunLoopHook;

txByte* fxDefaultRemapIDsHook(txMachine* the, txByte* theCode, txID* theIDs)
{
	mxCheck(the, 0);
	return theCode + 1;
}

txByte* fxDefaultRunLoopHook(txMachine* the, txByte* theCode, txSlot** theRoute, txJump* theJump)
{
	mxCheck(the, 0);
	return theCode + 1;
}

txRemapIDsHook fxGetRemapIDsHook()
{
	return gxRemapIDsHook;
}

txRunLoopHook fxGetRunLoopHook()
{
	return gxRunLoopHook;
}

void fxSetRemapIDsHook(txRemapIDsHook theHook)
{
	gxRemapIDsHook = theHook;
}

void fxSetRunLoopHook(txRunLoopHook theHook)
{
	gxRunLoopHook = theHook;
}


#ifdef mxTrace
short gxDoTrace = 1;
static char* gxCodeNames[XS_COUNT] = {
	"", "", "", "",
	"", "", "", "",
	"", "", "", "",
	"", "", "", "",
	"", "", "", "",
	"", "", "", "",
	"", "", "", "",
	"", "", "", "",
	"", "",

	/* XS_ADD */ "+",
	/* XS_ALIAS */ "alias",
	/* XS_BEGIN */ "begin",
	/* XS_BIND */ "obsolete",
	/* XS_BIT_AND */ "&",
	/* XS_BIT_NOT */ "~",
	/* XS_BIT_OR */ "|",
	/* XS_BIT_XOR */ "^",
	/* XS_BRANCH */ "branch",
	/* XS_BRANCH_ELSE */ "branch else",
	/* XS_BRANCH_IF */ "branch if",
	/* XS_BREAK */ "break",
	/* XS_CALL */ "call",
	/* XS_CATCH */ "catch",
	/* XS_DEBUGGER */ "debugger",
	/* XS_DECREMENT */ "decrement",
	/* XS_DELETE */ "delete",
	/* XS_DELETE_AT */ "delete at",
	/* XS_DELETE_MEMBER */ "delete member",
	/* XS_DELETE_MEMBER_AT */ "delete member at",
	/* XS_DIVIDE */ "/",
	/* XS_DUB */ "dub",
	/* XS_END */ "end",
	/* XS_ENUM */ "enum",
	/* XS_EQUAL */ "==",
	/* XS_FALSE */ "false",
	/* XS_FILE */ "file",
	/* XS_FUNCTION */ "function",
	/* XS_GET */ "get",
	/* XS_GET_AT */ "get at",
	/* XS_GET_MEMBER */ "get member",
	/* XS_GET_MEMBER_AT */ "get member at",
	/* XS_GLOBAL */ "global",
	/* XS_IN */ "in",
	/* XS_INCREMENT */ "increment",
	/* XS_INSTANCEOF */ "instanceof",
	/* XS_INSTANCIATE */ "instanciate",
	/* XS_INTEGER_8 */ "integer 8",
	/* XS_INTEGER_16 */ "integer 16",
	/* XS_INTEGER_32 */ "integer 32",
	/* XS_JUMP */ "jump",
	/* XS_LEFT_SHIFT */ "<<",
	/* XS_LESS */ "<",
	/* XS_LESS_EQUAL */ "<=",
	/* XS_LINE */ "line",
	/* XS_MINUS */ "-",
	/* XS_MODULO */ "%",
	/* XS_MORE */ ">",
	/* XS_MORE_EQUAL */ ">=",
	/* XS_MULTIPLY */ "*",
	/* XS_NEW */ "new", 
	/* XS_NOT */ "!",
	/* XS_NOT_EQUAL */ "!=",
	/* XS_NULL */ "null", 
	/* XS_NUMBER */ "number",
	/* XS_PARAMETERS */ "parameters",
	/* XS_PLUS */ "+",
	/* XS_POP */ "pop",
	/* XS_PUT_MEMBER */ "put member",
	/* XS_PUT_MEMBER_AT */ "put member at",
	/* XS_RESULT */ "result",
	/* XS_RETURN */ "return",
	/* XS_ROUTE */ "route",
	/* XS_SCOPE */ "scope",
	/* XS_SET */ "set",
	/* XS_SET_AT */ "set at",
	/* XS_SET_MEMBER */ "set member",
	/* XS_SET_MEMBER_AT */ "set member at",
	/* XS_SIGNED_RIGHT_SHIFT */ ">>",
	/* XS_STATUS */ "status",
	/* XS_STRICT_EQUAL */ "===",
	/* XS_STRICT_NOT_EQUAL */ "!==",
	/* XS_STRING */ "string",
	/* XS_SUBTRACT */ "-",
	/* XS_SWAP */ "swap",
	/* XS_THIS */ "this",
	/* XS_THROW */ "throw",
	/* XS_TRUE */ "true",
	/* XS_TYPEOF */ "typeof",
	/* XS_UNCATCH */ "uncatch",
	/* XS_UNDEFINED */ "undefined",
	/* XS_UNSCOPE */ "unscope",
	/* XS_UNSIGNED_RIGHT_SHIFT */ ">>>",
	/* XS_VOID */ "void",
	
	/* XS_ATTRIBUTE_PATTERN */ "attribute pattern",
	/* XS_DATA_PATTERN */ "data pattern",
	/* XS_PI_PATTERN */ "pi pattern",
	/* XS_EMBED_PATTERN */ "embed pattern",
	/* XS_JUMP_PATTERN */ "jump pattern",
	/* XS_REFER_PATTERN */ "refer pattern",
	/* XS_REPEAT_PATTERN */ "repeat pattern",
	/* XS_FLAG_INSTANCE */ "flag instance",
	
	/* XS_BRANCH2 */ "branch 2",
	/* XS_BRANCH_ELSE2 */ "branch else 2",
	/* XS_BRANCH_IF2 */ "branch if 2",
	/* XS_FUNCTION2 */ "function 2",
	/* XS_ROUTE2 */ "route 2",
	/* XS_GET_NEGATIVE_ID */ "get negative id",
	/* XS_SET_NEGATIVE_ID */ "set negative id",
	/* XS_BRANCH_ELSE_BOOL */ "branch else bool",
	/* XS_BRANCH_IF_BOOL */ "branche if bool",
	/* XS_BRANCH_ELSE_BOOL2 */ "branch else bool 2",
	/* XS_BRANCH_IF_BOOL2 */ "branch if bool 2",
	/* XS_STRING_POINTER */ "string simple",
	/* XS_STRING_CONCAT */ "string concat",
	/* XS_STRING_CONCATBY */ "string concat by",
	/* PSEUDO_BRANCH_ELSE0 */ "pseudo branch else 0",
	/* PSEUDO_BRANCH_ELSE1 */ "pseudo branch else 1",
	/* PSEUDO_GET_MEMBER */ "pseudo get member",
	/* PSEUDO_SET_MEMBER */ "pseudo set member",
	/* PSEUDO_LESS */ "pseudo less",
	/* PSEUDO_LESS_EQUAL */ "pseudo less equal",
	/* PSEUDO_INCREMENT */ "pseudo increment",
	/* PSEUDO_DECREMENT */ "pseudo decrement",
	/* PSEUDO_DUB */ "pseudo dub",
	/* PSEUDO_DOUBLE_GET_NEGATIVE */ "pseudo double get negative",
	/* PSEUDO_DOUBLE_GET_AT */ "pseudo double get at",
	/* PSEUDO_PUT_MEMBER */ "pseudo put member",
	/* XS_GET_MEMBER_FOR_CALL */ "get member for call",
	/* XS_GET_FOR_NEW */ "get for new",
	/* XS_NOP */"nop",
};

static void fxTraceCode(txMachine* the, txU1 theCode) 
{
	if (((XS_LABEL <= theCode) && (theCode < XS_COUNT)))
		fxReport(the, C_NULL, 0, "\n%d %s", the->stackTop - the->stack, gxCodeNames[theCode]);
	else
		fxReport(the, C_NULL, 0, "\n%d ?", the->stackTop - the->stack);
/*
	if (((XS_LABEL <= theCode) && (theCode < XS_COUNT)))
		fprintf(stderr, "\n%8.8lX %s", (txU4)the->stack, gxCodeNames[theCode]);
	else
		fprintf(stderr, "\n%8.8lX ?", (txU4)the->stack);
*/

}
static void fxTraceAcceleratorCode(txMachine* the, txU1 theCode, txSlot* theStack) 
{
	if (((XS_LABEL <= theCode) && (theCode < XS_COUNT)))
		fxReport(the, C_NULL, 0, "\n@ %d %s", the->stackTop - theStack, gxCodeNames[theCode]);
	else
		fxReport(the, C_NULL, 0, "\n@ %d ?", the->stackTop - theStack);
/*
	if (((XS_LABEL <= theCode) && (theCode < XS_COUNT)))
		fprintf(stderr, "\n%8.8lX %s", (txU4)the->stack, gxCodeNames[theCode]);
	else
		fprintf(stderr, "\n%8.8lX ?", (txU4)the->stack);
*/

}

static void fxTraceIndex(txMachine* the, txInteger theIndex) 
{
	fxReport(the, C_NULL, 0, " %d", theIndex);
	//fprintf(stderr, " %d", theIndex);
}

static void fxTraceReturn(txMachine* the) 
{
	fxReport(the, C_NULL, 0, "\n");
	//fprintf(stderr, "\n");
}

static void fxTraceSymbol(txMachine* the, txID theIndex) 
{
	txSlot* aSymbol = fxGetSymbol(the, theIndex);
	if (aSymbol)
		fxReport(the, C_NULL, 0, " [%s]", aSymbol->value.symbol.string);
	else if (theIndex != XS_NO_ID)
		fxReport(the, C_NULL, 0, " [%d]", theIndex);
	else
		fxReport(the, C_NULL, 0, " [?]");
/*
	txSlot* aSymbol = fxGetSymbol(the, theIndex);
	if (aSymbol)
		fprintf(stderr, " [%s]", aSymbol->value.symbol.string);
	else if (theIndex != XS_NO_ID)
		fprintf(stderr, " [%d]", theIndex);
	else
		fprintf(stderr, " [?]");
*/
}

#endif

#ifdef mxAccelerator
#include "xsAccelerator.c"
#endif

int count = 0;

void fxRunID(txMachine* the, txID theID)
{
	txSlot* aFunction = (the->stack + 1)->value.reference;
	mxInitSlot(--the->stack, XS_FRAME_KIND);
	the->stack->next = the->frame;
	the->stack->ID = theID;
	the->stack->flag = XS_NO_FLAG;
#ifdef mxDebug
	if (the->frame && (the->frame->flag & XS_STEP_INTO_FLAG))
		the->stack->flag = XS_STEP_INTO_FLAG | XS_STEP_OVER_FLAG;
#endif
	the->stack->value.frame.code = the->code;
	the->stack->value.frame.scope = the->scope;
	the->frame = the->stack;
	if (aFunction->next->kind == XS_CALLBACK_KIND) {
		the->scope = C_NULL;
		the->code = aFunction->next->next->value.closure.symbolMap;
		the->frame->flag |= XS_C_FLAG;
		mxInitSlot(--the->stack, XS_INTEGER_KIND);
#ifdef mxProfile
		fxDoCallback(the, aFunction->next->value.callback.address);
#else
		(*(aFunction->next->value.callback.address))(the);
#endif
	}
	else {
		txJump aJump;
		txSlot* volatile aRoute = C_NULL;
	#ifdef mxAccelerator
	#if (SUPPORT_NEON)
		if(count<1) {
			count++;
			the->accelerator = fxRunLoopAccelerator;
		} 
		else { //after load libfsk.so
			int features = FskHardwareGetARMCPU_All();
			if (features==FSK_ARCH_ARM_V7)
				the->accelerator = fxRunLoopAccelerator_arm_v7;
			else
				the->accelerator = fxRunLoopAccelerator;
		}
	#else
		the->accelerator = fxRunLoopAccelerator;
	#endif
	#endif
		
		the->code = aFunction->next->value.code;
	again:		
		aJump.nextJump = the->firstJump;
		aJump.stack = the->stack;
		aJump.scope = the->scope;
		aJump.frame = the->frame;
		aJump.code = the->code;
		the->firstJump = &aJump;
		if (c_setjmp(aJump.buffer) == 0) {
			fxRunLoop(the, (txSlot**)&aRoute, &aJump);
			the->firstJump = aJump.nextJump;
		}
		else {
			the->stack = aJump.stack;
			the->scope = aJump.scope;
			the->frame = aJump.frame;
			the->code = aJump.code;
			
			the->frame->flag |= XS_THROW_FLAG;
			//fprintf(stderr, "%8.8X %8.8X\n", aRoute, mxRoute);
			if (aRoute) {
				the->code = aRoute->value.route.code;
				the->stack = aRoute->value.route.stack;
				goto again;
			}
		}
		if (the->frame->flag & XS_THROW_FLAG)
			fxJump(the);
	}
	the->stack = mxArgv(-1);
	the->scope = the->frame->value.frame.scope;
	the->code = the->frame->value.frame.code;
	*(--the->stack) = *mxResult;
	the->frame = the->frame->next;
}


void fxRunLoop(txMachine* the, txSlot** theRoute, txJump* theJump)
{
	txID anID;
	txSize anOffset;
	txSlot* aScope;
	txSlot* aParameter;
	txSlot* aProperty = C_NULL;
	txSlot* anInstance;
	txSlot* aFunction;
	txSlot* aResult;
	txSlot aScratch;
	txBoolean aTest;
	txFlag aFlag;

	for (;;) {
	#ifdef mxAccelerator
		anID = the->accelerator(the, theRoute, theJump,&aProperty);
	#endif

	#ifdef mxFrequency
		the->fullFrequencies[*((txU1*)the->code)]++;
	#endif	
	
	#ifdef mxTrace
		if (gxDoTrace) 
			fxTraceCode(the, *((txU1*)the->code));
	#endif
		
		switch (*((txU1*)the->code++)) {
		case XS_BEGIN: {
			txInteger aParameterCount;
			txInteger aLocalCount;
			
#ifdef mxProfile
			fxBeginFunction(the);
#endif
			aProperty = the->stack + 2;	
			mxDecode2(the->code, aProperty->ID);
			aProperty->flag = XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG;
			aProperty->next = C_NULL;

			mxInitSlot(--the->stack, XS_ROUTE_KIND);
			the->stack->ID = XS_NO_ID;
			*theRoute = the->stack;

			aFlag = *the->code++;
			the->frame->flag |= aFlag & (XS_SANDBOX_FLAG | XS_STRICT_FLAG);
			
			mxZeroSlot(--the->stack); // scope
			mxZeroSlot(--the->stack); // binding
			mxZeroSlot(--the->stack); // parameters
					
			if (aFlag & XS_EVAL_FLAG) {
				if ((the->frame->next->ID == the->evalID) && ((the->frame->next + 3)->kind == XS_UNDEFINED_KIND)) {
					// direct call
				}
				else {
					*mxThis = mxGlobal;
					the->scope = &mxGlobal;
				}
				if (the->frame->flag & XS_STRICT_FLAG)
					aScope = the->scope;
				else
					aScope = C_NULL;
				mxZeroSlot(--the->stack);
			}
			else if (aFlag & XS_PROGRAM_FLAG) {
				*mxThis = mxGlobal;
				the->scope = &mxGlobal;
				aScope = C_NULL;
				mxZeroSlot(--the->stack);
			}
			else if (aFlag & XS_THIS_FLAG) {
				the->scope = &mxGlobal;
				aScope = C_NULL;
				mxZeroSlot(--the->stack);
			}
			else {
				if (!(the->frame->flag & XS_STRICT_FLAG)) {
					anInstance = mxThis;
					if (!mxIsReference(anInstance)) {
						if ((anInstance->kind == XS_UNDEFINED_KIND) || (anInstance->kind == XS_NULL_KIND))
							*mxThis = mxGlobal;
						else
							fxToInstance(the, mxThis);
					}
				}
				aScope = aProperty->value.reference->next->next; // function > instance > closure
				if (aScope->kind == XS_REFERENCE_KIND)
					aScope = aScope->value.reference->next; // closure > instance > scope
				else
					aScope = &mxGlobal;
			}
			if (aScope) {
				aParameter = the->frame + 4;
				aParameter->flag = XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG;
				aParameterCount = (txID)aParameter->value.integer;
				aParameter += aParameterCount;
				aLocalCount = *the->code++;
				while (aParameterCount && aLocalCount) {
					aProperty->next = aParameter;
					aProperty = aProperty->next;
					mxDecode2(the->code, aProperty->ID);
					aProperty->flag = XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG;
#ifdef mxTrace
					if (gxDoTrace)
						fxTraceSymbol(the, aProperty->ID);
#endif
					aLocalCount--;
					aParameterCount--;
					aParameter--;
				}
				while (aParameterCount) {
					aParameter->ID = XS_NO_ID;
					aParameter->flag = XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG;
					aParameterCount--;
					aParameter--;
				}
				while (aLocalCount) {
					mxZeroSlot(--the->stack);
					aProperty->next = the->stack;
					aProperty = aProperty->next;
					mxDecode2(the->code, aProperty->ID);
					aProperty->flag = XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG;
#ifdef mxTrace
					if (gxDoTrace)
						fxTraceSymbol(the, aProperty->ID);
#endif
					aLocalCount--;
				}
				aLocalCount = *the->code++;
				while (aLocalCount) {
					mxZeroSlot(--the->stack);
					aProperty->next = the->stack;
					aProperty = aProperty->next;
					mxDecode2(the->code, aProperty->ID);
					aProperty->flag = XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG;
#ifdef mxTrace
					if (gxDoTrace)
						fxTraceSymbol(the, aProperty->ID);
#endif
					aLocalCount--;
				}
				aProperty = mxFunction;
				anInstance = the->frame - 3;
				mxInitSlot(anInstance, XS_INSTANCE_KIND);
				anInstance->ID = XS_NO_ID;
				anInstance->flag = XS_STACK_FLAG;
				if (aProperty->ID != XS_NO_ID)
					anInstance->next = aProperty;
				else
					anInstance->next = aProperty->next;
				aProperty = the->frame - 2;
				mxInitSlot(aProperty, XS_REFERENCE_KIND);
				aProperty->next = aScope;
				aProperty->ID = XS_NO_ID;
				aProperty->value.reference = anInstance;
				the->scope = aProperty;
			}
			else {
				the->code++;
				aLocalCount = *the->code++;
				anInstance = the->scope->value.reference;
				while (aLocalCount) {
					mxDecode2(the->code, anID);
					aProperty = fxSetProperty(the, anInstance, anID, C_NULL);
					aProperty->flag |= XS_DONT_DELETE_FLAG;
					aLocalCount--;
				}
			}
		}	break;
		case XS_ROUTE:
			*theRoute = mxRoute;
			if (theJump) {
				theJump->stack = the->stack;
				theJump->scope = the->scope;
				theJump->frame = the->frame;
			}
			mxDecode2(the->code, anID);
			(*theRoute)->value.route.code = the->code + anID;
			(*theRoute)->value.route.stack = the->stack;
			break;
		case XS_ROUTE2:
			*theRoute = mxRoute;
			if (theJump) {
				theJump->stack = the->stack;
				theJump->scope = the->scope;
				theJump->frame = the->frame;
			}
			mxDecode4(the->code, anOffset);
			(*theRoute)->value.route.code = the->code + anOffset;
			(*theRoute)->value.route.stack = the->stack;
			break;
		case XS_PARAMETERS:
			anInstance = the->frame - 3;
			aProperty = mxParameters;
			fxNewArgumentsInstance(the);
			*aProperty = *(the->stack++);
			aProperty->next = anInstance->next;
			anInstance->next = aProperty;
			break;
		case XS_END:
			aProperty = mxParameters;
			if (aProperty->kind != XS_UNDEFINED_KIND) {
				txID aCount = (txID)mxArgc;
				anInstance = aProperty->value.reference;
				aProperty = anInstance->next;
				while (aProperty) {
					if ((aProperty->kind == XS_ACCESSOR_KIND) && (0 <= aProperty->ID) && (aProperty->ID < aCount)) {
						txSlot* aSlot = the->frame + 4 + aCount - aProperty->ID;
						aProperty->kind = aSlot->kind;
						aProperty->value = aSlot->value;
					}
					aProperty = aProperty->next;
				}
			}
			aScope = the->frame - 2;
			if ((aScope->kind == XS_REFERENCE_KIND) && !(aScope->value.reference->flag & XS_STACK_FLAG)) {
				txSlot* aSlot = aScope->value.reference->next;
				txSlot** aSlotAddress = &(aScope->value.reference->next);
				while (aSlot) {
					*aSlotAddress = fxDuplicateSlot(the, aSlot);
					aSlot = aSlot->next;
					aSlotAddress = &((*aSlotAddress)->next);
				}
			}
		
#ifdef mxProfile
			fxEndFunction(the);
#endif
			if (!the->frame->next || (the->frame->next->flag & XS_C_FLAG)) {
#ifdef mxTrace
				if (gxDoTrace)
					fxTraceReturn(the);
#endif
				return;	
			
			}
			if (the->frame->flag & XS_THROW_FLAG) {
				the->scope = the->frame->value.frame.scope;
				the->frame = the->frame->next;
				*theRoute = mxRoute;
				the->frame->flag |= XS_THROW_FLAG;
				the->code = (*theRoute)->value.route.code;
				the->stack = (*theRoute)->value.route.stack;
			}
			else {
				the->stack = mxArgv(-1);
				the->scope = the->frame->value.frame.scope;
				the->code = the->frame->value.frame.code;
				*(--the->stack) = *mxResult;
				the->frame = the->frame->next;
				*theRoute = mxRoute;
				if (theJump) {
					theJump->stack = the->stack;
					theJump->scope = the->scope;
					theJump->frame = the->frame;
				}
			}
			break;

		case XS_BIND:
			anID = *the->code++;
			aScope = the->scope;
			while (anID) {
				anID--;
				aScope = aScope->next;
			}
			mxInitSlot(--the->stack, aScope->kind);
			the->stack->value = aScope->value;
			break;
		case XS_BRANCH:
			mxDecode2(the->code, anID);
			the->code += anID;
			break;
		case XS_BRANCH2:
			mxDecode4(the->code, anOffset);
			the->code += anOffset;
			break;
		case XS_BRANCH_ELSE:
		case XS_BRANCH_ELSE_BOOL:
#ifndef mxAccelerator
			mxDecode2(the->code, anID);
#else
			the->code += 2;
#endif
			if (!fxRunTest(the)) 
				the->code += anID;
			break;
		case XS_BRANCH_ELSE2:
		case XS_BRANCH_ELSE_BOOL2:
			mxDecode4(the->code, anOffset);
			if (!fxRunTest(the)) 
				the->code += anOffset;
			break;
		case XS_BRANCH_IF:
		case XS_BRANCH_IF_BOOL:
#ifndef mxAccelerator
			mxDecode2(the->code, anID);
#else
			the->code += 2;
#endif
			if (fxRunTest(the)) 
				the->code += anID;
			break;
		case XS_BRANCH_IF2:
		case XS_BRANCH_IF_BOOL2:
			mxDecode4(the->code, anOffset);
			if (fxRunTest(the)) 
				the->code += anOffset;
			break;
		case XS_BREAK:
#ifdef mxTrace
			if (gxDoTrace)
				fxTraceReturn(the);
#endif
			the->code = NULL;
			return;
		case XS_DEBUGGER:
		#ifdef mxDebug
			fxDebugLoop(the, "debugger");
		#endif
			break;
		case XS_DUB:
			the->stack--;
			*(the->stack) = *(the->stack + 1);
			break;
		case XS_ENUM:
			aResult = the->stack;
			if (mxIsReference(aResult)) {
				anInstance = fxGetInstance(the, aResult);
				if (mxIsArray(anInstance)) {
					txInteger aLength;
					aProperty = anInstance->next;
					aLength = aProperty->value.array.length - 1;
					while (aLength >= 0) {
						if ((aProperty->value.array.address + aLength)->ID) {
							mxInitSlot(--the->stack, XS_INTEGER_KIND);
							the->stack->value.integer = aLength;
						}
						aLength--;
					}
				}
				if ((the->frame->flag & XS_SANDBOX_FLAG) || (anInstance->flag & XS_SANDBOX_FLAG)) {
					if (anInstance->flag & XS_SANDBOX_FLAG)
						anInstance = anInstance->value.instance.prototype;
					while (anInstance) {
						aProperty = anInstance->next;
						while (aProperty) {
							if (!(aProperty->flag & (XS_DONT_ENUM_FLAG | XS_DONT_SCRIPT_FLAG)))
								fxRunEnum(the, aProperty, aResult);
							aProperty = aProperty->next;
						}
						anInstance = fxGetParent(the, anInstance);
					}	
				}	
				else {
					while (anInstance) {
						aProperty = anInstance->next;
						while (aProperty) {
							if (!(aProperty->flag & (XS_DONT_ENUM_FLAG | XS_SANDBOX_FLAG)))
								fxRunEnum(the, aProperty, aResult);
							aProperty = aProperty->next;
						}
						anInstance = fxGetParent(the, anInstance);
					}
				}
			}
			mxInitSlot(aResult, XS_NULL_KIND);
			break;
		case XS_FILE:
			mxDecode2(the->code, (*theRoute)->ID);
			(*theRoute)->next = fxGetSymbol(the, (*theRoute)->ID);
			(*theRoute)->ID = 0;
			break;
		case XS_FUNCTION:
			mxDecode2(the->code, anID);
			mxPush(mxFunctionPrototype);
			anInstance = fxNewFunctionInstance(the);
			anInstance->next->value.code = (txByte *)fxNewChunk(the, anID);
			c_memcpy(anInstance->next->value.code, the->code, anID);
			the->code += anID;
			goto XS_FUNCTION_ALL;
		case XS_FUNCTION2:
			mxDecode4(the->code, anOffset);
			mxPush(mxFunctionPrototype);
			anInstance = fxNewFunctionInstance(the);
			anInstance->next->value.code = (txByte *)fxNewChunk(the, anOffset);
			c_memcpy(anInstance->next->value.code, the->code, anOffset);
			the->code += anOffset;
		XS_FUNCTION_ALL:
			if (the->scope != &mxGlobal) {
				txSlot* aScopeCopy;
				fxNewInstance(the);
				aScopeCopy = the->stack->value.reference;
				aScope = the->scope;
				while (aScope) {
					txSlot* aBinding = aScope->value.reference;
					txSlot* aBindingCopy;
					if (aBinding->flag & XS_STACK_FLAG) {
						aBinding->flag &= ~XS_STACK_FLAG;
						aBindingCopy = fxDuplicateSlot(the, aBinding);
						aBindingCopy->next = aBinding->next;
						aScope->value.reference = aBindingCopy;
					}	
					aScopeCopy = aScopeCopy->next = fxNewSlot(the);
					aScopeCopy->kind = aScope->kind;
					aScopeCopy->value = aScope->value;
					aScope = aScope->next;
				}
				aScopeCopy = anInstance->next->next;
				aScopeCopy->value = the->stack->value;
				aScopeCopy->kind = the->stack->kind;
				the->stack++;
			}
			break;
		case XS_GLOBAL:
			*(--the->stack) = the->stackTop[-1];
			break;
		case XS_INSTANCIATE:
			fxNewInstanceOf(the);
			break;
		case XS_LINE:
			mxDecode2(the->code, (*theRoute)->ID);
		#ifdef mxDebug
			fxDebugLine(the);
		#endif
			break;
		case PSEUDO_DUB:
			*the->stack = *(the->stack+1);
			break;
		case XS_POP:
			the->stack++;
			break;
		case XS_RESULT:
			//*mxResult = *(the->stack++);
			*mxResult = *(the->stack);
			break;
		case XS_SWAP:
			aScratch = *(the->stack);
			*(the->stack) = *(the->stack + 1);
			*(the->stack + 1) = aScratch;
			break;
		case PSEUDO_SET_MEMBER:
		case PSEUDO_GET_MEMBER:
		case XS_THIS:
			*(--the->stack) = *mxThis;
			break;
			
		case XS_JUMP:
			anID = (txID)the->stack->ID;
#ifdef mxTrace
			fxTraceIndex(the, anID);
#endif
			the->stack++;
			if (anID == XS_NO_ID) {
				the->frame->flag |= XS_THROW_FLAG;
				the->code = (*theRoute)->value.route.code;
				the->stack = (*theRoute)->value.route.stack;
			}
			else
				the->code += anID * 3;
			break;
		case XS_STATUS:
			mxDecode2(the->code, anID);
#ifdef mxTrace
			fxTraceIndex(the, anID);
#endif
			mxZeroSlot(--the->stack);
			the->stack->ID = anID;
			the->stack->value.integer = 0xF000F000;
			break;
		case XS_THROW:
			mxException = *(the->stack);
		#ifdef mxDebug
            fxToString(the, the->stack);
			fxDebugThrow(the, "throw %s", the->stack[0].value.string);
		#endif
		#if __FSK_LAYER__
			FskInstrumentedItemSendMessageMinimal(the, kFskXSInstrException, the);
		#endif
			the->frame->flag |= XS_THROW_FLAG;
			the->code = (*theRoute)->value.route.code;
			the->stack = (*theRoute)->value.route.stack;
			break;
			
		case XS_CATCH:
			mxDecode2(the->code, anID);
			the->frame->flag &= ~XS_THROW_FLAG;
			*(--the->stack) = mxException;
			mxZeroSlot(&mxException);
			the->stack->ID = anID;
			mxInitSlot(--the->stack, XS_INSTANCE_KIND);
			the->stack->next = the->stack + 1;
			the->stack->ID = XS_NO_ID;
			the->stack->flag |= XS_STACK_FLAG;
			mxInitSlot(--the->stack, XS_REFERENCE_KIND);
			the->stack->value.reference = the->stack + 1;
			the->stack->next = the->scope;
			the->stack->flag |= XS_THROW_FLAG; // @@
			the->scope = the->stack;
			break;
		case XS_UNCATCH:
			if (the->scope->flag & XS_THROW_FLAG) {
				the->scope = the->scope->next;
				*(the->stack + 3) = *(the->stack);
				the->stack += 3;
			}
			break;
			
		case XS_SCOPE:
			fxToInstance(the, the->stack);
			anInstance = fxGetInstance(the, the->stack);
			the->stack->value.reference = anInstance;
			the->stack->kind = XS_REFERENCE_KIND;
			the->stack->next = the->scope;
			the->stack->flag |= XS_THIS_FLAG;
			the->scope = the->stack;
			break;
		case XS_UNSCOPE:
			the->scope = the->scope->next;
			*(the->stack + 1) = *(the->stack);
			the->stack++;
			break;
			
			
		case XS_DELETE:
			mxZeroSlot(--the->stack);
			mxDecode2(the->code, anID);
			aTest = 1;
			if (anID != XS_NO_ID) {
				aScope = the->scope;
				aProperty = C_NULL;
				while (aScope) {
					if ((aProperty = fxGetOwnProperty(the, aScope->value.reference, anID)))
						break;
					aScope = aScope->next;
				}
				if (aProperty)
					aTest = fxRemoveProperty(the, aScope->value.reference, anID);
			}
			goto XS_DELETE_ALL;
		case XS_DELETE_AT:
			mxZeroSlot(--the->stack);
			anID = XS_NO_ID;
			fxRunAt(the, *the->code++, &anID);
			aTest = 0;
			goto XS_DELETE_ALL;
		case XS_DELETE_MEMBER:
			mxDecode2(the->code, anID);
			aTest = fxRemoveProperty(the, fxToOwnInstance(the, the->stack), anID);
			goto XS_DELETE_ALL;
		case XS_DELETE_MEMBER_AT:
			anID = XS_NO_ID;
			aTest = fxRemovePropertyAt(the, fxToOwnInstance(the, the->stack + 1), the->stack, &anID);
			the->stack++;
		XS_DELETE_ALL:
#ifdef mxTrace
			if (gxDoTrace) 
				fxTraceSymbol(the, anID);
#endif
			if (!aTest && (the->frame->flag & XS_STRICT_FLAG))
				mxDebugID(the, XS_TYPE_ERROR, "delete %s: no permission (strict mode)", anID);
			the->stack->value.boolean = aTest;
			the->stack->kind = XS_BOOLEAN_KIND;
			break;

		case XS_GET:
		case XS_GET_FOR_NEW:
			mxZeroSlot(--the->stack);
#ifndef mxAccelerator
			mxDecode2(the->code, anID);
#else
			the->code += 2;
			if(!aProperty)
#endif
			{
				aScope = the->scope;
				aProperty = C_NULL;
				do {
					if ((aProperty = fxGetProperty(the, aScope->value.reference, anID)))
						break;
					aScope = aScope->next;
				} while (aScope);
				if (!aProperty && (*the->code != XS_TYPEOF) && !(the->scope->flag & XS_THIS_FLAG))
				mxDebugID(the, XS_REFERENCE_ERROR, "get %s: no property", anID);
				if (aScope && (aScope->flag & XS_THIS_FLAG)) {
					the->stack->value = aScope->value;
					the->stack->kind = aScope->kind;
				}
			}
			goto XS_GET_ALL;
		case XS_GET_AT:
		case PSEUDO_BRANCH_ELSE1:
		case PSEUDO_INCREMENT:
		case PSEUDO_DECREMENT:
		case PSEUDO_BRANCH_ELSE0:
		case PSEUDO_LESS_EQUAL:
		case PSEUDO_LESS:
		case PSEUDO_DOUBLE_GET_NEGATIVE:
		case PSEUDO_DOUBLE_GET_AT:
		case XS_GET_NEGATIVE_ID:
			mxZeroSlot(--the->stack);
			anID = XS_NO_ID;
			aProperty = fxRunAt(the, *the->code++, &anID);
			goto XS_GET_ALL;
		case XS_GET_MEMBER:
		case XS_GET_MEMBER_FOR_CALL:
#ifndef mxAccelerator
			mxDecode2(the->code, anID);
#else
			the->code += 2;
			if(!aProperty)
#endif
				aProperty = fxGetProperty(the, fxToInstance(the, the->stack), anID);
			goto XS_GET_ALL;
		case XS_GET_MEMBER_AT:
			anID = XS_NO_ID;
#ifdef mxAccelerator
			if(!aProperty)
#endif
				aProperty = fxGetPropertyAt(the, fxToInstance(the, the->stack + 1), the->stack, &anID);
			the->stack++;
		XS_GET_ALL:
#ifdef mxTrace
			if (gxDoTrace) 
				fxTraceSymbol(the, anID);
#endif
			if (!aProperty) {
				if ((*the->code == XS_CALL) || (*the->code == XS_NEW)) {
					the->stack->ID = anID;
					--the->stack;
				}
				mxDebugID(the, XS_NO_ERROR, "get %s: no property", anID);
				mxZeroSlot(the->stack);
			}
			else if (aProperty->kind == XS_ACCESSOR_KIND) {
				mxInitSlot(--the->stack, XS_INTEGER_KIND);
				the->stack->value.integer = 0;
				/* THIS */
				if ((*the->code == XS_CALL) || (*the->code == XS_NEW)) {
					aScratch = *(the->stack + 1);
					*(--the->stack) = aScratch;
				}
				else {
					aScratch = *(the->stack);
					*(the->stack) = *(the->stack + 1);
					*(the->stack + 1) = aScratch;
				}
				/* FUNCTION */
				aFunction = aProperty->value.accessor.getter;
				if (!mxIsFunction(aFunction))
					mxDebugID(the, XS_TYPE_ERROR, "get %s: no getter", anID);
				mxInitSlot(--the->stack, XS_REFERENCE_KIND);
				the->stack->value.reference = aFunction;
				/* RESULT */
				mxZeroSlot(--the->stack);
				/* FRAME */
				mxInitSlot(--the->stack, XS_FRAME_KIND);
				the->stack->next = the->frame;
				the->stack->ID = anID;
				the->stack->flag = XS_NO_FLAG;
#ifdef mxDebug
				if (the->frame && (the->frame->flag & XS_STEP_INTO_FLAG))
					the->stack->flag |= XS_STEP_INTO_FLAG | XS_STEP_OVER_FLAG;
#endif
				the->stack->value.frame.code = the->code;
				the->stack->value.frame.scope = the->scope;
				if (aFunction->next->kind == XS_CALLBACK_KIND) {
					the->stack->flag |= XS_C_FLAG;
					the->frame = the->stack;
					the->scope = C_NULL;
					the->code = aFunction->next->next->value.closure.symbolMap;
					mxInitSlot(--the->stack, XS_INTEGER_KIND);
#ifdef mxProfile
					fxDoCallback(the, aFunction->next->value.callback.address);
#else
					(*(aFunction->next->value.callback.address))(the);
#endif
					the->stack = mxArgv(-1);
					the->scope = the->frame->value.frame.scope;
					the->code = the->frame->value.frame.code;
					*(--the->stack) = *mxResult;
					the->frame = the->frame->next;
				}
				else {
					the->frame = the->stack;
					the->code = aFunction->next->value.code;
				}
			}
			else {	
				if ((*the->code == XS_CALL) || (*the->code == XS_NEW)) {
					the->stack->ID = anID;
					--the->stack;
				}
				the->stack->kind = aProperty->kind;
				the->stack->value = aProperty->value;
			}
			break;
		case XS_CALL:
			anID = (the->stack + 1)->ID;
			aFunction = fxGetInstance(the, the->stack);
            if (!aFunction || !mxIsFunction(aFunction))
				mxDebugID(the, XS_TYPE_ERROR, "call %s: no function", anID);
			/* RESULT */
			mxZeroSlot(--the->stack);
			/* FRAME */
			mxInitSlot(--the->stack, XS_FRAME_KIND);
			the->stack->next = the->frame;
			the->stack->ID = anID;
			the->stack->flag = XS_NO_FLAG;
#ifdef mxDebug
			if (the->frame && (the->frame->flag & XS_STEP_INTO_FLAG))
				the->stack->flag |= XS_STEP_INTO_FLAG | XS_STEP_OVER_FLAG;
#endif
			the->stack->value.frame.code = the->code;
			the->stack->value.frame.scope = the->scope;
			if (aFunction->next->kind == XS_CALLBACK_KIND) {
				the->stack->flag |= XS_C_FLAG;
				the->frame = the->stack;
				the->scope = C_NULL;
				the->code = aFunction->next->next->value.closure.symbolMap;
				mxInitSlot(--the->stack, XS_INTEGER_KIND);
#ifdef mxProfile
				fxDoCallback(the, aFunction->next->value.callback.address);
#else
				(*(aFunction->next->value.callback.address))(the);
#endif
				the->stack = mxArgv(-1);
				the->scope = the->frame->value.frame.scope;
				the->code = the->frame->value.frame.code;
				*(--the->stack) = *mxResult;
				the->frame = the->frame->next;
			}
			else {
				the->frame = the->stack;
				the->code = aFunction->next->value.code;
			}
			break;
		case XS_NEW:
			anID = (the->stack + 1)->ID;
			aFunction = fxGetInstance(the, the->stack);
			if (!aFunction || !mxIsFunction(aFunction))
				mxDebugID(the, XS_TYPE_ERROR, "new %s: no function", anID);
			aProperty = aFunction->next->next->next;
			/* RESULT */
			mxInitSlot(--the->stack, aProperty->kind);
			the->stack->value = aProperty->value;
			fxNewInstanceOf(the);
			*(the->stack + 2) = *(the->stack); // THIS
			/* FRAME */
			mxInitSlot(--the->stack, XS_FRAME_KIND);
			the->stack->next = the->frame;
			the->stack->ID = anID;
			the->stack->flag = XS_NO_FLAG;
#ifdef mxDebug
			if (the->frame && (the->frame->flag & XS_STEP_INTO_FLAG))
				the->stack->flag |= XS_STEP_INTO_FLAG | XS_STEP_OVER_FLAG;
#endif
			the->stack->value.frame.code = the->code;
			the->stack->value.frame.scope = the->scope;
			if (aFunction->next->kind == XS_CALLBACK_KIND) {
				the->stack->flag |= XS_C_FLAG;
				the->frame = the->stack;
				the->scope = C_NULL;
				the->code = aFunction->next->next->value.closure.symbolMap;
				mxInitSlot(--the->stack, XS_INTEGER_KIND);
#ifdef mxProfile
				fxDoCallback(the, aFunction->next->value.callback.address);
#else
				(*(aFunction->next->value.callback.address))(the);
#endif
				the->stack = mxArgv(-1);
				the->scope = the->frame->value.frame.scope;
				the->code = the->frame->value.frame.code;
				*(--the->stack) = *mxResult;
				the->frame = the->frame->next;
			}
			else {
				the->frame = the->stack;
				the->code = aFunction->next->value.code;
			}
			if(*(txU1 *)the->code==XS_DUB) {
				the->stack--;
			    *(the->stack) = *(the->stack + 1);
				the->code++;
			}
			break;
			
		case PSEUDO_PUT_MEMBER:
		{
			/*txSlot aScratch;
			txFlag aMask ;
			aScratch = *(the->stack);
			*(the->stack) = *(the->stack + 1);
			*(the->stack + 1) = aScratch;
			mxDecode2(the->code, anID);
			aFlag = the->frame->flag & XS_SANDBOX_FLAG;
			aProperty = fxSetProperty(the, fxToOwnInstance(the, the->stack), anID, &aFlag);
			aMask = *the->code++;
			aFlag = *the->code++;
#ifdef mxTrace
			if (gxDoTrace) 
				fxTraceSymbol(the, anID);
#endif
			aProperty->flag = (aProperty->flag & (~(aMask & ~XS_ACCESSOR_FLAG))) | (aFlag & ~XS_ACCESSOR_FLAG);
			if (aFlag & XS_ACCESSOR_FLAG) {
				if (aProperty->kind != XS_ACCESSOR_KIND) {
					aProperty->kind = XS_ACCESSOR_KIND;
					aProperty->value.accessor.getter = C_NULL;
					aProperty->value.accessor.setter = C_NULL;
				}
				if (aFlag & XS_GETTER_FLAG)
					aProperty->value.accessor.getter = (the->stack + 1)->value.reference;
				else
					aProperty->value.accessor.setter = (the->stack + 1)->value.reference;
			}
			else {
				aProperty->kind = (the->stack + 1)->kind;
				aProperty->value = (the->stack + 1)->value;
			}
			the->stack+=2;*/
			break;
		}

		case XS_PUT_MEMBER:
			mxDecode2(the->code, anID);
			aFlag = the->frame->flag & XS_SANDBOX_FLAG;
			aProperty = fxSetProperty(the, fxToOwnInstance(the, the->stack), anID, &aFlag);
			goto XS_PUT_ALL;
		case XS_PUT_MEMBER_AT:
			anID = XS_NO_ID;
			aFlag = the->frame->flag & XS_SANDBOX_FLAG;
			aProperty = fxSetPropertyAt(the, fxToOwnInstance(the, the->stack + 1), the->stack, &anID, &aFlag);
			the->stack++;
		XS_PUT_ALL: {
			txFlag aMask = *the->code++;
			aFlag = *the->code++;
#ifdef mxTrace
			if (gxDoTrace) 
				fxTraceSymbol(the, anID);
#endif
			aProperty->flag = (aProperty->flag & (~(aMask & ~XS_ACCESSOR_FLAG))) | (aFlag & ~XS_ACCESSOR_FLAG);
			the->stack++;
			if (aFlag & XS_ACCESSOR_FLAG) {
				if (aProperty->kind != XS_ACCESSOR_KIND) {
					aProperty->kind = XS_ACCESSOR_KIND;
					aProperty->value.accessor.getter = C_NULL;
					aProperty->value.accessor.setter = C_NULL;
				}
				if (aFlag & XS_GETTER_FLAG)
					aProperty->value.accessor.getter = the->stack->value.reference;
				else
					aProperty->value.accessor.setter = the->stack->value.reference;
			}
			else {
				aProperty->kind = the->stack->kind;
				aProperty->value = the->stack->value;
			}
			} break;
			
		case XS_SET:
			mxZeroSlot(--the->stack);
#ifndef mxAccelerator
			mxDecode2(the->code, anID);
#else
			the->code += 2;
			if(!aProperty)
#endif
			{
				aScope = the->scope;
				aProperty = C_NULL;
				while (aScope) {
					if ((aProperty = fxGetOwnProperty(the, aScope->value.reference, anID)))
						break;
					aScope = aScope->next;
				}
				if (!aProperty) {
					if (the->frame->flag & XS_STRICT_FLAG)
						mxDebugID(the, XS_REFERENCE_ERROR, "set %s: no reference (strict mode)", anID);
					aScope = the->stackTop - 1;
					aProperty = fxSetProperty(the, aScope->value.reference, anID, C_NULL);
				}
				if (aScope && (aScope->flag & XS_THIS_FLAG)) {
					the->stack->value = aScope->value;
					the->stack->kind = aScope->kind;
				}
			}
			goto XS_SET_ALL;
		case XS_SET_NEGATIVE_ID:
		case XS_SET_AT:
			mxZeroSlot(--the->stack);
			anID = XS_NO_ID;
			aProperty = fxRunAt(the, *the->code++, &anID);
			goto XS_SET_ALL;
		case XS_SET_MEMBER:
#ifndef mxAccelerator
			mxDecode2(the->code, anID);
#else
			the->code += 2;
			if(!aProperty)
#endif
				aProperty = fxSetProperty(the, fxToOwnInstance(the, the->stack), anID, C_NULL);
			goto XS_SET_ALL;
		case XS_SET_MEMBER_AT:
			anID = XS_NO_ID;
#ifdef mxAccelerator
			if(!aProperty)
#endif
				aProperty = fxSetPropertyAt(the, fxToOwnInstance(the, the->stack + 1), the->stack, &anID, C_NULL);
			the->stack++;
		XS_SET_ALL:
#ifdef mxTrace
			if (gxDoTrace) 
				fxTraceSymbol(the, anID);
#endif
			if (!aProperty)
				mxDebugID(the, XS_TYPE_ERROR, "set %s: no patch", anID);
			else if (aProperty->kind == XS_ACCESSOR_KIND) {
				mxInitSlot(--the->stack, XS_INTEGER_KIND);
				the->stack->value.integer = 1;
				/* THIS */
				aScratch = *(the->stack);
				*(the->stack) = *(the->stack + 1);
				*(the->stack + 1) = aScratch;
				/* FUNCTION */
				aFunction = aProperty->value.accessor.setter;
				if (aFunction && mxIsFunction(aFunction)) {
					mxInitSlot(--the->stack, XS_REFERENCE_KIND);
					the->stack->value.reference = aFunction;
					/* RESULT */
					the->stack--;
					*(the->stack) = *(the->stack + 4);
					/* FRAME */
					mxInitSlot(--the->stack, XS_FRAME_KIND);
					the->stack->next = the->frame;
					the->stack->ID = anID;
					the->stack->flag = XS_NO_FLAG;
	#ifdef mxDebug
					if (the->frame && (the->frame->flag & XS_STEP_INTO_FLAG))
						the->stack->flag |= XS_STEP_INTO_FLAG | XS_STEP_OVER_FLAG;
	#endif
					the->stack->value.frame.code = the->code;
					the->stack->value.frame.scope = the->scope;
					if (aFunction->next->kind == XS_CALLBACK_KIND) {
						the->stack->flag |= XS_C_FLAG;
						the->frame = the->stack;
						the->scope = C_NULL;
						the->code = aFunction->next->next->value.closure.symbolMap;
						mxInitSlot(--the->stack, XS_INTEGER_KIND);
	#ifdef mxProfile
						fxDoCallback(the, aFunction->next->value.callback.address);
	#else
						(*(aFunction->next->value.callback.address))(the);
	#endif
						the->stack = mxArgv(-1);
						the->scope = the->frame->value.frame.scope;
						the->code = the->frame->value.frame.code;
						*(--the->stack) = *mxResult;
						the->frame = the->frame->next;
					}
					else {
						the->frame = the->stack;
						the->code = aFunction->next->value.code;
					}
				}
				else {
					the->stack += 2;
					if (the->frame->flag & XS_STRICT_FLAG)
						mxDebugID(the, XS_TYPE_ERROR, "set %s: no setter", anID);
				}
			}
			else {
				the->stack++;
				if (aProperty->flag & XS_DONT_SET_FLAG) {
					if (the->frame->flag & XS_STRICT_FLAG)
						mxDebugID(the, XS_TYPE_ERROR, "set %s: no permission (strict mode)", anID);
				}
				else {
					aProperty->kind = the->stack->kind;
					aProperty->value = the->stack->value;
				}
			}
			break;
		case XS_UNDEFINED:
			mxZeroSlot(--the->stack);
			break; 
		case XS_NULL:
			mxInitSlot(--the->stack, XS_NULL_KIND);
			break; 
		case XS_FALSE:
			mxInitSlot(--the->stack, XS_BOOLEAN_KIND);
			break;
		case XS_TRUE:
			mxInitSlot(--the->stack, XS_BOOLEAN_KIND);
			the->stack->value.boolean = 1;
			break;
		case XS_INTEGER_8:
			mxInitSlot(--the->stack, XS_INTEGER_KIND);
			the->stack->value.integer = *the->code++;
#ifdef mxTrace
			if (gxDoTrace)
				fxTraceIndex(the, the->stack->value.integer);
#endif
			break;
		case XS_INTEGER_16:
			mxInitSlot(--the->stack, XS_INTEGER_KIND);
			mxDecode2(the->code, anID);
			the->stack->value.integer = anID;
#ifdef mxTrace
			if (gxDoTrace)
				fxTraceIndex(the, the->stack->value.integer);
#endif
			break;
		case XS_INTEGER_32:
			mxInitSlot(--the->stack, XS_INTEGER_KIND);
			mxDecode4(the->code, the->stack->value.integer);
#ifdef mxTrace
			if (gxDoTrace)
				fxTraceIndex(the, the->stack->value.integer);
#endif
			break;
		case XS_NUMBER:
			mxInitSlot(--the->stack, XS_NUMBER_KIND);
			mxDecode8(the->code, the->stack->value.number);
			break;

		case XS_STRING_POINTER:
		{
#if mxStringLength
			mxDecode2(the->code,anID);
#else
			anID = c_strlen((txString)the->code) + 1;
#endif
			mxZeroSlot(--the->stack);
			the->stack->value.string = (txString)the->code;
			the->stack->kind = XS_STRING_KIND;
			the->code += anID;
		}
		break;

		case XS_STRING_CONCATBY: 
		{
			txString result;
			txSize aSize;
			txSize bSize;
#if mxStringAccelerator
	#ifndef mxAccelerator 
			mxZeroSlot(--the->stack);
		#if mxStringLength
			mxDecode2(the->code,anID);
		#else
			anID = c_strlen((txString)the->code) + 1;
		#endif
	#else
		#if mxStringLength
			the->code += 2;
		#endif
	#endif
#else
			mxZeroSlot(--the->stack);
	#if mxStringLength
	 		mxDecode2(the->code,anID);
	#else
			anID = c_strlen((txString)the->code) + 1;
	#endif
#endif
			the->stack->value.string = (txString)fxNewChunk(the, anID);
			the->stack->kind = XS_STRING_KIND;
			c_memmove(the->stack->value.string, the->code, anID);
			the->code += anID;
			
			fxToString(the, the->stack + 1);
			aSize = anID - 1;
			bSize = c_strlen((the->stack + 1)->value.string);
			result = (txString)fxNewChunk(the, aSize + bSize + 1);
			c_memcpy(result, the->stack->value.string, aSize);
			c_memcpy(result + aSize, (the->stack + 1)->value.string, bSize + 1);
			(the->stack + 1)->value.string = result;
			the->stack++;
			//the->code++;
		}
		break;

		case XS_STRING: 
		case XS_STRING_CONCAT: 
		{
#if mxStringAccelerator
	#ifndef mxAccelerator 
			mxZeroSlot(--the->stack);
		#if mxStringLength
			mxDecode2(the->code,anID);
		#else
			anID = c_strlen((txString)the->code) + 1;
		#endif
	#else
		#if mxStringLength
			the->code += 2;
		#endif
	#endif
#else
			mxZeroSlot(--the->stack);
	#if mxStringLength
	 		mxDecode2(the->code,anID);
	#else
			anID = c_strlen((txString)the->code) + 1;
	#endif
#endif
			the->stack->value.string = (txString)fxNewChunk(the, anID);
			the->stack->kind = XS_STRING_KIND;
			c_memmove(the->stack->value.string, the->code, anID);
			the->code += anID;
		}
		break;

		case XS_VOID:
			mxZeroSlot(the->stack);
			break;
		
		case XS_TYPEOF:
			aScratch = *(the->stack);
			mxZeroSlot(the->stack);
			switch (aScratch.kind) {
			case XS_UNDEFINED_KIND:
				fxCopyStringC(the, the->stack, "undefined");
				break;
			case XS_NULL_KIND:
				fxCopyStringC(the, the->stack, "object");
				break;
			case XS_BOOLEAN_KIND:
				fxCopyStringC(the, the->stack, "boolean");
				break;
			case XS_INTEGER_KIND:
			case XS_NUMBER_KIND:
				fxCopyStringC(the, the->stack, "number");
				break;
			case XS_STRING_KIND:
				fxCopyStringC(the, the->stack, "string");
				break;
			case XS_REFERENCE_KIND:
			case XS_ALIAS_KIND:
				anInstance = fxGetInstance(the, &aScratch);
				if (mxIsFunction(anInstance))
					fxCopyStringC(the, the->stack, "function");
				else
					fxCopyStringC(the, the->stack, "object");
				break;
			}
			break;
			
		case XS_INCREMENT:
			aResult = the->stack;
			if (aResult->kind == XS_INTEGER_KIND) {
				aResult->value.integer++;
			} 
			else {
				fxToNumber(the, aResult);
				aResult->value.number++;
			}
			break;
		case XS_DECREMENT:
			aResult = the->stack;
			if (aResult->kind == XS_INTEGER_KIND) {
				aResult->value.integer--;
			} 
			else {
				fxToNumber(the, aResult);
				aResult->value.number--;
			}
			break;
		case XS_PLUS:
			aResult = the->stack;
			if (aResult->kind == XS_INTEGER_KIND) {
			} 
			else
				fxToNumber(the, aResult);
			break;
		case XS_MINUS:
			aResult = the->stack;
			if (aResult->kind == XS_INTEGER_KIND) {
				aResult->value.integer = -aResult->value.integer;
			} 
			else {
				fxToNumber(the, aResult);
				aResult->value.number = -aResult->value.number;
			} 
			break;
		case XS_BIT_NOT:
			aResult = the->stack;
			fxToInteger(the, aResult);
			aResult->value.integer = ~aResult->value.integer;
			break;
		case XS_NOT:
			aResult = the->stack;
			fxToBoolean(the, aResult);
			aResult->value.boolean = !aResult->value.boolean;
			break;

		case XS_MULTIPLY:
			aParameter = the->stack;
			aResult = the->stack + 1;
			fxToNumber(the, aResult);
			fxToNumber(the, aParameter);
			aResult->value.number *= aParameter->value.number;
			the->stack++;
			break;
		case XS_DIVIDE:
			aParameter = the->stack;
			aResult = the->stack + 1;
			fxToNumber(the, aResult);
			fxToNumber(the, aParameter);
			aResult->value.number /= aParameter->value.number;
			the->stack++;
			break;
		case XS_MODULO:
			aParameter = the->stack;
			aResult = the->stack + 1;
			if ((aResult->kind == XS_INTEGER_KIND) && (aParameter->kind == XS_INTEGER_KIND) && (aParameter->value.integer != 0)) {
				aResult->value.integer %= aParameter->value.integer;
			} 
			else {
				fxToNumber(the, aResult);
				fxToNumber(the, aParameter);
				aResult->value.number = c_fmod(aResult->value.number, aParameter->value.number);
			}
			the->stack++;
			break;
		
		case XS_ADD:
			aParameter = the->stack;
			aResult = the->stack + 1;
			if ((aResult->kind == XS_STRING_KIND) || (aParameter->kind == XS_STRING_KIND)) {
				fxToString(the, aResult);
				fxToString(the, aParameter);
				fxConcatString(the, aResult, aParameter);
			}
			else if ((aResult->kind == XS_INTEGER_KIND) && (aParameter->kind == XS_INTEGER_KIND)) {
				//aResult->value.integer += aParameter->value.integer;
				txInteger a = aResult->value.integer;
				txInteger b = aParameter->value.integer;
				txInteger c = a + b;
				if (((a ^ c) & (b ^ c)) < 0)
					goto XS_ADD_OVERFLOW;
				aResult->value.integer = c;
			} 
			else {
		XS_ADD_OVERFLOW:
				fxToNumber(the, aResult);
				fxToNumber(the, aParameter);
				aResult->value.number += aParameter->value.number;
			}
			the->stack++;
			break;
		case XS_SUBTRACT:
			aParameter = the->stack;
			aResult = the->stack + 1;
			if ((aResult->kind == XS_INTEGER_KIND) && (aParameter->kind == XS_INTEGER_KIND)) {
				aResult->value.integer -= aParameter->value.integer;
			} 
			else {
				fxToNumber(the, aResult);
				fxToNumber(the, aParameter);
				aResult->value.number -= aParameter->value.number;
			}
			the->stack++;
			break;
			
		case XS_LEFT_SHIFT:
			aParameter = the->stack;
			aResult = the->stack + 1;
			fxToInteger(the, aResult);
			fxToInteger(the, aParameter);
			aResult->value.integer <<= aParameter->value.integer & 0x1f;
			the->stack++;
			break;
		case XS_SIGNED_RIGHT_SHIFT:
			aParameter = the->stack;
			aResult = the->stack + 1;
			fxToInteger(the, aResult);
			fxToInteger(the, aParameter);
			aResult->value.integer >>= aParameter->value.integer & 0x1f;
			the->stack++;
			break;
		case XS_UNSIGNED_RIGHT_SHIFT:
			aParameter = the->stack;
			aResult = the->stack + 1;
			{
				txUnsigned left = fxToUnsigned(the, aResult);
				txUnsigned right = fxToUnsigned(the, aParameter);
				fxUnsigned(the, aResult, left >> (right & 0x1F));
			}
			/*fxToNumber(the, aResult);
			fxToInteger(the, aParameter);
			{
				txU4 temp = (txU4)(aResult->value.number);
				temp >>= aParameter->value.integer & 0x1f;
				if (((txS4)temp) >= 0) {
					aResult->value.integer = temp;
					aResult->kind = XS_INTEGER_KIND;
				}
				else {
					aResult->value.number = temp;
					aResult->kind = XS_NUMBER_KIND;
				}
			}*/
			the->stack++;
			break;
			
		case XS_LESS:
			fxRunCompare(the, 0, 1, 0, 0);
			break;
		case XS_LESS_EQUAL:
			fxRunCompare(the, 0, 1, 1, 0);
			break;
		case XS_MORE:
			fxRunCompare(the, 0, 0, 0, 1);
			break;
		case XS_MORE_EQUAL:
			fxRunCompare(the, 0, 0, 1, 1);
			break;
		case XS_INSTANCEOF:
			if (!mxIsReference(the->stack))
				mxDebug0(the, XS_TYPE_ERROR, "instanceof: no reference");
			aFunction = fxGetInstance(the, the->stack);
			if (!mxIsFunction(aFunction))
				mxDebug0(the, XS_TYPE_ERROR, "instanceof: no constructor");
			aProperty = aFunction->next->next->next;
			the->stack++;
			aParameter = fxGetInstance(the, the->stack);
			mxInitSlot(the->stack, XS_BOOLEAN_KIND);
			if (aProperty->kind == XS_ALIAS_KIND) {
				while (aParameter) {
					if (aParameter->ID == aProperty->value.alias) {
						the->stack->value.boolean = 1;
						break;
					}
					aParameter = fxGetParent(the, aParameter);
				}
			}
			else if (aProperty->kind == XS_REFERENCE_KIND) {
				while (aParameter) {
					if (aParameter == aProperty->value.reference) {
						the->stack->value.boolean = 1;
						break;
					}
					aParameter = fxGetParent(the, aParameter);
                }
			}
			break;
		case XS_IN:
			if (!mxIsReference(the->stack))
				mxDebug0(the, XS_REFERENCE_ERROR, "in: no reference");
			anInstance = fxGetInstance(the, the->stack); 
			anID = XS_NO_ID;
			aProperty = fxGetPropertyAt(the, anInstance, the->stack + 1, &anID);
			the->stack++;
			mxInitSlot(the->stack, XS_BOOLEAN_KIND);
			if (aProperty)
				the->stack->value.boolean = 1;
			break;
			
		case XS_EQUAL:
			fxRunEqual(the, 0, 1, 0);
			break;
		case XS_NOT_EQUAL:
			fxRunEqual(the, 0, 0, 1);
			break;
			
		case XS_STRICT_EQUAL:
			fxRunStrictEqual(the, 0, 1, 0);
			break;
		case XS_STRICT_NOT_EQUAL:
			fxRunStrictEqual(the, 0, 0, 1);
			break;
			
		case XS_BIT_AND:
			aParameter = the->stack;
			aResult = the->stack + 1;
			fxToInteger(the, aResult);
			fxToInteger(the, aParameter);
			aResult->value.integer &= aParameter->value.integer;
			the->stack++;
			break;
		case XS_BIT_OR:
			aParameter = the->stack;
			aResult = the->stack + 1;
			fxToInteger(the, aResult);
			fxToInteger(the, aParameter);
			aResult->value.integer |= aParameter->value.integer;
			the->stack++;
			break;
		case XS_BIT_XOR:
			aParameter = the->stack;
			aResult = the->stack + 1;
			fxToInteger(the, aResult);
			fxToInteger(the, aParameter);
			aResult->value.integer ^= aParameter->value.integer;
			the->stack++;
			break;
			
			

		case XS_ATTRIBUTE_PATTERN:
		case XS_DATA_PATTERN:
		case XS_PI_PATTERN:
		case XS_EMBED_PATTERN:
		case XS_JUMP_PATTERN:
		case XS_REFER_PATTERN:
		case XS_REPEAT_PATTERN: {
			txKind aKind;
			txID aCount;
			txPatternData* aPatternData;
			txPatternPart* aPatternPart;
			
			aKind = *(the->code - 1);
			aCount = *the->code++;
			aPatternData = 
				(txPatternData *)fxNewChunk(the, sizeof(txPatternData) + ((aCount - 1) * sizeof(txPatternPart)));
		#ifdef mxDebug
			if ((*theRoute)->next) {
				aPatternData->pathID = (*theRoute)->next->ID;
				aPatternData->line = (*theRoute)->ID;
			}
		#endif
			aPatternData->alias = the->stack->value.alias;
			aPatternData->count = aCount;
			aPatternPart = aPatternData->parts;
			while (aCount) {
				mxDecode2(the->code, aPatternPart->namespaceID);
				mxDecode2(the->code, aPatternPart->nameID);
				aCount--;
				aPatternPart++;
			}
			the->stack->value.pattern = aPatternData;
			the->stack->kind = aKind;
			} break;
			
		case XS_FLAG_INSTANCE:
			if (the->stack->kind != XS_INTEGER_KIND)	
				mxDebug0(the, XS_TYPE_ERROR, "flag instance: no flag");
			the->stack++;
			if (!mxIsReference(the->stack))	
				mxDebug0(the, XS_TYPE_ERROR, "flag instance: no instance");
			anInstance = fxGetOwnInstance(the, the->stack);
			anInstance->flag = (txFlag)((the->stack - 1)->value.integer);
			break;
		case XS_ALIAS:
			if (the->stack->kind != XS_REFERENCE_KIND)	
				mxDebug0(the, XS_TYPE_ERROR, "alias instance: no instance");
			fxAliasInstance(the, the->stack);
			break;

		default:
			--the->code;
			the->code = (*gxRunLoopHook)(the, the->code, theRoute, theJump);
			break;
		}
	}
}

void fxRunAdd(txMachine* the)
{
	txSlot* aParameter = the->stack;
	txSlot* aResult = the->stack + 1;
	if ((aResult->kind == XS_STRING_KIND) || (aParameter->kind == XS_STRING_KIND)) {
		fxToString(the, aResult);
		fxToString(the, aParameter);
		fxConcatString(the, aResult, aParameter);
	}
	else if ((aResult->kind == XS_INTEGER_KIND) && (aParameter->kind == XS_INTEGER_KIND)) {
		//aResult->value.integer += aParameter->value.integer;
		txInteger a = aResult->value.integer;
		txInteger b = aParameter->value.integer;
		txInteger c = a + b;
		if (((a ^ c) & (b ^ c)) < 0)
			goto XS_ADD_OVERFLOW;
		aResult->value.integer = c;
	} 
	else {
XS_ADD_OVERFLOW:
		fxToNumber(the, aResult);
		fxToNumber(the, aParameter);
		aResult->value.number += aParameter->value.number;
	}
	the->stack++;
}

void fxRunArguments(txMachine* the)
{
	fxNewArgumentsInstance(the);
}

txSlot* fxRunAt(txMachine* the, txByte theIndex, txID* theID)
{
	txByte aCount = (txByte)mxArgc;
	txByte aLength = *(mxFunction->value.reference->next->value.code + 4);
	txSlot* aProperty;
	if (theIndex >= 0) {
		if (theIndex < aCount)
			aProperty = the->frame + 4 + aCount - theIndex;
		else
			aProperty = the->frame - 5 - (theIndex - aCount);
	}
	else {
		if (aLength <= aCount)
			aProperty = the->frame - 4 + theIndex;
		else
			aProperty = the->frame - 4 - (aLength - aCount) + theIndex;
	}
	*theID = aProperty->ID;
	return aProperty;
}

txBoolean fxRunCompare(txMachine* the, txBoolean testing, txBoolean theLess, txBoolean theEqual, txBoolean theMore)
{
	txBoolean result = 0; 
	txSlot* b = the->stack;
	txSlot* a = the->stack + 1;
	fxToPrimitive(the, a, XS_NUMBER_HINT);
	fxToPrimitive(the, b, XS_NUMBER_HINT);
	if ((a->kind == XS_STRING_KIND) && (b->kind == XS_STRING_KIND)) {
		int c = c_strcmp(a->value.string, b->value.string);
		if (c < 0)
			result = theLess;
		else if (c > 0)
			result = theMore;
		else
			result = theEqual;
	}
	else if ((a->kind == XS_INTEGER_KIND) && (b->kind == XS_INTEGER_KIND)) {
		if (a->value.integer < b->value.integer)
			result = theLess;
		else if (a->value.integer > b->value.integer)
			result = theMore;
		else
			result = theEqual;
	}
	else {
		fxToNumber(the, a);
		fxToNumber(the, b);
		if (c_isnan(a->value.number) || c_isnan(b->value.number))
			result = 0;
		else {
			if (a->value.number < b->value.number)
				result = theLess;
			else if (a->value.number > b->value.number)
				result = theMore;
			else
				result = theEqual;
		}	
	}
	if (testing)
		the->stack += 2;
	else {
		mxInitSlot(a, XS_BOOLEAN_KIND);
		a->value.boolean = result;
		the->stack++;
	}
	return result;
}

void fxRunDelta(txMachine* the, txInteger theDelta)
{	
	txSlot* aResult = the->stack;
	if (aResult->kind == XS_INTEGER_KIND) {
		aResult->value.integer += theDelta;
	} 
	else {
		fxToNumber(the, aResult);
		aResult->value.number += theDelta;
	}
}

void fxRunEnum(txMachine* the, txSlot* theProperty, txSlot* theLimit)
{
	if (!(theProperty->ID & 0x8000)) {
		mxInitSlot(--the->stack, XS_INTEGER_KIND);
		the->stack->value.integer = theProperty->ID;
	}
	else {
		txSlot* aSymbol = fxGetSymbol(the, theProperty->ID);
		if (aSymbol) {
			txSlot* aParameter = the->stack;
			while (aParameter < theLimit) {
				if (aParameter->value.string == aSymbol->value.symbol.string)
					break;
				aParameter++;
			}
			if (aParameter == theLimit) {
				mxInitSlot(--the->stack, XS_STRING_KIND);
				the->stack->value.string = aSymbol->value.symbol.string;
			}
		}
	}
}

txBoolean fxRunEqual(txMachine* the, txBoolean testing, txBoolean yes, txBoolean no)
{
	txBoolean result = no; 
	txSlot* b = the->stack;
	txSlot* a = the->stack + 1;
again:	
	if (a->kind == b->kind) {
		switch (a->kind) {
		case XS_UNDEFINED_KIND:
		case XS_NULL_KIND:
			result = yes;
			break;
		case XS_BOOLEAN_KIND:
			result = (a->value.boolean == b->value.boolean) ? yes : no;
			break;
		case XS_INTEGER_KIND:
			result = (a->value.integer == b->value.integer) ? yes : no;
			break;
		case XS_NUMBER_KIND:
			result = ((!c_isnan(a->value.number)) && (!c_isnan(b->value.number)) && (a->value.number == b->value.number)) ? yes : no;
			break;
		case XS_STRING_KIND:
			result = (c_strcmp(a->value.string, b->value.string) == 0) ? yes : no;
			break;
		case XS_REFERENCE_KIND:
		case XS_ALIAS_KIND:
			result = (fxGetInstance(the, a) == fxGetInstance(the, b)) ? yes : no;
			break;
		}
	}
	else if ((a->kind == XS_INTEGER_KIND) && (b->kind == XS_NUMBER_KIND)) {
		fxToNumber(the, a);
		goto again;
	}
	else if ((a->kind == XS_NUMBER_KIND) && (b->kind == XS_INTEGER_KIND)) {
		fxToNumber(the, b);
		goto again;
	}
	else if ((a->kind == XS_INTEGER_KIND) && (b->kind == XS_STRING_KIND)) {
		fxToNumber(the, a);
		fxToNumber(the, b);
		goto again;
	}
	else if ((a->kind == XS_STRING_KIND) && (b->kind == XS_INTEGER_KIND)) {
		fxToNumber(the, a);
		fxToNumber(the, b);
		goto again;
	}
	else if ((a->kind == XS_UNDEFINED_KIND) && (b->kind == XS_NULL_KIND))
		result = yes;
	else if ((a->kind == XS_NULL_KIND) && (b->kind == XS_UNDEFINED_KIND))
		result = yes;
	else if ((a->kind == XS_NUMBER_KIND) && (b->kind == XS_STRING_KIND)) {
		fxToNumber(the, b);
		goto again;
	}
	else if ((a->kind == XS_STRING_KIND) && (b->kind == XS_NUMBER_KIND)) {
		fxToNumber(the, a);
		goto again;
	}
	else if (a->kind == XS_BOOLEAN_KIND) {
		fxToNumber(the, a);
		goto again;
	}
	else if (b->kind == XS_BOOLEAN_KIND) {
		fxToNumber(the, b);
		goto again;
	}
	else if (((a->kind == XS_INTEGER_KIND) || (a->kind == XS_NUMBER_KIND)) && mxIsReference(b)) {
		fxToPrimitive(the, b, XS_NUMBER_HINT);
		goto again;
	}
	else if ((a->kind == XS_STRING_KIND) && mxIsReference(b)) {
		fxToPrimitive(the, b, XS_STRING_HINT);
		goto again;
	}
	else if (((b->kind == XS_INTEGER_KIND) || (b->kind == XS_NUMBER_KIND)) && mxIsReference(a)) {
		fxToPrimitive(the, a, XS_NUMBER_HINT);
		goto again;
	}
	else if ((b->kind == XS_STRING_KIND) && mxIsReference(a)) {
		fxToPrimitive(the, a, XS_STRING_HINT);
		goto again;
	}
	if (testing)
		the->stack += 2;
	else {
		mxInitSlot(a, XS_BOOLEAN_KIND);
		a->value.boolean = result;
		the->stack++;
	}
	return result;
}

void fxRunForIn(txMachine* the)
{
	txSlot* aProperty;
	txSlot* anInstance;
	txSlot* aResult;

	aResult = the->stack;
	if (mxIsReference(aResult)) {
		anInstance = fxGetInstance(the, aResult);
		if (mxIsArray(anInstance)) {
			txInteger aLength;
			aProperty = anInstance->next;
			aLength = aProperty->value.array.length - 1;
			while (aLength >= 0) {
				if ((aProperty->value.array.address + aLength)->ID) {
					mxInitSlot(--the->stack, XS_INTEGER_KIND);
					the->stack->value.integer = aLength;
				}
				aLength--;
			}
		}
		if ((the->frame->flag & XS_SANDBOX_FLAG) || (anInstance->flag & XS_SANDBOX_FLAG)) {
			if (anInstance->flag & XS_SANDBOX_FLAG)
				anInstance = anInstance->value.instance.prototype;
			while (anInstance) {
				aProperty = anInstance->next;
				while (aProperty) {
					if (!(aProperty->flag & (XS_DONT_ENUM_FLAG | XS_DONT_SCRIPT_FLAG)))
						fxRunEnum(the, aProperty, aResult);
					aProperty = aProperty->next;
				}
				anInstance = fxGetParent(the, anInstance);
			}	
		}	
		else {
			while (anInstance) {
				aProperty = anInstance->next;
				while (aProperty) {
					if (!(aProperty->flag & (XS_DONT_ENUM_FLAG | XS_SANDBOX_FLAG)))
						fxRunEnum(the, aProperty, aResult);
					aProperty = aProperty->next;
				}
				anInstance = fxGetParent(the, anInstance);
			}
		}
	}
	mxInitSlot(aResult, XS_NULL_KIND);
}

txBoolean fxRunIn(txMachine* the, txBoolean testing)
{
	txSlot* anInstance;
	txSlot* aProperty;
	txID anID;
	txBoolean result; 
	
	if (!mxIsReference(the->stack))
		mxDebug0(the, XS_REFERENCE_ERROR, "in: no reference");
	anInstance = fxGetInstance(the, the->stack); 
	anID = XS_NO_ID;
	aProperty = fxGetPropertyAt(the, anInstance, the->stack + 1, &anID);
	the->stack++;
	result = (aProperty) ? 1 : 0;
	if (testing)
		the->stack++;
	else {
		mxInitSlot(the->stack, XS_BOOLEAN_KIND);
		the->stack->value.boolean = result;
	}
	return result;
}

txBoolean fxRunInstanceof(txMachine* the, txBoolean testing)
{
	txSlot* aFunction;
	txSlot* aPrototype;
	txSlot* anInstance;
	txBoolean result = 0; 
	
	if (!mxIsReference(the->stack))
		mxDebug0(the, XS_TYPE_ERROR, "instanceof: no reference");
	aFunction = fxGetInstance(the, the->stack);
	if (!mxIsFunction(aFunction))
		mxDebug0(the, XS_TYPE_ERROR, "instanceof: no constructor");
	aPrototype = aFunction->next->next->next;
	the->stack++;
	if (!mxIsReference(the->stack))
		anInstance = C_NULL;
	else {
		anInstance = fxGetInstance(the, the->stack);
		while (anInstance) {
			if (anInstance == aPrototype) {
				result = 1;
				break;
			}
			anInstance = fxGetParent(the, anInstance);
		}
	}
	if (testing)
		the->stack++;
	else {
		mxInitSlot(the->stack, XS_BOOLEAN_KIND);
		the->stack->value.boolean = result;
	}
	return result;
}

void fxRunSign(txMachine* the, txInteger theSign)
{	
	txSlot* aResult = the->stack;
	if (aResult->kind == XS_INTEGER_KIND) {
		if (theSign < 0)
			aResult->value.integer = -aResult->value.integer;
	} 
	else {
		fxToNumber(the, aResult);
		if (theSign < 0)
			aResult->value.number = -aResult->value.number;
	}
}

txBoolean fxRunStrictEqual(txMachine* the, txBoolean testing, txBoolean yes, txBoolean no)
{
	txBoolean result = no; 
	txSlot* b = the->stack;
	txSlot* a = the->stack + 1;
again:	
	if (a->kind == b->kind) {
		switch (a->kind) {
		case XS_UNDEFINED_KIND:
		case XS_NULL_KIND:
			result = yes;
			break;
		case XS_BOOLEAN_KIND:
			result = (a->value.boolean == b->value.boolean) ? yes : no;
			break;
		case XS_INTEGER_KIND:
			result = (a->value.integer == b->value.integer) ? yes : no;
			break;
		case XS_NUMBER_KIND:
			if (c_isnan(a->value.number))
				result = (c_isnan(b->value.number)) ? yes : no;
			else if (c_isnan(b->value.number))
				result = no;
			else
				result = (a->value.number == b->value.number) ? yes : no;
			break;
		case XS_STRING_KIND:
			result = (c_strcmp(a->value.string, b->value.string) == 0) ? yes : no;
			break;
		case XS_REFERENCE_KIND:
		case XS_ALIAS_KIND:
			result = (fxGetInstance(the, a) == fxGetInstance(the, b)) ? yes : no;
			break;
		}
	}
	else if ((a->kind == XS_INTEGER_KIND) && (b->kind == XS_NUMBER_KIND)) {
		fxToNumber(the, a);
		goto again;
	}
	else if ((a->kind == XS_NUMBER_KIND) && (b->kind == XS_INTEGER_KIND)) {
		fxToNumber(the, b);
		goto again;
	}
	if (testing)
		the->stack += 2;
	else {
		mxInitSlot(a, XS_BOOLEAN_KIND);
		a->value.boolean = result;
		the->stack++;
	}
	return result;
}

void fxRunSubtract(txMachine* the)
{	
	txSlot* aParameter = the->stack;
	txSlot* aResult = the->stack + 1;
	if ((aResult->kind == XS_INTEGER_KIND) && (aParameter->kind == XS_INTEGER_KIND)) {
		aResult->value.integer -= aParameter->value.integer;
	}
	else {
		fxToNumber(the, aResult);
		fxToNumber(the, aParameter);
		aResult->value.number -= aParameter->value.number;
	}
	the->stack++;
}

txBoolean fxRunTest(txMachine* the)
{	
	txBoolean result;
	
	switch (the->stack->kind) {
	case XS_UNDEFINED_KIND:
	case XS_NULL_KIND:
		result = 0;
		break;
	case XS_BOOLEAN_KIND:
		result = the->stack->value.boolean;
		break;
	case XS_INTEGER_KIND:
		result = (the->stack->value.integer == 0) ? 0 : 1;
		break;
	case XS_NUMBER_KIND:
		switch (c_fpclassify(the->stack->value.number)) {
		case FP_NAN:
		case FP_ZERO:
			result = 0;
			break;
		default:
			result = 1;
			break;
		}
		break;
	case XS_STRING_KIND:
		if (c_strlen(the->stack->value.string) == 0)
			result = 0;
		else
			result = 1;
		break;
	default:
		result = 1;
		break;
	}
	the->stack++;
	return result;
}

void fxRunTypeof(txMachine* the)
{
	txSlot aScratch;
	txSlot* anInstance;
	
	aScratch = *(the->stack);
	mxZeroSlot(the->stack);
	switch (aScratch.kind) {
	case XS_UNDEFINED_KIND:
		fxCopyStringC(the, the->stack, "undefined");
		break;
	case XS_NULL_KIND:
		fxCopyStringC(the, the->stack, "object");
		break;
	case XS_BOOLEAN_KIND:
		fxCopyStringC(the, the->stack, "boolean");
		break;
	case XS_INTEGER_KIND:
	case XS_NUMBER_KIND:
		fxCopyStringC(the, the->stack, "number");
		break;
	case XS_STRING_KIND:
		fxCopyStringC(the, the->stack, "string");
		break;
	case XS_REFERENCE_KIND:
	case XS_ALIAS_KIND:
		anInstance = fxGetInstance(the, &aScratch);
		if (mxIsFunction(anInstance))
			fxCopyStringC(the, the->stack, "function");
		else
			fxCopyStringC(the, the->stack, "object");
		break;
	}
}

txBoolean fxIsSameSlot(txMachine* the, txSlot* a, txSlot* b)
{	
	mxPush(*b);
	mxPush(*a);
	return fxRunStrictEqual(the, 1, 1, 0);
}

void fxRemapID(txMachine* the, txByte* theCode, txID* theIDs)
{
	txID anID;

	mxDecode2(theCode, anID);
	if (anID != XS_NO_ID) {
		anID &= 0x7FFF;
		anID = theIDs[anID];
		theCode -= 2; 
		mxEncode2(theCode, anID);
	}
}

void fxRemapIDs(txMachine* the, txByte* theCode, txID* theIDs)
{
	txInteger aCount;

	for (;;) {
		//if (((XS_LABEL <= *((txU1*)theCode)) && (*((txU1*)theCode) < XS_COUNT)))
		//	fprintf(stderr, "%s\n", gxCodeNames[*((txU1*)theCode)]);
		//else
		//	fprintf(stderr, "%d\n", *((txU1*)theCode));
		switch (*((txU1*)theCode++)) {
		case XS_LABEL:
			return;
		case XS_BEGIN:
			fxRemapID(the, theCode, theIDs);
			theCode += 3;
			aCount = *theCode++;
			while (aCount) {
				fxRemapID(the, theCode, theIDs);
				theCode += 2;
				aCount--;
			}
			aCount = *theCode++;
			while (aCount) {
				fxRemapID(the, theCode, theIDs);
				theCode += 2;
				aCount--;
			}
			break;
		case XS_DELETE_AT:
		case XS_GET_AT:
		case XS_INTEGER_8:
		case XS_SET_AT:
		case PSEUDO_INCREMENT:
		case PSEUDO_DECREMENT:
		case PSEUDO_BRANCH_ELSE0:
		case PSEUDO_BRANCH_ELSE1:
		case PSEUDO_DOUBLE_GET_NEGATIVE:
		case PSEUDO_DOUBLE_GET_AT:
		case XS_GET_NEGATIVE_ID:
		case PSEUDO_LESS:
		case PSEUDO_LESS_EQUAL:
		case XS_SET_NEGATIVE_ID:
			theCode++;
			break;
		case XS_CATCH:
		case XS_DELETE:
		case XS_DELETE_MEMBER:
		case XS_FILE:
		case XS_GET:
		case XS_GET_FOR_NEW:
		case XS_SET:
		case XS_SET_MEMBER:
		case XS_GET_MEMBER:
		case XS_GET_MEMBER_FOR_CALL:
			fxRemapID(the, theCode, theIDs);
			theCode += 2;
			break;
		case XS_PUT_MEMBER:
		case PSEUDO_PUT_MEMBER:
			fxRemapID(the, theCode, theIDs);
			theCode += 4;
			break;
		case XS_PUT_MEMBER_AT:
			theCode += 2;
			break;
		case XS_BRANCH:
		case XS_BRANCH_ELSE:
		case XS_BRANCH_ELSE_BOOL:
		case XS_BRANCH_IF:
		case XS_BRANCH_IF_BOOL:
		case XS_FUNCTION:
		case XS_INTEGER_16:
		case XS_LINE:
		case XS_ROUTE:
		case XS_STATUS:
			theCode += 2;
			break;
		case XS_BRANCH2:
		case XS_BRANCH_ELSE2:
		case XS_BRANCH_ELSE_BOOL2:
		case XS_BRANCH_IF2:
		case XS_BRANCH_IF_BOOL2:
		case XS_FUNCTION2:
		case XS_INTEGER_32:
		case XS_ROUTE2:
			theCode += 4;
			break;
		case XS_NUMBER:
			theCode += 8;
			break;
		case XS_STRING_POINTER:
		case XS_STRING_CONCAT:
		case XS_STRING_CONCATBY:
		case XS_STRING:
		{
			txID len;
#if mxStringLength
			mxDecode2(theCode,len);
#else
			len = c_strlen((txString)theCode) + 1;
#endif
			theCode += len;
		}break;
		case XS_ADD:
		case XS_ALIAS:
		case XS_BIT_AND:
		case XS_BIT_NOT:
		case XS_BIT_OR:
		case XS_BIT_XOR:
		case XS_BREAK:
		case XS_CALL:
		case XS_DELETE_MEMBER_AT:
		case XS_DEBUGGER:
		case XS_DECREMENT:
		case XS_DIVIDE:
		case XS_DUB:
		case XS_END:
		case XS_ENUM:
		case XS_EQUAL:
		case XS_FALSE:
		case XS_FLAG_INSTANCE:
		case XS_GET_MEMBER_AT:
		case XS_GLOBAL:
		case XS_IN:
		case XS_INCREMENT:
		case XS_INSTANCEOF:
		case XS_INSTANCIATE:
		case XS_JUMP:
		case XS_LEFT_SHIFT:
		case XS_LESS:
		case XS_LESS_EQUAL:
		case XS_MINUS:
		case XS_MODULO:
		case XS_MORE:
		case XS_MORE_EQUAL:
		case XS_MULTIPLY:
		case XS_NEW:
		case XS_NOT:
		case XS_NOT_EQUAL:
		case XS_NULL:
		case XS_PARAMETERS:
		case XS_PLUS:
		case XS_POP:
		case PSEUDO_DUB:
		case XS_RESULT:
		case XS_RETURN:
		case XS_SCOPE:
		case XS_SET_MEMBER_AT:
		case XS_SIGNED_RIGHT_SHIFT:
		case XS_STRICT_EQUAL:
		case XS_STRICT_NOT_EQUAL:
		case XS_SUBTRACT:
		case XS_SWAP:
		case XS_THIS:
		case PSEUDO_SET_MEMBER:
		case PSEUDO_GET_MEMBER:
		case XS_THROW:
		case XS_TRUE:
		case XS_TYPEOF:
		case XS_UNCATCH:
		case XS_UNDEFINED:
		case XS_UNSCOPE:
		case XS_UNSIGNED_RIGHT_SHIFT:
		case XS_VOID:
			break;
		case XS_ATTRIBUTE_PATTERN:
		case XS_DATA_PATTERN:
		case XS_PI_PATTERN:
		case XS_EMBED_PATTERN:
		case XS_JUMP_PATTERN:
		case XS_REFER_PATTERN:
		case XS_REPEAT_PATTERN:
			aCount = *theCode++;
			while (aCount > 0) {
				fxRemapID(the, theCode, theIDs);
				theCode += 2;
				fxRemapID(the, theCode, theIDs);
				theCode += 2;
				aCount--;
			}
			break;
		default:
			--theCode;
			theCode = (*gxRemapIDsHook)(the, theCode, theIDs);
			break;
		}
	}
}

#ifdef mxFrequency
static char* gxFrequencyNames[XS_COUNT] = {
	"", "", "", "",
	"", "", "", "",
	"", "", "", "",
	"", "", "", "",
	"", "", "", "",
	"", "", "", "",
	"", "", "", "",
	"", "", "", "",
	"", "",
	
	"XS_ADD",
	"XS_ALIAS",
	"XS_BEGIN",
	"XS_BIND",
	"XS_BIT_AND",
	"XS_BIT_NOT",
	"XS_BIT_OR",
	"XS_BIT_XOR",
	"XS_BRANCH",
	"XS_BRANCH_ELSE",
	"XS_BRANCH_IF",
	"XS_BREAK",
	"XS_CALL",
	"XS_CATCH",
	"XS_DEBUGGER",
	"XS_DECREMENT",
	"XS_DELETE",
	"XS_DELETE_AT",
	"XS_DELETE_MEMBER",
	"XS_DELETE_MEMBER_AT",
	"XS_DIVIDE",
	"XS_DUB",
	"XS_END",
	"XS_ENUM",
	"XS_EQUAL",
	"XS_FALSE",
	"XS_FILE",
	"XS_FUNCTION",
	"XS_GET",
	"XS_GET_AT",
	"XS_GET_MEMBER",
	"XS_GET_MEMBER_AT",
	"XS_GLOBAL",
	"XS_IN",
	"XS_INCREMENT",
	"XS_INSTANCEOF",
	"XS_INSTANCIATE",
	"XS_INTEGER_8",
	"XS_INTEGER_16",
	"XS_INTEGER_32",
	"XS_JUMP",
	"XS_LEFT_SHIFT",
	"XS_LESS",
	"XS_LESS_EQUAL",
	"XS_LINE",
	"XS_MINUS",
	"XS_MODULO",
	"XS_MORE",
	"XS_MORE_EQUAL",
	"XS_MULTIPLY",
	"XS_NEW",
	"XS_NOT",
	"XS_NOT_EQUAL",
	"XS_NULL", 
	"XS_NUMBER",
	"XS_PARAMETERS",
	"XS_PLUS",
	"XS_POP",
	"XS_PUT_MEMBER",
	"XS_PUT_MEMBER_AT",
	"XS_RESULT",
	"XS_RETURN",
	"XS_ROUTE",
	"XS_SCOPE",
	"XS_SET",
	"XS_SET_AT",
	"XS_SET_MEMBER",
	"XS_SET_MEMBER_AT",
	"XS_SIGNED_RIGHT_SHIFT",
	"XS_STATUS",
	"XS_STRICT_EQUAL",
	"XS_STRICT_NOT_EQUAL",
	"XS_STRING",
	"XS_SUBTRACT",
	"XS_SWAP",
	"XS_THIS",
	"XS_THROW",
	"XS_TRUE",
	"XS_TYPEOF",
	"XS_UNCATCH",
	"XS_UNDEFINED",
	"XS_UNSCOPE",
	"XS_UNSIGNED_RIGHT_SHIFT",
	"XS_VOID",
	
	"XS_ATTRIBUTE_PATTERN",
	"XS_DATA_PATTERN",
	"XS_PI_PATTERN",
	"XS_EMBED_PATTERN",
	"XS_JUMP_PATTERN",
	"XS_REFER_PATTERN",
	"XS_REPEAT_PATTERN",
	"XS_FLAG_INSTANCE",
	
	"XS_BRANCH2",
	"XS_BRANCH_ELSE2",
	"XS_BRANCH_IF2",
	"XS_FUNCTION2",
	"XS_ROUTE2"
};

typedef struct {
	int code;
	long count;
	long fastCount;
} txFrequency;

static int fxCompareFrequency(const void* a, const void* b)
{
	return ((txFrequency*)b)->count - ((txFrequency*)a)->count;
}

void fxReportFrequency(txMachine* the)
{
	txFrequency frequencies[XS_COUNT - XS_LABEL];
	int i;
	double aCount = 0;
	
	for (i = XS_LABEL; i < XS_COUNT; i++) {
		frequencies[i - XS_LABEL].code = i;
		frequencies[i - XS_LABEL].count = the->fastFrequencies[i] + the->fullFrequencies[i];
		frequencies[i - XS_LABEL].fastCount = the->fastFrequencies[i];
		aCount += the->fastFrequencies[i] + the->fullFrequencies[i];
	}
	c_qsort(frequencies, XS_COUNT - XS_LABEL, sizeof(txFrequency), fxCompareFrequency);
	for (i = 0; i < XS_COUNT - XS_LABEL; i++) {
		fprintf(stderr, "%24s %12ld %10.2f%% %12ld %10.2f%%\n", 
			gxFrequencyNames[frequencies[i].code], 
			frequencies[i].count, 
			((double)(frequencies[i].count) * 100.0) / aCount,
			frequencies[i].fastCount, 
			(frequencies[i].count) 
				? ((double)(frequencies[i].fastCount) * 100.0) / (double)(frequencies[i].count)
				: 0);
	}
}

#endif

