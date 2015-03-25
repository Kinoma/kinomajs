;;
;;     Copyright (C) 2010-2015 Marvell International Ltd.
;;     Copyright (C) 2002-2010 Kinoma, Inc.
;;
;;     Licensed under the Apache License, Version 2.0 (the "License");
;;     you may not use this file except in compliance with the License.
;;     You may obtain a copy of the License at
;;
;;      http://www.apache.org/licenses/LICENSE-2.0
;;
;;     Unless required by applicable law or agreed to in writing, software
;;     distributed under the License is distributed on an "AS IS" BASIS,
;;     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
;;     See the License for the specific language governing permissions and
;;     limitations under the License.
;;

		AREA	|.text|, CODE, READONLY

		EXPORT  ippsZero_8u_arm

ippsZero_8u_arm PROC
		mov		r2, r1
		mov		r1, r0
		mov		r0, #0
		b		doset_8

		ENDP  ; ippsZero_8u_arm

		EXPORT  ippsZero_16u_arm 

ippsZero_16u_arm PROC
		mov		r2, r1, lsl #1
		mov		r1, r0
		mov		r0, #0
		b		doset_8

		ENDP  ; ippsZero_16u_arm

		EXPORT  ippsZero_32u_arm 

ippsZero_32u_arm PROC
		mov		r2, r1, lsl #2
		mov		r1, r0
		mov		r0, #0
		b		doset_32

		ENDP  ; ippsZero_32u_arm

		EXPORT  ippsSet_8u_arm

ippsSet_8u_arm PROC

value	RN 0
dst		RN 1
count	RN 2
scratch RN 3
temp	RN r12

		orr		value, value, value, lsl #8		; smear value across low word
		orr		value, value, value, lsl #16	; smear value across entire long

doset_8
		cmp		count, #4
		blt		tail							; less than 4 bytes

		and		scratch, dst, #3				; deal with misaligned destination
		subs	count, count, scratch			; decrement count by misalignment
		rsb		scratch, scratch, #3
		add     temp, pc, #0 
		add		pc, temp, scratch, lsl #2
		strb	value, [dst], #1
		strb	value, [dst], #1
		strb	value, [dst], #1
		cmp		count, #4
		blt		tail							; less than 4 bytes left, handle it in the tail

doset_32
		ands	scratch, count, #0x1c			; determine number of longs to copy to get 32 byte aligned
		beq		loop							; already aligned, so go
		sub		count, count, scratch			; subtract out bytes copied to get 32 byte aligned
		rsb		scratch, scratch, #32			; 32 - that
		add     temp, pc, #4					; skip over the first loop instruction (count -= 32)
		add		pc, temp, scratch				; go get aligned
loop
		sub		count, count, #32				; subtract 32
		str		value, [dst], #4
		str		value, [dst], #4
		str		value, [dst], #4
		str		value, [dst], #4
		str		value, [dst], #4
		str		value, [dst], #4
		str		value, [dst], #4
		cmp		count, #32
		str		value, [dst], #4

		bge		loop

tail											; count is 0, 1, 2, or 3
		rsb		scratch, count, #3				; scratch = 3 - count -> 3, 2, 1, 0
		add     temp, pc, #0
		add		pc, temp, scratch, lsl #2
		strb	value, [dst], #1
		strb	value, [dst], #1
		strb	value, [dst], #1
		mov		pc, lr							; done

		ENDP  ; ippsSet_8u_arm



		EXPORT  |PLD_arm|

|PLD_arm| PROC

		pld		[r0]
		mov		pc, lr							; done		

		ENDP  ; PLD_arm

		EXPORT  |CLZ_arm|

|CLZ_arm| PROC

		clz		r0, r0
		mov		pc, lr							; done		

		ENDP  ; CLZ_arm

		END
