	.global	memcmp
	.global	strncmp
	.global strcmp
	.weak	memcmp
	.weak	strncmp
	.weak	strcmp
  
	.section ".text.libc", "ax", @progbits

!;-----------------------------------------------------------------------------
!; int memcmp( const void *src1, const void *src2, size_t len );               
!; int strncmp( const char *src1, const char *src2, size_t len );              
!; int strcmp( const char *src1, const char *src2 );                           
!;-----------------------------------------------------------------------------

	.type	strcmp, #function
strcmp:
	mov	-1, %o2
	.size	strcmp, . - strcmp

	.type	strncmp, #function
strncmp:
	b	___cmp
	  clr	%o3
	.size	strncmp, . - strncmp
	
	.type	memcmp, #function
memcmp:
	#define	src %o0
	#define	result %o0
	#define	cosrc %o1
	#define	nmax %o2

	mov	0x100, %o3
	#define	stop %o3
___cmp:
	#define	tmp %g1
	#define	tmp2 %o4

.L.loop:
	deccc	nmax
	bcs,a	.L.done
	  clr	tmp
	ldub	[src], tmp
	ldub	[cosrc], tmp2
	subcc	tmp, tmp2, tmp
	bnz	.L.done
	  inc	src
	cmp	tmp2, stop
	bne,a	.L.loop
	  inc	cosrc

.L.done:
	retl
	  mov	tmp, result
	.size	memcmp, . - memcmp
