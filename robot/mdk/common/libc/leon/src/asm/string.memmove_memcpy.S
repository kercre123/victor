	.global	memmove
	.global	memcpy
	.weak	memmove
	.weak	memcpy

	.section ".text.libc", "ax", @progbits

!;-----------------------------------------------------------------------------
!; void *memmove( void *dst, const void *src, size_t len );                    
!; void *memcpy( void *dst, const void *src, size_t len );                     
!;-----------------------------------------------------------------------------

	.type	memmove, #function
	.weak	memmove
memmove:
	#define	dest %o0
	#define	result %o0
	#define	src %o1
	#define	n %o2

	#define i %o3
	#define	tmp %g1
	cmp	dest, src
	blu,a	.L.memcpy.loop
	  clr	i

.L.memrcpy.loop:
	deccc	n
	bcs	.L.done
	  ldub	[src + n], tmp
	b	.L.memrcpy.loop
	  stb	tmp, [dest + n]
	.size	 memmove, . - memmove

	.type	memcpy, #function
	.weak	memcpy
memcpy:

	clr	i
.L.memcpy.loop:
	cmp	i, n
	bgeu	.L.done
	  ldub	[src + i], tmp
	stb	tmp, [dest + i]
	b	.L.memcpy.loop
	  inc	i
.L.done:
	retl
	  nop
	.size	memcpy, . - memcpy
