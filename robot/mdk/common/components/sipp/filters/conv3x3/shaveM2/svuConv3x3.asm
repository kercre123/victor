;-------------------------------------------------------------------------------
.version 00.51.05
; alu, 27 aug 2013 
; note: u8 conv I/O, using a 3x3 fp16 matrix
;------------------------------------------------------------------------------- 

.include "myriad2SippDefs.inc"

.code svuConv3x3

.lalign 
svuConv3x3:

 iau.add    i8, i18, F_O_PARAMS  || lsu1.ldil i10,  1 
 iau.and   i10, i16, i10         || lsu1.ldil  i6,  0
 iau.shl   i10, i10,   2         || lsu0.ld.32 i8, i8
 iau.add   i10, i18, i10
 iau.add   i11, i10, F_O_LINES_I
 iau.add    i9, i10, F_O_LINES_O || lsu0.ld.32 i12, i11
 iau.add   i14, i18, F_O_SLICE_W || lsu0.ld.32 i15,  i9
                                    lsu0.ld.32  i7, i14
 nop
 lsu0.ldo.16  i4,  i8, 0x00
 lsu0.ldo.16  i4,  i8, 0x02
 lsu0.ldo.16  i4,  i8, 0x04
 lsu0.ldo.32 i12, i12, 0x00
 lsu0.ldo.32 i13, i12, 0x04
 lsu0.ldo.32 i14, i12, 0x08
 lsu0.ldo.16  i4,  i8, 0x06 
 lsu0.ldo.16  i4,  i8, 0x08 || cmu.cpivr.x16 v15, i4
 lsu0.ldo.16  i4,  i8, 0x0A || cmu.cpivr.x16 v16, i4
 lsu0.ldo.16  i4,  i8, 0x0C || cmu.cpivr.x16 v17, i4
 lsu0.ldo.16  i4,  i8, 0x0E  
 lsu0.ldo.16  i4,  i8, 0x10  

;Initial loads
                          lsu0.ld.128.U8.f16 v0, i12 || iau.incs i12, -8 ;Load Current
 cmu.cpivr.x16 v18, i4 || lsu0.ld.128.U8.f16 v1, i13 || iau.incs i13, -8
 cmu.cpivr.x16 v19, i4 || lsu0.ld.128.U8.f16 v2, i14 || iau.incs i14, -8
 cmu.cpivr.x16 v20, i4 || lsu0.ld.128.U8.f16 v3, i12 || iau.incs i12, 16 ;Load Prev
 cmu.cpivr.x16 v21, i4 || lsu0.ld.128.U8.f16 v4, i13 || iau.incs i13, 16
 cmu.cpivr.x16 v22, i4 || lsu0.ld.128.U8.f16 v5, i14 || iau.incs i14, 16
 cmu.cpivr.x16 v23, i4 || lsu0.ld.128.U8.f16 v6, i12 || iau.incs i12,  8
                          lsu0.ld.128.U8.f16 v7, i13 || iau.incs i13,  8
                          lsu0.ld.128.U8.f16 v8, i14 || iau.incs i14,  8
 nop 3
 
;##########################################
.lalign
__svuConvLoop:
;##########################################
 
 ;-----------------------------
 ; |v9  v0 v10|   |v15 v16 v17|
 ; |v11 v1 v12| * |v18 v19 v20|
 ; |v13 v2 v14|   |v21 v22 v23|
 ;-----------------------------

 vau.mul.f16 v25,  v0, v16 || cmu.alignvec v9,  v3, v0, 14
 vau.mul.f16 v24,  v9, v15 || cmu.alignvec v10, v0, v6,  2
 vau.mul.f16 v26, v10, v17 || cmu.alignvec v11, v4, v1, 14
 vau.mul.f16 v27, v11, v18 || cmu.alignvec v12, v1, v7,  2
 vau.mul.f16 v28,  v1, v19 || cmu.alignvec v13, v5, v2, 14
 vau.mul.f16 v29, v12, v20 || cmu.alignvec v14, v2, v8,  2

;At first iter, i6 =0, thus store is useless, then i6 = 8 
 vau.mul.f16 v30, v13, v21 || lsu0.st.128.f16.u8 v31, i15 || iau.add i15, i15, i6 
 vau.mul.f16 v31,  v2, v22 || lsu1.ldil i6, 8
 vau.mul.f16 v14, v14, v23

;Then slide the regs to reduce LOADS, load next data in advance
;Store back final result 
 vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0    || lsu0.ld.128.U8.f16  v6, i12 
 vau.add.f16 v26, v26, v27 || cmu.cpvv v4, v1    || lsu0.ld.128.U8.f16  v7, i13 
 vau.add.f16 v28, v28, v29 || cmu.cpvv v5, v2    || lsu0.ld.128.U8.f16  v8, i14 
 vau.add.f16 v24, v24, v14 || cmu.cpvv v0, v6    || iau.incs i7, -8
 vau.add.f16 v30, v30, v31 || cmu.cpvv v1, v7    || peu.pcix.neq, 0x00 || bru.bra __svuConvLoop 
                              cmu.cpvv v2, v8
 vau.add.f16 v24, v24, v26
 vau.add.f16 v28, v28, v30 ||  iau.incs i12, 8
                               iau.incs i13, 8
                               iau.incs i14, 8
 vau.add.f16 v31, v24, v28
 

;########################################
;The return:
;########################################
 bru.jmp i30
 nop
 lsu0.st.128.f16.u8 v31, i15 ;final store
 nop 4
