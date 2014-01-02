!leon pseudoinstructions
// SKIP instructions

// - IU direct flag checking (ALU logical ops)
#define skipnz	  be,a	  .+8; 
#define skipz	  bne,a   .+8; 
#define skipcc	  bcs,a	  .+8; 
  #define skipnc skipcc
#define skipcs	  bcc,a	  .+8; 
  #define skipc skipcs
#define skipvc	  bvs,a	  .+8; 
  #define skipnv skipvc
#define skipvs	  bvc,a	  .+8; 
  #define skipv skipvs
#define skippos	  bneg,a  .+8; 
  #define skipns skippos
#define skipneg	  bpos,a  .+8; 
  #define skips skipneg

// - IU indirect flag checking (ALU arithmetic ops)
#define skipne	  be,a	  .+8; 
#define skipe	  bne,a	  .+8; 
#define skipg	  ble,a	  .+8; 
  #define skipnle skipg
#define skiple	  bg,a	  .+8; 
  #define skipng skiple
#define skipge	  bl,a	  .+8; 
  #define skipnl skipge
#define skipl	  bge,a	  .+8; 
  #define skinpge skipl
#define skipgu	  bleu,a  .+8; 
  #define skipnleu skipgu
#define skipleu	  bgu,a   .+8; 
  #define skipngu skipleu
#define skipgeu	  bcs,a	  .+8; 
  #define skipnlu skipgeu
#define skiplu	  bcc,a	  .+8; 
  #define skipngeu skiplu
!------------------------
#define putchar %i0
#define dest %i1
#define fmt %i2
#define p_crt_arg %i3
#define crt_fmt %i4
#define c %i5

#define caller_retaddr %i7
#define tmp %i7

#define flags %l0
  #define FL_LEFTALIGN	    0x001
  #define FL_FORCESIGN	    0x002
  #define FL_FORCESIGNSPACE 0x004
  #define FL_ALT_FORMAT	    0x008
  #define FL_ZEROFILL	    0x010
  #define FL_SHORT	    0x020
  #define FL_SIGNED	    0x040
  #define FL_UPPERCASE	    0x080
  #define FL_NEGATIVE	    0x100
#define width %l1
#define prec %l2
#define nchars %l3
#define req_chars %l4
#define case_ch %l5

#define old_retaddr %l7
#define older_retaddr %l6
!--------------------------

		.section ".text.libc", "ax", @progbits
		.global	_xprintf
    .weak   _xprintf
		.align	4
		.type	_xprintf, #function
_xprintf:								!{
		save	%sp, -64, %sp

!; backup the return address
		st	caller_retaddr, [%fp + 76]
!; save va_list (arguments 3..5) on the stack and build p_crt_arg 
		st	%i3, [%fp + 80]
		st	%i4, [%fp + 84]
		st	%i5, [%fp + 88]
		add	%fp, 80, p_crt_arg
		mov	%g0, %y
!; nchars = 0; 
		clr	nchars						!}

!;-----------------------------------------------------------------------------

.L.charloop:								!{
!; _charloop: 
!;   if( !( c = *fmt )) goto _end; 
		ldub	[fmt], c
		tst	c
		bz	.L.end
!;   ++fmt; if( c == '%' ) goto _format; 
		cmp	c, '%'
		be	.L.format
		  inc	fmt
.L.put_percent:
!; _put_percent: 
!;   output_char( c ); 
		call	.L.output_char
		  nop
!;   goto _charloop; 
		b,a	.L.charloop					!}

!;=============================================================================
!;=============================================================================
!;=============================================================================

