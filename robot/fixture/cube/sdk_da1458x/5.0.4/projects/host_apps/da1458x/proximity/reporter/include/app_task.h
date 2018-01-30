/**
 ****************************************************************************************
 *
 * @file app_task.h
 *
 * @brief Handling of BLE events and responses.
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


/*
 * GLOBAL VARIABLE DECLARATIONS
 ****************************************************************************************
 */
extern struct app_env_tag app_env;

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

int gap_bond_req_ind_handler(ke_msg_id_t msgid,
                             struct gapc_bond_req_ind *param,
                             ke_task_id_t dest_id,
                             ke_task_id_t src_id);

int gap_ready_evt_handler(ke_msg_id_t msgid,
                          void *param,
                          ke_task_id_t dest_id,
                          ke_task_id_t src_id);

int gap_dev_inq_req_cmp_evt_handler(ke_msg_id_t msgid,
                                    void *param,
                                    ke_task_id_t dest_id,
                                    ke_task_id_t src_id);

int gapm_reset_req_evt_handler(ke_msg_id_t msgid,
                               struct gapm_cmp_evt *param,
                               ke_task_id_t dest_id,
                               ke_task_id_t src_id);

int gapm_set_mode_req_evt_handler(ke_msg_id_t msgid,
                                  struct gapm_cmp_evt *param,
                                  ke_task_id_t dest_id,
                                  ke_task_id_t src_id);

int gap_dev_inq_result_handler(ke_msg_id_t msgid,
                               struct gapm_adv_report_ind *param,
                               ke_task_id_t dest_id,
                               ke_task_id_t src_id);

int gap_le_create_conn_req_ind_handler(ke_msg_id_t msgid,
                                       struct gapc_connection_req_ind *param,
                                       ke_task_id_t dest_id,
                                       ke_task_id_t src_id);

int gap_discon_cmp_evt_handler(ke_msg_id_t msgid,
                               struct gapc_disconnect_ind *param,
                               ke_task_id_t dest_id,
                               ke_task_id_t src_id);

int gap_read_rssi_cmp_evt_handler(ke_msg_id_t msgid,
                                  struct gapc_con_rssi_ind *param,
                                  ke_task_id_t dest_id,
                                  ke_task_id_t src_id);

int gap_bond_ind_handler(ke_msg_id_t msgid,
                         struct gapc_bond_ind *param,
                         ke_task_id_t dest_id,
                         ke_task_id_t src_id);

int gapc_encrypt_req_ind_handler(ke_msg_id_t const msgid,
                                 struct gapc_encrypt_req_ind * param,
                                 ke_task_id_t const dest_id,
                                 ke_task_id_t const src_id);

int gapc_encrypt_ind_handler(ke_msg_id_t const msgid,
                             struct gapc_encrypt_ind *param,
                             ke_task_id_t const dest_id,
                             ke_task_id_t const src_id);

int llc_rd_tx_pw_lvl_cmp_evt_handler(ke_msg_id_t const msgid,
                                     struct llc_rd_tx_pw_lvl_cmd_complete const *param,
                                     ke_task_id_t const dest_id,
                                     ke_task_id_t const src_id);

int app_create_db_cfm_handler(ke_task_id_t const dest_id);

void proxr_level_upd_ind_handler(ke_msg_id_t msgid,
                                 struct proxr_level_upd_ind  *param,
                                 ke_task_id_t dest_id,
                                 ke_task_id_t src_id);

void proxr_lls_alert_ind_handler(ke_msg_id_t msgid,
                                 struct proxr_lls_alert_ind *param,
                                 ke_task_id_t dest_id,
                                 ke_task_id_t src_id);


#endif // APP_TASK_H_
