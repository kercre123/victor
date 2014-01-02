; ///
; /// @file
; /// @copyright All code copyright Movidius Ltd 2013, all rights reserved.
; ///            For License Warranty see: common/license.txt
; ///
; /// @brief
; ///

.version 00.51.04


.data .data.minMaxPos
.code .text.minMaxPos
;void minMaxLocat_asm(u8** in, u32 width, u8* minVal, u8* maxVal, u32* minPos, u32* maxPos, u8* maskAddr);
;                      i18        i17          i16        i15          i14           i13         i12
.lalign
minMaxPos_asm:

IAU.SUB i19 i19 4 ; save i20, i21 to stack
LSU0.ST.32 i20 i19 || IAU.SUB i19 i19 4
LSU0.ST.32 i21 i19
 
LSU0.LD.32 i18 i18            ; loads input img
LSU0.LDIL i3 0x000f
CMU.CMII.u32 i17, i3
PEU.PC1C LTE || BRU.BRA  ___compensation || LSU0.LDIL i4 0xff || IAU.XOR i5 i5 i5
IAU.AND i6 i17 i3   
IAU.SHR.u32 i3 i17 4  ; nr of repetitions i1 = i17/16(processing 8 pixels at once)

CMU.CPII i20 i12              || SAU.ISUBS.U32 i8 i17 i6

CMU.CPII i21 i18
LSU1.LDIL i0 0x0             || IAU.XOR i1 i1 i1 ; minVal
LSU1.LDIL i2 0xFF            || IAU.XOR i5 i5 i5 ; maxVal ;loads 0xFF in order to find out min value
LSU0.LDIL i4 ___min_max_loop || LSU1.LDIH i4 ___min_max_loop
CMU.CPIVR.x8 v1 i0                               ; maxVal copied in vec.
CMU.CPIVR.x8 v2 i2           || LSU1.LDIL i1 0x1 ; minVal copied in vec.

___min_max_loop_head:
; (Myriad2 COMPATIBILITY ISSUE): BRU.RPL: changed order of parameters at line number 33, in minMaxPosition.asm
    LSU0.LDO.64.L v4 i18 0 || LSU1.LDO.64.H v4 i18 8 || BRU.RPL i4 i3
    LSU0.LDO.64.L v5 i18 0 || LSU1.LDO.64.H v5 i18 8 || IAU.ADD i18 i18 16
    LSU0.LDO.64.L v6 i12 0 || LSU1.LDO.64.H v6 i12 8 || IAU.ADD i12 i12 16	
nop 5
    CMU.CPIVR.x8 v7 i1
    VAU.SUB.i8 v7 v6 v7    
   
___min_max_loop:
        ;multiply pixel values with mask values
        VAU.MUL.i8 v5 v5 v6
        ;multiply pixel values with mask values
        VAU.OR v4 v4 v7
nop
        ;max operation needed for finding out the maximum values
        CMU.MAX.u8 v1 v5 v1
nop
        CMU.MIN.u8 v2 v4 v2
nop

___store_min_max:
        CMU.MAX.u8 v1, v1, v1 || LSU0.SWZMC4.BYTE [2301] [UUUU]
        CMU.MIN.u8 v2, v2, v2 || LSU1.SWZMC4.BYTE [2301] [UUUU]
        CMU.MAX.u8 v1, v1, v1 || LSU0.SWZMC4.BYTE [1032] [UUUU]
        CMU.MIN.u8 v2, v2, v2 || LSU1.SWZMC4.BYTE [1032] [UUUU]
        CMU.MAX.u8 v1, v1, v1 || LSU0.SWZMC4.WORD [2301] [UUUU]
        CMU.MIN.u8 v2, v2, v2 || LSU1.SWZMC4.WORD [2301] [UUUU]
        CMU.MAX.u8 v1, v1, v1 || LSU0.SWZMC4.WORD [1032] [UUUU]
        CMU.MIN.u8 v2, v2, v2 || LSU1.SWZMC4.WORD [1032] [UUUU] ;last performed swizzle operation leads to a vrf
                                                       ;                filled with max computed values
nop 2

        CMU.CPVI i5 v1.3
        CMU.CPVI i4 v2.3 || IAU.SHR.u32 i5 i5 24
        IAU.SHR.u32 i4 i4 24

        LSU0.ST.8 i5 i15 ;stores the smallest value found in the img
        LSU0.ST.8 i4 i16 ; stores the highest value found in the img

___compensation:
    	CMU.CMZ.i8 i6       || LSU0.LDIL i7 ___compensation_end || LSU1.LDIH i7 ___compensation_end
    	PEU.PC1C EQ         || BRU.BRA ___min_max_calc
        IAU.ADD i0 i8 0     || SAU.XOR i9 i9 i9 ; min position counter
     	nop 5
; (Myriad2 COMPATIBILITY ISSUE): BRU.RPL: changed order of parameters at line number 76, in minMaxPosition.asm
        LSU0.LD.8 i1 i18 || IAU.ADD i18 i18 1 || bru.rpl i7 i6
        LSU0.LD.8 i2 i12 || IAU.ADD i12 i12 1
nop 5
	    CMU.CPII i3, i1
	    CMU.CMZ.i8 i2 ; check if mask value is 0
