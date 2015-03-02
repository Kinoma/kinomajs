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
#ifndef __FSK_ENDIAN__
#define __FSK_ENDIAN__

#include "Fsk.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */



#if TARGET_RT_BIG_ENDIAN
	#define FskEndianU64_BtoN(a) (a)
	#define FskEndianU32_BtoN(a) (a)
	#define FskEndianU16_BtoN(a) (a)
	#define FskEndianS32_BtoN(a) (a)
	#define FskEndianS16_BtoN(a) (a)

	#define FskEndianU64_NtoB(a) (a)
	#define FskEndianU32_NtoB(a) (a)
	#define FskEndianU16_NtoB(a) (a)
	#define FskEndianS32_NtoB(a) (a)
	#define FskEndianS16_NtoB(a) (a)
#else /* TARGET_RT_LITTLE_ENDIIAN */
	#define FskEndianU64_LtoN(a) (a)
	#define FskEndianU32_LtoN(a) (a)
	#define FskEndianU16_LtoN(a) (a)
	#define FskEndianS32_LtoN(a) (a)
	#define FskEndianS16_LtoN(a) (a)

	#define FskEndianU64_NtoL(a) (a)
	#define FskEndianU32_NtoL(a) (a)
	#define FskEndianU16_NtoL(a) (a)
	#define FskEndianS32_NtoL(a) (a)
	#define FskEndianS16_NtoL(a) (a)
#endif /* TARGET_RT_LITTLE_ENDIIAN */

#if TARGET_RT_LITTLE_ENDIAN
	#define FskEndianU64_BtoN(a) ((FskInt64)FskEndian64_Swap(a))
	#define FskEndianU32_BtoN(a) ((UInt32)FskEndian32_Swap(a))
	#define FskEndianU16_BtoN(a) ((UInt16)FskEndian16_Swap(a))
	#define FskEndianS32_BtoN(a) ((SInt32)FskEndian32_Swap(a))
	#define FskEndianS16_BtoN(a) ((SInt16)FskEndian16_Swap(a))

	#define FskEndianU64_NtoB(a) ((FskInt64)FskEndian64_Swap(a))
	#define FskEndianU32_NtoB(a) ((UInt32)FskEndian32_Swap(a))
	#define FskEndianU16_NtoB(a) ((UInt16)FskEndian16_Swap(a))
	#define FskEndianS32_NtoB(a) ((SInt32)FskEndian32_Swap(a))
	#define FskEndianS16_NtoB(a) ((SInt16)FskEndian16_Swap(a))
#else /* TARGET_RT_BIG_ENDIAN */
	#define FskEndianU64_LtoN(a) ((FskInt64)FskEndian64_Swap(a))
	#define FskEndianU32_LtoN(a) ((UInt32)FskEndian32_Swap(a))
	#define FskEndianU16_LtoN(a) ((UInt16)FskEndian16_Swap(a))
	#define FskEndianS32_LtoN(a) ((SInt32)FskEndian32_Swap(a))
	#define FskEndianS16_LtoN(a) ((SInt16)FskEndian16_Swap(a))

	#define FskEndianU64_NtoL(a) ((FskInt64)FskEndian64_Swap(a))
	#define FskEndianU32_NtoL(a) ((UInt32)FskEndian32_Swap(a))
	#define FskEndianU16_NtoL(a) ((UInt16)FskEndian16_Swap(a))
	#define FskEndianS32_NtoL(a) ((SInt32)FskEndian32_Swap(a))
	#define FskEndianS16_NtoL(a) ((SInt16)FskEndian16_Swap(a))
#endif /* TARGET_RT_BIG_ENDIAN */

#define FskEndian16_Swap(a)			\
        ((((UInt8)a) << 8)   	|	\
        (((UInt16)a) >> 8))

#define FskEndian32_Swap(a)                  \
        (((((UInt32)a)<<24) & 0xFF000000)  | \
        ((((UInt32)a)<< 8) & 0x00FF0000)  | \
        ((((UInt32)a)>> 8) & 0x0000FF00)  | \
        ((((UInt32)a)>>24) & 0x000000FF))

