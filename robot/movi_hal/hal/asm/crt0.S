#include <swcLeonUtilsDefines.h>
!; Note: Leon L1 ICache/DCache is enabled by default
!; This default can be overridden from the Makefile
!;

		!; processor startup
		.section ".sys.text.start", "ax", @progbits
		.type	start, #function
		.global start
start:
!; prepare WIM for setting up PSR
		wr	%g0, %wim
!; trigger caches flush
		  flush
#ifndef DISABLE_LEON_FPU
!; prepare PSR and traps config
		set	PSR_EF|PSR_PIL15|PSR_S|PSR_CWP7, %g1
#else
		set	PSR_PIL15|PSR_S|PSR_CWP7, %g1
#endif
		sethi	%hi( traphandler ), %g2
!; setup PSR
		wr	%g1, %psr
!; setup traps
		wr	%g2, %tbr
		set	ASR17_SVT|ASR17_DWT, %g3

!; initialize WIM properly (we're in window 7, so invalidate window 0)
		wr	%g0, WIM_INVD0, %wim
		nop

!; enable single vector trapping, disable write_error traps
		wr	%g3, %asr17
!; configure caches
		set	CCR_IB, %g2
#ifndef DISABLE_LEON_ICACHE
	   or %g2,CCR_ICS_ENABLED,%g2
#else 
!; Nop keeps our instuction addressing the same in either case
	   nop  		
#endif
#ifndef DISABLE_LEON_DCACHE
	   or %g2,CCR_DCS_ENABLED,%g2
#else 
!; Nop keeps our instuction addressing the same in either case
	   nop  		
#endif
!; prepare & optionally enable cache
		sta	%g2, [ %g0 ] __CCR_ASI

!; clear the application's .bss - also aligned to 8, size a multiple of 8
#ifndef DONT_CLEAR_BSS
		set	__bss_start, %g2
		set	__bss_end, %g3
		clr     %g1
.L.clear_bss:
		cmp	%g2, %g3
		inc	8, %g2
		blu,a	.L.clear_bss
		  std	%g0, [%g2 - 8]
#endif

#ifndef DONT_CLEAR_BSS
!; clear the system .system.bss - note: aligned to 8, goes all the way to RAM's top
		set	__sys_bss_end  , %sp
		set	__sys_bss_start, %g3
.L.clear_system_bss:
		dec	8, %sp
		cmp	%sp, %g3
		bgu	.L.clear_system_bss
		  std	%g0, [ %sp ]
!; place the stack just below the system bss
#else
		set	( __sys_bss_start ), %sp
#endif

!; enable traps
		rd	%psr, %g1
		wr	%g1, PSR_ET, %psr
		nop; nop
		clr	%g1
 
#ifndef DISABLE_LEON_FPU
!; use FPU inexact mode
        sethi %hi(__fsr_default) , %g2
		ld	[%g2 + %lo( __fsr_default )], %fsr
		nop; nop
#endif

!; setup stack and call main
    set 0x90107f00, %sp
    set 0x90107f00, %fp
    
		call	main
		  dec	64, %sp

!; "return" 0 when main returns
		.size	start, . - start

!;-----------------------------------------------------------------------------

		.global exit
		.type	exit, #function
exit:
!; clear ET
		rd      %psr, %g1
		bclr    PSR_ET, %g1
		wr      %g1, %psr
!; In VCS we want to exit the simulation 
!; First flush any debug messages by writing 0 to the debug_msg port
		  set	0x90007FF8, %g2 !; this is two delay instructions
		  st	%g0,[%g2]
!; Then signal simulation termination by writing 0xFFFFFFFF to the last ROM word      
		set	0x90007FFC, %g2
		set	0xFFFFFFFF, %g1
		st	%g1,[%g2]
!; Set %g1 to 1 to signal the debugger that this was a normal clean exit		
!; force error_mode with %tbr.tt=0x80 and %g1==0x1, and the exit code in %o0
		mov	1,%g1
		t	0
		.size	exit, . - exit

		.set  __mp_sabre, 1
		.global	__mp_sabre

#ifndef DISABLE_LEON_CACHE
		.set  __cache_on, 1
		.global	__cache_on
#else
		.set  __cache_off, 1
		.global	__cache_off
#endif

#ifndef DISABLE_LEON_FPU
		.set  __fpu_on, 1
		.global	__fpu_on
#else
		.set  __fpu_off, 1
		.global	__fpu_off
#endif

