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
                printf("app_exit() after GAPC_DISCONNECT_CMD completion\n");
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
        app_env.devices[app_env.num_of_devices].adv_addr_type = param->report.adv_addr_type;
        app_find_device_name(param->report.data,param->report.data_len, app_env.num_of_devices); 

        // check if this is our target device 
        if (bdaddr_compare(&param->report.adv_addr, &spota_options.target_spotar_bdaddr) )
        {
            app_env.target_idx = app_env.num_of_devices;
            printf("Target device was found. \n");

            // cancel scan operation
            app_cancel();
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

      // Start without pairing
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
        // unexpected disconnection
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
        case GAPC_TK_EXCH:
            app_gap_bond_tk_cfm(param->data.tk_type);
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

    if (app_env.state == APP_WR_MEM_DEV)
    {
        // check if write SPOTA_MEM_DEV completed successfully
        if (app_env.peer_device.spota.serv_status_value != SPOTA_STATUS_IMG_STARTED)
        {
                printf("error: SPOTA_SERV_STATUS = 0x%02X after writing to SPOTA_MEM_DEV\n", app_env.peer_device.spota.serv_status_value);
                // disconnect
                app_env.state = APP_DISCONNECTING;
                app_disconnect();
                return 0; // in order to exit this function immediately
        }

        // trigger next step - write SPOTA_GPIO_MAP
        app_suota_write_gpio_map();

    }
    else if (app_env.state == APP_WR_PATCH_DATA) // handle the notification only after the last chunk has been written
    {
        // check SPOTA_SERV_STATUS
        if (app_env.peer_device.spota.serv_status_value != SPOTA_STATUS_CMP_OK)
        {
            printf("Error: SPOTA_SERV_STATUS = 0x%02X after writing block \n", app_env.peer_device.spota.serv_status_value);
            // disconnect
            app_env.state = APP_DISCONNECTING;
            app_disconnect();
            return 0; // in order to exit this function immediately
        }

        if(is_last_block())
        {
            // no more blocks to write 

            // trigger next step - read SPOTA_MEM_INFO (the 2nd time)
            app_env.state = APP_RD_MEM_INFO_2;
            app_characteristic_read(KE_IDX_GET(src_id), app_env.peer_device.spota.mem_info_handle);
        }
        else
        {
            // advance to the next block

            next_block();

            if( is_last_block() ) // we may need a different block length for the last block
            {
                // trigger next step - write SPOTA_PATCH_LEN for last block
                app_suota_write_patch_len(block_length);
            }
            else
            {
                // trigger next step - start writing the block chunks
                app_suota_write_chunks();
            }
        }
    }

    else if (app_env.state == APP_WR_END_OF_SUOTA) // handle the notification after the end of SUOTA command has been issued
    {
        // check SPOTA_SERV_STATUS
        if (app_env.peer_device.spota.serv_status_value != SPOTA_STATUS_CMP_OK)
        {
            printf("error: SPOTA_SERV_STATUS = 0x%02X after sending END OF SUOTA \n", app_env.peer_device.spota.serv_status_value);
            // disconnect
            app_env.state = APP_DISCONNECTING;
            app_disconnect();
            return 0; // in order to exit this function immediately
        }

        // no more blocks to write - SUOTA procedure completed successfully
        printf("\nSUOTA procedure completed successfully \n\n");
            
        print_time();

        // send boot command - the receiver will reboot when it receives this command
        // the connection is expected to be lost
        {
            void app_suota_reboot(void);

            app_suota_reboot();
        }

        // trigger next step - disconnect
        app_env.state = APP_DISCONNECTING;
        app_disconnect();
    }
    else
    {
        // no handling of notifications in the rest of the states
        ///    printf(" SPOTA_SERV_STATUS = 0x%02X app_env.state = %d\n", app_env.peer_device.spota.serv_status_value, app_env.state );
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
    case GATTC_WRITE_NO_RESPONSE:

        if(param->status != CO_ERROR_NO_ERROR) // write characteristic command failed
        {
            if ((param->status == ATT_ERR_INSUFF_AUTHEN || param->status == ATT_ERR_INSUFF_ENC)
                && (start_pair == 1))
            {
                printf("warning: failed to write a characteristic (status=0x%02X). Pairing... \n", param->status);
                app_security_enable();
                start_pair = 0;
            }
            else
            {
                printf("error: failed to write a characteristic (status=0x%02X). Aborting... \n", param->status);
                // disconnect
                app_env.state = APP_DISCONNECTING;
                app_disconnect();
            }
        }
        else if (app_env.state == APP_EN_SERV_STATUS_NOTIFICATIONS) // write to SPOTA_SERV_STATUS Client Char. Conf. descriptor completed successfully
        {            
            // trigger next step - write to SPOTA_MEM_DEV
            app_suota_write_mem_dev();
        }
        else if (app_env.state == APP_WR_MEM_DEV) // write SPOTA_MEM_DEV completed successfully
        {
            // nothing to do - wait for notification on SPOTA_SERV_STATUS
        }
        else if (app_env.state == APP_WR_GPIO_MAP) // write SPOTA_GPIO_MAP completed successfully 
        {            
            // trigger next step - read spota_mem_info operation
            app_env.state = APP_RD_MEM_INFO;
            app_characteristic_read(KE_IDX_GET(src_id), app_env.peer_device.spota.mem_info_handle);
        }
        else if (app_env.state == APP_WR_PATCH_LEN) // write SPOTA_PATCH_LEN completed successfully
        {
            // init 1st chunk of block
            patch_chunck_offset = 0; // offset in block
            patch_chunck_length = (block_length - patch_chunck_offset) > 20 ? 20: (block_length - patch_chunck_offset);

            //
            // trigger next step - write SPOTA_PATCH_DATA
            //
            app_suota_write_chunks();
        }
        else if (app_env.state == APP_WR_PATCH_DATA) // write SPOTA_PATCH_DATA completed successfully
        {
            // sanity check - should necer happen
            if (expected_write_completion_events_counter <= 0 )
            {
                printf("Fatal error. Unexpected write completion \n");
                // disconnect
                app_env.state = APP_DISCONNECTING;
                app_disconnect();
                return 0;
            }

            --expected_write_completion_events_counter; // decrement expected write completions
        }

        break;

    case GATTC_READ:
        
        if(param->status != CO_ERROR_NO_ERROR) // read characteristic command failed
        {
            if ((param->status == ATT_ERR_INSUFF_AUTHEN || param->status == ATT_ERR_INSUFF_ENC)
                && (start_pair == 1))
            {
                printf("warning: failed to read a characteristic. (status=0x%02X). Pairing... \n", param->status);
                app_security_enable();
                start_pair = 0;
            }
            else
            {
                printf("error: failed to read a characteristic. Aborting... \n");
                // disconnect
                app_env.state = APP_DISCONNECTING;
                app_disconnect();
            }
        }

        else if (app_env.state == APP_RD_MEM_INFO) // read SPOTA_MEM_INFO completed  successfully
        {
            uint32_t mem_info = (app_env.peer_device.spota.mem_info_value[3] << 24) 
                              | (app_env.peer_device.spota.mem_info_value[2] << 16)
                              | (app_env.peer_device.spota.mem_info_value[1] << 8) 
                              | (app_env.peer_device.spota.mem_info_value[0])
                              ;

            printf("Memory Information: 0x%08X \n", mem_info);

            //
            // trigger next step - write SPOTA_PATCH_LEN of 1st block
            //

            // initialize block offset to point to the start of image
            block_offset = 0;
            block_length = (patch_length - block_offset) > suota_block_size 
                         ? suota_block_size
                         : (patch_length - block_offset);

            app_suota_write_patch_len(block_length);

            print_time();
        }

        else if (app_env.state == APP_RD_MEM_INFO_2) // read SPOTA_MEM_INFO completed successfully
        {
            uint32_t mem_info = (app_env.peer_device.spota.mem_info_value[3] << 24) 
                              | (app_env.peer_device.spota.mem_info_value[2] << 16)
                              | (app_env.peer_device.spota.mem_info_value[1] << 8) 
                              | (app_env.peer_device.spota.mem_info_value[0])
                              ;

            printf("Memory Information: 0x%08X \n", mem_info);
            
            //
            // trigger next step - send END OF SUOTA
            //
            app_suota_end();
        }

        break;

    default:
        break;
    }

    return 0;
}
