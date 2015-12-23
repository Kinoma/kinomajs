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
	\file	FskGrowableStorage.h
	\brief	Growable Storage.
*/
#ifndef __FSKGROWABLESTORAGE__
#define __FSKGROWABLESTORAGE__

#include <stdarg.h>
#ifndef __FSKUTILITIES__
	#include "FskUtilities.h"
#endif /* __FSKUTILITIES__ */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */




/* Opaque Structure definitions */
typedef       struct FskGrowableStorageRecord	*FskGrowableStorage;			/**< Pointer to a            record that represents growable storage. */
typedef const struct FskGrowableStorageRecord	*FskConstGrowableStorage;		/**< Pointer to an immutable record that represents growable storage. */
typedef       struct FskGrowableArrayRecord		*FskGrowableArray;				/**< Pointer to a            record that represents a growable array. */
typedef const struct FskGrowableArrayRecord		*FskConstGrowableArray;			/**< Pointer to an immutable record that represents a growable array. */
typedef       struct FskGrowableBlobArrayRecord	*FskGrowableBlobArray;			/**< Pointer to a            record that represents a growable blob array. */
typedef const struct FskGrowableBlobArrayRecord	*FskConstGrowableBlobArray;		/**< Pointer to an immutable record that represents a growable blob array. */



/********************************************************************************
 ********************************************************************************
 *****							Variable sized items						*****
 ********************************************************************************
 ********************************************************************************/


				/* Constructor and destructor */


/** Constructor for growable storage.
 *	\param[in]	maxSize	Initial allocation of memory, to avoid realloc later, for performance.
 *						The size, though, will be initialized to 0.
 *	\param[out]	storage	The resultant growable storage object.
 *	\return		kFskErrNone	if the operation was completed successfully.
 **/
FskAPI(FskErr)	FskGrowableStorageNew(UInt32 maxSize, FskGrowableStorage *storage);


/** Destructor for growable storage.
 *	\param[in]	storage	The growable storage object to be disposed.
 **/
FskAPI(void)	FskGrowableStorageDispose(FskGrowableStorage storage);


/** Reallocate memory so that maxSize==size.
 *	This may or may not save on allocated memory, depending on FskMemPtrRealloc behavior.
 *	This is typically done when the object is expected to be around for a while without changes.
 *	\param[in,out]	storage	The growable storage object to be disposed.
 **/
FskAPI(void)	FskGrowableStorageMinimize(FskGrowableStorage storage);


/** Disengage the growable storage object from the storage itself, disposing the growable object and retaining only the storage.
 *	FskGrowableStorageMinimize() is called first, though, to minimize the memory footprint.
 *	\param[in]	storage	The growable storage object to be disengaged.
 *	\param[out]	mem		The lone raw memory pointer, containing the actual storage.
 *						This should be disposed later with FskMemPtrDispose().
 *	\return		the number of bytes of storage returned in mem.
 **/
FskAPI(UInt32)	FskGrowableStorageDisengage(FskGrowableStorage storage, void **mem);


/** Disengage the growable storage object from the storage itself, disposing the growable object and retaining only the storage, represented as a C-string.
 *	FskGrowableStorageMinimize() is called first, though, to minimize the memory footprint.
 *	\param[in]	storage	The growable storage object to be disengaged.
 *	\param[out]	mem		The lone raw memory pointer, containing the actual storage.
 *						This should be disposed later with FskMemPtrDispose().
 *	\return		the number of bytes of storage returned in mem, including the terminating NULL byte.
 **/
FskAPI(UInt32)	FskGrowableStorageDisengageCString(FskGrowableStorage storage, char **mem);


				/* Size */


/** Get the current size of the growable storage.
 *	\param[in,out]	storage	The growable storage object to be queried.
 *	\return			the size, in bytes, of the storage.
 **/
FskAPI(UInt32)	FskGrowableStorageGetSize(FskConstGrowableStorage storage);


/** Set the size of the growable storage.
 *	This may make the storage large or smaller.
 *	If made smaller, the storage is simply truncated.
 *	If made larger, the contents of the enlarged region are undefined.
 *	\param[in,out]	storage	The growable storage object to be resized.
 *	\param[in]		size	The desired size of the growable storage.
 *	\return			kFskErrNone	if the operation was successful.
 **/
FskAPI(FskErr)	FskGrowableStorageSetSize(FskGrowableStorage storage, UInt32 size);


				/* Access to data */


/** Copy an item from the growable storage.
 *	\param[in]	storage		The growable storage object to be accessed.
 *	\param[in]	offset		The offset of the data to be accessed in the growable storage object.
 *	\param[out]	item		The location to copy the data from the growable storage.
 *	\param[in]	itemSize	The number of bytes in the item.
 *	\return		kFskErrNone	if the operation was successful.
 **/
FskAPI(FskErr)	FskGrowableStorageGetItem(FskConstGrowableStorage storage, UInt32 offset, void *item, UInt32 itemSize);


/** Get a mutable pointer to the specified location in the growable storage.
 *	\param[in]	storage		The growable storage object to be accessed.
 *	\param[in]	offset		The offset of the data to be accessed in the growable storage object.
 *	\param[out]	ptr			The desired pointer.
 *	\return		kFskErrNone			if the operation was successful.
 *	\return		kFskErrItemNotFound	if no item was found at the specified location.
 **/
FskAPI(FskErr)	FskGrowableStorageGetPointerToItem(FskGrowableStorage storage, UInt32 offset, void **ptr);


/** Get an immutable pointer to the specified location in the growable storage.
 *	\param[in]	storage		The growable storage object to be accessed.
 *	\param[in]	offset		The offset of the data to be accessed in the growable storage object.
 *	\param[out]	ptr			The desired pointer.
 *	\return		kFskErrNone			if the operation was successful.
 *	\return		kFskErrItemNotFound	if no item was found at the specified location.
 **/
FskAPI(FskErr)	FskGrowableStorageGetConstPointerToItem(FskConstGrowableStorage storage, UInt32 offset, const void **ptr);


/** Insert a new item of the specified size at the specified location in the growable storage, and return a pointer thereto.
 *	\param[in,out]	storage			The growable storage object to be accessed.
 *	\param[in]		offset			The offset of the data to be created in the growable storage object.
 *	\param[in]		itemSize		The desired number of bytes in the item to be created.
 *	\param[out]		ptr				The desired pointer.
 *	\return			kFskErrNone		if the operation was successful.
 *	\return			kFskErrBadData	if no item could be created at the specified location, because it would create a gap.
 **/
FskAPI(FskErr)	FskGrowableStorageGetPointerToNewItem(FskGrowableStorage storage, UInt32 offset, UInt32 itemSize, void **ptr);


/** Insert a new item of the specified size at the end of the growable storage, and return a pointer thereto.
 *	\param[in,out]	storage		The growable storage object to be accessed.
 *	\param[in]		itemSize	The desired number of bytes in the item to be created.
 *	\param[out]		ptr			The desired pointer.
 *	\return			kFskErrNone	if the operation was successful.
 **/
FskAPI(FskErr)	FskGrowableStorageGetPointerToNewEndItem(FskGrowableStorage storage, UInt32 itemSize, void **ptr);


				/* Editing operations */


/** Remove an item from the growable storage.
 *	\param[in,out]	storage		The growable storage object to be modified.
 *	\param[in]		offset		The offset of the data to be removed from the growable storage object.
 *	\param[in]		itemSize	The desired number of bytes in the item to be removed.
 *	\return			kFskErrNone	if the operation was successful.
 **/
