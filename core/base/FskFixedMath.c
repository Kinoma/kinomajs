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
#include "FskFixedMath.h"
#include <limits.h>
#include <math.h>


#if SUPPORT_INSTRUMENTATION
	#ifndef NULL
		#define NULL 0
	#endif /* NULL */
	FskInstrumentedSimpleType(FixedMath, fixedmath);
	#define LOGD(...)  do { FskInstrumentedTypePrintfDebug  (&gFixedMathTypeInstrumentation, __VA_ARGS__); } while(0)
	#define LOGI(...)  do { FskInstrumentedTypePrintfVerbose(&gFixedMathTypeInstrumentation, __VA_ARGS__); } while(0)
	#define LOGE(...)  do { FskInstrumentedTypePrintfMinimal(&gFixedMathTypeInstrumentation, __VA_ARGS__); } while(0)
#endif /* SUPPORT_INSTRUMENTATION */
#ifndef LOGD
	#define	LOGD(...)
#endif /* LOGD */
#ifndef LOGI
	#define	LOGI(...)
#endif /* LOGI */
#ifndef LOGE
	#define	LOGE(...)
#endif /* LOGE */


#define FIXED_ONE				0x00010000	/* 16 fractional bits */
#define FRACT_ONE				0x40000000	/* 30 fractional bits */


#ifndef PRAGMA_MARK_SUPPORTED
	#if defined(macintosh)
		#define PRAGMA_MARK_SUPPORTED 1
	#else /*  !PRAGMA_MARK_SUPPORTED */
		#define PRAGMA_MARK_SUPPORTED 0
	#endif /*  !PRAGMA_MARK_SUPPORTED */
#endif /*  PRAGMA_MARK_SUPPORTED */


#if _WIN32
	typedef unsigned __int64	FskUInt64;
#else /* !_WIN32 */
	typedef unsigned long long	FskUInt64;
#endif /* !_WIN32 */


#ifndef INT32_MAX
	#define INT32_MAX (2147483647)
	#define INT32_MIN (-2147483647-1)
#endif /* INT32_MAX */

/********************************************************************************
 ********************************************************************************
 *****								CPU Tuning								*****
 ********************************************************************************
 ********************************************************************************/

#if TARGET_OS_KPL
	#include "FskPlatformImplementation.Kpl.h"
#elif   TARGET_CPU_X86 && defined(linux)
    #define USE_ASSEMBLER_MULTIPLICATION    0
    #define USE_ASSEMBLER_DIVISION          0
    #define USE_ASSEMBLER_RATIO             0
    #define USE_MULTIPLICATION_SATURATION   0
    #define USE_DIVISION_SATURATION         0
    #define USE_SHIFT_SATURATE              0
#elif   TARGET_CPU_X86
	#if defined(__APPLE_CC__) && defined(__llvm__) && __llvm__	/* New LLVM compiler cannot grok our code */
		#define USE_ASSEMBLER_MULTIPLICATION	0
		#define USE_ASSEMBLER_DIVISION		0
		#define USE_ASSEMBLER_RATIO			0
	#else /* Not new Apple compiler */
		#define USE_ASSEMBLER_MULTIPLICATION	1
		#define USE_ASSEMBLER_DIVISION		1
		#define USE_ASSEMBLER_RATIO			1
	#endif /* Not new Apple compiler */
	#define USE_MULTIPLICATION_SATURATION	1
	#define USE_DIVISION_SATURATION			1
	#define USE_SHIFT_SATURATE				0
#elif TARGET_CPU_PPC
	#define USE_ASSEMBLER_MULTIPLICATION	1
	#define USE_ASSEMBLER_DIVISION			1
	#define USE_ASSEMBLER_RATIO				1
	#define USE_MULTIPLICATION_SATURATION	1
	#define USE_DIVISION_SATURATION			1
	#define USE_SHIFT_SATURATE				0
#elif TARGET_CPU_ARM && ANDROID_PLATFORM
    /* For now the compiler has issue.   to be fixed later??? */
	#define USE_ASSEMBLER_MULTIPLICATION	0
	#define USE_ASSEMBLER_DIVISION			0
	#define USE_ASSEMBLER_RATIO				0
	#define USE_MULTIPLICATION_SATURATION	1
	#define USE_DIVISION_SATURATION			1
	#define USE_SHIFT_SATURATE				1
#elif TARGET_CPU_ARM && !TARGET_OS_IPHONE
	#define USE_ASSEMBLER_MULTIPLICATION	1
	#define USE_ASSEMBLER_DIVISION			1
	#define USE_ASSEMBLER_RATIO				1
	#define USE_MULTIPLICATION_SATURATION	1
	#define USE_DIVISION_SATURATION			1
	#define USE_SHIFT_SATURATE				1
#elif TARGET_CPU_ARM && (TARGET_OS_IPHONE)
	#define USE_ASSEMBLER_MULTIPLICATION	0
	#define USE_ASSEMBLER_DIVISION			0
	#define USE_ASSEMBLER_RATIO				0
	#define USE_MULTIPLICATION_SATURATION	1
	#define USE_DIVISION_SATURATION			1
	#define USE_SHIFT_SATURATE				0
#elif TARGET_CPU_MIPS
	// Don't know about these...
	#define USE_ASSEMBLER_MULTIPLICATION	0
	#define USE_ASSEMBLER_DIVISION			0
	#define USE_ASSEMBLER_RATIO				0
	#define USE_MULTIPLICATION_SATURATION	1
	#define USE_DIVISION_SATURATION			1
	#define USE_SHIFT_SATURATE				0
#else /* TARGET_CPU_UNKNOWN */
	#error What is your CPU??
#endif /* TARGET_CPU */

#if TARGET_OS_WIN32
	// turning these on causes a link/code-gen failure on Win32 full release builds
	#undef USE_ASSEMBLER_DIVISION
	#define USE_ASSEMBLER_DIVISION 0
	#undef USE_ASSEMBLER_RATIO
	#define USE_ASSEMBLER_RATIO 0
#endif


#define SATURATE_S32(sign32)			((((SInt32)(sign32)) >> 31) ^ INT32_MAX)
#define SHIFT_SATURATE(x64, x32)		do { if ((SInt32)((x64) >> 32) != (SInt32)((x64) >> 31)) (x32) = SATURATE_S32((SInt32)((x64) >> 32)); } while (0)
#define COMPARE_SATURATE(x64, x32)		do { if	(x64 < (FskInt64)INT32_MIN)	x32 = INT32_MIN;			\
										else if	(x64 > (FskInt64)INT32_MAX) x32 = INT32_MAX; } while (0)

#if !USE_MULTIPLICATION_SATURATION
	#define MULT_SATURATE(x64, x32)	(void)x32
#elif !USE_SHIFT_SATURATE
	#define MULT_SATURATE(x64, x32)	COMPARE_SATURATE(x64, x32)
#else
	#define MULT_SATURATE(x64, x32)	SHIFT_SATURATE(x64, x32)
#endif

#if !USE_DIVISION_SATURATION
	#define DIV_SATURATE(x64, x32)	(void)x32
#elif !USE_SHIFT_SATURATE
	#define DIV_SATURATE(x64, x32)	COMPARE_SATURATE(x64, x32)
#else
	#define DIV_SATURATE(x64, x32)	SHIFT_SATURATE(x64, x32)
#endif


/********************************************************************************
 ********************************************************************************
 *****							Utility functions							*****
 ********************************************************************************
 ********************************************************************************/



/********************************************************************************
 * FskRoundAndSaturateFloatToUnityFract
 ********************************************************************************/

FskFract
FskRoundAndSaturateFloatToUnityFract(double d)
{
	FskFract	f;

	if      (d <= 0)	f = 0;
	else if (d >= 1)	f = FRACT_ONE;
	else				f = FskRoundPositiveFloatToFract(d);	/* Nan's will come here and do a default convert to int */

	return f;
}



/********************************************************************************
 * FskRoundAndSaturateFloatToNFixed
 ********************************************************************************/

FskFixed
FskRoundAndSaturateFloatToNFixed(double d, UInt32 fracBits)
{
	FskFixed	f;

	d *= (1 << fracBits);
	if (d < 0)	f = ((d -= 0.5) <= INT32_MIN) ? INT32_MIN : (FskFixed)d;
	else		f = ((d += 0.5) >= INT32_MAX) ? INT32_MAX : (FskFixed)d;	/* Nan's will come here and do a default convert to int */

	return f;
}


/********************************************************************************
 * FskLeadingZeros64
 ********************************************************************************/

SInt32
FskLeadingZeros64(FskInt64 x)
{
	int n;
#if TARGET_CPU_X86 && defined(_MSC_VER)
	SInt32 lo = ((SInt32*)(&x))[0];
	SInt32 hi = ((SInt32*)(&x))[1];
	_asm {
		mov		eax, -1
		bsr		eax, lo
		sub		eax, 32
		bsr		eax, hi
		sub		eax, 31
		neg		eax
		mov		n, eax;
	}
#elif TARGET_CPU_PPC
	register SInt32 hi = ((SInt32*)(&x))[0];
	register SInt32 lo = ((SInt32*)(&x))[1];
	asm {
		cntlzw	hi, hi
		cntlzw	lo, lo
	}
	n = (hi < 32) ? hi : hi + lo;
#elif 1
	UInt32 hi, lo;
	n = 0;
	hi = (UInt32)(x >> 32);
	lo = (UInt32)(x);
	if ( hi               == 0) n += 32;	else lo = hi;
	if ((hi = (lo >> 16)) == 0)	n += 16;	else lo = hi;
	if ((hi = (lo >>  8)) == 0)	n +=  8;	else lo = hi;
	if ((hi = (lo >>  4)) == 0)	n +=  4;	else lo = hi;
	if ((hi = (lo >>  2)) == 0)	n +=  2;	else lo = hi;
	if ((hi = (lo >>  1)) == 0)	n +=  1;
#else
	for (n = 64; n--; x <<= 1)
		if (x < 0)
			break;
	n = 63 - n;
#endif
	return n;
}


#if 0
/********************************************************************************
 * FskSaturateLongLongToLong
 ********************************************************************************/

static SInt32
FskSaturateLongLongToLong(FskInt64 ll)
{
	SInt32 l = (SInt32)ll;
	SHIFT_SATURATE(ll, l);
	return l;
}
#endif


#if PRAGMA_MARK_SUPPORTED
#pragma mark -
#endif /*  PRAGMA_MARK_SUPPORTED */
/********************************************************************************
 ********************************************************************************
 *****						Fixed point multiplication						*****
 ********************************************************************************
 ********************************************************************************/
#if !USE_ASSEMBLER_MULTIPLICATION



/********************************************************************************
 * FskFixedNMul
 *		with saturation instead of overflow
 ********************************************************************************/

