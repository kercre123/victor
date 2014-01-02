; ///
; /// @file
; /// @copyright All code copyright Movidius Ltd 2013, all rights reserved.
; ///            For License Warranty see: common/license.txt
; ///
; /// @brief
; ///

.version 00.70.00
.include svuCommonDefinitions.incl
.include svuCommonMacros.incl

.data .rodata.vect_multiply
.salign 16
    vect_multiply:
        .float16  0.0F16, 1.0F16, 2.0F16, 3.0F16, 4.0F16, 5.0F16, 6.0F16, 7.0F16

.set C_CMU0_GT 0x22222222
; --------------------- IRF -------------------------
.set iTMP0       i0
.set iTMP1       i1
.set iTMP2       i2
.set iTMP3       i3

; globals
.set first_step  i4
.set fill_color  i5
.set color_value i6
.set cmu0_gt     i7
.set p_cfg_save  i8
.set col         i9
.set output_aux  i10
.set out_stride  i11
.set in_stride   i12
.set out_height  i13
.set out_width   i14
.set output      i17

; --------------------- SRF -------------------------
.set sxstep_r s0
.set systep_r s1

; --------------------- VRF -------------------------
.set vTMP0       v0
.set vTMP1       v1
.set vTMP2       v2
.set vTMP3       v3
.set vTMP4       v4
.set vTMP5       v5
.set dest_new    v6
.set vTMP6       v6 ; duplicate
.set dest        v7
.set vTMP7       v7 ; duplicate
.set vTMP8       v9 ; duplicate
.set vTMP9       v10; duplicate
.set vTMP10      v11; duplicate
.set vTMP11      v12; duplicate

; globals
.set vin_stride  v8
.set vOne        v9
.set cxcy        v10
.set cx          v11
.set cy          v12
.set x           v13
.set y           v14
.set xstep_c     v15
.set ystep_c     v16
.set xstart_r    v17
.set ystart_r    v18
.set input       v19
.set in_width    v20
.set in_height   v21
.set _S3         v22
.set _S2         v23
.set _S1         v24
.set _S0         v25


;void WarpAffineTransformBilinear(u8** in, u8** out, u32 in_width, u32 in_height, u32 out_width, u32 out_height, u32 in_stride, u32 out_stride, u8 fill_color, half mat[2][3])
;                                     i18,      i17,     i16,          i15,           i14,           i13,            i12,           i11
.code .text.WarpAffineTransformBilinear_asm
.salign 16

WarpAffineTransformBilinear_asm:
    ;                                                                    || Set rounding mode to zero.
    LSU0.LDIL first_step, 1          || LSU1.LDIL iTMP1, 0x18
    LSU0.LDIL iTMP0, 0xFFE7          || LSU1.LDIH iTMP0, 0xFFFF          || CMU.CPTI p_cfg_save, P_CFG
    LSU0.LDIL cmu0_gt, C_CMU0_GT     || LSU1.LDIH cmu0_gt, C_CMU0_GT     || IAU.AND iTMP2, iTMP0, p_cfg_save ; Clear all rounding bits
    ; Initialize lines pointers
    LSU0.LDO32 fill_color, i19, 0x00 || LSU1.LDO32 i9, i19, 0x04         || IAU.OR iTMP2, iTMP1, iTMP2 ; Set our desired bits
    ; initialize multiplicator
    LSU0.LDIL iTMP3, __vect_multiply || LSU1.LDIH iTMP3, __vect_multiply || CMU.CPIT P_CFG, iTMP2
    LSU0.LD32r input, i18            || LSU1.LD32 output, i17            || CMU.CPIVR.x16 in_width, i16
    LSU0.LDO64.l vTMP2, iTMP3, 0     || LSU1.LDO64.h vTMP2, iTMP3, 8     || CMU.CPIVR.x16 in_height, i15

    PUSH_V_REG v24
    PUSH_V_REG v25

    ; Load b11, b12, b13, b21, b22, b23
    LSU0.LD16r xstep_c, i9  || LSU1.LDO16 sxstep_r, i9, 2 || IAU.ADD i9, i9, 4 || CMU.CPIVR.x32 vin_stride, in_stride
    LSU0.LD16r xstart_r, i9                               || IAU.ADD i9, i9, 2 || CMU.CPIVR.x16 vOne, first_step
    LSU0.LD16r ystep_c, i9  || LSU1.LDO16 systep_r, i9, 2 || IAU.ADD i9, i9, 4
    LSU0.LD16r ystart_r, i9                               || IAU.XOR color_value, color_value, color_value

    IAU.SUB fill_color, fill_color, 0 || LSU1.LDIL iTMP0, 8
    PEU.PC1I NEQ || LSU0.LD8 color_value, fill_color

    CMU.CPIVR.x16 vTMP4, iTMP0
    CMU.CPVV.i16.f16 vTMP4, vTMP4
    VAU.MUL.f16 vTMP0, xstep_c, vTMP2
    VAU.MUL.f16 vTMP1, ystep_c, vTMP2
    VAU.MUL.f16 xstep_c, xstep_c, vTMP4
    VAU.ADD.f16 xstart_r, xstart_r, vTMP0
    VAU.ADD.f16 ystart_r, ystart_r, vTMP1 || CMU.CPII col, out_width
    VAU.MUL.f16 ystep_c, ystep_c, vTMP4   || CMU.CPII output_aux, output ; output_aux = output
    CMU.CPVV x, xstart_r ; x = xstart_r
    CMU.CPVV y, ystart_r ; y = ystart_r;

    SAU.MUL.f16 s19, s19, s19
