/**
 ****************************************************************************************
 *
 * @file anps.c
 *
 * @brief Alert Notification Profile Server implementation.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 * $ Rev: $
 *
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @addtogroup ANPS
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwble_config.h"

#if (BLE_AN_SERVER)

#include "atts_util.h"
#include "anps.h"
#include "anps_task.h"
#include "anp_common.h"
#include "prf_utils.h"
#include "prf_types.h"

/*
 * ANS ATTRIBUTES
 ****************************************************************************************
 */

/// Alert Notification Service
const att_svc_desc_t anps_ans_svc                          = ATT_SVC_ALERT_NTF;

/// Supported New Alert Category Characteristic
const struct att_char_desc anps_supp_new_alert_cat_char    = ATT_CHAR(ATT_CHAR_PROP_RD,
                                                                      0,
                                                                      ATT_CHAR_SUP_NEW_ALERT_CAT);
/// New Alert Characteristic
const struct att_char_desc anps_new_alert_char             = ATT_CHAR(ATT_CHAR_PROP_NTF,
                                                                      0,
                                                                      ATT_CHAR_NEW_ALERT);
/// Supported Unread Alert Category Characteristic
const struct att_char_desc anps_supp_unread_alert_cat_char = ATT_CHAR(ATT_CHAR_PROP_RD,
                                                                      0,
                                                                      ATT_CHAR_SUP_UNREAD_ALERT_CAT);
/// Unread Alert Status Characteristic
const struct att_char_desc anps_unread_alert_status_char   = ATT_CHAR(ATT_CHAR_PROP_NTF,
                                                                      0,
                                                                      ATT_CHAR_UNREAD_ALERT_STATUS);
/// Alert Notification Control Point Characteristic
const struct att_char_desc anps_alert_ntf_ctnl_pt_char     = ATT_CHAR(ATT_CHAR_PROP_WR,
                                                                      0,
                                                                      ATT_CHAR_ALERT_NTF_CTNL_PT);

/*
 * ANS DATABASE
 ****************************************************************************************
 */