SInt32
FskFixedNMul(SInt32 x, SInt32 y, SInt32 n)
{
	FskInt64 p;

	p = (FskInt64)x * y;
	if (n > 31) {		/* 32-bit processors cannot shift more than 31 bits at a time */
		p >>= 31;
		n -= 31;
	}
	if (n > 0)
		p += ((FskInt64)1) << (n - 1);		/* Round */
	p >>= n;
	x = (SInt32)p;
	MULT_SATURATE(p, x);
	#if SUPPORT_INSTRUMENTATION
		if (x != p) { LOGD("FskFixedNMul saturating %lld to %d", p, (int)x); }
	#endif /* SUPPORT_INSTRUMENTATION */
	return x;
}


/********************************************************************************
 * FskFixMul
 *		with saturation instead of overflow
 ********************************************************************************/

FskFixed
FskFixMul(FskFixed x, FskFixed y)
{
	FskInt64 p;

	p = (((FskInt64)x * y) + (1 << (16 - 1))) >> 16;
	x = (FskFixed)p;
	MULT_SATURATE(p, x);
	#if SUPPORT_INSTRUMENTATION
		if (x != p) { LOGD("FskFixMul saturating %lld to %d", p, (int)x); }
	#endif /* SUPPORT_INSTRUMENTATION */
	return x;
}


/********************************************************************************
 * FskFracMul
 *		with saturation instead of overflow
 ********************************************************************************/

FskFract
FskFracMul(FskFract x, FskFract y)
{
	FskInt64 p;

	p = (((FskInt64)x * y) + (1 << (30 - 1))) >> 30;
	x = (FskFract)p;
	MULT_SATURATE(p, x);
	#if SUPPORT_INSTRUMENTATION
		if (x != p) { LOGD("FskFracMul saturating %lld to %d", p, (int)x); }
	#endif /* SUPPORT_INSTRUMENTATION */
	return x;
}

#endif /* USE_ASSEMBLER_MULTIPLICATION */


/********************************************************************************
 * FskMultiply32by32giving64
 ********************************************************************************/

FskInt64
FskMultiply32by32giving64(SInt32 a, SInt32 b)
{
	return (FskInt64)a * b;
}



#if PRAGMA_MARK_SUPPORTED
#pragma mark -
#endif /*  PRAGMA_MARK_SUPPORTED */
/********************************************************************************
 ********************************************************************************
 *****							Fixed point division						*****
 ********************************************************************************
 ********************************************************************************/


/********************************************************************************
 * FskDivide64by32giving32
 *		with saturation instead of overflow
 ********************************************************************************/

SInt32
FskDivide64by32giving32(FskInt64 n, SInt32 d)
{
	if (d != 0) {
		if ((((SInt32)(n >> 32)) ^ d) < 0)	n -= d >> 1;		/* Round */
		else								n += d >> 1;
		n /= d;													/* Divide */
		d  = (SInt32)(n);
		DIV_SATURATE(n, d);
		#if SUPPORT_INSTRUMENTATION
			if (d != n) { LOGD("FskDivide64by32giving32 saturating %lld to %d", n, (int)d); }
		#endif /* SUPPORT_INSTRUMENTATION */
	}
	else {
		if      (n < 0)	d = INT32_MIN;
		else if (n > 0)	d = INT32_MAX;
		else			d = 0;
		#if SUPPORT_INSTRUMENTATION
			LOGD("FskDivide64by32giving32(%lld, 0) saturating %d", n, (int)d);
		#endif /* SUPPORT_INSTRUMENTATION */
	}

	return d;
}


#if !USE_ASSEMBLER_DIVISION


/********************************************************************************
 * FskFixedNDiv
 *		with saturation instead of overflow
 ********************************************************************************/

FskFixed
FskFixedNDiv(FskFixed x, FskFixed y, SInt32 n)
{
	FskInt64 z;

	if (x == 0)											/* If numerator is zero ... */
		return 0;										/* ... quotient is zero */
	if (y == 0)											/* If denominator is zero ... */
		Lsaturate: return SATURATE_S32(x ^ y);			/* ... result is +/- "infinity" */

	z = x;

	/* Saturate rather than tossing most significant bits */
	if (n > 31) {
		if ((((x < 0) ? -x : x) >> (63 - n)) != 0)
			goto Lsaturate;
		z <<= 31;
		n -= 31;
	}

	z <<= n;
	return FskDivide64by32giving32(z, y);
}


/********************************************************************************
 * FskFixDiv
 *		with saturation instead of overflow
 ********************************************************************************/

FskFixed
FskFixDiv(FskFixed x, FskFixed y)
{
	return FskDivide64by32giving32(((FskInt64)x) << 16, y);
}


/********************************************************************************
 * FskFracDiv
 *		with saturation instead of overflow
 ********************************************************************************/

FskFract
FskFracDiv(FskFract x, FskFract y)
{
	return FskDivide64by32giving32(((FskInt64)x) << 30, y);
}


#endif /* USE_ASSEMBLER_DIVISION */


/********************************************************************************
 * FskFixNDiv64
 ********************************************************************************/

FskFract
FskFixNDiv64(FskInt64 num, FskInt64 den, SInt32 n)
{
	if (den != 0) {
		int nZeros, upShift, downShift;
		if (n > 0) {
			nZeros = FskLeadingZeros64((num < 0) ? -num : num);		/* 1 <= nZeros <= 64 */
			if ((upShift = nZeros - 2) > n)							/* 0 <= upShift <= n */
				upShift = n;			/* We can upshift the numerator the fully specified amount */
			downShift = n - upShift;	/* The remainder we downshift the denominator */

			/* Upshift numerator */
			if (upShift > 0) {
				if (upShift > 31) {		/* 32 processors cannot shift more than 31 bits at a time */
					num <<= 31;
					upShift -= 31;
				}
				num <<= upShift;
			}
			else if (upShift < 0) {		/* We need more headroom */
				upShift = -upShift;
				if (upShift > 31) {
					num >>= 31;
					upShift -= 31;
				}
				num >>= (upShift - 1);	/* Downshift ... */
				nZeros = (int)num & 1;	/* ... with rounding ... */
				num >>= 1;				/* ... being careful ... */
				num += nZeros;			/* ... not to overflow */
			}

			/* Downshift denominator */
			if (downShift > 0) {
				if (downShift > 31) {	/* 32 processors cannot shift more than 31 bits at a time */
					den >>= 31;
					downShift -= 31;
				}
				if (--downShift > 0)
					den >>= downShift;	/* Shift almost all of the way down */
				++den;					/* Round */
				den >>= 1;				/* And finish shifting */
				if (den == 0)
					goto zeroDenominator;
			}
		}
		else if (n < 0) {
			n = -n;
			nZeros = FskLeadingZeros64((den < 0) ? -den : den);		/* 1 <= nZeros <= 64 */
			if ((upShift = nZeros - 2) > n)
				upShift = n;			/* We can upshift the denominator the fully specified amount */
			downShift = n - upShift;	/* The remainder we downshift the numerator */

			/* Upshift denominator */
			if (upShift > 0) {
				if (upShift > 31) {		/* 32 processors cannot shift more than 31 bits at a time */
					den <<= 31;
					upShift -= 31;
				}
				den <<= upShift;
			}
			else if (upShift < 0) {
				upShift = -upShift;
				if (upShift > 31) {
					den >>= 31;
					upShift -= 31;
				}
				den >>= (upShift - 1);	/* Downshift ... */
				nZeros = (int)den & 1;	/* ... with rounding ... */
				den >>= 1;				/* ... being careful ... */
				den += nZeros;			/* ... not to overflow */
			}

			/* Downshift numerator */
			if (downShift > 0) {
				if (downShift > 31) {	/* 32 processors cannot shift more than 31 bits at a time */
					num >>= 31;
					downShift -= 31;
				}
				if (--downShift > 0)
					num >>= downShift;	/* Shift almost all of the way down */
				++num;					/* Round */
				num >>= 1;				/* And finish shifting */
			}
		}

		if (((SInt32)(num >> 32) ^ (SInt32)(den >> 32)) < 0)	num -= den >> 1;	/* Pre-bias to round */
		else													num += den >> 1;
		num /= den;
		n = (SInt32)num;
		DIV_SATURATE(num, n);
	}
	else {
	zeroDenominator:
		if      (num < 0)	n = INT32_MIN;
		else if (num > 0)	n = INT32_MAX;
		else				n = 0;
	}

	return n;
}



#if PRAGMA_MARK_SUPPORTED
#pragma mark -
#endif /*  PRAGMA_MARK_SUPPORTED */
/********************************************************************************
 ********************************************************************************
 *****							Fixed point ratio							*****
 ********************************************************************************
 ********************************************************************************/


/********************************************************************************
 * FskFixedNRatio
 * This maxes out properly if there would otherwise be overflow.
 ********************************************************************************/

#if !USE_ASSEMBLER_RATIO

static SInt32 ShiftMax(SInt32 x) {
	SInt32 n = 0;
	x = x ^ (x >> 1);
	if (x == 0)
		n = 31;
	else
		while ((x <<= 1) > 0)
			++n;
	return n;
}

FskFixed
FskFixedNRatio(FskFixed x, FskFixed num, FskFixed den, SInt32 n)
{
	FskInt64 p;
	SInt32 maxShift;

	if (x == 0 || num == 0)
		return 0;
	if (den == 0)
		return SATURATE_S32(x ^ num);

	p = (FskInt64)x * num;
	if (n < 0) {
		maxShift = ShiftMax(den);
		n = -n;
		if (n <= maxShift) {
			den <<= n;
		}
		else {
			den <<= maxShift;
			n -= maxShift;
			p >>= n;
		}
	}
	else {
		maxShift = ShiftMax((SInt32)(p >> 32));
		if (n <= maxShift) {
			p <<= n;
		}
		else {
			p <<= maxShift;
			n -= maxShift;
			den >>= n;
		}
	}
	return FskDivide64by32giving32(p, den);
}


#endif /* USE_ASSEMBLER_RATIO */


/********************************************************************************
 * FskFixedRatio
 ********************************************************************************/

FskFixed
FskFixedRatio(FskFixed x, FskFixed num, FskFixed den)
{
	return FskFixedNRatio(x, num, den, 0);
}


#if PRAGMA_MARK_SUPPORTED
#pragma mark ----- x86 -----
#endif /*  PRAGMA_MARK_SUPPORTED */
#if TARGET_CPU_X86
/********************************************************************************
 ********************************************************************************
 *****									X86									*****
 ********************************************************************************
 ********************************************************************************/

#if USE_ASSEMBLER_MULTIPLICATION

