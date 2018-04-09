/**
 ****************************************************************************************
 *
 * @file app_task.h
 *
 * @brief Header file for application handlers for ble events and responses.
 *
 * Copyright (C) 2012. Dialog Semiconductor Ltd, unpublished work. This computer 
 * program includes Confidential, Proprietary Information and is a Trade Secret of 
 * Dialog Semiconductor Ltd.  All use, disclosure, and/or reproduction is prohibited 
 * unless authorized in writing. All Rights Reserved.
 *
 * <bluetooth.support@diasemi.com> and contributors.
 *
 ****************************************************************************************
 */

#ifndef APP_TASK_H_
#define APP_TASK_H_


/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "ke_task.h"         // kernel task
#include "ke_msg.h"          // kernel message
#include <stdint.h>          // standard integer

/*
 * DEFINES
 ****************************************************************************************
 */

/// application states
enum
{
    /// Idle state
    APP_IDLE,
    /// Scanning state
    APP_CONNECTABLE,
    /// Connected state
    APP_CONNECTED,
    /// Number of defined states.
    APP_STATE_MAX,
    /// Scanning state
    APP_SCAN,
};

/*
 * GLOBAL VARIABLE DECLARATIONS
 ****************************************************************************************
 */
extern struct app_env_tag app_env;

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */
/**
 ****************************************************************************************
 * @brief Extracts device name from adv data if present and copy it to app_env.
 *
 * @param[in] adv_data      Pointer to advertise data.
 * @param[in] adv_data_len  Advertise data length.
 * @param[in] dev_indx      Devices index in device list.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
void app_find_device_name(unsigned char * adv_data, unsigned char adv_data_len, unsigned char dev_indx);
/**
 ****************************************************************************************
 * @brief Handles GAPM_CMP_EVT event for GAPM_SET_DEV_CONFIG_CMD.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
int gapm_set_dev_config_completion_handler(ke_msg_id_t msgid,
                                           struct gapm_cmp_evt *param,
                                           ke_task_id_t dest_id,
                                           ke_task_id_t src_id);
                                           
/**
 ****************************************************************************************
 * @brief Handles ready indication from the GAP.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */

int gapm_device_ready_ind_handler(ke_msg_id_t msgid,
                                  struct gap_ready_evt *param,
                                  ke_task_id_t dest_id,
                                  ke_task_id_t src_id);
/**
 ****************************************************************************************
 * @brief Handles GAPM_ADV_REPORT_IND event.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */

int gapm_adv_report_ind_handler(ke_msg_id_t msgid,
                                struct gapm_adv_report_ind *param,
                                ke_task_id_t dest_id,
                                ke_task_id_t src_id);
/**
 ****************************************************************************************
 * @brief Handles Connection request indication event.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
int gapc_connection_req_ind_handler(ke_msg_id_t msgid,
                                    struct gapc_connection_req_ind *param,
                                    ke_task_id_t dest_id,
                                    ke_task_id_t src_id);

/**
 ****************************************************************************************
 * @brief Handles Discconnection indication event.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
int gapc_disconnect_ind_handler(ke_msg_id_t msgid,
                                struct gapc_disconnect_ind *param,
                              ke_task_id_t dest_id,
                              ke_task_id_t src_id);
/**
 ****************************************************************************************
 * @brief Handles RSSI indication event.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
int gapc_con_rssi_ind_handler(ke_msg_id_t msgid,
                              struct gapc_con_rssi_ind *param,
                              ke_task_id_t dest_id,
                              ke_task_id_t src_id);
/**
 ****************************************************************************************
 * @brief Handle Bond indication.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
int gapc_bond_ind_handler(ke_msg_id_t msgid,
                          struct gapc_bond_ind *param,
                          ke_task_id_t dest_id,
                          ke_task_id_t src_id);
/**
 ****************************************************************************************
 * @brief Handle reset GAP request completion event.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
int gapm_reset_completion_handler(ke_msg_id_t msgid,
                                           struct gapm_cmp_evt *param,
                                           ke_task_id_t  dest_id,
                                           ke_task_id_t src_id);


/**
 ****************************************************************************************
 * @brief Handles the GAPC_BOND_REQ_IND event.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
int gapc_bond_req_ind_handler(ke_msg_id_t msgid,
                              struct gapc_bond_req_ind *param,
                                  ke_task_id_t dest_id,
                                  ke_task_id_t src_id);

/**
 ****************************************************************************************
 * @brief Handles the GAPC_ENCRYPT_REQ_IND event.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
int gapc_encrypt_req_ind_handler(ke_msg_id_t const msgid,
                                 struct gapc_encrypt_req_ind * param,
                                 ke_task_id_t const dest_id,
                                 ke_task_id_t const src_id);

/**
 ****************************************************************************************
 * @brief Handles the GAPC_ENCRYPT_IND event.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
int gapc_encrypt_ind_handler(ke_msg_id_t const msgid,
                             struct gapc_encrypt_ind *param,
                             ke_task_id_t const dest_id,
                             ke_task_id_t const src_id);
/**
 ****************************************************************************************
 * @brief Handles the PROXR_CREATE_DB_CFM message.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
int proxr_create_db_cfm_handler(ke_task_id_t const dest_id);
/**
 ****************************************************************************************
 * @brief Handles the DISS_CREATE_DB_CFM message.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
int diss_create_db_cfm_handler(ke_msg_id_t const msgid,  
                               struct diss_create_db_cfm *param, 
                               ke_task_id_t const dest_id, 
                               ke_task_id_t const src_id);


/**
 ****************************************************************************************
 * @brief Handles the PROXR_LEVEL_UPD_IND indication.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   Id of the receiving task instance (TASK_GAP).
 * @param[in] src_id    Id of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
void proxr_level_upd_ind_handler(ke_msg_id_t msgid,
                                 struct proxr_level_upd_ind  *param,
                                 ke_task_id_t dest_id,
                                 ke_task_id_t src_id);

/**
 ****************************************************************************************
 * @brief Handles the PROXR_LLS_ALERT_IND indication.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   Id of the receiving task instance (TASK_GAP).
 * @param[in] src_id    Id of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
void proxr_lls_alert_ind_handler(ke_msg_id_t msgid,
                                 struct proxr_lls_alert_ind *param,
                                 ke_task_id_t dest_id,
                                 ke_task_id_t src_id);

/// @} APPTASK

#endif // APP_TASK_H_