!;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
!; perform size correction based on the "h" size modifier to the value in %o0 ;
!; also compute absolute value and setup NEGATIVE flag, if signed ;;;;;;;;;;;;;
!;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
.L.load_dec:								!{
		btst	FL_SHORT, flags
		bz	.L.lddec.size_ok
		btst	FL_SIGNED, flags
		sll	%o0, 16, %g1
		bz,a	.L.lddec.size_ok
		  srl	%g1, 16, %o0
		sra	%g1, 16, %o0 ! signed branch
.L.lddec.size_ok:
		
!; check sign and compute absolute value 
		btst	%o4, %o0
		bz,a	.L.lddec.sign_ok
		  nop
		subcc	%g0, %o0, %o0
		bset	FL_NEGATIVE, flags
.L.lddec.sign_ok:

!; disable alt. format if argument is null 
		tst	%o0
		skipnz
		  bclr	FL_ALT_FORMAT, flags
		retl
		  inc	4, p_crt_arg
									!}
!;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
!; count the required chars for the value in %o0 (excluding sign); store to ;;;
!; req_chars (modes "d"/"i", "u", "p", "x"/"X", "o"); %o1 contains the radix ;;
!;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
.L.count_uchars:							!{
		mov	1, %g1
		mov	1, req_chars
!; compute the minimum power of the radix to which %o0 is smaller 
.L.cntu:
		umul	%g1, %o1, %g1
		mov	%y, %o5		!; detect overflow 
		tst	%o5
		bnz,a	.L.cntu.done
		  mov	%g0, %y		!; keep %y clear at all times 
		cmp	%g1, %o0
		bleu,a	.L.cntu
		  inc	req_chars
.L.cntu.done:
		retl
		! nop ! not required 
									!}

!;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
!; substracts from width the required number of chars (takes the precision ;;;;
!; into account); aditionally, %o3 is set if any bits in %o2 are set in flags ;
!;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
.L.fix_width:								!{
		mov	req_chars, %g1
		cmp	req_chars, prec
		skipge
		  mov	prec, %g1
		btst	%o2, flags
		skipz
		  add	%g1, %o3, %g1
		retl
		  sub	width, %g1, width				!}

!;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
!; does the padding (if appropriate) note that - at this stage - tmp contains ;
!; the required number of padding chars (from either width or prec); NOTE ;;;;;
!; THAT the caller has to copy %o7 to older_retaddr manually for .L.pad_right ;
!; and .L.pad_0s ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
!;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
.L.pad_right:								!{
		mov	' ', c
		b	.L.pad.loop
.L.pad_left:
		  mov	%o7, older_retaddr
		btst	FL_LEFTALIGN, flags
		bnz	.L.pad.done
		  mov	' ', c
		btst	FL_ZEROFILL, flags
		skipz
.L.pad_0s:
		  mov	'0', c
.L.pad.loop:
		deccc	tmp
		bneg,a	.L.pad.done
		  nop
		call	.L.output_char
		  nop
		b,a	.L.pad.loop
.L.pad.done:
		jmp	older_retaddr + 8
		! nop ! not needed 
									!}

!;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
!; "put" a character (store to a string if dest not NULL, and - always - call ;
!; the putchar() function; note, _xprintf provides a void putchar() function ;;
!; if the putchar parameter was NULL) ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
!;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
.L.output_char:								!{
		tst dest
		bz .L.dest_is_NULL
		  mov	%o7, old_retaddr
		stb	c, [dest]
		inc	dest
.L.dest_is_NULL:
		tst putchar
		skipz
		  jmpl	putchar, %o7
		    mov	c, %o0
		jmp	old_retaddr + 8
		  inc	nchars						!}

!;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
!; local itoa (expects x in %o0, radix in %o1, chars already set) ;;;;;;;;;;;;;
!;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
.L._xitoa:								!{
#define input_x c
#define x req_chars
#define radix flags
#define divisor prec

		mov	%o7, older_retaddr
!; build maximum divisor
		mov	1, divisor
.L.build_div:
		deccc	req_chars
		bnz,a	.L.build_div
		  umul	divisor, radix, divisor
		mov	input_x, x
.L.itoa.digit:
!; build the current digit in tmp and adjust x
		udiv	x, divisor, tmp
		umul	divisor, tmp, %g1
		sub	x, %g1, x
		add	tmp, '0', tmp
		cmp	tmp, '9'
		skiple
		  add	tmp, case_ch, tmp
		call	.L.output_char
		  mov	tmp, c
		udivcc divisor, radix, divisor
		bnz	.L.itoa.digit
		  mov	%g0, %y
!; return
		jmp	older_retaddr + 8
		! nop ! not needed, prec doesn't matter at this stage 
#undef input_x
#undef x
#undef radix
#undef divisor
									!}

