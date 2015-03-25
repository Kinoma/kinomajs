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
#define mxTest(_SLOT) \
	(*(--the->stack) = *(_SLOT), \
	fxRunTest(the))

static void fx_xs_infoset_compareAttributes(txMachine* the);

static void fx_xs_infoSet_parse(txMachine* the);
static void fxParseInfoSetAttribute(txMachine* the, txSlot* theAttribute, txSlot* theGrammar, txSlot* theInstance);
static void fxParseInfoSetAttributes(txMachine* the, txSlot* theElement, txSlot* theGrammar, txSlot* theInstance);
static void fxParseInfoSetCData(txMachine* the, txSlot* theData, txSlot* theGrammar, txSlot* theInstance);
static void fxParseInfoSetChildren(txMachine* the, txSlot* theElement, txSlot* theGrammar, txSlot* theInstance);
static void fxParseInfoSetElement(txMachine* the, txSlot* theElement, txSlot* theGrammar, txSlot* theInstance);
static void fxParseInfoSetError(txMachine* the, char* theFormat, ...);
static void fxParseInfoSetPI(txMachine* the, txSlot* thePI, txSlot* theGrammar, txSlot* theInstance);
static txSlot* fxParseInfoSetRule(txMachine* the, txSlot* theInstance, txSlot* theRule, txID* theID);
static void fxParseInfoSetValue(txMachine* the, txString theValue, txSlot* theNode, txSlot* theProperty);

static void fx_xs_infoSet_print(txMachine* the);
static void fxPrintInfoSetAttribute(txMachine* the, txStringStream* theStream, txSlot* theAttribute);
static void fxPrintInfoSetAttributes(txMachine* the, txStringStream* theStream, txSlot* theElement, txID theID);
static void fxPrintInfoSetCData(txMachine* the, txStringStream* theStream, txSlot* theCData, txInteger theLevel);
static void fxPrintInfoSetCharacters(txMachine* the, txStringStream* theStream, char* theCharacters);
static void fxPrintInfoSetChildren(txMachine* the, txStringStream* theStream, txSlot* theElement, txInteger theLevel);
static void fxPrintInfoSetComment(txMachine* the, txStringStream* theStream, txSlot* theComment, txInteger theLevel);
static void fxPrintInfoSetElement(txMachine* the, txStringStream* theStream, txSlot* theElement, txInteger theLevel);
static void fxPrintInfoSetPI(txMachine* the, txStringStream* theStream, txSlot* thePI, txInteger theLevel);
static void fxPrintInfoSetValue(txMachine* the, txStringStream* theStream, txSlot* theSlot, txFlag theFlag);

static void fx_xs_infoSet_scan(txMachine* the);
static void fxScanInfoSetAdd(txMachine* the, txSlot* theParent, txID theID, txSlot* theChild);
static void fxScanInfoSetArrange(txMachine* the, txSlot* theParent, txID theID);
static void fxScanInfoSetAttributes(txMachine* the, txSlot* theSlot);
static void fxScanInfoSetChild(txMachine* the, txID theID);
static void fxScanInfoSetComment(txMachine* the);
static void fxScanInfoSetName(txMachine* the, txSlot* theSlot);
static txSlot* fxScanInfoSetNamespace(txMachine* the, char* thePrefix, int theLength);
static void fxScanInfoSetNamespaces(txMachine* the, txSlot* theSlot);
static void fxScanInfoSetPI(txMachine* the);
static void fxScanInfoSetStartTag(txMachine* the);
static void fxScanInfoSetStopTag(txMachine* the);
static void fxScanInfoSetText(txMachine* the);
static void fxScanInfoSetValue(txMachine* the, txSlot* theSlot);

static void fx_xs_infoSet_serialize(txMachine* the);
static void fxSerializeInfoSetAttribute(txMachine* the, txSlot* theRoot, txSlot* theNode, txSlot* theProperty);
static void fxSerializeInfoSetCData(txMachine* the, txSlot* theRoot, txSlot* theNode, txSlot* theProperty);
static void fxSerializeInfoSetChild(txMachine* the);
static txBoolean fxSerializeInfoSetContent(txMachine* the, txSlot* theRoot, txSlot* theNode, txSlot* theProperty, txID theParent);
static txBoolean fxSerializeInfoSetContents(txMachine* the, txSlot* theRoot, txSlot* theNode, txSlot* theProperty);
static void fxSerializeInfoSetDocument(txMachine* the);
static void fxSerializeInfoSetElement(txMachine* the, txSlot* theRoot, txSlot* theNode, txSlot* theProperty);
static void fxSerializeInfoSetGrammar(txMachine* the, txSlot* theRoot, txSlot* theGrammar, txSlot* theInstance);
static void fxSerializeInfoSetName(txMachine* the, txSlot* theRoot, txSlot* theNode);
static void fxSerializeInfoSetNamespaces(txMachine* the, txSlot* theRoot);
static void fxSerializeInfoSetNode(txMachine* the, txSlot* theRoot, txSlot* theNode, txSlot* theProperty);
static void fxSerializeInfoSetPI(txMachine* the, txSlot* theRoot, txSlot* theNode, txSlot* theProperty);
static txSlot* fxSerializeInfoSetRule(txMachine* the, txSlot* theInstance, txSlot* theRule);
static void fxSerializeInfoSetValue(txMachine* the, txSlot* theRoot, txSlot* theNode, txSlot* theProperty);

static void fx_xs_infoset_get_attributes(txMachine* the);
static void fx_xs_infoset_set_attributes(txMachine* the);

static void fx_xs_execute(txMachine* the);
#ifdef mxDebug
static void fx_xs_debug_getAddress(txMachine* the);
static void fx_xs_debug_setAddress(txMachine* the);
static void fx_xs_debug_getAutomatic(txMachine* the);
static void fx_xs_debug_setAutomatic(txMachine* the);
static void fx_xs_debug_getBreakOnException(txMachine* the);
static void fx_xs_debug_setBreakOnException(txMachine* the);
static void fx_xs_debug_getConnected(txMachine* the);
static void fx_xs_debug_setConnected(txMachine* the);
static void fx_xs_debug_clearAllBreakpoints(txMachine* the);
static void fx_xs_debug_clearBreakpoint(txMachine* the);
static void fx_xs_debug_setBreakpoint(txMachine* the);
#endif

