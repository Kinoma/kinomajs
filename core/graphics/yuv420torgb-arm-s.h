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
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;yuv420i(interleave)
;features
; downscale: 0.0 ~ 1.0		;all coverage
; upscale  : 1.0 ~ 2.0		;all shapes, edge use generic block
; upscale  : 2.0 ~ 5.0		;reduced shape set, dege use generic block
; upscale  : 5.0 ~ 128.0	;all generic block
; generic  : 0.0 ~			;analog(non-interleave)
; bc	   : brightness and contrast control
; pixformat: 16RGB565SE and 32BGRA
; sprite   : supported except for generic case
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;
;performance historic data
;for reference, may not be up to date!!!!!
;
;scale factor		1.0  0.98 0.90 0.80 0.75 0.60 0.50 0.40 0.33
;analog				 52    55   64   80   90  136  187  283  400
;pattern			 65    69   75   88   95  ???  177  233  306
;pattern interlaced  74    75   86  101  107  142  174  237  306
;unity   interlaced  76   xx
;unity				 72   xx
;
;scale factor		1.0  1.10 1.20 1.33 1.50 1.75 2.0 3.0 4.0
;analog				 52   47   43  37    33  28   22  13   9
;pattern interlaced  74   71    67  61    52  42   31  xx   xx


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

SHIFT_R_rgb565se 	EQU (15)
SHIFT_G_rgb565se 	EQU (14)
SHIFT_B_rgb565se 	EQU (15)
SHIFT_R_bgra32 		EQU (12)
SHIFT_G_bgra32 		EQU (12)
SHIFT_B_bgra32		EQU (12)

BITS_R_rgb565se 	EQU (5)
BITS_G_rgb565se 	EQU (6)
BITS_B_rgb565se 	EQU (5)
BITS_R_bgra32 		EQU (8)
BITS_G_bgra32 		EQU (8)
BITS_B_bgra32		EQU (8)

BLEND_SPRITE_PARAM_BYTES	EQU		(16)
COPY_SPRITE_PARAM_BYTES		EQU		(8)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;	
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	MACRO
		SET_ALPHA_rgb565se $pix, $value
	MEND
  
	MACRO
		SET_ALPHA_bgra32 $pix, $value
	orr	$pix, $pix, #($value<<24)
	MEND


	MACRO
		ldr_rgb565se $a, $b, $c
	ldrh	$a, $b, $c
	MEND

	MACRO
		ldr_bgra32 $a, $b, $c
	ldr	$a, $b, $c
	MEND

	MACRO
		str_rgb565se $a, $b, $c
	strh	$a, $b, $c
	MEND

	MACRO
		str_bgra32 $a, $b, $c
	str	$a, $b, $c
	MEND

	MACRO
		ldrne_rgb565se $a, $b, $c
	ldrneh	$a, $b, $c
	MEND

	MACRO
		ldrne_bgra32 $a, $b, $c
	ldrne	$a, $b, $c
	MEND

	MACRO
		strne_rgb565se $a, $b, $c
	strneh	$a, $b, $c
	MEND

	MACRO
		strne_bgra32 $a, $b, $c
	strne	$a, $b, $c
	MEND

;;;;;;;;;;;;;;;;;;;;;;;;;;;;		
	MACRO
		ALIGN_4 $ppp
;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	add		$ppp, $ppp, #3		;align to long
	mov		$ppp, $ppp, lsr #2
	mov		$ppp, $ppp, lsl #2

	MEND

;;;;;;;;;;;;;;;;;;;;;;;;;;;;		
	MACRO
		COPY_LINE_xx $mark, $pix_format, $pix_bytes
;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	ands   w0, w0, #3
	beq	   $mark.end 
