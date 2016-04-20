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
#include "xs6All.h"
#if mxFsk
#include "FskEnvironment.h"
#endif

static void fxVReportException(void* console, txString thePath, txInteger theLine, txString theFormat, c_va_list theArguments);

#ifdef mxDebug

static void fxAbort(txMachine* the);
static txBoolean fxDebugLoopTest(txString* theBuffer, txString theName);
static txString fxDebugLoopValue(txString* theBuffer, txString theName);
static void fxEcho(txMachine* the, txString theString);
static void fxEchoAddress(txMachine* the, txSlot* theSlot);
static void fxEchoCharacter(txMachine* the, char theCharacter);
static void fxEchoFormat(txMachine* the, txString theFormat, c_va_list theArguments);
static void fxEchoFrame(txMachine* the, txSlot* theFrame, txSlot* theList);
static void fxEchoFrameName(txMachine* the, txSlot* theFrame);
static void fxEchoFramePathLine(txMachine* the, txSlot* theFrame);
static void fxEchoHost(txMachine* the, txSlot* theInstance, txSlot* theList);
static void fxEchoInteger(txMachine* the, txInteger theInteger);
static void fxEchoInstance(txMachine* the, txSlot* theInstance, txSlot* theList);
static void fxEchoNumber(txMachine* the, txNumber theNumber);
static void fxEchoPathLine(txMachine* the, txString thePath, txInteger theLine);
static void fxEchoProperty(txMachine* the, txSlot* theProperty, txSlot* theList, txString thePrefix, txInteger theIndex, txString theSuffix);
static void fxEchoStart(txMachine* the);
static void fxEchoStop(txMachine* the);
static void fxEchoString(txMachine* the, txString theString);
static void fxGo(txMachine* the);
static void fxListBreakpoints(txMachine* the);
static void fxListFiles(txMachine* the);
static void fxListFrames(txMachine* the);
static void fxListGlobal(txMachine* the);
static void fxListLocal(txMachine* the);
static void fxListModules(txMachine* the);
static txSlot* fxParseAddress(txMachine* the, txString theAddress);
static void fxSelect(txMachine* the, txString theAddress);
static void fxSortProperties(txMachine* the, txSlot** theSorter, txIndex lb, txIndex ub);
static int fxSortPropertiesCompare(txMachine* the, txSlot* i, txSlot* j);
static void fxSortPropertiesSwap(txMachine* the, txSlot** theSorter, txIndex i, txIndex j);
static void fxStep(txMachine* the);
static void fxStepInside(txMachine* the);
static void fxStepOutside(txMachine* the);
static void fxToggle(txMachine* the, txString theAddress);
static void fxReportException(txMachine* the, txString thePath, txInteger theLine, txString theFormat, ...);

void fxAbort(txMachine* the)
{
	fxWriteBreakpoints(the);
	fxDisconnect(the);
	c_exit(0);
}

void fxCheck(txMachine* the, txString thePath, txInteger theLine)
{
#if mxWindows
	fprintf(stdout, "%s(%ld): fatal!\n", thePath, (int)theLine);
#else
	fprintf(stdout, "%s:%d: fatal!\n", thePath, (int)theLine);
#endif
	c_exit(0);
}

void fxClearAllBreakpoints(txMachine* the)
{
	mxBreakpoints.value.list.first = C_NULL;
}

void fxClearBreakpoint(txMachine* the, txString thePath, txString theLine)
{
	txSlot* aKey;
	txInteger aLine;
	txSlot** aBreakpointAddress;
	txSlot* aBreakpoint;

	if (!theLine)
		return;
	if (!thePath)
		return;
	aLine = c_strtoul(theLine, NULL, 10);
	if ((aLine <= 0) || (0x00007FFF < aLine))
		return;
	aKey = fxFindName(the, thePath);
	if (!aKey)
		return;
	aBreakpointAddress = &(mxBreakpoints.value.list.first);
	while ((aBreakpoint = *aBreakpointAddress)) {
		if ((aBreakpoint->value.reference == aKey) && (aBreakpoint->ID == (txID)aLine)) {
			*aBreakpointAddress = aBreakpoint->next;
			break;
		}
		aBreakpointAddress = &(aBreakpoint->next);
	}
}

void fxDebugCommand(txMachine* the)
{
	txString echoBuffer = NULL;
	txString p;
    txString q;
    txString r;

	if (!fxIsConnected(the))
		return;
	fxNewInstance(the);

	fxReceive(the);
	echoBuffer = c_malloc(c_strlen(the->echoBuffer) + 1);
	if (!echoBuffer) goto bail;
	c_strcpy(echoBuffer, the->echoBuffer);
	p = echoBuffer;
	while (p) {
		p = c_strchr(p, '<');
		if (!p) goto bail;
		p++;
		p = c_strchr(p, '<');
		if (!p) goto bail;
		p++;
		if (fxDebugLoopTest(&p, "abort")) {
			fxAbort(the);
			goto bail;
		}
		else if (fxDebugLoopTest(&p, "clear-all-breakpoints")) {
			fxClearAllBreakpoints(the);
			fxEchoStart(the);
			fxListBreakpoints(the);
		}
		else if (fxDebugLoopTest(&p, "clear-breakpoint")) {
			q = fxDebugLoopValue(&p, "path");
			fxClearBreakpoint(the, q, fxDebugLoopValue(&p, "line"));
			fxEchoStart(the);
			fxListBreakpoints(the);
		}
		else if (fxDebugLoopTest(&p, "set-breakpoints")) {
			fxClearAllBreakpoints(the);
			r = p;
			while (r) {
				r = c_strstr(r, "<breakpoint ");
				if (!r)
					goto bail;
				q = fxDebugLoopValue(&r, "path");
				fxSetBreakpoint(the, q, fxDebugLoopValue(&r, "line"));
				p = r;
			}
			fxEchoStart(the);
			fxListBreakpoints(the);
		}
		else if (fxDebugLoopTest(&p, "set-breakpoint")) {
			q = fxDebugLoopValue(&p, "path");
			fxSetBreakpoint(the, q, fxDebugLoopValue(&p, "line"));
			fxEchoStart(the);
			fxListBreakpoints(the);
		}
		else
			goto bail;
		
		fxEcho(the, "<log></log>");
		fxEchoStop(the);
		fxSend(the);
		fxReceive(the);
		p = c_strstr(p, "HTTP/1.1 200 OK");
	}
bail:
	the->stack++;
	c_free(echoBuffer);
}

void fxDebugFile(txMachine* the, txSlot* theKey)
{
	txSlot* aFile = mxFiles.value.list.first;
	while (aFile) {
		if (aFile->value.reference == theKey)
			return;
		aFile =	aFile->next;
	}
	aFile = fxNewSlot(the);
	aFile->next = mxFiles.value.list.first;
	aFile->kind = XS_SYMBOL_KIND;
	aFile->ID = XS_NO_ID;
	aFile->value.reference = theKey;
	mxFiles.value.list.first = aFile;
	fxEchoStart(the);
	fxListFiles(the);
	fxEcho(the, "<log></log>");
	fxEchoStop(the);
	fxSend(the);
	fxReceive(the);
}

void fxDebugLine(txMachine* the)
{
	txSlot* aSlot;
	txSlot* aBreakpoint;
	txString path;

	if ((the->frame) && (aSlot = the->frame - 1) && (aSlot->next)) {
		aBreakpoint = C_NULL;
		if ((path = (txString)aSlot->next->value.key.string)) {
			aBreakpoint = mxBreakpoints.value.list.first;
			while (aBreakpoint) {
				if (aBreakpoint->value.reference->value.key.string == path)
					if (aBreakpoint->ID == aSlot->ID)
						break;
				aBreakpoint = aBreakpoint->next;
			}
		}
		if (aBreakpoint)
			fxDebugLoop(the, C_NULL, 0, "breakpoint");
		else if ((the->frame->flag & XS_STEP_OVER_FLAG))
			fxDebugLoop(the, C_NULL, 0, "step");
	}
}

