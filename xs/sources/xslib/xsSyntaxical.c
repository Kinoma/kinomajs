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

void fxBody(txScriptParser* theParser, txSlot* theList, txSlot* theSymbol, txFlag theFlag)
{
	txSlot* aRouteLabel;
	txSlot* aList;
	txSlot* aSymbol;
	txSlot* aCode;
	
	theParser->returnLabel = theParser->throwLabel = aRouteLabel = fxNewLabel(theParser, C_NULL);
	aList = fxNewList(theParser);
	fxAppendLine(theParser, aList);
	
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
			fxAppendLine(theParser, aList);
			fxMatchToken(theParser, XS_TOKEN_FUNCTION);
			fxFunctionExpression(theParser, aList, 1);
		}
		else
			fxStatement(theParser, aList);
	}
	theParser->level--;
	
	//fprintf(stderr, "%d %s\n", theParser->strict, theSymbol ? theSymbol->value.symbol.string : "?");
	aSymbol = fxGetSymbol(theParser->the, theParser->the->evalID);
	if (theSymbol && (theSymbol->ID == theParser->the->evalID)) {
		if (theParser->strict)
			fxReportParserError(theParser, "eval function in strict code");
	}
	if (fxSearchLocal(theParser, aSymbol, theParser->arguments)) {
		if (theParser->strict)
			fxReportParserError(theParser, "eval parameter in strict code");
	}
	else if (fxSearchLocal(theParser, aSymbol, theParser->variables)) {
		if (theParser->strict)
			fxReportParserError(theParser, "eval variable in strict code");
	}

	aSymbol = fxGetSymbol(theParser->the, theParser->the->argumentsID);
	if (theSymbol && (theSymbol->ID == aSymbol->ID)) {
		theParser->parameters = 0;
		if (theParser->strict)
			fxReportParserError(theParser, "arguments function in strict code");
	}
	if (fxSearchLocal(theParser, aSymbol, theParser->arguments)) {
		theParser->parameters = 0;
		if (theParser->strict)
			fxReportParserError(theParser, "arguments parameter in strict code");
	}
	else if (fxSearchLocal(theParser, aSymbol, theParser->variables)) {
		theParser->parameters = 0;
		if (theParser->strict)
			fxReportParserError(theParser, "arguments variable in strict code");
	}
	if (theParser->sandbox)
		theFlag |= XS_SANDBOX_FLAG;
	if (theParser->strict)
		theFlag |= XS_STRICT_FLAG;
	
	aList = fxPasteList(theParser, theParser->functions, aList);
	if (theFlag & (XS_EVAL_FLAG | XS_PROGRAM_FLAG | XS_THIS_FLAG))
		fxScopeGlobals(theParser, aList, theFlag);
	else {
		if (theParser->strict)
			fxCheckArguments(theParser);
		fxScopeLocals(theParser, aList, theFlag);
	}

	aCode = fxAppendReference(theParser, theList, XS_BEGIN, theSymbol);
	aCode->flag = theFlag;
	fxAppendArguments(theParser, theList);
	fxAppendVariables(theParser, theList);
	fxAppendReference(theParser, theList, XS_ROUTE, aRouteLabel);
	if (theParser->parameters)
		fxAppendCode(theParser, theList, XS_PARAMETERS);
	if (theParser->debug && theParser->path)
		fxAppendReference(theParser, theList, XS_FILE, theParser->path);
	fxPasteList(theParser, theList, aList);
	
	fxAppendLabel(theParser, theList, aRouteLabel);
	fxAppendCode(theParser, theList, XS_END);
}

void fxStatementBlock(txScriptParser* theParser, txSlot* theList)
{
	fxMatchToken(theParser, XS_TOKEN_LEFT_BRACE);
	while ((theParser->token != XS_TOKEN_EOF) && (theParser->token != XS_TOKEN_RIGHT_BRACE))
		fxStatement(theParser, theList);
	fxMatchToken(theParser, XS_TOKEN_RIGHT_BRACE);
}

void fxStatement(txScriptParser* theParser, txSlot* theList)
{
	fxAppendLine(theParser, theList);
	switch (theParser->token) {
	case XS_TOKEN_COMMA:
	case XS_TOKEN_SEMICOLON:
		fxGetNextToken(theParser);
		break;
	case XS_TOKEN_BREAK:
		fxBreakStatement(theParser, theList);
		fxSemicolon(theParser, theList);
		break;
	case XS_TOKEN_CONTINUE:
		fxContinueStatement(theParser, theList);
		fxSemicolon(theParser, theList);
		break;
	case XS_TOKEN_DEBUGGER:
		fxDebuggerStatement(theParser, theList);
		fxSemicolon(theParser, theList);
		break;
	case XS_TOKEN_DO:
		fxDoStatement(theParser, theList);
		fxSemicolon(theParser, theList);
		break;
	case XS_TOKEN_FOR:
		fxForStatement(theParser, theList);
		break;
	case XS_TOKEN_IF:
		fxIfStatement(theParser, theList);
		break;
	case XS_TOKEN_RETURN:
		fxReturnStatement(theParser, theList);
		fxSemicolon(theParser, theList);
		break;
	case XS_TOKEN_LEFT_BRACE:
		fxStatementBlock(theParser, theList);
		break;
	case XS_TOKEN_SWITCH:
		fxSwitchStatement(theParser, theList);
		break;
	case XS_TOKEN_THROW:
		fxThrowStatement(theParser, theList);
		fxSemicolon(theParser, theList);
		break;
	case XS_TOKEN_TRY:
		fxTryStatement(theParser, theList);
		break;
	case XS_TOKEN_VAR:
		fxVarStatement(theParser, theList);
		fxSemicolon(theParser, theList);
		break;
	case XS_TOKEN_WHILE:
		fxWhileStatement(theParser, theList);
		break;
	case XS_TOKEN_WITH:
		if (theParser->strict)
			fxReportParserError(theParser, "no with in strict code");
		fxWithStatement(theParser, theList);
		break;
	case XS_TOKEN_IDENTIFIER:
		if (theParser->token2 == XS_TOKEN_COLON) {
			txSlot* aBreakLabel = fxNewLabel(theParser, theParser->symbol);
			fxGetNextToken(theParser);
			fxMatchToken(theParser, XS_TOKEN_COLON);
			fxPushBreakLabel(theParser, aBreakLabel);
			if ((theParser->token == XS_TOKEN_DO) || (theParser->token == XS_TOKEN_FOR) || (theParser->token == XS_TOKEN_WHILE))
				theParser->continueSymbol = aBreakLabel->value.label.symbol;
			fxStatement(theParser, theList);
			fxPopBreakLabel(theParser, aBreakLabel);
			fxAppendLabel(theParser, theList, aBreakLabel);
			break;
		}
		/* continue */
	default:
		if (gxTokenFlags[theParser->token] & XS_TOKEN_BEGIN_EXPRESSION) {
			fxCommaExpression(theParser, theList);
			if (!theParser->arguments)
				fxAppendCode(theParser, theList, XS_RESULT);
			fxAppendCode(theParser, theList, XS_POP);
			fxSemicolon(theParser, theList);
		}
		else {
			fxReportParserError(theParser, "invalid token %s", gxTokenNames[theParser->token]);
			fxGetNextToken(theParser);
		}
		break;
	}
}

void fxSemicolon(txScriptParser* theParser, txSlot* theList)
{
	if ((theParser->crlf) || (gxTokenFlags[theParser->token] & XS_TOKEN_END_STATEMENT)) {
		if ((theParser->token == XS_TOKEN_COMMA) || (theParser->token == XS_TOKEN_SEMICOLON))
			fxGetNextToken(theParser);
	}
	else
		fxReportParserError(theParser, "missing ;");
}

