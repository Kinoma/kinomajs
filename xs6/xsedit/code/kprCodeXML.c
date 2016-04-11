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

static Boolean fxIsXMLIdentifierFirst(UInt32 c);
static Boolean fxIsXMLIdentifierNext(UInt32 c);
static FskErr fxParseXMLComment(KprCodeParser* parser);
static FskErr fxParseXMLData(KprCodeParser* parser);
static FskErr fxParseXMLName(KprCodeParser* parser);
static FskErr fxParseXMLProcessingInstruction(KprCodeParser* parser);
static FskErr fxParseXMLSpace(KprCodeParser* parser, int* length);
static FskErr fxParseXMLStartTag(KprCodeParser* parser);
static FskErr fxParseXMLStopTag(KprCodeParser* parser);
static FskErr fxParseXMLValue(KprCodeParser* parser);

FskErr KprCodeMeasureXML(KprCode self) 
{
	FskErr err = kFskErrNone;
	KprCodeParser parserRecord;
	KprCodeParser* parser = &parserRecord;
	bailIfError(KprCodeParserBegin(self, parser));
	while (parser->character) {
		bailIfError(fxParseXMLSpace(parser, NULL));
		if (parser->character == '<') {
			parser->offset = parser->input;
			KprCodeParserAdvance(parser);
			if (parser->character == '/') {
				KprCodeParserAdvance(parser);
				bailIfError(fxParseXMLStopTag(parser));
			}
			else if (parser->character == '?') {
				KprCodeParserAdvance(parser);
				bailIfError(fxParseXMLProcessingInstruction(parser));
			}
			else if (parser->character == '!') {
				KprCodeParserAdvance(parser);
				if (parser->character == '-') {
					KprCodeParserAdvance(parser);
					if (parser->character != '-') { err = kFskErrBadData; goto bail; }
					KprCodeParserAdvance(parser);
					bailIfError(fxParseXMLComment(parser));
				}
				else if (parser->character == '[') {
					KprCodeParserAdvance(parser);
					if (parser->character != 'C') { err = kFskErrBadData; goto bail; }
					KprCodeParserAdvance(parser);
					if (parser->character != 'D') { err = kFskErrBadData; goto bail; }
					KprCodeParserAdvance(parser);
					if (parser->character != 'A') { err = kFskErrBadData; goto bail; }
					KprCodeParserAdvance(parser);
					if (parser->character != 'T') { err = kFskErrBadData; goto bail; }
					KprCodeParserAdvance(parser);
					if (parser->character != 'A') { err = kFskErrBadData; goto bail; }
					KprCodeParserAdvance(parser);
					if (parser->character != '[') { err = kFskErrBadData; goto bail; }
					KprCodeParserAdvance(parser);
					bailIfError(fxParseXMLData(parser));
				}
				else { err = kFskErrBadData; goto bail; }
			}
			else
				bailIfError(fxParseXMLStartTag(parser));
			bailIfError(KprCodeParserColorAt(parser, 0, parser->input));
		}
		else
			KprCodeParserAdvance(parser);
	}
bail:
	if ((err == kFskErrNone) || (err == kFskErrBadData))
		err = KprCodeParserEnd(self, parser);
	return err;
}

Boolean fxIsXMLIdentifierFirst(UInt32 c)
{
	return ((('A' <= c) && (c <= 'Z')) || (('a' <= c) && (c <= 'z')) || (c == '_') || (c == ':')) ? 1 : 0;
}

Boolean fxIsXMLIdentifierNext(UInt32 c)
{
	return ((('0' <= c) && (c <= '9')) || (('A' <= c) && (c <= 'Z')) || (('a' <= c) && (c <= 'z')) || (c == '_') || (c == ':') || (c == '-')) ? 1 : 0;
}

FskErr fxParseXMLComment(KprCodeParser* parser)
{
	FskErr err = kFskErrNone;
	bailIfError(KprCodeParserColorAt(parser, 3, parser->offset));
	while (parser->character) {
		bailIfError(fxParseXMLSpace(parser, NULL));
		if (parser->character == '-') {
			KprCodeParserAdvance(parser);
			if (parser->character == '-') {
				KprCodeParserAdvance(parser);
				if (parser->character == '>') {
					KprCodeParserAdvance(parser);
					break;
				}
			}
		}
		KprCodeParserAdvance(parser);
	}
bail:
	return err;
}

