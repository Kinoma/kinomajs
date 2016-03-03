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
/**
	\file	FskGrowableStorage.c
	\brief	Resizable storage.
*/

#include "FskGrowableStorage.h"


/********************************************************************************
 ********************************************************************************
 ********************************************************************************
 ***							typedefs and macros							  ***
 ********************************************************************************
 ********************************************************************************
 ********************************************************************************/

#undef FskAssert		/* Why is this on for release builds? */
#define FskAssert(x)	/**< This turns off assertions for \param[in] x the condition to be asserted. */

/** The record used for Growable Storage. */
struct FskGrowableStorageRecord {
	UInt8	*storage;						/**< The actual storage. */
	UInt32	size;							/**< The virtual size of the storage. */
	UInt32	maxSize;						/**< The actual size of the storage. */
};
typedef struct FskGrowableStorageRecord FskGrowableStorageRecord;	/**< The record used for Growable Storage. */

/** The record used for Growable Arrays. */
struct FskGrowableArrayRecord {
	FskGrowableStorageRecord	storage;	/**< The variable sized storage. */
	UInt32						itemSize;	/**< The size of the items in the storage. */
	UInt32						numItems;	/**< The number of items in the storage. */
};

/** The record used for Growable Blob Arrays. */
struct FskGrowableBlobArrayRecord {
	FskGrowableArray		directory;		/**< The storage used for the fixed-sized portion of the storage. */
	FskGrowableStorage		data;			/**< The storage used for the variable-sized portion of the  storage. */
	UInt32					lastID;			/**< The counter used for assigning IDs automatically. */
	UInt32					flags;			/**< Boolean flags indicating the state of this blob array. */
	FskGrowableBlobCompare	compare;		/**< The comparison function to be used for sorting. */
};

#define BLOB_DIR_IDSORTED		1			/**< A flag in the FskGrowableBlobArrayRecord indicating that it has been sorted by ID. */

/** A part of the directory entry for blobs. */
struct BlobEntry {
	UInt32	id;								/**< The ID of this blob. */
	UInt32	offset;							/**< The offset of the blob. */
	UInt32	size;							/**< The size of the blob. */
};											/**< A part of the directory entry for blobs. */
typedef struct BlobEntry BlobEntry;			/**< A part of the directory entry for blobs. */

typedef union BlobEntryPtr      {       struct BlobEntry *be;       void *vd;       char *ch; } BlobEntryPtr;
typedef union ConstBlobEntryPtr { const struct BlobEntry *be; const void *vd; const char *ch; } ConstBlobEntryPtr;
typedef union BlobQueryPtr      {      FskBlobQueryResult rs;       void *vd;      UInt8 *u8; } BlobQueryPtr;
typedef union BlobDataPtr       {                   char *ch;       void *vd;                 } BlobDataPtr;

#define BLOCK_SIZE						(1 << 9)										/**< The quantum used for memory allocation. */
#define BLOB_ALIGN						4												/**< The alignment of blobs. Must be a power of 2 */
#define ALIGN_BLOB_SIZE(z)				(((z) + (BLOB_ALIGN - 1)) & ~(BLOB_ALIGN - 1))	/**< Bumps the argument up to the next block size. \param[in] z input. \return bumped-up size. */
#define BSEARCH_THRESH					100												/**< The threshold for using linear search rather than binary search. */


/********************************************************************************
 ********************************************************************************
 ********************************************************************************
 ***							Utility Functions							  ***
 ********************************************************************************
 ********************************************************************************
 ********************************************************************************/



/****************************************************************************//**
 * Resize Growable Storage
 *	\param[in,out]	storage	the storage to be resized.
 *	\param[in]		size	the desired size of the storage.
 ********************************************************************************/

static FskErr
ResizeGrowableStorage(FskGrowableStorage storage, UInt32 size)
{
	FskErr	err	= kFskErrNone;

	if (size > storage->maxSize) {
		UInt32		maxSize = (size + (BLOCK_SIZE-1)) & (~(BLOCK_SIZE-1));	/* coerce to multiple of BLOCK_SIZE */;
		FskMemPtr	p = storage->storage;
		BAIL_IF_ZERO(maxSize, err, kFskErrMemFull);							/* This happens when size >= 4294966785 */
		#ifndef BAD_REALLOC
			BAIL_IF_ERR(err = FskMemPtrRealloc(maxSize, &p));
		#else /* BAD_REALLOC */
			BAIL_IF_ERR(err = FskMemPtrNew(maxSize, &p));
			FskMemMove(p, storage->storage, storage->maxSize);
			FskMemPtrDispose(storage->storage);
		#endif /* BAD_REALLOC */
		storage->storage = p;												/* Only replace the pointer and maxSize if FskMemPtrRealloc was successful */
		storage->maxSize = maxSize;
	}
	storage->size = size;

bail:
	return err;
}


/***************************************************************************//**
 * Find the location of a substring in another string.
 * This is like FskStrStr or strstr, except it works with size-delimited strings,
 * rather than NULL-terminated C strings. However, you could use it for C strings thusly:
 *		MyMemMem(cstr1, FskStrLen(cstr1), cstr2, FskStrLen(cstr2));
 *	\param[in]	big
 *	\param[in]	big_len
 *	\param[in]	little
 *	\param[in]	little_len
 *	\return		a pointer to the location of the little string in the big string;
 *	\return		NULL, if the little string was not found.
 *******************************************************************************/

static const void*
MyMemMem(const void *big, UInt32 big_len, const void *little, UInt32 little_len) {
	const char *b0, *b1, *b, *l0, *l;
	size_t n;

	if (big_len < little_len)
		return NULL;

	l0 = (const char*)little;
	b0 = (const char*)big;
	b1 = b0 + big_len - little_len;
	for (; b0 <= b1; b0++) {
		if (*b0 == *l0) {
			for (b = b0 + 1, l = l0 + 1, n = little_len - 1; n--; ++b, ++l)
				if (*b != *l)
					goto move_on;
			return b0;
		}
	move_on:
		continue;
	}
	return NULL;
}



#if 0
#pragma mark -
#endif /* 0 */
/********************************************************************************
 ********************************************************************************
 ********************************************************************************
 ***									API									  ***
 ********************************************************************************
 ********************************************************************************
 ********************************************************************************/



/********************************************************************************
 ********************************************************************************
 *****							Variable Sized Items						*****
 ********************************************************************************
 ********************************************************************************/



/********************************************************************************
 * FskGrowableStorageNew
 ********************************************************************************/

FskErr
FskGrowableStorageNew(UInt32 maxSize, FskGrowableStorage *pStorage)
{
	FskErr				err;
	FskGrowableStorage	storage	 = NULL;

	maxSize = (maxSize == 0) ? BLOCK_SIZE : ((maxSize + (BLOCK_SIZE-1)) & (~(BLOCK_SIZE-1)));	/* coerce to multiple of BLOCK_SIZE */
	BAIL_IF_ERR(err = FskMemPtrNew(sizeof(struct FskGrowableStorageRecord), (FskMemPtr*)(void*)(&storage)));
	BAIL_IF_ERR(err = FskMemPtrNew(maxSize,                                 (FskMemPtr*)(void*)(&storage->storage)));
	storage->maxSize = maxSize;
	storage->size    = 0;

bail:
	if ((err != kFskErrNone) && (storage != NULL)) {
		FskMemPtrDispose(storage);
		storage = NULL;
	}
	*pStorage = storage;
	return err;
}


/********************************************************************************
 * FskGrowableStorageDispose
 ********************************************************************************/

void
FskGrowableStorageDispose(FskGrowableStorage storage)
{
	if (storage != NULL) {
		FskMemPtrDispose(storage->storage);
		FskMemPtrDispose(storage);
	}
}


/********************************************************************************
 * FskGrowableStorageDisengage
 ********************************************************************************/

UInt32
FskGrowableStorageDisengage(FskGrowableStorage storage, void **mem)
{
	UInt32 size = FskGrowableStorageGetSize(storage);
	*mem = NULL;
	if (storage != NULL) {
		FskGrowableStorageMinimize(storage);
		*mem = storage->storage;
		(void)FskMemPtrDispose(storage);
	}
	return size;
}


/********************************************************************************
 * FskGrowableStorageDisengageCString
 ********************************************************************************/

UInt32
FskGrowableStorageDisengageCString(FskGrowableStorage storage, char **mem) {
	if (kFskErrNone == FskGrowableStorageAppendItem(storage, "\0", 1))
		return FskGrowableStorageDisengage(storage, (void**)mem);
	*mem = NULL;
	return 0;
}


/********************************************************************************
 * FskGrowableStorageMinimize
 ********************************************************************************/

void
FskGrowableStorageMinimize(FskGrowableStorage storage)
{
	FskMemPtr		p		= storage->storage;

	if (kFskErrNone == FskMemPtrRealloc(storage->size, &p)) {
		storage->storage = p;
		storage->maxSize = storage->size;
	}
}

/********************************************************************************
 * FskGrowableStorageSetSize
 ********************************************************************************/

FskErr
FskGrowableStorageSetSize(FskGrowableStorage storage, UInt32 size)
{
	FskErr	err		= kFskErrNone;

	FskAssert(storage);
	err = ResizeGrowableStorage(storage, size);

	return err;
}



/********************************************************************************
 * FskGrowableStorageGetSize
 ********************************************************************************/

UInt32
FskGrowableStorageGetSize(FskConstGrowableStorage storage)
{
	return storage ? storage->size : 0;
}


/********************************************************************************
 * FskGrowableStorageAppendItem
 ********************************************************************************/

FskErr
FskGrowableStorageAppendItem(FskGrowableStorage storage, const void *item, UInt32 itemSize)
{
	FskErr	err				= kFskErrNone;
	UInt32	storageSize;

	FskAssert(storage);
	if (!itemSize) itemSize = FskStrLen(item);
	storageSize = storage->size;
	BAIL_IF_ERR(err = ResizeGrowableStorage(storage, storageSize + itemSize));
	FskMemMove(storage->storage + storageSize, item, itemSize);

bail:
	return err;
}


/********************************************************************************
 * FskGrowableStorageGetPointerToNewItem
 ********************************************************************************/

FskErr
FskGrowableStorageGetPointerToNewItem(FskGrowableStorage storage, UInt32 offset, UInt32 itemSize, void **pPtr)
{
	FskErr	err				= kFskErrNone;
	UInt32	storageSize;

	*pPtr = NULL;
	FskAssert(storage);
	storageSize = storage->size;
	BAIL_IF_TRUE((offset > storageSize), err, kFskErrBadData);
	BAIL_IF_ERR(err = ResizeGrowableStorage(storage, storageSize + itemSize));
	if (storageSize > offset)
		FskMemMove(storage->storage + offset + itemSize, storage->storage + offset, storageSize - offset);
	*pPtr = storage->storage + offset;

bail:
	return err;
}


