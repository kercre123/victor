/**
 ****************************************************************************************
 *
 * @file cscps.c
 *
 * @brief Cycling Speed and Cadence Profile Sensor implementation.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 * $ Rev $
 *
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @addtogroup CSCPS
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "rwble_config.h"   

#if (BLE_CSC_SENSOR)
#include "cscp_common.h"
#include "gap.h"
#include "gattc_task.h"
#include "atts_util.h"
#include "attm_util.h"
#include "cscps.h"
#include "cscps_task.h"
#include "prf_utils.h"

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// Cycling Speed and Cadence Profile Server Role Environment
struct cscps_env_tag cscps_env __attribute__((section("retention_mem_area0"),zero_init)); //@RETENTION MEMORY

/// Cycling Speed and Cadence Profile Server Role Task
static const struct ke_task_desc TASK_DESC_CSCPS = {cscps_state_handler, &cscps_default_handler, cscps_state,
                                                    CSCPS_STATE_MAX, CSCPS_IDX_MAX};

/*
 * EXPORTED FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */

void cscps_init(void)
{
    // Reset environment
    memset(&cscps_env, 0, sizeof(cscps_env));

    // Create CSCPS task
    ke_task_create(TASK_CSCPS, &TASK_DESC_CSCPS);

    // Go to IDLE state
    ke_state_set(TASK_CSCPS, CSCPS_DISABLED);
}

void cscps_send_rsp_ind(uint16_t handle, uint8_t req_op_code, uint8_t status)
{
    // Error response
    uint8_t rsp[CSCP_SC_CNTL_PT_RSP_MIN_LEN];

    // Response Code
    rsp[0] = CSCP_CTNL_PT_RSP_CODE;
    // Request Operation Code
    rsp[1] = req_op_code;
    // Response value
    rsp[2] = status;

    // Set the value in the database - If we are here the handle is valid
    attmdb_att_set_value(handle, 3, (uint8_t *)&rsp);

    // Send an indication
    prf_server_send_event((prf_env_struct *)&cscps_env, true, handle);

    // Wait for Confirm
    ke_state_set(TASK_CSCPS, CSCPS_WAIT_FOR_CFM);
}

void cscps_send_cmp_evt(uint8_t src_id, uint8_t dest_id, uint16_t conhdl,
                        uint8_t operation, uint8_t status)
{
    // Go back to the Connected state if the state is busy
    if ((ke_state_get(src_id) == CSCPS_BUSY)
        || (ke_state_get(src_id) == CSCPS_WAIT_FOR_CFM))
    {
        ke_state_set(src_id, CSCPS_CONNECTED);
    }

    // Set the operation code
    cscps_env.operation = CSCPS_RESERVED_OP_CODE;

    // Send the message
    struct cscps_cmp_evt *evt = KE_MSG_ALLOC(CSCPS_CMP_EVT,
                                             dest_id, src_id,
                                             cscps_cmp_evt);

    evt->conhdl     = conhdl;
    evt->operation  = operation;
    evt->status     = status;

    ke_msg_send(evt);
}

void cscps_disable(uint16_t conhdl)
{
	// Disable CSCS in database
    attmdb_svc_set_permission(cscps_env.shdl, PERM_RIGHT_DISABLE);

    //Send current configuration to APP
    struct cscps_disable_ind* ind = KE_MSG_ALLOC(CSCPS_DISABLE_IND,
                                                 cscps_env.con_info.appid, cscps_env.con_info.prf_id,
                                                 cscps_disable_ind);

    ind->conhdl = conhdl;

    // CSC Measurement Char. - Client Characteristic Configuration Descriptor
    ind->csc_meas_ntf_cfg = CSCPS_IS_NTFIND_ENABLED(CSCP_PRF_CFG_FLAG_CSC_MEAS_NTF)
                            ? PRF_CLI_START_NTF : PRF_CLI_STOP_NTFIND;

    // SC Control Point Char. - Client Characteristic Configuration Descriptor
    if (cscps_env.hdl_offset[CSCP_CSCS_SC_CTNL_PT_CHAR] != 0)
    {
        ind->sc_ctnl_pt_ntf_cfg = CSCPS_IS_NTFIND_ENABLED(CSCP_PRF_CFG_FLAG_SC_CTNL_PT_IND)
                                  ? PRF_CLI_START_IND : PRF_CLI_STOP_NTFIND;
    }

    // Total wheel revolutions
    ind->wheel_revol = cscps_env.wheel_revol;

    ke_msg_send(ind);

    // Reset the notification/indication status
    cscps_env.prf_cfg &= CSCP_FEAT_ALL_SUPP;

    // Go to idle state
    ke_state_set(TASK_CSCPS, CSCPS_IDLE);
}

#endif //(BLE_CSC_SENSOR)

/// @} CSCPS
