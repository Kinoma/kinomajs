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

static void fxGetNextKeyword(txParser* parser);
static txBoolean fxGetNextIdentiferX(txParser* parser, txU4* value);
static void fxGetNextNumber(txParser* parser, txNumber theNumber);
static void fxGetNextNumberB(txParser* parser);
static void fxGetNextNumberE(txParser* parser, int parseDot);
static void fxGetNextNumberO(txParser* parser, int c);
static void fxGetNextNumberX(txParser* parser);
static void fxGetNextString(txParser* parser, int c);
static txBoolean fxGetNextStringX(txParser* parser, int c, txU4* value);
static txBoolean fxGetNextString0(txParser* parser, int c, txU4* value);
static void fxGetNextTokenAux(txParser* parser);

#define XS_KEYWORD_COUNT 36
static txKeyword gxKeywords[XS_KEYWORD_COUNT] = {
	{ "break", XS_TOKEN_BREAK },
	{ "case", XS_TOKEN_CASE },
	{ "catch", XS_TOKEN_CATCH },
	{ "class", XS_TOKEN_CLASS },
	{ "const", XS_TOKEN_CONST },
	{ "continue", XS_TOKEN_CONTINUE },
	{ "debugger", XS_TOKEN_DEBUGGER },
	{ "default", XS_TOKEN_DEFAULT },
	{ "delete", XS_TOKEN_DELETE },
	{ "do", XS_TOKEN_DO },
	{ "else", XS_TOKEN_ELSE },
	{ "enum", XS_TOKEN_ENUM },
	{ "export", XS_TOKEN_EXPORT },
	{ "extends", XS_TOKEN_EXTENDS },
	{ "false", XS_TOKEN_FALSE },
	{ "finally", XS_TOKEN_FINALLY },
	{ "for", XS_TOKEN_FOR },
	{ "function", XS_TOKEN_FUNCTION },
	{ "if", XS_TOKEN_IF },
	{ "import", XS_TOKEN_IMPORT },
	{ "in", XS_TOKEN_IN },
	{ "instanceof", XS_TOKEN_INSTANCEOF },
	{ "new", XS_TOKEN_NEW }, 
	{ "null", XS_TOKEN_NULL }, 
	{ "return", XS_TOKEN_RETURN },
	{ "super", XS_TOKEN_SUPER },
	{ "switch", XS_TOKEN_SWITCH },
	{ "this", XS_TOKEN_THIS },
	{ "throw", XS_TOKEN_THROW },
	{ "true", XS_TOKEN_TRUE },
	{ "try", XS_TOKEN_TRY },
	{ "typeof", XS_TOKEN_TYPEOF },
	{ "var", XS_TOKEN_VAR },
	{ "void", XS_TOKEN_VOID },
	{ "while", XS_TOKEN_WHILE },
	{ "with", XS_TOKEN_WITH }
};

#define XS_STRICT_KEYWORD_COUNT 9
static txKeyword gxStrictKeywords[XS_STRICT_KEYWORD_COUNT] = {
	{ "implements", XS_TOKEN_IMPLEMENTS },
	{ "interface", XS_TOKEN_INTERFACE },
	{ "let", XS_TOKEN_LET },
	{ "package", XS_TOKEN_PACKAGE },
	{ "private", XS_TOKEN_PRIVATE },
	{ "protected", XS_TOKEN_PROTECTED },
	{ "public", XS_TOKEN_PUBLIC },
	{ "static", XS_TOKEN_STATIC },
	{ "yield", XS_TOKEN_YIELD }
};

void fxGetNextCharacter(txParser* parser)
{
	txU4 aResult;
	txUTF8Sequence const *aSequence = NULL;
	txInteger aSize;

	aResult = (txU4)(*(parser->getter))(parser->stream);
	if (aResult & 0x80) {  // According to UTF-8, aResult should be 1xxx xxxx when it is not a ASCII
	if (aResult != (txU4)C_EOF) {
		for (aSequence = gxUTF8Sequences; aSequence->size; aSequence++) {
			if ((aResult & aSequence->cmask) == aSequence->cval)
				break;
		}
		if (aSequence->size == 0) {
			fxReportParserWarning(parser, "invalid UTF-8 character %d", aResult);
			aResult = (txU4)C_EOF;
		}
		else {
			aSize = aSequence->size - 1;
			while (aSize) {
				aSize--;
				aResult = (aResult << 6) | ((*(parser->getter))(parser->stream) & 0x3F);
			}
			aResult &= aSequence->lmask;
		}
	}
	}
	parser->character = aResult;
}

