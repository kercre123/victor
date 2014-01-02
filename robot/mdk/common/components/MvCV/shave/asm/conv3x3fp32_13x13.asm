; ///
; /// @file
; /// @copyright All code copyright Movidius Ltd 2013, all rights reserved.
; ///            For License Warranty see: common/license.txt
; ///
; /// @brief
; ///

.version 00.50.00

;void icvcalcixiy_32f_tuned13x13( 
;      const u8* src,             -- i18
;      float* dstx,               -- i17
;      float* dsty,               -- i16
;      const float* smooth_k      -- unused
; )

.code .text.MvCV_conv3x3fp32_13x13
.salign 16

.lalign
conv3x3fp32_13x13_asm:
    ; load in the 3 rows we require (un-aligned loads)
    ; src1
    lsu0.ld128.u8.u16 v10, i18   || iau.incs i18, 8  || lsu1.ldil i5, 3
    lsu0.ld128.u8.u16 v11, i18   || iau.incs i18, 5  || lsu1.ldil i6, 10
    ; src2
    lsu0.ld128.u8.u16 v12, i18   || iau.incs i18, 8  || cmu.cpivr.x16 v16, i5
    lsu0.ld128.u8.u16 v13, i18   || iau.incs i18, 5  || cmu.cpivr.x16 v17, i6
    ; src3
    lsu0.ld128.u8.u16 v14, i18   || iau.incs i18, 8 
    lsu0.ld128.u8.u16 v15, i18   || iau.incs i18, 5

    ; convervative
    nop 2

    ; b0[2] = src3*k0 + src2*k1 + src1*k0 
    vau.macpz.i16      v10, v16
    vau.macp.i16       v12, v17
    vau.macpw.i16  v0, v14, v16
    vau.macpz.i16      v11, v16
    vau.macp.i16       v13, v17
    vau.macpw.i16  v1, v15, v16
    nop 2
    ; b1[2]  = src3 - src1
    vau.sub.i16    v2, v14, v10
    vau.sub.i16    v3, v15, v11
    nop
    ; b0[0]  = b0[2] shifted left by two
    vau.alignvec   v4, v0, v1, 4    || cmu.vrot v5, v1, 4
    ; b1[0]  = b1[2] shifted left by two
    vau.alignvec   v6, v2, v3, 4    || cmu.vrot v7, v3, 4
    ; b1[1]  = b1[2] shifted left by one
    vau.alignvec   v8, v2, v3, 2    || cmu.vrot v9, v3, 2

    ; for dstx and dsty, first do calc, then convert to fp16, then scale by 1/32, then convert to fp32
    ; dstx = b0[0] - b0[2]
    vau.sub.i16    v4, v4, v0    || lsu0.ldil i0, 0.03125F16
    vau.sub.i16    v5, v5, v1    || cmu.cpivr.x16 v13, i0

    ; dsty = b1[2]*k0 + b1[1]*k1 +b1[0]*k0;
    vau.macpz.i16      v2, v16   || cmu.cpvv.i16.f16 v4, v4
    vau.macp.i16       v8, v17   || cmu.cpvv.i16.f16 v5, v5
    vau.macpw.i16  v0, v6, v16
    vau.macpz.i16      v3, v16
    vau.macp.i16       v9, v17
    vau.macpw.i16  v1, v7, v16
    NOP
    vau.mul.f16   v4, v4, v13   || cmu.cpvv.i16.f16 v0, v0
    vau.mul.f16   v5, v5, v13
    vau.mul.f16   v0, v0, v13
                                    cmu.cpvv.f16l.f32 v10 v4
                                    cmu.cpvv.f16h.f32 v11 v4 
                                    cmu.cpvv.i16.f16 v1, v1  || lsu0.st64.l v10,  i17  || iau.incs i17, 8
                                                                lsu0.st64.h v10,  i17  || iau.incs i17, 8  
                                    cmu.cpvv.f16l.f32 v12 v5 || lsu0.st64.l v11,  i17  || iau.incs i17, 8
    vau.mul.f16   v1, v1, v13    || cmu.cpvi.x32 i12, v12.2  || lsu0.st64.h v11,  i17  || iau.incs i17, 8
                                    cmu.cpvv.f16l.f32 v10 v0 || lsu0.st64.l v12,  i17  || iau.incs i17, 8
                                    cmu.cpvv.f16h.f32 v11 v0 || lsu0.st32   i12,  i17
                                    cmu.cpvv.f16l.f32 v12 v1 || lsu0.st64.l v10,  i16  || iau.incs i16, 8
    bru.jmp i30                  || cmu.cpvi.x32 i12, v12.2  || lsu0.st64.h v10,  i16  || iau.incs i16, 8
                                                                lsu0.st64.l v11,  i16  || iau.incs i16, 8
                                                                lsu0.st64.h v11,  i16  || iau.incs i16, 8
                                                                lsu0.st64.l v12,  i16  || iau.incs i16, 8
                                                                lsu0.st32  i12,  i16 
    nop
    nop
    nop 15

.end
; bru.jmp i30                                                            
; BRU.SWIH 0x1F

bru.rpl i8, i7                  ||cmu.vdilv.x16 v5,v4, v0, v1   || LSU0.LDI128.u8.u16 v0, i18||LSU1.LDO16 i3, i18, -2    
