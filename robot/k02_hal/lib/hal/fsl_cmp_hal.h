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

#ifndef __FSL_CMP_HAL_H__
#define __FSL_CMP_HAL_H__

#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include "fsl_device_registers.h"

/*!
 * @addtogroup cmp_hal
 * @{
 */

/******************************************************************************
 * Definitions
 *****************************************************************************/

/*!
 * @brief CMP status return codes.
 */
typedef enum _cmp_status 
{
    kStatus_CMP_Success         = 0U, /*!< Success */
    kStatus_CMP_InvalidArgument = 1U, /*!< Invalid argument existed */
    kStatus_CMP_Failed          = 2U  /*!< Execution failed */
} cmp_status_t;

/*!
 * @brief Define the selections of hard block hystersis control level.
 * 
 * The hystersis control level indicates the smallest window between the two
 * input when asserting the change of output. it should be referred to the
 * Data Sheet for detail electrical characteristics. Generally, the lower level
 * represents the smaller window.
 */
typedef enum _cmp_hystersis_mode
{
    kCmpHystersisOfLevel0 = 0U, /*!< Level 0 */
    kCmpHystersisOfLevel1 = 1U, /*!< Level 1 */
    kCmpHystersisOfLevel2 = 2U, /*!< Level 2 */
    kCmpHystersisOfLevel3 = 3U  /*!< Level 3 */
} cmp_hystersis_mode_t;

/*!
 * @brief Define the selections of filter sample counter.
 * 
 * The selection item represents the number of consecutive samples that must
 * agree prior to the comparator output filter accepting a new output state.
 */
typedef enum _cmp_filter_counter_mode_t
{
    kCmpFilterCountSampleOf0 = 0U, /*!< Disable the filter */
    kCmpFilterCountSampleOf1 = 1U, /*!< One sample must agree */
    kCmpFilterCountSampleOf2 = 2U, /*!< 2 consecutive samples must agree */
    kCmpFilterCountSampleOf3 = 3U, /*!< 3 consecutive samples must agree */
    kCmpFilterCountSampleOf4 = 4U, /*!< 4 consecutive samples must agree */
    kCmpFilterCountSampleOf5 = 5U, /*!< 5 consecutive samples must agree */
    kCmpFilterCountSampleOf6 = 6U, /*!< 6 consecutive samples must agree */
    kCmpFilterCountSampleOf7 = 7U  /*!< 7 consecutive samples must agree */ 
} cmp_filter_counter_mode_t;

/*!
 * @brief Define the selections of reference voltage source for internal DAC.
 */
typedef enum _cmp_dac_ref_volt_src_mode_t
{
    kCmpDacRefVoltSrcOf1 = 0U, /*!< Vin1 - Vref_out */
    kCmpDacRefVoltSrcOf2 = 1U  /*!< Vin2 - Vdd */
} cmp_dac_ref_volt_src_mode_t;

/*!
 * @brief Define the selection of CMP channel mux.
 */
typedef enum _cmp_chn_mux_mode_t
{
    kCmpInputChn0 = 0U, /*!< Comparator input channel 0. @internal gui name="Input 0" */
    kCmpInputChn1 = 1U, /*!< Comparator input channel 1. @internal gui name="Input 1" */
    kCmpInputChn2 = 2U, /*!< Comparator input channel 2. @internal gui name="Input 2" */
    kCmpInputChn3 = 3U, /*!< Comparator input channel 3. @internal gui name="Input 3" */
    kCmpInputChn4 = 4U, /*!< Comparator input channel 4. @internal gui name="Input 4" */
    kCmpInputChn5 = 5U, /*!< Comparator input channel 5. @internal gui name="Input 5" */
    kCmpInputChn6 = 6U, /*!< Comparator input channel 6. @internal gui name="Input 6" */
    kCmpInputChn7 = 7U, /*!< Comparator input channel 7. @internal gui name="Input 7" */
    kCmpInputChnDac = kCmpInputChn7 /*!< Comparator input channel 7. @internal gui name="DAC output" */
} cmp_chn_mux_mode_t;


/*******************************************************************************
 * APIs
 ******************************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif

/*!
 * @brief Reset CMP's registers to a known state.
 *
 * This function is to reset CMP's registers to a known state. This state is
 * defined in Reference Manual, which is power on reset value.
 *
 * @param baseAddr Register base address for the module.
 */
