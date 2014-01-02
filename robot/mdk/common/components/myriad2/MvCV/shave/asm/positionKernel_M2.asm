; ///
; /// @file
; /// @copyright All code copyright Movidius Ltd 2013, all rights reserved.
; ///            For License Warranty see: common/license.txt
; ///
; /// @brief
; ///



.version 00.51.04

.data .data.pixelPos

.code .text.pixelPos
;void pixelPos_asm(u8** srcAddr, u8* maskAddr, u32 width, u8 pixelValue, u32* pixelPosition, u8 status)
;                      i18           i17          i16        i15                  i14           i13
pixelPos_asm:
LSU0.LD.32 i18 i18 ;Loads input image
LSU0.LDIL i1 ___pixelPositionLoop || LSU1.LDIH i1 ___pixelPositionLoop ;address for loop to exe
IAU.SHR.u32 i2 i16 4 ; nr of loop iterations, retained in i2 = i16 / 16;(processing 16 pixels at once)
LSU1.LDIL i3 0x10 || CMU.CMZ.i8 i2 ;compare position within vrf to 0x10(this means that there was no pixel match)
LSU0.LDIL i4 0x11 ;load status with "1's", meaning there was a pixel match
IAU.SHL i15 i15 24 ; shift left and right made to make sure of having only de desired value in irf
IAU.SHR.u32 i15 i15 24 ; shift left and right made to make sure of having only de desired value in irf
CMU.CPIVR.x8 v1 i15 ;|| IAU.AND i7 i16 i2; copies the value of searched pixel into a vrf
IAU.XOR i6 i6 i6
LSU0.LDIL i6 0x000f ; loads value to and width with
IAU.AND i6 i16 i6 ; (last 4 bits needed)

IAU.XOR i8 i8 i8 ;resets irf
IAU.XOR i9 i9 i9 ;resets irf
IAU.XOR i10 i10 i10 ;resets irf - used to count nr. of reps. for pixels
IAU.XOR i11 i11 i11 ;resets irf - used to compute position within the next set of 16 values processed
IAU.XOR i12 i12 i12 ;resets irf - used to store position within line
PEU.PC1C EQ  || BRU.BRA ___compensation || IAU.SUB i11 i11 16
CMU.CPIVR.x8 v3 i8
IAU.XOR i5 i5 i5 ;resets irf
LSU0.LDIL i5 0xFFFF || LSU1.LDIH i5 0xFFFF ;fill pixel_pos with 0xffff where no pixel match exists
IAU.XOR i0 i0 i0
LSU0.LDIL i7 0x1    || VAU.XOR v7 v7 v7
nop
IAU.XOR i16 i16 i16 || CMU.CMZ.i8 i15 
PEU.PC1C EQ || CMU.CPIVR.x8 v7 i7

___pixelPositionLoop:
        LSU0.LDO.64.L v0 i18  0  || LSU1.LDO.64.H v0 i18 8 || IAU.ADD i18 i18 16 
        LSU0.LDO.64.L v2 i17  0  || LSU1.LDO.64.H v2 i17 8 || IAU.ADD i17 i17 16
        cmu.cpivr.x8 v5 i5
nop 5
        VAU.MUL.i8   v0  v0  v2
nop
        VAU.SUB.i8   v7  v2  v7
        CMU.CMZ.i8 i15
        PEU.PC1C EQ || VAU.OR v0 v0 v7
        CMU.CMVV.u8  v0  v3 
        PEU.PVV8 GTE   || VAU.ADIFF.u8 v5 v0 v1
        IAU.MUL      i11 i10 0x10
        CMU.VNZ.x8  i8  v5 0x0
        IAU.BSFINV  i9  i8
        IAU.ADD     i12 i11 i9
        CMU.CMII.u8 i9 i3
        PEU.PC1C LT || IAU.ADD i0 i0 1
        CMU.CMII.u8 i0 i7
        PEU.PC1C EQ || LSU0.ST.32 i12 i14 || LSU1.ST.32 i4 i13 || IAU.ADD i16 i16 0xff || SAU.XOR i6 i6 i6
        IAU.ADD     i10 i10 1
        IAU.ADD i16 i16 1
        CMU.CMII.u32 i16 i2
        PEU.PC1C LT || BRU.JMP i1
nop 6
        
___compensation:
        IAU.ADD i11 i11 16
        CMU.CMZ.i8 i6 || IAU.XOR i1 i1 i1 || SAU.XOR i2, i2, i2 || LSU0.LDIL i5 ___compensation_loop_end || LSU1.LDIH i5 ___compensation_loop_end
        LSU0.LDIL i21 0x01 || LSU1.LDIH i21 0x00 || IAU.XOR i20 i20 i20 ;use for counting the # of occurrences of pixelValue
		PEU.PC1C EQ   || BRU.BRA __final
		nop 6
; (Myriad2 COMPATIBILITY ISSUE): BRU.RPL: changed order of parameters at line number 76, in positionKernel.asm
        LSU0.LD.8 i1 i18 || IAU.ADD i18 i18 1 || bru.rpl i5 i6
        LSU0.LD.8 i2 i17 || IAU.ADD i17 i17 1
nop 6
___compensation_loop_end:
		CMU.CMZ.i8 i2
        PEU.PC1C EQ  ||  IAU.ADD i1, i15, 1
        CMU.CMII.u8 i1 i15
        PEU.PC1C EQ || IAU.ADD i20 i20 1
		PEU.PC1C EQ || CMU.CMII.u8 i20 i21
		PEU.PC1C EQ || LSU0.ST.32 i11 i14 || LSU1.ST.32 i4 i13
        IAU.ADD i11 i11 1
__final:
BRU.JMP i30
nop 6


.end