#define FskEndian64_Swap(a)											\
        (((((FskInt64)a)<<56) & (((FskInt64)0xFF) << (7 << 3)))  |	\
         ((((FskInt64)a)<<40) & (((FskInt64)0xFF) << (6 << 3)))  |	\
         ((((FskInt64)a)<<24) & (((FskInt64)0xFF) << (5 << 3)))  |	\
         ((((FskInt64)a)<< 8) & (((FskInt64)0xFF) << (4 << 3)))  |	\
         ((((FskInt64)a)>> 8) & (((FskInt64)0xFF) << (3 << 3)))  |	\
         ((((FskInt64)a)>>24) & (((FskInt64)0xFF) << (2 << 3)))  |	\
         ((((FskInt64)a)>>40) & (((FskInt64)0xFF) << (1 << 3)))  |	\
         ((((FskInt64)a)>>56) & (((FskInt64)0xFF) << (0 << 3)))  )


#if !FSK_MISALIGNED_MEMORY_ACCESS_IS_IMPLEMENTED_IN_HARDWARE
	#if TARGET_RT_LITTLE_ENDIAN
    	#define FskMisaligned64_GetN(a) 		\
			(((FskInt64)((UInt8*)(a))[7] << 56) |	\
			((FskInt64)((UInt8*)(a))[6] << 48) |	\
			((FskInt64)((UInt8*)(a))[5] << 40) |	\
			((FskInt64)((UInt8*)(a))[4] << 32) |	\
			((FskInt64)((UInt8*)(a))[3] << 24) |	\
			((FskInt64)((UInt8*)(a))[2] << 16) |	\
			((FskInt64)((UInt8*)(a))[1] << 8) |	\
			((FskInt64)((UInt8*)(a))[0] << 0))
    	#define FskMisaligned32_GetN(a) 		\
			(((UInt32)((UInt8*)(a))[3] << 24) |	\
			((UInt32)((UInt8*)(a))[2] << 16) |	\
			((UInt32)((UInt8*)(a))[1] << 8) |	\
			((UInt32)((UInt8*)(a))[0] << 0))
    	#define FskMisaligned16_GetN(a) 		\
			(((UInt16)((UInt8*)(a))[1] << 8) |	\
			((UInt16)((UInt8*)(a))[0] << 0))
	#else /* TARGET_RT_BIG_ENDIAN */
		#define FskMisaligned64_GetN(a)			\
			(((FskInt64)((UInt8*)(a))[0] << 56) |	\
			((FskInt64)((UInt8*)(a))[1] << 48) |	\
			((FskInt64)((UInt8*)(a))[2] << 40) |	\
			((FskInt64)((UInt8*)(a))[3] << 32) |	\
			((FskInt64)((UInt8*)(a))[4] << 24) |	\
			((FskInt64)((UInt8*)(a))[5] << 16) |	\
			((FskInt64)((UInt8*)(a))[6] << 8) |	\
			((FskInt64)((UInt8*)(a))[7] << 0))
		#define FskMisaligned32_GetN(a)			\
			(((UInt32)((UInt8*)(a))[0] << 24) |	\
			((UInt32)((UInt8*)(a))[1] << 16) |	\
			((UInt32)((UInt8*)(a))[2] << 8) |	\
			((UInt32)((UInt8*)(a))[3] << 0))
		#define FskMisaligned16_GetN(a)			\
			(((UInt16)((UInt8*)(a))[0] << 8) |	\
			((UInt16)((UInt8*)(a))[1] << 0))
	#endif /* TARGET_RT_BIG_ENDIAN */
		#define FskMisaligned64_PutN(a,b) 		\
			(((char*)(b))[0]=((char*)(a))[0],	\
			((char*)(b))[1]=((char*)(a))[1],	\
			((char*)(b))[2]=((char*)(a))[2],	\
			((char*)(b))[3]=((char*)(a))[3],	\
			((char*)(b))[4]=((char*)(a))[4],	\
			((char*)(b))[5]=((char*)(a))[5],	\
			((char*)(b))[6]=((char*)(a))[6],	\
			((char*)(b))[7]=((char*)(a))[7])
		#define FskMisaligned32_PutN(a,b) 		\
			(((char*)(b))[0]=((char*)(a))[0],	\
			((char*)(b))[1]=((char*)(a))[1],	\
			((char*)(b))[2]=((char*)(a))[2],	\
			((char*)(b))[3]=((char*)(a))[3])
		#define FskMisaligned16_PutN(a,b)		\
			(((char*)(b))[0] = ((char*)(a))[0],	\
			((char*)(b))[1] = ((char*)(a))[1])
	#if TARGET_RT_LITTLE_ENDIAN
    	#define FskMisaligned64_GetBtoN(a) 		\
			(((FskInt64)((UInt8*)(a))[0] << 56) |	\
			((FskInt64)((UInt8*)(a))[1] << 48) |	\
			((FskInt64)((UInt8*)(a))[2] << 40) |	\
			((FskInt64)((UInt8*)(a))[3] << 32) |	\
			((FskInt64)((UInt8*)(a))[4] << 24) |	\
			((FskInt64)((UInt8*)(a))[5] << 16) | 	\
			((FskInt64)((UInt8*)(a))[6] << 8) | 	\
			((FskInt64)((UInt8*)(a))[7] << 0))
    	#define FskMisaligned32_GetBtoN(a) 		\
			(((UInt32)((UInt8*)(a))[0] << 24) |	\
			((UInt32)((UInt8*)(a))[1] << 16) | 	\
			((UInt32)((UInt8*)(a))[2] << 8) | 	\
			((UInt32)((UInt8*)(a))[3] << 0))
    	#define FskMisaligned16_GetBtoN(a) 		\
			(((UInt16)((UInt8*)(a))[0] << 8) | 	\
			((UInt16)((UInt8*)(a))[1] << 0))
	#else /* TARGET_RT_BIG_ENDIAN */
    	#define FskMisaligned64_GetBtoN(a) 	(FskMisaligned64_GetN(a))
    	#define FskMisaligned32_GetBtoN(a) 	(FskMisaligned32_GetN(a))
    	#define FskMisaligned16_GetBtoN(a) 	(FskMisaligned16_GetN(a))
	#endif /* TARGET_RT_BIG_ENDIAN */
	#define FskMisaligned32_GetLtoN(a) 		\
		(((UInt32)((UInt8*)(a))[0] << 0) |	\
		((UInt32)((UInt8*)(a))[1] << 8) | 	\
		((UInt32)((UInt8*)(a))[2] << 16) | 	\
		((UInt32)((UInt8*)(a))[3] << 24))
	#define FskMisaligned16_GetLtoN(a) 		\
		(((UInt16)((UInt8*)(a))[0] << 0) | 	\
		((UInt16)((UInt8*)(a))[1] << 8))