FskAPI(void)	FskGrowableStorageRemoveItem(FskGrowableStorage storage, UInt32 offset, UInt32 itemSize);


/** Append an item to the end of the growable storage.
 *	\param[in,out]	storage		The growable storage object to be modified.
 *	\param[in]		item		The item to be appended to the growable storage object.
 *	\param[in]		itemSize	The size of the item to be appended.
 *	\return			kFskErrNone	if the operation was successful.
 **/
FskAPI(FskErr)	FskGrowableStorageAppendItem(FskGrowableStorage storage, const void *item, UInt32 itemSize);


/** Insert an item into the middle of the growable storage.
 *	\param[in,out]	storage		The growable storage object to be modified.
 *	\param[in]		offset		The offset location where the object is to be inserted into the growable storage object.
 *	\param[in]		item		The item to be inserted into the growable storage object.
 *	\param[in]		itemSize	The size of the item to be inserted.
 *	\return			kFskErrNone	if the operation was successful.
 **/
FskAPI(FskErr)	FskGrowableStorageInsertItemAtPosition(FskGrowableStorage storage, UInt32 offset, const void *item, UInt32 itemSize);


/** Replace an item at an arbitrary location in the growable storage.
 *	\param[in,out]	storage		The growable storage object to be modified.
 *	\param[in]		item		The item to be inserted into the growable storage object.
 *	\param[in]		offset		The offset location where the object is to be replaced in the growable storage object.
 *	\param[in]		oldSize		The size of the item to be replaced.
 *	\param[in]		newSize		The size of the new item to take the place of the old one.
 *	\return			kFskErrNone	if the operation was successful.
 **/
FskAPI(FskErr)	FskGrowableStorageReplaceItem(FskGrowableStorage storage, const void *item, UInt32 offset, UInt32 oldSize, UInt32 newSize);


/** Printf a string and append it to the growable storage.
 *	\param[in]		storage		The growable storage object to be accessed.
 *	\param[in]		fmt			The printf-style format for the string to be printed.
 *	\param[in]		ap			Arguments to be printed.
 *	\return			kFskErrNone	if the operation was successful.
 **/
FskAPI(FskErr)	FskGrowableStorageVAppendF(FskGrowableStorage storage, const char *fmt, va_list ap)
				#if defined(__GNUC__)
					__attribute__ ((format(printf, 2, 0)))
				#endif
;


/** Printf a string and append it to the growable storage.
 *	\param[in]		storage		The growable storage object to be accessed.
 *	\param[in]		fmt			The printf-style format for the string to be printed.
 *	\param[in]		...			Arguments to be printed.
 *	\return			kFskErrNone	if the operation was successful.
 **/
FskAPI(FskErr)	FskGrowableStorageAppendF(FskGrowableStorage storage, const char *fmt, ...)
				#if defined(__GNUC__)
					__attribute__ ((format(printf, 2, 3)))
				#endif
;



/** Return a C string pointer to the growable storage.
 *	\param[in]		storage		The growable storage object to be accessed.
 *	\return			a pointer to the storage, useable as a C-string.
 **/
FskAPI(const char*)	FskGrowableStorageGetPointerToCString(FskGrowableStorage storage);


/********************************************************************************
 ********************************************************************************
 *****							Fixed sized items							*****
 ********************************************************************************
 ********************************************************************************/

				/* Constructor and destructor */


/** Constructor for a growable array.
 *	\param[in]	itemSize	The size of the items in the array.
 *	\param[in]	maxItems	The expected number of items initially allocated in the array, for performance.
 *							The number of items, though, will be initialized to 0.
 *	\param[out]	array		The resultant growable array object.
 *	\return		kFskErrNone	if the operation was completed successfully.
 **/
FskAPI(FskErr)	FskGrowableArrayNew(UInt32 itemSize, UInt32 maxItems, FskGrowableArray *array);


/** Destructor for a growable array.
 *	\param[in]	array		The growable array object to be disposed.
 *	\return		kFskErrNone	if the operation was completed successfully.
 **/
FskAPI(void)	FskGrowableArrayDispose(FskGrowableArray array);


/** Minimize the amount of memory taken by the growable array object.
 *	This may or may not save on allocated memory, depending on FskMemPtrRealloc behavior.
 *	This is typically done when the object is expected to be around for a while without changes.
 *	\param[in]	array		The growable array object to be minimized.
 *	\return		kFskErrNone	if the operation was completed successfully.
 **/
FskAPI(void)	FskGrowableArrayMinimize(FskGrowableArray array);


/** Disengage the growable array object from the storage itself, disposing the growable object and retaining only the storage.
 *	FskGrowableArrayMinimize() is called first, though, to minimize the memory footprint.
 *	\param[in]	array	The growable array object to be disengaged.
 *	\param[out]	mem		The lone raw memory pointer, containing the actual storage.
 *						This should be disposed later with FskMemPtrDispose().
 *	\return		the number of bytes of storage returned in mem.
 **/
FskAPI(UInt32)	FskGrowableArrayDisengage(FskGrowableArray array, void **mem);


				/* Size and Count */


/** Get the current number of items in the growable array.
 *	\param[in,out]	array	The growable array object to be queried.
 *	\return			the number of items in the growable array.
 **/
FskAPI(UInt32)	FskGrowableArrayGetItemCount(FskConstGrowableArray array);


/** Set the number of items in the growable array.
 *	This may make the array large or smaller.
 *	If made smaller, the array is simply truncated.
 *	If made larger, the contents of the new array elements are undefined.
 *	\param[in,out]	array		The growable array object to be modified.
 *	\param[in]		numItems	The desired capacity of items.
 *	\return			the number of items in the growable array.
 **/
FskAPI(FskErr)	FskGrowableArraySetItemCount(FskGrowableArray array, UInt32 numItems);


/**	Get the size of each element in the array.
 *	\param[in]	array	the array to be queried.
 *	\return		the size of each element in the array.
 **/
FskAPI(UInt32)	FskGrowableArrayGetItemSize(FskConstGrowableArray array);


/**	Set the size of each element in the array.
 *	The actual storage bytes are unmodified, but will no longer line up with the old array.
 *	This allows you to resize the contents in-place, e.g. convert from Fixed to double.
 *	\param[in,out]	array		the array to be reshaped.
 *	\param[in]		itemSize	the new size of elements in the array.
 *	\return			kFskErrNone	if the operation was successful.
 **/
FskAPI(FskErr)	FskGrowableArraySetItemSize(FskGrowableArray array, UInt32 itemSize);


				/* Editing operations */


/**	Remove an item from the array.
 *	\param[in,out]	array	the array to be modified.
 *	\param[in]		index	the index of the item to be removed.
 *	\return			kFskErrNone	if the operation was successful.
 **/
FskAPI(void)	FskGrowableArrayRemoveItem(FskGrowableArray array, UInt32 index);


/**	Append an item to the end of the array.
 *	\param[in,out]	array	the array to be modified.
 *	\param[in]		item	the item to be appended.
 *	\return			kFskErrNone	if the operation was successful.
 **/
FskAPI(FskErr)	FskGrowableArrayAppendItem(FskGrowableArray array, const void *item);


/**	Append an array of items to the end of the growable array.
 *	\param[in,out]	array		the array to be modified.
 *	\param[in]		items		the items to be appended.
 *	\param[in]		numItems	the number of items to be appended.
 *	\return			kFskErrNone	if the operation was successful.
 **/
