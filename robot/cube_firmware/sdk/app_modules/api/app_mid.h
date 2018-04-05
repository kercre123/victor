/**
 ****************************************************************************************
 *
 * @file app_mid.h
 *
 * @brief A collection of macros that generate and send messages to the stack.
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

#ifndef _APP_MID_H_
#define _APP_MID_H_

/**
 ****************************************************************************************
 * @addtogroup APP
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"             // SW configuration
#include "app_task.h"                // Application task Definition
#include "app.h"                     // Application Definition
#include "gapm_task.h"               // GAP Manager Task API
#include "gapc_task.h"               // GAP Controller Task API
#include "arch_api.h"

/*
 * DEFINITIONS
 ****************************************************************************************
 */

enum address_type
{
    ///Public BD address
    ADDRESS_PUBLIC                   = 0x00,
    ///Random BD Address
    ADDRESS_RAND,
};

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Create a disconnect message.
 * @param[in] connection_idx The connection id.
 * @return The pointer to the created message.
 ****************************************************************************************
 */
__INLINE struct gapc_disconnect_cmd* app_disconnect_msg_create(uint8_t connection_idx)
{
    struct gapc_disconnect_cmd *cmd = KE_MSG_ALLOC(GAPC_DISCONNECT_CMD,
                                                   KE_BUILD_ID(TASK_GAPC, connection_idx),
                                                   TASK_APP,
                                                   gapc_disconnect_cmd);
    cmd->operation=GAPC_DISCONNECT;
    return cmd;
}

/**
 ****************************************************************************************
 * @brief Send a disconnect message.
 * @param[in] Pointer to the disconnect message to send.
 * @return void
 ****************************************************************************************
 */
__INLINE void app_disconnect_msg_send(struct gapc_disconnect_cmd *cmd)
{
    ke_msg_send(cmd);
}

/**
 ****************************************************************************************
 * @brief Create a connect confirmation message.
 * @param[in] connection_idx The connection id.
 * @return The pointer to the created message.
 ****************************************************************************************
 */
__INLINE struct gapc_connection_cfm* app_connect_cfm_msg_create(uint8_t connection_idx)
{
    // confirm connection
    struct gapc_connection_cfm *cfm = KE_MSG_ALLOC(GAPC_CONNECTION_CFM,
            KE_BUILD_ID(TASK_GAPC, connection_idx), TASK_APP,
            gapc_connection_cfm);
    return cfm;
}

/**
 ****************************************************************************************
 * @brief Send a connect confirmation message.
 * @param[in] Pointer to the connect confirmation message to send.
 * @return void
 ****************************************************************************************
 */
__INLINE void app_connect_cfm_msg_send(struct gapc_connection_cfm*cmd)
{
     ke_msg_send(cmd);
}

/**
 ****************************************************************************************
 * @brief Create a start advertise message.
 * @return The pointer to the created message.
 ****************************************************************************************
 */
__INLINE struct gapm_start_advertise_cmd* app_advertise_start_msg_create(void)
{
    struct gapm_start_advertise_cmd* cmd = KE_MSG_ALLOC(GAPM_START_ADVERTISE_CMD,
                                TASK_GAPM, TASK_APP,
                                gapm_start_advertise_cmd);
    return cmd;
}

/**
 ****************************************************************************************
 * @brief Send an advertise_start_msg message.
 * @param[in] Pointer to the advertise start message to send.
 * @return void
 ****************************************************************************************
 */
__INLINE void app_advertise_start_msg_send(struct gapm_start_advertise_cmd* cmd)
{
    ke_msg_send(cmd);
}

/**
 ****************************************************************************************
 * @brief Create a cancel message.
 * @return The pointer to the created message.
 ****************************************************************************************
 */
__INLINE struct gapm_cancel_cmd* app_gapm_cancel_msg_create(void)
{
    struct gapm_cancel_cmd* cmd = KE_MSG_ALLOC(GAPM_CANCEL_CMD,
                                TASK_GAPM, TASK_APP,
                                gapm_cancel_cmd);
    cmd->operation = GAPM_CANCEL;
    return cmd;
}

/**
 ****************************************************************************************
 * @brief Send a cancellation message.
 * @param[in] Pointer to the cancellation message to send.
 * @return void
 ****************************************************************************************
 */
__INLINE void app_gapm_cancel_msg_send(struct gapm_cancel_cmd* cmd)
{
    ke_msg_send(cmd);
}

/**
 ****************************************************************************************
 * @brief Create an advertise stop message.
 * @return The pointer to the created message.
 ****************************************************************************************
 */
