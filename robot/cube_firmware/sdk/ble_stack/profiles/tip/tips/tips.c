/**
 ****************************************************************************************
 *
 * @file tips.c
 *
 * @brief Time Profile Server implementation.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 *
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @addtogroup TIPS
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "rwip_config.h"

#if (BLE_TIP_SERVER)
#include "attm_util.h"
#include "tips.h"
#include "tips_task.h"
#include "prf_utils.h"

/*
 * CTS, NDCS, RTUS ATTRIBUTES
 ****************************************************************************************
 */
/// Full CTS Database Description - Used to add attributes into the database
const struct attm_desc cts_att_db[CTS_IDX_NB] =
{
    // Current Time Service Declaration
    [CTS_IDX_SVC]                              =   {ATT_DECL_PRIMARY_SERVICE, PERM(RD, ENABLE), sizeof(tips_cts_svc),
                                                   sizeof(tips_cts_svc), (uint8_t *)&tips_cts_svc},

    // Current Time Characteristic Declaration
    [CTS_IDX_CURRENT_TIME_CHAR]                =   {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), sizeof(tips_curr_time_char),
                                                   sizeof(tips_curr_time_char), (uint8_t *)&tips_curr_time_char},
    // Current Time Characteristic Value
    [CTS_IDX_CURRENT_TIME_VAL]                 =   {ATT_CHAR_CT_TIME, PERM(RD, ENABLE)|PERM(NTF, ENABLE), CTS_CURRENT_TIME_VAL_LEN,
                                                 0, NULL},
    // Current Time Characteristic - Client Char. Configuration Descriptor
    [CTS_IDX_CURRENT_TIME_CFG]                 =   {ATT_DESC_CLIENT_CHAR_CFG, PERM(RD, ENABLE)|PERM(WR, ENABLE), sizeof(uint16_t),
                                                 0, NULL},

    // Local Time Information Characteristic Declaration
    [CTS_IDX_LOCAL_TIME_INFO_CHAR]            =   {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), sizeof(tips_loc_time_info_char),
                                                   sizeof(tips_loc_time_info_char), (uint8_t *)&tips_loc_time_info_char},
    // Local Time Information Characteristic Value
    [CTS_IDX_LOCAL_TIME_INFO_VAL]             =   {ATT_CHAR_LOCAL_TIME_INFO, PERM(RD, ENABLE), sizeof(struct tip_loc_time_info),
                                                 0, NULL},

    // Reference Time Information Characteristic Declaration
    [CTS_IDX_REF_TIME_INFO_CHAR]            =   {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), sizeof(tips_ref_time_info_char),
                                                 sizeof(tips_ref_time_info_char), (uint8_t *)&tips_ref_time_info_char},
    // Reference Time Info Characteristic Value
    [CTS_IDX_REF_TIME_INFO_VAL]             =   {ATT_CHAR_REFERENCE_TIME_INFO, PERM(RD, ENABLE), sizeof(struct tip_ref_time_info),
                                                 0, NULL},
};

/// Full NDCS Database Description - Used to add attributes into the database
const struct attm_desc ndcs_att_db[NDCS_IDX_NB] =
{
    // Next DST Change Service Declaration
    [NDCS_IDX_SVC]                          =   {ATT_DECL_PRIMARY_SERVICE, PERM(RD, ENABLE), sizeof(tips_ndcs_svc),
                                                   sizeof(tips_ndcs_svc), (uint8_t *)&tips_ndcs_svc},

    // Time with DST Characteristic Declaration
    [NDCS_IDX_TIME_DST_CHAR]                =   {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), sizeof(tips_time_with_dst_char),
                                                   sizeof(tips_time_with_dst_char), (uint8_t *)&tips_time_with_dst_char},
    // Time With DST Characteristic Value
    [NDCS_IDX_TIME_DST_VAL]                 =   {ATT_CHAR_TIME_WITH_DST, PERM(RD, ENABLE), NDCS_TIME_DST_VAL_LEN,
                                                 0, NULL},
};

