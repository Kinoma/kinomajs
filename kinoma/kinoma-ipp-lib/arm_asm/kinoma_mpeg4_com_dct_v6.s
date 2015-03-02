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
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	;armv6

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	AREA  |.text|, CODE, READONLY
	
 	EXPORT  |ippiDCT8x8Inv_16s_C1I_arm_v6|
 	EXPORT  |ippiDCT8x8Inv_16s8u_C1R_arm_v6|
 	EXPORT  |ippiDCT8x8Inv_4x4_16s_C1I_arm_v6|
 	EXPORT  |ippiDCT8x8Inv_4x4_16s8u_C1R_arm_v6|
 	EXPORT  |ippiDCT8x8Inv_2x2_16s_C1I_arm_v6|
 	EXPORT  |ippiDCT8x8Inv_2x2_16s8u_C1R_arm_v6|	

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

ROW_SHIFT		EQU			11
COL_SHIFT		EQU			20
W1				EQU			22725		;cos(i * M_PI / 16) * sqrt(2) * (1<<14) + 0.5
W2				EQU			21407		;cos(i * M_PI / 16) * sqrt(2) * (1<<14) + 0.5
W3				EQU			19266		;cos(i * M_PI / 16) * sqrt(2) * (1<<14) + 0.5
W4				EQU			16383		;cos(i * M_PI / 16) * sqrt(2) * (1<<14) + 0.5
W5				EQU			12873		;cos(i * M_PI / 16) * sqrt(2) * (1<<14) + 0.5
W6				EQU			8867		;cos(i * M_PI / 16) * sqrt(2) * (1<<14) + 0.5
W7				EQU			4520		;cos(i * M_PI / 16) * sqrt(2) * (1<<14) + 0.5

W31				EQU			(W1 | (W3 << 16))
W62				EQU			(W2 | (W6 << 16))
W75				EQU			(W5 | (W7 << 16))

W4n16			EQU			0xc001
W24				EQU			(W4 | (W2 << 16))
W24n			EQU			(W4n16| (-W2 << 16))
W64				EQU			(W4 | (W6 << 16))
W1n16			EQU			0xa73b
W51n			EQU			(W1n16| (-W5 << 16))
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


;;;;;;;;;;;;;;;;;;;;;;;;;;;;		
	MACRO
		idct_row_2_v6 $version, $dst, $offset
