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

#define mxValueSize 1024
typedef struct sxMarkupParser txMarkupParser;
struct sxMarkupParser {
	txMachine* the;
	void* stream;
	int (*streamGetter)(void*);
	txSlot* path;
	txInteger line;
	txMarkupCallbacks* callbacks;
	int c;
	int c0;
	int c1;
	int (*getter)(txMarkupParser*);
	int depth;
	char* entity;
	char name[1024];
	char value[mxValueSize];
};

typedef struct {
	char name[9];
	char nameLength;
	char value[5];
	char valueLength;
} txEntity;

static void fxGetCharacter(txMarkupParser* theParser);
static int fxGetFirstEntityCharacter(txMarkupParser* theParser);
static int fxGetNextCommentCharacter(txMarkupParser* theParser);
static int fxGetNextDataCharacter(txMarkupParser* theParser);
static int fxGetNextDocTypeCharacter(txMarkupParser* theParser);
static int fxGetNextEntityCharacter(txMarkupParser* theParser);
static int fxGetNextTextCharacter(txMarkupParser* theParser);
static int fxGetNextProcessingInstructionCharacter(txMarkupParser* theParser);
static int fxGetNextScriptCharacter(txMarkupParser* theParser);
static int fxGetNextValueCharacter(txMarkupParser* theParser);
static void fxParseComment(txMarkupParser* theParser);
static void fxParseDocument(txMarkupParser* theParser);
static void fxParseDocType(txMarkupParser* theParser);
static void fxParseName(txMarkupParser* theParser);
static void fxParseProcessingInstruction(txMarkupParser* theParser);
static void fxParseStartTag(txMarkupParser* theParser);
static void fxParseStopTag(txMarkupParser* theParser);
static void fxParseText(txMarkupParser* theParser);
static void fxParseValue(txMarkupParser* theParser);
static void fxPopArgument(txMachine* the);
static void fxPushArgument(txMachine* the);
static void fxReportMarkupError(txMarkupParser* theParser, char* theFormat, ...);
static int fxScanName(txMarkupParser* theParser, char* theName, size_t theSize);
static int fxScanSpace(txMarkupParser* theParser);
static txEntity* fxSearchEntity(txString theName);
static void fxSkipComment(txMarkupParser* theParser);
static void fxSkipDeclaration(txMarkupParser* theParser);
static void fxSkipDocType(txMarkupParser* theParser);
static void fxSkipProcessingInstruction(txMarkupParser* theParser);

void fxParseMarkup(txMachine* the, void* theStream, txGetter theGetter, 
		txSlot* thePath, long theLine, txMarkupCallbacks* theCallbacks)
{
	txMarkupParser aParser;

	c_memset(&aParser, 0, sizeof(txMarkupParser));
	aParser.the = the;
	aParser.stream = theStream;
	aParser.streamGetter = theGetter;
	aParser.path = thePath;
	aParser.line = theLine;
	if (thePath) {
		mxArgv(1)->value.string = thePath->value.symbol.string;
		mxArgv(1)->kind = XS_STRING_KIND;
	}
	mxArgv(2)->value.integer = theLine;
	mxArgv(2)->kind = XS_INTEGER_KIND;
	aParser.callbacks = theCallbacks;
	fxParseDocument(&aParser);
}

void fxParseMarkupBuffer(txMachine* the, void* theBuffer, txSize theSize, 
		txSlot* thePath, long theLine, txMarkupCallbacks* theCallbacks)
{
	txStringCStream aStream;

	aStream.buffer = theBuffer;
	aStream.offset = 0;
	aStream.size = theSize;
	fxParseMarkup(the, &aStream, fxStringCGetter, thePath, theLine, theCallbacks);
}

void fxGetCharacter(txMarkupParser* theParser)
{
	txU4 aResult;
	txUTF8Sequence *aSequence;
	txInteger aSize;

	aResult = (txU4)(*(theParser->streamGetter))(theParser->stream);
	if (aResult != (txU4)C_EOF) {
		for (aSequence = gxUTF8Sequences; aSequence->size; aSequence++) {
			if ((aResult & aSequence->cmask) == aSequence->cval)
				break;
		}
		if (aSequence->size == 0)
			fxReportMarkupError(theParser, "invalid UTF-8 character");
		aSize = aSequence->size - 1;
		while (aSize > 0) {
			aSize--;
			aResult = (aResult << 6) | ((*(theParser->streamGetter))(theParser->stream) & 0x3F);
		}
		aResult &= aSequence->lmask;
	}
	theParser->c = aResult;
}

int fxGetFirstEntityCharacter(txMarkupParser* theParser)
{
	int c;
	char* p;
	txEntity* anEntity;
	
	fxGetCharacter(theParser);
	if (theParser->c == '#') {
		fxGetCharacter(theParser);
		if (theParser->c == 'x') {
			fxGetCharacter(theParser);
			p = theParser->name;
			while ((('0' <= theParser->c) && (theParser->c <= '9')) 
					|| (('A' <= theParser->c) && (theParser->c <= 'F'))
					|| (('a' <= theParser->c) && (theParser->c <= 'f'))) {
				*p++ = theParser->c;
				fxGetCharacter(theParser);
			}
			*p = 0;
			c = c_strtoul(theParser->name, C_NULL, 16);
		}
		else {
			p = theParser->name;
			while (('0' <= theParser->c) && (theParser->c <= '9'))  {
				*p++ = theParser->c;
				fxGetCharacter(theParser);
			}
			*p = 0;
			c = c_strtoul(theParser->name, C_NULL, 10);
		}
	}
	else {
		fxScanName(theParser, theParser->name, sizeof(theParser->name));
		anEntity = fxSearchEntity(theParser->name);
		if (anEntity) {
			c_strcpy(theParser->name, anEntity->value);
			theParser->entity = theParser->name;
			c = fxGetNextEntityCharacter(theParser);
		}
		else {
			fxReportMarkupError(theParser, "entity not found: %s", theParser->name);
			c = 0;
		}
	}
	if (theParser->c == ';')
		fxGetCharacter(theParser);
	else
		fxReportMarkupError(theParser, "missing ;");
	return c;
}

