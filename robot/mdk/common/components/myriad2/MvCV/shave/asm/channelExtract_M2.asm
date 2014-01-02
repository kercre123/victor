; ///
; /// @file
; /// @copyright All code copyright Movidius Ltd 2013, all rights reserved.
; ///            For License Warranty see: common/license.txt
; ///
; /// @brief
; ///

.version 00.51.04

.data .data.channelExtract

.code .text.channelExtract

channelExtract_asm:
LSU1.LD.32  i18  i18 || LSU0.LD.32  i17  i17
LSU0.LDIL i9 0x30 || LSU1.LDIH i9 0x0 || IAU.SUB i10 i10 i10 ;48 pixels processed & contor for loop
nop 4

;void channelExtract(u8** in, u8** out, u32 width, u8 plane)
;                       i18     i17       i16        i15     

LSU0.LDIL i1 ___loopR || LSU1.LDIH i1 ___loopR ;loop for R-plane
LSU0.LDIL i2 ___loopG || LSU1.LDIH i2 ___loopG ;loop for G-plane
LSU0.LDIL i3 ___loopB || LSU1.LDIH i3 ___loopB ;loop for B-plane

LSU0.LDIL i4 0x0780 || LSU1.LDIH i4 0 ;load max width
VAU.SUB.i8  v20 v20 v20

LSU0.LDIL i13 ___exit || LSU1.LDIH i13 ___exit ;if plane number is greater then 2
LSU0.LDIL i6 0x0 || LSU1.LDIH i8 0x0 ;compare element for R-plane
LSU0.LDIL i7 0x1 || LSU1.LDIH i7 0x0 ;compare element for G-plane
LSU0.LDIL i8 0x2 || LSU1.LDIH i8 0x0 ;compare element for B-plane

IAU.SUB i19 i19 4
LSU0.ST.32  i20  i19    || IAU.SUB i19 i19 4
LSU0.ST.32  i21  i19    || IAU.SUB i19 i19 4
LSU0.ST.32  i22  i19    || IAU.SUB i19 i19 4
LSU0.ST.32  i23  i19    || IAU.SUB i19 i19 4
LSU0.ST.32  i24  i19    || IAU.SUB i19 i19 4
LSU0.ST.32  i25  i19    || IAU.SUB i19 i19 4
LSU0.ST.32  i26  i19    

IAU.SUB i19 i19 i9 ; making space for pixels compensation
IAU.SUB i11 i19 0  ;making copy for stack adress
IAU.ADD i10 i10 i9



LSU0.LDIL i20 ___loopRR || LSU1.LDIH i20 ___loopRR ; compensation loop for R-plane
LSU0.LDIL i21 ___loopGG || LSU1.LDIH i21 ___loopGG ; compensation loop for G-plane
LSU0.LDIL i22 ___loopBB || LSU1.LDIH i22 ___loopBB ; compensation loop for B-plane
LSU0.LDIL i23 ___loopRRR || LSU1.LDIH i23 ___loopRRR ; compensation loop for R-plane
LSU0.LDIL i24 ___loopGGG || LSU1.LDIH i24 ___loopGGG ; compensation loop for G-plane
LSU0.LDIL i25 ___loopBBB || LSU1.LDIH i25 ___loopBBB ; compensation loop for B-plane
LSU0.LDIL i26 0x31 || LSU1.LDIH i26 0x0 ;49


CMU.CMII.i32 i15 i8
PEU.PC1C GT || BRU.JMP i13
nop 6

CMU.CMII.i32 i15 i6
PEU.PC1C LT || BRU.JMP i13
nop 6

CMU.CMII.i32 i7 i15
PEU.PC1C EQ || BRU.JMP i2
nop 6

CMU.CMII.i32 i8 i15
PEU.PC1C EQ || BRU.JMP i3
nop 6

;code part for R-plane
___loopR:

CMU.CMII.i32 i16 i26
PEU.PC1C LT || BRU.JMP i23
nop 6

