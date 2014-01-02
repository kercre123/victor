; ///
; /// @file
; /// @copyright All code copyright Movidius Ltd 2012, all rights reserved.
; ///            For License Warranty see: common/license.txt
; ///
; /// @brief    
; ///

; Bilinear resize to challenge TMU supremacy
.version 0.50.00
.include biResize.h

.data resize_structure 0x1D003F00
ResizeData:
.fill 0x80
ResizeData1:
.int BufferV_Cr
.int (BufferV_Cr + MAX_WIDTH/2) 
.int BufferV_Cb
.int (BufferV_Cb + MAX_WIDTH/2) 
.int BufferV_Y
.int (BufferV_Y + MAX_WIDTH) 
.int 0x1 ; last position Luma input
.int 0x1 ; last position Chroma input

;.data resize_structure1 0x1c000000
;ResizeData2:
;.fill  0x8000 0x1 0x00


.code .text.resize_code 0x1D000000
;start:
Resize:
; read parameters, determine if init phase needed
                          LSU0.LDIL resize, ResizeData              || LSU1.LDIH resize, ResizeData
                          LSU0.LDO64.l v0, resize,  wi              || LSU1.LDIL i0,  BufferV
                          LSU0.LDO64.h v0, resize, (wi+0x08)        || LSU1.LDIH i0,  BufferV
                          LSU0.LDO64.l v1, resize,  shadow          || LSU1.LDIL i1, (BufferV+MAX_WIDTH)
                          LSU0.LDO64.h v1, resize, (shadow+0x08)    || LSU1.LDIH i1, (BufferV+MAX_WIDTH)
                          LSU0.LDO32 i_stride, resize, offsetX      || LSU1.LDIL i2,  IncrV
                          LSU0.LDO32 i_offset, resize, offsetY      || LSU1.LDIH i2,  IncrV
CMU.CPIV.x32 v30.0, i0 || LSU0.LDO32 i_inBuffer,  resize, inBuffer  || LSU1.LDIL i3,  ContribV
CMU.CPIV.x32 v30.1, i1 || LSU0.LDO32 i_outBuffer, resize, outBuffer || LSU1.LDIH i3,  ContribV
CMU.CPIV.x32 v30.2, i2 || LSU0.STO64.l v0, resize,  shadow          || LSU1.LDIL i0,  BufferH
CMU.CPIV.x32 v30.3, i3 || LSU0.STO64.h v0, resize, (shadow+0x08)    || LSU1.LDIH i0,  BufferH
                                                                       LSU1.LDIL i1,  BufferOut
CMU.CMVV.i32 v0, v1                                                 || LSU1.LDIH i1,  BufferOut
PEU.PC4C.OR 0x6 || BRU.BRA Resize_InitV
                          SAU.SUM.i32 i_wi, v0, 0x1                 || LSU1.LDIL i2,  IncrH
CMU.CPIV.x32 v31.0, i0 || SAU.SUM.i32 i_hi, v0, 0x2                 || LSU1.LDIH i2,  IncrH
CMU.CPIV.x32 v31.1, i1 || SAU.SUM.i32 i_wo, v0, 0x4                 || LSU1.LDIL i3,  ContribH
CMU.CPIV.x32 v31.2, i2 || SAU.SUM.i32 i_ho, v0, 0x8                 || LSU1.LDIH i3,  ContribH
CMU.CPIV.x32 v31.3, i3


; read increment
CMU.CPVI.x32 i0, v30.2  
LSU0.LD32.u8.u32 i_endp, i0
NOP 5
LSU0.LDIL i_posy 0xFFFF || LSU1.LDIH i_posy 0xFFFF
IAU.INCS i_endp, 0x01
LSU0.LDIL i14, ResizeData1              || LSU1.LDIH i14, ResizeData1 
LSU0.STO32 i_endp i14 0x18 || LSU1.STO32 i_endp i14 0x1c




ResizeLoop:

ResizeU:
LSU1.LDIH i13, BufferOut_Cr || LSU0.LDIL i13, BufferOut_Cr || IAU.INCS i_posy 1 
LSU0.LDIL i14, ResizeData1  || LSU1.LDIH i14, ResizeData1  || IAU.SHR.i32 i_posy i_posy 1
LSU0.LDO64.l v30 i14 0      || CMU.CPIV.x32 v31.1, i13
LSU0.LDO32 i_inBuffer,  resize, inBuffer || LSU1.LDO32 i_outBuffer, resize, outBuffer 
IAU.ADD i0, i_wi, i_stride || SAU.ADD.i32 i1,   i_hi, i_offset                       
IAU.MUL i2, i_wo, i_ho     || SAU.SHR.u32 i_wi, i_wi, 0x01     || BRU.BRA ResizeEntry
IAU.MUL i1, i1,   i0       || SAU.SHR.u32 i_hi, i_hi, 0x01
SAU.SHR.u32 i_stride, i_stride, 0x01   || LSU1.LDIL i30, ResizeV || LSU0.LDIH i30, ResizeV
SAU.SHR.u32 i_offset, i_offset, 0x01   
IAU.ADD i_inBuffer,  i_inBuffer,  i1   || SAU.SHR.u32 i_wo, i_wo, 0x01
SAU.SHR.u32 i_ho, i_ho, 0x01 || IAU.ADD i_outBuffer i_outBuffer i2


ResizeV:

LSU0.LDIL i14, ResizeData1              || LSU1.LDIH i14, ResizeData1
; restore i18 updated during Cr for Cb
|| IAU.SUB i_endp i_endp i18
LSU0.STO64.l v30 i14 0 || LSU1.LDO64.l v30 i14 8 ; load chroma addresses
IAU.ADD i0, i_wi, i_stride         || SAU.ADD.i32 i1, i_hi, i_offset || BRU.BRA ResizeEntry
IAU.MUL i2, i_wo, i_ho             || LSU1.LDIL i30, Resize_Y_0
IAU.MUL i1, i1,   i0               || LSU1.LDIH i30, Resize_Y_0
LSU1.LDIH i13, BufferOut_Cb        || LSU0.LDIL i13, BufferOut_Cb
IAU.ADD i_inBuffer, i_inBuffer, i1 || CMU.CPIV.x32 v31.1, i13 
IAU.ADD i_outBuffer, i_outBuffer, i2 



Resize_Y_0:
LSU0.LDIL i14, ResizeData1              || LSU1.LDIH i14, ResizeData1 || IAU.SHL i_posy i_posy 1
LSU0.LDO32 i_endp i14 0x18 || LSU1.STO32 i_endp i14 0x1c
LSU1.LDIH i13, BufferOut_Y || LSU0.LDIL i13, BufferOut_Y || IAU.SHL i_ho i_ho 0x01 || BRU.BRA ResizeEntry 
CMU.CPIV.x32 v31.1, i13    || IAU.SHL i_wo i_wo 0x01
IAU.SHL i_stride i_stride 1 
SAU.SHL.u32 i_hi i_hi , 0x01 || LSU0.LDO64.l v30 i14 0x10 || LSU1.STO64.l v30 i14 8 
LSU0.LDO32 i_inBuffer,  resize, inBuffer || LSU1.LDO32 i_outBuffer, resize, outBuffer
; restore width and hight
 || IAU.SHL i_wi i_wi 0x01
LSU1.LDIL i30, L_Interleave0 || LSU0.LDIH i30, L_Interleave0

L_Interleave0:

.if (YUV_FORMAT == YUV_422)
BRU.BRA Interleave422 
LSU0.LDIL i30 Resize_Y_1 
LSU1.LDIH i30 Resize_Y_1
NOP 3
.endif

Resize_Y_1:
BRU.BRA ResizeEntry || IAU.INCS i_posy, 0x01 
LSU1.LDIL i30, L_Interleave1
LSU1.LDIH i30, L_Interleave1
nop 3

L_Interleave1:

LSU0.LDIL i14, ResizeData1              || LSU1.LDIH i14, ResizeData1
LSU0.LDO32 i_endp i14 0x1C || LSU1.STO32 i_endp i14 0x18 
.if (YUV_FORMAT == YUV_422)
|| BRU.BRA Interleave422  
.endif
; first bring data to DDR previosly processed to DDR
IAU.SUB i13 i_ho 1 
LSU0.LDIL i30 ResizeU || LSU1.LDIH i30 ResizeU
CMU.CMII.i32 i_posy, i13 ; i_ho
|| LSU0.STO64.l v30 i14 0x10
PEU.PC1C GTE || LSU1.LDIL i30, exit        || LSU0.LDIH i30, exit
NOP
.if (YUV_FORMAT == YUV_420)
bru.jmp i30
nop 5
.endif

exit:
; wait for DMA writing task to finish
LSU1.LDIL i0, DMA_BUSY_MASK
CMU.CMTI.BITP i0, P_GPI
PEU.PC1C 0x6 || BRU.RPIM 0 || CMU.CMTI.BITP i0, P_GPI
BRU.SWIH 0x1F
NOP 5