/********************************************************************************
 * FskGrowableStorageGetPointerToNewEndItem
 ********************************************************************************/

FskErr
FskGrowableStorageGetPointerToNewEndItem(FskGrowableStorage storage, UInt32 itemSize, void **pPtr)
{
	FskErr	err				= kFskErrNone;
	UInt32	storageSize;

	*pPtr = NULL;
	FskAssert(storage);
	storageSize = storage->size;
	BAIL_IF_ERR(err = ResizeGrowableStorage(storage, storageSize + itemSize));
	*pPtr = storage->storage + storageSize;

bail:
	return err;
}


/********************************************************************************
 * FskGrowableStorageInsertItemAtPosition
 ********************************************************************************/

FskErr
FskGrowableStorageInsertItemAtPosition(FskGrowableStorage storage, UInt32 offset, const void *item, UInt32 itemSize)
{
	FskErr	err		= kFskErrNone;
	void	*p;

	if (!itemSize) itemSize = FskStrLen(item);
	BAIL_IF_ERR(err = FskGrowableStorageGetPointerToNewItem(storage, offset, itemSize, &p));
	FskMemMove(p, item, itemSize);

bail:
	return err;
}


/********************************************************************************
 * FskGrowableStorageRemoveItem
 ********************************************************************************/

void
FskGrowableStorageRemoveItem(FskGrowableStorage storage, UInt32 offset, UInt32 itemSize)
{
	if (storage != NULL) {
		UInt32 storageSize = storage->size;
		if ((offset + itemSize) < storageSize)	/* If there's stuff after this */
			FskMemMove(storage->storage + offset, storage->storage + offset + itemSize, storageSize - offset - itemSize);
		ResizeGrowableStorage(storage, storageSize - itemSize);
	}
}


/********************************************************************************
 * FskGrowableStorageReplaceItem
 ********************************************************************************/

FskErr
FskGrowableStorageReplaceItem(FskGrowableStorage storage, const void *item, UInt32 offset, UInt32 oldSize, UInt32 newSize)
{
	FskErr	err				= kFskErrNone;
	UInt32	storageSize;

	FskAssert(storage);
	if (!newSize) newSize = FskStrLen(item);
	storageSize = storage->size;
	if (newSize > oldSize) {
		BAIL_IF_ERR(err = ResizeGrowableStorage(storage, storageSize + newSize - oldSize));
		FskMemMove(storage->storage + offset + newSize, storage->storage + offset + oldSize, storageSize - offset - oldSize);
	}
	else if (newSize < oldSize) {
		FskMemMove(storage->storage + offset + newSize, storage->storage + offset + oldSize, storageSize - offset - oldSize);
		BAIL_IF_ERR(err = ResizeGrowableStorage(storage, storageSize + newSize - oldSize));
	}
	FskMemMove(storage->storage + offset, item, newSize);

bail:
	return err;
}


/********************************************************************************
 * FskGrowableStorageGetItem
 ********************************************************************************/

FskErr
FskGrowableStorageGetItem(FskConstGrowableStorage storage, UInt32 offset, void *item, UInt32 itemSize)
{
	FskErr	err		= kFskErrNone;

	FskAssert(storage);
	FskAssert(item);
	FskMemMove(item, storage->storage + offset, itemSize);

	return err;
}


/********************************************************************************
 * FskGrowableStorageGetPointerToItem
 ********************************************************************************/

FskErr
FskGrowableStorageGetPointerToItem(FskGrowableStorage storage, UInt32 offset, void **pPtr)
{
	FskErr	err		= kFskErrNone;

	FskAssert(storage);
	FskAssert(pPtr);
	BAIL_IF_FALSE((offset == 0 || offset < storage->size), err, kFskErrItemNotFound);
	*pPtr = storage->storage + offset;

bail:
	if (err != kFskErrNone)	*pPtr = NULL;
	return err;
}


/********************************************************************************
 * FskGrowableStorageGetConstPointerToItem
 ********************************************************************************/

FskErr
FskGrowableStorageGetConstPointerToItem(FskConstGrowableStorage storage, UInt32 offset, const void **pPtr)
{
	FskErr	err		= kFskErrNone;

	FskAssert(storage);
	FskAssert(pPtr);
	BAIL_IF_FALSE((offset == 0 || offset < storage->size), err, kFskErrItemNotFound);
	*pPtr = storage->storage + offset;

bail:
	if (err != kFskErrNone)	*pPtr = NULL;
	return err;
}


/********************************************************************************
 * FskGrowableStorageVAppendF
 ********************************************************************************/

FskErr
FskGrowableStorageVAppendF(FskGrowableStorage storage, const char *fmt, va_list ap) {
	FskErr		err;
	UInt32		n0, n1;
	BlobDataPtr	str;
	va_list		ap2;

	n0 = FskGrowableStorageGetSize(storage);
	(void)FskGrowableStorageSetSize(storage, n0 + 1);
	BAIL_IF_ERR(err = FskGrowableStorageGetPointerToItem(storage, n0, &str.vd));
#if !TARGET_OS_WIN32
	va_copy(ap2, ap);
	n1 = vsnprintf(str.ch, 1, fmt, ap2) + 1;
#else /* TARGET_OS_WIN32 */
	ap2 = ap;
	n1 = _vscprintf(fmt, ap2) + 1;
#endif /* TARGET_OS_WIN32 */
	va_end(ap2);
	(void)FskGrowableStorageSetSize(storage, n0 + n1 + 1);

	(void)FskGrowableStorageGetPointerToItem(storage, n0, &str.vd);
#if !TARGET_OS_WIN32
	n1 = vsnprintf(str.ch, n1 + 1, fmt, ap);
#else /* TARGET_OS_WIN32 */
	n1 = vsnprintf_s(str.ch, n1 + 1, _TRUNCATE, fmt, ap);
#endif /* TARGET_OS_WIN32 */
	err = FskGrowableStorageSetSize(storage, n0 + n1);
	va_end(ap);
bail:
	return err;
}


/********************************************************************************
 * FskGrowableStorageAppendF
 ********************************************************************************/

FskErr
FskGrowableStorageAppendF(FskGrowableStorage storage, const char *fmt, ...) {
	FskErr err;
	va_list ap;
	va_start(ap, fmt);
	err = FskGrowableStorageVAppendF(storage, fmt, ap);
	va_end(ap);
	return err;
}


/********************************************************************************
 * FskGrowableStorageGetPointerToCString
 ********************************************************************************/

const char*
FskGrowableStorageGetPointerToCString(FskGrowableStorage storage) {
	if (!storage)
		return "";
	if (storage->size >= storage->maxSize) {
		UInt32 maxSize = storage->maxSize;
		if (kFskErrNone != ResizeGrowableStorage(storage, maxSize + 1))
			return NULL;
		ResizeGrowableStorage(storage, maxSize);
	}
	storage->storage[storage->size] = 0;
	return (const char*)(storage->storage);
}


/********************************************************************************
 * FskGrowableStorageGetVprintfPointer
 ********************************************************************************/

const char*
FskGrowableStorageGetVprintfPointer(FskGrowableStorage *pStorage, const char *fmt, va_list ap) {
	FskErr err;
	FskGrowableStorage storage;

	BAIL_IF_NULL(pStorage, err, kFskErrInvalidParameter);
	if ((storage = *pStorage) != NULL) {
		err = FskGrowableStorageSetSize(storage, 0);
	}
	else {
		err = FskGrowableStorageNew(0, pStorage);
		storage = *pStorage;
	}
	BAIL_IF_ERR(err);
	if (fmt)
		BAIL_IF_ERR(err = FskGrowableStorageVAppendF(storage, fmt, ap));
	return FskGrowableStorageGetPointerToCString(storage);
bail:
	return "";
}


/********************************************************************************
 * FskGrowableStorageGetSprintfPointer
 ********************************************************************************/

const char*
FskGrowableStorageGetSprintfPointer(FskGrowableStorage *pStorage, const char *fmt, ...) {
	const char *str;
	va_list ap;
	va_start(ap, fmt);
	str = FskGrowableStorageGetVprintfPointer(pStorage, fmt, ap);
	va_end(ap);
	return str;
}


/******************************************************************************
 * FskGrowableStorageFindItem
 *******************************************************************************/

FskErr
FskGrowableStorageFindItem(FskConstGrowableStorage storage, const void *item, UInt32 itemSize, UInt32 startingIndex, UInt32 *foundIndex) {
	FskErr		err;
	const void	*foundPtr;

	*foundIndex = 0xFFFFFFFF;
	if (!itemSize )
		itemSize = FskStrLen(item);
	BAIL_IF_FALSE(startingIndex <= storage->size && itemSize <= storage->size && startingIndex + itemSize <= storage->size, err, kFskErrItemNotFound);	/* Guard against wraparound */
	if (NULL != (foundPtr = MyMemMem(storage->storage + startingIndex, storage->size - startingIndex, item, itemSize))) {
		*foundIndex = (UInt8*)foundPtr - storage->storage;
		err = kFskErrNone;
	}
	else {
		err = kFskErrItemNotFound;
	}
bail:
	return err;
}



#if 0
#pragma mark -
#endif /* 0 */
/********************************************************************************
 ********************************************************************************
 *****							Fixed Sized Items							*****
 ********************************************************************************
 ********************************************************************************/


/********************************************************************************
 * FskGrowableArrayNew
 ********************************************************************************/

FskErr
FskGrowableArrayNew(UInt32 itemSize, UInt32 maxItems, FskGrowableArray *pArray)
{
	FskErr				err;
	FskGrowableArray	array		= NULL;
	UInt32				maxSize		= itemSize * maxItems;

	maxSize = (maxSize == 0) ? BLOCK_SIZE : ((maxSize + (BLOCK_SIZE-1)) & (~(BLOCK_SIZE-1)));	/* coerce to multiple of BLOCK_SIZE */
	BAIL_IF_ERR(err = FskMemPtrNew(sizeof(struct FskGrowableArrayRecord), (FskMemPtr*)(void*)(&array)));
	BAIL_IF_ERR(err = FskMemPtrNew(maxSize,                        (FskMemPtr*)(&array->storage.storage)));
	array->storage.maxSize = maxSize;
	array->storage.size    = 0;
	array->itemSize        = itemSize;
	array->numItems        = 0;

bail:
	if ((err != kFskErrNone) && (array != NULL)) {
		FskMemPtrDispose(array);
		array = NULL;
	}
	*pArray = array;
	return err;
}


/********************************************************************************
 * FskGrowableArrayDispose
 ********************************************************************************/