FskFixed FskFixedNMul(FskFixed aop, FskFixed bop, SInt32 nsh)
{
	_asm {
		mov		eax, aop			/* Get x into accumulator */
		imul	bop					/* Multiply */

		/* Round */
		mov		ecx, nsh			/* Get shift amount */
		test	ecx, ecx
		je		LshiftDone
			cmp		ecx, 32
			jle	Lsm					/* Shift is greater or equal to 32 */
				mov	eax, edx		/* First downshift 32, because x86 cannot handle it */
				sar	edx, 31			/* Extend sign */
				sub	ecx, 32			/* Reduce the shift, nsh, by 32 */
			Lsm:					/* The shift, nsh, is now less than 32 */
			sub		ecx, 1			/* nsh - 1 */
			shrd	eax, edx, cl	/* Double-word shift, leaving one extra bit for rounding */
			sar		edx, cl
			add		eax, 1			/* Round */
			adc		edx, 0			/* Propagate carry from round */
			shrd	eax, edx, 1		/* Double-word shift to format output */
			sar		edx, 1
		LshiftDone:

		/* Overflow check */
		mov		ecx, eax			/* Get the LS word of the product */
		sar		ecx, 31				/* The sign of the LS word */
		cmp		ecx, edx			/* Is the MS word the same as the sign of the LS word? */
		je		Lok					/* If different, saturate */
			sar	edx, 31				/* Get the sign of the MS word */
			xor	edx, 0x7FFFFFFF		/* Compute the largest magnitude number with the same sign ... */
			mov	eax, edx			/* ... and use that as the result */
		Lok:

		mov	aop, eax
	}

	/* return result in eax */
	return aop;
}

FskFixed FskFixMul(FskFixed aop, FskFixed bop)
{
	_asm {
		mov		eax, aop			/* Get x into accumulator */
		imul	bop					/* Multiply */

		/* Round */
		add		eax, 0x8000			/* Round */
		adc		edx, 0				/* Propagate carry from round */

		shrd	eax, edx, 16		/* Double-word shift to format output */

		/* Overflow check */
		sar		edx, 16				/* Shift the MS word of the product into position */
		mov		ecx, eax			/* Get the LS word of the product */
		sar		ecx, 31				/* The sign of the LS word */
		cmp		ecx, edx			/* Is the MS word the same as the sign of the LS word? */
		je		Lok					/* If different, saturate */
			sar	edx, 31				/* Get the sign of the MS word */
			xor	edx, 0x7FFFFFFF		/* Compute the largest magnitude number with the same sign ... */
			mov	eax, edx			/* ... and use that as the result */
		Lok:

		mov	aop, eax
	}

	/* return result in eax */
	return aop;
}

FskFract FskFracMul(FskFract aop, FskFract bop)
{
	_asm {
		mov		eax, aop			/* Get x into accumulator */
		imul	bop					/* Multiply */

		/* Round */
		add	eax, 0x20000000			/* Round */
		adc	edx, 0					/* Propagate carry from round */

		shrd	eax, edx, 30		/* Double-word shift to format output */

		/* Overflow check */
		sar		edx, 30				/* Shift the MS word of the product into position */
		mov		ecx, eax			/* Get the LS word of the product */
		sar		ecx, 31				/* The sign of the LS word */
		cmp		ecx, edx			/* Is the MS word the same as the sign of the LS word? */
		je		Lok					/* If different, saturate */
			sar	edx, 31				/* Get the sign of the MS word */
			xor	edx, 0x7FFFFFFF		/* Compute the largest magnitude number with the same sign ... */
			mov	eax, edx			/* ... and use that as the result */
		Lok:
		mov	aop, eax
	}

	/* return result in eax */
	return aop;
}
#endif /* USE_ASSEMBLER_MULTIPLICATION */


#if USE_ASSEMBLER_DIVISION || USE_ASSEMBLER_RATIO

static UInt32 X86DivU64U32(FskInt64 num, FskFixed den)	/* Forward declaration to avoid inlining */
#ifdef __GNUC__
__attribute__((noinline))	/* This fails when inlining because "num" is never explicitly referenced. */
#endif /* __GNUC__ */
;

static UInt32 X86DivU64U32(FskInt64 num, FskFixed den)
{
	_asm {
		#if TARGET_OS_MAC
			mov		eax, (&den)[-4]			/* Compute normalization shift for numerator */
		#elif TARGET_OS_WIN32
			mov		eax, den[-4]			/* Compute normalization shift for numerator */
		#else
			#error Unknown OS
		#endif /* TARGET_OS */
		bsr		eax, eax					/* Most significant 1 of the numerator */
		bsr		ecx, den					/* Most significant 1 of the denominator */
		cmp		eax, ecx
		cmovg	ecx, eax					/* Take the largest */
		sub		ecx, 31
		neg		ecx							/* This is the number of bits needed to left justify */

		mov		eax, den					/* Normalize denominator, needed for overflow check */
		shl		eax, cl
		mov		den, eax

		#if TARGET_OS_MAC
			mov		eax, (&den)[-8]			/* Normalize numerator, needed for overflow check */
			mov		edx, (&den)[-4]
		#elif TARGET_OS_WIN32
			mov		eax, den[-8]			/* Normalize numerator, needed for overflow check */
			mov		edx, den[-4]
		#else
			#error Unknown OS
		#endif /* TARGET_OS */
		shld	edx, eax, cl
		shl		eax, cl

		mov		ecx, den					/* Check for overflow */
		cmp		edx, ecx
		jb		LnoOverflow
			mov		eax, -1
			jmp		Lreturn
		LnoOverflow:

		div		ecx							/* Unsigned divide */

		shr		ecx, 1						/* Round by first halving the divisor ...  */
		cmp		edx, ecx					/* ... then comparing it against the remainder. */
		jb		Lreturn						/* If the remainder is greater or equal to half the divisor ... */
		add		eax, 1						/* ... then round the quotient up */

	Lreturn:
		mov		den, eax
	}
	return den;
}


static UInt32 X86NDivU64U32(FskInt64 num, FskFixed den, SInt32 nsh)	/* Forward declaration to avoid inlining */
#ifdef __GNUC__
__attribute__((noinline))	/* This fails when inlining because "num" is never explicitly referenced. */
#endif /* __GNUC__ */
;

static UInt32 X86NDivU64U32(FskInt64 num, FskFixed den, SInt32 nsh)
{
	_asm {
		push	ebx						/* Save registers */
		push	esi

		#if TARGET_OS_MAC
			mov		eax, (&den)[-8]		/* Get LS numerator */
			mov		edx, (&den)[-4]		/* Get MS numerator */
		#elif TARGET_OS_WIN32
			mov		eax, den[-8]		/* Get LS numerator */
			mov		edx, den[-4]		/* Get MS numerator */
		#else
			#error Unknown OS
		#endif /* TARGET_OS */
		mov		ebx, den				/* Get denominator */
		mov		ecx, nsh				/* Get shift */

		/* Shift parameters appropriately to maintain accuracy.
		 * Given a positive shift, we upshift the denominator,
		 * Given a negative shift, we upshift the numerator.
		 * This will maintain full accuracy, unless there are not enough bits to shift into,
		 * in which case, we downshift the other.
		 */
		test	ecx, ecx				/* Is it greater or less than zero? */
		je		LnoShift				/* Jump if equal to zero */
		jns		LpositiveShift			/* Jump if greater than zero */

			/* Negative shift */
			neg		ecx					/* Change negative upshift to positive downshift */
			bsr		esi, ebx			/* Compute max upshift for denominator */
			sub		esi, 31
			neg		esi
			cmp		ecx, esi			/* Is shift greater than max? */
			jg		LDenUpNumDown
				shl		ebx, cl			/* Shift less than max: upshift denominator */
				jmp		LnoShift
			LDenUpNumDown:				/* Shift greater than max */
				sub		ecx, esi		/* Excess shift */
				shrd	eax, edx, cl	/* Downshift numerator by the excess */
				shr		edx, cl
				mov		ecx, esi
				shl		ebx, cl			/* Upshift denominator to the max */
				jmp		LnoShift

		LpositiveShift:

			/* Positive shift */
			test	edx, edx
			jne		Lsmall				/* At least 32 bits of headroom */
				cmp		ecx, 32
				jl		Lsmall			/* Shift is at least 32 bits */
					sub		ecx, 31
					shld	edx, eax, 31
					shl		eax, 31
			Lsmall:
			mov		esi, edx
			bsr		esi, esi
			sub		esi, 31
			neg		esi
			cmp		ecx, esi			/* Is shift greater than max? */
			jg		LNumUpDenDown
				shld	edx, eax, cl	/* Shift less than the max: upshift numerator */
				shl		eax, cl
				jmp		LnoShift
			LNumUpDenDown:				/* Shift is greater than the max */
				sub		ecx, esi		/* Compute excess shift */
				shr		ebx, cl			/* Downshift denominator by excess */
				mov		ecx, esi
				shld	edx, eax, cl	/* Upshift numerator to the max */
				shl		eax, cl
		LnoShift:

		/* All shifted: now call the division routine */
		mov		ecx, ebx				/* Move denominator to ecx */

		pop		esi						/* Restore saved registers */
		pop		ebx

		push	ecx						/* X86DivU64U32(eax, edx, ecx) */
		push	edx
		push	eax
		call	X86DivU64U32
		add		esp, 12

		mov		den, eax
	}

	return den;
}

#endif /* USE_ASSEMBLER_DIVISION || USE_ASSEMBLER_RATIO */


#if USE_ASSEMBLER_DIVISION
FskFixed FskFixedNDiv(FskFixed aop, FskFixed bop, SInt32 nsh)
{
	FskFixed sign;

	if (aop == 0)							/* If the numerator is 0 ... */
		return 0;							/* ... return 0 */
	if (bop == 0)							/* If the denominator is 0 ... */
		return SATURATE_S32(aop);			/* ... return +/- "infinity" */

	sign = aop ^ bop;						/* Compute the sign of the result */
	if (aop < 0)							/* Take absolute values of numerator and denominator */
		aop = -aop;
	if (bop < 0)
		bop = -bop;

	aop = X86NDivU64U32((UInt32)aop, bop, nsh);	/* Unsigned divide with shift */

	if (aop < 0)							/* Check for overflow ... */
		aop = SATURATE_S32(sign);			/* ... and saturate */
	else if (sign < 0)
		aop = -aop;							/* Negate if the sign is negative */

	return aop;
}