void fxDebugLoop(txMachine* the, txString path, txInteger line, txString message)
{
	txSlot* frame;
	txString p;
	txString q;

	if (!fxIsConnected(the))
		return;
	fxNewInstance(the);

	fxEchoStart(the);
	frame = the->frame;
	do {
		frame->flag &= ~XS_DEBUG_FLAG;
		frame = frame->next;
	} while (frame);
	the->frame->flag |= XS_DEBUG_FLAG;
	fxListFrames(the);
	fxListLocal(the);
	fxListGlobal(the);
	fxListModules(the);
	fxListFiles(the);

	fxEcho(the, "<break");
	frame = the->frame;
	while (frame && !path) {
		txSlot* environment = frame - 1;
		if (environment->next) {
			path = environment->next->value.key.string;
			line = environment->ID;
		}
		frame = frame->next;
	}
	if (path)
		fxEchoPathLine(the, path, line);
	fxEcho(the, "># Break: ");
	fxEchoString(the, message);
	fxEcho(the, "!\n</break>\n");

	fxRemoveReadableCallback(the);
	for (;;) {
		fxEchoStop(the);
		fxSend(the);
		fxReceive(the);
		p = c_strchr(the->echoBuffer, '<');
		if (!p) goto bail;
		p++;
		p = c_strchr(p, '<');
		if (!p) goto bail;
		p++;
		if (fxDebugLoopTest(&p, "abort")) {
			fxAbort(the);
			goto bail;
		}
		else if (fxDebugLoopTest(&p, "go-logout")) {
			fxLogout(the);
			fxGo(the);
			goto bail;
		}
		else if (fxDebugLoopTest(&p, "go")) {
			fxAddReadableCallback(the);
			fxGo(the);
			goto bail;
		}
		else if (fxDebugLoopTest(&p, "step-outside")) {
			fxStepOutside(the);
			goto bail;
		}
		else if (fxDebugLoopTest(&p, "step-inside")) {
			fxStepInside(the);
			goto bail;
		}
		else if (fxDebugLoopTest(&p, "step")) {
			fxStep(the);
			goto bail;
		}
		else if (fxDebugLoopTest(&p, "clear-all-breakpoints")) {
			fxClearAllBreakpoints(the);
			fxEchoStart(the);
			fxListBreakpoints(the);
		}
		else if (fxDebugLoopTest(&p, "clear-breakpoint")) {
			q = fxDebugLoopValue(&p, "path");
			fxClearBreakpoint(the, q, fxDebugLoopValue(&p, "line"));
			fxEchoStart(the);
			fxListBreakpoints(the);
		}
		else if (fxDebugLoopTest(&p, "set-breakpoint")) {
			q = fxDebugLoopValue(&p, "path");
			fxSetBreakpoint(the, q, fxDebugLoopValue(&p, "line"));
			fxEchoStart(the);
			fxListBreakpoints(the);
		}

		else if (fxDebugLoopTest(&p, "select")) {
			fxSelect(the, fxDebugLoopValue(&p, "id"));
			fxEchoStart(the);
			fxListLocal(the);
		}

		else if (fxDebugLoopTest(&p, "toggle")) {
			fxToggle(the, fxDebugLoopValue(&p, "id"));
			fxEchoStart(the);
			fxListLocal(the);
			fxListGlobal(the);
			fxListModules(the);
		}

		else if (fxDebugLoopTest(&p, "collect-garbage")) {
			fxCollectGarbage(the);
			fxEchoStart(the);
			fxEcho(the, "<break># Garbage collected!</break>");
		}

		else {
			fxEchoStart(the);
			fxEcho(the, "<break># xsbug: no such command!</break>");
		}
	}
bail:
	the->stack++;
}

txBoolean fxDebugLoopTest(txString* theBuffer, txString theName)
{
	txBoolean aResult = 0;
	txInteger aLength = c_strlen(theName);
	if (c_strncmp(*theBuffer, theName, aLength) == 0) {
		aResult = 1;
		*theBuffer += aLength;
	}
	return aResult;
}

txString fxDebugLoopValue(txString* theBuffer, txString theName)
{
	txString p;
	txString q;

	p = c_strstr(*theBuffer, theName);
	if (!p) return C_NULL;
	p = c_strchr(p, '"');
	if (!p) return C_NULL;
	p++;
	q = c_strchr(p, '"');
	if (!q) return C_NULL;
	*q++ = 0;
	*theBuffer = q;
	return p;
}

void fxDebugThrow(txMachine* the, txString path, txInteger line, txString message)
{
	if (fxIsConnected(the) && (the->breakOnExceptionFlag))
		fxDebugLoop(the, path, line, message);
	else {
		txSlot* frame = the->frame;
		while (frame && !path) {
			txSlot* slot = frame - 1;
			if (slot->next) {
				path = slot->next->value.key.string;
				line = slot->ID;
			}
			frame = frame->next;
		}
		fxReportException(the, path, line, "%s", message);
	}
}

void fxEcho(txMachine* the, txString theString)
{
	txInteger srcLength = c_strlen(theString);
	txInteger dstLength = the->echoSize - the->echoOffset;
	while (srcLength > dstLength) {
		c_memcpy(the->echoBuffer + the->echoOffset, theString, dstLength);
		theString += dstLength;
		srcLength -= dstLength;
		the->echoOffset = the->echoSize;
		fxSend(the);
		the->echoOffset = 0;
		dstLength = the->echoSize;
	}
	c_memcpy(the->echoBuffer + the->echoOffset, theString, srcLength);
	the->echoOffset += srcLength;
}

void fxEchoAddress(txMachine* the, txSlot* theSlot)
{
	static char gxHexaDigits[] = "0123456789ABCDEF";
	unsigned long aValue = (unsigned long)theSlot;
	unsigned long aMask = 0xF;
	int aShift;

	fxEcho(the, " value=\"@");
	aShift = (8 * sizeof(aValue)) - 4;
	while (aShift >= 0) {
		fxEchoCharacter(the, gxHexaDigits[(aValue & aMask << aShift) >> aShift]);
		aShift -= 4;
	}
	/*
	fxEchoCharacter(the, gxHexaDigits[(aValue & 0xF0000000) >> 28]);
	fxEchoCharacter(the, gxHexaDigits[(aValue & 0x0F000000) >> 24]);
	fxEchoCharacter(the, gxHexaDigits[(aValue & 0x00F00000) >> 20]);
	fxEchoCharacter(the, gxHexaDigits[(aValue & 0x000F0000) >> 16]);
	fxEchoCharacter(the, gxHexaDigits[(aValue & 0x0000F000) >> 12]);
	fxEchoCharacter(the, gxHexaDigits[(aValue & 0x00000F00) >> 8]);
	fxEchoCharacter(the, gxHexaDigits[(aValue & 0x000000F0) >> 4]);
	fxEchoCharacter(the, gxHexaDigits[(aValue & 0x0000000F)]);
	*/
	fxEcho(the, "\"");
}

void fxEchoCharacter(txMachine* the, char theCharacter)
{
	char c[2];
	c[0] = theCharacter;
	c[1] = 0;
	fxEchoString(the, c);
}

void fxEchoFormat(txMachine* the, txString theFormat, c_va_list theArguments)
{
	char *p, c;

	p = theFormat;
	while ((c = *p++)) {
		if (c != '%')
			fxEchoCharacter(the, c);
		else {
			if (c_strncmp(p, "c", 1) == 0) {
				fxEchoCharacter(the, c_va_arg(theArguments, int));
				p++;
			}
			else if (c_strncmp(p, "hd", 2) == 0) {
				fxEchoInteger(the, c_va_arg(theArguments, int));
				p += 2;
			}
			else if (c_strncmp(p, "d", 1) == 0) {
				fxEchoInteger(the, c_va_arg(theArguments, int));
				p++;
			}
			else if (c_strncmp(p, "ld", 2) == 0) {
				fxEchoInteger(the, c_va_arg(theArguments, long));
				p += 2;
			}
			else if (c_strncmp(p, "g", 1) == 0) {
				fxEchoNumber(the, c_va_arg(theArguments, double));
				p++;
			}
			else if (c_strncmp(p, "s", 1) == 0) {
				char *s = c_va_arg(theArguments, char *);
				fxEchoString(the, s);
				p++;
			}
			else {
				fxEchoCharacter(the, c);
				p++;
			}
		}
	}
}

