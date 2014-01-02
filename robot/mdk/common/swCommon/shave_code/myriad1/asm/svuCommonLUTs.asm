; ///
; /// @file
; /// @copyright All code copyright Movidius Ltd 2012, all rights reserved.
; ///            For License Warranty see: common/license.txt
; ///
; /// @brief    
; ///

.version 00.51.05
;-------------------------------------------------------------------------------

.code .text.svuGet16BitVals16BitLUT
;ushort8 svuGets16BitVals16BitLUT(short8 in_values(v23), s16* lut_memory(i18));
;ushort8 svuGetu16BitVals16BitLUT(ushort8 in_values(v23), u16* lut_memory(i18));
;_svuGet16BitVals16BitLUT:
svuGets16BitVals16BitLUT:
svuGetu16BitVals16BitLUT:
    ;Convert the values to 32 bit, makes it quicker to add the address to all
    cmu.cpiv v2.0, i18
    cmu.cpvv.i16.i32 v0, v23 || vau.swzword v1, v23 [1032]
    cmu.cpvv.i16.i32 v1, v1 || vau.swzword v2, v2, [0000] 
    ;multiply by 2 since we're dealing with 16 bit values
    vau.shl.u32 v0, v0, 1
    vau.shl.u32 v1, v1, 1
    vau.add.i32 v0, v0, v2
    vau.add.i32 v1, v1, v2
    cmu.cpvi i0, v0.0
    lsu0.ld16 i0, i0  || cmu.cpvi i1, v0.1 
    lsu0.ld16 i1, i1  || cmu.cpvi i2, v0.2 || iau.xor i0, i0, i0
    lsu0.ld16 i2, i2  || cmu.cpvi i3, v0.3 || iau.xor i1, i1, i1
    lsu0.ld16 i3, i3  || cmu.cpvi i4, v1.0 || iau.xor i2, i2, i2
    lsu0.ld16 i4, i4  || cmu.cpvi i5, v1.1 || iau.xor i3, i3, i3
    lsu0.ld16 i5, i5  || cmu.cpvi i6, v1.2 || iau.xor i4, i4, i4
    lsu0.ld16 i6, i6  || cmu.cpvi i7, v1.3 || vau.xor v23, v23, v23 || iau.xor i5, i5, i5
    lsu0.ld16 i7, i7  || cmu.cpiv v23.0, i0 || iau.xor i6, i6, i6
    cmu.cpiv v23.1, i1 || iau.xor i7, i7, i7
    cmu.cpiv v23.2, i2
    cmu.cpiv v23.3, i3 || vau.xor v1, v1, v1
    cmu.cpiv v1.0, i4
    cmu.cpiv v1.1, i5 
    cmu.cpiv v1.2, i6 || bru.jmp i30
    cmu.cpiv v1.3, i7
    cmu.cpvv.i32.i16s v23, v23
    cmu.cpvv.i32.i16s v1, v1
    vau.swzword v1, v1, [1032]
    peu.pven4word 0xC || vau.swzword v23, v1, [3210]