__mainloopBi:
    ; Get nearest four neighbors around source location.                                                            || Bi-linear interpolation.
    ; S0 = (float)input[yi*in_stride             + xi] ; S1 = (float)input[yi*in_stride             + xi + 1]          S0 = (1.0 - cx)*S0 + cx*S1
    ; S2 = (float)input[yi*in_stride + in_stride + xi] ; S3 = (float)input[yi*in_stride + in_stride + xi + 1];         S2 = (1.0 - cx)*S2 + cx*S3
    ;                                                                                                                  S0 = (1.0 - cy)*S0 + cy*S2
    CMU.CPVV.f16.i32s vTMP1, y    || VAU.ALIGNVEC vTMP3, y, y, 8
    CMU.CPVV.f16.i32s vTMP3, vTMP3|| VAU.ALIGNVEC vTMP2, x, x, 8
    CMU.CPVV.f16.i32s vTMP0, x    || VAU.MUL.i32 vTMP1, vTMP1, vin_stride
    CMU.CPVV.f16.i32s vTMP2, vTMP2|| VAU.MUL.i32 vTMP3, vTMP3, vin_stride
    CMU.CPVV.f16.u8fs cx, cx
    CMU.CPVV.f16.u8fs cy, cy      || VAU.ADD.u32s vTMP0, input, vTMP0
                                     VAU.ADD.u32s vTMP4, input, vTMP2
    CMU.CPVV.u8.u16 cx, cx        || VAU.ADD.u32s vTMP0, vTMP0, vTMP1
    CMU.CPVV.u8.u16 cy, cy        || VAU.ADD.u32s vTMP4, vTMP4, vTMP3
    ; Load the corresponding sources
    CMU.CPVI.x32 iTMP0, v0.0                                                                                           || VAU.MUL.u16s cxcy, cx, cy
    CMU.CPVI.x32 iTMP1, v0.1 || IAU.ADD iTMP2, iTMP0, in_stride                                                        || VAU.MACNZ.i16 _S0, vOne
    CMU.CPVI.x32 iTMP0, v0.2 || IAU.ADD iTMP3, iTMP1, in_stride || LSU0.LD16r vTMP3, iTMP0  || LSU1.LD16r vTMP7, iTMP2 || VAU.MACN.i16 cx, _S0
    CMU.CPVI.x32 iTMP1, v0.3 || IAU.ADD iTMP2, iTMP0, in_stride || LSU0.LD16r vTMP2, iTMP1  || LSU1.LD16r vTMP6, iTMP3 || VAU.MACN.i16 cy, _S0
    CMU.CPVI.x32 iTMP0, v4.0 || IAU.ADD iTMP3, iTMP1, in_stride || LSU0.LD16r vTMP1, iTMP0  || LSU1.LD16r vTMP5, iTMP2 || VAU.MACP.i16 cxcy, _S0
    CMU.CPVI.x32 iTMP1, v4.1 || IAU.ADD iTMP2, iTMP0, in_stride || LSU0.LD16r vTMP0, iTMP1  || LSU1.LD16r vTMP4, iTMP3 || VAU.MACP.i16 cy, _S2
    CMU.CPVI.x32 iTMP0, v4.2 || IAU.ADD iTMP3, iTMP1, in_stride || LSU0.LD16r vTMP11, iTMP0 || LSU1.LD16r _S0, iTMP2   || VAU.MACN.i16 cxcy, _S2
    CMU.CPVI.x32 iTMP1, v4.3 || IAU.ADD iTMP2, iTMP0, in_stride || LSU0.LD16r vTMP10, iTMP1 || LSU1.LD16r _S1, iTMP3   || VAU.MACP.i16 cx, _S1
                                IAU.ADD iTMP3, iTMP1, in_stride || LSU0.LD16r vTMP9, iTMP0  || LSU1.LD16r _S2, iTMP2   || VAU.MACN.i16 cxcy, _S1
                                                                   LSU0.LD16r vTMP8, iTMP1  || LSU1.LD16r _S3, iTMP3   || VAU.MACPW.i16 dest, cxcy, _S3
    NOP
    CMU.CPVCR.x16 v4.l v4.0
    CMU.CPVCR.x16 v5.l v0.0
    NOP
    CMU.CPIVR.x16 dest_new, color_value
    CMU.CPVCR.x16 v4.h v22.0                                                                                           || VAU.XOR vTMP1, vTMP1, vTMP1
    CMU.CPVCR.x16 v5.h v9.0                                                                                            || VAU.SUB.i16 vTMP2, vTMP1, 2
    CMU.VDILV.x8 _S3, _S2, vTMP4, vTMP0
    CMU.VDILV.x8 _S1, _S0, vTMP5, vTMP0                                                                                || VAU.NOT vOne, vTMP1
    CMU.CPVV.u8.u16 _S2, _S2                                                                || PEU.PVV16 GT            || VAU.AND dest_new, vTMP1, dest_new
    CMU.CPVV.u8.u16 _S3, _S3                                                                || PEU.PVV16 GT            || VAU.OR dest_new, dest_new, dest
    CMU.CPVV.u8.u16 _S0, _S0                                                                                           || VAU.FRAC.f16 cx, x
    CMU.CPVV.u8.u16 _S1, _S1  || IAU.SUB iTMP0, fill_color, 0                                                          || VAU.FRAC.f16 cy, y
    ; fill with color or let as it is
    PEU.PC1I NEQ   || CMU.CPIT C_CMU0, cmu0_gt
    PEU.PVL1S8 GT || LSU1.ST128.u16.u8 dest_new, output_aux || IAU.INCS output_aux, 8  || CMU.CPVV.f16.i16s vTMP0, x
                                                               IAU.INCS col, -8        || CMU.CPVV.f16.i16s vTMP1, y
    PEU.PC1I GT || BRU.BRA __mainloopBi
    ; if((xi >= -1) && (xi <= (in_width - 1)) && (yi >= -1) && (yi <= (in_height - 1)))
                   CMU.CMVV.i16 vTMP0, vTMP2                                       || VAU.ADD.f16 x, x, xstep_c
    PEU.ANDACC  || CMU.CMVV.i16 vTMP1, vTMP2                                       || VAU.ADD.f16 y, y, ystep_c
    PEU.ANDACC  || CMU.CMVV.i16 in_width, vTMP0                                    || SAU.SUB.i32 first_step, first_step, 1
    PEU.ANDACC  || CMU.CMVV.i16 in_height, vTMP1
    PEU.PC1S EQ || CMU.CPII output_aux, output
