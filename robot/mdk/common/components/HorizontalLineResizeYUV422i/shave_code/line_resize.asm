///   
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved. 
///            For License Warranty see: common/license.txt   
///
/// @brief     Shave common. 
/// 
 
.version 00.50.05

.include shared_types.incl

;This is the line width. In YUYV we need 2*MAXWIDTH, as may be seen below when allocating space
.set MAX_IN_WIDTH 1920
;864*2
.set MAX_OUT_WIDTH 1728

// 3D Mode
.set MODE_FRAME_PACKED       0x00
.set MODE_TOP_BOTTOM         0x06
.set MODE_SIDE_BY_SIDE_HALF  0x08
.set MODE_NONE_DETECTED      0xFF

.data .data.__CMX1__sect
.salign 16
    ;Input lines are padded with 5 pixel values since we will need to duplicate edge values 
    CMXFirstInputLine:
    .fill (MAX_IN_WIDTH*2+4*5), 1, 0x0
    
.data .data.__CMX2__sect
.salign 16
    CMXSecondInputLine:
    .fill (MAX_IN_WIDTH*2+4*5), 1, 0x0

.data .data.__CMX3__sect
.salign 16
    CMXThirdInputLine:
    .fill (MAX_IN_WIDTH*2+4*5), 1, 0x0

.data .data.HorizResizeGenData
.salign 16
    CMXFirstOutputLine:
    .fill (MAX_OUT_WIDTH*2), 1, 0x0
    CMXSecondOutputLine:
    .fill (MAX_OUT_WIDTH*2), 1, 0x0
    ;Indexes line will hold computed indexes for each pixel value
    indexes_line:
    .fill (MAX_OUT_WIDTH*4), 1, 0x0
    ;For Lanczos resize we also need a buffer to hold multiply coeficients for each
    ;color value. It will hold out_width Y values, and anotuer out_width for U and V
    ;values. And, of course since we're using 6 values filters, we'll have 6 of each.
    ;And they are 2 bytes values -> fp16
    precalc_filter_vals:
    .fill (MAX_OUT_WIDTH*2*6*2), 1, 0x0
    ;This brings total requrements for the data section to well over 64Kb.
    ;So, if the linker script is checked, it may be observed that the data window
    ;is set to 96 Kb and code section to 32 Kb
    cif_dma_done_adr:
    .int 0
    current_cif_buffer_adr:
    .int 0
    force_shave_graceful_halt:
    .int 0
    apply_side_by_side:
    .int 0

.code .text.OneLineResize
.salign 16
// Outputs nothing
// Input: i18 - line output address, i17 - output stride, i16 - in width, i15 - out width, i14 - in_out height, i13 - second_line and forwards
    OneLineResize:
    ;Set rounding mode to nearest. Safest rounding mode.
    lsu0.ldil i1, 0x2 || lsu1.ldil i0, 0xFFE1 
    cmu.cpti i2, P_CFG || lsu0.ldih i0, 0xFFFF
    ;Clear all rounding bits
    ;iau.and i2, i0, i2
    ;Set our desired bits
    iau.or i2, i1, i2
    cmu.cpit P_CFG, i2
 
    ;!!!!!!!!!!!!!!!!Don't forget to enable the register PUSH and POP's in case of integrating this with 
    ;compiler code !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

    ;--------------------------------------Filling in index line part--------------------------------
    ;Horizontal resize is done using Lanczsos resize 
    ;First off, while the CIF bring in it's first line, we compute the indexes needed to resize for
    ;one line and store them into indexes_line
    ;Bare in mind that we are working with YUYV422i data
    ;Start off by computing our resize ratio for Y
    iau.sub i0, i16, 1 || sau.sub.i32 i1, i15, 1
    ;                       || continue with the resize ratio for chromas
    cmu.cpis.i32.f32 s0, i0 || iau.shr.u32 i0, i16, 1 || sau.shr.u32 i2, i15, 1
    cmu.cpis.i32.f32 s1, i1 || iau.sub i0, i0, 1
    cmu.cpis.i32.f32 s3, i0 || iau.sub i2, i2, 1
    ;Compute (outwidth-1)/(inwidth-1) -> the resize ratio for the Y(luma) values
    sau.div.f32 s2, s0, s1 || cmu.cpis.i32.f32 s4, i2
    ;Zero the plane offsets|| Initialize 128.0 float value in i6
    vau.xor v3, v3, v3     || lsu0.ldil i6, 127.0F32 || lsu1.ldih i6, 127.0F32
    ;Ratio for chromas     || Zero the plane offsets
    sau.div.f32 s5, s3, s4 || vau.xor v4, v4, v4 || lsu0.ldil i0, 0
    cmu.cpis s6, i0 || lsu0.ldil i1, 1
    cmu.cpis s7, i0 || lsu0.ldil i2, 2
    cmu.cpis s8, i0 || lsu0.ldil i3, 3
    cmu.cpiv v5.0, i0
    ; Position offset 0 too since for lanczos we load U and V at the same time.
    cmu.cpiv v5.1, i0
    cmu.cpiv v6.1, i2
    cmu.cpiv v6.0, i1
    cmu.cpiv v5.2, i0
    cmu.cpiv v5.3, i0
    ;Setup ratio vector which will be used to compute the indexes inside one plane
    cmu.cpsv.x32 v0.0, s2 || sau.add.f32 s6, s6, s2 || vau.swzword v6, v6 [1010]
    cmu.cpsv.x32 v0.2, s2; || vau.add.i32 v9, v5, 4
    cmu.cpsv.x32 v0.1, s5
    cmu.cpsv.x32 v0.3, s5 || sau.add.f32 s7, s7, s5
    ;Set up initial offsets
    cmu.cpsv.x32 v3.2, s6 || sau.add.f32 s6, s6, s2 || vau.mul.i32 v6, v6, 2
    ;Copy 127.0f to the 127.0 float VRF, v11
    cmu.cpiv v11.0, i6
    cmu.cpsv.x32 v4.1, s7
    cmu.cpsv.x32 v4.3, s7 || sau.add.f32 s6, s6, s2 || vau.mul.f32 v0, v0, 2.0
    cmu.cpsv.x32 v4.0, s6
    nop
    cmu.cpsv.x32 v4.2, s6 || vau.swzword v11, v11 [0000]
    ;                 || Y values are computed 4 at a time so we need to multiply another time 
    peu.pven4word 0x5 || vau.mul.f32 v0, v0, 2.0
    ;Load precalc_vals starting store point
    lsu0.ldil i6, precalc_filter_vals || lsu1.ldih i6, precalc_filter_vals
    nop
    ;We have the ratio's set and the initial plane offsets set here
    ;Add 0.5 in order to have cmu.cpsv.f32.i32s actually round to nearest instead of truncating
    cmu.cpvv v9, v3; || vau.add.f32 v3, v3, 0.5
    cmu.cpvv v10, v4; || vau.add.f32 v4, v4, 0.5
    nop
    ;Find out how many 4 pixels batches must be processed
    iau.shr.u32 i4, i15, 2 || lsu0.ldil i5, indexes_line || lsu1.ldih i5, indexes_line
