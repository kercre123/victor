.version 0.51.05
.code .text.asmopt_reorder
;####################################################
asmopt_reorder:
;####################################################
; Arguments:
 ; i18 : short *r
 ; i17 : float *x
 ; i16 :  logm
 
   lsu0.ldil i15, 1 || lsu1.ldil i13, 0 || cmu.cpii i11, i18
   iau.shl i15, i15, i16
 
 ;@@@@@@@@@@@@@@@@@@@@@ 
 asmopt_reorder_loop:
 ;@@@@@@@@@@@@@@@@@@@@@
  lsu0.ld16 i12, i11    || lsu1.ldih   i12, 0    || sau.shl.u32 i10, i13, 2
  iau.add   i13, i13, 1 || cmu.cpii    i5,  i13
  iau.shl   i11, i13, 1 || sau.add.i32 i10, i17, i10
  iau.add   i11, i18, i11 
  lsu0.ld32  i8, i10    || lsu1.ldil  i6, asmopt_reorder_loop
                           lsu1.ldih  i6, asmopt_reorder_loop
  iau.shl    i9, i12, 2
  iau.add    i9, i17, i9
  lsu0.ld32  i7, i9    || iau.xor i0, i13, i15
  peu.pc1i NEQ         || bru.jmp i6
   nop
   nop
   cmu.cmii.u32 i12, i5
   peu.pc1c GT || lsu0.st32 i8,  i9
   peu.pc1c GT || lsu0.st32 i7, i10
  
 ;@@@@@@@@@@@@@@@@@@@@@
 asmopt_reorder_FIN:
 ;@@@@@@@@@@@@@@@@@@@@@
  bru.jmp i30
  nop
  nop
  nop
  nop
  nop
