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
#include "xs6All.h"

enum {
	XS_NO_JSON_TOKEN,
	XS_JSON_TOKEN_COLON,
	XS_JSON_TOKEN_COMMA,
	XS_JSON_TOKEN_EOF,
	XS_JSON_TOKEN_FALSE,
	XS_JSON_TOKEN_IDENTIFIER,
	XS_JSON_TOKEN_INTEGER,
	XS_JSON_TOKEN_LEFT_BRACE,
	XS_JSON_TOKEN_LEFT_BRACKET,
	XS_JSON_TOKEN_NULL,
	XS_JSON_TOKEN_NUMBER,
	XS_JSON_TOKEN_RIGHT_BRACE,
	XS_JSON_TOKEN_RIGHT_BRACKET,
	XS_JSON_TOKEN_STRING,
	XS_JSON_TOKEN_TRUE,
};

typedef struct {
	char text[30];
	txInteger token;
} txJSONKeyword;

typedef struct {
	txSlot* reviver;
	void* stream;
	txGetter getter;
	txSlot* path;
	int character;

	int line;
	int crlf;
	int escaped;
	txSlot* flags;
	txInteger integer;
	txNumber number;
	txSlot* string;
	txSlot* symbol;
	txInteger token;

	int line2;
	int crlf2;
	int escaped2;
	txSlot* flags2;
	txInteger integer2;
	txNumber number2;
	txSlot* string2;
	txSlot* symbol2;
	txInteger token2;
	
	int errorCount;
	
	char buffer[32 * 1024];			// must be last
} txJSONParser;

typedef struct {
	txString buffer;
	char indent[16];
	txInteger level;
	txSize offset;
	txSize size;
	txSlot* replacer;
	txSlot* stack;
} txJSONSerializer;

static void fx_JSON_parse(txMachine* the);
static void fxGetNextJSONCharacter(txMachine* the, txJSONParser* theParser);
static void fxGetNextJSONKeyword(txMachine* the, txJSONParser* theParser);
static void fxGetNextJSONNumber(txMachine* the, txJSONParser* theParser, int parseDot);
static void fxGetNextJSONToken(txMachine* the, txJSONParser* theParser);
static void fxParseJSON(txMachine* the, txJSONParser* theParser);
static void fxParseJSONValue(txMachine* the, txJSONParser* theParser);
static void fxParseJSONObject(txMachine* the, txJSONParser* theParser);
static void fxParseJSONArray(txMachine* the, txJSONParser* theParser);

static void fx_JSON_stringify(txMachine* the);
static void fxSerializeJSON(txMachine* the, txJSONSerializer* theSerializer);
static void fxSerializeJSONChar(txMachine* the, txJSONSerializer* theSerializer, char c);
static void fxSerializeJSONChars(txMachine* the, txJSONSerializer* theSerializer, char* s);
static void fxSerializeJSONIndent(txMachine* the, txJSONSerializer* theSerializer);
static void fxSerializeJSONInteger(txMachine* the, txJSONSerializer* theSerializer, txInteger theInteger);
static void fxSerializeJSONName(txMachine* the, txJSONSerializer* theSerializer, txInteger* theFlag);
static void fxSerializeJSONNumber(txMachine* the, txJSONSerializer* theSerializer, txNumber theNumber);
static void fxSerializeJSONOwnProperty(txMachine* the, txSlot* theContext, txInteger theID, txSlot* theProperty);
static void fxSerializeJSONProperty(txMachine* the, txJSONSerializer* theSerializer, txInteger* theFlag);
static void fxSerializeJSONString(txMachine* the, txJSONSerializer* theStream, txString theString);

#define XS_JSON_KEYWORD_COUNT 3
static const txJSONKeyword gxJSONKeywords[XS_JSON_KEYWORD_COUNT] = {
	{ "false", XS_JSON_TOKEN_FALSE },
	{ "null", XS_JSON_TOKEN_NULL },
	{ "true", XS_JSON_TOKEN_TRUE }
};

