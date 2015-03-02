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
#include "kxMarkup.h"

#include "FskMemory.h"
#include "FskString.h"
#include "FskTextConvert.h"

#define c_memset FskMemSet
#define c_malloc FskMemPtrAlloc
#define c_realloc FskMemPtrReallocC
#define c_free FskMemPtrDispose

#define mxValueSize (1024)
#define mxStartTagBufferSize (32768)
#define mxAttributeCount (64)
typedef struct sxMarkupParser txMarkupParser;
struct sxMarkupParser {
	void* stream;
	int (*streamGetter)(void*);
	char *path;
	int line;
	long characterEncoding;
	txMarkupCallbacks* callbacks;
	int c;
	int c0;
	int c1;
	int (*getter)(txMarkupParser*);
	int depth;
	char* entity;
	char name[1024];
	char value[mxValueSize];
	void *clientState;
	Boolean deviceCantTextConvert;
	Boolean failed;
	txAttribute attributes[mxAttributeCount];
	char startTagBuffer[mxStartTagBufferSize];
};

static void kxGetCharacter(txMarkupParser* theParser);
static txU4 convertOne(txMarkupParser *theParser, const unsigned char *src, UInt16 srcLen, UInt32 srcEncoding);
static int kxGetFirstEntityCharacter(txMarkupParser* theParser);
//static int kxGetNextCommentCharacter(txMarkupParser* theParser);
static int kxGetNextDataCharacter(txMarkupParser* theParser);
//static int kxGetNextDocTypeCharacter(txMarkupParser* theParser);
static int kxGetNextEntityCharacter(txMarkupParser* theParser);
static int kxGetNextTextCharacter(txMarkupParser* theParser);
static int kxGetNextProcessingInstructionCharacter(txMarkupParser* theParser);
//static int kxGetNextScriptCharacter(txMarkupParser* theParser);
static int kxGetNextValueCharacter(txMarkupParser* theParser);
static void kxParseComment(txMarkupParser* theParser);
static void kxParseDocument(txMarkupParser* theParser);
static void kxParseDocType(txMarkupParser* theParser);
static char *kxParseName(txMarkupParser* theParser);
static void kxParseProcessingInstruction(txMarkupParser* theParser);
static void kxParseStartTag(txMarkupParser* theParser);
static void kxParseStopTag(txMarkupParser* theParser);
static void kxParseText(txMarkupParser* theParser);
static char *kxParseValue(txMarkupParser* theParser, char *buffer, UInt32 bufferSize);
//static void kxPopArgument(txMachine* the);
//static void kxPushArgument(txMachine* the);
static void kxReportMarkupError(txMarkupParser* theParser, char* theFormat, ...);
static int kxScanName(txMarkupParser* theParser, char* theName, UInt32 theSize);
static int kxScanSpace(txMarkupParser* theParser);
static void kxSkipComment(txMarkupParser* theParser);
static void kxSkipDeclaration(txMarkupParser* theParser);
static void kxSkipDocType(txMarkupParser* theParser);
static void kxSkipProcessingInstruction(txMarkupParser* theParser);

typedef int (*txGetter)(void*);


static int kxStringCGetter(void* theStream);
static void kxParseMarkup(void* theStream, txGetter theGetter, 
		char* thePath, long theLine, long characterEncoding, txMarkupCallbacks* theCallbacks, void *state);

int kxStringCGetter(void* theStream)
{
	txStringCStream* aStream = theStream;
	
	if (aStream->offset < aStream->size) 
		return *(unsigned char *)(aStream->buffer + aStream->offset++);

	return C_EOF;
}

void kxParseMarkup(void* theStream, txGetter theGetter, 
		char* thePath, long theLine, long characterEncoding, txMarkupCallbacks* theCallbacks, void *state)
{
	txMarkupParser *aParser;

	aParser = (txMarkupParser *)c_malloc(sizeof(txMarkupParser));
	c_memset(aParser, 0, sizeof(txMarkupParser));
	aParser->stream = theStream;
	aParser->streamGetter = theGetter;
	aParser->path = thePath;
	aParser->line = theLine;
	aParser->characterEncoding = characterEncoding;
	aParser->callbacks = theCallbacks;
	aParser->clientState = state;
	
	kxParseDocument(aParser);
	
	c_free(aParser);
}

