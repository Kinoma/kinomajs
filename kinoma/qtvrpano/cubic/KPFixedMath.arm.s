;;
;;     Copyright (C) 2010-2015 Marvell International Ltd.
;;     Copyright (C) 2002-2010 Kinoma, Inc.
;;
;;     Licensed under the Apache License, Version 2.0 (the "License");
;;     you may not use this file except in compliance with the License.
;;     You may obtain a copy of the License at
;;
;;       http://www.apache.org/licenses/LICENSE-2.0
;;
;;     Unless required by applicable law or agreed to in writing, software
;;     distributed under the License is distributed on an "AS IS" BASIS,
;;     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
;;     See the License for the specific language governing permissions and
;;     limitations under the License.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


	AREA |.text|, CODE, READONLY	; name this block of code
	ENTRY							; mark first instruction


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; KPFixed KPFixMul(r0 = KPFixed a, r1 = KPFixed b)
; rounded, saturated result in r0
; destroys r0, r1, r2
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	export	KPFixMul
KPFixMul
	smull   r0, r2, r1, r0			; 32 x 32 --> 64: r0 * r1 --> r2:r1

	adds	r0, r0, #0x8000			; Round LS word
	adc		r2, r2, #0				; Propagate carry round into MS word

	mov		r0, r0, lsr #16			; Gather <47:16> of product into result
	orr		r0, r0, r2, lsl #16

	mov		r2, r2, asr #16			; Get the MS bits of the full product
	cmp		r2, r0, asr #31			; ... and compare with the sign of the result
	moveq	pc, lr					; Return if there is no overflow

	mvn		r0, r2, asr #31			; 0x00000000 negative, 0xFFFFFFFF positive
	eor		r0, r0, #0x80000000		; 0x80000000 negative, 0x7FFFFFFF positive
	mov		pc, lr					; Return saturated value


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; KPFract KPFracMul(r0 = KPFract a, r1 = KPFract b)
; rounded, saturated result in r0
; destroys r0, r1, r2
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	export	KPFracMul
KPFracMul
	smull   r0, r2, r1, r0			; 32 x 32 --> 64: r0 * r1 --> r2:r1

	adds	r0, r0, #0x20000000		; Round LS word
	adc		r2, r2, #0				; Propagate carry round into MS word

	mov		r0, r0, lsr #30			; Gather <61:30> of product into result
	orr		r0, r0, r2, lsl #2

	mov		r2, r2, asr #30			; Get the MS bits of the full product
	cmp		r2, r0, asr #31			; ... and compare with the sign of the result
	moveq	pc, lr					; Return if there is no overflow

	mvn		r0, r2, asr #31			; 0x00000000 negative, 0xFFFFFFFF positive
	eor		r0, r0, #0x80000000		; 0x80000000 negative, 0x7FFFFFFF positive
	mov		pc, lr					; Return saturated value


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; KPFixed KPFixedNMul(r0 = KPFixed a, r1 = KPFixed b, r2 = SInt32 n)
; rounded, saturated result in r0
; destroys r0, r1, r2, r3
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	export	KPFixedNMul
KPFixedNMul
	smull	r0, r3, r1, r0			; 32 x 32 --> 64: r0 * r1 --> r3:r0

	cmp		r2, #31					; Take care of shifts greater than 31
	subgt	r2, r2, #31				; Greater than 31: subtract 31 conditionally
	movgt	r0, r0, lsr #31			; Shift right by 31 conditionally
	orrgt	r0, r0, r3, lsl #1
	movgt	r3, r3, asr #31

	sub		r2, r2, #1				; n = n - 1, temporarily
	mov		r1, #1
	adds	r0, r0, r1, lsl r2		; Round LS word
	adc		r3, r3, #0				; Propagate rounding carry to MS word
	add		r2, r2, #1				; restore r2

	mov		r1, r3, asr r2			; Save MS word of product for overflow testing

	mov		r0, r0, lsr r2			; Select bits <n+31:n> for product
	rsb		r2, r2, #32
	orr		r0, r0, r3, lsl r2

	cmp		r1, r0, asr #31			; Compare upper part of product with sign
	moveq	pc, lr					; Return if there is no overflow

	mvn		r0, r1, asr #31			; 0x00000000 negative, 0xFFFFFFFF positive
	eor		r0, r0, #0x80000000		; 0x80000000 negative, 0x7FFFFFFF positive
	mov		pc, lr					; Return saturated value in r0


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; KPFixed KPFixDiv(r0 = KPFixed num, r1 = KPFixed den)
; rounded, saturated result in r0
; destroys r0, r1, r2, r3, r12
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	export KPFixDiv
KPFixDiv
	mov		r2, #16
	b		KPFixedNDiv


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; KPFract KPFracDiv(r0 = KPFract num, r1 = KPFract den)
; rounded, saturated result in r0
; destroys r0, r1, r2, r3, r12
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	export KPFracDiv
KPFracDiv
	mov		r2, #30
	b		KPFixedNDiv


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; KPFixed KPFixedNDiv(r0 = KPFixed num, r1 = KPFixed den, r2 = SInt32 count)
; rounded, saturated result in r0
; destroys r0, r1, r2, r3, r12
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	export	KPFixedNDiv
KPFixedNDiv
	cmp		r0, #0					; If the numerator is 0 ...
	moveq	pc, lr					; ... Return 0

	eor		r12, r0, r1				; Compute the sign of the result
	rsbmi	r0, r0, #0				; Absolute value of the numerator
	mov		r3, r2					; Move n to r3
	movs	r2, r1					; Move denominator to r2, freeing r1
	beq		Lsaturate				; Saturate if denominator is 0
	rsbmi	r2, r2, #0				; Absolute value of the denominator

	cmp		r3, #32
	blt		LsmallNumeratorShift
	rsb		r3, r3, #63				; n <-- 63 - n
	movs	r1, r0, lsr r3			; r1 <-- num >> (63 - original n)
	bne		Lsaturate
	rsb		r3, r3, #63				; n <-- original value of n
	sub		r1, r3, #32				; r1 <-- n - 32
	blt		LsmallNumeratorShift
	mov		r1, r0, lsl r1			; r1 = high bits of shifted numerator
	mov		r0, #0					; r0 =  low bits of shifted numerator
	b		LnumeratorShiftDone
