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
#define __FSKECMASCRIPT_PRIV__

#include "FskMedia.h"

#include "FskECMAScript.h"
#include "FskUtilities.h"
#include "QTReader.h"

#include <stddef.h>

#if SUPPORT_INSTRUMENTATION
    FskInstrumentedSimpleType(Property, property);

    static void dumpInstrumentedProperty(const char *message, void *obj, UInt32 propertyID, FskMediaPropertyValue property);
    static void dumpInstrumentedMetaData(const char *message, FskMediaMetaData meta, const char *name, FskMediaPropertyValue property);

    static FskInstrumentedTypeRecord gMetaDataTypeInstrumentation = {
        NULL,
        sizeof(FskInstrumentedTypeRecord),
        "metadata",
        FskInstrumentationOffset(FskMediaMetaDataRecord),
        NULL,
        0,
        NULL,
        NULL
    };
#else
    #define dumpInstrumentedProperty(...)
    #define dumpInstrumentedMetaData(...)
#endif

static FskMediaPropertyEntry findProperty(FskMediaPropertyEntry walker, UInt32 propertyID);

FskErr FskMediaHasProperty(FskMediaPropertyEntry properties, UInt32 propertyID, Boolean *get, Boolean *set, UInt32 *dataType)
{
	FskMediaPropertyEntry property = findProperty(properties, propertyID);
	if (NULL == property) {
		*set = false;
		*get = false;
		*dataType = kFskMediaPropertyTypeUndefined;
	}
	else {
		*set = NULL != property->set;
		*get = NULL != property->get;
		*dataType = property->dataType;
	}
	return kFskErrNone;
}

FskErr FskMediaSetProperty(FskMediaPropertyEntry properties, void *state, void *obj, UInt32 propertyID, FskMediaPropertyValue property)
{
	FskMediaPropertyEntry prop = findProperty(properties, propertyID);
	if (prop && prop->set) {
#if SUPPORT_INSTRUMENTATION
        dumpInstrumentedProperty("Set", obj, propertyID, property);
        if (property->type != prop->dataType)
            FskInstrumentedTypePrintfMinimal(&gPropertyTypeInstrumentation, "WARNING: Set propertyID %u on obj %p. Expected type %u, passed %u", (unsigned int)propertyID, obj, (unsigned int)prop->dataType, (unsigned int)property->type);
#endif
        return (prop->set)(state, obj, propertyID, property);
    }
    else
        dumpInstrumentedProperty("Ignore set", obj, propertyID, property);
	return kFskErrUnimplemented;
}

FskErr FskMediaGetProperty(FskMediaPropertyEntry properties, void *state, void *obj, UInt32 propertyID, FskMediaPropertyValue property)
{
	FskMediaPropertyEntry prop = findProperty(properties, propertyID);
	if (prop && prop->get) {
		FskErr err = (prop->get)(state, obj, propertyID, property);
        if (kFskErrNone == err)
            dumpInstrumentedProperty("Get", obj, propertyID, property);
        return err;
    }
    else
        dumpInstrumentedProperty("Ignore get", obj, propertyID, NULL);
	return kFskErrUnimplemented;
}

static FskMediaPropertyEntry findProperty(FskMediaPropertyEntry walker, UInt32 propertyID)
{
	if (walker)
		for ( ; kFskMediaPropertyUndefined != walker->id; walker += 1)
			if (walker->id == propertyID)
				return walker;
	return NULL;
}

FskErr FskMediaPropertyCopy(const FskMediaPropertyValue from, FskMediaPropertyValue to)
{
	FskErr err = kFskErrNone;

	*to = *from;

	switch (from->type) {
		case kFskMediaPropertyTypeImage:
		case kFskMediaPropertyTypeData:
			err = FskMemPtrNewFromData(from->value.data.dataSize, from->value.data.data, (FskMemPtr*)(void*)&to->value.data.data);
			break;

		case kFskMediaPropertyTypeEditList:
			err = FskMemPtrNewFromData((from->value.editList->count * sizeof(FskMediaEditRecord)) + sizeof(UInt32),
											from->value.editList, (FskMemPtr*)(void*)&to->value.editList);
			break;

		case kFskMediaPropertyTypeString:
			to->value.str = FskStrDoCopy(from->value.str);
			break;

		case kFskMediaPropertyTypeStringList:
			err = FskStrListDoCopy(from->value.str, &to->value.str);
			break;

		case kFskMediaPropertyTypeUInt32List:
			err = FskMemPtrNewFromData(from->value.integers.count * sizeof(UInt32),
										from->value.integers.integer, (FskMemPtr*)(void*)&to->value.integers.integer);
			break;
		case kFskMediaPropertyTypeFloatList:
			err = FskMemPtrNewFromData(from->value.numbers.count * sizeof(double),
										from->value.numbers.number, (FskMemPtr*)(void*)&to->value.numbers.number);
			break;
	}

	if (err)
		FskMemSet(to, 0, sizeof(*to));

	return err;
}

