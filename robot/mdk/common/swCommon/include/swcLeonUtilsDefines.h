///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved
///            For License Warranty see: common/license.txt
///
/// @brief     API manipulating Leon functionalities
///
/// Register defines for swcLeonUtils
///


#ifndef SWCLEONUTILSDEFINES_H_
#define SWCLEONUTILSDEFINES_H_

//==============================================================================
//== Processor State Register (PSR) ============================================
//==============================================================================

#define MASK_PSR_impl   0xf0000000  // SPARC implementation: 0x0F
#define POS_PSR_impl    28
#define MASK_PSR_ver    0x0f000000  // SPARC implementation vs: 0x03
#define POS_PSR_ver 24

#define MASK_PSR_icc    0x00f00000  // integer condition codes mask
#define POS_PSR_icc 20
#define PSR_N       0x00800000  // icc: negative
#define PSR_Z       0x00400000  // icc: zero
#define PSR_V       0x00200000  // icc: overflow
#define PSR_C       0x00100000  // icc: carry

#define PSR_EC      0x00002000  // enable coprocessor
#define PSR_EF      0x00001000  // enable FPU

#define MASK_PSR_PIL    0x00000f00  // processor interrupt level mask
#define POS_PSR_PIL 8
#define PSR_PIL0    0x00000000
#define PSR_PIL1    0x00000100
#define PSR_PIL2    0x00000200
#define PSR_PIL3    0x00000300
#define PSR_PIL4    0x00000400
#define PSR_PIL5    0x00000500
#define PSR_PIL6    0x00000600
#define PSR_PIL7    0x00000700
#define PSR_PIL8    0x00000800
#define PSR_PIL9    0x00000900
#define PSR_PIL10   0x00000a00
#define PSR_PIL11   0x00000b00
#define PSR_PIL12   0x00000c00
#define PSR_PIL13   0x00000d00
#define PSR_PIL14   0x00000e00
#define PSR_PIL15   0x00000f00

#define PSR_S       0x00000080  // supervisor
#define PSR_PS      0x00000040  // previous supervisor
#define PSR_ET      0x00000020  // enable traps

#define MASK_PSR_CWP    0x0000001f  // current window pointer mask
#define POS_PSR_CWP 0
#define PSR_CWP0    0x00000000
#define PSR_CWP1    0x00000001
#define PSR_CWP2    0x00000002
#define PSR_CWP3    0x00000003
#define PSR_CWP4    0x00000004
#define PSR_CWP5    0x00000005
#define PSR_CWP6    0x00000006
#define PSR_CWP7    0x00000007

//==============================================================================
//== Window Invalid Mask =======================================================
//==============================================================================

#define MASK_WIM_BITS   0x000000ff
#define WIM_INVD0   0x00000001
#define WIM_INVD1   0x00000002
#define WIM_INVD2   0x00000004
#define WIM_INVD3   0x00000008
#define WIM_INVD4   0x00000010
#define WIM_INVD5   0x00000020
#define WIM_INVD6   0x00000040
#define WIM_INVD7   0x00000080

//==============================================================================
//== Trap Base Register ========================================================
//==============================================================================

#define MASK_TBR_tba    0xfffff000
#define POS_TBR_tba 12
#define MASK_TBR_tt 0x00000ff0
#define POS_TBR_tt  4

