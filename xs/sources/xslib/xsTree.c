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
#include "xsScript.c"

static void fxTreeBody(txScriptParser* theParser, txSlot* theResult, txSlot* theSymbol, txFlag theFlag);
static void fxTreeStatementBlock(txScriptParser* theParser, txSlot* theResult);
static void fxTreeStatement(txScriptParser* theParser, txSlot* theResult);
static void fxTreeBreakStatement(txScriptParser* theParser, txSlot* theResult);
static void fxTreeContinueStatement(txScriptParser* theParser, txSlot* theResult);
static void fxTreeDebuggerStatement(txScriptParser* theParser, txSlot* theResult);
static void fxTreeDoStatement(txScriptParser* theParser, txSlot* theResult);
static void fxTreeForStatement(txScriptParser* theParser, txSlot* theResult);
static void fxTreeIfStatement(txScriptParser* theParser, txSlot* theResult);
static void fxTreeReturnStatement(txScriptParser* theParser, txSlot* theResult);
static void fxTreeSwitchStatement(txScriptParser* theParser, txSlot* theResult);
static void fxTreeThrowStatement(txScriptParser* theParser, txSlot* theResult);
static void fxTreeTryStatement(txScriptParser* theParser, txSlot* theResult);
static void fxTreeVarStatement(txScriptParser* theParser, txSlot* theResult);
static txInteger fxTreeVarStatementNoIn(txScriptParser* theParser, txSlot* theResult);
static void fxTreeWhileStatement(txScriptParser* theParser, txSlot* theResult);
static void fxTreeWithStatement(txScriptParser* theParser, txSlot* theResult);
static void fxTreeCommaExpression(txScriptParser* theParser, txSlot* theResult);
static txInteger fxTreeCommaExpressionNoIn(txScriptParser* theParser, txSlot* theResult);
static void fxTreeAssignmentExpression(txScriptParser* theParser, txSlot* theResult);
static void fxTreeConditionalExpression(txScriptParser* theParser, txSlot* theResult);
static void fxTreeOrExpression(txScriptParser* theParser, txSlot* theResult);
static void fxTreeAndExpression(txScriptParser* theParser, txSlot* theResult);
static void fxTreeBitOrExpression(txScriptParser* theParser, txSlot* theResult);
static void fxTreeBitXorExpression(txScriptParser* theParser, txSlot* theResult);
static void fxTreeBitAndExpression(txScriptParser* theParser, txSlot* theResult);
static void fxTreeEqualExpression(txScriptParser* theParser, txSlot* theResult);
static void fxTreeRelationalExpression(txScriptParser* theParser, txSlot* theResult);
static void fxTreeShiftExpression(txScriptParser* theParser, txSlot* theResult);
static void fxTreeAdditiveExpression(txScriptParser* theParser, txSlot* theResult);
static void fxTreeMultiplicativeExpression(txScriptParser* theParser, txSlot* theResult);
static void fxTreePrefixExpression(txScriptParser* theParser, txSlot* theResult);
static void fxTreePostfixExpression(txScriptParser* theParser, txSlot* theResult);
static void fxTreeNewExpression(txScriptParser* theParser, txSlot* theResult);
static void fxTreeCallExpression(txScriptParser* theParser, txSlot* theResult);
static void fxTreeLiteralExpression(txScriptParser* theParser, txSlot* theResult);
static void fxTreeFunctionExpression(txScriptParser* theParser, txSlot* theResult, txFlag declareIt);
static void fxTreeObjectExpression(txScriptParser* theParser, txSlot* theResult);
static void fxTreeArrayExpression(txScriptParser* theParser, txSlot* theResult);
static void fxTreeParameterBlock(txScriptParser* theParser, txSlot* theResult);
static void fxTreeArgumentBlock(txScriptParser* theParser, txSlot* theResult);
static void fxTreeAppendExpression(txScriptParser* theParser, txSlot* theResult);
static void fxTreeAppendStatement(txScriptParser* theParser, txSlot* theResult);
static void fxTreeAppendVariable(txScriptParser* theParser, txSlot* theResult);
static void fxTreeEscapeCString(txMachine* the);
static void fxTreeGetLine(txMachine* the);

enum {
	XSC_APPEND = XS_TOKEN_COUNT,
	XSC_COMPILE,
	XSC_CONTAINS,
	XSC_ARG,
	XSC_ARGS,
	XSC_ARRAY,
	XSC_BOOLEAN,
	XSC_CALL,
	XSC_EXPRESSIONS,
	XSC_FOR_IN,
	XSC_LABEL,
	XSC_MEMBER,
	XSC_MEMBER_AT,
	XSC_MINUS,
	XSC_OBJECT,
	XSC_PARAMS,
	XSC_PLUS,
	XSC_POST_DECREMENT,
	XSC_POST_INCREMENT,
	XSC_PROGRAM,
	XSC_PROPERTY,
	XSC_PROPERTY_AT,
	XSC_STATEMENT,
	XSC_STATEMENTS,
	XSC_UNDEFINED,
	XSC_VARS,
	XSC_TREE_COUNT
};

static txString gxTreeNames[XSC_TREE_COUNT] = {
	"",
	"XSCAdd",
	"XSCAddAssign",
	"XSCAnd",
	"XSCAssign",
	"XSCBitAnd",
	"XSCBitAndAssign",
	"XSCBitNot",
	"XSCBitOr",
	"XSCBitOrAssign",
	"XSCBitXor",
	"XSCBitXorAssign",
	"XSCBreak",
	"XSCCase",
	"XSCCatch",
	"",
	"",
	"",
	"",
	"XSCContinue",
	"XSCDebugger",
	"XSCPreDecrement",
	"XSCDefault",
	"XSCDelete",
	"XSCDivide",
	"XSCDivideAssign",
	"XSCDo",
	"",
	"",
	"",
	"",
	"XSCEqual",
	"",
	"",
	"",
	"",
	"XSCFor",
	"XSCFunction",
	"XSCIdentifier",
	"XSCIf",
	"",
	"",
	"XSCIn",
	"XSCPreIncrement",
	"XSCInstanceof",
	"XSCInteger",
	"",
	"",
	"",
	"",
	"XSCLeftShift",
	"XSCLeftShiftAssign",
	"XSCLess",
	"XSCLessEqual",
	"",
	"XSCModulo",
	"XSCModuloAssign",
	"XSCMore",
	"XSCMoreEqual",
	"XSCMultiply",
	"XSCMultiplyAssign",
	"XSCNew", 
	"XSCNot",
	"XSCNotEqual",
	"XSCNull", 
	"XSCOr",
	"",
	"",
	"",
	"",
	"XSCQuestionMark",
	"XSCNumber",
	"XSCRegexp",
	"XSCReturn",
	"",
	"",
	"",
	"",
	"",
	"XSCSignedRightShift",
	"XSCSignedRightShiftAssign",
	"",
	"XSCStrictEqual",
	"XSCStrictNotEqual",
	"XSCString",
	"XSCSubtract",
	"XSCSubtractAssign",
	"",
	"XSCSwitch",
	"XSCThis",
	"XSCThrow",
	"",
	"XSCTry",
	"XSCTypeof",
	"XSCUnsignedRightShift",
	"XSCUnsignedRightShiftAssign",
	"XSCVar",
	"XSCVoid",
	"XSCWhile",
	"XSCWith",
	"",
	"append",
	"compile",
	"contains",
	"XSCArg",
	"XSCArgs",
	"XSCArray",
	"XSCBoolean",
	"XSCCall",
	"XSCExpressions",
	"XSCForIn",
	"XSCLabel",
	"XSCMember",
	"XSCMemberAt",
	"XSCMinus",
	"XSCObject",
	"XSCParams",
	"XSCPlus",
	"XSCPostDecrement",
	"XSCPostIncrement",
	"XSCProgram",
	"XSCProperty",
	"XSCPropertyAt",
	"XSCStatement",
	"XSCStatements",
	"XSCUndefined",
	"XSCVars"
};
static txID gxTreeIDs[XSC_TREE_COUNT];

