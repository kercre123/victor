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

#ifndef __FSL_CADC_HAL_H__
#define __FSL_CADC_HAL_H__

#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include "fsl_device_registers.h"

/*!
 * @addtogroup cadc_hal
 * @{
 */

/******************************************************************************
 * Definitions
 *****************************************************************************/

/*!
 * @brief CADC status return codes.
 */
typedef enum _cadc_status
{
    kStatus_CADC_Success         = 0U, /*!< Success. */
    kStatus_CADC_InvalidArgument = 1U, /*!< Invalid argument existed. */
    kStatus_CADC_Failed          = 2U  /*!< Execution failed. */
} cadc_status_t;

/*!
 * @brief Defines the type of enumerating ADC converter's ID.
 */
typedef enum _cadc_conv_id
{
    kCAdcConvA = 0U,/*!< ID for ADC converter A. */
    kCAdcConvB = 1U /*!< ID for ADC converter B. */
} cadc_conv_id_t;

/*!
 * @brief Defines the type of enumerating ADC differential channel pair.
 * 
 * Note, "cadc_diff_chn_mode_t" and "cadc_chn_sel_mode_t" can determine to
 * select the single ADC sample channel.
 */
typedef enum _cadc_diff_chn_mode
{
    kCAdcDiffChnANA0_1 = 0U, /*!< ConvA's Chn 0 & 1. */
    kCAdcDiffChnANA2_3 = 1U, /*!< ConvA's Chn 2 & 3. */
    kCAdcDiffChnANA4_5 = 2U, /*!< ConvA's Chn 4 & 5. */
    kCAdcDiffChnANA6_7 = 3U, /*!< ConvA's Chn 6 & 7. */
    kCAdcDiffChnANB0_1 = 4U, /*!< ConvB's Chn 0 & 1. */
    kCAdcDiffChnANB2_3 = 5U, /*!< ConvB's Chn 2 & 3. */
    kCAdcDiffChnANB4_5 = 6U, /*!< ConvB's Chn 4 & 5. */
    kCAdcDiffChnANB6_7 = 7U  /*!< ConvB's Chn 6 & 7. */
} cadc_diff_chn_mode_t;

/*!
 * @brief Defines the type of enumerating ADC channel in differential pair.
 * 
 * Note, "cadc_diff_chn_mode_t" and "cadc_chn_sel_mode_t" can determine to
 * select the single ADC sample channel.
 */
typedef enum _cadc_chn_sel_mode
{
    kCAdcChnSelN = 0U, /*!< Select negative side channel. */
    kCAdcChnSelP = 1U, /*!< Select positive side channel. */
    kCAdcChnSelBoth = 2U /*!< Select both of them in differential mode..**/
} cadc_chn_sel_mode_t;

/*!
 * @brief Defines the type of enumerating ADC converter's scan mode.
 *
 * See to ADC_CTRL1[SMODE] from Reference Manual for more detailed information
 * about ADC converter's scan mode.
 */
typedef enum _cadc_scan_mode
{
    kCAdcScanOnceSequential      = 0U, /*!< Once (single) sequential. */
    kCAdcScanOnceParallel        = 1U, /*!< Once parallel. */
    kCAdcScanLoopSequential      = 2U, /*!< Loop sequential. */
    kCAdcScanLoopParallel        = 3U, /*!< Loop parallel. */
    kCAdcScanTriggeredSequential = 4U, /*!< Triggered sequential. */
    kCAdcScanTriggeredParalled   = 5U  /*!< Triggered parallel (default). */
} cadc_scan_mode_t;

/*!
 * @brief Defines the type of enumerating zero crossing detection mode for each slot.
 */
typedef enum _cadc_zero_crossing_mode
{
    kCAdcZeroCrossingDisable        = 0U, /*!< Zero crossing detection disabled. */
    kCAdcZeroCrossingAtRisingEdge   = 1U, /*!< Enable for positive to negative sign change. */
    kCAdcZeroCrossingAtFallingEdge  = 2U, /*!< Enable for negative to positive sign change. */
    kCAdcZeroCrossingAtBothEdge     = 3U  /*!< Enable for any sign change. */
} cadc_zero_crossing_mode_t;

/*!
 * @brief Defines the type of enumerating amplification mode for each slot.
 */
