.version 00.50.00

;/; ///
; /// @file
; /// @copyright All code copyright Movidius Ltd 2013, all rights reserved.
; ///            For License Warranty see: common/license.txt
; ///
; /// @brief   : Containd a function that applay downscale 2x horizontal with a gaussian filters with kernel 5x5. 
;                 Have to be use in combination with GaussVx2 for obtain correct output. 
;                 Gaussian 5x5 filter was decomposed in liniar 1x5, applayed horizontal and vertical
;                 Function not make replicate at margin, assumed the you have 3 more pixels on left and right from input buffer.


;FUNCTIONS DECLARATION
; ---------------------------------------------------------------------------
;void GaussHx2(unsigned char *inLine,unsigned char *outLine, int width)l
;  ***************************************************************************


;==================================================================================================================
;=================================== GAUSSIAN DOWNSCALE 2X HORIZONTAL ================START========================
;==================================================================================================================
;                                         i18                      i17            i16  
;void GaussHx2(unsigned char *inLine,unsigned char *outLine, int width)
;{
;    int i;
;    for (i = 0; i<(width<<1);i+=2)
;    {
;        outLine[i>>1] = (((inLine[i-2] + inLine[i+2]) * gaus_matrix[0]) + ((inLine[i-1] + inLine[i+1]) * gaus_matrix[1]) + (inLine[i]  * gaus_matrix[2]))>>8;
;    }
;}
; gauss horizontal filter
; i18         : unsigned char *inLine //input line
; i17         : unsigned char *outLine //output buffer
; i16        : int width //number of pixels
.code .text.MvCV_GaussHx2
.salign 16

.lalign
GaussHx2:
LSU0.LDI128.u8.u16 v0, i18      ||LSU1.LDO16 i3, i18, -2    ||iau.sub i6, i6, i6
LSU0.LDI128.u8.u16 v1, i18      ||LSU1.LDO8 i6, i18, 16        
LSU0.LDIL i0, 16                ||LSU1.LDIL i1, 64          ||iau.sub i3, i3, i3
LSU1.LDIL i2, 96                ||CMU.CPIVR.x16 v18, i0    
CMU.CPIVR.x16 v17, i1           ||iau.sub i4, i4, i4        ||lsu0.ldil i8, ____GaussDownH_Loop ||lsu1.ldih i8, ____GaussDownH_Loop
CMU.CPIVR.x16 v16, i2 
iau.sub i5, i5, i5
cmu.vdilv.x16 v5,v4, v0, v1     ||LSU0.LDI128.u8.u16 v0, i18||LSU1.LDO16 i3, i18, -2    
VAU.MACPZ.u16 v4, v16           ||CMU.VROT v6, v4, 2        ||IAU.FEXT.u32 i4, i3, 0, 8     ||LSU0.LDI128.u8.u16 v1, i18     ||LSU1.LDO8 i6, i18, 16        
VAU.MACP.u16  v5, v17           ||CMU.SHLIV.x16 v4, i4      ||IAU.FEXT.u32 i5, i3, 8, 8
VAU.MACP.u16  v4, v18           ||CMU.SHLIV.x16 v5, i5 
VAU.MACP.u16  v5, v17           ||CMU.CPIV.x16 v6.7 i6.l
VAU.MACPW.u16 v7, v18, v6 
iau.incs i16, -9
cmu.vdilv.x16 v5,v4, v0, v1     ||LSU0.LDI128.u8.u16 v0, i18||LSU1.LDO16 i3, i18, -2
VAU.MACPZ.u16 v4, v16           ||CMU.VROT v6, v4, 2        ||IAU.FEXT.u32 i4, i3, 0, 8     ||LSU0.LDI128.u8.u16 v1, i18     ||LSU1.LDO8 i6, i18, 16        
VAU.MACP.u16  v5, v17           ||CMU.SHLIV.x16 v4, i4      ||IAU.FEXT.u32 i5, i3, 8, 8
VAU.MACP.u16  v4, v18           ||CMU.SHLIV.x16 v5, i5 
VAU.MACP.u16  v5, v17           ||CMU.CPIV.x16 v6.7 i6.l
VAU.MACPW.u16 v7, v18, v6       ||CMU.VSZMBYTE v8, v7, [Z3Z1]
iau.shr.u32 i7, i16, 3
;// loop
nop 3
.lalign 
bru.rpl i8, i7                  ||cmu.vdilv.x16 v5,v4, v0, v1   || LSU0.LDI128.u8.u16 v0, i18||LSU1.LDO16 i3, i18, -2    
____GaussDownH_Loop:
VAU.MACPZ.u16 v4, v16           || CMU.VROT v6, v4, 2           ||IAU.FEXT.u32 i4, i3, 0, 8 ||LSU0.LDI128.u8.u16 v1, i18     ||LSU1.LDO8 i6, i18, 16        
VAU.MACP.u16  v5, v17           || CMU.SHLIV.x16 v4, i4         ||IAU.FEXT.u32 i5, i3, 8, 8
VAU.MACP.u16  v4, v18           || CMU.SHLIV.x16 v5, i5         ||LSU0.STI128.u16.u8 v8, i17
VAU.MACP.u16  v5, v17           || CMU.CPIV.x16 v6.7 i6.l    
VAU.MACPW.u16 v7, v18, v6       || CMU.VSZMBYTE v8, v7, [Z3Z1]
nop
;//end loop
BRU.JMP i30                     ||LSU0.STI128.u16.u8 v8, i17
nop 2
CMU.VSZMBYTE v8, v7, [Z3Z1]
LSU0.STI128.u16.u8 v8, i17
nop 
;==================================================================================================================
;=================================== GAUSSIAN DOWNSCALE 2X HORIZONTAL ================ END ========================
;==================================================================================================================

.end