___Fill4Pixelindexes:
    ;Get integer plane offsets || Get fractional part to determine best precalculated position to use
    cmu.cpvv.f32.i32s v7, v3   || vau.frac.f32 v9, v9 
    cmu.cpvv.f32.i32s v8, v4   || vau.frac.f32 v10, v10
    lsu0.ldil i0, precalc_filters || lsu1.ldih i0, precalc_filters
    ;Multiply with 128.0f to get desired precalculated position
    vau.mul.f32 v9, v9, v11 || cmu.cpiv v12.0, i0
    vau.mul.f32 v10, v10, v11
    ;Create integer offset with all positions to find exact filter values
    cmu.vszmword v12, v12, [0000]
    ;Convert to integer values
    cmu.cpvv.f32.i32s v9, v9
    cmu.cpvv.f32.i32s v10, v10
    ;Multiply with 6*2 because this is how much memory a precalculated filter takes
    vau.mul.i32 v9, v9, 12
    vau.mul.i32 v10,v10, 12
    nop
    ;-----------------DEBUG------------------------
    ;Inject slowness
    ;nop 50
    ;nop 50
    ;nop 50
    ;nop 50
    ;nop 50
    ;nop 50
    ;nop 50
    ;-----------------DEBUG------------------------
    ;Add precalc_filters offsets
    vau.add.i32 v9, v9, v12
    vau.add.i32 v10, v10, v12
    ;Multiply with their positions || Start moving addresses to i registers in order to load precalculated filters
    vau.mul.i32 v7, v7, v6         || cmu.cpvi i20, v9.0
    vau.mul.i32 v8, v8, v6         || cmu.cpvi i21, v9.1 || lsu0.ldo64.l v13, i20, 0 || lsu1.ldo64.h v13, i20, 8
    ;And compute next float offsets at the same time
    vau.add.f32 v3, v3, v0         || cmu.cpvi i22, v9.2 || lsu0.ldo64.l v14, i21, 0 || lsu1.ldo64.h v14, i21, 8
    cmu.cpvi i23, v9.3 || lsu0.ldo64.l v15, i22, 0 || lsu1.ldo64.h v15, i22, 8
    vau.add.i32 v7, v7, v5 || cmu.cpvi i24, v10.0 || lsu0.ldo64.l v16, i23, 0 || lsu1.ldo64.h v16, i23, 8
    vau.add.i32 v8, v8, v5 || cmu.cpvi i25, v10.1 || lsu0.ldo64.l v17, i24, 0 || lsu1.ldo64.h v17, i24, 8
    vau.add.f32 v4, v4, v0 || cmu.cpvi i26, v10.2 || lsu0.ldo64.l v18, i25, 0 || lsu1.ldo64.h v18, i25, 8
    ;Store first 2 pixels offsets
    lsu0.st128.u32.u16 v7, i5 || iau.add i5, i5, 8 || cmu.cpvi i27, v10.3 || lsu1.ldo64.l v19, i26, 0
    ;Store last 2 pixels offsets
    lsu0.st128.u32.u16 v8, i5 || iau.add i5, i5, 8 || lsu1.ldo64.h v19, i26, 8
    ;                                                     || 4 pixels batch ended
    lsu0.ldo64.l v20, i27, 0 || lsu1.ldo64.h v20, i27, 8 || iau.sub i4, i4, 1
    ;Start storing precalculated filter values. Load only 6 fp16s (12 bytes).
    peu.pl1en4word 0x3 || lsu1.sto64.h v13, i6, 8 || lsu0.sto64.l v13, i6, 0 || iau.add i6, i6, 12 || vau.sub.f32 v9, v3, 0.5
    peu.pl1en4word 0x3 || lsu1.sto64.h v14, i6, 8 || lsu0.sto64.l v14, i6, 0 || iau.add i6, i6, 12 || vau.sub.f32 v10, v4, 0.5
    peu.pl1en4word 0x3 || lsu1.sto64.h v15, i6, 8 || lsu0.sto64.l v15, i6, 0 || iau.add i6, i6, 12 || cmu.cmz.i32 i4
    peu.pc1c 0x2 || bru.bra ___Fill4Pixelindexes
    peu.pl1en4word 0x3 || lsu1.sto64.h v16, i6, 8 || lsu0.sto64.l v16, i6, 0 || iau.add i6, i6, 12 || vau.add.f32 v9, v9, 0.5
    peu.pl1en4word 0x3 || lsu1.sto64.h v17, i6, 8 || lsu0.sto64.l v17, i6, 0 || iau.add i6, i6, 12 || vau.add.f32 v10, v10, 0.5
    peu.pl1en4word 0x3 || lsu1.sto64.h v18, i6, 8 || lsu0.sto64.l v18, i6, 0 || iau.add i6, i6, 12
    peu.pl1en4word 0x3 || lsu1.sto64.h v19, i6, 8 || lsu0.st64.l v19, i6     || iau.add i6, i6, 12
    peu.pl1en4word 0x3 || lsu1.sto64.h v20, i6, 8 || lsu0.st64.l v20, i6     || iau.add i6, i6, 12
        
    ;--------------------------------------End of filling in index line part--------------------------------
    
    ;Set rounding mode to nearest. Safest rounding mode.
    lsu0.ldil i1, 0x2 || lsu1.ldil i0, 0xFFE1 
    cmu.cpti i2, P_CFG || lsu0.ldih i0, 0xFFFF
    ;Clear all rounding bits
    iau.and i2, i0, i2
    ;Set our desired bits
    iau.or i2, i1, i2
    cmu.cpit P_CFG, i2
    
    ;Some setup phase. Load i4 with the address of the cif_dma_done variable
    lsu0.ldil i0, cif_dma_done_adr || lsu1.ldih i0, cif_dma_done_adr
    lsu0.ld32 i4, i0
    ;Load i7 with the pointer for the first line that is going to be ready
    lsu0.ldih i9, CMXFirstOutputLine || lsu1.ldil i9, CMXFirstOutputLine
    ;Load i8 with the pointer to the line being worked on
    lsu0.ldih i10, CMXSecondOutputLine || lsu1.ldil i10, CMXSecondOutputLine
    lsu0.ldil i11, current_cif_buffer_adr || lsu1.ldih i11, current_cif_buffer_adr
    ;Clean my needed 8 registers
    iau.xor i20, i20, i20 || sau.xor i21, i21, i21 || lsu0.ldih i0, force_shave_graceful_halt || lsu1.ldil i0, force_shave_graceful_halt
    ;                                              || Load gracefull halt address into i28
    iau.xor i22, i22, i22 || sau.xor i23, i23, i23 || lsu0.ld32 i28, i0
    iau.xor i24, i24, i24 || sau.xor i25, i25, i25
    iau.xor i26, i26, i26 || sau.xor i27, i27, i27
    
    ;i4 now holds the cif_dma_done address and may be used to syncronize
    
    ;---------------Start of loop processing one line-----------------------------
    iau.xor i5, i5, i5
    ___OneLineProcess:
    
    ;Firstly, wait for the Leon to signal that a line has been finished
    ___WaitCIFSync:
    lsu0.ld32 i0, i4 || lsu1.ldil i1, 1
    nop
    nop
    ;I have the address of graceful halt at this point in i28. Let's check if it is 1.
    ;If so, then branch to shave finish
    lsu0.ld32 i2, i28
    nop
    nop
    cmu.cmii.u32 i0, i1
    ;if cif_dma_done is 0 then wait some more
    peu.pc1c 0x6 || bru.bra ___WaitCIFSync
    nop
    ;Compare to see if graceful halt is 0
    cmu.cmii.u32 i2, i1 || lsu0.ldil i3, ___OneLineResizeFinish || lsu1.ldih i3, ___OneLineResizeFinish
    peu.pc1c 0x1 || bru.jmp i3
    nop
    nop
    ;3 more to compensate for last jump 
    nop
    nop
    nop
    
    ;Then reset cif_dma_done to 0
    lsu0.ldil i0, 0 || lsu1.ldil i1, 0x00FF
    lsu0.st32 i0, i4 || cmu.vszmbyte i1, i1, [1010]
    
    ;Swap output buffers
    cmu.cpii i9, i10 || iau.add i10, i9, 0
    
    ;Process the received line
    ;Find out where was it stored
    lsu0.ld32 i8, i11 || cmu.cpiv v25.0, i1
    ;                                                          || prepare mask for u16->f16 conversion
    lsu0.ldil i12, indexes_line || lsu1.ldih i12, indexes_line || vau.swzword v25, v25, [0000]
    ;                                                 || Load index start for precalculated values
    lsu0.ld128.u16.u32 v1, i12 || iau.add i12, i12, 8 || lsu1.ldil i29, precalc_filter_vals
    lsu0.ld128.u16.u32 v2, i12 || iau.add i12, i12, 8 || lsu1.ldih i29, precalc_filter_vals
    lsu0.ld128.u16.u32 v3, i12 || iau.add i12, i12, 8
    lsu0.ld128.u16.u32 v4, i12 || iau.add i12, i12, 8
    ;Fill in one V register with the starting point of the buffer
    cmu.cpiv v0.0, i8
    vau.swzword v0, v0, [0000]
    peu.pven4word 0x5 || vau.add.i32 v0, v0, 4
    
    ;We'll try processing batches of 8 pixels for now
    ;See how many batches of 8 pixels are there in the output
    iau.shr.u32 i6, i15, 3
    cmu.cpii i7, i9 || iau.shl i20, i16, 1 
    ;We need to mirror first 2 values and last 3 values
    lsu0.ldo32 i22, i8, 8 || iau.add i20, i20, 8
    ;Get address of last pixel position that need to be mirrored
    iau.add i21, i20, i8
    iau.sub i21, i21, 4
    ;Read last pixel value too
    lsu0.ld32 i23, i21 || vau.xor v5, v5 v5
    ;Zero last element of V25 because we only use 6 filters
    cmu.vszmword v25, v25, [Z210] || vau.xor v6, v6, v6
    vau.xor v7, v7, v7 || cmu.vszmword v26, v25, [2210]
    vau.xor v8, v8, v8
    ;Swizzle bytes
    cmu.vszmbyte i22, i22, [3010]
    lsu0.sto32 i22, i8, 0 || lsu1.sto32 i22, i8, 4
    ;Swizzle bytes for last position
    cmu.vszmbyte i23, i23, [3212]
    lsu0.sto32 i23, i21, 4 || lsu1.sto32 i23, i21, 8
    lsu0.sto32 i23, i21, 12