void fxBreakStatement(txScriptParser* theParser, txSlot* theList)
{
	txSlot* aLabel;
	
	fxMatchToken(theParser, XS_TOKEN_BREAK);
	if ((!(theParser->crlf)) && (theParser->token == XS_TOKEN_IDENTIFIER)) {
		aLabel = fxFindLabel(theParser, theParser->breakLabel, theParser->symbol);
		fxGetNextToken(theParser);
	}
	else 
		aLabel = fxFindLabel(theParser, theParser->breakLabel, C_NULL);
	if (aLabel)
		fxAppendReference(theParser, theList, XS_BRANCH, aLabel);
	else
		fxReportParserError(theParser, "invalid break");
}

void fxContinueStatement(txScriptParser* theParser, txSlot* theList)
{
	txSlot* aLabel;
	
	fxMatchToken(theParser, XS_TOKEN_CONTINUE);
	if ((!(theParser->crlf)) && (theParser->token == XS_TOKEN_IDENTIFIER)) {
		aLabel = fxFindLabel(theParser, theParser->continueLabel, theParser->symbol);
		fxGetNextToken(theParser);
	}
	else 
		aLabel = fxFindLabel(theParser, theParser->continueLabel, C_NULL);
	if (aLabel)
		fxAppendReference(theParser, theList, XS_BRANCH, aLabel);
	else
		fxReportParserError(theParser, "invalid continue");
}

void fxDebuggerStatement(txScriptParser* theParser, txSlot* theList)
{
	fxMatchToken(theParser, XS_TOKEN_DEBUGGER);
	fxAppendCode(theParser, theList, XS_DEBUGGER);
}

void fxDoStatement(txScriptParser* theParser, txSlot* theList)
{
	txSlot* aStack = theParser->the->stack;
	txSlot* aBreakLabel;
	txSlot* aContinueSymbolLabel;
	txSlot* aContinueLabel;
	txSlot* aLoopLabel;
	
	fxMatchToken(theParser, XS_TOKEN_DO);
	aBreakLabel = fxNewLabel(theParser, C_NULL);
	if (theParser->continueSymbol) {
		aContinueSymbolLabel = fxNewLabel(theParser, theParser->continueSymbol);
		theParser->continueSymbol = C_NULL;
	}
	else
		aContinueSymbolLabel =  C_NULL;
	aContinueLabel = fxNewLabel(theParser, C_NULL);
	aLoopLabel = fxNewLabel(theParser, C_NULL);
	fxAppendLabel(theParser, theList, aLoopLabel);
	fxPushBreakLabel(theParser, aBreakLabel);
	if (aContinueSymbolLabel)
		fxPushContinueLabel(theParser, aContinueSymbolLabel);
	fxPushContinueLabel(theParser, aContinueLabel);
	fxStatement(theParser, theList);
	fxPopContinueLabel(theParser, aContinueLabel);
	if (aContinueSymbolLabel)
		fxPopContinueLabel(theParser, aContinueSymbolLabel);
	fxPopBreakLabel(theParser, aBreakLabel);
	fxMatchToken(theParser, XS_TOKEN_WHILE);
	fxAppendLabel(theParser, theList, aContinueLabel);
	if (aContinueSymbolLabel)
		fxAppendLabel(theParser, theList, aContinueSymbolLabel);
	fxAppendLine(theParser, theList);
	fxMatchToken(theParser, XS_TOKEN_LEFT_PARENTHESIS);
	fxCommaExpression(theParser, theList);
	fxMatchToken(theParser, XS_TOKEN_RIGHT_PARENTHESIS);
	fxAppendReference(theParser, theList, XS_BRANCH_IF, aLoopLabel);
	fxAppendLabel(theParser, theList, aBreakLabel);
	theParser->the->stack = aStack;
}

void fxForStatement(txScriptParser* theParser, txSlot* theList)
{
	txSlot* aStack = theParser->the->stack;
	txSlot* aLeftList;
	txSlot* aRightList;
	txSlot* aBreakLabel;
	txSlot* aContinueSymbolLabel;
	txSlot* aContinueLabel;
	txSlot* anEnumLabel;
	txSlot* aLoopLabel;
	txInteger aCount;
	
	fxMatchToken(theParser, XS_TOKEN_FOR);
	aLeftList = fxNewList(theParser);
	aRightList = fxNewList(theParser);
	aBreakLabel = fxNewLabel(theParser, C_NULL);
	if (theParser->continueSymbol) {
		aContinueSymbolLabel = fxNewLabel(theParser, theParser->continueSymbol);
		theParser->continueSymbol = C_NULL;
	}
	else
		aContinueSymbolLabel =  C_NULL;
	aContinueLabel = fxNewLabel(theParser, C_NULL);
	anEnumLabel = fxNewLabel(theParser, C_NULL);
	aLoopLabel = fxNewLabel(theParser, C_NULL);
	
	fxMatchToken(theParser, XS_TOKEN_LEFT_PARENTHESIS);
	
	if (theParser->token == XS_TOKEN_VAR)
		aCount = fxVarStatementNoIn(theParser, aLeftList);
	else
		aCount = fxCommaExpressionNoIn(theParser, aLeftList);
	
	if (theParser->token == XS_TOKEN_IN) {
		if (aCount != 1)
			fxReportParserError(theParser, "list instead of reference");
		else {
			txSlot* aSlot = aLeftList->value.list.last;
			if (aSlot) {
				if ((XS_GET <= aSlot->kind) && (aSlot->kind <= XS_GET_MEMBER_AT))
					aSlot->kind += XS_SET - XS_GET;
				else
					fxReportParserError(theParser, "value instead of reference");
			}
		}
		fxGetNextToken(theParser);
		fxCommaExpression(theParser, theList);
		fxAppendCode(theParser, theList, XS_ENUM);
		fxAppendLabel(theParser, theList, aLoopLabel);
		fxAppendCode(theParser, theList, XS_DUB);
		fxAppendCode(theParser, theList, XS_NULL);
		fxAppendCode(theParser, theList, XS_STRICT_EQUAL);
		fxAppendReference(theParser, theList, XS_BRANCH_ELSE, anEnumLabel);
		fxAppendCode(theParser, theList, XS_POP);
		fxAppendReference(theParser, theList, XS_BRANCH, aBreakLabel);
		fxAppendLabel(theParser, theList, anEnumLabel);
		fxPasteList(theParser, theList, aLeftList);
		fxAppendCode(theParser, theList, XS_POP);
	}
	else {
		if (aCount)
			fxAppendCode(theParser, aLeftList, XS_POP);
		fxPasteList(theParser, theList, aLeftList);
		fxMatchToken(theParser, XS_TOKEN_SEMICOLON);
		fxAppendLabel(theParser, theList, aLoopLabel);
		if (gxTokenFlags[theParser->token] & XS_TOKEN_BEGIN_EXPRESSION) {
			fxCommaExpression(theParser, theList);
			fxAppendReference(theParser, theList, XS_BRANCH_ELSE, aBreakLabel);
		}
		fxMatchToken(theParser, XS_TOKEN_SEMICOLON);
		if (gxTokenFlags[theParser->token] & XS_TOKEN_BEGIN_EXPRESSION) {
			fxCommaExpression(theParser, aRightList);
			fxAppendCode(theParser, aRightList, XS_POP);
		}		
	}
	fxMatchToken(theParser, XS_TOKEN_RIGHT_PARENTHESIS);

	fxPushBreakLabel(theParser, aBreakLabel);
	if (aContinueSymbolLabel)
		fxPushContinueLabel(theParser, aContinueSymbolLabel);
	fxPushContinueLabel(theParser, aContinueLabel);
	fxStatement(theParser, theList);
	fxPopContinueLabel(theParser, aContinueLabel);
	if (aContinueSymbolLabel)
		fxPopContinueLabel(theParser, aContinueSymbolLabel);
	fxPopBreakLabel(theParser, aBreakLabel);
	fxAppendLabel(theParser, theList, aContinueLabel);
	if (aContinueSymbolLabel)
		fxAppendLabel(theParser, theList, aContinueSymbolLabel);
	fxPasteList(theParser, theList, aRightList);
	fxAppendReference(theParser, theList, XS_BRANCH, aLoopLabel);
	fxAppendLabel(theParser, theList, aBreakLabel);
	
	theParser->the->stack = aStack;
}

