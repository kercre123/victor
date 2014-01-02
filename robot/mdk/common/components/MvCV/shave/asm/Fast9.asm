; ///
; /// @file
; /// @copyright All code copyright Movidius Ltd 2013, all rights reserved.
; ///            For License Warranty see: common/license.txt
; ///
; /// @brief
; ///
; FAST9 corner detection, early exclusion and pixel score calculation included
; input  : i18, pointer to parameters
; output : none


.version 00.51.04

.data .data.fast9_asm

.code .text.fast9_asm

fast9_asm:
;fast(u8** in_lines, u8* score,  unsigned int *base, u32 posy, u32 thresh, u32 width);
;         i18          i17               i16           i15        i14         i13   

LSU0.LD32  i1  i18 || IAU.ADD i18 i18 4
LSU0.LD32  i2  i18 || IAU.ADD i18 i18 4
LSU0.LD32  i3  i18 || IAU.ADD i18 i18 4
LSU0.LD32  i4  i18 || IAU.ADD i18 i18 4
LSU0.LD32  i5  i18 || IAU.ADD i18 i18 4
LSU0.LD32  i6  i18 || IAU.ADD i18 i18 4
LSU0.LD32  i7  i18 || IAU.ADD i18 i18 4
CMU.CPII i10, i14 ;threshold
CMU.CPII i14, i13 ;copy of width
CMU.CPII i18, i16 ;pointer to corner candidates buffer   
CMU.CPII i16, i14 ;width
IAU.SHL i15 i15 0x10
IAU.SUB i8 i8 i8
IAU.OR  i8 i15 i8

LSU1.LDIL i11, 0x01FF
LSU1.LDIH i11, 0x03FE
CMU.CPIV.x32 v5.0,  i30 
LSU1.LDIL i30, ___FAST_Exclude
LSU1.LDIH i30, ___FAST_Exclude


; takeoff
CMU.VSZMBYTE i0, i8, [ZZ10] 
CMU.CPII i15, i8             || IAU.ADD.u32s i8,  i4, i0  
CMU.CPIVR.x8 v4, i10         || IAU.ADD.u32s i9,  i1, i0      || LSU0.LDO64.h v6,  i8, -8
                                IAU.ADD.u32s i10, i7, i0	  || LSU0.LDI64.l v7,  i8 
                                SAU.ROL.x16 i12, i11, 0x02    || LSU0.LDI64.h v7,  i8
CMU.CPIV.x32 v14.0, i11      || SAU.ROL.x16 i11, i11, 0x04    || LSU0.LDI64.l v8,  i8
CMU.CPIV.x32 v14.1, i12      || SAU.ROL.x16 i12, i12, 0x04    || LSU0.LDI64.h v8,  i8
CMU.CPIV.x32 v14.2, i11      || SAU.ROL.x16 i11, i11, 0x04    || LSU0.LDI64.l v9,  i9
CMU.CPIV.x32 v14.3, i12      || SAU.ROL.x16 i12, i12, 0x04    || LSU0.LDI64.h v9,  i9 
CMU.CPIV.x32 v15.0, i11      || SAU.ROL.x16 i11, i11, 0x04    || LSU0.LDI64.l v10, i10
CMU.CPIV.x32 v15.1, i12      || SAU.ROL.x16 i12, i12, 0x04    || LSU0.LDI64.h v10, i10 
CMU.CPIV.x32 v15.2, i11      || VAU.ALIGNVEC v11, v6, v7, 0xD || LSU1.LDIL  i0, 0
CMU.CPIV.x32 v15.3, i12      || VAU.ALIGNVEC v12, v7, v8, 0x3 || LSU0.STI32 i0, i18
CMU.CPIV.x32 v16.0, i18      || VAU.AND v6, v7, v7            || LSU0.LDI64.l v8,  i8
CMU.CPIV.x32 v16.1, i0       || VAU.AND v7, v8, v8            || LSU0.LDI64.h v8,  i8

; warp zone
.lalign
___FAST_Exclude:
; calculate absolute differences early on
CMU.CPZV v17, 0x1  || IAU.SUB.u32s i16, i16, 0x10 || VAU.ADIFF.u8 v0,  v9,  v6     || LSU0.LDI64.l v9,  i9
PEU.PCIX.EQ 0x2    || SAU.SUM.u32 i30, v5, 0x1    || VAU.ADIFF.u8 v0,  v10, v6     || LSU0.LDI64.h v9,  i9
; check if both p1 and p9 are inside range
                      CMU.CMVV.u8 v0,  v4         || VAU.ADIFF.u8 v11, v11, v6     || LSU0.LDI64.l v10, i10 
PEU.ANDACC         || CMU.CMVV.u8 v0,  v4         || VAU.ADIFF.u8 v12, v12, v6     || LSU0.LDI64.h v10, i10
; ultra early rejection
PEU.PC16C.AND 0x4  || BRU.JMP i30                 || IAU.ADD.u32s i15, i15, 0x10  
; check if both p5 and p13 are inside range	
PEU.PVV8 0x4       || CMU.CMVV.u8 v11, v4         || VAU.NOT v18, v18              || LSU0.STI64.l v17, i17
PEU.ANDACC         || CMU.CMVV.u8 v12, v4         || VAU.ALIGNVEC v11, v6, v7, 0xD || LSU0.STI64.h v17, i17 
					  CMU.CPVV v6, v7             || VAU.ALIGNVEC v12, v7, v8, 0x3 || LSU0.LDI64.l v8,  i8
					  CMU.CPVV v7, v8                                              || LSU0.LDI64.h v8,  i8
