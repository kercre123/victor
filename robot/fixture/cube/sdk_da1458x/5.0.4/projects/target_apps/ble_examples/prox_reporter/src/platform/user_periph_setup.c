/**
 ****************************************************************************************
 *
 * @file user_periph_setup.c
 *
 * @brief Peripherals setup and initialization.
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
/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "rwip_config.h"             // SW configuration
#include "user_periph_setup.h"       // peripheral configuration
#include "global_io.h"
#include "gpio.h"
#include "uart.h"                    // UART initialization

#include "app_proxr.h"
#include "user_proxr.h"

#if BLE_BAS_SERVER
#include "app_bass.h"
#endif

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

#if DEVELOPMENT_DEBUG

void GPIO_reservations(void)
{

/*
* Application specific GPIOs reservation
*/
#if BLE_PROX_REPORTER
    #if USE_PUSH_BUTTON
    RESERVE_GPIO( PUSH_BUTTON, GPIO_BUTTON_PORT, GPIO_BUTTON_PIN, PID_GPIO);
    #endif // USE_PUSH_BUTTON
    RESERVE_GPIO( GREEN_LED, GPIO_ALERT_LED_PORT, GPIO_ALERT_LED_PIN, PID_GPIO);
#endif
#if (BLE_BAS_SERVER && USE_BAT_LEVEL_ALERT)
    //Setup LED GPIO for battery alert
    RESERVE_GPIO( RED_LED, GPIO_BAT_LED_PORT, GPIO_BAT_LED_PIN, PID_GPIO);
#endif

#if (BLE_SPOTA_RECEIVER)

    #if !defined(__DA14583__)
    RESERVE_GPIO(SPI_EN,  SPI_EN_GPIO_PORT,  SPI_EN_GPIO_PIN,  PID_SPI_EN);
    RESERVE_GPIO(SPI_CLK, SPI_CLK_GPIO_PORT, SPI_CLK_GPIO_PIN, PID_SPI_CLK);
    RESERVE_GPIO(SPI_DO,  SPI_DO_GPIO_PORT,  SPI_DO_GPIO_PIN,  PID_SPI_DO);
    RESERVE_GPIO(SPI_DI,  SPI_DI_GPIO_PORT,  SPI_DI_GPIO_PIN,  PID_SPI_DI);
    #else
        // The DA14583 GPIOs that are dedicated to its internal SPI flash
        // are automaticaly reserved by the GPIO driver.
    #endif

    // Example GPIO reservations for an I2C EEPROM.
    //RESERVE_GPIO( I2C_SCL, GPIO_PORT_0, GPIO_PIN_2, PID_I2C_SCL);
    //RESERVE_GPIO( I2C_SDA, GPIO_PORT_0, GPIO_PIN_3, PID_I2C_SDA);
#endif

}
#endif

/**
 ****************************************************************************************
 * @brief Map port pins
 *
 * The Uart and SPI port pins and GPIO ports(for debugging) are mapped
 ****************************************************************************************
 */
void set_pad_functions(void)        // set gpio port function mode
{

#if BLE_PROX_REPORTER
    #if USE_PUSH_BUTTON
    GPIO_ConfigurePin(GPIO_BUTTON_PORT, GPIO_BUTTON_PIN, INPUT_PULLUP, PID_GPIO, false); // Push Button
    #endif // USE_PUSH_BUTTON
    GPIO_ConfigurePin(GPIO_ALERT_LED_PORT, GPIO_ALERT_LED_PIN, OUTPUT, PID_GPIO, false); //Alert LED
#endif
#if (BLE_BAS_SERVER && USE_BAT_LEVEL_ALERT)
    GPIO_ConfigurePin(GPIO_BAT_LED_PORT, GPIO_BAT_LED_PIN, OUTPUT, PID_GPIO, false); //Battery alert LED
#endif

#if (BLE_SPOTA_RECEIVER)
    GPIO_ConfigurePin(SPI_EN_GPIO_PORT,  SPI_EN_GPIO_PIN,  OUTPUT, PID_SPI_EN,  true);
    GPIO_ConfigurePin(SPI_CLK_GPIO_PORT, SPI_CLK_GPIO_PIN, OUTPUT, PID_SPI_CLK, false);
    GPIO_ConfigurePin(SPI_DO_GPIO_PORT,  SPI_DO_GPIO_PIN,  OUTPUT, PID_SPI_DO,  false);
    GPIO_ConfigurePin(SPI_DI_GPIO_PORT,  SPI_DI_GPIO_PIN,  INPUT,  PID_SPI_DI,  false);

    // Example GPIO configuration for an I2C EEPROM.
    //GPIO_ConfigurePin(GPIO_PORT_0, GPIO_PIN_2, INPUT, PID_I2C_SCL, false);
    //GPIO_ConfigurePin(GPIO_PORT_0, GPIO_PIN_3, INPUT, PID_I2C_SDA, false);
#endif
}

void periph_init(void)  // set i2c, spi, uart, uart2 serial clks
{
    // Power up peripherals' power domain
    SetBits16(PMU_CTRL_REG, PERIPH_SLEEP, 0);
    while (!(GetWord16(SYS_STAT_REG) & PER_IS_UP)) ;

    SetBits16(CLK_16M_REG, XTAL16_BIAS_SH_ENABLE, 1);

    //rom patch
    patch_func();

    //Init pads
    set_pad_functions();

#if BLE_PROX_REPORTER
    app_proxr_port_reinit(GPIO_ALERT_LED_PORT, GPIO_ALERT_LED_PIN);
    #if USE_PUSH_BUTTON
    app_button_enable();
    #endif // USE_PUSH_BUTTON
#endif //BLE_PROX_REPORTER

#if BLE_BAS_SERVER
    app_batt_port_reinit();
#endif //BLE_BAS_SERVER

    // Enable the pads
    SetBits16(SYS_CTRL_REG, PAD_LATCH_EN, 1);
}