FskErr FskMediaPropertyEmpty(FskMediaPropertyValue property)
{
	if (NULL == property)
		return kFskErrNone;

	switch (property->type) {
		case kFskMediaPropertyTypeImage:
		case kFskMediaPropertyTypeData:
			FskMemPtrDisposeAt((void **)&property->value.data.data);
			property->value.data.dataSize = 0;
			break;

		case kFskMediaPropertyTypeEditList:
			FskMemPtrDisposeAt((void**)(void*)&property->value.editList);
			break;

		case kFskMediaPropertyTypeString:
		case kFskMediaPropertyTypeStringList:
			FskMemPtrDisposeAt((void**)(void*)&property->value.str);
			break;

		case kFskMediaPropertyTypeUInt32List:
			FskMemPtrDisposeAt((void**)(void*)&property->value.integers.integer);
			break;

		case kFskMediaPropertyTypeFloatList:
			FskMemPtrDisposeAt((void**)(void*)&property->value.numbers.number);
			break;
	}

	property->type = kFskMediaPropertyTypeUndefined;

	return kFskErrNone;
}

/*
	xs helpers

*/

#ifdef KPR_CONFIG
#include "FskManifest.xs.h"
typedef void *FskECMAScript;
#define FskGetVM(_THE) NULL
#else
#define FskGetVM(_THE) (FskECMAScript)xsGetContext(_THE)
#define xsID_length vm->id_length	
#define xsID_x vm->id_x	
#define xsID_y vm->id_y	
#define xsID_width vm->id_width	
#define xsID_height vm->id_height	
#endif

