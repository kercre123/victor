#include <rtems/asm.h>
#include <bspopts.h>
#include <swcLeonUtilsDefines.h>

!; Note: Leon L1 ICache/DCache is enabled by default
!; This default can be overridden from the Makefile
!;

		!; processor startup
		.section ".sys.text.start", "ax", @progbits
		.type	start, #function
		.global start, __bsp_mem_init
start:
/*
 *  Unexpected trap will halt the processor by forcing it to error state
 */

#define BAD_TRAP \
	ta 0; \
	nop; \
	nop; \
	nop;

/*
 *  Software trap. Treat as BAD_TRAP for the time being...
 */

#define SOFT_TRAP BAD_TRAP

/*
 *  The trap table has to be the first code in a boot PROM.  But because
 *  the Memory Configuration comes up thinking we only have 4K of PROM, we
 *  cannot have a full trap table and still have room left over to
 *  reprogram the Memory Configuration register correctly.  This file
 *  uses an abbreviated trap which has every entry which might be used
 *  before RTEMS installs its own trap table.
 */

        PUBLIC(trap_table)
SYM(trap_table):

	RTRAP( 0, SYM(hard_reset) );                  ! 00 reset trap
	BAD_TRAP;                                     ! 01 instruction access
                             	                      !    exception
	BAD_TRAP;                                     ! 02 illegal instruction
	BAD_TRAP;                                     ! 03 privileged instruction
	BAD_TRAP;                                     ! 04 fp disabled
	TRAP( 5, SYM(window_overflow_trap_handler) ); ! 05 window overflow
	TRAP( 6, SYM(window_underflow_trap_handler) );! 06 window underflow
;!	TRAP( 5, SYM(traphandler)); ! 05 window overflow
;!	TRAP( 6, SYM(traphandler));! 06 window underflow
	BAD_TRAP;                                     ! 07 memory address not aligned
	BAD_TRAP;                                     ! 08 fp exception
	BAD_TRAP;                                     ! 09 data access exception
	BAD_TRAP;                                     ! 0A tag overflow
	BAD_TRAP;                                     ! 0B undefined
	BAD_TRAP;                                     ! 0C undefined
	BAD_TRAP;                                     ! 0D undefined
	BAD_TRAP;                                     ! 0E undefined
	BAD_TRAP;                                     ! 0F undefined
	BAD_TRAP;                                     ! 10 undefined
	BAD_TRAP;                                     ! 11 masked errors
	BAD_TRAP;                                     ! 12 external 1
	BAD_TRAP;                                     ! 13 external 2
	BAD_TRAP;                                     ! 14 UART A RX/TX
	BAD_TRAP;                                     ! 15 UART B RX/TX
	BAD_TRAP;                                     ! 16 correctable memory error
	BAD_TRAP;                                     ! 17 UART error
	BAD_TRAP;                                     ! 18 DMA access error
	BAD_TRAP;                                     ! 19 DMA timeout
	BAD_TRAP;                                     ! 1A external 3
	BAD_TRAP;                                     ! 1B external 4
	BAD_TRAP;                                     ! 1C general purpose timer
	BAD_TRAP;                                     ! 1D real time clock
	BAD_TRAP;                                     ! 1E external 5
	BAD_TRAP;                                     ! 1F watchdog timeout

	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP;       ! 20 - 23 undefined
	BAD_TRAP;                                     ! 24 cp_disabled
	BAD_TRAP; BAD_TRAP; BAD_TRAP;       	      ! 25 - 27 undefined
	BAD_TRAP;                                     ! 28 cp_exception
	BAD_TRAP; BAD_TRAP; BAD_TRAP;		      ! 29 - 2B undefined
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP;       ! 2C - 2F undefined
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP;       ! 30 - 33 undefined
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP;       ! 34 - 37 undefined
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP;       ! 38 - 3B undefined
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP;       ! 3C - 3F undefined
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP;       ! 40 - 43 undefined
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP;       ! 44 - 47 undefined
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP;       ! 48 - 4B undefined
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP;       ! 4C - 4F undefined
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP;       ! 50 - 53 undefined
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP;       ! 54 - 57 undefined
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP;       ! 58 - 5B undefined
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP;       ! 5C - 5F undefined
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP;       ! 60 - 63 undefined
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP;       ! 64 - 67 undefined
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP;       ! 68 - 6B undefined
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP;       ! 6C - 6F undefined
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP;       ! 70 - 73 undefined
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP;       ! 74 - 77 undefined
	BAD_TRAP; BAD_TRAP; BAD_TRAP; BAD_TRAP;       ! 78 - 7B undefined