typedef enum _cadc_gain_mode
{
    kCAdcSGainBy1 = 0U, /*!< x1 amplification. */
    kCAdcSGainBy2 = 1U, /*!< x2 amplification. */
    kCAdcSGainBy4 = 2U  /*!< x4 amplification. */
} cadc_gain_mode_t;

/*!
 * @brief Defines the type of enumerating speed mode for each converter.
 *
 * These items represent the clock speed at which the ADC converter can operate.
 * Faster conversion speeds require greater current consumption.
 */
typedef enum _cadc_conv_speed_mode
{
    kCAdcConvClkLimitBy6_25MHz  = 0U, /*!< Conversion clock frequency <= 6.25 MHz;
                                            current consumption per converter = 6 mA. */
    kCAdcConvClkLimitBy12_5MHz  = 1U, /*!< Conversion clock frequency <= 12.5 MHz;
                                            current consumption per converter = 10.8 mA. */
    kCAdcConvClkLimitBy18_75MHz = 2U, /*!< Conversion clock frequency <= 18.75 MHz;
                                            current consumption per converter = 18 mA. */
    kCAdcConvClkLimitBy25MHz    = 3U  /*!< Conversion clock frequency <= 25 MHz;
                                            current consumption per converter = 25.2 mA. */
} cadc_conv_speed_mode_t;

/*!
 * @brief Defines the type of DMA trigger source mode for each converter.
 *
 * During sequential and simultaneous parallel scan modes, it selects between 
 * end of scan for ConvA's scan and RDY status as the DMA source. During
 * non-simultaneous parallel scan mode it selects between end of scan for
 * converters A and B, and the RDY status as the DMA source
 */
typedef enum _cadc_dma_trigger_scr_mode
{
    kCAdcDmaTriggeredByEndOfScan = 0U, /*!< DMA trigger source is end of scan interrupt. */
    kCAdcDmaTriggeredByConvReady = 1U  /*!< DMA trigger source is RDY status. */
} cadc_dma_trigger_src_mode_t;