FskErr FskMediaSetPropertyXSHelper(xsMachine *the, UInt32 dataType, UInt32 propertyID, xsIndex argIndex, FskMediaSetPropertyXSHelperProc set, void *obj)
{
	FskErr err = kFskErrNone;
	FskMediaPropertyValueRecord property;
	void *toDispose = NULL;
	FskECMAScript vm;

	switch (dataType) {
		case kFskMediaPropertyTypeInteger:
			property.value.integer = xsToInteger(xsArg(argIndex));
			break;

		case kFskMediaPropertyTypeFloat:
			property.value.number = xsToNumber(xsArg(argIndex));
			break;

		case kFskMediaPropertyTypeBoolean:
			property.value.b = xsToBoolean(xsArg(argIndex));
			break;

		case kFskMediaPropertyTypeString:
			property.value.str = xsToStringCopy(xsArg(argIndex));
			toDispose = property.value.str;
			break;

		case kFskMediaPropertyTypeData:
			vm = FskGetVM(the);
			property.value.data.data = xsGetHostData(xsArg(argIndex));
			property.value.data.dataSize = xsToInteger(xsGet(xsArg(argIndex), xsID_length));
			break;

		case kFskMediaPropertyTypeKey:
			property.value.key = (FskMediaDRMKey)xsGetHostData(xsArg(argIndex));
			break;

		case kFskMediaPropertyTypeEditList: {
			UInt32 count, i;
			FskMediaEditList editList;

			vm = FskGetVM(the);
			xsVars(1);
			count = xsToInteger(xsGet(xsArg(argIndex), xsID_length));
			FskMemPtrNew(sizeof(FskMediaEditListRecord) + sizeof(FskMediaEditRecord) * count, &editList);
			for (i = 0; i < count; i++) {
				xsVar(0) = xsGet(xsArg(argIndex), (xsIndex)i);
				editList->edit[i].movieDuration = xsToInteger(xsGet(xsVar(0), xsID("movieDuration")));
				editList->edit[i].rate = (SInt32)(xsToNumber(xsGet(xsVar(0), xsID("rate"))) * 65536.0);
				editList->edit[i].trackStartTime = xsToInteger(xsGet(xsVar(0), xsID("trackTime")));
			}
			editList->count = count;
			property.value.editList = editList;
			toDispose = editList;
			}
			break;

		case kFskMediaPropertyTypeMessage: {
			UInt32 i, count = xsToInteger(xsGet(xsGet(xsArg(argIndex), xsID("params")), xsID("length")));
			FskMediaMessage message;
			FskMemPtrNew(sizeof(FskMediaMessageRecord) + (sizeof(FskMediaMessageRecord) * count), &message);
			FskStrNCopy(message->str, xsToString(xsGet(xsArg(argIndex), xsID("message"))), sizeof(message->str));
			message->paramCount = count;
			for (i = 0; i < message->paramCount; ++i) {
				message->param[i] = xsToInteger(xsGet(xsGet(xsArg(argIndex), xsID("params")), (xsIndex)i));
			}
			property.value.message = message;
			toDispose = message;
			}
			break;

		case kFskMediaPropertyTypeXSObject: {
			FskMediaXSObject xsObject;
			FskMemPtrNew(sizeof(FskMediaXSObjectRecord), &xsObject);
			xsObject->object = xsArg(argIndex);
			xsObject->the = the;
			toDispose = xsObject;
			property.value.xsObject = xsObject;
			}
			break;

		case kFskMediaPropertyTypeRectangle:
			vm = FskGetVM(the);
			property.value.rect.x = xsToInteger(xsGet(xsArg(argIndex), xsID_x));
			property.value.rect.y = xsToInteger(xsGet(xsArg(argIndex), xsID_y));
			property.value.rect.width = xsToInteger(xsGet(xsArg(argIndex), xsID_width));
			property.value.rect.height = xsToInteger(xsGet(xsArg(argIndex), xsID_height));
			break;

		case kFskMediaPropertyTypePoint:
			vm = FskGetVM(the);
			property.value.point.x = xsToInteger(xsGet(xsArg(argIndex), xsID_x));
			property.value.point.y = xsToInteger(xsGet(xsArg(argIndex), xsID_y));
			break;

		case kFskMediaPropertyTypeDimension:
			vm = FskGetVM(the);
			property.value.dimension.width = xsToInteger(xsGet(xsArg(argIndex), xsID_width));
			property.value.dimension.height = xsToInteger(xsGet(xsArg(argIndex), xsID_height));
			break;

		case kFskMediaPropertyTypeStringList: {
			UInt32 size = 0;
			UInt32 length, i;
			vm = FskGetVM(the);
			property.value.str = NULL;

			if (!xsIsInstanceOf(xsArg(argIndex), xsArrayPrototype)) {
				// convert object properties to array
				xsArg(argIndex) = xsCall1(xsGet(xsGlobal, xsID("Media")), xsID("makeAA"), xsArg(argIndex));
			}

			length = xsToInteger(xsGet(xsArg(argIndex), xsID_length));
			for (i=0; i<length; i++) {
				char *str = xsToString(xsGet(xsArg(argIndex), (xsIndex)i));
				UInt32 len = FskStrLen(str) + 1;
				err = FskMemPtrRealloc(size + len + 1, (FskMemPtr*)(void*)&property.value.str);
				toDispose = property.value.str;
				BAIL_IF_ERR(err);
				FskMemMove(property.value.str + size, str, len);
				size += len;
                property.value.str[size] = 0;
			}
			}
			break;

		case kFskMediaPropertyTypeUInt32List: {
			xsIndex i;

			vm = FskGetVM(the);
			property.value.integers.count = xsToInteger(xsGet(xsArg(argIndex), xsID_length));
			err = FskMemPtrNew(sizeof(UInt32) * property.value.integers.count,
								(FskMemPtr*)(void*)&property.value.integers.integer);
			BAIL_IF_ERR(err);

			for (i = 0; i < (xsIndex)property.value.integers.count; i++)
				property.value.integers.integer[i] = xsToInteger(xsGet(xsArg(argIndex), i));
		}
		break;

		case kFskMediaPropertyTypeFloatList: {
			xsIndex i;

			vm = FskGetVM(the);
			property.value.numbers.count = xsToInteger(xsGet(xsArg(argIndex), xsID_length));
			err = FskMemPtrNew(sizeof(double) * property.value.numbers.count,
								(FskMemPtr*)(void*)&property.value.numbers.number);
			BAIL_IF_ERR(err);

			for (i = 0; i < (xsIndex)property.value.numbers.count; i++)
				property.value.numbers.number[i] = xsToNumber(xsGet(xsArg(argIndex), i));
		}
		break;

		case kFskMediaPropertyTypeRatio:
			property.value.ratio.numer = xsToInteger(xsGet(xsArg(argIndex), xsID("numer")));
			property.value.ratio.denom = xsToInteger(xsGet(xsArg(argIndex), xsID("denom")));
			break;

		case kFskMediaPropertyTypeSprites:
			property.value.sprites = xsGetHostData(xsArg(argIndex));
			break;
	}

	property.type = dataType;
	err = (set)(obj, propertyID, &property);
	BAIL_IF_ERR(err);

bail:
	FskMemPtrDispose(toDispose);

	return err;
}