#else /* FSK_MISALIGNED_MEMORY_ACCESS_IS_IMPLEMENTED_IN_HARDWARE */
	#define FskMisaligned64_GetBtoN(a)	(FskEndianU64_BtoN(*(FskInt64*)(a)))
	#define FskMisaligned64_GetN(a) 	(*(FskInt64*)(a))
	#define FskMisaligned64_PutN(a,b)	(*(FskInt64*)(b) = *(FskInt64*)(a))
	#define FskMisaligned32_GetBtoN(a)	(FskEndianU32_BtoN(*(UInt32*)(a)))
	#define FskMisaligned32_GetN(a) 	(*(UInt32*)(a))
	#define FskMisaligned32_PutN(a,b)	(*(UInt32*)(b) = *(UInt32*)(a))
	#define FskMisaligned16_PutN(a,b)	(*(UInt16*)(b) = *(UInt16*)(a))
	#define FskMisaligned16_GetN(a) 	(*(UInt16*)(a))
    #define FskMisaligned16_GetBtoN(a) 	(FskEndianU16_BtoN(*(UInt16*)(a)))
	#if TARGET_RT_LITTLE_ENDIAN
		#define FskMisaligned16_GetLtoN(a) (*(UInt16*)(a))
		#define FskMisaligned32_GetLtoN(a) (*(UInt32*)(a))
	#else
		#define FskMisaligned32_GetLtoN(a) 		\
			(((UInt32)((UInt8*)(a))[0] << 0) |	\
			((UInt32)((UInt8*)(a))[1] << 8) | 	\
			((UInt32)((UInt8*)(a))[2] << 16) | 	\
			((UInt32)((UInt8*)(a))[3] << 24))
		#define FskMisaligned16_GetLtoN(a) 		\
			(((UInt16)((UInt8*)(a))[0] << 0) | 	\
			((UInt16)((UInt8*)(a))[1] << 8))
	#endif
