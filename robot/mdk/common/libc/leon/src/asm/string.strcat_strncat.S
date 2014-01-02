	.global	strcat
	.global	strncat
	.weak	strcat
	.weak	strncat
	.section ".text.libc", "ax", @progbits

!;-----------------------------------------------------------------------------
!; char *strncat( char *dst, const char *src, size_t len );                    
!; char *strcat( char *dst, const char *src );                                 
!;-----------------------------------------------------------------------------

	.type	strcat, #function
strcat:
	mov	-1, %o2
	.size	strcat, . - strcat

	.type	strncat, #function
strncat:
	#define	dest %o0
	#define	result %o0
	#define	src %o1
	#define	nmax %o2	
	#define	tmp %g1
	#define tmp2 %o3
	mov	dest, tmp2
	
.L.seek.loop:
	ldub	[dest], tmp
	tst	tmp
	bnz,a	.L.seek.loop
	  inc	dest

.L.cpy.loop:
	ldub	[src], tmp
	tst	tmp
	bz	.L.done
	tst	nmax
	bz	.L.done
	  dec	nmax
	stb	tmp, [dest]
	inc src
	b	.L.cpy.loop
	  inc	dest

.L.done:
	clrb	[dest]
	retl
	  mov	tmp2, %o0
	.size	strncat, . - strncat