.code .text.svuGet8BitVals16BitLUT
;uchar8 svuGet8BitVals16BitLUT(ushort8 in_values(v23), u8* lut_memory(i18));
;uchar8 svuGet8BitVals16BitLUT(ushort8 in_values(v23), u16* lut_memory(i18));
svuGet8BitVals16BitLUT:
    ;Convert the values to 32 bit, makes it quicker to add the address to all
    cmu.cpiv v2.0, i18
    cmu.cpvv.i16.i32 v0, v23 || vau.swzword v1, v23 [1032]
    cmu.cpvv.i16.i32 v1, v1 || vau.swzword v2, v2, [0000] 
    vau.add.i32 v0, v0, v2
    vau.add.i32 v1, v1, v2
    cmu.cpvi i0, v0.0
    lsu0.ld8 i0, i0  || cmu.cpvi i1, v0.1 
    lsu0.ld8 i1, i1  || cmu.cpvi i2, v0.2  || iau.xor i0, i0, i0
    lsu0.ld8 i2, i2  || cmu.cpvi i3, v0.3  || iau.xor i1, i1, i1
    lsu0.ld8 i3, i3  || cmu.cpvi i4, v1.0  || iau.xor i2, i2, i2
    lsu0.ld8 i4, i4  || cmu.cpvi i5, v1.1  || iau.xor i3, i3, i3
    lsu0.ld8 i5, i5  || cmu.cpvi i6, v1.2  || iau.xor i4, i4, i4
    lsu0.ld8 i6, i6  || cmu.cpvi i7, v1.3  || vau.xor v0, v0, v0 || iau.xor i5, i5, i5
    lsu0.ld8 i7, i7  || cmu.cpiv v0.0, i0 || iau.xor i6, i6, i6
    cmu.cpiv v0.1, i1 || iau.xor i7, i7, i7
    cmu.cpiv v0.2, i2
    cmu.cpiv v0.3, i3 || vau.xor v1, v1, v1
    cmu.cpiv v1.0, i4
    cmu.cpiv v1.1, i5 
    cmu.cpiv v1.2, i6
    cmu.cpiv v1.3, i7
    cmu.cpvv.u32.u8s v0, v0 || bru.jmp i30
    cmu.cpvv.u32.u8s v1, v1
    vau.swzword v1, v1, [3201]
    peu.pven4word 0x2 || vau.swzword v0, v1, [3210]
    cmu.vszmword v23, v0, [ZZ10]
    ;Space the result
    cmu.cpvv.u8.u16 v23, v23
    
.code .text.svuGet8BitVals8BitLUT
;uchar8 svuGet8BitVals8BitLUT(uchar8 in_values(v23), u8* lut_memory(i18));
svuGet8BitVals8BitLUT:
    ;Convert the values to 32 bit, makes it quicker to add the address to all
    cmu.cpiv v2.0, i18
    cmu.cpvv.i16.i32 v0, v23 || vau.swzword v1, v23 [1032]
    cmu.cpvv.i16.i32 v1, v1 || vau.swzword v2, v2, [0000] 
    vau.add.i32 v0, v0, v2
    vau.add.i32 v1, v1, v2
    cmu.cpvi i0, v0.0
    lsu0.ld8 i0, i0  || cmu.cpvi i1, v0.1 
    lsu0.ld8 i1, i1  || cmu.cpvi i2, v0.2  || iau.xor i0, i0, i0
    lsu0.ld8 i2, i2  || cmu.cpvi i3, v0.3  || iau.xor i1, i1, i1
    lsu0.ld8 i3, i3  || cmu.cpvi i4, v1.0  || iau.xor i2, i2, i2
    lsu0.ld8 i4, i4  || cmu.cpvi i5, v1.1  || iau.xor i3, i3, i3
    lsu0.ld8 i5, i5  || cmu.cpvi i6, v1.2  || iau.xor i4, i4, i4
    lsu0.ld8 i6, i6  || cmu.cpvi i7, v1.3  || vau.xor v0, v0, v0 || iau.xor i5, i5, i5
    lsu0.ld8 i7, i7  || cmu.cpiv v0.0, i0 || iau.xor i6, i6, i6
    cmu.cpiv v0.1, i1 || iau.xor i7, i7, i7
    cmu.cpiv v0.2, i2
    cmu.cpiv v0.3, i3 || vau.xor v1, v1, v1
    cmu.cpiv v1.0, i4
    cmu.cpiv v1.1, i5 
    cmu.cpiv v1.2, i6
    cmu.cpiv v1.3, i7
    cmu.cpvv.u32.u8s v0, v0 || bru.jmp i30
    cmu.cpvv.u32.u8s v1, v1
    vau.swzword v1, v1, [3201]
    peu.pven4word 0x2 || vau.swzword v0, v1, [3210]
    cmu.vszmword v23, v0, [ZZ10]
    ;Space the result
    cmu.cpvv.u8.u16 v23, v23
    
