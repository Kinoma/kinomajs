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

#include "xsAll.h"

#include "expat.h"

typedef struct sxMarkupParser txMarkupParser;
struct sxMarkupParser {
	txMachine* the;
	txMarkupCallbacks* callbacks;
	XML_Parser expat;
	int offset;
	int size;
	int docType;
	char *text;
	int textSize;
};

typedef struct {
	char name[9];
	char nameLength;
	char value[5];
	char valueLength;
} txEntity;

static void fxReportMarkupError(txMachine* the, txMarkupParser* theParser);

static void fxBuildMarkupParser(txMarkupParser* theParser, txMachine* the, 
		txSlot* thePath, long theLine, txMarkupCallbacks* theCallbacks);
static void fxParseCharacter(void * data, const char *theText, int theSize);
static void fxParseComment(void * data, const XML_Char *theText);
static void fxParseDefault(void * data, const char *theText, int theSize);
static void fxParseProcessingInstruction(void * data, const char *theTarget, const char *theText);
static void fxParseStartDocType(void *data, const char *theName, const char *sysid, const char *pubid, int has_internal_subset);
static void fxSkippedEntityHandler(void *userData, const XML_Char *entityName, int is_parameter_entity);
static int fxUnknownEncodingHandler(void *encodingHandlerData, const XML_Char *name, XML_Encoding *info);

static void fxParseStartTag(void *data, const char *theName, const char **theAttributes);
static void fxParseStopDocType(void *data);
static void fxParseStopTag(void *data, const char *theName);
static void fxParseText(void *data);
static void fxPopArgument(txMachine* the);
static void fxPushArgument(txMachine* the);
static txEntity* fxSearchEntity(txString theName);

void fxReportMarkupError(txMachine* the, txMarkupParser* theParser)
{
	char* aPath;
	int aLine;
	char* aString;

	if (the->parseFlag & XS_SNIFF_FLAG)
		return;
	if (mxArgv(1)->kind == XS_STRING_KIND)
		aPath = mxArgv(1)->value.string;
	else
		aPath = C_NULL;
	aLine = XML_GetCurrentLineNumber(theParser->expat);
	aString = (char*)XML_ErrorString(XML_GetErrorCode(theParser->expat));
	if (!(the->parseFlag & XS_NO_ERROR_FLAG))
		fxReportError(the, aPath, aLine, "%s", aString);
	else if (!(the->parseFlag & XS_NO_WARNING_FLAG))
		fxReportWarning(the, aPath, aLine, "%s", aString);
	fxThrowMessage(the, XS_SYNTAX_ERROR, aString);
}

void fxParseMarkup(txMachine* the, void* theStream, txGetter theGetter, 
		txSlot* thePath, long theLine, txMarkupCallbacks* theCallbacks)
{
#define mxBufferSize 1024
	txMarkupParser aParser;
	char* aBuffer;
	int i;
	char* p;
	int c;
	
	mxTry(the) {
		fxBuildMarkupParser(&aParser, the, thePath, theLine, theCallbacks);
		for (;;) {
			aBuffer = (char *)XML_GetBuffer(aParser.expat, mxBufferSize);
			if (!aBuffer)
				mxDebug0(the, XS_UNKNOWN_ERROR, "cannot allocate expat buffer");
			for (i = 0, p = aBuffer; i < mxBufferSize; i++, p++) {
				c = (*theGetter)(theStream);
				if (c == C_EOF)
					break;
				*p = (char)c;
			}
			if (!XML_ParseBuffer(aParser.expat, i, (i < mxBufferSize) ? 1 : 0)) {
				fxReportMarkupError(the, &aParser);
				break;
			}
			if (i < mxBufferSize)
				break;
		}
		XML_ParserFree(aParser.expat);
#if __FSK_LAYER__
		c_free(aParser.text);
#endif
	}
	mxCatch(the) {
#if __FSK_LAYER__
		c_free(aParser.text);
#endif
		XML_ParserFree(aParser.expat);
		fxJump(the);
	}
}