__INLINE struct gapm_cancel_cmd* app_advertise_stop_msg_create(void)
{
    return(app_gapm_cancel_msg_create());
}

/**
 ****************************************************************************************
 * @brief Send an advertise stop  message.
 * @param[in] Pointer to the advertise stop message to send.
 * @return void
 ****************************************************************************************
 */
__INLINE void app_advertise_stop_msg_send(struct gapm_cancel_cmd* cmd)
{
    app_gapm_cancel_msg_send(cmd);
}

/**
 ****************************************************************************************
 * @brief Create a parameter update message.
 * @param[in] connection_idx The connection id where the message should be sent.
 * @return The pointer to the created message.
 ****************************************************************************************
 */
__INLINE  struct gapc_param_update_cmd* app_param_update_msg_create(uint8_t connection_idx)
{
  struct gapc_param_update_cmd* cmd = KE_MSG_ALLOC(GAPC_PARAM_UPDATE_CMD,
                                KE_BUILD_ID(TASK_GAPC,connection_idx), TASK_APP,
                                gapc_param_update_cmd);
  cmd->operation=GAPC_UPDATE_PARAMS;
  return cmd;
}


/**
 ****************************************************************************************
 * @brief Send a parameter update message.
 * @param[in] Pointer to the parameter update message to send.
 * @return void
 ****************************************************************************************
 */
__INLINE  void app_param_update_msg_send(struct gapc_param_update_cmd* cmd)
{
  ke_msg_send(cmd);
}


/**
 ****************************************************************************************
 * @brief Create a connection start message.
 * @return The pointer to the created message.
 ****************************************************************************************
 */
__INLINE  struct gapm_start_connection_cmd* app_connect_start_msg_create(void)
{
 struct gapm_start_connection_cmd* cmd=KE_MSG_ALLOC_DYN(GAPM_START_CONNECTION_CMD , TASK_GAPM, TASK_APP,
                                                gapm_start_connection_cmd, CFG_MAX_CONNECTIONS * sizeof(struct gap_bdaddr));
 return cmd;
}

/**
 ****************************************************************************************
 * @brief Send a connection start  message.
 * @param[in] Pointer to the connection start message to send.
 * @return void
 ****************************************************************************************
 */
__INLINE  void app_connect_start_msg_send(struct gapm_start_connection_cmd* cmd)
{
  ke_msg_send(cmd);
}


/**
 ****************************************************************************************
 * @brief Create a gap manager configuration message.
 * @return The pointer to the created message.
 ****************************************************************************************
 */
__INLINE  struct gapm_set_dev_config_cmd* app_gapm_configure_msg_create(void)
{
 struct gapm_set_dev_config_cmd* cmd=KE_MSG_ALLOC(GAPM_SET_DEV_CONFIG_CMD,
                                TASK_GAPM, TASK_APP,
                                gapm_set_dev_config_cmd);
  cmd->operation=GAPM_SET_DEV_CONFIG;
 return cmd;
}


/**
 ****************************************************************************************
 * @brief Send a gap manager configuration message.
 * @param[in] Pointer to the gap manager configuration message to send.
 * @return void
 ****************************************************************************************
 */
__INLINE  void app_gapm_configure_msg_send(struct gapm_set_dev_config_cmd* cmd)
{
  ke_msg_send(cmd);
}


/**
 ****************************************************************************************
 * @brief Create a gap bond confirmation message.
 * @param[in] connection_idx The id of the connection.
 * @return The pointer to the created message.
 ****************************************************************************************
 */
__INLINE struct gapc_bond_cfm* app_gapc_bond_cfm_msg_create (uint8_t connection_idx)
{
  struct gapc_bond_cfm* cmd=KE_MSG_ALLOC(GAPC_BOND_CFM, KE_BUILD_ID(TASK_GAPC, connection_idx), TASK_APP, gapc_bond_cfm);
  return cmd;
}

/**
 ****************************************************************************************
 * @brief Send a gap bond confirmation message.
 * @param[in] Pointer to the gap bond confirmation message to send.
 * @return void
 ****************************************************************************************
 */
__INLINE void app_gapc_bond_cfm_msg_send (struct gapc_bond_cfm* cmd)
{
  ke_msg_send(cmd);
}


/**
 ****************************************************************************************
 * @brief Create a GAPC_BOND_CFM pairing response message.
 * @param[in] connection_idx Connection index
 * @return The pointer to the created message
 ****************************************************************************************
 */