void fxIfStatement(txScriptParser* theParser, txSlot* theList)
{
	txSlot* aStack = theParser->the->stack;
	txSlot* anIfLabel;
	txSlot* anElseLabel;

	fxMatchToken(theParser, XS_TOKEN_IF);
	anIfLabel = fxNewLabel(theParser, C_NULL);
	fxMatchToken(theParser, XS_TOKEN_LEFT_PARENTHESIS);
	fxCommaExpression(theParser, theList);
	fxMatchToken(theParser, XS_TOKEN_RIGHT_PARENTHESIS);
	fxAppendReference(theParser, theList, XS_BRANCH_ELSE, anIfLabel);
	fxStatement(theParser, theList);
	if (theParser->token == XS_TOKEN_ELSE) {
		anElseLabel = fxNewLabel(theParser, C_NULL);
		fxAppendReference(theParser, theList, XS_BRANCH, anElseLabel);
		fxAppendLabel(theParser, theList, anIfLabel);
		fxGetNextToken(theParser);
		fxStatement(theParser, theList);
		fxAppendLabel(theParser, theList, anElseLabel);
	}
	else
		fxAppendLabel(theParser, theList, anIfLabel);
	theParser->the->stack = aStack;
}

void fxReturnStatement(txScriptParser* theParser, txSlot* theList)
{
	fxMatchToken(theParser, XS_TOKEN_RETURN);
	if (!theParser->arguments)
		fxReportParserError(theParser, "invalid return");
	
	if ((theParser->crlf) || (gxTokenFlags[theParser->token] & XS_TOKEN_END_STATEMENT)) {
		//fxAppendCode(theParser, theList, XS_UNDEFINED);
	}
	else {
		fxCommaExpression(theParser, theList);
		fxAppendCode(theParser, theList, XS_RESULT);
	}
	fxAppendReference(theParser, theList, XS_BRANCH, theParser->returnLabel);
}

void fxSwitchStatement(txScriptParser* theParser, txSlot* theList)
{
	txSlot* aStack = theParser->the->stack;
	txSlot* aList;
	txSlot* aBreakLabel;
	txSlot* aCaseLabel;
	txSlot* aDefaultLabel = C_NULL;
	
	fxMatchToken(theParser, XS_TOKEN_SWITCH);
	aList = fxNewList(theParser);
	aBreakLabel = fxNewLabel(theParser, C_NULL);
	fxMatchToken(theParser, XS_TOKEN_LEFT_PARENTHESIS);
	fxCommaExpression(theParser, theList);
	fxMatchToken(theParser, XS_TOKEN_RIGHT_PARENTHESIS);
	fxMatchToken(theParser, XS_TOKEN_LEFT_BRACE);
	while ((theParser->token == XS_TOKEN_CASE) || (theParser->token == XS_TOKEN_DEFAULT)) {
		if (theParser->token == XS_TOKEN_CASE) {
			fxGetNextToken(theParser);
			aCaseLabel = fxNewLabel(theParser, C_NULL);
			fxAppendCode(theParser, theList, XS_DUB);
			fxCommaExpression(theParser, theList);
			fxAppendCode(theParser, theList, XS_STRICT_NOT_EQUAL);
			fxAppendReference(theParser, theList, XS_BRANCH_ELSE, aCaseLabel);
			fxMatchToken(theParser, XS_TOKEN_COLON);
			fxAppendLabel(theParser, aList, aCaseLabel);
			fxPushBreakLabel(theParser, aBreakLabel);
			while (gxTokenFlags[theParser->token] & XS_TOKEN_BEGIN_STATEMENT)
				fxStatement(theParser, aList);
			fxPopBreakLabel(theParser, aBreakLabel);
		}
		else {
			fxGetNextToken(theParser);
			if (aDefaultLabel) 
				fxReportParserError(theParser, "invalid default");
			aDefaultLabel = fxNewLabel(theParser, C_NULL);
			fxMatchToken(theParser, XS_TOKEN_COLON);
			fxAppendLabel(theParser, aList, aDefaultLabel);
			fxPushBreakLabel(theParser, aBreakLabel);
			while (gxTokenFlags[theParser->token] & XS_TOKEN_BEGIN_STATEMENT)
				fxStatement(theParser, aList);
			fxPopBreakLabel(theParser, aBreakLabel);
		}
	}	
	if (aDefaultLabel)
		fxAppendReference(theParser, theList, XS_BRANCH, aDefaultLabel);
	else
		fxAppendReference(theParser, theList, XS_BRANCH, aBreakLabel);
	fxMatchToken(theParser, XS_TOKEN_RIGHT_BRACE);
	fxPasteList(theParser, theList, aList);
	fxAppendLabel(theParser, theList, aBreakLabel);
	fxAppendCode(theParser, theList, XS_POP);
	theParser->the->stack = aStack;
}

void fxThrowStatement(txScriptParser* theParser, txSlot* theList)
{
	fxMatchToken(theParser, XS_TOKEN_THROW);
	if ((theParser->crlf) || (gxTokenFlags[theParser->token] & XS_TOKEN_END_STATEMENT))
		fxAppendCode(theParser, theList, XS_UNDEFINED);
	else
		fxCommaExpression(theParser, theList);
	fxAppendCode(theParser, theList, XS_THROW);
}

void fxTryStatement(txScriptParser* theParser, txSlot* theList)
{
	txSlot* aStack = theParser->the->stack;
	txSlot* saveBreakLabel = theParser->breakLabel;
	txSlot* saveContinueLabel = theParser->continueLabel;
	txSlot* saveReturnLabel = theParser->returnLabel;
	txSlot* saveThrowLabel = theParser->throwLabel;
	txSlot* aFinallyLabel;
	txSlot* aBreakLabel;
	txSlot* aContinueLabel;
	txSlot* aReturnLabel;
	txSlot* aThrowLabel;
	txSlot* aSymbol;
	txID anID = 0;
	
	fxMatchToken(theParser, XS_TOKEN_TRY);
	aFinallyLabel = fxNewLabel(theParser, C_NULL);
	
	theParser->breakLabel = aBreakLabel = fxDubLabel(theParser, saveBreakLabel, &anID);
	theParser->continueLabel = aContinueLabel = fxDubLabel(theParser, saveContinueLabel, &anID);
	theParser->returnLabel = aReturnLabel = fxDubLabel(theParser, saveReturnLabel, &anID);
	theParser->throwLabel = aThrowLabel = fxNewLabel(theParser, C_NULL);
	fxAppendReference(theParser, theList, XS_ROUTE, aThrowLabel);
	fxStatementBlock(theParser, theList);
	fxAppendStatus(theParser, theList, anID);
	fxAppendReference(theParser, theList, XS_BRANCH, aFinallyLabel);
	fxAppendLabel(theParser, theList, aThrowLabel);
	
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
			fxGetNextToken(theParser);
		}
		else {
			aSymbol = fxNewSymbolC(theParser->the, "?");
			fxReportParserError(theParser, "missing identifier");
		}
		fxMatchToken(theParser, XS_TOKEN_RIGHT_PARENTHESIS);
		fxAppendReference(theParser, theList, XS_CATCH, aSymbol);
		
		theParser->throwLabel = aThrowLabel = fxNewLabel(theParser, C_NULL);
		fxAppendReference(theParser, theList, XS_ROUTE, aThrowLabel);
		fxStatementBlock(theParser, theList);
		fxAppendStatus(theParser, theList, anID);
		fxAppendReference(theParser, theList, XS_BRANCH, aFinallyLabel);
		fxAppendLabel(theParser, theList, aThrowLabel);
	}
	else
		aSymbol = C_NULL;
	fxAppendStatus(theParser, theList, XS_NO_ID);
	fxAppendReference(theParser, theList, XS_BRANCH, aFinallyLabel);
	fxBranchLabel(theParser, aBreakLabel, theList, aFinallyLabel);
	fxBranchLabel(theParser, aContinueLabel, theList, aFinallyLabel);
	fxBranchLabel(theParser, aReturnLabel, theList, aFinallyLabel);
	
	fxAppendLabel(theParser, theList, aFinallyLabel);
	if (aSymbol)
		fxAppendCode(theParser, theList, XS_UNCATCH);
	theParser->continueLabel = saveContinueLabel;
	theParser->breakLabel = saveBreakLabel;
	theParser->returnLabel = saveReturnLabel;
	theParser->throwLabel = saveThrowLabel;
	fxAppendReference(theParser, theList, XS_ROUTE, saveThrowLabel);
	
	if (theParser->token == XS_TOKEN_FINALLY) {
		fxGetNextToken(theParser);
		fxStatementBlock(theParser, theList);
	}
	
	fxAppendCode(theParser, theList, XS_JUMP);
	fxJumpLabel(theParser, aBreakLabel, theList);
	fxJumpLabel(theParser, aContinueLabel, theList);
	fxJumpLabel(theParser, aReturnLabel, theList);
	
	theParser->the->stack = aStack;
}

