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

#ifndef mxReport
#define mxReport 0
#endif

static void fx_require(txMachine* the);
static void fx_require_get_busy(txMachine* the);
static void fx_require_set_busy(txMachine* the);
static void fx_require_resolve(txMachine* the);
static void fx_require_weak(txMachine* the);

static void fx_Module(txMachine* the);
static void fx_Module_prototype_get_id(txMachine* the);
static void fx_Module_prototype_get_uri(txMachine* the);
static void fx_Module_prototype_get___dirname(txMachine* the);
static void fx_Module_prototype_get___filename(txMachine* the);

static void fx_Transfer(txMachine* the);

static txSlot* fxGetModule(txMachine* the, txID moduleID);
static void fxImportModule(txMachine* the, txID moduleID, txSlot* name);
static void fxOrderModule(txMachine* the, txSlot* module);

static txSlot* fxQueueModule(txMachine* the, txID moduleID, txSlot* name);
static void fxRecurseExports(txMachine* the, txID moduleID, txSlot* circularities, txSlot* exports);
static void fxResolveExports(txMachine* the, txSlot* module);
static void fxResolveLocals(txMachine* the, txSlot* module);
static void fxResolveModules(txMachine* the);
static void fxResolveTransfer(txMachine* the, txID fromID, txID importID, txSlot* transfer);
static void fxRunModules(txMachine* the);
static void fxSetModule(txMachine* the, txID moduleID, txSlot* module);

void fxBuildModule(txMachine* the)
{
	txSlot* slot;
	
	slot = fxLastProperty(the, fxNewHostFunctionGlobal(the, fx_require, 1, mxID(_require), XS_DONT_ENUM_FLAG));
	slot = fxNextHostAccessorProperty(the, slot, fx_require_get_busy, fx_require_set_busy, mxID(_busy), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, fx_require_resolve, 1, mxID(_resolve), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, fx_require_weak, 1, mxID(_weak), XS_DONT_ENUM_FLAG);
	the->stack++;
	
	mxPush(mxObjectPrototype);
	slot = fxLastProperty(the, fxNewObjectInstance(the));
	slot = fxNextHostAccessorProperty(the, slot, fx_Module_prototype_get_id, C_NULL, mxID(_id), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG);
	slot = fxNextHostAccessorProperty(the, slot, fx_Module_prototype_get_uri, C_NULL, mxID(_uri), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG);
	slot = fxNextHostAccessorProperty(the, slot, fx_Module_prototype_get___dirname, C_NULL, mxID(___dirname), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG);
	slot = fxNextHostAccessorProperty(the, slot, fx_Module_prototype_get___filename, C_NULL, mxID(___filename), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG);
	mxModulePrototype = *the->stack;
	fxNewHostConstructor(the, fx_Module, 1, XS_NO_ID);
	mxPull(mxModuleConstructor);
	
	mxPush(mxObjectPrototype);
	fxNewObjectInstance(the);
	mxTransferPrototype = *the->stack;
	fxNewHostConstructor(the, fx_Transfer, 1, XS_NO_ID);
	mxPull(mxTransferConstructor);
}

txID fxCurrentModuleID(txMachine* the)
{
	txSlot* frame = the->frame;
	while (frame) {
		txSlot* function = frame + 3;
		if (function->kind == XS_REFERENCE_KIND) {
			txSlot* module = mxFunctionInstanceModule(function->value.reference);
            if (module->kind == XS_REFERENCE_KIND) {
                txSlot* slot = mxModuleURI(module);
				return slot->value.ID;
            }
		}
		frame = frame->next;
	}
	return XS_NO_ID;
}

txSlot* fxRequireModule(txMachine* the, txID moduleID, txSlot* name)
{
	txSlot* module;
	moduleID = fxFindModule(the, moduleID, name);
	if (moduleID == XS_NO_ID) {
		fxToStringBuffer(the, name, the->nameBuffer, sizeof(the->nameBuffer));
		mxReferenceError("module \"%s\" not found", the->nameBuffer);
	}
	module = fxGetModule(the, moduleID);
	if (module)
		return module;
	module = fxGetOwnProperty(the, mxLoadedModules.value.reference, moduleID);
	if (!module) {
		module = fxGetOwnProperty(the, mxLoadingModules.value.reference, moduleID);
		if (!module) {
			module = fxQueueModule(the, moduleID, name);
		}
	}
	while ((module = mxLoadingModules.value.reference->next)) {
		#if mxReport
			fxReport(the, "# Loading module \"%s\"\n", fxGetKey(the, module->ID)->value.key.string);
		#endif
		fxLoadModule(the, module->ID);
	}
	fxResolveModules(the);
	fxRunModules(the);
	mxImportingModules.value.reference->next = C_NULL;
	return fxGetModule(the, moduleID);
}

