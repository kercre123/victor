/**
 ****************************************************************************************
 *
 * @file co_buf.h
 *
 * @brief Rx and Tx buffer management
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 *
 ****************************************************************************************
 */

#ifndef CO_BUF_H_
#define CO_BUF_H_

/**
 ****************************************************************************************
 * @addtogroup CO_BUF Buffer management
 * @ingroup COMMON
 * @brief Rx and Tx buffer management
 *
 * @{
 ****************************************************************************************
 */


/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include <stdlib.h>         // standard lib definitions
#include <stdint.h>         // standard integer
#include "rwip_config.h"    // stack configuration
#include "co_list.h"        // common list definition
#include "co_bt.h"          // common bt definitions

/*
 * DEFINES
 ****************************************************************************************
 */
 
/// Invalid RX handle
#define CO_BUF_RX_INVALID   0xFF


/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

#if (BLE_EMB_PRESENT)
/// Rx buffer tag
struct co_buf_rx_desc
{
    /// rx pointer
    uint16_t rxptr;
    /// status
    uint16_t rxstatus;
    /// rx header
    uint16_t rxheader;
    /// rx chass
    uint16_t rxchass;
    /// data
    uint8_t data[38];
};
#elif (BLE_HOST_PRESENT)
/// Rx buffer tag
struct co_buf_rx_desc
{
    bool used;
    uint8_t *data;
};
#endif //BLE_EMB_PRESENT / BLE_HOST_PRESENT

#if (BLE_EMB_PRESENT || BLE_HOST_PRESENT)
/// Tx desc tag
struct co_buf_tx_desc
{
    /// tx pointer
    uint16_t txptr;
    /// tx header
    uint16_t txheader;
    /// data
    uint8_t data[38];
};
/// Tx buffer tag
struct co_buf_tx_node
{
    struct co_list_hdr hdr;
    uint16_t idx;
};
#endif //BLE_EMB_PRESENT || BLE_HOST_PRESENT

#if (BLE_EMB_PRESENT)
/// Common buffer management environment
struct co_buf_env_tag
{
    /// List of free TX descriptors
    struct co_list tx_free;
    /// Array of TX descriptors (SW tag)
    struct co_buf_tx_node tx_node[BLE_TX_BUFFER_CNT];
    /// Pointer to TX descriptors
    struct co_buf_tx_desc *tx_desc;
    /// Pointer to TX descriptors
    struct co_buf_rx_desc *rx_desc;
    /// Index of the current RX buffer
    uint8_t rx_current;
};
#elif (BLE_HOST_PRESENT)
/// Common buffer management environment
struct co_buf_env_tag
{
    /// List of free TX descriptors
    struct co_list tx_free;
    /// Array of TX descriptors (SW tag)
    struct co_buf_tx_node tx_node[BLE_TX_BUFFER_CNT];
    /// Array of TX descriptors
    struct co_buf_tx_desc tx_desc[BLE_TX_BUFFER_CNT];
    /// Array of RX descriptors
    struct co_buf_rx_desc rx_desc[BLE_RX_BUFFER_CNT];
    /// Index of the current RX buffer
    uint8_t rx_current;
};
#endif //BLE_EMB_PRESENT / BLE_HOST_PRESENT

/*
 * GLOBAL VARIABLE DECLARATIONS
 ****************************************************************************************
 */
 
#if (BLE_EMB_PRESENT || BLE_HOST_PRESENT)
extern struct co_buf_env_tag co_buf_env;
#endif //BLE_EMB_PRESENT || BLE_HOST_PRESENT

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */
 
/**
 ****************************************************************************************
 * @brief Initialize all the buffer pools.
 *
 ****************************************************************************************
 */
void co_buf_init(void);
void co_buf_init_deep_sleep(void);
#if (BLE_EMB_PRESENT || BLE_HOST_PRESENT)

/**
 ****************************************************************************************
 * @brief Freeing of a RX buffer
 *
 * @param hdl Handle of the RX buffer to be freed
 *
 ****************************************************************************************
 */
void co_buf_rx_free(uint8_t hdl);

/**
 ****************************************************************************************
 * @brief Returns the pointer to the TX descriptor from the index
 *
 * @return The pointer to the TX descriptor corresponding to the index.
 *
 ****************************************************************************************
 */
__INLINE struct co_buf_tx_desc *co_buf_tx_desc_get(uint16_t idx)
{
    // Pop a descriptor from the TX free list
    return(&co_buf_env.tx_desc[idx]);
}

/**
 ****************************************************************************************
 * @brief Returns the pointer to the TX node from the index
 *
 * @return The pointer to the TX node corresponding to the index.
 *
 ****************************************************************************************
 */
__INLINE struct co_buf_tx_node *co_buf_tx_node_get(uint16_t idx)
{
    // Pop a descriptor from the TX free list
    return(&co_buf_env.tx_node[idx]);
}


/**
 ****************************************************************************************
 * @brief Allocation of a TX data buffer
 *
 * @return The pointer to the TX descriptor corresponding to the allocated buffer, NULL if
 *         no buffers are available anymore.
 *
 ****************************************************************************************
 */
__INLINE struct co_buf_tx_node *co_buf_tx_alloc(void)
{
    // Pop a descriptor from the TX free list
    return((struct co_buf_tx_node *)co_list_pop_front(&co_buf_env.tx_free));
}


/**
 ****************************************************************************************
 * @brief Get the handle of the next RX buffer in the list
 *
 ****************************************************************************************
 */
__INLINE uint8_t co_buf_rx_current_get(void)
{
    return (co_buf_env.rx_current);
}

/**
 ****************************************************************************************
 * @brief Set the current RX buffer
 *
 * @param hdl Handle of the current RX buffer
 *
 ****************************************************************************************
 */
__INLINE void co_buf_rx_current_set(uint8_t hdl)
{
    co_buf_env.rx_current = hdl;
}

/**
 ****************************************************************************************
 * @brief Get the handle of the next RX buffer in the list
 *
 * @param hdl Handle of the current RX buffer
 *
 ****************************************************************************************
 */
__INLINE uint8_t co_buf_rx_next(uint8_t hdl)
{
    return ((hdl + 1) % BLE_RX_BUFFER_CNT);
}

/**
 ****************************************************************************************
 * @brief Get the current RX buffer descriptor pointer and move to the next one
 *
 * @return The pointer to the RX buffer descriptor corresponding to handle.
 *
 ****************************************************************************************
 */
__INLINE struct co_buf_rx_desc *co_buf_rx_get(uint8_t hdl)
{
    return (&co_buf_env.rx_desc[hdl]);
}


/**
 ****************************************************************************************
 * @brief Freeing of a TX buffer
 *
 * @param buf  The pointer to the TX descriptor to be freed
 *
 ****************************************************************************************
 */
__INLINE void co_buf_tx_free(struct co_buf_tx_node *buf)
{
    // Push back the buffer in the TX free list
    co_list_push_back(&co_buf_env.tx_free, &buf->hdr);
}

#endif //BLE_EMB_PRESENT || BLE_HOST_PRESENT


/// @} CO_BUF

#endif // CO_BUF_H_
