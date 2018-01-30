/**
 ****************************************************************************************
 *
 * @file app_security_task.c
 *
 * @brief Application Security Task implementation
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 *
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @addtogroup APPSECTASK
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"

#if BLE_APP_SEC
#include "app_security.h"       // Application Security Definition
#include "app_security_task.h"  // Application Security Definition
#include "gapc_task.h"          // GAP Controller Task API
#include "app_api.h"
#include "user_callback_config.h"

/*
 * LOCAL FUNCTION DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Handles reception of bond request command.
 * @param[in] msgid     Id of the message received
 * @param[in] param     Pointer to the parameters of the message
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP)
 * @param[in] src_id    ID of the sending task instance
 * @return If the message was consumed or not
 ****************************************************************************************
 */
static int gapc_bond_req_ind_handler(ke_msg_id_t const msgid,
                                     struct gapc_bond_req_ind *param,
                                     ke_task_id_t const dest_id,
                                     ke_task_id_t const src_id)
{
    uint8_t connection_idx = KE_IDX_GET(src_id);
    switch(param->request)
    {
        // Bond Pairing request
        case GAPC_PAIRING_REQ:
        {
            EXECUTE_CALLBACK_PARAM1_PARAM2(app_on_pairing_request, connection_idx, param);
        }
        break;

        // Retrieve Temporary Key (TK)
        case GAPC_TK_EXCH:
        {
            if(param->data.tk_type == GAP_TK_DISPLAY || param->data.tk_type == GAP_TK_OOB)
            {
                EXECUTE_CALLBACK_PARAM1_PARAM2(app_on_tk_exch_nomitm, connection_idx, param);
            }
            else if (param->data.tk_type == GAP_TK_KEY_ENTRY)
            {
                EXECUTE_CALLBACK_PARAM(app_on_mitm_passcode_req, connection_idx);
            }
            else
            {
                ASSERT_ERR(0);
            }
        }
        break;

        // Retrieve Identity Resolving Key (IRK)
        case GAPC_IRK_EXCH:
        {
            EXECUTE_CALLBACK_PARAM(app_on_irk_exch, param);
        }
        break;

        // Retrieve Connection Signature Resolving Key (CSRK)
        case GAPC_CSRK_EXCH:
        {
            EXECUTE_CALLBACK_PARAM1_PARAM2(app_on_csrk_exch, connection_idx, param);
        }
        break;

        // Retrieve Long Term Key (LTK)
        case GAPC_LTK_EXCH:
        {
            EXECUTE_CALLBACK_PARAM1_PARAM2(app_on_ltk_exch, connection_idx, param);
        }
        break;

        default:
        {
            ASSERT_ERR(0);
        }
        break;
    }
    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of bond indication.
 * @param[in] msgid     Id of the message received
 * @param[in] param     Pointer to the parameters of the message
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP)
 * @param[in] src_id    ID of the sending task instance
 * @return If the message was consumed or not
 ****************************************************************************************
 */
static int gapc_bond_ind_handler(ke_msg_id_t const msgid,
                                 struct gapc_bond_ind *param,
                                 ke_task_id_t const dest_id,
                                 ke_task_id_t const src_id)
{
    uint8_t connection_idx = KE_IDX_GET(src_id);
    switch(param->info)
    {
        // Bond Pairing request
        case GAPC_PAIRING_SUCCEED:
        {
            // Save Authentication level
            app_sec_env[connection_idx].auth = param->data.auth;

            if (app_sec_env[connection_idx].auth & GAP_AUTH_BOND)
            {
                ASSERT_WARNING(connection_idx < APP_EASY_MAX_ACTIVE_CONNECTION);
                app_sec_env[connection_idx].peer_addr_type = app_env[connection_idx].peer_addr_type;
                memcpy(app_sec_env[connection_idx].peer_addr.addr, app_env[connection_idx].peer_addr.addr, BD_ADDR_LEN);
            }
            EXECUTE_CALLBACK_VOID(app_on_pairing_succeded);
        }
        break;

        case GAPC_PAIRING_FAILED:
        {
            // Disconnect
            app_easy_gap_disconnect(connection_idx);

            // Clear bond data
            app_sec_env[connection_idx].auth = 0;
        }
        break;

        case (GAPC_IRK_EXCH):
            // Save Identity Resolving Key
            app_sec_env[connection_idx].irk = param->data.irk;
            break;
        
        case (GAPC_LTK_EXCH):
        case (GAPC_CSRK_EXCH):
        default:
        {
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
 * @brief Handles reception of encrypt request command.
 * @param[in] msgid     Id of the message received
 * @param[in] param     Pointer to the parameters of the message
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP)
 * @param[in] src_id    ID of the sending task instance
 * @return If the message was consumed or not
 ****************************************************************************************
 */
static int gapc_encrypt_req_ind_handler(ke_msg_id_t const msgid,
                                        struct gapc_encrypt_req_ind *param,
                                        ke_task_id_t const dest_id,
                                        ke_task_id_t const src_id)
{
    EXECUTE_CALLBACK_PARAM1_PARAM2(app_on_encrypt_req_ind, KE_IDX_GET(src_id), param);

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of encrypt indication.
 * @param[in] msgid     Id of the message received
 * @param[in] param     Pointer to the parameters of the message
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP)
 * @param[in] src_id    ID of the sending task instance
 * @return If the message was consumed or not
 ****************************************************************************************
 */
static int gapc_encrypt_ind_handler(ke_msg_id_t const msgid,
                                    struct gapc_encrypt_ind *param,
                                    ke_task_id_t const dest_id,
                                    ke_task_id_t const src_id)
{
    EXECUTE_CALLBACK_PARAM(app_on_encrypt_ind, param->auth);

    return (KE_MSG_CONSUMED);
}

/*
 * GLOBAL VARIABLES DEFINITION
 ****************************************************************************************
 */

static const struct ke_msg_handler app_sec_process_handlers[] =
{
    {GAPC_BOND_REQ_IND,                     (ke_msg_func_t)gapc_bond_req_ind_handler},
    {GAPC_BOND_IND,                         (ke_msg_func_t)gapc_bond_ind_handler},
    {GAPC_ENCRYPT_REQ_IND,                  (ke_msg_func_t)gapc_encrypt_req_ind_handler},
    {GAPC_ENCRYPT_IND,                      (ke_msg_func_t)gapc_encrypt_ind_handler},
};

/*
 * GLOBAL FUNCTION DEFINITIONS
 ****************************************************************************************
 */

enum process_event_response app_sec_process_handler(ke_msg_id_t const msgid,
                                                    void const *param,
                                                    ke_task_id_t const dest_id,
                                                    ke_task_id_t const src_id,
                                                    enum ke_msg_status_tag *msg_ret)
{
    return app_std_process_event(msgid,
                                 param,
                                 src_id,
                                 dest_id,
                                 msg_ret,
                                 app_sec_process_handlers,
                                 sizeof(app_sec_process_handlers) / sizeof(struct ke_msg_handler));
}

#endif // BLE_APP_SEC

/// @} APPSECTASK
