                #include "math_consts.h"

                .section ".text", "ax", @progbits

!-[ acosf ]--------------------------------------------------------------------

                .global acosf
! float acosf( unsigned x_ ) {
acosf:
                .type   acosf, #function
!   return asinef( x_, 1 );
                b       asinef
                  mov   1, %o1
                .size   acosf, . - acosf
! }

!-[ asinf ]--------------------------------------------------------------------

                .global asinf
! float asinf( unsigned x_ ) {
asinf:
                .type   asinf, #function
!   return asinef( x_, 0 );
                clr     %o1
                .size   asinf, . - asinf
! }

!=[ asinef ]===================================================================

! static float asinef( unsigned x_, int acosine ) {
		#define	x_ %o0
		#define acosine %o1
asinef:
                .type   asinef, #function
                dec	72, %sp

!   const float *fp32c = fp32consts;
                #define fp32c %o2
                set     fp32consts, fp32c
!   int i;
!   float P, Q, x, g, t, tt;
		#define	i %o3
		#define P %f0
		#define Q %f1
		#define	x %f2
		#define g %f3
		#define t %f4
		#define tt %f5
!   unsigned nr = x_ << 1;
!   unsigned sgn = x_ >> 31;
		#define nr %o4
		#define	sgn %o5
		sll	x_, 1, nr
		srl	x_, 31, sgn
!   // if argument isNaN(x), isInf(x), |x|>1.0: return NaN
!   if( nr > (0x3f800000<<1) )
!     return fp32consts[FP32IDX_QNAN];
		set	0x3f800000<<1, %g1
		cmp	nr, %g1
		bgu,a	.L.done
		  ld	[ fp32c + 4*FP32IDX_QNAN ], P
!   x = *(float*)&x_;
!   x = fabsf( x );
!   i = acosine << 3; 
		st	x_, [ %sp + 64 ]
		set	0x32000000<<1, %g1
		ld	[ %sp + 64 ], x
		sll	acosine, 3, i
		fabss	x, x
!   if( nr <= (0x32000000<<1) ) { // if( x < z_rooteps )
!     P = x;
!     goto done_poli;
!   }
		cmp	nr, %g1
		bleu,a	.L.done_poli
		  fmovs	x, P
!   if( nr > (0x3f000000<<1)) { // if( x > 0.5 );
!     g = fp32consts[FP32IDX_1];
!     t = fp32consts[FP32IDX_2];
!     g -= x;
!     g /= t;
!     i ^= 8;
!     x = sqrtf( g );
!     x *= t;
!     x = -x;
!   }
!   else
!     g = x * x;
		set	0x3f000000<<1, %g1
		cmp	nr, %g1
		bleu,a	.L.no_adj
		  fmuls	x, x, g
		ld	[ fp32c + 4*FP32IDX_1], g
		ld	[ fp32c + 4*FP32IDX_2], t
		fsubs	g, x, g
		fdivs	g, t, g
		xor	i, 8, i
		fsqrts	g, x
		fmuls	x, t, x
		fnegs	x, x
.L.no_adj:
!   t  = fp32c[FP32IDX_TABLE_ASINE+1];
!   P  = fp32c[FP32IDX_TABLE_ASINE];   
!   Q  = g + t;
		ld	[ fp32c + 4*(FP32IDX_TABLE_ASINE+1) ], t
		ld	[ fp32c + 4*FP32IDX_TABLE_ASINE ], P
		fadds	g, t, Q
!   P *= g;
!   Q *= g;
		fmuls	P, g, P
		fmuls	Q, g, Q
!   P += fp32consts[FP32IDX_TABLE_ASINE+2];
!   Q += fp32consts[FP32IDX_TABLE_ASINE+3]; 
!   P *= g;
		ld	[ fp32c + 4*(FP32IDX_TABLE_ASINE+2) ], t
		ld	[ fp32c + 4*(FP32IDX_TABLE_ASINE+3) ], tt
		fadds	P, t, P
		fadds	Q, tt, Q
		fmuls	P, g, P
!   P /= Q;
!   P *= x;
!   P += x;
		fdivs	P, Q, P
		fmuls	P, x, P
		fadds	P, x, P
.L.done_poli:
! done_poli:
!   i +=( 4 + FP32IDX_TABLE_ASINE )<<2;
!   t = fp32consts[i/4]; tt = fp32consts[i/4+1];
		inc	(4+FP32IDX_TABLE_ASINE)<<2, i
		ld	[ fp32c + i ], t
		inc	4, i
		ld	[ fp32c + i ], tt
!   if( !acosine ) {
!     P += t;
!     if( sgn )
!       P = -P;
!   }
		tst	acosine
		bnz	.L.is_asine
		  tst	sgn
		fadds	P, t, P
		bnz,a	.L.done
		  fnegs	P, P
		b,a	.L.done
.L.is_asine:
!   else if( sgn )
!     P += tt;
		bnz,a	.L.done
		  fadds	P, tt, P
!   else
!     P = t - P;
		fsubs	t, P, P
.L.done:
!   return P;
		retl
		  inc	72, %sp
! }
		.size	asinef, . - asinef