void kxParseMarkupBuffer(void* theBuffer, txSize theSize, 
		char* thePath, long theLine, txMarkupCallbacks* theCallbacks, void *state)
{
	txStringCStream aStream;
	unsigned char *p = theBuffer;
	long characterEncoding;

	characterEncoding = charEncodingISO8859_1;
	if (theSize >= 3) {
		if ((0xef == p[0]) && (0xbb == p[1]) && (0xbf == p[2])) {
			theBuffer = p + 3;
			theSize -= 3;
			characterEncoding = charEncodingUTF8;
		}
		else
		if ((0xfe == p[0]) && (0xff == p[1])) {
			theBuffer = p + 2;
			theSize -= 2;
			characterEncoding = charEncodingUTF16BE;
		}
		else
		if ((0xff == p[0]) && (0xfe == p[1])) {
			theBuffer = p + 2;
			theSize -= 2;
			characterEncoding = charEncodingUTF16LE;
		}
	}

	aStream.buffer = theBuffer;
	aStream.offset = 0;
	aStream.size = theSize;
    
	if ((charEncodingISO8859_1 == characterEncoding) && (theSize >= 3)) {
		// sniff entire stream here to try to differentiate unlabeled UTF8 and Latin 1
		txSize count = theSize - 3;
		unsigned char *pp = p;
		characterEncoding = charEncodingUTF8;
		while (count) {
			unsigned char c = *pp++;
			count -= 1;
			if (0 == (c & 0x80))		// 1 byte
				continue;

			if (c < 0xc0) {
				characterEncoding = charEncodingISO8859_1;
				break;
			}
			else if (c < 0xe0) {
				if (0x80 == (pp[0] & 0xc0)) {
					pp += 1;			// 2 byte
					count -= 1;
				}
				else {
					characterEncoding = charEncodingISO8859_1;
					break;
				}
			}
			else {
				if ((0x80 == (pp[0] & 0xc0)) && (0x80 == (pp[1] & 0xc0))) {
					pp += 2;			// 3 byte
					count -= 2;
				}
				else {
					characterEncoding = charEncodingISO8859_1;
					break;
				}
			}
		}
	}

	kxParseMarkup(&aStream, kxStringCGetter, thePath, theLine, characterEncoding, theCallbacks, state);
}

typedef struct {
	txS4 size;
	txU4 cmask;
	txU4 cval;
	txS4 shift;
	txU4 lmask;
	txU4 lval;
} xUTF8Sequence;

static xUTF8Sequence xUTF8Sequences[] = {
	{1, 0x80, 0x00, 0*6, 0x0000007F, 0x00000000},
	{2, 0xE0, 0xC0, 1*6, 0x000007FF, 0x00000080},
	{3, 0xF0, 0xE0, 2*6, 0x0000FFFF, 0x00000800},
	{4, 0xF8, 0xF0, 3*6, 0x001FFFFF, 0x00010000},
	{5, 0xFC, 0xF8, 4*6, 0x03FFFFFF, 0x00200000},
	{6, 0xFE, 0xFC, 5*6, 0x7FFFFFFF, 0x04000000},
	{0, 0, 0, 0, 0, 0},
};

void kxGetCharacter(txMarkupParser* theParser)
{
	txU4 aResult;
	xUTF8Sequence *aSequence;
	txInteger aSize;
	UInt32 srcEncoding = theParser->characterEncoding;
	unsigned char src[2];
	UInt16 srcLen;

	aResult = (txU4)(*(theParser->streamGetter))(theParser->stream);
	if (((SInt32)aResult < 0x80) && ((charEncodingUTF8 == srcEncoding) || (charEncodingISO8859_1 == srcEncoding))) {
		theParser->c = aResult;
		return;
	}

	if (aResult == (txU4)C_EOF) {
		theParser->c = C_EOF;
		return;
	}

	if (theParser->deviceCantTextConvert) {
		theParser->c = aResult;
		return;
	}

	switch (srcEncoding) {
		default:
		case charEncodingISO8859_1:
			src[0] = (UInt8)aResult;
			srcLen = 1;
			break;

		case charEncodingUTF8:
			for (aSequence = xUTF8Sequences; aSequence->size; aSequence++) {
				if ((aResult & aSequence->cmask) == aSequence->cval)
					break;
			}
			if (aSequence->size == 0)
				kxReportMarkupError(theParser, "invalid UTF-8 character");
			aSize = aSequence->size - 1;
			while (aSize) {
				aSize--;
				aResult = (aResult << 6) | ((*(theParser->streamGetter))(theParser->stream) & 0x3F);
			}
			aResult &= aSequence->lmask;
			src[0] = (UInt8)(aResult >> 8);
			src[1] = (UInt8)aResult;
			srcLen = 2;
			srcEncoding = charEncodingUTF16BE;
			break;

		case charEncodingUTF16BE:
			aResult = (aResult << 8) | ((*(theParser->streamGetter))(theParser->stream) & 0xff);
			src[0] = (UInt8)(aResult >> 8);
			src[1] = (UInt8)aResult;
			srcLen = 2;
			srcEncoding = charEncodingUTF16BE;
			break;
			
		case charEncodingUTF16LE:
			aResult = aResult | (((*(theParser->streamGetter))(theParser->stream) & 0xff) << 8);
			src[0] = (UInt8)(aResult >> 8);
			src[1] = (UInt8)aResult;
			srcLen = 2;
			srcEncoding = charEncodingUTF16BE;
			break;
	}

	theParser->c = convertOne(theParser, src, srcLen, srcEncoding);
}

