                #include "math_consts.h"

                .section ".text", "ax", @progbits

!=[ infinityf() ]==============================================================

                .global infinityf
! float infinityf() {
infinityf:
                .type   infinityf, #function
!   return fp32consts[FP32IDX_INF];
                sethi   %hi( fp32consts + 4*FP32IDX_INF ), %g1
                retl
                  ld    [%g1 + %lo( fp32consts + 4*FP32IDX_INF )], %f0
! }
                .size   infinityf, . - infinityf

!=[ nanf() ]===================================================================

                .global nanf
! float nanf( const char __attribute__((unused)) *unused ) {
nanf:
                .type   nanf, #function
!   return fp32consts[FP32IDX_QNAN];
                sethi   %hi( fp32consts + 4*FP32IDX_QNAN ), %g1
                retl
                  ld    [%g1 + %lo( fp32consts + 4*FP32IDX_QNAN )], %f0
! }
                .size   nanf, . - nanf

!=[ finitef ]==================================================================

                .global finitef
! int finitef( unsigned x_ ) {
finitef:
                .type   finitef, #function
!   x <<= 1;
!   return x_ < 0xff000000;
                sll     %o0, 1, %o1
                set     0xff000000, %g1
                mov     1, %o0
                cmp     %o1, %g1
                bgeu,a  . + 8 ! skiplu
                    clr %o0
                retl
                  nop
! }
                .size   finitef, . - finitef

!=[ isinff ]===================================================================

                .global isinff
! int isinff( unsigned x_ ) {
isinff:
                .type   isinff, #function
!   return ( x_ == 0x7f800000 )? 1 :    (
!          ( x_ == 0xff800000 )? -1 : 0 );
                sethi   %hi( 0x7f800000 ), %o1
                sethi   %hi( 0xff800000 ), %o2
                cmp     %o0, %o1
                mov     %o0, %o1
                clr     %o0
                be,a    1f
                 mov    1, %o0
                cmp     %o1, %o2
                be,a    1f
                 mov    -1, %o0
1:
                retl
                 nop
! }
                .size   isinff, . - isinff

!=[ isnanf ]===================================================================

                .global isnanf
! int isnanf( unsigned x_ ) {
isnanf:
                .type   isnanf, #function
!   x <<= 1;
!   return x_ > 0xff000000;
                sll     %o0, 1, %o1
                set     0xff000000, %g1
                clr     %o0
                cmp     %o1, %g1
                bgu,a   . + 8 ! skipleu
                    mov 1, %o0
                retl
                  nop
! }
                .size   isnanf, . - isnanf

!=[ isposf ]===================================================================

                .global isposf
! int isposf( unsigned x_ ) {
isposf:
                .type   isposf, #function
!   return ( x_ >> 31 )^ 1;
                srl     %o0, 31, %o0
                retl
                  xor   %o0, 1, %o0
! }
                .size   isposf, . - isposf

!=[ numtestf ]=================================================================

                .global numtestf
! int numtestf( unsigned x_ ) {
numtestf:
                .type   numtestf, #function
!   x_ <<= 1;
                sll     %o0, 1, %g1
                set     0xff000000, %o1
!   if( !x_ )
!     return 0;
                tst     %g1
                bz,a    .L.nt.done
                  clr   %o0
!   if( x_ < 0xff000000 )
!     return NUM;
                cmp     %g1, %o1
                blu,a   .L.nt.done
                  mov   NUM, %o0
!   if( x_ == 0xff000000 )
!     return INF;
                be,a    .L.nt.done
                  mov   INF, %o0
!   return NAN;
                mov     NAN, %o0
.L.nt.done:
                retl
                  nop
! }
                .size   numtestf, . - numtestf

!=[ __fpclassifyf ]============================================================

                .global __fpclassifyf
! int __fpclassifyf( unsigned x_ ) {
__fpclassifyf:
                .type   __fpclassifyf, #function
!   x_ <<= 1;
                sll     %o0, 1, %g1
!   if( !x_ )
!     return FP_ZERO;
                tst     %g1
                set     0x01000000, %o1
                bz,a    .L.fpc.done
                  mov   FP_ZERO, %o0
!   if( x  < 0x01000000 )
!     return FP_SUBNORMAL;
                cmp     %g1, %o1
                set     0xff000000, %o1
                blu,a   .L.fpc.done
                  mov   FP_SUBNORMAL, %o0
!   if( x_ < 0xff000000 )
!     return FP_NORMAL;
                cmp     %g1, %o1
                blu,a   .L.fpc.done
                  mov   FP_NORMAL, %o0
!   if( x_ == 0xff000000 )
!     return FP_INFINITE;
                be,a    .L.nt.done
                  mov   FP_INFINITE, %o0
!   return FP_NAN;
                mov     FP_NAN, %o0
.L.fpc.done:
                retl
                  nop
! }
                .size   __fpclassifyf, . - __fpclassifyf

!=[ copysignf ]================================================================

                .global copysignf
! float copysignf( unsigned x_, unsigned y_ ) {
copysignf:
                .type   copysignf, #function
                add     %sp, -72, %sp
!   x_ &= ~0x80000000;
!   y_ &= 0x80000000;
!   x_ |= y_;
                set     0x80000000, %g1
                andn    %o0, %g1, %o0
                and     %o1, %g1, %o1
                bset    %o1, %o0
!   return *(float*)&x_;
                st      %o0, [%sp+64]
                ld      [%sp+64], %f0
                retl
                  add   %sp, 72, %sp
! }
                .size   copysignf, . - copysignf

