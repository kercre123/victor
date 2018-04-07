/**
 ****************************************************************************************
 *
 * @file rwble.h
 *
 * @brief Entry points of the BLE software
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 ****************************************************************************************
 */

#ifndef RWBLE_H_
#define RWBLE_H_

/**
 ****************************************************************************************
 * @addtogroup ROOT
 * @brief Entry points of the BLE stack
 *
 * This module contains the primitives that allow an application accessing and running the
 * BLE protocol stack
 *
 * @{
 ****************************************************************************************
 */


/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include <stdint.h>
#include <stdbool.h>
#include "rwip_config.h"
#include "compiler.h"
#include "co_error.h"

/*
 * DEFINITIONS
 ****************************************************************************************
 */

#define RX_END_SERVICE 1
#define EVT_END_SERVICE 2

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Initialize the BLE stack.
 ****************************************************************************************
 */
void rwble_init(void);

/**
 ****************************************************************************************
 * @brief Reset the BLE stack.
 ****************************************************************************************
 */
void rwble_reset(void);

/**
 ****************************************************************************************
 * @brief Gives FW/HW versions of RW-BLE stack.
 *
 ****************************************************************************************
 */
void rwble_version(uint8_t* fw_version, uint8_t* hw_version);

/**
 ****************************************************************************************
 * @brief Retrieve the slot clock.
 *
 * @return  Slot clock (x 625us)
 ****************************************************************************************
 */
uint32_t rwble_get_clock(void);

/**
 ****************************************************************************************
 * @brief Send an error message to Host.
 *
 * This function is used to send an error message to Host from platform.
 *
 * @param[in] error    Error detected by FW
 ****************************************************************************************
 */
void rwble_send_message(uint32_t error);


/**
 ****************************************************************************************
 * @brief RWBLE interrupt service routine
 *
 * This function is the interrupt service handler of RWBLE.
 *
 ****************************************************************************************
 */
__BLEIRQ void rwble_isr(void);

bool func_check_mem(void);
bool test_rxdone(void);
void measure_errors_received(unsigned char pkts);
/// @} RWBLE

#endif // RWBLE_H_
