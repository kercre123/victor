/*
 * Copyright (c) 2014, Freescale Semiconductor, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of Freescale Semiconductor, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#if !defined(__FSL_TPM_HAL_H__)
#define __FSL_TPM_HAL_H__

#include "fsl_device_registers.h"
#include <stdbool.h>
#include <assert.h>

/*!
 * @addtogroup tpm_hal
 * @{
 */

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*! @brief TPM clock source selection for TPM_SC[CMOD].*/
typedef enum _tpm_clock_mode
{
     kTpmClockSourceNoneClk = 0,    /*TPM clock mode, None CLK*/
     kTpmClockSourceModuleClk,      /*TPM clock mode, Module CLK*/
     kTpmClockSourceExternalClk,    /*TPM clock mode, External input clock*/
     kTpmClockSourceReservedClk     /*TPM clock mode, Reserved*/
}tpm_clock_mode_t;

/*! @brief TPM counting mode, up or down*/
typedef enum _tpm_counting_mode
{
     kTpmCountingUp = 0,           /*TPM counter mode, Up counting only*/
     kTpmCountingUpDown            /*TPM counter mode, Up/Down counting mode*/
}tpm_counting_mode_t;

/*! @brief TPM prescaler factor selection for clock source*/
typedef enum _tpm_clock_ps
{
     kTpmDividedBy1 = 0,          /*TPM module clock prescaler, by 1*/
     kTpmDividedBy2 ,             /*TPM module clock prescaler, by 2*/
     kTpmDividedBy4 ,             /*TPM module clock prescaler, by 4*/
     kTpmDividedBy8,              /*TPM module clock prescaler, by 8*/
     kTpmDividedBy16,             /*TPM module clock prescaler, by 16*/
     kTpmDividedBy32,             /*TPM module clock prescaler, by 32*/
     kTpmDividedBy64,             /*TPM module clock prescaler, by 64*/
     kTpmDividedBy128             /*TPM module clock prescaler, by 128*/
}tpm_clock_ps_t;

/*! @brief TPM trigger sources */
typedef enum _tpm_trigger_source_t
{
    kTpmExtTrig = 0,                /*TPM trigger source, external pin, EXTTRG_IN*/
    kTpmCmp0Trig,                   /*TPM trigger source, CMP0 output*/
    kTpmResvTrig2,                  /*TPM trigger source, Reserved*/
    kTpmResvTrig3,                  /*TPM trigger source, Reserved*/
    kTpmPit0Trig,                   /*TPM trigger source, Pit0 Overflow*/
    kTpmPit1Trig,                   /*TPM trigger source, Pit1 Overflow*/
    kTpmResvTrig6,                  /*TPM trigger source, Reserved*/
    kTpmResvTrig7,                  /*TPM trigger source, Reserved*/
    kTpmTpm0Trig,                   /*TPM trigger source, TPM0 Overflow*/
    kTpmTpm1Trig,                   /*TPM trigger source, TPM1 Overflow*/
    kTpmTpm2Trig,                   /*TPM trigger source, TPM2 Overflow*/
    kTpmResvTrig11,                 /*TPM trigger source, Reserved*/
    kTpmRtcAlarmTrig,               /*TPM trigger source, RTC Alarm*/
    kTpmRtcSecondTrig,              /*TPM trigger source, RTC Second*/
    kTpmLptimerTrig,                /*TPM trigger source, LPTimer*/
    kTpmResvTrig15,                 /*TPM trigger source, Reserved*/
}tpm_trigger_source_t;

/*! @brief TPM operation mode */
typedef enum _tpm_pwm_mode_t
{
    kTpmEdgeAlignedPWM = 0,
    kTpmCenterAlignedPWM
}tpm_pwm_mode_t;

/*! @brief TPM PWM output pulse mode, high-true or low-true on match up */
typedef enum _tpm_pwm_edge_mode_t
{
    kTpmHighTrue = 0,
    kTpmLowTrue
}tpm_pwm_edge_mode_t;

