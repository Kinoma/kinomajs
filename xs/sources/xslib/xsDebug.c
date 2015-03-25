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

#ifdef mxDebug

static void fxAbort(txMachine* the);
static void fxDebugLoopV(txMachine* the, txString theFormat, c_va_list theArguments);
static txBoolean fxDebugLoopTest(txString* theBuffer, txString theName);
static txString fxDebugLoopValue(txString* theBuffer, txString theName);
static void fxDebugThrowV(txMachine* the, txString theFormat, c_va_list theArguments);
static void fxEcho(txMachine* the, txString theString);
static void fxEchoAddress(txMachine* the, txSlot* theSlot);
static void fxEchoCharacter(txMachine* the, char theCharacter);
static void fxEchoFormat(txMachine* the, txString theFormat, c_va_list theArguments);
static void fxEchoFrame(txMachine* the, txSlot* theFrame, txSlot* theList);
static void fxEchoGrammar(txMachine* the, txSlot* theRoot, txSlot* theGrammar, txSlot* theList);
static void fxEchoHost(txMachine* the, txSlot* theInstance, txSlot* theList);
static void fxEchoID(txMachine* the, txID theID);
static void fxEchoInteger(txMachine* the, txInteger theInteger);
static void fxEchoInstance(txMachine* the, txSlot* theInstance, txSlot* theList);
static void fxEchoNode(txMachine* the, txSlot* theRoot, txSlot* theNode, txSlot* theList);
static void fxEchoNumber(txMachine* the, txNumber theNumber);
static void fxEchoPathLine(txMachine* the, txString thePath, txInteger theLine);
static void fxEchoProperty(txMachine* the, txSlot* theProperty, txSlot* theList, txString thePrefix, txInteger theIndex, txString theSuffix);
static void fxEchoStart(txMachine* the);
static void fxEchoStop(txMachine* the);
static void fxEchoString(txMachine* the, txString theString);
static void fxGo(txMachine* the);
static void fxInsertFile(txMachine* the, txSlot* thSymbol);
static void fxListBreakpoints(txMachine* the);
static void fxListFiles(txMachine* the);
static void fxListFrames(txMachine* the);
static void fxListGlobal(txMachine* the);
static void fxListGrammar(txMachine* the);
static void fxListLocal(txMachine* the);
static txSlot* fxParseAddress(txMachine* the, txString theAddress);
static void fxSelect(txMachine* the, txString theAddress);
static void fxSortProperties(txMachine* the, txSlot** theSorter, txIndex lb, txIndex ub);
static int fxSortPropertiesCompare(txMachine* the, txSlot* i, txSlot* j);
static void fxSortPropertiesSwap(txMachine* the, txSlot** theSorter, txIndex i, txIndex j);
static void fxStep(txMachine* the);
static void fxStepInside(txMachine* the);
static void fxStepOutside(txMachine* the);
static void fxToggle(txMachine* the, txString theAddress);

void fxAbort(txMachine* the)
{
	fxWriteBreakpoints(the);
	fxDisconnect(the);
	c_exit(0);
}

void fxCheck(txMachine* the, txString thePath, txInteger theLine)
{
#if mxWindows
	fprintf(stderr, "%s(%ld): fatal!\n", thePath, theLine);
	DebugBreak();
#else
	fxDebugLoop(the, "%s:%ld: fatal!\n", thePath, theLine);
#endif
	c_exit(0);
}

void fxClearAllBreakpoints(txMachine* the)
{
	mxBreakpoints.value.list.first = C_NULL;
}

void fxClearBreakpoint(txMachine* the, txString thePath, txString theLine)
{
	txSlot* aSymbol;
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
	aSymbol = fxFindSymbol(the, thePath);
	if (!aSymbol)
		return;
	aBreakpointAddress = &(mxBreakpoints.value.list.first);
	while ((aBreakpoint = *aBreakpointAddress)) {
		if ((aBreakpoint->value.reference == aSymbol) && (aBreakpoint->ID == (txID)aLine)) {
			*aBreakpointAddress = aBreakpoint->next;
			break;
		}
		aBreakpointAddress = &(aBreakpoint->next);
	}
}

void fxDebug(txMachine* the, txError theError, txString theFormat, ...)
{
	c_va_list arguments;
	txSlot* aSlot;
	txString aPath = C_NULL;
	txID aLine = 0;

	c_va_start(arguments, theFormat);
	if (fxIsConnected(the) && (theError != XS_NO_ERROR))
		fxDebugLoopV(the, theFormat, arguments);
	else {
		if ((aSlot = the->frame - 1) && (aSlot->next)) {
			if (the->frame->flag & XS_C_FLAG)
				aPath = (txString)aSlot->next;
			else
				aPath = aSlot->next->value.symbol.string;
			aLine = aSlot->ID;
		}
		if (theError != XS_NO_ERROR)
			fxVReportError(the, aPath, aLine, theFormat, arguments);
		else
			fxVReportWarning(the, aPath, aLine, theFormat, arguments);
	}
	c_va_end(arguments);
	if (theError != XS_NO_ERROR)
		fxThrowError(the, theError);
}

