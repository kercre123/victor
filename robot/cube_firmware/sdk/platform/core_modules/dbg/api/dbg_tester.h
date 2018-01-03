/**
****************************************************************************************
*
* @file dbg_tester.h
*
* @brief Tester emulator
*
* Copyright (C) RivieraWaves 2009-2013
*
*
****************************************************************************************
*/

#ifndef DBG_TESTER_H_
#define DBG_TESTER_H_

/**
 ****************************************************************************************
 * @addtogroup DBGTESTER Tester
 * @ingroup DBG
 * @brief Debug tester emulator
 *
 * The DBG tester emulator enables low level base band tests such as ACL/SCO/ESCO
 * loopback.
 *
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "rwip_config.h"       // stack configuration

#if (BT_TESTER)

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
******************************************************************************************
* @brief Function called to handle reception of LMP during tester emulation.
*
* @param[in] link_id            Link Id
* @param[in] test_scenario      Test scenario to execute
* @param[in] hopping_mode       Hopping mode
* @param[in] tx_freq            Tx Frequency
* @param[in] rx_freq            Rx Frequency
* @param[in] power_control      Power Control enable or disable
* @param[in] poll_period        Polling period
* @param[in] packet_type        Packet type
* @param[in] payload_length     Length of test sequence
*
* @return Error status
******************************************************************************************
*/
extern uint8_t dbg_tester_start(uint8_t link_id, uint8_t test_scenario, uint8_t hopping_mode,
                 uint8_t tx_freq, uint8_t rx_freq, uint8_t power_control, uint8_t poll_period,
                 uint8_t packet_type, uint16_t payload_length);

/**
******************************************************************************************
* @brief Function called to handle reception of LMP during tester emulation.
*
* @param[in] link_id  Link Id
* @param[in] pdu      Pointer to PDU buffer.
* @param[in] buflen   Length of PDU
*
******************************************************************************************
*/
extern void dbg_tester_forward_lmp(uint8_t link_id, uint8_t * pdu, uint8_t buflen);

#endif // BT_TESTER

/// @} DBGTESTER

#endif // DBG_TESTER_H_