void fxBuildTree(txMachine* the, txString thePath)
{
	FILE* aFile = NULL;
	txByte* aCode;
	txInteger anIndex;
	txString aString;
	txSlot* aSymbol;

	aFile = fopen(thePath, "r");
	if (!aFile)
		return;
#ifdef mxDebug
	aCode = fxParseScript(the, aFile, (txGetter)fgetc, 
			fxNewFileC(the, thePath), 1, XS_PROGRAM_FLAG | XS_DEBUG_FLAG, C_NULL);
#else
	aCode = fxParseScript(the, aFile, (txGetter)fgetc, 
			C_NULL, 0, XS_PROGRAM_FLAG, C_NULL);
#endif
	fclose(aFile);
	/* ARGC */
	mxZeroSlot(--the->stack);
	the->stack->kind = XS_INTEGER_KIND;
	the->stack->value.integer = 0;
	/* THIS */
	*(--the->stack) = mxGlobal;
	/* FUNCTION */
	mxZeroSlot(--the->stack);
	the->stack->value.code = aCode;
	the->stack->kind = XS_CODE_KIND;
	fxNewProgramInstance(the);
	/* RESULT */
	mxZeroSlot(--the->stack);
	fxRunID(the, XS_NO_ID);
	the->stack++;
	
	for (anIndex = 0; anIndex < XSC_TREE_COUNT; anIndex++) {
		aString = gxTreeNames[anIndex];
		if (aString[0])
			aSymbol = fxFindSymbol(the, aString);
		else
			aSymbol = C_NULL;
		if (aSymbol)
			gxTreeIDs[anIndex] = aSymbol->ID;
	}
	
	mxPush(mxGlobal);
	fxNewHostFunction(the, fxTreeEscapeCString, 1);
	fxQueueID(the, fxID(the, "escapeCString"), XS_NO_FLAG);
	fxNewHostFunction(the, fxTreeGetLine, 0);
	fxQueueID(the, fxID(the, "getLine"), XS_NO_FLAG);
	the->stack++; /* mxGlobal */
}

txByte* fxParseTree(txMachine* the, void* theStream, txGetter theGetter, 
		txSlot* thePath, long theLine, txFlag theFlags, txSize* theSize)
{
	txScriptParser *aParser;
	txByte* result;

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

	mxPushInteger(0);
	
	mxPushNull();
	if (theFlags & (XS_EVAL_FLAG | XS_PROGRAM_FLAG | XS_THIS_FLAG)) {
		aParser->level = -1;
	}
	else {
		fxTreeArgumentBlock(aParser, the->stack);
		aParser->arguments = the->stack;
	}
	mxPushInteger(0);
	mxPush(mxGlobal);
	fxNewID(the, gxTreeIDs[XSC_VARS]);
	aParser->variables = the->stack;
	
	aParser->parameters = 0;
	mxPushNull();
	fxTreeBody(aParser, the->stack, C_NULL, theFlags);
	fxMatchToken(aParser, XS_TOKEN_EOF);
	mxPushBoolean(aParser->parameters);
	mxPushInteger(4);
	mxPush(mxGlobal);
	fxNewID(the, gxTreeIDs[XSC_PROGRAM]);
	
	if (aParser->errorCount > 0) {
		c_strcpy(aParser->buffer, "ECMAScript error(s)!");
#ifdef mxDebug
		fxDebugLoop(the, aParser->buffer);
#endif
		fxThrowMessage(the, XS_SYNTAX_ERROR, aParser->buffer);
	}
	fxCallID(the, gxTreeIDs[XSC_COMPILE]);
	result = (txByte *)the->stack->value.string;
	*theSize = c_strlen(result);
	the->stack++;
	return result;
}

void fxTreeBody(txScriptParser* theParser, txSlot* theResult, txSlot* theSymbol, txFlag theFlag)
{
	txMachine* the = theParser->the;
	
	while ((theParser->token == XS_TOKEN_STRING) 
			&& (theParser->crlf 
				|| (theParser->token2 == XS_TOKEN_SEMICOLON)
				|| (theParser->token2 == XS_TOKEN_EOF)
				|| (theParser->token2 == XS_TOKEN_RIGHT_BRACE))) {
		if (!theParser->escaped && (c_strcmp(theParser->string->value.string, "use strict") == 0))
			theParser->strict |= 1;
		fxGetNextToken(theParser);
		if (theParser->token == XS_TOKEN_SEMICOLON)
			fxGetNextToken(theParser);
	}
	
	theParser->level++;
	while ((theParser->token != XS_TOKEN_EOF) && (theParser->token != XS_TOKEN_RIGHT_BRACE)) {
		if (theParser->token == XS_TOKEN_FUNCTION) {
			fxMatchToken(theParser, XS_TOKEN_FUNCTION);
			mxPushNull();
			fxTreeFunctionExpression(theParser, the->stack, 1);
			fxTreeAppendStatement(theParser, theResult);
		}
		else
			fxTreeStatement(theParser, theResult);
	}
	theParser->level--;
}

void fxTreeStatementBlock(txScriptParser* theParser, txSlot* theResult)
{
	fxMatchToken(theParser, XS_TOKEN_LEFT_BRACE);
	while ((theParser->token != XS_TOKEN_EOF) && (theParser->token != XS_TOKEN_RIGHT_BRACE))
		fxTreeStatement(theParser, theResult);
	fxMatchToken(theParser, XS_TOKEN_RIGHT_BRACE);
}

void fxTreeStatement(txScriptParser* theParser, txSlot* theResult)
{
	txMachine* the = theParser->the;
	
	switch (theParser->token) {
	case XS_TOKEN_COMMA:
	case XS_TOKEN_SEMICOLON:
		fxGetNextToken(theParser);
		break;
	case XS_TOKEN_BREAK:
		fxTreeBreakStatement(theParser, theResult);
		fxSemicolon(theParser, theResult);
		break;
	case XS_TOKEN_CONTINUE:
		fxTreeContinueStatement(theParser, theResult);
		fxSemicolon(theParser, theResult);
		break;
	case XS_TOKEN_DEBUGGER:
		fxTreeDebuggerStatement(theParser, theResult);
		fxSemicolon(theParser, theResult);
		break;
	case XS_TOKEN_DO:
		fxTreeDoStatement(theParser, theResult);
		fxSemicolon(theParser, theResult);
		break;
	case XS_TOKEN_FOR:
		fxTreeForStatement(theParser, theResult);
		break;
	case XS_TOKEN_IF:
		fxTreeIfStatement(theParser, theResult);
		break;
	case XS_TOKEN_RETURN:
		fxTreeReturnStatement(theParser, theResult);
		fxSemicolon(theParser, theResult);
		break;
	case XS_TOKEN_LEFT_BRACE:
		fxTreeStatementBlock(theParser, theResult);
		break;
	case XS_TOKEN_SWITCH:
		fxTreeSwitchStatement(theParser, theResult);
		break;
	case XS_TOKEN_THROW:
		fxTreeThrowStatement(theParser, theResult);
		fxSemicolon(theParser, theResult);
		break;
	case XS_TOKEN_TRY:
		fxTreeTryStatement(theParser, theResult);
		break;
	case XS_TOKEN_VAR:
		fxTreeVarStatement(theParser, theResult);
		fxSemicolon(theParser, theResult);
		break;
	case XS_TOKEN_WHILE:
		fxTreeWhileStatement(theParser, theResult);
		break;
	case XS_TOKEN_WITH:
		if (theParser->strict)
			fxReportParserError(theParser, "no with in strict code");
		fxTreeWithStatement(theParser, theResult);
		break;
	case XS_TOKEN_IDENTIFIER:
		if (theParser->token2 == XS_TOKEN_COLON) {
			fxReportParserError(theParser, "labelled statement cannot be compiled into c");
			mxPushString(theParser->symbol->value.symbol.string);
			mxPushInteger(1);
			mxPush(mxGlobal);
			fxNewID(the, gxTreeIDs[XSC_LABEL]);
			fxTreeAppendStatement(theParser, theResult);
			fxGetNextToken(theParser);
			fxMatchToken(theParser, XS_TOKEN_COLON);
			fxTreeStatement(theParser, theResult);
			break;
		}
		/* continue */
	default:
		if (gxTokenFlags[theParser->token] & XS_TOKEN_BEGIN_EXPRESSION) {
			mxPushNull();
			fxTreeCommaExpression(theParser, the->stack);
			mxPushInteger(1);
			mxPush(mxGlobal);
			fxNewID(the, gxTreeIDs[XSC_STATEMENT]);
			fxTreeAppendStatement(theParser, theResult);
			fxSemicolon(theParser, theResult);
		}
		else {
			fxReportParserError(theParser, "invalid token %s", gxTokenNames[theParser->token]);
			fxGetNextToken(theParser);
		}
		break;
	}
}

