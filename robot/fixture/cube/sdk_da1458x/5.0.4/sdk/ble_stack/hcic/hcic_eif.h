/**
 ****************************************************************************************
 *
 * @file hci_eif.h
 *
 * @brief HCIH EIF transport module header file.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 *
 ****************************************************************************************
 */

#ifndef HCI_EIF_H_
#define HCI_EIF_H_

/**
 ****************************************************************************************
 * @addtogroup HCI_EIF HCIH EIF
 * @ingroup HCI
 * @brief HCIH EIF module.
 *
 * This module creates the abstraction between UART driver and HCI generic functions
 * (designed for any transport layer).
 *
 * @{
 ****************************************************************************************
 */


/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"  // stack configuration

#if (BLE_HCIC_ITF)

#include <stdint.h>       // standard integer definition
#include <stdbool.h>      // standard boolean definition

/*
 * DEFINES
 ****************************************************************************************
 */

// Forward declaration
struct ke_msg;

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief HCIH EIF write function.
 *
 *@param[in] msg   Pointer to the message to be transmitted.
 *****************************************************************************************
 */
void hci_eif_write(struct ke_msg *msg);

#if (DEEP_SLEEP)
/**
 ****************************************************************************************
 * @brief Start UART flow
 *
 *****************************************************************************************
 */
void hci_eif_start(void);

/**
 ****************************************************************************************
 * @brief Stop UART flow if possible
 *
 * @return true if UART flow was stopped, false otherwise
 *****************************************************************************************
 */
bool hci_eif_stop(void);
#endif //DEEP_SLEEP

#endif //BLE_HCIC_ITF

/**
 ****************************************************************************************
 * @brief HCIH EIF transport initialization.
 *
 * Puts the UART driver in reception, waiting for simple 1 byte message type. Space for
 * reception is allocated with ke_msg_alloc and the pointer is handed to uart_env.rx. RX
 * interrupt is enabled.
 *
 *****************************************************************************************
 */
void hci_eif_init(void);

/// @} HCI_EIF

#endif // HCI_EIF_H_
