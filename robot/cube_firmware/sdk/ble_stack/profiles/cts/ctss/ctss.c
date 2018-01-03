/**
 ****************************************************************************************
 *
 * @file ctss.c
 *
 * @brief Current Time Service Server Implementation.
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

/**
 ****************************************************************************************
 * @addtogroup CTSS
 * @{
 ****************************************************************************************
 */

#include "rwip_config.h"

#if (BLE_CTS_SERVER)

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "cts_common.h"
#include "ctss_task.h"
#include "ctss.h"

#include "prf_utils.h"

/*
 * CTS ATTRIBUTES DEFINITION
 ****************************************************************************************
 */

/// Current Time Service
const att_svc_desc_t ctss_svc                           = ATT_SVC_CURRENT_TIME;

/// Current Time Characteristic
const struct att_char_desc ctss_curr_time_char          = ATT_CHAR(ATT_CHAR_PROP_RD|ATT_CHAR_PROP_NTF,
                                                                CTSS_CURRENT_TIME_CHAR,
                                                                ATT_CHAR_CT_TIME);
/// Local Time Info Characteristic
const struct att_char_desc ctss_loc_time_info_char      = ATT_CHAR(ATT_CHAR_PROP_RD,
                                                                CTSS_LOCAL_TIME_INFO_CHAR,
                                                                ATT_CHAR_LOCAL_TIME_INFO);
/// Reference Time Info Characteristic
const struct att_char_desc ctss_ref_time_info_char      = ATT_CHAR(ATT_CHAR_PROP_RD,
                                                                CTSS_REF_TIME_INFO_CHAR,
                                                                ATT_CHAR_REFERENCE_TIME_INFO);

/// Default value for the CCC descriptor (0x0000)
static const uint16_t default_ccc_val;

/// Full CTS Database Description - Used to add attributes into the database
const struct attm_desc att_db[CTSS_IDX_NB] =
{
    // Current Time Service Declaration
    [CTSS_IDX_SVC]                              = {ATT_DECL_PRIMARY_SERVICE, PERM(RD, ENABLE), sizeof(ctss_svc),
                                                    sizeof(ctss_svc), (uint8_t *) &ctss_svc},

    // Current Time Characteristic Declaration
    [CTSS_IDX_CURRENT_TIME_CHAR]                = {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), sizeof(ctss_curr_time_char),
                                                    sizeof(ctss_curr_time_char), (uint8_t *) &ctss_curr_time_char},
    // Current Time Characteristic Value
    [CTSS_IDX_CURRENT_TIME_VAL]                 = {ATT_CHAR_CT_TIME, PERM(RD, ENABLE) | PERM(NTF, ENABLE), CTSS_CURRENT_TIME_VAL_LEN,
                                                    0, NULL},
    // Current Time Characteristic - Client Char. Configuration Descriptor
    [CTSS_IDX_CURRENT_TIME_CFG]                 = {ATT_DESC_CLIENT_CHAR_CFG, PERM(RD, ENABLE) | PERM(WR, ENABLE), sizeof(uint16_t),
                                                    sizeof(default_ccc_val), (uint8_t *) &default_ccc_val},

    // Local Time Information Characteristic Declaration
    [CTSS_IDX_LOCAL_TIME_INFO_CHAR]             = {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), sizeof(ctss_loc_time_info_char),
                                                    sizeof(ctss_loc_time_info_char), (uint8_t *) &ctss_loc_time_info_char},
    // Local Time Information Characteristic Value
    [CTSS_IDX_LOCAL_TIME_INFO_VAL]              = {ATT_CHAR_LOCAL_TIME_INFO, PERM(RD, ENABLE), sizeof(struct cts_loc_time_info),
                                                    0, NULL},

    // Reference Time Information Characteristic Declaration
    [CTSS_IDX_REF_TIME_INFO_CHAR]               = {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), sizeof(ctss_ref_time_info_char),
                                                    sizeof(ctss_ref_time_info_char), (uint8_t *) &ctss_ref_time_info_char},
    // Reference Time Info Characteristic Value
    [CTSS_IDX_REF_TIME_INFO_VAL]                = {ATT_CHAR_REFERENCE_TIME_INFO, PERM(RD, ENABLE), sizeof(struct cts_ref_time_info),
                                                    0, NULL},
};

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// Current Time Service Server environment variable
struct ctss_env_tag ctss_env __attribute__((section("retention_mem_area0"),zero_init)); //@RETENTION MEMORY

/// BCSS task descriptor
static const struct ke_task_desc TASK_DESC_CTSS = {ctss_state_handler, &ctss_default_handler, ctss_state, CTSS_STATE_MAX, CTSS_IDX_MAX};

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */
void ctss_init(void)
{
    // Reset Environment
    memset(&ctss_env, 0, sizeof(ctss_env));

    // Create CTSS task
    ke_task_create(TASK_CTSS, &TASK_DESC_CTSS);

    // Go to disabled state
    ke_state_set(TASK_CTSS, CTSS_DISABLED);
}

void ctss_enable_cfm_send(struct prf_con_info *con_info, uint8_t status)
{
    // Send to APP the details of the discovered attributes on CTSS
    struct ctss_enable_cfm * rsp = KE_MSG_ALLOC(CTSS_ENABLE_CFM,
                                                con_info->appid, con_info->prf_id,
                                                ctss_enable_cfm);

    rsp->conhdl = gapc_get_conhdl(con_info->conidx);
    rsp->status = status;

    ke_msg_send(rsp);
}

void ctss_error_ind_send(struct prf_con_info *con_info, uint8_t status, ke_msg_id_t msg_id)
{
    struct prf_server_error_ind *ind = KE_MSG_ALLOC(CTSS_ERROR_IND,
                                                    con_info->appid, con_info->prf_id,
                                                    prf_server_error_ind);

    ind->conhdl    = gapc_get_conhdl(con_info->conidx);
    ind->status    = status;
    ind->msg_id    = msg_id;

    // Send the message
    ke_msg_send(ind);
}

void ctss_pack_curr_time_value(uint8_t *p_pckd_time, const struct cts_curr_time* p_curr_time_val)
{
    // Date-Time
    prf_pack_date_time(p_pckd_time, &(p_curr_time_val->exact_time_256.day_date_time.date_time));

    // Day of Week
    *(p_pckd_time + 7) = p_curr_time_val->exact_time_256.day_date_time.day_of_week;

    // Fraction 256
    *(p_pckd_time + 8) = p_curr_time_val->exact_time_256.fraction_256;

    // Adjust Reason
    *(p_pckd_time + 9) = p_curr_time_val->adjust_reason;
}

void ctss_disable(uint16_t conhdl)
{
    // Disable CTS
    attmdb_svc_set_permission(ctss_env.shdl, PERM(SVC, DISABLE));

    // Send APP cfg every time, C may have changed it
    struct ctss_disable_ind *ind = KE_MSG_ALLOC(CTSS_DISABLE_IND,
                                                ctss_env.con_info.appid,
                                                ctss_env.con_info.prf_id,
                                                ctss_disable_ind);

    ind->conhdl = conhdl;

    if ((ctss_env.features & CTSS_CURRENT_TIME_CFG) == CTSS_CURRENT_TIME_CFG)
    {
        ind->ntf_en = PRF_CLI_START_NTF;

        // Reset notifications bit field
        ctss_env.features &= ~CTSS_CURRENT_TIME_CFG;
    }

    ke_msg_send(ind);

    // Go to idle state
    ke_state_set(ctss_env.con_info.prf_id, CTSS_IDLE);
}

#endif // (BLE_CTS_SERVER)

/// @} CTSS
