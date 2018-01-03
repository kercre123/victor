/**
 ****************************************************************************************
 *
 * @file findt_task.c
 *
 * @brief Find Me Target Task implementation.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 *
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @addtogroup FINDTTASK
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "rwip_config.h"

#if (BLE_FINDME_TARGET)
#include "gattc_task.h"
#include "findt.h"
#include "findt_task.h"
#include "attm_util.h"
#include "prf_utils.h"

#define FINDT_MANDATORY_MASK        (0x07)

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref FINDT_CREATE_DB_REQ message.
 * The handler adds IAS into the database using the database
 * configuration value given in param.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int findt_create_db_req_handler(ke_msg_id_t const msgid,
                                       struct findt_create_db_req const *param,
                                       ke_task_id_t const dest_id,
                                       ke_task_id_t const src_id)
{
    //Service Configuration Flag
    uint8_t cfg_flag = FINDT_MANDATORY_MASK;
    //Database Creation Status
    uint8_t status;

    //Save Profile ID
    findt_env.con_info.prf_id = TASK_FINDT;

    /*---------------------------------------------------*
     * Immediate Alert Service Creation
     *---------------------------------------------------*/

    //Add Service Into Database
    status = attm_svc_create_db(&findt_env.shdl, (uint8_t *)&cfg_flag, FINDT_IAS_IDX_NB, NULL,
                               dest_id, &findt_att_db[0]);
    //Disable IAS
    attmdb_svc_set_permission(findt_env.shdl, PERM(SVC, DISABLE));

    //Go to Idle State
    if (status == ATT_ERR_NO_ERROR)
    {
        //If we are here, database has been fulfilled with success, go to idle state
        ke_state_set(TASK_FINDT, FINDT_IDLE);
    }

    //Send CFM to application
    struct findt_create_db_cfm * cfm = KE_MSG_ALLOC(FINDT_CREATE_DB_CFM, src_id,
                                                    TASK_FINDT, findt_create_db_cfm);
    cfm->status = status;
    ke_msg_send(cfm);

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref FINDT_ENABLE_REQ message.
 * The handler enables the Find Me profile - target Role.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int findt_enable_req_handler(ke_msg_id_t const msgid,
                                    struct findt_enable_req const *param,
                                    ke_task_id_t const dest_id,
                                    ke_task_id_t const src_id)
{
    // Save the application task id
    findt_env.con_info.appid = src_id;
    // Save the connection index associated to the profile
    findt_env.con_info.conidx = gapc_get_conidx(param->conhdl);

    // Check if the provided connection exist
    if (findt_env.con_info.conidx == GAP_INVALID_CONIDX)
    {
        // The connection doesn't exist, request disallowed
        prf_server_error_ind_send((prf_env_struct *)&findt_env, PRF_ERR_REQ_DISALLOWED,
                                  FINDT_ERROR_IND, FINDT_ENABLE_REQ);
    }
    else
    {
        // Enable Service + Set Security Level
        attmdb_svc_set_permission(findt_env.shdl, param->sec_lvl);

        // Go to connected state
        ke_state_set(TASK_FINDT, FINDT_CONNECTED);
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref GL2C_CODE_ATT_WR_CMD_IND message.
 * The message is redirected from TASK_SVC because at profile enable, the ATT handle is
 * register for TASK_FINDT. In the handler, an ATT Write Response/Error Response should
 * be sent for ATT protocol, but Alert Level Characteristic only supports WNR so no
 * response PDU is needed.
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
    uint8_t alert_lvl = 0x0000;

    if (KE_IDX_GET(src_id) == findt_env.con_info.conidx)
    {
        if(param->handle == findt_env.shdl + FINDT_IAS_IDX_ALERT_LVL_VAL)
        {
            alert_lvl = param->value[0];

            //Check if Alert Level is valid
            if ((param->value[0] <= FINDT_ALERT_HIGH))
            {
                //Update the attribute value
                attmdb_att_set_value(param->handle, sizeof(uint8_t), (uint8_t *)&alert_lvl);
                if(param->last)
                {
                    // Allocate the alert value change indication
                    struct findt_alert_ind *ind = KE_MSG_ALLOC(FINDT_ALERT_IND, findt_env.con_info.appid,
                                                               TASK_FINDT, findt_alert_ind);
                    // Fill in the parameter structure
                    ind->conhdl    = gapc_get_conhdl(findt_env.con_info.conidx);
                    ind->alert_lvl = alert_lvl;

                    // Send the message
                    ke_msg_send(ind);
                }
                // It was a Write Without Response so no RSP needed.
            }
        }
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Disconnection indication to FINDT.
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
    if (KE_IDX_GET(src_id) == findt_env.con_info.conidx)
    {
        findt_disable(param->conhdl);
    }

    return (KE_MSG_CONSUMED);
}

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// Disabled State handler definition.
const struct ke_msg_handler findt_disabled[] =
{
    {FINDT_CREATE_DB_REQ,  (ke_msg_func_t)findt_create_db_req_handler}
};

/// Idle State handler definition.
const struct ke_msg_handler findt_idle[] =
{
    {FINDT_ENABLE_REQ,  (ke_msg_func_t)findt_enable_req_handler}
};

/// Connected State handler definition.
const struct ke_msg_handler findt_connected[] =
{
    {GATTC_WRITE_CMD_IND, (ke_msg_func_t)gattc_write_cmd_ind_handler},
};

/// Default State handlers definition
const struct ke_msg_handler findt_default_state[] =
{
    {GAPC_DISCONNECT_IND,       (ke_msg_func_t)gapc_disconnect_ind_handler},
};

// Specifies the message handler structure for every input state.
const struct ke_state_handler findt_state_handler[FINDT_STATE_MAX] =
{
    [FINDT_DISABLED]    = KE_STATE_HANDLER(findt_disabled),
    [FINDT_IDLE]        = KE_STATE_HANDLER(findt_idle),
    [FINDT_CONNECTED]   = KE_STATE_HANDLER(findt_connected),
};

// Specifies the message handlers that are common to all states.
const struct ke_state_handler findt_default_handler = KE_STATE_HANDLER(findt_default_state);

// Defines the place holder for the states of all the task instances.
ke_state_t findt_state[FINDT_IDX_MAX] __attribute__((section("retention_mem_area0"),zero_init)); //@RETENTION MEMORY

#endif //BLE_FINDME_TARGET

/// @} FINDTTASK
