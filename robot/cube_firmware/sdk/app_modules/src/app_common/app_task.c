/**
 ****************************************************************************************
 *
 * @file app_task.c
 *
 * @brief RW APP Task implementation
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 *
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @addtogroup APPTASK
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"            // SW configuration

#if BLE_APP_PRESENT

#include <string.h>
#include "gapc_task.h"          // GAP Controller Task API
#include "gapm_task.h"          // GAP Manager Task API
#include "gap.h"                // GAP Definitions
#include "gapc.h"               // GAPC Definitions
#include "co_error.h"           // Error Codes Definition
#include "arch.h"               // Platform Definitions
#include "app_task.h"           // Application Task API
#include "app.h"                // Application Definition
#include "app_security.h"       // Application Definition
#include "app_security_task.h"  // Application Security Task API
#include "app_mid.h"
#include "app_callback.h"
#include "app_default_handlers.h"
#include "app_entry_point.h"
#include "user_callback_config.h"

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Handles ready indication from the GAP. - Reset the stack.
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gapm_device_ready_ind_handler(ke_msg_id_t const msgid,
                                         void const *param,
                                         ke_task_id_t const dest_id,
                                         ke_task_id_t const src_id)
{
    if (ke_state_get(dest_id) == APP_DISABLED)
    {
        // reset the lower layers.
        app_gapm_reset_op();
    }
    else
    {
        // APP_DISABLED state is used to wait the GAP_READY_EVT message
        ASSERT_ERR(0);
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles GAP manager command complete events.
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gapm_cmp_evt_handler(ke_msg_id_t const msgid,
                                struct gapm_cmp_evt const *param,
                                ke_task_id_t const dest_id,
                                ke_task_id_t const src_id)
{
    switch(param->operation)
    {
        // reset completed
        case GAPM_RESET:
        {
            if(param->status != GAP_ERR_NO_ERROR)
            {
                ASSERT_ERR(0); // unexpected error
            }
            else
            {
                // set device configuration
                app_easy_gap_dev_configure ();
            }
        }
        break;

        // device configuration updated
        case GAPM_SET_DEV_CONFIG:
        {
            if(param->status != GAP_ERR_NO_ERROR)
            {
                ASSERT_ERR(0); // unexpected error
            }
            else
            {
                EXECUTE_CALLBACK_VOID(app_on_set_dev_config_complete);
            }
        }
        break;

        // Non connectable advertising finished
        case GAPM_ADV_NON_CONN:
        {
           EXECUTE_CALLBACK_PARAM(app_on_adv_nonconn_complete, param->status);
        }
        break;

        // Undirected connectable advertising finished
        case GAPM_ADV_UNDIRECT:
        {
           EXECUTE_CALLBACK_PARAM(app_on_adv_undirect_complete, param->status);
        }
        break;

        // Directed connectable advertising finished
        case GAPM_ADV_DIRECT:
        {
            EXECUTE_CALLBACK_PARAM(app_on_adv_direct_complete, param->status);
        }
        break;

        case GAPM_SCAN_ACTIVE:
        case GAPM_SCAN_PASSIVE:
        {
            EXECUTE_CALLBACK_PARAM(app_on_scanning_completed, param->status);
        }
        break;

        case GAPM_CONNECTION_DIRECT:
            if (param->status == GAP_ERR_CANCELED)
            {
                EXECUTE_CALLBACK_VOID(app_on_connect_failed);
            }
        break;

        case GAPM_CANCEL:
        {
            if(param->status != GAP_ERR_NO_ERROR)
            {
                ASSERT_ERR(0); // unexpected error
            }
            if (app_process_catch_rest_cb != NULL)
            {
                app_process_catch_rest_cb(msgid, param, dest_id, src_id);
            }
         }
        break;

        default:
            if (app_process_catch_rest_cb != NULL)
            {
                app_process_catch_rest_cb(msgid, param, dest_id, src_id);
            }
        break;
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles connection complete event from the GAP. Will enable profile.
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gapc_connection_req_ind_handler(ke_msg_id_t const msgid,
                                           struct gapc_connection_req_ind const *param,
                                           ke_task_id_t const dest_id,
                                           ke_task_id_t const src_id)
{
    // Connection Index
    if (ke_state_get(dest_id) == APP_CONNECTABLE)
    {
        uint8_t connection_idx = KE_IDX_GET(src_id);
        ASSERT_WARNING(connection_idx < APP_EASY_MAX_ACTIVE_CONNECTION);
        app_env[connection_idx].conidx = connection_idx;

        if (connection_idx != GAP_INVALID_CONIDX)
        {
            app_env[connection_idx].connection_active = true;
            ke_state_set(TASK_APP, APP_CONNECTED);
            // Retrieve the connection info from the parameters
            app_env[connection_idx].conhdl = param->conhdl;
            app_env[connection_idx].peer_addr_type = param->peer_addr_type;
            memcpy(app_env[connection_idx].peer_addr.addr, param->peer_addr.addr, BD_ADDR_LEN);
            #if (BLE_APP_SEC)
            // send connection confirmation
                app_easy_gap_confirm(connection_idx, (enum gap_auth) app_sec_env[connection_idx].auth, GAP_AUTHZ_NOT_SET);
            #else // (BLE_APP_SEC)
                app_easy_gap_confirm(connection_idx, GAP_AUTH_REQ_NO_MITM_NO_BOND, GAP_AUTHZ_NOT_SET);
            #endif
        }
        EXECUTE_CALLBACK_PARAM1_PARAM2(app_on_connection, connection_idx, param);
    }
    else
    {
        // APP_CONNECTABLE state is used to wait the GAP_LE_CREATE_CONN_REQ_CMP_EVT message
        ASSERT_ERR(0);
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles GAP controller command complete events.
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gapc_cmp_evt_handler(ke_msg_id_t const msgid,
                                struct gapc_cmp_evt const *param,
                                ke_task_id_t const dest_id,
                                ke_task_id_t const src_id)
{
    switch(param->operation)
    {
        // reset completed
        case GAPC_UPDATE_PARAMS:
        {
            if (ke_state_get(dest_id) == APP_PARAM_UPD)
            {
                if ((param->status != CO_ERROR_NO_ERROR))
                {
                    // it's application specific what to do when the Param Upd request is rejected
                    EXECUTE_CALLBACK_PARAM(app_on_update_params_rejected, (param->status));

                }
                else
                {
                    // Go to Connected State
                    ke_state_set(dest_id, APP_CONNECTED);
                    // if state is APP_CONNECTED then the request was accepted
                    EXECUTE_CALLBACK_VOID(app_on_update_params_complete);

                }
            }
        }
        break;

        default:
        {
            if(param->status != GAP_ERR_NO_ERROR)
            {
                ASSERT_ERR(0); // unexpected error
            }
            if (app_process_catch_rest_cb != NULL)
            {
                app_process_catch_rest_cb(msgid, param, dest_id, src_id);
            }
        }
        break;
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles disconnection complete event from the GAP.
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gapc_disconnect_ind_handler(ke_msg_id_t const msgid,
                                       struct gapc_disconnect_ind const *param,
                                       ke_task_id_t const dest_id,
                                       ke_task_id_t const src_id)
{
    uint8_t state = ke_state_get(dest_id);
    if ((state == APP_SECURITY) || (state == APP_CONNECTED) || (state == APP_PARAM_UPD))
    {
         EXECUTE_CALLBACK_PARAM(app_on_disconnect, param);
    }
    else
    {
        // We are not in a Connected State
        ASSERT_ERR(0);
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the APP_MODULE_INIT_CMP_EVT messgage
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance
 * @param[in] src_id    ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int app_module_init_cmp_evt_handler(ke_msg_id_t const msgid,
                                           const struct app_module_init_cmp_evt *param,
                                           ke_task_id_t const dest_id,
                                           ke_task_id_t const src_id)
{
    if (ke_state_get(dest_id) == APP_DB_INIT)
    {
        if (param->status == CO_ERROR_NO_ERROR)
        {
            // Add next required service in the database
            if (app_db_init())
            {
                // No more service to add in the database, start application
                EXECUTE_CALLBACK_VOID(app_on_db_init_complete)
            }
        }
        else
        {
            // An error has occurred during database creation
            ASSERT_ERR(0);
        }
    }
    else
    {
        // APP_DB_INIT state is used to wait the APP_MODULE_INIT_CMP_EVT message
        ASSERT_ERR(0);
    }

    return (KE_MSG_CONSUMED);
}

/*
 ****************************************************************************************
 * @brief Handles GAPM_ADV_REPORT_IND event.
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
*/
static int gapm_adv_report_ind_handler(ke_msg_id_t msgid,
                                       struct gapm_adv_report_ind *param,
                                       ke_task_id_t dest_id,
                                       ke_task_id_t src_id)
{
    EXECUTE_CALLBACK_PARAM(app_on_adv_report_ind, param)
    return (KE_MSG_CONSUMED);
}

