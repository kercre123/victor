;-------------------------------------------------------------------------------
.version 00.51.05
;-------------------------------------------------------------------------------
; note: this is compiled code, just briefly tweaked
;       around branches

.include "myriad2SippDefs.inc"

.code svuCopy
.salign 16

 .lalign
 svuCopy:
        IAU.ADD    i10, i18, F_O_SLICE_W
        LSU0.LD.32 i10, i10
        NOP 5
        LSU1.LDIL i15 0x0001
        CMU.CMZ.i32 i10            ||  IAU.AND i10 i16 i15
        IAU.SHL i10 i10 2
        IAU.ADD i9 i18 i10
        IAU.ADD i17 i9 F_O_LINES_I
        LSU0.LD.32 i9 i17
        PEU.PC1C EQ                ||  BRU.BRA ___svuCopy_3
        IAU.ADD i10 i9 F_O_LINES_O   ;WARNING: uses old i9
        NOP 6

  .lalign
  ___svuCopy_1:
        LSU1.LD.32 i9 i9           ||  LSU0.LD.32 i10 i10
        NOP 4
        LSU0.LDIL i8 0
        IAU.ADD i7 i18 F_O_SLICE_W

  .lalign
  ___svuCopy_2:
        IAU.ADD i5 i9 i8
        LSU0.LD.8 i5 i5
        NOP 5
        IAU.ADD i6 i10 i8
        LSU0.ST.8 i5 i6
        LSU0.LD.32 i6 i7
        NOP 5
        IAU.ADD i8 i8 1
        CMU.CMII.u32 i8 i6
        PEU.PC1C LT            ||  BRU.BRA ___svuCopy_2
        NOP 6

  .lalign
  ___svuCopy_3:
        BRU.JMP i30
        NOP 6
.end
