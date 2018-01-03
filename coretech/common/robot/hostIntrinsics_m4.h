/**
File: hostIntrinsics_m4.h
Author: Peter Barnum
Created: 2014-05-22

This host intrinsics functions allow for compilation of ARM m4 intrinsics on any platform.
This set is incomplete, and there is no guarantee of correctness.

Copyright Anki, Inc. 2014
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#ifndef _ANKICORETECHEMBEDDED_COMMON_HOST_INTRINSICS_M4_H_
#define _ANKICORETECHEMBEDDED_COMMON_HOST_INTRINSICS_M4_H_

#include "coretech/common/robot/config.h"

#if !USE_M4_HOST_INTRINSICS

#ifdef USING_CHIP_SIMULATOR
#include <ARMCM4.h>
#else
#include <stm32f4xx.h>
#endif

#else

namespace Anki
{
  namespace Embedded
  {
    //
    // For more details, see http://www.keil.com/pack/doc/cmsis/Core/html/group__intrinsic___s_i_m_d__gr.html
    //

    //
    // Intrinsic functions for CPU instructions
    //

    //No Operation.
    //void __NOP();

    //Wait For Interrupt.
    //void __WFI();

    //Wait For Event.
    //void __WFE();

    //Send Event.
    //void __SEV();

    //Set Breakpoint.
    //void __BKPT(const u8 value);

    //Instruction Synchronization Barrier.
    //void __ISB();

    //Data Synchronization Barrier.
    //void __DSB();

    //Data Memory Barrier.
    //void __DMB();

    //Reverse byte order(32 bit)
    u32 __REV(const u32 value);

    //Reverse byte order(16 bit)
    u32 __REV16(const u32 value);

    //Reverse byte order in signed short value.
    s32 __REVSH(const s16 value);

    //Reverse bit order of value [not for Cortex-M0 variants].
    u32 __RBIT(const u32 value);

    //Rotate a value right by a number of bits.
    //u32 __ROR(const u32 value, const u32 shift);

    //LDR Exclusive(8 bit) [not for Cortex-M0 variants].
    //u8 __LDREXB(const volatile u8 *addr);

    //LDR Exclusive(16 bit) [not for Cortex-M0 variants].
    //u16 __LDREXH(const volatile u16 *addr);

    //LDR Exclusive(32 bit) [not for Cortex-M0 variants].
    //u32 __LDREXW(const volatile u32 *addr);

    //STR Exclusive(8 bit) [not for Cortex-M0 variants].
    //u32 __STREXB(const u8 value, const volatile u8 *addr);

    //STR Exclusive(16 bit) [not for Cortex-M0 variants].
    //u32 __STREXH(const u16 value, const volatile u16 *addr);

    //STR Exclusive(32 bit) [not for Cortex-M0 variants].
    //u32 __STREXW(const u32 value, const volatile u32 *addr);

    //Remove the exclusive lock [not for Cortex-M0 variants].
    //void __CLREX();

    //Signed Saturate [not for Cortex-M0 variants].
    s32 __SSAT(const s32 val, const u8 n);

    //Unsigned Saturate [not for Cortex-M0 variants].
    u32 __USAT(const s32 val, const u8 n);

    //Count leading zeros [not for Cortex-M0 variants].
    u8 __CLZ(const u32 value);

    //
    // Intrinsic functions for SIMD instructions
    //

    //GE setting quad 8-bit signed addition.
    u32 __SADD8(const u32 val1, const u32 val2);

    //Q setting quad 8-bit saturating addition.
    u32 __QADD8(const u32 val1, const u32 val2);

    //Quad 8-bit signed addition with halved results.
    u32 __SHADD8(const u32 val1, const u32 val2);

    //GE setting quad 8-bit unsigned addition.
    u32 __UADD8(const u32 val1, const u32 val2);

    //Quad 8-bit unsigned saturating addition.
    u32 __UQADD8(const u32 val1, const u32 val2);

    //Quad 8-bit unsigned addition with halved results.
    u32 __UHADD8(const u32 val1, const u32 val2);

    //GE setting quad 8-bit signed subtraction.
    u32 __SSUB8(const u32 val1, const u32 val2);

    //Q setting quad 8-bit saturating subtract.
    u32 __QSUB8(const u32 val1, const u32 val2);

    //Quad 8-bit signed subtraction with halved results.
    u32 __SHSUB8(const u32 val1, const u32 val2);

    //GE setting quad 8-bit unsigned subtract.
    u32 __USUB8(const u32 val1, const u32 val2);

    //Quad 8-bit unsigned saturating subtraction.
    u32 __UQSUB8(const u32 val1, const u32 val2);

    //Quad 8-bit unsigned subtraction with halved results.
    u32 __UHSUB8(const u32 val1, const u32 val2);

    //GE setting dual 16-bit signed addition.
    u32 __SADD16(const u32 val1, const u32 val2);

    //Q setting dual 16-bit saturating addition.
    u32 __QADD16(const u32 val1, const u32 val2);

    //Dual 16-bit signed addition with halved results.
    u32 __SHADD16(const u32 val1, const u32 val2);

    //GE setting dual 16-bit unsigned addition.
    u32 __UADD16(const u32 val1, const u32 val2);

    //Dual 16-bit unsigned saturating addition.
    u32 __UQADD16(const u32 val1, const u32 val2);

    //Dual 16-bit unsigned addition with halved results.
    u32 __UHADD16(const u32 val1, const u32 val2);

    //GE setting dual 16-bit signed subtraction.
    u32 __SSUB16(const u32 val1, const u32 val2);

    //Q setting dual 16-bit saturating subtract.
    u32 __QSUB16(const u32 val1, const u32 val2);

    //Dual 16-bit signed subtraction with halved results.
    //u32 __SHSUB16(const u32 val1, const u32 val2);

    //GE setting dual 16-bit unsigned subtract.
    u32 __USUB16(const u32 val1, const u32 val2);

    //Dual 16-bit unsigned saturating subtraction.
    u32 __UQSUB16(const u32 val1, const u32 val2);

    //Dual 16-bit unsigned subtraction with halved results.
    //u32 __UHSUB16(const u32 val1, const u32 val2);

    //GE setting dual 16-bit addition and subtraction with exchange.
    // TODO: verify implementation before using
    //u32 __SASX(const u32 val1, const u32 val2);

    //Q setting dual 16-bit add and subtract with exchange.
    //u32 __QASX(const u32 val1, const u32 val2);

    //Dual 16-bit signed addition and subtraction with halved results.
    //u32 __SHASX(const u32 val1, const u32 val2);

    //GE setting dual 16-bit unsigned addition and subtraction with exchange.
    // TODO: verify implementation before using
    //u32 __UASX(const u32 val1, const u32 val2);

    //Dual 16-bit unsigned saturating addition and subtraction with exchange.
    //u32 __UQASX(const u32 val1, const u32 val2);

    //Dual 16-bit unsigned addition and subtraction with halved results and exchange.
    //u32 __UHASX(const u32 val1, const u32 val2);

    //GE setting dual 16-bit signed subtraction and addition with exchange.
    // TODO: verify implementation before using
    //u32 __SSAX(const u32 val1, const u32 val2);

    //Q setting dual 16-bit subtract and add with exchange.
    //u32 __QSAX(const u32 val1, const u32 val2);

    //Dual 16-bit signed subtraction and addition with halved results.
    //u32 __SHSAX(const u32 val1, const u32 val2);

    //GE setting dual 16-bit unsigned subtract and add with exchange.
    // TODO: verify implementation before using
    //u32 __USAX(const u32 val1, const u32 val2);

    //Dual 16-bit unsigned saturating subtraction and addition with exchange.
    //u32 __UQSAX(const u32 val1, const u32 val2);

    //Dual 16-bit unsigned subtraction and addition with halved results and exchange.
    //u32 __UHSAX(const u32 val1, const u32 val2);

    //Unsigned sum of quad 8-bit unsigned absolute difference.
    // TODO: verify implementation before using
    //u32 __USAD8(const u32 val1, const u32 val2);

    //Unsigned sum of quad 8-bit unsigned absolute difference with 32-bit accumulate.
    //u32 __USADA8(const u32 val1, const u32 val2, const u32 val3);

    //Q setting dual 16-bit saturate.
    //u32 __SSAT16(const u32 val1, const u32 val2);

    //Q setting dual 16-bit unsigned saturate.
    //u32 __USAT16(const u32 val1, const u32 val2);

    //Dual extract 8-bits and zero-extend to 16-bits.
    //u32 __UXTB16(const u32 val);

    //Extracted 16-bit to 32-bit unsigned addition.
    //u32 __UXTAB16(const u32 val1, const u32 val2);

    //Dual extract 8-bits and sign extend each to 16-bits.
    //u32 __SXTB16(const u32 val);

    //Dual extracted 8-bit to 16-bit signed addition.
    //u32 __SXTAB16(const u32 val1, const u32 val2);

    //Q setting sum of dual 16-bit signed multiply.
    //u32 __SMUAD(const u32 val1, const u32 val2);

    //Q setting sum of dual 16-bit signed multiply with exchange.
    //u32 __SMUADX(const u32 val1, const u32 val2);

    //32-bit signed multiply with 32-bit truncated accumulator.
    //u32 __SMMLA(const s32 val1, s32 val2, s32 val3);

    //Q setting dual 16-bit signed multiply with single 32-bit accumulator.
    s32 __SMLAD(const u32 val1, const u32 val2, const s32 accumulator);

    //Q setting pre-exchanged dual 16-bit signed multiply with single 32-bit accumulator.
    //u32 __SMLADX(const u32 val1, const u32 val2, const u32 val3);

    //Dual 16-bit signed multiply with single 64-bit accumulator.
    //u64 __SMLALD(const u32 val1, const u32 val2, u64 val3);

    //Dual 16-bit signed multiply with exchange with single 64-bit accumulator.
    //u64 __SMLALDX(const u32 val1, const u32 val2, u64 val3);

    //Dual 16-bit signed multiply returning difference.
    //u32 __SMUSD(const u32 val1, const u32 val2);

    //Dual 16-bit signed multiply with exchange returning difference.
    //u32 __SMUSDX(const u32 val1, const u32 val2);

    //Q setting dual 16-bit signed multiply subtract with 32-bit accumulate.
    //u32 __SMLSD(const u32 val1, const u32 val2, const u32 val3);

    //Q setting dual 16-bit signed multiply with exchange subtract with 32-bit accumulate.
    //u32 __SMLSDX(const u32 val1, const u32 val2, const u32 val3);

    //Q setting dual 16-bit signed multiply subtract with 64-bit accumulate.
    //u64 __SMLSLD(const u32 val1, const u32 val2, u64 val3);

    //Q setting dual 16-bit signed multiply with exchange subtract with 64-bit accumulate.
    //u64 __SMLSLDX(const u32 val1, const u32 val2, u64 val3);

    //Select bytes based on GE bits.
    u32 __SEL(const u32 val1, const u32 val2);

    //Q setting saturating add.
    //u32 __QADD(const u32 val1, const u32 val2);

    //Q setting saturating subtract.
    //u32 __QSUB(const u32 val1, const u32 val2);

    //Halfword packing instruction. Combines bits[15:0] of val1 with bits[31:16] of val2 levitated with the val3.
    //u32 __PKHBT(const u32 val1, const u32 val2, const u32 val3);

    //Halfword packing instruction. Combines bits[31:16] of val1 with bits[15:0] of val2 right-shifted with the val3.
    //u32 __PKHTB(const u32 val1, const u32 val2, const u32 val3);
  } // namespace Embedded
} //namespace Anki

#endif // #if USE_M4_HOST_INTRINSICS

#endif // _ANKICORETECHEMBEDDED_COMMON_HOST_INTRINSICS_M4_H_
