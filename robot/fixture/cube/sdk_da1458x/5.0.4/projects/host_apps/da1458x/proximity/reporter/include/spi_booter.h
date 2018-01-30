/**
 ****************************************************************************************
 *
 * @file spi_hci_msg.h
 *
 * @brief hci over spi message exchange header file.
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

#ifndef _SPI_BOOTER_
#define _SPI_BOOTER_

/**
****************************************************************************************
* @brief Used to calculate the boot image CRC
* @param[in] length: 	Length of the image in 32-bit words
* @return CRC checksum (1 byte)
****************************************************************************************
*/
uint8_t calc_crc(uint32_t length);

/**
****************************************************************************************
* @brief Used to transmit the boot image header
* @param[in] crc: 		CRC of the boot image
* @param[in] length: 	Length of the payload in 32-bit words
* @param[in] mode: 		SPI mode used for the transfer
* @return Transfer error (0) or transfer OK (1)
****************************************************************************************
*/
uint32_t send_header(uint32_t length,uint8_t crc,uint32_t mode);


/**
****************************************************************************************
* @brief Used to transmit the boot image payload
* @param[in] mode: 		SPI mode used for the transfer
* @param[in] length: 	Length of the payload in 32-bit words
* @return Transfer error (0) or transfer OK (1)
****************************************************************************************
*/
uint8_t send_payload(uint32_t mode,uint32_t length);

/**
****************************************************************************************
* @brief Transmit a boot image to an SPI slave device
****************************************************************************************
*/
void spi_send_image(void);

// end _SPI_BOOTER_
#endif
