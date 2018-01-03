/**
 ****************************************************************************************
 *
 * @file spi_mouse.c
 *
 * @brief 3-wire SPI low level driver.
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
#include "gpio.h"
#include "spi_3wire.h"
#include "arch_sleep.h"

uint16_t TsradCounter;
SPI_Pin sdio;
SPI_Pin cs;

/**
 ****************************************************************************************
 * @brief Initialize communication with the 3-wire SPI
 * @param[in] spi_sdio_pin_register: Size of the data to be read
 * @param[in] TsradCounterToSet: Counter for the delay between Address and Data phase in reading
 * 
 ****************************************************************************************
 */
void initialize_3wire_spi(struct SPI_Config *cfg, bool enable_interrupt,  uint16_t TsradCounterToSet)
{ 
    sdio.port = cfg->sdio.port;                                   // initialize with param data;
    sdio.pin = cfg->sdio.pin;   
    cs.port = cfg->cs.port;
    cs.pin = cfg->cs.pin;
        
    //configure pins
    GPIO_ConfigurePin( cfg->cs.port, cfg->cs.pin, OUTPUT, PID_SPI_EN, true);
    GPIO_SetPinFunction( cfg->sdio.port, cfg->sdio.pin, INPUT, PID_SPI_DI);
    GPIO_SetPinFunction( cfg->clk.port, cfg->clk.pin, OUTPUT, PID_SPI_CLK);
    
    TsradCounter = TsradCounterToSet;                             // SPI read address-data delay (refer to sensor datasheet)
   
	// init SPI
    SetBits16(CLK_PER_REG, SPI_ENABLE, 1);                        // enable  clock for SPI                  
	SetBits16(SPI_CTRL_REG,SPI_ON,0);                             // close SPI block, if opened              
	SetBits16(SPI_CTRL_REG,SPI_WORD, cfg->SPI_Word_Mode);         // SPI word mode              
	SetBits16(SPI_CTRL_REG,SPI_SMN, cfg->SPI_Role);               // SPI master mode              
	SetBits16(SPI_CTRL_REG,SPI_POL, cfg->SPI_Polarity_Mode);      // SPI mode selection - polarity              
	SetBits16(SPI_CTRL_REG,SPI_PHA, cfg->SPI_PHA_Mode);           // SPI mode selection - phase              
	SetBits16(SPI_CTRL_REG,SPI_MINT, cfg->SPI_MINT_Mode);         // disable SPI interrupt to the ICU              
	SetBits16(SPI_CTRL_REG,SPI_CLK, cfg->SPI_XTAL_Freq);          // SPI block clock divider              
	SetBits16(SPI_CTRL_REG,SPI_ON, 1);                            // enable SPI block              
    if (enable_interrupt)                                         
        NVIC_EnableIRQ(SPI_IRQn);                                 // enable SPI interrupt, if MINT is '1' and enable_interrupt is set              
    
}


/**
 ****************************************************************************************
 * @brief Deactivate communication with the 3-wire SPI
 ****************************************************************************************
 */
void deactivate_3wire_spi(void)
{
    GPIO_SetActive(cs.port, cs.pin);                                            // leave CS high        
    GPIO_SetPinFunction( sdio.port, sdio.pin, INPUT, PID_SPI_DI);
	NVIC_DisableIRQ(SPI_IRQn);                                                  // disable SPI interrupt
	SetBits16(SPI_CTRL_REG, SPI_ON, 0);                                         // close SPI block, if opened
	SetBits16(CLK_PER_REG, SPI_ENABLE, 0);                                      // disable clock for SPI
}


/**
 ****************************************************************************************
 * @brief Write to 3-wire SPI
 * @param[in] registerIndex: Target address (A6..A0)
 * @param[in] valueToWrite: Value to write
 * 
 * @return  Number of read bytes
 ****************************************************************************************
 */
void write_to_3wire_SPI_register(uint8_t registerIndex, uint8_t valueToWrite)
{
    GPIO_SetInactive(cs.port, cs.pin);                              // pull CS low
   
    GPIO_SetPinFunction( sdio.port, sdio.pin, OUTPUT, PID_SPI_DO);  // configure SDIO as output  
     
    SetWord16(SPI_RX_TX_REG0, (uint16_t)(registerIndex | 0x80) );   // MSB set to HIGH - A6..A0 Address of register to write to
	do{
	} while (GetBits16(SPI_CTRL_REG,SPI_INT_BIT)==0);               // polling to wait for spi transmission
    
    SetWord16(SPI_CLEAR_INT_REG, 0);                                //Clear SPI_CTRL_REG[SPI_INT_BIT] interrupt
    
    SetWord16(SPI_RX_TX_REG0, (uint16_t)valueToWrite );             // write data
	do{
	} while (GetBits16(SPI_CTRL_REG,SPI_INT_BIT)==0);               // polling to wait for spi transmission
    
    SetWord16(SPI_CLEAR_INT_REG, 0);                                // Clear SPI_CTRL_REG[SPI_INT_BIT] interrupt
     
    GPIO_SetActive(cs.port, cs.pin);                                // set CS high
}    

 

