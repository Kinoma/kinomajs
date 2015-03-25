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
#include "xsScript.h"
#define PASS_LABEL(slot) {\
	while(slot->kind==XS_LABEL || slot->kind==XS_LINE || slot->kind==XS_NOP) {\
		slot = slot->next;\
	}\
}

#define PASS_LINE(slot) {\
	while(slot->kind==XS_LINE || slot->kind==XS_NOP) {\
		slot = slot->next;\
	}\
}

#if !defined(FALSE)
#define FALSE		(0)
#endif

#if !defined(TRUE)
#define TRUE		(1)
#endif

enum {
	STACK_UNDEF = 0,	/*not a command on run time*/
	STACK_SINGLE_OP,	/*command use stack as Parameter*/
	STACK_DOUBLE_OP,	/*command use stack and stack+1 as Parameter*/
	STACK_INSTANCE,		/*command use stack content as instance*/
	STACK_INDEX,		/*command use stack content as index*/
	STACK_DEC,			/*command will decriment stack and set stack*/
	STACK_CHUNK,		/*command maybe use fxNewChunk function*/
	STACK_JUMP,			/*command will do branch or call function*/
	STACK_STRING,		/*command*/
	STACK_CONCAT,		/*XS_ADD*/
	STACK_OTHER,		
};