___compensation_end:
	    PEU.PC1C EQ || LSU0.LDIL i1 0xFF || IAU.XOR i3 i3 i3
	    CMU.CMII.u8 i1 i4        
        PEU.PC1C LT ||  CMU.CPII i4 i1 || IAU.ADD i10 i0 0 || SAU.ADD.i8 i9 i9 1
        CMU.CMII.u8 i3 i5
        PEU.PC1C GT ||  CMU.CPII i5 i3 || IAU.ADD i11 i0 0 || SAU.ADD.i8 i9 i9 1
        IAU.ADD i0 i0 1
		nop	

	LSU0.ST.8 i5 i15   || LSU1.ST.8 i4 i16 ;stores the smallest and highest values found in the img
	CMU.CMZ.i8 i9
	PEU.PC1C NEQ      || LSU0.ST.32 i10 i14 || LSU1.ST.32 i11 i13
	PEU.PC1C NEQ      || BRU.JMP i30  
___min_max_calc:
;-----------------------------------------------------------------------------------------------
; starts looping for position of minimum elements from each line
IAU.XOR i0  i0  i0  || SAU.SHR.u32 i3 i17 4
IAU.XOR i8  i8  i8  || SAU.SUB.i32 i7 i7 i7
IAU.XOR i1  i1  i1  || VAU.XOR v3 v3 v3 

IAU.XOR i4  i4  i4  || CMU.CPII i12 i20 || LSU0.LDIL i5 0x10

IAU.XOR i9  i9  i9  || CMU.CPII i18 i21
IAU.XOR i10 i10 i10 || LSU0.LDIL i6 ___min_pos_loop || LSU1.LDIH i6 ___min_pos_loop
IAU.XOR i11 i11 i11 || CMU.CPVI i2 v2.0
CMU.CMZ.i8 i2       || IAU.ADD i7 i7 1
PEU.PC1C EQ         || CMU.CPIVR.x8 v3 i7

.lalign
___min_pos_loop:
    LSU0.LDO.64.L v8 i18 0 || LSU1.LDO.64.H v8 i18 8 || IAU.ADD i18 i18 16
    LSU0.LDO.64.L v10 i12 0 || LSU1.LDO.64.H v10 i12 8 || IAU.ADD i12 i12 16
nop 6
    VAU.MUL.i8   v8 v8 v10
nop
    VAU.SUB.i8   v4  v10  v3
    CMU.CMZ.i8 i2
    PEU.PC1C EQ || VAU.OR v8 v8 v4
    VAU.ADIFF.u8 v7 v8 v2
    IAU.MUL  i9 i10 0x10
    CMU.VNZ.x8  i0 v7 0x0
    IAU.BSFINV  i1 i0
    IAU.ADD     i8 i9 i1
    CMU.CMII.u8 i1 i5
    PEU.PC1C LT || IAU.ADD i11 i11 1
    CMU.CMII.u8 i11 i7
    PEU.PC1C EQ || LSU0.ST.32 i8 i14 || IAU.ADD i4 i4 0xFF
    IAU.ADD    i10 i10 1
    IAU.ADD i4 i4 1
    CMU.CMII.u32 i4 i3
    PEU.PC1C LT || BRU.JMP i6
nop 6
    

;-----------------------------------------------------------------------------------------------
; starts looping for position of maximum elements from each line

CMU.CPII i18 i21
LSU0.LDIL i6 ___maxim_pos_loop || LSU1.LDIH i6 ___maxim_pos_loop
LSU1.LDIL i5 0x10  ;compare position within vrf to 0x10(this means that there was no pixel match)
; (Myriad2 COMPATIBILITY ISSUE): CMU.CPII: SRF version no longer available at line number 138, in minMaxPosition.asm
IAU.XOR i0  i0  i0 || CMU.CPII i12 i20
IAU.XOR i8  i8  i8
IAU.XOR i1  i1  i1
IAU.XOR i4  i4  i4
IAU.XOR i9  i9  i9
IAU.XOR i10 i10 i10
IAU.XOR i11 i11 i11
IAU.XOR i7 i7 i7
LSU0.LDIL i7 0x1

.lalign
___maxim_pos_loop:
    LSU0.LDO.64.L v9 i18 0 || LSU1.LDO.64.H v9 i18 8 || IAU.ADD i18 i18 16
    LSU0.LDO.64.L v10 i12 0 || LSU1.LDO.64.H v10 i12 8 || IAU.ADD i12 i12 16
nop 6
    VAU.MUL.i8   v9 v9 v10
nop 2
    VAU.ADIFF.u8 v6 v9 v1
    IAU.MUL  i9 i10 0x10
    CMU.VNZ.x8  i0 v6 0x0
    IAU.BSFINV  i1 i0
    IAU.ADD     i8 i9 i1
    CMU.CMII.u8 i1 i5
    PEU.PC1C LT || IAU.ADD i11 i11 1
    CMU.CMII.u8 i11 i7	
    PEU.PC1C EQ || LSU0.ST.32 i8 i13 || IAU.ADD i4 i4 0xFF
    IAU.ADD    i10 i10 1
    IAU.ADD i4 i4 1
    CMU.CMII.u32 i4 i3
    PEU.PC1C LT || BRU.JMP i6
nop 6

; restore i20 and i21
LSU0.LD.32 i21 i19 || IAU.ADD i19 i19 4
LSU0.LD.32 i20 i19 || IAU.ADD i19 i19 4
nop 6


    bru.jmp i30
	nop 6




.end