___Copy_start:
    vau.add.i32 v1, v1, v0
    vau.add.i32 v2, v2, v0
    vau.add.i32 v3, v3, v0 || cmu.cpvi.x32 i0, v1.0
    ;                                               || load pixel values for Y0
    vau.add.i32 v4, v4, v0 || cmu.cpvi.x32 i1, v1.2 || lsu0.ldo64.l v9, i0, 0 || lsu1.ldo64.h v9, i0, 8
    ;Load pixel values for Y1                       || Get offsets for Y2
    lsu0.ldo64.l v11, i1, 0 || lsu1.ldo64.h v11, i1, 8 || cmu.cpvi.x32 i2, v2.0
    ;Load pixel values for Y2                          || Get offsets for Y3
    lsu0.ldo64.l v13, i2, 0 || lsu1.ldo64.h v13, i2, 8 || cmu.cpvi.x32 i3, v2.2
    ;Load pixel values for Y3                          || Get offsets for Y4
    lsu0.ldo64.l v15, i3, 0 || lsu1.ldo64.h v15, i3, 8 || cmu.cpvi.x32 i20, v3.0
    ;Load pixel values for Y4                            || Get offsets for Y5
    lsu0.ldo64.l v10, i20, 0 || lsu1.ldo64.h v10, i20, 8 || cmu.cpvi.x32 i21, v3.2
    ;Load pixel values for Y5                            || Get offsets for Y6
    lsu0.ldo64.l v12, i21, 0 || lsu1.ldo64.h v12, i21, 8 || cmu.cpvi.x32 i22, v4.0
    ;Load pixel values for Y6                            || Get offsets for Y7     || And with a mask to prepare f16 conversion
    lsu0.ldo64.l v14, i22, 0 || lsu1.ldo64.h v14, i22, 8 || cmu.cpvi.x32 i23, v4.2 || vau.and v9, v9, v25
    ;Load pixel values for Y7
    lsu0.ldo64.l v16, i23, 0 || lsu1.ldo64.h v16, i23, 8 || vau.and v11, v11, v25 || cmu.cpvv.i16.f16 v9, v9
    ;                                                  ||And start loading the Y0 output precalculated filter values for multiplication
    vau.and v13, v13, v25 || cmu.cpvv.i16.f16 v11, v11 || lsu0.ldo64.l v17, i29, (6*2*0+0)  || lsu1.ldo64.h v17, i29, (6*2*0+8)
    vau.and v15, v15, v25 || cmu.cpvv.i16.f16 v13, v13 || lsu0.ldo64.l v19, i29, (6*2*2+0)  || lsu1.ldo64.h v19, i29, (6*2*2+8)
    vau.and v10, v10, v25 || cmu.cpvv.i16.f16 v15, v15 || lsu0.ldo64.l v21, i29, (6*2*4+0)  || lsu1.ldo64.h v21, i29, (6*2*4+8)
    vau.and v12, v12, v25 || cmu.cpvv.i16.f16 v10, v10 || lsu0.ldo64.l v23, i29, (6*2*6+0)  || lsu1.ldo64.h v23, i29, (6*2*6+8)
    vau.and v14, v14, v25 || cmu.cpvv.i16.f16 v12, v12 || lsu0.ldo64.l v18, i29, (6*2*8+0)  || lsu1.ldo64.h v18, i29, (6*2*8+8)
    vau.and v16, v16, v25 || cmu.cpvv.i16.f16 v14, v14 || lsu0.ldo64.l v20, i29, (6*2*10+0) || lsu1.ldo64.h v20, i29, (6*2*10+8)
    ;Start multiplying Y values with the preselected lanczos filters || continue with the rest of operations
    vau.mul.f16 v9, v9, v17                                          || cmu.cpvv.i16.f16 v16, v16 || lsu0.ldo64.l v22, i29, (6*2*12+0) || lsu1.ldo64.h v22, i29, (6*2*12+8)
    vau.mul.f16 v11, v11, v19 || lsu0.ldo64.l v24, i29, (6*2*14+0) || lsu1.ldo64.h v24, i29, (6*2*14+8)
    ;Start working on Chroma values. Not such a trivial matter because 6 chroma values span over 24 bytes
    ;This means we need 2 vrf loads just to get the needed values for 1 U element or 1 V element
    ;The good news though is that V and U have the same ratio therefore, they will be spatially localised
    ;To the same input area which means we can extract both U and V from the same loads
    ;                         || Start loading U value for U0 out
    vau.mul.f16 v13, v13, v21 || cmu.cpvi.x32 i0, v1.1
    ;                         || Sum lanczos values      || take address for V0 out coefs || Load values for U0. first half 
    vau.mul.f16 v15, v15, v23 || sau.sum.f16 s9, v9, 0xF || cmu.cpvi.x32 i1, v2.1         || lsu0.ldo64.l v10,  i0, 0 || lsu1.ldo64.h v10,  i0, 8
    ;                                                                                     || Load values for V0. first half. etc. I'm not commenting anymore
    vau.mul.f16 v10, v10, v18 || sau.sum.f16 s11, v11, 0xF || cmu.cpvi.x32 i2, v3.1       || lsu0.ldo64.l v12,  i1, 0 || lsu1.ldo64.h v12,  i1, 8 
    vau.mul.f16 v12, v12, v20 || sau.sum.f16 s13, v13, 0xF || cmu.cpvi.x32 i3, v4.1       || lsu0.ldo64.l v14,  i2, 0 || lsu1.ldo64.h v14,  i2, 8
    vau.mul.f16 v14, v14, v22 || sau.sum.f16 s15, v15, 0xF || lsu0.ldo64.l v16, i3, 0 || lsu1.ldo64.h v16, i3, 8
    ;                                                         Start loading second halfs of the chroma needed values
    vau.mul.f16 v16, v16, v24 || sau.sum.f16 s10, v10, 0xF || lsu0.ldo64.l v9, i0, 16 || lsu1.ldo64.l v11, i1, 16
    ;                         || Y0 out
    sau.sum.f16 s12, v12, 0xF || cmu.cpsv.x16 v5.0, s9.l     || lsu0.ldo64.l v13, i2, 16 || lsu1.ldo64.l v15, i3, 16
    ;                         || Y1 out                      || Start loading Lanczos coeficients for U and V. Start with V0
    sau.sum.f16 s14, v14, 0xF || cmu.cpsv.x16 v5.2, s11.l    || lsu0.ldo64.l v17, i29, (6*2*3+0) || lsu1.ldo64.h v17, i29, (6*2*3+8)
    ;                         || Y2 out                      || Apply first byte swizzle. Refer to sheet "U-V setup" from OneLineRegisterMap.xls document in doc folder
    sau.sum.f16 s16, v16, 0xF || cmu.cpsv.x16 v5.4, s13.l    || vau.swzbyte v10, v10, [2031] || lsu0.ldo64.l v18, i29, (6*2*1+0) || lsu1.ldo64.h v18, i29, (6*2*1+8)
    ;Y3 out
    cmu.cpsv.x16 v5.6, s15.l    || vau.swzbyte v12, v12, [2031] || lsu0.ldo64.l v19, i29, (6*2*7+0)  || lsu1.ldo64.h v19, i29, (6*2*7+8)
    ;Y4 out
    cmu.cpsv.x16 v6.0, s10.l    || vau.swzbyte v14, v14, [2031] || lsu0.ldo64.l v20, i29, (6*2*5+0)  || lsu1.ldo64.h v20, i29, (6*2*5+8)
    ;Y5 out
    cmu.cpsv.x16 v6.2, s12.l    || vau.swzbyte v16, v16, [2031] || lsu0.ldo64.l v21, i29, (6*2*11+0)  || lsu1.ldo64.h v21, i29, (6*2*11+8)
    ;Y6 out
    cmu.cpsv.x16 v6.4, s14.l    || vau.swzbyte v9, v9, [2031]   || lsu0.ldo64.l v22, i29, (6*2*9+0)  || lsu1.ldo64.h v22, i29, (6*2*9+8)
    ;Y7 out
    cmu.cpsv.x16 v6.6, s16.l    || vau.swzbyte v11, v11, [2031] || lsu0.ldo64.l v23, i29, (6*2*15+0) || lsu1.ldo64.h v23, i29, (6*2*15+8)
    vau.swzbyte v13, v13, [2031]  || lsu0.ldo64.l v24, i29, (6*2*13+0) || lsu1.ldo64.h v24, i29, (6*2*13+8)
    ;                            || No need for this precalc_filters_val anymore so we can upgrade now
    vau.swzbyte v15, v15, [2031] || lsu0.ldil i0, (6*2*16)
    ;Apply second stage of swizzling
    lsu0.swzv8 [75316420] || vau.swzword v10, v10, [3210] || iau.add i29, i29, i0
    lsu0.swzv8 [75316420] || vau.swzword v12, v12, [3210]
    lsu0.swzv8 [75316420] || vau.swzword v14, v14, [3210]
    lsu0.swzv8 [75316420] || vau.swzword v16, v16, [3210]
    lsu0.swzv8 [75316420] || vau.swzword  v9,  v9, [3210]
    lsu0.swzv8 [75316420] || vau.swzword v11, v11, [3210]
    lsu0.swzv8 [75316420] || vau.swzword v13, v13, [3210]
    lsu0.swzv8 [75316420] || vau.swzword v15, v15, [3210]
    ;Step 3, combining chroma elements
    peu.pven4word 0x4 || lsu0.swz4v [3210] [3012] [PPPP] || vau.swzword v10,  v9, [3210]
    peu.pven4word 0x4 || lsu0.swz4v [3210] [3012] [PPPP] || vau.swzword v12, v11, [3210]
    peu.pven4word 0x4 || lsu0.swz4v [3210] [3012] [PPPP] || vau.swzword v14, v13, [3210]
    peu.pven4word 0x4 || lsu0.swz4v [3210] [3012] [PPPP] || vau.swzword v16, v15, [3210]
    ;Step 4, take V elements into separate vectors
    vau.swzbyte  v9, v10, [2301]
    vau.swzbyte v11, v12, [2301]
    vau.swzbyte v13, v14, [2301]
    vau.swzbyte v15, v16, [2301]
    ;And we're almost done, only thing left: multiply with the same mask Y values were multiplied
    ;in order to get all unneeded data zeroed
    vau.and  v9,  v9, v25
    ;                     || Start converting to fp16
    vau.and v10, v10, v25 || cmu.cpvv.i16.f16 v9, v9
    vau.and v11, v11, v25 || cmu.cpvv.i16.f16 v10, v10
    vau.and v12, v12, v25 || cmu.cpvv.i16.f16 v11, v11
    vau.and v13, v13, v25 || cmu.cpvv.i16.f16 v12, v12
    vau.and v14, v14, v25 || cmu.cpvv.i16.f16 v13, v13
    vau.and v15, v15, v25 || cmu.cpvv.i16.f16 v14, v14
    vau.and v16, v16, v25 || cmu.cpvv.i16.f16 v15, v15
    cmu.cpvv.i16.f16 v16, v16 || vau.mul.f16 v9, v9, v17
    vau.mul.f16 v10, v10, v18
    vau.mul.f16 v11, v11, v19
    vau.mul.f16 v12, v12, v20 || sau.sum.f16  s9,  v9, 0xF
    vau.mul.f16 v13, v13, v21 || sau.sum.f16 s10, v10, 0xF
    vau.mul.f16 v14, v14, v22 || sau.sum.f16 s11, v11, 0xF
    vau.mul.f16 v15, v15, v23 || sau.sum.f16 s12, v12, 0xF
    vau.mul.f16 v16, v16, v24 || sau.sum.f16 s13, v13, 0xF
    ;                         || And start moving results to the final line. V0 out
    sau.sum.f16 s14, v14, 0xF || cmu.cpsv.x16 v5.3, s9.l
    ;                         || U0 out
    sau.sum.f16 s15, v15, 0xF || cmu.cpsv.x16 v5.1, s10.l
    ;                         || V1 out
    sau.sum.f16 s16, v16, 0xF || cmu.cpsv.x16 v5.7, s11.l
    ;U1 out
    cmu.cpsv.x16 v5.5, s12.l
    ;V2 out
    cmu.cpsv.x16 v6.3, s13.l
    ;U2 out
    cmu.cpsv.x16 v6.1, s14.l
    ;V3 out
    cmu.cpsv.x16 v6.7, s15.l
    ;U3 out
    cmu.cpsv.x16 v6.5, s16.l
    ;Convert to fp16
    cmu.cpvv.f16.i16s v5, v5
    cmu.cpvv.f16.i16s v6, v6
    ;Clamp results to 0..255
    cmu.clamp0.i16 v5, v5, v26
    cmu.clamp0.i16 v6, v6, v26
    ;-------------------------DEBUG ONLY!----------------------------------
    ;Inject slowness to test resilience
    ;Zero chromas for debug
    ;lsu0.ldil i0, 0xFFFF || lsu1.ldih i0, 0x0000
    ;cmu.cpiv v31.0, i0 || lsu0.ldil i0, 0x0000 || lsu1.ldih i0, 0x0080
    ;vau.swzword v31, v31, [0000] || cmu.cpiv v30.0 ,i0
    ;vau.swzword v30, v30, [0000]
    ;vau.and v5, v5, v31
    ;vau.and v6, v6, v31
    ;vau.or v5, v5, v30
    ;vau.or v6, v6, v30
    ;Cut Y0
    ;lsu0.ldil i0, 0x0000 || lsu1.ldih i0, 0xFFFF
    ;cmu.cpiv v31.0, i0 || lsu0.ldil i0, 0x0080 || lsu1.ldih i0, 0x0000 
    ;cmu.cpiv v30.0, i0 || vau.swzword v31, v31, [0000]
    ;vau.swzword v30, v30, [0000]
    ;peu.pven4word 0x1 || vau.and v5, v5, v31
    ;peu.pven4word 0x1 || vau.or  v5, v5, v30
    ;;Cut Y1
    ;peu.pven4word 0x2 || vau.and v5, v5, v31
    ;peu.pven4word 0x2 || vau.or  v5, v5, v30
    ;;Cut Y2
    ;peu.pven4word 0x4 || vau.and v5, v5, v31
    ;peu.pven4word 0x4 || vau.or  v5, v5, v30
    ;;Cut Y3
    ;peu.pven4word 0x8 || vau.and v5, v5, v31
    ;peu.pven4word 0x8 || vau.or  v5, v5, v30
    ;;Cut Y4
    ;peu.pven4word 0x1 || vau.and v6, v6, v31
    ;peu.pven4word 0x1 || vau.or  v6, v6, v30
    ;;Cut Y5
    ;peu.pven4word 0x2 || vau.and v6, v6, v31
    ;peu.pven4word 0x2 || vau.or  v6, v6, v30
    ;;Cut Y6
    ;peu.pven4word 0x4 || vau.and v6, v6, v31
    ;peu.pven4word 0x4 || vau.or  v6, v6, v30
    ;;Cut Y7
    ;peu.pven4word 0x8 || vau.and v6, v6, v31
    ;peu.pven4word 0x8 || vau.or  v6, v6, v30
    
    ;nop 50
    ;nop 50
    ;nop 50
    ;-------------------------DEBUG ONLY!----------------------------------
    lsu0.ld128.u16.u32 v1, i12 || iau.sub i6, i6, 1
    ;                                     || Increment here to be able to use peu.pc1i
    peu.pc1i 0x2 || bru.bra ___Copy_start || iau.add i12, i12, 8
    lsu0.ld128.u16.u32 v2, i12 || iau.add i12, i12, 8
    lsu0.ld128.u16.u32 v3, i12 || iau.add i12, i12, 8
    lsu0.ld128.u16.u32 v4, i12 || iau.add i12, i12, 8
    lsu0.st128.u16.u8 v5, i7  || iau.add i7, i7, 8 || vau.xor v5, v5, v5
    lsu0.st128.u16.u8 v6, i7  || iau.add i7, i7, 8 || vau.xor v6, v6, v6
    
    ;-----------------DEBUG ONLY----------------
    ;nop 3
    ;Fill in current line with a marker
    ;lsu0.ldil i0, (128*2*1+12)
    ;iau.add i0, i9, i0
    ;lsu0.sto32 i30, i0, (4*0) || lsu1.sto32 i30, i0, (4*1)
    ;lsu0.sto32 i30, i0, (4*2) || lsu1.sto32 i30, i0, (4*3)
    
    ;lsu0.ldil i0, (128*2*2+12)
    ;iau.add i0, i9, i0
    ;lsu0.sto32 i30, i0, (4*0) || lsu1.sto32 i30, i0, (4*1)
    ;lsu0.sto32 i30, i0, (4*2) || lsu1.sto32 i30, i0, (4*3)
    
    ;lsu0.ldil i0, (128*2*3+12)
    ;iau.add i0, i9, i0
    ;lsu0.sto32 i30, i0, (4*0) || lsu1.sto32 i30, i0, (4*1)
    ;lsu0.sto32 i30, i0, (4*2) || lsu1.sto32 i30, i0, (4*3)
    
    ;lsu0.ldil i0, (128*2*4+12)
    ;iau.add i0, i9, i0
    ;lsu0.sto32 i30, i0, (4*0) || lsu1.sto32 i30, i0, (4*1)
    ;lsu0.sto32 i30, i0, (4*2) || lsu1.sto32 i30, i0, (4*3)
    
    ;lsu0.ldil i0, (128*2*5+12)
    ;iau.add i0, i9, i0
    ;lsu0.sto32 i30, i0, (4*0) || lsu1.sto32 i30, i0, (4*1)
    ;lsu0.sto32 i30, i0, (4*2) || lsu1.sto32 i30, i0, (4*3)
    
    ;lsu0.ldil i0, (128*2*6+12)
    ;iau.add i0, i9, i0
    ;lsu0.sto32 i30, i0, (4*0) || lsu1.sto32 i30, i0, (4*1)
    ;lsu0.sto32 i30, i0, (4*2) || lsu1.sto32 i30, i0, (4*3)
    
    ;lsu0.ldil i0, (128*2*7+12)
    ;iau.add i0, i9, i0
    ;lsu0.sto32 i30, i0, (4*0) || lsu1.sto32 i30, i0, (4*1)
    ;lsu0.sto32 i30, i0, (4*2) || lsu1.sto32 i30, i0, (4*3)
    
    ;lsu0.ldil i0, (128*2*8+12)
    ;iau.add i0, i9, i0
    ;lsu0.sto32 i30, i0, (4*0) || lsu1.sto32 i30, i0, (4*1)
    ;lsu0.sto32 i30, i0, (4*2) || lsu1.sto32 i30, i0, (4*3)
    
    ;lsu0.ldil i0, (128*2*9+12)
    ;iau.add i0, i9, i0
    ;lsu0.sto32 i30, i0, (4*0) || lsu1.sto32 i30, i0, (4*1)
    ;lsu0.sto32 i30, i0, (4*2) || lsu1.sto32 i30, i0, (4*3)
    
    ;cmu.cpis s18, i0
    ;Store indexes for position 128 (first broken)
    ;lsu0.ldil i0, (128*2)
    ;lsu0.ldil i1, indexes_line || lsu1.ldih i1, indexes_line
    ;iau.add i0, i1, i0
    ;lsu0.ldo64.l v31, i0, 0 || lsu1.ldo64.h v31, i0, 8
    ;cmu.cpis s19, i0
    ;cmu.cpis s20, i1
    ;-----------------DEBUG ONLY----------------
    
    ;--------------Setup and start DMAs to stream to resized buffer
    ;Source of DMA transfer
    cmu.cpii i0, i9
    ;lsu0.ldil i0, CMXFirstOutputLine || lsu1.ldih i0, CMXFirstOutputLine
    ;                                         || Load configuration
    lsu0.sta32 i0, SLICE_LOCAL, DMA0_SRC_ADDR || lsu1.ldil i1, 0x1 || iau.shl i3, i15, 1
    lsu0.sta32 i1, SLICE_LOCAL, DMA0_CFG || lsu1.sta32 i3, SLICE_LOCAL, DMA0_SIZE
    ;Set destination address                   || increase destination address
    lsu0.sta32 i18, SLICE_LOCAL, DMA0_DST_ADDR || iau.add i18, i18, i3 || lsu1.ldil i0, 0x0
    ;fire DMA0 task
    lsu0.sta32 i0, SLICE_LOCAL, DMA_TASK_REQ || iau.shl i3, i17, 1
    ;Add stride to i18
    iau.add i18, i18, i3
    
    ;Just for safety check that the DMA was ready
