; ///
; /// @file
; /// @copyright All code copyright Movidius Ltd 2013, all rights reserved.
; ///            For License Warranty see: common/license.txt
; ///
; /// @brief
; ///

.version 00.50.00

;/******************************************************************************
; @File         : HarrisResponse.asm
; @Author       : Marius Truica
; @Brief        : Contains a function that applies the Haris operator. Hardcoded for kernel radius = 3
; Date          : 16 - 01 - 2013
; E-mail        : marius.truica@movidius.com
; Copyright     :  Movidius Srl 2013,  Movidius Ltd 2013
;
;HISTORY
; Version        | Date       | Owner         | Purpose
; ---------------------------------------------------------------------------
; 1.0            | 16.01.2013 | Marius Truica    | First implementation
; 1.1			 | 24.02.2013 | Ovidiu Vesa		 | Removed bounds checking code
;  ***************************************************************************

;==================================================================================================================
;=================================== HARRIS SCORE ================START========================
;==================================================================================================================
; Needs an 8x8 patch to account for borders. Note that the kernel is not symmetric around the point:
; b b b b b b b b
; b o o o o o o b
; b o o o o o o b
; b o o o o o o b
; b o o o X o o b
; b o o o o o o b
; b o o o o o o b
; b b b b b b b b
;
;fp32 HarrisResponse(u8 *data, u32 x, u32 y, u32 step_width, fp32 k = 0.02) 
;{
;    int dx;
;    int dy;
;
;    float xx = 0;
;    float xy = 0;
;    float yy = 0;
;
;    for (u32 r = y - RADIUS; r < y + RADIUS; r++)
;    {
;        for (u32 c = x - RADIUS; c < x + RADIUS; c++)
;        {
;            int index = r * step_width + c;
;            dx = data[index - 1] - data[index + 1];
;            dy = data[index - step_width] - data[index + step_width];
;            xx += dx * dx;
;            xy += dx * dy;
;            yy += dy * dy;
;        }
;    }
;
;    float det = xx * yy - xy * xy;
;    float trace = xx + yy;
;
;    //k changes the response of edges.
;    //seems sensitive to window size, line thickness, and image blur =o/
;    return (det - k * trace * trace);
;}
; ComputeHarrisResponse
; i18         :  unsigned char *inbuffer //input buffer pointer
; i17         :  u32 x,
; i16         :  u32 y
; i15         :  u32 step_width
; i103         :  fp32 k = 0.02

.set RADIUS 3

.code .text.MvCV_HarrisResponse
.salign 16

HarrisResponse_asm:
IAU.SUB i19 i19 8
LSU0.ST.64.l v24  i19  || IAU.SUB i19 i19 8
LSU0.ST.64.h v24  i19  

iau.sub i2, i17, RADIUS
iau.sub i3, i16, RADIUS
iau.mul i1, i3, i15
nop

iau.add i0, i1, i2
iau.add i0, i18, i0
LSU0.LDI.128.U8.U16 v0, i0, i15   
LSU0.LDI.128.U8.U16 v1, i0, i15
LSU0.LDI.128.U8.U16 v2, i0, i15
LSU0.LDI.128.U8.U16 v3, i0, i15
LSU0.LDI.128.U8.U16 v4, i0, i15
LSU0.LDI.128.U8.U16 v5, i0, i15  
nop
LSU0.LDI.128.U8.U16 v6, i0, i15  ||CMU.ALIGNVEC v0 V0, v0 2
cmu.cpvv v24 v1                    || LSU1.SWZC8 [00765432] 
LSU0.LDI.128.U8.U16 v7, i0, i15    ||vau.sub.i16 v12 , v1, v24        ||CMU.ALIGNVEC v1 V1, v1 2    
nop
cmu.cpvv v24 v2                    || LSU1.SWZC8 [00765432] 
vau.sub.i16 v13, v2, v24           ||CMU.ALIGNVEC v2 V2, v2 2    
cmu.cpvv v24 v3                    || LSU1.SWZC8 [00765432] 
vau.sub.i16 v14, v3, v24           ||CMU.ALIGNVEC v3 V3, v3 2    
cmu.cpvv v24 v4                    || LSU1.SWZC8 [00765432] 
vau.sub.i16 v15, v4, v24           ||CMU.ALIGNVEC v4 V4, v4 2  
cmu.cpvv v24 v5                    || LSU1.SWZC8 [00765432] 
vau.sub.i16 v16, v5, v24           ||CMU.ALIGNVEC v5 V5, v5 2
cmu.cpvv v24 v6                    || LSU1.SWZC8 [00765432] 
vau.sub.i16 v17, V6, v24           ||CMU.VSZM.WORD v12, v13 [0DDD]
vau.sub.i16 v0, v0, v2             ||CMU.VSZM.WORD v14, v15 [0DDD]

vau.sub.i16 v1, v1, v3                                          ||CMU.CPVV.i16.i32 v18, v12 ||LSU0.SWZMC4.WORD [3210] [UUUU]
vau.sub.i16 v2, v2, v4                                          ||CMU.VSZM.WORD v16, v17 [0DDD]
vau.sub.i16 v3, v3, v5                                          ||CMU.CPVV.i16.i32 v19, v12 ||LSU0.SWZMC4.WORD [1032] [UUUU]
cmu.cpvv v24 v6                 ||LSU1.SWZC8 [07654321] 
vau.sub.i16 v4, v4, v24          ||CMU.CPVV.i16.i32 v20, v13 ||LSU0.SWZMC4.WORD [0321] [UUUU]
cmu.cpvv v24 v7                 ||LSU1.SWZC8 [07654321] 
vau.sub.i16 v5, v5, v24          ||CMU.CPVV.i16.i32 v21, v14 ||LSU0.SWZMC4.WORD [3210] [UUUU]
VAU.MACPZ.i32 v18, v18          ||CMU.CPVV.i16.i32 v22, v14     ||LSU0.SWZMC4.WORD [1032] [UUUU]
VAU.MACP.i32 v19, v19           ||CMU.CPVV.i16.i32 v23, v15     ||LSU0.SWZMC4.WORD [0321] [UUUU]
VAU.MACP.i32 v20, v20           ||CMU.CPVV.i16.i32 v12, v16     ||LSU0.SWZMC4.WORD [3210] [UUUU]
VAU.MACP.i32 v21, v21           ||CMU.CPVV.i16.i32 v13, v16     ||LSU0.SWZMC4.WORD [1032] [UUUU]
VAU.MACP.i32 v22, v22           ||CMU.CPVV.i16.i32 v14, v17     ||LSU0.SWZMC4.WORD [0321] [UUUU]
VAU.MACP.i32 v23, v23           ||CMU.VSZM.WORD v0, v1 [0DDD]
VAU.MACP.i32 v12, v12           ||CMU.VSZM.WORD v2, v3 [0DDD]
VAU.MACP.i32 v13, v13           ||CMU.VSZM.WORD v4, v5 [0DDD]
VAU.MACPW.i32 v15, v14, v14     ||CMU.CPVV.i16.i32 v6 , v0      ||LSU0.SWZMC4.WORD [3210] [UUUU]

VAU.MACPZ.i32 v6, v6            ||CMU.CPVV.i16.i32 v7 , v0      ||LSU0.SWZMC4.WORD [1032] [UUUU]
VAU.MACP.i32 v7, v7             ||CMU.CPVV.i16.i32 v8 , v1      ||LSU0.SWZMC4.WORD [0321] [UUUU]
VAU.MACP.i32 v8, v8             ||CMU.CPVV.i16.i32 v9 , v2      ||LSU0.SWZMC4.WORD [3210] [UUUU]
VAU.MACP.i32 v9, v9             ||CMU.CPVV.i16.i32 v10, v2      ||LSU0.SWZMC4.WORD [1032] [UUUU]
VAU.MACP.i32 v10, v10           ||CMU.CPVV.i16.i32 v11, v3      ||LSU0.SWZMC4.WORD [0321] [UUUU]
VAU.MACP.i32 v11, v11           ||CMU.CPVV.i16.i32 v0 , v4      ||LSU0.SWZMC4.WORD [3210] [UUUU]
VAU.MACP.i32 v0, v0             ||CMU.CPVV.i16.i32 v1 , v4      ||LSU0.SWZMC4.WORD [1032] [UUUU]
VAU.MACP.i32 v1, v1             ||CMU.CPVV.i16.i32 v2 , v5      ||LSU0.SWZMC4.WORD [0321] [UUUU]
VAU.MACPW.i32 v4, v2, v2
VAU.MACPZ.i32 v6, v18
VAU.MACP.i32 v7, v19
VAU.MACP.i32 v8, v20            ||SAU.SUMx.i32 i0, v15 ; xx
VAU.MACP.i32 v9, v21
VAU.MACP.i32 v10, v22
VAU.MACP.i32 v11, v23           ||SAU.SUMx.i32 i1, v4 ; yy
VAU.MACP.i32 v0, v12            ||CMU.CPII.i32.f32 i9 i0
VAU.MACP.i32 v1, v13            ||CMU.CPII.i32.f32 i10 i1
VAU.MACPW.i32 v5, v2, v14
sau.add.f32 i7, i9, i10;xy
nop 2
SAU.SUMx.i32 i2, v5
sau.mul.f32 i11, i9, i10
CMU.CPII.i32.f32 i8 i2
sau.mul.f32 i7, i7, i7
sau.mul.f32 i12, i8, i8 
nop
sau.mul.f32 i7, i7, i14 ; 
sau.sub.f32 i13, i11, i12 
nop 2
sau.sub.f32 i18, i13, i7
nop 2

LSU0.LD.64.h  v24  i19 || IAU.ADD i19 i19 8    
LSU0.LD.64.l  v24  i19 || IAU.ADD i19 i19 8


BRU.JMP i30 
nop 6
;==================================================================================================================
;=================================== HARRIS SCORE ================ END ========================
;==================================================================================================================

.end
