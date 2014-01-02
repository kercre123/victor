.version 00.40.00

;==============================================================================
;__     ___    ___      ___  _ ___   ___         _                  
;\ \   / / |  | \ \    / / || |__ \ / _ \       (_)                 
; \ \_/ /| |  | |\ \  / /| || |_ ) | | | |______ _ _ __   ___  __ _ 
;  \   / | |  | | \ \/ / |__   _/ /| | | |______| | '_ \ / _ \/ _` |
;   | |  | |__| |  \  /     | |/ /_| |_| |      | | |_) |  __/ (_| |
;   |_|   \____/    \/      |_|____|\___/       | | .__/ \___|\__, |
;                                              _/ | |          __/ |
;                                             |__/|_|         |___/ 
; yuv420 specific block handling JPEG-encode code
;==============================================================================


;==============================================================================
; include main JPEG encode defs
;==============================================================================


;~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
;set window-registers:
.include JpegEncoderDefines.incl

;~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
.data TEST_DATA
.align 16

  dummy_fin:
   .int 0

.code .text.mainjpeg
mainjpeg:
;=====================================================
; Initial setup (these would normally be set in regs)
;=====================================================
; s31 = Y buffer addr
; s30 = U buffer addr
; s29 = V buffer addr
; i26 = Addr where encoded output is stored
; i31 = img width
; i30 = img height

 lsu0.ldil i0,  Y_M16zeros_code || lsu1.ldil i1,  Y_M16zeros_len  || sau.sum.f32 s22, v0, 0
 lsu0.ldil i2,  Y_EOB_code      || lsu1.ldil i3,  Y_EOB_len       || sau.sum.f32 s23, v0, 0 || cmu.cpis s0, i0
 lsu0.ldil i4,  C_M16zeros_code || lsu1.ldil i5,  C_M16zeros_len  || sau.sum.f32 s24, v0, 0 || cmu.cpis s1, i1
 lsu0.ldil i6,  C_EOB_code      || lsu1.ldil i7,  C_EOB_len       || cmu.cpis    s2, i2
 lsu0.ldil i8,  0               || lsu1.ldil i17, L_MB_start      || cmu.cpis    s3, i3
 lsu0.ldil i21, 0               || lsu1.ldil i18, L_BLK_enc_start || cmu.cpis    s4, i4     || iau.add i28, i31, i8
 lsu0.ldil i22, 0               || lsu1.ldih i18, L_BLK_enc_start || cmu.cpis    s5, i5     || iau.add i27, i30, i8
 lsu0.ldih i17, L_MB_start      ||                                   cmu.cpis    s6, i6
                                                                     cmu.cpis    s7, i7

;================
L_MB_start: 
;================

;=========================================================================================
 L_encode_Y_1:
;================
   cmu.cpss s26, s1   || bru.jmp i18
    cmu.cpss s25, s0  || lsu0.ldil i25, TBL_HUFF_YDC || lsu1.ldih i25, TBL_HUFF_YDC
    cmu.cpss s28, s3  || lsu0.ldil i24, TBL_HUFF_YAC || lsu1.ldih i24, TBL_HUFF_YAC
    cmu.cpss s27, s2  || lsu0.ldil i23, TBL_QUANT_Y  || lsu1.ldih i23, TBL_QUANT_Y
    cmu.cpsi i29, s24 || lsu0.ldil i16, L_encode_Y_2 || iau.or    i2,  i31, i31
    cmu.cpsi i0,  s31 || lsu0.ldih i16, L_encode_Y_2
  
;=========================================================================================
 L_encode_Y_2:
;================
  bru.jmp  i18
   cmu.cpsi  i0,  s31
   iau.add   i0,  i0, 8
   lsu0.ldil i16, L_encode_Y_3 || lsu1.ldih i16, L_encode_Y_3
   iau.or    i2, i31, i31
   lsu0.ldil i23, TBL_QUANT_Y  || lsu1.ldih i23, TBL_QUANT_Y

;=========================================================================================
 L_encode_Y_3:
;================
  bru.jmp i18
   cmu.cpsi  i0,  s31
   iau.shl   i4,  i31, 3   
   iau.add   i0,  i0,  i4  
   iau.or    i2,  i31, i31 || lsu0.ldil i23, TBL_QUANT_Y  || lsu1.ldih i23, TBL_QUANT_Y
   lsu0.ldil i16, L_encode_Y_4 ||lsu1.ldih i16, L_encode_Y_4
   
;=========================================================================================
 L_encode_Y_4:
;================
  bru.jmp i18
   cmu.cpsi i0, s31
   iau.shl  i4, i31, 3
   iau.add  i4, i4,  8
   iau.add  i0, i0,  i4  || lsu0.ldil i23, TBL_QUANT_Y  || lsu1.ldih i23, TBL_QUANT_Y
   iau.or   i2, i31, i31 || lsu0.ldil i16, L_encode_U   || lsu1.ldih i16, L_encode_U
   
;=========================================================================================
 L_encode_U:
;================
 cmu.cpis s24, i29
 cmu.cpsi i29, s23  || bru.jmp i18
  cmu.cpss s26, s5  || lsu0.ldil i25, TBL_HUFF_CDC || lsu1.ldih i25, TBL_HUFF_CDC
  cmu.cpss s25, s4  || lsu0.ldil i24, TBL_HUFF_CAC || lsu1.ldih i24, TBL_HUFF_CAC
  cmu.cpsi i0,  s30 || lsu0.ldil i23, TBL_QUANT_C  || lsu1.ldih i23, TBL_QUANT_C
  cmu.cpss s28, s7  || iau.shr.u32 i2, i31, 1   
  cmu.cpss s27, s6  || lsu0.ldil i16, L_encode_V   || lsu1.ldih i16, L_encode_V
 
;=========================================================================================
 L_encode_V:
;================
 
 bru.jmp i18
  cmu.cpis s23, i29
  cmu.cpsi i29, s22
  cmu.cpsi i0,  s29 || lsu0.ldil i23, TBL_QUANT_C  || lsu1.ldih i23, TBL_QUANT_C
  lsu0.ldil i16, L_next_MB || lsu1.ldih i16, L_next_MB
  iau.shr.u32 i2, i31, 1
  
;==========================  
 L_next_MB:
;==========================
  iau.sub i28, i28, 16 || cmu.cpsi  i0,  s31     || sau.shr.u32 i12, i31, 1
  cmu.cmz.i32  i28     || lsu0.ldil i7,  16      || lsu1.ldil   i8,  8
  peu.pc1c EQ          || iau.mul   i10, i31, 15 || sau.mul.i32 i11, i12, 7
  nop ;Leave a nop, sau.mul is 2 c.c. latency !!! iau.mul = 1 c.c. latency
  peu.pc1c EQ          || iau.add   i7,  i7, i10 || sau.sub.i32 i27, i27, 16
  peu.pc1c EQ          || iau.add   i8,  i8, i11 || cmu.cpii    i28, i31
  
  iau.or i27, i27, i27 || cmu.cpis s22, i29
  peu.pc1i NEQ         || bru.jmp i17
   cmu.cpsi i1, s30    || iau.add i0, i0, i7
   cmu.cpsi i2, s29    || iau.add i1, i1, i8
   cmu.cpis s31, i0    || iau.add i2, i2, i8
   cmu.cpis s30, i1
   cmu.cpis s29, i2

;==========================
; Encode finish:
;==========================
bru.swih 0x1f
nop
nop
nop
nop
nop
nop
nop
nop
nop

;==============================================================================
; include main JPEG encode code
;==============================================================================
;.include jpeg_enc_core.asm


.end
