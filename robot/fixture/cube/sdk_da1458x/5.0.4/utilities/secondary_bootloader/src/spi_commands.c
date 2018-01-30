/**
 ****************************************************************************************
 *
 * @file spi_bootloader.c
 *
 * @brief SPI loader related functions
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
#include "user_periph_setup.h"
#include "spi_commands.h"
#include "spi_flash.h"
#include "spi.h"
#include "gpio.h"
 
  
    
SPI_Pad_t spi_FLASH_CS_Pad;
/**
****************************************************************************************
* @brief Initialize the spi flash
****************************************************************************************
**/
void spi_flash_peripheral_init(void)
{
    GPIO_ConfigurePin( SPI_GPIO_PORT, SPI_CS_PIN, OUTPUT, PID_SPI_EN, true );
    GPIO_ConfigurePin( SPI_GPIO_PORT, SPI_CLK_PIN, OUTPUT, PID_SPI_CLK, false );
    GPIO_ConfigurePin( SPI_GPIO_PORT, SPI_DO_PIN, OUTPUT, PID_SPI_DO, false );  
    GPIO_ConfigurePin( SPI_GPIO_PORT, SPI_DI_PIN, INPUT, PID_SPI_DI, false );
    spi_FLASH_CS_Pad.pin = SPI_CS_PIN;
    spi_FLASH_CS_Pad.port = SPI_GPIO_PORT;
    // Enable SPI & SPI FLASH
    spi_flash_init(SPI_FLASH_DEFAULT_SIZE, SPI_FLASH_DEFAULT_PAGE);
    spi_init(&spi_FLASH_CS_Pad, SPI_MODE_8BIT,   SPI_ROLE_MASTER,  SPI_CLK_IDLE_POL_LOW,SPI_PHA_MODE_0,SPI_MINT_DISABLE,SPI_XTAL_DIV_2);
}

/**
****************************************************************************************
* @brief  Initialize the flash size
****************************************************************************************
*/ 
void spi_flash_size_init(void)
{
    uint32_t flash_size = 0;//SPI_FLASH_DEFAULT_SIZE; // Default memory size in bytes
    uint32_t flash_page = 0;//SPI_FLASH_DEFAULT_PAGE; // Default memory page size in bytes
    uint16_t man_dev_id = 0;
    
    man_dev_id=spi_read_flash_memory_man_and_dev_id();
    switch(man_dev_id)
    {
        case W25X10CL_MANF_DEV_ID:
            flash_size = W25X10CL_SIZE;
            flash_page = W25X10CL_PAGE;
            break; 
        case W25X20CL_MANF_DEV_ID:
            flash_size = W25X20CL_SIZE;
            flash_page = W25X20CL_PAGE;
            break; 
        default:
            flash_size = SPI_FLASH_DEFAULT_SIZE; 
            flash_page = SPI_FLASH_DEFAULT_PAGE; 
          break;
    }
    spi_flash_init(flash_size, flash_page);
}

/**
****************************************************************************************
* @brief Write a single item to serial flash via SPI
* @param[in] x: data to send
****************************************************************************************
*/
void putByte_SPI(uint32 x)
{ 
    uint16 tmp; 
    SetWord16(SPI_RX_TX_REG0, (uint16_t)x);    // write (low part of) dataToSend
    do
    {
    } while (GetBits16(SPI_CTRL_REG, SPI_INT_BIT) == 0);  // polling to wait for spi transmission
    SetWord16(SPI_CLEAR_INT_REG, 0x01);            // clear pending flag
    tmp = GetWord16(SPI_RX_TX_REG0);

    (void) tmp; // SPI RX value is not used
}

/**
****************************************************************************************
* @brief Read a single item from serial flash via SPI
* @param[in] x: buffer to read the data 
****************************************************************************************
*/
inline void getByte_SPI(uint8* x  )
{
    const uint16_t DUMMY = 0xffff;
    SetWord16(SPI_RX_TX_REG0, (uint16) DUMMY);
    while (GetBits16(SPI_CTRL_REG, SPI_INT_BIT==0));
    SetWord16(SPI_CLEAR_INT_REG, 0x01);
    *(uint8*)x=(uint8)GetWord16(SPI_RX_TX_REG0);   
} 

/**
****************************************************************************************
* @brief Read a block of data from spu flash
* @param[in] destination_buffer: buffer to put the data
* @param[in] source_addr: starting position to read the data from spi
* @param[in] len: size of data to read  
****************************************************************************************
*/
void SpiFlashRead(unsigned long destination_buffer,unsigned long source_addr,unsigned long len)
{
    unsigned long i;

    spi_cs_low();
    putByte_SPI(0x03);
    putByte_SPI((source_addr&0xffffff)>>16);
    putByte_SPI((source_addr&0xffff)>>8);
    putByte_SPI(source_addr&0xff);
    SetWord16(SPI_RX_TX_REG0, (uint16) 0);

    __ASM volatile ("nop");
    __ASM volatile ("nop");
    for (i=0;i<len;i++)
    {
        SetWord16(SPI_RX_TX_REG0, (uint16) 0); 
        __ASM volatile ("nop");
        __ASM volatile ("nop");
        __ASM volatile ("nop");
        __ASM volatile ("nop");
        __ASM volatile ("nop");
        __ASM volatile ("nop");
        *(uint8*)(uint32)(destination_buffer+i)=(uint8)GetWord16(SPI_RX_TX_REG0);
    }
    spi_cs_high();
} 

