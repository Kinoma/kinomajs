/*
     Copyright (C) 2010-2015 Marvell International Ltd.
     Copyright (C) 2002-2010 Kinoma, Inc.

     Licensed under the Apache License, Version 2.0 (the "License");
     you may not use this file except in compliance with the License.
     You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

     Unless required by applicable law or agreed to in writing, software
     distributed under the License is distributed on an "AS IS" BASIS,
     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
     See the License for the specific language governing permissions and
     limitations under the License.
*/

#include "xsAll.h"

static void fx_Object(txMachine* the);
static void fx_Object_get_sandbox(txMachine* the);
static void fx_Object_hasOwnProperty(txMachine* the);
static void fx_Object_isPrototypeOf(txMachine* the);
static void fx_Object_propertyIsEnumerable(txMachine* the);
static void fx_Object_propertyIsScriptable(txMachine* the);
static void fx_Object_toString(txMachine* the);
static void fx_Object_valueOf(txMachine* the);

static void fx_Object_create(txMachine* the);
static void fx_Object_defineProperties(txMachine* the);
static void fx_Object_defineProperty(txMachine* the);
static void fx_Object_freeze(txMachine* the);
static void fx_Object_getOwnPropertyDescriptor(txMachine* the);
static void fx_Object_getOwnPropertyNames(txMachine* the);
static void fx_Object_getPrototypeOf(txMachine* the);
static void fx_Object_isExtensible(txMachine* the);
static void fx_Object_isFrozen(txMachine* the);
static void fx_Object_isSealed(txMachine* the);
static void fx_Object_keys(txMachine* the);
static void fx_Object_preventExtensions(txMachine* the);
static void fx_Object_seal(txMachine* the);

static void fxDefineProperty(txMachine* the, txSlot* theInstance, txID theID, txSlot* theDescriptor);
static void fxEnumProperty(txMachine* the, txSlot* theContext, txID theID, txSlot* theProperty);
static void fxFreezeProperty(txMachine* the, txSlot* theResult, txID theID, txSlot* theProperty);
static void fxIsPropertyFrozen(txMachine* the, txSlot* theResult, txID theID, txSlot* theProperty);
static void fxIsPropertySealed(txMachine* the, txSlot* theResult, txID theID, txSlot* theProperty);
static void fxSealProperty(txMachine* the, txSlot* theResult, txID theID, txSlot* theProperty);