#endif /* FSK_MISALIGNED_MEMORY_ACCESS_IS_IMPLEMENTED_IN_HARDWARE */



/********************************************************************************
 * FskEndianStruct_Swap
 * Swap elements in a heterogeneous data structure according to the structure specification string.
 *	'1' - byte, char, unsigned char, SInt8, UInt8
 *	'2' - short, unsigned short, UInt16, SInt16
 *	'4' - int, long, unsigned long, unsigned int, float, UInt32, SInt32, Float32
 *	'8' - long long, unsigned long long, double, _int64, SInt64, UInt64, Float64
 *	'g' or 'G' - SInt128, UInt128, Float128
 *	etc. - for data types up to 36 bytes long
 *
 * Example:
 *	struct Demo {
 *		void		*baseAddr;
 *		SInt32		rowBytes;
 * 		UInt16		width;
 * 		UInt16		height;
 *		UInt32		pixelFormat;
 *		double		scale;
 *		UInt8		depth;
 *		UInt8		hasAlpha;
 *	} demo;
 *	FskEndianStruct_Swap(&demo, "44224811");
 *
 * BEWARE: This will not work as expected if the structure has been padded by the compiler!
 ********************************************************************************/

FskAPI(void)	FskEndianStruct_Swap(void *structPtr, const char *spec);

#if TARGET_RT_BIG_ENDIAN
	#define FskEndianStruct_BtoN(p, s)	(void)(p)	/* identity */
	#define FskEndianStruct_NtoB(p, s)	(void)(p)	/* identity */
	#define FskEndianStruct_LtoN(p, s)	FskEndianStruct_Swap(p, s)
	#define FskEndianStruct_NtoL(p, s)	FskEndianStruct_Swap(p, s)
#else /* TARGET_RT_LITTLE_ENDIIAN */
	#define FskEndianStruct_LtoN(p, s)	(void)(p)	/* identity */
	#define FskEndianStruct_NtoL(p, s)	(void)(p)	/* identity */
	#define FskEndianStruct_BtoN(p, s)	FskEndianStruct_Swap(p, s)
	#define FskEndianStruct_NtoB(p, s)	FskEndianStruct_Swap(p, s)
#endif /* TARGET_RT_LITTLE_ENDIIAN */


/********************************************************************************
 * FskEndianVectorXX_Swap
 * swap a homogeneous vector of numbers
 ********************************************************************************/

FskAPI(void)	FskEndianVector16_Swap(  UInt16 *vec, UInt32 length);	/* Swap double-byte vector */
FskAPI(void)	FskEndianVector32_Swap(  UInt32 *vec, UInt32 length);	/* Swap   quad-byte vector */
FskAPI(void)	FskEndianVector64_Swap(FskInt64 *vec, UInt32 length);	/* Swap   octo-byte vector */

#if TARGET_RT_BIG_ENDIAN
	#define FskEndianVector16_BtoN(v, n)	(void)(v)	/* identity */
	#define FskEndianVector16_NtoB(v, n)	(void)(v)	/* identity */
	#define FskEndianVector32_BtoN(v, n)	(void)(v)	/* identity */
	#define FskEndianVector32_NtoB(v, n)	(void)(v)	/* identity */
	#define FskEndianVector64_BtoN(v, n)	(void)(v)	/* identity */
	#define FskEndianVector64_NtoB(v, n)	(void)(v)	/* identity */
	#define FskEndianVector16_LtoN(v, n)	FskEndianVector16_Swap(v, n)
	#define FskEndianVector16_NtoL(v, n)	FskEndianVector16_Swap(v, n)
	#define FskEndianVector32_LtoN(v, n)	FskEndianVector32_Swap(v, n)
	#define FskEndianVector32_NtoL(v, n)	FskEndianVector32_Swap(v, n)
	#define FskEndianVector64_LtoN(v, n)	FskEndianVector64_Swap(v, n)
	#define FskEndianVector64_NtoL(v, n)	FskEndianVector64_Swap(v, n)
