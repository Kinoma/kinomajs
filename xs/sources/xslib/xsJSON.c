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
void fxBuildJSON(txMachine* the)
{
	txSlot* xs;
	
	mxPush(mxGlobal);
	mxPush(mxObjectPrototype);
	fxNewObjectInstance(the);
	fxNewHostFunction(the, fx_JSON_parse, 2);
	fxQueueID(the, fxID(the, "parse"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_JSON_stringify, 3);
	fxQueueID(the, fxID(the, "stringify"), XS_DONT_ENUM_FLAG);
	fxQueueID(the, fxID(the, "JSON"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	the->stack++;
	
	xs = fxGetProperty(the, mxGlobal.value.reference, fxID(the, "xs"));
	mxPush(*xs);
	
	mxPush(mxObjectPrototype);
	fxNewObjectInstance(the);
	fxNewHostFunction(the, fx_JSON_parse, 1);
	fxQueueID(the, fxID(the, "parse"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	fxNewHostFunction(the, fx_JSON_stringify, 1);
	fxQueueID(the, fxID(the, "serialize"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	fxQueueID(the, fxID(the, "json"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	
	the->stack++;
}

void fx_JSON_parse(txMachine* the)
{
	txSlot* aSlot;
	txStringCStream aCStream;
	txStringStream aStream;

	aSlot = fxGetInstance(the, mxThis);
	if (aSlot->flag & XS_SANDBOX_FLAG)
		the->frame->flag |= XS_SANDBOX_FLAG;
	else
		the->frame->flag |= the->frame->next->flag & XS_SANDBOX_FLAG;
	if (mxArgc < 1)
		mxDebug0(the, XS_SYNTAX_ERROR, "JSON.parse: no buffer");
	aSlot = fxGetInstance(the, mxArgv(0));
	if (mxIsChunk(aSlot)) {
		aSlot = aSlot->next;
		aCStream.buffer = aSlot->value.host.data;
		aCStream.size = aSlot->next->value.integer;
		aCStream.offset = 0;
		fxParseJSON(the, &aCStream, fxStringCGetter);
	}
	else {
		fxToString(the, mxArgv(0));
		aStream.slot = mxArgv(0);
		aStream.size = c_strlen(aStream.slot->value.string);
		aStream.offset = 0;
		fxParseJSON(the, &aStream, fxStringGetter);
	}
	*mxResult = *(the->stack++);
}

#define XS_JSON_KEYWORD_COUNT 3
static txScriptKeyword gxJSONKeywords[XS_JSON_KEYWORD_COUNT] = {
	{ "false", XS_TOKEN_FALSE },
	{ "null", XS_TOKEN_NULL },
	{ "true", XS_TOKEN_TRUE }
};

void fxGetNextJSONKeyword(txScriptParser* theParser)
{
	int low, high, anIndex, aDelta;
	
	for (low = 0, high = XS_JSON_KEYWORD_COUNT; high > low;
			(aDelta < 0) ? (low = anIndex + 1) : (high = anIndex)) {
		anIndex = low + ((high - low) / 2);
		aDelta = c_strcmp(gxJSONKeywords[anIndex].text, theParser->buffer);
		if (aDelta == 0) {
			theParser->token2 = gxJSONKeywords[anIndex].token;
			return;
		}
	}
	theParser->symbol2 = fxNewSymbolC(theParser->the, theParser->buffer);
	theParser->token2 = XS_TOKEN_IDENTIFIER;
}

void fxGetNextJSONToken(txScriptParser* theParser)
{
	txMachine* the = theParser->the;
	int c;
	txString p;
	txString q;
	txString r;
	txString s;
	txU4 t = 0;
	txNumber aNumber;

	theParser->line = theParser->line2;

	theParser->crlf = theParser->crlf2;
	theParser->escaped = theParser->escaped2;
	theParser->flags->value.string = theParser->flags2->value.string;
	theParser->integer = theParser->integer2;
	theParser->number = theParser->number2;
	theParser->string->value.string = theParser->string2->value.string;
	theParser->symbol = theParser->symbol2;
	theParser->token = theParser->token2;
	
	theParser->crlf2 = 0;
	theParser->escaped2 = 0;
	theParser->flags2->value.string = mxEmptyString.value.string;
	theParser->integer2 = 0;
	theParser->number2 = 0;
	theParser->string2->value.string = mxEmptyString.value.string;
	theParser->symbol2 = C_NULL;
	theParser->token2 = XS_NO_TOKEN;
	while (theParser->token2 == XS_NO_TOKEN) {
		switch (theParser->character) {
		case C_EOF:
			theParser->token2 = XS_TOKEN_EOF;
			break;
		case 10:	
			theParser->line2++;
			fxGetNextCharacter(theParser);
			theParser->crlf2 = 1;
			break;
		case 13:	
			theParser->line2++;
			fxGetNextCharacter(theParser);
			if (theParser->character == 10)
				fxGetNextCharacter(theParser);
			theParser->crlf2 = 1;
			break;
			
		case '\t':
		case ' ':
			fxGetNextCharacter(theParser);
			break;
			
		case '0':
			fxGetNextCharacter(theParser);
			c = theParser->character;
			if (c == '.') {
				fxGetNextCharacter(theParser);
				c = theParser->character;
				if ((('0' <= c) && (c <= '9')) || (c == 'e') || (c == 'E'))
					fxGetNextNumber(theParser, 0);
				else {
					theParser->number2 = 0;
					theParser->token2 = XS_TOKEN_NUMBER;
				}
			}
			else if ((c == 'e') || (c == 'E')) {
				fxGetNextNumber(theParser, 0);
			}
			else if ((c == 'x') || (c == 'X')) {
				p = theParser->buffer;
				*p++ = '0';
				*p++ = 'x';
				for (;;) {
					fxGetNextCharacter(theParser);
					c = theParser->character;
					if (('0' <= c) && (c <= '9'))
						*p++ = c;
					else if (('A' <= c) && (c <= 'Z'))
						*p++ = c;
					else if (('a' <= c) && (c <= 'z'))
						*p++ = c;
					else
						break;
				}
				*p = 0;
				theParser->number2 = fxStringToNumber(theParser->the, theParser->buffer, 1);
				theParser->integer2 = (txInteger)theParser->number2;
				aNumber = theParser->integer2;
				if (theParser->number2 == aNumber)
					theParser->token2 = XS_TOKEN_INTEGER;
				else
					theParser->token2 = XS_TOKEN_NUMBER;
			}
			else {
				theParser->integer2 = 0;
				theParser->token2 = XS_TOKEN_INTEGER;
			}
			break;
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
		case '-':	
			fxGetNextNumber(theParser, 1);
			break;
		case ',':
			theParser->token2 = XS_TOKEN_COMMA;
			fxGetNextCharacter(theParser);
			break;	
		case ':':
			theParser->token2 = XS_TOKEN_COLON;
			fxGetNextCharacter(theParser);
			break;	
		case '[':
			theParser->token2 = XS_TOKEN_LEFT_BRACKET;
			fxGetNextCharacter(theParser);
			break;	
		case ']':
			theParser->token2 = XS_TOKEN_RIGHT_BRACKET;
			fxGetNextCharacter(theParser);
			break;	
		case '{':
			theParser->token2 = XS_TOKEN_LEFT_BRACE;
			fxGetNextCharacter(theParser);
			break;	
		case '}':
			theParser->token2 = XS_TOKEN_RIGHT_BRACE;
			fxGetNextCharacter(theParser);
			break;	
		case '"':
			p = theParser->buffer;
			q = p + sizeof(theParser->buffer) - 1;
			r = C_NULL;
			fxGetNextCharacter(theParser);
			for (;;) {
				if (theParser->character == C_EOF) {
					fxReportParserError(theParser, "end of file in string");			
					break;
				}
				else if ((theParser->character == 10) || (theParser->character == 13)) {
					fxReportParserError(theParser, "end of line in string");			
					break;
				}
				else if ((0 <= theParser->character) && (theParser->character < 32)) {
					fxReportParserError(theParser, "invalid character in string");			
					break;
				}
				else if (theParser->character == '"') {
					fxGetNextCharacter(theParser);
					break;
				}
				else if (theParser->character == '"') {
					fxGetNextCharacter(theParser);
					break;
				}
				else if (theParser->character == '\\') {
					theParser->escaped2 = 1;
					r = C_NULL;
					fxGetNextCharacter(theParser);
					switch (theParser->character) {
					case 10:
						theParser->line2++;
						fxGetNextCharacter(theParser);
						break;
					case 13:
						theParser->line2++;
						fxGetNextCharacter(theParser);
						if (theParser->character == 10)
							fxGetNextCharacter(theParser);
						break;
					case '\'':
						if (p < q) *p++ = '\'';
						fxGetNextCharacter(theParser);
						break;
					case '"':
						if (p < q) *p++ = '"';
						fxGetNextCharacter(theParser);
						break;
					case '\\':
						if (p < q) *p++ = '\\';
						fxGetNextCharacter(theParser);
						break;
					case '0':
						if (p < q) *p++ = 0;
						fxGetNextCharacter(theParser);
						break;
					case 'b':
						if (p < q) *p++ = '\b';
						fxGetNextCharacter(theParser);
						break;
					case 'f':
						if (p < q) *p++ = '\f';
						fxGetNextCharacter(theParser);
						break;
					case 'n':
						if (p < q) *p++ = '\n';
						fxGetNextCharacter(theParser);
						break;
					case 'r':
						if (p < q) *p++ = '\r';
						fxGetNextCharacter(theParser);
						break;
					case 't':
						if (p < q) *p++ = '\t';
						fxGetNextCharacter(theParser);
						break;
					case 'u':
						r = p;
						t = 5;
						if (p < q) *p++ = 'u';
						fxGetNextCharacter(theParser);
						break;
					case 'v':
						if (p < q) *p++ = '\v';
						fxGetNextCharacter(theParser);
						break;
					case 'x':
						r = p;
						t = 3;
						if (p < q) *p++ = 'x';
						fxGetNextCharacter(theParser);
						break;
					default:
						p = (txString)fsX2UTF8(theParser, theParser->character, (txU1*)p, q - p);
						fxGetNextCharacter(theParser);
						break;
					}
				}
				else {
					p = (txString)fsX2UTF8(theParser, theParser->character, (txU1*)p, q - p);
					if (r) {
						if ((txU4)(p - r) > t)
							r = C_NULL;
						else if (((txU4)(p - r) == t) && (p < q)) {
							*p = 0;
							t = c_strtoul(r + 1, &s, 16);
							if (!*s)
								p = (txString)fsX2UTF8(theParser, t, (txU1*)r, q - r);
							r = C_NULL;
						}
					}
					fxGetNextCharacter(theParser);
				}
			}
			*p = 0;
			if (p == q)
				fxReportParserError(theParser, "string overflow");			
			fxCopyStringC(theParser->the, theParser->string2, theParser->buffer);
			theParser->token2 = XS_TOKEN_STRING;
			break;
		default:
			if (fxIsIdentifierFirst(theParser->character)) {
				p = theParser->buffer;
				q = p + sizeof(theParser->buffer) - 1;
				for (;;) {
					if (p == q) {
						fxReportParserError(theParser, "identifier overflow");			
						break;
					}
					*p++ = theParser->character;
					fxGetNextCharacter(theParser);
					if (!fxIsIdentifierNext(theParser->character))
						break;
				}
				*p = 0;
				fxGetNextJSONKeyword(theParser);
			}
			else {
				mxDebug1(theParser->the, XS_SYNTAX_ERROR, "invalid character %d", theParser->character);
				fxGetNextCharacter(theParser);
			}
			break;
		}
	}
}

void fxParseJSON(txMachine* the, void* theStream, txGetter theGetter)
{
	txSlot* aStack = the->stack;
	txScriptParser *aParser;

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
	aParser->path = C_NULL;
	aParser->line2 = 0;
	
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

	fxGetNextCharacter(aParser);
	fxGetNextJSONToken(aParser);
	fxGetNextJSONToken(aParser);

	fxParseJSONValue(aParser);
	fxMatchToken(aParser, XS_TOKEN_EOF);
	
	if (aParser->errorCount > 0) {
		the->stack = aStack;
		c_strcpy(aParser->buffer, "JSON error(s)!");
#ifdef mxDebug
		fxDebugLoop(the, aParser->buffer);
#endif
		fxThrowMessage(the, XS_SYNTAX_ERROR, aParser->buffer);
	}
}

void fxParseJSONValue(txScriptParser* theParser)
{
	txMachine* the = theParser->the;
	switch (theParser->token) {
	case XS_TOKEN_FALSE:
		mxInitSlot(--the->stack, XS_BOOLEAN_KIND);
		the->stack->value.boolean = 0;
		fxGetNextJSONToken(theParser);
		break;
	case XS_TOKEN_TRUE:
		mxInitSlot(--the->stack, XS_BOOLEAN_KIND);
		the->stack->value.boolean = 1;
		fxGetNextJSONToken(theParser);
		break;
	case XS_TOKEN_NULL:
		mxInitSlot(--the->stack, XS_NULL_KIND);
		fxGetNextJSONToken(theParser);
		break;
	case XS_TOKEN_INTEGER:
		mxInitSlot(--the->stack, XS_INTEGER_KIND);
		the->stack->value.integer = theParser->integer;
		fxGetNextJSONToken(theParser);
		break;
	case XS_TOKEN_NUMBER:
		mxInitSlot(--the->stack, XS_NUMBER_KIND);
		the->stack->value.number = theParser->number;
		fxGetNextJSONToken(theParser);
		break;
	case XS_TOKEN_STRING:
		mxInitSlot(--the->stack, XS_STRING_KIND);
		the->stack->value.string = theParser->string->value.string;
		fxGetNextJSONToken(theParser);
		break;
	case XS_TOKEN_LEFT_BRACE:
		fxParseJSONObject(theParser);
		break;
	case XS_TOKEN_LEFT_BRACKET:
		fxParseJSONArray(theParser);
		break;
	default:
		mxInitSlot(--the->stack, XS_UNDEFINED_KIND);
		fxReportParserError(theParser, "invalid value");
		break;
	}
}

void fxParseJSONObject(txScriptParser* theParser)
{
	txMachine* the = theParser->the;
	txSlot* anObject;
	txSlot* aProperty;
	txID anID;

	fxGetNextJSONToken(theParser);
	*(--the->stack) = mxObjectPrototype;
	anObject = fxNewObjectInstance(the);
	for (;;) {
		if (theParser->token == XS_TOKEN_RIGHT_BRACE)
			break;
		if (theParser->token != XS_TOKEN_STRING) {
			fxReportParserError(theParser, "missing name");
			break;
		}
		mxInitSlot(--the->stack, XS_STRING_KIND);
		the->stack->value.string = theParser->string->value.string;
		fxGetNextJSONToken(theParser);
		if (theParser->token != XS_TOKEN_COLON) {
			fxReportParserError(theParser, "missing :");
			break;
		}
		fxGetNextJSONToken(theParser);
		fxParseJSONValue(theParser);
		aProperty = fxSetPropertyAt(the, anObject, the->stack + 1, &anID, C_NULL);
		aProperty->kind = the->stack->kind;
		aProperty->value = the->stack->value;
		the->stack += 2;
		if (theParser->token != XS_TOKEN_COMMA)
			break;
		fxGetNextJSONToken(theParser);
	}
	if (theParser->token != XS_TOKEN_RIGHT_BRACE)
		fxReportParserError(theParser, "missing }");
	fxGetNextJSONToken(theParser);
}

void fxParseJSONArray(txScriptParser* theParser)
{
	txMachine* the = theParser->the;
	txSlot* anArray;
	txIndex aLength;
	txSlot* anItem;

	fxGetNextJSONToken(theParser);
	*(--the->stack) = mxArrayPrototype;
	anArray = fxNewArrayInstance(the);
	aLength = 0;
	anItem = anArray->next;
	for (;;) {
		if (theParser->token == XS_TOKEN_RIGHT_BRACKET)
			break;
		fxParseJSONValue(theParser);
		aLength++;
		anItem->next = fxNewSlot(the);
		anItem = anItem->next;
		anItem->kind = the->stack->kind;
		anItem->value = the->stack->value;
		the->stack++;
		if (theParser->token != XS_TOKEN_COMMA)
			break;
		fxGetNextJSONToken(theParser);
	}
	anArray->next->value.array.length = aLength;
	fxCacheArray(the, anArray);
	if (theParser->token != XS_TOKEN_RIGHT_BRACKET)
		fxReportParserError(theParser, "missing ]");
	fxGetNextJSONToken(theParser);
}

void fx_JSON_stringify(txMachine* the)
{
	txSlot* aSlot;
	volatile txJSONSerializer aSerializer;
	txFlag aFlag;
	
	mxTry(the) {
		aSlot = fxGetInstance(the, mxThis);
		if (aSlot->flag & XS_SANDBOX_FLAG)
			the->frame->flag |= XS_SANDBOX_FLAG;
		else
			the->frame->flag |= the->frame->next->flag & XS_SANDBOX_FLAG;
		c_memset((txJSONSerializer*)&aSerializer, 0, sizeof(aSerializer));
		aSerializer.offset = 0;
		aSerializer.size = 1024;
		aSerializer.buffer = c_malloc(1024);
		if (!aSerializer.buffer)
			fxThrowError(the, XS_RANGE_ERROR);
	
		if (mxArgc > 1) {
			aSlot = mxArgv(1);
			if (mxIsReference(aSlot)) {
				aSlot = fxGetInstance(the, aSlot);
				if (mxIsFunction(aSlot))
					aSerializer.replacer = mxArgv(1);
				else if (mxIsArray(aSlot))
					mxDebug0(the, XS_SYNTAX_ERROR, "not yet implememented");
			}
		}
		if (mxArgc > 2) {
			aSlot = mxArgv(2);
			if (mxIsReference(aSlot)) {
				aSlot = fxGetInstance(the, aSlot);
				if (mxIsNumber(aSlot) || mxIsString(aSlot))
					aSlot = aSlot->next;
			}
			if ((aSlot->kind == XS_INTEGER_KIND) || (aSlot->kind == XS_NUMBER_KIND)) {
				txInteger aCount = fxToInteger(the, aSlot), anIndex;
				if (aCount < 0)
					aCount = 0;
				else if (aCount > 10)
					aCount = 10;
				for (anIndex = 0; anIndex < aCount; anIndex++)
					aSerializer.indent[anIndex] = ' ';
			}
			else if (aSlot->kind == XS_STRING_KIND)
				c_strncpy((char *)aSerializer.indent, aSlot->value.string, 10);
		}
	
		aSerializer.stack = the->stack;
		*(--the->stack) = mxObjectPrototype;
		fxNewObjectInstance(the);
		aFlag = 0;
		if (mxArgc > 0)
			*(--the->stack) = *mxArgv(0);
		else
			mxZeroSlot(--the->stack);
		*(--the->stack) = mxEmptyString;
		fxSerializeJSONProperty(the, (txJSONSerializer*)&aSerializer, &aFlag);
		the->stack++;
		
		if (aSerializer.offset) {
			mxResult->value.string = (txString)fxNewChunk(the, aSerializer.offset + 1);
			c_memcpy(mxResult->value.string, aSerializer.buffer, aSerializer.offset);
			mxResult->value.string[aSerializer.offset] = 0;
			mxResult->kind = XS_STRING_KIND;
		}
		c_free(aSerializer.buffer);
	}
	mxCatch(the) {
		if (aSerializer.buffer)
			c_free(aSerializer.buffer);
		fxJump(the);
	}
}

void fxSerializeJSONChar(txMachine* the, txJSONSerializer* theSerializer, char c)
{
	if (theSerializer->offset == theSerializer->size) {
		char* aBuffer;
		theSerializer->size += 1024;
		aBuffer = c_realloc(theSerializer->buffer, theSerializer->size);
		if (!aBuffer)
			fxSerializeJSONError(the, theSerializer, XS_RANGE_ERROR);
		theSerializer->buffer = aBuffer;
	}
	theSerializer->buffer[theSerializer->offset] = c;
	theSerializer->offset++;
}

void fxSerializeJSONChars(txMachine* the, txJSONSerializer* theSerializer, char* s)
{
	txSize aSize = c_strlen(s);
	if ((theSerializer->offset + aSize) >= theSerializer->size) {
		char* aBuffer;
		theSerializer->size += ((aSize / 1024) + 1) * 1024;
		aBuffer = c_realloc(theSerializer->buffer, theSerializer->size);
		if (!theSerializer->buffer)
			fxSerializeJSONError(the, theSerializer, XS_RANGE_ERROR);		
		theSerializer->buffer = aBuffer;
	}
	c_memcpy(theSerializer->buffer + theSerializer->offset, s, aSize);
	theSerializer->offset += aSize;
}

void fxSerializeJSONError(txMachine* the, txJSONSerializer* theSerializer, txError theError)
{
	txSlot* aProperty = the->stack;
	while (aProperty < theSerializer->stack) {
		if (aProperty->kind == XS_REFERENCE_KIND)
			aProperty->value.reference->flag &= ~XS_LEVEL_FLAG;
		aProperty++;
	}
	fxThrowError(the, theError);
}

void fxSerializeJSONIndent(txMachine* the, txJSONSerializer* theSerializer)
{
	txInteger aLevel;
	if (theSerializer->indent[0]) {
		fxSerializeJSONChar(the, theSerializer, '\r');
		for (aLevel = 0; aLevel < theSerializer->level; aLevel++)
			fxSerializeJSONChars(the, theSerializer, theSerializer->indent);
	}
}

void fxSerializeJSONInteger(txMachine* the, txJSONSerializer* theSerializer, txInteger theInteger)
{
	char aBuffer[256];
	fxIntegerToString(theInteger, aBuffer, sizeof(aBuffer));
	fxSerializeJSONChars(the, theSerializer, aBuffer);
}

void fxSerializeJSONName(txMachine* the, txJSONSerializer* theSerializer, txFlag* theFlag)
{
	txSlot* aSlot = the->stack;
	if (*theFlag & 1) {
		fxSerializeJSONChars(the, theSerializer, ",");
		fxSerializeJSONIndent(the, theSerializer);
	}
	else
		*theFlag |= 1;
	if (*theFlag & 2) {
		if (aSlot->kind == XS_INTEGER_KIND) {
			fxSerializeJSONChar(the, theSerializer, '"');
			fxSerializeJSONInteger(the, theSerializer, aSlot->value.integer);
			fxSerializeJSONChar(the, theSerializer, '"');
		}
		else
			fxSerializeJSONString(the, theSerializer, aSlot->value.string);
		fxSerializeJSONChars(the, theSerializer, ":");
	}
	the->stack++; // POP KEY
}

void fxSerializeJSONNumber(txMachine* the, txJSONSerializer* theSerializer, txNumber theNumber)
{
	int fpclass = c_fpclassify(theNumber);
	if ((fpclass != FP_NAN) && (fpclass != FP_INFINITE)) {
		char aBuffer[256];
		fxNumberToString(the, theNumber, aBuffer, sizeof(aBuffer), 0, 0);
		fxSerializeJSONChars(the, theSerializer, aBuffer);
	}
	else
		fxSerializeJSONChars(the, theSerializer, "null");
}

void fxSerializeJSONProperty(txMachine* the, txJSONSerializer* theSerializer, txFlag* theFlag)
{
	txSlot* aWrapper = the->stack + 2;
	txSlot* aValue = the->stack + 1;
	txSlot* aKey = the->stack;
	txSlot* anInstance;
	txSlot* aProperty;
	txFlag aFlag;
	txIndex aLength, anIndex;
	txFlag aMask;
	
	if (mxIsReference(aValue)) {
		anInstance = fxGetInstance(the, aValue);
		if (anInstance->flag & XS_LEVEL_FLAG)
			fxSerializeJSONError(the, theSerializer, XS_TYPE_ERROR);
		aProperty = fxGetProperty(the, anInstance, the->toJSONID);
		if (aProperty && mxIsReference(aProperty) && mxIsFunction(fxGetInstance(the, aProperty)))  {
			*(--the->stack) = *aKey;
			mxInitSlot(--the->stack, XS_INTEGER_KIND);
			the->stack->value.integer = 1;
			/* THIS */
			*(--the->stack) = *aValue;
			/* FUNCTION */
			*(--the->stack) = *aProperty;
			/* RESULT */
			mxZeroSlot(--the->stack);
			fxRunID(the, the->toJSONID);
			*aValue = *(the->stack++);
		}
	}
	else
		anInstance = C_NULL;
	if (theSerializer->replacer) {
		*(--the->stack) = *aKey;
		*(--the->stack) = *aValue;
		mxInitSlot(--the->stack, XS_INTEGER_KIND);
		the->stack->value.integer = 2;
		/* THIS */
		*(--the->stack) = *aWrapper;
		/* FUNCTION */
		*(--the->stack) = *theSerializer->replacer;
		/* RESULT */
		mxZeroSlot(--the->stack);
		fxRunID(the, XS_NO_ID);
		*aValue = *(the->stack++);
	}
again:
	switch (aValue->kind) {
	case XS_NULL_KIND:
		fxSerializeJSONName(the, theSerializer, theFlag);
		fxSerializeJSONChars(the, theSerializer, "null");
		break;
	case XS_BOOLEAN_KIND:
		fxSerializeJSONName(the, theSerializer, theFlag);
		fxSerializeJSONChars(the, theSerializer, aValue->value.boolean ? "true" : "false");
		break;
	case XS_INTEGER_KIND:
		fxSerializeJSONName(the, theSerializer, theFlag);
		fxSerializeJSONInteger(the, theSerializer, aValue->value.integer);
		break;
	case XS_NUMBER_KIND:
		fxSerializeJSONName(the, theSerializer, theFlag);
		fxSerializeJSONNumber(the, theSerializer, aValue->value.number);
		break;
	case XS_STRING_KIND:
		fxSerializeJSONName(the, theSerializer, theFlag);
		fxSerializeJSONString(the, theSerializer, aValue->value.string);
		break;
	case XS_ARRAY_KIND:
		fxSerializeJSONName(the, theSerializer, theFlag);
		anInstance->flag |= XS_LEVEL_FLAG;
		fxSerializeJSONChar(the, theSerializer, '[');
		theSerializer->level++;
		fxSerializeJSONIndent(the, theSerializer);
		aFlag = 4;
		aLength = aValue->value.array.length;
		for (anIndex = 0; anIndex < aLength; anIndex++) {
			aProperty = aValue->value.array.address + anIndex;
			mxInitSlot(--the->stack, aProperty->kind);
			the->stack->value = aProperty->value;
			mxInitSlot(--the->stack, XS_INTEGER_KIND);
			the->stack->value.integer = anIndex;
			fxSerializeJSONProperty(the, theSerializer, &aFlag);
		}
		theSerializer->level--;
		fxSerializeJSONIndent(the, theSerializer);
		fxSerializeJSONChar(the, theSerializer, ']');
		anInstance->flag &= ~XS_LEVEL_FLAG;
		break;
	case XS_REFERENCE_KIND:
	case XS_ALIAS_KIND:
		anInstance = fxGetInstance(the, aValue);
		if (anInstance->flag & XS_VALUE_FLAG) {
			aValue = anInstance->next;
			goto again;
		}
		fxSerializeJSONName(the, theSerializer, theFlag);
		fxSerializeJSONChar(the, theSerializer, '{');
		theSerializer->level++;
		fxSerializeJSONIndent(the, theSerializer);
		aFlag = 2;
		if ((the->frame->flag & XS_SANDBOX_FLAG) || (anInstance->flag & XS_SANDBOX_FLAG)) {
			if (anInstance->flag & XS_SANDBOX_FLAG)
				anInstance = anInstance->value.instance.prototype;
			aMask = XS_DONT_ENUM_FLAG | XS_DONT_SCRIPT_FLAG;
		}
		else
			aMask = XS_DONT_ENUM_FLAG | XS_SANDBOX_FLAG;
		anInstance->flag |= XS_LEVEL_FLAG;
		aProperty = anInstance->next;
		while (aProperty) {
			if (!(aProperty->flag & aMask)) {
				txID anID = aProperty->ID;
				if (anID != XS_NO_ID) {
					mxInitSlot(--the->stack, aProperty->kind);
					the->stack->value = aProperty->value;
					if (anID < 0) {
						txSlot *aSymbol = fxGetSymbol(the, aProperty->ID);
						mxInitSlot(--the->stack, XS_STRING_KIND);
						the->stack->value.string = aSymbol->value.symbol.string;
					}
					else {
						mxInitSlot(--the->stack, XS_INTEGER_KIND);
						the->stack->value.integer = anID;
					}
					fxSerializeJSONProperty(the, theSerializer, &aFlag);
				}
			}
			aProperty = aProperty->next;
		}
		theSerializer->level--;
		fxSerializeJSONIndent(the, theSerializer);
		fxSerializeJSONChar(the, theSerializer, '}');
		anInstance->flag &= ~XS_LEVEL_FLAG;
		break;
	default:
		if (*theFlag & 4) {
			if (*theFlag & 1) {
				fxSerializeJSONChars(the, theSerializer, ",");
				fxSerializeJSONIndent(the, theSerializer);
			}
			else
				*theFlag |= 1;
			fxSerializeJSONChars(the, theSerializer, "null");
		}
		break;
	}
	the->stack++; // POP VALUE
}

void fxSerializeJSONString(txMachine* the, txJSONSerializer* theStream, txString theString)
{
	static char gxDigits[] = "0123456789ABCDEF";
	txU1* aString = (txU1*)theString;
	txU4 aResult;
	txUTF8Sequence *aSequence;
	txInteger aSize;
	char aBuffer[8];
	
	fxSerializeJSONChar(the, theStream, '"');
	aString = (txU1*)theString;
	while ((aResult = *aString++)) {
		for (aSequence = gxUTF8Sequences; aSequence->size; aSequence++) {
			if ((aResult & aSequence->cmask) == aSequence->cval)
				break;
		}
		aSize = aSequence->size - 1;
		while (aSize > 0) {
			aSize--;
			aResult = (aResult << 6) | (*aString++ & 0x3F);
		}
		aResult &= aSequence->lmask;
		switch (aResult) {
		case 8: fxSerializeJSONChars(the, theStream, "\\b"); break;
		case 9: fxSerializeJSONChars(the, theStream, "\\t"); break;
		case 10: fxSerializeJSONChars(the, theStream, "\\n"); break;
		case 12: fxSerializeJSONChars(the, theStream, "\\f"); break;
		case 13: fxSerializeJSONChars(the, theStream, "\\r"); break;
		case 34: fxSerializeJSONChars(the, theStream, "\\\""); break;
		case 92: fxSerializeJSONChars(the, theStream, "\\\\"); break;
		default:
			if ((32 <= aResult) && (aResult < 127))
				fxSerializeJSONChar(the, theStream, (char)aResult);
			else {
				aBuffer[0] = '\\'; 
				aBuffer[1] = 'u';
				aBuffer[2] = gxDigits[(aResult & 0x0000F000) >> 12];
				aBuffer[3] = gxDigits[(aResult & 0x00000F00) >> 8];
				aBuffer[4] = gxDigits[(aResult & 0x000000F0) >> 4];
				aBuffer[5] = gxDigits[(aResult & 0x0000000F)];
				aBuffer[6] = 0;
				fxSerializeJSONChars(the, theStream, aBuffer);
			}
			break;
		}
	}
	fxSerializeJSONChar(the, theStream, '"');
}