void fxEchoFrame(txMachine* the, txSlot* theFrame, txSlot* theList)
{
	txInteger aCount, anIndex;
	txSlot* aFrame;
	txSlot* aScope = C_NULL;
	txSlot* aSlot;

	fxEcho(the, "<local");
	fxEcho(the, " name=\"");
	if (theFrame->flag & XS_STRICT_FLAG)
		fxEcho(the, "! ");
	fxEchoFrameName(the, theFrame);
	fxEcho(the, "\"");
	fxEchoAddress(the, theFrame);
	fxEchoFramePathLine(the, theFrame);
	fxEcho(the, " flags=\"-\">");
	fxEchoProperty(the, theFrame + 1, theList, "(return)", -1, C_NULL);
	fxEchoProperty(the, theFrame + 2, theList, "new.target", -1, C_NULL);
	fxEchoProperty(the, theFrame + 3, theList, "(function)", -1, C_NULL);
	fxEchoProperty(the, theFrame + 4, theList, "this", -1, C_NULL);
	if (theFrame->flag & XS_C_FLAG) {
		fxEchoProperty(the, theFrame + 5, theList, "argc", -1, C_NULL);
		aCount = (theFrame + 5)->value.integer;
		for (anIndex = 0; anIndex < aCount; anIndex++) {
			fxEchoProperty(the, theFrame + 5 + aCount - anIndex, theList, "arg(", anIndex, ")");
		}
		fxEchoProperty(the, theFrame - 1, theList, "varc", -1, C_NULL);
		aCount = (theFrame- 1)->value.integer;
		for (anIndex = 0; anIndex < aCount; anIndex++) {
			fxEchoProperty(the, theFrame - 2 - anIndex, theList, "var(", anIndex, ")");
		}
	}
	else {
		aFrame = the->frame;
		if (aFrame == theFrame)
			aScope = the->scope;
		else {
			aFrame = the->frame;
			while (aFrame->next != theFrame)
				aFrame = aFrame->next;
			if (aFrame)
				aScope = aFrame->value.frame.scope;
		}
		if (aScope) {
			aSlot = theFrame - 1;
			while (aSlot > aScope) {
				aSlot--;
				if (aSlot->ID)
					fxEchoProperty(the, aSlot, theList, C_NULL, -1, C_NULL);
			}
		}
	}
	fxEcho(the, "</local>");
}

void fxEchoFrameName(txMachine* the, txSlot* theFrame)
{
	char buffer[128] = "";
	fxBufferFrameName(the, buffer, sizeof(buffer), theFrame, "");
	fxEcho(the, buffer);
}

void fxEchoFramePathLine(txMachine* the, txSlot* theFrame)
{
	txSlot* aSlot;
	if (theFrame) {
		aSlot = theFrame - 1;
		if (aSlot->next)
			fxEchoPathLine(the, aSlot->next->value.key.string, aSlot->ID);
	}
}

void fxEchoHost(txMachine* the, txSlot* theInstance, txSlot* theList)
{
	txSlot* aRoot;
	txSlot* aProperty;

	aRoot = the->stack->value.reference;
	aProperty = aRoot->next;
	while (aProperty) {
		if (aProperty->value.accessor.setter == theInstance) {
			break;
		}
		aProperty = aProperty->next;
	}
	if (aProperty) {
		txSlot* anInstance = aProperty->value.accessor.getter;
		if ((aProperty->flag & XS_DEBUG_FLAG) && (!(anInstance->flag & XS_DEBUG_FLAG))) {
			txSlot* aParent = theInstance;
			while (aParent) {
				txSlot* aParentProperty = aParent->next;
				while (aParentProperty) {
					if ((aParentProperty->kind == XS_ACCESSOR_KIND) && (aParentProperty->value.accessor.getter)) {
						txSlot* anInstanceProperty = fxGetProperty(the, anInstance, aParentProperty->ID, XS_NO_ID, XS_ANY);
						if (!anInstanceProperty) {
							txSlot* aFunction = aParentProperty->value.accessor.getter;
							if (mxIsFunction(aFunction)) {
								fxBeginHost(the);
								mxPushInteger(0);
								/* THIS */
								mxPushReference(theInstance);
								/* FUNCTION */
								mxPushReference(aFunction);
								fxCall(the);
								anInstanceProperty = fxSetProperty(the, anInstance, aParentProperty->ID, XS_NO_ID, XS_ANY);
								anInstanceProperty->kind = the->stack->kind;
								anInstanceProperty->value = the->stack->value;
								the->stack++;
								fxEndHost(the);
							}
						}
					}
					aParentProperty = aParentProperty->next;
				}
				aParent = fxGetParent(the, aParent);
			}
            anInstance->flag |= XS_DEBUG_FLAG;
		}
	}
	else {
		fxNewInstance(the);
		aProperty = fxNewSlot(the);
		aProperty->next = aRoot->next;
		aProperty->ID = XS_NO_ID;
		aProperty->flag = XS_NO_FLAG;
		aProperty->kind = XS_REFERENCE_KIND;
		aProperty->value.accessor.getter = the->stack->value.reference;
		aProperty->value.accessor.setter = theInstance;
		the->stack++;
		aRoot->next = aProperty;
	}
    fxEchoProperty(the, aProperty, theList, "(host)", -1, C_NULL);
}

void fxEchoInteger(txMachine* the, txInteger theInteger)
{
	char aBuffer[256];

	fxIntegerToString(the->dtoa, theInteger, aBuffer, sizeof(aBuffer));
	fxEcho(the, aBuffer);
}