/*! @brief TPM input capture modes */
typedef enum _tpm_input_capture_mode_t
{
    kTpmRisingEdge = 1,
    kTpmFallingEdge,
    kTpmRiseOrFallEdge
}tpm_input_capture_mode_t;

/*! @brief TPM output compare modes */
typedef enum _tpm_output_compare_mode_t
{
    kTpmOutputNone = 0,
    kTpmToggleOutput,
    kTpmClearOutput,
    kTpmSetOutput,
    kTpmHighPulseOutput,
    kTpmLowPulseOutput
}tpm_output_compare_mode_t;

/*!
 * @brief FlexTimer driver PWM parameter
 *
 */
typedef struct TpmPwmParam
{
    tpm_pwm_mode_t mode;          /*!< TPM PWM operation mode */
    tpm_pwm_edge_mode_t edgeMode;    /*!< PWM output mode */
    uint32_t uFrequencyHZ;           /*!< PWM period in Hz */
    uint32_t uDutyCyclePercent;      /*!< PWM pulse width, value should be between 0 to 100
                                          0=inactive signal(0% duty cycle)...
                                          100=active signal (100% duty cycle). */
}tpm_pwm_param_t;


/*TPM timer control*/

/*!
 * @brief set TPM clock mod.
 * @param baseAddr TPM module base address.
 * @param mode  The TPM counter clock
 */
static inline void TPM_HAL_SetClockMode(uint32_t baseAddr, tpm_clock_mode_t mode)
{
    BW_TPM_SC_CMOD(baseAddr, mode);
}

/*!
 * @brief get TPM clock mod.
 * @param baseAddr TPM module base address.
 */
static inline tpm_clock_mode_t TPM_HAL_GetClockMode(uint32_t baseAddr)
{
    return (tpm_clock_mode_t) BR_TPM_SC_CMOD(baseAddr);
}

/*!
 * @brief set TPM clock divider.
 * @param baseAddr TPM module base address.
 * @param ps  The TPM peripheral clock prescale divider
 */
static inline void TPM_HAL_SetClockDiv(uint32_t baseAddr, tpm_clock_ps_t ps)
{
    BW_TPM_SC_PS(baseAddr, ps);
}

/*!
 * @brief get TPM clock divider.
 * @param baseAddr TPM module base address.
 */
static inline tpm_clock_ps_t TPM_HAL_GetClockDiv(uint32_t baseAddr)
{
    return (tpm_clock_ps_t)BR_TPM_SC_PS(baseAddr);
}

/*!
 * @brief Enable the TPM peripheral timer overflow interrupt.
 *
 * @param baseAddr TPM module base address.
 */
static inline void TPM_HAL_EnableTimerOverflowInt(uint32_t baseAddr)
{
    BW_TPM_SC_TOIE(baseAddr, 1);
}

/*!
 * @brief Disable the TPM peripheral timer overflow interrupt.
 *
 * @param baseAddr TPM module base address.
 */
static inline void TPM_HAL_DisableTimerOverflowInt(uint32_t baseAddr)
{
    BW_TPM_SC_TOIE(baseAddr, 0);
}

/*!
 * @brief Read the bit that controls TPM timer overflow interrupt enablement.
 *
 * @param baseAddr TPM module base address.
 * @retval true if overflow interrupt is enabled, false if not
 */
static inline bool TPM_HAL_IsOverflowIntEnabled(uint32_t baseAddr)
{
    return (bool)(BR_TPM_SC_TOIE(baseAddr));
}

/*!
 * @brief return TPM peripheral timer overflow interrupt flag.
 * @param baseAddr TPM module base address.
 * @retval true if overflow, false if not
 */
static inline bool TPM_HAL_GetTimerOverflowStatus(uint32_t baseAddr)
{
    return (bool)(BR_TPM_SC_TOF(baseAddr));
}

/*!
 * @brief Clear the TPM timer overflow interrupt flag.
 * @param baseAddr TPM module base address.
 */