/// Full RTUS Database Description - Used to add attributes into the database
const struct attm_desc rtus_att_db[RTUS_IDX_NB] =
{
    // Reference Time Information Service Declaration
    [RTUS_IDX_SVC]                          =   {ATT_DECL_PRIMARY_SERVICE, PERM(RD, ENABLE), sizeof(tips_rtus_svc),
                                                   sizeof(tips_rtus_svc), (uint8_t *)&tips_rtus_svc},

    // Time Update Control Point Characteristic Declaration
    [RTUS_IDX_TIME_UPD_CTNL_PT_CHAR]        =   {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), sizeof(tips_time_upd_contr_pt_char),
                                                   sizeof(tips_time_upd_contr_pt_char), (uint8_t *)&tips_time_upd_contr_pt_char},
    // Time Update Control Point Characteristic Value
    [RTUS_IDX_TIME_UPD_CTNL_PT_VAL]         =   {ATT_CHAR_TIME_UPDATE_CNTL_POINT, PERM(WR, ENABLE), sizeof(tip_time_upd_contr_pt),
                                                 0, NULL},

    // Time Update State Characteristic Declaration
    [RTUS_IDX_TIME_UPD_STATE_CHAR]            =   {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), sizeof(tips_time_upd_state_char),
                                                   sizeof(tips_time_upd_state_char), (uint8_t *)&tips_time_upd_state_char},
    // Time Update State Characteristic Value
    [RTUS_IDX_TIME_UPD_STATE_VAL]             =   {ATT_CHAR_TIME_UPDATE_STATE, PERM(RD, ENABLE), sizeof(struct tip_time_upd_state),
                                                 0, NULL},
};

/*
 * TIPS PROFILE ATTRIBUTES
 ****************************************************************************************
 */

/*
 * Services
 */
/// Current Time Service
const att_svc_desc_t tips_cts_svc      = ATT_SVC_CURRENT_TIME;
/// Next DST Change Service
const att_svc_desc_t tips_ndcs_svc     = ATT_SVC_NEXT_DST_CHANGE;
/// Reference Time Update Service
const att_svc_desc_t tips_rtus_svc     = ATT_SVC_REF_TIME_UPDATE;

/*
 * Current Time Service Characteristics
 */
///Current Time Characteristic
const struct att_char_desc tips_curr_time_char         = ATT_CHAR(ATT_CHAR_PROP_RD|ATT_CHAR_PROP_NTF,
                                                                    CTS_CURRENT_TIME_CHAR,
                                                                    ATT_CHAR_CT_TIME);
///Local Time Info Characteristic
const struct att_char_desc tips_loc_time_info_char     = ATT_CHAR(ATT_CHAR_PROP_RD,
                                                                    CTS_LOCAL_TIME_INFO_CHAR,
                                                                    ATT_CHAR_LOCAL_TIME_INFO);
///Reference Time Info Characteristic
const struct att_char_desc tips_ref_time_info_char      = ATT_CHAR(ATT_CHAR_PROP_RD,
                                                                    CTS_REF_TIME_INFO_CHAR,
                                                                    ATT_CHAR_REFERENCE_TIME_INFO);

/*
 * Next DST Change Service Characteristics
 */
///Time with DST Characteristic
const struct att_char_desc tips_time_with_dst_char     = ATT_CHAR(ATT_CHAR_PROP_RD,
                                                                    NDCS_TIME_DST_CHAR,
                                                                    ATT_CHAR_TIME_WITH_DST);

/*
 * Reference Time Update Service Characteristics
 */
///Time Update Control Point Characteristic
const struct att_char_desc tips_time_upd_contr_pt_char = ATT_CHAR(ATT_CHAR_PROP_WR_NO_RESP,
                                                                    RTUS_TIME_UPD_CTNL_PT_CHAR,
                                                                    ATT_CHAR_TIME_UPDATE_CNTL_POINT);
///Time Update State
const struct att_char_desc tips_time_upd_state_char     = ATT_CHAR(ATT_CHAR_PROP_RD,
                                                                     RTUS_TIME_UPD_STATE_CHAR,
                                                                     ATT_CHAR_TIME_UPDATE_STATE);

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

struct tips_env_tag tips_env __attribute__((section("retention_mem_area0"),zero_init)); //@RETENTION MEMORY
struct tips_idx_env_tag **tips_idx_envs __attribute__((section("retention_mem_area0"),zero_init)); //@RETENTION MEMORY

/// Time Tasks
static const struct ke_task_desc TASK_DESC_TIPS = {tips_state_handler, &tips_default_handler, tips_state, TIPS_STATE_MAX, TIPS_IDX_MAX};


