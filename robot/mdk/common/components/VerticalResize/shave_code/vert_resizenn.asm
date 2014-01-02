///   
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved. 
///            For License Warranty see: common/license.txt   
///
/// @brief     Vertical resize with image doubling. 
/// 
 
.version 00.50.05

.include shared_types.incl

;This is the line width. In YUYV we need 2*MAXWIDTH, as may be seen below when allocating space
.set MAX_WIDTH 1920

.data .data.VertResize422DataNN
.salign 16
    CMXLineBuffer_NN:
    .fill (MAX_WIDTH*2), 1, 0x0
    gracefull_halt_NN:
    .int 0
    
.code .text.VertResize422iNN
.salign 16
// Outputs nothing
// Input: i18 - output address, i17 - input address, i16 - in_out width, i15 - in height, i14 - out height
    VertResize422iNN:
    
    ;Some setup stage
    ;Calculate resize ratio
    iau.sub i0, i15, 1 || sau.sub.i32 i1, i14, 1
    cmu.cpis.i32.f32 s0, i0
    cmu.cpis.i32.f32 s1, i1 
    lsu0.ldil i0, 0
    sau.div.f32 s2, s0, s1 || cmu.cpis s3, i0
    ;Add 0.5 to round to nearest value when converting to int for line number
    sau.add.f32 s3, s3, 0.5
    ;This is the line byte width. It is needed to quickly compute the new line index when resizing
    iau.add i6, i16, i16
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    ;-----------------------Process the number of needed output lines-----------------------
    ;Get a counter set up
    cmu.cpii i5, i14
___ProcessOneLine:
    ;get the current index   || and prepare next line index
    cmu.cpsi.f32.i32s i4, s3 || sau.add.f32 s3, s3, s2
    nop
    ;multiply the line index to find out the position of the actual line
    iau.mul i4, i4, i6
    nop
    iau.add i4, i4, i17 || lsu0.ldil i3, CMXLineBuffer || lsu1.ldih i3, CMXLineBuffer
    ;At this point here, we have the input address in i4 and the output in i18
    ;Fire DMA's to bring in the required line
    lsu0.ldil i0, 0x1
    lsu0.sta32 i0, SLICE_LOCAL, DMA0_CFG  || lsu1.sta32 i4, SLICE_LOCAL, DMA0_SRC_ADDR
    lsu0.sta32 i6, SLICE_LOCAL, DMA0_SIZE || lsu1.sta32 i3, SLICE_LOCAL, DMA0_DST_ADDR
    ;Start only DMA task 0
    lsu0.ldil i0, 0x0
    lsu0.sta32 i0, SLICE_LOCAL, DMA_TASK_REQ
    
    ;Wait for DMAs to finish
    lsu1.ldil i2, 0x4 || lsu0.ldil i1, 0x0
___CheckDMAReadReady:
    iau.or i2, i1, i2
    cmu.cmii.u32 i2, i1
    peu.pc1c 0x6 || bru.bra ___CheckDMAReadReady || lsu0.lda32 i2, SLICE_LOCAL, DMA_TASK_STAT
    nop
    nop
    nop
    nop
    nop
    
    ;And now, write the line to the output twice so that we also double it's size
    ;We'll use two DMA tasks for this
    lsu0.ldil i3, CMXLineBuffer || lsu1.ldih i3, CMXLineBuffer
    lsu0.ldil i0, 0x1
    lsu0.sta32 i0, SLICE_LOCAL, DMA0_CFG       || lsu1.sta32 i0, SLICE_LOCAL, DMA1_CFG
    lsu0.sta32 i3, SLICE_LOCAL, DMA0_SRC_ADDR  || lsu1.sta32 i3, SLICE_LOCAL, DMA1_SRC_ADDR
    lsu0.sta32 i6, SLICE_LOCAL, DMA0_SIZE      || lsu1.sta32 i6, SLICE_LOCAL, DMA1_SIZE
    lsu0.sta32 i18, SLICE_LOCAL, DMA0_DST_ADDR || iau.add i18, i18, i6
    lsu1.sta32 i18, SLICE_LOCAL, DMA1_DST_ADDR || iau.add i18, i18, i6
    ;Start DMA0 and DMA1 tasks
    lsu0.ldil i0, 0x1
    lsu0.sta32 i0, SLICE_LOCAL, DMA_TASK_REQ

    ;Wait for DMAs to finish
    lsu1.ldil i2, 0x4 || lsu0.ldil i1, 0x0
___CheckDMAWriteReady:
    iau.or i2, i1, i2
    cmu.cmii.u32 i2, i1
    peu.pc1c 0x6 || bru.bra ___CheckDMAWriteReady || lsu0.lda32 i2, SLICE_LOCAL, DMA_TASK_STAT
    nop
    nop
    nop
    nop
    nop

    iau.sub i5, i5, 1
    peu.pc1i 0x2 || bru.bra ___ProcessOneLine
    nop
    nop
    nop
    nop
    nop
    ;-----------------------End of processing output lines----------------------------------

    bru.swih 0x7
    nop
    nop
    nop
    nop
    nop
    ;Need to set a return here so that the shave will restart executing when we restart it.
    bru.bra VertResize422iNN
    nop
    nop
    nop
    nop
    nop
.end