FskErr FskMediaGetPropertyXSHelper(xsMachine *the, FskMediaPropertyValue property)
{
	FskErr err = kFskErrNone;
	FskECMAScript vm;

	switch (property->type) {
		case kFskMediaPropertyTypeInteger:
			xsResult = xsInteger(property->value.integer);
			break;

		case kFskMediaPropertyTypeFloat:
			xsResult = xsNumber(property->value.number);
			break;

		case kFskMediaPropertyTypeBoolean:
			xsResult = xsBoolean(property->value.b);
			break;

		case kFskMediaPropertyTypeString:
			if (NULL == property->value.str)
				return kFskErrUnimplemented;

			xsResult = xsString(property->value.str);
			break;

		case kFskMediaPropertyTypeData:
			xsResult = xsNew1(xsGlobal, xsID_ArrayBuffer, xsInteger(property->value.data.dataSize));
			FskMemMove(xsToArrayBuffer(xsResult), property->value.data.data, (SInt32)property->value.data.dataSize);
			property->value.data.data = NULL;
			break;

		case kFskMediaPropertyTypePoint:
			vm = FskGetVM(the);
#ifdef KPR_CONFIG
			xsResult = xsNewInstanceOf(xsObjectPrototype);
#else
			xsResult = xsNewInstanceOf(xsGet(xsGlobal, vm->id_Point));
#endif
			xsSet(xsResult, xsID_x, xsInteger(property->value.point.x));
			xsSet(xsResult, xsID_y, xsInteger(property->value.point.y));
			break;

		case kFskMediaPropertyTypeDimension:
			vm = FskGetVM(the);
#ifdef KPR_CONFIG
			xsResult = xsNewInstanceOf(xsObjectPrototype);
#else
			xsResult = xsNewInstanceOf(xsGet(xsGet(xsGlobal, xsID("UI")), xsID("dimensions")));
#endif
			xsNewHostProperty(xsResult, xsID_width, xsInteger(property->value.dimension.width), xsDefault, xsDontScript);
			xsNewHostProperty(xsResult, xsID_height, xsInteger(property->value.dimension.height), xsDefault, xsDontScript);
			break;

		case kFskMediaPropertyTypeRectangle:
			vm = FskGetVM(the);
#ifdef KPR_CONFIG
			xsResult = xsNewInstanceOf(xsObjectPrototype);
			xsNewHostProperty(xsResult, xsID_x, xsInteger(property->value.rect.x), xsDefault, xsDontScript);
			xsNewHostProperty(xsResult, xsID_y, xsInteger(property->value.rect.y), xsDefault, xsDontScript);
			xsNewHostProperty(xsResult, xsID_width, xsInteger(property->value.rect.width), xsDefault, xsDontScript);
			xsNewHostProperty(xsResult, xsID_height, xsInteger(property->value.rect.height), xsDefault, xsDontScript);
#else
			xsResult = xsNewInstanceOf(xsGet(xsGlobal, vm->id_Rectangle));
			fskRectangleToXSRectangle(the, &property->value.rect, &xsResult);
#endif
			break;

		case kFskMediaPropertyTypeEditList: {
			UInt32 i;
			xsResult = xsNewInstanceOf(xsArrayPrototype);
			xsVars(1);

			for (i=0; i<property->value.editList->count; i++) {
				xsVar(0) = xsNew3(xsGet(xsGlobal, xsID("Media")), xsID("Edit"),
								xsNumber(property->value.editList->edit[i].movieDuration),
								xsNumber(property->value.editList->edit[i].trackStartTime),
								xsNumber(property->value.editList->edit[i].rate / 65536.0));
				xsSet(xsResult, (xsIndex)i, xsVar(0));
			}
			}
			break;

		case kFskMediaPropertyTypeStringList: {
			char *d = property->value.str;
			xsResult = xsNewInstanceOf(xsArrayPrototype);
			if (d) {
				xsIndex i = 0;
				xsVars(1);

				while (*d) {
					UInt32 len = FskStrLen(d) + 1;
					xsVar(0) = xsString(d);
					xsSet(xsResult, i, xsVar(0));
					i++;
					d += len;
				}
			}
			}
			break;

		case kFskMediaPropertyTypeImage: {
			UInt32 mimeLen = FskStrLen((const char *)property->value.data.data) + 1;

			vm = FskGetVM(the);
#ifdef KPR_CONFIG
			xsResult = xsNewInstanceOf(xsChunkPrototype);
#else
			xsResult = xsNew0(xsGet(xsGet(xsGet(xsGet(xsGlobal, xsID("Media")), xsID("Codec")), xsID("Video")), xsID("compressor")), vm->id_Chunk);
#endif
			xsSet(xsResult, xsID("format"), xsString((char *)property->value.data.data));
			property->value.data.dataSize -= mimeLen;
			xsSet(xsResult, xsID_length, xsInteger(property->value.data.dataSize));
			FskMemMove(xsGetHostData(xsResult), mimeLen + (char *)property->value.data.data, property->value.data.dataSize);
			}
			break;

		case kFskMediaPropertyTypeRatio:
			xsResult = xsNewInstanceOf(xsGet(xsGet(xsGlobal, xsID("Media")), xsID("ratio")));
			xsSet(xsResult, xsID("numer"), xsInteger(property->value.ratio.numer));
			xsSet(xsResult, xsID("denom"), xsInteger(property->value.ratio.denom));
			break;

		case kFskMediaPropertyTypeUInt32List: {
			xsIndex i;

			xsResult = xsNewInstanceOf(xsArrayPrototype);
			for (i = 0; i < (xsIndex)property->value.integers.count; i++) {
				UInt32 value = property->value.integers.integer[i];
				if (((SInt32)value) >= 0)
					xsSet(xsResult, i, xsInteger(value));
				else
					xsSet(xsResult, i, xsNumber(value));
			}
			}
			break;

		case kFskMediaPropertyTypeFloatList: {
			xsIndex i;

			xsResult = xsNewInstanceOf(xsArrayPrototype);
			for (i = 0; i < (xsIndex)property->value.numbers.count; i++)
				xsSet(xsResult, i, xsNumber(property->value.numbers.number[i]));
			}
			break;

		default:
			err = kFskErrUnimplemented;
			break;
	}

	FskMediaPropertyEmpty(property);

	return err;
}

