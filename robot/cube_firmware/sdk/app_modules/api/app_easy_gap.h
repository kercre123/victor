/**
 ****************************************************************************************
 *
 * @file app_easy_gap.h
 *
 * @brief Easy GAP API.
 *
 * Copyright (C) 2015. Dialog Semiconductor Ltd, unpublished work. This computer
 * program includes Confidential, Proprietary Information and is a Trade Secret of
 * Dialog Semiconductor Ltd.  All use, disclosure, and/or reproduction is prohibited
 * unless authorized in writing. All Rights Reserved.
 *
 * <bluetooth.support@diasemi.com> and contributors.
 *
 ****************************************************************************************
 */

#ifndef _APP_EASY_GAP_H_
#define _APP_EASY_GAP_H_

/**
 ****************************************************************************************
 * @addtogroup APP
 * @ingroup RICOW
 *
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#if (BLE_APP_PRESENT)

#include <stdint.h>          // Standard Integer Definition
#include <co_bt.h>           // Common BT Definitions
#include "arch.h"            // Platform Definitions
#include "gapc.h"

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Connection handle to connection Id transformation
 * @return int8_t Connection Id number
 ****************************************************************************************
 */
__INLINE int8_t conhdl_to_conidx(uint16_t conhdl)
{
    return (gapc_get_conidx(conhdl));
}

/**
 ****************************************************************************************
 * @brief Connection Id to connection handle transformation
 * @return int16_t Connection handle number
 ****************************************************************************************
 */
__INLINE int16_t conidx_to_conhdl(uint8_t conidx)
{
    return (gapc_get_conhdl(conidx));
}

/**
 ****************************************************************************************
 * @brief Connection handle to connection Id transformation
 * @return int8_t Connection Id number
 ****************************************************************************************
 */
int8_t active_conhdl_to_conidx(uint16_t conhdl);

/**
 ****************************************************************************************
 * @brief Connection Id to connection handle transformation
 * @return int16_t Connection handle number
 ****************************************************************************************
 */
int16_t active_conidx_to_conhdl(uint8_t connection_idx);

/**
 ****************************************************************************************
 * @brief Send BLE disconnect command.
 * @param[in] connection_idx Connection Id
 * @return void
 ****************************************************************************************
 */
void app_easy_gap_disconnect(uint8_t connection_idx);

/**
 ****************************************************************************************
 * @brief Send a connection confirmation message.
 * @param[in] connection_idx    Connection Id number
 * @param[in] auth              Authentication requirements
 * @param[in] authorize         Authorization setting
 * @return void
 ****************************************************************************************
 */
void app_easy_gap_confirm(uint8_t connection_idx, enum gap_auth auth, enum gap_authz authorize);

/**
 ****************************************************************************************
 * @brief Start advertising for undirected connection.
 * @return void
 ****************************************************************************************
 */
void app_easy_gap_undirected_advertise_start(void);

/**
 ****************************************************************************************
 * @brief Start advertising for directed connection.
 * @return void
 ****************************************************************************************
 */
void app_easy_gap_directed_advertise_start(void);

/**
 ****************************************************************************************
 * @brief Start advertising for non-connectable peripheral.
 * @return void
 ****************************************************************************************
 */
void app_easy_gap_non_connectable_advertise_start(void);

/**
 ****************************************************************************************
 * @brief Stop advertising.
 * @return void
 ****************************************************************************************
 */
void app_easy_gap_advertise_stop(void);

/**
 ****************************************************************************************
 * @brief Start advertising for undirected connection with timeout start.
 * @param[in] delay            Delay of the timer
 * @param[in] timeout_callback Timeout callback function
 * @return void
 ****************************************************************************************
 */
void app_easy_gap_undirected_advertise_with_timeout_start(uint16_t delay, void (*timeout_callback)(void));

/**
 ****************************************************************************************
 * @brief Stop advertising with timeout stop.
 * @return void
 ****************************************************************************************
 */
void app_easy_gap_advertise_with_timeout_stop(void);

/**
 ****************************************************************************************
 * @brief Get non connectable advertising message with filled parameters.
 * @return gapm_start_advertise_cmd Pointer to advertising message
 ****************************************************************************************
*/
struct gapm_start_advertise_cmd* app_easy_gap_non_connectable_advertise_get_active(void);

/**
 ****************************************************************************************
 * @brief Get undirected connectable advertising message with filled parameters.
 * @return gapm_start_advertise_cmd Pointer to advertising message
 ****************************************************************************************
*/
struct gapm_start_advertise_cmd* app_easy_gap_undirected_advertise_get_active(void);

/**
 ****************************************************************************************
 * @brief Get directed connectable advertising message with filled parameters.
 * @return gapm_start_advertise_cmd Pointer to advertising message
 ****************************************************************************************
*/
struct gapm_start_advertise_cmd* app_easy_gap_directed_advertise_get_active(void);

/**
 ****************************************************************************************
 * @brief Param update start request.
 * @param[in] connection_idx Connection Id number
 * @return void
 ****************************************************************************************
*/
void app_easy_gap_param_update_start(uint8_t connection_idx);

/**
 ****************************************************************************************
 * @brief Get param update request message with filled parameters.
 * @param[in] connection_idx Connection Id number
 * @return gapc_param_update_cmd Pointer to param update request message
 ****************************************************************************************
*/
struct gapc_param_update_cmd* app_easy_gap_param_update_get_active(uint8_t connection_idx);

/**
 ****************************************************************************************
 * @brief Start connection.
 * @return void
 ****************************************************************************************
*/
void app_easy_gap_start_connection_to(void);

/**
 ****************************************************************************************
 * @brief Start direct connection to known peer.
 * @param[in] peer_addr_type Peer to connect address type
 * @param[in] peer_addr      Peer to connect address
 * @param[in] intv           Connection interval
 * @return void
 ****************************************************************************************
*/
void app_easy_gap_start_connection_to_set(uint8_t peer_addr_type, uint8_t *peer_addr, uint16_t intv);

/**
 ****************************************************************************************
 * @brief Get connection message with filled parameters
 * @return gapm_start_connection_cmd Pointer to connection message
 ****************************************************************************************
*/
struct gapm_start_connection_cmd* app_easy_gap_start_connection_to_get_active(void);

/**
 ****************************************************************************************
 * @brief Get device configuration message with filled parameters
 * @return gapm_set_dev_config_cmd Pointer to device configuration message
 ****************************************************************************************
*/
struct gapm_set_dev_config_cmd* app_easy_gap_dev_config_get_active(void);

/**
 ****************************************************************************************
 * @brief Configure device
 * @return void
 ****************************************************************************************
*/
void app_easy_gap_dev_configure(void);

/// @} APP

#endif //(BLE_APP_PRESENT)

#endif // _APP_EASY_GAP_H_
