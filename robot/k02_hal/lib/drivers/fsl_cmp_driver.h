/*
 * Copyright (c) 2013 - 2014, Freescale Semiconductor, Inc.
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

#ifndef __FSL_CMP_DRIVER_H__
#define __FSL_CMP_DRIVER_H__

#include <stdint.h>
#include <stdbool.h>
#include "fsl_cmp_hal.h"

/*!
 * @addtogroup cmp_driver
 * @{
 */

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*!
 * @brief Defines a structure for configuring the comparator in the CMP module.
 *
 * This structure holds the configuration for the comparator
 * inside the CMP module. With the configuration, the CMP can be set as a
 * basic comparator without additional features. 
 */
typedef struct CmpUserConfig
{
    cmp_hystersis_mode_t hystersisMode; /*!< Set the hysteresis level. */
    bool pinoutEnable; /*!< Enable outputting the CMPO to pin. */
    bool pinoutUnfilteredEnable; /*!< Enable outputting unfiltered result to CMPO. */
    bool invertEnable; /*!< Enable inverting the comparator's result. */
    bool highSpeedEnable; /*!< Enable working in speed mode. */
#if FSL_FEATURE_CMP_HAS_DMA
    bool dmaEnable; /*!< Enable using DMA. */
#endif /* FSL_FEATURE_CMP_HAS_DMA */
    bool risingIntEnable; /*!< Enable using CMPO rising interrupt. */
    bool fallingIntEnable; /*!< Enable using CMPO falling interrupt. */
    cmp_chn_mux_mode_t plusChnMux; /*!< Set the Plus side input to comparator. */
    cmp_chn_mux_mode_t minusChnMux; /*!< Set the Minus side input to comparator. */
#if FSL_FEATURE_CMP_HAS_TRIGGER_MODE
    bool triggerEnable; /*!< Enable triggering mode.  */
#endif /* FSL_FEATURE_CMP_HAS_TRIGGER_MODE */
#if FSL_FEATURE_CMP_HAS_PASS_THROUGH_MODE
    bool passThroughEnable;  /*!< Enable using pass through mode. */
#endif /* FSL_FEATURE_CMP_HAS_PASS_THROUGH_MODE */
    
} cmp_user_config_t;

/*!
* @brief Definition selections of sample and filter mode in the CMP module.
*
* Comparator sample/filter is available in several modes. Use the enumeration
* to identify the comparator's status:
*
* kCmpContinuousMode - Continuous Mode:
        Both window control and filter blocks are completely bypassed. The
        comparator output is updated continuously. 
* kCmpSampleWithNoFilteredMode - Sample, Non-Filtered Mode:
        Window control is completely bypassed. The comparator output  is
        sampled whenever a rising-edge is detected on the filter block clock
        input. The filter clock prescaler can be configured as a
        divider from the bus clock.
* kCmpSampleWithFilteredMode - Sample, Filtered Mode:
        Similar to "Sample, Non-Filtered Mode", but the filter is active in
        this mode. The filter counter value also becomes
        configurable.
* kCmpWindowedMode - Windowed Mode:
        In Windowed Mode, only output of analog comparator is passed when
        the WINDOW signal is high. The last latched value is held when the WINDOW
        signal is low.
* kCmpWindowedFilteredMode - Window/Filtered Mode:
        This is a complex mode because it uses both window and filtering
        features. It also has the highest latency of all modes. This can be 
        approximated to up to 1 bus clock synchronization in the window function
        + ( ( filter counter * filter prescaler ) + 1) bus clock for the
        filter function.
*/
typedef enum _cmp_sample_filter_mode
{
    kCmpContinuousMode = 0U, /*!< Continuous Mode */
    kCmpSampleWithNoFilteredMode = 1U, /*!< Sample, Non-Filtered Mode */
    kCmpSampleWithFilteredMode = 2U, /*!< Sample, Filtered Mode */
    kCmpWindowedMode = 3U, /*!< Window Mode */
    kCmpWindowedFilteredMode = 4U /*!< Window/Filtered Mode */
} cmp_sample_filter_mode_t;

