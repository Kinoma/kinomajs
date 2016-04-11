/*
 *     Copyright (C) 2010-2016 Marvell International Ltd.
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
#include "kprCodeColor.h"

typedef struct {
	char* string;
	Boolean noRegExp;
} txKeyword;

static Boolean fxIsJSIdentifierFirst(UInt32 c);
static Boolean fxIsJSIdentifierNext(UInt32 c);
static Boolean fxIsKeyword(UInt8* string, UInt32 length, Boolean* noRegExp);
static FskErr fxParseJSCode(KprCodeParser* parser, Boolean template);
static void fxParseJSNumberB(KprCodeParser* parser);
static void fxParseJSNumberE(KprCodeParser* parser, int parseDot);
static void fxParseJSNumberO(KprCodeParser* parser);
static void fxParseJSNumberX(KprCodeParser* parser);
static FskErr fxParseJSString(KprCodeParser* parser);

#define XS_KEYWORD_COUNT 46
static const txKeyword gxKeywords[XS_KEYWORD_COUNT] = {
	{ "break", 0 },
	{ "case", 0 },
	{ "catch", 0 },
	{ "class", 0 },
	{ "const", 0 },
	{ "continue", 0 },
	{ "debugger", 0 },
	{ "default", 0 },
	{ "delete", 0 },
	{ "do", 0 },
	{ "else", 0 },
	{ "enum", 0 },
	{ "export", 0 },
	{ "extends", 0 },
	{ "false", 1 },
	{ "finally", 0 },
	{ "for", 0 },
	{ "function", 0 },
	{ "if", 0 },
	{ "implements", 0 },
	{ "import", 0 },
	{ "in", 0 },
	{ "instanceof", 0 },
	{ "interface", 0 },
	{ "let", 0 },
	{ "new", 0 }, 
	{ "null", 1 }, 
	{ "package", 0 },
	{ "private", 0 },
	{ "protected", 0 },
	{ "public", 0 },
	{ "return", 0 },
	{ "static", 0 },
	{ "super", 1 },
	{ "switch", 0 },
	{ "this", 1 },
	{ "throw", 0 },
	{ "true", 1 },
	{ "try", 0 },
	{ "typeof", 0 },
	{ "undefined", 1 },
	{ "var", 0 },
	{ "void", 0 },
	{ "while", 0 },
	{ "with", 0 },
	{ "yield", 0 },
};

FskErr KprCodeMeasureJS(KprCode self) 
{
	FskErr err = kFskErrNone;
	KprCodeParser parserRecord;
	KprCodeParser* parser = &parserRecord;
	bailIfError(KprCodeParserBegin(self, parser));
	bailIfError(fxParseJSCode(parser, 0));
bail:
	if ((err == kFskErrNone) || (err == kFskErrBadData))
		err = KprCodeParserEnd(self, parser);
	return err;
}

Boolean fxIsJSIdentifierFirst(UInt32 c)
{
	return ((('A' <= c) && (c <= 'Z')) || (('a' <= c) && (c <= 'z')) || (c == '$') || (c == '_')) ? 1 : 0;
}

Boolean fxIsJSIdentifierNext(UInt32 c)
{
	return ((('0' <= c) && (c <= '9')) || (('A' <= c) && (c <= 'Z')) || (('a' <= c) && (c <= 'z')) || (c == '$') || (c == '_')) ? 1 : 0;
}

Boolean fxIsKeyword(UInt8* string, UInt32 length, Boolean* noRegExp)
{
	int low, high, index, delta;
	char c  = string[length];
	string[length] = 0;
	for (low = 0, high = XS_KEYWORD_COUNT; high > low; (delta < 0) ? (low = index + 1) : (high = index)) {
		index = low + ((high - low) / 2);
		delta = FskStrCompare(gxKeywords[index].string, (const char*)string);
		if (delta == 0) {
			string[length] = c;
			*noRegExp = gxKeywords[index].noRegExp;
			return 1;
		}
	}
	string[length] = c;
	return 0;
}

FskErr fxParseJSCode(KprCodeParser* parser, Boolean template) 
{
	FskErr err = kFskErrNone;
	SInt32 braces = 0;
	SInt32 offset;
	UInt32 c;
	Boolean noRegExp = 0;
	for (;;) {
		offset = parser->input;
		if (!parser->character)
			break;
		switch (parser->character) {
		case 9:
			bailIfError(KprCodeParserTab(parser));
			break;
		case 10:
		case 13:
			bailIfError(KprCodeParserReturn(parser));
			break;
		case ' ':
			KprCodeParserAdvance(parser);
			break;
			
		case '0':
			KprCodeParserAdvance(parser);
			c = parser->character;
			if (c == '.') {
				KprCodeParserAdvance(parser);
				c = parser->character;
				if ((('0' <= c) && (c <= '9')) || (c == 'e') || (c == 'E'))
					fxParseJSNumberE(parser, 0);
			}
			else if ((c == 'b') || (c == 'B'))
				fxParseJSNumberB(parser);
			else if ((c == 'e') || (c == 'E'))
				fxParseJSNumberE(parser, 0);
			else if ((c == 'o') || (c == 'O'))
				fxParseJSNumberO(parser);
			else if ((c == 'x') || (c == 'X'))
				fxParseJSNumberX(parser);
			else if (('0' <= c) && (c <= '7'))
				fxParseJSNumberO(parser);
			bailIfError(KprCodeParserColorAt(parser, 2, offset));
			bailIfError(KprCodeParserColorAt(parser, 0, parser->input));
			noRegExp = 1;
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
			fxParseJSNumberE(parser, 1);
			bailIfError(KprCodeParserColorAt(parser, 2, offset));
			bailIfError(KprCodeParserColorAt(parser, 0, parser->input));
			noRegExp = 1;
			break;
		case '.':
			KprCodeParserAdvance(parser);
			c = parser->character;
			if (('0' <= c) && (c <= '9')) {
				fxParseJSNumberE(parser, 0);
				bailIfError(KprCodeParserColorAt(parser, 2, offset));
				bailIfError(KprCodeParserColorAt(parser, 0, parser->input));
			}
			noRegExp = 1;
			break;
				
		case '{':
			if (template)
				braces++;
            KprCodeParserAdvance(parser);
			noRegExp = 0;
			break;
		case '}':
			if (template) {
				if (braces == 0) {
					bailIfError(KprCodeParserColorAt(parser, 2, offset));
					return err;
				}
				braces--;
			}
            KprCodeParserAdvance(parser);
			noRegExp = 0;
			break;
		case ']':
		case ')':
            KprCodeParserAdvance(parser);
			noRegExp = 1;
			break;
		case '"':
		case '\'':
		case '`':
			bailIfError(KprCodeParserColorAt(parser, 2, offset));
			bailIfError(fxParseJSString(parser));
			noRegExp = 1;
			break;
		
		case '/':
			KprCodeParserAdvance(parser);
			if (parser->character == '*') {
				KprCodeParserAdvance(parser);
				bailIfError(KprCodeParserColorAt(parser, 3, offset));
				for (;;) {
					if (parser->character == 0) {
						break;
					}
					else if (parser->character == 9) {
						bailIfError(KprCodeParserTab(parser));
					}
					else if ((parser->character == 10) || (parser->character == 13)) {
						bailIfError(KprCodeParserReturn(parser));
					}
					else if (parser->character == '*') {
						KprCodeParserAdvance(parser);
						if (parser->character == '/') {
							KprCodeParserAdvance(parser);
							bailIfError(KprCodeParserColorAt(parser, 0, parser->input));
							break;
						}
					}
					else
						KprCodeParserAdvance(parser);
				}
			}
			else if (parser->character == '/') {
				KprCodeParserAdvance(parser);
				bailIfError(KprCodeParserColorAt(parser, 3, offset));
				for (;;) {
					if (parser->character == 0) {
						break;
					}
					else if (parser->character == 9) {
						bailIfError(KprCodeParserTab(parser));
					}
					else if ((parser->character == 10) || (parser->character == 13)) {
						bailIfError(KprCodeParserReturn(parser));
						bailIfError(KprCodeParserColorAt(parser, 0, parser->input));
						break;
					}
					else
						KprCodeParserAdvance(parser);
				}
			}
			else  if (!noRegExp) {
				bailIfError(KprCodeParserColorAt(parser, 2, offset));
				for (;;) {
					if (parser->character == 0) {
						break;
					}
					else if (parser->character == 9) {
						bailIfError(KprCodeParserTab(parser));
					}
					else if ((parser->character == 10) || (parser->character == 13)) {
						bailIfError(KprCodeParserReturn(parser));
						bailIfError(KprCodeParserColorAt(parser, 0, parser->input));
						break;
					}
					else if (parser->character == '/') {
						KprCodeParserAdvance(parser);
						while (('a' <= parser->character) && (parser->character <= 'z'))
							KprCodeParserAdvance(parser);
						bailIfError(KprCodeParserColorAt(parser, 0, parser->input));
						break;
					}
					else if (parser->character == '\\') {
						KprCodeParserAdvance(parser);
						if (parser->character == '/')
							KprCodeParserAdvance(parser);
					}
					else
						KprCodeParserAdvance(parser);
				}
				noRegExp = 1;
			}
			break;
			
		default:
			if (fxIsJSIdentifierFirst(parser->character)) {
				for (;;) {
					KprCodeParserAdvance(parser);
					if (!fxIsJSIdentifierNext(parser->character))
						break;
				}
				if (fxIsKeyword(parser->string + offset, parser->input - offset, &noRegExp)) {
					bailIfError(KprCodeParserColorAt(parser, 1, offset));
					bailIfError(KprCodeParserColorAt(parser, 0, parser->input));
				}
				else
					noRegExp = 1;
			}
			else {
				KprCodeParserAdvance(parser);
				noRegExp = 0;
			}
			break;		
		}
	}
bail:
	return err;
}

void fxParseJSNumberB(KprCodeParser* parser)
{
	UInt32 c;
	for (;;) {
		KprCodeParserAdvance(parser);
		c = parser->character;
		if (('0' <= c) && (c <= '1'))
			continue;
		break;
	}
}

void fxParseJSNumberE(KprCodeParser* parser, int parseDot)
{
	while (('0' <= parser->character) && (parser->character <= '9')) {
		KprCodeParserAdvance(parser);
	}
	if (parseDot) {
		if (parser->character == '.') {
			KprCodeParserAdvance(parser);
			while (('0' <= parser->character) && (parser->character <= '9')) {
				KprCodeParserAdvance(parser);
			}
		}
	}
	if ((parser->character == 'e') || (parser->character == 'E')) {
		KprCodeParserAdvance(parser);
		if ((parser->character == '+') || (parser->character == '-')) {
			KprCodeParserAdvance(parser);
		}
		while (('0' <= parser->character) && (parser->character <= '9')) {
			KprCodeParserAdvance(parser);
		}
	}
}

void fxParseJSNumberO(KprCodeParser* parser)
{
	UInt32 c;
	for (;;) {
		KprCodeParserAdvance(parser);
		c = parser->character;
		if (('0' <= c) && (c <= '7'))
			continue;
		break;
	}
}

void fxParseJSNumberX(KprCodeParser* parser)
{
	int c;
	for (;;) {
		KprCodeParserAdvance(parser);
		c = parser->character;
		if (('0' <= c) && (c <= '9'))
			continue;
		if (('a' <= c) && (c <= 'f'))
			continue;
		if (('A' <= c) && (c <= 'F'))
			continue;
		break;
	}
}

FskErr fxParseJSString(KprCodeParser* parser)
{
	FskErr err = kFskErrNone;
	UInt32 c = parser->character;
	KprCodeParserAdvance(parser);
	for (;;) {
		if (parser->character == 0) {
			break;
		}
		else if (parser->character == 9) {
			bailIfError(KprCodeParserTab(parser));
		}
		else if ((parser->character == 10) || (parser->character == 13)) {
			bailIfError(KprCodeParserReturn(parser));
			if (c != '`') {
				bailIfError(KprCodeParserColorAt(parser, 0, parser->input));
				break;
			}
		}
		else if (parser->character == c) {
			KprCodeParserAdvance(parser);
			bailIfError(KprCodeParserColorAt(parser, 0, parser->input));
			break;
		}
		else if (parser->character == '$') {
			KprCodeParserAdvance(parser);
			if ((c == '`') && (parser->character == '{')) {
				KprCodeParserAdvance(parser);
				bailIfError(KprCodeParserColorAt(parser, 0, parser->input));
				fxParseJSCode(parser, 1);
			}
		}
		else if (parser->character == '\\') {
			KprCodeParserAdvance(parser);
			if (parser->character == 9) {
				bailIfError(KprCodeParserTab(parser));
			}
			else if ((parser->character == 10) || (parser->character == 13)) {
				bailIfError(KprCodeParserReturn(parser));
			}
			else {
				KprCodeParserAdvance(parser);
			}	
		}	
		else
			KprCodeParserAdvance(parser);
	}	
bail:
	return err;
}