/// Full ANS Database Description - Used to add attributes into the database
const struct attm_desc ans_att_db[ANS_IDX_NB] =
{
    /// Alert Notification Service Declaration
    [ANS_IDX_SVC]                           =   {ATT_DECL_PRIMARY_SERVICE, PERM(RD, ENABLE), sizeof(anps_ans_svc),
                                                 sizeof(anps_ans_svc), (uint8_t *)&anps_ans_svc},

    /// Supported New Alert Category Characteristic Declaration
    [ANS_IDX_SUPP_NEW_ALERT_CAT_CHAR]       =   {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), sizeof(anps_supp_new_alert_cat_char),
                                                 sizeof(anps_supp_new_alert_cat_char), (uint8_t *)&anps_supp_new_alert_cat_char},
    /// Supported New Alert Category Characteristic Value
    [ANS_IDX_SUPP_NEW_ALERT_CAT_VAL]        =   {ATT_CHAR_SUP_NEW_ALERT_CAT, PERM(RD, ENABLE), sizeof(struct anp_cat_id_bit_mask),
                                                 0, NULL},

    /// New Alert Characteristic Declaration
    [ANS_IDX_NEW_ALERT_CHAR]                =   {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), sizeof(anps_new_alert_char),
                                                 sizeof(anps_new_alert_char), (uint8_t *)&anps_new_alert_char},
    /// New Alert Characteristic Value
    [ANS_IDX_NEW_ALERT_VAL]                 =   {ATT_CHAR_NEW_ALERT, PERM(NTF, ENABLE), ANS_NEW_ALERT_MAX_LEN,
                                                 0, NULL},
    /// New Alert Characteristic - Client Char. Configuration Descriptor
    [ANS_IDX_NEW_ALERT_CFG]                 =   {ATT_DESC_CLIENT_CHAR_CFG, PERM(RD, ENABLE)|PERM(WR, ENABLE), sizeof(uint16_t),
                                                 0, NULL},

    /// Supported Unread Alert Category Characteristic Declaration
    [ANS_IDX_SUPP_UNREAD_ALERT_CAT_CHAR]    =   {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), sizeof(anps_supp_unread_alert_cat_char),
                                                 sizeof(anps_supp_unread_alert_cat_char), (uint8_t *)&anps_supp_unread_alert_cat_char},
    /// Supported New Alert Category Characteristic Value
    [ANS_IDX_SUPP_UNREAD_ALERT_CAT_VAL]     =   {ATT_CHAR_SUP_UNREAD_ALERT_CAT, PERM(RD, ENABLE), sizeof(struct anp_cat_id_bit_mask),
                                                 0, NULL},

    /// Unread Alert Status Characteristic Declaration
    [ANS_IDX_UNREAD_ALERT_STATUS_CHAR]      =   {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), sizeof(anps_unread_alert_status_char),
                                                 sizeof(anps_unread_alert_status_char), (uint8_t *)&anps_unread_alert_status_char},
    /// Unread Alert Status Characteristic Value
    [ANS_IDX_UNREAD_ALERT_STATUS_VAL]       =   {ATT_CHAR_UNREAD_ALERT_STATUS, PERM(NTF, ENABLE), sizeof(struct anp_unread_alert),
                                                 0, NULL},
    /// Unread Alert Status Characteristic - Client Char. Configuration Descriptor
    [ANS_IDX_UNREAD_ALERT_STATUS_CFG]       =   {ATT_DESC_CLIENT_CHAR_CFG, PERM(RD, ENABLE)|PERM(WR, ENABLE), sizeof(uint16_t),
                                                 0, NULL},

    /// Alert Notification Control Point Characteristic Declaration
    [ANS_IDX_ALERT_NTF_CTNL_PT_CHAR]        =   {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), sizeof(anps_alert_ntf_ctnl_pt_char),
                                                 sizeof(anps_alert_ntf_ctnl_pt_char), (uint8_t *)&anps_alert_ntf_ctnl_pt_char},
    /// Alert Notification Control Point Characteristic Value
    [ANS_IDX_ALERT_NTF_CTNL_PT_VAL]         =   {ATT_CHAR_ALERT_NTF_CTNL_PT, PERM(WR, ENABLE), 2*sizeof(uint8_t), 0, NULL},
};

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// Alert Notification Profile Server Environment Variable
struct anps_env_tag anps_env __attribute__((section("retention_mem_area0"),zero_init)); //@RETENTION MEMORY
/// Alert Notification Profile Server Pool of Environments
struct anps_idx_env_tag **anps_idx_envs __attribute__((section("retention_mem_area0"),zero_init)); //@RETENTION MEMORY

/// Alert Notification Profile Server Role Task
static const struct ke_task_desc TASK_DESC_ANPS = {anps_state_handler, &anps_default_handler, anps_state,
                                                   ANPS_STATE_MAX, ANPS_IDX_MAX};

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

void anps_init(void)
{
    PRF_CLIENT_RESET(anps_idx_envs, ANPS);

    // Reset Common Environment
    memset(&anps_env, 0, sizeof(anps_env));

    // Create ANPS task
    ke_task_create(TASK_ANPS, &TASK_DESC_ANPS);

    // First instance goes to IDLE state
    ke_state_set(TASK_ANPS, ANPS_DISABLED);
}

void anps_send_cmp_evt(ke_task_id_t src_id, ke_task_id_t dest_id, uint16_t conhdl,
                       uint8_t operation, uint8_t status)
{
    // Come back to the Connected state if the state was busy.
    if (ke_state_get(src_id) == ANPS_BUSY)
    {
        ke_state_set(src_id, ANPS_CONNECTED);
    }

    // Send the message to the application
    struct anps_cmp_evt *evt = KE_MSG_ALLOC(ANPS_CMP_EVT,
                                            dest_id, src_id,
                                            anps_cmp_evt);

    evt->conhdl      = conhdl;
    evt->operation   = operation;
    evt->status      = status;

    ke_msg_send(evt);
}

