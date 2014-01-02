; mechanism to protect against multiple inclusions
.ifndef ENC_H
.set ENC_H 1


.set s_width         s24
.set s_height        s25
.set s_QPy           s26
.set s_CoeffStart    s27
.set s_YQAddr        s28
.set s_UVQAddr       s29
.set i_rowStart      i16
.set i_rowEnd        i17
;// i18 reserved as function parameter
.set i_desc          i19
.set i_smb           i20    ;// 4x4 sub macroblock idx
.set i_flag          i21
.set i_best          i21
.set i_cbp           i22
.set i_adr           i22
.set i_mode          i23    ;// 4x4 pred mode

;// vectors for intra pred neighbours
.set v_Top           v16
.set v_Left          v17
.set v_Mean          v18
.set v_Temp          v19
;// vectors used to store coefficients
.set v_Coeff0        v16
.set v_Coeff1        v17
.set v_Coeff2        v18
.set v_Coeff3        v19
.set v_Coeff0.0      v16.0
.set v_Coeff0.1      v16.1
.set v_Coeff0.2      v16.2
.set v_Coeff0.3      v16.3
;// vectors used to store residual data
.set v_Residual0     v20
.set v_Residual1     v21
.set v_Residual2     v22
.set v_Residual3     v23
.set v_Residual0.0   v20.0
.set v_Residual0.1   v20.1
.set v_Residual0.2   v20.2
.set v_Residual0.3   v20.3
.set v_Residual0.4   v20.4
.set v_Residual0.5   v20.5
.set v_Residual0.6   v20.6
.set v_Residual0.7   v20.7
;// vectors used to store predicted values
.set v_Pred0         v24
.set v_Pred1         v25
.set v_Pred2         v26
.set v_Pred3         v27
;// DC coeffs storage
.set v_DC0           v28
.set v_DC1           v29

.set v_amv           v26
.set v_amv.0         v26.0
.set v_amv.1         v26.1
.set v_amv.2         v26.2
.set v_amv.3         v26.3
.set v_amv.4         v26.4
.set v_amv.5         v26.5
.set v_amv.6         v26.6
.set v_amv.7         v26.7
.set v_mv            v28
.set v_mv.0          v28.0
.set v_SAD8x8        v29
.set v_mvc           v30
.set v_QParam        v30

.endif