void fxBuildInfoSet(txMachine* the)
{
	txSlot* xs = fxGetProperty(the, mxGlobal.value.reference, fxID(the, "xs"));
	mxPush(*xs);
	
	mxPush(mxObjectPrototype);
	fxNewObjectInstance(the);

	mxPush(mxObjectPrototype);
	fxNewObjectInstance(the);
	mxPush(mxArrayPrototype);
	fxNewArrayInstance(the);
	fxQueueID(the, fxID(the, "children"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SCRIPT_FLAG);
	mxZeroSlot(--the->stack);
	fxQueueID(the, fxID(the, "element"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SCRIPT_FLAG);
	mxZeroSlot(--the->stack);
	fxQueueID(the, fxID(the, "encoding"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SCRIPT_FLAG);
	mxZeroSlot(--the->stack);
	fxQueueID(the, fxID(the, "version"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SCRIPT_FLAG);
	fxAliasInstance(the, the->stack);
	fxQueueID(the, fxID(the, "document"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SCRIPT_FLAG);
	
	mxPush(mxObjectPrototype);
	fxNewObjectInstance(the);
	mxPush(mxArrayPrototype);
	fxNewArrayInstance(the);
	fxQueueID(the, fxID(the, "_attributes"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SCRIPT_FLAG);
	fxNewHostFunction(the, fx_xs_infoset_get_attributes, 0);
	fxQueueID(the, fxID(the, "attributes"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SCRIPT_FLAG | XS_GETTER_FLAG);
	fxNewHostFunction(the, fx_xs_infoset_set_attributes, 1);
	fxQueueID(the, fxID(the, "attributes"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SCRIPT_FLAG | XS_SETTER_FLAG);
	mxPush(mxArrayPrototype);
	fxNewArrayInstance(the);
	fxQueueID(the, fxID(the, "children"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SCRIPT_FLAG);
	mxZeroSlot(--the->stack);
	fxQueueID(the, fxID(the, "instance"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SCRIPT_FLAG);
	mxZeroSlot(--the->stack);
	fxQueueID(the, fxID(the, "name"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SCRIPT_FLAG);
	mxZeroSlot(--the->stack);
	fxQueueID(the, fxID(the, "namespace"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SCRIPT_FLAG);
	mxZeroSlot(--the->stack);
	fxQueueID(the, fxID(the, "parent"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SCRIPT_FLAG);
	mxZeroSlot(--the->stack);
	fxQueueID(the, fxID(the, "prefix"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SCRIPT_FLAG);
	mxPush(mxArrayPrototype);
	fxNewArrayInstance(the);
	fxQueueID(the, fxID(the, "xmlnsAttributes"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SCRIPT_FLAG);
	mxZeroSlot(--the->stack);
	fxQueueID(the, fxID(the, "_link"), XS_DONT_ENUM_FLAG | XS_DONT_SCRIPT_FLAG);
	fxAliasInstance(the, the->stack);
	fxQueueID(the, fxID(the, "element"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SCRIPT_FLAG);
	
	mxPush(mxObjectPrototype);
	fxNewObjectInstance(the);
	mxZeroSlot(--the->stack);
	fxQueueID(the, fxID(the, "name"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SCRIPT_FLAG);
	mxZeroSlot(--the->stack);
	fxQueueID(the, fxID(the, "namespace"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SCRIPT_FLAG);
	mxZeroSlot(--the->stack);
	fxQueueID(the, fxID(the, "parent"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SCRIPT_FLAG);
	mxZeroSlot(--the->stack);
	fxQueueID(the, fxID(the, "prefix"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SCRIPT_FLAG);
	mxZeroSlot(--the->stack);
	fxQueueID(the, fxID(the, "value"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SCRIPT_FLAG);
	mxZeroSlot(--the->stack);
	fxQueueID(the, fxID(the, "_link"), XS_DONT_ENUM_FLAG | XS_DONT_SCRIPT_FLAG);
	fxAliasInstance(the, the->stack);
	fxQueueID(the, fxID(the, "attribute"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SCRIPT_FLAG);
	
	mxPush(mxObjectPrototype);
	fxNewObjectInstance(the);
	mxZeroSlot(--the->stack);
	fxQueueID(the, fxID(the, "parent"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SCRIPT_FLAG);
	mxZeroSlot(--the->stack);
	fxQueueID(the, fxID(the, "value"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SCRIPT_FLAG);
	mxZeroSlot(--the->stack);
	fxQueueID(the, fxID(the, "_link"), XS_DONT_ENUM_FLAG | XS_DONT_SCRIPT_FLAG);
	fxAliasInstance(the, the->stack);
	fxQueueID(the, fxID(the, "cdata"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SCRIPT_FLAG);
	
	mxPush(mxObjectPrototype);
	fxNewObjectInstance(the);
	mxZeroSlot(--the->stack);
	fxQueueID(the, fxID(the, "name"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SCRIPT_FLAG);
	mxZeroSlot(--the->stack);
	fxQueueID(the, fxID(the, "namespace"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SCRIPT_FLAG);
	mxZeroSlot(--the->stack);
	fxQueueID(the, fxID(the, "parent"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SCRIPT_FLAG);
	mxZeroSlot(--the->stack);
	fxQueueID(the, fxID(the, "prefix"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SCRIPT_FLAG);
	mxZeroSlot(--the->stack);
	fxQueueID(the, fxID(the, "value"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SCRIPT_FLAG);
	mxZeroSlot(--the->stack);
	fxQueueID(the, fxID(the, "_link"), XS_DONT_ENUM_FLAG | XS_DONT_SCRIPT_FLAG);
	fxAliasInstance(the, the->stack);
	fxQueueID(the, fxID(the, "pi"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SCRIPT_FLAG);
	
	mxPush(mxObjectPrototype);
	fxNewObjectInstance(the);
	mxZeroSlot(--the->stack);
	fxQueueID(the, fxID(the, "parent"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SCRIPT_FLAG);
	mxZeroSlot(--the->stack);
	fxQueueID(the, fxID(the, "value"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SCRIPT_FLAG);
	mxZeroSlot(--the->stack);
	fxQueueID(the, fxID(the, "_link"), XS_DONT_ENUM_FLAG | XS_DONT_SCRIPT_FLAG);
	fxAliasInstance(the, the->stack);
	fxQueueID(the, fxID(the, "comment"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SCRIPT_FLAG);
	
	fxNewHostFunction(the, fx_xs_infoset_compareAttributes, 2);
	fxQueueID(the, fxID(the, "compareAttributes"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SCRIPT_FLAG | XS_DONT_SET_FLAG);
	fxNewHostFunction(the, fx_xs_infoSet_parse, 1);
	fxQueueID(the, fxID(the, "parse"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SCRIPT_FLAG | XS_DONT_SET_FLAG);
	fxNewHostFunction(the, fx_xs_infoSet_print, 1);
	fxQueueID(the, fxID(the, "print"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SCRIPT_FLAG | XS_DONT_SET_FLAG);
	fxNewHostFunction(the, fx_xs_infoSet_scan, 1);
	fxQueueID(the, fxID(the, "scan"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SCRIPT_FLAG | XS_DONT_SET_FLAG);
	fxNewHostFunction(the, fx_xs_infoSet_serialize, 1);
	fxQueueID(the, fxID(the, "serialize"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SCRIPT_FLAG | XS_DONT_SET_FLAG);
	
	mxPushInteger(0);
	fxQueueID(the, fxID(the, "PARSE_DEFAULT"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SCRIPT_FLAG | XS_DONT_SET_FLAG);
	mxPushInteger(1);
	fxQueueID(the, fxID(the, "PARSE_NO_SOURCE"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SCRIPT_FLAG | XS_DONT_SET_FLAG);
	mxPushInteger(2);
	fxQueueID(the, fxID(the, "PARSE_NO_ERROR"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SCRIPT_FLAG | XS_DONT_SET_FLAG);
	mxPushInteger(4);
	fxQueueID(the, fxID(the, "PARSE_NO_WARNING"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SCRIPT_FLAG | XS_DONT_SET_FLAG);
	mxPushInteger(64);
	fxQueueID(the, fxID(the, "PARSE_NO_REFERENCE"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SCRIPT_FLAG | XS_DONT_SET_FLAG);
	mxPushInteger(0);
	fxQueueID(the, fxID(the, "PRINT_DEFAULT"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SCRIPT_FLAG | XS_DONT_SET_FLAG);
	mxPushInteger(1);
	fxQueueID(the, fxID(the, "PRINT_NO_COMMENT"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SCRIPT_FLAG | XS_DONT_SET_FLAG);
	mxPushInteger(0);
	fxQueueID(the, fxID(the, "SERIALIZE_DEFAULT"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SCRIPT_FLAG | XS_DONT_SET_FLAG);
	mxPushInteger(64);
	fxQueueID(the, fxID(the, "SERIALIZE_NO_REFERENCE"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SCRIPT_FLAG | XS_DONT_SET_FLAG);

	mxPushStringC("xmlns");
	fxQueueID(the, fxID(the, "xmlnsPrefix"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SCRIPT_FLAG | XS_DONT_SET_FLAG);
	mxPushStringC("http://www.w3.org/XML/1998/namespace");
	fxQueueID(the, fxID(the, "xmlnsNamespace"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SCRIPT_FLAG | XS_DONT_SET_FLAG);

	fxAliasInstance(the, the->stack);
	fxQueueID(the, fxID(the, "infoset"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SCRIPT_FLAG);

	fxNewHostFunction(the, fx_xs_execute, 1);
	fxQueueID(the, fxID(the, "execute"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SCRIPT_FLAG | XS_DONT_SET_FLAG);
#ifdef mxDebug
	mxPush(mxObjectPrototype);
	fxNewObjectInstance(the);
	
	fxNewHostFunction(the, fx_xs_debug_getAddress, 0);
	fxQueueID(the, fxID(the, "getAddress"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SCRIPT_FLAG | XS_DONT_SET_FLAG);
	fxNewHostFunction(the, fx_xs_debug_setAddress, 1);
	fxQueueID(the, fxID(the, "setAddress"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SCRIPT_FLAG | XS_DONT_SET_FLAG);
	
	fxNewHostFunction(the, fx_xs_debug_getAutomatic, 0);
	fxQueueID(the, fxID(the, "getAutomatic"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SCRIPT_FLAG | XS_DONT_SET_FLAG);
	fxNewHostFunction(the, fx_xs_debug_setAutomatic, 1);
	fxQueueID(the, fxID(the, "setAutomatic"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SCRIPT_FLAG | XS_DONT_SET_FLAG);
	
	fxNewHostFunction(the, fx_xs_debug_getBreakOnException, 0);
	fxQueueID(the, fxID(the, "getBreakOnException"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SCRIPT_FLAG | XS_DONT_SET_FLAG);
	fxNewHostFunction(the, fx_xs_debug_setBreakOnException, 1);
	fxQueueID(the, fxID(the, "setBreakOnException"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SCRIPT_FLAG | XS_DONT_SET_FLAG);
	
	fxNewHostFunction(the, fx_xs_debug_getConnected, 0);
	fxQueueID(the, fxID(the, "getConnected"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SCRIPT_FLAG | XS_DONT_SET_FLAG);
	fxNewHostFunction(the, fx_xs_debug_setConnected, 1);
	fxQueueID(the, fxID(the, "setConnected"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SCRIPT_FLAG | XS_DONT_SET_FLAG);
	
	fxNewHostFunction(the, fx_xs_debug_clearAllBreakpoints, 0);
	fxQueueID(the, fxID(the, "clearAllBreakpoints"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SCRIPT_FLAG | XS_DONT_SET_FLAG);
	fxNewHostFunction(the, fx_xs_debug_clearBreakpoint, 2);
	fxQueueID(the, fxID(the, "clearBreakpoint"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SCRIPT_FLAG | XS_DONT_SET_FLAG);
	fxNewHostFunction(the, fx_xs_debug_setBreakpoint, 2);
	fxQueueID(the, fxID(the, "setBreakpoint"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SCRIPT_FLAG | XS_DONT_SET_FLAG);

	fxQueueID(the, fxID(the, "debug"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SCRIPT_FLAG);
#endif

	the->stack++;
}

void fx_xs_infoset_compareAttributes(txMachine* the)
{
	txInteger result = 0;
	txSlot* a = fxGetInstance(the, mxArgv(0));
	txSlot* b = fxGetInstance(the, mxArgv(1));
	txSlot* as;
	txSlot* bs;
	if (a && b) {
		as = fxGetOwnProperty(the, a, the->namespaceID);
		bs = fxGetOwnProperty(the, b, the->namespaceID);
		if (as && bs)
			result = c_strcmp(as->value.string, bs->value.string);
		else if (as)
			result = 1;
		else if (bs)
			result = -1;
		if (!result) {
			as = fxGetOwnProperty(the, a, the->nameID);
			bs = fxGetOwnProperty(the, b, the->nameID);
			if (c_strcmp(as->value.string, "xmlns") == 0)
				result = -1;
			else if (c_strcmp(bs->value.string, "xmlns") == 0)
				result = 1;
			else
				result = c_strcmp(as->value.string, bs->value.string);
		}
	}
	mxResult->value.integer = result;
	mxResult->kind = XS_INTEGER_KIND;
}

void fx_xs_infoSet_parse(txMachine* the)
{
	txSlot* aDocument;
	txFlag aFlag = 0;
	txSlot* aSlot;
	txSlot* grammars;
	txSlot* aGrammar;
	txSlot* aRule;
	txInteger aCount;
	txSlot* anArgument;
	
	fxVars(the, 1);
	if (mxArgc < 1)
		mxDebug0(the, XS_SYNTAX_ERROR, "xs.infoset.parse: no document");
	aDocument = fxGetInstance(the, mxArgv(0));
	if (!aDocument)
		mxDebug0(the, XS_SYNTAX_ERROR, "xs.infoset.parse: document is no instance");
	if ((mxArgc > 1) && (mxArgv(1)->kind == XS_INTEGER_KIND))
		aFlag = (txFlag)(mxArgv(1)->value.integer);
	else
		aFlag = 0;
	aFlag ^= 1;
	the->parseFlag = aFlag | XS_INFO_SET_FLAG;
	
	aSlot = fxGetProperty(the, mxGlobal.value.reference, the->grammarsID);
	grammars = fxGetInstance(the, aSlot);
	if (!(grammars->next))
		mxDebug0(the, XS_SYNTAX_ERROR, "parseInfoSet: no roots");
	fxNewInstance(the);
	aGrammar = aRule = the->stack->value.reference;
	if (mxArgc > 2) {
		aCount = mxArgc - 2;
		anArgument = mxArgv(2);
		while (aCount) {
			if (anArgument->kind != XS_ALIAS_KIND)
				mxDebug0(the, XS_TYPE_ERROR, "parseInfoSet: no prototype");
			aSlot = grammars->next;
			while (aSlot) {
				if (anArgument->value.alias == aSlot->value.reference->ID)
					break;
				aSlot = aSlot->next;
			}
			if (!aSlot)
				mxDebug0(the, XS_TYPE_ERROR, "parseInfoSet: no such root");				
			aRule->next = fxDuplicateSlot(the, aSlot->value.reference->next);
			aRule = aRule->next;
			aCount--;
			anArgument++;
		}
	}
	else {
		aSlot = grammars->next;
		while (aSlot) {
			aRule->next = fxDuplicateSlot(the, aSlot->value.reference->next);
			aRule = aRule->next;
			aSlot = aSlot->next;
		}
	}
	fxParseInfoSetChildren(the, aDocument, aGrammar, C_NULL);
	the->stack++;
}

void fxParseInfoSetAttribute(txMachine* the, txSlot* theAttribute, txSlot* theGrammar, txSlot* theInstance)
{
	txID aNamespaceID = XS_NO_ID;
	txSlot* aNamespace;
	txSlot* aSymbol;
	txID aNameID = XS_NO_ID;
	txSlot* aName;
	txSlot* aNode;
	txSlot* aProperty;
	txID anID;
	txSlot* aValue;

	aNamespace = fxGetOwnProperty(the, theAttribute, the->namespaceID);
	if (aNamespace) {
		aSymbol = fxFindSymbol(the, aNamespace->value.string);
		if (!aSymbol) {
			fxParseInfoSetError(the, "attribute namespace not found: %s", aNamespace->value.string);
			return;
		}
		aNamespaceID = aSymbol->ID;
	}
	aName = fxGetOwnProperty(the, theAttribute, the->nameID);
	if (aName) {
		aSymbol = fxFindSymbol(the, aName->value.string);
		if (!aSymbol) {
			fxParseInfoSetError(the, "attribute name not found: %s", aName->value.string);
			return;
		}
		aNameID = aSymbol->ID;
	}
	else {
		fxParseInfoSetError(the, "missing attribute name");
		return;
	}
	aNode = theGrammar->next;
	while (aNode) {
		if ((aNode->kind == XS_ATTRIBUTE_RULE) 
				&& (aNode->value.node.part.namespaceID == aNamespaceID) 
				&& (aNode->value.node.part.nameID == aNameID))
			break;
		aNode = aNode->next;
	}	
	if ((!aNode) && (aNamespaceID == XS_NO_ID)) {
		aNode = theGrammar->next;
		while (aNode) {
			if ((aNode->kind == XS_ATTRIBUTE_RULE)
					&& (aNode->value.node.part.nameID == aNameID)) 
				break;
			aNode = aNode->next;
		}
		if (aNode) {
			aSymbol = fxGetSymbol(the, aNode->value.node.part.namespaceID);
			fxParseInfoSetError(the, "attribute %s instead of {%s}:%s", aName->value.string, aSymbol->value.symbol.string, aName->value.string);
		}
	}
	if (!aNode) {
		if (aNamespace)
			fxParseInfoSetError(the, "attribute not found: {%s}:%s", aNamespace->value.string, aName->value.string);
		else
			fxParseInfoSetError(the, "attribute not found: %s", aName->value.string);
		return;
	}
	aProperty = fxParseInfoSetRule(the, theInstance, aNode, &anID);
	if (fxGetOwnProperty(the, aProperty, anID)) {
		if (aNamespace)
			fxParseInfoSetError(the, "redundant attribute: {%s}:%s", aNamespace->value.string, aName->value.string);
		else
			fxParseInfoSetError(the, "redundant attribute: %s", aName->value.string);
	}
	aProperty = fxSetProperty(the, aProperty, anID, C_NULL);
	aValue = fxGetOwnProperty(the, theAttribute, the->valueID);
	if (aValue)
		fxParseInfoSetValue(the, aValue->value.string, aNode, aProperty);
	else
		fxParseInfoSetError(the, "missing attribute value");
}

void fxParseInfoSetAttributes(txMachine* the, txSlot* theElement, txSlot* theGrammar, txSlot* theInstance)
{
	txSlot* aReference;
	txSlot* attributes;
	txSlot* anArray;
	txInteger aLength, anIndex;
	txSlot* anAttribute;
	
	aReference = fxGetOwnProperty(the, theElement, the->_attributesID);
	if (aReference) {
		attributes = fxGetInstance(the, aReference);
		if (attributes) {
			anArray = attributes->next;
			if (anArray && (anArray->kind == XS_ARRAY_KIND)) {
				aLength = anArray->value.array.length;
				for (anIndex = 0; anIndex < aLength; anIndex++) {
					aReference = anArray->value.array.address + anIndex;
					if (aReference->ID) {
						anAttribute = fxGetInstance(the, aReference);
						if (anAttribute)
							fxParseInfoSetAttribute(the, anAttribute, theGrammar, theInstance);
					}
				}
			}
		}
	}
}

void fxParseInfoSetCData(txMachine* the, txSlot* theData, txSlot* theGrammar, txSlot* theInstance)
{
	txSlot* aNode;
	txSlot* aProperty;
	txID anID;
	txSlot* aValue;

	aNode = theGrammar->next;
	while (aNode) {
		if (aNode->kind == XS_DATA_RULE)
			break;
		aNode = aNode->next;
	}
	if (!aNode)
		return;
	aProperty = fxParseInfoSetRule(the, theInstance, aNode, &anID);
	if (anID == XS_NO_ID)
		aProperty = fxSetProperty(the, aProperty, (txID)aProperty->next->value.integer, C_NULL);
	else if (anID & 0x8000)
		aProperty = fxSetProperty(the, aProperty, anID, C_NULL);
	else
		aProperty = mxResult;
	aValue = fxGetOwnProperty(the, theData, the->valueID);
	if (aValue)
		fxParseInfoSetValue(the, aValue->value.string, aNode, aProperty);
	else
		fxParseInfoSetError(the, "missing data value");
}

void fxParseInfoSetChildren(txMachine* the, txSlot* theElement, txSlot* theGrammar, txSlot* theInstance)
{
	txSlot* aSlot = fxGetInstance(the, mxThis);
	txSlot* element = fxGetProperty(the, aSlot, the->elementID);
	txSlot* cdata = fxGetProperty(the, aSlot, the->cdataID);
	txSlot* pi = fxGetProperty(the, aSlot, the->piID);
	txSlot* children;
	txSlot* anArray;
	txInteger aLength, anIndex;
	txSlot* aChild;
	
	aSlot = fxGetOwnProperty(the, theElement, the->childrenID);
	if (aSlot) {
		children = fxGetInstance(the, aSlot);
		if (children) {
			anArray = children->next;
			if (anArray && (anArray->kind == XS_ARRAY_KIND)) {
				aLength = anArray->value.array.length;
				for (anIndex = 0; anIndex < aLength; anIndex++) {
					aSlot = anArray->value.array.address + anIndex;
					if (aSlot->ID) {
						aChild = fxGetInstance(the, aSlot);
						if (aChild) {
							mxPush(*element);
							mxPush(*aSlot);
							if (fxIsInstanceOf(the)) {
								fxParseInfoSetElement(the, aChild, theGrammar, theInstance);
								continue;
							}
							mxPush(*cdata);
							mxPush(*aSlot);
							if (fxIsInstanceOf(the)) {
								fxParseInfoSetCData(the, aChild, theGrammar, theInstance);
								continue;
							}
							mxPush(*pi);
							mxPush(*aSlot);
							if (fxIsInstanceOf(the)) {
								fxParseInfoSetPI(the, aChild, theGrammar, theInstance);
								continue;
							}
						}
					}
				}
			}
		}
	}
}

void fxParseInfoSetElement(txMachine* the, txSlot* theElement, txSlot* theGrammar, txSlot* theInstance)
{
	txID aNamespaceID = XS_NO_ID;
	txSlot* aNamespace;
	txSlot* aSymbol;
	txID aNameID = XS_NO_ID;
	txSlot* aName;
	txSlot* aNode;
	txKind aKind;
	txSlot* aProperty;
	txID anID;

	aNamespace = fxGetOwnProperty(the, theElement, the->namespaceID);
	if (aNamespace) {
		aSymbol = fxFindSymbol(the, aNamespace->value.string);
		if (!aSymbol) {
			fxParseInfoSetError(the, "element namespace not found: %s", aNamespace->value.string);
			return;
		}
		aNamespaceID = aSymbol->ID;
	}
	aName = fxGetOwnProperty(the, theElement, the->nameID);
	if (aName) {
		aSymbol = fxFindSymbol(the, aName->value.string);
		if (!aSymbol) {
			fxParseInfoSetError(the, "element name not found: %s", aName->value.string);
			return;
		}
		aNameID = aSymbol->ID;
	}
	else {
		fxParseInfoSetError(the, "missing element name");
		return;
	}
	aNode = theGrammar->next;
	while (aNode) {
		if (((aNode->kind == XS_NODE_KIND) || (aNode->kind == XS_JUMP_RULE))
				&& (aNode->value.node.part.namespaceID == aNamespaceID) 
				&& (aNode->value.node.part.nameID == aNameID))
			break;
		aNode = aNode->next;
	}	
	if ((!aNode) && (aNamespaceID == XS_NO_ID)) {
		aNode = theGrammar->next;
		while (aNode) {
			if (((aNode->kind == XS_NODE_KIND) || (aNode->kind == XS_JUMP_RULE))
					&& (aNode->value.node.part.nameID == aNameID)) 
				break;
			aNode = aNode->next;
		}
		if (aNode) {
			aSymbol = fxGetSymbol(the, aNode->value.node.part.namespaceID);
			fxParseInfoSetError(the, "element %s instead of {%s}:%s", aName->value.string, aSymbol->value.symbol.string, aName->value.string);
		}
	}
	if (!aNode) {
		if (aNamespace)
			fxParseInfoSetError(the, "element not found: {%s}:%s", aNamespace->value.string, aName->value.string);
		else
			fxParseInfoSetError(the, "element not found: %s", aName->value.string);
		return;
	}
	
	if (aNode->kind == XS_NODE_KIND)
		theGrammar = aNode->value.node.link;
	else {
		aProperty = fxParseInfoSetRule(the, theInstance, aNode, &anID);
		if (anID == XS_NO_ID)
			aProperty = fxSetProperty(the, aProperty, (txID)aProperty->next->value.integer, C_NULL);
		else if (anID & 0x8000) {
			if (fxGetOwnProperty(the, aProperty, anID)) {
				if (aNamespace)
					fxParseInfoSetError(the, "redundant element: {%s}:%s", aNamespace->value.string, aName->value.string);
				else
					fxParseInfoSetError(the, "redundant element: %s", aName->value.string);
				return;
			}
			aProperty = fxSetProperty(the, aProperty, anID, C_NULL);
		}
		else {
			aProperty = mxResult;
		}
		
		mxZeroSlot(--the->stack);
		the->stack->value.alias = aNode->value.rule.data->alias;
		the->stack->kind = XS_ALIAS_KIND;
		fxNewInstanceOf(the);
		if ((the->parseFlag & 64) == 0) {
			mxZeroSlot(--the->stack);
			the->stack->value.reference = theElement;
			the->stack->kind = XS_REFERENCE_KIND;
			fxQueueID(the, the->__xs__infosetID, XS_DONT_ENUM_FLAG);
		}
		aProperty->value = the->stack->value;
		aProperty->kind = the->stack->kind;
		the->stack++;
		
		theInstance = aProperty->value.reference;
		theGrammar = aNode->value.rule.data->link;
	}
	
	aNode = theGrammar->next;
	while (aNode) {
		aKind = aNode->kind;
		if ((aKind == XS_EMBED_RULE) || (aKind == XS_REPEAT_RULE)) {
			aProperty = fxParseInfoSetRule(the, theInstance, aNode, &anID);
			aProperty = fxSetProperty(the, aProperty, anID, C_NULL);
			mxZeroSlot(--the->stack);
			if (aKind == XS_EMBED_RULE)
				the->stack->value.alias = aNode->value.rule.data->alias;
			else
				the->stack->value.alias = mxArrayPrototype.value.alias;
			the->stack->kind = XS_ALIAS_KIND;
			fxNewInstanceOf(the);
			aProperty->value = the->stack->value;
			aProperty->kind = the->stack->kind;
			the->stack++;
		}
		aNode = aNode->next;
	}

	mxVarv(0)->value.reference = theElement;
	mxVarv(0)->kind = XS_REFERENCE_KIND;
	fxParseInfoSetAttributes(the, theElement, theGrammar, theInstance);
	fxParseInfoSetChildren(the, theElement, theGrammar, theInstance);
}

void fxParseInfoSetError(txMachine* the, char* theFormat, ...)
{
#define XS_NO_MASK (XS_NO_ERROR_FLAG | XS_NO_WARNING_FLAG)
	char* aPath;
	int aLine;
	c_va_list arguments;

	if ((the->parseFlag & XS_NO_MASK) == XS_NO_MASK)
		return;
	aPath = C_NULL;
	aLine = 0;
	c_va_start(arguments, theFormat);
	if ((the->parseFlag & XS_NO_MASK) == XS_NO_ERROR_FLAG)
		fxVReportWarning(the, aPath, aLine, theFormat, arguments);
	else
		fxVReportError(the, aPath, aLine, theFormat, arguments);
	c_va_end(arguments);
	if ((the->parseFlag & XS_NO_MASK) == 0)
		mxDebug0(the, XS_SYNTAX_ERROR, "XML error");
}

void fxParseInfoSetPI(txMachine* the, txSlot* thePI, txSlot* theGrammar, txSlot* theInstance)
{
	txID aNamespaceID = XS_NO_ID;
	txSlot* aNamespace;
	txSlot* aSymbol;
	txID aNameID = XS_NO_ID;
	txSlot* aName;
	txSlot* aNode;
	txSlot* aProperty;
	txID anID;
	txSlot* aValue;

	aNamespace = fxGetOwnProperty(the, thePI, the->namespaceID);
	if (aNamespace) {
		aSymbol = fxFindSymbol(the, aNamespace->value.string);
		if (!aSymbol) {
			fxParseInfoSetError(the, "pi namespace not found: %s", aNamespace->value.string);
			return;
		}
		aNamespaceID = aSymbol->ID;
	}
	aName = fxGetOwnProperty(the, thePI, the->nameID);
	if (aName) {
		if (c_strcmp(aName->value.string, "xml") == 0)
			return;
		aSymbol = fxFindSymbol(the, aName->value.string);
		if (!aSymbol) {
			fxParseInfoSetError(the, "pi name not found: %s", aName->value.string);
			return;
		}
		aNameID = aSymbol->ID;
	}
	else {
		fxParseInfoSetError(the, "missing pi name");
		return;
	}
	aNode = theGrammar->next;
	while (aNode) {
		if ((aNode->kind == XS_PI_RULE) 
				&& (aNode->value.node.part.namespaceID == aNamespaceID) 
				&& (aNode->value.node.part.nameID == aNameID))
			break;
		aNode = aNode->next;
	}	
	if ((!aNode) && (aNamespaceID == XS_NO_ID)) {
		aNode = theGrammar->next;
		while (aNode) {
			if ((aNode->kind == XS_PI_RULE)
					&& (aNode->value.node.part.nameID == aNameID)) 
				break;
			aNode = aNode->next;
		}
		if (aNode) {
			aSymbol = fxGetSymbol(the, aNode->value.node.part.namespaceID);
			fxParseInfoSetError(the, "pi %s instead of {%s}:%s", aName->value.string, aSymbol->value.symbol.string, aName->value.string);
		}
	}
	if (!aNode) {
		if (aNamespace)
			fxParseInfoSetError(the, "pi not found: {%s}:%s", aNamespace->value.string, aName->value.string);
		else
			fxParseInfoSetError(the, "pi not found: %s", aName->value.string);
		return;
	}
	aProperty = fxParseInfoSetRule(the, theInstance, aNode, &anID);
	if (fxGetOwnProperty(the, aProperty, anID)) {
		if (aNamespace)
			fxParseInfoSetError(the, "redundant pi: {%s}:%s", aNamespace->value.string, aName->value.string);
		else
			fxParseInfoSetError(the, "redundant pi: %s", aName->value.string);
	}
	aProperty = fxSetProperty(the, aProperty, anID, C_NULL);
	aValue = fxGetOwnProperty(the, thePI, the->valueID);
	if (aValue)
		fxParseInfoSetValue(the, aValue->value.string, aNode, aProperty);
	else
		fxParseInfoSetError(the, "missing pi value");
}

txSlot* fxParseInfoSetRule(txMachine* the, txSlot* theInstance, txSlot* theRule, txID* theID)
{
	txSlot* aProperty = theInstance;
	txInteger aCount = theRule->value.rule.data->count;
	txID* anID = theRule->value.rule.data->IDs;
	while (aCount > 1) {
		aProperty = fxGetProperty(the, aProperty, *anID);
		aProperty = aProperty->value.reference;
		aCount--;
		anID++;
	}	
	*theID = *anID;
	return aProperty;
}

void fxParseInfoSetValue(txMachine* the, txString theValue, txSlot* theNode, txSlot* theProperty)
{
	mxZeroSlot(--the->stack);
	the->stack->value.string = theValue;
	the->stack->kind = XS_STRING_KIND;
	*(--the->stack) = *mxVarv(0);
	mxZeroSlot(--the->stack); /* path */
	mxZeroSlot(--the->stack); /* line */
	mxZeroSlot(--the->stack);
	the->stack->value.integer = 4;
	the->stack->kind = XS_INTEGER_KIND;
	mxZeroSlot(--the->stack);
	the->stack->value.alias = theNode->value.rule.data->alias;
	the->stack->kind = XS_ALIAS_KIND;
	fxCallID(the, the->parseID);
	theProperty->value = the->stack->value;
	theProperty->kind = the->stack->kind;
	the->stack++;
}

txID fxSearchInfoSetNamespace(txMachine* the, char* thePrefix, int theLength)
{
	txSlot* aReference;
	txSlot* anElement;
	txSlot* attributes;
	txSlot* anArray;
	txInteger aLength, anIndex;
	txSlot* anAttribute;
	txSlot* aName;
	txSlot* aValue;
	txSlot* aSymbol;

	aReference = mxArgv(0);
	while (aReference) {
		anElement = fxGetInstance(the, aReference);
		if (anElement) {
			aReference = fxGetOwnProperty(the, anElement, the->xmlnsAttributesID);
			if (aReference) {
				attributes = fxGetInstance(the, aReference);
				if (attributes) {
					anArray = attributes->next;
					if (anArray && (anArray->kind == XS_ARRAY_KIND)) {
						aLength = anArray->value.array.length;
						for (anIndex = 0; anIndex < aLength; anIndex++) {
							aReference = anArray->value.array.address + anIndex;
							if (aReference->ID) {
								anAttribute = fxGetInstance(the, aReference);
								if (anAttribute) {
									aName = fxGetOwnProperty(the, anAttribute, the->nameID);
									aValue = fxGetOwnProperty(the, anAttribute, the->valueID);
									if (aName && aValue) {
										if (c_strncmp(aName->value.string, thePrefix, theLength) == 0) {
											aSymbol = fxFindSymbol(the, aValue->value.string);
											if (aSymbol)
												return aSymbol->ID;
										}
									}
								}
							}
						}
					}
				}
			}
			aReference = fxGetOwnProperty(the, anElement, the->parentID);
		}
		else
			break;
	}
	if (thePrefix)
		fxParseInfoSetError(the, "prefix not found: %s", thePrefix);
	return XS_NO_ID;
}

void fx_xs_infoSet_print(txMachine* the)
{
	txSlot* aDocument;
	txFlag aFlag;
	txStringStream aStream;
	
	if (mxArgc < 1)
		mxDebug0(the, XS_SYNTAX_ERROR, "xs.infoset.print: no document");
	aDocument = fxGetInstance(the, mxArgv(0));
	if (!aDocument)
		mxDebug0(the, XS_SYNTAX_ERROR, "xs.infoset.print: document is no instance");
	if ((mxArgc > 1) && (mxArgv(1)->kind == XS_INTEGER_KIND))
		aFlag = (txFlag)(mxArgv(1)->value.integer);
	else
		aFlag = 0;
	aStream.slot = C_NULL;
	aStream.offset = 0;
	aStream.size = 0;
	the->parseFlag = aFlag | XS_INFO_SET_FLAG;
	fxPrintInfoSetChildren(the, &aStream, aDocument, 0);
	aStream.size = aStream.offset;
	mxResult->value.string = (txString)fxNewChunk(the, aStream.size + 1);
	mxResult->kind = XS_STRING_KIND;
	aStream.slot = mxResult;
	aStream.offset = 0;
	the->parseFlag = aFlag | XS_INFO_SET_FLAG;
	fxPrintInfoSetChildren(the, &aStream, aDocument, 0);
	*(aStream.slot->value.string + aStream.offset) = 0;
}

void fxPrintInfoSetAttribute(txMachine* the, txStringStream* theStream, txSlot* theAttribute)
{
	txSlot* aSlot;
	
	fxPrintInfoSetCharacters(the, theStream, " ");
	aSlot = fxGetProperty(the, theAttribute, the->prefixID);
	if (mxTest(aSlot)) {
		fxPrintInfoSetCharacters(the, theStream, aSlot->value.string);
		fxPrintInfoSetCharacters(the, theStream, ":");
	}
	aSlot = fxGetProperty(the, theAttribute, the->nameID);
	fxPrintInfoSetCharacters(the, theStream, aSlot->value.string);
	fxPrintInfoSetCharacters(the, theStream, "=\"");
	aSlot = fxGetProperty(the, theAttribute, the->valueID);
	fxPrintInfoSetValue(the, theStream, aSlot, 1);
	fxPrintInfoSetCharacters(the, theStream, "\"");
}

void fxPrintInfoSetAttributes(txMachine* the, txStringStream* theStream, txSlot* theElement, txID theID)
{
	txSlot* aReference;
	txSlot* aSlot;
	txSlot* attributes;
	txSlot* anArray;
	txInteger aLength, anIndex;
	txSlot* anAttribute;
	
	aReference = fxGetProperty(the, theElement, theID);
	if (mxTest(aReference)) {
		attributes = fxGetInstance(the, aReference);
		if (attributes) {
			anArray = attributes->next;
			if (anArray && (anArray->kind == XS_ARRAY_KIND)) {
				aLength = anArray->value.array.length;
				if (aLength) {
					if (!theStream->slot) {
						aSlot = fxGetInstance(the, mxThis);
						aSlot = fxGetProperty(the, aSlot, the->compareAttributesID);
						mxZeroSlot(--the->stack);
						the->stack->value = aSlot->value;
						the->stack->kind = aSlot->kind;
						mxZeroSlot(--the->stack);
						the->stack->value.integer = 1;
						the->stack->kind = XS_INTEGER_KIND;
						mxZeroSlot(--the->stack);
						the->stack->value = aReference->value;
						the->stack->kind = aReference->kind;
						fxCallID(the, the->sortID);
						the->stack++;
					}
					for (anIndex = 0; anIndex < aLength; anIndex++) {
						aReference = anArray->value.array.address + anIndex;
						if (aReference->ID) {
							anAttribute = fxGetInstance(the, aReference);
							if (anAttribute)
								fxPrintInfoSetAttribute(the, theStream, anAttribute);
						}
					}
				}
			}
		}
	}
}

void fxPrintInfoSetCData(txMachine* the, txStringStream* theStream, txSlot* theCData, txInteger theLevel)
{
	txSlot* aSlot;
	
	aSlot = fxGetProperty(the, theCData, the->valueID);
	fxPrintInfoSetValue(the, theStream, aSlot, 2);
}

void fxPrintInfoSetCharacters(txMachine* the, txStringStream* theStream, char* theCharacters)
{
	txInteger aLength = c_strlen(theCharacters);
	if (theStream->slot && (theStream->offset + aLength <= theStream->size))
		c_memcpy(theStream->slot->value.string + theStream->offset, theCharacters, aLength);
	theStream->offset += aLength;
}

void fxPrintInfoSetChildren(txMachine* the, txStringStream* theStream, txSlot* theElement, txInteger theLevel)
{
	txSlot* aSlot = fxGetInstance(the, mxThis);
	txSlot* element = fxGetProperty(the, aSlot, the->elementID);
	txSlot* cdata = fxGetProperty(the, aSlot, the->cdataID);
	txSlot* comment = fxGetProperty(the, aSlot, the->commentID);
	txSlot* pi = fxGetProperty(the, aSlot, the->piID);
	txSlot* children;
	txSlot* anArray;
	txInteger aLength, anIndex;
	txSlot* aChild;
	
	aSlot = fxGetProperty(the, theElement, the->childrenID);
	if (mxTest(aSlot)) {
		children = fxGetInstance(the, aSlot);
		if (children) {
			anArray = children->next;
			if (anArray && (anArray->kind == XS_ARRAY_KIND)) {
				aLength = anArray->value.array.length;
				for (anIndex = 0; anIndex < aLength; anIndex++) {
					aSlot = anArray->value.array.address + anIndex;
					if (aSlot->ID) {
						aChild = fxGetInstance(the, aSlot);
						if (aChild) {
							mxPush(*element);
							mxPush(*aSlot);
							if (fxIsInstanceOf(the)) {
								fxPrintInfoSetElement(the, theStream, fxGetInstance(the, aSlot), theLevel);
								continue;
							}
							mxPush(*cdata);
							mxPush(*aSlot);
							if (fxIsInstanceOf(the)) {
								fxPrintInfoSetCData(the, theStream, fxGetInstance(the, aSlot), theLevel);
								continue;
							}
							mxPush(*pi);
							mxPush(*aSlot);
							if (fxIsInstanceOf(the)) {
								fxPrintInfoSetPI(the, theStream, fxGetInstance(the, aSlot), theLevel);
								continue;
							}
							if ((the->parseFlag & 1) == 0) {
								mxPush(*comment);
								mxPush(*aSlot);
								if (fxIsInstanceOf(the)) {
									fxPrintInfoSetComment(the, theStream, fxGetInstance(the, aSlot), theLevel);
									continue;
								}
							}
						}
					}
				}
			}
		}
	}
}

void fxPrintInfoSetComment(txMachine* the, txStringStream* theStream, txSlot* theComment, txInteger theLevel)
{
	txSlot* aSlot;
	
	if ((theLevel == 0) && ((the->parseFlag & XS_INFO_SET_FLAG) == 0))
		fxPrintInfoSetCharacters(the, theStream, "\n");
	fxPrintInfoSetCharacters(the, theStream, "<!--");
	aSlot = fxGetProperty(the, theComment, the->valueID);
	fxPrintInfoSetValue(the, theStream, aSlot, 0);
	fxPrintInfoSetCharacters(the, theStream, "-->");
	if ((theLevel == 0) && ((the->parseFlag & XS_INFO_SET_FLAG) != 0))
		fxPrintInfoSetCharacters(the, theStream, "\n");
}

void fxPrintInfoSetElement(txMachine* the, txStringStream* theStream, txSlot* theElement, txInteger theLevel)
{
	txSlot* aSlot;
	
	if (theLevel == 0)
		the->parseFlag &= ~XS_INFO_SET_FLAG;
	fxPrintInfoSetCharacters(the, theStream, "<");
	aSlot = fxGetProperty(the, theElement, the->prefixID);
	if (mxTest(aSlot)) {
		fxPrintInfoSetCharacters(the, theStream, aSlot->value.string);
		fxPrintInfoSetCharacters(the, theStream, ":");
	}
	aSlot = fxGetProperty(the, theElement, the->nameID);
	fxPrintInfoSetCharacters(the, theStream, aSlot->value.string);
	fxPrintInfoSetAttributes(the, theStream, theElement, the->xmlnsAttributesID);
	fxPrintInfoSetAttributes(the, theStream, theElement, the->_attributesID);
	fxPrintInfoSetCharacters(the, theStream, ">");
	fxPrintInfoSetChildren(the, theStream, theElement, theLevel + 1);
	fxPrintInfoSetCharacters(the, theStream, "</");
	aSlot = fxGetProperty(the, theElement, the->prefixID);
	if (mxTest(aSlot)) {
		fxPrintInfoSetCharacters(the, theStream, aSlot->value.string);
		fxPrintInfoSetCharacters(the, theStream, ":");
	}
	aSlot = fxGetProperty(the, theElement, the->nameID);
	fxPrintInfoSetCharacters(the, theStream, aSlot->value.string);
	fxPrintInfoSetCharacters(the, theStream, ">");
}

void fxPrintInfoSetPI(txMachine* the, txStringStream* theStream, txSlot* thePI, txInteger theLevel)
{
	txSlot* aSlot;
	
	if ((theLevel == 0) && ((the->parseFlag & XS_INFO_SET_FLAG) == 0))
		fxPrintInfoSetCharacters(the, theStream, "\n");
	fxPrintInfoSetCharacters(the, theStream, "<?");
	aSlot = fxGetProperty(the, thePI, the->prefixID);
	if (mxTest(aSlot)) {
		fxPrintInfoSetCharacters(the, theStream, aSlot->value.string);
		fxPrintInfoSetCharacters(the, theStream, ":");
	}
	aSlot = fxGetProperty(the, thePI, the->nameID);
	fxPrintInfoSetCharacters(the, theStream, aSlot->value.string);
	aSlot = fxGetProperty(the, thePI, the->valueID);
	if (mxTest(aSlot)) {
		fxPrintInfoSetCharacters(the, theStream, " ");
		fxPrintInfoSetValue(the, theStream, aSlot, 0);
	}
	fxPrintInfoSetCharacters(the, theStream, "?>");
	if ((theLevel == 0) && ((the->parseFlag & XS_INFO_SET_FLAG) != 0))
		fxPrintInfoSetCharacters(the, theStream, "\n");
}

void fxPrintInfoSetValue(txMachine* the, txStringStream* theStream, txSlot* theSlot, txFlag theFlag)
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

	if (theSlot->kind != XS_STRING_KIND)
		return;
	aText = (unsigned char*)theSlot->value.string;
	aStop = aStart = aText;
	while ((aChar = *aText++)) {
		if (sEscape[aChar] & theFlag) {
			if (aStop > aStart) {
				*aStop = '\0';
				fxPrintInfoSetCharacters(the, theStream, (char*)aStart);
				*aStop = aChar;
			}
			switch (aChar) {
			case '"':
				fxPrintInfoSetCharacters(the, theStream, "&quot;");
				break;
			case '&':
				fxPrintInfoSetCharacters(the, theStream, "&amp;");
				break;
			case '<':
				fxPrintInfoSetCharacters(the, theStream, "&lt;");
				break;
			case '>':
				fxPrintInfoSetCharacters(the, theStream, "&gt;");
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
				fxPrintInfoSetCharacters(the, theStream, (char*)aBuffer);
				break;
			}
			aStart = ++aStop;
		}
		else
			aStop++;
	}
	if (aStop > aStart)
		fxPrintInfoSetCharacters(the, theStream, (char*)aStart);
}

static txMarkupCallbacks gxScanInfoSetCallbacks = {
	fxScanInfoSetStartTag,
	fxScanInfoSetStopTag,
	fxScanInfoSetText,
	C_NULL,
	fxScanInfoSetComment,
	fxScanInfoSetPI,
	C_NULL
};

void fx_xs_infoSet_scan(txMachine* the)
{
	txStringStream aStream;
	txStringCStream cStream;
	txSlot* aSlot;
	txSlot* aFile = C_NULL;
	long aLine = 0;
	txBoolean isChunk;

	if (mxArgc < 1)
		mxDebug0(the, XS_SYNTAX_ERROR, "xs.infoset.scan: no buffer");
	
	aSlot = fxGetInstance(the, mxArgv(0));
	isChunk = mxIsChunk(aSlot);
	if (isChunk) {
		aSlot = aSlot->next;
		cStream.buffer = aSlot->value.host.data;
		cStream.size = aSlot->next->value.integer;
		cStream.offset = 0;
	}
	else {
		fxToString(the, mxArgv(0));
		aStream.slot = mxArgv(0);
		aStream.size = c_strlen(aStream.slot->value.string);
		aStream.offset = 0;
	}
	if (mxArgc >= 3) {
		aFile = fxNewFile(the, mxArgv(1));
		aLine = mxArgv(2)->value.integer;
	}
	
	aSlot = fxGetInstance(the, mxThis);
	aSlot = fxGetProperty(the, aSlot, fxID(the, "document"));
	mxPush(*aSlot);
	fxNewInstanceOf(the);
	mxPushStringC("UTF-8");
	fxQueueID(the, fxID(the, "encoding"), XS_NO_FLAG);
	mxPushStringC("1.0");
	fxQueueID(the, fxID(the, "version"), XS_NO_FLAG);
	*mxResult = *the->stack++;
	{
		mxTry(the) {
			the->parseFlag = XS_INFO_SET_FLAG;
			if (aFile)
				the->parseFlag |= XS_DEBUG_FLAG;
			/* mxArgv(0) = tag, text, and so on */
			fxNewInstance(the);
			mxZeroSlot(--the->stack);
			fxQueueID(the, the->parentID, XS_GET_ONLY);
			mxZeroSlot(--the->stack);
			fxQueueID(the, the->nameID, XS_GET_ONLY);
			mxZeroSlot(--the->stack);
			fxQueueID(the, the->valueID, XS_GET_ONLY);
			/* mxArgv(1) = path */
			mxZeroSlot(--the->stack);
			/* mxArgv(2) = line */
			mxZeroSlot(--the->stack);
			/* ARGC */
			mxZeroSlot(--the->stack);
			the->stack->value.integer = 3;
			the->stack->kind = XS_INTEGER_KIND;
			/* THIS */
			*(--the->stack) = *mxThis;
			/* FUNCTION */
			mxZeroSlot(--the->stack);
			/* RESULT */
			*(--the->stack) = *mxResult;
			/* FRAME */
			mxZeroSlot(--the->stack);
			the->stack->next = the->frame;
			the->stack->ID = XS_NO_ID;
			the->stack->flag = XS_C_FLAG;
			the->stack->kind = XS_FRAME_KIND;
			the->stack->value.frame.scope = the->scope;
			the->frame = the->stack;
			/* VARC */
			mxZeroSlot(--the->stack);
			the->stack->kind = XS_INTEGER_KIND;
			the->stack->value.integer = 1;
			mxZeroSlot(--the->stack);
			
			if (isChunk)
				fxParseMarkup(the, &cStream, fxStringCGetter, aFile, aLine, &gxScanInfoSetCallbacks);
			else
				fxParseMarkup(the, &aStream, fxStringGetter, aFile, aLine, &gxScanInfoSetCallbacks);
			
			the->stack = the->frame + 5 + mxArgc;
			the->scope = the->frame->value.frame.scope;
			*(--the->stack) = *mxResult;
			the->frame = the->frame->next;
			
			fxScanInfoSetArrange(the, mxResult, the->childrenID);
		}
		mxCatch(the) {
			mxZeroSlot(--the->stack);
		}
	}
	*mxResult = *the->stack++;
}

void fxScanInfoSetAdd(txMachine* the, txSlot* theParent, txID theID, txSlot* theChild)
{
	txSlot* aLink;
	txSlot* children = fxGetOwnProperty(the, theParent->value.reference, theID);
	if (!children) {
		children = fxSetProperty(the, theParent->value.reference, theID, C_NULL);
		mxPush(mxArrayPrototype);
		fxNewArrayInstance(the);
		children->value = the->stack->value;
		children->kind = the->stack->kind;
		the->stack++;
	}
	aLink = fxQueueItem(the, children->value.reference);
	aLink->ID = (txID)children->value.reference->next->value.array.length;
	aLink->value = theChild->value;
	aLink->kind = theChild->kind;
}	
	
void fxScanInfoSetArrange(txMachine* the, txSlot* theParent, txID theID)
{
	txSlot* children = fxGetOwnProperty(the, theParent->value.reference, theID);
	if (children)
		fxCacheArray(the, children->value.reference);
}

void fxScanInfoSetAttributes(txMachine* the, txSlot* theSlot)
{
	txSlot* aParent = mxResult;
	txSlot* aSlot = fxGetInstance(the, mxThis);
	txSlot* aPrototype = fxGetProperty(the, aSlot, the->attributeID);
	txSlot* aName;

	aSlot = fxGetOwnProperty(the, theSlot->value.reference, the->valueID);
	if (mxIsReference(aSlot)) {
		aSlot = aSlot->value.reference->next;
		while (aSlot) {
			aName = fxGetOwnProperty(the, aSlot->value.reference, the->nameID);
			if (c_strncmp(aName->value.string, "xmlns", 5) != 0) {
				mxPush(*aPrototype);
				fxNewInstanceOf(the);
				mxPush(*aParent);
				fxQueueID(the, the->parentID, XS_NO_FLAG);
				if (c_strchr(aName->value.string, ':'))
					fxScanInfoSetName(the, aSlot);
				else {
					mxPush(*aName);
					fxQueueID(the, the->nameID, XS_NO_FLAG);
				}
				fxScanInfoSetValue(the, aSlot);
				fxScanInfoSetAdd(the, aParent, the->_attributesID, the->stack);
				the->stack++;
			}
			aSlot = aSlot->next;
		}
	}
}

void fxScanInfoSetChild(txMachine* the, txID theID)
{
	txSlot* aParent = mxResult;
	txSlot* aSlot = fxGetInstance(the, mxThis);
	aSlot = fxGetProperty(the, aSlot, theID);
	mxPush(*aSlot);
	fxNewInstanceOf(the);
	mxPush(*aParent);
	fxQueueID(the, the->parentID, XS_NO_FLAG);
	if (the->parseFlag & XS_DEBUG_FLAG) {
		*(--the->stack) = *mxArgv(1);
		fxQueueID(the, the->pathID, XS_DONT_ENUM_FLAG);
		*(--the->stack) = *mxArgv(2);
		fxQueueID(the, the->lineID, XS_DONT_ENUM_FLAG);
	}
	fxScanInfoSetAdd(the, aParent, the->childrenID, the->stack);
}

void fxScanInfoSetComment(txMachine* the)
{
	fxScanInfoSetChild(the, the->commentID);
	fxScanInfoSetValue(the, mxArgv(0));
	the->stack++;
}

void fxScanInfoSetName(txMachine* the, txSlot* theSlot)
{
	txSlot* aName;
	txString aColon;
	txInteger aLength;
	txSlot* anAttribute;
	txSlot* aValue;

	aName = fxGetOwnProperty(the, theSlot->value.reference, the->nameID);
	aColon = c_strchr(aName->value.string, ':');
	if (aColon) {
		aLength = c_strlen(aColon + 1);
		mxZeroSlot(--the->stack);
		the->stack->value.string = (txString)fxNewChunk(the, aLength + 1);
		aColon = c_strchr(aName->value.string, ':'); // gc!
		c_memcpy(the->stack->value.string, aColon + 1, aLength);
		the->stack->value.string[aLength] = 0;
		the->stack->kind = XS_STRING_KIND;
		fxQueueID(the, the->nameID, XS_NO_FLAG);
		
		aLength = aColon - aName->value.string;
		anAttribute = fxScanInfoSetNamespace(the, aName->value.string, aLength);
		if (anAttribute) {
			aValue = fxGetOwnProperty(the, anAttribute, the->valueID);
			mxPush(*aValue);
			fxQueueID(the, the->namespaceID, XS_NO_FLAG);
			aValue = fxGetOwnProperty(the, anAttribute, the->nameID);
			mxPush(*aValue);
			fxQueueID(the, the->prefixID, XS_NO_FLAG);
		}
		else {
			mxZeroSlot(--the->stack);
			the->stack->value.string = (txString)fxNewChunk(the, aLength + 1);
			c_memcpy(the->stack->value.string, aName->value.string, aLength);
			the->stack->value.string[aLength] = 0;
			the->stack->kind = XS_STRING_KIND;
			fxQueueID(the, the->prefixID, XS_NO_FLAG);
		}
	}
	else {
		mxPush(*aName);
		fxQueueID(the, the->nameID, XS_NO_FLAG);
		anAttribute = fxScanInfoSetNamespace(the, "xmlns", 5);
		if (anAttribute) {
			aValue = fxGetOwnProperty(the, anAttribute, the->valueID);
			mxPush(*aValue);
			fxQueueID(the, the->namespaceID, XS_NO_FLAG);
		}
	}
}

txSlot* fxScanInfoSetNamespace(txMachine* the, char* thePrefix, int theLength)
{
	txSlot* aReference;
	txSlot* anElement;
	txSlot* anArray;
	txSlot* aLink;
	txSlot* anAttribute;
	txSlot* aName;

	aReference = mxResult;
	while (aReference) {
		anElement = fxGetInstance(the, aReference);
		if (anElement) {
			aReference = fxGetOwnProperty(the, anElement, the->xmlnsAttributesID);
			if (aReference) {
				anArray = fxGetInstance(the, aReference);
				if (anArray) {
					aLink = anArray->next->next;
					while (aLink) {
						anAttribute = fxGetInstance(the, aLink);
						if (anAttribute) {
							aName = fxGetOwnProperty(the, anAttribute, the->nameID);
							if (((int)c_strlen(aName->value.string) == theLength) 
								&& (c_strncmp(aName->value.string,thePrefix,theLength) == 0))
								return anAttribute;
						}
						else
							break;
						aLink = aLink->next;	
					}
				}
			}
			aReference = fxGetOwnProperty(the, anElement, the->parentID);
		}
		else
			break;
	}
	return C_NULL;
}

void fxScanInfoSetNamespaces(txMachine* the, txSlot* theSlot)
{
	txSlot* aParent = mxResult;
	txSlot* aSlot = fxGetInstance(the, mxThis);
	txSlot* aPrototype = fxGetProperty(the, aSlot, the->attributeID);
	txSlot* xmlnsPrefix = fxGetProperty(the, aSlot, the->xmlnsPrefixID);
	txSlot* xmlnsNamespace = fxGetProperty(the, aSlot, the->xmlnsNamespaceID);
	txSlot* aName;
	txInteger aLength;
	
	aSlot = fxGetOwnProperty(the, theSlot->value.reference, the->valueID);
	if (mxIsReference(aSlot)) {
		aSlot = aSlot->value.reference->next;
		while (aSlot) {
			aName = fxGetOwnProperty(the, aSlot->value.reference, the->nameID);
			if (c_strncmp(aName->value.string, "xmlns", 5) == 0) {
				mxPush(*aPrototype);
				fxNewInstanceOf(the);
				mxPush(*aParent);
				fxQueueID(the, the->parentID, XS_NO_FLAG);
				if (aName->value.string[5] == ':') {
					aLength = c_strlen(aName->value.string) - 6;
					mxZeroSlot(--the->stack);
					the->stack->value.string = (txString)fxNewChunk(the, aLength + 1);
					c_memcpy(the->stack->value.string, aName->value.string + 6, aLength);
					the->stack->value.string[aLength] = 0;
					the->stack->kind = XS_STRING_KIND;
					fxQueueID(the, the->nameID, XS_NO_FLAG);
					mxPush(*xmlnsNamespace);
					fxQueueID(the, the->namespaceID, XS_NO_FLAG);
					mxPush(*xmlnsPrefix);
					fxQueueID(the, the->prefixID, XS_NO_FLAG);
				}
				else {
					mxPush(*xmlnsPrefix);
					fxQueueID(the, the->nameID, XS_NO_FLAG);
					mxPush(*xmlnsNamespace);
					fxQueueID(the, the->namespaceID, XS_NO_FLAG);
				}
				fxScanInfoSetValue(the, aSlot);
				fxScanInfoSetAdd(the, aParent, the->xmlnsAttributesID, the->stack);
				the->stack++;
			}
			aSlot = aSlot->next;
		}
	}
}

void fxScanInfoSetPI(txMachine* the)
{
	txSlot* aName = fxGetOwnProperty(the, mxArgv(0)->value.reference, the->nameID);
	if (c_strcmp(aName->value.string, "xml") == 0)
		return;
	fxScanInfoSetChild(the, the->piID);
	fxScanInfoSetName(the, mxArgv(0));
	fxScanInfoSetValue(the, mxArgv(0));
	the->stack++;
}

void fxScanInfoSetStartTag(txMachine* the)
{
	fxScanInfoSetChild(the, the->elementID);
	if (the->parseFlag & XS_INFO_SET_FLAG) {
		txSlot* aDocument = mxResult;
		txSlot* anElement = the->stack;
		mxPush(*aDocument);
		mxPush(*anElement);
		fxQueueID(the, the->elementID, XS_NO_FLAG);
		the->stack++;
		the->parseFlag &= ~XS_INFO_SET_FLAG;
	}
	*mxResult = *the->stack;
	fxScanInfoSetNamespaces(the, mxArgv(0));
	fxScanInfoSetName(the, mxArgv(0));
	fxScanInfoSetAttributes(the, mxArgv(0));
	the->stack++;
}

void fxScanInfoSetStopTag(txMachine* the)
{
	txSlot* aParent = mxResult;
	fxScanInfoSetArrange(the, aParent, the->xmlnsAttributesID);
	fxScanInfoSetArrange(the, aParent, the->_attributesID);
	fxScanInfoSetArrange(the, aParent, the->childrenID);
	aParent = fxGetProperty(the, aParent->value.reference, the->parentID);
	*mxResult = *aParent;
}

void fxScanInfoSetText(txMachine* the)
{
	fxScanInfoSetChild(the, the->cdataID);
	fxScanInfoSetValue(the, mxArgv(0));
	the->stack++;
}

void fxScanInfoSetValue(txMachine* the, txSlot* theSlot)
{
	txSlot* aValue = fxGetOwnProperty(the, theSlot->value.reference, the->valueID);
	mxPush(*aValue);
	fxQueueID(the, the->valueID, XS_NO_FLAG);
}

void fx_xs_infoSet_serialize(txMachine* the)
{
	txFlag aFlag;
	txSlot* aSlot;
	txSlot* grammars;
	txSlot* anInstance;
	txSlot* aRoot;
	txSlot* aNode;
	txSlot* aDocument;

	if (mxArgc < 1)
		mxDebug0(the, XS_SYNTAX_ERROR, "xs.infoset.serialize: no root");
	if ((mxArgc > 1) && (mxArgv(1)->kind == XS_INTEGER_KIND))
		aFlag = (txFlag)(mxArgv(1)->value.integer);
	else
		aFlag = 0;
	the->parseFlag = aFlag;

	aSlot = fxGetProperty(the, mxGlobal.value.reference, the->grammarsID);
	grammars = fxGetInstance(the, aSlot);
	if (!(grammars->next))
		mxDebug0(the, XS_SYNTAX_ERROR, "xs.infoset.serialze: no roots");

	fxToInstance(the, mxArgv(0));
	anInstance = fxGetInstance(the, mxArgv(0));
	aSlot = grammars->next;
	while (aSlot) {
		if (anInstance->ID == aSlot->value.reference->ID)
			break;
		aSlot = aSlot->next;
	}
	if (!aSlot)
		mxDebug0(the, XS_TYPE_ERROR, "xs.infoset.serialize: no such root");

	aRoot = aSlot->value.reference;
	aNode = aRoot->next;
	
	fxSerializeInfoSetDocument(the);
	aDocument = the->stack;
	switch (aNode->kind) {
	case XS_NODE_KIND:
		fxSerializeInfoSetElement(the, aRoot, aNode, C_NULL);
		fxSerializeInfoSetNode(the, aRoot, aNode->value.node.link->next, mxArgv(0));
		break;
	case XS_DATA_RULE:
		mxDebug0(the, XS_TYPE_ERROR, "xs.infoset.serialize: data root");
		break;
	case XS_PI_RULE:
		mxDebug0(the, XS_TYPE_ERROR, "xs.infoset.serialize: pi root");
		break;
	case XS_JUMP_RULE:
		fxSerializeInfoSetElement(the, aRoot, aNode, mxArgv(0));
		fxSerializeInfoSetGrammar(the, aRoot, aNode->value.rule.data->link, anInstance);
		break;
	}
	fxSerializeInfoSetNamespaces(the, aRoot);
	aSlot = fxSetProperty(the, aDocument->value.reference, the->elementID, C_NULL);
	aSlot->value = the->stack->value;
	aSlot->kind = the->stack->kind;
	fxSerializeInfoSetChild(the);
	*mxResult = *aDocument;
	the->stack++;
}

void fxSerializeInfoSetAttribute(txMachine* the, txSlot* theRoot, txSlot* theNode, txSlot* theProperty)
{
	txSlot* aParent = the->stack;
	txSlot* attributes = fxGetOwnProperty(the, aParent->value.reference, the->_attributesID);
	txSlot* aSlot = fxGetInstance(the, mxThis);
	aSlot = fxGetProperty(the, aSlot, the->attributeID);
	mxPush(*aSlot);
	fxNewInstanceOf(the);
	
	the->parseFlag |= XS_INFO_SET_FLAG;
	mxPush(*aParent);
	fxQueueID(the, the->parentID, XS_NO_FLAG);
	fxSerializeInfoSetName(the, theRoot, theNode);
	fxSerializeInfoSetValue(the, theRoot, theNode, theProperty);
	
	if (!attributes) {
		attributes = fxSetProperty(the, aParent->value.reference, the->_attributesID, C_NULL);
		mxPush(mxArrayPrototype);
		fxNewArrayInstance(the);
		attributes->value = the->stack->value;
		attributes->kind = the->stack->kind;
		the->stack++;
	}
	mxZeroSlot(--the->stack);
	the->stack->value.integer = 1;
	the->stack->kind = XS_INTEGER_KIND;
	mxZeroSlot(--the->stack);
	the->stack->value = attributes->value;
	the->stack->kind = attributes->kind;
	fxCallID(the, the->pushID);
	the->stack++;
}

void fxSerializeInfoSetCData(txMachine* the, txSlot* theRoot, txSlot* theNode, txSlot* theProperty)
{
	txSlot* aParent = the->stack;
	txSlot* aSlot = fxGetInstance(the, mxThis);
	aSlot = fxGetProperty(the, aSlot, the->cdataID);
	mxPush(*aSlot);
	fxNewInstanceOf(the);
	
	the->parseFlag |= XS_INFO_SET_FLAG;
	mxPush(*aParent);
	fxQueueID(the, the->parentID, XS_NO_FLAG);
	fxSerializeInfoSetValue(the, theRoot, theNode, theProperty);
	fxSerializeInfoSetChild(the);
}

void fxSerializeInfoSetChild(txMachine* the)
{
	if (the->parseFlag & XS_INFO_SET_FLAG) {
		txSlot* aParent = the->stack + 1;
		txSlot* children = fxGetOwnProperty(the, aParent->value.reference, the->childrenID);
		if (!children) {
			children = fxSetProperty(the, aParent->value.reference, the->childrenID, C_NULL);
			mxPush(mxArrayPrototype);
			fxNewArrayInstance(the);
			children->value = the->stack->value;
			children->kind = the->stack->kind;
			the->stack++;
		}
		mxZeroSlot(--the->stack);
		the->stack->value.integer = 1;
		the->stack->kind = XS_INTEGER_KIND;
		mxZeroSlot(--the->stack);
		the->stack->value = children->value;
		the->stack->kind = children->kind;
		fxCallID(the, the->pushID);
	}
	the->stack++;
}

txBoolean fxSerializeInfoSetContent(txMachine* the, txSlot* theRoot, txSlot* theNode, txSlot* theProperty, txID theParent)
{
	txSlot* aNode;
	
	if (theNode->kind == XS_NODE_KIND) {
		fxSerializeInfoSetElement(the, theRoot, theNode, C_NULL);
		aNode = theNode->value.node.link->next;
		while (aNode) {
			if (fxSerializeInfoSetContent(the, theRoot, aNode, theProperty, theParent)) {
				fxSerializeInfoSetChild(the);
				return 1;
			}
			aNode = aNode->next;
		}
		the->stack++;
	}
	else if (theParent != XS_NO_ID) {
		if ((theNode->kind == XS_JUMP_RULE) && (theNode->value.rule.data->alias == theParent)) {
			fxSerializeInfoSetNode(the, theRoot, theNode, theProperty);
			return 1;
		}
	}
	else {
		if (theNode->kind != XS_JUMP_RULE) {
			fxSerializeInfoSetNode(the, theRoot, theNode, theProperty);
			return 1;
		}
	}
	return 0;
}

txBoolean fxSerializeInfoSetContents(txMachine* the, txSlot* theRoot, txSlot* theNode, txSlot* theProperty)
{
	txSlot* aSlot;
	txID aParent;
	txSlot* aNode;
	
	aSlot = theNode->value.rule.data->link->next;
	if (mxIsReference(theProperty)) {
		aNode = fxGetInstance(the, theProperty);
		aParent = aNode->ID;
		aNode = theNode->next;
		while (aNode != aSlot) {
			if (fxSerializeInfoSetContent(the, theRoot, aNode, theProperty, aParent))
				return 1;
			aNode = aNode->next;
		}
	}
	aNode = theNode->next;
	while (aNode != aSlot) {
		if (fxSerializeInfoSetContent(the, theRoot, aNode, theProperty, XS_NO_ID))
			return 1;
		aNode = aNode->next;
	}
	return 0;
}

void fxSerializeInfoSetDocument(txMachine* the)
{
	txSlot* aSlot = fxGetInstance(the, mxThis);
	aSlot = fxGetProperty(the, aSlot, fxID(the, "document"));
	mxPush(*aSlot);
	fxNewInstanceOf(the);

	mxPush(mxArrayPrototype);
	fxNewArrayInstance(the);
	fxQueueID(the, the->childrenID, XS_NO_FLAG);
	mxPushStringC("UTF-8");
	fxQueueID(the, fxID(the, "encoding"), XS_NO_FLAG);
	mxPushStringC("1.0");
	fxQueueID(the, fxID(the, "version"), XS_NO_FLAG);
}

void fxSerializeInfoSetElement(txMachine* the, txSlot* theRoot, txSlot* theNode, txSlot* theProperty)
{
	txSlot* aParent = the->stack;
	txSlot* aSlot = fxGetInstance(the, mxThis);
	aSlot = fxGetProperty(the, aSlot, the->elementID);
	mxPush(*aSlot);
	fxNewInstanceOf(the);
	
	the->parseFlag |= XS_INFO_SET_FLAG;
	if (theProperty && ((the->parseFlag & 64) == 0)) {
		mxZeroSlot(--the->stack);
		the->stack->value = theProperty->value;
		the->stack->kind = theProperty->kind;
		fxQueueID(the, the->instanceID, XS_NO_FLAG);
	}
	mxPush(*aParent);
	fxQueueID(the, the->parentID, XS_NO_FLAG);
	fxSerializeInfoSetName(the, theRoot, theNode);
}

void fxSerializeInfoSetGrammar(txMachine* the, txSlot* theRoot, txSlot* theGrammar, txSlot* theInstance)
{
	txFlag aFlag;
	txSlot* aNode;
	txSlot* aProperty;
	txInteger aLength;
	txInteger anIndex;
	txSlot* anItem;
	
	aNode = theGrammar->next;
	while (aNode) {
		switch (aNode->kind) {
		case XS_NODE_KIND:
			aFlag = the->parseFlag & XS_INFO_SET_FLAG;
			fxSerializeInfoSetElement(the, theRoot, aNode, C_NULL);
			the->parseFlag &= ~XS_INFO_SET_FLAG;
			fxSerializeInfoSetGrammar(the, theRoot, aNode->value.node.link, theInstance);
			fxSerializeInfoSetChild(the);
			the->parseFlag |= aFlag;
			break;
		case XS_ATTRIBUTE_RULE:
			aProperty = fxSerializeInfoSetRule(the, theInstance, aNode);
			if (aProperty)
				fxSerializeInfoSetAttribute(the, theRoot, aNode, aProperty);
			break;
		case XS_DATA_RULE:
			aProperty = fxSerializeInfoSetRule(the, theInstance, aNode);
			if (aProperty)
				fxSerializeInfoSetCData(the, theRoot, aNode, aProperty);
			break;
		case XS_PI_RULE:
			aProperty = fxSerializeInfoSetRule(the, theInstance, aNode);
			if (aProperty)
				fxSerializeInfoSetPI(the, theRoot, aNode, aProperty);
			break;
		case XS_JUMP_RULE:
			aProperty = fxSerializeInfoSetRule(the, theInstance, aNode);
			if (aProperty && mxIsReference(aProperty)) {
				fxSerializeInfoSetElement(the, theRoot, aNode, aProperty);
				fxSerializeInfoSetGrammar(the, theRoot, aNode->value.rule.data->link, fxGetInstance(the, aProperty));
				fxSerializeInfoSetChild(the);
			}
			break;
		case XS_REFER_RULE:
			aProperty = fxSerializeInfoSetRule(the, theInstance, aNode);
			if (aProperty)
				fxSerializeInfoSetContents(the, theRoot, aNode, aProperty);
			aNode = aNode->value.rule.data->link;
			break;
		case XS_REPEAT_RULE:
			aProperty = fxSerializeInfoSetRule(the, theInstance, aNode);
			if (aProperty && mxIsReference(aProperty)) {
				aProperty = fxGetInstance(the, aProperty);
				if (aProperty->next->kind == XS_ARRAY_KIND) {
					aProperty = aProperty->next;
					aLength = aProperty->value.array.length;
					for (anIndex = 0; anIndex < aLength; anIndex++) {
						anItem = aProperty->value.array.address + anIndex;
						if (anItem->ID)
							fxSerializeInfoSetContents(the, theRoot, aNode, anItem);
					}
				}
				else {
					anItem = fxGetProperty(the, aProperty, the->lengthID);
					if (anItem) {
						aLength = fxToInteger(the, anItem);
						for (anIndex = 0; anIndex < aLength; anIndex++) {
							anItem = fxGetProperty(the, aProperty, (txID)anIndex);
							if (anItem)
								fxSerializeInfoSetContents(the, theRoot, aNode, anItem);
						}
					}
				}
			}
			aNode = aNode->value.rule.data->link;
			break;
		}
		aNode = aNode->next;
	}
}

void fxSerializeInfoSetName(txMachine* the, txSlot* theRoot, txSlot* theNode)
{
	txSlot* aSymbol = fxGetSymbol(the, theNode->value.node.part.nameID);
	txString aPrefix;
	if (aSymbol) {
		mxZeroSlot(--the->stack);
		the->stack->value.string = aSymbol->value.symbol.string;
		the->stack->kind = XS_STRING_KIND;
		fxQueueID(the, the->nameID, XS_NO_FLAG);
	}
	if (theNode->value.node.part.namespaceID != XS_NO_ID) {
		aSymbol = fxGetSymbol(the, theNode->value.node.part.namespaceID);
		if (aSymbol) {
			mxZeroSlot(--the->stack);
			the->stack->value.string = aSymbol->value.symbol.string;
			the->stack->kind = XS_STRING_KIND;
			fxQueueID(the, the->namespaceID, XS_NO_FLAG);
			aPrefix = fxSearchPrefix(the, theRoot, theNode->value.node.part.namespaceID);
			if (aPrefix) {
				mxZeroSlot(--the->stack);
				the->stack->value.string = aPrefix;
				the->stack->kind = XS_STRING_KIND;
				fxQueueID(the, the->prefixID, XS_NO_FLAG);
			}
		}
	}
}

void fxSerializeInfoSetNamespaces(txMachine* the, txSlot* theRoot)
{
	txSlot* aParent = the->stack;
	txSlot* aSlot = fxGetInstance(the, mxThis);
	txSlot* aPrototype = fxGetProperty(the, aSlot, the->attributeID);
	txSlot* xmlnsPrefix = fxGetProperty(the, aSlot, the->xmlnsPrefixID);
	txSlot* xmlnsNamespace = fxGetProperty(the, aSlot, the->xmlnsNamespaceID);
	txSlot* aPrefix;
	txSlot* attributes;
	txSlot* aSymbol;
	
	aPrefix = theRoot->next->next;
	if (aPrefix && (aPrefix->kind == XS_PREFIX_KIND)) {
		attributes = fxSetProperty(the, the->stack->value.reference, the->xmlnsAttributesID, C_NULL);
		mxPush(mxArrayPrototype);
		fxNewArrayInstance(the);
		attributes->value = the->stack->value;
		attributes->kind = the->stack->kind;
		the->stack++;
	
		while (aPrefix && (aPrefix->kind == XS_PREFIX_KIND)) {
			mxPush(*aPrototype);
			fxNewInstanceOf(the);
			mxPush(*aParent);
			fxQueueID(the, the->parentID, XS_NO_FLAG);
			
			if (aPrefix->value.prefix.string) {
				mxZeroSlot(--the->stack);
				the->stack->value.string = aPrefix->value.prefix.string;
				the->stack->kind = XS_STRING_KIND;
				fxQueueID(the, the->nameID, XS_NO_FLAG);
				mxPush(*xmlnsNamespace);
				fxQueueID(the, the->namespaceID, XS_NO_FLAG);
				mxPush(*xmlnsPrefix);
				fxQueueID(the, the->prefixID, XS_NO_FLAG);
			}
			else {
				mxPush(*xmlnsPrefix);
				fxQueueID(the, the->nameID, XS_NO_FLAG);
				mxPush(*xmlnsNamespace);
				fxQueueID(the, the->namespaceID, XS_NO_FLAG);
			}
		
			aSymbol = fxGetSymbol(the, aPrefix->value.prefix.part.namespaceID);
			if (aSymbol) {
				mxZeroSlot(--the->stack);
				the->stack->value.string = aSymbol->value.symbol.string;
				the->stack->kind = XS_STRING_KIND;
				fxQueueID(the, the->valueID, XS_NO_FLAG);
			}
			
			mxZeroSlot(--the->stack);
			the->stack->value.integer = 1;
			the->stack->kind = XS_INTEGER_KIND;
			mxZeroSlot(--the->stack);
			the->stack->value = attributes->value;
			the->stack->kind = attributes->kind;
			fxCallID(the, the->pushID);
			the->stack++;
			
			aPrefix = aPrefix->next;
		}
	}
}

void fxSerializeInfoSetNode(txMachine* the, txSlot* theRoot, txSlot* theNode, txSlot* theProperty)
{
	switch (theNode->kind) {
	case XS_NODE_KIND:
		fxSerializeInfoSetElement(the, theRoot, theNode, C_NULL);
		fxSerializeInfoSetNode(the, theRoot, theNode->value.node.link->next, theProperty);
		fxSerializeInfoSetChild(the);
		break;
	case XS_DATA_RULE:
		fxSerializeInfoSetCData(the, theRoot, theNode, theProperty);
		break;
	case XS_PI_RULE:
		fxSerializeInfoSetPI(the, theRoot, theNode, theProperty);
		break;
	case XS_JUMP_RULE:
		if (mxIsReference(theProperty)) {
			fxSerializeInfoSetElement(the, theRoot, theNode, theProperty);
			fxSerializeInfoSetGrammar(the, theRoot, theNode->value.rule.data->link, fxGetInstance(the, theProperty));
			fxSerializeInfoSetChild(the);
		}
		break;
	}
}

void fxSerializeInfoSetPI(txMachine* the, txSlot* theRoot, txSlot* theNode, txSlot* theProperty)
{
	txSlot* aParent = the->stack;
	txSlot* aSlot = fxGetInstance(the, mxThis);
	aSlot = fxGetProperty(the, aSlot, the->piID);
	mxPush(*aSlot);
	fxNewInstanceOf(the);
	
	the->parseFlag |= XS_INFO_SET_FLAG;
	mxPush(*aParent);
	fxQueueID(the, the->parentID, XS_NO_FLAG);
	fxSerializeInfoSetName(the, theRoot, theNode);
	fxSerializeInfoSetValue(the, theRoot, theNode, theProperty);
	fxSerializeInfoSetChild(the);
}

txSlot* fxSerializeInfoSetRule(txMachine* the, txSlot* theInstance, txSlot* theRule)
{
	txSlot* aProperty;
	txInteger aCount = theRule->value.rule.data->count;
	txID* anID = theRule->value.rule.data->IDs;
	while (aCount > 1) {
		aProperty = fxGetOwnProperty(the, theInstance, *anID);
		if (aProperty && mxIsReference(aProperty))
			theInstance = fxGetInstance(the, aProperty);
		else
			break;
		aCount--;
		anID++;
	}	
	if ((aCount == 1) && (*anID != XS_NO_ID))
		aProperty = fxGetOwnProperty(the, theInstance, *anID);
	else
		aProperty = C_NULL;
	return aProperty;
}

void fxSerializeInfoSetValue(txMachine* the, txSlot* theRoot, txSlot* theNode, txSlot* theProperty)
{
	mxZeroSlot(--the->stack);
	the->stack->value = theProperty->value;
	the->stack->kind = theProperty->kind;
	mxZeroSlot(--the->stack);
	the->stack->value.reference = theRoot;
	the->stack->kind = XS_REFERENCE_KIND;
	mxZeroSlot(--the->stack);
	the->stack->value.integer = 1;
	the->stack->kind = XS_INTEGER_KIND;
	mxZeroSlot(--the->stack);
	the->stack->value.integer = 3;
	the->stack->kind = XS_INTEGER_KIND;
	mxZeroSlot(--the->stack);
	the->stack->value.alias = theNode->value.rule.data->alias;
	the->stack->kind = XS_ALIAS_KIND;
	fxCallID(the, the->serializeID);
	{
		txSlot* result = the->stack;
		txSlot* instance = fxGetInstance(the, the->stack);

		if (mxIsChunk(instance)) {
			fxInteger(the, --the->stack, 0);
			*(--the->stack) = *result;
			fxCallID(the, the->toStringID);
			*result = *the->stack;
			the->stack++;
		}
	}

	fxQueueID(the, the->valueID, XS_NO_FLAG);
}

void fx_xs_infoset_get_attributes(txMachine* the)
{
	txSlot* aProperty = fxGetProperty(the, fxGetInstance(the, mxThis), the->_attributesID);
	mxResult->kind = aProperty->kind;
	mxResult->value = aProperty->value;
}

void fx_xs_infoset_set_attributes(txMachine* the)
{
	txSlot* aProperty = fxSetProperty(the, fxGetOwnInstance(the, mxThis), the->_attributesID, C_NULL);
	aProperty->kind = mxArgv(0)->kind;
	aProperty->value = mxArgv(0)->value;
}

void fx_xs_execute(txMachine* the)
{
	txStringStream aStream;
	txSlot* aSlot = C_NULL;
	txFlag aFlag = XS_SANDBOX_FLAG;
	txByte* aCode;
	
	if (mxArgc < 1)
		mxDebug0(the, XS_SYNTAX_ERROR, "xs.execute: no buffer");
		
	fxToString(the, mxArgv(0));
	aStream.slot = mxArgv(0);
	aStream.size = c_strlen(aStream.slot->value.string);
	aStream.offset = 0;
	
	if ((mxArgc >= 1) && (mxArgv(1)->kind != XS_UNDEFINED_KIND)) {
		aFlag |= XS_THIS_FLAG;
		aSlot = mxArgv(1);
	}
	else
		aFlag |= XS_PROGRAM_FLAG;
#ifdef mxDebug
	if (mxArgc >= 3)
		aCode = fxParseScript(the, &aStream, fxStringGetter, 
				fxNewFile(the, mxArgv(2)), mxArgv(3)->value.integer,
				aFlag | XS_DEBUG_FLAG, C_NULL);
	else
#endif
		aCode = fxParseScript(the, &aStream, fxStringGetter, C_NULL, 0,
				aFlag, C_NULL);
	/* ARGC */
	mxZeroSlot(--the->stack);
	the->stack->kind = XS_INTEGER_KIND;
	the->stack->value.integer = 0;
	/* THIS */
	*(--the->stack) = aSlot ? *aSlot : mxGlobal;
	/* FUNCTION */
	mxZeroSlot(--the->stack);
	the->stack->value.code = aCode;
	the->stack->kind = XS_CODE_KIND;
	fxNewProgramInstance(the);
	/* RESULT */
	mxZeroSlot(--the->stack);
	fxRunID(the, XS_NO_ID);
	*mxResult = *(the->stack++);
}

#ifdef mxDebug

void fx_xs_debug_getAddress(txMachine* the)
{
	fxCopyStringC(the, mxResult, fxGetAddress(the));
}

void fx_xs_debug_setAddress(txMachine* the)
{
	if (mxArgc < 1)
		mxDebug0(the, XS_SYNTAX_ERROR, "xs.debug.setAddress: no parameters");
	if (mxArgv(0)->kind != XS_STRING_KIND)
		mxDebug0(the, XS_SYNTAX_ERROR, "xs.debug.setAddress: no string");
	fxSetAddress(the, mxArgv(0)->value.string);
}

void fx_xs_debug_getAutomatic(txMachine* the)
{
	mxResult->value.boolean = fxGetAutomatic(the);
	mxResult->kind = XS_BOOLEAN_KIND;
}

void fx_xs_debug_setAutomatic(txMachine* the)
{
	if (mxArgc < 1)
		mxDebug0(the, XS_SYNTAX_ERROR, "xs.debug.setAutomatic: no parameters");
	if (mxArgv(0)->kind != XS_BOOLEAN_KIND)
		mxDebug0(the, XS_SYNTAX_ERROR, "xs.debug.setAutomatic: no boolean");
	fxSetAutomatic(the, mxArgv(0)->value.boolean);
}

void fx_xs_debug_getBreakOnException(txMachine* the)
{
	mxResult->value.boolean = the->breakOnExceptionFlag;
	mxResult->kind = XS_BOOLEAN_KIND;
}

void fx_xs_debug_setBreakOnException(txMachine* the)
{
	if (mxArgc < 1)
		mxDebug0(the, XS_SYNTAX_ERROR, "xs.debug.setBreakOnException: no parameters");
	if (mxArgv(0)->kind != XS_BOOLEAN_KIND)
		mxDebug0(the, XS_SYNTAX_ERROR, "xs.debug.setBreakOnException: no boolean");
	the->breakOnExceptionFlag = mxArgv(0)->value.boolean;
}

void fx_xs_debug_getConnected(txMachine* the)
{
	mxResult->value.boolean = fxIsConnected(the);
	mxResult->kind = XS_BOOLEAN_KIND;
}

void fx_xs_debug_setConnected(txMachine* the)
{
	if (mxArgc < 1)
		mxDebug0(the, XS_SYNTAX_ERROR, "xs.debug.setConnected: no parameters");
	if (mxArgv(0)->kind != XS_BOOLEAN_KIND)
		mxDebug0(the, XS_SYNTAX_ERROR, "xs.debug.setConnected: no boolean");
	if (mxArgv(0)->value.boolean)
		fxLogin(the);
	else
		fxLogout(the);
}

void fx_xs_debug_clearAllBreakpoints(txMachine* the)
{
	fxClearAllBreakpoints(the);
}

void fx_xs_debug_clearBreakpoint(txMachine* the)
{
	if (mxArgc < 2)
		mxDebug0(the, XS_SYNTAX_ERROR, "xs.debug.clearBreakpoint: not enough parameters");
	if (mxArgv(0)->kind != XS_STRING_KIND)
		mxDebug0(the, XS_SYNTAX_ERROR, "xs.debug.clearBreakpoint: file is no string");
	if (mxArgv(1)->kind != XS_STRING_KIND)
		mxDebug0(the, XS_SYNTAX_ERROR, "xs.debug.clearBreakpoint: line is no string");
	fxClearBreakpoint(the, mxArgv(0)->value.string, mxArgv(1)->value.string);
}

void fx_xs_debug_setBreakpoint(txMachine* the)
{
	if (mxArgc < 2)
		mxDebug0(the, XS_SYNTAX_ERROR, "xs.debug.setBreakpoint: not enough parameters");
	if (mxArgv(0)->kind != XS_STRING_KIND)
		mxDebug0(the, XS_SYNTAX_ERROR, "xs.debug.setBreakpoint: file is no string");
	if (mxArgv(1)->kind != XS_STRING_KIND)
		mxDebug0(the, XS_SYNTAX_ERROR, "xs.debug.setBreakpoint: line is no string");
	fxSetBreakpoint(the, mxArgv(0)->value.string, mxArgv(1)->value.string);
}

#endif















