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
#ifndef __XSSCRIPT__
#define __XSSCRIPT__

enum {
	XS_NO_TOKEN = 0,
	XS_TOKEN_ADD,
	XS_TOKEN_ADD_ASSIGN,
	XS_TOKEN_AND,
	XS_TOKEN_ASSIGN,
	XS_TOKEN_BIT_AND,
	XS_TOKEN_BIT_AND_ASSIGN,
	XS_TOKEN_BIT_NOT,
	XS_TOKEN_BIT_OR,
	XS_TOKEN_BIT_OR_ASSIGN,
	XS_TOKEN_BIT_XOR,
	XS_TOKEN_BIT_XOR_ASSIGN,
	XS_TOKEN_BREAK,
	XS_TOKEN_CASE,
	XS_TOKEN_CATCH,
	XS_TOKEN_CLASS,
	XS_TOKEN_COLON,
	XS_TOKEN_COMMA,
	XS_TOKEN_CONST,
	XS_TOKEN_CONTINUE,
	XS_TOKEN_DEBUGGER,
	XS_TOKEN_DECREMENT,
	XS_TOKEN_DEFAULT,
	XS_TOKEN_DELETE,
	XS_TOKEN_DIVIDE,
	XS_TOKEN_DIVIDE_ASSIGN,
	XS_TOKEN_DO,
	XS_TOKEN_DOT,
	XS_TOKEN_ELSE,
	XS_TOKEN_ENUM,
	XS_TOKEN_EOF,
	XS_TOKEN_EQUAL,
	XS_TOKEN_EXPORT,
	XS_TOKEN_EXTENDS,
	XS_TOKEN_FALSE,
	XS_TOKEN_FINALLY,
	XS_TOKEN_FOR,
	XS_TOKEN_FUNCTION,
	XS_TOKEN_IDENTIFIER,
	XS_TOKEN_IF,
	XS_TOKEN_IMPLEMENTS,
	XS_TOKEN_IMPORT,
	XS_TOKEN_IN,
	XS_TOKEN_INCREMENT,
	XS_TOKEN_INSTANCEOF,
	XS_TOKEN_INTEGER,
	XS_TOKEN_INTERFACE,
	XS_TOKEN_LEFT_BRACE,
	XS_TOKEN_LEFT_BRACKET,
	XS_TOKEN_LEFT_PARENTHESIS,
	XS_TOKEN_LEFT_SHIFT,
	XS_TOKEN_LEFT_SHIFT_ASSIGN,
	XS_TOKEN_LESS,
	XS_TOKEN_LESS_EQUAL,
	XS_TOKEN_LET,
	XS_TOKEN_MODULO,
	XS_TOKEN_MODULO_ASSIGN,
	XS_TOKEN_MORE,
	XS_TOKEN_MORE_EQUAL,
	XS_TOKEN_MULTIPLY,
	XS_TOKEN_MULTIPLY_ASSIGN,
	XS_TOKEN_NEW, 
	XS_TOKEN_NOT,
	XS_TOKEN_NOT_EQUAL,
	XS_TOKEN_NULL, 
	XS_TOKEN_OR,
	XS_TOKEN_PACKAGE,
	XS_TOKEN_PRIVATE,
	XS_TOKEN_PROTECTED,
	XS_TOKEN_PUBLIC,
	XS_TOKEN_QUESTION_MARK,
	XS_TOKEN_NUMBER,
	XS_TOKEN_REGEXP,
	XS_TOKEN_RETURN,
	XS_TOKEN_RIGHT_BRACE,
	XS_TOKEN_RIGHT_BRACKET,
	XS_TOKEN_RIGHT_PARENTHESIS,
	XS_TOKEN_SEMICOLON,
	XS_TOKEN_SHORT,
	XS_TOKEN_SIGNED_RIGHT_SHIFT,
	XS_TOKEN_SIGNED_RIGHT_SHIFT_ASSIGN,
	XS_TOKEN_STATIC,
	XS_TOKEN_STRICT_EQUAL,
	XS_TOKEN_STRICT_NOT_EQUAL,
	XS_TOKEN_STRING,
	XS_TOKEN_SUBTRACT,
	XS_TOKEN_SUBTRACT_ASSIGN,
	XS_TOKEN_SUPER,
	XS_TOKEN_SWITCH,
	XS_TOKEN_THIS,
	XS_TOKEN_THROW,
	XS_TOKEN_TRUE,
	XS_TOKEN_TRY,
	XS_TOKEN_TYPEOF,
	XS_TOKEN_UNSIGNED_RIGHT_SHIFT,
	XS_TOKEN_UNSIGNED_RIGHT_SHIFT_ASSIGN,
	XS_TOKEN_VAR,
	XS_TOKEN_VOID,
	XS_TOKEN_WHILE,
	XS_TOKEN_WITH,
	XS_TOKEN_YIELD,
	XS_TOKEN_COUNT
};

