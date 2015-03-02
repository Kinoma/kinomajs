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

	EXPORT  |dquant_trans_recon_arm|
	EXPORT  |dquant_trans_recon_dc_arm|
	EXPORT  |dquant_trans_recon_lossy_arm|
	EXPORT  |dquant_trans_recon_dc_lossy_arm|
	EXPORT  |pred_intra16x16_plane_arm|


coef		RN 0
dquant		RN 1
qp_per		RN 2
m			RN 3
co10		RN 4
co32		RN 5
dq10		RN 6
dq32		RN 7
t0			RN 8
t1			RN 9
pp0			RN 10
pp1			RN 11
pp2			RN 12
pp3			RN 14

pred		RN 0
dst			RN 1
CNST_255		RN 2
pitch		RN 14
m			RN 3
dst_3210	RN 4
dst_7654	RN 5
dst_ba98	RN 6
dst_fedc	RN 7

DQ_BITS		EQU		6
DQ_ROUND	EQU		(1<<(DQ_BITS-1))


|dquant_trans_recon_15_arm| PROC
	;pld		[coef]
	;pld		[dquant]
	;stmdb	sp!, {r4 - r11, lr} 
		
	smulbb  t1, co32, dq32				;2
	mov		t1, t1, lsl qp_per
	add		pp0, t0, t1
	sub		pp1, t0, t1

	smultt	t0, co10, dq10				;1
	mov		t0, t0, lsl qp_per
	smultt  t1, co32, dq32				;3

	ldrd	co10, [coef,   #8]			;load 4567
	ldrd	dq10, [dquant, #8]		

	mov		t1, t1, lsl qp_per
	sub		pp2, t1, t0, lsr #1
	add		pp3, t0, t1, lsr #1
	
	add		t0, pp0, pp3				;store 1234
	sub		t1, pp1, pp2
	strh	t0, [m, #0]
	strh	t1, [m, #2]

	add		t0, pp1, pp2
	sub		t1, pp0, pp3
	strh	t0, [m, #4]
	strh	t1, [m, #6]
		
	smulbb	t0, co10, dq10				;0	;process 4567
	mov		t0, t0, lsl qp_per
	smulbb  t1, co32, dq32				;2
	mov		t1, t1, lsl qp_per
	add		pp0, t0, t1
	sub		pp1, t0, t1

	smultt	t0, co10, dq10				;1
	mov		t0, t0, lsl qp_per
	smultt  t1, co32, dq32				;3

	ldrd	co10, [coef,   #16]			;load 8,9,10,11
	ldrd	dq10, [dquant, #16]		
	
	mov		t1, t1, lsl qp_per
	sub		pp2, t1, t0, lsr #1
	add		pp3, t0, t1, lsr #1

	add		t0, pp0, pp3				;store 4567
	sub		t1, pp1, pp2
	strh	t0, [m, #8]
	strh	t1, [m, #10]

	add		t0, pp1, pp2
	sub		t1, pp0, pp3
	strh	t0, [m, #12]
	strh	t1, [m, #14]

	smulbb	t0, co10, dq10				;0	;process 8,9,10,11
	mov		t0, t0, lsl qp_per
	smulbb  t1, co32, dq32				;2
	mov		t1, t1, lsl qp_per
	add		pp0, t0, t1
	sub		pp1, t0, t1

	smultt	t0, co10, dq10				;1
	mov		t0, t0, lsl qp_per
	smultt  t1, co32, dq32				;3

	ldrd	co10, [coef,   #24]			;load 12,13,14,15
	ldrd	dq10, [dquant, #24]		
	
	mov		t1, t1, lsl qp_per
	sub		pp2, t1, t0, lsr #1
	add		pp3, t0, t1, lsr #1

	add		t0, pp0, pp3				;store 8,9,10,11
	sub		t1, pp1, pp2
	strh	t0, [m, #16]
	strh	t1, [m, #18]

	add		t0, pp1, pp2
	sub		t1, pp0, pp3
	strh	t0, [m, #20]
	strh	t1, [m, #22]
		
	smulbb	t0, co10, dq10				;0	;process 12,13,14,15
	mov		t0, t0, lsl qp_per
	smulbb  t1, co32, dq32				;2
	mov		t1, t1, lsl qp_per
	add		pp0, t0, t1
	sub		pp1, t0, t1

	smultt	t0, co10, dq10				;1
	mov		t0, t0, lsl qp_per
	smultt  t1, co32, dq32				;3
	mov		t1, t1, lsl qp_per
	sub		pp2, t1, t0, lsr #1
	add		pp3, t0, t1, lsr #1

	add		t0, pp0, pp3				;store 12,13,14,15
	sub		t1, pp1, pp2
	strh	t0, [m, #24]
	strh	t1, [m, #26]

	add		t0, pp1, pp2
	sub		t1, pp0, pp3
	strh	t0, [m, #28]
	strh	t1, [m, #30]
		
	;;;;;;;;;;;;;;;;;;;;;;;
	;;;;;;;;;;;;;;;;;;;;;;;
	;;;;;;;;;;;;;;;;;;;;;;;
	;;;;;;;;;;;;;;;;;;;;;;;

	sub		pred, m, #16
	ldr		dst,  [sp, #36]
	mov		CNST_255, #255

	ldrsh	t0, [m, #0]					;0
	ldrsh	t1, [m, #16]		
	
	add		pp0, t0, t1
	sub     pp1, t0, t1

	ldrsh	t0, [m, #8]
	ldrsh	t1, [m, #24]		
	
	sub		pp2, t1, t0, asr #1
	add		pp3, t0, t1, asr #1

	ldrb	t1, [pred, #0]
	add		pp0, pp0, #(DQ_ROUND)
	add		t0, pp0, pp3
	add		t0, t0, t1, lsl #DQ_BITS
	mov		dst_3210, t0, asr #DQ_BITS
	cmp		dst_3210, CNST_255
	bichi	dst_3210, CNST_255, t0, asr #31 

	ldrb	t1, [pred, #12]
	sub		t0, pp0, pp3
	add		t0, t0, t1, lsl #DQ_BITS
	mov		dst_fedc, t0, asr #DQ_BITS
	cmp		dst_fedc, CNST_255
	bichi	dst_fedc, CNST_255, t0, asr #31 

	ldrb	t1, [pred, #4]
	add		pp1, pp1, #(DQ_ROUND)
	sub		t0, pp1, pp2
	add		t0, t0, t1, lsl #DQ_BITS
	mov		dst_7654, t0, asr #DQ_BITS
	cmp		dst_7654, CNST_255
	bichi	dst_7654, CNST_255, t0, asr #31 

	ldrb	t1, [pred, #8]
	add		t0, pp1, pp2
	add		t0, t0, t1, lsl #DQ_BITS
	mov		dst_ba98, t0, asr #DQ_BITS
	cmp		dst_ba98, CNST_255
	bichi	dst_ba98, CNST_255, t0, asr #31 

	ldrsh	t0, [m, #2]						;1
	ldrsh	t1, [m, #18]		
	
	add		pp0, t0, t1
	sub     pp1, t0, t1

	ldrsh	t0, [m, #10]
	ldrsh	t1, [m, #26]		
	
	sub		pp2, t1, t0, asr #1
	add		pp3, t0, t1, asr #1

	ldrb	t1, [pred, #1]
	add		pp0, pp0, #(DQ_ROUND)
	add		t0, pp0, pp3
	add		t0, t0, t1, lsl #DQ_BITS
	mov		t0, t0, asr #DQ_BITS
	cmp		t0, CNST_255
	bichi	t0, CNST_255, t0, asr #31 

	orr		dst_3210, dst_3210, t0, lsl #8

	ldrb	t1, [pred, #13]
	sub		t0, pp0, pp3
	add		t0, t0, t1, lsl #DQ_BITS
	mov		t0, t0, asr #DQ_BITS
	cmp		t0, CNST_255
	bichi	t0, CNST_255, t0, asr #31 

	orr		dst_fedc, dst_fedc, t0, lsl #8

	ldrb	t1, [pred, #5]
	add		pp1, pp1, #(DQ_ROUND)
	sub		t0, pp1, pp2
	add		t0, t0, t1, lsl #DQ_BITS
	mov		t0, t0, asr #DQ_BITS
	cmp		t0, CNST_255
	bichi	t0, CNST_255, t0, asr #31 

	orr		dst_7654, dst_7654, t0, lsl #8

	ldrb	t1, [pred, #9]
	add		t0, pp1, pp2
	add		t0, t0, t1, lsl #DQ_BITS
	mov		t0, t0, asr #DQ_BITS
	cmp		t0, CNST_255
	bichi	t0, CNST_255, t0, asr #31 

	orr		dst_ba98, dst_ba98, t0, lsl #8

	ldrsh	t0, [m, #4]						;2
	ldrsh	t1, [m, #20]		
	
	add		pp0, t0, t1
	sub     pp1, t0, t1

	ldrsh	t0, [m, #12]
	ldrsh	t1, [m, #28]		
	
	sub		pp2, t1, t0, asr #1
	add		pp3, t0, t1, asr #1

	ldrb	t1, [pred, #2]
	add		pp0, pp0, #(DQ_ROUND)
	add		t0, pp0, pp3
	add		t0, t0, t1, lsl #DQ_BITS
	mov		t0, t0, asr #DQ_BITS
	cmp		t0, CNST_255
	bichi	t0, CNST_255, t0, asr #31 

	orr		dst_3210, dst_3210, t0, lsl #16

	ldrb	t1, [pred, #14]
	sub		t0, pp0, pp3
	add		t0, t0, t1, lsl #DQ_BITS
	mov		t0, t0, asr #DQ_BITS
	cmp		t0, CNST_255
	bichi	t0, CNST_255, t0, asr #31 

	orr		dst_fedc, dst_fedc, t0, lsl #16

	ldrb	t1, [pred, #6]
	add		pp1, pp1, #(DQ_ROUND)
	sub		t0, pp1, pp2
	add		t0, t0, t1, lsl #DQ_BITS
	mov		t0, t0, asr #DQ_BITS
	cmp		t0, CNST_255
	bichi	t0, CNST_255, t0, asr #31 

	orr		dst_7654, dst_7654, t0, lsl #16

	ldrb	t1, [pred, #10]
	add		t0, pp1, pp2
	add		t0, t0, t1, lsl #DQ_BITS
	mov		t0, t0, asr #DQ_BITS
	cmp		t0, CNST_255
	bichi	t0, CNST_255, t0, asr #31 

	orr		dst_ba98, dst_ba98, t0, lsl #16
	
	ldrsh	t0, [m, #6]						;3
	ldrsh	t1, [m, #22]		
	
	add		pp0, t0, t1
	sub     pp1, t0, t1

	ldrsh	t0, [m, #14]
	ldrsh	t1, [m, #30]		
	
	sub		pp2, t1, t0, asr #1
	add		pp3, t0, t1, asr #1

	ldrb	t1, [pred, #3]
	add		pp0, pp0, #(DQ_ROUND)
	add		t0, pp0, pp3
	add		t0, t0, t1, lsl #DQ_BITS
	mov		t0, t0, asr #DQ_BITS
	cmp		t0, CNST_255
	bichi	t0, CNST_255, t0, asr #31 

	orr		dst_3210, dst_3210, t0, lsl #24

	ldrb	t1, [pred, #15]
	sub		t0, pp0, pp3
	add		t0, t0, t1, lsl #DQ_BITS
	mov		t0, t0, asr #DQ_BITS
	cmp		t0, CNST_255
	bichi	t0, CNST_255, t0, asr #31 

	orr		dst_fedc, dst_fedc, t0, lsl #24

	ldrb	t1, [pred, #7]
	add		pp1, pp1, #(DQ_ROUND)
	sub		t0, pp1, pp2
	add		t0, t0, t1, lsl #DQ_BITS
	mov		t0, t0, asr #DQ_BITS
	cmp		t0, CNST_255
	bichi	t0, CNST_255, t0, asr #31 

	orr		dst_7654, dst_7654, t0, lsl #24

	ldr		pitch,  [sp, #40]

	ldrb	t1, [pred, #11]
	add		t0, pp1, pp2
	add		t0, t0, t1, lsl #DQ_BITS
	mov		t0, t0, asr #DQ_BITS
	cmp		t0, CNST_255
	bichi	t0, CNST_255, t0, asr #31 

	orr		dst_ba98, dst_ba98, t0, lsl #24
	
	str		dst_3210, [dst, #0]
	str		dst_7654, [dst, pitch]!	
	str		dst_ba98, [dst, pitch]!	
	str		dst_fedc, [dst, pitch]
		
 	ldmia sp!, {r4 - r11, pc} 



	ENDP  ; |dquant_trans_recon_15_arm|


|dquant_trans_recon_15_lossy_arm| PROC
	;pld		[coef]
	;pld		[dquant]
	;stmdb	sp!, {r4 - r11, lr} 
		
	smulbb  t1, co32, dq32				;2
	mov		t1, t1, lsl qp_per
	add		pp0, t0, t1
	sub		pp1, t0, t1

	smultt	t0, co10, dq10				;1
	mov		t0, t0, lsl qp_per
	smultt  t1, co32, dq32				;3

	ldrd	co10, [coef,   #8]			;load 4567
	ldrd	dq10, [dquant, #8]		

	mov		t1, t1, lsl qp_per
	sub		pp2, t1, t0, lsr #1
	add		pp3, t0, t1, lsr #1
	
	add		t0, pp0, pp3				;store 1234
	sub		t1, pp1, pp2
	strh	t0, [m, #0]
	strh	t1, [m, #2]

	add		t0, pp1, pp2
	sub		t1, pp0, pp3
	strh	t0, [m, #4]
	strh	t1, [m, #6]
		
	smulbb	t0, co10, dq10				;0	;process 4567
	mov		t0, t0, lsl qp_per
	smulbb  t1, co32, dq32				;2
	mov		t1, t1, lsl qp_per
	add		pp0, t0, t1
	sub		pp1, t0, t1

	smultt	t0, co10, dq10				;1
	mov		t0, t0, lsl qp_per
	smultt  t1, co32, dq32				;3
	
	mov		t1, t1, lsl qp_per
	sub		pp2, t1, t0, lsr #1
	add		pp3, t0, t1, lsr #1

	add		t0, pp0, pp3				;store 4567
	sub		t1, pp1, pp2
	strh	t0, [m, #8]
	strh	t1, [m, #10]

	add		t0, pp1, pp2
	sub		t1, pp0, pp3
	strh	t0, [m, #12]
	strh	t1, [m, #14]
				
	;;;;;;;;;;;;;;;;;;;;;;;
	;;;;;;;;;;;;;;;;;;;;;;;
	;;;;;;;;;;;;;;;;;;;;;;;
	;;;;;;;;;;;;;;;;;;;;;;;

	sub		pred, m, #16
	ldr		dst,  [sp, #36]
	mov		CNST_255, #255

	ldrsh	t0, [m, #0]					;0
	mov		t1, #0		
	
	add		pp0, t0, t1
	sub     pp1, t0, t1

	ldrsh	t0, [m, #8]
	mov		t1, #0		;***
	
	sub		pp2, t1, t0, asr #1
	add		pp3, t0, t1, asr #1

	ldrb	t1, [pred, #0]
	add		pp0, pp0, #(DQ_ROUND)
	add		t0, pp0, pp3
	add		t0, t0, t1, lsl #DQ_BITS
	mov		dst_3210, t0, asr #DQ_BITS
	cmp		dst_3210, CNST_255
	bichi	dst_3210, CNST_255, t0, asr #31 

	ldrb	t1, [pred, #12]
	sub		t0, pp0, pp3
	add		t0, t0, t1, lsl #DQ_BITS
	mov		dst_fedc, t0, asr #DQ_BITS
	cmp		dst_fedc, CNST_255
	bichi	dst_fedc, CNST_255, t0, asr #31 

	ldrb	t1, [pred, #4]
	add		pp1, pp1, #(DQ_ROUND)
	sub		t0, pp1, pp2
	add		t0, t0, t1, lsl #DQ_BITS
	mov		dst_7654, t0, asr #DQ_BITS
	cmp		dst_7654, CNST_255
	bichi	dst_7654, CNST_255, t0, asr #31 

	ldrb	t1, [pred, #8]
	add		t0, pp1, pp2
	add		t0, t0, t1, lsl #DQ_BITS
	mov		dst_ba98, t0, asr #DQ_BITS
	cmp		dst_ba98, CNST_255
	bichi	dst_ba98, CNST_255, t0, asr #31 

	ldrsh	t0, [m, #2]						;1
	mov		t1, #0		
	
	add		pp0, t0, t1
	sub     pp1, t0, t1

	ldrsh	t0, [m, #10]
	mov		t1, #0		;***
	
	sub		pp2, t1, t0, asr #1
	add		pp3, t0, t1, asr #1

	ldrb	t1, [pred, #1]
	add		pp0, pp0, #(DQ_ROUND)
	add		t0, pp0, pp3
	add		t0, t0, t1, lsl #DQ_BITS
	mov		t0, t0, asr #DQ_BITS
	cmp		t0, CNST_255
	bichi	t0, CNST_255, t0, asr #31 

	orr		dst_3210, dst_3210, t0, lsl #8

	ldrb	t1, [pred, #13]
	sub		t0, pp0, pp3
	add		t0, t0, t1, lsl #DQ_BITS
	mov		t0, t0, asr #DQ_BITS
	cmp		t0, CNST_255
	bichi	t0, CNST_255, t0, asr #31 

	orr		dst_fedc, dst_fedc, t0, lsl #8

	ldrb	t1, [pred, #5]
	add		pp1, pp1, #(DQ_ROUND)
	sub		t0, pp1, pp2
	add		t0, t0, t1, lsl #DQ_BITS
	mov		t0, t0, asr #DQ_BITS
	cmp		t0, CNST_255
	bichi	t0, CNST_255, t0, asr #31 

	orr		dst_7654, dst_7654, t0, lsl #8

	ldrb	t1, [pred, #9]
	add		t0, pp1, pp2
	add		t0, t0, t1, lsl #DQ_BITS
	mov		t0, t0, asr #DQ_BITS
	cmp		t0, CNST_255
	bichi	t0, CNST_255, t0, asr #31 

	orr		dst_ba98, dst_ba98, t0, lsl #8

	ldrsh	t0, [m, #4]						;2
	mov		t1, #0		
	
	add		pp0, t0, t1
	sub     pp1, t0, t1

	ldrsh	t0, [m, #12]
	mov		t1, #0		;***		
	
	sub		pp2, t1, t0, asr #1
	add		pp3, t0, t1, asr #1

	ldrb	t1, [pred, #2]
	add		pp0, pp0, #(DQ_ROUND)
	add		t0, pp0, pp3
	add		t0, t0, t1, lsl #DQ_BITS
	mov		t0, t0, asr #DQ_BITS
	cmp		t0, CNST_255
	bichi	t0, CNST_255, t0, asr #31 

	orr		dst_3210, dst_3210, t0, lsl #16

	ldrb	t1, [pred, #14]
	sub		t0, pp0, pp3
	add		t0, t0, t1, lsl #DQ_BITS
	mov		t0, t0, asr #DQ_BITS
	cmp		t0, CNST_255
	bichi	t0, CNST_255, t0, asr #31 

	orr		dst_fedc, dst_fedc, t0, lsl #16

	ldrb	t1, [pred, #6]
	add		pp1, pp1, #(DQ_ROUND)
	sub		t0, pp1, pp2
	add		t0, t0, t1, lsl #DQ_BITS
	mov		t0, t0, asr #DQ_BITS
	cmp		t0, CNST_255
	bichi	t0, CNST_255, t0, asr #31 

	orr		dst_7654, dst_7654, t0, lsl #16

	ldrb	t1, [pred, #10]
	add		t0, pp1, pp2
	add		t0, t0, t1, lsl #DQ_BITS
	mov		t0, t0, asr #DQ_BITS
	cmp		t0, CNST_255
	bichi	t0, CNST_255, t0, asr #31 

	orr		dst_ba98, dst_ba98, t0, lsl #16
	
	ldrsh	t0, [m, #6]						;3
	mov		t1, #0		
	
	add		pp0, t0, t1
	sub     pp1, t0, t1

	ldrsh	t0, [m, #14]
	mov		t1, #0			;***		
	
	sub		pp2, t1, t0, asr #1
	add		pp3, t0, t1, asr #1

	ldrb	t1, [pred, #3]
	add		pp0, pp0, #(DQ_ROUND)
	add		t0, pp0, pp3
	add		t0, t0, t1, lsl #DQ_BITS
	mov		t0, t0, asr #DQ_BITS
	cmp		t0, CNST_255
	bichi	t0, CNST_255, t0, asr #31 

	orr		dst_3210, dst_3210, t0, lsl #24

	ldrb	t1, [pred, #15]
	sub		t0, pp0, pp3
	add		t0, t0, t1, lsl #DQ_BITS
	mov		t0, t0, asr #DQ_BITS
	cmp		t0, CNST_255
	bichi	t0, CNST_255, t0, asr #31 

	orr		dst_fedc, dst_fedc, t0, lsl #24

	ldrb	t1, [pred, #7]
	add		pp1, pp1, #(DQ_ROUND)
	sub		t0, pp1, pp2
	add		t0, t0, t1, lsl #DQ_BITS
	mov		t0, t0, asr #DQ_BITS
	cmp		t0, CNST_255
	bichi	t0, CNST_255, t0, asr #31 

	orr		dst_7654, dst_7654, t0, lsl #24

	ldr		pitch,  [sp, #40]

	ldrb	t1, [pred, #11]
	add		t0, pp1, pp2
	add		t0, t0, t1, lsl #DQ_BITS
	mov		t0, t0, asr #DQ_BITS
	cmp		t0, CNST_255
	bichi	t0, CNST_255, t0, asr #31 

	orr		dst_ba98, dst_ba98, t0, lsl #24
	
	str		dst_3210, [dst, #0]
	str		dst_7654, [dst, pitch]!	
	str		dst_ba98, [dst, pitch]!	
	str		dst_fedc, [dst, pitch]
		
 	ldmia sp!, {r4 - r11, pc} 

	ENDP  ; |dquant_trans_recon_15_lossy_arm|



|dquant_trans_recon_arm| PROC
	pld		[coef]
	pld		[dquant]
	stmdb	sp!, {r4 - r11, lr} 

	ldrd	co10, [coef,   #0]			;load 0123	
	ldrd	dq10, [dquant, #0]		
	
	smulbb	t0, co10, dq10				;0
	mov		t0, t0, lsl qp_per

	bl  dquant_trans_recon_15_arm

	ENDP  ; |dquant_trans_recon_arm|


|dquant_trans_recon_dc_arm| PROC
	pld		[coef]
	pld		[dquant]
	stmdb	sp!, {r4 - r11, lr} 

	ldrd	co10, [coef,   #0]			;load 0123	
	ldrd	dq10, [dquant, #0]		
	
	ldrh	t0, [m, #0]				;0 holds DC

	bl  dquant_trans_recon_15_arm

	ENDP  ; |dquant_trans_recon_arm|

|dquant_trans_recon_lossy_arm| PROC
	pld		[coef]
	pld		[dquant]
	stmdb	sp!, {r4 - r11, lr} 

	ldrd	co10, [coef,   #0]			;load 0123	
	ldrd	dq10, [dquant, #0]		
	
	smulbb	t0, co10, dq10				;0
	mov		t0, t0, lsl qp_per

	bl  dquant_trans_recon_15_lossy_arm

	ENDP  ; |dquant_trans_recon_arm|


|dquant_trans_recon_dc_lossy_arm| PROC
	pld		[coef]
	pld		[dquant]
	stmdb	sp!, {r4 - r11, lr} 

	ldrd	co10, [coef,   #0]			;load 0123	
	ldrd	dq10, [dquant, #0]		
	
	ldrh	t0, [m, #0]				;0 holds DC

	bl  dquant_trans_recon_15_lossy_arm

	ENDP  ; |dquant_trans_recon_arm|



icc			RN 0
ib			RN 1
ic			RN 2
dst			RN 3
pitch		RN 12
count		RN 5
ibb0		RN 6
CONST_255	RN 7
ibb			RN 8
tmp1		RN 9
tmp2		RN 10
tmp3		RN 11
tmp4		RN 14


	MACRO
		ONE_STEP_WITH $R

	add		$R, ibb, icc
	add		ibb, ibb, ib
	mov		$R, $R, asr #5
	cmp		$R, CONST_255
	bichi	$R, CONST_255, $R, asr #31 

	MEND

|pred_intra16x16_plane_arm| PROC
	ldr		pitch, [sp, #0]
	stmdb	sp!, {r4 - r11, lr} 

	mov		count, #15
	mov		CONST_255, #255
	sub     ibb0, ib, ib, lsl #3 	

start_pred_intra16x16_plane_arm
	mov		ibb, ibb0

	ONE_STEP_WITH tmp1
	ONE_STEP_WITH tmp2
	ONE_STEP_WITH tmp3
	ONE_STEP_WITH tmp4
	orr		tmp1, tmp1, tmp2, lsl #8
	orr		tmp1, tmp1, tmp3, lsl #16
	orr		tmp4, tmp1, tmp4, lsl #24
	str		tmp4, [dst]
	
	ONE_STEP_WITH tmp1
	ONE_STEP_WITH tmp2
	ONE_STEP_WITH tmp3
	ONE_STEP_WITH tmp4
	orr		tmp1, tmp1, tmp2, lsl #8
	orr		tmp1, tmp1, tmp3, lsl #16
	orr		tmp4, tmp1, tmp4, lsl #24
	str		tmp4, [dst, #4]
	
	ONE_STEP_WITH tmp1
	ONE_STEP_WITH tmp2
	ONE_STEP_WITH tmp3
	ONE_STEP_WITH tmp4
	orr		tmp1, tmp1, tmp2, lsl #8
	orr		tmp1, tmp1, tmp3, lsl #16
	orr		tmp4, tmp1, tmp4, lsl #24
	str		tmp4, [dst, #8]
	
	ONE_STEP_WITH tmp1
	ONE_STEP_WITH tmp2
	ONE_STEP_WITH tmp3
	ONE_STEP_WITH tmp4
	orr		tmp1, tmp1, tmp2, lsl #8
	orr		tmp1, tmp1, tmp3, lsl #16
	orr		tmp4, tmp1, tmp4, lsl #24
	str		tmp4, [dst, #12]
	
	
	add		dst, dst, pitch
	add		icc, icc, ic
	subs	count, count, #1
 	bpl		start_pred_intra16x16_plane_arm
 	
 	ldmia sp!, {r4 - r11, pc} 

	ENDP  ; |pred_intra16x16_plane_arm|


  END


