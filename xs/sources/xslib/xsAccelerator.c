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

//*(the_stack + 0) -> q0
//*(the_stack + 1) -> q1
//tmp_slot ->q2,q3
//value 1 slot ->d10
//value 0x1f slot->d9

//pseudo reg->d12,d13,d14,d15
/*pseudo*/

#define mxChunkResult(label) {\
	aBlock = the->firstBlock; \
	while (aBlock) { \
		if ((aBlock->current + theSize) <= aBlock->limit) { \
			aData = aBlock->current; \
			((txChunk*)aData)->size = theSize; \
			((txChunk*)aData)->temporary = C_NULL; \
			aBlock->current += theSize; \
			the->currentChunksSize += theSize; \
			if (the->peakChunksSize < the->currentChunksSize) \
				the->peakChunksSize = the->currentChunksSize; \
			result = (txString)(aData + sizeof(txChunk)); \
			goto label; \
		} \
		aBlock = aBlock->nextBlock; \
	} \
}

#define PSEUDO_PART_GET_NEGATIVE_ID2(aProperty0,aProperty1) {\
			txByte theIndex0 = *(the_code+1);\
			txByte theIndex1 = *(the_code+3);\
			aProperty0 = the->frame + theIndex0 - aOffset;\
			aProperty1 = the->frame + theIndex1 - aOffset;\
		}

#define PSEUDO_PART_GET_NEGATIVE_ID(aProperty) {\
			txByte theIndex = *(the_code+1);\
			aProperty = the->frame + theIndex - aOffset;\
	}

#define PSEUDO_PART_BRANCH_ELSE_C(aProperty) {\
			txKind kind; \
			txInteger results; \
			txInteger offset;\
			const txInteger kindList =	(1 << XS_UNDEFINED_KIND)\
									 |	(1 << XS_NULL_KIND)\
									 |	(1 << XS_BOOLEAN_KIND)\
									 |	(1 << XS_INTEGER_KIND)\
									 |	(1 << XS_REFERENCE_KIND);\
			kind = aProperty->kind;\
			if(*(txU1 *)the_code==XS_BRANCH_ELSE2) {\
				offset = 5;\
				mxImmediateS4(the_code, 1, anOffset);\
			}\
			else {\
				offset = 3;\
				mxImmediateS2(the_code, 1, anIndex);\
				anOffset = anIndex;\
			}\
			if ((((kindList) >> kind) & 1) != 0) {\
				results = (0 << XS_UNDEFINED_KIND) | (0 << XS_NULL_KIND) | (1 << XS_REFERENCE_KIND);\
				if (aProperty->value.integer != 0)\
					results |= (1 << XS_BOOLEAN_KIND) | (1 << XS_INTEGER_KIND);\
				if (((results >> kind) & 1) == 0) {\
					mxNextOpcode(the_code, anOffset+offset, opcode);\
				}\
				else {\
					mxNextOpcode(the_code, offset,opcode);\
				}\
			} \
			else if (kind == XS_STRING_KIND) {\
				if (*aProperty->value.string == 0) {\
					mxNextOpcode(the_code, anOffset+offset, opcode);\
				}\
				else {\
					mxNextOpcode(the_code, offset, opcode);\
				}\
			}\
			else\
			{\
				mxInitSlot(--the_stack, aProperty->kind);\
				the_stack->value = aProperty->value;\
				goto exit_accelerator;\
			}\
		}

#define PSEUDO_PART_BRANCH_ELSE_NEON(aProperty) {\
			txKind kind; \
			txInteger results; \
			txInteger offset;\
			const txInteger kindList =	(1 << XS_UNDEFINED_KIND)\
									 |	(1 << XS_NULL_KIND)\
									 |	(1 << XS_BOOLEAN_KIND)\
									 |	(1 << XS_INTEGER_KIND)\
									 |	(1 << XS_REFERENCE_KIND);\
			kind = aProperty->kind;\
			if(*(txU1 *)the_code==XS_BRANCH_ELSE2) {\
				offset = 5;\
				mxImmediateS4(the_code, 1, anOffset);\
			}\
			else {\
				offset = 3;\
				mxImmediateS2(the_code, 1, anIndex);\
				anOffset = anIndex;\
			}\
			if ((((kindList) >> kind) & 1) != 0) {\
				results = (0 << XS_UNDEFINED_KIND) | (0 << XS_NULL_KIND) | (1 << XS_REFERENCE_KIND);\
				if (aProperty->value.integer != 0)\
					results |= (1 << XS_BOOLEAN_KIND) | (1 << XS_INTEGER_KIND);\
				if (((results >> kind) & 1) == 0) {\
					mxNextOpcode(the_code, anOffset+offset, opcode);\
				}\
				else {\
					mxNextOpcode(the_code, offset,opcode);\
				}\
			} \
			else if (kind == XS_STRING_KIND) {\
				if (*aProperty->value.string == 0) {\
					mxNextOpcode(the_code, anOffset+offset, opcode);\
				}\
				else {\
					mxNextOpcode(the_code, offset, opcode);\
				}\
			}\
			else\
			{\
				__InitSlotValue(--the_stack,aProperty);\
				goto exit_accelerator;\
			}\
		}

#if SUPPORT_NEON
/*new*/
#define __FPU {\
	asm volatile(".fpu    neon");\
}

#define __InitSlot(slot,kind) { \
	asm volatile("vmov.i32  q2,#0 \t\n;\
				vmov.u8 d4[7],%0 \t\n;\
				vst1.64 {d4,d5},[%1]"::"r"(kind),"r"(slot));\
}

#define __InitSlotU8(slot,kind,value){\
	asm volatile("vmov.i32  q2,#0 \t\n;\
				vmov.u8 d4[7],%0 \t\n;\
				vmov.u8 d5[0],%1 \t\n;\
				vst1.64 {d4,d5},[%2]"::"r"(kind),"r"(value),"r"(slot));\
}

#define __InitSlotS8(slot,kind,value){\
	asm volatile("vmov.i32  d4,#0 \t\n;\
				vmov.u8 d4[7],%0 \t\n;\
				vmov.s32 d5[0],%1 \t\n;\
				vst1.64 {d4,d5},[%2]"::"r"(kind),"r"(value),"r"(slot));\
}


#define __InitSlotValue(slot,property) {\
	asm volatile("vmov.i32	d4,#0 \t\n;\
			  	  vmov.u8 d4[7],%0 \t\n;\
			  	  vld1.32 d5,[%1]  \t\n;\
			  	  vst1.64 {d4,d5},[%2]"::"r"((property)->kind),"r"(&((property)->value)),"r"(slot));\
}

#define __CopySlot(dst,src) {\
	asm volatile("vld1.64 {d4,d5},[%0] \t\n;\
			  	  vst1.64 {d4,d5},[%1] \t\n"::"r"(src),"r"(dst));\
}

#define __SwapSlot(slot0,slot1){\
	asm volatile("vld1.64 {d4,d5},[%0] \t\n;\
			  	  vld1.64 {d6,d7},[%1] \t\n;\
			  	  vst1.64 {d4,d5},[%1] \t\n;\
			  	  vst1.64 {d6,d7},[%0]"::"r"(slot0),"r"(slot1));\
}

#define __ZeroSlot(slot) {\
	asm volatile("vmov.i32  q2,#0 \t\n;\
				  vst1.64 {d4,d5},[%0]"::"r"(slot));\
}

#define __ZeroSlot3(slot){\
	asm volatile("vmov.i32 q2,#0 \t\n;\
				  vst1.64 {d4,d5},[%0]! \t\n;\
				  vst1.64 {d4,d5},[%0]! \t\n;\
				  vst1.64 {d4,d5},[%0]!  \t\n"::"r"(slot));\
}
#endif

#ifdef mxDebug
#define mxGetInstance { \
	mxImmediateS2(the_code, 1, anIndex); \
	if (the_stack->kind == XS_ALIAS_KIND) \
		resultInstance = the->aliasArray[the_stack->value.alias]; \
	else if (the_stack->kind == XS_REFERENCE_KIND) \
		resultInstance = the_stack->value.reference; \
	else \
		goto exit_accelerator; \
	if (resultInstance->kind != XS_INSTANCE_KIND) goto exit_accelerator; \
	if (resultInstance->flag & XS_SANDBOX_FLAG) {\
		resultInstance = resultInstance->value.instance.prototype;\
		resultFlag = XS_DONT_SCRIPT_FLAG;\
	}\
	else\
		resultFlag = frameFlag;\
}
#else
#define mxGetInstance { \
	mxImmediateS2(the_code, 1, anIndex); \
	if (the_stack->kind == XS_ALIAS_KIND) \
		resultInstance = the->aliasArray[the_stack->value.alias]; \
	else if (the_stack->kind == XS_REFERENCE_KIND) \
		resultInstance = the_stack->value.reference; \
	else \
		goto exit_accelerator; \
	if (resultInstance->flag & XS_SANDBOX_FLAG) {\
		resultInstance = resultInstance->value.instance.prototype;\
		resultFlag = XS_DONT_SCRIPT_FLAG;\
	}\
	else\
		resultFlag = frameFlag;\
}
#endif

#define mxNextOpcode(THE_CODE,OFFSET,OPCODE) { \
	THE_CODE += OFFSET; \
	OPCODE = *((txU1*)THE_CODE); \
}

#define mxImmediateS2(THE_CODE,OFFSET,S2) { \
	S2 = (((txS1*)THE_CODE)[OFFSET+0] << 8) | ((txU1*)THE_CODE)[OFFSET+1]; \
}

#define mxImmediateU2(THE_CODE,OFFSET,U2) { \
	U2 = (((txU1*)THE_CODE)[OFFSET+0] << 8) | ((txU1*)THE_CODE)[OFFSET+1]; \
}

#define mxImmediateS4(THE_CODE,OFFSET,S4) { \
	S4 = (((txS1*)THE_CODE)[OFFSET+0] << 24) | (((txU1*)THE_CODE)[OFFSET+1] << 16) | (((txU1*)THE_CODE)[OFFSET+2] << 8) | ((txU1*)THE_CODE)[OFFSET+3]; \
}

#define mxImmediateU4(THE_CODE,OFFSET,U4) { \
	U4 = (((txU1*)THE_CODE)[OFFSET+0] << 24) | (((txU1*)THE_CODE)[OFFSET+1] << 16) | (((txU1*)THE_CODE)[OFFSET+2] << 8) | ((txU1*)THE_CODE)[OFFSET+3]; \
}

#define mxGetSymbol(THE, THE_ID, RESULT)		\
{												\
	RESULT = C_NULL;							\
	if (THE_ID & 0x8000) {						\
		txID theID = THE_ID & 0x7FFF;		\
		if (theID < THE->symbolCount)			\
			RESULT = THE->symbolArray[theID];	\
	}											\
}

#if defined(__GNUC__) && defined(__OPTIMIZE__)
	#define mxCase(OPCODE) \
		OPCODE:

#if defined(mxFrequency)
	#define mxBreak \
		the->fastFrequencies[opcode]++; \
		goto *opcodes[opcode]
#elif defined(mxTrace)
	#define mxBreak \
		if (gxDoTrace) fxTraceAcceleratorCode(the, opcode, the_stack); \
		goto *opcodes[opcode]
#else
	#define mxBreak \
		goto *opcodes[opcode]
#endif

	#define mxSwitch(opcode) mxBreak;

	#define goto_decode_opcode \
		goto *opcodes[opcode]
	
#else
	#define mxCase(OPCODE) \
		case OPCODE:

	#define mxBreak \
		break

	#define mxSwitch(opcode) switch(opcode)

	#define goto_decode_opcode goto decode_opcode;
#endif

txID fxRunLoopAccelerator(txMachine* the, txSlot** theRoute, txJump* theJump,txSlot **resProperty);
#if SUPPORT_NEON
txID fxRunLoopAccelerator_arm_v7(txMachine* the, txSlot** theRoute, txJump* theJump,txSlot **resProperty);

