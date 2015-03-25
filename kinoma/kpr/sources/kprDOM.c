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
#include "kpr.h"

enum {
	ATTRIBUTE_PROTOTYPE,
	CDATA_PROTOTYPE,
	ELEMENT_PROTOTYPE,
	PI_PROTOTYPE,
	ATTRIBUTE_NAMES_PAGES,
	ATTRIBUTE_NAMES,
	ATTRIBUTE_VALUES_PAGES,
	ATTRIBUTE_VALUES,
	ELEMENT_TAGS_PAGES,
	ELEMENT_TAGS,
	NAMESPACE,
	NAME,
	TMP,
	VALUE,
	DOCUMENT
};

static void KPR_DOM_parseBinaryAdd(xsMachine *the, SInt32 PARENT, SInt32 CHILD, xsIndex id)
{
	if (!xsHasOwn(xsVar(PARENT), id))
		xsSet(xsVar(PARENT), id, xsNewInstanceOf(xsArrayPrototype));
	(void)xsCall1(xsGet(xsVar(PARENT), id), xsID("push"), xsVar(CHILD));
	xsSet(xsVar(CHILD), xsID("parent"), xsVar(PARENT));
}

static UInt32 KPR_DOM_parseBinaryInteger(xsMachine *the, UInt8 **address)
{
	UInt8* p = *address;
	UInt8 c;
	UInt32 mb = 0;
	do {
		c = *p++;
		mb = (mb << 7) | (c & 0x7f);
	} while (c & 0x80);
	*address = p;
	return mb;
}

