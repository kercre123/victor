/**
 ****************************************************************************************
 *
 * @file cpps.c
 *
 * @brief Cycling Power Profile Sensor implementation.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 * $ Rev $
 *
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @addtogroup CPPS
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "rwble_config.h" 

#if (BLE_CP_SENSOR)
#include "cpp_common.h"
#include "gap.h"
#include "gattc_task.h"
#include "atts_util.h"
#include "attm_util.h"
#include "cpps.h"
#include "cpps_task.h"
#include "prf_utils.h"

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// Cycling Power Profile Server Role Environment
struct cpps_env_tag cpps_env __attribute__((section("retention_mem_area0"),zero_init)); //@RETENTION MEMORY

/// Cycling Power Profile Server Role Task
static const struct ke_task_desc TASK_DESC_CPPS = {cpps_state_handler, &cpps_default_handler, cpps_state,
                                                    CPPS_STATE_MAX, CPPS_IDX_MAX};

/*
 * EXPORTED FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */

void cpps_init(void)
{
    // Reset environment
    memset(&cpps_env, 0, sizeof(cpps_env));
    cpps_env.ntf_pending = false;

    // Create CPPS task
    ke_task_create(TASK_CPPS, &TASK_DESC_CPPS);

    // Go to IDLE state
    ke_state_set(TASK_CPPS, CPPS_DISABLED);
}

void cpps_send_rsp_ind(uint16_t handle, uint8_t req_op_code, uint8_t status)
{
    // Error response
    uint8_t rsp[CPP_CP_CNTL_PT_RSP_MIN_LEN];

    // Response Code
    rsp[0] = CPP_CTNL_PT_RSP_CODE;
    // Request Operation Code
    rsp[1] = req_op_code;
    // Response value
    rsp[2] = status;

    // Set the value in the database - If we are here the handle is valid
    attmdb_att_set_value(handle, 3, (uint8_t *)&rsp);

    // Send an indication
    prf_server_send_event((prf_env_struct *)&cpps_env, true, handle);

    // Wait for Confirm
    ke_state_set(TASK_CPPS, CPPS_WAIT_FOR_CFM);
}

void cpps_send_cmp_evt(uint8_t src_id, uint8_t dest_id, uint16_t conhdl,
                        uint8_t operation, uint8_t status)
{
    // Go back to the Connected state if the state is busy
    if ((ke_state_get(src_id) == CPPS_BUSY)
        || (ke_state_get(src_id) == CPPS_WAIT_FOR_CFM))
    {
        ke_state_set(src_id, CPPS_CONNECTED);
    }

    // Set the operation code
    cpps_env.operation = CPPS_RESERVED_OP_CODE;

    // Send the message
    struct cpps_cmp_evt *evt = KE_MSG_ALLOC(CPPS_CMP_EVT,
                                             dest_id, src_id,
                                             cpps_cmp_evt);

    evt->conhdl     = conhdl;
    evt->operation  = operation;
    evt->status     = status;

    ke_msg_send(evt);
}

void cpps_disable(uint16_t conhdl)
{
    // Disable CPS in database
    attmdb_svc_set_permission(cpps_env.shdl, PERM_RIGHT_DISABLE);

    //Send current configuration to APP
    struct cpps_disable_ind* ind = KE_MSG_ALLOC(CPPS_DISABLE_IND,
                                                 cpps_env.con_info.appid, cpps_env.con_info.prf_id,
                                                 cpps_disable_ind);

    ind->conhdl = conhdl;

    // CP Measurement Char. - Client Characteristic Configuration Descriptor
    ind->prfl_ntf_ind_cfg = CPPS_IS_NTF_IND_BCST_ENABLED(CPP_PRF_CFG_FLAG_CP_MEAS_NTF);

    if (CPPS_IS_FEATURE_SUPPORTED(cpps_env.prfl_cfg, CPPS_MEAS_BCST_MASK))
    {
        // CP Measurement Char. - Server Characteristic Configuration Descriptor
        ind->prfl_ntf_ind_cfg |= CPPS_IS_NTF_IND_BCST_ENABLED(CPP_PRF_CFG_FLAG_SP_MEAS_NTF);
    }

    if (CPPS_IS_FEATURE_SUPPORTED(cpps_env.prfl_cfg, CPPS_VECTOR_MASK))
    {
        // CP Vector. - Client Characteristic Configuration Descriptor
        ind->prfl_ntf_ind_cfg |= CPPS_IS_NTF_IND_BCST_ENABLED(CPP_PRF_CFG_FLAG_VECTOR_NTF);
    }

    if (CPPS_IS_FEATURE_SUPPORTED(cpps_env.prfl_cfg, CPPS_CTNL_PT_MASK))
    {
        // Control Point Char. - Client Characteristic Configuration Descriptor
        ind->prfl_ntf_ind_cfg |= CPPS_IS_NTF_IND_BCST_ENABLED(CPP_PRF_CFG_FLAG_CTNL_PT_IND);
    }

    // Instantaneous Power
    ind->instantaneous_power = cpps_env.inst_power;

    ke_msg_send(ind);

    // Reset the notification/indication status
    cpps_env.prfl_cfg &= CPP_FEAT_ALL_SUPP;

    // Go to idle state
    ke_state_set(TASK_CPPS, CPPS_IDLE);
}

#endif //(BLE_CP_SENSOR)

/// @} CPPS
