		#include <swcLeonUtilsDefines.h>
		#include <isaac_registers.h>
		#include <DrvIcbDefines.h>

		!; trap handling (single vector trapping) 
		.section ".sys.text.traps", "ax", @progbits
		.type	traphandler, #function
		.global traphandler
		.align	4
traphandler:
#define ret_pc %l1
#define ret_npc %l2
		!; read all the system stuff we might care about 
		rd	%tbr, %g5
		rd      %wim, %g6
#define in_wim %g6
		!; extract trap ID in %l0
		and	%g5, MASK_TBR_tt, %l0
#define trap_type %l0
		rd	%psr, %g7
#define in_psr %g7
		mov     %g1, %l5
#define in_g1 %l5

!;----------------------------------------------------------------------------
!;- Register window overflow -------------------------------------------------
!;----------------------------------------------------------------------------

		!; trap overflow: use the ROM handler
		cmp	trap_type, TBR_tt_window_overflow
		bne,a	.L.is_underflow
		 cmp	trap_type, TBR_tt_window_underflow

__T_window_overflow:
		.type   __T_window_overflow, #function
		sethi   %hi( .L.save_regs ), %g1
		jmpl    %g1 + %lo( .L.save_regs ), %g1
		 wr    %g0, %wim
.L.return.reexecute:
		!; restore PSR (CWP/icc) 
		!; note, this WILL crash if we're not in trap handler's window 
		wr	in_psr, %psr
		 mov   in_g1, %g1
		 nop
		!; return, reexecuting the failed instruction 
		 jmp	ret_pc
		rett	ret_npc

!;~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

		!; save the next window on the stack 
		!; return address: %g1
		!; destroys: %g5; updates: %g6
.L.save_regs:
		!; rotate WIM to the right (assume %wim has just been cleared) 
		 srl	in_wim, 1, %g5
		 sll	in_wim, 7, in_wim
		 bset  %g5, in_wim
		!; get into the window to save (1 below) 
		save
		wr      in_wim, %wim
		!; save its contents 
		std	%l0, [ %sp + 0x00 ]
		std	%l2, [ %sp + 0x08 ]
		std	%l4, [ %sp + 0x10 ]
		std	%l6, [ %sp + 0x18 ]
		std	%i0, [ %sp + 0x20 ]
		std	%i2, [ %sp + 0x28 ]
		std	%i4, [ %sp + 0x30 ]
		std	%i6, [ %sp + 0x38 ]
		!; return to, aaa, the caller 
		jmp     %g1 + 8
		 restore

!;----------------------------------------------------------------------------
!;- Register window underflow ------------------------------------------------
!;----------------------------------------------------------------------------

		!; trap underflow: use the ROM handler 
.L.is_underflow:
!               cmp     trap_type, TBR_tt_window_underflow
		bne,a	.L.is_IRQ
		 cmp	trap_type, TBR_tt_IRQ1

__T_window_underflow:
		.type   __T_window_underflow, #function
		sethi   %hi( .L.restore_regs ), %g1
		jmpl    %g1 + %lo( .L.restore_regs ), %g1
		 wr    %g0, %wim
		b       .L.return.reexecute
		 save
		
!;~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

		!; restore current registers window from stack 
		!; return address: %g1 
.L.restore_regs:
		 sll	in_wim, 1, %g5
		 srl	in_wim, 7, in_wim
		 bset	%g5, in_wim
		!; get into the window to restore (2 above) 
		restore
		restore
		wr    	in_wim, %wim
		!; restore its contents  
		ldd	[ %sp + 0x00 ], %l0
		ldd	[ %sp + 0x08 ], %l2
		ldd	[ %sp + 0x10 ], %l4
		ldd	[ %sp + 0x18 ], %l6
		ldd	[ %sp + 0x20 ], %i0
		ldd	[ %sp + 0x28 ], %i2
		ldd	[ %sp + 0x30 ], %i4
		ldd	[ %sp + 0x38 ], %i6
		!; return to, aaa, the caller (perform only ONE save, TWO are needed)
		jmp     %g1 + 8 
		 save
		
!;----------------------------------------------------------------------------
!;- Non-IRQ trap with tt < 0x11: error! --------------------------------------
!;- Non-IRQ trap with tt > 0x1f: error! --------------------------------------
!;----------------------------------------------------------------------------

		! code < IRQ1: generate software breakpoint 
.L.is_IRQ:
!               cmp     trap_type, TBR_tt_IRQ1
		blu	__T_app_stop
		 cmp	trap_type, TBR_tt_IRQ15
		bgu	__T_app_stop
