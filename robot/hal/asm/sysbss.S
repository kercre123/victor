#include <DrvIcbDefines.h>

!;-----------------------------------------------------------------------------
!;- __fsr_default -------------------------------------------------------------
!;-----------------------------------------------------------------------------
#define	FSR_NS	0x00400000
#define	FSR_RD_ZERO	(1<<30)
#define	FSR_NF	(1<<22)
		.section ".sys.rodata.4", "", @progbits
		.global __fsr_default
		.type __fsr_default, #object

!; The Movidius default floating point processing is to round to zero and no-fault mode (no-fault mode is for denormals)
!; __fsr_default: .word FSR_RD_ZERO | FSR_NF

!; Putting the floating point rounding in standard IEEE754 compliant mode (same as Visual Studio)
__fsr_default: .word FSR_NF
		.size __fsr_default, 4

!;-----------------------------------------------------------------------------
!;- void (*irq_table)()[__N_IRQS__] -------------------------------------------
!;-----------------------------------------------------------------------------

		.section ".sys.bss.4", "a", @nobits
		.type	__irq_table, #object
		.global __irq_table
		.align	4
__irq_table:
		.skip	4 * __N_IRQS__
		.size	__irq_table, 4 * __N_IRQS__
