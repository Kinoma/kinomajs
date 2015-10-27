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
#include "xs6All.h"
#include "xs.h"

extern void* fxMapArchive(txString path, txCallbackAt callbackAt);
extern void fxRunLoop(txMachine* the);
extern void fxRunModule(txMachine* the, txString path);
extern void fxRunProgram(txMachine* the, txString path);
extern void fxUnmapArchive(void* it);

#if mxWindows
	#include <direct.h>
	#include <process.h>
	#define mxSeparator '\\'
#else
	#include <unistd.h>
	#define mxSeparator '/'
#endif

static void console_log(xsMachine* the);
static void print(xsMachine* the);
static void process_cwd(xsMachine* the);
static void process_execArgv(xsMachine* the);
static void process_getenv(xsMachine* the);

static int gargc;
static int gargi;
static char** gargv;
static char** process_then_parameters = NULL;

static int console_log_depth = 0;
void console_log(xsMachine* the)
{
	xsIntegerValue c = xsToInteger(xsArgc), i;
	xsBooleanValue space = 0;
	xsVars(4);
	console_log_depth++;
	for (i = 0; i < c; i++) {
		if (space)
			fprintf(stderr,  " ");
		else
			space = 1;
		switch (xsTypeOf(xsArg(i))) {
		case xsUndefinedType:
		case xsNullType:
		case xsBooleanType:
		case xsIntegerType:
		case xsNumberType:
			fprintf(stderr,  "%s", xsToString(xsArg(i)));
			break;
		case xsSymbolType:
			xsResult = xsCall1(xsGlobal, xsID("String"), xsArg(i));
			fprintf(stderr,  "%s", xsToString(xsResult));
			break;
		case xsStringType:
		case xsStringXType:
			if ((console_log_depth == 1) && (i == 0))
				fprintf(stderr,  "%s", xsToString(xsArg(i)));
			else
				fprintf(stderr,  "'%s'", xsToString(xsArg(i)));
			break;
		case xsReferenceType:
			if (console_log_depth < 3) {
				xsBooleanValue comma = 0;
				if (xsHas(xsArg(i), xsID("length"))) {
					xsIntegerValue length = xsToInteger(xsGet(xsArg(i), xsID("length"))), index;
					fprintf(stderr,  "[");
					for (index = 0; index < length; index++) {
						xsVar(1) = xsGet(xsArg(i), (xsIndex)index);
						if (comma)
							fprintf(stderr,  ",");
						else
							comma = 1;
						fprintf(stderr,  " ");
						fxPush(xsVar(1));
						fxPushCount(the, 1);
						fxPush(xsThis);
						fxPush(xsFunction);
						fxCall(the);
					}
					fprintf(stderr,  " ]");
				}
				else {
					fprintf(stderr,  "{");
					xsVar(0) = xsEnumerate(xsArg(i));
					for (;;) {
						xsVar(1) = xsCall0(xsVar(0), xsID("next"));
						if (xsTest(xsGet(xsVar(1), xsID("done"))))
							break;
						xsVar(2) = xsGet(xsVar(1), xsID("value"));
						xsVar(3) = xsGetAt(xsArg(i), xsVar(2));
						if (comma)
							fprintf(stderr,  ",");
						else
							comma = 1;
						fprintf(stderr,  " %s: ", xsToString(xsVar(2)));
						fxPush(xsVar(3));
						fxPushCount(the, 1);
						fxPush(xsThis);
						fxPush(xsFunction);
						fxCall(the);
					}
					fprintf(stderr,  " }");
				}
			}
			else
				fprintf(stderr,  "%s", xsToString(xsArg(i)));
			break;
		}
	}
	console_log_depth--;
	if (!console_log_depth)
		fprintf(stderr,  "\n");
}

void print(xsMachine* the)
{
	fprintf(stdout, "%s\n", xsToString(xsArg(0)));
}

void process_cwd(xsMachine* the)
{
	char path[PATH_MAX];
	realpath(".", path);
	xsResult = xsString(path);
}

void process_execArgv(xsMachine* the)
{
	int i;
	xsResult = xsNewInstanceOf(xsArrayPrototype);
	for (i = gargi; i < gargc; i++) {
		xsSetAt(xsResult, xsInteger(i - gargi), xsString(gargv[i]));
	}
}