static txByte stackUsage[] = {
	STACK_UNDEF,	/*XS_UNDEFINED_KIND*/
	STACK_UNDEF,	/*XS_NULL_KIND*/
	STACK_UNDEF,	/*XS_BOOLEAN_KIND*/
	STACK_UNDEF,	/*XS_INTEGER_KIND*/
	STACK_UNDEF,	/*XS_NUMBER_KIND*/
	STACK_UNDEF,	/*XS_REFERENCE_KIND*/
	STACK_UNDEF,	/*XS_REGEXP_KIND*/
	STACK_UNDEF,	/*XS_STRING_KIND*/
	STACK_UNDEF,	/*XS_DATE_KIND*/
	STACK_UNDEF,	/*XS_ARRAY_KIND*/
	STACK_UNDEF,	/*XS_CALLBACK_KIND*/
	STACK_UNDEF,	/*XS_CODE_KIND*/
	STACK_UNDEF,	/*XS_HOST_KIND*/
	STACK_UNDEF,	/*XS_SYMBOL_KIND*/
	STACK_UNDEF,	/*XS_INSTANCE_KIND*/
	STACK_UNDEF,	/*XS_FRAME_KIND*/
	STACK_UNDEF,	/*XS_ROUTE_KIND*/
	STACK_UNDEF,	/*XS_LIST_KIND*/
	STACK_UNDEF,	/*XS_ACCESSOR_KIND*/
	STACK_UNDEF,	/*XS_ALIAS_KIND*/
	STACK_UNDEF,	/*XS_GLOBAL_KIND*/	
	STACK_UNDEF,	/*XS_NODE_KIND*/
	STACK_UNDEF,	/*XS_PREFIX_KIND*/
	STACK_UNDEF,	/*XS_ATTRIBUTE_RULE*/
	STACK_UNDEF,	/*XS_DATA_RULE*/
	STACK_UNDEF,	/*XS_PI_RULE*/
	STACK_UNDEF,	/*XS_EMBED_RULE*/
	STACK_UNDEF,	/*XS_JUMP_RULE*/
	STACK_UNDEF,	/*XS_REFER_RULE*/
	STACK_UNDEF,	/*XS_REPEAT_RULE*/
	STACK_UNDEF,	/*XS_ERROR_RULE*/
	STACK_UNDEF,	/*XS_LOCAL*/
	STACK_UNDEF,	/*XS_LOCALS*/
	STACK_UNDEF,	/*XS_LABEL*/	
	STACK_CONCAT,	/*XS_ADD*/
	STACK_OTHER,	/*XS_ALIAS*/
	STACK_OTHER,	/*XS_BEGIN*/
	STACK_DEC,		/*XS_BIND*/
	STACK_DOUBLE_OP,/*XS_BIT_AND*/
	STACK_SINGLE_OP,/*XS_BIT_NOT*/
	STACK_DOUBLE_OP,/*XS_BIT_OR*/
	STACK_DOUBLE_OP,/*XS_BIT_XOR*/
	STACK_JUMP,		/*XS_BRANCH*/
	STACK_SINGLE_OP,/*XS_BRANCH_ELSE*/
	STACK_SINGLE_OP,/*XS_BRANCH_IF*/
	STACK_JUMP,		/*XS_BREAK*/
	STACK_JUMP,		/*XS_CALL*/
	STACK_OTHER,	/*XS_CATCH*/
	STACK_OTHER,	/*XS_DEBUGGER*/
	STACK_SINGLE_OP,/*XS_DECREMENT*/
	STACK_OTHER,	/*XS_DELETE*/
	STACK_OTHER,	/*XS_DELETE_AT*/
	STACK_OTHER,	/*XS_DELETE_MEMBER*/
	STACK_OTHER,	/*XS_DELETE_MEMBER_AT*/
	STACK_DOUBLE_OP,/*XS_DIVIDE*/
	STACK_OTHER,	/*XS_DUB*/
	STACK_JUMP,		/*XS_END*/
	STACK_OTHER,	/*XS_ENUM*/
	STACK_DOUBLE_OP,/*XS_EQUAL*/
	STACK_DEC,		/*XS_FALSE*/
	STACK_OTHER,	/*XS_FILE*/
	STACK_JUMP,		/*XS_FUNCTION*/
	STACK_DEC,		/*XS_GET*/
	STACK_DEC,		/*XS_GET_AT*/
	STACK_INSTANCE,	/*XS_GET_MEMBER*/
	STACK_INDEX,	/*XS_GET_MEMBER_AT*/
	STACK_DEC,		/*XS_GLOBAL*/
	STACK_OTHER,	/*XS_IN*/
	STACK_SINGLE_OP,/*XS_INCREMENT*/
	STACK_OTHER,	/*XS_INSTANCEOF*/
	STACK_OTHER,	/*XS_INSTANCIATE*/
	STACK_DEC,		/*XS_INTEGER_1*/
	STACK_DEC,		/*XS_INTEGER_16*/
	STACK_DEC,		/*XS_INTEGER_32*/
	STACK_JUMP,		/*XS_JUMP*/
	STACK_DOUBLE_OP,/*XS_LEFT_SHIFT*/
	STACK_DOUBLE_OP,/*XS_LESS*/
	STACK_DOUBLE_OP,/*XS_LESS_EQUAL*/
	STACK_OTHER,	/*XS_LINE*/
	STACK_SINGLE_OP,/*XS_MINUS*/
	STACK_DOUBLE_OP,/*XS_MODULO*/
	STACK_DOUBLE_OP,/*XS_MORE*/
	STACK_DOUBLE_OP,/*XS_MORE_EQUAL*/
	STACK_DOUBLE_OP,/*XS_MULTIPLY*/
	STACK_OTHER,	/*XS_NEW*/
	STACK_SINGLE_OP,/*XS_NOT*/
	STACK_DOUBLE_OP,/*XS_NOT_EQUAL*/
	STACK_DEC,		/*XS_NULL*/
	STACK_DEC,		/*XS_NUMBER*/
	STACK_OTHER,	/*XS_PARAMETERS*/
	STACK_SINGLE_OP,/*XS_PLUS*/
	STACK_OTHER,	/*XS_POP*/
	STACK_CHUNK,	/*XS_PUT_MEMBER*/
	STACK_CHUNK,	/*XS_PUT_MEMBER_AT*/
	STACK_OTHER,	/*XS_RESULT*/
	STACK_OTHER,	/*XS_RETURN*/
	STACK_OTHER,	/*XS_ROUTE*/
	STACK_OTHER,	/*XS_SCOPE*/
	STACK_OTHER,	/*XS_SET*/
	STACK_OTHER,	/*XS_SET_AT*/
	STACK_CHUNK,	/*XS_SET_MEMBER*/
	STACK_CHUNK,	/*XS_SET_MEMBER_AT*/
	STACK_DOUBLE_OP,/*XS_SIGNED_RIGHT_SHIFT*/
	STACK_DEC,		/*XS_STATUS*/
	STACK_DOUBLE_OP,/*XS_STRICT_EQUAL*/
	STACK_DOUBLE_OP,/*XS_STRICT_NOT_EQUAL*/
	STACK_STRING,	/*XS_STRING*/
	STACK_DOUBLE_OP,/*XS_SUBTRACT*/
	STACK_OTHER,	/*XS_SWAP*/
	STACK_DEC,		/*XS_THIS*/
	STACK_OTHER,	/*XS_THROW*/
	STACK_DEC,		/*XS_TRUE*/
	STACK_OTHER,	/*XS_TYPEOF*/
	STACK_OTHER,	/*XS_UNCATCH*/
	STACK_DEC,		/*XS_UNDEFINED*/
	STACK_OTHER,	/*XS_UNSCOPE*/
	STACK_DOUBLE_OP,/*XS_UNSIGNED_RIGHT_SHIFT*/
	STACK_SINGLE_OP,/*XS_VOID*/
		
	STACK_CHUNK,	/*XS_ATTRIBUTE_PATTERN*/
	STACK_CHUNK,	/*XS_DATA_PATTERN*/
	STACK_CHUNK,	/*XS_PI_PATTERN*/
	STACK_CHUNK,	/*XS_EMBED_PATTERN*/
	STACK_CHUNK,	/*XS_JUMP_PATTERN*/
	STACK_CHUNK,	/*XS_REFER_PATTERN*/
	STACK_CHUNK,	/*XS_REPEAT_PATTERN*/
	STACK_OTHER,	/*XS_FLAG_INSTANCE*/
		
	STACK_JUMP,		/*XS_BRANCH2*/
	STACK_SINGLE_OP,/*XS_BRANCH_ELSE2*/
	STACK_SINGLE_OP,/*XS_BRANCH_IF2*/
	STACK_JUMP,		/*XS_FUNCTION2*/
	STACK_OTHER,	/*XS_ROUTE2*/
	STACK_DEC,		/*XS_GET_NEGATIVE_ID*/
	STACK_OTHER,	/*XS_SET_NEGATIVE_ID*/
	STACK_SINGLE_OP,/*XS_BRANCH_ELSE_BOOL*/
	STACK_SINGLE_OP,/*XS_BRANCH_IF_BOOL*/
	STACK_SINGLE_OP,/*XS_BRANCH_ELSE_BOOL2*/
	STACK_SINGLE_OP,/*XS_BRANCH_IF_BOOL2*/
	STACK_STRING,	/*XS_STRING_POINTER*/
	STACK_STRING,	/*XS_STRING_CONCAT*/
	STACK_STRING,	/*XS_STRING_CONCATBY*/
	STACK_OTHER,	/*PSEUDO_BRANCH_ELSE0*/
	STACK_OTHER,	/*PSEUDO_BRANCH_ELSE1*/
	STACK_OTHER,	/*PSEUDO_GET_MEMBER*/
	STACK_OTHER,	/*PSEUDO_SET_MEMBER*/
	STACK_OTHER,	/*PSEUDO_LESS*/
	STACK_OTHER,	/*PSEUDO_LESS_EQUAL*/
	STACK_OTHER,	/*PSEUDO_INCREMENT*/
	STACK_OTHER,	/*PSEUDO_DECREMENT*/
	STACK_OTHER,	/*PSEUDO_DUB*/
	STACK_OTHER,	/*PSEUDO_DOUBLE_GET_NEGATIVE*/
	STACK_OTHER,	/*PSEUDO_DOUBLE_GET_AT*/
	STACK_OTHER,	/*PSEUDO_PUT_MEMBER*/
	STACK_OTHER,	/*XS_GET_MEMBER_FOR_CALL*/
	STACK_OTHER,	/*XS_GET_FOR_NEW*/
};
	
	
static txKind gxTokenCodes[XS_TOKEN_COUNT] = {
	/* XS_NO_TOKEN */ 0,
	/* XS_TOKEN_ADD */ XS_ADD,
	/* XS_TOKEN_ADD_ASSIGN */ XS_ADD,
	/* XS_TOKEN_AND */ 0,
	/* XS_TOKEN_ASSIGN */ 0,
	/* XS_TOKEN_BIT_AND */ XS_BIT_AND,
	/* XS_TOKEN_BIT_AND_ASSIGN */ XS_BIT_AND,
	/* XS_TOKEN_BIT_NOT */ XS_BIT_NOT,
	/* XS_TOKEN_BIT_OR */ XS_BIT_OR,
	/* XS_TOKEN_BIT_OR_ASSIGN */ XS_BIT_OR,
	/* XS_TOKEN_BIT_XOR */ XS_BIT_XOR,
	/* XS_TOKEN_BIT_XOR_ASSIGN */ XS_BIT_XOR,
	/* XS_TOKEN_BREAK */ 0,
	/* XS_TOKEN_CASE */ 0,
	/* XS_TOKEN_CATCH */ XS_CATCH,
	/* XS_TOKEN_CLASS */ 0,
	/* XS_TOKEN_COLON */ 0,
	/* XS_TOKEN_COMMA */ 0,
	/* XS_TOKEN_CONST */ 0,
	/* XS_TOKEN_CONTINUE */ 0,
	/* XS_TOKEN_DEBUGGER */ 0,
	/* XS_TOKEN_DECREMENT */ XS_DECREMENT,
	/* XS_TOKEN_DEFAULT */ 0,
	/* XS_TOKEN_DELETE */ 0,
	/* XS_TOKEN_DIVIDE */ XS_DIVIDE,
	/* XS_TOKEN_DIVIDE_ASSIGN */ XS_DIVIDE,
	/* XS_TOKEN_DO */ 0,
	/* XS_TOKEN_DOT */ 0,
	/* XS_TOKEN_ELSE */ 0,
	/* XS_TOKEN_ENUM */ XS_ENUM,
	/* XS_TOKEN_EOF */ 0,
	/* XS_TOKEN_EQUAL */ XS_EQUAL,
	/* XS_TOKEN_EXPORT */ 0,
	/* XS_TOKEN_EXTENDS */ 0,
	/* XS_TOKEN_FALSE */ XS_FALSE,
	/* XS_TOKEN_FINALLY */ 0,
	/* XS_TOKEN_FOR */ 0,
	/* XS_TOKEN_FUNCTION */ XS_FUNCTION,
	/* XS_TOKEN_IDENTIFIER */ 0,
	/* XS_TOKEN_IF */ 0,
	/* XS_TOKEN_IMPLEMENTS */ 0,
	/* XS_TOKEN_IMPORT */ 0,
	/* XS_TOKEN_IN */ XS_IN,
	/* XS_TOKEN_INCREMENT */ XS_INCREMENT,
	/* XS_TOKEN_INSTANCEOF */ XS_INSTANCEOF,
	/* XS_TOKEN_INTEGER */ XS_INTEGER_32,
	/* XS_TOKEN_INTERFACE */ 0,
	/* XS_TOKEN_LEFT_BRACE */ 0,
	/* XS_TOKEN_LEFT_BRACKET */ 0,
	/* XS_TOKEN_LEFT_PARENTHESIS */ 0,
	/* XS_TOKEN_LEFT_SHIFT */ XS_LEFT_SHIFT,
	/* XS_TOKEN_LEFT_SHIFT_ASSIGN */ XS_LEFT_SHIFT,
	/* XS_TOKEN_LESS */ XS_LESS,
	/* XS_TOKEN_LESS_EQUAL */ XS_LESS_EQUAL,
	/* XS_TOKEN_LET */ 0,
	/* XS_TOKEN_MODULO */ XS_MODULO,
	/* XS_TOKEN_MODULO_ASSIGN */ XS_MODULO,
	/* XS_TOKEN_MORE */ XS_MORE,
	/* XS_TOKEN_MORE_EQUAL */ XS_MORE_EQUAL,
	/* XS_TOKEN_MULTIPLY */ XS_MULTIPLY,
	/* XS_TOKEN_MULTIPLY_ASSIGN */ XS_MULTIPLY,
	/* XS_TOKEN_NEW */ 0, 
	/* XS_TOKEN_NOT */ XS_NOT,
	/* XS_TOKEN_NOT_EQUAL */ XS_NOT_EQUAL,
	/* XS_TOKEN_NULL */ XS_NULL, 
	/* XS_TOKEN_OR */ 0,
	/* XS_TOKEN_PACKAGE */ 0,
	/* XS_TOKEN_PRIVATE */ 0,
	/* XS_TOKEN_PROTECTED */ 0,
	/* XS_TOKEN_PUBLIC */ 0,
	/* XS_TOKEN_QUESTION_MARK */ 0,
	/* XS_TOKEN_NUMBER */ XS_NUMBER,
	/* XS_TOKEN_REGEXP */ 0,
	/* XS_TOKEN_RETURN */ XS_RETURN,
	/* XS_TOKEN_RIGHT_BRACE */ 0,
	/* XS_TOKEN_RIGHT_BRACKET */ 0,
	/* XS_TOKEN_RIGHT_PARENTHESIS */ 0,
	/* XS_TOKEN_SEMICOLON */ 0,
	/* XS_TOKEN_SHORT */ 0,
	/* XS_TOKEN_SIGNED_RIGHT_SHIFT */ XS_SIGNED_RIGHT_SHIFT,
	/* XS_TOKEN_SIGNED_RIGHT_SHIFT_ASSIGN */ XS_SIGNED_RIGHT_SHIFT,
	/* XS_TOKEN_STATIC */ 0,
	/* XS_TOKEN_STRICT_EQUAL */ XS_STRICT_EQUAL,
	/* XS_TOKEN_STRICT_NOT_EQUAL */ XS_STRICT_NOT_EQUAL,
	/* XS_TOKEN_STRING */ XS_STRING,
	/* XS_TOKEN_SUBTRACT */ XS_SUBTRACT,
	/* XS_TOKEN_SUBTRACT_ASSIGN */ XS_SUBTRACT,
	/* XS_TOKEN_SUPER */ 0,
	/* XS_TOKEN_SWITCH */ 0,
	/* XS_TOKEN_THIS */ XS_THIS,
	/* XS_TOKEN_THROW */ XS_THROW,
	/* XS_TOKEN_TRUE */ XS_TRUE,
	/* XS_TOKEN_TRY */ 0,
	/* XS_TOKEN_TYPEOF */ XS_TYPEOF,
	/* XS_TOKEN_UNSIGNED_RIGHT_SHIFT */ XS_UNSIGNED_RIGHT_SHIFT,
	/* XS_TOKEN_UNSIGNED_RIGHT_SHIFT_ASSIGN */ XS_UNSIGNED_RIGHT_SHIFT,
	/* XS_TOKEN_VAR */ 0,
	/* XS_TOKEN_VOID */ XS_VOID,
	/* XS_TOKEN_WHILE */ 0,
	/* XS_TOKEN_WITH */ 0,
	/* XS_TOKEN_YIELD */ 0
};

