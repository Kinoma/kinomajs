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

        GBLL    DEBUG_ON
DEBUG_ON SETL   {FALSE}

        GBLL    COMMENT_ON
DEBUG_ON SETL   {FALSE}


;//;;;;;;;;;;;;;;;;;;;;;;;;;;
;//;; a function template ;;;
	IF COMMENT_ON
;//;;;;;;;;;;;;;;;;;;;;;;;;;;

	INCLUDE kinoma_arm_asm_utilities-s.h	;//header, this file as example
	
	IMPORT external_proc
	
	AREA  |.text|, CODE, READONLY
	
	EXPORT  |my_example_proc_arm_v6|


reg_in_0	RN 0		;//register input, maximum 4
reg_in_1	RN 1
reg_in_2	RN 2
reg_in_3	RN 3
			
stack_in_4	RN 4		;//input by stack following register input
stack_in_5	RN 5
stack_in_6	RN 6

local_in_0	RN 7		;//local variable
local_in_1	RN 8


RA			RN 4		;first register to push into stack at entry
RZ			RN 8		;second last, and then it is lr=>pc


REGIS_SHIFT_id			EQU	(6*4)								;//registers pushed into stack to preserve caller state: 6 here as example
LOCAL_SHIFT_id			EQU	(2*4)								;//local stack variables: 2 here as example
SP_SHIFT_id				EQU	(REGIS_SHIFT_id + LOCAL_SHIFT_id)	;//total up shift from stack input

local_0_SHIFT_id 		EQU (0)									;//local stack variables: 2 here as example		
local_1_SHIFT_id 		EQU (4)

stack_in_4_SHIFT_id 	EQU (0*4 + SP_SHIFT_id)					;//stack input, 3 here as example
stack_in_5_SHIFT_id 	EQU (1*4 + SP_SHIFT_id)
stack_in_6_SHIFT_id 	EQU (1*4 + SP_SHIFT_id)

|my_example_proc_arm_v6| PROC

	stmdb   sp!, {RA-RZ, lr}								;//!!!MUST MATCH!!! REGIS_SHIFT_id, preserve 6 registers here as example
	sub     sp, sp, #LOCAL_SHIFT_id							;//expend stack for local stack variables
	
	;get stack input
    ldr		stack_in_4, [sp, #stack_in_4_SHIFT_id]
    ldr		stack_in_5, [sp, #stack_in_5_SHIFT_id]
    ldr		stack_in_6, [sp, #stack_in_6_SHIFT_id]

	;//
	;//do things and generate values in local stack variables
	;//

	str		local_in_0, [sp, #local_0_SHIFT_id]				;//push local stack variables to stack
	str		local_in_1, [sp, #local_1_SHIFT_id]	

	;//
	;//safe to corrupt registers used by local stack variable
	;//

	;load back local stack variables
	ldr		local_in_0, [sp, #local_0_SHIFT_id]	
	ldr		local_in_1, [sp, #local_1_SHIFT_id]	

	;//do other things
	
	add		sp, sp, #LOCAL_SHIFT_id							;//recover stack position in entry
	ldmia   sp!, {RA-RZ, pc}								

	ENDP  ; |my_example_proc_arm_v6|

  END

;//;;;;;;;;;;;;;;;;;;;;;;;;;;
;//;; a function template ;;;
       ENDIF   ;COMMENT_ON
;//;;;;;;;;;;;;;;;;;;;;;;;;;;

    ;//==========================================================================
    ;// Debug call to printf
    ;//  M_PRINTF $format, $val0, $val1, $val2
    ;//
    ;// Examples:
    ;//  M_PRINTF "x=%08x\n", r0
    ;//
    ;// This macro preserves the value of all registers including the
    ;// flags.
    ;//==========================================================================

    MACRO
    M_PRINTF  $mark, $format, $val0, $val1, $val2, 
    IF DEBUG_ON
    
    IMPORT  printf
    LCLA    nArgs
nArgs	SETA    0
    
    ;// save registers so we don't corrupt them
    STMFD   sp!, {r0-r12, lr}
    
    ;// Drop stack to give us some workspace
    SUB     sp, sp, #16
    
    ;// Save registers we need to print to the stack
    IF "$val2" <> ""
        ASSERT "$val1" <> ""
        STR    $val2, [sp, #8]
nArgs       SETA   nArgs+1
    ENDIF
    IF "$val1" <> ""
        ASSERT "$val0" <> ""
        STR    $val1, [sp, #4]
nArgs	    SETA   nArgs+1
    ENDIF
    IF "$val0"<>""
        STR    $val0, [sp]
nArgs	    SETA   nArgs+1
    ENDIF
    
    ;// Now we are safe to corrupt registers
    ;bnie***
	;ADR     r0, %FT00
    ADR     r0, $mark.00
    IF nArgs=1
      LDR   r1, [sp]
    ENDIF
    IF nArgs=2
      LDMIA sp, {r1,r2}
    ENDIF
    IF nArgs=3
      LDMIA sp, {r1,r2,r3}
    ENDIF
    
    ;// print the values
    MRS     r4, cpsr        ;// preserve flags
    BL      printf
    MSR     cpsr_f, r4      ;// restore flags
    ;bnie***
	;B       %FT01
    B       $mark.01
;bnie***
$mark.00      ;// string to print
    DCB     "$format", 0
    ALIGN
;bnie***
$mark.01      ;// Finished
    ADD     sp, sp, #16
    ;// Restore registers
    LDMFD	sp!, {r0-r12,lr}

    ENDIF   ;// DEBUG_ON

    MEND


  END