void fxGetNextKeyword(txParser* parser)
{
	int low, high, anIndex, aDelta;
	
	parser->symbol2 = fxNewParserSymbol(parser, parser->buffer);
	if (parser->token != XS_TOKEN_DOT) {
		for (low = 0, high = XS_KEYWORD_COUNT; high > low;
				(aDelta < 0) ? (low = anIndex + 1) : (high = anIndex)) {
			anIndex = low + ((high - low) / 2);
			aDelta = c_strcmp(gxKeywords[anIndex].text, parser->buffer);
			if (aDelta == 0) {
				parser->token2 = gxKeywords[anIndex].token;
				return;
			}
		}
		if ((parser->flags & mxStrictFlag)) {
			for (low = 0, high = XS_STRICT_KEYWORD_COUNT; high > low;
					(aDelta < 0) ? (low = anIndex + 1) : (high = anIndex)) {
				anIndex = low + ((high - low) / 2);
				aDelta = c_strcmp(gxStrictKeywords[anIndex].text, parser->buffer);
				if (aDelta == 0) {
					parser->token2 = gxStrictKeywords[anIndex].token;
					return;
				}
			}
		}
		if ((parser->flags & mxGeneratorFlag)) {
			if (c_strcmp("yield", parser->buffer) == 0) {
				parser->token2 = XS_TOKEN_YIELD;
				return;
			}
		}	
	}	
	parser->token2 = XS_TOKEN_IDENTIFIER;
}

txBoolean fxGetNextIdentiferX(txParser* parser, txU4* value)
{
	fxGetNextCharacter(parser);
	if (parser->character == 'u') {
		fxGetNextCharacter(parser);
		if (parser->character == '{') {
			fxGetNextCharacter(parser);
			while (fxGetNextStringX(parser, parser->character, value)) {
				fxGetNextCharacter(parser);
			}
			if (parser->character == '}') {
				fxGetNextCharacter(parser);
				return 1;
			}
		}
		else {
			if (fxGetNextStringX(parser, parser->character, value)) {
				fxGetNextCharacter(parser);
				if (fxGetNextStringX(parser, parser->character, value)) {
					fxGetNextCharacter(parser);
					if (fxGetNextStringX(parser, parser->character, value)) {
						fxGetNextCharacter(parser);
						if (fxGetNextStringX(parser, parser->character, value)) {
							fxGetNextCharacter(parser);
							return 1;
						}		
					}		
				}		
			}		
		}		
	}
	return 0;
}

void fxGetNextNumber(txParser* parser, txNumber theNumber)
{
	parser->number2 = theNumber;
	parser->integer2 = (txInteger)parser->number2;
	theNumber = parser->integer2;
	if (parser->number2 == theNumber)
		parser->token2 = XS_TOKEN_INTEGER;
	else
		parser->token2 = XS_TOKEN_NUMBER;
}

void fxGetNextNumberB(txParser* parser)
{
	txNumber aNumber = 0;
	int c;
	for (;;) {
		fxGetNextCharacter(parser);
		c = parser->character;
		if (('0' <= c) && (c <= '1'))
			aNumber = (aNumber * 2) + (c - '0');
		else
			break;
	}
	fxGetNextNumber(parser, aNumber);
}

void fxGetNextNumberE(txParser* parser, int parseDot)
{
	txString p = parser->buffer;
	if (parser->character == '-') {
		*p++ = (char)parser->character;
		fxGetNextCharacter(parser);
	}
	if (!parseDot)
		*p++ = '.';
	while (('0' <= parser->character) && (parser->character <= '9')) {
		*p++ = (char)parser->character;
		fxGetNextCharacter(parser);
	}
	if (parseDot) {
		if (parser->character == '.') {
			*p++ = (char)parser->character;
			fxGetNextCharacter(parser);
			while (('0' <= parser->character) && (parser->character <= '9')) {
				*p++ = (char)parser->character;
				fxGetNextCharacter(parser);
			}
		}
		else
			*p++ = '.';
	}
	if ((parser->character == 'e') || (parser->character == 'E')) {
		*p++ = '0';
		*p++ = (char)parser->character;
		fxGetNextCharacter(parser);
		if ((parser->character == '+') || (parser->character == '-')) {
			*p++ = (char)parser->character;
			fxGetNextCharacter(parser);
		}
		while (('0' <= parser->character) && (parser->character <= '9')) {
			*p++ = (char)parser->character;
			fxGetNextCharacter(parser);
		}
	}
	*p++ = 0;
	fxGetNextNumber(parser, c_strtod(parser->buffer, C_NULL));
}

void fxGetNextNumberO(txParser* parser, int c)
{
	txNumber aNumber = c - '0';
	for (;;) {
		fxGetNextCharacter(parser);
		c = parser->character;
		if (('0' <= c) && (c <= '7'))
			aNumber = (aNumber * 8) + (c - '0');
		else
			break;
	}
	fxGetNextNumber(parser, aNumber);
}

void fxGetNextNumberX(txParser* parser)
{
	txNumber aNumber = 0;
	int c;
	for (;;) {
		fxGetNextCharacter(parser);
		c = parser->character;
		if (('0' <= c) && (c <= '9'))
			aNumber = (aNumber * 16) + (c - '0');
		else if (('a' <= c) && (c <= 'f'))
			aNumber = (aNumber * 16) + (10 + c - 'a');
		else if (('A' <= c) && (c <= 'F'))
			aNumber = (aNumber * 16) + (10 + c - 'A');
		else
			break;
	}
	fxGetNextNumber(parser, aNumber);
}

