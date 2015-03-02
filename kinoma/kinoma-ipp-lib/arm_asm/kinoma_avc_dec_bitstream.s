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

	EXPORT  |ippiDecodeExpGolombOne_H264_1u16s_unsigned_arm|
	EXPORT  |ippiDecodeExpGolombOne_H264_1u16s_signed_arm|

return_value	RN 0
ppBitStream		RN 0
pBitOffset		RN 1
tmp2			RN 2
four_bytes		RN 3
tmp				RN 4
bit_offset		RN 5
p_stream		RN 6
num_zeros		RN 7
value			RN r12			; doesn't have to be save/restored - inter-call scratch
	MACRO
		ExpGolombOne_H264_1u16s

		ldr         p_stream, [ppBitStream] 
		ldr         bit_offset, [pBitOffset] 

		ldmia		p_stream, {four_bytes, tmp}
		mov			value, #1		
		add			tmp2, bit_offset, #1
		rsb			bit_offset, tmp2, #32
		mov			four_bytes, four_bytes, lsl bit_offset
		orr			four_bytes, four_bytes, tmp, lsr tmp2

		clz         num_zeros, four_bytes 

		rsb			tmp2, value, value, lsl num_zeros	; mask = (1 << num_zeros) - 1
		add			num_zeros, value, num_zeros, lsl #1	; (num_zeros * 2) + 1
		rsb			tmp, num_zeros, #32					; amount to shift four_bytes right
		and			value, tmp2, four_bytes, lsr tmp

		add         bit_offset, bit_offset, num_zeros
		rsbs		bit_offset, bit_offset, #31
		addmi		bit_offset, bit_offset, #32
		addmi		p_stream, p_stream, #4
		strmi		p_stream, [ppBitStream] 
		str			bit_offset, [pBitOffset] 
		
		add			return_value, value, tmp2
	MEND

|ippiDecodeExpGolombOne_H264_1u16s_unsigned_arm| PROC

		stmdb       sp!, {r4 - r7, lr} 

		ExpGolombOne_H264_1u16s

		ldmia       sp!, {r4 - r7, pc} 

	ENDP  ; |ippiDecodeExpGolombOne_H264_1u16s_arm|

|ippiDecodeExpGolombOne_H264_1u16s_signed_arm| PROC

		stmdb       sp!, {r4 - r7, lr} 

		ExpGolombOne_H264_1u16s

		add			return_value, return_value, #1
		tst			return_value, #1 
		mov			return_value, return_value, lsr #1 
		rsbne		return_value, return_value, #0 		

		ldmia       sp!, {r4 - r7, pc} 

	ENDP  ; |ippiDecodeExpGolombOne_H264_1u16s_arm|

  END


