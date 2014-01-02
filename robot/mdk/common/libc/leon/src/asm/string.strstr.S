	.global	strstr
	.weak	strstr

	.section ".text.libc", "ax", @progbits

!;-----------------------------------------------------------------------------
!; char *strstr( char *str, const char *substr );                              
!;-----------------------------------------------------------------------------

	.type	strstr, #function
strstr:
	#define	src %o0
	#define	result %o0
	#define	substr %o1

	#define i %o2
	#define	c %o3
	#define	tmp %g1
.L.loop.source:
	ldub	[src], c
	tst	c
	bz,a	.L.done
	  clr	result

	clr	i
.L.loop.both:
	ldub	[substr + i], tmp
	tst	tmp
	bz	.L.done
	  cmp	c, tmp
	bne,a	.L.loop.source
	  inc	src
	inc	i
	b	.L.loop.both
	  ldub	[src + i], c

.L.done:
	retl
	  nop
	.size	strstr, . - strstr