FskAPI(FskErr)	FskGrowableArrayAppendItems(FskGrowableArray array, const void *items, UInt32 numItems);


/**	Append an array of items to the end of the growable array, but in reversed order.
 *	\param[in,out]	array		the array to be modified.
 *	\param[in]		items		the items to be appended.
 *	\param[in]		numItems	the number of items to be appended.
 *	\return			kFskErrNone	if the operation was successful.
 **/
FskAPI(FskErr)	FskGrowableArrayAppendReversedItems(FskGrowableArray array, const void *items, UInt32 numItems);


/**	Insert an item at an arbitrary position in the growable array.
 *	\param[in,out]	array		the array to be modified.
 *	\param[in]		index		the index where the item is to be inserted.
 *	\param[in]		item		the item to be inserted.
 *	\return			kFskErrNone	if the operation was successful.
 **/
FskAPI(FskErr)	FskGrowableArrayInsertItemAtPosition(FskGrowableArray array, UInt32 index, const void *item);


/**	Replace an item in the growable array.
 *	\param[in,out]	array		the array to be modified.
 *	\param[in]		index		the index where the item is to be replaced.
 *	\param[in]		item		the replacement item.
 *	\return			kFskErrNone	if the operation was successful.
 **/
FskAPI(FskErr)	FskGrowableArrayReplaceItem(FskGrowableArray array, const void *item, UInt32 index);


/**	Swap two items in the growable array.
 *	\param[in,out]	array		the array to be modified.
 *	\param[in]		index0		the index of one item.
 *	\param[in]		index1		the index of the other item item.
 *	\return			kFskErrNone	if the operation was successful.
 **/
FskAPI(FskErr)	FskGrowableArraySwapItems(FskGrowableArray array, UInt32 index0, UInt32 index1);


				/* Sort and search */


/**	Sort the items in the growable array.
 *	\param[in,out]	array		the array to be sorted.
 *	\param[in]		comp		the function to be used to compare items in the array.
 *	\return			kFskErrNone	if the operation was successful.
 **/
FskAPI(FskErr)	FskGrowableArraySortItems(FskGrowableArray array, FskCompareFunction comp);


/**	Do a binary search for an item in a sorted growable array.
 *	The array should be pre-sorted with FskGrowableArraySortItems().
 *	\param[in]	array		the array to be searched.
 *	\param[in]	key			the key to look for.
 *	\param[in]	comp		the function to be used to compare items in the array.
 *	\return		a pointer to the requested item, if successful.
 *	\return		NULL,		if the operation was not successful.
 **/
FskAPI(void)*	FskGrowableArrayBSearchItems(FskConstGrowableArray array, const void *key, FskCompareFunction comp);


				/* Access to data */


/**	Get an item from a growable array.
 *	\param[in,out]	array		the growable array to accessed.
 *	\param[in]		index		the index of the desired item in the array.
 *	\param[out]		item		the location to which the selected item is copied.
 *	\return			kFskErrNone	if the operation was successful.
 **/
FskAPI(FskErr)	FskGrowableArrayGetItem(FskConstGrowableArray array, UInt32 index, void *item);


/**	Get a mutable pointer to an item in a growable array.
 *	\param[in]		array		the growable array to accessed.
 *	\param[in]		index		the index of the desired item in the array.
 *	\param[out]		ptr			a place to store a pointer to the selected item.
 *	\return			kFskErrNone	if the operation was successful.
 **/
FskAPI(FskErr)	FskGrowableArrayGetPointerToItem(FskGrowableArray array, UInt32 index, void **ptr);


/**	Get an immutable pointer to an item in a growable array.
 *	\param[in]		array		the growable array to accessed.
 *	\param[in]		index		the index of the desired item in the array.
 *	\param[out]		ptr			a place to store a pointer to the selected item.
 *	\return			kFskErrNone	if the operation was successful.
 **/
FskAPI(FskErr)	FskGrowableArrayGetConstPointerToItem(FskConstGrowableArray array, UInt32 index, const void **ptr);


/**	Insert a new item into a growable array, and return a pointer thereto.
 *	\param[in]		array		the growable array to accessed.
 *	\param[in]		index		the desired index for the new item in the array.
 *	\param[out]		ptr			a place to store a pointer to the selected item.
 *	\return			kFskErrNone	if the operation was successful.
 **/
FskAPI(FskErr)	FskGrowableArrayGetPointerToNewItem(FskGrowableArray array, UInt32 index, void **ptr);


/**	Insert a new item at the end of a growable array, and return a pointer thereto.
 *	\param[in]		array		the growable array to accessed.
 *	\param[out]		ptr			a place to store a pointer to the selected item.
 *	\return			kFskErrNone	if the operation was successful.
 **/
FskAPI(FskErr)	FskGrowableArrayGetPointerToNewEndItem(FskGrowableArray array, void **ptr);


/**	Get a mutable pointer to the last item in the growable array.
 *	\param[in]		array		the growable array to accessed.
 *	\param[out]		ptr			a place to store a pointer to the last item item.
 *	\return			kFskErrNone	if the operation was successful.
 **/
FskAPI(FskErr)	FskGrowableArrayGetPointerToLastItem(FskGrowableArray array, void **ptr);


/**	Get an immmutable pointer to the last item in the growable array.
 *	\param[in]		array		the growable array to accessed.
 *	\param[out]		ptr			a place to store a pointer to the last item item.
 *	\return			kFskErrNone	if the operation was successful.
 **/
FskAPI(FskErr)	FskGrowableArrayGetConstPointerToLastItem(FskConstGrowableArray array, const void **ptr);




/********************************************************************************
 ********************************************************************************
 *****									BLOBS								*****
 *****							Binary Large Objects						*****
 *****																		*****
 ***** These are composed of two parts:										*****
 *****		- a fixed size part, in the directory							*****
 *****		- a variable sized part											*****
 ***** An ID can be used to access either part, and is either assigned		*****
 ***** sequentially from 1, or can be explicitly given when adding items.	*****
 ********************************************************************************
 ********************************************************************************/

#define kFskGrowableBlobArrayUnassignedID	0xFFFFFFFF	/**< Indicates that no ID is assigned to the blob. */


/** Public blob record.
 * This is used in sorting, searching and query procs.
 */
struct FskBlobRecord {
	UInt32	id;									/**< The ID of the blob. */
	UInt32	size;								/**< The size of the blob. */
	void	*data;								/**< The variable-sized data for the blob. */
	void	*dir;								/**< The fixed-sized portion (directory) of the blob.*/
};
typedef struct FskBlobRecord FskBlobRecord;		/**< Public blob record. */


/** Comparison function used for blobs.
 *	\param[in]	blob1	pointer to blob 1.
 *	\param[in]	blob2	pointer to blob 2.
 *	\return		{-1, 0, +1} if item 1 is { < , ==,  > } item 2.
 */
typedef int (*FskGrowableBlobCompare)(const FskBlobRecord *blob1, const FskBlobRecord *blob2);


				/* Constructor and destructor */


/**	Allocate a new growable blob array.
 *	\param[in]		blobSize	the typical size of a blob. This is used to preallocate (blobSize * maxBlobs) bytes
 *								of blob storage, for performance, and is not critical.
 *	\param[in]		maxBlobs	the expected maximum number of blobs. This is used to preallocate memory for performance.
 *	\param[in]		dirDataSize	the size of an element in the directory. Typically this is fixed, but need not be.
 *								This is also used to preallocate (dirDataSize * maxBlobs) bytes of storage for performance.
 *	\param[out]		pArray		a place to store the new growable blob array.
 *	\return			kFskErrNone	if the operation was successful.
 **/