void CMP_HAL_Init(uint32_t baseAddr);

/*!
 * @brief Set the filter sample count.
 *
 * This function is to set the filter sample count. The value of filter sample
 * count represents the number of consecutive samples that must agree prior to
 * the comparator output filter accepting a new output state. 
 *
 * @param baseAddr Register base address for the module.
 * @param mode Filter count value mode, see to "cmp_filter_counter_mode_t".
 */
static inline void CMP_HAL_SetFilterCounterMode(uint32_t baseAddr, cmp_filter_counter_mode_t mode)
{
    BW_CMP_CR0_FILTER_CNT(baseAddr, (uint8_t)(mode));
}

/*!
 * @brief Set the programmable hystersis level.
 *
 * This function is to define the programmable hystersis level. The hystersis
 * values associated with each level are device-specific. See the Data Sheet of
 * the device for the exact values. Also, see to "cmp_hystersis_mode_t" for
 * some additional information.
 *
 * @param baseAddr Register base address for the module.
 * @param mode Hystersis level, see to "cmp_hysteresis_mode_t".
 */
static inline void CMP_HAL_SetHystersisMode(uint32_t baseAddr, cmp_hystersis_mode_t mode)
{
    BW_CMP_CR0_HYSTCTR(baseAddr, (uint8_t)(mode));
}

/*!
 * @brief Enable the comparator in CMP module.
 *
 * This function is to enable the comparator in CMP module. The analog 
 * comparator is the core component in CMP module. Only when it is enabled, all
 * the other functions for advanced features are meaningful.
 *
 * @param baseAddr Register base address for the module.
 */
static inline void CMP_HAL_Enable(uint32_t baseAddr)
{
    BW_CMP_CR1_EN(baseAddr, 1U);
}

/*!
 * @brief Disable the comparator in CMP module.
 *
 * This function is to Disable the comparator in CMP module. The analog 
 * comparator is the core component in CMP module. When the it is disabled, it
 * remains in the off state, and consumes no power. 
 *
 * @param baseAddr Register base address for the module.
 */
static inline void CMP_HAL_Disable(uint32_t baseAddr)
{
    BW_CMP_CR1_EN(baseAddr, 0U);
}

/*!
 * @brief Switch to enable the compare output signal connecting to pin.
 *
 * This function is to switch to enable the compare output signal connecting to
 * pin. The comparator output (CMPO) is driven out on the associated CMPO
 * output pin if the comparator owns the pin. 
 *
 * @param baseAddr Register base address for the module.
 * @param enable Switcher to enable the feature.
 */
static inline void CMP_HAL_SetOutputPinCmd(uint32_t baseAddr, bool enable)
{
    BW_CMP_CR1_OPE(baseAddr, (enable ? 1U : 0U) );
}

/*!
 * @brief Switch to enable the filter for output of compare logic in CMP module.
 *
 * This function is to switch to enable the filter for output of compare logic
 * in CMP module. When enabled, it would set the unfiltered comparator output
 * (CMPO) to equal COUT. When disabled, it would set the filtered comparator
 * output(CMPO) to equal COUTA.
 *
 * @param baseAddr Register base address for the module.
 * @param enable Switcher to enable the feature.
 */
static inline void CMP_HAL_SetUnfilteredOutCmd(uint32_t baseAddr, bool enable)
{
    BW_CMP_CR1_COS(baseAddr, (enable ? 1U : 0U) );
}

/*!
 * @brief Switch to enable the polarity of the analog comparator function.
 *
 * This function is to switch to enable the polarity of the analog comparator
 * function. When enabled, it would invert the comparator output logic.
 *
 * @param baseAddr Register base address for the module.
 * @param enable Switcher to enable the feature.
 */
static inline void CMP_HAL_SetInvertLogicCmd(uint32_t baseAddr, bool enable)
{
    BW_CMP_CR1_INV(baseAddr, (enable ? 1U : 0U) );
}

/*!
 * @brief Switch to enable the power (speed) comparison mode in CMP module.
 *
 * This function is to switch to enable the power (speed) comparison mode in
 * CMP module. When enabled, it would select the High-Speed (HS) comparison
 * mode. In this mode, CMP has faster output propagation delay and higher
 * current consumption.
 *
 * @param baseAddr Register base address for the module.
 * @param enable Switcher to enable the feature.
 */