__INLINE struct gapc_bond_cfm* app_gapc_bond_cfm_pairing_rsp_msg_create(uint8_t connection_idx)
{
    struct gapc_bond_cfm* cmd = app_gapc_bond_cfm_msg_create(connection_idx);
    cmd->request = GAPC_PAIRING_RSP;
    cmd->accept = 0x01;
    return cmd;
}

/**
 ****************************************************************************************
 * @brief Send a gap bond pairing message.
 * @param[in] Pointer to the gap bond pairing message to send.
 * @return void
 ****************************************************************************************
 */
__INLINE void app_gapc_bond_cfm_pairing_rsp_msg_send(struct gapc_bond_cfm* cmd)
{
    ke_msg_send(cmd);
}

/**
 ****************************************************************************************
 * @brief Create a gap bond temporary key exchange message.
 * @param[in] connection_idx The id of the connection.
 * @return The pointer to the created message.
 ****************************************************************************************
 */
__INLINE struct gapc_bond_cfm* app_gapc_bond_cfm_tk_exch_msg_create(uint8_t connection_idx)
{
    struct gapc_bond_cfm* cmd = app_gapc_bond_cfm_msg_create(connection_idx);
    cmd->request = GAPC_TK_EXCH;
    cmd->accept = 0x01;
    return cmd;
}
/**
 ****************************************************************************************
 * @brief Send a gap bond temporary key exchange message.
 * @param[in] Pointer to the gap bond temporary key exchange message to send.
 * @return void
 ****************************************************************************************
 */
__INLINE void app_gapc_bond_cfm_tk_exch_msg_send(struct gapc_bond_cfm* cmd)
{
    ke_msg_send(cmd);
}

/**
 ****************************************************************************************
 * @brief Create CSRK exchange message.
 * @param[in] connection_idx The id of the connection.
 * @return The pointer to the created message.
 ****************************************************************************************
 */
__INLINE struct gapc_bond_cfm* app_gapc_bond_cfm_csrk_exch_msg_create(uint8_t connection_idx)
{
    struct gapc_bond_cfm* cmd = app_gapc_bond_cfm_msg_create(connection_idx);
    cmd->request = GAPC_CSRK_EXCH;
    cmd->accept = 0x01;
    return cmd;
}

/**
 ****************************************************************************************
 * @brief Send a CSRK exchange message.
 * @param[in] Pointer to the gap bond CSRK exchange message to send.
 * @return void
 ****************************************************************************************
 */
 __INLINE void app_gapc_bond_cfm_csrk_exch_msg_send(struct gapc_bond_cfm* cmd)
{
    ke_msg_send(cmd);
}

/**
 ****************************************************************************************
 * @brief Create a gap bond Long Term Key (LTK) exchange message.
 * @param[in] connection_idx The id of the connection.
 * @return The pointer to the created message.
 ****************************************************************************************
 */
__INLINE struct gapc_bond_cfm* app_gapc_bond_cfm_ltk_exch_msg_create(uint8_t connection_idx)
{
    struct gapc_bond_cfm* cmd = app_gapc_bond_cfm_msg_create(connection_idx);
    cmd->request = GAPC_LTK_EXCH;
    cmd->accept = 0x01;
    return cmd;
}

/**
 ****************************************************************************************
 * @brief Send a gap bond ltk exchange message.
 * @param[in] Pointer to the gap bond ltk exchange message to send.
 * @return void
 ****************************************************************************************
 */
__INLINE void app_gapc_bond_cfm_ltk_exch_msg_send (struct gapc_bond_cfm* cmd)
{
    ke_msg_send(cmd);
}

/**
 ****************************************************************************************
 * @brief Create a gap encrypt confirmation message.
 * @param[in] connection_idx The id of the connection.
 * @return The pointer to the created message.
 ****************************************************************************************
 */
__INLINE struct gapc_encrypt_cfm* app_gapc_encrypt_cfm_msg_create(uint8_t connection_idx)
{
    struct gapc_encrypt_cfm* cmd = KE_MSG_ALLOC(GAPC_ENCRYPT_CFM, KE_BUILD_ID(TASK_GAPC, connection_idx), TASK_APP, gapc_encrypt_cfm);
    return cmd;
}

/**
 ****************************************************************************************
 * @brief Send a gap encrypt confirmation message.
 * @param[in] Pointer to the gap encrypt confirmation message to send.
 * @return void
 ****************************************************************************************
 */
__INLINE void app_gapc_encrypt_cfm_msg_send(struct gapc_bond_cfm* cmd)
{
    ke_msg_send(cmd);
}