FskAPI(FskErr)	FskGrowableBlobArrayNew(UInt32 blobSize, UInt32 maxBlobs, UInt32 dirDataSize, FskGrowableBlobArray *pArray);


/**	Allocate a new growable blob array from a string.
 *	The string is partitioned using the specified delimiter character into separate blobs
 *	\param[in]	str			the string.
 *	\param[in]	strSize		the length of the string, or 0 to use FskStrLen() to find the length.
 *	\param[in]	delim		the character used to separate records,typically '\n' for typical strings and '\0' for string lists.
 *	\param[in]	makeCopy	if true, copy the string data to the blob array;
 *							if false, use the string itself as data storage,so it will be disposed when the blob array is disposed.
 *	\param[in]	dirDataSize	the amount of storage to be used in the directory for each record.
 *	\param[out]	pArray		a place to store the new growable blob array.
 *	\return		kFskErrNone	if the operation was successful.
 **/
FskAPI(FskErr)	FskGrowableBlobArrayNewFromString(char *str, UInt32 strSize, char delim, Boolean makeCopy, UInt32 dirDataSize, FskGrowableBlobArray *pArray);


/**	Allocate a new growable blob array from a string list.
 *	\param[in]	str			the string.
 *	\param[in]	makeCopy	if true, copy the string data to the blob array;
 *							if false, use the string itself as data storage, so it will be disposed when the blob array is disposed.
 *	\param[in]	dirDataSize	the amount of storage to be used in the directory for each record.
 *	\param[out]	pArray		a place to store the new growable blob array.
 *	\return		kFskErrNone	if the operation was successful.
 **/
FskAPI(FskErr)	FskGrowableBlobArrayNewFromStringList(char *str, Boolean makeCopy, UInt32 dirDataSize, FskGrowableBlobArray *pArray);


/** Dispose of a growable blob array.
 *	\param[in]	array	the array to be disposed.
 **/
FskAPI(void)	FskGrowableBlobArrayDispose(FskGrowableBlobArray array);


				/* Size and count */


/** Get the number of items in the growable blob array.
 *	\param[in]	array	the array to be queried.
 *	\return		the number of items in the growable blob array.
 **/
FskAPI(UInt32)	FskGrowableBlobArrayGetItemCount( FskConstGrowableBlobArray array);


/** Set the number of items in the growable blob array.
 * If decreased, the blob storage of the last items is deleted along with their directory entries.
 * If increased, the new items have undefined entries in their directory and no blob storage.
 *	\param[in,out]	array		the array to be modified.
 *	\param[in]		numItems	the desired number of items in the array.
 *	\return			kFskErrNone	if the operation was successful.
 **/
FskAPI(FskErr)	FskGrowableBlobArraySetItemCount(      FskGrowableBlobArray array, UInt32 numItems);


/** Get the size of the blob storage for a particular item in the growable blob array.
 *	\param[in,out]	array	the array to be queried.
 *	\param[in]		index	the index of the item in the arrray to be queried.
 *	\return			the size of the blob storage for the selected item in the growable blob array.
 **/
FskAPI(UInt32)	FskGrowableBlobArrayGetSizeOfItem(FskConstGrowableBlobArray array, UInt32 index);


/** Set the size of the blob storage for a particular item in the growable blob array.
 *	\param[in,out]	array	the array to be modified.
 *	\param[in]		index	the index of the item in the array whose blob storage is to be modified.
 *	\param[in]		size	the desired size of the blob storage for the specified item in the array.
 *	\return			kFskErrNone	if the operation was successful.
 **/
FskAPI(FskErr)	FskGrowableBlobArraySetSizeOfItem(     FskGrowableBlobArray array, UInt32 index, UInt32 size);


				/* Directory access */


/** Get the size of the directory entries.
 *	This may be larger than specified with FskGrowableBlobArraySetDirectoryDataSize() or FskGrowableBlobArrayNew()
 *	in order to satisfy alignment requirements.
 *	\param[in,out]	array	the array to be queried.
 *	\return			the size of the each directory entry.
 **/
FskAPI(UInt32)	FskGrowableBlobArrayGetDirectoryDataSize(FskConstGrowableBlobArray array);


/** Set the size of each directory entry in the growable blob array.
 *	\param[in,out]	array		the array to be modified.
 *	\param[in]		dirDataSize	the desired size for each directory entry.
 *	\return			kFskErrNone	if the operation was successful.
 **/
FskAPI(FskErr)	FskGrowableBlobArraySetDirectoryDataSize(FskGrowableBlobArray array, UInt32 dirDataSize);


				/* Item IDs */


/** ID from index: get the ID for the specified item in the growable blob array.
 *	\param[in]	array		the array to be queried.
 *	\param[in]	index		the index of the item to be queried.
 *	\param[out]	id			location to store the ID of the selected item.
 *	\return		kFskErrNone	if the operation was successful.
 **/
FskAPI(FskErr)	FskGrowableBlobArrayGetIDOfItem(FskConstGrowableBlobArray array, UInt32 index, UInt32 *id);


/** Set the ID for the specified item in the growable blob array.
 *	This will cause a previously sorted array to become unsorted.
 *	\param[in]	array		the array to be modified.
 *	\param[in]	index		the index of the item to be modified.
 *	\param[out]	id			the id to store with the selected item.
 *							it is up to the caller to assure that the id is unique in the array.
 *	\return		kFskErrNone	if the operation was successful.
 **/
FskAPI(FskErr)	FskGrowableBlobArraySetIDOfItem(FskGrowableBlobArray array, UInt32 index, UInt32 id);


/** Index from ID: get the index for the specified item in the growable blob array.
 *	This will be faster if the array is pre-sorted.
 *	\param[in]	array		the array to be queried.
 *	\param[in]	id			the ID of the item to be queried.
 *	\param[out]	index		location to store the index of the selected item.
 *	\return		kFskErrNone	if the operation was successful.
 **/
FskAPI(FskErr)	FskGrowableBlobArrayGetIndexFromIDOfItem(FskConstGrowableBlobArray array, UInt32 id, UInt32 *index);


				/* Data access - new items. Pass *id=kFskGrowableBlobArrayUnassignedID or id=NULL to have the id assigned automatically */


/** Create a new item in the growable blob array at an arbitrary location, and return pointers to access it.
 *	\param[in,out]	array		the array to be modified.
 *	\param[in]		index		the location where the item is to be created.
 *	\param[in]		itemSize	the size of the blob storage to be allocated for the new item.
 *	\param[in,out]	id			the id of the item.
 *								On input, the desired ID, or kFskGrowableBlobArrayUnassignedID or NULL to have the id assigned automatically.
 *								On output, the assigned ID.
 *	\param[out]		ptr			a location to store a pointer to the  blob storage   for the item.
 *	\param[out]		dirData		a location to store a pointer to the directory entry for the item.
 *	\return			kFskErrNone	if the operation was successful.
 **/
FskAPI(FskErr)	FskGrowableBlobArrayGetPointerToNewItem(FskGrowableBlobArray array, UInt32 index, UInt32 itemSize, UInt32 *id, void **ptr, void **dirData);