void
FskGrowableArrayDispose(FskGrowableArray array)
{
	FskGrowableStorageDispose((FskGrowableStorage)array);	/* FskGrowableArray IS a FskGrowableStorage */
}


/********************************************************************************
 * FskGrowableArrayDisengage
 ********************************************************************************/

UInt32
FskGrowableArrayDisengage(FskGrowableArray array, void **mem)
{
	UInt32 size;
	*mem = NULL;
	if (!array)
		return 0;
	size = array->numItems;
	FskGrowableStorageMinimize(&array->storage);
	*mem = array->storage.storage;
	(void)FskMemPtrDispose(array);
	return size;
}


/********************************************************************************
 * FskGrowableArrayMinimize
 ********************************************************************************/

void
FskGrowableArrayMinimize(FskGrowableArray array)
{
	FskGrowableStorageMinimize(&array->storage);
}


/********************************************************************************
 * FskGrowableArraySetItemCount
 ********************************************************************************/

FskErr
FskGrowableArraySetItemCount(FskGrowableArray array, UInt32 numItems)
{
	FskErr err;
	if (kFskErrNone == (err = FskGrowableStorageSetSize(&array->storage, numItems * array->itemSize)))
		array->numItems = numItems;
	return err;
}


/********************************************************************************
 * FskGrowableArrayGetItemCount
 ********************************************************************************/

UInt32
FskGrowableArrayGetItemCount(FskConstGrowableArray array)
{
	return array ? array->numItems : 0;
}


/********************************************************************************
 * FskGrowableArraySetItemSize
 ********************************************************************************/

FskErr
FskGrowableArraySetItemSize(FskGrowableArray array, UInt32 itemSize)
{
	FskErr	err		= kFskErrNone;

	FskAssert(array);
	BAIL_IF_ERR(err = FskGrowableStorageSetSize(&array->storage, itemSize * FskGrowableArrayGetItemCount(array)));
	array->itemSize = itemSize;

bail:
	return err;
}


/********************************************************************************
 * FskGrowableArrayGetItemSize
 ********************************************************************************/

UInt32
FskGrowableArrayGetItemSize(FskConstGrowableArray array)
{
	return array->itemSize;
}


/********************************************************************************
 * FskGrowableArrayAppendItem
 ********************************************************************************/

FskErr
FskGrowableArrayAppendItem(FskGrowableArray array, const void *item)
{
	FskErr err;
	BAIL_IF_NULL(array, err, kFskErrNotDirectory);
	if (kFskErrNone == (err = FskGrowableStorageAppendItem(&array->storage, item, array->itemSize)))
		array->numItems++;
bail:
	return err;
}


/********************************************************************************
 * FskGrowableArrayAppendItems
 ********************************************************************************/

FskErr
FskGrowableArrayAppendItems(FskGrowableArray array, const void *items, UInt32 numItems)
{
	FskErr err;
	BAIL_IF_NULL(array, err, kFskErrNotDirectory);
	if (kFskErrNone == (err = FskGrowableStorageAppendItem(&array->storage, items, array->itemSize * numItems)))
		array->numItems += numItems;
bail:
	return err;
}


/********************************************************************************
 * FskGrowableArrayAppendReversedItems
 ********************************************************************************/

FskErr
FskGrowableArrayAppendReversedItems(FskGrowableArray array, const void *items, UInt32 numItems)
{
	FskErr		err						= kFskErrNone;
	UInt32		storageSize, itemSize;
	const UInt8	*fr;
	UInt8		*to;

	FskAssert(array);
	storageSize = array->storage.size;
	itemSize    = array->itemSize;
	BAIL_IF_ERR(err = ResizeGrowableStorage(&array->storage, storageSize + itemSize * numItems));
	to = array->storage.storage + storageSize;
	fr = (const UInt8*)items + itemSize * (numItems - 1);
	array->numItems += numItems;
	for (; numItems--; to += itemSize, fr -= itemSize)
		FskMemMove(to, fr, itemSize);

bail:
	return err;
}


/********************************************************************************
 * FskGrowableArrayInsertItemAtPosition
 ********************************************************************************/

FskErr
FskGrowableArrayInsertItemAtPosition(FskGrowableArray array, UInt32 index, const void *item)
{
	FskErr err;
	if (kFskErrNone == (err = FskGrowableStorageInsertItemAtPosition(&array->storage, index * array->itemSize, item, array->itemSize)))
		array->numItems++;
	return err;
}


/********************************************************************************
 * FskGrowableArrayRemoveItem
 ********************************************************************************/

void
FskGrowableArrayRemoveItem(FskGrowableArray array, UInt32 index)
{
	FskGrowableStorageRemoveItem(&array->storage, index * array->itemSize, array->itemSize);
	if (array->numItems)
		array->numItems--;
}


/********************************************************************************
 * FskGrowableArrayReplaceItem
 ********************************************************************************/

FskErr
FskGrowableArrayReplaceItem(FskGrowableArray array, const void *item, UInt32 index)
{
	return FskGrowableStorageReplaceItem(&array->storage, item, index * array->itemSize, array->itemSize, array->itemSize);
}


/********************************************************************************
 * FskGrowableArraySwapItems
 ********************************************************************************/

FskErr
FskGrowableArraySwapItems(FskGrowableArray array, UInt32 index0, UInt32 index1)
{
	FskErr	err	= kFskErrNone;
	UInt8	*p0, *p1, t;
	UInt32	n;

	FskAssert(array);
	for (n = array->itemSize, p0 = array->storage.storage + index0 * n, p1 = array->storage.storage + index1 * n; n--; ) {
		t = *p0;
		*p0++ = *p1;
		*p1++ = t;
	}

	return err;
}


/********************************************************************************
 * FskGrowableArraySortItems
 ********************************************************************************/

FskErr
FskGrowableArraySortItems(FskGrowableArray array, FskCompareFunction comp)
{
	FskErr	err					= kFskErrNone;
	UInt32	itemSize, itemCount;

	FskAssert(array);
	itemSize  = array->itemSize;
	itemCount = array->storage.size / itemSize;
	FskQSort(array->storage.storage, itemCount, itemSize, comp);

	return err;
}


/********************************************************************************
 * FskGrowableArrayBSearchItems
 ********************************************************************************/

void*
FskGrowableArrayBSearchItems(FskConstGrowableArray array, const void *key, FskCompareFunction comp)
{
	if (array == NULL)
		return NULL;
	return FskBSearch(key, array->storage.storage, array->numItems, array->itemSize, comp);
}


/********************************************************************************
 * FskGrowableArrayGetItem
 ********************************************************************************/

FskErr
FskGrowableArrayGetItem(FskConstGrowableArray array, UInt32 index, void *item)
{
	return FskGrowableStorageGetItem(&array->storage, index * array->itemSize, item, array->itemSize);
}


/********************************************************************************
 * FskGrowableArrayGetPointerToItem
 ********************************************************************************/

FskErr
FskGrowableArrayGetPointerToItem(FskGrowableArray array, UInt32 index, void **pPtr)
{
	return array->numItems ? FskGrowableStorageGetPointerToItem(&array->storage, index * array->itemSize, pPtr) : kFskErrItemNotFound;
}


/********************************************************************************
 * FskGrowableArrayGetPointerToNewItem
 ********************************************************************************/

FskErr
FskGrowableArrayGetPointerToNewItem(FskGrowableArray array, UInt32 index, void **pPtr)
{
	FskErr err;
	if (kFskErrNone == (err = FskGrowableStorageGetPointerToNewItem(&array->storage, index * array->itemSize, array->itemSize, pPtr)))
		array->numItems++;
	return err;
}


/********************************************************************************
 * FskGrowableArrayGetPointerToNewEndItem
 ********************************************************************************/

FskErr
FskGrowableArrayGetPointerToNewEndItem(FskGrowableArray array, void **pPtr)
{
	FskErr err;
	if (kFskErrNone == (err = FskGrowableStorageGetPointerToNewEndItem(&array->storage, array->itemSize, pPtr)))
		array->numItems++;
	return err;
}


/********************************************************************************
 * FskGrowableArrayGetConstPointerToItem
 ********************************************************************************/

FskErr
FskGrowableArrayGetConstPointerToItem(FskConstGrowableArray array, UInt32 index, const void **pPtr)
{
	return array->numItems ? FskGrowableStorageGetConstPointerToItem(&array->storage, index * array->itemSize, pPtr) : kFskErrItemNotFound;
}


/********************************************************************************
 * FskGrowableArrayGetConstPointerToLastItem
 ********************************************************************************/

FskErr
FskGrowableArrayGetConstPointerToLastItem(FskConstGrowableArray array, const void **pPtr)
{
	return FskGrowableStorageGetConstPointerToItem(&array->storage, array->storage.size - array->itemSize, pPtr);
}


/********************************************************************************
 * FskGrowableArrayGetPointerToLastItem
 ********************************************************************************/

FskErr
FskGrowableArrayGetPointerToLastItem(FskGrowableArray array, void **pPtr)
{
	return FskGrowableStorageGetPointerToItem(&array->storage, array->storage.size - array->itemSize, pPtr);
}


#if 0
#pragma mark -
#endif /* 0 */
/********************************************************************************
 ********************************************************************************
 *****									BLOBs								*****
 ********************************************************************************
 ********************************************************************************/

#define NONEXISTENT_OFFSET	(-1)	/**< Indicates that there is no variable sized portion of the object. */


/****************************************************************************//**
 * Compare blob strings first by size then lexicographically.
 * This will be the default comparison function for blobs. E.g. sorting
 * { "alpha", "alfa", "beta", "gamma", "gama", "delta", "dlta" } produces
 * { "alfa", "beta", "dlta", "gama", "alpha", "delta", "gamma" }.
 *	\param[in]	b0	the left  blob.
 *	\param[in]	b1	the right blob.
 *	\return		0	if the blobs are identical
 *	\return		<0	if the left blob is "less"    than the right blob.
 *	\return		>0	if the left blob is "greater" than the right blob.
 ********************************************************************************/

static int CompareBlobStringsBySize(const FskBlobRecord *b0, const FskBlobRecord *b1) {
	const UInt8	*p1, *p2;
	UInt32		n;
	int			result;
	if (0 == (result = (int)(b0->size) - (int)(b1->size)))							/* Indicate the smaller string */
		for (n = b0->size, p1 = (const UInt8*)(b0->data), p2 = (const UInt8*)(b1->data); n--;)
			if (0 != (result = *p1++ - *p2++))										/* or the one that is lexicographically smaller */
				break;
	return result;
}