FskFixed FskFixDiv(FskFixed aop, FskFixed bop)
{
	FskFixed sign;
	FskInt64 num;

	if (aop == 0)							/* If the numerator is 0 ... */
		return 0;							/* ... return 0 */
	if (bop == 0)							/* If the denominator is 0 ... */
		return SATURATE_S32(aop);			/* ... return +/- "infinity" */

	sign = aop ^ bop;						/* Compute the sign of the result */
	if (aop < 0)	aop = -aop;				/* Take absolute values of numerator and denominator */
	if (bop < 0)	bop = -bop;


	num = (UInt32)aop;						/* Shift numerator */
	num <<= 16;

	aop = X86DivU64U32(num, bop);			/* Unsigned divide */

	if (aop < 0)							/* Check for overflow ... */
		aop = SATURATE_S32(sign);			/* ... and saturate */
	else if (sign < 0)
		aop = -aop;							/* Negate if the sign is negative */

	return aop;
}

FskFract FskFracDiv(FskFract aop, FskFract bop)
{
	FskFixed sign;
	FskInt64 num;

	if (aop == 0)							/* If the numerator is 0 ... */
		return 0;							/* ... return 0 */
	if (bop == 0)							/* If the denominator is 0 ... */
		return SATURATE_S32(aop);			/* ... return +/- "infinity" */

	sign = aop ^ bop;						/* Compute the sign of the result */
	if (aop < 0)	aop = -aop;				/* Take absolute values of numerator and denominator */
	if (bop < 0)	bop = -bop;

	num = (UInt32)aop;						/* Shift numerator */
	num <<= 30;

	aop = X86DivU64U32(num, bop);			/* Unsigned divide */

	if (aop < 0)							/* Check for overflow ... */
		aop = SATURATE_S32(sign);			/* ... and saturate */
	else if (sign < 0)
		aop = -aop;							/* Negate if the sign is negative */

	return aop;
}
#endif /* USE_ASSEMBLER_DIVISION */


#if USE_ASSEMBLER_RATIO
FskFixed FskFixedNRatio(FskFixed aop, FskFixed bop, FskFixed dop, SInt32 nsh)
{
	int sign;

	if (aop == 0 || bop == 0)
		return 0;
	sign = aop ^ bop ^ dop;
	if (dop == 0)
		return SATURATE_S32(sign);

	if (aop < 0)	aop = -aop;
	if (bop < 0)	bop = -bop;
	if (dop < 0)	dop = -dop;

	_asm {
		mov		eax, aop
		mul		bop						/* Numerator in edx:eax */

		push 	nsh
		push	dop
		push	edx
		push	eax
		call	X86NDivU64U32
		add		esp, 16

		mov		aop, eax
	}

	if (aop < 0)
		aop = SATURATE_S32(sign);
	else if (sign < 0)
		aop = -aop;
	return aop;
}
#endif /* USE_ASSEMBLER_RATIO */


#endif /* TARGET_CPU_X86 */



#if PRAGMA_MARK_SUPPORTED
#pragma mark ----- PPC -----
#endif /*  PRAGMA_MARK_SUPPORTED */
#if TARGET_CPU_PPC
/********************************************************************************
 ********************************************************************************
 *****									PPC									*****
 ********************************************************************************
 ********************************************************************************/

#if USE_ASSEMBLER_MULTIPLICATION
FskFixed FskFixedNMul(register FskFixed a, register FskFixed b, register SInt32 n)
{
	register SInt32 t;
	asm {
		mullw	t,a,b				/* low 32 bits */
		mulhw	a,a,b				/* high 32 bits */
	}
	if ((b = 32 - n) >= 0) {
		if (n > 0) {
			register SInt32 rnd = 1 << (n - 1);
			asm {
				addc	t,t,rnd			/* round by adding (1 << (n - 1)) ... */
				addze	a,a				/* ... and propagating the carry */
			}
			if ((a >> n) != (a >> (n - 1)))
				return SATURATE_S32(a);
		}
		else {
			if (a != (t >> 31))
				return SATURATE_S32(a);
		}

		a = (FskFixed)(((UInt32)a << b) | ((UInt32)t >> n));	/* Less than 32 bit shift */
	}
	else {
		b = -b;
		a += (1 << (b - 1));		/* round */
		a >>= b;												/* More than 31 bit shift */
	}

	return a;
}

FskFixed FskFixMul(register FskFixed a, register FskFixed b)
{
	register SInt32 t;
	asm {
		mullw	t,a,b			/* low 32 bits */
		mulhw	a,a,b			/* high 32 bits */

		li		b,-32768		/* load -0x8000 for rounding */
		subc	t,t,b			/* round by adding 0x8000 ... */
		addze	a,a				/* ... and propagating the carry */
	}

	if ((a >> 31) != (a >> (16 - 1))) {
		a = ~(a >> 31);			/* 0xFFFFFFFF positive 0x00000000 negative */
		a ^= 0x80000000;		/* 0x7FFFFFFF positive 0x80000000 negative */
	}
	else {
		a = (FskFixed)(((UInt32)a << 16) | (((UInt32)t) >> 16));
	}

	return a;
}

FskFract FskFracMul(register FskFract a, register FskFract b)
{
	register UInt32 t;
	asm {
		mullw	t,a,b			/* low 32 bits */
		mulhw	a,a,b			/* high 32 bits */

		lis		b,8192			/* load 0x20000000 for rounding */
		addc	t,t,b			/* round by adding 0x20000000 ... */
		addze	a,a				/* ... and propagating the carry */

	}

	if ((a >> 31) != (a >> (30 - 1))) {
		a = ~(a >> 31);			/* 0xFFFFFFFF positive 0x00000000 negative */
		a ^= 0x80000000;		/* 0x7FFFFFFF positive 0x80000000 negative */
	}
	else {
		a = (FskFixed)(((UInt32)a << 2) | (((UInt32)t) >> 30));
	}

	return a;
}
#endif /* USE_ASSEMBLER_MULTIPLICATION */


#if USE_ASSEMBLER_DIVISION

FskFixed FskFixedNDiv(FskFixed a, FskFixed b, SInt32 n)
{
	if (b != 0) {
		double q = a;
		q /= b;
		q = ldexp(q, n);
		if (q < 0)	a = ((q -= 0.5) < INT32_MIN) ? INT32_MIN : (FskFixed)q;
		else		a = ((q += 0.5) > INT32_MAX) ? INT32_MAX : (FskFixed)q;
	}
	else if (a != 0) {
		a = SATURATE_S32(a);
	}
	return a;
}

FskFixed  FskFixDiv(FskFixed a, FskFixed b) {
	if (b != 0) {
		double q = a;
		q /= b;
		q *= 65536.0;
		if (q < 0)	a = ((q -= 0.5) < INT32_MIN) ? INT32_MIN : (FskFixed)q;
		else		a = ((q += 0.5) > INT32_MAX) ? INT32_MAX : (FskFixed)q;
	}
	else if (a != 0) {
		a = SATURATE_S32(a);
	}
	return a;
}

FskFract FskFracDiv(FskFract a, FskFract b) {
	if (b != 0) {
		double q = a;
		q /= b;
		q *= 1073741824.0;
		if (q < 0)	a = ((q -= 0.5) < INT32_MIN) ? INT32_MIN : (FskFract)q;
		else		a = ((q += 0.5) > INT32_MAX) ? INT32_MAX : (FskFract)q;
	}
	else if (a != 0) {
		a = SATURATE_S32(a);
	}
	return a;
}

#endif /* USE_ASSEMBLER_DIVISION */

#if USE_ASSEMBLER_RATIO
FskFixed FskFixedNRatio(FskFixed a, FskFixed b, FskFixed d, SInt32 n)
{
	if (d != 0) {
		double r = a;
		r *= b;
		r /= d;
		r = ldexp(r, n);

		if (r < 0)	a = ((r -= 0.5) < INT32_MIN) ? INT32_MIN : (FskFixed)r;
		else		a = ((r += 0.5) > INT32_MAX) ? INT32_MAX : (FskFixed)r;
	}
	else if (a != 0) {
		a = (b == 0) ? 0 : SATURATE_S32(a ^ b);
	}
	return a;
}
#endif /* USE_ASSEMBLER_RATIO */

#endif /* TARGET_CPU_PPC */



#if PRAGMA_MARK_SUPPORTED
#pragma mark -
#endif /*  PRAGMA_MARK_SUPPORTED */
/********************************************************************************
 ********************************************************************************
 *****						Fixed point square root							*****
 ********************************************************************************
 ********************************************************************************/


/********************************************************************************
 * FskFixedNSqrt
 ********************************************************************************/

FskFixed
FskFixedNSqrt(FskFixed a, SInt32 n)
{
	register UInt32		remHi, remLo, testDiv, count;
	register FskFixed	root;

	root = 0;										/* Clear root */

	remHi = 0;										/* Clear high part of partial remainder */
	remLo = a;										/* Get argument into low part of partial remainder */
	if (a < 0)	remLo = (UInt32)(-((SInt32)remLo));	/* If negative, compute imaginary root */
	count = 15+n/2;									/* Load loop counter */

	do {
		remHi = (remHi << 2) | (remLo >> 30); remLo <<= 2;	/* get 2 bits of arg */
		root <<= 1;									/* Get ready for the next bit in the root */
		testDiv = (root << 1) + 1;					/* Test divisor */
		if (remHi >= testDiv) {
			remHi -= testDiv;
			root += 1;
		}
	} while (count-- != 0);

	if (remHi > (UInt32)root) root++;				/* If the next bit would have been a 1, round up */
	if (a < 0)	root = -root;						/* Set imaginary bit of the root if the argument was negative */

	return root;
}


/********************************************************************************
 * FskFixSqrt
 ********************************************************************************/

FskFixed
FskFixSqrt(FskFixed a)
{
	return FskFixedNSqrt(a, 16);
}


/********************************************************************************
 * FskFracSqrt
 ********************************************************************************/

FskFract
FskFracSqrt(FskFract a)
{
	return FskFixedNSqrt(a, 30);
}


/********************************************************************************
 * FskFixedSqrt64to32
 ********************************************************************************/

FskFixed
FskFixedSqrt64to32(FskInt64 x)
{
	register UInt32		remHi, remLo, testDiv, count;
	register FskFixed	root;
	UInt32				imag	= 0;

	if (x < 0) {							/* If argument is negative, compute the imaginary root */
		x = -x;
		imag = 1;
	}
	root  = 0;								/* Clear root */
	remHi = 0;								/* Clear high part of partial remainder */
	remLo = (UInt32)(x >> 32);				/* Get MSBs of argument into low part of partial remainder */

	if (remLo != 0) {
		for (count = 16; count--; ) {
			remHi = (remHi << 2) | (remLo >> 30); remLo <<= 2;	/* get 2 bits of arg */
			root <<= 1; 					/* Get ready for the next bit in the root */
			testDiv = (root << 1) + 1;		/* Test divisor */
			if (remHi >= testDiv) {
				remHi -= testDiv;
				root += 1;
			}
		}
	}
	remLo = (UInt32)x;					/* Get LSBs of argument into low part of partial remainder */
	for (count = 16; count--; ) {
		remHi = (remHi << 2) | (remLo >> 30); remLo <<= 2;	/* get 2 bits of arg */
		root <<= 1;						/* Get ready for the next bit in the root */
		testDiv = (root << 1) + 1;		/* Test divisor */
		if (remHi >= testDiv) {
			remHi -= testDiv;
			root += 1;
		}
	}

	if (remHi > (UInt32)root) root++;	/* If the next bit would have been a 1, round up */

	if (imag)	root = -root;			/* Set the imaginary bit if the argument was negative */

	return root;
}


