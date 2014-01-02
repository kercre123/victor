; ///
; /// @file
; /// @copyright All code copyright Movidius Ltd 2012, all rights reserved.
; ///            For License Warranty see: common/license.txt
; ///
; /// @brief
; ///

.version 00.50.19

.include "svuCommonDefinitions.incl"

.code .text.scGetTrfReg ;text

;--------------------------------mutex functions----------------------------
scGetTrfReg:
    cmu.cpti i0, P_SVID  || iau.shl i18, i18, 2
    lsu0.ldil i1, 0x1100 ||lsu1.ldih i1, 0x8014   || iau.shl i0, i0, 16
    iau.add i0, i0, i1
    iau.add i0, i0, i18
    bru.jmp i30 ||lsu0.ld32 i18, i0
    nop 5

.code .text.scMutexRequest
;Parameters: mutex
scMutexRequest:
    lsu1.ldil  i0, MUTEX_REQUEST_0
    iau.or     i18, i18, i0
    lsu0.sta32 i18, SLICE_LOCAL, MUTEX_CTRL_ADDR 
    nop 1
    lsu0.lda32 i18, SLICE_LOCAL, MUTEX_CTRL_ADDR
    nop 5
    lsu0.ldil  i0, 0x20
    cmu.cmti.bitp i0, P_GPI
    peu.pcxc NEQ, 0x1 || cmu.cmti.bitp i0, P_GPI || bru.rpim 0
    nop 5
    bru.jmp i30
    nop 5

.code .text.scMutexRelease
;Parameters: mutex
scMutexRelease:
    bru.jmp     i30
    lsu1.ldil   i0, MUTEX_RELEASE_0
    iau.or      i0, i0, i18
    lsu0.sta32  i0, SLICE_LOCAL, MUTEX_CTRL_ADDR
    nop         2

.code .text.scMutexInitRandevu
;Parameters: mutex, randevu, wait_no
scMutexInitRandevu:
    bru.jmp i30
    nop 5

.code .text.scMutexWaitFinished
;Parameters: mutex, address_counter
scMutexWaitFinished:
;req mutex
    lsu1.ldil  i0, MUTEX_REQUEST_0
    iau.or     i18, i18, i0
    lsu0.sta32 i18, SLICE_LOCAL, MUTEX_CTRL_ADDR 
    nop 2
    lsu0.ldil   i0, 0x20
    cmu.cmti.bitp i0, P_GPI
    peu.pcxc    NEQ, 0x1 || cmu.cmti.bitp i0, P_GPI || bru.rpim 0
    nop 5
;dec counter
    lsu0.ld32   i1, i17
    nop 5
    iau.incs    i1, -1
    lsu0.st32   i1, i17
    nop
;release mutex
    lsu1.ldil   i0, MUTEX_RELEASE_0
    iau.add     i18, i18, i0
    lsu0.sta32  i18, SLICE_LOCAL, MUTEX_CTRL_ADDR
;wait for others
    ____loopWait:
        lsu0.ld32   i1, i17
        nop 5
        cmu.cmz.i32     i1
        peu.pc1c    NEQ || bru.bra  ____loopWait
        nop 5
        bru.jmp i30
        nop 5
