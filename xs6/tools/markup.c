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
#include "xs6Platform.h"
#include "xs.h"
#include "tools.xs.h"

typedef struct sxMarkupParser txMarkupParser;

struct sxMarkupParser {
	xsMachine* the;
	int c;
	int c0;
	int c1;
	char* data;
	int (*dataGetter)(txMarkupParser*);
	int dataOffset;
	int dataSize;
	int depth;
	char* entity;
	int line;
	int root;
	char* value;
	int (*valueGetter)(txMarkupParser*);
	int valueSize;
	char name[1024];
};

typedef struct {
	char name[9];
	char nameLength;
	char value[5];
	char valueLength;
} txEntity;

typedef struct {
	txS4 size;
	txU4 cmask;
	txU4 cval;
	txS4 shift;
	txU4 lmask;
	txU4 lval;
} txUTF8Sequence;

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
	LINE,
	NAME,
	NAMESPACE,
	PARENT,
	PATH,
	PI_PROTOTYPE,
	PREFIX,
	VALUE,
	XML_NAMESPACE,
	XML_PREFIX,
	COUNT
};

static int fxStringGetter(txMarkupParser* self);
static void fxGetCharacter(txMarkupParser* self);
static int fxGetFirstEntityCharacter(txMarkupParser* self);
static int fxGetNextCommentCharacter(txMarkupParser* self);
static int fxGetNextDataCharacter(txMarkupParser* self);
static int fxGetNextEntityCharacter(txMarkupParser* self);
static int fxGetNextTextCharacter(txMarkupParser* self);
static int fxGetNextProcessingInstructionCharacter(txMarkupParser* self);
static int fxGetNextScriptCharacter(txMarkupParser* self);
static int fxGetNextValueCharacter(txMarkupParser* self);
static void fxParseComment(txMarkupParser* self);
static void fxParseDocument(txMarkupParser* self);
static void fxParseName(txMarkupParser* self);
static void fxParseProcessingInstruction(txMarkupParser* self);
static void fxParseStartTag(txMarkupParser* self);
static void fxParseStopTag(txMarkupParser* self);
static void fxParseText(txMarkupParser* self);
static void fxParseValue(txMarkupParser* self);
static void fxReportMarkupError(txMarkupParser* self, char* theFormat, ...);
static int fxScanName(txMarkupParser* self, char* theName, size_t theSize);
static int fxScanSpace(txMarkupParser* self);
static txEntity* fxSearchEntity(xsStringValue theName);
static void fxSkipComment(txMarkupParser* self);
static void fxSkipDeclaration(txMarkupParser* self);
static void fxSkipDocType(txMarkupParser* self);
static void fxSkipProcessingInstruction(txMarkupParser* self);

txUTF8Sequence gxUTF8Sequences[] = {
	{1, 0x80, 0x00, 0*6, 0x0000007F, 0x00000000},
	{2, 0xE0, 0xC0, 1*6, 0x000007FF, 0x00000080},
	{3, 0xF0, 0xE0, 2*6, 0x0000FFFF, 0x00000800},
	{4, 0xF8, 0xF0, 3*6, 0x001FFFFF, 0x00010000},
	{5, 0xFC, 0xF8, 4*6, 0x03FFFFFF, 0x00200000},
	{6, 0xFE, 0xFC, 5*6, 0x7FFFFFFF, 0x04000000},
	{0, 0, 0, 0, 0, 0},
};

