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

#if mxUseApHash
static txSize AP_hash(txString str)
{
	txSize res = 0;
	int i;
	for(i=0; *str; i++)
	{
		if((i&1)==0)
			res ^= ((res << 7) ^ (*str++) ^ (res >> 3));
		else
			res ^= (~((res << 11) ^ (*str++) ^ (res >> 5)));
	}
	return res & 0x7fffffff;
}
#endif

txSlot* fxGetSymbol(txMachine* the, txID theID)
{
	txSlot* result = C_NULL;
	
	if (theID < 0) {
		theID &= 0x7FFF;
		if (theID < the->symbolCount)
			result = the->symbolArray[theID];
	}
	return result;
}

txSlot* fxFindSymbol(txMachine* the, txString theString)
{
	txString aString;
	txSize aSum;
	txSize aModulo;
	txSlot* result;
	
	aString = theString;
#if mxUseApHash
	aSum = AP_hash(aString);
#else
	aSum = 0;
	while(*aString != 0) {
		aSum = (aSum << 1) + *aString++;
	}
	aSum &= 0x7FFFFFFF;
#endif
	aModulo = aSum % the->symbolModulo;
	result = the->symbolTable[aModulo];
	while (result != C_NULL) {
		if (result->value.symbol.sum == aSum)
			#if mxCmpStr
			if (c_strcmp(result->value.symbol.string, theString) == 0)
			#endif	
				break;
		result = result->next;
	}
	return result;
}

txSlot* fxNewSymbol(txMachine* the, txSlot* theSlot)
{
	txString aString;
	txSize aSum;
	txSize aModulo;
	txSlot* result;
	txID anIndex;
	
	aString = theSlot->value.string;
#if mxUseApHash
	aSum = AP_hash(aString);
#else
	aSum = 0;
	while(*aString != 0) {
		aSum = (aSum << 1) + *aString++;
	}
	aSum &= 0x7FFFFFFF;
#endif
	aModulo = aSum % the->symbolModulo;
	result = the->symbolTable[aModulo];
	while (result != C_NULL) {
		if (result->value.symbol.sum == aSum)
			#if mxCmpStr
			if (c_strcmp(result->value.symbol.string, theSlot->value.string) == 0)
			#endif	
				break;
		result = result->next;
	}
	if (result == C_NULL) {
		anIndex = the->symbolIndex;
		if (anIndex == the->symbolCount) {
			--the->stack;
			fxThrowMessage(the, XS_UNKNOWN_ERROR, fxCopyStringC(the, the->stack, "not enough symbols"));
		}
		result = fxNewSlot(the);
		result->next = the->symbolTable[aModulo];
		result->flag = XS_NO_FLAG;
		result->kind = XS_SYMBOL_KIND;
		result->ID = 0x8000 | anIndex;
		result->value.symbol.string = theSlot->value.string;
		result->value.symbol.sum = aSum;
		the->symbolArray[anIndex] = result;
		the->symbolTable[aModulo] = result;
		the->symbolIndex++;
	}
	return result;
}

txSlot* fxNewSymbolC(txMachine* the, txString theString)
{
	txString aString;
	txSize aSum;
	txSize aModulo;
	txSlot* result;
	txID anIndex;
	
	aString = theString;
#if mxUseApHash
	aSum = AP_hash(aString);
#else
	aSum = 0;
	while(*aString != 0) {
		aSum = (aSum << 1) + *aString++;
	}
	aSum &= 0x7FFFFFFF;
#endif
	aModulo = aSum % the->symbolModulo;
	result = the->symbolTable[aModulo];
	while (result != C_NULL) {
		if (result->value.symbol.sum == aSum)
			#if mxCmpStr
			if (c_strcmp(result->value.symbol.string, theString) == 0)
			#endif	
				break;
		result = result->next;
	}
	if (result == C_NULL) {
		anIndex = the->symbolIndex;
		if (anIndex == the->symbolCount) {
			--the->stack;
			fxThrowMessage(the, XS_UNKNOWN_ERROR, fxCopyStringC(the, the->stack, "not enough symbols"));
		}
		result = fxNewSlot(the);
		result->next = the->symbolTable[aModulo];
		result->flag = XS_NO_FLAG;
		result->kind = XS_SYMBOL_KIND;
		result->ID = 0x8000 | anIndex;
		result->value.symbol.string = C_NULL;
		result->value.symbol.sum = aSum;
		the->symbolArray[anIndex] = result;
		the->symbolTable[aModulo] = result;
		the->symbolIndex++;
		result->value.symbol.string = (txString)fxNewChunk(the, c_strlen(theString) + 1);
		c_strcpy(result->value.symbol.string, theString);
	}
	return result;
}

#if mxUseApHash
txSlot* fxNewSymbolLink(txMachine* the, txString theString, txSize aSum)
{
	txSize aModulo;
	txSlot* result;
	txID anIndex;

	aModulo = aSum % the->symbolModulo;
	result = the->symbolTable[aModulo];
	while (result != C_NULL) {
		if (result->value.symbol.sum == aSum) {
			#if mxCmpStr
			if (c_strcmp(result->value.symbol.string, theString) == 0)
			#endif
				break;
		}
		result = result->next;
	}
	if (result == C_NULL) {
		anIndex = the->symbolIndex;
		if (anIndex == the->symbolCount) {
			--the->stack;
			fxThrowMessage(the, XS_UNKNOWN_ERROR, fxCopyStringC(the, the->stack, "not enough symbols"));
		}
		result = fxNewSlot(the);
		result->next = the->symbolTable[aModulo];
		result->kind = XS_SYMBOL_KIND;
		result->flag = XS_NO_FLAG;
		result->ID = 0x8000 | anIndex;
		result->value.symbol.string = C_NULL;
		result->value.symbol.sum = aSum;
		the->symbolArray[anIndex] = result;
		the->symbolTable[aModulo] = result;
		the->symbolIndex++;
		result->value.symbol.string = (txString)fxNewChunk(the, c_strlen(theString) + 1);
		c_strcpy(result->value.symbol.string, theString);
	}
	return result;
}
#endif
