; ///
; /// @file
; /// @copyright All code copyright Movidius Ltd 2012, all rights reserved.
; ///            For License Warranty see: common/license.txt
; ///
; /// @brief
; ///

.version 00.51.04

.include svuCommonDefinitions.incl

.code .text.scGetShaveNumber ;text

; Read own Shave ID field from TRF
scGetShaveNumber:
    bru.swp i30 || cmu.cpti i18, P_SVID
    nop 5

.code .text.scFifoRead
; Read 1 word from own FIFO
; i18: Shave number
scFifoRead:
    bru.swp i30 || lsu0.lda32 i18, SLICE_LOCAL, FIFO_RD_FRONT_L
    nop 5

.code .text.scFifoWrite
; Write 2 words to a Shave's FIFO
; void scFifoWrite(int svu_no, int word0, int word1);
; i18: Shave number
; i17: first word to write
; i16: second word to write
scFifoWrite:
    cmu.cpiv v0.0, i17 || lsu1.ldil i2, SHAVE0_FIFO || lsu0.ldil i3, 0x100
    cmu.cpiv v0.1, i16 || lsu0.ldih i1, 0x0FF0 || lsu1.ldil i1, 0x0000
    iau.mul i3, i3, i18
    nop
    iau.add i2, i2, i3
    iau.add i1, i1, i2
    bru.swp i30 || lsu0.st64.l v0, i1
    nop 5
    
.code .text.scFifoOwnClear
; Write anything to clear own FIFO
scFifoOwnClear:
    bru.swp i30 || lsu0.ldil i0, FIFO_CLEAR
    lsu0.sta32 i0, SLICE_LOCAL, FIFO_CLEAR
    nop 4