void fxVarStatement(txScriptParser* theParser, txSlot* theList)
{
	txSlot* aSymbol;
	txTokenFlag aFlag = gxTokenFlags[XS_TOKEN_IN];
	gxTokenFlags[XS_TOKEN_IN] = XS_TOKEN_RELATIONAL_EXPRESSION;
	fxMatchToken(theParser, XS_TOKEN_VAR);
	while (theParser->token == XS_TOKEN_IDENTIFIER) {
		fxAppendLine(theParser, theList);
		aSymbol = theParser->symbol;
		fxNewLocal(theParser, aSymbol, theParser->variables);
		fxGetNextToken(theParser);
		if (theParser->token == XS_TOKEN_ASSIGN) {
			fxGetNextToken(theParser);
			fxAssignmentExpression(theParser, theList);
			fxAppendReference(theParser, theList, XS_SET, aSymbol);
			fxAppendCode(theParser, theList, XS_POP);
		}
		if (theParser->token == XS_TOKEN_COMMA)
			fxGetNextToken(theParser);
		else
			break;
	}
	gxTokenFlags[XS_TOKEN_IN] = aFlag;
}

txInteger fxVarStatementNoIn(txScriptParser* theParser, txSlot* theList)
{
	txSlot* aSymbol;
	txInteger aCount = 0;
	txTokenFlag aFlag = gxTokenFlags[XS_TOKEN_IN];
	gxTokenFlags[XS_TOKEN_IN] = 0;
	fxMatchToken(theParser, XS_TOKEN_VAR);
	while (theParser->token == XS_TOKEN_IDENTIFIER) {
		aCount++;
		fxAppendLine(theParser, theList);
		aSymbol = theParser->symbol;
		fxNewLocal(theParser, aSymbol, theParser->variables);
		fxGetNextToken(theParser);
		if (theParser->token == XS_TOKEN_ASSIGN) {
			fxGetNextToken(theParser);
			fxAssignmentExpression(theParser, theList);
			fxAppendReference(theParser, theList, XS_SET, aSymbol);
		}
		else
			fxAppendReference(theParser, theList, XS_GET, aSymbol);
		if (theParser->token != XS_TOKEN_COMMA)
			break;
		fxGetNextToken(theParser);
		fxAppendCode(theParser, theList, XS_POP);
	}
	gxTokenFlags[XS_TOKEN_IN] = aFlag;
	return aCount;
}

void fxWhileStatement(txScriptParser* theParser, txSlot* theList)
{
	txSlot* aStack = theParser->the->stack;
	txSlot* aBreakLabel;
	txSlot* aContinueSymbolLabel;
	txSlot* aContinueLabel;
	
	fxMatchToken(theParser, XS_TOKEN_WHILE);
	aBreakLabel = fxNewLabel(theParser, C_NULL);
	if (theParser->continueSymbol) {
		aContinueSymbolLabel = fxNewLabel(theParser, theParser->continueSymbol);
		theParser->continueSymbol = C_NULL;
	}
	else
		aContinueSymbolLabel =  C_NULL;
	aContinueLabel = fxNewLabel(theParser, C_NULL);
	fxAppendLabel(theParser, theList, aContinueLabel);
	if (aContinueSymbolLabel)
		fxAppendLabel(theParser, theList, aContinueSymbolLabel);
	fxMatchToken(theParser, XS_TOKEN_LEFT_PARENTHESIS);
	fxCommaExpression(theParser, theList);
	fxMatchToken(theParser, XS_TOKEN_RIGHT_PARENTHESIS);
	fxAppendReference(theParser, theList, XS_BRANCH_ELSE, aBreakLabel);
	fxPushBreakLabel(theParser, aBreakLabel);
	if (aContinueSymbolLabel)
		fxPushContinueLabel(theParser, aContinueSymbolLabel);
	fxPushContinueLabel(theParser, aContinueLabel);
	fxStatement(theParser, theList);
	fxPopContinueLabel(theParser, aContinueLabel);
	if (aContinueSymbolLabel)
		fxPopContinueLabel(theParser, aContinueSymbolLabel);
	fxPopBreakLabel(theParser, aBreakLabel);
	fxAppendReference(theParser, theList, XS_BRANCH, aContinueLabel);
	fxAppendLabel(theParser, theList, aBreakLabel);
	theParser->the->stack = aStack;
}

void fxWithStatement(txScriptParser* theParser, txSlot* theList)
{
	txSlot* aStack = theParser->the->stack;
	txSlot* saveBreakLabel = theParser->breakLabel;
	txSlot* saveContinueLabel = theParser->continueLabel;
	txSlot* saveReturnLabel = theParser->returnLabel;
	txSlot* saveThrowLabel = theParser->throwLabel;
	txSlot* aScopeLabel;
	txSlot* aBreakLabel;
	txSlot* aContinueLabel;
	txSlot* aReturnLabel;
	txSlot* aThrowLabel;
	txID anID = 0;


	fxMatchToken(theParser, XS_TOKEN_WITH);
	
	fxMatchToken(theParser, XS_TOKEN_LEFT_PARENTHESIS);
	fxCommaExpression(theParser, theList);
	fxMatchToken(theParser, XS_TOKEN_RIGHT_PARENTHESIS);
	
	fxAppendCode(theParser, theList, XS_SCOPE);
	aScopeLabel = fxNewLabel(theParser, C_NULL);
	theParser->breakLabel = aBreakLabel = fxDubLabel(theParser, saveBreakLabel, &anID);
	theParser->continueLabel = aContinueLabel = fxDubLabel(theParser, saveContinueLabel, &anID);
	theParser->returnLabel = aReturnLabel = fxDubLabel(theParser, saveReturnLabel, &anID);
	theParser->throwLabel = aThrowLabel = fxNewLabel(theParser, C_NULL);
	fxAppendReference(theParser, theList, XS_ROUTE, aThrowLabel);
	fxStatement(theParser, theList);
	fxAppendStatus(theParser, theList, anID);
	fxAppendReference(theParser, theList, XS_BRANCH, aScopeLabel);
	fxBranchLabel(theParser, aBreakLabel, theList, aScopeLabel);
	fxBranchLabel(theParser, aContinueLabel, theList, aScopeLabel);
	fxBranchLabel(theParser, aReturnLabel, theList, aScopeLabel);
	fxAppendLabel(theParser, theList, aThrowLabel);
	fxAppendStatus(theParser, theList, XS_NO_ID);
	fxAppendLabel(theParser, theList, aScopeLabel);
	fxAppendCode(theParser, theList, XS_UNSCOPE);
	theParser->continueLabel = saveContinueLabel;
	theParser->breakLabel = saveBreakLabel;
	theParser->returnLabel = saveReturnLabel;
	theParser->throwLabel = saveThrowLabel;
	fxAppendReference(theParser, theList, XS_ROUTE, saveThrowLabel);
	fxAppendCode(theParser, theList, XS_JUMP);
	fxJumpLabel(theParser, aBreakLabel, theList);
	fxJumpLabel(theParser, aContinueLabel, theList);
	fxJumpLabel(theParser, aReturnLabel, theList);
	theParser->the->stack = aStack;
}