txU4 convertOne(txMarkupParser *theParser, const unsigned char *src, UInt16 srcLen, UInt32 srcEncoding)
{
	UInt16 saveSrcLen;
	txU4 output;
	unsigned char *result = NULL;

	if (1 == srcLen) {
		if (src[0] < 0x80)
			return src[0];
	}
	else
	if ((2 == srcLen) && (charEncodingUTF16BE == srcEncoding)) {
		if ((0 == src[0]) && (src[1] < 0x80))
			return src[1];
	}

	saveSrcLen = srcLen;

	switch (srcEncoding) {
		case charEncodingISO8859_1:
			FskTextLatin1ToUTF8((const char *)src, srcLen, (char **)(void *)&result, NULL);
			break;
		case charEncodingUTF8:
			result = (unsigned char *)FskStrDoCopy((const char *)src);
			break;
		case charEncodingUTF16BE:
			FskTextUnicode16BEToUTF8((UInt16 *)src, srcLen, (char **)(void *)&result, NULL);
			break;
		case charEncodingUTF16LE:
			FskTextUnicode16LEToUTF8((UInt16 *)src, srcLen, (char **)(void *)&result, NULL);
			break;
		default:
			result = NULL;		// should never happen
			break;

	}

	if (NULL != result) {
		UInt16 outputBuffer[2];
		UInt32 outputBufferSize = sizeof(outputBuffer);
		FskTextUTF8ToUnicode16NENoAlloc(result, FskStrLen((char *)result), outputBuffer, &outputBufferSize);
		output = outputBuffer[0];
		FskMemPtrDispose(result);
	}
	else {
		if (1 == saveSrcLen)
			output = src[0];
		else
			output = '?';
	}

	return output;
}

typedef struct {
	char name[9];
	char nameLength;
	char value[5];				// utf8
	char valueLength;
} txEntity;

static txEntity kxEntities[] = {
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
	{"zwnj", 4, "\xe2\x80\x8c", 3}, {"", 0, "", 0}
};

int kxGetFirstEntityCharacter(txMarkupParser* theParser)
{
	int c;
	char* p;

	kxGetCharacter(theParser);
	if (theParser->c == '#') {
		kxGetCharacter(theParser);
		if (theParser->c == 'x') {
			kxGetCharacter(theParser);
			p = theParser->name;
			while ((('0' <= theParser->c) && (theParser->c <= '9')) 
					|| (('A' <= theParser->c) && (theParser->c <= 'F'))
					|| (('a' <= theParser->c) && (theParser->c <= 'f'))) {
				*p++ = theParser->c;
				kxGetCharacter(theParser);
			}
			*p = 0;
			c = FskStrHexToNum(theParser->name, FskStrLen(theParser->name));
		}
		else {
			p = theParser->name;
			while (('0' <= theParser->c) && (theParser->c <= '9'))  {
				*p++ = theParser->c;
				kxGetCharacter(theParser);
			}
			*p = 0;
			c = FskStrToNum(theParser->name);
		}
	}
	else {
		const txEntity *walker;

		kxScanName(theParser, theParser->name, sizeof(theParser->name));

		c = '&';			// used if entity is unknown - we use & because in ASF files an ampersand may appear in a text element without implying an entity
		if (*theParser->name) {
			char entityLen = (char)FskStrLen(theParser->name);
			theParser->entity = theParser->name;			// if entity is unknown, we just return the name as the value

			for (walker = kxEntities; 0 != walker->nameLength; walker += 1) {
				if ((entityLen == walker->nameLength) && (0 == FskStrCompare(walker->name, theParser->name))) {
					c = convertOne(theParser, (unsigned char *)walker->value, (UInt16)FskStrLen(walker->value), charEncodingUTF8);
					FskStrCopy(theParser->name, walker->value);
					theParser->entity = NULL;
					break;
				}
			}
		}
	}
	if (theParser->c == ';')
		kxGetCharacter(theParser);
#if 0
	else
		kxReportMarkupError(theParser, "missing ;");
#endif
	return c;
}

