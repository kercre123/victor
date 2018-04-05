/**
 ****************************************************************************************
 *
 * @file main.c
 *
 * @brief I2C EEPROM example for DA14580/581 SDK
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

#include <stdio.h>
#include "global_io.h"
#include "common_uart.h"
#include "user_periph_setup.h"
#include "i2c_eeprom.h"

/*
 * DEFINES
 ****************************************************************************************
 */

#define BUFFER_SIZE             (256)

/*
 * VARIABLE DEFINITIONS
 ****************************************************************************************
 */

uint8_t wr_data[BUFFER_SIZE];
uint8_t rd_data[BUFFER_SIZE];

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

void system_init(void);
void i2c_test(void);

/**
 ****************************************************************************************
 * @brief Test write data.
 * @param[in] data              Pointer to the array of bytes to be written
 * @param[in] address           Starting address of the write process
 * @param[in] size              Size of the data to be written
 * @param[in|out] bytes_written Bytes that were actually written (due to memory size limitation)
 * @return void
 ****************************************************************************************
 */
static void test_write_data(uint8_t *data, uint32_t address, uint32_t size, uint32_t *bytes_written)
{
    printf_string("\n\rWriting page to EEPROM (values 0x00-FF)...\n\r");
    i2c_error_code code = i2c_eeprom_write_data(data, address, size, bytes_written);
    if (code == I2C_NO_ERROR)
    {
        printf_string("\n\rPage written.\n\r");
        printf_string("\n\rBytes written: 0x");
        printf_byte(((*bytes_written) >> 16) & 0xFF);
        printf_byte(((*bytes_written) >> 8) & 0xFF);
        printf_byte((*bytes_written) & 0xFF);
        printf_string("\n\r");
    }
    else
    {
        printf_string(" - Test failed! - I2C Error code: 0x");
        printf_byte(code & 0xFF);
        printf_string("\n\r");
    }
}

/**
 ****************************************************************************************
 * @brief Test write byte.
 * @param[in] address Memory position to write the byte to
 * @param[in] byte    Byte to be written
 * @return void
 ****************************************************************************************
 */
static void test_write_byte(uint32_t address, uint8_t byte)
{
    printf_string("\n\rWrite byte (0x");
    printf_byte(byte & 0xFF);
    printf_string(") @ address ");
    printf_byte_dec(address & 0xFF);
    printf_string(" (zero based)...\n\r");
    i2c_error_code code = i2c_eeprom_write_byte(address, byte);
    if (code == I2C_NO_ERROR)
    {
        printf_string("\n\rWrite done.\n\r");
    }
    else
    {
        printf_string(" - Test failed! - I2C Error code: 0x");
        printf_byte(code & 0xFF);
        printf_string("\n\r");
    }
}

/**
 ****************************************************************************************
 * @brief Test read byte.
 * @param[in] address  Memory address to read the byte from
 * @param[in|out] byte Pointer to the read byte
 * @return void
 ****************************************************************************************
 */
static void test_read_byte(uint32_t address, uint8_t *byte)
{
    i2c_error_code code = i2c_eeprom_read_byte(address, byte);
    printf_string("\n\rRead byte @ address ");
    printf_byte_dec(address & 0xFF);
    printf_string(": 0x");
    printf_byte((*byte) & 0xFF);
    printf_string("\n\r");
    if (code != I2C_NO_ERROR)
    {
        printf_string(" - Test failed! - I2C Error code: 0x");
        printf_byte(code & 0xFF);
        printf_string("\n\r");
    }
}

/**
 ****************************************************************************************
 * @brief Test read data.
 * @param[in] data           Pointer to the array of bytes to be read
 * @param[in] address        Starting memory address
 * @param[in] size           Size of the data to be read
 * @param[in|out] bytes_read Bytes that were actually read (due to memory size limitation)
 * @return void
 ****************************************************************************************
 */
static void test_read_data(uint8_t *data, uint32_t address, uint32_t size, uint32_t *bytes_read)
{
    i2c_error_code code = i2c_eeprom_read_data(data, address, size, bytes_read);
    if (code == I2C_NO_ERROR)
    {
        printf_string("\n\r");
        // Display Results
        for (uint32_t i = 0 ; i < size ; i++)
        {
            printf_byte(data[i]);
            printf_string(" ");
        }
        printf_string("\n\r\n\rBytes read: 0x");
        printf_byte(((*bytes_read) >> 16) & 0xFF);
        printf_byte(((*bytes_read) >> 8) & 0xFF);
        printf_byte((*bytes_read) & 0xFF);
        printf_string("\n\r");
    }
    else
    {
        printf_string(" - Test failed! - I2C Error code: 0x");
        printf_byte(code & 0xFF);
        printf_string("\n\r");
    }
}