___loopRR:
LSU0.LD.128.U8.U16  v1  i18 || IAU.ADD i18 i18 8   
LSU0.LD.128.U8.U16  v2  i18 || IAU.ADD i18 i18 8
LSU0.LD.128.U8.U16  v3  i18 || IAU.ADD i18 i18 8
LSU0.LD.128.U8.U16  v4  i18 || IAU.ADD i18 i18 8   
LSU0.LD.128.U8.U16  v5  i18 || IAU.ADD i18 i18 8   
LSU0.LD.128.U8.U16  v6  i18 || IAU.ADD i18 i18 8   
nop


VAU.ADD.i16 v7   v1 v20 || LSU1.SWZV8  [03623630] || IAU.ADD i10 i10 i9
                    
VAU.ADD.i16 v8   v2 v20 || LSU1.SWZV8  [03623741] || CMU.CMII.i32 i10 i16
                    
VAU.ADD.i16 v9   v3 v20 || LSU1.SWZV8  [03623752]
                    
VAU.ADD.i16 v10  v4 v20 || LSU1.SWZV8  [03623630]
                    
VAU.ADD.i16 v11  v5 v20 || LSU1.SWZV8  [03623741]
                    
VAU.ADD.i16 v12  v6 v20 || LSU1.SWZV8  [03623752]


LSU0.ST.128.U16.U8  v7  i17 || IAU.ADD i17 i17 3
PEU.PC1C LT || BRU.JMP i20
LSU0.ST.128.U16.U8  v8  i17 || IAU.ADD i17 i17 3
LSU0.ST.128.U16.U8  v9  i17 || IAU.ADD i17 i17 2
LSU0.ST.128.U16.U8  v10 i17 || IAU.ADD i17 i17 3
LSU0.ST.128.U16.U8  v11 i17 || IAU.ADD i17 i17 3
LSU0.ST.128.U16.U8  v12 i17 || IAU.ADD i17 i17 2
nop


___loopRRR:
LSU0.LD.128.U8.U16  v1  i18 || IAU.ADD i18 i18 8   
LSU0.LD.128.U8.U16  v2  i18 || IAU.ADD i18 i18 8
LSU0.LD.128.U8.U16  v3  i18 || IAU.ADD i18 i18 8
LSU0.LD.128.U8.U16  v4  i18 || IAU.ADD i18 i18 8   
LSU0.LD.128.U8.U16  v5  i18 || IAU.ADD i18 i18 8   
LSU0.LD.128.U8.U16  v6  i18 || IAU.ADD i18 i18 8   
nop


VAU.ADD.i16 v7   v1 v20 || LSU1.SWZV8  [03623630] 
                    
VAU.ADD.i16 v8   v2 v20 || LSU1.SWZV8  [03623741] 
                    
VAU.ADD.i16 v9   v3 v20 || LSU1.SWZV8  [03623752]
                    
VAU.ADD.i16 v10  v4 v20 || LSU1.SWZV8  [03623630]
                    
VAU.ADD.i16 v11  v5 v20 || LSU1.SWZV8  [03623741]
                    
VAU.ADD.i16 v12  v6 v20 || LSU1.SWZV8  [03623752]


LSU0.ST.128.U16.U8  v7  i11 || IAU.ADD i11 i11 3
LSU0.ST.128.U16.U8  v8  i11 || IAU.ADD i11 i11 3
LSU0.ST.128.U16.U8  v9  i11 || IAU.ADD i11 i11 2
LSU0.ST.128.U16.U8  v10 i11 || IAU.ADD i11 i11 3
LSU0.ST.128.U16.U8  v11 i11 || IAU.ADD i11 i11 3
LSU0.ST.128.U16.U8  v12 i11 || IAU.ADD i11 i11 2


IAU.SUB i14 i10 i9
IAU.SUB i5 i16 i14
CMU.CMII.i32 i16 i26 ;see if the width is smaller then 49
PEU.PC1C LT || IAU.SUB i5 i16 0