void fxGetNextString(txParser* parser, int c)
{
	txString p = parser->buffer;
	txString q = p + sizeof(parser->buffer) - 1;
	txString r, s;
	char character;
	txU4 t;
	for (;;) {
		if (parser->character == (txU4)C_EOF) {
			fxReportParserError(parser, "end of file in string");			
			break;
		}
		else if (parser->character == 10) {
			parser->line2++;
			if (c == '`') {
				p = (txString)fsX2UTF8(10, (txU1*)p, q - p);
				fxGetNextCharacter(parser);
			}
			else {
				fxReportParserError(parser, "end of line in string");			
				break;
			}
		}
		else if (parser->character == 13) {
			parser->line2++;
			if (c == '`') {
				p = (txString)fsX2UTF8(10, (txU1*)p, q - p);
				fxGetNextCharacter(parser);
				if (parser->character == 10)
					fxGetNextCharacter(parser);
			}
			else {
				fxReportParserError(parser, "end of line in string");			
				break;
			}
		}
		else if (parser->character == (txU4)c) {
			break;
		}
		else if (parser->character == '$') {
			fxGetNextCharacter(parser);
			if ((c == '`') && (parser->character == '{'))
				break;
			p = (txString)fsX2UTF8('$', (txU1*)p, q - p);
		}
		else if (parser->character == '\\') {
			parser->escaped2 = 1;
			p = (txString)fsX2UTF8('\\', (txU1*)p, q - p);
			fxGetNextCharacter(parser);
			switch (parser->character) {
			case 10:
				parser->line2++;
				p = (txString)fsX2UTF8(10, (txU1*)p, q - p);
				fxGetNextCharacter(parser);
				break;
			case 13:
				parser->line2++;
				p = (txString)fsX2UTF8(10, (txU1*)p, q - p);
				fxGetNextCharacter(parser);
				if (parser->character == 10)
					fxGetNextCharacter(parser);
				break;
			default:
				p = (txString)fsX2UTF8(parser->character, (txU1*)p, q - p);
				fxGetNextCharacter(parser);
				break;
			}	
		}	
		else {
			p = (txString)fsX2UTF8(parser->character, (txU1*)p, q - p);
			fxGetNextCharacter(parser);
		}
	}	
	*p = 0;
	if (p == q)
		fxReportParserWarning(parser, "string overflow");	
	parser->rawLength2 = p - parser->buffer;
	parser->raw2 = fxNewParserString(parser, parser->buffer, parser->rawLength2);
	if (parser->escaped2) {
		p = parser->buffer;
		q = p + sizeof(parser->buffer) - 1;	
		s = parser->raw2;
		character = *s++;
		while (character) {
			if (character == '\\') {
				character = *s++;
				switch (character) {
				case 10:
					if (c == '`')
						*p++ = 10;
					character = *s++;
					break;
				case 'b':
					*p++ = '\b';
					character = *s++;
					break;
				case 'f':
					*p++ = '\f';
					character = *s++;
					break;
				case 'n':
					*p++ = '\n';
					character = *s++;
					break;
				case 'r':
					*p++ = '\r';
					character = *s++;
					break;
				case 't':
					*p++ = '\t';
					character = *s++;
					break;
				case 'u':
					r = p;
					t = 0;
					*p++ = 'u';
					character = *s++;
					if (character == '{') {
						*p++ = character;
						character = *s++;
						while (fxGetNextStringX(parser, character, &t)) {
							*p++ = character;
							character = *s++;
						}
						if (character == '}') {
							p = (txString)fsX2UTF8(t, (txU1*)r, q - r);
							character = *s++;
						}
					}
					else {
						if (fxGetNextStringX(parser, character, &t)) {
							*p++ = character;
							character = *s++;
							if (fxGetNextStringX(parser, character, &t)) {
								*p++ = character;
								character = *s++;
								if (fxGetNextStringX(parser, character, &t)) {
									*p++ = character;
									character = *s++;
									if (fxGetNextStringX(parser, character, &t)) {
										p = (txString)fsX2UTF8(t, (txU1*)r, q - r);
										character = *s++;
									}		
								}		
							}		
						}		
					}		
					break;
				case 'v':
					*p++ = '\v';
					character = *s++;
					break;
				case 'x':
					r = p;
					t = 0;
					*p++ = 'x';
					character = *s++;
					if (fxGetNextStringX(parser, character, &t)) {
						*p++ = character;
						character = *s++;
						if (fxGetNextStringX(parser, character, &t)) {
							p = (txString)fsX2UTF8(t, (txU1*)r, q - r);
							character = *s++;
						}		
					}		
					break;
				case '0':
				case '1':
				case '2':
				case '3':
				case '4':
				case '5':
				case '6':
				case '7':
					r = p;
					t = character - '0';
					*p++ = character;
					character = *s++;
					while (fxGetNextString0(parser, character, &t)) {
						*p++ = character;
						character = *s++;
					}
					p = (txString)fsX2UTF8(t, (txU1*)r, q - r);
					if ((parser->flags & mxStrictFlag))
						fxReportParserError(parser, "octal escape sequence (strict mode)");			
					break;
				default:
					*p++ = character;
					character = *s++;
					break;
				}
			}
			else {
				*p++ = character;
				character = *s++;
			}
		}
		*p = 0;
		parser->stringLength2 = p - parser->buffer;
		parser->string2 = fxNewParserString(parser, parser->buffer, parser->stringLength2);
	}
	else {
		parser->stringLength2 = parser->rawLength2;
		parser->string2 = parser->raw2;
	}
}