static inline void TPM_HAL_ClearTimerOverflowFlag(uint32_t baseAddr)
{
    BW_TPM_SC_TOF(baseAddr, 1);
}

/*!
 * @brief set TPM center-aligned PWM select.
 * @param baseAddr TPM module base address.
 * @param mode 1:upcounting mode 0:up_down counting mode.
 */
static inline void TPM_HAL_SetCpwms(uint32_t baseAddr, uint8_t mode)
{
    assert(mode < 2);
    BW_TPM_SC_CPWMS(baseAddr, mode);
}

/*!
 * @brief get TPM center-aligned PWM selection value.
 * @param baseAddr TPM module base address.
 */
static inline bool TPM_HAL_GetCpwms(uint32_t baseAddr)
{
    return (bool)BR_TPM_SC_CPWMS(baseAddr);
}

/*!
 * @brief clear TPM peripheral current counter value.
 * @param baseAddr TPM module base address.
 */
static inline void  TPM_HAL_ClearCounter(uint32_t baseAddr)
{
    BW_TPM_CNT_COUNT(baseAddr, 0);
}

/*!
 * @brief return TPM peripheral current counter value.
 * @param baseAddr TPM module base address.
 * @retval current TPM timer counter value
 */
static inline uint16_t  TPM_HAL_GetCounterVal(uint32_t baseAddr)
{
    return BR_TPM_CNT_COUNT(baseAddr);
}

/*!
 * @brief set TPM peripheral timer modulo value,
 * @param baseAddr TPM module base address.
 * @param val The value to be set to the timer modulo
 */
static inline void TPM_HAL_SetMod(uint32_t baseAddr, uint16_t val)
{
    /*As RM mentioned, first clear TPM_CNT then write value to TPM_MOD*/
    BW_TPM_CNT_COUNT(baseAddr, 0);
    BW_TPM_MOD_MOD(baseAddr, val);
}

/*!
 * @brief return TPM peripheral counter modulo value.
 * @param baseAddr TPM module base address.
 * @retval TPM timer modula value
 */
static inline uint16_t  TPM_HAL_GetMod(uint32_t baseAddr)
{
    return BR_TPM_MOD_MOD(baseAddr);
}

/*TPM channel operate mode(Mode, edge and level selection) for capture, output, pwm*/

/*!
 * @brief Set TPM peripheral timer channel mode and edge level,
 *
 * TPM channel operate mode, MSnBA and ELSnBA shoud be set at the same time.
 *
 * @param baseAddr The TPM base address
 * @param channel  The TPM peripheral channel number
 * @param value    The value to set for MSnBA and ELSnBA
 */
static inline void TPM_HAL_SetChnMsnbaElsnbaVal(uint32_t baseAddr, uint8_t channel, uint8_t value)
{
    assert(channel < FSL_FEATURE_TPM_CHANNEL_COUNT);

    /* Keep CHIE bit value not changed by this function, so read it first and or with value*/
    value |= HW_TPM_CnSC_RD(baseAddr, channel) & BM_TPM_CnSC_CHIE;

    HW_TPM_CnSC_WR(baseAddr, channel, value);
}

/*!
 * @brief get TPM peripheral timer channel mode,
 * @param baseAddr TPM module base address.
 * @param channel  The TPM peripheral channel number
 * @retval The MSnB:MSnA mode value, will be 00,01, 10, 11
 */
static inline uint8_t TPM_HAL_GetChnMsnbaVal(uint32_t baseAddr, uint8_t channel)
{
    assert(channel < FSL_FEATURE_TPM_CHANNEL_COUNT);
    return (HW_TPM_CnSC_RD(baseAddr, channel) & (BM_TPM_CnSC_MSA | BM_TPM_CnSC_MSB)) >> BP_TPM_CnSC_MSA;
}

/*!
 * @brief get TPM peripheral timer channel edge level,
 * @param baseAddr TPM module base address.
 * @param channel  The TPM peripheral channel number
 * @retval The ELSnB:ELSnA mode value, will be 00,01, 10, 11
 */
