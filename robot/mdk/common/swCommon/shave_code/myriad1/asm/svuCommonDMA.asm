; ///
; /// @file
; /// @copyright All code copyright Movidius Ltd 2012, all rights reserved.
; ///            For License Warranty see: common/license.txt
; ///
; /// @brief
; ///

.version 00.51.04

.include svuCommonDefinitions.incl

.code .text.scDmaCopyBlocking ;text

;void scDmaCopyBlocking(unsigned char *dst, unsigned char *src, unsigned char len)
scDmaCopyBlocking:

    lsu1.ldil  i15, 1
    LSU0.STA32 i15, SLICE_LOCAL, DMA0_CFG
    LSU0.STA32 i18, SLICE_LOCAL, DMA0_DST_ADDR
    LSU0.STA32 i17, SLICE_LOCAL, DMA0_SRC_ADDR
    LSU0.STA32 i16, SLICE_LOCAL, DMA0_SIZE          || LSU1.ldil  i15, 0
    LSU0.STA32 i15, SLICE_LOCAL, DMA_TASK_REQ

    nop 20
    LSU1.LDIL i15, DMA_BUSY_MASK
    CMU.CMTI.BITP i15, P_GPI
    PEU.PCCX.NEQ 0x3F || BRU.RPIM 0 || CMU.CMTI.BITP i15, P_GPI

 ;Return to func:
    bru.jmp i30
    nop 5

.code .text.scDmaSetup
;Parameters: task, src, dest, size
; store value of 0x1 in i1 to enable dma task
scDmaSetup:
    ;Make sure DMA is idle before we start it again 
    LSU1.LDIL i2, DMA_BUSY_MASK
    CMU.CMTI.BITP i2, P_GPI
    PEU.PCCX.NEQ 0x3F || BRU.RPIM 0 || CMU.CMTI.BITP i2, P_GPI
    ;Need to accomodate first parameter as integer, as such we must or the value of i18
    lsu0.ldil i0, 0x4000 || iau.add  i18, i18, 1
    iau.shl i18, i18, 8 
    iau.or i18, i18, i0  || bru.jmp     i30
    lsu0.ldil i0,0x0000             || lsu1.ldih i0, 0x0ff0
    iau.add i0, i0, i18             || lsu0.ldil i1, 0x01 
    lsu1.sto32 i17, i0, 0x04
    lsu0.sto32 i16, i0, 0x08        || lsu1.sto32 i15, i0, 0xC
    lsu0.sto32  i1, i0, 0x00

.code .text.scDmaSetupStrideSrc
;Parameters: task, src, dest, size, width, stride
scDmaSetupStrideSrc:
    ;Make sure DMA is idle before we start it again 
    LSU1.LDIL i2, DMA_BUSY_MASK
    CMU.CMTI.BITP i2, P_GPI
    PEU.PCCX.NEQ 0x3F || BRU.RPIM 0 || CMU.CMTI.BITP i2, P_GPI
    ;Need to accomodate first parameter as integer, as such we must or the value of i18
    lsu0.ldil i0, 0x4000 || iau.add  i18, i18, 1
    iau.shl i18, i18, 8 
    iau.or i18, i18, i0 
    lsu0.ldil i0,0x0000             || lsu1.ldih i0, 0x0ff0 || bru.jmp     i30
    iau.add i0, i0, i18 ||     lsu1.ldil  i1, DMA_EN_STRIDE_TX
    lsu0.sto32 i1, i0, 0x00
    lsu1.sto32 i17, i0, 0x04
    lsu0.sto32 i16, i0, 8           || lsu1.sto32 i15, i0, 0xC
    lsu0.sto32 i14, i0, 0x10        || lsu1.sto32 i13, i0, 0x14

.code .text.scDmaSetupStrideDst
;Parameters: task, src, dest, size, width, stride
scDmaSetupStrideDst:
    ;Make sure DMA is idle before we start it again 
    LSU1.LDIL i2, DMA_BUSY_MASK
    CMU.CMTI.BITP i2, P_GPI
    PEU.PCCX.NEQ 0x3F || BRU.RPIM 0 || CMU.CMTI.BITP i2, P_GPI
    ;Need to accomodate first parameter as integer, as such we must or the value of i18
    lsu0.ldil i0, 0x4000 || iau.add  i18, i18, 1
    iau.shl i18, i18, 8 
    iau.or i18, i18, i0 
    lsu0.ldil i0,0x0000             || lsu1.ldih i0, 0x0ff0 || bru.jmp     i30
    iau.add i0, i0, i18 ||     lsu1.ldil  i1, DMA_EN_STRIDE_RX
    lsu0.sto32 i1, i0, 0x00
    lsu1.sto32 i17, i0, 0x04
    lsu0.sto32 i16, i0, 8           || lsu1.sto32 i15, i0, 0xC
    lsu0.sto32 i14, i0, 0x10        || lsu1.sto32 i13, i0, 0x14