!                nop ! no need

!;----------------------------------------------------------------------------
!;- Interrupt requests -------------------------------------------------------
!;----------------------------------------------------------------------------

__T_IRQ:
		.type   __T_IRQ, #function
		!; read ICB source register 
		sethi	%hi(ICB_BASE_ADR), %l0
		ld	[ %l0 + %lo(ICB_SRC_ADR) ], %l3 ! %l3 = ICB.SRC
		!; get out if we can't tell which it was (level-triggered IRQ
		!; which somehow got cleared already)
		cmp     %l3, __N_IRQS__
		bgeu    .L.return.reexecute
		!; disable the interrupt
		mov     1, %l4
		sll     %l4, %l3, %g1 !; %g1 = 1 <<( ICB.SRC & 0x1F )
		srl     %l3, 5, %l6
		sll     %l6, 2, %l6 !; %l6 =( ICB.SRC / 32 )* 4 = (ICB.SRC>=32)?4:0;
		add     %l0, %l6, %l0 
		!; clear the pending flag
		st      %g1, [ %l0 + %lo(ICB_CLEAR_0_ADR) ] !; ICB.CLEAR[n] = 1 <<( ICB.SRC & 0x1F )
		!; read the handler pointer 
		sll	%l3, 2, %l6 !; %l3 = ICB.SRC << 2
		set	__irq_table, %l7
		ld	[ %l7 + %l6 ], %l6
		!; bail out if we don't have a handler 
		tst	%l6
		bz,a	__T_IRQ_no_handler
		  ld      [ %l0 + %lo(ICB_ENABLE_0_ADR) ], %l6

__T_IRQ_have_handler:
		!; see if we're in the invalid window 
		sll	%l4, %g7, %g1 ! l5 = 1<<psr.cwp
		cmp	%g1, in_wim
		!; and backup input PSR to local register %l7
		bne	__T_IRQ_crt_window_valid
		  mov   in_psr, %l7

__T_IRQ_crt_window_invalid:
		!; we are: we need to put the NEXT window on the stack 
		!;   so 
		!; perform the overflow logic here 
		sethi %hi( .L.save_regs ), %g1
		jmpl    %g1 + %lo( .L.save_regs ), %g1
		  wr    %g0, %wim
		!; at this stage, %g5 was destroyed, so re-read it
__T_IRQ_crt_window_valid:
		!; setup this window's stack 
		add	%fp, -64, %sp
		!; backup %y 
		rd      %y, %l0
#ifdef USE_INTERRUPT_PIL
!; set PIL to interrupt's IL 
		!; note, %g5 MIGHT have been destroyed by now - so re-read TBR
		rd	%tbr, %g5
		!; shift/mask the IL in TBR 
		and     %g5, 0xF0, %g5
		!; prepare PSR ( PIL<-IRQ.IL, ET=0 ) 
		andn    %l7, MASK_PSR_PIL, %g7
		sll     %g5, 4, %g5
		bset    %g5, %g7
#else
!; set PIL to max (0xF)
		!; prepare PSR ( PIL<-0xF, ET=0 ) 
		or      %l7, MASK_PSR_PIL, %g7
#endif
__T_IRQ_call_handler:
		!; setup the handler's argument (ICB.SRC)
		mov   	%l3, %o0
		!; write PSR ( new PIL, ET=0 ) 
		wr	%g7, %psr
		!; write PSR ( new PIL, ET=1 ) 
		wr	%g7, PSR_ET, %psr
		mov	%g2, %l3 !; backup %g2
		mov	%g3, %l4 !; backup %g3
		!; call the handler - (*__irq_table[ICB.SRC])(ICB.SRC)
		call	%l6
		 mov	%g4, %l6 !; backup %g4
		
