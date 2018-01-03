/**
 ****************************************************************************************
 *
 * @file lld_data.h
 *
 * @brief Functions for data transmission/reception handling
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 *
 ****************************************************************************************
 */

#ifndef LLD_DATA_H_
#define LLD_DATA_H_

/**
 ****************************************************************************************
 * @addtogroup LLDDATA LLDDATA
 * @ingroup LLD
 * @brief Functions for data transmission/reception handling
 *
 * This module implements the primitives allowing the LLC asking for data transmission. It
 * configures the Tx lists and handles the reception.
 *
 * @{
 ****************************************************************************************
 */


/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include <stdint.h>
#include "co_buf.h"
#include "lld_evt.h"

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */
/**
 ****************************************************************************************
 * @brief Ask for a data transmission
 *
 * This function is called by the LLC or LLM to request for a transmission. It chains the
 * descriptor passed as parameter in the list of descriptors ready for transmission.
 *
 * @param[in] evt    Pointer to the event for which a transmission is requested
 * @param[in] txnode Pointer to the TX node
 *
 ****************************************************************************************
 */
void lld_data_tx_push(struct lld_evt_tag *evt, struct co_buf_tx_node *txnode);

/**
 ****************************************************************************************
 * @brief Loop back the TX data in the exchange memory
 *
 * This function chains the next descriptor pointer of the last TX descriptor with the
 * first TX descriptor. It therefore creates a loop.
 *
 * @param[in] evt Event associated with the data to loop
 *
 ****************************************************************************************
 */
void lld_data_tx_loop(struct lld_evt_tag *evt);

/**
 ****************************************************************************************
 * @brief Prepare buffer pointers in the control structure for TX
 *
 * This function is called by the event scheduler when an event for a connection is ready
 * to be programmed. It chains the descriptors ready for transmissions with the ones
 * already programmed, and update the control structure with the pointer to the first
 * descriptor.
 *
 * @param[in] evt Event for which the buffers have to be programmed
 *
 ****************************************************************************************
 */
void lld_data_tx_prog(struct lld_evt_tag *evt);

/**
 ****************************************************************************************
 * @brief Check the data that has been transmitted/received during an event
 *
 * This function is called by the event scheduler at the end of the event corresponding
 * to the connection handle passed as parameter or upon an RX interrupt. It confirms the
 * TX buffers that have been transmitted and indicates the received packets.
 *
 * @param[in] evt     Event tag for which data transmission/reception have to be checked
 * @param[in] rx_cnt  Number of received buffer that have to be handled
 * @param[in] rx_only Flag indicating if only received buffers have to be handled or both
 *                    RX and TX
 *
 *
 ****************************************************************************************
 */
void lld_data_check(struct lld_evt_tag *evt, uint8_t rx_cnt, bool rx_only);

/**
 ****************************************************************************************
 * @brief Flush the data currently programmed for transmission
 *
 * @param[in] evt Pointer to the event for which TX data has to be flushed
 ****************************************************************************************
 */
void lld_data_tx_flush(struct lld_evt_tag *evt);

/// @} LLDDATA

#endif // LLD_DATA_H_