/*
	metadata helper
*/

static FskMediaMetaDataID findMediaMetaDataID(FskMediaMetaData meta, const char *idName);

FskErr FskMediaMetaDataNew(FskMediaMetaData *meta)
{
	FskErr err = FskMemPtrNewClear(sizeof(FskMediaMetaDataRecord), (FskMemPtr *)meta);
	FskInstrumentedItemNew(*meta, NULL, &gMetaDataTypeInstrumentation);
    return err;
}

FskErr FskMediaMetaDataDispose(FskMediaMetaData meta)
{
	if (NULL == meta)
		return kFskErrNone;

	while (meta->ids) {
		FskMediaMetaDataID id = meta->ids;
		meta->ids = id->next;

		while (id->items) {
			FskMediaMetaDataItem item = id->items;
			id->items = item->next;
			if (!(item->flags & kFskMediaMetaDataFlagReference))
				FskMediaPropertyEmpty(&item->value);
			FskMemPtrDispose(item);
		}
		FskMemPtrDispose(id);
	}

	FskInstrumentedItemDispose(meta);

	FskMemPtrDispose(meta);

	return kFskErrNone;
}

FskErr FskMediaMetaDataAdd(FskMediaMetaData meta, const char *idName, UInt32 *index, const FskMediaPropertyValue value, UInt32 flags)
{
	FskErr err;
	FskMediaMetaDataID id;
	FskMediaMetaDataItem item;

    dumpInstrumentedMetaData("Add", meta, idName, value);

	id = findMediaMetaDataID(meta, idName);
	if (NULL == id) {
		err = FskMemPtrNewClear(sizeof(FskMediaMetaDataIDRecord) + FskStrLen(idName) + 1, &id);
		BAIL_IF_ERR(err);

		FskStrCopy(id->idName, idName);

		FskListAppend((FskList*)(void*)&meta->ids, id);
	}

	err = FskMemPtrNew(sizeof(FskMediaMetaDataItemRecord), &item);
	BAIL_IF_ERR(err);

	item->next = NULL;
	if (flags & (kFskMediaMetaDataFlagReference | kFskMediaMetaDataFlagOwnIt))
		item->value = *value;
	else {
		err = FskMediaPropertyCopy(value, &item->value);
		if (err) {
			FskMemPtrDispose(item);
			goto bail;
		}
	}
	item->flags = flags;

	FskListAppend((FskList*)(void*)&id->items, item);

	if (index)
		*index = FskListCount(id->items);

bail:
	return err;
}

FskErr FskMediaMetaDataRemove(FskMediaMetaData meta, const char *idName, UInt32 index)
{
	FskMediaMetaDataID id;
	FskMediaMetaDataItem item;

    dumpInstrumentedMetaData("Remove", meta, idName, NULL);

	id = findMediaMetaDataID(meta, idName);
	if (NULL == id)
		return kFskErrUnknownElement;

	if (0 == index)
		index = 1;

	for (item = id->items; NULL != item; item = item->next) {
		index -= 1;
		if (0 == index) {
			FskListRemove((FskList*)(void*)&id->items, item);
			if (!(item->flags & kFskMediaMetaDataFlagReference))
				FskMediaPropertyEmpty(&item->value);
			FskMemPtrDispose(item);
			if (NULL == id->items) {
				FskListRemove((FskList*)(void*)&meta->ids, id);
				FskMemPtrDispose(id);
			}
			return kFskErrNone;
		}
	}

	return kFskErrUnknownElement;
}

FskErr FskMediaMetaDataGet(FskMediaMetaData meta, const char *idName, UInt32 index, FskMediaPropertyValue value, UInt32 *flags)
{
	FskMediaMetaDataID id;
	FskMediaMetaDataItem item;

	if (NULL == meta)
		return kFskErrUnknownElement;

	id = findMediaMetaDataID(meta, idName);
	if (NULL == id) {
        dumpInstrumentedMetaData("Get not found", meta, idName, NULL);
		return kFskErrUnknownElement;
    }

	if (0 == index)
		index = 1;

	for (item = id->items; NULL != item; item = item->next) {
		index -= 1;
		if (0 == index) {
			if (value) *value = item->value;
			if (flags) *flags = item->flags;
            dumpInstrumentedMetaData("Get", meta, idName, value);
			return kFskErrNone;
		}
	}

	return kFskErrUnknownElement;
}

