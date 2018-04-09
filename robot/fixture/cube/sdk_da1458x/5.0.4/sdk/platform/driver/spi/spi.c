/**
 ****************************************************************************************
 *
 * @file spi.c
 *
 * @brief spi interface driver. V2_0
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

#include <stdint.h>
#include "global_io.h"
#include "spi.h"
#include <core_cm0.h>
#include "datasheet.h"


SPI_Pad_t spi_driver_cs_pad;	                                    // CS pad selection


/**
 ****************************************************************************************
  @brief SPI spi_cs_low, Start SPI transaction
  
 ****************************************************************************************
 */
void spi_cs_low(void)
{
    GPIO_SetInactive(spi_driver_cs_pad.port, spi_driver_cs_pad.pin);	// pull CS low   
}


/**
 ****************************************************************************************
  @brief SPI spi_cs_low, finish SPI transaction
  ****************************************************************************************
 */
void spi_cs_high(void)
{
	GPIO_SetActive(spi_driver_cs_pad.port, spi_driver_cs_pad.pin);		// push CS high
}



/**
 ****************************************************************************************
 * @brief SPI initialization
 *
 * @param[in] cs_pad_param: port/pin to be assigned to the SPI CS signal
 * @param[in] bitmode:   SPI_MODE_8BIT,	SPI_MODE_16BIT,	SPI_MODE_32BIT,	SPI_MODE_9BIT
 * @param[in] role:		 SPI_ROLE_MASTER, SPI_ROLE_SLAVE
 * @param[in] clk_pol:   SPI_CLK_IDLE_POL_LOW, SPI_CLK_IDLE_POL_HIGH
 * @param[in] pha_mode:  SPI_PHA_MODE_0, SPI_PHA_MODE_1 : spi sampling edge selection (refer to datasheet p.53-54)
 * @param[in] irq:		 SPI_MINT_DISABLE: disable SPI_INT_BIT to NVIC, SPI_MINT_ENABLE: enable SPI_INT_BIT to NVIC. Note that the SPI_INT interrupt is shared with AD_INT interrupt
 * @param[in] freq:		 SPI_XTAL_DIV_8, SPI_XTAL_DIV_4, SPI_XTAL_DIV_2, SPI_XTAL_DIV_14 - Select SPI_CLK clock frequency in master mode: (XTAL)/ (CLK_PER_REG * DIV) where div: 8,4,2,14
 *
 ****************************************************************************************
 */
void spi_init(SPI_Pad_t *cs_pad_param, SPI_Word_Mode_t bitmode, SPI_Role_t role, SPI_Polarity_Mode_t clk_pol, SPI_PHA_Mode_t pha_mode, SPI_MINT_Mode_t irq, SPI_XTAL_Freq_t freq)
{
	 
	//app_force_active_mode();					// block sleep

	// initialize cs structure with parameters data;
	spi_driver_cs_pad.port = cs_pad_param->port;
	spi_driver_cs_pad.pin = cs_pad_param->pin;
    
    spi_cs_high();  
	 
 	// enable clock for SPI
	SetBits16(CLK_PER_REG, SPI_ENABLE, 1);     

	// initialize SPI
	SetBits16(SPI_CTRL_REG, SPI_ON, 0);         // close SPI block, if opened
	SetBits16(SPI_CTRL_REG, SPI_WORD, bitmode); // set SPI bitmode
	SetBits16(SPI_CTRL_REG, SPI_SMN, role);	    // select role (master/slave)
	SetBits16(SPI_CTRL_REG, SPI_POL, clk_pol);  // select SPI clock idle polarity
	SetBits16(SPI_CTRL_REG, SPI_PHA, pha_mode);	// select SPI sampling edge selection (pha_mode - refer to datasheet p.53-54)
	SetBits16(SPI_CTRL_REG, SPI_MINT, irq);     // enable/disable SPI interrupt to the NVIC
	SetBits16(SPI_CTRL_REG, SPI_CLK, freq);     // SPI block clock divider
	SetBits16(SPI_CTRL_REG, SPI_ON, 1);         // enable SPI block
}


/**
 ****************************************************************************************
 * @brief selects the SPI bitmode
 * @param[in] spiBitMode: the desired SPI bitmode
 ****************************************************************************************
 */
void spi_set_bitmode(SPI_Word_Mode_t spiBitMode)
{
    SetBits16(SPI_CTRL_REG, SPI_ON, 0);             // close SPI block, if opened
    SetBits16(SPI_CTRL_REG, SPI_WORD, spiBitMode);  // set SPI bitmode
    SetBits16(SPI_CTRL_REG, SPI_ON, 1);             // open SPI block
}

/**
 ****************************************************************************************
 * @brief releases SPI
 ****************************************************************************************
 */
void spi_release(void)
{
	SetBits16(SPI_CTRL_REG, SPI_ON, 0);			// switch off SPI block
	SetBits16(CLK_PER_REG, SPI_ENABLE, 0);		// SPI clock
	GPIO_SetActive(spi_driver_cs_pad.port, spi_driver_cs_pad.pin);            // leave CS high        
}


/**
 ****************************************************************************************
 * @brief disables SPI block
 ****************************************************************************************
 */
void spi_disable(void)
{
	SetBits16(SPI_CTRL_REG, SPI_ON, 0);			// switch off SPI block
}
 

/**
****************************************************************************************
* @brief SPI access to slave without acting on CS (works in all bit modes)
* @param[in] dataToSend: data to send
* @return  data read 
****************************************************************************************
*/
uint32_t spi_access(uint32_t dataToSend)
{
	uint32_t dataRead = 0;
    uint16_t bitmode = 0;
    
 	bitmode = GetBits16(SPI_CTRL_REG, SPI_WORD);			// get SPI word mode

	if (bitmode > SPI_MODE_16BIT)							// check if bitmode is set in 9-bit or 32-bit
	{
		SetWord16(SPI_RX_TX_REG1, (uint16_t)(dataToSend >> 16));  // write high part of dataToSend
	}

	SetWord16(SPI_RX_TX_REG0, (uint16_t)dataToSend);		// write (low part of) dataToSend

 	do
	{
	} while (GetBits16(SPI_CTRL_REG, SPI_INT_BIT) == 0);	// polling to wait for spi transmission
    
	SetWord16(SPI_CLEAR_INT_REG, 0x01);						// clear pending flag
    
  // Read from Registers
	if (bitmode > SPI_MODE_16BIT)							// check if bitmode is set in 9-bit or 32-bit
	{
		dataRead = GetWord16(SPI_RX_TX_REG1) << 16;			// read high part of data from spi slave
	}

	dataRead += GetWord16(SPI_RX_TX_REG0);					//read (low part of) data from spi slave

	return dataRead;										// return data read from spi slave
}

/**
****************************************************************************************
* @brief SPI transaction to slave with CS handling (works in all bit modes)
* @param[in] dataToSend: data to send
* @return  data read 
****************************************************************************************
*/
uint32_t spi_transaction(uint32_t dataToSend)
{
	uint32_t dataRead = 0;
    
	spi_cs_low();
 
    dataRead = spi_access(dataToSend);						// basic SPI access
    
	spi_cs_high();
	 
	return dataRead;							            // return data read from spi slave
}


