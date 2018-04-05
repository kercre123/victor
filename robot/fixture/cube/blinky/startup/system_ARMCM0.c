/**************************************************************************//**
 * @file     system_ARMCM0.c
 * @brief    CMSIS Device System Source File for
 *           ARMCM0 Device Series
 * @version  V2.00
 * @date     18. August 2015
 ******************************************************************************/
/* Copyright (c) 2011 - 2015 ARM LIMITED
   All rights reserved.
   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are met:
   - Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.
   - Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the following disclaimer in the
     documentation and/or other materials provided with the distribution.
   - Neither the name of ARM nor the names of its contributors may be used
     to endorse or promote products derived from this software without
     specific prior written permission.
   *
   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
   ARE DISCLAIMED. IN NO EVENT SHALL COPYRIGHT HOLDERS AND CONTRIBUTORS BE
   LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
   SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
   INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
   CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
   ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
   POSSIBILITY OF SUCH DAMAGE.
   ---------------------------------------------------------------------------*/


#include "ARMCM0.h"
#include <string.h>
 
/**
 * Initialize the system
 *
 * @param  none
 * @return none
 *
 * @brief  Setup the microcontroller system.
 *         Initialize the System.
 */
void SystemInit (void)
{
 
    
  SetWord16(TIMER0_CTRL_REG,0x6);      // stop timer
  NVIC_DisableIRQ(SWTIM_IRQn);     // disable software timer interrupt
    
  if ((GetWord16(CLK_CTRL_REG) & RUNNING_AT_XTAL16M) == 0)
  {
    while( (GetWord16(SYS_STAT_REG) & XTAL16_SETTLED) == 0 );     // wait for XTAL16 settle
    SetBits16(CLK_CTRL_REG , SYS_CLK_SEL ,0);                     // switch to XTAL16
    while( (GetWord16(CLK_CTRL_REG) & RUNNING_AT_XTAL16M) == 0 ); // wait for actual switch
  }
}
