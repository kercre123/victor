                #include "math_consts.h"

                .section ".text", "ax", @progbits
                .global cosf
! float cosf( unsigned x_, int cosine ) {
cosf:
                .type   cosf, #function
!   return sinef( x_, 1 );
                b       sinef
                  mov   1, %o1
                .size   cosf, . - cosf
! }
                
                .global sinf
! float sinf( unsigned x_, int cosine ) {
sinf:
                .type   sinf, #function
!   return sinef( x_, 0 );
                clr     %o1
                .size   sinf, . - sinf
! }
                
! static float sinef( unsigned x_, int cosine ) {
sinef:
                .type   sinef, #function
                add     %sp, -72, %sp

!   const float *fp32c = fp32consts;
#define fp32c %o2
                set     fp32consts, fp32c

!   float y, y0, g;
                #define y %f0
                #define y0 %f1
                #define g %f2
!   int sgn, i;
                #define sgn %o3
                #define i %o4
                ! preload constants
                #define inv_pi %f3
                ld      [fp32c+(4*FP32IDX_INV_PI)], inv_pi
                #define half %f4
                ld      [fp32c+(4*FP32IDX_0_5)], half
                #define pi %f5
                ld      [fp32c+(4*FP32IDX_PI)], pi
!   i =( x_ >> 23 )& 0xff;
                srl     %o0, 23, %g1
                and     %g1, 0xff, i
!   // is it NaN / Inf / too large a number (i.e. at least 2^29)?
!   if( i >= 29+127 ) {
!     // return NaN
!     return fp32c[FP32IDX_QNAN];
!   }
                cmp     i, 29+127
                bgeu,a  .L.done
                  ld    [fp32c+(4*FP32IDX_QNAN)], y
!   y0 = *(float*)(&x_);
!   sgn = 0;
                st      %o0, [%sp+64]
                clr     sgn
!   y = fabsf( y0 );
!   y0 = y;
                ld      [%sp+64], y0
                fabss   y0, y
                fabss   y, y0
!   if( !cosine )
!     sgn = x_ >> 31;
!   else
!     y += fp32c[FP32IDX_HALF_PI];
                tst     %o1
                bz,a    .L.per_fn
                  srl   %o0, 31, sgn
                ld      [fp32c+(4*FP32IDX_HALF_PI)], g
                fadds   y, g, y
.L.per_fn:
!   // bring to an acceptable interval
!   y = y * fp32c[FP32IDX_INV_PI] + fp32c[FP32IDX_0_5];
                fmuls   y, inv_pi, y
                fadds   y, half, y
!   i = (int)y;
                fstoi   y, g
                st      g, [%sp+64]
                ld      [%sp+64], i
!   sgn ^=( i & 1 );
                and     i, 1, i
                xor     sgn, i, sgn
!   y = (float)i;
                fitos   g, y
!   if( cosine )
!     y -= fp32c[FP32IDX_0_5];
                tst     %o1
                bnz,a   .+8 ! skipz
                  fsubs y, half, y
!   y *= fp32c[FP32IDX_PI];
!   y = y0 - y;
                fmuls   y, pi, y
                fsubs   y0, y, y
!   g = y * y;
!   y0 = 0.0;
                fmuls   y, y, g
                fsubs   y, y, y0
!   fp32c += FP32IDX_TABLE_SINE;
!   for( i = FP32IDX_TABLE_SINE_N; i; --i ) {
!     y0 *= g;
!     y0 += *fp32c++;
!   }
                inc     4*FP32IDX_TABLE_SINE, fp32c
                mov     FP32IDX_TABLE_SINE_N, i
.L.for:
                ld      [fp32c], %f3
                fmuls   y0, g, y0
                fadds   y0, %f3, y0
                deccc   i
                bnz     .L.for
                  inc   4, fp32c
!   y0 *= g;
!   y0 *= y;
!   y += y0;
                fmuls   y0, g, y0
                fmuls   y0, y, y0
                fadds   y, y0, y
!   if( sgn )
!     y = -y;
                tst sgn
                bnz,a   .+8 ! skipz
                  fnegs y, y
!   return y;
.L.done:
                retl
                  add   %sp, 72, %sp
! }
                .size   sinef, . - sinef