#if 0
int kxGetNextCommentCharacter(txMarkupParser* theParser)
{
	int c;
	if ((theParser->c0 == '-') && (theParser->c1 == '-') && (theParser->c == '>'))
		c = C_EOF;
	else {
		c = theParser->c0;
		theParser->c0 = theParser->c1;
		theParser->c1 = theParser->c;
		kxGetCharacter(theParser);
		if ((c == 10) || ((c == 13) && (theParser->c0 != 10)))
			theParser->line++;
	}
	return c;
}
#endif

int kxGetNextDataCharacter(txMarkupParser* theParser)
{
	int c;
	if ((theParser->c0 == ']') && (theParser->c1 == ']') && (theParser->c == '>'))
		c = C_EOF;
	else {
		c = theParser->c0;
		theParser->c0 = theParser->c1;
		theParser->c1 = theParser->c;
		kxGetCharacter(theParser);
		if ((c == 10) || ((c == 13) && (theParser->c0 != 10)))
			theParser->line++;
	}
	return c;
}

#if 0
int kxGetNextDocTypeCharacter(txMarkupParser* theParser)
{
	int c;
	if ((theParser->c == '[') || (theParser->c == '>'))
		c = C_EOF;
	else {
		c = theParser->c;
		kxGetCharacter(theParser);
		if ((c == 10) || ((c == 13) && (theParser->c != 10)))
			theParser->line++;
	}
	return c;
}
#endif

int kxGetNextProcessingInstructionCharacter(txMarkupParser* theParser)
{
	int c;
	if ((theParser->c0 == '?') && (theParser->c == '>'))
		c = C_EOF;
	else {
		c = theParser->c0;
		theParser->c0 = theParser->c;
		kxGetCharacter(theParser);
		if ((c == 10) || ((c == 13) && (theParser->c0 != 10)))
			theParser->line++;
	}
	return c;
}

int kxGetNextEntityCharacter(txMarkupParser* theParser)
{
	txU1* p;
	txU4 c;
	
	p = (txU1*)(theParser->entity);
	c = (txU4)*p++;
	if (*p)
		theParser->entity = (txString)p;
	else
		theParser->entity = NULL;

	return (int)c;
}

#if 0
int kxGetNextScriptCharacter(txMarkupParser* theParser)
{
	int c;
	if ((theParser->c0 == '<') && (theParser->c == '/'))
		c = C_EOF;
	else {
		c = theParser->c0;
		theParser->c0 = theParser->c;
		kxGetCharacter(theParser);
		if ((c == 10) || ((c == 13) && (theParser->c0 != 10)))
			theParser->line++;
	}
	return c;
}
#endif

int kxGetNextTextCharacter(txMarkupParser* theParser)
{
	int c;
	if (theParser->entity)
		c = kxGetNextEntityCharacter(theParser);
	else if (theParser->c == '&')
		c = kxGetFirstEntityCharacter(theParser);
	else 
		if (theParser->c == '<')
		c = C_EOF;
	else {
		c = theParser->c;
		kxGetCharacter(theParser);
		if ((c == 10) || ((c == 13) && (theParser->c != 10)))
			theParser->line++;
	}
	return c;
}

int kxGetNextValueCharacter(txMarkupParser* theParser)
{
	int c;

	if (theParser->entity)
		c = kxGetNextEntityCharacter(theParser);
	else if (theParser->c == '&')
		c = kxGetFirstEntityCharacter(theParser);
	else

	if (theParser->c == theParser->c0)
		c = C_EOF;
	else {
		c = theParser->c;
		kxGetCharacter(theParser);
		if ((c == 10) || ((c == 13) && (theParser->c != 10)))
			theParser->line++;
	}
	return c;
}

void kxParseComment(txMarkupParser* theParser)
{
#if 0
	//@@ no comment callbacks
	if (theParser->callbacks->processComment) {
		txMachine* the = theParser->the;

		theParser->c0 = theParser->c;
		kxGetCharacter(theParser);
		if ((theParser->c == 10) || ((theParser->c == 13) && (theParser->c0 != 10)))
			theParser->line++;
		theParser->c1 = theParser->c;
		kxGetCharacter(theParser);
		if ((theParser->c == 10) || ((theParser->c == 13) && (theParser->c0 != 10)))
			theParser->line++;
		theParser->getter = kxGetNextCommentCharacter;

		mxArgv(2)->value.integer = theParser->line;
		if (!mxIsReference(mxArgv(0)))
			kxReportMarkupError(theParser, "invalid data");
		kxNewInstance(the);
		*(--the->stack) = *mxArgv(0);
		kxQueueID(the, the->parentID, XS_GET_ONLY);
		kxParseValue(theParser);
		kxQueueID(the, the->valueID, XS_GET_ONLY);
		kxPushArgument(the);
		(*(theParser->callbacks->processComment))(the);
		kxPopArgument(the);
		if (theParser->c != '>')
			kxReportMarkupError(theParser, "missing -->");
		kxGetCharacter(theParser);
	}
	else 
#endif
		kxSkipComment(theParser);
}

