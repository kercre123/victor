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
#include "user_periph_setup.h"            // periphera configuration
#include "global_io.h"
#include "gpio.h"
#include "spi_hci.h"                    // UART initialization

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

#if DEVELOPMENT_DEBUG

/**
 ****************************************************************************************
 * @brief GPIO_reservations. Globally reserved GPIOs
 *
 * @return void 
 ****************************************************************************************
*/

void GPIO_reservations(void)
{

/*
* Application specific GPIOs reservation
*/    
  
	RESERVE_GPIO( SPI_CLK, SPI_GPIO_PORT, SPI_CLK_PIN, PID_SPI_CLK);
	RESERVE_GPIO( SPI_EN, SPI_GPIO_PORT, SPI_CS_PIN, PID_SPI_EN);    
	RESERVE_GPIO( SPI_DO, SPI_GPIO_PORT, SPI_DO_PIN, PID_SPI_DO);  
	RESERVE_GPIO( SPI_DI, SPI_GPIO_PORT, SPI_DI_PIN, PID_SPI_DI); 
	RESERVE_GPIO( SPI_DREADY, SPI_GPIO_PORT, SPI_DREADY_PIN, PID_GPIO);
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
	GPIO_ConfigurePin( SPI_GPIO_PORT, SPI_CLK_PIN, INPUT_PULLUP,  PID_SPI_CLK, false);
	GPIO_ConfigurePin( SPI_GPIO_PORT, SPI_CS_PIN, INPUT_PULLDOWN,  PID_SPI_EN, false);
	GPIO_ConfigurePin( SPI_GPIO_PORT, SPI_DO_PIN, OUTPUT,  PID_SPI_DO, false); 
	GPIO_ConfigurePin( SPI_GPIO_PORT, SPI_DI_PIN, INPUT_PULLDOWN,  PID_SPI_DI, false);
	GPIO_ConfigurePin( SPI_GPIO_PORT, SPI_DREADY_PIN, OUTPUT, PID_GPIO, false);	
}


/**
 ****************************************************************************************
 * @brief Enable pad's and peripheral clocks assuming that peripherals' power domain is down.
 *
 * The Uart and SPi clocks are set. 
 ****************************************************************************************
 */
void periph_init(void)  // set i2c, spi, uart, uart2 serial clks
{
	// Power up peripherals' power domain
	SetBits16(PMU_CTRL_REG, PERIPH_SLEEP, 0);
	while (!(GetWord16(SYS_STAT_REG) & PER_IS_UP)) ; 
    

	SetBits16(CLK_16M_REG, XTAL16_BIAS_SH_ENABLE, 1);


	spi_hci_slave_init();
	
	//rom patch
	patch_func();
	
	//Init pads
	set_pad_functions();

	// Enable the pads
	SetBits16(SYS_CTRL_REG, PAD_LATCH_EN, 1);
}