static inline void CMP_HAL_SetHighSpeedCmd(uint32_t baseAddr, bool enable)
{
    BW_CMP_CR1_PMODE(baseAddr, (enable ? 1U : 0U) );
}

#if FSL_FEATURE_CMP_HAS_TRIGGER_MODE
/*!
 * @brief Switch to enable the trigger mode in CMP module.
 *
 * This function is to switch to enable the trigger mode in CMP module. CMP and
 * internal 6-bit DAC are configured to CMP Trigger mode when this function is
 * enabled. In addition, the CMP should be enabled. If the DAC is to be used as
 * a reference to the CMP, it should also be enabled. CMP Trigger mode depends
 * on an external timer resource to periodically enable the CMP and 6-bit DAC
 * in order to generate a triggered compare. Upon setting trigger mode, the CMP
 * and DAC are placed in a standby state until an external timer resource
 * trigger is received. See the chip configuration chapter in reference manual
 * for details about the external timer resource.
 *
 * @param baseAddr Register base address for the module.
 * @param enable Switcher to enable the feature.
 */
static inline void CMP_HAL_SetTriggerModeCmd(uint32_t baseAddr, bool enable)
{
    BW_CMP_CR1_TRIGM(baseAddr, (enable ? 1U : 0U) );
}
#endif /* FSL_FEATURE_CMP_HAS_TRIGGER_MODE */

#if FSL_FEATURE_CMP_HAS_WINDOW_MODE
/*!
 * @brief Switch to enable the window mode in CMP module.
 *
 * This function is to switch to enable the window mode in CMP module.
 * When any windowed mode is active, COUTA is clocked by the bus clock whenever
 * WINDOW signal is 1. The last latched value is held when WINDOW signal is 0.
 *
 * @param baseAddr Register base address for the module.
 * @param enable Switcher to enable the feature.
 */
static inline void CMP_HAL_SetWindowModeCmd(uint32_t baseAddr, bool enable)
{
    BW_CMP_CR1_WE(baseAddr, (enable ? 1U : 0U) );
}
#endif /* FSL_FEATURE_CMP_HAS_WINDOW_MODE */

/*!
 * @brief Switch to enable the sample mode in CMP module.
 *
 * This function is to switch to enable the sample mode in CMP module.
 * When any sampled mode is active, COUTA is sampled whenever a rising-edge of 
 * filter block clock input or WINDOW/SAMPLE signal is detected.
 *
 * @param baseAddr Register base address for the module.
 * @param enable Switcher to enable the feature.
 */
static inline void CMP_HAL_SetSampleModeCmd(uint32_t baseAddr, bool enable)
{
    BW_CMP_CR1_SE(baseAddr, (enable ? 1U : 0U) );
}

/*!
 * @brief Set the filter sample period in CMP module.
 *
 * This function is to set the filter sample period in CMP module.
 * The setting value specifies the sampling period, in bus clock cycles, of the
 * comparator output filter, when sample mode is disable. Setting the value to
 * 0x0 would disable the filter. This API has no effect when sample mode is
 * enabled. In that case, the external SAMPLE signal is used to determine the
 * sampling period.
 *
 * @param baseAddr Register base address for the module.
 * @param value Count of bus clock cycles for per sample.
 */
static inline void CMP_HAL_SetFilterPeriodValue(uint32_t baseAddr, uint8_t value)
{
    BW_CMP_FPR_FILT_PER(baseAddr, value);
}

/*!
 * @brief Get the comparator logic output in CMP module.
 *
 * This function is to get the comparator logic output in CMP module.
 * It would return the current value of the analog comparator output. The value
 * would be reset to 0 and read as de-assert value when the CMP module is
 * disable. When setting to invert mode, the comparator logic output would be
 * inverted as well.
 *
 * @param baseAddr Register base address for the module.
 * @return The logic output is assert or not.
 */
static inline bool CMP_HAL_GetOutputLogic(uint32_t baseAddr)
{
    return ( 1U == BR_CMP_SCR_COUT(baseAddr) );
}

/*!
 * @brief Get the logic output's falling edge event in CMP module.
 *
 * This function is to get the logic output's falling edge event in CMP module.
 * It would detect a falling-edge on COUT and return the assert state when 
 * falling-edge on COUT has occurred.
 *
 * @param baseAddr Register base address for the module.
 * @return The falling-edge on COUT has occurred or not.
 */