ResizeEntry:
; Input data initialization call
CMU.CPIV.x32 v29.2, i30 || IAU.AND i_posy, i_posy, i_posy || LSU0.LDIL i30, DMA_Read_wait || LSU1.LDIH i30, DMA_Read_wait
PEU.PC1I EQ || BRU.SWP i30  ; call is made only for first line
; read increment
CMU.CPVI.x32 i0, v30.2         || LSU0.LDIL i18, 0x02
nop 4

; V pass

; copy contribution address
CMU.CPVI.x32 i0,  v30.3                             || LSU1.LDIL i30, BilinearV
; copy IncrV address
CMU.CPVI.x32 i18, v30.2 || IAU.ADD i0,  i0,  i_posy || LSU1.LDIH i30, BilinearV
BRU.SWP i30             || IAU.ADD i18, i18, i_posy || LSU0.LD32.u8.u32 i12, i0
CMU.CPVI.x32 i13, v30.0 || IAU.INCS i18, 0x01
CMU.CPVI.x32 i14, v30.1                             || LSU0.LD32.u8.u32 i18, i18
CMU.CPVI.x32 i15, v31.0 
CMU.CMII.i32 i_endp, i_hi 
PEU.PC1C 0x3 || CMU.CPII i13, i14

; read phase
LSU1.LDIL i30, DMA_Read
LSU1.LDIH i30, DMA_Read
BRU.SWP i30 || LSU1.LDIL i0, DMA_BUSY_MASK
IAU.CMPI i18, 0x01
PEU.PC1I 0x1 || CMU.VSZMWORD v30, v30, [3201]
IAU.ADD i_endp i_endp i18
CMU.CMTI.BITP i0, P_GPI
PEU.PC1C 0x6 || BRU.RPIM 0 || CMU.CMTI.BITP i0, P_GPI

; H pass
CMU.CPVI.x32 i12, v31.2   || LSU1.LDIL i30, BilinearH
|| LSU0.LDIH i30, BilinearH
BRU.JMP i30               || LSU0.LDI32 i10, i12
; input pointer
CMU.CPVI.x32 i13, v31.3   || LSU0.LDI32 i11, i12
CMU.CPVI.x32 i14, v31.0   
CMU.CPVI.x32 i15, v31.1   || IAU.ADD i16 i14 i_wi
CMU.CPVI.x32 i30, v29.2 ; copy address for jumping back
IAU.INCS i16 -1
nop



ResizeExit:
; Bilinear vertical filter pass
; i12     : top contribs
; i13..14 : input pointers
; i15     : output pointer
; i30     : return address
.lalign
BilinearV:
;takeoff
IAU.XOR i11, i11, i11        || LSU0.LDI128.u8.u16 v0, i13
IAU.INCS i11, 0x100          || LSU0.LDI128.u8.u16 v1, i14
CMU.VSZMBYTE i12, i12, [ZZZ0]
IAU.SUB.u32s i11, i11, i12   || LSU0.LDI128.u8.u16 v2, i13
CMU.CPIVR.x16 v8, i12        || LSU0.LDI128.u8.u16 v3, i14
CMU.CPIVR.x16 v9, i11
IAU.SHR.u32 i8, i_wi, 0x04   || LSU0.LDI128.u8.u16 v0, i13
VAU.MACPZ.u16     v0, v8     || LSU0.LDI128.u8.u16 v1, i14
VAU.MACPW.u16 v4, v1, v9
                                LSU0.LDI128.u8.u16 v2, i13