/***************************************************************************//**
 * CompareBlobsAlphabetically
 * This yields the expected alphabetical sort.
 *	\param[in]	blob1	the left  blob.
 *	\param[in]	blob2	the right blob.
 *	\return		0		if the strings are identical,
 *	\return		<0		if blob1 comes before blob2.
 *	\return		>0		if blob1 comes after  blob2.
 *******************************************************************************/

static int CompareBlobsAlphabetically(const FskBlobRecord *blob1, const FskBlobRecord *blob2) {
	int i;

	if      (blob1->size <= blob2->size &&
			!(i = FskStrCompareWithLength((const char*)(blob1->data), (const char*)(blob2->data), blob1->size))
	)
		i = -1;
	else if (blob1->size >= blob2->size &&
			!(i = FskStrCompareWithLength((const char*)(blob1->data), (const char*)(blob2->data), blob2->size))
	)
		i = +1;
	else
		i = FskStrCompareWithLength((const char*)(blob1->data), (const char*)(blob2->data), blob1->size);
	return i;
}


/********************************************************************************
 * FskGrowableBlobArrayNew
 ********************************************************************************/

FskErr
FskGrowableBlobArrayNew(UInt32 blobSize, UInt32 maxBlobs, UInt32 dirDataSize, FskGrowableBlobArray *pArray)
{
	FskErr					err;
	FskGrowableBlobArray	array	 = NULL;

	dirDataSize = ALIGN_BLOB_SIZE(sizeof(BlobEntry) + dirDataSize);	/* Bump up to a multiple of 4 */

	BAIL_IF_ERR(err = FskMemPtrNewClear(sizeof(struct FskGrowableBlobArrayRecord), (FskMemPtr*)(void*)(&array)));
	BAIL_IF_ERR(err = FskGrowableArrayNew(dirDataSize, maxBlobs, &array->directory));
	BAIL_IF_ERR(err = FskGrowableStorageNew(maxBlobs * blobSize, &array->data));
	array->compare = CompareBlobStringsBySize;

bail:
	if (err != kFskErrNone) {
		FskGrowableBlobArrayDispose(array);
		array = NULL;
	}

	*pArray = array;
	return err;
}


/********************************************************************************
 * FskGrowableBlobArrayNewFromString
 ********************************************************************************/

FskErr
FskGrowableBlobArrayNewFromString(char *str, UInt32 strSize, char delim, Boolean makeCopy, UInt32 dirDataSize, FskGrowableBlobArray *pArray)
{
	FskGrowableBlobArray	array	 = NULL;
	BlobEntry				*dirPtr;
	FskErr					err;
	UInt32					numRecords, i, maxSize;
	char					*s, *s0, *s1;

	BAIL_IF_NULL(str, err, kFskErrEmpty);
	if (!strSize)	maxSize = (strSize = FskStrLen(str)) + 1;
	else			maxSize = strSize;
	dirDataSize = ALIGN_BLOB_SIZE(sizeof(BlobEntry) + dirDataSize);			/* Bump up to a multiple of 4 */
	for (numRecords = 0, s = str, i = strSize; i--; ++s)
		if (*s == delim)
			++numRecords;
	if (s[-1] != delim) {
		++numRecords;
		maxSize = strSize;													/* No terminator on the last record */
	}

	BAIL_IF_ERR(err = FskMemPtrNewClear(sizeof(struct FskGrowableBlobArrayRecord), (FskMemPtr*)(void*)(&array)));
	BAIL_IF_ERR(err = FskGrowableArrayNew(dirDataSize, numRecords, &array->directory));
	BAIL_IF_ERR(err = FskGrowableArraySetItemCount(array->directory, numRecords));
	if (makeCopy) {
		if (maxSize == strSize)												/* If there is no terminator on the last record, ... */
			++maxSize;														/* ... allocate an extra space for one */
		BAIL_IF_ERR(err = FskGrowableStorageNew(maxSize, &array->data));
		BAIL_IF_ERR(err = FskGrowableStorageSetSize(array->data, maxSize));
		FskMemCopy(array->data->storage, str, strSize);
		array->data->storage[strSize] = delim;								/* Make sure that the last record has a terminator */
	}
	else {
		BAIL_IF_ERR(err = FskMemPtrNew(sizeof(struct FskGrowableStorageRecord), (FskMemPtr*)(void*)(&array->data)));
		array->data->storage = (UInt8*)str;
		array->data->maxSize = maxSize;
		array->data->size    = strSize;
	}
	dirDataSize = array->directory->itemSize;
	dirPtr = (BlobEntry*)(array->directory->storage.storage);
	s1 = (s0 = (char*)(array->data->storage)) + strSize;

	for (i = 0, s = s0; i < numRecords; ++i, dirPtr = (BlobEntry*)((char*)dirPtr + dirDataSize)) {
		dirPtr->id = ++(array->lastID);
		dirPtr->offset = s - s0;
		for (; s < s1 && *s != delim; ++s) {}
		dirPtr->size = s++ - s0 - dirPtr->offset;
	}
	array->compare = CompareBlobsAlphabetically;

bail:
	if (err != kFskErrNone) {
		FskGrowableBlobArrayDispose(array);
		array = NULL;
	}

	*pArray = array;
	return err;
}


/********************************************************************************
 * FskGrowableBlobArrayNewFromStringList
 ********************************************************************************/

FskErr
FskGrowableBlobArrayNewFromStringList(char *str, Boolean makeCopy, UInt32 dirDataSize, FskGrowableBlobArray *pArray) {
	FskErr	err;
	UInt32	strSize;
	char	*t;

	BAIL_IF_NULL(str, err, kFskErrEmpty);
	t = str;
	do {						/* Compute the length of the String List, including the second terminating zero */
		while (*t++) {}
	} while (*t++);				/* Look for two zeros in a row */
	strSize = t - str;			/* Length of the string list */
	BAIL_IF_ERR(err = FskGrowableBlobArrayNewFromString(str, strSize, 0, makeCopy, dirDataSize, pArray));
	FskGrowableBlobArraySetItemCount(*pArray, FskGrowableBlobArrayGetItemCount(*pArray) - 1);
bail:
	return err;
}


/********************************************************************************
 * FskGrowableBlobArrayDispose
 ********************************************************************************/

void
FskGrowableBlobArrayDispose(FskGrowableBlobArray array)
{
	if (array != NULL) {
		if (array->directory != NULL)	FskGrowableArrayDispose(array->directory);
		if (array->data != NULL)		FskGrowableStorageDispose(array->data);
		FskMemPtrDispose(array);
	}
}


/********************************************************************************
 * FskGrowableBlobArraySetDirectoryDataSize
 ********************************************************************************/

FskErr
FskGrowableBlobArraySetDirectoryDataSize(FskGrowableBlobArray array, UInt32 dirDataSize)
{
	dirDataSize = ALIGN_BLOB_SIZE(sizeof(BlobEntry) + dirDataSize);
	return FskGrowableArraySetItemSize(array->directory, dirDataSize);
}


/********************************************************************************
 * FskGrowableBlobArrayGetDirectoryDataSize
 ********************************************************************************/

UInt32
FskGrowableBlobArrayGetDirectoryDataSize(FskConstGrowableBlobArray array)
{
	return FskGrowableArrayGetItemSize(array->directory) - sizeof(BlobEntry);
}


/********************************************************************************
 * FskGrowableBlobArraySetItemCount
 ********************************************************************************/

FskErr
FskGrowableBlobArraySetItemCount(FskGrowableBlobArray array, UInt32 numItems)
{
	FskErr	err		= kFskErrNone;
	UInt32	oldNum;

	BAIL_IF_NULL(array, err, kFskErrNotDirectory);
	oldNum = FskGrowableArrayGetItemCount(array->directory);
	if (oldNum != numItems) {
		BAIL_IF_ERR(err = FskGrowableArraySetItemCount(array->directory, numItems));
		if (numItems > oldNum) {
			void *ptr;
			BAIL_IF_ERR(err = FskGrowableArrayGetPointerToItem(array->directory, oldNum, &ptr));
			FskMemSet(ptr, 0, (numItems - oldNum) * sizeof(BlobEntry));			/* Clear new directory entries */
		}
		BAIL_IF_ERR(err = FskGrowableArraySetItemCount(array->directory, numItems));
		if (numItems == 0) {
			BAIL_IF_ERR(err = FskGrowableStorageSetSize(array->data, 0));		/* Reclaim storage */
		}
	}

bail:
	return err;
}


/********************************************************************************
 * FskGrowableBlobArrayGetItemCount
 ********************************************************************************/

UInt32
FskGrowableBlobArrayGetItemCount(FskConstGrowableBlobArray array)
{
	return array ? FskGrowableArrayGetItemCount(array->directory) : 0;
}


/********************************************************************************
 * FskGrowableBlobArrayRemoveItem
 ********************************************************************************/

void
FskGrowableBlobArrayRemoveItem(FskGrowableBlobArray array, UInt32 index)
{
	FskGrowableArrayRemoveItem(array->directory, index);	/* This does not reclaim storage */
}


/********************************************************************************
 * InitBlobDirEntry
 ********************************************************************************/

static FskErr
InitBlobDirEntry(BlobEntry *dirPtr, FskGrowableBlobArray array, UInt32 itemSize, UInt32 *id)
{
	FskErr	err;
	UInt32	z		= (itemSize + (BLOB_ALIGN - 1)) & ~(BLOB_ALIGN - 1);						/* Bump new item size up to a nice number */
	UInt32	offset	= FskGrowableStorageGetSize(array->data);									/* Get location of new data */

	FskMemSet(dirPtr, 0, array->directory->itemSize);											/* Clear directory entry */

	if ((err = FskGrowableStorageSetSize(array->data, offset + z)) == kFskErrNone) {			/* Allocate data storage */
		if ((id != NULL) && (*id != kFskGrowableBlobArrayUnassignedID)) {						/* If given an ID ... */
			array->lastID = *id;																/* ... use it ... */
			array->flags &= ~BLOB_DIR_IDSORTED;													/* .. indicate potential out-of-order */
		} else {																				/* otherwise ... */
			++(array->lastID);																	/* ... generate a new one sequentially ... */
		}

		dirPtr->id		= array->lastID;														/* Set the blob ID */
		dirPtr->size	= itemSize;																/* Set blob's size */
		dirPtr->offset	= offset;																/* Set blob's location */
		if (id != NULL)	*id = dirPtr->id;														/* ... and return it if desired */
	}
	else if (id != NULL)	*id = 0;															/* Return a null id if data allocation was unsuccessful */

	return err;
}


/********************************************************************************
 * FskGrowableBlobArrayGetPointerToNewEndItem
 ********************************************************************************/

