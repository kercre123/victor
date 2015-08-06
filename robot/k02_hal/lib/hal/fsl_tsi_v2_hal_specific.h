/*
 * Copyright (c) 2013, Freescale Semiconductor, Inc.
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
#ifndef __FSL_TSI_V2_HAL_SPECIFIC_H__
#define __FSL_TSI_V2_HAL_SPECIFIC_H__

#include <stdint.h>
#include "fsl_device_registers.h"
#include "fsl_tsi_hal.h"

// Just for right generation of documentation
#if defined(__DOXYGEN__)
  #define  FSL_FEATURE_TSI_VERSION 1
#endif

/*!
 * @addtogroup tsi_hal
 * @{
 */


/*! @file*/

/*******************************************************************************
 * Definitions
 ******************************************************************************/


/*!
 * @brief TSI low power scan intervals.
 *
 * These constants define the tsi low power scan intervals in a TSI instance.
 */
typedef enum _tsi_low_power_interval {
    kTsiLowPowerInterval_1ms = 0,   /*!< 1ms scan interval */
    kTsiLowPowerInterval_5ms = 1,   /*!< 5ms scan interval */
    kTsiLowPowerInterval_10ms = 2,  /*!< 10ms scan interval */
    kTsiLowPowerInterval_15ms = 3,  /*!< 15ms scan interval */
    kTsiLowPowerInterval_20ms = 4,  /*!< 20ms scan interval */
    kTsiLowPowerInterval_30ms = 5,  /*!< 30ms scan interval */
    kTsiLowPowerInterval_40ms = 6,  /*!< 40ms scan interval */
    kTsiLowPowerInterval_50ms = 7,  /*!< 50ms scan interval */
    kTsiLowPowerInterval_75ms = 8,  /*!< 75ms scan interval */
    kTsiLowPowerInterval_100ms = 9, /*!< 100ms scan interval */
    kTsiLowPowerInterval_125ms = 10, /*!< 125ms scan interval */
    kTsiLowPowerInterval_150ms = 11, /*!< 150ms scan interval */
    kTsiLowPowerInterval_200ms = 12, /*!< 200ms scan interval */
    kTsiLowPowerInterval_300ms = 13, /*!< 300ms scan interval */
    kTsiLowPowerInterval_400ms = 14, /*!< 400ms scan interval */
    kTsiLowPowerInterval_500ms = 15, /*!< 500ms scan interval */
} tsi_low_power_interval_t;

/*!
 * @brief TSI Reference oscillator charge current select.
 *
 * These constants define the tsi Reference oscillator charge current select in a TSI instance.
 */