void fxEchoInstance(txMachine* the, txSlot* theInstance, txSlot* theList)
{
	char aName[6] = "(...)";
	txSlot aKey;
	txSlot* aParent;
	txSlot* aProperty;
	txSlot* aSlot;
	txInteger aCount;
	txInteger anIndex;

	aKey.next = C_NULL;
	aKey.value.key.string = C_NULL;
	aKey.value.key.sum = 0;

	theInstance->flag |= XS_LEVEL_FLAG;
	theInstance->value.instance.garbage = theList->value.list.last;
	theList->value.list.last->next = &aKey;
	theList->value.list.last = &aKey;

	aParent = fxGetParent(the, theInstance);
	if (aParent) {
		fxEcho(the, "<property name=\"(...)\"");
		theList->value.list.last->value.key.string = aName;
		if (aParent->flag & XS_LEVEL_FLAG) {
			txSlot* aFirst = theList->value.list.first;
			txSlot* aLast = aParent->value.instance.garbage->next;
			txBoolean aDot = 0;
			fxEcho(the, " value=\"");
			while (aFirst != aLast) {
				if (aFirst->value.key.string) {
					if (aDot)
						fxEcho(the, ".");
					fxEchoString(the, aFirst->value.key.string);
					aDot = 1;
				}
				aFirst = aFirst->next;
			}
			fxEcho(the, "\" flags=\" ");
		}
		else {
			fxEchoAddress(the, theInstance);
			if (theInstance->flag & XS_DEBUG_FLAG)
				fxEcho(the, " flags=\"-");
			else
				fxEcho(the, " flags=\"+");
		}
		fxEcho(the, "CEW");
		fxEcho(the, "\"");
		if (aParent->flag & XS_LEVEL_FLAG)
			fxEcho(the, "/>");
		else if (theInstance->flag & XS_DEBUG_FLAG) {
			fxEcho(the, ">");
			fxEchoInstance(the, aParent, theList);
			fxEcho(the, "</property>");
		}
		else
			fxEcho(the, "/>");
		theList->value.list.last->value.key.string = C_NULL;
	}

	aProperty = theInstance->next;
	if (aProperty && (aProperty->flag & XS_INTERNAL_FLAG)) {
		switch (aProperty->kind) {
		case XS_CALLBACK_KIND:
		case XS_CODE_KIND:
		case XS_CODE_X_KIND:
			fxEchoProperty(the, aProperty, theList, "(function)", -1, C_NULL);
			aProperty = aProperty->next;
			if (aProperty->kind != XS_NULL_KIND)
				fxEchoProperty(the, aProperty, theList, "(home)", -1, C_NULL);
			aProperty = aProperty->next;
		#ifdef mxProfile
			aProperty = aProperty->next;
		#endif
			break;
		case XS_ARRAY_KIND:
			fxEchoProperty(the, aProperty, theList, "(array)", -1, C_NULL);
			break;
		case XS_ARRAY_BUFFER_KIND:
			fxEchoProperty(the, aProperty, theList, "(buffer)", -1, C_NULL);
			aProperty = aProperty->next;
			break;
		case XS_STRING_KIND:
		case XS_STRING_X_KIND:
			fxEchoProperty(the, aProperty, theList, "(string)", -1, C_NULL);
			aProperty = aProperty->next;
			break;
		case XS_BOOLEAN_KIND:
			fxEchoProperty(the, aProperty, theList, "(boolean)", -1, C_NULL);
			aProperty = aProperty->next;
			break;
		case XS_NUMBER_KIND:
			fxEchoProperty(the, aProperty, theList, "(number)", -1, C_NULL);
			aProperty = aProperty->next;
			break;
		case XS_DATA_VIEW_KIND:
			fxEchoProperty(the, aProperty, theList, "(view)", -1, C_NULL);
			aProperty = aProperty->next;
			fxEchoProperty(the, aProperty, theList, "(data)", -1, C_NULL);
			aProperty = aProperty->next;
			break;
		case XS_DATE_KIND:
			fxEchoProperty(the, aProperty, theList, "(date)", -1, C_NULL);
			aProperty = aProperty->next;
			break;
		case XS_REGEXP_KIND:
			fxEchoProperty(the, aProperty, theList, "(regexp)", -1, C_NULL);
			aProperty = aProperty->next;
			break;
		case XS_HOST_KIND:
			if (/*theFlag && */aProperty->value.host.data)
				fxEchoHost(the, theInstance, theList);
			else
           		fxEchoProperty(the, aProperty, theList, "(host)", -1, C_NULL);
			aProperty = aProperty->next;
			break;
		case XS_GLOBAL_KIND:
			break;
		case XS_PROMISE_KIND:
			break;
		case XS_STAR_KIND:
			aProperty = aProperty->next;
			break;
		case XS_MAP_KIND:
			aProperty = aProperty->next;
			anIndex = 0;
			aSlot = aProperty->value.list.first;
			while (aSlot) {
				if (!(aSlot->flag & XS_DONT_ENUM_FLAG)) {
					fxEchoProperty(the, aSlot, theList, "(.", anIndex, ")");
					fxEchoProperty(the, aSlot->next, theList, "(..", anIndex, ")");
				}
				anIndex++;
				aSlot = aSlot->next->next;
			}
			aProperty = aProperty->next;
			break;
		case XS_MODULE_KIND:
			aProperty = aProperty->next;
			aProperty = aProperty->next;
			aProperty = aProperty->next;
			fxEchoProperty(the, aProperty, theList, "(export)", -1, C_NULL);
			aProperty = aProperty->next;
			fxEchoProperty(the, aProperty, theList, "(uri)", -1, C_NULL);
			aProperty = aProperty->next;
			break;
		case XS_SET_KIND:
			aProperty = aProperty->next;
			anIndex = 0;
			aSlot = aProperty->value.list.first;
			while (aSlot) {
				if (!(aSlot->flag & XS_DONT_ENUM_FLAG))
					fxEchoProperty(the, aSlot, theList, "(.", anIndex, ")");
				anIndex++;
				aSlot = aSlot->next;
			}
			aProperty = aProperty->next;
			break;
		case XS_TYPED_ARRAY_KIND:
			fxEchoProperty(the, aProperty, theList, "(per item)", -1, C_NULL);
			aProperty = aProperty->next;
			fxEchoProperty(the, aProperty, theList, "(view)", -1, C_NULL);
			aProperty = aProperty->next;
			fxEchoProperty(the, aProperty, theList, "(items)", -1, C_NULL);
			aProperty = aProperty->next;
			break;
		case XS_PROXY_KIND:
			fxEchoProperty(the, aProperty, theList, "(proxy)", -1, C_NULL);
			break;
		}
	}
	aCount = 0;
	aSlot = aProperty;
	while (aSlot) {
		if (aSlot->ID < -1)
			aCount++;
		else {
			if (aSlot->kind == XS_ARRAY_KIND) {
				txSlot* item = aProperty->value.array.address;
				txIndex c = fxGetArraySize(the, aProperty), i;
				for (i = 0; i < c; i++) {
					txIndex index = *((txIndex*)item);
					fxEchoProperty(the, item, theList, "[", index, "]");
					item++;
				}
			}
		}
		aSlot = aSlot->next;
	}
	if (theInstance->ID >= 0) {
		aSlot = the->aliasArray[theInstance->ID];
		if (aSlot) {
			aSlot = aSlot->next;
			while (aSlot) {
				if (aSlot->ID < -1)
					aCount++;
				aSlot = aSlot->next;
			}
		}
	}
	if (aCount) {
		txSlot** aSorter = c_malloc(aCount * sizeof(txSlot*));
		if (aSorter) {
			txSlot** aSlotAddress = aSorter;
			aSlot = aProperty;
			while (aSlot) {
				if (aSlot->ID < -1)
					*aSlotAddress++ = aSlot;
				aSlot = aSlot->next;
			}
			if (theInstance->ID >= 0) {
				aSlot = the->aliasArray[theInstance->ID];
				if (aSlot) {
					aSlot = aSlot->next;
					while (aSlot) {
						if (aSlot->ID < -1)
							*aSlotAddress++ = aSlot;
						aSlot = aSlot->next;
					}
				}
			}
			fxSortProperties(the, aSorter, 0, aCount);
			aSlotAddress = aSorter;
			while (aCount) {
				aSlot = *aSlotAddress++;
				fxEchoProperty(the, aSlot, theList, C_NULL, -1, C_NULL);
				aCount--;
			}
			c_free(aSorter);
		}
		else {
			aSlot = aProperty;
			while (aSlot) {
				if (aSlot->ID < -1)
					fxEchoProperty(the, aSlot, theList, C_NULL, -1, C_NULL);
				aSlot = aSlot->next;
			}
			if (theInstance->ID >= 0) {
				aSlot = the->aliasArray[theInstance->ID];
				if (aSlot) {
					aSlot = aSlot->next;
					while (aSlot) {
						if (aSlot->ID < -1)
							fxEchoProperty(the, aSlot, theList, C_NULL, -1, C_NULL);
						aSlot = aSlot->next;
					}
				}
			}
		}
	}
	theList->value.list.last = theInstance->value.instance.garbage;
	theList->value.list.last->next = C_NULL;
	theInstance->value.instance.garbage = C_NULL;
	theInstance->flag &= ~XS_LEVEL_FLAG;
}

void fxEchoNumber(txMachine* the, txNumber theNumber)
{
	char aBuffer[256];

	fxNumberToString(the->dtoa, theNumber, aBuffer, sizeof(aBuffer), 0, 0);
	fxEcho(the, aBuffer);
}

void fxEchoPathLine(txMachine* the, txString thePath, txInteger theLine)
{
	if (thePath && theLine) {
		fxEcho(the, " path=\"");
		fxEchoString(the, thePath);
		fxEcho(the, "\"");
		fxEcho(the, " line=\"");
		fxEchoInteger(the, theLine);
		fxEcho(the, "\"");
	}
}

void fxEchoInstanceProperty(txMachine* the, txSlot* theProperty, txSlot* theList, txSlot* theInstance)
{
	if (theInstance) {
		if (theInstance->flag & XS_LEVEL_FLAG) {
			txSlot* aFirst = theList->value.list.first;
			txSlot* aLast = theInstance->value.instance.garbage->next;
			txBoolean aDot = 0;
			fxEcho(the, " value=\"");
			while (aFirst && (aFirst != aLast)) {
				if (aFirst->value.key.string) {
					if (aDot)
						fxEcho(the, ".");
					fxEchoString(the, aFirst->value.key.string);
					aDot = 1;
				}
				aFirst = aFirst->next;
			}
			fxEcho(the, "\"/>");
		}
		else {
			if (theProperty->flag & XS_DEBUG_FLAG) {
				fxEcho(the, ">");
				fxEchoInstance(the, theInstance, theList);
				fxEcho(the, "</property>");
			}
			else
				fxEcho(the, "/>");
		}
	}
	else {
		fxEcho(the, " value=\"null\"/>");
	}
}