FskErr
FskGrowableBlobArrayGetPointerToNewEndItem(FskGrowableBlobArray array, UInt32 itemSize, UInt32 *id, void **blob, void **dirData)
{
	FskErr		err;
	BlobEntry	*dirPtr;

	*blob = NULL;
	BAIL_IF_NULL(array, err, kFskErrNotDirectory);
	BAIL_IF_ERR(err = FskGrowableArrayGetPointerToNewEndItem(array->directory, (void**)(void*)(&dirPtr)));
	BAIL_IF_ERR(err = InitBlobDirEntry(dirPtr, array, itemSize, id));
	BAIL_IF_ERR(err = FskGrowableStorageGetPointerToItem(array->data, dirPtr->offset, blob));
	if (	(array->flags & BLOB_DIR_IDSORTED)
		&&	(array->directory->storage.size > array->directory->itemSize)
		&&	(((BlobEntry*)(((char*)dirPtr) - array->directory->itemSize))->id	<= dirPtr->id)
	)
		array->flags &= ~BLOB_DIR_IDSORTED;	/* Not sorted */

bail:
	if (dirData != NULL)	*dirData = (err == kFskErrNone) ? (dirPtr + 1) : NULL;	/* Return pointer to the public portion of the directory */
	return err;
}


/********************************************************************************
 * FskGrowableBlobArrayGetPointerToNewItem
 ********************************************************************************/

FskErr
FskGrowableBlobArrayGetPointerToNewItem(FskGrowableBlobArray array, UInt32 index, UInt32 itemSize, UInt32 *id, void **blob, void **dirData)
{
	FskErr		err;
	BlobEntry	*dirPtr;

	*blob = NULL;
	BAIL_IF_NULL(array, err, kFskErrNotDirectory);
	BAIL_IF_ERR(err = FskGrowableArrayGetPointerToNewItem(array->directory, index, (void**)(void*)(&dirPtr)));
	BAIL_IF_ERR(err = InitBlobDirEntry(dirPtr, array, itemSize, id));
	BAIL_IF_ERR(err = FskGrowableStorageGetPointerToItem(array->data, dirPtr->offset, blob));
	array->flags &= ~BLOB_DIR_IDSORTED;

bail:
	if (dirData != NULL)	*dirData = (err == kFskErrNone) ? (dirPtr + 1) : NULL;
	return err;
}


/********************************************************************************
 * FskGrowableBlobArrayGetPointerToItem
 ********************************************************************************/

FskErr
FskGrowableBlobArrayGetPointerToItem(FskGrowableBlobArray array, UInt32 index, void **blob, UInt32 *size, void **dirData)
{
	FskErr			err;
	BlobEntryPtr	dirPtr	= { NULL };
	void			*myPtr	= NULL;

	BAIL_IF_NULL(array, err, kFskErrItemNotFound);
	BAIL_IF_ERR(err = FskGrowableArrayGetPointerToItem(array->directory, index, &dirPtr.vd));
	BAIL_IF_ERR(err = FskGrowableStorageGetPointerToItem(array->data, dirPtr.be->offset, &myPtr));

bail:
	if (blob    != NULL)	*blob		= (err == kFskErrNone) ? myPtr			: NULL;
	if (dirData != NULL)	*dirData	= (err == kFskErrNone) ? (dirPtr.be + 1)	: NULL;
	if (size    != NULL)	*size		= (err == kFskErrNone) ? dirPtr.be->size	: 0;
	return err;
}


/********************************************************************************
 * FskGrowableBlobArrayGetConstPointerToItem
 ********************************************************************************/

FskErr
FskGrowableBlobArrayGetConstPointerToItem(FskConstGrowableBlobArray array, UInt32 index, const void **blob, UInt32 *size, const void **dirData)
{
	FskErr				err;
	ConstBlobEntryPtr	dirPtr	= { NULL };
	const void			*myPtr	= NULL;

	BAIL_IF_NULL(array, err, kFskErrItemNotFound);
	BAIL_IF_ERR(err = FskGrowableArrayGetConstPointerToItem(array->directory, index, &dirPtr.vd));
	BAIL_IF_ERR(err = FskGrowableStorageGetConstPointerToItem(array->data, dirPtr.be->offset, &myPtr));

bail:
	if (blob    != NULL)	*blob		= (err == kFskErrNone) ? myPtr			: NULL;
	if (dirData != NULL)	*dirData	= (err == kFskErrNone) ? (dirPtr.be + 1)	: NULL;
	if (size    != NULL)	*size		= (err == kFskErrNone) ? dirPtr.be->size	: 0;
	return err;
}


/********************************************************************************
 * FskGrowableBlobArrayGetSizeOfItem
 ********************************************************************************/

UInt32
FskGrowableBlobArrayGetSizeOfItem(FskConstGrowableBlobArray array, UInt32 index)
{
	FskErr				err;
	ConstBlobEntryPtr	dirPtr;
	UInt32				size	= 0;

	BAIL_IF_NULL(array, err, kFskErrItemNotFound);
	BAIL_IF_ERR(err = FskGrowableArrayGetConstPointerToItem(array->directory, index, &dirPtr.vd));
	size = dirPtr.be->size;

bail:
	return size;
}


/********************************************************************************
 * FskGrowableBlobArraySetSizeOfItem
 ********************************************************************************/

FskErr
FskGrowableBlobArraySetSizeOfItem(FskGrowableBlobArray array, UInt32 index, UInt32 size)
{
	FskErr			err;
	BlobEntryPtr	dirPtr;

	BAIL_IF_NULL(array, err, kFskErrItemNotFound);
	BAIL_IF_ERR(err = FskGrowableArrayGetPointerToItem(array->directory, index, &dirPtr.vd));
	if (size > dirPtr.be->size) {
		dirPtr.be->offset = FskGrowableStorageGetSize(array->data);										/* Waste old memory, if not at then end */
		BAIL_IF_ERR(FskGrowableStorageSetSize(array->data, dirPtr.be->offset + ALIGN_BLOB_SIZE(size)));	/* We could try compaction upon failure */
	}
	dirPtr.be->size = size;

bail:
	return err;
}


/********************************************************************************
 * FskGrowableBlobArrayEditItem
 ********************************************************************************/

FskErr
FskGrowableBlobArrayEditItem(FskGrowableBlobArray array, UInt32 index, UInt32 offset, UInt32 delBytes, const void *repl, UInt32 replBytes)
{
	BlobDataPtr		newBlob	= { NULL };
	BlobEntryPtr	dirPtr;
	FskErr			err;
	SInt32			tailSize;
	UInt32			tailOffset, newSize;
	BlobDataPtr		oldBlob;

	BAIL_IF_NULL(array, err, kFskErrItemNotFound);
	BAIL_IF_ERR(err = FskGrowableArrayGetPointerToItem(array->directory, index, &dirPtr.vd));
	tailOffset = offset + delBytes;
	tailSize = dirPtr.be->size - tailOffset;
	BAIL_IF_NEGATIVE(tailSize, err, kFskErrTooMany);
	BAIL_IF_ERR(err = FskGrowableStorageGetPointerToItem(array->data, dirPtr.be->offset, &oldBlob.vd));
	newSize = offset + replBytes + tailSize;
	BAIL_IF_ERR(err = FskMemPtrNew(newSize, &newBlob.vd));
	if (offset)				FskMemCopy(newBlob.ch,						oldBlob.ch,					offset);
	if (replBytes && repl)	FskMemCopy(newBlob.ch + offset,				repl,						replBytes);
	if (tailSize)			FskMemCopy(newBlob.ch + offset + replBytes,	oldBlob.ch + tailOffset,	tailSize);
	BAIL_IF_ERR(err = FskGrowableBlobArraySetSizeOfItem(array, index, newSize));
	(void)FskGrowableBlobArrayGetPointerToItem(array, index, &oldBlob.vd, NULL, &dirPtr.vd);
	FskMemCopy(oldBlob.vd, newBlob.vd, newSize);
bail:
	FskMemPtrDispose(newBlob.vd);
	return err;
}


/********************************************************************************
 * SetBlobDirAndData
 ********************************************************************************/

static void
SetBlobDirAndData(FskGrowableBlobArray array, const void *frDir, const void *frData, UInt32 dataSize, void *toDir, void *toData)
{
	UInt32 dirSize	= array->directory->itemSize - sizeof(BlobEntry);

	if (frData != NULL)		FskMemCopy(toData, frData, dataSize);								/* Copy data to blob */
	else					FskMemSet( toData, 0,      dataSize);								/* Clear new part of blob */
	if (dirSize > 0) {
		if (frDir != NULL)	FskMemCopy(toDir, frDir, dirSize);									/* Copy data to directory */
		else				FskMemSet( toDir, 0,     dirSize);									/* Clear new part of directory */
	}
}


/********************************************************************************
 * FskGrowableBlobArrayAppendItem
 ********************************************************************************/

FskErr
FskGrowableBlobArrayAppendItem(FskGrowableBlobArray array, const void *dir, const void *blob, UInt32 blobSize, UInt32 *id)
{
	FskErr		err;
	void		*myBlob, *myDir;

	if ((err = FskGrowableBlobArrayGetPointerToNewEndItem(array, blobSize, id, &myBlob, &myDir)) == kFskErrNone)
		SetBlobDirAndData(array, dir, blob, blobSize, myDir, myBlob);

	return err;
}


/********************************************************************************
 * FskGrowableBlobArrayInsertItemAtPosition
 ********************************************************************************/

FskErr
FskGrowableBlobArrayInsertItemAtPosition(FskGrowableBlobArray array, UInt32 index, const void *dir, const void *blob, UInt32 blobSize, UInt32 *id)
{
	FskErr	err;
	void	*myBlob, *myDir;

	if ((err = FskGrowableBlobArrayGetPointerToNewItem(array, index, blobSize, id, &myBlob, &myDir)) == kFskErrNone) {
		SetBlobDirAndData(array, dir, blob, blobSize, myDir, myBlob);
		array->flags &= ~BLOB_DIR_IDSORTED;
	}

	return err;
}


/********************************************************************************
 * FskGrowableBlobArrayReplaceItem
 ********************************************************************************/

FskErr
FskGrowableBlobArrayReplaceItem(FskGrowableBlobArray array, UInt32 index, const void *dir, const void *blob, UInt32 blobSize)
{
	FskErr	err;
	void	*myBlob, *myDir;

	if (	((err = FskGrowableBlobArraySetSizeOfItem(array, index, blobSize)) == kFskErrNone)
		&&	((err = FskGrowableBlobArrayGetPointerToItem(array, index, &myBlob, NULL, &myDir)) == kFskErrNone)
	) {
		SetBlobDirAndData(array, dir, blob, blobSize, myDir, myBlob);
	}

	return err;
}