static txTokenFlag gxTokenFlags[XS_TOKEN_COUNT] = {
	/* XS_NO_TOKEN */ 0,
	/* XS_TOKEN_ADD */ XS_TOKEN_BEGIN_STATEMENT | XS_TOKEN_BEGIN_EXPRESSION | XS_TOKEN_ADDITIVE_EXPRESSION | XS_TOKEN_PREFIX_EXPRESSION,
	/* XS_TOKEN_ADD_ASSIGN */ XS_TOKEN_ASSIGN_EXPRESSION,
	/* XS_TOKEN_AND */ 0,
	/* XS_TOKEN_ASSIGN */ XS_TOKEN_ASSIGN_EXPRESSION,
	/* XS_TOKEN_BIT_AND */ 0,
	/* XS_TOKEN_BIT_AND_ASSIGN */ XS_TOKEN_ASSIGN_EXPRESSION,
	/* XS_TOKEN_BIT_NOT */ XS_TOKEN_BEGIN_STATEMENT | XS_TOKEN_BEGIN_EXPRESSION | XS_TOKEN_PREFIX_EXPRESSION,
	/* XS_TOKEN_BIT_OR */ 0,
	/* XS_TOKEN_BIT_OR_ASSIGN */ XS_TOKEN_ASSIGN_EXPRESSION,
	/* XS_TOKEN_BIT_XOR */ 0,
	/* XS_TOKEN_BIT_XOR_ASSIGN */ XS_TOKEN_ASSIGN_EXPRESSION,
	/* XS_TOKEN_BREAK */ XS_TOKEN_BEGIN_STATEMENT,
	/* XS_TOKEN_CASE */ 0,
	/* XS_TOKEN_CATCH */ 0,
	/* XS_TOKEN_CLASS */ 0,
	/* XS_TOKEN_COLON */ 0,
	/* XS_TOKEN_COMMA */ XS_TOKEN_END_STATEMENT,
	/* XS_TOKEN_CONST */ 0,
	/* XS_TOKEN_CONTINUE */ XS_TOKEN_BEGIN_STATEMENT,
	/* XS_TOKEN_DEBUGGER */ XS_TOKEN_BEGIN_STATEMENT,
	/* XS_TOKEN_DECREMENT */ XS_TOKEN_BEGIN_STATEMENT | XS_TOKEN_BEGIN_EXPRESSION | XS_TOKEN_PREFIX_EXPRESSION | XS_TOKEN_POSTFIX_EXPRESSION,
	/* XS_TOKEN_DEFAULT */ 0,
	/* XS_TOKEN_DELETE */ XS_TOKEN_BEGIN_STATEMENT | XS_TOKEN_BEGIN_EXPRESSION | XS_TOKEN_PREFIX_EXPRESSION,
	/* XS_TOKEN_DIVIDE */ XS_TOKEN_MULTIPLICATIVE_EXPRESSION,
	/* XS_TOKEN_DIVIDE_ASSIGN */ XS_TOKEN_ASSIGN_EXPRESSION,
	/* XS_TOKEN_DO */ XS_TOKEN_BEGIN_STATEMENT,
	/* XS_TOKEN_DOT */ 0,
	/* XS_TOKEN_ELSE */ 0,
	/* XS_TOKEN_ENUM */ 0,
	/* XS_TOKEN_EOF */ XS_TOKEN_END_STATEMENT,
	/* XS_TOKEN_EQUAL */ XS_TOKEN_EQUAL_EXPRESSION,
	/* XS_TOKEN_EXPORT */ 0,
	/* XS_TOKEN_EXTENDS */ 0,
	/* XS_TOKEN_FALSE */ XS_TOKEN_BEGIN_STATEMENT | XS_TOKEN_BEGIN_EXPRESSION | XS_TOKEN_NO_REGEXP,
	/* XS_TOKEN_FINALLY */ 0,
	/* XS_TOKEN_FOR */ XS_TOKEN_BEGIN_STATEMENT,
	/* XS_TOKEN_FUNCTION */ XS_TOKEN_BEGIN_STATEMENT | XS_TOKEN_BEGIN_EXPRESSION,
	/* XS_TOKEN_IDENTIFIER */ XS_TOKEN_BEGIN_STATEMENT | XS_TOKEN_BEGIN_EXPRESSION | XS_TOKEN_NO_REGEXP,
	/* XS_TOKEN_IF */ XS_TOKEN_BEGIN_STATEMENT,
	/* XS_TOKEN_IMPLEMENTS */ 0,
	/* XS_TOKEN_IMPORT */ 0,
	/* XS_TOKEN_IN */ XS_TOKEN_RELATIONAL_EXPRESSION,
	/* XS_TOKEN_INCREMENT */ XS_TOKEN_BEGIN_STATEMENT | XS_TOKEN_BEGIN_EXPRESSION | XS_TOKEN_PREFIX_EXPRESSION | XS_TOKEN_POSTFIX_EXPRESSION,
	/* XS_TOKEN_INSTANCEOF */ XS_TOKEN_RELATIONAL_EXPRESSION,
	/* XS_TOKEN_INTEGER */ XS_TOKEN_BEGIN_STATEMENT | XS_TOKEN_BEGIN_EXPRESSION | XS_TOKEN_NO_REGEXP,
	/* XS_TOKEN_INTERFACE */ 0,
	/* XS_TOKEN_LEFT_BRACE */ XS_TOKEN_BEGIN_STATEMENT | XS_TOKEN_BEGIN_EXPRESSION,
	/* XS_TOKEN_LEFT_BRACKET */ XS_TOKEN_BEGIN_STATEMENT | XS_TOKEN_BEGIN_EXPRESSION,
	/* XS_TOKEN_LEFT_PARENTHESIS */ XS_TOKEN_BEGIN_STATEMENT | XS_TOKEN_BEGIN_EXPRESSION,
	/* XS_TOKEN_LEFT_SHIFT */ XS_TOKEN_SHIFT_EXPRESSION,
	/* XS_TOKEN_LEFT_SHIFT_ASSIGN */ XS_TOKEN_ASSIGN_EXPRESSION,
	/* XS_TOKEN_LESS */ XS_TOKEN_RELATIONAL_EXPRESSION,
	/* XS_TOKEN_LESS_EQUAL */ XS_TOKEN_RELATIONAL_EXPRESSION,
	/* XS_TOKEN_LET */ 0,
	/* XS_TOKEN_MODULO */ XS_TOKEN_MULTIPLICATIVE_EXPRESSION,
	/* XS_TOKEN_MODULO_ASSIGN */ XS_TOKEN_ASSIGN_EXPRESSION,
	/* XS_TOKEN_MORE */ XS_TOKEN_RELATIONAL_EXPRESSION,
	/* XS_TOKEN_MORE_EQUAL */ XS_TOKEN_RELATIONAL_EXPRESSION,
	/* XS_TOKEN_MULTIPLY */ XS_TOKEN_MULTIPLICATIVE_EXPRESSION,
	/* XS_TOKEN_MULTIPLY_ASSIGN */ XS_TOKEN_ASSIGN_EXPRESSION,
	/* XS_TOKEN_NEW */ XS_TOKEN_BEGIN_STATEMENT | XS_TOKEN_BEGIN_EXPRESSION, 
	/* XS_TOKEN_NOT */ XS_TOKEN_BEGIN_STATEMENT | XS_TOKEN_BEGIN_EXPRESSION | XS_TOKEN_PREFIX_EXPRESSION,
	/* XS_TOKEN_NOT_EQUAL */ XS_TOKEN_EQUAL_EXPRESSION,
	/* XS_TOKEN_NULL */ XS_TOKEN_BEGIN_STATEMENT | XS_TOKEN_BEGIN_EXPRESSION | XS_TOKEN_NO_REGEXP, 
	/* XS_TOKEN_OR */ 0,
	/* XS_TOKEN_PACKAGE */ 0,
	/* XS_TOKEN_PRIVATE */ 0,
	/* XS_TOKEN_PROTECTED */ 0,
	/* XS_TOKEN_PUBLIC */ 0,
	/* XS_TOKEN_QUESTION_MARK */ 0,
	/* XS_TOKEN_NUMBER */ XS_TOKEN_BEGIN_STATEMENT | XS_TOKEN_BEGIN_EXPRESSION | XS_TOKEN_NO_REGEXP,
	/* XS_TOKEN_REGEXP */ XS_TOKEN_BEGIN_STATEMENT | XS_TOKEN_BEGIN_EXPRESSION | XS_TOKEN_NO_REGEXP,
	/* XS_TOKEN_RETURN */ XS_TOKEN_BEGIN_STATEMENT,
	/* XS_TOKEN_RIGHT_BRACE */ XS_TOKEN_END_STATEMENT,
	/* XS_TOKEN_RIGHT_BRACKET */ XS_TOKEN_NO_REGEXP,
	/* XS_TOKEN_RIGHT_PARENTHESIS */ XS_TOKEN_NO_REGEXP,
	/* XS_TOKEN_SEMICOLON */ XS_TOKEN_BEGIN_STATEMENT | XS_TOKEN_END_STATEMENT,
	/* XS_TOKEN_SHORT */ 0,
	/* XS_TOKEN_SIGNED_RIGHT_SHIFT */ XS_TOKEN_SHIFT_EXPRESSION,
	/* XS_TOKEN_SIGNED_RIGHT_SHIFT_ASSIGN */ XS_TOKEN_ASSIGN_EXPRESSION,
	/* XS_TOKEN_STATIC */ 0,
	/* XS_TOKEN_STRICT_EQUAL */ XS_TOKEN_EQUAL_EXPRESSION,
	/* XS_TOKEN_STRICT_NOT_EQUAL */ XS_TOKEN_EQUAL_EXPRESSION,
	/* XS_TOKEN_STRING */ XS_TOKEN_BEGIN_STATEMENT | XS_TOKEN_BEGIN_EXPRESSION | XS_TOKEN_NO_REGEXP,
	/* XS_TOKEN_SUBTRACT */ XS_TOKEN_BEGIN_STATEMENT | XS_TOKEN_BEGIN_EXPRESSION | XS_TOKEN_ADDITIVE_EXPRESSION | XS_TOKEN_PREFIX_EXPRESSION,
	/* XS_TOKEN_SUBTRACT_ASSIGN */ XS_TOKEN_ASSIGN_EXPRESSION,
	/* XS_TOKEN_SUPER */ 0,
	/* XS_TOKEN_SWITCH */ XS_TOKEN_BEGIN_STATEMENT,
	/* XS_TOKEN_THIS */ XS_TOKEN_BEGIN_STATEMENT | XS_TOKEN_BEGIN_EXPRESSION | XS_TOKEN_NO_REGEXP,
	/* XS_TOKEN_THROW */ XS_TOKEN_BEGIN_STATEMENT,
	/* XS_TOKEN_TRUE */ XS_TOKEN_BEGIN_STATEMENT | XS_TOKEN_BEGIN_EXPRESSION | XS_TOKEN_NO_REGEXP,
	/* XS_TOKEN_TRY */ XS_TOKEN_BEGIN_STATEMENT,
	/* XS_TOKEN_TYPEOF */ XS_TOKEN_BEGIN_STATEMENT | XS_TOKEN_BEGIN_EXPRESSION | XS_TOKEN_PREFIX_EXPRESSION,
	/* XS_TOKEN_UNSIGNED_RIGHT_SHIFT */ XS_TOKEN_SHIFT_EXPRESSION,
	/* XS_TOKEN_UNSIGNED_RIGHT_SHIFT_ASSIGN */ XS_TOKEN_ASSIGN_EXPRESSION,
	/* XS_TOKEN_VAR */ XS_TOKEN_BEGIN_STATEMENT,
	/* XS_TOKEN_VOID */ XS_TOKEN_BEGIN_STATEMENT | XS_TOKEN_BEGIN_EXPRESSION | XS_TOKEN_PREFIX_EXPRESSION,
	/* XS_TOKEN_WHILE */ XS_TOKEN_BEGIN_STATEMENT,
	/* XS_TOKEN_WITH */ XS_TOKEN_BEGIN_STATEMENT,
	/* XS_TOKEN_YIELD */ 0
};

