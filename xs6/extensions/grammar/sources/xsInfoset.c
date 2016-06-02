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
#include "xs6Platform.h"
#undef mxImport
#define mxImport extern
#include "expat.h"
#include "xs.h"
#ifdef KPR_CONFIG
#include "FskManifest.xs.h"
#endif

typedef struct {
	char name[9];
	char nameLength;
	char value[5];
	char valueLength;
} Entity;

typedef struct {
	xsMachine* the;
	XML_Parser expat;
	int result;
	int root;
	char* textBuffer;
	int textOffset;
	int textSize;
} Scanner;

typedef struct {
	int offset;
	int size;
} Printer;

enum {
	ATTRIBUTE,
	ATTRIBUTES,
	ATTRIBUTE_PROTOTYPE,
	CDATA_PROTOTYPE,
	CHILD,
	CHILDREN,
	COMMENT_PROTOTYPE,
	DOCUMENT_PROTOTYPE,
	ELEMENT_PROTOTYPE,
	INSTANCE,
	IO,
	LINE,
	NAME,
	NAMESPACE,
	NO_NAMESPACE,
	NO_PREFIX,
	PARENT,
	PATH,
	PI_PROTOTYPE,
	PREFIX,
	PROPERTY,
	RULE,
	RULE_SET,
	VALUE,
	XML_NAMESPACE,
	XML_PREFIX,
	COUNT
};

static int compareEntities(const void *name, const void *entity);
static void scanCharacter(void *data, const char *text, int size);
static void scanComment(void *data, const char *text);
static void scanEntity(void *data, const XML_Char *entityName, int is_parameter_entity);
static void scanName(xsMachine* the, const char *name, int scanDefault);
static void scanNamespace(xsMachine* the, const char *prefix);
static void scanProcessingInstruction(void *data, const char *target, const char *text);
static void scanStartCdata(void *data);
static void scanStartTag(void *data, const char *tag, const char **attributes);
static void scanStopCdata(void *data);
static void scanStopTag(void *data, const char *name);
static void scanText(void *data);
static int scanUnknownEncoding(void *data, const XML_Char *name, XML_Encoding *info);
static void parseAttribute(xsMachine* the);
static void parseAttributes(xsMachine* the);
static void parseCData(xsMachine* the);
static void parseChild(xsMachine* the, xsIndex id, xsStringValue kind);
static void parseChildren(xsMachine* the);
static void parseDefaults(xsMachine* the);
static void parseElement(xsMachine* the);
static void parsePI(xsMachine* the);
static void parseRule(xsMachine* the);
static void parseValue(xsMachine* the);
static void printAttributes(xsMachine* the, Printer* stream);
static void printCData(xsMachine* the, Printer* stream, xsIntegerValue level);
static void printCharacters(xsMachine* the, Printer* stream, xsStringValue characters);
static void printChildren(xsMachine* the, Printer* stream, xsIntegerValue level);
static void printComment(xsMachine* the, Printer* stream, xsIntegerValue level);
static void printElement(xsMachine* the, Printer* stream, xsIntegerValue level);
static void printName(xsMachine* the, Printer* stream);
static void printPI(xsMachine* the, Printer* stream, xsIntegerValue level);
static void printValue(xsMachine* the, Printer* stream, xsIntegerValue theFlag);

