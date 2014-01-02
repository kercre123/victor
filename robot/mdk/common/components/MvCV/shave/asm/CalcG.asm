; ///
; /// @file
; /// @copyright All code copyright Movidius Ltd 2013, all rights reserved.
; ///            For License Warranty see: common/license.txt
; ///
; /// @brief
; ///

.version 00.50.00

;#########################################################################
;;;;
;; ;;                                                                   CALC G
;;;;
;#########################################################################
;;                                                      i18            v23              v22                   i17           v21                v20            i16

;                       void CalcG(const float* Ix, ClSize isz, CvPoint minI, const float* Iy, ClSize jsz, CvPoint minJ, Scalar& G)
; {
	; float Gxx = 0;
	; float Gyy = 0;
	; float Gxy = 0;
	; float D = 0;

	; for(int y = 0; y < jsz.height; y++)
	; {
		; const float* ix = Ix +
			; (y + minJ.y - minI.y) * (isz.width - 2) + minJ.x - minI.x;
		; const float* iy = Iy + (ix - Ix);

		; // Vectorize
		; for(int x = 0; x < jsz.width; x++)
		; {
			; // Compute the gradients matrix (eq. 24)
			; Gxx += ix[x] * ix[x];
			; Gxy += ix[x] * iy[x];
			; Gyy += iy[x] * iy[x];
		; }
	; }

	; // Determinant of gradients matrix must not be 0 so the
	; // matrix in (eq. 28) is invertible
	; D = Gxx * Gyy - Gxy * Gxy;

	; return Scalar(Gxx, Gyy, Gxy, D);
; }


.set i_Ix     i18
.set i_Iy     i17
.set v_isz_0  v23.0
.set v_minI   v22
.set v_jsz_0  v21.0
.set v_jsz_1  v21.1
.set v_minJ   v20


.code .text.MvCV_CalcG
.salign 16

.lalign
CalcG_asm:


    ;; computing  ix and  iy addresses
cmu.cpvi i3, v_isz_0     || vau.sub.i32 v0,v_minJ,v_minI  ;; minJ.y - minI.y & minJ.x - minI.x
iau.incs i3,-2           || sau.xor i8,i8,i8 
cmu.cpvi i4,v0.1         || vau.xor v10,v10,v10    || sau.shl.u32 i0,i3,2 
iau.mul i4,i4,i3         || cmu.cpvi i5,v0.0       || lsu1.ldil i6,0x10
nop  
iau.add i4,i4,i5         || vau.xor v5,v5,v5       || lsu1.ldih i6,0x0    || cmu.cpvi i5,v_jsz_0
iau.shl i4,i4,2 
iau.add i_Ix,i_Ix,i4     || vau.xor v6,v6,v6       || sau.shl.u32 i9,i5,2 
iau.add i_Iy,i_Iy,i4     || vau.xor v7,v7,v7       || cmu.cpvi i4,v_jsz_1  || sau.add.i32 i10,i_Ix,0   
sau.add.i32 i7,i_Iy,0 


  ;; repeat next loop untill you reach end of data block
____CalcGVerticalLoop:
      ;; read input data ; v0- ix  ; v1- iy
lsu0.ldxvi v0,i10,i6 || sau.add.i32 i2,i_Ix,i9 || cmu.cpvi i5,v_jsz_0
lsu1.ldxvi v1,i7,i6  || sau.add.i32 i3,i_Iy,i9   
      ;; read the end of each line, store zeros from width of the line untill multiple of 4. 
lsu0.ldxv v8,i2      
lsu1.ldxv v9,i3      
lsu0.stxv v10,i2     || iau.incs i5,-4 
lsu1.stxv v10,i3 
cmu.cpis s5,i2  
cmu.cpis s6,i3
nop 

      ;; repeat next loop untill you reach end of line
____CalcGHorizontalLoop:
      ;; compute v_Gxx in v5, v_Gyy in v7, v_Gxy in v6
vau.mul.f32 v2,v0,v0             || lsu0.ldxvi v0,i10,i6   
bru.bra ____CalcGHorizontalLoop || peu.pc1i GT
vau.mul.f32 v3,v0,v1             || lsu1.ldxvi v1,i7,i6      
vau.mul.f32 v4,v1,v1 
vau.add.f32 v5,v5,v2  
vau.add.f32 v6,v6,v3             || iau.incs i5,-4
vau.add.f32 v7,v7,v4

;; restore data from end of line, which you previously read (v8 and v9)
;; recalc addresses for next lines
iau.incs i4,-1
peu.pc1i GT || bru.bra ____CalcGVerticalLoop || cmu.cpsi i2,s5  
peu.pc1i GT || iau.add i_Ix,i_Ix,i0           || cmu.cpsi i3,s6  || lsu0.stxv v8,i2
peu.pc1i GT || iau.add i_Iy,i_Iy,i0           || lsu1.stxv v9,i3
peu.pc1i GT || iau.add i10,i_Ix,0         
peu.pc1i GT || iau.add i7,i_Iy,0          
nop 
;;end_CalcGVerticalLoop

.unset v_isz_0  v23.0
.set v_G        v23
.set v_Gxx_0    v23.0
.set v_Gyy_1    v23.1
.set v_Gxy_2    v23.2
.set v_D_3      v23.3
.set s_Gxx      s0
.set s_Gyy      s1
.set s_Gxy      s2
.set s_D        s4
sau.sum.f32 s_Gxx v5,0xf ;; Gxx
sau.sum.f32 s_Gyy v7,0xf ;; Gyy
sau.sum.f32 s_Gxy v6,0xf ;; Gxy
nop 2
cmu.cpsv.x32 v_Gxx_0, s_Gxx
sau.mul.f32 s3,s_Gxx,s_Gyy     || cmu.cpsv.x32 v_Gyy_1, s_Gyy
sau.mul.f32 s_D,s_Gxy,s_Gxy    || cmu.cpsv.x32 v_Gxy_2, s_Gxy 
nop
nop
sau.sub.f32 s3,s3,s_D 
bru.swp i30
nop 
cmu.cpsv.x32 v_D_3, s3
lsu0.sto64.l v_G,i16,0x0 || lsu1.sto64.h v_G,i16,0x8
nop 2



.unset i_Ix       i18
.unset i_Iy       i17
.unset v_Gxx_0    v23.0
.unset v_Gyy_1    v23.1
.unset v_Gxy_2    v23.2
.unset v_D_3      v23.3
.unset s_Gxx      s0
.unset s_Gyy      s1
.unset s_Gxy      s2
.unset s_D        s4


.end