/** Create a new item at the end of the the growable blob array , and return pointers to access it.
 *	\param[in,out]	array		the array to be modified.
 *	\param[in]		itemSize	the size of the blob storage to be allocated for the new item.
 *	\param[in,out]	id			the id of the item.
 *								On input, the desired ID, or kFskGrowableBlobArrayUnassignedID or NULL to have the id assigned automatically.
 *								On output, the assigned ID.
 *	\param[out]		ptr			a location to store a pointer to the  blob storage   for the item.
 *	\param[out]		dirData		a location to store a pointer to the directory entry for the item.
 *	\return			kFskErrNone	if the operation was successful.
 **/
FskAPI(FskErr)	FskGrowableBlobArrayGetPointerToNewEndItem(FskGrowableBlobArray array, UInt32 itemSize, UInt32 *id, void **ptr, void **dirData);


				/* Data access - existing items - from index */


/** Get mutable pointers to the directory entry and blob storage for the specified item, specified by index, in the growable blob array.
 *	\param[in]		array		the array to be modified.
 *	\param[in]		index		the index of the object to be accessed.
 *	\param[out]		ptr			a location to store a pointer to the  blob storage   for the item.
 *	\param[out]		size		a location to store the size  of the  blob storage   for the item.
 *	\param[out]		dirData		a location to store a pointer to the directory entry for the item.
 *	\return			kFskErrNone	if the operation was successful.
 **/
FskAPI(FskErr)	FskGrowableBlobArrayGetPointerToItem(FskGrowableBlobArray array, UInt32 index, void **ptr, UInt32 *size, void **dirData);


/** Get immutable pointers to the directory entry and blob storage for the specified item, specified by index, in the growable blob array.
 *	\param[in]		array		the array to be modified.
 *	\param[in]		index		the index of the object to be accessed.
 *	\param[out]		ptr			a location to store a pointer to the  blob storage   for the item.
 *	\param[out]		size		a location to store the size  of the  blob storage   for the item.
 *	\param[out]		dirData		a location to store a pointer to the directory entry for the item.
 *	\return			kFskErrNone	if the operation was successful.
 **/
FskAPI(FskErr)	FskGrowableBlobArrayGetConstPointerToItem(FskConstGrowableBlobArray array, UInt32 index, const void **ptr, UInt32 *size, const void **dirData);


				/* Data access - existing items - from ID */


/** Get mutable pointers to the directory entry and blob storage for the specified item, specified by ID, in the growable blob array.
 *	\param[in]		array		the array to be modified.
 *	\param[in]		id			the ID of the object to be accessed.
 *	\param[out]		ptr			a location to store a pointer to the  blob storage   for the item.
 *	\param[out]		size		a location to store the size  of the  blob storage   for the item.
 *	\param[out]		dirData		a location to store a pointer to the directory entry for the item.
 *	\return			kFskErrNone	if the operation was successful.
 **/
FskAPI(FskErr)	FskGrowableBlobArrayGetPointerFromItemID(FskGrowableBlobArray array, UInt32 id, void **ptr, UInt32 *size, void **dirData);


/** Get immutable pointers to the directory entry and blob storage for the specified item, specified by ID, in the growable blob array.
 *	\param[in]		array		the array to be modified.
 *	\param[in]		id			the ID of the object to be accessed.
 *	\param[out]		ptr			a location to store a pointer to the  blob storage   for the item.
 *	\param[out]		size		a location to store the size  of the  blob storage   for the item.
 *	\param[out]		dirData		a location to store a pointer to the directory entry for the item.
 *	\return			kFskErrNone	if the operation was successful.
 **/
FskAPI(FskErr)	FskGrowableBlobArrayGetConstPointerFromItemID(FskConstGrowableBlobArray array, UInt32 id, const void **ptr, UInt32 *size, const void **dirData);


				/* Editing operations */


/** Remove an item from the growable blob array.
 *	This messes up all the indices, but IDs are the same.
 *	\param[in,out]	array		the array to be modified.
 *	\param[in]		index		the index of the object to be removed.
 *	\return			kFskErrNone	if the operation was successful.
 **/
FskAPI(void)	FskGrowableBlobArrayRemoveItem(FskGrowableBlobArray array, UInt32 index);


/** Append an item to the end of the growable blob array.
 *	\param[in,out]	array		the array to be modified.
 *	\param[in]		dir			the directory entry to be copied into the new item.
 *	\param[in]		blob		the blob to be copied into the new item.
 *	\param[in]		blobSize	the size of the blob to be copied into the new item.
 *	\param[in,out]	id			the id of the item.
 *								On input, the desired ID, or kFskGrowableBlobArrayUnassignedID or NULL to have the id assigned automatically.
 *								On output, the assigned ID.
 *	\return			kFskErrNone	if the operation was successful.
 **/
FskAPI(FskErr)	FskGrowableBlobArrayAppendItem(FskGrowableBlobArray array, const void *dir, const void *blob, UInt32 blobSize, UInt32 *id);


/** Insert an item at an arbitrary position in the growable blob array.
 *	\param[in,out]	array		the array to be modified.
 *	\param[in]		index		the location where the item is to be created in the array.
 *	\param[in]		dir			the directory entry to be copied into the new item.
 *	\param[in]		blob		the blob to be copied into the new item.
 *	\param[in]		blobSize	the size of the blob to be copied into the new item.
 *	\param[in,out]	id			the id of the item.
 *								On input, the desired ID, or kFskGrowableBlobArrayUnassignedID or NULL to have the id assigned automatically.
 *								On output, the assigned ID.
 *	\return			kFskErrNone	if the operation was successful.
 **/
FskAPI(FskErr)	FskGrowableBlobArrayInsertItemAtPosition(FskGrowableBlobArray array, UInt32 index, const void *dir, const void *blob, UInt32 blobSize, UInt32 *id);


/** Replace an item at an arbitrary position in the growable blob array.
 *	The ID is not modified.
 *	\param[in,out]	array		the array to be modified.
 *	\param[in]		index		the location where the item is to be created in the array.
 *	\param[in]		dir			the directory entry to be copied into the new item.
 *	\param[in]		blob		the blob to be copied into the new item.
 *	\param[in]		blobSize	the size of the blob to be copied into the new item.
 *	\return			kFskErrNone	if the operation was successful.
 **/
FskAPI(FskErr)	FskGrowableBlobArrayReplaceItem(FskGrowableBlobArray array, UInt32 index, const void *dir, const void *blob, UInt32 blobSize);


/** Swap two elements in the growable blob array.
 *	\param[in,out]	array		the array to be modified.
 *	\param[in]		index0		the index of one item.
 *	\param[in]		index1		the index of the other item.
 *	\return			kFskErrNone	if the operation was successful.
 **/
FskAPI(FskErr)	FskGrowableBlobArraySwapItems(FskGrowableBlobArray array, UInt32 index0, UInt32 index1);


/** Edit the data in a blob of an item in the growable blob array.
 *	\param[in,out]	array			the array to be modified.
 *	\param[in]		index			the index of the item to be modified.
 *	\param[in]		offset			the offset to where the data is to be replaced.
 *	\param[in]		delBytes		the number of bytes to be deleted at the specified offset.
 *	\param[in]		repl			the replBytes bytes of data to replace the delBytes bytes removed at offset.
 *	\param[in]		replBytes		the number of bytes to replace the delBytes deleted at offset.
 *	\return			kFskErrNone	if the operation was successful.
 **/
FskAPI(FskErr)	FskGrowableBlobArrayEditItem(FskGrowableBlobArray array, UInt32 index, UInt32 offset, UInt32 delBytes, const void *repl, UInt32 replBytes);


				/* Sort and search */