;------------------------------------------------------------------
    IAU.INCS out_height, -1  || LSU0.LDIL first_step, 1 || CMU.CPSVR.x16 vTMP0, sxstep_r    || SAU.ADD.u32s output, output, out_stride
    PEU.PC1I GT              || BRU.BRA __mainloopBi    || CMU.CPSVR.x16 vTMP1, systep_r
    CMU.CPII col, out_width                             || VAU.ADD.f16 xstart_r, xstart_r, vTMP0
                                                           VAU.ADD.f16 ystart_r, ystart_r, vTMP1
    NOP
    CMU.CPVV x, xstart_r
    CMU.CPVV y, ystart_r
;------------------------------------------------------------------
    VAU.MUL.u16s cxcy, cx, cy

    ; Compute the last 8 pixels
    VAU.MACNZ.i16 _S0, vOne
    VAU.MACN.i16 cx, _S0
    VAU.MACN.i16 cy, _S0
    VAU.MACP.i16 cxcy, _S0
    VAU.MACP.i16 cx, _S1
    VAU.MACN.i16 cxcy, _S1
    VAU.MACP.i16 cy, _S2
    VAU.MACN.i16 cxcy, _S2
    VAU.MACPW.i16 dest, cxcy, _S3
    ; Recover rounding mode.
    CMU.CPIT P_CFG, p_cfg_save
    NOP 2

    ; fill with color or let as it is
    VAU.XOR vTMP1, vTMP1, vTMP1
    CMU.CPIVR.x16 dest_new, color_value
    PEU.PVV16 GT  || VAU.AND dest_new, vTMP1, dest_new
    PEU.PVV16 GT  || VAU.OR dest_new, dest_new, dest || IAU.SUB iTMP0, fill_color, 0
    PEU.PC1I NEQ  || CMU.CPIT C_CMU0, cmu0_gt
    PEU.PVL1S8 GT || LSU1.ST128.u16.u8 dest_new, output_aux

    POP_V_REG v25
    POP_V_REG v24
    BRU.JMP i30
    NOP 5

.end
