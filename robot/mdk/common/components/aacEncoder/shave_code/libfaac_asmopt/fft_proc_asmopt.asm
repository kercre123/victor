.version 0.51.05

.code .text.hand_fft_proc

;###############################
hand_fft_proc:
;###############################
 cmu.cpii i5, i14 || lsu0.ldil i1, 1
 
 ;=======================
 fft_proc_L1:
 ;=======================
   iau.shr.i32 i5, i5, 1 || lsu0.ldil i7, 0 || lsu1.ldil i3, 0
   
 ;=======================
 fft_proc_L2:
 ;=======================
   cmu.cpii i6, i7 || iau.add i7, i7, i1 || lsu0.ldil i4, 0 || lsu1.ldil i2, 0  
   
 ;=======================
 fft_proc_L3:
 ;=======================
   iau.shl  i8, i4,   2 || sau.shl.u32 i10, i7,  2
   iau.add i11, i8, i16 || sau.shl.u32  i9, i6,  2
   iau.add i12, i8, i15 || sau.add.i32  i0, i18, i10 || lsu0.ld32 s2, i11
   
   sau.add.i32 i10, i17, i10 || lsu0.ld32 s3, i12
   sau.add.i32 i13, i18,  i9 || lsu0.ld32 s5,  i0
   sau.add.i32  i9, i17,  i9 || lsu0.ld32 s7, i10
                                lsu0.ld32 s4, i13
                                lsu0.ld32 s6,  i9
   nop
   nop
   sau.mul.f32  s8, s5, s2 || iau.add  i4, i4, i5
   sau.mul.f32  s9, s7, s3 || iau.incs i6,  1
   sau.mul.f32 s10, s5, s3 || iau.incs i7,  1
   sau.mul.f32 s11, s7, s2 || iau.incs i2,  1
   sau.sub.f32  s0, s8, s9
   nop
   sau.add.f32 s1, s10, s11
   sau.sub.f32 s5,  s4,  s0 || cmu.cmii.u32 i2, i1
   sau.add.f32 s4,  s4,  s0 || lsu0.ldil i8, fft_proc_L3 || lsu1.ldih i8, fft_proc_L3
   peu.pc1c LT || bru.jmp i8
                          sau.sub.f32 s7, s6, s1
     lsu0.st32 s5,  i0 || sau.add.f32 s6, s6, s1
     lsu0.st32 s4, i13 || iau.shl i0, i1, 1
     lsu0.st32 s7, i10 || iau.add i0, i3, i0   || lsu1.ldil i8, fft_proc_L2
     lsu0.st32 s6,  i9 || cmu.cmii.u32 i0, i14 || lsu1.ldih i8, fft_proc_L2
   
   
 ;=======================  
  fft_proc_L2_next:
 ;=======================
  peu.pc1c LT          || bru.jmp i8 || cmu.cpii i3, i0
    nop
    nop
    nop
    iau.shl      i0, i1, 1
    cmu.cmii.u32 i0, i14 || lsu0.ldil i8, fft_proc_L1 || lsu1.ldih i8, fft_proc_L1
  
 ;=======================  
  fft_proc_L1_next:
 ;=======================
   peu.pc1c LT          || bru.jmp i8 || cmu.cpii i1, i0
   peu.pc1c GTE         || bru.jmp i30 ;FINISH
    nop
    nop
    nop
    nop
    nop   
