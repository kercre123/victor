/**
 ****************************************************************************************
 *
 * @file adc.c
 *
 * @brief ADC module source file.
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

#include "adc.h"

/*
 * FUNCTION DEFINITIONS
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
void adc_init(uint16_t mode, uint16_t sign, uint16_t attn)
{
    SetWord16(GP_ADC_CTRL_REG,  GP_ADC_LDO_EN | mode | sign);
    SetWord16(GP_ADC_CTRL_REG,  GP_ADC_LDO_EN | mode | GP_ADC_EN | sign);
    SetWord16(GP_ADC_CTRL2_REG, GP_ADC_DELAY_EN | GP_ADC_I20U | attn );      // Enable 3x attenuation
}

/**
 ****************************************************************************************
 * @brief Enable selected channel.
 * @param[in] input_selection Input channel
 * @return void
 ****************************************************************************************
 */
void adc_enable_channel(uint16_t input_selection)
{
    SetBits16(GP_ADC_CTRL_REG,GP_ADC_SEL,input_selection & 0xF);
}

/**
 ****************************************************************************************
 * @brief Disable ADC module.
 * @return void
 ****************************************************************************************
 */
void adc_disable(void)
{
    SetWord16(GP_ADC_CTRL_REG, 0);
}

/**
 ****************************************************************************************
 * @brief Get ADC sample.
 * @return ADC sample
 ****************************************************************************************
 */
int adc_get_sample(void)
{
    int cnt = ADC_TIMEOUT;

    SetBits16(GP_ADC_CTRL_REG, GP_ADC_START, 1);
    while (cnt-- && (GetWord16(GP_ADC_CTRL_REG) & GP_ADC_START) != 0x0000);
    SetWord16(GP_ADC_CLEAR_INT_REG, 0x0000); // Clear interrupt
    return GetWord16(GP_ADC_RESULT_REG);
}

/**
 ****************************************************************************************
 * @brief Introduces a variable microsend delay for use with ADC peripheral.
 * @param[in] nof_us Number of microseconds to delay
 * @return void
 ****************************************************************************************
 */
void adc_usDelay(uint32_t nof_us)
{
    while( nof_us-- ){
        __nop();
        __nop();
        __nop();
        __nop();
        __nop();
        __nop();
        __nop();
        __nop();
        __nop();
        __nop();
        __nop();
    }
}

/**
 ****************************************************************************************
 * @brief Gets ADC sample from VBAT1V or VBAT3V power supplies using the 20 usec delay.
 * @param[in] sample_vbat1v :true = sample VBAT1V, false = sample VBAT3V
 * @return ADC VBAT1V or VBAT3V sample
 ****************************************************************************************
 */
uint32_t adc_get_vbat_sample(bool sample_vbat1v)
{
    uint32_t adc_sample, adc_sample2;

    adc_init(GP_ADC_SE, GP_ADC_SIGN, GP_ADC_ATTN3X);
    adc_usDelay(20);

    if (sample_vbat1v)
        adc_enable_channel(ADC_CHANNEL_VBAT1V);
    else
        adc_enable_channel(ADC_CHANNEL_VBAT3V);

    adc_sample = adc_get_sample();
    adc_usDelay(1);
    adc_init(GP_ADC_SE, 0, GP_ADC_ATTN3X );

    if (sample_vbat1v)
        adc_enable_channel(ADC_CHANNEL_VBAT1V);
    else
        adc_enable_channel(ADC_CHANNEL_VBAT3V);

    adc_sample2 = adc_get_sample();
    //We have to divide the following result by 2 if
    //the 10 bit accuracy is enough
    adc_sample = (adc_sample2 + adc_sample);
    adc_disable();

    return adc_sample;
}

/**
 ****************************************************************************************
 * @brief ADC calibration sequence.
 * @return void
 ****************************************************************************************
 */
void adc_calibrate(void)
{
    uint32_t adc_res1, adc_res2;
    int32_t  adc_off_p, adc_off_n;
    int32_t  new_adc_off_p, new_adc_off_n;;

    SetWord16(GP_ADC_CTRL_REG, GP_ADC_LDO_EN|GP_ADC_SE|GP_ADC_EN);
    adc_usDelay(20);

    SetWord16(GP_ADC_OFFP_REG,  0x200);
    SetWord16(GP_ADC_OFFN_REG,  0x200);

    SetBits16(GP_ADC_CTRL_REG, GP_ADC_MUTE, 1);
    SetBits16(GP_ADC_CTRL_REG, GP_ADC_SIGN, 0);

    adc_res1 = adc_get_sample();
    adc_off_p = adc_res1 - 0x200;

    SetBits16(GP_ADC_CTRL_REG, GP_ADC_SIGN, 1);
    adc_res2 = adc_get_sample();
    adc_off_n = adc_res2 - 0x200;

    new_adc_off_p = 0x200 - 2 * adc_off_p;
    new_adc_off_n = 0x200 - 2 * adc_off_n;

    SetWord16(GP_ADC_OFFP_REG,  new_adc_off_p);
    SetWord16(GP_ADC_OFFN_REG,  new_adc_off_n);
}