int fxGetNextCommentCharacter(txMarkupParser* theParser)
{
	int c;
	if ((theParser->c0 == '-') && (theParser->c1 == '-') && (theParser->c == '>'))
		c = C_EOF;
	else {
		c = theParser->c0;
		theParser->c0 = theParser->c1;
		theParser->c1 = theParser->c;
		fxGetCharacter(theParser);
		if ((c == 10) || ((c == 13) && (theParser->c0 != 10)))
			theParser->line++;
	}
	return c;
}

int fxGetNextDataCharacter(txMarkupParser* theParser)
{
	int c;
	if ((theParser->c0 == ']') && (theParser->c1 == ']') && (theParser->c == '>'))
		c = C_EOF;
	else {
		c = theParser->c0;
		theParser->c0 = theParser->c1;
		theParser->c1 = theParser->c;
		fxGetCharacter(theParser);
		if ((c == 10) || ((c == 13) && (theParser->c0 != 10)))
			theParser->line++;
	}
	return c;
}

int fxGetNextDocTypeCharacter(txMarkupParser* theParser)
{
	int c;
	if ((theParser->c == '[') || (theParser->c == '>'))
		c = C_EOF;
	else {
		c = theParser->c;
		fxGetCharacter(theParser);
		if ((c == 10) || ((c == 13) && (theParser->c != 10)))
			theParser->line++;
	}
	return c;
}

int fxGetNextProcessingInstructionCharacter(txMarkupParser* theParser)
{
	int c;
	if ((theParser->c0 == '?') && (theParser->c == '>'))
		c = C_EOF;
	else {
		c = theParser->c0;
		theParser->c0 = theParser->c;
		fxGetCharacter(theParser);
		if ((c == 10) || ((c == 13) && (theParser->c0 != 10)))
			theParser->line++;
	}
	return c;
}

int fxGetNextEntityCharacter(txMarkupParser* theParser)
{
	txU1* p;
	txU4 c;
	txUTF8Sequence *aSequence;
	txInteger aSize;
	
	p = (txU1*)(theParser->entity);
	c = (txU4)*p++;
	for (aSequence = gxUTF8Sequences; aSequence->size; aSequence++) {
		if ((c & aSequence->cmask) == aSequence->cval)
			break;
	}
	aSize = aSequence->size - 1;
	while (aSize) {
		aSize--;
		c = (c << 6) | (txU4)*p++;
	}
	c &= aSequence->lmask;
	if (*p)
		theParser->entity = (txString)p;
	else
		theParser->entity = C_NULL;
	return (int)c;
}

int fxGetNextScriptCharacter(txMarkupParser* theParser)
{
	int c;
	if ((theParser->c0 == '<') && (theParser->c == '/'))
		c = C_EOF;
	else {
		c = theParser->c0;
		theParser->c0 = theParser->c;
		fxGetCharacter(theParser);
		if ((c == 10) || ((c == 13) && (theParser->c0 != 10)))
			theParser->line++;
	}
	return c;
}

int fxGetNextTextCharacter(txMarkupParser* theParser)
{
	int c;
	if (theParser->entity)
		c = fxGetNextEntityCharacter(theParser);
	else if (theParser->c == '&')
		c = fxGetFirstEntityCharacter(theParser);
	else if (theParser->c == '<')
		c = C_EOF;
	else {
		c = theParser->c;
		fxGetCharacter(theParser);
		if ((c == 10) || ((c == 13) && (theParser->c != 10)))
			theParser->line++;
	}
	return c;
}

int fxGetNextValueCharacter(txMarkupParser* theParser)
{
	int c;
	if (theParser->entity)
		c = fxGetNextEntityCharacter(theParser);
	else if (theParser->c == '&')
		c = fxGetFirstEntityCharacter(theParser);
	else if (theParser->c == theParser->c0)
		c = C_EOF;
	else {
		c = theParser->c;
		fxGetCharacter(theParser);
		if ((c == 10) || ((c == 13) && (theParser->c != 10)))
			theParser->line++;
	}
	return c;
}

void fxParseComment(txMarkupParser* theParser)
{
	if (theParser->callbacks->processComment) {
		txMachine* the = theParser->the;

		theParser->c0 = theParser->c;
		fxGetCharacter(theParser);
		if ((theParser->c == 10) || ((theParser->c == 13) && (theParser->c0 != 10)))
			theParser->line++;
		theParser->c1 = theParser->c;
		fxGetCharacter(theParser);
		if ((theParser->c == 10) || ((theParser->c == 13) && (theParser->c0 != 10)))
			theParser->line++;
		theParser->getter = fxGetNextCommentCharacter;

		mxArgv(2)->value.integer = theParser->line;
		if (!mxIsReference(mxArgv(0)))
			fxReportMarkupError(theParser, "invalid data");
		fxNewInstance(the);
		*(--the->stack) = *mxArgv(0);
		fxQueueID(the, the->parentID, XS_GET_ONLY);
		fxParseValue(theParser);
		fxQueueID(the, the->valueID, XS_GET_ONLY);
		fxPushArgument(the);
		(*(theParser->callbacks->processComment))(the);
		fxPopArgument(the);
		if (theParser->c != '>')
			fxReportMarkupError(theParser, "missing -->");
		fxGetCharacter(theParser);
	}
	else 
		fxSkipComment(theParser);
}