/**
 ****************************************************************************************
 * @brief Create a gap security request message.
 * @param[in] connection_idx The id of the connection.
 * @param[in] auth The authentication requirements.
 * @return The pointer to the created message.
 ****************************************************************************************
 */
__INLINE struct gapc_security_cmd* app_gapc_security_request_msg_create (uint8_t connection_idx, enum gap_auth auth)
{
  struct gapc_security_cmd * cmd = KE_MSG_ALLOC(GAPC_SECURITY_CMD,
            KE_BUILD_ID(TASK_GAPC, connection_idx), TASK_APP, gapc_security_cmd);
  cmd->operation = GAPC_SECURITY_REQ;
  cmd->auth=auth;

  return cmd;
}

/**
 ****************************************************************************************
 * @brief Send a gap security request message.
 * @param[in] Pointer to the gap security request message to send.
 * @return void
 ****************************************************************************************
 */
__INLINE  void app_gapc_security_request_msg_send (struct gapc_security_cmd* cmd)
{
 ke_msg_send(cmd);
}

/**
 ****************************************************************************************
 * @brief Create a gap manager reset message.
 * @return The pointer to the created message.
 ****************************************************************************************
 */
__INLINE struct gapm_reset_cmd* app_gapm_reset_msg_create(void)
{
        struct gapm_reset_cmd* cmd = KE_MSG_ALLOC(GAPM_RESET_CMD, TASK_GAPM, TASK_APP,
                gapm_reset_cmd);

        cmd->operation = GAPM_RESET;

        return cmd;
}

/**
 ****************************************************************************************
 * @brief Send a gap manager reset message.
 * @param[in] Pointer to the gap reset message to send.
 * @return void
 ****************************************************************************************
 */
__INLINE void app_gapm_reset_msg_send (struct gapm_reset_cmd* cmd)
{
 ke_msg_send(cmd);
}

//--------------------------------OPERATIONS

/**
 ****************************************************************************************
 * @brief Send a gap manager reset operation.
 * @return void
 ****************************************************************************************
 */
__INLINE void app_gapm_reset_op (void)
{
    struct gapm_reset_cmd* cmd = app_gapm_reset_msg_create();
    app_gapm_reset_msg_send(cmd);
}

/**
 ****************************************************************************************
 * @brief Disconnect from a given connection.
 * @param[in] connection_id The id of the given connection.
 * @param[in] reason The reason for the disconnection. Could be one of the following:
 *  CO_ERROR_AUTH_FAILURE, CO_ERROR_REMOTE_USER_TERM_CON, CO_ERROR_REMOTE_DEV_TERM_LOW_RESOURCES,
 *  CO_ERROR_REMOTE_DEV_POWER_OFF, CO_ERROR_UNSUPPORTED_REMOTE_FEATURE,
 *  CO_ERROR_PAIRING_WITH_UNIT_KEY_NOT_SUP, CO_ERROR_UNACCEPTABLE_CONN_INT
 * @return void
 ****************************************************************************************
 */
__INLINE void app_disconnect_op (uint8_t connection_id, uint8_t reason)
{
    struct gapc_disconnect_cmd *cmd = app_disconnect_msg_create(connection_id);
    cmd->reason = reason;
    ke_msg_send(cmd);
}

/**
 ****************************************************************************************
 * @brief Confirm connection operation.
 * @param[in] connection_id The id of the given connection.
 * @param[in] auth The authentication requirements.
 * @param[in] authorize The authorization requirements.
 * @return void
 ****************************************************************************************
 */
__INLINE void app_connect_confirm_op(uint8_t connection_id, enum gap_auth auth, enum gap_authz authorize)
{
    // confirm connection
    struct gapc_connection_cfm *cfm = app_connect_cfm_msg_create(connection_id);
    cfm->auth = auth;
    cfm->authorize = authorize;

    // Send the message
    app_connect_cfm_msg_send(cfm);
}

/**
 ****************************************************************************************
 * @brief Undirected advertise operation.
 * @param[in] address_src_type The source address type used during the advertize operation.
 * @param[in] interval The advertise interval.
 * @param[in] channel_map The channels used during the advertise operation.
 * @param[in] advertise_mode The advertising mode: GAP_NON_DISCOVERABLE, GAP_GEN_DISCOVERABLE,
    GAP_LIM_DISCOVERABLE, GAP_BROADCASTER_MODE
 * @param[in] adv_filt_policy The advertising filter policy:  ADV_ALLOW_SCAN_ANY_CON_ANY,
    ADV_ALLOW_SCAN_WLST_CON_ANY, ADV_ALLOW_SCAN_ANY_CON_WLST, ADV_ALLOW_SCAN_WLST_CON_WLST
 * @param[in] advertise_data Pointer to an array with the advertise data.
 * @param[in] advertise_data_len The length of the advertise data.
 * @param[in] scan_response_data Pointer to an array with the scan response data.
 * @param[in] scan_response_data_len The length of the scan response data.
 * @return void
 ****************************************************************************************
 */