#else /* TARGET_RT_LITTLE_ENDIIAN */
	#define FskEndianVector16_LtoN(v, n)	(void)(v)	/* identity */
	#define FskEndianVector16_NtoL(v, n)	(void)(v)	/* identity */
	#define FskEndianVector32_LtoN(v, n)	(void)(v)	/* identity */
	#define FskEndianVector32_NtoL(v, n)	(void)(v)	/* identity */
	#define FskEndianVector64_LtoN(v, n)	(void)(v)	/* identity */
	#define FskEndianVector64_NtoL(v, n)	(void)(v)	/* identity */
	#define FskEndianVector16_BtoN(v, n)	FskEndianVector16_Swap(v, n)
	#define FskEndianVector16_NtoB(v, n)	FskEndianVector16_Swap(v, n)
	#define FskEndianVector32_BtoN(v, n)	FskEndianVector32_Swap(v, n)
	#define FskEndianVector32_NtoB(v, n)	FskEndianVector32_Swap(v, n)
	#define FskEndianVector64_BtoN(v, n)	FskEndianVector64_Swap(v, n)
	#define FskEndianVector64_NtoB(v, n)	FskEndianVector64_Swap(v, n)
#endif /* TARGET_RT_LITTLE_ENDIIAN */

/*

    Read/Write with endian conversion, alignment safe, type punning warning avoidance
*/

#define FskUInt32Read_BtoN(ptr)                     \
        ((UInt32)((*(UInt8 *)(ptr)       << 24)  |  \
                  (*(UInt8 *)((ptr) + 1) << 16)  |  \
                  (*(UInt8 *)((ptr) + 2) <<  8)  |  \
                  (*(UInt8 *)((ptr) + 3))))
    
#define FskUInt16Read_BtoN(ptr)                     \
        ((UInt16)((*(UInt8 *)(ptr)       <<  8)  |  \
                  (*(UInt8 *)((ptr) + 1))))
    
#define FskUInt8Read(ptr)                          \
            ((UInt8)*(ptr))

#define FskUInt32Read_LtoN(ptr)                    \
        ((UInt32)((*(UInt8 *)(ptr)            ) |  \
                  (*(UInt8 *)((ptr) + 1) <<  8) |  \
                  (*(UInt8 *)((ptr) + 2) << 16) |  \
                  (*(UInt8 *)((ptr) + 3) << 24)))
    
#define FskUInt16Read_LtoN(ptr)                    \
        ((UInt16)((*(UInt8 *)(ptr)              |  \
                  (*(UInt8 *)((ptr) + 1) << 8))))

#define FskUInt32Write_NtoB(ptr, value)    \
    *(ptr)       = (UInt8)((value) >> 24); \
    *((ptr) + 1) = (UInt8)((value) >> 16); \
    *((ptr) + 2) = (UInt8)((value) >> 8);  \
    *((ptr) + 3) = (UInt8)((value))

#define FskUInt16Write_NtoB(ptr, value)    \
    *(ptr)       = (UInt8)((value) >> 8);  \
    *((ptr) + 1) = (UInt8)(value)

#define FskUInt8Write(ptr, value)          \
    *(ptr)       = (UInt8)(value)

#define FskUInt32Write_NtoL(ptr, value)    \
    *(ptr)     = (UInt8)(value);           \
    *((ptr) + 1) = (UInt8)((value) >> 8);  \
    *((ptr) + 2) = (UInt8)((value) >> 16); \
    *((ptr) + 3) = (UInt8)((value) >> 24)

#define FskUInt16Write_NtoL(ptr, value)    \
    *(ptr)       = (UInt8)(value);         \
    *((ptr) + 1) = (UInt8)((value) >> 8)
    

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __FSK_ENDIAN__ */