txID fxRunLoopAccelerator_arm_v7(txMachine* the, txSlot** theRoute, txJump* theJump,txSlot **resProperty)
{
	register txByte*	the_code;
	register txSlot*	the_stack;
	register txU1		opcode;
	register txID		anIndex = XS_NO_ID; /* is not used uninitialized! */
	register txSize		anOffset; /* is not used uninitialized! */
	register txSlot*	resultInstance;
	register txSlot*	resultProperty;
	register txSlot*	resultScope;
	register txFlag		resultFlag;
	txByte aCount = (txByte)mxArgc;
	txByte aLength = *(mxFunction->value.reference->next->value.code + 4);
	register txByte	aOffset = (aLength <= aCount)? 4:(aLength - aCount) + 4;
	register txFlag frameFlag = (the->frame->flag & XS_SANDBOX_FLAG)?XS_DONT_SCRIPT_FLAG:XS_SANDBOX_FLAG;

#if defined(__GNUC__) && defined(__OPTIMIZE__)
	static void *armOpcodes[] = {
		&&XS_UNDEFINED_KIND,
		&&XS_NULL_KIND,
		&&XS_BOOLEAN_KIND,
		&&XS_INTEGER_KIND,
		&&XS_NUMBER_KIND,
		&&XS_REFERENCE_KIND, /* 5 */
		&&XS_REGEXP_KIND,
		&&XS_STRING_KIND,
		&&XS_DATE_KIND,
		
		&&XS_ARRAY_KIND,
		&&XS_CALLBACK_KIND, /* 10 */
		&&XS_CODE_KIND,
		&&XS_HOST_KIND,

		&&XS_SYMBOL_KIND,
		&&XS_INSTANCE_KIND,

		&&XS_FRAME_KIND, /* 15 */
		&&XS_ROUTE_KIND,
		
		&&XS_LIST_KIND,
		&&XS_ACCESSOR_KIND,
		
		&&XS_ALIAS_KIND,
		&&XS_GLOBAL_KIND, /* 20 */
		
		&&XS_NODE_KIND,
		&&XS_PREFIX_KIND,
		&&XS_ATTRIBUTE_RULE,
		&&XS_DATA_RULE,
		&&XS_PI_RULE, /* 25 */
		&&XS_EMBED_RULE,
		&&XS_JUMP_RULE,
		&&XS_REFER_RULE,
		&&XS_REPEAT_RULE,
		&&XS_ERROR_RULE,

		&&XS_LOCAL,
		&&XS_LOCALS,
		&&XS_LABEL,
		
		&&XS_ADD,
		&&XS_ALIAS,
		&&XS_BEGIN,
		&&XS_BIND,
		&&XS_BIT_AND,
		&&XS_BIT_NOT,
		&&XS_BIT_OR,
		&&XS_BIT_XOR,
		&&XS_BRANCH,
		&&XS_BRANCH_ELSE,
		&&XS_BRANCH_IF,
		&&XS_BREAK,
		&&XS_CALL,
		&&XS_CATCH,
		&&XS_DEBUGGER,
		&&XS_DECREMENT,
		&&XS_DELETE,
		&&XS_DELETE_AT,
		&&XS_DELETE_MEMBER,
		&&XS_DELETE_MEMBER_AT,
		&&XS_DIVIDE,
		&&XS_DUB,
		&&XS_END,
		&&XS_ENUM,
		&&XS_EQUAL,
		&&XS_FALSE,
		&&XS_FILE,
		&&XS_FUNCTION,
		&&XS_GET,
		&&XS_GET_AT,
		&&XS_GET_MEMBER,
		&&XS_GET_MEMBER_AT,
		&&XS_GLOBAL,
		&&XS_IN,
		&&XS_INCREMENT,
		&&XS_INSTANCEOF,
		&&XS_INSTANCIATE,
		&&XS_INTEGER_8,
		&&XS_INTEGER_16,
		&&XS_INTEGER_32,
		&&XS_JUMP,
		&&XS_LEFT_SHIFT,
		&&XS_LESS,
		&&XS_LESS_EQUAL,
		&&XS_LINE,
		&&XS_MINUS,
		&&XS_MODULO,
		&&XS_MORE,
		&&XS_MORE_EQUAL,
		&&XS_MULTIPLY,
		&&XS_NEW,
		&&XS_NOT,
		&&XS_NOT_EQUAL,
		&&XS_NULL, 
		&&XS_NUMBER,
		&&XS_PARAMETERS,
		&&XS_PLUS,
		&&XS_POP,
		&&XS_PUT_MEMBER,
		&&XS_PUT_MEMBER_AT,
		&&XS_RESULT,
		&&XS_RETURN,
		&&XS_ROUTE,
		&&XS_SCOPE,
		&&XS_SET,
		&&XS_SET_AT,
		&&XS_SET_MEMBER,
		&&XS_SET_MEMBER_AT,
		&&XS_SIGNED_RIGHT_SHIFT,
		&&XS_STATUS,
		&&XS_STRICT_EQUAL,
		&&XS_STRICT_NOT_EQUAL,
		&&XS_STRING,
		&&XS_SUBTRACT,
		&&XS_SWAP,
		&&XS_THIS,
		&&XS_THROW,
		&&XS_TRUE,
		&&XS_TYPEOF,
		&&XS_UNCATCH,
		&&XS_UNDEFINED,
		&&XS_UNSCOPE,
		&&XS_UNSIGNED_RIGHT_SHIFT,
		&&XS_VOID,
		
		&&XS_ATTRIBUTE_PATTERN,
		&&XS_DATA_PATTERN,
		&&XS_PI_PATTERN,
		&&XS_EMBED_PATTERN,
		&&XS_JUMP_PATTERN,
		&&XS_REFER_PATTERN,
		&&XS_REPEAT_PATTERN,
		&&XS_FLAG_INSTANCE,
		
		&&XS_BRANCH2,
		&&XS_BRANCH_ELSE2,
		&&XS_BRANCH_IF2,
		&&XS_FUNCTION2,
		&&XS_ROUTE2,
		&&XS_GET_NEGATIVE_ID,
		&&XS_SET_NEGATIVE_ID,
		&&XS_BRANCH_ELSE_BOOL,
		&&XS_BRANCH_IF_BOOL,
		&&XS_BRANCH_ELSE_BOOL2,
		&&XS_BRANCH_IF_BOOL2,
		&&XS_STRING_POINTER,
		&&XS_STRING_CONCAT,
		&&XS_STRING_CONCATBY,
		&&PSEUDO_BRANCH_ELSE0,
		&&PSEUDO_BRANCH_ELSE1,
		&&PSEUDO_GET_MEMBER,
		&&PSEUDO_SET_MEMBER,
		&&PSEUDO_LESS,
		&&PSEUDO_LESS_EQUAL,
		&&PSEUDO_INCREMENT,
		&&PSEUDO_DECREMENT,
		&&PSEUDO_DUB,
		&&PSEUDO_DOUBLE_GET_NEGATIVE,
		&&PSEUDO_DOUBLE_GET_AT,
		&&PSEUDO_PUT_MEMBER,
		&&XS_GET_MEMBER_FOR_CALL,
		&&XS_GET_FOR_NEW,
	};
	register void **opcodes = armOpcodes;
#endif

	*resProperty = resultProperty = C_NULL;
	resultScope = C_NULL;
	the_code = the->code;
	the_stack = the->stack;
	mxNextOpcode(the_code, 0, opcode);
	
	__FPU;
	for (;;) {
#if !defined(__GNUC__) || ( defined(__GNUC__) && !defined(__OPTIMIZE__) )
		decode_opcode:
#ifdef mxFrequency
		the->fastFrequencies[opcode]++;
#endif
#ifdef mxTrace
		if (gxDoTrace) {
			fxTraceAcceleratorCode(the, opcode, the_stack);
		}
#endif
#endif
			
		mxSwitch (opcode) {
		accessor_kind_process: {
			txSlot* aFunction;
			__InitSlot(--the_stack, XS_INTEGER_KIND);
			/* THIS */
			__SwapSlot(the_stack,the_stack+1);
			/* FUNCTION */
			aFunction = resultProperty->value.accessor.getter;
			if (!mxIsFunction(aFunction))
				mxDebugID(the, XS_TYPE_ERROR, "get %s: no getter", anIndex);
			__InitSlot(--the_stack, XS_REFERENCE_KIND);
			the_stack->value.reference = aFunction;
			/* RESULT */
			__ZeroSlot(--the_stack);
			/* FRAME */
			__InitSlot(--the_stack, XS_FRAME_KIND);
			the_stack->next = the->frame;
			the_stack->ID = anIndex;
			the_stack->flag = XS_NO_FLAG;
#ifdef mxDebug
			if (the->frame && (the->frame->flag & XS_STEP_INTO_FLAG))
				the_stack->flag |= XS_STEP_INTO_FLAG | XS_STEP_OVER_FLAG;
#endif
			the_stack->value.frame.code = the_code;
			the_stack->value.frame.scope = the->scope;
			if (aFunction->next->kind == XS_CALLBACK_KIND) {
				the_stack->flag |= XS_C_FLAG;
				the->frame = the_stack;
				the->scope = C_NULL;
				the->code = aFunction->next->next->value.closure.symbolMap;
				__InitSlot(--the_stack, XS_INTEGER_KIND);
				the->stack = the_stack;
#ifdef mxProfile
				fxDoCallback(the, aFunction->next->value.callback.address);
#else
				(*(aFunction->next->value.callback.address))(the);
#endif
				the_stack = mxArgv(-1);
				the->scope = the->frame->value.frame.scope;
				the_code = the->frame->value.frame.code;
				*(--the_stack) = *mxResult;
				the->frame = the->frame->next;
			}
			else {
				txByte aLength;
				the->frame = the_stack;
				the_code = aFunction->next->value.code;
				aCount = (txByte)mxArgc;
				aLength = *(mxFunction->value.reference->next->value.code + 4);
				aOffset = (aLength <= aCount)? 4:(aLength - aCount) + 4;
				frameFlag = (the->frame->flag & XS_SANDBOX_FLAG)?XS_DONT_SCRIPT_FLAG:XS_SANDBOX_FLAG;
				mxNextOpcode(the_code, 0, opcode);
			}
			resultScope = C_NULL;
			*resProperty = resultProperty = C_NULL;
			mxBreak;
		}

		getProperty: {
			resultProperty = resultInstance->next;
			if (resultInstance->flag & XS_VALUE_FLAG) {
				if (anIndex < 0) {
					if (resultProperty->kind == XS_GLOBAL_KIND) {
					#ifdef mxDebug
						if ((anIndex & 0x7FFF) >= the->symbolCount)
							goto exit_accelerator;
					#endif
						resultProperty = (resultFlag & XS_SANDBOX_FLAG)?resultProperty->value.global.cache[anIndex & 0x7FFF]:resultProperty->value.global.sandboxCache[anIndex & 0x7FFF];
						if (!resultProperty) 
							goto exit_accelerator;
						goto_decode_opcode;
					}
				}
				else {
					if (resultProperty->kind == XS_ARRAY_KIND) {
						if ((txIndex)anIndex >= resultProperty->value.array.length) 
							goto exit_accelerator;
						resultProperty = resultProperty->value.array.address + anIndex;
						if (!resultProperty->ID)
							goto exit_accelerator;
						goto_decode_opcode;
					}
				}
			}
			while (resultProperty) {
				if (resultProperty->ID == anIndex) {
					if (!(resultProperty->flag & resultFlag)) {
						goto_decode_opcode;
					}
				}
				resultProperty = resultProperty->next;
			}
			if (resultInstance->ID >= 0) {
				if ((resultInstance = the->aliasArray[resultInstance->ID])) {
					goto getProperty;
				}
			}
			else if ((resultInstance = resultInstance->value.instance.prototype)) {
				goto getProperty;
			}
			if (resultScope) {
				resultScope = resultScope->next;
				if (resultScope) {
					if (resultScope->kind == XS_ALIAS_KIND)
						resultInstance = the->aliasArray[resultScope->value.alias];
					else if (resultScope->kind == XS_REFERENCE_KIND) 
						resultInstance = resultScope->value.reference;
					else
						goto exit_accelerator;
				#ifdef mxDebug
					if (resultInstance->kind != XS_INSTANCE_KIND)
						goto exit_accelerator;
				#endif
					if (resultInstance->flag & XS_SANDBOX_FLAG) {
						resultInstance = resultInstance->value.instance.prototype;
						resultFlag = XS_DONT_SCRIPT_FLAG;
					}	
					else
						resultFlag = frameFlag;
					goto getProperty;
				}
			}
			goto exit_accelerator;
		}
			
		putProperty: {
			resultProperty = resultInstance->next;
			if (resultInstance->flag & XS_VALUE_FLAG) {
				if (anIndex < 0) {
					if (resultProperty->kind == XS_GLOBAL_KIND) {
					#ifdef mxDebug
						if ((anIndex & 0x7FFF) >= the->symbolCount)
							goto exit_accelerator;
					#endif
						resultProperty = (resultFlag & XS_SANDBOX_FLAG)?resultProperty->value.global.cache[anIndex & 0x7FFF]:resultProperty->value.global.sandboxCache[anIndex & 0x7FFF];
						if (!resultProperty)
							goto exit_accelerator;
						goto_decode_opcode;
					}
				}
				else {
					if (resultProperty->kind == XS_ARRAY_KIND) {
						if ((txIndex)anIndex >= resultProperty->value.array.length) 
							goto exit_accelerator;
						resultProperty = resultProperty->value.array.address + anIndex;
						resultProperty->ID = XS_NO_ID;
						goto_decode_opcode;
					}
				}
			}
			while (resultProperty) {
				if (resultProperty->ID == anIndex) {
					if (!(resultProperty->flag & resultFlag)) {
						goto_decode_opcode;
					}
				}
				resultProperty = resultProperty->next;
			}
			if (resultScope) {
				resultScope = resultScope->next;
				if (resultScope) {
					if (resultScope->kind == XS_ALIAS_KIND)
						resultInstance = the->aliasArray[resultScope->value.alias];
					else if (resultScope->kind == XS_REFERENCE_KIND) 
						resultInstance = resultScope->value.reference;
					else
						goto exit_accelerator;
				#ifdef mxDebug
					if (resultInstance->kind != XS_INSTANCE_KIND)
						goto exit_accelerator;
				#endif
					if (resultInstance->flag & XS_SANDBOX_FLAG) {
						resultInstance = resultInstance->value.instance.prototype;
						resultFlag = XS_DONT_SCRIPT_FLAG;
					}	
					else
						resultFlag = frameFlag;
					goto putProperty;
				}
			}
			goto exit_accelerator;
		}

		mxCase(XS_GET_FOR_NEW)
			if (!resultScope) {
				mxImmediateS2(the_code, 1, anIndex);
				resultScope = the->scope;
				if (resultScope->kind == XS_ALIAS_KIND)
					resultInstance = the->aliasArray[resultScope->value.alias];
				else if (resultScope->kind == XS_REFERENCE_KIND) 
					resultInstance = resultScope->value.reference;
				else
					goto exit_accelerator;
			#ifdef mxDebug
				if (resultInstance->kind != XS_INSTANCE_KIND)
					goto exit_accelerator;
			#endif
				if (resultInstance->flag & XS_SANDBOX_FLAG) {
					resultInstance = resultInstance->value.instance.prototype;
					resultFlag = XS_DONT_SCRIPT_FLAG;
				}	
				else
					resultFlag = frameFlag;
				goto getProperty;
			}
#ifdef mxTrace
			if (gxDoTrace)
				fxTraceSymbol(the, anIndex);
#endif
			if (resultScope->flag & XS_THIS_FLAG) {
				__InitSlotValue(--the_stack, resultScope);
			}
			else
				__ZeroSlot(--the_stack);
			the_stack->ID = anIndex;
			__ZeroSlot(--the_stack);
			mxNextOpcode(the_code, 3, opcode);
			if (resultProperty->kind == XS_ACCESSOR_KIND) {
				goto accessor_kind_process;
			}
			else {
				__InitSlotValue(the_stack,resultProperty);
				resultScope = C_NULL;
				*resProperty = resultProperty = C_NULL;
				mxBreak;
			}
		
		mxCase(XS_GET)
			if (!resultScope) {
				mxImmediateS2(the_code, 1, anIndex);
				resultScope = the->scope;
				if (resultScope->kind == XS_ALIAS_KIND)
					resultInstance = the->aliasArray[resultScope->value.alias];
				else if (resultScope->kind == XS_REFERENCE_KIND) 
					resultInstance = resultScope->value.reference;
				else
					goto exit_accelerator;
			#ifdef mxDebug
				if (resultInstance->kind != XS_INSTANCE_KIND)
					goto exit_accelerator;
			#endif
				if (resultInstance->flag & XS_SANDBOX_FLAG) {
					resultInstance = resultInstance->value.instance.prototype;
					resultFlag = XS_DONT_SCRIPT_FLAG;
				}	
				else
					resultFlag = frameFlag;
				goto getProperty;
			}
#ifdef mxTrace
			if (gxDoTrace)
				fxTraceSymbol(the, anIndex);
#endif
			__ZeroSlot(--the_stack);
			mxNextOpcode(the_code, 3, opcode);
			if (resultProperty->kind == XS_ACCESSOR_KIND) {
				goto accessor_kind_process;
			}
			else {
				__InitSlotValue(the_stack,resultProperty);
				resultScope = C_NULL;
				*resProperty = resultProperty = C_NULL;
				mxBreak;
			}
		
		mxCase(XS_SET)
			if (!resultScope) {
				mxImmediateS2(the_code, 1, anIndex);
				resultScope = the->scope;
				if (resultScope->kind == XS_ALIAS_KIND)
					resultInstance = the->aliasArray[resultScope->value.alias];
				else if (resultScope->kind == XS_REFERENCE_KIND) 
					resultInstance = resultScope->value.reference;
				else
					goto exit_accelerator;
			#ifdef mxDebug
				if (resultInstance->kind != XS_INSTANCE_KIND)
					goto exit_accelerator;
			#endif
				if (resultInstance->flag & XS_SHARED_FLAG) 
					goto exit_accelerator;
				if (resultInstance->flag & XS_SANDBOX_FLAG) {
					resultInstance = resultInstance->value.instance.prototype;
					resultFlag = XS_DONT_SCRIPT_FLAG;
				}	
				else
					resultFlag = frameFlag;
				goto putProperty;
			}
			if (resultProperty->kind == XS_ACCESSOR_KIND) {
				*resProperty = resultProperty;
				goto exit_accelerator;
			}
			if (resultProperty->flag & XS_DONT_SET_FLAG) goto exit_accelerator;
#ifdef mxTrace
			if (gxDoTrace)
				fxTraceSymbol(the, anIndex);
#endif
			resultProperty->kind = the_stack->kind;
			resultProperty->value = the_stack->value;
			resultScope = C_NULL;
			*resProperty = resultProperty = C_NULL;
			mxNextOpcode(the_code, 3, opcode);
			mxBreak;

		mxCase(XS_GET_MEMBER_FOR_CALL)
			if (!resultProperty) {
				mxGetInstance;
				goto getProperty;
			}
#ifdef mxTrace
			if (gxDoTrace)
				fxTraceSymbol(the, anIndex);
#endif
			mxNextOpcode(the_code, 3, opcode);

			the_stack->ID = anIndex;
			--the_stack;
			if (resultProperty->kind == XS_ACCESSOR_KIND) {
				goto accessor_kind_process;
			}
			else {
				__InitSlotValue(the_stack,resultProperty);
				*resProperty = resultProperty = C_NULL;
				mxBreak;
			}

		mxCase(XS_GET_MEMBER)
			if (!resultProperty) {
				mxGetInstance;
				goto getProperty;
			}
#ifdef mxTrace
			if (gxDoTrace)
				fxTraceSymbol(the, anIndex);
#endif
			mxNextOpcode(the_code, 3, opcode);
			if (resultProperty->kind == XS_ACCESSOR_KIND) {
				goto accessor_kind_process;
			}
			else {
				__InitSlotValue(the_stack,resultProperty);
				*resProperty = resultProperty = C_NULL;
				mxBreak;
			}

		mxCase(XS_SET_MEMBER)
			if (!resultProperty) {
				mxImmediateS2(the_code, 1, anIndex);
	
				if (the_stack->kind == XS_ALIAS_KIND)
					resultInstance = the->aliasArray[the_stack->value.alias];
				else if (the_stack->kind == XS_REFERENCE_KIND) 
					resultInstance = the_stack->value.reference;
				else
				{
					goto exit_accelerator;
				}
			#ifdef mxDebug
				if (resultInstance->kind != XS_INSTANCE_KIND)
					goto exit_accelerator;
			#endif
				if (resultInstance->flag & XS_SHARED_FLAG) 
				{
					goto exit_accelerator;
				}
				if(resultInstance->flag & XS_SANDBOX_FLAG) {
					resultInstance = resultInstance->value.instance.prototype;
					resultFlag = XS_DONT_SCRIPT_FLAG;
				}
				else
					resultFlag = frameFlag;
					goto putProperty;
			}
			if (resultProperty->kind == XS_ACCESSOR_KIND) {
				*resProperty = resultProperty;
				goto exit_accelerator;
			}
			if (resultProperty->flag & XS_DONT_SET_FLAG) goto exit_accelerator;
#ifdef mxTrace
			if (gxDoTrace)
				fxTraceSymbol(the, anIndex);
#endif
			the_stack++;
			resultProperty->kind = the_stack->kind;
			resultProperty->value = the_stack->value;
			*resProperty = resultProperty = C_NULL;
			mxNextOpcode(the_code, 3, opcode);
			mxBreak;

		mxCase(XS_BEGIN) {
			txFlag aFlag;
			txSlot* aScope;
			txInteger aParameterCount;
			txInteger aLocalCount;
			txSlot* aParameter;
			txSlot* aProperty;
			txSlot*	anInstance;

			mxImmediateS2(the_code, 1, anIndex);
			aFlag = *(the_code + 3);
			if (aFlag & (XS_EVAL_FLAG | XS_PROGRAM_FLAG | XS_THIS_FLAG))
				goto exit_accelerator;
			the_code += 4;
#ifdef mxProfile
			fxBeginFunction(the);
#endif
			aProperty = the_stack + 2;	
			aProperty->ID = anIndex;
			aProperty->flag = XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG;
			aProperty->next = C_NULL;

			__InitSlot(--the_stack, XS_ROUTE_KIND);
			the_stack->ID = XS_NO_ID;
			*theRoute = the_stack;

			the->frame->flag |= aFlag & (XS_SANDBOX_FLAG | XS_STRICT_FLAG);
			the_stack -=3;
			__ZeroSlot3(the_stack);
			the_stack -=3;
					
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
			aParameter = the->frame + 4;
			aParameter->flag = XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG;
			aParameterCount = (txID)aParameter->value.integer;
			aParameter += aParameterCount;
			
			aLocalCount = *the_code++;
			if (aParameterCount <= aLocalCount) {
				aLocalCount -= aParameterCount;
				while (aParameterCount) {
					aProperty->next = aParameter;
					aProperty = aProperty->next;
					mxImmediateU2(the_code, 0, aProperty->ID);
					the_code += 2;
					aProperty->flag = XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG;
#ifdef mxTrace
					if (gxDoTrace)
						fxTraceSymbol(the, aProperty->ID);
#endif
					aParameterCount--;
					aParameter--;
				}
				while (aLocalCount) {
					__ZeroSlot(--the_stack);
					aProperty->next = the_stack;
					aProperty = aProperty->next;
					mxImmediateU2(the_code, 0, aProperty->ID);
					the_code += 2;
					aProperty->flag = XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG;
#ifdef mxTrace
					if (gxDoTrace)
						fxTraceSymbol(the, aProperty->ID);
#endif
					aLocalCount--;
				}
			} else {
				aParameterCount -= aLocalCount;
				while (aLocalCount) {
					aProperty->next = aParameter;
					aProperty = aProperty->next;
					mxImmediateU2(the_code, 0, aProperty->ID);
					the_code += 2;
					aProperty->flag = XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG;
#ifdef mxTrace
					if (gxDoTrace)
						fxTraceSymbol(the, aProperty->ID);
#endif
					aLocalCount--;
					aParameter--;
				}
				while (aParameterCount) {
					aParameter->ID = XS_NO_ID;
					aParameter->flag = XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG;
					aParameterCount--;
					aParameter--;
				}
			}
			aLocalCount = *the_code++;
			while (aLocalCount) {
				__ZeroSlot(--the_stack);
				aProperty->next = the_stack;
				aProperty = aProperty->next;
				mxImmediateU2(the_code, 0, aProperty->ID);
				the_code += 2;
				aProperty->flag = XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG;
#ifdef mxTrace
				if (gxDoTrace)
					fxTraceSymbol(the, aProperty->ID);
#endif
				aLocalCount--;
			}
			
			aProperty = mxFunction;
			anInstance = the->frame - 3;
			__InitSlot(anInstance, XS_INSTANCE_KIND);
			anInstance->ID = XS_NO_ID;
			anInstance->flag = XS_STACK_FLAG;
			if (aProperty->ID != XS_NO_ID)
				anInstance->next = aProperty;
			else
				anInstance->next = aProperty->next;
			aProperty = the->frame - 2;
			__InitSlot(aProperty, XS_REFERENCE_KIND);
			aProperty->next = aScope;
			aProperty->ID = XS_NO_ID;
			aProperty->value.reference = anInstance;
			the->scope = aProperty;
			txByte aLength;
			aCount = (txByte)mxArgc;
			aLength = *(mxFunction->value.reference->next->value.code + 4);
			aOffset = (aLength <= aCount)? 4:(aLength - aCount) + 4;
			frameFlag = (the->frame->flag & XS_SANDBOX_FLAG)?XS_DONT_SCRIPT_FLAG:XS_SANDBOX_FLAG;
			mxNextOpcode(the_code, 0, opcode);
			mxBreak;		
		}

		mxCase(XS_ROUTE) {
			txSlot* newRoute = mxRoute;

			if (theJump) {
				theJump->frame = the->frame;
				theJump->stack = the_stack;
				theJump->scope = the->scope;
			}
			*theRoute = newRoute;
			mxImmediateS2(the_code, 1, anIndex);
			mxNextOpcode(the_code, 3, opcode);
			newRoute->value.route.code = the_code + anIndex;
			newRoute->value.route.stack = the_stack;
			mxBreak;
		}

		mxCase(XS_ROUTE2) {
			txSlot* newRoute = mxRoute;

			if (theJump) {
				theJump->frame = the->frame;
				theJump->stack = the_stack;
				theJump->scope = the->scope;
			}
			*theRoute = newRoute;
			mxImmediateS4(the_code, 1, anOffset);
			mxNextOpcode(the_code, 5, opcode);
			newRoute->value.route.code = the_code + anOffset;
			newRoute->value.route.stack = the_stack;
			mxBreak;
		}
		
		mxCase(XS_END) {
			txSlot* aProperty;
			txSlot*	anInstance;
			
			if (((the->frame - 2)->kind == XS_REFERENCE_KIND) && !((the->frame - 2)->value.reference->flag & XS_STACK_FLAG))
				goto exit_accelerator;
			if (!the->frame->next || (the->frame->next->flag & XS_C_FLAG))
				goto exit_accelerator;

			aProperty = mxParameters;
			if (aProperty->kind != XS_UNDEFINED_KIND) {
				anIndex = (txID)((the->frame + 4)->value.integer);
				aProperty = aProperty->value.reference->next;
				while (aProperty) {
					if ((aProperty->kind == XS_ACCESSOR_KIND) && (0 <= aProperty->ID) && (aProperty->ID < anIndex)) {
						anInstance = the->frame + 4 + anIndex - aProperty->ID;
						aProperty->kind = anInstance->kind;
						aProperty->value = anInstance->value;
					}
					aProperty = aProperty->next;
				}
			}

#ifdef mxProfile
			fxEndFunction(the);
#endif
			if (the->frame->flag & XS_THROW_FLAG) {
				the->scope = the->frame->value.frame.scope;
				the->frame = the->frame->next;
				*theRoute = mxRoute;
				the->frame->flag |= XS_THROW_FLAG;
				the_code = (*theRoute)->value.route.code;
				the_stack = (*theRoute)->value.route.stack;
			}
			else {
				the_stack = mxArgv(-1);
				the->scope = the->frame->value.frame.scope;
				the_code = the->frame->value.frame.code;
				__CopySlot(--the_stack,mxResult);
				the->frame = the->frame->next;
				*theRoute = mxRoute;
				if (theJump) {
					theJump->stack = the_stack;
					theJump->scope = the->scope;
					theJump->frame = the->frame;
				}
			}
			txByte aLength;
			aCount = (txByte)mxArgc;
			aLength = *(mxFunction->value.reference->next->value.code + 4);
			aOffset = (aLength <= aCount)? 4:(aLength - aCount) + 4;
			frameFlag = (the->frame->flag & XS_SANDBOX_FLAG)?XS_DONT_SCRIPT_FLAG:XS_SANDBOX_FLAG;
			mxNextOpcode(the_code, 0, opcode);
			mxBreak;
		}

		mxCase(XS_BRANCH)
			mxImmediateS2(the_code, 1, anIndex);
			the_code += 3;
			mxNextOpcode(the_code, anIndex, opcode);
			mxBreak;

		mxCase(XS_BRANCH2)
			mxImmediateS4(the_code, 1, anOffset);
			the_code += 5;
			mxNextOpcode(the_code, anOffset, opcode);
			mxBreak;

		mxCase(XS_BRANCH_ELSE_BOOL)
		{
			mxImmediateS2(the_code, 1, anIndex);
			mxNextOpcode(the_code, 3, opcode);
			if(the_stack->value.integer==0)
				mxNextOpcode(the_code, anIndex, opcode);
			the_stack++;
			mxBreak;
		}

		mxCase(XS_BRANCH_ELSE) 
		{
			txInteger results; 
			txInteger kind; 
			const txInteger kindList =	(1 << XS_UNDEFINED_KIND)
									 |	(1 << XS_NULL_KIND)
									 |	(1 << XS_BOOLEAN_KIND)
									 |	(1 << XS_INTEGER_KIND)
									 |	(1 << XS_REFERENCE_KIND); 

			kind = the_stack->kind;
			mxImmediateS2(the_code, 1, anIndex);
			if ((((kindList) >> kind) & 1) != 0) {
				mxNextOpcode(the_code, 3, opcode);
				results = (0 << XS_UNDEFINED_KIND) | (0 << XS_NULL_KIND) | (1 << XS_REFERENCE_KIND);
				if (the_stack->value.integer != 0)
					results |= (1 << XS_BOOLEAN_KIND) | (1 << XS_INTEGER_KIND);
				the_stack++;
				if (((results >> kind) & 1) == 0)
					mxNextOpcode(the_code, anIndex, opcode);
				mxBreak;
			} 
			else if (kind == XS_STRING_KIND) {
				mxNextOpcode(the_code, 3, opcode);
				if (*the_stack->value.string == 0)
					mxNextOpcode(the_code, anIndex, opcode);
				the_stack++;
				mxBreak;
			} 
			else
				goto exit_accelerator;
		}

		mxCase(XS_BRANCH_ELSE_BOOL2) 
		{
			mxImmediateS4(the_code, 1, anOffset);
			mxNextOpcode(the_code, 5, opcode);
			if(the_stack->value.integer==0)
				mxNextOpcode(the_code, anOffset, opcode);
			the_stack++;
			mxBreak;
		}

		mxCase(XS_BRANCH_ELSE2) {
			txInteger results; 
			txInteger kind; 
			const txInteger kindList =	(1 << XS_UNDEFINED_KIND)
									 |	(1 << XS_NULL_KIND)
									 |	(1 << XS_BOOLEAN_KIND)
									 |	(1 << XS_INTEGER_KIND)
									 |	(1 << XS_REFERENCE_KIND); 

			kind = the_stack->kind;
			mxImmediateS4(the_code, 1, anOffset);
			if ((((kindList) >> kind) & 1) != 0) {
				mxNextOpcode(the_code, 5, opcode);
				results = (0 << XS_UNDEFINED_KIND) | (0 << XS_NULL_KIND) | (1 << XS_REFERENCE_KIND);
				if (the_stack->value.integer != 0)
					results |= (1 << XS_BOOLEAN_KIND) | (1 << XS_INTEGER_KIND);
				the_stack++;
				if (((results >> kind) & 1) == 0)
					mxNextOpcode(the_code, anOffset, opcode);
				mxBreak;
			} 
			else if (kind == XS_STRING_KIND) {
				mxNextOpcode(the_code, 5, opcode);
				if (*the_stack->value.string == 0)
					mxNextOpcode(the_code, anOffset, opcode);
				the_stack++;
				mxBreak;
			} 
			else
				goto exit_accelerator;
		}

		mxCase(XS_BRANCH_IF_BOOL)
		{
			mxImmediateS2(the_code, 1, anIndex);
			mxNextOpcode(the_code, 3, opcode);
			if(the_stack->value.integer!=0)
				mxNextOpcode(the_code, anIndex, opcode);
			the_stack++;
			mxBreak;
		}

		mxCase(XS_BRANCH_IF) {
			txInteger results; 
			txInteger kind; 
			const txInteger kindList =	(1 << XS_UNDEFINED_KIND)
									 |	(1 << XS_NULL_KIND)
									 |	(1 << XS_BOOLEAN_KIND)
									 |	(1 << XS_INTEGER_KIND)
									 |	(1 << XS_REFERENCE_KIND); 

			kind = the_stack->kind;
			mxImmediateS2(the_code, 1, anIndex);
			if ((((kindList) >> kind) & 1) != 0) {
				mxNextOpcode(the_code, 3, opcode);
				results = (0 << XS_UNDEFINED_KIND) | (0 << XS_NULL_KIND) | (1 << XS_REFERENCE_KIND);
				if (the_stack->value.integer != 0)
					results |= (1 << XS_BOOLEAN_KIND) | (1 << XS_INTEGER_KIND);
				the_stack++;
				if (((results >> kind) & 1) != 0)
					mxNextOpcode(the_code, anIndex, opcode);
				mxBreak;
			} 
			else if (kind == XS_STRING_KIND) {
				mxNextOpcode(the_code, 3, opcode);
				if (*the_stack->value.string != 0)
					mxNextOpcode(the_code, anIndex, opcode);
				the_stack++;
				mxBreak;
			} 
			else
				goto exit_accelerator;
		}

		mxCase(XS_BRANCH_IF_BOOL2) 
		{
			mxImmediateS4(the_code, 1, anOffset);
			mxNextOpcode(the_code, 5, opcode);
			if(the_stack->value.integer!=0)
				mxNextOpcode(the_code, anOffset, opcode);
			the_stack++;
			mxBreak;
		}

		mxCase(XS_BRANCH_IF2) 
		{
			txInteger results; 
			txInteger kind; 
			const txInteger kindList =	(1 << XS_UNDEFINED_KIND)
									 |	(1 << XS_NULL_KIND)
									 |	(1 << XS_BOOLEAN_KIND)
									 |	(1 << XS_INTEGER_KIND)
									 |	(1 << XS_REFERENCE_KIND); 

			kind = the_stack->kind;
			mxImmediateS4(the_code, 1, anOffset);
			if ((((kindList) >> kind) & 1) != 0) {
				mxNextOpcode(the_code, 5, opcode);
				results = (0 << XS_UNDEFINED_KIND) | (0 << XS_NULL_KIND) | (1 << XS_REFERENCE_KIND);
				if (the_stack->value.integer != 0)
					results |= (1 << XS_BOOLEAN_KIND) | (1 << XS_INTEGER_KIND);
				the_stack++;
				if (((results >> kind) & 1) != 0)
					mxNextOpcode(the_code, anOffset, opcode);
				mxBreak;
			} 
			else if (kind == XS_STRING_KIND) {
				mxNextOpcode(the_code, 5, opcode);
				if (*the_stack->value.string != 0)
					mxNextOpcode(the_code, anOffset, opcode);
				the_stack++;
				mxBreak;
			} 
			else
				goto exit_accelerator;
		}

		mxCase(XS_BREAK)
			goto exit_accelerator;

		mxCase(XS_DEBUGGER)
		#ifdef mxDebug
			goto exit_accelerator;
		#endif
			mxNextOpcode(the_code, 1, opcode);
			mxBreak;

		mxCase(XS_DUB)
			the_stack--;
			__CopySlot(the_stack,the_stack+1);
			mxNextOpcode(the_code, 1, opcode);
			mxBreak;

		mxCase(XS_FILE)
			mxImmediateU2(the_code, 1, (*theRoute)->ID);
			mxGetSymbol(the, (*theRoute)->ID, (*theRoute)->next);
			(*theRoute)->ID = 0;
			mxNextOpcode(the_code, 3, opcode);
			mxBreak;


		mxCase(XS_GLOBAL)
			mxNextOpcode(the_code, 1, opcode);
			__CopySlot(--the_stack,&the->stackTop[-1]);
			mxBreak;

		mxCase(XS_LINE)
		#ifdef mxDebug
			if ((the->frame) && (the->frame - 1) && ((the->frame - 1)->next)) {
				if (mxBreakpoints.value.list.first) goto exit_accelerator;
				if ((the->frame->flag & XS_STEP_OVER_FLAG)) goto exit_accelerator;
			}
		#endif
			mxImmediateU2(the_code, 1, (*theRoute)->ID);
			mxNextOpcode(the_code, 3, opcode);
			mxBreak;

		mxCase(XS_POP)
			mxNextOpcode(the_code, 1, opcode);
			the_stack++;
			mxBreak;
		
		mxCase(XS_RESULT)
			mxNextOpcode(the_code, 1, opcode);
			__CopySlot(mxResult,the_stack);
			mxBreak;
			
		mxCase(XS_SWAP) {
			__SwapSlot(the_stack,the_stack+1);
			mxNextOpcode(the_code, 1, opcode);
			mxBreak;
		}

		mxCase(XS_THIS)
			__CopySlot(--the_stack,mxThis);
			mxNextOpcode(the_code, 1, opcode);
			mxBreak;
		
		mxCase(XS_CALL) {
			txSlot* aFunction;
			if (the_stack->kind != XS_REFERENCE_KIND) goto exit_accelerator;
			aFunction = the_stack->value.reference;
			if (!(aFunction->flag & XS_VALUE_FLAG)) goto exit_accelerator;
			if (!((aFunction->next->kind == XS_CALLBACK_KIND) || (aFunction->next->kind == XS_CODE_KIND))) goto exit_accelerator;
			the_code++;
			/* RESULT */
			__ZeroSlot(--the_stack);
			/* FRAME */
			__InitSlot(--the_stack, XS_FRAME_KIND);
			the_stack->next = the->frame;
			the_stack->ID = (the_stack + 3)->ID;
			the_stack->flag = XS_NO_FLAG;
#ifdef mxDebug
			if (the->frame && (the->frame->flag & XS_STEP_INTO_FLAG))
				the_stack->flag |= XS_STEP_INTO_FLAG | XS_STEP_OVER_FLAG;
#endif
			the_stack->value.frame.code = the_code;
			the_stack->value.frame.scope = the->scope;
			if (aFunction->next->kind == XS_CALLBACK_KIND) {
				the_stack->flag |= XS_C_FLAG;
				the->frame = the_stack;
				the->scope = C_NULL;
				the->code = aFunction->next->next->value.closure.symbolMap;
				__InitSlot(--the_stack, XS_INTEGER_KIND);
				the->stack = the_stack;
#ifdef mxProfile
				fxDoCallback(the, aFunction->next->value.callback.address);
#else
				(*(aFunction->next->value.callback.address))(the);
#endif
				the_stack = mxArgv(-1);
				the->scope = the->frame->value.frame.scope;
				the_code = the->frame->value.frame.code;
				mxNextOpcode(the_code, 0, opcode);
				if(opcode!=XS_POP) {
					__CopySlot(--the_stack,mxResult);
				}
				else {
					mxNextOpcode(the_code, 1, opcode);
				}
				the->frame = the->frame->next;
			}
			else {
				if (the->status == XS_THROW_STATUS) {
					the->status = XS_NO_STATUS;
					the_stack->flag |= XS_THROW_FLAG;
				}
				txByte aLength;
				the->frame = the_stack;
				the_code = aFunction->next->value.code;
				mxNextOpcode(the_code, 0, opcode);
				aCount = (txByte)mxArgc;
				aLength = *(mxFunction->value.reference->next->value.code + 4);
				aOffset = (aLength <= aCount)? 4:(aLength - aCount) + 4;
				frameFlag = (the->frame->flag & XS_SANDBOX_FLAG)?XS_DONT_SCRIPT_FLAG:XS_SANDBOX_FLAG;
			}
			mxBreak;
		}

		mxCase(PSEUDO_DOUBLE_GET_NEGATIVE)
		{
			txSlot *aProperty0,*aProperty1;
			PSEUDO_PART_GET_NEGATIVE_ID2(aProperty0,aProperty1);
			if((!aProperty0) || (!aProperty1)) goto exit_accelerator;
			if(aProperty0->kind == XS_ACCESSOR_KIND) goto exit_accelerator;
			__InitSlotValue(--the_stack, aProperty0);
			if(aProperty1->kind == XS_ACCESSOR_KIND) {
				the_code+=2;
				goto exit_accelerator;
			}
			__InitSlotValue(--the_stack, aProperty1);
			mxNextOpcode(the_code, 4, opcode);
			mxBreak;
		}

		mxCase(XS_GET_NEGATIVE_ID)
		{
			txByte theIndex = *(the_code+1);
			txSlot* aProperty;
			aProperty = the->frame + theIndex - aOffset;
			if (!aProperty) goto exit_accelerator;
			if (aProperty->kind == XS_ACCESSOR_KIND) goto exit_accelerator;
			mxNextOpcode(the_code, 2, opcode);
			if ((opcode == XS_CALL) || (opcode == XS_NEW)) {
				__ZeroSlot(--the_stack);
				the_stack->ID = XS_NO_ID;
			}
			__InitSlotValue(--the_stack, aProperty);
			mxBreak;
		}

		mxCase(PSEUDO_DOUBLE_GET_AT) 
		{
			txByte theIndex0 = *(the_code+1);
			txByte theIndex1 = *(the_code+3);
			txSlot* aProperty0,*aProperty1;
			if(theIndex0>=0) 
				aProperty0 = the->frame + aCount - theIndex0 + ((theIndex0 < aCount)?4:(- 5));
			else 
				aProperty0 = the->frame + theIndex0 - aOffset;

			if(theIndex1>=0) 
				aProperty1 = the->frame + aCount - theIndex1 + ((theIndex1 < aCount)?4:(- 5));
			else 
				aProperty1 = the->frame + theIndex1 - aOffset;

			if((!aProperty0) || (!aProperty1)) goto exit_accelerator;
			if (aProperty0->kind == XS_ACCESSOR_KIND) goto exit_accelerator;
			__InitSlotValue(--the_stack, aProperty0);
			if (aProperty1->kind == XS_ACCESSOR_KIND) {
				the_code+=2;
				goto exit_accelerator;
			}
			__InitSlotValue(--the_stack, aProperty1);
			mxNextOpcode(the_code, 4, opcode);
			mxBreak;
		}

		mxCase(XS_GET_AT) 
		{
			txByte theIndex = *(the_code+1);
			{
				txSlot* aProperty;
				if(theIndex>=0) 
					aProperty = the->frame + aCount - theIndex + ((theIndex < aCount)?4:(- 5));
				else 
					aProperty = the->frame + theIndex - aOffset;
				
				if(!aProperty)	goto exit_accelerator;
				if (aProperty->kind == XS_ACCESSOR_KIND) goto exit_accelerator;

				mxNextOpcode(the_code, 2, opcode);
				if ((opcode == XS_CALL) || (opcode == XS_NEW)) {
					__ZeroSlot(--the_stack);
					the_stack->ID = XS_NO_ID;
				}
				__InitSlotValue(--the_stack, aProperty);
			}
			mxBreak;
		}

		mxCase(XS_SET_NEGATIVE_ID)
		{
			txByte theIndex = *(the_code+1);
			{
				txSlot* aProperty;
				aProperty = the->frame + theIndex - aOffset;
				if (!aProperty) goto exit_accelerator;
				if (aProperty->kind == XS_ACCESSOR_KIND) goto exit_accelerator;
				if (aProperty->flag & XS_DONT_SET_FLAG) {
					if (the->frame->flag & XS_STRICT_FLAG)
						goto exit_accelerator;
					the_stack->kind = aProperty->kind;
					the_stack->value = aProperty->value;
				}
				else {
					aProperty->kind = the_stack->kind;
					aProperty->value = the_stack->value;
				}
			}
			mxNextOpcode(the_code, 2, opcode);
			mxBreak;
		}

		mxCase(XS_SET_AT) 
		{
			txByte theIndex = *(the_code+1);
			{
				txSlot* aProperty;
				if(theIndex>=0) 
					aProperty = the->frame + aCount - theIndex + ((theIndex < aCount)?4:(- 5));
				else 
					aProperty = the->frame + theIndex - aOffset;
	
				if (!aProperty) goto exit_accelerator;
				if (aProperty->kind == XS_ACCESSOR_KIND) goto exit_accelerator;
				
				if (aProperty->flag & XS_DONT_SET_FLAG) {
					if (the->frame->flag & XS_STRICT_FLAG)
						goto exit_accelerator;
					the_stack->kind = aProperty->kind;
					the_stack->value = aProperty->value;
				}
				else {
					aProperty->kind = the_stack->kind;
					aProperty->value = the_stack->value;
				}
			}
			mxNextOpcode(the_code, 2, opcode);
			mxBreak;
		}

		mxCase(XS_GET_MEMBER_AT) {
			txInteger anInteger;
			txSlot* anInstance;
			txSlot* aProperty;
			if (the_stack->kind != XS_INTEGER_KIND) goto exit_accelerator;
			anInteger = the_stack->value.integer;
			if ((anInteger < 0) || (XS_MAX_INDEX < anInteger))
				goto exit_accelerator;
			anInstance = the_stack + 1;
			if (anInstance->kind == XS_ALIAS_KIND)
				anInstance = the->aliasArray[anInstance->value.alias];
			else if (anInstance->kind == XS_REFERENCE_KIND) 
				anInstance = anInstance->value.reference;
			else
				goto exit_accelerator;
			if (!(anInstance->flag & XS_VALUE_FLAG))
				goto exit_accelerator;
			aProperty = anInstance->next;
			if (aProperty->kind != XS_ARRAY_KIND)
				goto exit_accelerator;
			if ((txIndex)anInteger >= aProperty->value.array.length) 
				goto exit_accelerator;
			aProperty = aProperty->value.array.address + anInteger;
			if (!aProperty->ID)
				goto exit_accelerator;
			if (aProperty->kind == XS_ACCESSOR_KIND) {
				*resProperty = resultProperty;
				goto exit_accelerator;
			}
#ifdef mxTrace
			if (gxDoTrace)
				fxTraceSymbol(the, anIndex);
#endif
			the_stack++;
			mxNextOpcode(the_code, 1, opcode);
			if ((opcode == XS_CALL) || (opcode == XS_NEW)) {
				the_stack->ID = anIndex;
				--the_stack;
			}
			__InitSlotValue(the_stack, aProperty);
			*resProperty = C_NULL;
			mxBreak;
		}

		mxCase(XS_SET_MEMBER_AT) {
			txInteger anInteger;
			txSlot* anInstance;
			txSlot* aProperty;
			if (the_stack->kind != XS_INTEGER_KIND) goto exit_accelerator;
			anInteger = the_stack->value.integer;
			if ((anInteger < 0) || (XS_MAX_INDEX < anInteger))
				goto exit_accelerator;
			anInstance = the_stack + 1;
			if (anInstance->kind == XS_ALIAS_KIND)
				anInstance = the->aliasArray[anInstance->value.alias];
			else if (anInstance->kind == XS_REFERENCE_KIND) 
				anInstance = anInstance->value.reference;
			else
				goto exit_accelerator;
			if (!(anInstance->flag & XS_VALUE_FLAG))
				goto exit_accelerator;
			aProperty = anInstance->next;
			if (aProperty->kind != XS_ARRAY_KIND)
				goto exit_accelerator;
			if ((txIndex)anInteger >= aProperty->value.array.length) 
				goto exit_accelerator;
			aProperty = aProperty->value.array.address + anInteger;
			if (!aProperty->ID)
				goto exit_accelerator;
			if (aProperty->kind == XS_ACCESSOR_KIND) {
				*resProperty = resultProperty;
				goto exit_accelerator;
			}
			if (aProperty->flag & XS_DONT_SET_FLAG) goto exit_accelerator;
#ifdef mxTrace
			if (gxDoTrace)
				fxTraceSymbol(the, anIndex);
#endif
			the_stack += 2;
			aProperty->kind = the_stack->kind;
			aProperty->value = the_stack->value;
			*resProperty = C_NULL;
			mxNextOpcode(the_code, 1, opcode);
			mxBreak;
		}
			
		mxCase(XS_UNDEFINED)
			__ZeroSlot(--the_stack);
			mxNextOpcode(the_code, 1, opcode);
			mxBreak; 

		mxCase(XS_NULL)
			__InitSlot(--the_stack, XS_NULL_KIND);
			mxNextOpcode(the_code, 1, opcode);
			mxBreak;

		mxCase(XS_FALSE)
			__InitSlot(--the_stack, XS_BOOLEAN_KIND);
			mxNextOpcode(the_code, 1, opcode);
			mxBreak;

		mxCase(XS_TRUE)
			__InitSlot(--the_stack, XS_BOOLEAN_KIND);
			the_stack->value.boolean = 1;
			mxNextOpcode(the_code, 1, opcode);
			mxBreak;

		mxCase(XS_INTEGER_8)
			__InitSlotS8(--the_stack,XS_INTEGER_KIND,*(the_code+1));
			mxNextOpcode(the_code, 2, opcode);
			mxBreak;

		mxCase(XS_INTEGER_16)
			__InitSlot(--the_stack, XS_INTEGER_KIND);
			mxImmediateS2(the_code, 1, anIndex);
			the_stack->value.integer = anIndex;
			mxNextOpcode(the_code, 3, opcode);
			mxBreak;
		
		mxCase(XS_INTEGER_32)
			__InitSlot(--the_stack, XS_INTEGER_KIND);
			mxImmediateU4(the_code, 1, the_stack->value.integer);
			mxNextOpcode(the_code, 5, opcode);
			mxBreak;

		mxCase(XS_NUMBER)
			__InitSlot(--the_stack, XS_NUMBER_KIND);
			the_code++;
			mxDecode8(the_code, the_stack->value.number);
			mxNextOpcode(the_code, 0, opcode);
			mxBreak;

		mxCase(XS_VOID)
			__ZeroSlot(the_stack);
			mxNextOpcode(the_code, 1, opcode);
			mxBreak;
			
		mxCase(XS_INCREMENT)
			if (the_stack->kind != XS_INTEGER_KIND) goto exit_accelerator;
			the_stack->value.integer++;
			mxNextOpcode(the_code, 1, opcode);
			mxBreak;
			
		mxCase(XS_DECREMENT)
			if (the_stack->kind != XS_INTEGER_KIND) goto exit_accelerator;
			the_stack->value.integer--;
			mxNextOpcode(the_code, 1, opcode);
			mxBreak;
			
		mxCase(XS_PLUS)
			if (the_stack->kind != XS_INTEGER_KIND) goto exit_accelerator;
			mxNextOpcode(the_code, 1, opcode);
			mxBreak;
			
		mxCase(XS_MINUS)
			if (the_stack->kind != XS_INTEGER_KIND) goto exit_accelerator;
			the_stack->value.integer = -the_stack->value.integer;
			mxNextOpcode(the_code, 1, opcode);
			mxBreak;
			
		mxCase(XS_BIT_NOT)
			if (the_stack->kind != XS_INTEGER_KIND) goto exit_accelerator;
			the_stack->value.integer = ~the_stack->value.integer;
			mxNextOpcode(the_code, 1, opcode);
			mxBreak;
			
		mxCase(XS_NOT) {
			txInteger kind; 
			txInteger results; 
			const txInteger kindList =	(1 << XS_UNDEFINED_KIND)
									 |	(1 << XS_NULL_KIND)
									 |	(1 << XS_REFERENCE_KIND)
									 |	(1 << XS_STRING_KIND); 

			kind = the_stack->kind;

			if (kind == XS_BOOLEAN_KIND) {
				the_stack->value.boolean = !the_stack->value.boolean;
				mxNextOpcode(the_code, 1, opcode);
				mxBreak;
			} else

			if ((((kindList) >> kind) & 1) != 0) {
				results = (1 << XS_UNDEFINED_KIND) | (1 << XS_NULL_KIND) |  (0 << XS_REFERENCE_KIND) | (0 << XS_STRING_KIND);
				if ((kind == XS_STRING_KIND) && (*the_stack->value.string == 0))
					results |= 1 << XS_STRING_KIND;
				the_stack->kind = XS_BOOLEAN_KIND;
				the_stack->value.boolean = (txBoolean) ((results >> kind) & 1);
				mxNextOpcode(the_code, 1, opcode);
				mxBreak;
			} else
			goto exit_accelerator;
		}
			
		mxCase(XS_MULTIPLY)
			/* different behavior than the non-accelerated version */
			/* accelerate 16*16 signed integer multiply, can't overflow */
			if ((the_stack+0)->kind != XS_INTEGER_KIND) goto exit_accelerator;
			if ((the_stack+1)->kind != XS_INTEGER_KIND) goto exit_accelerator;
			if ((the_stack+0)->value.integer != (txS2)(the_stack+0)->value.integer)
				goto exit_accelerator;
			if ((the_stack+1)->value.integer != (txS2)(the_stack+1)->value.integer)
				goto exit_accelerator;
			(the_stack+1)->value.integer *= (the_stack+0)->value.integer;
			the_stack++;
			mxNextOpcode(the_code, 1, opcode);
			mxBreak;
			
		mxCase(XS_ADD)
			if ((the_stack+0)->kind != XS_INTEGER_KIND) goto exit_accelerator;
			if ((the_stack+1)->kind != XS_INTEGER_KIND) goto exit_accelerator;
			(the_stack+1)->value.integer += (the_stack+0)->value.integer;
			the_stack++;
			mxNextOpcode(the_code, 1, opcode);
			mxBreak;
	
		mxCase(XS_SUBTRACT)
			if ((the_stack+0)->kind != XS_INTEGER_KIND) goto exit_accelerator;
			if ((the_stack+1)->kind != XS_INTEGER_KIND) goto exit_accelerator;
			(the_stack+1)->value.integer -= (the_stack+0)->value.integer;
			the_stack++;
			mxNextOpcode(the_code, 1, opcode);
			mxBreak;

		mxCase(XS_LEFT_SHIFT)
			if ((the_stack+0)->kind != XS_INTEGER_KIND) goto exit_accelerator;
			if ((the_stack+1)->kind != XS_INTEGER_KIND) goto exit_accelerator;
			(the_stack+1)->value.integer <<= (the_stack+0)->value.integer & 0x1f;
			the_stack++;
			mxNextOpcode(the_code, 1, opcode);
			mxBreak;

		mxCase(XS_SIGNED_RIGHT_SHIFT)
			if ((the_stack+0)->kind != XS_INTEGER_KIND) goto exit_accelerator;
			if ((the_stack+1)->kind != XS_INTEGER_KIND) goto exit_accelerator;
			(the_stack+1)->value.integer >>= (the_stack+0)->value.integer & 0x1f;
			the_stack++;
			mxNextOpcode(the_code, 1, opcode);
			mxBreak;

		mxCase(XS_UNSIGNED_RIGHT_SHIFT)
			if ((the_stack+0)->kind != XS_INTEGER_KIND) goto exit_accelerator;
			if ((the_stack+1)->kind != XS_INTEGER_KIND) goto exit_accelerator;
			//if (0 == (0x1f & (the_stack+0)->value.integer)) goto exit_accelerator;
			{
				txU4 temp = (txU4)((the_stack+1)->value.integer);
				temp >>= (the_stack+0)->value.integer & 0x1f;
				(the_stack+1)->value.integer = temp;
			}
			the_stack++;
			mxNextOpcode(the_code, 1, opcode);
			mxBreak;

		mxCase(XS_LESS) 
		{
			txBoolean result; 

			if ((the_stack+0)->kind != XS_INTEGER_KIND) goto exit_accelerator;
			if ((the_stack+1)->kind != XS_INTEGER_KIND) goto exit_accelerator;
			result = (the_stack+1)->value.integer < (the_stack+0)->value.integer;
			the_stack++;
			__InitSlotU8(the_stack,XS_BOOLEAN_KIND,result);
			mxNextOpcode(the_code, 1, opcode);
			mxBreak;
		}

		mxCase(XS_LESS_EQUAL) 
		{
			txBoolean result; 

			if ((the_stack+0)->kind != XS_INTEGER_KIND) goto exit_accelerator;
			if ((the_stack+1)->kind != XS_INTEGER_KIND) goto exit_accelerator;
			result = (the_stack+1)->value.integer <= (the_stack+0)->value.integer;
			the_stack++;
			__InitSlotU8(the_stack,XS_BOOLEAN_KIND,result);
			mxNextOpcode(the_code, 1, opcode);
			mxBreak;
		}

		mxCase(XS_MORE) 
		{
			txBoolean result; 

			if ((the_stack+0)->kind != XS_INTEGER_KIND) goto exit_accelerator;
			if ((the_stack+1)->kind != XS_INTEGER_KIND) goto exit_accelerator;
			result = (the_stack+1)->value.integer > (the_stack+0)->value.integer;
			the_stack++;
			__InitSlotU8(the_stack,XS_BOOLEAN_KIND,result);
			mxNextOpcode(the_code, 1, opcode);
			mxBreak;
		}

		mxCase(XS_MORE_EQUAL) 
		{
			txBoolean result; 

			if ((the_stack+0)->kind != XS_INTEGER_KIND) goto exit_accelerator;
			if ((the_stack+1)->kind != XS_INTEGER_KIND) goto exit_accelerator;
			result = (the_stack+1)->value.integer >= (the_stack+0)->value.integer;
			the_stack++;
			__InitSlotU8(the_stack,XS_BOOLEAN_KIND,result);
			mxNextOpcode(the_code, 1, opcode);
			mxBreak;
		}

		mxCase(XS_EQUAL) {
			txBoolean result;
			txKind kind = (the_stack+0)->kind;
			
			if (kind != (the_stack+1)->kind) {
				const txInteger kindList =	(1 << XS_UNDEFINED_KIND)
										|	(1 << XS_NULL_KIND);
				txInteger kinds = (1 << kind) | (1 << (the_stack+1)->kind);
				
				if (kinds & kindList)				// at least one operand is null or undefined
					result = kinds == kindList;		// equal if one is null and the other is undefined, else not equal
				else
					goto exit_accelerator;
			}
			else {
				if ((kind == XS_UNDEFINED_KIND) || (kind == XS_NULL_KIND))
					result = 1;
				else if (kind == XS_BOOLEAN_KIND)
					result = (the_stack+1)->value.boolean == (the_stack+0)->value.boolean;
				else if (kind == XS_INTEGER_KIND)
					result = (the_stack+1)->value.integer == (the_stack+0)->value.integer;
				else if (kind == XS_REFERENCE_KIND)
					result = (the_stack+1)->value.reference == (the_stack+0)->value.reference;
				else if (kind == XS_STRING_KIND) {
					char *a = (the_stack+0)->value.string, *b = (the_stack+1)->value.string;
					while (1) {
						char aa = *a++;
						if (aa != *b++) {
							result = 0;
							break;
						}
						if (!aa) {
							result = 1;
							break;
						}
					}
				}
				else if (kind == XS_ALIAS_KIND)
					result = the->aliasArray[(the_stack+1)->value.alias] == the->aliasArray[(the_stack+0)->value.alias];
				else
					goto exit_accelerator;
			}
			the_stack++;
			__InitSlotU8(the_stack,XS_BOOLEAN_KIND,result);
			mxNextOpcode(the_code, 1, opcode);
			mxBreak;
		}
			
		mxCase(XS_STRICT_EQUAL) {
			txBoolean result;
			txKind kind = (the_stack+0)->kind;
			if (kind != (the_stack+1)->kind) {
				if ((XS_INTEGER_KIND == kind) && (XS_NUMBER_KIND == (the_stack+1)->kind))
					goto exit_accelerator;
				if ((XS_NUMBER_KIND == kind) && (XS_INTEGER_KIND == (the_stack+1)->kind))
					goto exit_accelerator;

				result = 0;
			}
			else if ((kind == XS_UNDEFINED_KIND) || (kind == XS_NULL_KIND))
				result = 1;
			else if (kind == XS_BOOLEAN_KIND)
				result = (the_stack+1)->value.boolean == (the_stack+0)->value.boolean;
			else if (kind == XS_INTEGER_KIND)
				result = (the_stack+1)->value.integer == (the_stack+0)->value.integer;
			else if (kind == XS_REFERENCE_KIND)
				result = (the_stack+1)->value.reference == (the_stack+0)->value.reference;
			else if (kind == XS_STRING_KIND) {
				char *a = (the_stack+0)->value.string, *b = (the_stack+1)->value.string;
				while (1) {
					char aa = *a++;
					if (aa != *b++) {
						result = 0;
						break;
					}
					if (!aa) {
						result = 1;
						break;
					}
				}
			}
			else
				goto exit_accelerator;
			the_stack++;
			__InitSlotU8(the_stack,XS_BOOLEAN_KIND,result);
			mxNextOpcode(the_code, 1, opcode);
			mxBreak;
		}
			
		mxCase(XS_NOT_EQUAL) {
			txBoolean result;
			txKind kind = (the_stack+0)->kind;

			if (kind != (the_stack+1)->kind) {
				const txInteger kindList =	(1 << XS_UNDEFINED_KIND)
										|	(1 << XS_NULL_KIND);
				txInteger kinds = (1 << kind) | (1 << (the_stack+1)->kind);

				if (kinds & kindList)				// at least one operand is null or undefined
					result = kinds != kindList;		// equal if one is null and the other is undefined, else not equal
				else
					goto exit_accelerator;
			}
			else {
				if ((kind == XS_UNDEFINED_KIND) || (kind == XS_NULL_KIND))
					result = 0;
				else if (kind == XS_BOOLEAN_KIND)
					result = (the_stack+1)->value.boolean != (the_stack+0)->value.boolean;
				else if (kind == XS_INTEGER_KIND)
					result = (the_stack+1)->value.integer != (the_stack+0)->value.integer;
				else if (kind == XS_REFERENCE_KIND)
					result = (the_stack+1)->value.reference != (the_stack+0)->value.reference;
				else if (kind == XS_STRING_KIND) {
					char *a = (the_stack+0)->value.string, *b = (the_stack+1)->value.string;
					while (1) {
						char aa = *a++;
						if (aa != *b++) {
							result = 1;
							break;
						}
						if (!aa) {
							result = 0;
							break;
						}
					}
				}
				else if (kind == XS_ALIAS_KIND)
					result = the->aliasArray[(the_stack+1)->value.alias] != the->aliasArray[(the_stack+0)->value.alias];
				else
					goto exit_accelerator;
			}
			the_stack++;
			__InitSlotU8(the_stack,XS_BOOLEAN_KIND,result);
			mxNextOpcode(the_code, 1, opcode);
			mxBreak;
		}

		mxCase(XS_STRICT_NOT_EQUAL) {
			txBoolean result;
			txKind kind = (the_stack+0)->kind;

			if (kind != (the_stack+1)->kind) {
				if ((XS_INTEGER_KIND == kind) && (XS_NUMBER_KIND == (the_stack+1)->kind))
					goto exit_accelerator;
				if ((XS_NUMBER_KIND == kind) && (XS_INTEGER_KIND == (the_stack+1)->kind))
					goto exit_accelerator;
				result = 1;
			}
			else {
				if ((kind == XS_UNDEFINED_KIND) || (kind == XS_NULL_KIND))
					result = 0;
				else if (kind == XS_BOOLEAN_KIND)
					result = (the_stack+1)->value.boolean != (the_stack+0)->value.boolean;
				else if (kind == XS_INTEGER_KIND)
					result = (the_stack+1)->value.integer != (the_stack+0)->value.integer;
				else if (kind == XS_REFERENCE_KIND)
					result = (the_stack+1)->value.reference != (the_stack+0)->value.reference;
				else
					goto exit_accelerator;
			}
			the_stack++;
			__InitSlotU8(the_stack,XS_BOOLEAN_KIND,result);

			mxNextOpcode(the_code, 1, opcode);
			mxBreak;
		}
			
		mxCase(XS_BIT_AND)
			if ((the_stack+0)->kind != XS_INTEGER_KIND) goto exit_accelerator;
			if ((the_stack+1)->kind != XS_INTEGER_KIND) goto exit_accelerator;
			(the_stack+1)->value.integer &= (the_stack+0)->value.integer;
			the_stack++;
			mxNextOpcode(the_code, 1, opcode);
			mxBreak;
		
		mxCase(XS_BIT_OR)
			if ((the_stack+0)->kind != XS_INTEGER_KIND) goto exit_accelerator;
			if ((the_stack+1)->kind != XS_INTEGER_KIND) goto exit_accelerator;
			(the_stack+1)->value.integer |= (the_stack+0)->value.integer;
			the_stack++;
			mxNextOpcode(the_code, 1, opcode);
			mxBreak;

		mxCase(XS_BIT_XOR)
			if ((the_stack+0)->kind != XS_INTEGER_KIND) goto exit_accelerator;
			if ((the_stack+1)->kind != XS_INTEGER_KIND) goto exit_accelerator;
			(the_stack+1)->value.integer ^= (the_stack+0)->value.integer;
			the_stack++;
			mxNextOpcode(the_code, 1, opcode);
			mxBreak;

		mxCase(XS_STRING_POINTER) 
		{
#if mxStringLength
			mxImmediateU2(the_code,1,anIndex);
#else
			anIndex = (txID)c_strlen((const char*)the_code+1) + 1;
#endif
			mxZeroSlot(--the_stack);
			the_stack->value.string = (txString)((char *)the_code + 1 + stringCommandOffset);
			the_stack->kind = XS_STRING_KIND;
			mxNextOpcode(the_code,anIndex+1+stringCommandOffset,opcode);
			mxBreak;
		}

		mxCase(PSEUDO_DUB)
		{
			*the_stack = *(the_stack+1);
			mxNextOpcode(the_code, 1, opcode);
			mxBreak;
		}


		mxCase(PSEUDO_INCREMENT)
		{
			txSlot* aProperty;
			PSEUDO_PART_GET_NEGATIVE_ID(aProperty);
			if (!aProperty) {
				goto exit_accelerator;
			}
			if (aProperty->kind == XS_ACCESSOR_KIND){
				goto exit_accelerator;
			}
			if (aProperty->kind != XS_INTEGER_KIND) goto exit_accelerator;
			if (aProperty->flag & XS_DONT_SET_FLAG)  goto exit_accelerator;
			aProperty->value.integer++;
			mxNextOpcode(the_code, 6, opcode);
			mxBreak;
		}

		mxCase(PSEUDO_DECREMENT)
		{
			txSlot* aProperty;
			PSEUDO_PART_GET_NEGATIVE_ID(aProperty);
			if (!aProperty) {
				goto exit_accelerator;
			}
			if (aProperty->kind == XS_ACCESSOR_KIND){
				goto exit_accelerator;
			}
			if (aProperty->kind != XS_INTEGER_KIND) goto exit_accelerator;
			if (aProperty->flag & XS_DONT_SET_FLAG)  goto exit_accelerator;
			aProperty->value.integer--;
			mxNextOpcode(the_code, 6, opcode);
			mxBreak;
		}

		mxCase(PSEUDO_LESS_EQUAL)
		{
			txSlot* aProperty0,*aProperty1;
			PSEUDO_PART_GET_NEGATIVE_ID2(aProperty0,aProperty1);
			if ((!aProperty0) || (!aProperty1)) goto exit_accelerator;
			if (aProperty0->kind == XS_ACCESSOR_KIND) goto exit_accelerator;
			if (aProperty1->kind == XS_ACCESSOR_KIND) goto exit_accelerator;
			the_code+=4;
			txBoolean result; 

			if (aProperty0->kind != XS_INTEGER_KIND || aProperty1->kind != XS_INTEGER_KIND) {
				__InitSlotValue(--the_stack,aProperty0);
				__InitSlotValue(--the_stack,aProperty1);
				goto exit_accelerator;
			}
			result = aProperty0->value.integer <= aProperty1->value.integer;
			the_code++;

			mxImmediateS2(the_code, 1, anIndex);
			mxNextOpcode(the_code, 3, opcode);
			if(result==0)
				mxNextOpcode(the_code, anIndex, opcode);
			mxBreak;
		}

		mxCase(PSEUDO_LESS)
		{
			txSlot* aProperty0,*aProperty1;
			PSEUDO_PART_GET_NEGATIVE_ID2(aProperty0,aProperty1);
			if ((!aProperty0) || (!aProperty1)) goto exit_accelerator;
			if (aProperty0->kind == XS_ACCESSOR_KIND) goto exit_accelerator;
			if (aProperty1->kind == XS_ACCESSOR_KIND) goto exit_accelerator;
			the_code+=4;
			txBoolean result; 

			if (aProperty0->kind != XS_INTEGER_KIND || aProperty1->kind != XS_INTEGER_KIND) {
				__InitSlotValue(--the_stack,aProperty0);
				__InitSlotValue(--the_stack,aProperty1);
				goto exit_accelerator;
			}
			result = aProperty0->value.integer < aProperty1->value.integer;
			the_code++;

			mxImmediateS2(the_code, 1, anIndex);
			mxNextOpcode(the_code, 3, opcode);
			if(result==0)
				mxNextOpcode(the_code, anIndex, opcode);
			mxBreak;

		}
	
		mxCase(PSEUDO_SET_MEMBER)
		{
			mxImmediateS2(the_code, 2, anIndex);
			txKind kind = mxThis->kind;
			if (XS_ALIAS_KIND == kind){
				resultInstance = the->aliasArray[mxThis->value.alias];
			}
			else if (XS_REFERENCE_KIND == kind) {
				resultInstance = mxThis->value.reference;
			}
			else { 
				__CopySlot(--the_stack,mxThis);
				the_code++;
				goto exit_accelerator;
			}
			if (resultInstance->flag & XS_SHARED_FLAG) {
				__CopySlot(--the_stack,mxThis);
				the_code++;
				goto exit_accelerator;
			}
			if(resultInstance->flag & XS_SANDBOX_FLAG) {
				resultInstance = resultInstance->value.instance.prototype;
				resultFlag = XS_DONT_SCRIPT_FLAG;
			}
			else
				resultFlag = frameFlag;

			resultProperty = resultInstance->next;
			if (resultInstance->flag & XS_VALUE_FLAG) {
				if (anIndex < 0) {
					if (resultProperty->kind == XS_GLOBAL_KIND) {
					#ifdef mxDebug
						if ((anIndex & 0x7FFF) >= the->symbolCount) {
							__CopySlot(--the_stack,mxThis);
							the_code++;
							goto exit_accelerator;
						}
					#endif
						resultProperty = (resultFlag & XS_SANDBOX_FLAG)?resultProperty->value.global.cache[anIndex & 0x7FFF]:resultProperty->value.global.sandboxCache[anIndex & 0x7FFF];
						if (!resultProperty) {
							__CopySlot(--the_stack,mxThis);
							the_code++;
							goto exit_accelerator;
						}
						goto gotPropertyInSet;
					}
				}
				else {
					if (resultProperty->kind == XS_ARRAY_KIND) {
						if ((txIndex)anIndex >= resultProperty->value.array.length) {
							__CopySlot(--the_stack,mxThis);
							the_code++;
							goto exit_accelerator;
						}
						resultProperty = resultProperty->value.array.address + anIndex;
						resultProperty->ID = XS_NO_ID;
						goto gotPropertyInSet;
					}
				}
			}
			
			while (resultProperty) {
				if (resultProperty->ID == anIndex) {
					if (!(resultProperty->flag & resultFlag)) {
						goto gotPropertyInSet;
					}
				}
				resultProperty = resultProperty->next;
			}
			__CopySlot(--the_stack,mxThis);
			the_code++;
			goto exit_accelerator;
			
			gotPropertyInSet:
			if (resultProperty->kind == XS_ACCESSOR_KIND) {
				__CopySlot(--the_stack,mxThis);
				the_code++;
				*resProperty = resultProperty;
				goto exit_accelerator;
			}
			if (resultProperty->flag & XS_DONT_SET_FLAG) {
				__CopySlot(--the_stack,mxThis);
				the_code++;
				*resProperty = resultProperty;
				goto exit_accelerator;
			}
			resultProperty->kind = the_stack->kind;
			resultProperty->value = the_stack->value;
			the_stack++;
			*resProperty = resultProperty = C_NULL;
			mxNextOpcode(the_code, 5, opcode);
			mxBreak;
		}

		mxCase(PSEUDO_BRANCH_ELSE0)
		{
			txSlot* aProperty;
			PSEUDO_PART_GET_NEGATIVE_ID(aProperty);
			if (!aProperty) {
				goto exit_accelerator;
			}
			if (aProperty->kind == XS_ACCESSOR_KIND){
				goto exit_accelerator;
			}
			the_code+=2;
			PSEUDO_PART_BRANCH_ELSE_NEON(aProperty);
			mxBreak;
		}

		mxCase(PSEUDO_GET_MEMBER)
		{
			mxImmediateS2(the_code, 2, anIndex);
			txKind kind = mxThis->kind;
				
			if (kind == XS_ALIAS_KIND) {
				resultInstance = the->aliasArray[mxThis->value.alias];
			}
			else if (kind == XS_REFERENCE_KIND) {
				resultInstance = mxThis->value.reference;
			}
			else {
				__CopySlot(--the_stack,mxThis);
				the_code++;
				goto exit_accelerator;
			}
#ifdef mxDebug
			if (resultInstance->kind != XS_INSTANCE_KIND) {
				__CopySlot(--the_stack,mxThis);
				the_code++;
				goto exit_accelerator;
			}
#endif
			if (resultInstance->flag & XS_SANDBOX_FLAG){
				resultInstance = resultInstance->value.instance.prototype;
				resultFlag = XS_DONT_SCRIPT_FLAG;
			}
			else
				resultFlag = frameFlag;
		getPropertyInPseudo: {
			resultProperty = resultInstance->next;
			if (resultInstance->flag & XS_VALUE_FLAG) {
				if (anIndex < 0) {
					if (resultProperty->kind == XS_GLOBAL_KIND) {
						#ifdef mxDebug
						if ((anIndex & 0x7FFF) >= the->symbolCount) {
							__CopySlot(--the_stack,mxThis);
							the_code++;
							goto exit_accelerator;
						}
						#endif
						resultProperty = (resultFlag & XS_SANDBOX_FLAG)?resultProperty->value.global.cache[anIndex & 0x7FFF]:resultProperty->value.global.sandboxCache[anIndex & 0x7FFF];
						if (!resultProperty) {
							__CopySlot(--the_stack,mxThis);
							the_code++;
							goto exit_accelerator;
						}
						goto gotProperty;
					}
				}
				else {
					if (resultProperty->kind == XS_ARRAY_KIND) {
						if ((txIndex)anIndex >= resultProperty->value.array.length){
							__CopySlot(--the_stack,mxThis);
							the_code++;
							goto exit_accelerator;
						}
						resultProperty = resultProperty->value.array.address + anIndex;
						if (!resultProperty->ID) {
							__CopySlot(--the_stack,mxThis);
							the_code++;
							goto exit_accelerator;
						}
						goto gotProperty;
					}
				}
			}
			while (resultProperty) {
				if (resultProperty->ID == anIndex) {
					if (!(resultProperty->flag & resultFlag)) {
						goto gotProperty;
					}
				}
				resultProperty = resultProperty->next;
			}
			if (resultInstance->ID >= 0) {
				if ((resultInstance = the->aliasArray[resultInstance->ID])) {
					goto getPropertyInPseudo;
				}
			}
			else if ((resultInstance = resultInstance->value.instance.prototype)) {
				goto getPropertyInPseudo;
			}
			__CopySlot(--the_stack,mxThis);
			the_code++;
			goto exit_accelerator;
		}
		gotProperty:
			if (resultProperty->kind == XS_ACCESSOR_KIND) { 
				__CopySlot(--the_stack,mxThis);
				the_code++;
				*resProperty = resultProperty;
				goto exit_accelerator;
			}
			mxNextOpcode(the_code,4,opcode);
			the_stack--;
			__InitSlotValue(the_stack,resultProperty)

			*resProperty = resultProperty = C_NULL;
			mxBreak;	
		}

		mxCase(PSEUDO_BRANCH_ELSE1) 
		{
			txSlot* aProperty;
			txByte theIndex = *(the_code+1);
			if(theIndex>=0) 
				aProperty = the->frame + aCount - theIndex + ((theIndex < aCount)?4:(- 5));
			else 
				aProperty = the->frame + theIndex - aOffset;

			if(!aProperty)	goto exit_accelerator;
			if (aProperty->kind == XS_ACCESSOR_KIND) goto exit_accelerator;
			the_code += 2;
			PSEUDO_PART_BRANCH_ELSE_NEON(aProperty);
			mxBreak;
		}

		mxCase(XS_STRING) 
#if mxStringAccelerator
		{
			txSize theSize;
			txBlock* aBlock;
			txByte* aData;
			txString aString;
			txString result;
			__ZeroSlot(--the_stack);
#if mxStringLength
			mxImmediateU2(the_code, 1, anIndex);
#else
			anIndex = (txID)c_strlen((const char *)the_code + 1) + 1;
#endif
			aString = (txString)(the_code+1+stringCommandOffset);
			mxNextOpcode(the_code,anIndex+1+stringCommandOffset,opcode);
			
			theSize = ((anIndex+(sizeof(txChunk) - 1))&~(sizeof(txChunk) - 1)) + sizeof(txChunk);
			mxChunkResult(out_string);
			the_code -= anIndex+1+stringCommandOffset;
			goto exit_accelerator;
out_string:
			the_stack->value.string = result;
			the_stack->kind = XS_STRING_KIND;
			c_memmove(the_stack->value.string, aString, anIndex);	
			mxBreak;
		}
#endif
		mxCase(XS_STRING_CONCAT)
#if mxStringAccelerator
		{
			txSize theSize;
			txBlock* aBlock;
			txByte* aData;
			txString aString;
			txSize aSize;
			txString result;
#if mxStringLength
			mxImmediateU2(the_code, 1, anIndex);
#else
			anIndex = (txID)c_strlen((const char *)the_code + 1) + 1;
#endif
			if(the_stack->kind!=XS_STRING_KIND) {
				__ZeroSlot(--the_stack);
				goto exit_accelerator;
			}
			aString = (txString)(the_code+1+stringCommandOffset);
			mxNextOpcode(the_code,anIndex+1+stringCommandOffset,opcode);
			aSize = c_strlen(the_stack->value.string);
			theSize = ((aSize + anIndex + sizeof(txChunk) - 1)&~(sizeof(txChunk) - 1)) + sizeof(txChunk);
			mxChunkResult(out_string_concat);
			__ZeroSlot(--the_stack);
			the_code -= anIndex+1+stringCommandOffset;
			goto exit_accelerator;
out_string_concat:
			c_memcpy(result, the_stack->value.string, aSize);
			c_memcpy(result + aSize, aString, anIndex);
			the_stack->value.string = result;
			the_stack->kind = XS_STRING_KIND;
			mxNextOpcode(the_code, 1, opcode);
			mxBreak;
		}
#endif
		mxCase(XS_STRING_CONCATBY)
#if mxStringAccelerator
		{
			txSize theSize;
			txBlock* aBlock;
			txByte* aData;
			txString aString;
			txSize aSize;
			txString result;
#if mxStringLength
			mxImmediateU2(the_code, 1, anIndex);
#else
			anIndex = (txID)c_strlen((const char *)the_code + 1) + 1;
#endif
			if(the_stack->kind!=XS_STRING_KIND) {
				__ZeroSlot(--the_stack);
				goto exit_accelerator;
			}
			aString = (txString)(the_code+1+stringCommandOffset);
			mxNextOpcode(the_code,anIndex+1+stringCommandOffset,opcode);
			aSize = c_strlen(the_stack->value.string);
			theSize = ((aSize + anIndex + sizeof(txChunk) - 1)&~(sizeof(txChunk) - 1)) + sizeof(txChunk);
			mxChunkResult(out_string_concatby);
			__ZeroSlot(--the_stack);
			the_code -= anIndex+1+stringCommandOffset;
			goto exit_accelerator;
out_string_concatby:
			c_memcpy(result , aString, anIndex - 1);
			c_memcpy(result + anIndex - 1, the_stack->value.string, aSize + 1);
			the_stack->value.string = result;
			the_stack->kind = XS_STRING_KIND;
			mxBreak;
		}
#endif
		mxCase(XS_UNDEFINED_KIND)
		mxCase(XS_NULL_KIND)
		mxCase(XS_BOOLEAN_KIND)
		mxCase(XS_INTEGER_KIND)
		mxCase(XS_NUMBER_KIND)
		mxCase(XS_REFERENCE_KIND)
		mxCase(XS_REGEXP_KIND)
		mxCase(XS_STRING_KIND)
		mxCase(XS_DATE_KIND)
		
		mxCase(XS_ARRAY_KIND)
		mxCase(XS_CALLBACK_KIND)
		mxCase(XS_CODE_KIND)
		mxCase(XS_HOST_KIND)

		mxCase(XS_SYMBOL_KIND)
		mxCase(XS_INSTANCE_KIND)

		mxCase(XS_FRAME_KIND)
		mxCase(XS_ROUTE_KIND)
		
		mxCase(XS_LIST_KIND)
		mxCase(XS_ACCESSOR_KIND)
		
		mxCase(XS_ALIAS_KIND)
		mxCase(XS_GLOBAL_KIND)
		
		mxCase(XS_NODE_KIND)
		mxCase(XS_PREFIX_KIND)
		mxCase(XS_ATTRIBUTE_RULE)
		mxCase(XS_DATA_RULE)
		mxCase(XS_PI_RULE)
		mxCase(XS_EMBED_RULE)
		mxCase(XS_JUMP_RULE)
		mxCase(XS_REFER_RULE)
		mxCase(XS_REPEAT_RULE)
		mxCase(XS_ERROR_RULE)

		mxCase(XS_LOCAL)
		mxCase(XS_LOCALS)
		mxCase(XS_LABEL)

		mxCase(XS_ALIAS)
		mxCase(XS_BIND)
		mxCase(XS_CATCH)
		mxCase(XS_DELETE)
		mxCase(XS_DELETE_AT)
		mxCase(XS_DELETE_MEMBER)
		mxCase(XS_DELETE_MEMBER_AT)
		mxCase(XS_DIVIDE)
		mxCase(XS_ENUM)
		mxCase(XS_FUNCTION)
		mxCase(XS_FUNCTION2)
		mxCase(XS_IN)
		mxCase(XS_INSTANCEOF)
		mxCase(XS_INSTANCIATE)
		mxCase(XS_JUMP)
		mxCase(XS_MODULO)
		mxCase(XS_NEW)
		mxCase(XS_PARAMETERS)
		mxCase(PSEUDO_PUT_MEMBER)
		mxCase(XS_PUT_MEMBER)
		mxCase(XS_PUT_MEMBER_AT)
		mxCase(XS_RETURN)
		mxCase(XS_SCOPE)
		mxCase(XS_STATUS)
		mxCase(XS_THROW)
		mxCase(XS_TYPEOF)
		mxCase(XS_UNCATCH)
		mxCase(XS_UNSCOPE)
		
		mxCase(XS_ATTRIBUTE_PATTERN)
		mxCase(XS_DATA_PATTERN)
		mxCase(XS_PI_PATTERN)
		mxCase(XS_EMBED_PATTERN)
		mxCase(XS_JUMP_PATTERN)
		mxCase(XS_REFER_PATTERN)
		mxCase(XS_REPEAT_PATTERN)
		mxCase(XS_FLAG_INSTANCE)
			goto exit_accelerator;

#if !defined(__GNUC__) || ( defined(__GNUC__) && !defined(__OPTIMIZE__) )
		default:
			goto exit_accelerator;
#endif
		}
	}

exit_accelerator:
#ifdef mxFrequency
	the->fastFrequencies[opcode]--;
#endif
	the->code = the_code;
	the->stack = the_stack;
	return anIndex;
}
#endif

