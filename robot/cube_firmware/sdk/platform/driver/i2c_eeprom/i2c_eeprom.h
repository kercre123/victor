/**
 ****************************************************************************************
 *
 * @file i2c_eeprom.h
 *
 * @brief eeprom driver over i2c interface header file.
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

/*******************************************************************************************/
/* Configuration of EEPROM IC's that have been tested                                      */
/* Define one of the following set of directives in your application                       */
/* (not in the body of the driver)                                                         */
/* a) Microchip 24LC02 (2 kBit - 8 byte page)                                              */
/*      #define I2C_EEPROM_SIZE   0x100            // EEPROM size in bytes                 */
/*      #define I2C_EEPROM_PAGE   8                // EEPROM's page size in bytes          */
/*      #define I2C_SPEED_MODE    I2C_FAST         // fast mode (400 kbits/s)              */
/*      #define I2C_ADDRESS_MODE  I2C_7BIT_ADDR    // 7-bit addressing                     */
/*      #define I2C_ADDRESS_SIZE  I2C_1BYTE_ADDR   // 8-bit memory address                 */
/* b) Microchip 24LC64 (64 kBit - 32 byte page)                                            */
/*      #define I2C_EEPROM_SIZE   0x2000           // EEPROM size in bytes                 */
/*      #define I2C_EEPROM_PAGE   32               // EEPROM's page size in bytes          */
/*      #define I2C_SPEED_MODE    I2C_FAST         // fast mode (400 kbits/s)              */
/*      #define I2C_ADDRESS_MODE  I2C_7BIT_ADDR    // 7-bit addressing                     */
/*      #define I2C_ADDRESS_SIZE  I2C_2BYTES_ADDR  // 16-bit memory address                */
/* c) ST M24M01 (1 MBit - 256 byte page)                                                   */
/*      #define I2C_EEPROM_SIZE   0x20000           // EEPROM size in bytes                */
/*      #define I2C_EEPROM_PAGE   256               // EEPROM's page size in bytes         */
/*      #define I2C_SPEED_MODE    I2C_FAST          // fast mode (400 kbits/s)             */
/*      #define I2C_ADDRESS_MODE  I2C_7BIT_ADDR     // 7-bit addressing                    */
/*      #define I2C_ADDRESS_SIZE  I2C_2BYTES_ADDR   // 16-bit memory address               */
/*                                                                                         */
/* Refer to EEPROM's datasheet to define I2C_SLAVE_ADDRESS                                 */
/* i2c_eeprom_init(I2C_SLAVE_ADDRESS, I2C_SPEED_MODE, I2C_ADDRESS_MODE, I2C_2BYTES_ADDR);  */
/*******************************************************************************************/

#ifndef _I2C_EEPROM_H_
#define _I2C_EEPROM_H_

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include <stdint.h>

/*
 * DEFINES
 ****************************************************************************************
 */

#define I2C_MAX_RETRIES                 (200000)

enum I2C_SPEED_MODES
{
    I2C_STANDARD = 1,
    I2C_FAST,
};

enum I2C_ADDRESS_MODES
{
    I2C_7BIT_ADDR,
    I2C_10BIT_ADDR,
};

enum I2C_ADRESS_BYTES_COUNT
{
    I2C_1BYTE_ADDR,
    I2C_2BYTES_ADDR,
    I2C_3BYTES_ADDR,
};

typedef enum
{
    I2C_NO_ERROR,
    I2C_7B_ADDR_NOACK_ERROR,
    I2C_INVALID_EEPROM_ADDRESS
} i2c_error_code;

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Initialize I2C controller as a master for I2C EEPROM handling.
 * @param[in] dev_address  Slave device address
 * @param[in] speed        Speed
 * @param[in] address_mode Addressing mode
 * @param[in] address_size Address size
 * @retun void
 ****************************************************************************************
 */
void i2c_eeprom_init(uint16_t dev_address, uint8_t speed, uint8_t address_mode, uint8_t address_size);

/**
 ****************************************************************************************
 * @brief Disable I2C controller and clock.
 * @return void
 ****************************************************************************************
 */
void i2c_eeprom_release(void);

/**
 ****************************************************************************************
 * @brief Poll until I2C eeprom is ready.
 * @return i2c_error_code Enumeration type that defines the returned error code
 ****************************************************************************************
 */
i2c_error_code i2c_wait_until_eeprom_ready(void);

/**
 ****************************************************************************************
 * @brief Read single byte from I2C EEPROM.
 * @param[in] address     Memory address to read the byte from
 * @param[in|out] byte    Pointer to the read byte
 * @return i2c_error_code Enumeration type that defines the returned error code
 ****************************************************************************************
 */
i2c_error_code i2c_eeprom_read_byte(uint32_t address, uint8_t *byte);

/**
 ****************************************************************************************
 * @brief Reads data from I2C EEPROM to memory position of given pointer.
 * @param[in|out] rd_data_ptr Read data pointer
 * @param[in] address         Starting memory address
 * @param[in] size            Size of the data to be read
 * @param[in|out] bytes_read  Bytes that were actually read (due to memory size limitation)
 * @return i2c_error_code     Enumeration type that defines the returned error code
 ****************************************************************************************
 */
i2c_error_code i2c_eeprom_read_data(uint8_t *rd_data_ptr, uint32_t address, uint32_t size, uint32_t *bytes_read);

/**
 ****************************************************************************************
 * @brief Write single byte to I2C EEPROM.
 * @param[in] address     Memory position to write the byte to
 * @param[in] byte        Byte to be written
 * @return i2c_error_code Enumeration type that defines the returned error code
 ****************************************************************************************
 */
i2c_error_code i2c_eeprom_write_byte(uint32_t address, uint8_t byte);

/**
 ****************************************************************************************
 * @brief Writes page to I2C EEPROM.
 * @param[in] address           Starting address of memory page (MUST BE MULTIPLE OF I2C_EEPROM_PAGE)
 * @param[in] wr_data_ptr       Pointer to the first of the bytes to be written
 * @param[in] size              Size of the data to be written (MUST BE LESS OR EQUAL TO I2C_EEPROM_PAGE)
 * @param[in|out] bytes_written Bytes that were actually written (due to memory size limitation)
 * @return i2c_error_code       Enumeration type that defines the returned error code
 ****************************************************************************************
 */
i2c_error_code i2c_eeprom_write_page(uint8_t *wr_data_ptr, uint32_t address, uint16_t size, uint32_t *bytes_written);

/**
 ****************************************************************************************
 * @brief Writes data to I2C EEPROM.
 * @param[in] wr_data_ptr       Pointer to the first of the bytes to be written
 * @param[in] address           Starting address of the write process
 * @param[in] size              Size of the data to be written
 * @param[in|out] bytes_written Bytes that were actually written (due to memory size limitation)
 * @return i2c_error_code       Enumeration type that defines the returned error code
 ****************************************************************************************
 */
i2c_error_code i2c_eeprom_write_data(uint8_t *wr_data_ptr, uint32_t address, uint32_t size, uint32_t *bytes_written);

#endif // _I2C_EEPROM_H_