static inline bool CMP_HAL_GetOutputFallingFlag(uint32_t baseAddr)
{
    return ( 1U == BR_CMP_SCR_CFF(baseAddr) );
}

/*!
 * @brief Clear the logic output's falling edge event in CMP module.
 *
 * This function is to clear the logic output's falling edge event in CMP module.
 *
 * @param baseAddr Register base address for the module.
 */
static inline void CMP_HAL_ClearOutputFallingFlag(uint32_t baseAddr)
{
    BW_CMP_SCR_CFF(baseAddr, 1U);
}

/*!
 * @brief Get the logic output's rising edge event in CMP module.
 *
 * This function is to get the logic output's rising edge event in CMP module.
 * It would detect a rising-edge on COUT and return the assert state when 
 * rising-edge on COUT has occurred.
 *
 * @param baseAddr Register base address for the module.
 * @return The rising-edge on COUT has occurred or not.
 */
static inline bool CMP_HAL_GetOutputRisingFlag(uint32_t baseAddr)
{
    return ( 1U == BR_CMP_SCR_CFR(baseAddr) );
}

/*!
 * @brief Clear the logic output's rising edge event in CMP module.
 *
 * This function is to clear the logic output's rising edge event in CMP module.
 *
 * @param baseAddr Register base address for the module.
 */
static inline void CMP_HAL_ClearOutputRisingFlag(uint32_t baseAddr)
{
    BW_CMP_SCR_CFR(baseAddr, 1U);
}

/*!
 * @brief Switch to enable requesting interrupt when falling-edge on COUT has occurred.
 *
 * This function is to switch to enable requesting interrupt  when falling-edge
 * on COUT has occurred. When enabled, it would generate a interrupt request
 * when falling-edge on COUT has occurred.
 *
 * @param baseAddr Register base address for the module.
 * @param enable Switcher to enable the feature.
 */
static inline void CMP_HAL_SetOutputFallingIntCmd(uint32_t baseAddr, bool enable)
{
    BW_CMP_SCR_IEF(baseAddr, (enable ? 1U : 0U) );
}

/*!
 * @brief Get the switcher of interrupt request on COUT's falling-edge.
 *
 * This function is to get the switcher of interrupt request on COUT's
 * falling-edge. When it is assert, a interrupt request would be generated when
 * falling-edge on COUT has occurred.
 *
 * @param baseAddr Register base address for the module.
 * @return The status of switcher to enable the feature.
 */
static inline bool CMP_HAL_GetOutputFallingIntCmd(uint32_t baseAddr)
{
    return ( 1U == BR_CMP_SCR_IEF(baseAddr) );
}

/*!
 * @brief Get the switcher of interrupt request on COUT's rising-edge.
 *
 * This function is to get the switcher of interrupt request on COUT's
 * rising-edge. When it is assert, a interrupt request would be generated when
 * rising-edge on COUT has occurred.
 *
 * @param baseAddr Register base address for the module.
 * @param enable Switcher to enable the feature.
 */
static inline void CMP_HAL_SetOutputRisingIntCmd(uint32_t baseAddr, bool enable)
{
    BW_CMP_SCR_IER(baseAddr, (enable ? 1U : 0U) );
}

/*!
 * @brief Get the switcher of interrupt request on COUT's rising-edge.
 *
 * This function is to get the switcher of interrupt request on COUT's
 * rising-edge. When it is assert, a interrupt request would be generated when
 * rising-edge on COUT has occurred.
 *
 * @param baseAddr Register base address for the module.
 * @return The status of switcher to enable the feature.
 */
static inline bool CMP_HAL_GetOutputRisingIntCmd(uint32_t baseAddr)
{
    return ( 1U == BR_CMP_SCR_IER(baseAddr) );
}

#if FSL_FEATURE_CMP_HAS_DMA 
/*!
 * @brief Switch to enable DMA trigger in CMP module.
 *
 * This function is to switch to enable DMA trigger in CMP module. Then the event
 * of falling-edge or rising-edge on COUT occurs, the DMA transfer would be
 * triggered from the CMP module.
 *
 * @param baseAddr Register base address for the module.
 * @param enable Switcher to enable the feature.
 */