void fxTreeBreakStatement(txScriptParser* theParser, txSlot* theResult)
{
	txMachine* the = theParser->the;
	
	fxMatchToken(theParser, XS_TOKEN_BREAK);
	if ((!(theParser->crlf)) && (theParser->token == XS_TOKEN_IDENTIFIER)) {
		fxReportParserError(theParser, "break label statement cannot be compiled into c");
		mxPushString(theParser->symbol->value.symbol.string);
		fxGetNextToken(theParser);
	}
	else {
		mxPushNull();
	}
	mxPushInteger(1);
	mxPush(mxGlobal);
	fxNewID(the, gxTreeIDs[XS_TOKEN_BREAK]);
	fxTreeAppendStatement(theParser, theResult);
}

void fxTreeContinueStatement(txScriptParser* theParser, txSlot* theResult)
{
	txMachine* the = theParser->the;
	
	fxMatchToken(theParser, XS_TOKEN_CONTINUE);
	if ((!(theParser->crlf)) && (theParser->token == XS_TOKEN_IDENTIFIER)) {
		fxReportParserError(theParser, "continue label statement cannot be compiled into c");
		mxPushString(theParser->symbol->value.symbol.string);
		fxGetNextToken(theParser);
	}
	else {
		mxPushNull();
	}
	mxPushInteger(1);
	mxPush(mxGlobal);
	fxNewID(the, gxTreeIDs[XS_TOKEN_CONTINUE]);
	fxTreeAppendStatement(theParser, theResult);
}

void fxTreeDebuggerStatement(txScriptParser* theParser, txSlot* theResult)
{
	txMachine* the = theParser->the;
	
	fxMatchToken(theParser, XS_TOKEN_DEBUGGER);
	mxPushInteger(0);
	mxPush(mxGlobal);
	fxNewID(the, gxTreeIDs[XS_TOKEN_DEBUGGER]);
	fxTreeAppendStatement(theParser, theResult);
}

void fxTreeDoStatement(txScriptParser* theParser, txSlot* theResult)
{
	txMachine* the = theParser->the;
	
	fxMatchToken(theParser, XS_TOKEN_DO);
	mxPushNull();
	fxTreeStatement(theParser, the->stack);
	fxMatchToken(theParser, XS_TOKEN_WHILE);
	fxMatchToken(theParser, XS_TOKEN_LEFT_PARENTHESIS);
	mxPushNull();
	fxTreeCommaExpression(theParser, the->stack);
	fxMatchToken(theParser, XS_TOKEN_RIGHT_PARENTHESIS);
	mxPushInteger(2);
	mxPush(mxGlobal);
	fxNewID(the, gxTreeIDs[XS_TOKEN_DO]);
	fxTreeAppendStatement(theParser, theResult);
}

void fxTreeForStatement(txScriptParser* theParser, txSlot* theResult)
{
	txMachine* the = theParser->the;
	txInteger aCount;
	
	fxMatchToken(theParser, XS_TOKEN_FOR);
	fxMatchToken(theParser, XS_TOKEN_LEFT_PARENTHESIS);
	mxPushNull();
	if (theParser->token == XS_TOKEN_VAR)
		aCount = fxTreeVarStatementNoIn(theParser, the->stack);
	else
		aCount = fxTreeCommaExpressionNoIn(theParser, the->stack);
	if (theParser->token == XS_TOKEN_IN) {
		if (aCount != 1)
			fxReportParserError(theParser, "list instead of reference");
		fxGetNextToken(theParser);
		mxPushNull();
		fxTreeCommaExpression(theParser, the->stack);
		fxMatchToken(theParser, XS_TOKEN_RIGHT_PARENTHESIS);
		mxPushNull();
		fxTreeStatement(theParser, the->stack);
		mxPushInteger(3);
		mxPush(mxGlobal);
		fxNewID(the, gxTreeIDs[XSC_FOR_IN]);
		fxTreeAppendStatement(theParser, theResult);
	}
	else {
		fxMatchToken(theParser, XS_TOKEN_SEMICOLON);
		mxPushNull();
		if (gxTokenFlags[theParser->token] & XS_TOKEN_BEGIN_EXPRESSION)
			fxTreeCommaExpressionNoIn(theParser, the->stack);
		fxMatchToken(theParser, XS_TOKEN_SEMICOLON);
		mxPushNull();
		if (gxTokenFlags[theParser->token] & XS_TOKEN_BEGIN_EXPRESSION)
			fxTreeCommaExpressionNoIn(theParser, the->stack);
		fxMatchToken(theParser, XS_TOKEN_RIGHT_PARENTHESIS);
		mxPushNull();
		fxTreeStatement(theParser, the->stack);
		mxPushInteger(4);
		mxPush(mxGlobal);
		fxNewID(the, gxTreeIDs[XS_TOKEN_FOR]);
		fxTreeAppendStatement(theParser, theResult);
	}
}

void fxTreeIfStatement(txScriptParser* theParser, txSlot* theResult)
{
	txMachine* the = theParser->the;

	fxMatchToken(theParser, XS_TOKEN_IF);
	fxMatchToken(theParser, XS_TOKEN_LEFT_PARENTHESIS);
	mxPushNull();
	fxTreeCommaExpression(theParser, the->stack);
	fxMatchToken(theParser, XS_TOKEN_RIGHT_PARENTHESIS);
	mxPushNull();
	fxTreeStatement(theParser, the->stack);
	mxPushNull();
	if (theParser->token == XS_TOKEN_ELSE) {
		fxMatchToken(theParser, XS_TOKEN_ELSE);
		fxTreeStatement(theParser, the->stack);
	}
	mxPushInteger(3);
	mxPush(mxGlobal);
	fxNewID(the, gxTreeIDs[XS_TOKEN_IF]);
	fxTreeAppendStatement(theParser, theResult);
}

void fxTreeReturnStatement(txScriptParser* theParser, txSlot* theResult)
{
	txMachine* the = theParser->the;

	fxMatchToken(theParser, XS_TOKEN_RETURN);
	if (!theParser->arguments)
		fxReportParserError(theParser, "invalid return");
	if ((theParser->crlf) || (gxTokenFlags[theParser->token] & XS_TOKEN_END_STATEMENT)) {
		mxPushInteger(0);
		mxPush(mxGlobal);
		fxNewID(the, gxTreeIDs[XSC_UNDEFINED]);
	}
	else {
		mxPushNull();
		fxTreeCommaExpression(theParser, the->stack);
	}
	mxPushInteger(1);
	mxPush(mxGlobal);
	fxNewID(the, gxTreeIDs[XS_TOKEN_RETURN]);
	fxTreeAppendStatement(theParser, theResult);
}

void fxTreeSwitchStatement(txScriptParser* theParser, txSlot* theResult)
{
	txMachine* the = theParser->the;
	txSlot* aSwitch;
	txSlot* aDefault = C_NULL;
	
	fxMatchToken(theParser, XS_TOKEN_SWITCH);
	fxMatchToken(theParser, XS_TOKEN_LEFT_PARENTHESIS);
	mxPushNull();
	fxTreeCommaExpression(theParser, the->stack);
	mxPushInteger(1);
	mxPush(mxGlobal);
	fxNewID(the, gxTreeIDs[XS_TOKEN_SWITCH]);
	aSwitch = the->stack;
	fxMatchToken(theParser, XS_TOKEN_RIGHT_PARENTHESIS);
	fxMatchToken(theParser, XS_TOKEN_LEFT_BRACE);
	while ((theParser->token == XS_TOKEN_CASE) || (theParser->token == XS_TOKEN_DEFAULT)) {
		if (theParser->token == XS_TOKEN_CASE) {
			fxGetNextToken(theParser);
			mxPushNull();
			fxTreeCommaExpression(theParser, the->stack);
			fxMatchToken(theParser, XS_TOKEN_COLON);
			mxPushNull();
			while (gxTokenFlags[theParser->token] & XS_TOKEN_BEGIN_STATEMENT)
				fxTreeStatement(theParser, the->stack);
			mxPushInteger(2);
			mxPush(mxGlobal);
			fxNewID(the, gxTreeIDs[XS_TOKEN_CASE]);
		}
		else {
			fxGetNextToken(theParser);
			if (aDefault) 
				fxReportParserError(theParser, "invalid default");
			fxMatchToken(theParser, XS_TOKEN_COLON);
			mxPushNull();
			mxPushNull();
			while (gxTokenFlags[theParser->token] & XS_TOKEN_BEGIN_STATEMENT)
				fxTreeStatement(theParser, the->stack);
			mxPushInteger(2);
			mxPush(mxGlobal);
			fxNewID(the, gxTreeIDs[XS_TOKEN_CASE]);
			aDefault = the->stack;
		}
		mxPushInteger(1);
		mxPush(*aSwitch);
		fxCallID(the, gxTreeIDs[XSC_APPEND]);
		the->stack++;
	}
	fxMatchToken(theParser, XS_TOKEN_RIGHT_BRACE);
	fxTreeAppendStatement(theParser, theResult);
}