FskErr fxParseXMLData(KprCodeParser* parser)
{
	FskErr err = kFskErrNone;
	bailIfError(KprCodeParserColorAt(parser, 1, parser->offset));
	bailIfError(KprCodeParserColorAt(parser, 0, parser->input));
	while (parser->character) {
		bailIfError(fxParseXMLSpace(parser, NULL));
		parser->offset = parser->input;
		if (parser->character == ']') {
			KprCodeParserAdvance(parser);
			if (parser->character == ']') {
				KprCodeParserAdvance(parser);
				if (parser->character == '>') {
					KprCodeParserAdvance(parser);
					bailIfError(KprCodeParserColorAt(parser, 1, parser->offset));
					break;
				}
			}
		}
		else
			KprCodeParserAdvance(parser);
	}
bail:
	return err;
}

FskErr fxParseXMLName(KprCodeParser* parser)
{
	if (fxIsXMLIdentifierFirst(parser->character)) {
		KprCodeParserAdvance(parser);
		while (fxIsXMLIdentifierNext(parser->character))
			KprCodeParserAdvance(parser);
		return kFskErrNone;
	}
	return kFskErrBadData;
}

FskErr fxParseXMLProcessingInstruction(KprCodeParser* parser)
{
	FskErr err = kFskErrNone;
	int length;
	bailIfError(KprCodeParserColorAt(parser, 1, parser->offset));
	bailIfError(fxParseXMLName(parser));
	bailIfError(fxParseXMLSpace(parser, &length));
	while ((parser->character) && (parser->character != '?')) {
		if (!length)
			return kFskErrBadData;
		bailIfError(fxParseXMLName(parser));
		bailIfError(fxParseXMLSpace(parser, NULL));
		if (parser->character != '=')
			return kFskErrBadData;
		KprCodeParserAdvance(parser);
		bailIfError(fxParseXMLSpace(parser, NULL));
		bailIfError(fxParseXMLValue(parser));
		bailIfError(fxParseXMLSpace(parser, &length));
	}
	if (parser->character != '?') return kFskErrBadData;
	KprCodeParserAdvance(parser);
	if (parser->character != '>') return kFskErrBadData;
	KprCodeParserAdvance(parser);
bail:
	return err;
}

FskErr fxParseXMLSpace(KprCodeParser* parser, int* length)
{
	FskErr err = kFskErrNone;
	int offset = parser->input;
	for (;;) {
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
		default:
            if (length)
                *length = parser->input - offset;
			return kFskErrNone;
		}
	}
bail:
	return err;
}

FskErr fxParseXMLStartTag(KprCodeParser* parser)
{
	FskErr err = kFskErrNone;
	int length;
	bailIfError(KprCodeParserColorAt(parser, 1, parser->offset));
	bailIfError(fxParseXMLName(parser));
	bailIfError(fxParseXMLSpace(parser, &length));
	while ((parser->character) && (parser->character != '/') && (parser->character != '>')) {
		if (!length)
			return kFskErrBadData;
		bailIfError(fxParseXMLName(parser));
		bailIfError(fxParseXMLSpace(parser, NULL));
		if (parser->character != '=')
			return kFskErrBadData;
		KprCodeParserAdvance(parser);
		bailIfError(fxParseXMLSpace(parser, NULL));
		bailIfError(fxParseXMLValue(parser));
		bailIfError(fxParseXMLSpace(parser, &length));
	}
	if (parser->character == '/')
		KprCodeParserAdvance(parser);
	if (parser->character != '>')
		return kFskErrBadData;
	KprCodeParserAdvance(parser);
bail:
	return err;
}

FskErr fxParseXMLStopTag(KprCodeParser* parser)
{
	FskErr err = kFskErrNone;
	bailIfError(KprCodeParserColorAt(parser, 1, parser->offset));
	bailIfError(fxParseXMLName(parser));
	bailIfError(fxParseXMLSpace(parser, NULL));
	if (parser->character != '>')
		return kFskErrBadData;
	KprCodeParserAdvance(parser);
bail:
	return err;
}

FskErr fxParseXMLValue(KprCodeParser* parser)
{
	FskErr err = kFskErrNone;
	UInt32 character = parser->character;
	if ((character != '"') && (character != '\''))
		return kFskErrBadData;
	bailIfError(KprCodeParserColorAt(parser, 2, parser->input));
	KprCodeParserAdvance(parser);
	while ((parser->character) && (parser->character != character)) {
		bailIfError(fxParseXMLSpace(parser, NULL));
		KprCodeParserAdvance(parser);
	}
	if (parser->character != character)
		return kFskErrBadData;
	KprCodeParserAdvance(parser);
	bailIfError(KprCodeParserColorAt(parser, 1, parser->input));
	parser->color = 1;
bail:
	return err;
}

