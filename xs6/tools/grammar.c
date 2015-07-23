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
#include "xs.h"
#include "tools.xs.h"

void kprGrammar_countLines(xsMachine* the)
{
	char* aString;
	char c;
	int i = 0;
	aString = xsToString(xsArg(0));
	while ((c = *aString)) {
		if (c <= 0x20) {
			if (c == 0x0A)
				i++;
		}
		else
			break;
		aString++;
	}
	xsResult = xsInteger(i);
}

void xs_nameToID(xsMachine* the)
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

void xs_assignValue(xsMachine* the)
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

