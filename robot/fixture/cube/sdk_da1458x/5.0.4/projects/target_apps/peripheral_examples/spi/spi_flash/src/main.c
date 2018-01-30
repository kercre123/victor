/**
 ****************************************************************************************
 *
 * @file main.c
 *
 * @brief SPI flash example for DA14580/581 SDK
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
#include "common_uart.h"
#include "user_periph_setup.h"
#include "spi_flash.h"

uint8_t rd_data[512];
uint8_t wr_data[512];
int8_t detected_spi_flash_device_index;
SPI_Pad_t spi_FLASH_CS_Pad;

void system_init(void);
void spi_test(void);

/**
 ****************************************************************************************
 * @brief  Main routine of the SPI flash example
 * 
 ****************************************************************************************
 */
int main (void)
{
    system_init();
    periph_init();
    spi_test();
    while(1);
}
/**
 ****************************************************************************************
 * @brief System Initiialization 
 *
 * 
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
 * @brief SPI and SPI flash Initialization function
 * 
 ****************************************************************************************
 */
static void spi_flash_peripheral_init()
{
    spi_FLASH_CS_Pad.pin = SPI_CS_PIN;
    spi_FLASH_CS_Pad.port = SPI_GPIO_PORT;
    // Enable SPI & SPI FLASH

    spi_init(&spi_FLASH_CS_Pad, SPI_MODE_8BIT, SPI_ROLE_MASTER, SPI_CLK_IDLE_POL_LOW, SPI_PHA_MODE_0, SPI_MINT_DISABLE, SPI_XTAL_DIV_8);

    detected_spi_flash_device_index = spi_flash_auto_detect();

    if(detected_spi_flash_device_index == SPI_FLASH_AUTO_DETECT_NOT_DETECTED)
    {
        // The device was not identified.
        // The default parameters are used (SPI_FLASH_SIZE, SPI_FLASH_PAGE)
        // Alternatively, an error can be asserted here.
        spi_flash_init(SPI_FLASH_SIZE, SPI_FLASH_PAGE);
    }
}


