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

static void xsc_addHostConstructor(txMachine* the);
static void xsc_addHostFunction(txMachine* the);
static void xsc_addHostObject(txMachine* the);
static void xsc_compareFiles(txMachine* the);
static void xsc_countParams(txMachine* the);
static void xsc_createDirectory(txMachine* the);
static void xsc_getModuleCode(txMachine* the);
static void xsc_getPath(txMachine* the);
static void xsc_getPlatform(txMachine* the);
static void xsc_getProgramCode(txMachine* the);
static void xsc_getStringCode(txMachine* the);
static void xsc_hasTarget(txMachine* the);
static void xsc_insertProperty(txMachine* the);
static void xsc_load(txMachine* the);
static void xsc_loadPackage(txMachine* the);
static void xsc_makePath(txMachine* the);
static void xsc_makePattern(txMachine* the);
static void xsc_print(txMachine* the);
static void xsc_reportError(txMachine* the);
static void xsc_reportWarning(txMachine* the);
static void xsc_resolvePath(txMachine* the);
static void xsc_save(txMachine* the);
static void xsc_searchPath(txMachine* the);
static void xsc_searchProperty(txMachine* the);
static void xsc_splitPath(txMachine* the);
static void xsc_trimString(txMachine* the);
static void xsc_parseArguments(txMachine *the, int argc, char *argv[]);
static void xsc_parseCommandFile(txMachine *the, char *commandFilePath, int *argumentCount, char ***argumentList);
static long xsc_getNextArgumentLength(char *arguments);

static void xsc_main(txMachine* the, int argc, char* argv[]);
static void xsc_throw(txMachine* the, char* theFormat, ...);

#if mxWindows
#define mxSeparator '\\'
#else
#define mxSeparator '/'
#endif

#define mxPropertyModulo 1993
txSlot* gxProperties = NULL;