void infoset_scan(xsMachine* the)
{
	int c = xsToInteger(xsArgc);
	txMarkupParser self;
	xsVars(COUNT);
	{
		xsTry {
			c_memset(&self, 0, sizeof(self));
			self.line = 1;
			self.value = c_malloc(8192);
			xsThrowIfNULL(self.value);
			self.valueSize = 8192;

			self.the = the;
			xsVar(ATTRIBUTE_PROTOTYPE) = xsGet(xsThis, xsID_attribute);
			xsVar(CDATA_PROTOTYPE) = xsGet(xsThis, xsID_cdata);
			xsVar(COMMENT_PROTOTYPE) = xsGet(xsThis, xsID_comment);
			xsVar(DOCUMENT_PROTOTYPE) = xsGet(xsThis, xsID_document);
			xsVar(ELEMENT_PROTOTYPE) = xsGet(xsThis, xsID_element);
			xsVar(NAMESPACE) = xsNull;
			xsVar(PATH) = (c > 1) ? xsArg(1) : xsUndefined;
			xsVar(PREFIX) = xsNull;
			xsVar(PI_PROTOTYPE) = xsGet(xsThis, xsID_pi);
			xsVar(XML_NAMESPACE) = xsString("http://www.w3.org/XML/1998/namespace");
			xsVar(XML_PREFIX) = xsString("xmlns");
		
			xsResult = xsNewInstanceOf(xsVar(DOCUMENT_PROTOTYPE));
			xsSet(xsResult, xsID_encoding, xsString("UTF-8"));
			xsSet(xsResult, xsID_version, xsString("1.0"));
			xsVar(CHILDREN) = xsNewInstanceOf(xsArrayPrototype);
			xsArrayCacheBegin(xsVar(CHILDREN));
			xsSet(xsResult, xsID_children, xsVar(CHILDREN));
			xsSet(xsResult, xsID_parent, xsNull);
			xsSet(xsResult, xsID_xmlnsAttributes, xsNull);
		
			if (c > 0) {
				self.dataGetter = fxStringGetter;
				self.data = xsToString(xsArg(0));
				self.dataOffset = 0;
				self.dataSize = c_strlen(self.data);
				fxParseDocument(&self);
			}
		}
		xsCatch {
			if (self.value)
				c_free(self.value);
		}
	}
}

int fxStringGetter(txMarkupParser* self)
{
	xsMachine* the = self->the;
	int result = C_EOF;
	if (self->dataOffset < self->dataSize) {
		self->data = xsToString(xsArg(0));
		result = *(self->data + self->dataOffset);
		self->dataOffset++;
	}
	return result;
}

void fxGetCharacter(txMarkupParser* self)
{
	txU4 aResult;
	txUTF8Sequence *aSequence;
	txS4 aSize;

	aResult = (txU4)(*(self->dataGetter))(self);
	if (aResult != (txU4)C_EOF) {
		for (aSequence = gxUTF8Sequences; aSequence->size; aSequence++) {
			if ((aResult & aSequence->cmask) == aSequence->cval)
				break;
		}
		if (aSequence->size == 0)
			fxReportMarkupError(self, "invalid UTF-8 character");
		aSize = aSequence->size - 1;
		while (aSize > 0) {
			aSize--;
			aResult = (aResult << 6) | ((*(self->dataGetter))(self) & 0x3F);
		}
		aResult &= aSequence->lmask;
	}
	self->c = aResult;
}

int fxGetFirstEntityCharacter(txMarkupParser* self)
{
	int c;
	char* p;
	txEntity* anEntity;
	
	fxGetCharacter(self);
	if (self->c == '#') {
		fxGetCharacter(self);
		if (self->c == 'x') {
			fxGetCharacter(self);
			p = self->name;
			while ((('0' <= self->c) && (self->c <= '9')) 
					|| (('A' <= self->c) && (self->c <= 'F'))
					|| (('a' <= self->c) && (self->c <= 'f'))) {
				*p++ = self->c;
				fxGetCharacter(self);
			}
			*p = 0;
			c = c_strtoul(self->name, C_NULL, 16);
		}
		else {
			p = self->name;
			while (('0' <= self->c) && (self->c <= '9'))  {
				*p++ = self->c;
				fxGetCharacter(self);
			}
			*p = 0;
			c = c_strtoul(self->name, C_NULL, 10);
		}
	}
	else {
		fxScanName(self, self->name, sizeof(self->name));
		anEntity = fxSearchEntity(self->name);
		if (anEntity) {
			c_strcpy(self->name, anEntity->value);
			self->entity = self->name;
			c = fxGetNextEntityCharacter(self);
		}
		else {
			fxReportMarkupError(self, "entity not found: %s", self->name);
			c = 0;
		}
	}
	if (self->c == ';')
		fxGetCharacter(self);
	else
		fxReportMarkupError(self, "missing ;");
	return c;
}

