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

#include "xsAll.h"

static void fxGetArgument(txMachine* the);
static void fxSetArgument(txMachine* the);
static void fxThrowTypeError(txMachine* the);
static void fx_decodeURI(txMachine* the);
static void fx_decodeURIComponent(txMachine* the);
static void fx_encodeURI(txMachine* the);
static void fx_encodeURIComponent(txMachine* the);
static void fx_eval(txMachine* the);
static void fx_get_sandbox(txMachine* the);
static void fx_isFinite(txMachine* the);
static void fx_isNaN(txMachine* the);
static void fx_parseInt(txMachine* the);
static void fx_parseFloat(txMachine* the);
static void fx_trace(txMachine* the);

static txByte gxNoCode[10] = { XS_BEGIN, -1, -1, 0, 0, 0, XS_ROUTE, 0, 0, XS_END };

static char gxURIEmptySet[128] = {
  /* 0 1 2 3 4 5 6 7 8 9 A B C D E F */
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 0x                    */
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 1x                    */
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 2x   !"#$%&'()*+,-./  */
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 3x  0123456789:;<=>?  */
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 4x  @ABCDEFGHIJKLMNO  */
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 5X  PQRSTUVWXYZ[\]^_  */
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 6x  `abcdefghijklmno  */
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 	/* 7X  pqrstuvwxyz{|}~   */
};

static char gxURIReservedSet[128] = {
  /* 0 1 2 3 4 5 6 7 8 9 A B C D E F */
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 0x                    */
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 1x                    */
	 0,0,0,1,1,0,1,0,0,0,0,1,1,0,0,1,	/* 2x   !"#$%&'()*+,-./  */
	 0,0,0,0,0,0,0,0,0,0,1,1,0,1,0,1,	/* 3x  0123456789:;<=>?  */
	 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 4x  @ABCDEFGHIJKLMNO  */
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 5X  PQRSTUVWXYZ[\]^_  */
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 6x  `abcdefghijklmno  */
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 	/* 7X  pqrstuvwxyz{|}~   */
};

static char gxURIUnescapedSet[128] = {
  /* 0 1 2 3 4 5 6 7 8 9 A B C D E F */
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 0x                    */
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 1x                    */
	 0,1,0,0,0,0,0,1,1,1,1,0,0,1,1,0,	/* 2x   !"#$%&'()*+,-./  */
	 1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,	/* 3x  0123456789:;<=>?  */
	 0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,	/* 4x  @ABCDEFGHIJKLMNO  */
	 1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,1,	/* 5X  PQRSTUVWXYZ[\]^_  */
	 0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,	/* 6x  `abcdefghijklmno  */
	 1,1,1,1,1,1,1,1,1,1,1,0,0,0,1,0 	/* 7X  pqrstuvwxyz{|}~   */
};

static char gxURIReservedAndUnescapedSet[128] = {
  /* 0 1 2 3 4 5 6 7 8 9 A B C D E F */
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 0x                    */
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 1x                    */
	 0,1,0,1,1,0,1,1,1,1,1,1,1,1,1,1,	/* 2x   !"#$%&'()*+,-./  */
	 1,1,1,1,1,1,1,1,1,1,1,1,0,1,0,1,	/* 3x  0123456789:;<=>?  */
	 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,	/* 4x  @ABCDEFGHIJKLMNO  */
	 1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,1,	/* 5X  PQRSTUVWXYZ[\]^_  */
	 0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,	/* 6x  `abcdefghijklmno  */
	 1,1,1,1,1,1,1,1,1,1,1,0,0,0,1,0 	/* 7X  pqrstuvwxyz{|}~   */
};