void fxResolveModule(txMachine* the, txID moduleID, txScript* script, void* data, txDestructor destructor)
{
	txSlot** fromAddress = &(mxLoadingModules.value.reference->next);
	txSlot** toAddress = &(mxLoadedModules.value.reference->next);
	txSlot* module;
	txSlot* slot;
	txSlot* transfer;
	txSlot* from;
	while ((module = *fromAddress)) {
		if (module->ID == moduleID) {
			*fromAddress = module->next;
			module->next = C_NULL;
			break;
		}
        fromAddress = &(module->next);
	}
	mxPushClosure(module);
	slot = mxModuleHost(module);
	slot->value.host.data = data;
	slot->value.host.variant.destructor = destructor;
	fxRunScript(the, script, module, C_NULL, C_NULL, module);
	the->stack++;
	while ((module = *toAddress))
        toAddress = &(module->next);
	*toAddress = module = the->stack->value.closure;
	the->stack++;
	transfer = mxModuleTransfers(module)->value.reference->next;
	while (transfer) {
		from = mxTransferFrom(transfer);
		if (from->kind != XS_NULL_KIND) {
			fxImportModule(the, module->ID, from);
			if (from->kind != XS_SYMBOL_KIND) {
				txString path = C_NULL;
				txInteger line = 0;
			#ifdef mxDebug
				slot = mxTransferClosure(transfer)->next;
				if (slot) {
					path = slot->value.string;
					line = slot->next->value.integer;
				}
			#endif	
				fxToStringBuffer(the, from, the->nameBuffer, sizeof(the->nameBuffer));
				fxThrowMessage(the, path, line, XS_REFERENCE_ERROR, "module \"%s\" not found", the->nameBuffer); 
			}
		}
		transfer = transfer->next;
	}
}

void fx_require_get_busy(txMachine* the)
{
	txID moduleID = fxCurrentModuleID(the);
	txSlot* property = fxGetOwnProperty(the, mxRequiredModules.value.reference, moduleID);
	mxResult->value.boolean = property ? 1 : 0;
	mxResult->kind = XS_BOOLEAN_KIND;
}

void fx_require_set_busy(txMachine* the)
{
	txID moduleID = fxCurrentModuleID(the);
	mxPushSlot(mxArgv(0));
	if (fxRunTest(the)) {
		txSlot* module = fxGetModule(the, moduleID);
		txSlot* property = fxSetProperty(the, mxRequiredModules.value.reference, module->ID, C_NULL);
		property->value = module->value;
		property->kind = XS_REFERENCE_KIND;
	}
	else {
		fxRemoveProperty(the, mxRequiredModules.value.reference, moduleID);
	}
}

void fx_require(txMachine* the)
{
	txSlot* module;
	txID defaultID;
	txSlot* export;
	mxTry(the) {
		if (mxArgc == 0)
			mxSyntaxError("no module id");
		fxToString(the, mxArgv(0));
		the->requireFlag |= XS_REQUIRE_FLAG;
		module = fxRequireModule(the, fxCurrentModuleID(the), mxArgv(0));
		
		defaultID = mxID(__default_);
		export = mxModuleExports(module)->value.reference->next;
		while (export) {
			if (export->ID == defaultID) {
				txSlot* closure = mxTransferClosure(export);
				mxResult->kind = closure->value.closure->kind;
				mxResult->value = closure->value.closure->value;
				break;
			}
			export = export->next;
		}
				
		the->requireFlag &= ~XS_REQUIRE_FLAG;
	}
	mxCatch(the) {
		the->requireFlag &= ~XS_REQUIRE_FLAG;
		fxJump(the);
	}
}