/*
   This is a sad patch to make sure that we know where the
   MEC timer control register mirror is so we can stop the timers
   from an external debugger. It is needed because the control
   register is write-only. Trap 0x7C cannot occure in ERC32...

   We also use this location to store the last location of the
   usable RAM in order not to overwrite the remote debugger with
   the RTEMS work-space area.

*/

	.global SYM(_ERC32_MEC_Timer_Control_Mirror), SYM(rdb_start), SYM(CLOCK_SPEED)
	.global SYM(Configuration)

SYM(rdb_start):
SYM(_ERC32_MEC_Timer_Control_Mirror):
	BAD_TRAP; BAD_TRAP;                           ! 7C - 7D undefined

SYM(CLOCK_SPEED):
	.word	0x0C, 0, 0, 0				! 7E (12 MHz default)
	BAD_TRAP; 					! 7F undefined

  /*
   *  Software traps
   *
   *  NOTE: At the risk of being redundant... this is not a full
   *        table.  The setjmp on the SPARC requires a window flush trap
   *        handler and RTEMS will preserve the entries that were
   *        installed before.
   */

	TRAP( 0x80, SYM(syscall) ); 			! 80 syscall SW trap
	SOFT_TRAP; SOFT_TRAP;  	            	! 81 - 82
	TRAP( 0x83, SYM(window_flush_trap_handler) ); ! 83 flush windows SW trap

	SOFT_TRAP; SOFT_TRAP; SOFT_TRAP; SOFT_TRAP;   ! 84 - 87
	SOFT_TRAP; SOFT_TRAP; SOFT_TRAP; SOFT_TRAP;   ! 88 - 8B
	SOFT_TRAP; SOFT_TRAP; SOFT_TRAP; SOFT_TRAP;   ! 8C - 8F
	SOFT_TRAP; SOFT_TRAP; SOFT_TRAP; SOFT_TRAP;   ! 90 - 93
	SOFT_TRAP; SOFT_TRAP; SOFT_TRAP; SOFT_TRAP;   ! 94 - 97
	SOFT_TRAP; SOFT_TRAP; SOFT_TRAP; SOFT_TRAP;   ! 98 - 9B
	SOFT_TRAP; SOFT_TRAP; SOFT_TRAP; SOFT_TRAP;   ! 9C - 9F
	SOFT_TRAP; SOFT_TRAP; SOFT_TRAP; SOFT_TRAP;   ! A0 - A3
	SOFT_TRAP; SOFT_TRAP; SOFT_TRAP; SOFT_TRAP;   ! A4 - A7
	SOFT_TRAP; SOFT_TRAP; SOFT_TRAP; SOFT_TRAP;   ! A8 - AB
	SOFT_TRAP; SOFT_TRAP; SOFT_TRAP; SOFT_TRAP;   ! AC - AF
	SOFT_TRAP; SOFT_TRAP; SOFT_TRAP; SOFT_TRAP;   ! B0 - B3
	SOFT_TRAP; SOFT_TRAP; SOFT_TRAP; SOFT_TRAP;   ! B4 - B7
	SOFT_TRAP; SOFT_TRAP; SOFT_TRAP; SOFT_TRAP;   ! B8 - BB
	SOFT_TRAP; SOFT_TRAP; SOFT_TRAP; SOFT_TRAP;   ! BC - BF
	SOFT_TRAP; SOFT_TRAP; SOFT_TRAP; SOFT_TRAP;   ! C0 - C3
	SOFT_TRAP; SOFT_TRAP; SOFT_TRAP; SOFT_TRAP;   ! C4 - C7
	SOFT_TRAP; SOFT_TRAP; SOFT_TRAP; SOFT_TRAP;   ! C8 - CB
	SOFT_TRAP; SOFT_TRAP; SOFT_TRAP; SOFT_TRAP;   ! CC - CF
	SOFT_TRAP; SOFT_TRAP; SOFT_TRAP; SOFT_TRAP;   ! D0 - D3
	SOFT_TRAP; SOFT_TRAP; SOFT_TRAP; SOFT_TRAP;   ! D4 - D7
	SOFT_TRAP; SOFT_TRAP; SOFT_TRAP; SOFT_TRAP;   ! D8 - DB
	SOFT_TRAP; SOFT_TRAP; SOFT_TRAP; SOFT_TRAP;   ! DC - DF
	SOFT_TRAP; SOFT_TRAP; SOFT_TRAP; SOFT_TRAP;   ! E0 - E3
	SOFT_TRAP; SOFT_TRAP; SOFT_TRAP; SOFT_TRAP;   ! E4 - E7
	SOFT_TRAP; SOFT_TRAP; SOFT_TRAP; SOFT_TRAP;   ! E8 - EB
	SOFT_TRAP; SOFT_TRAP; SOFT_TRAP; SOFT_TRAP;   ! EC - EF
	SOFT_TRAP; SOFT_TRAP; SOFT_TRAP; SOFT_TRAP;   ! F0 - F3
	SOFT_TRAP; SOFT_TRAP; SOFT_TRAP; SOFT_TRAP;   ! F4 - F7
	SOFT_TRAP; SOFT_TRAP; SOFT_TRAP; SOFT_TRAP;   ! F8 - FB
	SOFT_TRAP; SOFT_TRAP; SOFT_TRAP; SOFT_TRAP;   ! FC - FF

