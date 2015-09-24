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
#define FskAssert(x)

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
	FskGrowableArray	directory;			/**< The storage used for the fixed-sized portion of the storage. */
	FskGrowableStorage	data;				/**< The storage used for the variable-sized portion of the  storage. */
	UInt32				lastID;				/**< The counter used for assigning IDs automatically. */
	UInt32				flags;				/**< Boolean flags indicating the state of this blob array. */
};

#define BLOB_DIR_IDSORTED		1			/**< A flag in the FskGrowableBlobArrayRecord indicating that it has been sorted by ID. */

/** A part of the directory entry for blobs. */
struct BlobEntry {
	UInt32	id;								/**< The ID of this blob. */
	UInt32	offset;							/**< The offset of the blob. */
	UInt32	size;							/**< The size of the blob. */
};											/**< A part of the directory entry for blobs. */
typedef struct BlobEntry BlobEntry;			/**< A part of the directory entry for blobs. */


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
		UInt32	maxSize = (size + (BLOCK_SIZE-1)) & (~(BLOCK_SIZE-1));	/* coerce to multiple of BLOCK_SIZE */
		FskMemPtr		p		= storage->storage;
		#ifndef BAD_REALLOC
			BAIL_IF_ERR(err = FskMemPtrRealloc(maxSize, &p));
		#else /* BAD_REALLOC */
			BAIL_IF_ERR(err = FskMemPtrNew(maxSize, &p));
			FskMemMove(p, storage->storage, storage->maxSize);
			FskMemPtrDispose(storage->storage);
		#endif /* BAD_REALLOC */
		storage->storage = p;											/* Only replace the pointer and maxSize if FskMemPtrRealloc was successful */
		storage->maxSize = maxSize;
	}
	storage->size = size;

bail:
	return err;
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
	FskErr	err;
	UInt32	n0, n1;
	char	*str;
	va_list	ap2;

	n0 = FskGrowableStorageGetSize(storage);
	(void)FskGrowableStorageSetSize(storage, n0 + 1);
	BAIL_IF_ERR(err = FskGrowableStorageGetPointerToItem(storage, n0, (void**)&str));
#if !TARGET_OS_WIN32
	va_copy(ap2, ap);
	n1 = vsnprintf(str, 1, fmt, ap2) + 1;
#else /* TARGET_OS_WIN32 */
	ap2 = ap;
	n1 = _vscprintf(fmt, ap2) + 1;
#endif /* TARGET_OS_WIN32 */
	va_end(ap2);
	(void)FskGrowableStorageSetSize(storage, n0 + n1 + 1);

	(void)FskGrowableStorageGetPointerToItem(storage, n0, (void**)&str);
#if !TARGET_OS_WIN32
	n1 = vsnprintf(str, n1 + 1, fmt, ap);
#else /* TARGET_OS_WIN32 */
	n1 = vsnprintf_s(str, n1 + 1, _TRUNCATE, fmt, ap);
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
	if (storage->size >= storage->maxSize) {
		UInt32 maxSize = storage->maxSize;
		if (kFskErrNone != ResizeGrowableStorage(storage, maxSize + 1))
			return NULL;
		ResizeGrowableStorage(storage, maxSize);
	}
	storage->storage[storage->size] = 0;
	return (const char*)(storage->storage);
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
	if (kFskErrNone == (err = FskGrowableStorageAppendItem(&array->storage, item, array->itemSize)))
		array->numItems++;
	return err;
}


/********************************************************************************
 * FskGrowableArrayAppendItems
 ********************************************************************************/

FskErr
FskGrowableArrayAppendItems(FskGrowableArray array, const void *items, UInt32 numItems)
{
	FskErr err;
	if (kFskErrNone == (err = FskGrowableStorageAppendItem(&array->storage, items, array->itemSize * numItems)))
		array->numItems += numItems;
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

bail:
	if (err != kFskErrNone) {
		FskGrowableBlobArrayDispose(array);
		array = NULL;
	}

	*pArray = array;
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
FskGrowableBlobArrayGetDirectoryDataSize(FskGrowableBlobArray array)
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
	UInt32	oldNum	= FskGrowableArrayGetItemCount(array->directory);

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
			if (id != NULL)	*id = dirPtr->id;													/* ... and return it if desired */
		}

		dirPtr->id		= array->lastID;														/* Set the blob ID */
		dirPtr->size	= itemSize;																/* Set blob's size */
		dirPtr->offset	= offset;																/* Set blob's location */
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
	FskErr		err;
	BlobEntry	*dirPtr	= NULL;
	void		*myPtr	= NULL;

	BAIL_IF_NULL(array, err, kFskErrItemNotFound);
	BAIL_IF_ERR(err = FskGrowableArrayGetPointerToItem(array->directory, index, (void**)(void*)(&dirPtr)));
	BAIL_IF_ERR(err = FskGrowableStorageGetPointerToItem(array->data, dirPtr->offset, &myPtr));

