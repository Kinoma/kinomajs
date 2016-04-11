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
#include "mc_stdio.h"
#include "mc_module.h"
#if mxMC
#define FILE	MC_FILE
#endif

#define CONSOLE_LOG_FILE	0x01
#define CONSOLE_XSBUG		0x02

static int console_xsbug = 0;
static int console_log_depth = 0;

#define CONSOLE_OUT(...)	do {if (!console_xsbug || !xsVTrace(the, __VA_ARGS__)) fprintf(console_out, __VA_ARGS__);} while (0)

static void indent(xsMachine* the, FILE *console_out, int n)
{
	int indent = console_log_depth + n;

	while (--indent >= 0)
		CONSOLE_OUT("  ");
}

void
xs_console_log(xsMachine* the)
{
	xsIntegerValue c = xsToInteger(xsArgc), i;
	xsBooleanValue space = 0;
	xsBooleanValue comma = 0;
	xsBooleanValue nonl = 0;
	int options = 0;
	FILE *console_out = stdout;

	xsVars(4);
	console_log_depth++;
	for (i = 0; i < c; i++) {
		if (space && !nonl)
			CONSOLE_OUT("\n");
		else
			space = 1;
		switch (xsTypeOf(xsArg(i))) {
		case xsUndefinedType:
		case xsNullType:
		case xsBooleanType:
		case xsIntegerType:
		case xsNumberType:
			CONSOLE_OUT("%s", xsToString(xsArg(i)));
			break;
		case xsSymbolType:
			xsResult = xsCall1(xsGlobal, xsID("String"), xsArg(i));
			CONSOLE_OUT("%s", xsToString(xsResult));
			break;
		case xsStringType:
		case xsStringXType:
			if (i == 0 || options) {
				const char *opt = xsToString(xsArg(i));
				options = 0;
				if (strcmp(opt, "-n") == 0) {
					nonl++;
					options++;
				}
				else if (strcmp(opt, "-stderr") == 0) {
					console_out = stderr;
					options++;
				}
				else if (strcmp(opt, "-stdout") == 0) {
					console_out = stdout;
					options++;
				}
				else if (strncmp(opt, "-", 1) == 0)	/* end of the option args */
					break;
				if (options) {
					space = 0;
					break;
				}
			}
			if (console_log_depth == 1)
				CONSOLE_OUT("%s", xsToString(xsArg(i)));
			else
				CONSOLE_OUT("\"%s\"", xsToString(xsArg(i)));
			break;
		case xsReferenceType:
			if (xsIsInstanceOf(xsArg(i), xsFunctionPrototype)) {
				CONSOLE_OUT("function(...) { ... }");
			}
			else if (xsIsInstanceOf(xsArg(i), xsArrayPrototype)) {
				xsGet(xsVar(0), xsArg(i), xsID("length"));
				xsIntegerValue length = xsToInteger(xsVar(0)), index;
				CONSOLE_OUT("[\n");
				for (index = 0; index < length; index++) {
					xsGet(xsVar(1), xsArg(i), (xsIndex)index);
					if (comma)
						CONSOLE_OUT(",\n");
					else
						comma = 1;
					indent(the, console_out, 0);
					fxPush(xsVar(1));
					fxPushCount(the, 1);
					fxPush(xsThis);
					fxPush(xsFunction);
					fxCall(the);
				}
				CONSOLE_OUT("\n"); indent(the, console_out, -1); CONSOLE_OUT("]");
			}
			else {
				CONSOLE_OUT( "{\n");
				xsVar(0) = xsEnumerate(xsArg(i));
				for (;;) {
					xsVar(1) = xsCall0(xsVar(0), xsID("next"));
					xsGet(xsVar(2), xsVar(1), xsID("done"));
					if (xsTest(xsVar(2)))
						break;
					xsGet(xsVar(2), xsVar(1), xsID("value"));
					xsGetAt(xsVar(3), xsArg(i), xsVar(2));
					if (comma)
						CONSOLE_OUT( ",\n");
					else
						comma = 1;
					indent(the, console_out, 0); CONSOLE_OUT("%s: ", xsToString(xsVar(2)));
					fxPush(xsVar(3));
					fxPushCount(the, 1);
					fxPush(xsThis);
					fxPush(xsFunction);
					fxCall(the);
				}
				CONSOLE_OUT("\n"); indent(the, console_out, -1); CONSOLE_OUT("}");
			}
			break;
		}
	}
	console_log_depth--;
	if (!console_log_depth && !nonl)
		CONSOLE_OUT("\n");
}

void
xs_console_setEnable(xsMachine *the)
{
	unsigned int flags = xsToInteger(xsArg(0));

	mc_log_set_enable(flags & CONSOLE_LOG_FILE);
	console_xsbug = flags & CONSOLE_XSBUG;
}

void
xs_console_getEnable(xsMachine *the)
{
	unsigned int flags = 0;

	if (console_xsbug)
		flags |= CONSOLE_XSBUG;
	if (mc_log_get_enable())
		flags |= CONSOLE_LOG_FILE;
	xsSetInteger(xsResult, flags);
}

void
xs_console_load(xsMachine *the)
{
	xsVars(1);
	xsSetInteger(xsVar(0), CONSOLE_LOG_FILE);
	xsSet(xsThis, xsID("LOGFILE"), xsVar(0));
	xsSetInteger(xsVar(0), CONSOLE_XSBUG);
	xsSet(xsThis, xsID("XSBUG"), xsVar(0));
}
