; ///
; /// @file
; /// @copyright All code copyright Movidius Ltd 2013, all rights reserved.
; ///            For License Warranty see: common/license.txt
; ///
; /// @brief
; ///

.version 00.51.04

.data .data.convolution

.salign 16

.code .text.convolution

;void Convolution_asm(u8** in(i18), u8** out(i17), u32 kernelSize(i16), half* conv(i15), u32 inWidth(i14))
Convolution_asm:
    IAU.MUL          i4  i16          i16        ||  SAU.SUB.f16      s2  s2  s2
    LSU0.LDIL        i8   ___forLoop             ||  LSU1.LDIH        i8 ___forLoop
    LSU1.LD32        i17 i17                     ||  IAU.SUB          i12 i12 i12
    SAU.ADD.f16      s2  s2 1
    CMU.CPII         i5  i18                     ||  SAU.SUB.f32      s3  s3  s3    ||  IAU.SUB          i3  i3  i3
    CMU.CPII         i6  i15                     ||  LSU0.LDIL        i7  0x8
    IAU.SHL          i9 i16 1

    ___forLoopHead:
        BRU.RPL i8  i14
        ___getNewLines:

            LSU0.LD32         i1   i5            ||  IAU.ADD           i5  i5  4
            nop 4
            IAU.SUB           i2  i2  i2; i2 = 0 max 8, or kernel size if kernel size < 8
            IAU.ADD           i1  i1  i12

            LSU0.LD128.u8.f16 v0  i1       ||  IAU.ADD      i1  i1  8
            LSU0.LDO64.l      v1  i6 0    ||  LSU1.LDO64.h v1  i6 8
            LSU0.LD128.u8.f16 v2  i1
            LSU0.LDO64.l      v3 i6 16    ||  LSU1.LDO64.h v3  i6 24
            nop
            IAU.ADD           i6  i6  i9

            // conv pe 1linia aia
                ___getNewCols:
                    // loox kernel ori < 8
                    CMU.CPVS.x16 s0.l v0.0       ||  IAU.ADD i2  i2  1
                    CMU.CPVS.x16 s1.l v1.0       ||  IAU.ADD i3  i3  1
                    SAU.MUL.F16  s2   s0.l s1.l
                    nop 2
                    SAU.ADD.f16  s3   s3.l s2.l

                    CMU.CMII.u32 i2   i16
                    PEU.PC1C LT                  ||  BRU.BRA  ___getNewCols
                    CMU.VROT     v0   v0  2
                    CMU.VROT     v1   v1  2
                    CMU.CMII.u32 i7   i16
                    PEU.PC1C LT                  ||  CMU.CPVV v0  v2
                    PEU.PC1C LT                  ||  CMU.CPVV v1  v3


                    CMU.CMII.u32 i3  i4
                    PEU.PC1C LT                  ||  BRU.BRA ___getNewLines
                    nop 5


        ___forLoop:
            nop
            CMU.CPII           i5  i18           ||  IAU.SUB     i3  i3  i3
            CMU.CPII           i6  i15
            CMU.CPSI.f16.i16s  i11 s3.l
            IAU.ADD            i12 i12  1        ||  SAU.SUB.f32 s3  s3  s3
            LSU0.ST8           i11 i17           ||  IAU.ADD     i17 i17 1

    BRU.jmp i30
    nop 5

.end

