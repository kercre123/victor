/**
 ****************************************************************************************
 *
 * @file bmss_task.c
 *
 * @brief C file - Bond Management Service Server Role Task Implementation.
 *
 * Copyright (C) 2015. Dialog Semiconductor Ltd, unpublished work. This computer
 * program includes Confidential, Proprietary Information and is a Trade Secret of
 * Dialog Semiconductor Ltd. All use, disclosure, and/or reproduction is prohibited
 * unless authorized in writing. All Rights Reserved.
 *
 * <bluetooth.support@diasemi.com> and contributors.
 *
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"

#if (BLE_BM_SERVER)
#include "gap.h"
#include "gapc.h"
#include "gattc_task.h"
#include "atts_util.h"
#include "bmss.h"
#include "bmss_task.h"
#include "attm_cfg.h"
#include "attm_db.h"
#include "prf_utils.h"
#include "attm_util.h"

/*
 * DEFINES
 ****************************************************************************************
 */
#define BMS_FEAT_CROP_MASK  (0xFF000000)
#define BMS_ALL_FEAT_MASK   (BMS_FEAT_DEL_BOND | \
                                BMS_FEAT_DEL_BOND_AUTHZ | \
                                BMS_FEAT_DEL_ALL_BOND | \
                                BMS_FEAT_DEL_ALL_BOND_AUTHZ | \
                                BMS_FEAT_DEL_ALL_BOND_BUT_PEER | \
                                BMS_FEAT_DEL_ALL_BOND_BUT_PEER_AUTHZ)

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref BMSS_CREATE_DB_REQ message.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
  */