/** Sort the array by ID, for faster ID search.
 *	\param[in,out]	array			the array to be sorted.
 *	\return			kFskErrNone	if the operation was successful.
 **/
FskAPI(FskErr)	FskGrowableBlobArraySortItemsByID(FskGrowableBlobArray array);


/** Set the blob comparison function. This is used in both FskGrowableBlobArraySortItems() and FskGrowableBlobArrayBSearchItems().
 *	\param[in,out]	array			the array to be sorted.
 *	\param[in]		comp			the comparison function to be used for sorting.
 *	\return			kFskErrNone	if the operation was successful.
 **/
FskAPI(FskErr)	FskGrowableBlobArraySetCompareFunction(FskGrowableBlobArray array, FskGrowableBlobCompare comp);


/** Sort the array by the method previously set with FskGrowableBlobArraySetCompareFunction().
 *	\param[in,out]	array			the array to be sorted.
 *	\return			kFskErrNone					if the sort was completed successfully.
 *	\return			kFskErrNotDirectory			if array==NULL.
 *	\return			kFskErrExtensionNotFound	if the sort function was not previously set with FskGrowableBlobArraySetSortFunction().
 **/
FskAPI(FskErr)	FskGrowableBlobArraySortItems(FskGrowableBlobArray array);


/** Binary search for an item in the array.
 *	\param[in]	array			the array to be searched.
 *	\param[in]	key				pointer to a FskBlobRecord that describes the key.
 *	\param[out]	pIndex			a place to store the index of the matched item.
 *	\return		kFskErrNone					if the item was found.
 *	\return		kFskErrItemNotFound			if the item was not found.
 *	\return		kFskErrNotDirectory			if array==NULL.
 *	\return		kFskErrExtensionNotFound	if the sort function was not previously set with FskGrowableBlobArraySetSortFunction().
 **/
FskAPI(FskErr)	FskGrowableBlobArrayBSearchItems(FskConstGrowableBlobArray array, const FskBlobRecord *key, UInt32 *pIndex);


struct FskBlobQueryResultRecord;								/**< The result of a query. */
typedef struct FskBlobQueryResultRecord *FskBlobQueryResult;	/**< The result of a query. */


/** Query that returns multiple items.
 *	\param[in]	array			the array to be queried.
 *	\param[in]	query			the query procedure that returns 0 if the item is to be appended to the query result.
 *	\param[in]	queryData		a pointer that gets passed to the query procedure.
 *	\param[out]	pResult			a place to store the result of the query.
 *	\note		do not dispose of the query result, because it is an ephemeral part of the blob array.
 *	\note		any modifications to the blob array will corrupt this query.
 *	\return		kFskErrNone			if the query returned at least one match.
 *	\return		kFskErrItemNotFound	if no items were returned by the query. NULL is returned in *pResult in this case.
 **/
FskAPI(FskErr)	FskGrowableBlobArrayQuery(FskConstGrowableBlobArray array, FskGrowableBlobCompare query, const FskBlobRecord *queryData, FskBlobQueryResult *pResult);


/** Refine a previous query.
 *	\param[in]		array			the array to be queried.
 *	\param[in]		query			the query procedure that returns 0 if the item is to be appended to the query result.
 *	\param[in]		queryData		a pointer that gets passed to the query procedure.
 *	\param[in,out]	pResult			on input, a location that holds the previous query.
 *									on output, a location where the refined query is stored.
 *	\note		do not dispose of the query result, because it is an ephemeral part of the blob array.
 *	\note		any modifications to the blob array will corrupt this query.
 *	\return		kFskErrNone			if the query returned at least one match.
 *	\return		kFskErrItemNotFound	if no items were returned by the query. If another refinement is desired, call
 *									FskGrowableBlobArrayQueryUnrefine() then FskGrowableBlobArrayQueryRefine().
 *	\return		kFskErrBadState		if the previous query has been corrupted.
 **/
FskAPI(FskErr)	FskGrowableBlobArrayQueryRefine(FskConstGrowableBlobArray array, FskGrowableBlobCompare query, const FskBlobRecord *queryData, FskBlobQueryResult *pResult);


/** Unrefine a previous query.
 *	\param[in,out]	pResult			on input, a location that holds the previous query.
 *									on output, a location where the query prior tothe previous query is stored.
 *	\note		do not dispose of the query result, because it is an ephemeral part of the blob array.
 *	\note		any modifications to the blob array will corrupt this query.
 *	\return		kFskErrNone			if the query returned at least one match.
 *	\return		kFskErrItemNotFound	if no items were returned by the query.
 *	\return		kFskErrBadState		if the previous query has been corrupted.
 **/
FskAPI(FskErr)	FskGrowableBlobArrayQueryUnrefine(FskBlobQueryResult *pResult);


/** Count the number of matches in the query.
 *	\param[in]	result			the query result to be measured.
 *	\return		the number of matches in the query.
 **/
FskAPI(UInt32)	FskGrowableBlobArrayQueryCount(FskBlobQueryResult result);


/** Access the query matches.
 *	\param[in]		result			the query result to be accessed.
 *	\param[in]		queryIndex		the index of the query match to be retrieved.
 *	\param[out]		blobIndex		a place to store the index of the specified query match.
 *	\return			kFskErrNone			if the specified query result was retrieved successfully.
 *	\return			kFskErrOutOfRange	if the specified index is greater than the number of matches in the query result.
 **/
FskAPI(FskErr)	FskGrowableBlobArrayQueryGet(FskBlobQueryResult result, UInt32 queryIndex, UInt32 *blobIndex);

				/* Tidying up */


/** Minimize and arrange storage.
 *	\param[in]	array		the array to be compacted.
 *	\return		kFskErrNone	if the operation was successful.
 **/
FskAPI(FskErr)	FskGrowableBlobArrayCompact(FskConstGrowableBlobArray array);




/********************************************************************************
 ********************************************************************************
 *****		Equivalence Class Collection - a bag of bags of elements		*****
 *****				A specialization of Growable Blob array.				*****
 ********************************************************************************
 ********************************************************************************/