__INLINE void app_advertise_undirected_start_op (enum gapm_own_addr_src address_src_type, 
                                                 uint16_t interval, 
                                                 uint8_t channel_map,
                                                 enum gap_adv_mode advertise_mode,
                                                 enum adv_filter_policy adv_filt_policy,
                                                 uint8_t* advertise_data, 
                                                 uint8_t advertise_data_len,
                                                 uint8_t* scan_response_data, 
                                                 uint8_t scan_response_data_len)
{
    struct gapm_start_advertise_cmd* cmd;
    cmd = app_advertise_start_msg_create ();

    cmd->op.code=GAPM_ADV_UNDIRECT;
    cmd->op.addr_src=address_src_type;
    cmd->intv_max=interval;
    cmd->intv_min=interval;
    cmd->channel_map=channel_map;
    cmd->info.host.mode=advertise_mode;
    cmd->info.host.adv_filt_policy=adv_filt_policy;
    cmd->info.host.adv_data_len=advertise_data_len;
    ASSERT_ERROR(advertise_data_len<APP_ADV_DATA_MAX_SIZE);
    memcpy(cmd->info.host.adv_data,advertise_data,advertise_data_len);
    cmd->info.host.scan_rsp_data_len=scan_response_data_len;
    ASSERT_ERROR(scan_response_data_len<SCAN_RSP_DATA_LEN);
    memcpy(cmd->info.host.adv_data,advertise_data,advertise_data_len);
    app_advertise_start_msg_send(cmd);
}

/**
 ****************************************************************************************
 * @brief Directed advertise operation.
 * @param[in] address_src_type The source address type used during the advertize operation.
 * @param[in] interval The advertise interval.
 * @param[in] channel_map The channels used during the advertise operation.
 * @param[in] target_address_type Address type of the target device
 * @param[in] target_address Address of the target device
 * @return void
 ****************************************************************************************
 */
__INLINE void app_advertise_directed_start_op (enum gapm_own_addr_src address_src_type, uint16_t interval, uint8_t channel_map, \
                                               enum address_type target_address_type, uint8_t* target_address)
{
    struct gapm_start_advertise_cmd* cmd;
    cmd = app_advertise_start_msg_create ();
    cmd->op.code=GAPM_ADV_DIRECT;
    cmd->op.addr_src=address_src_type;
    cmd->intv_max=interval;
    cmd->intv_min=interval;
    cmd->channel_map=channel_map;
    cmd->info.direct.addr_type=target_address_type;
    memcpy(cmd->info.direct.addr.addr, target_address,BD_ADDR_LEN);
    app_advertise_start_msg_send(cmd);
}

/**
 ****************************************************************************************
 * @brief Stop the active advertise operation.
 * @return void
 ****************************************************************************************
 */
__INLINE void app_advertise_stop_op(void)
{
    // Disable Advertising
    struct gapm_cancel_cmd *cmd = app_advertise_stop_msg_create();
    app_advertise_stop_msg_send(cmd);
}

/**
 ****************************************************************************************
 * @brief Update parameters operation.
 * @param[in] connection_idx The id of the connection.
 * @param[in] latency The slave latency measured in connection event periods.
 * @param[in] intv_min The new preferred minimum connection interval measured in 1.25 ms slots.
 * @param[in] intv_max The new preferred maximum connection interval measured in 1.25 ms slots.
 * @param[in] connection_event_len_min The new preferred minimum connection event length
 * measured in 1.25 ms slots.
 * @param[in] connection_event_len_max The new preferred maximum connection event length
 * measured in 1.25 ms slots.
 * @return void
 ****************************************************************************************
 */
