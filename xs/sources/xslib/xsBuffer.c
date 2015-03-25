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

#include "xsCompiler.h"

static void xscBuildSymbol(txMachine* the, xsSymbolsData* data, txSlot* aSymbol, txID anID, txSize aModulo);
static void xscBuildSymbolsCode(txMachine* the, xsSymbolsData* theData, txByte* theCode);
static void xscBuildSymbolsID(txMachine* the, xsSymbolsData* theData, txByte* theCode);
static void xscGrowBuffer(txMachine* the, xsBuffer*  theBuffer, txSize theSize);

void xscAppend(txMachine* the, txByte* theData, txSize theSize)
{
	xsBuffer* aBuffer = xsGetContext(the);
	xscGrowBuffer(the, aBuffer, theSize);
	memmove(aBuffer->current, theData, theSize);
	aBuffer->current += theSize;
}

void xscAppend1(txMachine* the, txByte theValue)
{
	xsBuffer* aBuffer = xsGetContext(the);
	xscGrowBuffer(the, aBuffer, 1);
	*(aBuffer->current) = theValue;
	aBuffer->current++;
}

void xscAppend2(txMachine* the, txID theValue)
{
	xsBuffer* aBuffer = xsGetContext(the);
	xscGrowBuffer(the, aBuffer, 2);
	mxEncode2(aBuffer->current, theValue);
}

void xscAppend4(txMachine* the, txInteger theValue)
{
	xsBuffer* aBuffer = xsGetContext(the);
	xscGrowBuffer(the, aBuffer, 4);
	mxEncode4(aBuffer->current, theValue);
}

void xscAppend8(txMachine* the, txNumber theValue)
{
	xsBuffer* aBuffer = xsGetContext(the);
	xscGrowBuffer(the, aBuffer, 8);
	mxEncode8(aBuffer->current, theValue);
}

void xscAppendFunction(txMachine* the)
{
	txInteger aLength;
	
	aLength = xsToInteger(xsGet(xsResult, xsID("size")));
	if (aLength < 0x00007FFF) {
		xscAppend1(the, XS_FUNCTION);
		xscAppend2(the, (txID)aLength);
	}
	else {
		xscAppend1(the, XS_FUNCTION2);
		xscAppend4(the, aLength);
	}
	xsResult = xsGet(xsResult, xsID("code"));
	xscAppend(the, xsResult.value.code, aLength);
}

void xscAppendID(txMachine* the, txByte theCode, txID theID)
{
	xscAppend1(the, theCode);
	xscAppend2(the, theID);
}

void xscAppendInteger(txMachine* the, txInteger theInteger)
{
	if ((-128 <= theInteger) && (theInteger <= 127)) {
		xscAppend1(the, XS_INTEGER_8);
		xscAppend1(the, (txByte)theInteger);
	}
	else if ((-32768 <= theInteger) && (theInteger <= 32767)) {
		xscAppend1(the, XS_INTEGER_16);
		xscAppend2(the, (txID)theInteger);
	}
	else {
		xscAppend1(the, XS_INTEGER_32);
		xscAppend4(the, theInteger);
	}
}

void xscAppendPattern(txMachine* the, txByte theCode)
{
	xsIntegerValue c, i;

	xscAppend1(the, theCode);
	c = xsToInteger(xsGet(xsVar(1), the->lengthID));
	xscAppend1(the, (txByte)c);
	for (i = 0; i < c; i++) {
		xsVar(2) = xsGet(xsVar(1), (xsIndex)i);
		xsVar(3) = xsGet(xsVar(2), xsID("namespace"));
		if (xsTest(xsVar(3)))
			xscAppend2(the, xsToID(xsVar(3)));
		else
			xscAppend2(the, XS_NO_ID);
		xsVar(3) = xsGet(xsVar(2), xsID("name"));
		if (xsTest(xsVar(3)))
			xscAppend2(the, xsToID(xsVar(3)));
		else
			xscAppend2(the, XS_NO_ID);
	}
}

void xscAppendString(txMachine* the, txString theString)
{
		txID aLength = strlen(theString) + 1;
		xsBuffer* aBuffer = xsGetContext(the);
		xscGrowBuffer(the, aBuffer, aLength+1+stringCommandOffset);
		*(aBuffer->current) = XS_STRING;
		aBuffer->current++;
#if mxStringLength
		mxEncode2(aBuffer->current, aLength);
#endif
		memmove(aBuffer->current, theString, aLength);
		aBuffer->current += aLength;
}

