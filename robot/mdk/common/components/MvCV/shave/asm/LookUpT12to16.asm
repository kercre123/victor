; ///
; /// @file
; /// @copyright All code copyright Movidius Ltd 2013, all rights reserved.
; ///            For License Warranty see: common/license.txt
; ///
; /// @brief
; ///

.version 00.50.31.7

;void LookupTable(const u8** src, u8** dest, const u8* lut, u32 width, u32 height)

.data .data.lut12to16
.code .text.lut12to16
LUT12to16_asm:



lsu0.ldil i1, ___loop_end  || lsu1.ldih i1, ___loop_end
lsu0.ld32 i18, i18                                        ;load the beginning address of src
lsu0.ld32 i17, i17                                        ;load the beginning address of dst
lsu1.ldil i6 0x0fff                                       ;loads value to and width with
cmu.cpii  i16, i16                                        ;load LUT pointer
iau.shr.u32 i2, i15, 2 || cmu.cpivr.x32 v6 i6
iau.xor i0 i0 i0       || cmu.cpivr.x32 v5 i16

___pre_processing:
lsu0.ldi128.u16.u32 v0 i18
nop 5
vau.and v1, v0, v6
vau.shl.u32 v2, v1, 0x01
nop
vau.add.i32 v3, v2, v5
nop


cmu.cpvi i0, v3.0
cmu.cpvi i10, v3.1 || lsu0.ld16 i4, i0
cmu.cpvi i11, v3.2 || lsu0.ld16 i5, i10
cmu.cpvi i3, v3.3  || lsu0.ld16 i6, i11
lsu0.ld16 i7, i3
nop 2
// first stop

lsu0.ldi128.u16.u32 v0, i18
nop 5
vau.and v1, v0, v6
vau.shl.u32 v2, v1, 0x01
nop
vau.add.i32 v3, v2, v5
nop
// second stop


___loop_end_start:
lsu0.ldi128.u16.u32 v0, i18  ||  bru.rpl i1 i2
nop 5
// third stop

lsu0.sti16 i4, i17 ||cmu.cpvi i0, v3.0  || vau.and v1, v0, v6
___loop_end:
lsu0.sti16 i5, i17 ||cmu.cpvi i10, v3.1  || lsu1.ld16 i4, i0  || vau.shl.u32 v2, v1, 0x01
lsu0.sti16 i6, i17 ||cmu.cpvi i11, v3.2  || lsu1.ld16 i5, i10
lsu0.sti16 i7, i17 ||cmu.cpvi i3, v3.3   || lsu1.ld16 i6, i11 || vau.add.i32 v3, v2, v5
lsu1.ld16 i7, i3
nop 2


bru.jmp i30
nop 5
.end