LsmallNumeratorShift
	rsb		r1, r3, #32				; r1 = 32 - n
	mov		r1, r0, lsr r1			; r1 = high bits of shifted numerator
	mov		r0, r0, lsl r3			; r0 =  low bits of shifted numerator
LnumeratorShiftDone					; Done shifting: r3 is now free!

	adds	r0, r0, r2, lsr #1		; lo numerator += denominator / 2, for rounding
	adc		r1, r1, #0				; hi numerator " "

	mov		r3, lr					; Save return address
	bl		ArmDivU64U32			; Call saturating, rounding UInt64 / UInt32
	mov		lr, r3					; Restore return address

Ensign								; This label is used by KPFixedNRatio, too
	cmp		r0, #0x80000000			; Is unsigned quotient < 2147483648?
	bhs		Lsaturate				; No: saturate
	cmp		r12, #0					; Check sign
	rsblt	r0, r0, #0				; Negate if sign should be negative
	mov		pc, lr					; Return

Lsaturate
	mvn		r0, r12, asr #31		; FFFFFFFF positive, 00000000 negative, if denominator is 0
	eor		r0, r0, #0x80000000		; 7FFFFFFF positive, 80000000 negative, if denominator is 0
	mov		pc, lr					; Return +/- "infinity", if denominator is 0


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; KPFixed KPFixedNRatio(r0 = KPFixed r0, r1 = KPFixed b, r2 = KPFixed den, r3 = SInt32 n)
; rounded, saturated result in r0
; destroys r0, r1, r2, r3, r12
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	export	KPFixedNRatio
KPFixedNRatio
	cmp		r0, #0					; Check for a zero numerator ...
	moveq	pc, lr
	cmp		r1, #0
	moveq	r0, #0
	moveq	pc, lr					; ... and return zero if the numerator is zero

	eor		r12, r0, r1				; Compute the sign of the numerator
	cmp		r2, #0					; Check for zero denominator ...
	beq		Lsaturate				; ... and saturate to the same sign of the numerator

	mov		r12, sp					; Save registers on the stack
	stmdb	sp!, {r4, r12, lr}
	sub		sp, sp, #12

	smull   r0, r12, r1, r0			; 32 x 32 --> 64: r0 * r1 --> r1:r0
	mov		r1, r12

	eor		r12, r1, r2				; Compute the sign of the ratio
	cmp		r2, #0
	rsblt	r2, r2, #0				; Absolute value of denominator
	cmp		r1, #0
	bge		LpositiveNumerator
	rsbs	r0, r0, #0				; Absolute value of numerator
	rsc		r1, r1, #0