/**
 ****************************************************************************************
 * @brief Read from 3-wire SPI
 * @param[in] registerIndex: Target address (A6..A0)
 * 
 * @return  byte read
 ****************************************************************************************
 */
uint8_t read_from_3wire_SPI_register(uint8_t registerIndex, uint8_t is_last_transaction)
{
    static uint8_t i, dataRead;

    GPIO_SetInactive(cs.port, cs.pin);                              // pull CS low   
     
    GPIO_SetPinFunction( sdio.port, sdio.pin, OUTPUT, PID_SPI_DO);  // configure SDIO as output 
    
    SetWord16(SPI_RX_TX_REG0, (uint16_t)(registerIndex) );          // MSB set to HIGH - A6..A0 Address of register to write to
	do{
	} while (GetBits16(SPI_CTRL_REG,SPI_INT_BIT)==0);               // polling to wait for spi transmission
    
    SetWord16(SPI_CLEAR_INT_REG, 0);                                //Clear SPI_CTRL_REG[SPI_INT_BIT] interrupt
    
    GPIO_SetPinFunction( sdio.port, sdio.pin, INPUT, PID_SPI_DI);   // configure SDIO as input  
    
    for (i=0; i<TsradCounter; i++);                                 // {DEV.NOTE#: For Mouse sensor Delay > Tsrad = 4us <-- suitable counter is 6}
    
    SetWord16(SPI_RX_TX_REG0, (uint16_t)0x00 );                     // dummy write data - read data
	do{
	} while (GetBits16(SPI_CTRL_REG,SPI_INT_BIT)==0);               // polling to wait for spi transmission

    SetWord16(SPI_CLEAR_INT_REG, 0);                                //Clear SPI_CTRL_REG[SPI_INT_BIT] interrupt    
     
	dataRead = GetWord16(SPI_RX_TX_REG0);                           // read received byte
	
	SetWord16(SPI_CLEAR_INT_REG, 0x01);                             // clear pending flag   
	
    if(is_last_transaction)
      GPIO_SetActive(cs.port, cs.pin);                              // set CS high 
    
    return dataRead;
}   

void burst_write_to_3wire_SPI_register(uint8_t registerIndex)
{ 
   GPIO_SetPinFunction( sdio.port, sdio.pin, OUTPUT, PID_SPI_DO);   // configure SDIO as output 
   SetWord16(SPI_RX_TX_REG0, (uint16_t)(registerIndex) );           // MSB set to HIGH - A6..A0 Address of register to write to
   do{
   } while (GetBits16(SPI_CTRL_REG,SPI_INT_BIT)==0);                // polling to wait for spi transmission

   SetWord16(SPI_CLEAR_INT_REG, 0);                                 // Clear SPI_CTRL_REG[SPI_INT_BIT] interrupt
   GPIO_SetPinFunction( sdio.port, sdio.pin, INPUT, PID_SPI_DI);    // configure SDIO as input  

}   

uint8_t burst_read_from_3wire_SPI_register(void)
{
   static uint8_t dataRead;
   
   SetWord16(SPI_RX_TX_REG0, (uint16_t)0x00 );                      // dummy write data - read data
   do{
   } while (GetBits16(SPI_CTRL_REG,SPI_INT_BIT)==0);                // polling to wait for spi transmission
   
   SetWord16(SPI_CLEAR_INT_REG, 0);                                 // Clear SPI_CTRL_REG[SPI_INT_BIT] interrupt
   dataRead = GetWord16(SPI_RX_TX_REG0);                            // read received byte
   SetWord16(SPI_CLEAR_INT_REG, 0x01);                              // clear pending flag   
   
   return dataRead;
}   

void spi3wire_cs_high (void)
{
      GPIO_SetActive(cs.port, cs.pin);                              // set CS high 
}

void spi3wire_cs_low (void)
{
      GPIO_SetInactive(cs.port, cs.pin);                            // set CS low 
}

