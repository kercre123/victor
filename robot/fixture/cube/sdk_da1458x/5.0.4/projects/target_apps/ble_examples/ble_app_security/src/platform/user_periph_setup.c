/**
 ****************************************************************************************
 *
 * @file user_periph_setup.c
 *
 * @brief Peripherals setup and initialization.
 *
 * Copyright (C) 2015. Dialog Semiconductor Ltd, unpublished work. This computer
 * program includes Confidential, Proprietary Information and is a Trade Secret of
 * Dialog Semiconductor Ltd. All use, disclosure, and/or reproduction is prohibited
 * unless authorized in writing. All Rights Reserved.
 *
 * <bluetooth.support@diasemi.com> and contributors.
 *
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"             // SW configuration
#include "user_periph_setup.h"       // peripheral configuration
#include "global_io.h"
#include "gpio.h"
#include "uart.h"                    // UART initialization
#include "user_config.h"

#if DEVELOPMENT_DEBUG

void GPIO_reservations(void)
{
/*
 * Globally reserved GPIOs reservation
 */

/*
 * Application specific GPIOs reservation. Used only in Development mode (#if DEVELOPMENT_DEBUG)
 * i.e.
 *   RESERVE_GPIO(DESCRIPTIVE_NAME, GPIO_PORT_0, GPIO_PIN_1, PID_GPIO);    //Reserve P_01 as Generic Purpose I/O
 */

#ifdef CFG_PRINTF_UART2
    RESERVE_GPIO(UART2_TX, GPIO_UART2_TX_PORT, GPIO_UART2_TX_PIN, PID_UART2_TX);
    RESERVE_GPIO(UART2_RX, GPIO_UART2_RX_PORT, GPIO_UART2_RX_PIN, PID_UART2_RX);
#endif

#if defined (USER_CFG_APP_BOND_DB_USE_SPI_FLASH)
    // SPI FLASH
    #if !defined(__DA14583__)
    RESERVE_GPIO(SPI_EN,  SPI_EN_GPIO_PORT,  SPI_EN_GPIO_PIN,  PID_SPI_EN);
    RESERVE_GPIO(SPI_CLK, SPI_CLK_GPIO_PORT, SPI_CLK_GPIO_PIN, PID_SPI_CLK);
    RESERVE_GPIO(SPI_DO,  SPI_DO_GPIO_PORT,  SPI_DO_GPIO_PIN,  PID_SPI_DO);
    RESERVE_GPIO(SPI_DI,  SPI_DI_GPIO_PORT,  SPI_DI_GPIO_PIN,  PID_SPI_DI);
    #else
    // The DA14583 GPIOs that are dedicated to its internal SPI flash
    // are automatically reserved by the GPIO driver.
    #endif
#elif defined (USER_CFG_APP_BOND_DB_USE_I2C_EEPROM)
    // I2C EEPROM
    RESERVE_GPIO( I2C_SCL, I2C_GPIO_PORT, I2C_SCL_PIN, PID_I2C_SCL);
    RESERVE_GPIO( I2C_SDA, I2C_GPIO_PORT, I2C_SDA_PIN, PID_I2C_SDA);
#endif
}
#endif //DEVELOPMENT_DEBUG

void set_pad_functions(void)        // set gpio port function mode
{
#ifdef CFG_PRINTF_UART2
    GPIO_ConfigurePin(GPIO_UART2_TX_PORT, GPIO_UART2_TX_PIN, OUTPUT, PID_UART2_TX, false );
    GPIO_ConfigurePin(GPIO_UART2_RX_PORT, GPIO_UART2_RX_PIN, INPUT, PID_UART2_RX, false );
#endif

#if defined (USER_CFG_APP_BOND_DB_USE_SPI_FLASH)
    // SPI FLASH
    GPIO_ConfigurePin(SPI_EN_GPIO_PORT,  SPI_EN_GPIO_PIN,  OUTPUT, PID_SPI_EN,  true);
    GPIO_ConfigurePin(SPI_CLK_GPIO_PORT, SPI_CLK_GPIO_PIN, OUTPUT, PID_SPI_CLK, false);
    GPIO_ConfigurePin(SPI_DO_GPIO_PORT,  SPI_DO_GPIO_PIN,  OUTPUT, PID_SPI_DO,  false);
    GPIO_ConfigurePin(SPI_DI_GPIO_PORT,  SPI_DI_GPIO_PIN,  INPUT,  PID_SPI_DI,  false);
#elif defined (USER_CFG_APP_BOND_DB_USE_I2C_EEPROM)
    // I2C EEPROM
    GPIO_ConfigurePin(I2C_GPIO_PORT, I2C_SCL_PIN, INPUT, PID_I2C_SCL, false);
    GPIO_ConfigurePin(I2C_GPIO_PORT, I2C_SDA_PIN, INPUT, PID_I2C_SDA, false);
#endif

/*
 * Configure application ports.
 * i.e.
 *   GPIO_ConfigurePin( GPIO_PORT_0, GPIO_PIN_1, OUTPUT, PID_GPIO, false ); // Set P_01 as Generic purpose Output
 */
}

void periph_init(void)
{
    // Power up peripherals' power domain
    SetBits16(PMU_CTRL_REG, PERIPH_SLEEP, 0);
    while (!(GetWord16(SYS_STAT_REG) & PER_IS_UP));

    SetBits16(CLK_16M_REG, XTAL16_BIAS_SH_ENABLE, 1);

    // Rom patch
    patch_func();

    // Init pads
    set_pad_functions();

    // (Re)Initialize peripherals
    // i.e.
    // uart_init(UART_BAUDRATE_115K2, 3);

#ifdef CFG_PRINTF_UART2
    SetBits16(CLK_PER_REG, UART2_ENABLE, 1);
    uart2_init(UART_BAUDRATE_115K2, 3);
#endif

    // Enable the pads
    SetBits16(SYS_CTRL_REG, PAD_LATCH_EN, 1);
}
