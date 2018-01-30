/**
 ****************************************************************************************
 *
 * @file main.c
 *
 * @brief Secondary Bootloader application
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
#include <stdio.h>
#include "global_io.h"
#include "core_cm0.h"
#include "user_periph_setup.h"
#include "uart_booter.h"
#include "i2c_eeprom.h"
#include "bootloader.h"
#include "spi_commands.h"
#include "gpio.h"
#include "uart.h"
  
 void Start_run_user_application(void);
__asm void sw_reset(void);

/**
****************************************************************************************
* @brief sw reset 
****************************************************************************************
*/
__asm void sw_reset(void)
{
  LDR r0,=0x20000000
  LDR r1,[r0,#0]
  MOV sp,r1
  LDR r2,[r0,#4]
  BX r2
}

 /**
****************************************************************************************
* @brief Run the user application after receiving a binany from uart or reading the binary from a non volatile memory booting 
****************************************************************************************
*/
void Start_run_user_application(void)
{
    volatile unsigned short tmp;

#ifdef SPI_FLASH_SUPPORTED
    spi_release();

    // reset SPI GPIOs
    GPIO_ConfigurePin( SPI_GPIO_PORT, SPI_CS_PIN, INPUT_PULLUP, PID_GPIO, true );
    GPIO_ConfigurePin( SPI_GPIO_PORT, SPI_CLK_PIN, INPUT_PULLDOWN, PID_GPIO, false );
    GPIO_ConfigurePin( SPI_GPIO_PORT, SPI_DO_PIN, INPUT_PULLDOWN, PID_GPIO, false );
    GPIO_ConfigurePin( SPI_GPIO_PORT, SPI_DI_PIN, INPUT_PULLDOWN, PID_GPIO, false );
#endif

#ifdef EEPROM_FLASH_SUPPORTED
    i2c_eeprom_release();

    // reset I2C GPIOs
    GPIO_ConfigurePin(I2C_GPIO_PORT, I2C_SCL_PIN, INPUT_PULLDOWN, PID_GPIO, false);
    GPIO_ConfigurePin(I2C_GPIO_PORT, I2C_SDA_PIN, INPUT_PULLDOWN, PID_GPIO, false);
#endif

    if (*(volatile unsigned*)0x20000000 & 0x20000000)
    {
        tmp = GetWord16(SYS_CTRL_REG);
        tmp&=~0x0003;
        SetWord16(SYS_CTRL_REG,tmp);
        sw_reset();
    }
    tmp = GetWord16(SYS_CTRL_REG);
    //  tmp&=~0x0003;
    tmp|=0x8000;
    SetWord16(SYS_CTRL_REG,tmp);
}

extern int FwDownload(void);

int main (void)
{ 
#if defined(SPI_FLASH_SUPPORTED ) || defined(EEPROM_FLASH_SUPPORTED)
    int ret = -1;
#endif

#ifdef UART_SUPPORTED
    int fw_size;
    unsigned short i;
    char* from ; 
    char*  to;
#endif

    SetWord16(SET_FREEZE_REG,FRZ_WDOG);
    SetBits16(PMU_CTRL_REG, PERIPH_SLEEP,0);   // exit peripheral power down
    // Power up peripherals' power domain 
    while (!(GetWord16(SYS_STAT_REG) & PER_IS_UP));  

    while (1){

#ifdef UART_SUPPORTED
        if (GPIO_GetPinStatus(UART_GPIO_PORT, UART_RX_PIN)) {
            uart_initialization();                      // initialize UART and UART pins
            fw_size = FwDownload();                     // download FW
            uart_release();                             // release UART and reset UART pins
            if (fw_size>0){
                from=(char*) (SYSRAM_COPY_BASE_ADDRESS);  
                to=(char*) (SYSRAM_BASE_ADDRESS);         
                for(i=0;i<fw_size;i++)  to[i] = from[i]; 

                SetWord16(WATCHDOG_REG, 0xC8);          // 200 * 10.24ms active time for initialization!
                SetWord16(RESET_FREEZE_REG, FRZ_WDOG);  // Start WDOG
                Start_run_user_application();
            }
        }
#endif

#ifdef SPI_FLASH_SUPPORTED
        ret =spi_loadActiveImage();
        if (!ret) {
            SetWord16(WATCHDOG_REG, 0xC8);          // 200 * 10.24ms active time for initialization!
            SetWord16(RESET_FREEZE_REG, FRZ_WDOG);  // Start WDOG
            Start_run_user_application();
        }
#endif

#ifdef EEPROM_FLASH_SUPPORTED
        ret = eeprom_loadActiveImage();
        if (!ret) {
            SetWord16(WATCHDOG_REG, 0xC8);          // 200 * 10.24ms active time for initialization!
            SetWord16(RESET_FREEZE_REG, FRZ_WDOG);  // Start WDOG
            Start_run_user_application();
        }
#endif

    } // while (1)
} 