void fxCommaExpression(txScriptParser* theParser, txSlot* theList)
{
	txTokenFlag aFlag = gxTokenFlags[XS_TOKEN_IN];
	gxTokenFlags[XS_TOKEN_IN] = XS_TOKEN_RELATIONAL_EXPRESSION;
	while (gxTokenFlags[theParser->token] & XS_TOKEN_BEGIN_EXPRESSION) {
		fxAssignmentExpression(theParser, theList);
		if (theParser->token != XS_TOKEN_COMMA) 
			break;
		fxGetNextToken(theParser);
		fxAppendCode(theParser, theList, XS_POP);
	}
	gxTokenFlags[XS_TOKEN_IN] = aFlag;
}

txInteger fxCommaExpressionNoIn(txScriptParser* theParser, txSlot* theList)
{
	txInteger aCount = 0;
	txTokenFlag aFlag = gxTokenFlags[XS_TOKEN_IN];
	gxTokenFlags[XS_TOKEN_IN] = 0;
	while (gxTokenFlags[theParser->token] & XS_TOKEN_BEGIN_EXPRESSION) {
		aCount++;
		fxAssignmentExpression(theParser, theList);
		if (theParser->token != XS_TOKEN_COMMA) 
			break;
		fxGetNextToken(theParser);
		fxAppendCode(theParser, theList, XS_POP);
	}
	gxTokenFlags[XS_TOKEN_IN] = aFlag;
	return aCount;
}

void fxAssignmentExpression(txScriptParser* theParser, txSlot* theList)
{
	txSlot* aStack = theParser->the->stack;
	txSlot* aSlot;
	txToken aToken;
	txSlot* aList;
	
	fxAppendLine(theParser, theList);
	aSlot = theList->value.list.last;
	fxConditionalExpression(theParser, theList);
	while (gxTokenFlags[theParser->token] & XS_TOKEN_ASSIGN_EXPRESSION) {
		aToken = theParser->token;
		fxGetNextToken(theParser);
		aList = fxNewList(theParser);
		if (aToken != XS_TOKEN_ASSIGN)
			fxCopyList(theParser, aList, theList, aSlot);
		else
			fxCutList(theParser, aList, theList, aSlot);
		fxAssignmentExpression(theParser, theList);
		if (aToken != XS_TOKEN_ASSIGN) 
			fxAppendCode(theParser, theList, gxTokenCodes[aToken]);
		fxPasteList(theParser, theList, aList);
		aSlot = theList->value.list.last;
		if (aSlot) {
			if ((XS_GET <= aSlot->kind) && (aSlot->kind <= XS_GET_MEMBER_AT)) {
				if (theParser->strict)
					fxCheckNoArgumentsNoEval(theParser, aSlot);
				aSlot->kind += XS_SET - XS_GET;
			}
			else
				fxReportParserError(theParser, "value instead of reference");
		}
	}
	theParser->the->stack = aStack;
}

void fxConditionalExpression(txScriptParser* theParser, txSlot* theList)
{
	txSlot* aStack = theParser->the->stack;
	txSlot* anIfLabel;
	txSlot* anElseLabel;

	fxOrExpression(theParser, theList);
	if (theParser->token == XS_TOKEN_QUESTION_MARK) {
		fxGetNextToken(theParser);
		anIfLabel = fxNewLabel(theParser, C_NULL);
		anElseLabel = fxNewLabel(theParser, C_NULL);
		fxAppendReference(theParser, theList, XS_BRANCH_ELSE, anElseLabel);
		fxAssignmentExpression(theParser, theList);
		fxAppendReference(theParser, theList, XS_BRANCH, anIfLabel);
		fxAppendLabel(theParser, theList, anElseLabel);
		fxMatchToken(theParser, XS_TOKEN_COLON);
		fxAssignmentExpression(theParser, theList);
		fxAppendLabel(theParser, theList, anIfLabel);
	}
	theParser->the->stack = aStack;
}

void fxOrExpression(txScriptParser* theParser, txSlot* theList)
{
	txSlot* aStack = theParser->the->stack;
	txSlot* aLabel;

	fxAndExpression(theParser, theList);
	if (theParser->token == XS_TOKEN_OR) {
		aLabel = fxNewLabel(theParser, C_NULL);
		while (theParser->token == XS_TOKEN_OR) {
			fxAppendCode(theParser, theList, XS_DUB);
			fxAppendReference(theParser, theList, XS_BRANCH_IF, aLabel);
			fxAppendCode(theParser, theList, XS_POP);
			fxGetNextToken(theParser);
			fxAndExpression(theParser, theList);
		}
		fxAppendLabel(theParser, theList, aLabel);
	}
	theParser->the->stack = aStack;
}

void fxAndExpression(txScriptParser* theParser, txSlot* theList)
{
	txSlot* aStack = theParser->the->stack;
	txSlot* aLabel;

	fxBitOrExpression(theParser, theList);
	if (theParser->token == XS_TOKEN_AND) {
		aLabel = fxNewLabel(theParser, C_NULL);
		while (theParser->token == XS_TOKEN_AND) {
			fxAppendCode(theParser, theList, XS_DUB);
			fxAppendReference(theParser, theList, XS_BRANCH_ELSE, aLabel);
			fxAppendCode(theParser, theList, XS_POP);
			fxGetNextToken(theParser);
			fxBitOrExpression(theParser, theList);
		}
		fxAppendLabel(theParser, theList, aLabel);
	}
	theParser->the->stack = aStack;
}

void fxBitOrExpression(txScriptParser* theParser, txSlot* theList)
{
	fxBitXorExpression(theParser, theList);
	while (theParser->token == XS_TOKEN_BIT_OR) {
		fxGetNextToken(theParser);
		fxBitXorExpression(theParser, theList);
		fxAppendCode(theParser, theList, XS_BIT_OR);
	}
}

void fxBitXorExpression(txScriptParser* theParser, txSlot* theList)
{
	fxBitAndExpression(theParser, theList);
	while (theParser->token == XS_TOKEN_BIT_XOR) {
		fxGetNextToken(theParser);
		fxBitAndExpression(theParser, theList);
		fxAppendCode(theParser, theList, XS_BIT_XOR);
	}
}

void fxBitAndExpression(txScriptParser* theParser, txSlot* theList)
{
	fxEqualExpression(theParser, theList);
	while (theParser->token == XS_TOKEN_BIT_AND) {
		fxGetNextToken(theParser);
		fxEqualExpression(theParser, theList);
		fxAppendCode(theParser, theList, XS_BIT_AND);
	}
}

void fxEqualExpression(txScriptParser* theParser, txSlot* theList)
{
	txToken aToken;
	
	fxRelationalExpression(theParser, theList);
	while (gxTokenFlags[theParser->token] & XS_TOKEN_EQUAL_EXPRESSION) {
		aToken = theParser->token;
		fxGetNextToken(theParser);
		fxRelationalExpression(theParser, theList);
		fxAppendCode(theParser, theList, gxTokenCodes[aToken]);
	}
}