void fxParseDocument(txMarkupParser* theParser)
{
	txMachine* the = theParser->the;

	fxGetCharacter(theParser);
	for (;;) {
		if (theParser->depth == 0)
			fxScanSpace(theParser);
		if (theParser->c == C_EOF)
			break;
		else if (theParser->c == '<') {
			fxGetCharacter(theParser);
			if (theParser->c == '/') {
				fxGetCharacter(theParser);
				fxParseStopTag(theParser);
			}
			else if (theParser->c == '?') {
				fxGetCharacter(theParser);
				fxParseProcessingInstruction(theParser);
			}
			else if (theParser->c == '!') {
				fxGetCharacter(theParser);
				if (theParser->c == '-') {
					fxGetCharacter(theParser);
					if (theParser->c != '-')
						fxReportMarkupError(theParser, "missing -");
					fxGetCharacter(theParser);
					fxParseComment(theParser);
				}
				else if (theParser->c == '[') {
					fxGetCharacter(theParser);
					if (theParser->c != 'C')
						fxReportMarkupError(theParser, "missing C");
					fxGetCharacter(theParser);
					if (theParser->c != 'D')
						fxReportMarkupError(theParser, "missing D");
					fxGetCharacter(theParser);
					if (theParser->c != 'A')
						fxReportMarkupError(theParser, "missing A");
					fxGetCharacter(theParser);
					if (theParser->c != 'T')
						fxReportMarkupError(theParser, "missing T");
					fxGetCharacter(theParser);
					if (theParser->c != 'A')
						fxReportMarkupError(theParser, "missing A");
					fxGetCharacter(theParser);
					if (theParser->c != '[')
						fxReportMarkupError(theParser, "missing [");
					fxGetCharacter(theParser);
					theParser->c0 = theParser->c;
					fxGetCharacter(theParser);
					theParser->c1 = theParser->c;
					fxGetCharacter(theParser);
					theParser->getter = fxGetNextDataCharacter;
					fxParseText(theParser);
					if (theParser->c != '>')
						fxReportMarkupError(theParser, "missing ]]>");
					fxGetCharacter(theParser);
				}
				else if (theParser->depth == 0) {
					if (theParser->c != 'D')
						fxReportMarkupError(theParser, "missing D");
					fxGetCharacter(theParser);
					if (theParser->c != 'O')
						fxReportMarkupError(theParser, "missing O");
					fxGetCharacter(theParser);
					if (theParser->c != 'C')
						fxReportMarkupError(theParser, "missing C");
					fxGetCharacter(theParser);
					if (theParser->c != 'T')
						fxReportMarkupError(theParser, "missing T");
					fxGetCharacter(theParser);
					if (theParser->c != 'Y')
						fxReportMarkupError(theParser, "missing Y");
					fxGetCharacter(theParser);
					if (theParser->c != 'P')
						fxReportMarkupError(theParser, "missing P");
					fxGetCharacter(theParser);
					if (theParser->c != 'E')
						fxReportMarkupError(theParser, "missing E");
					fxGetCharacter(theParser);
					fxParseDocType(theParser);
				}
				else
					fxReportMarkupError(theParser, "invalid declaration");
			}
			else
				fxParseStartTag(theParser);
		}
		else {
			if (the->hacked) {
				theParser->c0 = theParser->c;
				fxGetCharacter(theParser);
				theParser->getter = fxGetNextScriptCharacter;
			}
			else
				theParser->getter = fxGetNextTextCharacter;
			fxParseText(theParser);
			if (the->hacked && (theParser->c != C_EOF)) {
				fxGetCharacter(theParser);
				fxParseStopTag(theParser);
			}
		}
	}
	if (mxIsReference(mxArgv(0))) {
		char * aName = mxArgv(0)->value.reference->next->next->value.string;
		if (aName)
			fxReportMarkupError(theParser, "EOF instead of </%s>", aName);
	}
}

void fxParseDocType(txMarkupParser* theParser)
{
	if (theParser->callbacks->processDoctype) {
		txMachine* the = theParser->the;

		theParser->getter = fxGetNextDocTypeCharacter;
		mxArgv(2)->value.integer = theParser->line;
		fxNewInstance(the);
		*(--the->stack) = *mxArgv(0);
		fxQueueID(the, the->parentID, XS_GET_ONLY);

		fxNewInstance(the);
		*(--the->stack) = *mxArgv(0);
		fxQueueID(the, the->parentID, XS_GET_ONLY);
		fxParseValue(theParser);
		fxQueueID(the, the->valueID, XS_GET_ONLY);
		fxPushArgument(the);
		(*(theParser->callbacks->processDoctype))(the);
		fxPopArgument(the);
	}
	fxSkipDocType(theParser);
}

void fxParseName(txMarkupParser* theParser)
{
	int aLength;
	
	txMachine* the = theParser->the;
	mxZeroSlot(--the->stack);
	the->stack->value.string = fxNewChunk(the, 1024);
	the->stack->kind = XS_STRING_KIND;
	aLength = fxScanName(theParser, the->stack->value.string, 1024);
	if (!aLength)
		fxReportMarkupError(theParser, "missing name");
	fxResizeString(the, the->stack, aLength + 1);
}