void kxParseDocument(txMarkupParser* theParser)
{
	kxGetCharacter(theParser);
	while (false == theParser->failed) {
		if (theParser->depth == 0)
			kxScanSpace(theParser);
		if (theParser->c == C_EOF)
			break;
		else if (theParser->c == '<') {
			kxGetCharacter(theParser);
			if (theParser->c == '/') {
				kxGetCharacter(theParser);
				kxParseStopTag(theParser);
			}
			else if (theParser->c == '?') {
				kxGetCharacter(theParser);
				kxParseProcessingInstruction(theParser);
			}
			else if (theParser->c == '!') {
				kxGetCharacter(theParser);
				if (theParser->c == '-') {
					kxGetCharacter(theParser);
					if (theParser->c != '-')
						kxReportMarkupError(theParser, "missing -");
					kxGetCharacter(theParser);
					kxParseComment(theParser);
				}
				else if (theParser->c == '[') {
					kxGetCharacter(theParser);
					if (theParser->c != 'C')
						kxReportMarkupError(theParser, "missing C");
					kxGetCharacter(theParser);
					if (theParser->c != 'D')
						kxReportMarkupError(theParser, "missing D");
					kxGetCharacter(theParser);
					if (theParser->c != 'A')
						kxReportMarkupError(theParser, "missing A");
					kxGetCharacter(theParser);
					if (theParser->c != 'T')
						kxReportMarkupError(theParser, "missing T");
					kxGetCharacter(theParser);
					if (theParser->c != 'A')
						kxReportMarkupError(theParser, "missing A");
					kxGetCharacter(theParser);
					if (theParser->c != '[')
						kxReportMarkupError(theParser, "missing [");
					kxGetCharacter(theParser);
					theParser->c0 = theParser->c;
					kxGetCharacter(theParser);
					theParser->c1 = theParser->c;
					kxGetCharacter(theParser);
					theParser->getter = kxGetNextDataCharacter;
					kxParseText(theParser);
					if (theParser->c != '>')
						kxReportMarkupError(theParser, "missing ]]>");
					kxGetCharacter(theParser);
				}
				else if (theParser->depth == 0) {
					if (theParser->c != 'D')
						kxReportMarkupError(theParser, "missing D");
					kxGetCharacter(theParser);
					if (theParser->c != 'O')
						kxReportMarkupError(theParser, "missing O");
					kxGetCharacter(theParser);
					if (theParser->c != 'C')
						kxReportMarkupError(theParser, "missing C");
					kxGetCharacter(theParser);
					if (theParser->c != 'T')
						kxReportMarkupError(theParser, "missing T");
					kxGetCharacter(theParser);
					if (theParser->c != 'Y')
						kxReportMarkupError(theParser, "missing Y");
					kxGetCharacter(theParser);
					if (theParser->c != 'P')
						kxReportMarkupError(theParser, "missing P");
					kxGetCharacter(theParser);
					if (theParser->c != 'E')
						kxReportMarkupError(theParser, "missing E");
					kxGetCharacter(theParser);
					kxParseDocType(theParser);
				}
				else
					kxReportMarkupError(theParser, "invalid declaration");
			}
			else
				kxParseStartTag(theParser);
		}
		else {
			theParser->getter = kxGetNextTextCharacter;
			kxParseText(theParser);
		}
	}
#if 0
	//@@ don't detect premature end of file
	if (mxIsReference(mxArgv(0))) {
		char * aName = mxArgv(0)->value.reference->next->next->value.string;
		if (aName)
			kxReportMarkupError(theParser, "EOF instead of </%s>", aName);
	}
#endif
}

void kxParseDocType(txMarkupParser* theParser)
{
#if 0
	//@@ don't parse doctype
	if (theParser->callbacks->processDoctype) {
		txMachine* the = theParser->the;

		theParser->getter = kxGetNextDocTypeCharacter;
		mxArgv(2)->value.integer = theParser->line;
		kxNewInstance(the);
		*(--the->stack) = *mxArgv(0);
		kxQueueID(the, the->parentID, XS_GET_ONLY);

		kxNewInstance(the);
		*(--the->stack) = *mxArgv(0);
		kxQueueID(the, the->parentID, XS_GET_ONLY);
		kxParseValue(theParser);
		kxQueueID(the, the->valueID, XS_GET_ONLY);
		kxPushArgument(the);
		(*(theParser->callbacks->processDoctype))(the);
		kxPopArgument(the);
	}
#endif
	kxSkipDocType(theParser);
}

