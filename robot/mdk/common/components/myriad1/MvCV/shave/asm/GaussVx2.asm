.version 00.50.00
; ///
; /// @file
; /// @copyright All code copyright Movidius Ltd 2013, all rights reserved.
; ///            For License Warranty see: common/license.txt
; ///
; /// @brief  Containd a function that applay downscale 2x vertical with a gaussian filters with kernel 5x5. 
;                 Have to be use in combination with GaussHx2 for obtain correct output. 
;                 Gaussian 5x5 filter was decomposed in liniar 1x5, applayed horizontal and vertical


;FUNCTIONS DECLARATION
; ---------------------------------------------------------------------------
;void GaussVx2(unsigned char **inLine,unsigned char *outLine, int width)l
;  ***************************************************************************


;==================================================================================================================
;==================================== GAUSSIAN DOWNSCALE 2X VERTICAL =================START========================
;==================================================================================================================
;                                         i18                      i17            i16  
;void GaussVx2(unsigned char **inLine,unsigned char *outLine, int width)
;{
;    int i;
;    for (i = 0; i<((width));i++)
;    {
;        outLine[i] = (((inLine[0][i] + inLine[4][i]) * gaus_matrix[0]) + ((inLine[1][i] + inLine[3][i]) * gaus_matrix[1]) + (inLine[2][i]  * gaus_matrix[2]))>>8;
;    }
;}
; gauss vertical filter
; i18         : unsigned char **inLine //input lines 5 lines 
; i17         : unsigned char *outLine //output buffer
; i16        : int width //number of pixels
; .code MvCV_GaussVx2
; .salign 16

.code .text.GaussVx2
.lalign
GaussVx2:
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
VAU.MACP.u16  v3, v17           ||LSU0.LDI128.u8.u16 v4, i4    ||LSU1.STI128.u16.u8 v7, i17
VAU.MACPW.u16 v7, v18, v4     
;// end loop
BRU.JMP i30  
nop 2
CMU.VSZMBYTE v7, v7, [Z3Z1]
LSU1.STI128.u16.u8 v7, i17
nop 
;==================================================================================================================
;==================================== GAUSSIAN DOWNSCALE 2X VERTICAL ================= END ========================
;==================================================================================================================


.end