/*!
* @brief Defines a structure to configure the Window/Filter in CMP module.
*
* This structure holds the configuration for the Window/Filter inside
* the CMP module. With the configuration, the CMP module can operate in 
* advanced mode.
*/
typedef struct CmpSampleFilterConfig
{
    cmp_sample_filter_mode_t workMode; /*!< Sample/Filter's work mode. */
    bool useExtSampleOrWindow; /*!< Switcher to use external WINDOW/SAMPLE signal. */
    uint8_t filterClkDiv; /*!< Filter's prescaler which divides from the bus clock.  */
    cmp_filter_counter_mode_t filterCount; /*!< Sample count for filter. See "cmp_filter_counter_mode_t". */
} cmp_sample_filter_config_t;

/*!
 * @brief Defines a structure to configure the internal DAC in CMP module.
 *
 * This structure holds the configuration for DAC
 * inside the CMP module. With the configuration, the internal DAC
 * provides a reference voltage level and is chosen as the CMP input.
 */
typedef struct CmpDacConfig
{
    cmp_dac_ref_volt_src_mode_t refVoltSrcMode; /*!< Select the reference voltage source for internal DAC. */
    uint8_t dacValue; /*!< Set the value for internal DAC. */
} cmp_dac_config_t;

/*!
 * @brief Defines type of flags for the CMP event.
 */
typedef enum _cmp_flag
{
    kCmpFlagOfCoutRising  = 0U, /*!< Identifier to indicate if the COUT change from logic zero to one. */
    kCmpFlagOfCoutFalling = 1U  /*!< Identifier to indicate if the COUT change from logic one to zero. */
} cmp_flag_t;

/*!
 * @brief Internal driver state information.
 *
 * The contents of this structure are internal to the driver and should not be
 *  modified by users. Also, contents of the structure are subject to change in
 *  future releases.
 */
typedef struct CmpState
{
    bool isInUsed; /* If the CMP instance is in use. All the CMP instances share
        * the same clock gate and are aligned to use clock.*/
} cmp_state_t;

/*! @brief Table of base addresses for CMP instances. */
extern const uint32_t g_cmpBaseAddr[];

/*! @brief Table to save CMP IRQ enumeration numbers defined in CMSIS header file. */
extern const IRQn_Type g_cmpIrqId[HW_CMP_INSTANCE_COUNT];