void fxTreeThrowStatement(txScriptParser* theParser, txSlot* theResult)
{
	txMachine* the = theParser->the;

	fxMatchToken(theParser, XS_TOKEN_THROW);
	if ((theParser->crlf) || (gxTokenFlags[theParser->token] & XS_TOKEN_END_STATEMENT)) {
		mxPushInteger(0);
		mxPush(mxGlobal);
		fxNewID(the, gxTreeIDs[XSC_UNDEFINED]);
	}
	else {
		mxPushNull();
		fxTreeCommaExpression(theParser, the->stack);
	}
	mxPushInteger(1);
	mxPush(mxGlobal);
	fxNewID(the, gxTreeIDs[XS_TOKEN_THROW]);
	fxTreeAppendStatement(theParser, theResult);
}

void fxTreeTryStatement(txScriptParser* theParser, txSlot* theResult)
{
	txMachine* the = theParser->the;
	txSlot* aSymbol;
	
	fxMatchToken(theParser, XS_TOKEN_TRY);
	mxPushNull();
	fxTreeStatementBlock(theParser, the->stack);
	if (theParser->token == XS_TOKEN_CATCH) {
		fxGetNextToken(theParser);
		fxMatchToken(theParser, XS_TOKEN_LEFT_PARENTHESIS);
		if (theParser->token == XS_TOKEN_IDENTIFIER) {
			aSymbol = theParser->symbol;
			if (theParser->strict) {
				if (aSymbol->ID == theParser->the->argumentsID)
					fxReportParserError(theParser, "catch arguments (strict mode)");
				if (aSymbol->ID == theParser->the->evalID)
					fxReportParserError(theParser, "catch eval (strict mode)");
			}
			mxPushString(aSymbol->value.symbol.string);
			mxPushInteger(1);
			mxPush(mxGlobal);
			fxNewID(the, gxTreeIDs[XSC_ARG]);
			fxGetNextToken(theParser);
		}
		else {
			fxReportParserError(theParser, "missing identifier");
			mxPushNull();
		}
		fxMatchToken(theParser, XS_TOKEN_RIGHT_PARENTHESIS);
		mxPushNull();
		fxTreeStatementBlock(theParser, the->stack);
	}
	else {
		mxPushNull();
		mxPushNull();
	}
	mxPushNull();
	if (theParser->token == XS_TOKEN_FINALLY) {
		fxGetNextToken(theParser);
		fxTreeStatementBlock(theParser, the->stack);
	}
	mxPushInteger(4);
	mxPush(mxGlobal);
	fxNewID(the, gxTreeIDs[XS_TOKEN_TRY]);
	fxTreeAppendStatement(theParser, theResult);
}

void fxTreeVarStatement(txScriptParser* theParser, txSlot* theResult)
{
	txMachine* the = theParser->the;
	txSlot* aSymbol;
	txTokenFlag aFlag = gxTokenFlags[XS_TOKEN_IN];
	gxTokenFlags[XS_TOKEN_IN] = XS_TOKEN_RELATIONAL_EXPRESSION;
	fxMatchToken(theParser, XS_TOKEN_VAR);
	while (theParser->token == XS_TOKEN_IDENTIFIER) {
		aSymbol = theParser->symbol;
		mxPushString(aSymbol->value.symbol.string);
		mxPushInteger(1);
		mxPush(mxGlobal);
		fxNewID(the, gxTreeIDs[XS_TOKEN_VAR]);
		fxTreeAppendVariable(theParser, theResult);
		fxGetNextToken(theParser);
		if (theParser->token == XS_TOKEN_ASSIGN) {
			fxGetNextToken(theParser);
			mxPushString(aSymbol->value.symbol.string);
			mxPushInteger(1);
			mxPush(mxGlobal);
			fxNewID(the, gxTreeIDs[XS_TOKEN_IDENTIFIER]);
			mxPushNull();
			fxTreeAssignmentExpression(theParser, the->stack);
			mxPushInteger(2);
			mxPush(mxGlobal);
			fxNewID(the, gxTreeIDs[XS_TOKEN_ASSIGN]);
			mxPushInteger(1);
			mxPush(mxGlobal);
			fxNewID(the, gxTreeIDs[XSC_STATEMENT]);
			fxTreeAppendStatement(theParser, theResult);
		}
		if (theParser->token != XS_TOKEN_COMMA)
			break;
		fxGetNextToken(theParser);
	}
	gxTokenFlags[XS_TOKEN_IN] = aFlag;
}

txInteger fxTreeVarStatementNoIn(txScriptParser* theParser, txSlot* theResult)
{
	txMachine* the = theParser->the;
	txSlot* aSymbol;
	txInteger aCount = 0;
	txTokenFlag aFlag = gxTokenFlags[XS_TOKEN_IN];
	gxTokenFlags[XS_TOKEN_IN] = 0;
	fxMatchToken(theParser, XS_TOKEN_VAR);
	while (theParser->token == XS_TOKEN_IDENTIFIER) {
		aSymbol = theParser->symbol;
		mxPushString(aSymbol->value.symbol.string);
		mxPushInteger(1);
		mxPush(mxGlobal);
		fxNewID(the, gxTreeIDs[XS_TOKEN_VAR]);
		fxTreeAppendVariable(theParser, theResult);
		fxGetNextToken(theParser);
		if (theParser->token == XS_TOKEN_ASSIGN) {
			fxGetNextToken(theParser);
			mxPushString(aSymbol->value.symbol.string);
			mxPushInteger(1);
			mxPush(mxGlobal);
			fxNewID(the, gxTreeIDs[XS_TOKEN_IDENTIFIER]);
			mxPushNull();
			fxTreeAssignmentExpression(theParser, the->stack);
			mxPushInteger(2);
			mxPush(mxGlobal);
			fxNewID(the, gxTreeIDs[XS_TOKEN_ASSIGN]);
			fxTreeAppendExpression(theParser, theResult);
		}
		else {
			mxPushString(aSymbol->value.symbol.string);
			mxPushInteger(1);
			mxPush(mxGlobal);
			fxNewID(the, gxTreeIDs[XS_TOKEN_IDENTIFIER]);
			fxTreeAppendExpression(theParser, theResult);
		}
		aCount++;
		if (theParser->token != XS_TOKEN_COMMA)
			break;
		fxGetNextToken(theParser);
	}
	gxTokenFlags[XS_TOKEN_IN] = aFlag;
	return aCount;
}

void fxTreeWhileStatement(txScriptParser* theParser, txSlot* theResult)
{
	txMachine* the = theParser->the;
	
	fxMatchToken(theParser, XS_TOKEN_WHILE);
	fxMatchToken(theParser, XS_TOKEN_LEFT_PARENTHESIS);
	mxPushNull();
	fxTreeCommaExpression(theParser, the->stack);
	fxMatchToken(theParser, XS_TOKEN_RIGHT_PARENTHESIS);
	mxPushNull();
	fxTreeStatement(theParser, the->stack);
	mxPushInteger(2);
	mxPush(mxGlobal);
	fxNewID(the, gxTreeIDs[XS_TOKEN_WHILE]);
	fxTreeAppendStatement(theParser, theResult);
}

void fxTreeWithStatement(txScriptParser* theParser, txSlot* theResult)
{
	txMachine* the = theParser->the;

	fxReportParserError(theParser, "with statement cannot be compiled into c");
	fxMatchToken(theParser, XS_TOKEN_WITH);
	fxMatchToken(theParser, XS_TOKEN_LEFT_PARENTHESIS);
	mxPushNull();
	fxTreeCommaExpression(theParser, the->stack);
	fxMatchToken(theParser, XS_TOKEN_RIGHT_PARENTHESIS);
	mxPushNull();
	fxTreeStatement(theParser, the->stack);
	mxPushInteger(2);
	mxPush(mxGlobal);
	fxNewID(the, gxTreeIDs[XS_TOKEN_WITH]);
	fxTreeAppendStatement(theParser, theResult);
}