void fxBuildJSON(txMachine* the)
{
    static const txHostFunctionBuilder gx_JSON_builders[] = {
		{ fx_JSON_parse, 2, _parse },
		{ fx_JSON_stringify, 3, _stringify },
		{ C_NULL, 0, 0 },
    };
    const txHostFunctionBuilder* builder;
	txSlot* slot;
	mxPush(mxObjectPrototype);
	slot = fxLastProperty(the, fxNewObjectInstance(the));
	for (builder = gx_JSON_builders; builder->callback; builder++)
		slot = fxNextHostFunctionProperty(the, slot, builder->callback, builder->length, mxID(builder->id), XS_DONT_ENUM_FLAG);
	slot = fxNextStringProperty(the, slot, "JSON", mxID(_Symbol_toStringTag), XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	slot = fxSetGlobalProperty(the, mxGlobal.value.reference, mxID(_JSON), C_NULL);
	slot->flag = XS_GET_ONLY;
	slot->kind = the->stack->kind;
	slot->value = the->stack->value;
	the->stack++;
}

void fx_JSON_parse(txMachine* the)
{
	volatile txJSONParser* aParser = C_NULL;
	txSlot* slot;
	txStringCStream aCStream;
	txStringStream aStream;

	mxTry(the) {
		if (mxArgc < 1)
			mxSyntaxError("no buffer");
		aParser = c_malloc(sizeof(txJSONParser));
		if (NULL == aParser)
			mxUnknownError("out of memory");
		c_memset((txJSONParser*)aParser, 0, sizeof(txJSONParser) - sizeof(aParser->buffer));
		if (mxArgc > 1) {
			slot = mxArgv(1);
			if (slot->kind == XS_REFERENCE_KIND) {
				slot = slot->value.reference;
				if (slot->next && ((slot->next->kind == XS_CODE_KIND) || (slot->next->kind == XS_CODE_X_KIND) || (slot->next->kind == XS_CALLBACK_KIND)))
					aParser->reviver = slot;
			}
		}
		slot = mxArgv(0);
		if (slot->kind == XS_REFERENCE_KIND) {
			slot = slot->value.reference;
			if (slot->flag & XS_VALUE_FLAG) {
				slot = slot->next;
				if (slot->kind == XS_HOST_KIND) {
					aCStream.buffer = slot->value.host.data;
					mxPushSlot(mxArgv(0));
					fxGetID(the, mxID(_length));
					aCStream.size = fxToInteger(the, the->stack++);
					aCStream.offset = 0;
					aParser->stream = &aCStream;
					aParser->getter = fxStringCGetter;
				}
			}
		}
		if (!aParser->stream) {
			fxToString(the, mxArgv(0));
			aStream.slot = mxArgv(0);
			aStream.size = c_strlen(aStream.slot->value.string);
			aStream.offset = 0;
			aParser->stream = &aStream;
			aParser->getter = fxStringGetter;
		}
		fxParseJSON(the, (txJSONParser*)aParser);
		mxPullSlot(mxResult);
		c_free((txJSONParser*)aParser);
		// @@ reviver
	}
	mxCatch(the) {
		if (aParser)
			c_free((txJSONParser*)aParser);
		fxJump(the);
	}
}

void fxGetNextJSONCharacter(txMachine* the, txJSONParser* theParser)
{
	txU4 aResult;
	const txUTF8Sequence *aSequence = NULL;
	txInteger aSize;

	aResult = (txU4)(*(theParser->getter))(theParser->stream);
	if (aResult & 0x80) {  // According to UTF-8, aResult should be 1xxx xxxx when it is not a ASCII
	if (aResult != (txU4)C_EOF) {
		for (aSequence = gxUTF8Sequences; aSequence->size; aSequence++) {
			if ((aResult & aSequence->cmask) == aSequence->cval)
				break;
		}
		if (aSequence->size == 0) {
			mxSyntaxError("invalid UTF-8 character %d", aResult);
			aResult = C_EOF;
		}
		else {
			aSize = aSequence->size - 1;
			while (aSize) {
				aSize--;
				aResult = (aResult << 6) | ((*(theParser->getter))(theParser->stream) & 0x3F);
			}
			aResult &= aSequence->lmask;
		}
	}
	}
	theParser->character = aResult;
#ifdef mxColor
	theParser->offset += theParser->size;
	if (aSequence)
		theParser->size = aSequence->size;
	else
		theParser->size = (aResult != C_EOF)? 1 : 0 /* C_EOF */;
#endif
}

void fxGetNextJSONKeyword(txMachine* the, txJSONParser* theParser)
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
	theParser->symbol2 = fxNewNameC(the, theParser->buffer);
	theParser->token2 = XS_JSON_TOKEN_IDENTIFIER;
}