void fxBuildGlobal(txMachine* the)
{
	the->argumentsID = (txID)fxID(the, "arguments");
	the->bodyID = (txID)fxID(the, "body");
	the->calleeID = (txID)fxID(the, "callee");
	the->callerID = (txID)fxID(the, "caller");
	the->chunkID = (txID)fxID(the, "(chunk)");
	the->configurableID = (txID)fxID(the, "configurable");
	the->constructorID = (txID)fxID(the, "constructor");
	the->enumerableID = (txID)fxID(the, "enumerable");
	the->evalID = (txID)fxID(the, "eval");
	the->execID = (txID)fxID(the, "exec");
	the->getID = (txID)fxID(the, "get");
	the->globalID = (txID)fxID(the, "global");
	the->grammarID = (txID)fxID(the, "__xs__grammar");
	the->grammarsID = (txID)fxID(the, "__xs__grammars");
	the->indexID = (txID)fxID(the, "index");
	the->inputID = (txID)fxID(the, "input");
	the->instanceID = (txID)fxID(the, "instance");
	the->instancesID = (txID)fxID(the, "__xs__instances");
	the->lengthID = (txID)fxID(the, "length");
	the->lineID = (txID)fxID(the, "__xs__line");
	the->nameID = (txID)fxID(the, "name");
	the->namespaceID = (txID)fxID(the, "namespace");
	the->parentID = (txID)fxID(the, "parent");
	the->parseID = (txID)fxID(the, "parse");
	the->pathID = (txID)fxID(the, "__xs__path");
	the->patternsID = (txID)fxID(the, "__xs__patterns");
	the->peekID = (txID)fxID(the, "peek");
	the->pokeID = (txID)fxID(the, "poke");
	the->prefixID = (txID)fxID(the, "prefix");
	the->prefixesID = (txID)fxID(the, "__xs__prefixes");
	the->prototypeID = (txID)fxID(the, "prototype");
	the->rootsID = (txID)fxID(the, "__xs__roots");
	the->sandboxID = (txID)fxID(the, "sandbox");
	the->serializeID = (txID)fxID(the, "serialize");
	the->setID = (txID)fxID(the, "set");
	the->toJSONID = (txID)fxID(the, "toJSON");
	the->toStringID = (txID)fxID(the, "toString");
	the->valueID = (txID)fxID(the, "value");
	the->valueOfID = (txID)fxID(the, "valueOf");
	the->writableID = (txID)fxID(the, "writable");

	the->attributeID = (txID)fxID(the, "attribute");
	the->cdataID = (txID)fxID(the, "cdata");
	the->childrenID = (txID)fxID(the, "children");
	the->commentID = (txID)fxID(the, "comment");
	the->compareAttributesID = (txID)fxID(the, "compareAttributes");
	the->elementID = (txID)fxID(the, "element");
	the->piID = (txID)fxID(the, "pi");
	the->pushID = (txID)fxID(the, "push");
	the->sortID = (txID)fxID(the, "sort");
	the->xmlnsAttributesID = (txID)fxID(the, "xmlnsAttributes");
	the->xmlnsNamespaceID = (txID)fxID(the, "xmlnsNamespace");
	the->xmlnsPrefixID = (txID)fxID(the, "xmlnsPrefix");
	the->_attributesID = (txID)fxID(the, "_attributes");
	the->__xs__infosetID = (txID)fxID(the, "__xs__infoset");

	/* -6 */
	mxZeroSlot(--the->stack);
	the->stack->value.code = (txByte *)fxNewChunk(the, sizeof(gxNoCode));
	c_memcpy(the->stack->value.code, gxNoCode, sizeof(gxNoCode));
	the->stack->kind = XS_CODE_KIND;	
	/* -7 */
	mxZeroSlot(--the->stack);
	fxCopyStringC(the, the->stack, "");
	/* -8 */
	mxZeroSlot(--the->stack);
	/* -9 */
	mxZeroSlot(--the->stack);
	the->stack->value.alias = 1; /* for fxNewHostConstructor and fxNewHostFunction */
	/* -10 */
	mxZeroSlot(--the->stack);
	/* -11 */
	mxZeroSlot(--the->stack);
	/* -12 */
	mxZeroSlot(--the->stack);
	/* -13 */
	mxZeroSlot(--the->stack);
	/* -14 */
	mxZeroSlot(--the->stack);
	/* -15 */
	mxZeroSlot(--the->stack);
	/* -16 */
	mxZeroSlot(--the->stack);
	/* -17 */
	mxZeroSlot(--the->stack);
	/* -18 */
	mxZeroSlot(--the->stack);
	/* -19 */
	mxZeroSlot(--the->stack);
	/* -20 */
	mxZeroSlot(--the->stack);
	/* -21 */
	mxZeroSlot(--the->stack);
	/* -22 */
	mxZeroSlot(--the->stack);
	/* -23 */
	mxZeroSlot(--the->stack);
	/* -24 */
	mxZeroSlot(--the->stack);
	/* -25 */
	fxNewHostFunction(the, fxGetArgument, 0);
	/* -26 */
	fxNewHostFunction(the, fxSetArgument, 1);
	/* -27 */
	fxNewHostFunction(the, fxThrowTypeError, 0);
		
	mxPush(mxGlobal);
	mxPushNumber(C_INFINITY);
	fxQueueID(the, fxID(the, "Infinity"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	mxPushNumber(C_NAN);
	fxQueueID(the, fxID(the, "NaN"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	mxZeroSlot(--the->stack);
	fxQueueID(the, fxID(the, "undefined"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	fxNewHostFunction(the, fx_get_sandbox, 0);
	fxQueueID(the, fxID(the, "sandbox"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG | XS_DONT_SCRIPT_FLAG | XS_GETTER_FLAG);
	fxNewHostFunction(the, fx_decodeURI, 1);
	fxQueueID(the, fxID(the, "decodeURI"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_decodeURIComponent, 1);
	fxQueueID(the, fxID(the, "decodeURIComponent"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_encodeURI, 1);
	fxQueueID(the, fxID(the, "encodeURI"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_encodeURIComponent, 1);
	fxQueueID(the, fxID(the, "encodeURIComponent"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_eval, 1);
	fxQueueID(the, fxID(the, "eval"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_isFinite, 1);
	fxQueueID(the, fxID(the, "isFinite"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_isNaN, 1);
	fxQueueID(the, fxID(the, "isNaN"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_parseInt, 2);
	fxQueueID(the, fxID(the, "parseInt"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_parseFloat, 1);
	fxQueueID(the, fxID(the, "parseFloat"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_trace, 1);
	fxQueueID(the, fxID(the, "trace"), XS_DONT_ENUM_FLAG);

	the->stack++;
}

void fxGetArgument(txMachine* the)
{
	txIndex aCount;
	txSlot* aFrame;
	txSlot* aSlot;
/*	
	mxPush(*mxFunction);
	fxGetID(the, fxID(the, "@")); 
	aSlot = the->stack->value.reference;
	mxResult->kind = aSlot->kind;
	mxResult->value = aSlot->value;
*/	
	mxPush(*mxThis);
	fxGetID(the, fxID(the, "caller")); 
	aCount = the->stack->value.array.length;
	aFrame = the->stack->value.array.address;
	aSlot = aFrame + 4 + aCount - the->frame->ID;
	mxResult->kind = aSlot->kind;
	mxResult->value = aSlot->value;
}

void fxSetArgument(txMachine* the)
{
	txIndex aCount;
	txSlot* aFrame;
	txSlot* aSlot;
/*	
	mxPush(*mxFunction);
	fxGetID(the, fxID(the, "@")); 
	aSlot = the->stack->value.reference;
	aSlot->kind = mxArgv(0)->kind;
	aSlot->value = mxArgv(0)->value;
*/	
	mxPush(*mxThis);
	fxGetID(the, fxID(the, "caller")); 
	aCount = the->stack->value.array.length;
	aFrame = the->stack->value.array.address;
	aSlot = aFrame + 4 + aCount - the->frame->ID;
	aSlot->kind = mxArgv(0)->kind;
	aSlot->value = mxArgv(0)->value;
}

void fxThrowTypeError(txMachine* the)
{
	mxDebug0(the, XS_TYPE_ERROR, "no access in strict code");
}

void fx_decodeURI(txMachine* the)
{
	if (mxArgc < 1)
		mxDebug0(the, XS_SYNTAX_ERROR, "decodeURI: no URI parameter");
	fxToString(the, mxArgv(0));
	fxDecodeURI(the, gxURIReservedSet);
}

void fx_decodeURIComponent(txMachine* the)
{
	if (mxArgc < 1)
		mxDebug0(the, XS_SYNTAX_ERROR, "decodeURIComponent: no URI Component parameter");
	fxToString(the, mxArgv(0));
	fxDecodeURI(the, gxURIEmptySet);
}

void fx_encodeURI(txMachine* the)
{
	if (mxArgc < 1)
		mxDebug0(the, XS_SYNTAX_ERROR, "encodeURI: no URI parameter");
	fxEncodeURI(the, gxURIReservedAndUnescapedSet);
}

void fx_encodeURIComponent(txMachine* the)
{
	if (mxArgc < 1)
		mxDebug0(the, XS_SYNTAX_ERROR, "encodeURIComponent: no URI Component parameter");
	fxEncodeURI(the, gxURIUnescapedSet);
}

void fx_eval(txMachine* the)
{
	txFlag aFlag;
	txSlot* aSlot;
	txStringStream aStream;

	if (mxArgc < 1)
		mxDebug0(the, XS_SYNTAX_ERROR, "eval: no x parameter");
	if (mxArgv(0)->kind != XS_STRING_KIND) {
		*mxResult = *mxArgv(0);
		return;
	}
	the->scope = the->frame->value.frame.scope;
	aFlag = XS_EVAL_FLAG;
	if ((the->frame->ID == the->evalID) && (mxThis->kind == XS_UNDEFINED_KIND))
		aFlag |= the->frame->next->flag & XS_STRICT_FLAG;
	aSlot = fxGetInstance(the, mxThis);
	if (aSlot && (aSlot->flag & XS_SANDBOX_FLAG))
		aFlag |= XS_SANDBOX_FLAG;
	else
		aFlag |= the->frame->next->flag & XS_SANDBOX_FLAG;
	aStream.slot = mxArgv(0);
	aStream.offset = 0;
	aStream.size = c_strlen(fxToString(the, mxArgv(0)));
	/* ARGC */
	mxZeroSlot(--the->stack);
	the->stack->kind = XS_INTEGER_KIND;
	the->stack->value.integer = 0;
	/* THIS */
	*(--the->stack) = *(the->frame->next + 3);
	/* FUNCTION */
	mxZeroSlot(--the->stack);
	the->stack->value.code = fxParseScript(the, &aStream, fxStringGetter, C_NULL, 0, aFlag, C_NULL);
	the->stack->kind = XS_CODE_KIND;
	fxNewProgramInstance(the);
	/* RESULT */
	mxZeroSlot(--the->stack);
	fxRunID(the, XS_NO_ID);
	*mxResult = *(the->stack++);
}

void fx_get_sandbox(txMachine* the)
{
	txSlot* anInstance;

	anInstance = fxNewSlot(the);
	anInstance->next = C_NULL;
	anInstance->ID = XS_NO_ID;
	anInstance->flag = XS_SANDBOX_FLAG;
	anInstance->kind = XS_INSTANCE_KIND;
	anInstance->value.instance.garbage = C_NULL;
	anInstance->value.instance.prototype = mxGlobal.value.reference;
	mxResult->value.reference = anInstance;
	mxResult->kind = XS_REFERENCE_KIND;
}

void fx_isFinite(txMachine* the)
{
	int fpclass;
	if (mxArgc < 1)
		mxDebug0(the, XS_SYNTAX_ERROR, "isFinite: no number parameter");
	fxToNumber(the, mxArgv(0));
	mxResult->kind = XS_BOOLEAN_KIND;
	fpclass = c_fpclassify(mxArgv(0)->value.number);
	if ((fpclass != FP_NAN) && (fpclass != FP_INFINITE))
		mxResult->value.boolean = 1;
}

void fx_isNaN(txMachine* the)
{
	int fpclass;
	if (mxArgc < 1)
		mxDebug0(the, XS_SYNTAX_ERROR, "isNaN: no number parameter");
	fxToNumber(the, mxArgv(0));
	mxResult->kind = XS_BOOLEAN_KIND;
	fpclass = c_fpclassify(mxArgv(0)->value.number);
	if (fpclass == FP_NAN)
		mxResult->value.boolean = 1;
}

void fx_parseInt(txMachine* the)
{
	txInteger aRadix, aDigit;
	txNumber aSign, aResult;
	txString s, r;
	char c;
	
	if (mxArgc < 1)
		mxDebug0(the, XS_SYNTAX_ERROR, "parseInt: no string parameter");
	fxToString(the, mxArgv(0));
	if (mxArgc > 1) {
		aRadix = fxToInteger(the, mxArgv(1));
		if (aRadix) {
			if ((aRadix < 2) || (36 < aRadix)) {
				mxResult->kind = XS_NUMBER_KIND;
				mxResult->value.number = C_NAN;
				return;
			}
		}
	}
	else
		aRadix = 0;
	s = mxArgv(0)->value.string;
	while ((c = *s)) {
		if ((c != ' ') && (c != '\f') && (c != '\n') && (c != '\r')
				&& (c != '\t') && (c != '\v'))
			break;
		s++;
	}
	aSign = 1;
	if (c == '+')
		s++;
	else if (c == '-') {
		s++;
		aSign = -1;
	}
	if ((*s == '0') && ((*(s + 1) == 'x') || (*(s + 1) == 'X'))) {
		if ((aRadix == 0) || (aRadix == 16)) {
			aRadix = 16;
			s += 2;
		}
	}
	if (aRadix == 0)
		aRadix = 10;
	aResult = 0;
	r = s;
	while ((c = *s)) {
		if (('0' <= c) && (c <= '9'))
			aDigit = c - '0';
		else if (('a' <= c) && (c <= 'z'))
			aDigit = 10 + c - 'a';
		else if (('A' <= c) && (c <= 'Z'))
			aDigit = 10 + c - 'A';
		else
			break;
		if (aDigit >= aRadix)
			break;
		aResult = (aResult * aRadix) + aDigit;
		s++;
	}
	if (r == s) {
		mxResult->kind = XS_NUMBER_KIND;
		mxResult->value.number = C_NAN;
	}
	else {
		aResult *= aSign;
		aRadix = (txInteger)aResult;
		aSign = aRadix;
		if (aSign == aResult) {
			mxResult->value.integer = aRadix;
			mxResult->kind = XS_INTEGER_KIND;
		}
		else {
			mxResult->value.number = aResult;
			mxResult->kind = XS_NUMBER_KIND;
		}
	}
}

void fx_parseFloat(txMachine* the)
{
	if (mxArgc < 1)
		mxDebug0(the, XS_SYNTAX_ERROR, "parseFloat: no string parameter");
	fxToString(the, mxArgv(0));
	mxResult->kind = XS_NUMBER_KIND;
	mxResult->value.number = fxStringToNumber(the, mxArgv(0)->value.string, 0);
}

void fx_trace(txMachine* the)
{
#if mxDebug
	if (mxArgc > 0) {
		fxToString(the, mxArgv(0));
		fxReport(the, NULL, 0, "%s", mxArgv(0)->value.string);
	}
#endif
}

static txS1* gxURIHexa = (txS1*)"0123456789ABCDEF";

#define mxURIXDigit(X) \
	((('0' <= (X)) && ((X) <= '9')) \
		? ((X) - '0') \
		: ((('a' <= (X)) && ((X) <= 'f')) \
			? (10 + (X) - 'a') \
			: (10 + (X) - 'A')))
			
void fxDecodeURI(txMachine* the, txString theSet)
{
	txU1* src;
	txS4 size;
	txU1 c, d;
	txS1 *result;
	txU1* dst;
	
	src = (txU1*)mxArgv(0)->value.string;
	size = 0;
	while ((c = *src++)) {
		if (c == '%') {
			c = *src++;
			if (c == 0)
				mxDebug0(the, XS_URI_ERROR, "invalid URI");
			d = mxURIXDigit(c) << 4;
			c = *src++;
			if (c == 0)
				mxDebug0(the, XS_URI_ERROR, "invalid URI");
			d += mxURIXDigit(c);
			if ((d < 128) && (theSet[(int)d]))
				size += 3;
			else
				size += 1;
		}
		else
			size += 1;
	}		
	size += 1;

	if (size == (src - (txU1*)mxArgv(0)->value.string)) {
		mxResult->value.string = mxArgv(0)->value.string;
		mxResult->kind = XS_STRING_KIND;
		return;
	}

	result = fxNewChunk(the, size);
	
	src = (txU1*)mxArgv(0)->value.string;
	dst = (txU1*)result;
	while ((c = *src++)) {
		if (c == '%') {
			c = *src++;
			d = mxURIXDigit(c) << 4;
			c = *src++;
			d += mxURIXDigit(c);
			if ((d < 128) && (theSet[(int)d])) {
				*dst++ = *(src - 3);
				*dst++ = *(src - 2);
				*dst++ = *(src - 1);
			}
			else
				*dst++ = d;
		}
		else
			*dst++ = c;
	}
	*dst = 0;
	
	mxResult->value.string = (char *)result;
	mxResult->kind = XS_STRING_KIND;
}

void fxEncodeURI(txMachine* the, txString theSet)
{
	txU1* src;
	txS4 size;
	txU1 c;
	txS1 *result;
	txU1* dst;

	fxToString(the, mxArgv(0));

	src = (txU1*)mxArgv(0)->value.string;
	size = 0;
	while ((c = *src++)) {
		if ((c < 128) && (theSet[(int)c]))
			size += 1;
		else
			size += 3;
	}
	size += 1;

	if (size == (src - (txU1*)mxArgv(0)->value.string)) {
		mxResult->value.string = mxArgv(0)->value.string;
		mxResult->kind = XS_STRING_KIND;
		return;
	}

	result = fxNewChunk(the, size);

	src = (txU1*)mxArgv(0)->value.string;
	dst = (txU1*)result;
	while ((c = *src++)) {
		if ((c < 128) && (theSet[(int)c]))
			*dst++ = c;
		else {
			*dst++ = '%';
			*dst++ = gxURIHexa[c >> 4];
			*dst++ = gxURIHexa[c & 15];
		}
	}
	*dst = 0;
	
	mxResult->value.string = (char *)result;
	mxResult->kind = XS_STRING_KIND;
}



