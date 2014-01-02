                #include "math_consts.h"

! nice Gaisler bug: 0x3e9d1bcf+0x3f317218 == 0.5 when rounding to nearest or 
! +inf (should be 0)
                
                .section ".text", "ax", @progbits

!-[ logf ]---------------------------------------------------------------------

                .global logf
! float logf( unsigned x_ ) {
logf:
                .type   logf, #function
!   return logarithmf( x_, -1 );
                mov     -1, %o1
                .size   logf, . - logf
! }
                
!=[ logarithm ]================================================================

! static float logarithmf( unsigned x_, int adjust_const_ofs ) {
logarithmf:
                .type   logarithmf, #function
                dec	72, %sp
! {
!   const float *fp32c = fp32consts;
                #define fp32c %o2
                set     fp32consts, fp32c

!   float x, z, w, q, t, tt;
                #define x %f0
                #define z %f1
                #define w %f2
                #define q %f3
                #define t %f4
                #define tt %f5

!   // filter out bad values
!   int s = x_ >> 31;
                #define s %o3
                srl     %o0, 31, s
!   // log(negative) = nan
!   if( s )
!     return fp32c[FP32IDX_QNAN];
                tst     s
                bnz,a    .L.done
                  ld    [fp32c+4*FP32IDX_QNAN], %f0
!   int e = x_ >> 23; // we know already that the sign bit 0
!   // log(0) = -inf
!   if( !e )
!     return fp32c[FP32IDX_INF_N];
                #define e %o4
                srl     %o0, 23, e
                tst     e
                bz,a    .L.done
                  ld    [fp32c+4*FP32IDX_INF_N], %f0
!   // log(inf) = inf; log(nan) = nan
!   if( e == 0xff )
!     return *(float*)&x_;
                cmp     e, 0xff
                bne     .L.ok
                  set   0x7f800000, %g1
                st      %o0, [%sp+64]
                b       .L.done
                  ld    [%sp+64], %f0

.L.ok:

!   hack: with rounding to nearest, log(e) == 0.5!!!
                set     0x402DF854, %o5
                cmp     %o0, %o5
                be,a    .L.adj_done
                  ld    [fp32c+4*FP32IDX_1], %f0

!   // frexp business - get exponent of normalized format
!   x_ &= ~0x7f800000;
!   x_ |= 0x3f000000;
!   x = *(float*)&x_;
!   t = fp32c[FP32IDX_0_5];
!   w = fp32c[FP32IDX_SQRT_0_5];
                set     0x3f000000, %o5
                bclr    %g1, %o0
                ld      [fp32c+4*FP32IDX_0_5], t
                bset    %o5, %o0
                ld      [fp32c+4*FP32IDX_SQRT_0_5], w
                st      %o0, [%sp+64]
                ld      [%sp+64], x

!   z = x - t;
                fsubs   x, t, z
!   e -= 126;
!   q = fp32c[FP32IDX_1];
!   if( z > w ) {
!     z -= t;
!     x *= t;
!   }
!   else {
!     --e;
!     x = z * t;
!   }
                fcmps   z, w
                dec     126, e
                fbl     .L.not_gte
                  ld    [fp32c+4*FP32IDX_1], q
.L.gte:
                fsubs   z, t, z
                b       .L.after_if
                  fmuls x, t, x
.L.not_gte:
                fmuls   z, t, x
                dec     e
.L.after_if:
!   x += t;
!   x = z / x;
!   w = x * x;
                fadds   x, t, x
                fdivs   z, x, x
                fmuls   x, x, w

!   z = t - t;
!   fp32c += FP32IDX_TABLE_LOGARITHM;
                fsubs   t, t, z
                inc     4*FP32IDX_TABLE_LOGARITHM, fp32c
!   for( s = 3; s > 0; --s ) {
                mov     3, s
.L.iter:
!     t = *fp32c; tt = fp32c[1];
                ld      [fp32c], t
                ld      [fp32c+4], tt
!     z *= w; q *= w;
                fmuls   z, w, z
                fmuls   q, w, q
!     z += t; q += tt;
                fadds   z, t, z
                fadds   q, tt, q
!     fp32c += 2;
!   }  
                deccc   s
                bnz     .L.iter
                  inc   8, fp32c
!   w *= x;
!   w *= z;
!   w /= q;
!   x += w;
                fmuls   w, x, w
                fmuls   w, z, w
                fdivs   w, q, w
                dec     4*(FP32IDX_TABLE_LOGARITHM+6), fp32c
                fadds   x, w, x
!   if( e ) {
!     t = *fp32c;
!     tt = (float)e;
!     t *= tt;
!     x += t;
!   }
                tst     e
                bz      .L.adj_done
                  st      e, [%sp + 64]
                ld      [fp32c+4*(FP32IDX_TABLE_LOGARITHM+6)], t
                ld      [%sp+64], tt
                fitos   tt, tt
                fmuls   t, tt, t
                fadds   x, t, x
.L.adj_done:
!   if( adjust_const_ofs >= 0 )
!     x *= fp32c[adjust_const_ofs];
                cmp     %o1, 0
                bl      .L.done
                  sll   %o1, 2, %o1
                ld    [ fp32c + %o1 ], t
                fmuls   x, t, x

!   return x;
.L.done:
                retl
                  inc	72, %sp
! }
                .size   logarithmf, . - logarithmf

!-[ log2f ]--------------------------------------------------------------------

                .global log2f
! float log2f( unsigned x_ ) {
log2f:
                .type   log2f, #function
!   return logarithmf( x_, FP32IDX_LOG_E );
                b       logarithmf
                  mov   FP32IDX_INV_LN_2, %o1
                .size   log2f, . - log2f
! }
                
!-[ log10f ]-------------------------------------------------------------------

                .global log10f
! float log10f( unsigned x_ ) {
log10f:
                .type   log10f, #function
!   return logarithmf( x_, FP32IDX_LOG_E );
                b       logarithmf
                  mov   FP32IDX_LOG_E, %o1
                .size   log10f, . - log10f
! }
                
