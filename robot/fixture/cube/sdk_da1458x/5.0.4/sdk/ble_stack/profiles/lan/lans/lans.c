/**
 ****************************************************************************************
 *
 * @file lans.c
 *
 * @brief Location and Navigation Profile Sensor implementation.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 * $ Rev $
 *
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @addtogroup LANS
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "rwble_config.h" 

#if (BLE_LN_SENSOR)
#include "lan_common.h"
#include "gap.h"
#include "gattc_task.h"
#include "atts_util.h"
#include "attm_util.h"
#include "lans.h"
#include "lans_task.h"
#include "prf_utils.h"

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// Location and Navigation Profile Server Role Environment
struct lans_env_tag lans_env __attribute__((section("retention_mem_area0"),zero_init)); //@RETENTION MEMORY

/// Location and Navigation Profile Server Role Task
static const struct ke_task_desc TASK_DESC_LANS = {lans_state_handler, &lans_default_handler, lans_state,
                                                    LANS_STATE_MAX, LANS_IDX_MAX};

/*
 * EXPORTED FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */

void lans_init(void)
{
    // Reset environment
    memset(&lans_env, 0, sizeof(lans_env));
    lans_env.ntf_pending = false;
    lans_env.navi_enabled = false;

    // Create LANS task
    ke_task_create(TASK_LANS, &TASK_DESC_LANS);

    // Go to IDLE state
    ke_state_set(TASK_LANS, LANS_DISABLED);
}

void lans_send_rsp_ind(uint16_t handle, uint8_t req_op_code, uint8_t status)
{
    // Error response
    uint8_t rsp[LANP_LAN_LN_CNTL_PT_RSP_MAX_LEN];

    // Response Code
    rsp[0] = LANP_LN_CTNL_PT_RESPONSE_CODE;
    // Request Operation Code
    rsp[1] = req_op_code;
    // Response value
    rsp[2] = status;

    // Set the value in the database - If we are here the handle is valid
    attmdb_att_set_value(handle, 3, (uint8_t *)&rsp);

    // Send an indication
    prf_server_send_event((prf_env_struct *)&lans_env, true, handle);

    // Wait for Confirm
    ke_state_set(TASK_LANS, LANS_WAIT_FOR_CFM);
}

void lans_send_cmp_evt(uint8_t src_id, uint8_t dest_id, uint16_t conhdl,
                        uint8_t operation, uint8_t status)
{
    // Go back to the Connected state if the state is busy
    if ((ke_state_get(src_id) == LANS_BUSY)
        || (ke_state_get(src_id) == LANS_WAIT_FOR_CFM))
    {
        ke_state_set(src_id, LANS_CONNECTED);
    }

    // Set the operation code
    lans_env.operation = LANS_RESERVED_OP_CODE;

    // Send the message
    struct lans_cmp_evt *evt = KE_MSG_ALLOC(LANS_CMP_EVT,
                                             dest_id, src_id,
                                             lans_cmp_evt);

    evt->conhdl     = conhdl;
    evt->operation  = operation;
    evt->status     = status;

    ke_msg_send(evt);
}

void lans_disable(uint16_t conhdl)
{
    // Disable LNS in database
    attmdb_svc_set_permission(lans_env.shdl, PERM_RIGHT_DISABLE);

    //Send current configuration to APP
    struct lans_disable_ind* ind = KE_MSG_ALLOC(LANS_DISABLE_IND,
                                                 lans_env.con_info.appid, lans_env.con_info.prf_id,
                                                 lans_disable_ind);

    ind->conhdl = conhdl;

    // Location and speed Char. - Client Characteristic Configuration Descriptor
    ind->prfl_ntf_ind_cfg = LANS_IS_NTF_IND_ENABLED(LANS_PRF_CFG_FLAG_LOC_SPEED_NTF);

    if (LANS_IS_FEATURE_SUPPORTED(lans_env.prfl_cfg, LANS_LN_CTNL_PT_MASK))
    {
        // CP Vector. - Client Characteristic Configuration Descriptor
        ind->prfl_ntf_ind_cfg |= LANS_IS_NTF_IND_ENABLED(LANS_PRF_CFG_FLAG_LN_CTNL_PT_IND);
    }

    if (LANS_IS_FEATURE_SUPPORTED(lans_env.prfl_cfg, LANS_NAVI_MASK))
    {
        // Control Point Char. - Client Characteristic Configuration Descriptor
        ind->prfl_ntf_ind_cfg |= LANS_IS_NTF_IND_ENABLED(LANS_PRF_CFG_FLAG_NAVIGATION_NTF);
    }

    ke_msg_send(ind);

    // Reset the notification/indication status
    lans_env.prfl_cfg &= LANP_FEAT_ALL_SUPP;

    // Go to idle state
    ke_state_set(TASK_LANS, LANS_IDLE);
}

#endif //(BLE_LN_SENSOR)

/// @} LANS
