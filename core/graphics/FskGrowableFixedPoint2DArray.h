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
#ifndef __FSKGROWABLEFIXEDPOINT2DARRAY__
#define __FSKGROWABLEFIXEDPOINT2DARRAY__

#ifndef __FSKGROWABLESTORAGE__
# include "FskGrowableStorage.h"
#endif /* __FSKGROWABLESTORAGE__ */

#ifndef __FSKFIXEDMATH__
# include "FskFixedMath.h"
#endif /* __FSKFIXEDMATH__ */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */



////////////////////////////////////////////////////////////////////////////////
////						Growable FixedPoint2D Array						////
////////////////////////////////////////////////////////////////////////////////


/* This trivial subclass of FskGrowableArray is mostly for convenience and self-documentation. :-) */
typedef struct	FskGrowableArrayRecord *FskGrowableFixedPoint2DArray;
#define			FskGrowableFixedPoint2DArrayNew(maxPoints, array)							FskGrowableArrayNew(sizeof(FskFixedPoint2D), maxPoints, array)
#define			FskGrowableFixedPoint2DArrayDispose(array)									FskGrowableArrayDispose(array)
#define			FskGrowableFixedPoint2DArrayAppendItem(array, item)						FskGrowableArrayAppendItem(array, item)
#define			FskGrowableFixedPoint2DArrayAppendItems(array, item, numItems)			FskGrowableArrayAppendItems(array, item, numItems)
#define			FskGrowableFixedPoint2DArrayAppendReversedItems(array, item, numItems)	FskGrowableArrayAppendReversedItems(array, item, numItems)
#define			FskGrowableFixedPoint2DArrayGetPointerToItem(array, index, ptr)			FskGrowableArrayGetPointerToItem(array, index, (void**)ptr)
#define			FskGrowableFixedPoint2DArrayGetConstPointerToItem(array, index, ptr)		FskGrowableArrayGetConstPointerToItem(array, index, (const void**)ptr)
#define			FskGrowableFixedPoint2DArrayGetItem(array, index, item)					FskGrowableArrayGetItem(array, index, item)
#define			FskGrowableFixedPoint2DArrayGetItemCount(array)							FskGrowableArrayGetItemCount(array)
#define			FskGrowableFixedPoint2DArraySetItemCount(array, numItems)					FskGrowableArraySetItemCount(array, numItems)


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __FSKGROWABLEFIXEDPOINT2DARRAY__ */