typedef enum _tsi_reference_osc_charge_current {
    kTsiRefOscChargeCurrent_1uA = 0,        /*!< Reference oscillator charge current is 1uA */
    kTsiRefOscChargeCurrent_2uA = 1,        /*!< Reference oscillator charge current is 2uA */
    kTsiRefOscChargeCurrent_3uA = 2,        /*!< Reference oscillator charge current is 3uA */
    kTsiRefOscChargeCurrent_4uA = 3,        /*!< Reference oscillator charge current is 4uA */
    kTsiRefOscChargeCurrent_5uA = 4,        /*!< Reference oscillator charge current is 5uA */
    kTsiRefOscChargeCurrent_6uA = 5,        /*!< Reference oscillator charge current is 6uA */
    kTsiRefOscChargeCurrent_7uA = 6,        /*!< Reference oscillator charge current is 7uA */
    kTsiRefOscChargeCurrent_8uA = 7,        /*!< Reference oscillator charge current is 8uA */
    kTsiRefOscChargeCurrent_9uA = 8,        /*!< Reference oscillator charge current is 9uA */
    kTsiRefOscChargeCurrent_10uA = 9,       /*!< Reference oscillator charge current is 10uA */
    kTsiRefOscChargeCurrent_11uA = 10,      /*!< Reference oscillator charge current is 11uA */
    kTsiRefOscChargeCurrent_12uA = 11,      /*!< Reference oscillator charge current is 12uA */
    kTsiRefOscChargeCurrent_13uA = 12,      /*!< Reference oscillator charge current is 13uA */
    kTsiRefOscChargeCurrent_14uA = 13,      /*!< Reference oscillator charge current is 14uA */
    kTsiRefOscChargeCurrent_15uA = 14,      /*!< Reference oscillator charge current is 15uA */
    kTsiRefOscChargeCurrent_16uA = 15,      /*!< Reference oscillator charge current is 16uA */
    kTsiRefOscChargeCurrent_17uA = 16,      /*!< Reference oscillator charge current is 17uA */
    kTsiRefOscChargeCurrent_18uA = 17,      /*!< Reference oscillator charge current is 18uA */
    kTsiRefOscChargeCurrent_19uA = 18,      /*!< Reference oscillator charge current is 19uA */
    kTsiRefOscChargeCurrent_20uA = 19,      /*!< Reference oscillator charge current is 20uA */
    kTsiRefOscChargeCurrent_21uA = 20,      /*!< Reference oscillator charge current is 21uA */
    kTsiRefOscChargeCurrent_22uA = 21,      /*!< Reference oscillator charge current is 22uA */
    kTsiRefOscChargeCurrent_23uA = 22,      /*!< Reference oscillator charge current is 23uA */
    kTsiRefOscChargeCurrent_24uA = 23,      /*!< Reference oscillator charge current is 24uA */
    kTsiRefOscChargeCurrent_25uA = 24,      /*!< Reference oscillator charge current is 25uA */
    kTsiRefOscChargeCurrent_26uA = 25,      /*!< Reference oscillator charge current is 26uA */
    kTsiRefOscChargeCurrent_27uA = 26,      /*!< Reference oscillator charge current is 27uA */
    kTsiRefOscChargeCurrent_28uA = 27,      /*!< Reference oscillator charge current is 28uA */
    kTsiRefOscChargeCurrent_29uA = 28,      /*!< Reference oscillator charge current is 29uA */
    kTsiRefOscChargeCurrent_30uA = 29,      /*!< Reference oscillator charge current is 30uA */
    kTsiRefOscChargeCurrent_31uA = 30,      /*!< Reference oscillator charge current is 31uA */
    kTsiRefOscChargeCurrent_32uA = 31,      /*!< Reference oscillator charge current is 32uA */
} tsi_reference_osc_charge_current_t;

/*!
 * @brief TSI Reference oscillator charge current select limits.
 *
 * These constants define the limits of the TSI Reference oscillator charge current select in a TSI instance.
 */
typedef struct _tsi_reference_osc_charge_current_limits
{
  tsi_reference_osc_charge_current_t    upper;  /*!< Reference oscillator charge current upper limit */
  tsi_reference_osc_charge_current_t    lower;  /*!< Reference oscillator charge current lower limit */
}tsi_reference_osc_charge_current_limits_t;

/*!
 * @brief TSI External oscillator charge current select.
 *
 * These constants define the tsi External oscillator charge current select in a TSI instance.
 */
