;-------------------------------------------------------------------------------
.version 00.51.05
; alu, 31 aug 2013 
; note: u8 I/O, blur matrix: 1 2 1 
;                            2 4 2
;                            1 2 1 
;------------------------------------------------------------------------------- 

.include "myriad2SippDefs.inc"

.code svuLumaBlur

;##########################################
.lalign 
svuLumaBlur:
;##########################################
                            lsu1.ldil i10,  1 
 iau.and   i10, i16, i10 || lsu1.ldil  i6,  0
 iau.shl   i10, i10,   2
 iau.add   i10, i18, i10
 iau.add   i11, i10, F_O_LINES_I
 iau.add    i9, i10, F_O_LINES_O || lsu0.ld.32 i12, i11
 iau.add   i14, i18, F_O_SLICE_W || lsu0.ld.32 i15,  i9
                                    lsu0.ld.32  i7, i14
 nop 4
 lsu0.ldo.32 i12, i12, 0x00
 lsu0.ldo.32 i13, i12, 0x04
 lsu0.ldo.32 i14, i12, 0x08
 nop 4
 

;Initial loads
 lsu0.ld.128.u8.u16 v0, i12 || iau.incs i12, -8 ;Load Current
 lsu0.ld.128.u8.u16 v1, i13 || iau.incs i13, -8
 lsu0.ld.128.u8.u16 v2, i14 || iau.incs i14, -8
 lsu0.ld.128.u8.u16 v3, i12 || iau.incs i12, 16 ;Load Prev
 lsu0.ld.128.u8.u16 v4, i13 || iau.incs i13, 16
 lsu0.ld.128.u8.u16 v5, i14 || iau.incs i14, 16
 lsu0.ld.128.u8.u16 v6, i12 || iau.incs i12,  8
 lsu0.ld.128.u8.u16 v7, i13 || iau.incs i13,  8
 lsu0.ld.128.u8.u16 v8, i14 || iau.incs i14,  8
 nop 3

 cmu.alignvec v9,  v3, v0, 14
 cmu.alignvec v10, v0, v6,  2
 cmu.alignvec v11, v4, v1, 14
 cmu.alignvec v12, v1, v7,  2
 cmu.alignvec v13, v5, v2, 14
 cmu.alignvec v14, v2, v8,  2

;##########################################
.lalign
__svuLumaBlurLoop:
;##########################################
 
  vau.shl.x16 v19,  v1,   2  || cmu.cpvv v3, v0 || lsu0.ld.128.u8.u16  v6, i12
  vau.add.i16 v17,  v0,  v2  || cmu.cpvv v4, v1 || lsu0.ld.128.u8.u16  v7, i13
  vau.add.i16 v18, v11, v12  || cmu.cpvv v5, v2 || lsu0.ld.128.u8.u16  v8, i14
  vau.add.i16 v15,  v9, v10  || cmu.cpvv v0, v6 || lsu0.st.128.u16.u8 v20, i15 || iau.add i15, i15, i6
  vau.add.i16 v16, v13, v14  || cmu.cpvv v1, v7 || lsu1.ldil i6, 8
  vau.add.i16 v17, v17, v18  || cmu.cpvv v2, v8              || iau.incs i7, -8  
  vau.add.i16 v15, v15, v16  || cmu.alignvec v9,  v3, v0, 14 || peu.pcix.neq, 0x00 || bru.bra __svuLumaBlurLoop
  vau.shl.x16 v17, v17,   1  || cmu.alignvec v10, v0, v6,  2 
  vau.add.i16 v20, v19, v15  || cmu.alignvec v11, v4, v1, 14 
                                cmu.alignvec v12, v1, v7,  2 || iau.incs i12, 8	     
  vau.add.i16 v20, v20, v17  || cmu.alignvec v13, v5, v2, 14 || iau.incs i13, 8
                                cmu.alignvec v14, v2, v8,  2 || iau.incs i14, 8
  vau.shr.u16 v20, v20, 4


;########################################
;The return:
;########################################
 bru.jmp i30
 nop
 lsu0.st.128.u16.u8 v20, i15 ;final store
 nop 6