txBoolean fxGetNextStringX(txParser* parser, int c, txU4* value)
{
	if (('0' <= c) && (c <= '9'))
		*value = (*value * 16) + (c - '0');
	else if (('a' <= c) && (c <= 'f'))
		*value = (*value * 16) + (10 + c - 'a');
	else if (('A' <= c) && (c <= 'F'))
		*value = (*value * 16) + (10 + c - 'A');
	else
		return 0;
	return 1;
}

txBoolean fxGetNextString0(txParser* parser, int c, txU4* value)
{
	if (('0' <= c) && (c <= '7'))
		*value = (*value * 8) + (c - '0');
	else
		return 0;
	return 1;
}

void fxGetNextToken(txParser* parser)
{
	if (!parser->ahead)
		fxGetNextTokenAux(parser);
	parser->line = parser->line2;
	parser->crlf = parser->crlf2;
	parser->escaped = parser->escaped2;
	parser->integer = parser->integer2;
	parser->modifierLength = parser->modifierLength2;
	parser->modifier = parser->modifier2;
	parser->number = parser->number2;
	parser->rawLength = parser->rawLength2;
	parser->raw = parser->raw2;
	parser->stringLength = parser->stringLength2;
	parser->string = parser->string2;
	parser->symbol = parser->symbol2;
	parser->token = parser->token2;
	parser->ahead = 0;
}

void fxGetNextToken2(txParser* parser)
{
	if (!parser->ahead)
		fxGetNextTokenAux(parser);
	parser->ahead = 1;
}