void KPR_DOM_parseBinary(xsMachine *the)
{
	UInt8* p;
	UInt8* q;
	UInt8 /* version, pubid, charset, */ page, *strtbl;
	SInt32 PARENT = DOCUMENT;
	SInt32 CHILD = PARENT + 1;
	UInt32 length;
	Boolean processingAttributes = 0, hasContent = 0;
	xsVars(64);
	
	xsEnterSandbox();
	xsVar(ELEMENT_TAGS_PAGES) = xsGet(xsArg(1), xsID("tagTokenPages"));
	xsVar(TMP) = xsGetAt(xsVar(ELEMENT_TAGS_PAGES), xsInteger(0));
	xsVar(ELEMENT_TAGS) = xsGet(xsVar(TMP), xsID("tokens"));
	xsVar(NAMESPACE) = xsGet(xsVar(TMP), xsID("namespace"));
	
	xsVar(ATTRIBUTE_NAMES_PAGES) = xsGet(xsArg(1), xsID("attributeStartTokenPages"));
	xsVar(ATTRIBUTE_NAMES) = xsGet(xsGetAt(xsVar(ATTRIBUTE_NAMES_PAGES), xsInteger(0)), xsID("tokens"));
	
	xsVar(ATTRIBUTE_VALUES_PAGES) = xsGet(xsArg(1), xsID("attributeValueTokenPages"));
	xsVar(ATTRIBUTE_VALUES) = xsGet(xsGetAt(xsVar(ATTRIBUTE_VALUES_PAGES), xsInteger(0)), xsID("tokens"));
	xsLeaveSandbox();

	xsVar(ELEMENT_PROTOTYPE) = xsGet(xsThis, xsID("element"));
	xsVar(ATTRIBUTE_PROTOTYPE) = xsGet(xsThis, xsID("attribute"));
	xsVar(CDATA_PROTOTYPE) = xsGet(xsThis, xsID("cdata"));
	xsVar(PI_PROTOTYPE) = xsGet(xsThis, xsID("pi"));
	xsVar(VALUE) = xsString("");
	xsResult = xsVar(DOCUMENT) = xsNewInstanceOf(xsGet(xsThis, xsID("document")));
	
	p = xsGetHostData(xsArg(0));
	q = p + xsToInteger(xsGet(xsArg(0), xsID("length")));
//	version = *p++;
//	pubid = *p++;
//	charset = *p++;
	xsSet(xsResult, xsID("encoding"), xsString("UTF-8")); // @@
	xsSet(xsResult, xsID("version"), xsString("1.0")); // @@
	length = KPR_DOM_parseBinaryInteger(the, &p);
	strtbl = p;
	p += length;
	while (p < q) {
		UInt8 token = *p++;
		switch (token) {
		case 0x00:	// SWITCH PAGE
			page = *p++;
			xsEnterSandbox();
			if (processingAttributes) {
				xsVar(ATTRIBUTE_NAMES) = xsGet(xsGetAt(xsVar(ATTRIBUTE_NAMES_PAGES), xsInteger(page)), xsID("tokens"));
				xsVar(ATTRIBUTE_VALUES) = xsGet(xsGetAt(xsVar(ATTRIBUTE_VALUES_PAGES), xsInteger(page)), xsID("tokens"));
			}
			else {
				xsVar(TMP) = xsGetAt(xsVar(ELEMENT_TAGS_PAGES), xsInteger(page));
				xsVar(ELEMENT_TAGS) = xsGet(xsVar(TMP), xsID("tokens"));
				xsVar(NAMESPACE) = xsGet(xsVar(TMP), xsID("namespace"));
			}
			xsLeaveSandbox();
			break;
		case 0x02:	// ENTITY
			xsVar(TMP) = xsCall1(xsGet(xsGlobal, xsID("String")), xsID("fromCharCode"), xsInteger(KPR_DOM_parseBinaryInteger(the, &p)));
			xsVar(VALUE) = xsCall1(xsVar(VALUE), xsID("concat"), xsVar(TMP));
			break;
		case 0x03:	// STR_I
			xsVar(VALUE) = xsCall1(xsVar(VALUE), xsID("concat"), xsString((xsStringValue)p));
			p += FskStrLen((xsStringValue)p) + 1;
			break;
		case 0x40:	// EXT_I_0
		case 0x41:	// EXT_I_1
		case 0x42:	// EXT_I_2
			p += FskStrLen((xsStringValue)p) + 1;
			break;
		case 0x43:	// PI
			// not supported
			while (*p++ != 0x01)
				;
			break;
		case 0x80:	// EXT_T_0
		case 0x81:	// EXT_T_1
		case 0x82:	// EXT_T_2
			/* length = */ KPR_DOM_parseBinaryInteger(the, &p);
			break;
		case 0x83:	// STR_T
			xsVar(TMP) = xsString((xsStringValue)strtbl + KPR_DOM_parseBinaryInteger(the, &p));
			xsVar(VALUE) = xsCall1(xsVar(VALUE), xsID("concat"), xsVar(TMP));
			break;
		case 0xc0:	// EXT_0
		case 0xc1:	// EXT_1
		case 0xc2:	// EXT_2
			break;
		case 0xc3:	// OPAQUE
			length = KPR_DOM_parseBinaryInteger(the, &p);
			p += length;
			break;
		default:
			if (processingAttributes) {
				if (token < 128) {
					if (xsTest(xsVar(VALUE))) {
						xsVar(CHILD) = xsNewInstanceOf(xsVar(ATTRIBUTE_PROTOTYPE));
						xsSet(xsVar(CHILD), xsID("name"), xsVar(NAME));
						xsSet(xsVar(CHILD), xsID("value"), xsVar(VALUE));
						KPR_DOM_parseBinaryAdd(the, PARENT, CHILD, xsID("_attributes"));
						xsVar(VALUE) = xsString("");
					}
					if (token == 0x01) { // END
						processingAttributes = false;
						if (!hasContent) {
							PARENT--;
							CHILD--;
						}
					}
					else {
						if (token == 0x04) { // LITERAL
							xsVar(NAME) = xsString((xsStringValue)strtbl + KPR_DOM_parseBinaryInteger(the, &p));
						}
						else {
							xsVar(NAME) = xsGetAt(xsVar(ATTRIBUTE_NAMES), xsInteger(token));
						}
						xsVar(TMP) = xsCall1(xsVar(NAME), xsID("split"), xsString("="));
						if (xsToInteger(xsGet(xsVar(TMP), xsID("length"))) == 2) {
							xsVar(NAME) = xsGetAt(xsVar(TMP), xsInteger(0));
							xsVar(VALUE) = xsGetAt(xsVar(TMP), xsInteger(1));
						}
					}
				}
				else {
					xsVar(TMP) = xsGetAt(xsVar(ATTRIBUTE_VALUES), xsInteger(token - 128));
					xsVar(VALUE) = xsCall1(xsVar(VALUE), xsID("concat"), xsVar(TMP));
				}
			}
			else {
				if (xsTest(xsVar(VALUE))) {
					xsVar(CHILD) = xsNewInstanceOf(xsVar(CDATA_PROTOTYPE));
					xsSet(xsVar(CHILD), xsID("value"), xsVar(VALUE));
					KPR_DOM_parseBinaryAdd(the, PARENT, CHILD, xsID("children"));
					xsVar(VALUE) = xsString("");
				}
				if (token == 0x01) { // END
					PARENT--;
					CHILD--;
				}
				else {
					if ((token & 0x3f) == 0x04) {	// LITERAL (or A or C or AC)
						xsVar(NAME) = xsString((xsStringValue)strtbl + KPR_DOM_parseBinaryInteger(the, &p));
					}
					else {
						xsVar(NAME) = xsGetAt(xsVar(ELEMENT_TAGS), xsInteger(token & 0x3f));
					}
					processingAttributes = (token & 0x80) != 0;
					hasContent = (token & 0x40) != 0;
					xsVar(CHILD) = xsNewInstanceOf(xsVar(ELEMENT_PROTOTYPE));
					xsSet(xsVar(CHILD), xsID("namespace"), xsVar(NAMESPACE));
					xsSet(xsVar(CHILD), xsID("name"), xsVar(NAME));
					if (PARENT == DOCUMENT)
						xsSet(xsResult, xsID("element"), xsVar(CHILD));
					KPR_DOM_parseBinaryAdd(the, PARENT, CHILD, xsID("children"));
					if (processingAttributes || hasContent) {
						PARENT++;
						CHILD++;
					}
				}
			}
			break;
		}
	}
}

void KPR_DOM_serializeBinaryAux(xsMachine *the)
{
	xsIntegerValue c, i, size;
	UInt8* p;
	xsVars(1);
	c = xsToInteger(xsGet(xsArg(0), xsID("length")));
	size = 0;
	for (i = 0; i < c; i++) {
		xsVar(0) = xsGetAt(xsArg(0), xsInteger(i));
		if (xsTypeOf(xsVar(0)) == xsIntegerType)
			size++;
		else
			size += FskStrLen(xsToString(xsVar(0))) + 1;
	}
	xsResult = xsNew1(xsGlobal, xsID_Chunk, xsInteger(size));
	p = xsGetHostData(xsResult);
	for (i = 0; i < c; i++) {
		xsVar(0) = xsGetAt(xsArg(0), xsInteger(i));
		if (xsTypeOf(xsVar(0)) == xsIntegerType)
			*p++ = (UInt8)xsToInteger(xsVar(0));
		else {
			xsStringValue string = xsToString(xsVar(0));
			size = FskStrLen(string) + 1;
			FskMemCopy(p, string, size);
			p += size;
		}
	}
}