typedef enum _tsi_external_osc_charge_current {
    kTsiExtOscChargeCurrent_1uA = 0,        /*!< External oscillator charge current is 1uA */
    kTsiExtOscChargeCurrent_2uA = 1,        /*!< External oscillator charge current is 2uA */
    kTsiExtOscChargeCurrent_3uA = 2,        /*!< External oscillator charge current is 3uA */
    kTsiExtOscChargeCurrent_4uA = 3,        /*!< External oscillator charge current is 4uA */
    kTsiExtOscChargeCurrent_5uA = 4,        /*!< External oscillator charge current is 5uA */
    kTsiExtOscChargeCurrent_6uA = 5,        /*!< External oscillator charge current is 6uA */
    kTsiExtOscChargeCurrent_7uA = 6,        /*!< External oscillator charge current is 7uA */
    kTsiExtOscChargeCurrent_8uA = 7,        /*!< External oscillator charge current is 8uA */
    kTsiExtOscChargeCurrent_9uA = 8,        /*!< External oscillator charge current is 9uA */
    kTsiExtOscChargeCurrent_10uA = 9,       /*!< External oscillator charge current is 10uA */
    kTsiExtOscChargeCurrent_11uA = 10,      /*!< External oscillator charge current is 11uA */
    kTsiExtOscChargeCurrent_12uA = 11,      /*!< External oscillator charge current is 12uA */
    kTsiExtOscChargeCurrent_13uA = 12,      /*!< External oscillator charge current is 13uA */
    kTsiExtOscChargeCurrent_14uA = 13,      /*!< External oscillator charge current is 14uA */
    kTsiExtOscChargeCurrent_15uA = 14,      /*!< External oscillator charge current is 15uA */
    kTsiExtOscChargeCurrent_16uA = 15,      /*!< External oscillator charge current is 16uA */
    kTsiExtOscChargeCurrent_17uA = 16,      /*!< External oscillator charge current is 17uA */
    kTsiExtOscChargeCurrent_18uA = 17,      /*!< External oscillator charge current is 18uA */
    kTsiExtOscChargeCurrent_19uA = 18,      /*!< External oscillator charge current is 19uA */
    kTsiExtOscChargeCurrent_20uA = 19,      /*!< External oscillator charge current is 20uA */
    kTsiExtOscChargeCurrent_21uA = 20,      /*!< External oscillator charge current is 21uA */
    kTsiExtOscChargeCurrent_22uA = 21,      /*!< External oscillator charge current is 22uA */
    kTsiExtOscChargeCurrent_23uA = 22,      /*!< External oscillator charge current is 23uA */
    kTsiExtOscChargeCurrent_24uA = 23,      /*!< External oscillator charge current is 24uA */
    kTsiExtOscChargeCurrent_25uA = 24,      /*!< External oscillator charge current is 25uA */
    kTsiExtOscChargeCurrent_26uA = 25,      /*!< External oscillator charge current is 26uA */
    kTsiExtOscChargeCurrent_27uA = 26,      /*!< External oscillator charge current is 27uA */
    kTsiExtOscChargeCurrent_28uA = 27,      /*!< External oscillator charge current is 28uA */
    kTsiExtOscChargeCurrent_29uA = 28,      /*!< External oscillator charge current is 29uA */
    kTsiExtOscChargeCurrent_30uA = 29,      /*!< External oscillator charge current is 30uA */
    kTsiExtOscChargeCurrent_31uA = 30,      /*!< External oscillator charge current is 31uA */
    kTsiExtOscChargeCurrent_32uA = 31,      /*!< External oscillator charge current is 32uA */
} tsi_external_osc_charge_current_t;

/*!
 * @brief TSI External oscillator charge current select limits.
 *
 * These constants define the limits of the TSI External oscillator charge current select in a TSI instance.
 */
typedef struct _tsi_external_osc_charge_current_limits
{
  tsi_external_osc_charge_current_t    upper;  /*!< External oscillator charge current upper limit */
  tsi_external_osc_charge_current_t    lower;  /*!< External oscillator charge current lower limit */
}tsi_external_osc_charge_current_limits_t;

/*!
 * @brief TSI Internal capacitance trim value.
 *
 * These constants define the tsi Internal capacitance trim value in a TSI instance.
 */
typedef enum _tsi_internal_cap_trim {
    kTsiIntCapTrim_0_5pF = 0,        /*!< 0.5 pF internal reference capacitance */
    kTsiIntCapTrim_0_6pF = 1,        /*!< 0.6 pF internal reference capacitance */
    kTsiIntCapTrim_0_7pF = 2,        /*!< 0.7 pF internal reference capacitance */
    kTsiIntCapTrim_0_8pF = 3,        /*!< 0.8 pF internal reference capacitance */
    kTsiIntCapTrim_0_9pF = 4,        /*!< 0.9 pF internal reference capacitance */
    kTsiIntCapTrim_1_0pF = 5,        /*!< 1.0 pF internal reference capacitance */
    kTsiIntCapTrim_1_1pF = 6,        /*!< 1.1 pF internal reference capacitance */
    kTsiIntCapTrim_1_2pF = 7,        /*!< 1.2 pF internal reference capacitance */
} tsi_internal_cap_trim_t;

/*!
 * @brief TSI Delta voltage applied to analog oscillators.
 *
 * These constants define the tsi Delta voltage applied to analog oscillators in a TSI instance.
 */
typedef enum _tsi_osc_delta_voltage {
    kTsiOscDeltaVoltage_100mV = 0,        /*!< 100 mV delta voltage is applied */
    kTsiOscDeltaVoltage_150mV = 1,        /*!< 150 mV delta voltage is applied */
    kTsiOscDeltaVoltage_200mV = 2,        /*!< 200 mV delta voltage is applied */
    kTsiOscDeltaVoltage_250mV = 3,        /*!< 250 mV delta voltage is applied */
    kTsiOscDeltaVoltage_300mV = 4,        /*!< 300 mV delta voltage is applied */
    kTsiOscDeltaVoltage_400mV = 5,        /*!< 400 mV delta voltage is applied */
    kTsiOscDeltaVoltage_500mV = 6,        /*!< 500 mV delta voltage is applied */
    kTsiOscDeltaVoltage_600mV = 7,        /*!< 600 mV delta voltage is applied */
} tsi_osc_delta_voltage_t;