VAU.MACPZ.u16     v2, v8     || LSU0.LDI128.u8.u16 v3, i14
VAU.MACPW.u16 v5, v3, v9     || IAU.SUB.u32s i8, i8, 0x02
; warp
.lalign
BilinearV_Loop:
CMU.VSZMBYTE  v4, v4, [Z3Z1] || LSU0.LDI128.u8.u16 v0, i13 || BRU.BRA BilinearV_Loop || PEU.PCIX.NEQ 0
VAU.MACPZ.u16     v0, v8     || LSU0.LDI128.u8.u16 v1, i14
VAU.MACPW.u16 v4, v1, v9     || LSU0.STI128.u16.u8 v4, i15
CMU.VSZMBYTE  v5, v5, [Z3Z1] || LSU0.LDI128.u8.u16 v2, i13
VAU.MACPZ.u16     v2, v8     || LSU0.LDI128.u8.u16 v3, i14
VAU.MACPW.u16 v5, v3, v9     || LSU0.STI128.u16.u8 v5, i15 || IAU.SUB.u32s i8, i8, 0x01
; landing
CMU.VSZMBYTE  v4, v4, [Z3Z1]
VAU.MACPZ.u16     v0, v8     || LSU0.STI128.u16.u8 v4, i15
VAU.MACPW.u16 v4, v1, v9
CMU.VSZMBYTE  v5, v5, [Z3Z1]
VAU.MACPZ.u16     v2, v8     || LSU0.STI128.u16.u8 v5, i15
VAU.MACPW.u16 v5, v3, v9     || BRU.JMP i30
CMU.VSZMBYTE  v4, v4, [Z3Z1]
LSU0.STI128.u16.u8 v4, i15
nop
CMU.VSZMBYTE  v5, v5, [Z3Z1]
LSU0.STI128.u16.u8 v5, i15


; Bilinear horizontal filter pass
; i10..11 : increment
; i12     : increment pointer
; i13     : contribution pointers
; i14     : input pointer
; i15     : output pointer
; i16     : limit for input pointer
.lalign
BilinearH:
; takeoff
                               IAU.FEXT.u32 i9, i10, 0x00, 0x08
                               IAU.FEXT.u32 i9, i10, 0x08, 0x08 || LSU0.LDI16 i0,  i14, i9
                               IAU.FEXT.u32 i9, i10, 0x10, 0x08 || LSU0.LDI16 i1,  i14, i9
                               IAU.FEXT.u32 i9, i10, 0x18, 0x08 || LSU0.LDI16 i2,  i14, i9
                               IAU.SHR.u32  i8, i_wo, 0x03      || LSU0.LDI16 i3,  i14, i9
                               IAU.FEXT.u32 i9, i11, 0x00, 0x08 || LSU0.LDI64 i10, i12
VAU.SUB.u16s v10, v10, v10  || IAU.FEXT.u32 i9, i11, 0x08, 0x08 || LSU0.LDI16 i4,  i14, i9
                               IAU.FEXT.u32 i9, i11, 0x10, 0x08 || LSU0.LDI16 i5,  i14, i9
VAU.ADD.u16s v10, v10, 0x01 || IAU.FEXT.u32 i9, i11, 0x18, 0x08 || LSU0.LDI16 i6,  i14, i9
                                                                   LSU0.LDI16 i7,  i14, i9
VAU.SHL.u16  v10, v10, 0x08
                               IAU.FEXT.u32 i9, i10, 0x00, 0x08 || LSU0.LDI128.u8.u16 v8, i13
                               IAU.FEXT.u32 i9, i10, 0x08, 0x08 || LSU0.LDI16 i0,  i14, i9
CMU.CPIV.x16 v0.0, i0.l     || IAU.FEXT.u32 i9, i10, 0x10, 0x08 || LSU0.LDI16 i1,  i14, i9
CMU.CPIV.x16 v0.1, i1.l     || IAU.FEXT.u32 i9, i10, 0x18, 0x08 || LSU0.LDI16 i2,  i14, i9
CMU.CPIV.x16 v0.2, i2.l     || IAU.SUB.u32s i8, i8, 0x02        || LSU0.LDI16 i3,  i14, i9
                               IAU.FEXT.u32 i9, i11, 0x00, 0x08 || LSU0.LDI64 i10, i12
