/* Copyright (c) 2009 Nordic Semiconductor. All Rights Reserved.
 *
 * The information contained herein is confidential property of Nordic 
 * Semiconductor ASA.Terms and conditions of usage are described in detail 
 * in NORDIC SEMICONDUCTOR STANDARD SOFTWARE LICENSE AGREEMENT. 
 *
 * Licensees are granted free, non-transferable use of the information. NO
 * WARRENTY of ANY KIND is provided. This heading must NOT be removed from
 * the file.
 *              
 * $LastChangedRevision: 133 $
 */

/** @file
* @brief  Implementation of the SPI HAL module for nRF24LU1+
 */
#include "nrf24lu1p.h"
#include "hal_spi.h"

#define SPI_DATA    0x01
#define SPI_START   0x02
#define SPI_STOP    0x04

void hal_spi_master_init(hal_spi_clkdivider_t ck, hal_spi_mode_t mode, hal_spi_byte_order_t bo)
{
  uint8_t smctl;
  uint8_t temp = mode;  // mode is not used in nRF24LU1
  temp = bo;            // byte_order is not used in nRF24LU1

  I3FR = 1;             // rising edge SPI ready detect
  P0EXP = 0x01;         // Map SPI master on P0
  INTEXP = 0x02;        // Select SPI master on IEX3
  SPIF = 0;             // Clear any pending interrupts
  switch(ck)
  {
    case SPI_CLK_DIV2:
      smctl = 0x11;
      break;

    case SPI_CLK_DIV4:
      smctl = 0x12;
      break;

    case SPI_CLK_DIV8:
      smctl = 0x13;
      break;

    case  SPI_CLK_DIV16:
      smctl = 0x14;
      break;

    case SPI_CLK_DIV32:
      smctl = 0x15;
      break;

    case SPI_CLK_DIV64:
    default:
      smctl = 0x16;
      break;
  }
  SMCTL = smctl;        // Enable SPI master with the specified divide factor
}

uint8_t hal_spi_master_read_write(uint8_t pLoad)
{
  SPIF = 0;             // Clear interrupt request
  SMDAT = pLoad;        // Start the SPI operation by writing the data
  while(SPIF == 0)      // Wait until SPI has finished transmitting
    ;
  return SMDAT;         // Return the the read byte
}
