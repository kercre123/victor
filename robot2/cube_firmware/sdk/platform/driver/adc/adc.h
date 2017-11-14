/**
 ****************************************************************************************
 *
 * @file adc.h
 *
 * @brief ADC module header file.
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

#ifndef _ADC_H_
#define _ADC_H_

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "global_io.h"
#include <stdint.h>
#include <stdbool.h>

/*
 * DEFINES
 ****************************************************************************************
 */

/* ADC channels */
#define ADC_CHANNEL_P00         (0)
#define ADC_CHANNEL_P01         (1)
#define ADC_CHANNEL_P02         (2)
#define ADC_CHANNEL_P03         (3)
#define ADC_CHANNEL_AVS         (4)
#define ADC_CHANNEL_VDD_REF     (5)
#define ADC_CHANNEL_VDD_RTT     (6)
#define ADC_CHANNEL_VBAT3V      (7)
#define ADC_CHANNEL_VDCDC       (8)
#define ADC_CHANNEL_VBAT1V      (9)

#define ADC_POLARITY_SIGNED     (0x400)
#define ADC_POLARITY_UNSIGNED   (0x0)

#define ADC_TIMEOUT             (100000)

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief ADC module initialization.
 * @param[in] mode  :0 = Differential mode, GP_ADC_SE(0x800) = Single ended mode
 * @param[in] sign  :0 = Default, GP_ADC_SIGN(0x0400) = Conversion with opposite sign at
                     input and output to cancel out the internal offset of the ADC and
                     low-frequency
 * @param[in] attn  :0 = attenuation x1, GP_ADC_ATTN3X(0x0002) = attenuation x3
 * @return void
 ****************************************************************************************
 */
void adc_init(uint16_t mode, uint16_t sign, uint16_t attn);

/**
 ****************************************************************************************
 * @brief Enable selected channel.
 * @param[in] input_selection Input channel
 * @return void
 ****************************************************************************************
 */
void adc_enable_channel(uint16_t input_selection);

/**
 ****************************************************************************************
 * @brief Disable ADC module.
 * @return void
 ****************************************************************************************
 */
void adc_disable(void);

/**
 ****************************************************************************************
 * @brief Get ADC sample.
 * @return ADC sample
 ****************************************************************************************
 */
int adc_get_sample(void);

/**
 ****************************************************************************************
 * @brief ADC calibration sequence.
 * @return void
 ****************************************************************************************
 */
void adc_calibrate(void);

/**
 ****************************************************************************************
 * @brief Gets ADC sample from VBAT1V or VBAT3V power supplies using the 20 usec delay.
 * @param[in] sample_vbat1v :true = sample VBAT1V, false = sample VBAT3V
 * @return ADC VBAT1V or VBAT3V sample
 ****************************************************************************************
 */
uint32_t adc_get_vbat_sample(bool sample_vbat1v);

#endif // _ADC_H_