#if defined(__cplusplus)
extern "C" {
#endif

/*******************************************************************************
 * API
 ******************************************************************************/

/*!
 * @brief Initialize all the ADC registers to a known state.
 *
 * The initial states of ADC registers are set as the Reference Manual describes.
 *
 * @param baseAddr Register base address for the module.
 */
void CADC_HAL_Init(uint32_t baseAddr);

/*!
 * @brief Switch to enable DMA for ADC converter.
 *
 * @param baseAddr Register base address for the module.
 * @param convId ID for ADC converter, see to "cadc_conv_id_t".
 * @param enable Assertion to enable the feature.
 */
void CADC_HAL_SetConvDmaCmd(uint32_t baseAddr, cadc_conv_id_t convId, bool enable);

/*!
 * @brief Switch to enable DMA for ADC converter.
 *
 * @param baseAddr Register base address for the module.
 * @param convId ID for ADC converter, see to "cadc_conv_id_t".
 * @param enable Assertion to enable the feature.
 */
void CADC_HAL_SetConvStopCmd(uint32_t baseAddr, cadc_conv_id_t convId, bool enable);

/*!
 * @brief Trigger the ADC converter's conversion by software.
 *
 * @param baseAddr Register base address for the module.
 * @param convId ID for ADC converter, see to "cadc_conv_id_t".
 */
void CADC_HAL_SetConvStartCmd(uint32_t baseAddr, cadc_conv_id_t convId);

/*!
 * @brief Switch to enable input SYNC signal to trigger conversion.
 *
 * @param baseAddr Register base address for the module.
 * @param convId ID for ADC converter, see to "cadc_conv_id_t".
 * @param enable Assertion to enable the feature.
 */
void CADC_HAL_SetConvSyncCmd(uint32_t baseAddr, cadc_conv_id_t convId, bool enable);

/*!
 * @brief Switch to enable interrupt caused by end of scan for each converter.
 *
 * @param baseAddr Register base address for the module.
 * @param convId ID for ADC converter, see to "cadc_conv_id_t".
 * @param enable Assertion to enable the feature.
 */
void CADC_HAL_SetConvEndOfScanIntCmd(uint32_t baseAddr, cadc_conv_id_t convId, bool enable);

/*!
 * @brief Switch to enable interrupt caused by zero crossing detection for ADC module.
 *
 * @param baseAddr Register base address for the module.
 * @param enable Assertion to enable the feature.
 */
static inline void CADC_HAL_SetZeroCrossingIntCmd(uint32_t baseAddr, bool enable)
{
    BW_ADC_CTRL1_ZCIE(baseAddr, enable?1U:0U );
}

/*!
 * @brief Switch to enable interrupt caused by meet low limit for ADC module.
 *
 * @param baseAddr Register base address for the module.
 * @param enable Assertion to enable the feature.
 */
static inline void CADC_HAL_SetLowLimitIntCmd(uint32_t baseAddr, bool enable)
{
    BW_ADC_CTRL1_LLMTIE(baseAddr, enable?1U:0U );
}

/*!
 * @brief Switch to enable interrupt caused by meet high limit for ADC module.
 *
 * @param baseAddr Register base address for the module.
 * @param enable Assertion to enable the feature.
 */
static inline void CADC_HAL_SetHighLimitIntCmd(uint32_t baseAddr, bool enable)
{
    BW_ADC_CTRL1_HLMTIE(baseAddr, enable?1U:0U );
}

/*!
 * @brief Switch to enable differential sample mode for ADC conversion channel.
 *
 * @param baseAddr Register base address for the module.
 * @param chns Selection of differential channel pair, see to "cadc_diff_chn_mode_t".
 * @param enable Assertion to enable the feature.
 */
void CADC_HAL_SetChnDiffCmd(uint32_t baseAddr, cadc_diff_chn_mode_t chns, bool enable);

/*!
 * @brief Configure the scan mode for ADC module.
 *
 * @param baseAddr Register base address for the module.
 * @param mode Selection of scan mode, see to "cadc_scan_mode_t".
 */
static inline void CADC_HAL_SetScanMode(uint32_t baseAddr, cadc_scan_mode_t mode)
{
    BW_ADC_CTRL1_SMODE(baseAddr, (uint16_t)mode );
}

/*!
 * @brief Switch to enable simultaneous mode for ADC module.
 *
 * @param baseAddr Register base address for the module.
 * @param enable Assertion to enable the feature.
 */
static inline void CADC_HAL_SetParallelSimultCmd(uint32_t baseAddr, bool enable)
{
    BW_ADC_CTRL2_SIMULT(baseAddr, enable?1U:0U );
}

/*!
 * @brief Set divider for conversion clock from system clock.
 *
 * @param baseAddr Register base address for the module.
 * @param convId ID for ADC converter, see to "cadc_conv_id_t".
 * @param divider 6-bit divider value.
 */
void CADC_HAL_SetConvClkDiv(uint32_t baseAddr, cadc_conv_id_t convId, uint16_t divider);

/*!
 * @brief Set zero crossing detection for each slot in conversion sequence.
 *
 * @param baseAddr Register base address for the module.
 * @param slotNum Slot number in conversion sequence.
 * @param mode Zero crossing detection mode, see to "cadc_zero_crossing_mode_t".
 */
void CADC_HAL_SetSlotZeroCrossingMode(uint32_t baseAddr, uint32_t slotNum, 
    cadc_zero_crossing_mode_t mode);

/*!
 * @brief Set sample channel for each slot in conversion sequence.
 *
 * @param baseAddr Register base address for the module.
 * @param slotNum Slot number in conversion sequence.
 * @param diffChns Selection of differential channel pair, see to "cadc_diff_chn_mode_t".
 * @param selMode  Selection of each channel in differential channel pair, see to "cadc_chn_sel_mode_t".
 */
void CADC_HAL_SetSlotSampleChn(uint32_t baseAddr, uint32_t slotNum, 
    cadc_diff_chn_mode_t diffChns, cadc_chn_sel_mode_t selMode);

/*!
 * @brief Switch to enable sample for each slot in conversion sequence.
 *
 * @param baseAddr Register base address for the module.
 * @param slotNum Slot number in conversion sequence.
 * @param enable Assertion to enable the feature.
 */
void CADC_HAL_SetSlotSampleEnableCmd(uint32_t baseAddr, uint32_t slotNum, bool enable);

/*!
 * @brief Check if the conversion is in process for each converter.
 *
 * @param baseAddr Register base address for the module.
 * @param convId ID for ADC converter, see to "cadc_conv_id_t".
 * @return Assertion of indicated event.
 */
bool CADC_HAL_GetConvInProgressFlag(uint32_t baseAddr, cadc_conv_id_t convId);

/*!
 * @brief Check if the end of scan event is asserted.
 *
 * @param baseAddr Register base address for the module.
 * @param convId ID for ADC converter, see to "cadc_conv_id_t".
 * @return Assertion of indicated event.
 */
bool CADC_HAL_GetConvEndOfScanIntFlag(uint32_t baseAddr, cadc_conv_id_t convId);

/*!
 * @brief Clear the end of scan flag.
 *
 * @param baseAddr Register base address for the module.
 * @param convId ID for ADC converter, see to "cadc_conv_id_t".
 */
void CADC_HAL_ClearConvEndOfScanIntFlag(uint32_t baseAddr, cadc_conv_id_t convId);

/*!
 * @brief Check if main zero crossing event is asserted.
 *
 * @param baseAddr Register base address for the module.
 * @return assertion of indicated event.
 */
static inline bool CADC_HAL_GetZeroCrossingIntFlag(uint32_t baseAddr)
{
    return (1U == BR_ADC_STAT_ZCI(baseAddr) );
}

/*!
 * @brief Clear all zero crossing flag.
 *
 * This operation will clear the main zero crossing event's flag.
 *
 * @param baseAddr Register base address for the module.
 */
static inline void CADC_HAL_ClearAllZeorCrossingFlag(uint32_t baseAddr)
{
    BW_ADC_ZXSTAT_ZCS(baseAddr, 0xFFFFU);
}

/*!
 * @brief Check if the slot's crossing event flag is asserted.
 *
 * @param baseAddr Register base address for the module.
 * @param slotNum Slot number in conversion sequence.
 * @return assertion of indicated event.
 */
static inline bool CADC_HAL_GetSlotZeroCrossingFlag(uint32_t baseAddr, uint32_t slotNum)
{
    return (0U != ((1U<<slotNum) & BR_ADC_ZXSTAT_ZCS(baseAddr)) );
}

/*!
 * @brief Clear the crossing event flag for slot.
 *
 * @param baseAddr Register base address for the module.
 * @param slotNum Slot number in conversion sequence.
 */
static inline void CADC_HAL_ClearSlotZeroCrossingFlag(uint32_t baseAddr, uint32_t slotNum)
{
    BW_ADC_ZXSTAT_ZCS(baseAddr, (1U<<slotNum) );
}

/*!
 * @brief Check if main low limit event is asserted.
 *
 * @param baseAddr Register base address for the module.
 * @return Assertion of indicated event.
 */
static inline bool CADC_HAL_GetLowLimitIntFlag(uint32_t baseAddr)
{
    return (1U == BR_ADC_STAT_LLMTI(baseAddr) );
}

/*!
 * @brief Clear all the low limit flag.
 *
 * This operation will clear the main low limit event's flag.
 *
 * @param baseAddr Register base address for the module.
 */
static inline void CADC_HAL_ClearAllLowLimitFlag(uint32_t baseAddr)
{
    BW_ADC_LOLIMSTAT_LLS(baseAddr, 0xFFFFU);
}

/*!
 * @brief Check if slot's low limit flag is asserted.
 *
 * @param baseAddr Register base address for the module.
 * @param slotNum Slot number in conversion sequence.
 * @return Assertion of indicated event.
 */
static inline bool CADC_HAL_GetSlotLowLimitFlag(uint32_t baseAddr, uint32_t slotNum)
{
    return (0U != ((1U<<slotNum) & BR_ADC_LOLIMSTAT_LLS(baseAddr)) );
}

/*!
 * @brief Clear slot's low limit flag.
 *
 * @param baseAddr Register base address for the module.
 * @param slotNum Slot number in conversion sequence.
 */
static inline void CADC_HAL_ClearSlotLowLimitFlag(uint32_t baseAddr, uint32_t slotNum)
{
    BW_ADC_LOLIMSTAT_LLS(baseAddr, (1U<<slotNum) );
}

/*!
 * @brief Check if the main high limit event is asserted.
 *
 * @param baseAddr Register base address for the module.
 * @return Assertion of indicated event.
 */
static inline bool CADC_HAL_GetHighLimitIntFlag(uint32_t baseAddr)
{
    return (1U == BR_ADC_STAT_HLMTI(baseAddr) );
}

/*!
 * @brief Clear all the high limit flag.
 *
 * This operation will clear the main high limit event's flag.
 *
 * @param baseAddr Register base address for the module.
 */
static inline void CADC_HAL_ClearAllHighLimitFlag(uint32_t baseAddr)
{
    BW_ADC_HILIMSTAT_HLS(baseAddr, 0xFFFFU);
}

/*!
 * @brief Check if slot's high limit flag is asserted.
 *
 * @param baseAddr Register base address for the module.
 * @param slotNum Slot number in conversion sequence.
 * @return Assertion of indicated event.
 */
static inline bool CADC_HAL_GetSlotHighLimitFlag(uint32_t baseAddr, uint32_t slotNum)
{
    return (0U != ((1U<<slotNum) & BR_ADC_HILIMSTAT_HLS(baseAddr)) );
}

/*!
 * @brief Clear slot's high limit flag.
 *
 * @param baseAddr Register base address for the module.
 * @param slotNum Slot number in conversion sequence.
 */
static inline void CADC_HAL_ClearSlotHighLimitFlag(uint32_t baseAddr, uint32_t slotNum)
{
    BW_ADC_HILIMSTAT_HLS(baseAddr, (1U<<slotNum) );
}

static inline bool CADC_HAL_GetSlotReadyFlag(uint32_t baseAddr, uint32_t slotNum)
{
    return (0U != ((1U<<slotNum) & BR_ADC_RDY_RDY(baseAddr)) );
}

/*!
 * @brief Check if the slot's conversion value is negative.
 *
 * @param baseAddr Register base address for the module.
 * @param slotNum Slot number in conversion sequence.
 * @return Assertion of indicated event.
 */
static inline bool CADC_HAL_GetSlotValueNegativeSign(uint32_t baseAddr, uint32_t slotNum)
{
    return (1U == BR_ADC_RSLTn_SEXT(baseAddr, slotNum) );
}

/*!
 * @brief Get the slot's conversion value without sign.
 *
 * @param baseAddr Register base address for the module.
 * @param slotNum Slot number in conversion sequence.
 * @return 12-bit slot's conversion value.
 */
static inline uint16_t CADC_HAL_GetSlotValueData(uint32_t baseAddr, uint32_t slotNum)
{
    return BR_ADC_RSLTn_RSLT(baseAddr, slotNum);
}

/*!
 * @brief Set the slot's conversion low limit value.
 *
 * @param baseAddr Register base address for the module.
 * @param slotNum Slot number in conversion sequence.
 * @param val 12-bit slot's conversion low limit value.
 */
static inline void CADC_HAL_SetSlotLowLimitValue(uint32_t baseAddr, uint32_t slotNum, uint16_t val)
{
    BW_ADC_LOLIMn_LLMT(baseAddr, slotNum, val);
}

/*!
 * @brief Get the slot's conversion low limit value.
 *
 * @param baseAddr Register base address for the module.
 * @param slotNum Slot number in conversion sequence.
 * @return 12-bit slot's conversion low limit value.
 */
static inline uint16_t CADC_HAL_GetSlotLowLimitValue(uint32_t baseAddr, uint32_t slotNum)
{
    return BR_ADC_LOLIMn_LLMT(baseAddr, slotNum);
}

/*!
 * @brief Set the slot's conversion high limit value.
 *
 * @param baseAddr Register base address for the module.
 * @param slotNum Slot number in conversion sequence.
 * @param val 12-bit slot's conversion high limit value.
 */
static inline void CADC_HAL_SetSlotHighLimitValue(uint32_t baseAddr, uint32_t slotNum, uint16_t val)
{
    BW_ADC_HILIMn_HLMT(baseAddr, slotNum, val);
}

/*!
 * @brief Get the slot's conversion high limit value.
 *
 * @param baseAddr Register base address for the module.
 * @param slotNum Slot number in conversion sequence.
 * @return 12-bit slot's conversion high limit value.
 */
static inline uint16_t CADC_HAL_GetSlotHighLimitValue(uint32_t baseAddr, uint32_t slotNum)
{
    return BR_ADC_HILIMn_HLMT(baseAddr, slotNum);
}

/*!
 * @brief Set the slot's conversion high limit value.
 *
 * The value of the offset is used to correct the ADC result before it is
 * stored in the result registers. The offset value is subtracted from the ADC
 * result.
 * 
 * @param baseAddr Register base address for the module.
 * @param slotNum Slot number in conversion sequence.
 * @param val 12-bit slot's conversion high limit value.
 */
static inline void CADC_HAL_SetSlotOffsetValue(uint32_t baseAddr, uint32_t slotNum, uint16_t val)
{
    BW_ADC_OFFSTn_OFFSET(baseAddr, slotNum, val);
}

/*!
 * @brief Get the slot's conversion high limit value.
 *
 * The value of the offset is used to correct the ADC result before it is
 * stored in the result registers. The offset value is subtracted from the ADC
 * result.
 * 
 * @param baseAddr Register base address for the module.
 * @param slotNum Slot number in conversion sequence.
 * @return 12-bit slot's conversion high limit value.
 */
static inline uint16_t CADC_HAL_GetSlotOffsetValue(uint32_t baseAddr, uint32_t slotNum)
{
    return BR_ADC_OFFSTn_OFFSET(baseAddr, slotNum);
}

/*!
 * @brief Switch to enable auto-standby mode for ADC module.
 *
 * This API selects auto-standby mode. Auto-standby's configuration is ignored
 * if auto-powerdown mode is active. When the ADC is idle, auto-standby mode
 * selects the standby clock as the ADC clock source and puts the converters
 * into standby current mode. At the start of any scan, the conversion clock
 * is selected as the ADC clock and then a delay of some ADC clock cycles 
 * is imposed for current levels to stabilize. After this delay, the ADC will
 * initiate the scan. When the ADC returns to the idle state, the standby 
 * clock is again selected and the converters revert to the standby current
 * state.
 * 
 * @param baseAddr Register base address for the module.
 * @param enable Assertion to enable the feature.
 */
static inline void CADC_HAL_SetAutoStandbyCmd(uint32_t baseAddr, bool enable)
{
    BW_ADC_PWR_ASB(baseAddr, enable?1U:0U );
}

/*!
 * @brief Check if the ADC converter is powered up.
 * 
 * @param baseAddr Register base address for the module.
 * @param convId ID for ADC converter, see to "cadc_conv_id_t".
 * @return Assertion of indicated event.
 */
bool CADC_HAL_GetConvPowerUpFlag(uint32_t baseAddr, cadc_conv_id_t convId);

/*!
 * @brief Set the count power up delay of ADC clock for all the converter.
 * 
 * This 6-bit setting value determines the number of ADC clocks provided to
 * power up an ADC converter before allowing a scan to start. It also
 * determines the number of ADC clocks of delay provided in auto-powerdown
 * and auto-standby modes between when the ADC goes from the idle to active
 * state and when the scan is allowed to start. The default value is 26 ADC
 * clocks. Accuracy of the initial conversions in a scan will be degraded if
 * the setting is set to too small a value.
 *
 * @param baseAddr Register base address for the module.
 * @param val 6-bit setting value.
 */
static inline void CADC_HAL_SetPowerUpDelayClk(uint32_t baseAddr, uint16_t val)
{
    BW_ADC_PWR_PUDELAY(baseAddr, val);
}

/*!
 * @brief Get the count power up delay of ADC clock for all the converter.
 *
 * @param baseAddr Register base address for the module.
 * @return 6-bit setting value.
 */
static inline uint16_t CADC_HAL_GetPowerUpDelayClk(uint32_t baseAddr)
{
    return BR_ADC_PWR_PUDELAY(baseAddr);
}

/*!
 * @brief Switch to enable auto-powerdown mode for ADC module.
 *
 * Auto-powerdown mode powers down converters when not in use for a scan. 
 * Auto-powerdown (APD) takes precedence over auto-standby (ASB). When a scan
 * is started in APD mode, a delay of setting ADC clock cycles is imposed
 * during which the needed converter(s), if idle, are powered up. The ADC will
 * then initiate a scan equivalent to that done when APD is not active. When
 * the scan is completed, the converter(s) are powered down again.
 * 
 * @param baseAddr Register base address for the module.
 * @param enable Assertion to enable the feature.
 */
static inline void CADC_HAL_SetAutoPowerDownCmd(uint32_t baseAddr, bool enable)
{
    BW_ADC_PWR_APD(baseAddr, enable?1U:0U );
}

/*!
 * @brief Switch to enable power up for each ADC converter.
 * 
 * @param baseAddr Register base address for the module.
 * @param convId Selection of ID for ADC converter.
 * @param enable Assertion to enable the feature.
 */
void CADC_HAL_SetConvPowerUpCmd(uint32_t baseAddr, cadc_conv_id_t convId, bool enable);

/*!
 * @brief Switch to use channel 3 as VrefH for each ADC converter.
 * 
 * @param baseAddr Register base address for the module.
 * @param convId Selection of ID for ADC converter.
 * @param enable Assertion to enable the feature.
 */
void CADC_HAL_SetConvUseChnVrefHCmd(uint32_t baseAddr, cadc_conv_id_t convId, bool enable);

/*!
 * @brief Switch to use channel 2 as VrefL for each ADC converter.
 * 
 * @param baseAddr Register base address for the module.
 * @param convId Selection of ID for ADC converter.
 * @param enable Assertion to enable the feature.
 */
void CADC_HAL_SetConvUseChnVrefLCmd(uint32_t baseAddr, cadc_conv_id_t convId, bool enable);

/*!
 * @brief Set the amplification for each input channel.
 * 
 * @param baseAddr Register base address for the module.
 * @param chnNum Channel number of input mux.
 * @param mode Selection of gain mode, see to "cadc_gain_mode_t".
 */
void CADC_HAL_SetChnGainMode(uint32_t baseAddr, uint32_t chnNum, cadc_gain_mode_t mode);

/*!
 * @brief Switch to enable the sync point for each slot in sequence.
 * 
 * The sync point provides the ability to pause and await a new sync while
 * processing samples programmed in conversion sequence. This API determines
 * whether a sample in a scan occurs immediately or if the sample waits for an
 * enabled sync input to occur. The sync input must occur after the conversion
 * of the current sample completes.
 *
 * @param baseAddr Register base address for the module.
 * @param slotNum Slot number in conversion sequence.
 * @param enable Assertion of indicated feature.
 */
void CADC_HAL_SetSlotSyncPointCmd(uint32_t baseAddr, uint32_t slotNum, bool enable);

/*!
 * @brief Set the conversion speed control mode for each ADC converter.
 * 
 * Note, faster conversion speeds require greater current consumption.
 *
 * @param baseAddr Register base address for the module.
 * @param convId Selection of ID for ADC converter.
 * @param mode Selection of speed mode, see to "cadc_conv_clk_limit_mode_t".
 */
void CADC_HAL_SetConvSpeedLimitMode(uint32_t baseAddr, cadc_conv_id_t convId, cadc_conv_speed_mode_t mode);

/*!
 * @brief Set the DMA trigger mode for ADC module.
 *
 * @param baseAddr Register base address for the module.
 * @param mode Selection of setting mode, see to "cadc_dma_trigger_src_mode_t".
 */
static inline void CADC_HAL_SetDmaTriggerSrcMode(uint32_t baseAddr, cadc_dma_trigger_src_mode_t mode)
{
    BW_ADC_CTRL3_DMASRC(baseAddr, (kCAdcDmaTriggeredByConvReady==mode)?1U:0U );
}

/*!
 * @brief Set the conversion speed control mode for each ADC converter.
 * 
 * During sequential and parallel simultaneous scan modes, the setting value
 * controls the sampling time of the first sample after a scan is initiated
 * on both converters A and B. In parallel non-simultaneous mode, the setting
 * value for each converter affects converter A or B only. The default value
 * is 0 which corresponds to a sampling time of 2 ADC clocks. Each increment
 * of the setting value corresponds to an additional ADC clock cycle of
 * sampling time with a maximum sampling time of 9 ADC clocks. In sequential
 * scan mode, the converter A's setting will be ignored whenever the channel
 * selected for the next sample is on the other converter. In other words,
 * during a sequential scan, if a sample converts a converter A channel 
 * (ANA0-ANA7) and the next sample converts a converter B channel (ANB0-ANB7) 
 * or vice versa, the setting value will be ignored and use the default
 * sampling time for the next sample.
 *
 * @param baseAddr Register base address for the module.
 * @param convId Selection of ID for ADC converter.
 * @param 3-bit setting value.
 */
void CADC_HAL_SetConvSampleWindow(uint32_t baseAddr, cadc_conv_id_t convId, uint16_t val);

/*!
 * @brief Switch to enable scan interrupt for each slot in sequence.
 *
 * This API is used with ready flag detection to select the samples that will
 * generate a scan interrupt.
 * 
 * @param baseAddr Register base address for the module.
 * @param slotNum Slot number in conversion sequence.
 * @param enable Assertion of indicated feature.
 */
void CADC_HAL_SetSlotScanIntCmd(uint32_t baseAddr, uint32_t slotNum, bool enable);


#if defined(__cplusplus)
extern }
#endif

/*!
 * @}
 */

#endif /* __FSL_CADC_HAL_H__ */
/******************************************************************************
 * EOF
 *****************************************************************************/