void fx_require_resolve(txMachine* the)
{
	txID moduleID;
	txSlot* key;
	if (mxArgc == 0)
		mxSyntaxError("no module id");
	fxToString(the, mxArgv(0));
	moduleID = fxFindModule(the, fxCurrentModuleID(the), mxArgv(0));
	if (moduleID == XS_NO_ID)
		return;
	key = fxGetKey(the, moduleID);
	if (key->kind == XS_KEY_KIND)
		mxResult->kind = XS_STRING_KIND;
	else
		mxResult->kind = XS_STRING_X_KIND;
	mxResult->value.string = key->value.key.string;
}

void fx_require_weak(txMachine* the)
{
	mxTry(the) {
		if (mxArgc == 0)
			mxSyntaxError("no module id");
		the->requireFlag |= XS_REQUIRE_WEAK_FLAG;
		
		mxPushSlot(mxArgv(0));
		mxPushInteger(1);
		mxPushUndefined();
		mxPushSlot(mxThis);
		fxCall(the);
		mxPullSlot(mxResult);
		
		the->requireFlag &= ~XS_REQUIRE_WEAK_FLAG;
	}
	mxCatch(the) {
		the->requireFlag &= ~XS_REQUIRE_WEAK_FLAG;
		fxJump(the);
	}
}

void fx_Module(txMachine* the)
{
	txInteger c = mxArgc, i;
	txSlot* slot;
	txSlot* property;
	slot = mxArgv(0);
	property = mxModuleFunction(mxThis);	
	property->kind = slot->kind;
	property->value = slot->value;
	mxPush(mxObjectPrototype);
	slot = fxLastProperty(the, fxNewObjectInstance(the));
	for (i = 1; i < c; i++)
		slot = fxNextSlotProperty(the, slot, mxArgv(i), XS_NO_ID, XS_DONT_ENUM_FLAG);
	property = mxModuleTransfers(mxThis);	
	mxPullSlot(property);
}

void fx_Module_prototype_get_id(txMachine* the)
{
	txSlot* slot = mxModuleID(mxThis);
	mxResult->kind = slot->kind;
	mxResult->value = slot->value;
}

void fx_Module_prototype_get_uri(txMachine* the)
{
	txSlot* slot = mxModuleURI(mxThis);
	txSlot* key = fxGetKey(the, slot->value.ID);
	if (key->kind == XS_KEY_KIND)
		mxResult->kind = XS_STRING_KIND;
	else
		mxResult->kind = XS_STRING_X_KIND;
	mxResult->value.string = key->value.key.string;
}

void fx_Module_prototype_get___dirname(txMachine* the)
{
	txSlot* slot = mxModuleURI(mxThis);
	txSlot* key = fxGetKey(the, slot->value.ID);
	if (key->kind == XS_KEY_KIND)
		mxResult->kind = XS_STRING_KIND;
	else
		mxResult->kind = XS_STRING_X_KIND;
	mxResult->value.string = key->value.key.string;
	mxPushInteger(7);
	mxPushStringC("/");
	mxPushInteger(1);
	mxPushSlot(mxResult);
	fxCallID(the, mxID(_lastIndexOf));
	mxPushInteger(2);
	mxPushSlot(mxResult);
	fxCallID(the, mxID(_slice));
	mxPullSlot(mxResult);
}

void fx_Module_prototype_get___filename(txMachine* the)
{
	txSlot* slot = mxModuleURI(mxThis);
	txSlot* key = fxGetKey(the, slot->value.ID);
	if (key->kind == XS_KEY_KIND)
		mxResult->kind = XS_STRING_KIND;
	else
		mxResult->kind = XS_STRING_X_KIND;
	mxResult->value.string = key->value.key.string;
	mxPushInteger(7);
	mxPushInteger(1);
	mxPushSlot(mxResult);
	fxCallID(the, mxID(_slice));
	mxPullSlot(mxResult);
}

