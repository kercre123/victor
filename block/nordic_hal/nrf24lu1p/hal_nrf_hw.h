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
* @brief hal_nrf macros for nRF24LU1+
 *
 * @defgroup hal_nrf24lu1p_hal_nrf_hw hal_nrf_hw
 * @{
 * @ingroup hal_nrf24lu1p
 *
 */

#ifndef HAL_NRF_LU1_H__
#define HAL_NRF_LU1_H__
#include "nrf24lu1p.h"

/** Macro that set radio's CSN line LOW.
 *
 */
#define CSN_LOW() do { RFCSN = 0; } while(false)

/** Macro that set radio's CSN line HIGH.
 *
 */
#define CSN_HIGH() do { RFCSN = 1; } while(false)

/** Macro that set radio's CE line LOW.
 *
 */
#define CE_LOW() do { RFCE = 0; } while(false)

/** Macro that set radio's CE line HIGH.
 *
 */
#define CE_HIGH() do { RFCE = 1; } while(false)

/** Macro for writing the radio SPI data register.
 *
 */
#define HAL_NRF_HW_SPI_WRITE(d) do{RFDAT = d; RFSPIF = 0;} while(false)

/** Macro for reading the radio SPI data register.
 *
 */
#define HAL_NRF_HW_SPI_READ() RFDAT
  
/** Macro specifyng the radio SPI busy flag.
 *
 */
#define HAL_NRF_HW_SPI_BUSY (!RFSPIF)

/**
 * Pulses the CE to nRF24L01 for at least 10 us
 */
#define CE_PULSE() do { \
  uint8_t count; \
  count = 20; \
  CE_HIGH();  \
  while(count--){} \
  CE_LOW();  \
  } while(false)

#endif // HAL_NRF_LU1_H__

/** @} */