IAU.SUB i11 i19 0
LSU0.LDIL i1 ___compensation || LSU1.LDIH i1 ___compensation ;loop for R-plane
IAU.SUB i10 i10 i10


___compensation:
LSU0.LD.8    i12  i11  || IAU.ADD i11 i11 1
nop 6
LSU0.ST.8    i12  i17  || IAU.ADD i17 i17 1
IAU.ADD i10 i10 3
CMU.CMII.i32 i10 i5
PEU.PC1C LT || BRU.JMP i1
nop 6
IAU.ADD i19 i19 i9
LSU0.LD.32  i26  i19  || IAU.ADD i19 i19 4
LSU0.LD.32  i25  i19  || IAU.ADD i19 i19 4
LSU0.LD.32  i24  i19  || IAU.ADD i19 i19 4
LSU0.LD.32  i23  i19  || IAU.ADD i19 i19 4
LSU0.LD.32  i22  i19  || IAU.ADD i19 i19 4
LSU0.LD.32  i21  i19  || IAU.ADD i19 i19 4
LSU0.LD.32  i20  i19  || IAU.ADD i19 i19 4


BRU.jmp i30
nop 6


;code part for G-plane
___loopG:

CMU.CMII.i32 i16 i26
PEU.PC1C LT || BRU.JMP i24
nop 6

___loopGG:
LSU0.LD.128.U8.U16  v1  i18 || IAU.ADD i18 i18 8   
LSU0.LD.128.U8.U16  v2  i18 || IAU.ADD i18 i18 8
LSU0.LD.128.U8.U16  v3  i18 || IAU.ADD i18 i18 8
LSU0.LD.128.U8.U16  v4  i18 || IAU.ADD i18 i18 8   
LSU0.LD.128.U8.U16  v5  i18 || IAU.ADD i18 i18 8   
LSU0.LD.128.U8.U16  v6  i18 || IAU.ADD i18 i18 8   
nop


VAU.ADD.i16 v7   v1 v20 || LSU1.SWZV8  [03623741] || IAU.ADD i10 i10 i9
                    
VAU.ADD.i16 v8   v2 v20 || LSU1.SWZV8  [03623752] || CMU.CMII.i32 i10 i16
                    
VAU.ADD.i16 v9   v3 v20 || LSU1.SWZV8  [03623630]
                    
VAU.ADD.i16 v10  v4 v20 || LSU1.SWZV8  [03623741]
                    
VAU.ADD.i16 v11  v5 v20 || LSU1.SWZV8  [03623752]
                    
VAU.ADD.i16 v12  v6 v20 || LSU1.SWZV8  [03623630]


LSU0.ST.128.U16.U8  v7  i17 || IAU.ADD i17 i17 3
PEU.PC1C LT || BRU.JMP i21
LSU0.ST.128.U16.U8  v8  i17 || IAU.ADD i17 i17 2
LSU0.ST.128.U16.U8  v9  i17 || IAU.ADD i17 i17 3
LSU0.ST.128.U16.U8  v10 i17 || IAU.ADD i17 i17 3
LSU0.ST.128.U16.U8  v11 i17 || IAU.ADD i17 i17 2
LSU0.ST.128.U16.U8  v12 i17 || IAU.ADD i17 i17 3
nop


___loopGGG:
LSU0.LD.128.U8.U16  v1  i18 || IAU.ADD i18 i18 8   
LSU0.LD.128.U8.U16  v2  i18 || IAU.ADD i18 i18 8
LSU0.LD.128.U8.U16  v3  i18 || IAU.ADD i18 i18 8
LSU0.LD.128.U8.U16  v4  i18 || IAU.ADD i18 i18 8   
LSU0.LD.128.U8.U16  v5  i18 || IAU.ADD i18 i18 8   
LSU0.LD.128.U8.U16  v6  i18 || IAU.ADD i18 i18 8   
nop