#define TBR_tt_reset                    0x000
#define TBR_tt_instr_access_exception   0x010
#define TBR_tt_illegal_instr            0x020
#define TBR_tt_privileged_instr         0x030
#define TBR_tt_fp_disabled              0x040
#define TBR_tt_window_overflow          0x050
#define TBR_tt_window_underflow         0x060
#define TBR_tt_mem_address_not_aligned  0x070
#define TBR_tt_fp_exception             0x080
#define TBR_tt_data_access_exception    0x090
#define TBR_tt_tag_overflow     0x0A0
#define TBR_tt_watchpoint       0x0B0
#define TBR_tt_IRQ1             0x110
#define TBR_tt_IRQ2             0x120
#define TBR_tt_IRQ3             0x130
#define TBR_tt_IRQ4             0x140
#define TBR_tt_IRQ5             0x150
#define TBR_tt_IRQ6             0x160
#define TBR_tt_IRQ7             0x170
#define TBR_tt_IRQ8             0x180
#define TBR_tt_IRQ9             0x190
#define TBR_tt_IRQ10            0x1A0
#define TBR_tt_IRQ11            0x1B0
#define TBR_tt_IRQ12            0x1C0
#define TBR_tt_IRQ13            0x1D0
#define TBR_tt_IRQ14            0x1E0
#define TBR_tt_IRQ15            0x1F0
#define TBR_tt_r_register_access_error  0x200
#define TBR_tt_instr_access_error   0x210
#define TBR_tt_cp_disabled      0x240
#define TBR_tt_unimplemented_FLUSH  0x250
#define TBR_tt_cp_exception     0x280
#define TBR_tt_data_access_error    0x290
#define TBR_tt_division_by_0        0x2A0
#define TBR_tt_data_store_error     0x2B0
#define TBR_tt_data_access_MMU_miss 0x2C0
#define TBR_tt_instr_access_MMU_miss    0x3C0

#define TBR_tt_user_trap_0  0x800
#define TBR_tt_user_trap_127    0xFF0

//==============================================================================
//== FPU State Register ========================================================
//==============================================================================

#define MASK_FSR_RD 0xC0000000  // rounding direction mask
#define POS_FSR_RD  30
#define FSR_RD_NEAREST  0x00000000  // round towards nearest
#define FSR_RD_ZERO 0x40000000  // round towards zero
#define FSR_RD_INF  0x80000000  // round towards +Inf
#define FSR_RD_NINF 0xC0000000  // round towared -Inf

#define MASK_FSR_TEM    0x0f800000  // trap enable mask
#define POS_FSR_TEM 25
#define FSR_NVM     0x08000000  // (NV) invalid ops (0/0, Inf-Inf)
#define FSR_OFM     0x04000000  // (OF) overflow (|res|>MaxRepr)
#define FSR_UFM     0x02000000  // (UF) underflow (|res|<MinRepr)
#define FSR_DZM     0x01000000  // (DZ) divide by 0 ( x/0 )
#define FSR_NXM     0x00800000  // (NX) inexact result

#define FSR_NS  0x00400000      // nonstandard mode (denormals=0)

#define MASK_FSR_ver    0x000E0000  // FPU version: 0x02
#define POS_FSR_ver 17

#define MASK_FSR_tt 0x0001C000  // Trap type
#define POS_FSR_rrm 14
#define FSR_tt_NONE 0x00000000  // (0x0) no trap
#define FSR_tt_IEEE 0x00004000  // (0x1) IEEE754 exception
#define FSR_tt_UNF  0x00008000  // (0x2) unfinised FPop
#define FSR_tt_SEQUENCE 0x00010000  // (0x4) sequence error

#define FSR_QNE     0x00002000  // deferred trap queue not empty

#define MASK_FSR_fcc    0x00000C00  // Compare results
#define POS_FSR_fcc 10
#define FSR_EQ      0x00000000  // (EQ) equal
#define FSR_LT      0x00000400  // (LT) less than
#define FSR_GT      0x00000800  // (GT) greater than
#define FSR_UNORDERED   0x00000C00  // (UO) unordered (one is NaN)

#define MASK_FSR_AEXC   0x000003E0  // Accrued exceptions
#define POS_FSR_AEXC    5
#define FSR_NVA     0x00000200
#define FSR_OFA     0x00000100
#define FSR_UFA     0x00000080
#define FSR_DFA     0x00000040
#define FSR_NXA     0x00000020

#define MASK_FSR_CEXC   0x0000001F  // Current exceptions
#define POS_FSR_CEXC    0
#define FSR_NVC     0x00000010
#define FSR_OFC     0x00000008
#define FSR_UFC     0x00000004
#define FSR_DFC     0x00000002
#define FSR_NXC     0x00000001

