/**
 ****************************************************************************************
 *
 * @file app_task.c
 *
 * @brief Handling of ble events and responses.
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


#include "app_task.h" 
#include "app.h" 
#include "queue.h" 

#include "gapm_task.h" 
#include "gapc_task.h" 
#include "gattc_task.h" 

#include "ble_msg.h" 


/**
 ****************************************************************************************
 * @brief Extracts device name from adv data if present and copy it to app_env.
 *
 * @param[in] adv_data      Pointer to advertise data.
 * @param[in] adv_data_len  Advertise data length.
 * @param[in] dev_indx      Devices index in device list.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
void app_find_device_name(unsigned char * adv_data, unsigned char adv_data_len, unsigned char dev_indx)
{
    unsigned char indx = 0;

    while (indx < adv_data_len)
    {
        if (adv_data[indx+1] == 0x09)
        {
            memcpy(app_env.devices[dev_indx].data, &adv_data[indx+2], (size_t) adv_data[indx]);
            app_env.devices[dev_indx].data_len = (unsigned char ) adv_data[indx];
        }
        indx += (unsigned char ) adv_data[indx]+1;
    }
}

/**
 ****************************************************************************************
 * @brief Handles GAPM command completion events.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
int gap_cmp_evt_handler(ke_msg_id_t msgid,
                        struct gapm_cmp_evt *param,
                        ke_task_id_t dest_id,
                        ke_task_id_t src_id)
{
    if (param->status ==  CO_ERROR_NO_ERROR)
    {
        if(param->operation == GAPM_RESET)
        {
            gapm_reset_cmd_completed = TRUE;
            app_set_mode(); // initialize 
        }
        else if(param->operation == GAPM_SET_DEV_CONFIG)
        {
            // start scanning for target device
            printf("Scanning... \n");
            app_env.state = APP_SCAN;            
            app_inq();
        }
    }
    else
    {
        if(param->operation == GAPM_SCAN_ACTIVE || param->operation == GAPM_SCAN_PASSIVE)
        {
            // scan operation has completed
            app_env.state = APP_IDLE; 

            // check if the target device was found
            if (app_env.target_idx != -1)
            {
                // connect to the target
                printf("Connecting to target device...\n");
                app_connect(app_env.target_idx);
            }
            else
            {
                if ( app_env.scan_attempts_made < MAX_SCANNING_ATTEMPTS )
                {
                    // do next scan attempt
                    printf("Scanning...\n");
                    app_env.state = APP_SCAN;
                    app_inq();
                }
                else
                {
                    // no more scan attempts -> exit application
                    printf("Target device was not found. \n");
                    app_exit();
                }
            }

        }
    }

    return (KE_MSG_CONSUMED);
}


/**
 ****************************************************************************************
 * @brief Handles GAPC command completion events.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
int gapc_cmp_evt_handler(ke_msg_id_t msgid,
                         struct gapc_cmp_evt *param,
                         ke_task_id_t dest_id,
                         ke_task_id_t src_id)
{
    switch (param->operation)
    {
        case GAPC_ENCRYPT:
            if (param->status != CO_ERROR_NO_ERROR)
            {
                app_env.peer_device.bonded = 0;
                app_disconnect();
            }
            break;
        
        case GAPC_DISCONNECT:
            if (app_env.state == APP_DISCONNECTING)
            {
                printf("app_exit() after GAPC_DISCONNECT_CMD completion");
                app_exit();
            }

        default:
            break;
    }

    return 0;
}

/**
 ****************************************************************************************
 * @brief Handles ready indication from the GAP.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
int gap_ready_evt_handler(ke_msg_id_t msgid,
                          void *param,
                          ke_task_id_t dest_id,
                          ke_task_id_t src_id)
{
    app_rst_gap();

    return 0;
}

/**
 ****************************************************************************************
 * @brief Handles Inquiry result event.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
int gap_dev_inq_result_handler(ke_msg_id_t msgid,
                               struct gapm_adv_report_ind *param,
                               ke_task_id_t dest_id,
                               ke_task_id_t src_id)
{
    unsigned char recorded;

    if (app_env.state != APP_SCAN)
        return -1;
    
    recorded = app_device_recorded(&param->report.adv_addr); 
    
    if (recorded < MAX_SCAN_DEVICES) // update Name
    {
        app_find_device_name(param->report.data,param->report.data_len, recorded); 
    }
    else
    {
        app_env.devices[app_env.num_of_devices].free = false;
        app_env.devices[app_env.num_of_devices].rssi = (char)(((479 * param->report.rssi)/1000) - 112.5) ;
        memcpy(app_env.devices[app_env.num_of_devices].adv_addr.addr, param->report.adv_addr.addr, BD_ADDR_LEN );
        app_find_device_name(param->report.data,param->report.data_len, app_env.num_of_devices); 

        // check if this is our target device 
        if (bdaddr_compare(&param->report.adv_addr, &spota_options.target_spotar_bdaddr) )
        {
            app_env.target_idx = app_env.num_of_devices;
            printf("Target device was found. \n");
        };

        app_env.num_of_devices++;        
    }

    return 0;
}

/**
 ****************************************************************************************
 * @brief Handles Connection request indication event.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
unsigned int start_pair;
int gapc_connection_req_ind_handler(ke_msg_id_t msgid,
                                    struct gapc_connection_req_ind *param,
                                    ke_task_id_t dest_id,
                                    ke_task_id_t src_id)
{
  start_pair = 1;

  if (app_env.state == APP_IDLE)
  {
      // We are now connected
      app_env.state = APP_CONNECTED;

      // Retrieve the connection info from the parameters
      app_env.peer_device.device.conhdl = param->conhdl;
      // Retrieve the connection index from the src_id
      app_env.peer_device.device.conidx = KE_IDX_GET(src_id);

      // On Reconnection check if device is bonded and send pairing request. Otherwise it is not bonded.
      if (bdaddr_compare(&app_env.peer_device.device.adv_addr, &param->peer_addr))
      {
          if (app_env.peer_device.bonded)
              start_pair = 0;
      }
      
      memcpy(app_env.peer_device.device.adv_addr.addr, param->peer_addr.addr, sizeof(struct bd_addr));

      app_connect_confirm(GAP_AUTH_REQ_NO_MITM_NO_BOND);

      app_security_enable();
    }

    return 0;
}

/**
 ****************************************************************************************
 * @brief Handles Discconnection completion event.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
int gapc_disconnect_ind_handler(ke_msg_id_t msgid,
                                struct gapc_disconnect_ind *param,
                                ke_task_id_t dest_id,
                                ke_task_id_t src_id)
{
    if (param->conhdl != app_env.peer_device.device.conhdl)
    {
        return 0;
    }

    if (app_env.state == APP_DISCONNECTING)
    {
        printf("Exiting... \n");
        app_exit();
    }
    else
    {
        /// unexpected disconnection
        printf("Error: unexpected disconnection (reason = 0x%02X).\nProcedure failed\n", param->reason);
        app_exit();
    }

    return 0;
}

/**
 ****************************************************************************************
 * @brief Handle Bond indication.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
int gap_bond_ind_handler(ke_msg_id_t msgid,
                         struct gapc_bond_ind *param,
                         ke_task_id_t dest_id,
                         ke_task_id_t src_id)
{
    switch (param->info)
    {
        case GAPC_PAIRING_SUCCEED:
            if (param->data.auth | GAP_AUTH_BOND)
                app_env.peer_device.bonded = 1;
            
            // discover spota service
            app_env.peer_device.spota.svc_found = 0;
            app_env.peer_device.spota.svc_start_handle = 0;
            app_env.peer_device.spota.svc_end_handle = 0;
            app_env.peer_device.spota.mem_dev_handle = 0;
            app_env.peer_device.spota.gpio_map_handle = 0;
            app_env.peer_device.spota.mem_info_handle = 0;
            app_env.peer_device.spota.patch_len_handle = 0;
            app_env.peer_device.spota.patch_data_handle = 0;
            app_env.peer_device.spota.serv_status_handle = 0;
            app_env.peer_device.spota.serv_status_cli_char_cfg_desc_handle = 0;            

            app_discover_svc_by_uuid_16(app_env.peer_device.device.conidx, SPOTA_PRIMARY_SERVICE_UUID);

            break;

        case GAPC_IRK_EXCH:
            memcpy (app_env.peer_device.irk.irk.key, param->data.irk.irk.key, KEY_LEN);
            memcpy (app_env.peer_device.irk.addr.addr.addr, param->data.irk.addr.addr.addr, KEY_LEN);
            app_env.peer_device.irk.addr.addr_type = param->data.irk.addr.addr_type;
            break;

        case GAPC_LTK_EXCH:
            app_env.peer_device.ediv = param->data.ltk.ediv;
            memcpy (app_env.peer_device.rand_nb, param->data.ltk.randnb.nb, RAND_NB_LEN);
            app_env.peer_device.ltk.key_size = param->data.ltk.key_size;
            memcpy (app_env.peer_device.ltk.ltk.key, param->data.ltk.ltk.key, param->data.ltk.key_size);
            app_env.peer_device.ltk.ediv = param->data.ltk.ediv;
            memcpy (app_env.peer_device.ltk.randnb.nb, param->data.ltk.randnb.nb, RAND_NB_LEN);
            break;

        case GAPC_PAIRING_FAILED:
            printf("Pairing failed. Aborting...\n");
            app_env.peer_device.bonded = 0;
            app_disconnect();
            break;
    }

    return 0;
}

/**
 ****************************************************************************************
 * @brief Handles GAPC_BOND_REQ_IND event.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
int gap_bond_req_ind_handler(ke_msg_id_t msgid,
                             struct gapc_bond_req_ind *param,
                             ke_task_id_t dest_id,
                             ke_task_id_t src_id)
{
    switch (param->request)
    {
        case GAPC_CSRK_EXCH:
            app_gap_bond_cfm();
            break;

        default:
            break;
    }
    
    return 0;
}

/**
 ****************************************************************************************
 * @brief Handles GATTC_DISC_SVC_IND event.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
int gattc_disc_svc_ind_handler(ke_msg_id_t msgid,
                               struct gattc_disc_svc_ind *param,
                               ke_task_id_t dest_id,
                               ke_task_id_t src_id)
{
    uint16_t uuid = 0;

    if(param->uuid_len == 2)
    {
        uuid = (param->uuid[1] << 8) | param->uuid[0];
        
        if (uuid == SPOTA_PRIMARY_SERVICE_UUID)
        {
            app_env.peer_device.spota.svc_found = 1;
            app_env.peer_device.spota.svc_start_handle = param->start_hdl;
            app_env.peer_device.spota.svc_end_handle = param->end_hdl;
        }
    }

    return 0;
}

/**
 ****************************************************************************************
 * @brief Handles GATTC_DISC_CHAR_IND event.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
int gattc_disc_char_ind_handler(ke_msg_id_t msgid,
                                struct gattc_disc_char_ind *param,
                                ke_task_id_t dest_id,
                                ke_task_id_t src_id)
{
    // SPOTA characteristics have 128 bit UUIDs
    if (param->uuid_len != 16 )
    {
        return 0;
    }

    if      ( 0 == memcmp(param->uuid, SPOTA_MEM_DEV_UUID, 16) ) { app_env.peer_device.spota.mem_dev_handle = param->pointer_hdl; }
    else if ( 0 == memcmp(param->uuid, SPOTA_GPIO_MAP_UUID, 16) ) { app_env.peer_device.spota.gpio_map_handle = param->pointer_hdl; }
    else if ( 0 == memcmp(param->uuid, SPOTA_MEM_INFO_UUID, 16) ) {  app_env.peer_device.spota.mem_info_handle= param->pointer_hdl; }
    else if ( 0 == memcmp(param->uuid, SPOTA_PATCH_LEN_UUID, 16) ) {  app_env.peer_device.spota.patch_len_handle = param->pointer_hdl; }
    else if ( 0 == memcmp(param->uuid, SPOTA_PATCH_DATA_UUID, 16) ) {  app_env.peer_device.spota.patch_data_handle = param->pointer_hdl; }
    else if ( 0 == memcmp(param->uuid, SPOTA_SERV_STATUS_UUID, 16) ) { app_env.peer_device.spota.serv_status_handle = param->pointer_hdl; }

    return 0;
}

/**
 ****************************************************************************************
 * @brief Handles GATTC_DISC_CHAR_DESC_IND event.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
int gattc_disc_char_desc_ind_handler(ke_msg_id_t msgid,
                                     struct gattc_disc_char_desc_ind *param,
                                     ke_task_id_t dest_id,
                                     ke_task_id_t src_id)
{
    uint16_t uuid = 0;

    if (param->uuid_len == 2)
    {
        uuid = (param->uuid[1] << 8) | param->uuid[0];

        switch(uuid)
        {
            case CLIENT_CHARACTERISTIC_CONFIGURATION_DESCRIPTOR_UUID:
                app_env.peer_device.spota.serv_status_cli_char_cfg_desc_handle = param->attr_hdl;
                break;
        default:
            break;
        }
    }
    return 0;
};

/**
 ****************************************************************************************
 * @brief Handles GATTC_READ_IND event.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
int gattc_read_ind_handler(ke_msg_id_t msgid,
                           struct gattc_read_ind *param,
                           ke_task_id_t dest_id,
                           ke_task_id_t src_id)
{
    if (param->handle == app_env.peer_device.spota.mem_dev_handle)
    {
        memcpy(app_env.peer_device.spota.mem_dev_value, param->value, param->length);
    }
    else if (param->handle == app_env.peer_device.spota.gpio_map_handle)
    {
        memcpy(app_env.peer_device.spota.gpio_map_value, param->value, param->length);
    }
    else if (param->handle == app_env.peer_device.spota.mem_info_handle)
    {
        memcpy(app_env.peer_device.spota.mem_info_value, param->value, param->length);
    }
    else if (param->handle == app_env.peer_device.spota.patch_len_handle)
    {
        memcpy(app_env.peer_device.spota.patch_len_value, param->value, param->length);
    }
    else if (param->handle == app_env.peer_device.spota.patch_data_handle)
    {
        memcpy(app_env.peer_device.spota.patch_data_value, param->value, param->length);
    }
    else if (param->handle == app_env.peer_device.spota.serv_status_handle)
    {
        app_env.peer_device.spota.serv_status_value = param->value[0];
    }

    return 0;
}

/**
 ****************************************************************************************
 * @brief Handles GATTC_EVENT_IND event.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
int gattc_event_ind_handler(ke_msg_id_t msgid,
                            struct gattc_event_ind *param,
                            ke_task_id_t dest_id,
                            ke_task_id_t src_id)
{
    // handle notifications from SPOTA_SERV_STATUS only 
    if(param->handle != app_env.peer_device.spota.serv_status_handle)
    {
        return 0;
    }
    
    // retrieve notification value
    app_env.peer_device.spota.serv_status_value = param->value[0];
    
    if (app_env.state == APP_WR_PATCH_DATA) // handle the notification only after the last chunk has been written
    {
        bool last_chunk = (patch_chunck_offset + patch_chunck_length) == patch_length; // true if last patch chunk was written
        
        if (last_chunk)
        {
            // check SPOTA_SERV_STATUS
            if (app_env.peer_device.spota.serv_status_value != SPOTA_STATUS_CMP_OK)
            {
                printf("error: SPOTA_SERV_STATUS = 0x%02X after writing last chunk in SPOTA_PATCH_DATA \n", app_env.peer_device.spota.serv_status_value);
                // disconnect
                app_env.state = APP_DISCONNECTING;
                app_disconnect();
                return 0; // in order to exit this function immediately
            }
            
            // no more chunks to write - SPOTA procedure completed successfully
            printf("\nSPOTA procedure finished successfully! \n\n");
            
            //
            // trigger next step - read SPOTA_MEM_INFO (the 2nd time)
            //
            app_env.state = APP_RD_MEM_INFO_2;
            app_characteristic_read(KE_IDX_GET(src_id), app_env.peer_device.spota.mem_info_handle);
        }
    }


    return 0;
}

/**
 ****************************************************************************************
 * @brief Handles GATTC command completion event.
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP).
 * @param[in] src_id    ID of the sending task instance.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
int gattc_cmp_evt_handler(ke_msg_id_t msgid,
                          struct gattc_cmp_evt *param,
                          ke_task_id_t dest_id,
                          ke_task_id_t src_id)
{
    switch(param->req_type)
    {

    case GATTC_DISC_BY_UUID_SVC:
        
        if(param->status != CO_ERROR_NO_ERROR && param->status != ATT_ERR_ATTRIBUTE_NOT_FOUND )
        {
            printf("Error while trying to discover SPOTA service by UUID (status = 0x%02X) \n", param->status);

            // disconnect
            app_env.state = APP_DISCONNECTING;
            app_disconnect();
        }
        else
        {
            if (app_env.peer_device.spota.svc_found != 1)
            {
                printf("Peer device does not support the SPOTA service \n");

                // disconnect
                app_env.state = APP_DISCONNECTING;
                app_disconnect();
            }
            else
            {
                // SPOTA service was discovered. Now discover SPOTA characteristics.
                app_discover_all_char( KE_IDX_GET(src_id), app_env.peer_device.spota.svc_start_handle, app_env.peer_device.spota.svc_end_handle);
            }
        };
        break;


    case GATTC_DISC_ALL_CHAR:

        if(param->status != CO_ERROR_NO_ERROR && param->status != ATT_ERR_ATTRIBUTE_NOT_FOUND )
        {
            printf("Error while trying to discover SPOTA service characteristics (status = 0x%02X) \n", param->status);

            // disconnect
            app_env.state = APP_DISCONNECTING;
            app_disconnect();
        }
        else if ( app_env.peer_device.spota.mem_dev_handle == 0 ||
                  app_env.peer_device.spota.gpio_map_handle == 0 ||
                  app_env.peer_device.spota.mem_info_handle == 0 ||
                  app_env.peer_device.spota.patch_len_handle == 0 ||
                  app_env.peer_device.spota.patch_data_handle == 0 || 
                  app_env.peer_device.spota.serv_status_handle == 0 )
        {
            printf("Error: Missing characteristics in peer device's SPOTA service.\n");

            // disconnect
            app_env.state = APP_DISCONNECTING;
            app_disconnect();
        }
        else
        {
            // trigger next step - discover SPOTA_SERV_STATUS Client char descriptor
            app_discover_char_desc(KE_IDX_GET(src_id), app_env.peer_device.spota.serv_status_handle, app_env.peer_device.spota.svc_end_handle);
        }
        break;


    case GATTC_DISC_DESC_CHAR:

        if(param->status != CO_ERROR_NO_ERROR && param->status != ATT_ERR_ATTRIBUTE_NOT_FOUND )
        {
            printf("Error while trying to discover SPOTA_SERV_STATUS characteristic descriptors (status = 0x%02X) \n", param->status);

            // disconnect
            app_env.state = APP_DISCONNECTING;
            app_disconnect();
        }
        else if ( app_env.peer_device.spota.serv_status_cli_char_cfg_desc_handle == 0 )
        {
            printf("Error: Missing Client Char. Conf. descriptor for SPOTA_SERV_STATUS characteristic.\n");

            // disconnect
            app_env.state = APP_DISCONNECTING;
            app_disconnect();
        }
        else
        {
            // trigger next step - enable SPOTA_SERV_STATUS notifications
            uint8_t v[2];

            app_env.state = APP_EN_SERV_STATUS_NOTIFICATIONS;
            
            v[1] = 0x00;
            v[0] = 0x01;
            app_characteristic_write(KE_IDX_GET(src_id), app_env.peer_device.spota.serv_status_cli_char_cfg_desc_handle, v, 2);
        }
        break;

            
    case GATTC_WRITE:

        if(param->status != CO_ERROR_NO_ERROR) // write characteristic command failed
        {
            printf("error: failed to write a characteristic (status=0x%02X). Aborting... \n", param->status);
            // disconnect
            app_env.state = APP_DISCONNECTING;
            app_disconnect();
        }
        else if (app_env.state == APP_EN_SERV_STATUS_NOTIFICATIONS) // write to SPOTA_SERV_STATUS Client Char. Conf. descriptor completed successfully
        {            
            // trigger next step - write to SPOTA_MEM_DEV
            {
                uint8_t value[4];
                uint8_t value_length = 4;

                app_env.state = APP_WR_MEM_DEV;

                value[3] = spota_options.mem_type;
                value[2] = 0;
                value[1] = 0;
                value[0] = 0;
            
                switch(spota_options.mem_type)
                {
                    case SPOTA_MEM_DEV_SYSTEM_RAM:
                    case SPOTA_MEM_DEV_RETENTION_RAM:
                        break;

                    case SPOTA_MEM_DEV_I2C:
                        value[2] = 0;
                        value[1] = (spota_options.i2c_options.patch_base_address >> 8) & 0xFF; // Byte #1 of patch base address (MSB)
                        value[0] = (spota_options.i2c_options.patch_base_address     ) & 0xFF; // Byte #0 of patch base address
                        break;

                    case SPOTA_MEM_DEV_SPI:
                        value[2] = (spota_options.spi_options.patch_base_address >> 16) & 0xFF; // Byte #2 of patch base address (MSB)
                        value[1] = (spota_options.spi_options.patch_base_address >>  8) & 0xFF; // Byte #1 of patch base address
                        value[0] = (spota_options.spi_options.patch_base_address      ) & 0xFF; // Byte #0 of patch base address
                        break;

                    default:
                        break;
                }
                app_characteristic_write( KE_IDX_GET(src_id), app_env.peer_device.spota.mem_dev_handle, &value[0], value_length);
            }

        }
        else if (app_env.state == APP_WR_MEM_DEV) // write SPOTA_MEM_DEV completed successfully
        {
            // trigger next step - read SPOTA_SERV_STATUS 
            app_env.state = APP_WR_MEM_DEV_STATUS;
            app_characteristic_read(KE_IDX_GET(src_id), app_env.peer_device.spota.serv_status_handle);
        }
        else if (app_env.state == APP_WR_GPIO_MAP) // write SPOTA_GPIO_MAP completed successfully 
        {            
            // trigger next step - read SPOTA_SERV_STATUS 
            app_env.state = APP_WR_GPIO_MAP_STATUS;
            app_characteristic_read(KE_IDX_GET(src_id), app_env.peer_device.spota.serv_status_handle);
        }
        else if (app_env.state == APP_WR_PATCH_LEN) // write SPOTA_MEM_DEV completed successfully
        {            
            // trigger next step - read SPOTA_SERV_STATUS
            app_env.state = APP_WR_PATCH_LEN_STATUS;
            app_characteristic_read(KE_IDX_GET(src_id), app_env.peer_device.spota.serv_status_handle);
        }
        else if (app_env.state == APP_WR_PATCH_DATA) // write SPOTA_PATCH_DATA completed successfully
        {               
            // trigger next step - write next patch chunk if one is available
            {
                bool last_chunk = (patch_chunck_offset + patch_chunck_length) == patch_length;

                if (!last_chunk)
                {
                    // update next chunk offset and length
                    patch_chunck_offset += patch_chunck_length;
                    patch_chunck_length = (patch_length - patch_chunck_offset) > 20 ? 20 : (patch_length - patch_chunck_offset);

                    //
                    // trigger next step - write next chunk to SPOTA_PATCH_DATA
                    //
                    app_env.state  = APP_WR_PATCH_DATA;
                    app_characteristic_write( KE_IDX_GET(src_id), app_env.peer_device.spota.patch_data_handle, &patch_data[patch_chunck_offset], patch_chunck_length);

                }
                else
                {
                    // do nothing - wait for the SPOTA_SERV_STATUS notification
                }
            }

        }
        break;


    case GATTC_READ:
        
        if(param->status != CO_ERROR_NO_ERROR) // read characteristic command failed
        {
            printf("error: failed to read a characteristic. Aborting... \n");
            // disconnect
            app_env.state = APP_DISCONNECTING;
            app_disconnect();
        }
        else if (app_env.state == APP_WR_MEM_DEV_STATUS) // read SPOTA_SERV_STATUS completed successfully
        {                        
            // check SPOTA_SERV_STATUS
            if (app_env.peer_device.spota.serv_status_value != SPOTA_STATUS_SRV_STARTED)
            {
                printf("error: SPOTA_SERV_STATUS = 0x%02X after writing to SPOTA_MEM_DEV \n", app_env.peer_device.spota.serv_status_value);
                // disconnect
                app_env.state = APP_DISCONNECTING;
                app_disconnect();
                return 0; // in order to exit this function immediately
            }

            // trigger next step: 
            //   if memory device is I2C or SPI 
            //     then go to state APP_WR_GPIO_MAP
            //     else go to state APP_RD_MEM_INFO
            //
            if (spota_options.mem_type == SPOTA_MEM_DEV_SYSTEM_RAM || spota_options.mem_type == SPOTA_MEM_DEV_RETENTION_RAM )
            {
                app_env.state = APP_RD_MEM_INFO;
                app_characteristic_read(KE_IDX_GET(src_id), app_env.peer_device.spota.mem_info_handle);
            }
            else if (spota_options.mem_type == SPOTA_MEM_DEV_I2C)
            {
                uint8_t v[4];

                app_env.state = APP_WR_GPIO_MAP;

                v[3] = (spota_options.i2c_options.i2c_device_address >> 8 ) & 0xFF; // MSB of I2C device address
                v[2] = (spota_options.i2c_options.i2c_device_address      ) & 0xFF; // LSB of I2C device address
                v[1] = spota_options.i2c_options.scl_gpio;
                v[0] = spota_options.i2c_options.sda_gpio;
                
                app_characteristic_write(KE_IDX_GET(src_id), app_env.peer_device.spota.gpio_map_handle, v, 4);
            }
            else if (spota_options.mem_type == SPOTA_MEM_DEV_SPI)
            {
                uint8_t v[4];

                app_env.state = APP_WR_GPIO_MAP;

                v[3] = spota_options.spi_options.miso_gpio;
                v[2] = spota_options.spi_options.mosi_gpio;
                v[1] = spota_options.spi_options.cs_gpio;
                v[0] = spota_options.spi_options.sck_gpio;
                
                app_characteristic_write(KE_IDX_GET(src_id), app_env.peer_device.spota.gpio_map_handle, v, 4);
            }
            else
            {
                // should never happen
            }

        }
        else if (app_env.state == APP_WR_GPIO_MAP_STATUS) // read SPOTA_SERV_STATUS completed successfully
        {
            // check SPOTA_SERV_STATUS
            if (app_env.peer_device.spota.serv_status_value != SPOTA_STATUS_SRV_STARTED)
            {
                printf("error: SPOTA_SERV_STATUS = 0x%02X after writing to SPOTA_GPIO_MAP \n", app_env.peer_device.spota.serv_status_value);
                // disconnect
                app_env.state = APP_DISCONNECTING;
                app_disconnect();
                return 0; // in order to exit this function immediately
            }

            // trigger next step - read spota_mem_info operation
            app_env.state = APP_RD_MEM_INFO;
            app_characteristic_read(KE_IDX_GET(src_id), app_env.peer_device.spota.mem_info_handle);
        }
        else if (app_env.state == APP_RD_MEM_INFO) // read SPOTA_MEM_INFO completed  successfully
        {
            uint16_t applied_patches_count = (app_env.peer_device.spota.mem_info_value[3] << 8) | app_env.peer_device.spota.mem_info_value[2];
            uint16_t applied_patches_words = (app_env.peer_device.spota.mem_info_value[1] << 8) | app_env.peer_device.spota.mem_info_value[0];

            printf("Memory Information: \n");
            printf("\t number of patches = %d \n",  applied_patches_count); 
            printf("\t size of patches   = %d words (%d bytes) \n",  applied_patches_words, 4 * applied_patches_words); 

            // trigger next step - read status of read spota_mem_info operation
            app_env.state = APP_RD_MEM_INFO_STATUS;
            app_characteristic_read(KE_IDX_GET(src_id), app_env.peer_device.spota.serv_status_handle);
        }
        else if (app_env.state == APP_RD_MEM_INFO_STATUS) // read SPOTA_SERV_STATUS completed  successfully
        {
            // check SPOTA_SERV_STATUS
            if (app_env.peer_device.spota.serv_status_value != SPOTA_STATUS_SRV_STARTED)
            {
                printf("error: SPOTA_SERV_STATUS = 0x%02X after reading SPOTA_MEM_INFO \n", app_env.peer_device.spota.serv_status_value);
                // disconnect
                app_env.state = APP_DISCONNECTING;
                app_disconnect();
                return 0; // in order to exit this function immediately
            }

            // trigger next step - write SPOTA_PATCH_LEN
            app_env.state = APP_WR_PATCH_LEN;
            {
                uint8_t value[] = {patch_length & 0xFF, (patch_length >> 8 )& 0xFF };
                uint8_t value_length = 2;
                
                app_characteristic_write( KE_IDX_GET(src_id), app_env.peer_device.spota.patch_len_handle, &value[0], value_length);
            }
        }
        else if (app_env.state == APP_WR_PATCH_LEN_STATUS) // read SPOTA_SERV_STATUS completed successfully
        {
            // check SPOTA_SERV_STATUS
            if (app_env.peer_device.spota.serv_status_value != SPOTA_STATUS_SRV_STARTED)
            {
                printf("error: SPOTA_SERV_STATUS = 0x%02X after writing SPOTA_PATCH_LEN \n", app_env.peer_device.spota.serv_status_value);
                // disconnect
                app_env.state = APP_DISCONNECTING;
                app_disconnect();
                return 0; // in order to exit this function immediately
            }

            // init 1st chunk of patch data
            patch_chunck_offset = 0;
            patch_chunck_length = (patch_length - patch_chunck_offset) > 20 ? 20: (patch_length - patch_chunck_offset);

            // trigger next step - write SPOTA_PATCH_DATA
            app_env.state = APP_WR_PATCH_DATA;
            app_characteristic_write( KE_IDX_GET(src_id), app_env.peer_device.spota.patch_data_handle, &patch_data[patch_chunck_offset], patch_chunck_length);
        }
        else if (app_env.state == APP_RD_MEM_INFO_2) // read SPOTA_MEM_INFO completed successfully
        {
            uint16_t applied_patches_count = (app_env.peer_device.spota.mem_info_value[3] << 8) | app_env.peer_device.spota.mem_info_value[2];
            uint16_t applied_patches_words = (app_env.peer_device.spota.mem_info_value[1] << 8) | app_env.peer_device.spota.mem_info_value[0];

            printf("Memory Information: \n");
            printf("\t number of patches = %d \n",  applied_patches_count); 
            printf("\t size of patches   = %d words (%d bytes) \n",  applied_patches_words, 4 * applied_patches_words); 
            
            //
            // trigger next step - disconnect
            //
            app_env.state = APP_DISCONNECTING;
            app_disconnect();
        }

        break;

    default:
        break;
    }

    return 0;
}
