/**
 ****************************************************************************************
 *
 * @file hcih_eif.h
 *
 * @brief HCIH EIF transport module header file.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 *
 ****************************************************************************************
 */

#ifndef HCIH_EIF_H_
#define HCIH_EIF_H_

/**
 ****************************************************************************************
 * @addtogroup HCIH_EIF HCIH EIF
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

#include "rwip_config.h"     // SW configuration

#if (BLEHL_HCIH_ITF)

#include <stdint.h>       // standard integer definition
#include <stdbool.h>      // standard boolean definition


/*
 * DEFINES
 ****************************************************************************************
 */

///HCI TX states
enum HCIH_STATE_TX
{
     ///HCI TX Start State - when packet is ready to be sent
     HCIH_STATE_TX_ONGOING,
     ///HCI TX Done State - TX ended with no error
     HCIH_STATE_TX_IDLE,
};

/*
 * GLOBAL VARIABLE DECLARATIONS
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
 * @brief HCIH EIF transport initialization.
 *
 * Puts the UART driver in reception, waiting for simple 1 byte message type. Space for
 * reception is allocated with ke_msg_alloc and the pointer is handed to uart_env.rx. RX
 * interrupt is enabled.
 *
 *****************************************************************************************
 */
void hcih_eif_init(void);


/**
 ****************************************************************************************
 * @brief HCIH EIF write function.
 *
 *@param[in] msg   Pointer to the message to be transmitted.
 *****************************************************************************************
 */
void hcih_eif_write(struct ke_msg *msg);

#endif //BLEHL_HCIH_ITF

/// @} HCIH_EIF

#endif // HCIH_EIF_H_
