/**
 ****************************************************************************************
 *
 * @file wpts.c
 *
 * @brief Wireless Power Transfer service. 
 *
 * Copyright (C) 2013 Dialog Semiconductor GmbH and its Affiliates, unpublished work
 * This computer program includes Confidential, Proprietary Information and is a Trade Secret 
 * of Dialog Semiconductor GmbH and its Affiliates. All use, disclosure, and/or 
 * reproduction is prohibited unless authorized in writing. All Rights Reserved.
 *
 ****************************************************************************************
 */


/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwble_config.h"

#if (BLE_WPTS)

#include "wpts.h"
#include "wpts_task.h"
#include "attm_db.h"
#include "gapc.h"

/*
 *  WPTS SERVICE ATTRIBUTES VALUES DEFINTION
 ****************************************************************************************
 */
 
/// WPT Service

const att_svc_desc_t wpt_svc     = ATT_SVC_WPT;
 
/// wpts_1 value attribute UUID

struct att_char128_desc wpt_charging_ptu_ctrl_char = {ATT_CHAR_PROP_RD | ATT_CHAR_PROP_WR_NO_RESP,
                                                                    {0,0},
                                                                    WPTS_PRU_CHARGING_CTRL_UUID_128};

struct att_char128_desc wpt_ptu_static_char = {ATT_CHAR_PROP_RD | ATT_CHAR_PROP_WR_NO_RESP,
                                                                    {0,0},
                                                                    WPTS_PTU_STATIC_UUID_128};

struct att_char128_desc wpt_charging_pru_alert_char = {ATT_CHAR_PROP_RD | ATT_CHAR_PROP_NTF,
                                                                    {0,0},
                                                                    WPTS_PRU_ALERT_UUID_128};

struct att_char128_desc wpt_charging_pru_static_char = {ATT_CHAR_PROP_RD,
                                                                    {0,0},
                                                                    WPTS_PRU_STATIC_UUID_128};

struct att_char128_desc wpt_charging_pru_dynamic_char = {ATT_CHAR_PROP_RD,
                                                                    {0,0},
                                                                    WPTS_PRU_DYNAMIC_UUID_128};

                                                                    
/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

struct wpts_env_tag wpts_env __attribute__((section("retention_mem_area0"),zero_init)); //@RETENTION MEMORY

static const struct ke_task_desc TASK_DESC_WPTS = {wpts_state_handler, &wpts_default_handler, wpts_state, WPTS_STATE_MAX, WPTS_IDX_MAX};

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

void wpts_init(void)
{
    // Reset Environment
    memset(&wpts_env, 0, sizeof(wpts_env));
    
    // Create wpts task
    ke_task_create(TASK_WPTS, &TASK_DESC_WPTS);

    ke_state_set(TASK_WPTS, WPTS_DISABLED);
}

void wpts_disable(void)
{
    // Disable LLS in database
    attmdb_svc_set_permission(wpts_env.wpt_shdl, PERM_RIGHT_DISABLE);

    // If IAS is supported

    struct wpts_disable_ind *ind = KE_MSG_ALLOC(WPTS_DISABLE_IND,
                                                wpts_env.con_info.appid, TASK_WPTS,
                                                wpts_disable_ind);

    // Fill in the parameter structure
    ind->conhdl = gapc_get_conhdl(wpts_env.con_info.conidx);

    // Send the message
    ke_msg_send(ind);

    // Go to idle state
    ke_state_set(TASK_WPTS, WPTS_IDLE);
}

void wpts_alert_send_cfm_send(ke_task_id_t const dest_id, uint8_t status)
{
    // Send CFM to APP that value has been sent or not
    struct wpts_alert_send_cfm *cfm = KE_MSG_ALLOC(WPTS_ALERT_SEND_CFM, dest_id,
                                                   TASK_WPTS, wpts_alert_send_cfm);

    cfm->conhdl = gapc_get_conhdl(wpts_env.con_info.conidx);
    cfm->status = status;

    ke_msg_send(cfm);
}

void wpts_static_val_cfm_send(ke_task_id_t const dest_id, uint8_t status)
{
    // Send CFM to APP that value has been sent or not
    struct wpts_pru_static_val_cfm *cfm = KE_MSG_ALLOC(WPTS_PRU_STATIC_VAL_UPD_DB_CFM, dest_id,
                                                       TASK_WPTS, wpts_pru_static_val_cfm);
    
    cfm->conhdl = gapc_get_conhdl(wpts_env.con_info.conidx);
    cfm->status = status;

    ke_msg_send(cfm);
}

void wpts_dynamic_val_cfm_send(ke_task_id_t const dest_id, uint8_t status)
{
    // Send CFM to APP that value has been sent or not
    struct wpts_pru_dynamic_val_cfm *cfm = KE_MSG_ALLOC(WPTS_PRU_DYNAMIC_VAL_UPD_DB_CFM, dest_id,
                                                        TASK_WPTS, wpts_pru_dynamic_val_cfm);

    cfm->conhdl = gapc_get_conhdl(wpts_env.con_info.conidx);
    cfm->status = status;

    ke_msg_send(cfm);
}

void wpts_error_ind_send(ke_task_id_t const dest_id, uint8_t const error_id, uint8_t reason)
{
    // Send info about error to APP
    struct wpts_error_ind *ind = KE_MSG_ALLOC(WPTS_ERROR_IND, dest_id,
                                              TASK_WPTS, wpts_error_ind);

    ind->error_id = error_id;
    ind->reason = reason;

    ke_msg_send(ind);
}

#endif //BLE_WPTS
