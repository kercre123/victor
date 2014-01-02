.version 00.40.00

.code .text.RC_Frame
.lalign
RC_Frame:
SAU.SUB.f32 s_updateRC, s_updateRC, 1.0 || LSU0.LDO32 i5, enc, cmdCfg || LSU1.LDIL i0, RC_trail_buffer
                                           LSU0.LDO32 i3, enc, fps    || LSU1.LDIH i0, RC_trail_buffer
                                           LSU0.LD64.l  v18, i0       || LSU1.LDIL i1, RC_update_table
CMU.CMZ.f32 s_updateRC                  || LSU0.LDO64.h v18, i0, 0x08 || LSU1.LDIH i1, RC_update_table
PEU.PC1C 0x1                            || LSU0.LDI64.l v16, i1
PEU.PC1C 0x1                            || LSU0.LDI64.h v16, i1
; rate control update not yet required
PEU.PC1C 0x6 || BRU.BRA EC_NextRow
PEU.PC1C 0x1 || IAU.INCS i3, 500        || LSU1.LDIL i4, 1000
PEU.PC1C 0x1 || SAU.DIV.i32 i1, i3, i4  || LSU0.LDO32 i_qp, enc, frameQp
CMU.SHLIV.x32 v18, i18                  || LSU0.LDO32 i18,  enc, tgtRate
SAU.SUM.u32 i2, v18, 0xF                || LSU0.ST64.l  v18, i0
                                           LSU0.STO64.h v18, i0, 0x08
RC_Window:
; check if rate control enabled
IAU.FEXT.u32 i0, i5, cfg_RC_pos, cfg_RC_len || LSU1.LDIL i3, 0x06
PEU.PC1I 0x1 || BRU.BRA EC_NextRow
PEU.PC1I 0x6 || SAU.MOD.i32 i3, i_qp, i3
LSU1.LDIL i0, RC_WINDOW
CMU.CPIS.i32.f32 s_updateRC, i0
BRU.RPIM 0x05
IAU.SHR.u32 i2, i2, 0x02
IAU.MUL i2, i2, i1
LSU1.LDIL i1, 100
IAU.MUL i4, i2, i1
LSU0.STO32 i2, enc, avgRate
SAU.DIV.i32 i4, i4, i18
BRU.RPIM 0x10
CMU.CMII.i32 i2, i18
PEU.PC1C 0x2 || IAU.SUB i4, i4, i1 || SAU.ADD.i32 i3, i3, 0x02
PEU.PC1C 0x5 || IAU.SUB i4, i1, i4
CMU.LUT16.u32 i3, i3, 0x2
BRU.BRA EC_NextRow || LSU1.LDIL i1, 0x1F
IAU.SUB i0, i4, i3
PEU.PCC0I.AND 0x2, 0x2 || IAU.ADD i_qp, i_qp, 0x01
PEU.PCC0I.AND 0x5, 0x2 || IAU.SUB i_qp, i_qp, 0x01
IAU.CLAMP0.i32 i_qp, i_qp, i1
LSU0.STO32 i_qp, enc, frameQp