void fxRelationalExpression(txScriptParser* theParser, txSlot* theList)
{
	txToken aToken;
	
	fxShiftExpression(theParser, theList);
	while (gxTokenFlags[theParser->token] & XS_TOKEN_RELATIONAL_EXPRESSION) {
		aToken = theParser->token;
		fxGetNextToken(theParser);
		fxShiftExpression(theParser, theList);
		fxAppendCode(theParser, theList, gxTokenCodes[aToken]);
	}
}

void fxShiftExpression(txScriptParser* theParser, txSlot* theList)
{
	txToken aToken;
	
	fxAdditiveExpression(theParser, theList);
	while (gxTokenFlags[theParser->token] & XS_TOKEN_SHIFT_EXPRESSION) {
		aToken = theParser->token;
		fxGetNextToken(theParser);
		fxAdditiveExpression(theParser, theList);
		fxAppendCode(theParser, theList, gxTokenCodes[aToken]);
	}
}

void fxAdditiveExpression(txScriptParser* theParser, txSlot* theList)
{
	txToken aToken;
	
	fxMultiplicativeExpression(theParser, theList);
	while (gxTokenFlags[theParser->token] & XS_TOKEN_ADDITIVE_EXPRESSION) {
		aToken = theParser->token;
		fxGetNextToken(theParser);
		fxMultiplicativeExpression(theParser, theList);
		fxAppendCode(theParser, theList, gxTokenCodes[aToken]);
	}
}

void fxMultiplicativeExpression(txScriptParser* theParser, txSlot* theList)
{
	txToken aToken;
	
	fxPrefixExpression(theParser, theList);
	while (gxTokenFlags[theParser->token] & XS_TOKEN_MULTIPLICATIVE_EXPRESSION) {
		aToken = theParser->token;
		fxGetNextToken(theParser);
		fxPrefixExpression(theParser, theList);
		fxAppendCode(theParser, theList, gxTokenCodes[aToken]);
	}
}

void fxPrefixExpression(txScriptParser* theParser, txSlot* theList)
{
	txSlot* aStack = theParser->the->stack;
	txToken aToken;
	txSlot* aSlot;
	txSlot* aList;
	
	if (gxTokenFlags[theParser->token] & XS_TOKEN_PREFIX_EXPRESSION) {
		aToken = theParser->token;
		fxGetNextToken(theParser);
		aSlot = theList->value.list.last;
		fxPrefixExpression(theParser, theList);
		if (aToken == XS_TOKEN_ADD)
			fxAppendCode(theParser, theList, XS_PLUS);
		else if (aToken == XS_TOKEN_SUBTRACT)
			fxAppendCode(theParser, theList, XS_MINUS);
		else if ((aToken == XS_TOKEN_DECREMENT) || (aToken == XS_TOKEN_INCREMENT)) {
			aList = fxNewList(theParser);
			fxCopyList(theParser, aList, theList, aSlot);
			fxAppendCode(theParser, theList, gxTokenCodes[aToken]);
			fxPasteList(theParser, theList, aList);
			aSlot = theList->value.list.last;
			if (aSlot) {
				if ((XS_GET <= aSlot->kind) && (aSlot->kind <= XS_GET_MEMBER_AT)) {
					if (theParser->strict)
						fxCheckNoArgumentsNoEval(theParser, aSlot);
					aSlot->kind += XS_SET - XS_GET;
				}
				else
					fxReportParserError(theParser, "value instead of reference");
			}
		}
		else if (aToken == XS_TOKEN_DELETE) {
			aSlot = theList->value.list.last;
			if (aSlot) {
				if (theParser->strict && (aSlot->kind == XS_GET))
					fxReportParserError(theParser, "delete identifier (strict mode)");
				if ((XS_GET <= aSlot->kind) && (aSlot->kind <= XS_GET_MEMBER_AT))
					aSlot->kind += XS_DELETE - XS_GET;
				else
					fxAppendReference(theParser, theList, XS_DELETE, C_NULL);
					//fxReportParserError(theParser, "value instead of reference");
			}
		}
		else
			fxAppendCode(theParser, theList, gxTokenCodes[aToken]);
	}
	else
		fxPostfixExpression(theParser, theList);
	theParser->the->stack = aStack;
}

void fxPostfixExpression(txScriptParser* theParser, txSlot* theList)
{
	txSlot* aStack = theParser->the->stack;
	txSlot* aSlot;
	txToken aToken;
	txSlot* aList;
	
	aSlot = theList->value.list.last;
	fxCallExpression(theParser, theList);
	if ((!(theParser->crlf)) && (gxTokenFlags[theParser->token] & XS_TOKEN_POSTFIX_EXPRESSION)) {
		aToken = theParser->token;
		fxGetNextToken(theParser);
		aList = fxNewList(theParser);
		fxCopyList(theParser, aList, theList, aSlot);
		fxAppendCode(theParser, theList, XS_DUB);
		fxAppendCode(theParser, theList, gxTokenCodes[aToken]);
		fxPasteList(theParser, theList, aList);
		aSlot = theList->value.list.last;
		if (aSlot) {
			if ((XS_GET <= aSlot->kind) && (aSlot->kind <= XS_GET_MEMBER_AT)) {
				if (theParser->strict)
					fxCheckNoArgumentsNoEval(theParser, aSlot);
				aSlot->kind += XS_SET - XS_GET;
			}
			else
				fxReportParserError(theParser, "value instead of reference");
		}
		fxAppendCode(theParser, theList, XS_POP);
	}
	theParser->the->stack = aStack;
}

void fxCallExpression(txScriptParser* theParser, txSlot* theList)
{
	txSlot* aStack = theParser->the->stack;
	txSlot* aSlot;
	txSlot* aList;
	txSlot* aCall;

	aSlot = theList->value.list.last;
	fxLiteralExpression(theParser, theList);
	for (;;) {
		if (theParser->token == XS_TOKEN_DOT) {
			fxGetNextToken(theParser);
			if (theParser->token == XS_TOKEN_IDENTIFIER) {
				fxAppendReference(theParser, theList, XS_GET_MEMBER, (txSlot*)(theParser->symbol));
				fxGetNextToken(theParser);
			}
			else
				fxReportParserError(theParser, "missing property");
		}
		else if (theParser->token == XS_TOKEN_LEFT_BRACKET) {
			fxGetNextToken(theParser);
			fxCommaExpression(theParser, theList);
			fxMatchToken(theParser, XS_TOKEN_RIGHT_BRACKET);
			fxAppendCode(theParser, theList, XS_GET_MEMBER_AT);
		}
		else if (theParser->token == XS_TOKEN_LEFT_PARENTHESIS) {
			aList = fxNewList(theParser);
			fxCutList(theParser, aList, theList, aSlot);
			fxParameterBlock(theParser, theList);
			aCall = aList->value.list.last;
			if (aCall) {
				if ((XS_GET <= aCall->kind) && (aCall->kind <= XS_GET_MEMBER_AT)) {
				}
				else {
					fxAppendCode(theParser, theList, XS_UNDEFINED);
				}
			}
			fxPasteList(theParser, theList, aList);
			fxAppendCode(theParser, theList, XS_CALL);
		}
		else
			break;
	} 
	theParser->the->stack = aStack;
}

