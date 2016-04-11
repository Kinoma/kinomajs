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

enum {
	XS_NO_JSON_TOKEN,
	XS_JSON_TOKEN_COLON,
	XS_JSON_TOKEN_COMMA,
	XS_JSON_TOKEN_EOF,
	XS_JSON_TOKEN_FALSE,
	XS_JSON_TOKEN_LEFT_BRACE,
	XS_JSON_TOKEN_LEFT_BRACKET,
	XS_JSON_TOKEN_NULL,
	XS_JSON_TOKEN_NUMBER,
	XS_JSON_TOKEN_RIGHT_BRACE,
	XS_JSON_TOKEN_RIGHT_BRACKET,
	XS_JSON_TOKEN_STRING,
	XS_JSON_TOKEN_TRUE,
};

static FskErr fxParseJSONArray(KprCodeParser* parser);
static FskErr fxParseJSONObject(KprCodeParser* parser);
static FskErr fxParseJSONToken(KprCodeParser* parser);
static FskErr fxParseJSONValue(KprCodeParser* parser);

FskErr KprCodeMeasureJSON(KprCode self) 
{
	FskErr err = kFskErrNone;
	KprCodeParser parserRecord;
	KprCodeParser* parser = &parserRecord;
	bailIfError(KprCodeParserBegin(self, parser));
	bailIfError(fxParseJSONToken(parser));
	bailIfError(fxParseJSONValue(parser));
	if (parser->token != XS_JSON_TOKEN_EOF)
		err = kFskErrBadData;
bail:
	if ((err == kFskErrNone) || (err == kFskErrBadData))
		err = KprCodeParserEnd(self, parser);
	return err;
}

FskErr fxParseJSONArray(KprCodeParser* parser)
{
	FskErr err = kFskErrNone;
	bailIfError(fxParseJSONToken(parser));
	for (;;) {
		if (parser->token == XS_JSON_TOKEN_RIGHT_BRACKET)
			break;
		bailIfError(fxParseJSONValue(parser));
		if (parser->token != XS_JSON_TOKEN_COMMA)
			break;
		bailIfError(fxParseJSONToken(parser));
	}
	if (parser->token != XS_JSON_TOKEN_RIGHT_BRACKET)
		return kFskErrBadData;
	bailIfError(fxParseJSONToken(parser));
bail:
	return err;
}

FskErr fxParseJSONObject(KprCodeParser* parser)
{
	FskErr err = kFskErrNone;
	bailIfError(fxParseJSONToken(parser));
	for (;;) {
		if (parser->token == XS_JSON_TOKEN_RIGHT_BRACE)
			break;
		if (parser->token != XS_JSON_TOKEN_STRING)
			return kFskErrBadData;
		bailIfError(KprCodeParserColorAt(parser, 1, parser->offset));
		bailIfError(KprCodeParserColorAt(parser, 0, parser->input));
		bailIfError(fxParseJSONToken(parser));
		if (parser->token != XS_JSON_TOKEN_COLON)
			return kFskErrBadData;
		bailIfError(fxParseJSONToken(parser));
		bailIfError(fxParseJSONValue(parser));
		if (parser->token != XS_JSON_TOKEN_COMMA)
			break;
		bailIfError(fxParseJSONToken(parser));
	}
	if (parser->token != XS_JSON_TOKEN_RIGHT_BRACE)
		return kFskErrBadData;
	bailIfError(fxParseJSONToken(parser));
bail:
	return err;
}

