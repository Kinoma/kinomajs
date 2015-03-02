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
	AREA  |.text|, CODE, READONLY
	
	EXPORT  |ippiSAD16x16_8u32s_aligned_4_arm_v6|		;ref has rowbytes 16
	EXPORT  |ippiSAD16x16_8u32s_misaligned_arm_v6|
	EXPORT  |ippiSAD4x4_8u32s_arm_v6|
	EXPORT  |ippiSAD8x8_8u32s_arm_v6|
	EXPORT  |ippiMeanAbsDev16x16_8u32s_C1R_arm_v6|
	EXPORT  |ippiSubSAD8x8_8u16s_C1R_16x16_arm_v6|	
	EXPORT  |ippiAdd8x8_16s8u_C1IRS_arm_v6|	
	
	

pSrc			RN 0	
srcStep			RN 1
pRef			RN 2
refStep			RN 3

ss1				RN 4
ss2				RN 5 
rr1				RN 6
rr2				RN 7
rrrr			RN 8
tm1				RN 9 
tm2				RN 10
tm3				RN 11

sum				RN 12
height			RN 14

RA				RN 4		;first register to push into stack at entry
RZ				RN 11		;second last, and then it is lr=>pc

REGIS_SHIFT_16x16_SAD	EQU	(9*4)											;//registers pushed into stack to preserve caller state: 6 here as example
LOCAL_SHIFT_16x16_SAD	EQU	(0*0)											;//local stack variables: 2 here as example
SP_SHIFT_16x16_SAD		EQU	(REGIS_SHIFT_16x16_SAD + LOCAL_SHIFT_16x16_SAD)	;//total up shift from stack input

pSAD_SHIFT_16x16_SAD 	EQU (0*4 + SP_SHIFT_16x16_SAD)						;input from stack
mcType_SHIFT_16x16_SAD 	EQU (1*4 + SP_SHIFT_16x16_SAD)



;;;;;;;;;;;;;;;;;;;;;;;;;;;;		
	MACRO
		PACK_4_Step1 $dst, $ppp, $tm1, $tm2, $tm3
;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	
	ldrb	$dst, [$ppp], #1
	ldrb	$tm1, [$ppp], #1 
	ldrb	$tm2, [$ppp], #1 
	ldrb	$tm3, [$ppp], #1

	MEND

;;;;;;;;;;;;;;;;;;;;;;;;;;;;		
	MACRO
		PACK_4_Step2 $dst, $tm1, $tm2, $tm3
;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	
	orr		$dst, $dst, $tm1, lsl #8
	orr		$dst, $dst, $tm2, lsl #16
	orr		$dst, $dst, $tm3, lsl #24

	MEND

;;;;;;;;;;;;;;;;;;;;;;;;;;;;		
	MACRO
		SAD_16x16
;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	stmdb   sp!, {RA-RZ, lr}								;//!!!MUST MATCH!!! REGIS_SHIFT_id, preserve 6 registers here as example
	;sub     sp, sp, #LOCAL_SHIFT_16x16_SAD					;//expend stack for local stack variables 	
 	
	mov		sum,    #0
	mov		height, #16
 	sub		refStep, refStep, #15
 	sub		srcStep, srcStep, #12
	