__T_IRQ_done:
		!; reset %psr to what it was initially in the trap handler 
		!; (we're only doing this to disable traps)
		wr	%l7, %psr !; doesn't change CWP; ET = 0, PIL = original PIL
		 mov	%l6, %g4 !; restore %g4
		 add   	%l7, 1, %g5 !; %g5&0x1f := psr.cwp+1
		 mov	%l4, %g3 !; restore %g3

		mov	1, %g7 !; %g7 = 1
		and     %g5, 0x7, %g5 !; %g5 = (1+psr.cwp)%8
		mov	%l3, %g2 !; restore %g2
		rd    	%wim, in_wim !; re-read %wim - could have changed in ISR
		sll	%g7, %g5, %g7 !; %g7 = 1<<((psr.cwp+1)%8)
		wr    	%l0, %y !; restore %y
		cmp	%g7, in_wim
		!; and restore backup of input PSR from local register %l7
		bne	.L.return.reexecute
		  mov   %l7, in_psr

__T_IRQ_prev_window_invalid:
		sethi   %hi( .L.return.reexecute - 8 ), %g1
		!; we are: we need to get the PREVIOUS window from the stack
		!;   SO
		!; --cwp (we're fine, the cwp+1 one is invalid)
		!; (we need this because the restore_regs code does two restore's)
		save
		!; restore the window we're returning to
		bset    %lo( .L.return.reexecute - 8 ), %g1
		b       .L.restore_regs
		  wr    %g0, %wim

__T_IRQ_no_handler:
		!; disable the interrupt source 
		bclr    %g1, %l6
		st      %l6, [ %l0 + %lo(ICB_ENABLE_0_ADR) ]
		!; return, clearing the pending flag (again, just in case it's level triggered) 
		b       .L.return.reexecute
		  st    %g1, [ %l0 + %lo(ICB_CLEAR_0_ADR) ]

!;----------------------------------------------------------------------------
!;- if we got here, it wasn't a window_* trap, and it wasn't an IRQ => error -
!;----------------------------------------------------------------------------

__T_app_stop:
		! save the return addresses & get back to the trapped window
		mov     ret_pc, %g5
		restore ret_npc, 0, %g6
		! reexecute the failing instruction, in its window, but with
		! ET = 0 => we get error mode on the exact cause of failure
		jmp     %g5
		 jmp    %g6
		.size	traphandler, . - traphandler

!;============================================================================

		.section ".text.swcLeonSetPIL", "ax", @progbits
		.type   swcLeonSetPIL, #function
		.global swcLeonSetPIL
swcLeonSetPIL:
		!; input is in %o0, output in %o0 
		!; extract current PIL / form new PSR
		rd      %psr, %o1
		sll     %o0, POS_PSR_PIL, %o2 ! %o2 = new_pil<<PIL_POS
		and     %o1, MASK_PSR_PIL, %o0 ! %o0 = psr&PIL_MASK
		bclr    MASK_PSR_PIL, %o1 ! %o1 = psr&~PIL_MASK
		and     %o2, MASK_PSR_PIL, %o2 ! %o3 = (new_pil<<PIL_POS)&PIL_MASK
		wr      %o2, %o1, %psr
		 srl    %o0, POS_PSR_PIL, %o0 ! return the old PIL
		 retl
		  nop
		.size   swcLeonSetPIL, . - swcLeonSetPIL

!;============================================================================

		.section ".text.swcLeonDisableTraps", "ax", @progbits
		.type	swcLeonDisableTraps, #function
		.global swcLeonDisableTraps
swcLeonDisableTraps:
		rd      %psr, %o0
		andn    %o0, PSR_ET, %o1
		wr      %o1, %psr
		 and    %o0, PSR_ET, %o0
		 retl
		  srl   %o0, 5, %o0 ! return the old ET
		.size   swcLeonDisableTraps, . - swcLeonDisableTraps

!;============================================================================

		.section ".text.swcLeonEnableTraps", "ax", @progbits
		.type	swcLeonEnableTraps, #function
		.global swcLeonEnableTraps
swcLeonEnableTraps:
		rd      %psr, %o0
		or      %o0, PSR_ET, %o1
		wr      %o1, %psr
		 and    %o0, PSR_ET, %o0
		 retl
		  srl   %o0, 5, %o0 ! return the old ET
		.size   swcLeonEnableTraps, . - swcLeonEnableTraps

!;============================================================================

		.section ".text.swcLeonFlushWindows", "ax", @progbits
		.type	swcLeonFlushWindows, #function
		.global swcLeonFlushWindows
swcLeonFlushWindows:
		! disable IRQs / enable traps
		rd      %psr, %g2
		or      %g2, MASK_PSR_PIL|PSR_ET, %g3
		wr      %g3, %psr
		 mov    %o7, %g4
		 and    %g2, MASK_PSR_CWP, %o0 ! return the CWP
		 nop
		save    %g0, 0, %sp
		save    %g0, 0, %sp
		save    %g0, 0, %sp
		save    %g0, 0, %sp
		save    %g0, 0, %sp
		save    %g0, 0, %sp
		wr      %g2, %psr
		 nop
		 jmp    %g4 + 8 ! retl %g4 ;)
		  nop
		.size   swcLeonFlushWindows, . - swcLeonFlushWindows