void fxLiteralExpression(txScriptParser* theParser, txSlot* theList)
{
	txSlot* aSymbol;
	
	switch (theParser->token) {
	case XS_TOKEN_THIS:
	case XS_TOKEN_NULL:
	case XS_TOKEN_TRUE:
	case XS_TOKEN_FALSE:
		fxAppendCode(theParser, theList, gxTokenCodes[theParser->token]);
		fxGetNextToken(theParser);
		break;
	case XS_TOKEN_INTEGER:
		fxAppendInteger(theParser, theList, theParser->integer);
		fxGetNextToken(theParser);
		break;
	case XS_TOKEN_NUMBER:
		fxAppendNumber(theParser, theList, theParser->number);
		fxGetNextToken(theParser);
		break;
	case XS_TOKEN_STRING:
		fxAppendString(theParser, theList, theParser->string->value.string);
		fxGetNextToken(theParser);
		break;
	case XS_TOKEN_REGEXP:
		fxAppendString(theParser, theList, theParser->string->value.string);
		fxAppendString(theParser, theList, theParser->flags->value.string);
		fxAppendInteger(theParser, theList, 2);
		aSymbol = fxNewSymbolC(theParser->the, "RegExp");
		fxAppendReference(theParser, theList, XS_GET, aSymbol);
		fxAppendCode(theParser, theList, XS_NEW);
		fxGetNextToken(theParser);
		break;
	case XS_TOKEN_IDENTIFIER:
		aSymbol = theParser->symbol;
		if (c_strcmp(aSymbol->value.symbol.string, "undefined") == 0) {
			fxAppendCode(theParser, theList, XS_UNDEFINED);
		}
		else {
			if (c_strcmp(aSymbol->value.symbol.string, "arguments") == 0)
				theParser->parameters = 1;
			fxAppendReference(theParser, theList, XS_GET, aSymbol);
		}
		fxGetNextToken(theParser);
		break;
	case XS_TOKEN_FUNCTION:
		fxMatchToken(theParser, XS_TOKEN_FUNCTION);
		fxFunctionExpression(theParser, theList, 0);
		break;
	case XS_TOKEN_NEW:
		fxMatchToken(theParser, XS_TOKEN_NEW);
		fxNewExpression(theParser, theList);
		break;
	case XS_TOKEN_LEFT_BRACE:
		fxObjectExpression(theParser, theList);
		break;
	case XS_TOKEN_LEFT_BRACKET:
		fxArrayExpression(theParser, theList);
		break;
	case XS_TOKEN_LEFT_PARENTHESIS:
		fxMatchToken(theParser, XS_TOKEN_LEFT_PARENTHESIS);
		fxCommaExpression(theParser, theList);
		fxMatchToken(theParser, XS_TOKEN_RIGHT_PARENTHESIS);
		break;
	default:
		fxReportParserError(theParser, "missing expression");
		break;
	}
}

void fxFunctionExpression(txScriptParser* theParser, txSlot* theList, txFlag declareIt)
{
	txSlot* aStack = theParser->the->stack;
	txSlot* aBreakLabel = theParser->breakLabel;
	txSlot* aContinueLabel = theParser->continueLabel;
	txSlot* aReturnLabel = theParser->returnLabel;
	txSlot* aThrowLabel = theParser->throwLabel;
	txSlot* arguments = theParser->arguments;
	txSlot* variables = theParser->variables;
	txSlot* functions = theParser->functions;
	txBoolean aParametersFlag = theParser->parameters;
	txBoolean aStrictFlag = theParser->strict;

	txSlot* aFunctionLabel;
	txSlot* aSymbol = C_NULL;
	
	theParser->line0 = 0; // ???
	
	theParser->breakLabel = NULL;
	theParser->continueLabel = NULL;
	theParser->returnLabel = NULL;
	theParser->throwLabel = NULL;
	theParser->arguments = fxNewList(theParser);
	theParser->arguments->next = arguments;
	theParser->variables = fxNewList(theParser);
	theParser->variables->next = variables;
	theParser->functions = fxNewList(theParser);
	theParser->parameters = 0;
	
	if (declareIt)
		theList = functions;
	if (theParser->token == XS_TOKEN_IDENTIFIER) {
		aSymbol = theParser->symbol;
		fxGetNextToken(theParser);
	}
	else if (declareIt)
		fxReportParserError(theParser, "missing identifier");
	
	aFunctionLabel = fxNewLabel(theParser, C_NULL);
	fxAppendReference(theParser, theList, XS_FUNCTION, aFunctionLabel);
	fxArgumentBlock(theParser, theList);
	fxMatchToken(theParser, XS_TOKEN_LEFT_BRACE);
	fxBody(theParser, theList, aSymbol, XS_NO_FLAG);
	theParser->strict = aStrictFlag;
	fxMatchToken(theParser, XS_TOKEN_RIGHT_BRACE);
	fxAppendLabel(theParser, theList, aFunctionLabel);
	
	theParser->parameters = aParametersFlag;
	theParser->functions = functions;
	theParser->variables = variables;
	theParser->arguments = arguments;
	theParser->throwLabel = aThrowLabel;
	theParser->returnLabel = aReturnLabel;
	theParser->continueLabel = aContinueLabel;
	theParser->breakLabel = aBreakLabel;
	
	if (declareIt && aSymbol) {
		fxNewLocal(theParser, aSymbol, theParser->variables);
		fxAppendReference(theParser, theList, XS_SET, aSymbol);
		fxAppendCode(theParser, theList, XS_POP);
	}
	theParser->the->stack = aStack;
}

void fxNewExpression(txScriptParser* theParser, txSlot* theList)
{
	txSlot* aStack = theParser->the->stack;
	txSlot* aSlot;
	txSlot* aList;
	txSlot* aCall;

	aSlot = theList->value.list.last;
	fxLiteralExpression(theParser, theList);
	for (;;) {
		if (theParser->token == XS_TOKEN_DOT) {
			fxGetNextToken(theParser);
			if (theParser->token == XS_TOKEN_IDENTIFIER) {
				fxAppendReference(theParser, theList, XS_GET_MEMBER, (txSlot*)(theParser->symbol));
				fxGetNextToken(theParser);
			}
			else
				fxReportParserError(theParser, "missing property");
		}
		else if (theParser->token == XS_TOKEN_LEFT_BRACKET) {
			fxGetNextToken(theParser);
			fxCommaExpression(theParser, theList);
			fxMatchToken(theParser, XS_TOKEN_RIGHT_BRACKET);
			fxAppendCode(theParser, theList, XS_GET_MEMBER_AT);
		}
		else
			break;
	} 
	aCall = theList->value.list.last;
	aList = fxNewList(theParser);
	fxCutList(theParser, aList, theList, aSlot);
	if (theParser->token == XS_TOKEN_LEFT_PARENTHESIS)
		fxParameterBlock(theParser, theList);
	else
		fxAppendInteger(theParser, theList, 0);
	if (aCall) {
		if ((XS_GET <= aCall->kind) && (aCall->kind <= XS_GET_MEMBER_AT)) {
		}
		else {
			fxAppendCode(theParser, theList, XS_UNDEFINED);
		}
	}
	fxPasteList(theParser, theList, aList);
	fxAppendCode(theParser, theList, XS_NEW);
	theParser->the->stack = aStack;
}