char *kxParseName(txMarkupParser* theParser)
{
	int aLength;
	char *result = (char *)c_malloc(1024);
	aLength = kxScanName(theParser, result, 1024);
	if (!aLength)
		kxReportMarkupError(theParser, "missing name");
	return (char *)c_realloc(result, aLength + 1);
}

void kxParseProcessingInstruction(txMarkupParser* theParser)
{
	char *name = NULL, *value = NULL;

	name = kxParseName(theParser);

	kxScanSpace(theParser);
	
	theParser->c0 = theParser->c;
	kxGetCharacter(theParser);
	theParser->getter = kxGetNextProcessingInstructionCharacter;

	value = kxParseValue(theParser, NULL, 0);

	if (NULL != theParser->callbacks->processProcessingInstruction)
		(*(theParser->callbacks->processProcessingInstruction))(theParser->clientState, name, value);

	if (0 == FskStrCompare("xml", name)) {
		// try to find the character encoding
		char *encoding = FskStrStr(value, "encoding");
		if (NULL != encoding) {
			encoding += 8;
			encoding = FskStrStripHeadSpace(encoding);
			if ('=' == *encoding++) {
				char quote;
				encoding = FskStrStripHeadSpace(encoding);
				quote = *encoding++;
				if (('\'' == quote) || ('\"' == quote)) {
					char *endQuote = FskStrChr(encoding, quote);
					if (NULL != endQuote) {
						*endQuote = 0;
						if (0 == FskStrCompareCaseInsensitive("UTF-8", encoding))
							theParser->characterEncoding = charEncodingUTF8;
					}
				}
			}
		}
		
	}

	c_free(name);
	c_free(value);

	if (theParser->c != '>')
		kxReportMarkupError(theParser, "missing ?>");
	kxGetCharacter(theParser);
}

void kxParseStartTag(txMarkupParser* theParser)
{
	txIndex anIndex;
	int aLength;
	char *tagName = NULL;
	txAttribute *attributes = &theParser->attributes[0];
	char *stringBuf = theParser->startTagBuffer;
	char *stringBufEnd = theParser->startTagBuffer + mxStartTagBufferSize;
	Boolean stopTag = false;

	attributes->name = NULL;
	attributes->value = NULL;

	aLength = kxScanName(theParser, stringBuf, stringBufEnd - stringBuf);
	if (0 == aLength) {
		kxReportMarkupError(theParser, "out of buffer");
		goto bail;
	}
	tagName = stringBuf; stringBuf += aLength + 1;

	anIndex = 0;
	aLength = kxScanSpace(theParser);
	while ((theParser->c != C_EOF) && (theParser->c != '/') && (theParser->c != '>')) {
		char *attributeName, *attributeValue;
		UInt32 attributeValueLen;

		if (!aLength) {
			// kxReportMarkupError(theParser, "missing space");
			goto skipAhead;
		}

		aLength = kxScanName(theParser, stringBuf, stringBufEnd - stringBuf);
		if (0 == aLength) {
			kxReportMarkupError(theParser, "out of buffer");
			goto bail;
		}
		attributeName = stringBuf; stringBuf += aLength + 1;

		kxScanSpace(theParser);
		if (theParser->c != '=') {
//			kxReportMarkupError(theParser, "missing =");
			goto skipAhead;		// handles case where attribute name appears without value. this apparently really happens in html. live and learn.
		}
		kxGetCharacter(theParser);
		kxScanSpace(theParser);
		if ((theParser->c != '\'') && (theParser->c != '"')) {
//			kxReportMarkupError(theParser, "missing left quote");
			int prev; 
skipAhead:
			prev = theParser->c;
			while ((theParser->c != '>') && !theParser->failed) {
				prev = theParser->c;
				kxGetCharacter(theParser);
			}
			stopTag = '/' == prev;
			goto notXML;
		}
		theParser->c0 = theParser->c;
		kxGetCharacter(theParser);
		theParser->getter = kxGetNextValueCharacter;

		attributeValue = kxParseValue(theParser, stringBuf, stringBufEnd - stringBuf);
		if (NULL == attributeValue) {
			kxReportMarkupError(theParser, "out of buffer");
			goto bail;
		}
		attributeValueLen = FskStrLen(attributeValue);
		attributeValue = stringBuf; stringBuf += attributeValueLen + 1;

		if (theParser->c != theParser->c0)
			kxReportMarkupError(theParser, "missing right quote");

		if (anIndex >= (mxAttributeCount - 1)) {
			kxReportMarkupError(theParser, "too many attributes");
			goto bail;
		}

		attributes[anIndex].name = attributeName;
		attributes[anIndex].value = attributeValue;
		attributes[anIndex].valueLen = attributeValueLen;
		attributes[anIndex + 1].name = NULL;
		attributes[anIndex + 1].value = NULL;

		kxGetCharacter(theParser);

		anIndex++;
		aLength = kxScanSpace(theParser);
	}

	if (theParser->callbacks->processStartTag) {
		(*(theParser->callbacks->processStartTag))(theParser->clientState, tagName, attributes);
		if (theParser->callbacks->processGetCharacterEncoding) {
			long result = (theParser->callbacks->processGetCharacterEncoding)(theParser->clientState);
			if (result != charEncodingUnknown)
				theParser->characterEncoding = result;
		}
	}

	if (theParser->c == '/') {
		kxGetCharacter(theParser);
		stopTag = true;
	}

notXML:
	if (stopTag) {
		if (theParser->callbacks->processStopTag)
			(*(theParser->callbacks->processStopTag))(theParser->clientState, tagName);
	}
	else {
		theParser->depth++;
	}
	
bail:
	if (theParser->c == '>')
		kxGetCharacter(theParser);
	else
		kxReportMarkupError(theParser, "missing >");
}