static txString gxTokenNames[XS_TOKEN_COUNT] = {
	/* XS_NO_TOKEN */ "",
	/* XS_TOKEN_ADD */ "+",
	/* XS_TOKEN_ADD_ASSIGN */ "+=",
	/* XS_TOKEN_AND */ "&&",
	/* XS_TOKEN_ASSIGN */ "=",
	/* XS_TOKEN_BIT_AND */ "&",
	/* XS_TOKEN_BIT_AND_ASSIGN */ "&=",
	/* XS_TOKEN_BIT_NOT */ "~",
	/* XS_TOKEN_BIT_OR */ "|",
	/* XS_TOKEN_BIT_OR_ASSIGN */ "|=",
	/* XS_TOKEN_BIT_XOR */ "^",
	/* XS_TOKEN_BIT_XOR_ASSIGN */ "^=",
	/* XS_TOKEN_BREAK */ "break",
	/* XS_TOKEN_CASE */ "case",
	/* XS_TOKEN_CATCH */ "catch",
	/* XS_TOKEN_CLASS */ "class",
	/* XS_TOKEN_COLON */ ":",
	/* XS_TOKEN_COMMA */ ",",
	/* XS_TOKEN_CONST */ "const",
	/* XS_TOKEN_CONTINUE */ "continue",
	/* XS_TOKEN_DEBUGGER */ "debugger",
	/* XS_TOKEN_DECREMENT */ "--",
	/* XS_TOKEN_DEFAULT */ "default",
	/* XS_TOKEN_DELETE */ "delete",
	/* XS_TOKEN_DIVIDE */ "/",
	/* XS_TOKEN_DIVIDE_ASSIGN */ "/=",
	/* XS_TOKEN_DO */ "do",
	/* XS_TOKEN_DOT */ ".",
	/* XS_TOKEN_ELSE */ "else",
	/* XS_TOKEN_ENUM */ "enum",
	/* XS_TOKEN_EOF */ "",
	/* XS_TOKEN_EQUAL */ "==",
	/* XS_TOKEN_EXPORT */ "export",
	/* XS_TOKEN_EXTENDS */ "extends",
	/* XS_TOKEN_FALSE */ "false",
	/* XS_TOKEN_FINALLY */ "finally",
	/* XS_TOKEN_FOR */ "for",
	/* XS_TOKEN_FUNCTION */ "function",
	/* XS_TOKEN_IDENTIFIER */ "identifier",
	/* XS_TOKEN_IF */ "if",
	/* XS_TOKEN_IMPLEMENTS */ "implements",
	/* XS_TOKEN_IMPORT */ "import",
	/* XS_TOKEN_IN */ "in",
	/* XS_TOKEN_INCREMENT */ "++",
	/* XS_TOKEN_INSTANCEOF */ "instanceof",
	/* XS_TOKEN_INTEGER */ "integer",
	/* XS_TOKEN_INTERFACE */ "interface",
	/* XS_TOKEN_LEFT_BRACE */ "{",
	/* XS_TOKEN_LEFT_BRACKET */ "[",
	/* XS_TOKEN_LEFT_PARENTHESIS */ "(",
	/* XS_TOKEN_LEFT_SHIFT */ "<<",
	/* XS_TOKEN_LEFT_SHIFT_ASSIGN */ "<<=",
	/* XS_TOKEN_LESS */ "<",
	/* XS_TOKEN_LESS_EQUAL */ "<=",
	/* XS_TOKEN_LET */ "let",
	/* XS_TOKEN_MODULO */ "%",
	/* XS_TOKEN_MODULO_ASSIGN */ "%=",
	/* XS_TOKEN_MORE */ ">",
	/* XS_TOKEN_MORE_EQUAL */ ">=",
	/* XS_TOKEN_MULTIPLY */ "*",
	/* XS_TOKEN_MULTIPLY_ASSIGN */ "*=",
	/* XS_TOKEN_NEW */ "new", 
	/* XS_TOKEN_NOT */ "!",
	/* XS_TOKEN_NOT_EQUAL */ "!=",
	/* XS_TOKEN_NULL */ "null", 
	/* XS_TOKEN_OR */ "||",
	/* XS_TOKEN_PACKAGE */ "package",
	/* XS_TOKEN_PRIVATE */ "private",
	/* XS_TOKEN_PROTECTED */ "protected",
	/* XS_TOKEN_PUBLIC */ "public",
	/* XS_TOKEN_QUESTION_MARK */ "?",
	/* XS_TOKEN_NUMBER */ "number",
	/* XS_TOKEN_REGEXP */ "regexp",
	/* XS_TOKEN_RETURN */ "return",
	/* XS_TOKEN_RIGHT_BRACE */ "}",
	/* XS_TOKEN_RIGHT_BRACKET */ "]",
	/* XS_TOKEN_RIGHT_PARENTHESIS */ ")",
	/* XS_TOKEN_SEMICOLON */ ";",
	/* XS_TOKEN_SHORT */ "short",
	/* XS_TOKEN_SIGNED_RIGHT_SHIFT */ ">>",
	/* XS_TOKEN_SIGNED_RIGHT_SHIFT_ASSIGN */ ">>=",
	/* XS_TOKEN_STATIC */ "static",
	/* XS_TOKEN_STRICT_EQUAL */ "===",
	/* XS_TOKEN_STRICT_NOT_EQUAL */ "!==",
	/* XS_TOKEN_STRING */ "string",
	/* XS_TOKEN_SUBTRACT */ "-",
	/* XS_TOKEN_SUBTRACT_ASSIGN */ "-=",
	/* XS_TOKEN_SUPER */ "super",
	/* XS_TOKEN_SWITCH */ "switch",
	/* XS_TOKEN_THIS */ "this",
	/* XS_TOKEN_THROW */ "throw",
	/* XS_TOKEN_TRUE */ "true",
	/* XS_TOKEN_TRY */ "try",
	/* XS_TOKEN_TYPEOF */ "typeof",
	/* XS_TOKEN_UNSIGNED_RIGHT_SHIFT */ ">>>",
	/* XS_TOKEN_UNSIGNED_RIGHT_SHIFT_ASSIGN */ ">>>=",
	/* XS_TOKEN_VAR */ "var",
	/* XS_TOKEN_VOID */ "void",
	/* XS_TOKEN_WHILE */ "while",
	/* XS_TOKEN_WITH */ "with",
	/* XS_TOKEN_YIELD */ "yield"
};

static txBoolean checkPseudoCrement(txSlot *slot)
{
	txInteger id;
	if(slot->kind!=XS_GET_NEGATIVE_ID)
		return FALSE;
	id = slot->value.integer;
	slot = slot->next;
	if(slot->kind!=XS_DUB)
		return FALSE;
	slot = slot->next->next;
	if(slot->kind!=XS_SET_NEGATIVE_ID || slot->value.integer!=id)
		return FALSE;
	slot = slot->next;
	if(slot->kind!=XS_POP)
		return FALSE;
	slot = slot->next;
	if(slot->kind!=XS_POP)
		return FALSE;
	return TRUE;
}