!;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
!; get a decimal value from the format string (width or precision); returns ;;;
!; result in prec; handles `*' ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
!;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
.L.get_decimal:								!{
		cmp	c, '*'
		be,a	.L.getdec.star
		  inc	crt_fmt

.L.getdec:
		subcc	c, '0', %o1
		bneg	.L.getdec.end
		cmp	%o1, 9
		bgu	.L.getdec.end

		umul	prec, %o0, %g1
		add	%g1, %o1, prec
		
		inc	crt_fmt
		b	.L.getdec
.L.getdec.star:
		  ldub	[crt_fmt], c
		ld	[p_crt_arg], prec
		inc	4, p_crt_arg
.L.getdec.end:
		retl
		  nop
									!}

!;=============================================================================
!;=============================================================================
!;=============================================================================

.L.format:								!{
!; _format: 
!;   flags = 0; 
		clr	flags

!;   crt_fmt = fmt; c = *crt_fmt; 
		mov	fmt, crt_fmt
		ldub	[crt_fmt], c

!;   if( c == '%' ) { ++fmt; goto _put_percent; } 
		cmp	c, '%'
		be,a	.L.put_percent
		  inc	fmt						!}

!;-----------------------------------------------------------------------------

!; load flags 
.L.process_flags:							!{
		cmp	c, '-'
		be,a	.L.next_flag
		  bset	FL_LEFTALIGN, flags

		cmp	c, '+'
		be,a	.L.next_flag
		  bset	FL_FORCESIGN, flags

		cmp	c, ' '
		be,a	.L.next_flag
		  bset	FL_FORCESIGNSPACE, flags

		cmp	c, '#'
		be,a	.L.next_flag
		  bset	FL_ALT_FORMAT, flags

		cmp	c, '0'
		be,a	.L.next_flag
		  bset	FL_ZEROFILL, flags

		b,a	.L.flags_end
.L.next_flag:
		inc	crt_fmt
		b	.L.process_flags
		  ldub	[crt_fmt], c
.L.flags_end:								!}

!;-----------------------------------------------------------------------------

!; load width 
.L.get_width:								!{
		clr	prec
		call	.L.get_decimal
		  mov	10, %o0
!; if negative, get absolute value and set `-' flag (left align) 
		addcc	prec, %g0, width
		bpos	.L.wdth.sign_ok
		  mov	-1, prec
		sub	%g0, prec, width
		bset	FL_LEFTALIGN, flags
.L.wdth.sign_ok:							!}

!;-----------------------------------------------------------------------------

!; process precision field (if any) 
.L.maybe_precision:							!{
		cmp	c, '.'
		bne	.L.precision.done
		  nop
		clr	prec
.L.get_prec:
		inc	crt_fmt
		call	.L.get_decimal
		  ldub	[crt_fmt], c
!; negative precision gets ignored by design 
.L.precision.done:							!}

!;-----------------------------------------------------------------------------

!; load size field 
.L.size:								!{
		cmp	c, 'h'
		be,a	.L.next_size
		  bset	FL_SHORT, flags

		cmp	c, 'l'
		be,a	.L.next_size
		  bclr	FL_SHORT, flags

		b,a	.L.size_end
.L.next_size:
		inc	crt_fmt
		b	.L.size
		  ldub	[crt_fmt], c
.L.size_end:								!}

!;-----------------------------------------------------------------------------