void fxBuild_xsc(txMachine* the)
{
	mxPush(mxGlobal);

	mxPush(mxObjectPrototype);
	fxNewObjectInstance(the);
	mxPushBoolean(0);
	fxQueueID(the, fxID(the, "binary"), 0);
	mxPushBoolean(0);
	fxQueueID(the, fxID(the, "debug"), 0);
	mxPushInteger(0);
	fxQueueID(the, fxID(the, "tree"), 0);
	mxPushInteger(0);
	fxQueueID(the, fxID(the, "errorCount"), 0);
	mxPushNull();
	fxQueueID(the, fxID(the, "grammarPath"), 0);
	mxPush(mxArrayPrototype);
	fxNewArrayInstance(the);
	mxPushNull();
	fxQueueID(the, 0, 0);
	fxQueueID(the, fxID(the, "inputPaths"), 0);
	mxPushBoolean(0);
	fxQueueID(the, fxID(the, "make"), 0);
	mxPushBoolean(0);
	fxQueueID(the, fxID(the, "module"), 0);
	mxPushNull();
	fxQueueID(the, fxID(the, "outputPath"), 0);
	mxPush(mxArrayPrototype);
	fxNewArrayInstance(the);
	fxQueueID(the, fxID(the, "packages"), 0);
	mxPushBoolean(0);
	fxQueueID(the, fxID(the, "program"), 0);
	mxPush(mxArrayPrototype);
	fxNewArrayInstance(the);
	fxQueueID(the, fxID(the, "scriptPaths"), 0);
	mxPush(mxObjectPrototype);
	fxNewObjectInstance(the);
	fxQueueID(the, fxID(the, "targets"), 0);
	mxPushBoolean(0);
	fxQueueID(the, fxID(the, "verbose"), 0);
	mxPushBoolean(0);
	fxQueueID(the, fxID(the, "xsIDFlag"), 0);
	mxPushInteger(0);
	fxQueueID(the, fxID(the, "warningCount"), 0);
	fxNewHostFunction(the, xsc_addHostConstructor, 0);
	fxQueueID(the, fxID(the, "addHostConstructor"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, xsc_addHostFunction, 0);
	fxQueueID(the, fxID(the, "addHostFunction"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, xsc_addHostObject, 0);
	fxQueueID(the, fxID(the, "addHostObject"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, xsc_compareFiles, 0);
	fxQueueID(the, fxID(the, "compareFiles"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, xsc_countParams, 0);
	fxQueueID(the, fxID(the, "countParams"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, xsc_createDirectory, 0);
	fxQueueID(the, fxID(the, "createDirectory"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, xsc_getModuleCode, 0);
	fxQueueID(the, fxID(the, "getModuleCode"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, xsc_getPath, 0);
	fxQueueID(the, fxID(the, "getPath"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, xsc_getPlatform, 0);
	fxQueueID(the, fxID(the, "getPlatform"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, xsc_getProgramCode, 0);
	fxQueueID(the, fxID(the, "getProgramCode"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, xsc_getStringCode, 0);
	fxQueueID(the, fxID(the, "getStringCode"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, xsc_hasTarget, 0);
	fxQueueID(the, fxID(the, "hasTarget"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, xsc_insertProperty, 0);
	fxQueueID(the, fxID(the, "insertProperty"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, xsc_load, 0);
	fxQueueID(the, fxID(the, "load"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, xsc_loadPackage, 0);
	fxQueueID(the, fxID(the, "loadPackage"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, xsc_makePath, 0);
	fxQueueID(the, fxID(the, "makePath"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, xsc_makePattern, 0);
	fxQueueID(the, fxID(the, "makePattern"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, xsc_print, 0);
	fxQueueID(the, fxID(the, "print"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, xsc_reportError, 0);
	fxQueueID(the, fxID(the, "reportError"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, xsc_reportWarning, 0);
	fxQueueID(the, fxID(the, "reportWarning"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, xsc_resolvePath, 0);
	fxQueueID(the, fxID(the, "resolvePath"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, xsc_save, 0);
	fxQueueID(the, fxID(the, "save"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, xsc_searchPath, 0);
	fxQueueID(the, fxID(the, "searchPath"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, xsc_searchProperty, 0);
	fxQueueID(the, fxID(the, "searchProperty"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, xsc_splitPath, 0);
	fxQueueID(the, fxID(the, "splitPath"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, xsc_trimString, 0);
	fxQueueID(the, fxID(the, "trimString"), XS_DONT_ENUM_FLAG);
	fxQueueID(the, fxID(the, "xsc"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);

	the->stack++;
}

void xsc_addHostConstructor(txMachine* the)
{
	xsIntegerValue c;

	xsVars(1);
	xsResult = xsNewInstanceOf(xsObjectPrototype);
	xsSet(xsResult, xsID("selector"), xsInteger(1));
	xsSet(xsResult, xsID("name"), xsArg(0));
	xsSet(xsResult, the->lengthID, xsCall1(xsThis, xsID("countParams"), xsArg(1)));
	xsSet(xsResult, xsID("code"), xsArg(2));
	xsVar(0) = xsGet(xsThis, xsID("hosts"));
	c = xsToInteger(xsGet(xsVar(0), the->lengthID));
	xsSetAt(xsVar(0), xsInteger(c), xsResult);
	xsResult = xsInteger(c);
}

void xsc_addHostFunction(txMachine* the)
{
	xsIntegerValue c;

	xsVars(1);
	xsResult = xsNewInstanceOf(xsObjectPrototype);
	xsSet(xsResult, xsID("selector"), xsInteger(0));
	xsSet(xsResult, xsID("name"), xsArg(0));
	xsSet(xsResult, the->lengthID, xsCall1(xsThis, xsID("countParams"), xsArg(1)));
	xsSet(xsResult, xsID("code"), xsArg(2));
	xsVar(0) = xsGet(xsThis, xsID("hosts"));
	c = xsToInteger(xsGet(xsVar(0), the->lengthID));
	xsSetAt(xsVar(0), xsInteger(c), xsResult);
	xsResult = xsInteger(c);
}

void xsc_addHostObject(txMachine* the)
{
	xsIntegerValue c;

	xsVars(1);
	xsResult = xsNewInstanceOf(xsObjectPrototype);
	xsSet(xsResult, xsID("selector"), xsInteger(2));
	xsSet(xsResult, xsID("name"), xsArg(0));
	xsSet(xsResult, the->lengthID, xsInteger(0));
	xsSet(xsResult, xsID("code"), xsUndefined);
	xsVar(0) = xsGet(xsThis, xsID("hosts"));
	c = xsToInteger(xsGet(xsVar(0), the->lengthID));
	xsSetAt(xsVar(0), xsInteger(c), xsResult);
	xsResult = xsInteger(c);
}

void xsc_compareFiles(txMachine* the)
{
	char *aPath1 = NULL;
	char *aPath2 = NULL;

	aPath1 = xsToString(xsArg(0));
	aPath2 = xsToString(xsArg(1));
#if mxWindows
	xsResult = xsInteger(_stricmp(aPath1, aPath2) == 0);
#elif mxMacOSX
	xsResult = xsInteger(strcasecmp(aPath1, aPath2) == 0);
#else
	xsResult = xsInteger(strcmp(aPath1, aPath2) == 0);
#endif
}

void xsc_countParams(txMachine* the)
{
	if (strlen(xsToString(xsArg(0)))) {
		xsResult = xsCall1(xsArg(0), xsID("split"), xsString(","));
		xsResult = xsGet(xsResult, the->lengthID);
	}
	else
		xsResult = xsInteger(0);
}

void xsc_createDirectory(txMachine* the)
{
	char* aPath;

	aPath = xsToString(xsArg(0));
#if mxWindows
	_mkdir(aPath);
#else
	mkdir(aPath, 0744);
#endif
	xsResult = xsArg(0);
}

typedef struct {
	txString arguments;
	txBoolean body;
	FILE* file;
	txInteger offset;
	txInteger size;
} xsc_getModuleCodeStream;

int xsc_getModuleCodeStreamGetter(void* theStream)
{
	xsc_getModuleCodeStream* aStream = (xsc_getModuleCodeStream*)theStream;
	int result = C_EOF;
	if (aStream->offset < aStream->size) {
		result = *(aStream->arguments + aStream->offset);
		aStream->offset++;
	}
	else if (aStream->offset == aStream->size) {
		result = '{';
		aStream->offset++;
	}
	else if (aStream->body) {
		result = fgetc(aStream->file);
		if (result == C_EOF) {
			result = '}';
			aStream->body = 0;
		}
	}
	return result;
}

void xsc_getModuleCode(txMachine* the)
{
	xsc_getModuleCodeStream aStream;
	txFlag aFlag;
	txSize aSize;
	xsVars(1);

	xsVar(0) = xsGet(xsThis, xsID("arguments"));
	aStream.arguments = xsTest(xsVar(0)) ? xsToString(xsVar(0)) : "(require, exports, module)";
	aStream.body = 1;
	aStream.file = fopen(xsToString(xsArg(0)), "r");
	xsElseError(aStream.file);
	aStream.offset = 0;
	aStream.size = c_strlen(aStream.arguments);
	aFlag = XS_SANDBOX_FLAG;
	if (xsToBoolean(xsGet(xsThis, xsID("debug"))))
		aFlag |= XS_DEBUG_FLAG;
	if (xsToBoolean(xsArg(1))) {
		xsVar(0).value.string = (txString)fxParseTree(the, &aStream, (txGetter)xsc_getModuleCodeStreamGetter,
				fxNewSymbol(the, &xsArg(0)), 1, aFlag, &aSize);
		xsVar(0).kind = XS_STRING_KIND;
	}
	else {
		xsVar(0).value.code = fxParseScript(the, &aStream, (txGetter)xsc_getModuleCodeStreamGetter,
				fxNewSymbol(the, &xsArg(0)), 1, aFlag, &aSize);
		xsVar(0).kind = XS_CODE_KIND;
	}
	fclose(aStream.file);
	xsResult = xsNewInstanceOf(xsObjectPrototype);
	xsSet(xsResult, xsID("code"), xsVar(0));
	xsSet(xsResult, xsID("size"), xsInteger(aSize));
}

void xsc_getProgramCode(txMachine* the)
{
	char* aPath;
	FILE* aFile = NULL;
	txFlag aFlag;
	txSize aSize;
	xsVars(1);

	aPath = xsToString(xsArg(0));
	aFile = fopen(aPath, "r");
	xsElseError(aFile);
	aFlag = XS_PROGRAM_FLAG;
	if (xsToBoolean(xsGet(xsThis, xsID("program"))))
		aFlag |= XS_SANDBOX_FLAG;
	if (xsToBoolean(xsGet(xsThis, xsID("debug"))))
		aFlag |= XS_DEBUG_FLAG;
	if (xsToBoolean(xsArg(1))) {
		xsVar(0).value.string = (txString)fxParseTree(the, aFile, (txGetter)fgetc,
				fxNewSymbol(the, &xsArg(0)), 1, aFlag, &aSize);
		xsVar(0).kind = XS_STRING_KIND;
	}
	else {
		xsVar(0).value.code = fxParseScript(the, aFile, (txGetter)fgetc,
				fxNewSymbol(the, &xsArg(0)), 1, aFlag, &aSize);
		xsVar(0).kind = XS_CODE_KIND;
	}
	fclose(aFile);
	xsResult = xsNewInstanceOf(xsObjectPrototype);
	xsSet(xsResult, xsID("code"), xsVar(0));
	xsSet(xsResult, xsID("size"), xsInteger(aSize));
}

void xsc_getPath(txMachine* the)
{
	xsIntegerValue c, i;
	xsVars(1);

	xsVar(0) = xsGet(xsThis, xsID("inputPaths"));
	c = xsToInteger(xsGet(xsVar(0), the->lengthID));
	for (i = 0; i < c; i++) {
		xsResult = xsCall3(xsThis, xsID("makePath"), xsGetAt(xsVar(0), xsInteger(i)), xsArg(0), xsNull);
		xsResult = xsCall2(xsThis, xsID("resolvePath"), xsResult, xsFalse);
		if (xsTest(xsResult))
			return;
	}
}

void xsc_getPlatform(txMachine* the)
{
	#if mxWindows
		xsResult = xsString("Windows");
	#elif mxMacOSX
		xsResult = xsString("MacOSX");
	#elif mxLinux
		xsResult = xsString("Linux");
	#elif mxSolaris
		xsResult = xsString("Solaris");
	#endif
}

void xsc_getStringCode(txMachine* the)
{
	txStringStream aStream;
	txFlag aFlag;
	txSize aSize;
	xsVars(3);

	aStream.slot = &xsArg(0);
	aStream.offset = 0;
	aStream.size = strlen(xsToString(xsArg(0)));
	xsVar(0) = xsGet(xsArg(1), xsID("__xs__path"));
	xsVar(1) = xsGet(xsArg(1), xsID("__xs__line"));
	aFlag = XS_NO_FLAG;
	if (!xsToBoolean(xsArg(2)))
		aFlag = XS_PROGRAM_FLAG;
	if (xsToBoolean(xsGet(xsThis, xsID("debug"))))
		aFlag |= XS_DEBUG_FLAG;

	if (xsToBoolean(xsArg(3))) {
		xsVar(2).value.string = (txString)fxParseTree(the, &aStream, fxStringGetter,
				fxNewSymbol(the, &xsVar(0)), xsToInteger(xsVar(1)), aFlag, &aSize);
		xsVar(2).kind = XS_STRING_KIND;
	}
	else {
		xsVar(2).value.code = fxParseScript(the, &aStream, fxStringGetter,
				fxNewSymbol(the, &xsVar(0)), xsToInteger(xsVar(1)), aFlag, &aSize);
		xsVar(2).kind = XS_CODE_KIND;
	}
	xsResult = xsNewInstanceOf(xsObjectPrototype);
	xsSet(xsResult, xsID("code"), xsVar(2));
	xsSet(xsResult, xsID("size"), xsInteger(aSize));
}

void xsc_hasTarget(txMachine* the)
{
	xsTry {
		xsResult = xsCat3(xsString("with (xsc.targets) eval("), xsArg(0), xsString(");"));
		xsResult = xsCall1(xsGlobal, xsID("eval"), xsResult);
	}
	xsCatch {
		xsResult = xsFalse;
	}
}

void xsc_insertProperty(txMachine* the)
{
	xsStringValue aString;
	xsIntegerValue aSum;

	aString = xsToString(xsArg(0));
	aSum = 0;
	while(*aString != 0) {
		aSum = (aSum << 1) + *aString++;
	}
	aSum &= 0x7FFFFFFF;
	xsResult = gxProperties[aSum % mxPropertyModulo];
	if (xsTypeOf(xsResult) == xsReferenceType)
		(void)xsCall3(xsResult, xsID("insertProperty"), xsArg(0), xsArg(1), xsArg(2));
	else {
		gxProperties[aSum % mxPropertyModulo] = xsArg(1);
		xsSet(xsArg(1), xsID("qualifiedName"), xsArg(0));
	}
}

void xsc_load(txMachine* the)
{
	char* aPath;
	FILE* aFile = NULL;

	aPath = xsToString(xsArg(0));
	aFile = fopen(aPath, "r");
	xsElseError(aFile);
	xsResult = xsParse(aFile, (xsGetter)fgetc, aPath, 1, xsNoMixtureFlag | xsDebugFlag);
	fclose(aFile);
}

void xsc_loadPackage(txMachine* the)
{
	xsIntegerValue c, i;
	xsVars(2);
	xsVar(0) = xsGet(xsThis, xsID("packages"));
	c = xsToInteger(xsGet(xsVar(0), the->lengthID));
	for (i = 0; i < c; i++) {
		xsVar(1) = xsGet(xsGetAt(xsVar(0), xsInteger(i)), xsID("__xs__path"));
		if (xsToBoolean(xsCall2(xsThis, xsID("compareFiles"), xsVar(1), xsArg(0))))
			return;
	}
	xsResult = xsCall1(xsThis, xsID("load"), xsArg(0));
	if (xsTypeOf(xsResult) == xsUndefinedType)
		xsc_throw(the, "'%s': invalid file", xsToString(xsArg(0)));
	xsSetAt(xsVar(0), xsInteger(c), xsResult);
}

void xsc_makePath(txMachine* the)
{
	char aPath[1024];
	int aLength;
	char* aString;

	aPath[0] = 0;
	if (xsTypeOf(xsArg(0)) == xsStringType) {
		aString = xsToString(xsArg(0));
		aLength = strlen(aString);
		if (aLength > 0) {
			strcat(aPath, aString);
			if (aPath[aLength - 1] != mxSeparator) {
				aPath[aLength] = mxSeparator;
				aPath[aLength + 1] = 0;
			}
		}
	}
	if (xsTypeOf(xsArg(1)) == xsStringType) {
		aString = xsToString(xsArg(1));
		strcat(aPath, aString);
	}
	if (xsTypeOf(xsArg(2)) == xsStringType) {
		aString = xsToString(xsArg(2));
		aLength = strlen(aString);
		if (aLength > 0) {
			if (aString[0] != '.')
				strcat(aPath, ".");
			strcat(aPath, aString);
		}
	}
	xsResult = xsString(aPath);
}

void xsc_makePattern(txMachine* the)
{
	xsStringValue aString;
	xsIntegerValue c, i;
	xsVars(4);

	xsArg(1) = xsGet(xsArg(1), xsID("pattern"));
	xsVar(0) = xsNew0(xsGlobal, xsID("Array"));
	xsSet(xsVar(0), xsID("attributeFlag"), xsFalse);
	xsSet(xsVar(0), xsID("defaultFlag"), xsFalse);
	xsSet(xsVar(0), xsID("piFlag"), xsFalse);
	xsSet(xsVar(0), xsID("rootFlag"), xsFalse);
	xsSet(xsVar(0), xsID("skipFlag"), xsFalse);

	if (!xsTest(xsArg(1)))
		xsSet(xsVar(0), xsID("skipFlag"), xsTrue);
	else {
		aString = xsToString(xsArg(1));
		if (aString[0] == '/') {
			xsSet(xsVar(0), xsID("rootFlag"), xsTrue);
			xsArg(1) = xsCall1(xsArg(1), xsID("split"), xsString("/"));
			(void)xsCall0(xsArg(1), xsID("shift"));
		}
		else
			xsArg(1) = xsCall1(xsArg(1), xsID("split"), xsString("/"));
		c = xsToInteger(xsGet(xsArg(1), the->lengthID));
		for (i = 0; i < c; i++) {
			xsVar(1) = xsGetAt(xsArg(1), xsInteger(i));

			aString = xsToString(xsVar(1));
			if (!strcmp(aString, ".")) {
				if (i < (c - 1))
					return;
				xsSet(xsVar(0), xsID("defaultFlag"), xsTrue);
				xsVar(1) = xsString(".");
				xsVar(2) = xsNull;
			}
			else if (aString[0] == '@') {
				if (i < (c - 1))
					return;
				xsSet(xsVar(0), xsID("attributeFlag"), xsTrue);
				xsVar(1) = xsCall2(xsVar(1), xsID("substring"), xsInteger(1), xsInteger(0x7FFFFFFF));
				xsVar(2) = xsCall1(xsVar(1), xsID("split"), xsString(":"));
				if (2 == xsToInteger(xsGet(xsVar(2), the->lengthID))) {
					xsVar(1) = xsGet(xsVar(2), 1);
					xsVar(2) = xsGet(xsVar(2), 0);
					xsVar(2) = xsCall1(xsArg(0), xsID("getNamespaceUri"), xsVar(2));
					if (!xsTest(xsVar(2)))
						return;
				}
				else
					xsVar(2) = xsNull;
			}
			else if (aString[0] == '?') {
				if (i < (c - 1))
					return;
				xsSet(xsVar(0), xsID("piFlag"), xsTrue);
				xsVar(1) = xsCall2(xsVar(1), xsID("substring"), xsInteger(1), xsInteger(0x7FFFFFFF));
				xsVar(2) = xsCall1(xsVar(1), xsID("split"), xsString(":"));
				if (2 == xsToInteger(xsGet(xsVar(2), the->lengthID))) {
					xsVar(1) = xsGet(xsVar(2), 1);
					xsVar(2) = xsGet(xsVar(2), 0);
					xsVar(2) = xsCall1(xsArg(0), xsID("getNamespaceUri"), xsVar(2));
					if (!xsTest(xsVar(2)))
						return;
				}
				else
					xsVar(2) = xsCall1(xsArg(0), xsID("getNamespaceUri"), xsString(""));
			}
			else {
				xsVar(2) = xsCall1(xsVar(1), xsID("split"), xsString(":"));
				if (2 == xsToInteger(xsGet(xsVar(2), the->lengthID))) {
					xsVar(1) = xsGet(xsVar(2), 1);
					xsVar(2) = xsGet(xsVar(2), 0);
					xsVar(2) = xsCall1(xsArg(0), xsID("getNamespaceUri"), xsVar(2));
					if (!xsTest(xsVar(2)))
						return;
				}
				else
					xsVar(2) = xsCall1(xsArg(0), xsID("getNamespaceUri"), xsString(""));
			}
			xsVar(3) = xsNew0(xsGlobal, xsID("Object"));
			xsSet(xsVar(3), xsID("namespace"), xsVar(2));
			xsSet(xsVar(3), xsID("name"), xsVar(1));
			xsSetAt(xsVar(0), xsInteger(i), xsVar(3));
		}
	}
	xsResult = xsVar(0);
}

void xsc_print(txMachine* the)
{
	char *aString = NULL;

	aString = xsToString(xsArg(0));
	fprintf(stderr, "%s\n", aString);
}

void xsc_reportError(txMachine* the)
{
	char* aPath;
	long aLine;
	int aCount;

	if (xsTest(xsArg(0))) {
		aPath = xsToString(xsGet(xsArg(0), xsID("__xs__path")));
		aLine = xsToInteger(xsGet(xsArg(0), xsID("__xs__line")));
	#if mxWindows
		fprintf(stderr, "%s(%ld): error: ", aPath, aLine);
	#else
		fprintf(stderr, "%s:%ld: error: ", aPath, aLine);
	#endif
	}
	fprintf(stderr, "%s!\n", xsToString(xsArg(1)));
	aCount = xsToInteger(xsGet(xsThis, xsID("errorCount")));
	xsSet(xsThis, xsID("errorCount"), xsInteger(aCount + 1));
}

void xsc_reportWarning(txMachine* the)
{
	char* aPath;
	long aLine;
	int aCount;

	if (xsTest(xsArg(0))) {
		aPath = xsToString(xsGet(xsArg(0), xsID("__xs__path")));
		aLine = xsToInteger(xsGet(xsArg(0), xsID("__xs__line")));
	#if mxWindows
		fprintf(stderr, "%s(%ld): warning: ", aPath, aLine);
	#else
		fprintf(stderr, "%s:%ld: warning: ", aPath, aLine);
	#endif
	}
	fprintf(stderr, "%s!\n", xsToString(xsArg(1)));
	aCount = xsToInteger(xsGet(xsThis, xsID("warningCount")));
	xsSet(xsThis, xsID("warningCount"), xsInteger(aCount + 1));
}

void xsc_resolvePath(txMachine* the)
{
#if mxWindows
	char* srcPath;
	char aPath[1024];
	DWORD attributes;
	int aResult = 0;

	srcPath = xsToString(xsArg(0));
	if (_fullpath(aPath, srcPath, 1024) != NULL) {
		attributes = GetFileAttributes(aPath);
		if (attributes != 0xFFFFFFFF) {
			if (xsToInteger(xsArgc) == 1)
				aResult = 1;
			else if (xsToBoolean(xsArg(1)))
				aResult = (attributes & FILE_ATTRIBUTE_DIRECTORY) ? 1 : 0;
			else
				aResult = (attributes & FILE_ATTRIBUTE_DIRECTORY) ? 0 : 1;
		}
	}
#else
	char* srcPath;
	char aPath[PATH_MAX];
	struct stat a_stat;
	int aResult = 0;

	srcPath = xsToString(xsArg(0));
	if (realpath(srcPath, aPath) != NULL) {
		if (stat(aPath, &a_stat) == 0) {
			if (xsToInteger(xsArgc) == 1)
				aResult = 1;
			else if (xsToBoolean(xsArg(1)))
				aResult = S_ISDIR(a_stat.st_mode) ? 1 : 0;
			else
				aResult = S_ISREG(a_stat.st_mode) ? 1 : 0;
		}
	}
#endif
	if (aResult) {
		if (xsToBoolean(xsArg(1))) {
			aResult = strlen(aPath) - 1;
			if (aPath[aResult] == mxSeparator)
				aPath[aResult] = 0;
		}
		xsResult = xsString(aPath);
	}
}

void xsc_save(txMachine* the)
{
	xsBuffer* aBuffer = xsGetContext(the);
	xsIntegerValue c, i;
	FILE* aFile;
	xsStringValue aName;
	xsIntegerValue aSelector;
	xsIntegerValue aLength;
  	unsigned char* p;
	xsVars(2);

	xsVar(0) = xsGet(xsThis, xsID("hosts"));
	if (xsTest(xsVar(0))) {
		c = xsToInteger(xsGet(xsVar(0), the->lengthID));

		if (xsTest(xsGet(xsThis, xsID("verbose")))) {
			xsVar(1) = xsCat3(xsString("# Generating '"), xsArg(0), xsString(".h'..."));
			(void)xsCall1(xsThis, xsID("print"), xsVar(1));
		}
		xsVar(1) = xsCat2(xsArg(0), xsString(".h"));
		aFile = fopen(xsToString(xsVar(1)), "w");
		xsElseError(aFile);
		fprintf(aFile, "/* XSC GENERATED FILE; DO NOT EDIT! */\n\n");
		fprintf(aFile, "#include \"xs.h\"\n\n");
		aName = xsToString(xsArg(1));
		fprintf(aFile, "mxExport xsGrammar %sGrammar;\n\n", aName);
		i = 0;
		while (i < c) {
			xsVar(1) = xsGetAt(xsVar(0), xsInteger(i));
			aSelector = xsToInteger(xsGet(xsVar(1), xsID("selector")));
			aName = xsToString(xsGet(xsVar(1), xsID("name")));
			switch (aSelector) {
			case 0:
				fprintf(aFile, "extern void %s(xsMachine* the);\n", aName);
				break;
			case 1:
				fprintf(aFile, "extern void %s(xsMachine* the);\n", aName);
				break;
			case 2:
				fprintf(aFile, "extern void %s(void* theData);\n", aName);
				break;
			}
			i++;
		}
		if (xsTest(xsGet(xsThis, xsID("xsIDFlag")))) {
			fprintf(aFile, "\n");
			aName = aBuffer->symbols;
			aLength = *((unsigned char*)aName);
			aName++;
			aLength <<= 8;
			aLength += *((unsigned char*)aName);
			/*fprintf(aFile, "// %d \n", aLength);*/
			aName++;
			for (i = 0; i < aLength; i++) {
				char *p = aName;
				char s;
				while ((s = *aName++)) {
					if (!((('0' <= s) && (s <= '9'))
					||	(('A' <= s) && (s <= 'Z'))
					||	(('a' <= s) && (s <= 'z'))
					||	(s == '_')))
						break;
				}
				if (!s)
					fprintf(aFile, "#define xsID_%s (the->code[%d])\n", p, i);
				else
					while ((s = *aName++));
				#if mxUseApHash
				aName += symbolEncodeLength;
				#endif
			}

		}

		fclose(aFile);

		if (xsTest(xsGet(xsThis, xsID("verbose")))) {
			xsVar(1) = xsCat3(xsString("# Generating '"), xsArg(0), xsString(".c'..."));
			(void)xsCall1(xsThis, xsID("print"), xsVar(1));
		}
		xsVar(1) = xsCat2(xsArg(0), xsString(".c"));
		aFile = fopen(xsToString(xsVar(1)), "w");
		xsElseError(aFile);
		fprintf(aFile, "/* XSC GENERATED FILE; DO NOT EDIT! */\n\n");
		aName = xsToString(xsArg(1));
		fprintf(aFile, "#include \"%s.xs.h\"\n\n", aName);

		fprintf(aFile, "static void %sGrammarCallback(xsMachine* the)\n", aName);
		fprintf(aFile, "{\n");
		if (c) {
			fprintf(aFile, "\tstatic struct {\n");
			fprintf(aFile, "\t\tshort selector;\n");
			fprintf(aFile, "\t\tshort length;\n");
			fprintf(aFile, "\t\txsCallback callback;\n");
			fprintf(aFile, "\t} xsHosts[%d] = {\n", c);
			i = 0;
			while (i < c) {
				xsVar(1) = xsGetAt(xsVar(0), xsInteger(i));
				aSelector = xsToInteger(xsGet(xsVar(1), xsID("selector")));
				aName = xsToString(xsGet(xsVar(1), xsID("name")));
				aLength = xsToInteger(xsGet(xsVar(1), the->lengthID));
				fprintf(aFile, "\t\t{ %d, %d, (xsCallback)%s }", aSelector, aLength, aName);
				i++;
				if (i < c)
					fprintf(aFile, ",\n");
				else
					fprintf(aFile, "\n");
			}
			fprintf(aFile, "\t};\n");
			fprintf(aFile, "\tint i;\n");
			fprintf(aFile, "\txsResult = xsNewInstanceOf(xsArrayPrototype);\n");
			fprintf(aFile, "\txsNewHostProperty(xsGlobal, xsID(\"@\"), xsResult, xsDontScript, xsDontScript);\n");
			fprintf(aFile, "\tfor (i = 0; i < %d; i++)\n", c);
			fprintf(aFile, "\t\tswitch (xsHosts[i].selector) {\n");
			fprintf(aFile, "\t\tcase 0:\n");
			fprintf(aFile, "\t\t\txsNewHostProperty(xsResult, i, xsNewHostFunction(xsHosts[i].callback, xsHosts[i].length), xsDontScript, xsDontScript);\n");
			fprintf(aFile, "\t\t\tbreak;\n");
			fprintf(aFile, "\t\tcase 1:\n");
			fprintf(aFile, "\t\t\txsNewHostProperty(xsResult, i, xsNewHostConstructor(xsHosts[i].callback, xsHosts[i].length, xsObjectPrototype), xsDontScript, xsDontScript);\n");
			fprintf(aFile, "\t\t\tbreak;\n");
			fprintf(aFile, "\t\tcase 2:\n");
			fprintf(aFile, "\t\t\txsNewHostProperty(xsResult, i, xsNewHostObject((xsDestructor)xsHosts[i].callback), xsDontScript, xsDontScript);\n");
			fprintf(aFile, "\t\t\tbreak;\n");
			fprintf(aFile, "\t\t}\n");
		}
		fprintf(aFile, "}\n\n");

		i = 0;
		while (i < c) {
			xsVar(1) = xsGetAt(xsVar(0), xsInteger(i));
			xsResult = xsGet(xsVar(1), xsID("code"));
			if (xsTest(xsResult)) {
				aName = xsToString(xsGet(xsVar(1), xsID("name")));
				fprintf(aFile, "void %s(xsMachine* the)\n", aName);
				fprintf(aFile, "{\n");
				xsResult = xsGet(xsResult, xsID("code"));
				fprintf(aFile, "%s", xsResult.value.code);
				fprintf(aFile, "}\n\n");
			}
			i++;
		}

		if (xsTest(xsGet(xsThis, xsID("binary")))) {
			aName = xsToString(xsArg(1));
			fprintf(aFile, "xsGrammar %sGrammar = {\n", aName);
			fprintf(aFile, "\t%sGrammarCallback,\n", aName);
			fprintf(aFile, "\t(xsStringValue)0,\n");
			fprintf(aFile, "\t0,\n");
			fprintf(aFile, "\t(xsStringValue)0,\n");
			fprintf(aFile, "\t0,\n");
			fprintf(aFile, "\t\"%s\"\n", aName);
			fprintf(aFile, "};\n");
			fclose(aFile);
		}
		else {
			c = aBuffer->symbolsSize;
			fprintf(aFile, "#define xscGrammarSymbolsSize %d\n", c);
			fprintf(aFile, "static char xscGrammarSymbols[xscGrammarSymbolsSize] = {");
			if (c) {
				c--;
				p = (unsigned char*)aBuffer->symbols;
				for (i = 0; i < c; i++) {
					if ((i % 16) == 0)
						fprintf(aFile, "\n\t");
					fprintf(aFile, "0x%2.2X,", *p);
					p++;
				}
				fprintf(aFile, "0x%2.2X", *p);
			}
			fprintf(aFile, "\n};\n\n");

			c = aBuffer->current - aBuffer->bottom;
			fprintf(aFile, "#define xscGrammarCodeSize %d\n", c);
			fprintf(aFile, "static char xscGrammarCode[xscGrammarCodeSize] = {");
			if (c) {
				c--;
				p = (unsigned char*)aBuffer->bottom;
				for (i = 0; i < c; i++) {
					if ((i % 16) == 0)
						fprintf(aFile, "\n\t");
					fprintf(aFile, "0x%2.2X,", *p);
					p++;
				}
				fprintf(aFile, "0x%2.2X", *p);
			}
			fprintf(aFile, "\n};\n\n");

			aName = xsToString(xsArg(1));
			fprintf(aFile, "xsGrammar %sGrammar = {\n", aName);
			fprintf(aFile, "\t%sGrammarCallback,\n", aName);
			fprintf(aFile, "\txscGrammarSymbols,\n");
			fprintf(aFile, "\txscGrammarSymbolsSize,\n");
			fprintf(aFile, "\txscGrammarCode,\n");
			fprintf(aFile, "\txscGrammarCodeSize,\n");
			fprintf(aFile, "\t\"%s\"\n", aName);
			fprintf(aFile, "};\n");

			fclose(aFile);
		}
	}

	if (xsTest(xsGet(xsThis, xsID("binary")))) {
		if (xsTest(xsGet(xsThis, xsID("verbose")))) {
			xsVar(1) = xsCat3(xsString("# Generating '"), xsArg(0), xsString("b'..."));
			(void)xsCall1(xsThis, xsID("print"), xsVar(1));
		}
		xsVar(1) = xsCat2(xsArg(0), xsString("b"));
		aFile = fopen(xsToString(xsVar(1)), "wb");
		xsElseError(aFile);

		aLength = 8 + 8 + aBuffer->symbolsSize + 8 + (aBuffer->current - aBuffer->bottom);
		aLength = htonl(aLength);
		xsElseError(fwrite(&aLength, 4, 1, aFile) == 1);
		xsElseError(fwrite("XS11", 4, 1, aFile) == 1);

		c = aBuffer->symbolsSize;
		aLength = 8 + c;
		aLength = htonl(aLength);
		xsElseError(fwrite(&aLength, 4, 1, aFile) == 1);
		xsElseError(fwrite("SYMB", 4, 1, aFile) == 1);
		xsElseError(fwrite(aBuffer->symbols, c, 1, aFile) == 1);

		c = aBuffer->current - aBuffer->bottom;
		aLength = 8 + c;
		aLength = htonl(aLength);
		xsElseError(fwrite(&aLength, 4, 1, aFile) == 1);
		xsElseError(fwrite("CODE", 4, 1, aFile) == 1);
		xsElseError(fwrite(aBuffer->bottom, c, 1, aFile) == 1);

		fclose(aFile);
	}
}

void xsc_searchPath(txMachine* the)
{
	char aPath[1024];
	char* aName = xsToString(xsArg(0));
#if mxWindows
	char* aSlash = aName;
	while (*aSlash) {
		if (*aSlash == '/')
			*aSlash = '\\';
		aSlash++;
	}
#endif
	if (aName[0] == mxSeparator) {
		char* aHome = getenv("XS_HOME");
		if (!aHome)
			return;
		strcpy(aPath, aHome);
		strcat(aPath, aName);
		xsResult = xsCall2(xsThis, xsID("resolvePath"), xsString(aPath), xsArg(1));
	}
	else
		xsResult = xsCall2(xsThis, xsID("resolvePath"), xsArg(0), xsArg(1));
}

void xsc_searchProperty(txMachine* the)
{
	xsStringValue aString;
	xsIntegerValue aSum;

	aString = xsToString(xsArg(0));
	aSum = 0;
	while(*aString != 0) {
		aSum = (aSum << 1) + *aString++;
	}
	aSum &= 0x7FFFFFFF;
	xsResult = gxProperties[aSum % mxPropertyModulo];
	if (xsTypeOf(xsResult) == xsReferenceType)
		xsResult = xsCall1(xsResult, xsID("searchProperty"), xsArg(0));
}

void xsc_splitPath(txMachine* the)
{
	char *aPath;
	char *aSeparator = NULL;
	char *aDot = NULL;
	int aLength;
	char aDirectory[1024];
	char aName[1024];
	char anExtension[1024];

	aPath = xsToString(xsArg(0));
	aSeparator = strrchr(aPath, mxSeparator);
	if (aSeparator == NULL)
		aSeparator = aPath;
	else
		aSeparator++;
	aDot = strrchr(aSeparator, '.');
	if (aDot == NULL)
		aDot = aSeparator + strlen(aSeparator);
	aLength = aSeparator - aPath;
	strncpy(aDirectory, aPath, aLength);
	aDirectory[aLength - 1] = 0;
	aLength = aDot - aSeparator;
	strncpy(aName, aSeparator, aLength);
	aName[aLength] = 0;
	strcpy(anExtension, aDot);
	xsResult = xsNewInstanceOf(xsArrayPrototype);
	xsSet(xsResult, 0, xsString(aDirectory));
	xsSet(xsResult, 1, xsString(aName));
	xsSet(xsResult, 2, xsString(anExtension));
}

void xsc_trimString(txMachine* the)
{
	xsVars(1);
	xsVar(0) = xsNew1(xsGlobal, xsID("RegExp"), xsString("^(\\s*)([\\W\\w]*)(\\b\\s*$)"));
	if (xsTest(xsCall1(xsVar(0), xsID("test"), xsArg(0))))
		xsResult = xsCall2(xsArg(0), xsID("replace"), xsVar(0), xsString("$2"));
	else
		xsResult = xsArg(0);
}

void xsc_parseArguments(txMachine *the, int argc, char *argv[])
{
	int argi;

	for (argi = 1; argi < argc; argi++) {
		if  (!strcmp(argv[argi], "-a")) {
			argi++;
			if (argi == argc)
				xsc_throw(the, "-a: no arguments");
			xsSet(xsThis, xsID("arguments"), xsString(argv[argi]));
		}
		else if (!strcmp(argv[argi], "-b")) {
			xsSet(xsThis, xsID("binary"), xsTrue);
		}
		else if  (!strcmp(argv[argi], "-d")) {
			xsSet(xsThis, xsID("debug"), xsTrue);
		}
		else if  (!strcmp(argv[argi], "-e")) {
			xsSet(xsThis, xsID("tree"), xsTrue);
		}
		else if  (!strcmp(argv[argi], "-i")) {
			argi++;
			if (argi == argc)
				xsc_throw(the, "-i: no directory");
			xsResult = xsCall2(xsThis, xsID("resolvePath"), xsString(argv[argi]), xsTrue);
			if (xsTypeOf(xsResult) != xsStringType)
				xsc_throw(the, "-i '%s': directory not found", argv[argi]);
			xsVar(0) = xsGet(xsThis, xsID("inputPaths"));
			xsSetAt(xsVar(0), xsGet(xsVar(0), the->lengthID), xsResult);
		}
		else if  (!strcmp(argv[argi], "-m")) {
			if (xsTest(xsGet(xsThis, xsID("program"))))
				xsc_throw(the, "-m: already a program");
			xsSet(xsThis, xsID("module"), xsTrue);
		}
		else if  (!strcmp(argv[argi], "-o")) {
			argi++;
			if (argi == argc)
				xsc_throw(the, "-o: no directory");
			xsResult = xsCall2(xsThis, xsID("resolvePath"), xsString(argv[argi]), xsTrue);
			if (xsTypeOf(xsResult) != xsStringType)
				xsc_throw(the, "-o '%s': directory not found", argv[argi]);
			xsVar(0) = xsGet(xsThis, xsID("outputPath"));
			if (xsTypeOf(xsVar(0)) != xsNullType)
				xsc_throw(the, "-o '%s': too many -o directories", argv[argi]);
			xsSet(xsThis, xsID("outputPath"), xsResult);
		}
		else if  (!strcmp(argv[argi], "-p")) {
			if (xsTest(xsGet(xsThis, xsID("module"))))
				xsc_throw(the, "-p: already a module");
			xsSet(xsThis, xsID("program"), xsTrue);
		}
		else if  (!strcmp(argv[argi], "-t")) {
			argi++;
			if (argi == argc)
				xsc_throw(the, "-t: no name");
			xsVar(0) = xsGet(xsThis, xsID("targets"));
			xsSet(xsVar(0), xsID(argv[argi]), xsTrue);
		}
		else if  (!strcmp(argv[argi], "-v")) {
			xsSet(xsThis, xsID("verbose"), xsTrue);
		}
		else if  (!strcmp(argv[argi], "-xsID")) {
			xsSet(xsThis, xsID("xsIDFlag"), xsTrue);
		}
		else if  (argv[argi][0] == '@') {
			int argumentCount = 0, index;
			char **argumentList = NULL;

			xsc_parseCommandFile(the, ++argv[argi], &argumentCount, &argumentList);

			xsc_parseArguments(the, argumentCount, argumentList);

			if (argumentList) {
				for (index = 0; index < argumentCount; index++) {
					if (argumentList[index])
						c_free(argumentList[index]);
				}

				c_free(argumentList);
			}
		}
		else {
			xsResult = xsCall2(xsThis, xsID("resolvePath"), xsString(argv[argi]), xsFalse);
			if (xsTypeOf(xsResult) != xsStringType)
				xsc_throw(the, "'%s': file not found", argv[argi]);
			xsVar(0) = xsGet(xsThis, xsID("grammarPath"));
			if (xsTypeOf(xsVar(0)) != xsNullType)
				xsc_throw(the, "'%s': too many files", argv[argi]);
			xsSet(xsThis, xsID("grammarPath"), xsResult);
		}
	}
}

void xsc_parseCommandFile(txMachine *the, char *commandFilePath, int *argumentCountOut, char ***argumentListOut)
{
	FILE	*commandFile = NULL;
	long	commandFileLength, bufferLength, index, argumentCount = 0, argumentLength;
	char	*buffer = NULL, **argumentList = NULL, *current, *argument, *argumentCopy;

	*argumentCountOut = 0;
	*argumentListOut = NULL;

	commandFile = fopen(commandFilePath, "r");
	if (commandFile == NULL) goto bail;

	fseek(commandFile, 0, SEEK_END);
	commandFileLength = ftell(commandFile);
	rewind(commandFile);

	buffer = c_malloc(commandFileLength + 1);
	if (buffer == NULL) goto bail;

	bufferLength = fread(buffer, 1, commandFileLength, commandFile);
	if (bufferLength == 0) goto bail;

	buffer[bufferLength] = 0;

	for (index = 0; index < bufferLength; index++, argumentCount++)
		index += xsc_getNextArgumentLength(buffer + index);

	argumentList = c_malloc((argumentCount + 1) * sizeof(char *));
	if (argumentList == NULL) goto bail;

	argumentList[0] = NULL;
	current = buffer;

	for (index = 0; index < argumentCount; index++, current += argumentLength + 1)
	{
		argumentLength = xsc_getNextArgumentLength(current);

		argumentCopy = c_malloc(argumentLength + 1);
		if (argumentCopy == NULL) goto bail;

		strncpy(argumentCopy, current, argumentLength);
		argumentCopy[argumentLength] = 0;

		argumentList[index + 1] = argumentCopy;
	}

	*argumentCountOut = argumentCount + 1;
	*argumentListOut = argumentList;
	argumentList = NULL;

bail:
	if (buffer)
		c_free(buffer);

	if (commandFile)
		fclose(commandFile);

	if (argumentList) {
		for (index = 0; index < argumentCount + 1; index++) {
			if (argumentList[index])
				c_free(argumentList[index]);
		}

		c_free(argumentList);
	}
}

long xsc_getNextArgumentLength(char *arguments)
{
	char	*current;
	long	argumentLength = 0;

	argumentLength = strcspn(arguments, "\" ");							/* Look for the next quote, space, or end-of-string */

	while (arguments[argumentLength] == '"')							/* When a quote is found ... */
	{
		argumentLength++;												/* ... advance to the next character after the quote, and ... */
		argumentLength += strcspn(arguments + argumentLength, "\"");	/* ... find the mating quote (or end-of-string). */
		if (!arguments[argumentLength])									/* If end-of-string, ... */
			break;														/* ... abort; */
		argumentLength++;												/* otherwise, skip over this quote, ... */
		argumentLength += strcspn(arguments + argumentLength, "\" ");	/* ... and search for another quote (or end-of-string) */
	}

	return argumentLength;
}

void xsc_main(txMachine* the, int argc, char* argv[])
{
	xsBooleanValue isTree;
	xsVars(5);

	xsc_parseArguments(the, argc, argv);
	isTree = xsTest(xsGet(xsThis, xsID("tree")));
	
	xsVar(0) = xsGet(xsThis, xsID("grammarPath"));
	if (xsTypeOf(xsVar(0)) != xsStringType)
		xsc_throw(the, "no file");
	xsVar(1) = xsCall1(xsThis, xsID("splitPath"), xsVar(0));
	xsVar(2) = xsGet(xsVar(1), 0);
	xsVar(3) = xsGet(xsVar(1), 1);
	xsVar(4) = xsGet(xsVar(1), 2);

	xsVar(0) = xsGet(xsThis, xsID("inputPaths"));
	xsSet(xsVar(0), 0, xsVar(2));

	xsVar(0) = xsGet(xsThis, xsID("outputPath"));
	if (xsTypeOf(xsVar(0)) == xsNullType)
		xsSet(xsThis, xsID("outputPath"), xsVar(2));

	xsVar(0) = xsGet(xsThis, xsID("grammarPath"));
	if (xsTest(xsGet(xsThis, xsID("verbose")))) {
		xsVar(1) = xsCat3(xsString("# Parsing '"), xsVar(0), xsString("'..."));
		(void)xsCall1(xsThis, xsID("print"), xsVar(1));
	}

	if (!isTree)
		xscCreateBuffer(the);

	if (!c_strcmp(xsToString(xsVar(4)), ".js")) {
		xsBooleanValue isModule = xsTest(xsGet(xsThis, xsID("module")));
		xsBooleanValue isProgram = xsTest(xsGet(xsThis, xsID("program")));
		if (!isModule && !isProgram) {
			xsStringValue aPath = xsToString(xsVar(0));
			FILE* aFile = fopen(aPath, "r");
			char aBuffer[16];
			xsIntegerValue aLength;
			xsElseError(aFile);
			aLength = fread(aBuffer, 1, sizeof(aBuffer) - 1, aFile);
			fclose(aFile);
			aBuffer[aLength] = 0;
			if (!c_strncmp(aBuffer, "//@module", 9)) {
				isModule = 1;
				xsSet(xsThis, xsID("module"), xsTrue);
			}
			else if (!c_strncmp(aBuffer, "//@program", 10)) {
				isProgram = 1;
				xsSet(xsThis, xsID("program"), xsTrue);
			}
		}
		if (isModule) {
			if (isTree) {
				xsResult = xsCall2(xsThis, xsID("getModuleCode"), xsVar(0), xsTrue);
			}
			else {
				xscAppendID(the, XS_BEGIN, -1);
				xscAppend1(the, XS_PROGRAM_FLAG);
				xscAppend1(the, 0);
				xscAppend1(the, 0);
				xscAppend1(the, XS_ROUTE);
				xscAppend2(the, 3);
				xscAppend1(the, XS_BRANCH);
				xscAppend2(the, 1);
				xscAppend1(the, XS_END);
				xsResult = xsCall2(xsThis, xsID("getModuleCode"), xsVar(0), xsFalse);
				xscAppendFunction(the);
				xscAppend1(the, XS_RESULT);
				xscAppend1(the, XS_POP);
				xscAppend1(the, XS_END);
			}
		}
		else if (isProgram) {
			if (isTree) {
				xsResult = xsCall2(xsThis, xsID("getProgramCode"), xsVar(0), xsTrue);
			}
			else {
				txInteger aLength;
				xsResult = xsCall2(xsThis, xsID("getProgramCode"), xsVar(0), xsFalse);
				aLength = xsToInteger(xsGet(xsResult, xsID("size")));
				xsResult = xsGet(xsResult, xsID("code"));
				xscAppend(the, xsResult.value.code, aLength);
			}
		}
		else {
			xsc_throw(the, "no module, no program");
		}
	}
	else {
		xsVar(1) = xsGet(xsGlobal, xsID("xsObjectPrototype"));
		(void)xsCall0(xsVar(1), xsID("initialize"));
		xsVar(1) = xsGet(xsGlobal, xsID("xsFunctionPrototype"));
		(void)xsCall0(xsVar(1), xsID("initialize"));
		xsVar(1) = xsGet(xsGlobal, xsID("xsArrayPrototype"));
		(void)xsCall0(xsVar(1), xsID("initialize"));
		xsVar(1) = xsGet(xsGlobal, xsID("xsStringPrototype"));
		(void)xsCall0(xsVar(1), xsID("initialize"));
		xsVar(1) = xsGet(xsGlobal, xsID("xsBooleanPrototype"));
		(void)xsCall0(xsVar(1), xsID("initialize"));
		xsVar(1) = xsGet(xsGlobal, xsID("xsNumberPrototype"));
		(void)xsCall0(xsVar(1), xsID("initialize"));
		xsVar(1) = xsGet(xsGlobal, xsID("xsDatePrototype"));
		(void)xsCall0(xsVar(1), xsID("initialize"));
		xsVar(1) = xsGet(xsGlobal, xsID("xsRegExpPrototype"));
		(void)xsCall0(xsVar(1), xsID("initialize"));
		xsVar(1) = xsGet(xsGlobal, xsID("xsErrorPrototype"));
		(void)xsCall0(xsVar(1), xsID("initialize"));
		xsVar(1) = xsGet(xsGlobal, xsID("xsChunkPrototype"));
		(void)xsCall0(xsVar(1), xsID("initialize"));
		xsVar(1) = xsGet(xsGlobal, xsID("xsDocumentPrototype"));
		(void)xsCall0(xsVar(1), xsID("initialize"));
		xsVar(1) = xsGet(xsGlobal, xsID("xsInfoSetPrototype"));
		(void)xsCall0(xsVar(1), xsID("initialize"));
		xsVar(1) = xsGet(xsGlobal, xsID("xsElementPrototype"));
		(void)xsCall0(xsVar(1), xsID("initialize"));
		xsVar(1) = xsGet(xsGlobal, xsID("xsAttributePrototype"));
		(void)xsCall0(xsVar(1), xsID("initialize"));
		xsVar(1) = xsGet(xsGlobal, xsID("xsCDataPrototype"));
		(void)xsCall0(xsVar(1), xsID("initialize"));
		xsVar(1) = xsGet(xsGlobal, xsID("xsPIPrototype"));
		(void)xsCall0(xsVar(1), xsID("initialize"));
		xsVar(1) = xsGet(xsGlobal, xsID("xsCommentPrototype"));
		(void)xsCall0(xsVar(1), xsID("initialize"));

		xsVar(1) = xsCall1(xsThis, xsID("loadPackage"), xsVar(0));

		(void)xsCall3(xsVar(1), xsID("target"), xsNull, xsTrue, xsString(""));
		xsVar(0) = xsGet(xsThis, xsID("errorCount"));
		if (xsTest(xsVar(0)))
			xsc_throw(the, "%ld: error(s)", xsToInteger(xsVar(0)));

		(void)xsCall3(xsVar(1), xsID("crossReference"), xsNull, xsString(""), xsFalse);
		xsVar(0) = xsGet(xsThis, xsID("errorCount"));
		if (xsTest(xsVar(0)))
			xsc_throw(the, "%ld: error(s)", xsToInteger(xsVar(0)));

		xsVar(0) = xsNew0(xsGlobal, xsID("Array"));
		xsSet(xsThis, xsID("hosts"), xsVar(0));

		xscAppendID(the, XS_BEGIN, -1);
		xscAppend1(the, XS_PROGRAM_FLAG);
		xscAppend1(the, 0);
		xscAppend1(the, 0);
		xscAppend1(the, XS_ROUTE);
		xscAppend2(the, 3);
		xscAppend1(the, XS_BRANCH);
		xscAppend2(the, 1);
		xscAppend1(the, XS_END);
		xscAppend1(the, XS_THIS);
		(void)xsCall0(xsVar(1), xsID("codePrototype"));
		xscAppend1(the, XS_POP);
		(void)xsCall0(xsVar(1), xsID("codeGrammar"));
		xscAppendInteger(the, 0);
		xscAppend1(the, XS_THIS);
		xscAppendID(the, XS_GET_MEMBER_FOR_CALL, xsID("__xs__link"));
		xscAppend1(the, XS_CALL);
		xscAppend1(the, XS_POP);
		if (xsTest(xsGet(xsVar(0), the->lengthID))) {
			xscAppend1(the, XS_THIS);
			xscAppendID(the, XS_DELETE_MEMBER, xsID("@"));
			xscAppend1(the, XS_POP);
		}
		xscAppend1(the, XS_END);
	}
	if (isTree) {
		FILE* aFile;
		xsDebugger();
		xsVar(0) = xsGet(xsThis, xsID("outputPath"));
		xsVar(0) = xsCall3(xsThis, xsID("makePath"), xsVar(0), xsVar(3), xsVar(4));
		xsVar(1) = xsCat2(xsVar(0), xsString(".tree"));
		xsVar(2) = xsGet(xsResult, xsID("code"));
		xsVar(3) = xsGet(xsResult, xsID("size"));
		aFile = fopen(xsToString(xsVar(1)), "w");
		xsElseError(aFile);
		xsElseError(fwrite(xsToString(xsVar(2)), xsToInteger(xsVar(3)), 1, aFile) == 1);
		fclose(aFile);
	}
	else {
		xscAppend1(the, XS_LABEL);

		xsVar(0) = xsGet(xsThis, xsID("outputPath"));
		xsVar(0) = xsCall3(xsThis, xsID("makePath"), xsVar(0), xsVar(3), xsVar(4));

		xscBuildSymbols(the);

		(void)xsCall2(xsThis, xsID("save"), xsVar(0), xsVar(3));

		xscDeleteBuffer(the);
	}
}

void xsc_throw(txMachine* the, char* theFormat, ...)
{
	c_va_list arguments;
	static char aBuffer[4096];

	c_va_start(arguments, theFormat);
	vsprintf(aBuffer, theFormat, arguments);
	c_va_end(arguments);
	xsThrow(xsNew1(xsGlobal, xsID("Error"), xsString(aBuffer)));
}

int main(int argc, char* argv[])
{
	txAllocation anAllocation = {
		2048 * 1024, /* initialChunkSize */
		512 * 1024, /* incrementalChunkSize */
		1000 * 1000, /* initialHeapCount */
		50 * 1000, /* incrementalHeapCount */
		2048, /* stackCount */
		0x7FFF, /* symbolCount */
		1993 /* symbolModulo */
	};
	txMachine* aMachine;
	int result = 1;

	gxProperties = c_calloc(mxPropertyModulo, sizeof(xsSlot));

	aMachine = xsNewMachine(&anAllocation, NULL, NULL);
	if (gxProperties && aMachine) {

		xsBeginHost(aMachine);

		{
			xsTry {
				fxBuild_xsc(aMachine);
				fxBuild_xscPackage(aMachine);
				xsThis = xsGet(xsGlobal, xsID("xsc"));
				xsResult = xsCall1(xsThis, xsID("resolvePath"), xsString(argv[0]));
				xsResult = xsCall1(xsThis, xsID("splitPath"), xsResult);
				xsResult = xsGetAt(xsResult, xsInteger(0));
				xsResult = xsCall3(xsThis, xsID("makePath"), xsResult, xsString("xsc"), xsString("js"));
				fxBuildTree(aMachine, xsToString(xsResult));
				xsc_main(the, argc, argv);
				result = 0;
			}
			xsCatch {
				char* aMessage = xsToString(xsGet(xsException, xsID("message")));
				fprintf(stderr, "### %s\n", aMessage);
			}
		}
		xsEndHost(aMachine);

		xsDeleteMachine(aMachine);
		c_free(gxProperties);
	}
	else
		fprintf(stderr, "### Cannot allocate machine!\n");

	return result;
}

