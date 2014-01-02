; ///
; /// @file
; /// @copyright All code copyright Movidius Ltd 2013, all rights reserved.
; ///            For License Warranty see: common/license.txt
; ///
; /// @brief
; ///

.version 00.50.05
.data .rodata.pyrdown
.salign 16

.code .text.pyrdown ;text
.salign 16
;==================================================================================================================
;==================================== GAUSSIAN DOWNSCALE 2X VERTICAL ================= END ========================
;==================================================================================================================
pyrdown_asm:
LSU0.LD32 i17 i17
IAU.SUB i10 i16 0
IAU.SUB i13 i16 0
IAU.SUB i1 i1 i1
IAU.SUB i19 i19 1
LSU0.ST8  i1  i19  
IAU.SUB i19 i19 1
LSU0.ST8  i1  i19    
IAU.SUB i19 i19 i16
IAU.SUB i11 i19 0
IAU.SUB i12 i19 0
IAU.SUB i19 i19 1
LSU0.ST8  i1  i19   
IAU.SUB i19 i19 1
LSU0.ST8  i1  i19   
LSU0.LDO64 i0,  i18, 0x00       ||LSU1.LDIL i5, 16
LSU0.LDO64 i2,  i18, 0x08       ||LSU1.LDIL i6, 64              		||CMU.CPIVR.x16 v18, i5
LSU0.LDO32 i4,  i18, 0x10       ||LSU1.LDIL i9, 96              		||CMU.CPIVR.x16 v17, i6 
lsu0.ldil i8, ____GaussDownV_Loop   ||lsu1.ldih i8, ____GaussDownV_Loop ||CMU.CPIVR.x16 v16, i9 
iau.incs i16, -1
iau.shr.u32 i7, i16, 3
LSU0.LDI128.u8.u16 v0, i0
LSU0.LDI128.u8.u16 v1, i1
LSU0.LDI128.u8.u16 v2, i2
LSU0.LDI128.u8.u16 v3, i3
LSU0.LDI128.u8.u16 v4, i4
LSU0.LDI128.u8.u16 v0, i0
VAU.MACPZ.u16 v0, v18           ||LSU0.LDI128.u8.u16 v1, i1
VAU.MACP.u16  v1, v17           ||LSU0.LDI128.u8.u16 v2, i2
VAU.MACP.u16  v2, v16           ||LSU0.LDI128.u8.u16 v3, i3
VAU.MACP.u16  v3, v17           ||LSU0.LDI128.u8.u16 v4, i4
VAU.MACPW.u16 v7, v18, v4 
;//loop
nop 4
.lalign
____GaussDownV_Loop:
bru.rpl i8, i7                  ||LSU0.LDI128.u8.u16 v0, i0    
VAU.MACPZ.u16 v0, v18           ||LSU0.LDI128.u8.u16 v1, i1
VAU.MACP.u16  v1, v17           ||LSU0.LDI128.u8.u16 v2, i2
VAU.MACP.u16  v2, v16           ||LSU0.LDI128.u8.u16 v3, i3    ||CMU.VSZMBYTE v7, v7, [Z3Z1]
VAU.MACP.u16  v3, v17           ||LSU0.LDI128.u8.u16 v4, i4    ||LSU1.STI128.u16.u8 v7, i11
VAU.MACPW.u16 v7, v18, v4     
;// end loop
nop 3
CMU.VSZMBYTE v7, v7, [Z3Z1]
LSU1.STI128.u16.u8 v7, i11
nop 

;==================================================================================================================
;=================================== GAUSSIAN DOWNSCALE 2X HORIZONTAL ================START========================
;==================================================================================================================


LSU0.LDI128.u8.u16 v0, i12      ||LSU1.LDO16 i3, i12, -2    ||iau.sub i6, i6, i6
LSU0.LDI128.u8.u16 v1, i12      ||LSU1.LDO8 i6, i12, 16        
LSU0.LDIL i0, 16                ||LSU1.LDIL i1, 64          ||iau.sub i3, i3, i3
LSU1.LDIL i2, 96                ||CMU.CPIVR.x16 v18, i0    
CMU.CPIVR.x16 v17, i1           ||iau.sub i4, i4, i4        ||lsu0.ldil i8, ____GaussDownH_Loop ||lsu1.ldih i8, ____GaussDownH_Loop
CMU.CPIVR.x16 v16, i2 
iau.sub i5, i5, i5
cmu.vdilv.x16 v5,v4, v0, v1     ||LSU0.LDI128.u8.u16 v0, i12||LSU1.LDO16 i3, i12, -2    
VAU.MACPZ.u16 v4, v16           ||CMU.VROT v6, v4, 2        ||IAU.FEXT.u32 i4, i3, 0, 8     ||LSU0.LDI128.u8.u16 v1, i12     ||LSU1.LDO8 i6, i12, 16        
VAU.MACP.u16  v5, v17           ||CMU.SHLIV.x16 v4, i4      ||IAU.FEXT.u32 i5, i3, 8, 8
VAU.MACP.u16  v4, v18           ||CMU.SHLIV.x16 v5, i5 
VAU.MACP.u16  v5, v17           ||CMU.CPIV.x16 v6.7 i6.l
VAU.MACPW.u16 v7, v18, v6 
iau.incs i16, -9
cmu.vdilv.x16 v5,v4, v0, v1     ||LSU0.LDI128.u8.u16 v0, i12||LSU1.LDO16 i3, i12, -2
VAU.MACPZ.u16 v4, v16           ||CMU.VROT v6, v4, 2        ||IAU.FEXT.u32 i4, i3, 0, 8     ||LSU0.LDI128.u8.u16 v1, i12     ||LSU1.LDO8 i6, i12, 16        
VAU.MACP.u16  v5, v17           ||CMU.SHLIV.x16 v4, i4      ||IAU.FEXT.u32 i5, i3, 8, 8
VAU.MACP.u16  v4, v18           ||CMU.SHLIV.x16 v5, i5 
VAU.MACP.u16  v5, v17           ||CMU.CPIV.x16 v6.7 i6.l
VAU.MACPW.u16 v7, v18, v6       ||CMU.VSZMBYTE v8, v7, [Z3Z1]
iau.shr.u32 i7, i16, 3
;// loop
nop 3
.lalign 
bru.rpl i8, i7                  ||cmu.vdilv.x16 v5,v4, v0, v1   || LSU0.LDI128.u8.u16 v0, i12||LSU1.LDO16 i3, i12, -2    
____GaussDownH_Loop:
VAU.MACPZ.u16 v4, v16           || CMU.VROT v6, v4, 2           ||IAU.FEXT.u32 i4, i3, 0, 8 ||LSU0.LDI128.u8.u16 v1, i12     ||LSU1.LDO8 i6, i12, 16        
VAU.MACP.u16  v5, v17           || CMU.SHLIV.x16 v4, i4         ||IAU.FEXT.u32 i5, i3, 8, 8
VAU.MACP.u16  v4, v18           || CMU.SHLIV.x16 v5, i5         ||LSU0.STI128.u16.u8 v8, i17
VAU.MACP.u16  v5, v17           || CMU.CPIV.x16 v6.7 i6.l    
VAU.MACPW.u16 v7, v18, v6       || CMU.VSZMBYTE v8, v7, [Z3Z1]
nop
;//end loop
BRU.JMP i30                     ||LSU0.STI128.u16.u8 v8, i17
nop 
IAU.ADD i19 i19 4
CMU.VSZMBYTE v8, v7, [Z3Z1]
LSU0.STI128.u16.u8 v8, i17
IAU.ADD i19 i19 i13


.end