void spi_protection_features_test(void)
{
    unsigned char buf1[][2] = {{0xE0, 0x0E}, {0xD0, 0x0D}, {0xB0, 0x0B}, {0x70, 0x07}};

    switch(detected_spi_flash_device_index)
    {
        case SPI_FLASH_DEVICE_INDEX_W25X10:
            printf_string("\n\rW25X10 SPI memory protection features demo.");
            
            printf_string("\n\r1) Unprotecting the whole memory array and doing a full erase");
            spi_flash_chip_erase_forced();  // the whole memory array gets unprotected in this function
            
            printf_string("\n\r2) Retrieving the two bytes at addresses 0x00000 and 0x10000");
            printf_string("\n\r   Reading [0x00000] = 0x");
            spi_flash_read_data(rd_data, 0, 1);
            printf_byte(rd_data[0]);
            printf_string(" and [0x10000] = 0x");
            spi_flash_read_data(rd_data, 0x10000, 1);
            printf_byte(rd_data[0]);
            printf_string("\n\r  (must be [0x00000] = 0xFF and [0x10000] = 0xFF, as the memory has been cleared)");
                
            printf_string("\n\r3) Writing [0x00000]<- 0xE0 and [0x10000]<- 0x0E to the unprotected memory");
            spi_flash_write_data(&buf1[0][0], 0, 1);
            spi_flash_write_data(&buf1[0][1], 0x10000, 1);
            printf_string("\n\r   Reading [0x00000] = 0x");
            spi_flash_read_data(rd_data, 0, 1);
            printf_byte(rd_data[0]);
            printf_string(" and [0x10000] = 0x");
            spi_flash_read_data(rd_data, 0x10000, 1);
            printf_byte(rd_data[0]); 
            printf_string("\n\r  (must be [0x00000] = 0xE0 and [0x10000] = 0x0E)");
            
            printf_string("\n\r4) Enabling memory protection for the whole memory array.");
                
            spi_flash_configure_memory_protection(W25x10_MEM_PROT_ALL);
            printf_string("\n\r5) Writing [0x00000]<- 0xD0 and [0x10000]<- 0x0D to the fully protected memory");
            spi_flash_write_data(&buf1[1][0], 0, 1);
            spi_flash_write_data(&buf1[1][1], 0x10000, 1);
            printf_string("\n\r   Reading [0x00000] = 0x");
            spi_flash_read_data(rd_data, 0, 1);
            printf_byte(rd_data[0]);
            printf_string(" and [0x10000] = 0x");
            spi_flash_read_data(rd_data, 0x10000, 1);
            printf_byte(rd_data[0]); 
            printf_string("\n\r  (must be [0x00000] = 0xE0 and [0x10000] = 0x0E), the old values)");

            printf_string("\n\r6) Enabling memory protection ONLY for the lower half (0..0xFFFF) of the memory array");
            spi_flash_configure_memory_protection(W25x10_MEM_PROT_LOWER_HALF);
            printf_string("\n\r7) Writing [0x00000]<- 0xB0 and [0x10000]<- 0x0B to the 'lower-half only protected' memory");
            spi_flash_write_data(&buf1[2][0], 0, 1);
            spi_flash_write_data(&buf1[2][1], 0x10000, 1);
            printf_string("\n\r   Reading [0x00000] = 0x");
            spi_flash_read_data(rd_data, 0, 1);
            printf_byte(rd_data[0]);
            printf_string(" and [0x10000] = 0x");
            spi_flash_read_data(rd_data, 0x10000, 1);
            printf_byte(rd_data[0]); 
            printf_string("\n\r  (must be [0x00000] = 0xE0 and [0x10000] = 0x0A (= 0x0E AND 0x0B), as only the upper half is writable)");

            printf_string("\n\r8) Enabling memory protection ONLY for the upper half (0x10000..0x1FFFF) of the memory array");
            spi_flash_configure_memory_protection(W25x10_MEM_PROT_UPPER_HALF);
            printf_string("\n\r9) Writing [0x00000]<- 0x70 and [0x10000]<- 0x07 to the 'upper-half only protected' memory");
            spi_flash_write_data(&buf1[3][0], 0, 1);
            spi_flash_write_data(&buf1[3][1], 0x10000, 1);
            printf_string("\n\r   Reading [0x00000] = 0x");
            spi_flash_read_data(rd_data, 0, 1);
            printf_byte(rd_data[0]);
            printf_string(" and [0x10000] = 0x");
            spi_flash_read_data(rd_data, 0x10000, 1);
            printf_byte(rd_data[0]); 
            printf_string("\n\r  (must be [0x00000] = 0x60 and [0x10000] = 0x0A, as only the lower half is writable)");
            
            printf_string("\n\r10) Unprotecting the whole memory array and doing a full erase");
            spi_flash_chip_erase_forced();  // the whole memory array gets unprotected in this function
        break;
        
        case SPI_FLASH_DEVICE_INDEX_W25X20:
            printf_string("\n\rW25X20 SPI memory protection features demo.");
            
            printf_string("\n\r1) Unprotecting the whole memory array and doing a full erase");
            spi_flash_chip_erase_forced();  // the whole memory array gets unprotected in this function
            
            printf_string("\n\r2) Retrieving the two bytes at addresses 0x00000 and 0x20000");
            printf_string("\n\r   Reading [0x00000] = 0x");
            spi_flash_read_data(rd_data, 0, 1);
            printf_byte(rd_data[0]);
            printf_string(" and [0x20000] = 0x");
            spi_flash_read_data(rd_data, 0x20000, 1);
            printf_byte(rd_data[0]);
            printf_string("\n\r  (must be [0x00000] = 0xFF and [0x20000] = 0xFF, as the memory has been cleared)");
            
            printf_string("\n\r3) Writing [0x00000]<- 0xE0 and [0x20000]<- 0x0E to the unprotected memory");
            spi_flash_write_data(&buf1[0][0], 0, 1);
            spi_flash_write_data(&buf1[0][1], 0x20000, 1);
            printf_string("\n\r   Reading [0x00000] = 0x");
            spi_flash_read_data(rd_data, 0, 1);
            printf_byte(rd_data[0]);
            printf_string(" and [0x20000] = 0x");
            spi_flash_read_data(rd_data, 0x20000, 1);
            printf_byte(rd_data[0]); 
            printf_string("\n\r  (must be [0x00000] = 0xE0 and [0x20000] = 0x0E)");
            
            printf_string("\n\r4) Enabling memory protection for the whole memory array.");

            spi_flash_configure_memory_protection(W25x20_MEM_PROT_ALL);
            printf_string("\n\r5) Writing [0x00000]<- 0xD0 and [0x20000]<- 0x0D to the fully protected memory");
            spi_flash_write_data(&buf1[1][0], 0, 1);
            spi_flash_write_data(&buf1[1][1], 0x20000, 1);
            printf_string("\n\r   Reading [0x00000] = 0x");
            spi_flash_read_data(rd_data, 0, 1);
            printf_byte(rd_data[0]);
            printf_string(" and [0x10000] = 0x");
            spi_flash_read_data(rd_data, 0x20000, 1);
            printf_byte(rd_data[0]); 
            printf_string("\n\r  (must be [0x00000] = 0xE0 and [0x20000] = 0x0E), the old values)");

            printf_string("\n\r6) Enabling memory protection ONLY for the lower half (0..0x1FFFF) of the memory array");
            spi_flash_configure_memory_protection(W25x20_MEM_PROT_LOWER_HALF);
            printf_string("\n\r7) Writing [0x00000]<- 0xB0 and [0x20000]<- 0x0B to the 'lower-half only protected' memory");
            spi_flash_write_data(&buf1[2][0], 0, 1);
            spi_flash_write_data(&buf1[2][1], 0x20000, 1);
            printf_string("\n\r   Reading [0x00000] = 0x");
            spi_flash_read_data(rd_data, 0, 1);
            printf_byte(rd_data[0]);
            printf_string(" and [0x10000] = 0x");
            spi_flash_read_data(rd_data, 0x20000, 1);
            printf_byte(rd_data[0]);
            printf_string("\n\r  (must be [0x00000] = 0xE0 and [0x20000] = 0x0A (= 0x0E AND 0x0B), as only the upper half is writable)");

            printf_string("\n\r8) Enabling memory protection ONLY for the upper half (0x20000..0x3FFFF) of the memory array");
            spi_flash_configure_memory_protection(W25x20_MEM_PROT_UPPER_HALF);
            printf_string("\n\r9) Writing [0x00000]<- 0x70 and [0x20000]<- 0x07 to the 'upper-half only protected' memory");
            spi_flash_write_data(&buf1[3][0], 0, 1);
            spi_flash_write_data(&buf1[3][1], 0x20000, 1);
            printf_string("\n\r   Reading [0x00000] = 0x");
            spi_flash_read_data(rd_data, 0, 1);
            printf_byte(rd_data[0]);
            printf_string(" and [0x20000] = 0x");
            spi_flash_read_data(rd_data, 0x20000, 1);
            printf_byte(rd_data[0]); 
            printf_string("\n\r  (must be [0x00000] = 0x60 and [0x20000] = 0x0A, as only the lower half is writable)");
            
            printf_string("\n\r  IMPORTANT NOTE: The API supports the protection also in quarters of the memory array for this device.");
            
            printf_string("\n\r10) Unprotecting the whole memory array and doing a full erase");
            spi_flash_chip_erase_forced();  // the whole memory array gets unprotected in this function
        break;

        case SPI_FLASH_DEVICE_INDEX_AT25Dx011:
            printf_string("\n\rAT25Dx011 SPI memory protection features demo.");
            
            printf_string("\n\r1) Unprotecting the whole memory array and doing a full erase");
            spi_flash_chip_erase_forced();  // the whole memory array gets unprotected in this function
            
            printf_string("\n\r2) Retrieving the two bytes at addresses 0x00000 and 0x10000");
            printf_string("\n\r   Reading [0x00000] = 0x");
            spi_flash_read_data(rd_data, 0, 1);
            printf_byte(rd_data[0]);
            printf_string(" and [0x10000] = 0x");
            spi_flash_read_data(rd_data, 0x10000, 1);
            printf_byte(rd_data[0]);
            printf_string("\n\r  (must be [0x00000] = 0xFF and [0x10000] = 0xFF, as the memory has been cleared)");
                
            printf_string("\n\r3) Writing [0x00000]<- 0xE0 and [0x10000]<- 0x0E to the unprotected memory");
            spi_flash_write_data(&buf1[0][0], 0, 1);
            spi_flash_write_data(&buf1[0][1], 0x10000, 1);
            printf_string("\n\r   Reading [0x00000] = 0x");
            spi_flash_read_data(rd_data, 0, 1);
            printf_byte(rd_data[0]);
            printf_string(" and [0x10000] = 0x");
            spi_flash_read_data(rd_data, 0x10000, 1);
            printf_byte(rd_data[0]); 
            printf_string("\n\r  (must be [0x00000] = 0xE0 and [0x10000] = 0x0E)");
            
            printf_string("\n\r4) Enabling memory protection for the whole memory array.");
            
            spi_flash_configure_memory_protection(AT25Dx011_MEM_PROT_ENTIRE_MEMORY_PROTECTED);
            printf_string("\n\r5) Writing [0x00000]<- 0xD0 and [0x10000]<- 0x0D to the fully protected memory");
            spi_flash_write_data(&buf1[1][0], 0, 1);
            spi_flash_write_data(&buf1[1][1], 0x10000, 1);
            printf_string("\n\r   Reading [0x00000] = 0x");
            spi_flash_read_data(rd_data, 0, 1);
            printf_byte(rd_data[0]);
            printf_string(" and [0x10000] = 0x");
            spi_flash_read_data(rd_data, 0x10000, 1);
            printf_byte(rd_data[0]); 
            printf_string("\n\r  (must be [0x00000] = 0xE0 and [0x10000] = 0x0E), the old values)");
          
            printf_string("\n\r6) Unprotecting the whole memory array and doing a full erase");
            spi_flash_chip_erase_forced();  // the whole memory array gets unprotected in this function
        break;
    }
}


