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

	EXPORT	|jpeg_idct_4x4_arm_v6|
	EXPORT	|jpeg_idct_2x2_arm_v6|
	EXPORT	|yuv2rgb565_arm_v6|
	EXPORT	|jpeg_idct_add_sat_arm_v6|

w12				RN 0
w34				RN 1
wk				RN 2
quant			RN 3
coef			RN 4	
	
qq0				RN 5
qq1				RN 6
qq2				RN 7
qq3				RN 8
qx				RN 9
qq5				RN 10
qq6				RN 11
qq7				RN 12
qq				RN 14


;;;;;;;;;;;;;;;;;;;;;;;;;;;;		
	MACRO
		IDCT_COL_4_ARM_V6 $offset, $version
;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	
    ldrsh qq0, [coef, #(8*2*0+$offset)]
    ldrsh qq1, [coef, #(8*2*1+$offset)]
    ldrsh qq2, [coef, #(8*2*2+$offset)]
    ldrsh qq3, [coef, #(8*2*3+$offset)]
    ldrsh qq5, [coef, #(8*2*5+$offset)]
    ldrsh qq6, [coef, #(8*2*6+$offset)]
    ldrsh qq7, [coef, #(8*2*7+$offset)]

	cmp		qq1, #0
	cmpeq	qq2, #0
	cmpeq	qq3, #0
	cmpeq	qq5, #0
	cmpeq	qq6, #0
	cmpeq	qq7, #0
	beq $version.mark_dc
		
    ldrsh qq, [quant, #(8*2*0+$offset)]
    ldrsh qx,[quant, #(8*2*1+$offset)]
    mul  qq0, qq, qq0			
    ldrsh qq, [quant, #(8*2*2+$offset)]
    mul  qq1, qx, qq1			
    ldrsh qx, [quant, #(8*2*3+$offset)]
    mul  qq2, qq, qq2			
    ldrsh qq, [quant, #(8*2*5+$offset)]
    mul  qq3, qx, qq3			
    ldrsh qx, [quant, #(8*2*6+$offset)]
    mul  qq5, qq, qq5			
    ldrsh qq, [quant, #(8*2*7+$offset)]
    mul  qq6, qx, qq6			
    mul  qq7, qq, qq7   
    
	mov		qq0, qq0, lsl #9
	sub		qq2, qq2, qq6
	smulbb  qq2, w12,qq2
	add		qq6, qq0, qq2
	sub		qq2, qq0, qq2
	
	sub		qq5, qq5, qq3
	sub		qq1, qq1, qq7
	add		qq3, qq5, qq1
	smultb	qq3, w34, qq3
	smultb	qq1, w12, qq1
	sub		qq1, qq1, qq3
	smlabb	qq5, w34, qq5, qq3
				
	add     qq, qq6, qq5
	mov     qq, qq, asr #9
	strh    qq, [wk, #(8*2*0+$offset)]
	sub     qq, qq2, qq1
	mov     qq, qq, asr #9
	strh    qq, [wk, #(8*2*1+$offset)]
	add     qq, qq2, qq1
	mov     qq, qq, asr #9
	strh    qq, [wk, #(8*2*2+$offset)]
	sub     qq, qq6, qq5
	mov     qq, qq, asr #9
	strh    qq, [wk, #(8*2*3+$offset)]
    bl		$version.end

$version.mark_dc
	ldrh	qq, [quant, #(8*2*0+$offset)]
	mul     qq, qq0, qq
	strh    qq, [wk, #(8*2*0+$offset)]
	strh    qq, [wk, #(8*2*1+$offset)]
	strh    qq, [wk, #(8*2*2+$offset)]
	strh    qq, [wk, #(8*2*3+$offset)]
  
$version.end  

	MEND



cnst_255		RN 3
out				RN 4
cnst_128		RN 9


;;;;;;;;;;;;;;;;;;;;;;;;;;;;		
	MACRO
		IDCT_ROW_4_ARM_V6 $offset, $version
;;;;;;;;;;;;;;;;;;;;;;;;;;;;

    ldrsh qq0, [wk, #(2*0+$offset)]
    ldrsh qq1, [wk, #(2*1+$offset)]
    ldrsh qq2, [wk, #(2*2+$offset)]
    ldrsh qq3, [wk, #(2*3+$offset)]
    ldrsh qq5, [wk, #(2*5+$offset)]
    ldrsh qq6, [wk, #(2*6+$offset)]
    ldrsh qq7, [wk, #(2*7+$offset)]
	  
	cmp		qq1, #0
	cmpeq	qq2, #0
	cmpeq	qq3, #0
	cmpeq	qq5, #0
	cmpeq	qq6, #0
	cmpeq	qq7, #0
    beq		$version.mark_row_8_dc

	mov		qq0, qq0, lsl #9
	sub		qq2, qq2, qq6
	smulbb  qq2, w12,qq2
	add		qq6, qq0, qq2
	sub		qq2, qq0, qq2
	
	sub		qq5, qq5, qq3
	sub		qq1, qq1, qq7
	add		qq3, qq5, qq1
	smultb	qq3, w34, qq3
	smultb	qq1, w12, qq1
	sub		qq1, qq1, qq3
	smlabb	qq5, w34, qq5, qq3
	
	add     qq, qq6, qq5
	add		qq, cnst_128, qq, asr #14
	usat	qq, #8, qq
	strb    qq, [out, #0]
																	
	sub     qq, qq2, qq1
	add		qq, cnst_128, qq, asr #14
	usat	qq, #8, qq
	strb    qq, [out, #1]
	
	add     qq, qq2, qq1
	add		qq, cnst_128, qq, asr #14
	usat	qq, #8, qq
	strb    qq, [out, #2]
	
	sub     qq, qq6, qq5
	add		qq, cnst_128, qq, asr #14
	usat	qq, #8, qq
	strb    qq, [out, #3]
		
	bl		$version.end

$version.mark_row_8_dc
	add		qq, cnst_128, qq0, asr #5
	usat	qq, #8, qq
	strb    qq, [out, #0]
	strb    qq, [out, #1]
	strb    qq, [out, #2]
	strb    qq, [out, #3]

$version.end

	MEND


REGIS_SHIFT_4x4		EQU	(9*4)
CACHE_SHIFT_4x4		EQU	(0*4)
SP_SHIFT_4x4		EQU	(REGIS_SHIFT_4x4 + CACHE_SHIFT_4x4)

;tmp1_SHIFT_4x4 		EQU (0*4)
;tmp2_SHIFT_4x4 		EQU (1*4)
coef_SHIFT_4x4 		EQU (0*4 + SP_SHIFT_4x4)
out0_SHIFT_4x4 		EQU (1*4 + SP_SHIFT_4x4)
out1_SHIFT_4x4 		EQU (2*4 + SP_SHIFT_4x4)
out2_SHIFT_4x4 		EQU (3*4 + SP_SHIFT_4x4)
out3_SHIFT_4x4 		EQU (4*4 + SP_SHIFT_4x4)

|jpeg_idct_4x4_arm_v6| PROC
    stmfd sp!,{r4-r11,lr}
 
  	ldr		coef, [sp, #coef_SHIFT_4x4]
	
	IDCT_COL_4_ARM_V6 0, idct_col_4x4_0_
	IDCT_COL_4_ARM_V6 2, idct_col_4x4_1_
	IDCT_COL_4_ARM_V6 4, idct_col_4x4_2_
	IDCT_COL_4_ARM_V6 6, idct_col_4x4_3_
	IDCT_COL_4_ARM_V6 10,idct_col_4x4_5_
	IDCT_COL_4_ARM_V6 12,idct_col_4x4_6_
	IDCT_COL_4_ARM_V6 14,idct_col_4x4_7_

	mov	cnst_255, #255
	mov	cnst_128, #128

  	ldr out, [sp, #out0_SHIFT_4x4]
	IDCT_ROW_4_ARM_V6  0, idct_row_4x4_0_
   	
   	ldr out, [sp, #out1_SHIFT_4x4]
	IDCT_ROW_4_ARM_V6 16, idct_row_4x4_1_ 
 
   	ldr out, [sp, #out2_SHIFT_4x4]
	IDCT_ROW_4_ARM_V6 32, idct_row_4x4_2_
										 
   	ldr out, [sp, #out3_SHIFT_4x4]		 
	IDCT_ROW_4_ARM_V6 48, idct_row_4x4_3_
  
    ldmfd   sp!,{r4-r11,pc} 

  ENDP



w12				RN 0
w34				RN 1
wk				RN 2
quant			RN 3
coef			RN 4	
	
qq0				RN 5
qq1				RN 6
qx				RN 7
qq3				RN 8
qy				RN 9
qq5				RN 10
qz				RN 11
qq7				RN 12
qq				RN 14


;;;;;;;;;;;;;;;;;;;;;;;;;;;;		
	MACRO
		IDCT_COL_2_ARM_V6 $offset, $version
;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	
    ldrsh qq0, [coef, #(8*2*0+$offset)]
    ldrsh qq1, [coef, #(8*2*1+$offset)]
    ldrsh qq3, [coef, #(8*2*3+$offset)]
    ldrsh qq5, [coef, #(8*2*5+$offset)]
    ldrsh qq7, [coef, #(8*2*7+$offset)]

	cmp		qq1, #0
	cmpeq	qq3, #0
	cmpeq	qq5, #0
	cmpeq	qq7, #0
	beq $version.mark_dc
		
    ldrsh qq, [quant, #(8*2*0+$offset)]
    ldrsh qx, [quant, #(8*2*1+$offset)]
    ldrsh qy, [quant, #(8*2*3+$offset)]
    ldrsh qz, [quant, #(8*2*5+$offset)]
    mul  qq0, qq, qq0			
    ldrsh qq, [quant, #(8*2*7+$offset)]
    mul  qq1, qx, qq1			
    mul  qq3, qy, qq3			
    mul  qq5, qz, qq5			
    mul  qq7, qq, qq7   
    	
	sub		qq5, qq5, qq3
	sub		qq1, qq1, qq7
	add		qq3, qq5, qq1
	smultb	qq3, w34,qq3
	smultb	qq1, w12,qq1
	smulbb	qq5, w34,qq5
	sub		qq1, qq1, qq5
	sub		qq1, qq1, qq3, lsl #1
				
	sub     qq, qq0, qq1, asr #10
	strh    qq, [wk, #(8*2*0+$offset)]
	add     qq, qq0, qq1, asr #10
	strh    qq, [wk, #(8*2*1+$offset)]
    bl		$version.end

$version.mark_dc
	ldrh	qq, [quant, #(8*2*0+$offset)]
	mul     qq, qq0, qq
	strh    qq, [wk, #(8*2*0+$offset)]
	strh    qq, [wk, #(8*2*1+$offset)]
  
$version.end  

	MEND



cnst_255		RN 3
out				RN 4
cnst_128		RN 9


;;;;;;;;;;;;;;;;;;;;;;;;;;;;		
	MACRO
		IDCT_ROW_2_ARM_V6 $offset, $version
;;;;;;;;;;;;;;;;;;;;;;;;;;;;

    ldrsh qq0, [wk, #(2*0+$offset)]
    ldrsh qq1, [wk, #(2*1+$offset)]
    ldrsh qq3, [wk, #(2*3+$offset)]
    ldrsh qq5, [wk, #(2*5+$offset)]
    ldrsh qq7, [wk, #(2*7+$offset)]
	  
	cmp		qq1, #0
	cmpeq	qq3, #0
	cmpeq	qq5, #0
	cmpeq	qq7, #0
    beq		$version.mark_dc

	sub		qq5, qq5, qq3						
	sub		qq1, qq1, qq7						
	add		qq3, qq5, qq1						
	smultb	qq3, w34,qq3						
	smultb	qq1, w12,qq1						
	smulbb	qq5, w34,qq5						
	sub		qq1, qq1, qq5						
	sub		qq1, qq1, qq3, lsl #1					
	
	sub     qq, qq0, qq1, asr #10
	add		qq, cnst_128, qq, asr #5
	usat	qq, #8, qq
	strb    qq, [out, #0]
																	
	add     qq, qq0, qq1, asr #10
	add		qq, cnst_128, qq, asr #5
	usat	qq, #8, qq
	strb    qq, [out, #1]
			
	bl		$version.end

$version.mark_dc
	add     qq, cnst_128, qq0, asr #5
	cmp		qq, cnst_255
    bichi	qq, cnst_255, qq, asr #31 
	strb    qq, [out, #0]
	strb    qq, [out, #1]

$version.end

	MEND

REGIS_SHIFT_2x2		EQU	(9*4)
CACHE_SHIFT_2x2		EQU	(0*4)
SP_SHIFT_2x2		EQU	(REGIS_SHIFT_2x2 + CACHE_SHIFT_2x2)

coef_SHIFT_2x2 		EQU (0*4 + SP_SHIFT_2x2)
out0_SHIFT_2x2 		EQU (1*4 + SP_SHIFT_2x2)
out1_SHIFT_2x2 		EQU (2*4 + SP_SHIFT_2x2)
		   
|jpeg_idct_2x2_arm_v6| PROC
    stmfd sp!,{r4-r11,lr}
 
  	ldr	  coef, [sp, #coef_SHIFT_2x2]
	
	IDCT_COL_2_ARM_V6 0, idct_col_2x2_0_
	IDCT_COL_2_ARM_V6 2, idct_col_2x2_1_
	IDCT_COL_2_ARM_V6 6, idct_col_2x2_3_
	IDCT_COL_2_ARM_V6 10,idct_col_2x2_5_
	IDCT_COL_2_ARM_V6 14,idct_col_2x2_7_
								  
	mov	cnst_255, #255
	mov	cnst_128, #128

  	ldr out, [sp, #out0_SHIFT_2x2]
	IDCT_ROW_2_ARM_V6  0, idct_row_2x2_0_
   	
   	ldr out, [sp, #out1_SHIFT_2x2]
	IDCT_ROW_2_ARM_V6 16, idct_row_2x2_1_ 
   
    ldmfd   sp!,{r4-r11,pc} 

  ENDP



w1				RN 0
w3				RN 1
cnst_31			RN 2
cnst_63			RN 3

y0				RN 4
u0				RN 5	
v0				RN 6
out				RN 7
col				RN 8
y				RN 9
cb				RN 10
cr				RN 11
pix				RN 12

REGIS_SHIFT_clr		EQU	(10*4)
CACHE_SHIFT_clr		EQU	(0*4)
SP_SHIFT_clr		EQU	(REGIS_SHIFT_clr + CACHE_SHIFT_clr)

y0_SHIFT_clr 		EQU (0*4 + SP_SHIFT_clr)
u0_SHIFT_clr 		EQU (1*4 + SP_SHIFT_clr)
v0_SHIFT_clr 		EQU (2*4 + SP_SHIFT_clr)
out_SHIFT_clr 		EQU (3*4 + SP_SHIFT_clr)
col_SHIFT_clr 		EQU (4*4 + SP_SHIFT_clr)
		   
|yuv2rgb565_arm_v6| PROC
    stmfd sp!,{r4-r12,lr}
 
  	ldr		y0, [sp, #y0_SHIFT_clr]
  	ldr		u0, [sp, #u0_SHIFT_clr]
  	ldr		v0, [sp, #v0_SHIFT_clr]
  	ldr		out,[sp, #out_SHIFT_clr]
  	ldr		col,[sp, #col_SHIFT_clr]
		
yuv2rgb565_start 
	ldrb	cr, [v0], #1
	ldrb	y,  [y0], #1
	ldrb	cb, [u0], #1
 		
	sub		cr, cr, #128
	sub		cb, cb, #128
	mul		cr, w1, cr
	mul		cb, w3, cb
	
	add		pix, cb, cb, lsl #2
	add		pix, y, pix, asr #7
	usat	pix, #5, pix, asr #3
		
	add		cb, cb, cr, asr #1
	sub		cb, y,  cb, asr #7
	usat	cb, #6, cb, asr #2
	orr		pix, pix, cb, lsl #5
	
	add		cr, y, cr, asr #7
	usat	cr, #5, cr, asr #3
	orr		pix, pix, cr, lsl #11
  	
	strh	pix, [out], #2
	subs	col, col, #1
	bne		yuv2rgb565_start	
									     
    ldmfd   sp!,{r4-r12,pc} 

  ENDP



src				RN 0
dst				RN 1
const_128_128	RN 2

m10				RN 3
m32				RN 4
m54				RN 5
m76				RN 6
m0				RN 7
m1				RN 8


REGIS_SHIFT_as		EQU	(7*4)
CACHE_SHIFT_as		EQU	(0*4)
SP_SHIFT_as			EQU	(REGIS_SHIFT_as + CACHE_SHIFT_as)
		   
|jpeg_idct_add_sat_arm_v6| PROC
    stmfd sp!,{r3-r8,lr}
 
  	ldr		m10, [src, #0]
  	ldr		m32, [src, #4]
  	ldr		m54, [src, #8]
  	ldr		m76, [src, #12]
	
	sadd16  m10, m10, const_128_128
	sadd16  m32, m32, const_128_128
	sadd16  m54, m54, const_128_128
	sadd16  m76, m76, const_128_128
	
	usat16  m10, #8, m10				;10						     
	usat16  m32, #8, m32				;32				     
	usat16  m54, #8, m54				;54				     
	usat16  m76, #8, m76				;76
	
	pkhtb   m0, m32, m10, asr #16		;31
	pkhbt   m1, m10, m32, lsl #16		;20
	orr		m1, m1, m0, lsl #8
	str		m1, [dst, #0]
	
	pkhtb   m0, m76, m54, asr #16		;75
	pkhbt   m1, m54, m76, lsl #16		;64
	orr		m1, m1, m0, lsl #8
	str		m1, [dst, #4]
	
    ldmfd   sp!,{r3-r8,pc} 

  ENDP
  
  
  
  
  

	END

