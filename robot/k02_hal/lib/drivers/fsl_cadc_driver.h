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

#ifndef __FSL_CADC_DRIVER_H__
#define __FSL_CADC_DRIVER_H__

#include <stdint.h>
#include <stdbool.h>
#include "fsl_cadc_hal.h"
#include "fsl_clock_manager.h"
#include "fsl_interrupt_manager.h"

/*!
 * @addtogroup cadc_driver
 * @{
 */

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*!
 * @brief Defines a structure to configure the CyclicADC module during initialization.
 *
 * This structure holds the configuration when initializing the CyclicADC module.
 */
typedef struct
{
    /* Functional control. */
    bool zeroCrossingIntEnable; /*!< Global zero crossing interrupt enable. */
    bool lowLimitIntEnable;     /*!< Global low limit interrupt enable. */
    bool highLimitIntEnable;    /*!< Global high limit interrupt enable. */
    cadc_scan_mode_t scanMode;  /*!< ADC scan mode control. See "cadc_scan_mode_t". */
    bool parallelSimultModeEnable; /*!< Parallel scans done simultaneously enable. */
    cadc_dma_trigger_src_mode_t dmaSrcMode; /*!< DMA trigger source. See "cadc_dma_trigger_src_mode_t". */
    
    /* Power control. */
    bool autoStandbyEnable;     /*!< Auto standby mode enable. */
    uint16_t powerUpDelayCount; /*!< Power up delay. */
    bool autoPowerDownEnable;   /*!< Auto power down mode enable. */
} cadc_user_config_t;

/*!
 * @brief Defines a structure to configure each converter in the CyclicADC module.
 *
 * This structure holds the configuration for each converter in the CyclicADC module.
 * Normally, there are two converters, ConvA and ConvB in the cyclic ADC
 * module. However, each converter can be configured separately for some features.
 */
typedef struct
{
    bool dmaEnable;     /*!< DMA enable. */

    /*
    * When this bit is asserted, the current scan is stopped and no further
    * scans can start.  Any further SYNC input pulses or software trigger are
    * ignored until this bit has been cleared. After the ADC is in stop mode,
    * the results registers can be modified by the processor. Any changes to
    * the result registers in stop mode are treated as if the analog core
    * supplied the data. Therefore, limit checking, zero crossing, and
    * associated interrupts can occur when authorized. 
    */
    bool stopEnable;    /*!< Stop mode enable. */
    
    bool syncEnable;    /*!< Enable external sync input to trigger conversion. */
    bool endOfScanIntEnable; /*!< End of scan interrupt enable. */

    /*
    * For ConvA:
    * The IRQ enable covers the Conversion Complete and Scan Interrupt for any
    * scan type except ConvB (mostly ConvA) scan in non-simultaneous parallel
    * scan mode.
    *
    * For ConvB:
    * The IRQ enable covers the Conversion Complete and Scan Interrupt for
    * ConvB scan in non-simultaneous parallel scan mode.
    */
    bool convIRQEnable;      /*!< Enable IRQ in NVIC; Cover the slot interrupt in the following configuration. */
    
    /*
    * For Clock Divisor Select:
    * The divider circuit generates the ADC clock by dividing the system clock:
    *   • When the value is set to 0, the divisor is 2.
    *   • For all other setting values, the divisor is 1 more than the decimal
    *     value of the setting value.
    * A divider value must be chosen to prevent the ADC clock from exceeding the
    * maximum frequency.
    */
    uint16_t clkDivValue;    /*!< ADC clock divider from the bus clock. */

    bool powerOnEnable;      /*!< Disable manual power down for converter. */
    bool useChnInputAsVrefH; /*!< Use input channel as high reference voltage, such as AN2. */
    bool useChnInputAsVrefL; /*!< Use input channel as low reference voltage, such as AN3. */
    cadc_conv_speed_mode_t speedMode; /*!< ADC speed control mode, see "cadc_conv_speed_mode_t". */

    /*
    * For ConvA:
    * During sequential and parallel simultaneous scan modes, the 
    * "sampleWindowCount" controls the sampling time of the first sample after
    * a scan is initiated on both converters A and B.
    * In parallel non-simultaneous mode, this field affects ConvA only. 
    * In sequential scan mode, this field setting is ignored whenever
    * the channel selected for the next sample is on the other converter. In
    * other words, during a sequential scan, if a sample converts a ConvA
    * channel (ANA0-ANA7) and the next sample converts a ConvB channel
    * (ANB0-ANB7) or vice versa, this field is ignored and uses the
    * default sampling time (value 0) for the next sample.
    *
    * For ConvB:
    * During parallel non-simultaneous scan mode, the "sampleWindowCount" for 
    * ConvB is used to control the sampling time of the first sample after
    * a scan is initiated. During sequential and parallel simultaneous scan
    * modes, "sampleWindowCount" is ignored and the sampling window for both 
    * converters is controlled by the "sampleWindowCount" for ConvA. 
    * 
    * For setting value:
    * The value 0 corresponds to a sampling time of 2 ADC clocks. Each increment
    * of "sampleWindowCount" corresponds to an additional ADC clock cycle of
    * sampling time with a maximum sampling time of 9 ADC clocks.
    */
    uint16_t sampleWindowCount; /*!< Sample window count. */
} cadc_conv_config_t;