VAU.ADD.i16 v7   v1 v20 || LSU1.SWZV8  [03623741] 
                    
VAU.ADD.i16 v8   v2 v20 || LSU1.SWZV8  [03623752] 
                    
VAU.ADD.i16 v9   v3 v20 || LSU1.SWZV8  [03623630]
                    
VAU.ADD.i16 v10  v4 v20 || LSU1.SWZV8  [03623741]
                    
VAU.ADD.i16 v11  v5 v20 || LSU1.SWZV8  [03623752]
                    
VAU.ADD.i16 v12  v6 v20 || LSU1.SWZV8  [03623630]


LSU0.ST.128.U16.U8  v7  i11 || IAU.ADD i11 i11 3
LSU0.ST.128.U16.U8  v8  i11 || IAU.ADD i11 i11 2
LSU0.ST.128.U16.U8  v9  i11 || IAU.ADD i11 i11 3
LSU0.ST.128.U16.U8  v10 i11 || IAU.ADD i11 i11 3
LSU0.ST.128.U16.U8  v11 i11 || IAU.ADD i11 i11 2
LSU0.ST.128.U16.U8  v12 i11 || IAU.ADD i11 i11 3

IAU.SUB i14 i10 i9
IAU.SUB i5 i16 i14
CMU.CMII.i32 i16 i26 ;see if the width is smaller then 49
PEU.PC1C LT || IAU.SUB i5 i16 0

IAU.SUB i11 i19 0
LSU0.LDIL i1 ___compensationG || LSU1.LDIH i1 ___compensationG ;loop for R-plane
IAU.SUB i10 i10 i10



___compensationG:
LSU0.LD.8    i12  i11  || IAU.ADD i11 i11 1
nop 6
LSU0.ST.8    i12  i17  || IAU.ADD i17 i17 1
IAU.ADD i10 i10 3
CMU.CMII.i32 i10 i5
PEU.PC1C LT || BRU.JMP i1
nop 6

IAU.ADD i19 i19 i9
LSU0.LD.32  i26  i19  || IAU.ADD i19 i19 4
LSU0.LD.32  i25  i19  || IAU.ADD i19 i19 4
LSU0.LD.32  i24  i19  || IAU.ADD i19 i19 4
LSU0.LD.32  i23  i19  || IAU.ADD i19 i19 4
LSU0.LD.32  i22  i19  || IAU.ADD i19 i19 4
LSU0.LD.32  i21  i19  || IAU.ADD i19 i19 4
LSU0.LD.32  i20  i19  || IAU.ADD i19 i19 4

BRU.jmp i30
nop 6

;code part for B-plane
___loopB:

CMU.CMII.i32 i16 i26
PEU.PC1C LT || BRU.JMP i25
nop 6

___loopBB:

LSU0.LD.128.U8.U16  v1  i18 || IAU.ADD i18 i18 8   
LSU0.LD.128.U8.U16  v2  i18 || IAU.ADD i18 i18 8
LSU0.LD.128.U8.U16  v3  i18 || IAU.ADD i18 i18 8
LSU0.LD.128.U8.U16  v4  i18 || IAU.ADD i18 i18 8   
LSU0.LD.128.U8.U16  v5  i18 || IAU.ADD i18 i18 8   
LSU0.LD.128.U8.U16  v6  i18 || IAU.ADD i18 i18 8   
nop


VAU.ADD.i16 v7   v1 v20 || LSU1.SWZV8  [03623652] || IAU.ADD i10 i10 i9
                    
VAU.ADD.i16 v8   v2 v20 || LSU1.SWZV8  [03623630] || CMU.CMII.i32 i10 i16
                    
VAU.ADD.i16 v9   v3 v20 || LSU1.SWZV8  [03623741]
                    
VAU.ADD.i16 v10  v4 v20 || LSU1.SWZV8  [03623652]
                    
VAU.ADD.i16 v11  v5 v20 || LSU1.SWZV8  [03623630]
                    