CMU.CPIV.x16 v0.3, i3.l     || IAU.FEXT.u32 i9, i11, 0x08, 0x08 || LSU0.LDI16 i4,  i14, i9
CMU.CPIV.x16 v0.4, i4.l     || IAU.FEXT.u32 i9, i11, 0x10, 0x08 || LSU0.LDI16 i5,  i14, i9
CMU.CPIV.x16 v0.5, i5.l     || IAU.FEXT.u32 i9, i11, 0x18, 0x08 || LSU0.LDI16 i6,  i14, i9
CMU.CPIV.x16 v0.6, i6.l                                         || LSU0.LDI16 i7,  i14, i9
CMU.CPIV.x16 v0.7, i7.l
.lalign
BilinearH_Loop:
; warp
CMU.VSZMBYTE v2, v0, [Z2Z0] || IAU.FEXT.u32 i9, i10, 0x00, 0x08 || LSU0.LDI128.u8.u16 v8, i13  || VAU.SUB.u16s v9, v10, v8
CMU.VSZMBYTE v3, v0, [Z3Z1] || IAU.FEXT.u32 i9, i10, 0x08, 0x08 || LSU0.LDI16 i0,  i14, i9
CMU.CPIV.x16 v0.0, i0.l     || IAU.FEXT.u32 i9, i10, 0x10, 0x08 || LSU0.LDI16 i1,  i14, i9     || VAU.MUL.u16s v2, v2, v8
CMU.CPIV.x16 v0.1, i1.l     || IAU.FEXT.u32 i9, i10, 0x18, 0x08 || LSU0.LDI16 i2,  i14, i9     || VAU.MUL.u16s v3, v3, v9
CMU.CPIV.x16 v0.2, i2.l     || IAU.SUB.u32s i8, i8, 0x01        || LSU0.LDI16 i3,  i14, i9
PEU.PCIX.NEQ 0              || IAU.FEXT.u32 i9, i11, 0x00, 0x08 || LSU0.LDI64 i10, i12         || BRU.BRA BilinearH_Loop
CMU.CPIV.x16 v0.3, i3.l     || IAU.FEXT.u32 i9, i11, 0x08, 0x08 || LSU0.LDI16 i4,  i14, i9     || VAU.ADD.u16s v2, v2, v3
CMU.CPIV.x16 v0.4, i4.l     || IAU.FEXT.u32 i9, i11, 0x10, 0x08 || LSU0.LDI16 i5,  i14, i9
CMU.CPIV.x16 v0.5, i5.l     || IAU.FEXT.u32 i9, i11, 0x18, 0x08 || LSU0.LDI16 i6,  i14, i9     || VAU.SHR.u16  v2, v2, 0x08
CMU.CPIV.x16 v0.6, i6.l                                         || LSU0.LDI16 i7,  i14, i9
CMU.CPIV.x16 v0.7, i7.l                                         || LSU0.STI128.u16.u8 v2, i15
; landing
CMU.VSZMBYTE v2, v0, [Z2Z0] || LSU0.LDI128.u8.u16 v8, i13 || VAU.SUB.u16s v9, v10, v8
CMU.VSZMBYTE v3, v0, [Z3Z1]
CMU.CPIV.x16 v0.0, i0.l     || VAU.MUL.u16s v2, v2, v8
CMU.CPIV.x16 v0.1, i1.l     || VAU.MUL.u16s v3, v3, v9
CMU.CPIV.x16 v0.2, i2.l
CMU.CMII.i32 i14 i16
CMU.CPIV.x16 v0.3, i3.l     || VAU.ADD.u16s v2, v2, v3
CMU.CPIV.x16 v0.4, i4.l
CMU.CPIV.x16 v0.5, i5.l     || VAU.SHR.u16  v2, v2, 0x08
CMU.CPIV.x16 v0.6, i6.l     || PEU.PCCX.EQ   0x01 || IAU.AND i7 i6 i6 ; restrict input pointer inside the vertical buffer
CMU.CPIV.x16 v0.7, i7.l     || LSU0.STI128.u16.u8 v2, i15
CMU.VSZMBYTE v2, v0, [Z2Z0] || VAU.SUB.u16s v9, v10, v8
CMU.VSZMBYTE v3, v0, [Z3Z1]
VAU.MUL.u16s v2, v2, v8
VAU.MUL.u16s v3, v3, v9
.if (YUV_FORMAT == YUV_422)
BRU.JMP i30
.else
BRU.BRA DMA_Write_420
.endif
nop
VAU.ADD.u16s v2, v2, v3
nop
CMU.VSZMBYTE v2, v2, [Z3Z1]
LSU0.STI128.u16.u8 v2, i15