int fxGetNextCommentCharacter(txMarkupParser* self)
{
	int c;
	if ((self->c0 == '-') && (self->c1 == '-') && (self->c == '>'))
		c = C_EOF;
	else {
		c = self->c0;
		self->c0 = self->c1;
		self->c1 = self->c;
		fxGetCharacter(self);
		if ((c == 10) || ((c == 13) && (self->c0 != 10)))
			self->line++;
	}
	return c;
}

int fxGetNextDataCharacter(txMarkupParser* self)
{
	int c;
	if ((self->c0 == ']') && (self->c1 == ']') && (self->c == '>'))
		c = C_EOF;
	else {
		c = self->c0;
		self->c0 = self->c1;
		self->c1 = self->c;
		fxGetCharacter(self);
		if ((c == 10) || ((c == 13) && (self->c0 != 10)))
			self->line++;
	}
	return c;
}

int fxGetNextProcessingInstructionCharacter(txMarkupParser* self)
{
	int c;
	if ((self->c0 == '?') && (self->c == '>'))
		c = C_EOF;
	else {
		c = self->c0;
		self->c0 = self->c;
		fxGetCharacter(self);
		if ((c == 10) || ((c == 13) && (self->c0 != 10)))
			self->line++;
	}
	return c;
}

int fxGetNextEntityCharacter(txMarkupParser* self)
{
	txU1* p;
	txU4 c;
	txUTF8Sequence *aSequence;
	txS4 aSize;
	
	p = (txU1*)(self->entity);
	c = (txU4)*p++;
	for (aSequence = gxUTF8Sequences; aSequence->size; aSequence++) {
		if ((c & aSequence->cmask) == aSequence->cval)
			break;
	}
	aSize = aSequence->size - 1;
	while (aSize) {
		aSize--;
		c = (c << 6) | (txU4)*p++;
	}
	c &= aSequence->lmask;
	if (*p)
		self->entity = (xsStringValue)p;
	else
		self->entity = C_NULL;
	return (int)c;
}

int fxGetNextScriptCharacter(txMarkupParser* self)
{
	int c;
	if ((self->c0 == '<') && (self->c == '/'))
		c = C_EOF;
	else {
		c = self->c0;
		self->c0 = self->c;
		fxGetCharacter(self);
		if ((c == 10) || ((c == 13) && (self->c0 != 10)))
			self->line++;
	}
	return c;
}

int fxGetNextTextCharacter(txMarkupParser* self)
{
	int c;
	if (self->entity)
		c = fxGetNextEntityCharacter(self);
	else if (self->c == '&')
		c = fxGetFirstEntityCharacter(self);
	else if (self->c == '<')
		c = C_EOF;
	else {
		c = self->c;
		fxGetCharacter(self);
		if ((c == 10) || ((c == 13) && (self->c != 10)))
			self->line++;
	}
	return c;
}

int fxGetNextValueCharacter(txMarkupParser* self)
{
	int c;
	if (self->entity)
		c = fxGetNextEntityCharacter(self);
	else if (self->c == '&')
		c = fxGetFirstEntityCharacter(self);
	else if (self->c == self->c0)
		c = C_EOF;
	else {
		c = self->c;
		fxGetCharacter(self);
		if ((c == 10) || ((c == 13) && (self->c != 10)))
			self->line++;
	}
	return c;
}