FskErr fxParseJSONToken(KprCodeParser* parser) 
{
	FskErr err = kFskErrNone;
	int i;
	parser->token = XS_NO_JSON_TOKEN;
	while (parser->token == XS_NO_JSON_TOKEN) {
		parser->offset = parser->input;
		switch (parser->character) {
		case 0:
			parser->token = XS_JSON_TOKEN_EOF;
			break;
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
			
		case '-':
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			if (parser->character == '-')
				KprCodeParserAdvance(parser);
			if (('0' <= parser->character) && (parser->character <= '9')) {
				if (parser->character == '0') {
					KprCodeParserAdvance(parser);
				}
				else {
					KprCodeParserAdvance(parser);
					while (('0' <= parser->character) && (parser->character <= '9')) {
						KprCodeParserAdvance(parser);
					}
				}
				if (parser->character == '.') {
					KprCodeParserAdvance(parser);
					if (('0' <= parser->character) && (parser->character <= '9')) {
						KprCodeParserAdvance(parser);
						while (('0' <= parser->character) && (parser->character <= '9')) {
							KprCodeParserAdvance(parser);
						}
					}
					else
						return kFskErrBadData;
				}
				if ((parser->character == 'e') || (parser->character == 'E')) {
					KprCodeParserAdvance(parser);
					if ((parser->character== '+') || (parser->character == '-')) {
						KprCodeParserAdvance(parser);
					}
					if (('0' <= parser->character) && (parser->character <= '9')) {
						KprCodeParserAdvance(parser);
						while (('0' <= parser->character) && (parser->character <= '9')) {
							KprCodeParserAdvance(parser);
						}
					}
					else
						return kFskErrBadData;
				}
			}
			else
				return kFskErrBadData;	
			parser->token = XS_JSON_TOKEN_NUMBER;
			break;
		case ',':
			KprCodeParserAdvance(parser);
			parser->token = XS_JSON_TOKEN_COMMA;
			break;	
		case ':':
			KprCodeParserAdvance(parser);
			parser->token = XS_JSON_TOKEN_COLON;
			break;	
		case '[':
			KprCodeParserAdvance(parser);
			parser->token = XS_JSON_TOKEN_LEFT_BRACKET;
			break;	
		case ']':
			KprCodeParserAdvance(parser);
			parser->token = XS_JSON_TOKEN_RIGHT_BRACKET;
			break;	
		case '{':
			KprCodeParserAdvance(parser);
			parser->token = XS_JSON_TOKEN_LEFT_BRACE;
			break;	
		case '}':
			KprCodeParserAdvance(parser);
			parser->token = XS_JSON_TOKEN_RIGHT_BRACE;
			break;	
		case '"':
			KprCodeParserAdvance(parser);
			for (;;) {
				if (parser->character < 32) {
					return kFskErrBadData;				
					break;
				}
				else if (parser->character == '"') {
					KprCodeParserAdvance(parser);
					break;
				}
				else if (parser->character == '\\') {
					KprCodeParserAdvance(parser);
					switch (parser->character) {
					case '"':
					case '/':
					case '\\':
					case 'b':
					case 'f':
					case 'n':
					case 'r':
					case 't':
						KprCodeParserAdvance(parser);
						break;
					case 'u':
						KprCodeParserAdvance(parser);
						for (i = 0; i < 4; i++) {
							if (('0' <= parser->character) && (parser->character <= '9'))
								KprCodeParserAdvance(parser);
							else if (('a' <= parser->character) && (parser->character <= 'f'))
								KprCodeParserAdvance(parser);
							else if (('A' <= parser->character) && (parser->character <= 'F'))
								KprCodeParserAdvance(parser);
							else
								return kFskErrBadData;
						}
						break;
					default:
						return kFskErrBadData;
						break;
					}
				}
				else {
					KprCodeParserAdvance(parser);
				}
			}
			parser->token = XS_JSON_TOKEN_STRING;
			break;
		case 'f':
			KprCodeParserAdvance(parser);
			if (parser->character != 'a') return kFskErrBadData;	
			KprCodeParserAdvance(parser);
			if (parser->character != 'l') return kFskErrBadData;	
			KprCodeParserAdvance(parser);
			if (parser->character != 's') return kFskErrBadData;	
			KprCodeParserAdvance(parser);
			if (parser->character != 'e') return kFskErrBadData;	
			KprCodeParserAdvance(parser);
			parser->token = XS_JSON_TOKEN_FALSE;
			break;
		case 'n':
			KprCodeParserAdvance(parser);
			if (parser->character != 'u') return kFskErrBadData;	
			KprCodeParserAdvance(parser);
			if (parser->character != 'l') return kFskErrBadData;	
			KprCodeParserAdvance(parser);
			if (parser->character != 'l') return kFskErrBadData;	
			KprCodeParserAdvance(parser);
			parser->token = XS_JSON_TOKEN_NULL;
			break;
		case 't':
			KprCodeParserAdvance(parser);
			if (parser->character != 'r') return kFskErrBadData;	
			KprCodeParserAdvance(parser);
			if (parser->character != 'u') return kFskErrBadData;	
			KprCodeParserAdvance(parser);
			if (parser->character != 'e') return kFskErrBadData;	
			KprCodeParserAdvance(parser);
			parser->token = XS_JSON_TOKEN_TRUE;
			break;
		default:
			return kFskErrBadData;	
		}
	}
bail:
	return err;
}

FskErr fxParseJSONValue(KprCodeParser* parser)
{
	FskErr err = kFskErrNone;
	switch (parser->token) {
	case XS_JSON_TOKEN_FALSE:
	case XS_JSON_TOKEN_TRUE:
	case XS_JSON_TOKEN_NULL:
	case XS_JSON_TOKEN_NUMBER:
	case XS_JSON_TOKEN_STRING:
		bailIfError(KprCodeParserColorAt(parser, 2, parser->offset));
		bailIfError(KprCodeParserColorAt(parser, 0, parser->input));
		bailIfError(fxParseJSONToken(parser));
		break;
	case XS_JSON_TOKEN_LEFT_BRACE:
		bailIfError(fxParseJSONObject(parser));
		break;
	case XS_JSON_TOKEN_LEFT_BRACKET:
		bailIfError(fxParseJSONArray(parser));
		break;
	default:
		return kFskErrBadData;	
	}
bail:
	return err;
}
