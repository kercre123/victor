; ///
; /// @file
; /// @copyright All code copyright Movidius Ltd 2013, all rights reserved.
; ///            For License Warranty see: common/license.txt
; ///
; /// @brief
; ///

.version 00.70.00

.data .rodata.vect_multiply
.salign 16
    __vect_multiply:
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
.set value_2     i15
.set output      i17
.set iTMP4       i18

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
.set vTMP6       v6
.set vTMP7       v7
.set vValue2     v8

; globals
.set vNULL       v9
.set vin_stride  v10
.set dest        v11
.set dest_new    v12
.set x           v13
.set y           v14
.set xi          v15
.set yi          v16
.set xstep_c     v17
.set ystep_c     v18
.set xstart_r    v19
.set ystart_r    v20
.set input       v21
.set in_width    v22
.set in_height   v23

;void WarpAffineTransformNN(u8** in, u8** out, u32 in_width, u32 in_height, u32 out_width, u32 out_height, u32 in_stride, u32 out_stride, u8 fill_color, half mat[2][3])
;                               i18,      i17,     i16,          i15,           i14,           i13,            i12,           i11
.code .text.WarpAffineTransformNN_asm
.salign 16

WarpAffineTransformNN_asm:
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

    VAU.XOR vNULL, vNULL, vNULL
    CMU.CPIVR.x32 vin_stride, in_stride
    ; Load b11, b12, b13, b21, b22, b23
    LSU0.LD16r xstep_c, i9  || LSU1.LDO16 sxstep_r, i9, 2 || IAU.ADD i9, i9, 4 ; b11 & b12
    LSU0.LD16r xstart_r, i9                               || IAU.ADD i9, i9, 2 ; b13
    LSU0.LD16r ystep_c, i9  || LSU1.LDO16 systep_r, i9, 2 || IAU.ADD i9, i9, 4 ; b21 & b22
    LSU0.LD16r ystart_r, i9                               || IAU.XOR color_value, color_value, color_value
    LSU0.LDIL iTMP0, 8                                    || IAU.SUB fill_color, fill_color, 0
    PEU.PC1I NEQ || LSU0.LD8 color_value, fill_color

    CMU.CPIVR.x16 vTMP4, iTMP0            || VAU.SUB.i16 vValue2, vNULL, 2
    CMU.CPVV.i16.f16 vTMP4, vTMP4
    VAU.MUL.f16 vTMP0, xstep_c, vTMP2
    VAU.MUL.f16 vTMP1, ystep_c, vTMP2
    VAU.MUL.f16 xstep_c, xstep_c, vTMP4
    VAU.ADD.f16 xstart_r, xstart_r, vTMP0
    VAU.ADD.f16 ystart_r, ystart_r, vTMP1 || CMU.CPII col, out_width
    VAU.MUL.f16 ystep_c, ystep_c, vTMP4   || CMU.CPII output_aux, output ; output_aux = output
    CMU.CPVV x, xstart_r ; x = xstart_r
    CMU.CPVV y, ystart_r ; y = ystart_r;

    SAU.MUL.f16 s19, s19, s19 || VAU.SUB.i16 vValue2, vNULL, 2
    NOP
