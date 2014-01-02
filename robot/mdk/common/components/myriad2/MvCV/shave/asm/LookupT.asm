; ///
; /// @file
; /// @copyright All code copyright Movidius Ltd 2013, all rights reserved.
; ///            For License Warranty see: common/license.txt
; ///
; /// @brief
; ///

.version 00.50.34

;void LookupTable(const u8** src, u8** dest, const u8* lut, u32 width, u32 height)
;						i18			i17			i16				i15			i14

.data .data.lut8to8
.code .text.lut8to8

LUT8to8_asm:
.lalign

lsu0.ldil i1, ___loop_end  || lsu1.ldih i1, ___loop_end
lsu0.ldil i11, ___compensation  || lsu1.ldih i11, ___compensation
LSU0.LD.32 i18, i18  || iau.xor i10 i10 i10                                  ;load the beginning address of src
LSU0.LD.32 i17, i17  || iau.xor i7 i7 i7                                  ;load the beginning address of src
cmu.cpii  i16, i16  || iau.xor i0 i0 i0                 ;load LUT pointer
iau.shr.u32 i2, i15, 4
LSU1.LDIL i6 0x000f || LSU0.LDIH i6 0x0                                     ;load value to and width with
iau.and i0 i6 i15

CMU.CMZ.i32  i2
peu.pc1c eq || bru.jmp i11
nop 6

___loop_start:
; (Myriad2 COMPATIBILITY ISSUE): BRU.RPL: changed order of parameters at line number 34, in LookupT.asm
    LSU0.LDO.8 i7, i18, 0 || LSU1.LDO.8 i10, i18, 8 || bru.rpl i1, i2
    LSU0.LDO.8 i7, i18, 1 || LSU1.LDO.8 i10, i18, 9
    LSU0.LDO.8 i7, i18, 2 || LSU1.LDO.8 i10, i18, 10
    LSU0.LDO.8 i7, i18, 3 || LSU1.LDO.8 i10, i18, 11
    LSU0.LDO.8 i7, i18, 4 || LSU1.LDO.8 i10, i18, 12
    LSU0.LDO.8 i7, i18, 5 || LSU1.LDO.8 i10, i18, 13
	nop
    LSU0.LDO.8 i7, i18, 6 || LSU1.LDO.8 i10, i18, 14         || iau.add i11, i7, i16  || sau.add.i32 i5, i10, i16  //add to LUT pointer the offset value (i7/i10)
    LSU0.LDO.8 i7, i18, 7 || LSU1.LDO.8 i10, i18, 15         || iau.add i12, i7, i16  || sau.add.i32 i4, i10, i16

    LSU0.LD.32.u8.u32 i11, i11 || LSU1.LD.32.u8.u32 i5, i5   || iau.add i13, i7, i16  || sau.add.i32 i3, i10, i16//load from LUT the output value
    LSU0.LD.32.u8.u32 i12, i12 || LSU1.LD.32.u8.u32 i4, i4   || iau.add i14, i7, i16  || sau.add.i32 i11, i10, i16
    LSU0.LD.32.u8.u32 i13, i13 || LSU1.LD.32.u8.u32 i3, i3   || iau.add i15, i7, i16  || sau.add.i32 i12, i10, i16
    LSU0.LD.32.u8.u32 i14, i14 || LSU1.LD.32.u8.u32 i11, i11 || iau.add i8,  i7, i16  || sau.add.i32 i13, i10, i16
	nop
    LSU0.LD.32.u8.u32 i15, i15 || LSU1.LD.32.u8.u32 i12, i12 || iau.add i9,  i7, i16  || sau.add.i32 i14, i10, i16
    LSU0.LD.32.u8.u32 i8, i8   || LSU1.LD.32.u8.u32 i13, i13 || iau.add i10, i7, i16  || sau.add.i32 i15, i10, i16
    LSU0.LD.32.u8.u32 i9, i9   || LSU1.LD.32.u8.u32 i14, i14
    LSU0.LD.32.u8.u32 i10, i10 || LSU1.LD.32.u8.u32 i15, i15 || iau.add  i18, i18, 16



    LSU0.STO.8 i11, i17, 0  || LSU1.STO.8 i5 , i17, 8
    LSU0.STO.8 i12, i17, 1  || LSU1.STO.8 i4, i17, 9
___loop_end:
    LSU0.STO.8 i13, i17, 2  || LSU1.STO.8 i3, i17, 10
    LSU0.STO.8 i14, i17, 3  || LSU1.STO.8 i11, i17, 11
    LSU0.STO.8 i15, i17, 4  || LSU1.STO.8 i12, i17, 12
    LSU0.STO.8 i8,  i17, 5  || LSU1.STO.8 i13, i17, 13
    LSU0.STO.8 i9,  i17, 6  || LSU1.STO.8 i14, i17, 14
    LSU0.STO.8 i10, i17, 7  || LSU1.STO.8 i15, i17, 15  || iau.add i17, i17, 16
	nop

___compensation:
    lsu0.ldil i1 ___compensate || lsu1.ldih i1 ___compensate
    lsu1.ldil i3 0x0  || iau.xor i2 i2 i2
    cmu.cmii.i8 i0 i3
    peu.pc1c eq || bru.jmp i30
nop 6


___compensate:
    LSU0.LD.8 i7, i18 || iau.add i18 i18 1
	IAU.ADD i2 i2 1
	cmu.cmii.i8 i2 i0
nop 5
	IAU.ADD i12 i7 i16
	LSU0.LD.8 i11, i12
	peu.pc1c lt || bru.jmp i1
nop 5
	LSU0.ST.8 i11, i17 ||  iau.add i17 i17 1





BRU.jmp i30
nop 6
.end