bail:
	if (blob    != NULL)	*blob		= (err == kFskErrNone) ? myPtr			: NULL;
	if (dirData != NULL)	*dirData	= (err == kFskErrNone) ? (dirPtr + 1)	: NULL;
	if (size    != NULL)	*size		= (err == kFskErrNone) ? dirPtr->size	: 0;
	return err;
}


/********************************************************************************
 * FskGrowableBlobArrayGetConstPointerToItem
 ********************************************************************************/

FskErr
FskGrowableBlobArrayGetConstPointerToItem(FskConstGrowableBlobArray array, UInt32 index, const void **blob, UInt32 *size, const void **dirData)
{
	FskErr			err;
	const BlobEntry *dirPtr	= NULL;
	const void		*myPtr	= NULL;

	BAIL_IF_NULL(array, err, kFskErrItemNotFound);
	BAIL_IF_ERR(err = FskGrowableArrayGetConstPointerToItem(array->directory, index, (const void**)(const void*)(&dirPtr)));
	BAIL_IF_ERR(err = FskGrowableStorageGetConstPointerToItem(array->data, dirPtr->offset, &myPtr));

bail:
	if (blob    != NULL)	*blob		= (err == kFskErrNone) ? myPtr			: NULL;
	if (dirData != NULL)	*dirData	= (err == kFskErrNone) ? (dirPtr + 1)	: NULL;
	if (size    != NULL)	*size		= (err == kFskErrNone) ? dirPtr->size	: 0;
	return err;
}


/********************************************************************************
 * FskGrowableBlobArrayGetSizeOfItem
 ********************************************************************************/

UInt32
FskGrowableBlobArrayGetSizeOfItem(FskConstGrowableBlobArray array, UInt32 index)
{
	FskErr			err;
	const BlobEntry	*dirPtr;
	UInt32			size	= 0;

	BAIL_IF_NULL(array, err, kFskErrItemNotFound);
	BAIL_IF_ERR(err = FskGrowableArrayGetConstPointerToItem(array->directory, index, (const void**)(const void*)(&dirPtr)));
	size = dirPtr->size;

bail:
	return size;
}


/********************************************************************************
 * FskGrowableBlobArraySetSizeOfItem
 ********************************************************************************/

FskErr
FskGrowableBlobArraySetSizeOfItem(FskGrowableBlobArray array, UInt32 index, UInt32 size)
{
	FskErr		err;
	BlobEntry	*dirPtr;

	BAIL_IF_NULL(array, err, kFskErrItemNotFound);
	BAIL_IF_ERR(err = FskGrowableArrayGetPointerToItem(array->directory, index, (void**)(void*)(&dirPtr)));
	if (size > dirPtr->size) {
		dirPtr->offset = FskGrowableStorageGetSize(array->data);										/* Waste old memory, if not at then end */
		BAIL_IF_ERR(FskGrowableStorageSetSize(array->data, dirPtr->offset + ALIGN_BLOB_SIZE(size)));	/* We could try compaction upon failure */
	}
	dirPtr->size = size;

bail:
	return err;
}


/********************************************************************************
 * FskGrowableBlobArrayAppendDataToBlob
 ********************************************************************************/

FskErr
FskGrowableBlobArrayAppendDataToBlob(FskGrowableBlobArray array, UInt32 index, void *data, UInt32 size, UInt32 *offsetInBlob)
{
	FskErr		err		= kFskErrNone;
	void		*blob;
	UInt32		offset;

	BAIL_IF_ZERO(size, offset, NONEXISTENT_OFFSET);												/* If zero size, return NONEXISTENT_OFFSET */
	offset = FskGrowableBlobArrayGetSizeOfItem(array, index);									/* Get old size */
	BAIL_IF_ERR(err = FskGrowableBlobArraySetSizeOfItem(array, index, offset + size));			/* Set new size */
	BAIL_IF_ERR(err = FskGrowableBlobArrayGetPointerToItem(array, index, &blob, NULL, NULL));	/* Get pointer to blob */
	blob = (void*)((char*)blob + offset);														/* Get pointer to new data in blob */
	if (data != NULL)	FskMemCopy(blob, data, size);											/* Copy data to blob */
	else				FskMemSet(blob, 0, size);												/* Clear new part of blob */

bail:
	if (offsetInBlob != NULL)	*offsetInBlob = offset;

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
	FskErr		err;
	BlobEntry	*d0, *d1;
	UInt32	t;																					/* The id's and indices stay the same */

	BAIL_IF_NULL(array, err, kFskErrItemNotFound);
	BAIL_IF_ERR(err = FskGrowableArrayGetPointerToItem(array->directory, index0, (void**)(void*)(&d0)));
	BAIL_IF_ERR(err = FskGrowableArrayGetPointerToItem(array->directory, index1, (void**)(void*)(&d1)));
	t = d0->offset;		d0->offset = d1->offset;	d1->offset = t;								/* Swap offsets */
	t = d0->size;		d0->size   = d1->size;		d1->size   = t;								/* Swap sizes */
	if ((t = array->directory->itemSize - sizeof(BlobEntry)) > 0) {								/* Swap extra directory information */
		UInt32	*p0, *p1, i;
		for (i = t / sizeof(UInt32), p0 = (UInt32*)(d0 + 1), p1 = (UInt32*)(d1 + 1); i--; ) {
			t     = *p0;
			*p0++ = *p1;
			*p1++ = t;
		}
	}

bail:
	return err;
}


