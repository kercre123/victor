	.global	strcspn
	.global	strlen
	.weak	strcspn
	.weak	strlen

	.section ".text.libc", "ax", @progbits

!;-----------------------------------------------------------------------------
!; int strcspn( const char *src, const char *set );                            
!; int strlen( const char *src );                                              
!;-----------------------------------------------------------------------------

	.type	strlen, #function
strlen:
	sethi	%hi( __const_0d ), %o1
	bset	%lo( __const_0d ), %o1
	.size	strlen, . - strlen

	.type	strcspn, #function
strcspn:
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

.L.loop.exclude:
	ldub	[iset], tmp
	cmp	c, tmp
	be	.L.done
	  tst	tmp
	bnz,a	.L.loop.exclude
	  inc	iset

	b	.L.loop.source
	  inc	i

.L.done:
	retl
	  mov	i, result
	.size	strcspn, . - strcspn