/**
 ****************************************************************************************
 * @brief SPI and SPI flash test function
 * 
 ****************************************************************************************
 */
void spi_test(void)
{
    uint16_t man_dev_id;
    uint64_t unique_id;
    uint32_t jedec_id;
    int16_t btrd;
    int16_t i;

    uint16_t read_size = 256;  // set this variable to the desired read size
    uint16_t write_size = 512; // set this variable to the desired read size
    wr_data[0] = 0;
    
    for(i = 1 ; i < 512 ; i++)
    {
        wr_data[i] = wr_data[i-1] + 1;
    }
    
    printf_string("\n\r\n\r************");
    printf_string("\n\r* SPI TEST *\n\r");
    printf_string("************\n\r");
  
    // Enable FLASH and SPI
    spi_flash_peripheral_init();
    // spi_flash_chip_erase();
    // Read SPI Flash Manufacturer/Device ID
    man_dev_id = spi_read_flash_memory_man_and_dev_id();
    
    spi_cs_low();
    
    spi_cs_high();
    
    // Erase flash
    spi_flash_chip_erase();
   
    // Read SPI Flash first 256 bytes
    printf_string("\n\r\n\rReading SPI Flash first 256 bytes...");
    btrd = spi_flash_read_data(rd_data, 0, read_size);
    // Display Results
    for(i = 0 ; i < read_size ; i++)
    {
        printf_byte(rd_data[i]);
        printf_string(" ");
    }
    printf_string("\n\r\n\rBytes Read: 0x");
    printf_byte((btrd >> 8) & 0xFF);
    printf_byte((btrd) & 0xFF);
    printf_string("\n\r");
     
    // Read SPI Flash JEDEC ID
    jedec_id = spi_read_flash_jedec_id();
    printf_string("\n\rSPI flash JEDEC ID is ");
    printf_byte((jedec_id >> 16) & 0xFF);
    printf_byte((jedec_id >> 8) & 0xFF);
    printf_byte((jedec_id) & 0xFF);
    
    switch(detected_spi_flash_device_index)
    {
        case SPI_FLASH_DEVICE_INDEX_W25X10:
            printf_string("\n\rYou are using W25X10 (1-MBit) SPI flash device.");
        break;
        
        case SPI_FLASH_DEVICE_INDEX_W25X20:
            printf_string("\n\rYou are using W25X10 (2-MBit) SPI flash device.");
        break;

        case SPI_FLASH_DEVICE_INDEX_AT25Dx011:
            printf_string("\n\rYou are using AT25DF011 or AT25DS011 (1-MBit) SPI flash device.");
        break;
        
    }
    
    
    if((detected_spi_flash_device_index == SPI_FLASH_DEVICE_INDEX_W25X10) ||
       (detected_spi_flash_device_index == SPI_FLASH_DEVICE_INDEX_W25X20))
    {
        // Read SPI Flash Manufacturer/Device ID
        man_dev_id = spi_read_flash_memory_man_and_dev_id();
        printf_string("\n\r\n\rSPI flash Manufacturer/Device ID is ");
        printf_byte((man_dev_id >> 8) & 0xFF);
        printf_byte((man_dev_id) & 0xFF);
        
        unique_id = spi_read_flash_unique_id();
        printf_string("\n\r\n\rSPI flash Unique ID Number is ");
        printf_byte(((unique_id >> 32) >> 24) & 0xFF);
        printf_byte(((unique_id >> 32) >>16) & 0xFF);
        printf_byte(((unique_id >> 32) >>8) & 0xFF);
        printf_byte((unique_id >> 32) & 0xFF);
        printf_byte((unique_id >> 24) & 0xFF);
        printf_byte((unique_id >> 16) & 0xFF);
        printf_byte((unique_id >> 8) & 0xFF);
        printf_byte((unique_id) & 0xFF);
    }

    // Program Page example (256 bytes)
    printf_string("\n\r\n\rPerforming Program Page...");
    spi_flash_page_program(wr_data, 0, 256);
    printf_string("Page programmed. (");
    printf_byte(spi_flash_read_status_reg());
    printf_string(")\n\r");
    
    // Read SPI Flash first 256 bytes
    printf_string("\n\r\n\rReading SPI Flash first 256 bytes...");
    btrd = spi_flash_read_data(rd_data, 0 ,read_size);
    // Display Results
    for(i = 0 ; i < read_size ; i++)
    {
        printf_byte(rd_data[i]);
        printf_string(" ");
    }
    printf_string("\n\r\n\rBytes Read: 0x");
    printf_byte((btrd >> 8) & 0xFF);
    printf_byte((btrd) & 0xFF);
    printf_string("\n\r");

    printf_string("\n\rPerforming Sector Erase...");
    spi_flash_block_erase(0, SECTOR_ERASE);
    printf_string("Sector erased. (");
    printf_byte(spi_flash_read_status_reg());
    printf_string(")\n\r");

    // Write data example (512 bytes)
    printf_string("\n\r\n\rPerforming 512 byte write...");
    spi_flash_write_data(wr_data, 0, 512);
    printf_string("Data written. (");
    printf_byte(spi_flash_read_status_reg());
    printf_string(")\n\r");
    
    // Read SPI Flash first 512 bytes
    printf_string("\n\r\n\rReading SPI Flash first 512 bytes...");
    btrd = spi_flash_read_data(rd_data, 0 ,write_size);
    // Display Results
    for(i = 0 ; i < write_size ; i++)
    {
        printf_byte(rd_data[i]);
        printf_string(" ");
    }
    printf_string("\n\r\n\rBytes Read: 0x");
    printf_byte((btrd >> 8) & 0xFF);
    printf_byte((btrd) & 0xFF);
    printf_string("\n\r");
    
    // SPI FLASH memory protection features
    spi_protection_features_test();
    
    printf_string("\n\rEnd of test\n\r");
}