/*!
 * @brief Defines a structure to configure each input channel.
 *
 * This structure holds the configuration for each input channel. In CcylicADC
 * module, the input channels are handled by a differential sample.
 * However, the user can still configure the function for each channel when
 * set to operate as a single end sample.
 */
typedef struct
{
    cadc_chn_sel_mode_t diffSelMode; /*!< Select which channel is indicated in a pair. */
    bool diffEnable; /* Operate in a differential pair or not. */
    cadc_gain_mode_t gainMode; /* Gain mode for each channel. See "cadc_gain_mode_t". */
} cadc_chn_config_t;

/*!
 * @brief Defines a structure to configure each slot.
 *
 * This structure holds the configuration for each slot in a conversion sequence.
 */
typedef struct
{
    bool slotEnable; /* Make the slot available. */
    bool syncPointEnable; /*!< Sample waits for an enabled SYNC input to occur. */
    bool scanIntEnable; /*!< Scan interrupt enable. */

    /* Select the input channel for slot. */
    cadc_diff_chn_mode_t diffChns;  /*!< Select the differential pair. */
    cadc_chn_sel_mode_t  diffSel;   /*!< Positive or negative channel in differential pair. */

    /* Event detection mode. */
    cadc_zero_crossing_mode_t zeroCrossingMode; /*!< Select zero crossing detection mode.*/
    uint16_t lowLimitValue; /*!< Select low limit for hardware compare. */
    uint16_t highLimitValue;/*!< Select high limit for hardware compare. */
    uint16_t offsetValue;   /*!< Select sign change limit for hardware compare. */
} cadc_slot_config_t;

/*!
 * @brief Defines types for an enumerating event.
 */
typedef enum _cadc_flag
{
    /* Converter events. */
    kCAdcConvInProgress = 0U,   /*!< Conversion in progress for each converter. */
    kCAdcConvEndOfScanInt = 1U, /*!< End of scan interrupt. */
    kCAdcConvPowerUp = 2U,      /*!< The converter is powered up. */

    /* Global events. */
    kCAdcZeroCrossingInt = 3U,  /*!< Zero crossing interrupt. */
    kCAdcLowLimitInt = 4U,      /*!< Low limit interrupt. */
    kCAdcHighLimitInt = 5U,     /*!< High limit interrupt. */

    /* Slot events. */
    kCAdcSlotReady = 6U,            /*!< Sample is ready to be read. */
    kCAdcSlotLowLimitEvent = 7U,    /*!< Low limit event for each slot. */
    kCAdcSlotHighLimitEvent = 8U,   /*!< High limit event for each slot. */
    kCAdcSlotCrossingEvent = 9U     /*!< Zero crossing event for each slot. */
} cadc_flag_t;

/*! @brief Table of base addresses for ADC instances. */
extern const uint32_t g_cadcBaseAddr[];

/*! @brief Table to save ADC IRQ enumeration numbers defined in the CMSIS header file. */
extern const IRQn_Type g_cadcErrIrqId[HW_ADC_INSTANCE_COUNT];
extern const IRQn_Type g_cadcConvAIrqId[HW_ADC_INSTANCE_COUNT];
extern const IRQn_Type g_cadcConvBIrqId[HW_ADC_INSTANCE_COUNT];