/********************************************************************************
 * FskFixedHypot
 ********************************************************************************/

FskFixed
FskFixedHypot(FskFixed x, FskFixed y)
{
	return FskFixedSqrt64to32((FskInt64)x * x + (FskInt64)y * y);
}


#if PRAGMA_MARK_SUPPORTED
#pragma mark -
#endif /*  PRAGMA_MARK_SUPPORTED */
/********************************************************************************
 ********************************************************************************
 *****						Fixed point trigonometry						*****
 ********************************************************************************
 ********************************************************************************/

#define COSCALE 0x11616E8E	/* 291597966 = 0.2715717684432241 * 2^30, valid for j>13 */

static FskFixedDegrees arctantab[] = {
	#define QUARTER (90 << 16)
	#define MAXITER 22	/* the resolution of the arctan table */
		4157273, 2949120, 1740967, 919879, 466945, 234379, 117304, 58666,
		29335, 14668, 7334, 3667, 1833, 917, 458, 229,
		115, 57, 29, 14, 7, 4, 2, 1
};


/********************************************************************************
 * PseudoRotate
 *
 * To rotate a vector through an angle of theta, we calculate:
 *
 *	x' = x cos(theta) - y sin(theta)
 *	y' = x sin(theta) + y cos(theta)
 *
 * The rotate() routine performs multiple rotations of the form:
 *
 *	x[i+1] = cos(theta[i]) { x[i] - y[i] tan(theta[i]) }
 *	y[i+1] = cos(theta[i]) { y[i] + x[i] tan(theta[i]) }
 *
 * with the constraint that tan(theta[i]) = pow(2, -i), which can be
 * implemented by shifting. We always shift by either a positive or
 * negative angle, so the convergence has the ringing property. Since the
 * cosine is always positive for positive and negative angles between -90
 * and 90 degrees, a predictable magnitude scaling occurs at each step,
 * and can be compensated for instead at the end of the iterations by a
 * composite scaling of the product of all the cos(theta[i])'s.
 ********************************************************************************/

static void
PseudoRotate(SInt32 *px, SInt32 *py, register FskFixedDegrees theta)
{
	register int i;
	register SInt32 x, y, xtemp;
	register SInt32 *arctanptr;

	x = *px;
	y = *py;

	/* Get angle between -90 and 90 degrees */
	while (theta < -QUARTER) {
		x = -x;
		y = -y;
		theta += 2 * QUARTER;
	}
	while (theta > QUARTER) {
		x = -x;
		y = -y;
		theta -= 2 * QUARTER;
	}

	/* Initial pseudorotation, with left shift */
	arctanptr = arctantab;
	if (theta < 0) {
		xtemp = x + (y << 1);
		y     = y - (x << 1);
		x     = xtemp;
		theta += *arctanptr++;
	}
	else {
		xtemp = x - (y << 1);
		y     = y + (x << 1);
		x     = xtemp;
		theta -= *arctanptr++;
	}

	/* Subsequent pseudorotations, with right shifts */
	for (i = 0; i <= MAXITER; i++) {
		if (theta < 0) {
			xtemp = x + (y >> i);
			y     = y - (x >> i);
			x     = xtemp;
			theta += *arctanptr++;
		}
		else {
			xtemp = x - (y >> i);
			y     = y + (x >> i);
			x     = xtemp;
			theta -= *arctanptr++;
		}
	}

	*px = x;
	*py = y;
}


/********************************************************************************
 * PseudoPolarize
 ********************************************************************************/

static void
PseudoPolarize(SInt32 *argx, SInt32 *argy)
{
	register SInt32	theta, x, y, yi, *arctanptr;
	register int i;

	x = *argx;
	y = *argy;

	/* Get the vector into the right half plane */
	theta = 0;
	if (x < 0) {
		x = -x;
		y = -y;
		theta = 2 * QUARTER;
	}

	if (y > 0)
		theta = - theta;

	arctanptr = arctantab;
	if (y < 0) {	/* Rotate positive */
		yi = y + (x << 1);
		x  = x - (y << 1);
		y  = yi;
		theta -= *arctanptr++;	/* Subtract angle */
	}
	else {		/* Rotate negative */
		yi = y - (x << 1);
		x  = x + (y << 1);
		y  = yi;
		theta += *arctanptr++;	/* Add angle */
	}

	for (i = 0; i <= MAXITER; i++) {
		if (y < 0) {	/* Rotate positive */
			yi = y + (x >> i);
			x  = x - (y >> i);
			y  = yi;
			theta -= *arctanptr++;
		}
		else {		/* Rotate negative */
			yi = y - (x >> i);
			x  = x + (y >> i);
			y  = yi;
			theta += *arctanptr++;
		}
	}

	*argx = x;
	*argy = theta;
}


/********************************************************************************
 * FxPreNorm() block normalizes the arguments to a magnitude suitable for
 * CORDIC pseudorotations.  The returned value is the block exponent.
 ********************************************************************************/

static int
FxPreNorm(SInt32 *argx, SInt32 *argy)
{
	register SInt32	x, y;
	int				signx, signy;
	register int	shiftexp;

	shiftexp = 0;		/* Block normalization exponent */
	signx = signy = 1;

	if ((x = *argx) < 0) {
		x = -x;
		signx = -signx;
	}
	if ((y = *argy) < 0) {
		y = -y;
		signy = -signy;
	}
	/* Prenormalize vector for maximum precision */
	if (x < y) {	/* |y| > |x| */
		while (y < (1 << 27)) {
			x <<= 1;
			y <<= 1;
			shiftexp--;
		}
		while (y > (1 << 28)) {
			x >>= 1;
			y >>= 1;
			shiftexp++;
		}
	}
	else {		/* |x| > |y| */
		while (x < (1 << 27)) {
			x <<= 1;
			y <<= 1;
			shiftexp--;
		}
		while (x > (1 << 28)) {
			x >>= 1;
			y >>= 1;
			shiftexp++;
		}
	}

	*argx = (signx < 0) ? -x : x;
	*argy = (signy < 0) ? -y : y;
	return(shiftexp);
}


/********************************************************************************
 * FskFracCosineSine
 *
 * Return a unit vector corresponding to theta.
 * sin and cos are fixed-point fractions.
 ********************************************************************************/

void
FskFracCosineSine(FskFixedDegrees theta, FskFract *cosineSine)
{
	cosineSine[0] = COSCALE;
	cosineSine[1] = 0;
	PseudoRotate(cosineSine+0, cosineSine+1, theta);
}


/********************************************************************************
 * FskFixedVectorRotate
 *
 * rotates the vector (argx, argy) by the angle theta.
 ********************************************************************************/

void
FskFixedVectorRotate(SInt32 xy[2], FskFixedDegrees theta)
{
	int shiftexp;

	if (((xy[0] == 0) && (xy[1] == 0)) || (theta == 0))
		return;

	shiftexp = FxPreNorm(xy+0, xy+1);	/* Prenormalize vector */
	PseudoRotate(xy+0, xy+1, theta);	/* Perform CORDIC pseudorotation */
	xy[0] = FskFracMul(xy[0], COSCALE);	/* Compensate for CORDIC enlargement */
	xy[1] = FskFracMul(xy[1], COSCALE);
	if (shiftexp < 0) {					/* Denormalize vector */
		xy[0] >>= -shiftexp;
		xy[1] >>= -shiftexp;
	}
	else {
		xy[0] <<= shiftexp;
		xy[1] <<= shiftexp;
	}
}


/********************************************************************************
 * FskFixedVectorInclination
 *
 * Determine the angle of the vector.
 ********************************************************************************/

FskFixedDegrees
FskFixedVectorInclination(SInt32 x, SInt32 y)
{
	if (y == 0) {
		if (x >= 0)	return(0);
		else		return(180 << 16);
	}
	FxPreNorm(&x, &y);		/* Prenormalize vector for maximum precision */
	PseudoPolarize(&x, &y);	/* Convert to polar coordinates */
	return(y);
}


/********************************************************************************
 * FskFixedPolarize
 ********************************************************************************/

void
FskFixedPolarize(SInt32 *argx, SInt32 *argy)
{
	int shiftexp;

	if ((*argx == 0) && (*argy == 0)) {
		return;
	}

	/* Prenormalize vector for maximum precision */
	shiftexp = FxPreNorm(argx, argy);

	/* Perform CORDIC conversion to polar coordinates */
	PseudoPolarize(argx, argy);

	/* Scale radius to undo pseudorotation enlargement factor */
	*argx = FskFracMul(*argx, COSCALE);

	/* Denormalize radius */
	*argx = (shiftexp < 0) ? (*argx >> -shiftexp) : (*argx << shiftexp);
}



#if PRAGMA_MARK_SUPPORTED
#pragma mark -
#endif /*  PRAGMA_MARK_SUPPORTED */
/********************************************************************************
 ********************************************************************************
 ********************************************************************************
 ***							Bezier Utilities							  ***
 ********************************************************************************
 ********************************************************************************
 ********************************************************************************/


/********************************************************************************
 * FskFixedDeCasteljau
 * eval is lower triangular, and contains at least
 *		(order) * (order + 1) / 2
 * elements
 *	...
 *	b03	...
 *	b02	b12	...
 *	b01	b11	b21	...
 *	b0	b1	b2	b3	...
 ********************************************************************************/

void
FskFixedDeCasteljau(
	const FskFixed		*control,
	UInt32				order,
	SInt32				strideBytes,
	register FskFract	t,
	FskFixed			*eval
)
{
	FskFixed				*b0			= eval + ((order * (order-1)) >> 1);
	register char			*c			= (char*)control;
	register FskFixed		*b, *d;
	register unsigned int	i, j;

	/* Copy data for easy processing */
	for (i = order, b = b0; i--; c += strideBytes)
		*b++ = *((FskFixed*)c);

	/* Repeated linear interpolation */
	for (i = order - 1, d = b0 - 1, b = d + order; i--; b--)
		for (j = i+1; j--;  b--)
			*d-- = b[-1] + FskFixedNMulInline(t, (b[0] - b[-1]), 30);
}


/********************************************************************************
 * FskFixedDeCasteljauKernel
 ********************************************************************************/