;    lsu1.ldil i2, 0x4 || lsu0.ldil i1, 0x0
;___CheckDMAReady:
;    iau.or i2, i1, i2
;    cmu.cmii.u32 i2, i1
;    peu.pc1c 0x6 || bru.bra ___CheckDMAReady || lsu0.lda32 i2, SLICE_LOCAL, DMA_TASK_STAT
;    nop
;    nop
;    nop
;    nop
;    nop
    
    ;--------------End of setup
    iau.add i5, i5, 1
    cmu.cmii.u32 i5, i14 || lsu0.ldil i0, 1
    peu.pc1c 0x4 || bru.bra ___OneLineProcess
    cmu.cmii.u32 i5, i0
    ;Starting from the second line we must take other buffers into account
    ;See explanation in HRESStartShave for this call here
    peu.pc1c 0x1 || cmu.cpii i18, i13
    nop
    nop
    nop
    
    ;----------------End of loop processing one line------------------------------
___OneLineResizeFinish:   
    ;Just for safety check that the DMA was ready
    lsu1.ldil i2, 0x4 || lsu0.ldil i1, 0x0
___CheckDMAReady:
    iau.or i2, i1, i2
    cmu.cmii.u32 i2, i1
    peu.pc1c 0x6 || bru.bra ___CheckDMAReady || lsu0.lda32 i2, SLICE_LOCAL, DMA_TASK_STAT
    nop
    nop
    nop
    nop
    nop
    
    ;!!!!!!!!!!!!!!!!Don't forget to enable the register PUSH and POP's in case of integrating this with 
    ;compiler code !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! 
    bru.swih 0x7
    nop
    nop
    nop
    nop
    nop
    ;Need to set a return here so that the shave will restart executing when we restart it.
    bru.bra OneLineResize
    nop
    nop
    nop
    nop
    nop

