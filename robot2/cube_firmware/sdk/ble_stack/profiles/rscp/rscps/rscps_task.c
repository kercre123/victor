/**
 ****************************************************************************************
 *
 * @file rscps_task.c
 *
 * @brief Running Speed and Cadence Profile Sensor Task Implementation.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 * $ Rev $
 *
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @addtogroup RSCPSTASK
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "rwble_config.h" 

#if (BLE_RSC_SENSOR)
#include "rscp_common.h"
#include "gap.h"
#include "gattc_task.h"
#include "atts_util.h"
#include "attm_util.h"
#include "rscps.h"
#include "rscps_task.h"
#include "prf_utils.h"

/*
 *  RUNNING SPEED AND CADENCE SERVICE ATTRIBUTES
 ****************************************************************************************
 */

/// Running Speed and Cadence Sensor Service
static const att_svc_desc_t rscps_rscs_svc              = ATT_SVC_RUNNING_SPEED_CADENCE;

/// RSC Measurement characteristic value
static const struct att_char_desc rscps_rsc_meas_char   = ATT_CHAR(ATT_CHAR_PROP_NTF,
                                                                   RSCP_RSCS_RSC_MEAS_CHAR,
                                                                   ATT_CHAR_RSC_MEAS);
/// RSC Feature characteristic value
static const struct att_char_desc rscps_rsc_feat_char   = ATT_CHAR(ATT_CHAR_PROP_RD,
                                                                   RSCP_RSCS_RSC_FEAT_CHAR,
                                                                   ATT_CHAR_RSC_FEAT);
/// Sensor Location characteristic value
static const struct att_char_desc rscps_sensor_loc_char = ATT_CHAR(ATT_CHAR_PROP_RD,
                                                                   RSCP_RSCS_SENSOR_LOC_CHAR,
                                                                   ATT_CHAR_SENSOR_LOC);
/// SC Control Point characteristic value
static const struct att_char_desc rscps_sc_ctnl_pt_char = ATT_CHAR(ATT_CHAR_PROP_WR | ATT_CHAR_PROP_IND,
                                                                   RSCP_RSCS_SC_CTNL_PT_CHAR,
                                                                   ATT_CHAR_SC_CNTL_PT);

/// Full RSCPS Database Description - Used to add attributes into the database
static const struct attm_desc rscps_att_db[RSCS_IDX_NB] =
{
    /// Running Speed and Cadence Service Declaration
    [RSCS_IDX_SVC]                     =   {ATT_DECL_PRIMARY_SERVICE, PERM(RD, ENABLE), sizeof(rscps_rscs_svc),
                                            sizeof(rscps_rscs_svc), (uint8_t *)&rscps_rscs_svc},

    /// RSC Measurement Characteristic Declaration
    [RSCS_IDX_RSC_MEAS_CHAR]           =   {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), sizeof(rscps_rsc_meas_char),
                                            sizeof(rscps_rsc_meas_char), (uint8_t *)&rscps_rsc_meas_char},
    /// RSC Measurement Characteristic Value
    [RSCS_IDX_RSC_MEAS_VAL]            =   {ATT_CHAR_RSC_MEAS, PERM(NTF, ENABLE), RSCP_RSC_MEAS_MAX_LEN,
                                            0, NULL},
    /// RSC Measurement Characteristic - Client Characteristic Configuration Descriptor
    [RSCS_IDX_RSC_MEAS_NTF_CFG]        =   {ATT_DESC_CLIENT_CHAR_CFG, PERM(RD, ENABLE) | PERM(WR, ENABLE), sizeof(uint16_t),
                                            0, NULL},

    /// RSC Feature Characteristic Declaration
    [RSCS_IDX_RSC_FEAT_CHAR]           =   {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), sizeof(rscps_rsc_feat_char),
                                            sizeof(rscps_rsc_feat_char), (uint8_t *)&rscps_rsc_feat_char},
    /// RSC Feature Characteristic Value
    [RSCS_IDX_RSC_FEAT_VAL]            =   {ATT_CHAR_RSC_FEAT, PERM(RD, ENABLE), sizeof(uint16_t),
                                            0, NULL},

    /// Sensor Location Characteristic Declaration
    [RSCS_IDX_SENSOR_LOC_CHAR]         =   {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), sizeof(rscps_sensor_loc_char),
                                            sizeof(rscps_sensor_loc_char), (uint8_t *)&rscps_sensor_loc_char},
    /// Sensor Location Characteristic Value
    [RSCS_IDX_SENSOR_LOC_VAL]          =   {ATT_CHAR_SENSOR_LOC, PERM(RD, ENABLE), sizeof(uint8_t),
                                            0, NULL},

    /// SC Control Point Characteristic Declaration
    [RSCS_IDX_SC_CTNL_PT_CHAR]         =   {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), sizeof(rscps_sc_ctnl_pt_char),
                                            sizeof(rscps_sc_ctnl_pt_char), (uint8_t *)&rscps_sc_ctnl_pt_char},
    /// SC Control Point Characteristic Value - The response has the maximal length
    [RSCS_IDX_SC_CTNL_PT_VAL]          =   {ATT_CHAR_SC_CNTL_PT, PERM(WR, ENABLE) | PERM(IND, ENABLE), RSCP_SC_CNTL_PT_RSP_MAX_LEN,
                                            0, NULL},
    /// SC Control Point Characteristic - Client Characteristic Configuration Descriptor
    [RSCS_IDX_SC_CTNL_PT_NTF_CFG]      =   {ATT_DESC_CLIENT_CHAR_CFG, PERM(RD, ENABLE) | PERM(WR, ENABLE), sizeof(uint16_t),
                                            0, NULL},
};

