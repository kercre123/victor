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
#ifndef __FSL_DMA_HAL_H__
#define __FSL_DMA_HAL_H__

#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include "fsl_device_registers.h"

/*!
 * @addtogroup dma_hal
 * @{
 */

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*! @brief DMA status */
typedef enum _dma_status
{
    kStatus_DMA_Success = 0U,
    kStatus_DMA_InvalidArgument = 1U, /*!< Parameter is not available for the current
                                                     configuration. */
    kStatus_DMA_Fail = 2U             /*!< Function operation  failed. */
} dma_status_t;

/*! @brief DMA transfer size type*/
typedef enum _dma_transfer_size {
    kDmaTransfersize32bits = 0x0U,    /*!< 32 bits are transferred for every read/write */
    kDmaTransfersize8bits = 0x1U,     /*!< 8 bits are transferred for every read/write */
    kDmaTransfersize16bits = 0x2U     /*!< 16b its are transferred for every read/write */
} dma_transfer_size_t;

/*! @brief Configuration type for the DMA modulo */
typedef enum _dma_modulo {
    kDmaModuloDisable = 0x0U,
    kDmaModulo16Bytes = 0x1U,
    kDmaModulo32Bytes = 0x2U,
    kDmaModulo64Bytes = 0x3U,
    kDmaModulo128Bytes = 0x4U,
    kDmaModulo256Bytes = 0x5U,
    kDmaModulo512Bytes = 0x6U,
    kDmaModulo1KBytes = 0x7U,
    kDmaModulo2KBytes = 0x8U,
    kDmaModulo4KBytes = 0x9U,
    kDmaModulo8KBytes = 0xaU,
    kDmaModulo16KBytes = 0xbU,
    kDmaModulo32KBytes = 0xcU,
    kDmaModulo64KBytes = 0xdU,
    kDmaModulo128KBytes = 0xeU,
    kDmaModulo256KBytes = 0xfU,
} dma_modulo_t;

/*! @brief DMA channel link type */
typedef enum _dma_channel_link_type {
    kDmaChannelLinkDisable = 0x0U,          /*!< No channel link */
    kDmaChannelLinkChan1AndChan2 = 0x1U,    /*!< Perform a link to channel 1 after each cycle-steal
                                                 transfer followed by a link and to channel 2 after the
                                                 BCR decrements to zeros. */
    kDmaChannelLinkChan1 = 0x2U,            /*!< Perform a link to channel 1 after each cycle-steal
                                                 transfer. */
    kDmaChannelLinkChan1AfterBCR0 = 0x3     /*!< Perform a link to channel1 after the BCR decrements
                                                 to zero.  */
} dma_channel_link_type_t;

/*! @brief Data structure for data structure configuration */
typedef struct DmaChannelLinkConfig {
    dma_channel_link_type_t linkType;   /*!< Channel link type */
    uint32_t channel1;                  /*!< Channel 1 configuration  */
    uint32_t channel2;                  /*!< Channel 2 configuration  */
} dma_channel_link_config_t;

/*! @brief Data structure to get status of the DMA channel status */
typedef union DmaErrorStatus {
    struct {
        uint32_t dmaBytesToBeTransffered : 24;  /*!< Bytes to be transferred */
        uint32_t dmaTransDone : 1;              /*!< DMA channel transfer is done. */
        uint32_t dmaBusy : 1;                   /*!< DMA is running. */
        uint32_t dmaPendingRequest : 1;         /*!< A transfer  remains. */
        uint32_t _reserved1 : 1;
        uint32_t dmaDestBusError : 1;           /*!< Bus error on destination address */
        uint32_t dmaSourceBusError : 1;         /*!< Bus error on source address */
        uint32_t dmaConfigError : 1;            /*!< Configuration error */
        uint32_t _reserved0 : 1;
    } u;
    uint32_t b;
} dma_error_status_t;

/*! @brief Type for DMA transfer. */
typedef enum _dma_transfer_type {
    kDmaPeripheralToMemory,     /*!< Transfer from the peripheral to memory */
    kDmaMemoryToPeripheral,     /*!< Transfer from the memory to peripheral */
    kDmaMemoryToMemory,         /*!< Transfer from the memory to memory */
    kDmaPeripheralToPeripheral  /*!< Transfer from the peripheral to peripheral */
} dma_transfer_type_t;

/*******************************************************************************
 * API
 ******************************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif

/*!
 * @name DMA HAL channel configuration
 * @{
 */

/*!
 * @brief Sets all registers of the channel to 0.
 *
 * @param baseAddr DMA baseAddr.
 * @param channel DMA channel.
 */
void DMA_HAL_Init(uint32_t baseAddr, uint32_t channel);

/*!
 * @brief Sets all registers of the channel to 0.
 *
 * @param baseAddr DMA baseAddr.
 * @param channel DMA channel.
 * @param size Size to be transferred on each DMA write/read. Source/Dest share the same write/read
 * size.
 * @param type Transfer type.
 * @param sourceAddr Source address.
 * @param destAddr Destination address.
 * @param length Bytes to be transferred.
 */
void DMA_HAL_ConfigTransfer(
    uint32_t baseAddr, uint32_t channel, dma_transfer_size_t size, dma_transfer_type_t type,
    uint32_t sourceAddr, uint32_t destAddr, uint32_t length);

/*!
 * @brief Configures the source address.
 *
 * Each SAR contains the byte address used by the DMA to read data. The SARn is typically
 * aligned on a 0-modulo-size boundary-that is on the natural alignment of the source data.
 * Bits 31-20 of this register must be written with one of the only four allowed values. Each of these
 * allowed values corresponds to a valid region of the devices' memory map. The allowed values
 * are:
 * 0x000x_xxxx
 * 0x1FFx_xxxx
 * 0x200x_xxxx
 * 0x400x_xxxx
 * After they are written with one of the allowed values, bits 31-20 read back as the written value.
 * After they are written with any other value, bits 31-20 read back as an indeterminate value.
 *
 * This function enables the request for a specified channel.
 *
 * @param baseAddr DMA baseAddr.
 * @param channel DMA channel.
 * @param address memory address pointing to the source address.
 */
static inline void DMA_HAL_SetSourceAddr(
        uint32_t baseAddr, uint32_t channel, uint32_t address)
{
    assert(channel < FSL_FEATURE_DMA_DMAMUX_CHANNELS);
    HW_DMA_SARn_WR(baseAddr, channel, address);
}

/*!
 * @brief Configures the source address.
 *
 * Each DAR contains the byte address used by the DMA to read data. The DARn is typically
 * aligned on a 0-modulo-size boundary-that is on the natural alignment of the source data.
 * Bits 31-20 of this register must be written with one of the only four allowed values. Each of these
 * allowed values corresponds to a valid region of the devices' memory map. The allowed values
 * are:
 * 0x000x_xxxx
 * 0x1FFx_xxxx
 * 0x200x_xxxx
 * 0x400x_xxxx
 * After they are written with one of the allowed values, bits 31-20 read back as the written value.
 * After they are written with any other value, bits 31-20 read back as an indeterminate value.
 *
 * This function enables the request for specified channel.
 *
 * @param baseAddr DMA baseAddr.
 * @param channel DMA channel.
 * @param address Destination address.
 */
static inline void DMA_HAL_SetDestAddr(
        uint32_t baseAddr, uint32_t channel, uint32_t address)
{
    assert(channel < FSL_FEATURE_DMA_DMAMUX_CHANNELS);
    HW_DMA_DARn_WR(baseAddr, channel, address);
}

/*!
 * @brief Configures the bytes to be transferred.
 *
 * Transfer bytes must be written with a value equal to or less than 0F_FFFFh. After being written
 * with a value in this range, bits 23-20 of the BCR read back as 1110b. A write to the BCR with a value
 * greater than 0F_FFFFh causes a configuration error when the channel starts to execute. After
 * they are written with a value in this range, bits 23-20 of BCR read back as 1111b.
 *
 * @param baseAddr DMA baseAddr.
 * @param channel DMA channel.
 * @param count bytes to be transferred.
 */
static inline void DMA_HAL_SetTransferCount(
                    uint32_t baseAddr, uint32_t channel, uint32_t count)
{
    assert(channel < FSL_FEATURE_DMA_DMAMUX_CHANNELS);
    BW_DMA_DSR_BCRn_BCR(baseAddr, channel, count);
}

/*!
 * @brief Gets the left bytes not to be transferred.
 *
 * @param baseAddr DMA baseAddr.
 * @param channel DMA channel.
 * @return unfinished bytes.
 */
static inline uint32_t DMA_HAL_GetUnfinishedByte(uint32_t baseAddr, uint32_t channel)
{
    assert(channel < FSL_FEATURE_DMA_DMAMUX_CHANNELS);
    return HW_DMA_DSR_BCRn_RD(baseAddr, channel) & BM_DMA_DSR_BCRn_BCR;
}

/*!
 * @brief Enables the interrupt for the DMA channel after the work is done.
 *
 * This function enables the request for specified channel.
 *
 * @param baseAddr DMA baseAddr.
 * @param channel DMA channel.
 * @param enable True means enable interrupt, false means disable.
 */
static inline void DMA_HAL_SetIntCmd(uint32_t baseAddr, uint8_t channel, bool enable)
{
    assert(channel < FSL_FEATURE_DMA_DMAMUX_CHANNELS);
    BW_DMA_DCRn_EINT(baseAddr, channel, enable);
}

/*!
 * @brief Configures the DMA transfer mode to cycle steal or continuous modes.
 *
 * If continuous mode is enabled, DMA continuously makes write/read transfers until BCR decrement to
 * 0. If continuous mode is disabled, DMA write/read is only triggered on every request.
 *s
 * @param baseAddr DMA baseAddr.
 * @param channel DMA channel.
 * @param enable 1 means cycle-steal mode, 0 means continuous mode.
 */
static inline void DMA_HAL_SetCycleStealCmd(
        uint32_t baseAddr, uint8_t channel, bool enable)
{
    assert(channel < FSL_FEATURE_DMA_DMAMUX_CHANNELS);
    BW_DMA_DCRn_CS(baseAddr, channel, enable);
}

/*!
 * @brief Configures the auto-align feature.
 *
 * If auto-align is enabled, the appropriate address register increments, regardless of whether it is a source increment or
 * a destination increment.
 *
 * @param baseAddr DMA baseAddr.
 * @param channel DMA channel.
 * @param enable 0 means disable auto-align. 1 means enable auto-align.
 */
static inline void DMA_HAL_SetAutoAlignCmd(
        uint32_t baseAddr, uint8_t channel, bool enable)
{
    assert(channel < FSL_FEATURE_DMA_DMAMUX_CHANNELS);
    BW_DMA_DCRn_AA(baseAddr, channel, enable);
}

/*!
 * @brief Configures the a-sync DMA request feature.
 *
 * Enables/Disables the a-synchronization mode in a STOP mode for each DMA channel.
 *
 * @param baseAddr DMA baseAddr.
 * @param channel DMA channel.
 * @param enable 0 means disable DMA request a-sync. 1 means enable DMA request -.
 */
static inline void DMA_HAL_SetAsyncDmaRequestCmd(
        uint32_t baseAddr, uint8_t channel, bool enable)
{
    assert(channel < FSL_FEATURE_DMA_DMAMUX_CHANNELS);
    BW_DMA_DCRn_EADREQ(baseAddr, channel, enable);
}

/*!
 * @brief Enables/Disables the source increment.
 *
 * Controls whether the source address increments after each successful transfer. If enabled, the
 * SAR increments by 1,2,4 as determined by the transfer size.
 *
 * @param baseAddr DMA baseAddr.
 * @param channel DMA channel.
 * @param enable Enabled/Disable increment.
 */
static inline void DMA_HAL_SetSourceIncrementCmd(
        uint32_t baseAddr, uint32_t channel, bool enable)
{
    assert(channel < FSL_FEATURE_DMA_DMAMUX_CHANNELS);
    BW_DMA_DCRn_SINC(baseAddr, channel, enable);
}

/*!
 * @brief Enables/Disables destination increment.
 *
 * Controls whether the destination address increments after each successful transfer. If enabled, the
 * DAR increments by 1,2,4 as determined by the transfer size.
 *
 * @param baseAddr DMA baseAddr.
 * @param channel DMA channel.
 * @param enable Enabled/Disable increment.
 */
static inline void DMA_HAL_SetDestIncrementCmd(
        uint32_t baseAddr, uint32_t channel, bool enable)
{
    assert(channel < FSL_FEATURE_DMA_DMAMUX_CHANNELS);
    BW_DMA_DCRn_DINC(baseAddr, channel, enable);
}

/*!
 * @brief Configures the source transfer size.
 *
 * @param baseAddr DMA baseAddr.
 * @param channel DMA channel.
 * @param transfersize enum type for transfer size.
 */
static inline void DMA_HAL_SetSourceTransferSize(
        uint32_t baseAddr, uint32_t channel, dma_transfer_size_t transfersize)
{
    assert(channel < FSL_FEATURE_DMA_DMAMUX_CHANNELS);
    BW_DMA_DCRn_SSIZE(baseAddr, channel, transfersize);
}

/*!
 * @brief Configures the destination transfer size.
 *
 * @param baseAddr DMA baseAddr.
 * @param channel DMA channel.
 * @param transfersize enum type for transfer size.
 */
static inline void DMA_HAL_SetDestTransferSize(
        uint32_t baseAddr, uint32_t channel, dma_transfer_size_t transfersize)
{
    assert(channel < FSL_FEATURE_DMA_DMAMUX_CHANNELS);
    BW_DMA_DCRn_DSIZE(baseAddr, channel, transfersize);
}

/*!
 * @brief Triggers the start.
 *
 * When the DMA begins the transfer, the START bit is cleared automatically after one module clock and always
 * reads as logic 0.
 *
 * @param baseAddr DMA baseAddr.
 * @param channel DMA channel.
 * @param enable Enable/disable trigger start.
 */
static inline void DMA_HAL_SetTriggerStartCmd(uint32_t baseAddr, uint32_t channel, bool enable)
{
    assert(channel < FSL_FEATURE_DMA_DMAMUX_CHANNELS);
    BW_DMA_DCRn_START(baseAddr, channel, enable);
}

/*!
 * @brief  Configures the modulo for the source address.
 *
 * @param baseAddr DMA baseAddr.
 * @param channel DMA channel.
 * @param modulo enum data type for source modulo.
 */
static inline void DMA_HAL_SetSourceModulo(
        uint32_t baseAddr, uint32_t channel, dma_modulo_t modulo)
{
    assert(channel < FSL_FEATURE_DMA_DMAMUX_CHANNELS);
    BW_DMA_DCRn_SMOD(baseAddr, channel, modulo);
}

/*!
 * @brief Configures the modulo for the destination address.
 *
 * @param baseAddr DMA baseAddr.
 * @param channel DMA channel.
 * @param modulo enum data type for dest modulo.
 */
static inline void DMA_HAL_SetDestModulo(
        uint32_t baseAddr, uint32_t channel, dma_modulo_t modulo)
{
    assert(channel < FSL_FEATURE_DMA_DMAMUX_CHANNELS);
    BW_DMA_DCRn_DMOD(baseAddr, channel, modulo);
}

/*!
 * @brief Enables/Disables the DMA request.
 *
 * @param baseAddr DMA baseAddr.
 * @param channel DMA channel.
 * @param enable Enable/disable dma request.
 */
static inline void DMA_HAL_SetDmaRequestCmd(
        uint32_t baseAddr, uint32_t channel, bool enable)
{
    assert(channel < FSL_FEATURE_DMA_DMAMUX_CHANNELS);
    BW_DMA_DCRn_ERQ(baseAddr, channel, enable);
}

/*!
 * @brief Configures the DMA request state after the work is done.
 *
 * Disables/Enables the DMA request after a DMA DONE is generated. If it works in the loop mode, this bit
 * should not be set.
 * @param baseAddr DMA base address.
 * @param channel DMA channel.
 * @param enable 0 means DMA request would not be disabled after work done. 1 means disable.
 */
static inline void DMA_HAL_SetDisableRequestAfterDoneCmd(
        uint32_t baseAddr, uint32_t channel, bool enable)
{
    assert(channel < FSL_FEATURE_DMA_DMAMUX_CHANNELS);
    BW_DMA_DCRn_D_REQ(baseAddr, channel, enable);

}

/*!
 * @brief Configures the channel link feature.
 *
 * @param baseAddr DMA base address.
 * @param channel DMA channel.
 * @param mode Mode of channel link in DMA.
 */
void DMA_HAL_SetChanLink(
        uint32_t baseAddr, uint8_t channel, dma_channel_link_config_t *mode);

/*!
 * @brief Clears the status of the DMA channel.
 *
 * This function clears the status for a specified DMA channel. The error status and done status
 * are cleared.
 * @param baseAddr DMA base address.
 * @param channel DMA channel.
 */
static inline void DMA_HAL_ClearStatus(uint32_t baseAddr, uint8_t channel)
{
    assert(channel < FSL_FEATURE_DMA_DMAMUX_CHANNELS);
    BW_DMA_DSR_BCRn_DONE(baseAddr, channel, 1U);
}

/*!
 * @brief Gets the DMA controller channel status.
 *
 * Gets the status of the DMA channel. The user can get the error status, as to whether the descriptor is finished or there are bytes left.
 * @param baseAddr DMA base address.
 * @param channel DMA channel.
 * @return Status of the DMA channel.
 */
static inline dma_error_status_t DMA_HAL_GetStatus(
        uint32_t baseAddr, uint8_t channel)
{
    assert(channel < FSL_FEATURE_DMA_DMAMUX_CHANNELS);

    dma_error_status_t status;

    status.b = HW_DMA_DSR_BCRn_RD(baseAddr, channel);

    return status;
}
/* @} */

#if defined(__cplusplus)
}
#endif

/*! @} */

#endif /* __FSL_DMA_HAL_H__ */
/*******************************************************************************
 * EOF
 ******************************************************************************/