FskErr FskMediaMetaDataGetNext(FskMediaMetaData meta, void **metaDataIterator, const char **idName, UInt32 *indexOut, FskMediaPropertyValue value, UInt32 *flags)
{
	FskMediaMetaDataID id = (FskMediaMetaDataID)*metaDataIterator;
	FskMediaMetaDataItem item;
	UInt32 i;

	if (NULL == id) {
		id = meta->ids;
		*indexOut = 1;
	}
	else
		*indexOut += 1;

	for (; NULL != id; id = id->next, *indexOut = 1) {
		for (item = id->items, i = 1; NULL != item; item = item->next, i++) {
			if (*indexOut == i) {
				*metaDataIterator = id;
				*indexOut = i;
				if (idName) *idName = id->idName;
				if (value) *value = item->value;
				if (flags) *flags = item->flags;
                dumpInstrumentedMetaData("Get next", meta, *idName, value);
				return kFskErrNone;
			}
		}
	}

	*metaDataIterator = NULL;
	*indexOut = 0;

	return kFskErrUnknownElement;
}

FskErr FskMediaMetaDataGetForMediaPlayer(FskMediaMetaData meta, const char *metaDataType, UInt32 index, FskMediaPropertyValue valueOut, UInt32 *flags)
{
	FskErr err;
	FskMediaPropertyValueRecord value;

	err = FskMediaMetaDataGet(meta, metaDataType, index, &value, flags);
	if (err) return err;

	return FskMediaPropertyCopy(&value, valueOut);
}

FskMediaMetaDataID findMediaMetaDataID(FskMediaMetaData meta, const char *idName)
{
	FskMediaMetaDataID id;

	for (id = meta->ids; NULL != id; id = id->next) {
		if (0 == FskStrCompare(idName, id->idName))
			return id;
	}

	return NULL;
}

typedef struct  {
	const char *fsk;

	UInt16		dataType;
	UInt16		flags;

	UInt32		quickTime;
	UInt32		mp4;
	UInt32		iTunes;
	UInt32		memoryStickVideo;

	UInt32		id3v20;
	UInt32		id3v23;

	const char	*windowsMedia;
} FskTagMapRecord, *FskTagMap;

#define mdString	kFskMediaPropertyTypeString
#define mdBinary	kFskMediaPropertyTypeData
#define mdImage		kFskMediaPropertyTypeImage

static const FskTagMapRecord gTagMap[] = {
	// fsk			type		flags	quicktime				mp4/3gpp				iTunes					MSV		id3v20		id3v23	windowsmedia
	{"8bdl",		mdBinary,	0,		'8bdl',					'8bdl',					0,						0,		0,			0,		NULL},
	{"Album",		mdString,	0,		'\251alb',				'albm',					kiTunesUserDataAlbum,	0,		'\0TAL',	'TALB',	"WM/AlbumTitle"},
	{"AlbumArt",	mdImage,	0,		0,						0,						kiTunesUserDataCoverArt,-1,		'\0PIC',	'APIC',	"WM/Picture"},
	{"Artist",		mdString,	0,		'\251ART',				'perf',					'\251ART',				0,		'\0TP1',	'TPE1',	"Author"},
	{"Author",		mdString,	0,		kQTUserDataAuthor,		kMP4UserDataAuthor,		0,						0,		0,			0,		NULL},
	{"BPM",			mdString,	0,		0,						0,						'tmpo',					0,		'\0TBP',	'TBPM',	NULL},
	{"Comment",		mdString,	0,		'\251cmt',				0,						kiTunesUserDataComment,	0,		0,			0,		NULL},
	{"Composer",	mdString,	0,		'\251com',				0,						0,						0,		'\0TCM',	'TCOM',	"WM/Composer"},
	{"Copyright",	mdString,	0,		kQTUserDataCopyright,	kMP4UserDataCopyright,	0,						0,		'\0TCR',	'TCOP',	"Copyright"},
	{"Date",		mdString,	0,		kQTUserDataDate,		0,						kQTUserDataDate,		3,		0,			0,		NULL},
	{"FileType",	mdString,	0,		0,						0,						0,						1,		'\0TFT',	'TFLT',	NULL},
	{"FullName",	mdString,	0,		kQTUserDataFullName,	kMP4UserDataFullName,	kQTUserDataFullName,	1,		'\0TT2',	'TIT2',	"Title"},
	{"Genre",		mdString,	0,		kiTunesUserDataGenre,	'gnre',					kiTunesUserDataGenre,	0,		'\0TCO',	'TCON',	"WM/Genre"},
	{"ISRC",		mdString,	0,		0,						0,						0,						0,		'\0TRC',	'TSRC',	"WM/ISRC"},
	{"Lyricist",	mdString,	0,		0,						0,						0,						0,		'\0TXT',	'TEXT',	"WM/Writer"},
	{"PartOfSet",	mdString,	0,		0,						0,						0,						0,		'\0TPA',	'TPOS',	NULL},
	{"Publisher",	mdString,	0,		0,						0,						0,						0,		'\0TPB',	'TPUB',	"WM/Publisher"},
	{"Tool",		mdString,	0,		kQTUserDataSoftware,	0,						kiTunesUserDataTool,	0,		'\0TSS',	'TSSE',	"WM/ToolName"},
	{"TrackLength",	mdString,	0,		0,						0,						0,						0,		'\0TLE',	'TLEN',	"Duration"},
	{"TrackNumber",	mdString,	0,		kQTUserDataTrack,		0,						kiTunesUserDataTrackNum,0,		'\0TRK',	'TRCK',	"WM/TrackNumber"},
	{"Year",		mdString,	0,		0,						'yrrc',					kiTunesUserDataDate,	0,		'\0TYE',	'TYER',	"WM/Year"},
	{NULL,			0,			0,		0,						0,						0,						0,		0,			0,		NULL}
};