static inline uint8_t TPM_HAL_GetChnElsnbaVal(uint32_t baseAddr, uint8_t channel)
{
    assert(channel < FSL_FEATURE_TPM_CHANNEL_COUNT);
    return (HW_TPM_CnSC_RD(baseAddr, channel) & (BM_TPM_CnSC_ELSA | BM_TPM_CnSC_ELSB)) >> BP_TPM_CnSC_ELSA;
}

/*!
 * @brief enable TPM peripheral timer channel(n) interrupt.
 * @param baseAddr TPM module base address.
 * @param channel  The TPM peripheral channel number
 */
static inline void TPM_HAL_EnableChnInt(uint32_t baseAddr, uint8_t channel)
{
    assert(channel < FSL_FEATURE_TPM_CHANNEL_COUNT);
    BW_TPM_CnSC_CHIE(baseAddr, channel, 1);
}

/*!
 * @brief disable TPM peripheral timer channel(n) interrupt.
 * @param baseAddr TPM module base address.
 * @param channel  The TPM peripheral channel number
 */
static inline void TPM_HAL_DisableChnInt(uint32_t baseAddr, uint8_t channel)
{
    assert(channel < FSL_FEATURE_TPM_CHANNEL_COUNT);
    BW_TPM_CnSC_CHIE(baseAddr, channel, 0);
}

/*!
 * @brief get TPM peripheral timer channel(n) interrupt enabled or not.
 * @param baseAddr TPM module base address.
 * @param channel  The TPM peripheral channel number
 */
static inline bool TPM_HAL_IsChnIntEnabled(uint32_t baseAddr, uint8_t channel)
{
    assert(channel < FSL_FEATURE_TPM_CHANNEL_COUNT);
    return (bool)(BR_TPM_CnSC_CHIE(baseAddr, channel));
}

/*!
 * @brief return if any event for TPM peripheral timer channel has occourred ,
 * @param baseAddr TPM module base address.
 * @param channel  The TPM peripheral channel number.
 * @retval true if event occourred, false otherwise
 */
static inline bool TPM_HAL_GetChnStatus(uint32_t baseAddr, uint8_t channel)
{
    assert(channel < FSL_FEATURE_TPM_CHANNEL_COUNT);
    return (bool)(BR_TPM_CnSC_CHF(baseAddr, channel));
}

/*!
 * @brief return if any event for TPM peripheral timer channel has occourred ,
 * @param baseAddr TPM module base address.
 * @param channel  The TPM peripheral channel number.
 * @retval true if event occourred, false otherwise
 */
static inline void TPM_HAL_ClearChnInt(uint32_t baseAddr, uint8_t channel)
{
    assert(channel < FSL_FEATURE_TPM_CHANNEL_COUNT);
    BW_TPM_CnSC_CHF(baseAddr, channel, 0x1);
}

/*TPM Channel control*/
/*!
 * @brief set TPM peripheral timer channel counter value,
 * @param baseAddr TPM module base address.
 * @param channel  The TPM peripheral channel number.
 * @param val counter value to be set
 */
static inline void TPM_HAL_SetChnCountVal(uint32_t baseAddr, uint8_t channel, uint16_t val)
{
    assert(channel < FSL_FEATURE_TPM_CHANNEL_COUNT);
    BW_TPM_CnV_VAL(baseAddr, channel, val);
}

/*!
 * @brief get TPM peripheral timer channel counter value,
 * @param baseAddr TPM module base address.
 * @param channel  The TPM peripheral channel number.
 * @retval val return current channel counter value
 */
static inline uint16_t TPM_HAL_GetChnCountVal(uint32_t baseAddr, uint8_t channel)
{
    assert(channel < FSL_FEATURE_TPM_CHANNEL_COUNT);
    return BR_TPM_CnV_VAL(baseAddr, channel);
}

/*!
 * @brief get TPM peripheral timer  channel event status
 * @param baseAddr TPM module base address.
 * @retval val return current channel event status value
 */