.code .text.scDmaSetupFull
;Parameters: task, cfg, src, dest, size, width, stride
scDmaSetupFull:
    ;Make sure DMA is idle before we start it again 
    LSU1.LDIL i1, DMA_BUSY_MASK
    CMU.CMTI.BITP i1, P_GPI
    PEU.PCCX.NEQ 0x3F || BRU.RPIM 0 || CMU.CMTI.BITP i1, P_GPI
    ;Need to accomodate first parameter as integer, as such we must or the value of i18
    lsu0.ldil i0, 0x4000 || iau.add  i18, i18, 1
    iau.shl i18, i18, 8 
    iau.or i18, i18, i0  || bru.jmp     i30
    lsu0.ldil i0,0x0000             || lsu1.ldih i0, 0x0ff0
    iau.add i0, i0, i18
    lsu0.sto32 i17, i0, 0x00        || lsu1.sto32 i16, i0, 0x04
    lsu0.sto32 i15, i0, 8           || lsu1.sto32 i14, i0, 0xC
    lsu0.sto32 i13, i0, 0x10        || lsu1.sto32 i12, i0, 0x14

.code .text.scDmaMemsetDDR
scDmaMemsetDDR:
	LSU0.LDIL i5, 0x100  || LSU1.LDIH i5, 0 ;base line with desired value on the stack
	IAU.SUB i4 i4 i4 ;temporary contor for filling the stack
	LSU0.LDIL i6, ___loop  || LSU1.LDIH i6, ___loop ; loop for filling the stack
	___loop:
		IAU.SUB i19 i19 1
		LSU0.ST8 i17  i19
		IAU.ADD i4 i4 1
		CMU.CMII.i32 i4 i5
		PEU.PC1C LT || BRU.JMP i6	
		nop 5
		
	LSU0.LDIL i6, ___loop2  || LSU1.LDIH i6, ___loop2 ;loading loop for dma tranzactions

	IAU.SUB i8 i4 0 
	IAU.SUB i10 i10 i10

	IAU.SUB i2 i15 i14
		
	CMU.CMII.i32 i16 i2
	PEU.PC1C LT || IAU.SUB i2 i16 0
	
	CMU.CMII.i32 i2 i4
	PEU.PC1C LT || IAU.SUB i8 i2 0
	
	

	
	___loop2:
		lsu1.ldil  i5, 1
    	LSU0.STA32 i5, SLICE_LOCAL, DMA0_CFG
    	LSU0.STA32 i18, SLICE_LOCAL, DMA0_DST_ADDR
    	LSU0.STA32 i19, SLICE_LOCAL, DMA0_SRC_ADDR
    	LSU0.STA32 i8, SLICE_LOCAL, DMA0_SIZE          || LSU1.ldil  i5, 0
    	LSU0.STA32 i5, SLICE_LOCAL, DMA_TASK_REQ
    	nop 20
    	LSU1.LDIL i5, DMA_BUSY_MASK
    	CMU.CMTI.BITP i5, P_GPI
    	PEU.PCCX.NEQ 0x3F || BRU.RPIM 0 || CMU.CMTI.BITP i5, P_GPI


    	IAU.ADD i18 i18 i8 ;incrementing destination adress
		IAU.SUB i2 i2 i8 ;decrementing bytes per line
		IAU.SUB i16 i16 i8
		
		CMU.CMII.i32 i2 i4
		PEU.PC1C LT ||  IAU.SUB i8 i2 0
		
		CMU.CMII.i32 i2 i10
		PEU.PC1C NEQ || BRU.JMP i6	
		nop 5
		;bru.swih 0x1f
		IAU.ADD i18 i18 i14
		IAU.SUB i8 i4 0
		
		IAU.SUB i2 i15 i14
		CMU.CMII.i32 i16 i2
		PEU.PC1C LT ||  IAU.SUB i2 i16 0
		
		
		CMU.CMII.i32 i16 i4
		PEU.PC1C LT ||  IAU.SUB i8 i16 0
		
		CMU.CMII.i32 i16 i10 
		PEU.PC1C NEQ || BRU.JMP i6	
		nop 5
		
	IAU.ADD i19 i19 i4
	bru.jmp i30 
   	nop 5