/********************************************************************************
 * FskGrowableBlobArraySwapItems
 ********************************************************************************/

FskErr
FskGrowableBlobArraySwapItems(FskGrowableBlobArray array, UInt32 index0, UInt32 index1)
{
	FskErr			err;
	BlobEntryPtr	d0, d1;
	UInt32			t;																			/* The id's and indices stay the same */

	BAIL_IF_NULL(array, err, kFskErrItemNotFound);
	BAIL_IF_ERR(err = FskGrowableArrayGetPointerToItem(array->directory, index0, &d0.vd));
	BAIL_IF_ERR(err = FskGrowableArrayGetPointerToItem(array->directory, index1, &d1.vd));
	t = d0.be->offset;	d0.be->offset = d1.be->offset;	d1.be->offset = t;						/* Swap offsets */
	t = d0.be->size;	d0.be->size   = d1.be->size;	d1.be->size   = t;						/* Swap sizes */
	if ((t = array->directory->itemSize - sizeof(BlobEntry)) > 0) {								/* Swap extra directory information */
		UInt32	*p0, *p1, i;
		for (i = t / sizeof(UInt32), p0 = (UInt32*)(d0.be + 1), p1 = (UInt32*)(d1.be + 1); i--; ) {
			t     = *p0;
			*p0++ = *p1;
			*p1++ = t;
		}
	}

bail:
	return err;
}


/********************************************************************************
 * CompareID
 ********************************************************************************/

static int
CompareID(const void *vID, const void *vDir)
{
	UInt32			id			= *((const UInt32*)vID);
	const BlobEntry *dirEntry	= (const BlobEntry*)vDir;

	if      (id < dirEntry->id)	return -1;
	else if (id > dirEntry->id)	return +1;
	else						return 0;
}


/********************************************************************************
 * FskGrowableBlobArraySortItemsByID
 ********************************************************************************/

FskErr
FskGrowableBlobArraySortItemsByID(FskGrowableBlobArray array)
{
	FskErr	err;

	BAIL_IF_NULL(array, err, kFskErrNotDirectory);
	BAIL_IF_ERR(err = FskGrowableArraySortItems(array->directory, CompareID));
	array->flags |= BLOB_DIR_IDSORTED;

bail:
	return err;
}


/********************************************************************************
 * FskGrowableBlobArraySort globals
 ********************************************************************************/

static FskConstGrowableBlobArray gBlobArray;


/********************************************************************************
 * CompareBlobs
 ********************************************************************************/

static int CompareBlobs(const void *vb1, const void* vb2) {
	const BlobEntry	*b1	= (const BlobEntry*)vb1,
					*b2	= (const BlobEntry*)vb2;
	const char		*b0	= (const char*)(gBlobArray->data->storage);
	FskBlobRecord	s1, s2;

	s1.id	= b1->id;
	s1.size	= b1->size;
	s1.data	= (void*)(b1->offset + b0);
	s1.dir	= (void*)(b1 + 1);
	s2.id	= b2->id;
	s2.size	= b2->size;
	s2.data	= (void*)(b2->offset + b0);
	s2.dir	= (void*)(b2 + 1);
	return (*gBlobArray->compare)(&s1, &s2);
}


/********************************************************************************
 * CompareKeyBlob
 ********************************************************************************/

static int CompareKeyBlob(const void *vk1, const void* vb2) {
	const BlobEntry	*b2	= (const BlobEntry*)vb2;
	FskBlobRecord	s2;

	s2.id	= b2->id;
	s2.size	= b2->size;
	s2.data	= b2->offset + gBlobArray->data->storage;
	s2.dir	= (void*)(b2 + 1);
	return (*gBlobArray->compare)((const FskBlobRecord*)vk1, &s2);
}


/********************************************************************************
 * FskGrowableBlobArraySetCompareFunction
 ********************************************************************************/

FskErr
FskGrowableBlobArraySetCompareFunction(FskGrowableBlobArray array, FskGrowableBlobCompare comp) {
	if (!array)
		return kFskErrNotDirectory;
	if      (NULL    == comp)	comp = CompareBlobStringsBySize;
	else if (1 == (long)comp)	comp = CompareBlobsAlphabetically;
	array->compare = comp;
	return kFskErrNone;
}


/********************************************************************************
 * FskGrowableBlobArraySortItems
 ********************************************************************************/

FskErr
FskGrowableBlobArraySortItems(FskGrowableBlobArray array)
{
	FskErr	err	= kFskErrNone;

	if (!array) {
		err = kFskErrNotDirectory;							/* Not unusual to be called with a NULL array */
		goto bail;
	}
	BAIL_IF_NULL(array->compare, err, kFskErrExtensionNotFound);

	gBlobArray = array;
	FskQSort(array->directory->storage.storage, array->directory->numItems, array->directory->itemSize, &CompareBlobs);

bail:
	return err;
}


/********************************************************************************
 * FskGrowableBlobArrayBSearchItems
 ********************************************************************************/

FskErr
FskGrowableBlobArrayBSearchItems(FskConstGrowableBlobArray array, const FskBlobRecord *key, UInt32 *pIndex)
{
	FskErr				err		= kFskErrItemNotFound;
	FskGrowableArray	directory;
	const BlobEntry		*dirPtr;

	*pIndex = -1;
	if (!array) {
		err = kFskErrNotDirectory;							/* Not unusual to be called with a NULL array */
		goto bail;
	}
	BAIL_IF_NULL(array->compare, err, kFskErrExtensionNotFound);

	gBlobArray = array;
	directory = array->directory;
	if (NULL != (dirPtr = FskBSearch(key, directory->storage.storage, directory->numItems, directory->itemSize, &CompareKeyBlob))) {
		*pIndex = ((const UInt8*)dirPtr - directory->storage.storage) / directory->itemSize;
		err = kFskErrNone;
	}

bail:
	return err;
}


/********************************************************************************
 * FskGrowableBlobArrayCompact
 ********************************************************************************/

FskErr
FskGrowableBlobArrayCompact(FskConstGrowableBlobArray array)
{
	UInt8			*tmp	= NULL;
	FskErr			err;
	BlobEntryPtr	dirPtr;
	UInt8			*data;
	UInt32			z, n, dirSize;

	BAIL_IF_NULL(array, err, kFskErrNotDirectory);
	z = FskGrowableStorageGetSize(array->data);	/* The current size of the data is always larger that the final result */
	BAIL_IF_ERR(err = FskMemPtrNew(z, &tmp));															/* Allocate tmp */
	BAIL_IF_ERR(err = FskGrowableStorageGetPointerToItem(array->data, 0, (void**)(void*)(&data)));		/* Get data ptr */
	FskMemCopy(tmp, data, z);																			/* Copy data to tmp */

	BAIL_IF_ERR(err = FskGrowableArrayGetPointerToItem(array->directory, 0, &dirPtr.vd));
	n = FskGrowableArrayGetItemCount(array->directory);
	dirSize	= array->directory->itemSize;
	for (z = 0; n--; z += ALIGN_BLOB_SIZE(dirPtr.be->size), dirPtr.ch += dirSize) {
		FskMemCopy(data + z, tmp + dirPtr.be->offset, dirPtr.be->size);
		dirPtr.be->offset = z;
	}
	FskGrowableStorageSetSize(array->data, z);

bail:
	if (tmp != NULL)	FskMemPtrDispose(tmp);
	return err;
}


/********************************************************************************
 * GetDirectoryEntryFromIdOfItemInGrowableBlobArray
 ********************************************************************************/

static BlobEntry*
GetDirectoryEntryFromIdOfItemInGrowableBlobArray(FskGrowableBlobArray array, UInt32 theID)
{
	register UInt32		numItems;
	register BlobEntry	*dirPtr		= NULL;

	if (	((numItems = FskGrowableArrayGetItemCount(array->directory)) < BSEARCH_THRESH)		/* If the list is small ... */
		||	!(array->flags & BLOB_DIR_IDSORTED)													/* ... or unsorted ... */
	) {																							/* Do a linear search */
		void			*ptr;
		register UInt32	id			= theID;
		register UInt32	dirSize		= array->directory->itemSize;
		if (FskGrowableArrayGetPointerToItem(array->directory, 0, &ptr) == kFskErrNone)
			for (dirPtr = ptr, id = theID; numItems--; dirPtr = (BlobEntry*)((char*)dirPtr + dirSize))
				if (dirPtr->id == id)
					return dirPtr;
	}
	else {
		dirPtr = FskGrowableArrayBSearchItems(array->directory, &theID, CompareID);				/* Do a binary search */
	}

	return dirPtr;
}


/********************************************************************************
 * GetConstDirectoryEntryFromIdOfItemInGrowableBlobArray
 ********************************************************************************/

static const BlobEntry*
GetConstDirectoryEntryFromIdOfItemInGrowableBlobArray(FskConstGrowableBlobArray array, UInt32 theID)
{
	register UInt32				numItems;
	register const BlobEntry	*dirPtr;

	if (!array)
		return NULL;
	if (	((numItems = FskGrowableArrayGetItemCount(array->directory)) < BSEARCH_THRESH)		/* If the list is small ... */
		||	!(array->flags & BLOB_DIR_IDSORTED)													/* ... or unsorted ... */
	) {																							/* Do a linear search */
		const void		*ptr;
		register UInt32	id			= theID;
		register UInt32	dirSize		= array->directory->itemSize;

		if (FskGrowableArrayGetConstPointerToItem(array->directory, 0, &ptr) != kFskErrNone)
			return NULL;

			for (dirPtr = ptr, id = theID; numItems--; dirPtr = (const BlobEntry*)((const char*)dirPtr + dirSize))
				if (dirPtr->id == id)
					return dirPtr;
	}
	else {
		dirPtr = FskGrowableArrayBSearchItems(array->directory, &theID, CompareID);				/* Do a binary search */
	}

	return dirPtr;
}


/********************************************************************************
 * FskGrowableBlobArrayGetIndexFromIDOfItem
 ********************************************************************************/

FskErr
FskGrowableBlobArrayGetIndexFromIDOfItem(FskConstGrowableBlobArray array, UInt32 id, UInt32 *index)
{
	FskErr				err;
	ConstBlobEntryPtr	dirPtr, dir0;

	*index = kFskGrowableBlobArrayUnassignedID;
	BAIL_IF_NULL((dirPtr.be = GetConstDirectoryEntryFromIdOfItemInGrowableBlobArray(array, id)), err, kFskErrItemNotFound);
	BAIL_IF_ERR(err = FskGrowableArrayGetConstPointerToItem(array->directory, 0, &dir0.vd));
	*index = (UInt32)((dirPtr.ch - dir0.ch) / array->directory->itemSize);

bail:
	return err;
}