void anps_send_ntf_status_update_ind(struct anps_idx_env_tag *idx_env, uint8_t alert_type)
{
    // Send the message to the application
    struct anps_ntf_status_update_ind *ind = KE_MSG_ALLOC(ANPS_NTF_STATUS_UPDATE_IND,
                                                          idx_env->con_info.appid, idx_env->con_info.prf_id,
                                                          anps_ntf_status_update_ind);

    ind->conhdl      = gapc_get_conhdl(idx_env->con_info.conidx);
    ind->alert_type  = alert_type;
    ind->ntf_ccc_cfg = ANPS_IS_ALERT_ENABLED(idx_env, alert_type) ? PRF_CLI_START_NTF : PRF_CLI_STOP_NTFIND;

    if (alert_type == ANP_NEW_ALERT)
    {
        ind->cat_ntf_cfg.cat_id_mask_0 = (uint8_t)(idx_env->ntf_new_alert_cfg & 0x00FF);
        ind->cat_ntf_cfg.cat_id_mask_1 = (uint8_t)((idx_env->ntf_new_alert_cfg & 0xFF00) >> 8);
    }
    else
    {
        ind->cat_ntf_cfg.cat_id_mask_0 = (uint8_t)(idx_env->ntf_unread_alert_cfg & 0x00FF);
        ind->cat_ntf_cfg.cat_id_mask_1 = (uint8_t)(idx_env->ntf_unread_alert_cfg >> 8);
    }

    ke_msg_send(ind);
}

void anps_send_ntf_immediate_req_ind(struct anps_idx_env_tag *idx_env, uint8_t alert_type,
                                     uint8_t category_id)
{
    uint16_t req_cat;

    // Send the message to the application
    struct anps_ntf_immediate_req_ind *ind = KE_MSG_ALLOC(ANPS_NTF_IMMEDIATE_REQ_IND,
                                                          idx_env->con_info.appid, idx_env->con_info.prf_id,
                                                          anps_ntf_immediate_req_ind);

    ind->conhdl      = gapc_get_conhdl(idx_env->con_info.conidx);
    ind->alert_type  = alert_type;

    if (alert_type == ANP_NEW_ALERT)
    {
        if (category_id == CAT_ID_ALL_SUPPORTED_CAT)
        {
            // All category that are supported and enabled shall be notified
            req_cat = idx_env->ntf_new_alert_cfg;
        }
        else
        {
            req_cat = (1 << category_id);
        }
    }
    else        // Unread alert
    {
        if (category_id == CAT_ID_ALL_SUPPORTED_CAT)
        {
            // All category that are supported and enabled shall be notified
            req_cat = idx_env->ntf_unread_alert_cfg;
        }
        else
        {
            req_cat = (1 << category_id);
        }
    }

    ind->cat_ntf_cfg.cat_id_mask_0 = (uint8_t)(req_cat & 0x00FF);
    ind->cat_ntf_cfg.cat_id_mask_1 = (uint8_t)((req_cat & 0xFF00) >> 8);

    ke_msg_send(ind);
}

void anps_disable(struct anps_idx_env_tag *anps_idx_env, uint16_t conhdl)
{
    // Disable ANS
    attmdb_svc_set_permission(anps_env.ans_shdl, PERM(SVC, DISABLE));

    // Send Configuration
    struct anps_disable_ind *ind = KE_MSG_ALLOC(ANPS_DISABLE_IND,
                                                anps_idx_env->con_info.appid, anps_idx_env->con_info.prf_id,
                                                anps_disable_ind);

    ind->conhdl                      = conhdl;
    ind->new_alert_ntf_cfg           = ANPS_IS_ALERT_ENABLED(anps_idx_env, ANP_NEW_ALERT)
                                       ? PRF_CLI_START_NTF : PRF_CLI_STOP_NTFIND;
    ind->unread_alert_status_ntf_cfg = ANPS_IS_ALERT_ENABLED(anps_idx_env, ANP_UNREAD_ALERT)
                                       ? PRF_CLI_START_NTF : PRF_CLI_STOP_NTFIND;

    ke_msg_send(ind);

    // Go to Idle state
    ke_state_set(anps_idx_env->con_info.prf_id, ANPS_IDLE);

    // Remove the connection environment
    PRF_CLIENT_DISABLE(anps_idx_envs, KE_IDX_GET(anps_idx_env->con_info.prf_id), ANPS);
}

#endif //(BLE_AN_SERVER)

/// @} ANPS