void fxEchoProperty(txMachine* the, txSlot* theProperty, txSlot* theList, txString thePrefix, txInteger theIndex, txString theSuffix)
{
	char aName[256];
	txSlot* anInstance;

	c_strcpy(aName, "");
	if (thePrefix) {
		c_strcat(aName, thePrefix);
		if (theSuffix) {
			if (theIndex >= 0) {
				int aLength = c_strlen(aName);
				fxIntegerToString(the->dtoa, theIndex, aName + aLength, sizeof(aName) - aLength);
				c_strcat(aName, theSuffix);
			}
			else {
				c_strcat(aName, " (");
				c_strcat(aName, theSuffix);
				c_strcat(aName, ")");
			}
		}
	}
	else {
		fxIDToString(the, theProperty->ID, aName, sizeof(aName));
	}

	if (theProperty->kind == XS_CLOSURE_KIND)
		theProperty = theProperty->value.closure;

	if (mxIsReference(theProperty))
		anInstance = fxGetInstance(the, theProperty);
	else
		anInstance = C_NULL;

	fxEcho(the, "<property");

	fxEcho(the, " flags=\"");
	if (theProperty->flag & XS_DEBUG_FLAG)
		fxEcho(the, "-");
	else if (theProperty->kind == XS_ACCESSOR_KIND)
		fxEcho(the, "+");
	else if (theProperty->kind == XS_HOME_KIND)
		fxEcho(the, "+");
	else if (theProperty->kind == XS_PROXY_KIND)
		fxEcho(the, "+");
	else if (anInstance && !(anInstance->flag & XS_LEVEL_FLAG))
		fxEcho(the, "+");
	else
		fxEcho(the, " ");
	if (theProperty->flag & XS_DONT_DELETE_FLAG)
		fxEcho(the, "C");
	else
		fxEcho(the, "c");
	if (theProperty->flag & XS_DONT_ENUM_FLAG)
		fxEcho(the, "E");
	else
		fxEcho(the, "e");
	if (theProperty->kind == XS_ACCESSOR_KIND)
		fxEcho(the, "_");
	else  if (theProperty->flag & XS_DONT_SET_FLAG)
		fxEcho(the, "W");
	else
		fxEcho(the, "w");
	if ((theProperty->kind == XS_REFERENCE_KIND) && (anInstance->ID >= 0))
		fxEcho(the, ".");
	fxEcho(the, "\"");

	fxEcho(the, " name=\"");
	fxEchoString(the, aName);
	fxEcho(the, "\"");

	theList->value.list.last->value.key.string = aName;

	switch (theProperty->kind) {
	case XS_REFERENCE_KIND:
        if (anInstance) {
			if (anInstance->flag & XS_LEVEL_FLAG) {
				txSlot* aFirst = theList->value.list.first;
				txSlot* aLast = anInstance->value.instance.garbage->next;
				txBoolean aDot = 0;
				fxEcho(the, " value=\"");
				while (aFirst && (aFirst != aLast)) {
					if (aFirst->value.key.string) {
						if (aDot)
							fxEcho(the, ".");
						fxEchoString(the, aFirst->value.key.string);
						aDot = 1;
					}
					aFirst = aFirst->next;
				}
				fxEcho(the, "\"/>");
			}
			else {
				fxEchoAddress(the, theProperty);
				if (theProperty->flag & XS_DEBUG_FLAG) {
					fxEcho(the, ">");
					fxEchoInstance(the, anInstance, theList);
					fxEcho(the, "</property>");
				}
				else
					fxEcho(the, "/>");
			}
		}
		else {
			fxEcho(the, " value=\"null\"/>");
		}
		break;
	case XS_ACCESSOR_KIND:
		fxEchoAddress(the, theProperty);
		if (theProperty->flag & XS_DEBUG_FLAG) {
			fxEcho(the, ">");
			fxEcho(the, "<property flags=\" \" name=\"get\"");
			if (theProperty->value.accessor.getter) {
				fxEcho(the, ">");
				fxEchoInstance(the, theProperty->value.accessor.getter, theList);
				fxEcho(the, "</property>");
			}
			else {
				fxEcho(the, " value=\"undefined\"/>");
			}
			fxEcho(the, "<property flags=\" \" name=\"set\"");
			if (theProperty->value.accessor.setter) {
				fxEcho(the, ">");
				fxEchoInstance(the, theProperty->value.accessor.setter, theList);
				fxEcho(the, "</property>");
			}
			else {
				fxEcho(the, " value=\"undefined\"/>");
			}
			fxEcho(the, "</property>");
		}
		else
			fxEcho(the, "/>");
		break;
	case XS_HOME_KIND:
		fxEchoAddress(the, theProperty);
		if (theProperty->flag & XS_DEBUG_FLAG) {
			fxEcho(the, ">");
			fxEcho(the, "<property flags=\" \" name=\"(object)\"");
			fxEchoInstanceProperty(the, theProperty, theList, theProperty->value.home.object);
			fxEcho(the, "<property flags=\" \" name=\"(module)\"");
			fxEchoInstanceProperty(the, theProperty, theList, theProperty->value.home.module);
			fxEcho(the, "</property>");
		}
		else
			fxEcho(the, "/>");
		break;
	case XS_PROXY_KIND:
		fxEchoAddress(the, theProperty);
		if (theProperty->flag & XS_DEBUG_FLAG) {
			fxEcho(the, ">");
			fxEcho(the, "<property flags=\" \" name=\"(target)\"");
			if (theProperty->value.proxy.target) {
				fxEcho(the, ">");
				fxEchoInstance(the, theProperty->value.proxy.target, theList);
				fxEcho(the, "</property>");
			}
			else {
				fxEcho(the, " value=\"undefined\"/>");
			}
			fxEcho(the, "<property flags=\" \" name=\"(handler)\"");
			if (theProperty->value.proxy.handler) {
				fxEcho(the, ">");
				fxEchoInstance(the, theProperty->value.proxy.handler, theList);
				fxEcho(the, "</property>");
			}
			else {
				fxEcho(the, " value=\"undefined\"/>");
			}
			fxEcho(the, "</property>");
		}
		else
			fxEcho(the, "/>");
		break;

	case XS_UNDEFINED_KIND:
		fxEcho(the, " value=\"undefined\"/>");
		break;
	case XS_NULL_KIND:
		fxEcho(the, " value=\"null\"/>");
		break;

	case XS_CALLBACK_KIND:
		fxEcho(the, " value=\"(C code)\"/>");
		break;
	case XS_CODE_KIND:
	case XS_CODE_X_KIND:
		fxEcho(the, " value=\"(byte code)\"/>");
		break;
	case XS_ARRAY_KIND:
		fxEcho(the, " value=\"");
		fxEchoInteger(the, theProperty->value.array.length);
		fxEcho(the, " items\"/>");
		break;
	case XS_ARRAY_BUFFER_KIND:
		fxEcho(the, " value=\"");
		fxEchoInteger(the, theProperty->value.arrayBuffer.length);
		fxEcho(the, " bytes\"/>");
		break;
	case XS_STRING_KIND:
	case XS_STRING_X_KIND:
		fxEcho(the, " value=\"'");
		fxEchoString(the, theProperty->value.string);
		fxEcho(the, "'\"/>");
		break;
	case XS_BOOLEAN_KIND:
		fxEcho(the, " value=\"");
		if (theProperty->value.boolean)
			fxEcho(the, "true");
		else
			fxEcho(the, "false");
		fxEcho(the, "\"/>");
		break;
	case XS_INTEGER_KIND:
		fxEcho(the, " value=\"");
		fxEchoInteger(the, theProperty->value.integer);
		fxEcho(the, "\"/>");
		break;
	case XS_NUMBER_KIND:
		fxEcho(the, " value=\"");
		fxEchoNumber(the, theProperty->value.number);
		fxEcho(the, "\"/>");
		break;
	case XS_DATE_KIND:
		fxEcho(the, " value=\"");
		fxEchoNumber(the, theProperty->value.number);
		fxEcho(the, "\"/>");
		break;
	case XS_REGEXP_KIND:
		fxEcho(the, " value=\"\"/>");
		break;
	case XS_HOST_KIND:
		fxEcho(the, " value=\"\"/>");
		break;
	case XS_KEY_KIND:
	case XS_KEY_X_KIND:
		fxEcho(the, " value=\"'");
		fxEchoString(the, theProperty->value.key.string);
		fxEcho(the, "'\"/>");
		break;
	case XS_SYMBOL_KIND:
		anInstance = fxGetKey(the, theProperty->value.symbol);
		fxEcho(the, " value=\"Symbol(");
		if (anInstance)
			fxEchoString(the, anInstance->value.string);
		fxEcho(the, ")\"/>");
		break;
	case XS_DATA_VIEW_KIND:
		fxEcho(the, " value=\"");
		fxEchoInteger(the, theProperty->value.dataView.offset);
		fxEcho(the, ", ");
		fxEchoInteger(the, theProperty->value.dataView.size);
		fxEcho(the, " bytes\"/>");
		break;
	case XS_TYPED_ARRAY_KIND:
		fxEcho(the, " value=\"");
		fxEchoInteger(the, theProperty->value.typedArray->size);
		fxEcho(the, " bytes\"/>");
		break;
	default:
		fxEcho(the, "/>");
		break;
	}
}