.code .text.OneLineResizeSBS
.salign 16
// Outputs nothing
// Input: i18 - line output address, i17 - output stride, i16 - in width, i15 - out width, i14 - in_out height, i13 - second_line and forwards
    OneLineResizeSBS:
    ;Set rounding mode to nearest. Safest rounding mode.
    lsu0.ldil i1, 0x2
    cmu.cpti i2, P_CFG
    iau.or i2, i1, i2
    cmu.cpit P_CFG, i2
 
    ;!!!!!!!!!!!!!!!!Don't forget to enable the register PUSH and POP's in case of integrating this with 
    ;compiler code !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

    ;--------------------------------------Filling in index line part--------------------------------
    ;Horizontal resize is done using Lanczsos resize 
    ;First off, while the CIF bring in it's first line, we compute the indexes needed to resize for
    ;one line and store them into indexes_line
    ;Bare in mind that we are working with YUYV422i data
    ;Start off by computing our resize ratio for Y
    iau.sub i0, i16, 1 || sau.sub.i32 i1, i15, 1
    ;                       || continue with the resize ratio for chromas
    cmu.cpis.i32.f32 s0, i0 || iau.shr.u32 i0, i16, 1 || sau.shr.u32 i2, i15, 1
    cmu.cpis.i32.f32 s1, i1 || iau.sub i0, i0, 1
    cmu.cpis.i32.f32 s3, i0 || iau.sub i2, i2, 1
    ;Compute (outwidth-1)/(inwidth-1) -> the resize ratio for the Y(luma) values
    sau.div.f32 s2, s0, s1 || cmu.cpis.i32.f32 s4, i2
    ;Zero the plane offsets|| Initialize 128.0 float value in i6
    vau.xor v3, v3, v3     || lsu0.ldil i6, 127.0F32 || lsu1.ldih i6, 127.0F32
    ;Ratio for chromas     || Zero the plane offsets
    sau.div.f32 s5, s3, s4 || vau.xor v4, v4, v4 || lsu0.ldil i0, 0
    cmu.cpis s6, i0 || lsu0.ldil i1, 1
    cmu.cpis s7, i0 || lsu0.ldil i2, 2
    cmu.cpis s8, i0 || lsu0.ldil i3, 3
    cmu.cpiv v5.0, i0
    ; Position offset 0 too since for lanczos we load U and V at the same time.
    cmu.cpiv v5.1, i0
    cmu.cpiv v6.1, i2
    cmu.cpiv v6.0, i1
    cmu.cpiv v5.2, i0
    cmu.cpiv v5.3, i0
    ;Setup ratio vector which will be used to compute the indexes inside one plane
    cmu.cpsv.x32 v0.0, s2 || sau.add.f32 s6, s6, s2 || vau.swzword v6, v6 [1010]
    cmu.cpsv.x32 v0.2, s2; || vau.add.i32 v9, v5, 4
    cmu.cpsv.x32 v0.1, s5
    cmu.cpsv.x32 v0.3, s5 || sau.add.f32 s7, s7, s5
    ;Set up initial offsets
    cmu.cpsv.x32 v3.2, s6 || sau.add.f32 s6, s6, s2 || vau.mul.i32 v6, v6, 2
    ;Copy 127.0f to the 127.0 float VRF, v11
    cmu.cpiv v11.0, i6
    cmu.cpsv.x32 v4.1, s7
    cmu.cpsv.x32 v4.3, s7 || sau.add.f32 s6, s6, s2 || vau.mul.f32 v0, v0, 2.0
    cmu.cpsv.x32 v4.0, s6
    nop
    cmu.cpsv.x32 v4.2, s6 || vau.swzword v11, v11 [0000]
    ;                 || Y values are computed 4 at a time so we need to multiply another time 
    peu.pven4word 0x5 || vau.mul.f32 v0, v0, 2.0
    ;Load precalc_vals starting store point
    lsu0.ldil i6, precalc_filter_vals || lsu1.ldih i6, precalc_filter_vals
    nop
    ;We have the ratio's set and the initial plane offsets set here
    ;Add 0.5 in order to have cmu.cpsv.f32.i32s actually round to nearest instead of truncating
    cmu.cpvv v9, v3 ;|| vau.add.f32 v3, v3, 0.5
    cmu.cpvv v10, v4 ;|| vau.add.f32 v4, v4, 0.5
    nop
    ;Find out how many 4 pixels batches must be processed
    iau.shr.u32 i4, i15, 2 || lsu0.ldil i5, indexes_line || lsu1.ldih i5, indexes_line