/*******************************************************************************
 * API
 ******************************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif

/*! 
 * @name CADC Driver
 * @{
 */

/*!
 * @brief Populates the user configuration structure for the CyclicADC common settings.
 *
 * This function populates the cadc_user_config_t structure with
 * default settings, which are used in polling mode for ADC conversion.
 * These settings are:
 *
 * .zeroCrossingIntEnable = false;
 * .lowLimitIntEnable = false;
 * .highLimitIntEnable = false;
 * .scanMode = kCAdcScanOnceSequential;
 * .parallelSimultModeEnable = false;
 * .dmaSrcMode = kCAdcDmaTriggeredByEndOfScan;
 * .autoStandbyEnable = false;
 * .powerUpDelayCount = 0x2AU;
 * .autoPowerDownEnable = false;
 *
 * @param configPtr Pointer to structure of "cadc_user_config_t".
 * @return Execution status.
 */
cadc_status_t CADC_DRV_StructInitUserConfigDefault(cadc_user_config_t *configPtr);

/*!
 * @brief Initializes the CyclicADC module for a global configuration.
 *
 * This function configures the CyclicADC module for the global configuration
 * which is shared by all converters.
 *
 * @param instance Instance ID number.
 * @param configPtr Pointer to structure of "cadc_user_config_t".
 * @return Execution status.
 */
cadc_status_t CADC_DRV_Init(uint32_t instance, cadc_user_config_t *configPtr);

/*!
 * @brief De-initializes the CyclicADC module.
 *
 * This function shuts down the CyclicADC module and disables
 * related IRQ.
 *
 * @param instance Instance ID number.
 */
void CADC_DRV_Deinit(uint32_t instance);

/*!
 * @brief Populates the user configuration structure for each converter.
 *
 * This function populates the cadc_conv_config_t structure with
 * default settings, which are used in polling mode for ADC conversion.
 * These settings are:
 *
 * .dmaEnable = false;
 * .stopEnable = false;
 * .syncEnable = false;
 * .endOfScanIntEnable = false; 
 * .convIRQEnable = false;
 * .clkDivValue = 0x3FU;
 * .powerOnEnable = true;
 * .useChnInputAsVrefH = false;
 * .useChnInputAsVrefL = false;
 * .speedMode = kCAdcConvClkLimitBy25MHz;
 * .sampleWindowCount = 0U;
 *
 * @param configPtr Pointer to structure of "cadc_conv_config_t".
 * @return Execution status.
 */
cadc_status_t CADC_DRV_StructInitConvConfigDefault(cadc_conv_config_t *configPtr);

/*!
 * @brief Configures each converter in the CyclicADC module.
 *
 * This function configures each converter in the CyclicADC module. However, when
 * the multiple converters are operating simultaneously, the converter settings
 * are interrelated. For more information, see the appropriate device
 * reference manual.
 *
 * @param instance Instance ID number.
 * @param convId Converter ID. See "cadc_conv_id_t".
 * @param configPtr Pointer to configure structure. See "cadc_conv_config_t".
 * @return Execution status.
 */
cadc_status_t CADC_DRV_ConfigConverter(
    uint32_t instance, cadc_conv_id_t convId, cadc_conv_config_t *configPtr);

/*!
 * @brief Configures the input channel for ADC conversion.
 *
 * This function configures the input channel for ADC conversion. The CyclicADC
 * module input channels are organized in pairs. The configuration can
 * be set for each channel in the pair.
 *
 * @param instance Instance ID number.
 * @param diffChns Differential channel pair. See "cadc_diff_chn_mode_t".
 * @param configPtr Pointer to configure structure. See "cadc_chn_config_t".
 * @return Execution status.
 */
cadc_status_t CADC_DRV_ConfigSampleChn(
    uint32_t instance, cadc_diff_chn_mode_t diffChns, cadc_chn_config_t *configPtr);

/*!
 * @brief Configures each slot for the ADC conversion sequence.
 *
 * This function configures each slot in the ADC conversion sequence. ADC conversion
 * sequence is the basic execution unit in the CyclicADC module. However, the
 * sequence should be configured slot-by-slot. The end of the sequence is a
 * slot that is configured as disabled.
 *
 * @param instance Instance ID number.
 * @param slotNum Indicated slot number, available in range of 0 - 15. 
 * @param configPtr Pointer to configure structure. See "cadc_slot_config_t".
 * @return Execution status.
 */