LpositiveNumerator

	cmp		r3, #0					; Positive or negative shift?
	blt		LrightShift

	; Positive shift
	mov		r4, r1					; Get the MS word of the numerator
	eor		r4, r4, r4, lsr #1		; Compute something to help determine the max shift
	clz		r4, r4					; Determine the maximum amount that we can upshift the numerator (version 5 and above)
	cmp		r3, r4					; Does the specified shift exceed this amount?
	bge		LNumUpDenDown

	; There is enough room to upshift the numerator without overflowing
	mov		r1, r1, lsl r3			; Shift numerator left n bits
	rsb		r3, r3, #32
	orr		r1, r1, r0, lsr r3
	rsb		r3, r3, #32
	mov		r0, r0, lsl r3
	b		LRatioShiftDone

	; There is not enough room to upshift the numerator without overflowing,
	; so we upshift the numerator as much as we can, and downshift the denominator the rest
LNumUpDenDown
	sub		r4, r4, #1				; The exact maximum we can upshift the numerator
	sub		r3, r3, r4				; The remainder, we will downshift the denominator
	mov		r1, r1, lsl r4			; Shift numerator left r4 bits
	rsb		r4, r4, #32
	orr		r1, r1, r0, lsr r4
	rsb		r4, r4, #32
	mov		r0, r0, lsl r4
	mov		r2, r2, lsr r3			; Downshift the denominator
	b		LRatioShiftDone

	; Negative shift
LrightShift
	rsb		r3, r3, #0				; Convert the negative left shift to a positive right shift
	mov		r4, r2					; Get denominator
	eor		r4, r4, r4, lsr #1		; Compute something to help determine the max shift
	clz		r4, r4					; Determine the maximum amount that we can upshift the denominator (version 5 and above)
	cmp		r3, r4					; Does the specified shift exceed this amount?
	bge		LDenUpNumDown

	; There is enough room to upshift the denominator all the way
	mov		r2, r2, lsl r3			; Shift the denominator up
	b		LRatioShiftDone

	; There is not enough room to upshift the denominator all the way
LDenUpNumDown
	cmp		r4, #0					; Assure that we don't get a negative shift
	subgt	r4, r4, #1				; The exact maximum we can upshift the denominator
	sub		r3, r3, r4				; The remainder we downshift the numerator
	mov		r2, r2, lsl r4			; Shift the denominator left
	mov		r0, r0, lsr r3			; Shift numerator right by r3 bits
	rsb		r3, r3, #32
	orr		r0, r0, r1, lsl r3
	rsb		r3, r3, #32
	mov		r1, r1 lsr r3