/**
 ****************************************************************************************
 * @brief  Main routine of the I2C EEPROM example
 ****************************************************************************************
 */
int main (void)
{
    system_init();
    periph_init();
    i2c_test();
    while(1);
}

/**
 ****************************************************************************************
 * @brief System initialization.
 * @return void
 ****************************************************************************************
 */
void system_init(void)
{
    SetWord16(CLK_AMBA_REG, 0x00);                 // set clocks (hclk and pclk ) 16MHz
    SetWord16(SET_FREEZE_REG,FRZ_WDOG);            // stop watch dog
    SetBits16(SYS_CTRL_REG,PAD_LATCH_EN,1);        // open pads
    SetBits16(SYS_CTRL_REG,DEBUGGER_ENABLE,1);     // open debugger
    SetBits16(PMU_CTRL_REG, PERIPH_SLEEP,0);       // exit peripheral power down
}

/**
 ****************************************************************************************
 * @brief I2C EEPROM test suite.
 * @return void
 ****************************************************************************************
 */
void i2c_test(void)
{
    uint8_t byte;
    uint16_t i;
    uint32_t bytes_read = 0;
    uint32_t bytes_written = 0;
    uint16_t read_size = BUFFER_SIZE;  // the desired read size
    uint16_t write_size = BUFFER_SIZE; // the desired write size

    // Populate write vector
    for (i = 0 ; i < write_size ; i++)
    {
        wr_data[i] = i;
    }
    // Reset read vector
    for (i = 0 ; i < read_size ; i++)
    {
        rd_data[i] = 0;
    }

    // Initialize I2C interface for the I2C EEPROM
    i2c_eeprom_init(I2C_SLAVE_ADDRESS, I2C_SPEED_MODE, I2C_ADDRESS_MODE, I2C_ADDRESS_SIZE);

    printf_string("\n\r\n\r************");
    printf_string("\n\r* I2C TEST *\n\r");
    printf_string("************\n\r");

    // Page Write
    printf_string("\n\r*** Test 1 ***\n\r");
    test_write_data(wr_data, 0, write_size, &bytes_written);

    // Page Read
    printf_string("\n\r*** Test 2 ***\n\r");
    printf_string("\n\rReading EEPROM...\n\r");
    test_read_data(rd_data, 0, read_size, &bytes_read);

    // Setting 1 : 0x5A@22 (dec)
    // Write Byte
    printf_string("\n\r*** Test 3 ***\n\r");
    test_write_byte(22, 0x5A);

    // Random Read Byte
    printf_string("\n\r*** Test 4 ***\n\r");
    test_read_byte(22, &byte);

    // Setting 2 : 0x6A@00 (dec)
    // Write Byte
    printf_string("\n\r*** Test 5 ***\n\r");
    test_write_byte(0, 0x6A);

    // Random Read Byte
    printf_string("\n\r*** Test 6 ***\n\r");
    test_read_byte(0, &byte);

    // Setting 3 : 0x7A@255 (dec)
    // Write Byte
    printf_string("\n\r*** Test 7 ***\n\r");
    test_write_byte(255, 0x7A);

    // Random Read Byte
    printf_string("\n\r*** Test 8 ***\n\r");
    test_read_byte(255, &byte);

    // Setting 4 : 0xFF@30 (dec)
    // Write Byte
    printf_string("\n\r*** Test 9 ***\n\r");
    test_write_byte(30, 0xFF);

    // Random Read Byte
    printf_string("\n\r*** Test 10 ***\n\r");
    test_read_byte(30, &byte);

    // Page read for verification
    printf_string("\n\r*** Test 11 ***\n\r");
    printf_string("\n\rPage Read to verify that new bytes have been written correctly\n\r");
    printf_string("--------------------------------------------------------------\n\r");
    test_read_data(rd_data, 0, read_size, &bytes_read);

    i2c_eeprom_release();
    printf_string("\n\r");
    printf_string("\n\rEnd of test\n\r");
}
