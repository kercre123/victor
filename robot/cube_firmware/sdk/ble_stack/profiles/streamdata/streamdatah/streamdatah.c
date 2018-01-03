/**
 ****************************************************************************************
 *
 * @file streamdatah.c
 *
 * @brief StreamData Host implementation.
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

/**
 ****************************************************************************************
 * @addtogroup STREAMDATAH
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwble_config.h"

#if (BLE_STREAMDATA_HOST)

#include "gattc_task.h"
#include "streamdatah.h"
#include "streamdatah_task.h"
#include "prf_utils.h"

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

struct streamdatah_env_tag **streamdatah_envs __attribute__((section("retention_mem_area0"),zero_init)); //@RETENTION MEMORY

/// PROXR task descriptor
static const struct ke_task_desc TASK_DESC_STREAMDATAH = {streamdatah_state_handler, &streamdatah_default_handler, streamdatah_state, STREAMDATAH_STATE_MAX, STREAMDATAH_IDX_MAX};

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

void streamdatah_init(void)
{
    // Reset all the profile role tasks
    PRF_CLIENT_RESET(streamdatah_envs, STREAMDATAH);
}

void streamdatah_enable_cfm_send(struct streamdatah_env_tag *streamdatah_env, struct prf_con_info *con_info, uint8_t status)
{
    //format response to app
    struct streamdatah_enable_cfm * cfm = KE_MSG_ALLOC(STREAMDATAH_ENABLE_CFM,
                                                 con_info->appid, con_info->prf_id,
                                                 streamdatah_enable_cfm);

    cfm->conhdl = gapc_get_conhdl(con_info->conidx);
    cfm->status = status;

    if (status == PRF_ERR_OK)
    {
        cfm->streamdatah    = streamdatah_env->streamdatah;

		// Register STREAMDATAH task in gatt for indication/notifications
		prf_register_atthdl2gatt(&streamdatah_env->con_info, &streamdatah_env->streamdatah.svc);

        // Go to connected state
        ke_state_set(con_info->prf_id, STREAMDATAH_CONNECTED);
    }
    else
    {
        PRF_CLIENT_ENABLE_ERROR(streamdatah_envs, con_info->prf_id, STREAMDATAH);
    }

    ke_msg_send(cfm);
}

/*
void streamdatah_read_char_rsp_send(struct streamdatah_env_tag *streamdatah_env,
                              struct gattc_read_ind *param, uint8_t status)
{
    // Received value length
    uint8_t length = (param != NULL) ? param->data.len : 0;

    struct streamdatah_rd_char_rsp *rsp = KE_MSG_ALLOC_DYN(STREAMDATAH_RD_CHAR_RSP,
                                                     streamdatah_env->con_info.appid, streamdatah_env->con_info.prf_id,
                                                     streamdatah_rd_char_rsp, length);

    rsp->conhdl     = streamdatah_env->con_info.conhdl;
    rsp->status     = status;
    rsp->char_code  = streamdatah_env->last_char_code;

    if (param != NULL)
    {
        memcpy(&rsp->val, &param->data.data, length);
    }

    ke_msg_send(rsp);
}
*/

void streamdatah_write_char_rsp_send(struct streamdatah_env_tag *streamdatah_env, uint8_t status)
{
    // Send response to app
    struct streamdatah_wr_char_rsp *wr_cfm = KE_MSG_ALLOC(STREAMDATAH_WR_CHAR_RSP,
                                                    streamdatah_env->con_info.appid, streamdatah_env->con_info.prf_id,
                                                    streamdatah_wr_char_rsp);

    wr_cfm->conhdl    = gapc_get_conhdl(streamdatah_env->con_info.conidx);
    wr_cfm->status    = status;

    ke_msg_send(wr_cfm);
}

#endif //BLE_STREAMDATA_HOST

/// @} STREAMDATAH