/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */
void tips_init(void)
{
    PRF_CLIENT_RESET(tips_idx_envs, TIPS);

    // Reset Environment
    memset(&tips_env, 0, sizeof(tips_env));

    // Create TIPS task
    ke_task_create(TASK_TIPS, &TASK_DESC_TIPS);

    // Go to IDLE state
    ke_state_set(TASK_TIPS, TIPS_DISABLED);
}

void tips_enable_cfm_send(struct prf_con_info *con_info, uint8_t status)
{
    // Send to APP the details of the discovered attributes on TIPS
    struct tips_enable_cfm * rsp = KE_MSG_ALLOC(TIPS_ENABLE_CFM,
                                                con_info->appid, con_info->prf_id,
                                                tips_enable_cfm);

    rsp->conhdl = gapc_get_conhdl(con_info->conidx);
    rsp->status = status;

    ke_msg_send(rsp);
}

void tips_error_ind_send(struct prf_con_info *con_info, uint8_t status, ke_msg_id_t msg_id)
{
    struct prf_server_error_ind *ind = KE_MSG_ALLOC(TIPS_ERROR_IND,
                                                    con_info->appid, con_info->prf_id,
                                                    prf_server_error_ind);

    ind->conhdl    = gapc_get_conhdl(con_info->conidx);
    ind->status    = status;
    ind->msg_id    = msg_id;

    // send the message
    ke_msg_send(ind);
}

void tips_pack_curr_time_value(uint8_t *p_pckd_time, const struct tip_curr_time* p_curr_time_val)
{
    // Date-Time
    prf_pack_date_time(p_pckd_time, &(p_curr_time_val->exact_time_256.day_date_time.date_time));

    //Day of Week
    *(p_pckd_time + 7) = p_curr_time_val->exact_time_256.day_date_time.day_of_week;

    //Fraction 256
    *(p_pckd_time + 8) = p_curr_time_val->exact_time_256.fraction_256;

    //Adjust Reason
    *(p_pckd_time + 9) = p_curr_time_val->adjust_reason;
}

void tips_pack_time_dst_value(uint8_t *p_pckd_time_dst, const struct tip_time_with_dst* p_time_dst_val)
{
    // Date-Time
    prf_pack_date_time(p_pckd_time_dst, &(p_time_dst_val->date_time));

    // DST Offset
    *(p_pckd_time_dst + 7) = p_time_dst_val->dst_offset;
}

void tips_disable(struct tips_idx_env_tag *tips_idx_env, uint16_t conhdl)
{
    // Disable CTS
    attmdb_svc_set_permission(tips_env.cts_shdl, PERM(SVC, DISABLE));

    if (TIPS_IS_SUPPORTED(TIPS_NDCS_SUP))
    {
        // Disable NDCS
        attmdb_svc_set_permission(tips_env.ndcs_shdl, PERM(SVC, DISABLE));
    }

    if (TIPS_IS_SUPPORTED(TIPS_RTUS_SUP))
    {
        // Disable RTUS
        attmdb_svc_set_permission(tips_env.rtus_shdl, PERM(SVC, DISABLE));
    }

    // Send APP cfg every time, C may have changed it
    struct tips_disable_ind *ind = KE_MSG_ALLOC(TIPS_DISABLE_IND,
                                                tips_idx_env->con_info.appid, tips_idx_env->con_info.prf_id,
                                                tips_disable_ind);

    ind->conhdl = conhdl;

    if ((tips_idx_env->ntf_state & TIPS_CTS_CURRENT_TIME_CFG) == TIPS_CTS_CURRENT_TIME_CFG)
    {
        ind->current_time_ntf_en    = PRF_CLI_START_NTF;

        //Reset notifications bit field
        tips_idx_env->ntf_state &= ~TIPS_CTS_CURRENT_TIME_CFG;
    }

    ke_msg_send(ind);

    //Go to idle state
    ke_state_set(tips_idx_env->con_info.prf_id, TIPS_IDLE);

    PRF_CLIENT_DISABLE(tips_idx_envs, KE_IDX_GET(tips_idx_env->con_info.prf_id), TIPS);
}
#endif //BLE_TIP_SERVER

/// @} TIPS