void fxParseProcessingInstruction(txMarkupParser* theParser)
{
	if (theParser->callbacks->processProcessingInstruction) {
		txMachine* the = theParser->the;
		
		mxArgv(2)->value.integer = theParser->line;
		if (!mxIsReference(mxArgv(0)))
			fxReportMarkupError(theParser, "invalid data");
		fxNewInstance(the);
		*(--the->stack) = *mxArgv(0);
		fxQueueID(the, the->parentID, XS_GET_ONLY);
		
		fxParseName(theParser);
		fxQueueID(the, the->nameID, XS_GET_ONLY);
        fxScanSpace(theParser);
		
		mxArgv(2)->value.integer = theParser->line;
		theParser->c0 = theParser->c;
		fxGetCharacter(theParser);
		theParser->getter = fxGetNextProcessingInstructionCharacter;
		
		fxParseValue(theParser);
		fxQueueID(the, the->valueID, XS_GET_ONLY);
		fxPushArgument(the);
		(*(theParser->callbacks->processProcessingInstruction))(the);
		fxPopArgument(the);
		if (theParser->c != '>')
			fxReportMarkupError(theParser, "missing ?>");
		fxGetCharacter(theParser);
	}
	else 
		fxSkipProcessingInstruction(theParser);
}

void fxParseStartTag(txMarkupParser* theParser)
{
	txMachine* the = theParser->the;
	txID anIndex;
	int aLength;
	
	mxArgv(2)->value.integer = theParser->line;
	fxNewInstance(the);
	*(--the->stack) = *mxArgv(0);
	fxQueueID(the, the->parentID, XS_GET_ONLY);
	fxParseName(theParser);
	fxQueueID(the, the->nameID, XS_GET_ONLY);
	fxNewInstance(the);
	anIndex = 0;
	aLength = fxScanSpace(theParser);
	while ((theParser->c != C_EOF) && (theParser->c != '/') && (theParser->c != '>')) {
		if (!aLength)
			fxReportMarkupError(theParser, "missing space");
		fxNewInstance(the);
		fxParseName(theParser);
		fxQueueID(the, the->nameID, XS_GET_ONLY);
		fxScanSpace(theParser);
		if (theParser->c != '=')
			fxReportMarkupError(theParser, "missing =");
		fxGetCharacter(theParser);
		fxScanSpace(theParser);
		if ((theParser->c != '\'') && (theParser->c != '"'))
			fxReportMarkupError(theParser, "missing left quote");
		theParser->c0 = theParser->c;
		fxGetCharacter(theParser);
		theParser->getter = fxGetNextValueCharacter;
		fxParseValue(theParser);
		fxQueueID(the, the->valueID, XS_GET_ONLY);
		if (theParser->c != theParser->c0)
			fxReportMarkupError(theParser, "missing right quote");
		fxGetCharacter(theParser);
		fxQueueID(the, anIndex, XS_GET_ONLY);
		anIndex++;
		aLength = fxScanSpace(theParser);
	}
	fxQueueID(the, the->valueID, XS_GET_ONLY);
	fxPushArgument(the);
	if (theParser->callbacks->processStartTag)
		(*(theParser->callbacks->processStartTag))(the);
	if (theParser->c == '/') {
		fxGetCharacter(theParser);
		if (theParser->callbacks->processStopTag)
			(*(theParser->callbacks->processStopTag))(the);
		fxPopArgument(the);
	}
	else {
		theParser->depth++;
	}
	if (theParser->c == '>')
		fxGetCharacter(theParser);
	else
		fxReportMarkupError(theParser, "missing >");
}

void fxParseStopTag(txMarkupParser* theParser)
{
	txMachine* the = theParser->the;
	char* aName;
	
	mxArgv(2)->value.integer = theParser->line;
	if (!fxScanName(theParser, theParser->name, sizeof(theParser->name)))
		fxReportMarkupError(theParser, "missing element name");
	if (!mxIsReference(mxArgv(0)))
		fxReportMarkupError(theParser, "invalid </%s>", theParser->name);
	aName = mxArgv(0)->value.reference->next->next->value.string;
	if (!aName)
		fxReportMarkupError(theParser, "invalid </%s>", theParser->name);
	if (c_strcmp(theParser->name, aName) != 0)
		fxReportMarkupError(theParser, "</%s> instead of </%s>", theParser->name, aName);
	if (theParser->callbacks->processStopTag)
		(*(theParser->callbacks->processStopTag))(the);
	fxPopArgument(the);
	fxScanSpace(theParser);
	theParser->depth--;
	if (theParser->c == '>')
		fxGetCharacter(theParser);
	else
		fxReportMarkupError(theParser, "missing >");
}

void fxParseText(txMarkupParser* theParser)
{
	txMachine* the = theParser->the;

	mxArgv(2)->value.integer = theParser->line;
	if (!mxIsReference(mxArgv(0)))
		fxReportMarkupError(theParser, "invalid data");
	fxNewInstance(the);
	*(--the->stack) = *mxArgv(0);
	fxQueueID(the, the->parentID, XS_GET_ONLY);
	fxParseValue(theParser);
	fxQueueID(the, the->valueID, XS_GET_ONLY);
	fxPushArgument(the);
	if (theParser->callbacks->processText)
		(*(theParser->callbacks->processText))(the);
	fxPopArgument(the);
}