void fxEchoStart(txMachine* the)
{
	the->echoOffset = 0;
	fxEcho(the, "POST / HTTP/1.1\15\12Content-type: \"xml/xsbug\"\15\12\15\12");
	fxEcho(the, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
	fxEcho(the, "<xsbug>");
}

void fxEchoStop(txMachine* the)
{
	fxEcho(the, "</xsbug>");
}

void fxEchoString(txMachine* the, txString theString)
{
	static txByte gxEscape[256] = {
	/* 0 1 2 3 4 5 6 7 8 9 A B C D E F */
		 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 0x                    */
		 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 1x                    */
		 1,1,0,1,1,1,0,1,1,1,1,1,1,1,1,1,	/* 2x   !"#$%&'()*+,-./  */
		 1,1,1,1,1,1,1,1,1,1,1,1,0,1,0,1,	/* 3x  0123456789:;<=>?  */
		 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,	/* 4x  @ABCDEFGHIJKLMNO  */
		 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,	/* 5X  PQRSTUVWXYZ[\]^_  */
		 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,	/* 6x  `abcdefghijklmno  */
		 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0, 	/* 7X  pqrstuvwxyz{|}~   */
		 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,	/* 8X                    */
		 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,	/* 9X                    */
		 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,	/* AX                    */
		 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,	/* BX                    */
		 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,	/* CX                    */
		 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,	/* FX                    */
		 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,	/* EX                    */
		 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 	/* FX                    */
	};
	unsigned char tmp;
	unsigned char* src;
	unsigned char* dst;
	unsigned char* start;
	unsigned char* stop;

	src = (unsigned char*)theString;
	dst = (unsigned char*)the->echoBuffer + the->echoOffset;
	start = (unsigned char*)the->echoBuffer;
	stop = (unsigned char*)the->echoBuffer + the->echoSize - 1;
	while ((tmp = *src++)) {
		if (dst + 6 > stop) {
			the->echoOffset = dst - start;
			fxSend(the);
			dst = start;
		}
		if (gxEscape[tmp])
			*dst++ = tmp;
		else {
			*(dst++) = '&';
			*(dst++) = '#';
			if (tmp >= 100) {
				*(dst++) = '0' + (tmp / 100);
				tmp %= 100;
				*(dst++) = '0' + (tmp / 10);
				tmp %= 10;
				*(dst++) = '0' + tmp;
			}
			else if (tmp >= 10) {
				*(dst++) = '0' + (tmp / 10);
				tmp %= 10;
				*(dst++) = '0' + tmp;
			}
			else {
				*(dst++) = '0' + tmp;
			}
			*(dst++) = ';';
		}
	}
	the->echoOffset = dst - start;
}

void fxGo(txMachine* the)
{
	txSlot* aSlot = the->frame;
	while (aSlot) {
		aSlot->flag &= ~(XS_STEP_INTO_FLAG | XS_STEP_OVER_FLAG);
		aSlot = aSlot->next;
	}
}

txSlot* fxGetNextBreakpoint(txMachine* the, txSlot* theBreakpoint, txString thePath, txString theLine)
{
	if (theBreakpoint)
		theBreakpoint = theBreakpoint->next;
	else
		theBreakpoint = mxBreakpoints.value.list.first;
	if (theBreakpoint) {
		c_strcpy(thePath, theBreakpoint->value.reference->value.key.string);
		fxIntegerToString(the->dtoa, theBreakpoint->ID, theLine, 32);
	}
	return theBreakpoint;
}

void fxListBreakpoints(txMachine* the)
{
	txSlot* aBreakpoint;

	fxEcho(the, "<breakpoints>");
	aBreakpoint = mxBreakpoints.value.list.first;
	while (aBreakpoint)	{
		fxEcho(the, "<breakpoint  path=\"");
		fxEchoString(the, aBreakpoint->value.reference->value.key.string);
		fxEcho(the, "\" line=\"");
		fxEchoInteger(the, aBreakpoint->ID);
		fxEcho(the, "\"/>");
		aBreakpoint = aBreakpoint->next;
	}
	fxEcho(the, "</breakpoints>");
}

void fxListFiles(txMachine* the)
{
	txSlot* aFile;

	fxEcho(the, "<files>");
	aFile = mxFiles.value.list.first;
	while (aFile)	{
		fxEcho(the, "<file  path=\"");
		fxEchoString(the, aFile->value.reference->value.key.string);
		fxEcho(the, "\"/>");
		aFile = aFile->next;
	}
	fxEcho(the, "</files>");
}

void fxListFrames(txMachine* the)
{
	txSlot* aFrame;

	fxEcho(the, "<frames>");
	aFrame = the->frame;
	while (aFrame) {
		fxEcho(the, "<frame");
		fxEcho(the, " name=\"");
		if (aFrame->flag & XS_STRICT_FLAG)
			fxEcho(the, "! ");
		fxEchoFrameName(the, aFrame);
		fxEcho(the, "\"");
		fxEchoAddress(the, aFrame);
		fxEchoFramePathLine(the, aFrame);
		fxEcho(the, "/>");
		aFrame = aFrame->next;
	}
	fxEcho(the, "</frames>");
}

void fxListGlobal(txMachine* the)
{
	txSlot aKey;
	txSlot aList;
	txSlot* aProperty;
	txInteger aCount;
	txSlot** aSlotAddress;

	aKey.next = C_NULL;
	aKey.value.key.string = C_NULL;
	aKey.value.key.sum = 0;

	aList.value.list.first = &aKey;
	aList.value.list.last = &aKey;

	fxEcho(the, "<global>");
	aProperty = mxGlobal.value.reference->next->next;
	aCount = 0;
	aSlotAddress = the->sorter;
	while (aProperty) {
		if (aProperty->ID >= -1)
			fxEchoProperty(the, aProperty, &aList, C_NULL, -1, C_NULL);
		else {
			aCount++;
			*aSlotAddress++ = aProperty;
		}
		aProperty = aProperty->next;
	}
	if (aCount) {
		fxSortProperties(the, the->sorter, 0, aCount);
		aSlotAddress = the->sorter;
		while (aCount) {
			aProperty = *aSlotAddress++;
			fxEchoProperty(the, aProperty, &aList, C_NULL, -1, C_NULL);
			aCount--;
		}
	}
	fxEcho(the, "</global>");
}

void fxListLocal(txMachine* the)
{
	txSlot aList;
	txSlot aKey;
	txSlot* aFrame;

	aKey.next = C_NULL;
	aKey.value.key.string = C_NULL;
	aKey.value.key.sum = 0;
	aList.value.list.first = &aKey;
	aList.value.list.last = &aKey;
	aFrame = the->frame;
	while (aFrame) {
		if (aFrame->flag & XS_DEBUG_FLAG)
			fxEchoFrame(the, aFrame, &aList);
		aFrame = aFrame->next;
	}
}

void fxEchoExport(txMachine* the, txSlot* export, txSlot* list)
{
	txSlot* closure = mxTransferClosure(export);
	txSlot* key = fxGetKey(the, export->ID);
	fxEchoProperty(the, closure, list, key->value.key.string, 0, C_NULL);
}

void fxListModules(txMachine* the)
{
	txSlot aList;
	txSlot aKey;
	txSlot* table = mxModules.value.reference->next;
	txSlot** address = table->value.table.address;
	txInteger modulo = table->value.table.length;

	txSlot* slot;
	txSlot* key;
	txInteger c;
	txSlot* transfers;
	txSlot* transfer;
	txSlot* exports;
	txSlot* export;
	aKey.next = C_NULL;
	aKey.value.key.string = C_NULL;
	aKey.value.key.sum = 0;
	aList.value.list.first = &aKey;
	aList.value.list.last = &aKey;
	fxEcho(the, "<grammar>");
	
	while (modulo) {
		txSlot* entry = *address;
		while (entry) {
			txSlot* module = entry->value.entry.slot;
			
			fxEcho(the, "<node");
			fxEcho(the, " flags=\"");
			if (module->flag & XS_DEBUG_FLAG)
				fxEcho(the, "-");
			else
				fxEcho(the, "+");
			fxEcho(the, "\"");
			fxEcho(the, " name=\"");
			slot = mxModuleID(module);
			if ((slot->kind == XS_STRING_KIND) || (slot->kind == XS_STRING_X_KIND))
				fxEcho(the, slot->value.string);
			else
				fxEcho(the, "(c)");
			fxEcho(the, "\"");
			fxEchoAddress(the, module);
			if (module->flag & XS_DEBUG_FLAG) {
				fxEcho(the, ">");
				slot = mxModuleURI(module);
				if (slot->kind == XS_SYMBOL_KIND) {
					key = fxGetKey(the, slot->value.symbol);
					fxEcho(the, "<property flags=\" cew\" name=\"(uri)\" value=\"");
					fxEchoString(the, key->value.key.string);
					fxEcho(the, "\"/>");
				}
				exports = mxModuleExports(module);
				export = exports->value.reference->next;
				c = 0;
				while (export) {
					c++;
					export = export->next;
				}
				if (c) {
					fxEcho(the, "<node");
					fxEcho(the, " name=\"(export)\"");
					fxEcho(the, " flags=\"");
					if (exports->flag & XS_DEBUG_FLAG)
						fxEcho(the, "-");
					else if (c)
						fxEcho(the, "+");
					else
						fxEcho(the, " ");
					fxEcho(the, "\"");
					fxEchoAddress(the, exports);
					if (exports->flag & XS_DEBUG_FLAG) {
						txSlot** sorter = c_malloc(c * sizeof(txSlot*));
						fxEcho(the, ">");
						if (sorter) {
							txSlot** address = sorter;
							export = exports->value.reference->next;
							while (export) {
								*address++ = export;
								export = export->next;
							}
							fxSortProperties(the, sorter, 0, c);
							address = sorter;
							while (c) {
								export = *address++;
								fxEchoExport(the, export, &aList);
								c--;
							}
							c_free(sorter);
						}
						else {
							export = exports->value.reference->next;
							while (export) {
								fxEchoExport(the, export, &aList);
								export = export->next;
							}
						}
						fxEcho(the, "</node>");
					}
					else
						fxEcho(the, "/>");
				}

				transfers = mxModuleTransfers(module);
				transfer = transfers->value.reference->next;
				c = 0;
				while (transfer) {
					txSlot* local = mxTransferLocal(transfer);
					if (local->kind != XS_NULL_KIND)
						c++;
					transfer = transfer->next;
				}
				if (c) {
					fxEcho(the, "<node");
					fxEcho(the, " name=\"(scope)\"");
					fxEcho(the, " flags=\"");
					if (transfers->flag & XS_DEBUG_FLAG)
						fxEcho(the, "-");
					else if (c)
						fxEcho(the, "+");
					else
						fxEcho(the, " ");
					fxEcho(the, "\"");
					fxEchoAddress(the, transfers);
					if (transfers->flag & XS_DEBUG_FLAG) {
						txSlot** sorter = c_malloc(c * sizeof(txSlot*));
						fxEcho(the, ">");
						if (sorter) {
							txSlot** address = sorter;
							transfer = transfers->value.reference->next;
							while (transfer) {
								txSlot* closure = mxTransferClosure(transfer);
								*address++ = closure;
								transfer = transfer->next;
							}
							fxSortProperties(the, sorter, 0, c);
							address = sorter;
							while (c) {
								txSlot* closure = *address++;
								fxEchoProperty(the, closure,  &aList,C_NULL, 0, C_NULL);
								c--;
							}
							c_free(sorter);
						}
						else {
							transfer = transfers->value.reference->next;
							while (transfer) {
								txSlot* closure = mxTransferClosure(transfer);
								fxEchoProperty(the, closure,  &aList,C_NULL, 0, C_NULL);
								transfer = transfer->next;
							}
						}
						fxEcho(the, "</node>");
					}
					else
						fxEcho(the, "/>");
				}
				
				fxEcho(the, "</node>");
			}
			else
				fxEcho(the, "/>");
				
			entry = entry->next;
		}
		address++;
		modulo--;
	}
	fxEcho(the, "</grammar>");
}

void fxLogin(txMachine* the)
{
#if mxFsk
	txString name;
#endif		
	if (!fxIsConnected(the)) {
		fxConnect(the);
		if (!fxIsConnected(the))
			return;
	}
	fxReadBreakpoints(the);
	fxEchoStart(the);
	fxEcho(the, "<login name=\"");
	if (the->name)
		fxEchoString(the, the->name);
	else
		fxEchoString(the, "xslib");
	fxEcho(the, "\" value=\"");
#if mxFsk
	name = FskEnvironmentGet("NAME");
	if (name)
		fxEchoString(the, name);
	else
#endif		
		fxEcho(the, "XS6");
	fxEcho(the, "\"/>");
	fxListBreakpoints(the);
	fxEchoStop(the);
	fxSend(the);
	fxDebugCommand(the);
}

void fxLogout(txMachine* the)
{
	if (!fxIsConnected(the))
		return;
	fxWriteBreakpoints(the);
	fxDisconnect(the);
}

txSlot* fxParseAddress(txMachine* the, txString theAddress)
{
	unsigned char* p = (unsigned char*)theAddress + 1;
	unsigned long c;
	unsigned long aValue = 0;
	int aShift = (8 * sizeof(aValue)) - 4;
	while (aShift >= 0) {
		c = *p++;
		c -= '0';
		if (c > 9) c -= 7;
		aValue |= c << aShift;
		aShift -= 4;
	}
	return (txSlot*)aValue;
}

void fxSelect(txMachine* the, txString theAddress)
{
	txSlot* aSelection;
	txSlot* aFrame;

	aSelection = fxParseAddress(the, theAddress);
	aFrame = the->frame;
	while (aFrame) {
		if (aFrame == aSelection)
			aFrame->flag |= XS_DEBUG_FLAG;
		else
			aFrame->flag &= ~XS_DEBUG_FLAG;
		aFrame = aFrame->next;
	}
}

void fxSetBreakpoint(txMachine* the, txString thePath, txString theLine)
{
	txSlot* aKey;
	txInteger aLine;
	txSlot* aBreakpoint;

	if (!thePath)
		return;
	if (!theLine)
		return;
	aLine = c_strtoul(theLine, NULL, 10);
	if ((aLine <= 0) || (0x00007FFF < aLine))
		return;
	aKey = fxNewNameC(the, thePath);
	if (!aKey)
		return;
	fxDebugFile(the, aKey);
	aBreakpoint = mxBreakpoints.value.list.first;
	while (aBreakpoint)	{
		if ((aBreakpoint->value.reference == aKey) && (aBreakpoint->ID == (txID)aLine))
			break;
		aBreakpoint = aBreakpoint->next;
	}
	if (!aBreakpoint) {
		aBreakpoint = fxNewSlot(the);
		aBreakpoint->next = mxBreakpoints.value.list.first;
		aBreakpoint->kind = XS_SYMBOL_KIND;
		aBreakpoint->ID = (txID)aLine;
		aBreakpoint->value.reference = aKey;
		mxBreakpoints.value.list.first = aBreakpoint;
	}
}

void fxSortProperties(txMachine* the, txSlot** theSorter, txIndex beg, txIndex end)
{
  if (end > beg + 1) {
    txSlot* piv = theSorter[beg];
    txIndex l = beg + 1, r = end;
    while (l < r) {
      if (fxSortPropertiesCompare(the, theSorter[l], piv) <= 0)
        l++;
      else
        fxSortPropertiesSwap(the, theSorter, l, --r);
    }
    fxSortPropertiesSwap(the, theSorter, --l, beg);
    fxSortProperties(the, theSorter, beg, l);
    fxSortProperties(the, theSorter, r, end);
  }
}

int fxSortPropertiesCompare(txMachine* the, txSlot* i, txSlot* j)
{
	i = fxGetKey(the, i->ID);
	j = fxGetKey(the, j->ID);
	if (i && j)
		return c_strcmp(i->value.key.string, j->value.key.string);
	if (i)
		return 1;
	if (j)
		return -1;
	return 0;
}

void fxSortPropertiesSwap(txMachine* the, txSlot** theSorter, txIndex i, txIndex j)
{
	txSlot* a = theSorter[i];
	txSlot* b = theSorter[j];
	theSorter[i] = b;
	theSorter[j] = a;
}

void fxStep(txMachine* the)
{
	txSlot* aSlot = the->frame;
	while (aSlot) {
		aSlot->flag &= ~XS_STEP_INTO_FLAG;
		aSlot->flag |= XS_STEP_OVER_FLAG;
		aSlot = aSlot->next;
	}
}

void fxStepInside(txMachine* the)
{
	txSlot* aSlot = the->frame;
	while (aSlot) {
		aSlot->flag |= XS_STEP_INTO_FLAG | XS_STEP_OVER_FLAG;
		aSlot = aSlot->next;
	}
}

void fxStepOutside(txMachine* the)
{
	txSlot* aSlot = the->frame;
	if (aSlot) {
		aSlot->flag &= ~(XS_STEP_INTO_FLAG | XS_STEP_OVER_FLAG);
		aSlot = aSlot->next;
		while (aSlot) {
			aSlot->flag &= ~XS_STEP_INTO_FLAG;
			aSlot->flag |= XS_STEP_OVER_FLAG;
			aSlot = aSlot->next;
		}
	}
}

void fxToggle(txMachine* the, txString theAddress)
{
	txSlot* aProperty;
/*
	txSlot* aHeap;
	txSlot* aLimit;
*/

	if (theAddress) {
		aProperty = fxParseAddress(the, theAddress);;
		aProperty->flag ^= XS_DEBUG_FLAG;
		/*if ((the->stack <= aProperty) && (aProperty < the->stackTop)) {
			aProperty->flag ^= XS_DEBUG_FLAG;
			return;
		}
		aHeap = the->firstHeap;
		while (aHeap) {
			aLimit = aHeap->value.reference;
			if ((aHeap < aProperty) && (aProperty < aLimit)) {
				aProperty->flag ^= XS_DEBUG_FLAG;
				return;
			}
			aHeap = aHeap->next;
		}
		if (the->sharedMachine) {
			aHeap = the->sharedMachine->firstHeap;
			while (aHeap) {
				aLimit = aHeap->value.reference;
				if ((aHeap < aProperty) && (aProperty < aLimit)) {
					aProperty->flag ^= XS_DEBUG_FLAG;
					return;
				}
				aHeap = aHeap->next;
			}
		}*/
	}
	fxEcho(the, "<break># xsbug: no such address!</break>");
}

#endif

void fxReport(txMachine* the, txString theFormat, ...)
{
	c_va_list arguments;

	c_va_start(arguments, theFormat);
	fxVReport(the, theFormat, arguments);
	c_va_end(arguments);
}

void fxReportException(txMachine* the, txString thePath, txInteger theLine, txString theFormat, ...)
{
	c_va_list arguments;

	c_va_start(arguments, theFormat);
	fxVReportException(the, thePath, theLine, theFormat, arguments);
	c_va_end(arguments);
}

void fxReportError(txMachine* the, txString thePath, txInteger theLine, txString theFormat, ...)
{
	c_va_list arguments;

	c_va_start(arguments, theFormat);
	fxVReportError(the, thePath, theLine, theFormat, arguments);
	c_va_end(arguments);
}

void fxReportWarning(txMachine* the, txString thePath, txInteger theLine, txString theFormat, ...)
{
	c_va_list arguments;

	c_va_start(arguments, theFormat);
	fxVReportWarning(the, thePath, theLine, theFormat, arguments);
	c_va_end(arguments);
}

void fxVReport(void* console, txString theFormat, c_va_list theArguments)
{
#if (__FSK_LAYER__ && SUPPORT_INSTRUMENTATION) || mxDebug
	txMachine* the = console;
#endif
#if __FSK_LAYER__ && SUPPORT_INSTRUMENTATION
	if (FskInstrumentedItemHasListenersForLevel(the, kFskInstrumentationLevelNormal)) {
		void *data[2];
		data[0] = theFormat;
		data[1] = (void *)&theArguments;
		FskInstrumentedItemSendMessageForLevel(the, kFskXSInstrTrace, data, kFskInstrumentationLevelNormal);
	}
#endif
#ifdef mxDebug
	if (fxIsConnected(the)) {
		fxEchoStart(the);
		fxEcho(the, "<log>");
		fxEchoFormat(the, theFormat, theArguments);
		fxEcho(the, "</log>");
		fxEchoStop(the);
		fxSend(the);
		fxReceive(the);
	}
#endif
#ifndef mxNoConsole
	vfprintf(stdout, theFormat, theArguments);
#endif
}

void fxVReportException(void* console, txString thePath, txInteger theLine, txString theFormat, c_va_list theArguments)
{
#if (__FSK_LAYER__ && SUPPORT_INSTRUMENTATION) || mxDebug
	txMachine* the = console;
#endif
#if __FSK_LAYER__ && SUPPORT_INSTRUMENTATION
	if (FskInstrumentedItemHasListenersForLevel(the, kFskInstrumentationLevelMinimal)) {
		void *data[4];
		data[0] = theFormat;
		data[1] = (void *)&theArguments;
		data[2] = thePath;
		data[3] = (void *)theLine;
		FskInstrumentedItemSendMessageForLevel(the, kFskXSInstrException, data, kFskInstrumentationLevelMinimal);
	}
#endif
#ifdef mxDebug
	if (fxIsConnected(the)) {
		fxEchoStart(the);
		fxEcho(the, "<log");
		fxEchoPathLine(the, thePath, theLine);
		fxEcho(the, "># Exception: ");
		fxEchoFormat(the, theFormat, theArguments);
		fxEcho(the, "!\n</log>");
		fxEchoStop(the);
		fxSend(the);
		fxReceive(the);
	}
#endif
#ifndef mxNoConsole
	if (thePath && theLine)
#if mxWindows
		fprintf(stdout, "%s(%d): exception: ", thePath, (int)theLine);
#else
		fprintf(stdout, "%s:%d: exception: ", thePath, (int)theLine);
#endif
	else
		fprintf(stdout, "# exception: ");
	vfprintf(stdout, theFormat, theArguments);
	fprintf(stdout, "!\n");
#endif
}

void fxVReportError(void* console, txString thePath, txInteger theLine, txString theFormat, c_va_list theArguments)
{
#if (__FSK_LAYER__ && SUPPORT_INSTRUMENTATION) || mxDebug
	txMachine* the = console;
#endif
#if __FSK_LAYER__ && SUPPORT_INSTRUMENTATION
	if (FskInstrumentedItemHasListenersForLevel(the, kFskInstrumentationLevelMinimal)) {
		void *data[4];
		data[0] = theFormat;
		data[1] = (void *)&theArguments;
		data[2] = thePath;
		data[3] = (void *)theLine;
		FskInstrumentedItemSendMessageForLevel(the, kFskXSInstrReportError, data, kFskInstrumentationLevelMinimal);
	}
#endif
#ifdef mxDebug
	if (fxIsConnected(the)) {
		fxEchoStart(the);
		fxEcho(the, "<log");
		fxEchoPathLine(the, thePath, theLine);
		fxEcho(the, "># Error: ");
		fxEchoFormat(the, theFormat, theArguments);
		fxEcho(the, "!\n</log>");
		fxEchoStop(the);
		fxSend(the);
		fxReceive(the);
	}
#endif
#ifndef mxNoConsole
	if (thePath && theLine)
#if mxWindows
		fprintf(stdout, "%s(%d): error: ", thePath, (int)theLine);
#else
		fprintf(stdout, "%s:%d: error: ", thePath, (int)theLine);
#endif
	else
		fprintf(stdout, "# error: ");
	vfprintf(stdout, theFormat, theArguments);
	fprintf(stdout, "!\n");
#endif
}

void fxVReportWarning(void* console, txString thePath, txInteger theLine, txString theFormat, c_va_list theArguments)
{
#if (__FSK_LAYER__ && SUPPORT_INSTRUMENTATION) || mxDebug
	txMachine* the = console;
#endif
#if __FSK_LAYER__ && SUPPORT_INSTRUMENTATION
	if (FskInstrumentedItemHasListenersForLevel(the, kFskInstrumentationLevelMinimal)) {
		void *data[4];
		data[0] = theFormat;
		data[1] = &theArguments;
		data[2] = thePath;
		data[3] = (void *)theLine;
		FskInstrumentedItemSendMessageForLevel(the, kFskXSInstrReportWarning, data, kFskInstrumentationLevelMinimal);
	}
#endif
#ifdef mxDebug
	if (fxIsConnected(the)) {
		fxEchoStart(the);
		fxEcho(the, "<log");
		fxEchoPathLine(the, thePath, theLine);
		fxEcho(the, "># Warning: ");
		fxEchoFormat(the, theFormat, theArguments);
		fxEcho(the, "!\n</log>");
		fxEchoStop(the);
		fxSend(the);
		fxReceive(the);
	}
#endif
#ifndef mxNoConsole
	if (thePath && theLine)
#if mxWindows
		fprintf(stdout, "%s(%d): warning: ", thePath, (int)theLine);
#else
		fprintf(stdout, "%s:%d: warning: ", thePath, (int)theLine);
#endif
	else
		fprintf(stdout, "# warning: ");
	vfprintf(stdout, theFormat, theArguments);
	fprintf(stdout, "!\n");
#endif
}
