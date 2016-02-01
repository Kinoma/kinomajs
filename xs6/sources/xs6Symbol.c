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
#include "xs6All.h"

static void fx_Symbol(txMachine* the);
static void fx_Symbol_for(txMachine* the);
static void fx_Symbol_keyFor(txMachine* the);
static void fx_Symbol_prototype_toPrimitive(txMachine* the);
static void fx_Symbol_prototype_toString(txMachine* the);
static void fx_Symbol_prototype_valueOf(txMachine* the);
static txSlot* fxCheckSymbol(txMachine* the, txSlot* it);

void fxBuildSymbol(txMachine* the)
{
    static const txHostFunctionBuilder gx_Symbol_prototype_builders[] = {
		{ fx_Symbol_prototype_toString, 0, _toString },
		{ fx_Symbol_prototype_valueOf, 0, _valueOf },
		{ C_NULL, 0, 0 },
    };
    static const txHostFunctionBuilder gx_Symbol_builders[] = {
		{ fx_Symbol_for, 1, _for },
		{ fx_Symbol_keyFor, 1, _keyFor },
		{ C_NULL, 0, 0 },
    };
    const txHostFunctionBuilder* builder;
	txSlot* slot;
	mxPush(mxObjectPrototype);
	slot = fxLastProperty(the, fxNewSymbolInstance(the));
	for (builder = gx_Symbol_prototype_builders; builder->callback; builder++)
		slot = fxNextHostFunctionProperty(the, slot, builder->callback, builder->length, mxID(builder->id), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, fx_Symbol_prototype_toPrimitive, 1, mxID(_Symbol_toPrimitive), XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	slot = fxNextStringXProperty(the, slot, "Symbol", mxID(_Symbol_toStringTag), XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	mxSymbolPrototype = *the->stack;
	slot = fxLastProperty(the, fxNewHostConstructorGlobal(the, fx_Symbol, 0, mxID(_Symbol), XS_DONT_ENUM_FLAG));
	for (builder = gx_Symbol_builders; builder->callback; builder++)
		slot = fxNextHostFunctionProperty(the, slot, builder->callback, builder->length, mxID(builder->id), XS_DONT_ENUM_FLAG);
	slot = fxNextSymbolProperty(the, slot, mxID(_Symbol_hasInstance), mxID(_hasInstance), XS_GET_ONLY);
	slot = fxNextSymbolProperty(the, slot, mxID(_Symbol_isConcatSpreadable), mxID(_isConcatSpreadable), XS_GET_ONLY);
	slot = fxNextSymbolProperty(the, slot, mxID(_Symbol_iterator), mxID(_iterator), XS_GET_ONLY);
	slot = fxNextSymbolProperty(the, slot, mxID(_Symbol_match), mxID(_match), XS_GET_ONLY);
	slot = fxNextSymbolProperty(the, slot, mxID(_Symbol_replace), mxID(_replace), XS_GET_ONLY);
	slot = fxNextSymbolProperty(the, slot, mxID(_Symbol_search), mxID(_search), XS_GET_ONLY);
	slot = fxNextSymbolProperty(the, slot, mxID(_Symbol_species), mxID(_species), XS_GET_ONLY);
	slot = fxNextSymbolProperty(the, slot, mxID(_Symbol_split), mxID(_split), XS_GET_ONLY);
	slot = fxNextSymbolProperty(the, slot, mxID(_Symbol_toPrimitive), mxID(_toPrimitive), XS_GET_ONLY);
	slot = fxNextSymbolProperty(the, slot, mxID(_Symbol_toStringTag), mxID(_toStringTag), XS_GET_ONLY);
	slot = fxNextSymbolProperty(the, slot, mxID(_Symbol_unscopables), mxID(_unscopables), XS_GET_ONLY);
	the->stack++;
}

txSlot* fxNewSymbolInstance(txMachine* the)
{
	txSlot* instance;
	txSlot* property;
	instance = fxNewObjectInstance(the);
	property = fxNextSymbolProperty(the, instance, XS_NO_ID, XS_NO_ID, XS_INTERNAL_FLAG | XS_GET_ONLY);
	return instance;
}

void fx_Symbol(txMachine* the)
{
	txID id = the->keyIndex;
	txSlot* description;
	if (mxTarget->kind != XS_UNDEFINED_KIND)
		mxTypeError("new Symbol");
	if (id == the->keyCount)
		mxUnknownError("not enough IDs");
	if (mxArgc > 0) {
		fxToString(the, mxArgv(0));
		description = fxNewSlot(the);
		description->kind = mxArgv(0)->kind;
		description->value.string = mxArgv(0)->value.string;
	}
	else
		description = C_NULL;
	the->keyArray[id] = description;
	the->keyIndex++;
	mxResult->kind = XS_SYMBOL_KIND;
	mxResult->value.symbol = 0x8000 | id;
}

void fx_Symbol_for(txMachine* the)
{
	txString string;
	txU1* p;
	txU4 sum;
	txU4 modulo;
	txSlot* result;
	txID index;
	if (mxArgc < 1)
		mxSyntaxError("no key parameter");
	string = fxToString(the, mxArgv(0));
	p = (txU1*)string;
	sum = 0;
	while(*p != 0) {
		sum = (sum << 1) + *p++;
	}
	sum &= 0x7FFFFFFF;
	modulo = sum % the->symbolModulo;
	result = the->symbolTable[modulo];
	while (result != C_NULL) {
		if (result->value.key.sum == sum)
			if (c_strcmp(result->value.key.string, string) == 0)
				break;
		result = result->next;
	}
	if (result == C_NULL) {
		index = the->keyIndex;
		if (index == the->keyCount)
			mxUnknownError("not enough IDs");
		result = fxNewSlot(the);
		result->next = the->symbolTable[modulo];
		result->kind = XS_KEY_KIND;
		result->ID = 0x8000 | index;
		result->value.key.string = mxArgv(0)->value.string;
		result->value.key.sum = sum;
		the->keyArray[index] = result;
		the->keyIndex++;
		the->symbolTable[modulo] = result;
	}
	mxResult->kind = XS_SYMBOL_KIND;
	mxResult->value.symbol = result->ID;
}

void fx_Symbol_keyFor(txMachine* the)
{
	txSlot* slot;
	txSlot* key;
	if (mxArgc < 1) mxSyntaxError("no sym parameter");
	slot = fxCheckSymbol(the, mxArgv(0));
	if (!slot) mxTypeError("sym is no symbol");
	key = fxGetKey(the, slot->value.symbol);
	if (key && ((key->flag & XS_DONT_ENUM_FLAG) == 0)) {
		mxResult->kind = XS_STRING_KIND;
		mxResult->value.string = key->value.key.string;
	}
}

void fx_Symbol_prototype_toPrimitive(txMachine* the)
{
	txSlot* slot = fxCheckSymbol(the, mxThis);
	if (!slot) mxTypeError("this is no symbol");
	mxResult->kind = slot->kind;
	mxResult->value = slot->value;
}

void fx_Symbol_prototype_toString(txMachine* the)
{
	txSlot* slot = fxCheckSymbol(the, mxThis);
	if (!slot) mxTypeError("this is no symbol");
	mxResult->kind = slot->kind;
	mxResult->value = slot->value;
	fxSymbolToString(the, mxResult);
}

void fx_Symbol_prototype_valueOf(txMachine* the)
{
	txSlot* slot = fxCheckSymbol(the, mxThis);
	if (!slot) mxTypeError("this is no symbol");
	mxResult->kind = slot->kind;
	mxResult->value = slot->value;
}

txSlot* fxCheckSymbol(txMachine* the, txSlot* it)
{
	txSlot* result = C_NULL;
	if (it->kind == XS_SYMBOL_KIND)
		result = it;
	else if (it->kind == XS_REFERENCE_KIND) {
		txSlot* instance = it->value.reference;
		it = instance->next;
		if ((it) && (it->flag & XS_INTERNAL_FLAG) && (it->kind == XS_SYMBOL_KIND) && (instance != mxSymbolPrototype.value.reference))
			result = it;
	}
	return result;
}

void fxSymbolToString(txMachine* the, txSlot* slot)
{
	txSlot* key = fxGetKey(the, slot->value.symbol);
	fxCopyStringC(the, slot, "Symbol(");
	if (key)
		fxConcatStringC(the, slot, key->value.string);
	fxConcatStringC(the, slot, ")");
}

txSlot* fxGetKey(txMachine* the, txID theID)
{
	txSlot* result = C_NULL;
	
	if (theID < 0) {
		theID &= 0x7FFF;
		if (theID < the->keyCount)
			result = the->keyArray[theID];
	}
	return result;
}

txSlot* fxFindName(txMachine* the, txString theString)
{
	txU1* aString;
	txU4 aSum;
	txU4 aModulo;
	txSlot* result;
	
	aString = (txU1*)theString;
	aSum = 0;
	while(*aString != 0) {
		aSum = (aSum << 1) + *aString++;
	}
	aSum &= 0x7FFFFFFF;
	aModulo = aSum % the->nameModulo;
	result = the->nameTable[aModulo];
	while (result != C_NULL) {
		if (result->value.key.sum == aSum)
			if (c_strcmp(result->value.key.string, theString) == 0)
				break;
		result = result->next;
	}
	return result;
}

txSlot* fxNewName(txMachine* the, txSlot* theSlot)
{
	txU1* string;
	txU4 sum;
	txU4 modulo;
	txSlot* result;
	txID index;
	
	string = (txU1*)theSlot->value.string;
	sum = 0;
	while(*string != 0) {
		sum = (sum << 1) + *string++;
	}
	sum &= 0x7FFFFFFF;
	modulo = sum % the->nameModulo;
	result = the->nameTable[modulo];
	while (result != C_NULL) {
		if (result->value.key.sum == sum)
			if (c_strcmp(result->value.key.string, theSlot->value.string) == 0)
				break;
		result = result->next;
	}
	if (result == C_NULL) {
		index = the->keyIndex;
		if (index == the->keyCount)
			mxUnknownError("not enough IDs");
		result = fxNewSlot(the);
		result->next = the->nameTable[modulo];
		result->flag = XS_DONT_ENUM_FLAG;
		result->kind = XS_KEY_KIND;
		result->ID = 0x8000 | index;
		result->value.key.string = theSlot->value.string;
		result->value.key.sum = sum;
		the->keyArray[index] = result;
		the->keyIndex++;
		the->nameTable[modulo] = result;
	}
	return result;
}

txSlot* fxNewNameC(txMachine* the, txString theString)
{
	txU1* string;
	txU4 sum;
	txU4 modulo;
	txSlot* result;
	txID index;
	
	string = (txU1*)theString;
	sum = 0;
	while(*string != 0) {
		sum = (sum << 1) + *string++;
	}
	sum &= 0x7FFFFFFF;
	modulo = sum % the->nameModulo;
	result = the->nameTable[modulo];
	while (result != C_NULL) {
		if (result->value.key.sum == sum)
			if (c_strcmp(result->value.key.string, theString) == 0)
				break;
		result = result->next;
	}
	if (result == C_NULL) {
		index = the->keyIndex;
		if (index == the->keyCount)
			mxUnknownError("not enough IDs");
		result = fxNewSlot(the);
		result->next = the->nameTable[modulo];
		result->flag = XS_DONT_ENUM_FLAG;
		result->kind = XS_KEY_KIND;
		result->ID = 0x8000 | index;
		result->value.key.string = C_NULL;
		result->value.key.sum = sum;
		the->keyArray[index] = result;
		the->keyIndex++;
		the->nameTable[modulo] = result;
		result->value.key.string = (txString)fxNewChunk(the, c_strlen(theString) + 1);
		c_strcpy(result->value.key.string, theString);
	}
	return result;
}

txSlot* fxNewNameX(txMachine* the, txString theString)
{
	txU1* string;
	txU4 sum;
	txU4 modulo;
	txSlot* result;
	txID index;
	
	string = (txU1*)theString;
	sum = 0;
	while(*string != 0) {
		sum = (sum << 1) + *string++;
	}
	sum &= 0x7FFFFFFF;
	modulo = sum % the->nameModulo;
	result = the->nameTable[modulo];
	while (result != C_NULL) {
		if (result->value.key.sum == sum)
			if (c_strcmp(result->value.key.string, theString) == 0)
				break;
		result = result->next;
	}
	if (result == C_NULL) {
		index = the->keyIndex;
		if (index == the->keyCount)
			mxUnknownError("not enough IDs");
		result = fxNewSlot(the);
		result->next = the->nameTable[modulo];
		result->flag = XS_DONT_ENUM_FLAG;
		result->kind = XS_KEY_X_KIND;
		result->ID = 0x8000 | index;
		result->value.key.string = theString;
		result->value.key.sum = sum;
		the->keyArray[index] = result;
		the->keyIndex++;
		the->nameTable[modulo] = result;
	}
	return result;
}

txSlot* fxAt(txMachine* the, txSlot* slot)
{
	txIndex index;
	txString string;
	txSlot* key;
again:
	if ((slot->kind == XS_INTEGER_KIND) && fxIntegerToIndex(the->dtoa, slot->value.integer, &index)) {
		slot->value.at.id = 0;
		slot->value.at.index = index;
	}
	else if ((slot->kind == XS_NUMBER_KIND) && fxNumberToIndex(the->dtoa, slot->value.number, &index)) {
		slot->value.at.id = 0;
		slot->value.at.index = index;
	}
	else if (slot->kind == XS_SYMBOL_KIND) {
		slot->value.at.id = slot->value.symbol;
		slot->value.at.index = XS_NO_ID;
	}
    else {
        if (slot->kind == XS_REFERENCE_KIND) {
            fxToPrimitive(the, slot, XS_STRING_HINT);
            goto again;
        }
        string = fxToString(the, slot);
        if (fxStringToIndex(the->dtoa, string, &index)) {
            slot->value.at.id = 0;
            slot->value.at.index = index;
        }
        else {
            if (slot->kind == XS_STRING_X_KIND)
                key = fxNewNameX(the, string);
            else
                key = fxNewName(the, slot);
            slot->value.at.id = key->ID;
            slot->value.at.index = XS_NO_ID;
        }
    }
	slot->kind = XS_AT_KIND;
	return slot;
}

void fxKeyAt(txMachine* the, txID id, txIndex index, txSlot* slot)
{
	if (id) {
		txSlot* key = fxGetKey(the, id);
		if (key && (key->flag & XS_DONT_ENUM_FLAG)) {
			if (key->kind == XS_KEY_KIND) {
				slot->kind = XS_STRING_KIND;
				slot->value.string = key->value.key.string;
			}
			else{
				slot->kind = XS_STRING_X_KIND;
				slot->value.string = key->value.key.string;
			}
		}
		else {
			slot->kind = XS_SYMBOL_KIND;
			slot->value.symbol = id;
		}
	}
	else {
		char buffer[16];
		fxCopyStringC(the, slot, fxNumberToString(the->dtoa, index, buffer, sizeof(buffer), 0, 0));
	}
}


txInteger fxSlotToIndex(txMachine* the, txSlot* slot, txIndex* index)
{
	txString string;
	txSlot* key;
again:
	if ((slot->kind == XS_INTEGER_KIND) && fxIntegerToIndex(the->dtoa, slot->value.integer, index))
		return 0;
	if ((slot->kind == XS_NUMBER_KIND) && fxNumberToIndex(the->dtoa, slot->value.number, index))
		return 0;
	if (slot->kind == XS_SYMBOL_KIND)
		return slot->value.symbol;
	if (slot->kind == XS_REFERENCE_KIND) {
		fxToPrimitive(the, slot, XS_STRING_HINT);
		goto again;
	}
	string = fxToString(the, slot);
	if (fxStringToIndex(the->dtoa, string, index))
		return 0;
	if (slot->kind == XS_STRING_X_KIND)
		key = fxNewNameX(the, string);
	else
		key = fxNewName(the, slot);
	return key->ID;
}

void fxIDToString(txMachine* the, txInteger id, txString theBuffer, txSize theSize)
{
	if (id < 0) {
		id &= 0x7FFF;
		if (id < the->keyCount) {
			txSlot* key = the->keyArray[id];
			if (key) {
				if ((key->kind == XS_KEY_KIND) || (key->kind == XS_KEY_X_KIND))
					snprintf(theBuffer, theSize, "%s", key->value.key.string);
				else if ((key->kind == XS_STRING_KIND) || (key->kind == XS_STRING_X_KIND))
					snprintf(theBuffer, theSize, "[%s]", key->value.string);
				else
					c_strcpy(theBuffer, "");
			}
			else
				c_strcpy(theBuffer, "");
		}
		else
			c_strcpy(theBuffer, "?");
	}
	else
		fxIntegerToString(the->dtoa, id, theBuffer, theSize);
}
