/**
 ****************************************************************************************
 *
 * @file glps_task.c
 *
 * @brief Glucose Profile Sensor Task Implementation.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 * $Rev$
 *
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @addtogroup GLPSTASK
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"

#if (BLE_GL_SENSOR)

#include "gap.h"
#include "gattc_task.h"
#include "atts_util.h"
#include "attm_util.h"
#include "glps.h"
#include "glps_task.h"
#include "prf_utils.h"

/*
 *  GLUCOSE PROFILE ATTRIBUTES
 ****************************************************************************************
 */

/// Glucose Sensor Service
static const att_svc_desc_t glps_gls_svc = ATT_SVC_GLUCOSE;


/// Glucose Measurement characteristic value
static const struct att_char_desc glps_meas_char =
        ATT_CHAR(ATT_CHAR_PROP_NTF, 0,
                  ATT_CHAR_GLUCOSE_MEAS);
/// Glucose Measurement Context characteristic value
static const struct att_char_desc glps_meas_ctx_char =
        ATT_CHAR(ATT_CHAR_PROP_NTF, 0,
                  ATT_CHAR_GLUCOSE_MEAS_CTX);
/// Glucose Feature characteristic value
static const struct att_char_desc glps_feature_char =
        ATT_CHAR(ATT_CHAR_PROP_RD, 0,
                  ATT_CHAR_GLUCOSE_FEATURE);
/// Record Access Control Point characteristic value
static const struct att_char_desc glps_racp_char =
        ATT_CHAR(ATT_CHAR_PROP_WR | ATT_CHAR_PROP_IND, 0,
                  ATT_CHAR_REC_ACCESS_CTRL_PT);



/// Full GLPS Database Description - Used to add attributes into the database
static const struct attm_desc glps_att_db[GLS_IDX_NB] =
{
    // Glucose Service Declaration
    [GLS_IDX_SVC]                      =   {ATT_DECL_PRIMARY_SERVICE, PERM(RD, ENABLE), sizeof(glps_gls_svc),
                                            sizeof(glps_gls_svc), (uint8_t *)&glps_gls_svc},

    // Glucose Measurement Characteristic Declaration
    [GLS_IDX_MEAS_CHAR]                =   {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), sizeof(glps_meas_char),
                                            sizeof(glps_meas_char), (uint8_t *)&glps_meas_char},
    // Glucose Measurement Characteristic Value
    [GLS_IDX_MEAS_VAL]                 =   {ATT_CHAR_GLUCOSE_MEAS, PERM(NTF, ENABLE), GLP_MEAS_MAX_LEN,
                                            0, NULL},
    // Glucose Measurement Characteristic - Client Characteristic Configuration Descriptor
    [GLS_IDX_MEAS_NTF_CFG]             =   {ATT_DESC_CLIENT_CHAR_CFG, PERM(RD, ENABLE)|PERM(WR, ENABLE), sizeof(uint16_t),
                                            0, NULL},

    // Glucose Measurement Context Characteristic Declaration
    [GLS_IDX_MEAS_CTX_CHAR]            =   {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), sizeof(glps_meas_ctx_char),
                                            sizeof(glps_meas_ctx_char), (uint8_t *)&glps_meas_ctx_char},
    // Glucose Measurement Context Characteristic Value
    [GLS_IDX_MEAS_CTX_VAL]             =   {ATT_CHAR_GLUCOSE_MEAS_CTX, PERM(NTF, ENABLE), GLP_MEAS_CTX_MAX_LEN,
                                            0, NULL},
    // Glucose Measurement Context Characteristic - Client Characteristic Configuration Descriptor
    [GLS_IDX_MEAS_CTX_NTF_CFG]         =   {ATT_DESC_CLIENT_CHAR_CFG, PERM(RD, ENABLE)|PERM(WR, ENABLE), sizeof(uint16_t),
                                            0, NULL},

    // Glucose Features Characteristic Declaration
    [GLS_IDX_FEATURE_CHAR]             =   {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), sizeof(glps_feature_char),
                                            sizeof(glps_feature_char), (uint8_t *)&glps_feature_char},
    // Glucose Features Characteristic Value
    [GLS_IDX_FEATURE_VAL]              =   {ATT_CHAR_GLUCOSE_FEATURE, PERM(RD, ENABLE), sizeof(uint16_t),
                                            0, NULL},

    // Record Access Control Point characteristic Declaration
    [GLS_IDX_REC_ACCESS_CTRL_CHAR]     =   {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), sizeof(glps_racp_char),
                                            sizeof(glps_racp_char), (uint8_t *)&glps_racp_char},
    // Record Access Control Point characteristic Value
    [GLS_IDX_REC_ACCESS_CTRL_VAL]      =   {ATT_CHAR_REC_ACCESS_CTRL_PT, PERM(IND, ENABLE)|PERM(WR, UNAUTH), GLP_REC_ACCESS_CTRL_MAX_LEN,
                                            0, NULL},
    // Record Access Control Point characteristic - Client Characteristic Configuration Descriptor
    [GLS_IDX_REC_ACCESS_CTRL_IND_CFG]  =   {ATT_DESC_CLIENT_CHAR_CFG, PERM(RD, ENABLE)|PERM(WR, ENABLE), sizeof(uint16_t),
                                            0, NULL},
};

