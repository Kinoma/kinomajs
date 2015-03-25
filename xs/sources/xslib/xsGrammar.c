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
#include "xsGrammar.h"

void fx_chunk_serialize(txMachine *the);

void fxBuildGrammar(txMachine* the)
{
	mxPush(mxGlobal);
	
	fxNewInstance(the);
	fxNewHostFunction(the, fx_grammar_array, 2);
	fxQueueID(the, fxID(the, "array"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	fxNewHostFunction(the, fx_grammar_boolean, 1);
	fxQueueID(the, fxID(the, "boolean"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	fxNewHostFunction(the, fx_grammar_chunk, 1);
	fxQueueID(the, fxID(the, "chunk"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	fxNewHostFunction(the, fx_grammar_custom, 2);
	fxQueueID(the, fxID(the, "custom"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	fxNewHostFunction(the, fx_grammar_date, 1);
	fxQueueID(the, fxID(the, "date"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	fxNewHostFunction(the, fx_grammar_link, 0);
	fxQueueID(the, fxID(the, "link"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	fxNewHostFunction(the, fx_grammar_namespace, 2);
	fxQueueID(the, fxID(the, "namespace"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	fxNewHostFunction(the, fx_grammar_number, 1);
	fxQueueID(the, fxID(the, "number"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	fxNewHostFunction(the, fx_grammar_object, 3);
	fxQueueID(the, fxID(the, "object"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	fxNewHostFunction(the, fx_grammar_reference, 2);
	fxQueueID(the, fxID(the, "reference"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	fxNewHostFunction(the, fx_grammar_regexp, 1);
	fxQueueID(the, fxID(the, "regexp"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	fxNewHostFunction(the, fx_grammar_script, 1);
	fxQueueID(the, fxID(the, "script"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	fxNewHostFunction(the, fx_grammar_string, 1);
	fxQueueID(the, fxID(the, "string"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	
	fxAliasInstance(the, the->stack);
	fxNewHostConstructor(the, fx_Grammar, 1);
	
	fxNewHostFunction(the, fx_Grammar_parse, 1);
	fxQueueID(the, fxID(the, "parse"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_xs_serialize, 1);
	fxQueueID(the, fxID(the, "stringify"), XS_DONT_ENUM_FLAG);
	
	fxQueueID(the, fxID(the, "Grammar"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	
	fxNewHostFunction(the, fx_link, 0);
	fxQueueID(the, fxID(the, "__xs__link"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SCRIPT_FLAG | XS_DONT_SET_FLAG);
	
	fxNewHostFunction(the, fx_patch, 2);
	fxQueueID(the, fxID(the, "__xs__patch"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SCRIPT_FLAG | XS_DONT_SET_FLAG);
	
	fxNewHostFunction(the, fx_pattern, 1);
	fxQueueID(the, fxID(the, "__xs__pattern"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SCRIPT_FLAG | XS_DONT_SET_FLAG);
	
	fxNewHostFunction(the, fx_prefix, 1);
	fxQueueID(the, fxID(the, "__xs__prefix"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SCRIPT_FLAG | XS_DONT_SET_FLAG);
	
	fxNewHostFunction(the, fx_root, 1);
	fxQueueID(the, fxID(the, "__xs__root"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SCRIPT_FLAG | XS_DONT_SET_FLAG);
	
	fxNewInstance(the);
	fxNewHostFunction(the, fx_xs_isInstanceOf, 2);
	fxQueueID(the, fxID(the, "isInstanceOf"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SCRIPT_FLAG | XS_DONT_SET_FLAG);
	fxNewHostFunction(the, fx_xs_newInstanceOf, 1);
	fxQueueID(the, fxID(the, "newInstanceOf"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SCRIPT_FLAG | XS_DONT_SET_FLAG);
	fxNewHostFunction(the, fx_xs_parse, 1);
	fxQueueID(the, fxID(the, "parse"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SCRIPT_FLAG | XS_DONT_SET_FLAG);
	fxNewHostFunction(the, fx_xs_parsePrefix, 2);
	fxQueueID(the, fxID(the, "parsePrefix"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SCRIPT_FLAG | XS_DONT_SET_FLAG);
	fxNewHostFunction(the, fx_xs_serialize, 1);
	fxQueueID(the, fxID(the, "serialize"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SCRIPT_FLAG | XS_DONT_SET_FLAG);
	fxNewHostFunction(the, fx_xs_serializeNamespace, 2);
	fxQueueID(the, fxID(the, "serializeNamespace"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SCRIPT_FLAG | XS_DONT_SET_FLAG);
	fxNewHostFunction(the, fx_xs_script, 1);
	fxQueueID(the, fxID(the, "script"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SCRIPT_FLAG | XS_DONT_SET_FLAG);
#ifdef mxProfile
	fxNewHostFunction(the, fx_xs_isProfiling, 0);
	fxQueueID(the, fxID(the, "isProfiling"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SCRIPT_FLAG | XS_DONT_SET_FLAG);
	fxNewHostFunction(the, fx_xs_getProfilingDirectory, 0);
	fxQueueID(the, fxID(the, "getProfilingDirectory"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SCRIPT_FLAG | XS_DONT_SET_FLAG);
	fxNewHostFunction(the, fx_xs_setProfilingDirectory, 1);
	fxQueueID(the, fxID(the, "setProfilingDirectory"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SCRIPT_FLAG | XS_DONT_SET_FLAG);
	fxNewHostFunction(the, fx_xs_startProfiling, 0);
	fxQueueID(the, fxID(the, "startProfiling"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SCRIPT_FLAG | XS_DONT_SET_FLAG);
	fxNewHostFunction(the, fx_xs_stopProfiling, 0);
	fxQueueID(the, fxID(the, "stopProfiling"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SCRIPT_FLAG | XS_DONT_SET_FLAG);
#endif
	mxPushInteger(0);
	fxQueueID(the, fxID(the, "PARSE_DEFAULT"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SCRIPT_FLAG | XS_DONT_SET_FLAG);
	mxPushInteger(1);
	fxQueueID(the, fxID(the, "PARSE_NO_SOURCE"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SCRIPT_FLAG | XS_DONT_SET_FLAG);
	mxPushInteger(2);
	fxQueueID(the, fxID(the, "PARSE_NO_ERROR"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SCRIPT_FLAG | XS_DONT_SET_FLAG);
	mxPushInteger(4);
	fxQueueID(the, fxID(the, "PARSE_NO_WARNING"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SCRIPT_FLAG | XS_DONT_SET_FLAG);
	mxPushInteger(8);
	fxQueueID(the, fxID(the, "PARSE_SNIFF"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SCRIPT_FLAG | XS_DONT_SET_FLAG);
	mxPushInteger(128);
	fxQueueID(the, fxID(the, "PARSE_DEBUG"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SCRIPT_FLAG | XS_DONT_SET_FLAG);
	fxQueueID(the, fxID(the, "xs"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SCRIPT_FLAG | XS_DONT_SET_FLAG);

	fxNewInstance(the);
	fxNewHostFunction(the, fx_boolean_parse, 1);
	fxQueueID(the, fxID(the, "parse"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	fxNewHostFunction(the, fx_all_serialize, 1);
	fxQueueID(the, fxID(the, "serialize"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	fxAliasInstance(the, the->stack);
	fxQueueID(the, fxID(the, "__xs__boolean"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SCRIPT_FLAG | XS_DONT_SET_FLAG);

	fxNewInstance(the);
	fxNewHostFunction(the, fx_chunk_parse, 1);
	fxQueueID(the, fxID(the, "parse"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	fxNewHostFunction(the, fx_chunk_serialize, 1);
	fxQueueID(the, fxID(the, "serialize"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	fxAliasInstance(the, the->stack);
	fxQueueID(the, fxID(the, "__xs__chunk"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SCRIPT_FLAG | XS_DONT_SET_FLAG);
	
	fxNewInstance(the);
	fxNewHostFunction(the, fx_date_parse, 1);
	fxQueueID(the, fxID(the, "parse"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	fxNewHostFunction(the, fx_all_serialize, 1);
	fxQueueID(the, fxID(the, "serialize"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	fxAliasInstance(the, the->stack);
	fxQueueID(the, fxID(the, "__xs__date"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SCRIPT_FLAG | XS_DONT_SET_FLAG);
	
	fxNewInstance(the);
	fxNewHostFunction(the, fx_number_parse, 1);
	fxQueueID(the, fxID(the, "parse"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	fxNewHostFunction(the, fx_all_serialize, 1);
	fxQueueID(the, fxID(the, "serialize"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	fxAliasInstance(the, the->stack);
	fxQueueID(the, fxID(the, "__xs__number"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SCRIPT_FLAG | XS_DONT_SET_FLAG);
	
	fxNewInstance(the);
	fxNewHostFunction(the, fx_regexp_parse, 1);
	fxQueueID(the, fxID(the, "parse"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	fxNewHostFunction(the, fx_regexp_serialize, 1);
	fxQueueID(the, fxID(the, "serialize"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	fxAliasInstance(the, the->stack);
	fxQueueID(the, fxID(the, "__xs__regexp"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SCRIPT_FLAG | XS_DONT_SET_FLAG);

	fxNewInstance(the);
	fxNewHostFunction(the, fx_string_parse, 1);
	fxQueueID(the, fxID(the, "parse"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	fxNewHostFunction(the, fx_all_serialize, 1);
	fxQueueID(the, fxID(the, "serialize"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	fxAliasInstance(the, the->stack);
	fxQueueID(the, fxID(the, "__xs__string"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SCRIPT_FLAG | XS_DONT_SET_FLAG);

	fxNewInstance(the);
	fxNewHostFunction(the, fx_script_parse, 1);
	fxQueueID(the, fxID(the, "parse"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	fxNewHostFunction(the, fx_all_serialize, 1);
	fxQueueID(the, fxID(the, "serialize"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	fxAliasInstance(the, the->stack);
	fxQueueID(the, fxID(the, "__xs__script"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SCRIPT_FLAG | XS_DONT_SET_FLAG);

	fxNewInstance(the);
	fxAliasInstance(the, the->stack);
	fxQueueID(the, the->grammarsID, XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SCRIPT_FLAG | XS_DONT_SET_FLAG);

	fxNewInstance(the);
	fxAliasInstance(the, the->stack);
	fxQueueID(the, fxID(the, "__xs__prefixes"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SCRIPT_FLAG | XS_DONT_SET_FLAG);

	fxNewInstance(the);
	fxAliasInstance(the, the->stack);
	fxQueueID(the, the->rootsID, XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SCRIPT_FLAG | XS_DONT_SET_FLAG);

	
	the->stack++;
}

static txID fxLookupAlias(txMachine* the, txSlot* theSlot)
{
	txID aCount, anIndex;
	txSlot** aSlotAddress;

	aCount = the->aliasIndex;
	for (anIndex = 0, aSlotAddress = the->aliasArray; anIndex < aCount; anIndex++, aSlotAddress++) {
		if (*aSlotAddress == theSlot)
			return anIndex;
	}
	return -1;
}

void fx_grammar_array(txMachine* the)
{
	if (mxArgc < 1)
		mxDebug0(the, XS_SYNTAX_ERROR, "Grammar.prototype.array: no pattern parameter");
	*(--the->stack) = *mxArgv(0);
	if (fxRunTest(the)) {
		txInteger c = mxArgc, i;
		txSlot* anInstance;
		if (c < 2)
			mxDebug0(the, XS_RANGE_ERROR, "Grammar.prototype.array: no content parameter");
		fxNewInstance(the);
		anInstance = fxGetOwnInstance(the, the->stack);
		for (i = 1; i < c; i++) {
			txSlot* aProperty = fxSetProperty(the, anInstance, (txID)(i - 1), C_NULL);
			txSlot* anInstance = fxGetInstance(the, mxArgv(i));
			txID anAlias;
			if (!anInstance)
				mxDebug1(the, XS_TYPE_ERROR, "Grammar.prototype.array: content %d is no object", i);
			anAlias = fxLookupAlias(the, anInstance);
			if (anAlias < 0)
				mxDebug1(the, XS_TYPE_ERROR, "Grammar.prototype.array: content %d has no grammar", i);
			aProperty->kind = XS_ALIAS_KIND;
			aProperty->value.alias = anAlias;
		}
		fxAliasInstance(the, the->stack);
		fxMakePattern(the, mxArgv(0), XS_REPEAT_PATTERN, "array");
	}
	else
		fxNull(the, --the->stack);
	*mxResult = *the->stack++;
}

void fx_grammar_boolean(txMachine* the)
{
	if (mxArgc < 1)
		mxDebug0(the, XS_SYNTAX_ERROR, "Grammar.prototype.boolean: no pattern parameter");
	*(--the->stack) = *mxArgv(0);
	if (fxRunTest(the)) {
		*(--the->stack) = mxGlobal;
		fxGetID(the, fxID(the, "__xs__boolean"));
		fxMakePattern(the, mxArgv(0), XS_DATA_PATTERN, "boolean");
	}
	else
		fxNull(the, --the->stack);
	*mxResult = *the->stack++;
}

void fx_grammar_chunk(txMachine* the)
{
	if (mxArgc < 1)
		mxDebug0(the, XS_SYNTAX_ERROR, "Grammar.prototype.chunk: no pattern parameter");
	*(--the->stack) = *mxArgv(0);
	if (fxRunTest(the)) {
		*(--the->stack) = mxGlobal;
		fxGetID(the, fxID(the, "__xs__chunk"));
		fxMakePattern(the, mxArgv(0), XS_DATA_PATTERN, "chunk");
	}
	else
		fxNull(the, --the->stack);
	*mxResult = *the->stack++;
}

void fx_grammar_custom(txMachine* the)
{
	if (mxArgc < 1)
		mxDebug0(the, XS_SYNTAX_ERROR, "Grammar.prototype.custom: no pattern parameter");
	*(--the->stack) = *mxArgv(0);
	if (fxRunTest(the)) {
		txSlot* anInstance;
		txSlot* aProperty;
		if (mxArgc < 2)
			mxDebug0(the, XS_SYNTAX_ERROR, "Grammar.prototype.custom: no io parameter");
		anInstance = fxGetInstance(the, mxArgv(1));
		if (!anInstance)
			mxDebug0(the, XS_TYPE_ERROR, "Grammar.prototype.custom: io is no object");
		aProperty = fxGetProperty(the, anInstance, the->parseID);
		if (!anInstance)
			mxDebug0(the, XS_TYPE_ERROR, "Grammar.prototype.custom: io has no parse property");
		anInstance = fxGetInstance(the, aProperty);
		if (!anInstance || !mxIsFunction(anInstance))
			mxDebug0(the, XS_TYPE_ERROR, "Grammar.prototype.custom: io.parse is no function");
		aProperty->flag &= ~XS_SANDBOX_FLAG;
		*(--the->stack) = *mxArgv(1);
		fxAliasInstance(the, the->stack);
		fxMakePattern(the, mxArgv(0), XS_DATA_PATTERN, "custom");
	}
	else
		fxNull(the, --the->stack);
	*mxResult = *the->stack++;
}

void fx_grammar_date(txMachine* the)
{
	if (mxArgc < 1)
		mxDebug0(the, XS_SYNTAX_ERROR, "Grammar.prototype.date: no pattern parameter");
	*(--the->stack) = *mxArgv(0);
	if (fxRunTest(the)) {
		*(--the->stack) = mxGlobal;
		fxGetID(the, fxID(the, "__xs__date"));
		fxMakePattern(the, mxArgv(0), XS_DATA_PATTERN, "date");
	}
	else
		fxNull(the, --the->stack);
	*mxResult = *the->stack++;
}

void fx_grammar_link(txMachine* the)
{
	txSlot* anInstance;
	txSlot* aProperty;
	
	anInstance = fxGetInstance(the, mxThis);
	aProperty = fxGetProperty(the, anInstance, the->prefixesID);
	mxPush(*aProperty);
	mxPushInteger(1);
	mxPush(mxGlobal);
	fxCallID(the, fxID(the, "__xs__prefix"));
	the->stack++;
	
	mxPushInteger(0);
	mxPush(mxGlobal);
	fxCallID(the, fxID(the, "__xs__link"));
}

void fx_grammar_namespace(txMachine* the)
{
	txSlot* anInstance;
	txSlot* aSymbol;
	txSlot* aProperty;
	
	if (mxArgc < 1)
		mxDebug0(the, XS_SYNTAX_ERROR, "Grammar.prototype.namespace: no uri parameter");
	anInstance = fxGetInstance(the, mxThis);
	aProperty = fxGetProperty(the, anInstance, the->prefixesID);
	anInstance = fxGetOwnInstance(the, aProperty);
	fxToString(the, mxArgv(0));
	aSymbol = fxNewSymbol(the, mxArgv(0));
	aProperty = fxSetProperty(the, anInstance, aSymbol->ID, C_NULL);
	aProperty->kind = mxEmptyString.kind;
	aProperty->value = mxEmptyString.value;
	if (mxArgc > 1) {
		*(--the->stack) = *mxArgv(1);
		if (fxRunTest(the)) {
			fxToString(the, mxArgv(1));
			aProperty->kind = mxArgv(1)->kind;
			aProperty->value = mxArgv(1)->value;
		}
	}
}

void fx_grammar_number(txMachine* the)
{
	if (mxArgc < 1)
		mxDebug0(the, XS_SYNTAX_ERROR, "Grammar.prototype.number: no pattern parameter");
	*(--the->stack) = *mxArgv(0);
	if (fxRunTest(the)) {
		*(--the->stack) = mxGlobal;
		fxGetID(the, fxID(the, "__xs__number"));
		fxMakePattern(the, mxArgv(0), XS_DATA_PATTERN, "number");
	}
	else
		fxNull(the, --the->stack);
	*mxResult = *the->stack++;
}

void fx_grammar_object(txMachine* the)
{
	txSlot* anInstance;
	txID anAlias;
	txSlot* aProperty;
	txBoolean rootFlag = 0;

	if (mxArgc < 1)
		mxDebug0(the, XS_SYNTAX_ERROR, "Grammar.prototype.object: no prototype parameter");
	anInstance = fxGetInstance(the, mxArgv(0));
	if (!anInstance)
		mxDebug0(the, XS_TYPE_ERROR, "Grammar.prototype.object: prototype is no object");
	if (fxLookupAlias(the, mxArgv(0)) >= 0)
		mxDebug0(the, XS_TYPE_ERROR, "Grammar.prototype.object: prototype has already a grammar");
	anAlias = fxLookupAlias(the, anInstance->value.instance.prototype);
	if (anAlias >= 0)
		anInstance->ID = anAlias;
	fxAliasInstance(the, mxArgv(0));
	mxPush(*mxArgv(0));
	
	fxNewInstance(the);
	anInstance = fxGetOwnInstance(the, the->stack);
	if (mxArgc < 2)
		mxDebug0(the, XS_SYNTAX_ERROR, "Grammar.prototype.object: no pattern parameter");
	mxPush(*mxArgv(1));
	if (fxRunTest(the)) {
		mxPush(*mxArgv(0));
		rootFlag = fxMakePattern(the, mxArgv(1), XS_JUMP_PATTERN, "object");
	}
	else
		fxNull(the, --the->stack);
	aProperty = fxSetProperty(the, anInstance, XS_NO_ID, C_NULL);
	aProperty->kind = the->stack->kind;
	aProperty->value = the->stack->value;
	*mxResult = *the->stack++;
	
	if (rootFlag) {
		mxPush(*mxArgv(0));
		mxPushInteger(1);
		mxPush(mxGlobal);
		fxCallID(the, fxID(the, "__xs__root"));
		the->stack++;
	}
		
	if ((mxArgc > 2) && !mxIsUndefined(mxArgv(2))) {
		txSlot* properties = fxGetInstance(the, mxArgv(2));
		if (properties)
			fxEachOwnProperty(the, properties, XS_DONT_ENUM_FLAG, fx_grammar_object_step, anInstance);
	}
	mxPushInteger(2);
	mxPush(mxGlobal);
	fxCallID(the, fxID(the, "__xs__pattern"));
	the->stack++;
}

void fx_grammar_object_step(txMachine* the, txSlot* theContext, txID theID, txSlot* theProperty) 
{
	txSlot* aProperty = fxSetProperty(the, theContext, theID, C_NULL);
	aProperty->kind = theProperty->kind;
	aProperty->value = theProperty->value;
}

void fx_grammar_reference(txMachine* the)
{
	if (mxArgc < 1)
		mxDebug0(the, XS_SYNTAX_ERROR, "Grammar.prototype.reference: no pattern parameter");
	*(--the->stack) = *mxArgv(0);
	if (fxRunTest(the)) {
		txInteger c = mxArgc, i;
		txSlot* anInstance;
		if (c < 2)
			mxDebug0(the, XS_RANGE_ERROR, "Grammar.prototype.reference: no content parameter");
		fxNewInstance(the);
		anInstance = fxGetOwnInstance(the, the->stack);
		for (i = 1; i < c; i++) {
			txSlot* aProperty = fxSetProperty(the, anInstance, (txID)(i - 1), C_NULL);
			txSlot* anInstance = fxGetInstance(the, mxArgv(i));
			txID anAlias;
			if (!anInstance)
				mxDebug1(the, XS_TYPE_ERROR, "Grammar.prototype.reference: content %d is no object", i);
			anAlias = fxLookupAlias(the, anInstance);
			if (anAlias < 0)
				mxDebug1(the, XS_TYPE_ERROR, "Grammar.prototype.reference: content %d has no grammar", i);
			aProperty->kind = XS_ALIAS_KIND;
			aProperty->value.alias = anAlias;
		}
		fxAliasInstance(the, the->stack);
		fxMakePattern(the, mxArgv(0), XS_REFER_PATTERN, "reference");
	}
	else
		fxNull(the, --the->stack);
	*mxResult = *the->stack++;
}

void fx_grammar_regexp(txMachine* the)
{
	if (mxArgc < 1)
		mxDebug0(the, XS_SYNTAX_ERROR, "Grammar.prototype.regexp: no pattern parameter");
	*(--the->stack) = *mxArgv(0);
	if (fxRunTest(the)) {
		*(--the->stack) = mxGlobal;
		fxGetID(the, fxID(the, "__xs__regexp"));
		fxMakePattern(the, mxArgv(0), XS_DATA_PATTERN, "regexp");
	}
	else
		fxNull(the, --the->stack);
	*mxResult = *the->stack++;
}

void fx_grammar_script(txMachine* the)
{
	if (mxArgc < 1)
		mxDebug0(the, XS_SYNTAX_ERROR, "Grammar.prototype.script: no pattern parameter");
	*(--the->stack) = *mxArgv(0);
	if (fxRunTest(the)) {
		*(--the->stack) = mxGlobal;
		fxGetID(the, fxID(the, "__xs__script"));
		fxMakePattern(the, mxArgv(0), XS_DATA_PATTERN, "script");
	}
	else
		fxNull(the, --the->stack);
	*mxResult = *the->stack++;
}

void fx_grammar_string(txMachine* the)
{
	if (mxArgc < 1)
		mxDebug0(the, XS_SYNTAX_ERROR, "Grammar.prototype.string: no pattern parameter");
	*(--the->stack) = *mxArgv(0);
	if (fxRunTest(the)) {
		*(--the->stack) = mxGlobal;
		fxGetID(the, fxID(the, "__xs__string"));
		fxMakePattern(the, mxArgv(0), XS_DATA_PATTERN, "string");
	}
	else
		fxNull(the, --the->stack);
	*mxResult = *the->stack++;
}

void fx_Grammar(txMachine* the)
{
	txSlot* anInstance;
	txSlot* aProperty;

	anInstance = fxGetOwnInstance(the, mxThis);
	aProperty = fxSetProperty(the, anInstance, the->prefixesID, C_NULL);
	fxNewInstance(the);
	aProperty->kind = the->stack->kind;
	aProperty->value = the->stack->value;
	the->stack++;
}

void fx_Grammar_parse(txMachine* the)
{
#ifdef mxDebug
	char aBuffer[1024];
#endif	
	txFlag aFlag = XS_NO_ERROR_FLAG | XS_NO_WARNING_FLAG;
	char* aPath = C_NULL;
	long aLine = 0;
	txSlot* aSlot;
	txStringCStream aCStream;
	txStringStream aStream;

	if (mxArgc < 1)
		mxDebug0(the, XS_SYNTAX_ERROR, "Grammar.parse: no buffer");
	if (mxArgc > 1) {
		*(--the->stack) = *mxArgv(1);
		if (fxRunTest(the))
			aFlag = XS_NO_FLAG;
	}
#ifdef mxDebug
	aFlag |= XS_DEBUG_FLAG;
	if (mxArgc > 2) {
		aPath = fxToString(the, mxArgv(2));
		if (c_strlen(aPath) < sizeof(aBuffer)) {
			c_strcpy(aBuffer, aPath);
			aFlag |= XS_SOURCE_FLAG;
			aPath = aBuffer;
			aLine = 1;
		}
		else
			aPath = C_NULL;
	}
	if (mxArgc > 3)
		aLine = fxToInteger(the, mxArgv(3));
#endif	

	aSlot = fxGetInstance(the, mxThis);
	if (aSlot->flag & XS_SANDBOX_FLAG)
		the->frame->flag |= XS_SANDBOX_FLAG;
	else
		the->frame->flag |= the->frame->next->flag & XS_SANDBOX_FLAG;

	/* mxArgv(0) = tag, text, and so on */
	mxZeroSlot(--the->stack);
	/* mxArgv(1) = path */
	mxZeroSlot(--the->stack);
	/* mxArgv(2) = line */
	mxZeroSlot(--the->stack);
	/* mxArgc */
	mxZeroSlot(--the->stack);
	the->stack->value.integer = 3;
	the->stack->kind = XS_INTEGER_KIND;

	aSlot = fxGetInstance(the, mxArgv(0));
	if (mxIsChunk(aSlot)) {
		aSlot = aSlot->next;
		aCStream.buffer = aSlot->value.host.data;
		aCStream.size = aSlot->next->value.integer;
		aCStream.offset = 0;
		fxParse(the, &aCStream, fxStringCGetter, aPath, aLine, aFlag);
	}
	else {
		fxToString(the, mxArgv(0));
		aStream.slot = mxArgv(0);
		aStream.size = c_strlen(aStream.slot->value.string);
		aStream.offset = 0;
		fxParse(the, &aStream, fxStringGetter, aPath, aLine, aFlag);
	}
	*mxResult = *the->stack++;
}

txBoolean fxMakePattern(txMachine* the, txSlot* theSlot, txKind theKind, txString theName)
{
	txSlot* anInstance;
	txSlot* aProperty;
	txString aString;
	char aBuffer[1024];
	txBoolean rootFlag = 0;
	txPatternData* aPatternData;
	txInteger c, i;
	txString aColon;

	anInstance = fxGetInstance(the, mxThis);
	aProperty = fxGetProperty(the, anInstance, the->prefixesID);
	anInstance = fxGetOwnInstance(the, aProperty);
	aString = fxToString(the, theSlot);
	c = c_strlen(aString);
	if (c > (txInteger)sizeof(aBuffer) - 4)
		mxDebug1(the, XS_RANGE_ERROR, "Grammar.prototype.%s: too long pattern", theName);
	c_strcpy(aBuffer, aString);
	aString = aBuffer;
	if (*aString == '/') {
		rootFlag = 1;
		aString++;
	}
	c = 1;
	while (*aString) {
		if (*aString == '@') {
			if (theKind != XS_DATA_PATTERN)
				mxDebug1(the, XS_SYNTAX_ERROR, "Grammar.prototype.%s: attribute pattern not allowed", theName);
			theKind = XS_ATTRIBUTE_PATTERN;
		}
		else if (*aString == '?') {
			if (theKind != XS_DATA_PATTERN)
				mxDebug1(the, XS_SYNTAX_ERROR, "Grammar.prototype.%s: pi pattern not allowed", theName);
			theKind = XS_PI_PATTERN;
		}
		else if (*aString == '/') {
			if (theKind == XS_ATTRIBUTE_PATTERN)
				mxDebug1(the, XS_SYNTAX_ERROR, "Grammar.prototype.%s: invalid attribute pattern", theName);
			if (theKind == XS_PI_PATTERN)
				mxDebug1(the, XS_SYNTAX_ERROR, "Grammar.prototype.%s: invalid pi pattern", theName);
			*aString = 0;
			c++;
		}
		aString++;
	}
	if ((theKind == XS_DATA_PATTERN) || (theKind == XS_REFER_PATTERN) || (theKind == XS_REPEAT_PATTERN)) {
		if (*(aString - 1) != '.') {
			*(aString + 1) = '.';
			*(aString + 2) = 0;
			c++;
		}
	}
	aPatternData = fxNewChunk(the, sizeof(txPatternData) + ((c - 1) * sizeof(txPatternPart)));
	aPatternData->alias = the->stack->value.alias;
	aPatternData->count = (txID)c;
	the->stack->value.pattern = aPatternData;
	the->stack->kind = theKind;
	aString = aBuffer;
	if (rootFlag)
		aString++;
	for (i = 0; i < c; i++) {
		if (*aString == '.') {
			aString++;
			if ((i < (c - 1)) || (*aString))
				mxDebug1(the, XS_SYNTAX_ERROR, "Grammar.prototype.%s: invalid default pattern", theName);
			the->stack->value.pattern->parts[i].namespaceID = XS_NO_ID;
			aProperty = fxNewSymbolC(the, ".");
			the->stack->value.pattern->parts[i].nameID = aProperty->ID;
			break;
		}
		if (*aString == '@')
			aString++;
		else if (*aString == '?')
			aString++;
		aColon = c_strchr(aString, ':');
		if (aColon) {
			*aColon = 0;
			fxCopyStringC(the, --the->stack, aString);
			aString = aColon + 1;
		}
		else
			fxCopyStringC(the, --the->stack, "");
		the->stack->ID = XS_NO_ID;
		if (aColon || (theKind != XS_ATTRIBUTE_PATTERN))
			fxEachOwnProperty(the, anInstance, XS_NO_FLAG, fxMakePatternStep, the->stack);
		if (aColon && (the->stack->ID == XS_NO_ID))
			mxDebug1(the, XS_SYNTAX_ERROR, "Grammar.prototype.%s: unknown prefix", theName);
		the->stack++;
		the->stack->value.pattern->parts[i].namespaceID = (the->stack - 1)->ID;
		aProperty = fxNewSymbolC(the, aString);
		the->stack->value.pattern->parts[i].nameID = aProperty->ID;
		while (*aString) aString++;
		aString++;
	}
	return rootFlag;
}

void fxMakePatternStep(txMachine* the, txSlot* theContext, txID theID, txSlot* theProperty) 
{
	if (c_strcmp(fxToString(the, theContext), fxToString(the, theProperty)) == 0)
		theContext->ID = theID;
}

void fx_link(txMachine* the)
{
	txID aCount, anIndex;
	txSlot* aSlot;
	txSlot* aParent;
	txSlot* aLink;
	
	mxTry(the) {
		aCount = the->aliasIndex;
		the->linkArray = (txSlot **)c_calloc(aCount, sizeof(txSlot*));
		if (!the->linkArray)
			fxJump(the);
		for (anIndex = 0; anIndex < aCount; anIndex++) {
			fxNewInstance(the);
			the->linkArray[anIndex] = the->stack->value.reference;
			the->stack++;
		}
		for (anIndex = 0; anIndex < aCount; anIndex++) {
			aSlot = the->aliasArray[anIndex];
			if (aSlot->ID >= 0) {
				aParent = the->aliasArray[aSlot->ID];
				if ((aParent->flag & XS_VALUE_FLAG) == 0) {
					aLink = the->linkArray[aSlot->ID];
					aSlot = fxGetOwnProperty(the, aLink, the->instancesID);
					if (!aSlot) {
						aSlot = fxSetProperty(the, aLink, the->instancesID, C_NULL);
						fxNewInstance(the);
						aSlot->value.reference = the->stack->value.reference;
						aSlot->kind = the->stack->kind;
						the->stack++;
					}
					aSlot = aSlot->value.reference;
					while (aSlot->next)
						aSlot = aSlot->next;
					aSlot->next = fxNewSlot(the);
					aSlot = aSlot->next;
					aSlot->value.alias = anIndex;
					aSlot->kind = XS_ALIAS_KIND;
				}
			}
		}
		the->scratch.value.integer = 0;
		fxLinkRoots(the);
		if (the->scratch.value.integer)
			mxDebug0(the, XS_SYNTAX_ERROR, "Grammar error(s)");
		c_free(the->linkArray);
		the->linkArray = C_NULL;
	}
	mxCatch(the) {
		if (the->linkArray) {
			c_free(the->linkArray);
			the->linkArray = C_NULL;
		}
		fxJump(the);
	}
}

void fx_patch(txMachine* the)
{
	txSlot* anInstance;
	txSlot* srcSlot;
	txSlot* dstSlot;
	txSlot* aSlot;

	mxCheck(the, mxArgv(0)->kind == XS_ALIAS_KIND);
	mxCheck(the, mxArgv(1)->kind == XS_REFERENCE_KIND);
	anInstance = fxGetOwnInstance(the, mxArgv(0));
	if (anInstance->flag & XS_DONT_PATCH_FLAG) 
		mxDebug0(the, XS_TYPE_ERROR, "grammar: no patch");
	
	fxNewInstance(the);
	dstSlot = the->stack->value.reference;
	
	srcSlot = fxGetOwnProperty(the, anInstance, the->patternsID);
	if (srcSlot) {
		srcSlot = srcSlot->value.reference->next;
		while (srcSlot) {
			dstSlot->next = fxDuplicateSlot(the, srcSlot);
			dstSlot = dstSlot->next;
			srcSlot = srcSlot->next;
		}
	}
	
	srcSlot = mxArgv(1)->value.reference->next;
	while (srcSlot) {
		aSlot = fxGetOwnProperty(the, the->stack->value.reference, srcSlot->ID);
		if (aSlot) {
			srcSlot->ID = XS_NO_ID;
			aSlot->kind = srcSlot->kind;
			aSlot->value = srcSlot->value;
		}
		srcSlot = srcSlot->next;
	}
	
	srcSlot = mxArgv(1)->value.reference->next;
	while (srcSlot) {
		if (srcSlot->ID != XS_NO_ID) {
			dstSlot->next = fxDuplicateSlot(the, srcSlot);
			dstSlot = dstSlot->next;
		}
		srcSlot = srcSlot->next;
	}
	
	aSlot = fxSetProperty(the, anInstance, the->patternsID, C_NULL);
	aSlot->flag = XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SCRIPT_FLAG | XS_DONT_SET_FLAG;
	aSlot->value.reference = the->stack->value.reference;	
	aSlot->kind = the->stack->kind;
	the->stack++;
}

void fx_pattern(txMachine* the)
{
	txSlot* anInstance;
	txFlag anInstanceFlag;
	txFlag aFlag = XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SCRIPT_FLAG | XS_DONT_SET_FLAG;
	txSlot* aSlot;

	mxCheck(the, mxArgv(0)->kind == XS_ALIAS_KIND);
	mxCheck(the, mxArgv(1)->kind == XS_REFERENCE_KIND);
	anInstance = fxGetOwnInstance(the, mxArgv(0));
		
	anInstanceFlag = anInstance->flag;
	anInstance->flag &= ~XS_DONT_PATCH_FLAG;
	aSlot = fxSetProperty(the, anInstance, the->patternsID, &aFlag);
	anInstance->flag = anInstanceFlag;
	
	aSlot->flag = aFlag;
	aSlot->value.reference = mxArgv(1)->value.reference;	
	aSlot->kind = mxArgv(1)->kind;
}

void fx_prefix(txMachine* the)
{
	txSlot* srcSlot;
	txSlot* dstSlot;
	txSlot* prefixes;
	
	mxCheck(the, mxArgv(0)->kind == XS_REFERENCE_KIND);
	srcSlot = mxArgv(0)->value.reference->next;
	dstSlot = fxGetProperty(the, mxGlobal.value.reference, the->prefixesID);
	prefixes = fxGetOwnInstance(the, dstSlot);
	while (srcSlot) {
		if (srcSlot->ID != XS_NO_ID) {
			dstSlot = fxGetOwnProperty(the, prefixes, srcSlot->ID);
			if (dstSlot)
				fxUnlinkRoots(the, srcSlot->ID);
			else
				dstSlot = fxSetProperty(the, prefixes, srcSlot->ID, C_NULL);
			if (srcSlot->kind == XS_STRING_KIND) {
				dstSlot->kind = srcSlot->kind;
				dstSlot->value = srcSlot->value;
			}
		}
		srcSlot = srcSlot->next;
	}
}

void fx_root(txMachine* the)
{
	txSlot* aSlot;

	mxCheck(the, mxArgv(0)->kind == XS_ALIAS_KIND);
	aSlot = fxGetProperty(the, mxGlobal.value.reference, the->rootsID);
	aSlot = fxGetOwnInstance(the, aSlot);
	while (aSlot->next)
		aSlot = aSlot->next;
	aSlot->next = fxNewSlot(the);
	aSlot = aSlot->next;
	aSlot->value.alias = mxArgv(0)->value.alias;
	aSlot->kind = XS_ALIAS_KIND;
}

void fx_xs_isInstanceOf(txMachine* the)
{
	*(--the->stack) = *mxArgv(1);
	*(--the->stack) = *mxArgv(0);
	mxResult->value.boolean = fxIsInstanceOf(the);
	mxResult->kind = XS_BOOLEAN_KIND;
}

void fx_xs_newInstanceOf(txMachine* the)
{
	*(--the->stack) = *mxArgv(0);
	fxNewInstanceOf(the);
	*mxResult = *the->stack++;
}

void fx_xs_parse(txMachine* the)
{
	txStringStream aStream;
	txFlag aFlag;
	txInteger aCount, anIndex;

	aStream.slot = mxArgv(0);
	aStream.offset = 0;
	aStream.size = c_strlen(mxArgv(0)->value.string);
	if ((mxArgc > 1) && (mxArgv(1)->kind == XS_INTEGER_KIND))
		aFlag = (txFlag)(mxArgv(1)->value.integer);
	else
		aFlag = 0;
	aFlag ^= 1;
	
	/* mxArgv(0) = tag, text, and so on */
	mxZeroSlot(--the->stack);
	/* mxArgv(1) = path */
	mxZeroSlot(--the->stack);
	/* mxArgv(2) = line */
	mxZeroSlot(--the->stack);
	if (mxArgc > 2) {
		aCount = mxArgc - 2;
		for (anIndex = mxArgc - 1; anIndex >= 2; anIndex--)
			*(--the->stack) = *mxArgv(anIndex);
	}
	else
		aCount = 0;
	/* ARGC */
	mxZeroSlot(--the->stack);
	the->stack->value.integer = 3 + aCount;
	the->stack->kind = XS_INTEGER_KIND;
		
	fxParse(the, &aStream, fxStringGetter, C_NULL, 0, aFlag);
	
	*mxResult = *the->stack++;
}

void fx_xs_parsePrefix(txMachine* the)
{
	txString aPrefix = fxToString(the, mxArgv(1));
	txID anID;
	if (the->parseFlag & XS_INFO_SET_FLAG)
		anID = fxSearchInfoSetNamespace(the, aPrefix, c_strlen(aPrefix));
	else
		anID = fxSearchNamespace(the, aPrefix, c_strlen(aPrefix));
	if (anID != XS_NO_ID) {
		txSlot* aSymbol = fxGetSymbol(the, anID);
		if (aSymbol) {
			mxResult->value.string = aSymbol->value.symbol.string;
			mxResult->kind = XS_STRING_KIND;
		}
	}
}

void fx_xs_serialize(txMachine* the)
{
	txInteger aLength;
	txString aBuffer;
	
	*(--the->stack) = *mxArgv(0);
	fxSerializeBuffer(the, C_NULL, 0);
	aLength = the->stack->value.integer;
	the->stack++;
	
	aBuffer = (txString)c_malloc(aLength + 1);
	if (!aBuffer)
		fxJump(the);
		
	*(--the->stack) = *mxArgv(0);
	fxSerializeBuffer(the, aBuffer, aLength);
	aLength = the->stack->value.integer;
	aBuffer[aLength] = 0;
	the->stack++;
		
	fxCopyStringC(the, mxResult, aBuffer);
	c_free(aBuffer);
}

void fx_xs_serializeNamespace(txMachine* the)
{
	if (mxIsReference(mxArgv(0))) {
		txSlot* aSymbol = fxFindSymbol(the, fxToString(the, mxArgv(1)));
		if (aSymbol) {
			txString aPrefix = fxSearchPrefix(the, mxArgv(0)->value.reference, aSymbol->ID);
			if (aPrefix) {
				mxResult->value.string = aPrefix;
				mxResult->kind = XS_STRING_KIND;
			}
		}
	}
}

void fx_xs_script(txMachine* the)
{
	txInteger aResult = fxScript(the);
	if (aResult)
		aResult--;
	mxResult->kind = XS_INTEGER_KIND;
	mxResult->value.integer = aResult;
}

#ifdef mxProfile

void fx_xs_isProfiling(txMachine* the)
{
	mxResult->kind = XS_BOOLEAN_KIND;
	mxResult->value.boolean = fxIsProfiling(the);
}

void fx_xs_getProfilingDirectory(txMachine* the)
{
	if (the->profileDirectory)
		fxString(the, mxResult, the->profileDirectory);
}

void fx_xs_setProfilingDirectory(txMachine* the)
{
	txString aString;
	txInteger aLength;
	
	if (mxArgc < 1)
		mxDebug0(the, XS_SYNTAX_ERROR, "xs.setProfilingDirectory: no directory parameter");
	if (the->profileDirectory) {
		c_free(the->profileDirectory);
		the->profileDirectory = C_NULL;
	}
	*(--the->stack) = *mxArgv(0);
	if (fxRunTest(the)) {
        aString = fxToString(the, mxArgv(0));
        aLength = c_strlen(aString) + 1;
        the->profileDirectory = c_malloc(aLength);
        if (!the->profileDirectory)
            fxJump(the);
        c_memcpy(the->profileDirectory, aString, aLength);
	}
}

void fx_xs_startProfiling(txMachine* the)
{
	fxStartProfiling(the);
}

void fx_xs_stopProfiling(txMachine* the)
{
	fxStopProfiling(the);
}

#endif

void fx_all_serialize(txMachine* the)
{
	*mxResult = *mxArgv(0);
	fxToString(the, mxResult);
}

void fx_chunk_serialize(txMachine *the)
{
	extern void fx_Chunk_toString(txMachine* the);
	txSlot* anInstance = fxGetInstance(the, mxArgv(0));
	txSlot* aProperty = fxGetProperty(the, anInstance, the->toStringID);
	aProperty = aProperty->value.reference->next;
	if ((XS_CALLBACK_KIND != aProperty->kind) || (fx_Chunk_toString != aProperty->value.callback.address)) {
		*mxResult = *mxArgv(0);
		fxToString(the, mxResult);
	}
	else
		*mxResult = *mxArgv(0);
}

void fx_boolean_parse(txMachine* the)
{
	if (c_strlen(mxArgv(0)->value.string) == 0) {
		mxResult->kind = XS_BOOLEAN_KIND;
		mxResult->value.boolean = 0;
	}
	else if (c_strcmp(mxArgv(0)->value.string, "false") == 0) {
		mxResult->kind = XS_BOOLEAN_KIND;
		mxResult->value.boolean = 0;
	}
	else  {
		mxResult->kind = XS_BOOLEAN_KIND;
		mxResult->value.boolean = 1;
	}
}

void fx_chunk_parse(txMachine* the)
{
	mxZeroSlot(--the->stack);
	the->stack->value = mxArgv(0)->value;
	the->stack->kind = mxArgv(0)->kind;
	mxZeroSlot(--the->stack);
	the->stack->value.integer = 1;
	the->stack->kind = XS_INTEGER_KIND;
	*(--the->stack) = mxGlobal;
	fxNewID(the, fxID(the, "Chunk"));
	*mxResult = *the->stack;
	the->stack++;
}

void fx_date_parse(txMachine* the)
{
	mxZeroSlot(--the->stack);
	the->stack->value = mxArgv(0)->value;
	the->stack->kind = mxArgv(0)->kind;
	mxZeroSlot(--the->stack);
	the->stack->value.integer = 1;
	the->stack->kind = XS_INTEGER_KIND;
	*(--the->stack) = mxGlobal;
	fxGetID(the, fxID(the, "Date"));
	fxCallID(the, the->parseID);
	mxZeroSlot(--the->stack);
	the->stack->value.integer = 1;
	the->stack->kind = XS_INTEGER_KIND;
	*(--the->stack) = mxGlobal;
	fxNewID(the, fxID(the, "Date"));
	*mxResult = *the->stack;
	the->stack++;
}

void fx_number_parse(txMachine* the)
{
	txInteger anInteger;
	txNumber aNumber;

	*mxResult = *mxArgv(0);
	fxToNumber(the, mxResult);
	anInteger = (txInteger)mxResult->value.number;
	aNumber = anInteger;
	if (mxResult->value.number == aNumber) {
		mxResult->value.integer = anInteger;
		mxResult->kind = XS_INTEGER_KIND;
	}
}

void fx_regexp_parse(txMachine* the)
{
	txString aString = mxArgv(0)->value.string;
	txInteger aLength = c_strlen(aString);
	txInteger anOffset = 0;
	if (*aString == '/') {
		txString aSlash = c_strrchr(aString, '/');
		if (aSlash) {
			anOffset = aSlash - aString;
	}
	if (aLength && anOffset)
		mxZeroSlot(--the->stack);
		the->stack->value.string = (txString)fxNewChunk(the, anOffset);
		c_memcpy(the->stack->value.string, mxArgv(0)->value.string + 1, anOffset - 1);
		the->stack->value.string[anOffset - 1] = 0;
		the->stack->kind = XS_STRING_KIND;
		mxZeroSlot(--the->stack);
		the->stack->value.string = (txString)fxNewChunk(the, aLength - anOffset + 1);
		c_memcpy(the->stack->value.string, mxArgv(0)->value.string + anOffset + 1, aLength - anOffset);
		the->stack->value.string[aLength - anOffset] = 0;
		the->stack->kind = XS_STRING_KIND;
		mxZeroSlot(--the->stack);
		the->stack->value.integer = 2;
		the->stack->kind = XS_INTEGER_KIND;
	}
	else {
		mxZeroSlot(--the->stack);
		the->stack->value = mxArgv(0)->value;
		the->stack->kind = mxArgv(0)->kind;
		mxZeroSlot(--the->stack);
		the->stack->value.integer = 1;
		the->stack->kind = XS_INTEGER_KIND;
	}
	*(--the->stack) = mxGlobal;
	fxNewID(the, fxID(the, "RegExp"));
	*mxResult = *the->stack;
	the->stack++;
}

void fx_regexp_serialize(txMachine* the)
{
	txSlot* aRegExp;
	txSlot* aProperty;
	
	aRegExp = fxGetInstance(the, mxArgv(0));
	aProperty = fxGetProperty(the, aRegExp, fxID(the, "source"));
	mxResult->value = aProperty->value;
	mxResult->kind = aProperty->kind;
}

void fx_script_parse(txMachine* the)
{
	txStringStream aStream;
	txSlot* aFrame;
	txSlot* aSlot;
#ifdef mxDebug
	txSlot* aFile;
	txSlot* aLine;
#endif
	txByte* aCode;
	
	mxPush(*mxArgv(0));
	if (!fxRunTest(the))
		return;
	fxToString(the, mxArgv(0));
	aStream.slot = mxArgv(0);
	aStream.size = c_strlen(aStream.slot->value.string);
	aStream.offset = 0;
	
	aFrame = the->frame->next;
	aSlot = aFrame + 4 + ((aFrame + 4)->value.integer);
	aSlot = fxGetOwnProperty(the, aSlot->value.reference, the->parentID);
	aSlot = fxGetOwnProperty(the, aSlot->value.reference, the->instanceID);
#ifdef mxDebug
	aFile = aFrame + 4 + ((aFrame + 4)->value.integer) - 1;
	aLine = aFrame + 4 + ((aFrame + 4)->value.integer) - 2;
	aCode = fxParseScript(the, &aStream, fxStringGetter, 
			fxNewFile(the, aFile), aLine->value.integer,
			XS_SANDBOX_FLAG | XS_THIS_FLAG | XS_DEBUG_FLAG, C_NULL);
#else
	aCode = fxParseScript(the, &aStream, fxStringGetter, C_NULL, 0,
			XS_SANDBOX_FLAG | XS_THIS_FLAG, C_NULL);
#endif
	/* ARGC */
	mxZeroSlot(--the->stack);
	the->stack->kind = XS_INTEGER_KIND;
	the->stack->value.integer = 0;
	/* THIS */
	*(--the->stack) = *aSlot;
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

void fx_string_parse(txMachine* the)
{
	*mxResult = *mxArgv(0);
}

void fxLinkContents(txMachine* the, txSlot* theGrammar, txID theAlias, txSlot* theStack)
{
	txSlot* aSlot;
	
	aSlot = fxGetOwnProperty(the, the->aliasArray[theAlias], the->patternsID);
	if (aSlot && mxIsReference(aSlot)) {
		aSlot = fxGetInstance(the, aSlot);
		aSlot = aSlot->next; /* contents */
		if (aSlot->kind != XS_NULL_KIND)
			fxLinkPattern(the, theGrammar, aSlot, theStack);
	}
	aSlot = fxGetOwnProperty(the, the->linkArray[theAlias], the->instancesID);
	if (aSlot && mxIsReference(aSlot)) {
		aSlot = aSlot->value.reference->next;
		while (aSlot) {
			fxLinkContents(the, theGrammar, aSlot->value.alias, theStack);
			aSlot = aSlot->next;
		}
	}
}

void fxLinkError(txMachine* the, txSlot* theGrammar, txPatternPart* thePatternPart, 
		txSlot* thePattern, txSlot* theStack)
{
	txSlot* aSymbol;
	txString aPath;
	txInteger aLine;
	txString aNamespace = C_NULL;
	txString aName = "?";

#ifdef mxDebug
	aSymbol = fxGetSymbol(the, thePattern->value.pattern->pathID);
	if (aSymbol) {
		aPath = aSymbol->value.symbol.string;
		aLine = thePattern->value.pattern->line;
	}
	else {
#endif
		aPath = C_NULL;
		aLine = 0;
#ifdef mxDebug
	}
#endif
	if (thePatternPart->namespaceID != XS_NO_ID) {
		aSymbol = fxGetSymbol(the, thePatternPart->namespaceID);
		if (aSymbol)
			aNamespace = aSymbol->value.symbol.string;
	}
	if (thePatternPart->nameID != XS_NO_ID) {
		aSymbol = fxGetSymbol(the, thePatternPart->nameID);
		if (aSymbol)
			aName = aSymbol->value.symbol.string;
	}
	if (aNamespace)	
		fxReportError(the, aPath, aLine, "pattern {%s}:%s already used", aNamespace, aName);
	else
		fxReportError(the, aPath, aLine, "pattern %s already used", aName);
	the->scratch.value.integer++;
	aSymbol = fxPutRule(the, theGrammar, thePatternPart, thePattern, theStack);
	aSymbol->kind = XS_ERROR_RULE;
}

void fxLinkNamespace(txMachine* the, txSlot* theRoot, txID theNamespaceID)
{
	txInteger anIndex;
	txSlot** aSlotAddress;
	txSlot* aSlot;
	txSlot* aPrefix;

	anIndex = 0;
	aSlotAddress = &(theRoot->next->next);
	while ((aSlot = *aSlotAddress) && (aSlot->kind == XS_PREFIX_KIND)) {
		if (aSlot->value.rule.part.namespaceID == theNamespaceID)
			return;
		anIndex++;
		aSlotAddress = &(aSlot->next);
	}
	*aSlotAddress = aPrefix = fxNewSlot(the);
	aPrefix->next = C_NULL;
	aPrefix->ID = XS_NO_ID;
	aPrefix->kind = XS_PREFIX_KIND;
	aPrefix->value.prefix.part.namespaceID = theNamespaceID;
	aPrefix->value.prefix.part.nameID = XS_NO_ID;
	aPrefix->value.prefix.string = C_NULL;
}

void fxLinkPattern(txMachine* the, txSlot* theGrammar, txSlot* thePattern, txSlot* theStack)
{
	txInteger aCount;
	txInteger anIndex;
	txPatternPart aPatternPart;
	txSlot* aSlot;
	txSlot* aRule;

	aCount = thePattern->value.pattern->count - 1;
	for (anIndex = 0; anIndex < aCount; anIndex++) {
		aPatternPart = thePattern->value.pattern->parts[anIndex];
		aSlot = fxGetGrammarNode(the, theGrammar, &aPatternPart);
		if (!aSlot || (aSlot->kind == XS_ATTRIBUTE_RULE) || (aSlot->kind == XS_PI_RULE)) {
			aSlot = fxPutGrammarNode(the, theGrammar, &aPatternPart);
			fxNewInstance(the);
			aSlot->value.node.link = theGrammar = the->stack->value.reference;
			the->stack++;
		}
		else if (aSlot->kind == XS_NODE_KIND)
			theGrammar = aSlot->value.node.link;
		else {
			fxLinkError(the, theGrammar, &aPatternPart, thePattern, theStack);
			return;
		}
	}
	aPatternPart = thePattern->value.pattern->parts[anIndex];

	switch (thePattern->kind) {
	case XS_ATTRIBUTE_PATTERN:
		aSlot = fxGetGrammarNode(the, theGrammar, &aPatternPart);
		if (aSlot && (aSlot->kind == XS_ATTRIBUTE_RULE))
			fxLinkError(the, theGrammar, &aPatternPart, thePattern, theStack);
		else
			fxPutRule(the, theGrammar, &aPatternPart, thePattern, theStack);
		break;
	case XS_DATA_PATTERN:
		aSlot = fxGetGrammarNode(the, theGrammar, &aPatternPart);
		if (aSlot && (aSlot->kind == XS_DATA_RULE))
			fxLinkError(the, theGrammar, &aPatternPart, thePattern, theStack);
		else
			fxPutRule(the, theGrammar, &aPatternPart, thePattern, theStack);
		break;
	case XS_PI_PATTERN:
		aSlot = fxGetGrammarNode(the, theGrammar, &aPatternPart);
		if (aSlot && (aSlot->kind == XS_PI_RULE))
			fxLinkError(the, theGrammar, &aPatternPart, thePattern, theStack);
		else
			fxPutRule(the, theGrammar, &aPatternPart, thePattern, theStack);
		break;
		
	case XS_REFER_PATTERN:
		aRule = fxPutRule(the, theGrammar, &aPatternPart, thePattern, theStack);
		aSlot = the->aliasArray[thePattern->value.pattern->alias];
		aSlot = aSlot->next;
		while (aSlot) {
			if (mxIsReference(aSlot))
				fxLinkContents(the, theGrammar, aSlot->value.alias, theStack);
			else if (aSlot->kind != XS_NULL_KIND)
				fxLinkPattern(the, theGrammar, aSlot, theStack);
			aSlot = aSlot->next;
		}
		aSlot = theGrammar;
		while (aSlot->next)
			aSlot = aSlot->next;
		aRule->value.rule.data->link = aSlot;
		break;
	case XS_REPEAT_PATTERN:
		aRule = fxPutRule(the, theGrammar, &aPatternPart, thePattern, theStack);
		mxZeroSlot(--the->stack);
		the->stack->ID = XS_NO_ID;
		aSlot = the->aliasArray[thePattern->value.pattern->alias];
		aSlot = aSlot->next;
		while (aSlot) {
			if (mxIsReference(aSlot))
				fxLinkContents(the, theGrammar, aSlot->value.alias, theStack);
			else if (aSlot->kind != XS_NULL_KIND)
				fxLinkPattern(the, theGrammar, aSlot, theStack);
			aSlot = aSlot->next;
		}
		the->stack++;
		aSlot = theGrammar;
		while (aSlot->next)
			aSlot = aSlot->next;
		aRule->value.rule.data->link = aSlot;
		break;
		
	case XS_EMBED_PATTERN:
		fxPutRule(the, theGrammar, &aPatternPart, thePattern, theStack);
		fxLinkPatterns(the, theGrammar, thePattern->value.pattern->alias, theStack);
		break;
	
	case XS_JUMP_PATTERN:
		aSlot = fxGetGrammarNode(the, theGrammar, &aPatternPart);
		if (aSlot && (aSlot->kind != XS_ATTRIBUTE_RULE) && (aSlot->kind != XS_PI_RULE)) {
			if (!fxCompareRule(the, theGrammar, aSlot, thePattern, theStack))
				fxLinkError(the, theGrammar, &aPatternPart, thePattern, theStack);
		}
		else {
			aRule = fxPutRule(the, theGrammar, &aPatternPart, thePattern, theStack);
			aSlot = fxLinkPrototype(the, thePattern->value.pattern->alias);
			aRule->value.rule.data->link = aSlot;
		}
		break;
		
	}
}

void fxLinkPatterns(txMachine* the, txSlot* theGrammar, txID theAlias, txSlot* theStack)
{
	txSlot* aPrototype;
	txSlot* aSlot;
	
	fxNewInstance(the); /* to override */
	aPrototype = the->aliasArray[theAlias];
	while (aPrototype) {
		aSlot = fxGetOwnProperty(the, aPrototype, the->patternsID);
		if (aSlot && mxIsReference(aSlot)) {
			aSlot = fxGetInstance(the, aSlot);
			aSlot = aSlot->next->next; /* skip contents */
			while (aSlot) {
				if (!fxGetOwnProperty(the, the->stack->value.reference, aSlot->ID)) {
					fxSetProperty(the, the->stack->value.reference, aSlot->ID, C_NULL);
					if (aSlot->kind != XS_NULL_KIND) {
						the->stack->ID = aSlot->ID;
						fxLinkPattern(the, theGrammar, aSlot, theStack);
					}
				}
				aSlot = aSlot->next;
			}
		}
		aPrototype = fxGetParent(the, aPrototype);
	}
	the->stack++;
}

void fxLinkPrefixes(txMachine* the, txSlot* theRoot, txSlot* theGrammar)
{
	txSlot* aRule;
	txSlot* aSlot;
	
	aRule = theGrammar->next;
	while (aRule) {
		switch (aRule->kind) {
		case XS_NODE_KIND:
			fxLinkNamespace(the, theRoot, aRule->value.rule.part.namespaceID);
			fxLinkPrefixes(the, theRoot, aRule->value.node.link);
			break;
		case XS_PREFIX_KIND:
			break;
		case XS_ATTRIBUTE_RULE:
			if (aRule->value.rule.part.namespaceID != XS_NO_ID)
				fxLinkNamespace(the, theRoot, aRule->value.rule.part.namespaceID);
			break;
		case XS_DATA_RULE:
			break;
		case XS_PI_RULE:
			fxLinkNamespace(the, theRoot, aRule->value.rule.part.namespaceID);
			break;
		case XS_EMBED_RULE:
			break;
		case XS_JUMP_RULE:
			fxLinkNamespace(the, theRoot, aRule->value.rule.part.namespaceID);
			aSlot = aRule->value.rule.data->link;
			if (!(aSlot->flag & XS_LEVEL_FLAG)) {
				aSlot->flag |= XS_LEVEL_FLAG;
				fxLinkPrefixes(the, theRoot, aSlot);
			}
			break;
		case XS_REFER_RULE:
			break;
		case XS_REPEAT_RULE:
			break;
		case XS_ERROR_RULE:
			break;
		}
		aRule = aRule->next;
	}
}

txSlot* fxLinkPrototype(txMachine* the, txID theAlias)
{
	txSlot* aLink;
	txSlot* aSlot;
	
	aLink = the->linkArray[theAlias];
	aSlot = fxGetOwnProperty(the, aLink, the->grammarID);
	if (!aSlot) {
		aSlot = fxSetProperty(the, aLink, the->grammarID, C_NULL);
		fxNewInstance(the);
		aSlot->value.reference = the->stack->value.reference;
		aSlot->kind = the->stack->kind;
		the->stack++;
		fxLinkPatterns(the, aSlot->value.reference, theAlias, the->stack);
	}
	return aSlot->value.reference;
}

void fxLinkRoot(txMachine* the, txID theAlias)
{
	txSlot* aRoot;
	txSlot* aSlot;
	txSlot* prefixes;
	txInteger anIndex;
	txFlag aFlag;
	txSlot** aPrefixAddress;
	txSlot* aPrefix;
	txString aString;
	char aBuffer[32] = "xs";

	aRoot = the->stack->value.reference;
	aRoot->ID = theAlias;
	
	aSlot = fxGetOwnProperty(the, the->aliasArray[theAlias], the->patternsID);
	if (aSlot && mxIsReference(aSlot)) {
		aSlot = fxGetInstance(the, aSlot);
		aSlot = aSlot->next; /* contents */
		if (aSlot->kind != XS_NULL_KIND)
			fxLinkPattern(the, aRoot, aSlot, the->stack + 1); /* root: the->stack->ID = 0 */
	}
	
	fxLinkPrefixes(the, aRoot, aRoot);
	for (anIndex = 0; anIndex < the->aliasIndex; anIndex++) {
		aSlot = fxGetOwnProperty(the, the->linkArray[anIndex], the->grammarID);
		if (aSlot)
			aSlot->value.reference->flag &= ~XS_LEVEL_FLAG;
	}
	aSlot = fxGetProperty(the, mxGlobal.value.reference, the->prefixesID);
	prefixes = fxGetInstance(the, aSlot);
	anIndex = 0;
	aFlag = 0;
	aPrefixAddress = &(aRoot->next->next);
	while ((aPrefix = *aPrefixAddress)) {
		if (aPrefix->value.prefix.part.namespaceID == XS_NO_ID) {
			aFlag = 1;
			*aPrefixAddress = aPrefix->next;
			break;
		}
		aPrefixAddress = &(aPrefix->next);
	}
	aPrefix = aRoot->next->next;
	while (aPrefix) {
		if (anIndex || aFlag) {
			aSlot = fxGetOwnProperty(the, prefixes, aPrefix->value.prefix.part.namespaceID);
			if (aSlot && (aSlot->kind == XS_STRING_KIND)) {
				aString = aSlot->value.string;
				if (aString) {
					aSlot = aRoot->next->next;
					while (aSlot) {
						if (aSlot->value.prefix.string) {
							if (c_strcmp(aString, aSlot->value.prefix.string) == 0) {
								aString = C_NULL;
								break;
							}
						}
						aSlot = aSlot->next;
					}
				}
			}
			else
				aString = C_NULL;
			if (!aString) {
				fxIntegerToString(anIndex, aBuffer + 2, sizeof(aBuffer) - 2);
				aString = (txString)fxNewChunk(the, c_strlen(aBuffer) + 1);
				c_strcpy(aString, aBuffer);
			}
			aPrefix->value.prefix.string = aString;
		}
		anIndex++;
		aPrefix = aPrefix->next;
	}
}

void fxLinkRoots(txMachine* the)
{
	txSlot* aSlot;
	txSlot* grammars;
	txSlot** aSlotAddress;
	txSlot* aGrammar;

	aSlot = fxGetProperty(the, mxGlobal.value.reference, the->grammarsID);
	grammars = fxGetOwnInstance(the, aSlot);
	aSlot = fxGetProperty(the, mxGlobal.value.reference, the->rootsID);
	aSlot = fxGetInstance(the, aSlot);
	aSlot = aSlot->next;
	while (aSlot) {
		aSlotAddress = &(grammars->next);
		while ((aGrammar = *aSlotAddress)) {
			if (aGrammar->value.reference->ID == aSlot->value.alias)
				break;
			aSlotAddress = &(aGrammar->next);
		}
		if (!aGrammar) {
			fxNewInstance(the);
			fxLinkRoot(the, aSlot->value.alias);
			aGrammar = *aSlotAddress = fxNewSlot(the);
			aGrammar->value.reference = the->stack->value.reference;
			aGrammar->kind = the->stack->kind;
			the->stack++;
		}
		aSlot = aSlot->next;
	}
}

txSlot* fxGetGrammarNode(txMachine* the, txSlot* theGrammar, txPatternPart* thePatternPart)
{
	txSlot* aSlot;

	aSlot = theGrammar->next;
	while (aSlot) {
		if ((aSlot->value.node.part.namespaceID == thePatternPart->namespaceID) && (aSlot->value.node.part.nameID == thePatternPart->nameID))
			return aSlot;
		aSlot = aSlot->next;
	}
	return C_NULL;
}

txSlot* fxPutGrammarNode(txMachine* the, txSlot* theGrammar, txPatternPart* thePatternPart)
{
	txSlot* aSlot;

	aSlot = theGrammar;
	while (aSlot->next)
		aSlot = aSlot->next;
	aSlot->next = fxNewSlot(the);
	aSlot = aSlot->next;
	aSlot->ID = XS_NO_ID;
	aSlot->kind = XS_NODE_KIND;
	aSlot->value.node.part.namespaceID = thePatternPart->namespaceID;
	aSlot->value.node.part.nameID = thePatternPart->nameID;
	aSlot->value.node.link = C_NULL;
	return aSlot;
}

txSlot* fxPutRule(txMachine* the, txSlot* theGrammar, txPatternPart* thePatternPart, 
		txSlot* thePattern, txSlot* theStack)
{
	txSlot* aRule;
#ifdef mxDebug
	txPatternData* aPatternData;
#endif
	txRuleData* aRuleData;
	txID* anID;

	aRule = fxPutGrammarNode(the, theGrammar, thePatternPart);
	aRule->kind = thePattern->kind + XS_ATTRIBUTE_RULE - XS_ATTRIBUTE_PATTERN;
	aRule->value.rule.data = (txRuleData *)fxNewChunk(the, sizeof(txRuleData) + ((theStack - the->stack - 1) * sizeof(txID)));
	aRuleData = aRule->value.rule.data;
#ifdef mxDebug
	aPatternData = thePattern->value.pattern;
	aRuleData->pathID = aPatternData->pathID;
	aRuleData->line = aPatternData->line;
#endif
	aRuleData->link = C_NULL;
	aRuleData->alias = thePattern->value.pattern->alias;
	aRuleData->count = theStack - the->stack;
	anID = aRuleData->IDs;
	while (theStack > the->stack) {
		theStack--;
		*anID = theStack->ID;
		anID++;
	}
	return aRule;
}

txBoolean fxCompareRule(txMachine* the, txSlot* theGrammar, txSlot* theRule, 
		txSlot* thePattern, txSlot* theStack)
{
	txRuleData* aRuleData;
	txID* anID;

	if (theRule->kind != thePattern->kind + XS_ATTRIBUTE_RULE - XS_ATTRIBUTE_PATTERN)
		return 0;
	aRuleData = theRule->value.rule.data;
	if (aRuleData->alias != thePattern->value.pattern->alias)
		return 0;
	if (aRuleData->count != theStack - the->stack)
		return 0;
	anID = aRuleData->IDs;
	while (theStack > the->stack) {
		theStack--;
		if (*anID != theStack->ID)
			return 0;
		anID++;
	}
	return 1;
}

#if mxDump
void fxDumpGrammar(txMachine* the, txSlot* theGrammar, txInteger theTabCount)
{
	txSlot* aSlot;

	aSlot = theGrammar->next;
	while (aSlot) {
		fxDumpNode(the, aSlot, theTabCount);
		aSlot = aSlot->next;
	}
}

void fxDumpName(txMachine* the, txSlot* theNode)
{
	txSlot* aSymbol;

	if (theNode->value.node.part.namespaceID != XS_NO_ID) {
		aSymbol = fxGetSymbol(the, theNode->value.node.part.namespaceID);
		if (aSymbol)
			fprintf(stderr, "%s:", aSymbol->value.symbol.string);
		else
			fprintf(stderr, "%d:", theNode->value.node.part.namespaceID);
	}
	aSymbol = fxGetSymbol(the, theNode->value.node.part.nameID);
	if (aSymbol)
		fprintf(stderr, "%s ", aSymbol->value.symbol.string);
	else
		fprintf(stderr, "%d ", theNode->value.node.part.nameID);
	fprintf(stderr, "=> ");
}

void fxDumpNode(txMachine* the, txSlot* theNode, txInteger theTabCount)
{
	txInteger aTabIndex;

	for (aTabIndex = 0; aTabIndex < theTabCount; aTabIndex++)
		fprintf(stderr, "\t");
	switch (theNode->kind) {
	case XS_NODE_KIND:
		fprintf(stderr, "grammar ");
		fxDumpName(the, theNode);
		fprintf(stderr, "\n");
		fxDumpGrammar(the, theNode->value.node.link, theTabCount + 1);
		break;
	case XS_PREFIX_KIND:
		fprintf(stderr, "prefix ");
		fxDumpName(the, theNode);
		if (theNode->value.prefix.string)
			fprintf(stderr, "%s", theNode->value.prefix.string);
		fprintf(stderr, "\n");
		break;
	case XS_ATTRIBUTE_RULE:
		fprintf(stderr, "attribute ");
		fxDumpName(the, theNode);
		fxDumpRule(the, theNode);
		fprintf(stderr, "\n");
		break;
	case XS_DATA_RULE:
		fprintf(stderr, "data ");
		fxDumpName(the, theNode);
		fxDumpRule(the, theNode);
		fprintf(stderr, "\n");
		break;
	case XS_EMBED_RULE:
		fprintf(stderr, "embed ");
		fxDumpRule(the, theNode);
		fprintf(stderr, "\n");
		break;
	case XS_JUMP_RULE:
		fprintf(stderr, "jump ");
		fxDumpName(the, theNode);
		fxDumpRule(the, theNode);
		fprintf(stderr, "\n");
		fxDumpGrammar(the, theNode->value.rule.data->link, 0);
		break;
	case XS_REFER_RULE:
		fprintf(stderr, "refer ");
		fxDumpName(the, theNode);
		fxDumpRule(the, theNode);
		fprintf(stderr, "\n");
		break;
	case XS_REPEAT_RULE:
		fprintf(stderr, "repeat ");
		fxDumpName(the, theNode);
		fxDumpRule(the, theNode);
		fprintf(stderr, "\n");
		break;
	case XS_ERROR_RULE:
		fprintf(stderr, "error ");
		fxDumpName(the, theNode);
		fxDumpRule(the, theNode);
		fprintf(stderr, "\n");
		break;
	default:
		fprintf(stderr, "?\n");
		break;
	}
}

void fxDumpRoots(txMachine* the)
{
	txSlot* aSlot;
	txSlot* aPrototype;

	fprintf(stderr, "############\n");
	aSlot = fxGetProperty(the, mxGlobal.value.reference, the->grammarsID);
	aSlot = fxGetInstance(the, aSlot);
	aSlot = aSlot->next;
	while (aSlot) {
		fxDumpGrammar(the, aSlot->value.reference, 0);
		aSlot = aSlot->next;
	}
}

void fxDumpRule(txMachine* the, txSlot* theNode)
{
	txSlot* aSymbol;
	txInteger aCount = theNode->value.rule.data->count;
	txID* anID = theNode->value.rule.data->IDs;
	while (aCount > 1) {
		aSymbol = fxGetSymbol(the, *anID);
		if (aSymbol)
			fprintf(stderr, "%s.", aSymbol->value.symbol.string);
		else
			fprintf(stderr, "%d.", *anID);
		aCount--;
		anID++;
	}
	aSymbol = fxGetSymbol(the, *anID);
	if (aSymbol)
		fprintf(stderr, "%s=", aSymbol->value.symbol.string);
	else if (*anID == XS_NO_ID)
		fprintf(stderr, "[i]=");
	else
		fprintf(stderr, "%d=", *anID);
	fprintf(stderr, "%8.8lX", (txU4)theNode->value.rule.data->link);
	fprintf(stderr, " %d", (txU4)theNode->value.rule.data->alias);
}

#endif

txString fxSearchPrefix(txMachine* the, txSlot* theRoot, txID theNamespaceID)
{
	txSlot* aPrefix;

	aPrefix = theRoot->next->next;
	while (aPrefix) {
		if (aPrefix->value.rule.part.namespaceID == theNamespaceID)
			return aPrefix->value.prefix.string;
		aPrefix = aPrefix->next;
	}
	return C_NULL;
} 

void fxUnlinkRoots(txMachine* the, txID theNamespaceID)
{
	txSlot* aSlot;
	txSlot** aSlotAddress;
	txSlot* aPrefix;
	
	aSlot = fxGetProperty(the, mxGlobal.value.reference, the->grammarsID);
	aSlot = fxGetOwnInstance(the, aSlot);
	aSlotAddress = &(aSlot->next);
	while ((aSlot = *aSlotAddress)) {
		aPrefix = aSlot->value.reference->next->next;
		while (aPrefix) {
			if (aPrefix->value.rule.part.namespaceID == theNamespaceID)
				break;
			aPrefix = aPrefix->next;
		}
		if (aPrefix)
			*aSlotAddress = aSlot->next;
		else
			aSlotAddress = &(aSlot->next);
	}
} 

#include "xsParse.c"

#include "xsSerialize.c"