#if defined(__cplusplus)
extern "C" {
#endif

/*******************************************************************************
 * APIs
 ******************************************************************************/

/*!
 * @brief Populates the initial user configuration with default settings. 
 *
 * This function populates the initial user configuration with default settings. 
 * The default settings enable the CMP module to operate as  a comparator.
 * The settings are :\n
 *     \li.hystersisMode = kCmpHystersisOfLevel0
 *     \li.pinoutEnable = true
 *     \li.pinoutUnfilteredEnable = true
 *     \li.invertEnable = false
 *     \li.highSpeedEnable = false
 *     \li.dmaEnable = false
 *     \li.risingIntEnable = false
 *     \li.fallingIntEnable = false
 *     \li.triggerEnable = false
 * However, it is still recommended to fill some fields of structure such as
 * channel mux according to the application requirements. Note that this function does not set the
 * configuration to hardware.
 *
 * @param userConfigPtr Pointer to structure of configuration. See "cmp_user_config_t".
 * @param plusInput Plus Input mux selection. See  "cmp_chn_mux_mode_t".
 * @param minusInput Minus Input mux selection. See  "cmp_chn_mux_mode_t".
 * @return Execution status.
 */
cmp_status_t CMP_DRV_StructInitUserConfigDefault(cmp_user_config_t *userConfigPtr,
    cmp_chn_mux_mode_t plusInput, cmp_chn_mux_mode_t minusInput);

/*!
 * @brief Initializes the CMP module. 
 *
 * This function initializes the CMP module, enables the clock and
 * sets the interrupt switcher. The CMP module is
 * configured as a basic comparator.
 *
 * @param instance CMP instance ID.
 * @param userConfigPtr Pointer to structure of configuration. See  "cmp_user_config_t".
 * @param userStatePtr Pointer to structure of context. See  "cmp_state_t".
 * @return Execution status.
 */
cmp_status_t CMP_DRV_Init(uint32_t instance, cmp_user_config_t *userConfigPtr,
    cmp_state_t *userStatePtr);

/*!
 * @brief De-initializes the CMP module. 
 *
 * This function de-initializes the CMP module. It  shuts down the CMP
 * clock and disables the interrupt. This API should be called when CMP is no
 * longer used in the application. It also reduces power consumption.
 *
 * @param instance CMP instance ID.
 */
void CMP_DRV_Deinit(uint32_t instance);

/*!
 * @brief Starts the CMP module. 
 *
 * This function starts the CMP module. The configuration does not take
 * effect until the module is started.
 *
 * @param instance CMP instance ID.
 */
void CMP_DRV_Start(uint32_t instance);

/*!
 * @brief Stops the CMP module. 
 *
 * This function stops the CMP module. Note that this function does not shut down
 * the module, but only pauses the features.
 *
 * @param instance CMP instance ID.
 */
void CMP_DRV_Stop(uint32_t instance);

/*!
 * @brief Enables the internal DAC in the CMP module. 
 *
 * This function enables the internal DAC in the CMP module. It takes
 * effect only when the internal DAC has been chosen as an input
 * channel for the comparator. Then, the DAC channel can be programmed to provide
 * a reference voltage level.
 *
 * @param instance CMP instance ID.
 * @param dacConfigPtr Pointer to structure of configuration. See "cmp_dac_config_t".
 * @return Execution status.
 */
cmp_status_t CMP_DRV_EnableDac(uint32_t instance, cmp_dac_config_t *dacConfigPtr);

/*!
 * @brief Disables the internal DAC in the CMP module. 
 *
 * This function disables the internal DAC in the CMP module. It should be
 * called if the internal DAC is no longer used by the application.
 *
 * @param instance CMP instance ID.
 */
void CMP_DRV_DisableDac(uint32_t instance);

/*!
 * @brief Configures the Sample\Filter feature in the CMP module. 
 *
 * This function configures the CMP working in Sample\Filter modes. These
 * modes are advanced features in addition to the basic comparator such as 
 * Window Mode, Filter Mode, etc. See  
 * "cmp_sample_filter_config_t"for detailed description.
 *
 * @param instance CMP instance ID.
 * @param configPtr Pointer to a structure of configurations.
 *        See "cmp_sample_filter_config_t".
 * @return Execution status.
 */
cmp_status_t CMP_DRV_ConfigSampleFilter(uint32_t instance, cmp_sample_filter_config_t *configPtr);

/*!
 * @brief Gets the output of the CMP module. 
 *
 * This function gets the output of the CMP module.
 * The output source depends on the configuration when initializing the comparator.
 * When the cmp_user_config_t.pinoutUnfilteredEnable is false, the output is
 * processed by the filter. Otherwise, the output is that the signal did not pass
 * the filter.
 *
 * @param instance CMP instance ID.
 * @return Output logic's assertion. When not inverted, plus side > minus side, it is true.
 */
bool CMP_DRV_GetOutput(uint32_t instance);

/*!
 * @brief Gets the state of the CMP module. 
 *
 * This function  gets the state of the CMP module. It returns if the indicated
 * event has been detected.
 *
 * @param instance CMP instance ID.
 * @param flag Represent events or states. See "cmp_flag_t".
 * @return Assertion if indicated event occurs.
 */
bool CMP_DRV_GetFlag(uint32_t instance, cmp_flag_t flag);

/*!
 * @brief Clears the event record of the CMP module. 
 *
 * This function clears the event record of the CMP module. 
 *
 * @param instance CMP instance ID.
 * @param flag Represent events or states. See "cmp_flag_t".
 */
void CMP_DRV_ClearFlag(uint32_t instance, cmp_flag_t flag);

#if defined(__cplusplus)
extern }
#endif

/*!
 *@}
 */

#endif /* __FSL_CMP_DRIVER_H__ */
/******************************************************************************
 * EOF
 *****************************************************************************/
 