void fxTreeCommaExpression(txScriptParser* theParser, txSlot* theResult)
{
	txMachine* the = theParser->the;
	txTokenFlag aFlag = gxTokenFlags[XS_TOKEN_IN];
	gxTokenFlags[XS_TOKEN_IN] = XS_TOKEN_RELATIONAL_EXPRESSION;
	while (gxTokenFlags[theParser->token] & XS_TOKEN_BEGIN_EXPRESSION) {
		mxPushNull();
		fxTreeAssignmentExpression(theParser, the->stack);
		fxTreeAppendExpression(theParser, theResult);
		if (theParser->token != XS_TOKEN_COMMA) 
			break;
		fxGetNextToken(theParser);
	}
	gxTokenFlags[XS_TOKEN_IN] = aFlag;
}

txInteger fxTreeCommaExpressionNoIn(txScriptParser* theParser, txSlot* theResult)
{
	txMachine* the = theParser->the;
	txInteger aCount = 0;
	txTokenFlag aFlag = gxTokenFlags[XS_TOKEN_IN];
	gxTokenFlags[XS_TOKEN_IN] = 0;
	while (gxTokenFlags[theParser->token] & XS_TOKEN_BEGIN_EXPRESSION) {
		mxPushNull();
		fxTreeAssignmentExpression(theParser, the->stack);
		fxTreeAppendExpression(theParser, theResult);
		aCount++;
		if (theParser->token != XS_TOKEN_COMMA) 
			break;
		fxGetNextToken(theParser);
	}
	gxTokenFlags[XS_TOKEN_IN] = aFlag;
	return aCount;
}

void fxTreeAssignmentExpression(txScriptParser* theParser, txSlot* theResult)
{
	txMachine* the = theParser->the;
	txToken aToken;
	
	fxTreeConditionalExpression(theParser, theResult);
	while (gxTokenFlags[theParser->token] & XS_TOKEN_ASSIGN_EXPRESSION) {
		aToken = theParser->token;
		fxGetNextToken(theParser);
		mxPush(*theResult);
		mxPushNull();
		fxTreeAssignmentExpression(theParser, the->stack);
		mxPushInteger(2);
		mxPush(mxGlobal);
		fxNewID(the, gxTreeIDs[aToken]);
		*theResult = *(the->stack++);
	}
}

void fxTreeConditionalExpression(txScriptParser* theParser, txSlot* theResult)
{
	txMachine* the = theParser->the;

	fxTreeOrExpression(theParser, theResult);
	if (theParser->token == XS_TOKEN_QUESTION_MARK) {
		fxGetNextToken(theParser);
		mxPush(*theResult);
		mxPushNull();
		fxTreeAssignmentExpression(theParser, the->stack);
		fxMatchToken(theParser, XS_TOKEN_COLON);
		mxPushNull();
		fxTreeAssignmentExpression(theParser, the->stack);
		mxPushInteger(3);
		mxPush(mxGlobal);
		fxNewID(the, gxTreeIDs[XS_TOKEN_QUESTION_MARK]);
		*theResult = *(the->stack++);
	}
}

void fxTreeOrExpression(txScriptParser* theParser, txSlot* theResult)
{
	txMachine* the = theParser->the;

	fxTreeAndExpression(theParser, theResult);
	while (theParser->token == XS_TOKEN_OR) {
		fxGetNextToken(theParser);
		mxPush(*theResult);
		mxPushNull();
		fxTreeAndExpression(theParser, the->stack);
		mxPushInteger(2);
		mxPush(mxGlobal);
		fxNewID(the, gxTreeIDs[XS_TOKEN_OR]);
		*theResult = *(the->stack++);
	}
}

void fxTreeAndExpression(txScriptParser* theParser, txSlot* theResult)
{
	txMachine* the = theParser->the;

	fxTreeBitOrExpression(theParser, theResult);
	while (theParser->token == XS_TOKEN_AND) {
		fxGetNextToken(theParser);
		mxPush(*theResult);
		mxPushNull();
		fxTreeBitOrExpression(theParser, the->stack);
		mxPushInteger(2);
		mxPush(mxGlobal);
		fxNewID(the, gxTreeIDs[XS_TOKEN_AND]);
		*theResult = *(the->stack++);
	}
}

void fxTreeBitOrExpression(txScriptParser* theParser, txSlot* theResult)
{
	txMachine* the = theParser->the;

	fxTreeBitXorExpression(theParser, theResult);
	while (theParser->token == XS_TOKEN_BIT_OR) {
		fxGetNextToken(theParser);
		mxPush(*theResult);
		mxPushNull();
		fxTreeBitXorExpression(theParser, the->stack);
		mxPushInteger(2);
		mxPush(mxGlobal);
		fxNewID(the, gxTreeIDs[XS_TOKEN_BIT_OR]);
		*theResult = *(the->stack++);
	}
}

void fxTreeBitXorExpression(txScriptParser* theParser, txSlot* theResult)
{
	txMachine* the = theParser->the;

	fxTreeBitAndExpression(theParser, theResult);
	while (theParser->token == XS_TOKEN_BIT_XOR) {
		fxGetNextToken(theParser);
		mxPush(*theResult);
		mxPushNull();
		fxTreeBitAndExpression(theParser, the->stack);
		mxPushInteger(2);
		mxPush(mxGlobal);
		fxNewID(the, gxTreeIDs[XS_TOKEN_BIT_XOR]);
		*theResult = *(the->stack++);
	}
}

void fxTreeBitAndExpression(txScriptParser* theParser, txSlot* theResult)
{
	txMachine* the = theParser->the;

	fxTreeEqualExpression(theParser, theResult);
	while (theParser->token == XS_TOKEN_BIT_AND) {
		fxGetNextToken(theParser);
		mxPush(*theResult);
		mxPushNull();
		fxTreeEqualExpression(theParser, the->stack);
		mxPushInteger(2);
		mxPush(mxGlobal);
		fxNewID(the, gxTreeIDs[XS_TOKEN_BIT_AND]);
		*theResult = *(the->stack++);
	}
}

void fxTreeEqualExpression(txScriptParser* theParser, txSlot* theResult)
{
	txMachine* the = theParser->the;
	txToken aToken;
	
	fxTreeRelationalExpression(theParser, theResult);
	while (gxTokenFlags[theParser->token] & XS_TOKEN_EQUAL_EXPRESSION) {
		aToken = theParser->token;
		fxGetNextToken(theParser);
		mxPush(*theResult);
		mxPushNull();
		fxTreeRelationalExpression(theParser, the->stack);
		mxPushInteger(2);
		mxPush(mxGlobal);
		fxNewID(the, gxTreeIDs[aToken]);
		*theResult = *(the->stack++);
	}
}

void fxTreeRelationalExpression(txScriptParser* theParser, txSlot* theResult)
{
	txMachine* the = theParser->the;
	txToken aToken;
	
	fxTreeShiftExpression(theParser, theResult);
	while (gxTokenFlags[theParser->token] & XS_TOKEN_RELATIONAL_EXPRESSION) {
		aToken = theParser->token;
		fxGetNextToken(theParser);
		mxPush(*theResult);
		mxPushNull();
		fxTreeShiftExpression(theParser, the->stack);
		mxPushInteger(2);
		mxPush(mxGlobal);
		fxNewID(the, gxTreeIDs[aToken]);
		*theResult = *(the->stack++);
	}
}

void fxTreeShiftExpression(txScriptParser* theParser, txSlot* theResult)
{
	txMachine* the = theParser->the;
	txToken aToken;
	
	fxTreeAdditiveExpression(theParser, theResult);
	while (gxTokenFlags[theParser->token] & XS_TOKEN_SHIFT_EXPRESSION) {
		aToken = theParser->token;
		fxGetNextToken(theParser);
		mxPush(*theResult);
		mxPushNull();
		fxTreeAdditiveExpression(theParser, the->stack);
		mxPushInteger(2);
		mxPush(mxGlobal);
		fxNewID(the, gxTreeIDs[aToken]);
		*theResult = *(the->stack++);
	}
}

void fxTreeAdditiveExpression(txScriptParser* theParser, txSlot* theResult)
{
	txMachine* the = theParser->the;
	txToken aToken;
	
	fxTreeMultiplicativeExpression(theParser, theResult);
	while (gxTokenFlags[theParser->token] & XS_TOKEN_ADDITIVE_EXPRESSION) {
		aToken = theParser->token;
		fxGetNextToken(theParser);
		mxPush(*theResult);
		mxPushNull();
		fxTreeMultiplicativeExpression(theParser, the->stack);
		mxPushInteger(2);
		mxPush(mxGlobal);
		fxNewID(the, gxTreeIDs[aToken]);
		*theResult = *(the->stack++);
	}
}