/* This is a collection of equivalence classes, each of which contains multiple elements.
 * Each class can have an arbitrary number of elements (including the degenerate 0),
 * and there is no requirement that any class have the same number elements as any other class.
 *
 * The equivalence classes are usually considered to be unordered, although their order does not change
 * unless some sorting functions are called. The elements of the classes are always ordered,
 * as there is no sorting function available for elements within a class,
 * and arduous editing methods must be used in order to reorder the elements.
 * Typically, the first element is special, perhaps the "representative element" or the "key" or the "name".
 * Each class has an associated ID, which can be specified upon creation, or assigned automatically.
 * The class ID is persistent for the life of the class, and does not change when resorted.
 * There is no requirement that the class IDs be unique, though it is usually advantageous to be unique;
 * A possible use of non-unique IDs is to give the class as a particular type, e.g. string, integer, bitmap, etc.
 * If more properties need to be associated with each class, use the Growable Blob Array directly.
 *
 * The primary functions are to:
 *	- build the collection of equivalence classes, and
 *	- determine which equivalence class a particular specimen belongs to.
 *
 * We provide convenience macros to access Blob Arrays in a way that is more friendly to an Equivalence Class Collection.
 * Of course, all of the Blob Array functions are available, since FskGrowableEquivalences is a subclass of FskGrowableBlobArray.
 *
 * The elements of the classes can be anything that can be stored flat at a single memory location of a particular size.
 * Be aware that elements are copied into the equivalence class using a shallow copy,
 * so that any references contained within must be persistent for the life of the element within the equivalence class.
 * Frequently, the elements are C strings, but they can be anything that can be bracketed with a pointer and size.
 *
 * We do provide 4 new functions that are frequently used in Equivalence Class Collections:
 *
 *		FskGrowableEquivalencesAppendMultipleElementClass()		- to build an entire equivalence class in one call
 *		FskGrowableEquivalencesAppendElementToClass()			- to add a new element to an existing class
 *		FskGrowableEquivalencesFindIndexOfElement()				- to search for an element anywhere in any of the classes
 *		FskGrowableEquivalencesFindIndexOfElementInPosition()	- to search for an element in a particular position (e.g. key position = 0)
 *
 * To iterate through all elements of all classes:
 *
 *		UInt32 classIndex, numClasses, classID, classSize, elementIndex;
 *		for (classIndex = 0, numClasses = FskGrowableEquivalencesGetClassCount(coll); classIndex < numClasses; ++classIndex) {
 *			const FskEquivalenceBlob *blob;
 *			(void)FskGrowableEquivalencesGetConstPointerToClass(coll, classIndex, (const void**)(&blob), &classSize);
 *			(void)FskGrowableBlobArrayGetIDOfItem(coll, classIndex, &classID);
 *			for (elementIndex = 0; elementIndex < blob->numElements; ++elementIndex) {
 *				const void	*elPtr	= FskEquivalenceElementGetPointer(blob, elementIndex);
 *				UInt32		elSize	= FskEquivalenceElementGetSize(   blob, elementIndex);
 *				(*visitProc)(elPtr, elSize, classID, classSize, classIndex, elementIndex, userData);
 *			}
 *		}
 */

typedef       struct FskGrowableBlobArrayRecord	*FskGrowableEquivalences;							/**< Pointer to a            record that represents a set of growable equivalence classes. */
typedef const struct FskGrowableBlobArrayRecord	*FskConstGrowableEquivalences;						/**< Pointer to an immutable record that represents a set of growable equivalence classes. */
typedef struct FskEquivalenceElementLocation { UInt32 offset, size; } FskEquivalenceElementLocation;						/**< Location and size for an element in the class. */
typedef struct FskEquivalenceBlob { UInt32	numElements; FskEquivalenceElementLocation element[1]; } FskEquivalenceBlob;	/**< Directory for the elements in the class. */
typedef FskGrowableBlobCompare	FskEquivalenceElementCompare;										/**< Equivalences share the same kind of comparison function as blobs. */


/** Determine the size of a blob.
 *	\param[in]	numElements		the number of elements in the class.
 *	\param[in]	elementBytes	the total number of bytes needed so store all of the elements in the class.
 *	\return		the number of bytes needed to store the directory and the element sin th eblob.
 */
#define FskEquivalenceBlobSize(numElements, elementBytes)	(sizeof(FskEquivalenceBlob) + ((numElements)-1) * sizeof(FskEquivalenceElementLocation) + (elementBytes))

/** Get the pointer of an equivalence element.
 *	\param[in]	blobPtr	the pointer to the blob.
 *	\param[in]	index	the index of the element to be accessed from the blob.
 *	\return		the pointer to the element.
 */
#define FskEquivalenceElementGetPointer(blobPtr, index)		(void*)(((FskEquivalenceBlob*)(blobPtr))->element[index].offset + (char*)(blobPtr))

/** Get the size of an equivalence element.
 *	\param[in]	blobPtr	the pointer to the blob.
 *	\param[in]	index	the index of the element to be accessed from the blob.
 *	\return		the size of the element.
 */
#define FskEquivalenceElementGetSize(blobPtr, index)		(((FskEquivalenceBlob*)(blobPtr))->element[index].size)

/** Append a new equivalence class with the specified number of elements, zero or more.
 *	\param[in]		coll		The collection to be modified.
 *	\param[in,out]	id			the id of the class.
 *								On input, the desired ID, or kFskGrowableBlobArrayUnassignedID or NULL to have the id assigned automatically.
 *								On output, the assigned ID.
 *	\param[in]		extraBytes	The amount of extra zero-padding at the end. This can be used for null termination or reserving space for later use.
 *	\param[in]		numElements	The number of elements (zero or greater) to be gathered into this class.
 *	\param[in]		ptr			The element to be appended. Can be repeated in (ptr, size) pairs. Can be NULL if size != 0.
 *	\param[in]		size		The size of the element to be appended. Can be repeated in (ptr, size) pairs.
 *								Can be 0 if ptr != 0 implying a C-string, whose size if determined with size=FskStrLen(ptr)+1.
 *	\note			This procedure can be used for arbitrary types of elements, though strings may be the most common.
 *					It is not necessary to null-terminate each string as C does, because its length is stored separately, as is the case with Pascal and C++ strings.
 *					The most flexible would be to null-terminate each string and null-terminate each class (i.e. doubly-terminate the last string),
 *					so that it can be treated as a StringList; this is afforded with extraBytes=1.
 *	\return			kFskErrNone	if the operation was completed successfully.
 */
FskAPI(FskErr)	 FskGrowableEquivalencesAppendMultipleElementClass(FskGrowableEquivalences coll, UInt32 *id, UInt32 extraBytes, UInt32 numElements, /* const void *ptr, UInt32 size, */ ...);

/** Append a new element to the specified class.
 *	\param[in]		coll		The collection to be appended.
 *	\param[in]		index		The index of the class to be augmented.
 *	\param[in]		ptr			The element to be appended.
 *	\param[in]		size		The size of the element to be appended. 0 implies a C-string, whose size if determined with FskStrLen().
 *	\param[in]		extraBytes	The amount of extra zero-padding at the end. This can be used for null termination or reserving space for later use.
 *	\return			kFskErrNone	if the operation was completed successfully.
 */
FskAPI(FskErr)	FskGrowableEquivalencesAppendElementToClass(FskGrowableEquivalences coll, UInt32 index, const void *ptr, UInt32 size, UInt32 extraBytes);

/** Find the index of the equivalence class that contains the given element.
 *	The classes are iterated in ascending index, and the elements within each class are iterated in ascending order;
 *	if the comparison function matches multiple elements, this procedure will return the first one encountered in its iteration.
 *	\param[in]		coll		The collection to be searched.
 *	\param[in,out]	key			Structure containing the pointer to the key element and its size.
 *								If the size is 0, it implies a C-string, and it will be modified with the results of a call to  FskStrLen().
 *								The other fields (id and dir) will be preserved and communicated to the comparison function call.
 *	\param[in]		compare		A function to be used for comparison; NULL matches size and content.
 *	\param[out]		pIndex		A location to store the index of the matched element.
 *	\return			kFskErrNone	if the operation was completed successfully.
 */
FskAPI(FskErr)	FskGrowableEquivalencesFindClassIndexOfElement(FskConstGrowableEquivalences coll, FskBlobRecord *key, FskEquivalenceElementCompare compare, UInt32 *pIndex);