void fxParseMarkupBuffer(txMachine* the, void* theBuffer, txSize theSize, 
		txSlot* thePath, long theLine, txMarkupCallbacks* theCallbacks)
{
	txMarkupParser aParser;
	
	mxTry(the) {
		fxBuildMarkupParser(&aParser, the, thePath, theLine, theCallbacks);
		if (!XML_Parse(aParser.expat, (const char *)theBuffer, theSize, 1)) {
			fxReportMarkupError(the, &aParser);
		}
#if __FSK_LAYER__
		c_free(aParser.text);
#endif
		XML_ParserFree(aParser.expat);
	}
	mxCatch(the) {
#if __FSK_LAYER__
		c_free(aParser.text);
#endif
		XML_ParserFree(aParser.expat);
		fxJump(the);
	}
}

void fxBuildMarkupParser(txMarkupParser* theParser, txMachine* the, 
		txSlot* thePath, long theLine, txMarkupCallbacks* theCallbacks)
{
	c_memset(theParser, 0, sizeof(txMarkupParser));
	theParser->the = the;
	if (thePath) {
		mxArgv(1)->value.string = thePath->value.symbol.string;
		mxArgv(1)->kind = XS_STRING_KIND;
	}
	mxArgv(2)->value.integer = theLine;
	mxArgv(2)->kind = XS_INTEGER_KIND;
	theParser->callbacks = theCallbacks;
	theParser->expat = XML_ParserCreate(NULL);
	if (!theParser->expat)
		mxDebug0(the, XS_UNKNOWN_ERROR, "cannot allocate expat");
	XML_SetUserData(theParser->expat, theParser);
	XML_SetElementHandler(theParser->expat, fxParseStartTag, fxParseStopTag);
	XML_SetCharacterDataHandler(theParser->expat, fxParseCharacter);
	XML_SetCommentHandler(theParser->expat, fxParseComment);
	XML_SetProcessingInstructionHandler(theParser->expat, fxParseProcessingInstruction);
	XML_SetDefaultHandlerExpand(theParser->expat, fxParseDefault);
	XML_SetDoctypeDeclHandler(theParser->expat, fxParseStartDocType, fxParseStopDocType);
	XML_SetUnknownEncodingHandler(theParser->expat, fxUnknownEncodingHandler, NULL);
	XML_SetSkippedEntityHandler(theParser->expat, fxSkippedEntityHandler);
	XML_UseForeignDTD(theParser->expat, XML_TRUE);
}

void fxParseCharacter(void * data, const char *theText, int theSize)
{
#if __FSK_LAYER__
	txMarkupParser* aParser = (txMarkupParser*)data;
	txMachine* the = aParser->the;
	int aSize;
	char* aText;

	if (!aParser->offset) {
		mxArgv(2)->value.integer = XML_GetCurrentLineNumber(aParser->expat);
		
		fxNewInstance(the);
		*(--the->stack) = *mxArgv(0);
		fxQueueID(the, the->parentID, XS_GET_ONLY);
		
		aSize = theSize + 1;
		do 
			aParser->size += 1024; 
		while (aSize > aParser->size);
		aText = (char *)fxNewChunk(the, aParser->size);

		mxZeroSlot(--the->stack);
		the->stack->value.string = aText;
		the->stack->kind = XS_STRING_KIND;
		fxQueueID(the, the->valueID, XS_GET_ONLY);
		
		fxPushArgument(the);
	}

	aSize = aParser->offset + theSize + 1;
	if (aSize > aParser->textSize) {
		aParser->text = c_realloc(aParser->text, (aSize + 8191) & ~0x01fff);
		aParser->textSize = (aSize + 8191) & ~0x01fff;
	}
	c_memmove(aParser->text + aParser->offset, theText, theSize);
	aParser->offset += theSize;
	aParser->text[aParser->offset] = 0;
#else
	txMarkupParser* aParser = (txMarkupParser*)data;
	txMachine* the = aParser->the;
	int aSize;
	char* aText;
	txSlot* aSlot;

	if (!aParser->offset) {
		mxArgv(2)->value.integer = XML_GetCurrentLineNumber(aParser->expat);
		
		fxNewInstance(the);
		*(--the->stack) = *mxArgv(0);
		fxQueueID(the, the->parentID, XS_GET_ONLY);
		
		aSize = theSize + 1;
		do 
			aParser->size += 1024; 
		while (aSize > aParser->size);
		aText = (char *)fxNewChunk(the, aParser->size);

		mxZeroSlot(--the->stack);
		the->stack->value.string = aText;
		the->stack->kind = XS_STRING_KIND;
		fxQueueID(the, the->valueID, XS_GET_ONLY);
		
		fxPushArgument(the);
	}
	else {
		aSize = aParser->offset + theSize + 1;
		if (aSize > aParser->size) {
			do 
				aParser->size += 1024; 
			while (aSize > aParser->size);

			aSlot = mxArgv(0)->value.reference->next->next;
			aText = (char *)fxRenewChunk(the, aSlot->value.string, aParser->size);
			if (!aText) {
				aText = (char *)fxNewChunk(the, aParser->size);
				c_memcpy(aText, aSlot->value.string, aParser->offset);
				aSlot->value.string = aText;
			}

		}
	}
	aSlot = mxArgv(0)->value.reference->next->next;
	aText = aSlot->value.string;
	c_memcpy(aText + aParser->offset, theText, theSize);
	aParser->offset += theSize;
	aText[aParser->offset] = 0;
#endif
}