void fxParseValue(txMarkupParser* theParser)
{
	txMachine* the = theParser->the;
	size_t i;
	size_t j;
	txU1* p;
	txU4 c;
	txUTF8Sequence* aSequence;
	int aShift;
	
	i = 0;
	j = 1024;
	mxZeroSlot(--the->stack);
	the->stack->value.string = fxNewChunk(theParser->the, j);
	p = (txU1*)the->stack->value.string;
	the->stack->kind = XS_STRING_KIND;
	c = (txU4)(*(theParser->getter))(theParser);
	while (c != (txU4)C_EOF) {
		if (c < 128) {
			i++;
			if (i >= j) {
				j += 1024;
				p = (txU1*)fxResizeString(the, the->stack, j);
				p += i - 1;
			}
			*p++ = (txU1)c;
		}
		else {
			for (aSequence = gxUTF8Sequences; aSequence->size; aSequence++)
				if (c <= aSequence->lmask)
					break;
			if (aSequence->size == 0)
				fxReportMarkupError(theParser, "invalid value");
			else {
				i += aSequence->size;
				if (i >= j) {
					j += 1024;
					p = (txU1*)fxResizeString(the, the->stack, j);
					p += i - aSequence->size;
				}
				aShift = aSequence->shift;
				*p++ = (txU1)(aSequence->cval | (c >> aShift));
				while (aShift > 0) {
					aShift -= 6;
					*p++ = (txU1)(0x80 | ((c >> aShift) & 0x3F));
				}
			}
		}
		c = (txU4)(*(theParser->getter))(theParser);
	}
	i++;
	*p = 0;
	fxResizeString(the, the->stack, i);
}

void fxPopArgument(txMachine* the)
{
	mxArgv(0)->kind = mxArgv(0)->value.reference->next->kind;
	mxArgv(0)->value.reference = mxArgv(0)->value.reference->next->value.reference;
}

void fxPushArgument(txMachine* the)
{
	mxArgv(0)->value.reference = the->stack->value.reference;
	mxArgv(0)->kind = the->stack->kind;
	the->stack++;
}

void fxReportMarkupError(txMarkupParser* theParser, char* theFormat, ...)
{
#define XS_NO_MASK (XS_NO_ERROR_FLAG | XS_NO_WARNING_FLAG)
	txMachine* the = theParser->the;
	char *aPath;
	c_va_list arguments;

	if ((the->parseFlag & XS_NO_MASK) == XS_NO_MASK)
		return;

	if (theParser->path)
		aPath = theParser->path->value.symbol.string;
	else
		aPath = C_NULL;
	c_va_start(arguments, theFormat);

	if (!(the->parseFlag & XS_SNIFF_FLAG)) {
		if ((the->parseFlag & XS_NO_MASK) == XS_NO_ERROR_FLAG)
			fxVReportWarning(theParser->the, aPath, theParser->line, theFormat, arguments);
		else
			fxVReportError(theParser->the, aPath, theParser->line, theFormat, arguments);
	}
	c_va_end(arguments);
	if (!(the->parseFlag & XS_SNIFF_FLAG))
		if (!(the->parseFlag & XS_NO_MASK))
			mxDebug0(the, XS_SYNTAX_ERROR, "XML error");
}

int fxScanName(txMarkupParser* theParser, char* theName, size_t theSize)
{
	size_t i;
	char* p;
	int c;

	i = 0;
	p = theName;
	c = theParser->c;
	if ((('A' <= c) && (c <= 'Z')) || (('a' <= c) && (c <= 'z')) 
			|| (c == '_') || (c == ':')) {
		i++;
		*p++ = c;
		for (;;) {
			fxGetCharacter(theParser);
			c = theParser->c;
			if ((('0' <= c) && (c <= '9')) || (('A' <= c) && (c <= 'Z')) || (('a' <= c) && (c <= 'z')) 
					|| (c == '.') || (c == '-') || (c == '_') || (c == ':')) {
				i++;
				if (i < theSize)
					*p++ = c;
				else if (i == theSize)
					fxReportMarkupError(theParser, "name overflow");
			}
			else
				break;
		}
	}
	*p = 0;
	return i;	
}

int fxScanSpace(txMarkupParser* theParser)
{
	int i = 0;
	
	for (;;) {
		switch (theParser->c) {
		case 10:
			i++;	
			theParser->line++;
			fxGetCharacter(theParser);
			break;
		case 13:	
			i++;	
			theParser->line++;
			fxGetCharacter(theParser);
			if (theParser->c == 10)
				fxGetCharacter(theParser);
			break;
		case '\t':
		case ' ':
			i++;	
			fxGetCharacter(theParser);
			break;
		default:
			return i;
		}
	}
}

void fxSkipComment(txMarkupParser* theParser)
{
	int c;
	theParser->c0 = theParser->c;
	fxGetCharacter(theParser);
	theParser->c1 = theParser->c;
	fxGetCharacter(theParser);
	do {
		if ((theParser->c0 == '-') && (theParser->c1 == '-') && (theParser->c == '>'))
			c = C_EOF;
		else {
			c = theParser->c0;
			theParser->c0 = theParser->c1;
			theParser->c1 = theParser->c;
			fxGetCharacter(theParser);
			if ((c == 10) || ((c == 13) && (theParser->c0 != 10)))
				theParser->line++;
		}
	} while (c != C_EOF);
	if (theParser->c == '>')
		fxGetCharacter(theParser);
	else
		fxReportMarkupError(theParser, "missing -->");
}

void fxSkipDeclaration(txMarkupParser* theParser)
{
	int c;
	do {
		if (theParser->c == '>')
			c = C_EOF;
		else {
			c = theParser->c;
			fxGetCharacter(theParser);
			if ((c == 10) || ((c == 13) && (theParser->c != 10)))
				theParser->line++;
		}
	} while (c != C_EOF);
	if (theParser->c == '>')
		fxGetCharacter(theParser);
	else
		fxReportMarkupError(theParser, "missing >");
}