__INLINE void app_param_update_op(uint8_t connection_idx, uint16_t intv_min, uint16_t intv_max, uint16_t latency,\
                                  uint16_t supervision_time_out, uint16_t connection_event_len_min,\
                                  uint16_t connection_event_len_max)
{
     struct gapc_param_update_cmd *cmd = app_param_update_msg_create(connection_idx);
            #ifndef __DA14581__
                cmd->params.intv_max=intv_max;
                cmd->params.intv_min=intv_min;
                cmd->params.latency=latency;
                cmd->params.time_out=supervision_time_out;
            #else
                cmd->intv_max=intv_max;
                cmd->intv_min=intv_min;
                cmd->latency=latency;
                cmd->time_out=supervision_time_out;
                cmd->ce_len_min=connection_event_len_min;
                cmd->ce_len_max=connection_event_len_max;
            #endif
    app_param_update_msg_send(cmd);
}


/**
 ****************************************************************************************
 * @brief Update parameters operation.
 * @param[in] connection_idx The id of the connection.
 * @param[in] latency The slave latency measured in connection event periods.
 * @param[in] intv_min_us The new preferred minimum connection interval measured in us.
 * @param[in] intv_max_us The new preferred maximum connection interval measured in us.
 * @param[in] connection_event_len_min_us The new preferred minimum connection event length
 * measured in us.
 * @param[in] connection_event_len_max_us The new preferred maximum connection event length
 * measured in us.
 * @return void
 ****************************************************************************************
 */

__INLINE void app_param_update_op_us(uint8_t connection_idx, uint32_t intv_min_us, uint32_t intv_max_us, uint16_t latency,\
                                  uint32_t supervision_time_out_us, uint32_t connection_event_len_min_us,\
                                  uint32_t connection_event_len_max_us)
{
    app_param_update_op(connection_idx, (uint16_t) US_TO_DOUBLESLOTS(intv_min_us),(uint16_t) US_TO_DOUBLESLOTS(intv_max_us),\
                        latency,(uint16_t) US_TO_TIMERUNITS(supervision_time_out_us),\
                        (uint16_t) US_TO_DOUBLESLOTS(connection_event_len_min_us),(uint16_t) US_TO_DOUBLESLOTS(connection_event_len_max_us));

}


/**
 ****************************************************************************************
 * @brief GAP Manager configure operation.
 * @param[in] role The role of the device: GAP_NO_ROLE, GAP_OBSERVER_SCA, GAP_BROADCASTER_ADV,
 * GAP_CENTRAL_MST, GAP_PERIPHERAL_SLV
 * @param[in] irk Pointer to an array that contains the device Identity Root Key (IRK) .
 * @param[in] appearance The device appearance fill in according to
 * https://developer.bluetooth.org/gatt/characteristics/Pages/CharacteristicViewer.aspx?u=org.bluetooth.characteristic.gap.appearance.xml
 * @param[in] appearance_write_perm Appearance write permission requirement.
 * @param[in] name_write_perm Name write permission requirement.
 * @param[in] max_mtu Maximum mtu supported.
 * @param[in] connection_intv_min Preferred minimum connection interval measured in 1.25 ms slots.
 * @param[in] connection_intv_max Preferred maximum connection interval measured in 1.25 ms slots.
 * @param[in] connection_latency Preferred slave latency measured in connection events.
 * @param[in] supervision_timeout Preferred supervision timeout measured in 10 ms slots.
 * @param[in] flags Privacy settings.
 * Privacy settings bit field (0b1 = enabled, 0b0 = disabled)
 * - [bit 0]: Privacy Support
 * - [bit 1]: Multiple Bond Support (Peripheral only); If enabled, privacy flag is read only.
 * - [bit 2]: Reconnection address visible.* 
 * @return void
 ****************************************************************************************
 */
__INLINE void app_gapm_configure_op (enum gap_role role, uint8_t* irk, uint16_t appearance,\
                                     enum gapm_write_att_perm appearance_write_perm,\
                                     enum gapm_write_att_perm name_write_perm,\
                                     uint16_t max_mtu, uint16_t connection_intv_min, uint16_t connection_intv_max,\
                                     uint16_t connection_latency, uint16_t supervision_timeout, uint8_t flags)
{
    struct gapm_set_dev_config_cmd* cmd = app_gapm_configure_msg_create();

    cmd->role=role;
    memcpy(cmd->irk.key,irk,KEY_LEN);
  	/// Device Appearance (0x0000 - Unknown appearance)
	//Fill in according to https://developer.bluetooth.org/gatt/characteristics/Pages/CharacteristicViewer.aspx?u=org.bluetooth.characteristic.gap.appearance.xml
    cmd->appearance=appearance;
    cmd->appearance_write_perm=appearance_write_perm;
    cmd->name_write_perm=name_write_perm;
    cmd->max_mtu=max_mtu;
    cmd->con_intv_min=connection_intv_min;
    cmd->con_intv_max=connection_intv_max;
    cmd->con_latency=connection_latency;
    cmd->superv_to=supervision_timeout;
    /// Privacy settings bit field (0b1 = enabled, 0b0 = disabled)
    ///  - [bit 0]: Privacy Support
    ///  - [bit 1]: Multiple Bond Support (Peripheral only); If enabled, privacy flag is
    ///             read only.
    ///  - [bit 2]: Reconnection address visible.
    cmd->flags=flags;

    app_gapm_configure_msg_send(cmd);
}