.code .text.svuGet16_8BitVals8BitLUT
;uchar16 svuGet16_8BitVals8BitLUT(uchar16 in_values(v23), u8* lut_memory(i18));
svuGet16_8BitVals8BitLUT:
    ;Convert the values to 32 bit, makes it quicker to add the address to all
    cmu.cpiv v4.0, i18
    vau.swzword v4, v4, [0000] 
    cmu.cpvv.u8.u16 v1, v23 || vau.swzword v3, v23, [1032]
    cmu.cpvv.u8.u16 v3, v3
    cmu.cpvv.i16.i32 v0, v1 || vau.swzword v1, v1, [1032]
    cmu.cpvv.i16.i32 v1, v1
    cmu.cpvv.i16.i32 v2, v3 || vau.swzword v3, v3, [1032]
    cmu.cpvv.i16.i32 v3, v3 || vau.add.i32 v0, v4, v0
    vau.add.i32 v1, v4, v1
    vau.add.i32 v2, v4, v2
    vau.add.i32 v3, v4, v3 || cmu.cpvi i0, v0.0
    lsu0.ld8 i0, i0    || cmu.cpvi i1,  v0.1
    lsu0.ld8 i1, i1    || cmu.cpvi i2,  v0.2 || iau.xor i0,   i0, i0
    lsu0.ld8 i2, i2    || cmu.cpvi i3,  v0.3 || iau.xor i1,   i1, i1
    lsu0.ld8 i3, i3    || cmu.cpvi i4,  v1.0 || iau.xor i2,   i2, i2
    lsu0.ld8 i4, i4    || cmu.cpvi i5,  v1.1 || iau.xor i3,   i3, i3
    lsu0.ld8 i5, i5    || cmu.cpvi i6,  v1.2 || iau.xor i4,   i4, i4
    lsu0.ld8 i6, i6    || cmu.cpvi i7,  v1.3 || iau.xor i5,   i5, i5
    lsu0.ld8 i7, i7    || cmu.cpvi i8,  v2.0 || iau.xor i6,   i6, i6
    lsu0.ld8 i8, i8    || cmu.cpvi i9,  v2.1 || iau.xor i7,   i7, i7
    lsu0.ld8 i9, i9    || cmu.cpvi i10, v2.2 || iau.xor i8,   i8, i8
    lsu0.ld8 i10, i10  || cmu.cpvi i11, v2.3 || iau.xor i9,   i9, i9
    lsu0.ld8 i11, i11  || cmu.cpvi i12, v3.0 || iau.xor i10, i10, i10
    lsu0.ld8 i12, i12  || cmu.cpvi i13, v3.1 || iau.xor i11, i11, i11
    lsu0.ld8 i13, i13  || cmu.cpvi i14, v3.2 || iau.xor i12, i12, i12
    lsu0.ld8 i14, i14  || cmu.cpvi i15, v3.3 || iau.xor i13, i13, i13 || vau.xor v23, v23, v23
    lsu0.ld8 i15, i15  || iau.xor i14, i14, i14 || cmu.cpiv v23.0, i0
    iau.xor i15, i15, i15 || cmu.cpiv v23.1, i1
    cmu.cpiv v23.2, i2
    cmu.cpiv v23.3, i3 || vau.xor v1, v1, v1
    cmu.cpiv v1.0, i4
    cmu.cpiv v1.1, i5
    cmu.cpiv v1.2, i6
    cmu.cpiv v1.3, i7 || vau.xor v2, v2, v2
    cmu.cpiv v2.0, i8
    cmu.cpiv v2.1, i9
    cmu.cpiv v2.2, i10
    cmu.cpiv v2.3, i11 || vau.xor v3, v3, v3
    cmu.cpiv v3.0, i12
    cmu.cpiv v3.1, i13
    cmu.cpiv v3.2, i14
    cmu.cpiv v3.3, i15
    ;Compress result
    cmu.cpvv.u32.u8s v23, v23
    cmu.cpvv.u32.u8s v1, v1 || bru.jmp i30
    cmu.cpvv.u32.u8s v2, v2
    cmu.cpvv.u32.u8s v3, v3
    peu.pven4word 0x2 || lsu0.swz4v [3210] [3201] [PPPP] || vau.swzword v23, v1, [3210]
    peu.pven4word 0x4 || lsu0.swz4v [3210] [3021] [PPPP] || vau.swzword v23, v2, [3210]
    peu.pven4word 0x8 || lsu0.swz4v [3210] [0321] [PPPP] || vau.swzword v23, v3, [3210]
   
.end
