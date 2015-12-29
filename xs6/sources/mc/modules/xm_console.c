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
#include "mc_stdio.h"
#include "mc_module.h"

static int console_log_depth = 0;

static void indent(int n)
{
	int indent = console_log_depth + n;

	while (--indent >= 0)
		fprintf(stderr,  "  ");
}

void console_log(xsMachine* the)
{
	xsIntegerValue c = xsToInteger(xsArgc), i;
	xsBooleanValue space = 0;
	xsBooleanValue comma = 0;
	xsBooleanValue nonl = 0;

	xsVars(4);
	console_log_depth++;
	for (i = 0; i < c; i++) {
		if (space && !nonl)
			fprintf(stderr,  "\n");
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
			if (i == 0 && strcmp(xsToString(xsArg(i)), "-n") == 0) {
				nonl++;
				break;
			}
			if (console_log_depth == 1)
				fprintf(stderr,  "%s", xsToString(xsArg(i)));
			else
				fprintf(stderr,  "\"%s\"", xsToString(xsArg(i)));
			break;
		case xsReferenceType:
			if (xsIsInstanceOf(xsArg(i), xsFunctionPrototype)) {
				fprintf(stderr, "function(...) { ... }");
			}
			else if (xsIsInstanceOf(xsArg(i), xsArrayPrototype)) {
				xsGet(xsVar(0), xsArg(i), xsID("length"));
				xsIntegerValue length = xsToInteger(xsVar(0)), index;
				fprintf(stderr,  "[\n");
				for (index = 0; index < length; index++) {
					xsGet(xsVar(1), xsArg(i), (xsIndex)index);
					if (comma)
						fprintf(stderr,  ",\n");
					else
						comma = 1;
					indent(0);
					fxPush(xsVar(1));
					fxPushCount(the, 1);
					fxPush(xsThis);
					fxPush(xsFunction);
					fxCall(the);
				}
				fprintf(stderr, "\n"); indent(-1); fprintf(stderr,  "]");
			}
			else {
				fprintf(stderr,  "{\n");
				xsVar(0) = xsEnumerate(xsArg(i));
				for (;;) {
					xsVar(1) = xsCall0(xsVar(0), xsID("next"));
					xsGet(xsVar(2), xsVar(1), xsID("done"));
					if (xsTest(xsVar(2)))
						break;
					xsGet(xsVar(2), xsVar(1), xsID("value"));
					xsGetAt(xsVar(3), xsArg(i), xsVar(2));
					if (comma)
						fprintf(stderr,  ",\n");
					else
						comma = 1;
					indent(0); fprintf(stderr,  "%s: ", xsToString(xsVar(2)));
					fxPush(xsVar(3));
					fxPushCount(the, 1);
					fxPush(xsThis);
					fxPush(xsFunction);
					fxCall(the);
				}
				fprintf(stderr, "\n"); indent(-1); fprintf(stderr,  "}");
			}
			break;
		}
	}
	console_log_depth--;
	if (!console_log_depth && !nonl)
		fprintf(stderr,  "\n");
}

void console_get_enable(xsMachine *the)
{
	int f = mc_log_get_enable();
	xsSetInteger(xsResult, f);
}

void console_set_enable(xsMachine *the)
{
	int f = xsToInteger(xsArg(0));
	mc_log_set_enable(f);
}