/** Find the index of the equivalence class that contains the given element in the specified position.
 *	The classes are iterated in ascending index, and the elements within each class are iterated in ascending order;
 *	if the comparison function matches multiple elements, this procedure will return the first one encountered in its iteration.
 *	\param[in]		coll		The collection to be searched.
 *	\param[in,out]	key			Structure containing the pointer to the key element and its size.
 *								If the size is 0, it implies a C-string, and it will be modified with the results of a call to  FskStrLen().
 *								The other fields (id and dir) will be preserved and communicated to the comparison function call.
 *	\param[in]		position	Position of the element in the class.
 *	\param[in]		compare		The comparison function to use for the search.
 *	\param[out]		pIndex		A location to store the index of the matched element.
 *	\return			kFskErrNone			if the operation was completed successfully.
 *	\return			kFskErrItemNotFound	if the element could not be found in the specified position of any class.
 *	\bug			There is no way to fetch multiple matches with this call.
 *					However, see the "To iterate through all elements of all classes" section above.
 */
FskAPI(FskErr)	FskGrowableEquivalencesFindClassIndexOfElementInPosition(FskConstGrowableEquivalences coll, FskBlobRecord *key, UInt32 position, FskEquivalenceElementCompare compare, UInt32 *pIndex);

/** Constructor for a Growable Equivalence Class Collection.
 *	This allocates a data structure with zero classes, but preallocates storage for the expected number.
 *	\param[in]	classBytes	the expected number of bytes to be used per class, for pre-allocation.
 *	\param[out]	maxClasses	the expected number of classes of average size classBytes, for pre-allocation.
 *	\param[out]	pColl		a place to store the new growable equivalence class collection.
 **/
FskErr		FskGrowableEquivalencesNew(UInt32 classBytes, UInt32 maxClasses, FskGrowableBlobArray *pColl);
#define		FskGrowableEquivalencesNew(classBytes, maxClasses, pColl)	FskGrowableBlobArrayNew(classBytes, maxClasses, 0, pColl)

/** Destructor for a Growable Equivalence Class Collection.
 *	\param[in]	coll		the equivalence class collection to be disposed.
 **/
void		FskGrowableEquivalencesDispose(FskGrowableEquivalences coll);
#define		FskGrowableEquivalencesDispose(coll)	FskGrowableBlobArrayDispose(coll)

/** Get the number of equivalence classes in the collection.
 *	\param[in]	coll		the dictionary to be queried.
 *	\return		the number of items in the dictionary.
 **/
UInt32		FskGrowableEquivalencesGetClassCount(FskConstGrowableEquivalences coll);
#define		FskGrowableEquivalencesGetClassCount(coll)	FskGrowableBlobArrayGetItemCount(coll)

/** Set the number of classes in the collection.
 *	\param[in]	coll		the collection to be queried.
 *	\param[in]	numClasses	the desired number of classes in the collection.
 *	\return		kFskErrNone	if the operation was successful.
 **/
FskErr		FskGrowableEquivalencesSetClassCount(FskGrowableEquivalences coll, UInt32 numClasses);
#define		FskGrowableEquivalencesSetClassCount(coll, numClasses)	FskGrowableBlobArraySetItemCount(coll, numClasses)

/** Remove an item from the growable collection.
 *	\param[in,out]	coll		the array to be modified.
 *	\param[in]		index		the index of the item to be removed.
 **/
void		FskGrowableEquivalencesRemoveClass(FskGrowableEquivalences coll, UInt32 index);
#define		FskGrowableEquivalencesRemoveClass(coll, index)	FskGrowableBlobArrayRemoveItem(coll, index)

/** Minimize and arrange storage.
 *	\param[in]	coll		the collection to be compacted.
 *	\return		kFskErrNone	if the operation was successful.
 **/
FskErr		FskGrowableEquivalencesCompact(FskConstGrowableEquivalences coll);
#define		FskGrowableEquivalencesCompact(coll)	FskGrowableBlobArrayCompact(coll)

/** Append a new class with the specified amount of storage at the end of the growable storage, and return pointers thereto.
 *	\param[in,out]	coll		The collection to be modified.
 *	\param[in]		classBytes	the size of the storage to be allocated for the new class.
 *	\param[in,out]	id			the id of the class.
 *								On input, the desired ID, or kFskGrowableBlobArrayUnassignedID or NULL to have the id assigned automatically.
 *								On output, the assigned ID.
 *	\param[out]		ptr			a location to store a pointer to the storage for the class.
 *	\return			kFskErrNone	if the operation was successful.
 **/
FskErr		FskGrowableEquivalencesGetPointerToNewEndClass(FskGrowableEquivalences coll, UInt32 classBytes, UInt32 *id, void **ptr);
#define		FskGrowableEquivalencesGetPointerToNewEndClass(coll, classBytes, id, ptr)	FskGrowableBlobArrayGetPointerToNewEndItem(coll, classBytes, id, ptr, NULL)

/**	Get a mutable pointer to the class storage in a growable collection.
 *	\param[in]		coll		The collection to be accessed.
 *	\param[in]		index		the index of the desired item in the collection.
 *	\param[out]		ptr			A place to store a pointer to the class storage.
 *	\param[out]		size		A place to store the total number of bytes allocated for the class.
 *	\return			kFskErrNone	if the operation was successful.
 **/
FskErr		FskGrowableEquivalencesGetPointerToClass(FskGrowableEquivalences coll, UInt32 index, void **ptr, UInt32 *size);
#define		FskGrowableEquivalencesGetPointerToClass(coll, index, ptr, size)	FskGrowableBlobArrayGetPointerToItem(coll, index, ptr, size, NULL)

/**	Get immutable pointers to the strings of an item in a growable collection.
 *	\param[in]		coll		The collection to be accessed.
 *	\param[in]		index		the index of the desired item in the collection.
 *	\param[out]		ptr			a location to store a pointer to the storage for the class.
 *	\param[out]		size		A place to store the total number of bytes allocated for the class.
 *	\return			kFskErrNone	if the operation was successful.
 **/
FskErr		FskGrowableEquivalencesGetConstPointerToClass(FskConstGrowableEquivalences coll, UInt32 index, const void **ptr, UInt32 *size);
#define		FskGrowableEquivalencesGetConstPointerToClass(coll, index, ptr, size)	FskGrowableBlobArrayGetConstPointerToItem(coll, index, ptr, size, NULL)

/** Find the ID of the equivalence class that contains the given element.
 *	The classes are iterated in ascending index, and the elements within each class are iterated in ascending order;
 *	if the comparison function matches multiple elements, this procedure will return the first one encountered in its iteration.
 *	\param[in]		coll		The collection to be searched.
 *	\param[in,out]	key			Structure containing the pointer to the key element and its size.
 *								If the size is 0, it implies a C-string, and it will be modified with the results of a call to  FskStrLen().
 *								The other fields (id and dir) will be preserved and communicated to the comparison function call.
 *	\param[in]		compare		A function to be used for comparison; NULL matches size and content. The key is supplied as the first blob.
 *	\param[out]		pID			A location to store the ID of the matched element.
 *	\return			kFskErrNone	if the operation was completed successfully.
 *	\bug			There is no way to fetch multiple matches with this call.
 *					However, see the "To iterate through all elements of all classes" section above.
 */
FskErr	FskGrowableEquivalencesFindClassIDOfElement(FskConstGrowableEquivalences coll, FskBlobRecord *key, FskEquivalenceElementCompare compare, UInt32 *pID);
#define FskGrowableEquivalencesFindClassIDOfElement(coll, key, compare, pID)	((kFskErrNone == (err = FskGrowableEquivalencesFindClassIndexOfElement(coll, key, compare, pID))) \
																					? FskGrowableBlobArrayGetIDOfItem(coll, *pID, pID) : err)


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __FSKGROWABLESTORAGE__ */