___Fill4PixelindexesSBS:
    ;Get integer plane offsets || Get fractional part to determine best precalculated position to use
    cmu.cpvv.f32.i32s v7, v3   || vau.frac.f32 v9, v9 
    cmu.cpvv.f32.i32s v8, v4   || vau.frac.f32 v10, v10
    lsu0.ldil i0, precalc_filters || lsu1.ldih i0, precalc_filters
    ;Multiply with 128.0f to get desired precalculated position
    vau.mul.f32 v9, v9, v11 || cmu.cpiv v12.0, i0
    vau.mul.f32 v10, v10, v11
    ;Create integer offset with all positions to find exact filter values
    cmu.vszmword v12, v12, [0000]
    ;Convert to integer values
    cmu.cpvv.f32.i32s v9, v9
    cmu.cpvv.f32.i32s v10, v10
    ;Multiply with 6*2 because this is how much memory a precalculated filter takes
    vau.mul.i32 v9, v9, 12
    vau.mul.i32 v10,v10, 12
    nop
    ;-----------------DEBUG------------------------
    ;Inject slowness
    ;nop 50
    ;nop 50
    ;nop 50
    ;nop 50
    ;nop 50
    ;nop 50
    ;nop 50
    ;-----------------DEBUG------------------------
    ;Add precalc_filters offsets
    vau.add.i32 v9, v9, v12
    vau.add.i32 v10, v10, v12
    ;Multiply with their positions || Start moving addresses to i registers in order to load precalculated filters
    vau.mul.i32 v7, v7, v6         || cmu.cpvi i20, v9.0
    vau.mul.i32 v8, v8, v6         || cmu.cpvi i21, v9.1 || lsu0.ldo64.l v13, i20, 0 || lsu1.ldo64.h v13, i20, 8
    ;And compute next float offsets at the same time
    vau.add.f32 v3, v3, v0         || cmu.cpvi i22, v9.2 || lsu0.ldo64.l v14, i21, 0 || lsu1.ldo64.h v14, i21, 8
    cmu.cpvi i23, v9.3 || lsu0.ldo64.l v15, i22, 0 || lsu1.ldo64.h v15, i22, 8
    vau.add.i32 v7, v7, v5 || cmu.cpvi i24, v10.0 || lsu0.ldo64.l v16, i23, 0 || lsu1.ldo64.h v16, i23, 8
    vau.add.i32 v8, v8, v5 || cmu.cpvi i25, v10.1 || lsu0.ldo64.l v17, i24, 0 || lsu1.ldo64.h v17, i24, 8
    vau.add.f32 v4, v4, v0 || cmu.cpvi i26, v10.2 || lsu0.ldo64.l v18, i25, 0 || lsu1.ldo64.h v18, i25, 8
    ;Store first 2 pixels offsets
    lsu0.st128.u32.u16 v7, i5 || iau.add i5, i5, 8 || cmu.cpvi i27, v10.3 || lsu1.ldo64.l v19, i26, 0
    ;Store last 2 pixels offsets
    lsu0.st128.u32.u16 v8, i5 || iau.add i5, i5, 8 || lsu1.ldo64.h v19, i26, 8
    ;                                                    || 4 pixels batch ended
    lsu0.ldo64.l v20, i27, 0 || lsu1.ldo64.h v20, i27, 8 || iau.sub i4, i4, 1
    ;Start storing precalculated filter values. Load only 6 fp16s (12 bytes).
    peu.pl1en4word 0x3 || lsu1.sto64.h v13, i6, 8 || lsu0.sto64.l v13, i6, 0 || iau.add i6, i6, 12 || vau.sub.f32 v9, v3, 0.5
    peu.pl1en4word 0x3 || lsu1.sto64.h v14, i6, 8 || lsu0.sto64.l v14, i6, 0 || iau.add i6, i6, 12 || vau.sub.f32 v10, v4, 0.5
    peu.pl1en4word 0x3 || lsu1.sto64.h v15, i6, 8 || lsu0.sto64.l v15, i6, 0 || iau.add i6, i6, 12 || cmu.cmz.i32 i4
    peu.pc1c 0x2 || bru.bra ___Fill4PixelindexesSBS
    peu.pl1en4word 0x3 || lsu1.sto64.h v16, i6, 8 || lsu0.sto64.l v16, i6, 0 || iau.add i6, i6, 12 || vau.add.f32 v9, v9, 0.5
    peu.pl1en4word 0x3 || lsu1.sto64.h v17, i6, 8 || lsu0.sto64.l v17, i6, 0 || iau.add i6, i6, 12 || vau.add.f32 v10, v10, 0.5
    peu.pl1en4word 0x3 || lsu1.sto64.h v18, i6, 8 || lsu0.sto64.l v18, i6, 0 || iau.add i6, i6, 12
    peu.pl1en4word 0x3 || lsu1.sto64.h v19, i6, 8 || lsu0.st64.l v19, i6     || iau.add i6, i6, 12
    peu.pl1en4word 0x3 || lsu1.sto64.h v20, i6, 8 || lsu0.st64.l v20, i6     || iau.add i6, i6, 12
        
    ;--------------------------------------End of filling in index line part--------------------------------
    
    ;Set rounding mode to nearest. Safest rounding mode.
    lsu0.ldil i1, 0x2 || lsu1.ldil i0, 0xFFE1 
    cmu.cpti i2, P_CFG || lsu0.ldih i0, 0xFFFF
    ;Clear all rounding bits
    iau.and i2, i0, i2
    ;Set our desired bits
    iau.or i2, i1, i2
    cmu.cpit P_CFG, i2
    
    ;Some setup phase. Load i4 with the address of the cif_dma_done variable
    lsu0.ldil i0, cif_dma_done_adr || lsu1.ldih i0, cif_dma_done_adr
    lsu0.ld32 i4, i0
    ;Load i7 with the pointer for the first line that is going to be ready
    lsu0.ldih i9, CMXFirstOutputLine || lsu1.ldil i9, CMXFirstOutputLine
    ;Load i8 with the pointer to the line being worked on
    lsu0.ldih i10, CMXSecondOutputLine || lsu1.ldil i10, CMXSecondOutputLine
    lsu0.ldil i11, current_cif_buffer_adr || lsu1.ldih i11, current_cif_buffer_adr
    ;Clean my needed 8 registers
    iau.xor i20, i20, i20 || sau.xor i21, i21, i21 || lsu0.ldih i0, force_shave_graceful_halt || lsu1.ldil i0, force_shave_graceful_halt
    ;                                              || Load gracefull halt address into i28
    iau.xor i22, i22, i22 || sau.xor i23, i23, i23 || lsu0.ld32 i28, i0
    iau.xor i24, i24, i24 || sau.xor i25, i25, i25
    iau.xor i26, i26, i26 || sau.xor i27, i27, i27
    
    ;i4 now holds the cif_dma_done address and may be used to syncronize
    
    ;---------------Start of loop processing one line-----------------------------
    iau.xor i5, i5, i5
    ___OneLineProcessSBS:
    
    ;Firstly, wait for the Leon to signal that a line has been finished
    ___WaitCIFSyncSBS:
    lsu0.ld32 i0, i4 || lsu1.ldil i1, 1
    nop
    nop
    ;I have the address of graceful halt at this point in i28. Let's check if it is 1.
    ;If so, then branch to shave finish
    lsu0.ld32 i2, i28
    nop
    nop
    cmu.cmii.u32 i0, i1
    ;if cif_dma_done is 0 then wait some more
    peu.pc1c 0x6 || bru.bra ___WaitCIFSyncSBS
    nop
    ;Compare to see if graceful halt is 0
    cmu.cmii.u32 i2, i1 || lsu0.ldil i3, ___OneLineResizeFinishSBS || lsu1.ldih i3, ___OneLineResizeFinishSBS
    peu.pc1c 0x1 || bru.jmp i3
    nop
    nop
    ;3 more to compensate for last jump 
    nop
    nop
    nop
    
    ;Then reset cif_dma_done to 0
    lsu0.ldil i0, 0 || lsu1.ldil i1, 0x00FF
    lsu0.st32 i0, i4 || cmu.vszmbyte i1, i1, [1010]
    
    ;Swap output buffers
    cmu.cpii i9, i10 || iau.add i10, i9, 0
    
    ;Process the received line
    ;Find out where was it stored
    lsu0.ld32 i8, i11 || cmu.cpiv v25.0, i1
    ;                                                          || prepare mask for u16->f16 conversion
    lsu0.ldil i12, indexes_line || lsu1.ldih i12, indexes_line || vau.swzword v25, v25, [0000]
    ;                                                 || Load index start for precalculated values
    lsu0.ld128.u16.u32 v1, i12 || iau.add i12, i12, 8 || lsu1.ldil i29, precalc_filter_vals
    lsu0.ld128.u16.u32 v2, i12 || iau.add i12, i12, 8 || lsu1.ldih i29, precalc_filter_vals
    lsu0.ld128.u16.u32 v3, i12 || iau.add i12, i12, 8
    lsu0.ld128.u16.u32 v4, i12 || iau.add i12, i12, 8
    ;Fill in one V register with the starting point of the buffer
    cmu.cpiv v0.0, i8
    vau.swzword v0, v0, [0000]
    peu.pven4word 0x5 || vau.add.i32 v0, v0, 4
    
    ;We'll try processing batches of 8 pixels for now
    ;See how many batches of 8 pixels are there in the output
    iau.shr.u32 i6, i15, 3
    cmu.cpii i7, i9 || iau.shl i20, i16, 1 
    ;We need to mirror first 2 values and last 3 values
    lsu0.ldo32 i22, i8, 8 || iau.add i20, i20, 8
    ;Get address of last pixel position that need to be mirrored
    iau.add i21, i20, i8
    iau.sub i21, i21, 4
    ;Read last pixel value too
    lsu0.ld32 i23, i21 || vau.xor v5, v5 v5
    ;Zero last element of V25 because we only use 6 filters
    cmu.vszmword v25, v25, [Z210] || vau.xor v6, v6, v6
    vau.xor v7, v7, v7 || cmu.vszmword v26, v25, [2210]
    vau.xor v8, v8, v8
    ;Swizzle bytes
    cmu.vszmbyte i22, i22, [3010]
    ;                                              || And also copy to old values register v27
    lsu0.sto32 i22, i8, 0 || lsu1.sto32 i22, i8, 4 || cmu.cpiv v27.0, i22
    ;Swizzle bytes for last position
    cmu.vszmbyte i23, i23, [3212]
    lsu0.sto32 i23, i21, 4 || lsu1.sto32 i23, i21, 8
    lsu0.sto32 i23, i21, 12
