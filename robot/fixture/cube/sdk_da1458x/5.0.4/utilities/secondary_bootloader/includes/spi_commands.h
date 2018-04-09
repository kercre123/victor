 /**
 ****************************************************************************************
 *
 * @file spi_bootloader.h
 *
 * @brief  spi loader settings
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
 
#ifndef _SPI_BOOTLOADER_H
#define _SPI_BOOTLOADER_H
#include "global_io.h" 

/**
****************************************************************************************
* @brief Initialize the spi flash
****************************************************************************************
**/
void spi_flash_peripheral_init(void);

/**
****************************************************************************************
* @brief Read a block of data from spu flash
* @param[in] destination_buffer: buffer to put the data
* @param[in] source_addr: starting position to read the data from spi
* @param[in] len: size of data to read  
****************************************************************************************
*/
void SpiFlashRead(unsigned long destination_buffer,unsigned long source_addr,unsigned long len);

/**
****************************************************************************************
* @brief  Initialize the flash size
****************************************************************************************
*/
void spi_flash_size_init(void);

#endif