/********************************************************************************
 * FskGrowableBlobArraySortItems
 ********************************************************************************/

FskErr
FskGrowableBlobArraySortItems(FskGrowableBlobArray array, FskCompareFunction comp)
{
	return array ? FskGrowableArraySortItems(array->directory, comp) : kFskErrNotDirectory;
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
 * FskGrowableBlobArrayBSearchItems
 ********************************************************************************/

void*
FskGrowableBlobArrayBSearchItems(FskConstGrowableBlobArray array, const void *key, FskCompareFunction comp)
{
	BlobEntry	*dirPtr;
	void		*p			= NULL;

	if ((dirPtr = FskGrowableArrayBSearchItems(array->directory, key, comp)) != NULL)
		(void)FskGrowableStorageGetPointerToItem(array->data, dirPtr->offset, &p);

	return p;
}


/********************************************************************************
 * FskGrowableBlobArrayCompact
 ********************************************************************************/

FskErr
FskGrowableBlobArrayCompact(FskConstGrowableBlobArray array)
{
	FskErr		err;
	BlobEntry	*dirPtr;
	UInt8		*data;
	UInt8		*tmp	= NULL;
	UInt32		z, n, dirSize;

	BAIL_IF_NULL(array, err, kFskErrNotDirectory);
	z = FskGrowableStorageGetSize(array->data);	/* The current size of the data is always larger that the final result */
	BAIL_IF_ERR(err = FskMemPtrNew(z, &tmp));															/* Allocate tmp */
	BAIL_IF_ERR(err = FskGrowableStorageGetPointerToItem(array->data, 0, (void**)(void*)(&data)));		/* Get data ptr */
	FskMemCopy(tmp, data, z);																			/* Copy data to tmp */

	BAIL_IF_ERR(err = FskGrowableArrayGetPointerToItem(array->directory, 0, (void**)(void*)(&dirPtr)));
	n = FskGrowableArrayGetItemCount(array->directory);
	dirSize	= array->directory->itemSize;
	for (z = 0; n--; z += ALIGN_BLOB_SIZE(dirPtr->size), dirPtr = (BlobEntry*)((char*)dirPtr + dirSize)) {
		FskMemCopy(data + z, tmp + dirPtr->offset, dirPtr->size);
		dirPtr->offset = z;
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
	FskErr			err;
	const BlobEntry	*dirPtr, *dir0;

	*index = kFskGrowableBlobArrayUnassignedID;
	BAIL_IF_NULL((dirPtr = GetConstDirectoryEntryFromIdOfItemInGrowableBlobArray(array, id)), err, kFskErrItemNotFound);
	BAIL_IF_ERR(err = FskGrowableArrayGetConstPointerToItem(array->directory, 0, (const void**)(const void*)(&dir0)));
	*index = (UInt32)(((const char*)dirPtr - (const char*)dir0) / array->directory->itemSize);

bail:
	return err;
}


/********************************************************************************
 * FskGrowableBlobArrayGetIDOfItem
 ********************************************************************************/

FskErr
FskGrowableBlobArrayGetIDOfItem(FskConstGrowableBlobArray array, UInt32 index, UInt32 *id)
{
	FskErr			err;
	const BlobEntry	*dirPtr;

	BAIL_IF_NULL(array, err, kFskErrItemNotFound);
	err = FskGrowableArrayGetConstPointerToItem(array->directory, index, (const void**)(const void*)(&dirPtr));

bail:
	*id = (err == kFskErrNone) ? dirPtr->id : 0;	/* Returns 0 if error, since 0 is an invalid id */
	return err;
}


/********************************************************************************
 * FskGrowableBlobArraySetIDOfItem
 ********************************************************************************/

FskErr
FskGrowableBlobArraySetIDOfItem(FskGrowableBlobArray array, UInt32 index, UInt32 id)
{
	FskErr		err			= kFskErrNone;
	BlobEntry	*dirPtr;

	BAIL_IF_NULL(array, err, kFskErrItemNotFound);
	BAIL_IF_ERR(err = FskGrowableArrayGetPointerToItem(array->directory, index, (void**)(void*)(&dirPtr)));
	dirPtr->id = id;
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
