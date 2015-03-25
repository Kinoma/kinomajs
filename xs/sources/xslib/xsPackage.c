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

#include "xsCompiler.h"

static void ioDocumentation_parse(txMachine* the);
static void xsPackageItem_codeGrammar(txMachine* the);
static void xsPackageItem_codePrototype(txMachine* the);
static void xsPackageItem_crossReference(txMachine* the);
static void xsPackageItem_target(txMachine* the);
static void xsPackage_codeGrammar(txMachine* the);
static void xsPackage_codePrototype(txMachine* the);
static void xsPackage_crossReference(txMachine* the);
static void xsPackage_getNamespaceUri(txMachine* the);
static void xsPackage_target(txMachine* the);
static void xsImport_target(txMachine* the);
static void xsNamespace_crossReference(txMachine* the);
static void xsNamespace_target(txMachine* the);
static void xsObjectItem_codeGrammarItem(txMachine* the);
static void xsObjectItem_codeFileLine(txMachine* the);
static void xsObjectItem_hasPattern(txMachine* the);
static void xsObjectItem_target(txMachine* the);
static void xsPatch_codeGrammar(txMachine* the);
static void xsPatch_codePrototype(txMachine* the);
static void xsPatch_crossReference(txMachine* the);
static void xsPatch_target(txMachine* the);
static void xsProgram_codePrototype(txMachine* the);
static void xsProgram_crossReference(txMachine* the);
static void xsProperty_codeGrammar(txMachine* the);
static void xsProperty_codeGrammarItem(txMachine* the);
static void xsProperty_codeGrammarPattern(txMachine* the);
static void xsProperty_codePrototypeAssignment(txMachine* the);
static void xsProperty_codeQualifiedName(txMachine* the);
static void xsProperty_crossReference(txMachine* the);
static void xsProperty_forceDefaultPattern(txMachine* the);
static void xsProperty_hasPattern(txMachine* the);
static void xsProperty_insertProperty(txMachine* the);
static void xsProperty_searchProperty(txMachine* the);
static void xsLiteral_codeGrammarIO(txMachine* the);
static void xsLiteral_codeGrammarPattern(txMachine* the);
static void xsLiteral_codePrototype(txMachine* the);
static void xsBoolean_codeGrammarIO(txMachine* the);
static void xsChunk_codeGrammarIO(txMachine* the);
static void xsCustom_codeGrammarIO(txMachine* the);
static void xsCustom_crossReference(txMachine* the);
static void xsDate_codeGrammarIO(txMachine* the);
static void xsFunction_codePrototype(txMachine* the);
static void xsFunction_crossReference(txMachine* the);
static void xsNull_codePrototype(txMachine* the);
static void xsNumber_codeGrammarIO(txMachine* the);
static void xsRegExp_codeGrammarIO(txMachine* the);
static void xsRegExp_codePrototype(txMachine* the);
static void xsString_codeGrammarIO(txMachine* the);
static void xsUndefined_codePrototype(txMachine* the);
static void xsReference_codeGrammarPattern(txMachine* the);
static void xsReference_codePrototype(txMachine* the);
static void xsReference_crossReference(txMachine* the);
static void xsArray_codeGrammarPattern(txMachine* the);
static void xsArray_codePrototype(txMachine* the);
static void xsObject_codeGrammar(txMachine* the);
static void xsObject_codeGrammarPattern(txMachine* the);
static void xsObject_codePrototype(txMachine* the);
static void xsObject_crossReference(txMachine* the);
static void xsObject_target(txMachine* the);
static void xsTarget_target(txMachine* the);
static void xsECMAPrototype_initialize(txMachine* the);
static void xsECMAPrototype_codeQualifiedName(txMachine* the);
static void xsInfoSetPrototype_initialize(txMachine* the);
static void xsInfoSetPrototype_codeQualifiedName(txMachine* the);

