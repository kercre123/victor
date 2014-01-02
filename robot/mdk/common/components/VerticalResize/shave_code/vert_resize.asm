///   
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved. 
///            For License Warranty see: common/license.txt   
///
/// @brief     Vertical resize with Lanczos filtering. 
/// 
 
.version 00.50.05

.include shared_types.incl

.code .text.OutLineCompute
.salign 16
;void OutLineCompute(unsigned char* input_lines[6](i18), unsigned char* out_line(i17),
; unsigned int width(i16), unsigned char filters_offset(i15))
    OutLineCompute:
    lsu0.ldil i0, precalc_filters || lsu1.ldih i0, precalc_filters
    ;Load the lines pointers addresses || load precalculated filters
    
    ;And compute proper place to load coeficients from by multiplying with 6*2
    iau.mul i15, i15, 12
    nop
    ;And add address offset
    iau.add i0, i0, i15
    
    lsu0.ldo32 i4, i18, 0 || lsu1.ld64.l v6, i0
    lsu0.ldo32 i5, i18, 4 || lsu1.ldo64.h v6, i0, 8
    ;                     || Take the number of batches of 8 pixels existing
    lsu0.ldo32 i6, i18, 8 || iau.shr.u32 i10, i16, 3
    lsu0.ldo32 i7, i18, 12
    lsu0.ldo32 i8, i18, 16
    lsu0.ldo32 i9, i18, 20
    iau.sub i10, i10, 1
    ;Take in value 1 of filter
    lsu0.swzv8 [11111111] || vau.swzword v7, v6, [3210]
    ;and do the same for all values
    lsu0.swzv8 [22222222] || vau.swzword v8, v6, [3210]
    lsu0.swzv8 [33333333] || vau.swzword v9, v6, [3210]
    lsu0.swzv8 [44444444] || vau.swzword v10, v6, [3210]
    lsu0.swzv8 [55555555] || vau.swzword v11, v6, [3210]
    lsu0.swzv8 [00000000] || vau.swzword v6, v6, [3210]
    
    ;cmu.cpii i4, i4
    ;cmu.cpii i5, i5
    ;cmu.cpii i6, i6
    ;cmu.cpii i7, i7
    ;cmu.cpii i8, i8
    ;cmu.cpii i9, i9
    
___Process8values:
    ;Load 8 values of each line
    lsu0.ld128.u8.f16 v0, i4 || iau.add i4, i4, 8
    lsu0.ld128.u8.f16 v1, i5 || iau.add i5, i5, 8
    lsu0.ld128.u8.f16 v2, i6 || iau.add i6, i6, 8
    lsu0.ld128.u8.f16 v3, i7 || iau.add i7, i7, 8
    lsu0.ld128.u8.f16 v4, i8 || iau.add i8, i8, 8
    lsu0.ld128.u8.f16 v5, i9 || iau.add i9, i9, 8
    ;Start multipling
    vau.mul.f16 v0, v0, v6
    vau.mul.f16 v1, v1, v7
    vau.mul.f16 v2, v2, v8
    vau.mul.f16 v3, v3, v9
    vau.mul.f16 v4, v4, v10
    vau.mul.f16 v5, v5, v11
    ;Start adding
    vau.add.f16 v0, v0, v1
    vau.add.f16 v2, v2, v3
    vau.add.f16 v4, v4, v5
    nop
    vau.add.f16 v0, v0, v2 || iau.sub i10, i10, 1
    peu.pc1i 0x3 || bru.bra ___Process8values
    nop
    vau.add.f16 v0, v0, v4
    nop
    nop
    lsu0.st128.f16.u8 v0, i17 || iau.add i17, i17, 8

    bru.jmp i30
    nop
    nop
    nop
    nop
    nop
.end
