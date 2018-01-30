/**
 ****************************************************************************************
 *
 * @file image.h
 *
 * @brief Definition of image header structure.
 *
 * Copyright (C) 2014. Dialog Semiconductor Ltd, unpublished work. This computer 
 * program includes Confidential, Proprietary Information and is a Trade Secret of 
 * Dialog Semiconductor Ltd.  All use, disclosure, and/or reproduction is prohibited 
 * unless authorized in writing. All Rights Reserved.
 *
 * <bluetooth.support@diasemi.com> and contributors.
 *
 ****************************************************************************************
 */

#ifndef __IMAGE_H
#define __IMAGE_H

#include <stdint.h>

/* header for single image */
struct image_header {
	uint8_t signature[2];
	uint8_t valid_flag;
	uint8_t image_id;
	uint8_t code_size[4];
	uint8_t CRC[4] ;
	uint8_t version[16];
	uint8_t timestamp[4];
	uint8_t flags;
	uint8_t reserved[31];
};

/* single image header flags */
#define IMG_ENCRYPTED		0x01

/* AN-B-001 header for SPI */
struct an_b_001_spi_header {
	uint8_t preamble[2];
	uint8_t empty[4];
	uint8_t len[2];
};

/* AN-B-001 header for I2C/EEPROM */
struct an_b_001_i2c_header {
	uint8_t preamble[2];
	uint8_t len[2];
	uint8_t crc;
	uint8_t dummy[27];
};

/* product header */
struct product_header {
	uint8_t signature[2];
	uint8_t version[2];
	uint8_t offset1[4];
	uint8_t offset2[4];
	uint8_t bd_address[6];
	uint8_t pad[2];
	uint8_t cfg_offset[4];
};

enum multi_options {
		SPI = 1,
		EEPROM = 2,
		BOOTLOADER = 4,
		CONFIG = 8
};


#endif  /* __IMAGE_H */