void kxParseStopTag(txMarkupParser* theParser)
{	
	if (!kxScanName(theParser, theParser->name, sizeof(theParser->name)))
		kxReportMarkupError(theParser, "missing element name");
#if 0
	//@@ no validation on end tags
	if (!mxIsReference(mxArgv(0)))
		kxReportMarkupError(theParser, "invalid </%s>", theParser->name);
	aName = mxArgv(0)->value.reference->next->next->value.string;
	if (c_strcmp(theParser->name, aName) != 0)
		kxReportMarkupError(theParser, "</%s> instead of </%s>", theParser->name, aName);
#endif

	if (theParser->callbacks->processStopTag)
		(*(theParser->callbacks->processStopTag))(theParser->clientState, theParser->name);
	kxScanSpace(theParser);
	theParser->depth--;
	if (theParser->c == '>')
		kxGetCharacter(theParser);
	else
		kxReportMarkupError(theParser, "missing >");
}

void kxParseText(txMarkupParser* theParser)
{
#if 0
	//@@ check to make sure we are in a valid state
	if (!mxIsReference(mxArgv(0)))
		kxReportMarkupError(theParser, "invalid data");
#endif
	char *text = kxParseValue(theParser, theParser->startTagBuffer, mxStartTagBufferSize);
	if (text && theParser->callbacks->processText)
		(*(theParser->callbacks->processText))(theParser->clientState, text);
}