void process_getenv(xsMachine* the)
{
	xsStringValue result = getenv(xsToString(xsArg(0)));
	if (result)
		xsResult = xsString(result);
}

void process_then(xsMachine* the)
{
	xsIntegerValue c = xsToInteger(xsArgc), i;
	process_then_parameters = malloc(sizeof(char *)*(c + 1));
	for (i = 0; i < c; i++) {
		xsStringValue string = xsToString(xsArg(i));
		xsIntegerValue length = strlen(string) + 1;
		process_then_parameters[i] = malloc(length);
		c_memcpy(process_then_parameters[i], string, length);
	}
	process_then_parameters[c] = NULL;
}

void weakTest(void* data)
{
	if (data) {
		fprintf(stdout, "FREE %s\n", data);
		free(data);
	}
}

void WeakTest(xsMachine* the)
{
	xsStringValue string = xsToString(xsArg(0));
	xsIntegerValue length = strlen(string);
	xsStringValue data = malloc(length + 1);
	xsElseError(data);
	memcpy(data, string, length);
	data[length] = 0;
	xsSetHostData(xsThis, data);
}

void gc(xsMachine* the)
{
	xsCollectGarbage();
}

int main(int argc, char* argv[]) 
{
	xsCreation creation_mc = {
		25*1024,	/* initial chunk size */
		2048,		/* incremental chunk size */
		50*1024/16,	/* initial heap count	-- will be calculated later */
		128,		/* incremental heap count	-- wasting 16 bytes / allocation */
		548,		/* stack count */
		2048+512,	/* key count */
		97,		/* name modulo */
		127,		/* symbol modulo */
	};
	xsCreation creation_tool = {
		128 * 1024 * 1024, 	/* initialChunkSize */
		16 * 1024 * 1024, 	/* incrementalChunkSize */
		8 * 1024 * 1024, 		/* initialHeapCount */
		1 * 1024 * 1024, 		/* incrementalHeapCount */
		4096, 		/* stackCount */
		4096*3, 		/* keyCount */
		1993, 		/* nameModulo */
		127 		/* symbolModulo */
	};
	xsCreation* creation = &creation_tool;
	int error = 0;
	int argi = 1;
	void* archive = NULL;
	xsBooleanValue program = 0;
	xsMachine* machine;
	char path[PATH_MAX];
	char modulePath[PATH_MAX];
	xsStringValue extension;
	xsStringValue slash;
	xsStringValue name;
	int size;
	
	if (argi < argc) {
		if (!strcmp(argv[argi], "-a")) {
			argi++;
			if (argi < argc) {
                if (realpath(argv[argi], path)) {
                	archive = fxMapArchive(path, NULL);
                    if (!archive) {
                        fprintf(stderr, "# invalid archive: %s\n", path);
                        return 1;
                    }
                    if (strstr(path, "mc.xsa")) {
                  		creation = &creation_mc;
                    }
                }
				else {
					fprintf(stderr, "# archive not found: %s\n", argv[argi]);
					return 1;
				}
				argi++;
			}
		}
	}
	if (argi < argc) {
		if (!strcmp(argv[argi], "-p")) {
			program = 1;
			argi++;
		}
	}
	gargc = argc;
	gargi = argi;
	gargv = argv;
	machine = xsCreateMachine(creation, archive, "xsr6", NULL);
	xsBeginHost(machine);
	{
		xsVars(2);
		//xsStartProfiling();
		{
			xsTry {
				xsVar(0) = xsNewInstanceOf(xsObjectPrototype);
				xsVar(1) = xsNewHostFunction(console_log, 0);
				xsSet(xsVar(0), xsID("log"), xsVar(1));
				xsSet(xsGlobal, xsID("console"), xsVar(0));

				xsVar(0) = xsNewHostObject(weakTest);
				xsVar(1) = xsNewHostConstructor(WeakTest, 1, xsVar(0));
				xsSet(xsGlobal, xsID("WeakTest"), xsVar(1));
				xsVar(1) = xsNewHostFunction(gc, 0);
				xsSet(xsGlobal, xsID("gc"), xsVar(1));

				xsResult = xsModulePaths();
				realpath(argv[0], modulePath);
				slash = strrchr(modulePath, mxSeparator);
				if (slash) {
					strcpy(slash + 1, "modules");
					size = c_strlen(modulePath);
					modulePath[size++] = mxSeparator;
					modulePath[size] = 0;
					xsCall1(xsResult, xsID("add"), xsString(modulePath));
				}

				if (argi == argc) {
					fprintf(stderr, "# no module, no program\n");
					error = 1;
				}	
				else if (program) {
					xsVar(0) = xsNewHostFunction(print, 0);
					xsSet(xsGlobal, xsID("print"), xsVar(0));
					while (argi < argc) {
						if (argv[argi][0] != '-') {
							xsElseError(realpath(argv[argi], path));
							strcpy(modulePath, path);
							slash = strrchr(modulePath, mxSeparator);
							*(slash + 1) = 0;
							xsCall1(xsResult, xsID("add"), xsString(modulePath));
							strcat(modulePath, "modules/");
							xsCall1(xsResult, xsID("add"), xsString(modulePath));
							fxRunProgram(the, path);
						}
						argi++;
					}
					fxRunLoop(the);
				}
				else {
					xsVar(0) = xsNewInstanceOf(xsObjectPrototype);
					xsVar(1) = xsNewHostFunction(process_cwd, 0);
					xsSet(xsVar(0), xsID("cwd"), xsVar(1));
				#ifdef mxDebug
					xsSet(xsVar(0), xsID("debug"), xsTrue);
				#else
					xsSet(xsVar(0), xsID("debug"), xsFalse);
				#endif
					xsVar(1) = xsNewHostFunction(process_execArgv, 0);
					xsSet(xsVar(0), xsID("execArgv"), xsVar(1));
					xsVar(1) = xsNewHostFunction(process_getenv, 0);
					xsSet(xsVar(0), xsID("getenv"), xsVar(1));
					xsVar(1) = xsNewHostFunction(process_then, 1);
					xsSet(xsVar(0), xsID("then"), xsVar(1));
					xsSet(xsVar(0), xsID("platform"), xsString("darwin"));
					xsSet(xsGlobal, xsID("process"), xsVar(0));
		
					strcpy(path, argv[argi]);
					slash = strrchr(path, mxSeparator);
					if (slash) {
						*slash = 0;
						realpath(path, modulePath);
						size = c_strlen(modulePath);
						modulePath[size++] = mxSeparator;
						modulePath[size] = 0;
						xsCall1(xsResult, xsID("add"), xsString(modulePath));
						strcat(modulePath, "modules");
						size = c_strlen(modulePath);
						modulePath[size++] = mxSeparator;
						modulePath[size] = 0;
						xsCall1(xsResult, xsID("add"), xsString(modulePath));
						name = slash + 1;
					
					}
					else {
						realpath(".", modulePath);
						size = c_strlen(modulePath);
						modulePath[size++] = mxSeparator;
						modulePath[size] = 0;
						xsCall1(xsResult, xsID("add"), xsString(modulePath));
						strcat(modulePath, "modules");
						size = c_strlen(modulePath);
						modulePath[size++] = mxSeparator;
						modulePath[size] = 0;
						xsCall1(xsResult, xsID("add"), xsString(modulePath));
						name = path;
					}
					extension = strrchr(name, '.');
					if (extension)
						*extension = 0;
					fxRunModule(the, name);
				}
			}
			xsCatch {
				xsStringValue message = xsToString(xsException);
				fprintf(stderr, "### %s\n", message);
				error = 1;
			}
		}
		//xsStopProfiling();
	}
	xsEndHost(the);
	xsDeleteMachine(machine);
	fxUnmapArchive(archive);
	if (!error && process_then_parameters) {
	#if mxWindows
		if (_spawnvp(_P_WAIT, process_then_parameters[0], process_then_parameters) < 0)
			fprintf(stderr, "### Cannot execute %s!\n", process_then_parameters[0]);
	#else
		execvp(process_then_parameters[0], process_then_parameters);
	#endif
	}
	return error;
}
