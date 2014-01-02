                .section ".text", "ax", @progbits
                .global frexpf
! float frexpf( unsigned x_, int *exp ) {
                #define x_ %o0
                #define exp %o1
frexpf:
                .type   frexpf, #function
                add     %sp, -72, %sp
!   unsigned e = x_ & 0x7f800000;
                #define e %o2
                set     0x7f800000, %g1
                andcc   x_, %g1, e
!   if( e == 0x7f800000 || !e ) {
!     *exp = 0;
!     return *(float*)&x_;
!   }
                bz,a    .L.done
                  clr   e
                cmp     e, %g1
                be,a    .L.done
                  clr   e
!   x_ &= ~0x7f800000;
!   e >>= 23;
!   e -= 126;
!   x_ |= 0x3f000000;
                andn    x_, %g1, x_
                srl     e, 23, e
                set     0x3f000000, %g1
                sub     e, 126, e
                bset    %g1, x_
.L.done:
!   *exp = e;
                st      e, [exp]
!   return *(float*)&x_;
                st      x_, [%sp+64]
                ld      [%sp+64], %f0
                retl
                  add   %sp, 72, %sp
! }
                .size   frexpf, . - frexpf
