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
#include "FskEndian.h"


/********************************************************************************
 * FskEndianStruct_Swap
 * Swap elements in a heterogeneous data structure according to the structure specification string.
 *	'1' - byte, char, unsigned char, SInt8, UInt8
 *	'2' - short, unsigned short, UInt16, SInt16
 *	'4' - int, long, unsigned long, unsigned int, float, UInt32, SInt32, Float32
 *	'8' - long long, unsigned long long, double _int64, SInt64, UInt64, Float64
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

void
FskEndianStruct_Swap(void *structPtr, const char *spec)
{
	UInt8	*ptr	= (UInt8*)structPtr;
	UInt8	buf[36], *p, *q;
	int		n, i;
	
	while ((n = *spec++) != 0) {
		if (		('0' <= n) && (n <= '9'))	n -= '0';		/* 0 - 9 */
		else if (	('a' <= n) && (n <= 'z'))	n -= 'a' - 0xA;	/* 10 - 36 */
		else if (	('A' <= n) && (n <= 'Z'))	n -= 'A' - 0xA;	/* 10 - 36 */
		else									n  = 0;
		for (i = n, q = ptr, p = buf; i--; )	*p++ = *q++;	/* Copy forward   to  buffer */
		for (i = n; i--; )						*ptr++ = *--p;	/* Copy backward from buffer */
	}
}


/********************************************************************************
 * FskEndianVector16_Swap
 * Swap a vector of SInt16, UInt16, etc.
 ********************************************************************************/

void
FskEndianVector16_Swap(register UInt16 *vec, register UInt32 length)
{
	register UInt16 pix;

	for ( ; length--; vec++) {
		pix = *vec;
		pix = FskEndian16_Swap(pix);
		*vec = pix;
	}
}


/********************************************************************************
 * FskEndianVector32_Swap
 * Swap a vector of UInt32, SInt32, Float32, void*
 ********************************************************************************/

void
FskEndianVector32_Swap(register UInt32 *vec, register UInt32 length)
{
	register UInt32 pix;

	for ( ; length--; vec++) {
		pix = *vec;
		pix = FskEndian32_Swap(pix);
		*vec = pix;
	}
}


/********************************************************************************
 * FskEndianVector64_Swap
 * Swap a vector of UInt64, SInt64, FskInt64, Float64
 ********************************************************************************/

void
FskEndianVector64_Swap(register FskInt64 *vec, register UInt32 length)
{
	FskInt64 pix;
	
	for ( ; length--; vec++) {
		pix = *vec;
		pix = FskEndian64_Swap(pix);
		*vec = pix;
	}
}
