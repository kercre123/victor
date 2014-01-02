; /// @file
; /// @copyright All code copyright Movidius Ltd 2013, all rights reserved.
; ///            For License Warranty see: common/license.txt
; ///
; /// @brief
; ///

.version 00.51.04

.data .data.absoluteDiff

.code .text.absoluteDiff
;void AbsDiff(u8** in1(i18), u8** in2(i17), u8** out(i16),u32 width(i15));
AbsoluteDiff_asm:
LSU0.LDIL i2 0x001F || LSU1.LDIH i2 0x0 
LSU0.LD32 i18 i18 || LSU1.LD32 i17 i17
LSU0.LD32 i16 i16
IAU.AND i3 i15 i2 ;width mod 32
LSU0.ldil i5, ___loop || LSU1.ldih i5, ___loop || IAU.SHR.u32  i1 i15 5

CMU.CMZ.i32  i1 || LSU0.ldil i7, ___compensate || LSU1.ldih i7, ___compensate
peu.pc1c eq || bru.jmp i7
nop 5

LSU0.LDo64.l        v1  i18 0  || LSU1.LDo64.l        v2  i17 0 
LSU0.LDo64.h        v1  i18 8  || LSU1.LDo64.h        v2  i17 8 
LSU0.LDo64.l        v3  i18 16 || LSU1.LDo64.l        v4  i17 16
LSU0.LDo64.h        v3  i18 24 || LSU1.LDo64.h        v4  i17 24 
IAU.SUB i10 i10 i10 || bru.rpl i5 i1
nop
IAU.ADD i18 i18 32 
IAU.ADD i17 i17 32
___loop:
VAU.ADIFF.u8  v5 v1 v2 || LSU0.LDo64.l        v1  i18 0  || LSU1.LDo64.l        v2  i17 0 
VAU.ADIFF.u8  v6 v3 v4 || LSU0.LDo64.h        v1  i18 8  || LSU1.LDo64.h        v2  i17 8 
LSU0.ST64.l        v5  i16  || IAU.ADD i16 i16 8 || LSU1.LDo64.l        v3  i18 16
LSU0.ST64.h        v5  i16  || IAU.ADD i16 i16 8 || LSU1.LDo64.l        v4  i17 16
LSU0.ST64.l        v6  i16  || IAU.ADD i16 i16 8 || LSU1.LDo64.h        v3  i18 24
LSU0.ST64.h        v6  i16  || IAU.ADD i16 i16 8 || LSU1.LDo64.h        v4  i17 24 



CMU.CMZ.i32  i3
peu.pc1c eq || bru.jmp i30
nop 5


___compensate:
LSU0.LDo64.l        v1  i18 0  || LSU1.LDo64.l        v2  i17 0  || IAU.SUB i19 i19 0x20
LSU0.LDo64.h        v1  i18 8  || LSU1.LDo64.h        v2  i17 8  || IAU.SUB i12  i19 0
LSU0.LDo64.l        v3  i18 16 || LSU1.LDo64.l        v4  i17 16 || IAU.SUB i13  i19 0
LSU0.LDo64.h        v3  i18 24 || LSU1.LDo64.h        v4  i17 24 
nop 2
IAU.ADD i18 i18 32 
IAU.ADD i17 i17 32
VAU.ADIFF.u8  v5 v1 v2
VAU.ADIFF.u8  v6 v3 v4

LSU0.ST64.l        v5  i12  || IAU.ADD i12 i12 8
LSU0.ST64.h        v5  i12  || IAU.ADD i12 i12 8
LSU0.ST64.l        v6  i12  || IAU.ADD i12 i12 8
LSU0.ST64.h        v6  i12  || IAU.ADD i12 i12 8
LSU0.ldil i8, ___compensate_loop || LSU1.ldih i8, ___compensate_loop

lsu0.ldi8 i7, i13 || bru.rpl i8 i3
___compensate_loop:
nop 5
lsu0.st8 i7, i16 || IAU.ADD i16 i16 1

IAU.ADD i19 i19 0x20



BRU.jmp i30
nop 5

.end
 

