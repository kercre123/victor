; ///
; /// @file
; /// @copyright All code copyright Movidius Ltd 2012, all rights reserved.
; ///            For License Warranty see: common/license.txt
; ///
; /// @brief    
; ///

.version 00.51.05
;-------------------------------------------------------------------------------

.code .text.add_simd
    add_simd:
        bru.jmp i30 || vau.add.i32 v23, v23, v22
        nop
        nop
        nop
        nop
        nop
        
.end