void fxTreeMultiplicativeExpression(txScriptParser* theParser, txSlot* theResult)
{
	txMachine* the = theParser->the;
	txToken aToken;
	
	fxTreePrefixExpression(theParser, theResult);
	while (gxTokenFlags[theParser->token] & XS_TOKEN_MULTIPLICATIVE_EXPRESSION) {
		aToken = theParser->token;
		fxGetNextToken(theParser);
		mxPush(*theResult);
		mxPushNull();
		fxTreePrefixExpression(theParser, the->stack);
		mxPushInteger(2);
		mxPush(mxGlobal);
		fxNewID(the, gxTreeIDs[aToken]);
		*theResult = *(the->stack++);
	}
}

void fxTreePrefixExpression(txScriptParser* theParser, txSlot* theResult)
{
	txMachine* the = theParser->the;
	txToken aToken;
	
	if (gxTokenFlags[theParser->token] & XS_TOKEN_PREFIX_EXPRESSION) {
		aToken = theParser->token;
		fxGetNextToken(theParser);
		mxPushNull();
		fxTreePrefixExpression(theParser, the->stack);
		mxPushInteger(1);
		mxPush(mxGlobal);
		if (aToken == XS_TOKEN_ADD)
			fxNewID(the, gxTreeIDs[XSC_PLUS]);
		else if (aToken == XS_TOKEN_SUBTRACT)
			fxNewID(the, gxTreeIDs[XSC_MINUS]);
		else
			fxNewID(the, gxTreeIDs[aToken]);
		*theResult = *(the->stack++);
	}
	else
		fxTreePostfixExpression(theParser, theResult);
}

void fxTreePostfixExpression(txScriptParser* theParser, txSlot* theResult)
{
	txMachine* the = theParser->the;
	txToken aToken;
	
	fxTreeCallExpression(theParser, theResult);
	if ((!(theParser->crlf)) && (gxTokenFlags[theParser->token] & XS_TOKEN_POSTFIX_EXPRESSION)) {
		aToken = theParser->token;
		fxGetNextToken(theParser);
		mxPush(*theResult);
		mxPushInteger(1);
		mxPush(mxGlobal);
		if (aToken == XS_TOKEN_INCREMENT)
			fxNewID(the, gxTreeIDs[XSC_POST_INCREMENT]);
		else
			fxNewID(the, gxTreeIDs[XSC_POST_DECREMENT]);
		*theResult = *(the->stack++);
	}
}

void fxTreeCallExpression(txScriptParser* theParser, txSlot* theResult)
{
	txMachine* the = theParser->the;

	fxTreeLiteralExpression(theParser, theResult);
	for (;;) {
		if (theParser->token == XS_TOKEN_DOT) {
			fxGetNextToken(theParser);
			if (theParser->token == XS_TOKEN_IDENTIFIER) {
				theParser->symbol->flag |= XS_TO_C_FLAG;
				mxPush(*theResult);
				mxPushString(theParser->symbol->value.symbol.string);
				mxPushInteger(2);
				mxPush(mxGlobal);
				fxNewID(the, gxTreeIDs[XSC_MEMBER]);
				*theResult = *(the->stack++);
				fxGetNextToken(theParser);
			}
			else
				fxReportParserError(theParser, "missing property");
		}
		else if (theParser->token == XS_TOKEN_LEFT_BRACKET) {
			fxGetNextToken(theParser);
			mxPush(*theResult);
			mxPushNull();
			fxTreeCommaExpression(theParser, the->stack);
			mxPushInteger(2);
			mxPush(mxGlobal);
			fxNewID(the, gxTreeIDs[XSC_MEMBER_AT]);
			*theResult = *(the->stack++);
			fxMatchToken(theParser, XS_TOKEN_RIGHT_BRACKET);
		}
		else if (theParser->token == XS_TOKEN_LEFT_PARENTHESIS) {
			mxPush(*theResult);
			mxPushNull();
			fxTreeParameterBlock(theParser, the->stack);
			mxPushInteger(2);
			mxPush(mxGlobal);
			fxNewID(the, gxTreeIDs[XSC_CALL]);
			*theResult = *(the->stack++);
		}
		else
			break;
	} 
}

void fxTreeLiteralExpression(txScriptParser* theParser, txSlot* theResult)
{
	txMachine* the = theParser->the;
	txSlot* aSymbol;
	
	switch (theParser->token) {
	case XS_TOKEN_THIS:
	case XS_TOKEN_NULL:
		mxPushInteger(0);
		mxPush(mxGlobal);
		fxNewID(the, gxTreeIDs[theParser->token]);
		*theResult = *(the->stack++);
		fxGetNextToken(theParser);
		break;
	case XS_TOKEN_TRUE:
		mxPushBoolean(1);
		mxPushInteger(1);
		mxPush(mxGlobal);
		fxNewID(the, gxTreeIDs[XSC_BOOLEAN]);
		*theResult = *(the->stack++);
		fxGetNextToken(theParser);
		break;
	case XS_TOKEN_FALSE:
		mxPushBoolean(0);
		mxPushInteger(1);
		mxPush(mxGlobal);
		fxNewID(the, gxTreeIDs[XSC_BOOLEAN]);
		*theResult = *(the->stack++);
		fxGetNextToken(theParser);
		break;
	case XS_TOKEN_INTEGER:
		mxPushInteger(theParser->integer);
		mxPushInteger(1);
		mxPush(mxGlobal);
		fxNewID(the, gxTreeIDs[theParser->token]);
		*theResult = *(the->stack++);
		fxGetNextToken(theParser);
		break;
	case XS_TOKEN_NUMBER:
		mxPushNumber(theParser->number);
		mxPushInteger(1);
		mxPush(mxGlobal);
		fxNewID(the, gxTreeIDs[theParser->token]);
		*theResult = *(the->stack++);
		fxGetNextToken(theParser);
		break;
	case XS_TOKEN_REGEXP:
		aSymbol = fxNewSymbolC(theParser->the, "RegExp");
		aSymbol->flag |= XS_TO_C_FLAG;
		mxPushString(theParser->string->value.string);
		mxPushString(theParser->flags->value.string);
		mxPushInteger(2);
		mxPush(mxGlobal);
		fxNewID(the, gxTreeIDs[theParser->token]);
		*theResult = *(the->stack++);
		fxGetNextToken(theParser);
		break;
	case XS_TOKEN_STRING:
		mxPushString(theParser->string->value.string);
		mxPushInteger(1);
		mxPush(mxGlobal);
		fxNewID(the, gxTreeIDs[theParser->token]);
		*theResult = *(the->stack++);
		fxGetNextToken(theParser);
		break;
	case XS_TOKEN_IDENTIFIER:
		aSymbol = theParser->symbol;
		if (c_strcmp(aSymbol->value.symbol.string, "undefined") == 0) {
			mxPushInteger(0);
			mxPush(mxGlobal);
			fxNewID(the, gxTreeIDs[XSC_UNDEFINED]);
		}
		else {
			if (c_strcmp(aSymbol->value.symbol.string, "arguments") == 0)
				theParser->parameters = 1;
			aSymbol->flag |= XS_TO_C_FLAG;
			mxPushString(aSymbol->value.symbol.string);
			mxPushInteger(1);
			mxPush(mxGlobal);
			fxNewID(the, gxTreeIDs[XS_TOKEN_IDENTIFIER]);
		}
		*theResult = *(the->stack++);
		fxGetNextToken(theParser);
		break;
	case XS_TOKEN_FUNCTION:
		fxMatchToken(theParser, XS_TOKEN_FUNCTION);
		fxTreeFunctionExpression(theParser, theResult, 0);
		break;
	case XS_TOKEN_NEW:
		fxMatchToken(theParser, XS_TOKEN_NEW);
		fxTreeNewExpression(theParser, theResult);
		break;
	case XS_TOKEN_LEFT_BRACE:
		fxTreeObjectExpression(theParser, theResult);
		break;
	case XS_TOKEN_LEFT_BRACKET:
		fxTreeArrayExpression(theParser, theResult);
		break;
	case XS_TOKEN_LEFT_PARENTHESIS:
		fxMatchToken(theParser, XS_TOKEN_LEFT_PARENTHESIS);
		fxTreeCommaExpression(theParser, theResult);
		fxMatchToken(theParser, XS_TOKEN_RIGHT_PARENTHESIS);
		break;
	default:
		fxReportParserError(theParser, "missing expression");
		break;
	}
}