/**
 ****************************************************************************************
 * @brief GAP Manager configure operation.
 * @param[in] role The role of the device: GAP_NO_ROLE, GAP_OBSERVER_SCA, GAP_BROADCASTER_ADV,
 * GAP_CENTRAL_MST, GAP_PERIPHERAL_SLV
 * @param[in] irk Pointer to an array that contains the device Identity Root Key (IRK) .
 * @param[in] appearance The device appearance fill in according to
 * https://developer.bluetooth.org/gatt/characteristics/Pages/CharacteristicViewer.aspx?u=org.bluetooth.characteristic.gap.appearance.xmlThe new preferred minimum connection interval measured in us.
 * @param[in] appearance_write_perm Appearance write permission requirement.
 * @param[in] name_write_perm Name write permission requirement.
 * @param[in] max_mtu Maximum mtu supported.
 * @param[in] connection_intv_min Preferred minimum connection interval measured in us.
 * @param[in] connection_intv_max Preferred maximum connection interval measured in us.
 * @param[in] connection_latency Preferred slave latency measured in connection events.
 * @param[in] supervision_timeout Preferred supervision timeout measured in us.
 * @param[in] flags Privace settings.
 * Privacy settings bit field (0b1 = enabled, 0b0 = disabled)
 * - [bit 0]: Privacy Support
 * - [bit 1]: Multiple Bond Support (Peripheral only); If enabled, privacy flag is read only.
 * - [bit 2]: Reconnection address visible.* 
 * @return void
 ****************************************************************************************
 */
__INLINE void app_gapm_configure_op_us (enum gap_role role, uint8_t* irk, uint16_t appearance,\
                                     enum gapm_write_att_perm appearance_write_perm,\
                                     enum gapm_write_att_perm name_write_perm,\
                                     uint16_t max_mtu, uint32_t connection_intv_min_us, uint32_t connection_intv_max_us,\
                                     uint16_t connection_latency, uint32_t supervision_timeout_us, uint8_t flags)
{
   app_gapm_configure_op(role, irk, appearance,appearance_write_perm,name_write_perm,max_mtu,\
                         (uint16_t) US_TO_DOUBLESLOTS(connection_intv_min_us), (uint16_t) US_TO_DOUBLESLOTS(connection_intv_max_us),\
                         connection_latency, (uint16_t) US_TO_TIMERUNITS(supervision_timeout_us),\
                         flags);
}

/**
 ****************************************************************************************
 * @brief Start security operation.
 * @param[in] connection_idx The id of the connection.
 * @param[in] auth The authentication requirements.
 * @return None.
 ****************************************************************************************
 */
__INLINE  void app_security_request_op(uint8_t connection_idx, enum gap_auth auth)
{
    // Send security request command
    struct gapc_security_cmd * cmd = app_gapc_security_request_msg_create(connection_idx,auth);
    app_gapc_security_request_msg_send(cmd);

}

/**
 ****************************************************************************************
 * @brief Gap Bonding Pairing Response operation.
 * @param[in] connection_idx The id of the connection.
 * @param[in] io_capabilities Device capabilities: GAP_IO_CAP_DISPLAY_ONLY, GAP_IO_CAP_DISPLAY_YES_NO,
 *  GAP_IO_CAP_KB_ONLY, GAP_IO_CAP_NO_INPUT_NO_OUTPUT, GAP_IO_CAP_KB_DISPLAY.
 * @param[in] oob Out of band info: GAP_OOB_AUTH_DATA_NOT_PRESENT, GAP_OOB_AUTH_DATA_PRESENT
 * @param[in] authentication The authentication requirements.
 * @param[in] key_size The key size.
 * @param[in] initiator_key_dist Initiator key distriburion flags: GAP_KDIST_NONE, GAP_KDIST_ENCKEY,
 *  GAP_KDIST_IDKEY, GAP_KDIST_SIGNKEY
 * @param[in] responder_key_dist Responder key distriburion flags: GAP_KDIST_NONE, GAP_KDIST_ENCKEY,
 *  GAP_KDIST_IDKEY, GAP_KDIST_SIGNKEY
 * @param[in] security_requirements Security definition.
 * @return void
 ****************************************************************************************
 */