___Copy_startSBS:
    vau.add.i32 v1, v1, v0
    vau.add.i32 v2, v2, v0
    vau.add.i32 v3, v3, v0 || cmu.cpvi.x32 i0, v1.0
    ;                                               || load pixel values for Y0
    vau.add.i32 v4, v4, v0 || cmu.cpvi.x32 i1, v1.2 || lsu0.ldo64.l v9, i0, 0 || lsu1.ldo64.h v9, i0, 8
    ;Load pixel values for Y1                       || Get offsets for Y2
    lsu0.ldo64.l v11, i1, 0 || lsu1.ldo64.h v11, i1, 8 || cmu.cpvi.x32 i2, v2.0
    ;Load pixel values for Y2                          || Get offsets for Y3
    lsu0.ldo64.l v13, i2, 0 || lsu1.ldo64.h v13, i2, 8 || cmu.cpvi.x32 i3, v2.2
    ;Load pixel values for Y3                          || Get offsets for Y4
    lsu0.ldo64.l v15, i3, 0 || lsu1.ldo64.h v15, i3, 8 || cmu.cpvi.x32 i20, v3.0
    ;Load pixel values for Y4                            || Get offsets for Y5
    lsu0.ldo64.l v10, i20, 0 || lsu1.ldo64.h v10, i20, 8 || cmu.cpvi.x32 i21, v3.2
    ;Load pixel values for Y5                            || Get offsets for Y6
    lsu0.ldo64.l v12, i21, 0 || lsu1.ldo64.h v12, i21, 8 || cmu.cpvi.x32 i22, v4.0
    ;Load pixel values for Y6                            || Get offsets for Y7     || And with a mask to prepare f16 conversion
    lsu0.ldo64.l v14, i22, 0 || lsu1.ldo64.h v14, i22, 8 || cmu.cpvi.x32 i23, v4.2 || vau.and v9, v9, v25
    ;Load pixel values for Y7
    lsu0.ldo64.l v16, i23, 0 || lsu1.ldo64.h v16, i23, 8 || vau.and v11, v11, v25 || cmu.cpvv.i16.f16 v9, v9
    ;                                                  ||And start loading the Y0 output precalculated filter values for multiplication
    vau.and v13, v13, v25 || cmu.cpvv.i16.f16 v11, v11 || lsu0.ldo64.l v17, i29, (6*2*0+0)  || lsu1.ldo64.h v17, i29, (6*2*0+8)
    vau.and v15, v15, v25 || cmu.cpvv.i16.f16 v13, v13 || lsu0.ldo64.l v19, i29, (6*2*2+0)  || lsu1.ldo64.h v19, i29, (6*2*2+8)
    vau.and v10, v10, v25 || cmu.cpvv.i16.f16 v15, v15 || lsu0.ldo64.l v21, i29, (6*2*4+0)  || lsu1.ldo64.h v21, i29, (6*2*4+8)
    vau.and v12, v12, v25 || cmu.cpvv.i16.f16 v10, v10 || lsu0.ldo64.l v23, i29, (6*2*6+0)  || lsu1.ldo64.h v23, i29, (6*2*6+8)
    vau.and v14, v14, v25 || cmu.cpvv.i16.f16 v12, v12 || lsu0.ldo64.l v18, i29, (6*2*8+0)  || lsu1.ldo64.h v18, i29, (6*2*8+8)
    vau.and v16, v16, v25 || cmu.cpvv.i16.f16 v14, v14 || lsu0.ldo64.l v20, i29, (6*2*10+0) || lsu1.ldo64.h v20, i29, (6*2*10+8)
    ;Start multiplying Y values with the preselected lanczos filters || continue with the rest of operations
    vau.mul.f16 v9, v9, v17                                          || cmu.cpvv.i16.f16 v16, v16 || lsu0.ldo64.l v22, i29, (6*2*12+0) || lsu1.ldo64.h v22, i29, (6*2*12+8)
    vau.mul.f16 v11, v11, v19 || lsu0.ldo64.l v24, i29, (6*2*14+0) || lsu1.ldo64.h v24, i29, (6*2*14+8)
    ;Start working on Chroma values. Not such a trivial matter because 6 chroma values span over 24 bytes
    ;This means we need 2 vrf loads just to get the needed values for 1 U element or 1 V element
    ;The good news though is that V and U have the same ratio therefore, they will be spatially localised
    ;To the same input area which means we can extract both U and V from the same loads
    ;                         || Start loading U value for U0 out
    vau.mul.f16 v13, v13, v21 || cmu.cpvi.x32 i0, v1.1
    ;                         || Sum lanczos values      || take address for V0 out coefs || Load values for U0. first half 
    vau.mul.f16 v15, v15, v23 || sau.sum.f16 s9, v9, 0xF || cmu.cpvi.x32 i1, v2.1         || lsu0.ldo64.l v10,  i0, 0 || lsu1.ldo64.h v10,  i0, 8
    ;                                                                                     || Load values for V0. first half. etc. I'm not commenting anymore
    vau.mul.f16 v10, v10, v18 || sau.sum.f16 s11, v11, 0xF || cmu.cpvi.x32 i2, v3.1       || lsu0.ldo64.l v12,  i1, 0 || lsu1.ldo64.h v12,  i1, 8 
    vau.mul.f16 v12, v12, v20 || sau.sum.f16 s13, v13, 0xF || cmu.cpvi.x32 i3, v4.1       || lsu0.ldo64.l v14,  i2, 0 || lsu1.ldo64.h v14,  i2, 8
    vau.mul.f16 v14, v14, v22 || sau.sum.f16 s15, v15, 0xF || lsu0.ldo64.l v16, i3, 0 || lsu1.ldo64.h v16, i3, 8
    ;                                                         Start loading second halfs of the chroma needed values
    vau.mul.f16 v16, v16, v24 || sau.sum.f16 s10, v10, 0xF || lsu0.ldo64.l v9, i0, 16 || lsu1.ldo64.l v11, i1, 16
    ;                         || Y0 out
    sau.sum.f16 s12, v12, 0xF || cmu.cpsv.x16 v5.0, s9.l     || lsu0.ldo64.l v13, i2, 16 || lsu1.ldo64.l v15, i3, 16
    ;                         || Y1 out                      || Start loading Lanczos coeficients for U and V. Start with V0
    sau.sum.f16 s14, v14, 0xF || cmu.cpsv.x16 v5.2, s11.l    || lsu0.ldo64.l v17, i29, (6*2*3+0) || lsu1.ldo64.h v17, i29, (6*2*3+8)
    ;                         || Y2 out                      || Apply first byte swizzle. Refer to sheet "U-V setup" from OneLineRegisterMap.xls document in doc folder
    sau.sum.f16 s16, v16, 0xF || cmu.cpsv.x16 v5.4, s13.l    || vau.swzbyte v10, v10, [2031] || lsu0.ldo64.l v18, i29, (6*2*1+0) || lsu1.ldo64.h v18, i29, (6*2*1+8)
    ;Y3 out
    cmu.cpsv.x16 v5.6, s15.l    || vau.swzbyte v12, v12, [2031] || lsu0.ldo64.l v19, i29, (6*2*7+0)  || lsu1.ldo64.h v19, i29, (6*2*7+8)
    ;Y4 out
    cmu.cpsv.x16 v6.0, s10.l    || vau.swzbyte v14, v14, [2031] || lsu0.ldo64.l v20, i29, (6*2*5+0)  || lsu1.ldo64.h v20, i29, (6*2*5+8)
    ;Y5 out
    cmu.cpsv.x16 v6.2, s12.l    || vau.swzbyte v16, v16, [2031] || lsu0.ldo64.l v21, i29, (6*2*11+0)  || lsu1.ldo64.h v21, i29, (6*2*11+8)
    ;Y6 out
    cmu.cpsv.x16 v6.4, s14.l    || vau.swzbyte v9, v9, [2031]   || lsu0.ldo64.l v22, i29, (6*2*9+0)  || lsu1.ldo64.h v22, i29, (6*2*9+8)
    ;Y7 out
    cmu.cpsv.x16 v6.6, s16.l    || vau.swzbyte v11, v11, [2031] || lsu0.ldo64.l v23, i29, (6*2*15+0) || lsu1.ldo64.h v23, i29, (6*2*15+8)
    vau.swzbyte v13, v13, [2031]  || lsu0.ldo64.l v24, i29, (6*2*13+0) || lsu1.ldo64.h v24, i29, (6*2*13+8)
    ;                            || No need for this precalc_filters_val anymore so we can upgrade now
    vau.swzbyte v15, v15, [2031] || lsu0.ldil i0, (6*2*16)
    ;Apply second stage of swizzling
    lsu0.swzv8 [75316420] || vau.swzword v10, v10, [3210] || iau.add i29, i29, i0
    lsu0.swzv8 [75316420] || vau.swzword v12, v12, [3210]
    lsu0.swzv8 [75316420] || vau.swzword v14, v14, [3210]
    lsu0.swzv8 [75316420] || vau.swzword v16, v16, [3210]
    lsu0.swzv8 [75316420] || vau.swzword  v9,  v9, [3210]
    lsu0.swzv8 [75316420] || vau.swzword v11, v11, [3210]
    lsu0.swzv8 [75316420] || vau.swzword v13, v13, [3210]
    lsu0.swzv8 [75316420] || vau.swzword v15, v15, [3210]
    ;Step 3, combining chroma elements
    peu.pven4word 0x4 || lsu0.swz4v [3210] [3012] [PPPP] || vau.swzword v10,  v9, [3210]
    peu.pven4word 0x4 || lsu0.swz4v [3210] [3012] [PPPP] || vau.swzword v12, v11, [3210]
    peu.pven4word 0x4 || lsu0.swz4v [3210] [3012] [PPPP] || vau.swzword v14, v13, [3210]
    peu.pven4word 0x4 || lsu0.swz4v [3210] [3012] [PPPP] || vau.swzword v16, v15, [3210]
    ;Step 4, take V elements into separate vectors
    vau.swzbyte  v9, v10, [2301]
    vau.swzbyte v11, v12, [2301]
    vau.swzbyte v13, v14, [2301]
    vau.swzbyte v15, v16, [2301]
    ;And we're almost done, only thing left: multiply with the same mask Y values were multiplied
    ;in order to get all unneeded data zeroed
    vau.and  v9,  v9, v25
    ;                     || Start converting to fp16
    vau.and v10, v10, v25 || cmu.cpvv.i16.f16 v9, v9
    vau.and v11, v11, v25 || cmu.cpvv.i16.f16 v10, v10
    vau.and v12, v12, v25 || cmu.cpvv.i16.f16 v11, v11
    vau.and v13, v13, v25 || cmu.cpvv.i16.f16 v12, v12
    vau.and v14, v14, v25 || cmu.cpvv.i16.f16 v13, v13
    vau.and v15, v15, v25 || cmu.cpvv.i16.f16 v14, v14
    vau.and v16, v16, v25 || cmu.cpvv.i16.f16 v15, v15
    cmu.cpvv.i16.f16 v16, v16 || vau.mul.f16 v9, v9, v17
    vau.mul.f16 v10, v10, v18
    vau.mul.f16 v11, v11, v19
    vau.mul.f16 v12, v12, v20 || sau.sum.f16  s9,  v9, 0xF
    vau.mul.f16 v13, v13, v21 || sau.sum.f16 s10, v10, 0xF
    vau.mul.f16 v14, v14, v22 || sau.sum.f16 s11, v11, 0xF
    vau.mul.f16 v15, v15, v23 || sau.sum.f16 s12, v12, 0xF
    vau.mul.f16 v16, v16, v24 || sau.sum.f16 s13, v13, 0xF
    ;                         || And start moving results to the final line. V0 out
    sau.sum.f16 s14, v14, 0xF || cmu.cpsv.x16 v5.3, s9.l
    ;                         || U0 out
    sau.sum.f16 s15, v15, 0xF || cmu.cpsv.x16 v5.1, s10.l
    ;                         || V1 out
    sau.sum.f16 s16, v16, 0xF || cmu.cpsv.x16 v5.7, s11.l
    ;U1 out
    cmu.cpsv.x16 v5.5, s12.l
    ;V2 out
    cmu.cpsv.x16 v6.3, s13.l
    ;U2 out
    cmu.cpsv.x16 v6.1, s14.l
    ;V3 out
    cmu.cpsv.x16 v6.7, s15.l
    ;U3 out
    cmu.cpsv.x16 v6.5, s16.l
    ;Convert to fp16
    cmu.cpvv.f16.i16s v5, v5
    cmu.cpvv.f16.i16s v6, v6
    ;Clamp results to 0..255
    cmu.clamp0.i16 v5, v5, v26
    cmu.clamp0.i16 v6, v6, v26
    nop
    ;And now to double the size since this is Side by we're talking about
    ;First compress pixels to u8
    cmu.cpvv.u16.u8s v5, v5
    cmu.cpvv.u16.u8s v6, v6
    ;Then merge the 8 pixels
    peu.pven4word 0xC || vau.swzword v5, v6, [1032]
    ;Get the rotated values needed to compute Y middle pixels and V middle pixels
    cmu.vrot v6, v5, 12
    cmu.vrot v7, v5, 14
    ;Add in last values to the queue
    ;For Us and Vs it is easy as we just take in the whole lot last values || For Y's it's a bit complicated. we need to take in only 16 bytes so using temporary storage
    peu.pven4word 0x1 || vau.swzword v6, v27, [3210]                       || cmu.cpvi.x32 i0, v27.0
    cmu.cpiv.x16 v7.0, i0.h
    ;Good! Now let's have the averages
    vau.avg.u8 v6, v6, v5
    vau.avg.u8 v7, v7, v5
    nop
    ;Perfect so far! Now we must interleave pixels but in a strange fashion.
    ;First let's create the required Y vectors -> 1 vector of 16 values
    ;Create one vector with chromas zeroed
    vau.and v8, v5, v26
    ;Rotate it once to the left || save last pixels for next round
    cmu.vrot v8, v8, 15         || vau.swzword v27, v5, [2103]
    ;And add the averaged values in
    peu.pven4byte 0x5 || vau.or v8, v7, v8
    ;Good again! Y are all in place. Same trick on Us and Vs
    cmu.vrot v6, v6, 1
    vau.and v6, v6, v26
    peu.pven4byte 0xA || vau.or v6, v6, v5
    ;We're almost there, jsut one more byte swizzle on v6 since
    ;we want to have values as U, V, U, V... not U, U, V, V...
    vau.swzbyte v9, v6 [3120]
    ;And interleave!
    cmu.vilv.x8 v8, v9, v9, v8
    ;-------------------------DEBUG ONLY!----------------------------------
    ;Inject slowness to test resilience
    ;Zero chromas for debug
    ;lsu0.ldil i0, 0xFFFF || lsu1.ldih i0, 0x0000
    ;cmu.cpiv v31.0, i0 || lsu0.ldil i0, 0x0000 || lsu1.ldih i0, 0x0080
    ;vau.swzword v31, v31, [0000] || cmu.cpiv v30.0 ,i0
    ;vau.swzword v30, v30, [0000]
    ;vau.and v5, v5, v31
    ;vau.and v6, v6, v31
    ;vau.or v5, v5, v30
    ;vau.or v6, v6, v30
    ;Cut Y0
    ;lsu0.ldil i0, 0x0000 || lsu1.ldih i0, 0xFFFF
    ;cmu.cpiv v31.0, i0 || lsu0.ldil i0, 0x0080 || lsu1.ldih i0, 0x0000 
    ;cmu.cpiv v30.0, i0 || vau.swzword v31, v31, [0000]
    ;vau.swzword v30, v30, [0000]
    ;peu.pven4word 0x1 || vau.and v5, v5, v31
    ;peu.pven4word 0x1 || vau.or  v5, v5, v30
    ;;Cut Y1
    ;peu.pven4word 0x2 || vau.and v5, v5, v31
    ;peu.pven4word 0x2 || vau.or  v5, v5, v30
    ;;Cut Y2
    ;peu.pven4word 0x4 || vau.and v5, v5, v31
    ;peu.pven4word 0x4 || vau.or  v5, v5, v30
    ;;Cut Y3
    ;peu.pven4word 0x8 || vau.and v5, v5, v31
    ;peu.pven4word 0x8 || vau.or  v5, v5, v30
    ;;Cut Y4
    ;peu.pven4word 0x1 || vau.and v6, v6, v31
    ;peu.pven4word 0x1 || vau.or  v6, v6, v30
    ;;Cut Y5
    ;peu.pven4word 0x2 || vau.and v6, v6, v31
    ;peu.pven4word 0x2 || vau.or  v6, v6, v30
    ;;Cut Y6
    ;peu.pven4word 0x4 || vau.and v6, v6, v31
    ;peu.pven4word 0x4 || vau.or  v6, v6, v30
    ;;Cut Y7
    ;peu.pven4word 0x8 || vau.and v6, v6, v31
    ;peu.pven4word 0x8 || vau.or  v6, v6, v30
    
    ;nop 50
    ;nop 50
    ;nop 50
    ;-------------------------DEBUG ONLY!----------------------------------
    lsu0.ld128.u16.u32 v1, i12 || iau.sub i6, i6, 1
    ;                                     || Increment here to be able to use peu.pc1i
    peu.pc1i 0x2 || bru.bra ___Copy_startSBS || iau.add i12, i12, 8
    lsu0.ld128.u16.u32 v2, i12 || iau.add i12, i12, 8
    lsu0.ld128.u16.u32 v3, i12 || iau.add i12, i12, 8
    lsu0.ld128.u16.u32 v4, i12 || iau.add i12, i12, 8
    lsu0.sto64.l v8, i7, 0  || lsu1.sto64.h v8, i7, 8 || iau.add i7, i7, 16 || vau.xor v5, v5, v5
    lsu0.sto64.l v9, i7, 0  || lsu1.sto64.h v9, i7, 8 || iau.add i7, i7, 16 || vau.xor v6, v6, v6
    
    ;-----------------DEBUG ONLY----------------
    ;nop 3
    ;;Fill in current line with a marker
    ;lsu0.ldil i0, (128*2*1+12)
    ;iau.add i0, i9, i0
    ;lsu0.sto32 i30, i0, (4*0) || lsu1.sto32 i30, i0, (4*1)
    ;lsu0.sto32 i30, i0, (4*2) || lsu1.sto32 i30, i0, (4*3)
    
    ;lsu0.ldil i0, (128*2*2+12)
    ;iau.add i0, i9, i0
    ;lsu0.sto32 i30, i0, (4*0) || lsu1.sto32 i30, i0, (4*1)
    ;lsu0.sto32 i30, i0, (4*2) || lsu1.sto32 i30, i0, (4*3)
    
    ;lsu0.ldil i0, (128*2*3+12)
    ;iau.add i0, i9, i0
    ;lsu0.sto32 i30, i0, (4*0) || lsu1.sto32 i30, i0, (4*1)
    ;lsu0.sto32 i30, i0, (4*2) || lsu1.sto32 i30, i0, (4*3)
    
    ;lsu0.ldil i0, (128*2*4+12)
    ;iau.add i0, i9, i0
    ;lsu0.sto32 i30, i0, (4*0) || lsu1.sto32 i30, i0, (4*1)
    ;lsu0.sto32 i30, i0, (4*2) || lsu1.sto32 i30, i0, (4*3)
    
    ;lsu0.ldil i0, (128*2*5+12)
    ;iau.add i0, i9, i0
    ;lsu0.sto32 i30, i0, (4*0) || lsu1.sto32 i30, i0, (4*1)
    ;lsu0.sto32 i30, i0, (4*2) || lsu1.sto32 i30, i0, (4*3)
    
    ;lsu0.ldil i0, (128*2*6+12)
    ;iau.add i0, i9, i0
    ;lsu0.sto32 i30, i0, (4*0) || lsu1.sto32 i30, i0, (4*1)
    ;lsu0.sto32 i30, i0, (4*2) || lsu1.sto32 i30, i0, (4*3)
    
    ;lsu0.ldil i0, (128*2*7+12)
    ;iau.add i0, i9, i0
    ;lsu0.sto32 i30, i0, (4*0) || lsu1.sto32 i30, i0, (4*1)
    ;lsu0.sto32 i30, i0, (4*2) || lsu1.sto32 i30, i0, (4*3)
    
    ;lsu0.ldil i0, (128*2*8+12)
    ;iau.add i0, i9, i0
    ;lsu0.sto32 i30, i0, (4*0) || lsu1.sto32 i30, i0, (4*1)
    ;lsu0.sto32 i30, i0, (4*2) || lsu1.sto32 i30, i0, (4*3)
    
    ;lsu0.ldil i0, (128*2*9+12)
    ;iau.add i0, i9, i0
    ;lsu0.sto32 i30, i0, (4*0) || lsu1.sto32 i30, i0, (4*1)
    ;lsu0.sto32 i30, i0, (4*2) || lsu1.sto32 i30, i0, (4*3)
    
    ;cmu.cpis s18, i0
    ;Store indexes for position 128 (first broken)
    ;lsu0.ldil i0, (128*2)
    ;lsu0.ldil i1, indexes_line || lsu1.ldih i1, indexes_line
    ;iau.add i0, i1, i0
    ;lsu0.ldo64.l v31, i0, 0 || lsu1.ldo64.h v31, i0, 8
    ;cmu.cpis s19, i0
    ;cmu.cpis s20, i1
    ;-----------------DEBUG ONLY----------------
    
    ;--------------Setup and start DMAs to stream to resized buffer
    ;Source of DMA transfer
    cmu.cpii i0, i9
    ;lsu0.ldil i0, CMXFirstOutputLine || lsu1.ldih i0, CMXFirstOutputLine
    ;                                         || Load configuration
    lsu0.sta32 i0, SLICE_LOCAL, DMA0_SRC_ADDR || lsu1.ldil i1, 0x1 || iau.shl i3, i15, 2
    lsu0.sta32 i1, SLICE_LOCAL, DMA0_CFG || lsu1.sta32 i3, SLICE_LOCAL, DMA0_SIZE
    ;Set destination address                   || increase destination address
    lsu0.sta32 i18, SLICE_LOCAL, DMA0_DST_ADDR || iau.add i18, i18, i3 || lsu1.ldil i0, 0x0
    ;fire DMA0 task
    lsu0.sta32 i0, SLICE_LOCAL, DMA_TASK_REQ || iau.shl i3, i17, 1
    ;Add stride to i18
    iau.add i18, i18, i3
    
    ;Just for safety check that the DMA was ready