void
FskFixedDeCasteljauKernel(
	FskFixed			*tri,	/* [order * (order + 1) / 2] */
	UInt32				order,
	register FskFract	t
)
{
	register FskFixed		*b, *d;
	register unsigned int	i, j;

	/* Repeated linear interpolation */
	for (i = order - 1, d = tri + ((order * (order-1)) >> 1) - 1, b = d + order; i--; b--)
		for (j = i+1; j--;  b--)
			*d-- = b[-1] + FskFixedNMulInline(t, (b[0] - b[-1]), 30);
}


/********************************************************************************
 * FskFixedSplitBezier
 ********************************************************************************/

void
FskFixedSplitBezier(
	const FskFixed		*control,
	UInt32				order,
	UInt32				dimension,
	FskFixed			t,
	FskFixed			*L,
	FskFixed			*R
)
{
	FskFixed		eval[kFskMaxBezierOrder*(kFskMaxBezierOrder+1)/2];
	unsigned int	strideBytes = sizeof(FskFixed) * dimension;
	unsigned int	i;
	FskFixed		*fl, *fr, *l, *r;

	for ( ; dimension--; control++, L++, R++) {
		FskFixedDeCasteljau(control, order, strideBytes, t, eval);
		for (i = 0, l = (FskFixed*)((char*)L + (order - 1) * strideBytes), r = R, fl = fr = eval;
			i < order;
			i++, l = (FskFixed*)((char*)l - strideBytes), r = (FskFixed*)((char*)r + strideBytes), fl += i, fr += i + 1
		) {
			*l = *fl;
			*r = *fr;
		}
	}
}


/********************************************************************************
 * FskFixedBisectDeCasteljau
 ********************************************************************************/
#define AVOID_DECASTELJAU_OVERFLOW	/* Better to be robust, even if it is a little slower */

void
FskFixedBisectDeCasteljau(
	const FskFixed		*control,
	UInt32				order,
	SInt32				strideBytes,
	FskFixed			*eval
)
{
	FskFixed				*b0			= eval + ((order * (order-1)) >> 1);
	register char			*c			= (char*)control;
	register FskFixed		*b, *d;
	register unsigned int	i, j;

	/* Copy data for easy processing */
	for (i = order, b = b0; i--; c += strideBytes)
		*b++ = *((FskFixed*)c);

	/* Repeated averaging */
	for (i = order - 1, d = b0 - 1, b = d + order; i--; b--)
		for (j = i+1; j--;  b--)
			#ifndef AVOID_DECASTELJAU_OVERFLOW
				*d-- = (b[-1] + b[0]) >> 1;								/* We need to assure that abs(b[k]) < 0x20000000 */
			#else /* AVOID_DECASTELJAU_OVERFLOW */
				*d-- = (b[-1] >> 1) + (b[0] >> 1) + (b[-1] & b[0] & 1);
			#endif /* AVOID_DECASTELJAU_OVERFLOW */
}


/********************************************************************************
 * FskFixedBisectBezier
 ********************************************************************************/

void
FskFixedBisectBezier(
	const FskFixed		*control,
	UInt32				order,
	UInt32				dimension,
	FskFixed			*L,
	FskFixed			*R
)
{
	FskFixed		eval[kFskMaxBezierOrder*(kFskMaxBezierOrder+1)/2];
	unsigned int	strideBytes = sizeof(FskFixed) * dimension;
	unsigned int	i;
	FskFixed		*fl, *fr, *l, *r;

	for ( ; dimension--; control++, L++, R++) {
		FskFixedBisectDeCasteljau(control, order, strideBytes, eval);
		for (i = 0, l = (FskFixed*)((char*)L + (order - 1) * strideBytes), r = R, fl = fr = eval;
			i < order;
			i++, l = (FskFixed*)((char*)l - strideBytes), r = (FskFixed*)((char*)r + strideBytes), fl += i, fr += i + 1
		) {
			*l = *fl;
			*r = *fr;
		}
	}
}


/********************************************************************************
 * FskFixedDeviationOfBezierControlPoints2D
 ********************************************************************************/

FskFixed
FskFixedDeviationOfBezierControlPoints2D(const FskFixedPoint2D *points, SInt32 order)
{
	FskFixed				t, d;
	FskFractVector2D		baseVec, skewVec;
	int						i;
	const FskFixedPoint2D	*p;

	/* Compute unit vector along base */
	p = points + order - 1;
	baseVec.x = p->x - points->x;
	baseVec.y = p->y - points->y;
	FskFixedVectorNormalize(&baseVec.x, 2);

	/* Find maximum distance of control points from the base */
	for (i = order - 2, p--, d = 0; i--; p--) {
		skewVec.x = p->x - points->x;
		skewVec.y = p->y - points->y;
		t = FskFractCrossProduct2D(&baseVec, &skewVec);
		if (t < 0)
			t = -t;
		if (d < t)
			d = t;
	}

	return(d);
}


/********************************************************************************
 * FskFixedDerivativeOfBezier
 ********************************************************************************/

void
FskFixedDerivativeOfBezier(const FskFixed *coeff, FskFixed *deriv, SInt32 order)
{
	int i;

	for (i = --order; i--; coeff++, deriv++)
		*deriv = (coeff[1] - coeff[0]) * order;
}


/********************************************************************************
 * FskFixedEvaluateBezierVector
 ********************************************************************************/

void
FskFixedEvaluateBezierVector(const FskFixed *coeff, SInt32 order, UInt32 dimension, FskFixed t, FskFixed *position, FskFixed *tangent)
{
	SInt32		strideBytes	= sizeof(FskFixed) * dimension;
	FskFixed	eval[kFskMaxBezierOrder*(kFskMaxBezierOrder+1)/2];
	int			i;

	for (i = dimension; i--; coeff++) {
		FskFixedDeCasteljau(coeff, order, strideBytes, t, eval);
		if (position != NULL)	*position++ = eval[0];
		if (tangent  != NULL)	*tangent++  = (eval[2] - eval[1]) * order;
	}
}


/********************************************************************************
 * FskFixedHorner -- evaluation of a power-basis polynomial via Horner's rule
 ********************************************************************************/

FskFixed
FskFixedHorner(const FskFixed *coeff, SInt32 order, FskFract x)
{
	FskFixed acc;

	for (coeff += order--, acc = *--coeff; order--; )
		acc = FskFracMul(x, acc) + *--coeff;
	return(acc);
}


#if PRAGMA_MARK_SUPPORTED
#pragma mark -
#endif /*  PRAGMA_MARK_SUPPORTED */
/********************************************************************************
 ********************************************************************************
 ********************************************************************************
 *****						Vector and Matrix Functions						*****
 ********************************************************************************
 ********************************************************************************
 ********************************************************************************/


/********************************************************************************
 * FskFixedVectorNorm
 ********************************************************************************/

FskFixed
FskFixedVectorNorm(register const FskFixed *v, register SInt32 n)
{
	register FskInt64 acc;

	for (acc = 0; n--; v++)
		acc += (FskInt64)(*v) * (*v);
	return FskFixedSqrt64to32(acc);
}


/********************************************************************************
 * FskFixedVectorNormalize
 ********************************************************************************/

FskFract
FskFixedVectorNormalize(register FskFract *v, register SInt32 n)
{
	register FskFract norm;

	if ((norm = FskFixedVectorNorm(v, n)) != 0) {
		for ( ; n--; v++)
			*v = FskFixedNDivInline(*v, norm, 30);
	}
	else {
		for ( ; n--; v++)
			*v = 0;
	}
	return(norm);
}


/********************************************************************************
 * FskFixedDistance
 ********************************************************************************/

FskFixed
FskFixedDistance(const FskFixed *p0, const FskFixed *p1, SInt32 n)
{
	register FskInt64	acc;
	register FskFixed	d;

	for (acc = 0; n--; ) {
		d = *p1++ - *p0++;
		acc += (FskInt64)(d) * (d);
	}
	return FskFixedSqrt64to32(acc);
}


/********************************************************************************
 * FskFixedNDotProduct
 ********************************************************************************/

FskFixed
FskFixedNDotProduct(const FskFixed *a, const FskFixed *b, UInt32 length, SInt32 numSubBits)
{
	FskInt64	acc	= (FskInt64)(1 << (numSubBits-1));
	FskFixed	d;

	while (length--)
		acc += (FskInt64)*a++ * *b++;
	acc >>= numSubBits;
	d    = (FskFixed)(acc);
	MULT_SATURATE(acc, d);

	return d;
}


/********************************************************************************
 * FskFixedDotProduct
 ********************************************************************************/

FskFixed
FskFixedDotProduct(const FskFixed *a, const FskFixed *b, UInt32 n)
{
	return FskFixedNDotProduct(a, b, n, 16);
}


/********************************************************************************
 * FskFractDotProduct
 ********************************************************************************/

FskFract
FskFractDotProduct(const FskFract *a, const FskFract *b, UInt32 n)
{
	return FskFixedNDotProduct(a, b, n, 30);
}


/********************************************************************************
 * FskFixedInterpolate
 ********************************************************************************/

void
FskFixedInterpolate(FskFract t, UInt32 n, const FskFixed *p0, const FskFixed *p1, FskFixed *pt)
{
	for ( ; n--; p0++, p1++, pt++)
		*pt = FskFracMul(t, (*p1 - *p0)) + *p0;
}


/********************************************************************************
 * FskFixedNLinear2D
 ********************************************************************************/

FskFixed
FskFixedNLinear2D(FskFixed a0, FskFixed b0, FskFixed a1, FskFixed b1, SInt32 n)
{
	FskInt64	acc;
	FskFixed	c;

	acc  = (FskInt64)1 << (n - 1);	/* Rounding */
	acc += (FskInt64)a0 * b0;		/* First  accumulated product */
	acc += (FskInt64)a1 * b1;		/* Second accumulated product */
	acc >>= n;						/* Normalize */
	c    = (FskFixed)(acc);
	MULT_SATURATE(acc, c);

	return c;
}


/********************************************************************************
 * FskFixedNAffine2D
 ********************************************************************************/

FskFixed
FskFixedNAffine2D(FskFixed a0, FskFixed b0, FskFixed a1, FskFixed b1, FskFixed a2, SInt32 n)
{
	FskInt64	acc;
	FskFixed	c;

	acc  = (FskInt64)1  << (n - 1);	/* Rounding */
	acc += (FskInt64)a2 << n;		/* Offset */
	acc += (FskInt64)a0 * b0;		/* First  accumulated product */
	acc += (FskInt64)a1 * b1;		/* Second accumulated product */
	acc >>= n;						/* Normalize */
	c    = (FskFixed)(acc);
	MULT_SATURATE(acc, c);

	return c;
}


