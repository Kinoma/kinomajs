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
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	;armv5te

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	AREA  |.text|, CODE, READONLY
	
 	EXPORT  |ippiDCT8x8Inv_16s_C1I_arm_v5|
 	EXPORT  |ippiDCT8x8Inv_4x4_16s_C1I_arm_v5|
 	EXPORT  |ippiDCT8x8Inv_2x2_16s_C1I_arm_v5|
 	EXPORT  |ippiDCT8x8Inv_16s8u_C1R_clip_arm_v5|


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

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


|idct_row_2_v5| PROC
        ldr    a3, [a1]					 ;a3,a4 = row[1:0:3:2]
		movs   v1, a3, lsr #16
        beq    row_dc_only_2

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
        strd   a3, [a1]

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
        strd  a3, [a1, #8]

        mov    pc, lr

row_dc_only_2
        mov    a3, a3, lsl #19
        orr    a3, a3, a3, lsr #16
        mov    a4, a3
        strd   a3, [a1]
        strd   a3, [a1, #8]

        mov    pc, lr

	ENDP  ; |idct_row_2_v5|


|idct_row_4_v5| PROC

        str    lr, [sp, #-4]!

        ldrd   a3, [a1]					 ;a3,a4 = row[1:0:3:2]
		orrs   v1, a4, a3, lsr #16
        beq    row_dc_only_row_4_v5

        mov    v1, #(1<<(ROW_SHIFT-1))
        
		mov	   ip, a3, lsl #16			;ip = row[0]<<16  =>  (row[0] * 16384)<<2
		add	   v1, v1, ip, asr #2		;v1 = v1 + ip>>2  =>  v1 = v1 + row[0] * 16384
		sub    v1, v1, ip, asr #16
		
        ldr    ip, long_w62_row_4_v5				;W62
        smulbb lr, ip, a4				;W2 * row[2]
        smultb a2, ip, a4				;W6 * row[2]

        ldr    ip, long_w31_row_4_v5				;ip = W1 | (W3 << 16)

        sub    v4, v1, lr				;v4 = v1 - W2 * row[2]
        sub    v3, v1, a2				;v3 = v1 - W6 * row[2]
        add    v2, v1, a2				;v2 = v1 + W6 * row[2]
        add    v1, v1, lr				;v1 = v1 - W2 * row[2]

        ldr    lr, long_w75_row_4_v5				;W75

        smultt a2, ip, a3
        smulbt v5, ip, a3
        smlatt v5, ip, a4, v5
        smultt v6, lr, a4
        sub    v6, v6, a2
        smultt fp, lr, a3
        smulbt a2, ip, a4
        smulbt v7, lr, a3
        sub    v7, v7, a2
        smulbt a2, lr, a4
        sub    fp, fp, a2

		add   a2, v1, v5
		mov   a2, a2, lsl #5
		mov   ip, a2, lsr #16
		sub   a2, v2, v6
		mov   lr, a2, lsr #11
		orr   a3, ip, lr, lsl #16
		add   a2, v3, v7
		mov   a2, a2, lsl #5
		mov   ip, a2, lsr #16
		add   a2, v4, fp
		mov   lr, a2, lsr #11
		orr   a4, ip, lr, lsl #16
        strd   a3, [a1]

		sub   a2, v4, fp
		mov   a2, a2, lsl #5
		mov   ip, a2, lsr #16
		sub   a2, v3, v7
		mov   lr, a2, lsr #11
		orr   a3, ip, lr, lsl #16
		add   a2, v2, v6
		mov   a2, a2, lsl #5
		mov   ip, a2, lsr #16
		sub   a2, v1, v5
		mov   lr, a2, lsr #11
		orr   a4, ip, lr, lsl #16
        strd   a3, [a1, #8]

        ldr    pc, [sp], #4

row_dc_only_row_4_v5
        mov    a3, a3, lsl #19
        orr    a3, a3, a3, lsr #16
        mov    a4, a3
        strd   a3, [a1]
        strd   a3, [a1, #8]

        ldr    pc, [sp], #4

long_w31_row_4_v5
     	DCD		W31
long_w62_row_4_v5
     	DCD		W62
long_w75_row_4_v5
     	DCD		W75
long_w24_row_4_v5
     	DCD		W24
long_w24n_row_4_v5
     	DCD		W24n
long_w64_row_4_v5
     	DCD		W64    


	ENDP  ; |idct_row_4_v5|

|idct_row_8_v5| PROC

        str    lr, [sp, #-4]!

        ldrd   v1, [a1, #8]
        ldrd   a3, [a1]					 ;a3,a4 = row[1:0:3:2]
        orrs   v1, v1, v2
		orreqs v1, a4, a3, lsr #16
        beq    row_dc_only_8_v5

        mov    v1, #(1<<(ROW_SHIFT-1))
        
		mov	   ip, a3, lsl #16			;ip = row[0]<<16  =>  (row[0] * 16384)<<2
		add	   v1, v1, ip, asr #2		;v1 = v1 + ip>>2  =>   v1 = v1 + row[0] * 16384
		sub    v1, v1, ip, asr #16
		
        ldr    ip, long_w62_row_8_v5	;W62
        smulbb lr, ip, a4				;W2 * row[2]
        smultb a2, ip, a4				;W6 * row[2]

        ldr    ip, long_w31_row_8_v5	;W31

        sub    v4, v1, lr				;v4 = v1 - W2 * row[2]
        sub    v3, v1, a2				;v3 = v1 - W6 * row[2]
        add    v2, v1, a2				;v2 = v1 + W6 * row[2]
        add    v1, v1, lr				;v1 = v1 - W2 * row[2]

        ldr    lr, long_w75_row_8_v5	;W75

        smultt a2, ip, a3
        smulbt v6, ip, a3
        smlatt v6, ip, a4, v6
        smultt v5, lr, a4
        sub    v5, v5, a2
        smultt v7, lr, a3
        smulbt a2, ip, a4
        smulbt fp, lr, a3
        sub    fp, fp, a2
        smulbt a2, lr, a4
        ldrd   a3, [a1, #8]				;a3,a4=row[5:4:7:6] 
        sub    v7, v7, a2

        orrs   a2, a3, a4
		beq		mark1_row_8_v5

        smlatt v7, ip, a3, v7
        smlabt v5, ip, a3, v5
        smlabt v5, lr, a4, v5
        smlatt v6, lr, a4, v6
        smlabt v6, lr, a3, v6
        smlatt fp, lr, a3, fp
        smulbt a2, ip, a4
        smlatt fp, ip, a4, fp
        sub    v7, v7, a2

        ldr    ip, long_w62_row_8_v5				;W62
		mov	   lr, a3, lsl #16			;ip = row[4]<<16  =>  (row[0] * 16384)<<2
		mov	   a2, lr, asr #2			;a2 = row[4] * 16384
		sub    a2, a2, lr, asr #16

        smultb lr, ip, a4				;lr  = W6 * row[6] 
        add    v4, v4, a2				;v4 += W4 * row[4] 
        sub    v4, v4, lr				;v4 -= W6 * row[6] 
        add    v1, v1, a2				;v1 += W4 * row[4] 
        add    v1, v1, lr				;v1 += W6 * row[6] 
        smulbb lr, ip, a4				;lr  = W2 * row[6] 
        sub    v3, v3, a2				;v3 -= W4 * row[4] 
        add    v3, v3, lr				;v3 += W2 * row[6] 
        sub    v2, v2, a2				;v2 -= W4 * row[4] 
        sub    v2, v2, lr				;v2 -= W2 * row[6] 
mark1_row_8_v5
		add   a2, v1, v6
		mov   a2, a2, lsl #5
		mov   ip, a2, lsr #16
		sub   a2, v2, v5
		mov   lr, a2, lsr #11
		orr   a3, ip, lr, lsl #16
		add   a2, v3, fp
		mov   a2, a2, lsl #5
		mov   ip, a2, lsr #16
		add   a2, v4, v7
		mov   lr, a2, lsr #11
		orr   a4, ip, lr, lsl #16
        strd   a3, [a1]

		sub   a2, v4, v7
		mov   a2, a2, lsl #5
		mov   ip, a2, lsr #16
		sub   a2, v3, fp
		mov   lr, a2, lsr #11
		orr   a3, ip, lr, lsl #16
		add   a2, v2, v5
		mov   a2, a2, lsl #5
		mov   ip, a2, lsr #16
		sub   a2, v1, v6
		mov   lr, a2, lsr #11
		orr   a4, ip, lr, lsl #16
        strd   a3, [a1, #8]

        ldr    pc, [sp], #4

row_dc_only_8_v5
        mov    a3, a3, lsl #19
        orr    a3, a3, a3, lsr #16
        mov    a4, a3
        strd   a3, [a1]
        strd   a3, [a1, #8]

        ldr    pc, [sp], #4

long_w31_row_8_v5
     	DCD		W31
long_w62_row_8_v5
     	DCD		W62
long_w75_row_8_v5
     	DCD		W75
long_w24_row_8_v5
     	DCD		W24
long_w24n_row_8_v5
     	DCD		W24n
long_w64_row_8_v5


	ENDP  ; |idct_row_8_v5|
	
	
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
		idct_col_2
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
		idct_col_4_v5
;;;;;;;;;;;;;;;;;;;;;;;;;;;;	

        ldr    a4, [a1]					;a4 = col[1:0] 
        ldr    a2, [a1, #(16*2)]
        ldr    ip, long_w62_4x4_v5		;W62

        mov    v1, #((1<<(COL_SHIFT-1))/W4) 
        add    v2, v1, a4, asr #16
        mov    a4, a4, lsl #16
        add    v1, v1, a4, asr #16
        ldr    a4, [a1, #(16*1)]
        rsb    v1, v1, v1, lsl #14
        rsb    v2, v2, v2, lsl #14

        smulbb lr, ip, a2
        smultb a3, ip, a2
        sub    v7, v1, lr
        add    v3, v1, a3
        sub    v5, v1, a3
        add    v1, v1, lr
        smultt a3, ip, a2
        smulbt lr, ip, a2
        add    v4, v2, a3
        sub    v6, v2, a3
        sub    fp, v2, lr
        add    v2, v2, lr

        ldr    ip, long_w31_4x4_v5				;W31
        ldr    lr, long_w75_4x4_v5				;W75
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
        sub    v3, v3, a3
        sub    v1, v1, a2

        ldmfd  sp!, {a3, a4}

		store_sub_0_1  #0, fp, v7    
        ldmfd  sp!, {a3, a4}
        str    a2, [a1, #(16*7)]

		store_sub_0_2  #16, v6, v5    
        ldmfd  sp!, {a3, a4}
        str    a2, [a1, #(16*6)]

		store_sub_0_1  #32, v4, v3    
        ldmfd  sp!, {a3, a4}
        str    a2, [a1, #(16*5)]
		
 		store_sub_0_1  #48, v2, v1    
        str    a2, [a1, #(16*4)]
		
 	MEND	

;;;;;;;;;;;;;;;;;;;;;;;;;;;;		
	MACRO
		idct_col_8_v5
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
        ldr    ip, long_w62_col_8_v5				;ip = W2 | (W6 << 16)
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
        ldr    ip, long_w31_col_8_v5		;W31
        ldr    lr, long_w75_col_8_v5		;W75
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

|ippiDCT8x8Inv_16s_C1I_arm_v5| PROC
 
        stmfd  sp!, {r4-r11, lr}

        bl     idct_row_8_v5
        add    a1, a1, #16
        bl     idct_row_8_v5
        add    a1, a1, #16
        bl     idct_row_8_v5
        add    a1, a1, #16
        bl     idct_row_8_v5
        add    a1, a1, #16
        bl     idct_row_8_v5
        add    a1, a1, #16
        bl     idct_row_8_v5
        add    a1, a1, #16
        bl     idct_row_8_v5
        add    a1, a1, #16
        bl     idct_row_8_v5

        sub    a1, a1, #(16*7)
        idct_col_8_v5
        add    a1, a1, #4
        idct_col_8_v5
        add    a1, a1, #4
        idct_col_8_v5
        add    a1, a1, #4
        idct_col_8_v5

        ldmfd  sp!, {r4-r11, pc}

long_w31_col_8_v5
     	DCD		W31
long_w62_col_8_v5
     	DCD		W62
long_w75_col_8_v5
     	DCD		W75
long_w24_col_8_v5
     	DCD		W24
long_w24n_col_8_v5
     	DCD		W24n
long_w64_col_8_v5
     	DCD		W64   

	ENDP  ; |ippiDCT8x8Inv_16s_C1I_arm_v5|

|ippiDCT8x8Inv_4x4_16s_C1I_arm_v5| PROC
        stmfd  sp!, {r4-r11, lr}

        bl     idct_row_4_v5
        add    a1, a1, #16
        bl     idct_row_4_v5
        add    a1, a1, #16
        bl     idct_row_4_v5
        add    a1, a1, #16
        bl     idct_row_4_v5

        sub    a1, a1, #(16*3)
        idct_col_4_v5
        add    a1, a1, #4
        idct_col_4_v5		
        add    a1, a1, #4
        idct_col_4_v5		
        add    a1, a1, #4
        idct_col_4_v5

        ldmfd  sp!, {r4-r11, pc}

long_w31_4x4_v5
     	DCD		W31
long_w62_4x4_v5
     	DCD		W62
long_w75_4x4_v5
     	DCD		W75
long_w24_4x4_v5
     	DCD		W24
long_w24n_4x4_v5
     	DCD		W24n
long_w64_4x4_v5
     	DCD		W64    
	ENDP  ; |ippiDCT8x8Inv_4x4_16s_C1I_arm|

|ippiDCT8x8Inv_2x2_16s_C1I_arm_v5| PROC
 
        stmfd  sp!, {r4-r11, lr}

        ldr    ip, long_w31_2	
        ldr    v3, long_w75_2	
        bl     idct_row_2_v5
        add    a1, a1, #16
        bl     idct_row_2_v5

        sub    a1, a1, #(16*1)

        idct_col_2
        add    a1, a1, #4
        idct_col_2
        add    a1, a1, #4
        idct_col_2
        add    a1, a1, #4
        idct_col_2

        ldmfd  sp!, {r4-r11, pc}

long_w31_2
     	DCD		W31
long_w75_2
     	DCD		W75

	ENDP  ; |ippiDCT8x8Inv_2x2_16s_C1I_arm_v5|


|ippiDCT8x8Inv_16s8u_C1R_clip_arm_v5| PROC

src			RN 0
dst			RN 1
dstStep		RN 2
src0 		RN 3
src1 		RN 4
counter		RN 5
out0        RN 6
out1        RN 7
kff         RN 14

		stmdb  sp!, {r4-r7, lr}

        mov    counter, #8
        mov    kff, #255

loop
		ldrsh  out0, [src]
		ldrsh  src1, [src, #2]
		cmp    out0, kff
		bichi  out0, kff, out0, asr #31
		
		ldrsh  src0, [src, #4]
		cmp    src1, kff
		bichi  src1, kff, src1, asr #31
		orr    out0, out0, src1, lsl #8

		ldrsh  src1, [src, #6]
		cmp    src0, kff
		bichi  src0, kff, src0, asr #31
		orr    out0, out0, src0, lsl #16

		ldrsh  out1, [src, #8]
		cmp    src1, kff
		bichi  src1, kff, src1, asr #31
		orr    out0, out0, src1, lsl #24

		ldrsh  src1, [src, #10]
		cmp    out1, kff
		bichi  out1, kff, out1, asr #31
		
		ldrsh  src0, [src, #12]
		cmp    src1, kff
		bichi  src1, kff, src1, asr #31
		orr    out1, out1, src1, lsl #8

		ldrsh  src1, [src, #14]
        pld    [dst]
		cmp    src0, kff
		bichi  src0, kff, src0, asr #31
		orr    out1, out1, src0, lsl #16

		cmp    src1, kff
		bichi  src1, kff, src1, asr #31
		orr    out1, out1, src1, lsl #24

        subs   counter, counter, #1

        stmia  dst, {out0, out1}
        
		pld    [src, #16]
        add    src, src, #16		; @@ can't we eliminate this?
        add    dst, dst, dstStep

        bne    loop

		ldmia sp!, {r4-r7, pc}

	ENDP  ; |ippiDCT8x8Inv_16s8u_C1R_clip|

  END