cadc_status_t CADC_DRV_ConfigSeqSlot(
    uint32_t instance, uint32_t slotNum, cadc_slot_config_t *configPtr);

/*!
 * @brief Triggers the ADC conversion sequence by software.
 *
 * This function triggers the ADC conversion by executing a software command. It
 * starts the conversion if no other SYNC input (hardware trigger) is needed.
 *
 * @param instance Instance ID number.
 * @param convId Indicated converter. See "cadc_conv_id_t". 
 */
void CADC_DRV_SoftTriggerConv(uint32_t instance, cadc_conv_id_t convId);

/*!
 * @brief Reads the conversion value and returns an absolute value.
 *
 * This function reads the conversion value from each slot in a conversion sequence.
 * The return value is the absolute value without being signed. 
 *
 * @param instance Instance ID number.
 * @param slotNum Indicated slot number, available in range of 0 - 15. 
 */
uint16_t CADC_DRV_GetSeqSlotConvValueRAW(uint32_t instance, uint32_t slotNum);

/*!
 * @brief Reads the conversion value and returns a signed value.
 *
 * This function reads the conversion value from each slot in the conversion sequence.
 * The return value is a signed value. When the read value is 
 * negative, the sample value is lower then the offset value.
 *
 * @param instance Instance ID number.
 * @param slotNum Indicated slot number, available in range of 0 - 15. 
 */
int16_t CADC_DRV_GetSeqSlotConvValueSigned(uint32_t instance, uint32_t slotNum);

/*!
 * @brief Gets the global event flag.
 *
 * This function gets the global flag of the CyclicADC module.
 *
 * @param instance Instance ID number.
 * @param flag Indicated event. See "cadc_flag_t".
 * @return Assertion of indicated event. 
 */
bool CADC_DRV_GetFlag(uint32_t instance, cadc_flag_t flag);

/*!
 * @brief Clears the global event flag.
 *
 * This function clears the global event flag of the CyclicADC module.
 *
 * @param instance Instance ID number.
 * @param flag Indicated event. See "cadc_flag_t".
 */
void CADC_DRV_ClearFlag(uint32_t instance, cadc_flag_t flag);

/*!
 * @brief Gets the flag for each converter event.
 *
 * This function gets the flag for each converter event.
 *
 * @param instance Instance ID number.
 * @param convId Indicated converter.
 * @param flag Indicated event. See "cadc_flag_t".
 * @return Assertion of indicated event. 
 */
bool CADC_DRV_GetConvFlag(uint32_t instance, cadc_conv_id_t convId, cadc_flag_t flag);

/*!
 * @brief Clears the flag for each converter event.
 *
 * This function clears the flag for each converter event.
 *
 * @param instance Instance ID number.
 * @param convId Indicated converter.
 * @param flag Indicated event. See  "cadc_flag_t".
 */
void CADC_DRV_ClearConvFlag(uint32_t instance, cadc_conv_id_t convId, cadc_flag_t flag);

/*!
 * @brief Gets the flag for each slot event.
 *
 * This function gets the flag for each slot event in the conversion in
 * sequence.
 *
 * @param instance Instance ID number.
 * @param slotNum Indicated slot number, available in range of 0 - 15.
 * @param flag Indicated event. See "cadc_flag_t".
 * @return Assertion of indicated event. 
 */
bool CADC_DRV_GetSlotFlag(uint32_t instance, uint32_t slotNum, cadc_flag_t flag);

/*!
 * @brief Clears the flag for each slot event.
 *
 * This function clears the flag for each slot event in the conversion in
 * sequence.
 *
 * @param instance Instance ID number.
 * @param slotNum Indicated slot number, available in range of 0 - 15.
 * @param flag Indicated event. See "cadc_flag_t".
 */
void CADC_DRV_ClearSlotFlag(uint32_t instance, uint32_t slotNum, cadc_flag_t flag);

/* @} */

#if defined(__cplusplus)
}
#endif

/*! @} */

#endif /* __FSL_CADC_DRIVER_H__ */
/******************************************************************************
 * EOF
 *****************************************************************************/

