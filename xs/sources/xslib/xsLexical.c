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
 
#define XS_KEYWORD_COUNT 36
static txScriptKeyword gxKeywords[XS_KEYWORD_COUNT] = {
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
static txScriptKeyword gxStrictKeywords[XS_STRICT_KEYWORD_COUNT] = {
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

void fxGetNextCharacter(txScriptParser* theParser)
{
	txU4 aResult;
	txUTF8Sequence *aSequence = NULL;
	txInteger aSize;

	aResult = (txU4)(*(theParser->getter))(theParser->stream);
	if (aResult & 0x80) {  // According to UTF-8, aResult should be 1xxx xxxx when it is not a ASCII
	if (aResult != (txU4)C_EOF) {
		for (aSequence = gxUTF8Sequences; aSequence->size; aSequence++) {
			if ((aResult & aSequence->cmask) == aSequence->cval)
				break;
		}
		if (aSequence->size == 0) {
			fxReportParserWarning(theParser, "invalid UTF-8 character %d", aResult);
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

void fxGetNextKeyword(txScriptParser* theParser)
{
	int low, high, anIndex, aDelta;
	
	for (low = 0, high = XS_KEYWORD_COUNT; high > low;
			(aDelta < 0) ? (low = anIndex + 1) : (high = anIndex)) {
		anIndex = low + ((high - low) / 2);
		aDelta = c_strcmp(gxKeywords[anIndex].text, theParser->buffer);
		if (aDelta == 0) {
			theParser->token2 = gxKeywords[anIndex].token;
			return;
		}
	}
	if (theParser->strict) {
		for (low = 0, high = XS_STRICT_KEYWORD_COUNT; high > low;
				(aDelta < 0) ? (low = anIndex + 1) : (high = anIndex)) {
			anIndex = low + ((high - low) / 2);
			aDelta = c_strcmp(gxStrictKeywords[anIndex].text, theParser->buffer);
			if (aDelta == 0) {
				theParser->token2 = gxStrictKeywords[anIndex].token;
				return;
			}
		}
	}
	theParser->symbol2 = fxNewSymbolC(theParser->the, theParser->buffer);
	theParser->token2 = XS_TOKEN_IDENTIFIER;
}

void fxGetNextNumber(txScriptParser* theParser, int parseDot)
{
	txNumber aNumber;
	txString p = theParser->buffer;
	if (theParser->character == '-') {
		*p++ = theParser->character;
		fxGetNextCharacter(theParser);
	}
	if (!parseDot)
		*p++ = '.';
	while (('0' <= theParser->character) && (theParser->character <= '9')) {
		*p++ = theParser->character;
		fxGetNextCharacter(theParser);
	}
	if (parseDot) {
		if (theParser->character == '.') {
			*p++ = theParser->character;
			fxGetNextCharacter(theParser);
			while (('0' <= theParser->character) && (theParser->character <= '9')) {
				*p++ = theParser->character;
				fxGetNextCharacter(theParser);
			}
		}
		else
			*p++ = '.';
	}
	if ((theParser->character == 'e') || (theParser->character == 'E')) {
		*p++ = '0';
		*p++ = theParser->character;
		fxGetNextCharacter(theParser);
		if ((theParser->character == '+') || (theParser->character == '-')) {
			*p++ = theParser->character;
			fxGetNextCharacter(theParser);
		}
		while (('0' <= theParser->character) && (theParser->character <= '9')) {
			*p++ = theParser->character;
			fxGetNextCharacter(theParser);
		}
	}
	*p++ = 0;
	theParser->number2 = fxStringToNumberCompile(theParser->the, theParser->buffer, 1);
	theParser->integer2 = (txInteger)theParser->number2;
	aNumber = theParser->integer2;
	if (theParser->number2 == aNumber)
		theParser->token2 = XS_TOKEN_INTEGER;
	else
		theParser->token2 = XS_TOKEN_NUMBER;
}

void fxGetNextToken(txScriptParser* theParser)
{
	txMachine* the = theParser->the;
	int c;
	txString p;
	txString q;
	txString r;
	txString s;
	txU4 t = 0;
	txNumber aNumber;
#ifdef mxColor
	char aBuffer[256];
#endif
	theParser->line = theParser->line2;
	theParser->crlf = theParser->crlf2;
	theParser->escaped = theParser->escaped2;
	theParser->flags->value.string = theParser->flags2->value.string;
	theParser->integer = theParser->integer2;
	theParser->number = theParser->number2;
	theParser->string->value.string = theParser->string2->value.string;
#ifdef mxColor
	theParser->startOffset = theParser->startOffset2;
	theParser->stopOffset = theParser->stopOffset2;
#endif
	theParser->symbol = theParser->symbol2;
	theParser->token = theParser->token2;
	
	theParser->crlf2 = 0;
	theParser->escaped2 = 0;
	theParser->flags2->value.string = mxEmptyString.value.string;
	theParser->integer2 = 0;
	theParser->number2 = 0;
	theParser->string2->value.string = mxEmptyString.value.string;
	theParser->symbol2 = C_NULL;
#ifdef mxColor
	theParser->startOffset2 = theParser->offset;
#endif
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
		#ifdef mxColor
			theParser->startOffset2 = theParser->offset;
		#endif
			break;
		case 13:	
			theParser->line2++;
			fxGetNextCharacter(theParser);
			if (theParser->character == 10)
				fxGetNextCharacter(theParser);
			theParser->crlf2 = 1;
		#ifdef mxColor
			theParser->startOffset2 = theParser->offset;
		#endif
			break;
			
		case 11:
		case 12:
		case 160:
		case ' ':
		case '\t':
			fxGetNextCharacter(theParser);
		#ifdef mxColor
			theParser->startOffset2 = theParser->offset;
		#endif
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
			else if (('0' <= c) && (c <= '9')) {
				if (theParser->strict)
					fxReportParserWarning(theParser, "octal number (strict mode)");			
				p = theParser->buffer;
				*p++ = '0';
				*p++ = c;
				for (;;) {
					fxGetNextCharacter(theParser);
					c = theParser->character;
					if (('0' <= c) && (c <= '9'))
						*p++ = c;
					else
						break;
				}
				*p = 0;
				theParser->number2 = fxStringToNumberCompile(theParser->the, theParser->buffer, 1);
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
			fxGetNextNumber(theParser, 1);
			break;
		case '.':
			fxGetNextCharacter(theParser);
			if (('0' <= theParser->character) && (theParser->character <= '9'))
				fxGetNextNumber(theParser, 0);
			else
				theParser->token2 = XS_TOKEN_DOT;
			break;	
		case ',':
			theParser->token2 = XS_TOKEN_COMMA;
			fxGetNextCharacter(theParser);
			break;	
		case ';':
			theParser->token2 = XS_TOKEN_SEMICOLON;
			fxGetNextCharacter(theParser);
			break;	
		case ':':
			theParser->token2 = XS_TOKEN_COLON;
			fxGetNextCharacter(theParser);
			break;	
		case '?':
			theParser->token2 = XS_TOKEN_QUESTION_MARK;
			fxGetNextCharacter(theParser);
			break;	
		case '(':
			theParser->token2 = XS_TOKEN_LEFT_PARENTHESIS;
			fxGetNextCharacter(theParser);
			break;	
		case ')':
			theParser->token2 = XS_TOKEN_RIGHT_PARENTHESIS;
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
		case '=':
			fxGetNextCharacter(theParser);
			if (theParser->character == '=') {			
				fxGetNextCharacter(theParser);
				if (theParser->character == '=') {
					theParser->token2 = XS_TOKEN_STRICT_EQUAL;
					fxGetNextCharacter(theParser);
				}
				else
					theParser->token2 = XS_TOKEN_EQUAL;
			}
			else
				theParser->token2 = XS_TOKEN_ASSIGN;
			break;
		case '<':
			fxGetNextCharacter(theParser);
			if (theParser->character == '<') {
				fxGetNextCharacter(theParser);
				if (theParser->character == '=') {
					theParser->token2 = XS_TOKEN_LEFT_SHIFT_ASSIGN;
					fxGetNextCharacter(theParser);
				}
				else
					theParser->token2 = XS_TOKEN_LEFT_SHIFT;
			}
			else  if (theParser->character == '=') {
				theParser->token2 = XS_TOKEN_LESS_EQUAL;
				fxGetNextCharacter(theParser);
			}
			else
				theParser->token2 = XS_TOKEN_LESS;
			break;	
		case '>':
			fxGetNextCharacter(theParser);
			if (theParser->character == '>') {			
				fxGetNextCharacter(theParser);
				if (theParser->character == '>') {			
					fxGetNextCharacter(theParser);
					if (theParser->character == '=') {
						theParser->token2 = XS_TOKEN_UNSIGNED_RIGHT_SHIFT_ASSIGN;
						fxGetNextCharacter(theParser);
					}
					else
						theParser->token2 = XS_TOKEN_UNSIGNED_RIGHT_SHIFT;
				}
				else if (theParser->character == '=') {
					theParser->token2 = XS_TOKEN_SIGNED_RIGHT_SHIFT_ASSIGN;
					fxGetNextCharacter(theParser);
				}
				else
					theParser->token2 = XS_TOKEN_SIGNED_RIGHT_SHIFT;
			}
			else if (theParser->character == '=') {
				theParser->token2 = XS_TOKEN_MORE_EQUAL;
				fxGetNextCharacter(theParser);
			}
			else
				theParser->token2 = XS_TOKEN_MORE;
			break;	
		case '!':
			fxGetNextCharacter(theParser);
			if (theParser->character == '=') {			
				fxGetNextCharacter(theParser);
				if (theParser->character == '=') {
					theParser->token2 = XS_TOKEN_STRICT_NOT_EQUAL;
					fxGetNextCharacter(theParser);
				}
				else
					theParser->token2 = XS_TOKEN_NOT_EQUAL;
			}
			else
				theParser->token2 = XS_TOKEN_NOT;
			break;
		case '~':
			theParser->token2 = XS_TOKEN_BIT_NOT;
			fxGetNextCharacter(theParser);
			break;
		case '&':
			fxGetNextCharacter(theParser);
			if (theParser->character == '=') {	
				theParser->token2 = XS_TOKEN_BIT_AND_ASSIGN;
				fxGetNextCharacter(theParser);
			}
			else if (theParser->character == '&') {
				theParser->token2 = XS_TOKEN_AND;
				fxGetNextCharacter(theParser);
			}
			else
				theParser->token2 = XS_TOKEN_BIT_AND;
			break;
		case '|':
			fxGetNextCharacter(theParser);
			if (theParser->character == '=') {
				theParser->token2 = XS_TOKEN_BIT_OR_ASSIGN;
				fxGetNextCharacter(theParser);
			}
			else if (theParser->character == '|') {
				theParser->token2 = XS_TOKEN_OR;
				fxGetNextCharacter(theParser);
			}
			else
				theParser->token2 = XS_TOKEN_BIT_OR;
			break;
		case '^':
			fxGetNextCharacter(theParser);
			if (theParser->character == '=') {
				theParser->token2 = XS_TOKEN_BIT_XOR_ASSIGN;
				fxGetNextCharacter(theParser);
			}
			else
				theParser->token2 = XS_TOKEN_BIT_XOR;
			break;
		case '+':	
			fxGetNextCharacter(theParser);
			if (theParser->character == '=') {
				theParser->token2 = XS_TOKEN_ADD_ASSIGN;
				fxGetNextCharacter(theParser);
			}
			else if (theParser->character == '+') {
				theParser->token2 = XS_TOKEN_INCREMENT;
				fxGetNextCharacter(theParser);
			}
			else
				theParser->token2 = XS_TOKEN_ADD;
			break;
		case '-':	
			fxGetNextCharacter(theParser);
			if (theParser->character == '=') {
				theParser->token2 = XS_TOKEN_SUBTRACT_ASSIGN;
				fxGetNextCharacter(theParser);
			}
			else if (theParser->character == '-') {
				theParser->token2 = XS_TOKEN_DECREMENT;
				fxGetNextCharacter(theParser);
			}
			else
				theParser->token2 = XS_TOKEN_SUBTRACT;
			break;
		case '*':	
			fxGetNextCharacter(theParser);
			if (theParser->character == '=') {
				theParser->token2 = XS_TOKEN_MULTIPLY_ASSIGN;
				fxGetNextCharacter(theParser);
			}
			else
				theParser->token2 = XS_TOKEN_MULTIPLY;
			break;
		case '/':
			fxGetNextCharacter(theParser);
			if (theParser->character == '*') {
			#ifdef mxColor
				if (theParser->stream2) {
					theParser->putter("\t\t<Comment from=\"", theParser->stream2);
					theParser->putter(fxIntegerToString(theParser->startOffset2, aBuffer, sizeof(aBuffer)), theParser->stream2);
					theParser->putter("\"", theParser->stream2);
				}
			#endif
				fxGetNextCharacter(theParser);
				for (;;) {
					if (theParser->character == C_EOF)
						break;
					else if (theParser->character == 10) {
						theParser->line2++;
						fxGetNextCharacter(theParser);
					}
					else if (theParser->character == 13) {
						theParser->line2++;
						fxGetNextCharacter(theParser);
						if (theParser->character == 10)
							fxGetNextCharacter(theParser);
					}
					else if (theParser->character == '*') {
						fxGetNextCharacter(theParser);
						if (theParser->character == '/') {
							fxGetNextCharacter(theParser);
						#ifdef mxColor
							if (theParser->stream2) {
								theParser->putter(" to=\"", theParser->stream2);
								theParser->putter(fxIntegerToString(theParser->offset, aBuffer, sizeof(aBuffer)), theParser->stream2);
								theParser->putter("\"/>\n", theParser->stream2);
							}
							theParser->startOffset2 = theParser->offset;
						#endif
							break;
						}
					}
					else
						fxGetNextCharacter(theParser);
				}
			}
			else if (theParser->character == '/') {
			#ifdef mxColor
				if (theParser->stream2) {
					theParser->putter("\t\t<Comment from=\"", theParser->stream2);
					theParser->putter(fxIntegerToString(theParser->startOffset2, aBuffer, sizeof(aBuffer)), theParser->stream2);
					theParser->putter("\"", theParser->stream2);
				}
			#endif
				if (theParser->debug) {
					fxGetNextCharacter(theParser);
					if (theParser->character != '@') goto bail;
					fxGetNextCharacter(theParser);
					if (theParser->character != 'l') goto bail;
					fxGetNextCharacter(theParser);
					if (theParser->character != 'i') goto bail;
					fxGetNextCharacter(theParser);
					if (theParser->character != 'n') goto bail;
					fxGetNextCharacter(theParser);
					if (theParser->character != 'e') goto bail;
					fxGetNextCharacter(theParser);
					if (theParser->character != ' ') goto bail;
					fxGetNextCharacter(theParser);
					t = 0;
					while (('0' <= theParser->character) && (theParser->character <= '9')) {
						t = (t * 10) + (theParser->character - '0');
						fxGetNextCharacter(theParser);
					}
					if (!t) goto bail;
					if (theParser->character == ' ') {
						fxGetNextCharacter(theParser);
						if (theParser->character != '"') goto bail;
						fxGetNextCharacter(theParser);
						p = theParser->buffer;
						q = p + sizeof(theParser->buffer) - 1;
						while ((theParser->character != C_EOF) && (theParser->character != 10) && (theParser->character != 13) && (theParser->character != '"')) {
							if (p == q) goto bail;
							*p++ = theParser->character;
							fxGetNextCharacter(theParser);
						}	
						if (theParser->character != '"') goto bail;
						fxGetNextCharacter(theParser);
						if ((theParser->character != C_EOF) && (theParser->character != 10) && (theParser->character != 13)) goto bail;
						*p = 0;
						theParser->path = fxNewFileC(the, theParser->buffer);
					}
					if ((theParser->character != C_EOF) && (theParser->character != 10) && (theParser->character != 13)) goto bail;
					theParser->line2 = t - 1;
				}
			bail:
				while ((theParser->character != C_EOF) && (theParser->character != 10) && (theParser->character != 13))
					fxGetNextCharacter(theParser);
			#ifdef mxColor
				if (theParser->stream2) {
					theParser->putter(" to=\"", theParser->stream2);
					theParser->putter(fxIntegerToString(theParser->offset, aBuffer, sizeof(aBuffer)), theParser->stream2);
					theParser->putter("\"/>\n", theParser->stream2);
				}
				theParser->startOffset2 = theParser->offset;
			#endif
			}
			else if ((theParser->crlf2) || ((gxTokenFlags[theParser->token] & XS_TOKEN_NO_REGEXP) == 0)) {
				theParser->token2 = XS_TOKEN_NULL;
				p = theParser->buffer;
				q = p + sizeof(theParser->buffer) - 1;
				for (;;) {
					if (p == q) {
						fxReportParserWarning(theParser, "regular expression overflow");			
						break;
					}
					else if (theParser->character == C_EOF) {
						fxReportParserWarning(theParser, "end of file in regular expression");			
						break;
					}
					else if ((theParser->character == 10) || (theParser->character == 13)) {
						fxReportParserWarning(theParser, "end of line in regular expression");			
						break;
					}
					else if (theParser->character == '/') {
						*p = 0;
						fxCopyStringC(theParser->the, theParser->string2, theParser->buffer);
						theParser->token2 = XS_TOKEN_REGEXP;
						p = theParser->buffer;
						q = p + sizeof(theParser->buffer) - 1;
						for (;;) {
							fxGetNextCharacter(theParser);
							if (p == q) {
								fxReportParserWarning(theParser, "regular expression overflow");			
								break;
							}
							else if (fxIsIdentifierNext(theParser->character))
								*p++ = theParser->character;
							else {
								if (p != theParser->buffer) {
									*p = 0;
									fxCopyStringC(theParser->the, theParser->flags2, theParser->buffer);
								}
								break;
							}
						}
						break;
					}
					else if (theParser->character == '\\') {
						*p++ = theParser->character;
                        fxGetNextCharacter(theParser);
                        if (p == q) {
                            fxReportParserWarning(theParser, "regular expression overflow");			
                            break;
                        }
					}
                    p = (txString)fsX2UTF8(theParser, theParser->character, (txU1*)p, q - p);
                    //*p++ = theParser->character;
                    fxGetNextCharacter(theParser);
				}
			}
			else if (theParser->character == '=') {
				theParser->token2 = XS_TOKEN_DIVIDE_ASSIGN;
				fxGetNextCharacter(theParser);
			}
			else 
				theParser->token2 = XS_TOKEN_DIVIDE;
			break;
		case '%':	
			fxGetNextCharacter(theParser);
			if (theParser->character == '=') {
				theParser->token2 = XS_TOKEN_MODULO_ASSIGN;
				fxGetNextCharacter(theParser);
			}
			else
				theParser->token2 = XS_TOKEN_MODULO;
			break;
		
		case '"':
		case '\'':
			c = theParser->character;
			p = theParser->buffer;
			q = p + sizeof(theParser->buffer) - 1;
			r = C_NULL;
			fxGetNextCharacter(theParser);
			for (;;) {
				if (theParser->character == C_EOF) {
					fxReportParserWarning(theParser, "end of file in string");			
					break;
				}
				else if ((theParser->character == 10) || (theParser->character == 13)) {
					fxReportParserWarning(theParser, "end of line in string");			
					break;
				}
				else if (theParser->character == c) {
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
					#ifdef mxColor
						theParser->offset = 0;
					#endif
						break;
					case 13:
						theParser->line2++;
						fxGetNextCharacter(theParser);
						if (theParser->character == 10)
							fxGetNextCharacter(theParser);
					#ifdef mxColor
						theParser->offset = 0;
					#endif
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
				fxReportParserWarning(theParser, "string overflow");			
			fxCopyStringC(theParser->the, theParser->string2, theParser->buffer);
			theParser->token2 = XS_TOKEN_STRING;
			break;
		default:
			if (fxIsIdentifierFirst(theParser->character)) {
				p = theParser->buffer;
				q = p + sizeof(theParser->buffer) - 1;
				for (;;) {
					if (p == q) {
						fxReportParserWarning(theParser, "identifier overflow");			
						break;
					}
					*p++ = theParser->character;
					fxGetNextCharacter(theParser);
					if (!fxIsIdentifierNext(theParser->character))
						break;
				}
				*p = 0;
				fxGetNextKeyword(theParser);
			}
			else {
				fxReportParserWarning(theParser, "invalid character %d", theParser->character);
				fxGetNextCharacter(theParser);
			}
			break;
		}
	}
#ifdef mxColor
	theParser->stopOffset2 = theParser->offset;
	if (theParser->stream2 && *gxTokenTags[theParser->token2]) {
		theParser->putter("\t\t<", theParser->stream2);
		theParser->putter(gxTokenTags[theParser->token2], theParser->stream2);
		theParser->putter(" from=\"", theParser->stream2);
		theParser->putter(fxIntegerToString(theParser->startOffset2, aBuffer, sizeof(aBuffer)), theParser->stream2);
		theParser->putter("\" to=\"", theParser->stream2);
		theParser->putter(fxIntegerToString(theParser->stopOffset2, aBuffer, sizeof(aBuffer)), theParser->stream2);
		theParser->putter("\"/>\n", theParser->stream2);
	}
#endif
}

void fxMatchToken(txScriptParser* theParser, txToken theToken)
{
	if (theParser->token == theToken)
		fxGetNextToken(theParser);
	else
		fxReportParserError(theParser, "missing %s", gxTokenNames[theToken]);
}

txBoolean fxIsIdentifierFirst(char c)
{
	return ((('A' <= c) && (c <= 'Z')) || (('a' <= c) && (c <= 'z')) || (c == '$') || (c == '_')) ? 1 : 0;
}

txBoolean fxIsIdentifierNext(char c)
{
	return ((('0' <= c) && (c <= '9')) || (('A' <= c) && (c <= 'Z')) || (('a' <= c) && (c <= 'z')) || (c == '$') || (c == '_')) ? 1 : 0;
}

txU1* fsX2UTF8(txScriptParser* theParser, txU4 c, txU1* p, txU4 theSize)
{
	txU4 i;
	txUTF8Sequence *aSequence;
	txS4 aShift;
	
	i = 0;
	for (aSequence = gxUTF8Sequences; aSequence->size; aSequence++)
		if (c <= aSequence->lmask)
			break;
	if (aSequence->size == 0)
		fxReportParserWarning(theParser, "invalid string");
	else {
		i += aSequence->size;
		if (i < theSize) {
			aShift = aSequence->shift;
			*p++ = (unsigned char)(aSequence->cval | (c >> aShift));
			while (aShift > 0) {
				aShift -= 6;
				*p++ = (unsigned char)(0x80 | ((c >> aShift) & 0x3F));
			}
		}
		else {
			*p = 0;
			p += theSize;
		}
	}
	return p;
}