/********************************************************************************
 * FskFixedCrossProduct2D
 ********************************************************************************/

FskFixed
FskFixedCrossProduct2D(const FskFixedVector2D *a, const FskFixedVector2D *b)
{
	return FskFixedNLinear2D(a->x, b->y, -a->y, b->x, 16);
}


/********************************************************************************
 * FskFractCrossProduct2D
 ********************************************************************************/

FskFract
FskFractCrossProduct2D(const FskFractVector2D *a, const FskFractVector2D *b)
{
	return FskFixedNLinear2D(a->x, b->y, -a->y, b->x, 30);
}


/********************************************************************************
 * FskFixedPointLineDistance2D
 ********************************************************************************/

FskFixed
FskFixedPointLineDistance2D(const FskFixedPoint2D *point, const FskFixedPoint2D *line)
{
	FskFixedVector2D lin, stt;

	lin.x = line[1].x - line[0].x;
	lin.y = line[1].y - line[0].y;
	stt.x = point->x  - line[0].x;
	stt.y = point->y  - line[0].y;
	FskFixedVectorNormalize(&lin.x, 2);
	return FskFixedCrossProduct2D(&lin, &stt);
}


/********************************************************************************
 * FskFixedPerpVector2D
 * Rotate a vector 90 degrees. This works in-place.
 ********************************************************************************/

void
FskFixedPerpVector2D(const FskFixedVector2D *fr, FskFixedVector2D *to)
{
	FskFixedVector2D f = *fr;

	to->x = -f.y;
	to->y =  f.x;
}


/********************************************************************************
 * FskFixedIdentityMatrix3x2
 ********************************************************************************/

void
FskFixedIdentityMatrix3x2(FskFixedMatrix3x2 *M)
{
	M->M[0][0] = M->M[1][1] = 0x00010000;
	M->M[0][1] = M->M[1][0] = M->M[2][0] = M->M[2][1] = 0;
}


/********************************************************************************
 * FskFixedMatrixNorm2x2
 *	This norm is the geometric average of the magnitude of the eigenvectors.
 *	It yields a suitable average scale factor.
 *	Not found in the literature, we may as well call it the Turkowski norm.
 ********************************************************************************/

FskFixed
FskFixedMatrixNorm2x2(const FskFixed *M, SInt32 nCols)
{
	FskInt64 x;

	x = ((FskInt64)M[0*nCols+0] * M[1*nCols+1] - (FskInt64)M[0*nCols+1] * M[1*nCols+0]);
	if (x < 0)
		x = -x;
	return FskFixedSqrt64to32(x);
}


/********************************************************************************
 * FskFixedMatrixInvert2x2
 *
 *	This works in-place
 ********************************************************************************/

FskFixed
FskFixedMatrixInvert2x2(const FskFixed *fr, FskFixed *to)
{
	FskInt64	d, rnd, t[2][2];

	t[0][0] = fr[0];		t[0][1] = fr[1];				/* Convert from 32 to 64 bits */
	t[1][0] = fr[2];		t[1][1] = fr[3];

	if ((d = (t[0][0]*t[1][1] - t[1][0]*t[0][1])) == 0)		/* Determinant as 32.32 */
		return 0;

	rnd = d >> 1;											/* Rounding term to add prior to division */
	to[0] =  (FskFixed)(((t[1][1] << 32) + rnd) / d);
	to[1] = -(FskFixed)(((t[0][1] << 32) + rnd) / d);
	to[2] = -(FskFixed)(((t[1][0] << 32) + rnd) / d);
	to[3] =  (FskFixed)(((t[0][0] << 32) + rnd) / d);

	return((FskFixed)(d >> 16));							/* Return 16.16 determinant */
}


/********************************************************************************
 * FskFixedMatrixInvert3x2
 *
 *	This works in-place
 ********************************************************************************/

FskFixed
FskFixedMatrixInvert3x2(const FskFixedMatrix3x2 *fr, FskFixedMatrix3x2 *to)
{
	FskFixedMatrix3x2	M;
	FskFixed			det;

	det = FskFixedMatrixInvert2x2(fr->M[0], M.M[0]);
	M.M[2][0] = -FskFixedNLinear2D(fr->M[2][0], M.M[0][0], fr->M[2][1], M.M[1][0], 16);
	M.M[2][1] = -FskFixedNLinear2D(fr->M[2][0], M.M[0][1], fr->M[2][1], M.M[1][1], 16);
	*to = M;

	return det;
}


/********************************************************************************
 * FskFixedGeneralMatrixInvert3x2
 * The source matrix fr has frSBits fractional bits for the upper 2x2, frTBits for the lower 1x2
 * The result matrix to has toSBits fractional bits for the upper 2x2, toTBits for the lower 1x2
 *	This works in-place
 ********************************************************************************/

FskFixed
FskFixedGeneralMatrixInvert3x2(
	const FskFixedMatrix3x2	*fr,	SInt32	frSBits,	SInt32	frTBits,
	FskFixedMatrix3x2		*to,	SInt32	toSBits,	SInt32	toTBits
)
{
	FskFixedMatrix3x2	M;
	FskInt64			d;
	SInt32				n0		= frSBits + toSBits;
	SInt32				n1		= frSBits + toTBits - frTBits;

	if ((d = ((FskInt64)(fr->M[0][0]) * fr->M[1][1] - (FskInt64)(fr->M[1][0]) * fr->M[0][1])) == 0)		/* Determinant has (2*frSBits) fractional bits */
		return 0;

	M.M[0][0] = FskFixNDiv64((FskInt64)( fr->M[1][1]), d, n0);
	M.M[0][1] = FskFixNDiv64((FskInt64)(-fr->M[0][1]), d, n0);
	M.M[1][0] = FskFixNDiv64((FskInt64)(-fr->M[1][0]), d, n0);
	M.M[1][1] = FskFixNDiv64((FskInt64)( fr->M[0][0]), d, n0);
	M.M[2][0] = FskFixNDiv64((FskInt64)( fr->M[1][0]) * fr->M[2][1] - (FskInt64)(fr->M[1][1]) * fr->M[2][0], d, n1);
	M.M[2][1] = FskFixNDiv64((FskInt64)( fr->M[0][1]) * fr->M[2][0] - (FskInt64)(fr->M[0][0]) * fr->M[2][1], d, n1);
	*to = M;

	if ((n0 = (SInt32)(d >> frSBits)) == 0)		/* Determinant isn't zero, but we're getting underflow */
		n0 = 1;									/* Set it to the smallest nonzero value */
	return (FskFixed)n0;						/* Return the determinant with frSBits fractional bits */
}


/********************************************************************************
 * FskFixedNMultiplyMatrix3x2
 * n = nL + nR - nP
 *	This works in-place
 ********************************************************************************/

void
FskFixedNMultiplyMatrix3x2(const FskFixedMatrix3x2 *L, const FskFixedMatrix3x2 *R, FskFixedMatrix3x2 *P, SInt32 n)
{
	FskFixedMatrix3x2 C;

	C.M[0][0] = FskFixedNLinear2D(L->M[0][0], R->M[0][0],    L->M[0][1], R->M[1][0],    n);
	C.M[0][1] = FskFixedNLinear2D(L->M[0][0], R->M[0][1],    L->M[0][1], R->M[1][1],    n);

	C.M[1][0] = FskFixedNLinear2D(L->M[1][0], R->M[0][0],    L->M[1][1], R->M[1][0],    n);
	C.M[1][1] = FskFixedNLinear2D(L->M[1][0], R->M[0][1],    L->M[1][1], R->M[1][1],    n);

	C.M[2][0] = FskFixedNAffine2D(L->M[2][0], R->M[0][0],    L->M[2][1], R->M[1][0],    R->M[2][0],    n);
	C.M[2][1] = FskFixedNAffine2D(L->M[2][0], R->M[0][1],    L->M[2][1], R->M[1][1],    R->M[2][1],    n);

	*P = C;
}


/********************************************************************************
 * FskFixedGeneralMultiplyMatrix3x2
 *		L * R
 * The  left   matrix L is has LScaleBits fractional bits for its upper 2x2 and LTransBits fractional bits for its lower 1x2
 * The  right  matrix R is has RScaleBits fractional bits for its upper 2x2 and RTransBits fractional bits for its lower 1x2
 * The product matrix P is has PScaleBits fractional bits for its upper 2x2 and PTransBits fractional bits for its lower 1x2
 * This works in-place.
 ********************************************************************************/

void
FskFixedGeneralMultiplyMatrix3x2(
	const FskFixedMatrix3x2	*L,		SInt32 LScaleBits,	SInt32 LTransBits,
	const FskFixedMatrix3x2	*R,		SInt32 RScaleBits,	SInt32 RTransBits,
	FskFixedMatrix3x2		*P,		SInt32 PScaleBits,	SInt32 PTransBits
) {
	FskFixedMatrix3x2 C;
	SInt32	n0	= LScaleBits + RScaleBits - PScaleBits;		/* We should probably make sure these are positive */
	SInt32	n1	= LTransBits + RScaleBits - PTransBits;
	SInt32	n2	= LTransBits + RScaleBits - RTransBits;

	C.M[0][0] = FskFixedNLinear2D(L->M[0][0], R->M[0][0],    L->M[0][1], R->M[1][0],    n0);
	C.M[0][1] = FskFixedNLinear2D(L->M[0][0], R->M[0][1],    L->M[0][1], R->M[1][1],    n0);
	C.M[1][0] = FskFixedNLinear2D(L->M[1][0], R->M[0][0],    L->M[1][1], R->M[1][0],    n0);
	C.M[1][1] = FskFixedNLinear2D(L->M[1][0], R->M[0][1],    L->M[1][1], R->M[1][1],    n0);
	C.M[2][0] = (FskFixed)(((FskInt64)(L->M[2][0]) * R->M[0][0] + (FskInt64)(L->M[2][1]) * R->M[1][0] + ((FskInt64)(R->M[2][0]) << n2) + ((FskInt64)1 << (n1 - 1))) >> n1);
	C.M[2][1] = (FskFixed)(((FskInt64)(L->M[2][0]) * R->M[0][1] + (FskInt64)(L->M[2][1]) * R->M[1][1] + ((FskInt64)(R->M[2][1]) << n2) + ((FskInt64)1 << (n1 - 1))) >> n1);

	*P = C;
}


/********************************************************************************
 * FskFixedNMultiplyInverseMatrix3x2
 *		L^-1 * R
 * The left matrix L is assumed to have 16 fractional bits.
 * The right matrix may have any number of fractional bits.
 * This works in-place.
 ********************************************************************************/

