 /**
 ****************************************************************************************
 *
 * @file bcss_task.c
 *
 * @brief Header file - Body Composition Service Server Role Task Implementation.
 *
 * Copyright (C) 2015 Dialog Semiconductor Ltd, unpublished work. This computer
 * program includes Confidential, Proprietary Information and is a Trade Secret of
 * Dialog Semiconductor Ltd. All use, disclosure, and/or reproduction is prohibited
 * unless authorized in writing. All Rights Reserved.
 *
 * <bluetooth.support@diasemi.com> and contributors.
 *
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @addtogroup BCSSTASK
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"

#if (BLE_BCS_SERVER)

#include "ke_mem.h"
#include "gap.h"
#include "gattc.h"
#include "gattc_task.h"
#include "attm_util.h"
#include "atts_util.h"

#include "bcss.h"
#include "bcss_task.h"

#include "prf_utils.h"

/*
 * LOCAL FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref BCSS_CREATE_DB_REQ message.
 * The handler adds BCSS into the database
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int bcss_create_db_req_handler(ke_msg_id_t const msgid,
                                    struct bcss_create_db_req const *param,
                                    ke_task_id_t const dest_id,
                                    ke_task_id_t const src_id)
{
    // Database Creation Status
    uint8_t status;
    struct bcss_create_db_cfm *cfm;

    // Save profile id
    bcss_env.con_info.prf_id = TASK_BCSS;

    // In case of instantiating as a secondary service
    if (!param->is_primary)
        bcss_att_db[BCSS_IDX_SVC].uuid = ATT_DECL_SECONDARY_SERVICE;

    // Add Service Into Database
    status = attm_svc_create_db(&bcss_env.shdl, NULL, BCSS_IDX_NB, NULL,
                                                    dest_id, &bcss_att_db[0]);

    // Disable BCSS
    attmdb_svc_set_permission(bcss_env.shdl, PERM(SVC, DISABLE));

    // If we are here, database has been fulfilled with success, go to idle
    if (status == ATT_ERR_NO_ERROR)
        ke_state_set(TASK_BCSS, BCSS_IDLE);

    // Send response to application
    cfm = KE_MSG_ALLOC(BCSS_CREATE_DB_CFM, src_id, TASK_BCSS,
                                                            bcss_create_db_cfm);
    cfm->status = status;
    cfm->shdl = bcss_env.shdl;
    cfm->ehdl = bcss_env.shdl + BCSS_IDX_NB - 1;

    ke_msg_send(cfm);

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref BCSS_ENABLE_REQ message.
 * The handler enables the BCSS for the given connection and sets the CCC
 * descriptor value to the one stored by the app
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int bcss_enable_req_handler(ke_msg_id_t const msgid,
                                    struct bcss_enable_req const *param,
                                    ke_task_id_t const dest_id,
                                    ke_task_id_t const src_id)
{
    uint32_t feat = param->features;
    uint16_t ind_cfg = PRF_CLI_STOP_NTFIND;

    // Save the application task id
    bcss_env.con_info.appid = src_id;
    // Save the connection handle associated to the profile
    bcss_env.con_info.conidx = gapc_get_conidx(param->conhdl);

    // Store supported features
    bcss_env.features = param->features;

    // Check if the provided connection exist
    if (bcss_env.con_info.conidx == GAP_INVALID_CONIDX)
    {
        // The connection doesn't exist, request disallowed
        prf_server_error_ind_send((prf_env_struct *)&bcss_env,
                                    PRF_ERR_REQ_DISALLOWED, BCSS_ERROR_IND,
                                    BCSS_ENABLE_REQ);

        return (KE_MSG_CONSUMED);
    }

    // Set Body Composition Feature Value in database - Not supposed to change during connection
    attmdb_att_set_value(bcss_env.shdl + BCSS_IDX_FEAT_VAL, sizeof(feat),
                                                        (uint8_t *)&feat);

    // Restore Measurements Indication configuration
    if ((param->con_type == PRF_CON_NORMAL) &&
                                    (param->meas_ind_en == PRF_CLI_START_IND))
        ind_cfg = param->meas_ind_en;

    attmdb_att_set_value(bcss_env.shdl + BCSS_IDX_MEAS_IND_CFG,
                                                    sizeof(param->meas_ind_en),
                                                    (uint8_t *) &ind_cfg);

    // Enable Service + Set Security Level
    attmdb_svc_set_permission(bcss_env.shdl, param->sec_lvl);

    // Go to connected state
    ke_state_set(TASK_BCSS, BCSS_CONNECTED);

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref BCSS_MEAS_VAL_IND_REQ message.
 * The handler triggers the Measurement Value indication.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int bcss_meas_val_ind_req_handler(ke_msg_id_t const msgid,
                                    struct bcss_meas_val_ind_req const *param,
                                    ke_task_id_t const dest_id,
                                    ke_task_id_t const src_id)
{
    uint16_t *ind_cfg;
    uint16_t len;
    uint8_t status;
    uint16_t mtu;

    if (bcss_env.meas)
    {
        bcss_ind_cfm_send(PRF_ERR_REQ_DISALLOWED);
        return (KE_MSG_CONSUMED);
    }

    status = attmdb_att_get_value(bcss_env.shdl + BCSS_IDX_MEAS_IND_CFG, &(len),
                                                        (uint8_t **)&ind_cfg);

    mtu = gattc_get_mtu(gapc_get_conidx(param->conhdl));

    if (status != ATT_ERR_NO_ERROR)
    {
        bcss_ind_cfm_send(status);
        return (KE_MSG_CONSUMED);
    }

    switch (*ind_cfg)
    {
        case PRF_CLI_START_IND:
            if (gapc_get_conidx(param->conhdl) == bcss_env.con_info.conidx)
            {
                bcss_indicate(&param->meas, bcss_env.features, mtu);
                break;
            }
        case PRF_CLI_START_NTF:
        default:
            status = PRF_ERR_IND_DISABLED;
            bcss_ind_cfm_send(status);
            break;
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref GATTC_WRITE_CMD_IND message.
 * The handler stores the Client Characteristic Descriptor value and also stores
 * this value in the environment variable for quicker access.
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
    uint8_t status;
    uint16_t ind_cfg;

    // Handle only CCC descriptor write
    if (param->handle != bcss_env.shdl + BCSS_IDX_MEAS_IND_CFG)
    {
        status = PRF_APP_ERROR;
        goto done;
    }

    ind_cfg = co_read16p(&param->value[0]);

    // Only update configuration if value is for stop or enable indication
    if ((ind_cfg != PRF_CLI_STOP_NTFIND) && (ind_cfg != PRF_CLI_START_IND))
    {
        status = PRF_APP_ERROR;
        goto done;
    }

    // Set CCC Descriptor value in the DB
    attmdb_att_set_value(bcss_env.shdl + BCSS_IDX_MEAS_IND_CFG, sizeof(ind_cfg),
                                                        (uint8_t *)&ind_cfg);

    // Notify if its the last write in case of prepare writes
    if (param->last)
    {
        // Inform the APP of configuration change
        struct bcss_meas_val_ind_cfg_ind *ind =
                            KE_MSG_ALLOC(BCSS_MEAS_VAL_IND_CFG_IND,
                                            bcss_env.con_info.appid, TASK_BCSS,
                                            bcss_meas_val_ind_cfg_ind);

        ind->conhdl = gapc_get_conhdl(bcss_env.con_info.conidx);
        ind->ind_cfg = ind_cfg;

        ke_msg_send(ind);
    }

    status = PRF_ERR_OK;

done:
    // Send write response
    atts_write_rsp_send(bcss_env.con_info.conidx, param->handle, status);

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref GATTC_CMP_EVT message.
 * The handler notifies the app about the indication status.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gattc_cmp_evt_handler(ke_msg_id_t const msgid,
                                    struct gattc_cmp_evt const *param,
                                    ke_task_id_t const dest_id,
                                    ke_task_id_t const src_id)
{
    switch(param->req_type)
    {
        case GATTC_NOTIFY:
        case GATTC_INDICATE:
            // Indication continuation
            if (bcss_env.ind_cont_feat)
            {
                uint16_t mtu = gattc_get_mtu(bcss_env.con_info.conidx);

                bcss_indicate(bcss_env.meas, bcss_env.ind_cont_feat, mtu);
            }
            // Done with multiple indications
            else
            {
                if (bcss_env.meas)
                {
                    ke_free(bcss_env.meas);
                    bcss_env.meas = NULL;
                }

                bcss_ind_cfm_send(param->status);
            }
            break;
        default:
            break;
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref GAPC_DISCONNECT_IND message.
 * The handler notifies the app about the disconnection. Indication configuration
 * is sent to the app for storing.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gapc_disconnect_ind_handler(ke_msg_id_t const msgid,
                                        struct gapc_disconnect_ind const *param,
                                        ke_task_id_t const dest_id,
                                        ke_task_id_t const src_id)
{
    // Clear pending indication
    if (bcss_env.meas)
    {
        ke_free(bcss_env.meas);
        bcss_env.meas = NULL;

        bcss_env.ind_cont_feat = 0;

        bcss_ind_cfm_send(PRF_ERR_DISCONNECTED);
    }

    // Check Connection Handle
    if (KE_IDX_GET(src_id) == bcss_env.con_info.conidx)
        bcss_disable(param->conhdl);

    return (KE_MSG_CONSUMED);
}

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// Disabled State handler definition.
const struct ke_msg_handler bcss_disabled[] =
{
    {BCSS_CREATE_DB_REQ,        (ke_msg_func_t) bcss_create_db_req_handler},
};

/// Idle State handler definition.
const struct ke_msg_handler bcss_idle[] =
{
    {BCSS_ENABLE_REQ,           (ke_msg_func_t) bcss_enable_req_handler},
};

/// Connected State handler definition.
const struct ke_msg_handler bcss_connected[] =
{
    {BCSS_MEAS_VAL_IND_REQ,     (ke_msg_func_t) bcss_meas_val_ind_req_handler},
    {GATTC_WRITE_CMD_IND,       (ke_msg_func_t) gattc_write_cmd_ind_handler},
    {GATTC_CMP_EVT,             (ke_msg_func_t) gattc_cmp_evt_handler},
};

/// Default State handlers definition
const struct ke_msg_handler bcss_default_state[] =
{
    {GAPC_DISCONNECT_IND,       (ke_msg_func_t)gapc_disconnect_ind_handler},
};

/// Specifies the message handler structure for every input state.
const struct ke_state_handler bcss_state_handler[BCSS_STATE_MAX] =
{
    [BCSS_DISABLED]       = KE_STATE_HANDLER(bcss_disabled),
    [BCSS_IDLE]           = KE_STATE_HANDLER(bcss_idle),
    [BCSS_CONNECTED]      = KE_STATE_HANDLER(bcss_connected),
};

/// Specifies the message handlers that are common to all states.
const struct ke_state_handler bcss_default_handler = KE_STATE_HANDLER(bcss_default_state);

/// Defines the place holder for the states of all the task instances.
ke_state_t bcss_state[BCSS_IDX_MAX] __attribute__((section("retention_mem_area0"),zero_init)); //@RETENTION MEMORY

#endif /* #if (BLE_BCS_SERVER) */

/// @} BCSSTASK