void fxObjectExpression(txScriptParser* theParser, txSlot* theList)
{
	txSlot* aStack = theParser->the->stack;
	txSlot* aSymbol;
	txSlot* aUniqueList;
	txSlot* aUniqueSlot;
	txSlot* aList;
	txSlot* aSlot;
	txString aString;
	txFlag aFlag;
	char aBuffer[256];
	
	fxMatchToken(theParser, XS_TOKEN_LEFT_BRACE);
	fxAppendInteger(theParser, theList, 0);
	aSymbol = fxNewSymbolC(theParser->the, "Object");
	fxAppendReference(theParser, theList, XS_GET, aSymbol);
	fxAppendCode(theParser, theList, XS_NEW);
	aUniqueList = fxNewList(theParser);
	for (;;) {
		aList = fxNewList(theParser);
		aSlot = C_NULL;
		aString = C_NULL;
		aFlag = XS_NO_FLAG;
		if (theParser->token == XS_TOKEN_INTEGER) {
			fxAppendInteger(theParser, aList, theParser->integer);
			aSlot = fxAppendCode(theParser, aList, XS_PUT_MEMBER_AT);
			fxCopyStringC(theParser->the, theParser->string, fxIntegerToString(theParser->integer, aBuffer, sizeof(aBuffer)));
			aString = theParser->string->value.string;
		}
		else if (theParser->token == XS_TOKEN_NUMBER)  {
			fxAppendNumber(theParser, aList, theParser->number);
			aSlot = fxAppendCode(theParser, aList, XS_PUT_MEMBER_AT);
			fxCopyStringC(theParser->the, theParser->string, fxNumberToString(theParser->the, theParser->number, aBuffer, sizeof(aBuffer), 0, 0));
			aString = theParser->string->value.string;
		}
		else if (theParser->token == XS_TOKEN_STRING) {
			fxAppendString(theParser, aList, theParser->string->value.string);
			aSlot = fxAppendCode(theParser, aList, XS_PUT_MEMBER_AT);
			aString = theParser->string->value.string;
		}
		else if (theParser->token == XS_TOKEN_IDENTIFIER) {
			aSymbol = theParser->symbol;
			if (theParser->token2 == XS_TOKEN_COLON) {
				aSlot = fxAppendReference(theParser, aList, XS_PUT_MEMBER, aSymbol);
				aString = aSymbol->value.symbol.string;
			}
			else if (c_strcmp(aSymbol->value.symbol.string, "get") == 0)
				aFlag = XS_GETTER_FLAG;
			else if (c_strcmp(aSymbol->value.symbol.string, "set") == 0)
				aFlag = XS_SETTER_FLAG;
			if (aFlag) {
				fxGetNextToken(theParser);
				if (theParser->token2 != XS_TOKEN_LEFT_PARENTHESIS)
					fxReportParserError(theParser, "missing %s", gxTokenNames[XS_TOKEN_LEFT_PARENTHESIS]);
				if (theParser->token == XS_TOKEN_INTEGER) {
					fxAppendInteger(theParser, aList, theParser->integer);
					aSlot = fxAppendCode(theParser, aList, XS_PUT_MEMBER_AT);
					fxCopyStringC(theParser->the, theParser->string, fxIntegerToString(theParser->integer, aBuffer, sizeof(aBuffer)));
					aString = theParser->string->value.string;
				}
				else if (theParser->token == XS_TOKEN_NUMBER)  {
					fxAppendNumber(theParser, aList, theParser->number);
					aSlot = fxAppendCode(theParser, aList, XS_PUT_MEMBER_AT);
					fxCopyStringC(theParser->the, theParser->string, fxNumberToString(theParser->the, theParser->number, aBuffer, sizeof(aBuffer), 0, 0));
					aString = theParser->string->value.string;
				}
				else if (theParser->token == XS_TOKEN_STRING) {
					fxAppendString(theParser, aList, theParser->string->value.string);
					aSlot = fxAppendCode(theParser, aList, XS_PUT_MEMBER_AT);
					aString = theParser->string->value.string;
				}
				else if (theParser->token == XS_TOKEN_IDENTIFIER) {
					aSymbol = theParser->symbol;
					aSlot = fxAppendReference(theParser, aList, XS_PUT_MEMBER, aSymbol);
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
			aSlot->flag = aFlag;
			fxAppendCode(theParser, theList, XS_DUB);
			fxGetNextToken(theParser);
			if (aFlag) {
				fxFunctionExpression(theParser, theList, 0);
			}
			else {
				fxMatchToken(theParser, XS_TOKEN_COLON);
				fxAssignmentExpression(theParser, theList);
			
			}
			fxAppendCode(theParser, theList, XS_SWAP);
			fxPasteList(theParser, theList, aList);
			fxAppendCode(theParser, theList, XS_POP);
		}
		else 
			break;
		if (theParser->token == XS_TOKEN_RIGHT_BRACE)
			break;
		fxMatchToken(theParser, XS_TOKEN_COMMA);
	}
	fxMatchToken(theParser, XS_TOKEN_RIGHT_BRACE);
	theParser->the->stack = aStack;
}

void fxArrayExpression(txScriptParser* theParser, txSlot* theList)
{
	txSlot* aSymbol;
	int elision = 1;
	int anIndex = 0;

	fxMatchToken(theParser, XS_TOKEN_LEFT_BRACKET);
	fxAppendInteger(theParser, theList, 0);
	aSymbol = fxNewSymbolC(theParser->the, "Array");
	fxAppendReference(theParser, theList, XS_GET, aSymbol);
	fxAppendCode(theParser, theList, XS_NEW);
	while ((theParser->token == XS_TOKEN_COMMA) || (gxTokenFlags[theParser->token] & XS_TOKEN_BEGIN_EXPRESSION)) {
		if (theParser->token == XS_TOKEN_COMMA) {
			fxGetNextToken(theParser);
			if (!elision)
				elision = 1;
			else {
				fxAppendCode(theParser, theList, XS_DUB);
				fxAppendCode(theParser, theList, XS_UNDEFINED);
				fxAppendCode(theParser, theList, XS_SWAP);
				fxAppendInteger(theParser, theList, anIndex);
				fxAppendCode(theParser, theList, XS_SET_MEMBER_AT);
				fxAppendCode(theParser, theList, XS_POP);
				anIndex++;
			}
		}
		else {
			if (!elision)
				fxReportParserError(theParser, "missing ,");
			else
				elision = 0;
			fxAppendCode(theParser, theList, XS_DUB);
			fxAssignmentExpression(theParser, theList);
			fxAppendCode(theParser, theList, XS_SWAP);
			fxAppendInteger(theParser, theList, anIndex);
			fxAppendCode(theParser, theList, XS_SET_MEMBER_AT);
			fxAppendCode(theParser, theList, XS_POP);
			anIndex++;
		}
	}
	fxMatchToken(theParser, XS_TOKEN_RIGHT_BRACKET);
}

void fxParameterBlock(txScriptParser* theParser, txSlot* theList)
{
	txSlot* aStack = theParser->the->stack;
	int aCount;
	
	fxMatchToken(theParser, XS_TOKEN_LEFT_PARENTHESIS);
	aCount = 0;
	while (gxTokenFlags[theParser->token] & XS_TOKEN_BEGIN_EXPRESSION) {
		fxAssignmentExpression(theParser, theList);
		if (theParser->token != XS_TOKEN_RIGHT_PARENTHESIS)
			fxMatchToken(theParser, XS_TOKEN_COMMA);
		aCount++;
	}
	fxMatchToken(theParser, XS_TOKEN_RIGHT_PARENTHESIS);
	fxAppendInteger(theParser, theList, aCount);
	theParser->the->stack = aStack;
}

void fxArgumentBlock(txScriptParser* theParser, txSlot* theList)
{
	fxMatchToken(theParser, XS_TOKEN_LEFT_PARENTHESIS);
	while (theParser->token == XS_TOKEN_IDENTIFIER) {
		fxNewLocal(theParser, theParser->symbol, theParser->arguments);
		fxGetNextToken(theParser);
		if (theParser->token != XS_TOKEN_RIGHT_PARENTHESIS)
			fxMatchToken(theParser, XS_TOKEN_COMMA);
	}
	fxMatchToken(theParser, XS_TOKEN_RIGHT_PARENTHESIS);
}

void fxCheckNoArgumentsNoEval(txScriptParser* theParser, txSlot* theSlot)
{
	if ((XS_GET <= theSlot->kind) && (theSlot->kind <= XS_GET_MEMBER)) {
		if (theParser->strict) {
			if (theSlot->value.reference->ID == theParser->the->argumentsID)
				fxReportParserError(theParser, "set arguments (strict mode)");
			if (theSlot->value.reference->ID == theParser->the->evalID)
				fxReportParserError(theParser, "set eval (strict mode)");
		}
	}
}
