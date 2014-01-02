	.global	memset
	.weak	memset

	.section ".text.libc", "ax", @progbits

!;-----------------------------------------------------------------------------
!; void *memset( void *dst, int ch, size_t len );                              
!;-----------------------------------------------------------------------------

	.type	memset, #function
memset:
	#define	dest %o0
	#define	result %o0
	#define	ch %o1
	#define	n %o2
	#define	tmp %g1

.L.loop:
	deccc	n
	bcc,a	.L.loop
	  stb	ch, [dest + n]
	retl
	  nop
	.size	memset, . - memset
