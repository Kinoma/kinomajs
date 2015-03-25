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
#include "xs.h"

#define XS_SAME_ID -2

#define xsCat2(_SLOT0,_SLOT1) \
	(xsOverflow(-3), \
	*(--the->stack) = (_SLOT1), \
	*(--the->stack) = (_SLOT0), \
	fxInteger(the, --the->stack, 2), \
	fxCatenate(the), \
	*(the->stack++))

#define xsCat3(_SLOT0,_SLOT1,_SLOT2) \
	(xsOverflow(-4), \
	*(--the->stack) = (_SLOT2), \
	*(--the->stack) = (_SLOT1), \
	*(--the->stack) = (_SLOT0), \
	fxInteger(the, --the->stack, 3), \
	fxCatenate(the), \
	*(the->stack++))

#define xsCat4(_SLOT0,_SLOT1,_SLOT2,_SLOT3) \
	(xsOverflow(-5), \
	*(--the->stack) = (_SLOT3), \
	*(--the->stack) = (_SLOT2), \
	*(--the->stack) = (_SLOT1), \
	*(--the->stack) = (_SLOT0), \
	fxInteger(the, --the->stack, 4), \
	fxCatenate(the), \
	*(the->stack++))
	
#define xsCat5(_SLOT0,_SLOT1,_SLOT2,_SLOT3,_SLOT4) \
	(xsOverflow(-6), \
	*(--the->stack) = (_SLOT4), \
	*(--the->stack) = (_SLOT3), \
	*(--the->stack) = (_SLOT2), \
	*(--the->stack) = (_SLOT1), \
	*(--the->stack) = (_SLOT0), \
	fxInteger(the, --the->stack, 5), \
	fxCatenate(the), \
	*(the->stack++))
	
#define xsToID(_SLOT) \
	(xsOverflow(-1), \
	*(--the->stack) = (_SLOT), \
	fxToID(the))

typedef struct {
	txSize symbolModulo;
	txSlot** symbolTable;
	txID symbolCount;
	txID symbolIndex;
	txSlot** symbolArray;
} xsSymbolsData;

typedef struct {
	txByte* current;
	txByte* bottom;
	txByte* top;
	void* symbols;
	txSize symbolsSize;
} xsBuffer;

extern void fxBuild_xscPackage(txMachine* the);

extern void xscAppend(txMachine* the, txByte* theData, txSize theSize);
extern void xscAppend1(txMachine* the, txByte theValue);
extern void xscAppend2(txMachine* the, txID theValue);
extern void xscAppend4(txMachine* the, txInteger theValue);
extern void xscAppend8(txMachine* the, txNumber theValue);
extern void xscAppendFunction(txMachine* the);
extern void xscAppendID(txMachine* the, txByte theCode, txID theID);
extern void xscAppendInteger(txMachine* the, txInteger theInteger);
extern void xscAppendPattern(txMachine* the, txByte theCode);
extern void xscAppendString(txMachine* the, txString aString);
extern void xscBuildSymbols(txMachine* the);
extern void fxCatenate(txMachine* the);
extern void xscCreateBuffer(txMachine* the);
extern void xscDeleteBuffer(txMachine* the);
extern void fxQueuePattern(txMachine* the, txKind theKind, 
		txID theNamespaceID, txID theNameID, 
		txID theID, txFlag theFlag);
extern txID fxToID(txMachine* the);
