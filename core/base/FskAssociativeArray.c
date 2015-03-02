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
#include "FskUtilities.h"
#include "FskAssociativeArray.h"

// ------------------------------------------------------------------------
FskAssociativeArray FskAssociativeArrayNew(void)
{
	FskAssociativeArray	aa;

	if (kFskErrNone != FskMemPtrNewClear(sizeof(FskAssociativeArrayRec), &aa))
		return NULL;

	return aa;
}

// ------------------------------------------------------------------------
void FskAssociativeArrayDispose(FskAssociativeArray array)
{
	FskAssociativeArrayNameList	list;

	if (!array)
		return;

	while ((list = FskListRemoveFirst((void**)(void*)&array->arrayHead)))
		FskMemPtrDispose(list);

	FskMemPtrDispose(array);
}


// ------------------------------------------------------------------------
void FskAssociativeArrayElementSet(FskAssociativeArray array, const char *name, const void *value, UInt32 valueSize, SInt32 valueType)
{
	FskAssociativeArrayNameList	list;
	SInt32 nameLen = FskStrLen(name) + 1;

	if (kFskStringType == valueType)
		valueSize = FskStrLen((const char *)value) + 1;
	else if (kFskBlobType == valueType)
		;
	else
		valueSize = 0;

	FskAssociativeArrayElementDispose(array, name);

	if (kFskErrNone == FskMemPtrNew(sizeof(FskAssociativeArrayNameListRec) + nameLen + valueSize, &list)) {
		unsigned char *d = list->data;

		list->name = (char *)d;
		FskMemMove(d, name, nameLen);
		d += nameLen;
		list->valueType = valueType;
		list->valueSize = valueSize;
		list->next = NULL;

		if ((kFskStringType == valueType) || (kFskBlobType == valueType)) {
			FskMemMove(d, value, valueSize);
			list->value = (char *)d;
		}
		else
			list->value = (char *)value;

		FskListPrepend((FskList*)(void*)&array->arrayHead, (FskListElement)list);
	}
}

// ------------------------------------------------------------------------
void FskAssociativeArrayElementCatenateString(FskAssociativeArray array, const char *name, const char *value, Boolean withCommaIfExisting)
{
	char *existing;
	SInt32	valueType;
	UInt32	valueSize;

	existing = (char*)FskAssociativeArrayElementGet(array, name, NULL, &valueSize, &valueType);
	if (valueType != kFskStringType)
		existing = NULL;

	if (existing) {
		char *t;
		int existingLen = valueSize, newLen, added = 2;

		if (value == NULL)
			return;

		if (withCommaIfExisting)
			added = 3;

		newLen = FskStrLen(value);
		if (kFskErrNone == FskMemPtrNew(existingLen + newLen + added, &t)) {
			FskStrCopy(t, existing);

			existingLen --;
			if (withCommaIfExisting)
				t[existingLen++] = ',';
			t[existingLen++] = ' ';

			FskStrCopy(&t[existingLen], value);

			FskAssociativeArrayElementSet(array, name, t, 0, kFskStringType);
			FskMemPtrDispose(t);
		}
	}
	else {
		if (value == NULL)
			value = "\n";
		FskAssociativeArrayElementSet(array, name, value, 0, kFskStringType);
	}
}

// ------------------------------------------------------------------------
void FskAssociativeArrayElementDispose(FskAssociativeArray array, const char *name)
{
	FskAssociativeArrayNameList		list, last = NULL;

	// try to find the list of values associated with name
	list = array->arrayHead;
	while (list) {
		if (FskStrCompareCaseInsensitive(name, list->name) == 0) {
			// kill existing list
			if (last)
				last->next = list->next;
			else
				array->arrayHead = list->next;

			FskMemPtrDispose(list);

			break; // while(list)
		}
		last = list;
		list = list->next;
	}
}


// ------------------------------------------------------------------------
void *FskAssociativeArrayElementGet(FskAssociativeArray array, const char *name, void **value, UInt32 *valueSize, SInt32 *valueType)
{
	FskAssociativeArrayNameList		list;

	if (!array || !name)
		return NULL;

	// try to find the list of values associated with name
	list = array->arrayHead;
	while (list) {
		if (FskStrCompareCaseInsensitive(name, list->name) == 0) {
			if (value)
				*value = list->value;
			if (valueSize)
				*valueSize = list->valueSize;
			if (valueType)
				*valueType = list->valueType;
			return list->value;
		}
		list = list->next;
	}

	if (value)
		*value = NULL;
	if (valueSize)
		*valueSize = 0;
	if (valueType)
		*valueType = kFskUnknownType;

	return NULL;
}

// ------------------------------------------------------------------------
char *FskAssociativeArrayElementGetString(FskAssociativeArray array, const char *name)
{
	char *ret;
	SInt32	valueType;

	ret = (char*)FskAssociativeArrayElementGet(array, name, NULL, NULL, &valueType);
	if (ret && valueType == kFskStringType)
		return ret;
	else
		return NULL;
}

// ------------------------------------------------------------------------
SInt32 FskAssociativeArrayElementGetInteger(FskAssociativeArray array, const char *name)
{
	SInt32	ret;
	SInt32	valueType;

	ret = (SInt32)FskAssociativeArrayElementGet(array, name, NULL, NULL, &valueType);
	if (ret && valueType == kFskIntegerType)
		return ret;
	else
		return 0;
}

// ------------------------------------------------------------------------
void *FskAssociativeArrayElementGetReference(FskAssociativeArray array, const char *name)
{
	void	*ret;
	SInt32	valueType;

	ret = FskAssociativeArrayElementGet(array, name, NULL, NULL, &valueType);
	if (ret && valueType == kFskReferenceType)
		return ret;
	else
		return NULL;
}

// ------------------------------------------------------------------------
FskAssociativeArrayIterator FskAssociativeArrayIteratorNew(FskAssociativeArray array)
{
	if (!array || !array->arrayHead)
		return NULL;

	return array->arrayHead;
}

// ------------------------------------------------------------------------
FskAssociativeArrayIterator FskAssociativeArrayIteratorNext(FskAssociativeArrayIterator iter)
{
	if (iter->next)
		return iter->next;
	return NULL;
}

// ------------------------------------------------------------------------
void FskAssociativeArrayIteratorDispose(FskAssociativeArrayIterator iter)
{
}