void fxGetNextTokenAux(txParser* parser)
{
	int c;
	txString p;
	txString q;
	txU4 t = 0;
	parser->crlf2 = 0;
	parser->escaped2 = 0;
	parser->integer2 = 0;
	parser->modifierLength2 = 0;
	parser->modifier2 = parser->emptyString;
	parser->number2 = 0;
	parser->rawLength2 = 0;
	parser->raw2 = parser->emptyString;
	parser->stringLength2 = 0;
	parser->string2 = parser->emptyString;
	parser->symbol2 = C_NULL;
	parser->token2 = XS_NO_TOKEN;
	while (parser->token2 == XS_NO_TOKEN) {
		switch (parser->character) {
		case C_EOF:
			parser->token2 = XS_TOKEN_EOF;
			break;
		case 10:	
			parser->line2++;
			fxGetNextCharacter(parser);
			parser->crlf2 = 1;
		#ifdef mxColor
			parser->startOffset2 = parser->offset;
		#endif
			break;
		case 13:	
			parser->line2++;
			fxGetNextCharacter(parser);
			if (parser->character == 10)
				fxGetNextCharacter(parser);
			parser->crlf2 = 1;
		#ifdef mxColor
			parser->startOffset2 = parser->offset;
		#endif
			break;
			
		case 11:
		case 12:
		case 160:
		case ' ':
		case '\t':
			fxGetNextCharacter(parser);
		#ifdef mxColor
			parser->startOffset2 = parser->offset;
		#endif
			break;
			
		case '0':
			fxGetNextCharacter(parser);
			c = parser->character;
			if (c == '.') {
				fxGetNextCharacter(parser);
				c = parser->character;
				if ((('0' <= c) && (c <= '9')) || (c == 'e') || (c == 'E'))
					fxGetNextNumberE(parser, 0);
				else {
					parser->number2 = 0;
					parser->token2 = XS_TOKEN_NUMBER;
				}
			}
			else if ((c == 'b') || (c == 'B')) {
				fxGetNextNumberB(parser);
			}
			else if ((c == 'e') || (c == 'E')) {
				fxGetNextNumberE(parser, 0);
			}
			else if ((c == 'o') || (c == 'O')) {
				fxGetNextNumberO(parser, '0');
			}
			else if ((c == 'x') || (c == 'X')) {
				fxGetNextNumberX(parser);
			}
			else if (('0' <= c) && (c <= '7')) {
				if ((parser->flags & mxStrictFlag))
					fxReportParserError(parser, "octal number (strict mode)");			
				fxGetNextNumberO(parser, c);
			}
			else {
				parser->integer2 = 0;
				parser->token2 = XS_TOKEN_INTEGER;
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
			fxGetNextNumberE(parser, 1);
			break;
		case '.':
			fxGetNextCharacter(parser);
			if (parser->character == '.') {	
				fxGetNextCharacter(parser);
				if (parser->character == '.') {	
					parser->token2 = XS_TOKEN_SPREAD;
					fxGetNextCharacter(parser);
				}
				else {
					fxReportParserError(parser, "invalid character %d", parser->character);
				}		
			}		
			else if (('0' <= parser->character) && (parser->character <= '9'))
				fxGetNextNumberE(parser, 0);
			else
				parser->token2 = XS_TOKEN_DOT;
			break;	
		case ',':
			parser->token2 = XS_TOKEN_COMMA;
			fxGetNextCharacter(parser);
			break;	
		case ';':
			parser->token2 = XS_TOKEN_SEMICOLON;
			fxGetNextCharacter(parser);
			break;	
		case ':':
			parser->token2 = XS_TOKEN_COLON;
			fxGetNextCharacter(parser);
			break;	
		case '?':
			parser->token2 = XS_TOKEN_QUESTION_MARK;
			fxGetNextCharacter(parser);
			break;	
		case '(':
			parser->token2 = XS_TOKEN_LEFT_PARENTHESIS;
			fxGetNextCharacter(parser);
			break;	
		case ')':
			parser->token2 = XS_TOKEN_RIGHT_PARENTHESIS;
			fxGetNextCharacter(parser);
			break;	
		case '[':
			parser->token2 = XS_TOKEN_LEFT_BRACKET;
			fxGetNextCharacter(parser);
			break;	
		case ']':
			parser->token2 = XS_TOKEN_RIGHT_BRACKET;
			fxGetNextCharacter(parser);
			break;	
		case '{':
			parser->token2 = XS_TOKEN_LEFT_BRACE;
			fxGetNextCharacter(parser);
			break;	
		case '}':
			parser->token2 = XS_TOKEN_RIGHT_BRACE;
			fxGetNextCharacter(parser);
			break;	
		case '=':
			fxGetNextCharacter(parser);
			if (parser->character == '=') {			
				fxGetNextCharacter(parser);
				if (parser->character == '=') {
					parser->token2 = XS_TOKEN_STRICT_EQUAL;
					fxGetNextCharacter(parser);
				}
				else
					parser->token2 = XS_TOKEN_EQUAL;
			}
			else if (parser->character == '>') {	
				parser->token2 = XS_TOKEN_ARROW;
				fxGetNextCharacter(parser);
			}
			else	
				parser->token2 = XS_TOKEN_ASSIGN;
			break;
		case '<':
			fxGetNextCharacter(parser);
			if (parser->character == '<') {
				fxGetNextCharacter(parser);
				if (parser->character == '=') {
					parser->token2 = XS_TOKEN_LEFT_SHIFT_ASSIGN;
					fxGetNextCharacter(parser);
				}
				else
					parser->token2 = XS_TOKEN_LEFT_SHIFT;
			}
			else  if (parser->character == '=') {
				parser->token2 = XS_TOKEN_LESS_EQUAL;
				fxGetNextCharacter(parser);
			}
			else
				parser->token2 = XS_TOKEN_LESS;
			break;	
		case '>':
			fxGetNextCharacter(parser);
			if (parser->character == '>') {			
				fxGetNextCharacter(parser);
				if (parser->character == '>') {			
					fxGetNextCharacter(parser);
					if (parser->character == '=') {
						parser->token2 = XS_TOKEN_UNSIGNED_RIGHT_SHIFT_ASSIGN;
						fxGetNextCharacter(parser);
					}
					else
						parser->token2 = XS_TOKEN_UNSIGNED_RIGHT_SHIFT;
				}
				else if (parser->character == '=') {
					parser->token2 = XS_TOKEN_SIGNED_RIGHT_SHIFT_ASSIGN;
					fxGetNextCharacter(parser);
				}
				else
					parser->token2 = XS_TOKEN_SIGNED_RIGHT_SHIFT;
			}
			else if (parser->character == '=') {
				parser->token2 = XS_TOKEN_MORE_EQUAL;
				fxGetNextCharacter(parser);
			}
			else
				parser->token2 = XS_TOKEN_MORE;
			break;	
		case '!':
			fxGetNextCharacter(parser);
			if (parser->character == '=') {			
				fxGetNextCharacter(parser);
				if (parser->character == '=') {
					parser->token2 = XS_TOKEN_STRICT_NOT_EQUAL;
					fxGetNextCharacter(parser);
				}
				else
					parser->token2 = XS_TOKEN_NOT_EQUAL;
			}
			else
				parser->token2 = XS_TOKEN_NOT;
			break;
		case '~':
			parser->token2 = XS_TOKEN_BIT_NOT;
			fxGetNextCharacter(parser);
			break;
		case '&':
			fxGetNextCharacter(parser);
			if (parser->character == '=') {	
				parser->token2 = XS_TOKEN_BIT_AND_ASSIGN;
				fxGetNextCharacter(parser);
			}
			else if (parser->character == '&') {
				parser->token2 = XS_TOKEN_AND;
				fxGetNextCharacter(parser);
			}
			else
				parser->token2 = XS_TOKEN_BIT_AND;
			break;
		case '|':
			fxGetNextCharacter(parser);
			if (parser->character == '=') {
				parser->token2 = XS_TOKEN_BIT_OR_ASSIGN;
				fxGetNextCharacter(parser);
			}
			else if (parser->character == '|') {
				parser->token2 = XS_TOKEN_OR;
				fxGetNextCharacter(parser);
			}
			else
				parser->token2 = XS_TOKEN_BIT_OR;
			break;
		case '^':
			fxGetNextCharacter(parser);
			if (parser->character == '=') {
				parser->token2 = XS_TOKEN_BIT_XOR_ASSIGN;
				fxGetNextCharacter(parser);
			}
			else
				parser->token2 = XS_TOKEN_BIT_XOR;
			break;
		case '+':	
			fxGetNextCharacter(parser);
			if (parser->character == '=') {
				parser->token2 = XS_TOKEN_ADD_ASSIGN;
				fxGetNextCharacter(parser);
			}
			else if (parser->character == '+') {
				parser->token2 = XS_TOKEN_INCREMENT;
				fxGetNextCharacter(parser);
			}
			else
				parser->token2 = XS_TOKEN_ADD;
			break;
		case '-':	
			fxGetNextCharacter(parser);
			if (parser->character == '=') {
				parser->token2 = XS_TOKEN_SUBTRACT_ASSIGN;
				fxGetNextCharacter(parser);
			}
			else if (parser->character == '-') {
				parser->token2 = XS_TOKEN_DECREMENT;
				fxGetNextCharacter(parser);
			}
			else
				parser->token2 = XS_TOKEN_SUBTRACT;
			break;
		case '*':	
			fxGetNextCharacter(parser);
			if (parser->character == '=') {
				parser->token2 = XS_TOKEN_MULTIPLY_ASSIGN;
				fxGetNextCharacter(parser);
			}
			else
				parser->token2 = XS_TOKEN_MULTIPLY;
			break;
		case '/':
			fxGetNextCharacter(parser);
			if (parser->character == '*') {
				fxGetNextCharacter(parser);
				for (;;) {
					if (parser->character == (txU4)C_EOF)
						break;
					else if (parser->character == 10) {
						parser->line2++;
						fxGetNextCharacter(parser);
					}
					else if (parser->character == 13) {
						parser->line2++;
						fxGetNextCharacter(parser);
						if (parser->character == 10)
							fxGetNextCharacter(parser);
					}
					else if (parser->character == '*') {
						fxGetNextCharacter(parser);
						if (parser->character == '/') {
							fxGetNextCharacter(parser);
							break;
						}
					}
					else
						fxGetNextCharacter(parser);
				}
			}
			else if (parser->character == '/') {
				fxGetNextCharacter(parser);
				p = parser->buffer;
				q = p + sizeof(parser->buffer) - 1;
				while ((parser->character != (txU4)C_EOF) && (parser->character != 10) && (parser->character != 13)) {
					if (p < q)
						*p++ = (char)parser->character;
					fxGetNextCharacter(parser);
				}	
				*p = 0;
				p = parser->buffer;
				if (!c_strcmp(p, "@module")) {
					if (parser->token2 == XS_NO_TOKEN)
						parser->flags |= mxCommonModuleFlag;
				}
				else if (!c_strcmp(p, "@program")) {
					if (parser->token2 == XS_NO_TOKEN)
						parser->flags |= mxCommonProgramFlag;
				}
				else if (parser->flags & mxDebugFlag) {
					if (!c_strncmp(p, "@line ", 6)) {
						p += 6;
						t = 0;
						c = *p++;
						while (('0' <= c) && (c <= '9')) {
							t = (t * 10) + (c - '0');
							c = *p++;
						}
						if (!t) goto bail;
						if (c == ' ') {
							c = *p++;
							if (c != '"') goto bail;
							q = p;
							c = *q++;
							while ((c != 0) && (c != 10) && (c != 13) && (c != '"'))
								c = *q++;
							if (c != '"') goto bail;
							*(--q) = 0;
							parser->path = fxNewParserSymbol(parser, p);
						}
						parser->line2 = t - 1;
					}
					else if (!c_strncmp(p, "# sourceMappingURL=", 19) || !c_strncmp(p, "@ sourceMappingURL=", 19)) {
						p += 19;
						q = p;
						c = *q++;
						while ((c != 0) && (c != 10) && (c != 13))
							c = *q++;
						*q = 0;
						parser->name = fxNewParserString(parser, p, q - p);
					}
				}
			bail:
				;
			}
			else if ((parser->crlf2) || (fxAcceptRegExp(parser))) {
				parser->token2 = XS_TOKEN_NULL;
				p = parser->buffer;
				q = p + sizeof(parser->buffer) - 1;
				for (;;) {
					if (p == q) {
						fxReportParserWarning(parser, "regular expression overflow");			
						break;
					}
					else if (parser->character == (txU4)C_EOF) {
						fxReportParserWarning(parser, "end of file in regular expression");			
						break;
					}
					else if ((parser->character == 10) || (parser->character == 13)) {
						fxReportParserWarning(parser, "end of line in regular expression");			
						break;
					}
					else if (parser->character == '/') {
						*p = 0;
						parser->stringLength2 = p - parser->buffer;
						parser->string2 = fxNewParserString(parser, parser->buffer, parser->stringLength2);
						parser->token2 = XS_TOKEN_REGEXP;
						p = parser->buffer;
						q = p + sizeof(parser->buffer) - 1;
						for (;;) {
							fxGetNextCharacter(parser);
							if (p == q) {
								fxReportParserWarning(parser, "regular expression overflow");			
								break;
							}
							else if (fxIsIdentifierNext((char)parser->character))
								*p++ = (char)parser->character;
							else {
								if (p != parser->buffer) {
									*p = 0;
									parser->modifierLength2 = p - parser->buffer;
									parser->modifier2 = fxNewParserString(parser, parser->buffer, parser->modifierLength2);
								}
								break;
							}
						}
						break;
					}
					else if (parser->character == '\\') {
						*p++ = (char)parser->character;
                        fxGetNextCharacter(parser);
                        if (p == q) {
                            fxReportParserWarning(parser, "regular expression overflow");			
                            break;
                        }
					}
                    p = (txString)fsX2UTF8(parser->character, (txU1*)p, q - p);
                    //*p++ = parser->character;
                    fxGetNextCharacter(parser);
				}
			}
			else if (parser->character == '=') {
				parser->token2 = XS_TOKEN_DIVIDE_ASSIGN;
				fxGetNextCharacter(parser);
			}
			else 
				parser->token2 = XS_TOKEN_DIVIDE;
			break;
		case '%':	
			fxGetNextCharacter(parser);
			if (parser->character == '=') {
				parser->token2 = XS_TOKEN_MODULO_ASSIGN;
				fxGetNextCharacter(parser);
			}
			else
				parser->token2 = XS_TOKEN_MODULO;
			break;
		
		case '"':
		case '\'':
			c = parser->character;
			fxGetNextCharacter(parser);
			fxGetNextString(parser, c);
			parser->token2 = XS_TOKEN_STRING;
			fxGetNextCharacter(parser);
			break;
			
		case '`':
			fxGetNextCharacter(parser);
			fxGetNextString(parser, '`');
			if (parser->character == '{')
				parser->token2 = XS_TOKEN_TEMPLATE_HEAD;
			else
				parser->token2 = XS_TOKEN_TEMPLATE;
			fxGetNextCharacter(parser);
			break;
			
		case '@':
			if (parser->flags & mxCFlag)
				parser->token2 = XS_TOKEN_HOST;
            else
                fxReportParserError(parser, "invalid character @");
            fxGetNextCharacter(parser);
			break;
			
		default:
			p = parser->buffer;
			q = p + sizeof(parser->buffer) - 1;
			if (fxIsIdentifierFirst((char)parser->character)) {
				*p++ = (char)parser->character;
				fxGetNextCharacter(parser);
			}
			else if (parser->character == '\\') {
				t = 0;
				if (fxGetNextIdentiferX(parser, &t))
					p = (txString)fsX2UTF8(t, (txU1*)p, q - p);				
				else
					p = C_NULL;
			}
			else
				p = C_NULL;
			if (p) {
				for (;;) {
					if (p == q) {
						fxReportParserWarning(parser, "identifier overflow");			
						break;
					}
					if (fxIsIdentifierNext((char)parser->character)) {
						*p++ = (char)parser->character;
						fxGetNextCharacter(parser);
					}
					else if (parser->character == '\\') {
						t = 0;
						if (fxGetNextIdentiferX(parser, &t))
							p = (txString)fsX2UTF8(t, (txU1*)p, q - p);				
						else {
							p = C_NULL;
							break;
						}
					}
					else {
						*p = 0;
						fxGetNextKeyword(parser);
						break;
					}
				}
			}
			if (!p) {
				fxReportParserWarning(parser, "invalid character %d", parser->character);
				fxGetNextCharacter(parser);
			}
			break;
		}
	}
}

void fxGetNextTokenTemplate(txParser* parser)
{
	parser->crlf2 = 0;
	parser->escaped2 = 0;
	parser->integer2 = 0;
	parser->modifierLength2 = 0;
	parser->modifier2 = parser->emptyString;
	parser->number2 = 0;
	parser->rawLength2 = 0;
	parser->raw2 = parser->emptyString;
	parser->stringLength2 = 0;
	parser->string2 = parser->emptyString;
	parser->symbol2 = C_NULL;
	parser->token2 = XS_NO_TOKEN;
	fxGetNextString(parser, '`');
	if (parser->character == '{')
		parser->token2 = XS_TOKEN_TEMPLATE_MIDDLE;
	else
		parser->token2 = XS_TOKEN_TEMPLATE_TAIL;
	fxGetNextCharacter(parser);
	parser->line = parser->line2;
	parser->crlf = parser->crlf2;
	parser->escaped = parser->escaped2;
	parser->integer = parser->integer2;
	parser->modifierLength = parser->modifierLength2;
	parser->modifier = parser->modifier2;
	parser->number = parser->number2;
	parser->rawLength = parser->rawLength2;
	parser->raw = parser->raw2;
	parser->stringLength = parser->stringLength2;
	parser->string = parser->string2;
	parser->symbol = parser->symbol2;
	parser->token = parser->token2;
	parser->ahead = 0;
}

void fxGetNextTokenJSON(txParser* parser)
{
	int c;
	txString p;
	txString q;

	parser->line = parser->line2;
	
	parser->crlf = parser->crlf2;
	parser->escaped = parser->escaped2;
	parser->integer = parser->integer2;
	parser->modifierLength = parser->modifierLength2;
	parser->modifier = parser->modifier2;
	parser->number = parser->number2;
	parser->rawLength = parser->rawLength2;
	parser->raw = parser->raw2;
	parser->stringLength = parser->stringLength2;
	parser->string = parser->string2;
	parser->symbol = parser->symbol2;
	parser->token = parser->token2;
	
	parser->crlf2 = 0;
	parser->escaped2 = 0;
	parser->integer2 = 0;
	parser->modifierLength2 = 0;
	parser->modifier2 = parser->emptyString;
	parser->number2 = 0;
	parser->rawLength2 = 0;
	parser->raw2 = parser->emptyString;
	parser->stringLength2 = 0;
	parser->string2 = parser->emptyString;
	parser->symbol2 = C_NULL;
	parser->token2 = XS_NO_TOKEN;
	
	while (parser->token2 == XS_NO_TOKEN) {
		switch (parser->character) {
		case C_EOF:
			parser->token2 = XS_TOKEN_EOF;
			break;
		case 10:	
			parser->line2++;
			fxGetNextCharacter(parser);
			parser->crlf2 = 1;
			break;
		case 13:	
			parser->line2++;
			fxGetNextCharacter(parser);
			if (parser->character == 10)
				fxGetNextCharacter(parser);
			parser->crlf2 = 1;
			break;
			
		case '\t':
		case ' ':
			fxGetNextCharacter(parser);
			break;
			
		case '0':
			fxGetNextCharacter(parser);
			c = parser->character;
			if (c == '.') {
				fxGetNextCharacter(parser);
				c = parser->character;
				if ((('0' <= c) && (c <= '9')) || (c == 'e') || (c == 'E'))
					fxGetNextNumberE(parser, 0);
				else {
					parser->number2 = 0;
					parser->token2 = XS_TOKEN_NUMBER;
				}
			}
			else if ((c == 'b') || (c == 'B')) {
				fxGetNextNumberB(parser);
			}
			else if ((c == 'e') || (c == 'E')) {
				fxGetNextNumberE(parser, 0);
			}
			else if ((c == 'o') || (c == 'O')) {
				fxGetNextNumberO(parser, '0');
			}
			else if ((c == 'x') || (c == 'X')) {
				fxGetNextNumberX(parser);
			}
			else {
				parser->integer2 = 0;
				parser->token2 = XS_TOKEN_INTEGER;
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
			fxGetNextNumberE(parser, 1);
			break;
		case ',':
			parser->token2 = XS_TOKEN_COMMA;
			fxGetNextCharacter(parser);
			break;	
		case ':':
			parser->token2 = XS_TOKEN_COLON;
			fxGetNextCharacter(parser);
			break;	
		case '[':
			parser->token2 = XS_TOKEN_LEFT_BRACKET;
			fxGetNextCharacter(parser);
			break;	
		case ']':
			parser->token2 = XS_TOKEN_RIGHT_BRACKET;
			fxGetNextCharacter(parser);
			break;	
		case '{':
			parser->token2 = XS_TOKEN_LEFT_BRACE;
			fxGetNextCharacter(parser);
			break;	
		case '}':
			parser->token2 = XS_TOKEN_RIGHT_BRACE;
			fxGetNextCharacter(parser);
			break;	
		case '"':
			fxGetNextCharacter(parser);
			fxGetNextString(parser, '"');
			parser->token2 = XS_TOKEN_STRING;
			fxGetNextCharacter(parser);
			break;
		default:
			if (fxIsIdentifierFirst((char)parser->character)) {
				p = parser->buffer;
				q = p + sizeof(parser->buffer) - 1;
				for (;;) {
					if (p == q) {
						fxReportParserError(parser, "identifier overflow");			
						break;
					}
					*p++ = (char)parser->character;
					fxGetNextCharacter(parser);
					if (!fxIsIdentifierNext((char)parser->character))
						break;
				}
				*p = 0;
				if (!c_strcmp("false", parser->buffer)) 
					parser->token2 = XS_TOKEN_FALSE;
				else if (!c_strcmp("null", parser->buffer)) 
					parser->token2 = XS_TOKEN_NULL;
				else if (!c_strcmp("true", parser->buffer)) 
					parser->token2 = XS_TOKEN_TRUE;
				else {
					parser->symbol2 = fxNewParserSymbol(parser, parser->buffer);
					parser->token2 = XS_TOKEN_IDENTIFIER;
				}
			}
			else {
				fxReportParserWarning(parser, "invalid character %d", parser->character);
				fxGetNextCharacter(parser);
			}
			break;
		}
	}
}

