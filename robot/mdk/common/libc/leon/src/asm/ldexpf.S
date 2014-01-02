                .section ".text", "ax", @progbits

!=[ ldexpf ]===================================================================

                .global ldexpf
! float ldexp( unsigned x_, int e ) {
                #define x_ %o0
                #define e %o1
ldexpf:
                .type   ldexpf, #function
                add     %sp, -72, %sp
!   unsigned exp =( x_>> 23 )& 0xff;
		#define	exp %o2
		srl	x_, 23, exp
!   if( !exp )
!     return *(float*)&x_;
		andcc	exp, 0xff, exp
		bz,a	.L.done
		  clr	exp
!   if( exp == 0xff )
!     return *(float*)&x_;
		cmp	exp, 0xff
		be,a	.L.done
		  clr	exp
!   exp += e;
		addcc	exp, e, exp
!   if( (int)exp <= 0 ) {
!     x_ &= 0x80000000;
!     return *(float*)&x_;
!   }
.L.underflow:
		bg	.L.overflow
		  set	0x80000000, %o3
		and	x_, %o3, x_
		b	.L.done
		  clr	exp
.L.overflow:
!   // overflow:  return correctly signed Inf
!   if( exp > 0xfe ) {
!     x_ &= 0x80000000;
!     exp = 0x7f800000;
!     x_ |= exp;
!     return *(float*)&x_;
!   }
		cmp	exp, 0xfe
		bleu	.L.normal
		  set	0x7f800000, %g1
		and	x_, %o3, x_
		b	.L.done
		  mov	%g1, exp
.L.normal:
!   // none: we're cool
!   x_ &= ~0x7f800000;
!   exp <<= 23;
		sll	exp, 23, exp
		andn	x_, %g1, x_
!   x_ |= exp;
!   return *(float*)&x_;
.L.done:
		bset	exp, x_
		st	x_, [%sp+64]
		ld	[%sp+64], %f0
! }
		retl
		  add	%sp, 72, %sp
		.size	ldexpf, . - ldexpf