txID fxRunLoopAccelerator(txMachine* the, txSlot** theRoute, txJump* theJump,txSlot **resProperty)
{
	register txByte*	the_code;
	register txSlot*	the_stack;
	register txU1		opcode;
	register txID		anIndex = XS_NO_ID; /* is not used uninitialized! */
	register txSize		anOffset; /* is not used uninitialized! */
	register txSlot*	resultInstance;
	register txSlot*	resultProperty;
	register txSlot*	resultScope;
	register txFlag		resultFlag;
	register txByte aOffset;
	register txByte aCount = (txByte)mxArgc;
	register txFlag frameFlag;
	txByte aLength;
	aLength = *(mxFunction->value.reference->next->value.code + 4);
	aOffset = (aLength <= aCount)? 4:(aLength - aCount) + 4;
	frameFlag = (the->frame->flag & XS_SANDBOX_FLAG)?XS_DONT_SCRIPT_FLAG:XS_SANDBOX_FLAG;

#if defined(__GNUC__) && defined(__OPTIMIZE__)
	static void *sOpcodes[] = {
		&&XS_UNDEFINED_KIND,
		&&XS_NULL_KIND,
		&&XS_BOOLEAN_KIND,
		&&XS_INTEGER_KIND,
		&&XS_NUMBER_KIND,
		&&XS_REFERENCE_KIND, /* 5 */
		&&XS_REGEXP_KIND,
		&&XS_STRING_KIND,
		&&XS_DATE_KIND,
		
		&&XS_ARRAY_KIND,
		&&XS_CALLBACK_KIND, /* 10 */
		&&XS_CODE_KIND,
		&&XS_HOST_KIND,

		&&XS_SYMBOL_KIND,
		&&XS_INSTANCE_KIND,

		&&XS_FRAME_KIND, /* 15 */
		&&XS_ROUTE_KIND,
		
		&&XS_LIST_KIND,
		&&XS_ACCESSOR_KIND,
		
		&&XS_ALIAS_KIND,
		&&XS_GLOBAL_KIND, /* 20 */
		
		&&XS_NODE_KIND,
		&&XS_PREFIX_KIND,
		&&XS_ATTRIBUTE_RULE,
		&&XS_DATA_RULE,
		&&XS_PI_RULE, /* 25 */
		&&XS_EMBED_RULE,
		&&XS_JUMP_RULE,
		&&XS_REFER_RULE,
		&&XS_REPEAT_RULE,
		&&XS_ERROR_RULE,

		&&XS_LOCAL,
		&&XS_LOCALS,
		&&XS_LABEL,
		
		&&XS_ADD,
		&&XS_ALIAS,
		&&XS_BEGIN,
		&&XS_BIND,
		&&XS_BIT_AND,
		&&XS_BIT_NOT,
		&&XS_BIT_OR,
		&&XS_BIT_XOR,
		&&XS_BRANCH,
		&&XS_BRANCH_ELSE,
		&&XS_BRANCH_IF,
		&&XS_BREAK,
		&&XS_CALL,
		&&XS_CATCH,
		&&XS_DEBUGGER,
		&&XS_DECREMENT,
		&&XS_DELETE,
		&&XS_DELETE_AT,
		&&XS_DELETE_MEMBER,
		&&XS_DELETE_MEMBER_AT,
		&&XS_DIVIDE,
		&&XS_DUB,
		&&XS_END,
		&&XS_ENUM,
		&&XS_EQUAL,
		&&XS_FALSE,
		&&XS_FILE,
		&&XS_FUNCTION,
		&&XS_GET,
		&&XS_GET_AT,
		&&XS_GET_MEMBER,
		&&XS_GET_MEMBER_AT,
		&&XS_GLOBAL,
		&&XS_IN,
		&&XS_INCREMENT,
		&&XS_INSTANCEOF,
		&&XS_INSTANCIATE,
		&&XS_INTEGER_8,
		&&XS_INTEGER_16,
		&&XS_INTEGER_32,
		&&XS_JUMP,
		&&XS_LEFT_SHIFT,
		&&XS_LESS,
		&&XS_LESS_EQUAL,
		&&XS_LINE,
		&&XS_MINUS,
		&&XS_MODULO,
		&&XS_MORE,
		&&XS_MORE_EQUAL,
		&&XS_MULTIPLY,
		&&XS_NEW,
		&&XS_NOT,
		&&XS_NOT_EQUAL,
		&&XS_NULL, 
		&&XS_NUMBER,
		&&XS_PARAMETERS,
		&&XS_PLUS,
		&&XS_POP,
		&&XS_PUT_MEMBER,
		&&XS_PUT_MEMBER_AT,
		&&XS_RESULT,
		&&XS_RETURN,
		&&XS_ROUTE,
		&&XS_SCOPE,
		&&XS_SET,
		&&XS_SET_AT,
		&&XS_SET_MEMBER,
		&&XS_SET_MEMBER_AT,
		&&XS_SIGNED_RIGHT_SHIFT,
		&&XS_STATUS,
		&&XS_STRICT_EQUAL,
		&&XS_STRICT_NOT_EQUAL,
		&&XS_STRING,
		&&XS_SUBTRACT,
		&&XS_SWAP,
		&&XS_THIS,
		&&XS_THROW,
		&&XS_TRUE,
		&&XS_TYPEOF,
		&&XS_UNCATCH,
		&&XS_UNDEFINED,
		&&XS_UNSCOPE,
		&&XS_UNSIGNED_RIGHT_SHIFT,
		&&XS_VOID,
		
		&&XS_ATTRIBUTE_PATTERN,
		&&XS_DATA_PATTERN,
		&&XS_PI_PATTERN,
		&&XS_EMBED_PATTERN,
		&&XS_JUMP_PATTERN,
		&&XS_REFER_PATTERN,
		&&XS_REPEAT_PATTERN,
		&&XS_FLAG_INSTANCE,
		
		&&XS_BRANCH2,
		&&XS_BRANCH_ELSE2,
		&&XS_BRANCH_IF2,
		&&XS_FUNCTION2,
		&&XS_ROUTE2,
		&&XS_GET_NEGATIVE_ID,
		&&XS_SET_NEGATIVE_ID,
		&&XS_BRANCH_ELSE_BOOL,
		&&XS_BRANCH_IF_BOOL,
		&&XS_BRANCH_ELSE_BOOL2,
		&&XS_BRANCH_IF_BOOL2,
		&&XS_STRING_POINTER,
		&&XS_STRING_CONCAT,
		&&XS_STRING_CONCATBY,
		&&PSEUDO_BRANCH_ELSE0,
		&&PSEUDO_BRANCH_ELSE1,
		&&PSEUDO_GET_MEMBER,
		&&PSEUDO_SET_MEMBER,
		&&PSEUDO_LESS,
		&&PSEUDO_LESS_EQUAL,
		&&PSEUDO_INCREMENT,
		&&PSEUDO_DECREMENT,
		&&PSEUDO_DUB,
		&&PSEUDO_DOUBLE_GET_NEGATIVE,
		&&PSEUDO_DOUBLE_GET_AT,
		&&PSEUDO_PUT_MEMBER,
		&&XS_GET_MEMBER_FOR_CALL,
		&&XS_GET_FOR_NEW,
	};
	register void **opcodes = sOpcodes;
#endif

	*resProperty = resultProperty = C_NULL;
	resultScope = C_NULL;
	the_code = the->code;
	the_stack = the->stack;
	mxNextOpcode(the_code, 0, opcode);

	for (;;) {
#if !defined(__GNUC__) || ( defined(__GNUC__) && !defined(__OPTIMIZE__) )
		decode_opcode:
#ifdef mxFrequency
		the->fastFrequencies[opcode]++;
#endif
#ifdef mxTrace
		if (gxDoTrace) {
			fxTraceAcceleratorCode(the, opcode, the_stack);
		}
#endif
#endif
			
		mxSwitch (opcode) {
		accessor_kind_process: {
			txSlot aScratch;
			txSlot* aFunction;
			mxInitSlot(--the_stack, XS_INTEGER_KIND);
			/* THIS */
			aScratch = *(the_stack);
			*(the_stack) = *(the_stack + 1);
			*(the_stack + 1) = aScratch;
			/* FUNCTION */
			aFunction = resultProperty->value.accessor.getter;
			if (!mxIsFunction(aFunction))
				mxDebugID(the, XS_TYPE_ERROR, "get %s: no getter", anIndex);
			mxInitSlot(--the_stack, XS_REFERENCE_KIND);
			the_stack->value.reference = aFunction;
			/* RESULT */
			mxZeroSlot(--the_stack);
			/* FRAME */
			mxInitSlot(--the_stack, XS_FRAME_KIND);
			the_stack->next = the->frame;
			the_stack->ID = anIndex;
			the_stack->flag = XS_NO_FLAG;
#ifdef mxDebug
			if (the->frame && (the->frame->flag & XS_STEP_INTO_FLAG))
				the_stack->flag |= XS_STEP_INTO_FLAG | XS_STEP_OVER_FLAG;
#endif
			the_stack->value.frame.code = the_code;
			the_stack->value.frame.scope = the->scope;
			if (aFunction->next->kind == XS_CALLBACK_KIND) {
				the_stack->flag |= XS_C_FLAG;
				the->frame = the_stack;
				the->scope = C_NULL;
				the->code = aFunction->next->next->value.closure.symbolMap;
				mxInitSlot(--the_stack, XS_INTEGER_KIND);
				the->stack = the_stack;
#ifdef mxProfile
				fxDoCallback(the, aFunction->next->value.callback.address);
#else
				(*(aFunction->next->value.callback.address))(the);
#endif
				the_stack = mxArgv(-1);
				the->scope = the->frame->value.frame.scope;
				the_code = the->frame->value.frame.code;
				*(--the_stack) = *mxResult;
				the->frame = the->frame->next;
			}
			else {
				txByte aLength;
				the->frame = the_stack;
				the_code = aFunction->next->value.code;
				aCount = (txByte)mxArgc;
				aLength = *(mxFunction->value.reference->next->value.code + 4);
				aOffset = (aLength <= aCount)? 4:(aLength - aCount) + 4;
				frameFlag = (the->frame->flag & XS_SANDBOX_FLAG)?XS_DONT_SCRIPT_FLAG:XS_SANDBOX_FLAG;
				mxNextOpcode(the_code, 0, opcode);
			}
			resultScope = C_NULL;
			*resProperty = resultProperty = C_NULL;
			mxBreak;
		}

		getProperty: {
			resultProperty = resultInstance->next;
			if (resultInstance->flag & XS_VALUE_FLAG) {
				if (anIndex < 0) {
					if (resultProperty->kind == XS_GLOBAL_KIND) {
					#ifdef mxDebug
						if ((anIndex & 0x7FFF) >= the->symbolCount)
							goto exit_accelerator;
					#endif
						resultProperty = (resultFlag & XS_SANDBOX_FLAG)?resultProperty->value.global.cache[anIndex & 0x7FFF]:resultProperty->value.global.sandboxCache[anIndex & 0x7FFF];
						if (!resultProperty) 
							goto exit_accelerator;
						goto_decode_opcode;
					}
				}
				else {
					if (resultProperty->kind == XS_ARRAY_KIND) {
						if ((txIndex)anIndex >= resultProperty->value.array.length) 
							goto exit_accelerator;
						resultProperty = resultProperty->value.array.address + anIndex;
						if (!resultProperty->ID)
							goto exit_accelerator;
						goto_decode_opcode;
					}
				}
			}
			while (resultProperty) {
				if (resultProperty->ID == anIndex) {
					if (!(resultProperty->flag & resultFlag)) {
						goto_decode_opcode;
					}
				}
				resultProperty = resultProperty->next;
			}
			if (resultInstance->ID >= 0) {
				if ((resultInstance = the->aliasArray[resultInstance->ID])) {
					goto getProperty;
				}
			}
			else if ((resultInstance = resultInstance->value.instance.prototype)) {
				goto getProperty;
			}
			if (resultScope) {
				resultScope = resultScope->next;
				if (resultScope) {
					if (resultScope->kind == XS_ALIAS_KIND)
						resultInstance = the->aliasArray[resultScope->value.alias];
					else if (resultScope->kind == XS_REFERENCE_KIND) 
						resultInstance = resultScope->value.reference;
					else
						goto exit_accelerator;
				#ifdef mxDebug
					if (resultInstance->kind != XS_INSTANCE_KIND)
						goto exit_accelerator;
				#endif
					if (resultInstance->flag & XS_SANDBOX_FLAG) {
						resultInstance = resultInstance->value.instance.prototype;
						resultFlag = XS_DONT_SCRIPT_FLAG;
					}	
					else
						resultFlag = frameFlag;
					goto getProperty;
				}
			}
			goto exit_accelerator;
		}
			
		putProperty: {
			resultProperty = resultInstance->next;
			if (resultInstance->flag & XS_VALUE_FLAG) {
				if (anIndex < 0) {
					if (resultProperty->kind == XS_GLOBAL_KIND) {
					#ifdef mxDebug
						if ((anIndex & 0x7FFF) >= the->symbolCount)
							goto exit_accelerator;
					#endif
						resultProperty = (resultFlag & XS_SANDBOX_FLAG)?resultProperty->value.global.cache[anIndex & 0x7FFF]:resultProperty->value.global.sandboxCache[anIndex & 0x7FFF];
						if (!resultProperty)
							goto exit_accelerator;
						goto_decode_opcode;
					}
				}
				else {
					if (resultProperty->kind == XS_ARRAY_KIND) {
						if ((txIndex)anIndex >= resultProperty->value.array.length) 
							goto exit_accelerator;
						resultProperty = resultProperty->value.array.address + anIndex;
						resultProperty->ID = XS_NO_ID;
						goto_decode_opcode;
					}
				}
			}
			while (resultProperty) {
				if (resultProperty->ID == anIndex) {
					if (!(resultProperty->flag & resultFlag)) {
						goto_decode_opcode;
					}
				}
				resultProperty = resultProperty->next;
			}
			if (resultScope) {
				resultScope = resultScope->next;
				if (resultScope) {
					if (resultScope->kind == XS_ALIAS_KIND)
						resultInstance = the->aliasArray[resultScope->value.alias];
					else if (resultScope->kind == XS_REFERENCE_KIND) 
						resultInstance = resultScope->value.reference;
					else
						goto exit_accelerator;
				#ifdef mxDebug
					if (resultInstance->kind != XS_INSTANCE_KIND)
						goto exit_accelerator;
				#endif
					if (resultInstance->flag & XS_SANDBOX_FLAG) {
						resultInstance = resultInstance->value.instance.prototype;
						resultFlag = XS_DONT_SCRIPT_FLAG;
					}	
					else
						resultFlag = frameFlag;
					goto putProperty;
				}
			}
			goto exit_accelerator;
		}
		
		mxCase(XS_GET_FOR_NEW)
			if (!resultScope) {
				mxImmediateS2(the_code, 1, anIndex);
				resultScope = the->scope;
				if (resultScope->kind == XS_ALIAS_KIND)
					resultInstance = the->aliasArray[resultScope->value.alias];
				else if (resultScope->kind == XS_REFERENCE_KIND) 
					resultInstance = resultScope->value.reference;
				else
					goto exit_accelerator;
			#ifdef mxDebug
				if (resultInstance->kind != XS_INSTANCE_KIND)
					goto exit_accelerator;
			#endif
				if (resultInstance->flag & XS_SANDBOX_FLAG) {
					resultInstance = resultInstance->value.instance.prototype;
					resultFlag = XS_DONT_SCRIPT_FLAG;
				}	
				else
					resultFlag = frameFlag;
				goto getProperty;
			}
#ifdef mxTrace
			if (gxDoTrace)
				fxTraceSymbol(the, anIndex);
#endif
			if (resultScope->flag & XS_THIS_FLAG) {
				mxInitSlot(--the_stack, resultScope->kind);
				the_stack->value = resultScope->value;
			}
			else
				mxZeroSlot(--the_stack);
			the_stack->ID = anIndex;
			mxZeroSlot(--the_stack);
			mxNextOpcode(the_code, 3, opcode);
			if (resultProperty->kind == XS_ACCESSOR_KIND) {
				goto accessor_kind_process;
			}
			else {
				mxInitSlot(the_stack, resultProperty->kind);
				the_stack->value = resultProperty->value;
				resultScope = C_NULL;
				*resProperty = resultProperty = C_NULL;
				mxBreak;
			}

		mxCase(XS_GET)
			if (!resultScope) {
				mxImmediateS2(the_code, 1, anIndex);
				resultScope = the->scope;
				if (resultScope->kind == XS_ALIAS_KIND)
					resultInstance = the->aliasArray[resultScope->value.alias];
				else if (resultScope->kind == XS_REFERENCE_KIND) 
					resultInstance = resultScope->value.reference;
				else
					goto exit_accelerator;
			#ifdef mxDebug
				if (resultInstance->kind != XS_INSTANCE_KIND)
					goto exit_accelerator;
			#endif
				if (resultInstance->flag & XS_SANDBOX_FLAG) {
					resultInstance = resultInstance->value.instance.prototype;
					resultFlag = XS_DONT_SCRIPT_FLAG;
				}	
				else
					resultFlag = frameFlag;
				goto getProperty;
			}
#ifdef mxTrace
			if (gxDoTrace)
				fxTraceSymbol(the, anIndex);
#endif
			mxZeroSlot(--the_stack);
			mxNextOpcode(the_code, 3, opcode);
			if (resultProperty->kind == XS_ACCESSOR_KIND) {
				goto accessor_kind_process;
			}
			else {
				mxInitSlot(the_stack, resultProperty->kind);
				the_stack->value = resultProperty->value;
				resultScope = C_NULL;
				*resProperty = resultProperty = C_NULL;
				mxBreak;
			}

		
		mxCase(XS_SET)
			if (!resultScope) {
				mxImmediateS2(the_code, 1, anIndex);
				resultScope = the->scope;
				if (resultScope->kind == XS_ALIAS_KIND)
					resultInstance = the->aliasArray[resultScope->value.alias];
				else if (resultScope->kind == XS_REFERENCE_KIND) 
					resultInstance = resultScope->value.reference;
				else
					goto exit_accelerator;
			#ifdef mxDebug
				if (resultInstance->kind != XS_INSTANCE_KIND)
					goto exit_accelerator;
			#endif
				if (resultInstance->flag & XS_SHARED_FLAG) 
					goto exit_accelerator;
				if (resultInstance->flag & XS_SANDBOX_FLAG) {
					resultInstance = resultInstance->value.instance.prototype;
					resultFlag = XS_DONT_SCRIPT_FLAG;
				}	
				else
					resultFlag = frameFlag;
				goto putProperty;
			}
			if (resultProperty->kind == XS_ACCESSOR_KIND) {
				*resProperty = resultProperty;
				goto exit_accelerator;
			}
			if (resultProperty->flag & XS_DONT_SET_FLAG) goto exit_accelerator;
#ifdef mxTrace
			if (gxDoTrace)
				fxTraceSymbol(the, anIndex);
#endif
			resultProperty->kind = the_stack->kind;
			resultProperty->value = the_stack->value;
			resultScope = C_NULL;
			*resProperty = resultProperty = C_NULL;
			mxNextOpcode(the_code, 3, opcode);
			mxBreak;
		
		mxCase(XS_GET_MEMBER_FOR_CALL)
			if (!resultProperty) {
				mxGetInstance;
				goto getProperty;
			}
			if (resultProperty->kind == XS_ACCESSOR_KIND) {
				*resProperty = resultProperty;
				goto exit_accelerator;
			}
#ifdef mxTrace
			if (gxDoTrace)
				fxTraceSymbol(the, anIndex);
#endif
			the_stack->ID = anIndex;
			--the_stack;
			mxNextOpcode(the_code, 3, opcode);
			if (resultProperty->kind == XS_ACCESSOR_KIND) {
				goto accessor_kind_process;
			}
			else {
				mxInitSlot(the_stack, resultProperty->kind);
				the_stack->value = resultProperty->value;
				*resProperty = resultProperty = C_NULL;
				mxBreak;
			}

		mxCase(XS_GET_MEMBER)
			if (!resultProperty) {
				mxGetInstance;
				goto getProperty;
			}
#ifdef mxTrace
			if (gxDoTrace)
				fxTraceSymbol(the, anIndex);
#endif
			mxNextOpcode(the_code, 3, opcode);
			if (resultProperty->kind == XS_ACCESSOR_KIND) {
				goto accessor_kind_process;
			}
			else {
				mxInitSlot(the_stack, resultProperty->kind);
				the_stack->value = resultProperty->value;
				*resProperty = resultProperty = C_NULL;
				mxBreak;
			}

		mxCase(XS_SET_MEMBER)
			if (!resultProperty) {
				mxImmediateS2(the_code, 1, anIndex);
	
				if (the_stack->kind == XS_ALIAS_KIND)
					resultInstance = the->aliasArray[the_stack->value.alias];
				else if (the_stack->kind == XS_REFERENCE_KIND) 
					resultInstance = the_stack->value.reference;
				else
				{
					goto exit_accelerator;
				}
			#ifdef mxDebug
				if (resultInstance->kind != XS_INSTANCE_KIND)
					goto exit_accelerator;
			#endif
				if (resultInstance->flag & XS_SHARED_FLAG) 
				{
					goto exit_accelerator;
				}
				if(resultInstance->flag & XS_SANDBOX_FLAG) {
					resultInstance = resultInstance->value.instance.prototype;
					resultFlag = XS_DONT_SCRIPT_FLAG;
				}
				else
					resultFlag = frameFlag;
				goto putProperty;
			}
			if (resultProperty->kind == XS_ACCESSOR_KIND) {
				*resProperty = resultProperty;
				goto exit_accelerator;
			}
			if (resultProperty->flag & XS_DONT_SET_FLAG) goto exit_accelerator;
#ifdef mxTrace
			if (gxDoTrace)
				fxTraceSymbol(the, anIndex);
#endif
			the_stack++;
			resultProperty->kind = the_stack->kind;
			resultProperty->value = the_stack->value;
			*resProperty = resultProperty = C_NULL;
			mxNextOpcode(the_code, 3, opcode);
			mxBreak;
		
		mxCase(XS_BEGIN) {
			txFlag aFlag;
			txSlot* aScope;
			txInteger aParameterCount;
			txInteger aLocalCount;
			txSlot* aParameter;
			txSlot* aProperty;
			txSlot*	anInstance;
			txByte aLength;

			mxImmediateS2(the_code, 1, anIndex);
			aFlag = *(the_code + 3);
			if (aFlag & (XS_EVAL_FLAG | XS_PROGRAM_FLAG | XS_THIS_FLAG))
				goto exit_accelerator;
			the_code += 4;
#ifdef mxProfile
			fxBeginFunction(the);
#endif
			aProperty = the_stack + 2;	
			aProperty->ID = anIndex;
			aProperty->flag = XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG;
			aProperty->next = C_NULL;

			mxInitSlot(--the_stack, XS_ROUTE_KIND);
			the_stack->ID = XS_NO_ID;
			*theRoute = the_stack;

			the->frame->flag |= aFlag & (XS_SANDBOX_FLAG | XS_STRICT_FLAG);
			
			mxZeroSlot(--the_stack);
			mxZeroSlot(--the_stack);
			mxZeroSlot(--the_stack);
					
			if (!(the->frame->flag & XS_STRICT_FLAG)) {
				anInstance = mxThis;
				if (!mxIsReference(anInstance)) {
					if ((anInstance->kind == XS_UNDEFINED_KIND) || (anInstance->kind == XS_NULL_KIND))
						*mxThis = mxGlobal;
					else{
						fxToInstance(the, mxThis);
					}
				}
			}
			aScope = aProperty->value.reference->next->next; // function > instance > closure
			if (aScope->kind == XS_REFERENCE_KIND)
				aScope = aScope->value.reference->next; // closure > instance > scope
			else
				aScope = &mxGlobal;
			aParameter = the->frame + 4;
			aParameter->flag = XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG;
			aParameterCount = (txID)aParameter->value.integer;
			aParameter += aParameterCount;
			
			aLocalCount = *the_code++;
			if (aParameterCount <= aLocalCount) {
				aLocalCount -= aParameterCount;
				while (aParameterCount) {
					aProperty->next = aParameter;
					aProperty = aProperty->next;
					mxImmediateU2(the_code, 0, aProperty->ID);
					the_code += 2;
					aProperty->flag = XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG;
#ifdef mxTrace
					if (gxDoTrace)
						fxTraceSymbol(the, aProperty->ID);
#endif
					aParameterCount--;
					aParameter--;
				}
				while (aLocalCount) {
					mxZeroSlot(--the_stack);
					aProperty->next = the_stack;
					aProperty = aProperty->next;
					mxImmediateU2(the_code, 0, aProperty->ID);
					the_code += 2;
					aProperty->flag = XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG;
#ifdef mxTrace
					if (gxDoTrace)
						fxTraceSymbol(the, aProperty->ID);
#endif
					aLocalCount--;
				}
			} else {
				aParameterCount -= aLocalCount;
				while (aLocalCount) {
					aProperty->next = aParameter;
					aProperty = aProperty->next;
					mxImmediateU2(the_code, 0, aProperty->ID);
					the_code += 2;
					aProperty->flag = XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG;
#ifdef mxTrace
					if (gxDoTrace)
						fxTraceSymbol(the, aProperty->ID);
#endif
					aLocalCount--;
					aParameter--;
				}
				while (aParameterCount) {
					aParameter->ID = XS_NO_ID;
					aParameter->flag = XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG;
					aParameterCount--;
					aParameter--;
				}
			}
			aLocalCount = *the_code++;
			while (aLocalCount) {
				mxZeroSlot(--the_stack);
				aProperty->next = the_stack;
				aProperty = aProperty->next;
				mxImmediateU2(the_code, 0, aProperty->ID);
				the_code += 2;
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

			mxNextOpcode(the_code, 0, opcode);
			aCount = (txByte)mxArgc;
			aLength = *(mxFunction->value.reference->next->value.code + 4);
			aOffset = (aLength <= aCount)? 4:(aLength - aCount) + 4;
			frameFlag = (the->frame->flag & XS_SANDBOX_FLAG)?XS_DONT_SCRIPT_FLAG:XS_SANDBOX_FLAG;
			mxBreak;		
		}
		mxCase(XS_ROUTE) {
			txSlot* newRoute = mxRoute;

			if (theJump) {
				theJump->frame = the->frame;
				theJump->stack = the_stack;
				theJump->scope = the->scope;
			}
			*theRoute = newRoute;
			mxImmediateS2(the_code, 1, anIndex);
			mxNextOpcode(the_code, 3, opcode);
			newRoute->value.route.code = the_code + anIndex;
			newRoute->value.route.stack = the_stack;
			mxBreak;
		}
		mxCase(XS_ROUTE2) {
			txSlot* newRoute = mxRoute;

			if (theJump) {
				theJump->frame = the->frame;
				theJump->stack = the_stack;
				theJump->scope = the->scope;
			}
			*theRoute = newRoute;
			mxImmediateS4(the_code, 1, anOffset);
			mxNextOpcode(the_code, 5, opcode);
			newRoute->value.route.code = the_code + anOffset;
			newRoute->value.route.stack = the_stack;
			mxBreak;
		}
		
		mxCase(XS_END) {
			txSlot* aProperty;
			txSlot*	anInstance;
			txByte aLength;
			
			if (((the->frame - 2)->kind == XS_REFERENCE_KIND) && !((the->frame - 2)->value.reference->flag & XS_STACK_FLAG))
				goto exit_accelerator;
			if (!the->frame->next || (the->frame->next->flag & XS_C_FLAG))
				goto exit_accelerator;

			aProperty = mxParameters;
			if (aProperty->kind != XS_UNDEFINED_KIND) {
				anIndex = (txID)((the->frame + 4)->value.integer);
				aProperty = aProperty->value.reference->next;
				while (aProperty) {
					if ((aProperty->kind == XS_ACCESSOR_KIND) && (0 <= aProperty->ID) && (aProperty->ID < anIndex)) {
						anInstance = the->frame + 4 + anIndex - aProperty->ID;
						aProperty->kind = anInstance->kind;
						aProperty->value = anInstance->value;
					}
					aProperty = aProperty->next;
				}
			}

#ifdef mxProfile
			fxEndFunction(the);
#endif
			if (the->frame->flag & XS_THROW_FLAG) {
				the->scope = the->frame->value.frame.scope;
				the->frame = the->frame->next;
				*theRoute = mxRoute;
				the->frame->flag |= XS_THROW_FLAG;
				the_code = (*theRoute)->value.route.code;
				the_stack = (*theRoute)->value.route.stack;
			}
			else {
				the_stack = mxArgv(-1);
				the->scope = the->frame->value.frame.scope;
				the_code = the->frame->value.frame.code;
				*(--the_stack) = *mxResult;
				the->frame = the->frame->next;
				*theRoute = mxRoute;
				if (theJump) {
					theJump->stack = the_stack;
					theJump->scope = the->scope;
					theJump->frame = the->frame;
				}
			}
			aCount = (txByte)mxArgc;
			aLength = *(mxFunction->value.reference->next->value.code + 4);
			aOffset = (aLength <= aCount)? 4:(aLength - aCount) + 4;
			frameFlag = (the->frame->flag & XS_SANDBOX_FLAG)?XS_DONT_SCRIPT_FLAG:XS_SANDBOX_FLAG;
			mxNextOpcode(the_code, 0, opcode);
			mxBreak;
		}

		mxCase(XS_BRANCH)
			mxImmediateS2(the_code, 1, anIndex);
			the_code += 3;
			mxNextOpcode(the_code, anIndex, opcode);
			mxBreak;

		mxCase(XS_BRANCH2)
			mxImmediateS4(the_code, 1, anOffset);
			the_code += 5;
			mxNextOpcode(the_code, anOffset, opcode);
			mxBreak;

		mxCase(XS_BRANCH_ELSE_BOOL)
		{
			mxImmediateS2(the_code, 1, anIndex);
			mxNextOpcode(the_code, 3, opcode);
			if(the_stack->value.integer==0)
				mxNextOpcode(the_code, anIndex, opcode);
			the_stack++;
			mxBreak;
		}

		mxCase(XS_BRANCH_ELSE) {
			txInteger results; 
			txInteger kind; 
			const txInteger kindList =	(1 << XS_UNDEFINED_KIND)
									 |	(1 << XS_NULL_KIND)
									 |	(1 << XS_BOOLEAN_KIND)
									 |	(1 << XS_INTEGER_KIND)
									 |	(1 << XS_REFERENCE_KIND); 

			kind = the_stack->kind;
			mxImmediateS2(the_code, 1, anIndex);
			if ((((kindList) >> kind) & 1) != 0) {
				mxNextOpcode(the_code, 3, opcode);
				results = (0 << XS_UNDEFINED_KIND) | (0 << XS_NULL_KIND) | (1 << XS_REFERENCE_KIND);
				if (the_stack->value.integer != 0)
					results |= (1 << XS_BOOLEAN_KIND) | (1 << XS_INTEGER_KIND);
				the_stack++;
				if (((results >> kind) & 1) == 0)
					mxNextOpcode(the_code, anIndex, opcode);
				mxBreak;
			} 
			else if (kind == XS_STRING_KIND) {
				mxNextOpcode(the_code, 3, opcode);
				if (*the_stack->value.string == 0)
					mxNextOpcode(the_code, anIndex, opcode);
				the_stack++;
				mxBreak;
			} 
			else
				goto exit_accelerator;
		}

		mxCase(XS_BRANCH_ELSE_BOOL2) 
		{
			mxImmediateS4(the_code, 1, anOffset);
			mxNextOpcode(the_code, 5, opcode);
			if(the_stack->value.integer==0)
				mxNextOpcode(the_code, anOffset, opcode);
			the_stack++;
			mxBreak;
		}

		mxCase(XS_BRANCH_ELSE2) {
			txInteger results; 
			txInteger kind; 
			const txInteger kindList =	(1 << XS_UNDEFINED_KIND)
									 |	(1 << XS_NULL_KIND)
									 |	(1 << XS_BOOLEAN_KIND)
									 |	(1 << XS_INTEGER_KIND)
									 |	(1 << XS_REFERENCE_KIND); 

			kind = the_stack->kind;
			mxImmediateS4(the_code, 1, anOffset);
			if ((((kindList) >> kind) & 1) != 0) {
				mxNextOpcode(the_code, 5, opcode);
				results = (0 << XS_UNDEFINED_KIND) | (0 << XS_NULL_KIND) | (1 << XS_REFERENCE_KIND);
				if (the_stack->value.integer != 0)
					results |= (1 << XS_BOOLEAN_KIND) | (1 << XS_INTEGER_KIND);
				the_stack++;
				if (((results >> kind) & 1) == 0)
					mxNextOpcode(the_code, anOffset, opcode);
				mxBreak;
			} 
			else if (kind == XS_STRING_KIND) {
				mxNextOpcode(the_code, 5, opcode);
				if (*the_stack->value.string == 0)
					mxNextOpcode(the_code, anOffset, opcode);
				the_stack++;
				mxBreak;
			} 
			else
				goto exit_accelerator;
		}

		mxCase(XS_BRANCH_IF_BOOL)
		{
			mxImmediateS2(the_code, 1, anIndex);
			mxNextOpcode(the_code, 3, opcode);
			if(the_stack->value.integer!=0)
				mxNextOpcode(the_code, anIndex, opcode);
			the_stack++;
			mxBreak;
		}

		mxCase(XS_BRANCH_IF) {
			txInteger results; 
			txInteger kind; 
			const txInteger kindList =	(1 << XS_UNDEFINED_KIND)
									 |	(1 << XS_NULL_KIND)
									 |	(1 << XS_BOOLEAN_KIND)
									 |	(1 << XS_INTEGER_KIND)
									 |	(1 << XS_REFERENCE_KIND); 

			kind = the_stack->kind;
			mxImmediateS2(the_code, 1, anIndex);
			if ((((kindList) >> kind) & 1) != 0) {
				mxNextOpcode(the_code, 3, opcode);
				results = (0 << XS_UNDEFINED_KIND) | (0 << XS_NULL_KIND) | (1 << XS_REFERENCE_KIND);
				if (the_stack->value.integer != 0)
					results |= (1 << XS_BOOLEAN_KIND) | (1 << XS_INTEGER_KIND);
				the_stack++;
				if (((results >> kind) & 1) != 0)
					mxNextOpcode(the_code, anIndex, opcode);
				mxBreak;
			} 
			else if (kind == XS_STRING_KIND) {
				mxNextOpcode(the_code, 3, opcode);
				if (*the_stack->value.string != 0)
					mxNextOpcode(the_code, anIndex, opcode);
				the_stack++;
				mxBreak;
			} 
			else
				goto exit_accelerator;
		}

		mxCase(XS_BRANCH_IF_BOOL2) 
		{
			mxImmediateS4(the_code, 1, anOffset);
			mxNextOpcode(the_code, 5, opcode);
			if(the_stack->value.integer!=0)
				mxNextOpcode(the_code, anOffset, opcode);
			the_stack++;
			mxBreak;
		}

		mxCase(XS_BRANCH_IF2) {
			txInteger results; 
			txInteger kind; 
			const txInteger kindList =	(1 << XS_UNDEFINED_KIND)
									 |	(1 << XS_NULL_KIND)
									 |	(1 << XS_BOOLEAN_KIND)
									 |	(1 << XS_INTEGER_KIND)
									 |	(1 << XS_REFERENCE_KIND); 

			kind = the_stack->kind;
			mxImmediateS4(the_code, 1, anOffset);
			if ((((kindList) >> kind) & 1) != 0) {
				mxNextOpcode(the_code, 5, opcode);
				results = (0 << XS_UNDEFINED_KIND) | (0 << XS_NULL_KIND) | (1 << XS_REFERENCE_KIND);
				if (the_stack->value.integer != 0)
					results |= (1 << XS_BOOLEAN_KIND) | (1 << XS_INTEGER_KIND);
				the_stack++;
				if (((results >> kind) & 1) != 0)
					mxNextOpcode(the_code, anOffset, opcode);
				mxBreak;
			} 
			else if (kind == XS_STRING_KIND) {
				mxNextOpcode(the_code, 5, opcode);
				if (*the_stack->value.string != 0)
					mxNextOpcode(the_code, anOffset, opcode);
				the_stack++;
				mxBreak;
			} 
			else
				goto exit_accelerator;
		}

		mxCase(XS_BREAK)
			goto exit_accelerator;

		mxCase(XS_DEBUGGER)
		#ifdef mxDebug
			goto exit_accelerator;
		#endif
			mxNextOpcode(the_code, 1, opcode);
			mxBreak;

		mxCase(XS_DUB)
			the_stack--;
			*(the_stack) = *(the_stack + 1);
			mxNextOpcode(the_code, 1, opcode);
			mxBreak;

		mxCase(XS_FILE)
			mxImmediateU2(the_code, 1, (*theRoute)->ID);
			mxGetSymbol(the, (*theRoute)->ID, (*theRoute)->next);
			(*theRoute)->ID = 0;
			mxNextOpcode(the_code, 3, opcode);
			mxBreak;


		mxCase(XS_GLOBAL)
			mxNextOpcode(the_code, 1, opcode);
			*(--the_stack) = the->stackTop[-1];
			mxBreak;


		mxCase(XS_LINE)
		#ifdef mxDebug
			if ((the->frame) && (the->frame - 1) && ((the->frame - 1)->next)) {
				if (mxBreakpoints.value.list.first) goto exit_accelerator;
				if ((the->frame->flag & XS_STEP_OVER_FLAG)) goto exit_accelerator;
			}
		#endif
			mxImmediateU2(the_code, 1, (*theRoute)->ID);
			mxNextOpcode(the_code, 3, opcode);
			mxBreak;

		mxCase(PSEUDO_DUB)
			*the_stack = *(the_stack+1);
			mxNextOpcode(the_code, 1, opcode);
			mxBreak;

		mxCase(XS_POP)
			mxNextOpcode(the_code, 1, opcode);
			the_stack++;
			mxBreak;

		mxCase(XS_RESULT)
			mxNextOpcode(the_code, 1, opcode);
		    *mxResult = *(the_stack);
			mxBreak;
			
		mxCase(XS_SWAP) {
			txSlot aScratch;
			aScratch = *(the_stack);
			*(the_stack) = *(the_stack + 1);
			*(the_stack + 1) = aScratch;
			mxNextOpcode(the_code, 1, opcode);
			mxBreak;
		}

		mxCase(XS_THIS)
			*(--the_stack) = *mxThis;
			mxNextOpcode(the_code, 1, opcode);
			mxBreak;
		
		mxCase(XS_CALL) {
			txSlot* aFunction;
			txByte aLength;

			if (the_stack->kind != XS_REFERENCE_KIND) goto exit_accelerator;
			aFunction = the_stack->value.reference;
			if (!(aFunction->flag & XS_VALUE_FLAG)) goto exit_accelerator;
			if (!((aFunction->next->kind == XS_CALLBACK_KIND) || (aFunction->next->kind == XS_CODE_KIND))) goto exit_accelerator;
			the_code++;
			/* RESULT */
			mxZeroSlot(--the_stack);
			/* FRAME */
			mxInitSlot(--the_stack, XS_FRAME_KIND);
			the_stack->next = the->frame;
			the_stack->ID = (the_stack + 3)->ID;
			the_stack->flag = XS_NO_FLAG;
#ifdef mxDebug
			if (the->frame && (the->frame->flag & XS_STEP_INTO_FLAG))
				the_stack->flag |= XS_STEP_INTO_FLAG | XS_STEP_OVER_FLAG;
#endif
			the_stack->value.frame.code = the_code;
			the_stack->value.frame.scope = the->scope;
			if (aFunction->next->kind == XS_CALLBACK_KIND) {
				the_stack->flag |= XS_C_FLAG;
				the->frame = the_stack;
				the->scope = C_NULL;
				the->code = aFunction->next->next->value.closure.symbolMap;
				mxInitSlot(--the_stack, XS_INTEGER_KIND);
				the->stack = the_stack;
#ifdef mxProfile
				fxDoCallback(the, aFunction->next->value.callback.address);
#else
				(*(aFunction->next->value.callback.address))(the);
#endif
				the_stack = mxArgv(-1);
				the->scope = the->frame->value.frame.scope;
				the_code = the->frame->value.frame.code;
				*(--the_stack) = *mxResult;
				the->frame = the->frame->next;
			}
			else {
				if (the->status == XS_THROW_STATUS) {
					the->status = XS_NO_STATUS;
					the_stack->flag |= XS_THROW_FLAG;
				}
				the->frame = the_stack;
				the_code = aFunction->next->value.code;
				aCount = (txByte)mxArgc;
				aLength = *(mxFunction->value.reference->next->value.code + 4);
				aOffset = (aLength <= aCount)? 4:(aLength - aCount) + 4;
				frameFlag = (the->frame->flag & XS_SANDBOX_FLAG)?XS_DONT_SCRIPT_FLAG:XS_SANDBOX_FLAG;
			}
			mxNextOpcode(the_code, 0, opcode);
			mxBreak;
		}

		mxCase(PSEUDO_DOUBLE_GET_NEGATIVE)
		{
			txSlot *aProperty0,*aProperty1;
			PSEUDO_PART_GET_NEGATIVE_ID2(aProperty0,aProperty1);
			if((!aProperty0) || (!aProperty1)) goto exit_accelerator;
			if(aProperty0->kind == XS_ACCESSOR_KIND) goto exit_accelerator;
			mxInitSlot(--the_stack, aProperty0->kind);
			the_stack->value = aProperty0->value;
			if(aProperty1->kind == XS_ACCESSOR_KIND) {
				the_code += 2;
				goto exit_accelerator;
			}
			mxInitSlot(--the_stack, aProperty1->kind);
			the_stack->value = aProperty1->value;

			mxNextOpcode(the_code, 4, opcode);
			mxBreak;
		}

		mxCase(XS_GET_NEGATIVE_ID)
		{
			txByte theIndex = *(the_code+1);
			txSlot* aProperty;
			aProperty = the->frame + theIndex - aOffset;
			if (!aProperty) goto exit_accelerator;
			if (aProperty->kind == XS_ACCESSOR_KIND) goto exit_accelerator;
			mxNextOpcode(the_code, 2, opcode);
			if ((opcode == XS_CALL) || (opcode == XS_NEW)) {
				mxZeroSlot(--the_stack);
				the_stack->ID = XS_NO_ID;
			}
			mxInitSlot(--the_stack, aProperty->kind);
			the_stack->value = aProperty->value;
			mxBreak;
		}

		mxCase(PSEUDO_DOUBLE_GET_AT) 
		{
			txByte theIndex0 = *(the_code+1);
			txByte theIndex1 = *(the_code+3);
			txSlot* aProperty0,*aProperty1;
			if(theIndex0>=0) 
				aProperty0 = the->frame + aCount - theIndex0 + ((theIndex0 < aCount)?4:(- 5));
			else 
				aProperty0 = the->frame + theIndex0 - aOffset;

			if(theIndex1>=0) 
				aProperty1 = the->frame + aCount - theIndex1 + ((theIndex1 < aCount)?4:(- 5));
			else 
				aProperty1 = the->frame + theIndex1 - aOffset;

			if((!aProperty0) || (!aProperty1)) goto exit_accelerator;
			if (aProperty0->kind == XS_ACCESSOR_KIND) goto exit_accelerator;
			mxInitSlot(--the_stack, aProperty0->kind);
			the_stack->value = aProperty0->value;
			if (aProperty1->kind == XS_ACCESSOR_KIND) {
				the_code += 2;
				goto exit_accelerator;
			}
			mxNextOpcode(the_code, 4, opcode);
			mxInitSlot(--the_stack, aProperty1->kind);
			the_stack->value = aProperty1->value;
			mxBreak;
		}

		mxCase(XS_GET_AT) 
		{
			txByte theIndex = *(the_code+1);
			{
				txSlot* aProperty;
				if(theIndex>=0) 
					aProperty = the->frame + aCount - theIndex + ((theIndex < aCount)?4:(- 5));
				else 
					aProperty = the->frame + theIndex - aOffset;
			
				if(!aProperty) goto exit_accelerator;
				if (aProperty->kind == XS_ACCESSOR_KIND) goto exit_accelerator;

				mxNextOpcode(the_code, 2, opcode);
				if ((opcode == XS_CALL) || (opcode == XS_NEW)) {
					mxZeroSlot(--the_stack);
					the_stack->ID = XS_NO_ID;
				}
				mxInitSlot(--the_stack, aProperty->kind);
				the_stack->value = aProperty->value;
			}
			mxBreak;
		}

		mxCase(XS_SET_NEGATIVE_ID)
		{
			txByte theIndex = *(the_code+1);
			{
				txSlot* aProperty;
				aProperty = the->frame + theIndex - aOffset;
				if (!aProperty) goto exit_accelerator;
				if (aProperty->kind == XS_ACCESSOR_KIND) goto exit_accelerator;
				if (aProperty->flag & XS_DONT_SET_FLAG) {
					if (the->frame->flag & XS_STRICT_FLAG)
						goto exit_accelerator;
					the_stack->kind = aProperty->kind;
					the_stack->value = aProperty->value;
				}
				else {
					aProperty->kind = the_stack->kind;
					aProperty->value = the_stack->value;
				}
			}
			mxNextOpcode(the_code, 2, opcode);
			mxBreak;
		}

		mxCase(XS_SET_AT) 
		{
			txByte theIndex = *(the_code+1);
			{
				txSlot* aProperty;
				if(theIndex>=0) 
					aProperty = the->frame + aCount - theIndex + ((theIndex < aCount)?4:(- 5));
				else 
					aProperty = the->frame + theIndex - aOffset;
				
				if (!aProperty) goto exit_accelerator;
				if (aProperty->kind == XS_ACCESSOR_KIND) goto exit_accelerator;
				
				if (aProperty->flag & XS_DONT_SET_FLAG) {
					if (the->frame->flag & XS_STRICT_FLAG)
						goto exit_accelerator;
					the_stack->kind = aProperty->kind;
					the_stack->value = aProperty->value;
				}
				else {
					aProperty->kind = the_stack->kind;
					aProperty->value = the_stack->value;
				}
			}
			mxNextOpcode(the_code, 2, opcode);
			mxBreak;
		}

		mxCase(XS_GET_MEMBER_AT) {
			txInteger anInteger;
			txSlot* anInstance;
			txSlot* aProperty;
			if (the_stack->kind != XS_INTEGER_KIND) goto exit_accelerator;
			anInteger = the_stack->value.integer;
			if ((anInteger < 0) || (XS_MAX_INDEX < anInteger))
				goto exit_accelerator;
			anInstance = the_stack + 1;
			if (anInstance->kind == XS_ALIAS_KIND)
				anInstance = the->aliasArray[anInstance->value.alias];
			else if (anInstance->kind == XS_REFERENCE_KIND) 
				anInstance = anInstance->value.reference;
			else
				goto exit_accelerator;
			if (!(anInstance->flag & XS_VALUE_FLAG))
				goto exit_accelerator;
			aProperty = anInstance->next;
			if (aProperty->kind != XS_ARRAY_KIND)
				goto exit_accelerator;
			if ((txIndex)anInteger >= aProperty->value.array.length) 
				goto exit_accelerator;
			aProperty = aProperty->value.array.address + anInteger;
			if (!aProperty->ID)
				goto exit_accelerator;
			if (aProperty->kind == XS_ACCESSOR_KIND) {
				*resProperty = resultProperty;
				goto exit_accelerator;
			}
#ifdef mxTrace
			if (gxDoTrace)
				fxTraceSymbol(the, anIndex);
#endif
			the_stack++;
			mxNextOpcode(the_code, 1, opcode);
			if ((opcode == XS_CALL) || (opcode == XS_NEW)) {
				the_stack->ID = anIndex;
				--the_stack;
			}
			mxInitSlot(the_stack, aProperty->kind);
			the_stack->value = aProperty->value;
			*resProperty = C_NULL;
			mxBreak;
		}

		mxCase(XS_SET_MEMBER_AT) {
			txInteger anInteger;
			txSlot* anInstance;
			txSlot* aProperty;
			if (the_stack->kind != XS_INTEGER_KIND) goto exit_accelerator;
			anInteger = the_stack->value.integer;
			if ((anInteger < 0) || (XS_MAX_INDEX < anInteger))
				goto exit_accelerator;
			anInstance = the_stack + 1;
			if (anInstance->kind == XS_ALIAS_KIND)
				anInstance = the->aliasArray[anInstance->value.alias];
			else if (anInstance->kind == XS_REFERENCE_KIND) 
				anInstance = anInstance->value.reference;
			else
				goto exit_accelerator;
			if (!(anInstance->flag & XS_VALUE_FLAG))
				goto exit_accelerator;
			aProperty = anInstance->next;
			if (aProperty->kind != XS_ARRAY_KIND)
				goto exit_accelerator;
			if ((txIndex)anInteger >= aProperty->value.array.length) 
				goto exit_accelerator;
			aProperty = aProperty->value.array.address + anInteger;
			if (!aProperty->ID)
				goto exit_accelerator;
			if (aProperty->kind == XS_ACCESSOR_KIND) {
				*resProperty = resultProperty;
				goto exit_accelerator;
			}
			if (aProperty->flag & XS_DONT_SET_FLAG) goto exit_accelerator;
#ifdef mxTrace
			if (gxDoTrace)
				fxTraceSymbol(the, anIndex);
#endif
			the_stack += 2;
			aProperty->kind = the_stack->kind;
			aProperty->value = the_stack->value;
			*resProperty = C_NULL;
			mxNextOpcode(the_code, 1, opcode);
			mxBreak;
		}
			
		mxCase(XS_UNDEFINED)
			mxZeroSlot(--the_stack);
			mxNextOpcode(the_code, 1, opcode);
			mxBreak; 

		mxCase(XS_NULL)
			mxInitSlot(--the_stack, XS_NULL_KIND);
			mxNextOpcode(the_code, 1, opcode);
			mxBreak; 

		mxCase(XS_FALSE)
			mxInitSlot(--the_stack, XS_BOOLEAN_KIND);
			mxNextOpcode(the_code, 1, opcode);
			mxBreak;

		mxCase(XS_TRUE)
			mxInitSlot(--the_stack, XS_BOOLEAN_KIND);
			the_stack->value.boolean = 1;
			mxNextOpcode(the_code, 1, opcode);
			mxBreak;

		mxCase(XS_INTEGER_8)
			mxInitSlot(--the_stack, XS_INTEGER_KIND);
			the_stack->value.integer = *(the_code+1);
			mxNextOpcode(the_code, 2, opcode);
			mxBreak;

		mxCase(XS_INTEGER_16)
			mxInitSlot(--the_stack, XS_INTEGER_KIND);
			mxImmediateS2(the_code, 1, anIndex);
			the_stack->value.integer = anIndex;
			mxNextOpcode(the_code, 3, opcode);
			mxBreak;

		mxCase(XS_INTEGER_32)
			mxInitSlot(--the_stack, XS_INTEGER_KIND);
			mxImmediateU4(the_code, 1, the_stack->value.integer);
			mxNextOpcode(the_code, 5, opcode);
			mxBreak;

		mxCase(XS_NUMBER)
			mxInitSlot(--the_stack, XS_NUMBER_KIND);
			the_code++;
			mxDecode8(the_code, the_stack->value.number);
			mxNextOpcode(the_code, 0, opcode);
			mxBreak;

		mxCase(XS_VOID)
			mxZeroSlot(the_stack);
			mxNextOpcode(the_code, 1, opcode);
			mxBreak;

			
		mxCase(XS_INCREMENT)
			if (the_stack->kind != XS_INTEGER_KIND) goto exit_accelerator;
			the_stack->value.integer++;
			mxNextOpcode(the_code, 1, opcode);
			mxBreak;
			
		mxCase(XS_DECREMENT)
			if (the_stack->kind != XS_INTEGER_KIND) goto exit_accelerator;
			the_stack->value.integer--;
			mxNextOpcode(the_code, 1, opcode);
			mxBreak;
			
		mxCase(XS_PLUS)
			if (the_stack->kind != XS_INTEGER_KIND) goto exit_accelerator;
			mxNextOpcode(the_code, 1, opcode);
			mxBreak;
			
		mxCase(XS_MINUS)
			if (the_stack->kind != XS_INTEGER_KIND) goto exit_accelerator;
			the_stack->value.integer = -the_stack->value.integer;
			mxNextOpcode(the_code, 1, opcode);
			mxBreak;
			
		mxCase(XS_BIT_NOT)
			if (the_stack->kind != XS_INTEGER_KIND) goto exit_accelerator;
			the_stack->value.integer = ~the_stack->value.integer;
			mxNextOpcode(the_code, 1, opcode);
			mxBreak;
			
		mxCase(XS_NOT) {
			txInteger kind; 
			txInteger results; 
			const txInteger kindList =	(1 << XS_UNDEFINED_KIND)
									 |	(1 << XS_NULL_KIND)
									 |	(1 << XS_REFERENCE_KIND)
									 |	(1 << XS_STRING_KIND); 

			kind = the_stack->kind;

			if (kind == XS_BOOLEAN_KIND) {
				the_stack->value.boolean = !the_stack->value.boolean;
				mxNextOpcode(the_code, 1, opcode);
				mxBreak;
			} else

			if ((((kindList) >> kind) & 1) != 0) {
				results = (1 << XS_UNDEFINED_KIND) | (1 << XS_NULL_KIND) |  (0 << XS_REFERENCE_KIND) | (0 << XS_STRING_KIND);
				if ((kind == XS_STRING_KIND) && (*the_stack->value.string == 0))
					results |= 1 << XS_STRING_KIND;
				the_stack->kind = XS_BOOLEAN_KIND;
				the_stack->value.boolean = (txBoolean) ((results >> kind) & 1);
				mxNextOpcode(the_code, 1, opcode);
				mxBreak;
			} else
			goto exit_accelerator;
		}
			
		mxCase(XS_MULTIPLY)
			/* different behavior than the non-accelerated version */
			/* accelerate 16*16 signed integer multiply, can't overflow */
			if ((the_stack+0)->kind != XS_INTEGER_KIND) goto exit_accelerator;
			if ((the_stack+1)->kind != XS_INTEGER_KIND) goto exit_accelerator;
			if ((the_stack+0)->value.integer != (txS2)(the_stack+0)->value.integer)
				goto exit_accelerator;
			if ((the_stack+1)->value.integer != (txS2)(the_stack+1)->value.integer)
				goto exit_accelerator;
			(the_stack+1)->value.integer *= (the_stack+0)->value.integer;
			the_stack++;
			mxNextOpcode(the_code, 1, opcode);
			mxBreak;
			
		mxCase(XS_ADD)
			if ((the_stack+0)->kind != XS_INTEGER_KIND) goto exit_accelerator;
			if ((the_stack+1)->kind != XS_INTEGER_KIND) goto exit_accelerator;
			(the_stack+1)->value.integer += (the_stack+0)->value.integer;
			the_stack++;
			mxNextOpcode(the_code, 1, opcode);
			mxBreak;
			
		mxCase(XS_SUBTRACT)
			if ((the_stack+0)->kind != XS_INTEGER_KIND) goto exit_accelerator;
			if ((the_stack+1)->kind != XS_INTEGER_KIND) goto exit_accelerator;
			(the_stack+1)->value.integer -= (the_stack+0)->value.integer;
			the_stack++;
			mxNextOpcode(the_code, 1, opcode);
			mxBreak;
			
		mxCase(XS_LEFT_SHIFT)
			if ((the_stack+0)->kind != XS_INTEGER_KIND) goto exit_accelerator;
			if ((the_stack+1)->kind != XS_INTEGER_KIND) goto exit_accelerator;
			(the_stack+1)->value.integer <<= (the_stack+0)->value.integer & 0x1f;
			the_stack++;
			mxNextOpcode(the_code, 1, opcode);
			mxBreak;
			
		mxCase(XS_SIGNED_RIGHT_SHIFT)
			if ((the_stack+0)->kind != XS_INTEGER_KIND) goto exit_accelerator;
			if ((the_stack+1)->kind != XS_INTEGER_KIND) goto exit_accelerator;
			(the_stack+1)->value.integer >>= (the_stack+0)->value.integer & 0x1f;
			the_stack++;
			mxNextOpcode(the_code, 1, opcode);
			mxBreak;
			
		mxCase(XS_UNSIGNED_RIGHT_SHIFT)
			if ((the_stack+0)->kind != XS_INTEGER_KIND) goto exit_accelerator;
			if ((the_stack+1)->kind != XS_INTEGER_KIND) goto exit_accelerator;
			//if (0 == (0x1f & (the_stack+0)->value.integer)) goto exit_accelerator;
			{
				txU4 temp = (txU4)((the_stack+1)->value.integer);
				temp >>= (the_stack+0)->value.integer & 0x1f;
				(the_stack+1)->value.integer = temp;
			}
			the_stack++;
			mxNextOpcode(the_code, 1, opcode);
			mxBreak;
			
		mxCase(XS_LESS) {
			txBoolean result; 

			if ((the_stack+0)->kind != XS_INTEGER_KIND) goto exit_accelerator;
			if ((the_stack+1)->kind != XS_INTEGER_KIND) goto exit_accelerator;
			result = (the_stack+1)->value.integer < (the_stack+0)->value.integer;
			the_stack++;
			mxInitSlot(the_stack, XS_BOOLEAN_KIND);
			the_stack->value.boolean = result;
			mxNextOpcode(the_code, 1, opcode);
			mxBreak;
		}

		mxCase(XS_LESS_EQUAL) {
			txBoolean result; 

			if ((the_stack+0)->kind != XS_INTEGER_KIND) goto exit_accelerator;
			if ((the_stack+1)->kind != XS_INTEGER_KIND) goto exit_accelerator;
			result = (the_stack+1)->value.integer <= (the_stack+0)->value.integer;
			the_stack++;
			mxInitSlot(the_stack, XS_BOOLEAN_KIND);
			the_stack->value.boolean = result;
			mxNextOpcode(the_code, 1, opcode);
			mxBreak;
		}

		mxCase(XS_MORE) {
			txBoolean result; 

			if ((the_stack+0)->kind != XS_INTEGER_KIND) goto exit_accelerator;
			if ((the_stack+1)->kind != XS_INTEGER_KIND) goto exit_accelerator;
			result = (the_stack+1)->value.integer > (the_stack+0)->value.integer;
			the_stack++;
			mxInitSlot(the_stack, XS_BOOLEAN_KIND);
			the_stack->value.boolean = result;
			mxNextOpcode(the_code, 1, opcode);
			mxBreak;
		}

		mxCase(XS_MORE_EQUAL) {
			txBoolean result; 

			if ((the_stack+0)->kind != XS_INTEGER_KIND) goto exit_accelerator;
			if ((the_stack+1)->kind != XS_INTEGER_KIND) goto exit_accelerator;
			result = (the_stack+1)->value.integer >= (the_stack+0)->value.integer;
			the_stack++;
			mxInitSlot(the_stack, XS_BOOLEAN_KIND);
			the_stack->value.boolean = result;
			mxNextOpcode(the_code, 1, opcode);
			mxBreak;
		}

		mxCase(XS_EQUAL) {
			txBoolean result;
			txKind kind = (the_stack+0)->kind;
			
			if (kind != (the_stack+1)->kind) {
				const txInteger kindList =	(1 << XS_UNDEFINED_KIND)
										|	(1 << XS_NULL_KIND);
				txInteger kinds = (1 << kind) | (1 << (the_stack+1)->kind);
				
				if (kinds & kindList)				// at least one operand is null or undefined
					result = kinds == kindList;		// equal if one is null and the other is undefined, else not equal
				else
					goto exit_accelerator;
			}
			else {
				if ((kind == XS_UNDEFINED_KIND) || (kind == XS_NULL_KIND))
					result = 1;
				else if (kind == XS_BOOLEAN_KIND)
					result = (the_stack+1)->value.boolean == (the_stack+0)->value.boolean;
				else if (kind == XS_INTEGER_KIND)
					result = (the_stack+1)->value.integer == (the_stack+0)->value.integer;
				else if (kind == XS_REFERENCE_KIND)
					result = (the_stack+1)->value.reference == (the_stack+0)->value.reference;
				else if (kind == XS_STRING_KIND) {
					char *a = (the_stack+0)->value.string, *b = (the_stack+1)->value.string;
					while (1) {
						char aa = *a++;
						if (aa != *b++) {
							result = 0;
							break;
						}
						if (!aa) {
							result = 1;
							break;
						}
					}
				}
				else if (kind == XS_ALIAS_KIND)
					result = the->aliasArray[(the_stack+1)->value.alias] == the->aliasArray[(the_stack+0)->value.alias];
				else
					goto exit_accelerator;
			}
			the_stack++;
			mxInitSlot(the_stack, XS_BOOLEAN_KIND);
			the_stack->value.boolean = result;
			mxNextOpcode(the_code, 1, opcode);
			mxBreak;
		}
			
		mxCase(XS_STRICT_EQUAL) {
			txBoolean result;
			txKind kind = (the_stack+0)->kind;
			if (kind != (the_stack+1)->kind) {
				if ((XS_INTEGER_KIND == kind) && (XS_NUMBER_KIND == (the_stack+1)->kind))
					goto exit_accelerator;
				if ((XS_NUMBER_KIND == kind) && (XS_INTEGER_KIND == (the_stack+1)->kind))
					goto exit_accelerator;

				result = 0;
			}
			else if ((kind == XS_UNDEFINED_KIND) || (kind == XS_NULL_KIND))
				result = 1;
			else if (kind == XS_BOOLEAN_KIND)
				result = (the_stack+1)->value.boolean == (the_stack+0)->value.boolean;
			else if (kind == XS_INTEGER_KIND)
				result = (the_stack+1)->value.integer == (the_stack+0)->value.integer;
			else if (kind == XS_REFERENCE_KIND)
				result = (the_stack+1)->value.reference == (the_stack+0)->value.reference;
			else if (kind == XS_STRING_KIND) {
				char *a = (the_stack+0)->value.string, *b = (the_stack+1)->value.string;
				while (1) {
					char aa = *a++;
					if (aa != *b++) {
						result = 0;
						break;
					}
					if (!aa) {
						result = 1;
						break;
					}
				}
			}
			else
				goto exit_accelerator;
			the_stack++;
			mxInitSlot(the_stack, XS_BOOLEAN_KIND);
			the_stack->value.boolean = result;
			mxNextOpcode(the_code, 1, opcode);
			mxBreak;
		}
			
		mxCase(XS_NOT_EQUAL) {
			txBoolean result;
			txKind kind = (the_stack+0)->kind;

			if (kind != (the_stack+1)->kind) {
				const txInteger kindList =	(1 << XS_UNDEFINED_KIND)
										|	(1 << XS_NULL_KIND);
				txInteger kinds = (1 << kind) | (1 << (the_stack+1)->kind);

				if (kinds & kindList)				// at least one operand is null or undefined
					result = kinds != kindList;		// equal if one is null and the other is undefined, else not equal
				else
					goto exit_accelerator;
			}
			else {
				if ((kind == XS_UNDEFINED_KIND) || (kind == XS_NULL_KIND))
					result = 0;
				else if (kind == XS_BOOLEAN_KIND)
					result = (the_stack+1)->value.boolean != (the_stack+0)->value.boolean;
				else if (kind == XS_INTEGER_KIND)
					result = (the_stack+1)->value.integer != (the_stack+0)->value.integer;
				else if (kind == XS_REFERENCE_KIND)
					result = (the_stack+1)->value.reference != (the_stack+0)->value.reference;
				else if (kind == XS_STRING_KIND) {
					char *a = (the_stack+0)->value.string, *b = (the_stack+1)->value.string;
					while (1) {
						char aa = *a++;
						if (aa != *b++) {
							result = 1;
							break;
						}
						if (!aa) {
							result = 0;
							break;
						}
					}
				}
				else if (kind == XS_ALIAS_KIND)
					result = the->aliasArray[(the_stack+1)->value.alias] != the->aliasArray[(the_stack+0)->value.alias];
				else
					goto exit_accelerator;
			}
			the_stack++;
			mxInitSlot(the_stack, XS_BOOLEAN_KIND);
			the_stack->value.boolean = result;
			mxNextOpcode(the_code, 1, opcode);
			mxBreak;
		}

		mxCase(XS_STRICT_NOT_EQUAL) {
			txBoolean result;
			txKind kind = (the_stack+0)->kind;

			if (kind != (the_stack+1)->kind) {
				if ((XS_INTEGER_KIND == kind) && (XS_NUMBER_KIND == (the_stack+1)->kind))
					goto exit_accelerator;
				if ((XS_NUMBER_KIND == kind) && (XS_INTEGER_KIND == (the_stack+1)->kind))
					goto exit_accelerator;
				result = 1;
			}
			else {
				if ((kind == XS_UNDEFINED_KIND) || (kind == XS_NULL_KIND))
					result = 0;
				else if (kind == XS_BOOLEAN_KIND)
					result = (the_stack+1)->value.boolean != (the_stack+0)->value.boolean;
				else if (kind == XS_INTEGER_KIND)
					result = (the_stack+1)->value.integer != (the_stack+0)->value.integer;
				else if (kind == XS_REFERENCE_KIND)
					result = (the_stack+1)->value.reference != (the_stack+0)->value.reference;
				else
					goto exit_accelerator;
			}
			the_stack++;
			mxInitSlot(the_stack, XS_BOOLEAN_KIND);
			the_stack->value.boolean = result;
			mxNextOpcode(the_code, 1, opcode);
			mxBreak;
		}
			
		mxCase(XS_BIT_AND)
			if ((the_stack+0)->kind != XS_INTEGER_KIND) goto exit_accelerator;
			if ((the_stack+1)->kind != XS_INTEGER_KIND) goto exit_accelerator;
			(the_stack+1)->value.integer &= (the_stack+0)->value.integer;
			the_stack++;
			mxNextOpcode(the_code, 1, opcode);
			mxBreak;
			
		mxCase(XS_BIT_OR)
			if ((the_stack+0)->kind != XS_INTEGER_KIND) goto exit_accelerator;
			if ((the_stack+1)->kind != XS_INTEGER_KIND) goto exit_accelerator;
			(the_stack+1)->value.integer |= (the_stack+0)->value.integer;
			the_stack++;
			mxNextOpcode(the_code, 1, opcode);
			mxBreak;
			
		mxCase(XS_BIT_XOR)
			if ((the_stack+0)->kind != XS_INTEGER_KIND) goto exit_accelerator;
			if ((the_stack+1)->kind != XS_INTEGER_KIND) goto exit_accelerator;
			(the_stack+1)->value.integer ^= (the_stack+0)->value.integer;
			the_stack++;
			mxNextOpcode(the_code, 1, opcode);
			mxBreak;


		mxCase(PSEUDO_INCREMENT)
		{
			txSlot* aProperty;
			PSEUDO_PART_GET_NEGATIVE_ID(aProperty);
			if (!aProperty) {
				goto exit_accelerator;
			}
			if (aProperty->kind == XS_ACCESSOR_KIND){
				goto exit_accelerator;
			}
			if (aProperty->kind != XS_INTEGER_KIND) goto exit_accelerator;
			if (aProperty->flag & XS_DONT_SET_FLAG)  goto exit_accelerator;
			aProperty->value.integer++;
			mxNextOpcode(the_code, 6, opcode);
			mxBreak;
		}

		mxCase(PSEUDO_DECREMENT)
		{
			txSlot* aProperty;
			PSEUDO_PART_GET_NEGATIVE_ID(aProperty);
			if (!aProperty) {
				goto exit_accelerator;
			}
			if (aProperty->kind == XS_ACCESSOR_KIND){
				goto exit_accelerator;
			}
			if (aProperty->kind != XS_INTEGER_KIND) goto exit_accelerator;
			if (aProperty->flag & XS_DONT_SET_FLAG)  goto exit_accelerator;
			aProperty->value.integer--;
			mxNextOpcode(the_code, 6, opcode);
			mxBreak;
		}

		mxCase(PSEUDO_LESS_EQUAL)
		{
			txSlot* aProperty0,*aProperty1;
			txBoolean result; 
			PSEUDO_PART_GET_NEGATIVE_ID2(aProperty0,aProperty1);
			if ((!aProperty0) || (!aProperty1)) goto exit_accelerator;
			if (aProperty0->kind == XS_ACCESSOR_KIND) goto exit_accelerator;
			if (aProperty1->kind == XS_ACCESSOR_KIND) goto exit_accelerator;
			the_code+=4;

			if (aProperty0->kind != XS_INTEGER_KIND || aProperty1->kind != XS_INTEGER_KIND) {
				mxInitSlot(--the_stack, aProperty0->kind);
				the_stack->value = aProperty0->value;
				mxInitSlot(--the_stack, aProperty1->kind);
				the_stack->value = aProperty1->value;
				goto exit_accelerator;
			}
			result = aProperty0->value.integer <= aProperty1->value.integer;
			the_code++;

			mxImmediateS2(the_code, 1, anIndex);
			mxNextOpcode(the_code, 3, opcode);
			if(result==0)
				mxNextOpcode(the_code, anIndex, opcode);
			mxBreak;
		}

		mxCase(PSEUDO_LESS)
		{
			txSlot* aProperty0,*aProperty1;
			txBoolean result; 
			PSEUDO_PART_GET_NEGATIVE_ID2(aProperty0,aProperty1);
			if ((!aProperty0) || (!aProperty1)) goto exit_accelerator;
			if (aProperty0->kind == XS_ACCESSOR_KIND) goto exit_accelerator;
			if (aProperty1->kind == XS_ACCESSOR_KIND) goto exit_accelerator;
			the_code+=4;

			if (aProperty0->kind != XS_INTEGER_KIND || aProperty1->kind != XS_INTEGER_KIND) {
				mxInitSlot(--the_stack, aProperty0->kind);
				the_stack->value = aProperty0->value;
				mxInitSlot(--the_stack, aProperty1->kind);
				the_stack->value = aProperty1->value;
				goto exit_accelerator;
			}
			result = aProperty0->value.integer < aProperty1->value.integer;
			the_code++;

			mxImmediateS2(the_code, 1, anIndex);
			mxNextOpcode(the_code, 3, opcode);
			if(result==0)
				mxNextOpcode(the_code, anIndex, opcode);
			mxBreak;
		}

		mxCase(PSEUDO_SET_MEMBER)
		{
			txKind kind;
			mxImmediateS2(the_code, 2, anIndex);
			kind = mxThis->kind;
			if (XS_ALIAS_KIND == kind){
				resultInstance = the->aliasArray[mxThis->value.alias];
			}
			else if (XS_REFERENCE_KIND == kind) {
				resultInstance = mxThis->value.reference;
			}
			else {
				*(--the_stack) = *mxThis;
				the_code++;
				goto exit_accelerator;
			}
			if ((resultInstance->flag & XS_SHARED_FLAG)) {
				*(--the_stack) = *mxThis;
				the_code++;
				goto exit_accelerator;
			}
			if(resultInstance->flag & XS_SANDBOX_FLAG) {
				resultInstance = resultInstance->value.instance.prototype;
				resultFlag = XS_DONT_SCRIPT_FLAG;
			}
			else
				resultFlag = frameFlag;
			resultProperty = resultInstance->next;
			if (resultInstance->flag & XS_VALUE_FLAG) {
				if (anIndex < 0) {
					if (resultProperty->kind == XS_GLOBAL_KIND) {
					#ifdef mxDebug
						if ((anIndex & 0x7FFF) >= the->symbolCount) {
							*(--the_stack) = *mxThis;
							the_code++;
							goto exit_accelerator;
						}
					#endif
						resultProperty = (resultFlag & XS_SANDBOX_FLAG)?resultProperty->value.global.cache[anIndex & 0x7FFF]:resultProperty->value.global.sandboxCache[anIndex & 0x7FFF];
						if (!resultProperty) {
							*(--the_stack) = *mxThis;
							the_code++;
							goto exit_accelerator;
						}
						goto gotPropertyInSet;
					}
				}
				else {
					if (resultProperty->kind == XS_ARRAY_KIND) {
						if ((txIndex)anIndex >= resultProperty->value.array.length) {
							*(--the_stack) = *mxThis;
							the_code++;
							goto exit_accelerator;
						}
						resultProperty = resultProperty->value.array.address + anIndex;
						resultProperty->ID = XS_NO_ID;
						goto gotPropertyInSet;
					}
				}
			}
			
			while (resultProperty) {
				if (resultProperty->ID == anIndex) {
					if (!(resultProperty->flag & resultFlag)) {
						goto gotPropertyInSet;
					}
				}
				resultProperty = resultProperty->next;
			}
			*(--the_stack) = *mxThis;
			the_code++;
			goto exit_accelerator;

			
			gotPropertyInSet:
			if (resultProperty->kind == XS_ACCESSOR_KIND) {
				*(--the_stack) = *mxThis;
				the_code++;
				*resProperty = resultProperty;
				goto exit_accelerator;
			}
			if (resultProperty->flag & XS_DONT_SET_FLAG) goto exit_accelerator;
			
			resultProperty->kind = the_stack->kind;
			resultProperty->value = the_stack->value;
			the_stack++;
			
			*resProperty = resultProperty = C_NULL;
			mxNextOpcode(the_code, 5, opcode);
			mxBreak;
		}

		mxCase(PSEUDO_BRANCH_ELSE0)
		{
			txSlot* aProperty;
			PSEUDO_PART_GET_NEGATIVE_ID(aProperty);
			if (!aProperty) {
				goto exit_accelerator;
			}
			if (aProperty->kind == XS_ACCESSOR_KIND){
				goto exit_accelerator;
			}
			the_code+=2;
			PSEUDO_PART_BRANCH_ELSE_C(aProperty);
			mxBreak;
		}

		mxCase(PSEUDO_GET_MEMBER)
		{
			txKind kind = mxThis->kind;
			mxImmediateS2(the_code, 2, anIndex);
			if (kind == XS_ALIAS_KIND) {
				resultInstance = the->aliasArray[mxThis->value.alias];
			}
			else if (kind == XS_REFERENCE_KIND) {
				resultInstance = mxThis->value.reference;
			}
			else {
				*(--the_stack) = *mxThis;
				the_code++;
				goto exit_accelerator;
			}
#ifdef mxDebug
			if (resultInstance->kind != XS_INSTANCE_KIND) {
				*(--the_stack) = *mxThis;
				the_code++;
				goto exit_accelerator;
			
			}
#endif
			if (resultInstance->flag & XS_SANDBOX_FLAG){
				resultInstance = resultInstance->value.instance.prototype;
				resultFlag = XS_DONT_SCRIPT_FLAG;
			}
			else
				resultFlag = frameFlag;
		getPropertyInPseudo: {
			resultProperty = resultInstance->next;
			if (resultInstance->flag & XS_VALUE_FLAG) {
				if (anIndex < 0) {
					if (resultProperty->kind == XS_GLOBAL_KIND) {
						#ifdef mxDebug
						if ((anIndex & 0x7FFF) >= the->symbolCount) {
							*(--the_stack) = *mxThis;
							the_code++;
							goto exit_accelerator;
						}
						#endif
						resultProperty = (resultFlag & XS_SANDBOX_FLAG)?resultProperty->value.global.cache[anIndex & 0x7FFF]:resultProperty->value.global.sandboxCache[anIndex & 0x7FFF];
						if (!resultProperty) {
							*(--the_stack) = *mxThis;
							the_code++;
							goto exit_accelerator;
						}
						goto gotProperty;
					}
				}
				else {
					if (resultProperty->kind == XS_ARRAY_KIND) {
						if ((txIndex)anIndex >= resultProperty->value.array.length){
							*(--the_stack) = *mxThis;
							the_code++;
							goto exit_accelerator;
						}
						resultProperty = resultProperty->value.array.address + anIndex;
						if (!resultProperty->ID) {
							*(--the_stack) = *mxThis;
							the_code++;
							goto exit_accelerator;
						}
						goto gotProperty;
					}
				}
			}
			while (resultProperty) {
				if (resultProperty->ID == anIndex) {
					if (!(resultProperty->flag & resultFlag)) {
						goto gotProperty;
					}
				}
				resultProperty = resultProperty->next;
			}
			if (resultInstance->ID >= 0) {
				if ((resultInstance = the->aliasArray[resultInstance->ID])) {	
					goto getPropertyInPseudo;
				}
			}
			else if ((resultInstance = resultInstance->value.instance.prototype)) {
				goto getPropertyInPseudo;
			}
			*(--the_stack) = *mxThis;
			the_code++;
			goto exit_accelerator;
		}
		gotProperty:
			if (resultProperty->kind == XS_ACCESSOR_KIND) {
				*(--the_stack) = *mxThis;
				the_code++;
				*resProperty = resultProperty;
				goto exit_accelerator;
			}
			mxNextOpcode(the_code,4,opcode);
			the_stack--;
			mxInitSlot(the_stack,resultProperty->kind);
			the_stack->value = resultProperty->value;
			*resProperty = resultProperty = C_NULL;
			mxBreak;	
		}

		mxCase(PSEUDO_BRANCH_ELSE1)
		{
			txSlot* aProperty;
			txByte theIndex = *(the_code+1);
			if(theIndex>=0) 
				aProperty = the->frame + aCount - theIndex + ((theIndex < aCount)?4:(- 5));
			else 
				aProperty = the->frame + theIndex - aOffset;
			
			if(!aProperty)	goto exit_accelerator;
			if (aProperty->kind == XS_ACCESSOR_KIND) goto exit_accelerator;
	
			the_code += 2;
			PSEUDO_PART_BRANCH_ELSE_C(aProperty);
			mxBreak;
		}

		mxCase(XS_STRING_POINTER)
		{
#if mxStringLength
			mxImmediateU2(the_code,1,anIndex);
#else
			anIndex = (txID)c_strlen((const char*)the_code+1) + 1;
#endif
			mxZeroSlot(--the_stack);
			the_stack->value.string = (txString)((char *)the_code + 1 + stringCommandOffset);
			the_stack->kind = XS_STRING_KIND;
			mxNextOpcode(the_code,anIndex+1+stringCommandOffset,opcode);
			mxBreak;
		}

		mxCase(XS_STRING) 
#if mxStringAccelerator
		{
			txSize theSize;
			txBlock* aBlock;
			txByte* aData;
			txString aString;
			txString result;
			mxZeroSlot(--the_stack);
#if mxStringLength
			mxImmediateU2(the_code, 1, anIndex);
#else
			anIndex = (txID)c_strlen((const char *)the_code + 1) + 1;
#endif
			aString = (txString)(the_code+1+stringCommandOffset);
			mxNextOpcode(the_code,anIndex+1+stringCommandOffset,opcode);
			
			theSize = ((anIndex+(sizeof(txChunk) - 1))&~(sizeof(txChunk) - 1)) + sizeof(txChunk);
			mxChunkResult(out_string);
			the_code -= anIndex+1+stringCommandOffset;
			goto exit_accelerator;
out_string:
			the_stack->value.string = result;
			the_stack->kind = XS_STRING_KIND;
			c_memmove(the_stack->value.string, aString, anIndex);
			mxBreak;
		}
#endif
		mxCase(XS_STRING_CONCAT)
#if mxStringAccelerator
		{
			txSize theSize;
			txBlock* aBlock;
			txByte* aData;
			txString aString;
			txSize aSize;
			txString result;
#if mxStringLength
			mxImmediateU2(the_code, 1, anIndex);
#else
			anIndex = (txID)c_strlen((const char *)the_code + 1) + 1;
#endif
			if(the_stack->kind!=XS_STRING_KIND) {
				mxZeroSlot(--the_stack);
				goto exit_accelerator;
			}
			aString = (txString)(the_code+1+stringCommandOffset);
			mxNextOpcode(the_code,anIndex+1+stringCommandOffset,opcode);
			aSize = c_strlen(the_stack->value.string);
			theSize = ((aSize + anIndex + sizeof(txChunk) - 1)&~(sizeof(txChunk) - 1)) + sizeof(txChunk);
			mxChunkResult(out_string_concat);
			mxZeroSlot(--the_stack);
			the_code -= anIndex+1+stringCommandOffset;
			goto exit_accelerator;
out_string_concat:
			c_memcpy(result, the_stack->value.string, aSize);
			c_memcpy(result + aSize, aString, anIndex);
			the_stack->value.string = result;
			the_stack->kind = XS_STRING_KIND;
			mxNextOpcode(the_code, 1, opcode);
			mxBreak;
		}
#endif
		mxCase(XS_STRING_CONCATBY)
#if mxStringAccelerator
		{
			txSize theSize;
			txBlock* aBlock;
			txByte* aData;
			txString aString;
			txSize aSize;
			txString result;
#if mxStringLength
			mxImmediateU2(the_code, 1, anIndex);
#else
			anIndex = (txID)c_strlen((const char *)the_code + 1) + 1;
#endif
			if(the_stack->kind!=XS_STRING_KIND) {
				mxZeroSlot(--the_stack);
				goto exit_accelerator;
			}
			aString = (txString)(the_code+1+stringCommandOffset);
			mxNextOpcode(the_code,anIndex+1+stringCommandOffset,opcode);

			aSize = c_strlen(the_stack->value.string);
			theSize = ((aSize + anIndex + sizeof(txChunk) - 1)&~(sizeof(txChunk) - 1)) + sizeof(txChunk);
			mxChunkResult(out_string_concatby);
			mxZeroSlot(--the_stack);
			the_code -= anIndex+1+stringCommandOffset;
			goto exit_accelerator;
out_string_concatby:
			c_memcpy(result , aString, anIndex - 1);
			c_memcpy(result + anIndex - 1, the_stack->value.string, aSize + 1);
			the_stack->value.string = result;
			the_stack->kind = XS_STRING_KIND;
			mxBreak;
		}
#endif
		mxCase(XS_UNDEFINED_KIND)
		mxCase(XS_NULL_KIND)
		mxCase(XS_BOOLEAN_KIND)
		mxCase(XS_INTEGER_KIND)
		mxCase(XS_NUMBER_KIND)
		mxCase(XS_REFERENCE_KIND)
		mxCase(XS_REGEXP_KIND)
		mxCase(XS_STRING_KIND)
		mxCase(XS_DATE_KIND)
		
		mxCase(XS_ARRAY_KIND)
		mxCase(XS_CALLBACK_KIND)
		mxCase(XS_CODE_KIND)
		mxCase(XS_HOST_KIND)

		mxCase(XS_SYMBOL_KIND)
		mxCase(XS_INSTANCE_KIND)

		mxCase(XS_FRAME_KIND)
		mxCase(XS_ROUTE_KIND)
		
		mxCase(XS_LIST_KIND)
		mxCase(XS_ACCESSOR_KIND)
		
		mxCase(XS_ALIAS_KIND)
		mxCase(XS_GLOBAL_KIND)
		
		mxCase(XS_NODE_KIND)
		mxCase(XS_PREFIX_KIND)
		mxCase(XS_ATTRIBUTE_RULE)
		mxCase(XS_DATA_RULE)
		mxCase(XS_PI_RULE)
		mxCase(XS_EMBED_RULE)
		mxCase(XS_JUMP_RULE)
		mxCase(XS_REFER_RULE)
		mxCase(XS_REPEAT_RULE)
		mxCase(XS_ERROR_RULE)

		mxCase(XS_LOCAL)
		mxCase(XS_LOCALS)
		mxCase(XS_LABEL)

		mxCase(XS_ALIAS)
		mxCase(XS_BIND)
		mxCase(XS_CATCH)
		mxCase(XS_DELETE)
		mxCase(XS_DELETE_AT)
		mxCase(XS_DELETE_MEMBER)
		mxCase(XS_DELETE_MEMBER_AT)
		mxCase(XS_DIVIDE)
		mxCase(XS_ENUM)
		mxCase(XS_FUNCTION)
		mxCase(XS_FUNCTION2)
		mxCase(XS_IN)
		mxCase(XS_INSTANCEOF)
		mxCase(XS_INSTANCIATE)
		mxCase(XS_JUMP)
		mxCase(XS_MODULO)
		mxCase(XS_NEW)
		mxCase(XS_PARAMETERS)
		mxCase(PSEUDO_PUT_MEMBER)
		mxCase(XS_PUT_MEMBER)
		mxCase(XS_PUT_MEMBER_AT)
		mxCase(XS_RETURN)
		mxCase(XS_SCOPE)
		mxCase(XS_STATUS)
		mxCase(XS_THROW)
		mxCase(XS_TYPEOF)
		mxCase(XS_UNCATCH)
		mxCase(XS_UNSCOPE)
		
		mxCase(XS_ATTRIBUTE_PATTERN)
		mxCase(XS_DATA_PATTERN)
		mxCase(XS_PI_PATTERN)
		mxCase(XS_EMBED_PATTERN)
		mxCase(XS_JUMP_PATTERN)
		mxCase(XS_REFER_PATTERN)
		mxCase(XS_REPEAT_PATTERN)
		mxCase(XS_FLAG_INSTANCE)
			goto exit_accelerator;

#if !defined(__GNUC__) || ( defined(__GNUC__) && !defined(__OPTIMIZE__) )
		default:
			goto exit_accelerator;
#endif
		}
	}

exit_accelerator:

#ifdef mxFrequency
	the->fastFrequencies[opcode]--;
#endif

	the->code = the_code;
	the->stack = the_stack;
	return anIndex;
}