char *kxParseValue(txMarkupParser* theParser, char *buffer, UInt32 bufferSize)
{
	UInt32 i;
	UInt32 j;
	txU1* p;
	txU4 c;
//	txUTF8Sequence* aSequence;
//	int aShift;
	char *result = NULL;
	
	i = 0;
	if (NULL == buffer) {
		j = 1024;
		result = (char *)c_malloc(j);
	}
	else {
		j = bufferSize;
		result = buffer;
	}
	p = (txU1*)result;
	c = (txU4)(*(theParser->getter))(theParser);
	while (c != (txU4)C_EOF) {
		if (c < 128) {
			i++;
			if (i >= j) {
				if (buffer)
					return NULL;		// overflow
				j += 1024;
				result = (char *)c_realloc(result, j);
				p = (unsigned char *)result + i - 1;
			}
			*p++ = (txU1)c;
		}
		else {
			xUTF8Sequence *aSequence;
			txS4 aShift;

			for (aSequence = xUTF8Sequences; aSequence->size; aSequence++)
				if (c <= aSequence->lmask)
					break;
			if (aSequence->size == 0)
				kxReportMarkupError(theParser, "invalid value");
			else {
				i += aSequence->size;
				if (i >= j) {
					if (buffer)
						return NULL;		// overflow
					j += 1024;
					result = (char *)c_realloc(result, j);
					p = (txU1*)(result + i - aSequence->size);
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
	return (NULL == buffer) ? (char *)c_realloc(result, i) : buffer;
}

#if 0
//@@ not using xs!
void kxPopArgument(txMachine* the)
{
	mxArgv(0)->kind = mxArgv(0)->value.reference->next->kind;
	mxArgv(0)->value.reference = mxArgv(0)->value.reference->next->value.reference;
}

void kxPushArgument(txMachine* the)
{
	mxArgv(0)->value.reference = the->stack->value.reference;
	mxArgv(0)->kind = the->stack->kind;
	the->stack++;
}

#endif

void kxReportMarkupError(txMarkupParser* theParser, char* theFormat, ...)
{
	theParser->failed = true;
}

int kxScanName(txMarkupParser* theParser, char* theName, UInt32 theSize)
{
	UInt32 i;
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
			kxGetCharacter(theParser);
			c = theParser->c;
			if ((('0' <= c) && (c <= '9')) || (('A' <= c) && (c <= 'Z')) || (('a' <= c) && (c <= 'z')) 
					|| (c == '.') || (c == '-') || (c == '_') || (c == ':')) {
				i++;
				if (i < theSize)
					*p++ = c;
				else if (i == theSize)
					kxReportMarkupError(theParser, "name overflow");
			}
			else
				break;
		}
	}
	*p = 0;
	return i;	
}

int kxScanSpace(txMarkupParser* theParser)
{
	int i = 0;
	
	for (;;) {
		switch (theParser->c) {
		case 10:
			i++;	
			theParser->line++;
			kxGetCharacter(theParser);
			break;
		case 13:	
			i++;	
			theParser->line++;
			kxGetCharacter(theParser);
			if (theParser->c == 10)
				kxGetCharacter(theParser);
			break;
		case '\t':
		case ' ':
			i++;	
			kxGetCharacter(theParser);
			break;
		default:
			return i;
		}
	}
}

void kxSkipComment(txMarkupParser* theParser)
{
	int c;
	theParser->c0 = theParser->c;
	kxGetCharacter(theParser);
	theParser->c1 = theParser->c;
	kxGetCharacter(theParser);
	do {
		if ((theParser->c0 == '-') && (theParser->c1 == '-') && (theParser->c == '>'))
			c = C_EOF;
		else {
			c = theParser->c0;
			theParser->c0 = theParser->c1;
			theParser->c1 = theParser->c;
			kxGetCharacter(theParser);
			if ((c == 10) || ((c == 13) && (theParser->c0 != 10)))
				theParser->line++;
		}
	} while (c != C_EOF);
	if (theParser->c == '>')
		kxGetCharacter(theParser);
	else
		kxReportMarkupError(theParser, "missing -->");
}

void kxSkipDeclaration(txMarkupParser* theParser)
{
	int c;
	do {
		if (theParser->c == '>')
			c = C_EOF;
		else {
			c = theParser->c;
			kxGetCharacter(theParser);
			if ((c == 10) || ((c == 13) && (theParser->c != 10)))
				theParser->line++;
		}
	} while (c != C_EOF);
	if (theParser->c == '>')
		kxGetCharacter(theParser);
	else
		kxReportMarkupError(theParser, "missing >");
}

void kxSkipDocType(txMarkupParser* theParser)
{
	int c;
	do {
		if ((theParser->c == '[') || (theParser->c == '>'))
			c = C_EOF;
		else {
			c = theParser->c;
			kxGetCharacter(theParser);
			if ((c == 10) || ((c == 13) && (theParser->c != 10)))
				theParser->line++;
		}
	} while (c != C_EOF);
	if (theParser->c == '[') {
		kxGetCharacter(theParser);
		for (;;) {
			kxScanSpace(theParser);
			if (theParser->c == C_EOF)
				break;
			else if (theParser->c == ']')
				break;
			else if (theParser->c == '<') {
				kxGetCharacter(theParser);
				if (theParser->c == '?') {
					kxGetCharacter(theParser);
					kxSkipProcessingInstruction(theParser);
				}
				else if (theParser->c == '!') {
					kxGetCharacter(theParser);
					if (theParser->c == '-') {
						kxGetCharacter(theParser);
						if (theParser->c != '-')
							kxReportMarkupError(theParser, "missing -");
						kxGetCharacter(theParser);
						kxSkipComment(theParser);
					}
					else
						kxSkipDeclaration(theParser);
				}
			}
		}
		if (theParser->c == ']')
			kxGetCharacter(theParser);
		else
			kxReportMarkupError(theParser, "missing ]");
		kxScanSpace(theParser);
	}
	if (theParser->c == '>')
		kxGetCharacter(theParser);
	else
		kxReportMarkupError(theParser, "missing >");
}
				
void kxSkipProcessingInstruction(txMarkupParser* theParser)
{
	int c;
	theParser->c0 = theParser->c;
	kxGetCharacter(theParser);
	do {
		if ((theParser->c0 == '?') && (theParser->c == '>'))
			c = C_EOF;
		else {
			c = theParser->c0;
			theParser->c0 = theParser->c;
			kxGetCharacter(theParser);
			if ((c == 10) || ((c == 13) && (theParser->c0 != 10)))
				theParser->line++;
		}
	} while (c != C_EOF);
	if (theParser->c == '>')
		kxGetCharacter(theParser);
	else
		kxReportMarkupError(theParser, "missing ?>");
}
