		#include "math_consts.h"

!=[ tanf ]=====================================================================

		.global	tanf
! float ref_tanf( unsigned x_ ) {
		#define	x_ %o0
tanf:
                .type   tanf, #function
                dec	72, %sp

!   const float *fp32c = fp32consts;
                #define fp32c %o1
                set     fp32consts, fp32c
!   float x, y, g, xnum, t, tt;
!   int N;
		#define xnum %f0
		#define x %f1
		#define y %f2
		#define g %f3
		#define t %f4
		#define tt %f5
		#define N %o2
!   N = x_ << 1;
!   if( (unsigned)N >= (0x4cc00e00<<1) )
!     return fp32c[FP32IDX_QNAN];
		sll	x_, 1, N
		set	(0x4cc00e00<<1), %g1
		cmp	N, %g1
		bgeu,a	.L.done
		  ld	[ fp32c + 4*FP32IDX_QNAN ], xnum
!   N = x_ >> 31;
!   x = *(float*)&x_;
!   y = fabsf( x );
		st	x_, [ %sp + 64 ]
		srl	x_, 31, N
		ld	[ %sp + 64 ], x
		fabss	x, y
!   y *= fp32c[FP32IDX_2_INV_PI];
!   y += fp32c[FP32IDX_0_5];
		ld	[ fp32c + 4*FP32IDX_2_INV_PI ], t
		fmuls	y, t, y
		ld	[ fp32c + 4*FP32IDX_0_5 ], tt
		fadds	y, tt, y
!   if( N )
!     y = -y;
		tst	N
		bnz,a	. + 8 ! skipz
		  fnegs	y, y
!   y = (float)((int)y);
		fstoi	y, t
		fitos	t, y
		st	t, [ %sp + 64 ]
!   y *= fp32c[FP32IDX_HALF_PI];
		ld	[ fp32c + 4*FP32IDX_HALF_PI ], t
		fmuls	y, t, y
!   x -= y;
!   y = fabsf( x );
		fsubs	x, y, x
		fabss	x, y
!   xnum = fp32c[FP32IDX_ROOTEPS];
!   t = fp32c[FP32IDX_1];
!   N = (int)y;
!   if( y < xnum ) {
!     xnum = x;
!     y = t;
!   }
		ld	[ fp32c + 4*FP32IDX_ROOTEPS ], xnum
		ld	[ fp32c + 4*FP32IDX_1 ], t
		fcmps	y, xnum
		fmovs	x, xnum
		ld	[ %sp + 64 ], N
		fbl,a	.L.done_poli
		  fmovs	t, y
!   else {
!     g = x * x;
!     y = fp32c[FP32IDX_TABLE_TAN+1];
!     xnum = fp32c[FP32IDX_TABLE_TAN];
		fmuls	x, x, g
		ld	[ fp32c + 4*(FP32IDX_TABLE_TAN+1)], y
		ld	[ fp32c + 4*FP32IDX_TABLE_TAN], xnum
!     y *= g;
!     xnum *= g;
		fmuls	y, g, y
		fmuls	xnum, g, xnum
!     y += fp32c[FP32IDX_TABLE_TAN+2];
		ld	[ fp32c + 4*(FP32IDX_TABLE_TAN+2)], tt
		fadds	y, tt, y
!     xnum *= x;
!     y *= g;
		fmuls	xnum, x, xnum
		fmuls	y, g, y
!     xnum += x;
!     y += t;
		fadds	xnum, x, xnum
		fadds	y, t, y
!   }
.L.done_poli:
!   if( N & 1 ) {
!     x = xnum;
!     xnum = y;
!     y = -x;
!   }
		andcc	N, 1, %g0
		bz	.L.final_div
		  fmovs	xnum, x
		fmovs	y, xnum
		fnegs	x, y
!   xnum /= y;
.L.final_div:
		fdivs	xnum, y, xnum
!   return xnum;
.L.done:
		retl
		  inc	72, %sp
! }
		.size	tanf, . - tanf