/*
 *  This is the hard reset code.
 */

#define PSR_INIT   0x10c0       /* Disable traps, set s and ps */
#define WIM_INIT   2
#define STACK_SIZE 8 * 1024

       PUBLIC(hard_reset)
SYM(hard_reset):

/* Common initialisation */

	set     SYM(trap_table), %g1    ! Initialize TBR
	mov     %g1, %tbr

	set	__sys_bss_start, %sp  ! TODO: Review this

	mov	%psr, %g1         	! Initialize WIM
	add	%g1, 1, %g2
	and	%g2, 0x7, %g2
	set	1, %g3
	sll	%g3, %g2, %g3
	mov     %g3, %wim


	set	(SYM(rdb_start)), %g6	! End of RAM
	st	%sp, [%g6]
	sub	%sp, 4, %sp		! stack starts at end of RAM - 4
	andn	%sp, 0x0f, %sp		! align stack on 16-byte boundary
	mov     %sp, %fp                ! Set frame pointer
	nop



!; trigger caches flush
		  flush
#ifndef DISABLE_LEON_FPU
!; prepare PSR and traps config
		set	PSR_EF|PSR_PIL15|PSR_S|PSR_CWP7, %g1
#else
		set	PSR_PIL15|PSR_S|PSR_CWP7, %g1
#endif
!; setup PSR
		wr	%g1, %psr

!; configure caches
		set	CCR_IB, %g2
#ifndef DISABLE_LEON_ICACHE
	   or %g2,CCR_ICS_ENABLED,%g2
!;           nop
#else 
!; Nop keeps our instuction addressing the same in either case
	   nop  		
#endif
#ifndef DISABLE_LEON_DCACHE
	   or %g2,CCR_DCS_ENABLED,%g2
!;          nop
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

!; setup stack and call boot_card
		call	boot_card
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


       PUBLIC(BSP_fatal_return)
SYM(BSP_fatal_return):
	mov	1, %g1
        ta      0                       ! Halt if _main returns ...
        nop

!;-----------------------------------------------------------------------------
!;- __fsr_default -------------------------------------------------------------
!;-----------------------------------------------------------------------------
#define	FSR_NF	(1<<22)
		.section ".sys.rodata.4", "a", @progbits
		.global __fsr_default
		.type __fsr_default, #object
__fsr_default: .word FSR_RD_ZERO | FSR_NF
		.size __fsr_default, 4