PEU.PVV8 0x4        							  || VAU.NOT v17, v17
; OR together detection results we should do a full search only if at least one of both pairs exceeds range		
                                                     VAU.OR  v18, v18, v17 
                 	  CMU.CMZ.i8 v18              || LSU1.LDIH i14, 0xFFFF	
; early rejection					  
PEU.PC16C.AND 0x4  || BRU.JMP i30                 || IAU.ADD.u32s i15, i15, 0x10 
                      CMU.VNZ.x8  i14, v18, 0			  

___FAST_FullSearch:
                                  IAU.BSFINV  i13, i14  || LSU1.LDIL i12, 0x01  
CMU.VSZMBYTE i0, i15, [ZZ10]   || IAU.SHL i12, i12, i13
                                  IAU.ADD i12, i13, i0  || SAU.OR i14, i14, i12
SAU.ADD.u32s i13, i13, i17     || IAU.ADD i0, i7, i12    
CMU.CPIV.x32 v5.2, i9          || IAU.ADD i0, i6, i12   || LSU0.LDO32 i12, i0, -1
CMU.CPIV.x32 v5.3, i10         || IAU.ADD i0, i5, i12   || LSU0.LDO64 i12, i0, -2  
CMU.VSZMBYTE i15, i12, [DD10]  || IAU.ADD i0, i4, i12   || LSU0.LDO64 i12, i0, -3 
SAU.SUB.u32s i13, i13, 0x10    || IAU.ADD i0, i3, i12   || LSU0.LDO64 i12, i0, -3
                                  IAU.ADD i0, i2, i12   || LSU0.LDO64 i12, i0, -3			  
CMU.CPIV.x32 v16.2, i13        || IAU.ADD i0, i1, i12   || LSU0.LDO64 i12, i0, -2
CMU.VSZMBYTE i10, i12, [Z012]                           || LSU0.LDO32 i12, i0, -1
                                  IAU.FINS i10, i12, 0x18, 0x08	|| SAU.SWZBYTE i9, i13, [0000] || PEU.PSEN4BYTE 0x8										
CMU.CPIV.x32 v1.2, i10         || IAU.FINS i11, i12, 0x00, 0x08 || SAU.SWZBYTE i9, i13, [2222] || PEU.PSEN4BYTE 0x4
CMU.VSZMBYTE i0, i12, [ZZZ3]   || IAU.FINS i11, i12, 0x08, 0x08 || SAU.SWZBYTE i9, i13, [2222] || PEU.PSEN4BYTE 0x2
CMU.CPIVR.x8 v0, i0            || IAU.FINS i11, i12, 0x10, 0x08 || SAU.SWZBYTE i9, i13, [2222] || PEU.PSEN4BYTE 0x1
CMU.CPIV.x32 v1.1, i9          || IAU.FINS i11, i12, 0x18, 0x08 || VAU.ADD.u8s v2, v0, v4 
CMU.CPIV.x32 v1.3, i11		   || IAU.FINS i12, i13, 0x18, 0x08 || VAU.SUB.u8s v3, v0, v4
CMU.CPIV.x32 v1.0, i12                                    
; mark passing pixels
 VAU.SUB.u8s v2, v1, v2  || LSU1.LDIH i12, 0 
 VAU.SUB.u8s v3, v3, v1  || LSU1.LDIH i13, 0
; get into irf comparison results
CMU.VNZ.x8  i12, v2, 0x0                        
CMU.VNZ.x8  i13, v3, 0x0 || IAU.ONES  i11, i12   
CMU.CPVI.x32 i9,  v5.2   ||  IAU.ONES  i0,  i13
; comparison results locked as bitmask, wreak havoc in order to find a match 
                     CMU.CPIVR.x16 v13, i12   || IAU.SUB.u32s i0, i0, i11     || SAU.SUM.u8  i11, v2, 0xF
PEU.PCIX.NEQ 0x06 || CMU.CPIVR.x16 v13, i13                                   || SAU.SUM.u8  i11, v3, 0xF 
                     CMU.CPVI.x32 i12, v16.1  || IAU.NOT i0, i14              || VAU.AND v2, v13, v14
PEU.PCIX.NEQ 0    || CMU.CPVI.x32 i13, v16.0  || BRU.BRA ___FAST_FullSearch      || VAU.AND v3, v13, v15    
PEU.PCIX.EQ  0    || CMU.CMVV.u16 v2, v14     || BRU.JMP i30                  || IAU.XOR i0, i0, i0 
PEU.ORACC         || CMU.CMVV.u16 v3, v15                                     || SAU.ADD.u32s i12, i12, 0x01 
; final predicate and store candidate    
PEU.PC8C.OR 0x1   || CMU.CPVI.x32 i10, v16.2  || IAU.SHR.u32 i11, i11, 0x04   || LSU0.STI32 i15, i18  
; update candidate number
PEU.PC8C.OR 0x1   || CMU.CPIV.x32 v16.1, i12  || IAU.ADD.u32s i0, i0, 0x20    || LSU0.ST32  i12, i13 
; update score buffer
PEU.PCIX.NEQ 0x10 || CMU.CPVI.x32 i10, v5.3   || IAU.FINS i15, i0, 0x00, 0x04 || LSU0.ST8   i11, i10
                                                 IAU.ADD.u32s i15, i15, 0x10

.end
 