void fxDebugID(txMachine* the, txError theError, txString theFormat, txID theID)
{
	char aBuffer[16];
	txString aString = aBuffer;
	txSlot* aSymbol;

	aSymbol = fxGetSymbol(the, theID);
	if (aSymbol)
		aString = aSymbol->value.symbol.string;
	else if (theID != XS_NO_ID)
		fxIntegerToString(theID, aBuffer, sizeof(aBuffer));
	else
		c_strcpy(aBuffer, "?");
	fxDebug(the, theError, theFormat, aString);
}

void fxDebugLine(txMachine* the)
{
	txSlot* aSlot;
	txSlot* aBreakpoint;
	txSlot* aSymbol;

	if ((the->frame) && (aSlot = the->frame - 1) && (aSlot->next)) {
		aBreakpoint = C_NULL;
		if ((aSymbol = aSlot->next)) {
			aBreakpoint = mxBreakpoints.value.list.first;
			while (aBreakpoint) {
				if (aBreakpoint->value.reference == aSymbol)
					if (aBreakpoint->ID == aSlot->ID)
						break;
				aBreakpoint = aBreakpoint->next;
			}
		}
		if (aBreakpoint)
			fxDebugLoop(the, "breakpoint");
#ifdef mxSDK
		else if ((the->frame->flag & XS_STEP_OVER_FLAG) && (the->frame->flag & XS_SANDBOX_FLAG))
#else
		else if ((the->frame->flag & XS_STEP_OVER_FLAG))
#endif
			fxDebugLoop(the, "step");
	}
}

void fxDebugLoop(txMachine* the, txString theFormat, ...)
{
	c_va_list arguments;

	c_va_start(arguments, theFormat);
	fxDebugLoopV(the, theFormat, arguments);
	c_va_end(arguments);
}