void fxParseDocument(txMarkupParser* self)
{
	xsMachine* the = self->the;

	fxGetCharacter(self);
	for (;;) {
		fxScanSpace(self);
		if (self->c == C_EOF)
			break;
		else if (self->c == '<') {
			fxGetCharacter(self);
			if (self->c == '/') {
				fxGetCharacter(self);
				fxParseStopTag(self);
			}
			else if (self->c == '?') {
				fxGetCharacter(self);
				fxParseProcessingInstruction(self);
			}
			else if (self->c == '!') {
				fxGetCharacter(self);
				if (self->c == '-') {
					fxGetCharacter(self);
					if (self->c != '-')
						fxReportMarkupError(self, "missing -");
					fxGetCharacter(self);
					fxParseComment(self);
				}
				else if (self->c == '[') {
					fxGetCharacter(self);
					if (self->c != 'C')
						fxReportMarkupError(self, "missing C");
					fxGetCharacter(self);
					if (self->c != 'D')
						fxReportMarkupError(self, "missing D");
					fxGetCharacter(self);
					if (self->c != 'A')
						fxReportMarkupError(self, "missing A");
					fxGetCharacter(self);
					if (self->c != 'T')
						fxReportMarkupError(self, "missing T");
					fxGetCharacter(self);
					if (self->c != 'A')
						fxReportMarkupError(self, "missing A");
					fxGetCharacter(self);
					if (self->c != '[')
						fxReportMarkupError(self, "missing [");
					fxGetCharacter(self);
					self->c0 = self->c;
					fxGetCharacter(self);
					self->c1 = self->c;
					fxGetCharacter(self);
					self->valueGetter = fxGetNextDataCharacter;
					fxParseText(self);
					if (self->c != '>')
						fxReportMarkupError(self, "missing ]]>");
					fxGetCharacter(self);
				}
				else if (self->depth == 0) {
					if (self->c != 'D')
						fxReportMarkupError(self, "missing D");
					fxGetCharacter(self);
					if (self->c != 'O')
						fxReportMarkupError(self, "missing O");
					fxGetCharacter(self);
					if (self->c != 'C')
						fxReportMarkupError(self, "missing C");
					fxGetCharacter(self);
					if (self->c != 'T')
						fxReportMarkupError(self, "missing T");
					fxGetCharacter(self);
					if (self->c != 'Y')
						fxReportMarkupError(self, "missing Y");
					fxGetCharacter(self);
					if (self->c != 'P')
						fxReportMarkupError(self, "missing P");
					fxGetCharacter(self);
					if (self->c != 'E')
						fxReportMarkupError(self, "missing E");
					fxGetCharacter(self);
					fxSkipDocType(self);
				}
				else
					fxReportMarkupError(self, "invalid declaration");
			}
			else
				fxParseStartTag(self);
		}
		else {
			self->c0 = self->c;
			fxGetCharacter(self);
			self->valueGetter = fxGetNextScriptCharacter;
			fxParseText(self);
			if (self->c != C_EOF) {
				fxGetCharacter(self);
				fxParseStopTag(self);
			}
		}
	}
	if (!xsIsInstanceOf(xsResult, xsVar(DOCUMENT_PROTOTYPE))) {
		xsVar(NAME) = xsGet(xsResult, xsID_name);
		fxReportMarkupError(self, "EOF instead of </%s>", xsToString(xsVar(NAME)));
	}
}

void fxParseComment(txMarkupParser* self)
{
	xsMachine* the = self->the;

	self->c0 = self->c;
	fxGetCharacter(self);
	if ((self->c == 10) || ((self->c == 13) && (self->c0 != 10)))
		self->line++;
	self->c1 = self->c;
	fxGetCharacter(self);
	if ((self->c == 10) || ((self->c == 13) && (self->c0 != 10)))
		self->line++;
	self->valueGetter = fxGetNextCommentCharacter;
	xsVar(LINE) = xsInteger(self->line);
	fxParseValue(self);
	if (self->c != '>')
		fxReportMarkupError(self, "missing -->");
	fxGetCharacter(self);
	
	xsVar(CHILD) = xsNewInstanceOf(xsVar(COMMENT_PROTOTYPE));
	xsSet(xsVar(CHILD), xsID_path, xsVar(PATH));
	xsSet(xsVar(CHILD), xsID_line, xsVar(LINE));
	xsSet(xsVar(CHILD), xsID_parent, xsResult);
	xsSet(xsVar(CHILD), xsID_value, xsVar(VALUE));
	xsArrayCacheItem(xsVar(CHILDREN), xsVar(CHILD));
}

void fxParseName(txMarkupParser* self)
{
	xsMachine* the = self->the;
	int aLength = fxScanName(self, self->name, sizeof(self->name));
	if (!aLength)
		fxReportMarkupError(self, "missing name");
	xsVar(NAME) = xsString(self->name);	
}

