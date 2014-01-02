                .section ".text", "ax", @progbits

!=[ modff ]====================================================================

                .global modff
! float modff( unsigned x_, float *iptr ) {
                #define x_ %o0
                #define iptr %o1
modff:
                .type   modff, #function
                dec	72, %sp
!   float x, y;
!   unsigned t;
!   unsigned signbit;
		#define	x %f0
		#define y %f1
		#define t %o2
		#define signbit %o3
!   unsigned exp = x_ << 1;
		#define exp %o4
		sll	x_, 1, exp
!   if( exp < (127<<24) ) { 
!     *(unsigned*)iptr = x_ & 0x80000000; 
!     return *(float*)&x_; 
!   }
		set	127<<24, %g1
		cmp	exp, %g1
		bgeu	.L.mf.not_small
		  set	0x80000000, signbit
		and	x_, signbit, %g1
		b	.L.mf.ret_arg
		  st	%g1, [ iptr ]
.L.mf.not_small:
!   if( exp >= ((23+127)<<24) ) { 
!   integral:
!     *(unsigned*)iptr = x_;
!     if( exp <= 0xff000000 )
!       x_ &= 0x80000000;
!     return *(float*)&x_; // fractional part is +-0
!   }
!   exp >>= 24;
		set	(23+127)<<24, %g1
		cmp	exp, %g1
		blu,a	.L.mf.not_large
		  srl	exp, 24, exp
.L.mf.integral:
		st	x_, [ iptr ]
		set	0xff000000, %g1
		cmp	exp, %g1
		bleu,a	.+8 ! skipgu
		  and	x_, signbit, x_
.L.mf.ret_arg:
		st	x_, [ %sp + 64 ]
		ld	[ %sp + 64 ], x
.L.mf.ret:
		retl
		  inc	72, %sp
.L.mf.not_large:
!   exp -= 0x7f;
!   t = ~0x007fffff;
!   t >>= exp; 
		dec	0x7f, exp
		set	0xff800000, t
		sra	t, exp, t
!   if( !( x_ & ~t )) goto integral;
		andncc	x_, t, %g0
		bz	.L.mf.integral
!   *(unsigned*)iptr = x_ & t;
!   y = *iptr;
!   x = *(float*)&x_;
		and	x_, t, %g1
		st	x_, [ %sp + 64 ]
		st	%g1, [ iptr ]
		ld	[ %sp + 64 ], x
		ld	[ iptr ], y
!   x -= y;
!   return x;
		b	.L.mf.ret
		  fsubs	x, y, x
! }
                .size   modff, . - modff
                #undef	x_
                #undef	iptr
		#undef	x
		#undef	y
		#undef	t
		#undef	signbit
		#undef	exp

!=[ ceilf ]====================================================================

                .global ceilf
! float ceilf( unsigned x_ ) {
                #define x_ %o0
ceilf:
                .type   ceilf, #function
                dec	72, %sp
!   unsigned t;
		#define	t %o1
!   unsigned sgn = x_ & 0x80000000;
!   unsigned exp =( x_ >> 23 )& 0xff;
!   if( !exp ) {
!     x_ = sgn;
!     return *(float*)&x_;
!   }
		#define sgn %o2
		#define	exp %o3
		srl	x_, 23, exp
		set	0x80000000, %g1
		andcc	exp, 0xff, exp
		and	x_, %g1, sgn
		bz,a	.L.c.done
		  mov	sgn, x_
!   exp -= 127;
!   if( (int)exp < 0 ) { 
!     if( sgn )
!       x_ = sgn;
!     else
!       x_ = 0x3f800000;
!     return *(float*)&x_;
!   }
		deccc	127, exp
		bge,a	.L.c.not_small
		  cmp	exp, 23
		tst	sgn
		bnz,a	.L.c.done
		  mov	sgn, x_
		b	.L.c.done
		  set	0x3f800000, x_
.L.c.not_small:
!   if( exp >= 23 )
!     return *(float*)&x_;
!   t = (int)0xff800000 >> exp;
		bgeu	.L.c.done
		  set	0xff800000, %g1
		sra	%g1, exp, t
!   if( !( x_ & ~t ))
!     return *(float*)&x_;
		andncc	x_, t, %g0
		bz	.L.c.done
		set	0x00800000, %g1
!   x_ &= t;
!   if( !sgn ) {
!     t = 0x00800000 >> exp;
!     x_ += t;
!   }
		tst	sgn
		and	x_, t, x_
		bnz	.L.c.done
		  srl	%g1, exp, t
		add	x_, t, x_
.L.c.done:
!   return *(float*)&x_;
		st	x_, [ %sp + 64 ]
		ld	[ %sp + 64 ], %f0
		retl
		  inc	72, %sp
! }
		.size	ceilf, . - ceilf
                #undef	x_
		#undef	t
		#undef	sgn
		#undef	exp

!=[ floorf ]===================================================================

                .global floorf
! float floorf( unsigned x_ ) {
                #define x_ %o0
floorf:
                .type   floorf, #function
                dec	72, %sp
!   unsigned t;
		#define t %o1
!   unsigned sgn = x_ & 0x80000000;
!   unsigned exp =( x_ >> 23 )& 0xff;
!   if( !exp ) {
!     x_ = sgn;
!     return *(float*)&x_;
!   }
		#define sgn %o2
		#define	exp %o3
		srl	x_, 23, exp
		set	0x80000000, %g1
		andcc	exp, 0xff, exp
		and	x_, %g1, sgn
		bz,a	.L.f.done
		  mov	sgn, x_
!   exp -= 127;
!   if( (int)exp < 0 ) { 
!     if( !sgn )
!       x_ = sgn;
!     else
!       x_ = 0xbf800000;
!     return *(float*)&x_;
!   }
		deccc	127, exp
		bge,a	.L.f.not_small
		  cmp	exp, 23
		tst	sgn
		bz,a	.L.f.done
		  mov	sgn, x_
		b	.L.f.done
		  set	0xbf800000, x_
.L.f.not_small:
!   if( exp >= 23 )
!     return *(float*)&x_;
!   t = (int)0xff800000 >> exp;
		bgeu	.L.c.done
		  set	0xff800000, %g1
		sra	%g1, exp, t
!   if( !( x_ & ~t ))
!     return *(float*)&x_;
		andncc	x_, t, %g0
		bz	.L.f.done
		set	0x00800000, %g1
!   x_ &= t;
!   if( sgn ) {
!     t = 0x00800000 >> exp;
!     x_ += t;
!   }
		tst	sgn
		and	x_, t, x_
		bz	.L.f.done
		  srl	%g1, exp, t
		add	x_, t, x_
.L.f.done:
!   return *(float*)&x_;
		st	x_, [ %sp + 64 ]
		ld	[ %sp + 64 ], %f0
		retl
		  inc	72, %sp
! }
		.size	floorf, . - floorf
                #undef	x_
		#undef	t
		#undef	sgn
		#undef	exp