!; the type field switch 
.L.type:								!{	

!;- %n: store character count -------------------------------------------------

		cmp	c, 'n'
		be,a	.L.nfmt
		  ld	[p_crt_arg], %g1
		
!;- %c, %s: character / string format -----------------------------------------

		cmp	c, 'c'
		be	.L.char_output

		cmp	c, 's'
		be	.L.str_output
		  clr	req_chars

!;- %d, %i, %u, %x, %X, %o, %p: integer format --------------------------------

!; prepare for the integer types 
		mov	16, %o1 ! radix: hexadecimal 
		mov	FL_ALT_FORMAT, %o2 ! width decrease condition 
		mov	2, %o3 ! width decrease amount for alt ('0x') 
		clr	%o4 ! negative detection mask 
		mov	'A' - '0' - 10, case_ch

!; switch 
		cmp	c, 'p'
		be	.L.pfmt
		cmp	c, 'X'
		be	.L.xfmt
		cmp	c, 'x'
		be,a	.L.xfmt
		  mov	'a' - '0' - 10, case_ch
		
		mov	1, %o3 ! width decrease amount for alt ('0') 

		cmp	c, 'o'
		be,a	.L.ofmt
		  mov	8, %o1 ! radix: octal 
		
		cmp	c, 'u'
		be	.L.ufmt
		  mov	10, %o1 ! radix: decimal 

		mov	FL_FORCESIGN | FL_FORCESIGNSPACE | FL_NEGATIVE, %o2
						! width condition 
		set	0x80000000, %o4 ! negative detection mask 

		cmp	c, 'd'
		be,a	.L.difmt
		  bset	FL_SIGNED, flags
		cmp	c, 'i'
		be,a	.L.difmt
		  bset	FL_SIGNED, flags

!;- %f, %e, %E, %g, %G: floating point format ---------------------------------

!		cmp	c, 'f'
!		be	.L.ffmt
		
!		cmp	c, 'E'
!		be,a	.L.efmt
!		  mov	'E', case_ch
!		cmp	c, 'e'
!		be,a	.L.efmt
!		  mov	'e', case_ch
		
!		cmp	c, 'G'
!		be,a	.L.gfmt
!		  mov	'E', case_ch
!		cmp	c, 'g'
!		be,a	.L.gfmt
!		  mov	'e', case_ch

!; not a known type -> this is not a format string afterall
		b	.L.put_percent
		  mov	'%', c						!}
		
!;=============================================================================

!;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
!; store number of chars already written (%n) ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
!; note, %hn means store as short (instead of long) ;;;;;;;;;;;;;;;;;;;;;;;;;;;
!;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
.L.nfmt:								!{
		inc	4, p_crt_arg
		btst	FL_SHORT, flags
		bnz,a	.L.fmt_done
		  sth	nchars, [%g1]
		st	nchars, [%g1]

.L.fmt_done:
		b	.L.charloop
		  add	crt_fmt, 1, fmt					!}

!;=============================================================================

!;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
!; print pointer or hex (%p, %x, %X) ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
!;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
.L.pfmt:								!{
		bset	FL_ALT_FORMAT, flags
		bclr	FL_SHORT, flags
.L.xfmt:								!}

!;-----------------------------------------------------------------------------

!;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
!; print octal (%o) ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
!;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
.L.ofmt:								!{
		b	.L.number_output				!}

!;-----------------------------------------------------------------------------

!;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
!; print unsigned (%u) ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
!; print decimal (%d, %i) ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
!;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
.L.ufmt:								!{
		  bclr	FL_FORCESIGN | FL_FORCESIGNSPACE, flags
									!}
.L.difmt:								!{
		bclr	FL_ALT_FORMAT, flags				!}

!;-----------------------------------------------------------------------------