void fx_Transfer(txMachine* the)
{
	txInteger c = mxArgc, i;
	txSlot* property;
	txSlot* slot;
	property = fxLastProperty(the, mxThis->value.reference);
	property = fxNextSlotProperty(the, property, mxArgv(0), mxID(_local), XS_DONT_ENUM_FLAG);
	property = fxNextSlotProperty(the, property, mxArgv(1), mxID(_from), XS_DONT_ENUM_FLAG);
	property = fxNextSlotProperty(the, property, mxArgv(2), mxID(_import), XS_DONT_ENUM_FLAG);
	if (c > 3) {
		mxPush(mxObjectPrototype);
		slot = fxLastProperty(the, fxNewObjectInstance(the));
		for (i = 3; i < c; i++)
			slot = fxNextSlotProperty(the, slot, mxArgv(i), XS_NO_ID, XS_DONT_ENUM_FLAG);
		property = fxNextSlotProperty(the, property, the->stack, mxID(_aliases), XS_DONT_ENUM_FLAG);
		the->stack++;
	}
	else {
		property = fxNextNullProperty(the, property, mxID(_aliases), XS_DONT_ENUM_FLAG);
	}
	property = fxNextNullProperty(the, property, mxID(_closure), XS_DONT_ENUM_FLAG);
#ifdef mxDebug
	slot = the->frame->next;
	if (slot) {
		slot = slot - 1;
		if (slot->next) {
			property = fxNextSlotProperty(the, property, slot->next, XS_NO_ID, XS_DONT_ENUM_FLAG);
			property = fxNextIntegerProperty(the, property, slot->ID, XS_NO_ID, XS_DONT_ENUM_FLAG);
		}
	}
#endif
}

txSlot* fxGetModule(txMachine* the, txID moduleID)
{
	txSlot* table = mxModules.value.reference->next;
	txSlot* key = fxGetKey(the, moduleID);
	txU4 sum = key->value.key.sum;
	txU4 modulo = sum % table->value.table.length;
	txSlot** address = &(table->value.table.address[modulo]);
	txSlot* entry;
	while ((entry = *address)) {
		if (entry->value.entry.sum == sum) {
			txSlot* result = entry->value.entry.slot;
			if (result->ID == moduleID)
				return result;
		}
		address = &entry->next;
	}
	return C_NULL;
}

void fxImportModule(txMachine* the, txID moduleID, txSlot* name)
{
	txSlot* module;
	moduleID = fxFindModule(the, moduleID, name);
	if (moduleID == XS_NO_ID)
		return;
	module = fxGetModule(the, moduleID);
	if (module) {
		txSlot* slot = fxNewSlot(the);
		slot->next = mxImportingModules.value.reference->next;
		slot->kind = module->kind;
		slot->value = module->value;
		mxImportingModules.value.reference->next = slot;
	}
	else {
		module = fxGetOwnProperty(the, mxLoadedModules.value.reference, moduleID);
		if (!module) {
			module = fxGetOwnProperty(the, mxLoadingModules.value.reference, moduleID);
			if (!module) {
				module = fxQueueModule(the, moduleID, name);
			}
		}
	}
	name->kind = XS_SYMBOL_KIND;
	name->value.ID = moduleID;
}

void fxOrderModule(txMachine* the, txSlot* module)
{
	txSlot* transfer = mxModuleTransfers(module)->value.reference->next;
	txSlot** fromAddress = &(mxLoadedModules.value.reference->next);
	txSlot** toAddress = &(mxResolvingModules.value.reference->next);
	txSlot* from;
	txSlot* to;
	while ((from = *fromAddress)) {
		if (from == module) {
			*fromAddress = module->next;
			module->next = C_NULL;
			break;
		}
        fromAddress = &(from->next);
	}
	while (transfer) {
		from = mxTransferFrom(transfer);
		if (from->kind != XS_NULL_KIND) {
			to = mxLoadedModules.value.reference->next;
			while (to) {
				if (to->ID == from->value.ID)
					break;
				to = to->next;
			}
			if (to)
				fxOrderModule(the, to);
		}
		transfer = transfer->next;
	}
	while ((to = *toAddress)) {
		if (to->ID == module->ID)
			return;
        toAddress = &(to->next);
	}
	*toAddress = module;
}

