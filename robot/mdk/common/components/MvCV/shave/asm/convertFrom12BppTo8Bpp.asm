.version 00.50.00

;/******************************************************************************
; @File         : convertFrom12BppTo8Bpp.asm
; @Author       : xxx xxx
; @Brief        : 
; Date          : 25 - 01 - 2013
; E-mail        : xxx.xxx@movidius.com
; Copyright     : C Movidius Srl 2012, C Movidius Ltd 2012
;
;HISTORY
; Version        | Date       | Owner         | Purpose
; ---------------------------------------------------------------------------
; 1.0            | 25.01.2011 | xxx xxx    | First implementation
;  ***************************************************************************

;FUNCTIONS DECLARATION
; ---------------------------------------------------------------------------
;void convert12BppTo8Bpp(u8* dst, u8* src, u32 width);
;  ***************************************************************************

;// convert from 12 bpp to 8 bpp
;void convert12BppTo8Bpp(u8* dst, u8* src, u32 width)
;{
;  u32 i;
;  for(i = 0; i < (width); i++)
;  {
;    dst[i] = (src[i<<1]>>4) | (src[(i << 1) + 1]<<4);
;  }
;}


.code .text.convert12BppTo8Bpp
.lalign
convert12BppTo8Bpp:
LSU0.LDI64.l v0, i17 ||IAU.SHR.i32 i16, i16, 0x02
LSU0.LDI64.l v0, i17
LSU0.LDI64.l v0, i17
LSU0.LDI64.l v0, i17
LSU0.LDI64.l v0, i17
LSU0.LDI64.l v0, i17
LSU0.LDI64.l v0, i17 ||VAU.SHR.U16 v1, v0, 4
LSU0.LDI64.l v0, i17 ||VAU.SHR.U16 v1, v0, 4
LSU0.LDI64.l v0, i17 ||VAU.SHR.U16 v1, v0, 4 ||CMU.VDILV.x8 v3, v2, v1, v1
;start loop
bru.rpi i16 || LSU0.LDI64.l v0, i17 ||VAU.SHR.U16 v1, v0, 4 ||CMU.VDILV.x8 v3, v2, v1, v1 ||LSU1.ST64.l v2, i18 || iau.incs i18, 0x04
;end loop
bru.jmp i30
nop 5





