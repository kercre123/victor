; ///
; /// @file
; /// @copyright All code copyright Movidius Ltd 2013, all rights reserved.
; ///            For License Warranty see: common/license.txt
; ///
; /// @brief
; ///

.version 00.50.34.0

;void LookupTable(const u16** src, u8** dest, const u8* lut, u32 width, u32 height)

.data .data.lut10to8
.code .text.lut10to8
LUT10to8_asm:

LSU0.LD.32 i18, i18                                        ;load the beginning address of src
LSU0.LD.32 i17, i17                                        ;load the beginning address of dst
IAU.SUB i19 i19 4 || lsu0.ldil i1, ___loop_end  || lsu1.ldih i1, ___loop_end
LSU0.ST.32 i20 i19 || IAU.SUB i19 i19 4
LSU0.ST.32 i21 i19 || IAU.SUB i19 i19 4
LSU0.ST.32 i22 i19 || IAU.SUB i19 i19 4
LSU0.ST.32 i23 i19 || IAU.SUB i19 i19 4
LSU0.ST.32 i24 i19 || lsu1.ldil i6 0x03ff    

LSU0.LDIL i22 0x000f || LSU1.LDIH i22 0x0
lsu0.ldil i24, ___compensate  || lsu1.ldih i24, ___compensate

iau.shr.u32 i2, i15, 4 || cmu.cpivr.x32 v6 i6
iau.xor i0 i0 i0       || cmu.cpivr.x32 v0 i16



CMU.CMZ.i32  i2 || iau.and i23 i22 i15
peu.pc1c eq || bru.jmp i24
nop 6

___pre_processing:
LSU0.LDI.128.U16.U32 v1 i18 
LSU0.LDI.128.U16.U32 v2 i18 
LSU0.LDI.128.U16.U32 v3 i18 
LSU0.LDI.128.U16.U32 v4 i18 
nop 3
vau.and v5, v1, v6
vau.and v7, v2, v6
vau.and v8, v3, v6
vau.and v9, v4, v6
VAU.ADD.i32 v10  v5 v0
VAU.ADD.i32 v11  v7 v0
cmu.cpvi i0, v10.0  || VAU.ADD.i32 v12  v8 v0
cmu.cpvi i3, v10.1  || LSU0.LD.16 i0, i0  || VAU.ADD.i32 v13  v9 v0
cmu.cpvi i4, v10.2  || LSU0.LD.16 i3, i3
; (Myriad2 COMPATIBILITY ISSUE): BRU.RPL: changed order of parameters at line number 53, in LookUpT10to8.asm
cmu.cpvi i5, v10.3  || LSU0.LD.16 i4, i4 || LSU1.LD.128.U16.U32 v1 i18 || IAU.ADD i18 i18 8 || bru.rpl i1, i2
cmu.cpvi i6, v11.0  || LSU0.LD.16 i5, i5 || LSU1.LD.128.U16.U32 v2 i18 || IAU.ADD i18 i18 8
cmu.cpvi i7, v11.1  || LSU0.LD.16 i6, i6 || LSU1.LD.128.U16.U32 v3 i18 || IAU.ADD i18 i18 8
cmu.cpvi i8, v11.2  || LSU0.LD.16 i7, i7 || LSU1.LD.128.U16.U32 v4 i18 || IAU.ADD i18 i18 8
nop
cmu.cpvi i9, v11.3  || LSU0.LD.16 i8, i8   || LSU1.STO.8 i0 i17 0 
cmu.cpvi i10, v12.0 || LSU0.LD.16 i9, i9   || LSU1.STO.8 i3 i17 1
cmu.cpvi i11, v12.1 || LSU0.LD.16 i10, i10 || LSU1.STO.8 i4 i17 2   || vau.and v5, v1, v6
cmu.cpvi i12, v12.2 || LSU0.LD.16 i11, i11 || LSU1.STO.8 i5 i17 3   || vau.and v7, v2, v6
cmu.cpvi i13, v12.3 || LSU0.LD.16 i12, i12 || LSU1.STO.8 i6 i17 4   || vau.and v8, v3, v6
cmu.cpvi i14, v13.0 || LSU0.LD.16 i13, i13 || LSU1.STO.8 i7 i17 5   || vau.and v9, v4, v6
nop
cmu.cpvi i15, v13.1 || LSU0.LD.16 i14, i14 || LSU1.STO.8 i8 i17 6   || VAU.ADD.i32 v10  v5 v0
cmu.cpvi i20, v13.2 || LSU0.LD.16 i15, i15 || LSU1.STO.8 i9 i17 7   || VAU.ADD.i32 v11  v7 v0
cmu.cpvi i21, v13.3 || LSU0.LD.16 i20, i20 || LSU1.STO.8 i10 i17 8  || VAU.ADD.i32 v12  v8 v0
LSU0.LD.16 i21, i21  || LSU1.STO.8 i11 i17 9  || VAU.ADD.i32 v13  v9 v0
nop
___loop_end:
cmu.cpvi i0, v10.0 || LSU0.STO.8 i12 i17 10
cmu.cpvi i3, v10.1  || LSU0.LD.16 i0, i0 || LSU1.STO.8 i13 i17 11
cmu.cpvi i4, v10.2  || LSU0.LD.16 i3, i3 || LSU1.STO.8 i14 i17 12
LSU0.STO.8 i15 i17 13
LSU0.STO.8 i20 i17 14
LSU0.STO.8 i21 i17 15 || IAU.ADD i17 i17 16
nop
___compensate:
nop 2
    lsu0.ldil i1 ___loop  || lsu1.ldih i1 ___loop
	lsu0.ldil i4 ___final || lsu1.ldih i4 ___final
    lsu1.ldil i3 0x0  || iau.xor i2 i2 i2
    cmu.cmii.i8 i23 i3
	lsu0.ldil i6 0x03ff || lsu1.ldih i6 0x0
    peu.pc1c eq || bru.jmp i4
nop 6
	
___loop:
    LSU0.LDI.16 i7, i18 
	IAU.ADD i2 i2 1
	cmu.cmii.i8 i2 i23
nop 4
	IAU.AND i7 i7 i6
	IAU.ADD i12 i7 i16
	LSU0.LD.8 i11, i12
	peu.pc1c lt || bru.jmp i1
nop 5
	LSU0.ST.8 i11, i17 ||  iau.add i17 i17 1
	
	
___final:
LSU0.LD.32  i24  i19 || IAU.ADD i19 i19 4
LSU0.LD.32  i23  i19 || IAU.ADD i19 i19 4
LSU0.LD.32  i22  i19 || IAU.ADD i19 i19 4
LSU0.LD.32  i21  i19 || IAU.ADD i19 i19 4
LSU0.LD.32  i20  i19 || IAU.ADD i19 i19 4
nop 6

bru.jmp i30
nop 6
.end
