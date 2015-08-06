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
#ifndef __FSL_LPUART_HAL_H__
#define __FSL_LPUART_HAL_H__

#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include "fsl_device_registers.h"

/*!
 * @addtogroup lpuart_hal
 * @{
 */

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define LPUART_SHIFT (16U)
#define LPUART_BAUD_REG_ID (0U)
#define LPUART_STAT_REG_ID (1U)
#define LPUART_CTRL_REG_ID (2U)
#define LPUART_DATA_REG_ID (3U)
#define LPUART_MATCH_REG_ID (4U)
#define LPUART_MODIR_REG_ID (5U)

/*! @brief Error codes for the LPUART driver.*/
typedef enum _lpuart_status
{
    kStatus_LPUART_Success                  = 0x00U,
    kStatus_LPUART_BaudRateCalculationError = 0x01U,
    kStatus_LPUART_RxStandbyModeError       = 0x02U,
    kStatus_LPUART_ClearStatusFlagError     = 0x03U,
    kStatus_LPUART_TxNotDisabled            = 0x04U,
    kStatus_LPUART_RxNotDisabled            = 0x05U,
    kStatus_LPUART_TxBusy                   = 0x06U,
    kStatus_LPUART_RxBusy                   = 0x07U,
    kStatus_LPUART_NoTransmitInProgress     = 0x08U,
    kStatus_LPUART_NoReceiveInProgress      = 0x09U,
    kStatus_LPUART_Timeout                  = 0x0AU,
    kStatus_LPUART_Initialized              = 0x0BU,
    kStatus_LPUART_NoDataToDeal             = 0x0CU,
    kStatus_LPUART_RxOverRun                = 0x0DU
} lpuart_status_t;

/*! @brief LPUART number of stop bits*/
typedef enum _lpuart_stop_bit_count {
    kLpuartOneStopBit = 0x0U, /*!< one stop bit @internal gui name="1" */
    kLpuartTwoStopBit = 0x1U, /*!< two stop bits @internal gui name="2" */
} lpuart_stop_bit_count_t;

/*! @brief LPUART parity mode*/
typedef enum _lpuart_parity_mode {
    kLpuartParityDisabled = 0x0U, /*!< parity disabled @internal gui name="Disabled" */
    kLpuartParityEven     = 0x2U, /*!< parity enabled, type even, bit setting: PE|PT = 10 @internal gui name="Even" */
    kLpuartParityOdd      = 0x3U, /*!< parity enabled, type odd,  bit setting: PE|PT = 11 @internal gui name="Odd" */
} lpuart_parity_mode_t;

/*! @brief LPUART number of bits in a character*/
typedef enum  _lpuart_bit_count_per_char {
    kLpuart8BitsPerChar  = 0x0U, /*!< 8-bit data characters @internal gui name="8" */
    kLpuart9BitsPerChar  = 0x1U, /*!< 9-bit data characters @internal gui name="9" */
    kLpuart10BitsPerChar = 0x2U, /*!< 10-bit data characters @internal gui name="10" */
} lpuart_bit_count_per_char_t;

/*! @brief LPUART operation configuration constants*/
typedef enum _lpuart_operation_config {
    kLpuartOperates = 0x0U, /*!< LPUART continues to operate normally.*/
    kLpuartStops    = 0x1U, /*!< LPUART stops operation. */
} lpuart_operation_config_t;

/*! @brief LPUART wakeup from standby method constants*/
typedef enum _lpuart_wakeup_method {
    kLpuartIdleLineWake = 0x0U, /*!< Idle-line wakes the LPUART receiver from standby. */
    kLpuartAddrMarkWake = 0x1U, /*!< Addr-mark wakes LPUART receiver from standby.*/
} lpuart_wakeup_method_t;

/*! @brief LPUART idle line detect selection types*/
typedef enum _lpuart_idle_line_select {
    kLpuartIdleLineAfterStartBit = 0x0U, /*!< LPUART idle character bit count start after start bit */
    kLpuartIdleLineAfterStopBit  = 0x1U, /*!< LPUART idle character bit count start after stop bit */
} lpuart_idle_line_select_t;

/*!
 * @brief LPUART break character length settings for transmit/detect.
 *
 * The actual maximum bit times may vary depending on the LPUART instance.
 */
typedef enum _lpuart_break_char_length {
    kLpuartBreakChar10BitMinimum = 0x0U, /*!< LPUART break char length 10 bit times (if M = 0, SBNS = 0)
                                      or 11 (if M = 1, SBNS = 0 or M = 0, SBNS = 1) or 12 (if M = 1,
                                      SBNS = 1 or M10 = 1, SNBS = 0) or 13 (if M10 = 1, SNBS = 1) */
    kLpuartBreakChar13BitMinimum = 0x1U, /*!< LPUART break char length 13 bit times (if M = 0, SBNS = 0
                                           or M10 = 0, SBNS = 1) or 14 (if M = 1, SBNS = 0 or M = 1,
                                           SBNS = 1) or 15 (if M10 = 1, SBNS = 1 or M10 = 1, SNBS = 0) */
} lpuart_break_char_length_t;

/*! @brief LPUART single-wire mode TX direction*/
typedef enum _lpuart_singlewire_txdir {
    kLpuartSinglewireTxdirIn  = 0x0U, /*!< LPUART Single Wire mode TXDIR input*/
    kLpuartSinglewireTxdirOut = 0x1U, /*!< LPUART Single Wire mode TXDIR output*/
} lpuart_singlewire_txdir_t;

/*! @brief LPUART Configures the match addressing mode used.*/
typedef enum _lpuart_match_config {
    kLpuartAddressMatchWakeup    = 0x0U, /*!< Address Match Wakeup*/
    kLpuartIdleMatchWakeup       = 0x1U, /*!< Idle Match Wakeup*/
    kLpuartMatchOnAndMatchOff    = 0x2U, /*!< Match On and Match Off*/
    kLpuartEnablesRwuOnDataMatch = 0x3U, /*!< Enables RWU on Data Match and Match On/Off for transmitter CTS input*/
} lpuart_match_config_t;

/*! @brief LPUART infra-red transmitter pulse width options*/
typedef enum _lpuart_ir_tx_pulsewidth {
    kLpuartIrThreeSixteenthsWidth  = 0x0U, /*!< 3/16 pulse*/
    kLpuartIrOneSixteenthWidth     = 0x1U, /*!< 1/16 pulse*/
    kLpuartIrOneThirtysecondsWidth = 0x2U, /*!< 1/32 pulse*/
    kLpuartIrOneFourthWidth        = 0x3U, /*!< 1/4 pulse*/
} lpuart_ir_tx_pulsewidth_t;

/*!
 * @brief LPUART Configures the number of idle characters that must be received
 * before the IDLE flag is set.
 */
typedef enum _lpuart_idle_char {
    kLpuart_1_IdleChar   = 0x0U, /*!< 1 idle character*/
    kLpuart_2_IdleChar   = 0x1U, /*!< 2 idle character*/
    kLpuart_4_IdleChar   = 0x2U, /*!< 4 idle character*/
    kLpuart_8_IdleChar   = 0x3U, /*!< 8 idle character*/
    kLpuart_16_IdleChar  = 0x4U, /*!< 16 idle character*/
    kLpuart_32_IdleChar  = 0x5U, /*!< 32 idle character*/
    kLpuart_64_IdleChar  = 0x6U, /*!< 64 idle character*/
    kLpuart_128_IdleChar = 0x7U, /*!< 128 idle character*/
} lpuart_idle_char_t;

/*! @brief LPUART Transmits the CTS Configuration. Configures the source of the CTS input.*/
typedef enum _lpuart_cts_source {
    kLpuartCtsSourcePin = 0x0U,  /*!< CTS input is the LPUART_CTS pin.*/
    kLpuartCtsSourceInvertedReceiverMatch = 0x1U, /*!< CTS input is the inverted Receiver Match result.*/
} lpuart_cts_source_t;

/*!
 * @brief LPUART Transmits CTS Source.Configures if the CTS state is checked at
 * the start of each character or only when the transmitter is idle.
 */
typedef enum _lpuart_cts_config {
    kLpuartCtsSampledOnEachChar = 0x0U, /*!< CTS input is sampled at the start of each character.*/
    kLpuartCtsSampledOnIdle     = 0x1U, /*!< CTS input is sampled when the transmitter is idle.*/
} lpuart_cts_config_t;

/*! @brief Structure for idle line configuration settings*/
typedef struct LpuartIdleLineConfig {
    unsigned idleLineType : 1; /*!< ILT, Idle bit count start: 0 - after start bit (default),
                                    1 - after stop bit */
    unsigned rxWakeIdleDetect : 1; /*!< RWUID, Receiver Wake Up Idle Detect. IDLE status bit
                                        operation during receive standbyControls whether idle
                                        character that wakes up receiver will also set
                                        IDLE status bit 0 - IDLE status bit doesn't
                                        get set (default), 1 - IDLE status bit gets set*/
} lpuart_idle_line_config_t;

/*!
 * @brief LPUART status flags.
 *
 * This provides constants for the LPUART status flags for use in the UART functions.
 */
typedef enum _lpuart_status_flag {
    kLpuartTxDataRegEmpty         = LPUART_STAT_REG_ID << LPUART_SHIFT | BP_LPUART_STAT_TDRE,    /*!< Tx data register empty flag, sets when Tx buffer is empty */
    kLpuartTxComplete             = LPUART_STAT_REG_ID << LPUART_SHIFT | BP_LPUART_STAT_TC,      /*!< Transmission complete flag, sets when transmission activity complete */
    kLpuartRxDataRegFull          = LPUART_STAT_REG_ID << LPUART_SHIFT | BP_LPUART_STAT_RDRF,    /*!< Rx data register full flag, sets when the receive data buffer is full */
    kLpuartIdleLineDetect         = LPUART_STAT_REG_ID << LPUART_SHIFT | BP_LPUART_STAT_IDLE,    /*!< Idle line detect flag, sets when idle line detected */
    kLpuartRxOverrun              = LPUART_STAT_REG_ID << LPUART_SHIFT | BP_LPUART_STAT_OR,      /*!< Rxr Overrun, sets when new data is received before data is read from receive register */
    kLpuartNoiseDetect            = LPUART_STAT_REG_ID << LPUART_SHIFT | BP_LPUART_STAT_NF,      /*!< Rxr takes 3 samples of each received bit.  If any of these samples differ, noise flag sets */
    kLpuartFrameErr               = LPUART_STAT_REG_ID << LPUART_SHIFT | BP_LPUART_STAT_FE,      /*!< Frame error flag, sets if logic 0 was detected where stop bit expected */
    kLpuartParityErr              = LPUART_STAT_REG_ID << LPUART_SHIFT | BP_LPUART_STAT_PF,      /*!< If parity enabled, sets upon parity error detection */
    kLpuartLineBreakDetect        = LPUART_STAT_REG_ID << LPUART_SHIFT | BP_LPUART_STAT_LBKDE,   /*!< LIN break detect interrupt flag, sets when LIN break char detected and LIN circuit enabled */
    kLpuartRxActiveEdgeDetect     = LPUART_STAT_REG_ID << LPUART_SHIFT | BP_LPUART_STAT_RXEDGIF, /*!< Rx pin active edge interrupt flag, sets when active edge detected */
    kLpuartRxActive               = LPUART_STAT_REG_ID << LPUART_SHIFT | BP_LPUART_STAT_RAF,     /*!< Receiver Active Flag (RAF), sets at beginning of valid start bit */
    kLpuartNoiseInCurrentWord     = LPUART_DATA_REG_ID << LPUART_SHIFT | BP_LPUART_DATA_NOISY,     /*!< NOISY bit, sets if noise detected in current data word */
    kLpuartParityErrInCurrentWord = LPUART_DATA_REG_ID << LPUART_SHIFT | BP_LPUART_DATA_PARITYE,   /*!< PARITYE bit, sets if noise detected in current data word */
#if FSL_FEATURE_LPUART_HAS_ADDRESS_MATCHING
    kLpuartMatchAddrOne              = LPUART_STAT_REG_ID << LPUART_SHIFT | BP_LPUART_STAT_MA1F,    /*!< Address one match flag */
    kLpuartMatchAddrTwo              = LPUART_STAT_REG_ID << LPUART_SHIFT | BP_LPUART_STAT_MA2F,    /*!< Address two match flag */
#endif
} lpuart_status_flag_t;

/*! @brief LPUART interrupt configuration structure, default settings are 0 (disabled)*/
typedef enum _lpuart_interrupt {
    kLpuartIntLinBreakDetect = LPUART_BAUD_REG_ID << LPUART_SHIFT | BP_LPUART_BAUD_LBKDIE,  /*!< LIN break detect. */
    kLpuartIntRxActiveEdge   = LPUART_BAUD_REG_ID << LPUART_SHIFT | BP_LPUART_BAUD_RXEDGIE, /*!< RX Active Edge. */
    kLpuartIntTxDataRegEmpty = LPUART_CTRL_REG_ID << LPUART_SHIFT | BP_LPUART_CTRL_TIE,     /*!< Transmit data register empty. */
    kLpuartIntTxComplete     = LPUART_CTRL_REG_ID << LPUART_SHIFT | BP_LPUART_CTRL_TCIE,    /*!< Transmission complete. */
    kLpuartIntRxDataRegFull  = LPUART_CTRL_REG_ID << LPUART_SHIFT | BP_LPUART_CTRL_RIE,     /*!< Receiver data register full. */
    kLpuartIntIdleLine       = LPUART_CTRL_REG_ID << LPUART_SHIFT | BP_LPUART_CTRL_ILIE,    /*!< Idle line. */
    kLpuartIntRxOverrun      = LPUART_CTRL_REG_ID << LPUART_SHIFT | BP_LPUART_CTRL_ORIE,    /*!< Receiver Overrun. */
    kLpuartIntNoiseErrFlag   = LPUART_CTRL_REG_ID << LPUART_SHIFT | BP_LPUART_CTRL_NEIE,    /*!< Noise error flag. */
    kLpuartIntFrameErrFlag   = LPUART_CTRL_REG_ID << LPUART_SHIFT | BP_LPUART_CTRL_FEIE,    /*!< Framing error flag. */
    kLpuartIntParityErrFlag  = LPUART_CTRL_REG_ID << LPUART_SHIFT | BP_LPUART_CTRL_PEIE,    /*!< Parity error flag. */
#if FSL_FEATURE_LPUART_HAS_ADDRESS_MATCHING
    kLpuartIntMatchAddrOne   = LPUART_CTRL_REG_ID << LPUART_SHIFT | BP_LPUART_CTRL_MA1IE,   /*!< Match address one flag. */
    kLpuartIntMatchAddrTwo   = LPUART_CTRL_REG_ID << LPUART_SHIFT | BP_LPUART_CTRL_MA2IE,   /*!< Match address two flag. */
#endif
} lpuart_interrupt_t;


/*******************************************************************************
 * API
 ******************************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif

/*!
 * @name LPUART Common Configurations
 * @{
 */

/*!
 * @brief Initializes the LPUART controller to known state.
 *
 * @param baseAddr LPUART base address.
 */
void LPUART_HAL_Init(uint32_t baseAddr);

/*!
 * @brief Enable/Disable the LPUART transmitter.
 *
 * @param baseAddr LPUART base address.
 * @param enable Enable(true) or disable(false) transmitter.
 */
static inline void LPUART_HAL_SetTransmitterCmd(uint32_t baseAddr, bool enable)
{
    BW_LPUART_CTRL_TE(baseAddr, enable);
}

/*!
 * @brief Gets the LPUART transmitter enabled/disabled configuration.
 *
 * @param baseAddr LPUART base address
 * @return State of LPUART transmitter enable(true)/disable(false)
 */
static inline bool LPUART_HAL_GetTransmitterCmd(uint32_t baseAddr)
{
    return BR_LPUART_CTRL_TE(baseAddr);
}

/*!
 * @brief Enable/Disable the LPUART receiver.
 *
 * @param baseAddr LPUART base address
 * @param enable Enable(true) or disable(false) receiver.
 */
static inline void LPUART_HAL_SetReceiverCmd(uint32_t baseAddr, bool enable)
{
    BW_LPUART_CTRL_RE(baseAddr, enable);
}

/*!
 * @brief Gets the LPUART receiver enabled/disabled configuration.
 *
 * @param baseAddr LPUART base address
 * @return State of LPUART receiver enable(true)/disable(false)
 */
static inline bool LPUART_HAL_GetReceiverCmd(uint32_t baseAddr)
{
    return BR_LPUART_CTRL_RE(baseAddr);
}

/*!
 * @brief Configures the LPUART baud rate.
 *
 *  In some LPUART instances the user must disable the transmitter/receiver
 *  before calling this function.
 *  Generally, this may be applied to all LPUARTs to ensure safe operation.
 *
 * @param baseAddr LPUART base address.
 * @param sourceClockInHz LPUART source input clock in Hz.
 * @param desiredBaudRate LPUART desired baud rate.
 * @return  An error code or kStatus_Success
 */
lpuart_status_t LPUART_HAL_SetBaudRate(uint32_t baseAddr,
                                       uint32_t sourceClockInHz,
                                       uint32_t desiredBaudRate);

/*!
 * @brief Sets the LPUART baud rate modulo divisor.
 *
 * @param baseAddr LPUART base address.
 * @param baudRateDivisor The baud rate modulo division "SBR"
 */
static inline void LPUART_HAL_SetBaudRateDivisor(uint32_t baseAddr, uint32_t baudRateDivisor)
{
    assert ((baudRateDivisor < 0x1FFF) && (baudRateDivisor > 1));
    BW_LPUART_BAUD_SBR(baseAddr, baudRateDivisor);
}

#if FSL_FEATURE_LPUART_HAS_BAUD_RATE_OVER_SAMPLING_SUPPORT
/*!
 * @brief Sets the LPUART baud rate oversampling ratio (Note: Feature available on select
 *        LPUART instances used together with baud rate programming)
 *        The oversampling ratio should be set between 4x (00011) and 32x (11111). Writing
 *        an invalid oversampling ratio results in an error and is set to a default
 *        16x (01111) oversampling ratio.
 *        IDisable the transmitter/receiver before calling
 *        this function.
 *
 * @param baseAddr LPUART base address.
 * @param overSamplingRatio The oversampling ratio "OSR"
 */
static inline void LPUART_HAL_SetOversamplingRatio(uint32_t baseAddr, uint32_t overSamplingRatio)
{
    assert(overSamplingRatio < 0x1F);
    BW_LPUART_BAUD_OSR(baseAddr, overSamplingRatio);
}
#endif

#if FSL_FEATURE_LPUART_HAS_BOTH_EDGE_SAMPLING_SUPPORT
/*!
 * @brief Configures the LPUART baud rate both edge sampling (Note: Feature available on select
 *        LPUART instances used with baud rate programming)
 *        When enabled, the received data is sampled on both edges of the baud rate clock.
 *        This must be set when the oversampling ratio is between 4x and 7x.
 *        This function should only be called when the receiver is disabled.
 *
 * @param baseAddr LPUART base address.
 * @param enable   Enable (1) or Disable (0) Both Edge Sampling
 * @return An error code or kStatus_Success
 */
static inline void LPUART_HAL_SetBothEdgeSamplingCmd(uint32_t baseAddr, bool enable)
{
    BW_LPUART_BAUD_BOTHEDGE(baseAddr, enable);
}
#endif

/*!
 * @brief Configures the number of bits per character in the LPUART controller.
 *
 *  In some LPUART instances, the user should disable the transmitter/receiver
 *  before calling this function.
 *  Generally, this may be applied to all LPUARTs to ensure safe operation.
 *
 * @param baseAddr LPUART base address.
 * @param bitCountPerChar  Number of bits per char (8, 9, or 10, depending on the LPUART instance)
 */
void LPUART_HAL_SetBitCountPerChar(uint32_t baseAddr, lpuart_bit_count_per_char_t bitCountPerChar);

/*!
 * @brief Configures parity mode in the LPUART controller.
 *
 *  In some LPUART instances, the user should disable the transmitter/receiver
 *  before calling this function.
 *  Generally, this may be applied to all LPUARTs to ensure safe operation.
 *
 * @param baseAddr LPUART base address.
 * @param parityModeType  Parity mode (enabled, disable, odd, even - see parity_mode_t struct)
 */
void LPUART_HAL_SetParityMode(uint32_t baseAddr, lpuart_parity_mode_t parityModeType);

/*!
 * @brief Configures the number of stop bits in the LPUART controller.
 *  In some LPUART instances, the user should disable the transmitter/receiver
 *  before calling this function.
 *  Generally, this may be applied to all LPUARTs to ensure safe operation.
 *
 * @param baseAddr LPUART base address.
 * @param stopBitCount Number of stop bits (1 or 2 - see lpuart_stop_bit_count_t struct)
 * @return  An error code (an unsupported setting in some LPUARTs) or kStatus_Success
 */
static inline void LPUART_HAL_SetStopBitCount(uint32_t baseAddr, lpuart_stop_bit_count_t stopBitCount)
{
    BW_LPUART_BAUD_SBNS(baseAddr, stopBitCount);
}

/*!
 * @brief Configures the transmit and receive inversion control in the LPUART controller.
 *
 * This function should only be called when the LPUART is between transmit and receive packets.
 *
 * @param baseAddr LPUART base address.
 * @param enable Enable (1) or disable (0) transmit inversion
 */
static inline void LPUART_HAL_SetTxInversionCmd(uint32_t baseAddr, bool enable)
{
    BW_LPUART_CTRL_TXINV(baseAddr, enable);
}

/*!
 * @brief Configures the transmit and receive inversion control in the LPUART controller.
 *
 * This function should only be called when the LPUART is between transmit and receive packets.
 *
 * @param baseAddr LPUART base address.
 * @param enable Enable (1) or disable (0) receive inversion
 */
static inline void LPUART_HAL_SetRxInversionCmd(uint32_t baseAddr, bool enable)
{
    BW_LPUART_STAT_RXINV(baseAddr, enable);
}

/*!
 * @brief  Get LPUART tx/rx data register address.
 *
 * @return  LPUART tx/rx data register address.
 */
static inline uint32_t LPUART_HAL_GetDataRegAddr(uint32_t baseAddr)
{
    return (uint32_t)HW_LPUART_DATA_ADDR(baseAddr);
}

/*@}*/

/*!
 * @name LPUART Interrupts and DMA
 * @{
 */

/*!
 * @brief Configures the LPUART module interrupts to enable/disable various interrupt sources.
 *
 * @param   baseAddr LPUART module base address.
 * @param   interrupt LPUART interrupt configuration data.
 * @param   enable   true: enable, false: disable.
 */
void LPUART_HAL_SetIntMode(uint32_t baseAddr, lpuart_interrupt_t interrupt, bool enable);

/*!
 * @brief Returns whether the LPUART module interrupts is enabled/disabled.
 *
 * @param   baseAddr LPUART module base address.
 * @param   interrupt LPUART interrupt configuration data.
 * @return  true: enable, false: disable.
 */
bool LPUART_HAL_GetIntMode(uint32_t baseAddr, lpuart_interrupt_t interrupt);

/*!
 * @brief Enable/Disable the transmission_complete_interrupt.
 *
 * @param baseAddr LPUART base address
 * @param enable   true: enable, false: disable.
 */
static inline void LPUART_HAL_SetTxDataRegEmptyIntCmd(uint32_t baseAddr, bool enable)
{
    BW_LPUART_CTRL_TIE(baseAddr, enable);
}

/*!
 * @brief Gets the configuration of the transmission_data_register_empty_interrupt enable setting.
 *
 * @param baseAddr LPUART base address
 * @return  Bit setting of the interrupt enable bit
 */
static inline bool LPUART_HAL_GetTxDataRegEmptyIntCmd(uint32_t baseAddr)
{
    return BR_LPUART_CTRL_TIE(baseAddr);
}

/*!
 * @brief Enables the rx_data_register_full_interrupt.
 *
 * @param baseAddr LPUART base address
 * @param enable   true: enable, false: disable.
 */
static inline void LPUART_HAL_SetRxDataRegFullIntCmd(uint32_t baseAddr, bool enable)
{
    BW_LPUART_CTRL_RIE(baseAddr, enable);
}

/*!
 * @brief Gets the configuration of the rx_data_register_full_interrupt enable.
 *
 * @param baseAddr LPUART base address
 * @return Bit setting of the interrupt enable bit
 */
static inline bool LPUART_HAL_GetRxDataRegFullIntCmd(uint32_t baseAddr)
{
    return BR_LPUART_CTRL_RIE(baseAddr);
}

#if FSL_FEATURE_LPUART_HAS_DMA_ENABLE
/*!
 * @brief  LPUART configures DMA requests for Transmitter and Receiver.
 *
 * @param baseAddr LPUART base address
 * @param enable Transmit DMA request configuration (enable:1 /disable: 0)
 */
static inline void LPUART_HAL_SetTxDmaCmd(uint32_t baseAddr, bool enable)
{
    BW_LPUART_BAUD_TDMAE(baseAddr, enable);
}

/*!
 * @brief  LPUART configures DMA requests for Transmitter and Receiver.
 *
 * @param baseAddr LPUART base address
 * @param enable Receive DMA request configuration (enable: 1/disable: 0)
 */
static inline void LPUART_HAL_SetRxDmaCmd(uint32_t baseAddr, bool enable)
{
    BW_LPUART_BAUD_RDMAE(baseAddr, enable);
}

/*!
 * @brief  Gets the LPUART Transmit DMA request configuration.
 *
 * @param baseAddr LPUART base address
 * @return Transmit DMA request configuration (enable: 1/disable: 0)
 */
static inline bool LPUART_HAL_IsTxDmaEnabled(uint32_t baseAddr)
{
    return BR_LPUART_BAUD_TDMAE(baseAddr);
}

/*!
 * @brief  Gets the LPUART receive DMA request configuration.
 *
 * @param baseAddr LPUART base address
 * @return Receives the DMA request configuration (enable: 1/disable: 0).
 */
static inline bool LPUART_HAL_IsRxDmaEnabled(uint32_t baseAddr)
{
    return BR_LPUART_BAUD_RDMAE(baseAddr);
}

#endif

/*@}*/

/*!
 * @name LPUART Transfer Functions
 * @{
 */

/*!
 * @brief Sends the LPUART 8-bit character.
 *
 * @param baseAddr LPUART Instance
 * @param data     data to send (8-bit)
 */
static inline void LPUART_HAL_Putchar(uint32_t baseAddr, uint8_t data)
{
    HW_LPUART_DATA_WR(baseAddr, data);
}

/*!
 * @brief Sends the LPUART 9-bit character.
 *
 * @param baseAddr LPUART Instance
 * @param data     data to send (9-bit)
 */
void LPUART_HAL_Putchar9(uint32_t baseAddr, uint16_t data);

/*!
 * @brief Sends the LPUART 10-bit character (Note: Feature available on select LPUART instances).
 *
 * @param baseAddr LPUART Instance
 * @param data   data to send (10-bit)
 * @return An error code or kStatus_Success
 */
lpuart_status_t LPUART_HAL_Putchar10(uint32_t baseAddr, uint16_t data);

/*!
 * @brief Gets the LPUART 8-bit character.
 *
 * @param baseAddr LPUART base address
 * @param readData Data read from receive (8-bit)
 */
static inline void  LPUART_HAL_Getchar(uint32_t baseAddr, uint8_t *readData)
{
    *readData = (uint8_t)HW_LPUART_DATA_RD(baseAddr);
}

/*!
 * @brief Gets the LPUART 9-bit character.
 *
 * @param baseAddr LPUART base address
 * @param readData Data read from receive (9-bit)
 */
void LPUART_HAL_Getchar9(uint32_t baseAddr, uint16_t *readData);

/*!
 * @brief Gets the LPUART 10-bit character.
 *
 * @param baseAddr LPUART base address
 * @param readData Data read from receive (10-bit)
 */
void LPUART_HAL_Getchar10(uint32_t baseAddr, uint16_t *readData);

/*!
 * @brief Send out multiple bytes of data using polling method.
 *
 * This function only supports 8-bit transaction.
 *
 * @param   baseAddr LPUART module base address.
 * @param   txBuff The buffer pointer which saves the data to be sent.
 * @param   txSize Size of data to be sent in unit of byte.
 */
void LPUART_HAL_SendDataPolling(uint32_t baseAddr, const uint8_t *txBuff, uint32_t txSize);

/*!
 * @brief Receive multiple bytes of data using polling method.
 *
 * This function only supports 8-bit transaction.
 *
 * @param   baseAddr LPUART module base address.
 * @param   rxBuff The buffer pointer which saves the data to be received.
 * @param   rxSize Size of data need to be received in unit of byte.
 * @return  Whether the transaction is success or rx overrun.
 */
lpuart_status_t LPUART_HAL_ReceiveDataPolling(uint32_t baseAddr, uint8_t *rxBuff, uint32_t rxSize);

/*!
 * @brief Configures the number of idle characters that must be received before the IDLE flag is set.
 *
 * @param baseAddr LPUART base address
 * @param idleConfig Idle characters configuration
 */
static inline void LPUART_HAL_SetIdleChar(uint32_t baseAddr, lpuart_idle_char_t idleConfig)
{
    BW_LPUART_CTRL_IDLECFG(baseAddr, idleConfig);
}

/*!
 * @brief Gets the configuration of the number of idle characters that must be received
 * before the IDLE flag is set.
 *
 * @param baseAddr LPUART base address
 * @return  idle characters configuration
 */
static inline lpuart_idle_char_t LPUART_HAL_GetIdleChar(uint32_t baseAddr)
{
    return (lpuart_idle_char_t)BR_LPUART_CTRL_IDLECFG(baseAddr);
}

/*!
 * @brief  Checks whether the current data word was received with noise.
 *
 * @param baseAddr LPUART base address.
 * @return The status of the NOISY bit in the LPUART extended data register
 */
static inline bool LPUART_HAL_IsCurrentDataWithNoise(uint32_t baseAddr)
{
    return BR_LPUART_DATA_NOISY(baseAddr);
}

/*!
 * @brief Checks whether the current data word was received with frame error.
 *
 * @param baseAddr LPUART base address
 * @return The status of the FRETSC bit in the LPUART extended data register
 */
static inline bool LPUART_HAL_IsCurrentDataWithFrameError(uint32_t baseAddr)
{
    return BR_LPUART_DATA_FRETSC(baseAddr);
}

/*!
 * @brief Set this bit to indicate a break or idle character is to be transmitted
 *        instead of the contents in DATA[T9:T0].
 *
 * @param baseAddr LPUART base address
 * @param specialChar T9 is used to indicate a break character when 0 an idle
 * character when 1, the contents of DATA[T8:T0] should be zero.
 */
static inline void LPUART_HAL_SetTxSpecialChar(uint32_t baseAddr, uint8_t specialChar)
{
    BW_LPUART_DATA_FRETSC(baseAddr, specialChar);
}

/*!
 * @brief Checks whether the current data word was received with parity error.
 *
 * @param baseAddr LPUART base address
 * @return The status of the PARITYE bit in the LPUART extended data register
 */
static inline bool LPUART_HAL_IsCurrentDataWithParityError(uint32_t baseAddr)
{
    return BR_LPUART_DATA_PARITYE(baseAddr);
}

/*!
 * @brief Checks whether the receive buffer is empty.
 *
 * @param baseAddr LPUART base address
 * @return TRUE if the receive-buffer is empty, else FALSE.
 */
static inline bool LPUART_HAL_IsReceiveBufferEmpty(uint32_t baseAddr)
{
    return BR_LPUART_DATA_RXEMPT(baseAddr);
}

/*!
 * @brief Checks whether the previous BUS state was idle before this byte is received.
 *
 * @param baseAddr LPUART base address
 * @return TRUE if the previous BUS state was IDLE, else FALSE.
 */
static inline bool LPUART_HAL_WasPreviousReceiverStateIdle(uint32_t baseAddr)
{
    return BR_LPUART_DATA_IDLINE(baseAddr);
}

/*@}*/

/*!
 * @name LPUART Special Feature Configurations
 * @{
 */

/*!
 * @brief Configures the LPUART operation in wait mode (operates or stops operations in wait mode).
 *
 * In some LPUART instances, the user should disable the transmitter/receiver
 * before calling this function.
 * Generally, this may be applied to all LPUARTs to ensure safe operation.
 *
 * @param baseAddr LPUART base address
 * @param mode     LPUART wait mode operation - operates or stops to operate in wait mode.
 */
static inline void  LPUART_HAL_SetWaitModeOperation(uint32_t baseAddr, lpuart_operation_config_t mode)
{
    /* In CPU wait mode: 0 - lpuart clocks continue to run; 1 - lpuart clocks freeze */
    BW_LPUART_CTRL_DOZEEN(baseAddr, mode);
}

/*!
 * @brief Gets the LPUART operation in wait mode (operates or stops operations in wait mode).
 *
 * @param baseAddr LPUART base address
 * @return LPUART wait mode operation configuration
 *         - kLpuartOperates or KLpuartStops in wait mode
 */
static inline lpuart_operation_config_t LPUART_HAL_GetWaitModeOperation(uint32_t baseAddr)
{
    /* In CPU wait mode: 0 - lpuart clocks continue to run; 1 - lpuart clocks freeze  */
    return (lpuart_operation_config_t)BR_LPUART_CTRL_DOZEEN(baseAddr);
}

/*!
 * @brief Configures the LPUART loopback operation (enable/disable loopback operation)
 *
 * In some LPUART instances, the user should disable the transmitter/receiver
 * before calling this function.
 * Generally, this may be applied to all LPUARTs to ensure safe operation.
 *
 * @param baseAddr LPUART base address
 * @param enable   LPUART loopback mode - disabled (0) or enabled (1)
 */
void LPUART_HAL_SetLoopbackCmd(uint32_t baseAddr, bool enable);

/*!
 * @brief Configures the LPUART single-wire operation (enable/disable single-wire mode)
 *
 * In some LPUART instances, the user should disable the transmitter/receiver
 * before calling this function.
 * Generally, this may be applied to all LPUARTs to ensure safe operation.
 *
 * @param baseAddr LPUART base address
 * @param enable   LPUART loopback mode - disabled (0) or enabled (1)
 */
void LPUART_HAL_SetSingleWireCmd(uint32_t baseAddr, bool enable);

/*!
 * @brief Configures the LPUART transmit direction while in single-wire mode.
 *
 * @param baseAddr LPUART base address
 * @param direction LPUART single-wire transmit direction - input or output
 */
static inline void LPUART_HAL_SetTxdirInSinglewireMode(uint32_t baseAddr,
                                                 lpuart_singlewire_txdir_t direction)
{
    BW_LPUART_CTRL_TXDIR(baseAddr, direction);
}

/*!
 * @brief  Places the LPUART receiver in standby mode.
 *
 * @param baseAddr LPUART base address
 * @return Error code or kStatus_Success
 */
lpuart_status_t LPUART_HAL_SetReceiverInStandbyMode(uint32_t baseAddr);

/*!
 * @brief  Places the LPUART receiver in a normal mode (disable standby mode operation).
 *
 * @param baseAddr LPUART base address
 */
static inline void LPUART_HAL_PutReceiverInNormalMode(uint32_t baseAddr)
{
    BW_LPUART_CTRL_RWU(baseAddr, 0);
}

/*!
 * @brief  Checks whether the LPUART receiver is in a standby mode.
 *
 * @param baseAddr LPUART base address
 * @return LPUART in normal more (0) or standby (1)
 */
static inline bool LPUART_HAL_IsReceiverInStandby(uint32_t baseAddr)
{
    return BR_LPUART_CTRL_RWU(baseAddr);
}

/*!
 * @brief  LPUART receiver wakeup method (idle line or addr-mark) from standby mode
 *
 * @param baseAddr LPUART base address
 * @param method   LPUART wakeup method: 0 - Idle-line wake (default), 1 - addr-mark wake
 */
static inline void LPUART_HAL_SetReceiverWakeupMode(uint32_t baseAddr,
                                                    lpuart_wakeup_method_t method)
{
    BW_LPUART_CTRL_WAKE(baseAddr, method);
}

/*!
 * @brief  Gets the LPUART receiver wakeup method (idle line or addr-mark) from standby mode.
 *
 * @param baseAddr LPUART base address
 * @return  LPUART wakeup method: kLpuartIdleLineWake: 0 - Idle-line wake (default),
 *          kLpuartAddrMarkWake: 1 - addr-mark wake
 */
static inline lpuart_wakeup_method_t LPUART_HAL_GetReceiverWakeupMode(uint32_t baseAddr)
{
    return (lpuart_wakeup_method_t)BR_LPUART_CTRL_WAKE(baseAddr);
}

/*!
 * @brief  LPUART idle-line detect operation configuration (idle line bit-count start and wake
 *         up affect on IDLE status bit).
 *
 * In some LPUART instances, the user should disable the transmitter/receiver
 * before calling this function.
 * Generally, this may be applied to all LPUARTs to ensure safe operation.
 *
 * @param baseAddr LPUART base address
 * @param config   LPUART configuration data for idle line detect operation
 */
void LPUART_HAL_SetIdleLineDetect(uint32_t baseAddr,
                                  const lpuart_idle_line_config_t *config);

/*!
 * @brief  LPUART break character transmit length configuration
 *
 * In some LPUART instances, the user should disable the transmitter before calling
 * this function. Generally, this may be applied to all LPUARTs to ensure safe operation.
 *
 * @param baseAddr LPUART base address
 * @param length   LPUART break character length setting: 0 - minimum 10-bit times (default),
 *                   1 - minimum 13-bit times
 */
static inline void LPUART_HAL_SetBreakCharTransmitLength(uint32_t baseAddr,
                                             lpuart_break_char_length_t length)
{
    BW_LPUART_STAT_BRK13(baseAddr, length);
}

/*!
 * @brief  LPUART break character detect length configuration
 *
 * @param baseAddr LPUART base address
 * @param length  LPUART break character length setting: 0 - minimum 10-bit times (default),
 *                  1 - minimum 13-bit times
 */
static inline void LPUART_HAL_SetBreakCharDetectLength(uint32_t baseAddr,
                                           lpuart_break_char_length_t length)
{
    BW_LPUART_STAT_LBKDE(baseAddr, length);
}

/*!
 * @brief  LPUART transmit sends break character configuration.
 *
 * @param baseAddr LPUART base address
 * @param enable LPUART normal/queue break char - disabled (normal mode, default: 0) or
 *                 enabled (queue break char: 1)
 */
static inline void LPUART_HAL_QueueBreakCharToSend(uint32_t baseAddr, bool enable)
{
    BW_LPUART_CTRL_SBK(baseAddr, enable);
}

/*!
 * @brief LPUART configures match address mode control
 *
 * @param baseAddr LPUART base address
 * @param config MATCFG: Configures the match addressing mode used.
 */
static inline void LPUART_HAL_SetMatchAddressMode(uint32_t baseAddr, lpuart_match_config_t config)
{
    BW_LPUART_BAUD_MATCFG(baseAddr, config);
}

/*!
 * @brief Configures address match register 1
 *
 * The MAEN bit must be cleared before configuring MA value, so the enable/disable
 * and set value must be included inside one function.
 *
 * @param baseAddr LPUART base address
 * @param enable Match address model enable (true)/disable (false)
 * @param value Match address value to program into match address register 1
 */
void LPUART_HAL_SetMatchAddressReg1(uint32_t baseAddr, bool enable, uint8_t value);

/*!
 * @brief Configures address match register 2
 *
 * The MAEN bit must be cleared before configuring MA value, so the enable/disable
 * and set value must be included inside one function.
 *
 * @param baseAddr LPUART base address
 * @param enable Match address model enable (true)/disable (false)
 * @param value Match address value to program into match address register 2
 */
void LPUART_HAL_SetMatchAddressReg2(uint32_t baseAddr, bool enable, uint8_t value);

/*!
 * @brief LPUART sends the MSB first configuration
 *
 * In some LPUART instances, the user should disable the transmitter/receiver
 * before calling this function.
 * Generally, this may be applied to all LPUARTs to ensure safe operation.
 *
 * @param baseAddr LPUART base address
 * @param enable  false - LSB (default, disabled), true - MSB (enabled)
 */
static inline void LPUART_HAL_SetSendMsbFirstCmd(uint32_t baseAddr, bool enable)
{
    BW_LPUART_STAT_MSBF(baseAddr, enable);
}

/*!
 * @brief  LPUART enable/disable re-sync of received data configuration
 *
 * @param baseAddr LPUART base address
 * @param enable  re-sync of received data word configuration:
 *                true - re-sync of received data word (default)
 *                false - disable the re-sync
 */
static inline void LPUART_HAL_SetReceiveResyncCmd(uint32_t baseAddr, bool enable)
{
    /* When disabled, the resynchronization of the received data word when a data
     * one followed by data zero transition is detected. This bit should only be
     * changed when the receiver is disabled. */
    BW_LPUART_BAUD_RESYNCDIS(baseAddr, enable);
}

#if FSL_FEATURE_LPUART_HAS_MODEM_SUPPORT
/*!
 * @brief  Transmits the CTS source configuration.
 *
 * @param baseAddr LPUART base address
 * @param source   LPUART CTS source
 */
static inline void LPUART_HAL_SetCtsSource(uint32_t baseAddr,
                                           lpuart_cts_source_t source)
{
    BW_LPUART_MODIR_TXCTSSRC(baseAddr, source);
}

/*!
 * @brief  Transmits the CTS configuration.
 *
 * Note: configures if the CTS state is checked at the start of each character
 * or only when the transmitter is idle.
 *
 * @param baseAddr LPUART base address
 * @param mode     LPUART CTS configuration
 */
static inline void LPUART_HAL_SetCtsMode(uint32_t baseAddr, lpuart_cts_config_t mode)
{
    BW_LPUART_MODIR_TXCTSC(baseAddr, mode);
}

/*!
 * @brief Enable/Disable the transmitter clear-to-send.
 *
 * @param baseAddr LPUART base address
 * @param enable  disable(0)/enable(1) transmitter CTS.
 */
static inline void LPUART_HAL_SetTxCtsCmd(uint32_t baseAddr, bool enable)
{
    BW_LPUART_MODIR_TXCTSE(baseAddr, enable);
}

/*!
 * @brief  Enable/Disable the receiver request-to-send.
 *
 * Note: do not enable both Receiver RTS (RXRTSE) and Transmit RTS (TXRTSE).
 *
 * @param baseAddr LPUART base address
 * @param enable  disable(0)/enable(1) receiver RTS.
 */
static inline void LPUART_HAL_SetRxRtsCmd(uint32_t baseAddr, bool enable)
{
    BW_LPUART_MODIR_RXRTSE(baseAddr, enable);
}

/*!
 * @brief  Enable/Disable the transmitter request-to-send.
 * Note: do not enable both Receiver RTS (RXRTSE) and Transmit RTS (TXRTSE).
 *
 * @param baseAddr LPUART base address
 * @param enable  disable(0)/enable(1) transmitter RTS.
 */
static inline void LPUART_HAL_SetTxRtsCmd(uint32_t baseAddr, bool enable)
{
    BW_LPUART_MODIR_TXRTSE(baseAddr, enable);
}

/*!
 * @brief Configures the transmitter RTS polarity.
 *
 * @param baseAddr LPUART base address
 * @param polarity Settings to choose RTS polarity (0=active low, 1=active high)
 */
static inline void LPUART_HAL_SetTxRtsPolarityMode(uint32_t baseAddr, bool polarity)
{
    BW_LPUART_MODIR_TXRTSPOL(baseAddr, polarity);
}

#endif  /* FSL_FEATURE_LPUART_HAS_MODEM_SUPPORT */

#if FSL_FEATURE_LPUART_HAS_IR_SUPPORT
/*!
 * @brief  Configures the LPUART infrared operation.
 *
 * @param baseAddr LPUART base address
 * @param enable    Enable (1) or disable (0) the infrared operation
 * @param pulseWidth The transmit narrow pulse width of type lpuart_ir_tx_pulsewidth_t
 */
void LPUART_HAL_SetInfrared(uint32_t baseAddr, bool enable,
                            lpuart_ir_tx_pulsewidth_t pulseWidth);
#endif  /* FSL_FEATURE_LPUART_HAS_IR_SUPPORT */

/*@}*/

/*!
 * @name LPUART Status Flags
 * @{
 */

/*!
 * @brief  LPUART get status flag
 *
 * @param baseAddr LPUART base address
 * @param statusFlag  The status flag to query
 * @return Whether the current status flag is set(true) or not(false).
 */
bool LPUART_HAL_GetStatusFlag(uint32_t baseAddr, lpuart_status_flag_t statusFlag);

/*!
 * @brief  Gets the LPUART Transmit data register empty flag.
 *
 * This function returns the state of the LPUART Transmit data register empty flag.
 *
 * @param baseAddr LPUART module base address.
 * @return The status of Transmit data register empty flag, which is set when
 * transmit buffer is empty.
 */
static inline bool LPUART_HAL_IsTxDataRegEmpty(uint32_t baseAddr)
{
    return BR_LPUART_STAT_TDRE(baseAddr);
}

/*!
 * @brief  Gets the LPUART receive data register full flag.
 *
 * @param baseAddr LPUART base address
 * @return Status of the receive data register full flag, sets when the receive data buffer is full.
 */
static inline bool LPUART_HAL_IsRxDataRegFull(uint32_t baseAddr)
{
    return BR_LPUART_STAT_RDRF(baseAddr);
}

/*!
 * @brief  Gets the LPUART transmission complete flag.
 *
 * @param   baseAddr LPUART base address
 * @return  Status of Transmission complete flag, sets when transmitter is idle
 *          (transmission activity complete)
 */
static inline bool LPUART_HAL_IsTxComplete(uint32_t baseAddr)
{
    return BR_LPUART_STAT_TC(baseAddr);
}

/*!
 * @brief LPUART clears an individual status flag (see lpuart_status_flag_t for list of status bits).
 *
 * @param baseAddr LPUART base address
 * @param statusFlag  Desired LPUART status flag to clear
 * @return An error code or kStatus_Success
 */
lpuart_status_t LPUART_HAL_ClearStatusFlag(uint32_t baseAddr, lpuart_status_flag_t statusFlag);

/*@}*/

#if defined(__cplusplus)
}
#endif

/*! @}*/

#endif /* __FSL_LPUART_HAL_H__ */
/*******************************************************************************
 * EOF
 ******************************************************************************/