void fxParseProcessingInstruction(txMarkupParser* self)
{
	xsMachine* the = self->the;
	xsVar(LINE) = xsInteger(self->line);
	
	fxParseName(self);
    fxScanSpace(self);
	self->c0 = self->c;
	fxGetCharacter(self);
	self->valueGetter = fxGetNextProcessingInstructionCharacter;
	fxParseValue(self);
	if (self->c != '>')
		fxReportMarkupError(self, "missing ?>");
	fxGetCharacter(self);
	
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

void fxParseStartTag(txMarkupParser* self)
{
	xsMachine* the = self->the;
	int aLength;
	
	xsVar(LINE) = xsInteger(self->line);
	fxParseName(self);
	
	xsVar(CHILD) = xsNewInstanceOf(xsVar(ELEMENT_PROTOTYPE));
	xsSet(xsVar(CHILD), xsID_path, xsVar(PATH));
	xsSet(xsVar(CHILD), xsID_line, xsVar(LINE));
	xsSet(xsVar(CHILD), xsID_name, xsVar(NAME));
	xsSet(xsVar(CHILD), xsID_namespace, xsVar(NAMESPACE));
	xsSet(xsVar(CHILD), xsID_prefix, xsVar(PREFIX));
	xsSet(xsVar(CHILD), xsID_parent, xsResult);
	if (!self->root) {
		self->root = 1;
		xsSet(xsResult, xsID_element, xsVar(CHILD));
	}
	xsArrayCacheItem(xsVar(CHILDREN), xsVar(CHILD));
	xsResult = xsVar(CHILD);
	
	xsVar(CHILDREN) = xsNewInstanceOf(xsArrayPrototype);
	xsArrayCacheBegin(xsVar(CHILDREN));
	
	aLength = fxScanSpace(self);
	while ((self->c != C_EOF) && (self->c != '/') && (self->c != '>')) {
		if (!aLength)
			fxReportMarkupError(self, "missing space");
		fxParseName(self);
		fxScanSpace(self);
		if (self->c != '=')
			fxReportMarkupError(self, "missing =");
		fxGetCharacter(self);
		fxScanSpace(self);
		if ((self->c != '\'') && (self->c != '"'))
			fxReportMarkupError(self, "missing left quote");
		self->c0 = self->c;
		fxGetCharacter(self);
		self->valueGetter = fxGetNextValueCharacter;
		fxParseValue(self);
		if (self->c != self->c0)
			fxReportMarkupError(self, "missing right quote");
		fxGetCharacter(self);
		aLength = fxScanSpace(self);
		
		xsVar(CHILD) = xsNewInstanceOf(xsVar(ATTRIBUTE_PROTOTYPE));
		xsSet(xsVar(CHILD), xsID_parent, xsResult);
		xsSet(xsVar(CHILD), xsID_path, xsVar(PATH));
		xsSet(xsVar(CHILD), xsID_line, xsVar(LINE));
		xsSet(xsVar(CHILD), xsID_name, xsVar(NAME));
		xsSet(xsVar(CHILD), xsID_namespace, xsNull);
		xsSet(xsVar(CHILD), xsID_prefix, xsNull);
		xsSet(xsVar(CHILD), xsID_value, xsVar(VALUE));
		xsArrayCacheItem(xsVar(CHILDREN), xsVar(CHILD));
	}
	xsArrayCacheEnd(xsVar(CHILDREN));
	xsSet(xsResult, xsID_attributes, xsVar(CHILDREN));
	xsVar(CHILDREN) = xsNewInstanceOf(xsArrayPrototype);
	xsArrayCacheBegin(xsVar(CHILDREN));
	xsSet(xsResult, xsID_children, xsVar(CHILDREN));

	if (self->c == '/') {
		xsArrayCacheEnd(xsVar(CHILDREN));
		xsResult = xsGet(xsResult, xsID_parent);
		xsVar(CHILDREN) = xsGet(xsResult, xsID_children);
		
		fxGetCharacter(self);
		
	}
	else {
		self->depth++;
	}
	if (self->c == '>')
		fxGetCharacter(self);
	else
		fxReportMarkupError(self, "missing >");
}

void fxParseStopTag(txMarkupParser* self)
{
	xsMachine* the = self->the;
	int aLength = fxScanName(self, self->name, sizeof(self->name));
	if (!aLength)
		fxReportMarkupError(self, "missing name");
	fxScanSpace(self);
	self->depth--;
	if (self->c == '>')
		fxGetCharacter(self);
	else
		fxReportMarkupError(self, "missing >");
	xsVar(NAME) = xsGet(xsResult, xsID_name);
	if (strcmp(self->name, xsToString(xsVar(NAME))))
		fxReportMarkupError(self, "</%s> instead of </%s>", self->name, xsToString(xsVar(NAME)));
		
	xsArrayCacheEnd(xsVar(CHILDREN));
	xsResult = xsGet(xsResult, xsID_parent);
	xsVar(CHILDREN) = xsGet(xsResult, xsID_children);
}

void fxParseText(txMarkupParser* self)
{
	xsMachine* the = self->the;
	xsVar(LINE) = xsInteger(self->line);
	
	fxParseValue(self);
	
	xsVar(CHILD) = xsNewInstanceOf(xsVar(CDATA_PROTOTYPE));
	xsSet(xsVar(CHILD), xsID_path, xsVar(PATH));
	xsSet(xsVar(CHILD), xsID_line, xsVar(LINE));
	xsSet(xsVar(CHILD), xsID_parent, xsResult);
	xsSet(xsVar(CHILD), xsID_value, xsVar(VALUE));
	xsArrayCacheItem(xsVar(CHILDREN), xsVar(CHILD));
}

void fxParseValue(txMarkupParser* self)
{
	xsMachine* the = self->the;
	size_t i;
	size_t j;
	txU1* p;
	txU4 c;
	txUTF8Sequence* aSequence;
	int aShift;
	
	i = 0;
	j = self->valueSize;
	p = (txU1*)self->value;
	c = (txU4)(*(self->valueGetter))(self);
	while (c != (txU4)C_EOF) {
		if (c < 128) {
			i++;
			if (i >= j) {
				self->valueSize += 1024;
				self->value = c_realloc(self->value, self->valueSize);
				j = self->valueSize;
				p = ((txU1*)self->value) + i - 1;
			}
			*p++ = (txU1)c;
		}
		else {
			for (aSequence = gxUTF8Sequences; aSequence->size; aSequence++)
				if (c <= aSequence->lmask)
					break;
			if (aSequence->size == 0)
				fxReportMarkupError(self, "invalid value");
			else {
				i += aSequence->size;
				if (i >= j) {
					self->valueSize += 1024;
					self->value = c_realloc(self->value, self->valueSize);
					j = self->valueSize;
					p = ((txU1*)self->value) + i - aSequence->size;
				}
				aShift = aSequence->shift;
				*p++ = (txU1)(aSequence->cval | (c >> aShift));
				while (aShift > 0) {
					aShift -= 6;
					*p++ = (txU1)(0x80 | ((c >> aShift) & 0x3F));
				}
			}
		}
		c = (txU4)(*(self->valueGetter))(self);
	}
	i++;
	*p = 0;
	xsVar(VALUE) = xsString(self->value);
}

void fxReportMarkupError(txMarkupParser* self, char* theFormat, ...)
{
	xsMachine* the = self->the;
	c_va_list arguments;
	c_va_start(arguments, theFormat);
	vsnprintf(self->value, self->valueSize, theFormat, arguments);
	c_va_end(arguments);
	xsVar(VALUE) = xsString(self->value);
	xsCall3(xsThis, xsID_reportError, xsVar(PATH), xsVar(LINE), xsVar(VALUE));
	xsThrow(xsNewInstanceOf(xsSyntaxErrorPrototype));
}

int fxScanName(txMarkupParser* self, char* theName, size_t theSize)
{
	size_t i;
	char* p;
	int c;

	i = 0;
	p = theName;
	c = self->c;
	if ((('A' <= c) && (c <= 'Z')) || (('a' <= c) && (c <= 'z')) 
			|| (c == '_') || (c == ':')) {
		i++;
		*p++ = c;
		for (;;) {
			fxGetCharacter(self);
			c = self->c;
			if ((('0' <= c) && (c <= '9')) || (('A' <= c) && (c <= 'Z')) || (('a' <= c) && (c <= 'z')) 
					|| (c == '.') || (c == '-') || (c == '_') || (c == ':')) {
				i++;
				if (i < theSize)
					*p++ = c;
				else if (i == theSize)
					fxReportMarkupError(self, "name overflow");
			}
			else
				break;
		}
	}
	*p = 0;
	return i;	
}

int fxScanSpace(txMarkupParser* self)
{
	int i = 0;
	
	for (;;) {
		switch (self->c) {
		case 10:
			i++;	
			self->line++;
			fxGetCharacter(self);
			break;
		case 13:	
			i++;	
			self->line++;
			fxGetCharacter(self);
			if (self->c == 10)
				fxGetCharacter(self);
			break;
		case '\t':
		case ' ':
			i++;	
			fxGetCharacter(self);
			break;
		default:
			return i;
		}
	}
}

void fxSkipComment(txMarkupParser* self)
{
	int c;
	self->c0 = self->c;
	fxGetCharacter(self);
	self->c1 = self->c;
	fxGetCharacter(self);
	do {
		if ((self->c0 == '-') && (self->c1 == '-') && (self->c == '>'))
			c = C_EOF;
		else {
			c = self->c0;
			self->c0 = self->c1;
			self->c1 = self->c;
			fxGetCharacter(self);
			if ((c == 10) || ((c == 13) && (self->c0 != 10)))
				self->line++;
		}
	} while (c != C_EOF);
	if (self->c == '>')
		fxGetCharacter(self);
	else
		fxReportMarkupError(self, "missing -->");
}

void fxSkipDeclaration(txMarkupParser* self)
{
	int c;
	do {
		if (self->c == '>')
			c = C_EOF;
		else {
			c = self->c;
			fxGetCharacter(self);
			if ((c == 10) || ((c == 13) && (self->c != 10)))
				self->line++;
		}
	} while (c != C_EOF);
	if (self->c == '>')
		fxGetCharacter(self);
	else
		fxReportMarkupError(self, "missing >");
}

void fxSkipDocType(txMarkupParser* self)
{
	int c;
	do {
		if ((self->c == '[') || (self->c == '>'))
			c = C_EOF;
		else {
			c = self->c;
			fxGetCharacter(self);
			if ((c == 10) || ((c == 13) && (self->c != 10)))
				self->line++;
		}
	} while (c != C_EOF);
	if (self->c == '[') {
		fxGetCharacter(self);
		for (;;) {
			fxScanSpace(self);
			if (self->c == C_EOF)
				break;
			else if (self->c == ']')
				break;
			else if (self->c == '<') {
				fxGetCharacter(self);
				if (self->c == '?') {
					fxGetCharacter(self);
					fxSkipProcessingInstruction(self);
				}
				else if (self->c == '!') {
					fxGetCharacter(self);
					if (self->c == '-') {
						fxGetCharacter(self);
						if (self->c != '-')
							fxReportMarkupError(self, "missing -");
						fxGetCharacter(self);
						fxSkipComment(self);
					}
					else
						fxSkipDeclaration(self);
				}
			}
		}
		if (self->c == ']')
			fxGetCharacter(self);
		else
			fxReportMarkupError(self, "missing ]");
		fxScanSpace(self);
	}
	if (self->c == '>')
		fxGetCharacter(self);
	else
		fxReportMarkupError(self, "missing >");
}
				
void fxSkipProcessingInstruction(txMarkupParser* self)
{
	int c;
	self->c0 = self->c;
	fxGetCharacter(self);
	do {
		if ((self->c0 == '?') && (self->c == '>'))
			c = C_EOF;
		else {
			c = self->c0;
			self->c0 = self->c;
			fxGetCharacter(self);
			if ((c == 10) || ((c == 13) && (self->c0 != 10)))
				self->line++;
		}
	} while (c != C_EOF);
	if (self->c == '>')
		fxGetCharacter(self);
	else
		fxReportMarkupError(self, "missing ?>");
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
	return c_strcmp((xsStringValue)theName, ((txEntity*)theEntity)->name);
}

txEntity* fxSearchEntity(xsStringValue theName)
{
	return (txEntity* )bsearch(theName, gxEntities, mxEntityCount, sizeof(txEntity), fxCompareEntities);
}