void fxBuildObject(txMachine* the)
{
	mxPush(mxGlobal);
	
	fxNewInstance(the);
	fxNewHostFunction(the, fx_Object_get_sandbox, 0);
	fxQueueID(the, fxID(the, "sandbox"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG | XS_DONT_SCRIPT_FLAG | XS_GETTER_FLAG);
	fxNewHostFunction(the, fx_Object_hasOwnProperty, 1);
	fxQueueID(the, fxID(the, "hasOwnProperty"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_Object_isPrototypeOf, 1);
	fxQueueID(the, fxID(the, "isPrototypeOf"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_Object_propertyIsEnumerable, 1);
	fxQueueID(the, fxID(the, "propertyIsEnumerable"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_Object_propertyIsScriptable, 1);
	fxQueueID(the, fxID(the, "propertyIsScriptable"), XS_DONT_ENUM_FLAG | XS_DONT_SCRIPT_FLAG);
	fxNewHostFunction(the, fx_Object_toString, 0);
	fxQueueID(the, fxID(the, "toLocaleString"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_Object_toString, 0);
	fxQueueID(the, fxID(the, "toString"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_Object_valueOf, 0);
	fxQueueID(the, fxID(the, "valueOf"), XS_DONT_ENUM_FLAG);
	fxAliasInstance(the, the->stack);
	mxObjectPrototype = *the->stack;
	fxNewHostConstructor(the, fx_Object, 1);
	
	fxNewHostFunction(the, fx_Object_create, 2);
	fxQueueID(the, fxID(the, "create"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_Object_defineProperties, 2);
	fxQueueID(the, fxID(the, "defineProperties"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_Object_defineProperty, 3);
	fxQueueID(the, fxID(the, "defineProperty"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_Object_freeze, 1);
	fxQueueID(the, fxID(the, "freeze"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_Object_getOwnPropertyDescriptor, 2);
	fxQueueID(the, fxID(the, "getOwnPropertyDescriptor"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_Object_getOwnPropertyNames, 1);
	fxQueueID(the, fxID(the, "getOwnPropertyNames"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_Object_getPrototypeOf, 1);
	fxQueueID(the, fxID(the, "getPrototypeOf"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_Object_isExtensible, 1);
	fxQueueID(the, fxID(the, "isExtensible"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_Object_isFrozen, 1);
	fxQueueID(the, fxID(the, "isFrozen"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_Object_isSealed, 1);
	fxQueueID(the, fxID(the, "isSealed"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_Object_keys, 1);
	fxQueueID(the, fxID(the, "keys"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_Object_preventExtensions, 1);
	fxQueueID(the, fxID(the, "preventExtensions"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_Object_seal, 1);
	fxQueueID(the, fxID(the, "seal"), XS_DONT_ENUM_FLAG);
	
	the->stack->value.reference->next->next->next->flag |= XS_DONT_SET_FLAG;
	 *(--the->stack) = mxObjectPrototype;
	fxPutID(the, fxID(the, "constructor"), XS_DONT_ENUM_FLAG, XS_DONT_ENUM_FLAG);
	fxQueueID(the, fxID(the, "Object"), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);

	fxNewHostFunction(the, fx_Object_toString, 0);
	fxQueueID(the, fxID(the, "toLocaleString"), XS_DONT_ENUM_FLAG);
	fxNewHostFunction(the, fx_Object_toString, 0);
	fxQueueID(the, fxID(the, "toString"), XS_DONT_ENUM_FLAG);
	the->stack++;;
}

txSlot* fxNewObjectInstance(txMachine* the)
{
	txSlot* anInstance;

	anInstance = fxNewSlot(the);
	anInstance->next = C_NULL;
	anInstance->flag = XS_NO_FLAG;
	anInstance->kind = XS_INSTANCE_KIND;
	anInstance->value.instance.garbage = C_NULL;
	anInstance->value.instance.prototype = C_NULL;
	if (the->stack->kind == XS_ALIAS_KIND)
		anInstance->ID = the->stack->value.alias;
	else
		anInstance->value.instance.prototype = the->stack->value.reference;
	the->stack->value.reference = anInstance;
	the->stack->kind = XS_REFERENCE_KIND;
	
	return anInstance;
}

void fx_Object(txMachine* the)
{
	if ((mxArgc == 0) || (mxArgv(0)->kind == XS_UNDEFINED_KIND) || (mxArgv(0)->kind == XS_NULL_KIND)) {
		if (mxResult->kind == XS_UNDEFINED_KIND) {
			mxPush(mxObjectPrototype);
			fxNewObjectInstance(the);
			*mxResult = *(the->stack++);
		}
	}
	else {
		*mxResult = *mxArgv(0);
		fxToInstance(the, mxResult);
	}
}

void fx_Object_get_sandbox(txMachine* the)
{
	txSlot* aPrototype;
	txSlot* anInstance;
	
	aPrototype = fxGetInstance(the, mxThis);
	if (aPrototype->flag & XS_SANDBOX_FLAG)
		*mxResult = *mxThis;
	else {
		anInstance = fxNewSlot(the);
		anInstance->next = C_NULL;
		anInstance->ID = XS_NO_ID;
		anInstance->flag = XS_SANDBOX_FLAG;
		anInstance->kind = XS_INSTANCE_KIND;
		anInstance->value.instance.garbage = C_NULL;
		anInstance->value.instance.prototype = aPrototype;
		mxResult->value.reference = anInstance;
		mxResult->kind = XS_REFERENCE_KIND;
	}
}

void fx_Object_hasOwnProperty(txMachine* the)
{
	txSlot* anInstance;
	txSlot* aProperty;
	txID anID;

	the->frame->flag |= the->frame->next->flag & XS_SANDBOX_FLAG;
	if (mxArgc < 1)
		mxDebug0(the, XS_SYNTAX_ERROR, "Object.prototype.hasOwnProperty: no property parameter");
	anInstance = fxGetInstance(the, mxThis);
	aProperty = fxGetOwnPropertyAt(the, anInstance, mxArgv(0), &anID);
	mxResult->kind = XS_BOOLEAN_KIND;
	if (aProperty)
		mxResult->value.boolean = 1;
	else {
		if (anInstance->flag & XS_SANDBOX_FLAG)
			anInstance = anInstance->value.instance.prototype;
		if ((anInstance->flag & XS_VALUE_FLAG) && (anID < 0)) {
			aProperty = anInstance->next;
			switch (aProperty->kind) {
			case XS_ARRAY_KIND:
				if (anID == the->lengthID)
					mxResult->value.boolean = 1;
				break;
			case XS_CALLBACK_KIND:
			case XS_CODE_KIND:
				if (anID == the->lengthID)
					mxResult->value.boolean = 1;
				else if (anID == the->prototypeID)
					mxResult->value.boolean = 1;
				break;
			case XS_STRING_KIND:
				if (anID == the->lengthID)
					mxResult->value.boolean = 1;
				break;
			}
		}
	}
}

void fx_Object_isPrototypeOf(txMachine* the)
{
	txSlot* anObject;
	txSlot* aPrototype;
	
	if (mxArgc < 1)
		mxDebug0(the, XS_SYNTAX_ERROR, "Object.prototype.isPrototypeOf: no prototype parameter");
	mxResult->kind = XS_BOOLEAN_KIND;
	if (mxIsReference(mxArgv(0))) {
		anObject = fxGetInstance(the, mxThis);
		if (anObject->flag & XS_SANDBOX_FLAG)
			anObject = anObject->value.instance.prototype;
		aPrototype = fxGetParent(the, fxGetInstance(the, mxArgv(0)));
		if (aPrototype && (aPrototype->flag & XS_SANDBOX_FLAG))
			aPrototype = aPrototype->value.instance.prototype;
		while (aPrototype) {
			if (anObject == aPrototype) {
				mxResult->value.boolean = 1;
				break;
			}
			aPrototype = fxGetParent(the, aPrototype);
		}
	}
}

void fx_Object_propertyIsEnumerable(txMachine* the)
{
	txSlot* aProperty;
	txID anID;

	the->frame->flag |= the->frame->next->flag & XS_SANDBOX_FLAG;
	if (mxArgc < 1)
		mxDebug0(the, XS_SYNTAX_ERROR, "Object.prototype.propertyIsEnumerable: no property parameter");
	mxResult->kind = XS_BOOLEAN_KIND;
	aProperty = fxGetOwnPropertyAt(the, fxGetInstance(the, mxThis), mxArgv(0), &anID);
	if (aProperty && ((aProperty->flag & XS_DONT_ENUM_FLAG) == 0))
		mxResult->value.boolean = 1;
}

void fx_Object_propertyIsScriptable(txMachine* the)
{
	txSlot* aProperty;
	txID anID;

	if (mxArgc < 1)
		mxDebug0(the, XS_SYNTAX_ERROR, "Object.prototype.propertyIsScriptable: no property parameter");
	fxToInstance(the, mxThis);
	mxResult->kind = XS_BOOLEAN_KIND;
	aProperty = fxGetOwnPropertyAt(the, fxGetInstance(the, mxThis), mxArgv(0), &anID);
	if (aProperty && ((aProperty->flag & XS_DONT_SCRIPT_FLAG) == 0))
		mxResult->value.boolean = 1;
}

void fx_Object_toString(txMachine* the)
{
	txSlot* thisObject;
	char aBuffer[256];
	
	thisObject = fxGetInstance(the, mxThis);
	c_strcpy(aBuffer, "[object ");
	if (thisObject->flag & XS_VALUE_FLAG) {
		switch (thisObject->next->kind) {
		case XS_CALLBACK_KIND:
		case XS_CODE_KIND:
			c_strcat(aBuffer, "Function");
			break;
		case XS_ARRAY_KIND:
			c_strcat(aBuffer, "Array");
			break;
		case XS_STRING_KIND:
			c_strcat(aBuffer, "String");
			break;
		case XS_BOOLEAN_KIND:
			c_strcat(aBuffer, "Boolean");
			break;
		case XS_NUMBER_KIND:
			c_strcat(aBuffer, "Number");
			break;
		case XS_DATE_KIND:
			c_strcat(aBuffer, "Date");
			break;
		case XS_REGEXP_KIND:
			c_strcat(aBuffer, "RegExp");
			break;
		case XS_GLOBAL_KIND:
			c_strcat(aBuffer, "global");
			break;
		case XS_UNDEFINED_KIND:
			c_strcat(aBuffer, "Math");
			break;
		default:
			c_strcat(aBuffer, "Object");
			break;
		}
	}
	else 
		c_strcat(aBuffer, "Object");
	c_strcat(aBuffer, "]");
	fxCopyStringC(the, mxResult, aBuffer);
}

void fx_Object_valueOf(txMachine* the)
{
	*mxResult = *mxThis;
}

void fx_Object_create(txMachine* the)
{
	txSlot* properties;
	txSlot* anInstance;

	the->frame->flag |= the->frame->next->flag & XS_SANDBOX_FLAG;
	if (mxArgc < 1)
		mxDebug0(the, XS_SYNTAX_ERROR, "Object.create: no prototype parameter");
	anInstance = fxGetInstance(the, mxArgv(0));
	if (!anInstance)
		mxDebug0(the, XS_TYPE_ERROR, "Object.create: prototype is no object");
	if (anInstance->flag & XS_SANDBOX_FLAG) {
		mxInitSlot(--the->stack, XS_REFERENCE_KIND);
		the->stack->value.reference = anInstance->value.instance.prototype;
	}
	else {
		mxInitSlot(--the->stack, mxArgv(0)->kind);
		the->stack->value = mxArgv(0)->value;
	}
	fxNewInstanceOf(the);
	*mxResult = *(the->stack++);
	if ((mxArgc > 1) && !mxIsUndefined(mxArgv(1))) {
		fxToInstance(the, mxArgv(1));
		properties = fxGetInstance(the, mxArgv(1));
		if (properties) {
			anInstance = fxGetOwnInstance(the, mxResult);
			fxEachOwnProperty(the, properties, XS_DONT_ENUM_FLAG, fxDefineProperty, anInstance);
		}
	}
}

void fx_Object_defineProperties(txMachine* the)
{
	txSlot* properties;
	txSlot* anInstance;

	the->frame->flag |= the->frame->next->flag & XS_SANDBOX_FLAG;
	if (mxArgc < 1)
		mxDebug0(the, XS_TYPE_ERROR, "Object.defineProperties: no instance parameter");
	if (mxArgc < 2)
		mxDebug0(the, XS_TYPE_ERROR, "Object.defineProperties: no properties parameter");
	anInstance = fxGetOwnInstance(the, mxArgv(0));
	if (!anInstance)
		mxDebug0(the, XS_TYPE_ERROR, "Object.defineProperties: instance is no object");
	*mxResult = *mxArgv(0);
	fxToInstance(the, mxArgv(1));
	properties = fxGetInstance(the, mxArgv(1));
	fxEachOwnProperty(the, properties, XS_DONT_ENUM_FLAG, fxDefineProperty, anInstance);
}

void fx_Object_defineProperty(txMachine* the)
{
	txSlot* anInstance;
	txSlot* aSymbol;

	the->frame->flag |= the->frame->next->flag & XS_SANDBOX_FLAG;
	if (mxArgc < 1)
		mxDebug0(the, XS_SYNTAX_ERROR, "Object.defineProperty: no instance parameter");
	if (mxArgc < 2)
		mxDebug0(the, XS_SYNTAX_ERROR, "Object.defineProperty: no property parameter");
	if (mxArgc < 3)
		mxDebug0(the, XS_SYNTAX_ERROR, "Object.defineProperty: no attributes parameter");
	anInstance = fxGetOwnInstance(the, mxArgv(0));
	if (!anInstance)
		mxDebug0(the, XS_TYPE_ERROR, "Object.defineProperty: instance is no object");
	*mxResult = *mxArgv(0);
	fxToString(the, mxArgv(1));	
	aSymbol = fxNewSymbol(the, mxArgv(1));
	fxDefineProperty(the, anInstance, aSymbol->ID, mxArgv(2));
}

void fx_Object_freeze(txMachine* the)
{
	txSlot* anInstance;

	if (mxArgc < 1)
		mxDebug0(the, XS_SYNTAX_ERROR, "Object.freeze: no instance parameter");
	anInstance = fxGetOwnInstance(the, mxArgv(0));
	if (!anInstance)
		mxDebug0(the, XS_TYPE_ERROR, "Object.freeze: instance is no object");
	if (anInstance->flag & XS_SANDBOX_FLAG)
		anInstance = anInstance->value.instance.prototype;
	fxEachOwnProperty(the, anInstance, XS_NO_FLAG, fxFreezeProperty, mxResult);
	anInstance->flag |= XS_DONT_PATCH_FLAG;
}

void fx_Object_getOwnPropertyDescriptor(txMachine* the)
{
	txSlot* anInstance;
	txSlot* aProperty;
	txID anID;
	txFlag aFlag;	
	txSlot* aSlot;

	the->frame->flag |= the->frame->next->flag & XS_SANDBOX_FLAG;
	if (mxArgc < 1)
		mxDebug0(the, XS_SYNTAX_ERROR, "Object.getOwnPropertyDescriptor: no instance parameter");
	if (mxArgc < 2)
		mxDebug0(the, XS_SYNTAX_ERROR, "Object.getOwnPropertyDescriptor: no name parameter");
	anInstance = fxGetInstance(the, mxArgv(0));
	if (!anInstance)
		mxDebug0(the, XS_TYPE_ERROR, "Object.getOwnPropertyDescriptor: instance is no object");
	aProperty = fxGetOwnPropertyAt(the, anInstance, mxArgv(1), &anID);
	if (aProperty) {
		mxZeroSlot(--the->stack);
	}
	else if ((anInstance->flag & XS_VALUE_FLAG) && (anID < 0)) {
		switch (anInstance->next->kind) {
		case XS_ARRAY_KIND:
			if (anID == the->lengthID) {
				mxPush(*mxArgv(0));
				fxGetID(the, anID);
				the->stack->flag = XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG;
				aProperty = the->stack;
			}
			break;
		case XS_CALLBACK_KIND:
		case XS_CODE_KIND:
			if (anID == the->lengthID) {
				mxPush(*mxArgv(0));
				fxGetID(the, anID);
				the->stack->flag = XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG;
				aProperty = the->stack;
			}
			else if (anID == the->prototypeID) {
				mxPush(*mxArgv(0));
				fxGetID(the, anID);
				the->stack->flag = XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG;
				aProperty = the->stack;
			}
			break;
		case XS_STRING_KIND:
			if (anID == the->lengthID) {
				mxPush(*mxArgv(0));
				fxGetID(the, anID);
				the->stack->flag = XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG;
				aProperty = the->stack;
			}
			break;
		}
	}
	if (aProperty) {
		aFlag = the->frame->next->flag & XS_SANDBOX_FLAG;
		mxPush(mxObjectPrototype);
		fxNewObjectInstance(the);
		aSlot = the->stack->value.reference;
		aSlot = aSlot->next = fxNewSlot(the);
		aSlot->flag = aFlag;
		aSlot->ID = the->configurableID;
		aSlot->kind = XS_BOOLEAN_KIND;
		aSlot->value.boolean = (aProperty->flag & XS_DONT_DELETE_FLAG) ? 0 : 1;
		aSlot = aSlot->next = fxNewSlot(the);
		aSlot->flag = aFlag;
		aSlot->ID = the->enumerableID;
		aSlot->kind = XS_BOOLEAN_KIND;
		aSlot->value.boolean = (aProperty->flag & XS_DONT_ENUM_FLAG) ? 0 : 1;
		if (aProperty->kind == XS_ACCESSOR_KIND) {
			aSlot = aSlot->next = fxNewSlot(the);
			aSlot->flag = aFlag;
			aSlot->ID = the->getID;
			if ((aSlot->value.reference = aProperty->value.accessor.getter))
				aSlot->kind = XS_REFERENCE_KIND;
			else
				aSlot->kind = XS_UNDEFINED_KIND;
			aSlot = aSlot->next = fxNewSlot(the);
			aSlot->flag = aFlag;
			aSlot->ID = the->setID;
			if ((aSlot->value.reference = aProperty->value.accessor.setter))
				aSlot->kind = XS_REFERENCE_KIND;
			else
				aSlot->kind = XS_UNDEFINED_KIND;
		}
		else {
			aSlot = aSlot->next = fxNewSlot(the);
			aSlot->flag = aFlag;
			aSlot->ID = the->valueID;
			aSlot->kind = aProperty->kind;
			aSlot->value = aProperty->value;
			aSlot = aSlot->next = fxNewSlot(the);
			aSlot->flag = aFlag;
			aSlot->ID = the->writableID;
			aSlot->kind = XS_BOOLEAN_KIND;
			aSlot->value.boolean = (aProperty->flag & XS_DONT_SET_FLAG) ? 0 : 1;
		}
		mxResult->kind = the->stack->kind;		
		mxResult->value = the->stack->value;
		the->stack++;
		the->stack++;
	}
}

void fx_Object_getOwnPropertyNames(txMachine* the)
{
	txSlot* anInstance;
	txSlot* anArray;
	txSlot aContext;
	txFlag aFlag;
	
	if (mxArgc < 1)
		mxDebug0(the, XS_SYNTAX_ERROR, "Object.getOwnPropertyNames: no instance parameter");
	anInstance = fxGetInstance(the, mxArgv(0));
	if (!anInstance)
		mxDebug0(the, XS_TYPE_ERROR, "Object.getOwnPropertyNames: instance is no object");
	if (anInstance->flag & XS_SANDBOX_FLAG) {
		anInstance = anInstance->value.instance.prototype;
		aFlag = XS_DONT_SCRIPT_FLAG;
	}
	else if (the->frame->next->flag & XS_SANDBOX_FLAG)
		aFlag = XS_DONT_SCRIPT_FLAG;
	else
		aFlag = XS_SANDBOX_FLAG;
	mxPush(mxArrayPrototype);
	anArray = fxNewArrayInstance(the);
	*mxResult = *(the->stack++);
	aContext.value.array.length = 0;
	aContext.value.array.address = anArray->next;
	fxEachOwnProperty(the, anInstance, aFlag, fxEnumProperty, &aContext);
	anArray->next->value.array.length = aContext.value.array.length;
	fxCacheArray(the, anArray);
}

void fx_Object_getPrototypeOf(txMachine* the)
{
	txSlot* anInstance;
	txSlot* aSlot;

	if (mxArgc < 1)
		mxDebug0(the, XS_TYPE_ERROR, "Object.getPrototypeOf: no instance parameter");
	anInstance = fxGetInstance(the, mxArgv(0));
	if (!anInstance)
		mxDebug0(the, XS_TYPE_ERROR, "Object.getPrototypeOf: instance is no object");
	if (anInstance->flag & XS_SANDBOX_FLAG)
		anInstance = anInstance->value.instance.prototype;
	if (anInstance->ID >= 0) {
		mxResult->value.alias = anInstance->ID;
		mxResult->kind = XS_ALIAS_KIND;
	}
	else {
		aSlot = anInstance->value.instance.prototype;
		if (aSlot) {
			mxResult->value.reference = aSlot;
			mxResult->kind = XS_REFERENCE_KIND;
		}
		else
			mxResult->kind = XS_NULL_KIND;
	}
}

void fx_Object_isExtensible(txMachine* the)
{
	txSlot* anInstance;

	if (mxArgc < 1)
		mxDebug0(the, XS_SYNTAX_ERROR, "Object.isExtensible: no instance parameter");
	anInstance = fxGetInstance(the, mxArgv(0));
	if (!anInstance)
		mxDebug0(the, XS_TYPE_ERROR, "Object.isExtensible: instance is no object");
	if (anInstance->flag & XS_SANDBOX_FLAG)
		anInstance = anInstance->value.instance.prototype;
	mxResult->kind = XS_BOOLEAN_KIND;
	mxResult->value.boolean = (anInstance->flag & XS_DONT_PATCH_FLAG) ? 0 : 1;
}

void fx_Object_isFrozen(txMachine* the)
{
	txSlot* anInstance;

	if (mxArgc < 1)
		mxDebug0(the, XS_SYNTAX_ERROR, "Object.isFrozen: no instance parameter");
	anInstance = fxGetInstance(the, mxArgv(0));
	if (!anInstance)
		mxDebug0(the, XS_TYPE_ERROR, "Object.isFrozen: instance is no object");
	if (anInstance->flag & XS_SANDBOX_FLAG)
		anInstance = anInstance->value.instance.prototype;
	mxResult->kind = XS_BOOLEAN_KIND;
	mxResult->value.boolean = 1;
	fxEachOwnProperty(the, anInstance, XS_NO_FLAG, fxIsPropertyFrozen, mxResult);
	if (!(anInstance->flag & XS_DONT_PATCH_FLAG))
		mxResult->value.boolean = 0;
}

void fx_Object_isSealed(txMachine* the)
{
	txSlot* anInstance;

	if (mxArgc < 1)
		mxDebug0(the, XS_SYNTAX_ERROR, "Object.isSealed: no instance parameter");
	anInstance = fxGetInstance(the, mxArgv(0));
	if (!anInstance)
		mxDebug0(the, XS_TYPE_ERROR, "Object.isSealed: instance is no object");
	if (anInstance->flag & XS_SANDBOX_FLAG)
		anInstance = anInstance->value.instance.prototype;
	mxResult->kind = XS_BOOLEAN_KIND;
	mxResult->value.boolean = 1;
	fxEachOwnProperty(the, anInstance, XS_NO_FLAG, fxIsPropertySealed, mxResult);
	if (!(anInstance->flag & XS_DONT_PATCH_FLAG))
		mxResult->value.boolean = 0;
}

void fx_Object_keys(txMachine* the)
{
	txSlot* anInstance;
	txSlot* anArray;
	txSlot aContext;
	txFlag aFlag;

	if (mxArgc < 1)
		mxDebug0(the, XS_SYNTAX_ERROR, "Object.keys: no instance parameter");
	anInstance = fxGetInstance(the, mxArgv(0));
	if (!anInstance)
		mxDebug0(the, XS_TYPE_ERROR, "Object.keys: instance is no object");
	if (anInstance->flag & XS_SANDBOX_FLAG) {
		anInstance = anInstance->value.instance.prototype;
		aFlag = XS_DONT_ENUM_FLAG | XS_DONT_SCRIPT_FLAG;
	}
	else if (the->frame->next->flag & XS_SANDBOX_FLAG)
		aFlag = XS_DONT_ENUM_FLAG | XS_DONT_SCRIPT_FLAG;
	else
		aFlag = XS_DONT_ENUM_FLAG | XS_SANDBOX_FLAG;
		
	mxPush(mxArrayPrototype);
	anArray = fxNewArrayInstance(the);
	*mxResult = *(the->stack++);
	aContext.value.array.length = 0;
	aContext.value.array.address = anArray->next;
	fxEachOwnProperty(the, anInstance, aFlag, fxEnumProperty, &aContext);
	anArray->next->value.array.length = aContext.value.array.length;
	fxCacheArray(the, anArray);
}

void fx_Object_preventExtensions(txMachine* the)
{
	txSlot* anInstance;

	if (mxArgc < 1)
		mxDebug0(the, XS_TYPE_ERROR, "Object.preventExtensions: no instance parameter");
	anInstance = fxGetOwnInstance(the, mxArgv(0));
	if (!anInstance)
		mxDebug0(the, XS_TYPE_ERROR, "Object.preventExtensions: instance is no object");
	if (anInstance->flag & XS_SANDBOX_FLAG)
		anInstance = anInstance->value.instance.prototype;
	anInstance->flag |= XS_DONT_PATCH_FLAG;
	*mxResult = *mxArgv(0);
}

void fx_Object_seal(txMachine* the)
{
	txSlot* anInstance;

	if (mxArgc < 1)
		mxDebug0(the, XS_SYNTAX_ERROR, "Object.seal: no instance parameter");
	anInstance = fxGetOwnInstance(the, mxArgv(0));
	if (!anInstance)
		mxDebug0(the, XS_TYPE_ERROR, "Object.seal: instance is no object");
	if (anInstance->flag & XS_SANDBOX_FLAG)
		anInstance = anInstance->value.instance.prototype;
	fxEachOwnProperty(the, anInstance, XS_NO_FLAG, fxSealProperty, mxResult);
	anInstance->flag |= XS_DONT_PATCH_FLAG;
}

void fxDefineProperty(txMachine* the, txSlot* theInstance, txID theID, txSlot* theDescriptor)
{
	txSlot* configurable = C_NULL;
	txSlot* enumerable = C_NULL;
	txSlot* get = C_NULL;
	txSlot* set = C_NULL;
	txSlot* value = C_NULL;
	txSlot* writable = C_NULL;
	txSlot* getFunction = C_NULL;
	txSlot* setFunction = C_NULL;
	txSlot* aProperty;
	txFlag aFlag = XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG;

	theDescriptor = fxGetInstance(the, theDescriptor);
	if (theDescriptor) {
		configurable = fxGetProperty(the, theDescriptor, the->configurableID);
		enumerable = fxGetProperty(the, theDescriptor, the->enumerableID);
		get = fxGetProperty(the, theDescriptor, the->getID);
		set = fxGetProperty(the, theDescriptor, the->setID);
		value = fxGetProperty(the, theDescriptor, the->valueID);
		writable = fxGetProperty(the, theDescriptor, the->writableID);
	}
	if (get) {
		if (get->kind != XS_UNDEFINED_KIND) {
			getFunction = fxGetInstance(the, get);
			if (!getFunction || !mxIsFunction(getFunction))
				mxDebug0(the, XS_TYPE_ERROR, "Object.defineProperty: get is no function");
		}
		if (value)
			mxDebug0(the, XS_TYPE_ERROR, "Object.defineProperty: get and value");
		if (writable)
			mxDebug0(the, XS_TYPE_ERROR, "Object.defineProperty: get and writable");
	}
	if (set) {
		if (set->kind != XS_UNDEFINED_KIND) {
			setFunction = fxGetInstance(the, set);
			if (!setFunction || !mxIsFunction(setFunction))
				mxDebug0(the, XS_TYPE_ERROR, "Object.defineProperty: set is no function");
		}
		if (value)
			mxDebug0(the, XS_TYPE_ERROR, "Object.defineProperty: set and value");
		if (writable)
			mxDebug0(the, XS_TYPE_ERROR, "Object.defineProperty: set and writable");
	}
	aProperty = fxGetOwnProperty(the, theInstance, theID);
	if (aProperty) {
		if (aProperty->flag & XS_DONT_DELETE_FLAG) {
			if (configurable) {
				if (fxToBoolean(the, configurable)) goto not_configurable;
				configurable = C_NULL;
			}
			if (enumerable) {
				if (fxToBoolean(the, enumerable)) {
					if (aProperty->flag & XS_DONT_ENUM_FLAG) goto not_configurable;
				}
				else {
					if (!(aProperty->flag & XS_DONT_ENUM_FLAG)) goto not_configurable;
				}
				enumerable = C_NULL;
			}
			if (get || set) {
				if (aProperty->kind != XS_ACCESSOR_KIND) goto not_configurable;
				if (get) {
					if (aProperty->value.accessor.getter != getFunction) goto not_configurable;
					get = C_NULL;
				}	
				if (set) {
					if (aProperty->value.accessor.setter != setFunction) goto not_configurable;
					set = C_NULL;
				}	
			}
			else if (value || writable) {
				if (aProperty->kind == XS_ACCESSOR_KIND) goto not_configurable;
				if (aProperty->flag & XS_DONT_SET_FLAG) {
					if (writable) {
						if (fxToBoolean(the, writable)) goto not_configurable; 
						writable = C_NULL;
					}
					if (value) {
						if (!fxIsSameSlot(the, aProperty, value)) goto not_configurable;
						value = C_NULL;
					}
				}
			}
		}
		
	}
	else {
		aProperty = fxSetProperty(the, theInstance, theID, &aFlag);
		if (!aProperty)
			mxDebug0(the, XS_TYPE_ERROR, "Object.defineProperty: instance is not extensible");
	}
	if (configurable) {
		if (fxToBoolean(the, configurable))
			aProperty->flag &= ~XS_DONT_DELETE_FLAG;
		else
			aProperty->flag |= XS_DONT_DELETE_FLAG;
	}
	if (enumerable) {
		if (fxToBoolean(the, enumerable))
			aProperty->flag &= ~XS_DONT_ENUM_FLAG;
		else
			aProperty->flag |= XS_DONT_ENUM_FLAG;
	}
	if (get || set) {
		if (aProperty->kind != XS_ACCESSOR_KIND) {
			aProperty->kind = XS_ACCESSOR_KIND;
			aProperty->value.accessor.getter = C_NULL;
			aProperty->value.accessor.setter = C_NULL;
		}
		if (get)
			aProperty->value.accessor.getter = getFunction;
		if (set)
			aProperty->value.accessor.setter = setFunction;
	}
	else if (value || writable) {
		if (writable) {
			if (fxToBoolean(the, writable))
				aProperty->flag &= ~XS_DONT_SET_FLAG;
			else
				aProperty->flag |= XS_DONT_SET_FLAG;
		}
		if (value) {
			aProperty->kind = value->kind;
			aProperty->value = value->value;
		}
		else if (aProperty->kind == XS_ACCESSOR_KIND) {
			aProperty->kind = XS_UNDEFINED_KIND;
		}
	}
	return;
not_configurable:
	mxDebug0(the, XS_TYPE_ERROR, "Object.defineProperty: property is not configurable");
}

void fxEnumProperty(txMachine* the, txSlot* theContext, txID theID, txSlot* theProperty) 
{
	txSlot* anItem = theContext->value.array.address->next = fxNewSlot(the);
	char aBuffer[16];
	if (theID < 0) {
		txSlot* aSymbol = fxGetSymbol(the, theID);
		anItem->kind = XS_STRING_KIND;
		anItem->value.string = aSymbol->value.symbol.string;
	}
	else
		fxCopyStringC(the, anItem, fxIntegerToString(theID, aBuffer, sizeof(aBuffer)));
	theContext->value.array.address = anItem;
	theContext->value.array.length++;
}

void fxFreezeProperty(txMachine* the, txSlot* theResult, txID theID, txSlot* theProperty)
{
	if (theProperty->kind != XS_ACCESSOR_KIND) 
		theProperty->flag |= XS_DONT_SET_FLAG;
	theProperty->flag |= XS_DONT_DELETE_FLAG;
}

void fxIsPropertyFrozen(txMachine* the, txSlot* theResult, txID theID, txSlot* theProperty)
{
	if (theProperty->kind != XS_ACCESSOR_KIND) 
		if (!(theProperty->flag & XS_DONT_SET_FLAG))
			theResult->value.boolean = 0;
	if (!(theProperty->flag & XS_DONT_DELETE_FLAG))
		theResult->value.boolean = 0;
}

void fxIsPropertySealed(txMachine* the, txSlot* theResult, txID theID, txSlot* theProperty)
{
	if (!(theProperty->flag & XS_DONT_DELETE_FLAG))
		theResult->value.boolean = 0;
}

void fxSealProperty(txMachine* the, txSlot* theResult, txID theID, txSlot* theProperty)
{
	theProperty->flag |= XS_DONT_DELETE_FLAG;
}