/*!
 * @brief TSI Active mode clock divider.
 *
 * These constants define the active mode clock divider in a TSI instance.
 */
typedef enum _tsi_active_mode_clock_divider {
    kTsiActiveClkDiv_1div = 0,          /*!< Active mode clock divider is set to 1 */
    kTsiActiveClkDiv_2048div = 1,       /*!< Active mode clock divider is set to 2048 */
} tsi_active_mode_clock_divider_t;

/*!
 * @brief TSI Active mode clock source.
 *
 * These constants define the active mode clock source in a TSI instance.
 */
typedef enum _tsi_active_mode_clock_source {
    kTsiActiveClkSource_BusClock = 0,      /*!< Active mode clock source is set to Bus Clock */
    kTsiActiveClkSource_MCGIRCLK = 1,      /*!< Active mode clock source is set to MCG Internal reference clock */
    kTsiActiveClkSource_OSCERCLK = 2,      /*!< Active mode clock source is set to System oscillator output */
} tsi_active_mode_clock_source_t;

/*!
 * @brief TSI active mode prescaler.
 *
 * These constants define the tsi active mode prescaler in a TSI instance.
 */
typedef enum _tsi_active_mode_prescaler {
    kTsiActiveModePrescaler_1div = 0,          /*!< Input clock source divided by 1 */
    kTsiActiveModePrescaler_2div = 1,          /*!< Input clock source divided by 2 */
    kTsiActiveModePrescaler_4div = 2,          /*!< Input clock source divided by 4 */
    kTsiActiveModePrescaler_8div = 3,          /*!< Input clock source divided by 8 */
    kTsiActiveModePrescaler_16div = 4,         /*!< Input clock source divided by 16 */
    kTsiActiveModePrescaler_32div = 5,         /*!< Input clock source divided by 32 */
    kTsiActiveModePrescaler_64div = 6,         /*!< Input clock source divided by 64 */
    kTsiActiveModePrescaler_128div = 7,        /*!< Input clock source divided by 128 */
} tsi_active_mode_prescaler_t;

/*!
* @brief TSI active mode prescaler limits.
*
* These constants define the limits of the TSI active mode prescaler in a TSI instance.
*/
typedef struct _tsi_active_mode_prescaler_limits {
  tsi_active_mode_prescaler_t upper;        /*!< Input clock source prescaler upper limit */
  tsi_active_mode_prescaler_t lower;        /*!< Input clock source prescaler lower limit */
}tsi_active_mode_prescaler_limits_t;

/*!
* @brief TSI operation mode limits
*
* These constants is used to specify the valid range of settings for the recalibration process of TSI parameters
*/
typedef struct _tsi_parameter_limits {
  tsi_n_consecutive_scans_limits_t              consNumberOfScan;       /*!< number of consecutive scan limits */
  tsi_reference_osc_charge_current_limits_t     refOscChargeCurrent;    /*!< Reference oscillator charge current limits */
  tsi_external_osc_charge_current_limits_t      extOscChargeCurrent;    /*!< External oscillator charge current limits */
  tsi_active_mode_prescaler_limits_t            activeModePrescaler;    /*!< Input clock source prescaler limits */
}tsi_parameter_limits_t;


#if (FSL_FEATURE_TSI_VERSION == 1)
/*!
 * @brief TSI configuration structure.
 *
 * This structure contains the settings for the most common TSI configurations including
 * the TSI module charge currents, number of scans, thresholds, trimming etc.
 */
typedef struct TsiConfig {
    tsi_electrode_osc_prescaler_t ps;       /*!< Prescaler */
    tsi_external_osc_charge_current_t extchrg;  /*!< Electrode charge current */
    tsi_reference_osc_charge_current_t refchrg;  /*!< Reference charge current */
    tsi_n_consecutive_scans_t nscn;     /*!< Number of scans. */
    uint8_t lpclks;   /*!< Low power clock. */
    tsi_active_mode_clock_source_t amclks;   /*!< Active mode clock source. */
    tsi_active_mode_clock_divider_t amclkdiv; /*!< Active mode prescaler. */
    tsi_active_mode_prescaler_t ampsc;    /*!< Active mode prescaler. */
    tsi_low_power_interval_t lpscnitv; /*!< Low power scan interval. */
    tsi_osc_delta_voltage_t delvol;   /*!< Delta voltage. */
    tsi_internal_cap_trim_t captrm;   /*!< Internal capacitence trimmer. */
    uint16_t thresh;   /*!< High threshold. */
    uint16_t thresl;   /*!< Low threshold. */
}tsi_config_t;

