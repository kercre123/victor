/**
 ****************************************************************************************
 *
 * @file gtl_hci.h
 *
 * @brief HCI part of the Generic Transport Layer
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 ****************************************************************************************
 */

#ifndef GTL_HCI_H_
#define GTL_HCI_H_

/**
 ****************************************************************************************
 * @addtogroup GTL Generic Transport Layer
 * @brief Generic Transport Layer, based on GTL functionality.
 *
 * This module creates the abstraction between UART driver and GTL generic functions
 * (designed for any transport layer).
 *
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"     // SW configuration

#if (GTL_ITF && BLE_EMB_PRESENT)

#include <stdint.h>


/*
 * DEFINES
 ****************************************************************************************
 */


/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief GTL HCI header reception handler
 *****************************************************************************************
 */
void gtl_hci_rx_header(void);

/**
 ****************************************************************************************
 * @brief GTL HCI payload reception handler
 *****************************************************************************************
 */
void gtl_hci_rx_payload(void);

/**
 ****************************************************************************************
 * @brief GTL HCI event formatter
 *
 * @param[out]  length   Length of the event
 * @return               Pointer to the event data
 *****************************************************************************************
 */
uint8_t * gtl_hci_tx_evt(uint8_t * length);

#endif //GTL_ITF && BLE_EMB_PRESENT

/// @} GTL_EIF
#endif // GTL_HCI_H_