; Initialize increments and contributions
; input as global var ;-)
.lalign
Resize_InitV:
; init vertical filter
CMU.CPIS.i32.f32 s2, i_hi
CMU.CPIS.i32.f32 s3, i_ho   || LSU1.LDIL i0, 0x04
CMU.CPIVR.x32 v8,  i0       || LSU1.LDIL i0, 0x100
CMU.CPIVR.x32 v9,  i0       || IAU.XOR  i0, i0, i0 || SAU.DIV.f32 s4, s2, s3
CMU.CPIV.x32 v3.0, i0       || IAU.INCS i0, 0x01
CMU.CPIV.x32 v3.1, i0       || IAU.INCS i0, 0x01
CMU.CPIV.x32 v3.2, i0       || IAU.INCS i0, 0x01
CMU.CPIV.x32 v3.3, i0
CMU.CPVV.i32.f32 v3,  v3
CMU.CPVV.i32.f32 v8,  v8
VAU.ADD.f32 v3, v3, 0.5
BRU.BRA Resize_InitLoop     || IAU.SUB i0, i_hi, 0x01
CMU.CPIVR.x32 v1, i0        || IAU.XOR i4, i4, i4
CMU.CPVI.x32 i13, v30.2
CMU.CPVI.x32 i14, v30.3
CMU.CPII i15, i_ho          || LSU1.LDIL i30, Resize_InitH
CMU.CPSVR.x32 v4, s4        || LSU1.LDIH i30, Resize_InitH
.lalign
Resize_InitH:
; init horizontal filter
CMU.CPIS.i32.f32 s2, i_wi
CMU.CPIS.i32.f32 s3, i_wo   || LSU1.LDIL i0, 0x04
CMU.CPIVR.x32 v8,  i0       || LSU1.LDIL i0, 0x100
CMU.CPIVR.x32 v9,  i0       || IAU.XOR  i0, i0, i0 || SAU.DIV.f32 s4, s2, s3
CMU.CPIV.x32 v3.0, i0       || IAU.INCS i0, 0x01
CMU.CPIV.x32 v3.1, i0       || IAU.INCS i0, 0x01
CMU.CPIV.x32 v3.2, i0       || IAU.INCS i0, 0x01
CMU.CPIV.x32 v3.3, i0
CMU.CPVV.i32.f32 v3,  v3
CMU.CPVV.i32.f32 v8,  v8
VAU.ADD.f32 v3, v3, 0.5
BRU.BRA Resize_InitLoop     || IAU.SUB i0, i_wi, 0x01
CMU.CPIVR.x32 v1, i0        || IAU.XOR i4, i4, i4
CMU.CPVI.x32 i13, v31.2
CMU.CPVI.x32 i14, v31.3
CMU.CPII i15, i_wo          || LSU1.LDIL i30, Resize
CMU.CPSVR.x32 v4, s4        || LSU1.LDIH i30, Resize

.lalign
Resize_InitLoop:
; transpose into original image by multiply scaling factor
VAU.MUL.f32 v2, v3, v4
VAU.ADD.f32 v3, v3, v8
CMU.CPVV.i32.f32 v7, v9
CMU.CPVV.f32.i32s v0, v2
; multiply wi 256 to get contribution as u8
VAU.MUL.f32 v2, v2, v7
CMU.MIN.i32 v0, v0, v1
nop
; extract and store increments
CMU.CPVI.x32 i5, v0.0
CMU.CPVI.x32 i5, v0.1   || IAU.SUB.u32s i0,  i5,  i4   || SAU.OR i4, i5, i5
CMU.CPVI.x32 i5, v0.2   || IAU.SUB.u32s i0,  i5,  i4   || SAU.OR i4, i5, i5  || LSU0.STI8 i0, i13
CMU.CPVI.x32 i5, v0.3   || IAU.SUB.u32s i0,  i5,  i4   || SAU.OR i4, i5, i5  || LSU0.STI8 i0, i13
                           IAU.SUB.u32s i0,  i5,  i4   || SAU.OR i4, i5, i5  || LSU0.STI8 i0, i13
                                                                                LSU0.STI8 i0, i13
; calculate contributions, stored in u8, other part is calculated in each filter
CMU.CPVV.f32.i32s v0, v2 || IAU.SUB.u32s i15, i15, 0x04
PEU.PC1I 0x6 || BRU.BRA Resize_InitLoop
PEU.PC1I 0x1 || BRU.JMP i30
CMU.VSZMBYTE v0, v0, [ZZZ0]
CMU.CPVV.u32.u8s v0, v0
CMU.CPVI.x32 i0, v0.0
LSU0.STI32 i0, i14
nop

; DMA read task
; i18     : number of lines to fetch
.lalign
DMA_Read:
; waiting for DMA to finish is done when we call the function

;LSU1.LDIL i0, DMA_BUSY_MASK
;CMU.CMTI.BITP i0, P_GPI
;PEU.PC1C 0x6 || BRU.RPIM 0 || CMU.CMTI.BITP i0, P_GPI

                                IAU.ADD i1, i_stride, i_wi   || LSU1.LDIL  i0,   0x01
                                IAU.SUB i5, i_endp, 0x01     || LSU0.STA32 i0,   LOCAL_SLICE, DMA0_CFG