void fxSkipDocType(txMarkupParser* theParser)
{
	int c;
	do {
		if ((theParser->c == '[') || (theParser->c == '>'))
			c = C_EOF;
		else {
			c = theParser->c;
			fxGetCharacter(theParser);
			if ((c == 10) || ((c == 13) && (theParser->c != 10)))
				theParser->line++;
		}
	} while (c != C_EOF);
	if (theParser->c == '[') {
		fxGetCharacter(theParser);
		for (;;) {
			fxScanSpace(theParser);
			if (theParser->c == C_EOF)
				break;
			else if (theParser->c == ']')
				break;
			else if (theParser->c == '<') {
				fxGetCharacter(theParser);
				if (theParser->c == '?') {
					fxGetCharacter(theParser);
					fxSkipProcessingInstruction(theParser);
				}
				else if (theParser->c == '!') {
					fxGetCharacter(theParser);
					if (theParser->c == '-') {
						fxGetCharacter(theParser);
						if (theParser->c != '-')
							fxReportMarkupError(theParser, "missing -");
						fxGetCharacter(theParser);
						fxSkipComment(theParser);
					}
					else
						fxSkipDeclaration(theParser);
				}
			}
		}
		if (theParser->c == ']')
			fxGetCharacter(theParser);
		else
			fxReportMarkupError(theParser, "missing ]");
		fxScanSpace(theParser);
	}
	if (theParser->c == '>')
		fxGetCharacter(theParser);
	else
		fxReportMarkupError(theParser, "missing >");
}
				
void fxSkipProcessingInstruction(txMarkupParser* theParser)
{
	int c;
	theParser->c0 = theParser->c;
	fxGetCharacter(theParser);
	do {
		if ((theParser->c0 == '?') && (theParser->c == '>'))
			c = C_EOF;
		else {
			c = theParser->c0;
			theParser->c0 = theParser->c;
			fxGetCharacter(theParser);
			if ((c == 10) || ((c == 13) && (theParser->c0 != 10)))
				theParser->line++;
		}
	} while (c != C_EOF);
	if (theParser->c == '>')
		fxGetCharacter(theParser);
	else
		fxReportMarkupError(theParser, "missing ?>");
}