FskFixed
FskFixedNMultiplyInverseMatrix3x2(const FskFixedMatrix3x2 *L, const FskFixedMatrix3x2 *R, FskFixedMatrix3x2 *P, SInt32 LFracBits)
{
	FskInt64 det;

	if ((det = (FskInt64)(L->M[1][1]) * L->M[0][0] - (FskInt64)(L->M[0][1]) * L->M[1][0]) != 0) {
		FskFixedMatrix3x2 C;
		C.M[0][0] = FskFixNDiv64((FskInt64)(R->M[0][0]) * L->M[1][1] - (FskInt64)(R->M[1][0]) * L->M[0][1], det, LFracBits);
		C.M[0][1] = FskFixNDiv64((FskInt64)(R->M[0][1]) * L->M[1][1] - (FskInt64)(R->M[1][1]) * L->M[0][1], det, LFracBits);
		C.M[1][0] = FskFixNDiv64((FskInt64)(R->M[1][0]) * L->M[0][0] - (FskInt64)(R->M[0][0]) * L->M[1][0], det, LFracBits);
		C.M[1][1] = FskFixNDiv64((FskInt64)(R->M[1][1]) * L->M[0][0] - (FskInt64)(R->M[0][1]) * L->M[1][0], det, LFracBits);
		C.M[2][0] = FskFixedNAffine2D(-L->M[2][0], C.M[0][0], -L->M[2][1], C.M[1][0], R->M[2][0], LFracBits);
		C.M[2][1] = FskFixedNAffine2D(-L->M[2][0], C.M[0][1], -L->M[2][1], C.M[1][1], R->M[2][1], LFracBits);
		*P = C;
		if ((LFracBits = (SInt32)(det >> LFracBits)) == 0)	/* Determinant is not 0, but we get underflow */
			LFracBits = 1;									/* So we set it to the smallest nonzero value */
		return (FskFixed)LFracBits;							/* Return the determinant */
	}
	else {
		P->M[0][0] = P->M[0][1] = P->M[1][0] = P->M[1][1] = 0;
		P->M[2][0] = R->M[2][0];
		P->M[2][1] = R->M[2][1];
		return (FskFixed)0;
	}
}


/********************************************************************************
 * FskFixedMultiplyMatrixInverse3x2
 *		L * R^1
 * This assumes that L, R, and P all have 16 fractional bits.
 *	This works in-place
 ********************************************************************************/

FskFixed
FskFixedMultiplyMatrixInverse3x2(const FskFixedMatrix3x2 *L, const FskFixedMatrix3x2 *R, FskFixedMatrix3x2 *P)
{
	FskInt64	det;
	FskFixed	t[2];

	t[0] = L->M[2][0] - R->M[2][0];
	t[1] = L->M[2][1] - R->M[2][1];

	if ((det = (FskInt64)(R->M[0][0]) * R->M[1][1] - (FskInt64)(R->M[0][1]) * R->M[1][0]) != 0) {
		FskFixedMatrix3x2 C;
		C.M[0][0] = FskFixNDiv64((FskInt64)(L->M[0][0]) * R->M[1][1] - (FskInt64)(L->M[0][1]) * R->M[1][0], det, 16);
		C.M[0][1] = FskFixNDiv64((FskInt64)(L->M[0][1]) * R->M[0][0] - (FskInt64)(L->M[0][0]) * R->M[0][1], det, 16);
		C.M[1][0] = FskFixNDiv64((FskInt64)(L->M[1][0]) * R->M[1][1] - (FskInt64)(L->M[1][1]) * R->M[1][0], det, 16);
		C.M[1][1] = FskFixNDiv64((FskInt64)(L->M[1][1]) * R->M[0][0] - (FskInt64)(L->M[1][0]) * R->M[0][1], det, 16);
		C.M[2][0] = FskFixNDiv64((FskInt64)(      t[0]) * R->M[1][1] - (FskInt64)(      t[1]) * R->M[1][0], det, 16);
		C.M[2][1] = FskFixNDiv64((FskInt64)(      t[1]) * R->M[0][0] - (FskInt64)(      t[0]) * R->M[0][1], det, 16);
		*P = C;
		if ((t[0] = (FskFixed)(det >> 16)) == 0)	/* Determinant is not 0, but we get underflow */
			t[0] = 1;								/* So we set it to the smallest nonzero value */
		return t[0];								/* Return the determinant */
	}
	else {
		P->M[0][0] = P->M[0][1] = P->M[1][0] = P->M[1][1] = 0;
		P->M[2][0] = t[0];
		P->M[2][1] = t[1];
		return (FskFixed)0;
	}
}


/********************************************************************************
 * FskTransformFixedRowPoints2D
 *
 *	Transform the points by the given matrix. It is assumed that the points are arranged in rows,
 *	and that they are row-scanned (as per C convention).
 *	This works in-place
 ********************************************************************************/

void
FskTransformFixedRowPoints2D(const FskFixedPoint2D *src, UInt32 numPoints, const FskFixedMatrix3x2 *M, FskFixedPoint2D *dst)
{
	FskFixedPoint2D		q;

	for ( ; numPoints--; src++, dst++) {
		q.x = FskFixedNAffine2D(M->M[0][0], src->x,    M->M[1][0], src->y,    M->M[2][0],    16);
		q.y = FskFixedNAffine2D(M->M[0][1], src->x,    M->M[1][1], src->y,    M->M[2][1],    16);
		*dst = q;
	}
}


/********************************************************************************
 * FskInverseTransformFixedRowPoints2D
 *
 *	Transform the points by the given inverse of the matrix. It is assumed that
 *	the points are arranged in rows, and that they are row-scanned (as per C convention).
 *	A scaled determinant is returned, suitable mostly for comparison against zero.
 *	This works in-place.
 ********************************************************************************/

FskFixed
FskInverseTransformFixedRowPoints2D(const FskFixedPoint2D *src, UInt32 numPoints, const FskFixedMatrix3x2 *M, FskFixedPoint2D *dst)
{
	FskFixed			d;
#if 0	/* Less robust, but may be faster for more than one point */
	FskFixedMatrix3x2	Mi;

	if ((d = FskFixedMatrixInvert3x2(M, &Mi)) != 0) {
		FskTransformFixedRowPoints2D(src, numPoints, &Mi, dst);
	}
	return d;
#else	/* More robust and faster for one point */
	FskFract	f;
	FskFixed	b0, b1, x, y;

	/* Find maximum element for pivot */
	if ((d = M->M[0][0]) < 0)	d = -d;		x = 0;
	if ((f = M->M[0][1]) < 0)	f = -f;		if (d < f) {	d = f;	x = 1;	}
	if ((f = M->M[1][0]) < 0)	f = -f;		if (d < f) {	d = f;	x = 2;	}
	if ((f = M->M[1][1]) < 0)	f = -f;		if (d < f) {	d = f;	x = 3;	}

	switch (x) {
		case 0:
			f = FskFracDiv(M->M[0][1], M->M[0][0]);
			if ((d = M->M[1][1] - FskFracMul(f, M->M[1][0])) != 0) {
				for ( ; numPoints--; src++, dst++) {
					b0 = src->x - M->M[2][0];
					b1 = src->y - M->M[2][1];
					y = FskFixDiv(b1 - FskFracMul(f, b0), d);
					x = FskFixDiv(b0 - FskFixMul(y, M->M[1][0]), M->M[0][0]);
					dst->x = x;
					dst->y = y;
				}
			}
			break;
		case 1:
			f = FskFracDiv(M->M[0][0], M->M[0][1]);
			if ((d = M->M[1][0] - FskFracMul(f, M->M[1][1])) != 0) {
				for ( ; numPoints--; src++, dst++) {
					b0 = src->x - M->M[2][0];
					b1 = src->y - M->M[2][1];
					y = FskFixDiv(b0 - FskFracMul(f, b1), d);
					x = FskFixDiv(b1 - FskFixMul(y, M->M[1][1]), M->M[0][1]);
					dst->x = x;
					dst->y = y;
				}
			}
			break;
		case 2:
			f = FskFracDiv(M->M[1][1], M->M[1][0]);
			if ((d = M->M[0][1] - FskFracMul(f, M->M[0][0])) != 0) {
				for ( ; numPoints--; src++, dst++) {
					b0 = src->x - M->M[2][0];
					b1 = src->y - M->M[2][1];
					x = FskFixDiv(b1 - FskFracMul(f, b0), d);
					y = FskFixDiv(b0 - FskFixMul(x, M->M[0][0]), M->M[1][0]);
					dst->x = x;
					dst->y = y;
				}
			}
			break;
		case 3:
			f = FskFracDiv(M->M[1][0], M->M[1][1]);
			if ((d = M->M[0][0] - FskFracMul(f, M->M[0][1])) != 0) {
				for ( ; numPoints--; src++, dst++) {
					b0 = src->x - M->M[2][0];
					b1 = src->y - M->M[2][1];
					x = FskFixDiv(b0 - FskFracMul(f, b1), d);
					y = FskFixDiv(b1 - FskFixMul(x, M->M[0][1]), M->M[1][1]);
					dst->x = x;
					dst->y = y;
				}
			}
			break;
	}
#endif

	return d;
}


/********************************************************************************
 * FskSolveFixedMatrix3x2Equation
 *
 *	Solves the equation
 *		b = a M
 *	for a, given the knowns M and b
 * Returning the determinant as the function value.
 ********************************************************************************/

FskFixed
FskSolveFixedMatrix3x2Equation(const FskFixedPoint2D *b, FskFixedPoint2D *a, const FskFixedMatrix3x2 *M)
{
	FskFixed	x0	= b->x - M->M[2][0];
	FskFixed	y0	= b->y - M->M[2][1];

#ifdef ACCURATE_SOLUTION
	FskInt64	det	= (FskInt64)(M->M[0][0]) * M->M[1][1] - (FskInt64)(M->M[0][1]) * M->M[1][0];

	if (det != 0) {
		a->x = FskFixNDiv64((FskInt64)(M->M[1][1]) * x0 - (FskInt64)(M->M[1][0]) * y0, det);
		a->y = FskFixNDiv64((FskInt64)(M->M[0][0]) * y0 - (FskInt64)(M->M[0][1]) * x0, det);
	}

	return (FskFixed)(det >> 16);
#else /* FAST_SOLUTION */
	FskFixed	det;

	a->x = FskFixedNLinear2D(M->M[1][1],         x0, -M->M[1][0],         y0, 16);	/* 64-bit accumulations are accurate */
	a->y = FskFixedNLinear2D(M->M[0][0],         y0, -M->M[0][1],         x0, 16);
	det  = FskFixedNLinear2D(M->M[0][0], M->M[1][1], -M->M[0][1], M->M[1][0], 16);
	if (det != 0) {
		a->x = FskFixDiv(a->x, det);
		a->y = FskFixDiv(a->y, det);
	}

	return det;
#endif /* FAST_SOLUTION */
}