/*static txSlot *getBranchDest(txSlot* aList,txU4 addr)
{
	txSlot *aSlot;
	aSlot = aList->next;
	while(aSlot) {
		if((txU4)aSlot==addr && aSlot->kind==XS_LABEL)
		{
			return aSlot;
		}
		aSlot = aSlot->next;
	}
	return C_NULL;
	
}*/
txByte* fxParseScript(txMachine* the, void* theStream, txGetter theGetter, 
		txSlot* thePath, long theLine, txFlag theFlags, txSize* theSize)
{
	txSlot* aStack = the->stack;
	txScriptParser *aParser;
	txSlot* aScript;
	txSlot* aSlot;
	txID anID;
	txSize aSize;
	txSize aDelta;
	txSize anOffset;
	txByte* aCode;
	txByte* result;
	txU4 index = 0;
	//txSlot *aSlotTmp;
	txU4 logicPos = 0;
	txU4 dubPos = 0;

	txBoolean branchBoolFlag = FALSE;
	txSlot *prevSlot = C_NULL;
	txSlot *prevPrevSlot = C_NULL;

	aParser = the->parser;
	if (NULL == aParser) {
		aParser = c_malloc(sizeof(txScriptParser));
		if (NULL == aParser) {
			const char *msg = "Out of memory!";
#ifdef mxDebug
			fxDebugLoop(the, (char *)msg);
#endif
			fxThrowMessage(the, XS_UNKNOWN_ERROR, (char *)msg);
		}
		the->parser = aParser;
	}
	c_memset(aParser, 0, sizeof(txScriptParser) - sizeof(aParser->buffer));
	aParser->the = the;
	aParser->stream = theStream;
	aParser->getter = theGetter;
	aParser->path = thePath;
	aParser->line2 = theLine;
	
	mxZeroSlot(--the->stack);
	aParser->flags = the->stack;
	aParser->flags->value.string = mxEmptyString.value.string;
	aParser->flags->kind = mxEmptyString.kind;
	mxZeroSlot(--the->stack);
	aParser->string = the->stack;
	aParser->string->value.string = mxEmptyString.value.string;
	aParser->string->kind = mxEmptyString.kind;
	mxZeroSlot(--the->stack);
	aParser->flags2 = the->stack;
	aParser->flags2->value.string = mxEmptyString.value.string;
	aParser->flags2->kind = mxEmptyString.kind;
	mxZeroSlot(--the->stack);
	aParser->string2 = the->stack;
	aParser->string2->value.string = mxEmptyString.value.string;
	aParser->string2->kind = mxEmptyString.kind;

	if (theFlags & XS_DEBUG_FLAG)
		aParser->debug = 1;
	if (theFlags & XS_SANDBOX_FLAG)
		aParser->sandbox = 1;
	if (theFlags & XS_STRICT_FLAG)
		aParser->strict = 1;

	fxGetNextCharacter(aParser);
	fxGetNextToken(aParser);
	fxGetNextToken(aParser);

	aScript = fxNewList(aParser);
	if (theFlags & (XS_EVAL_FLAG | XS_PROGRAM_FLAG | XS_THIS_FLAG)) {
		aParser->level = -1;
	}
	else {
		aParser->arguments = fxNewList(aParser);
		fxArgumentBlock(aParser, aScript);
		fxMatchToken(aParser, XS_TOKEN_LEFT_BRACE);
	}
	aParser->variables = fxNewList(aParser);
	aParser->functions = fxNewList(aParser);
	aParser->parameters = 0;
	fxBody(aParser, aScript, C_NULL, theFlags);
	if (!(theFlags & (XS_EVAL_FLAG | XS_PROGRAM_FLAG | XS_THIS_FLAG))) {
		fxMatchToken(aParser, XS_TOKEN_RIGHT_BRACE);
	}
	fxMatchToken(aParser, XS_TOKEN_EOF);
	if (aParser->errorCount > 0) {
		c_strcpy(aParser->buffer, "ECMAScript error(s)!");
#ifdef mxDebug
        fxDebugLoop(the, "ECMAScript error(s) in: %s:%d", aParser->path ? aParser->path->value.symbol.string : "(unknown)", aParser->lineFirstError);
#endif
		fxThrowMessage(the, XS_SYNTAX_ERROR, aParser->buffer);
	}
	
	

	aSlot = aScript->value.list.first;
	aSize = 0;
	aDelta = 0;
	while (aSlot) {
		switch (aSlot->kind) {
		case XS_LABEL:
			aSlot->value.integer = aSize;
			break;
			
		case XS_BEGIN:
			aSize += 4;
			break;
		
		case XS_GET_AT:
		{
			if(aSlot->next->kind==XS_GET_AT) {
				if(aSlot->next->next->kind!=XS_LESS && aSlot->next->next->kind!=XS_LESS_EQUAL
				&& aSlot->next->next->kind!=XS_CALL && aSlot->next->next->kind!=XS_NEW) {
						aSlot->kind = PSEUDO_DOUBLE_GET_AT;
				}
			}
			aSize += 2;
			break;
		}
		
		case XS_SET_AT:
		{
			if(aSlot->next->kind==XS_POP){
				txInteger valIndex = aSlot->value.integer;
				txSlot *aSlot0 = aSlot->next;
				txSlot *curSlot = aSlot->next->next;
				PASS_LINE(curSlot);
				if(curSlot->kind==XS_GET_AT)
					if(curSlot->value.integer == valIndex) {
						aSlot0->kind = XS_NOP;
						curSlot->kind = XS_NOP;
					}
			}
			aSize += 2;
			break;
		}
		
		case XS_SET_NEGATIVE_ID:
		{
			if(aSlot->next->kind==XS_POP) {
				txInteger valIndex = aSlot->value.integer;
				txSlot *aSlot0 = aSlot->next;
				txSlot *curSlot = aSlot->next->next;
				PASS_LINE(curSlot);
				if(curSlot->kind==XS_GET_NEGATIVE_ID)
					if(curSlot->value.integer == valIndex) {
						aSlot0->kind = XS_NOP;
						curSlot->kind = XS_NOP;
					}
			}
			aSize += 2;
			break;
		}
		
		case XS_INCREMENT:
			if(checkPseudoCrement(prevPrevSlot))
			{
				prevPrevSlot->kind = PSEUDO_INCREMENT;
				prevSlot->kind = XS_NOP;
				aSlot->next->next->kind = XS_NOP;
				aSize--;
			}
			aSize++;
			break;
			
		case XS_DECREMENT:
			if(checkPseudoCrement(prevPrevSlot))
			{
				prevPrevSlot->kind = PSEUDO_DECREMENT;
				prevSlot->kind = XS_NOP;
				aSlot->next->next->kind = XS_NOP;
				aSize--;
			}
			aSize++;
			break;


			
		case XS_GET:
		{
			txSlot *curSlot = aSlot->next;
			PASS_LABEL(curSlot);
			//if(curSlot->kind!=XS_CALL && curSlot->kind!=XS_NEW)
			if(curSlot->kind==XS_CALL || curSlot->kind==XS_NEW)
				aSlot->kind = XS_GET_FOR_NEW;
		}
		aSize += 3;
		break;
		
		case XS_SET_MEMBER:
		{
			if(prevSlot->kind==XS_THIS) {
				if(aSlot->next->kind==XS_POP) {
					prevSlot->kind = PSEUDO_SET_MEMBER;
				}
			}
			aSize += 3;
			break;
		}
		
		case XS_GET_MEMBER:
		{
			if(prevSlot->kind==XS_THIS){
				txSlot *curSlot = aSlot->next;
				PASS_LABEL(curSlot);
				if(curSlot->kind!=XS_CALL && curSlot->kind!=XS_NEW) {
					prevSlot->kind = PSEUDO_GET_MEMBER;
				}
				else
					aSlot->kind = XS_GET_MEMBER_FOR_CALL;
			}
			else if(prevSlot->kind==XS_GET || prevSlot->kind==XS_GET_NEGATIVE_ID || prevSlot->kind==XS_GET_AT) {
				if(aSlot->next->kind==prevSlot->kind && aSlot->next->value.integer==prevSlot->value.integer) {
					if(aSlot->next->next->kind==XS_GET_MEMBER && aSlot->next->next->value.reference==aSlot->value.reference) {
								aSlot->next->kind = XS_DUB;
								aSlot->next->next->kind = XS_NOP;
					}
					else {
						txSlot *curSlot = aSlot->next;
						PASS_LABEL(curSlot);
						if(curSlot->kind==XS_CALL || curSlot->kind==XS_NEW)
							aSlot->kind = XS_GET_MEMBER_FOR_CALL;				
					}
				}
				else {
					txSlot *curSlot = aSlot->next;
					PASS_LABEL(curSlot);
					if(curSlot->kind==XS_CALL || curSlot->kind==XS_NEW)
						aSlot->kind = XS_GET_MEMBER_FOR_CALL;
				}
			}
			/*else if(prevSlot->kind==XS_LABEL) {
				txSlot *curSlot = aSlot->next;
				PASS_LINE(curSlot);
				if(curSlot->kind!=XS_CALL && curSlot->kind!=XS_NEW) {
					prevPrevSlot->kind = (prevPrevSlot->kind==XS_THIS)?PSEUDO_GET_MEMBER:prevPrevSlot->kind;
				}
				else
					aSlot->kind = XS_GET_MEMBER_FOR_CALL;
			}*/
			else {
				txSlot *curSlot = aSlot->next;
				PASS_LABEL(curSlot);
				if(curSlot->kind==XS_CALL || curSlot->kind==XS_NEW)
					aSlot->kind = XS_GET_MEMBER_FOR_CALL;
			}
			aSize += 3;
			break;
		}

		case XS_PUT_MEMBER:
			/*if(prevSlot->kind==XS_SWAP) {
				if(aSlot->next->kind==XS_POP) {
					prevSlot->kind = XS_NOP;
					aSize--;
					aSlot->kind = PSEUDO_PUT_MEMBER;
					aSlot->next->kind = XS_NOP;
				}
			}*/
			aSize += 5;
			break;
			
		case XS_EQUAL:
		{
			aSize++;
			logicPos = index;
			branchBoolFlag = TRUE;
			break;
		}
		
		case XS_NOT:
		case XS_NOT_EQUAL:
		case XS_STRICT_EQUAL:
		case XS_STRICT_NOT_EQUAL:
		case XS_MORE:
		case XS_MORE_EQUAL:
		{   
			aSize++;
			logicPos = index;
			branchBoolFlag = TRUE;
			break;
		}
			
		case XS_BRANCH:
		{
			/*aSlotTmp = getBranchDest(aSlot,(txU4)aSlot->value.reference);
			if(aSlotTmp)
			{
				aSlotTmp = aSlotTmp->next;
				PASS_LABEL(aSlotTmp);
				if(aSlotTmp) {
					aSlot->value.reference = (aSlotTmp->kind==XS_BRANCH)?aSlotTmp->value.reference:aSlot->value.reference;
				}
			}*/
			aSize += 3;
			aDelta += 2;
			break;
		}
		case XS_BRANCH_ELSE:
		case XS_BRANCH_IF:
		{
			/*aSlotTmp = getBranchDest(aSlot,(txU4)aSlot->value.reference);
			if(aSlotTmp)
			{
				aSlotTmp = aSlotTmp->next;
				PASS_LABEL(aSlotTmp);
				if(aSlotTmp) {
					if(aSlotTmp->kind==XS_BRANCH) {
						aSlot->value.reference = aSlotTmp->value.reference;
					}
					else if(aSlotTmp->kind==aSlot->kind) {
						if(prevSlot->kind==XS_DUB) {
							if(aSlot->next->kind==XS_POP){
								prevSlot->kind = XS_NOP;
								aSize--;
								aSlot->next->kind = XS_NOP;
								aSlot->value.reference = aSlotTmp->value.reference;
								
							}
						}
					}
				}
			}*/
				
			if(branchBoolFlag) {
				if(((index-logicPos)==1)||((index-logicPos)==2 && (index - dubPos)==1)) {
					aSlot->kind += XS_BRANCH_ELSE_BOOL - XS_BRANCH_ELSE;
				}						
				branchBoolFlag = FALSE;
				logicPos = 0;
			}
			if(aSlot->kind==XS_BRANCH_ELSE) {
				prevSlot->kind = (prevSlot->kind==XS_GET_NEGATIVE_ID)?PSEUDO_BRANCH_ELSE0:prevSlot->kind;
				prevSlot->kind = (prevSlot->kind==XS_GET_AT)?PSEUDO_BRANCH_ELSE1:prevSlot->kind;
			}
			aSize += 3;
			aDelta += 2;
			break;
		}
		
		case XS_FUNCTION:
		case XS_ROUTE:
			aSize += 3;
			aDelta += 2;
			break;
		case XS_THROW:
			aSize++;
			break;
		case XS_LOCAL:
		case XS_DELETE_AT:
		case XS_INTEGER_8:
			aSize += 2;
			break;
		case XS_CATCH:
		case XS_DELETE:
		case XS_DELETE_MEMBER:
		case XS_FILE:
		case XS_SET:
		case XS_PUT_MEMBER_AT:
		case XS_INTEGER_16:
		case XS_STATUS:
			aSize += 3;
			break;
		case XS_INTEGER_32:
			aSize += 5;
			break;
		case XS_NUMBER:
			aSize += 9;
			break;
		case XS_STRING:
			if(stackUsage[aSlot->next->kind] == STACK_DOUBLE_OP || stackUsage[aSlot->next->kind] == STACK_SINGLE_OP || stackUsage[aSlot->next->kind] == STACK_INDEX)
				aSlot->kind = XS_STRING_POINTER;
			else if(stackUsage[aSlot->next->kind] == STACK_DEC && stackUsage[aSlot->next->next->kind] == STACK_DOUBLE_OP)
				aSlot->kind = XS_STRING_POINTER;
			else if(aSlot->next->kind == XS_ADD)
				aSlot->kind = XS_STRING_CONCAT;
			else if((aSlot->next->kind == XS_GET_AT || aSlot->next->kind == XS_GET_NEGATIVE_ID) && aSlot->next->next->kind==XS_ADD) {
				txSlot tmp = *aSlot->next;
				aSlot->next->kind = XS_STRING_CONCATBY;
				aSlot->next->value.string = aSlot->value.string;
				aSlot->kind = tmp.kind;
				aSlot->value.integer = tmp.value.integer;
				aSize += 2;
				aSlot->next->next->kind = XS_NOP;
				break;
			}
			aSize += 1 + (int)c_strlen(aSlot->value.string) + 1 + stringCommandOffset;
			break;

		case XS_STRING_CONCATBY:
			aSize += 1 + (int)c_strlen(aSlot->value.string) + 1 + stringCommandOffset;
			break;
			
		case XS_LINE:
			aSize += 3;
			break;
			
		case XS_LESS:
		{
			if(prevSlot->kind == XS_GET_NEGATIVE_ID) {
				if(prevPrevSlot->kind==XS_GET_NEGATIVE_ID) {
					if(aSlot->next->kind==XS_BRANCH_ELSE){
						prevPrevSlot->kind = PSEUDO_LESS;
					}
				}
			}
			logicPos = index;
			branchBoolFlag = TRUE;
			aSize++;
			break;
		}
		
		case XS_LESS_EQUAL:
		{
			if(prevSlot->kind == XS_GET_NEGATIVE_ID) {
				if(prevPrevSlot->kind==XS_GET_NEGATIVE_ID) {
					prevPrevSlot->kind = (aSlot->next->kind==XS_BRANCH_ELSE)?PSEUDO_LESS_EQUAL:prevPrevSlot->kind;
				}
			}
			logicPos = index;
			branchBoolFlag = TRUE;
			aSize++;
			break;
		}
			
		case XS_GET_NEGATIVE_ID:
		{
			if(aSlot->next->kind==XS_GET_NEGATIVE_ID) {
				if(aSlot->next->next->kind!=XS_LESS && aSlot->next->next->kind!=XS_LESS_EQUAL
				&& aSlot->next->next->kind!=XS_CALL && aSlot->next->next->kind!=XS_NEW) {
					aSlot->kind = PSEUDO_DOUBLE_GET_NEGATIVE;
				}
			}
			aSize += 2;
			break;
		}
		
		case XS_DUB:
		{
			if(prevSlot->kind==XS_POP && prevPrevSlot->kind!=XS_CALL) {
				prevSlot->kind = PSEUDO_DUB;
				aSlot->kind = XS_NOP;
			}
			else {
				dubPos = index;
				aSize++;
			}
			break;
		}
			
		case XS_NOP:
			break;
		default:
			aSize++;
			break;
		}
		index++;
		prevPrevSlot = prevSlot;
		prevSlot = aSlot;
		aSlot = aSlot->next;
	}
	if (aSize > 32767) {
		aSlot = aScript->value.list.first;
		aSize = 0;
		while (aSlot) {
			aSlot->ID = (txID)aSize;
			switch (aSlot->kind) {
			case XS_LABEL:
				aSlot->value.integer = aSize;
				break;
				
			case XS_BEGIN:
				aSize += 4;
				break;
			case XS_LOCAL:
			case XS_DELETE_AT:
			case XS_GET_AT:
			case XS_SET_AT:
			case PSEUDO_BRANCH_ELSE1:
			case XS_INTEGER_8:
			case PSEUDO_INCREMENT:
			case PSEUDO_DECREMENT:
			case PSEUDO_LESS:
			case PSEUDO_LESS_EQUAL:
			case XS_SET_NEGATIVE_ID:
			case XS_GET_NEGATIVE_ID:
			case PSEUDO_DOUBLE_GET_NEGATIVE:
			case PSEUDO_DOUBLE_GET_AT:
			case PSEUDO_BRANCH_ELSE0:
				aSize += 2;
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
			case XS_PUT_MEMBER_AT:
			case XS_INTEGER_16:
			case XS_STATUS:
			case XS_LINE:
				aSize += 3;
				break;
				
			case XS_PUT_MEMBER:
			case PSEUDO_PUT_MEMBER:
				aSize += 5;
				break;
				
			case XS_BRANCH:
			case XS_BRANCH_ELSE:
			case XS_BRANCH_IF:
			case XS_BRANCH_ELSE_BOOL:
			case XS_BRANCH_IF_BOOL:
			case XS_FUNCTION:
			case XS_ROUTE:
				anOffset = aSlot->value.reference->value.integer - aSize + 3;
				if ((anOffset < -32768) || ((anOffset + aDelta) > 32767)) {
					if (aSlot->kind == XS_BRANCH) aSlot->kind = XS_BRANCH2;
					else if (aSlot->kind == XS_BRANCH_ELSE) aSlot->kind = XS_BRANCH_ELSE2;
					else if (aSlot->kind == XS_BRANCH_IF) aSlot->kind = XS_BRANCH_IF2;
					else if (aSlot->kind == XS_BRANCH_ELSE_BOOL) aSlot->kind = XS_BRANCH_ELSE_BOOL2;
					else if (aSlot->kind == XS_BRANCH_IF_BOOL) aSlot->kind = XS_BRANCH_IF_BOOL2;
					else if (aSlot->kind == XS_FUNCTION) aSlot->kind = XS_FUNCTION2;
					else if (aSlot->kind == XS_ROUTE) aSlot->kind = XS_ROUTE2;
					aSize += 5;
				}
				else {
					aSize += 3;
					aDelta -= 2;
				}
				break;
			
			case XS_INTEGER_32:
				aSize += 5;
				break;
			case XS_NUMBER:
				aSize += 9;
				break;
			case XS_STRING_POINTER:
			case XS_STRING_CONCAT:
			case XS_STRING_CONCATBY:
			case XS_STRING:
				aSize += 1 + (int)c_strlen(aSlot->value.string) + 1 + stringCommandOffset;
				break;
				
			case XS_NOP:
				break;
			default:
				aSize++;
				break;
			}
            aSlot = aSlot->next;
		}
	}

	result = (txByte *)fxNewChunk(the, aSize);
 	if (NULL == result) {
		const char *msg = "Out of memory!";
#ifdef mxDebug
		fxDebugLoop(the, (char *)msg);
#endif
		fxThrowMessage(the, XS_UNKNOWN_ERROR, (char *)msg);
	}
	if (theSize)
		*theSize = aSize;

	aSlot = aScript->value.list.first;
	aCode = result;
	while (aSlot) {
		switch (aSlot->kind) {
		case XS_LABEL:
			break;
			
		case XS_BEGIN:
			*aCode = aSlot->kind;
			aCode++;
			if (aSlot->value.reference)
				anID = aSlot->value.reference->ID;
			else
				anID = (txID)XS_NO_ID;
			mxEncode2(aCode, anID);
			*aCode = aSlot->flag;
			aCode++;
			break;
		case XS_LOCALS:
			*aCode = (txByte)aSlot->value.integer;
			aCode++;
			break;
		case XS_LOCAL:
			if (aSlot->value.label.symbol)
				anID = aSlot->value.label.symbol->ID;
			else
				anID = (txID)XS_NO_ID;
			mxEncode2(aCode, anID);
			break;

		case XS_DELETE_AT:
		case XS_GET_AT:
		case XS_SET_AT:
		case XS_BIND:
		case XS_INTEGER_8:
		case PSEUDO_BRANCH_ELSE0:
		case PSEUDO_BRANCH_ELSE1:
		case PSEUDO_INCREMENT:
		case PSEUDO_DECREMENT:
		case PSEUDO_LESS:
		case PSEUDO_LESS_EQUAL:
		case PSEUDO_DOUBLE_GET_NEGATIVE:
		case PSEUDO_DOUBLE_GET_AT:
		case XS_SET_NEGATIVE_ID:
		case XS_GET_NEGATIVE_ID:
			*aCode = aSlot->kind;
			*(aCode + 1) = (txByte)aSlot->value.integer;
			aCode += 2;
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
			*aCode = aSlot->kind;
			aCode++;
			if (aSlot->value.reference)
				anID = aSlot->value.reference->ID;
			else
				anID = (txID)XS_NO_ID;
			mxEncode2(aCode, anID);
			break;
			
		case XS_PUT_MEMBER:
		case PSEUDO_PUT_MEMBER:
			*aCode = aSlot->kind;;
			aCode++;
			anID = aSlot->value.reference->ID;
			mxEncode2(aCode, anID);
			*aCode = aSlot->flag;
			*(aCode + 1) = aSlot->flag;
			aCode += 2;
			break;
		case XS_PUT_MEMBER_AT:
			*aCode = aSlot->kind;
			*(aCode + 1) = aSlot->flag;
			*(aCode + 2) = aSlot->flag;
			aCode += 3;
			break;
	
		case XS_BRANCH:
		case XS_BRANCH_ELSE:
		case XS_BRANCH_IF:
		case XS_BRANCH_ELSE_BOOL:
		case XS_BRANCH_IF_BOOL:
		case XS_FUNCTION:
		case XS_ROUTE:
			anID = (txID)(aSlot->value.reference->value.integer - ((aCode - result) + 3));
			*aCode = aSlot->kind;
			aCode++;
			mxEncode2(aCode, anID);
			break;
	
		case XS_BRANCH2:
		case XS_BRANCH_ELSE2:
		case XS_BRANCH_IF2:
		case XS_BRANCH_ELSE_BOOL2:
		case XS_BRANCH_IF_BOOL2:
		case XS_FUNCTION2:
		case XS_ROUTE2:
			aDelta = aSlot->value.reference->value.integer - ((aCode - result) + 5);
			*aCode = aSlot->kind;
			aCode++;
			mxEncode4(aCode, aDelta);
			break;
		
		case XS_INTEGER_16:
		case XS_STATUS:
			*aCode = aSlot->kind;
			aCode++;
			anID = (txID)aSlot->value.integer;
			mxEncode2(aCode, anID);
			break;
		case XS_INTEGER_32:
			*aCode = aSlot->kind;
			aCode++;
			mxEncode4(aCode, aSlot->value.integer);
			break;
		case XS_NUMBER:
			*aCode = aSlot->kind;
			aCode++;
			mxEncode8(aCode, aSlot->value.number);
			break;
		case XS_STRING_POINTER:
		case XS_STRING_CONCAT:
		case XS_STRING_CONCATBY:
		case XS_STRING:
		{
			*aCode = aSlot->kind;
			aCode++;
			anID = (txID)(c_strlen(aSlot->value.string)) + 1;
#if mxStringLength
			mxEncode2(aCode,anID);
#endif
			c_memcpy(aCode, aSlot->value.string, anID);
			aCode += anID;
		}break;
		
		case XS_LINE:
			*aCode = aSlot->kind;
			aCode++;
			anID = (txID)(aSlot->value.integer);
			mxEncode2(aCode, anID);
			break;
		
		case XS_NOP:
			break;
		default:
			*aCode = aSlot->kind;
			aCode++;
			break;
		}
		aSlot = aSlot->next;
	}
	
	the->stack = aStack;
	return result;
}