#define XS_TOKEN_BEGIN_STATEMENT 1
#define XS_TOKEN_BEGIN_EXPRESSION 2
#define XS_TOKEN_ASSIGN_EXPRESSION 4
#define XS_TOKEN_EQUAL_EXPRESSION 8
#define XS_TOKEN_RELATIONAL_EXPRESSION 16
#define XS_TOKEN_SHIFT_EXPRESSION 32
#define XS_TOKEN_ADDITIVE_EXPRESSION 64
#define XS_TOKEN_MULTIPLICATIVE_EXPRESSION 128
#define XS_TOKEN_PREFIX_EXPRESSION 256
#define XS_TOKEN_POSTFIX_EXPRESSION 512
#define XS_TOKEN_END_STATEMENT 1024
#define XS_TOKEN_REFERENCE_EXPRESSION 2048
#define XS_TOKEN_NO_REGEXP 4096

/* lexical */
static void fxGetNextCharacter(txScriptParser* theParser);
static void fxGetNextKeyword(txScriptParser* theParser);
static void fxGetNextNumber(txScriptParser* theParser, int parseDot);
static void fxGetNextToken(txScriptParser* theParser);
static void fxMatchToken(txScriptParser* theParser, txToken theToken);
static txBoolean fxIsIdentifierFirst(char c);
static txBoolean fxIsIdentifierNext(char c);
static txU1* fsX2UTF8(txScriptParser* theParser, txU4 c, txU1* p, txU4 theSize);
/* syntaxical */
static void fxBody(txScriptParser* theParser, txSlot* theList, txSlot* theSymbol, txFlag theFlag);
static void fxStatementBlock(txScriptParser* theParser, txSlot* theList);
static void fxStatement(txScriptParser* theParser, txSlot* theList);
static void fxSemicolon(txScriptParser* theParser, txSlot* theList);
static void fxBreakStatement(txScriptParser* theParser, txSlot* theList);
static void fxContinueStatement(txScriptParser* theParser, txSlot* theList);
static void fxDebuggerStatement(txScriptParser* theParser, txSlot* theList);
static void fxDoStatement(txScriptParser* theParser, txSlot* theList);
static void fxForStatement(txScriptParser* theParser, txSlot* theList);
static void fxIfStatement(txScriptParser* theParser, txSlot* theList);
static void fxReturnStatement(txScriptParser* theParser, txSlot* theList);
static void fxSwitchStatement(txScriptParser* theParser, txSlot* theList);
static void fxThrowStatement(txScriptParser* theParser, txSlot* theList);
static void fxTryStatement(txScriptParser* theParser, txSlot* theList);
static void fxVarStatement(txScriptParser* theParser, txSlot* theList);
static txInteger fxVarStatementNoIn(txScriptParser* theParser, txSlot* theList);
static void fxWhileStatement(txScriptParser* theParser, txSlot* theList);
static void fxWithStatement(txScriptParser* theParser, txSlot* theList);
static void fxCommaExpression(txScriptParser* theParser, txSlot* theList);
static txInteger fxCommaExpressionNoIn(txScriptParser* theParser, txSlot* theList);
static void fxAssignmentExpression(txScriptParser* theParser, txSlot* theList);
static void fxConditionalExpression(txScriptParser* theParser, txSlot* theList);
static void fxOrExpression(txScriptParser* theParser, txSlot* theList);
static void fxAndExpression(txScriptParser* theParser, txSlot* theList);
static void fxBitOrExpression(txScriptParser* theParser, txSlot* theList);
static void fxBitXorExpression(txScriptParser* theParser, txSlot* theList);
static void fxBitAndExpression(txScriptParser* theParser, txSlot* theList);
static void fxEqualExpression(txScriptParser* theParser, txSlot* theList);
static void fxRelationalExpression(txScriptParser* theParser, txSlot* theList);
static void fxShiftExpression(txScriptParser* theParser, txSlot* theList);
static void fxAdditiveExpression(txScriptParser* theParser, txSlot* theList);
static void fxMultiplicativeExpression(txScriptParser* theParser, txSlot* theList);
static void fxPrefixExpression(txScriptParser* theParser, txSlot* theList);
static void fxPostfixExpression(txScriptParser* theParser, txSlot* theList);
static void fxNewExpression(txScriptParser* theParser, txSlot* theList);
static void fxCallExpression(txScriptParser* theParser, txSlot* theList);
static void fxLiteralExpression(txScriptParser* theParser, txSlot* theList);
static void fxFunctionExpression(txScriptParser* theParser, txSlot* theList, txFlag declareIt);
static void fxObjectExpression(txScriptParser* theParser, txSlot* theList);
static void fxArrayExpression(txScriptParser* theParser, txSlot* theList);
static void fxParameterBlock(txScriptParser* theParser, txSlot* theList);
static void fxArgumentBlock(txScriptParser* theParser, txSlot* theList);
static void fxCheckNoArgumentsNoEval(txScriptParser* theParser, txSlot* theSlot);
/* utilities */
static txSlot* fxAppendArguments(txScriptParser* theParser, txSlot* theList);
static txSlot* fxAppendCode(txScriptParser* theParser, txSlot* theList, txKind theCode);
static txSlot* fxAppendLabel(txScriptParser* theParser, txSlot* theList, txSlot* theSlot);
static txSlot* fxAppendLine(txScriptParser* theParser, txSlot* theList);
static txSlot* fxAppendInteger(txScriptParser* theParser, txSlot* theList, txInteger theInteger);
static txSlot* fxAppendNumber(txScriptParser* theParser, txSlot* theList, txNumber theNumber);
static txSlot* fxAppendReference(txScriptParser* theParser, txSlot* theList, txKind theCode, txSlot* theReference);
static txSlot* fxAppendStatus(txScriptParser* theParser, txSlot* theList, txID theID);
static txSlot* fxAppendString(txScriptParser* theParser, txSlot* theList, txString theString);
static txSlot* fxAppendVariables(txScriptParser* theParser, txSlot* theList);
static void fxBranchLabel(txScriptParser* theParser, txSlot* theLabel, txSlot* theList, txSlot* theBranch);
static void fxCheckArguments(txScriptParser* theParser);
static txSlot* fxCopyList(txScriptParser* theParser, txSlot* theList, txSlot* fromList, txSlot* fromSlot);
static txSlot* fxCutList(txScriptParser* theParser, txSlot* theList, txSlot* fromList, txSlot* fromSlot);
static txSlot* fxDubLabel(txScriptParser* theParser, txSlot* theLabel, txID* theID);
static txSlot* fxFindLabel(txScriptParser* theParser, txSlot* theList, txSlot* theSymbol);
static void fxJumpLabel(txScriptParser* theParser, txSlot* theLabel, txSlot* theList);
static txSlot* fxNewLabel(txScriptParser* theParser, txSlot* theSymbol);
static txSlot* fxNewList(txScriptParser* theParser);
static txSlot* fxNewLocal(txScriptParser* theParser, txSlot* theSymbol, txSlot* theList);
static txSlot* fxPasteList(txScriptParser* theParser, txSlot* theList, txSlot* fromList);
static void fxPopBreakLabel(txScriptParser* theParser, txSlot* theLabel);
static void fxPopContinueLabel(txScriptParser* theParser, txSlot* theLabel);
static void fxPushBreakLabel(txScriptParser* theParser, txSlot* theLabel);
static void fxPushContinueLabel(txScriptParser* theParser, txSlot* theLabel);
static void fxReportParserError(txScriptParser* theParser, txString theFormat, ...);
static void fxReportParserWarning(txScriptParser* theParser, txString theFormat, ...);
static void fxScopeGlobals(txScriptParser* theParser, txSlot* theList, txFlag theFlag);
static void fxScopeLocals(txScriptParser* theParser, txSlot* theList, txFlag theFlag);
static txSlot* fxSearchList(txScriptParser* theParser, txString theString, txSlot* theList);
static txSlot* fxSearchLocal(txScriptParser* theParser, txSlot* theSymbol, txSlot* theList);