void fxTreeFunctionExpression(txScriptParser* theParser, txSlot* theResult, txFlag declareIt)
{
	txMachine* the = theParser->the;
	txSlot* aSymbol = C_NULL;
	txSlot* arguments = theParser->arguments;
	txSlot* variables = theParser->variables;
	txBoolean aParametersFlag = theParser->parameters;
	txBoolean aStrictFlag = theParser->strict;
	
	//fxReportParserError(theParser, "function expression cannot be compiled into c");
	if (theParser->token == XS_TOKEN_IDENTIFIER) {
		aSymbol = theParser->symbol;
		mxPushString(aSymbol->value.symbol.string);
		fxGetNextToken(theParser);
	}
	else {
		if (declareIt)
			fxReportParserError(theParser, "missing identifier");
		mxPushNull();
	}
	mxPushNull();
	fxTreeArgumentBlock(theParser, the->stack);
	theParser->arguments = the->stack;
	mxPushInteger(0);
	mxPush(mxGlobal);
	fxNewID(the, gxTreeIDs[XSC_VARS]);
	theParser->variables = the->stack;
	theParser->parameters = 0;
	fxMatchToken(theParser, XS_TOKEN_LEFT_BRACE);
	mxPushNull();
	fxTreeBody(theParser, the->stack, aSymbol, XS_NO_FLAG);
	fxMatchToken(theParser, XS_TOKEN_RIGHT_BRACE);
	mxPushInteger(4);
	mxPush(mxGlobal);
	fxNewID(the, gxTreeIDs[XS_TOKEN_FUNCTION]);
	*theResult = *(the->stack++);
	
	theParser->parameters = aParametersFlag;
	theParser->strict = aStrictFlag;
	theParser->variables = variables;
	theParser->arguments = arguments;
}

void fxTreeNewExpression(txScriptParser* theParser, txSlot* theResult)
{
	txMachine* the = theParser->the;

	fxTreeLiteralExpression(theParser, theResult);
	for (;;) {
		if (theParser->token == XS_TOKEN_DOT) {
			fxGetNextToken(theParser);
			if (theParser->token == XS_TOKEN_IDENTIFIER) {
				theParser->symbol->flag |= XS_TO_C_FLAG;
				mxPush(*theResult);
				mxPushString(theParser->symbol->value.symbol.string);
				mxPushInteger(2);
				mxPush(mxGlobal);
				fxNewID(the, gxTreeIDs[XSC_MEMBER]);
				*theResult = *(the->stack++);
				fxGetNextToken(theParser);
			}
			else
				fxReportParserError(theParser, "missing property");
		}
		else if (theParser->token == XS_TOKEN_LEFT_BRACKET) {
			fxGetNextToken(theParser);
			mxPush(*theResult);
			mxPushNull();
			fxTreeCommaExpression(theParser, the->stack);
			mxPushInteger(2);
			mxPush(mxGlobal);
			fxNewID(the, gxTreeIDs[XSC_MEMBER_AT]);
			*theResult = *(the->stack++);
			fxMatchToken(theParser, XS_TOKEN_RIGHT_BRACKET);
		}
		else
			break;
	} 
	mxPush(*theResult);
	mxPushNull();
	if (theParser->token == XS_TOKEN_LEFT_PARENTHESIS)
		fxTreeParameterBlock(theParser, the->stack);
	mxPushInteger(2);
	mxPush(mxGlobal);
	fxNewID(the, gxTreeIDs[XS_TOKEN_NEW]);
	*theResult = *(the->stack++);
}

void fxTreeObjectExpression(txScriptParser* theParser, txSlot* theResult)
{
	txMachine* the = theParser->the;
	txSlot* aStack = the->stack;
	txSlot* aUniqueList = fxNewList(theParser);
	txSlot* aUniqueSlot;
	txSlot* aSymbol;
	txString aString;
	txFlag aFlag;
	char aBuffer[256];
	
	mxPushInteger(0);
	mxPush(mxGlobal);
	fxNewID(the, gxTreeIDs[XSC_OBJECT]);
	*theResult = *(the->stack++);
	fxMatchToken(theParser, XS_TOKEN_LEFT_BRACE);
	for (;;) {
		aSymbol = C_NULL;
		aString = C_NULL;
		aFlag = XS_NO_FLAG;
		if (theParser->token == XS_TOKEN_INTEGER) {
			mxPushInteger(theParser->integer);
			mxPushInteger(1);
			mxPush(mxGlobal);
			fxNewID(the, gxTreeIDs[theParser->token]);
			fxCopyStringC(theParser->the, theParser->string, fxIntegerToString(theParser->integer, aBuffer, sizeof(aBuffer)));
			aString = theParser->string->value.string;
		}
		else if (theParser->token == XS_TOKEN_NUMBER)  {
			mxPushNumber(theParser->number);
			mxPushInteger(1);
			mxPush(mxGlobal);
			fxNewID(the, gxTreeIDs[theParser->token]);
			fxCopyStringC(theParser->the, theParser->string, fxNumberToString(theParser->the, theParser->number, aBuffer, sizeof(aBuffer), 0, 0));
			aString = theParser->string->value.string;
		}
		else if (theParser->token == XS_TOKEN_STRING) {
			mxPushString(theParser->string->value.string);
			mxPushInteger(1);
			mxPush(mxGlobal);
			fxNewID(the, gxTreeIDs[theParser->token]);
			aString = theParser->string->value.string;
		}
		else if (theParser->token == XS_TOKEN_IDENTIFIER) {
			aSymbol = theParser->symbol;
			if (theParser->token2 == XS_TOKEN_COLON) {
				aSymbol->flag |= XS_TO_C_FLAG;
				mxPushString(aSymbol->value.symbol.string);
				aString = aSymbol->value.symbol.string;
			}
			else if (c_strcmp(aSymbol->value.symbol.string, "get") == 0)
				aFlag = XS_GETTER_FLAG;
			else if (c_strcmp(aSymbol->value.symbol.string, "set") == 0)
				aFlag = XS_SETTER_FLAG;
			else 
				fxReportParserError(theParser, "missing %s", gxTokenNames[XS_TOKEN_COLON]);
			if (aFlag) {
				fxGetNextToken(theParser);
				if (theParser->token2 != XS_TOKEN_LEFT_PARENTHESIS)
					fxReportParserError(theParser, "missing %s", gxTokenNames[XS_TOKEN_LEFT_PARENTHESIS]);
				if (theParser->token == XS_TOKEN_INTEGER) {
					mxPushInteger(theParser->integer);
					mxPushInteger(1);
					mxPush(mxGlobal);
					fxNewID(the, gxTreeIDs[theParser->token]);
					fxCopyStringC(theParser->the, theParser->string, fxIntegerToString(theParser->integer, aBuffer, sizeof(aBuffer)));
					aString = theParser->string->value.string;
				}
				else if (theParser->token == XS_TOKEN_NUMBER)  {
					mxPushNumber(theParser->number);
					mxPushInteger(1);
					mxPush(mxGlobal);
					fxNewID(the, gxTreeIDs[theParser->token]);
					fxCopyStringC(theParser->the, theParser->string, fxNumberToString(theParser->the, theParser->number, aBuffer, sizeof(aBuffer), 0, 0));
					aString = theParser->string->value.string;
				}
				else if (theParser->token == XS_TOKEN_STRING) {
					mxPushString(theParser->string->value.string);
					mxPushInteger(1);
					mxPush(mxGlobal);
					fxNewID(the, gxTreeIDs[theParser->token]);
					aString = theParser->string->value.string;
				}
				else if (theParser->token == XS_TOKEN_IDENTIFIER) {
					aSymbol = theParser->symbol;
					aSymbol->flag |= XS_TO_C_FLAG;
					mxPushString(aSymbol->value.symbol.string);
					aString = aSymbol->value.symbol.string;
				}
			}
		}
		if (aString) {
			aUniqueSlot = fxSearchList(theParser, aString, aUniqueList);
			if (aUniqueSlot) {
				if (aFlag == XS_GETTER_FLAG) {
					if (aUniqueSlot->flag == XS_GETTER_FLAG)
						fxReportParserError(theParser, "getter already defined");
					else if (aUniqueSlot->flag != XS_SETTER_FLAG)
						fxReportParserError(theParser, "property already defined");
				}
				else if (aFlag == XS_SETTER_FLAG) {
					if (aUniqueSlot->flag == XS_SETTER_FLAG)
						fxReportParserError(theParser, "setter already defined");
					else if (aUniqueSlot->flag != XS_GETTER_FLAG)
						fxReportParserError(theParser, "property already defined");
				}
				else {
					if (aUniqueSlot->flag == XS_GETTER_FLAG)
						fxReportParserError(theParser, "getter already defined");
					else if (aUniqueSlot->flag == XS_SETTER_FLAG)
						fxReportParserError(theParser, "setter already defined");
					else if (theParser->strict)
						fxReportParserError(theParser, "property already defined (strict mode)");
				}
			}
			else {
				aUniqueSlot = fxAppendString(theParser, aUniqueList, aString);
				aUniqueSlot->flag = aFlag;
			}
			mxPushInteger(aFlag);
			fxGetNextToken(theParser);
			if (aFlag) {
				mxPushNull();
				fxTreeFunctionExpression(theParser, the->stack, 0);
			}
			else {
				fxMatchToken(theParser, XS_TOKEN_COLON);
				mxPushNull();
				fxTreeAssignmentExpression(theParser, the->stack);
			}
			mxPushInteger(3);
			mxPush(mxGlobal);
			if (aSymbol)
				fxNewID(the, gxTreeIDs[XSC_PROPERTY]);
			else
				fxNewID(the, gxTreeIDs[XSC_PROPERTY_AT]);
			
			mxPushInteger(1);
			mxPush(*theResult);
			fxCallID(the, gxTreeIDs[XSC_APPEND]);
			the->stack++;
		}
		if (theParser->token == XS_TOKEN_RIGHT_BRACE)
			break;
		fxMatchToken(theParser, XS_TOKEN_COMMA);
	}
	fxMatchToken(theParser, XS_TOKEN_RIGHT_BRACE);
	the->stack = aStack;
}