CMU.CPVI.x32 i2, v30.0       || IAU.MUL i4, i1, i_endp       || LSU0.STA32 i0,   LOCAL_SLICE, DMA1_CFG
CMU.CPVI.x32 i3, v30.1       || IAU.MUL i5, i1, i5           || LSU0.STA32 i_wi, LOCAL_SLICE, DMA0_SIZE
BRU.JMP i30                                                  || LSU0.STA32 i_wi, LOCAL_SLICE, DMA1_SIZE
                                IAU.ADD i4, i4, i_inBuffer   || LSU0.STA32 i2,   LOCAL_SLICE, DMA0_DST_ADDR
                                IAU.ADD i5, i5, i_inBuffer   || LSU0.STA32 i3,   LOCAL_SLICE, DMA1_DST_ADDR
								
CMU.CMZ.i32 i18                                              || LSU0.STA32 i4,   LOCAL_SLICE, DMA0_SRC_ADDR								
								IAU.FEXT.u32  i0 i18 0x1 0x1 || LSU0.STA32 i5,   LOCAL_SLICE, DMA1_SRC_ADDR
PEU.PC1C 0x6                                                 || LSU0.STA32 i0,   LOCAL_SLICE, DMA_TASK_REQ

; DMA write task
.lalign
DMA_Write:
; resize call is over - increment Y and make sure DMA transfer is over
;LSU1.LDIL i0, DMA_BUSY_MASK
;CMU.CMTI.BITP i0, P_GPI
;PEU.PC1C 0x6 || BRU.RPIM 0 || CMU.CMTI.BITP i0, P_GPI

; resize call is over - increment Y and make sure DMA transfer is over
LSU1.LDIL i0, DMA_BUSY_MASK
CMU.CMTI.BITP i0, P_GPI
PEU.PC1C 0x6 || BRU.RPIM 0 || CMU.CMTI.BITP i0, P_GPI

LSU1.LDIH i2, BufferOut     || LSU0.LDIL i2, BufferOut
BRU.JMP i30                  || LSU1.LDIL  i0,   0x01        
|| IAU.SHL i15 i_wo 1
IAU.MUL i3, i_posy, i15         || LSU0.STA32 i0,   LOCAL_SLICE, DMA0_CFG 
                                                               LSU0.STA32 i15, LOCAL_SLICE, DMA0_SIZE
                                IAU.ADD i3, i3, i_outBuffer  || LSU0.STA32 i2,   LOCAL_SLICE, DMA0_SRC_ADDR
                                IAU.XOR i0, i0, i0           || LSU0.STA32 i3,   LOCAL_SLICE, DMA0_DST_ADDR
LSU0.STA32 i0,   LOCAL_SLICE, DMA_TASK_REQ


DMA_Read_wait:
; resize call is over - increment Y and make sure DMA transfer is over
LSU1.LDIL i0, DMA_BUSY_MASK
CMU.CMTI.BITP i0, P_GPI
PEU.PC1C 0x6 || BRU.RPIM 0   || CMU.CMTI.BITP i0, P_GPI
                                IAU.ADD i1, i_stride, i_wi   || LSU1.LDIL  i0,   0x01
                                IAU.SUB i5, i_endp, 0x01     || LSU0.STA32 i0,   LOCAL_SLICE, DMA0_CFG
CMU.CPVI.x32 i2, v30.0       || IAU.MUL i4, i1, i_endp       || LSU0.STA32 i0,   LOCAL_SLICE, DMA1_CFG
CMU.CPVI.x32 i3, v30.1       || IAU.MUL i5, i1, i5           || LSU0.STA32 i_wi, LOCAL_SLICE, DMA0_SIZE
                                                                LSU0.STA32 i_wi, LOCAL_SLICE, DMA1_SIZE
                                IAU.ADD i4, i4, i_inBuffer   || LSU0.STA32 i2,   LOCAL_SLICE, DMA0_DST_ADDR
                                IAU.ADD i5, i5, i_inBuffer   || LSU0.STA32 i3,   LOCAL_SLICE, DMA1_DST_ADDR
								
CMU.CMZ.i32 i18                                              || LSU0.STA32 i4,   LOCAL_SLICE, DMA0_SRC_ADDR
								IAU.FEXT.u32  i0 i18 0x1 0x1 || LSU0.STA32 i5,   LOCAL_SLICE, DMA1_SRC_ADDR
PEU.PC1C 0x6                                                 || LSU0.STA32 i0,   LOCAL_SLICE, DMA_TASK_REQ
BRU.JMP i30
NOP 2
; resize call is over - increment Y and make sure DMA transfer is over
LSU1.LDIL i0, DMA_BUSY_MASK
CMU.CMTI.BITP i0, P_GPI
PEU.PC1C 0x6 || BRU.RPIM 0 || CMU.CMTI.BITP i0, P_GPI



