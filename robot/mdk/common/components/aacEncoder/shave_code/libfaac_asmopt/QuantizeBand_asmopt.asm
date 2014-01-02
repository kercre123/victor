.version 0.51.05

.set MAGIC_FLOAT 0x4B000000
.set MAGIC_INT   0x4B000000 ;same thing...

.code .text.asmopt_QuantizeBand

;###############################
asmopt_QuantizeBand:
;###############################
;Arguments:
; i18: const float *xp
; i17: int         *pi
; i16: int          offset
; i15: int         end;
; s23: float        istep;not used for now, as it's 1.0f

 iau.shl i13, i16, 2
 iau.add i13, i18, i13
 
 ;@@@@@@@@@@@@@@@@@@@@@@
 asmopt_QBand_loop:
 ;@@@@@@@@@@@@@@@@@@@@@@
  lsu0.ldo32  s22, i13,   0 || lsu1.ldil i0, MAGIC_FLOAT
  lsu0.ldo32  s21, i13,   4 || lsu1.ldih i0, MAGIC_FLOAT
  lsu0.ldo32  s20, i13,   8 || cmu.cpis  s18, i0
  lsu0.ldo32  s19, i13,  12 || lsu1.ldil i4, MAGIC_INT
                               lsu1.ldih i4, MAGIC_INT
                               lsu1.ldil i5, lookup_adj43
  sau.add.f32 s22, s22, s18 || lsu1.ldih i5, lookup_adj43
  sau.add.f32 s21, s21, s18
  sau.add.f32 s20, s20, s18
  sau.add.f32 s19, s19, s18 || cmu.cpsi i3, s22
  
  cmu.cpsi i2, s21  || iau.sub i3, i3, i4
  cmu.cpsi i1, s20  || iau.sub i2, i2, i4 || sau.shl.u32 i3, i3, 2
  cmu.cpsi i0, s19  || iau.sub i1, i1, i4 || sau.shl.u32 i2, i2, 2
                       iau.sub i0, i0, i4 || sau.shl.u32 i1, i1, 2
                       iau.add i3, i3, i5 || sau.shl.u32 i0, i0, 2
  lsu0.ld32 s18, i3 || iau.add i2, i2, i5
  lsu0.ld32 s17, i2 || iau.add i1, i1, i5
  lsu0.ld32 s16, i1 || iau.add i0, i0, i5
  lsu0.ld32 s15, i0
  
  iau.shl i6, i16,  2
  iau.add i6, i17, i6
  
  sau.add.f32 s18, s18, s22 || iau.incs i16, 4
  sau.add.f32 s17, s17, s21 || iau.shl  i13, i16, 2 || cmu.cmii.u32 i16, i15 
  sau.add.f32 s16, s16, s20 || lsu1.ldil i0, asmopt_QBand_loop || iau.add i13, i18, i13
  sau.add.f32 s15, s15, s19 || lsu1.ldih i0, asmopt_QBand_loop || cmu.cpsi i3, s18
  
  peu.pc1c LT || bru.jmp i0
  
  cmu.cpsi i2, s17 || iau.sub i3, i3, i4
  cmu.cpsi i1, s16 || iau.sub i2, i2, i4 || lsu0.sto32 i3, i6, 0
  cmu.cpsi i0, s15 || iau.sub i1, i1, i4 || lsu0.sto32 i2, i6, 4
                      iau.sub i0, i0, i4 || lsu0.sto32 i1, i6, 8
                                            lsu0.sto32 i0, i6, 12
  
;###############################
asmopt_QuantizeBand_FIN:
;###############################
 bru.jmp i30 ;FINISH
    nop
    nop
    nop
    nop
    nop   