txSlot* fxQueueModule(txMachine* the, txID moduleID, txSlot* name)
{
	txSlot** address = &(mxLoadingModules.value.reference->next);
	txSlot* slot;
	while ((slot = *address)) {
		if (slot->ID == moduleID) {
			break;
		}
		address = &(slot->next);
	}
	mxCheck(the, slot == C_NULL);
	
	mxPush(mxModulePrototype);
	slot = fxNewObjectInstance(the);
	slot->flag |= XS_VALUE_FLAG;
	/* HOST */
	slot = slot->next = fxNewSlot(the);
	slot->kind = XS_MODULE_KIND;
	slot->value.host.data = C_NULL;
	slot->value.host.variant.destructor = C_NULL;
	/* FUNCTION */
	slot = fxNextNullProperty(the, slot, XS_NO_ID, XS_DONT_ENUM_FLAG);
	/* TRANSFERS */
	slot = fxNextNullProperty(the, slot, XS_NO_ID, XS_DONT_ENUM_FLAG);
	/* EXPORTS */
	slot = fxNextNullProperty(the, slot, XS_NO_ID, XS_DONT_ENUM_FLAG);
	/* ID */
	slot = fxNextSlotProperty(the, slot, name, XS_NO_ID, XS_DONT_ENUM_FLAG);
	/* URI */
	slot = fxNextSymbolProperty(the, slot, moduleID, XS_NO_ID, XS_DONT_ENUM_FLAG);
	
	*address = slot = fxNewSlot(the);
	slot->ID = moduleID;
	slot->kind = the->stack->kind;
	slot->value = the->stack->value;
	the->stack++;
	return slot;
}

void fxRecurseExports(txMachine* the, txID moduleID, txSlot* circularities, txSlot* exports)
{
	txSlot* circularitiesInstance;
	txSlot* circularity;
	txSlot* module;
	txSlot* transfers;
	txSlot* exportsInstance;
	txSlot* export;
	txSlot* transfer;
	txSlot* aliases;
	txSlot* alias;
	txSlot* local;
	txSlot* circularitiesCopy;
	txSlot* circularityCopy;
	txSlot* from;
	txSlot* stars;
	txSlot* star;
	circularitiesInstance = circularities->value.reference;
	if (fxGetOwnProperty(the, circularitiesInstance, moduleID))
		return;
	circularity = fxNewSlot(the);
	circularity->next = circularitiesInstance->next;
	circularity->ID = moduleID;
	circularitiesInstance->next = circularity;
	module = fxGetModule(the, moduleID);
	exportsInstance = exports->value.reference;
	export = exportsInstance;
	transfers = mxModuleExports(module);
	if (transfers->kind == XS_REFERENCE_KIND) {
		transfer = transfers->value.reference->next;
		while (transfer) {
			export = export->next = fxNewSlot(the);
			export->ID = transfer->ID;
			export->kind = transfer->kind;
			export->value = transfer->value;
			transfer = transfer->next;
		}
		return;
	}
	transfers = mxModuleTransfers(module);
	if (transfers->kind == XS_REFERENCE_KIND) {
		transfer = transfers->value.reference->next;
		while (transfer) {
			aliases = mxTransferAliases(transfer);
			if (aliases->kind == XS_REFERENCE_KIND) {
				alias = aliases->value.reference->next;
				while (alias) {
					export = export->next = fxNewSlot(the);
					export->ID = alias->value.ID;
					export->kind = XS_REFERENCE_KIND;
					export->value.reference = transfer->value.reference;
					alias = alias->next;
				}
			}
			transfer = transfer->next;
		}
		transfer = transfers->value.reference->next;
		while (transfer) {
			local = mxTransferLocal(transfer);
			aliases = mxTransferAliases(transfer);
			if ((local->kind == XS_NULL_KIND) && (aliases->kind == XS_NULL_KIND)) {
				from = mxTransferFrom(transfer);
				fxNewInstance(the);
				circularitiesCopy = the->stack;
				circularity = circularitiesInstance->next;
				circularityCopy = circularitiesCopy->value.reference;
				while (circularity) {
					circularityCopy = circularityCopy->next = fxNewSlot(the);
					circularityCopy->ID = circularity->ID;
					circularity = circularity->next;
				}
				fxNewInstance(the);
				stars = the->stack;
				fxRecurseExports(the, from->value.ID, circularitiesCopy, stars);
				star = stars->value.reference->next;
				while (star) {
					if (star->ID != mxID(_default)) {
						if (!fxGetOwnProperty(the, exportsInstance, star->ID)) {
							export = export->next = fxNewSlot(the);
							export->ID = star->ID;
							export->kind = XS_REFERENCE_KIND;
							export->value.reference = star->value.reference;
						}
					}
					star = star->next;
				}
				the->stack++;
				the->stack++;
			}
			transfer = transfer->next;
		}
	}
}