void xscBuildSymbol(txMachine* the, xsSymbolsData* data, txSlot* aSymbol, txID anID, txSize aModulo)
{
	txSlot* result = c_malloc(sizeof(txSlot));
	xsElseError(result);
	result->next = data->symbolTable[aModulo];
	result->flag = XS_NO_FLAG;
	result->kind = XS_SYMBOL_KIND;
	result->ID = 0x8000 | anID;
	result->value.symbol.sum = aSymbol->value.symbol.sum;
	result->value.symbol.string = aSymbol->value.symbol.string;
	data->symbolArray[anID] = result;
	data->symbolTable[aModulo] = result;
}

void xscBuildSymbols(txMachine* the)
{
	xsBuffer* aBuffer = xsGetContext(the);
	xsSymbolsData* data = NULL;
	FILE* aFile = NULL;
	txInteger aSize;
	txString aString = NULL;
	txID aCount;
	txID anID;
	txSlot* aSymbol;
	txSize aModulo;
	txSlot* result;
	txID anIndex;
	txByte* aCode;
	txInteger aLength;
	txBoolean failed;
	static char* prefix = "#define xsID_";
	static char* suffix = " (the->code[";
	txInteger prefixLength = c_strlen(prefix);
	txInteger suffixLength = c_strlen(suffix);
	
	mxTry(the) {
		data = c_calloc(1, sizeof(xsSymbolsData));
		xsElseError(data);

		xsVar(1) = xsCat2(xsVar(0), xsString(".h"));
		aFile = fopen(xsToString(xsVar(1)), "rb");
		if (aFile) {
			fseek(aFile, 0, SEEK_END);
			aSize = ftell(aFile);
			rewind(aFile);
			aString = c_malloc(aSize + 1);
			xsElseError(aString);
			fread(aString, 1, aSize, aFile);
			aString[aSize] = 0;
			fclose(aFile);
			aFile = C_NULL;
		}
		
		aCount = 0;
		if (aString) {
			txString p = c_strstr(aString, prefix), q;
			while (p) {
				p += prefixLength;
				q = c_strstr(p, suffix);
				q += suffixLength;
				anID = c_strtol(q, &p, 10);
				if (aCount <= anID)
					aCount = anID + 1;
				p = c_strstr(p, prefix);
			}
		}
		if (aCount < the->symbolCount)
			aCount = the->symbolCount;
			
		data->symbolCount = aCount;
		data->symbolIndex = 0;
		data->symbolArray = c_calloc(aCount, sizeof(txSlot*));
		xsElseError(data->symbolArray);
		data->symbolModulo = the->symbolModulo;
		data->symbolTable = c_calloc(the->symbolModulo, sizeof(txSlot*));
		xsElseError(data->symbolTable);
	
		if (aString) {
			txString p = c_strstr(aString, prefix), q;
			while (p) {
				p += prefixLength;
				q = c_strstr(p, suffix);
				*q = 0;
				aSymbol = fxNewSymbolC(the, p);
				aModulo = aSymbol->value.symbol.sum % data->symbolModulo;
				q += suffixLength;
				anID = c_strtol(q, &p, 10);
				xscBuildSymbol(the, data, aSymbol, anID, aModulo);
				data->symbolIndex = anID + 1;
				p = c_strstr(p, prefix);
			}
			c_free(aString);
			aString = C_NULL;
		}
			
		aCount = the->symbolIndex;
		anID = 0;
		for (anIndex = 0; anIndex < aCount; anIndex++) {
			aSymbol = the->symbolArray[anIndex];
			if (aSymbol->flag & XS_TO_C_FLAG) {
				aModulo = aSymbol->value.symbol.sum % data->symbolModulo;
				result = data->symbolTable[aModulo];
				while (result != C_NULL) {
					if (result->value.symbol.sum == aSymbol->value.symbol.sum)
						#if mxCmpStr
						if (result->value.symbol.string == aSymbol->value.symbol.string)
						#endif	
							break;
					result = result->next;
				}
				if (result == C_NULL) {
					while (data->symbolArray[anID])
						anID++;
					xscBuildSymbol(the, data, aSymbol, anID, aModulo);
					if (data->symbolIndex <= anID)
						data->symbolIndex = anID + 1;
				}
			}
		}
		
		aCount = data->symbolIndex;
		anIndex = 0;
		while (anID < aCount) {
			while ((anID < aCount) && data->symbolArray[anID])
				anID++;
			if (anID < aCount) {
				char aName[16];
				sprintf(aName, "@%d", anIndex);
				aSymbol = fxNewSymbolC(the, aName);
				aModulo = aSymbol->value.symbol.sum % data->symbolModulo;
				xscBuildSymbol(the, data, aSymbol, anID, aModulo);
			}
		}
		
		aCode = aBuffer->bottom;
		xscBuildSymbolsCode(the, data, aCode);
		aCount = data->symbolIndex;
		
		aSize = 2;
		for (anIndex = 0; anIndex < aCount; anIndex++)
			aSize += strlen(data->symbolArray[anIndex]->value.symbol.string) + symbolEncodeLength + 1;
		aBuffer->symbols = c_malloc(aSize);
		xsElseError(aBuffer->symbols);
		aBuffer->symbolsSize = aSize;
		
		aCode = aBuffer->symbols;
		mxEncode2(aCode, aCount);
		for (anIndex = 0; anIndex < aCount; anIndex++) {
			aString = data->symbolArray[anIndex]->value.symbol.string;
			aLength = strlen(aString) + 1;
			memcpy(aCode, aString, aLength);
			aCode += aLength;
#if mxUseApHash
			mxEncode4(aCode,data->symbolArray[anIndex]->value.symbol.sum);
#endif
		}
		aString = C_NULL;
		failed = 0;
	}
	mxCatch(the) {
		failed = 1;
	}	
	if (data) {
		if (data->symbolTable)
			c_free(data->symbolTable);
		if (data->symbolArray) {
			aCount = data->symbolIndex;
			for (anIndex = 0; anIndex < aCount; anIndex++)
				c_free(data->symbolArray[anIndex]);
			c_free(data->symbolArray);
		}
		if (aString)
			c_free(aString);
		if (aFile)
			fclose(aFile);
		c_free(data);
	}
	if (failed)
		xsThrow(xsException);
}

#if 0
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
	/* XS_FLAG_INSTANCE */ "flag instance"
};
#endif

void xscBuildSymbolsCode(txMachine* the, xsSymbolsData* theData, txByte* theCode)
{
	txID aCount;

	for (;;) {
// 		if (((XS_LABEL <= *((txU1*)theCode)) && (*((txU1*)theCode) < XS_COUNT)))
// 			fprintf(stderr, "%s\n", gxCodeNames[*((txU1*)theCode)]);
// 		else
// 			fprintf(stderr, "%d\n", *((txU1*)theCode));
		switch (*((txU1*)theCode++)) {
		case XS_LABEL:
			return;
		case XS_BEGIN:
			xscBuildSymbolsID(the, theData, theCode);
			theCode += 3;
			aCount = *theCode++;
			while (aCount) {
				xscBuildSymbolsID(the, theData, theCode);
				theCode += 2;
				aCount--;
			}
			aCount = *theCode++;
			while (aCount) {
				xscBuildSymbolsID(the, theData, theCode);
				theCode += 2;
				aCount--;
			}
			break;
		case PSEUDO_INCREMENT:
		case PSEUDO_DECREMENT:
		case XS_DELETE_AT:
		case XS_GET_AT:
		case PSEUDO_BRANCH_ELSE0:
		case PSEUDO_BRANCH_ELSE1:
		case PSEUDO_LESS:
		case PSEUDO_LESS_EQUAL:
		case PSEUDO_DOUBLE_GET_NEGATIVE:
		case PSEUDO_DOUBLE_GET_AT:
		case XS_GET_NEGATIVE_ID:
		case XS_INTEGER_8:
		case XS_SET_AT:
		case XS_SET_NEGATIVE_ID:
			theCode++;
			break;
		case XS_CATCH:
		case XS_DELETE:
		case XS_DELETE_MEMBER:
		case XS_FILE:
		case XS_GET:
		case XS_GET_FOR_NEW:
		case XS_GET_MEMBER:
		case XS_GET_MEMBER_FOR_CALL:
		case XS_SET:
		case XS_SET_MEMBER:
			xscBuildSymbolsID(the, theData, theCode);
			theCode += 2;
			break;
		case XS_PUT_MEMBER:
		case PSEUDO_PUT_MEMBER:
			xscBuildSymbolsID(the, theData, theCode);
			theCode += 4;
			break;
		case XS_PUT_MEMBER_AT:
			theCode += 3;
			break;
		case XS_BRANCH:
		case XS_BRANCH_ELSE:
		case XS_BRANCH_IF:
		case XS_BRANCH_ELSE_BOOL:
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
		case XS_BRANCH_IF2:
		case XS_BRANCH_ELSE_BOOL2:
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
			len = c_strlen((const char *)theCode) + 1;
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
				xscBuildSymbolsID(the, theData, theCode);
				theCode += 2;
				xscBuildSymbolsID(the, theData, theCode);
				theCode += 2;
				aCount--;
			}
			break;
		default:
			xsDebugger();
			break;
		}
	}
}

