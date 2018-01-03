/**
 ****************************************************************************************
 *
 * @file udsc.c
 *
 * @brief User Data Service Client implementation.
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
 * @addtogroup UDSC
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "rwip_config.h"

#if (BLE_UDS_CLIENT)
#include "udsc.h"
#include "udsc_task.h"
#include "gap.h"
#include "prf_utils.h"

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// Device Information Service Client Pool of Environments
struct udsc_env_tag **udsc_envs __attribute__((section("retention_mem_area0"), zero_init)); //@RETENTION MEMORY

/// User Data task descriptor
static const struct ke_task_desc TASK_DESC_UDSC = {udsc_state_handler, &udsc_default_handler, udsc_state, UDSC_STATE_MAX, UDSC_IDX_MAX};

/*
 * EXPORTED FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */

void udsc_init(void)
{
    // Reset all the profile role tasks
    PRF_CLIENT_RESET(udsc_envs, UDSC);
}

void udsc_enable_cfm_send(struct udsc_env_tag *udsc_env, struct prf_con_info *con_info, uint8_t status)
{
    // Send to APP the details of the discovered attributes on BLPS
    struct udsc_enable_cfm * rsp = KE_MSG_ALLOC(UDSC_ENABLE_CFM,
                                                con_info->appid, con_info->prf_id,
                                                udsc_enable_cfm);

    rsp->conhdl = gapc_get_conhdl(con_info->conidx);
    rsp->status = status;

    if (status == PRF_ERR_OK)
    {
        rsp->uds = udsc_env->uds;

        // Register for indications
        prf_register_atthdl2gatt(&udsc_env->con_info, &udsc_env->uds.svc);

        // Go to connected state
        ke_state_set(udsc_env->con_info.prf_id, UDSC_CONNECTED);
    }
    else
    {
        PRF_CLIENT_ENABLE_ERROR(udsc_envs, con_info->prf_id, UDSC);
    }

    ke_msg_send(rsp);
}

void udsc_cmp_evt_ind_send(struct udsc_env_tag *udsc_env, uint8_t status)
{
    struct udsc_cmp_evt_ind *ind = KE_MSG_ALLOC(UDSC_CMP_EVT_IND,
                                              udsc_env->con_info.appid,
                                              udsc_env->con_info.prf_id,
                                              udsc_cmp_evt_ind);

    ind->conhdl = gapc_get_conhdl(udsc_env->con_info.conidx);
    // It will be an PRF status code
    ind->status = status;

    // Send the message
    ke_msg_send(ind);
}

void udsc_read_char_val_rsp_send(struct udsc_env_tag *udsc_env, uint8_t status,
                                            uint8_t char_code, uint16_t length,
                                            const uint8_t *value)
{
    struct udsc_read_char_val_rsp *rsp = ke_msg_alloc(UDSC_READ_CHAR_VAL_RSP,
                                                    udsc_env->con_info.appid,
                                                    udsc_env->con_info.prf_id,
                                                    sizeof(struct udsc_read_char_val_rsp) + length);

    rsp->conhdl = gapc_get_conhdl(udsc_env->con_info.conidx);
    rsp->char_code = char_code;
    rsp->status = status;

    rsp->val_len = length;

    // Fill in the data if success
    if (status == PRF_ERR_OK)
    {
        memcpy(rsp->val, value, length);
    }

    ke_msg_send(rsp);
}

void udsc_set_char_val_cfm_send(struct udsc_env_tag *udsc_env, uint8_t status)
{
    struct udsc_set_char_val_cfm *cfm;

    cfm = KE_MSG_ALLOC(UDSC_SET_CHAR_VAL_CFM,
                                        udsc_env->con_info.appid,
                                        udsc_env->con_info.prf_id,
                                        udsc_set_char_val_cfm);

    cfm->conhdl = gapc_get_conhdl(udsc_env->con_info.conidx);
    cfm->status = status;

    ke_msg_send(cfm);
}

#endif /* (BLE_UDS_CLIENT) */

/// @} UDSC