/* JSON */

#include "xsJSON.c"

/* lexical */
#include "xsLexical.c"

/* syntaxical */

#include "xsSyntaxical.c"

/* utilities */

txSlot* fxAppendArguments(txScriptParser* theParser, txSlot* theList)
{
	txSlot* result = fxAppendCode(theParser, theList, XS_LOCALS);
	if (theParser->arguments) {
		if (theParser->arguments->value.list.last) {
			result->value.integer = theParser->arguments->value.list.last->value.local.index + 1;
			fxPasteList(theParser, theList, theParser->arguments);
		}
	}
	return result;
}

txSlot* fxAppendCode(txScriptParser* theParser, txSlot* theList, txKind theCode)
{
	txSlot* result = fxNewSlot(theParser->the);
	result->next = C_NULL;
	result->ID = 0;
	result->flag = XS_NO_FLAG;
	result->kind = theCode;
	result->value.list.first = C_NULL;
	result->value.list.last = C_NULL;
	return fxAppendLabel(theParser, theList, result);
}

txSlot* fxAppendInteger(txScriptParser* theParser, txSlot* theList, txInteger theInteger)
{
	txSlot* result;
	if ((-128 <= theInteger) && (theInteger <= 127)) {
		result = fxAppendCode(theParser, theList, XS_INTEGER_8);
		result->value.integer = theInteger;
	}
	else if ((-32768 <= theInteger) && (theInteger <= 32767)) {
		result = fxAppendCode(theParser, theList, XS_INTEGER_16);
		result->value.integer = theInteger;
	}
	else {
		result = fxAppendCode(theParser, theList, XS_INTEGER_32);
		result->value.integer = theInteger;
	}
	return result;
}