//==============================================================================
//== Hardware Breakpoint Registers =============================================
//==============================================================================

#define MASK_HBRK_ADDR  0xC0000000  // mask for BP address / BP mask

//==============================================================================
//== Processor Configuration Register (ASR17) ==================================
//==============================================================================

#define LEON_PROCESSOR_INDEX_MASK          ( 1 << 28    )  // 0 => LEON_OS ; 1 => LEON_RT    
#define ASR17_DWT                          ( 0x00004000 )  // Write Error Trap
#define ASR17_SVT                          ( 0x00002000 )  // Single Vector Trapping

//==============================================================================
//== Cache Control Register ====================================================
//==============================================================================

#define __CCR_ASI   0x02
#define __CCR_OFS   0x00000000

#define CACHE_CONTROL_REG_OFS   (0x00000000)
#define ICACHE_CONFIG_REG_OFS   (0x00000008)
#define DCACHE_CONFIG_REG_OFS   (0x0000000C)

#define CCR_FI      (1<<21)             // command: trigger icache flush
#define CCR_FD      (1<<22)             // command: trigger dcache flush

#define POS_CCR_IP  15
#define CCR_IP      (1<<POS_CCR_IP)     // status: icache flush pending
#define POS_CCR_DP  14
#define CCR_DP      (1<<POS_CCR_DP)     // status: dcache flush pending

#define CCR_DS      (1<<23)             // config: data cache snoop
#define CCR_DF      (1<<5)              // config: dcache freeze-on-interrupt
#define CCR_IF      (1<<4)              // config: icache freeze-on-interrupt

#define MASK_CCR_DCS     (3<<2)
#define CCR_DCS_ENABLED  (3<<2)         // config: dcache enabled
#define CCR_DCS_FROZEN   (1<<2)         // config: dcache frozen
#define CCR_DCS_DISABLED (0<<2)         // config: dcache disabled

#define MASK_CCR_ICS     (3<<0)
#define CCR_ICS_ENABLED  (3<<0)         // config: icache enabled
#define CCR_ICS_FROZEN   (1<<0)         // config: icache frozen
#define CCR_ICS_DISABLED (0<<0)         // config: icache disabled

// Note: The IB bit no longer exists for Myriad2 (Leon4)            
#define CCR_IB      (1<<16)             // config: icache burst fetch
            
/////////////////////////////////////////////////////////////////////
// The following are changes to the CCR register in Leon4 (Myriad2)
////////////////////////////////////////////////////////////////////
// New bits but not used in Myriad2
// DDE  7.. 6  (Parity error in data of data cache)
// DTE  9.. 8  (Parity error in tags of data cache)
// DDE 11..10  (Parity error in data of instruction cache)
// DTE 13..12  (Parity error in tags of instruction cache)
// Bits 17 (Snoop Tags), 16 (Icache burst fetch) no longer exist
// FT  20..19  (Data protection scheme 00-none 01-byte parity checks) [Only 00 supported]
// 27..24      (Test bits, not used set to 0)
// 28          (PS -- Partity select, not used set to 0)
////////////////////////////////////////////////////////////////////
                                    
#define __NOCACHE_ASI       0x01
#define __ICACHE_TAGS_ASI   0x0C
#define __ICACHE_DATA_ASI   0x0D
#define __DCACHE_TAGS_ASI   0x0E
#define __DCACHE_DATA_ASI   0x0F

// #define __ICACHE_FLUSH_ASI  0x15 // this is incorrect in the documentation
// #define __DCACHE_FLUSH_ASI  0x16 // this is incorrect in the documentation
#define __ICACHE_FLUSH_ASI_DO_NOT_USE  0x10 // this also triggers MMU page flush
#define __DCACHE_FLUSH_ASI  0x11

#define _ASM __asm__ __volatile__
#define NOP  _ASM("nop;":::"memory")

#endif /* SWCLEONUTILSDEFINES_H_ */