void fxResolveExports(txMachine* the, txSlot* module)
{
	txSlot* transfer;
	txSlot* from;
	txSlot* import;

	transfer = mxModuleExports(module)->value.reference->next;
	while (transfer) {
		from = mxTransferFrom(transfer);
		if (from->kind != XS_NULL_KIND) {
			import = mxTransferImport(transfer);
			if (import->kind != XS_NULL_KIND) {
				fxResolveTransfer(the, from->value.ID, import->value.ID, transfer);
			}
		}
		transfer = transfer->next;
	}
}

void fxResolveLocals(txMachine* the, txSlot* module)
{
	txSlot* transfer;
	txSlot* local;
	txSlot* from;
	txSlot* import;
	txSlot* export;
	txSlot* closure;
	txSlot* property;

	transfer = mxModuleTransfers(module)->value.reference->next;
	while (transfer) {
		local = mxTransferLocal(transfer);
		if (local->kind != XS_NULL_KIND) {
			from = mxTransferFrom(transfer);
			if (from->kind != XS_NULL_KIND) {
				import = mxTransferImport(transfer);
				if (import->kind != XS_NULL_KIND) {
					fxResolveTransfer(the, from->value.ID, import->value.ID, transfer);
				}
				else {
					txSlot* slot = fxGetModule(the, from->value.ID);
					export = mxModuleExports(slot)->value.reference->next;
					property = mxTransferClosure(transfer)->value.closure->value.reference->next;
					while (export) {
						closure = mxTransferClosure(export);
						property = property->next = fxNewSlot(the);	
						property->ID = export->ID;
						property->kind = closure->kind;
						property->value = closure->value;
						export = export->next;
					}
				}
			}
		}
		transfer = transfer->next;
	}
}

void fxResolveModules(txMachine* the)
{
	txSlot* modules;
	txSlot* module;
	txSlot* transfer;
	txSlot* local;
	txSlot* from;
	txSlot* import;
	txSlot* closure;
	txSlot* instance;
	txSlot* property;
	txSlot* reference;
	txSlot* circularities;
	txSlot* exports;
	txSlot* closures;
	txSlot* object;
	
	modules = mxLoadedModules.value.reference;
	while ((module = modules->next)) {
		fxOrderModule(the, module);
	}
	modules = mxResolvingModules.value.reference;
	
	module = modules->next;
	while (module) {
		fxIDToString(the, module->ID, the->nameBuffer, sizeof(the->nameBuffer));
		#if mxReport
			fxReport(the, "# Resolving module \"%s\"\n", the->nameBuffer);
		#endif
		module = module->next;
	}
	
	module = modules->next;
	while (module) {
		transfer = mxModuleTransfers(module)->value.reference->next;
		while (transfer) {
			local = mxTransferLocal(transfer);
			if (local->kind != XS_NULL_KIND) {
				from = mxTransferFrom(transfer);
				import = mxTransferImport(transfer);
				if (from->kind == XS_NULL_KIND) {
					closure = mxTransferClosure(transfer);
					closure->value.closure = fxNewSlot(the);
                    closure->kind = XS_CLOSURE_KIND;
				}
				else if (import->kind == XS_NULL_KIND) {
					closure = mxTransferClosure(transfer);
					instance = fxNewInstance(the);
					instance->flag = XS_VALUE_FLAG | XS_DONT_PATCH_FLAG;
  					property = instance->next = fxNewSlot(the);
					property->flag = XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG;
					property->kind = XS_STAR_KIND;
					property->value.reference = module->value.reference;
					reference = fxNewSlot(the);
					reference->value.reference = instance;
					reference->kind = XS_REFERENCE_KIND;
					closure->value.closure = reference;
                    closure->kind = XS_CLOSURE_KIND;
                    the->stack++;
				}
			}
			transfer = transfer->next;
		}
		fxSetModule(the, module->ID, module);
		if (!(the->requireFlag & XS_REQUIRE_WEAK_FLAG)) {
			property = fxSetProperty(the, mxRequiredModules.value.reference, module->ID, C_NULL);
			property->value = module->value;
			property->kind = XS_REFERENCE_KIND;
		}
		module = module->next;
	}

	module = modules->next;
	while (module) {
		fxNewInstance(the);
		circularities = the->stack;
		fxNewInstance(the);
		exports = the->stack;
		fxRecurseExports(the, module->ID, circularities, exports);
		mxModuleExports(module)->kind = XS_REFERENCE_KIND;
		mxModuleExports(module)->value.reference = exports->value.reference;
		the->stack++;
		the->stack++;
		module = module->next;
	}

	module = modules->next;
	while (module) {
		fxResolveExports(the, module);
		module = module->next;
	}
	module = modules->next;
	while (module) {
		fxResolveLocals(the, module);
		module = module->next;
	}
	
	module = modules->next;
	while (module) {
		txSlot* function = mxModuleFunction(module);
		if (function->kind == XS_REFERENCE_KIND) {
			closures = mxFunctionInstanceClosures(function->value.reference);
			fxNewInstance(the);
			closures->kind = the->stack->kind;
			closures->value = the->stack->value;
			closure = closures->value.reference;
			the->stack++;
			closure = closure->next = fxNewSlot(the);
			closure->kind = XS_WITH_KIND;
			closure->value.reference = C_NULL;
			transfer = mxModuleTransfers(module)->value.reference->next;
			while (transfer) {
				local = mxTransferLocal(transfer);
				if (local->kind != XS_NULL_KIND) {
					object = mxTransferClosure(transfer);
                    object->ID = local->value.ID;
					closure = closure->next = fxNewSlot(the);
					closure->ID = local->value.ID;
					closure->kind = object->kind;
					closure->value = object->value;
				}
				transfer = transfer->next;
			}
		}
		module = module->next;
	}
}