__INLINE void app_gapc_bond_cfm_pairing_rsp_op (uint8_t connection_idx, enum gap_io_cap io_capabilities, enum gap_oob oob, enum gap_auth authentication, uint8_t key_size,\
                                           enum gap_kdist initiator_key_dist, enum gap_kdist responder_key_dist, enum gap_sec_req security_requirements)
{
    struct gapc_bond_cfm* cfm = app_gapc_bond_cfm_msg_create ( connection_idx );
    cfm->data.pairing_feat.oob=oob;
    cfm->data.pairing_feat.key_size=key_size;
    cfm->data.pairing_feat.iocap=io_capabilities;
    cfm->data.pairing_feat.auth=authentication;
    cfm->data.pairing_feat.sec_req=security_requirements;
    cfm->data.pairing_feat.ikey_dist=initiator_key_dist;
    cfm->data.pairing_feat.rkey_dist=responder_key_dist;
    app_gapc_bond_cfm_msg_send (cfm);
}

/**
 ****************************************************************************************
 * @brief Gap Bonding Temporary Key exchange operation.
 * @param[in] connection_idx The id of the connection.
 * @param[in] temporary_key Array containing the temporary key.
 * @return void
 ****************************************************************************************
 */
__INLINE void app_gapc_bond_cfm_tk_exch_op (uint8_t connection_idx, uint8_t* temporary_key)
{
    struct gapc_bond_cfm* cmd=app_gapc_bond_cfm_tk_exch_msg_create(connection_idx);
    memcpy((void*)cmd->data.tk.key,temporary_key,KEY_LEN);
    app_gapc_bond_cfm_tk_exch_msg_send(cmd);
}

/**
 ****************************************************************************************
 * @brief Gap Bonding Connection Signature Resolving Key exchange operation.
 * @param[in] connection_idx The id of the connection.
 * @param[in] csrk Array containing the csrk key.
 * @return void
 ****************************************************************************************
 */
__INLINE void app_gapc_bond_cfm_csrk_exch_op(uint8_t connection_idx, uint8_t* csrk)
{
   struct gapc_bond_cfm* cmd=app_gapc_bond_cfm_csrk_exch_msg_create(connection_idx);
   memcpy((void*)cmd->data.csrk.key,csrk,KEY_LEN);
   app_gapc_bond_cfm_csrk_exch_msg_send(cmd);
}

/**
 ****************************************************************************************
 * @brief Gap Bonding Long Term Key exchange operation.
 * @param[in] connection_idx The id of the connection.
 * @param[in] long_term_key Array containing the long term key.
 * @param[in] encryption_key_size Encryption key size.
 * @param[in] random_number Random number.
 * @param[in] encryption_diversifier Encryption diversifier.
 * @return void
 ****************************************************************************************
 */
__INLINE void app_gapc_bond_cfm_ltk_exch_op(uint8_t connection_idx, uint8_t* long_term_key, uint8_t encryption_key_size, \
                                            uint8_t* random_number, uint16_t encryption_diversifier)
{
    struct gapc_bond_cfm* cmd=app_gapc_bond_cfm_ltk_exch_msg_create(connection_idx);

    cmd->data.ltk.key_size = encryption_key_size;
    cmd->data.ltk.ediv = encryption_diversifier;

    memcpy(&(cmd->data.ltk.randnb), &(random_number), RAND_NB_LEN);
    memcpy(&(cmd->data.ltk.ltk), &(long_term_key), KEY_LEN);

    app_gapc_bond_cfm_ltk_exch_msg_send(cmd);
}

/**
 ****************************************************************************************
 * @brief Gap Bonding Encrypt confirmation operation.
 * @param[in] connection_idx The id of the connection.
 * @param[in] found Confirm that the entry has been found.
 * @param[in] key_size Size of the key.
 * @param[in] long_term_key The long term key.
 * @return void
 ****************************************************************************************
 */
__INLINE  void app_gapc_encrypt_cfm_op (uint8_t connection_idx, bool found, uint8_t key_size, uint8_t* long_term_key)
{
    struct gapc_encrypt_cfm* cmd=app_gapc_encrypt_cfm_msg_create(connection_idx);
    cmd->found = found;
    if (found == true)
    {
        cmd->key_size = key_size;
        memcpy(&(cmd->ltk), long_term_key, KEY_LEN);
    }
}

/// @} APP

#endif // _APP_MID_H_
