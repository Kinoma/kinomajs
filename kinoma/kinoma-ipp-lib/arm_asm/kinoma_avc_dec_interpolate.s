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

	EXPORT  |ippiInterpolateLuma_H264_8u_C1R_arm|
	EXPORT  |ippiInterpolateLuma_H264_8u_C1R_16x16_arm|
	EXPORT  |ippiInterpolateLuma_H264_8u_C1R_8x16_arm|

;t0 - 4*1 + 20*t2 + 20*t3 - 4*t4 + t5
;define FILTER_ONE( r, t0, t1, t2, t3, t4, t5 )		\
;{													\
;	int mm = (t1+t4 ) - ((t2+t3 )<<2);				\
;	r = (t0+t5) - ( mm + (mm<<2));					\
;}
;
;#define FILTER_AND_CLIP( tmp0, t0, t1, t2, t3, t4, t5 ) \
;		FILTER_ONE(tmp0, t0, t1, t2, t3, t4, t5 );		\
;		tmp0 = (16 + tmp0 )>>5;							\
;		Clip1(const_255, tmp0);							


src			RN 0
srcStep		RN 1
dst			RN 2
dstStep		RN 3
t0		    RN 4
t1			RN 5
t2			RN 6
t3			RN 7
t4			RN 8
t5			RN 9
CNST_255	RN 10
tmp0		RN 11
mm			RN 12

DX_OFFSET		EQU		36
DY_OFFSET		EQU		40
WIDTH_OFFSET	EQU		44
HEIGHT_OFFSET	EQU		48
MB_MAX			EQU		16
TMP_XY			EQU		22

;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	MACRO
		MY_ENTER
;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	
	stmdb	sp!, {r4 - r11, lr}	

	MEND
	
;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	MACRO
		MY_RETURN
;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	
	ldmia	sp!, {r4 - r11, pc}	

	MEND

;;;;;;;;;;;;;;;;;;;;;;;;;;;;		
	MACRO
		NOP $offset
;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	MEND
	
;;;;;;;;;;;;;;;;;;;;;;;;;;;;		
	MACRO
		GET_DX_sp $stack_offset