/* json */
static void fx_JSON_parse(txMachine* the);
static void fxGetNextJSONKeyword(txScriptParser* theParser);
static void fxGetNextJSONToken(txScriptParser* theParser);
static void fxParseJSONValue(txScriptParser* theParser);
static void fxParseJSONObject(txScriptParser* theParser);
static void fxParseJSONArray(txScriptParser* theParser);
typedef struct {
	txString buffer;
	char indent[16];
	txInteger level;
	txSize offset;
	txSize size;
	txSlot* replacer;
	txSlot* stack;
} txJSONSerializer;
static void fx_JSON_stringify(txMachine* the);
static void fxSerializeJSONChar(txMachine* the, txJSONSerializer* theSerializer, char c);
static void fxSerializeJSONChars(txMachine* the, txJSONSerializer* theSerializer, char* s);
static void fxSerializeJSONError(txMachine* the, txJSONSerializer* theSerializer, txError theError);
static void fxSerializeJSONIndent(txMachine* the, txJSONSerializer* theSerializer);
static void fxSerializeJSONInteger(txMachine* the, txJSONSerializer* theSerializer, txInteger theInteger);
static void fxSerializeJSONName(txMachine* the, txJSONSerializer* theSerializer, txFlag* theFlag);
static void fxSerializeJSONNumber(txMachine* the, txJSONSerializer* theSerializer, txNumber theNumber);
static void fxSerializeJSONProperty(txMachine* the, txJSONSerializer* theSerializer, txFlag* theFlag);
static void fxSerializeJSONString(txMachine* the, txJSONSerializer* theStream, txString theString);

#endif