static inline uint32_t TPM_HAL_GetStatusRegVal(uint32_t baseAddr)
{
    return HW_TPM_STATUS_RD(baseAddr);
}

/*!
 * @brief clear TPM peripheral timer clear status register value,
 * @param baseAddr TPM module base address.
 * @param tpm_status tpm channel or overflow flag to clear
 */
static inline void TPM_HAL_ClearStatusReg(uint32_t baseAddr, uint16_t tpm_status)
{
    HW_TPM_STATUS_WR(baseAddr, tpm_status);
}

/*!
 * @brief set TPM peripheral timer trigger.
 * @param baseAddr TPM module base address.
 * @param trigger_num  0-15
 */
static inline void TPM_HAL_SetTriggerSrc(uint32_t baseAddr, tpm_trigger_source_t trigger_num)
{
    BW_TPM_CONF_TRGSEL(baseAddr, trigger_num);
}

/*!
 * @brief set TPM peripheral timer running on trigger or not .
 * @param baseAddr TPM module base address.
 * @param enable true to enable, 1 to enable
 */
static inline void TPM_HAL_SetTriggerMode(uint32_t baseAddr, bool enable)
{
    BW_TPM_CONF_CSOT (baseAddr, enable);
}

/*!
 * @brief enable TPM timer counter reload on selected trigger or not.
 * @param baseAddr TPM module base address.
 * @param enable  true to enable, false to disable.
 */
static inline void TPM_HAL_SetReloadOnTriggerMode(uint32_t baseAddr, bool enable)
{
    BW_TPM_CONF_CROT(baseAddr, enable);
}

/*!
 * @brief enable TPM timer counter sotp on selected trigger or not.
 * @param baseAddr TPM module base address.
 * @param enable  true to enable, false to disable.
 */
static inline void TPM_HAL_SetStopOnOverflowMode(uint32_t baseAddr, bool enable)
{
    BW_TPM_CONF_CSOO(baseAddr, enable);
}

/*!
 * @brief enable TPM timer global time base.
 * @param baseAddr TPM module base address.
 * @param enable  true to enable, false to disable.
 */
static inline void TPM_HAL_EnableGlobalTimeBase(uint32_t baseAddr, bool enable)
{
    BW_TPM_CONF_GTBEEN(baseAddr, enable);
}

/*!
 * @brief set BDM mode.
 * @param baseAddr TPM module base address.
 * @param enable  false pause, true continue work
 */
static inline void TPM_HAL_SetDbgMode(uint32_t baseAddr, bool enable)
{
    BW_TPM_CONF_DBGMODE(baseAddr, enable ? 3 : 0);
}

/*!
 * @brief set WAIT mode behavior.
 * @param baseAddr TPM module base address.
 * @param enable  0 continue running, 1 stop running
 */
static inline void TPM_HAL_SetWaitMode(uint32_t baseAddr, bool enable)
{
    BW_TPM_CONF_DOZEEN(baseAddr, enable ? 0 : 1);
}

/*hal functionality*/

/*!
 * @brief reset tpm registers
 *
 * @param baseAddr TPM module base address.
 * @param instance The TPM peripheral instance number.
 */
void TPM_HAL_Reset(uint32_t baseAddr, uint32_t instance);

/*!
 * @brief Enables the TPM PWM output mode.
 *
 * @param baseAddr TPM module base address.
 * @param config PWM configuration parameter
 * @param channel The TPM channel number.
 */
void TPM_HAL_EnablePwmMode(uint32_t baseAddr, tpm_pwm_param_t *config, uint8_t channel);

/*!
 * @brief Disables the TPM channel.
 *
 * @param baseAddr TPM module base address.
 * @param channel The TPM channel number.
 */
void TPM_HAL_DisableChn(uint32_t baseAddr, uint8_t channel);

/*! @}*/

#endif /* __FSL_TPM_HAL_H__*/
/*******************************************************************************
 * EOF
 ******************************************************************************/