.L.number_output:							!{
!; load data
		call	.L.load_dec
		  ld	[p_crt_arg], %o0

!; count minimum required chars and fix width (also backup data and radix) 
!; = substract max(precision,req_chars) from it 
		call	.L.count_uchars
		  st	%o1, [ %fp + 68 ] ! backup radix 
		call	.L.fix_width
		  st	%o0, [ %fp + 72 ] ! backup data 

!; pre-sign/prefix padding (skip if zero-filling) 
		btst	FL_ZEROFILL, flags
		bnz	.L.put_sign
		  nop
		call	.L.pad_left
		  mov	width, tmp
		mov	tmp, width
!; output sign 
.L.put_sign:
		btst	FL_NEGATIVE, flags
		bnz,a	.L.put_sign.do
		  mov	'-', c
		btst	FL_FORCESIGN | FL_FORCESIGNSPACE, flags
		bz	.L.put_sign.done
		  mov	'+', c
		btst	FL_FORCESIGNSPACE, flags
		skipz
		  mov	' ', c
.L.put_sign.do:
		call	.L.output_char
		  nop
.L.put_sign.done:

!; output prefix - skip if argument is null 
.L.put_prefix:
		btst	FL_ALT_FORMAT, flags
		bz,a	.L.put_prefix.done
		  nop
		call	.L.output_char
		  mov	'0', c
		ld	[ %fp + 68 ], %o1 ! actually the radix 
		cmp	%o1, 16
		bne,a	.L.put_prefix.done
		  nop
		call	.L.output_char
		  mov	'x', c
.L.put_prefix.done:

!; post-sign padding (only happens if zero-filling, right-aligned) 
		call	.L.pad_left
		  mov	width, tmp
		mov	tmp, width

!; precision-based padding (only happens if zero-filling, right-aligned) 
		sub	prec, req_chars, tmp
		call	.L.pad_0s
		  mov	%o7, older_retaddr

!; output the value 
		ld	[ %fp + 68 ], flags ! actually the radix 
		call	.L._xitoa
		  ld	[ %fp + 72 ], c ! actually the data 

!; post-data padding (if left-aligned) 
.L.rpad.do:
		call	.L.pad_right
		  mov	width, tmp

  		b,a	.L.fmt_done					!}

!;=============================================================================

!;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
!; print char (%c) ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
!;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
.L.char_output:								!{

!; pre-char padding (only happens if right-aligned) 
		call	.L.pad_left
		  sub	width, 1, tmp
		mov	tmp, width

!; load data and output the value 
		ld	[p_crt_arg], c
		call	.L.output_char
		  and	c, 0xFF, c

		b	.L.rpad.do
		  inc	4, p_crt_arg					!}

!;=============================================================================

!;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
!; print string (%s) ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
!;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
.L.str_output:								!{

!; count string length 
		ld	[p_crt_arg], c
		clr	req_chars
.L.strlen:
		ldub	[c + req_chars], %g1
		tst	%g1
		bnz,a	.L.strlen
		  inc	req_chars

!; adjust prec so as not to exceed the string length 
		cmp	req_chars, prec
		skipgeu
		  mov	req_chars, prec
		
!; pre-string padding (only happens if right-aligned) 
		call	.L.pad_left
		  sub	width, prec, tmp
		mov	tmp, width

!; dump the first prec chars 
		ld	[p_crt_arg], flags
.L._puts:
		deccc	prec
		bcs,a	.L._puts.done
		  nop
		call	.L.output_char
		  ldub	[flags], c
		b	.L._puts
		  inc	flags
.L._puts.done:

		b	.L.rpad.do
		  inc	4, p_crt_arg					!}

!;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
!; print double, exponential notation (%e, %E) ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
!;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
!.L.efmt:								!{

!  		b,a	.L.fmt_done					!}

!;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
!; print double, fixed point notation (%f) ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
!;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
!.L.ffmt:								!{

!  		b,a	.L.fmt_done					!}

!;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
!; print double, smart notation (either %f or %e/%E) (%g, %G) ;;;;;;;;;;;;;;;;;
!;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
!.L.gfmt:								!{

 ! 		b,a	.L.fmt_done					!}

!;-----------------------------------------------------------------------------

.L.end:
!; _end:
!; restore the return address
		ld	[%fp + 76], caller_retaddr
!;   *dest = 0; 
!;   return nchars; 
		tst dest
		bnz,a .L.dest_is_NULL_last
		  clrb	[dest]
.L.dest_is_NULL_last:
		ret
		  restore nchars, 0, %o0

!;=============================================================================

		  .size	_xprintf, . - _xprintf
