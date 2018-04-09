/**
 ****************************************************************************************
 *
 * @file peripherals.c
 *
 * @brief Peripherals initialization fucntions
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
#include "global_io.h"
#include "uart.h"
#include <core_cm0.h>
#include "peripherals.h"
#include "user_periph_setup.h"
#include "i2c_eeprom.h"

_uart_sel_pins uart_sel_pins;
_spi_sel_pins spi_sel_pins;
_i2c_sel_pins i2c_sel_pins;
void init_uart_pads(void);
void init_spi_pads(void);
void init_i2c_pads(void);
  
 /**
 ****************************************************************************************
 * @brief Enable pad's and peripheral clocks assuming that peripherals' power domain is down.
 *        The Uart and SPi clocks are set. 
 * 
 ****************************************************************************************
 */
void periph_init(void)  // set i2c, spi, uart, uart2 serial clks
{
    // system init
    SetWord16(CLK_AMBA_REG, 0x00);              	// set clocks (hclk and pclk ) 16MHz
    SetWord16(SET_FREEZE_REG, FRZ_WDOG);         	// stop watch dog
    SetBits16(SYS_CTRL_REG, PAD_LATCH_EN, 1);     // open pads
    SetBits16(SYS_CTRL_REG, DEBUGGER_ENABLE, 1);  // open debugger
    SetBits16(PMU_CTRL_REG, PERIPH_SLEEP, 0);    	// exit peripheral power down
    // Power up peripherals' power domain
    SetBits16(PMU_CTRL_REG, PERIPH_SLEEP, 0);
    while (!(GetWord16(SYS_STAT_REG) & PER_IS_UP));

    // Init pads
		init_uart_pads();
		init_spi_pads();
		init_i2c_pads();
}

/**
 ****************************************************************************************
 * @brief set gpio port function mode for the UART interface
 * 
 ****************************************************************************************
 */
void init_uart_pads(void)
{
	memset(&uart_sel_pins, 0, sizeof(uart_sel_pins));
	uart_sel_pins.uart_port_tx = UART_GPIO_PORT;
	uart_sel_pins.uart_tx_pin = UART_TX_PIN;
    uart_sel_pins.uart_port_rx = UART_GPIO_PORT;
	uart_sel_pins.uart_rx_pin = UART_RX_PIN;
}

void set_pad_uart(void)
{
    GPIO_ConfigurePin( (GPIO_PORT) uart_sel_pins.uart_port_tx, (GPIO_PIN) uart_sel_pins.uart_tx_pin, OUTPUT, PID_UART1_TX, false);
    GPIO_ConfigurePin( (GPIO_PORT) uart_sel_pins.uart_port_rx, (GPIO_PIN) uart_sel_pins.uart_rx_pin, INPUT, PID_UART1_RX, false);
}

void update_uart_pads(GPIO_PORT port, GPIO_PIN tx_pin, GPIO_PIN rx_pin)
{
    uart_sel_pins.uart_port_tx = port;
    uart_sel_pins.uart_tx_pin = tx_pin;
    uart_sel_pins.uart_port_rx = port;
    uart_sel_pins.uart_rx_pin = rx_pin;
}

/**
 ****************************************************************************************
 * @brief set gpio port function mode for SPI interface
 * 
 ****************************************************************************************
 */
void init_spi_pads(void)
{
	memset(&spi_sel_pins, 0, sizeof(_spi_sel_pins));
	spi_sel_pins.spi_cs_port = SPI_CS_PORT;
	spi_sel_pins.spi_cs_pin = SPI_CS_PIN;
	spi_sel_pins.spi_clk_port = SPI_CLK_PORT;
	spi_sel_pins.spi_clk_pin = SPI_CLK_PIN;
	spi_sel_pins.spi_do_port = SPI_DO_PORT;
	spi_sel_pins.spi_do_pin = SPI_DO_PIN;
	spi_sel_pins.spi_di_port = SPI_DI_PORT;
	spi_sel_pins.spi_di_pin = SPI_DI_PIN;
}

void set_pad_spi(void)     
{
    GPIO_ConfigurePin( (GPIO_PORT) spi_sel_pins.spi_cs_port, (GPIO_PIN) spi_sel_pins.spi_cs_pin, OUTPUT, PID_SPI_EN, true );
    GPIO_ConfigurePin( (GPIO_PORT) spi_sel_pins.spi_clk_port, (GPIO_PIN) spi_sel_pins.spi_clk_pin, OUTPUT, PID_SPI_CLK, false );
    GPIO_ConfigurePin( (GPIO_PORT) spi_sel_pins.spi_do_port, (GPIO_PIN) spi_sel_pins.spi_do_pin, OUTPUT, PID_SPI_DO, false );	
    GPIO_ConfigurePin( (GPIO_PORT) spi_sel_pins.spi_di_port, (GPIO_PIN) spi_sel_pins.spi_di_pin, INPUT, PID_SPI_DI, false );
} 

/**
 ****************************************************************************************
 * @brief set gpio port function mode for EEPROM interface
 * 
 ****************************************************************************************
 */
void init_i2c_pads(void)
{
	memset(&i2c_sel_pins, 0, sizeof(_i2c_sel_pins));
	i2c_sel_pins.i2c_scl_port = I2C_SCL_PORT;
	i2c_sel_pins.i2c_scl_pin = I2C_SCL_PIN;
	i2c_sel_pins.i2c_sda_port = I2C_SDA_PORT;
	i2c_sel_pins.i2c_sda_pin = I2C_SDA_PIN;
}

void set_pad_eeprom(void)       
{
    GPIO_ConfigurePin( (GPIO_PORT) i2c_sel_pins.i2c_scl_port, (GPIO_PIN) i2c_sel_pins.i2c_scl_pin, INPUT, PID_I2C_SCL, false);
    GPIO_ConfigurePin( (GPIO_PORT) i2c_sel_pins.i2c_sda_port, (GPIO_PIN) i2c_sel_pins.i2c_sda_pin, INPUT, PID_I2C_SDA, false);
} 