#elif (FSL_FEATURE_TSI_VERSION == 2)
/*!
 * @brief TSI configuration structure.
 *
 * This structure contains the settings for the most common TSI configurations including
 * the TSI module charge currents, number of scans, thresholds, trimming etc.
 */
typedef struct TsiConfig {
    tsi_electrode_osc_prescaler_t ps;       /*!< Prescaler */
    tsi_external_osc_charge_current_t extchrg;  /*!< Electrode charge current */
    tsi_reference_osc_charge_current_t refchrg;  /*!< Reference charge current */
    tsi_n_consecutive_scans_t nscn;     /*!< Number of scans. */
    uint8_t lpclks;   /*!< Low power clock. */
    tsi_active_mode_clock_source_t amclks;   /*!< Active mode clock source. */
    tsi_active_mode_prescaler_t ampsc;    /*!< Active mode prescaler. */
    tsi_low_power_interval_t lpscnitv; /*!< Low power scan interval. */
    uint16_t thresh;   /*!< High threshold. */
    uint16_t thresl;   /*!< Low threshold. */
}tsi_config_t;

#else
#error TSI version not supported.
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*!
* @brief Enable periodical (hardware) trigger scan.
*
* @param    baseAddr TSI module base address.
* @return   None.
*/
static inline void TSI_HAL_EnablePeriodicalScan(uint32_t baseAddr)
{
    BW_TSI_GENCS_STM(baseAddr, 1);
}

/*!
* @brief Enable error interrupt.
*
* @param    baseAddr TSI module base address.
* @return   None.
*/
static inline void TSI_HAL_EnableErrorInterrupt(uint32_t baseAddr)
{
    BW_TSI_GENCS_ERIE(baseAddr, 1);
}

/*!
* @brief Disable error interrupt.
*
* @param    baseAddr TSI module base address.
* @return   None.
*/
static inline void TSI_HAL_DisableErrorInterrupt(uint32_t baseAddr)
{
    BW_TSI_GENCS_ERIE(baseAddr, 0);
}

/*!
* @brief Enable TSI module interrupt.
*
* @param    baseAddr TSI module base address.
* @return   None.
*/
static inline void TSI_HAL_EnableInterrupt(uint32_t baseAddr)
{
    BW_TSI_GENCS_TSIIE(baseAddr, 1);
}

/*!
* @brief Disable TSI interrupt.
*
* @param    baseAddr TSI module base address.
* @return   None.
*/
static inline void TSI_HAL_DisableInterrupt(uint32_t baseAddr)
{
    BW_TSI_GENCS_TSIIE(baseAddr, 0);
}

/*!
* @brief Get interrupt enable flag.
*
* @param    baseAddr TSI module base address.
* @return   State of enable interrupt flag.
*/
static inline uint32_t TSI_HAL_IsInterruptEnabled(uint32_t baseAddr)
{
    return BR_TSI_GENCS_TSIIE(baseAddr);
}

/*!
* @brief Start measurement (trigger the new measurement).
*
* @param    baseAddr TSI module base address.
* @return   None.
*/
static inline void TSI_HAL_StartSoftwareTrigger(uint32_t baseAddr)
{
    HW_TSI_GENCS_SET(baseAddr, BF_TSI_GENCS_SWTS(1));
}

/*!
* @brief Get overrun flag.
*
* @param    baseAddr TSI module base address.
* @return   State of over run flag.
*/
static inline uint32_t TSI_HAL_IsOverrun(uint32_t baseAddr)
{
    return (uint32_t)BR_TSI_GENCS_OVRF(baseAddr);
}

/*!
* @brief Clear over run flag
*
* @param    baseAddr TSI module base address.
* @return   None.
*/
static inline void TSI_HAL_ClearOverrunFlag(uint32_t baseAddr)
{
    BW_TSI_GENCS_OVRF(baseAddr, 1);
}

