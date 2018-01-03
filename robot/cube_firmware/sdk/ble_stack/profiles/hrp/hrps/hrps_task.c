/**
 ****************************************************************************************
 *
 * @file hrps_task.c
 *
 * @brief Heart Rate Profile Sensor Task Implementation.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 * $Rev$
 *
 ****************************************************************************************
 */


/**
 ****************************************************************************************
 * @addtogroup HRPSTASK
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "rwip_config.h"

#if (BLE_HR_SENSOR)
#include "gap.h"
#include "gattc_task.h"
#include "atts_util.h"
#include "attm_util.h"
#include "hrps.h"
#include "hrps_task.h"

#include "prf_utils.h"


/*
 * LOCAL FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */


/**
 ****************************************************************************************
 * @brief Handles reception of the @ref HRPS_CREATE_DB_REQ message.
 * The handler adds HRS into the database using the database
 * configuration value given in param.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int hrps_create_db_req_handler(ke_msg_id_t const msgid,
                                      struct hrps_create_db_req const *param,
                                      ke_task_id_t const dest_id,
                                      ke_task_id_t const src_id)
{
    //Service Configuration Flag
    uint8_t cfg_flag = HRPS_MANDATORY_MASK;
    //Database Creation Status
    uint8_t status;

    //Save Profile ID
    hrps_env.con_info.prf_id = TASK_HRPS;
    //Save Database Configuration
    hrps_env.features = param->features;

    /*---------------------------------------------------*
     * Heart Rate Service Creation
     *---------------------------------------------------*/
    //Set Configuration Flag Value
    if (HRPS_IS_SUPPORTED(HRPS_BODY_SENSOR_LOC_CHAR_SUP))
    {
        cfg_flag |= HRPS_BODY_SENSOR_LOC_MASK;
    }
    if (HRPS_IS_SUPPORTED(HRPS_ENGY_EXP_FEAT_SUP))
    {
        cfg_flag |= HRPS_HR_CTNL_PT_MASK;
    }

    //Add Service Into Database
    status = attm_svc_create_db(&hrps_env.shdl, (uint8_t *)&cfg_flag, HRS_IDX_NB, NULL,
                               dest_id, &hrps_att_db[0]);
    //Disable HRS
    attmdb_svc_set_permission(hrps_env.shdl, PERM(SVC, DISABLE));

    //Go to Idle State
    if (status == ATT_ERR_NO_ERROR)
    {
        //If we are here, database has been fulfilled with success, go to idle test
        ke_state_set(TASK_HRPS, HRPS_IDLE);
    }

    //Send response to application
    struct hrps_create_db_cfm * cfm = KE_MSG_ALLOC(HRPS_CREATE_DB_CFM, src_id,
                                                   TASK_HRPS, hrps_create_db_cfm);
    cfm->status = status;
    ke_msg_send(cfm);

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref HRPS_ENABLE_REQ message.
 * The handler enables the Heart Rate Sensor Profile.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int hrps_enable_req_handler(ke_msg_id_t const msgid,
                                   struct hrps_enable_req const *param,
                                   ke_task_id_t const dest_id,
                                   ke_task_id_t const src_id)
{
    uint16_t value = 0;

    // Save the application task id
    hrps_env.con_info.appid = src_id;
    // Save the connection handle associated to the profile
    hrps_env.con_info.conidx = gapc_get_conidx(param->conhdl);

    // Check if the provided connection exist
    if (hrps_env.con_info.conidx == GAP_INVALID_CONIDX)
    {
        // The connection doesn't exist, request disallowed
        prf_server_error_ind_send((prf_env_struct *)&hrps_env, PRF_ERR_REQ_DISALLOWED,
                                  HRPS_ERROR_IND, HRPS_ENABLE_REQ);
    }
    else
    {
        // If this connection is a not configuration one, apply config saved by app
        if(param->con_type == PRF_CON_NORMAL)
        {
            value = param->hr_meas_ntf_en;

            if (param->hr_meas_ntf_en == PRF_CLI_START_NTF)
            {
                hrps_env.features |= HRPS_HR_MEAS_NTF_CFG;
            }
        }

        //Set HR Meas. Char. NTF Configuration in DB
        attmdb_att_set_value(hrps_env.shdl + HRS_IDX_HR_MEAS_NTF_CFG, sizeof(uint16_t),
                             (uint8_t *)&value);

        if (HRPS_IS_SUPPORTED(HRPS_BODY_SENSOR_LOC_CHAR_SUP))
        {
            //Set Body Sensor Location Char Value in DB - Not supposed to change during connection
            attmdb_att_set_value(hrps_env.shdl + HRS_IDX_BOBY_SENSOR_LOC_VAL, sizeof(uint8_t),
                                 (uint8_t *)&param->body_sensor_loc);
        }

        // Enable Service + Set Security Level
        attmdb_svc_set_permission(hrps_env.shdl, param->sec_lvl);

        // Go to connected state
        ke_state_set(TASK_HRPS, HRPS_CONNECTED);
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref HRPS_MEAS_SEND_REQ message.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int hrps_meas_send_req_handler(ke_msg_id_t const msgid,
                                      struct hrps_meas_send_req const *param,
                                      ke_task_id_t const dest_id,
                                      ke_task_id_t const src_id)
{
    // Status
    uint8_t status = PRF_ERR_OK;
    // Packed Heart Measurement Value
    uint8_t packed_hr[HRPS_HT_MEAS_MAX_LEN];
    // Packet size
    uint8_t size;

    if((param->conhdl == gapc_get_conhdl(hrps_env.con_info.conidx))
            && (param->meas_val.nb_rr_interval <= HRS_MAX_RR_INTERVAL))
    {
        //Pack the HR Measurement value
        size = hrps_pack_meas_value(&packed_hr[0], &param->meas_val);

        // Check if notifications are enabled
        if(HRPS_IS_SUPPORTED(HRPS_HR_MEAS_NTF_CFG))
        {
            //Update value in DB
            attmdb_att_set_value(hrps_env.shdl + HRS_IDX_HR_MEAS_VAL,
                                 size, &packed_hr[0]);

            //send notification through GATT
            prf_server_send_event((prf_env_struct *)&hrps_env, false,
                    hrps_env.shdl + HRS_IDX_HR_MEAS_VAL);
        }
        //notification not enabled, simply don't send anything
        else
        {
            status = PRF_ERR_NTF_DISABLED;
        }
    }
    else
    {
        status = PRF_ERR_INVALID_PARAM;
    }

    if (status != PRF_ERR_OK)
    {
        // Value has not been sent
        hrps_meas_send_cfm_send(status);
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref GL2C_CODE_ATT_WR_CMD_IND message.
 * The handler compares the new values with current ones and notifies them if they changed.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gattc_write_cmd_ind_handler(ke_msg_id_t const msgid,
                                      struct gattc_write_cmd_ind const *param,
                                      ke_task_id_t const dest_id,
                                      ke_task_id_t const src_id)
{
    uint16_t value = 0x0000;
    uint8_t status = PRF_ERR_OK;

    if (KE_IDX_GET(src_id) == hrps_env.con_info.conidx)
    {
        //BP Measurement Char. - Client Char. Configuration
        if (param->handle == (hrps_env.shdl + HRS_IDX_HR_MEAS_NTF_CFG))
        {
            //Extract value before check
            memcpy(&value, &(param->value), sizeof(uint16_t));

            if ((value == PRF_CLI_STOP_NTFIND) || (value == PRF_CLI_START_NTF))
            {
                if (value == PRF_CLI_STOP_NTFIND)
                {
                    hrps_env.features &= ~HRPS_HR_MEAS_NTF_CFG;
                }
                else //PRF_CLI_START_NTF
                {
                    hrps_env.features |= HRPS_HR_MEAS_NTF_CFG;
                }
            }
            else
            {
                status = PRF_APP_ERROR;
            }

            if (status == PRF_ERR_OK)
            {
                //Update the attribute value
                attmdb_att_set_value(param->handle, sizeof(uint16_t), (uint8_t *)&value);
                if(param->last)
                {
                    //Inform APP of configuration change
                    struct hrps_cfg_indntf_ind * ind = KE_MSG_ALLOC(HRPS_CFG_INDNTF_IND,
                                                                    hrps_env.con_info.appid, TASK_HRPS,
                                                                    hrps_cfg_indntf_ind);

                    ind->conhdl = gapc_get_conhdl(hrps_env.con_info.conidx);
                    memcpy(&ind->cfg_val, &value, sizeof(uint16_t));

                    ke_msg_send(ind);
                }
            }
        }
        //HR Control Point Char. Value
        else
        {
            if (HRPS_IS_SUPPORTED(HRPS_ENGY_EXP_FEAT_SUP))
            {
                //Extract value
                memcpy(&value, &(param->value), sizeof(uint8_t));

                if (value == 0x1)
                {
                    //inform APP of configuration change
                    struct hrps_energy_exp_reset_ind * ind = KE_MSG_ALLOC(HRPS_ENERGY_EXP_RESET_IND,
                                                                          hrps_env.con_info.appid,
                                                                          TASK_HRPS,
                                                                          hrps_energy_exp_reset_ind);

                    ind->conhdl = gapc_get_conhdl(hrps_env.con_info.conidx);
                    ke_msg_send(ind);
                }
                else
                {
                    status = HRS_ERR_HR_CNTL_POINT_NOT_SUPPORTED;
                }
            }
            else
            {
                //Allowed to send Application Error if value inconvenient
                status = HRS_ERR_HR_CNTL_POINT_NOT_SUPPORTED;
            }
        }
    }

    //Send write response
    atts_write_rsp_send(hrps_env.con_info.conidx, param->handle, status);

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles @ref GATTC_CMP_EVT for GATTC_NOTIFY message meaning that Measurement
 * notification has been correctly sent to peer device (but not confirmed by peer device).
 * *
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
    if(param->req_type == GATTC_NOTIFY)
    {
        hrps_meas_send_cfm_send(param->status);
    }
    return (KE_MSG_CONSUMED);
}


/**
 ****************************************************************************************
 * @brief Disconnection indication to HRPS.
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
    //Check Connection Handle
    if (KE_IDX_GET(src_id) == hrps_env.con_info.conidx)
    {
        hrps_disable(param->conhdl);
    }

    return (KE_MSG_CONSUMED);
}

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// Disabled State handler definition.
const struct ke_msg_handler hrps_disabled[] =
{
    {HRPS_CREATE_DB_REQ,       (ke_msg_func_t) hrps_create_db_req_handler}
};

/// Idle State handler definition.
const struct ke_msg_handler hrps_idle[] =
{
    {HRPS_ENABLE_REQ,       (ke_msg_func_t) hrps_enable_req_handler}
};

/// Connected State handler definition.
const struct ke_msg_handler hrps_connected[] =
{
    {HRPS_MEAS_SEND_REQ,     (ke_msg_func_t) hrps_meas_send_req_handler},
    {GATTC_WRITE_CMD_IND,    (ke_msg_func_t) gattc_write_cmd_ind_handler},
    {GATTC_CMP_EVT,          (ke_msg_func_t) gattc_cmp_evt_handler},
};

/* Default State handlers definition. */
const struct ke_msg_handler hrps_default_state[] =
{
    {GAPC_DISCONNECT_IND,        (ke_msg_func_t)gapc_disconnect_ind_handler},
};

/// Specifies the message handler structure for every input state.
const struct ke_state_handler hrps_state_handler[HRPS_STATE_MAX] =
{
    [HRPS_DISABLED]       = KE_STATE_HANDLER(hrps_disabled),
    [HRPS_IDLE]           = KE_STATE_HANDLER(hrps_idle),
    [HRPS_CONNECTED]      = KE_STATE_HANDLER(hrps_connected),
};

/// Specifies the message handlers that are common to all states.
const struct ke_state_handler hrps_default_handler = KE_STATE_HANDLER(hrps_default_state);

/// Defines the place holder for the states of all the task instances.
ke_state_t hrps_state[HRPS_IDX_MAX] __attribute__((section("retention_mem_area0"),zero_init)); //@RETENTION MEMORY

#endif /* #if (BLE_HR_SENSOR) */

/// @} HRPSTASK