/*
 * LOCAL FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref RSCPS_CREATE_DB_REQ message.
 * @param[in] msgid Id of the message received.
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int rscps_create_db_req_handler(ke_msg_id_t const msgid,
                                       struct rscps_create_db_req *param,
                                       ke_task_id_t const dest_id,
                                       ke_task_id_t const src_id)
{
    // Service Configuration Flag
    uint16_t cfg_flag = RSCPS_MANDATORY_MASK;
    // Database Creation Status
    uint8_t status;

    // Check if a Running Speed and Cadence service has already been added in the database
    if (ke_state_get(TASK_RSCPS) == RSCPS_DISABLED)
    {
        /*
         * Check if the Sensor Location characteristic shall be added.
         * Mandatory if the Multiple Sensor Location feature is supported, otherwise optional.
         */
        if ((param->sensor_loc_supp == RSCPS_SENSOR_LOC_SUPP) ||
            (RSCPS_IS_FEATURE_SUPPORTED(param->rsc_feature, RSCP_FEAT_MULT_SENSOR_LOC_SUPP)))
        {
            cfg_flag |= RSCPS_SENSOR_LOC_MASK;
        }

        /*
         * Check if the SC Control Point characteristic shall be added
         * Mandatory if at least one SC Control Point procedure is supported, otherwise excluded.
         */
        if (RSCPS_IS_FEATURE_SUPPORTED(param->rsc_feature, RSCP_FEAT_TOTAL_DST_MEAS_SUPP) ||
            RSCPS_IS_FEATURE_SUPPORTED(param->rsc_feature, RSCP_FEAT_CALIB_PROC_SUPP)     ||
            RSCPS_IS_FEATURE_SUPPORTED(param->rsc_feature, RSCP_FEAT_MULT_SENSOR_LOC_SUPP))
        {
            cfg_flag |= RSCPS_SC_CTNL_PT_MASK;
        }

        // Add service in the database
        status = attm_svc_create_db(&rscps_env.shdl, (uint8_t *)&cfg_flag, RSCS_IDX_NB,
                                    &rscps_env.hdl_offset[0], dest_id, &rscps_att_db[0]);

        // Check if an error has occurred
        if (status == ATT_ERR_NO_ERROR)
        {
            // Force the unused bits of the RSC Feature Char value to 0
            param->rsc_feature &= RSCP_FEAT_ALL_SUPP;

            // Store the value in the environment
            rscps_env.prf_cfg = param->rsc_feature;

            // Set the value of the RSC Feature characteristic
            attmdb_att_set_value(rscps_env.shdl + RSCS_IDX_RSC_FEAT_VAL, sizeof(uint16_t),
                                 (uint8_t *)&param->rsc_feature);

            // If the Sensor Location characteristic has been added, set the value
            if (rscps_env.hdl_offset[RSCP_RSCS_SENSOR_LOC_CHAR] != 0x00)
            {
                // Check if the provided value is valid, else force the location to Other
                if (param->sensor_loc >= RSCP_LOC_MAX)
                {
                    param->sensor_loc = RSCP_LOC_OTHER;
                }

                // Set the value of the Sensor Location characteristic
                attmdb_att_set_value(rscps_env.shdl + RSCS_IDX_SENSOR_LOC_VAL, sizeof(uint8_t),
                                     (uint8_t *)&param->sensor_loc);
            }

            // Disable RSCS
            attmdb_svc_set_permission(rscps_env.shdl, PERM(SVC, DISABLE));

            // If we are here, database has been fulfilled with success, go to idle state
            ke_state_set(TASK_RSCPS, RSCPS_IDLE);
        }
    }
    else
    {
        // Request is disallowed, a RSCS has already been added
        status = PRF_ERR_REQ_DISALLOWED;
    }

    // Send complete event message to the application
    rscps_send_cmp_evt(TASK_RSCPS, src_id, GAP_INVALID_CONHDL, RSCPS_CREATE_DB_OP_CODE, status);

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref RSCPS_ENABLE_REQ message.
 * @param[in] msgid Id of the message received
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int rscps_enable_req_handler(ke_msg_id_t const msgid,
                                    struct rscps_enable_req *param,
                                    ke_task_id_t const dest_id,
                                    ke_task_id_t const src_id)
{
    // Status
    uint8_t status = PRF_ERR_OK;

    // Check the current state of the task and if the provided connection handle is valid
    if ((ke_state_get(TASK_RSCPS) == RSCPS_IDLE) &&
        (gapc_get_conidx(param->conhdl) != GAP_INVALID_CONIDX))
    {
        // Fill the Connection Information structure
        rscps_env.con_info.conidx = gapc_get_conidx(param->conhdl);
        rscps_env.con_info.prf_id = dest_id;
        rscps_env.con_info.appid  = src_id;

        // Reset the store Client Characteristic Configuration status
        rscps_env.prf_cfg &= RSCP_FEAT_ALL_SUPP;

        // Set the RSC Measurement Char. Client Characteristic Configuration value
        if (param->con_type == PRF_CON_DISCOVERY)
        {
            // Force the configuration to 0
            param->rsc_meas_ntf_cfg   = PRF_CLI_STOP_NTFIND;
        }
        else
        {
            // Check the provided value
            if (param->rsc_meas_ntf_cfg == PRF_CLI_START_NTF)
            {
                // Store the status
                RSCPS_ENABLE_NTFIND(RSCP_PRF_CFG_FLAG_RSC_MEAS_NTF);
            }
            else if (param->rsc_meas_ntf_cfg != PRF_CLI_STOP_NTFIND)
            {
                status = PRF_ERR_INVALID_PARAM;
            }
        }

        // If supported, set the SC Control Point Char. Client Characteristic Configuration value
        if ((status == PRF_ERR_OK) &&
            (rscps_env.hdl_offset[RSCP_RSCS_SC_CTNL_PT_CHAR] != 0x00))
        {
            if (param->con_type == PRF_CON_DISCOVERY)
            {
                // Force the configuration to 0
                param->sc_ctnl_pt_ntf_cfg   = PRF_CLI_STOP_NTFIND;
            }
            else
            {
                // Check the provided value
                if (param->sc_ctnl_pt_ntf_cfg == PRF_CLI_START_IND)
                {
                    // Store the status
                    RSCPS_ENABLE_NTFIND(RSCP_PRF_CFG_FLAG_SC_CTNL_PT_IND);
                }
                else if (param->sc_ctnl_pt_ntf_cfg != PRF_CLI_STOP_NTFIND)
                {
                    status = PRF_ERR_INVALID_PARAM;
                }
            }
        }

        if (status == PRF_ERR_OK)
        {
            // Set the RSC Measurement Char. Client Characteristic Configuration value in the database
            attmdb_att_set_value(rscps_env.shdl + RSCS_IDX_RSC_MEAS_NTF_CFG,
                                 sizeof(uint16_t), (uint8_t *)&param->rsc_meas_ntf_cfg);

            if (rscps_env.hdl_offset[RSCP_RSCS_SC_CTNL_PT_CHAR] != 0x00)
            {
                // Set the SC Control Point Char. Client Characteristic Configuration value in the database
                attmdb_att_set_value(rscps_env.shdl + rscps_env.hdl_offset[RSCP_RSCS_SC_CTNL_PT_CHAR] + 2,
                                     sizeof(uint16_t), (uint8_t *)&param->sc_ctnl_pt_ntf_cfg);
            }

            // Enable RSCS
            attmdb_svc_set_permission(rscps_env.shdl, param->sec_lvl);

            // Go to Connected State
            ke_state_set(dest_id, RSCPS_CONNECTED);
        }
    }
    else
    {
        // The request is disallowed (profile already enabled for this connection, or not enough memory, ...)
        status = PRF_ERR_REQ_DISALLOWED;
    }

    // Send response to application
    rscps_send_cmp_evt(dest_id, src_id, param->conhdl, RSCPS_ENABLE_OP_CODE, status);

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref RSCPS_NTF_RSC_MEAS_CMD message.
 * @param[in] msgid Id of the message received.
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance.
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int rscps_ntf_rsc_meas_cmd_handler(ke_msg_id_t const msgid,
                                          struct rscps_ntf_rsc_meas_cmd *param,
                                          ke_task_id_t const dest_id,
                                          ke_task_id_t const src_id)
{
    // Message status
    uint8_t msg_status = KE_MSG_CONSUMED;
    // State
    ke_state_t state   = ke_state_get(TASK_RSCPS);

    // State shall be Connected or Busy
    if (state >= RSCPS_CONNECTED)
    {
        // Status
        uint8_t status     = PRF_ERR_OK;

        // Packed Measurement
        uint8_t pckd_meas[RSCP_RSC_MEAS_MAX_LEN];
        // Packed Measurement length
        uint8_t pckd_meas_len = RSCP_RSC_MEAS_MIN_LEN;

        do
        {
            // Check the connection handle
            if (param->conhdl != gapc_get_conhdl(rscps_env.con_info.conidx))
            {
                status = PRF_ERR_INVALID_PARAM;
                break;
            }

            // Check if sending of notifications has been enabled
            if (!RSCPS_IS_NTFIND_ENABLED(RSCP_PRF_CFG_FLAG_RSC_MEAS_NTF))
            {
                status = PRF_ERR_NTF_DISABLED;
                break;
            }

            // Check the current state of the task
            if (state == RSCPS_BUSY)
            {
                // Keep the message for later, status will be PRF_ERR_OK
                msg_status = KE_MSG_SAVED;
                break;
            }

            ASSERT_ERR(rscps_env.operation == RSCPS_RESERVED_OP_CODE);

            // Force the unused bits of the flag value to 0
            param->flags &= RSCP_MEAS_ALL_PRESENT;

            // Check the provided flags value
            if (!RSCPS_IS_FEATURE_SUPPORTED(rscps_env.prf_cfg, RSCP_FEAT_INST_STRIDE_LEN_SUPP) &&
                RSCPS_IS_PRESENT(param->flags, RSCP_MEAS_INST_STRIDE_LEN_PRESENT))
            {
                // Force Measurement Instantaneous Stride Length Present to No (Not supported)
                param->flags &= ~RSCP_MEAS_INST_STRIDE_LEN_PRESENT;
            }

            if (!RSCPS_IS_FEATURE_SUPPORTED(rscps_env.prf_cfg, RSCP_FEAT_TOTAL_DST_MEAS_SUPP) &&
                RSCPS_IS_PRESENT(param->flags, RSCP_MEAS_TOTAL_DST_MEAS_PRESENT))
            {
                // Force Total Distance Measurement Present to No (Not supported)
                param->flags &= ~RSCP_MEAS_TOTAL_DST_MEAS_PRESENT;
            }

            // Flag value
            pckd_meas[0] = param->flags;
            // Instantaneous Speed
            co_write16p(&pckd_meas[1], param->inst_speed);
            // Instantaneous Cadence
            pckd_meas[3] = param->inst_cad;

            // Instantaneous Stride Length
            if (RSCPS_IS_PRESENT(param->flags, RSCP_MEAS_INST_STRIDE_LEN_PRESENT))
            {
                co_write16p(&pckd_meas[pckd_meas_len], param->inst_stride_len);
                pckd_meas_len += 2;
            }

            // Total Distance
            if (RSCPS_IS_PRESENT(param->flags, RSCP_MEAS_TOTAL_DST_MEAS_PRESENT))
            {
                co_write32p(&pckd_meas[pckd_meas_len], param->total_dist);
                pckd_meas_len += 4;
            }

            // Set the value in the database
            attmdb_att_set_value(rscps_env.shdl + RSCS_IDX_RSC_MEAS_VAL, pckd_meas_len,
                                 (uint8_t *)&pckd_meas[0]);

            // Send the notification
            prf_server_send_event((prf_env_struct *)&rscps_env, false,
                                  rscps_env.shdl + RSCS_IDX_RSC_MEAS_VAL);

            // Configure the environment
            rscps_env.operation = RSCPS_SEND_RSC_MEAS_OP_CODE;
            // Go to busy state
            ke_state_set(TASK_RSCPS, RSCPS_BUSY);
        } while (0);

        if (status != PRF_ERR_OK)
        {
            // Send response to application
            rscps_send_cmp_evt(rscps_env.con_info.prf_id, rscps_env.con_info.appid, gapc_get_conhdl(rscps_env.con_info.conidx),
                               RSCPS_SEND_RSC_MEAS_OP_CODE, status);
        }
    }
    else
    {
        // The profile has not been enabled for a connection or the database has not been created
        rscps_send_cmp_evt(dest_id, src_id, param->conhdl, RSCPS_SEND_RSC_MEAS_OP_CODE, PRF_ERR_REQ_DISALLOWED);
    }

    return (int)msg_status;
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref RSCPS_SC_CTNL_PT_CFM message.
 * @param[in] msgid Id of the message received.
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance.
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int rscps_sc_ctnl_pt_cfm_handler(ke_msg_id_t const msgid,
                                          struct rscps_sc_ctnl_pt_cfm *param,
                                          ke_task_id_t const dest_id,
                                          ke_task_id_t const src_id)
{
    // SC Control Point Response
    uint8_t rsp[RSCP_SC_CNTL_PT_RSP_MAX_LEN];
    // Response Length (At least 3)
    uint8_t rsp_len = RSCP_SC_CNTL_PT_RSP_MIN_LEN;

    do
    {
        // Check the current operation
        if (rscps_env.operation < RSCPS_CTNL_PT_CUMUL_VAL_OP_CODE)
        {
            // The confirmation has been sent without request indication, ignore
            break;
        }

        // The SC Control Point Characteristic is supported if we are here
        ASSERT_ERR(rscps_env.hdl_offset[RSCP_RSCS_SC_CTNL_PT_CHAR] != 0x00);

        // Set the operation code (Response Code)
        rsp[0] = RSCP_CTNL_PT_RSP_CODE;
        // Set the response value
        rsp[2] = (param->status == PRF_ERR_OK) ? RSCP_CTNL_PT_RESP_SUCCESS : RSCP_CTNL_PT_RESP_FAILED;

        switch (rscps_env.operation)
        {
            // Set cumulative value
            case (RSCPS_CTNL_PT_CUMUL_VAL_OP_CODE):
            {
                // Set the request operation code
                rsp[1] = RSCP_CTNL_PT_OP_SET_CUMUL_VAL;
            } break;

            // Start Sensor Calibration
            case (RSCPS_CTNL_PT_START_CAL_OP_CODE):
            {
                // Set the request operation code
                rsp[1] = RSCP_CTNL_PT_OP_START_CALIB;
            } break;

            // Update Sensor Location
            case (RSCPS_CTNL_PT_UPD_LOC_OP_CODE):
            {
                // Set the request operation code
                rsp[1] = RSCP_CTNL_PT_OP_UPD_LOC;

                if (param->status == PRF_ERR_OK)
                {
                    // The Sensor Location Characteristic is supported if we are here
                    ASSERT_ERR(rscps_env.hdl_offset[RSCP_RSCS_SENSOR_LOC_CHAR] != 0x00);

                    // If operation is successful, update the value in the database
                    attmdb_att_set_value(rscps_env.shdl + rscps_env.hdl_offset[RSCP_RSCS_SENSOR_LOC_CHAR] + 1,
                                         sizeof(uint8_t), (uint8_t *)&param->value.sensor_loc);
                }
            } break;

            case (RSCPS_CTNL_PT_SUPP_LOC_OP_CODE):
            {
                // Set the request operation code
                rsp[1] = RSCP_CTNL_PT_OP_REQ_SUPP_LOC;

                if (param->status == PRF_ERR_OK)
                {
                    // Counter
                    uint8_t counter;

                    // Set the list of supported location
                    for (counter = 0; counter < RSCP_LOC_MAX; counter++)
                    {
                        if (((param->value.supp_sensor_loc >> counter) & 0x0001) == 0x0001)
                        {
                            rsp[rsp_len] = counter;
                            rsp_len++;
                        }
                    }
                }
            } break;

            default:
            {
                ASSERT_ERR(0);
            } break;
        }

        // Set the value in the database - If we are here the handle is valid
        attmdb_att_set_value(rscps_env.shdl + rscps_env.hdl_offset[RSCP_RSCS_SC_CTNL_PT_CHAR] + 1,
                             rsp_len, (uint8_t *)&rsp);

        // Send the response indication to the peer device
        prf_server_send_event((prf_env_struct *)&rscps_env, true,
                              rscps_env.shdl + rscps_env.hdl_offset[RSCP_RSCS_SC_CTNL_PT_CHAR] + 1);
    } while (0);

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref GATTC_WRITE_CMD_IND message.
 * @param[in] msgid Id of the message received.
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance.
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gattc_write_cmd_ind_handler(ke_msg_id_t const msgid,
                                       struct gattc_write_cmd_ind const *param,
                                       ke_task_id_t const dest_id,
                                       ke_task_id_t const src_id)
{
    // Message status
    uint8_t msg_status = KE_MSG_CONSUMED;

    // check if waiting for indication 
    if((ke_state_get(TASK_RSCPS) == RSCPS_WAIT_FOR_CFM)
            && (KE_TYPE_GET(src_id) == TASK_GATTC))
    {
        // reschedule message once to ensure that indication reception is not on-going.
        ke_msg_forward(param, dest_id, dest_id);

        // Message forwarded do not free it
        msg_status = KE_MSG_NO_FREE;
    }
    // Check if a connection exists
    else if (ke_state_get(TASK_RSCPS) >= RSCPS_CONNECTED)
    {
        // RSC Measurement Characteristic, Client Characteristic Configuration Descriptor
        if (param->handle == (rscps_env.shdl + RSCS_IDX_RSC_MEAS_NTF_CFG))
        {
            uint16_t ntf_cfg;
            // Status
            uint8_t status = RSCP_ERROR_CCC_INVALID_PARAM;

            // Get the value
            co_write16p(&ntf_cfg, param->value[0]);

            // Check if the value is correct
            if (ntf_cfg <= PRF_CLI_START_NTF)
            {
                status = PRF_ERR_OK;

                // Set the value in the database
                attmdb_att_set_value(param->handle, sizeof(uint16_t), (uint8_t *)&ntf_cfg);

                // Save the new configuration in the environment
                if (ntf_cfg == PRF_CLI_STOP_NTFIND)
                {
                    RSCPS_DISABLE_NTFIND(RSCP_PRF_CFG_FLAG_RSC_MEAS_NTF);
                }
                else    // ntf_cfg == PRF_CLI_START_NTF
                {
                    RSCPS_ENABLE_NTFIND(RSCP_PRF_CFG_FLAG_RSC_MEAS_NTF);
                }

                // Inform the HL about the new configuration
                struct rscps_ntf_ind_cfg_ind *ind = KE_MSG_ALLOC(RSCPS_NTF_IND_CFG_IND,
                                                                 rscps_env.con_info.appid, rscps_env.con_info.prf_id,
                                                                 rscps_ntf_ind_cfg_ind);

                ind->conhdl    = gapc_get_conhdl(rscps_env.con_info.conidx);
                ind->char_code = RSCP_RSCS_RSC_MEAS_CHAR;
                ind->ntf_cfg   = ntf_cfg;

                ke_msg_send(ind);
            }
            // else status is RSCP_ERROR_CCC_INVALID_PARAM

            // Send the write response to the peer device
            atts_write_rsp_send(rscps_env.con_info.conidx, param->handle, status);
        }
        else        // Should be the SC Control Point Characteristic
        {
            ASSERT_ERR(rscps_env.hdl_offset[RSCP_RSCS_SC_CTNL_PT_CHAR] != 0x00);

            // SC Control Point, Client Characteristic Configuration Descriptor
            if (param->handle == (rscps_env.shdl + rscps_env.hdl_offset[RSCP_RSCS_SC_CTNL_PT_CHAR] + 2))
            {
                uint16_t ntf_cfg;
                // Status
                uint8_t status = RSCP_ERROR_CCC_INVALID_PARAM;

                // Get the value
                co_write16p(&ntf_cfg, param->value[0]);

                // Check if the value is correct
                if ((ntf_cfg == PRF_CLI_STOP_NTFIND) || (ntf_cfg == PRF_CLI_START_IND))
                {
                    status = PRF_ERR_OK;

                    // Set the value in the database
                    attmdb_att_set_value(param->handle, sizeof(uint16_t), (uint8_t *)&ntf_cfg);

                    // Save the new configuration in the environment
                    if (ntf_cfg == PRF_CLI_STOP_NTFIND)
                    {
                        RSCPS_DISABLE_NTFIND(RSCP_PRF_CFG_FLAG_SC_CTNL_PT_IND);
                    }
                    else    // ntf_cfg == PRF_CLI_START_IND
                    {
                        RSCPS_ENABLE_NTFIND(RSCP_PRF_CFG_FLAG_SC_CTNL_PT_IND);
                    }

                    // Inform the HL about the new configuration
                    struct rscps_ntf_ind_cfg_ind *ind = KE_MSG_ALLOC(RSCPS_NTF_IND_CFG_IND,
                                                                     rscps_env.con_info.appid, rscps_env.con_info.prf_id,
                                                                     rscps_ntf_ind_cfg_ind);

                    ind->conhdl    = gapc_get_conhdl(rscps_env.con_info.conidx);
                    ind->char_code = RSCP_RSCS_SC_CTNL_PT_CHAR;
                    ind->ntf_cfg   = ntf_cfg;

                    ke_msg_send(ind);
                }
                // else status is RSCP_ERROR_CCC_INVALID_PARAM

                // Send the write response to the peer device
                atts_write_rsp_send(rscps_env.con_info.conidx, param->handle, status);
            }
            // SC Control Point Characteristic
            else if (param->handle == (rscps_env.shdl + rscps_env.hdl_offset[RSCP_RSCS_SC_CTNL_PT_CHAR] + 1))
            {
                // Write Response Status
                uint8_t wr_status  = PRF_ERR_OK;
                // Indication Status
                uint8_t ind_status = RSCP_CTNL_PT_RESP_NOT_SUPP;

                do
                {
                    // Check if sending of indications has been enabled
                    if (!RSCPS_IS_NTFIND_ENABLED(RSCP_PRF_CFG_FLAG_SC_CTNL_PT_IND))
                    {
                        // CCC improperly configured
                        wr_status = RSCP_ERROR_CCC_INVALID_PARAM;

                        break;
                    }

                    if (rscps_env.operation >= RSCPS_CTNL_PT_CUMUL_VAL_OP_CODE)
                    {
                        // A procedure is already in progress
                        wr_status = RSCP_ERROR_PROC_IN_PROGRESS;

                        break;
                    }

                    if (rscps_env.operation == RSCPS_SEND_RSC_MEAS_OP_CODE)
                    {
                        // Keep the message until the end of the current procedure
                        msg_status = KE_MSG_NO_FREE;

                        break;
                    }

                    // Allocate a request indication message for the application
                    struct rscps_sc_ctnl_pt_req_ind *req_ind = KE_MSG_ALLOC(RSCPS_SC_CTNL_PT_REQ_IND,
                                                                            rscps_env.con_info.appid, rscps_env.con_info.prf_id,
                                                                            rscps_sc_ctnl_pt_req_ind);

                    // Connection Handle
                    req_ind->conhdl    = gapc_get_conhdl(rscps_env.con_info.conidx);
                    // Operation Code
                    req_ind->op_code   = param->value[0];

                    // Operation Code
                    switch(param->value[0])
                    {
                        // Set Cumulative value
                        case (RSCP_CTNL_PT_OP_SET_CUMUL_VAL):
                        {
                            // Check if the Total Distance Measurement feature is supported
                            if (RSCPS_IS_FEATURE_SUPPORTED(rscps_env.prf_cfg, RSCP_FEAT_TOTAL_DST_MEAS_SUPP))
                            {
                                // The request can be handled
                                ind_status = PRF_ERR_OK;

                                rscps_env.operation = RSCPS_CTNL_PT_CUMUL_VAL_OP_CODE;

                                // Cumulative value
                                req_ind->value.cumul_value = co_read32p(&param->value[1]);
                            }
                        } break;

                        // Start Sensor calibration
                        case (RSCP_CTNL_PT_OP_START_CALIB):
                        {
                            // Check if the Calibration Procedure feature is supported
                            if (RSCPS_IS_FEATURE_SUPPORTED(rscps_env.prf_cfg, RSCP_FEAT_CALIB_PROC_SUPP))
                            {
                                // The request can be handled
                                ind_status = PRF_ERR_OK;

                                rscps_env.operation = RSCPS_CTNL_PT_START_CAL_OP_CODE;
                            }
                        } break;

                        // Update sensor location
                        case (RSCP_CTNL_PT_OP_UPD_LOC):
                        {
                            // Check if the Multiple Sensor Location feature is supported
                            if (RSCPS_IS_FEATURE_SUPPORTED(rscps_env.prf_cfg, RSCP_FEAT_MULT_SENSOR_LOC_SUPP))
                            {
                                // Check the sensor location value
                                if (param->value[1] < RSCP_LOC_MAX)
                                {
                                    // The request can be handled
                                    ind_status = PRF_ERR_OK;

                                    rscps_env.operation = RSCPS_CTNL_PT_UPD_LOC_OP_CODE;

                                    // Sensor Location
                                    req_ind->value.sensor_loc = param->value[1];
                                }
                                else
                                {
                                    // Provided parameter in not within the defined range
                                    ind_status = RSCP_CTNL_PT_RESP_INV_PARAM;
                                }
                            }
                        } break;

                        // Request supported sensor locations
                        case (RSCP_CTNL_PT_OP_REQ_SUPP_LOC):
                        {
                            // Check if the Multiple Sensor Location feature is supported
                            if (RSCPS_IS_FEATURE_SUPPORTED(rscps_env.prf_cfg, RSCP_FEAT_MULT_SENSOR_LOC_SUPP))
                            {
                                // The request can be handled
                                ind_status = PRF_ERR_OK;

                                rscps_env.operation = RSCPS_CTNL_PT_SUPP_LOC_OP_CODE;
                            }
                        } break;

                        default:
                        {
                            // Operation Code is invalid, status is already RSCP_CTNL_PT_RESP_NOT_SUPP
                        } break;
                    }

                    // Go to the Busy status
                    ke_state_set(TASK_RSCPS, RSCPS_BUSY);

                    // If no error raised, inform the application about the request
                    if (ind_status == PRF_ERR_OK)
                    {
                        // Send the request indication to the application
                        ke_msg_send(req_ind);
                    }
                    else
                    {
                        // Free the allocated message
                        ke_msg_free(ke_param2msg(req_ind));

                        // Send an error indication
                        rscps_send_rsp_ind(param->handle, param->value[0], ind_status);

                        rscps_env.operation = RSCPS_CTNL_ERR_IND_OP_CODE;
                    }
                } while (0);

                // Send the write response to the peer device
                atts_write_rsp_send(rscps_env.con_info.conidx, param->handle, wr_status);
            }
            else
            {
                ASSERT_ERR(0);
            }
        }
    }
    // else drop the message

    return (int)msg_status;
}

/**
 ****************************************************************************************
 * @brief Handles @ref GATT_NOTIFY_CMP_EVT message meaning that a notification or an indication
 * has been correctly sent to peer device (but not confirmed by peer device).
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance
 * @param[in] src_id    ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gattc_cmp_evt_handler(ke_msg_id_t const msgid,  struct gattc_cmp_evt const *param,
                                 ke_task_id_t const dest_id, ke_task_id_t const src_id)
{
    // Check if a connection exists
    if (ke_state_get(TASK_RSCPS) >= RSCPS_CONNECTED)
    {
        switch (param->req_type)
        {
            case (GATTC_NOTIFY):
            {
                    ASSERT_ERR(rscps_env.operation == RSCPS_SEND_RSC_MEAS_OP_CODE);

                    // Inform the application that a procedure has been completed
                    rscps_send_cmp_evt(rscps_env.con_info.prf_id, rscps_env.con_info.appid, gapc_get_conhdl(rscps_env.con_info.conidx),
                                       RSCPS_SEND_RSC_MEAS_OP_CODE, param->status);
                // else ignore the message
            } break;

            case (GATTC_INDICATE):
            {
                    ASSERT_ERR(rscps_env.operation >= RSCPS_CTNL_PT_CUMUL_VAL_OP_CODE);

                    // Inform the application that a procedure has been completed
                    rscps_send_cmp_evt(rscps_env.con_info.prf_id, rscps_env.con_info.appid, gapc_get_conhdl(rscps_env.con_info.conidx),
                                       rscps_env.operation, param->status);
                // else ignore the message
            } break;

            default:
            {
                ASSERT_ERR(0);
            } break;
        }
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Disconnection indication to RSCPS.
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance
 * @param[in] src_id    ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gapc_disconnect_ind_handler(ke_msg_id_t const msgid,
                                        struct gapc_disconnect_ind const *param,
                                        ke_task_id_t const dest_id,
                                        ke_task_id_t const src_id)
{
    // Check if a connection exists
    if (KE_IDX_GET(src_id) == rscps_env.con_info.conidx)
    {
        rscps_disable(param->conhdl);
    }

    return (KE_MSG_CONSUMED);
}

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// Specifies the default message handlers
const struct ke_msg_handler rscps_default_state[] =
{
    {RSCPS_CREATE_DB_REQ,           (ke_msg_func_t)rscps_create_db_req_handler},
    {RSCPS_ENABLE_REQ,              (ke_msg_func_t)rscps_enable_req_handler},
    {RSCPS_NTF_RSC_MEAS_CMD,        (ke_msg_func_t)rscps_ntf_rsc_meas_cmd_handler},
    {RSCPS_SC_CTNL_PT_CFM,          (ke_msg_func_t)rscps_sc_ctnl_pt_cfm_handler},

    {GATTC_WRITE_CMD_IND,           (ke_msg_func_t)gattc_write_cmd_ind_handler},
    {GATTC_CMP_EVT,                 (ke_msg_func_t)gattc_cmp_evt_handler},

    {GAPC_DISCONNECT_IND,           (ke_msg_func_t)gapc_disconnect_ind_handler},
};

/// Specifies the message handler structure for every input state.
const struct ke_state_handler rscps_state_handler[RSCPS_STATE_MAX] =
{
    [RSCPS_DISABLED]    = KE_STATE_HANDLER_NONE,
    [RSCPS_IDLE]        = KE_STATE_HANDLER_NONE,
    [RSCPS_CONNECTED]   = KE_STATE_HANDLER_NONE,
    [RSCPS_BUSY]        = KE_STATE_HANDLER_NONE,
    [RSCPS_WAIT_FOR_CFM] = KE_STATE_HANDLER_NONE,
};

/// Specifies the message handlers that are common to all states.
const struct ke_state_handler rscps_default_handler = KE_STATE_HANDLER(rscps_default_state);

/// Defines the place holder for the states of all the task instances.
ke_state_t rscps_state[RSCPS_IDX_MAX] __attribute__((section("retention_mem_area0"),zero_init)); //@RETENTION MEMORY

#endif //(BLE_RSC_SENSOR)

/// @} RSCPSTASK