void fxParseComment(void * data, const char *theText)
{
	txMarkupParser* aParser = (txMarkupParser*)data;
	txMachine* the = aParser->the;
	char* aText;
	int aSize;
		
	if (aParser->callbacks->processComment) {
		mxArgv(2)->value.integer = XML_GetCurrentLineNumber(aParser->expat);
			
		fxNewInstance(the);
		*(--the->stack) = *mxArgv(0);
		fxQueueID(the, the->parentID, XS_GET_ONLY);
			
		mxZeroSlot(--the->stack);
		aSize = c_strlen(theText);
		aText = the->stack->value.string = (txString)fxNewChunk(aParser->the, aSize + 1);
		the->stack->kind = XS_STRING_KIND;
		fxQueueID(the, the->valueID, XS_GET_ONLY);
		c_memcpy(aText, theText, aSize);
		aText[aSize] = 0;
			
		fxPushArgument(the);
		(*(aParser->callbacks->processComment))(the);
		fxPopArgument(the);
	}
}

void fxParseDefault(void *data, const char *theText, int theSize)
{
	txMarkupParser* aParser = (txMarkupParser*)data;

	if ((*theText == '&') && (theText[theSize - 1] == ';')) {
		*((char*)theText + theSize - 1) = 0;
		fxSkippedEntityHandler(data, theText + 1, 0);
		*((char*)theText + theSize - 1) = ';';
	}
	else if (aParser->docType) {
		fxParseCharacter(data, theText, theSize);
	}
}

static void fxParseProcessingInstruction(void * data, const char *theTarget, const char *theText)
{
	txMarkupParser* aParser = (txMarkupParser*)data;
	txMachine* the = aParser->the;
	char* aText;
	int aSize;
		
	if (aParser->callbacks->processProcessingInstruction) {
		fxParseText(data);

		mxArgv(2)->value.integer = XML_GetCurrentLineNumber(aParser->expat);
			
		fxNewInstance(the);
		*(--the->stack) = *mxArgv(0);
		fxQueueID(the, the->parentID, XS_GET_ONLY);
			
		mxZeroSlot(--the->stack);
		aSize = c_strlen(theTarget);
		aText = the->stack->value.string = (txString)fxNewChunk(aParser->the, aSize + 1);
		the->stack->kind = XS_STRING_KIND;
		fxQueueID(the, the->nameID, XS_GET_ONLY);
		c_memcpy(aText, theTarget, aSize);
		aText[aSize] = 0;
			
		mxZeroSlot(--the->stack);
		aSize = c_strlen(theText);
		aText = the->stack->value.string = (txString)fxNewChunk(aParser->the, aSize + 1);
		the->stack->kind = XS_STRING_KIND;
		fxQueueID(the, the->valueID, XS_GET_ONLY);
		c_memcpy(aText, theText, aSize);
		aText[aSize] = 0;

		fxPushArgument(the);
		(*(aParser->callbacks->processProcessingInstruction))(the);
		fxPopArgument(the);
	}
}

void fxParseStartDocType(void *data, const char *theName, const char *sysid, const char *pubid, int has_internal_subset)
{
	txMarkupParser* aParser = (txMarkupParser*)data;

	aParser->docType++;
}