#define mxEntityCount 253
static Entity gxEntities[mxEntityCount] = {
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

static int gWindows1252[] = {
	0x20AC, 0x0000, 0x201A, 0x0192, 0x201E, 0x2026, 0x2020, 0x2021, 0x02C6, 0x2030, 0x0160, 0x2039, 0x0152, 0x0000, 0x017D, 0x0000,
	0x0000, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014, 0x02DC, 0x2122, 0x0161, 0x203A, 0x0153, 0x0000, 0x017E, 0x0178,
	0x00A0, 0x00A1, 0x00A2, 0x00A3, 0x00A4, 0x00A5, 0x00A6, 0x00A7, 0x00A8, 0x00A9, 0x00AA, 0x00AB, 0x00AC, 0x00AD, 0x00AE, 0x00AF,
	0x00B0, 0x00B1, 0x00B2, 0x00B3, 0x00B4, 0x00B5, 0x00B6, 0x00B7, 0x00B8, 0x00B9, 0x00BA, 0x00BB, 0x00BC, 0x00BD, 0x00BE, 0x00BF,
	0x00C0, 0x00C1, 0x00C2, 0x00C3, 0x00C4, 0x00C5, 0x00C6, 0x00C7, 0x00C8, 0x00C9, 0x00CA, 0x00CB, 0x00CC, 0x00CD, 0x00CE, 0x00CF,
	0x00D0, 0x00D1, 0x00D2, 0x00D3, 0x00D4, 0x00D5, 0x00D6, 0x00D7, 0x00D8, 0x00D9, 0x00DA, 0x00DB, 0x00DC, 0x00DD, 0x00DE, 0x00DF,
	0x00E0, 0x00E1, 0x00E2, 0x00E3, 0x00E4, 0x00E5, 0x00E6, 0x00E7, 0x00E8, 0x00E9, 0x00EA, 0x00EB, 0x00EC, 0x00ED, 0x00EE, 0x00EF,
	0x00F0, 0x00F1, 0x00F2, 0x00F3, 0x00F4, 0x00F5, 0x00F6, 0x00F7, 0x00F8, 0x00F9, 0x00FA, 0x00FB, 0x00FC, 0x00FD, 0x00FE, 0x00FF
};

static int gISO8895_15[] = {
	0x00A0, 0x00A1, 0x00A2, 0x00A3, 0x20AC, 0x00A5, 0x0160, 0x00A7, 0x0161, 0x00A9, 0x00AA, 0x00AB, 0x00AC, 0x00AD, 0x00AE, 0x00AF,
	0x00B0, 0x00B1, 0x00B2, 0x00B3, 0x017D, 0x00B5, 0x00B6, 0x00B7, 0x017E, 0x00B9, 0x00BA, 0x00BB, 0x0152, 0x0153, 0x0178, 0x00BF,
	0x00C0, 0x00C1, 0x00C2, 0x00C3, 0x00C4, 0x00C5, 0x00C6, 0x00C7, 0x00C8, 0x00C9, 0x00CA, 0x00CB, 0x00CC, 0x00CD, 0x00CE, 0x00CF,
	0x00D0, 0x00D1, 0x00D2, 0x00D3, 0x00D4, 0x00D5, 0x00D6, 0x00D7, 0x00D8, 0x00D9, 0x00DA, 0x00DB, 0x00DC, 0x00DD, 0x00DE, 0x00DF,
	0x00E0, 0x00E1, 0x00E2, 0x00E3, 0x00E4, 0x00E5, 0x00E6, 0x00E7, 0x00E8, 0x00E9, 0x00EA, 0x00EB, 0x00EC, 0x00ED, 0x00EE, 0x00EF,
	0x00F0, 0x00F1, 0x00F2, 0x00F3, 0x00F4, 0x00F5, 0x00F6, 0x00F7, 0x00F8, 0x00F9, 0x00FA, 0x00FB, 0x00FC, 0x00FD, 0x00FE, 0x00FF
};

typedef struct {
	void* stream;
	xsGetter getter;
} xsStreamGetter;

void fxParse(xsMachine* the, void* theStream, xsGetter theGetter, xsStringValue thePath, xsIntegerValue theLine, xsFlag theFlag)
{
	xsStreamGetter streamGetter;
	xsSlot* stack = the->stack;
	xsIntegerValue c = xsToInteger(stack[0]);
	stack[c] = xsGet(xsGet(xsGlobal, xsID("xs")), xsID("infoset"));
	stack[c - 1] = xsNewHostObject(NULL);
	streamGetter.stream = theStream;
	streamGetter.getter = theGetter;
	xsSetHostData(stack[c - 1], &streamGetter);
	if (thePath && theLine)
		stack[c - 2] = xsCall3(stack[c], xsID("scan"), stack[c - 1], xsString(thePath), xsInteger(theLine));
	else if (thePath)
		stack[c - 2] = xsCall2(stack[c], xsID("scan"), stack[c - 1], xsString(thePath));
	else
		stack[c - 2] = xsCall1(stack[c], xsID("scan"), stack[c - 1]);
	stack[c] = xsCall2(stack[c], xsID("parse"), stack[c - 2], xsInteger(theFlag));
	the->stack = stack + c;
}

void fxSerialize(xsMachine* the, void* theStream, xsPutter thePutter)
{
	xsUnknownError("TBD");
}

void xs_infoset_compareAttributes(xsMachine* the)
{
}

void xs_infoset_scan(xsMachine* the)
{
	int c = xsToInteger(xsArgc);
	Scanner scanner;
	Scanner* self;
	xsVars(COUNT);
	xsTry {
		self = &scanner;
		c_memset(self, 0, sizeof(Scanner));
		if (c < 1)
			xsSyntaxError("no buffer");
		
		self->expat = XML_ParserCreate(NULL);
		xsThrowIfNULL(self->expat);
		XML_SetUserData(self->expat, self);
		XML_SetElementHandler(self->expat, scanStartTag, scanStopTag);
		XML_SetCdataSectionHandler(self->expat, scanStartCdata, scanStopCdata);
		XML_SetCharacterDataHandler(self->expat, scanCharacter);
		XML_SetCommentHandler(self->expat, scanComment);
		XML_SetProcessingInstructionHandler(self->expat, scanProcessingInstruction);
		XML_SetUnknownEncodingHandler(self->expat, scanUnknownEncoding, NULL);
		XML_SetSkippedEntityHandler(self->expat, scanEntity);
		
		self->result = 1;
		self->textBuffer = c_malloc(8192);
		xsThrowIfNULL(self->textBuffer);
		self->textSize = 8192;
		
		self->the = the;
		xsVar(ATTRIBUTE_PROTOTYPE) = xsGet(xsThis, xsID_attribute);
		xsVar(CDATA_PROTOTYPE) = xsGet(xsThis, xsID_cdata);
		xsVar(COMMENT_PROTOTYPE) = xsGet(xsThis, xsID_comment);
		xsVar(DOCUMENT_PROTOTYPE) = xsGet(xsThis, xsID_document);
		xsVar(ELEMENT_PROTOTYPE) = xsGet(xsThis, xsID_element);
		xsVar(NO_NAMESPACE) = xsString("");
		xsVar(NO_PREFIX) = xsString("");
		xsVar(PATH) = (c > 1) ? xsArg(1) : xsUndefined;
		xsVar(PI_PROTOTYPE) = xsGet(xsThis, xsID_pi);
		xsVar(XML_NAMESPACE) = xsGet(xsThis, xsID_xmlnsNamespace);
		xsVar(XML_PREFIX) = xsGet(xsThis, xsID_xmlnsPrefix);
		
		xsResult = xsNewInstanceOf(xsVar(DOCUMENT_PROTOTYPE));
		xsSet(xsResult, xsID_encoding, xsString("UTF-8"));
		xsSet(xsResult, xsID_version, xsString("1.0"));
		xsVar(CHILDREN) = xsNewInstanceOf(xsArrayPrototype);
		xsArrayCacheBegin(xsVar(CHILDREN));
		xsSet(xsResult, xsID_children, xsVar(CHILDREN));
		xsSet(xsResult, xsID_parent, xsNull);
		xsSet(xsResult, xsID_xmlnsAttributes, xsNull);

		if (xsIsInstanceOf(xsArg(0), xsChunkPrototype)) {
			xsStringValue buffer = xsGetHostData(xsArg(0));
			xsIntegerValue size = xsToInteger(xsGet(xsArg(0), xsID_length));
			self->result = XML_Parse(self->expat, (const char *)buffer, size, 1);
		}
		else if (xsTypeOf(xsArg(0)) == xsStringType) {
			xsStringValue string = xsToString(xsArg(0));
			xsIntegerValue stringOffset = 0;
			xsIntegerValue stringSize = c_strlen(string);
			while (self->result && (stringOffset < stringSize)) {
				xsIntegerValue size = stringSize - stringOffset;
				xsStringValue buffer = (char *)XML_GetBuffer(self->expat, 1024);
				xsThrowIfNULL(buffer);
				if (size > 1024) 
					size = 1024;
				c_memcpy(buffer, string + stringOffset, size);
				self->result = XML_ParseBuffer(self->expat, size, (size < 1024) ? 1 : 0);
				stringOffset += size;
				string = xsToString(xsArg(0)); // @@ gc
			}
		}
		else {
			xsStreamGetter* streamGetter = xsGetHostData(xsArg(0));
			while (self->result) {
				xsIntegerValue i;
				xsStringValue p, buffer = (char *)XML_GetBuffer(self->expat, 1024);
				xsThrowIfNULL(buffer);
				for (i = 0, p = buffer; i < 1024; i++, p++) {
					int c = (*(streamGetter->getter))(streamGetter->stream);
					if (c == C_EOF)
						break;
					*p = (char)c;
				}
				self->result = XML_ParseBuffer(self->expat, i, (i < 1024) ? 1 : 0);
				if (i < 1024)
					break;
			}
		}
		
		xsDelete(xsResult, xsID_xmlnsAttributes);
		xsDelete(xsResult, xsID_parent);
		xsArrayCacheEnd(xsVar(CHILDREN));
		
		if (!self->result) {
			xsVar(LINE) = xsInteger(XML_GetCurrentLineNumber(self->expat));
			xsVar(VALUE) = xsString((char*)XML_ErrorString(XML_GetErrorCode(self->expat)));
			if (xsHas(xsThis, xsID_reportError))
				xsCall3_noResult(xsThis, xsID_reportError, xsVar(PATH), xsVar(LINE), xsVar(VALUE));
			xsThrow(xsNewInstanceOf(xsSyntaxErrorPrototype));
		}
		c_free(self->textBuffer);
		self->textBuffer = NULL;
		XML_ParserFree(self->expat);
		self->expat = NULL;
	}
	xsCatch {
		if (self->textBuffer)
			c_free(self->textBuffer);
		if (self->expat)
			XML_ParserFree(self->expat);
	}
}

int compareEntities(const void *name, const void *entity)
{
	return c_strcmp((char*)name, ((Entity*)entity)->name);
}

void scanCharacter(void *data, const char *text, int size)
{
	Scanner* self = data;
	xsMachine* the = self->the;
	int textSize = self->textOffset + size + 1;
	if (textSize > self->textSize) {
		textSize = (textSize + 8191) & ~0x01fff;
		self->textBuffer = c_realloc(self->textBuffer, textSize);
		self->textSize = textSize;
	}
	c_memcpy(self->textBuffer + self->textOffset, text, size);
	if (!self->textOffset)
		xsVar(LINE) = xsInteger(XML_GetCurrentLineNumber(self->expat));
	self->textOffset += size;
}

void scanComment(void *data, const char *text)
{
	Scanner* self = data;
	xsMachine* the = self->the;
	scanText(data);
	xsVar(VALUE) = xsString((xsStringValue)text);
	xsVar(LINE) = xsInteger(XML_GetCurrentLineNumber(self->expat));
	xsVar(CHILD) = xsNewInstanceOf(xsVar(COMMENT_PROTOTYPE));
	xsSet(xsVar(CHILD), xsID_path, xsVar(PATH));
	xsSet(xsVar(CHILD), xsID_line, xsVar(LINE));
	xsSet(xsVar(CHILD), xsID_parent, xsResult);
	xsSet(xsVar(CHILD), xsID_value, xsVar(VALUE));
	xsArrayCacheItem(xsVar(CHILDREN), xsVar(CHILD));
}

void scanEntity(void *data, const XML_Char *entityName, int is_parameter_entity)
{
	Scanner* self = data;
	xsMachine* the = self->the;
	Entity* entity = (Entity*)bsearch(entityName, gxEntities, mxEntityCount, sizeof(Entity), compareEntities);
	if (entity)
		scanCharacter(data, entity->value, entity->valueLength);
	else {
		xsVar(LINE) = xsInteger(XML_GetCurrentLineNumber(self->expat));
		xsVar(VALUE) = xsString((xsStringValue)entityName);
		xsCall3_noResult(xsThis, xsID_reportError, xsVar(PATH), xsVar(LINE), xsVar(VALUE));
		scanCharacter(data, entityName, c_strlen(entityName));
	}
}

void scanName(xsMachine* the, const char *name, int scanDefault)
{
	char *colon = c_strchr(name, ':');
	if (colon) {
		*colon = 0;
		scanNamespace(the, name);
		xsVar(NAME) = xsString(colon + 1);
		xsVar(PREFIX) = xsString((xsStringValue)name);
		*colon = ':';
	}
	else if (scanDefault) {
		scanNamespace(the, "xmlns");
		xsVar(NAME) = xsString((xsStringValue)name);
		xsVar(PREFIX) = xsNull;
	}
	else {
		xsVar(NAME) = xsString((xsStringValue)name);
		xsVar(PREFIX) = xsNull;
		xsVar(NAMESPACE) = xsNull;
	}
}

void scanNamespace(xsMachine* the, const char *prefix)
{
	xsIntegerValue i, c;
	char* name;
	xsVar(PARENT) = xsResult;
	while (xsTest(xsVar(PARENT))) {
		xsVar(ATTRIBUTES) = xsGet(xsVar(PARENT), xsID_xmlnsAttributes);
		if (xsTest(xsVar(ATTRIBUTES))) {
			c = xsToInteger(xsGet(xsVar(ATTRIBUTES), xsID_length));
			for (i = 0; i < c; i++) {
				xsVar(ATTRIBUTE) = xsGet(xsVar(ATTRIBUTES), i);
				name = xsToString(xsGet(xsVar(ATTRIBUTE), xsID_name));
				if (c_strcmp(name, prefix) == 0) {
					xsVar(NAMESPACE) = xsGet(xsVar(ATTRIBUTE), xsID_value);
					return;
				}
			}
		}
		xsVar(PARENT) = xsGet(xsVar(PARENT), xsID_parent);
	}
	xsVar(NAMESPACE) = xsNull;
}

void scanProcessingInstruction(void *data, const char *target, const char *text)
{
	Scanner* self = data;
	xsMachine* the = self->the;
	scanText(data);
	scanName(the, target, 1);
	xsVar(VALUE) = xsString((xsStringValue)text);
	xsVar(LINE) = xsInteger(XML_GetCurrentLineNumber(self->expat));
	xsVar(CHILD) = xsNewInstanceOf(xsVar(PI_PROTOTYPE));
	xsSet(xsVar(CHILD), xsID_path, xsVar(PATH));
	xsSet(xsVar(CHILD), xsID_line, xsVar(LINE));
	xsSet(xsVar(CHILD), xsID_parent, xsResult);
	xsSet(xsVar(CHILD), xsID_name, xsVar(NAME));
	xsSet(xsVar(CHILD), xsID_namespace, xsVar(NAMESPACE));
	xsSet(xsVar(CHILD), xsID_prefix, xsVar(PREFIX));
	xsSet(xsVar(CHILD), xsID_value, xsVar(VALUE));
	xsArrayCacheItem(xsVar(CHILDREN), xsVar(CHILD));
}

void scanStartCdata(void *data)
{
}

void scanStartTag(void *data, const char *tag, const char **attributes)
{
	Scanner* self = data;
	xsMachine* the = self->the;
	const char **attribute;
	char* name;
	char* value;
	char* colon;
	scanText(data);
	xsVar(LINE) = xsInteger(XML_GetCurrentLineNumber(self->expat));
	xsVar(CHILD) = xsNewInstanceOf(xsVar(ELEMENT_PROTOTYPE));
	xsSet(xsVar(CHILD), xsID_path, xsVar(PATH));
	xsSet(xsVar(CHILD), xsID_line, xsVar(LINE));
	xsSet(xsVar(CHILD), xsID_parent, xsResult);
	if (!self->root) {
		self->root = 1;
		xsSet(xsResult, xsID_element, xsVar(CHILD));
	}
	xsArrayCacheItem(xsVar(CHILDREN), xsVar(CHILD));
	xsResult = xsVar(CHILD);
	
	xsVar(CHILDREN) = xsNewInstanceOf(xsArrayPrototype);
	xsArrayCacheBegin(xsVar(CHILDREN));
	attribute = attributes;
	while (*attribute) {
		name = (char*)*attribute;
		attribute++;
		value = (char*)*attribute;
		attribute++;
		if (c_strncmp(name, "xmlns", 5) == 0) {
			colon = name + 5;
			if (*colon == ':') {
				*colon = 0;
				xsVar(NAME) = xsString(colon + 1);
				*colon = ':';
				xsVar(PREFIX) = xsVar(XML_PREFIX);
			}
			else {
				xsVar(NAME) = xsVar(XML_PREFIX);
				xsVar(PREFIX) = xsUndefined;
			}
			xsVar(NAMESPACE) = xsVar(XML_NAMESPACE);
			xsVar(VALUE) = xsString(value);
			xsVar(CHILD) = xsNewInstanceOf(xsVar(ATTRIBUTE_PROTOTYPE));
			xsSet(xsVar(CHILD), xsID_parent, xsResult);
			xsSet(xsVar(CHILD), xsID_path, xsVar(PATH));
			xsSet(xsVar(CHILD), xsID_line, xsVar(LINE));
			xsSet(xsVar(CHILD), xsID_name, xsVar(NAME));
			xsSet(xsVar(CHILD), xsID_namespace, xsVar(NAMESPACE));
			xsSet(xsVar(CHILD), xsID_prefix, xsVar(PREFIX));
			xsSet(xsVar(CHILD), xsID_value, xsVar(VALUE));
			xsArrayCacheItem(xsVar(CHILDREN), xsVar(CHILD));
		}
	}
	xsArrayCacheEnd(xsVar(CHILDREN));
	xsSet(xsResult, xsID_xmlnsAttributes, xsVar(CHILDREN));
	
	xsVar(CHILDREN) = xsNewInstanceOf(xsArrayPrototype);
	xsArrayCacheBegin(xsVar(CHILDREN));
	attribute = attributes;
	while (*attribute) {
		name = (char*)*attribute;
		attribute++;
		value = (char*)*attribute;
		attribute++;
		if (c_strncmp(name, "xmlns", 5) != 0) {
			scanName(the, name, 0);
			xsVar(VALUE) = xsString(value);
			xsVar(CHILD) = xsNewInstanceOf(xsVar(ATTRIBUTE_PROTOTYPE));
			xsSet(xsVar(CHILD), xsID_parent, xsResult);
			xsSet(xsVar(CHILD), xsID_path, xsVar(PATH));
			xsSet(xsVar(CHILD), xsID_line, xsVar(LINE));
			xsSet(xsVar(CHILD), xsID_name, xsVar(NAME));
			xsSet(xsVar(CHILD), xsID_namespace, xsVar(NAMESPACE));
			xsSet(xsVar(CHILD), xsID_prefix, xsVar(PREFIX));
			xsSet(xsVar(CHILD), xsID_value, xsVar(VALUE));
			xsArrayCacheItem(xsVar(CHILDREN), xsVar(CHILD));
		}
	}
	xsArrayCacheEnd(xsVar(CHILDREN));
	xsSet(xsResult, xsID__attributes, xsVar(CHILDREN));

	scanName(the, tag, 1);
	xsSet(xsResult, xsID_name, xsVar(NAME));
	xsSet(xsResult, xsID_namespace, xsVar(NAMESPACE));
	xsSet(xsResult, xsID_prefix, xsVar(PREFIX));
	xsVar(CHILDREN) = xsNewInstanceOf(xsArrayPrototype);
	xsArrayCacheBegin(xsVar(CHILDREN));
	xsSet(xsResult, xsID_children, xsVar(CHILDREN));
}

void scanStopCdata(void *data)
{
}

void scanStopTag(void *data, const char *name)
{
	Scanner* self = data;
	xsMachine* the = self->the;
	scanText(data);
	xsArrayCacheEnd(xsVar(CHILDREN));
	xsResult = xsGet(xsResult, xsID_parent);
	xsVar(CHILDREN) = xsGet(xsResult, xsID_children);
}

void scanText(void *data)
{
	Scanner* self = data;
	xsMachine* the = self->the;
	if (self->textOffset) {
		self->textBuffer[self->textOffset] = 0;
		xsVar(VALUE) = xsString(self->textBuffer);
		self->textOffset = 0;
		xsVar(CHILD) = xsNewInstanceOf(xsVar(CDATA_PROTOTYPE));
		xsSet(xsVar(CHILD), xsID_path, xsVar(PATH));
		xsSet(xsVar(CHILD), xsID_line, xsVar(LINE));
		xsSet(xsVar(CHILD), xsID_parent, xsResult);
		xsSet(xsVar(CHILD), xsID_value, xsVar(VALUE));
		xsArrayCacheItem(xsVar(CHILDREN), xsVar(CHILD));
	}
}

int scanUnknownEncoding(void *data, const XML_Char *name, XML_Encoding *info)
{
	int i;
	if (0 == c_strcmp(name, "windows-1252")) {
		for (i = 0; i < 128; i++)
			info->map[i] = i;
		for (i = 128; i < 255; i++)
			info->map[i] = gWindows1252[i - 128];
		return XML_STATUS_OK;
	}
	if (0 == c_strcmp(name, "iso-8859-15")) {
		for (i = 0; i < 0x0a0; i++)
			info->map[i] = i;
		for (i = 0x0a0; i < 255; i++)
			info->map[i] = gISO8895_15[i - 0x0a0];
		return XML_STATUS_OK;
	}
	return XML_STATUS_ERROR;
}

void xs_infoset_parse(xsMachine* the)
{
	xsVars(COUNT);
	xsVar(ATTRIBUTE_PROTOTYPE) = xsGet(xsThis, xsID_attribute);
	xsVar(CDATA_PROTOTYPE) = xsGet(xsThis, xsID_cdata);
	xsVar(COMMENT_PROTOTYPE) = xsGet(xsThis, xsID_comment);
	xsVar(DOCUMENT_PROTOTYPE) = xsGet(xsThis, xsID_document);
	xsVar(ELEMENT_PROTOTYPE) = xsGet(xsThis, xsID_element);
	if ((xsToInteger(xsArgc) < 1) || (!xsIsInstanceOf(xsArg(0), xsVar(DOCUMENT_PROTOTYPE))))
		xsTypeError("document is no infoset.document");
	xsVar(RULE_SET) = xsGet(xsGet(xsGlobal, xsID_Grammar), xsID_ruleSet);
	xsVar(CHILD) = xsGet(xsArg(0), xsID_element);
	parseElement(the);
}

void parseAttribute(xsMachine* the)
{
	parseChild(the, xsID_attributeRules, "attribute");
	if (xsTest(xsVar(RULE)))
		parseValue(the);
}

void parseAttributes(xsMachine* the)
{
	xsIntegerValue c, i;
	if (xsTest(xsVar(CHILDREN))) {
		c = xsToInteger(xsGet(xsVar(CHILDREN), xsID_length));
		for (i = 0; i < c; i++) {
			xsVar(CHILD) = xsGet(xsVar(CHILDREN), i);
			parseAttribute(the);
		}
	}
}

void parseCData(xsMachine* the)
{
	xsVar(RULE) = xsGet(xsVar(RULE_SET), xsID_dataRule);
	if (xsTest(xsVar(RULE)))
		parseValue(the);
}

void parseChild(xsMachine* the, xsIndex id, xsStringValue kind)
{
	xsStringValue namespace;
	xsIndex namespaceID;
	xsStringValue name;
	xsIndex nameID;
	xsStringValue test;
	xsVar(RULE) = xsGet(xsVar(CHILD), xsID_namespace);
	if (xsTest(xsVar(RULE))) {
		namespace = xsToString(xsVar(RULE));
		namespaceID = xsFindID(namespace);
		if (!namespaceID)
			xsLog("# %s namespace not found: %s\n", kind, namespace);
	}
	else {
		namespace = NULL;
		namespaceID = xsID_null;
	}
	xsVar(RULE) = xsGet(xsVar(CHILD), xsID_name);
	if (xsTest(xsVar(RULE))) {
		name = xsToString(xsVar(RULE));
		nameID = xsFindID(name);
		if (!nameID)
			xsLog("# %s name not found: %s\n", kind, nameID);
	}
	else {
		name = NULL;
		nameID = xsID_null;
		xsLog("# missing %s name\n", kind);
	}
	if (name) {
		test = xsName(nameID);
		xsVar(RULE) = xsGet(xsVar(RULE_SET), id);
		if (xsTest(xsVar(RULE))) {
			xsVar(RULE) = xsGet(xsVar(RULE), namespaceID);
			if (xsTest(xsVar(RULE)))
				xsVar(RULE) = xsGet(xsVar(RULE), nameID);
		}
		if (!xsTest(xsVar(RULE))) {
			if (namespace)
				xsLog("# %s not found: {%s}:%s\n", kind, namespace, name);
			else
				xsLog("# %s not found: %s\n", kind, name);
		}
	}
}

void parseChildren(xsMachine* the)
{
	xsIntegerValue c, i;
	if (xsTest(xsVar(CHILDREN))) {
		c = xsToInteger(xsGet(xsVar(CHILDREN), xsID_length));
		for (i = 0; i < c; i++) {
			xsVar(CHILD) = xsGet(xsVar(CHILDREN), i);
			if (xsIsInstanceOf(xsVar(CHILD), xsVar(ELEMENT_PROTOTYPE)))
				parseElement(the);
			else if (xsIsInstanceOf(xsVar(CHILD), xsVar(CDATA_PROTOTYPE)))
				parseCData(the);
			else if (xsIsInstanceOf(xsVar(CHILD), xsVar(PI_PROTOTYPE)))
				parsePI(the);
		}
	}
}

void parseDefaults(xsMachine* the)
{
	xsIntegerValue c, i;
	if (xsTest(xsVar(CHILDREN))) {
		c = xsToInteger(xsGet(xsVar(CHILDREN), xsID_length));
		for (i = 0; i < c; i++) {
			xsVar(RULE) = xsGet(xsVar(CHILDREN), i);
			xsVar(IO) = xsGet(xsVar(RULE), xsID_io);
			xsVar(VALUE) = xsNewInstanceOf(xsVar(IO));
			parseRule(the);
		}
	}
}

void parseElement(xsMachine* the)
{
	xsIntegerValue c, i;
	xsIndex id;
	parseChild(the, xsID_elementRules, "element");
	if (xsTest(xsVar(RULE))) {
		xsVar(PARENT) = xsVar(CHILD);
		
		xsVar(RULE_SET) = xsGet(xsVar(RULE), xsID_ruleSet);
		xsSet(xsVar(PARENT), xsID_ruleSet, xsVar(RULE_SET));

		c = xsToInteger(xsGet(xsVar(RULE), xsID_length));
		if (c) {
			xsVar(PROPERTY) = xsVar(INSTANCE);
			c--;
			for (i = 0; i < c; i++) {
				id = xsToInteger(xsGet(xsVar(RULE), i));
				xsVar(PROPERTY) = xsGet(xsVar(PROPERTY), id);
			}
			id = xsToInteger(xsGet(xsVar(RULE), i));
			xsVar(IO) = xsGet(xsVar(RULE), xsID_io);
			xsVar(INSTANCE) = xsNewInstanceOf(xsVar(IO));
			xsVar(PATH) = xsGet(xsVar(CHILD), xsID_path);
			xsSet(xsVar(INSTANCE), xsID___xs__path, xsVar(PATH));
			xsVar(LINE) = xsGet(xsVar(CHILD), xsID_line);
			xsSet(xsVar(INSTANCE), xsID___xs__line, xsVar(LINE));
			if (id < 0) {
				if (id == -1)
					id = xsToInteger(xsGet(xsVar(PROPERTY), xsID_length));
				xsSet(xsVar(PROPERTY), id, xsVar(INSTANCE));
			}
			else {
				xsResult = xsVar(INSTANCE);
			}
		}
		xsSet(xsVar(PARENT), xsID_instance, xsVar(INSTANCE));
	
		xsVar(CHILDREN) = xsGet(xsVar(RULE_SET), xsID_defaultRules);
		parseDefaults(the);
		xsVar(CHILDREN) = xsGet(xsVar(PARENT), xsID__attributes);
		parseAttributes(the);
		xsVar(CHILDREN) = xsGet(xsVar(PARENT), xsID_children);
		parseChildren(the);
	
		xsDelete(xsVar(PARENT), xsID_instance);
		xsDelete(xsVar(PARENT), xsID_ruleSet);
		
		xsVar(PARENT) = xsGet(xsVar(PARENT), xsID_parent);
		xsVar(CHILDREN) = xsGet(xsVar(PARENT), xsID_children);
		xsVar(RULE_SET) = xsGet(xsVar(PARENT), xsID_ruleSet);
		xsVar(INSTANCE) = xsGet(xsVar(PARENT), xsID_instance);
	}
}

void parsePI(xsMachine* the)
{
	parseChild(the, xsID_piRules, "pi");
	if (xsTest(xsVar(RULE)))
		parseValue(the);
}

void parseRule(xsMachine* the)
{
	xsIntegerValue c, i;
	xsIndex id;
	c = xsToInteger(xsGet(xsVar(RULE), xsID_length));
	if (c) {
		xsVar(PROPERTY) = xsVar(INSTANCE);
        c--;
		for (i = 0; i < c; i++) {
			id = xsToInteger(xsGet(xsVar(RULE), i));
			xsVar(PROPERTY) = xsGet(xsVar(PROPERTY), id);
		}
		id = xsToInteger(xsGet(xsVar(RULE), i));
		if (id == -1)
			id = xsToInteger(xsGet(xsVar(PROPERTY), xsID_length));
		xsSet(xsVar(PROPERTY), id, xsVar(VALUE));
	}
}

void parseValue(xsMachine* the)
{
	xsVar(VALUE) = xsGet(xsVar(CHILD), xsID_value);
	xsVar(IO) = xsGet(xsVar(RULE), xsID_io);
	xsVar(VALUE) = xsCall1(xsVar(IO), xsID_parse, xsVar(VALUE));
	parseRule(the);
}

void xs_infoset_print(xsMachine* the)
{
	Printer self;
	xsVars(COUNT);
	
	xsVar(DOCUMENT_PROTOTYPE) = xsGet(xsThis, xsID_document);
	xsVar(ELEMENT_PROTOTYPE) = xsGet(xsThis, xsID_element);
	xsVar(ATTRIBUTE_PROTOTYPE) = xsGet(xsThis, xsID_attribute);
	xsVar(CDATA_PROTOTYPE) = xsGet(xsThis, xsID_cdata);
	xsVar(COMMENT_PROTOTYPE) = xsGet(xsThis, xsID_comment);
	xsVar(PI_PROTOTYPE) = xsGet(xsThis, xsID_pi);
	
	if (xsToInteger(xsArgc) < 1)
		xsSyntaxError("xs.infoset.stringify: no document parameter");
	if (!xsIsInstanceOf(xsArg(0), xsVar(DOCUMENT_PROTOTYPE)))	
		xsSyntaxError("xs.infoset.stringify: document is no instance of xs.infoset.document");
	
	self.size = self.offset = 0;
	xsVar(PARENT) = xsArg(0);
	printChildren(the, &self, 0);
	xsResult = xsStringBuffer(NULL, self.offset);
	self.size = self.offset;
	self.offset = 0;
	xsVar(PARENT) = xsArg(0);
	printChildren(the, &self, 0);
}

void printAttributes(xsMachine* the, Printer* printer)
{
	xsIntegerValue c, i;
	if (xsTest(xsVar(ATTRIBUTES))) {
		c = xsToInteger(xsGet(xsVar(ATTRIBUTES), xsID_length));
		for (i = 0; i < c; i++) {
			xsVar(ATTRIBUTE) = xsGet(xsVar(ATTRIBUTES), i);
			xsVar(PREFIX) = xsGet(xsVar(ATTRIBUTE), xsID_prefix);
			xsVar(NAME) = xsGet(xsVar(ATTRIBUTE), xsID_name);
			xsVar(VALUE) = xsGet(xsVar(ATTRIBUTE), xsID_value);
			printCharacters(the, printer, " ");
			printName(the, printer);
			printCharacters(the, printer, "=\"");
			printValue(the, printer, 1);
			printCharacters(the, printer, "\"");
		}
	}
}

void printCData(xsMachine* the, Printer* printer, xsIntegerValue level)
{
	xsVar(VALUE) = xsGet(xsVar(CHILD), xsID_value);
	printValue(the, printer, 2);
}

void printCharacters(xsMachine* the, Printer* printer, xsStringValue characters)
{
	xsIntegerValue length = c_strlen(characters);
	if (printer->offset + length <= printer->size) {
		c_memcpy(xsToString(xsResult) + printer->offset, characters, length);
	}
	printer->offset += length;
}

void printChildren(xsMachine* the, Printer* printer, xsIntegerValue level)
{
	xsIntegerValue c, i;
	xsVar(CHILDREN) = xsGet(xsVar(PARENT), xsID_children);
	if (xsTest(xsVar(CHILDREN))) {
		c = xsToInteger(xsGet(xsVar(CHILDREN), xsID_length));
		for (i = 0; i < c; i++) {
			xsVar(CHILD) = xsGet(xsVar(CHILDREN), i);
			if (xsIsInstanceOf(xsVar(CHILD), xsVar(ELEMENT_PROTOTYPE)))
				printElement(the, printer, level);
			else if (xsIsInstanceOf(xsVar(CHILD), xsVar(CDATA_PROTOTYPE)))
				printCData(the, printer, level);
			else if (xsIsInstanceOf(xsVar(CHILD), xsVar(COMMENT_PROTOTYPE)))
				printComment(the, printer, level);
			else if (xsIsInstanceOf(xsVar(CHILD), xsVar(PI_PROTOTYPE)))
				printPI(the, printer, level);
		}
	}
}

void printComment(xsMachine* the, Printer* printer, xsIntegerValue level)
{
	xsVar(VALUE) = xsGet(xsVar(CHILD), xsID_value);
	printCharacters(the, printer, "<!--");
	printValue(the, printer, 0);
	printCharacters(the, printer, "-->");
}

void printElement(xsMachine* the, Printer* printer, xsIntegerValue level)
{
	xsVar(PARENT) = xsVar(CHILD);
	
	xsVar(PREFIX) = xsGet(xsVar(PARENT), xsID_prefix);
	xsVar(NAME) = xsGet(xsVar(PARENT), xsID_name);
	printCharacters(the, printer, "<");
	printName(the, printer);
	
	xsVar(ATTRIBUTES) = xsGet(xsVar(PARENT), xsID_xmlnsAttributes);
	printAttributes(the, printer);
	xsVar(ATTRIBUTES) = xsGet(xsVar(PARENT), xsID__attributes);
	printAttributes(the, printer);
	
	printCharacters(the, printer, ">");
	
	printChildren(the, printer, level + 1);
	
	xsVar(PREFIX) = xsGet(xsVar(PARENT), xsID_prefix);
	xsVar(NAME) = xsGet(xsVar(PARENT), xsID_name);
	printCharacters(the, printer, "</");
	printName(the, printer);
	printCharacters(the, printer, ">");
	
	xsVar(PARENT) = xsGet(xsVar(PARENT), xsID_parent);
	xsVar(CHILDREN) = xsGet(xsVar(PARENT), xsID_children);
}

void printName(xsMachine* the, Printer* printer)
{
	if (xsTest(xsVar(PREFIX))) {
		printCharacters(the, printer, xsToString(xsVar(PREFIX)));
		printCharacters(the, printer, ":");
	}
	printCharacters(the, printer, xsToString(xsVar(NAME)));
}

void printPI(xsMachine* the, Printer* printer, xsIntegerValue level)
{
	xsVar(PREFIX) = xsGet(xsVar(CHILD), xsID_prefix);
	xsVar(NAME) = xsGet(xsVar(CHILD), xsID_name);
	xsVar(VALUE) = xsGet(xsVar(CHILD), xsID_value);
	printCharacters(the, printer, "<?");
	printName(the, printer);
	if (xsTest(xsVar(VALUE))) {
		printCharacters(the, printer, " ");
		printValue(the, printer, 0);
	}
	printCharacters(the, printer, "?>");
}

void printValue(xsMachine* the, Printer* printer, xsIntegerValue theFlag)
{
	static unsigned char sEscape[256] = {
	/*  0 1 2 3 4 5 6 7 8 9 A B C D E F */
		3,3,3,3,3,3,3,3,3,1,1,3,3,3,3,3,	/* 0x                    */
		3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,	/* 1x                    */
		0,0,1,0,0,0,3,0,0,0,0,0,0,0,0,0,	/* 2x   !"#$%&'()*+,-./  */
		0,0,0,0,0,0,0,0,0,0,0,0,3,0,2,0,	/* 3x  0123456789:;<=>?  */
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 4x  @ABCDEFGHIJKLMNO  */
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 5X  PQRSTUVWXYZ[\]^_  */
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 6x  `abcdefghijklmno  */
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 	/* 7X  pqrstuvwxyz{|}~   */
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 8X                    */
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 9X                    */
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* AX                    */
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* BX                    */
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* CX                    */
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* FX                    */
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* EX                    */
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 	/* FX                    */
	};
	static unsigned char sHexa[] = "0123456789ABCDEF";
	unsigned char* aText;
	unsigned char* aStop;
	unsigned char* aStart;
	unsigned char aBuffer[8];
	unsigned char aChar;

	aText = (unsigned char*)xsToString(xsVar(VALUE));
	aStop = aStart = aText;
	while ((aChar = *aText++)) {
		if (sEscape[aChar] & theFlag) {
			if (aStop > aStart) {
				*aStop = '\0';
				printCharacters(the, printer, (char*)aStart);
				*aStop = aChar;
			}
			switch (aChar) {
			case '"':
				printCharacters(the, printer, "&quot;");
				break;
			case '&':
				printCharacters(the, printer, "&amp;");
				break;
			case '<':
				printCharacters(the, printer, "&lt;");
				break;
			case '>':
				printCharacters(the, printer, "&gt;");
				break;
			default:
				aStart = aBuffer;
				*(aStart++) = '&';
				*(aStart++) = '#';
				*(aStart++) = 'x';
				if (aChar >= 16)
					*(aStart++) = sHexa[aChar / 16];
				*(aStart++) = sHexa[aChar % 16];
				*(aStart++) = ';';
				*aStart = 0;
				printCharacters(the, printer, (char*)aBuffer);
				break;
			}
			aStart = ++aStop;
		}
		else
			aStop++;
	}
	if (aStop > aStart)
		printCharacters(the, printer, (char*)aStart);
}

void xs_infoset_serialize(xsMachine* the)
{
}

void xs_infoset_reportError(xsMachine* the)
{
	xsLog("%s:%ld: %s\n", xsToString(xsArg(0)), xsToInteger(xsArg(1)), xsToString(xsArg(2)));
}


