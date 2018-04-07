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

// application states
enum
{
    // Idle state
    APP_IDLE,
    // Scanning state
    APP_SCAN,
    // Connected state
    APP_CONNECTED,

    APP_EN_SERV_STATUS_NOTIFICATIONS,

    APP_WR_MEM_DEV,

    APP_WR_GPIO_MAP,

    APP_RD_MEM_INFO,

    APP_WR_PATCH_LEN,

    APP_WR_PATCH_DATA,

    APP_WR_END_OF_SUOTA,

    APP_RD_MEM_INFO_2,

    APP_DISCONNECTING,

    // Number of defined states.
    APP_STATE_MAX
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

int gap_ready_evt_handler(ke_msg_id_t msgid,
                          void  *param,
                          ke_task_id_t dest_id,
                          ke_task_id_t src_id);

int gap_dev_inq_result_handler(ke_msg_id_t msgid,
                               struct gapm_adv_report_ind *param,
                               ke_task_id_t dest_id,
                               ke_task_id_t src_id);

int gapc_connection_req_ind_handler(ke_msg_id_t msgid,
                                    struct gapc_connection_req_ind *param,
                                    ke_task_id_t dest_id,
                                    ke_task_id_t src_id);

int gapc_disconnect_ind_handler(ke_msg_id_t msgid,
                                struct gapc_disconnect_ind *param,
                                ke_task_id_t dest_id,
                                ke_task_id_t src_id);

int gap_bond_ind_handler(ke_msg_id_t msgid,
                         struct gapc_bond_ind *param,
                         ke_task_id_t dest_id,
                         ke_task_id_t src_id);

int gap_bond_req_ind_handler(ke_msg_id_t msgid,
                             struct gapc_bond_req_ind *param,
                             ke_task_id_t dest_id,
                             ke_task_id_t src_id);

int gap_cmp_evt_handler(ke_msg_id_t msgid,
                        struct gapm_cmp_evt  *param,
                        ke_task_id_t  dest_id,
                        ke_task_id_t src_id);

int gapc_cmp_evt_handler(ke_msg_id_t msgid,
                        struct gapc_cmp_evt *param,
                        ke_task_id_t dest_id,
                        ke_task_id_t src_id);

int gattc_disc_svc_ind_handler(ke_msg_id_t msgid,
                               struct gattc_disc_svc_ind *param,
                               ke_task_id_t dest_id,
                               ke_task_id_t src_id);

int gattc_disc_char_ind_handler(ke_msg_id_t msgid,
                                struct gattc_disc_char_ind *param,
                                ke_task_id_t dest_id,
                                ke_task_id_t src_id);

int gattc_disc_char_desc_ind_handler(ke_msg_id_t msgid,
                                     struct gattc_disc_char_desc_ind *param,
                                     ke_task_id_t dest_id,
                                     ke_task_id_t src_id);

int gattc_cmp_evt_handler(ke_msg_id_t msgid,
                          struct gattc_cmp_evt *param,
                          ke_task_id_t dest_id,
                          ke_task_id_t src_id);

int gattc_read_ind_handler(ke_msg_id_t msgid,
                           struct gattc_read_ind *param,
                           ke_task_id_t dest_id,
                           ke_task_id_t src_id);

int gattc_event_ind_handler(ke_msg_id_t msgid,
                            struct gattc_event_ind *param,
                            ke_task_id_t dest_id,
                            ke_task_id_t src_id);


#endif // APP_TASK_H_