static inline void CMP_HAL_SetDmaCmd(uint32_t baseAddr, bool enable)
{
    BW_CMP_SCR_DMAEN(baseAddr, (enable ? 1U : 0U) );
}
#endif /* FSL_FEATURE_CMP_HAS_DMA  */

/*!
 * @brief Switch to enable internal 6-bit DAC in CMP module.
 *
 * This function is to switch to enable internal 6-bit DAC in CMP module. When
 * enabled, the internal 6-bit DAC could be used as an input channel to the
 * analog comparator.
 *
 * @param baseAddr Register base address for the module.
 * @param enable Switcher to enable the feature.
 */
static inline void CMP_HAL_SetDacCmd(uint32_t baseAddr, bool enable)
{
    BW_CMP_DACCR_DACEN(baseAddr, (enable ? 1U : 0U) );
}

/*!
 * @brief Set the reference voltage source for internal 6-bit DAC in CMP module.
 *
 * This function is to set the reference voltage source for internal 6-bit DAC
 * in CMP module.
 *
 * @param baseAddr Register base address for the module.
 * @param mode Selection of the feature, see to "cmp_dac_ref_volt_src_mode_t".
 */
static inline void CMP_HAL_SetDacRefVoltSrcMode(uint32_t baseAddr, cmp_dac_ref_volt_src_mode_t mode)
{
    BW_CMP_DACCR_VRSEL(baseAddr, (uint8_t)mode );
}

/*!
 * @brief Set the output value for internal 6-bit DAC in CMP module.
 *
 * This function is to set the output value for internal 6-bit DAC in CMP module.
 * The output voltage of DAC would be DACO = (V in /64) * (value + 1) , so the
 * DACO range is from Vin/64 to Vin.
 *
 * @param baseAddr Register base address for the module.
 * @param value Setting value, 6-bit available.
 */
static inline void CMP_HAL_SetDacValue(uint32_t baseAddr, uint8_t value)
{
    BW_CMP_DACCR_VOSEL(baseAddr, value);
}

/*!
 * @brief Set the plus channel for analog comparator.
 *
 * This function is to set the plus channel for analog comparator. 
 * The input channels of plus and minus side are comes from the same channel
 * mux. When an inappropriate operation selects the same input for both mux,
 * the comparator automatically shuts down to prevent itself from becoming a
 * noise generator. For channel's use cases, see to the speculation for each
 * SOC.
 *
 * @param baseAddr Register base address for the module.
 * @param mode Channel mux mode, see to "cmp_chn_mux_mode_t".
 */
static inline void CMP_HAL_SetPlusInputChnMuxMode(uint32_t baseAddr, cmp_chn_mux_mode_t mode)
{
    BW_CMP_MUXCR_PSEL(baseAddr, (uint8_t)(mode) );
}

/*!
 * @brief Set the minus channel for analog comparator.
 *
 * This function is to set the minus channel for analog comparator. 
 * The input channels of plus and minus side are comes from the same channel
 * mux. When an inappropriate operation selects the same input for both mux,
 * the comparator automatically shuts down to prevent itself from becoming a
 * noise generator. For channel's use cases, see to the speculation for each
 * SOC.
 *
 * @param baseAddr Register base address for the module.
 * @param mode Channel mux mode, see to "cmp_chn_mux_mode_t".
 */
static inline void CMP_HAL_SetMinusInputChnMuxMode(uint32_t baseAddr, cmp_chn_mux_mode_t mode)
{
    BW_CMP_MUXCR_MSEL(baseAddr, (uint8_t)(mode) );
}

#if FSL_FEATURE_CMP_HAS_PASS_THROUGH_MODE
/*!
 * @brief Switch to enable the mux pass through mode.
 *
 * This function is to switch to enable the mux pass through mode.
 *
 * @param baseAddr Register base address for the module.
 * @param enable Switcher to enable the feature.
 */
static inline void CMP_HAL_SetPassThroughModeCmd(uint32_t baseAddr, bool enable)
{
    BW_CMP_MUXCR_PSTM(baseAddr, (enable ? 1U : 0U) );
}
#endif /* FSL_FEATURE_CMP_HAS_PASS_THROUGH_MODE */

#if defined(__cplusplus)
}
#endif

/*!
 * @}
 */

#endif /* __FSL_CMP_HAL_H__ */

/*******************************************************************************
 * EOF
 ******************************************************************************/