.if (YUV_FORMAT == YUV_420)
DMA_Write_420:
; resize call is over - increment Y and make sure DMA transfer is over
LSU1.LDIL i0, DMA_BUSY_MASK
CMU.CMTI.BITP i0, P_GPI
PEU.PC1C 0x6 || BRU.RPIM 0 || CMU.CMTI.BITP i0, P_GPI
CMU.CPVI.x32 i2, v31.1
BRU.JMP i30                     || LSU1.LDIL  i0,   0x01        
IAU.MUL i3, i_posy, i_wo         || LSU0.STA32 i0,   LOCAL_SLICE, DMA0_CFG 
LSU0.STA32 i_wo, LOCAL_SLICE, DMA0_SIZE
IAU.ADD i3, i3, i_outBuffer  || LSU0.STA32 i2,   LOCAL_SLICE, DMA0_SRC_ADDR
IAU.XOR i0, i0, i0           || LSU0.STA32 i3,   LOCAL_SLICE, DMA0_DST_ADDR
LSU0.STA32 i0,   LOCAL_SLICE, DMA_TASK_REQ
.endif


; Input parameters
.set i13_LumaBufferAddr   i13
.set i14_CrBufferAddr     i14
.set i15_CbBufferAddr     i15
.set i18_YUV422BufferAddr i18
.set i16_StepNr           i16 ;Linewidth / 8 /2  ; i_wo width

.set i30_JmpReturn        i30 ; return address

; Internal data used by the component
.set i17_JmpAddr          i17
.set v11_LumaData         v11
.set v18_LumaData         v18
.set v12_CrData           v12
.set v13_CbData           v13
.set v12_CrCbData0        v12
.set v13_CrCbData1        v13


Interleave422:

LSU1.LDIH i13_LumaBufferAddr, BufferOut_Y     || LSU0.LDIL i13_LumaBufferAddr, BufferOut_Y || IAU.SHR.i32 i16_StepNr i_wo 4
LSU1.LDIH i14_CrBufferAddr, BufferOut_Cr      || LSU0.LDIL i14_CrBufferAddr, BufferOut_Cr  || IAU.XOR i18 i18 i18 ; reset Conditon code register
LSU1.LDIH i15_CbBufferAddr, BufferOut_Cb      || LSU0.LDIL i15_CbBufferAddr, BufferOut_Cb 
LSU1.LDIH i18_YUV422BufferAddr, BufferOut     || LSU0.LDIL i18_YUV422BufferAddr, BufferOut 
LSU0.LDIL i17_JmpAddr StartInterleaveStep     || LSU1.LDIH i17_JmpAddr StartInterleaveStep
StartInterleaveStep:
LSU0.LDI128.u8.u16  v12_CrData    i14_CrBufferAddr   
|| PEU.PCIX.NEQ   0x20 || LSU1.STI128.u16.u8  v11_LumaData   i18_YUV422BufferAddr

LSU0.LDI128.u8.u16  v13_CbData    i15_CbBufferAddr   
|| PEU.PCIX.NEQ   0x20 || LSU1.STI128.u16.u8  v12_CrCbData0  i18_YUV422BufferAddr

LSU0.LDI128.u8.u16  v11_LumaData  i13_LumaBufferAddr 
|| PEU.PCIX.NEQ   0x20 || LSU1.STI128.u16.u8  v18_LumaData   i18_YUV422BufferAddr

LSU0.LDI128.u8.u16  v18_LumaData  i13_LumaBufferAddr
|| PEU.PCIX.NEQ   0x20 || LSU1.STI128.u16.u8  v13_CrCbData1  i18_YUV422BufferAddr
BRU.JMP i17_JmpAddr
IAU.INCS i16_StepNr -1 
PEU.PC1I LTE || IAU.INCS i16_StepNr 1 || LSU1.LDIH i17_JmpAddr, DMA_Write     || LSU0.LDIL i17_JmpAddr, DMA_Write
CMU.VILV.x16 v12_CrCbData0  v13_CrCbData1   v13_CbData    v12_CrData
CMU.VILV.x16 v11_LumaData   v12_CrCbData0   v12_CrCbData0 v11_LumaData
CMU.VILV.x16 v18_LumaData   v13_CrCbData1   v13_CrCbData1 v18_LumaData
EndInterleaveStep:

