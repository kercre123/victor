; ///
; /// @file
; /// @copyright All code copyright Movidius Ltd 2013, all rights reserved.
; ///            For License Warranty see: common/license.txt
; ///
; /// @brief
; ///

.version 00.51.04

; /------------------/
;    Local defines
; /------------------/

.set srcImage    i18
.set maskImage   i17
.set destImage   i16
.set widthImage  i15
.set heightImage i14

; /------------------/
;    CODE SECTION
; /------------------/
.code .text.AccumulateSquare_asm
;void AccumulateSquare(u8** srcAddr, u8** maskAddr, fp32** destAddr, u32 width, u32 height)

AccumulateSquare_asm:

LSU0.LD32 srcImage  srcImage
LSU0.LD32 maskImage maskImage
LSU0.LD32 destImage destImage
LSU0.LDIL i0 32
    NOP 4

LSU0.LDIL i2 ___Repeat  || LSU1.LDIH i2 ___Repeat
LSU0.LDIL i3 ___Repeat2 || LSU1.LDIH i3 ___Repeat2

IAU.SHR.U32 i6 widthImage 5

CMU.CMII.i32 widthImage i0
PEU.PC1C LT  || IAU.SHR.u32  i7  widthImage  3  ||  BRU.BRA ___Process_Last_Bits_Loop 
    NOP 5

___Loop:
    // Load the src image
	LSU0.LD128.u8.f16 v0  srcImage  || IAU.ADD srcImage  srcImage  8  || BRU.RPL i2 i6
	LSU0.LD128.u8.f16 v1  srcImage  || IAU.ADD srcImage  srcImage  8
	LSU0.LD128.u8.f16 v2  srcImage  || IAU.ADD srcImage  srcImage  8
	LSU0.LD128.u8.f16 v3  srcImage  || IAU.ADD srcImage  srcImage  8
 
    // Load the mask image
	LSU0.LD128.u8.f16 v12 maskImage || IAU.ADD maskImage maskImage 8
    LSU0.LD128.u8.f16 v13 maskImage || IAU.ADD maskImage maskImage 8
    LSU0.LD128.u8.f16 v14 maskImage || IAU.ADD maskImage maskImage 8
    LSU0.LD128.u8.f16 v15 maskImage || IAU.ADD maskImage maskImage 8
        NOP

    // Process src and mask images to match the f32 request
    CMU.CPVV.f16l.f32 v6  v0
    CMU.CPVV.f16h.f32 v7  v0
	CMU.CPVV.f16l.f32 v17 v12
    CMU.CPVV.f16h.f32 v18 v12

	CMU.CPVV.f16l.f32 v8  v1
    CMU.CPVV.f16h.f32 v9  v1
    CMU.CPVV.f16l.f32 v19 v13
    CMU.CPVV.f16h.f32 v20 v13

    CMU.CPVV.f16l.f32 v10 v2
    CMU.CPVV.f16h.f32 v11 v2
    CMU.CPVV.f16l.f32 v4  v14
    CMU.CPVV.f16h.f32 v5  v14

    CMU.CPVV.f16l.f32 v12 v3
    CMU.CPVV.f16h.f32 v13 v3
    CMU.CPVV.f16l.f32 v14 v15
    CMU.CPVV.f16h.f32 v15 v15


    // Load the current destination image
	LSU0.LD64.l  v22 destImage           || LSU1.LDO64.h v22 destImage  (8*1)   || VAU.MUL.F32 v6  v6  v6
	LSU0.LDO64.l v23 destImage  (8*2)    || LSU1.LDO64.h v23 destImage  (8*3)   || VAU.MUL.F32 v7  v7  v7

    LSU0.LDO64.l v0 destImage   (8*4)    || LSU1.LDO64.h v0 destImage   (8*5)   || VAU.MUL.F32 v8  v8  v8
    LSU0.LDO64.l v1 destImage   (8*6)    || LSU1.LDO64.h v1 destImage   (8*7)   || VAU.MUL.F32 v9  v9  v9

    LSU0.LDO64.l v2 destImage   (8*8)    || LSU1.LDO64.h v2 destImage   (8*9)   || VAU.MUL.F32 v10 v10 v10
    LSU0.LDO64.l v3 destImage   (8*10)   || LSU1.LDO64.h v3 destImage   (8*11)  || VAU.MUL.F32 v11 v11 v11

    LSU0.LDO64.l v24 destImage  (8*12)   || LSU1.LDO64.h v24 destImage  (8*13)  || VAU.MUL.F32 v12 v12 v12
    LSU0.LDO64.l v25 destImage  (8*14)   || LSU1.LDO64.h v25 destImage  (8*15)  || VAU.MUL.F32 v13 v13 v13


