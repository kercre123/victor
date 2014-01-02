#include "math_consts.h"

                .section ".text", "ax", @progbits

!=[ exp2f ]====================================================================

                .global exp2f
! float exp2f( unsigned x_ ) {
                #define x_ %o0
exp2f:
                .type   exp2f, #function
                dec	72, %sp
!   return expf( *(float*)&x_ * LN2 )
		#define	fp32c %o1
                set     fp32consts, fp32c
                st      x_, [ %sp + 64 ]
                ld      [ fp32c + 4*FP32IDX_LN_2 ], %f1
                ld      [ %sp + 64 ], %f0
                fmuls   %f0, %f1, %f0
                st      %f0, [ %sp + 64 ]
                b       .L.expf_entry
                  ld      [ %sp + 64 ], %o0
! }
                .size   exp2f, . - exp2f

!=[ exp10f ]===================================================================

                .global exp10f
! float exp10f( unsigned x_ ) {
exp10f:
                .type   exp10f, #function
                dec	72, %sp
!   return expf( *(float*)&x_ * LN10 )
                set     fp32consts, fp32c
                st      x_, [ %sp + 64 ]
                ld      [ fp32c + 4*FP32IDX_LN_10 ], %f1
                ld      [ %sp + 64 ], %f0
                fmuls   %f0, %f1, %f0
                st      %f0, [ %sp + 64 ]
                b       .L.expf_entry
                  ld      [ %sp + 64 ], %o0
! }
                .size   exp10f, . - exp10f

!=[ expf ]=====================================================================

                .global expf
! float expf( unsigned x_ ) {
expf:
                .type   expf, #function
                dec	72, %sp
! {
!   const float *fp32c = fp32consts;
                set     fp32consts, fp32c
.L.expf_entry:
!   float half;
!   float x, z, P, Q, t, tt;
!   int N;
		#define x %f0
		#define z %f1
		#define P %f2
		#define Q %f3
		#define half %f4
		#define t %f5
		#define tt %f6
		#define N %o2
!   unsigned sgn = x_ & 0x80000000;
!   // make x_ positive
!   x_ ^= sgn;
		#define sgn %o3
		set	0x80000000, %g1
		and	x_, %g1, sgn
		xor	x_, sgn, x_
!   // really small input value - |x| < 0.000001 - return 1.0
!   if( x_ <= 0x35863400  ) {
!     x_ = 0x3f800000;
!     return *(float*)&x_;
!   }
		set	0x35863400, %g1
		cmp	x_, %g1
		bleu,a	.L.done1
		  set	0x3f800000, x_
!   // NaNs - return the argument
!   if( x_ > 0x7f800000 )
!     return *(float*)&x_;
		set	0x7f800000, %g1
		cmp	x_, %g1
		bgu	.L.done1
!   // really large positive input value - x >= 72.609375: return +Inf
!   // note, this also covers the case when input value is +Inf
!   if( !sgn ) {
!     if( x_ >= 0x42913800 ) {
!       x_ = 0x7f800000;
!       return *(float*)&x_;
!     }
!   }
		tst	sgn
		bnz,a	.L.is_neg
		  set	0x42aeb000, %g1
		set	0x42913800, %g1
		cmp	x_, %g1
		bgeu,a	.L.done1
		  set	0x7f800000, x_
		b,a	.L.proceed
!   // really large negative input value - |x| >= 87.343750: return 0.0
!   // note, this also covers the case when input value is -Inf
!   else { // if( sgn )
!     if( x_ >= 0x42aeb000 ) {
!       x_ = 0;
!       return *(float*)&x_;
!     }
!   }
.L.is_neg:
		cmp	x_, %g1
		bgeu,a	.L.done1
	          clr	x_
.L.proceed:
!   half = fp32c[FP32IDX_0_5];
!   P = fp32c[FP32IDX_INV_LN_2];
!   Q = fp32c[FP32IDX_LN_2];
		tst	sgn
		ld	[fp32c+4*FP32IDX_0_5], half
		ld	[fp32c+4*FP32IDX_INV_LN_2], P
		ld	[fp32c+4*FP32IDX_LN_2], Q
!   x = *(float*)&x_;
		st	x_, [ %sp + 64 ]
		ld	[ %sp + 64 ], x
!   z = x * P;
!   z += half;
!   // do the sign correction (x_ contains the absolute value of the argument)
!   if( sgn ) {
!     x = -x;
!     z = -z;
!   }
		fmuls	x, P, z
		bz	.L.proceed2
		  fadds	z, half, z
		fnegs	x, x
		fnegs	z, z
.L.proceed2:
!   N = (int)z;
!   z = (float)N;
		fstoi	z, t
		st	t, [ %sp + 64 ]
		fitos	t, z
		ld	[ %sp + 64 ], N
!   z *= Q;
!   x -= z;
!   z = x * x;
		fmuls	z, Q, z
		fsubs	x, z, x
		fmuls	x, x, z

!   fp32c += FP32IDX_TABLE_EXP + 1;
!   P = half - half; Q = fp32c[-1];
		fsubs	half, half, P
		inc     4*(1+FP32IDX_TABLE_EXP), fp32c
		ld	[ fp32c - 4 ], Q
!   for( sgn = 3; sgn > 0; --sgn ) {
		mov	3, sgn
.L.iter:
!     t = *fp32c; tt = fp32c[1];
		ld	[ fp32c ], t
		ld	[ fp32c + 4 ], tt
!     P *= z; Q *= z;
		fmuls	P, z, P
		fmuls	Q, z, Q
!     P += t; Q += tt;
		fadds	P, t, P
		fadds	Q, tt, Q
!     fp32c += 2;
!   }
		deccc	sgn
		bnz	.L.iter
		  inc	8, fp32c
!   P *= x;
!   Q -= P;
!   P /= Q;
!   P += half;
		fmuls	P, x, P
                fsubs   Q, P, Q
                fdivs   P, Q, P
                fadds   P, half, P
!   ++N;
!   return ldexpf( P, N );
                st      P, [ %sp + 64 ]
                add     N, 1, %o1
                sethi   %hi( ldexpf ), %g1
                ld      [ %sp + 64 ], %o0
                jmp     %g1 + %lo( ldexpf )
                  inc	72, %sp
! -> return *(float*)&x_;
.L.done1:
		st	x_, [ %sp + 64 ]
		ld	[ %sp + 64 ], %f0
		retl
		  inc	72, %sp
! }
                .size   expf, . - expf
