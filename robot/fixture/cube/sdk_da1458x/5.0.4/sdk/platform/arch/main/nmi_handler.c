/**
 ****************************************************************************************
 *
 * @file nmi_handler.c
 *
 * @brief NMI hundler. 
 *
 * Copyright (C) 2012. Dialog Semiconductor Ltd, unpublished work. This computer 
 * program includes Confidential, Proprietary Information and is a Trade Secret of 
 * Dialog Semiconductor Ltd.  All use, disclosure, and/or reproduction is prohibited 
 * unless authorized in writing. All Rights Reserved.
 *
 * <bluetooth.support@diasemi.com> and contributors.
 *
 ****************************************************************************************
 */

#include "rwip_config.h"        // RW SW configuration
#include "global_io.h"
#include "ARMCM0.h"
#include "arch_wdg.h"
#include "arch.h"

/*
 * This is the base address in Retention RAM where the stacked information will be copied.
 */
#define STATUS_BASE (0x81850)

extern void dbg_prod_output(int mode, unsigned long *hardfault_args);

void NMI_HandlerC(unsigned long *hardfault_args)
{
    // Reached this point due to a WDOG timeout
    
    SetBits16(PMU_CTRL_REG, RADIO_SLEEP, 1);        // turn off radio PD
    SetBits16(PMU_CTRL_REG, PERIPH_SLEEP, 1);       // turn off peripheral power domain
    SetBits16(CLK_RADIO_REG, BLE_LP_RESET, 1);      // reset the BLE LP timer
    NVIC_ClearPendingIRQ(BLE_WAKEUP_LP_IRQn);       // clear any pending LP IRQs

    if (DEVELOPMENT_DEBUG)
    {
        wdg_freeze();            // Stop WDOG
        SetBits16(SYS_CTRL_REG, DEBUGGER_ENABLE, 1);    // enable debugger to be able to re-attach
        
        *(volatile unsigned long *)(STATUS_BASE       ) = hardfault_args[0];    // R0
        *(volatile unsigned long *)(STATUS_BASE + 0x04) = hardfault_args[1];    // R1
        *(volatile unsigned long *)(STATUS_BASE + 0x08) = hardfault_args[2];    // R2
        *(volatile unsigned long *)(STATUS_BASE + 0x0C) = hardfault_args[3];    // R3
        *(volatile unsigned long *)(STATUS_BASE + 0x10) = hardfault_args[4];    // R12
        *(volatile unsigned long *)(STATUS_BASE + 0x14) = hardfault_args[5];    // LR
        *(volatile unsigned long *)(STATUS_BASE + 0x18) = hardfault_args[6];    // PC
        *(volatile unsigned long *)(STATUS_BASE + 0x1C) = hardfault_args[7];    // PSR
        *(volatile unsigned long *)(STATUS_BASE + 0x20) = (unsigned long)hardfault_args;    // Stack Pointer

        *(volatile unsigned long *)(STATUS_BASE + 0x24) = (*((volatile unsigned long *)(0xE000ED28)));    // CFSR
        *(volatile unsigned long *)(STATUS_BASE + 0x28) = (*((volatile unsigned long *)(0xE000ED2C)));    // HFSR
        *(volatile unsigned long *)(STATUS_BASE + 0x2C) = (*((volatile unsigned long *)(0xE000ED30)));    // DFSR
        *(volatile unsigned long *)(STATUS_BASE + 0x30) = (*((volatile unsigned long *)(0xE000ED3C)));    // AFSR
        *(volatile unsigned long *)(STATUS_BASE + 0x34) = (*((volatile unsigned long *)(0xE000ED34)));    // MMAR
        *(volatile unsigned long *)(STATUS_BASE + 0x38) = (*((volatile unsigned long *)(0xE000ED38)));    // BFAR
        
        if ((GetWord16(SYS_STAT_REG) & DBG_IS_UP) == DBG_IS_UP)
            __asm("BKPT #0\n");
        else
        {
            while(1);
        }
    }
    else // DEVELOPMENT_DEBUG
    {
        if (PRODUCTION_DEBUG_OUTPUT)
        {
            // Power up peripherals' power domain
            SetBits16(PMU_CTRL_REG, PERIPH_SLEEP, 0);
            while (!(GetWord16(SYS_STAT_REG) & PER_IS_UP));
            
            dbg_prod_output(0, hardfault_args);
        }
    }
    
    // Remap addres 0x00 to ROM and force execution
    SetWord16(SYS_CTRL_REG, (GetWord16(SYS_CTRL_REG) & ~REMAP_ADR0) | SW_RESET );

}