.salign 16
__mainloopNN:
    PEU.PVV16 GT || VAU.AND dest, vNULL, dest
    PEU.PVV16 GT || VAU.OR dest, dest, dest_new  || IAU.SUB iTMP0, fill_color, 0
    ; Fill with color or let as it is
    PEU.PC1I NEQ  || CMU.CPIT C_CMU0, cmu0_gt
    ; Get nearest neighbor around source location.
    ; S0 = input[yi*in_stride + xi]
    CMU.CPVV.f16.i32s vTMP1, y     || VAU.ALIGNVEC vTMP3, y, y, 8         || PEU.PVL0S8 GT || LSU0.ST128.u16.u8 dest, output_aux || IAU.INCS output_aux, 8
    CMU.CPVV.f16.i32s vTMP3, vTMP3 || VAU.ALIGNVEC vTMP2, x, x, 8
    CMU.CPVV.f16.i32s vTMP0, x     || VAU.MUL.i32 vTMP1, vTMP1, vin_stride
    CMU.CPVV.f16.i32s vTMP2, vTMP2 || VAU.MUL.i32 vTMP3, vTMP3, vin_stride
    CMU.CPVV.f16.i16s xi, x
    CMU.CPVV.f16.i16s yi, y        || VAU.ADD.u32s vTMP0, input, vTMP0
                                      VAU.ADD.u32s vTMP4, input, vTMP2  ; || if((xi >= -1) && (xi <= (in_width - 1)) && (yi >= -1) && (yi <= (in_height - 1)))
                                      VAU.ADD.u32s vTMP0, vTMP0, vTMP1                   || CMU.CMVV.i16 xi, vValue2
                                      VAU.ADD.u32s vTMP4, vTMP4, vTMP3    || PEU.ANDACC  || CMU.CMVV.i16 yi, vValue2
                                                                             PEU.ANDACC  || CMU.CMVV.i16 in_width, xi
                                                                             PEU.ANDACC  || CMU.CMVV.i16 in_height, yi
    ; Load the corresponding sources
    CMU.CPVI.x32 iTMP0, v0.0
    CMU.CPVI.x32 iTMP1, v0.1
    CMU.CPVI.x32 iTMP2, v0.2
    CMU.CPVI.x32 iTMP3, v0.3
    CMU.CPVI.x32 iTMP4, v4.0 || LSU0.LD8r vTMP3, iTMP0 || LSU1.LD8r vTMP2, iTMP1
    CMU.CPVI.x32 iTMP0, v4.1 || LSU0.LD8r vTMP1, iTMP2 || LSU1.LD8r vTMP0, iTMP3
    CMU.CPVI.x32 iTMP1, v4.2
    CMU.CPVI.x32 iTMP2, v4.3 || LSU0.LD8r vTMP7, iTMP4 || LSU1.LD8r vTMP6, iTMP0 || VAU.ADD.f16 x, x, xstep_c
                                LSU0.LD8r vTMP5, iTMP1 || LSU1.LD8r vTMP4, iTMP2 || VAU.ADD.f16 y, y, ystep_c
    IAU.INCS first_step, -1
    PEU.PC1I EQ || CMU.CPII output_aux, output
    CMU.CPVCR.x16 v12.l v0.0 || IAU.INCS col, -8
    PEU.PC1I GT || BRU.BRA __mainloopNN
    NOP
    CMU.CPIVR.x16 dest, color_value
    CMU.CPVCR.x16 v12.h v4.0
    CMU.VDILV.x8 vTMP7, dest_new, dest_new, dest_new
    CMU.CPVV.u8.u16 dest_new, dest_new
;------------------------------------------------------------------
    IAU.INCS out_height, -1  || LSU0.LDIL first_step, 1 || CMU.CPSVR.x16 vTMP0, sxstep_r    || SAU.ADD.u32s output, output, out_stride
    PEU.PC1I GT              || BRU.BRA __mainloopNN    || CMU.CPSVR.x16 vTMP1, systep_r
    CMU.CPII col, out_width                             || VAU.ADD.f16 xstart_r, xstart_r, vTMP0
                                                           VAU.ADD.f16 ystart_r, ystart_r, vTMP1
    NOP
    CMU.CPVV x, xstart_r
    CMU.CPVV y, ystart_r
;------------------------------------------------------------------
    ; Recover rounding mode.
    CMU.CPIT P_CFG, p_cfg_save
    ; fill with color or let as it is
    PEU.PVV16 GT  || VAU.AND dest, vNULL, dest
    PEU.PVV16 GT  || VAU.OR dest, dest, dest_new || IAU.SUB iTMP0, fill_color, 0
    PEU.PC1I NEQ  || CMU.CPIT C_CMU0, cmu0_gt
    PEU.PVL1S8 GT || LSU1.ST128.u16.u8 dest, output_aux

    BRU.JMP i30
    NOP 5

.end