#define mxEntityCount 253
static txEntity gxEntities[mxEntityCount] = {
	{"AElig", 5, "\xc3\x86", 2},{"Aacute", 6, "\xc3\x81", 2},{"Acirc", 5, "\xc3\x82", 2},{"Agrave", 6, "\xc3\x80", 2},
	{"Alpha", 5, "\xce\x91", 2},{"Aring", 5, "\xc3\x85", 2},{"Atilde", 6, "\xc3\x83", 2},{"Auml", 4, "\xc3\x84", 2},
	{"Beta", 4, "\xce\x92", 2},{"Ccedil", 6, "\xc3\x87", 2},{"Chi", 3, "\xce\xa7", 2},{"Dagger", 6, "\xe2\x80\xa1", 3},
	{"Delta", 5, "\xce\x94", 2},{"ETH", 3, "\xc3\x90", 2},{"Eacute", 6, "\xc3\x89", 2},{"Ecirc", 5, "\xc3\x8a", 2},
	{"Egrave", 6, "\xc3\x88", 2},{"Epsilon", 7, "\xce\x95", 2},{"Eta", 3, "\xce\x97", 2},{"Euml", 4, "\xc3\x8b", 2},
	{"Gamma", 5, "\xce\x93", 2},{"Iacute", 6, "\xc3\x8d", 2},{"Icirc", 5, "\xc3\x8e", 2},{"Igrave", 6, "\xc3\x8c", 2},
	{"Iota", 4, "\xce\x99", 2},{"Iuml", 4, "\xc3\x8f", 2},{"Kappa", 5, "\xce\x9a", 2},{"Lambda", 6, "\xce\x9b", 2},
	{"Mu", 2, "\xce\x9c", 2},{"Ntilde", 6, "\xc3\x91", 2},{"Nu", 2, "\xce\x9d", 2},{"OElig", 5, "\xc5\x92", 2},
	{"Oacute", 6, "\xc3\x93", 2},{"Ocirc", 5, "\xc3\x94", 2},{"Ograve", 6, "\xc3\x92", 2},{"Omega", 5, "\xce\xa9", 2},
	{"Omicron", 7, "\xce\x9f", 2},{"Oslash", 6, "\xc3\x98", 2},{"Otilde", 6, "\xc3\x95", 2},{"Ouml", 4, "\xc3\x96", 2},
	{"Phi", 3, "\xce\xa6", 2},{"Pi", 2, "\xce\xa0", 2},{"Prime", 5, "\xe2\x80\xb3", 3},{"Psi", 3, "\xce\xa8", 2},
	{"Rho", 3, "\xce\xa1", 2},{"Scaron", 6, "\xc5\xa0", 2},{"Sigma", 5, "\xce\xa3", 2},{"THORN", 5, "\xc3\x9e", 2},
	{"Tau", 3, "\xce\xa4", 2},{"Theta", 5, "\xce\x98", 2},{"Uacute", 6, "\xc3\x9a", 2},{"Ucirc", 5, "\xc3\x9b", 2},
	{"Ugrave", 6, "\xc3\x99", 2},{"Upsilon", 7, "\xce\xa5", 2},{"Uuml", 4, "\xc3\x9c", 2},{"Xi", 2, "\xce\x9e", 2},
	{"Yacute", 6, "\xc3\x9d", 2},{"Yuml", 4, "\xc5\xb8", 2},{"Zeta", 4, "\xce\x96", 2},{"aacute", 6, "\xc3\xa1", 2},
	{"acirc", 5, "\xc3\xa2", 2},{"acute", 5, "\xc2\xb4", 2},{"aelig", 5, "\xc3\xa6", 2},{"agrave", 6, "\xc3\xa0", 2},
	{"alefsym", 7, "\xe2\x84\xb5", 3},{"alpha", 5, "\xce\xb1", 2},{"amp", 3, "\x26", 1},{"and", 3, "\xe2\x88\xa7", 3},
	{"ang", 3, "\xe2\x88\xa0", 3},{"apos", 4, "\x27", 1},{"aring", 5, "\xc3\xa5", 2},{"asymp", 5, "\xe2\x89\x88", 3},
	{"atilde", 6, "\xc3\xa3", 2},{"auml", 4, "\xc3\xa4", 2},{"bdquo", 5, "\xe2\x80\x9e", 3},{"beta", 4, "\xce\xb2", 2},
	{"brvbar", 6, "\xc2\xa6", 2},{"bull", 4, "\xe2\x80\xa2", 3},{"cap", 3, "\xe2\x88\xa9", 3},{"ccedil", 6, "\xc3\xa7", 2},
	{"cedil", 5, "\xc2\xb8", 2},{"cent", 4, "\xc2\xa2", 2},{"chi", 3, "\xcf\x87", 2},{"circ", 4, "\xcb\x86", 2},
	{"clubs", 5, "\xe2\x99\xa3", 3},{"cong", 4, "\xe2\x89\x85", 3},{"copy", 4, "\xc2\xa9", 2},{"crarr", 5, "\xe2\x86\xb5", 3},
	{"cup", 3, "\xe2\x88\xaa", 3},{"curren", 6, "\xc2\xa4", 2},{"dArr", 4, "\xe2\x87\x93", 3},{"dagger", 6, "\xe2\x80\xa0", 3},
	{"darr", 4, "\xe2\x86\x93", 3},{"deg", 3, "\xc2\xb0", 2},{"delta", 5, "\xce\xb4", 2},{"diams", 5, "\xe2\x99\xa6", 3},
	{"divide", 6, "\xc3\xb7", 2},{"eacute", 6, "\xc3\xa9", 2},{"ecirc", 5, "\xc3\xaa", 2},{"egrave", 6, "\xc3\xa8", 2},
	{"empty", 5, "\xe2\x88\x85", 3},{"emsp", 4, "\xe2\x80\x83", 3},{"ensp", 4, "\xe2\x80\x82", 3},{"epsilon", 7, "\xce\xb5", 2},
	{"equiv", 5, "\xe2\x89\xa1", 3},{"eta", 3, "\xce\xb7", 2},{"eth", 3, "\xc3\xb0", 2},{"euml", 4, "\xc3\xab", 2},
	{"euro", 4, "\xe2\x82\xac", 3},{"exist", 5, "\xe2\x88\x83", 3},{"fnof", 4, "\xc6\x92", 2},{"forall", 6, "\xe2\x88\x80", 3},
	{"frac12", 6, "\xc2\xbd", 2},{"frac14", 6, "\xc2\xbc", 2},{"frac34", 6, "\xc2\xbe", 2},{"frasl", 5, "\xe2\x81\x84", 3},
	{"gamma", 5, "\xce\xb3", 2},{"ge", 2, "\xe2\x89\xa5", 3},{"gt", 2, "\x3e", 1},{"hArr", 4, "\xe2\x87\x94", 3},
	{"harr", 4, "\xe2\x86\x94", 3},{"hearts", 6, "\xe2\x99\xa5", 3},{"hellip", 6, "\xe2\x80\xa6", 3},{"iacute", 6, "\xc3\xad", 2},
	{"icirc", 5, "\xc3\xae", 2},{"iexcl", 5, "\xc2\xa1", 2},{"igrave", 6, "\xc3\xac", 2},{"image", 5, "\xe2\x84\x91", 3},
	{"infin", 5, "\xe2\x88\x9e", 3},{"int", 3, "\xe2\x88\xab", 3},{"iota", 4, "\xce\xb9", 2},{"iquest", 6, "\xc2\xbf", 2},
	{"isin", 4, "\xe2\x88\x88", 3},{"iuml", 4, "\xc3\xaf", 2},{"kappa", 5, "\xce\xba", 2},{"lArr", 4, "\xe2\x87\x90", 3},
	{"lambda", 6, "\xce\xbb", 2},{"lang", 4, "\xe2\x8c\xa9", 3},{"laquo", 5, "\xc2\xab", 2},{"larr", 4, "\xe2\x86\x90", 3},
	{"lceil", 5, "\xe2\x8c\x88", 3},{"ldquo", 5, "\xe2\x80\x9c", 3},{"le", 2, "\xe2\x89\xa4", 3},{"lfloor", 6, "\xe2\x8c\x8a", 3},
	{"lowast", 6, "\xe2\x88\x97", 3},{"loz", 3, "\xe2\x97\x8a", 3},{"lrm", 3, "\xe2\x80\x8e", 3}, {"lsaquo", 6, "\xe2\x80\xb9", 3},
	{"lsquo", 5, "\xe2\x80\x98", 3},{"lt", 2, "\x3c", 1},{"macr", 4, "\xc2\xaf", 2},{"mdash", 5, "\xe2\x80\x94", 3},
	{"micro", 5, "\xc2\xb5", 2},{"middot", 6, "\xc2\xb7", 2},{"minus", 5, "\xe2\x88\x92", 3},{"mu", 2, "\xce\xbc", 2},
	{"nabla", 5, "\xe2\x88\x87", 3},{"nbsp", 4, "\xc2\xa0", 2},{"ndash", 5, "\xe2\x80\x93", 3},{"ne", 2, "\xe2\x89\xa0", 3},
	{"ni", 2, "\xe2\x88\x8b", 3},{"not", 3, "\xc2\xac", 2},{"notin", 5, "\xe2\x88\x89", 3},{"nsub", 4, "\xe2\x8a\x84", 3},
	{"ntilde", 6, "\xc3\xb1", 2},{"nu", 2, "\xce\xbd", 2},{"oacute", 6, "\xc3\xb3", 2},{"ocirc", 5, "\xc3\xb4", 2},
	{"oelig", 5, "\xc5\x93", 2},{"ograve", 6, "\xc3\xb2", 2},{"oline", 5, "\xe2\x80\xbe", 3},{"omega", 5, "\xcf\x89", 2},
	{"omicron", 7, "\xce\xbf", 2},{"oplus", 5, "\xe2\x8a\x95", 3},{"or", 2, "\xe2\x88\xa8", 3},{"ordf", 4, "\xc2\xaa", 2},
	{"ordm", 4, "\xc2\xba", 2},{"oslash", 6, "\xc3\xb8", 2},{"otilde", 6, "\xc3\xb5", 2},{"otimes", 6, "\xe2\x8a\x97", 3},
	{"ouml", 4, "\xc3\xb6", 2},{"para", 4, "\xc2\xb6", 2},{"part", 4, "\xe2\x88\x82", 3},{"permil", 6, "\xe2\x80\xb0", 3},
	{"perp", 4, "\xe2\x8a\xa5", 3},{"phi", 3, "\xcf\x86", 2},{"pi", 2, "\xcf\x80", 2},{"piv", 3, "\xcf\x96", 2},
	{"plusmn", 6, "\xc2\xb1", 2},{"pound", 5, "\xc2\xa3", 2},{"prime", 5, "\xe2\x80\xb2", 3},{"prod", 4, "\xe2\x88\x8f", 3},
	{"prop", 4, "\xe2\x88\x9d", 3},{"psi", 3, "\xcf\x88", 2},{"quot", 4, "\x22", 1},{"rArr", 4, "\xe2\x87\x92", 3},
	{"radic", 5, "\xe2\x88\x9a", 3},{"rang", 4, "\xe2\x8c\xaa", 3},{"raquo", 5, "\xc2\xbb", 2},{"rarr", 4, "\xe2\x86\x92", 3},
	{"rceil", 5, "\xe2\x8c\x89", 3},{"rdquo", 5, "\xe2\x80\x9d", 3},{"real", 4, "\xe2\x84\x9c", 3},{"reg", 3, "\xc2\xae", 2},
	{"rfloor", 6, "\xe2\x8c\x8b", 3},{"rho", 3, "\xcf\x81", 2},{"rlm", 3, "\xe2\x80\x8f", 3},{"rsaquo", 6, "\xe2\x80\xba", 3},
	{"rsquo", 5, "\xe2\x80\x99", 3},{"sbquo", 5, "\xe2\x80\x9a", 3},{"scaron", 6, "\xc5\xa1", 2},{"sdot", 4, "\xe2\x8b\x85", 3},
	{"sect", 4, "\xc2\xa7", 2},{"shy", 3, "\xc2\xad", 2},{"sigma", 5, "\xcf\x83", 2},{"sigmaf", 6, "\xcf\x82", 2},
	{"sim", 3, "\xe2\x88\xbc", 3},{"spades", 6, "\xe2\x99\xa0", 3},{"sub", 3, "\xe2\x8a\x82", 3},{"sube", 4, "\xe2\x8a\x86", 3},
	{"sum", 3, "\xe2\x88\x91", 3},{"sup", 3, "\xe2\x8a\x83", 3},{"sup1", 4, "\xc2\xb9", 2},{"sup2", 4, "\xc2\xb2", 2},
	{"sup3", 4, "\xc2\xb3", 2},{"supe", 4, "\xe2\x8a\x87", 3},{"szlig", 5, "\xc3\x9f", 2},{"tau", 3, "\xcf\x84", 2},
	{"there4", 6, "\xe2\x88\xb4", 3},{"theta", 5, "\xce\xb8", 2},{"thetasym", 8, "\xcf\x91", 2},{"thinsp", 6, "\xe2\x80\x89", 3},
	{"thorn", 5, "\xc3\xbe", 2},{"tilde", 5, "\xcb\x9c", 2},{"times", 5, "\xc3\x97", 2},{"trade", 5, "\xe2\x84\xa2", 3},
	{"uArr", 4, "\xe2\x87\x91", 3},{"uacute", 6, "\xc3\xba", 2},{"uarr", 4, "\xe2\x86\x91", 3},{"ucirc", 5, "\xc3\xbb", 2},
	{"ugrave", 6, "\xc3\xb9", 2},{"uml", 3, "\xc2\xa8", 2},{"upsih", 5, "\xcf\x92", 2},{"upsilon", 7, "\xcf\x85", 2},
	{"uuml", 4, "\xc3\xbc", 2},{"weierp", 6, "\xe2\x84\x98", 3},{"xi", 2, "\xce\xbe", 2},{"yacute", 6, "\xc3\xbd", 2},
	{"yen", 3, "\xc2\xa5", 2},{"yuml", 4, "\xc3\xbf", 2},{"zeta", 4, "\xce\xb6", 2},{"zwj", 3, "\xe2\x80\x8d", 3},
	{"zwnj", 4, "\xe2\x80\x8c", 3}
};

static int fxCompareEntities(const void *theName, const void *theEntity)
{
	return c_strcmp((txString)theName, ((txEntity*)theEntity)->name);
}

txEntity* fxSearchEntity(txString theName)
{
	return (txEntity* )c_bsearch(theName, gxEntities, mxEntityCount, sizeof(txEntity), fxCompareEntities);
}