$mark.start
	ldr$pix_format w1, [dst, #$pix_bytes]!
	str$pix_format w1, [w5,  #$pix_bytes]!
	
	subs  w0, w0, #1 
    bne $mark.start	
$mark.end

	MEND

;;;;;;;;;;;;;;;;;;;;;;;;;;;;		
	MACRO
		BLOCK_MEMSET $block_value, $block_width, $block_stride, $block_height, $mark, $pix_format, $pix_bytes
;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	cmp    $block_width,  #0
	beq	   $mark.end
	cmp    $block_height, #0
	beq	   $mark.end
	mov	   xxx, $block_width
$mark.start
	str$pix_format   $block_value, [dst, #$pix_bytes]!  
	subs   xxx, xxx,  #1
	bne	   $mark.start

	subs   $block_height, $block_height, #1
	movne  xxx, $block_width
	add    dst, dst, $block_stride
	bne	   $mark.start
$mark.end
	MEND


;;;;;;;;;;;;;;;;;;;;;;;;;;;;		
	MACRO
		COPY_BUFFER $version, $pix_format, $pix_bytes
;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	cmp    this_width, #0x01
	bne	   $version.generic_start

$version.single_top		
	ldr$pix_format    this_w0, [this_src], #$pix_bytes
	str$pix_format    this_w0, [this_dst], #$pix_bytes
			
	subs    this_height, this_height, #1 
	beq		$version.end
	addne	this_src, this_src, drb
	addne	this_dst, this_dst, drb
	subne	this_src, this_src, this_width, lsl #($pix_bytes>>1)
	subne	this_dst, this_dst, this_width, lsl #($pix_bytes>>1)
	movne	this_width, this_width, lsr #1
    bne     $version.single_top	

$version.generic_start
	mov		this_width, this_width, lsr #1

$version.generic_top		
	ldr$pix_format    this_w0, [this_src], #$pix_bytes
	ldr$pix_format    this_w1, [this_src], #$pix_bytes
	str$pix_format    this_w0, [this_dst], #$pix_bytes
	str$pix_format    this_w1, [this_dst], #$pix_bytes
	
	subs    this_width, this_width, #1 
    bne     $version.generic_top	
	
	;take care of last one
	ldrh    this_width,  [pattern, #0]	
	ands	this_width, this_width, #1
	ldrne$pix_format  this_w0, [this_src], #$pix_bytes
	strne$pix_format  this_w0, [this_dst], #$pix_bytes
	
	subs    this_height, this_height, #1 
	ldrneh  this_width,  [pattern, #0]	
	addne	this_src, this_src, drb
	addne	this_dst, this_dst, drb
	subne	this_src, this_src, this_width, lsl #($pix_bytes>>1)
	subne	this_dst, this_dst, this_width, lsl #($pix_bytes>>1)
	movne	this_width, this_width, lsr #1
    bne     $version.generic_top	

$version.end


	MEND

;;;;;;;;;;;;;;;;;;;;;;;;;;;;		
	MACRO
		BLEND_AND_COPY $version, $pix_format, $pix_bytes, $back_buffer_shift, $front_buffer_shift
;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;local variable
this_width			RN 8	
this_height			RN 9
this_src			RN 10
this_dst			RN 11
this_w0				RN 12
this_w1				RN 14

	ALIGN_4 pattern

	mov		w0, r0
	ldr     r0, [pattern, #4]	
	mov		w1, r1
	mov		w2, r2
	mov		w3, r3
	bl		blend_proc
	mov		r0, w0
	mov		r1, w1
	mov		r2, w2
	mov		r3, w3
	
	ldr this_src, [sp, #$back_buffer_shift]
	ldr this_dst, [sp, #$front_buffer_shift]

	ldrh    this_width,  [pattern, #0]	
	ldrh    this_height, [pattern, #2]	
	
	add		this_dst, this_dst, #$pix_bytes

	COPY_BUFFER	$version, $pix_format, $pix_bytes

	ldrh    this_width,  [pattern, #0]	
	ldr     dst, [sp, #$front_buffer_shift]
	add	    pattern, pattern, #COPY_SPRITE_PARAM_BYTES	;skip width, height
	add		dst, dst, this_width, lsl #($pix_bytes>>1)

	MEND

;;;;;;;;;;;;;;;;;;;;;;;;;;;;		
	MACRO
		GET_PATTERN_INTERLACE $switch, $mark
;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	ldrb	w5, [pattern], #1	
	add		w4, pc, #($switch - $mark)	
	ldr     pc, [w4, w5, lsl #2]	

$mark	
	MEND

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;	
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;;;;;;;;;;;;;;;;;;;;;;;;;;;		
	MACRO
		DOWN_SCALE_PROC_TABLE $version

     DCD	$version.case_common_EOF				;0
     DCD	$version.case_down_EOL_0				;1
     DCD	$version.case_down_SKIP					;4
     DCD	$version.case_down_5					;5
     DCD	$version.case_down_6					;6
     DCD	$version.case_down_EOL_1				;2
     DCD	$version.case_down_7					;7
     DCD	$version.case_down_9					;8
     DCD	$version.case_down_QH_1					;16
     DCD	$version.case_down_QH_2_HOR				;17
     DCD	$version.case_down_QH_2_VER				;18
     DCD	$version.case_down_10					;9
     DCD	$version.case_down_11					;10
     DCD	$version.case_down_EOL_2				;3
     DCD	$version.case_down_13					;11
     DCD	$version.case_down_14					;12
     DCD	$version.case_down_15					;13
     DCD	$version.case_down_reserved_0			;14
     DCD	$version.case_down_reserved_1			;15
     DCD	$version.case_common_Switch_Buffer_S	;19
     DCD	$version.case_common_Copy_Buffer_S		;20
     DCD	$version.case_common_Blend_S			;21

	MEND

;;;;;;;;;;;;;;;;;;;;;;;;;;;;		
	MACRO
		PATTERN_DOWN_SCALE_BC_CONVERSION $arch, $version, $pix_format, $pix_bytes
;;;;;;;;;;;;;;;;;;;;;;;;;;;;

    stmfd   sp!,{r4-r11,lr}
    sub     sp,sp,#CACHE_SHIFT_dspi

    add     w5,sp,#pattern_SHIFT_dspi
    ldmia   w5,{pattern-dst_width_1}			
 	
	mov		w4, drb, asl #1
	sub		w4, w4, dst_width_1, asl #($pix_bytes>>1)
	str		w4, [sp, #dst_double_stride_SHIFT_dspi]	

	ldr		src_width_1, [pattern], #4

	sub		w4, w4, drb
	str		w4, [sp, #dst_single_stride_SHIFT_dspi]	

	add		w4, src_width_1, src_width_1, lsl #1			;yuv_width_bytes = width * 3
	sub		w4, yuvrb_1, w4
	str		w4, [sp, #yuv_double_stride_SHIFT_dspi]	
	
	sub		dst, dst, #$pix_bytes
	SET_CONST$pix_format
		
	GET_PATTERN_INTERLACE $version.switch_band_9, $version.mark_first	

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

$version.case_common_EOF					
    add     sp,sp,#CACHE_SHIFT_dspi
    ldmfd   sp!,{r4-r11,pc} 

$version.case_down_EOL_0					
	ldrb w5, [pattern], #1	
	ldr yuvrb_2, [sp, #yuvrb_SHIFT_dspi]
	add	 w2, pc, #($version.switch_band_9 - $version.mark_EOL_0)	    
	ldr  w2, [w2, w5,lsl #2]	
$version.mark_EOL_0	
	add yuv, yuv, yuvrb_2
	mov	 pc, w2

$version.case_down_SKIP			;00
	ldrb w5, [pattern], #1	
	add	 w2, pc, #($version.switch_band_9 - $version.mark_SKIP)	    
	ldr  w2, [w2, w5, lsl #2]	
$version.mark_SKIP		
	add  yuv, yuv, #6
	mov	 pc, w2

								;00
$version.case_down_5			;01
    ldrb	w0, [yuv], #1		;u
    ldrb	w1, [yuv], #1		;v
    ldrb    w5, [yuv, #3]		;00
	PATTERN_INTERLEAVE_1_in_4 $arch, $version.switch_band_9, $version.pattern_5_mark, $pix_format, $pix_bytes
    add		yuv, yuv, #4
	mov		pc, w2

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;version.switch_band_5
;	DOWN_SCALE_PROC_TABLE $version
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
								;01
$version.case_down_6			;00
    ldrb	w0, [yuv], #1		;u
    ldrb	w1, [yuv], #1		;v
    ldrb    w5, [yuv, #1]		;00
	PATTERN_INTERLEAVE_1_in_4 $arch, $version.switch_band_9, $version.pattern_6_mark, $pix_format, $pix_bytes
    add		yuv, yuv, #4
	mov		pc, w2

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;$version.switch_band_6
;	DOWN_SCALE_PROC_TABLE $version
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

$version.case_down_EOL_1	
	ldrb w5, [pattern], #1	
	ldr yuv_double_stride_2, [sp, #yuv_double_stride_SHIFT_dspi]
	ldr dst_single_stride_2, [sp, #dst_single_stride_SHIFT_dspi]
	add	 w2, pc, #($version.switch_band_9 - $version.mark_EOL_1)	    
	ldr  w2, [w2, w5,lsl #2]	
$version.mark_EOL_1	
	add yuv, yuv, yuv_double_stride_2
	add dst, dst, dst_single_stride_2				
	mov	 pc, w2

								;01
$version.case_down_7			;01
    ldrb	w0, [yuv], #1		;u
    ldrb	w1, [yuv], #1		;v
    ldrb    w5, [yuv, #1]		;yp 
	PATTERN_INTERLEAVE_BASIC_step1 $arch, $pix_format, $pix_bytes
    ldrb    w5, [yuv, #3]		;yp 
	PATTERN_INTERLEAVE_BASIC_step2 $arch, $version.switch_band_9, $version.pattern_7_mark, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, drb]
	add		yuv, yuv, #4
	mov		pc, w2

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;$version.switch_band_7
;	DOWN_SCALE_PROC_TABLE $version
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
								;00
$version.case_down_9			;10 	
    ldrb	w0, [yuv], #1		;u
    ldrb	w1, [yuv], #1		;v
    ldrb    w5, [yuv, #2]		;00
	PATTERN_INTERLEAVE_1_in_4 $arch, $version.switch_band_9, $version.pattern_9_mark, $pix_format, $pix_bytes
    add		yuv, yuv, #4
	mov		pc, w2

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
$version.switch_band_9
	DOWN_SCALE_PROC_TABLE $version
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

$version.case_down_QH_1
    ldrb	w0, [yuv], #1		;u
    ldrb	w1, [yuv], #1		;v
    ldrb    w5, [yuv], #1		
    ldrb    w4, [yuv], #1		
    ldrb    w3, [yuv], #1		
    ldrb    w2, [yuv], #1		
	add		w5, w5, w4
	add		w5, w5, w3
	add		w5, w5, w2
    mov		w5, w5, lsr #2
	PATTERN_INTERLEAVE_1_in_4 $arch, $version.switch_band_9, $version.pattern_QH_1_mark, $pix_format, $pix_bytes
	mov		pc, w2

$version.case_down_QH_2_HOR
	PATTERN_INTERLEAVE_2_in_4_hor_avg $arch, $version.switch_band_9, $version.pattern_QH_2_HOR_mark, $pix_format, $pix_bytes

$version.case_down_QH_2_VER
	PATTERN_INTERLEAVE_2_in_4_ver_avg $arch, $version.switch_band_9, $version.pattern_QH_2_VER_mark, $pix_format, $pix_bytes

								;10
$version.case_down_10			;00
    ldrb	w0, [yuv], #1		;u
    ldrb	w1, [yuv], #1		;v
    ldrb    w5, [yuv, #0]		;00
	PATTERN_INTERLEAVE_1_in_4 $arch, $version.switch_band_9, $version.pattern_10_mark, $pix_format, $pix_bytes
    add		yuv, yuv, #4
	mov		pc, w2

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;$version.switch_band_10
;	DOWN_SCALE_PROC_TABLE $version
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
								;10
$version.case_down_11			;10
    ldrb	w0, [yuv], #1		;u
    ldrb	w1, [yuv], #1		;v
    ldrb    w5, [yuv, #0]		;yp 
	PATTERN_INTERLEAVE_BASIC_step1 $arch, $pix_format, $pix_bytes
    ldrb    w5, [yuv, #2]		;yp 
	PATTERN_INTERLEAVE_BASIC_step2 $arch, $version.switch_band_14, $version.pattern_11_mark, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, drb]
	add		yuv, yuv, #4
	mov		pc, w2

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;$version.switch_band_11
;	DOWN_SCALE_PROC_TABLE $version
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

$version.case_down_EOL_2
	ldr yuv_double_stride_2, [sp, #yuv_double_stride_SHIFT_dspi]
	ldr dst_double_stride_2, [sp, #dst_double_stride_SHIFT_dspi]
	add yuv, yuv, yuv_double_stride_2
	add dst,dst,dst_double_stride_2				
	GET_PATTERN_INTERLACE $version.switch_band_14, $version.mark_after_EOL_2	

								;00
$version.case_down_13			;11
    ldrb	w0, [yuv], #1		;u
    ldrb	w1, [yuv], #3		;v
    ldrb    w5, [yuv], #1		;yp 
	PATTERN_INTERLEAVE_BASIC_step1 $arch, $pix_format, $pix_bytes
    ldrb    w5, [yuv], #1				;yp 
	PATTERN_INTERLEAVE_BASIC_step2 $arch, $version.switch_band_14, $version.pattern_13_mark, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #$pix_bytes]!
	mov		pc, w2

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;$version.switch_band_13
;	DOWN_SCALE_PROC_TABLE $version
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
								;11
$version.case_down_14			;00
    ldrb	w0, [yuv], #1		;u
    ldrb	w1, [yuv], #1		;v
    ldrb    w5, [yuv], #1		;yp 
	PATTERN_INTERLEAVE_BASIC_step1 $arch, $pix_format, $pix_bytes
    ldrb    w5, [yuv], #3		;yp 
	PATTERN_INTERLEAVE_BASIC_step2 $arch, $version.switch_band_14, $version.pattern_14_mark, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #$pix_bytes]!
	mov		pc, w2

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
$version.switch_band_14
	DOWN_SCALE_PROC_TABLE $version
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
									
								;11
$version.case_down_15			;11
	PATTERN_INTERLEAVE_4_in_4 $arch, $version.switch_band_14, $version.mark_case_kDownScalePattern_15, $pix_format, $pix_bytes

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;$version.switch_band_15
;	DOWN_SCALE_PROC_TABLE $version
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	
$version.case_down_reserved_0
$version.case_down_reserved_1
	GET_PATTERN_INTERLACE $version.switch_band_14, $version.case_down_reserved_mark	

$version.case_common_Switch_Buffer_S
	str		dst, [sp, #sprite_front_buffer_SHIFT_dspi]
	ldr		dst, [sp, #sprite_back_buffer_SHIFT_dspi]
	sub		dst, dst, #$pix_bytes
	
	GET_PATTERN_INTERLACE $version.switch_band_14, $version.case_common_Switch_Buffer_S_mark	

$version.case_common_Copy_Buffer_S
	BLEND_AND_COPY $version.blend_and_copy, $pix_format, $pix_bytes, sprite_back_buffer_SHIFT_dspi, sprite_front_buffer_SHIFT_dspi
	GET_PATTERN_INTERLACE $version.switch_band_14, $version.case_common_Copy_Buffer_S_mark	

$version.case_common_Blend_S
	ALIGN_4 pattern
	mov		w0, r0
	mov		w1, r1
	mov		w2, r2
	mov		w3, r3
	mov		r0, pattern
	bl		parse_proc
	mov		r0, w0
	mov		r1, w1
	mov		r2, w2
	mov		r3, w3
	
	add	    pattern, pattern, #BLEND_SPRITE_PARAM_BYTES	;skip width, height

	GET_PATTERN_INTERLACE $version.switch_band_14, $version.case_common_Pattern_Blend_S_mark	

	MEND

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;	
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


	MACRO
		UP_SCALE_PROC_TABLE $version

     DCD	$version.case_common_EOF			;0
     DCD	$version.case_up_1				
     DCD	$version.case_up_2				
     DCD	$version.case_up_3				
     DCD	$version.case_up_4				
     DCD	$version.case_up_5				
     DCD	$version.case_up_6				
     DCD	$version.case_up_7				
     DCD	$version.case_up_8				
     DCD	$version.case_up_9				
     DCD	$version.case_up_10			
     DCD	$version.case_up_11			
     DCD	$version.case_up_12			
     DCD	$version.case_up_13			
     DCD	$version.case_up_14			
     DCD	$version.case_up_15			
     DCD	$version.case_up_16_4x4
	 DCD	$version.case_up_reserved_0			;17
	 DCD	$version.case_up_reserved_1			;18
     DCD	$version.case_common_Switch_Buffer	;19
     DCD	$version.case_common_Copy_Buffer	;20
     DCD	$version.case_common_Blend 			;21
     DCD	$version.case_up_EOL_2				;22
     DCD	$version.case_up_EOL_3				;23
     DCD	$version.case_up_EOL_4				;24
     DCD	$version.case_up_GENERIC_EOL		;25
     DCD	$version.case_up_GENERIC_BLOCK		;26
     DCD	$version.case_up_4x5				;27
     DCD	$version.case_up_4x6 
     DCD	$version.case_up_5x4
     DCD	$version.case_up_5x5
     DCD	$version.case_up_5x6
     DCD	$version.case_up_6x4 
     DCD	$version.case_up_6x5
     DCD	$version.case_up_6x6
     DCD	$version.case_up_6x7
     DCD	$version.case_up_6x8 
     DCD	$version.case_up_7x6
     DCD	$version.case_up_7x7
     DCD	$version.case_up_7x8
     DCD	$version.case_up_8x6 
     DCD	$version.case_up_8x7
     DCD	$version.case_up_8x8
     DCD	$version.case_up_8x9
     DCD	$version.case_up_8x10
     DCD	$version.case_up_9x8
     DCD	$version.case_up_9x9
     DCD	$version.case_up_9x10
	 DCD	$version.case_up_10x8
     DCD	$version.case_up_10x9
     DCD	$version.case_up_10x10				;50
						  
	MEND				  
						  
						  

;;;;;;;;;;;;;;;;;;;;;;;;;;;;		
	MACRO
		PATTERN_UP_SCALE_BC_CONVERSION $arch, $version, $pix_format, $pix_bytes
;;;;;;;;;;;;;;;;;;;;;;;;;;;;

    stmfd   sp!,{r4-r11,lr}
    sub     sp,sp,#CACHE_SHIFT_usxxpi

    add     w5,sp,#pattern_SHIFT_usxxpi
    ldmia   w5,{pattern-dst_width_1}			
 	
	sub		w4, drb, dst_width_1, lsl #($pix_bytes>>1)
	str		w4, [sp, #dst_single_stride_SHIFT_usxxpi]	

	add		w4, w4, drb
	str		w4, [sp, #dst_double_stride_SHIFT_usxxpi]	

	add		w4, w4, drb
	str		w4, [sp, #dst_tripple_stride_SHIFT_usxxpi]	

	add		w4, w4, drb
	str		w4, [sp, #dst_quad_stride_SHIFT_usxxpi]	

	ldr		src_width_1, [pattern], #4

	add		w4, src_width_1, src_width_1, lsl #1			;yuv_width_bytes = width * 3
	sub		w4, yuvrb_1, w4
	str		w4, [sp, #yuv_double_stride_SHIFT_usxxpi]	
	
	sub		dst, dst, #$pix_bytes

	SET_CONST$pix_format
		
	GET_PATTERN_INTERLACE $version.switch_band_2, $version.first_mark	

;;;;;;;;;;;;;;;;;;

$version.case_common_EOF					
    add     sp,sp,#CACHE_SHIFT_usxxpi
    ldmfd   sp!,{r4-r11,pc} 
 

						;ab 
$version.case_up_1		;cd
							PATTERN_upscale_step_1 $arch, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #$pix_bytes]!
							PATTERN_upscale_step_2 $arch, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #$pix_bytes]
							PATTERN_upscale_step_3 $arch, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, drb]
							PATTERN_upscale_step_4 $arch, $version.switch_band_2, $version.pattern_1_mark, $pix_format, $pix_bytes
	add		dst, dst, #$pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, drb]
	
	mov		pc, w2

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;$version.switch_band_1
;	UP_SCALE_PROC_TABLE $version
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


						;ab
$version.case_up_2		;cd	
						;cd
							PATTERN_upscale_step_1 $arch, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #$pix_bytes]!
							PATTERN_upscale_step_2 $arch, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #$pix_bytes]
							PATTERN_upscale_step_3 $arch, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, drb]
							PATTERN_upscale_step_4 $arch, $version.switch_band_2, $version.pattern_2_mark, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #$pix_bytes]!
	str$pix_format	w3, [dst, drb]

	sub		dst, dst, drb
	
	mov		pc, w2

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
$version.switch_band_2
	UP_SCALE_PROC_TABLE $version
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

						;ab
						;ab
$version.case_up_3	;cd
							PATTERN_upscale_step_1 $arch, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #$pix_bytes]!
	str$pix_format	w3, [dst, drb]
							PATTERN_upscale_step_2 $arch, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #$pix_bytes]!
	str$pix_format	w3, [dst, drb]
							PATTERN_upscale_step_3 $arch, $pix_format, $pix_bytes
	sub		dst, dst, #$pix_bytes
	add		dst, dst, drb, lsl #1;
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst], #$pix_bytes	
	
							PATTERN_upscale_step_4 $arch, $version.switch_band_2, $version.pattern_3_mark, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst], #0
	sub		dst, dst, drb, lsl #1;
	
	mov		pc, w2

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;$version.switch_band_3
;	UP_SCALE_PROC_TABLE $version
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

						;ab
						;ab
$version.case_up_4		;cd
						;cd
							PATTERN_upscale_step_1 $arch, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #$pix_bytes]!
	str$pix_format	w3, [dst, drb]
							PATTERN_upscale_step_2 $arch, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #$pix_bytes]!
	str$pix_format	w3, [dst, drb]
							PATTERN_upscale_step_3 $arch, $pix_format, $pix_bytes
	sub		dst, dst, #($pix_bytes<<1)
	add		dst, dst, drb, lsl #1;
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #$pix_bytes]!	
	str$pix_format	w3, [dst, drb]
							PATTERN_upscale_step_4 $arch, $version.switch_band_2, $version.pattern_4_mark, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #$pix_bytes]!
	str$pix_format	w3, [dst, drb]
	sub		dst, dst, drb, lsl #1;
	
	mov		pc, w2

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;$version.switch_band_4
;	UP_SCALE_PROC_TABLE $version
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

						;abb
$version.case_up_5		;cdd
							PATTERN_upscale_step_1 $arch, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #$pix_bytes]!
							PATTERN_upscale_step_2 $arch, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #$pix_bytes]
	str$pix_format	w3, [dst, #($pix_bytes<<1)]
							PATTERN_upscale_step_3 $arch, $pix_format, $pix_bytes
	add		dst, dst, drb
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst], #$pix_bytes
							PATTERN_upscale_step_4 $arch, $version.switch_band_7, $version.pattern_5_mark, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst], #$pix_bytes
	str$pix_format	w3, [dst], #0
	sub		dst, dst, drb
	
	mov		pc, w2

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;$version.switch_band_5
;	UP_SCALE_PROC_TABLE $version
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

						;abb
$version.case_up_6		;cdd
						;cdd
							PATTERN_upscale_step_1 $arch, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #$pix_bytes]!
							PATTERN_upscale_step_2 $arch, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #$pix_bytes]
	str$pix_format	w3, [dst, #($pix_bytes<<1)]
							PATTERN_upscale_step_3 $arch, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, drb]
							PATTERN_upscale_step_4 $arch, $version.switch_band_7, $version.pattern_6_mark, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #$pix_bytes]!
	str$pix_format	w3, [dst, drb]
	str$pix_format	w3, [dst, #$pix_bytes]!
	str$pix_format	w3, [dst, drb]
	sub		dst, dst, drb
	
	mov		pc, w2

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;$version.switch_band_6
;	UP_SCALE_PROC_TABLE $version
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

						;abb
						;abb
$version.case_up_7		;cdd
							PATTERN_upscale_step_1 $arch, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #$pix_bytes]!
	str$pix_format	w3, [dst, drb]
							PATTERN_upscale_step_2 $arch, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #$pix_bytes]!
	str$pix_format	w3, [dst, drb]
	str$pix_format	w3, [dst, #$pix_bytes]!
	str$pix_format	w3, [dst, drb]
							PATTERN_upscale_step_3 $arch, $pix_format, $pix_bytes
	sub		dst, dst, #($pix_bytes<<1)
	add		dst, dst, drb, lsl #1;
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst], #$pix_bytes
							PATTERN_upscale_step_4 $arch, $version.switch_band_7, $version.pattern_7_mark, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst], #$pix_bytes
	str$pix_format	w3, [dst], #0
	sub		dst, dst, drb, lsl #1;
	
	mov		pc, w2

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
$version.switch_band_7
	UP_SCALE_PROC_TABLE $version
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

						;abb
						;abb
$version.case_up_8		;cdd
						;cdd
							PATTERN_upscale_step_1 $arch, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #$pix_bytes]!
	str$pix_format	w3, [dst, drb]
							PATTERN_upscale_step_2 $arch, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #$pix_bytes]!
	str$pix_format	w3, [dst, drb]
	str$pix_format	w3, [dst, #$pix_bytes]!
	str$pix_format	w3, [dst, drb]
							PATTERN_upscale_step_3 $arch, $pix_format, $pix_bytes
	sub		dst, dst, #($pix_bytes*3)
	add		dst, dst, drb, lsl #1;
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #$pix_bytes]!
	str$pix_format	w3, [dst, drb]
							PATTERN_upscale_step_4 $arch, $version.switch_band_7, $version.pattern_8_mark, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #$pix_bytes]!
	str$pix_format	w3, [dst, drb]
	str$pix_format	w3, [dst, #$pix_bytes]!
	str$pix_format	w3, [dst, drb]
	sub		dst, dst, drb, lsl #1;
	
	mov		pc, w2

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;$version.switch_band_8
;	UP_SCALE_PROC_TABLE $version
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	
						;aab
$version.case_up_9		;ccd
							PATTERN_upscale_step_1 $arch, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #$pix_bytes]!
	str$pix_format	w3, [dst, #$pix_bytes]
							PATTERN_upscale_step_2 $arch, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #($pix_bytes<<1)]
							PATTERN_upscale_step_3 $arch, $pix_format, $pix_bytes
	add		dst, dst, drb
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst], #$pix_bytes
	str$pix_format	w3, [dst], #$pix_bytes
							PATTERN_upscale_step_4 $arch, $version.switch_band_7, $version.pattern_9_mark, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst], #0
	sub		dst, dst, drb
 	
	mov		pc, w2

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;$version.switch_band_9
;	UP_SCALE_PROC_TABLE $version
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

						;aab
$version.case_up_10		;ccd
						;ccd
							PATTERN_upscale_step_1 $arch, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #$pix_bytes]!
	str$pix_format	w3, [dst, #$pix_bytes]
							PATTERN_upscale_step_2 $arch, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #($pix_bytes<<1)]
							PATTERN_upscale_step_3 $arch, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, drb]
	str$pix_format	w3, [dst, #$pix_bytes]!
	str$pix_format	w3, [dst, drb]	
							PATTERN_upscale_step_4 $arch, $version.switch_band_11, $version.pattern_10_mark, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #$pix_bytes]!
	str$pix_format	w3, [dst, drb]
	sub		dst, dst, drb
 	
	mov		pc, w2

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;$version.switch_band_10
;	UP_SCALE_PROC_TABLE $version
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
						;aab
						;aab
$version.case_up_11		;ccd
							PATTERN_upscale_step_1 $arch, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #$pix_bytes]!
	str$pix_format	w3, [dst, drb]
	str$pix_format	w3, [dst, #$pix_bytes]!
	str$pix_format	w3, [dst, drb]
							PATTERN_upscale_step_2 $arch, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #$pix_bytes]!
	str$pix_format	w3, [dst, drb]
							PATTERN_upscale_step_3 $arch, $pix_format, $pix_bytes
	sub		dst, dst, #($pix_bytes<<1)
	add		dst, dst, drb, lsl #1;
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst], #$pix_bytes
	str$pix_format	w3, [dst], #$pix_bytes
							PATTERN_upscale_step_4 $arch, $version.switch_band_11, $version.pattern_11_mark, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst], #0
	sub		dst, dst, drb, lsl #1;

	mov		pc, w2

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
$version.switch_band_11
	UP_SCALE_PROC_TABLE $version
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

							
						;aab
						;aab
$version.case_up_12		;ccd
						;ccd
							PATTERN_upscale_step_1 $arch, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #$pix_bytes]!
	str$pix_format	w3, [dst, drb]
	str$pix_format	w3, [dst, #$pix_bytes]!
	str$pix_format	w3, [dst, drb]
							PATTERN_upscale_step_2 $arch, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #$pix_bytes]!
	str$pix_format	w3, [dst, drb]
							PATTERN_upscale_step_3 $arch, $pix_format, $pix_bytes
	sub		dst, dst, #($pix_bytes*3)
	add		dst, dst, drb, lsl #1;
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #$pix_bytes]!	
	str$pix_format	w3, [dst, drb]
	str$pix_format	w3, [dst, #$pix_bytes]!	
	str$pix_format	w3, [dst, drb]
							PATTERN_upscale_step_4 $arch, $version.switch_band_11, $version.pattern_12_mark, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #$pix_bytes]!	
	str$pix_format	w3, [dst, drb]
	sub		dst, dst, drb, lsl #1;

	mov		pc, w2

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;$version.switch_band_12
;	UP_SCALE_PROC_TABLE $version
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


						;aabb
$version.case_up_13		;ccdd 	
							PATTERN_upscale_step_1 $arch, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #$pix_bytes]!
	str$pix_format	w3, [dst, #$pix_bytes]
							PATTERN_upscale_step_2 $arch, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #($pix_bytes<<1)]
	str$pix_format	w3, [dst, #($pix_bytes*3)]
							PATTERN_upscale_step_3 $arch, $pix_format, $pix_bytes
	add		dst, dst, drb
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst], #$pix_bytes
	str$pix_format	w3, [dst], #$pix_bytes
							PATTERN_upscale_step_4 $arch, $version.switch_band_11, $version.pattern_13_mark, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst], #$pix_bytes
	str$pix_format	w3, [dst], #0
	sub		dst, dst, drb
	
	mov		pc, w2

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;$version.switch_band_13
;	UP_SCALE_PROC_TABLE $version
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

$version.case_up_14		;aabb
						;ccdd
						;ccdd
							PATTERN_upscale_step_1 $arch, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #$pix_bytes]!
	str$pix_format	w3, [dst, #$pix_bytes]
							PATTERN_upscale_step_2 $arch, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #($pix_bytes<<1)]
	str$pix_format	w3, [dst, #($pix_bytes*3)]
							PATTERN_upscale_step_3 $arch, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, drb]
	str$pix_format	w3, [dst, #$pix_bytes]!
	str$pix_format	w3, [dst, drb]
							PATTERN_upscale_step_4 $arch, $version.switch_band_15, $version.pattern_14_mark, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #$pix_bytes]!
	str$pix_format	w3, [dst, drb]
	str$pix_format	w3, [dst, #$pix_bytes]!
	str$pix_format	w3, [dst, drb]
	sub		dst, dst, drb
	
	mov		pc, w2

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;$version.switch_band_14
;	UP_SCALE_PROC_TABLE $version
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

						;aabb
						;aabb
$version.case_up_15		;ccdd
							PATTERN_upscale_step_1 $arch, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #$pix_bytes]!
	str$pix_format	w3, [dst, drb]
	str$pix_format	w3, [dst, #$pix_bytes]!
	str$pix_format	w3, [dst, drb]
							PATTERN_upscale_step_2 $arch, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #$pix_bytes]!
	str$pix_format	w3, [dst, drb]
	str$pix_format	w3, [dst, #$pix_bytes]!
	str$pix_format	w3, [dst, drb]
							PATTERN_upscale_step_3 $arch, $pix_format, $pix_bytes
	sub		dst, dst, #($pix_bytes*3)
	add		dst, dst, drb, lsl #1;
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst], #$pix_bytes
	str$pix_format	w3, [dst], #$pix_bytes
							PATTERN_upscale_step_4 $arch, $version.switch_band_15, $version.pattern_15_mark, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst], #$pix_bytes
	str$pix_format	w3, [dst], #0
	sub		dst, dst, drb, lsl #1;
	
	mov		pc, w2

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
$version.switch_band_15
	UP_SCALE_PROC_TABLE $version
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

						;aabb
						;aabb
$version.case_up_16_4x4	;ccdd
						;ccdd
							PATTERN_upscale_step_1 $arch, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #$pix_bytes]!
	str$pix_format	w3, [dst, drb]
	str$pix_format	w3, [dst, #$pix_bytes]!
	str$pix_format	w3, [dst, drb]
							PATTERN_upscale_step_2 $arch, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #$pix_bytes]!
	str$pix_format	w3, [dst, drb]
	str$pix_format	w3, [dst, #$pix_bytes]!
	str$pix_format	w3, [dst, drb]
							PATTERN_upscale_step_3 $arch, $pix_format, $pix_bytes
	sub		dst, dst, #($pix_bytes<<2)
	add		dst, dst, drb, lsl #1;
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #$pix_bytes]!
	str$pix_format	w3, [dst, drb]
	str$pix_format	w3, [dst, #$pix_bytes]!
	str$pix_format	w3, [dst, drb]
							PATTERN_upscale_step_4 $arch, $version.switch_band_15, $version.pattern_16_mark, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #$pix_bytes]!
	str$pix_format	w3, [dst, drb]
	str$pix_format	w3, [dst, #$pix_bytes]!
	str$pix_format	w3, [dst, drb]
	sub		dst, dst, drb, lsl #1;
	
	mov		pc, w2

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;$version.switch_band_16
;	UP_SCALE_PROC_TABLE $version
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

$version.case_up_reserved_0					
$version.case_up_reserved_1					
	GET_PATTERN_INTERLACE $version.switch_band_15, $version.case_reserved_not_implemented_mark


$version.case_common_Switch_Buffer
	str		dst, [sp, #sprite_front_buffer_SHIFT_usxxpi]
	ldr		dst, [sp, #sprite_back_buffer_SHIFT_usxxpi]
	sub		dst, dst, #$pix_bytes
	
	GET_PATTERN_INTERLACE $version.switch_band_15, $version.case_common_Switch_Buffer_S_mark	

$version.case_common_Copy_Buffer
	BLEND_AND_COPY $version.blend_and_copy, $pix_format, $pix_bytes, sprite_back_buffer_SHIFT_usxxpi, sprite_front_buffer_SHIFT_usxxpi
	GET_PATTERN_INTERLACE $version.switch_band_15, $version.Pattern_Copy_Buffer_S_mark	

$version.case_common_Blend
	ALIGN_4 pattern
	mov		w0, r0
	mov		w1, r1
	mov		w2, r2
	mov		w3, r3
	mov		r0, pattern
	bl		parse_proc
	mov		r0, w0
	mov		r1, w1
	mov		r2, w2
	mov		r3, w3
	
	add	    pattern, pattern, #BLEND_SPRITE_PARAM_BYTES	;skip width, height

	GET_PATTERN_INTERLACE $version.switch_band_15, $version.Pattern_Blend_S_mark	

$version.case_up_EOL_2					
$version.case_up_EOL_3					
$version.case_up_EOL_4					
	GET_PATTERN_INTERLACE $version.switch_band_15, $version.case_EOL_not_implemented_mark

$version.case_up_GENERIC_EOL					
repeats_x	RN 10

	ldrb repeats_x, [pattern], #1	
	ldr yuv_double_stride_2, [sp, #yuv_double_stride_SHIFT_usxxpi]
	ldr dst_single_stride_2, [sp, #dst_single_stride_SHIFT_usxxpi]
	add yuv, yuv, yuv_double_stride_2
	add dst,dst,dst_single_stride_2
	mla dst, repeats_x, drb, dst
					
	GET_PATTERN_INTERLACE $version.switch_band_15, $version.case_GENERIC_EOL_mark

$version.case_up_GENERIC_BLOCK			;26
	
	;a
							PATTERN_upscale_step_1 $arch, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [sp, #(generic_block_cache_SHIFT_usxxpi + (0*$pix_bytes))]

	;b
							PATTERN_upscale_step_2 $arch, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [sp, #(generic_block_cache_SHIFT_usxxpi + (1*$pix_bytes))]

	;c
							PATTERN_upscale_step_3 $arch, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [sp, #(generic_block_cache_SHIFT_usxxpi + (2*$pix_bytes))]

	;d
							PATTERN_upscale_step_f $arch, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [sp, #(generic_block_cache_SHIFT_usxxpi + (3*$pix_bytes))]

block_value			RN 8	
block_width			RN 9
block_stride		RN 10
block_height		RN 11
dst_cache			RN 12
xxx					RN 14

	;01
	;23
	mov	 dst_cache, dst
	ldr$pix_format block_value, [sp, #(generic_block_cache_SHIFT_usxxpi + (0*$pix_bytes))]
	ldrb block_width,  [pattern, #0]	;x0<=x0,x1,y0,y1
	ldrb block_height, [pattern, #2]	;y0<=x0,x1,y0,y1
	sub	 block_stride, drb, block_width, lsl #($pix_bytes>>1)
	BLOCK_MEMSET block_value, block_width, block_stride, block_height, $version.case_up_GENERIC_BLOCK_mark_0, $pix_format, $pix_bytes

	ldr$pix_format block_value, [sp, #(generic_block_cache_SHIFT_usxxpi + (2*$pix_bytes))]
	ldrb block_height, [pattern, #3]	;y1<=x0,x1,y0,y1
	BLOCK_MEMSET block_value, block_width, block_stride, block_height, $version.case_up_GENERIC_BLOCK_mark_2, $pix_format, $pix_bytes

	add  dst, dst_cache, block_width, lsl #($pix_bytes>>1)
	ldr$pix_format block_value, [sp, #(generic_block_cache_SHIFT_usxxpi + (1*$pix_bytes))]
	ldrb block_width,  [pattern, #1]	;x1<=x0,x1,y0,y1
	ldrb block_height, [pattern, #2]	;y0<=x0,x1,y0,y1
	sub	 block_stride, drb, block_width, lsl #($pix_bytes>>1)
	BLOCK_MEMSET block_value, block_width, block_stride, block_height, $version.case_up_GENERIC_BLOCK_mark_1, $pix_format, $pix_bytes

	ldr$pix_format block_value, [sp, #(generic_block_cache_SHIFT_usxxpi + (3*$pix_bytes))]
	ldrb block_height, [pattern, #3]	;y1<=x0,x1,y0,y1
	BLOCK_MEMSET block_value, block_width, block_stride, block_height, $version.case_up_GENERIC_BLOCK_mark_3, $pix_format, $pix_bytes

	ldrb xxx,  [pattern, #0]								;x0
	add	 pattern, pattern, #4
	add	dst, dst_cache, block_width, lsl #($pix_bytes>>1)	;x1
	add	dst, dst,       xxx        , lsl #($pix_bytes>>1)

	GET_PATTERN_INTERLACE $version.switch_band_4x5, $version.case_generic_block_mark


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;$version.switch_band_00
;	UP_SCALE_PROC_TABLE $version
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

						;aabb
						;aabb
						;aabb		
						;ccdd
$version.case_up_4x5	;ccdd
	;a23
							PATTERN_upscale_step_1 $arch, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #(1*$pix_bytes)]!			;first line of sub-block
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	
	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	
	sub	dst, dst, drb, lsl #1

	;b23
							PATTERN_upscale_step_2 $arch, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #(2*$pix_bytes)]!		;first line of sub-block
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	
	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]

	add	dst, dst, drb

	;c22
							PATTERN_upscale_step_3 $arch, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #(-2*$pix_bytes)]!	;first line of sub-block
	str$pix_format	w3, [dst, #( 1*$pix_bytes)]
	
	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]

	sub	dst, dst, drb

	;d22
							PATTERN_upscale_step_4 $arch, $version.switch_band_4x5, $version.pattern_4x5_mark, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #(2*$pix_bytes)]!		;first line of sub-block
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	
	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]!

	sub	dst, dst, drb, lsl #2
	
	mov		pc, w2

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
$version.switch_band_4x5
	UP_SCALE_PROC_TABLE $version
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

						;aabb
						;aabb
						;aabb		
						;ccdd
						;ccdd
$version.case_up_4x6	;ccdd
	;a23
							PATTERN_upscale_step_1 $arch, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #(1*$pix_bytes)]!			;first line of sub-block
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	
	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	
	sub	dst, dst, drb, lsl #1

	;b23
							PATTERN_upscale_step_2 $arch, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #(2*$pix_bytes)]!		;first line of sub-block
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	
	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]

	add	dst, dst, drb

	;c23
							PATTERN_upscale_step_3 $arch, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #(-2*$pix_bytes)]!	;first line of sub-block
	str$pix_format	w3, [dst, #( 1*$pix_bytes)]
	
	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]

	sub	dst, dst, drb, lsl #1

	;d23
							PATTERN_upscale_step_4 $arch, $version.switch_band_4x5, $version.pattern_4x6_mark, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #(2*$pix_bytes)]!		;first line of sub-block
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	
	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]!

	sub	dst, dst, drb, lsl #2
	sub	dst, dst, drb
	
	mov		pc, w2

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;$version.switch_band_4x6
;	UP_SCALE_PROC_TABLE $version
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

						;aaabb
						;aaabb		
						;cccdd
$version.case_up_5x4  	;cccdd
 
	;a32
							PATTERN_upscale_step_1 $arch, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #$pix_bytes]!			;first line of sub-block
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	
	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	
	sub	dst, dst, drb

	;b22
							PATTERN_upscale_step_2 $arch, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #(3*$pix_bytes)]!		;first line of sub-block
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	
	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]

	add	dst, dst, drb

	;c32
							PATTERN_upscale_step_3 $arch, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #(-3*$pix_bytes)]!	;first line of sub-block
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	
	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]

	sub	dst, dst, drb

	;d22
							PATTERN_upscale_step_4 $arch, $version.switch_band_5x5, $version.pattern_5x4_mark, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #(3*$pix_bytes)]!		;first line of sub-block
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	
	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]!

	sub		dst, dst, drb, lsl #2
	add	dst, dst, drb
	
	mov		pc, w2

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;$version.switch_band_5x4
;	UP_SCALE_PROC_TABLE $version
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

						;aaabb
						;aaabb
						;aaabb		
						;cccdd
$version.case_up_5x5  	;cccdd
 
	;a33
							PATTERN_upscale_step_1 $arch, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #$pix_bytes]!			;first line of sub-block
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	
	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	
	sub	dst, dst, drb, lsl #1

	;b23
							PATTERN_upscale_step_2 $arch, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #(3*$pix_bytes)]!		;first line of sub-block
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	
	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]

	add	dst, dst, drb

	;c32
							PATTERN_upscale_step_3 $arch, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #(-3*$pix_bytes)]!	;first line of sub-block
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	
	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]

	sub	dst, dst, drb

	;d22
							PATTERN_upscale_step_4 $arch, $version.switch_band_5x5, $version.pattern_5x5_mark, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #(3*$pix_bytes)]!		;first line of sub-block
	str$pix_format	w3, [dst, #$pix_bytes]
	
	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #$pix_bytes]!

	sub		dst, dst, drb, lsl #2
	
	mov		pc, w2

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
$version.switch_band_5x5
	UP_SCALE_PROC_TABLE $version
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

						;aaabb
						;aaabb
						;aaabb		
						;cccdd
						;cccdd
$version.case_up_5x6  	;cccdd
	;a33
							PATTERN_upscale_step_1 $arch, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #$pix_bytes]!			;first line of sub-block
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	
	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	
	sub	dst, dst, drb, lsl #1

	;b23
							PATTERN_upscale_step_2 $arch, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #(3*$pix_bytes)]!		;first line of sub-block
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	
	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]

	add	dst, dst, drb
	
	;c33
							PATTERN_upscale_step_3 $arch, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #(-3*$pix_bytes)]!	;first line of sub-block
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	
	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]

	sub	dst, dst, drb, lsl #1

	;d23
							PATTERN_upscale_step_4 $arch, $version.switch_band_5x5, $version.pattern_5x6_mark, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #(3*$pix_bytes)]!		;first line of sub-block
	str$pix_format	w3, [dst, #$pix_bytes]
	
	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #$pix_bytes]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #$pix_bytes]!

	sub		dst, dst, drb, lsl #2
	sub		dst, dst, drb
	
	mov		pc, w2


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;$version.switch_band_5x6
;	UP_SCALE_PROC_TABLE $version
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;



						;aaabbb
						;aaabbb
						;aaabbb		
						;cccddd
$version.case_up_6x5  	;cccddd
 
	;a33
							PATTERN_upscale_step_1 $arch, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #(1*$pix_bytes)]!			;first line of sub-block
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	
	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	
	sub	dst, dst, drb, lsl #1

	;b33
							PATTERN_upscale_step_2 $arch, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #(3*$pix_bytes)]!		;first line of sub-block
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	
	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]

	add	dst, dst, drb

	;c32
							PATTERN_upscale_step_3 $arch, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #(-3*$pix_bytes)]!	;first line of sub-block
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	
	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]

	sub	dst, dst, drb

	;d32
							PATTERN_upscale_step_4 $arch, $version.switch_band_6x4, $version.pattern_6x5_mark, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #(3*$pix_bytes)]!		;first line of sub-block
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	
	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]!

	sub		dst, dst, drb, lsl #2
	
	mov		pc, w2


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;$version.switch_band_6x5
;	UP_SCALE_PROC_TABLE $version
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


						;aaabbb
						;aaabbb		
						;cccddd
$version.case_up_6x4  	;cccddd
 
	;a33
							PATTERN_upscale_step_1 $arch, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #(1*$pix_bytes)]!			;first line of sub-block
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	
	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	
	sub	dst, dst, drb

	;b33
							PATTERN_upscale_step_2 $arch, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #(3*$pix_bytes)]!		;first line of sub-block
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	
	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]

	add	dst, dst, drb

	;c32
							PATTERN_upscale_step_3 $arch, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #(-3*$pix_bytes)]!	;first line of sub-block
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	
	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]

	sub	dst, dst, drb

	;d32
							PATTERN_upscale_step_4 $arch, $version.switch_band_6x4, $version.pattern_6x4_mark, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #(3*$pix_bytes)]!		;first line of sub-block
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	
	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]!

	sub		dst, dst, drb, lsl #2
	add		dst, dst, drb
	
	mov		pc, w2

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
$version.switch_band_6x4
	UP_SCALE_PROC_TABLE $version
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

						;aaabbb
						;aaabbb
						;aaabbb		
						;cccddd
						;cccddd
$version.case_up_6x6  	;cccddd
 
	;a33
							PATTERN_upscale_step_1 $arch, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #(1*$pix_bytes)]!			;first line of sub-block
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	
	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	
	sub	dst, dst, drb, lsl #1

	;b33
							PATTERN_upscale_step_2 $arch, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #(3*$pix_bytes)]!		;first line of sub-block
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	
	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]

	add	dst, dst, drb

	;c33
							PATTERN_upscale_step_3 $arch, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #(-3*$pix_bytes)]!	;first line of sub-block
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	
	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]

	sub	dst, dst, drb, lsl #1

	;d33
							PATTERN_upscale_step_4 $arch, $version.switch_band_6x4, $version.pattern_6x6_mark, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #(3*$pix_bytes)]!		;first line of sub-block
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	
	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]!

	sub		dst, dst, drb, lsl #2
	sub		dst, dst, drb
	
	mov		pc, w2

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;$version.switch_band_6x6
;	UP_SCALE_PROC_TABLE $version
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

						;aaabbb
						;aaabbb
						;aaabbb		
						;aaabbb		
						;cccddd
						;cccddd
$version.case_up_6x7  	;cccddd
 
	;a34
							PATTERN_upscale_step_1 $arch, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #(1*$pix_bytes)]!			;first line of sub-block
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	
	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	
	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	
	sub	dst, dst, drb, lsl #1
	sub	dst, dst, drb

	;b34
							PATTERN_upscale_step_2 $arch, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #(3*$pix_bytes)]!		;first line of sub-block
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	
	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]

	add	dst, dst, drb

	;c33
							PATTERN_upscale_step_3 $arch, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #(-3*$pix_bytes)]!	;first line of sub-block
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	
	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]

	sub	dst, dst, drb, lsl #1

	;d33
							PATTERN_upscale_step_4 $arch, $version.switch_band_6x8, $version.pattern_6x7_mark, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #(3*$pix_bytes)]!		;first line of sub-block
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	
	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]!

	sub		dst, dst, drb, lsl #2
	sub		dst, dst, drb, lsl #1
	
	mov		pc, w2

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;$version.switch_band_6x7
;	UP_SCALE_PROC_TABLE $version
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

						;aaabbb
						;aaabbb
						;aaabbb		
						;aaabbb		
						;cccddd
						;cccddd
						;cccddd
$version.case_up_6x8  	;cccddd
 
	;a34
							PATTERN_upscale_step_1 $arch, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #(1*$pix_bytes)]!			;first line of sub-block
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	
	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	
	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	
	sub	dst, dst, drb, lsl #1
	sub	dst, dst, drb

	;b34
							PATTERN_upscale_step_2 $arch, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #(3*$pix_bytes)]!		;first line of sub-block
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	
	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]

	add	dst, dst, drb

	;c33
							PATTERN_upscale_step_3 $arch, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #(-3*$pix_bytes)]!	;first line of sub-block
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	
	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]

	sub	dst, dst, drb, lsl #1
	sub	dst, dst, drb

	;d33
							PATTERN_upscale_step_4 $arch, $version.switch_band_6x8, $version.pattern_6x8_mark, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #(3*$pix_bytes)]!		;first line of sub-block
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	
	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]!

	sub		dst, dst, drb, lsl #3
	add		dst, dst, drb
	
	mov		pc, w2

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
$version.switch_band_6x8
	UP_SCALE_PROC_TABLE $version
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

						;aaaabbb
						;aaaabbb		
						;aaaabbb		
						;ccccddd
						;ccccddd
$version.case_up_7x6  	;ccccddd
 
	;a43
							PATTERN_upscale_step_1 $arch, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #(1*$pix_bytes)]!			;first line of sub-block
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	
	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	
	sub	dst, dst, drb, lsl #1

	;b33
							PATTERN_upscale_step_2 $arch, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #(4*$pix_bytes)]!		;first line of sub-block
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	
	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]

	add	dst, dst, drb

	;c43
							PATTERN_upscale_step_3 $arch, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #(-4*$pix_bytes)]!	;first line of sub-block
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	
	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]

	sub	dst, dst, drb, lsl #1

	;d33
							PATTERN_upscale_step_4 $arch, $version.switch_band_6x8, $version.pattern_7x6_mark, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #(4*$pix_bytes)]!		;first line of sub-block
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	
	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]!

	sub		dst, dst, drb, lsl #2
	sub		dst, dst, drb
	
	mov		pc, w2

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;$version.switch_band_7x6
;	UP_SCALE_PROC_TABLE $version
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

						;aaaabbb
						;aaaabbb		
						;aaaabbb		
						;aaaabbb		
						;ccccddd
						;ccccddd
$version.case_up_7x7  	;ccccddd
 
	;a44
							PATTERN_upscale_step_1 $arch, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #(1*$pix_bytes)]!			;first line of sub-block
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	
	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	
	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	
	sub	dst, dst, drb, lsl #1
	sub	dst, dst, drb

	;b34
							PATTERN_upscale_step_2 $arch, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #(4*$pix_bytes)]!		;first line of sub-block
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	
	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]

	add	dst, dst, drb

	;c43
							PATTERN_upscale_step_3 $arch, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #(-4*$pix_bytes)]!	;first line of sub-block
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	
	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]

	sub	dst, dst, drb, lsl #1

	;d33
							PATTERN_upscale_step_4 $arch, $version.switch_band_7x8, $version.pattern_7x7_mark, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #(4*$pix_bytes)]!		;first line of sub-block
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	
	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]!

	sub		dst, dst, drb, lsl #2
	sub		dst, dst, drb, lsl #1
	
	mov		pc, w2

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;$version.switch_band_7x7
;	UP_SCALE_PROC_TABLE $version
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

						;aaaabbb
						;aaaabbb		
						;aaaabbb		
						;aaaabbb		
						;ccccddd
						;ccccddd
						;ccccddd
$version.case_up_7x8  	;ccccddd
 
	;a44
							PATTERN_upscale_step_1 $arch, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #(1*$pix_bytes)]!			;first line of sub-block
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	
	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	
	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	
	sub	dst, dst, drb, lsl #1
	sub	dst, dst, drb

	;b34
							PATTERN_upscale_step_2 $arch, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #(4*$pix_bytes)]!		;first line of sub-block
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	
	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]

	add	dst, dst, drb

	;c44
							PATTERN_upscale_step_3 $arch, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #(-4*$pix_bytes)]!	;first line of sub-block
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	
	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]

	sub	dst, dst, drb, lsl #1
	sub	dst, dst, drb

	;d34
							PATTERN_upscale_step_4 $arch, $version.switch_band_7x8, $version.pattern_7x8_mark, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #(4*$pix_bytes)]!		;first line of sub-block
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	
	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]!

	sub		dst, dst, drb, lsl #3
	add		dst, dst, drb
	
	mov		pc, w2

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
$version.switch_band_7x8
	UP_SCALE_PROC_TABLE $version
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

						;aaaabbbb
						;aaaabbbb		
						;aaaabbbb		
						;aaaabbbb		
						;ccccdddd
						;ccccdddd
$version.case_up_8x7  	;ccccdddd
	;a44
							PATTERN_upscale_step_1 $arch, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #(1*$pix_bytes)]!			;first line of sub-block
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	
	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	
	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	
	sub	dst, dst, drb, lsl #1
	sub	dst, dst, drb

	;b44
							PATTERN_upscale_step_2 $arch, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #(4*$pix_bytes)]!		;first line of sub-block
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	
	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]

	add	dst, dst, drb

	;c43
							PATTERN_upscale_step_3 $arch, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #(-4*$pix_bytes)]!	;first line of sub-block
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	
	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]

	sub	dst, dst, drb, lsl #1

	;d43
							PATTERN_upscale_step_4 $arch, $version.switch_band_7x8, $version.pattern_8x7_mark, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #(4*$pix_bytes)]!		;first line of sub-block
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	
	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]!


	sub		dst, dst, drb, lsl #3
	add		dst, dst, drb, lsl #1
	
	mov		pc, w2

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;$version.switch_band_8x7
;	UP_SCALE_PROC_TABLE $version
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

						;aaaabbbb
						;aaaabbbb		
						;aaaabbbb		
						;ccccdddd
						;ccccdddd
$version.case_up_8x6  	;ccccdddd
	;a43
							PATTERN_upscale_step_1 $arch, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #(1*$pix_bytes)]!			;first line of sub-block
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	
	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
		
	sub	dst, dst, drb, lsl #1

	;b43
							PATTERN_upscale_step_2 $arch, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #(4*$pix_bytes)]!		;first line of sub-block
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	
	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]

	add	dst, dst, drb

	;c43
							PATTERN_upscale_step_3 $arch, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #(-4*$pix_bytes)]!	;first line of sub-block
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	
	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]

	sub	dst, dst, drb, lsl #1

	;d43
							PATTERN_upscale_step_4 $arch, $version.switch_band_8x8, $version.pattern_8x6_mark, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #(4*$pix_bytes)]!		;first line of sub-block
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	
	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]!


	sub		dst, dst, drb, lsl #2
	sub		dst, dst, drb
	
	mov		pc, w2

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;$version.switch_band_8x6
;	UP_SCALE_PROC_TABLE $version
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

						;aaaabbbb
						;aaaabbbb		
						;aaaabbbb		
						;aaaabbbb		
						;ccccdddd
						;ccccdddd
						;ccccdddd
$version.case_up_8x8  	;ccccdddd
	;a44
							PATTERN_upscale_step_1 $arch, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #(1*$pix_bytes)]!			;first line of sub-block
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	
	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	
	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	
	sub	dst, dst, drb, lsl #1
	sub	dst, dst, drb

	;b44
							PATTERN_upscale_step_2 $arch, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #(4*$pix_bytes)]!		;first line of sub-block
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	
	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]

	add	dst, dst, drb

	;c44
							PATTERN_upscale_step_3 $arch, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #(-4*$pix_bytes)]!	;first line of sub-block
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	
	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]

	sub	dst, dst, drb, lsl #1
	sub	dst, dst, drb

	;d44
							PATTERN_upscale_step_4 $arch, $version.switch_band_8x8, $version.pattern_8x8_mark, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #(4*$pix_bytes)]!		;first line of sub-block
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	
	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]!

	sub		dst, dst, drb, lsl #3
	add		dst, dst, drb
	
	mov		pc, w2

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
$version.switch_band_8x8
	UP_SCALE_PROC_TABLE $version
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

						;aaaabbbb
						;aaaabbbb		
						;aaaabbbb		
						;aaaabbbb		
						;aaaabbbb		
						;ccccdddd
						;ccccdddd
						;ccccdddd
$version.case_up_8x9  	;ccccdddd
	;a45
							PATTERN_upscale_step_1 $arch, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #(1*$pix_bytes)]!			;first line of sub-block
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	
	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	
	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	
	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	
	sub	dst, dst, drb, lsl #2

	;b45
							PATTERN_upscale_step_2 $arch, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #(4*$pix_bytes)]!		;first line of sub-block
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	
	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]

	add	dst, dst, drb

	;c44
							PATTERN_upscale_step_3 $arch, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #(-4*$pix_bytes)]!	;first line of sub-block
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	
	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]

	sub	dst, dst, drb, lsl #1
	sub	dst, dst, drb

	;d44
							PATTERN_upscale_step_4 $arch, $version.switch_band_8x8, $version.pattern_8x9_mark, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #(4*$pix_bytes)]!		;first line of sub-block
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	
	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]!

	sub		dst, dst, drb, lsl #3
	
	mov		pc, w2

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;$version.switch_band_8x9
;	UP_SCALE_PROC_TABLE $version
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

						;aaaabbbb
						;aaaabbbb		
						;aaaabbbb		
						;aaaabbbb		
						;aaaabbbb		
						;ccccdddd
						;ccccdddd
						;ccccdddd
						;ccccdddd
$version.case_up_8x10 	;ccccdddd 
	;a45
							PATTERN_upscale_step_1 $arch, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #(1*$pix_bytes)]!			;first line of sub-block
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	
	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	
	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	
	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	
	sub	dst, dst, drb, lsl #2

	;b45
							PATTERN_upscale_step_2 $arch, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #(4*$pix_bytes)]!		;first line of sub-block
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	
	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]

	add	dst, dst, drb

	;c45
							PATTERN_upscale_step_3 $arch, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #(-4*$pix_bytes)]!	;first line of sub-block
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	
	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]

	sub	dst, dst, drb, lsl #2

	;d44
							PATTERN_upscale_step_4 $arch, $version.switch_band_9x8, $version.pattern_8x10_mark, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #(4*$pix_bytes)]!		;first line of sub-block
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	
	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]!

	sub		dst, dst, drb, lsl #3
	sub		dst, dst, drb
	
	mov		pc, w2

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;$version.switch_band_8x10
;	UP_SCALE_PROC_TABLE $version
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

						;aaaaabbbb
						;aaaaabbbb		
						;aaaaabbbb		
						;aaaaabbbb		
						;cccccdddd
						;cccccdddd
						;cccccdddd
$version.case_up_9x8  	;cccccdddd 
	;a54
							PATTERN_upscale_step_1 $arch, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #(1*$pix_bytes)]!			;first line of sub-block
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	str$pix_format	w3, [dst, #(4*$pix_bytes)]
	
	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	str$pix_format	w3, [dst, #(4*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	str$pix_format	w3, [dst, #(4*$pix_bytes)]
	
	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	str$pix_format	w3, [dst, #(4*$pix_bytes)]
		
	sub	dst, dst, drb, lsl #2
	add	dst, dst, drb

	;b44
							PATTERN_upscale_step_2 $arch, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #(5*$pix_bytes)]!		;first line of sub-block
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	
	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]

	add	dst, dst, drb

	;c54
							PATTERN_upscale_step_3 $arch, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #(-5*$pix_bytes)]!	;first line of sub-block
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	str$pix_format	w3, [dst, #(4*$pix_bytes)]
	
	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	str$pix_format	w3, [dst, #(4*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	str$pix_format	w3, [dst, #(4*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	str$pix_format	w3, [dst, #(4*$pix_bytes)]

	sub	dst, dst, drb, lsl #2
	add	dst, dst, drb

	;d44
							PATTERN_upscale_step_4 $arch, $version.switch_band_9x8, $version.pattern_9x8_mark, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #(5*$pix_bytes)]!		;first line of sub-block
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	
	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]!

	sub		dst, dst, drb, lsl #3
	add		dst, dst, drb
	
	mov		pc, w2

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
$version.switch_band_9x8
	UP_SCALE_PROC_TABLE $version
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

						;aaaaabbbb
						;aaaaabbbb		
						;aaaaabbbb		
						;aaaaabbbb		
						;aaaaabbbb		
						;cccccdddd
						;cccccdddd
						;cccccdddd
$version.case_up_9x9  	;cccccdddd
	;a55
							PATTERN_upscale_step_1 $arch, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #(1*$pix_bytes)]!			;first line of sub-block
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	str$pix_format	w3, [dst, #(4*$pix_bytes)]
	
	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	str$pix_format	w3, [dst, #(4*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	str$pix_format	w3, [dst, #(4*$pix_bytes)]
	
	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	str$pix_format	w3, [dst, #(4*$pix_bytes)]
		
	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	str$pix_format	w3, [dst, #(4*$pix_bytes)]
		
	sub	dst, dst, drb, lsl #2

	;b45
							PATTERN_upscale_step_2 $arch, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #(5*$pix_bytes)]!		;first line of sub-block
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	
	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]

	add	dst, dst, drb

	;c54
							PATTERN_upscale_step_3 $arch, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #(-5*$pix_bytes)]!	;first line of sub-block
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	str$pix_format	w3, [dst, #(4*$pix_bytes)]
	
	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	str$pix_format	w3, [dst, #(4*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	str$pix_format	w3, [dst, #(4*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	str$pix_format	w3, [dst, #(4*$pix_bytes)]

	sub	dst, dst, drb, lsl #2
	add	dst, dst, drb

	;d44
							PATTERN_upscale_step_4 $arch, $version.switch_band_9x8, $version.pattern_9x9_mark, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #(5*$pix_bytes)]!		;first line of sub-block
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	
	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]!

	sub		dst, dst, drb, lsl #3
	
	mov		pc, w2

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;$version.switch_band_9x9
;	UP_SCALE_PROC_TABLE $version
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

						;aaaaabbbb
						;aaaaabbbb		
						;aaaaabbbb		
						;aaaaabbbb		
						;aaaaabbbb		
						;cccccdddd
						;cccccdddd
						;cccccdddd
						;cccccdddd
$version.case_up_9x10 	;cccccdddd
	;a55
							PATTERN_upscale_step_1 $arch, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #(1*$pix_bytes)]!			;first line of sub-block
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	str$pix_format	w3, [dst, #(4*$pix_bytes)]
	
	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	str$pix_format	w3, [dst, #(4*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	str$pix_format	w3, [dst, #(4*$pix_bytes)]
	
	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	str$pix_format	w3, [dst, #(4*$pix_bytes)]
		
	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	str$pix_format	w3, [dst, #(4*$pix_bytes)]
		
	sub	dst, dst, drb, lsl #2

	;b45
							PATTERN_upscale_step_2 $arch, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #(5*$pix_bytes)]!		;first line of sub-block
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	
	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]

	add	dst, dst, drb

	;c55
							PATTERN_upscale_step_3 $arch, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #(-5*$pix_bytes)]!	;first line of sub-block
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	str$pix_format	w3, [dst, #(4*$pix_bytes)]
	
	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	str$pix_format	w3, [dst, #(4*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	str$pix_format	w3, [dst, #(4*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	str$pix_format	w3, [dst, #(4*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	str$pix_format	w3, [dst, #(4*$pix_bytes)]

	sub	dst, dst, drb, lsl #2

	;d45
							PATTERN_upscale_step_4 $arch, $version.switch_band_9x10, $version.pattern_9x10_mark, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #(5*$pix_bytes)]!		;first line of sub-block
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	
	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]!

	sub		dst, dst, drb, lsl #3
	sub		dst, dst, drb
	
	mov		pc, w2

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
$version.switch_band_9x10
	UP_SCALE_PROC_TABLE $version
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	
						;aaaaabbbbb
						;aaaaabbbbb		
						;aaaaabbbbb		
						;aaaaabbbbb		
						;aaaaabbbbb		
						;cccccddddd
						;cccccddddd
						;cccccddddd
$version.case_up_10x9 	;cccccddddd
	;a55
							PATTERN_upscale_step_1 $arch, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #(1*$pix_bytes)]!			;first line of sub-block
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	str$pix_format	w3, [dst, #(4*$pix_bytes)]
	
	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	str$pix_format	w3, [dst, #(4*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	str$pix_format	w3, [dst, #(4*$pix_bytes)]
	
	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	str$pix_format	w3, [dst, #(4*$pix_bytes)]
		
	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	str$pix_format	w3, [dst, #(4*$pix_bytes)]
		
	sub	dst, dst, drb, lsl #2

	;b55
							PATTERN_upscale_step_2 $arch, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #(5*$pix_bytes)]!		;first line of sub-block
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	str$pix_format	w3, [dst, #(4*$pix_bytes)]
	
	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	str$pix_format	w3, [dst, #(4*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	str$pix_format	w3, [dst, #(4*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	str$pix_format	w3, [dst, #(4*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	str$pix_format	w3, [dst, #(4*$pix_bytes)]

	add	dst, dst, drb

	;c54
							PATTERN_upscale_step_3 $arch, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #(-5*$pix_bytes)]!	;first line of sub-block
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	str$pix_format	w3, [dst, #(4*$pix_bytes)]
	
	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	str$pix_format	w3, [dst, #(4*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	str$pix_format	w3, [dst, #(4*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	str$pix_format	w3, [dst, #(4*$pix_bytes)]

	sub	dst, dst, drb, lsl #2
	add	dst, dst, drb

	;d54
							PATTERN_upscale_step_4 $arch, $version.switch_band_9x10, $version.pattern_10x9_mark, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #(5*$pix_bytes)]!		;first line of sub-block
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	str$pix_format	w3, [dst, #(4*$pix_bytes)]
	
	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	str$pix_format	w3, [dst, #(4*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	str$pix_format	w3, [dst, #(4*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	str$pix_format	w3, [dst, #(4*$pix_bytes)]!

	sub		dst, dst, drb, lsl #3
	
	mov		pc, w2

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;$version.switch_band_10x9
;	UP_SCALE_PROC_TABLE $version
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

						;aaaaabbbbb
						;aaaaabbbbb		
						;aaaaabbbbb		
						;aaaaabbbbb		
						;cccccddddd
						;cccccddddd
						;cccccddddd
$version.case_up_10x8 	;cccccddddd
	;a55
							PATTERN_upscale_step_1 $arch, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #(1*$pix_bytes)]!			;first line of sub-block
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	str$pix_format	w3, [dst, #(4*$pix_bytes)]
	
	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	str$pix_format	w3, [dst, #(4*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	str$pix_format	w3, [dst, #(4*$pix_bytes)]
	
	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	str$pix_format	w3, [dst, #(4*$pix_bytes)]
				
	sub	dst, dst, drb, lsl #2
	add	dst, dst, drb

	;b55
							PATTERN_upscale_step_2 $arch, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #(5*$pix_bytes)]!		;first line of sub-block
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	str$pix_format	w3, [dst, #(4*$pix_bytes)]
	
	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	str$pix_format	w3, [dst, #(4*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	str$pix_format	w3, [dst, #(4*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	str$pix_format	w3, [dst, #(4*$pix_bytes)]

	add	dst, dst, drb

	;c54
							PATTERN_upscale_step_3 $arch, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #(-5*$pix_bytes)]!	;first line of sub-block
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	str$pix_format	w3, [dst, #(4*$pix_bytes)]
	
	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	str$pix_format	w3, [dst, #(4*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	str$pix_format	w3, [dst, #(4*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	str$pix_format	w3, [dst, #(4*$pix_bytes)]

	sub	dst, dst, drb, lsl #2
	add	dst, dst, drb

	;d54
							PATTERN_upscale_step_4 $arch, $version.switch_band_10x8, $version.pattern_10x8_mark, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #(5*$pix_bytes)]!		;first line of sub-block
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	str$pix_format	w3, [dst, #(4*$pix_bytes)]
	
	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	str$pix_format	w3, [dst, #(4*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	str$pix_format	w3, [dst, #(4*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	str$pix_format	w3, [dst, #(4*$pix_bytes)]!

	sub		dst, dst, drb, lsl #3
	add	dst, dst, drb
	
	mov		pc, w2

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
$version.switch_band_10x8
	UP_SCALE_PROC_TABLE $version
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

						;aaaaabbbbb
						;aaaaabbbbb		
						;aaaaabbbbb		
						;aaaaabbbbb		
						;aaaaabbbbb		
						;cccccddddd
						;cccccddddd
						;cccccddddd
						;cccccddddd
$version.case_up_10x10	;cccccddddd 
	;a55
							PATTERN_upscale_step_1 $arch, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #(1*$pix_bytes)]!			;first line of sub-block
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	str$pix_format	w3, [dst, #(4*$pix_bytes)]
	
	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	str$pix_format	w3, [dst, #(4*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	str$pix_format	w3, [dst, #(4*$pix_bytes)]
	
	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	str$pix_format	w3, [dst, #(4*$pix_bytes)]
		
	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	str$pix_format	w3, [dst, #(4*$pix_bytes)]
		
	sub	dst, dst, drb, lsl #2

	;b55
							PATTERN_upscale_step_2 $arch, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #(5*$pix_bytes)]!		;first line of sub-block
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	str$pix_format	w3, [dst, #(4*$pix_bytes)]
	
	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	str$pix_format	w3, [dst, #(4*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	str$pix_format	w3, [dst, #(4*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	str$pix_format	w3, [dst, #(4*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	str$pix_format	w3, [dst, #(4*$pix_bytes)]

	add	dst, dst, drb

	;c55
							PATTERN_upscale_step_3 $arch, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #(-5*$pix_bytes)]!	;first line of sub-block
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	str$pix_format	w3, [dst, #(4*$pix_bytes)]
	
	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	str$pix_format	w3, [dst, #(4*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	str$pix_format	w3, [dst, #(4*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	str$pix_format	w3, [dst, #(4*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	str$pix_format	w3, [dst, #(4*$pix_bytes)]

	sub	dst, dst, drb, lsl #2

	;d55
							PATTERN_upscale_step_4 $arch, $version.switch_band_10x8, $version.pattern_10x10_mark, $pix_format, $pix_bytes
	SET_ALPHA$pix_format w3, 0xff
	str$pix_format	w3, [dst, #(5*$pix_bytes)]!		;first line of sub-block
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	str$pix_format	w3, [dst, #(4*$pix_bytes)]
	
	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	str$pix_format	w3, [dst, #(4*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	str$pix_format	w3, [dst, #(4*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	str$pix_format	w3, [dst, #(4*$pix_bytes)]

	str$pix_format	w3, [dst, drb]!
	str$pix_format	w3, [dst, #(1*$pix_bytes)]
	str$pix_format	w3, [dst, #(2*$pix_bytes)]
	str$pix_format	w3, [dst, #(3*$pix_bytes)]
	str$pix_format	w3, [dst, #(4*$pix_bytes)]!

	sub		dst, dst, drb, lsl #3
	sub		dst, dst, drb
	
	mov		pc, w2

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;$version.switch_band_10x10
;	UP_SCALE_PROC_TABLE $version
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	MEND


  END