void fxParseStartTag(void *data, const char *theName, const char **theAttributes)
{
	txMarkupParser* aParser = (txMarkupParser*)data;
	txMachine* the = aParser->the;
	char** aPointer;
	txID anIndex;
	
	fxParseText(data);

	mxArgv(2)->value.integer = XML_GetCurrentLineNumber(aParser->expat);
	
	fxNewInstance(the);
	*(--the->stack) = *mxArgv(0);
	fxQueueID(the, the->parentID, XS_GET_ONLY);
	mxPushStringC((char*)theName);
	fxQueueID(the, the->nameID, XS_GET_ONLY);
	
	fxNewInstance(the);
	aPointer = (char**)theAttributes;
	anIndex = 0;
	while (*aPointer) {
		fxNewInstance(the);
		mxPushStringC(*aPointer);
		fxQueueID(the, the->nameID, XS_GET_ONLY);
		aPointer++;
		mxPushStringC(*aPointer);
		fxQueueID(the, the->valueID, XS_GET_ONLY);
		aPointer++;
		fxQueueID(the, anIndex, XS_GET_ONLY);
		anIndex++;
	}
	fxQueueID(the, the->valueID, XS_GET_ONLY);
	
	fxPushArgument(the);
	
	if (aParser->callbacks->processStartTag)
		(*(aParser->callbacks->processStartTag))(the);
}

void fxParseStopDocType(void *data)
{
	txMarkupParser* aParser = (txMarkupParser*)data;
	txMachine* the = aParser->the;
	txSlot* aSlot;
//@@ not dealing with FSK_LAYER case
	if (aParser->offset) {
		aSlot = mxArgv(0)->value.reference->next->next;
		fxRenewChunk(the, aSlot->value.string, aParser->offset + 1);
		if (aParser->callbacks->processDoctype)
			(*(aParser->callbacks->processDoctype))(the);
		fxPopArgument(the);
		aParser->offset = 0;
		aParser->size = 0;
	}
	
	aParser->docType--;
}

void fxParseStopTag(void *data, const char *theName)
{
	txMarkupParser* aParser = (txMarkupParser*)data;
	txMachine* the = aParser->the;

	fxParseText(data);
	
	mxArgv(2)->value.integer = XML_GetCurrentLineNumber(aParser->expat);
	if (aParser->callbacks->processStopTag)
		(*(aParser->callbacks->processStopTag))(the);
	fxPopArgument(the);
}

void fxParseText(void *data)
{
	txMarkupParser* aParser = (txMarkupParser*)data;
	txMachine* the = aParser->the;
	txSlot* aSlot;
	char* aText;

	if (aParser->offset) {
		aSlot = mxArgv(0)->value.reference->next->next;
		aText = fxRenewChunk(the, aSlot->value.string, aParser->offset + 1);
#if __FSK_LAYER__
		if (!aText)
			aSlot->value.string = (char *)fxNewChunk(the, aParser->offset + 1);

		c_memmove(aSlot->value.string, aParser->text, aParser->offset + 1);
#endif
		if (aParser->callbacks->processText)
			(*(aParser->callbacks->processText))(the);
		fxPopArgument(the);
		aParser->offset = 0;
		aParser->size = 0;
	}
}