txSlot* fxAppendLabel(txScriptParser* theParser, txSlot* theList, txSlot* theLabel)
{
	if (theList->value.list.first == C_NULL)
		theList->value.list.first = (txSlot*)theLabel;
	else
		theList->value.list.last->next = (txSlot*)theLabel;
	theList->value.list.last = (txSlot*)theLabel;
	return theLabel;
} 

txSlot* fxAppendLine(txScriptParser* theParser, txSlot* theList)
{
	txSlot* result = C_NULL;
	
	if (theParser->debug && theParser->path && (theParser->line0 != theParser->line)) {
		theParser->line0 = theParser->line;
		result = fxAppendCode(theParser, theList, XS_LINE);
		result->value.integer = theParser->line;
	}
	return result;
}

txSlot* fxAppendNumber(txScriptParser* theParser, txSlot* theList, txNumber theNumber)
{
	txSlot* result = fxAppendCode(theParser, theList, XS_NUMBER);
	result->value.number = theNumber;
	return result;
}

txSlot* fxAppendReference(txScriptParser* theParser, txSlot* theList, txKind theCode, txSlot* theReference)
{
	txSlot* result = fxAppendCode(theParser, theList, theCode);
	result->value.reference = theReference;
	return result;
}

txSlot* fxAppendStatus(txScriptParser* theParser, txSlot* theList, txID theID)
{
	txSlot* result = fxAppendCode(theParser, theList, XS_STATUS);
	result->value.integer = theID;
	return result;
}

txSlot* fxAppendString(txScriptParser* theParser, txSlot* theList, txString theString)
{
	txSlot* result = fxAppendCode(theParser, theList, XS_STRING);
	result->value.string = theString;
	return result;
}

txSlot* fxAppendVariables(txScriptParser* theParser, txSlot* theList)
{
	txSlot* result = fxAppendCode(theParser, theList, XS_LOCALS);
	if (theParser->variables) {
		if (theParser->variables->value.list.last) {
			result->value.integer = theParser->variables->value.list.last->value.local.index + 1;
			fxPasteList(theParser, theList, theParser->variables);
		}
	}
	return result;
}

void fxBranchLabel(txScriptParser* theParser, txSlot* theLabel, txSlot* theList, txSlot* theBranch)
{
	while (theLabel) {
		theLabel->value.label.symbol = theLabel->next;
		theLabel->next = C_NULL;
		fxAppendLabel(theParser, theList, theLabel);
		fxAppendStatus(theParser, theList, theLabel->ID);
		fxAppendReference(theParser, theList, XS_BRANCH, theBranch);
		theLabel = theLabel->value.label.link;
	}
}

void fxCheckArguments(txScriptParser* theParser)
{
	txSlot* aLocal;
	txSlot* aNextLocal;
	
	aLocal = theParser->arguments->value.list.first;
	while (aLocal) {
		aNextLocal = aLocal->next;
		while (aNextLocal) {
			if (aLocal->value.local.symbol == aNextLocal->value.local.symbol) {
				fxReportParserError(theParser, "parameter already defined");
				return;
			}
			aNextLocal = aNextLocal->next;
		}
		aLocal = aLocal->next;
	}
}

txSlot* fxCopyList(txScriptParser* theParser, txSlot* theList, txSlot* fromList, txSlot* fromSlot)
{
	txSlot* aSlot = fromSlot ? fromSlot->next : fromList->value.list.first;
	while (aSlot) {
		fxAppendLabel(theParser, theList, fxDuplicateSlot(theParser->the, aSlot));
		aSlot = aSlot->next;
	}
	return theList;
}

txSlot* fxCutList(txScriptParser* theParser, txSlot* theList, txSlot* fromList, txSlot* fromSlot)
{
	if (fromSlot) {
		theList->value.list.first = fromSlot->next;
		fromSlot->next = C_NULL;
	}
	else {
		theList->value.list.first = fromList->value.list.first;
		fromList->value.list.first = C_NULL;
	}
	theList->value.list.last = fromList->value.list.last;
	fromList->value.list.last = fromSlot;
	return theList;
}

txSlot* fxDubLabel(txScriptParser* theParser, txSlot* theLabel, txID* theID)
{
	txSlot* result = C_NULL;
	txSlot* aLabel;
	txID anID = *theID;
	if (theLabel) {
		result = aLabel = fxNewLabel(theParser, theLabel->value.label.symbol);
		aLabel->next = theLabel;
		aLabel->ID = anID++;
		theLabel = theLabel->value.label.link;
		while (theLabel) {
			if (theLabel->value.label.symbol == C_NULL)
				break;
			aLabel = aLabel->value.label.link = fxNewLabel(theParser, theLabel->value.label.symbol);
			aLabel->next = theLabel;
			aLabel->ID = anID++;
			theLabel = theLabel->value.label.link;
		}
	}
	*theID = anID;
	return result;
}

txSlot* fxFindLabel(txScriptParser* theParser, txSlot* theLabel, txSlot* theSymbol)
{
	if (theSymbol) {
		while (theLabel) {
			if (theLabel->value.label.symbol == theSymbol)
				break;
			theLabel = theLabel->value.label.link;
		}
	}
	else {
		while (theLabel) {
			if (theLabel->value.label.symbol == C_NULL)
				break;
			theLabel = theLabel->value.label.link;
		}
	}
	return theLabel;
}

