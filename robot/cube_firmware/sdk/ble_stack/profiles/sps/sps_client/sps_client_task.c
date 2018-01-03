/**
 ****************************************************************************************
 *
 * @file sps_client_task.c
 *
 * @brief StreamData client Task implementation.
 *
 * Copyright (C) 2012. Dialog Semiconductor Ltd, unpublished work. This computer 
 * program includes Confidential, Proprietary Information and is a Trade Secret of 
 * Dialog Semiconductor Ltd.  All use, disclosure, and/or reproduction is prohibited 
 * unless authorized in writing. All Rights Reserved.
 *
 * <bluetooth.support@diasemi.com> and contributors.
 *
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @addtogroup SPS_CLIENT_TASK
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "rwble_config.h"

#if (BLE_SPS_CLIENT)

#include "sps_client_task.h"
#include "prf_utils.h"
#include "prf_utils_128.h"
#include "app_sps_scheduler.h"

//Flags
uint8_t tx_busy_flag __attribute__((section("retention_mem_area0"),zero_init)); //@RETENTION MEMORY

/// State machine used to retrieve Device Information Service characteristics information
const struct prf_char_def_128 sps_client_sps_char[SPS_CLIENT_MAX] =
{
    [SPS_SERVER_TX_DATA]    = {SPS_SERVER_TX_UUID, ATT_MANDATORY, ATT_CHAR_PROP_NTF},
    [SPS_SERVER_RX_DATA]    = {SPS_SERVER_RX_UUID, ATT_MANDATORY, ATT_CHAR_PROP_WR_NO_RESP},
    [SPS_FLOW_CTRL] 		= {SPS_FLOW_CTRL_UUID, ATT_MANDATORY, ATT_CHAR_PROP_WR_NO_RESP | ATT_CHAR_PROP_NTF}
};
/*
 * Variable 
 ****************************************************************************************
 */

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref GATTC_DISC_SVC_IND message.
 * Get the start and ending handle of sps in the enviroment.
 * @param[in] msgid Id of the message received .
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance 
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */ 
static int gattc_disc_svc_ind_handler(ke_msg_id_t const msgid,
                                             struct gattc_disc_svc_ind const *param,
                                             ke_task_id_t const dest_id,
                                             ke_task_id_t const src_id)
{
    // Get the address of the environment
    struct sps_client_env_tag *sps_client_env = PRF_CLIENT_GET_ENV(dest_id, sps_client);

    // Even if we get multiple responses we only store 1 range
    if (sps_client_env->last_uuid_req == ATT_DECL_PRIMARY_SERVICE)
    {
        sps_client_env->sps_server.svc.shdl = param->start_hdl;
        sps_client_env->sps_server.svc.ehdl = param->end_hdl;
        sps_client_env->nb_streamdatrad_svc++;				
    }
		
    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Enable the StreamData client role, used after connection.
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance
 * @param[in] src_id    ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int sps_client_enable_req_handler(ke_msg_id_t const msgid,
                                    struct sps_client_enable_req const *param,
                                    ke_task_id_t const dest_id,
                                    ke_task_id_t const src_id)
{
    // Status
    uint8_t status;
    
    // Device Information Service Client Role Task Environment
    struct sps_client_env_tag *sps_client_env;
    
    // Connection Information
    struct prf_con_info con_info;

    // Fill the Connection Information structure
    con_info.conidx = gapc_get_conidx(param->conhdl);
    con_info.prf_id = dest_id;
    con_info.appid  = src_id;

    //Reset flags
    tx_busy_flag=0;
    
    // Add an environment for the provided device
    status =  PRF_CLIENT_ENABLE(con_info, param, sps_client);

    if (status == PRF_ERR_FEATURE_NOT_SUPPORTED)
    {
        // The message has been forwarded to another task id.
        return (KE_MSG_NO_FREE);
    }
    else if (status == PRF_ERR_OK)
    {
			
        sps_client_env = PRF_CLIENT_GET_ENV(dest_id, sps_client);

        // Discovery connection
        if (param->con_type == PRF_CON_DISCOVERY)
        { 
            // Start discovering LLS on peer
			uint8_t sps_data_service_uuid[]=SPS_SERVICE_UUID;
            sps_client_env->last_uuid_req = ATT_DECL_PRIMARY_SERVICE;
            memcpy(sps_client_env->last_svc_req, sps_data_service_uuid, ATT_UUID_128_LEN);
            prf_disc_svc_send_128(&sps_client_env->con_info, sps_client_env->last_svc_req);
            // Set state to discovering
            ke_state_set(dest_id, SPS_CLIENT_DISCOVERING);
        }
        else
        {
            //copy over data that has been stored
            sps_client_env->sps_server     = param->sps_server;

            //send confirmation of enable request to application
            sps_client_enable_cfm_send(sps_client_env, &con_info, PRF_ERR_OK);
        }
    }
    else
    {
        sps_client_enable_cfm_send(NULL, &con_info, status);
    }

    // message is consumed
    return (KE_MSG_CONSUMED);
}


/**
 ****************************************************************************************
 * @brief Handles reception of the @ref GATTC_DISC_CHAR_IND message.
 * Characteristics for the currently desired service handle range are obtained and kept.
 * @param[in] msgid Id of the message received .
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance 
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gattc_disc_char_ind_handler(ke_msg_id_t const msgid,
                                          struct gattc_disc_char_ind const *param,
                                          ke_task_id_t const dest_id,
                                          ke_task_id_t const src_id)
{
    // Get the address of the environment
    struct sps_client_env_tag *sps_client_env = PRF_CLIENT_GET_ENV(dest_id, sps_client);

    
    prf_search_chars_128(sps_client_env->sps_server.svc.ehdl, SPS_CLIENT_MAX,
    &sps_client_env->sps_server.chars[0], &sps_client_sps_char[0], param, &sps_client_env->last_char_code);

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles @ref GATTC_CMP_EVT for GATTC_NOTIFY message meaning that
 * notification has been correctly sent to peer device (but not confirmed by peer device).
 * *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance
 * @param[in] src_id    ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gattc_cmp_evt_handler(ke_msg_id_t const msgid,
                                struct gattc_cmp_evt const *param,
                                ke_task_id_t const dest_id,
                                ke_task_id_t const src_id)
{
    uint8_t status = 0;
    uint8_t state = ke_state_get(dest_id);
    struct sps_client_env_tag *sps_client_env = PRF_CLIENT_GET_ENV(dest_id, sps_client);
    uint8_t *data_ptr;
    unsigned read_amount;

    if(state == SPS_CLIENT_DISCOVERING)
    {
        if ((param->status == ATT_ERR_ATTRIBUTE_NOT_FOUND)||(param->status == ATT_ERR_NO_ERROR))
        {
            //Currently discovering sps_server Service
            uint8_t stream_data_service_uuid[]=SPS_SERVICE_UUID;
            if (!memcmp(sps_client_env->last_svc_req, stream_data_service_uuid, ATT_UUID_128_LEN))
            {
                if (sps_client_env->last_uuid_req == ATT_DECL_PRIMARY_SERVICE)
                {
                    if (sps_client_env->sps_server.svc.shdl == ATT_INVALID_HANDLE)
                    {
                        sps_client_enable_cfm_send(sps_client_env, &sps_client_env->con_info, PRF_ERR_STOP_DISC_CHAR_MISSING);
                    }
                    // Too many services found only one such service should exist
                    else if(sps_client_env->nb_streamdatrad_svc > 1)
                    {
                        // stop discovery procedure.
                        sps_client_enable_cfm_send(sps_client_env, &sps_client_env->con_info, PRF_ERR_MULTIPLE_SVC);
                    }
                    else
                    {
                        // Discover StreamData Device characteristics
                        prf_disc_char_all_send(&(sps_client_env->con_info), &(sps_client_env->sps_server.svc));
                        
                        // Keep last UUID requested and for which service in env
                        sps_client_env->last_uuid_req = ATT_DECL_CHARACTERISTIC;
                    }
                }
                else if (sps_client_env->last_uuid_req == ATT_DECL_CHARACTERISTIC)
                {
                    status = prf_check_svc_char_validity_128(SPS_CLIENT_MAX, sps_client_env->sps_server.chars, sps_client_sps_char);
                    if (status == PRF_ERR_OK)
                    {
                        sps_client_env->nb_streamdatrad_svc = 0;
                        sps_client_enable_cfm_send(sps_client_env, &sps_client_env->con_info, status);
                    }
                    else
                    {
                        sps_client_enable_cfm_send(sps_client_env, &sps_client_env->con_info, status);
                    }
                }
            }
        }
    }
    else if (state == SPS_CLIENT_CONNECTED)
    {
        if(param->req_type == GATTC_WRITE_NO_RESPONSE)
        {
            tx_busy_flag--;
            if (( tx_busy_flag == 0) && (ble_flags.txAllowed == TRUE))
            {
                if ((read_amount = app_item_count(&uarttoble_buffer)) > TX_WAIT_LEVEL)
                {
                    if (read_amount > TX_SIZE)
                    {
                        read_amount = TX_SIZE;
                    }
                    read_amount = (read_amount<TX_START_FRAME_DATA_SIZE?read_amount:(((read_amount-TX_START_FRAME_DATA_SIZE) / TX_CONTINUE_FRAME_DATA_SIZE)*TX_CONTINUE_FRAME_DATA_SIZE + TX_START_FRAME_DATA_SIZE));
                    read_amount = app_get_item_address(&uarttoble_buffer, &data_ptr, read_amount);
                    prf_gatt_write(&sps_client_env->con_info, sps_client_env->sps_server.chars[SPS_SERVER_RX_DATA].val_hdl, data_ptr,sizeof(uint8_t) * read_amount, GATTC_WRITE_NO_RESPONSE);
                    app_release_items(&uarttoble_buffer, read_amount);
                    tx_busy_flag++;
                    app_check_uart_xon();
                }
            }
        }
    }
		
    return (KE_MSG_CONSUMED);
}


/**
 ****************************************************************************************
 * @brief Disconnection indication to StreamData client profiel
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
    PRF_CLIENT_DISABLE_IND_SEND(sps_client_envs, dest_id, SPS_CLIENT,param->conhdl);
    
    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref GATTC_EVENT_IND message.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */

static int gattc_event_ind_handler(ke_msg_id_t const msgid,
                                        struct gattc_event_ind const *param,
                                        ke_task_id_t const dest_id,
                                        ke_task_id_t const src_id)
{

    struct sps_client_env_tag *sps_client_env = PRF_CLIENT_GET_ENV(dest_id, sps_client);
    
    if(sps_client_env->sps_server.chars[SPS_FLOW_CTRL].val_hdl == param->handle)
    {				
        app_update_transmit_allowed(&ble_flags, param->value[0]);
    }
    else if (sps_client_env->sps_server.chars[SPS_SERVER_TX_DATA].val_hdl == param->handle)
    {
        app_ble_push((uint8_t*)&(param->value[0]), param->length);
    }

    return (KE_MSG_CONSUMED);
}


/**
 ****************************************************************************************
 * @brief 
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int sps_client_init_ble_tx_req_handler(ke_msg_id_t const msgid,
                                        struct sps_client_init_ble_tx_req const *param,
                                        ke_task_id_t const dest_id,
                                        ke_task_id_t const src_id)
{
    uint8_t *data_ptr;
    unsigned read_amount;
    struct sps_client_env_tag *sps_client_env = PRF_CLIENT_GET_ENV(dest_id, sps_client);
    
    if (ke_state_get(TASK_SPS_CLIENT)==SPS_CLIENT_CONNECTED)
    {
        if ((tx_busy_flag == 0) && (ble_flags.txAllowed == TRUE))
        {
            if ((read_amount = app_item_count(&uarttoble_buffer)) > 0 )
            {
                if (read_amount > TX_SIZE)
                {
                    read_amount = TX_SIZE;
                }
                read_amount = (read_amount<TX_START_FRAME_DATA_SIZE?read_amount:(((read_amount-TX_START_FRAME_DATA_SIZE) / TX_CONTINUE_FRAME_DATA_SIZE)*TX_CONTINUE_FRAME_DATA_SIZE + TX_START_FRAME_DATA_SIZE));
                read_amount = app_get_item_address(&uarttoble_buffer, &data_ptr, read_amount);
                prf_gatt_write(&sps_client_env->con_info, sps_client_env->sps_server.chars[SPS_SERVER_RX_DATA].val_hdl, data_ptr,sizeof(uint8_t) * read_amount, GATTC_WRITE_NO_RESPONSE);
                app_release_items(&uarttoble_buffer, read_amount);
                tx_busy_flag++;
                app_check_uart_xon();
            }
        }
    }

    
    return (KE_MSG_CONSUMED);
}    
/**
 ****************************************************************************************
 * @brief Send flow control state @ref sps_client_send_notify_flow_control_state_req message.
 * @param[in] msgid Id of the message received 
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int sps_client_send_notify_flow_control_state_req_handler(ke_msg_id_t const msgid,
                                   struct sps_client_send_notify_flow_control_state_req const *param,
                                   ke_task_id_t const dest_id,
                                   ke_task_id_t const src_id)
 
{ 
    
    struct sps_client_env_tag *sps_client_env = PRF_CLIENT_GET_ENV(dest_id, sps_client);
   
    // Send notification to the client
    prf_gatt_write(&sps_client_env->con_info, sps_client_env->sps_server.chars[SPS_FLOW_CTRL].val_hdl, (uint8_t*) &param->flow_control_state,sizeof (uint8_t),GATTC_WRITE_NO_RESPONSE);
    
    tx_busy_flag++;
    
    return (KE_MSG_CONSUMED);
}

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

// CONNECTED State handlers definition.
const struct ke_msg_handler sps_client_connected[] =
{
    {SPS_CLIENT_ENABLE_REQ,      			(ke_msg_func_t)sps_client_enable_req_handler},
    {GATTC_EVENT_IND, 						(ke_msg_func_t)gattc_event_ind_handler},
    {GAPC_DISCONNECT_IND,         			(ke_msg_func_t)gapc_disconnect_ind_handler},
    {SPS_CLIENT_INIT_BLE_TX_REQ,            (ke_msg_func_t)sps_client_init_ble_tx_req_handler},
    {SPS_CLIENT_SEND_FLOW_CONTROL_REQ,	    (ke_msg_func_t)sps_client_send_notify_flow_control_state_req_handler},
};	

// Specifies the Discovering state message handlers
const struct ke_msg_handler sps_client_discovering[] =
{
		
    {GATTC_DISC_CHAR_IND,                   (ke_msg_func_t)gattc_disc_char_ind_handler},
    {GATTC_DISC_SVC_IND,                    (ke_msg_func_t)gattc_disc_svc_ind_handler},
};

/// Default State handlers definition
const struct ke_msg_handler sps_client_default_state[] =
{
    {SPS_CLIENT_ENABLE_REQ,                 (ke_msg_func_t)sps_client_enable_req_handler},
    {GAPC_DISCONNECT_IND,                   (ke_msg_func_t)gapc_disconnect_ind_handler},
    {GATTC_CMP_EVT,                         (ke_msg_func_t)gattc_cmp_evt_handler},
};

// Specifies the message handler structure for every input state.
const struct ke_state_handler sps_client_state_handler[SPS_CLIENT_STATE_MAX] =
{
    [SPS_CLIENT_IDLE]        = KE_STATE_HANDLER_NONE,
    [SPS_CLIENT_CONNECTED]   = KE_STATE_HANDLER(sps_client_connected),
    [SPS_CLIENT_DISCOVERING] = KE_STATE_HANDLER(sps_client_discovering),
};

// Specifies the message handlers that are common to all states.
const struct ke_state_handler sps_client_default_handler = KE_STATE_HANDLER(sps_client_default_state);

// Defines the place holder for the states of all the task instances.
ke_state_t sps_client_state[SPS_CLIENT_IDX_MAX] __attribute__((section("exchange_mem_case1")));

#endif //BLE_SPS_CLIENT

/// @} SPS_CLIENTTASK