LRatioShiftDone

	adds	r0, r0, r2, lsr #1		; Bias numerator by half the divisor to effect rounding
	adc		r1, r1, #0
	bl		ArmDivU64U32

	add		sp, sp, #12				; restore registers
	ldmia	sp, {r4, sp, lr}

	b		Ensign


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; UInt32 ArmDivU64U32(r0:r1 = UInt64 num, r2 = UInt32 den)
; UInt32 ArmDivU64U32(r0 = UInt32 numLo, r1 = UInt32 numHi, r2 = UInt32 den)
; rounded, saturated result in r0
; destroys r0, r1, r2
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	export ArmDivU64U32
ArmDivU64U32
	cmp		r1, r2					; See if there will be overflow
	mvnhs	r0, #0					; Saturate to 0xFFFFFFFF instead of overflowing
	movhs	pc, lr					; Return the saturated quotient instead of overflowing

	rsb		r2, r2, #0				; negate divisor to simplify computations below

    adds	r0, r0, r0				; next bit of numerator/remainder in C
    adcs	r1, r2, r1, lsl #1		; rem = 2*rem - divisor
    rsbcc	r1, r2, r1				; if it failed add divisor back on
    adcs	r0, r0, r0				; insert answer bit and get numerator bit
	adcs	r1, r2, r1, lsl #1		; 1: rem = 2*rem - divisor
	rsbcc	r1, r2, r1				; 1: if it failed add divisor back on
	adcs	r0, r0, r0				; 1: insert answer bit and get numerator bit
	adcs	r1, r2, r1, lsl #1		; 2: rem = 2*rem - divisor
	rsbcc	r1, r2, r1				; 2: if it failed add divisor back on
	adcs	r0, r0, r0				; 2: insert answer bit and get numerator bit
	adcs	r1, r2, r1, lsl #1		; 3: rem = 2*rem - divisor
	rsbcc	r1, r2, r1				; 3: if it failed add divisor back on
	adcs	r0, r0, r0				; 3: insert answer bit and get numerator bit
	adcs	r1, r2, r1, lsl #1		; 4: rem = 2*rem - divisor
	rsbcc	r1, r2, r1				; 4: if it failed add divisor back on
	adcs	r0, r0, r0				; 4: insert answer bit and get numerator bit
	adcs	r1, r2, r1, lsl #1		; 5: rem = 2*rem - divisor
	rsbcc	r1, r2, r1				; 5: if it failed add divisor back on
	adcs	r0, r0, r0				; 5: insert answer bit and get numerator bit
	adcs	r1, r2, r1, lsl #1		; 6: rem = 2*rem - divisor
	rsbcc	r1, r2, r1				; 6: if it failed add divisor back on
	adcs	r0, r0, r0				; 6: insert answer bit and get numerator bit
	adcs	r1, r2, r1, lsl #1		; 7: rem = 2*rem - divisor
	rsbcc	r1, r2, r1				; 7: if it failed add divisor back on
	adcs	r0, r0, r0				; 7: insert answer bit and get numerator bit
	adcs	r1, r2, r1, lsl #1		; 8: rem = 2*rem - divisor
	rsbcc	r1, r2, r1				; 8: if it failed add divisor back on
	adcs	r0, r0, r0				; 8: insert answer bit and get numerator bit
	adcs	r1, r2, r1, lsl #1		; 9: rem = 2*rem - divisor
	rsbcc	r1, r2, r1				; 9: if it failed add divisor back on
	adcs	r0, r0, r0				; 9: insert answer bit and get numerator bit
	adcs	r1, r2, r1, lsl #1		; 10: rem = 2*rem - divisor
	rsbcc	r1, r2, r1				; 10: if it failed add divisor back on
	adcs	r0, r0, r0				; 10: insert answer bit and get numerator bit
	adcs	r1, r2, r1, lsl #1		; 11: rem = 2*rem - divisor
	rsbcc	r1, r2, r1				; 11: if it failed add divisor back on
	adcs	r0, r0, r0				; 11: insert answer bit and get numerator bit
	adcs	r1, r2, r1, lsl #1		; 12: rem = 2*rem - divisor
	rsbcc	r1, r2, r1				; 12: if it failed add divisor back on
	adcs	r0, r0, r0				; 12: insert answer bit and get numerator bit
	adcs	r1, r2, r1, lsl #1		; 13: rem = 2*rem - divisor
	rsbcc	r1, r2, r1				; 13: if it failed add divisor back on
	adcs	r0, r0, r0				; 13: insert answer bit and get numerator bit
	adcs	r1, r2, r1, lsl #1		; 14: rem = 2*rem - divisor
	rsbcc	r1, r2, r1				; 14: if it failed add divisor back on
	adcs	r0, r0, r0				; 14: insert answer bit and get numerator bit
	adcs	r1, r2, r1, lsl #1		; 15: rem = 2*rem - divisor
	rsbcc	r1, r2, r1				; 15: if it failed add divisor back on
	adcs	r0, r0, r0				; 15: insert answer bit and get numerator bit
	adcs	r1, r2, r1, lsl #1		; 16: rem = 2*rem - divisor
	rsbcc	r1, r2, r1				; 16: if it failed add divisor back on
	adcs	r0, r0, r0				; 16: insert answer bit and get numerator bit
	adcs	r1, r2, r1, lsl #1		; 17: rem = 2*rem - divisor
	rsbcc	r1, r2, r1				; 17: if it failed add divisor back on
	adcs	r0, r0, r0				; 17: insert answer bit and get numerator bit
	adcs	r1, r2, r1, lsl #1		; 18: rem = 2*rem - divisor
	rsbcc	r1, r2, r1				; 18: if it failed add divisor back on
	adcs	r0, r0, r0				; 18: insert answer bit and get numerator bit
	adcs	r1, r2, r1, lsl #1		; 19: rem = 2*rem - divisor
	rsbcc	r1, r2, r1				; 19: if it failed add divisor back on
	adcs	r0, r0, r0				; 19: insert answer bit and get numerator bit
	adcs	r1, r2, r1, lsl #1		; 20: rem = 2*rem - divisor
	rsbcc	r1, r2, r1				; 20: if it failed add divisor back on
	adcs	r0, r0, r0				; 20: insert answer bit and get numerator bit
	adcs	r1, r2, r1, lsl #1		; 21: rem = 2*rem - divisor
	rsbcc	r1, r2, r1				; 21: if it failed add divisor back on
	adcs	r0, r0, r0				; 21: insert answer bit and get numerator bit
	adcs	r1, r2, r1, lsl #1		; 22: rem = 2*rem - divisor
	rsbcc	r1, r2, r1				; 22: if it failed add divisor back on
	adcs	r0, r0, r0				; 22: insert answer bit and get numerator bit
	adcs	r1, r2, r1, lsl #1		; 23: rem = 2*rem - divisor
	rsbcc	r1, r2, r1				; 23: if it failed add divisor back on
	adcs	r0, r0, r0				; 23: insert answer bit and get numerator bit
	adcs	r1, r2, r1, lsl #1		; 24: rem = 2*rem - divisor
	rsbcc	r1, r2, r1				; 24: if it failed add divisor back on
	adcs	r0, r0, r0				; 24: insert answer bit and get numerator bit
	adcs	r1, r2, r1, lsl #1		; 25: rem = 2*rem - divisor
	rsbcc	r1, r2, r1				; 25: if it failed add divisor back on
	adcs	r0, r0, r0				; 25: insert answer bit and get numerator bit
	adcs	r1, r2, r1, lsl #1		; 26: rem = 2*rem - divisor
	rsbcc	r1, r2, r1				; 26: if it failed add divisor back on
	adcs	r0, r0, r0				; 26: insert answer bit and get numerator bit
	adcs	r1, r2, r1, lsl #1		; 27: rem = 2*rem - divisor
	rsbcc	r1, r2, r1				; 27: if it failed add divisor back on
	adcs	r0, r0, r0				; 27: insert answer bit and get numerator bit
	adcs	r1, r2, r1, lsl #1		; 28: rem = 2*rem - divisor
	rsbcc	r1, r2, r1				; 28: if it failed add divisor back on
	adcs	r0, r0, r0				; 28: insert answer bit and get numerator bit
	adcs	r1, r2, r1, lsl #1		; 29: rem = 2*rem - divisor
	rsbcc	r1, r2, r1				; 29: if it failed add divisor back on
	adcs	r0, r0, r0				; 29: insert answer bit and get numerator bit
	adcs	r1, r2, r1, lsl #1		; 30: rem = 2*rem - divisor
	rsbcc	r1, r2, r1				; 30: if it failed add divisor back on
	adcs	r0, r0, r0				; 30: insert answer bit and get numerator bit
	adcs	r1, r2, r1, lsl #1		; 31: rem = 2*rem - divisor
	rsbcc	r1, r2, r1				; 31 if it failed add divisor back on
	adcs	r0, r0, r0				; 31: insert answer bit and get numerator bit

	; The result is rounded

	mov		pc, lr					; Return

	END								; Mark end of file