;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	ldr		ddx,  [sp, #(DX_OFFSET+$stack_offset)]

	MEND

;;;;;;;;;;;;;;;;;;;;;;;;;;;;		
	MACRO
		GET_DY_sp $stack_offset
;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	ldr		ddy,  [sp, #(DY_OFFSET+$stack_offset)]

	MEND

;;;;;;;;;;;;;;;;;;;;;;;;;;;;		
	MACRO
		GET_WIDTH_sp $stack_offset
;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	ldr		width,  [sp, #(WIDTH_OFFSET+$stack_offset)]	;height shares mm

	MEND

;;;;;;;;;;;;;;;;;;;;;;;;;;;;		
	MACRO
		GET_HEIGHT_sp $stack_offset
;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	ldr		height,  [sp, #(HEIGHT_OFFSET+$stack_offset)]	;height shares mm

	MEND

;;;;;;;;;;;;;;;;;;;;;;;;;;;;		
	MACRO
		GET_HEIGHT_8 $stack_offset
;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	ldr		height,  [sp, #8]	;height shares mm

	MEND

;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	MACRO
		FILTER_ONE $r, $t0, $t1, $t2, $t3, $t4, $t5
;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	add		mm, $t2, $t3
	add		$r, $t1, $t4
	sub		mm, $r,  mm, asl #2
	add		mm, mm,  mm, asl #2
	add		$r, $t0, $t5
	sub		$r, $r,  mm

	MEND

	
;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	MACRO
		FILTER_AND_CLIP $r, $t0, $t1, $t2, $t3, $t4, $t5
;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	FILTER_ONE $r, $t0, $t1, $t2, $t3, $t4, $t5
	add		$r, $r,  #16
	mov		$r, $r, asr #5
	cmp		$r, CNST_255
	bichi	$r, CNST_255, $r, asr #31 
	
	MEND


;;;;;;;;;;;;;;;;;;;;;;;;;;;;	
	MACRO
		FILTER_AND_CLIP_t2 $r, $t0, $t1, $t2, $t3, $t4, $t5
;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	
	FILTER_AND_CLIP $r, $t0, $t1, $t2, $t3, $t4, $t5	
	add		$r, $r, $t2
	add		$r, $r, #1
	mov		$r, $r, asr #1

	MEND

;;;;;;;;;;;;;;;;;;;;;;;;;;;;		
	MACRO
		FILTER_AND_CLIP_t3 $r, $t0, $t1, $t2, $t3, $t4, $t5
;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	
	FILTER_AND_CLIP $r, $t0, $t1, $t2, $t3, $t4, $t5	
	add		$r, $r, $t3
	add		$r, $r, #1
	mov		$r, $r, asr #1

	MEND

;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	MACRO
		FILTER_AND_CLIP_hi $r, $t0, $t1, $t2, $t3, $t4, $t5
;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	FILTER_ONE $r, $t0, $t1, $t2, $t3, $t4, $t5
	add		$r, $r,  #512		;???
	mov		$r, $r, asr #10
	cmp		$r, CNST_255
	bichi	$r, CNST_255, $r, asr #31 
	
	MEND

;;;;;;;;;;;;;;;;;;;;;;;;;;;;		
	MACRO
		FILTER_AND_CLIP_hi_t2 $r, $t0, $t1, $t2, $t3, $t4, $t5
;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	
	FILTER_AND_CLIP_hi $r, $t0, $t1, $t2, $t3, $t4, $t5	
	add		mm, $t2, #16
	mov		mm, mm, asr #5
	cmp		mm, CNST_255
	bichi	mm, CNST_255, mm, asr #31 
	add		$r, $r, mm
	add		$r, $r, #1
	mov		$r, $r, asr #1

	MEND

;;;;;;;;;;;;;;;;;;;;;;;;;;;;		
	MACRO
		FILTER_AND_CLIP_hi_t3 $r, $t0, $t1, $t2, $t3, $t4, $t5
;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	
	FILTER_AND_CLIP_hi $r, $t0, $t1, $t2, $t3, $t4, $t5	
	add		mm, $t3, #16
	mov		mm, mm, asr #5
	cmp		mm, CNST_255
	bichi	mm, CNST_255, mm, asr #31 
	add		$r, $r, mm
	add		$r, $r, #1
	mov		$r, $r, asr #1

	MEND

;;;;;;;;;;;;;;;;;;;;;;;;;;;;		
	MACRO
		AVERAGE_FOUR $r, $t0, $t1
;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	and		mm, mask, $t0
	and		$r, mask, $t1
	add		mm, mm,   $r
	add		mm, round,mm
	and		$r, mask, mm, lsr #1
	
	and		mm, mask, $t0, lsr #8
	and		$t1,mask, $t1, lsr #8
	add		mm, mm,   $t1
	add		mm, round,mm
	and		mm, mask, mm, lsr #1
	
	orr		$r, $r, mm, lsl #8
	
	MEND

;;;;;;;;;;;;;;;;;;;;;;;;;;;;		
	MACRO
		HORIZONTAL_PATTERN $PATTERN, $m_src, $m_SRC_BYTES, $m_LDRX, $m_srcStep, $m_dst, $m_DST_BYTES, $m_STRX, $m_dstStep, $m_GET_WIDTH, $m_STACK_OFFSET, $m_start_marker, $m_continue_marker
;;;;;;;;;;;;;;;;;;;;;;;;;;;;

$m_start_marker
	pld		 [$m_src, $m_srcStep]
	pld		 [$m_dst, $m_dstStep]
	$m_LDRX    t0, [$m_src, $m_srcStep]!		
	$m_LDRX    t1, [$m_src, #$m_SRC_BYTES*1]		
	$m_LDRX    t2, [$m_src, #$m_SRC_BYTES*2]		
	$m_LDRX    t3, [$m_src, #$m_SRC_BYTES*3]		
	$m_LDRX    t4, [$m_src, #$m_SRC_BYTES*4]		
	$m_LDRX    t5, [$m_src, #$m_SRC_BYTES*5]		
	$PATTERN tmp0,  t0, t1, t2, t3, t4, t5
	$m_STRX	tmp0, [$m_dst, $m_dstStep]!
	$m_LDRX    t0, [$m_src, #$m_SRC_BYTES*6]		
	$PATTERN tmp0,  t1, t2, t3, t4, t5, t0
	$m_STRX	tmp0, [$m_dst, #$m_DST_BYTES*1]
	$m_LDRX    t1, [$m_src, #$m_SRC_BYTES*7]		
	$PATTERN tmp0,  t2, t3, t4, t5, t0, t1
	$m_STRX	tmp0, [$m_dst, #$m_DST_BYTES*2]
	$m_LDRX    t2, [$m_src, #$m_SRC_BYTES*8]		
	$PATTERN tmp0,  t3, t4, t5, t0, t1, t2

	$m_GET_WIDTH $m_STACK_OFFSET

	$m_STRX	tmp0, [$m_dst, #$m_DST_BYTES*3]

	cmp		width, #4				;width shares mm
	beq		$m_continue_marker

	$m_LDRX    t3, [$m_src, #$m_SRC_BYTES*9]		
	$PATTERN tmp0,  t4, t5, t0, t1, t2, t3
	$m_STRX	tmp0, [$m_dst, #$m_DST_BYTES*4]
	$m_LDRX    t4, [$m_src, #$m_SRC_BYTES*10]		
	$PATTERN tmp0,  t5, t0, t1, t2, t3, t4
	$m_STRX	tmp0, [$m_dst, #$m_DST_BYTES*5]
	$m_LDRX    t5, [$m_src, #$m_SRC_BYTES*11]		
	$PATTERN tmp0,  t0, t1, t2, t3, t4, t5
	$m_STRX	tmp0, [$m_dst, #$m_DST_BYTES*6]
	$m_LDRX    t0, [$m_src, #$m_SRC_BYTES*12]		
	$PATTERN tmp0,  t1, t2, t3, t4, t5, t0

	$m_GET_WIDTH $m_STACK_OFFSET

	$m_STRX	tmp0, [$m_dst, #$m_DST_BYTES*7]

	cmp		width, #8				;width shares mm
	beq		$m_continue_marker
						   
	$m_LDRX    t1, [$m_src, #$m_SRC_BYTES*13]   		
	$PATTERN tmp0,  t2, t3, t4, t5, t0, t1
	$m_STRX	tmp0, [$m_dst, #$m_DST_BYTES*8]				   
	$m_LDRX    t2, [$m_src, #$m_SRC_BYTES*14]   		
	$PATTERN tmp0,  t3, t4, t5, t0, t1, t2
	$m_STRX	tmp0, [$m_dst, #$m_DST_BYTES*9]
	$m_LDRX    t3, [$m_src, #$m_SRC_BYTES*15]   		
	$PATTERN tmp0,  t4, t5, t0, t1, t2, t3
	$m_STRX	tmp0, [$m_dst,#$m_DST_BYTES*10]
	$m_LDRX    t4, [$m_src, #$m_SRC_BYTES*16]   		
	$PATTERN tmp0,  t5, t0, t1, t2, t3, t4
	$m_STRX	tmp0, [$m_dst,#$m_DST_BYTES*11]

	$m_LDRX    t5, [$m_src, #$m_SRC_BYTES*17]		
	$PATTERN tmp0,  t0, t1, t2, t3, t4, t5
	$m_STRX	tmp0, [$m_dst,#$m_DST_BYTES*12]
	$m_LDRX    t0, [$m_src, #$m_SRC_BYTES*18]   		
	$PATTERN tmp0,  t1, t2, t3, t4, t5, t0
	$m_STRX	tmp0, [$m_dst,#$m_DST_BYTES*13]
	$m_LDRX    t1, [$m_src, #$m_SRC_BYTES*19]     		
	$PATTERN tmp0,  t2, t3, t4, t5, t0, t1
	$m_STRX	tmp0, [$m_dst,#$m_DST_BYTES*14]
	$m_LDRX    t2, [$m_src, #$m_SRC_BYTES*20]     		
	$PATTERN tmp0,  t3, t4, t5, t0, t1, t2
	$m_STRX	tmp0, [$m_dst,#$m_DST_BYTES*15]

$m_continue_marker
	subs	height, height, #1
 	bpl		$m_start_marker

	MEND



;;;;;;;;;;;;;;;;;;;;;;;;;;;;		
	MACRO
		VERTICAL_PATTERN $PATTERN, $m_src, $m_SRC_BYTES, $m_LDRX, $m_srcStep, $m_dst, $m_DST_BYTES, $m_STRX, $m_dstStep, $m_GET_HEIGHT, $m_STACK_OFFSET, $m_start_marker, $m_continue_marker
;;;;;;;;;;;;;;;;;;;;;;;;;;;;

$m_start_marker
	sub		$m_src, $m_src, src_chunk
	sub		$m_dst, $m_dst, dst_chunk	
	
	$m_LDRX	t0, [$m_src]
	$m_LDRX	t1, [$m_src, $m_srcStep]!
	$m_LDRX	t2, [$m_src, $m_srcStep]!
	$m_LDRX	t3, [$m_src, $m_srcStep]!
	$m_LDRX	t4, [$m_src, $m_srcStep]!
	$m_LDRX	t5, [$m_src, $m_srcStep]!
	$PATTERN tmp0,  t0, t1, t2, t3, t4, t5
	$m_STRX	tmp0, [$m_dst]	  
	$m_LDRX	t0, [$m_src, $m_srcStep]!		
	$PATTERN tmp0,  t1, t2, t3, t4, t5, t0
	$m_STRX	tmp0, [$m_dst, $m_dstStep]!	  
	$m_LDRX	t1, [$m_src, $m_srcStep]!		
	$PATTERN tmp0,  t2, t3, t4, t5, t0, t1
	$m_STRX	tmp0, [$m_dst, $m_dstStep]!	  
	$m_LDRX	t2, [$m_src, $m_srcStep]!		
	$PATTERN tmp0,  t3, t4, t5, t0, t1, t2

	$m_GET_HEIGHT $m_STACK_OFFSET
	$m_STRX	tmp0, [$m_dst, $m_dstStep]!	  

	cmp		height, #4				;height shares mm
	beq		$m_continue_marker
							  
	$m_LDRX	t3, [$m_src, $m_srcStep]!		
	$PATTERN tmp0,  t4, t5, t0, t1, t2, t3
	$m_STRX	tmp0, [$m_dst, $m_dstStep]!	  
	$m_LDRX	t4, [$m_src, $m_srcStep]!		
	$PATTERN tmp0,  t5, t0, t1, t2, t3, t4
	$m_STRX	tmp0, [$m_dst, $m_dstStep]!	  
	$m_LDRX	t5, [$m_src, $m_srcStep]!		
	$PATTERN tmp0,  t0, t1, t2, t3, t4, t5
	$m_STRX	tmp0, [$m_dst, $m_dstStep]!	  
	$m_LDRX	t0, [$m_src, $m_srcStep]!		
	$PATTERN tmp0,  t1, t2, t3, t4, t5, t0
	
	$m_GET_HEIGHT $m_STACK_OFFSET
	$m_STRX	tmp0, [$m_dst, $m_dstStep]!	  

	cmp		height, #8				;height shares mm
	beq		$m_continue_marker
							  
	$m_LDRX	t1, [$m_src, $m_srcStep]! 		
	$PATTERN tmp0,  t2, t3, t4, t5, t0, t1
	$m_STRX	tmp0, [$m_dst, $m_dstStep]!	  				   
	$m_LDRX	t2, [$m_src, $m_srcStep]!  		
	$PATTERN tmp0,  t3, t4, t5, t0, t1, t2
	$m_STRX	tmp0, [$m_dst, $m_dstStep]!	  
	$m_LDRX	t3, [$m_src, $m_srcStep]!  		
	$PATTERN tmp0,  t4, t5, t0, t1, t2, t3
	$m_STRX	tmp0, [$m_dst, $m_dstStep]!	  
	$m_LDRX	t4, [$m_src, $m_srcStep]!  		
	$PATTERN tmp0,  t5, t0, t1, t2, t3, t4
	$m_STRX	tmp0, [$m_dst, $m_dstStep]!	  
							  
	$m_LDRX	t5, [$m_src, $m_srcStep]!		
	$PATTERN tmp0,  t0, t1, t2, t3, t4, t5
	$m_STRX	tmp0, [$m_dst, $m_dstStep]!	  
	$m_LDRX	t0, [$m_src, $m_srcStep]!  		
	$PATTERN tmp0,  t1, t2, t3, t4, t5, t0
	$m_STRX	tmp0, [$m_dst, $m_dstStep]!	  
	$m_LDRX	t1, [$m_src, $m_srcStep]!    		
	$PATTERN tmp0,  t2, t3, t4, t5, t0, t1
	$m_STRX	tmp0, [$m_dst, $m_dstStep]!	  
	$m_LDRX	t2, [$m_src, $m_srcStep]!    		
	$PATTERN tmp0,  t3, t4, t5, t0, t1, t2

	$m_STRX	tmp0, [$m_dst, $m_dstStep]!

$m_continue_marker
	ldr		src_chunk, [sp, #4]		;src_chunk shares mm
	ldr		dst_chunk, [sp, #0]		;dst_chunk shares tmp0
	subs	width, width, #1
 	bpl		$m_start_marker

	MEND

;;;;;;;;;;;;;;;;;;;;;;;;;;;;		
	MACRO
		VERTICAL_PATTERN_setup $m_src, $m_SRC_BYTES, $m_srcStep, $m_dst, $m_DST_BYTES, $m_dstStep
;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	mul		dst_chunk, height, $m_dstStep				;dst_chunk shares tmp0
	sub		dst_chunk, dst_chunk, $m_dstStep
	sub		dst_chunk, dst_chunk, #$m_DST_BYTES
	str		dst_chunk, [sp, #0]
	
	mul		src_chunk, $m_srcStep, height 				;src_chunk shares mm
	add		src_chunk, src_chunk, $m_srcStep, lsl #2
	sub		src_chunk, src_chunk, #$m_SRC_BYTES
	str	    src_chunk, [sp, #4]
	add     $m_dst, $m_dst, dst_chunk
	sub     $m_src, $m_src, $m_srcStep, lsl #1
	add     $m_src, $m_src, src_chunk

	MEND


;;;;;;;;;;;;;;;;;;;;;;;;;;;;		
	MACRO
		VERTICAL_PATTERN_short_step $PATTERN, $m_src, $m_SRC_BYTES, $m_LDRX, $m_srcStep, $m_dst, $m_DST_BYTES, $m_STRX, $m_dstStep, $m_GET_HEIGHT, $m_STACK_OFFSET, $m_start_marker, $m_continue_marker
;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	VERTICAL_PATTERN_setup			  $m_src, $m_SRC_BYTES, $m_srcStep, $m_dst, $m_DST_BYTES, $m_dstStep
	VERTICAL_PATTERN		$PATTERN, $m_src, $m_SRC_BYTES, $m_LDRX, $m_srcStep, $m_dst, $m_DST_BYTES, $m_STRX, $m_dstStep, $m_GET_HEIGHT, $m_STACK_OFFSET, $m_start_marker, $m_continue_marker

	MEND

;;;;;;;;;;;;;;;;;;;;;;;;;;;;		
	MACRO
		VERTICAL_PATTERN_long_step_no $PATTERN, $m_src, $m_SRC_BYTES, $m_LDRX, $m_srcStep, $m_dst, $m_DST_BYTES, $m_STRX, $m_dstStep, $m_GET_WIDTH, $m_GET_HEIGHT, $m_STACK_OFFSET, $m_start_1_marker, $m_continue_1_marker, $m_start_2_marker, $m_continue_2_marker
;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	VERTICAL_PATTERN_setup			  $m_src, $m_SRC_BYTES, $m_srcStep, $m_dst, $m_DST_BYTES, $m_dstStep
	VERTICAL_PATTERN		$PATTERN, $m_src, $m_SRC_BYTES, $m_LDRX, $m_srcStep, $m_dst, $m_DST_BYTES, $m_STRX, $m_dstStep, $m_GET_HEIGHT, $m_STACK_OFFSET, $m_start_1_marker, $m_continue_1_marker

	MEND


;;;;;;;;;;;;;;;;;;;;;;;;;;;;		
	MACRO
		VERTICAL_PATTERN_long_step $PATTERN, $m_src, $m_SRC_BYTES, $m_LDRX, $m_srcStep, $m_dst, $m_DST_BYTES, $m_STRX, $m_dstStep, $m_GET_WIDTH, $m_GET_HEIGHT, $m_STACK_OFFSET, $m_start_1_marker, $m_continue_1_marker, $m_start_2_marker, $m_continue_2_marker
;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;height == 4 not supported

;dst_chunk	RN 11
;src_chunk	RN 12	;shares with mm

;height		RN 12	;most likely
;width		RN 14
	
	mov	height, height, lsr #1
	str			height, [sp, #8]
	VERTICAL_PATTERN_setup			  $m_src, $m_SRC_BYTES, $m_srcStep, $m_dst, $m_DST_BYTES, $m_dstStep
	VERTICAL_PATTERN		$PATTERN, $m_src, $m_SRC_BYTES, $m_LDRX, $m_srcStep, $m_dst, $m_DST_BYTES, $m_STRX, $m_dstStep, GET_HEIGHT_8, 0, $m_start_1_marker, $m_continue_1_marker

	$m_GET_WIDTH $m_STACK_OFFSET
	add		$m_dst, $m_dst, dstStep
	sub		width, width, #1	
	sub		$m_dst, $m_dst, width
	add		$m_dst, $m_dst, dst_chunk
	
	sub		$m_src, $m_src, $m_srcStep, lsl #2
	sub		$m_src, $m_src, width
	add		$m_src, $m_src, src_chunk

	VERTICAL_PATTERN		$PATTERN, $m_src, $m_SRC_BYTES, $m_LDRX, $m_srcStep, $m_dst, $m_DST_BYTES, $m_STRX, $m_dstStep, GET_HEIGHT_8, 0, $m_start_2_marker, $m_continue_2_marker	
	
	MEND


;;;;;;;;;;;;;;;;;;;;;;;;;;;;		
	MACRO
		VERTICAL_PATTERN_4_setup $m_src, $m_SRC_BYTES, $m_srcStep, $m_dst, $m_DST_BYTES, $m_dstStep
;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	mov		dst_chunk, $m_dstStep, lsl #2				;dst_chunk shares tmp0
	sub		dst_chunk, dst_chunk, $m_dstStep
	sub		dst_chunk, dst_chunk, #$m_DST_BYTES
	str		dst_chunk, [sp, #0]
	
	mov		src_chunk, $m_srcStep, lsl #2				;src_chunk shares mm
	add		src_chunk, src_chunk, $m_srcStep, lsl #2
	sub		src_chunk, src_chunk, #$m_SRC_BYTES
	str	    src_chunk, [sp, #4]
	add     $m_dst, $m_dst, dst_chunk
	sub     $m_src, $m_src, $m_srcStep, lsl #1
	add     $m_src, $m_src, src_chunk

	MEND

;;;;;;;;;;;;;;;;;;;;;;;;;;;;		
	MACRO
		VERTICAL_PATTERN_4 $PATTERN, $m_src, $m_SRC_BYTES, $m_LDRX, $m_srcStep, $m_dst, $m_DST_BYTES, $m_STRX, $m_dstStep, $m_start_marker
;;;;;;;;;;;;;;;;;;;;;;;;;;;;

$m_start_marker
	sub		$m_src, $m_src, src_chunk
	sub		$m_dst, $m_dst, dst_chunk	
	
	$m_LDRX	t0, [$m_src]
	$m_LDRX	t1, [$m_src, $m_srcStep]!
	$m_LDRX	t2, [$m_src, $m_srcStep]!
	$m_LDRX	t3, [$m_src, $m_srcStep]!
	$m_LDRX	t4, [$m_src, $m_srcStep]!
	$m_LDRX	t5, [$m_src, $m_srcStep]!
	$PATTERN tmp0,  t0, t1, t2, t3, t4, t5
	$m_STRX	tmp0, [$m_dst]	  
	$m_LDRX	t0, [$m_src, $m_srcStep]!		
	$PATTERN tmp0,  t1, t2, t3, t4, t5, t0
	$m_STRX	tmp0, [$m_dst, $m_dstStep]!	  
	$m_LDRX	t1, [$m_src, $m_srcStep]!		
	$PATTERN tmp0,  t2, t3, t4, t5, t0, t1
	$m_STRX	tmp0, [$m_dst, $m_dstStep]!	  
	$m_LDRX	t2, [$m_src, $m_srcStep]!		
	$PATTERN tmp0,  t3, t4, t5, t0, t1, t2

	ldr		src_chunk, [sp, #4]		;src_chunk shares mm
	$m_STRX	tmp0, [$m_dst, $m_dstStep]!	  

	ldr		dst_chunk, [sp, #0]		;dst_chunk shares tmp0
	subs	width, width, #1
 	bpl		$m_start_marker

	MEND
	

;;;;;;;;;;;;;;;;;;;;;;;;;;;;		
	MACRO
		VERTICAL_PATTERN_4_long_step $PATTERN, $m_src, $m_SRC_BYTES, $m_LDRX, $m_srcStep, $m_dst, $m_DST_BYTES, $m_STRX, $m_dstStep, $m_GET_WIDTH, $m_GET_HEIGHT, $m_STACK_OFFSET, $m_start_1_marker, $m_start_2_marker, $m_start_3_marker, $m_start_4_marker, $m_end_marker
;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;dst_chunk	RN 11
;src_chunk	RN 12	;shares with mm

;height		RN 12	;most likely
;width		RN 14
	
	VERTICAL_PATTERN_4_setup		  $m_src, $m_SRC_BYTES, $m_srcStep, $m_dst, $m_DST_BYTES, $m_dstStep
	VERTICAL_PATTERN_4		$PATTERN, $m_src, $m_SRC_BYTES, $m_LDRX, $m_srcStep, $m_dst, $m_DST_BYTES, $m_STRX, $m_dstStep, $m_start_1_marker

	$m_GET_HEIGHT $m_STACK_OFFSET
	cmp		height, #4				;height shares mm
	beq		$m_end_marker

	$m_GET_WIDTH  $m_STACK_OFFSET	
	add		$m_dst, $m_dst, dstStep
	sub		width, width, #1	
	sub		$m_dst, $m_dst, width
	add		$m_dst, $m_dst, dst_chunk
	
	sub		$m_src, $m_src, $m_srcStep, lsl #2
	sub		$m_src, $m_src, width
	add		$m_src, $m_src, src_chunk

	VERTICAL_PATTERN_4		$PATTERN, $m_src, $m_SRC_BYTES, $m_LDRX, $m_srcStep, $m_dst, $m_DST_BYTES, $m_STRX, $m_dstStep, $m_start_2_marker

	$m_GET_HEIGHT $m_STACK_OFFSET
	cmp		height, #8				;height shares mm
	beq		$m_end_marker

	$m_GET_WIDTH  $m_STACK_OFFSET	
	add		$m_dst, $m_dst, dstStep
	sub		width, width, #1	
	sub		$m_dst, $m_dst, width
	add		$m_dst, $m_dst, dst_chunk
	
	sub		$m_src, $m_src, $m_srcStep, lsl #2
	sub		$m_src, $m_src, width
	add		$m_src, $m_src, src_chunk

	VERTICAL_PATTERN_4		$PATTERN, $m_src, $m_SRC_BYTES, $m_LDRX, $m_srcStep, $m_dst, $m_DST_BYTES, $m_STRX, $m_dstStep, $m_start_3_marker

	$m_GET_WIDTH  $m_STACK_OFFSET	
	add		$m_dst, $m_dst, dstStep
	sub		width, width, #1	
	sub		$m_dst, $m_dst, width
	add		$m_dst, $m_dst, dst_chunk
	
	sub		$m_src, $m_src, $m_srcStep, lsl #2
	sub		$m_src, $m_src, width
	add		$m_src, $m_src, src_chunk

	VERTICAL_PATTERN_4		$PATTERN, $m_src, $m_SRC_BYTES, $m_LDRX, $m_srcStep, $m_dst, $m_DST_BYTES, $m_STRX, $m_dstStep, $m_start_4_marker

$m_end_marker
	
	MEND



;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;	
|ippiInterpolateLuma_H264_8u_C1R_arm| PROC
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

dx		RN 4
dy		RN 5
w		RN 6
h		RN 7
idx		RN 8
offset	RN 9
addr	RN 9

	MY_ENTER 
	ldr		dx, [sp, #36]
	ldr		dy, [sp, #40]
	add		idx, dx, dy, asl #2
	add		addr, pc, idx, asl #2		
	ldr		addr, [addr, #4]			;ldr	pc, [addr] ;could have done this...
	bx		addr	
	ENDP  ; |ippiInterpolateLuma_H264_8u_C1R_arm|

luma_case_array_subroutines
	DCD	case_0_arm				
	DCD	case_1_arm
	DCD	case_2_arm
	DCD	case_3_arm
	DCD	case_4_short_step_arm
	DCD	case_5_7_13_15_arm	
	DCD	case_6_arm			
	DCD	case_5_7_13_15_arm	
	DCD	case_8_short_step_arm			
	DCD	case_9_arm			
	DCD	case_10_arm			
	DCD	case_11_arm			
	DCD	case_12_short_step_arm
	DCD	case_5_7_13_15_arm
	DCD	case_14_arm			
	DCD	case_5_7_13_15_arm


	
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;	
|ippiInterpolateLuma_H264_8u_C1R_16x16_arm| PROC
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

dx		RN 4
dy		RN 5
w		RN 6
h		RN 7
idx		RN 8
offset	RN 9
addr	RN 9

	MY_ENTER 
	ldr		dx, [sp, #36]
	ldr		dy, [sp, #40]
	add		idx, dx, dy, asl #2
	add		addr, pc, idx, asl #2		
	ldr		addr, [addr, #4]			;ldr	pc, [addr] ;could have done this...
	bx		addr	
	ENDP  ; |ippiInterpolateLuma_H264_8u_C1R_16x16_arm|

luma_16x16_case_array_subroutines
	DCD	case_0_16x16_arm				
	DCD	case_1_arm
	DCD	case_2_arm
	DCD	case_3_arm
	DCD	case_4_long_step_arm
	DCD	case_5_7_13_15_arm	
	DCD	case_6_arm			
	DCD	case_5_7_13_15_arm	
	DCD	case_8_long_step_arm			
	DCD	case_9_arm			
	DCD	case_10_arm			
	DCD	case_11_arm			
	DCD	case_12_long_step_arm
	DCD	case_5_7_13_15_arm
	DCD	case_14_arm			
	DCD	case_5_7_13_15_arm


	
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;	
|ippiInterpolateLuma_H264_8u_C1R_8x16_arm| PROC
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

dx		RN 4
dy		RN 5
w		RN 6
h		RN 7
idx		RN 8
offset	RN 9
addr	RN 9

	MY_ENTER 
	ldr		dx, [sp, #36]
	ldr		dy, [sp, #40]
	add		idx, dx, dy, asl #2
	add		addr, pc, idx, asl #2		
	ldr		addr, [addr, #4]			;ldr	pc, [addr] ;could have done this...
	bx		addr	
	ENDP  ; |ippiInterpolateLuma_H264_8u_C1R_16x16_arm|

luma_8x16_case_array_subroutines
	DCD	case_0_8x16_arm				
	DCD	case_1_arm
	DCD	case_2_arm
	DCD	case_3_arm
	DCD	case_4_long_step_arm
	DCD	case_5_7_13_15_arm	
	DCD	case_6_arm			
	DCD	case_5_7_13_15_arm	
	DCD	case_8_long_step_arm			
	DCD	case_9_arm			
	DCD	case_10_arm			
	DCD	case_11_arm			
	DCD	case_12_long_step_arm
	DCD	case_5_7_13_15_arm
	DCD	case_14_arm			
	DCD	case_5_7_13_15_arm



;;;;;;;;;;;;;;;;;;;;;;;;;;;;	
|case_0_16x16_arm| PROC
;;;;;;;;;;;;;;;;;;;;;;;;;;;;	

height		RN 14

	mov		height, #15
	sub		src, src, srcStep
	sub		dst, dst, dstStep
	tst		src, #3
	bne		start_case_0_16x16_arm_u8

start_case_0_16x16_arm_u32	
	ldr		t0, [src, srcStep]!
	ldr		t1, [src, #4]
	ldr		t2, [src, #8]
	ldr		t3, [src, #12]
	str		t0, [dst, dstStep]!
	str		t1, [dst, #4]
	str		t2, [dst, #8]
	str		t3, [dst, #12]
	subs	height, height, #1
	bpl		start_case_0_16x16_arm_u32	
	MY_RETURN 

start_case_0_16x16_arm_u8

	ldrb    t0, [src, srcStep]!
	ldrb    t3, [src, #3]
	ldrb    t2, [src, #2]
	ldrb    t1, [src, #1]
	orr     t3, t2, t3, lsl #8 
	orr     t3, t1, t3, lsl #8 
	orr     t3, t0, t3, lsl #8 
	str		t3, [dst, dstStep]!
	
	ldrb    t3, [src, #7]
	ldrb    t2, [src, #6]
	ldrb    t1, [src, #5]
	ldrb    t0, [src, #4]
	orr     t3, t2, t3, lsl #8 
	orr     t3, t1, t3, lsl #8 
	orr     t3, t0, t3, lsl #8 
	str		t3, [dst, #4]
	
	ldrb    t3, [src, #11]
	ldrb    t2, [src, #10]
	ldrb    t1, [src, #9]
	ldrb    t0, [src, #8]
	orr     t3, t2, t3, lsl #8 
	orr     t3, t1, t3, lsl #8 
	orr     t3, t0, t3, lsl #8 
	str		t3,  [dst, #8]

	ldrb    t3, [src, #15]
	ldrb    t2, [src, #14]
	ldrb    t1, [src, #13]
	ldrb    t0, [src, #12]
	orr     t3, t2, t3, lsl #8 
	orr     t3, t1, t3, lsl #8 
	orr     t3, t0, t3, lsl #8 
	str		t3, [dst, #12]

	subs	height, height, #1
 	bpl		start_case_0_16x16_arm_u8	
	
 	MY_RETURN

	ENDP  ; |case_0_16x16_arm|


;;;;;;;;;;;;;;;;;;;;;;;;;;;;	
|case_0_8x16_arm| PROC
;;;;;;;;;;;;;;;;;;;;;;;;;;;;	

height		RN 14

	mov		height, #15
	sub		src, src, srcStep
	sub		dst, dst, dstStep
	tst		src, #3
	bne		start_case_0_8x16_arm_u8

start_case_0_8x16_arm_u32	
	ldr		t0, [src, srcStep]!
	ldr		t1, [src, #4]
	str		t0, [dst, dstStep]!
	str		t1, [dst, #4]
	subs	height, height, #1
	bpl		start_case_0_8x16_arm_u32	
	MY_RETURN 

start_case_0_8x16_arm_u8

	ldrb    t0, [src, srcStep]!
	ldrb    t3, [src, #3]
	ldrb    t2, [src, #2]
	ldrb    t1, [src, #1]
	orr     t3, t2, t3, lsl #8 
	orr     t3, t1, t3, lsl #8 
	orr     t3, t0, t3, lsl #8 
	str		t3, [dst, dstStep]!
	
	ldrb    t3, [src, #7]
	ldrb    t2, [src, #6]
	ldrb    t1, [src, #5]
	ldrb    t0, [src, #4]
	orr     t3, t2, t3, lsl #8 
	orr     t3, t1, t3, lsl #8 
	orr     t3, t0, t3, lsl #8 
	str		t3, [dst, #4]
	
	subs	height, height, #1
 	bpl		start_case_0_8x16_arm_u8	
	
 	MY_RETURN

	ENDP  ; |case_0_8x16_arm|


;;;;;;;;;;;;;;;;;;;;;;;;;;;;	
|case_0_arm| PROC
;;;;;;;;;;;;;;;;;;;;;;;;;;;;	
width		RN 12		;shares mm
height		RN 14

	GET_WIDTH_sp 0	
	GET_HEIGHT_sp 0	
	sub		height, height, #1
	sub		src, src, srcStep
	sub		dst, dst, dstStep
	tst		src, #3
	bne		start_case_0_arm_u8

start_case_0_arm_u32	
	ldr		t0, [src, srcStep]!
	str		t0, [dst, dstStep]!
	
	cmp		width, #4
	beq		continue_case_0_arm_u32
	
	ldr		t1, [src, #4]
	str		t1, [dst, #4]

	cmp		width, #8
	beq		continue_case_0_arm_u32

	ldr		t2, [src, #8]
	ldr		t3, [src, #12]
	str		t2, [dst, #8]
	str		t3, [dst, #12]

continue_case_0_arm_u32

	subs	height, height, #1
	bpl		start_case_0_arm_u32	
	
	MY_RETURN 

start_case_0_arm_u8

	ldrb    t0, [src, srcStep]!
	ldrb    t3, [src, #3]
	ldrb    t2, [src, #2]
	ldrb    t1, [src, #1]
	orr     t3, t2, t3, lsl #8 
	orr     t3, t1, t3, lsl #8 
	orr     t3, t0, t3, lsl #8 
	str		t3, [dst, dstStep]!

	cmp		width, #4
	beq		continue_case_0_arm_u8
	
	ldrb    t3, [src, #7]
	ldrb    t2, [src, #6]
	ldrb    t1, [src, #5]
	ldrb    t0, [src, #4]
	orr     t3, t2, t3, lsl #8 
	orr     t3, t1, t3, lsl #8 
	orr     t3, t0, t3, lsl #8 
	str		t3, [dst, #4]

	cmp		width, #8
	beq		continue_case_0_arm_u8
	
	ldrb    t3, [src, #11]
	ldrb    t2, [src, #10]
	ldrb    t1, [src, #9]
	ldrb    t0, [src, #8]
	orr     t3, t2, t3, lsl #8 
	orr     t3, t1, t3, lsl #8 
	orr     t3, t0, t3, lsl #8 
	str		t3,  [dst, #8]

	ldrb    t3, [src, #15]
	ldrb    t2, [src, #14]
	ldrb    t1, [src, #13]
	ldrb    t0, [src, #12]
	orr     t3, t2, t3, lsl #8 
	orr     t3, t1, t3, lsl #8 
	orr     t3, t0, t3, lsl #8 
	str		t3, [dst, #12]

continue_case_0_arm_u8

	subs	height, height, #1
 	bpl		start_case_0_arm_u8	
	
 	MY_RETURN

	ENDP  ; |case_0_arm|


;;;;;;;;;;;;;;;
;GROUP 1, 3, 2;
;;;;;;;;;;;;;;;
		
;;;;;;;;;;;;;;;;;;;;;;;;;;;;	
|case_1_arm| PROC
;;;;;;;;;;;;;;;;;;;;;;;;;;;;	

width		RN 12		;shares mm
height		RN 14

	GET_HEIGHT_sp 0	
	mov		CNST_255, #255
	sub		src, src, srcStep
	sub		src, src, #2
	sub		dst, dst, dstStep

	sub		height, height, #1
	HORIZONTAL_PATTERN FILTER_AND_CLIP_t2, src, 1, ldrb, srcStep, dst, 1, strb, dstStep, GET_WIDTH_sp, 0, start_case_1_arm	, continue_hor_case_1_arm
		
 	MY_RETURN 
	ENDP  ; |case_1_arm|

;;;;;;;;;;;;;;;;;;;;;;;;;;;;	
|case_3_arm| PROC
;;;;;;;;;;;;;;;;;;;;;;;;;;;;	

	GET_HEIGHT_sp 0	
	mov		CNST_255, #255
	sub		src, src, srcStep
	sub		src, src, #2
	sub		dst, dst, dstStep

	sub		height, height, #1
	HORIZONTAL_PATTERN FILTER_AND_CLIP_t3, src, 1, ldrb, srcStep, dst, 1, strb, dstStep, GET_WIDTH_sp, 0, start_case_3_arm, continue_hor_case_3_arm	
		
 	MY_RETURN 

	ENDP  ; |case_3_arm|


;;;;;;;;;;;;;;;;;;;;;;;;;;;;	
|case_2_arm| PROC
;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	
	GET_HEIGHT_sp 0	
	mov		CNST_255, #255
	sub		src, src, srcStep
	sub		src, src, #2
	sub		dst, dst, dstStep

	sub		height, height, #1
	HORIZONTAL_PATTERN FILTER_AND_CLIP, src, 1, ldrb, srcStep, dst, 1, strb, dstStep, GET_WIDTH_sp, 0, start_case_2_arm, continue_hor_case_2_arm
		
 	MY_RETURN 

	ENDP  ; |case_2_arm|


;;;;;;;;;;;;;;;;
;GROUP 4, 12, 8;
;;;;;;;;;;;;;;;;

;;;;;;;;;;;;;;;;;;;;;;;;;;;;	
|case_4_long_step_arm| PROC
;;;;;;;;;;;;;;;;;;;;;;;;;;;;	

;t0		    RN 4
;t1			RN 5
;t2			RN 6
;t3			RN 7
;t4			RN 8
;t5			RN 9
;CNST_255	RN 10
;tmp0		RN 11
;mm			RN 12

dst_chunk	RN 11
src_chunk	RN 12	;shared with mm
height		RN 12
width		RN 14

	GET_WIDTH_sp  0
	GET_HEIGHT_sp 0	
	mov		CNST_255, #255
		
	sub		width, width, #1	
	sub		sp, sp, #12
	;VERTICAL_PATTERN_long_step FILTER_AND_CLIP_t2, src, 1, ldrb, srcStep, dst, 1, strb, dstStep, GET_WIDTH_sp, GET_HEIGHT_sp, 12, start_case_4_long_step_1_arm, continue_case_4_long_step_1_arm, start_case_4_long_2_arm, continue_case_4_long_2_arm	
	VERTICAL_PATTERN_4_long_step FILTER_AND_CLIP_t2, src, 1, ldrb, srcStep, dst, 1, strb, dstStep, GET_WIDTH_sp, GET_HEIGHT_sp, 12, start_case_4_long_step_1_arm, start_case_4_long_step_2_arm, start_case_4_long_step_3_arm, start_case_4_long_step_4_arm, end_case_4_long_step__arm
	add		sp, sp, #12

 	MY_RETURN 

	ENDP  ; |case_4_arm|

;;;;;;;;;;;;;;;;;;;;;;;;;;;;	
|case_4_short_step_arm| PROC
;;;;;;;;;;;;;;;;;;;;;;;;;;;;	

	GET_WIDTH_sp  0
	GET_HEIGHT_sp 0	
	mov		CNST_255, #255
		
	sub		width, width, #1	
	sub		sp, sp, #8
	VERTICAL_PATTERN_short_step FILTER_AND_CLIP_t2, src, 1, ldrb, srcStep, dst, 1, strb, dstStep, GET_HEIGHT_sp, 8, start_case_4_short_step_arm, continue_case_4_short_step_arm	
	add		sp, sp, #8

 	MY_RETURN 

	ENDP  ; |case_4_arm|


;;;;;;;;;;;;;;;;;;;;;;;;;;;;	
|case_12_short_step_arm| PROC
;;;;;;;;;;;;;;;;;;;;;;;;;;;;	

	GET_WIDTH_sp  0
	GET_HEIGHT_sp 0	
	mov		CNST_255, #255
	
	sub		width, width, #1	
	sub		sp, sp, #8
	VERTICAL_PATTERN_short_step FILTER_AND_CLIP_t3, src, 1, ldrb, srcStep, dst, 1, strb, dstStep, GET_HEIGHT_sp, 8, start_case_12_short_step_arm, continue_case_12_short_step_arm	
	add		sp, sp, #8
	
 	MY_RETURN

	ENDP  ; |case_12_arm|
	

;;;;;;;;;;;;;;;;;;;;;;;;;;;;	
|case_12_long_step_arm| PROC
;;;;;;;;;;;;;;;;;;;;;;;;;;;;	

	GET_WIDTH_sp  0
	GET_HEIGHT_sp 0	
	mov		CNST_255, #255
	
	sub		width, width, #1	
	sub		sp, sp, #12
	;VERTICAL_PATTERN_long_step FILTER_AND_CLIP_t3, src, 1, ldrb, srcStep, dst, 1, strb, dstStep, GET_WIDTH_sp, GET_HEIGHT_sp, 12, start_case_12_long_1_arm, continue_case_12_long_1_arm, start_case_12_long_2_arm, continue_case_12_long_2_arm	
	VERTICAL_PATTERN_4_long_step FILTER_AND_CLIP_t3, src, 1, ldrb, srcStep, dst, 1, strb, dstStep, GET_WIDTH_sp, GET_HEIGHT_sp, 12, start_case_12_long_1_arm, start_case_12_long_2_arm, start_case_12_long_3_arm, start_case_12_long_4_arm, end_case_12_long_arm	
	add		sp, sp, #12
	
 	MY_RETURN

	ENDP  ; |case_12_arm|
	
	
;;;;;;;;;;;;;;;;;;;;;;;;;;;;	
|case_8_long_step_arm| PROC
;;;;;;;;;;;;;;;;;;;;;;;;;;;;	

	GET_WIDTH_sp  0
	GET_HEIGHT_sp 0	
	mov		CNST_255, #255
	
	sub		width, width, #1
	sub		sp, sp, #12
	;VERTICAL_PATTERN_long_step FILTER_AND_CLIP, src, 1, ldrb, srcStep, dst, 1, strb, dstStep, GET_WIDTH_sp, GET_HEIGHT_sp, 12, start_case_8_long_1_arm, continue_case_8_long_1_arm, start_case_8_long_2_arm, continue_case_8_long_2_arm	
	VERTICAL_PATTERN_4_long_step FILTER_AND_CLIP, src, 1, ldrb, srcStep, dst, 1, strb, dstStep, GET_WIDTH_sp, GET_HEIGHT_sp, 12, start_case_8_long_1_arm, start_case_8_long_2_arm, start_case_8_long_3_arm, start_case_8_long_4_arm, end_case_8_long_arm
	add		sp, sp, #12

 	MY_RETURN
 	
 	ENDP  ; |case_8_arm|
	

;;;;;;;;;;;;;;;;;;;;;;;;;;;;	
|case_8_short_step_arm| PROC
;;;;;;;;;;;;;;;;;;;;;;;;;;;;	

	GET_WIDTH_sp  0
	GET_HEIGHT_sp 0	
	mov		CNST_255, #255
	
	sub		width, width, #1
	sub		sp, sp, #8
	VERTICAL_PATTERN_short_step FILTER_AND_CLIP, src, 1, ldrb, srcStep, dst, 1, strb, dstStep, GET_HEIGHT_sp, 8, start_case_8_short_step_arm, continue_case_8_short_step_arm	
	add		sp, sp, #8

 	MY_RETURN
 	
 	ENDP  ; |case_8_arm|
	

;;;;;;;;;;;;;;;;;;;;;;;;;;;;	
|case_6_arm| PROC
;;;;;;;;;;;;;;;;;;;;;;;;;;;;	

;t0		    RN 4
;t1			RN 5
;t2			RN 6
;t3			RN 7
;t4			RN 8
;t5			RN 9
;CNST_255	RN 10
;tmp0		RN 11
;mm			RN 12

;dst_chunk	RN 11
;src_chunk	RN 12

srcStep		RN 1		;switch srcStep and dstStep back
dstStep		RN 3		;
width		RN 10		;shares CNST_255
height		RN 14

	GET_WIDTH_sp  0
	GET_HEIGHT_sp 0	
	
	sub		sp, sp, #(MB_MAX*TMP_XY*2+8)						;stack moved up by 8 bytes
	str		dst, [sp, #0]
	str		dstStep, [sp, #4]
	
	sub		src, src, srcStep, lsl #1
	sub		src, src, srcStep
	sub		src, src, #2

	mov		dstStep, width, lsl #1
	add		dst, sp, #8
	sub		dst, dst, dstStep
	add		height, height, #5				;sub		height, height, #1
	
	;height as a counter, changed after the call
	;width shares mm flushed after the call
	;no sp change in HORIZONTAL_PATTERN
	HORIZONTAL_PATTERN FILTER_ONE, src, 1, ldrb, srcStep, dst, 2, strh, dstStep, NOP, NOP, start_hor_case_6_arm, continue_case_6_arm

srcStep		RN 3		;switch srcStep and dstStep automatically
dstStep		RN 1		;
height		RN 12
width		RN 14

	ldr		dst, [sp, #0]
	ldr		dstStep, [sp, #4]
	add		src, sp, #8
	add		src, src, srcStep, lsl #1
	GET_HEIGHT_sp	(MB_MAX*TMP_XY*2+8)	;height shares mm flushed after the call	
	mov		width,  srcStep, lsr #1				;srcStep holds width<<1	
	sub		width, width, #1					;width as a counter, changed after the call
	mov		CNST_255, #255
			
	VERTICAL_PATTERN_short_step FILTER_AND_CLIP_hi_t2, src, 2, ldrsh, srcStep, dst, 1, strb, dstStep, GET_HEIGHT_sp, MB_MAX*TMP_XY*2+8, start_ver_case_6_arm, continue_ver_case_6_arm		

 	add		sp, sp, #(MB_MAX*TMP_XY*2+8)

 	MY_RETURN
	
	ENDP  ; |case_6_arm|


;;;;;;;;;;;;;;;;;;;;;;;;;;;;	
|case_14_arm| PROC
;;;;;;;;;;;;;;;;;;;;;;;;;;;;	

srcStep		RN 1		;switch srcStep and dstStep back
dstStep		RN 3		;
width		RN 10		;shares CNST_255
height		RN 14

	GET_WIDTH_sp  0
	GET_HEIGHT_sp 0	
	
	sub		sp, sp, #(MB_MAX*TMP_XY*2+8)						;stack moved up by 8 bytes
	str		dst, [sp, #0]
	str		dstStep, [sp, #4]
	
	sub		src, src, srcStep, lsl #1
	sub		src, src, srcStep
	sub		src, src, #2

	mov		dstStep, width, lsl #1
	add		dst, sp, #8
	sub		dst, dst, dstStep
	add		height, height, #5				;sub		height, height, #1
	
	;height as a counter, changed after the call
	;width shares mm flushed after the call
	;no sp change in HORIZONTAL_PATTERN
	HORIZONTAL_PATTERN FILTER_ONE, src, 1, ldrb, srcStep, dst, 2, strh, dstStep, NOP, NOP, start_hor_case_14_arm, continue_hor_case_14_arm

srcStep		RN 3		;switch srcStep and dstStep automatically
dstStep		RN 1		;
height		RN 12
width		RN 14

	ldr		dst, [sp, #0]
	ldr		dstStep, [sp, #4]
	add		src, sp, #8
	add		src, src, srcStep, lsl #1
	GET_HEIGHT_sp	(MB_MAX*TMP_XY*2+8)		;height shares mm flushed after the call	
	mov		width,  srcStep, lsr #1			;srcStep holds width<<1	
	sub		width, width, #1				;width as a counter, changed after the call
	mov		CNST_255, #255
			
	VERTICAL_PATTERN_short_step FILTER_AND_CLIP_hi_t3, src, 2, ldrsh, srcStep, dst, 1, strb, dstStep, GET_HEIGHT_sp, MB_MAX*TMP_XY*2+8, start_ver_case_14_arm, continue_ver_case_14_arm		

 	add		sp, sp, #(MB_MAX*TMP_XY*2+8)

 	MY_RETURN
	
	ENDP  ; |case_14_arm|


;;;;;;;;;;;;;;;;;;;;;;;;;;;;	
|case_10_arm| PROC
;;;;;;;;;;;;;;;;;;;;;;;;;;;;	

srcStep		RN 1		;switch srcStep and dstStep back
dstStep		RN 3		;
height		RN 10		;shares CNST_255
width		RN 14

	GET_WIDTH_sp  0
	GET_HEIGHT_sp 0	
	
	sub		sp, sp, #(MB_MAX*TMP_XY*2+16)						;stack moved up by 8 bytes
	str		dst, [sp, #8]
	str		dstStep, [sp, #12]
	add		dst, sp, #16
	
	sub		src, src, #2
	add		width, width, #6				;sub		height, height, #1
	mov		dstStep, width, lsl #1
	sub		width, width, #1
	VERTICAL_PATTERN_short_step FILTER_ONE, src, 1, ldrb, srcStep, dst, 2, strh, dstStep, NOP, NOP, start_ver_case_10_arm, continue_ver_case_10_arm		
	
srcStep		RN 3		;switch srcStep and dstStep automatically
dstStep		RN 1		;
width		RN 12
height		RN 14
	
	ldr		dst, [sp, #8]
	ldr		dstStep, [sp, #12]
	GET_HEIGHT_sp	(MB_MAX*TMP_XY*2+16)	;height shares mm flushed after the call	
	add		src, sp, #16
	sub		src, src, srcStep
	mov		width, srcStep, lsr #1				;srcStep holds width<<1	
	mov		CNST_255, #255
	sub		height, height, #1					;width as a counter, changed after the call
	sub		dst, dst, dstStep
	
	HORIZONTAL_PATTERN FILTER_AND_CLIP_hi, src, 2, ldsh, srcStep, dst, 1, strb, dstStep, GET_WIDTH_sp, MB_MAX*TMP_XY*2+16, start_hor_case_10_arm, continue_hor_case_10_arm

 	add		sp, sp, #(MB_MAX*TMP_XY*2+16)

	MY_RETURN

	ENDP  ; |case_10_arm|


;;;;;;;;;;;;;;;;;;;;;;;;;;;;	
|case_9_arm| PROC
;;;;;;;;;;;;;;;;;;;;;;;;;;;;	

srcStep		RN 1		;switch srcStep and dstStep back
dstStep		RN 3		;
height		RN 10		;shares CNST_255
width		RN 14

	GET_WIDTH_sp  0
	GET_HEIGHT_sp 0	
	
	sub		sp, sp, #(MB_MAX*TMP_XY*2+16)						;stack moved up by 8 bytes
	str		dst, [sp, #8]
	str		dstStep, [sp, #12]
	add		dst, sp, #16
	
	sub		src, src, #2
	add		width, width, #6				;sub		height, height, #1
	mov		dstStep, width, lsl #1
	sub		width, width, #1
	VERTICAL_PATTERN_short_step FILTER_ONE, src, 1, ldrb, srcStep, dst, 2, strh, dstStep, NOP, NOP, start_ver_case_9_arm, continue_ver_case_9_arm		
	
srcStep		RN 3		;switch srcStep and dstStep automatically
dstStep		RN 1		;
width		RN 12
height		RN 14
	
	ldr		dst, [sp, #8]
	ldr		dstStep, [sp, #12]
	GET_HEIGHT_sp	(MB_MAX*TMP_XY*2+16)	;height shares mm flushed after the call	
	add		src, sp, #16
	sub		src, src, srcStep
	mov		width, srcStep, lsr #1				;srcStep holds width<<1	
	mov		CNST_255, #255
	sub		height, height, #1					;width as a counter, changed after the call
	sub		dst, dst, dstStep
	
	HORIZONTAL_PATTERN FILTER_AND_CLIP_hi_t2, src, 2, ldsh, srcStep, dst, 1, strb, dstStep, GET_WIDTH_sp, MB_MAX*TMP_XY*2+16, start_hor_case_9_arm, continue_hor_case_9_arm

 	add		sp, sp, #(MB_MAX*TMP_XY*2+16)

	MY_RETURN

	ENDP  ; |case_9_arm|


;;;;;;;;;;;;;;;;;;;;;;;;;;;;	
|case_11_arm| PROC
;;;;;;;;;;;;;;;;;;;;;;;;;;;;	

srcStep		RN 1		;switch srcStep and dstStep back
dstStep		RN 3		;
height		RN 10		;shares CNST_255
width		RN 14

	GET_WIDTH_sp  0
	GET_HEIGHT_sp 0	
	
	sub		sp, sp, #(MB_MAX*TMP_XY*2+16)						;stack moved up by 8 bytes
	str		dst, [sp, #8]
	str		dstStep, [sp, #12]
	add		dst, sp, #16
	
	sub		src, src, #2
	add		width, width, #6				;sub		height, height, #1
	mov		dstStep, width, lsl #1
	sub		width, width, #1
	VERTICAL_PATTERN_short_step FILTER_ONE, src, 1, ldrb, srcStep, dst, 2, strh, dstStep, NOP, NOP, start_ver_case_11_arm, continue_ver_case_11_arm		
	
srcStep		RN 3		;switch srcStep and dstStep automatically
dstStep		RN 1		;
width		RN 12
height		RN 14
	
	ldr		dst, [sp, #8]
	ldr		dstStep, [sp, #12]
	GET_HEIGHT_sp	(MB_MAX*TMP_XY*2+16)	;height shares mm flushed after the call	
	add		src, sp, #16
	sub		src, src, srcStep
	mov		width, srcStep, lsr #1				;srcStep holds width<<1	
	mov		CNST_255, #255
	sub		height, height, #1					;width as a counter, changed after the call
	sub		dst, dst, dstStep
	
	HORIZONTAL_PATTERN FILTER_AND_CLIP_hi_t3, src, 2, ldsh, srcStep, dst, 1, strb, dstStep, GET_WIDTH_sp, MB_MAX*TMP_XY*2+16, start_hor_case_11_arm, continue_hor_case_11_arm

 	add		sp, sp, #(MB_MAX*TMP_XY*2+16)

	MY_RETURN
	
	ENDP  ; |case_11_arm|


	
	
;;;;;;;;;;;;;;;;;;;;;;;;;;;;	
|case_5_7_13_15_arm| PROC
;;;;;;;;;;;;;;;;;;;;;;;;;;;;	

;t0		    RN 4
;t1			RN 5
;t2			RN 6
;t3			RN 7
;t4			RN 8
;t5			RN 9
;CNST_255	RN 10
;tmp0		RN 11
;mm			RN 12

;dst_chunk	RN 11
;src_chunk	RN 12

srcStep		RN 1
dstStep		RN 3
width		RN 12
height		RN 14
ddy			RN 4		;share t0
ddx			RN 4

	GET_WIDTH_sp	0
	GET_HEIGHT_sp	0	
	GET_DY_sp		0
	
	sub		sp, sp, #(MB_MAX*MB_MAX*2+24)						;stack moved up by 8 bytes
	str		src, [sp, #8]
	str		srcStep, [sp, #12]
	str		dst, [sp, #16]
	str		dstStep, [sp, #20]
	
	mov		ddy, ddy, lsr #1
	mul		tmp0, srcStep, ddy
	add		src, src, tmp0
	sub		src, src, srcStep
	sub		src, src, #2
	mov		CNST_255, #255

	mov		dstStep, width
	add		dst, sp, #24
	sub		dst, dst, dstStep
	sub		height, height, #1
	
	;height as a counter, changed after the call
	;width shares mm flushed after the call
	;no sp change in HORIZONTAL_PATTERN
	HORIZONTAL_PATTERN FILTER_AND_CLIP, src, 1, ldrb, srcStep, dst, 1, strb, dstStep, GET_WIDTH_sp, MB_MAX*MB_MAX*2+24, start_hor_case_5_arm, continue_hor_case_5_arm

width		RN 14
height		RN 12

	GET_DX_sp MB_MAX*MB_MAX*2+24
	GET_HEIGHT_sp	(MB_MAX*MB_MAX*2+24)	;height shares mm flushed after the call	
	ldr		src, [sp, #8]
	ldr		srcStep, [sp, #12]

	add		dst, sp, #(MB_MAX*MB_MAX+24)
	mov		width, dstStep					;srcStep holds width
	add		src, src, ddx, lsr #1
	sub		width, width, #1					;width as a counter, changed after the call

	VERTICAL_PATTERN_short_step FILTER_AND_CLIP, src, 1, ldrb, srcStep, dst, 1, strb, dstStep, GET_HEIGHT_sp, MB_MAX*MB_MAX*2+24, start_ver_case_5_arm, continue_ver_case_5_arm

srcStep		RN 3		;switch srcStep and dstStep automatically
dstStep		RN 1		;
mask		RN 8		;shares  t4
round		RN 9		;shares  t5
width		RN 10		;shares  CNST_255
height		RN 14

	GET_HEIGHT_sp	(MB_MAX*MB_MAX*2+24)	;height shares mm flushed after the call	
	ldr		dst, [sp, #16]
	ldr		dstStep, [sp, #20]
	ldr		mask,  mask_mark
	ldr		round, round_mark
	mov		width, srcStep					;srcStep holds width
	add		src, sp, #24
	sub		src, src, srcStep
	sub		dst, dst, dstStep

	sub		height, height, #1

start_average_case_5_arm	
	ldr		t0, [src, srcStep]!
	ldr		t1, [src, #(0+MB_MAX*MB_MAX)]
	AVERAGE_FOUR tmp0, t0, t1
	str		tmp0, [dst, dstStep]!
	
	cmp		width, #4
	beq		continue_average_case_5_arm
	
	ldr		t0, [src, #4]
	ldr		t1, [src, #(4+MB_MAX*MB_MAX)]
	AVERAGE_FOUR tmp0, t0, t1
	str		tmp0, [dst, #4]

	cmp		width, #8
	beq		continue_average_case_5_arm

	ldr		t0, [src, #8]
	ldr		t1, [src, #(8+MB_MAX*MB_MAX)]
	AVERAGE_FOUR tmp0, t0, t1
	str		tmp0, [dst, #8]

	ldr		t0, [src, #12]
	ldr		t1, [src, #(12+MB_MAX*MB_MAX)]
	AVERAGE_FOUR tmp0, t0, t1
	str		tmp0, [dst, #12]

continue_average_case_5_arm
	subs	height, height, #1
 	bpl		start_average_case_5_arm

 	add		sp, sp, #(MB_MAX*MB_MAX*2+24)
	MY_RETURN

mask_mark
    DCD		0x00ff00ff
round_mark	
	DCD		0x00010001	

	ENDP  ; |case_5_7_13_15_arm|


  END
