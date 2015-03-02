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

#ifndef mxDump
#define mxDump 0
#endif

enum {
	/* literal flag */
	XS_DATA_LITERAL = 0,
	XS_ATTRIBUTE_LITERAL = 1,
	XS_PI_LITERAL = 2
};

enum {
	/* serialize flag */
	XS_ATTRIBUTE_FLAG = 8,
	XS_DATA_FLAG = 16,
	XS_CONTENTS_FLAG = 32
};

static void fx_grammar_array(txMachine* the);
static void fx_grammar_boolean(txMachine* the);
static void fx_grammar_chunk(txMachine* the);
static void fx_grammar_custom(txMachine* the);
static void fx_grammar_date(txMachine* the);
static void fx_grammar_link(txMachine* the);
static void fx_grammar_namespace(txMachine* the);
static void fx_grammar_number(txMachine* the);
static void fx_grammar_object(txMachine* the);
static void fx_grammar_reference(txMachine* the);
static void fx_grammar_regexp(txMachine* the);
static void fx_grammar_script(txMachine* the);
static void fx_grammar_string(txMachine* the);

static void fx_grammar_object_step(txMachine* the, txSlot* theContext, txID theID, txSlot* theProperty) ;
static txBoolean fxMakePattern(txMachine* the, txSlot* theSlot, txKind theKind, txString theName);
static void fxMakePatternStep(txMachine* the, txSlot* theContext, txID theID, txSlot* theProperty);

static void fx_Grammar(txMachine* the);
static void fx_Grammar_parse(txMachine* the);

static void fx_link(txMachine* the);
static void fx_patch(txMachine* the);
static void fx_pattern(txMachine* the);
static void fx_prefix(txMachine* the);
static void fx_root(txMachine* the);

static void fx_xs_isInstanceOf(txMachine* the);
static void fx_xs_newInstanceOf(txMachine* the);

static void fx_xs_parse(txMachine* the);
static void fx_xs_parsePrefix(txMachine* the);
static void fx_xs_serialize(txMachine* the);
static void fx_xs_serializeNamespace(txMachine* the);
static void fx_xs_script(txMachine* the);

#ifdef mxProfile
static void fx_xs_isProfiling(txMachine* the);
static void fx_xs_getProfilingDirectory(txMachine* the);
static void fx_xs_setProfilingDirectory(txMachine* the);
static void fx_xs_startProfiling(txMachine* the);
static void fx_xs_stopProfiling(txMachine* the);
#endif

static void fx_all_serialize(txMachine* the);
static void fx_boolean_parse(txMachine* the);
static void fx_chunk_parse(txMachine* the);
static void fx_date_parse(txMachine* the);
static void fx_number_parse(txMachine* the);
static void fx_regexp_parse(txMachine* the);
static void fx_regexp_serialize(txMachine* the);
static void fx_script_parse(txMachine* the);
static void fx_string_parse(txMachine* the);

static void fxLinkContents(txMachine* the, txSlot* theGrammar, txID theAlias, txSlot* theStack);
static void fxLinkError(txMachine* the, txSlot* theGrammar, txPatternPart* thePatternPart, 
		txSlot* thePattern, txSlot* theStack);
static void fxLinkNamespace(txMachine* the, txSlot* theRoot, txID theNamespaceID);
static void fxLinkPattern(txMachine* the, txSlot* theGrammar, txSlot* thePattern, txSlot* theStack);
static void fxLinkPatterns(txMachine* the, txSlot* theGrammar, txID theAlias, txSlot* theStack);
static void fxLinkPrefixes(txMachine* the, txSlot* theRoot, txSlot* theGrammar);
static txSlot* fxLinkPrototype(txMachine* the, txID theAlias);
static void fxLinkRoot(txMachine* the, txID theAlias);
static void fxLinkRoots(txMachine* the);
static txSlot* fxGetGrammarNode(txMachine* the, txSlot* theGrammar, txPatternPart* thePatternPart);
static txSlot* fxPutGrammarNode(txMachine* the, txSlot* theGrammar, txPatternPart* thePatternPart);
static txSlot* fxPutRule(txMachine* the, txSlot* theGrammar, txPatternPart* thePatternPart, 
		txSlot* thePattern, txSlot* theStack);
static txBoolean fxCompareRule(txMachine* the, txSlot* theGrammar, txSlot* theRule, 
		txSlot* thePattern, txSlot* theStack);
static void fxUnlinkRoots(txMachine* the, txID theNamespaceID);

#if mxDump
static void fxDumpGrammar(txMachine* the, txSlot* theGrammar, txInteger theTabCount);
static void fxDumpRule(txMachine* the, txSlot* theNode);
static void fxDumpName(txMachine* the, txSlot* theNode);
static void fxDumpNode(txMachine* the, txSlot* theNode, txInteger theTabCount);
static void fxDumpRoots(txMachine* the);
#endif


static void fxProcessAttribute(txMachine* the, txSlot* theInstance, txSlot* theGrammar, 
		txID theNamespaceID, txString theName, txString theValue);
static void fxProcessAttributes(txMachine* the, txSlot* theInstance, txSlot* theGrammar, 
		txID theNamespaceID, txSlot* theAttributes);
static void fxProcessLiteral(txMachine* the, txString theValue, txSlot* theNode, txSlot* theProperty);
static void fxProcessNamespaces(txMachine* the);
static void fxProcessPI(txMachine* the);
static txSlot* fxProcessRule(txMachine* the, txSlot* theInstance, txSlot* theRule, txID* theID);
static void fxProcessStartTag(txMachine* the);
static void fxProcessStopTag(txMachine* the);
static void fxProcessText(txMachine* the);
static void fxReportGrammarError(txMachine* the, char* theFormat, ...);
static txID fxSearchNamespace(txMachine* the, char* thePrefix, int theLength);

static txBoolean fxSerializeContent(txMachine* the, txSerializer* theSerializer, txSlot* theProperty, 
		txSlot* theNode, txID theParent);
static txBoolean fxSerializeContents(txMachine* the, txSerializer* theSerializer, txSlot* theProperty, txSlot* theNode);
static void fxSerializeGrammar(txMachine* the, txSerializer* theSerializer, txSlot* theInstance, txSlot* theGrammar);
static void fxSerializeEntity(txMachine* the, txSerializer* theSerializer, char* theText, txFlag theFlag);
static void fxSerializeLiteral(txMachine* the, txSerializer* theSerializer, txSlot* theProperty, txSlot* theNode, txFlag theFlag);
static void fxSerializeNode(txMachine* the, txSerializer* theSerializer, txSlot* theProperty, txSlot* theNode, txBoolean exist);
static txSlot* fxSerializeRule(txMachine* the, txSlot* theInstance, txSlot* theRule);
static void fxSerializeStart(txMachine* the, txSerializer* theSerializer, int theState);
static void fxSerializeStop(txMachine* the, txSerializer* theSerializer);
static void fxSerializeTag(txMachine* the, txSerializer* theSerializer, txSlot* theNode, txByte xmlns);
static void fxSerializeText(txMachine* the, txSerializer* theSerializer, txString theText);

txMarkupCallbacks gxGrammarMarkupCallbacks = {
	fxProcessStartTag,
	fxProcessStopTag,
	fxProcessText,
	C_NULL,
	C_NULL,
	fxProcessPI,
	C_NULL
};