static SInt32 getFormatOffset(UInt32 metaDataFormat);

FskErr FskMediaMetaDataFormatTagToFskTag(UInt32 metaDataFormat, void *id, const char **fskID, UInt32 *metaDataType, UInt32 *flags)
{
	FskTagMap walker;
	SInt32 mapOffset = getFormatOffset(metaDataFormat);

	if (mapOffset < 0)
		return kFskErrUnknownElement;

	for (walker = (FskTagMap)gTagMap; NULL != walker->fsk; walker++) {
		if (kFskMetaDataFormatWindowsMedia == metaDataFormat) {
			if ((NULL == walker->windowsMedia) || (0 != FskStrCompare((char *)id, walker->windowsMedia)))
				continue;
		}
		else {
			if (*(UInt32 *)(mapOffset + (unsigned char *)walker) != *(UInt32 *)id)
				continue;
		}

		if (fskID) *fskID = walker->fsk;
		if (metaDataType) *metaDataType = walker->dataType;
		if (flags) *flags = walker->flags;

		return kFskErrNone;
	}

	return kFskErrUnknownElement;
}

FskErr FskMediaMetaDataFskTagToFormatTag(const char *fskID, UInt32 metaDataFormat, void **id, UInt32 *metaDataType, UInt32 *flags)
{
	FskTagMap walker;
	SInt32 mapOffset = getFormatOffset(metaDataFormat);

	if (mapOffset < 0)
		return kFskErrUnknownElement;

	for (walker = (FskTagMap)gTagMap; NULL != walker->fsk; walker++) {
		UInt32 tag;

		if (0 != FskStrCompare(fskID, walker->fsk))
			continue;

		tag = *(UInt32 *)(mapOffset + (unsigned char *)walker);
		if (0 == tag)
			return kFskErrUnknownElement;

		if (id) *(UInt32 *)id = tag;
		if (metaDataType) *metaDataType = walker->dataType;
		if (flags) *flags = walker->flags;

		return kFskErrNone;
	}

	return kFskErrUnknownElement;
}

SInt32 getFormatOffset(UInt32 metaDataFormat)
{
	SInt32 mapOffset = -1;

	switch (metaDataFormat) {
		case kFskMetaDataFormatQuickTime:
			mapOffset = offsetof(FskTagMapRecord, quickTime);
			break;

		case kFskMetaDataFormatMP4:
			mapOffset = offsetof(FskTagMapRecord, mp4);
			break;

		case kFskMetaDataFormatiTunes:
			mapOffset = offsetof(FskTagMapRecord, iTunes);
			break;

		case kFskMetaDataFormatMemoryStickVideo:
			mapOffset = offsetof(FskTagMapRecord, memoryStickVideo);
			break;

		case kFskMetaDataFormatID3v20:
			mapOffset = offsetof(FskTagMapRecord, id3v20);
			break;

		case kFskMetaDataFormatID3v23:
			mapOffset = offsetof(FskTagMapRecord, id3v23);
			break;

		case kFskMetaDataFormatWindowsMedia:
			mapOffset = offsetof(FskTagMapRecord, windowsMedia);
			break;
	}

	return mapOffset;
}

//@@ no doubt DLNA conformance tool wants us to validate this much more..
FskErr FskMediaParseNPT(const char *npt, double *result)
{
    char *c1 = FskStrChr(npt, ':');

    if (c1) {
        if (6 != FskStrLen(c1)) goto bail;
        if (':' != c1[3]) goto bail;

        *result  = FskStrToNum(npt) * 60 * 60;
        *result += FskStrToNum(c1 + 1) * 60;
        *result += FskStrToD(c1 + 3, NULL);

        return kFskErrNone;
    }
    else {
        *result = FskStrToD(npt, NULL);

        return kFskErrNone;
    }

bail:
    return kFskErrBadData;
}


#if SUPPORT_INSTRUMENTATION

