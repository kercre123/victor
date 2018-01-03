/**
 ****************************************************************************************
 *
 * @file pasps.c
 *
 * @brief Phone Alert Status Profile Server implementation.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 * $Rev: $
 *
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @addtogroup PASPS
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "rwble_config.h" 

#if (BLE_PAS_SERVER)
#include "pasp_common.h"
#include "atts_util.h"
#include "pasps.h"
#include "pasps_task.h"
#include "prf_utils.h"

/*
 * PHONE ALERT STATUS SERVICE ATTRIBUTES
 ****************************************************************************************
 */

/// Full PAS Database Description - Used to add attributes into the database
const struct attm_desc pasps_att_db[PASS_IDX_NB] =
{
    // Phone Alert Status Service Declaration
    [PASS_IDX_SVC]                   =   {ATT_DECL_PRIMARY_SERVICE, PERM(RD, ENABLE), sizeof(pasps_pass_svc),
                                          sizeof(pasps_pass_svc), (uint8_t *)&pasps_pass_svc},

    // Alert Status Characteristic Declaration
    [PASS_IDX_ALERT_STATUS_CHAR]     =   {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), sizeof(pasps_alert_status_char),
                                          sizeof(pasps_alert_status_char), (uint8_t *)&pasps_alert_status_char},
    // Alert Status Characteristic Value
    [PASS_IDX_ALERT_STATUS_VAL]      =   {ATT_CHAR_ALERT_STATUS, PERM(RD, ENABLE) | PERM(NTF, ENABLE), sizeof(uint8_t),
                                          0, NULL},
    // Alert Status Characteristic - Client Characteristic Configuration Descriptor
    [PASS_IDX_ALERT_STATUS_CFG]      =   {ATT_DESC_CLIENT_CHAR_CFG, PERM(RD, ENABLE)|PERM(WR, ENABLE), sizeof(uint16_t),
                                          0, NULL},

    // Ringer Setting Characteristic Declaration
    [PASS_IDX_RINGER_SETTING_CHAR]   =   {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), sizeof(pasps_ringer_setting_char),
                                          sizeof(pasps_ringer_setting_char), (uint8_t *)&pasps_ringer_setting_char},
    // Ringer Settings Characteristic Value
    [PASS_IDX_RINGER_SETTING_VAL]    =   {ATT_CHAR_RINGER_SETTING, PERM(RD, ENABLE) | PERM(NTF, ENABLE), sizeof(uint8_t),
                                          0, NULL},
    // Ringer Settings Characteristic - Client Characteristic Configuration Descriptor
    [PASS_IDX_RINGER_SETTING_CFG]    =   {ATT_DESC_CLIENT_CHAR_CFG, PERM(RD, ENABLE)|PERM(WR, ENABLE), sizeof(uint16_t),
                                          0, NULL},

    // Ringer Control Point Characteristic Declaration
    [PASS_IDX_RINGER_CTNL_PT_CHAR]   =   {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), sizeof(pasps_ringer_ctnl_pt_char),
                                          sizeof(pasps_ringer_ctnl_pt_char), (uint8_t *)&pasps_ringer_ctnl_pt_char},
    // Ringer Control Point Characteristic Value
    [PASS_IDX_RINGER_CTNL_PT_VAL]    =   {ATT_CHAR_RINGER_CNTL_POINT, PERM(WR, ENABLE), sizeof(uint8_t),
                                          0, NULL},
};

/// Phone Alert Status Service
const att_svc_desc_t pasps_pass_svc                   = ATT_SVC_PHONE_ALERT_STATUS;

/// Phone Alert Status Service Alert Status Characteristic
const struct att_char_desc pasps_alert_status_char   = ATT_CHAR(ATT_CHAR_PROP_RD | ATT_CHAR_PROP_NTF,
                                                                0,
                                                                ATT_CHAR_ALERT_STATUS);

/// Phone Alert Status Service Ringer Setting Characteristic
const struct att_char_desc pasps_ringer_setting_char = ATT_CHAR(ATT_CHAR_PROP_RD | ATT_CHAR_PROP_NTF,
                                                                0,
                                                                ATT_CHAR_RINGER_SETTING);

/// Phone Alert Status Service Ringer Settings Characteristic
const struct att_char_desc pasps_ringer_ctnl_pt_char = ATT_CHAR(ATT_CHAR_PROP_WR_NO_RESP,
                                                                0,
                                                                ATT_CHAR_RINGER_CNTL_POINT);

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// Phone Alert Status Profile Server Environment Variable
struct pasps_env_tag pasps_env __attribute__((section("retention_mem_area0"),zero_init)); //@RETENTION MEMORY
/// Phone Alert Status Profile Server Environment Variable
struct pasps_idx_env_tag **pasps_idx_envs __attribute__((section("retention_mem_area0"),zero_init)); //@RETENTION MEMORY

/// Phone Alert Status Profile Server Role Task
static const struct ke_task_desc TASK_DESC_PASPS = {pasps_state_handler, &pasps_default_handler, pasps_state,
                                                    PASPS_STATE_MAX, PASPS_IDX_MAX};

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

void pasps_init(void)
{
    PRF_CLIENT_RESET(pasps_idx_envs, PASPS);

    // Reset Common Environment
    memset(&pasps_env, 0, sizeof(struct pasps_env_tag));

    // Create PASPS task
    ke_task_create(TASK_PASPS, &TASK_DESC_PASPS);

    // First instance goes to IDLE state
    ke_state_set(TASK_PASPS, PASPS_DISABLED);
}

void pasps_send_cmp_evt(ke_task_id_t src_id, ke_task_id_t dest_id, uint16_t conhdl,
                        uint8_t operation, uint8_t status)
{
    // Come back to the Connected state if the state was busy.
    if (ke_state_get(src_id) == PASPS_BUSY)
    {
        ke_state_set(src_id, PASPS_CONNECTED);
    }

    // Send the message to the application
    struct pasps_cmp_evt *evt = KE_MSG_ALLOC(PASPS_CMP_EVT,
                                             dest_id, src_id,
                                             pasps_cmp_evt);

    evt->conhdl      = conhdl;
    evt->operation   = operation;
    evt->status      = status;

    ke_msg_send(evt);
}

void pasps_disable(struct pasps_idx_env_tag *idx_env, uint16_t conhdl)
{
    // Disable PAS service
    attmdb_svc_set_permission(pasps_env.pass_shdl, PERM(SVC, DISABLE));

    struct pasps_disable_ind *ind = KE_MSG_ALLOC(PASPS_DISABLE_IND,
                                                 idx_env->con_info.appid, idx_env->con_info.prf_id,
                                                 pasps_disable_ind);

    memset(ind, 0x00, sizeof(struct pasps_disable_ind));

    ind->conhdl = conhdl;

    if (PASPS_IS_NTF_ENABLED(idx_env, PASPS_FLAG_ALERT_STATUS_CFG))
    {
        ind->alert_status_ntf_cfg = PRF_CLI_START_NTF;
    }

    if (PASPS_IS_NTF_ENABLED(idx_env, PASPS_FLAG_RINGER_SETTING_CFG))
    {
        ind->ringer_setting_ntf_cfg = PRF_CLI_START_NTF;
    }

    ke_msg_send(ind);

    // Go to idle state
    ke_state_set(idx_env->con_info.prf_id, PASPS_IDLE);

    // Free the environment allocated for this connection
    PRF_CLIENT_DISABLE(pasps_idx_envs, KE_IDX_GET(idx_env->con_info.prf_id), PASPS);
}

#endif //(BLE_PASP_SERVER)

/// @} PASPS