void fxJumpLabel(txScriptParser* theParser, txSlot* theLabel, txSlot* theList)
{
	while (theLabel) {
		fxAppendReference(theParser, theList, XS_BRANCH, theLabel->value.label.symbol);
		theLabel = theLabel->value.label.link;
	}
}

txSlot* fxNewLabel(txScriptParser* theParser, txSlot* theSymbol)
{
	txSlot* result = fxNewSlot(theParser->the);
	result->next = C_NULL;
	result->ID = XS_NO_ID;
	result->flag = XS_NO_FLAG;
	result->kind = XS_LABEL;
	result->value.label.link = C_NULL;
	result->value.label.symbol = theSymbol;
	
	mxZeroSlot(--theParser->the->stack);
	theParser->the->stack->kind = XS_LIST_KIND;
	theParser->the->stack->value.list.first = result;
	theParser->the->stack->value.list.last = result;
	return result;
}

txSlot* fxNewList(txScriptParser* theParser)
{
	mxZeroSlot(--theParser->the->stack);
	theParser->the->stack->kind = XS_LIST_KIND;
	return theParser->the->stack;
}

txSlot* fxNewLocal(txScriptParser* theParser, txSlot* theSymbol, txSlot* theList)
{
	txSlot* aLocal;
	txSlot* result;
	if (theList == C_NULL) 
		return C_NULL;
	if (theParser->variables == theList) {
		if (theParser->arguments) {
			result = theParser->arguments->value.list.first;
			while (result) {
				if (result->value.local.symbol == theSymbol)
					return result;
				result = result->next;
			}
		}
		result = theParser->variables->value.list.first;
		while (result) {
			if (result->value.local.symbol == theSymbol)
				return result;
			result = result->next;
		}
	}
	aLocal = theList->value.list.last;
	result = fxNewSlot(theParser->the);
	result->next = C_NULL;
	result->flag = XS_NO_FLAG;
	result->kind = XS_LOCAL;
	result->value.local.index = (aLocal) ? aLocal->value.local.index + 1 : 0;
	result->value.local.symbol = theSymbol;
	if (aLocal)
		aLocal->next = result;
	else
		theList->value.list.first = result;
	theList->value.list.last = result;
	return result;
}

txSlot* fxPasteList(txScriptParser* theParser, txSlot* theList, txSlot* fromList)
{
	if (fromList->value.list.first != C_NULL) {
		if (theList->value.list.first == C_NULL)
			theList->value.list.first = fromList->value.list.first;
		else
			theList->value.list.last->next = fromList->value.list.first;
		theList->value.list.last = fromList->value.list.last;
	}
	return theList;
}

void fxPopBreakLabel(txScriptParser* theParser, txSlot* theLabel)
{
	theParser->breakLabel = theLabel->value.label.link;
	theLabel->value.label.link = C_NULL;
}

void fxPopContinueLabel(txScriptParser* theParser, txSlot* theLabel)
{
	theParser->continueLabel = theLabel->value.label.link;
	theLabel->value.label.link = C_NULL;
}

void fxPushBreakLabel(txScriptParser* theParser, txSlot* theLabel)
{
	theLabel->value.label.link = theParser->breakLabel;
	theParser->breakLabel = theLabel;
}

void fxPushContinueLabel(txScriptParser* theParser, txSlot* theLabel)
{
	theLabel->value.label.link = theParser->continueLabel;
	theParser->continueLabel = theLabel;
}

void fxReportParserError(txScriptParser* theParser, txString theFormat, ...)
{
	char *aPath;
	c_va_list arguments;
	
    if (0 == theParser->errorCount)
        theParser->lineFirstError = theParser->line;
	theParser->errorCount++;
	if (theParser->path)
		aPath = theParser->path->value.symbol.string;
	else
		aPath = C_NULL;
	c_va_start(arguments, theFormat);
#ifdef mxColor
	if (theParser->stream2)
		fxColorVReportError(theParser->the, aPath, theParser->line, theFormat, arguments);
	else
#endif
#ifdef mxCreate
    fxKinomaCreateReportError(theParser->the, aPath, theParser->line, theFormat, arguments);
#else
   fxVReportError(theParser->the, aPath, theParser->line, theFormat, arguments);
#endif
	c_va_end(arguments);
}

void fxReportParserWarning(txScriptParser* theParser, txString theFormat, ...)
{
	char *aPath;
	c_va_list arguments;
	
	theParser->warningCount++;
	if (theParser->path)
		aPath = theParser->path->value.symbol.string;
	else
		aPath = C_NULL;
	c_va_start(arguments, theFormat);
#ifdef mxColor
	if (theParser->stream2)
		fxColorVReportWarning(theParser->the, aPath, theParser->line2, theFormat, arguments);
	else
#endif
#ifdef mxCreate
    fxKinomaCreateReportWarning(theParser->the, aPath, theParser->line, theFormat, arguments);
#else
   fxVReportError(theParser->the, aPath, theParser->line2, theFormat, arguments);
#endif
	c_va_end(arguments);
}

void fxScopeGlobals(txScriptParser* theParser, txSlot* theList, txFlag theFlag)
{
	txInteger aLevel;
	txSlot** aSlotAddress = C_NULL;
	txSlot* aSlot = C_NULL;
	txSlot* aLocal = C_NULL;

	aLevel = 0;
	aSlotAddress = &(theList->value.list.first);
	if (!(theFlag & XS_THIS_FLAG))
		return;
	while ((aSlot = *aSlotAddress)) {
		switch (aSlot->kind) {
		case XS_BEGIN:
		case XS_CATCH:
		case XS_SCOPE:
			aLevel++;
			break;
		case XS_END:
		case XS_UNCATCH:
		case XS_UNSCOPE:
			aLevel--;
			break;
			
		case XS_LINE:
			break;
		
		case XS_DELETE:
		case XS_GET:
		case XS_SET:
			if (aLevel == 0) {
				if (aSlot->value.reference) {
					aSlot->kind += XS_DELETE_MEMBER - XS_DELETE;
					aLocal = fxNewSlot(theParser->the);
					aLocal->next = aSlot;
					aLocal->kind = XS_THIS;
					*aSlotAddress = aLocal;
				}
			}
			break;
		}
		aSlotAddress = &(aSlot->next);
	}
	if (theFlag & XS_THIS_FLAG)
		theParser->variables = C_NULL;
}

void fxScopeLocals(txScriptParser* theParser, txSlot* theList, txFlag theFlags)
{
	txInteger aLevel;
/*	txSlot** aSlotAddress = C_NULL; */
	txSlot* aSlot = C_NULL;
	txSlot* aLocal = C_NULL;

	aLevel = 0;
/*	aSlotAddress = &(theList->value.list.first); */
	aSlot = theList->value.list.first;
	while (aSlot) {
		switch (aSlot->kind) {
		case XS_BEGIN:
		case XS_CATCH:
		case XS_SCOPE:
			aLevel++;
			break;
		case XS_END:
		case XS_UNCATCH:
		case XS_UNSCOPE:
			aLevel--;
			break;
			
		case XS_LINE:
			break;
		
		case XS_DELETE:
		case XS_GET:
		case XS_SET:
			if (aSlot->value.reference) {
				if (aLevel == 0) {
					if ((aLocal = fxSearchLocal(theParser, aSlot->value.reference, theParser->arguments))) {
						aSlot->kind += XS_DELETE_AT - XS_DELETE;
						aSlot->value.integer = aLocal->value.local.index;
						if(aSlot->value.integer<0) {
							if(aSlot->kind==XS_GET_AT)
								aSlot->kind = XS_GET_NEGATIVE_ID;
							else if(aSlot->kind==XS_SET_AT)
								aSlot->kind = XS_SET_NEGATIVE_ID;
						}
					}
					else if ((aLocal = fxSearchLocal(theParser, aSlot->value.reference, theParser->variables))) {
						aSlot->value.integer = -1 - aLocal->value.local.index;
						aSlot->kind += XS_DELETE_AT - XS_DELETE;
						if(aSlot->value.integer<0) {
							if(aSlot->kind==XS_GET_AT)
								aSlot->kind = XS_GET_NEGATIVE_ID;
							else if(aSlot->kind==XS_SET_AT)
								aSlot->kind = XS_SET_NEGATIVE_ID;
						}
					}
					/*else {
						txMachine* the = theParser->the;
						the->frame->flag |= XS_SANDBOX_FLAG;
						aLocal = fxGetProperty(the, mxGlobal.value.reference, aSlot->value.reference->ID);
						the->frame->flag &= ~XS_SANDBOX_FLAG;
						if (!aLocal) {
							if (aSlot->kind == XS_DELETE)
								fxReportParserWarning(theParser, "delete %s", aSlot->value.reference->value.symbol.string);
							else if (aSlot->kind == XS_GET)
								fxReportParserWarning(theParser, "get %s", aSlot->value.reference->value.symbol.string);
							else
								fxReportParserWarning(theParser, "set %s", aSlot->value.reference->value.symbol.string);
						}
					}*/
					/*else if (!aFlag && (aSlot->value.reference->ID != theParser->the->argumentsID)) {
						aSlot->kind += XS_DELETE_MEMBER - XS_DELETE;
						aLocal = fxNewSlot(theParser->the);
						aLocal->next = aSlot;
						aLocal->kind = XS_GLOBAL;
						*aSlotAddress = aLocal;
					}*/
				}
			}
			break;
		}
		aSlot = aSlot->next;
	}
}

txSlot* fxSearchList(txScriptParser* theParser, txString theString, txSlot* theList)
{
	txSlot* result;
	
	result = theList->value.list.first;
	while (result) {
		if (c_strcmp(result->value.string, theString) == 0)
			return result;
		result = result->next;
	}
	return C_NULL;
}

txSlot* fxSearchLocal(txScriptParser* theParser, txSlot* theSymbol, txSlot* theList)
{
	txSlot* result;
	
	if (theList) {
		result = theList->value.list.first;
		while (result) {
			if (result->value.local.symbol == theSymbol)
				return result;
			result = result->next;
		}
	}
	return C_NULL;
}