___Test:

    // Check if mask is 0
    CMU.CMZ.f32 v17
    PEU.PVV32 GT    || VAU.ADD.f32 v22 v6  v22
	CMU.CMZ.f32 v18
    PEU.PVV32 GT    || VAU.ADD.f32 v23 v7  v23

	CMU.CMZ.f32 v19
    PEU.PVV32 GT    || VAU.ADD.f32 v0  v8  v0
    CMU.CMZ.f32 v20
    PEU.PVV32 GT    || VAU.ADD.f32 v1  v9  v1

    CMU.CMZ.f32 v4
    PEU.PVV32 GT    || VAU.ADD.f32 v2  v10 v2
    CMU.CMZ.f32 v5
    PEU.PVV32 GT    || VAU.ADD.f32 v3  v11 v3

    CMU.CMZ.f32 v14
    PEU.PVV32 GT    || VAU.ADD.f32 v24 v12 v24
    CMU.CMZ.f32 v15
    PEU.PVV32 GT    || VAU.ADD.f32 v25 v13 v25



    // Update the destination image
	LSU0.ST64.l  v22 destImage          || LSU1.STO64.h v22 destImage (8*1)
	LSU0.STO64.l v23 destImage  (8*2)   || LSU1.STO64.h v23 destImage (8*3)
    LSU0.STO64.l v0  destImage  (8*4)   || LSU1.STO64.h v0  destImage (8*5)

___Repeat:
    LSU0.STO64.l v1  destImage  (8*6)   || LSU1.STO64.h v1  destImage (8*7)
    LSU0.STO64.l v2  destImage  (8*8)   || LSU1.STO64.h v2  destImage (8*9)
    LSU0.STO64.l v3  destImage  (8*10)  || LSU1.STO64.h v3  destImage (8*11)
    LSU0.STO64.l v24 destImage  (8*12)  || LSU1.STO64.h v24 destImage (8*13)
    LSU0.STO64.l v25 destImage  (8*14)  || LSU1.STO64.h v25 destImage (8*15)
	IAU.ADD      destImage destImage (8*16)

    ; Process last bits
    SAU.SHL.u32  i7  i6  5
        NOP 2
        
    ; The remaining bits
    IAU.SUB      i7  widthImage  i7 

    CMU.CMZ.i8   i7                   || BRU.JMP i30
        NOP 5
    IAU.SHR.u32  i7  i7  3
        NOP 2

___Process_Last_Bits_Loop:

    LSU0.LD128.u8.f16 v0    srcImage  || IAU.ADD srcImage   srcImage  8   || BRU.RPL  i3 i7
    // Load the mask image
    LSU0.LD128.u8.f16 v12   maskImage || IAU.ADD maskImage  maskImage 8
        NOP 4
        
    // Process src and mask images to match the f32 request
    CMU.CPVV.f16l.f32 v6    v0
    CMU.CPVV.f16h.f32 v7    v0
    CMU.CPVV.f16l.f32 v17   v12
    CMU.CPVV.f16h.f32 v18   v12
    NOP 4
 
    // Load the current destination image
    LSU0.LD64.l  v22 destImage        || LSU1.LDO64.h v22 destImage (8*1)  || VAU.MUL.F32 v6 v6 v6
    LSU0.LDO64.l v23 destImage (8*2)  || LSU1.LDO64.h v23 destImage (8*3)  || VAU.MUL.F32 v7 v7 v7
        NOP 4

    // Check if mask is 0
    CMU.CMZ.f32 v17
    PEU.PVV32 GT    || VAU.ADD.f32 v22 v6 v22
    CMU.CMZ.f32 v18
    PEU.PVV32 GT    || VAU.ADD.f32 v23 v7 v23
        
___Repeat2:
        NOP 
     // Update the destination image
     LSU0.STO64.l v22 destImage  0      || LSU1.STO64.h v22 destImage (8*1) 
     LSU0.STO64.l v23 destImage (8*2)   || LSU1.STO64.h v23 destImage (8*3) ||  IAU.ADD destImage destImage 32
         NOP 3

___End:
    BRU.JMP i30
        NOP 5
.end


