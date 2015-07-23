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
#include "xs6Script.h"

void fxInitializeParser(txParser* parser, void* console)
{
	c_memset(parser, 0, sizeof(txParser));
	parser->first = C_NULL;
	parser->console = console;
	
	parser->nodeArray = fxNewParserChunkClear(parser, 0x7FFF * sizeof(txNode*));
	parser->nodeLimit = parser->nodeArray + 0x7FFF;
	parser->nodePointer = parser->nodeArray;
	
	parser->dtoa = fxNew_dtoa();
	parser->symbolModulo = 1993;
	parser->symbolTable = fxNewParserChunkClear(parser, parser->symbolModulo * sizeof(txSymbol*));
	parser->symbolCount = 0x7FFF;
	parser->symbolIndex = 0;
	parser->symbolArray = fxNewParserChunkClear(parser, parser->symbolCount * sizeof(txSymbol*));

	parser->emptyString = fxNewParserString(parser, "", 0);

	parser->__dirnameSymbol = fxNewParserSymbol(parser, "__dirname");
	parser->__filenameSymbol = fxNewParserSymbol(parser, "__filename");
	parser->__proto__Symbol = fxNewParserSymbol(parser, "__proto__");
	parser->allSymbol = fxNewParserSymbol(parser, "*");
	parser->argsSymbol = fxNewParserSymbol(parser, "args");
	parser->argumentsSymbol = fxNewParserSymbol(parser, "arguments");
	parser->arrowSymbol = fxNewParserSymbol(parser, "=>");
	parser->asSymbol = fxNewParserSymbol(parser, "as");
	parser->constructorSymbol = fxNewParserSymbol(parser, "constructor");
	parser->defaultSymbol = fxNewParserSymbol(parser, "*default*");
	parser->doneSymbol = fxNewParserSymbol(parser, "done");
	parser->evalSymbol = fxNewParserSymbol(parser, "eval");
	parser->exportsSymbol = fxNewParserSymbol(parser, "exports");
	parser->fromSymbol = fxNewParserSymbol(parser, "from");
	parser->getSymbol = fxNewParserSymbol(parser, "get");
	parser->idSymbol = fxNewParserSymbol(parser, "id");
	parser->includeSymbol = fxNewParserSymbol(parser, "include");
	parser->lengthSymbol = fxNewParserSymbol(parser, "length");
	parser->letSymbol = fxNewParserSymbol(parser, "let");
	parser->moduleSymbol = fxNewParserSymbol(parser, "module");
	parser->nextSymbol = fxNewParserSymbol(parser, "next");
	parser->ofSymbol = fxNewParserSymbol(parser, "of");
	parser->prototypeSymbol = fxNewParserSymbol(parser, "prototype");
	parser->rawSymbol = fxNewParserSymbol(parser, "raw");
	parser->RegExpSymbol = fxNewParserSymbol(parser, "RegExp");
	parser->returnSymbol = fxNewParserSymbol(parser, "return");
	parser->setSymbol = fxNewParserSymbol(parser, "set");
	parser->sliceSymbol = fxNewParserSymbol(parser, "slice");
	parser->thisSymbol = fxNewParserSymbol(parser, "this");
	parser->targetSymbol = fxNewParserSymbol(parser, "target");
	parser->undefinedSymbol = fxNewParserSymbol(parser, "undefined");
	parser->uriSymbol = fxNewParserSymbol(parser, "uri");
	parser->valueSymbol = fxNewParserSymbol(parser, "value");
	parser->withSymbol = fxNewParserSymbol(parser, "with");
}

void* fxNewParserChunk(txParser* parser, txSize size)
{
	txParserChunk* block = c_malloc(sizeof(txParserChunk) + size);
	if (!block)
		fxThrowMemoryError(parser);
	block->next = parser->first;
	parser->first = block;
	return block + 1;
}

void* fxNewParserChunkClear(txParser* parser, txSize size)
{
	void* result = fxNewParserChunk(parser, size);
    c_memset(result, 0, size);
	return result;
}

txString fxNewParserString(txParser* parser, txString buffer, txSize size)
{
	txString result = fxNewParserChunk(parser, size + 1);
	c_memcpy(result, buffer, size);
	result[size] = 0;
	return result;
}

txSymbol* fxNewParserSymbol(txParser* parser, txString theString)
{
	txString aString;
	txSize aLength;
	txSize aSum;
	txSize aModulo;
	txSymbol* aSymbol;
	txID anID;
	
	aString = theString;
	aLength = 0;
	aSum = 0;
	while(*aString != 0) {
		aLength++;
		aSum = (aSum << 1) + *aString++;
	}
	aSum &= 0x7FFFFFFF;
	aModulo = aSum % parser->symbolModulo;
	aSymbol = parser->symbolTable[aModulo];
	while (aSymbol != C_NULL) {
		if (aSymbol->sum == aSum)
			if (c_strcmp(aSymbol->string, theString) == 0)
				break;
		aSymbol = aSymbol->next;
	}
	if (aSymbol == C_NULL) {
		anID = parser->symbolIndex;
		if (anID == parser->symbolCount) {
			exit(1);
		}
		aSymbol = fxNewParserChunk(parser, sizeof(txSymbol));
		aSymbol->next = parser->symbolTable[aModulo];
		aSymbol->ID = anID;
		aSymbol->length = aLength + 1;
		aSymbol->string = fxNewParserString(parser, theString, aLength);
		aSymbol->sum = aSum;
		aSymbol->usage = 0;
		parser->symbolArray[anID] = aSymbol;
		parser->symbolTable[aModulo] = aSymbol;
		parser->symbolIndex++;
	}
	return aSymbol;
}

void fxReportParserError(txParser* parser, txString theFormat, ...)
{
	c_va_list arguments;
	parser->errorCount++;
	c_va_start(arguments, theFormat);
    fxVReportError(parser->console, parser->path ? parser->path->string : C_NULL, parser->line, theFormat, arguments);
	c_va_end(arguments);
}

void fxReportParserWarning(txParser* parser, txString theFormat, ...)
{
	c_va_list arguments;
	parser->warningCount++;
	c_va_start(arguments, theFormat);
	fxVReportWarning(parser->console, parser->path ? parser->path->string : C_NULL, parser->line, theFormat, arguments);
	c_va_end(arguments);
}

void fxReportLineError(txParser* parser, txInteger line, txString theFormat, ...)
{
	c_va_list arguments;
	parser->errorCount++;
	c_va_start(arguments, theFormat);
	fxVReportError(parser->console, parser->path ? parser->path->string : C_NULL, line, theFormat, arguments);
	c_va_end(arguments);
}

void fxTerminateParser(txParser* parser)
{
	txParserChunk* block = parser->first;
	while (block) {
		txParserChunk* next = block->next;
		c_free(block);
		block = next;
	}
	if (parser->dtoa)
		fxDelete_dtoa(parser->dtoa);
}

void fxThrowMemoryError(txParser* parser)
{
	parser->error = C_ENOMEM;
	c_longjmp(parser->firstJump->jmp_buf, 1);
}

void fxThrowParserError(txParser* parser, txInteger count)
{
	parser->error = C_EINVAL;
	c_longjmp(parser->firstJump->jmp_buf, 1);
}