static int bmss_create_db_req_handler(ke_msg_id_t const msgid,
                                      struct bmss_create_db_req const *param,
                                      ke_task_id_t const dest_id,
                                      ke_task_id_t const src_id)
{
    //Database Creation Status
    uint8_t status;

    /*
    att_size_t   *p_bms_auth_code_size = (att_size_t *)&bms_att_db[BMS_IDX_BM_CTRL_PT_VAL].max_length;
    *p_bms_auth_code_size = param->authz_code_len;
    */

    // Save Profile ID
    bmss_env.con_info.prf_id = TASK_BMSS;
    // Save Database Configuration
    bmss_env.features = param->features;
    bmss_env.reliable_writes = param->reliable_writes;

    // Create BMS in the DB
    status = attm_svc_create_db(&bmss_env.shdl, NULL, BMS_IDX_NB, NULL,
                                dest_id, &bms_att_db[0]);

    if (status == ATT_ERR_NO_ERROR)
    {
        uint16_t value = param->reliable_writes ? ATT_EXT_RELIABLE_WRITE : 0;
        att_size_t bms_feat_len = 1;
        uint8_t index = 0;
        uint32_t features;

        // Set extended properties in DB
        attmdb_att_set_value(bmss_env.shdl + BMS_IDX_BM_CTRL_PT_EXT_PROP, sizeof(uint16_t),
                             (uint8_t *)&value);

        // Prepare value for write and filter out unsupported feature bits
        co_write32p(&features, (param->features & BMS_ALL_FEAT_MASK));

        if (features != 0)
        {
            // Check how many feature octets should be save in DB
            for (index = 0; index < BMS_FEAT_MAX_LEN; index ++)
            {
                if (features & (BMS_FEAT_CROP_MASK >> (8 * index)))
                {
                    bms_feat_len = BMS_FEAT_MAX_LEN - index;
                    break;
                }
            }

            // Set Features in DB
            status = attmdb_att_set_value(bmss_env.shdl + BMS_IDX_BM_FEAT_VAL,
                                            bms_feat_len, (uint8_t *)&features);
        }
        else
        {
            status = PRF_ERR_INVALID_PARAM;
        }
    }

    // Disable BMS
    attmdb_svc_set_permission(bmss_env.shdl, PERM(SVC, DISABLE));

    // Go to Idle State
    if (status == ATT_ERR_NO_ERROR)
    {
        // If we are here, database has been fulfilled with success, go to idle state
        ke_state_set(TASK_BMSS, BMSS_IDLE);
    }

    // Send confirmation to application
    struct bmss_create_db_cfm *cfm = KE_MSG_ALLOC(BMSS_CREATE_DB_CFM,
                                                  src_id,
                                                  TASK_BMSS,
                                                  bmss_create_db_cfm);

    cfm->status = status;
    ke_msg_send(cfm);

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref BMSS_ENABLE_REQ message.
 * @param[in] msgid     Id of the message received (probably unused).
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (probably unused).
 * @param[in] src_id    ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int bmss_enable_req_handler(ke_msg_id_t const msgid,
                                    struct bmss_enable_req const *param,
                                    ke_task_id_t const dest_id,
                                    ke_task_id_t const src_id)
{
    // Save the application task id
    bmss_env.con_info.appid = src_id;
    // Save the connection handle associated to the profile
    bmss_env.con_info.conidx = gapc_get_conidx(param->conhdl);

    // Check if the provided connection exist
    if (bmss_env.con_info.conidx == GAP_INVALID_CONIDX)
    {
        // The connection doesn't exist, request disallowed
        prf_server_error_ind_send((prf_env_struct *)&bmss_env, PRF_ERR_REQ_DISALLOWED,
                                   BMSS_ERROR_IND, BMSS_ENABLE_REQ);
    }
    else
    {
        // Enable Service + Set Security Level
        attmdb_svc_set_permission(bmss_env.shdl, param->sec_lvl);

        // Go to Connected state
        ke_state_set(TASK_BMSS, BMSS_CONNECTED);
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref GATTC_WRITE_CMD_IND message.
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
    // Write response status
    uint8_t status = PRF_ERR_OK;

    if (KE_IDX_GET(src_id) != bmss_env.con_info.conidx)
    {
        status = PRF_APP_ERROR;
        goto done;
    }

    if ((param->handle != (bmss_env.shdl + BMS_IDX_BM_CTRL_PT_VAL)))
    {
        status = ATT_ERR_INVALID_HANDLE;
        goto done;
    }

    if (param->offset + param->length > BMS_CTRL_PT_MAX_LEN)
    {
        status = ATT_ERR_INVALID_ATTRIBUTE_VAL_LEN;
        goto done;
    }

    // Update value in database
    status = attmdb_att_update_value(param->handle, param->length, param->offset,
                                        (uint8_t*)&(param->value[0]));
    if (status != PRF_ERR_OK)
    {
        goto done;
    }

    // Verify written op-code and respond if done updating the value
    if (param->last)
    {
        att_size_t length;
        uint8_t* value;

        // Get the final value after all updates
        status = attmdb_att_get_value(param->handle, &length, &value);
        if (status != PRF_ERR_OK)
        {
            goto done;
        }

        switch (value[0])
        {
            case BMS_OPCODE_DEL_BOND:
                if (!(bmss_env.features & BMS_FEAT_DEL_BOND_SUPPORTED))
                {
                    status = BMS_ATT_OPCODE_NOT_SUPPORTED;
                }
                break;
            case BMS_OPCODE_DEL_ALL_BOND:
                if (!(bmss_env.features & BMS_FEAT_DEL_ALL_BOND_SUPPORTED))
                {
                    status = BMS_ATT_OPCODE_NOT_SUPPORTED;
                }
                break;
            case BMS_OPCODE_DEL_ALL_BOND_BUT_PEER:
                if (!(bmss_env.features & BMS_FEAT_DEL_ALL_BOND_BUT_PEER_SUPPORTED))
                {
                    status = BMS_ATT_OPCODE_NOT_SUPPORTED;
                }
                break;
            default:
                status = BMS_ATT_OPCODE_NOT_SUPPORTED;
                break;
        }

        if (status == PRF_ERR_OK)
        {
            struct bmss_del_bond_req_ind *ind = KE_MSG_ALLOC_DYN(BMSS_DEL_BOND_REQ_IND,
                                                                    bmss_env.con_info.appid,
                                                                    TASK_BMSS,
                                                                    bmss_del_bond_req_ind,
                                                                    length - 1);

            // Extract OpCode
            ind->operation.op_code = value[0];
            ind->operation.operand_length = length - 1;

            // Extract Authorization Code
            if (ind->operation.operand_length)
            {
                memcpy(&(ind->operation.operand), &(value[1]),
                        ind->operation.operand_length);
            }

            ke_msg_send(ind);

            return (KE_MSG_CONSUMED);
        }
    }

done:
    if(param->response)
    {
        // Send Write Response
        atts_write_rsp_send(bmss_env.con_info.conidx, param->handle, status);
    }
    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref BMSS_DEL_BOND_CFM message.
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance
 * @param[in] src_id    ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int bmss_del_bond_cfm_handler(ke_msg_id_t const msgid,
                                     struct bmss_del_bond_cfm const *param,
                                     ke_task_id_t const dest_id,
                                     ke_task_id_t const src_id)
{
    uint8_t status;

    status = param->status;

    // Accept only success or certain error codes from the spec.
    if ((status != PRF_ERR_OK) && (status != ATT_ERR_INSUFF_AUTHOR))
        status = BMS_ATT_OPERATION_FAILED;

    // Send write response
    atts_write_rsp_send(bmss_env.con_info.conidx, bmss_env.shdl + BMS_IDX_BM_CTRL_PT_VAL, status);

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref GAPC_DISCONNECT_IND message. Disconnection
 * indication to BMSS.
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
    // Check Connection Handle
    if (KE_IDX_GET(src_id) == bmss_env.con_info.conidx)
    {
        bmss_disable(param->conhdl);
    }

    return (KE_MSG_CONSUMED);
}

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// Disabled State handler definition.
const struct ke_msg_handler bmss_disabled[] =
{
    {BMSS_CREATE_DB_REQ,        (ke_msg_func_t) bmss_create_db_req_handler},
};

/// Idle State handler definition.
const struct ke_msg_handler bmss_idle[] =
{
    {BMSS_ENABLE_REQ,           (ke_msg_func_t) bmss_enable_req_handler},
};

/// Connected State handler definition.
const struct ke_msg_handler bmss_connected[] =
{
    {GATTC_WRITE_CMD_IND,       (ke_msg_func_t) gattc_write_cmd_ind_handler},
    {BMSS_DEL_BOND_CFM,         (ke_msg_func_t) bmss_del_bond_cfm_handler},
};

/// Default State handlers definition
const struct ke_msg_handler bmss_default_state[] =
{
    {GAPC_DISCONNECT_IND,       (ke_msg_func_t) gapc_disconnect_ind_handler},
};

/// Specifies the message handler structure for every input state.
const struct ke_state_handler bmss_state_handler[BMSS_STATE_MAX] =
{
    [BMSS_DISABLED]    = KE_STATE_HANDLER(bmss_disabled),
    [BMSS_IDLE]        = KE_STATE_HANDLER(bmss_idle),
    [BMSS_CONNECTED]   = KE_STATE_HANDLER(bmss_connected),
};

/// Specifies the message handlers that are common to all states.
const struct ke_state_handler bmss_default_handler = KE_STATE_HANDLER(bmss_default_state);

/// Defines the place holder for the states of all the task instances.
ke_state_t bmss_state[BMSS_IDX_MAX] __attribute__((section("retention_mem_area0"),zero_init));

#endif // BLE_BM_SERVER

