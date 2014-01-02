	.global	strrchr
	.weak	strrchr

	.section ".text.libc", "ax", @progbits

!;-----------------------------------------------------------------------------
!; char *strrchr( char *str, int ch );                                         
!;-----------------------------------------------------------------------------

	.type	strrchr, #function
strrchr:
	#define	src %o0
	#define	result %o0
	#define ch %o1
  #define skipne	  be,a	  .+8;

	and	ch, 0xFF, ch
	#define	tmp %g1
	#define tmp2 %o2
	clr	tmp2
.L.loop:
	ldub	[src], tmp
	cmp	tmp, ch
	skipne
	  mov	src, tmp2
	tst	tmp
	bnz,a	.L.loop
	  inc	src
	retl
	  mov	tmp2, result
	.size	strrchr, . - strrchr