void fxGetNextJSONNumber(txMachine* the, txJSONParser* theParser, int parseDot)
{
	txNumber aNumber;
	txString p = theParser->buffer;
	if (theParser->character == '-') {
		*p++ = theParser->character;
		fxGetNextJSONCharacter(the, theParser);
	}
	if (!parseDot)
		*p++ = '.';
	while (('0' <= theParser->character) && (theParser->character <= '9')) {
		*p++ = theParser->character;
		fxGetNextJSONCharacter(the, theParser);
	}
	if (parseDot) {
		if (theParser->character == '.') {
			*p++ = theParser->character;
			fxGetNextJSONCharacter(the, theParser);
			while (('0' <= theParser->character) && (theParser->character <= '9')) {
				*p++ = theParser->character;
				fxGetNextJSONCharacter(the, theParser);
			}
		}
		else
			*p++ = '.';
	}
	if ((theParser->character == 'e') || (theParser->character == 'E')) {
		*p++ = '0';
		*p++ = theParser->character;
		fxGetNextJSONCharacter(the, theParser);
		if ((theParser->character == '+') || (theParser->character == '-')) {
			*p++ = theParser->character;
			fxGetNextJSONCharacter(the, theParser);
		}
		while (('0' <= theParser->character) && (theParser->character <= '9')) {
			*p++ = theParser->character;
			fxGetNextJSONCharacter(the, theParser);
		}
	}
	*p++ = 0;
	theParser->number2 = fxStringToNumber(the->dtoa, theParser->buffer, 1);
	theParser->integer2 = (txInteger)theParser->number2;
	aNumber = theParser->integer2;
	if (theParser->number2 == aNumber)
		theParser->token2 = XS_JSON_TOKEN_INTEGER;
	else
		theParser->token2 = XS_JSON_TOKEN_NUMBER;
}