void xscBuildSymbolsID(txMachine* the, xsSymbolsData* theData, txByte* theCode)
{
	txID anIndex;
	txSlot* aSymbol;
	txSize aModulo;
	txSlot* result;
	
	mxDecode2(theCode, anIndex);
	if ((aSymbol = fxGetSymbol(the, anIndex))) {
		aModulo = aSymbol->value.symbol.sum % theData->symbolModulo;
		result = theData->symbolTable[aModulo];
		while (result != C_NULL) {
			if (result->value.symbol.sum == aSymbol->value.symbol.sum)
				if (result->value.symbol.string == aSymbol->value.symbol.string)
					break;
			result = result->next;
		}
		if (result == C_NULL) {
			anIndex = theData->symbolIndex;
			result = c_malloc(sizeof(txSlot));
			xsElseError(result);
			result->next = theData->symbolTable[aModulo];
			result->flag = XS_NO_FLAG;
			result->kind = XS_SYMBOL_KIND;
			result->ID = 0x8000 | anIndex;
			result->value.symbol.sum = aSymbol->value.symbol.sum;
			result->value.symbol.string = aSymbol->value.symbol.string;
			theData->symbolArray[anIndex] = result;
			theData->symbolTable[aModulo] = result;
			theData->symbolIndex++;
		}
		theCode -= 2; 
		mxEncode2(theCode, result->ID);
	}
}

void fxCatenate(txMachine* the)
{
	txInteger aCount, anIndex;
	
	aCount = the->stack->value.integer;
	the->stack++;
	fxToString(the, the->stack);
	for (anIndex = 1; anIndex < aCount; anIndex++) {
		fxToString(the, the->stack + anIndex);
		fxConcatString(the, the->stack, the->stack + anIndex);
	}
	aCount--;
	(the->stack + aCount)->value = (the->stack)->value;
	the->stack += aCount;
}

void xscCreateBuffer(txMachine* the)
{
	xsBuffer* aBuffer;
	
	aBuffer = c_calloc(1, sizeof(xsBuffer));
	xsElseError(aBuffer);
	aBuffer->bottom = c_malloc(1024);
	xsElseError(aBuffer->bottom);
	aBuffer->current = aBuffer->bottom;
	aBuffer->top = aBuffer->bottom + 1024;
	aBuffer->symbols = NULL;
	aBuffer->symbolsSize = 0;
	xsSetContext(the, aBuffer);
}

void xscDeleteBuffer(txMachine* the)
{
	xsBuffer* aBuffer = xsGetContext(the);
	
	if (aBuffer) {
		if (aBuffer->bottom)
			c_free(aBuffer->bottom);
		if (aBuffer->symbols)
			c_free(aBuffer->symbols);
		c_free(aBuffer);
		xsSetContext(the, NULL);
	}
}

void xscGrowBuffer(txMachine* the, xsBuffer* theBuffer, txSize theSize)
{
	txInteger anOffset, aSize;

	if (theBuffer->current + theSize > theBuffer->top) {
		anOffset = theBuffer->current - theBuffer->bottom;
		aSize = (((anOffset + theSize) / 1024) + 1) * 1024;
		theBuffer->bottom = c_realloc(theBuffer->bottom, aSize);
		xsElseError(theBuffer->bottom);
		theBuffer->top = theBuffer->bottom + aSize;
		theBuffer->current = theBuffer->bottom + anOffset;
	}
}

void fxQueuePattern(txMachine* the, txKind theKind, 
		txID theNamespaceID, txID theNameID, 
		txID theID, txFlag theFlag)
{
	txPatternData* aPatternData;

	aPatternData = fxNewChunk(the, sizeof(txPatternData));
	aPatternData->alias = the->stack->value.alias;
	aPatternData->count = 1;
	aPatternData->parts[0].namespaceID = theNamespaceID;
	aPatternData->parts[0].nameID = theNameID;
	the->stack->value.pattern = aPatternData;
	the->stack->kind = theKind;
	if (theID == XS_SAME_ID)
		fxQueueID(the, theNameID, theFlag);
	else
		fxQueueID(the, theID, theFlag);
}

txID fxToID(txMachine* the)
{
	txSlot* aSymbol;

	fxToString(the, the->stack);
	aSymbol = fxNewSymbol(the, the->stack);
	the->stack++;
	return aSymbol->ID;
}
