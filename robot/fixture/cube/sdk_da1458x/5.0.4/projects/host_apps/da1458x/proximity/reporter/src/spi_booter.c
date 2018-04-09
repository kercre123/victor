/**
 ****************************************************************************************
 *
 * @file spi_booter.c
 *
 * @brief External SPI master boot routines
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
#include "app.h"
#include "global_io.h"
#include "gpio.h"
#include "queue.h"
#include "user_periph_setup.h"
#include "peripherals.h"
#include "spi.h"
#include "spi_booter.h"
#include "uart.h"


// Image file to be loaded
#ifdef PROX_REPORTER_EXT_581
#include "prox_reporter_ext_581.h"
#else
#include "prox_reporter_ext.h"
#endif

#define SPI_8BIT (0)
#define SPI_16BIT (1)
#define SPI_32BIT (2)
#define SPI_ACK 0x02
#define SPI_NACK 0x20
#define CODE_BASE_ADDRESS 0x20000000
#define SPI_LENGTH IMAGE_SIZE/4  // Amount of data in 32-bit words


uint32_t send_header(uint32_t length,uint8_t crc,uint32_t mode);
uint8_t calc_crc(uint32_t length);
uint8_t send_payload(uint32_t mode,uint32_t length);
SPI_Pad_t spi_booter_pad;


/**
****************************************************************************************
* @brief Transmit a boot image to an SPI slave device
****************************************************************************************
*/
void spi_send_image (void)
{
    uint8_t mode;
    uint8_t crc;
    uint8_t header_ack;
    uint8_t payload_ack;

    spi_booter_pad.pin = SPI_CS_PIN;
    spi_booter_pad.port = SPI_GPIO_PORT;

    crc = calc_crc(SPI_LENGTH);
    mode = 0x00;
    spi_init(&spi_booter_pad, SPI_MODE_8BIT, SPI_ROLE_MASTER, SPI_CLK_IDLE_POL_LOW, SPI_PHA_MODE_0, SPI_MINT_DISABLE, SPI_XTAL_DIV_4);
    do {
        header_ack = send_header(SPI_LENGTH,crc,mode);
        if (header_ack)
        {
            payload_ack = send_payload(SPI_8BIT,SPI_LENGTH);
            break;
        }
    } while((header_ack&payload_ack)!=1);
    spi_release();
}


/**
****************************************************************************************
* @brief Used to transmit the boot image payload
* @param[in] mode:         SPI mode used for the transfer
* @param[in] length:     Length of the payload in 32-bit words
* @return Transfer error (0) or transfer OK (1)
****************************************************************************************
*/
uint8_t send_payload(uint32_t mode,uint32_t length)
{
    uint32_t spi_read_data;
    uint32_t *ptr;
    uint32_t i;
    ptr     = (uint32_t*)program_t;
    spi_cs_high();

    /*
    i=0;
    do{
        i++;
    }while(i<200);
    */
    spi_cs_low();
    for(i=0;i<length;i++)
    {
        switch(mode)
        {
            case SPI_8BIT:
                spi_access((*ptr)&0xFF);
                spi_access((*ptr>>8)&0xFF);
                spi_access((*ptr>>16)&0xFF);
                spi_access((*ptr>>24)&0xFF);
            break;

            case SPI_16BIT:
                spi_access((*ptr)&0xFFFF);
                spi_access((*ptr>>16)&0xFFFF);
                break;

            case SPI_32BIT:
                spi_access(*ptr);
                break;

            default:
                break;
        };

        ptr++;
    }

    spi_cs_high();

    spi_cs_low();
    spi_read_data=spi_access(0x00);
    if((spi_read_data&0xFF)!=0xAA)
    {
        spi_cs_high();
        return 0;
    }

    spi_read_data=spi_access(0x00);
    if((spi_read_data&0xFF)!=SPI_ACK)
    {
        spi_cs_high();
        return 0;
    }
    spi_cs_high();
    return 1;
}


/**
****************************************************************************************
* @brief Used to transmit the boot image header
* @param[in] crc:         CRC of the boot image
* @param[in] length:     Length of the payload in 32-bit words
* @param[in] mode:         SPI mode used for the transfer
* @return Transfer error (0) or transfer OK (1)
****************************************************************************************
*/
uint32_t send_header(uint32_t length,uint8_t crc,uint32_t mode)
{
    uint32_t spi_read_data;

    spi_cs_low();
    spi_access(0x70);
    spi_access(0x50);
    spi_access(0x00);
    spi_read_data=spi_access(length&0xFF);
    if(spi_read_data!=SPI_ACK)
    {
        spi_cs_high();
        return 0;
    }
    spi_read_data=spi_access((length>>8)&0xFF);
    spi_read_data=spi_access(crc);
    spi_read_data=spi_access(mode);
    if(spi_read_data!=SPI_ACK)
    {
        spi_cs_high();
        return 0;
    }
    spi_access(0x00);
    spi_cs_high();
    return 1;
}


/**
****************************************************************************************
* @brief Used to calculate the boot image CRC
* @param[in] length:     Length of the image in 32-bit words
* @return CRC checksum (1 byte)
****************************************************************************************
*/
uint8_t calc_crc(uint32_t length)
{
    uint32_t i;
    uint32_t temp;
    uint8_t crc;
    crc=0xFF;
    for(i=0;i<length;i++)
    {
        temp=GetWord32(program_t+4*i);
        crc^=(0xFF&(temp>>24));
        crc^=(0xFF&(temp>>16));
        crc^=(0xFF&(temp>>8 ));
        crc^=(0xFF&(temp    ));
    }
    return crc;
}