static const char *gPropertyNames[] = {
	"undefined",
	"enabled",
	"mediaType",
	"drmKey",
	"drmInfo",
	"timeScale",
	"flags",
	"duration",
	"editList",
	"dimensions",
	"format",
	"esds",
	"bitRate",
	"sampleRate",
	"channelCount",
	"securityType",
	"securityData",
	"id",
	"message",
	"clip",
	"xsObject",
	"crop",
	"dataSize",
	"requestHeaders",
	"samplesPerChunk",
	"volume",
	"pixelFormat",
	"quality",
	"encryptedMetaData",
	"trInfo",
	"omaTagMode",
	"metaDataItems",
	"profile",
	"canChangeSampleRate",
	"canChangeChannelCount",
	"playRate",
	"spooler",
	"time",
	"state",
	"frameRate",
	"keyFrameRate",
	"compressionSettings",
	"pixelAspectRatio",
	"balance",
	"buffer",
	"contrast",
	"brightness",
	"bufferDuration",
	"scrub",
	"eq",
	"redirect",
	"error",
	"markTimes",
	"authentication",
	"authorization",
	"seekableSegment",
	"downloadPath",
	"downloadPosition",
	"download",
	"sprites",
	"rotation",
	"location",
	"hibernate",
	"local"
};

static const char *gTypeNames[] = {
    "undefined",
    "integer",
    "float",
    "boolean",
    "string",
    "rectangle",
    "point",
    "data",
    "key",
    "editList",
    "message",
    "xsObject",
    "stringList",
    "image",
    "uint32List",
    "dimension",
    "spooler",
    "ratio",
    "floatList",
    "sprites"
};

static void formatMediaPropertyValue(char *value, UInt32 valueSize, FskMediaPropertyValue property);

void dumpInstrumentedProperty(const char *message, void *obj, UInt32 propertyID, FskMediaPropertyValue property)
{
    char value[512];
    const char *propertyName = (propertyID <= kFskMediaPropertyLocal) ? gPropertyNames[propertyID] : NULL;
    const char *typeName = (property && (property->type <= kFskMediaPropertyTypeSprites)) ? gTypeNames[property->type] : "unknown type";

    if (NULL == propertyName) {
        if (kFskMediaPropertyFormatInfo == propertyID) propertyName = "formatInfo"; else
        if (kFskMediaPropertyDLNASources == propertyID) propertyName = "dlnaSources"; else
        if (kFskMediaPropertyDLNASinks == propertyID) propertyName = "dlnaSinks"; else
        if (kFskMediaPropertyMaxFramesToQueue == propertyID) propertyName = "maxFramesToQueue"; else
        if (kFskMediaPropertyPerformanceInfo == propertyID) propertyName = "performanceInfo"; else
        if (kFskMediaPropertyPlayMode == propertyID) propertyName = "playMode"; else
            propertyName = "unknown";
    }

    if (NULL == property) {
        FskPropertyPrintfDebug("%s %s (%d), object %p", message, propertyName, propertyID, obj);
        return;
    }

    formatMediaPropertyValue(value, sizeof(value), property);

    FskPropertyPrintfDebug("%s %s (%d), value %s, type %s (%d), object %p", message, propertyName, propertyID, value, typeName, property ? property->type : 0, obj);
}

void dumpInstrumentedMetaData(const char *message, FskMediaMetaData meta, const char *name, FskMediaPropertyValue property)
{
    const char *typeName = (property && (property->type <= kFskMediaPropertyTypeSprites)) ? gTypeNames[property->type] : "unknown type";
    char value[512];

    if (NULL == property) {
        FskInstrumentedItemPrintfDebug(meta, "%s %s", message, name);
        return;
    }

    formatMediaPropertyValue(value, sizeof(value), property);

    FskInstrumentedItemPrintfNormal(meta, "%s %s, value %s, type %s (%u)", message, name, value, typeName, (unsigned int)(property ? property->type : 0));
}

void formatMediaPropertyValue(char *value, UInt32 valueSize, FskMediaPropertyValue property)
{
    switch (property->type) {
        case kFskMediaPropertyTypeInteger:
            snprintf(value, valueSize, "%u", (unsigned int)property->value.integer);
            break;

        case kFskMediaPropertyTypeFloat:
            snprintf(value, valueSize, "%.16g", property->value.number);
            break;

        case kFskMediaPropertyTypeBoolean:
            snprintf(value, valueSize, property->value.b ? "true" : "false");
            break;

        case kFskMediaPropertyTypeString:
            snprintf(value, valueSize, "%s", property->value.str ? property->value.str : "(null string)");
            break;

        case kFskMediaPropertyTypeDimension:
            snprintf(value, valueSize, "%d, %d", (int)property->value.dimension.width, (int)property->value.dimension.height);
            break;

        case kFskMediaPropertyTypeRatio:
            snprintf(value, valueSize, "%d / %d", (int)property->value.ratio.numer, (int)property->value.ratio.denom);
            break;

        case kFskMediaPropertyTypeData:
            snprintf(value, valueSize, "%u bytes", (unsigned)property->value.data.dataSize);
            break;

        case kFskMediaPropertyTypeImage:
            snprintf(value, valueSize, "mime %s, %u bytes", (char*)(property->value.data.data), (unsigned)property->value.data.dataSize);
            break;

        case kFskMediaPropertyTypeUndefined:
        default:
            FskStrCopy(value, "(cannot display)");
            break;
    }
}

#endif
