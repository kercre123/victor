	.global	strchr
	.global	memchr
	.weak	strchr
	.weak	memchr

	.section ".text.libc", "ax", @progbits

!;-----------------------------------------------------------------------------
!; char *strchr( char *str, int ch );                                          
!; void *memchr( void *str, int ch, size_t len );                              
!;-----------------------------------------------------------------------------

	.type	strchr, #function
strchr:
	mov	-1, %o2
	b	___chr
	  clr	%o3
	.size	strchr, . - strchr

	.type	memchr, #function
memchr:
	#define	src %o0
	#define	result %o0
	#define	ch %o1
	#define	n %o2
	#define	tmp %g1

	mov	0x100, %o3
	#define	stop %o3
___chr:
	and	ch, 0xFF, ch
.L.loop:
	deccc	n
	bcs,a	.L.done
	  clr	result
	ldub	[src], tmp
	cmp	tmp, ch
	be	.L.done
	cmp	tmp, stop
	bne,a	.L.loop
	  inc	src
	clr	result
.L.done:
	retl
	  nop
	.size	memchr, . - memchr
