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
 	INCLUDE kinoma_arm_asm_utilities-s.h
  
   
    IMPORT SwapMemoryAndRemovePreventingBytes_misaligned_c

	AREA  |.text|, CODE, READONLY

	EXPORT  |SwapMemoryAndRemovePreventingBytes_aligned4_arm_v6|

;register input
dst			RN 0	
src			RN 1
src_size	RN 2

;***bnie: get rid of this later  3/2/2009
dst_size			RN 3
dst_size_in_out		RN 3
dst_size_stack_add  RN 10
;***bnie		
			
dst0		RN 9
			
RA			RN 4
RZ			RN 10

|SwapMemoryAndRemovePreventingBytes_aligned4_arm_v6| PROC

	stmdb   sp!, {RA-RZ, lr} 
	
	mov		dst0, dst			;save original dst to get progress later
	
	;***bnie: later we want to get rid of the need to return dst_size from stack at all
	; so temporarily save the stack address to r10
	mov		dst_size_stack_add,  dst_size_in_out
	;***
	
count		RN 4
tmp			RN 5
bbb1		RN 6
bbb2		RN 7
bbb3		RN 8

	mov		count, src_size, lsr #2
	sub  	count, count, #1

	ldr		bbb1, [src], #4
	ldr		bbb2, [src], #4
	pld		[src, #16]
	pld		[src, #32]
	pld		[dst, #16]
	pld		[dst, #32]
	rev		bbb1, bbb1
	
	;;;;;;;;;;;;;;;;;;;;;;;
	;major fast path start
	;;;;;;;;;;;;;;;;;;;;;;;
copy_loop
	mov		tmp, bbb1, lsr #8
	cmp		tmp, #0x00000003
	movne	tmp, bbb1, lsl #8
	cmpne	tmp, #0x00000300
	beq		generic_case

	rev		bbb2, bbb2
	mov		bbb3, bbb1, lsl #16
	orr		bbb3, bbb3, bbb2, lsr #16
	
	mov		tmp, bbb3, lsr #8
	cmp		tmp, #0x00000003
	movne	tmp, bbb3, lsl #8
	cmpne	tmp, #0x00000300
	beq		generic_case

	str		bbb1, [dst], #4
	mov		bbb1, bbb2						;next
	subs	count, count, #1
	ldrne	bbb2, [src], #4
	pld		[src, #16]
	pld		[src, #32]
	pld		[dst, #16]
	pld		[dst, #32]
	bne	    copy_loop

	;;;;;;;;;;;;;;;;;;;;;;;
	;major fast path end
	;;;;;;;;;;;;;;;;;;;;;;;
	
	;take care of leftover (less than 4 bytes)
	mov		dst_size, src_size, lsr #2
	mov		dst_size, dst_size, lsl #2
	
left_bytes	RN 4	;count	
ddd3		RN 5	;tmp
;bbb1		RN 6
ddd1		RN 7	;bbb2			
ddd2		RN 8	;bbb3

	and		left_bytes, src_size, #3
	cmp		left_bytes, #3
	ldrgeb	ddd1, [src, #2]
	movlt	ddd1, #0
	cmp		left_bytes, #2
	ldrgeb	ddd2, [src, #1]
	movlt	ddd2, #0
	cmp		left_bytes, #1
	ldrgeb	ddd3, [src, #0]
	movlt	ddd3, #0
	mov		bbb2, ddd1, lsl #8
	orr		bbb2, bbb2, ddd2, lsl #16
	orr		bbb2, bbb2, ddd3, lsl #24

mark_1		RN 2	;src_size

	mov		tmp, bbb1, lsr #8
	cmp		tmp, #0x00000003
	moveq	mark_1, #1
	movne	mark_1, #0

	mov		tmp, bbb1, lsl #8
	cmp		tmp, #0x00000300
	moveq	mark_1, #2

	cmp		mark_1, #0
	streq	bbb1, [dst], #4		
	
	cmp		left_bytes, #0						;left_bytes check 
	streq	dst_size, [dst_size_stack_add]		;***bnie: get rid of it later
	ldmeqia sp!, {RA-RZ, pc}	;return
	
	
	mov		bbb1, bbb1, lsl #16
	orr		bbb1, bbb1, bbb2, lsr #16
	
mark_2		RN 4	;left_bytes<=count

	mov		tmp, bbb1, lsr #8
	cmp		tmp, #0x00000003
	moveq	mark_2, #1
	movne	mark_2, #0

	mov		tmp, bbb1, lsl #8
	cmp		tmp, #0x00000300
	moveq	mark_2, #2

	adds	tmp, mark_1, mark_2
	streq	bbb2, [dst], #4
	addeq	dst_size, dst_size, #4
	streq	dst_size, [dst_size_stack_add]		;***bnie: get rid of it later
	ldmeqia sp!, {RA-RZ, pc}					;return

	add		src, src, #4
	cmp		mark_1, #0
	subeq	dst, dst, #4
	
generic_case

progress	RN 5	;tmp
	sub		progress, dst, dst0
	sub		src, src, #8				;step back 2 long
	sub		src_size, src_size, progress
	mov		dst_size, dst_size_stack_add
	bl		SwapMemoryAndRemovePreventingBytes_misaligned_c
	ldr		dst_size, [dst_size_stack_add]
	add		dst_size, dst_size, progress
	streq	dst_size, [dst_size_stack_add]		;***bnie: get rid of it later
	ldmia   sp!, {RA-RZ, pc} 

	ENDP  ; |ippiDecodeExpGolombOne_H264_1u16s_arm|

  END


