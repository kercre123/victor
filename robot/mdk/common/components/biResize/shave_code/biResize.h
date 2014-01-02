; ///
; /// @file
; /// @copyright All code copyright Movidius Ltd 2012, all rights reserved.
; ///            For License Warranty see: common/license.txt
; ///
; /// @brief     
; ///

; mechanism to protect against multiple inclusions
.ifndef RESIZE_CFG
.set RESIZE_CFG 1

.include "svuCommonDefinitions.incl"
;/* resize data structure offsets */
;/* API */
.set wi            0x00
.set hi            (wi+0x04)          ;// 0x04
.set wo            (hi+0x04)          ;// 0x08
.set ho            (wo+0x04)          ;// 0x0C
.set offsetX       (ho+0x04)          ;// 0x10
.set offsetY       (offsetX+0x04)     ;// 0x14
.set inBuffer      (offsetY+0x04)     ;// 0x18
.set outBuffer     (inBuffer+0x04)    ;// 0x1C
.set shadow        (outBuffer+0x04)   ;// 0x20
.set sizeof_resize (shadow+0x10)

.set YUV_420 0
.set YUV_422 1
.set YUV_FORMAT YUV_422

.set i_endp_Cr   i19
.set i_stride    i20
.set i_offset    i21
.set i_endp      i22
.set i_posy      i23
.set i_wi        i24
.set i_hi        i25
.set i_wo        i26
.set i_ho        i27
.set i_inBuffer  i28
.set i_outBuffer i29
.set resize      i31

.set MAX_WIDTH   0x800
;/* when changing max sizes manually remap buffers if required */
.set BufferOut_Y   0x1C000000
.set BufferH_Y   ( BufferOut_Y+MAX_WIDTH)
.set ContribH    ( BufferH_Y+MAX_WIDTH)
.set IncrH       ( ContribH+MAX_WIDTH)
.set BufferV_Y   ( IncrH+MAX_WIDTH)
.set ContribV    ( BufferV_Y+2*MAX_WIDTH)
.set IncrV       ( ContribV+MAX_WIDTH)
.set BufferOut_Cr ( IncrV +MAX_WIDTH)
.set BufferV_Cr  ( BufferOut_Cr+MAX_WIDTH/2)
.set BufferOut_Cb ( BufferV_Cr +MAX_WIDTH)
.set BufferV_Cb  ( BufferOut_Cb+MAX_WIDTH/2)
; YUV422 buffer
.set BufferOut    (BufferV_Cb + MAX_WIDTH) 

.set BufferH BufferH_Y
.set BufferV BufferV_Y



.set LOCAL_SLICE       0x4