;    lsu1.ldil i2, 0x4 || lsu0.ldil i1, 0x0
;___CheckDMAReady:
;    iau.or i2, i1, i2
;    cmu.cmii.u32 i2, i1
;    peu.pc1c 0x6 || bru.bra ___CheckDMAReady || lsu0.lda32 i2, SLICE_LOCAL, DMA_TASK_STAT
;    nop
;    nop
;    nop
;    nop
;    nop
    
    ;--------------End of setup
    iau.add i5, i5, 1
    cmu.cmii.u32 i5, i14 || lsu0.ldil i0, 1
    peu.pc1c 0x4 || bru.bra ___OneLineProcessSBS
    cmu.cmii.u32 i5, i0
    ;Starting from the second line we must take other buffers into account
    ;See explanation in HRESStartShave for this call here
    peu.pc1c 0x1 || cmu.cpii i18, i13
    nop
    nop
    nop
    
    ;----------------End of loop processing one line------------------------------
___OneLineResizeFinishSBS:   
    ;Just for safety check that the DMA was ready
    lsu1.ldil i2, 0x4 || lsu0.ldil i1, 0x0
___CheckDMAReadySBS:
    iau.or i2, i1, i2
    cmu.cmii.u32 i2, i1
    peu.pc1c 0x6 || bru.bra ___CheckDMAReadySBS || lsu0.lda32 i2, SLICE_LOCAL, DMA_TASK_STAT
    nop
    nop
    nop
    nop
    nop
    
    ;!!!!!!!!!!!!!!!!Don't forget to enable the register PUSH and POP's in case of integrating this with 
    ;compiler code !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! 
    bru.swih 0x7
    nop
    nop
    nop
    nop
    nop
    ;Need to set a return here so that the shave will restart executing when we restart it.
    bru.bra OneLineResizeSBS
    nop
    nop
    nop
    nop
    nop    
    
.end
