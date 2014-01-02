;-------------------------------------------------------------------------------
.version 00.51.05
; alu, 22 nov 2013 
;------------------------------------------------------------------------------- 

.include "myriad2SippDefs.inc"

.code svuStress00

.lalign 
;########################################
svuStress00:
;########################################
  
 ;clear regs so I don't get xxx-es 
  lsu0.ldil  i0, 0
  lsu0.ldil i14, 0
  cmu.cpivr.x32 v24, i0 
  cmu.cpivr.x32 v25, i0
  cmu.cpivr.x32  v0, i0
  cmu.cpivr.x32  v3, i0
 
  
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  vau.add.f16 v24, v24, v25 || cmu.cpvv v3, v0  || iau.incs i14, 16  || lsu0.ldil i3, 0 || lsu1.ldih i3, 4
  
;########################################
;The return:
;########################################
 bru.jmp i30
 nop 6