void fxGetNextJSONToken(txMachine* the, txJSONParser* theParser)
{
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
	theParser->token2 = XS_NO_JSON_TOKEN;
	while (theParser->token2 == XS_NO_JSON_TOKEN) {
		switch (theParser->character) {
		case C_EOF:
			theParser->token2 = XS_JSON_TOKEN_EOF;
			break;
		case 10:	
			theParser->line2++;
			fxGetNextJSONCharacter(the, theParser);
			theParser->crlf2 = 1;
			break;
		case 13:	
			theParser->line2++;
			fxGetNextJSONCharacter(the, theParser);
			if (theParser->character == 10)
				fxGetNextJSONCharacter(the, theParser);
			theParser->crlf2 = 1;
			break;
			
		case '\t':
		case ' ':
			fxGetNextJSONCharacter(the, theParser);
			break;
			
		case '0':
			fxGetNextJSONCharacter(the, theParser);
			c = theParser->character;
			if (c == '.') {
				fxGetNextJSONCharacter(the, theParser);
				c = theParser->character;
				if ((('0' <= c) && (c <= '9')) || (c == 'e') || (c == 'E'))
					fxGetNextJSONNumber(the, theParser, 0);
				else {
					theParser->number2 = 0;
					theParser->token2 = XS_JSON_TOKEN_NUMBER;
				}
			}
			else if ((c == 'e') || (c == 'E')) {
				fxGetNextJSONNumber(the, theParser, 0);
			}
			else if ((c == 'x') || (c == 'X')) {
				p = theParser->buffer;
				*p++ = '0';
				*p++ = 'x';
				for (;;) {
					fxGetNextJSONCharacter(the, theParser);
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
				theParser->number2 = fxStringToNumber(the->dtoa, theParser->buffer, 1);
				theParser->integer2 = (txInteger)theParser->number2;
				aNumber = theParser->integer2;
				if (theParser->number2 == aNumber)
					theParser->token2 = XS_JSON_TOKEN_INTEGER;
				else
					theParser->token2 = XS_JSON_TOKEN_NUMBER;
			}
			else {
				theParser->integer2 = 0;
				theParser->token2 = XS_JSON_TOKEN_INTEGER;
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
			fxGetNextJSONNumber(the, theParser, 1);
			break;
		case ',':
			theParser->token2 = XS_JSON_TOKEN_COMMA;
			fxGetNextJSONCharacter(the, theParser);
			break;	
		case ':':
			theParser->token2 = XS_JSON_TOKEN_COLON;
			fxGetNextJSONCharacter(the, theParser);
			break;	
		case '[':
			theParser->token2 = XS_JSON_TOKEN_LEFT_BRACKET;
			fxGetNextJSONCharacter(the, theParser);
			break;	
		case ']':
			theParser->token2 = XS_JSON_TOKEN_RIGHT_BRACKET;
			fxGetNextJSONCharacter(the, theParser);
			break;	
		case '{':
			theParser->token2 = XS_JSON_TOKEN_LEFT_BRACE;
			fxGetNextJSONCharacter(the, theParser);
			break;	
		case '}':
			theParser->token2 = XS_JSON_TOKEN_RIGHT_BRACE;
			fxGetNextJSONCharacter(the, theParser);
			break;	
		case '"':
			p = theParser->buffer;
			q = p + sizeof(theParser->buffer) - 1;
			r = C_NULL;
			fxGetNextJSONCharacter(the, theParser);
			for (;;) {
				if (theParser->character == C_EOF) {
					mxSyntaxError("end of file in string");			
					break;
				}
				else if ((theParser->character == 10) || (theParser->character == 13)) {
					mxSyntaxError("end of line in string");			
					break;
				}
				else if ((0 <= theParser->character) && (theParser->character < 32)) {
					mxSyntaxError("invalid character in string");			
					break;
				}
				else if (theParser->character == '"') {
					fxGetNextJSONCharacter(the, theParser);
					break;
				}
				else if (theParser->character == '"') {
					fxGetNextJSONCharacter(the, theParser);
					break;
				}
				else if (theParser->character == '\\') {
					theParser->escaped2 = 1;
					r = C_NULL;
					fxGetNextJSONCharacter(the, theParser);
					switch (theParser->character) {
					case 10:
						theParser->line2++;
						fxGetNextJSONCharacter(the, theParser);
						break;
					case 13:
						theParser->line2++;
						fxGetNextJSONCharacter(the, theParser);
						if (theParser->character == 10)
							fxGetNextJSONCharacter(the, theParser);
						break;
					case '\'':
						if (p < q) *p++ = '\'';
						fxGetNextJSONCharacter(the, theParser);
						break;
					case '"':
						if (p < q) *p++ = '"';
						fxGetNextJSONCharacter(the, theParser);
						break;
					case '\\':
						if (p < q) *p++ = '\\';
						fxGetNextJSONCharacter(the, theParser);
						break;
					case '0':
						if (p < q) *p++ = 0;
						fxGetNextJSONCharacter(the, theParser);
						break;
					case 'b':
						if (p < q) *p++ = '\b';
						fxGetNextJSONCharacter(the, theParser);
						break;
					case 'f':
						if (p < q) *p++ = '\f';
						fxGetNextJSONCharacter(the, theParser);
						break;
					case 'n':
						if (p < q) *p++ = '\n';
						fxGetNextJSONCharacter(the, theParser);
						break;
					case 'r':
						if (p < q) *p++ = '\r';
						fxGetNextJSONCharacter(the, theParser);
						break;
					case 't':
						if (p < q) *p++ = '\t';
						fxGetNextJSONCharacter(the, theParser);
						break;
					case 'u':
						r = p;
						t = 5;
						if (p < q) *p++ = 'u';
						fxGetNextJSONCharacter(the, theParser);
						break;
					case 'v':
						if (p < q) *p++ = '\v';
						fxGetNextJSONCharacter(the, theParser);
						break;
					case 'x':
						r = p;
						t = 3;
						if (p < q) *p++ = 'x';
						fxGetNextJSONCharacter(the, theParser);
						break;
					default:
						p = (txString)fsX2UTF8(theParser->character, (txU1*)p, q - p);
						fxGetNextJSONCharacter(the, theParser);
						break;
					}
				}
				else {
					p = (txString)fsX2UTF8(theParser->character, (txU1*)p, q - p);
					if (r) {
						if ((txU4)(p - r) > t)
							r = C_NULL;
						else if (((txU4)(p - r) == t) && (p < q)) {
							*p = 0;
							t = c_strtoul(r + 1, &s, 16);
							if (!*s)
								p = (txString)fsX2UTF8(t, (txU1*)r, q - r);
							r = C_NULL;
						}
					}
					fxGetNextJSONCharacter(the, theParser);
				}
			}
			*p = 0;
			if (p == q)
				mxSyntaxError("string overflow");			
			fxCopyStringC(the, theParser->string2, theParser->buffer);
			theParser->token2 = XS_JSON_TOKEN_STRING;
			break;
		default:
			if (fxIsIdentifierFirst(theParser->character)) {
				p = theParser->buffer;
				q = p + sizeof(theParser->buffer) - 1;
				for (;;) {
					if (p == q) {
						mxSyntaxError("identifier overflow");			
						break;
					}
					*p++ = theParser->character;
					fxGetNextJSONCharacter(the, theParser);
					if (!fxIsIdentifierNext(theParser->character))
						break;
				}
				*p = 0;
				fxGetNextJSONKeyword(the, theParser);
			}
			else {
				mxSyntaxError("invalid character %d", theParser->character);
				fxGetNextJSONCharacter(the, theParser);
			}
			break;
		}
	}
}

void fxParseJSON(txMachine* the, txJSONParser* theParser)
{
	theParser->line2 = 0;
	mxPush(mxEmptyString);
	theParser->flags = the->stack;
	mxPush(mxEmptyString);
	theParser->string = the->stack;
	mxPush(mxEmptyString);
	theParser->flags2 = the->stack;
	mxPush(mxEmptyString);
	theParser->string2 = the->stack;
	fxGetNextJSONCharacter(the, theParser);
	fxGetNextJSONToken(the, theParser);
	fxGetNextJSONToken(the, theParser);
	fxParseJSONValue(the, theParser);
	if (theParser->token != XS_JSON_TOKEN_EOF)
		mxSyntaxError("missing EOF");
}

void fxParseJSONValue(txMachine* the, txJSONParser* theParser)
{
	switch (theParser->token) {
	case XS_JSON_TOKEN_FALSE:
		mxPushBoolean(0);
		fxGetNextJSONToken(the, theParser);
		break;
	case XS_JSON_TOKEN_TRUE:
		mxPushBoolean(1);
		fxGetNextJSONToken(the, theParser);
		break;
	case XS_JSON_TOKEN_NULL:
		mxPushNull();
		fxGetNextJSONToken(the, theParser);
		break;
	case XS_JSON_TOKEN_INTEGER:
		mxPushInteger(theParser->integer);
		fxGetNextJSONToken(the, theParser);
		break;
	case XS_JSON_TOKEN_NUMBER:
		mxPushNumber(theParser->number);
		fxGetNextJSONToken(the, theParser);
		break;
	case XS_JSON_TOKEN_STRING:
		mxPushString(theParser->string->value.string);
		fxGetNextJSONToken(the, theParser);
		break;
	case XS_JSON_TOKEN_LEFT_BRACE:
		fxParseJSONObject(the, theParser);
		break;
	case XS_JSON_TOKEN_LEFT_BRACKET:
		fxParseJSONArray(the, theParser);
		break;
	default:
		mxPushUndefined();
		mxSyntaxError("invalid value");
		break;
	}
}

void fxParseJSONObject(txMachine* the, txJSONParser* theParser)
{
	txSlot* anObject;
	txSlot* aProperty;
	txInteger anID;

	fxGetNextJSONToken(the, theParser);
	mxPush(mxObjectPrototype);
	anObject = fxNewObjectInstance(the);
	for (;;) {
		if (theParser->token == XS_JSON_TOKEN_RIGHT_BRACE)
			break;
		if (theParser->token != XS_JSON_TOKEN_STRING) {
			mxSyntaxError("missing name");
			break;
		}
		mxPushString(theParser->string->value.string);
		fxSlotToID(the, the->stack, &anID);
		if (!theParser->reviver)
			the->stack++;
		fxGetNextJSONToken(the, theParser);
		if (theParser->token != XS_JSON_TOKEN_COLON) {
			mxSyntaxError("%ld: missing :", theParser->line);
			break;
		}
		fxGetNextJSONToken(the, theParser);
		fxParseJSONValue(the, theParser);
		if (theParser->reviver) {
			mxPushInteger(2);
			mxPushReference(anObject);
			mxPushReference(theParser->reviver);
			fxCall(the);
		}
		if (the->stack->kind != XS_UNDEFINED_KIND) {
			aProperty = fxSetProperty(the, anObject, anID, C_NULL);
			aProperty->kind = the->stack->kind;
			aProperty->value = the->stack->value;
		}
		the->stack++;
		if (theParser->token != XS_JSON_TOKEN_COMMA)
			break;
		fxGetNextJSONToken(the, theParser);
	}
	if (theParser->token != XS_JSON_TOKEN_RIGHT_BRACE)
		mxSyntaxError("missing }");
	fxGetNextJSONToken(the, theParser);
}

void fxParseJSONArray(txMachine* the, txJSONParser* theParser)
{
	txSlot* anArray;
	txIndex aLength;
	txSlot* anItem;

	fxGetNextJSONToken(the, theParser);
	mxPush(mxArrayPrototype);
	anArray = fxNewArrayInstance(the);
	aLength = 0;
	anItem = anArray->next;
	for (;;) {
		if (theParser->token == XS_JSON_TOKEN_RIGHT_BRACKET)
			break;
		fxParseJSONValue(the, theParser);
		aLength++;
		anItem->next = fxNewSlot(the);
		anItem = anItem->next;
		anItem->kind = the->stack->kind;
		anItem->value = the->stack->value;
		the->stack++;
		if (theParser->token != XS_JSON_TOKEN_COMMA)
			break;
		fxGetNextJSONToken(the, theParser);
	}
	anArray->next->value.array.length = aLength;
	fxCacheArray(the, anArray);
	if (theParser->token != XS_JSON_TOKEN_RIGHT_BRACKET)
		mxSyntaxError("missing ]");
	fxGetNextJSONToken(the, theParser);
}

void fx_JSON_stringify(txMachine* the)
{
	volatile txJSONSerializer aSerializer;
	mxTry(the) {
		c_memset((txJSONSerializer*)&aSerializer, 0, sizeof(aSerializer));
		fxSerializeJSON(the, (txJSONSerializer*)&aSerializer);
		if (aSerializer.offset) {
			fxSerializeJSONChar(the, (txJSONSerializer*)&aSerializer, 0);
			mxResult->value.string = (txString)fxNewChunk(the, aSerializer.offset);
			c_memcpy(mxResult->value.string, aSerializer.buffer, aSerializer.offset);
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

void fxSerializeJSON(txMachine* the, txJSONSerializer* theSerializer)
{
	txSlot* aSlot;
	txInteger aFlag;
	
	aSlot = fxGetInstance(the, mxThis);
	theSerializer->offset = 0;
	theSerializer->size = 1024;
	theSerializer->buffer = c_malloc(1024);
	if (!theSerializer->buffer)
		mxUnknownError("out of memory");

	if (mxArgc > 1) {
		aSlot = mxArgv(1);
		if (mxIsReference(aSlot)) {
			aSlot = fxGetInstance(the, aSlot);
			if (mxIsFunction(aSlot))
				theSerializer->replacer = mxArgv(1);
			else if (mxIsArray(aSlot))
				mxSyntaxError("not yet implememented");
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
				theSerializer->indent[anIndex] = ' ';
		}
		else if (mxIsStringPrimitive(aSlot))
			c_strncpy((char *)theSerializer->indent, aSlot->value.string, 10);
	}

	theSerializer->stack = the->stack;
	mxPush(mxObjectPrototype);
	fxNewObjectInstance(the);
	aFlag = 0;
	if (mxArgc > 0)
		mxPushSlot(mxArgv(0));
	else
		mxPushUndefined();
	mxPush(mxEmptyString);
	fxSerializeJSONProperty(the, theSerializer, &aFlag);
	the->stack++;
}

void fxSerializeJSONChar(txMachine* the, txJSONSerializer* theSerializer, char c)
{
    //fprintf(stderr, "%c", c);
	if (theSerializer->offset == theSerializer->size) {
		char* aBuffer;
		theSerializer->size += 1024;
		aBuffer = c_realloc(theSerializer->buffer, theSerializer->size);
		if (!aBuffer)
			mxUnknownError("out of memory");
		theSerializer->buffer = aBuffer;
	}
	theSerializer->buffer[theSerializer->offset] = c;
	theSerializer->offset++;
}

void fxSerializeJSONChars(txMachine* the, txJSONSerializer* theSerializer, char* s)
{
    //fprintf(stderr, "%s", s);
	txSize aSize = c_strlen(s);
	if ((theSerializer->offset + aSize) >= theSerializer->size) {
		char* aBuffer;
		theSerializer->size += ((aSize / 1024) + 1) * 1024;
		aBuffer = c_realloc(theSerializer->buffer, theSerializer->size);
		if (!aBuffer)
			mxUnknownError("out of memory");
		theSerializer->buffer = aBuffer;
	}
	c_memcpy(theSerializer->buffer + theSerializer->offset, s, aSize);
	theSerializer->offset += aSize;
}

void fxSerializeJSONIndent(txMachine* the, txJSONSerializer* theSerializer)
{
	txInteger aLevel;
	if (theSerializer->indent[0]) {
		fxSerializeJSONChar(the, theSerializer, '\n');
		for (aLevel = 0; aLevel < theSerializer->level; aLevel++)
			fxSerializeJSONChars(the, theSerializer, theSerializer->indent);
	}
}

void fxSerializeJSONInteger(txMachine* the, txJSONSerializer* theSerializer, txInteger theInteger)
{
	char aBuffer[256];
	fxIntegerToString(the->dtoa, theInteger, aBuffer, sizeof(aBuffer));
	fxSerializeJSONChars(the, theSerializer, aBuffer);
}

void fxSerializeJSONName(txMachine* the, txJSONSerializer* theSerializer, txInteger* theFlag)
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
		fxNumberToString(the->dtoa, theNumber, aBuffer, sizeof(aBuffer), 0, 0);
		fxSerializeJSONChars(the, theSerializer, aBuffer);
	}
	else
		fxSerializeJSONChars(the, theSerializer, "null");
}

void fxSerializeJSONOwnProperty(txMachine* the, txSlot* theContext, txInteger theID, txSlot* theProperty) 
{
	txJSONSerializer* theSerializer = theContext->value.regexp.code;
	txInteger* theFlag = theContext->value.regexp.offsets;
	if (theID < 0) {
		txSlot* key = fxGetKey(the, (txID)theID);
		if (key && (key->flag & XS_DONT_ENUM_FLAG)) {
			if (key->kind == XS_KEY_KIND) {
				mxPushSlot(theProperty);
				mxPushString(key->value.key.string);
				fxSerializeJSONProperty(the, theSerializer, theFlag);
			}
			else {
				mxPushSlot(theProperty);
				mxPushStringX(key->value.key.string);
				fxSerializeJSONProperty(the, theSerializer, theFlag);
			}
		}
	}
	else {
		mxPushSlot(theProperty);
		mxPushInteger(theID);
		fxSerializeJSONProperty(the, theSerializer, theFlag);
	}
}

void fxSerializeJSONProperty(txMachine* the, txJSONSerializer* theSerializer, txInteger* theFlag)
{
	txSlot* aWrapper = the->stack + 2;
	txSlot* aValue = the->stack + 1;
	txSlot* aKey = the->stack;
	txSlot* anInstance;
	txSlot* aProperty;
	txInteger aFlag;
	txIndex aLength, anIndex;
	
	if (mxIsReference(aValue)) {
		anInstance = fxGetInstance(the, aValue);
		if (anInstance->flag & XS_LEVEL_FLAG)
			mxTypeError("cyclic value");
		aProperty = fxGetProperty(the, anInstance, mxID(_toJSON));
		if (aProperty && mxIsReference(aProperty) && mxIsFunction(fxGetInstance(the, aProperty)))  {
			mxPushSlot(aKey);
			/* COUNT */
			mxPushInteger(1);
			/* THIS */
			mxPushSlot(aValue);
			/* FUNCTION */
			mxPushSlot(aProperty);
			fxCall(the);
			mxPullSlot(aValue);
		}
	}
	else
		anInstance = C_NULL;
	if (theSerializer->replacer) {
		mxPushSlot(aKey);
		mxPushSlot(aValue);
		/* COUNT */
		mxPushInteger(2);
		/* THIS */
		mxPushSlot(aWrapper);
		/* FUNCTION */
		mxPushSlot(theSerializer->replacer);
		fxCall(the);
		mxPullSlot(aValue);
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
	case XS_STRING_X_KIND:
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
			mxPushSlot(aProperty);
			mxPushInteger(anIndex);
			fxSerializeJSONProperty(the, theSerializer, &aFlag);
		}
		theSerializer->level--;
		fxSerializeJSONIndent(the, theSerializer);
		fxSerializeJSONChar(the, theSerializer, ']');
		anInstance->flag &= ~XS_LEVEL_FLAG;
		break;
	case XS_REFERENCE_KIND:
		anInstance = fxGetInstance(the, aValue);
		if (anInstance->flag & XS_VALUE_FLAG) {
			aValue = anInstance->next;
			if (aValue->kind != XS_PROXY_KIND)
				goto again;
		}
		fxSerializeJSONName(the, theSerializer, theFlag);
		anInstance->flag |= XS_LEVEL_FLAG;
		fxSerializeJSONChar(the, theSerializer, '{');
		theSerializer->level++;
		fxSerializeJSONIndent(the, theSerializer);
		aFlag = 2;
		{
			txSlot aContext;
			aContext.value.regexp.code = theSerializer;
			aContext.value.regexp.offsets = &aFlag;
			fxEachInstanceProperty(the, anInstance, XS_EACH_ENUMERABLE_FLAG | XS_EACH_STRING_FLAG, fxSerializeJSONOwnProperty, &aContext, anInstance);
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
	const txUTF8Sequence *aSequence;
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