/*!
* @brief Get external electrode error flag.
*
* @param    baseAddr TSI module base address.
* @return   Stae of external electrode error flag
*/
static inline uint32_t TSI_HAL_GetExternalElectrodeErrorFlag(uint32_t baseAddr)
{
    return (uint32_t)BR_TSI_GENCS_EXTERF(baseAddr);
}

/*!
* @brief Clear external electrode error flag
*
* @param    baseAddr TSI module base address.
* @return   None.
*/
static inline void TSI_HAL_ClearExternalElectrodeErrorFlag(uint32_t baseAddr)
{
    BW_TSI_GENCS_EXTERF(baseAddr, 1);
}

/*!
* @brief Set low power scan interval.
*
* @param    baseAddr    TSI module base address.
* @param    interval    Interval for low power scan.
* @return   None.
*/
static inline void TSI_HAL_SetLowPowerScanInterval(uint32_t baseAddr, tsi_low_power_interval_t interval)
{
    BW_TSI_GENCS_LPSCNITV(baseAddr, interval);
}

/*!
* @brief Get low power scan interval.
*
* @param   baseAddr    TSI module base address.
* @return  Interval for low power scan.
*/
static inline tsi_low_power_interval_t TSI_HAL_GetLowPowerScanInterval(uint32_t baseAddr)
{
    return (tsi_low_power_interval_t)BR_TSI_GENCS_LPSCNITV(baseAddr);
}

/*!
* @brief Set low power clock.
*
* @param   baseAddr TSI module base address.
* @param clock Low power clock selection.
*/
static inline void TSI_HAL_SetLowPowerClock(uint32_t baseAddr, uint32_t clock)
{
    BW_TSI_GENCS_LPCLKS(baseAddr, clock);
}

/*!
* @brief Get low power clock.
*
* @param    baseAddr    TSI module base address.
* @return   Low power clock selection.
*/
static inline uint32_t TSI_HAL_GetLowPowerClock(uint32_t baseAddr)
{
    return BR_TSI_GENCS_LPCLKS(baseAddr);
}

/*!
* @brief Set the reference oscilator charge current.
*
* @param    baseAddr    TSI module base address.
* @param    current     The charge current.
* @return   None.
*/
static inline void TSI_HAL_SetReferenceChargeCurrent(uint32_t baseAddr, tsi_reference_osc_charge_current_t current)
{
    BW_TSI_SCANC_REFCHRG(baseAddr, current);
}

/*!
* @brief Get the reference oscilator charge current.
*
* @param    baseAddr    TSI module base address.
* @return   The charge current.
*/
static inline tsi_reference_osc_charge_current_t TSI_HAL_GetReferenceChargeCurrent(uint32_t baseAddr)
{
    return (tsi_reference_osc_charge_current_t)BR_TSI_SCANC_REFCHRG(baseAddr);
}

#if (FSL_FEATURE_TSI_VERSION == 1)
/*!
* @brief Set internal capacitance trim.
*
* @param    baseAddr    TSI module base address.
* @param    trim        Trim value.
* @return   None.
*/
static inline void TSI_HAL_SetInternalCapacitanceTrim(uint32_t baseAddr, tsi_internal_cap_trim_t trim)
{
    BW_TSI_SCANC_CAPTRM(baseAddr, trim);
}

/*!
* @brief Get internal capacitance trim.
*
* @param    baseAddr    TSI module base address.
* @return   Trim value.
*/
static inline tsi_internal_cap_trim_t TSI_HAL_GetInternalCapacitanceTrim(uint32_t baseAddr)
{
    return (tsi_internal_cap_trim_t)BR_TSI_SCANC_CAPTRM(baseAddr);
}

#endif

/*!
* @brief Set electrode charge current.
*
* @param    baseAddr    TSI module base address.
* @param    current     Electrode current.
* @return   None.
*/
static inline void TSI_HAL_SetElectrodeChargeCurrent(uint32_t baseAddr, tsi_external_osc_charge_current_t current)
{
    BW_TSI_SCANC_EXTCHRG(baseAddr, current);
}

/*!
* @brief Get electrode charge current.
*
* @param   baseAddr    TSI module base address.
* @return  Charge current.
*/
static inline tsi_external_osc_charge_current_t TSI_HAL_GetElectrodeChargeCurrent(uint32_t baseAddr)
{
    return (tsi_external_osc_charge_current_t)BR_TSI_SCANC_EXTCHRG(baseAddr);
}

