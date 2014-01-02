	.global	strcpy
	.global	strncpy.
	.weak	strcpy
	.weak	strncpy

	.section ".text.libc", "ax", @progbits

!;-----------------------------------------------------------------------------
!; char *strncpy( char *dst, const char *src, size_t len );                    
!; char *strcpy( char *dst, const char *src );                                 
!;-----------------------------------------------------------------------------

	.type	strcpy, #function
strcpy:
	mov	-1, %o2
	.size	strcpy, . - strcpy

	.type	strncpy, #function
strncpy:
	#define	dest %o0
	#define	result %o0
	#define	src %o1
	#define	n %o2

	#define	tmp %g1
	#define tmp2 %o3
	mov	dest, tmp2
.L.copy.loop:
	deccc	n
	bcs	.L.done
	  ldub	[src], tmp
	tst	tmp
	stb	tmp, [dest]
	inc	src
	bnz,a	.L.copy.loop
	  inc	dest

	cmp	n, 0
	bl	.L.done
.L.fill.loop:
	deccc	n
	clrb	[dest]
	bcc,a	.L.fill.loop
	  inc	dest

.L.done:
	retl
	  mov	tmp2, result
	.size	strncpy, . - strncpy
