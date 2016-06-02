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
#undef mxImport
#define mxImport extern
#include "expat.h"
#include "xs.h"

#ifdef KPR_CONFIG
#include "FskExtensions.h"
#include "FskManifest.xs.h"

FskExport(FskErr) xsGrammar_fskLoad(FskLibrary it)
{
	return kFskErrNone;
}

FskExport(FskErr) xsGrammar_fskUnload(FskLibrary it)
{
	return kFskErrNone;
}
#endif

void Grammar_assignValue(xsMachine* the)
{
	xsIntegerValue c, i;
	xsIndex id;
	c = xsToInteger(xsGet(xsArg(0), xsID_length));
	if (c) {
		xsResult = xsArg(1);
        c--;
		for (i = 0; i < c; i++) {
			id = (xsIndex)xsToInteger(xsGet(xsArg(0), i));
			xsResult = xsGet(xsResult, id);
		}
		id = (xsIndex)xsToInteger(xsGet(xsArg(0), i));
		if (id < 0) {
			if (id == -1)
				id = (xsIndex)xsToInteger(xsGet(xsResult, xsID_length));
			xsSet(xsResult, id, xsArg(2));
		}
	}
}

void Grammar_nameToID(xsMachine* the)
{
	char buffer[1024];
	xsIntegerValue id;
	xsToStringBuffer(xsArg(0), buffer, sizeof(buffer));
	if (buffer[0] == '*')
		id = 0;
	else if (buffer[0] == '@')
		id = -1;
	else
		id = xsID(buffer);
	xsResult = xsInteger(id);
}

void patchGrammar(xsMachine* the)
{
	xsVars(1);
	xsVar(0) = xsGet(xsGlobal, xsID_Grammar);
	xsNewHostProperty(xsVar(0), xsID_assignValue, xsNewHostFunction(Grammar_assignValue, 1), xsDontEnum, xsDontScript);
	xsNewHostProperty(xsVar(0), xsID_nameToID, xsNewHostFunction(Grammar_nameToID, 1), xsDontEnum, xsDontScript);
	xsNewHostProperty(xsVar(0), xsID_parse, xsNewHostFunction(xs_parse, 1), xsDontEnum, xsDontScript);
	xsNewHostProperty(xsVar(0), xsID_stringify, xsNewHostFunction(xs_serialize, 1), xsDontEnum, xsDontScript);
}

void xs_parse(xsMachine* the)
{
	xsIntegerValue c = xsToInteger(xsArgc);
	xsVars(2);
	if (c < 1)
		xsSyntaxError("no buffer");
	xsVar(0) = xsGet(xsGet(xsGlobal, xsID_xs), xsID_infoset);
	if (c > 3)
		xsVar(1) = xsCall3(xsVar(0), xsID_scan, xsArg(0), xsArg(2), xsArg(3));
	else if (c > 2)
		xsVar(1) = xsCall2(xsVar(0), xsID_scan, xsArg(0), xsArg(2));
	else if (c > 1)
		xsVar(1) = xsCall1(xsVar(0), xsID_scan, xsArg(0));
	else
		xsVar(1) = xsCall1(xsVar(0), xsID_scan, xsArg(0));
	if (c > 1)
		xsResult = xsCall2(xsVar(0), xsID_parse, xsVar(1), xsArg(1));
	else
		xsResult = xsCall1(xsVar(0), xsID_parse, xsVar(1));
}

void xs_serialize(xsMachine* the)
{
	
}





