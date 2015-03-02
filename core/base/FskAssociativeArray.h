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
#ifndef __FSK_ASSOCIATIVE_ARRAY_H__
#define __FSK_ASSOCIATIVE_ARRAY_H__

#include "Fsk.h"
#include "FskList.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {
	kFskUnknownType,
	kFskStringType,
	kFskIntegerType,
	kFskReferenceType,
	kFskBlobType
};

typedef struct FskAssociativeArrayNameListRec {
	struct FskAssociativeArrayNameListRec	*next;
	char	*name;
	char 	*value;
	UInt32	valueSize;
	SInt32	valueType;

	unsigned char data[1];
} FskAssociativeArrayNameListRec, *FskAssociativeArrayNameList;

typedef struct FskAssociativeArrayRec {
	FskAssociativeArrayNameList	arrayHead;
} FskAssociativeArrayRec, *FskAssociativeArray;

typedef FskAssociativeArrayNameListRec FskAssociativeArrayIteratorRec;
typedef FskAssociativeArrayNameList FskAssociativeArrayIterator;


/* Constructor (new) and Destructor (Dispose)
 */
FskAPI(FskAssociativeArray) FskAssociativeArrayNew(void);
FskAPI(void) FskAssociativeArrayDispose(FskAssociativeArray array);

/* FskAssociativeArrayElementSet will associate a typed value with the name.
   If there is already a value, the new value will overwrite the old value.
 */
FskAPI(void) FskAssociativeArrayElementSet(FskAssociativeArray array, const char *name, const void *value, UInt32 valueSize, SInt32 valueType);

#define FskAssociativeArrayElementSetString(array, name, value)	\
	FskAssociativeArrayElementSet(array, name, value, FskStrLen(value), kFskStringType)
#define FskAssociativeArrayElementSetInteger(array, name, value)	\
	FskAssociativeArrayElementSet(array, name, value, sizeof(SInt32), kFskIntegerType)
#define FskAssociativeArrayElementSetReference(array, name, value)	\
	FskAssociativeArrayElementSet(array, name, value, sizeof(void*), kFskReferenceType)

FskAPI(void) FskAssociativeArrayElementCatenateString(FskAssociativeArray array, const char *name, const char *value, Boolean withCommaIfExisting);

FskAPI(void) FskAssociativeArrayElementDispose(FskAssociativeArray array, const char *name);


/* FskAssociativeArrayElementFind will return the value list associated with the name.
 */
FskAPI(void) *FskAssociativeArrayElementGet(FskAssociativeArray array, const char *name, void **value, UInt32 *valueSize, SInt32 *valueType);

FskAPI(char *)FskAssociativeArrayElementGetString(FskAssociativeArray array, const char *name);
FskAPI(SInt32) FskAssociativeArrayElementGetInteger(FskAssociativeArray array, const char *name);
FskAPI(void) *FskAssociativeArrayElementGetReference(FskAssociativeArray array, const char *name);


FskAPI(FskAssociativeArrayIterator) FskAssociativeArrayIteratorNew(FskAssociativeArray array);
FskAPI(FskAssociativeArrayIterator) FskAssociativeArrayIteratorNext(FskAssociativeArrayIterator iter);
FskAPI(void) FskAssociativeArrayIteratorDispose(FskAssociativeArrayIterator iter);

#ifdef __cplusplus
}
#endif

#endif // __FSK_ASSOCIATIVE_ARRAY_H__
