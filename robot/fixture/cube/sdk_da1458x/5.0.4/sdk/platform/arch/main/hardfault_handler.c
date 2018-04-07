/**
 ****************************************************************************************
 *
 * @file hardfault_handler.c
 *
 * @brief Hardfault handler source codoe file.
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

// HardFault_Handler() code added to boot_vectors.s

#include "global_io.h"
#include "ARMCM0.h"
#include "arch.h"
#include "uart.h"
#include "gpio.h"
#include "arch_wdg.h"

#define STATUS_BASE (0x81800)

#if PRODUCTION_DEBUG_OUTPUT

#if (DEVELOPMENT_DEBUG)
#error "DEVELOPMENT_DEBUG cannot be combined with PRODUCTION_DEBUG_OUTPUT"
#endif

static const char dbg_names[8][4] = { "r0 ", "r1 ", "r2 ", "r3 ", "r12", "lr ", "pc ", "psr" };

void dbg_prod_prepare(unsigned char *buf, int value, int index)
{
    int i;
    int nibble;
    
    buf[0] = dbg_names[index][0];
    buf[1] = dbg_names[index][1];
    buf[2] = dbg_names[index][2];
    buf[3] = '=';
    buf[4] = '0';
    buf[5] = 'x';
    
    for (i = 13; i >= 6; i--)
    {
        nibble = value & 0xF;
        if (nibble < 10)
            buf[i] = '0' + nibble;
        else
            buf[i] = 'a' + nibble - 10;
        value >>= 4;
    }
    buf[14] = '\r';
    buf[15] = '\n';
}

void dbg_prod_output(int mode, unsigned long *hardfault_args)
{
    unsigned char string[16];
    int i;
    
    // Switch to XTAL16 clock, if necessary
    if (GetBits16(CLK_CTRL_REG, RUNNING_AT_XTAL16M) == 0)
    {
        while ( !GetBits16(SYS_STAT_REG, XTAL16_SETTLED) ); // this takes some mili seconds

        SetBits16(CLK_CTRL_REG, SYS_CLK_SEL, 0);    // select XTAL 16MHz
    }
    
    // Configure UART
    GPIO_ConfigurePin(PRODUCTION_DEBUG_PORT, PRODUCTION_DEBUG_PIN, OUTPUT, PID_UART1_TX, false);
    SetBits16(CLK_PER_REG, UART1_ENABLE, 1);        // enable clock - always @16MHz
    uart_init(UART_BAUDRATE_115K2, 3);
    
    // Debug status - short messages only!
    if (mode)
        uart_write("\n--- HF STATUS\r\n", 16, NULL);
    else
        uart_write("\n--- NMI STATUS\r\n", 17, NULL);
    for (volatile int j = 0; j < 5000; j++);
    
    for (i = 0; i < 8; i++)
    {
        dbg_prod_prepare(string, hardfault_args[i], i);
        uart_write(string, 16, NULL);
        for (volatile int j = 0; j < 5000; j++);
    }
}
#endif

/*
 * This is the base address in Retention RAM where the stacked information will be copied.
 */


/*
 * Usage
 * -----
 *
 * Upon exception, all valuable (and available) information will be copied to the area
 * with base address equal to STATUS_BASE. The user can press the Reset button and
 * restart the debugger. Since the Retention RAM is not reset, the information will be
 * accessible via any Memory window.
 *
 * If SmartSnippets is used and P1[3] is available then the user may use the call
 * to set_pxact_gpio() to have a visual indication of the occurrence of a Hard Fault 
 * exception.
 */
 
void HardFault_HandlerC(unsigned long *hardfault_args)
{
    
    if (DEVELOPMENT_DEBUG)
    {
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
        if (USE_WDOG)
            wdg_freeze();            // Stop WDOG
        
        if ((GetWord16(SYS_STAT_REG) & DBG_IS_UP) == DBG_IS_UP)
            __asm("BKPT #0\n");
        else
            while(1);
    }
    else // DEVELOPMENT_DEBUG
    {
        if (PRODUCTION_DEBUG_OUTPUT)
        {
            if (USE_WDOG)        
                wdg_reload(WATCHDOG_DEFAULT_PERIOD);                  // Reset WDOG! 200 * 10.24ms active time for UART to finish printing!
        
            dbg_prod_output(1, hardfault_args);
        }

        
        // Force execution of NMI Handler
        wdg_reload(1);
        wdg_resume();
        while(1);
    }
    
}