/********************************************************************************
 * FskGrowableBlobArrayGetIDOfItem
 ********************************************************************************/

FskErr
FskGrowableBlobArrayGetIDOfItem(FskConstGrowableBlobArray array, UInt32 index, UInt32 *id)
{
	FskErr				err;
	ConstBlobEntryPtr	dirPtr;

	BAIL_IF_NULL(array, err, kFskErrItemNotFound);
	err = FskGrowableArrayGetConstPointerToItem(array->directory, index, &dirPtr.vd);

bail:
	*id = (err == kFskErrNone) ? dirPtr.be->id : 0;	/* Returns 0 if error, since 0 is an invalid id */
	return err;
}


/********************************************************************************
 * FskGrowableBlobArraySetIDOfItem
 ********************************************************************************/

FskErr
FskGrowableBlobArraySetIDOfItem(FskGrowableBlobArray array, UInt32 index, UInt32 id)
{
	FskErr			err		= kFskErrNone;
	BlobEntryPtr	dirPtr;

	BAIL_IF_NULL(array, err, kFskErrItemNotFound);
	BAIL_IF_ERR(err = FskGrowableArrayGetPointerToItem(array->directory, index, &dirPtr.vd));
	dirPtr.be->id = id;
	array->flags &= ~BLOB_DIR_IDSORTED;		/* This will probably cause the data to be unsorted */

bail:
	return err;
}


/********************************************************************************
 * FskGrowableBlobArrayGetPointerFromItemID
 ********************************************************************************/

FskErr
FskGrowableBlobArrayGetPointerFromItemID(FskGrowableBlobArray array, UInt32 id, void **blob, UInt32 *size, void **dirData)
{
	FskErr		err;
	BlobEntry	*dirPtr;

	*blob = NULL;
	BAIL_IF_NULL((dirPtr = GetDirectoryEntryFromIdOfItemInGrowableBlobArray(array, id)), err, kFskErrItemNotFound);
	BAIL_IF_ERR(err = FskGrowableStorageGetPointerToItem(array->data, dirPtr->offset, blob));
	if (dirData != NULL)	*dirData = dirPtr + 1;
	if (size != NULL)		*size = dirPtr->size;

bail:
	return err;
}


/********************************************************************************
 * FskGrowableBlobArrayGetConstPointerFromItemID
 ********************************************************************************/

FskErr
FskGrowableBlobArrayGetConstPointerFromItemID(FskConstGrowableBlobArray array, UInt32 id, const void **blob, UInt32 *size, const void **dirData)
{
	FskErr			err;
	const BlobEntry	*dirPtr;

	*blob = NULL;
	BAIL_IF_NULL((dirPtr = GetConstDirectoryEntryFromIdOfItemInGrowableBlobArray(array, id)), err, kFskErrItemNotFound);
	BAIL_IF_ERR(err = FskGrowableStorageGetConstPointerToItem(array->data, dirPtr->offset, blob));
	if (dirData != NULL)	*dirData = dirPtr + 1;
	if (size != NULL)		*size = dirPtr->size;

bail:
	return err;
}

/****************************************************************************//**
 * Fsk Blob Query Result Record.
 * The result of a query.
 ********************************************************************************/

struct FskBlobQueryResultRecord {
	UInt32	magic;		/**< a magic number to detect corruption of this ephemeral data structure. */
	UInt32	unrefine;	/**< The number of bytes to roll back a refinement. */
	UInt32	numItems;	/**< The number of items in this query result. */
	UInt32	items[1];	/**< The indices of the items that match the query. */
};


#if TARGET_RT_LITTLE_ENDIAN
	#define	QUERY_MAGIC	0x59525551	/**< "QURY" or 'YRUQ' */
#else
	#define QUERY_MAGIC	0x51555259	/**< "QURY" or 'QURY' */
#endif

/********************************************************************************
 * FskGrowableBlobArrayQuery
 ********************************************************************************/

FskErr
FskGrowableBlobArrayQuery(FskConstGrowableBlobArray array, FskGrowableBlobCompare query, const FskBlobRecord *queryData, FskBlobQueryResult *pResult)
{
	BlobQueryPtr		q;
	FskBlobRecord		blobRec;
	FskErr				err;
	UInt32				dataSize, index, numItems, queryOffset;
	ConstBlobEntryPtr	dirPtr;

	*pResult = NULL;														/* Set result in case we don't find anything */
	if (0 == (numItems = FskGrowableBlobArrayGetItemCount(array)))			/* If there is nothing in the array or the array does not exist, ... */
		return kFskErrItemNotFound;											/* ... there is nothing to be found */
	if (!query)	query = &CompareBlobStringsBySize;							/* Supply a generic query if none was provided */
	dataSize	= FskGrowableStorageGetSize(&array->directory->storage),	/* Save the actual data size */
	queryOffset	= (dataSize + 3) & (~3);									/* Bump it up to the next multiple of 4, to align the query result */

	/* Allocate storage for an empty query */
	*pResult = NULL;
	BAIL_IF_ERR(err = FskGrowableStorageSetSize(&array->directory->storage, queryOffset + sizeof(*(q.rs)) - sizeof(q.rs->items)));	/* q is uninitialized, but its value shouldn't matter to sizeof */

	for (index = 0; index < numItems; ++index) {
		BAIL_IF_ERR(err = FskGrowableArrayGetConstPointerToItem(array->directory, index, &dirPtr.vd));
		BAIL_IF_ERR(err = FskGrowableStorageGetConstPointerToItem(array->data, dirPtr.be->offset, (const void**)(&blobRec.data)));
		blobRec.id   = dirPtr.be->id;
		blobRec.size = dirPtr.be->size;
		blobRec.dir  = (void*)(dirPtr.be + 1);
		if (0 == (*query)(queryData, &blobRec))																		/* If the query yields a match, ... */
			BAIL_IF_ERR(err = FskGrowableStorageAppendItem(&array->directory->storage, &index, sizeof(index)));		/* ... append another query result */
	}
	BAIL_IF_ERR(err = FskGrowableStorageGetPointerToItem(&array->directory->storage, queryOffset, &q.vd));	/* Get final pointer to query results */
	q.rs->numItems	= (FskGrowableStorageGetSize(&array->directory->storage) - queryOffset - (sizeof(*(q.rs)) - sizeof(q.rs->items))) / sizeof(q.rs->items[0]);	/* Count query results */
	if (q.rs->numItems) {
		q.rs->unrefine	= 0;													/* Top of the refine stack */
		q.rs->magic	= QUERY_MAGIC;											/* Since query results are ephemeral, this can detect when the blob array has been disrupted */
		*pResult	= q.rs;
	} else {
		err = kFskErrItemNotFound;
	}

bail:
	FskGrowableStorageSetSize(&array->directory->storage, dataSize);		/* Restore the original size */
	return err;
}


/********************************************************************************
 * FskGrowableBlobArrayQueryRefine
 ********************************************************************************/

FskErr
FskGrowableBlobArrayQueryRefine(FskConstGrowableBlobArray array, FskGrowableBlobCompare query, const FskBlobRecord *queryData, FskBlobQueryResult *pResult)
{
	FskErr				err			= kFskErrNone;
	const UInt32		dataSize	= FskGrowableStorageGetSize(&array->directory->storage);
	BlobQueryPtr		q;
	FskBlobRecord		blobRec;
	UInt32				currOffset, index, numMatches, prevOffset, prevSize, subDex;
	ConstBlobEntryPtr	dirPtr;

	BAIL_IF_NULL(array, err, kFskErrItemNotFound);
	BAIL_IF_FALSE(NULL != (q.rs = *pResult) && q.rs->magic == QUERY_MAGIC, err, kFskErrBadState);

	numMatches = q.rs->numItems;
	prevOffset = q.u8                                - array->directory->storage.storage;		/* The offset of the previous query */
	currOffset = (UInt8*)(&q.rs->items[q.rs->numItems]) - array->directory->storage.storage;	/* The offset of the current  query */
	prevSize   = currOffset - prevOffset;														/* The  size  of the previous query */
	BAIL_IF_ERR(err = FskGrowableStorageSetSize(&array->directory->storage, currOffset + sizeof(*(q.rs)) - sizeof(q.rs->items)));

	for (index = 0; index < numMatches; ++index) {
		subDex = q.rs->items[index];
		BAIL_IF_ERR(err = FskGrowableArrayGetConstPointerToItem(array->directory, subDex, &dirPtr.vd));
		BAIL_IF_ERR(err = FskGrowableStorageGetConstPointerToItem(array->data, dirPtr.be->offset, (const void**)(&blobRec.data)));
		blobRec.id   = dirPtr.be->id;
		blobRec.size = dirPtr.be->size;
		blobRec.dir  = (void*)(dirPtr.be + 1);
		if (0 == (*query)(queryData, &blobRec)) {
			BAIL_IF_ERR(err = FskGrowableStorageAppendItem(&array->directory->storage, &subDex, sizeof(subDex)));	/* This can move memory, ... */
			BAIL_IF_ERR(err = FskGrowableStorageGetPointerToItem(&array->directory->storage, prevOffset, &q.vd));	/* ... so we reset the pointer */
		}
	}
	BAIL_IF_ERR(err = FskGrowableStorageGetPointerToItem(&array->directory->storage, currOffset, &q.vd));
	q.rs->numItems	= (FskGrowableStorageGetSize(&array->directory->storage) - currOffset - (sizeof(*(q.rs)) - sizeof(q.rs->items))) / sizeof(q.rs->items[0]);
	q.rs->unrefine	= prevSize;
	q.rs->magic	= QUERY_MAGIC;
	*pResult	= q.rs;
	if (!q.rs->numItems)
		err = kFskErrItemNotFound;

bail:
	FskGrowableStorageSetSize(&array->directory->storage, dataSize);		/* Restore */
	return err;
}


/********************************************************************************
 * FskGrowableBlobArrayQueryUnrefine
 ********************************************************************************/

FskErr
FskGrowableBlobArrayQueryUnrefine(FskBlobQueryResult *pResult)
{
	FskErr				err		= kFskErrNone;
	FskBlobQueryResult	result	= *pResult;

	BAIL_IF_FALSE(result->magic == QUERY_MAGIC, err, kFskErrBadState);
	BAIL_IF_ZERO(result->unrefine, err, kFskErrEmpty);
	result = (FskBlobQueryResult)((char*)result - result->unrefine);
	BAIL_IF_FALSE(result->magic == QUERY_MAGIC, err, kFskErrBadState);
	*pResult = result;
bail:
	return err;
}


/********************************************************************************
 * FskGrowableBlobArrayQueryCount
 ********************************************************************************/