;;;;;;;;;;;;;;;;;;;;;;;;;;;;		
 
        ldr    a3, [a1, #(2*($offset+0))]					 ;a3,a4 = row[1:0]
		movs   v1, a3, lsr #16
        beq    $version.dc_only

        mov    v1, #(1<<(ROW_SHIFT-1))
        
		mov	   a2, a3, lsl #16			;v2 = row[0]<<16  =>  (row[0] * 16384)<<2
		add	   v1, v1, a2, asr #2		;v1 = v1 + v2>>2  =>  v1 = v1 + row[0] * 16384
		sub    v1, v1, a2, asr #16
		
        smultt v5, ip, a3
        smulbt v6, ip, a3
        smultt v7, v3, a3
        smulbt fp, v3, a3

		add   a2, v1, v6
		mov   a2, a2, lsl #5
		mov   a4, a2, lsr #16
		add   a2, v1, v5
		mov   v4, a2, lsr #11
		orr   a3, a4, v4, lsl #16
		add   a2, v1, fp
		mov   a2, a2, lsl #5
		mov   a4, a2, lsr #16
		add   a2, v1, v7
		mov   v4, a2, lsr #11
		orr   a4, a4, v4, lsl #16
        strd   a3, [$dst, #(2*($offset+0))]

		sub   a2, v1, v7
		mov   a2, a2, lsl #5
		mov   v4, a2, lsr #16
		sub   a2, v1, fp
		mov   a4, a2, lsr #11
		orr   a3, v4, a4, lsl #16
		sub   a2, v1, v5
		mov   a2, a2, lsl #5
		mov   v4, a2, lsr #16
		sub   a2, v1, v6
		mov   v1, a2, lsr #11
		orr   a4, v4, v1, lsl #16
        strd  a3, [$dst, #(2*($offset+4))]

		b $version.finish

$version.dc_only
        mov    a3, a3, lsl #19
        orr    a3, a3, a3, lsr #16
        mov    a4, a3
        strd   a3, [$dst, #(2*($offset+0))]
        strd   a3, [$dst, #(2*($offset+4))]

$version.finish
	MEND

;;;;;;;;;;;;;;;;;;;;;;;;;;;;		
	MACRO
		ROW_STEP_1 $version
;;;;;;;;;;;;;;;;;;;;;;;;;;;;		
	    ldr    r5,  long_w24_$version	;W24
        ldr    r6,  long_w62_$version	;W62
        ldr    r14, long_w75_$version	;W75
        mov    r4, #(1<<(ROW_SHIFT-1))	;round						   => round
        smlabb r4, r5, r2, r4			;w4*r0 + round				   => a0
        smultb r5, r6, r3				;w6*r2					     
        smulbb r1, r6, r3				;w2*r2

		pkhtb  r3, r3, r2, asr #16		;r3,2 tb r1,0,  asr #16		   =>r3,1
       
        sub    r7, r4, r1				;a0 - w2*r2					   => a3
        sub    r6, r4, r5				;a0 - w6*r2					   => a2
        add    r5, r4, r5				;a0 + w6*r2					   => a1
        add    r4, r4, r1				;a0 + w2*r2					   => a0

 		pkhtb  r2, r12, r14, asr #16	;w3,1 tb w7,5,  asr #16		   =>w3,7
        smuad  r8, r3, r12              ;r3,1 *+  w3,1		w1*r1 + w3*r3   =>b0
        smusdx r9, r3, r2				;w3,7 x*- r3,1		(w3*r1 - w7*r3) =>b1 
  		pkhbt  r2, r12, r14, lsl #16	;w7,5 tb w3,1   lsl #16	 =>w5,1 
        smusdx r11, r3, r14				;r3,1 x*- w7,5		w7*r1 - w5*r3   =>b3
        smusdx r10, r3, r2				;r3,1 x*- w5,1		(w5*r1 - w1*r3 )=>b2 
	MEND


;;;;;;;;;;;;;;;;;;;;;;;;;;;;		
	MACRO
		idct_row_4_v6 $version, $dst, $offset
;;;;;;;;;;;;;;;;;;;;;;;;;;;;		
	
        ldrd   r2, [r0, #($offset*2)]					;r1,0,3,2
		orrs   r4, r3, r2, lsr #16
        beq    $version.dc_only_4_v6

		ROW_STEP_1 $version

		add   r1, r4, r8				;a0 + b0
		mov   r1, r1, lsl #5			;
		mov   r2, r1, lsr #16			;r0: ((a0 + b0)<<5)>>16
		add   r1, r5, r9				;a1 + b1
		mov   r3, r1, lsr #ROW_SHIFT	;r1: (a1 + b1)>>11
		orr   r2, r2, r3, lsl #16		;r1,0	
		add   r1, r6, r10				;a2 + b2
		mov   r1, r1, lsl #5
		mov   r3, r1, lsr #16			;r2: ((a2 + b2)<<5)>>16
		add   r1, r7, r11				;a3 + b3 
		mov   r14, r1, lsr #ROW_SHIFT	;r3: (a3 + b3)>>11
		orr   r3, r3, r14, lsl #16		;r3,2
        strd   r2, [$dst, #($offset*2)]					;r1,0,3,2

		sub   r1, r7, r11				;a3 - b3
		mov   r1, r1, lsl #5
		mov   r2, r1, lsr #16			;r4: ((a3 - b3)<<5)>>16
		sub   r1, r6, r10				;a2 - b2
		mov   r3, r1, lsr #ROW_SHIFT	;r5: (a2 - b2)>>11
		orr   r2, r2, r3, lsl #16		;r5,4
		sub   r1, r5, r9				;a1 - b1
		mov   r1, r1, lsl #5			;
		mov   r3, r1, lsr #16			;r6: ((a1 - b1)<<5)>>16
		sub   r1, r4, r8				;a0 - b0
		mov   r14, r1, lsr #ROW_SHIFT	;r7: ((a0 - b0)<<5)>>16
		orr   r3, r3, r14, lsl #16		;r7,6
        strd   r2, [$dst, #($offset*2+8)]			;r5,4,7,6

		b $version.finish_4_v6

$version.dc_only_4_v6
        mov    r2, r2, lsl #19
        orr    r2, r2, r2, lsr #16
        mov    r3, r2
        strd   r2, [$dst, #($offset*2  )]
        strd   r2, [$dst, #($offset*2+8)]

		b $version.finish_4_v6

long_w31_$version
     	DCD		W31
long_w62_$version
     	DCD		W62
long_w75_$version
     	DCD		W75
long_w24_$version
     	DCD		W24
long_w24n_$version
     	DCD		W24n
long_w64_$version
     	DCD		W64

$version.finish_4_v6
	
	MEND

;;;;;;;;;;;;;;;;;;;;;;;;;;;;		
	MACRO
		A_STEP_2 $rc64, $a0, $a1, $a2, $a3, $w64, $w24n, $cond
;;;;;;;;;;;;;;;;;;;;;;;;;;;;		
		
		smlad$cond  $a0, $rc64, $w64,  $a0			;a0 +=  w6 *r6 + w4 *r4
		smlad$cond  $a1, $rc64, $w24n, $a1			;a1 +=  w2n*r6 + w4n*r4
		smlsd$cond  $a2, $rc64, $w24n, $a2			;a3 += -w2n*r6 + w4n*r4
		smlsd$cond  $a3, $rc64, $w64,  $a3			;a4 += -w6 *r6 + w4 *r4

	MEND


;;;;;;;;;;;;;;;;;;;;;;;;;;;;		
	MACRO
		B_STEP_2 $rc75, $b0, $b1, $b2, $b3, $w75_73, $w31, $w51n, $cond
;;;;;;;;;;;;;;;;;;;;;;;;;;;;		
		smlad$cond  $b0,     $rc75, $w75_73, $b0	;b0 +=  w7*r7 + w5 *r5
		pkhtb$cond  $w75_73, $w31,  $w75_73,asr #16	;w3,1, w7,5 					=>w7,3
		smlsdx$cond $b3,     $rc75, $w31,    $b3	;b3 += -w1*r7 + w3 *r5 
		smlad$cond  $b1,     $rc75, $w51n,   $b1	;b1 += w5n*r7 + w1n*r5
		smlad$cond  $b2,     $rc75, $w75_73, $b2	;b2 += w3 *r7 + w7 *r5 
	MEND


;;;;;;;;;;;;;;;;;;;;;;;;;;;;		
	MACRO
		idct_row_8_v6 $version, $dst, $offset
;;;;;;;;;;;;;;;;;;;;;;;;;;;;	
        
        ldr    r12, long_w31_$version	;W31

        ldrd   r4, [r0, #($offset*2 + 8)]	;r1,0,3,2
        ldrd   r2, [r0, #($offset*2    )]	;r1,0,3,2
		orrs   r4, r4, r5
		orreqs v4, r3, r2, lsr #16
        beq    $version.dc_only

		;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
		;		step 1
		;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

		ROW_STEP_1 $version

        ldrd   r2, [r0, #($offset*2 + 8)]	;r5,4,7,6
		orrs   r12, r2, r3
		beq	   $version.conclude
		
		;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
		;		step 2
		;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
        ldr    r12, long_w64_$version	;w64
        ldr    r14, long_w24n_$version	;w24n
		pkhbt  r1, r2, r3, lsl #16		;r5,4, r7,6  lsl #16		   =>r6,4
		
		A_STEP_2 r1, r4, r5, r6, r7, r12, r14

        ldr    r12, long_w75_$version	;w7,5
        ldr    r14, long_w31_$version	;w3,1
		pkhtb  r1, r3,  r2,  asr #16	;r7,6, r5,4  lsl #16			=>r7,5
        ldr    r3, long_w51n_$version	;w51n
	
		B_STEP_2 r1, r8, r9, r10, r11, r12, r14, r3

$version.conclude	
		add   r1, r4, r8				;a0 + b0
		mov   r1, r1, lsl #5			;
		mov   r2, r1, lsr #16			;r0: ((a0 + b0)<<5)>>16
		add   r1, r5, r9				;a1 + b1
		mov   r3, r1, lsr #ROW_SHIFT	;r1: (a1 + b1)>>11
		orr   r2, r2, r3, lsl #16		;r1,0	
		add   r1, r6, r10				;a2 + b2
		mov   r1, r1, lsl #5
		mov   r3, r1, lsr #16			;r2: ((a2 + b2)<<5)>>16
		add   r1, r7, r11				;a3 + b3 
		mov   r14, r1, lsr #ROW_SHIFT	;r3: (a3 + b3)>>11
		orr   r3, r3, r14, lsl #16		;r3,2
        strd   r2, [$dst, #($offset*2+0)]	;r1,0,3,2

		sub   r1, r7, r11				;a3 - b3
		mov   r1, r1, lsl #5
		mov   r2, r1, lsr #16			;r4: ((a3 - b3)<<5)>>16
		sub   r1, r6, r10				;a2 - b2
		mov   r3, r1, lsr #ROW_SHIFT	;r5: (a2 - b2)>>11
		orr   r2, r2, r3, lsl #16		;r5,4
		sub   r1, r5, r9				;a1 + b1
		mov   r1, r1, lsl #5			;
		mov   r3, r1, lsr #16			;r6: ((a1 - b1)<<5)>>16
		sub   r1, r4, r8				;a0 - b0
		mov   r14, r1, lsr #ROW_SHIFT	;r7: ((a0 - b0)<<5)>>16
		orr   r3, r3, r14, lsl #16		;r7,6
        strd   r2, [$dst, #($offset*2+8)]	;r5,4,7,6

		b	   $version.finish

$version.dc_only
        mov    r2, r2, lsl #19
        orr    r2, r2, r2, lsr #16
        mov    r3, r2
        strd   r2, [$dst, #($offset*2+0)]
        strd   r2, [$dst, #($offset*2+8)]

		b $version.finish

long_w31_$version
     	DCD		W31
long_w62_$version
     	DCD		W62
long_w75_$version
     	DCD		W75
long_w24_$version
     	DCD		W24
long_w24n_$version
        DCD		W24n
long_w64_$version
        DCD		W64
long_w51n_$version        
        DCD		W51n

$version.finish

	MEND
	
	
;;;;;;;;;;;;;;;;;;;;;;;;;;;;		
	MACRO
		store_sub_2_1		$addr1, $addr2
;;;;;;;;;;;;;;;;;;;;;;;;;;;;		

        subs   a2, v1, v6
        sub    v5, v2, v4
        mov    a2, a2, asr #4
        mov	   a2, a2, lsr #16       
        mov    v5, v5, asr #20
        add    a2, a2, v5, lsl #16
        str    a2, [a1, $addr1]
        adds   v6, v1, v6
        add    v4, v2, v4
        mov    v4, v4, asr #20
        mov    a2, v6, asr #4
        mov    a2, a2, lsr #16        
        add    a2, a2, v4, lsl #16
        str    a2, [a1, $addr2]
	MEND
		
;;;;;;;;;;;;;;;;;;;;;;;;;;;;		
	MACRO
		store_sub_2_2		$addr1, $addr2
;;;;;;;;;;;;;;;;;;;;;;;;;;;;		

        adds   a2, v1, v6
        add    v5, v2, v4
        mov    a2, a2, asr #4
        mov	   a2, a2, lsr #16        
        mov    v5, v5, asr #20
        add    a2, a2, v5, lsl #16
        str    a2, [a1, $addr1]
        subs   v6, v1, v6
        sub    v4, v2, v4
        mov    v4, v4, asr #20
        mov    a2, v6, asr #4
        mov	   a2, a2, lsr #16        
        add    a2, a2, v4, lsl #16
        str    a2, [a1, $addr2]
	MEND		

;;;;;;;;;;;;;;;;;;;;;;;;;;;;		
	MACRO
		store_sub_0_1		$addr1, $in1, $in2
;;;;;;;;;;;;;;;;;;;;;;;;;;;;		

        adds   a2, a3, $in1
        add    ip, a4, $in2
        mov    ip, ip, asr #20
        mov    a2, a2, asr #4
        mov    a2, a2, lsr #16
        add    a2, a2, ip, lsl #16
        str    a2, [a1, $addr1]
	    subs   a3, a3, $in1
        sub    a4, a4, $in2
        mov    a4, a4, asr #20
        mov    a2, a3, asr #4
        mov    a2, a2, lsr #16
        add    a2, a2, a4, lsl #16
	MEND	


;;;;;;;;;;;;;;;;;;;;;;;;;;;;		
	MACRO
		store_sub_0_2		$addr1, $in1, $in2
;;;;;;;;;;;;;;;;;;;;;;;;;;;;		
       
        subs   a2, a3, $in1
        sub    ip, a4, $in2
        mov    ip, ip, asr #20
        mov    a2, a2, asr #4
        mov	   a2, a2, lsr #16       
        add    a2, a2, ip, lsl #16
        adds   a3, a3, $in1
        add    a4, a4, $in2
        mov    a4, a4, asr #20
        str    a2, [a1, $addr1]
        mov    a2, a3, asr #4
        mov    a2, a2, lsr #16        
        add    a2, a2, a4, lsl #16
	MEND	

;;;;;;;;;;;;;;;;;;;;;;;;;;;;		
	MACRO
		idct_col_2_v6
;;;;;;;;;;;;;;;;;;;;;;;;;;;;		
		
        ldr    a2, [a1]              ;a4 = col[1:0] 
        ldr    a4, [a1, #(16*1)]

        mov    v1, #((1<<(COL_SHIFT-1))/W4) 
        add    v2, v1, a2, asr #16
        rsb    v2, v2, v2, lsl #14
        mov    a2, a2, lsl #16
        add    v1, v1, a2, asr #16
        rsb    v1, v1, v1, lsl #14
		
        smultb v6, ip, a4
        smultt v4, ip, a4
        rsb    v6, v6, #0
        rsb    v4, v4, #0
		store_sub_2_1 #16, #96

        smulbb v6, v3, a4
        smulbt v4, v3, a4
 		store_sub_2_2 #32, #80
 		
        smultb v6, v3, a4
        smultt v4, v3, a4
 		store_sub_2_2 #48, #64

        smulbb v6, ip, a4
        smulbt v4, ip, a4
 		store_sub_2_2 #0, #112
		
 	MEND	

;;;;;;;;;;;;;;;;;;;;;;;;;;;;		
	MACRO
		idct_col_4_v6
;;;;;;;;;;;;;;;;;;;;;;;;;;;;		

        ldrh   r2, [r0, #2]!			;c0
        ldrh   r8, [r0, #1*16]			;c1
        ldrh   r3, [r0, #2*16]			;c2
        ldrh   r7, [r0, #3*16]			;c3

        ldr    r5, long_w24_4x4_v6		;w24
        ldr    r6, long_w62_4x4_v6		;w62
        mov    r4, #((1<<(COL_SHIFT-1))/W4)	;round					   => round
		add    r4, r4, r2

        smulbb r4, r5, r4				;w4*c0 + round				   => a0
        smultb r5, r6, r3				;w6*c2					     
        smulbb r1, r6, r3				;w2*c2

		orr	   r3, r8, r7, lsl #16
       
        sub    r7, r4, r1				;a0 - w2*r2					   => a3
        sub    r6, r4, r5				;a0 - w6*r2					   => a2
        add    r5, r4, r5				;a0 + w6*r2					   => a1
        add    r4, r4, r1				;a0 + w2*r2					   => a0

 		pkhtb  r2, r12, r14, asr #16	;w3,1 tb w7,5,  asr #16		   =>w3,7
        smuad  r8, r3, r12             ;r3,1 *+  w3,1		w1*r1 + w3*r3   =>b0
        smusdx r9, r3, r2				;w3,7 x*- r3,1		(w7*r3 - w3*r1) =>b1 
  		pkhbt  r2, r12, r14, lsl #16	;w7,5 tb w3,1   lsl #16	 =>w5,1 
        smusdx r11, r3, r14				;r3,1 x*- w7,5		w7*r1 - w5*r3   =>b3
        smusdx r10, r3, r2				;r3,1 x*- w5,1		(w5*r1 - w1*r3 )=>b2 

		add   r1, r4, r8				;a0 + b0
		mov   r2, r1, asr #20			;
		strh  r2, [r0, #0*16]
		
		add   r1, r5, r9				;a1 + b1
		mov   r3, r1, asr #20			;
		strh  r3, [r0, #1*16]
		
		add   r1, r6, r10				;a2 + b2
		mov   r2, r1, asr #20
		strh  r2, [r0, #2*16]
		
		add   r1, r7, r11				;a3 + b3 
		mov   r3, r1, asr #20			;
		strh  r3, [r0, #3*16]

		sub   r1, r7, r11				;a3 - b3
		mov   r2, r1, asr #20
		strh  r2, [r0, #4*16]
		
		sub   r1, r6, r10				;a2 - b2
		mov   r3, r1, asr #20			;
		strh  r3, [r0, #5*16]
		
		sub   r1, r5, r9				;a1 - b1
		mov   r2, r1, asr #20			;
		strh  r2, [r0, #6*16]
		
		sub   r1, r4, r8				;a0 - b0
		mov   r3, r1, asr #20			;
		strh  r3, [r0, #7*16]
	MEND

;;;;;;;;;;;;;;;;;;;;;;;;;;;;		
	MACRO
		idct_col_8_v6
;;;;;;;;;;;;;;;;;;;;;;;;;;;;	

        ldr    a4, [a1]					;a4 = col[1:0] 
        ldr    a3, [a1, #(16*4)]
        mov    ip, #16384
        sub    ip, ip, #1				;ip = W4 

;       mov    v1, #(1<<(COL_SHIFT-1))
;		smlabt v2, ip, a4, v1			;v2 = W4 * col[1] + (1<<(COL_SHIFT-1)) 
;		smlabb v1, ip, a4, v1			;v1 = W4 * col[0] + (1<<(COL_SHIFT-1)) 
  
        mov    v1, #((1<<(COL_SHIFT-1))/W4) 
        add    v2, v1, a4, asr #16
        rsb    v2, v2, v2, lsl #14
        mov    a4, a4, lsl #16
        add    v1, v1, a4, asr #16
        rsb    v1, v1, v1, lsl #14

        smulbb lr, ip, a3
        smulbt a3, ip, a3
        ldr    ip, long_w62_col_8_v6				;ip = W2 | (W6 << 16)
        ldr    a4, [a1, #(16*2)]
        sub    v3, v1, lr
        mov    v5, v3
        add    v7, v1, lr
        mov    v1, v7
        sub    v4, v2, a3
        mov    v6, v4
        add    fp, v2, a3
		mov    v2, fp
		
        smulbb lr, ip, a4
        smultb a3, ip, a4
        sub    v7, v7, lr
        add    v1, v1, lr
        add    v3, v3, a3
        sub    v5, v5, a3
        smultt a3, ip, a4
        smulbt lr, ip, a4
        ldr    a4, [a1, #(16*6)]
        add    v4, v4, a3
        sub    v6, v6, a3
        add    v2, v2, lr
        sub    fp, fp, lr

        smultt lr, ip, a4
        smulbt a3, ip, a4
        sub    fp, fp, lr
        add    v2, v2, lr
        add    v6, v6, a3
        sub    v4, v4, a3
        smultb lr, ip, a4
        smulbb a3, ip, a4
        ldr    a4, [a1, #(16*1)]
        sub    v7, v7, lr
        add    v1, v1, lr
        ldr    ip, long_w31_col_8_v6		;W31
        ldr    lr, long_w75_col_8_v6		;W75
        add    v5, v5, a3
        sub    v3, v3, a3

        stmfd  sp!, {r4-r11}

        smulbb fp, ip, a4
        smulbt v7, ip, a4
        smultb v6, ip, a4
        smultt v5, ip, a4
        smulbb v4, lr, a4
        smulbt v3, lr, a4
        smultb v2, lr, a4
        smultt v1, lr, a4
        ldr    a4, [a1, #(16*3)]
        rsb    v5, v5, #0
        rsb    v6, v6, #0

        smulbb a3, ip, a4
        smulbb a2, lr, a4
        smlatb fp, ip, a4, fp
        smlatt v7, ip, a4, v7
        smlatb v6, lr, a4, v6
        smlatt v5, lr, a4, v5
        sub    v4, v4, a3
        sub    v2, v2, a2
        smulbt a3, ip, a4
        smulbt a2, lr, a4
        ldr    a4, [a1, #(16*5)]
        sub    v3, v3, a3
        ldr    a3, [a1, #(16*7)]
        sub    v1, v1, a2

        smlabb fp, lr, a4, fp
        smlabt v7, lr, a4, v7
        smlabb v6, ip, a4, v6
        smlabt v5, ip, a4, v5
        smlatb v4, lr, a4, v4
        smlatt v3, lr, a4, v3
        smlatb v2, ip, a4, v2
        smlatt v1, ip, a4, v1

        smulbb a4, ip, a3
        smlatb fp, lr, a3, fp
        smlatt v7, lr, a3, v7
        smlabb v6, lr, a3, v6
        smlabt v5, lr, a3, v5
        smlatb v4, ip, a3, v4
        smlatt v3, ip, a3, v3
        sub    v2, v2, a4
        smulbt a4, ip, a3
        sub    v1, v1, a4

        ldmfd  sp!, {a3, a4}
        store_sub_0_1 #0, fp, v7        
        ldmfd  sp!, {a3, a4}
        str    a2, [a1, #(16*7)]

		store_sub_0_2 #16, v6, v5
        ldmfd  sp!, {a3, a4}
        str    a2, [a1, #(16*6)]

        store_sub_0_1 #32, v4, v3        
        ldmfd  sp!, {a3, a4}
        str    a2, [a1, #(16*5)]

        store_sub_0_1 #48, v2, v1        
        str    a2, [a1, #(16*4)]

 	MEND	  	


|ippiDCT8x8Inv_16s_C1I_arm_v6| PROC
 
        stmfd  sp!, {r1-r11, lr}

        idct_row_8_v6 row_8_v6_16s_step0, r0, 0*8
        idct_row_8_v6 row_8_v6_16s_step1, r0, 1*8
        idct_row_8_v6 row_8_v6_16s_step2, r0, 2*8
        idct_row_8_v6 row_8_v6_16s_step3, r0, 3*8
        idct_row_8_v6 row_8_v6_16s_step4, r0, 4*8
        idct_row_8_v6 row_8_v6_16s_step5, r0, 5*8
        idct_row_8_v6 row_8_v6_16s_step6, r0, 6*8
        idct_row_8_v6 row_8_v6_16s_step7, r0, 7*8
  
        idct_col_8_v6
        add    a1, a1, #4
        idct_col_8_v6
        add    a1, a1, #4
        idct_col_8_v6
        add    a1, a1, #4
        idct_col_8_v6

        ldmfd  sp!, {r1-r11, pc}
        
long_w31_col_8_v6
     	DCD		W31
long_w62_col_8_v6
     	DCD		W62
long_w75_col_8_v6
     	DCD		W75
long_w24_col_8_v6
     	DCD		W24
long_w24n_col_8_v6
     	DCD		W24n
long_w64_col_8_v6
     	DCD		W64   

	ENDP  ; |ippiDCT8x8Inv_16s_C1I_arm_v5|


|ippiDCT8x8Inv_4x4_16s_C1I_arm_v6| PROC
        stmfd  sp!, {r4-r11, lr}

        ldr    r12, long_w31_4x4_v6			;W31
        
        idct_row_4_v6 row_4_v6_16s_step0, r0, 0*8
        idct_row_4_v6 row_4_v6_16s_step1, r0, 1*8
        idct_row_4_v6 row_4_v6_16s_step2, r0, 2*8
        idct_row_4_v6 row_4_v6_16s_step3, r0, 3*8
        sub    a1, a1, #2
        
        ldr    r14, long_w75_4x4_v6		;w75
        idct_col_4_v6
        idct_col_4_v6
        idct_col_4_v6
        idct_col_4_v6
        idct_col_4_v6
        idct_col_4_v6
        idct_col_4_v6
        idct_col_4_v6

        ldmfd  sp!, {r4-r11, pc}

long_w31_4x4_v6
     	DCD		W31
long_w62_4x4_v6
     	DCD		W62
long_w75_4x4_v6
     	DCD		W75
long_w24_4x4_v6
     	DCD		W24
	ENDP  ; |ippiDCT8x8Inv_4x4_16s_C1I_arm|

|ippiDCT8x8Inv_2x2_16s_C1I_arm_v6| PROC
 
        stmfd  sp!, {r4-r11, lr}

        ldr    ip, long_w31_2_v6	
        ldr    v3, long_w75_2_v6	
        idct_row_2_v6 row_2_v6_16s_step0, r0, 0*8
        idct_row_2_v6 row_2_v6_16s_step1, r0, 1*8

        idct_col_2_v6
        add    a1, a1, #4
        idct_col_2_v6
        add    a1, a1, #4
        idct_col_2_v6
        add    a1, a1, #4
        idct_col_2_v6

        ldmfd  sp!, {r4-r11, pc}

long_w31_2_v6
     	DCD		W31
long_w75_2_v6
     	DCD		W75

	ENDP  ; |ippiDCT8x8Inv_2x2_16s_C1I_arm_v6|


;;;;;;;;;;;;;;;;;;;;;;;;;;;;		
	MACRO
		PACK_0123 $smulxx_1, $smulxx_2
;;;;;;;;;;;;;;;;;;;;;;;;;;;;		
 
        $smulxx_1 r1,  ww,  r4				;w1 * c10			==>b00
		add	   r2, r6, r1					;a00 + b00
		usat   r2, #8, r2, asr #20			;0
		
		sub	   r3, r6, r1					;a00 - b00
		usat   r3, #8, r3, asr #20			;0
		
	    $smulxx_2 r1,  ww, r4				;w1 * c11			==>b01
		add	   r10, r7, r1					;a01 + b01
		usat   r10, #8, r10, asr #20		;1
		orr	   r2, r2, r10, lsl #8			;xx,xx,01,00
		
		sub	   r10, r7, r1					;a01 - b01
		usat   r10, #8, r10,  asr #20		;1
		orr	   r3, r3, r10, lsl #8			;xx,xx,01,00
	
        $smulxx_1 r1, ww, r5				;w1 * c10			==>b02
		add	   r10, r8, r1					;a01 + b01
		usat   r10, #8, r10,  asr #20		;1
		orr	   r2, r2, r10, lsl #16			;xx,xx,01,00
		
		sub	   r10, r8, r1					;a01 - b01
		usat   r10, #8, r10,  asr #20		;1
		orr	   r3, r3, r10, lsl #16			;xx,xx,01,00
		
	    $smulxx_2 r1, ww, r5				;w1 * c11			==>b03
		add	   r10, r9, r1					;a01 + b01
		usat   r10, #8, r10,  asr #20		;1
		orr	   r2, r2, r10, lsl #24			;xx,xx,01,00
		
		sub	   r10, r9, r1					;a01 - b01
		usat   r10, #8, r10,  asr #20		;1
		orr	   r3, r3, r10, lsl #24			;xx,xx,01,00
 
 	MEND	

;;;;;;;;;;;;;;;;;;;;;;;;;;;;		
	MACRO
		idct_col_2_16s8u_v6 $src, $offset
;;;;;;;;;;;;;;;;;;;;;;;;;;;;		
		
        ldrd    r2, [$src, #2*($offset+0)]	;c01,00,03,02
        ldrd    r4, [$src, #2*($offset+8)]	;c11,10,13,12

        mov    r1, #((1<<(COL_SHIFT-1))/W4)	;round
        mov    r6, r2, lsl #16				;c00
        add    r6, r1, r6, asr #16			;c00 + round
        rsb    r6, r6, r6, lsl #14			;w4*(c00 + round)	=>a00
        add    r7, r1, r2, asr #16			;c01 + round		
        rsb    r7, r7, r7, lsl #14			;w4*(c01 + round)	=>a01
        
        mov    r8, r3, lsl #16				;c02
        add    r8, r1, r8, asr #16			;c02 + round		
        rsb    r8, r8, r8, lsl #14			;w4*(c02 + round)	==>a02
        add    r9, r1, r3, asr #16			;c03 + round
        rsb    r9, r9, r9, lsl #14			;w4*(c03 + round)	==>a03
		
        ldr    ww, long_w31_2_16s8u_v6	
 		sub    dst, dst, drb			;level -1
		
		PACK_0123 smulbb, smulbt 
        str	   r2, [dst, drb]			;level 0, -1
		add    dst, dst, drb, lsl #3	;level 7
        str	   r3, [dst], drb			;level 7, 8
 		sub    dst, dst, drb, lsl #3	;level 0
	
		PACK_0123 smultb, smultt 
        str	   r2, [dst, drb]!			;level 1, 1
		add    dst, dst, drb, lsl #2	;level 5
        str	   r3, [dst, drb]!			;level 6, 6
 		sub    dst, dst, drb, lsl #2	;level 2
     
        ldr    ww, long_w75_2_16s8u_v6	
		PACK_0123 smulbb, smulbt 
        str	   r2, [dst], drb			;level 2, 3
		add    dst, dst, drb, lsl #1	;level 5
        str	   r3, [dst]				;level 5
 		sub    dst, dst, drb, lsl #1	;level 3 		
 		
 		PACK_0123 smultb, smultt 
        str	   r2, [dst], drb			;level 3, 4
        str	   r3, [dst]				;level 4
 		sub    dst, dst, drb, lsl #2	;level 0 		
 	MEND	


;;;;;;;;;;;;;;;;;;;;;;;;;;;;		
	MACRO
		idct_col_4_16s8u_v6 $src, $offset
;;;;;;;;;;;;;;;;;;;;;;;;;;;;		

        ldrh   r2, [$src, #(0*16+$offset)]	;c0
        ldr    r5, long_w24_4x4_16s8u_v6	;w24

        mov    r4, #((1<<(COL_SHIFT-1))/W4)	;round					=> round
		add    r4, r4, r2					;c0 + round
        ldrh   r1, [$src, #(2*16+$offset)]	;c2
        ldrh   r2, [$src, #(1*16+$offset)]	;c1
        smulbb r4, r5, r4					;w4*(c0 + round)		=> a0
        ldrh   r7, [$src, #(3*16+$offset)]	;c3

        smultb r5, r0, r1					;w6*c2					     
        smulbb r1, r0, r1					;w2*c2

		orr	   r3, r2, r7, lsl #16		;c3,1

        sub    r6, r4, r5				;a0 - w6*c2	
        add    r5, r4, r5				;a0 + w6*c2	
        sub    r7, r4, r1				;a0 - w2*c2	
        add    r4, r4, r1				;a0 + w2*c2	

 		pkhtb  r2, r12, r14, asr #16	;w3,1 tb w7,5,  asr #16		   =>w3,7
        smuad  r8, r3, r12              ;c3,1 *+  w3,1		w1*c1 + w3*c3   =>b0
        smusdx r9, r3, r2				;w3,7 x*- r3,1		(w7*c3 - w3*c1) =>b1 
  		pkhbt  r2, r12, r14, lsl #16	;w7,5 tb w3,1   lsl #16	 =>w5,1 
        smusdx r1, r3, r14				;c3,1 x*- w7,5		w7*c1 - w5*c3   =>b3
        smusdx r2, r3, r2				;c3,1 x*- w5,1		(w5*c1 - w1*c3 )=>b2 

		add   r3, r4, r8				;a0 + b0
		usat  r3, #8, r3, asr #20
		strb  r3, [r10], r11
		
		add   r3, r5, r9				;a1 + b1
		usat  r3, #8, r3, asr #20
		strb  r3, [r10], r11
		
		add   r3, r6, r2				;a2 + b2
		usat  r3, #8, r3, asr #20
		strb  r3, [r10], r11
		
		add   r3, r7, r1				;a3 + b3 
		usat  r3, #8, r3, asr #20
		strb  r3, [r10], r11

		sub   r3, r7, r1				;a3 - b3
		usat  r3, #8, r3, asr #20
		strb  r3, [r10], r11
		
		sub   r3, r6, r2				;a2 - b2
		usat  r3, #8, r3, asr #20
		strb  r3, [r10], r11
		
		sub   r3, r5, r9				;a1 - b1
		usat  r3, #8, r3, asr #20
		strb  r3, [r10], r11
		
		sub   r3, r4, r8				;a0 - b0
		usat  r3, #8, r3, asr #20
		strb  r3, [r10], r11
		
		sub   r10, r10, r11, lsl #3
		add   r10, r10, #1
	MEND  	

;;;;;;;;;;;;;;;;;;;;;;;;;;;;		
	MACRO
		idct_col_8_16s8u_v6 $src, $offset
;;;;;;;;;;;;;;;;;;;;;;;;;;;;		

        ldrh   r2, [$src, #(0*16+$offset)]	;c0
        ldr    r5, long_w24_col_8_16s8u_v6	;w24
        ldr    r6, long_w62_col_8_16s8u_v6	;w62
        ldr    r12,long_w31_col_8_16s8u_v6	;W31
        ldr    r14,long_w75_col_8_16s8u_v6	;w75

        mov    r4, #((1<<(COL_SHIFT-1))/W4)	;round					=> round
		add    r4, r4, r2					;c0 + round
        ldrh   r1, [$src, #(2*16+$offset)]				;c2
        ldrh   r2, [$src, #(1*16+$offset)]				;c1
        smulbb r4, r5, r4					;w4*(c0 + round)		=> a0
        ldrh   r7, [$src, #(3*16+$offset)]				;c3

        smultb r5, r6, r1					;w6*c2					     
        smulbb r1, r6, r1					;w2*c2

		orr	   r3, r2, r7, lsl #16		;c3,1

        sub    r6, r4, r5				;a0 - w6*c2	
        add    r5, r4, r5				;a0 + w6*c2	
        sub    r7, r4, r1				;a0 - w2*c2	
        add    r4, r4, r1				;a0 + w2*c2	

 		pkhtb  r2, r12, r14, asr #16	;w3,1 tb w7,5,  asr #16		   =>w3,7
        smuad  r8, r3, r12              ;c3,1 *+  w3,1		w1*c1 + w3*c3   =>b0
        smusdx r9, r3, r2				;w3,7 x*- r3,1		(w7*c3 - w3*c1) =>b1 
  		pkhbt  r2, r12, r14, lsl #16	;w7,5 tb w3,1   lsl #16	 =>w5,1 
        smusdx r1, r3, r14				;c3,1 x*- w7,5		w7*c1 - w5*c3   =>b3
        smusdx r2, r3, r2				;c3,1 x*- w5,1		(w5*c1 - w1*c3 )=>b2 

		ldrh  r12, [$src, #(4*16+$offset)]		;c4
		ldrh  r14, [$src, #(6*16+$offset)]		;c6
		orrs  r11, r12, r14, lsl #16			;c6,4

        ldrne r12, long_w64_col_8_16s8u_v6		;w64
        ldrne r14, long_w24n_col_8_16s8u_v6		;w24n
		A_STEP_2 r11, r4, r5, r6, r7, r12, r14, ne
		
		ldrh  r12, [$src, #(5*16+$offset)]		;c5
		ldrh  r14, [$src, #(7*16+$offset)]		;c7
		orrs  r11, r12, r14, lsl #16			;c7,5
        
        ldrne r12, long_w75_col_8_16s8u_v6		;w7,5
        ldrne r14, long_w31_col_8_16s8u_v6		;w3,1
        ldrne r3,  long_w51n_col_8_16s8u_v6		;w51n
		B_STEP_2 r11, r8, r9, r2, r1, r12, r14, r3, ne

		add   r3, r4, r8				;a0 + b0
		usat  r3, #8, r3, asr #20
		strb  r3, [r10], r0
		
		add   r3, r5, r9				;a1 + b1
		usat  r3, #8, r3, asr #20
		strb  r3, [r10], r0
		
		add   r3, r6, r2				;a2 + b2
		usat  r3, #8, r3, asr #20
		strb  r3, [r10], r0
		
		add   r3, r7, r1				;a3 + b3 
		usat  r3, #8, r3, asr #20
		strb  r3, [r10], r0

		sub   r3, r7, r1				;a3 - b3
		usat  r3, #8, r3, asr #20
		strb  r3, [r10], r0
		
		sub   r3, r6, r2				;a2 - b2
		usat  r3, #8, r3, asr #20
		strb  r3, [r10], r0
		
		sub   r3, r5, r9				;a1 - b1
		usat  r3, #8, r3, asr #20
		strb  r3, [r10], r0
		
		sub   r3, r4, r8				;a0 - b0
		usat  r3, #8, r3, asr #20
		strb  r3, [r10], r0
		
		sub   r10, r10, r0, lsl #3
		add   r10, r10, #1
	MEND  	     	

|ippiDCT8x8Inv_16s8u_C1R_arm_v6| PROC
 
        stmfd  sp!, {r1-r11, lr}

        ands   r3, sp, #7
        subne  sp, sp, #4
        sub	   sp, sp,  #(64*2 + 16)
        str	   r1, [sp, #(64*2 + 0)]
        str	   r2, [sp, #(64*2 + 4)]
        str	   r3, [sp, #(64*2 + 8)]

        idct_row_8_v6 row_8_v6_16s8u_step0, sp, 0*8
        idct_row_8_v6 row_8_v6_16s8u_step1, sp, 1*8
        idct_row_8_v6 row_8_v6_16s8u_step2, sp, 2*8
        idct_row_8_v6 row_8_v6_16s8u_step3, sp, 3*8
        idct_row_8_v6 row_8_v6_16s8u_step4, sp, 4*8
        idct_row_8_v6 row_8_v6_16s8u_step5, sp, 5*8
        idct_row_8_v6 row_8_v6_16s8u_step6, sp, 6*8
        idct_row_8_v6 row_8_v6_16s8u_step7, sp, 7*8
		
        ldr   r10, [sp, #(64*2+0)]				;dst
        ldr   r0,  [sp,  #(64*2+4)]				;drb

        idct_col_8_16s8u_v6 sp, 0*2
        idct_col_8_16s8u_v6 sp, 1*2
        idct_col_8_16s8u_v6 sp, 2*2
        idct_col_8_16s8u_v6 sp, 3*2
        idct_col_8_16s8u_v6 sp, 4*2
        idct_col_8_16s8u_v6 sp, 5*2
        idct_col_8_16s8u_v6 sp, 6*2
        idct_col_8_16s8u_v6 sp, 7*2

		ldr	    r1, [sp, #(64*2+8)]
        cmp		r1, #0
        addeq	sp, sp, #(64*2+16  )
        addne	sp, sp, #(64*2+16+4)

        ldmfd  sp!, {r1-r11, pc}
        
long_w31_col_8_16s8u_v6
     	DCD		W31
long_w62_col_8_16s8u_v6
     	DCD		W62
long_w75_col_8_16s8u_v6
     	DCD		W75
long_w24_col_8_16s8u_v6
     	DCD		W24
long_w24n_col_8_16s8u_v6
     	DCD		W24n
long_w64_col_8_16s8u_v6
     	DCD		W64   
long_w51n_col_8_16s8u_v6        
        DCD		W51n


	ENDP  ; |ippiDCT8x8Inv_16s_C1I_arm_v5|

|ippiDCT8x8Inv_4x4_16s8u_C1R_arm_v6| PROC
        stmfd  sp!, {r1-r11, lr}

        ldr    r12, long_w31_4x4_16s8u_v6		;W31
        ands   r3, sp, #7
        subne  sp, sp, #4
        sub	   sp, sp,  #(64*2 + 16)
        str	   r1, [sp, #(64*2 + 0)]
        str	   r2, [sp, #(64*2 + 4)]
        str	   r3, [sp, #(64*2 + 8)]
        
        idct_row_4_v6 row_16s8u_step0, sp, 0*8
        idct_row_4_v6 row_16s8u_step1, sp, 1*8
        idct_row_4_v6 row_16s8u_step2, sp, 2*8
        idct_row_4_v6 row_16s8u_step3, sp, 3*8
               
        ldr    r14, long_w75_4x4_16s8u_v6		;w75
        ldr    r0, long_w62_4x4_16s8u_v6		;w62
        ldr    r10, [sp, #(64*2+0)]				;dst
        ldr    r11, [sp, #(64*2+4)]				;drb
        idct_col_4_16s8u_v6 sp, 0*2
        idct_col_4_16s8u_v6 sp, 1*2
        idct_col_4_16s8u_v6 sp, 2*2
        idct_col_4_16s8u_v6 sp, 3*2
        idct_col_4_16s8u_v6 sp, 4*2
        idct_col_4_16s8u_v6 sp, 5*2
        idct_col_4_16s8u_v6 sp, 6*2
        idct_col_4_16s8u_v6 sp, 7*2

		ldr	    r1, [sp, #(64*2+8)]
        cmp		r1, #0
        addeq	sp, sp, #(64*2+16  )
        addne	sp, sp, #(64*2+16+4)

        ldmfd  sp!, {r1-r11, pc}

long_w31_4x4_16s8u_v6
     	DCD		W31
long_w62_4x4_16s8u_v6
     	DCD		W62
long_w75_4x4_16s8u_v6
     	DCD		W75
long_w24_4x4_16s8u_v6
     	DCD		W24
	ENDP  ; |ippiDCT8x8Inv_4x4_16s_C1I_arm|


ww					RN 11
dst					RN 12
drb					RN 14

|ippiDCT8x8Inv_2x2_16s8u_C1R_arm_v6| PROC
 
        stmfd  sp!, {r1-r11, lr}

        ands   r3, sp, #7
        subne  sp, sp, #4
        sub	   sp, sp,  #(64*2 + 16)
        str	   r1, [sp, #(64*2 + 0)]
        str	   r2, [sp, #(64*2 + 4)]
        str	   r3, [sp, #(64*2 + 8)]

        ldr    ip, long_w31_2_16s8u_v6	
        ldr    v3, long_w75_2_16s8u_v6	
        idct_row_2_v6 row_2_v6_16s8u_step0, sp, 0*8
        idct_row_2_v6 row_2_v6_16ssu_step1, sp, 1*8
        
        ldr   r12, [sp, #(64*2+0)]				;dst
        ldr   r14, [sp,  #(64*2+4)]				;drb

        idct_col_2_16s8u_v6 sp, 0
        add	   r12, r12, #4
        idct_col_2_16s8u_v6 sp, 4

		ldr	    r1, [sp, #(64*2+8)]
        cmp		r1, #0
        addeq	sp, sp, #(64*2+16  )
        addne	sp, sp, #(64*2+16+4)

        ldmfd  sp!, {r1-r11, pc}

long_w31_2_16s8u_v6
     	DCD		W31
long_w75_2_16s8u_v6
     	DCD		W75

	ENDP  ; |ippiDCT8x8Inv_2x2_16s_C1I_arm_v6|



  END