mark_16x16_SAD
	
 	pld	 [pSrc,srcStep]
 	pld	 [pRef,refStep]
 	
	ldrb	rr2, [pRef], #1
	ldrb	tm1, [pRef], #1 
	ldrb	tm2, [pRef], #1 
	ldrb	tm3, [pRef], #1
	ldrb	rr1, [pRef], #1
	ldr		ss2, [pSrc], #4
	ldr		ss1, [pSrc], #4 
	orr		rr2, rr2, tm1, lsl #8
	ldrb	tm1, [pRef], #1 
	orr		rr2, rr2, tm2, lsl #16
	ldrb	tm2, [pRef], #1 
	orr		rr2, rr2, tm3, lsl #24
	ldrb	tm3, [pRef], #1

	usada8	sum, ss2, rr2, sum
	
	orr		rr1, rr1, tm1, lsl #8
	ldr		ss2, [pSrc], #4 
	ldrb	rr2, [pRef], #1
	ldrb	tm1, [pRef], #1 
	orr		rr1, rr1, tm2, lsl #16
	ldrb	tm2, [pRef], #1 
	orr		rr1, rr1, tm3, lsl #24
	ldrb	tm3, [pRef], #1
	usada8	sum, ss1, rr1, sum
	
	orr		rr2, rr2, tm1, lsl #8
	ldr		ss1, [pSrc], srcStep
	ldrb	rr1, [pRef], #1
	ldrb	tm1, [pRef], #1 
	orr		rr2, rr2, tm2, lsl #16
	ldrb	tm2, [pRef], #1 
	orr		rr2, rr2, tm3, lsl #24
	ldrb	tm3, [pRef], refStep
	usada8	sum, ss2, rr2, sum
	
	orr		rr1, rr1, tm1, lsl #8
	orr		rr1, rr1, tm2, lsl #16
	orr		rr1, rr1, tm3, lsl #24
	usada8	sum, ss1, rr1, sum
	 	
 	subs height, height, #1
	bne	 mark_16x16_SAD	
	
	ldr		tm1, [sp, #pSAD_SHIFT_16x16_SAD]
	str		sum, [tm1]

    ;add    sp,sp,#LOCAL_SHIFT_16x16_SAD
    ldmfd  sp!,{RA-RZ,pc} 

	MEND


|ippiSAD16x16_8u32s_misaligned_arm_v6| PROC
	SAD_16x16
  ENDP	;ippiSAD16x16_8u32s_misaligned_arm_v6






pSrc			RN 0	
srcStep			RN 1
pRef			RN 2
pSAD			RN 3

ss1				RN 4
ss2				RN 5 
ss3				RN 6
ss4				RN 7
rr1				RN 8
rr2				RN 9 
rr3				RN 10
rr4				RN 11

sum				RN 12
height			RN 14

RA				RN 4		;first register to push into stack at entry
RZ				RN 11		;second last, and then it is lr=>pc

REGIS_SHIFT_16x16_al4_SAD	EQU	(9*4)													;//registers pushed into stack to preserve caller state: 6 here as example
LOCAL_SHIFT_16x16_al4_SAD	EQU	(0*0)													;//local stack variables: 2 here as example
SP_SHIFT_16x16_al4_SAD		EQU	(REGIS_SHIFT_16x16_al4_SAD + LOCAL_SHIFT_16x16_al4_SAD)	;//total up shift from stack input

pSAD_SHIFT_16x16_al4_SAD 	EQU (0*4 + SP_SHIFT_16x16_al4_SAD)						;input from stack
mcType_SHIFT_16x16_al4_SAD 	EQU (1*4 + SP_SHIFT_16x16_al4_SAD)


|ippiSAD16x16_8u32s_aligned_4_arm_v6| PROC

	stmdb   sp!, {RA-RZ, lr}								;//!!!MUST MATCH!!! REGIS_SHIFT_id, preserve 6 registers here as example
	;sub     sp, sp, #LOCAL_SHIFT_16x16_al4_SAD					;//expend stack for local stack variables 	
 	
	mov		sum,    #0
	mov		height, #16
 	sub		srcStep, srcStep, #12
	
mark_16x16_al4_SAD
	
 	pld	 [pSrc,srcStep]
 	
	ldr		rr1, [pRef], #4
	ldr		rr2, [pRef], #4
	ldr		rr3, [pRef], #4
	ldr		rr4, [pRef], #4
	ldr		ss1, [pSrc], #4
	ldr		ss2, [pSrc], #4 
	ldr		ss3, [pSrc], #4
	ldr		ss4, [pSrc], srcStep 

	usada8	sum, ss1, rr1, sum
	usada8	sum, ss2, rr2, sum
	usada8	sum, ss3, rr3, sum	
	usada8	sum, ss4, rr4, sum
	 	
 	subs height, height, #1
	bne	 mark_16x16_al4_SAD	
	
	str		sum, [pSAD]

    ldmfd  sp!,{RA-RZ,pc} 

  ENDP	;ippiSAD16x16_8u32s_aligned_4_arm_v6




pSrc			RN 0	
srcStep			RN 1
pRef			RN 2
refStep			RN 3

ss1				RN 4
ss2				RN 5 
rr1				RN 6
rr2				RN 7
rrrr			RN 8
tm1				RN 9 
tm2				RN 10
tm3				RN 11

sum				RN 12
height			RN 14

RA				RN 4		;first register to push into stack at entry
RZ				RN 11		;second last, and then it is lr=>pc

REGIS_SHIFT_8x8_SAD	EQU	(9*4)											;//registers pushed into stack to preserve caller state: 6 here as example
LOCAL_SHIFT_8x8_SAD	EQU	(0*0)											;//local stack variables: 2 here as example
SP_SHIFT_8x8_SAD		EQU	(REGIS_SHIFT_8x8_SAD + LOCAL_SHIFT_8x8_SAD)	;//total up shift from stack input

pSAD_SHIFT_8x8_SAD 	EQU (0*4 + SP_SHIFT_8x8_SAD)						;input from stack
mcType_SHIFT_8x8_SAD 	EQU (1*4 + SP_SHIFT_8x8_SAD)



;;;;;;;;;;;;;;;;;;;;;;;;;;;;		
	MACRO
		PACK_44_Step1 $dst, $ppp, $nnn, $tm1, $tm2, $tm3
;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	
	ldrb	$dst, [$ppp, #(0+$nnn) ] 
	ldrb	$tm1, [$ppp, #(1+$nnn) ] 
	ldrb	$tm2, [$ppp, #(2+$nnn) ] 
	ldrb	$tm3, [$ppp, #(3+$nnn) ]

	MEND

;;;;;;;;;;;;;;;;;;;;;;;;;;;;		
	MACRO
		PACK_44_Step2 $dst, $tm1, $tm2, $tm3
;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	
	orr		$dst, $dst, $tm1, lsl #8
	orr		$dst, $dst, $tm2, lsl #16
	orr		$dst, $dst, $tm3, lsl #24

	MEND

;;;;;;;;;;;;;;;;;;;;;;;;;;;;		
	MACRO
		SAD_8x8
;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	stmdb   sp!, {RA-RZ, lr}								;//!!!MUST MATCH!!! REGIS_SHIFT_id, preserve 6 registers here as example
	;sub     sp, sp, #LOCAL_SHIFT_8x8_SAD					;//expend stack for local stack variables 	
 	
	mov		sum,    #0
	mov		height, #8
	
mark_8x8_SAD
	
 	pld	 [pSrc,srcStep]
 	pld	 [pRef,refStep]
 	
	PACK_44_Step1  rr1, pRef, 4, tm1, tm2, tm3
	ldr		ss1, [pSrc, #4 ] 
	PACK_44_Step2  rr1, tm1, tm2, tm3
	usada8	sum, ss1, rr1, sum
		
	PACK_44_Step1  rr2, pRef, 0, tm1, tm2, tm3	
	ldr		ss2, [pSrc], srcStep
	PACK_44_Step2  rr2, tm1, tm2, tm3
	usada8	sum, ss2, rr2, sum
	add		pRef, pRef, refStep
 	
 	subs height, height, #1
	bne	 mark_8x8_SAD	
	
	ldr		tm1, [sp, #pSAD_SHIFT_8x8_SAD]
	str		sum, [tm1]

    ;add    sp,sp,#LOCAL_SHIFT_8x8_SAD
    ldmfd  sp!,{RA-RZ,pc} 

	MEND


|ippiSAD8x8_8u32s_arm_v6| PROC
	SAD_8x8
  ENDP	;ippiSAD8x8_8u32s_arm_v6



pSrc			RN 0	
srcStep			RN 1
pRef			RN 2
refStep			RN 3

ss1				RN 4
ss2				RN 5 
rr1				RN 6
rr2				RN 7
rrrr			RN 8
tm1				RN 9 
tm2				RN 10
tm3				RN 11

sum				RN 12
height			RN 14

RA				RN 4		;first register to push into stack at entry
RZ				RN 11		;second last, and then it is lr=>pc

REGIS_SHIFT_4x4_SAD	EQU	(9*4)											;//registers pushed into stack to preserve caller state: 6 here as example
LOCAL_SHIFT_4x4_SAD	EQU	(0*0)											;//local stack variables: 2 here as example
SP_SHIFT_4x4_SAD		EQU	(REGIS_SHIFT_4x4_SAD + LOCAL_SHIFT_4x4_SAD)	;//total up shift from stack input

pSAD_SHIFT_4x4_SAD 	EQU (0*4 + SP_SHIFT_4x4_SAD)						;input from stack
mcType_SHIFT_4x4_SAD 	EQU (1*4 + SP_SHIFT_4x4_SAD)


;;;;;;;;;;;;;;;;;;;;;;;;;;;;		
	MACRO
		SAD_4x4
;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	stmdb   sp!, {RA-RZ, lr}								;//!!!MUST MATCH!!! REGIS_SHIFT_id, preserve 6 registers here as example
	;sub     sp, sp, #LOCAL_SHIFT_4x4_SAD					;//expend stack for local stack variables 	
 	sub		refStep, refStep, #3

	;;;
 	ldrb	rr1, [pRef], #1 
	ldrb	tm1, [pRef], #1  
	ldrb	tm2, [pRef], #1  
	ldrb	tm3, [pRef], refStep
 	ldrb	rr2, [pRef], #1 
	
	ldr		ss1, [pSrc], srcStep 
	mov		sum,    #0

	orr		rr1, rr1, tm1, lsl #8
	ldrb	tm1, [pRef], #1  
	orr		rr1, rr1, tm2, lsl #16
	ldrb	tm2, [pRef], #1  
	orr		rr1, rr1, tm3, lsl #24
	ldrb	tm3, [pRef], refStep
	
	ldr		ss2, [pSrc], srcStep 
	usada8	sum, ss1, rr1, sum
	 		
	orr		rr2, rr2, tm1, lsl #8
	orr		rr2, rr2, tm2, lsl #16
	orr		rr2, rr2, tm3, lsl #24

	usada8	sum, ss2, rr2, sum
	 	
	;;;
 	ldrb	rr1, [pRef], #1 
	ldrb	tm1, [pRef], #1  
	ldrb	tm2, [pRef], #1  
	ldrb	tm3, [pRef], refStep
 	ldrb	rr2, [pRef], #1 
	
	ldr		ss1, [pSrc], srcStep 

	orr		rr1, rr1, tm1, lsl #8
	ldrb	tm1, [pRef], #1  
	orr		rr1, rr1, tm2, lsl #16
	ldrb	tm2, [pRef], #1  
	orr		rr1, rr1, tm3, lsl #24
	ldrb	tm3, [pRef], refStep
	
	ldr		ss2, [pSrc], srcStep 
	usada8	sum, ss1, rr1, sum
	 		
	orr		rr2, rr2, tm1, lsl #8
	orr		rr2, rr2, tm2, lsl #16
	orr		rr2, rr2, tm3, lsl #24

	usada8	sum, ss2, rr2, sum


	ldr		tm1, [sp, #pSAD_SHIFT_4x4_SAD]
	str		sum, [tm1]

    ;add    sp,sp,#LOCAL_SHIFT_4x4_SAD
    ldmfd  sp!,{RA-RZ,pc} 

	MEND


|ippiSAD4x4_8u32s_arm_v6| PROC
	SAD_4x4
  ENDP	;ippiSAD4x4_8u32s_arm_v6



pSrc			RN 0	
srcStep			RN 1
pDst			RN 2
const_0			RN 3
const_mean		RN 3

ss0				RN 4
ss1				RN 5 
ss2				RN 6
ss3				RN 7
rrrr			RN 8
tm1				RN 9 
tm2				RN 10
tm3				RN 11

sum				RN 12
height			RN 14

RA				RN 4		;first register to push into stack at entry
RZ				RN 11		;second last, and then it is lr=>pc

REGIS_SHIFT_16x16_MAD	EQU	(9*4)											;//registers pushed into stack to preserve caller state: 6 here as example
LOCAL_SHIFT_16x16_MAD	EQU	(0*0)											;//local stack variables: 2 here as example
SP_SHIFT_16x16_MAD		EQU	(REGIS_SHIFT_16x16_MAD + LOCAL_SHIFT_16x16_MAD)	;//total up shift from stack input

pSAD_SHIFT_16x16_MAD 	EQU (0*4 + SP_SHIFT_16x16_MAD)						;input from stack
mcType_SHIFT_16x16_MAD 	EQU (1*4 + SP_SHIFT_16x16_MAD)

|ippiMeanAbsDev16x16_8u32s_C1R_arm_v6| PROC
	stmdb   sp!, {RA-RZ, lr}								;//!!!MUST MATCH!!! REGIS_SHIFT_id, preserve 6 registers here as example
	;sub     sp, sp, #LOCAL_SHIFT_16x16_MAD					;//expend stack for local stack variables 	
 	
 	pld		[pSrc]
 	pld		[pSrc,srcStep]
	sub		pSrc, pSrc, srcStep				;reset pSrc
	mov		sum,     #0
	mov		const_0, #0
	mov		height,  #16
	
mark_16x16_SUM
	
 	
	ldrd ss0, [pSrc, srcStep]!
	ldrd ss2, [pSrc, #8]
 	pld	 [pSrc,srcStep, lsl #1]

	usada8	sum, ss0, const_0, sum
	usada8	sum, ss1, const_0, sum
	usada8	sum, ss2, const_0, sum
	usada8	sum, ss3, const_0, sum
	
 	subs height, height, #1
	bne	 mark_16x16_SUM	
	
	sub		pSrc, pSrc, srcStep, lsl #4					;reset pSrc
	add		sum, sum, #128
	mov		const_mean, sum, lsr #8
	orr		const_mean, const_mean, const_mean, lsl #16
	orr		const_mean, const_mean, const_mean, lsl #8
	mov		sum,     #0
	mov		height,  #16

mark_16x16_MAD
	
	ldrd ss0, [pSrc, srcStep]!
	ldrd ss2, [pSrc, #8]
 	pld	 [pSrc,srcStep, lsl #1]

	usada8	sum, ss0, const_mean, sum
	usada8	sum, ss1, const_mean, sum
	usada8	sum, ss2, const_mean, sum
	usada8	sum, ss3, const_mean, sum
	
 	subs height, height, #1
	bne	 mark_16x16_MAD	

	str		sum, [pDst]

    ;add    sp,sp,#LOCAL_SHIFT_16x16_MAD
    ldmfd  sp!,{RA-RZ,pc} 

  ENDP	;ippiMeanAbsDev16x16_8u32s_C1R_arm_v6




pS1				RN 0	
src1Step		RN 1
src_stride		RN 1
pS2				RN 2
pD				RN 3
sum				RN 4

w1				RN 5
w2				RN 6 
w3				RN 7
w4				RN 8
dd10			RN 9
dd11			RN 10
dd20			RN 11
dd21			RN 12

height			RN 14

RA				RN 4		;first register to push into stack at entry
RZ				RN 11		;second last, and then it is lr=>pc

REGIS_SHIFT_8x8_SubSAD	EQU	(9*4)											;//registers pushed into stack to preserve caller state: 6 here as example
LOCAL_SHIFT_8x8_SubSAD	EQU	(0*0)											;//local stack variables: 2 here as example
SP_SHIFT_8x8_SubSAD		EQU	(REGIS_SHIFT_8x8_SubSAD + LOCAL_SHIFT_8x8_SubSAD)	;//total up shift from stack input

pSAD_SHIFT_8x8_SubSAD 	EQU (0*4 + SP_SHIFT_8x8_SubSAD)						;input from stack

|ippiSubSAD8x8_8u16s_C1R_16x16_arm_v6| PROC
	stmdb   sp!, {RA-RZ, lr}								;//!!!MUST MATCH!!! REGIS_SHIFT_id, preserve 6 registers here as example
	;sub     sp, sp, #LOCAL_SHIFT_8x8_SubSAD					;//expend stack for local stack variables 	
 	
	mov		sum,     #0
	mov		height,  #8
	sub		src_stride, src1Step, #8 
	
mark_8x8_SubSAD_SUM
	
 	ldrb w1, [pS1], #1
 	ldrb w2, [pS1], #1
 	ldrb w3, [pS2], #1
 	ldrb w4, [pS2], #1
 	
 	;0,1,2,3
 	orr		dd10, w1, w2, lsl #16	
 	ldrb	w1, [pS1], #1
 	ldrb	w2, [pS1], #1
 	orr		dd20, w3, w4, lsl #16		
 	ldrb	w3, [pS2], #1
 	ldrb	w4, [pS2], #1
 	orr		dd11, w1, w2, lsl #16	
 	ssub16	w2, dd10, dd20
 	ldrb	w1, [pS1], #1
 	str		w2, [pD],  #4
 	ldrb	w2, [pS1], #1
 	
 	orr		dd21, w3, w4, lsl #16	
  	ssub16	w4, dd11, dd21
 	ldrb	w3, [pS2], #1
 	str		w4, [pD], #4
 	ldrb	w4, [pS2], #1

 	orr		dd10, dd10, dd11, lsl #8
 	orr		dd20, dd20, dd21, lsl #8
	usada8	sum, dd10, dd20, sum

 	;4,5,6,7
 	orr		dd10, w1, w2, lsl #16	
 	ldrb	w1, [pS1], #1
 	ldrb	w2, [pS1], #1
 	orr		dd20, w3, w4, lsl #16		
 	ldrb	w3, [pS2], #1
 	ldrb	w4, [pS2], #1
 	orr		dd11, w1, w2, lsl #16	
 	ssub16	w2, dd10, dd20
 	str		w2, [pD],  #4
 	
 	orr		dd21, w3, w4, lsl #16	
  	ssub16	w4, dd11, dd21
 	str		w4, [pD],  #4

 	orr		dd10, dd10, dd11, lsl #8
 	orr		dd20, dd20, dd21, lsl #8
	usada8	sum, dd10, dd20, sum

 	;;;
 	add pS1, pS1, src_stride
 	add pS2, pS2, #8
 	
 	subs height, height, #1
	bne	 mark_8x8_SubSAD_SUM	
	
	ldr	 w1,  [sp, #pSAD_SHIFT_8x8_SubSAD]
	str	 sum, [w1]

    ;add    sp,sp,#LOCAL_SHIFT_8x8_SubSAD
    ldmfd  sp!,{RA-RZ,pc} 

  ENDP	;ippiSubSAD8x8_8u16s_C1R_16x16_arm_v6




pSrc			RN 0	
srcStep			RN 1
pDst			RN 2
dstStep			RN 3

ss0				RN 4
ss1				RN 5 
dd0				RN 6
dd1				RN 7
w2				RN 8
w3				RN 9
height			RN 10

RA				RN 4		;first register to push into stack at entry
RZ				RN 10		;second last, and then it is lr=>pc

REGIS_SHIFT_8x8_add88	EQU	(8*4)											;//registers pushed into stack to preserve caller state: 6 here as example
LOCAL_SHIFT_8x8_add88	EQU	(0*0)											;//local stack variables: 2 here as example
SP_SHIFT_8x8_add88		EQU	(REGIS_SHIFT_8x8_add88 + LOCAL_SHIFT_8x8_add88)	;//total up shift from stack input

pSAD_SHIFT_8x8_add88 	EQU (0*4 + SP_SHIFT_8x8_add88)						;input from stack

|ippiAdd8x8_16s8u_C1IRS_arm_v6| PROC
	stmdb   sp!, {RA-RZ, lr}								;//!!!MUST MATCH!!! REGIS_SHIFT_id, preserve 6 registers here as example
	;sub     sp, sp, #LOCAL_SHIFT_8x8_add88					;//expend stack for local stack variables 	
 	
	mov		height,  #8
	
mark_start_add88
	
	ldrh	ss0, [pSrc, #0*2]
	ldrh	w2,  [pSrc, #2*2]
	ldrh	ss1, [pSrc, #1*2]
	ldrh	w3,  [pSrc, #3*2]
 	ldrb	dd0, [pDst, #0*1]
	ldrb	dd1, [pDst, #1*1]
 	
 	orr		ss0, ss0, w2, lsl #16
	ldrb	w2, [pDst, #2*1]
 	orr		ss1, ss1, w3, lsl #16
 	ldrb	w3, [pDst, #3*1]
	
  	orr		dd0, dd0, w2, lsl #16
 	orr		dd1, dd1, w3, lsl #16
	
	sadd16	w2, ss0, dd0
	sadd16	w3, ss1, dd1
	usat16	w2, #8, w2
	usat16	w3, #8, w3
	orr		w2, w2, w3, lsl #8
	str		w2, [pDst, #0*4]
 	
 	;;;;;;;;;;;;;;;;;;;;;;;;;
 	;;;;;;;;;;;;;;;;;;;;;;;;;
 	
 	ldrh	ss0, [pSrc, #4*2]
	ldrh	w2,  [pSrc, #6*2]
	ldrh	ss1, [pSrc, #5*2]
	ldrh	w3,  [pSrc, #7*2]
 	ldrb	dd0, [pDst, #4*1]
	ldrb	dd1, [pDst, #5*1]
 	
 	orr		ss0, ss0, w2, lsl #16
	ldrb	w2,  [pDst, #6*1]
 	orr		ss1, ss1, w3, lsl #16
	ldrb	w3,  [pDst, #7*1]
 
  	orr		dd0, dd0, w2, lsl #16
 	orr		dd1, dd1, w3, lsl #16
	
	sadd16	w2, ss0, dd0
	sadd16	w3, ss1, dd1
	usat16	w2, #8, w2
	usat16	w3, #8, w3
	orr		w2, w2, w3, lsl #8
	str		w2, [pDst, #1*4]
 
	add		pDst, pDst, dstStep;
	add		pSrc, pSrc, srcStep;
		
	;;;
 	subs height, height, #1
	bne	 mark_start_add88	
	
    ;add    sp,sp,#LOCAL_SHIFT_8x8_add88D
    ldmfd  sp!,{RA-RZ,pc} 

  ENDP	;ippiAdd8x8_16s8u_C1IRS_arm_v6


  END