void fxDebugLoopV(txMachine* the, txString theFormat, c_va_list theArguments)
{
	txSlot* aSlot;
	txString p;
	txString q;

	if (!fxIsConnected(the))
		return;
	fxNewInstance(the);

	fxEchoStart(the);
	aSlot = the->frame;
	do {
		aSlot->flag &= ~XS_DEBUG_FLAG;
		aSlot = aSlot->next;
	} while (aSlot);
	the->frame->flag |= XS_DEBUG_FLAG;
	fxListFrames(the);
	fxListLocal(the);
	fxListGlobal(the);
	fxListGrammar(the);
	fxListFiles(the);

	fxEcho(the, "<break");
#ifdef mxSDK
	if (the->frame->flag & XS_SANDBOX_FLAG) {
#endif
	if ((aSlot = the->frame - 1) && (aSlot->next)) {
		if (the->frame->flag & XS_C_FLAG)
			fxEchoPathLine(the, (txString)aSlot->next, aSlot->ID);
		else
			fxEchoPathLine(the, aSlot->next->value.symbol.string, aSlot->ID);
	}
#ifdef mxSDK
	}
#endif
	fxEcho(the, "># Break: ");
	fxEchoFormat(the, theFormat, theArguments);
	fxEcho(the, "!\n</break>\n");

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
		else if (fxDebugLoopTest(&p, "go")) {
			fxGo(the);
			goto bail;
		}
		else if (fxDebugLoopTest(&p, "step")) {
			fxStep(the);
			goto bail;
		}
		else if (fxDebugLoopTest(&p, "step-inside")) {
			fxStepInside(the);
			goto bail;
		}
		else if (fxDebugLoopTest(&p, "step-outside")) {
			fxStepOutside(the);
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
			fxListGrammar(the);
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
	if (((*theBuffer)[aLength] == ' ') || ((*theBuffer)[aLength] == '/')) {
		if (c_strncmp(*theBuffer, theName, aLength) == 0) {
			aResult = 1;
			*theBuffer += aLength;
		}
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

void fxDebugThrow(txMachine* the, txString theFormat, ...)
{
	c_va_list arguments;

	c_va_start(arguments, theFormat);

	if (fxIsConnected(the)) {
        if (the->breakOnExceptionFlag)
            fxDebugLoopV(the, theFormat, arguments);
        else
            fxDebugThrowV(the, theFormat, arguments);
    }
    else {
        char *path = NULL;
        int line = 0;

#ifdef mxSDK
        if (the->frame->flag & XS_SANDBOX_FLAG) {
#endif
            txSlot* aSlot;
            if ((aSlot = the->frame - 1) && (aSlot->next)) {
                if (the->frame->flag & XS_C_FLAG)
                    path = (txString)aSlot->next, line = aSlot->ID;
                else
                    path = aSlot->next->value.symbol.string, line = aSlot->ID;
            }
#ifdef mxSDK
        }
#endif
        fxVReport(the, path, line, theFormat, arguments);
        fxVReport(the, NULL, 0, "\n", arguments);
    }

    c_va_end(arguments);
}

void fxDebugThrowV(txMachine* the, txString theFormat, c_va_list theArguments)
{
	txSlot* aSlot;
	fxEchoStart(the);
	fxEcho(the, "<log");
#ifdef mxSDK
	if (the->frame->flag & XS_SANDBOX_FLAG) {
#endif
	if ((aSlot = the->frame - 1) && (aSlot->next)) {
		if (the->frame->flag & XS_C_FLAG)
			fxEchoPathLine(the, (txString)aSlot->next, aSlot->ID);
		else
			fxEchoPathLine(the, aSlot->next->value.symbol.string, aSlot->ID);
	}
#ifdef mxSDK
	}
#endif
	fxEcho(the, "># Exception: ");
	fxEchoFormat(the, theFormat, theArguments);
	fxEcho(the, "!\n</log>");
	fxEchoStop(the);
	fxSend(the);
	fxReceive(the);
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
	txSlot* aSlot;
	txSlot* aNextSlot;

	fxEcho(the, "<local");
	fxEcho(the, " name=\"");
#ifndef mxSDK
	if (theFrame->flag & XS_SANDBOX_FLAG)
		fxEcho(the, "$ ");
#endif
	if (theFrame->flag & XS_STRICT_FLAG)
		fxEcho(the, "! ");
	fxEchoID(the, theFrame->ID);
	fxEcho(the, "\"");
	fxEchoAddress(the, theFrame);
	if ((aSlot = theFrame - 1) && (aSlot->next)) {
		if (theFrame->flag & XS_C_FLAG)
			fxEchoPathLine(the, (txString)aSlot->next, aSlot->ID);
		else
			fxEchoPathLine(the, aSlot->next->value.symbol.string, aSlot->ID);
	}
	fxEcho(the, " flags=\"-\">");
#ifndef mxSDK
	aFrame = the->frame;
	aSlot = C_NULL;
	if (aFrame == theFrame)
		aSlot = the->scope;
	else {
		aFrame = the->frame;
		while (aFrame->next != theFrame)
			aFrame = aFrame->next;
		if (aFrame)
			aSlot = aFrame->value.frame.scope;
	}
	if (aSlot) {
		if (theFrame->flag & XS_C_FLAG)
			aSlot = aSlot->next;
		anIndex = 0;
		while (aSlot) {
			fxEchoProperty(the, aSlot, theList, "(scope ", anIndex, ")");
			aSlot = aSlot->next;
			anIndex++;
		}
	}
#endif
	fxEchoProperty(the, theFrame + 1, theList, "(return)", -1, C_NULL);
	fxEchoProperty(the, theFrame + 3, theList, "this", -1, C_NULL);
	if (theFrame->flag & XS_C_FLAG) {
		fxEchoProperty(the, theFrame + 4, theList, "argc", -1, C_NULL);
		aCount = (theFrame + 4)->value.integer;
		for (anIndex = 0; anIndex < aCount; anIndex++) {
			fxEchoProperty(the, theFrame + 4 + aCount - anIndex, theList, "arg(", anIndex, ")");
		}
		fxEchoProperty(the, theFrame - 1, theList, "varc", -1, C_NULL);
		aCount = (theFrame- 1)->value.integer;
		for (anIndex = 0; anIndex < aCount; anIndex++) {
			fxEchoProperty(the, theFrame - 2 - anIndex, theList, "var(", anIndex, ")");
		}
	}
	else {
		fxEchoProperty(the, theFrame + 4, theList, "arguments.length", -1, C_NULL);
		aCount = (theFrame + 4)->value.integer;
		aNextSlot = (theFrame + 2)->next;
		if (aNextSlot && (aNextSlot->ID == the->argumentsID))
			aNextSlot = aNextSlot->next;
		for (anIndex = 0; anIndex < aCount; anIndex++) {
			aSlot = theFrame + 4 + aCount - anIndex;
			if (aSlot->ID != XS_NO_ID) {
				fxEchoProperty(the, aSlot, theList, C_NULL, -1, C_NULL);
				aNextSlot = aSlot->next;
			}
			else
				fxEchoProperty(the, aSlot, theList, "arguments[", anIndex, "]");
		}
		while (aNextSlot) {
			fxEchoProperty(the, aNextSlot, theList, C_NULL, -1, C_NULL);
			aNextSlot = aNextSlot->next;
		}
	}
	fxEcho(the, "</local>");
}

void fxEchoGrammar(txMachine* the, txSlot* theRoot, txSlot* theGrammar, txSlot* theList)
{
	txSlot aSymbol;
	txSlot* aNode;

	if (theList->value.list.first == theList->value.list.last) {
		aNode = theRoot->next->next;
		while (aNode) {
			fxEchoNode(the, theRoot, aNode, theList);
			aNode = aNode->next;
		}
	}

	aSymbol.next = C_NULL;
	aSymbol.value.symbol.string = C_NULL;
	aSymbol.value.symbol.sum = 0;

	theGrammar->flag |= XS_LEVEL_FLAG;
	theGrammar->value.instance.garbage = theList->value.list.last;
	theList->value.list.last->next = &aSymbol;
	theList->value.list.last = &aSymbol;

	aNode = theGrammar->next;
	while (aNode) {
		fxEchoNode(the, theRoot, aNode, theList);
		aNode = aNode->next;
	}

	theList->value.list.last = theGrammar->value.instance.garbage;
	theList->value.list.last->next = C_NULL;
	theGrammar->value.instance.garbage = C_NULL;
	theGrammar->flag &= ~XS_LEVEL_FLAG;
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
					if ((aParentProperty->ID != the->sandboxID) && (aParentProperty->kind == XS_ACCESSOR_KIND) && (aParentProperty->value.accessor.getter)) {
						txSlot* anInstanceProperty = fxGetProperty(the, anInstance, aParentProperty->ID);
						if (!anInstanceProperty) {
							txSlot* aFunction = aParentProperty->value.accessor.getter;
							if (mxIsFunction(aFunction)) {
								mxInitSlot(--the->stack, XS_INTEGER_KIND);
								the->stack->value.integer = 0;
								/* THIS */
								mxInitSlot(--the->stack, XS_REFERENCE_KIND);
								the->stack->value.reference = theInstance;
								/* FUNCTION */
								mxInitSlot(--the->stack, XS_REFERENCE_KIND);
								the->stack->value.reference = aFunction;
								/* RESULT */
								mxZeroSlot(--the->stack);
								fxRunID(the, aParentProperty->ID);
								anInstanceProperty = fxSetProperty(the, anInstance, aParentProperty->ID, C_NULL);
								anInstanceProperty->kind = the->stack->kind;
								anInstanceProperty->value = the->stack->value;
								the->stack++;
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

void fxEchoID(txMachine* the, txID theID)
{
	txSlot* aSymbol;
	char aBuffer[32];

	if (theID != XS_NO_ID) {
		aSymbol = fxGetSymbol(the, theID);
		if (aSymbol)
			fxEchoString(the, aSymbol->value.symbol.string);
		else {
			fxIntegerToString(theID, aBuffer, sizeof(aBuffer));
			fxEcho(the, aBuffer);
		}
	}
	else
		fxEcho(the, "?");
}

void fxEchoInteger(txMachine* the, txInteger theInteger)
{
	char aBuffer[256];

	fxIntegerToString(theInteger, aBuffer, sizeof(aBuffer));
	fxEcho(the, aBuffer);
}

void fxEchoInstance(txMachine* the, txSlot* theInstance, txSlot* theList)
{
	char aName[6] = "(...)";
	txSlot aSymbol;
	txSlot* aParent;
	txSlot* aProperty;
	txSlot* aSlot;
	txInteger aCount;
	txInteger anIndex;

	if (theInstance->flag & XS_SANDBOX_FLAG)
		theInstance = theInstance->value.instance.prototype;

	aSymbol.next = C_NULL;
	aSymbol.value.symbol.string = C_NULL;
	aSymbol.value.symbol.sum = 0;

	theInstance->flag |= XS_LEVEL_FLAG;
	theInstance->value.instance.garbage = theList->value.list.last;
	theList->value.list.last->next = &aSymbol;
	theList->value.list.last = &aSymbol;

	aParent = fxGetParent(the, theInstance);
	if (aParent) {
		fxEcho(the, "<property name=\"(...)\"");
		theList->value.list.last->value.symbol.string = aName;
		if (aParent->flag & XS_LEVEL_FLAG) {
			txSlot* aFirst = theList->value.list.first;
			txSlot* aLast = aParent->value.instance.garbage->next;
			txBoolean aDot = 0;
			fxEcho(the, " value=\"");
			while (aFirst != aLast) {
				if (aFirst->value.symbol.string) {
					if (aDot)
						fxEcho(the, ".");
					fxEchoString(the, aFirst->value.symbol.string);
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
#ifndef mxSDK
		fxEcho(the, " s");
		if ((theInstance->ID < 0) && (aParent->flag & XS_SHARED_FLAG))
			fxEcho(the, "*");
#endif
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
		theList->value.list.last->value.symbol.string = C_NULL;
	}

	aProperty = theInstance->next;
	if (theInstance->flag & XS_VALUE_FLAG) {
		switch (aProperty->kind) {
		case XS_CALLBACK_KIND:
			fxEchoProperty(the, aProperty, theList, "(C function)", -1, C_NULL);
			aProperty = aProperty->next;
#ifndef mxSDK
			fxEchoProperty(the, aProperty, theList, "(C closure)", -1, C_NULL);
			aProperty = aProperty->next;
#endif
			fxEchoProperty(the, aProperty, theList, "(prototype)", -1, C_NULL);
			aProperty = aProperty->next;
			break;
		case XS_CODE_KIND:
			fxEchoProperty(the, aProperty, theList, "(function)", -1, C_NULL);
			aProperty = aProperty->next;
#ifndef mxSDK
			fxEchoProperty(the, aProperty, theList, "(closure)", -1, C_NULL);
			aProperty = aProperty->next;
#endif
			fxEchoProperty(the, aProperty, theList, "(prototype)", -1, C_NULL);
			aProperty = aProperty->next;
			break;
		case XS_ARRAY_KIND:
			fxEchoProperty(the, aProperty, theList, "(array)", -1, C_NULL);
			aSlot = aProperty->value.array.address;
			aCount = aProperty->value.array.length;
			for (anIndex = 0; anIndex < aCount; anIndex++) {
				if (aSlot->ID)
					fxEchoProperty(the, aSlot, theList, "[", anIndex, "]");
				aSlot++;
			}
			aProperty = aProperty->next;
			break;
		case XS_STRING_KIND:
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
			aProperty = aProperty->next;
			break;
		}
	}
	aCount = 0;
	aSlot = aProperty;
	while (aSlot) {
		if (aSlot->ID >= -1)
			fxEchoProperty(the, aSlot, theList, C_NULL, -1, C_NULL);
		else
			aCount++;
		aSlot = aSlot->next;
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
		}
	}
	theList->value.list.last = theInstance->value.instance.garbage;
	theList->value.list.last->next = C_NULL;
	theInstance->value.instance.garbage = C_NULL;
	theInstance->flag &= ~XS_LEVEL_FLAG;
}

void fxEchoNode(txMachine* the, txSlot* theRoot, txSlot* theNode, txSlot* theList)
{
	txRuleData* aRuleData;
	txSlot* aGrammar;
	txSlot* aSlot;
	txString aPrefix;

	switch (theNode->kind) {
	case XS_NODE_KIND:
		aRuleData = C_NULL;
		aGrammar = theNode->value.node.link;
		break;
	case XS_PREFIX_KIND:
		aRuleData = C_NULL;
		aGrammar = C_NULL;
		break;
	case XS_JUMP_RULE:
		aRuleData = theNode->value.rule.data;
		aGrammar = aRuleData->link;
		break;
	case XS_EMBED_RULE:
	case XS_REFER_RULE:
	case XS_REPEAT_RULE:
		return;
	default:
		aRuleData = theNode->value.rule.data;
		aGrammar = C_NULL;
		break;
	}

	fxEcho(the, "<node");

	fxEcho(the, " flags=\"");
	if (theNode->flag & XS_DEBUG_FLAG)
		fxEcho(the, "-");
	else if (aGrammar && !(aGrammar->flag & XS_LEVEL_FLAG))
		fxEcho(the, "+");
	else
		fxEcho(the, " ");
	if (theNode->kind == XS_PREFIX_KIND) {
		aSlot = fxGetSymbol(the, theNode->value.node.part.namespaceID);
		if (aSlot)
			fxEcho(the, aSlot->value.symbol.string);
		else
			fxEcho(the, "?");
	}
	else {
		if (aRuleData) {
			txInteger aCount = aRuleData->count;
			txID* anID = aRuleData->IDs;
			aSlot = fxGetSymbol(the, *anID);
			if (aSlot)
				fxEcho(the, aSlot->value.symbol.string);
			aCount--;
			anID++;
			while (aCount > 0) {
				aSlot = fxGetSymbol(the, *anID);
				if (aSlot) {
					fxEcho(the, ".");
					fxEcho(the, aSlot->value.symbol.string);
				}
				else if (*anID == XS_NO_ID)
					fxEcho(the, "[]");
				aCount--;
				anID++;
			}
		}
	}
	fxEcho(the, "\"");

	fxEcho(the, " name=\"");
	if (theNode->kind == XS_PREFIX_KIND) {
		fxEcho(the, "xmlns");
		aPrefix = theNode->value.prefix.string;
		if (aPrefix) {
			fxEcho(the, ":");
			fxEcho(the, aPrefix);
		}
	}
	else {
		switch (theNode->kind) {
		case XS_ATTRIBUTE_RULE:
			fxEcho(the, "@");
			break;
		case XS_DATA_RULE:
			break;
		case XS_PI_RULE:
			fxEcho(the, "?");
			break;
		case XS_ERROR_RULE:
			fxEcho(the, "! ");
			break;
		default:
			if (theList->value.list.first == theList->value.list.last)
				fxEcho(the, "/");
			break;
		}
		aPrefix = fxSearchPrefix(the, theRoot, theNode->value.node.part.namespaceID);
		if (aPrefix) {
			fxEcho(the, aPrefix);
			fxEcho(the, ":");
		}
		aSlot = fxGetSymbol(the, theNode->value.node.part.nameID);
		if (aSlot)
			fxEcho(the, aSlot->value.symbol.string);
		else
			fxEcho(the, "?");
	}
	fxEcho(the, "\"");

	if (aRuleData) {
		aSlot = fxGetSymbol(the, aRuleData->pathID);
		if (aSlot)
			fxEchoPathLine(the, aSlot->value.symbol.string, aRuleData->line);
	}

	if (aGrammar && !(aGrammar->flag & XS_LEVEL_FLAG)) {
		fxEchoAddress(the, theNode);
		if (theNode->flag & XS_DEBUG_FLAG) {
			fxEcho(the, ">");
			fxEchoGrammar(the, theRoot, aGrammar, theList);
			fxEcho(the, "</node>");
		}
		else
			fxEcho(the, "/>");
	}
	else
		fxEcho(the, "/>");
}

void fxEchoNumber(txMachine* the, txNumber theNumber)
{
	char aBuffer[256];

	fxNumberToString(the, theNumber, aBuffer, sizeof(aBuffer), 0, 0);
	fxEcho(the, aBuffer);
}

void fxEchoPathLine(txMachine* the, txString thePath, txInteger theLine)
{
	if (thePath && theLine) {
		fxEcho(the, " path=\"");
		fxEchoString(the, mxCleanPath(thePath));
		fxEcho(the, "\"");
		fxEcho(the, " line=\"");
		fxEchoInteger(the, theLine);
		fxEcho(the, "\"");
	}
}

void fxEchoProperty(txMachine* the, txSlot* theProperty, txSlot* theList, txString thePrefix, txInteger theIndex, txString theSuffix)
{
	char aName[256];
	txSlot* anInstance;

#ifdef mxSDK
	if (theProperty->flag & XS_DONT_SCRIPT_FLAG)
		return;
#endif

	if (mxIsReference(theProperty))
		anInstance = fxGetInstance(the, theProperty);
	else if (theProperty->kind == XS_CLOSURE_KIND)
		anInstance = theProperty->value.closure.reference;
	else
		anInstance = C_NULL;

	c_strcpy(aName, "");
	if (thePrefix) {
		c_strcat(aName, thePrefix);
		if (theSuffix) {
			int aLength = c_strlen(aName);
			fxIntegerToString(theIndex, aName + aLength, sizeof(aName) - aLength);
			c_strcat(aName, theSuffix);
		}
	}
	else {
		if (theProperty->ID != XS_NO_ID) {
			txSlot* aSymbol = fxGetSymbol(the, theProperty->ID);
			if (aSymbol) {
				c_strncat(aName, aSymbol->value.symbol.string, 255 - c_strlen(aName));
				aName[255] = 0;
			}
			else
				fxIntegerToString(theProperty->ID, aName, sizeof(aName));
		}
		else
			c_strcpy(aName, "?");
	}
#ifdef mxSDK
	if (c_strncmp(aName, "__xs__", 6) == 0)
		return;
#endif

	fxEcho(the, "<property");

	fxEcho(the, " flags=\"");
	if (theProperty->flag & XS_DEBUG_FLAG)
		fxEcho(the, "-");
	else if (theProperty->kind == XS_ACCESSOR_KIND)
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
#ifndef mxSDK
	if (theProperty->flag & XS_DONT_SCRIPT_FLAG)
		fxEcho(the, " S");
	else if (theProperty->flag & XS_SANDBOX_FLAG)
		fxEcho(the, " $");
	else
		fxEcho(the, " s");
	if ((theProperty->kind == XS_REFERENCE_KIND) && (anInstance->flag & XS_SHARED_FLAG))
		fxEcho(the, ".");
#endif
	fxEcho(the, "\"");

	fxEcho(the, " name=\"");
	fxEchoString(the, aName);
	fxEcho(the, "\"");

	theList->value.list.last->value.symbol.string = aName;

	switch (theProperty->kind) {
	case XS_REFERENCE_KIND:
	case XS_ALIAS_KIND:
	case XS_CLOSURE_KIND:
        if (anInstance) {
			if (anInstance->flag & XS_LEVEL_FLAG) {
				txSlot* aFirst = theList->value.list.first;
				txSlot* aLast = anInstance->value.instance.garbage->next;
				txBoolean aDot = 0;
				fxEcho(the, " value=\"");
				while (aFirst && (aFirst != aLast)) {
					if (aFirst->value.symbol.string) {
						if (aDot)
							fxEcho(the, ".");
						fxEchoString(the, aFirst->value.symbol.string);
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

	case XS_UNDEFINED_KIND:
		fxEcho(the, " value=\"undefined\"/>");
		break;
	case XS_NULL_KIND:
		fxEcho(the, " value=\"null\"/>");
		break;

	case XS_CALLBACK_KIND:
		fxEcho(the, " value=\"");
		fxEchoInteger(the, theProperty->value.callback.length);
		fxEcho(the, " arguments\"/>");
		break;
	case XS_CODE_KIND:
		fxEcho(the, " value=\"");
		fxEchoInteger(the, *(theProperty->value.code + 4));
		fxEcho(the, " arguments\"/>");
		break;
	case XS_ARRAY_KIND:
		fxEcho(the, " value=\"");
		fxEchoInteger(the, theProperty->value.array.length);
		fxEcho(the, " items\"/>");
		break;
	case XS_STRING_KIND:
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


	case XS_NODE_KIND:
		fxEcho(the, " value=\"node\"/>");
		break;
	case XS_ATTRIBUTE_RULE:
		fxEcho(the, " value=\"attribute rule\"/>");
		break;
	case XS_DATA_RULE:
		fxEcho(the, " value=\"data rule\"/>");
		break;
	case XS_PI_RULE:
		fxEcho(the, " value=\"pi rule\"/>");
		break;
	case XS_EMBED_RULE:
		fxEcho(the, " value=\"embed rule\"/>");
		break;
	case XS_JUMP_RULE:
		fxEcho(the, " value=\"jump rule\"/>");
		break;
	case XS_REFER_RULE:
		fxEcho(the, " value=\"refer rule\"/>");
		break;
	case XS_REPEAT_RULE:
		fxEcho(the, " value=\"repeat rule\"/>");
		break;
	case XS_ATTRIBUTE_PATTERN:
		fxEcho(the, " value=\"attribute pattern\"/>");
		break;
	case XS_DATA_PATTERN:
		fxEcho(the, " value=\"data pattern\"/>");
		break;
	case XS_PI_PATTERN:
		fxEcho(the, " value=\"pi pattern\"/>");
		break;
	case XS_EMBED_PATTERN:
		fxEcho(the, " value=\"embed pattern\"/>");
		break;
	case XS_JUMP_PATTERN:
		fxEcho(the, " value=\"jump pattern\"/>");
		break;
	case XS_REFER_PATTERN:
		fxEcho(the, " value=\"refer pattern\"/>");
		break;
	case XS_REPEAT_PATTERN:
		fxEcho(the, " value=\"repeat pattern\"/>");
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
		c_strcpy(thePath, theBreakpoint->value.reference->value.symbol.string);
		fxIntegerToString(theBreakpoint->ID, theLine, 32);
	}
	return theBreakpoint;
}

void fxInsertFile(txMachine* the, txSlot* theSymbol)
{
	txSlot* aFile = mxFiles.value.list.first;
	while (aFile) {
		if (aFile->value.reference == theSymbol)
			return;
		aFile =	aFile->next;
	}
	aFile = fxNewSlot(the);
	aFile->next = mxFiles.value.list.first;
	aFile->ID = XS_NO_ID;
	aFile->value.reference = theSymbol;
	mxFiles.value.list.first = aFile;
}

void fxListBreakpoints(txMachine* the)
{
	txSlot* aBreakpoint;

	fxEcho(the, "<breakpoints>");
	aBreakpoint = mxBreakpoints.value.list.first;
	while (aBreakpoint)	{
		fxEcho(the, "<breakpoint  path=\"");
		fxEchoString(the, aBreakpoint->value.reference->value.symbol.string);
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
#ifdef mxSDK
        txString aPath = aFile->value.reference->value.symbol.string;
		if (c_strstr(aPath, "/applications/")) {
#endif
		fxEcho(the, "<file  path=\"");
		fxEchoString(the, (txString)mxCleanPath(aFile->value.reference->value.symbol.string));
		fxEcho(the, "\"/>");
#ifdef mxSDK
		}
#endif
		aFile = aFile->next;
	}
	fxEcho(the, "</files>");
}

void fxListFrames(txMachine* the)
{
	txSlot* aFrame;
	txSlot* aSlot;

	fxEcho(the, "<frames>");
	aFrame = the->frame;
	while (aFrame) {
#ifdef mxSDK
		if (aFrame->flag & XS_SANDBOX_FLAG) {
#endif
		fxEcho(the, "<frame");
		fxEcho(the, " name=\"");
#ifndef mxSDK
		if (aFrame->flag & XS_SANDBOX_FLAG)
			fxEcho(the, "$ ");
#endif
		if (aFrame->flag & XS_STRICT_FLAG)
			fxEcho(the, "! ");
		fxEchoID(the, aFrame->ID);
		fxEcho(the, "\"");
		fxEchoAddress(the, aFrame);
		if ((aSlot = aFrame - 1) && (aSlot->next)) {
			if (aFrame->flag & XS_C_FLAG)
				fxEchoPathLine(the, (txString)aSlot->next, aSlot->ID);
			else
				fxEchoPathLine(the, aSlot->next->value.symbol.string, aSlot->ID);
		}
		fxEcho(the, "/>");
#ifdef mxSDK
		}
#endif
		aFrame = aFrame->next;
	}
	fxEcho(the, "</frames>");
}

void fxListGlobal(txMachine* the)
{
	txSlot aSymbol;
	txSlot aList;
	txSlot* aProperty;
	txInteger aCount;
	txSlot** aSlotAddress;

	aSymbol.next = C_NULL;
	aSymbol.value.symbol.string = C_NULL;
	aSymbol.value.symbol.sum = 0;

	aList.value.list.first = &aSymbol;
	aList.value.list.last = &aSymbol;

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

void fxListGrammar(txMachine* the)
{
	txSlot aSymbol;
	txSlot aList;
	txFlag aFlag;
	txSlot* aSlot;

	aSymbol.next = C_NULL;
	aSymbol.value.symbol.string = C_NULL;
	aSymbol.value.symbol.sum = 0;

	aList.value.list.first = &aSymbol;
	aList.value.list.last = &aSymbol;

	fxEcho(the, "<grammar>");

#ifndef mxSDK
	aFlag = the->frame->flag;
	the->frame->flag &= ~XS_SANDBOX_FLAG;
	aSlot = fxGetProperty(the, mxGlobal.value.reference, the->grammarsID);
	the->frame->flag = aFlag;
	if (aSlot) {
		aSlot = fxGetInstance(the, aSlot);
		aSlot = aSlot->next;
		while (aSlot) {
			fxEchoNode(the, aSlot->value.reference, aSlot->value.reference->next, &aList);
			aSlot = aSlot->next;
		}
	}
#endif

	fxEcho(the, "</grammar>");
}

void fxListLocal(txMachine* the)
{
	txSlot aList;
	txSlot aSymbol;
	txSlot* aFrame;

	aSymbol.next = C_NULL;
	aSymbol.value.symbol.string = C_NULL;
	aSymbol.value.symbol.sum = 0;
	aList.value.list.first = &aSymbol;
	aList.value.list.last = &aSymbol;
	aFrame = the->frame;
	while (aFrame) {
		if (aFrame->flag & XS_DEBUG_FLAG)
			fxEchoFrame(the, aFrame, &aList);
		aFrame = aFrame->next;
	}
}

void fxLogin(txMachine* the)
{
	if (!fxIsConnected(the)) {
		fxConnect(the);
		if (!fxIsConnected(the))
			return;
	}
	fxReadBreakpoints(the);
	fxEchoStart(the);
	fxEcho(the, "<login  name=\"");
	if (the->name)
		fxEchoString(the, the->name);
	else
		fxEchoString(the, "xslib");
	fxEcho(the, "\"/>");
	fxListBreakpoints(the);
	fxEchoStop(the);
	fxSend(the);
	fxReceive(the);
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
	txSlot* aSymbol;
	txInteger aLine;
	txSlot* aBreakpoint;

	if (!thePath)
		return;
	if (!theLine)
		return;
	aLine = c_strtoul(theLine, NULL, 10);
	if ((aLine <= 0) || (0x00007FFF < aLine))
		return;
	aSymbol = fxNewFileC(the, thePath);
	if (!aSymbol)
		return;
	aBreakpoint = mxBreakpoints.value.list.first;
	while (aBreakpoint)	{
		if ((aBreakpoint->value.reference == aSymbol) && (aBreakpoint->ID == (txID)aLine))
			break;
		aBreakpoint = aBreakpoint->next;
	}
	if (!aBreakpoint) {
		aBreakpoint = fxNewSlot(the);
		aBreakpoint->next = mxBreakpoints.value.list.first;
		aBreakpoint->ID = (txID)aLine;
		aBreakpoint->value.reference = aSymbol;
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
	i = fxGetSymbol(the, i->ID);
	j = fxGetSymbol(the, j->ID);
	return c_strcmp(i->value.symbol.string, j->value.symbol.string);
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

txSlot* fxNewFile(txMachine* the, txSlot* theSlot)
{
	txSlot* aSymbol = C_NULL;

	if (theSlot->kind == XS_STRING_KIND) {
		aSymbol = fxFindSymbol(the, theSlot->value.string);
		if (!aSymbol)
			aSymbol = fxNewSymbol(the, theSlot);
	#ifdef mxDebug
		fxInsertFile(the, aSymbol);
	#endif
	}
	return aSymbol;
}

txSlot* fxNewFileC(txMachine* the, txString thePath)
{
	txSlot* aSymbol = C_NULL;

	if (thePath) {
		aSymbol = fxFindSymbol(the, thePath);
		if (!aSymbol)
			aSymbol = fxNewSymbolC(the, thePath);
	#ifdef mxDebug
		fxInsertFile(the, aSymbol);
	#endif
	}
	return aSymbol;
}

void fxReport(txMachine* the, txString thePath, txInteger theLine, txString theFormat, ...)
{
	c_va_list arguments;

	c_va_start(arguments, theFormat);
	fxVReport(the, thePath, theLine, theFormat, arguments);
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

void fxVReport(txMachine* the, txString thePath, txInteger theLine, txString theFormat, c_va_list theArguments)
{
#if __FSK_LAYER__ && SUPPORT_INSTRUMENTATION
	if (FskInstrumentedItemHasListenersForLevel(the, kFskInstrumentationLevelNormal)) {
		void *data[2];
		data[0] = theFormat;
		data[1] = (void *)&theArguments;
		FskInstrumentedItemSendMessageForLevel(the, kFskXSInstrTrace, data, kFskInstrumentationLevelNormal);
	}
#endif
#ifdef mxSDK
	if (thePath) {
		if (!c_strstr(thePath, "/applications/"))
			return;
	}
	else {
		txSlot* aFrame = the->frame->next;
		if (!aFrame)
			return;
		if ((aFrame->flag & XS_SANDBOX_FLAG) == 0)
			return;
	}
#endif
#ifdef mxDebug
	if (fxIsConnected(the)) {
		fxEchoStart(the);
		fxEcho(the, "<log");
		fxEchoPathLine(the, thePath, theLine);
		fxEcho(the, ">");
		fxEchoFormat(the, theFormat, theArguments);
		fxEcho(the, "</log>");
		fxEchoStop(the);
		fxSend(the);
		fxReceive(the);
	}
	else {
        if (thePath && theLine)
            fprintf(stderr, "%s:%d ", mxCleanPath(thePath), (int)theLine);
		vfprintf(stderr, theFormat, theArguments);
	}
#endif
}

void fxVReportError(txMachine* the, txString thePath, txInteger theLine, txString theFormat, c_va_list theArguments)
{
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
#ifdef mxSDK
#if 0
// !! FIXME
// this code prevents syntax errors from being sent to the debugger for some
// reason and needs to be fixed!!!

	if (thePath) {
		if (!c_strstr(thePath, "/applications/"))
			return;
	}
	else {
		txSlot* aFrame = the->frame->next;
		if (!aFrame)
			return;
		if ((aFrame->flag & XS_SANDBOX_FLAG) == 0)
			return;
	}
#endif
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
	else {
#endif
		if (thePath != NULL)
#if mxWindows
			fprintf(stderr, "%s(%d): error: ", mxCleanPath(thePath), (int)theLine);
#else
			fprintf(stderr, "%s:%d: error: ", mxCleanPath(thePath), (int)theLine);
#endif
		else
			fprintf(stderr, "# error: ");
		vfprintf(stderr, theFormat, theArguments);
		fprintf(stderr, "!\n");
#ifdef mxDebug
	}
#endif
}

void fxVReportWarning(txMachine* the, txString thePath, txInteger theLine, txString theFormat, c_va_list theArguments)
{
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
#ifdef mxSDK
	if (thePath) {
		if (!c_strstr(thePath, "/applications/"))
			return;
	}
	else {
		txSlot* aFrame = the->frame->next;
		if (!aFrame)
			return;
		if ((aFrame->flag & XS_SANDBOX_FLAG) == 0)
			return;
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
	else {
#endif
		if (thePath != NULL)
#if mxWindows
			fprintf(stderr, "%s(%d): warning: ", mxCleanPath(thePath), (int)theLine);
#else
			fprintf(stderr, "%s:%d: warning: ", mxCleanPath(thePath), (int)theLine);
#endif
		else
			fprintf(stderr, "# warning: ");
		vfprintf(stderr, theFormat, theArguments);
		fprintf(stderr, "!\n");
#ifdef mxDebug
	}
#endif
}