/*
 ****************************************************************************************
 * @brief Handles GAPC_SECURITY_IND event.
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
*/
#if (BLE_APP_SEC)
static int gapc_security_ind_handler(ke_msg_id_t msgid,
                                     struct gapc_security_ind *param,
                                     ke_task_id_t const dest_id,
                                     ke_task_id_t const src_id)
{
    EXECUTE_CALLBACK_PARAM(app_on_security_req_ind, param->auth)
    return (KE_MSG_CONSUMED);
}
#endif

/*
 * GLOBAL VARIABLES DEFINITION
 ****************************************************************************************
 */

static const struct ke_msg_handler app_gap_process_handlers[]=
{
    {GAPM_DEVICE_READY_IND,                 (ke_msg_func_t)gapm_device_ready_ind_handler},
    {GAPM_CMP_EVT,                          (ke_msg_func_t)gapm_cmp_evt_handler},
    {GAPC_CMP_EVT,                          (ke_msg_func_t)gapc_cmp_evt_handler},
    {GAPC_CONNECTION_REQ_IND,               (ke_msg_func_t)gapc_connection_req_ind_handler},
    {GAPC_DISCONNECT_IND,                   (ke_msg_func_t)gapc_disconnect_ind_handler},
    {APP_MODULE_INIT_CMP_EVT,               (ke_msg_func_t)app_module_init_cmp_evt_handler},
    {GAPM_ADV_REPORT_IND,                   (ke_msg_func_t)gapm_adv_report_ind_handler},
#if (BLE_APP_SEC)
    {GAPC_SECURITY_IND,                     (ke_msg_func_t)gapc_security_ind_handler},
#endif
};

/* Default State handlers definition. */
const struct ke_msg_handler app_default_state[] =
{
    {KE_MSG_DEFAULT_HANDLER,                (ke_msg_func_t)app_entry_point_handler},
};

/* Specifies the message handlers that are common to all states. */
const struct ke_state_handler app_default_handler = KE_STATE_HANDLER(app_default_state);

/* Defines the place holder for the states of all the task instances. */
ke_state_t app_state[APP_IDX_MAX] __attribute__((section("retention_mem_area0"), zero_init)); //RETENTION MEMORY

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

enum process_event_response app_gap_process_handler (ke_msg_id_t const msgid,
                                                     void const *param,
                                                     ke_task_id_t const dest_id,
                                                     ke_task_id_t const src_id,
                                                     enum ke_msg_status_tag *msg_ret)
{
    return app_std_process_event(msgid, param, src_id, dest_id, msg_ret, app_gap_process_handlers,
                                         sizeof(app_gap_process_handlers) / sizeof(struct ke_msg_handler));
}

#endif // BLE_APP_PRESENT

/// @} APPTASK