#if (FSL_FEATURE_TSI_VERSION == 1)
/*!
* @brief Set delta voltage.
*
* @param    baseAddr    TSI module base address.
* @param    voltage     delta voltage.
* @return   None.
*/
static inline void TSI_HAL_SetDeltaVoltage(uint32_t baseAddr, uint32_t voltage)
{
    BW_TSI_SCANC_DELVOL(baseAddr, voltage);
}

/*!
* @brief Get delta voltage.
*
* @param    baseAddr    TSI module base address.
* @return   Delta voltage.
*/
static inline uint32_t TSI_HAL_GetDeltaVoltage(uint32_t baseAddr)
{
    return BR_TSI_SCANC_DELVOL(baseAddr);
}

#endif

/*!
* @brief Set scan modulo value.
*
* @param    baseAddr    TSI module base address.
* @param    modulo      Scan modulo value.
* @return   None.
*/
static inline void TSI_HAL_SetScanModulo(uint32_t baseAddr, uint32_t modulo)
{
    BW_TSI_SCANC_SMOD(baseAddr, modulo);
}

/*!
* @brief Get scan modulo value.
*
* @param    baseAddr    TSI module base address.
* @return   Scan modulo value.
*/
static inline uint32_t TSI_HAL_GetScanModulo(uint32_t baseAddr)
{
    return BR_TSI_SCANC_SMOD(baseAddr);
}

#if (FSL_FEATURE_TSI_VERSION == 1)
/*!
* @brief Set active mode clock divider.
*
* @param    baseAddr    TSI module base address.
* @param    divider     A value for divider.
* @return   None.
*/
static inline void TSI_HAL_SetActiveModeClockDivider(uint32_t baseAddr, uint32_t divider)
{
    BW_TSI_SCANC_AMCLKDIV(baseAddr, divider);
}

/*!
* @brief Get active mode clock divider.
*
* @param    baseAddr    TSI module base address.
* @return   A value for divider.
*/
static inline uint32_t TSI_HAL_GetActiveModeClockDivider(uint32_t baseAddr)
{
    return BR_TSI_SCANC_AMCLKDIV(baseAddr);
}
#endif

/*!
* @brief Set active mode source.
*
* @param    baseAddr    TSI module base address.
* @param    source      Active mode clock source (LPOSCCLK, MCGIRCLK, OSCERCLK).
* @return   None.
*/
static inline void TSI_HAL_SetActiveModeSource(uint32_t baseAddr, uint32_t source)
{
    BW_TSI_SCANC_AMCLKS(baseAddr, source);
}

/*!
* @brief Get active mode source.
*
* @param    baseAddr    TSI module base address.
* @return   Source value.
*/
static inline uint32_t TSI_HAL_GetActiveModeSource(uint32_t baseAddr)
{
    return BR_TSI_SCANC_AMCLKS(baseAddr);
}

/*!
* @brief Set active mode prescaler.
*
* @param    baseAddr    TSI module base address.
* @param    prescaler   Prescaler's value.
* @return   None.
*/
static inline void TSI_HAL_SetActiveModePrescaler(uint32_t baseAddr, tsi_active_mode_prescaler_t prescaler)
{
    BW_TSI_SCANC_AMPSC(baseAddr, prescaler);
}

/*!
* @brief Get active mode prescaler.
*
* @param    baseAddr    TSI module base address.
* @return   Prescaler's value.
*/
static inline uint32_t TSI_HAL_GetActiveModePrescaler(uint32_t baseAddr)
{
    return BR_TSI_SCANC_AMPSC(baseAddr);
}

/*!
* @brief Set low power channel. Only one channel can wake up MCU.
*
* @param    baseAddr    TSI module base address.
* @param    channel     Channel number.
* @return   None.
*/
static inline void TSI_HAL_SetLowPowerChannel(uint32_t baseAddr, uint32_t channel)
{
    assert(channel < FSL_FEATURE_TSI_CHANNEL_COUNT);  
    BW_TSI_PEN_LPSP(baseAddr, channel);
}

/*!
 * @brief Get low power channel. Only one channel can wake up MCU.
 *
 * @param   baseAddr TSI module base address.
 * @return Channel number.
 */
static inline uint32_t TSI_HAL_GetLowPowerChannel(uint32_t baseAddr)
{
    return BR_TSI_PEN_LPSP(baseAddr);
}