void fxBuild_xscPackage(txMachine* the)
{
	txSlot* aProperty;
	txSlot ioStringAlias;
	txSlot ioDocumentationAlias;
	txSlot xsPackageItemAlias;
	txSlot xsPackageAlias;
	txSlot xsObjectItemAlias;
	txSlot xsPropertyAlias;
	txSlot xsLiteralAlias;
	txSlot xsReferenceAlias;
	txSlot xsObjectAlias;
	
	txSlot aReference;
	txSlot anAlias;
	
	mxPush(mxGlobal);
	
	mxPushInteger(0);
	fxQueueID(the, fxID(the, "cProgramIndex"), XS_NO_FLAG);

	/* ioString */
	aProperty = fxGetProperty(the, the->stack->value.reference, fxID(the, "__xs__string"));
	mxZeroSlot(&ioStringAlias);
	ioStringAlias.kind = aProperty->kind;
	ioStringAlias.value = aProperty->value;
	
	/* ioDocumentation */
	mxPush(mxObjectPrototype);
	fxNewObjectInstance(the);
	/* properties */
	fxNewHostFunction(the, ioDocumentation_parse, 1);
	fxQueueID(the, fxID(the, "parse"), XS_NO_FLAG);
	fxAliasInstance(the, the->stack);
	ioDocumentationAlias = anAlias = *(the->stack);
	fxQueueID(the, fxID(the, "ioDocumentation"), XS_NO_FLAG);

	/* xsPackageItem */
	mxPush(mxObjectPrototype);
	fxNewObjectInstance(the);
	/* properties */
	fxNewHostFunction(the, xsPackageItem_codeGrammar, 0);
	fxQueueID(the, fxID(the, "codeGrammar"), XS_NO_FLAG);
	fxNewHostFunction(the, xsPackageItem_codePrototype, 0);
	fxQueueID(the, fxID(the, "codePrototype"), XS_NO_FLAG);
	fxNewHostFunction(the, xsPackageItem_crossReference, 0);
	fxQueueID(the, fxID(the, "crossReference"), XS_NO_FLAG);
	fxNewHostFunction(the, xsPackageItem_target, 0);
	fxQueueID(the, fxID(the, "target"), XS_NO_FLAG);
	fxAliasInstance(the, the->stack);
	xsPackageItemAlias = anAlias = *(the->stack);
	fxQueueID(the, fxID(the, "xsPackageItem"), XS_NO_FLAG);
	
	/* xsPackage */
	mxPush(mxObjectPrototype);
	fxNewObjectInstance(the);
	aReference = *(the->stack);
	/* properties */
	mxPush(mxEmptyString);
	fxQueueID(the, fxID(the, "cFlag"), XS_NO_FLAG);
	mxPush(mxEmptyString);
	fxQueueID(the, fxID(the, "deleteFlag"), XS_NO_FLAG);
	mxPush(mxEmptyString);
	fxQueueID(the, fxID(the, "enumFlag"), XS_NO_FLAG);
	mxPush(mxEmptyString);
	fxQueueID(the, fxID(the, "scriptFlag"), XS_NO_FLAG);
	mxPush(mxEmptyString);
	fxQueueID(the, fxID(the, "setFlag"), XS_NO_FLAG);
	mxPushBoolean(1);
	fxQueueID(the, fxID(the, "linked"), XS_NO_FLAG);
	mxPushBoolean(0);
	fxQueueID(the, fxID(the, "patched"), XS_NO_FLAG);
	fxNewHostFunction(the, xsPackage_codeGrammar, 0);
	fxQueueID(the, fxID(the, "codeGrammar"), XS_NO_FLAG);
	fxNewHostFunction(the, xsPackage_codePrototype, 0);
	fxQueueID(the, fxID(the, "codePrototype"), XS_NO_FLAG);
	fxNewHostFunction(the, xsPackage_crossReference, 0);
	fxQueueID(the, fxID(the, "crossReference"), XS_NO_FLAG);
	fxNewHostFunction(the, xsPackage_getNamespaceUri, 0);
	fxQueueID(the, fxID(the, "getNamespaceUri"), XS_NO_FLAG);
	fxNewHostFunction(the, xsPackage_target, 0);
	fxQueueID(the, fxID(the, "target"), XS_NO_FLAG);
	fxAliasInstance(the, the->stack);
	xsPackageAlias = anAlias = *(the->stack);
	fxQueueID(the, fxID(the, "xsPackage"), XS_NO_FLAG);
	/* patterns */
	mxPush(aReference);
	fxNewInstance(the);
	mxPush(anAlias);
	fxQueuePattern(the, XS_JUMP_PATTERN, XS_NO_ID, fxID(the, "package"), XS_NO_ID, XS_NO_FLAG);
	fxNewInstance(the);
	mxPush(xsPackageItemAlias);
	fxQueueID(the, 0, XS_NO_FLAG);
	mxPush(ioDocumentationAlias);
	fxQueuePattern(the, XS_PI_PATTERN, XS_NO_ID, fxID(the, "xsdoc"), 1, XS_NO_FLAG);
	fxAliasInstance(the, the->stack);
	fxQueuePattern(the, XS_REPEAT_PATTERN, XS_NO_ID, XS_NO_ID, fxID(the, "items"), XS_NO_FLAG);
	mxPush(ioStringAlias);
	fxQueuePattern(the, XS_ATTRIBUTE_PATTERN, XS_NO_ID, fxID(the, "c"), fxID(the, "cFlag"), XS_NO_FLAG);
	mxPush(ioStringAlias);
	fxQueuePattern(the, XS_ATTRIBUTE_PATTERN, XS_NO_ID, fxID(the, "delete"), fxID(the, "deleteFlag"), XS_NO_FLAG);
	mxPush(ioStringAlias);
	fxQueuePattern(the, XS_ATTRIBUTE_PATTERN, XS_NO_ID, fxID(the, "enum"), fxID(the, "enumFlag"), XS_NO_FLAG);
	mxPush(ioStringAlias);
	fxQueuePattern(the, XS_ATTRIBUTE_PATTERN, XS_NO_ID, fxID(the, "script"), fxID(the, "scriptFlag"), XS_NO_FLAG);
	mxPush(ioStringAlias);
	fxQueuePattern(the, XS_ATTRIBUTE_PATTERN, XS_NO_ID, fxID(the, "set"), fxID(the, "setFlag"), XS_NO_FLAG);
	fxQueueID(the, the->patternsID, XS_NO_FLAG);
	the->stack++;
	
	/* xsImport */
	mxPush(xsPackageItemAlias);
	fxNewObjectInstance(the);
	aReference = *(the->stack);
	/* properties */
	mxPush(mxEmptyString);
	fxQueueID(the, fxID(the, "href"), XS_NO_FLAG);
	mxPushStringC("static");
	fxQueueID(the, fxID(the, "link"), XS_NO_FLAG);
	fxNewHostFunction(the, xsImport_target, 3);
	fxQueueID(the, fxID(the, "target"), XS_NO_FLAG);
	fxAliasInstance(the, the->stack);
	anAlias = *(the->stack);
	fxQueueID(the, fxID(the, "xsImport"), XS_NO_FLAG);
	/* patterns */
	mxPush(aReference);
	fxNewInstance(the);
	mxPush(anAlias);
	fxQueuePattern(the, XS_JUMP_PATTERN, XS_NO_ID, fxID(the, "import"), XS_NO_ID, XS_NO_FLAG);
	mxPush(ioStringAlias);
	fxQueuePattern(the, XS_ATTRIBUTE_PATTERN, XS_NO_ID, fxID(the, "href"), XS_SAME_ID, XS_NO_FLAG);
	mxPush(ioStringAlias);
	fxQueuePattern(the, XS_ATTRIBUTE_PATTERN, XS_NO_ID, fxID(the, "link"), XS_SAME_ID, XS_NO_FLAG);
	fxQueueID(the, the->patternsID, XS_NO_FLAG);
	the->stack++;

	/* xsNamespace */
	mxPush(xsPackageItemAlias);
	fxNewObjectInstance(the);
	aReference = *(the->stack);
	/* properties */
	mxPush(mxEmptyString);
	fxQueueID(the, fxID(the, "prefix"), XS_NO_FLAG);
	mxPush(mxEmptyString);
	fxQueueID(the, fxID(the, "uri"), XS_NO_FLAG);
	fxNewHostFunction(the, xsNamespace_crossReference, 3);
	fxQueueID(the, fxID(the, "crossReference"), XS_NO_FLAG);
	fxNewHostFunction(the, xsNamespace_target, 3);
	fxQueueID(the, fxID(the, "target"), XS_NO_FLAG);
	fxAliasInstance(the, the->stack);
	anAlias = *(the->stack);
	fxQueueID(the, fxID(the, "xsNamespace"), XS_NO_FLAG);
	/* patterns */
	mxPush(aReference);
	fxNewInstance(the);
	mxPush(anAlias);
	fxQueuePattern(the, XS_JUMP_PATTERN, XS_NO_ID, fxID(the, "namespace"), XS_NO_ID, XS_NO_FLAG);
	mxPush(ioStringAlias);
	fxQueuePattern(the, XS_ATTRIBUTE_PATTERN, XS_NO_ID, fxID(the, "prefix"), XS_SAME_ID, XS_NO_FLAG);
	mxPush(ioStringAlias);
	fxQueuePattern(the, XS_ATTRIBUTE_PATTERN, XS_NO_ID, fxID(the, "uri"), XS_SAME_ID, XS_NO_FLAG);
	fxQueueID(the, the->patternsID, XS_NO_FLAG);
	the->stack++;

	/* xsObjectItem */
	mxPush(xsPackageItemAlias);
	fxNewObjectInstance(the);
	/* properties */
	mxPushBoolean(1);
	fxQueueID(the, fxID(the, "linked"), XS_NO_FLAG);
	fxNewHostFunction(the, xsObjectItem_codeGrammarItem, 0);
	fxQueueID(the, fxID(the, "codeGrammarItem"), XS_NO_FLAG);
	fxNewHostFunction(the, xsObjectItem_codeFileLine, 0);
	fxQueueID(the, fxID(the, "codeFileLine"), XS_NO_FLAG);
	fxNewHostFunction(the, xsObjectItem_hasPattern, 0);
	fxQueueID(the, fxID(the, "hasPattern"), XS_NO_FLAG);
	fxNewHostFunction(the, xsObjectItem_target, 0);
	fxQueueID(the, fxID(the, "target"), XS_NO_FLAG);
	fxAliasInstance(the, the->stack);
	xsObjectItemAlias = anAlias = *(the->stack);
	fxQueueID(the, fxID(the, "xsObjectItem"), XS_NO_FLAG);
	
	/* xsDocumentation */
	mxPush(xsObjectItemAlias);
	fxNewObjectInstance(the);
	/* properties */
	fxAliasInstance(the, the->stack);
	fxQueueID(the, fxID(the, "xsDocumentation"), XS_NO_FLAG);
	
	/* xsPatch */
	mxPush(xsObjectItemAlias);
	fxNewObjectInstance(the);
	aReference = *(the->stack);
	/* properties */
	mxPush(mxEmptyString);
	fxQueueID(the, fxID(the, "prototype"), XS_NO_FLAG);
	fxNewHostFunction(the, xsPatch_codeGrammar, 0);
	fxQueueID(the, fxID(the, "codeGrammar"), XS_NO_FLAG);
	fxNewHostFunction(the, xsPatch_codePrototype, 0);
	fxQueueID(the, fxID(the, "codePrototype"), XS_NO_FLAG);
	fxNewHostFunction(the, xsPatch_crossReference, 0);
	fxQueueID(the, fxID(the, "crossReference"), XS_NO_FLAG);
	fxNewHostFunction(the, xsPatch_target, 0);
	fxQueueID(the, fxID(the, "target"), XS_NO_FLAG);
	fxAliasInstance(the, the->stack);
	anAlias = *(the->stack);
	fxQueueID(the, fxID(the, "xsPatch"), XS_NO_FLAG);
	/* patterns */
	mxPush(aReference);
	fxNewInstance(the);
	mxPush(anAlias);
	fxQueuePattern(the, XS_JUMP_PATTERN, XS_NO_ID, fxID(the, "patch"), XS_NO_ID, XS_NO_FLAG);
	mxPush(ioStringAlias);
	fxQueuePattern(the, XS_ATTRIBUTE_PATTERN, XS_NO_ID, fxID(the, "prototype"), XS_SAME_ID, XS_NO_FLAG);
	fxNewInstance(the);
	mxPush(xsObjectItemAlias);
	fxQueueID(the, 0, XS_NO_FLAG);
	mxPush(ioDocumentationAlias);
	fxQueuePattern(the, XS_PI_PATTERN, XS_NO_ID, fxID(the, "xsdoc"), 1, XS_NO_FLAG);
	fxAliasInstance(the, the->stack);
	fxQueuePattern(the, XS_REPEAT_PATTERN, XS_NO_ID, XS_NO_ID, fxID(the, "items"), XS_NO_FLAG);
	fxQueueID(the, the->patternsID, XS_NO_FLAG);
	the->stack++;
	
	/* xsProgram */
	mxPush(xsObjectItemAlias);
	fxNewObjectInstance(the);
	aReference = *(the->stack);
	/* properties */
	mxPush(mxEmptyString);
	fxQueueID(the, fxID(the, "c"), XS_NO_FLAG);
	mxPush(mxEmptyString);
	fxQueueID(the, fxID(the, "href"), XS_NO_FLAG);
	mxPush(mxEmptyString);
	fxQueueID(the, fxID(the, "value"), XS_NO_FLAG);
	fxNewHostFunction(the, xsProgram_codePrototype, 0);
	fxQueueID(the, fxID(the, "codePrototype"), XS_NO_FLAG);
	fxNewHostFunction(the, xsProgram_crossReference, 0);
	fxQueueID(the, fxID(the, "crossReference"), XS_NO_FLAG);
	fxAliasInstance(the, the->stack);
	anAlias = *(the->stack);
	fxQueueID(the, fxID(the, "xsProgram"), XS_NO_FLAG);
	/* patterns */
	mxPush(aReference);
	fxNewInstance(the);
	mxPush(anAlias);
	fxQueuePattern(the, XS_JUMP_PATTERN, XS_NO_ID, fxID(the, "program"), XS_NO_ID, XS_NO_FLAG);
	mxPush(ioStringAlias);
	fxQueuePattern(the, XS_ATTRIBUTE_PATTERN, XS_NO_ID, fxID(the, "c"), XS_SAME_ID, XS_NO_FLAG);
	mxPush(ioStringAlias);
	fxQueuePattern(the, XS_ATTRIBUTE_PATTERN, XS_NO_ID, fxID(the, "href"), XS_SAME_ID, XS_NO_FLAG);
	mxPush(ioStringAlias);
	fxQueuePattern(the, XS_DATA_PATTERN, XS_NO_ID, XS_NO_ID, fxID(the, "value"), XS_NO_FLAG);
	fxQueueID(the, the->patternsID, XS_NO_FLAG);
	the->stack++;
	
	/* xsProperty */
	mxPush(xsObjectItemAlias);
	fxNewObjectInstance(the);
	aReference = *(the->stack);
	/* properties */
	mxPush(mxEmptyString);
	fxQueueID(the, fxID(the, "name"), XS_NO_FLAG);
	mxPush(mxEmptyString);
	fxQueueID(the, fxID(the, "pattern"), XS_NO_FLAG);
	mxPush(mxEmptyString);
	fxQueueID(the, fxID(the, "deleteFlag"), XS_NO_FLAG);
	mxPush(mxEmptyString);
	fxQueueID(the, fxID(the, "enumFlag"), XS_NO_FLAG);
	mxPush(mxEmptyString);
	fxQueueID(the, fxID(the, "scriptFlag"), XS_NO_FLAG);
	mxPush(mxEmptyString);
	fxQueueID(the, fxID(the, "setFlag"), XS_NO_FLAG);
	mxPushInteger(0);
	fxQueueID(the, fxID(the, "masks"), XS_NO_FLAG);
	mxPushInteger(0);
	fxQueueID(the, fxID(the, "flags"), XS_NO_FLAG);
	mxPushNull();
	fxQueueID(the, fxID(the, "nextProperty"), XS_NO_FLAG);
	mxPushNull();
	fxQueueID(the, fxID(the, "_package"), XS_NO_FLAG);
	fxNewHostFunction(the, xsProperty_codeGrammar, 0);
	fxQueueID(the, fxID(the, "codeGrammar"), XS_NO_FLAG);
	fxNewHostFunction(the, xsProperty_codeGrammarItem, 0);
	fxQueueID(the, fxID(the, "codeGrammarItem"), XS_NO_FLAG);
	fxNewHostFunction(the, xsProperty_codeGrammarPattern, 0);
	fxQueueID(the, fxID(the, "codeGrammarPattern"), XS_NO_FLAG);
	fxNewHostFunction(the, xsProperty_codePrototypeAssignment, 0);
	fxQueueID(the, fxID(the, "codePrototypeAssignment"), XS_NO_FLAG);
	fxNewHostFunction(the, xsProperty_codeQualifiedName, 0);
	fxQueueID(the, fxID(the, "codeQualifiedName"), XS_NO_FLAG);
	fxNewHostFunction(the, xsProperty_crossReference, 0);
	fxQueueID(the, fxID(the, "crossReference"), XS_NO_FLAG);
	fxNewHostFunction(the, xsProperty_forceDefaultPattern, 0);
	fxQueueID(the, fxID(the, "forceDefaultPattern"), XS_NO_FLAG);
	fxNewHostFunction(the, xsProperty_hasPattern, 0);
	fxQueueID(the, fxID(the, "hasPattern"), XS_NO_FLAG);
	fxNewHostFunction(the, xsProperty_insertProperty, 0);
	fxQueueID(the, fxID(the, "insertProperty"), XS_NO_FLAG);
	fxNewHostFunction(the, xsProperty_searchProperty, 0);
	fxQueueID(the, fxID(the, "searchProperty"), XS_NO_FLAG);
	fxAliasInstance(the, the->stack);
	xsPropertyAlias = anAlias = *(the->stack);
	fxQueueID(the, fxID(the, "xsProperty"), XS_NO_FLAG);
	/* patterns */
	mxPush(aReference);
	fxNewInstance(the);
	mxPushNull();
	fxQueueID(the, XS_NO_ID, XS_NO_FLAG);
	mxPush(ioStringAlias);
	fxQueuePattern(the, XS_ATTRIBUTE_PATTERN, XS_NO_ID, fxID(the, "name"), XS_SAME_ID, XS_NO_FLAG);
	mxPush(ioStringAlias);
	fxQueuePattern(the, XS_ATTRIBUTE_PATTERN, XS_NO_ID, fxID(the, "pattern"), XS_SAME_ID, XS_NO_FLAG);
	mxPush(ioStringAlias);
	fxQueuePattern(the, XS_ATTRIBUTE_PATTERN, XS_NO_ID, fxID(the, "delete"), fxID(the, "deleteFlag"), XS_NO_FLAG);
	mxPush(ioStringAlias);
	fxQueuePattern(the, XS_ATTRIBUTE_PATTERN, XS_NO_ID, fxID(the, "enum"), fxID(the, "enumFlag"), XS_NO_FLAG);
	mxPush(ioStringAlias);
	fxQueuePattern(the, XS_ATTRIBUTE_PATTERN, XS_NO_ID, fxID(the, "script"), fxID(the, "scriptFlag"), XS_NO_FLAG);
	mxPush(ioStringAlias);
	fxQueuePattern(the, XS_ATTRIBUTE_PATTERN, XS_NO_ID, fxID(the, "set"), fxID(the, "setFlag"), XS_NO_FLAG);
	fxQueueID(the, the->patternsID, XS_NO_FLAG);
	the->stack++;

	/* xsLiteral */
	mxPush(xsPropertyAlias);
	fxNewObjectInstance(the);
	/* properties */
	fxNewHostFunction(the, xsLiteral_codeGrammarIO, 0);
	fxQueueID(the, fxID(the, "codeGrammarIO"), XS_NO_FLAG);
	fxNewHostFunction(the, xsLiteral_codeGrammarPattern, 0);
	fxQueueID(the, fxID(the, "codeGrammarPattern"), XS_NO_FLAG);
	fxNewHostFunction(the, xsLiteral_codePrototype, 0);
	fxQueueID(the, fxID(the, "codePrototype"), XS_NO_FLAG);
	fxAliasInstance(the, the->stack);
	xsLiteralAlias = anAlias = *(the->stack);
	fxQueueID(the, fxID(the, "xsLiteral"), XS_NO_FLAG);

	/* xsBoolean */
	mxPush(xsLiteralAlias);
	fxNewObjectInstance(the);
	aReference = *(the->stack);
	/* properties */
	mxPushStringC("false");
	fxQueueID(the, fxID(the, "value"), XS_NO_FLAG);
	fxNewHostFunction(the, xsBoolean_codeGrammarIO, 0);
	fxQueueID(the, fxID(the, "codeGrammarIO"), XS_NO_FLAG);
	fxAliasInstance(the, the->stack);
	anAlias = *(the->stack);
	fxQueueID(the, fxID(the, "xsBoolean"), XS_NO_FLAG);
	/* patterns */
	mxPush(aReference);
	fxNewInstance(the);
	mxPush(anAlias);
	fxQueuePattern(the, XS_JUMP_PATTERN, XS_NO_ID, fxID(the, "boolean"), XS_NO_ID, XS_NO_FLAG);
	mxPush(ioStringAlias);
	fxQueuePattern(the, XS_ATTRIBUTE_PATTERN, XS_NO_ID, fxID(the, "value"), XS_SAME_ID, XS_NO_FLAG);
	fxQueueID(the, the->patternsID, XS_NO_FLAG);
	the->stack++;

	/* xsChunk */
	mxPush(xsLiteralAlias);
	fxNewObjectInstance(the);
	aReference = *(the->stack);
	/* properties */
	mxPushStringC("");
	fxQueueID(the, fxID(the, "value"), XS_NO_FLAG);
	fxNewHostFunction(the, xsChunk_codeGrammarIO, 0);
	fxQueueID(the, fxID(the, "codeGrammarIO"), XS_NO_FLAG);
	fxAliasInstance(the, the->stack);
	anAlias = *(the->stack);
	fxQueueID(the, fxID(the, "xsChunk"), XS_NO_FLAG);
	/* patterns */
	mxPush(aReference);
	fxNewInstance(the);
	mxPush(anAlias);
	fxQueuePattern(the, XS_JUMP_PATTERN, XS_NO_ID, fxID(the, "chunk"), XS_NO_ID, XS_NO_FLAG);
	mxPush(ioStringAlias);
	fxQueuePattern(the, XS_ATTRIBUTE_PATTERN, XS_NO_ID, fxID(the, "value"), XS_SAME_ID, XS_NO_FLAG);
	fxQueueID(the, the->patternsID, XS_NO_FLAG);
	the->stack++;

	/* xsCustom */
	mxPush(xsLiteralAlias);
	fxNewObjectInstance(the);
	aReference = *(the->stack);
	/* properties */
	mxPush(mxEmptyString);
	fxQueueID(the, fxID(the, "io"), XS_NO_FLAG);
	mxPush(mxEmptyString);
	fxQueueID(the, fxID(the, "value"), XS_NO_FLAG);
	fxNewHostFunction(the, xsCustom_codeGrammarIO, 0);
	fxQueueID(the, fxID(the, "codeGrammarIO"), XS_NO_FLAG);
	fxNewHostFunction(the, xsCustom_crossReference, 0);
	fxQueueID(the, fxID(the, "crossReference"), XS_NO_FLAG);
	fxAliasInstance(the, the->stack);
	anAlias = *(the->stack);
	fxQueueID(the, fxID(the, "xsCustom"), XS_NO_FLAG);
	/* patterns */
	mxPush(aReference);
	fxNewInstance(the);
	mxPush(anAlias);
	fxQueuePattern(the, XS_JUMP_PATTERN, XS_NO_ID, fxID(the, "custom"), XS_NO_ID, XS_NO_FLAG);
	mxPush(ioStringAlias);
	fxQueuePattern(the, XS_ATTRIBUTE_PATTERN, XS_NO_ID, fxID(the, "io"), XS_SAME_ID, XS_NO_FLAG);
	mxPush(ioStringAlias);
	fxQueuePattern(the, XS_ATTRIBUTE_PATTERN, XS_NO_ID, fxID(the, "value"), XS_SAME_ID, XS_NO_FLAG);
	fxQueueID(the, the->patternsID, XS_NO_FLAG);
	the->stack++;

	/* xsDate */
	mxPush(xsLiteralAlias);
	fxNewObjectInstance(the);
	aReference = *(the->stack);
	/* properties */
	mxPushStringC("01 Jan 1970 00:00:00 GMT");
	fxQueueID(the, fxID(the, "value"), XS_NO_FLAG);
	fxNewHostFunction(the, xsDate_codeGrammarIO, 0);
	fxQueueID(the, fxID(the, "codeGrammarIO"), XS_NO_FLAG);
	fxAliasInstance(the, the->stack);
	anAlias = *(the->stack);
	fxQueueID(the, fxID(the, "xsDate"), XS_NO_FLAG);
	/* patterns */
	mxPush(aReference);
	fxNewInstance(the);
	mxPush(anAlias);
	fxQueuePattern(the, XS_JUMP_PATTERN, XS_NO_ID, fxID(the, "date"), XS_NO_ID, XS_NO_FLAG);
	mxPush(ioStringAlias);
	fxQueuePattern(the, XS_ATTRIBUTE_PATTERN, XS_NO_ID, fxID(the, "value"), XS_SAME_ID, XS_NO_FLAG);
	fxQueueID(the, the->patternsID, XS_NO_FLAG);
	the->stack++;

	/* xsFunction */
	mxPush(xsLiteralAlias);
	fxNewObjectInstance(the);
	aReference = *(the->stack);
	/* properties */
	mxPush(mxEmptyString);
	fxQueueID(the, fxID(the, "c"), XS_NO_FLAG);
	mxPush(mxEmptyString);
	fxQueueID(the, fxID(the, "check"), XS_NO_FLAG);
	mxPush(mxEmptyString);
	fxQueueID(the, fxID(the, "params"), XS_NO_FLAG);
	mxPush(mxEmptyString);
	fxQueueID(the, fxID(the, "prototype"), XS_NO_FLAG);
	mxPush(mxEmptyString);
	fxQueueID(the, fxID(the, "value"), XS_NO_FLAG);
	fxNewHostFunction(the, xsFunction_codePrototype, 0);
	fxQueueID(the, fxID(the, "codePrototype"), XS_NO_FLAG);
	fxNewHostFunction(the, xsFunction_crossReference, 0);
	fxQueueID(the, fxID(the, "crossReference"), XS_NO_FLAG);
	fxAliasInstance(the, the->stack);
	anAlias = *(the->stack);
	fxQueueID(the, fxID(the, "xsFunction"), XS_NO_FLAG);
	/* patterns */
	mxPush(aReference);
	fxNewInstance(the);
	mxPush(anAlias);
	fxQueuePattern(the, XS_JUMP_PATTERN, XS_NO_ID, fxID(the, "function"), XS_NO_ID, XS_NO_FLAG);
	mxPush(ioStringAlias);
	fxQueuePattern(the, XS_ATTRIBUTE_PATTERN, XS_NO_ID, fxID(the, "c"), XS_SAME_ID, XS_NO_FLAG);
	mxPush(ioStringAlias);
	fxQueuePattern(the, XS_ATTRIBUTE_PATTERN, XS_NO_ID, fxID(the, "check"), XS_SAME_ID, XS_NO_FLAG);
	mxPush(ioStringAlias);
	fxQueuePattern(the, XS_ATTRIBUTE_PATTERN, XS_NO_ID, fxID(the, "params"), XS_SAME_ID, XS_NO_FLAG);
	mxPushNull();
	fxQueueID(the, fxID(the, "pattern"), XS_NO_FLAG);
	mxPush(ioStringAlias);
	fxQueuePattern(the, XS_ATTRIBUTE_PATTERN, XS_NO_ID, fxID(the, "prototype"), XS_SAME_ID, XS_NO_FLAG);
	mxPush(ioStringAlias);
	fxQueuePattern(the, XS_DATA_PATTERN, XS_NO_ID, XS_NO_ID, fxID(the, "value"), XS_NO_FLAG);
	fxQueueID(the, the->patternsID, XS_NO_FLAG);
	the->stack++;

	/* xsNull */
	mxPush(xsLiteralAlias);
	fxNewObjectInstance(the);
	/* properties */
	aReference = *(the->stack);
	fxNewHostFunction(the, xsNull_codePrototype, 0);
	fxQueueID(the, fxID(the, "codePrototype"), XS_NO_FLAG);
	fxAliasInstance(the, the->stack);
	anAlias = *(the->stack);
	fxQueueID(the, fxID(the, "xsNull"), XS_NO_FLAG);
	/* patterns */
	mxPush(aReference);
	fxNewInstance(the);
	mxPush(anAlias);
	fxQueuePattern(the, XS_JUMP_PATTERN, XS_NO_ID, fxID(the, "null"), XS_NO_ID, XS_NO_FLAG);
	mxPushNull();
	fxQueueID(the, fxID(the, "pattern"), XS_NO_FLAG);
	fxQueueID(the, the->patternsID, XS_NO_FLAG);
	the->stack++;

	/* xsNumber */
	mxPush(xsLiteralAlias);
	fxNewObjectInstance(the);
	aReference = *(the->stack);
	/* properties */
	mxPushStringC("0");
	fxQueueID(the, fxID(the, "value"), XS_NO_FLAG);
	fxNewHostFunction(the, xsNumber_codeGrammarIO, 0);
	fxQueueID(the, fxID(the, "codeGrammarIO"), XS_NO_FLAG);
	fxAliasInstance(the, the->stack);
	anAlias = *(the->stack);
	fxQueueID(the, fxID(the, "xsNumber"), XS_NO_FLAG);
	/* patterns */
	mxPush(aReference);
	fxNewInstance(the);
	mxPush(anAlias);
	fxQueuePattern(the, XS_JUMP_PATTERN, XS_NO_ID, fxID(the, "number"), XS_NO_ID, XS_NO_FLAG);
	mxPush(ioStringAlias);
	fxQueuePattern(the, XS_ATTRIBUTE_PATTERN, XS_NO_ID, fxID(the, "value"), XS_SAME_ID, XS_NO_FLAG);
	fxQueueID(the, the->patternsID, XS_NO_FLAG);
	the->stack++;

	/* xsRegExp */
	mxPush(xsLiteralAlias);
	fxNewObjectInstance(the);
	aReference = *(the->stack);
	/* properties */
	mxPushStringC("//");
	fxQueueID(the, fxID(the, "value"), XS_NO_FLAG);
	fxNewHostFunction(the, xsRegExp_codeGrammarIO, 0);
	fxQueueID(the, fxID(the, "codeGrammarIO"), XS_NO_FLAG);
	fxNewHostFunction(the, xsRegExp_codePrototype, 0);
	fxQueueID(the, fxID(the, "codePrototype"), XS_NO_FLAG);
	fxAliasInstance(the, the->stack);
	anAlias = *(the->stack);
	fxQueueID(the, fxID(the, "xsRegExp"), XS_NO_FLAG);
	/* patterns */
	mxPush(aReference);
	fxNewInstance(the);
	mxPush(anAlias);
	fxQueuePattern(the, XS_JUMP_PATTERN, XS_NO_ID, fxID(the, "regexp"), XS_NO_ID, XS_NO_FLAG);
	mxPush(ioStringAlias);
	fxQueuePattern(the, XS_ATTRIBUTE_PATTERN, XS_NO_ID, fxID(the, "value"), XS_SAME_ID, XS_NO_FLAG);
	fxQueueID(the, the->patternsID, XS_NO_FLAG);
	the->stack++;

	/* xsString */
	mxPush(xsLiteralAlias);
	fxNewObjectInstance(the);
	aReference = *(the->stack);
	/* properties */
	mxPush(mxEmptyString);
	fxQueueID(the, fxID(the, "value"), XS_NO_FLAG);
	fxNewHostFunction(the, xsString_codeGrammarIO, 0);
	fxQueueID(the, fxID(the, "codeGrammarIO"), XS_NO_FLAG);
	fxAliasInstance(the, the->stack);
	anAlias = *(the->stack);
	fxQueueID(the, fxID(the, "xsString"), XS_NO_FLAG);
	/* patterns */
	mxPush(aReference);
	fxNewInstance(the);
	mxPush(anAlias);
	fxQueuePattern(the, XS_JUMP_PATTERN, XS_NO_ID, fxID(the, "string"), XS_NO_ID, XS_NO_FLAG);
	mxPush(ioStringAlias);
	fxQueuePattern(the, XS_ATTRIBUTE_PATTERN, XS_NO_ID, fxID(the, "value"), XS_SAME_ID, XS_NO_FLAG);
	fxQueueID(the, the->patternsID, XS_NO_FLAG);
	the->stack++;

	/* xsUndefined */
	mxPush(xsLiteralAlias);
	fxNewObjectInstance(the);
	aReference = *(the->stack);
	/* properties */
	fxNewHostFunction(the, xsUndefined_codePrototype, 0);
	fxQueueID(the, fxID(the, "codePrototype"), XS_NO_FLAG);
	fxAliasInstance(the, the->stack);
	anAlias = *(the->stack);
	fxQueueID(the, fxID(the, "xsUndefined"), XS_NO_FLAG);
	/* patterns */
	mxPush(aReference);
	fxNewInstance(the);
	mxPush(anAlias);
	fxQueuePattern(the, XS_JUMP_PATTERN, XS_NO_ID, fxID(the, "undefined"), XS_NO_ID, XS_NO_FLAG);
	mxPushNull();
	fxQueueID(the, fxID(the, "pattern"), XS_NO_FLAG);
	fxQueueID(the, the->patternsID, XS_NO_FLAG);
	the->stack++;

	/* xsReference */
	mxPush(xsPropertyAlias);
	fxNewObjectInstance(the);
	aReference = *(the->stack);
	/* properties */
	mxPush(mxEmptyString);
	fxQueueID(the, fxID(the, "contents"), XS_NO_FLAG);
	fxNewHostFunction(the, xsReference_codeGrammarPattern, 0);
	fxQueueID(the, fxID(the, "codeGrammarPattern"), XS_NO_FLAG);
	fxNewHostFunction(the, xsReference_codePrototype, 0);
	fxQueueID(the, fxID(the, "codePrototype"), XS_NO_FLAG);
	fxNewHostFunction(the, xsReference_crossReference, 0);
	fxQueueID(the, fxID(the, "crossReference"), XS_NO_FLAG);
	fxAliasInstance(the, the->stack);
	xsReferenceAlias = anAlias = *(the->stack);
	fxQueueID(the, fxID(the, "xsReference"), XS_NO_FLAG);
	/* patterns */
	mxPush(aReference);
	fxNewInstance(the);
	mxPush(anAlias);
	fxQueuePattern(the, XS_JUMP_PATTERN, XS_NO_ID, fxID(the, "reference"), XS_NO_ID, XS_NO_FLAG);
	mxPush(ioStringAlias);
	fxQueuePattern(the, XS_ATTRIBUTE_PATTERN, XS_NO_ID, fxID(the, "contents"), XS_SAME_ID, XS_NO_FLAG);
	fxQueueID(the, the->patternsID, XS_NO_FLAG);
	the->stack++;

	/* xsArray */
	mxPush(xsReferenceAlias);
	fxNewObjectInstance(the);
	/* properties */
	aReference = *(the->stack);
	fxNewHostFunction(the, xsArray_codeGrammarPattern, 0);
	fxQueueID(the, fxID(the, "codeGrammarPattern"), XS_NO_FLAG);
	fxNewHostFunction(the, xsArray_codePrototype, 0);
	fxQueueID(the, fxID(the, "codePrototype"), XS_NO_FLAG);
	fxAliasInstance(the, the->stack);
	anAlias = *(the->stack);
	fxQueueID(the, fxID(the, "xsReference"), XS_NO_FLAG);
	/* patterns */
	mxPush(aReference);
	fxNewInstance(the);
	mxPush(anAlias);
	fxQueuePattern(the, XS_JUMP_PATTERN, XS_NO_ID, fxID(the, "array"), XS_NO_ID, XS_NO_FLAG);
	fxQueueID(the, the->patternsID, XS_NO_FLAG);
	the->stack++;

	/* xsObject */
	mxPush(xsPropertyAlias);
	fxNewObjectInstance(the);
	aReference = *(the->stack);
	/* properties */
	mxPush(mxEmptyString);
	fxQueueID(the, fxID(the, "c"), XS_NO_FLAG);
	mxPushBoolean(0);
	fxQueueID(the, fxID(the, "grammarFlag"), XS_NO_FLAG);
	mxPush(mxEmptyString);
	fxQueueID(the, fxID(the, "prototype"), XS_NO_FLAG);
	fxNewHostFunction(the, xsObject_codeGrammar, 0);
	fxQueueID(the, fxID(the, "codeGrammar"), XS_NO_FLAG);
	fxNewHostFunction(the, xsObject_codeGrammarPattern, 0);
	fxQueueID(the, fxID(the, "codeGrammarPattern"), XS_NO_FLAG);
	fxNewHostFunction(the, xsObject_codePrototype, 0);
	fxQueueID(the, fxID(the, "codePrototype"), XS_NO_FLAG);
	fxNewHostFunction(the, xsObject_crossReference, 0);
	fxQueueID(the, fxID(the, "crossReference"), XS_NO_FLAG);
	fxNewHostFunction(the, xsObject_target, 0);
	fxQueueID(the, fxID(the, "target"), XS_NO_FLAG);
	fxAliasInstance(the, the->stack);
	xsObjectAlias = anAlias = *(the->stack);
	fxQueueID(the, fxID(the, "xsObject"), XS_NO_FLAG);
	/* patterns */
	mxPush(aReference);
	fxNewInstance(the);
	mxPush(anAlias);
	fxQueuePattern(the, XS_JUMP_PATTERN, XS_NO_ID, fxID(the, "object"), XS_NO_ID, XS_NO_FLAG);
	mxPush(ioStringAlias);
	fxQueuePattern(the, XS_ATTRIBUTE_PATTERN, XS_NO_ID, fxID(the, "c"), XS_SAME_ID, XS_NO_FLAG);
	mxPush(ioStringAlias);
	fxQueuePattern(the, XS_ATTRIBUTE_PATTERN, XS_NO_ID, fxID(the, "prototype"), XS_SAME_ID, XS_NO_FLAG);
	fxNewInstance(the);
	mxPush(xsObjectItemAlias);
	fxQueueID(the, 0, XS_NO_FLAG);
	mxPush(ioDocumentationAlias);
	fxQueuePattern(the, XS_PI_PATTERN, XS_NO_ID, fxID(the, "xsdoc"), 1, XS_NO_FLAG);
	fxAliasInstance(the, the->stack);
	fxQueuePattern(the, XS_REPEAT_PATTERN, XS_NO_ID, XS_NO_ID, fxID(the, "items"), XS_NO_FLAG);
	fxQueueID(the, the->patternsID, XS_NO_FLAG);
	the->stack++;
	
	/* xsTarget */
	mxPush(xsObjectItemAlias);
	fxNewObjectInstance(the);
	aReference = *(the->stack);
	/* properties */
	mxPush(mxEmptyString);
	fxQueueID(the, fxID(the, "name"), XS_NO_FLAG);
	fxNewHostFunction(the, xsTarget_target, 3);
	fxQueueID(the, fxID(the, "target"), XS_NO_FLAG);
	fxAliasInstance(the, the->stack);
	anAlias = *(the->stack);
	fxQueueID(the, fxID(the, "xsTarget"), XS_NO_FLAG);
	/* patterns */
	mxPush(aReference);
	fxNewInstance(the);
	mxPush(anAlias);
	fxQueuePattern(the, XS_JUMP_PATTERN, XS_NO_ID, fxID(the, "target"), XS_NO_ID, XS_NO_FLAG);
	mxPush(ioStringAlias);
	fxQueuePattern(the, XS_ATTRIBUTE_PATTERN, XS_NO_ID, fxID(the, "name"), XS_SAME_ID, XS_NO_FLAG);
	fxNewInstance(the);
	mxPush(xsObjectItemAlias);
	fxQueueID(the, 0, XS_NO_FLAG);
	mxPush(ioDocumentationAlias);
	fxQueuePattern(the, XS_PI_PATTERN, XS_NO_ID, fxID(the, "xsdoc"), 1, XS_NO_FLAG);
	fxAliasInstance(the, the->stack);
	fxQueuePattern(the, XS_REPEAT_PATTERN, XS_NO_ID, XS_NO_ID, fxID(the, "items"), XS_NO_FLAG);
	fxQueueID(the, the->patternsID, XS_NO_FLAG);
	the->stack++;
	
	mxPush(xsObjectAlias);
	fxNewObjectInstance(the);
	mxPushBoolean(1);
	fxQueueID(the, fxID(the, "grammarFlag"), XS_NO_FLAG);
	fxNewHostFunction(the, xsECMAPrototype_initialize, 0);
	fxQueueID(the, fxID(the, "initialize"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, xsECMAPrototype_codeQualifiedName, 0);
	fxQueueID(the, fxID(the, "codeQualifiedName"), XS_DONT_ENUM_FLAG);
	fxAliasInstance(the, the->stack);
	anAlias = *(the->stack);
	fxQueueID(the, fxID(the, "xsECMAPrototype"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	
	mxPush(anAlias);
	fxNewObjectInstance(the);
	mxPushStringC("Object");
	fxQueueID(the, fxID(the, "name"), 0);
	fxQueueID(the, fxID(the, "xsObjectPrototype"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	
	mxPush(anAlias);
	fxNewObjectInstance(the);
	mxPushStringC("Function");
	fxQueueID(the, fxID(the, "name"), 0);
	fxQueueID(the, fxID(the, "xsFunctionPrototype"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	
	mxPush(anAlias);
	fxNewObjectInstance(the);
	mxPushStringC("Array");
	fxQueueID(the, fxID(the, "name"), 0);
	fxQueueID(the, fxID(the, "xsArrayPrototype"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	
	mxPush(anAlias);
	fxNewObjectInstance(the);
	mxPushStringC("String");
	fxQueueID(the, fxID(the, "name"), 0);
	fxQueueID(the, fxID(the, "xsStringPrototype"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	
	mxPush(anAlias);
	fxNewObjectInstance(the);
	mxPushStringC("Boolean");
	fxQueueID(the, fxID(the, "name"), 0);
	fxQueueID(the, fxID(the, "xsBooleanPrototype"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	
	mxPush(anAlias);
	fxNewObjectInstance(the);
	mxPushStringC("Number");
	fxQueueID(the, fxID(the, "name"), 0);
	fxQueueID(the, fxID(the, "xsNumberPrototype"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	
	mxPush(anAlias);
	fxNewObjectInstance(the);
	mxPushStringC("Date");
	fxQueueID(the, fxID(the, "name"), 0);
	fxQueueID(the, fxID(the, "xsDatePrototype"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	
	mxPush(anAlias);
	fxNewObjectInstance(the);
	mxPushStringC("RegExp");
	fxQueueID(the, fxID(the, "name"), 0);
	fxQueueID(the, fxID(the, "xsRegExpPrototype"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	
	mxPush(anAlias);
	fxNewObjectInstance(the);
	mxPushStringC("Error");
	fxQueueID(the, fxID(the, "name"), 0);
	fxQueueID(the, fxID(the, "xsErrorPrototype"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	
	mxPush(anAlias);
	fxNewObjectInstance(the);
	mxPushStringC("Chunk");
	fxQueueID(the, fxID(the, "name"), 0);
	fxQueueID(the, fxID(the, "xsChunkPrototype"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	
	mxPush(xsObjectAlias);
	fxNewObjectInstance(the);
	mxPushBoolean(1);
	fxQueueID(the, fxID(the, "grammarFlag"), XS_NO_FLAG);
	fxNewHostFunction(the, xsInfoSetPrototype_initialize, 0);
	fxQueueID(the, fxID(the, "initialize"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, xsInfoSetPrototype_codeQualifiedName, 0);
	fxQueueID(the, fxID(the, "codeQualifiedName"), XS_DONT_ENUM_FLAG);
	mxPushStringC("");
	fxQueueID(the, fxID(the, "name"), 0);
	fxAliasInstance(the, the->stack);
	anAlias = *(the->stack);
	fxQueueID(the, fxID(the, "xsInfoSetPrototype"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	
	mxPush(anAlias);
	fxNewObjectInstance(the);
	mxPushStringC("document");
	fxQueueID(the, fxID(the, "name"), 0);
	fxQueueID(the, fxID(the, "xsDocumentPrototype"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	
	mxPush(anAlias);
	fxNewObjectInstance(the);
	mxPushStringC("element");
	fxQueueID(the, fxID(the, "name"), 0);
	fxQueueID(the, fxID(the, "xsElementPrototype"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	
	mxPush(anAlias);
	fxNewObjectInstance(the);
	mxPushStringC("attribute");
	fxQueueID(the, fxID(the, "name"), 0);
	fxQueueID(the, fxID(the, "xsAttributePrototype"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	
	mxPush(anAlias);
	fxNewObjectInstance(the);
	mxPushStringC("cdata");
	fxQueueID(the, fxID(the, "name"), 0);
	fxQueueID(the, fxID(the, "xsCDataPrototype"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	
	mxPush(anAlias);
	fxNewObjectInstance(the);
	mxPushStringC("pi");
	fxQueueID(the, fxID(the, "name"), 0);
	fxQueueID(the, fxID(the, "xsPIPrototype"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	
	mxPush(anAlias);
	fxNewObjectInstance(the);
	mxPushStringC("comment");
	fxQueueID(the, fxID(the, "name"), 0);
	fxQueueID(the, fxID(the, "xsCommentPrototype"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	
	mxPush(xsPackageAlias);
	mxPushInteger(1);
	mxPush(mxGlobal);
	fxCallID(the, fxID(the, "__xs__root"));
	the->stack++;
	
	mxPushInteger(0);
	mxPush(mxGlobal);
	fxCallID(the, fxID(the, "__xs__link"));
	the->stack++;
	
	the->stack++; /* mxGlobal */
	
	fxDebugger(the, NULL, 0);
}

void ioDocumentation_parse(txMachine* the)
{
	txSlot* aProperty;

	aProperty = fxGetProperty(the, mxGlobal.value.reference, fxID(the, "xsDocumentation"));
	mxZeroSlot(--the->stack);
	the->stack->value = aProperty->value;
	the->stack->kind = aProperty->kind;
	fxNewObjectInstance(the);
	mxResult->value = the->stack->value;
	mxResult->kind = the->stack->kind;
}

void xsPackageItem_codeGrammar(txMachine* the)
{
}

void xsPackageItem_codePrototype(txMachine* the)
{
}

void xsPackageItem_crossReference(txMachine* the)
{
}

void xsPackageItem_target(txMachine* the)
{
}

void xsPackage_codeGrammar(txMachine* the)
{
	xsIntegerValue c, i;
	xsVars(3);

	if (xsTest(xsGet(xsThis, xsID("linked"))) || xsTest(xsGet(xsThis, xsID("patched")))) {
		xsVar(0) = xsGet(xsThis, xsID("namespaces"));
		c = xsToInteger(xsGet(xsVar(0), the->lengthID));
		if (c) {
			xscAppendInteger(the, 0);
			xscAppend1(the, XS_THIS);
			xscAppendID(the, XS_GET_MEMBER_FOR_CALL, xsID("Object"));
			xscAppend1(the, XS_NEW);
			
			for (i = 0; i < c; i++) {
				xsVar(1) = xsGetAt(xsVar(0), xsInteger(i));
				xscAppend1(the, XS_DUB);
				xsVar(2) = xsGet(xsVar(1), xsID("prefix"));
				if (xsTest(xsVar(2)))
					xscAppendString(the, xsToString(xsVar(2)));
				else
					xscAppend1(the, XS_NULL);
				xscAppend1(the, XS_SWAP);
				xsVar(2) = xsGet(xsVar(1), xsID("uri"));
				xscAppendID(the, XS_SET_MEMBER, xsToID(xsVar(2)));
				xscAppend1(the, XS_POP);
			}
			xscAppendInteger(the, 1);
			xscAppend1(the, XS_THIS);
			xscAppendID(the, XS_GET_MEMBER_FOR_CALL, xsID("__xs__prefix"));
			xscAppend1(the, XS_CALL);
			xscAppend1(the, XS_POP);
		}
	}
	if (xsTest(xsGet(xsThis, xsID("linked")))) {
		xsVar(0) = xsGet(xsThis, xsID("items"));
		c = xsToInteger(xsGet(xsVar(0), the->lengthID));
		for (i = 0; i < c; i++) 
			(void)xsCall0(xsGetAt(xsVar(0), xsInteger(i)), xsID("codeGrammar"));
	}
}

void xsPackage_codePrototype(txMachine* the)
{
	xsIntegerValue c, i;
	xsVars(1);

	if (xsTest(xsGet(xsThis, xsID("linked")))) {
		xsVar(0) = xsGet(xsThis, xsID("items"));
		c = xsToInteger(xsGet(xsVar(0), the->lengthID));
		for (i = 0; i < c; i++) 
			(void)xsCall0(xsGetAt(xsVar(0), xsInteger(i)), xsID("codePrototype"));
	}
}

void xsPackage_crossReference(txMachine* the)
{
	xsIntegerValue c, i;
	xsVars(2);
	xsVar(0) = xsGet(xsGlobal, xsID("xsc"));
	
	if (xsTest(xsGet(xsVar(0), xsID("verbose")))) {
		if (xsTest(xsGet(xsThis, xsID("linked")))) {
			xsVar(1) = xsCat3(xsString("# Compiling '"), xsGet(xsThis, xsID("__xs__path")), xsString("'..."));
			(void)xsCall1(xsVar(0), xsID("print"), xsVar(1));
		}
	}
	xsVar(1) = xsNewInstanceOf(xsArrayPrototype);
	xsSet(xsThis, xsID("namespaces"), xsVar(1));
	
	xsVar(1) = xsGet(xsThis, xsID("items"));
	c = xsToInteger(xsGet(xsVar(1), the->lengthID));
	for (i = 0; i < c; i++) 
		(void)xsCall3(xsGetAt(xsVar(1), xsInteger(i)), xsID("crossReference"), xsThis, xsString(""), xsArg(2));
}

void xsPackage_getNamespaceUri(txMachine* the)
{
	xsIntegerValue c, i;
	xsStringValue aPrefix;
	xsVars(2);
	
	xsVar(0) = xsGet(xsThis, xsID("namespaces"));
	c = xsToInteger(xsGet(xsVar(0), the->lengthID));
	for (i = 0; i < c; i++) {
		xsVar(1) = xsGetAt(xsVar(0), xsInteger(i));
		aPrefix = xsToString(xsGet(xsVar(1), xsID("prefix")));
		if (strcmp(aPrefix, xsToString(xsArg(0))) == 0) {
			xsResult = xsGet(xsVar(1), xsID("uri"));
			return;
		}
	}
	xsResult = xsNull;
}

void xsPackage_target(txMachine* the)
{
	xsIntegerValue c, i;
	xsVars(2);
	
	xsVar(0) = xsGet(xsThis, xsID("items"));
	xsVar(1) = xsNewInstanceOf(xsArrayPrototype);
	if (xsTypeOf(xsVar(0)) != xsUndefinedType) {
		xsArg(2) = xsCat2(xsArg(2), xsString(" "));
		c = xsToInteger(xsGet(xsVar(0), the->lengthID));
		for (i = 0; i < c; i++) 
			(void)xsCall3(xsGetAt(xsVar(0), xsInteger(i)), xsID("target"), xsVar(1), xsArg(1), xsArg(2));
	}
	xsSet(xsThis, xsID("items"), xsVar(1));
	if (xsTest(xsArg(0))) {
		c = xsToInteger(xsGet(xsArg(0), the->lengthID));
		xsSetAt(xsArg(0), xsInteger(c), xsThis);
	}
	xsSet(xsThis, xsID("linked"), xsArg(1));
}

void xsImport_target(txMachine* the)
{
	xsStringValue aString;
	xsVars(5);
	xsVar(0) = xsGet(xsGlobal, xsID("xsc"));

	xsVar(1) = xsGet(xsThis, xsID("link"));
	aString = xsToString(xsVar(1));
	if (!strcmp(aString, "dynamic"))
		xsArg(1) = xsFalse;
	else if (strcmp(aString, "static"))
		(void)xsCall2(xsVar(0), xsID("reportError"), xsThis, xsString("the link attribute must be dynamic or static"));

	xsVar(1) = xsGet(xsThis, xsID("href"));
	if (!xsTest(xsVar(1)))
		(void)xsCall2(xsVar(0), xsID("reportError"), xsThis, xsString("missing href attribute"));
	else {
		xsVar(2) = xsCall1(xsVar(0), xsID("getPath"), xsVar(1));
		if (!xsTest(xsVar(2)))
			(void)xsCall2(xsVar(0), xsID("reportError"), xsThis, xsString("file not found"));
		else {
			xsVar(3) = xsCall1(xsVar(0), xsID("loadPackage"), xsVar(2));
			if (xsTest(xsGet(xsVar(0), xsID("verbose")))) {
				if (!xsTest(xsGet(xsVar(0), xsID("make")))) {
					if (xsTest(xsVar(3))) {
						if (xsTest(xsArg(1)))
							xsVar(4) = xsCat4(xsArg(2), xsString("# Importing statically '"), xsVar(2), xsString("'..."));
						else
							xsVar(4) = xsCat4(xsArg(2), xsString("# Importing dynamically '"), xsVar(2), xsString("'..."));
					}
					else
						xsVar(4) = xsCat4(xsArg(2), xsString("# Skipping '"), xsVar(2), xsString("'..."));
					(void)xsCall1(xsVar(0), xsID("print"), xsVar(4));
				}
			}
			if (xsTest(xsVar(3)))
				(void)xsCall3(xsVar(3), xsID("target"), xsArg(0), xsArg(1), xsArg(2));
		}
	}
}

void xsNamespace_crossReference(txMachine* the)
{
	xsIntegerValue c, i;
	xsStringValue aPrefix;
	xsVars(4);
	
	xsVar(0) = xsGet(xsGlobal, xsID("xsc"));
	xsVar(1) = xsGet(xsThis, xsID("uri"));
	if (!xsTest(xsVar(1))) {
		(void)xsCall2(xsVar(0), xsID("reportError"), xsThis, xsString("missing uri attribute"));
		return;
	}
	xsVar(1) = xsGet(xsThis, xsID("prefix"));
	xsVar(2) = xsGet(xsArg(0), xsID("namespaces"));
	c = xsToInteger(xsGet(xsVar(2), the->lengthID));
	for (i = 0; i < c; i++) {
		xsVar(3) = xsGetAt(xsVar(2), xsInteger(i));
		aPrefix = xsToString(xsGet(xsVar(3), xsID("prefix")));
		if (strcmp(aPrefix, xsToString(xsVar(1))) == 0) {
			(void)xsCall2(xsVar(0), xsID("reportError"), xsThis, xsString("prefix already defined"));
			(void)xsCall2(xsVar(0), xsID("reportError"), xsVar(3), xsString("here"));
			return;
		}
	}
	xsSetAt(xsVar(2), xsInteger(c), xsThis);
}

void xsNamespace_target(txMachine* the)
{
	xsIntegerValue c;

	c = xsToInteger(xsGet(xsArg(0), the->lengthID));
	xsSetAt(xsArg(0), xsInteger(c), xsThis);
}

void xsObjectItem_codeGrammarItem(txMachine* the)
{
}

void xsObjectItem_codeFileLine(txMachine* the)
{
	xsVars(1);
	
	xsVar(0) = xsGet(xsGlobal, xsID("xsc"));
	if (xsTest(xsGet(xsVar(0), xsID("debug")))) {
		xsVar(0) = xsGet(xsThis, xsID("__xs__path"));
		xscAppendID(the, XS_FILE, xsToID(xsVar(0)));
		xsVar(0) = xsGet(xsThis, xsID("__xs__line"));
		xscAppendID(the, XS_LINE, (xsIndex)xsToInteger(xsVar(0)));
	}
}

void xsObjectItem_hasPattern(txMachine* the)
{
	xsResult = xsFalse;
}

void xsObjectItem_target(txMachine* the)
{
	xsIntegerValue c;

	c = xsToInteger(xsGet(xsArg(0), the->lengthID));
	xsSetAt(xsArg(0), xsInteger(c), xsThis);
	xsSet(xsThis, xsID("linked"), xsArg(1));
}

void xsPatch_codeGrammar(txMachine* the)
{
	xsIntegerValue c, i;
	xsVars(2);
	
	xsVar(0) = xsGet(xsThis, xsID("items"));
	c = xsToInteger(xsGet(xsVar(0), the->lengthID));
	for (i = 0; i < c; i++) 
		if (xsTest(xsCall0(xsGetAt(xsVar(0), xsInteger(i)), xsID("hasPattern"))))
			break;
	if (i < c) {
		xscAppendInteger(the, 0);
		xscAppend1(the, XS_THIS);
		xscAppendID(the, XS_GET_MEMBER_FOR_CALL, xsID("Object"));
		xscAppend1(the, XS_NEW);
		for (i = 0; i < c; i++) 
			(void)xsCall0(xsGetAt(xsVar(0), xsInteger(i)), xsID("codeGrammarItem"));
	
		(void)xsCall0(xsThis, xsID("codeFileLine"));
		xsVar(1) = xsGet(xsThis, xsID("prototype"));
		(void)xsCall0(xsVar(1), xsID("codeQualifiedName"));
		xscAppend1(the, XS_SWAP);
		xscAppendInteger(the, 2);
		xscAppend1(the, XS_THIS);
		xscAppendID(the, XS_GET_MEMBER_FOR_CALL, xsID("__xs__patch"));
		xscAppend1(the, XS_CALL);
		xscAppend1(the, XS_POP);
	}
	for (i = 0; i < c; i++) 
		(void)xsCall0(xsGetAt(xsVar(0), xsInteger(i)), xsID("codeGrammar"));
}

void xsPatch_codePrototype(txMachine* the)
{
	xsIntegerValue c, i;
	xsVars(1);
	
	xsVar(0) = xsGet(xsThis, xsID("prototype"));
	(void)xsCall0(xsVar(0), xsID("codeQualifiedName"));
	xsVar(0) = xsGet(xsThis, xsID("items"));
	c = xsToInteger(xsGet(xsVar(0), the->lengthID));
	for (i = 0; i < c; i++) 
		(void)xsCall0(xsGetAt(xsVar(0), xsInteger(i)), xsID("codePrototype"));
	xscAppend1(the, XS_POP);
}

void xsPatch_crossReference(txMachine* the)
{
	xsIntegerValue c, i;
	xsVars(2);
	
	xsVar(0) = xsGet(xsGlobal, xsID("xsc"));
	xsVar(1) = xsGet(xsThis, xsID("prototype"));
	xsArg(1) = xsCat2(xsVar(1), xsString("."));
	if (xsTest(xsVar(1))) {
		xsVar(1) = xsCall1(xsVar(0), xsID("searchProperty"), xsVar(1));
		if (!xsTest(xsVar(1)))
			(void)xsCall2(xsVar(0), xsID("reportError"), xsThis, xsString("prototype does not exist"));
		else if (!xsIsInstanceOf(xsVar(1), xsGet(xsGlobal, xsID("xsObject"))))
			(void)xsCall2(xsVar(0), xsID("reportError"), xsThis, xsString("prototype is no object"));
		xsSet(xsThis, xsID("prototype"), xsVar(1));
	}
	else
		(void)xsCall2(xsVar(0), xsID("reportError"), xsThis, xsString("missing prototype attribute"));
	xsVar(0) = xsGet(xsThis, xsID("items"));
	c = xsToInteger(xsGet(xsVar(0), the->lengthID));
	for (i = 0; i < c; i++) 
		(void)xsCall3(xsGetAt(xsVar(0), xsInteger(i)), xsID("crossReference"), xsArg(0), xsArg(1), xsTrue);
}

void xsPatch_target(txMachine* the)
{
	xsIntegerValue c, i;
	xsVars(2);
	
	xsVar(0) = xsGet(xsThis, xsID("items"));
	xsVar(1) = xsNewInstanceOf(xsArrayPrototype);
	if (xsTest(xsVar(0))) {
		c = xsToInteger(xsGet(xsVar(0), the->lengthID));
		for (i = 0; i < c; i++) 
			(void)xsCall3(xsGetAt(xsVar(0), xsInteger(i)), xsID("target"), xsVar(1), xsArg(1), xsArg(2));
	}
	xsSet(xsThis, xsID("items"), xsVar(1));
	c = xsToInteger(xsGet(xsArg(0), the->lengthID));
	xsSetAt(xsArg(0), xsInteger(c), xsThis);
	xsSet(xsThis, xsID("linked"), xsArg(1));
}

void xsProgram_codePrototype(txMachine* the)
{
	xsIntegerValue i;
	xsVars(2);
	
	xsResult = xsGet(xsThis, xsID("value"));
	xsVar(0) = xsGet(xsGlobal, xsID("xsc"));
	xsVar(1) = xsGet(xsThis, xsID("c"));
	if (xsTest(xsVar(1))) {
		i = xsToInteger(xsCall3(xsVar(0), xsID("addHostFunction"), xsVar(1), xsString(""), xsResult));
		xscAppendInteger(the, 0);
		xscAppend1(the, PSEUDO_GET_MEMBER);
		xscAppendID(the, XS_GET_MEMBER, xsID("@"));
		xscAppendInteger(the, i);
		xscAppend1(the, XS_GET_MEMBER_AT);
		xscAppend1(the, XS_CALL);
		xscAppend1(the, XS_POP);
	}
	else {
		xscAppendFunction(the);
		xscAppend1(the, XS_THIS);
		xscAppendID(the, XS_SET_MEMBER, xsID("@script"));
		xscAppend1(the, XS_POP);
		xscAppendInteger(the, 0);
		xscAppend1(the, XS_THIS);
		xscAppendID(the, XS_GET_MEMBER_FOR_CALL, xsID("@script"));
		xscAppend1(the, XS_CALL);
		xscAppend1(the, XS_POP);
		xscAppend1(the, XS_THIS);
		xscAppendID(the, XS_DELETE_MEMBER, xsID("@script"));
		xscAppend1(the, XS_POP);
	}
}

void xsProgram_crossReference(txMachine* the)
{
	xsVars(3);
	
	xsVar(0) = xsGet(xsGlobal, xsID("xsc"));
	if (xsTest(xsGet(xsThis, xsID("linked")))) {
		xsIntegerValue code = 0;
		if (xsTest(xsCall1(xsThis, xsID("hasOwnProperty"), xsString("c")))) {
			xsVar(1) = xsGet(xsThis, xsID("c"));
			if (!xsTest(xsVar(1)) || !strncmp(xsToString(xsVar(1)), "false", 5))
				code = 1;
			else if (!strncmp(xsToString(xsVar(1)), "true ", 4))
				code = 2;
			else if (xsTest(xsCall1(xsThis, xsID("hasOwnProperty"), xsString("value"))))
				(void)xsCall2(xsVar(0), xsID("reportError"), xsThis, xsString("a program element with a c attribute must be empty"));
			else if (xsTest(xsCall1(xsThis, xsID("hasOwnProperty"), xsString("href"))))
				(void)xsCall2(xsVar(0), xsID("reportError"), xsThis, xsString("a program element cannot have both c and href attributes"));
		}
		else {
			xsVar(1) = xsGet(xsArg(0), xsID("cFlag"));
			if (!xsTest(xsVar(1)) || !strncmp(xsToString(xsVar(1)), "false", 5))
				code = 1;
			else
				code = 2;
		}
		if (code) {
			if (xsTest(xsCall1(xsThis, xsID("hasOwnProperty"), xsString("href")))) {
				if (xsTest(xsCall1(xsThis, xsID("hasOwnProperty"), xsString("value"))))
					(void)xsCall2(xsVar(0), xsID("reportError"), xsThis, xsString("a program element with an href attribute must be empty"));
				else {
					xsVar(1) = xsGet(xsThis, xsID("href"));
					xsVar(1) = xsCall1(xsVar(0), xsID("getPath"), xsVar(1));
					if (!xsTest(xsVar(1)))
						(void)xsCall2(xsVar(0), xsID("reportError"), xsThis, xsString("file not found"));
					else
						xsVar(2) = xsCall2(xsVar(0), xsID("getProgramCode"), xsVar(1), xsBoolean(code == 2));
				}
			}
			else {
				xsVar(1) = xsGet(xsThis, xsID("value"));
				xsVar(2) = xsCall4(xsVar(0), xsID("getStringCode"), xsVar(1), xsThis, xsFalse, xsBoolean(code == 2));
			}
			xsSet(xsThis, xsID("value"), xsVar(2));
			if (code == 2) {
				xsVar(2) = xsGet(xsGlobal, xsID("cProgramIndex"));
				xsSet(xsGlobal, xsID("cProgramIndex"), xsInteger(xsToInteger(xsVar(2)) + 1));
				xsVar(2) = xsCall1(xsString("xsc_program_"), xsID("concat"), xsVar(2));
				xsSet(xsThis, xsID("c"), xsVar(2));
			}
			else
				xsDelete(xsThis, xsID("c"));
		}
	}
}

void xsProperty_codeGrammar(txMachine* the)
{
	xsVars(1);
	
	xsVar(0) = xsGet(xsThis, xsID("pattern"));
	if (xsTest(xsVar(0))) {
		if (xsTest(xsGet(xsVar(0), xsID("rootFlag")))) {
			xsVar(0) = xsGet(xsGlobal, xsID("xsc"));
			(void)xsCall2(xsVar(0), xsID("reportError"), xsThis, xsString("root pattern but no object"));
		}
	}
}

void xsProperty_codeGrammarItem(txMachine* the)
{
	xsVars(1);
	
	if (xsTest(xsGet(xsThis, xsID("pattern")))) {
		xscAppend1(the, XS_DUB);
		(void)xsCall0(xsThis, xsID("codeGrammarPattern"));
		xscAppend1(the, XS_SWAP);
		xsVar(0) = xsGet(xsThis, xsID("name"));
		xscAppendID(the, XS_SET_MEMBER, xsToID(xsVar(0)));
		xscAppend1(the, XS_POP);
	}
}

void xsProperty_codeGrammarPattern(txMachine* the)
{
	xsDebugger();
}

void xsProperty_codePrototypeAssignment(txMachine* the)
{
	xsVars(3);
	
	xsVar(0) = xsGet(xsThis, xsID("name"));
	xsVar(1) = xsGet(xsThis, xsID("masks"));
	xsVar(2) = xsGet(xsThis, xsID("flags"));
	xscAppendID(the, XS_PUT_MEMBER, xsToID(xsVar(0)));
	xscAppend1(the, (txByte)xsToInteger(xsVar(1)));
	xscAppend1(the, (txByte)xsToInteger(xsVar(2)));
}

void xsProperty_codeQualifiedName(txMachine* the)
{
	xsIntegerValue c, i;
	xsVars(1);

	xscAppend1(the, XS_THIS);
	xsVar(0) = xsGet(xsThis, xsID("qualifiedName"));
	xsVar(0) = xsCall1(xsVar(0), xsID("split"), xsString("."));
	c = xsToInteger(xsGet(xsVar(0), the->lengthID));
	for (i = 0; i < c; i++) 
		xscAppendID(the, XS_GET_MEMBER, xsToID(xsGetAt(xsVar(0), xsInteger(i))));
}

void xsProperty_crossReference(txMachine* the)
{
	xsIntegerValue masks, flags;
	xsVars(4); /* see instances */ 
	xsVar(0) = xsGet(xsGlobal, xsID("xsc"));
	
	xsVar(1) = xsGet(xsThis, xsID("name"));
	if (xsTest(xsVar(1)))
		(void)xsCall3(xsVar(0), xsID("insertProperty"), xsCat2(xsArg(1), xsVar(1)), xsThis, xsArg(2));
	else
		(void)xsCall2(xsVar(0), xsID("reportError"), xsThis, xsString("missing name attribute"));
	if (xsTest(xsCall1(xsThis, xsID("hasOwnProperty"), xsString("pattern")))) {
		xsVar(1) = xsGet(xsThis, xsID("pattern"));
		xsVar(1) = xsCall2(xsVar(0), xsID("makePattern"), xsArg(0), xsThis);
		if (xsTypeOf(xsVar(1)) != xsUndefinedType)
			xsSet(xsThis, xsID("pattern"), xsVar(1));
		else
			(void)xsCall2(xsVar(0), xsID("reportError"), xsThis, xsString("invalid pattern"));
	}
	else
		xsSet(xsThis, xsID("pattern"), xsNull);
		
	if (!xsTest(xsCall1(xsThis, xsID("hasOwnProperty"), xsString("deleteFlag")))) {
		xsVar(1) = xsGet(xsArg(0), xsID("deleteFlag"));
		xsSet(xsThis, xsID("deleteFlag"), xsVar(1));
	}
	if (!xsTest(xsCall1(xsThis, xsID("hasOwnProperty"), xsString("enumFlag")))) {
		xsVar(1) = xsGet(xsArg(0), xsID("enumFlag"));
		xsSet(xsThis, xsID("enumFlag"), xsVar(1));
	}
	if (!xsTest(xsCall1(xsThis, xsID("hasOwnProperty"), xsString("scriptFlag")))) {
		xsVar(1) = xsGet(xsArg(0), xsID("scriptFlag"));
		xsSet(xsThis, xsID("scriptFlag"), xsVar(1));
	}
	if (!xsTest(xsCall1(xsThis, xsID("hasOwnProperty"), xsString("setFlag")))) {
		xsVar(1) = xsGet(xsArg(0), xsID("setFlag"));
		xsSet(xsThis, xsID("setFlag"), xsVar(1));
	}
	
	masks = XS_DONT_SCRIPT_FLAG;
	flags = XS_DONT_SCRIPT_FLAG;
	xsVar(1) = xsGet(xsThis, xsID("deleteFlag"));
	if (xsTest(xsVar(1))) {
		masks |= XS_DONT_DELETE_FLAG;
		if (!strcmp(xsToString(xsVar(1)), "false"))
			flags |= XS_DONT_DELETE_FLAG;
	}
	xsVar(1) = xsGet(xsThis, xsID("enumFlag"));
	if (xsTest(xsVar(1))) {
		masks |= XS_DONT_ENUM_FLAG;
		if (!strcmp(xsToString(xsVar(1)), "false"))
			flags |= XS_DONT_ENUM_FLAG;
	}
	xsVar(1) = xsGet(xsThis, xsID("scriptFlag"));
	if (xsTest(xsVar(1))) {
		masks |= XS_DONT_SCRIPT_FLAG;
		if (!strcmp(xsToString(xsVar(1)), "true")) {
			flags &= ~XS_DONT_SCRIPT_FLAG;
			/* xsCall2(xsVar(0), xsID("reportWarning"), xsThis, xsString("scriptable property")); */
		}
	}
	xsVar(1) = xsGet(xsThis, xsID("setFlag"));
	if (xsTest(xsVar(1))) {
		masks |= XS_DONT_SET_FLAG;
		if (!strcmp(xsToString(xsVar(1)), "false"))
			flags |= XS_DONT_SET_FLAG;
	}
	xsSet(xsThis, xsID("masks"), xsInteger(masks));
	xsSet(xsThis, xsID("flags"), xsInteger(flags));
}

void xsProperty_forceDefaultPattern(txMachine* the)
{
	xsIntegerValue c;
	xsVars(2);

	xsVar(0) = 	xsGet(xsThis, xsID("pattern"));
	if (!xsTest(xsGet(xsVar(0), xsID("defaultFlag")))) {
		xsSet(xsVar(0), xsID("defaultFlag"), xsTrue);
		xsVar(1) = xsNew0(xsGlobal, xsID("Object"));
		xsSet(xsVar(1), xsID("namespace"), xsNull);
		xsSet(xsVar(1), xsID("name"), xsString("."));
		c = xsToInteger(xsGet(xsVar(0), the->lengthID));
		xsSetAt(xsVar(0), xsInteger(c), xsVar(1));
	}
}

void xsProperty_hasPattern(txMachine* the)
{
	xsVars(1);

	xsVar(0) = xsGet(xsThis, xsID("pattern"));
	xsResult = (xsTest(xsVar(0))) ? xsTrue : xsFalse;
}

void xsProperty_insertProperty(txMachine* the)
{
	xsStringValue aString;
	xsVars(1);

	aString = xsToString(xsGet(xsThis, xsID("qualifiedName")));
	if (!strcmp(aString, xsToString(xsArg(0)))) {
		if (xsTest(xsArg(2)))
			xsSet(xsArg(1), xsID("qualifiedName"), xsArg(0));
		else {
			xsVar(0) = xsGet(xsGlobal, xsID("xsc"));
			(void)xsCall2(xsVar(0), xsID("reportError"), xsArg(1), xsString("name already defined"));
			(void)xsCall2(xsVar(0), xsID("reportError"), xsThis, xsString("here"));
		}
	}
	else {
		xsVar(0) = xsGet(xsThis, xsID("nextProperty"));
		if (xsTest(xsVar(0)))
			xsResult = xsCall3(xsVar(0), xsID("insertProperty"), xsArg(0), xsArg(1), xsArg(2));
		else {
			xsSet(xsThis, xsID("nextProperty"), xsArg(1));
			xsSet(xsArg(1), xsID("qualifiedName"), xsArg(0));
		}
	}
}

void xsProperty_searchProperty(txMachine* the)
{
	xsStringValue aString;
	xsVars(1);
	
	aString = xsToString(xsGet(xsThis, xsID("qualifiedName")));
	if (!strcmp(aString, xsToString(xsArg(0))))
		xsResult = xsThis;
	else {
		xsVar(0) = xsGet(xsThis, xsID("nextProperty"));
		if (xsTest(xsVar(0)))
			xsResult = xsCall1(xsVar(0), xsID("searchProperty"), xsArg(0));
	}
}

void xsLiteral_codeGrammarIO(txMachine* the)
{
	xsDebugger();
}

void xsLiteral_codeGrammarPattern(txMachine* the)
{
	xsVars(4); // xscAppendPattern

	xsVar(0) = xsGet(xsGlobal, xsID("xsc"));
	xsVar(1) = 	xsGet(xsThis, xsID("pattern"));
	(void)xsCall0(xsThis, xsID("codeFileLine"));
	if (xsTest(xsGet(xsVar(1), xsID("skipFlag")))) 
		xscAppend1(the, XS_NULL);
	else if (xsTest(xsGet(xsVar(1), xsID("attributeFlag")))) {
		(void)xsCall0(xsThis, xsID("codeGrammarIO"));
		xscAppendPattern(the, XS_ATTRIBUTE_PATTERN);
	}
	else if (xsTest(xsGet(xsVar(1), xsID("piFlag")))) {
		(void)xsCall0(xsThis, xsID("codeGrammarIO"));
		xscAppendPattern(the, XS_PI_PATTERN);
	}
	else {
		(void)xsCall0(xsThis, xsID("forceDefaultPattern"));
		(void)xsCall0(xsThis, xsID("codeGrammarIO"));
		xscAppendPattern(the, XS_DATA_PATTERN);
	}
}

void xsLiteral_codePrototype(txMachine* the)
{
	xscAppend1(the, XS_DUB);
	(void)xsCall0(xsThis, xsID("codeFileLine"));
	xscAppendString(the, xsToString(xsGet(xsThis, xsID("value"))));
	xscAppendInteger(the, 1);
	(void)xsCall0(xsThis, xsID("codeGrammarIO"));
	xscAppendID(the, XS_GET_MEMBER_FOR_CALL, xsID("parse"));
	xscAppend1(the, XS_CALL);
	xscAppend1(the, XS_SWAP);
	(void)xsCall0(xsThis, xsID("codePrototypeAssignment"));
	xscAppend1(the, XS_POP);
}

void xsBoolean_codeGrammarIO(txMachine* the)
{
	xscAppend1(the, XS_THIS);
	xscAppendID(the, XS_GET_MEMBER, xsID("__xs__boolean"));
}

void xsChunk_codeGrammarIO(txMachine* the)
{
	xscAppend1(the, XS_THIS);
	xscAppendID(the, XS_GET_MEMBER, xsID("__xs__chunk"));
}

void xsCustom_codeGrammarIO(txMachine* the)
{
	(void)xsCall0(xsGet(xsThis, xsID("io")), xsID("codeQualifiedName"));
}

void xsCustom_crossReference(txMachine* the)
{
	xsProperty_crossReference(the);
	xsVar(1) = xsGet(xsThis, xsID("io"));
	if (xsTest(xsVar(1))) {
		xsVar(1) = xsCall1(xsVar(0), xsID("searchProperty"), xsVar(1));
		if (!xsTest(xsVar(1)))
			(void)xsCall2(xsVar(0), xsID("reportError"), xsThis, xsString("io does not exist"));
		else if (!xsIsInstanceOf(xsVar(1), xsGet(xsGlobal, xsID("xsObject"))))
			(void)xsCall2(xsVar(0), xsID("reportError"), xsThis, xsString("io is no object"));
		xsSet(xsThis, xsID("io"), xsVar(1));
	}
	else
		(void)xsCall2(xsVar(0), xsID("reportError"), xsThis, xsString("missing io attribute"));
}

void xsDate_codeGrammarIO(txMachine* the)
{
	xscAppend1(the, XS_THIS);
	xscAppendID(the, XS_GET_MEMBER, xsID("__xs__date"));
}

void xsFunction_codePrototype(txMachine* the)
{
	xsIntegerValue i, j, k;
	xsStringValue s;
	xsVars(5);
	
	xsResult = xsGet(xsThis, xsID("value"));
	xsVar(0) = xsGet(xsGlobal, xsID("xsc"));
	xsVar(1) = xsGet(xsThis, xsID("c"));
	xsVar(2) = xsGet(xsThis, xsID("check"));
	xsVar(3) = xsGet(xsThis, xsID("prototype"));
	xsVar(4) = xsGet(xsThis, xsID("params"));
	
	xscAppend1(the, XS_DUB);
	if (xsTest(xsVar(1))) {
		if (xsTest(xsVar(3)))
			i = xsToInteger(xsCall3(xsVar(0), xsID("addHostConstructor"), xsVar(1), xsVar(4), xsResult));
		else
			i = xsToInteger(xsCall3(xsVar(0), xsID("addHostFunction"), xsVar(1), xsVar(4), xsResult));
		xscAppend1(the, PSEUDO_GET_MEMBER);
		xscAppendID(the, XS_GET_MEMBER, xsID("@"));
		xscAppendInteger(the, i);
		xscAppend1(the, XS_GET_MEMBER_AT);
	}
	else if (xsTest(xsVar(2))) {
		i = xsToInteger(xsGet(xsResult, xsID("size")));
		s = xsToString(xsVar(2));
		j = c_strlen(s);
		k = i + j + 4 + stringCommandOffset*2;
		if (k > 0x7FFF)
			(void)xsCall2(xsVar(0), xsID("reportError"), xsThis, xsString("the function is too big to be checked!"));
		xscAppend1(the, XS_FUNCTION);
		xscAppend2(the, (txID)k);
		xsResult = xsGet(xsResult, xsID("code"));
		xscAppend(the, xsResult.value.code, i);
		xscAppendString(the, "");
		xscAppendString(the, s);
	}
	else {
		xscAppendFunction(the);
	}
	if (xsTest(xsVar(3))) {
		xscAppend1(the, XS_DUB);
		(void)xsCall0(xsVar(3), xsID("codeQualifiedName"));
		xscAppend1(the, XS_SWAP);
		xscAppendID(the, XS_SET_MEMBER, xsID("prototype"));
		xscAppend1(the, XS_POP);
	}
	xscAppend1(the, XS_SWAP);
	(void)xsCall0(xsThis, xsID("codePrototypeAssignment"));
	xscAppend1(the, XS_POP);
}

void xsFunction_crossReference(txMachine* the)
{
	xsIntegerValue masks, flags;
	
	xsProperty_crossReference(the);
	xsVar(1) = xsGet(xsThis, xsID("name"));
	masks = xsToInteger(xsGet(xsThis, xsID("masks")));
	flags = xsToInteger(xsGet(xsThis, xsID("flags")));
	if (!strncmp(xsToString(xsVar(1)), "get ", 4)) {
		xsVar(1) = xsCall2(xsVar(1), xsID("substring"), xsInteger(4), xsInteger(0x7FFFFFFF));
		xsSet(xsThis, xsID("name"), xsVar(1));
		masks |= XS_GETTER_FLAG;
		flags |= XS_GETTER_FLAG;
	}
	else if (!strncmp(xsToString(xsVar(1)), "set ", 4)) {
		xsVar(1) = xsCall2(xsVar(1), xsID("substring"), xsInteger(4), xsInteger(0x7FFFFFFF));
		xsSet(xsThis, xsID("name"), xsVar(1));
		masks |= XS_SETTER_FLAG;
		flags |= XS_SETTER_FLAG;
	}
	xsSet(xsThis, xsID("masks"), xsInteger(masks));
	xsSet(xsThis, xsID("flags"), xsInteger(flags));
	
	if (xsTest(xsGet(xsThis, xsID("linked")))) {
		xsIntegerValue code = 0;
		if (xsTest(xsCall1(xsThis, xsID("hasOwnProperty"), xsString("c")))) {
			xsVar(1) = xsGet(xsThis, xsID("c"));
			if (!xsTest(xsVar(1)) || !strncmp(xsToString(xsVar(1)), "false", 5))
				code = 1;
			else if (!strncmp(xsToString(xsVar(1)), "true", 4))
				code = 2;
			else if (xsTest(xsCall1(xsThis, xsID("hasOwnProperty"), xsString("value"))))
				(void)xsCall2(xsVar(0), xsID("reportError"), xsThis, xsString("a function element with a c attribute must be empty"));
			else if (xsTest(xsCall1(xsThis, xsID("hasOwnProperty"), xsString("check"))))
				(void)xsCall2(xsVar(0), xsID("reportError"), xsThis, xsString("a function element with a c attribute cannot be checked"));
		}
		else {
			xsVar(1) = xsGet(xsArg(0), xsID("cFlag"));
			if (!xsTest(xsVar(1)) || !strncmp(xsToString(xsVar(1)), "false", 5))
				code = 1;
			else
				code = 2;
		}
		if (code) {
			xsVar(1) = xsCat5(xsString("("), xsGet(xsThis, xsID("params")), xsString("){"), xsGet(xsThis, xsID("value")), xsString("}"));
			xsVar(2) = xsCall4(xsVar(0), xsID("getStringCode"), xsVar(1), xsThis, xsTrue, xsBoolean(code == 2));
			xsSet(xsThis, xsID("value"), xsVar(2));
			if (code == 2) {
				char c, *p;
				xsVar(2) = xsGet(xsThis, xsID("qualifiedName"));
				xsVar(2) = xsCall1(xsString("xsc_"), xsID("concat"), xsVar(2));
				p = xsToString(xsVar(2));
				while ((c = *p)) {
					if ((('0' <= c) && (c <= '9')) || (('A' <= c) && (c <= 'Z')) || (('a' <= c) && (c <= 'z'))) 
						*p = c;
					else
						*p = '_';
					p++;
				}
				xsSet(xsThis, xsID("c"), xsVar(2));
			}
			else
				xsDelete(xsThis, xsID("c"));
		}
	}
	
	xsVar(1) = xsGet(xsThis, xsID("prototype"));
	if (xsTest(xsVar(1))) {
		if (flags & XS_GETTER_FLAG)
			(void)xsCall2(xsVar(0), xsID("reportError"), xsThis, xsString("a getter has no prototype"));
		else if (flags & XS_SETTER_FLAG)
			(void)xsCall2(xsVar(0), xsID("reportError"), xsThis, xsString("a setter has no prototype"));
		else {
			xsVar(1) = xsCall1(xsVar(0), xsID("searchProperty"), xsVar(1));
			if (!xsTest(xsVar(1)))
				(void)xsCall2(xsVar(0), xsID("reportError"), xsThis, xsString("prototype does not exist"));
			else if (!xsIsInstanceOf(xsVar(1), xsGet(xsGlobal, xsID("xsObject"))))
				(void)xsCall2(xsVar(0), xsID("reportError"), xsThis, xsString("prototype is no object"));
			xsSet(xsThis, xsID("prototype"), xsVar(1));
		}
	}
}
void xsNull_codePrototype(txMachine* the)
{
	xscAppend1(the, XS_DUB);
	(void)xsCall0(xsThis, xsID("codeFileLine"));
	xscAppend1(the, XS_NULL);
	xscAppend1(the, XS_SWAP);
	(void)xsCall0(xsThis, xsID("codePrototypeAssignment"));
	xscAppend1(the, XS_POP);
}

void xsNumber_codeGrammarIO(txMachine* the)
{
	xscAppend1(the, PSEUDO_GET_MEMBER);
	xscAppendID(the, XS_GET_MEMBER, xsID("__xs__number"));
}

void xsRegExp_codeGrammarIO(txMachine* the)
{
	xscAppend1(the, PSEUDO_GET_MEMBER);
	xscAppendID(the, XS_GET_MEMBER, xsID("__xs__regexp"));
}

void xsRegExp_codePrototype(txMachine* the)
{
	xscAppend1(the, XS_DUB);
	(void)xsCall0(xsThis, xsID("codeFileLine"));
	xscAppendString(the, xsToString(xsGet(xsThis, xsID("value"))));
	xscAppendInteger(the, 1);
	(void)xsCall0(xsThis, xsID("codeGrammarIO"));
	xscAppendID(the, XS_GET_MEMBER_FOR_CALL, xsID("parse"));
	xscAppend1(the, XS_CALL);
	xscAppend1(the, XS_ALIAS);
	xscAppend1(the, XS_SWAP);
	(void)xsCall0(xsThis, xsID("codePrototypeAssignment"));
	xscAppend1(the, XS_POP);
}

void xsString_codeGrammarIO(txMachine* the)
{
	xscAppend1(the, PSEUDO_GET_MEMBER);
	xscAppendID(the, XS_GET_MEMBER, xsID("__xs__string"));
}

void xsUndefined_codePrototype(txMachine* the)
{
	xscAppend1(the, XS_DUB);
	(void)xsCall0(xsThis, xsID("codeFileLine"));
	xscAppend1(the, XS_UNDEFINED);
	xscAppend1(the, XS_SWAP);
	(void)xsCall0(xsThis, xsID("codePrototypeAssignment"));
	xscAppend1(the, XS_POP);
}

void xsReference_codeGrammarPattern(txMachine* the)
{
	xsIntegerValue c, i;
	xsVars(4);
	
	xsVar(0) = xsGet(xsGlobal, xsID("xsc"));
	xsVar(1) = xsGet(xsThis, xsID("pattern"));
	xsVar(2) = xsGet(xsThis, xsID("contents"));
	if (xsTest(xsGet(xsVar(1), xsID("skipFlag")))) 
		xscAppend1(the, XS_NULL);
	else if (xsTest(xsGet(xsVar(1), xsID("attributeFlag")))) {
		(void)xsCall2(xsVar(0), xsID("reportError"), xsArg(1), xsString("@ pattern in reference"));
	}
	else if (xsTest(xsGet(xsVar(1), xsID("piFlag")))) {
		(void)xsCall2(xsVar(0), xsID("reportError"), xsArg(1), xsString("? pattern in reference"));
	}
	else {
		(void)xsCall0(xsThis, xsID("forceDefaultPattern"));
		xscAppendInteger(the, 0);
		xscAppend1(the, XS_THIS);
		xscAppendID(the, XS_GET_MEMBER_FOR_CALL, xsID("Object"));
		xscAppend1(the, XS_NEW);
		
		c = xsToInteger(xsGet(xsVar(2), the->lengthID));
		for (i = 0; i < c; i++) {
			xscAppend1(the, XS_DUB);
			xsVar(3) = xsGetAt(xsVar(2), xsInteger(i));
			if (xsIsInstanceOf(xsVar(3), xsGet(xsGlobal, xsID("xsObject"))))
				(void)xsCall0(xsVar(3), xsID("codeQualifiedName"));
			else
				(void)xsCall0(xsVar(3), xsID("codeGrammarPattern"));
			xscAppend1(the, XS_SWAP);
			xscAppendInteger(the, i);
			xscAppend1(the, XS_SET_MEMBER_AT);
			xscAppend1(the, XS_POP);
		}
		xscAppend1(the, XS_ALIAS);
		(void)xsCall0(xsThis, xsID("codeFileLine"));
		xscAppendPattern(the, XS_REFER_PATTERN);
	}
}

void xsReference_codePrototype(txMachine* the)
{
	xsVars(1);
	
	xscAppend1(the, XS_DUB);
	(void)xsCall0(xsThis, xsID("codeFileLine"));
	xsVar(0) = xsGet(xsThis, xsID("contents"));
	xsVar(0) = xsGet(xsVar(0), 0);
	(void)xsCall0(xsVar(0), xsID("codeQualifiedName"));
	xscAppend1(the, XS_SWAP);
	(void)xsCall0(xsThis, xsID("codePrototypeAssignment"));
	xscAppend1(the, XS_POP);
}

void xsReference_crossReference(txMachine* the)
{
	xsIntegerValue c, i;
	
	xsProperty_crossReference(the);
	xsVar(1) = xsGet(xsThis, xsID("contents"));
	if (xsTest(xsVar(1))) {
		xsVar(1) = xsCall1(xsVar(1), xsID("split"), xsString(","));
		c = xsToInteger(xsGet(xsVar(1), the->lengthID));
		for (i = 0; i < c; i++) {
			xsVar(2) = xsGetAt(xsVar(1), xsInteger(i));
			xsVar(2) = xsCall1(xsVar(0), xsID("trimString"), xsVar(2));
			xsVar(3) = xsCall1(xsVar(0), xsID("searchProperty"), xsVar(2));
			if (!xsTest(xsVar(3))) {
				xsVar(2) = xsCat2(xsVar(2), xsString(" does not exist"));
				(void)xsCall2(xsVar(0), xsID("reportError"), xsThis, xsVar(2));
			}
			else if (xsIsInstanceOf(xsVar(3), xsGet(xsGlobal, xsID("xsReference")))) {
				xsVar(2) = xsCat2(xsVar(2), xsString(" is a reference"));
				(void)xsCall2(xsVar(0), xsID("reportError"), xsThis, xsVar(2));
			}
			else if (xsIsInstanceOf(xsVar(3), xsGet(xsGlobal, xsID("xsReference")))) {
				xsVar(2) = xsCat2(xsVar(2), xsString(" is an array"));
				(void)xsCall2(xsVar(0), xsID("reportError"), xsThis, xsVar(2));
			}
			else if (xsIsInstanceOf(xsVar(3), xsGet(xsGlobal, xsID("xsObject")))) {
				xsSetAt(xsVar(1), xsInteger(i), xsVar(3));
			}
			else if (!xsTest(xsGet(xsVar(3), xsID("pattern")))) {
				xsVar(2) = xsCat2(xsVar(2), xsString(" has no pattern"));
				(void)xsCall2(xsVar(0), xsID("reportError"), xsThis, xsVar(2));
			}
			else
				xsSetAt(xsVar(1), xsInteger(i), xsVar(3));
		}
		xsSet(xsThis, xsID("contents"), xsVar(1));
	}
	else
		(void)xsCall2(xsVar(0), xsID("reportError"), xsThis, xsString("missing contents attribute"));
}

void xsArray_codeGrammarPattern(txMachine* the)
{
	xsIntegerValue c, i;
	xsVars(4);
	
	xsVar(0) = xsGet(xsGlobal, xsID("xsc"));
	xsVar(1) = xsGet(xsThis, xsID("pattern"));
	xsVar(2) = xsGet(xsThis, xsID("contents"));
	if (xsTest(xsGet(xsVar(1), xsID("skipFlag")))) 
		xscAppend1(the, XS_NULL);
	else if (xsTest(xsGet(xsVar(1), xsID("attributeFlag")))) {
		(void)xsCall2(xsVar(0), xsID("reportError"), xsArg(1), xsString("@ pattern in array"));
	}
	else if (xsTest(xsGet(xsVar(1), xsID("piFlag")))) {
		(void)xsCall2(xsVar(0), xsID("reportError"), xsArg(1), xsString("? pattern in array"));
	}
	else {
		(void)xsCall0(xsThis, xsID("forceDefaultPattern"));
		xscAppendInteger(the, 0);
		xscAppend1(the, XS_THIS);
		xscAppendID(the, XS_GET_MEMBER_FOR_CALL, xsID("Object"));
		xscAppend1(the, XS_NEW);
		
		c = xsToInteger(xsGet(xsVar(2), the->lengthID));
		for (i = 0; i < c; i++) {
			xscAppend1(the, XS_DUB);
			xsVar(3) = xsGetAt(xsVar(2), xsInteger(i));
			if (xsIsInstanceOf(xsVar(3), xsGet(xsGlobal, xsID("xsObject"))))
				(void)xsCall0(xsVar(3), xsID("codeQualifiedName"));
			else
				(void)xsCall0(xsVar(3), xsID("codeGrammarPattern"));
			xscAppend1(the, XS_SWAP);
			xscAppendInteger(the, i);
			xscAppend1(the, XS_SET_MEMBER_AT);
			xscAppend1(the, XS_POP);
		}
		xscAppend1(the, XS_ALIAS);
		(void)xsCall0(xsThis, xsID("codeFileLine"));
		xscAppendPattern(the, XS_REPEAT_PATTERN);
	}
}

void xsArray_codePrototype(txMachine* the)
{
	xscAppend1(the, XS_DUB);
	(void)xsCall0(xsThis, xsID("codeFileLine"));
	xscAppendInteger(the, 0);
	xscAppend1(the, XS_THIS);
	xscAppendID(the, XS_GET_MEMBER_FOR_CALL, xsID("Array"));
	xscAppend1(the, XS_NEW);
	xscAppend1(the, XS_SWAP);
	(void)xsCall0(xsThis, xsID("codePrototypeAssignment"));
	xscAppend1(the, XS_POP);
}

void xsObject_codeGrammar(txMachine* the)
{
	xsIntegerValue c, i;
	xsVars(4);

	if (xsTest(xsGet(xsThis, xsID("grammarFlag")))) 
		return;
	xsSet(xsThis, xsID("grammarFlag"), xsTrue);
	
	xsVar(0) = xsGet(xsThis, xsID("prototype"));
	if (xsTest(xsVar(0)))
		if (xsTest(xsGet(xsVar(0), xsID("linked"))))
			(void)xsCall0(xsVar(0), xsID("codeGrammar"));
	
	xsVar(0) = xsGet(xsThis, xsID("pattern"));
	if (xsTest(xsVar(0)))
		if (xsTest(xsGet(xsVar(0), xsID("rootFlag")))) {
			(void)xsCall0(xsThis, xsID("codeQualifiedName"));
			xscAppendInteger(the, 1);
			xscAppend1(the, XS_THIS);
			xscAppendID(the, XS_GET_MEMBER_FOR_CALL, xsID("__xs__root"));
			xscAppend1(the, XS_CALL);
			xscAppend1(the, XS_POP);
		}
		
	xscAppendInteger(the, 0);
	xscAppend1(the, XS_THIS);
	xscAppendID(the, XS_GET_MEMBER_FOR_CALL, xsID("Object"));
	xscAppend1(the, XS_NEW);
	xscAppend1(the, XS_DUB);
	if (xsTest(xsVar(0)))
		(void)xsCall0(xsThis, xsID("codeGrammarPattern"));
	else
		xscAppend1(the, XS_NULL);
	xscAppend1(the, XS_SWAP);
	xscAppendID(the, XS_SET_MEMBER, XS_NO_ID);
	xscAppend1(the, XS_POP);
		
	xsVar(0) = xsGet(xsThis, xsID("items"));
	c = xsToInteger(xsGet(xsVar(0), the->lengthID));
	for (i = 0; i < c; i++) 
		(void)xsCall0(xsGetAt(xsVar(0), xsInteger(i)), xsID("codeGrammarItem"));
		
	(void)xsCall0(xsThis, xsID("codeFileLine"));
	(void)xsCall0(xsThis, xsID("codeQualifiedName"));
	xscAppend1(the, XS_SWAP);
	xscAppendInteger(the, 2);
	xscAppend1(the, XS_THIS);
	xscAppendID(the, XS_GET_MEMBER_FOR_CALL, xsID("__xs__pattern"));
	xscAppend1(the, XS_CALL);
	xscAppend1(the, XS_POP);
	
	for (i = 0; i < c; i++) 
		(void)xsCall0(xsGetAt(xsVar(0), xsInteger(i)), xsID("codeGrammar"));
}

void xsObject_codeGrammarPattern(txMachine* the)
{
	xsVars(4);
	xsVar(0) = xsGet(xsGlobal, xsID("xsc"));
	xsVar(1) = xsGet(xsThis, xsID("pattern"));

	if (xsTest(xsGet(xsVar(1), xsID("skipFlag")))) 
		xscAppend1(the, XS_NULL);
	else if (xsTest(xsGet(xsVar(1), xsID("attributeFlag")))) {
		(void)xsCall2(xsVar(0), xsID("reportError"), xsArg(1), xsString("@ pattern in object"));
	}
	else if (xsTest(xsGet(xsVar(1), xsID("piFlag")))) {
		(void)xsCall2(xsVar(0), xsID("reportError"), xsArg(1), xsString("? pattern in object"));
	}
	else {
		(void)xsCall0(xsThis, xsID("codeFileLine"));
		(void)xsCall0(xsThis, xsID("codeQualifiedName"));
		if (xsTest(xsGet(xsVar(1), xsID("defaultFlag")))) 
			xscAppendPattern(the, XS_EMBED_PATTERN);
		else
			xscAppendPattern(the, XS_JUMP_PATTERN);
	}
}

void xsObject_codePrototype(txMachine* the)
{
	xsIntegerValue c, i;
	xsVars(4);

	xsVar(0) = xsGet(xsGlobal, xsID("xsc"));
	xsVar(1) = xsGet(xsThis, xsID("prototype"));
	xsVar(2) = xsGet(xsThis, xsID("c"));
	xsVar(3) = xsGet(xsThis, xsID("items"));
	
	xscAppend1(the, XS_DUB);
	(void)xsCall0(xsThis, xsID("codeFileLine"));
	if (xsTest(xsVar(1))) {
		(void)xsCall0(xsVar(1), xsID("codeQualifiedName"));
		xscAppend1(the, XS_INSTANCIATE);
	}
	else if (xsTest(xsVar(2))) {
		i = xsToInteger(xsCall1(xsVar(0), xsID("addHostObject"), xsVar(2)));
		xscAppend1(the, PSEUDO_GET_MEMBER);
		xscAppendID(the, XS_GET_MEMBER, xsID("@"));
		xscAppendInteger(the, i);
		xscAppend1(the, XS_GET_MEMBER_AT);
	}
	else {
		xscAppendInteger(the, 0);
		xscAppend1(the, XS_THIS);
		xscAppendID(the, XS_GET_MEMBER_FOR_CALL, xsID("Object"));
		xscAppend1(the, XS_NEW);
	}
	
	xscAppend1(the, XS_ALIAS);
	xscAppend1(the, XS_SWAP);
	(void)xsCall0(xsThis, xsID("codePrototypeAssignment"));
	
	c = xsToInteger(xsGet(xsVar(3), the->lengthID));
	for (i = 0; i < c; i++) 
		(void)xsCall0(xsGetAt(xsVar(3), xsInteger(i)), xsID("codePrototype"));
		
	xscAppend1(the, XS_POP);
}

void xsObject_crossReference(txMachine* the)
{
	xsIntegerValue c, i;
	
	xsProperty_crossReference(the);
	
	if (xsTest(xsCall1(xsThis, xsID("hasOwnProperty"), xsString("c")))) {
		if (xsTest(xsCall1(xsThis, xsID("hasOwnProperty"), xsString("prototype"))))
			(void)xsCall2(xsVar(0), xsID("reportError"), xsThis, xsString("a host object has no prototype"));
	}
	else {
		xsVar(1) = xsGet(xsThis, xsID("prototype"));
		if (xsTest(xsVar(1))) {
			xsVar(1) = xsCall1(xsVar(0), xsID("searchProperty"), xsVar(1));
			if (!xsTest(xsVar(1)))
				(void)xsCall2(xsVar(0), xsID("reportError"), xsThis, xsString("prototype does not exist"));
			else if (!xsIsInstanceOf(xsVar(1), xsGet(xsGlobal, xsID("xsObject"))))
				(void)xsCall2(xsVar(0), xsID("reportError"), xsThis, xsString("prototype is no object"));
			xsSet(xsThis, xsID("prototype"), xsVar(1));
		}
	}
		
		
	xsVar(1) = xsGet(xsThis, xsID("name"));
	xsArg(1) = xsCat3(xsArg(1), xsVar(1), xsString("."));
	xsVar(1) = xsGet(xsThis, xsID("items"));
	c = xsToInteger(xsGet(xsVar(1), the->lengthID));
	for (i = 0; i < c; i++) 
		(void)xsCall3(xsGetAt(xsVar(1), xsInteger(i)), xsID("crossReference"), xsArg(0), xsArg(1), xsArg(2));
}

void xsObject_target(txMachine* the)
{
	xsIntegerValue c, i;
	xsVars(2);
	
	xsVar(0) = xsGet(xsThis, xsID("items"));
	xsVar(1) = xsNewInstanceOf(xsArrayPrototype);
	if (xsTest(xsVar(0))) {
		c = xsToInteger(xsGet(xsVar(0), the->lengthID));
		for (i = 0; i < c; i++) 
			(void)xsCall3(xsGetAt(xsVar(0), xsInteger(i)), xsID("target"), xsVar(1), xsArg(1), xsArg(2));
	}
	xsSet(xsThis, xsID("items"), xsVar(1));
	c = xsToInteger(xsGet(xsArg(0), the->lengthID));
	xsSetAt(xsArg(0), xsInteger(c), xsThis);
	xsSet(xsThis, xsID("linked"), xsArg(1));
}

void xsTarget_target(txMachine* the)
{
	xsIntegerValue c, i;
	xsVars(2);
	
	xsVar(0) = xsGet(xsGlobal, xsID("xsc"));
	xsVar(1) = xsGet(xsThis, xsID("name"));
	if (!xsTest(xsVar(1)))
		(void)xsCall2(xsVar(0), xsID("reportError"), xsThis, xsString("missing name attribute"));
	else if (xsTest(xsCall1(xsVar(0), xsID("hasTarget"), xsVar(1)))) {
		xsVar(1) = xsGet(xsThis, xsID("items"));
		c = xsToInteger(xsGet(xsVar(1), the->lengthID));
		for (i = 0; i < c; i++) 
			(void)xsCall3(xsGetAt(xsVar(1), xsInteger(i)), xsID("target"), xsArg(0), xsArg(1), xsArg(2));
	}
}

void xsECMAPrototype_initialize(txMachine* the)
{
	xsVars(2);
	xsVar(0) = xsGet(xsGlobal, xsID("xsc"));
	xsVar(1) = xsCat2(xsGet(xsThis, xsID("name")), xsString(".prototype"));
	(void)xsCall3(xsVar(0), xsID("insertProperty"), xsVar(1), xsThis, xsFalse);
}

void xsECMAPrototype_codeQualifiedName(txMachine* the)
{
	xscAppend1(the, PSEUDO_GET_MEMBER);
	xscAppendID(the, XS_GET_MEMBER, xsToID(xsGet(xsThis, xsID("name"))));
	xscAppendID(the, XS_GET_MEMBER, xsID("prototype"));
}

void xsInfoSetPrototype_initialize(txMachine* the)
{
	xsVars(3);
	xsVar(0) = xsGet(xsGlobal, xsID("xsc"));
	xsVar(1) = xsGet(xsThis, xsID("name"));
	if (xsTest(xsVar(1)))
		xsVar(2) = xsCat2(xsString("xs.infoset."), xsVar(1));
	else
		xsVar(2) = xsString("xs.infoset");
	(void)xsCall3(xsVar(0), xsID("insertProperty"), xsVar(2), xsThis, xsFalse);
}

void xsInfoSetPrototype_codeQualifiedName(txMachine* the)
{
	xsVars(1);
	xscAppend1(the, PSEUDO_GET_MEMBER);
	xscAppendID(the, XS_GET_MEMBER, xsID("xs"));
	xscAppendID(the, XS_GET_MEMBER, xsID("infoset"));
	xsVar(0) = xsGet(xsThis, xsID("name"));
	if (xsTest(xsVar(0)))
		xscAppendID(the, XS_GET_MEMBER, xsToID(xsVar(0)));
}