static int gWindows1252[] = {
			0x20AC, 0x0000, 0x201A, 0x0192, 0x201E, 0x2026, 0x2020, 0x2021, 0x02C6, 0x2030, 0x0160, 0x2039, 0x0152, 0x0000, 0x017D, 0x0000,

			0x0000, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014, 0x02DC, 0x2122, 0x0161, 0x203A, 0x0153, 0x0000, 0x017E, 0x0178,

			0x00A0, 0x00A1, 0x00A2, 0x00A3, 0x00A4, 0x00A5, 0x00A6, 0x00A7, 0x00A8, 0x00A9, 0x00AA, 0x00AB, 0x00AC, 0x00AD, 0x00AE, 0x00AF,

			0x00B0, 0x00B1, 0x00B2, 0x00B3, 0x00B4, 0x00B5, 0x00B6, 0x00B7, 0x00B8, 0x00B9, 0x00BA, 0x00BB, 0x00BC, 0x00BD, 0x00BE, 0x00BF,

			0x00C0, 0x00C1, 0x00C2, 0x00C3, 0x00C4, 0x00C5, 0x00C6, 0x00C7, 0x00C8, 0x00C9, 0x00CA, 0x00CB, 0x00CC, 0x00CD, 0x00CE, 0x00CF,

			0x00D0, 0x00D1, 0x00D2, 0x00D3, 0x00D4, 0x00D5, 0x00D6, 0x00D7, 0x00D8, 0x00D9, 0x00DA, 0x00DB, 0x00DC, 0x00DD, 0x00DE, 0x00DF,

			0x00E0, 0x00E1, 0x00E2, 0x00E3, 0x00E4, 0x00E5, 0x00E6, 0x00E7, 0x00E8, 0x00E9, 0x00EA, 0x00EB, 0x00EC, 0x00ED, 0x00EE, 0x00EF,

			0x00F0, 0x00F1, 0x00F2, 0x00F3, 0x00F4, 0x00F5, 0x00F6, 0x00F7, 0x00F8, 0x00F9, 0x00FA, 0x00FB, 0x00FC, 0x00FD, 0x00FE, 0x00FF};

static int gISO8895_15[] = {
			0x00A0, 0x00A1, 0x00A2, 0x00A3, 0x20AC, 0x00A5, 0x0160, 0x00A7, 0x0161, 0x00A9, 0x00AA, 0x00AB, 0x00AC, 0x00AD, 0x00AE, 0x00AF,

			0x00B0, 0x00B1, 0x00B2, 0x00B3, 0x017D, 0x00B5, 0x00B6, 0x00B7, 0x017E, 0x00B9, 0x00BA, 0x00BB, 0x0152, 0x0153, 0x0178, 0x00BF,

			0x00C0, 0x00C1, 0x00C2, 0x00C3, 0x00C4, 0x00C5, 0x00C6, 0x00C7, 0x00C8, 0x00C9, 0x00CA, 0x00CB, 0x00CC, 0x00CD, 0x00CE, 0x00CF,

			0x00D0, 0x00D1, 0x00D2, 0x00D3, 0x00D4, 0x00D5, 0x00D6, 0x00D7, 0x00D8, 0x00D9, 0x00DA, 0x00DB, 0x00DC, 0x00DD, 0x00DE, 0x00DF,

			0x00E0, 0x00E1, 0x00E2, 0x00E3, 0x00E4, 0x00E5, 0x00E6, 0x00E7, 0x00E8, 0x00E9, 0x00EA, 0x00EB, 0x00EC, 0x00ED, 0x00EE, 0x00EF,

			0x00F0, 0x00F1, 0x00F2, 0x00F3, 0x00F4, 0x00F5, 0x00F6, 0x00F7, 0x00F8, 0x00F9, 0x00FA, 0x00FB, 0x00FC, 0x00FD, 0x00FE, 0x00FF};

int fxUnknownEncodingHandler(void *encodingHandlerData, const XML_Char *name, XML_Encoding *info)
{
	int i;

	if (0 == c_strcmp(name, "windows-1252")) {
		for (i = 0; i < 128; i++)
			info->map[i] = i;
		for (i = 128; i < 255; i++)
			info->map[i] = gWindows1252[i - 128];
	}
	else
	if (0 == c_strcmp(name, "iso-8859-15")) {
		for (i = 0; i < 0x0a0; i++)
			info->map[i] = i;
		for (i = 0x0a0; i < 255; i++)
			info->map[i] = gISO8895_15[i - 0x0a0];
	}
	else
		return XML_STATUS_ERROR;

	return XML_STATUS_OK;
}

void fxSkippedEntityHandler(void *userData, const XML_Char *entityName, int is_parameter_entity)
{
	txMarkupParser* aParser = (txMarkupParser*)userData;
	txMachine* the = aParser->the;
	txEntity* anEntity;

	anEntity = fxSearchEntity((char*)entityName);
	if (anEntity) {
		fxParseCharacter(userData, anEntity->value, anEntity->valueLength);
	}
	else {
		fxReportError(the, mxArgv(1)->value.string, mxArgv(2)->value.integer, 
				"entity not found: %s", entityName);
		mxDebug0(the, XS_SYNTAX_ERROR, "XML error");
	}
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