void fxTreeArrayExpression(txScriptParser* theParser, txSlot* theResult)
{
	txMachine* the = theParser->the;
	int elision = 1;

	mxPushInteger(0);
	mxPush(mxGlobal);
	fxNewID(the, gxTreeIDs[XSC_ARRAY]);
	*theResult = *(the->stack++);
	fxMatchToken(theParser, XS_TOKEN_LEFT_BRACKET);
	while ((theParser->token == XS_TOKEN_COMMA) || (gxTokenFlags[theParser->token] & XS_TOKEN_BEGIN_EXPRESSION)) {
		if (theParser->token == XS_TOKEN_COMMA) {
			fxGetNextToken(theParser);
			if (elision) {
				mxPushInteger(0);
				mxPush(mxGlobal);
				fxNewID(the, gxTreeIDs[XSC_UNDEFINED]);
				mxPushInteger(1);
				mxPush(*theResult);
				fxCallID(the, gxTreeIDs[XSC_APPEND]);
				the->stack++;
			}
			else
				elision = 1;
		}
		else {
			if (elision) {
				mxPushNull();
				fxTreeAssignmentExpression(theParser, the->stack);
				mxPushInteger(1);
				mxPush(*theResult);
				fxCallID(the, gxTreeIDs[XSC_APPEND]);
				the->stack++;
				elision = 0;
			}
			else 
				fxReportParserError(theParser, "missing ,");
		}
	}
	fxMatchToken(theParser, XS_TOKEN_RIGHT_BRACKET);
}

void fxTreeParameterBlock(txScriptParser* theParser, txSlot* theResult)
{
	txMachine* the = theParser->the;

	mxPushInteger(0);
	mxPush(mxGlobal);
	fxNewID(the, gxTreeIDs[XSC_PARAMS]);
	*theResult = *(the->stack++);
	fxMatchToken(theParser, XS_TOKEN_LEFT_PARENTHESIS);
	while (gxTokenFlags[theParser->token] & XS_TOKEN_BEGIN_EXPRESSION) {
		mxPushNull();
		fxTreeAssignmentExpression(theParser, the->stack);
		mxPushInteger(1);
		mxPush(*theResult);
		fxCallID(the, gxTreeIDs[XSC_APPEND]);
		the->stack++;
		if (theParser->token != XS_TOKEN_RIGHT_PARENTHESIS)
			fxMatchToken(theParser, XS_TOKEN_COMMA);
	}
	fxMatchToken(theParser, XS_TOKEN_RIGHT_PARENTHESIS);
}

void fxTreeArgumentBlock(txScriptParser* theParser, txSlot* theResult)
{
	txMachine* the = theParser->the;

	mxPushInteger(0);
	mxPush(mxGlobal);
	fxNewID(the, gxTreeIDs[XSC_ARGS]);
	*theResult = *(the->stack++);
	fxMatchToken(theParser, XS_TOKEN_LEFT_PARENTHESIS);
	while (theParser->token == XS_TOKEN_IDENTIFIER) {
		mxPushString(theParser->symbol->value.symbol.string);
		mxPushInteger(1);
		mxPush(mxGlobal);
		fxNewID(the, gxTreeIDs[XSC_ARG]);
		mxPushInteger(1);
		mxPush(*theResult);
		fxCallID(the, gxTreeIDs[XSC_APPEND]);
		the->stack++;
		fxGetNextToken(theParser);
		if (theParser->token != XS_TOKEN_RIGHT_PARENTHESIS)
			fxMatchToken(theParser, XS_TOKEN_COMMA);
	}
	fxMatchToken(theParser, XS_TOKEN_RIGHT_PARENTHESIS);
}

void fxTreeAppendExpression(txScriptParser* theParser, txSlot* theResult)
{
	txMachine* the = theParser->the;

	if (theResult->kind == XS_NULL_KIND) {
		*theResult = *(the->stack++);
	}
	else {
		txID anID = gxTreeIDs[XSC_EXPRESSIONS];
		mxPushInteger(1);
		if (theResult->ID != anID) {
			mxPush(*theResult);
			mxPushInteger(1);
			mxPushInteger(0);
			mxPush(mxGlobal);
			fxNewID(the, anID);
			*theResult = *the->stack;
			theResult->ID = anID;
			fxCallID(the, gxTreeIDs[XSC_APPEND]);
			the->stack++;
		}
		mxPush(*theResult);
		fxCallID(the, gxTreeIDs[XSC_APPEND]);
		the->stack++;
	}
}

void fxTreeAppendStatement(txScriptParser* theParser, txSlot* theResult)
{
	txMachine* the = theParser->the;

	mxPushInteger(1);
	if (theResult->kind == XS_NULL_KIND) {
		mxPushInteger(0);
		mxPush(mxGlobal);
		fxNewID(the, gxTreeIDs[XSC_STATEMENTS]);
		*theResult = *the->stack;
	}
	else {
		mxPush(*theResult);
	}
	fxCallID(the, gxTreeIDs[XSC_APPEND]);
	the->stack++;
}

void fxTreeAppendVariable(txScriptParser* theParser, txSlot* theResult)
{
	txMachine* the = theParser->the;

	the->scratch = *the->stack;
	if (theParser->arguments) {
		mxPush(the->scratch);
		mxPushInteger(1);
		mxPush(*(theParser->arguments));
		fxCallID(the, gxTreeIDs[XSC_CONTAINS]);
		if (fxRunTest(the)) {
			the->stack++;
			return;
		}
	}
	
	mxPush(the->scratch);
	mxPushInteger(1);
	mxPush(*(theParser->variables));
	fxCallID(the, gxTreeIDs[XSC_CONTAINS]);
	if (fxRunTest(the)) {
		the->stack++;
		return;
	}
	mxPushInteger(1);
	mxPush(*(theParser->variables));
	fxCallID(the, gxTreeIDs[XSC_APPEND]);
	the->stack++;
}

void fxTreeEscapeCString(txMachine* the)
{
	static txS1* gxURIHexa = (txS1*)"0123456789ABCDEF";
	txU1* src;
	txS4 size;
	txU1 c;
	txS1 *result;
	txU1* dst;

	fxToString(the, mxArgv(0));

	src = (txU1*)mxArgv(0)->value.string;
	size = 0;
	while ((c = *src++)) {
		if ((31 < c) && (c < 127))
			size += 1;
		else
			size += 4;
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
		if ((31 < c) && (c < 127))
			*dst++ = c;
		else {
			*dst++ = '\\';
			*dst++ = 'x';
			*dst++ = gxURIHexa[c >> 4];
			*dst++ = gxURIHexa[c & 15];
		}
	}
	*dst = 0;
	
	mxResult->value.string = (char *)result;
	mxResult->kind = XS_STRING_KIND;
}

void fxTreeGetLine(txMachine* the)
{
	mxResult->value.integer = the->parser->line;
	mxResult->kind = XS_INTEGER_KIND;
}


