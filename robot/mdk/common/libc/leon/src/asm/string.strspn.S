	.global	strspn
	.weak	strspn

	.section ".text.libc", "ax", @progbits

!;-----------------------------------------------------------------------------
!; int strspn( const char *src, const char *set );                             
!;-----------------------------------------------------------------------------

	.type	strspn, #function
strspn:
	#define	src %o0
	#define	result %o0
	#define	sset %o1

	#define i %o2
	#define	c %o3
	#define iset %o4
	#define	tmp %g1
	clr	i
.L.loop.source:
	ldub	[src + i], c
	tst	c
	bz	.L.done
	  mov	sset, iset

.L.loop.include:
	ldub	[iset], tmp
	tst	tmp
	bz	.L.done
	  cmp	c, tmp
	bne,a	.L.loop.include
	  inc	iset

	b	.L.loop.source
	  inc	i

.L.done:
	retl
	  mov	i, result
	.size	strspn, . - strspn