/*!
* @brief Enable channel.
*
* @param    baseAddr    TSI module base address.
* @param    channel     Channel to be enabled.
* @return   None.
*/
static inline void TSI_HAL_EnableChannel(uint32_t baseAddr, uint32_t channel)
{
    assert(channel < FSL_FEATURE_TSI_CHANNEL_COUNT);  
    HW_TSI_PEN_SET(baseAddr, (1U << channel));
}

/*!
* @brief Enable channels. The function enables channels by mask. It can set all
*          at once.
*
* @param    baseAddr        TSI module base address.
* @param    channelsMask    Channels mask to be enabled.
* @return   None.
*/
static inline void TSI_HAL_EnableChannels(uint32_t baseAddr, uint32_t channelsMask)
{
    HW_TSI_PEN_SET(baseAddr, (uint16_t)channelsMask);
}

/*!
* @brief Disable channel.
*
* @param    baseAddr    TSI module base address.
* @param    channel     Channel to be disabled.
* @return   None.
*/
static inline void TSI_HAL_DisableChannel(uint32_t baseAddr, uint32_t channel)
{
    HW_TSI_PEN_CLR(baseAddr, (1U << channel));
}

/*!
* @brief Disable channels. The function disables channels by mask. It can set all
*          at once.
*
* @param    baseAddr        TSI module base address.
* @param    channelsMask    Channels mask to be disabled.
* @return   None.
*/
static inline void TSI_HAL_DisableChannels(uint32_t baseAddr, uint32_t channelsMask)
{
    HW_TSI_PEN_CLR(baseAddr, channelsMask);
}

/*!
 * @brief Returns if channel is enabled.
 *
 * @param   baseAddr    TSI module base address.
 * @param   channel     Channel to be checked.
 *
 * @return True - if channel is enabled, false - otherwise.
 */
static inline uint32_t TSI_HAL_GetEnabledChannel(uint32_t baseAddr, uint32_t channel)
{
    assert(channel < FSL_FEATURE_TSI_CHANNEL_COUNT);  
    return (HW_TSI_PEN_RD(baseAddr) & (1U << channel));
}

/*!
* @brief Returns mask of enabled channels.
*
* @param    baseAddr    TSI module base address.
* @return   Channels mask that are enabled.
*/
static inline uint32_t TSI_HAL_GetEnabledChannels(uint32_t baseAddr)
{
    return (uint32_t)HW_TSI_PEN_RD(baseAddr);
}

/*!
* @brief Set the Wake up channel counter.
*
* @param    baseAddr    TSI module base address.
* @return   Wake up counter value.
*/
static inline uint16_t TSI_HAL_GetWakeUpChannelCounter(uint32_t baseAddr)
{
  return BR_TSI_WUCNTR_WUCNT(baseAddr);
}

/*!
* @brief Get tsi counter on actual channel.
*
* @param    baseAddr    TSI module base address.
* @param    channel     Index of TSI channel.
*
* @return   The counter value.
*/
static inline uint32_t TSI_HAL_GetCounter(uint32_t baseAddr, uint32_t channel)
{
    assert(channel < FSL_FEATURE_TSI_CHANNEL_COUNT);  
    uint16_t *counter =  (uint16_t *)(HW_TSI_CNTR1_ADDR(baseAddr) + (channel * 2U));
    return (uint32_t)(*counter);
}

/*!
* @brief Set low threshold.
*
* @param    baseAddr        TSI module base address.
* @param    low_threshold   Low counter threshold.
* @return   None.
*/
static inline void TSI_HAL_SetLowThreshold(uint32_t baseAddr, uint32_t low_threshold)
{
    BW_TSI_THRESHOLD_LTHH(baseAddr, low_threshold);
}

/*!
* @brief Set high threshold.
*
* @param    baseAddr        TSI module base address.
* @param    high_threshold  High counter threshold.
* @return   None.
*/
static inline void TSI_HAL_SetHighThreshold(uint32_t baseAddr, uint32_t high_threshold)
{
    BW_TSI_THRESHOLD_HTHH(baseAddr, high_threshold);
}

#ifdef __cplusplus
}
#endif


/*! @}*/

#endif /* __FSL_TSI_V2_HAL_SPECIFIC_H__*/
/*******************************************************************************
 * EOF
 ******************************************************************************/