void fxResolveTransfer(txMachine* the, txID fromID, txID importID, txSlot* transfer)
{
	txSlot* module;
	txSlot* export;
	txSlot* exportClosure;
	txSlot* transferClosure;
	txSlot* from;
	txSlot* import;
	
	module = fxGetModule(the, fromID);
	export = fxGetOwnProperty(the, mxModuleExports(module)->value.reference, importID);
	if (export) {
		exportClosure = mxTransferClosure(export);
		if (exportClosure->kind != XS_NULL_KIND) {
			transferClosure = mxTransferClosure(transfer);
			transferClosure->value =  exportClosure->value;
			transferClosure->kind = exportClosure->kind;
		}
		else {
			from = mxTransferFrom(export);
			import = mxTransferImport(export);
			if ((from->value.ID != mxTransferFrom(transfer)->value.ID) || (import->value.ID != mxTransferImport(transfer)->value.ID))
				fxResolveTransfer(the, from->value.ID, import->value.ID, transfer);
		}
	}
	else {
		txString path = C_NULL;
		txInteger line = 0;
	#ifdef mxDebug
		txSlot* slot = mxTransferClosure(transfer)->next;
		if (slot) {
			path = slot->value.string;
			line = slot->next->value.integer;
		}
	#endif	
		fxIDToString(the, importID, the->nameBuffer, sizeof(the->nameBuffer));
		fxThrowMessage(the, path, line, XS_REFERENCE_ERROR, "import %s not found", the->nameBuffer);
	}
}

void fxRunModules(txMachine* the)
{
	txSlot* modules = fxNewInstance(the);
	txSlot* module = modules->next = mxResolvingModules.value.reference->next;
	mxResolvingModules.value.reference->next = C_NULL;
	while (module) {
		txSlot* function = mxModuleFunction(module);
		if (function->kind == XS_REFERENCE_KIND) {
			#if mxReport
				fxReport(the, "# Running module \"%s\"\n", fxGetKey(the, module->ID)->value.key.string);
			#endif
			mxCall(function, module, 0);
			the->stack++;
		}
		function->kind = XS_NULL_KIND;
		fxRemoveProperty(the, module->value.reference, mxID(0));
		module = module->next;
	}
	the->stack++;
}

void fxSetModule(txMachine* the, txID moduleID, txSlot* module)
{
	txSlot* table = mxModules.value.reference->next;
	txSlot* key = fxGetKey(the, moduleID);
	txU4 sum = key->value.key.sum;
	txU4 modulo = sum % table->value.table.length;
	txSlot** address = &(table->value.table.address[modulo]);
	txSlot* result;
	txSlot* entry;
    
    result = fxNewSlot(the);
    result->ID = moduleID;
    result->kind = module->kind;
    result->value = module->value;
	mxPushClosure(result);    
    
	entry = fxNewSlot(the);
	entry->next = *address;
	entry->kind = XS_ENTRY_KIND;
	entry->value.entry.slot = result;
	entry->value.entry.sum = sum;
	*address = entry;
	
	mxPop();
}