/*
 * LOCAL FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref GLPS_CREATE_DB_REQ message.
 * The handler adds GLS into the database using the database
 * configuration value given in param.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int glps_create_db_req_handler(ke_msg_id_t const msgid,
                                      struct glps_create_db_req const *param,
                                      ke_task_id_t const dest_id,
                                      ke_task_id_t const src_id)
{
    //Service Configuration Flag
    uint16_t cfg_flag = GLPS_MANDATORY_MASK;
    //Database Creation Status
    uint8_t status;

    //Save Profile ID
    glps_env.con_info.prf_id = TASK_GLPS;

    //Save Database Configuration
    if(param->meas_ctx_supported)
    {
        GLPS_SET(MEAS_CTX_SUPPORTED);
    }

    // set start handle or automatically set it when creating database (start_hdl = 0)
    glps_env.shdl=param->start_hdl;

    /*---------------------------------------------------*
     * Glucose Service Creation
     *---------------------------------------------------*/
    //Set Configuration Flag Value
    if (GLPS_IS(MEAS_CTX_SUPPORTED))
    {
        cfg_flag |= GLPS_MEAS_CTX_PRES_MASK;
    }

    //Add Service Into Database
    status = attm_svc_create_db(&glps_env.shdl, (uint8_t *)&cfg_flag, GLS_IDX_NB, NULL,
                               dest_id, &glps_att_db[0]);
    //Disable GLS
    attmdb_svc_set_permission(glps_env.shdl, PERM(SVC, DISABLE));

    //Go to Idle State
    if (status == ATT_ERR_NO_ERROR)
    {
        //If we are here, database has been fulfilled with success, go to idle test
        ke_state_set(TASK_GLPS, GLPS_IDLE);
    }

    //Send response to application
    struct glps_create_db_cfm * cfm = KE_MSG_ALLOC(GLPS_CREATE_DB_CFM, src_id,
                                                   TASK_GLPS, glps_create_db_cfm);
    cfm->status = status;
    ke_msg_send(cfm);

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref GLPS_ENABLE_REQ message.
 * The handler enables the Glucose Sensor Profile and initialize readable values.
 * @param[in] msgid Id of the message received (probably unused).off
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int glps_enable_req_handler(ke_msg_id_t const msgid,
                                   struct glps_enable_req const *param,
                                   ke_task_id_t const dest_id,
                                   ke_task_id_t const src_id)
{
    uint8_t status = PRF_ERR_REQ_DISALLOWED;

    if(!GLPS_IS(ENABLE))
    {
        // Check if the provided connection exist
        if (gapc_get_conidx(param->conhdl) != GAP_INVALID_CONIDX)
        {
            status = PRF_ERR_OK;

            GLPS_SET(ENABLE);

            //Value used to initialize all readable value in DB
            uint16_t indntf_cfg = PRF_CLI_STOP_NTFIND;

            // Save the application task id
            glps_env.con_info.appid = src_id;
            // Save the connection handle associated to the profile
            glps_env.con_info.conidx = gapc_get_conidx(param->conhdl);

            // No RACP at service start.
            GLPS_CLEAR(RACP_ON_GOING);
            GLPS_CLEAR(SENDING_MEAS);

            // Configure Glucose Measuremment IND Cfg in DB
            if(param->con_type == PRF_CON_NORMAL)
            {
                glps_env.evt_cfg = param->evt_cfg;
            }
            else
            {
                glps_env.evt_cfg = 0;
            }

            if((glps_env.evt_cfg & GLPS_MEAS_NTF_CFG) != 0)
            {
                indntf_cfg = PRF_CLI_START_NTF;
            }

            //Set Glucose measurement notification configuration
            attmdb_att_set_value(GLPS_HANDLE(GLS_IDX_MEAS_NTF_CFG), sizeof(uint16_t),
                                 (uint8_t *)&indntf_cfg);

            // Configure Intermediate Cuff Pressure NTF Cfg in DB

            //Configure Glucose measurement context notification
            if (GLPS_IS(MEAS_CTX_SUPPORTED))
            {
                indntf_cfg = PRF_CLI_STOP_NTFIND;

                if((glps_env.evt_cfg & GLPS_MEAS_CTX_NTF_CFG) != 0)
                {
                    indntf_cfg = PRF_CLI_START_NTF;
                }

                //Set Glucose measurement context notification configuration
                attmdb_att_set_value(GLPS_HANDLE(GLS_IDX_MEAS_CTX_NTF_CFG), sizeof(uint16_t),
                                     (uint8_t *)&indntf_cfg);
            }

            indntf_cfg = PRF_CLI_STOP_NTFIND;

            if((glps_env.evt_cfg & GLPS_RACP_IND_CFG) != 0)
            {
                indntf_cfg = PRF_CLI_START_IND;
            }

            //Set record access control point indication configuration
            attmdb_att_set_value(GLPS_HANDLE(GLS_IDX_REC_ACCESS_CTRL_IND_CFG), sizeof(uint16_t),
                                 (uint8_t *)&indntf_cfg);


            //Set Glucose sensor features
            attmdb_att_set_value(GLPS_HANDLE(GLS_IDX_FEATURE_VAL), sizeof(uint16_t),
                                 (uint8_t *)&param->features);

            // Enable Service + Set Security Level
            attmdb_svc_set_permission(glps_env.shdl, param->sec_lvl);

            // Go to connected state
            ke_state_set(TASK_GLPS, GLPS_CONNECTED);
        }
    }

    // send completed information to APP task that contains error status
    struct glps_enable_cfm * cmp_evt = KE_MSG_ALLOC(GLPS_ENABLE_CFM, src_id,
                                                    TASK_GLPS, glps_enable_cfm);

    cmp_evt->conhdl     = param->conhdl;
    cmp_evt->status     = status;

    ke_msg_send(cmp_evt);

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref GLPS_MEAS_SEND_REQ message.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int glps_meas_send_req_handler(ke_msg_id_t const msgid,
                                      struct glps_send_meas_with_ctx_req const *param,
                                      ke_task_id_t const dest_id,
                                      ke_task_id_t const src_id)
{
    uint8_t status = PRF_ERR_OK;

    if(param->conhdl == gapc_get_conhdl(glps_env.con_info.conidx))
    {
        // device already sending a measurement
        if(GLPS_IS(SENDING_MEAS))
        {
            //Cannot send another measurement in parallel
            status = (PRF_ERR_REQ_DISALLOWED);
        }
        else
        {
            // inform that device is sending a measurement
            GLPS_SET(SENDING_MEAS);


            // check if context is supported
            if((msgid == GLPS_SEND_MEAS_WITH_CTX_REQ) && !(GLPS_IS(MEAS_CTX_SUPPORTED)))
            {
                // Context not supported
                status = (PRF_ERR_FEATURE_NOT_SUPPORTED);
            }
            // check if notifications enabled
            else if(((glps_env.evt_cfg & GLPS_MEAS_NTF_CFG) == 0)
                    || (((glps_env.evt_cfg & GLPS_MEAS_CTX_NTF_CFG) == 0)
                        && (msgid == GLPS_SEND_MEAS_WITH_CTX_REQ)))
            {
                // Not allowed to send measurement if Notifications not enabled.
                status = (PRF_ERR_NTF_DISABLED);

            }
            else
            {
                struct attm_elmt * attm_elmt;

                // retrieve value pointer in database
                attm_elmt = attmdb_get_attribute(GLPS_HANDLE(GLS_IDX_MEAS_VAL));

                // pack measured value in database
                attm_elmt->length = glps_pack_meas_value(attm_elmt->value, &(param->meas),
                        param->seq_num);

                prf_server_send_event((prf_env_struct *)&glps_env, false,
                        GLPS_HANDLE(GLS_IDX_MEAS_VAL));

                // Measurement value notification not yet sent
                GLPS_CLEAR(MEAS_SENT);

                if(msgid == GLPS_SEND_MEAS_WITH_CTX_REQ)
                {
                    // 2 notification complete messages expected
                    GLPS_SET(MEAS_CTX_SENT);

                    // retrieve value pointer in database
                    attm_elmt = attmdb_get_attribute(GLPS_HANDLE(GLS_IDX_MEAS_CTX_VAL));

                    // pack measured value in database
                    attm_elmt->length = glps_pack_meas_ctx_value(attm_elmt->value,
                                                            &(param->ctx), param->seq_num);

                    prf_server_send_event((prf_env_struct *)&glps_env, false,
                            GLPS_HANDLE(GLS_IDX_MEAS_CTX_VAL));
                }
                else
                {
                    // 1 notification complete messages expected
                    GLPS_CLEAR(MEAS_CTX_SENT);
                }
            }
        }
    }
    else
    {
        //Wrong Connection Handle
        status = (PRF_ERR_INVALID_PARAM);
    }

    // send command complete if an error occurs
    if(status != PRF_ERR_OK)
    {
        // allow to send other measurements
        GLPS_CLEAR(SENDING_MEAS);

        // send completed information to APP task that contains error status
        struct glps_req_cmp_evt * cmp_evt = KE_MSG_ALLOC(GLPS_REQ_CMP_EVT, glps_env.con_info.appid,
                TASK_GLPS, glps_req_cmp_evt);

        cmp_evt->conhdl     = param->conhdl;
        cmp_evt->request    = GLPS_SEND_MEAS_REQ_NTF_CMP;
        cmp_evt->status     = status;

        ke_msg_send(cmp_evt);
    }

    return (KE_MSG_CONSUMED);
}


/**
 ****************************************************************************************
 * @brief Handles reception of the @ref GLPS_RACP_RSP_REQ message.
 * Send when a RACP requests is finished
 *
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int glps_racp_rsp_req_handler(ke_msg_id_t const msgid,
                                     struct glps_racp_rsp_req const *param,
                                     ke_task_id_t const dest_id,
                                     ke_task_id_t const src_id)
{
    struct glp_racp_rsp racp_rsp;
    // Status
    uint8_t status = PRF_ERR_OK;

    // check connection handle
    if(param->conhdl == gapc_get_conhdl(glps_env.con_info.conidx))
    {
        // check if op code valid
        if((param->op_code < GLP_REQ_REP_STRD_RECS)
            || (param->op_code > GLP_REQ_REP_NUM_OF_STRD_RECS))
        {
            //Wrong op code
            status = PRF_ERR_INVALID_PARAM;
        }
        // check if RACP on going
        else if((param->op_code != GLP_REQ_ABORT_OP) && !(GLPS_IS(RACP_ON_GOING)))
        {
            //Cannot send response since no RACP on going
            status = PRF_ERR_REQ_DISALLOWED;
        }
        else
        {
            // Number of stored record calculation succeed.
            if((param->op_code == GLP_REQ_REP_NUM_OF_STRD_RECS)
                    && ( param->status == GLP_RSP_SUCCESS))
            {
                racp_rsp.op_code = GLP_REQ_NUM_OF_STRD_RECS_RSP;
                racp_rsp.operand.num_of_record = param->num_of_record;
            }
            // Send back status information
            else
            {
                racp_rsp.op_code = GLP_REQ_RSP_CODE;
                racp_rsp.operand.rsp.op_code_req = param->op_code;
                racp_rsp.operand.rsp.status = param->status;
            }

            // There is no more RACP on going
            GLPS_CLEAR(RACP_ON_GOING);

            // send RACP indication
            glps_send_racp_rsp(&(racp_rsp), glps_env.con_info.appid);
        }
    }
    else
    {
        //Wrong Connection Handle
        status = PRF_ERR_INVALID_PARAM;
    }

    if (status != PRF_ERR_OK)
    {
        prf_server_error_ind_send((prf_env_struct *)&glps_env, status,
                                  GLPS_ERROR_IND, GLPS_RACP_RSP_REQ);
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
    uint8_t status = ATT_ERR_INSUFF_AUTHOR;

    // check connection
    if (KE_IDX_GET(src_id) == glps_env.con_info.conidx)
    {
        uint8_t att_idx = GLPS_IDX(param->handle);
        status = PRF_ERR_OK;

        // check if it's a client configuration char
        if(glps_att_db[att_idx].uuid == ATT_DESC_CLIENT_CHAR_CFG)
        {
            uint16_t cli_cfg;
            uint8_t evt_mask = GLPS_IND_NTF_EVT(att_idx);

            // get client configuration
            cli_cfg = co_read16(&(param->value));

            // stop indication/notification
            if(cli_cfg == PRF_CLI_STOP_NTFIND)
            {
                glps_env.evt_cfg &= ~evt_mask;
            }
            // start indication/notification (check that char value accept it)
            else if((((glps_att_db[att_idx-1].perm & PERM(IND, ENABLE)) != 0)
                && cli_cfg == PRF_CLI_START_IND)
               ||(((glps_att_db[att_idx-1].perm & PERM(NTF, ENABLE)) != 0)
                       && cli_cfg == PRF_CLI_START_NTF))
            {
                glps_env.evt_cfg |= evt_mask;
            }
            // improper value
            else
            {
                status = GLP_ERR_IMPROPER_CLI_CHAR_CFG;
            }

            if (status == PRF_ERR_OK)
            {
                //Inform APP of configuration change
                struct glps_cfg_indntf_ind * ind = KE_MSG_ALLOC(GLPS_CFG_INDNTF_IND,
                        glps_env.con_info.appid, TASK_GLPS,
                        glps_cfg_indntf_ind);

                //Update the attribute value
                attmdb_att_set_value(param->handle, sizeof(uint16_t), (uint8_t *)&cli_cfg);
                ind->conhdl = gapc_get_conhdl(glps_env.con_info.conidx);
                ind->evt_cfg = glps_env.evt_cfg;

                ke_msg_send(ind);
            }
        }

        else if (glps_att_db[att_idx].uuid == ATT_CHAR_REC_ACCESS_CTRL_PT)
        {
            uint8_t reqstatus;
            uint8_t* value;
            uint16_t length;
            struct glp_racp_req racp_req;

            if((glps_env.evt_cfg & GLPS_RACP_IND_CFG) == 0)
            {
                // do nothing since indication not enabled for this characteristic
                status = GLP_ERR_IMPROPER_CLI_CHAR_CFG;
            }
            // If a request is on going
            else if(GLPS_IS(RACP_ON_GOING))
            {
                // if it's an abort command, execute it.
                if((param->offset == 0) && (param->value[0] == GLP_REQ_ABORT_OP))
                {
                    //forward abort operation to application
                    struct glps_racp_req_ind * req = KE_MSG_ALLOC(GLPS_RACP_REQ_IND,
                            glps_env.con_info.appid, TASK_GLPS,
                            glps_racp_req_ind);

                    req->conhdl = gapc_get_conhdl(glps_env.con_info.conidx);
                    req->racp_req.op_code = GLP_REQ_ABORT_OP;
                    req->racp_req.filter.operator = 0;

                    ke_msg_send(req);
                }
                else
                {
                    // do nothing since a procedure already in progress
                    status = GLP_ERR_PROC_ALREADY_IN_PROGRESS;
                }
            }
            else
            {
                // Update the attribute value (note several write could be required since
                // attribute length > (ATT_MTU-3)
                attmdb_att_update_value(param->handle, param->length, param->offset,
                        (uint8_t*)&(param->value[0]));

                // retrieve full data.
                attmdb_att_get_value(param->handle, &length, &value);

                // unpack record access control point value
                reqstatus = glps_unpack_racp_req(value, length, &racp_req);

                // check unpacked status
                switch(reqstatus)
                {
                    case PRF_APP_ERROR:
                    {
                        /* Do nothing, ignore request since it's not complete and maybe
                         * requires several peer write to be performed.
                         */
                    }
                    break;
                    case PRF_ERR_OK:
                    {
                        // check wich request shall be send to api task
                        switch(racp_req.op_code)
                        {
                            case GLP_REQ_REP_STRD_RECS:
                            case GLP_REQ_DEL_STRD_RECS:
                            case GLP_REQ_REP_NUM_OF_STRD_RECS:
                            {
                                //forward request operation to application
                                struct glps_racp_req_ind * req = KE_MSG_ALLOC(GLPS_RACP_REQ_IND,
                                        glps_env.con_info.appid, TASK_GLPS,
                                        glps_racp_req_ind);

                                // RACP on going.
                                GLPS_SET(RACP_ON_GOING);

                                req->conhdl = gapc_get_conhdl(glps_env.con_info.conidx);
                                req->racp_req = racp_req;

                                ke_msg_send(req);
                            }
                            break;
                            case GLP_REQ_ABORT_OP:
                            {
                                // nothing to abort, send an error message.
                                struct glp_racp_rsp racp_rsp;

                                racp_rsp.op_code = GLP_REQ_RSP_CODE;
                                racp_rsp.operand.rsp.op_code_req =
                                        racp_req.op_code;
                                racp_rsp.operand.rsp.status = GLP_RSP_ABORT_UNSUCCESSFUL;

                                // send record access control response indication
                                glps_send_racp_rsp(&(racp_rsp),
                                        TASK_GLPS);
                            }
                            break;
                            default:
                            {
                                // nothing to do since it's handled during unpack
                            }
                            break;
                        }
                    }
                    break;
                    default:
                    {
                        /* There is an error in record access control request, inform peer
                         * device that request is incorrect. */
                        struct glp_racp_rsp racp_rsp;

                        racp_rsp.op_code = GLP_REQ_RSP_CODE;
                        racp_rsp.operand.rsp.op_code_req = racp_req.op_code;
                        racp_rsp.operand.rsp.status = reqstatus;

                        // send record access control response indication
                        glps_send_racp_rsp(&(racp_rsp),
                                TASK_GLPS);
                    }
                    break;
                }
            }
        }
        // not expected request
        else
        {
            status = ATT_ERR_WRITE_NOT_PERMITTED;
        }
    }

    if(param->response)
    {
        //Send write response
        atts_write_rsp_send(glps_env.con_info.conidx, param->handle, status);
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles @ref GATTC_CMP_EVT for GATTC_NOTIFY and GATT_INDICATE message meaning
 * that Measurement notification/indication has been correctly sent to peer device
 *
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance
 * @param[in] src_id    ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gattc_cmp_evt_handler(ke_msg_id_t const msgid, struct gattc_cmp_evt const *param,
                                 ke_task_id_t const dest_id, ke_task_id_t const src_id)
{
    switch(param->req_type)
    {
        case GATTC_NOTIFY:
        {
            /* send message indication if an error occurs,
             * or if all notification complete event has been received
             */
            if((param->status != PRF_ERR_OK)
                    || (!(GLPS_IS(MEAS_CTX_SENT)))
                    || (GLPS_IS(MEAS_CTX_SENT) && (GLPS_IS(MEAS_SENT))))
            {
                GLPS_CLEAR(SENDING_MEAS);

                // send completed information to APP task
                struct glps_req_cmp_evt * cmp_evt = KE_MSG_ALLOC(GLPS_REQ_CMP_EVT, glps_env.con_info.appid,
                        TASK_GLPS, glps_req_cmp_evt);

                cmp_evt->conhdl     = gapc_get_conhdl(glps_env.con_info.conidx);
                cmp_evt->request    = GLPS_SEND_MEAS_REQ_NTF_CMP;
                cmp_evt->status     = param->status;

                ke_msg_send(cmp_evt);
            }
            else
            {
                // Measurement value notification sent
                GLPS_SET(MEAS_SENT);
            }
        }
        break;
        case GATTC_INDICATE:
        {
            // verify if indication should be conveyed to application task
            if(glps_env.racp_ind_src == glps_env.con_info.appid)
            {
                // send completed information to APP task
                struct glps_req_cmp_evt * cmp_evt = KE_MSG_ALLOC(GLPS_REQ_CMP_EVT, glps_env.con_info.appid,
                        TASK_GLPS, glps_req_cmp_evt);

                cmp_evt->conhdl     = gapc_get_conhdl(glps_env.con_info.conidx);
                cmp_evt->request    = GLPS_SEND_RACP_RSP_IND_CMP;
                cmp_evt->status     = param->status;

                ke_msg_send(cmp_evt);
            }
        }
        break;

        default: break;
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Disconnection indication to GLPS.
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
    if (KE_IDX_GET(src_id) == glps_env.con_info.conidx)
    {
        glps_disable(PRF_ERR_DISCONNECTED, param->conhdl);
    }

    return (KE_MSG_CONSUMED);
}

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// Disabled State handler definition.
const struct ke_msg_handler glps_disabled[] =
{
    {GLPS_CREATE_DB_REQ,                  (ke_msg_func_t) glps_create_db_req_handler}
};

/// Idle State handler definition.
const struct ke_msg_handler glps_idle[] =
{
    {GLPS_ENABLE_REQ,                     (ke_msg_func_t) glps_enable_req_handler}
};

/// Connected State handler definition.
const struct ke_msg_handler glps_connected[] =
{
    {GATTC_WRITE_CMD_IND,                 (ke_msg_func_t) gattc_write_cmd_ind_handler},
    {GLPS_SEND_MEAS_WITH_CTX_REQ,         (ke_msg_func_t) glps_meas_send_req_handler},
    {GLPS_SEND_MEAS_WITHOUT_CTX_REQ,      (ke_msg_func_t) glps_meas_send_req_handler},
    {GLPS_RACP_RSP_REQ,                   (ke_msg_func_t) glps_racp_rsp_req_handler},
    {GATTC_CMP_EVT,                       (ke_msg_func_t) gattc_cmp_evt_handler},
};

/// Default State handlers definition
const struct ke_msg_handler glps_default_state[] =
{
    {GAPC_DISCONNECT_IND,        (ke_msg_func_t)gapc_disconnect_ind_handler},
};

/// Specifies the message handler structure for every input state.
const struct ke_state_handler glps_state_handler[GLPS_STATE_MAX] =
{
    [GLPS_DISABLED]       = KE_STATE_HANDLER(glps_disabled),
    [GLPS_IDLE]           = KE_STATE_HANDLER(glps_idle),
    [GLPS_CONNECTED]      = KE_STATE_HANDLER(glps_connected),
};

/// Specifies the message handlers that are common to all states.
const struct ke_state_handler glps_default_handler = KE_STATE_HANDLER(glps_default_state);

/// Defines the place holder for the states of all the task instances.
ke_state_t glps_state[GLPS_IDX_MAX] __attribute__((section("retention_mem_area0"),zero_init)); //@RETENTION MEMORY

#endif /* #if (BLE_GL_SENSOR) */

/// @} GLPSTASK