VAU.ADD.i16 v12  v6 v20 || LSU1.SWZV8  [03623741]


LSU0.ST.128.U16.U8  v7  i17 || IAU.ADD i17 i17 2
PEU.PC1C LT || BRU.JMP i22
LSU0.ST.128.U16.U8  v8  i17 || IAU.ADD i17 i17 3
LSU0.ST.128.U16.U8  v9  i17 || IAU.ADD i17 i17 3
LSU0.ST.128.U16.U8  v10 i17 || IAU.ADD i17 i17 2
LSU0.ST.128.U16.U8  v11 i17 || IAU.ADD i17 i17 3
LSU0.ST.128.U16.U8  v12 i17 || IAU.ADD i17 i17 3
nop


___loopBBB:
LSU0.LD.128.U8.U16  v1  i18 || IAU.ADD i18 i18 8   
LSU0.LD.128.U8.U16  v2  i18 || IAU.ADD i18 i18 8
LSU0.LD.128.U8.U16  v3  i18 || IAU.ADD i18 i18 8
LSU0.LD.128.U8.U16  v4  i18 || IAU.ADD i18 i18 8   
LSU0.LD.128.U8.U16  v5  i18 || IAU.ADD i18 i18 8   
LSU0.LD.128.U8.U16  v6  i18 || IAU.ADD i18 i18 8   
nop


VAU.ADD.i16 v7   v1 v20 || LSU1.SWZV8  [03623652] 
                    
VAU.ADD.i16 v8   v2 v20 || LSU1.SWZV8  [03623630] 
                    
VAU.ADD.i16 v9   v3 v20 || LSU1.SWZV8  [03623741]
                    
VAU.ADD.i16 v10  v4 v20 || LSU1.SWZV8  [03623652]
                    
VAU.ADD.i16 v11  v5 v20 || LSU1.SWZV8  [03623630]
                    
VAU.ADD.i16 v12  v6 v20 || LSU1.SWZV8  [03623741]


LSU0.ST.128.U16.U8  v7  i11 || IAU.ADD i11 i11 2
LSU0.ST.128.U16.U8  v8  i11 || IAU.ADD i11 i11 3
LSU0.ST.128.U16.U8  v9  i11 || IAU.ADD i11 i11 3
LSU0.ST.128.U16.U8  v10 i11 || IAU.ADD i11 i11 2
LSU0.ST.128.U16.U8  v11 i11 || IAU.ADD i11 i11 3
LSU0.ST.128.U16.U8  v12 i11 || IAU.ADD i11 i11 3

IAU.SUB i14 i10 i9
IAU.SUB i5 i16 i14
CMU.CMII.i32 i16 i26 ;see if the width is smaller then 49
PEU.PC1C LT || IAU.SUB i5 i16 0


IAU.SUB i11 i19 0
LSU0.LDIL i1 ___compensationB || LSU1.LDIH i1 ___compensationB ;loop for R-plane
IAU.SUB i10 i10 i10



___compensationB:
LSU0.LD.8    i12  i11  || IAU.ADD i11 i11 1
nop 6
LSU0.ST.8    i12  i17  || IAU.ADD i17 i17 1
IAU.ADD i10 i10 3
CMU.CMII.i32 i10 i5
PEU.PC1C LT || BRU.JMP i1
nop 6

___exit:
IAU.ADD i19 i19 i9
LSU0.LD.32  i26  i19  || IAU.ADD i19 i19 4
LSU0.LD.32  i25  i19  || IAU.ADD i19 i19 4
LSU0.LD.32  i24  i19  || IAU.ADD i19 i19 4
LSU0.LD.32  i23  i19  || IAU.ADD i19 i19 4
LSU0.LD.32  i22  i19  || IAU.ADD i19 i19 4
LSU0.LD.32  i21  i19  || IAU.ADD i19 i19 4
LSU0.LD.32  i20  i19  || IAU.ADD i19 i19 4

BRU.jmp i30
nop 6




.end
