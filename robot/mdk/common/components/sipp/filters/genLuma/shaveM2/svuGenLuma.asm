;-------------------------------------------------------------------------------
.version 00.51.05
; alu, 30 aug 2013 
; note: u8 I/O 
;  Luma[i] = (inR[i]*1 + inG[i]*2 + inB[i]*1) >> 2; 
;------------------------------------------------------------------------------- 

.include "myriad2SippDefs.inc"

.code svuGenLuma

;########################################
.lalign 
svuGenLuma:
;########################################

IAU.ADD    i0, i18, F_O_SLICE_W || LSU1.LDIL i17, 0x0001
IAU.AND   i10, i16, i17         || LSU0.LD.32  i0, i0 
IAU.SHL   i10, i10, 2
IAU.ADD    i8, i18, F_O_PARENTS
IAU.ADD    i9, i18, i10         || LSU0.LD.32  i8,  i8
IAU.ADD   i16,  i9, F_O_LINES_I
                                   LSU0.LD.32  i9, i16
IAU.ADD   i10,  i9, F_O_LINES_O
                                   LSU0.LD.32 i10, i10
nop 2
IAU.ADD    i8, i8, F_O_PL_STRIDE
                                   LSU0.LD.32  i7, i8
                                   LSU0.LD.32  i9, i9
nop
nop
nop
nop
nop
IAU.SHL   i17, i7,   1
IAU.ADD   i8,  i9,  i7 || lsu0.ld.128.u8.u16 v0, i9 
IAU.ADD   i7,  i9, i17 || lsu0.ld.128.u8.u16 v1, i8 
   iau.incs i9, 8      || lsu0.ld.128.u8.u16 v2, i7 
   iau.incs i8, 8 ;Gre 
   iau.incs i7, 8 ;Blu
   nop 4

;########################################
.lalign
___svuGenLumaLoop:
;########################################

vau.add.i16 v3, v0, v2     || lsu0.ld.128.u8.u16 v0, i9 || iau.incs i0, -8
vau.add.i16 v4, v1, v1     || lsu0.ld.128.u8.u16 v2, i7 || peu.pcix.neq 0x00 || bru.bra ___svuGenLumaLoop
                              lsu0.ld.128.u8.u16 v1, i8 
vau.add.i16 v3, v3, v4     || iau.incs i9, 8 
                              iau.incs i8, 8 
vau.shr.u16 v3, v3,  2     || iau.incs i7, 8 
nop
lsu0.st.128.u16.u8 v3, i10 || iau.incs i10, 8 ;store Luma


;########################################
;The return:
;########################################
 bru.jmp i30
 nop 6