UInt32
FskGrowableBlobArrayQueryCount(FskBlobQueryResult result)
{
	return (result && result->magic == QUERY_MAGIC) ? result->numItems : 0;
}


/********************************************************************************
 * FskGrowableBlobArrayQueryGet
 ********************************************************************************/

FskErr
FskGrowableBlobArrayQueryGet(FskBlobQueryResult result, UInt32 queryIndex, UInt32 *blobIndex)
{
	FskErr	err	= kFskErrNone;
	BAIL_IF_FALSE(result && result->magic == QUERY_MAGIC, err, kFskErrBadState);
	BAIL_IF_FALSE(queryIndex < result->numItems, err, kFskErrOutOfRange);
	*blobIndex = result->items[queryIndex];
bail:
	return err;
}


#if 0
#pragma mark -
#endif /* 0 */
/********************************************************************************
 ********************************************************************************
 *****							Equivalence classes							*****
 ********************************************************************************
 ********************************************************************************/


/********************************************************************************
 * FskGrowableEquivalencesAppendMultipleElementClass
 ********************************************************************************/

FskErr FskGrowableEquivalencesAppendMultipleElementClass(FskGrowableEquivalences coll, UInt32 *id, UInt32 extraBytes, UInt32 numElements, /* const void *ptr, UInt32 size, */ ...) {
	FskEquivalenceBlob	*blob;
	FskErr				err;
	UInt32				i, offset, size;
	const void			*ptr;
	va_list				ap;

	va_start(ap, numElements);
	for (i = 0, offset = 0; i < numElements; ++i) {
		ptr  = va_arg(ap, const void*);
		size = va_arg(ap, UInt32);
		if (!size && ptr)	size = FskStrLen((const char*)ptr);
		offset += size;																						/* Compute the size of storage for all of the elements */
	}
	if (extraBytes)
		offset += extraBytes + numElements;
	va_end(ap);
	offset = FskEquivalenceBlobSize(numElements, offset);													/* Compute the size of storage for the directory and elements */
	BAIL_IF_ERR(err = FskGrowableEquivalencesGetPointerToNewEndClass(coll, offset, id, (void**)(&blob)));	/* Allocate blob accordingly */
	offset = (char*)(&blob->element[blob->numElements = numElements]) - (char*)blob;						/* Set the number of elements, and initialize offset to storage */
	va_start(ap, numElements);
	for (i = 0; i < numElements; ++i) {																		/* For each element, ... */
		ptr  = va_arg(ap, const void*);
		size = va_arg(ap, UInt32);
		if (!size && ptr)	size = FskStrLen((const char*)ptr);
		blob->element[i].offset = offset;																	/* ... determine where it will be stored, ... */
		blob->element[i].size   = size;
		if (ptr) FskMemCopy((char*)blob + offset, ptr, size);												/* ... and store it there */
		offset += size;
		if (extraBytes)
			((char*)blob)[offset++] = 0;
	}
	va_end(ap);
	if (extraBytes) {																						/* Clear padding at end, if specified */
		if (1 == extraBytes)	((char*)blob)[offset] = 0;
		else					FskMemSet((char*)blob + offset, 0, extraBytes);
	}

bail:
	return err;
}


/********************************************************************************
 * FskGrowableEquivalencesAppendElementToClass
 ********************************************************************************/

FskErr FskGrowableEquivalencesAppendElementToClass(FskGrowableEquivalences coll, UInt32 index, const void *ptr, UInt32 size, UInt32 extraBytes) {
	FskEquivalenceBlob	*oBlob		= NULL;
	FskEquivalenceBlob	*blob;
	FskErr				err;
	UInt32				oSize, nSize, i;

	if (!size && ptr)
		size = FskStrLen((const char*)ptr);

	// TODO: Optimize if ((nSize <= oSize) && ((blob->element[blob->numElements-1].offset + blob->element[blob->numElements-1].size + size + 8) == nSize))
	BAIL_IF_ERR(err = FskGrowableEquivalencesGetPointerToClass(coll, index, (void**)(&blob), &oSize));		/* Get existing class */
	for (i = 0, nSize = size; i < blob->numElements; ++i)													/* Compute new size by adding size of new element, ... */
		nSize += blob->element[i].size;																		/* ... size of the old elements, ... */
	if (extraBytes)																							/* If using extra bytes, ... */
		nSize += extraBytes + blob->numElements + 1;														/* also insert zeros between records */
	nSize = FskEquivalenceBlobSize(blob->numElements + 1, nSize);												/* ... and size of the new directory */

	BAIL_IF_ERR(err = FskMemPtrNewFromData(oSize, blob, &oBlob));											/* Copy existing class */
	BAIL_IF_ERR(err = FskGrowableBlobArraySetSizeOfItem(coll, index, nSize));								/* Resize class storage to accommodate the new element */
	BAIL_IF_ERR(err = FskGrowableEquivalencesGetPointerToClass(coll, index, (void**)(&blob), &nSize));		/* Get the resized class */
	blob->numElements = oBlob->numElements + 1;
	for (i = 0, nSize = (char*)(&blob->element[blob->numElements]) - (char*)blob; i < oBlob->numElements;) {
		blob->element[i].offset = nSize;
		blob->element[i].size   = oBlob->element[i].size;
		FskMemCopy((char*)blob + nSize, (char*)oBlob + oBlob->element[i].offset, blob->element[i].size);	/* Copy old elements */
		nSize += blob->element[i++].size;
		if (extraBytes)
			((char*)blob)[nSize++] = 0;
	}
	blob->element[i].offset = nSize;
	blob->element[i].size   = size;
	if (ptr)
		FskMemCopy((char*)blob + nSize, ptr, size);															/* Append new element */
	nSize += size;			/* Offset of the extra bytes */
	if (extraBytes) {
		if (1 == extraBytes)	{	((char*)blob)[nSize] = 0;	((char*)blob)[++nSize] = 0; }
		else					{	FskMemSet((char*)blob + nSize, 0, 1 + extraBytes);		}
	}

bail:
	FskMemPtrDispose(oBlob);
	return err;
}


/********************************************************************************
 * FskGrowableEquivalencesFindClassIndexOfElement
 ********************************************************************************/

FskErr FskGrowableEquivalencesFindClassIndexOfElement(FskConstGrowableEquivalences coll, FskBlobRecord *key, FskEquivalenceElementCompare compare, UInt32 *pIndex) {
	FskErr						err			= kFskErrItemNotFound;
	UInt32						classIndex, numClasses, elemIndex;
	const FskEquivalenceBlob	*equiv;
	FskBlobRecord				elem;

	if (!key->size)	key->size = FskStrLen((const char*)(key->data));

	elem.id  = 0;
	elem.dir = NULL;
	for (classIndex = 0, numClasses = FskGrowableEquivalencesGetClassCount(coll); classIndex < numClasses; ++classIndex) {
		(void)FskGrowableEquivalencesGetConstPointerToClass(coll, classIndex, (const void**)(&equiv), NULL);
		for (elemIndex = 0; elemIndex < equiv->numElements; ++elemIndex) {
			elem.data = FskEquivalenceElementGetPointer(equiv, elemIndex);
			elem.size = FskEquivalenceElementGetSize(   equiv, elemIndex);
			if (compare	? (0 == (*compare)(key, &elem))
						: (key->size == elem.size && 0 == FskMemCompare(key->data, elem.data, elem.size))
			) {
				err = kFskErrNone;
				goto bail;
			}
		}
	}

bail:
	*pIndex = classIndex;
	return err;
}




/********************************************************************************
 * FskGrowableEquivalencesFindClassIndexOfElementInPosition
 ********************************************************************************/

FskErr FskGrowableEquivalencesFindClassIndexOfElementInPosition(FskConstGrowableEquivalences coll, FskBlobRecord *key, UInt32 position, FskEquivalenceElementCompare compare, UInt32 *pIndex) {
	FskErr						err			= kFskErrItemNotFound;
	UInt32						classIndex, numClasses;
	const FskEquivalenceBlob	*equiv;
	FskBlobRecord				elem;

	if (!key->size)	key->size = FskStrLen((const char*)(key->data));

	elem.id  = 0;
	elem.dir = NULL;
	for (classIndex = 0, numClasses = FskGrowableEquivalencesGetClassCount(coll); classIndex < numClasses; ++classIndex) {
		(void)FskGrowableEquivalencesGetConstPointerToClass(coll, classIndex, (const void**)(&equiv), NULL);
		if (position < equiv->numElements) {
			elem.data = FskEquivalenceElementGetPointer(equiv, position);
			elem.size = FskEquivalenceElementGetSize(   equiv, position);
			if (compare	? (0 == (*compare)(key, &elem))
						: (key->size == elem.size && 0 == FskMemCompare(key->data, elem.data, elem.size))
			) {
				err = kFskErrNone;
				goto bail;
			}
		}
	}

bail:
	*pIndex = classIndex;
	return err;
}


/********************************************************************************
 * FskGrowableCStringArrayNewFromString
 ********************************************************************************/

FskErr
FskGrowableCStringArrayNewFromString(const char *str, UInt32 strSize, char delim, FskGrowableBlobArray *pArray)
{
	FskErr	err;
	UInt32	i, z, *d;
	char	*s;

	if (delim == 0)
		err = FskGrowableBlobArrayNewFromStringList((char*)str, 1, 0, pArray);
	else
		err = FskGrowableBlobArrayNewFromString((char*)str, strSize, delim, 1, 0, pArray);
	BAIL_IF_ERR(err);
	for (i = FskGrowableBlobArrayGetItemCount(*pArray); i--;) {
		FskGrowableBlobArrayGetPointerToItem(*pArray, i, (void**)&s, &z, (void**)&d);
		s[z] = 0;	/* Convert the delimiter to a null */
		++d[-1];	/* Increase the size of the string to include the null terminator */
	}

bail:
	return err;
}


/********************************************************************************
 * FskGrowableCStringArrayInsertPrintfItemAtPosition
 ********************************************************************************/

FskErr
FskGrowableCStringArrayInsertPrintfItemAtPosition(FskGrowableBlobArray array, UInt32 index, const char *fmt, ...)
{
	FskGrowableStorage	str	= NULL;
	FskErr				err;
	va_list				ap;

	BAIL_IF_ERR(err = FskGrowableStorageNew(0, &str));
	va_start(ap, fmt);
	err = FskGrowableStorageVAppendF(str, fmt, ap);
	va_end(ap);
	if (kFskErrNone == err)
		err = FskGrowableBlobArrayInsertItemAtPosition(array, index, NULL, FskGrowableStorageGetPointerToCString(str), FskGrowableStorageGetSize(str)+1, NULL);
bail:
	FskGrowableStorageDispose(str);
	return err;
}

