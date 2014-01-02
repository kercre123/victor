; ///
; /// @file
; /// @copyright All code copyright Movidius Ltd 2012, all rights reserved.
; ///            For License Warranty see: common/license.txt
; ///
; /// @brief    
; ///

.version 00.51.05
;-------------------------------------------------------------------------------

.set DMA_BUSY_MASK		0x0008
.set DMA_TASK_REQ 		0x4004
.set DMA_TASK_STAT 		0x4008
.set DMA0_CFG			0x4100
.set DMA0_SRC_ADDR		0x4104
.set DMA0_DST_ADDR		0x4108
.set DMA0_SIZE			0x410C
.set DMA0_LINE_WIDTH	0x4110
.set DMA0_LINE_STRIDE	0x4114
.set P_GPI              0x1E

.code .text.yuv444_to_yuv420
.salign 16

yuv444_to_yuv420:

		IAU.SHL i1 i16 1 ; width * 2
		IAU.SUB i19 i19 i1 ;space for two lines
		IAU.SUB i3 i19 0 ;pointer to first line
		IAU.ADD i4 i19 i16 ;pointer to second line(processed line)
		LSU0.LDIL i5, loop  || LSU1.LDIH i5, loop ;loop for dma transfer
		IAU.SUB i6 i6 i6 ;contor for loop
		
		;copy chroma
		loop:
		lsu1.ldil  i7, 1
		LSU0.STA32 i7, SLICE_LOCAL, DMA0_CFG
		LSU0.STA32 i3, SLICE_LOCAL, DMA0_DST_ADDR
		LSU0.STA32 i18, SLICE_LOCAL, DMA0_SRC_ADDR
		LSU0.STA32 i16, SLICE_LOCAL, DMA0_SIZE          || LSU1.ldil  i7, 0
		LSU0.STA32 i7, SLICE_LOCAL, DMA_TASK_REQ

		nop 20
		LSU1.LDIL i7, DMA_BUSY_MASK
		CMU.CMTI.BITP i7, P_GPI
		PEU.PCCX.NEQ 0x3F || BRU.RPIM 0 || CMU.CMTI.BITP i7, P_GPI
		
		
		lsu1.ldil  i7, 1
		LSU0.STA32 i7, SLICE_LOCAL, DMA0_CFG
		LSU0.STA32 i17, SLICE_LOCAL, DMA0_DST_ADDR
		LSU0.STA32 i3, SLICE_LOCAL, DMA0_SRC_ADDR
		LSU0.STA32 i16, SLICE_LOCAL, DMA0_SIZE          || LSU1.ldil  i7, 0
		LSU0.STA32 i7, SLICE_LOCAL, DMA_TASK_REQ

		nop 20
		LSU1.LDIL i7, DMA_BUSY_MASK
		CMU.CMTI.BITP i7, P_GPI
		PEU.PCCX.NEQ 0x3F || BRU.RPIM 0 || CMU.CMTI.BITP i7, P_GPI
		
		IAU.ADD i18 i18 i16
		IAU.ADD i17 i17 i16
		
		IAU.ADD i6 i6 1
		CMU.CMII.i32 i6 i15
		PEU.PC1C LT || BRU.JMP i5
		nop 5
		
		LSU0.LDIL i5, loop1  || LSU1.LDIH i5, loop1 ;loop for dma transfer
		LSU0.LDIL i8, loop2  || LSU1.LDIH i8, loop2 ;loop for dma transfer
		IAU.SUB i6 i6 i6 ;reset contor for loop
		IAU.SUB i9 i3 0 ;copy for first line 
		IAU.SUB i10 i4 0;copy for second line
		IAU.SUB i12 i12 i12 ;contor for line
		IAU.SHR.u32 i14 i16 1 ;width/2
				
		;copy chromas :U-plane first
		loop1:
		lsu1.ldil  i7, 1
		LSU0.STA32 i7, SLICE_LOCAL, DMA0_CFG
		LSU0.STA32 i3, SLICE_LOCAL, DMA0_DST_ADDR
		LSU0.STA32 i18, SLICE_LOCAL, DMA0_SRC_ADDR
		LSU0.STA32 i16, SLICE_LOCAL, DMA0_SIZE          || LSU1.ldil  i7, 0
		LSU0.STA32 i7, SLICE_LOCAL, DMA_TASK_REQ
		nop 20
		LSU1.LDIL i7, DMA_BUSY_MASK
		CMU.CMTI.BITP i7, P_GPI
		PEU.PCCX.NEQ 0x3F || BRU.RPIM 0 || CMU.CMTI.BITP i7, P_GPI
		
		loop2:
		LSU0.LD8  i11  i9
		IAU.ADD i9 i9 2
		IAU.ADD i12 i12 2
		CMU.CMII.i32 i12 i16
		nop 2
		LSU0.ST8  i11  i10
		IAU.ADD i10 i10 1
		PEU.PC1C LT   || BRU.JMP i8
		nop 5

		lsu1.ldil  i7, 1
		LSU0.STA32 i7, SLICE_LOCAL, DMA0_CFG
		LSU0.STA32 i17, SLICE_LOCAL, DMA0_DST_ADDR
		LSU0.STA32 i4, SLICE_LOCAL, DMA0_SRC_ADDR
		LSU0.STA32 i14, SLICE_LOCAL, DMA0_SIZE          || LSU1.ldil  i7, 0
		LSU0.STA32 i7, SLICE_LOCAL, DMA_TASK_REQ
		nop 20
		LSU1.LDIL i7, DMA_BUSY_MASK
		CMU.CMTI.BITP i7, P_GPI
		PEU.PCCX.NEQ 0x3F || BRU.RPIM 0 || CMU.CMTI.BITP i7, P_GPI
		
		
		
		IAU.SUB i9 i3 0 ;copy for first line 
		IAU.SUB i10 i4 0;copy for second line
		IAU.ADD i18 i18 i16
		IAU.ADD i18 i18 i16 ;skip one line for resizing
		IAU.ADD i17 i17 i14 ;increment output line with width/2
		IAU.SUB i12 i12 i12 ;reseting line contor
		
		IAU.ADD i6 i6 2
		CMU.CMII.i32 i6 i15
		PEU.PC1C LT || BRU.JMP i5
		nop 5
		
		
		LSU0.LDIL i5, loop3  || LSU1.LDIH i5, loop3 ;loop for dma transfer
		LSU0.LDIL i8, loop4  || LSU1.LDIH i8, loop4 ;loop for dma transfer
		IAU.SUB i6 i6 i6 ;reset contor for loop
		IAU.SUB i9 i3 0 ;copy for first line 
		IAU.SUB i10 i4 0;copy for second line
		IAU.SUB i12 i12 i12 ;contor for line
				
		;copy chromas :V-plane second
		loop3:
		lsu1.ldil  i7, 1
		LSU0.STA32 i7, SLICE_LOCAL, DMA0_CFG
		LSU0.STA32 i3, SLICE_LOCAL, DMA0_DST_ADDR
		LSU0.STA32 i18, SLICE_LOCAL, DMA0_SRC_ADDR
		LSU0.STA32 i16, SLICE_LOCAL, DMA0_SIZE          || LSU1.ldil  i7, 0
		LSU0.STA32 i7, SLICE_LOCAL, DMA_TASK_REQ
		nop 20
		LSU1.LDIL i7, DMA_BUSY_MASK
		CMU.CMTI.BITP i7, P_GPI
		PEU.PCCX.NEQ 0x3F || BRU.RPIM 0 || CMU.CMTI.BITP i7, P_GPI
		
		loop4:
		LSU0.LD8  i11  i9
		IAU.ADD i9 i9 2
		IAU.ADD i12 i12 2
		CMU.CMII.i32 i12 i16
		nop 2
		LSU0.ST8  i11  i10
		IAU.ADD i10 i10 1
		PEU.PC1C LT   || BRU.JMP i8
		nop 5
		
		lsu1.ldil  i7, 1
		LSU0.STA32 i7, SLICE_LOCAL, DMA0_CFG
		LSU0.STA32 i17, SLICE_LOCAL, DMA0_DST_ADDR
		LSU0.STA32 i4, SLICE_LOCAL, DMA0_SRC_ADDR
		LSU0.STA32 i14, SLICE_LOCAL, DMA0_SIZE          || LSU1.ldil  i7, 0
		LSU0.STA32 i7, SLICE_LOCAL, DMA_TASK_REQ
		nop 20
		LSU1.LDIL i7, DMA_BUSY_MASK
		CMU.CMTI.BITP i7, P_GPI
		PEU.PCCX.NEQ 0x3F || BRU.RPIM 0 || CMU.CMTI.BITP i7, P_GPI
		
		
		
		IAU.SUB i9 i3 0 ;copy for first line 
		IAU.SUB i10 i4 0;copy for second line
		IAU.ADD i18 i18 i16
		IAU.ADD i18 i18 i16 ;skip one line for resizing
		IAU.ADD i17 i17 i14 ;increment output line with width/2
		IAU.SUB i12 i12 i12 ;reseting line contor
		
		IAU.ADD i6 i6 2
		CMU.CMII.i32 i6 i15
		PEU.PC1C LT || BRU.JMP i5
		nop 5
		
		bru.swih 0x1f
		nop 5
.end
