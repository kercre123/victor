/**
 ****************************************************************************************
 *
 * @file customs_common.c
 *
 * @brief Custom Service profile task source file.
 *
 * Copyright (C) 2015. Dialog Semiconductor Ltd, unpublished work. This computer 
 * program includes Confidential, Proprietary Information and is a Trade Secret of 
 * Dialog Semiconductor Ltd.  All use, disclosure, and/or reproduction is prohibited 
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
 #include "rwble_config.h"              // SW configuration
 #if (BLE_CUSTOM_SERVER)
//#include "custs1.h"
#include "gattc_task.h"
#include "att.h"
#include "attm_db.h"
#include "prf_types.h"
/*
 * DEFINES
 ****************************************************************************************
 */
 
 /*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */
 
 
/**
 ****************************************************************************************
 * @brief Validates the value of the Client Characteristic CFG.
 * @param[in] bool  indicates whether the CFG is Notification (true) or Indication (false)
 * @param[in] param Pointer to the parameters of the message.
 * @return status.
 ****************************************************************************************
 */

int check_client_char_cfg(bool is_notification, struct gattc_write_cmd_ind const *param)
{
    uint8_t status = PRF_ERR_OK;
    uint16_t ntf_cfg = 0;
    
    if (param->length != sizeof(uint16_t))
    {
        status =  ATT_ERR_INVALID_ATTRIBUTE_VAL_LEN;
    }
    else
    {
        ntf_cfg = *((uint16_t*)param->value);
        
        if (is_notification)
        {
            if ( (ntf_cfg != PRF_CLI_STOP_NTFIND) && (ntf_cfg != PRF_CLI_START_NTF) )
            {
                status =  PRF_ERR_INVALID_PARAM;
            }
        }
        else
        {
            if ( (ntf_cfg != PRF_CLI_STOP_NTFIND) && (ntf_cfg != PRF_CLI_START_IND) )
            {
                status =  PRF_ERR_INVALID_PARAM;
            }
        }
    }
    
    return status;
}

/**
 ****************************************************************************************
 * @brief  Find the handle of the Characteristic Value having as input the Client Characteristic CFG handle
 * @param[in] cfg_handle  the Client Characteristic CFG handle
 * @return the coresponding value handle
 ****************************************************************************************
 */

uint16_t get_value_handle( uint16_t cfg_handle )
{
    uint8_t uuid[GATT_UUID_128_LEN];
    uint8_t uuid_len;
    uint16_t handle = cfg_handle;
    struct attm_svc_db *srv;
    
    srv = attmdb_get_service(handle);
    
    /* Iterate back the database to find the  characteristic declaration. 
    ** According to spec (3.3 CHARACTERISTIC DEFINITION): 
    ** "The Characteristic Value declaration shall exist immediately following
    ** the characteristic declaration"
    */
    while( (handle >= srv->start_hdl) && (handle <= srv->last_hdl)  )
    {
        // Retrieve UUID
        attmdb_att_get_uuid(handle, &uuid_len, &(uuid[0]));
        
        // check for Characteristic declaration
        if ((uint16_t)*(uint16_t *)&uuid[0] == ATT_DECL_CHARACTERISTIC)
            return( handle + 1);
        
        handle--;
    }
    
    return 0;  //Should not reach this point. something is wrong with the database
}

/**
 ****************************************************************************************
 * @brief  Find the handle of Client Characteristic CFG having as input the Characteristic value handle
 * @param[in] value_handle  the Characteristic value handle
 * @param[in] max_handle    the last handle of the service
 * @return the coresponding Client Characteristic CFG handle
 ****************************************************************************************
 */
uint16_t get_cfg_handle( uint16_t value_handle, uint16_t last_handle )
{
    uint8_t uuid[GATT_UUID_128_LEN];
    uint8_t uuid_len;
    uint16_t handle = value_handle;
    struct attm_svc_db *srv;
    
    srv = attmdb_get_service(handle);
    
    /* Iterate the database to find the client characteristic configuration. 
    */
    while( (handle >= srv->start_hdl) && (handle <= srv->last_hdl)  )
    {
        // Retrieve UUID
        attmdb_att_get_uuid(handle, &uuid_len, &(uuid[0]));
            
        // check for Client Characteristic Configuration
        if ((uint16_t)*(uint16_t *)&uuid[0] == ATT_DESC_CLIENT_CHAR_CFG) 
            return( handle );
        else if ((uint16_t)*(uint16_t *)&uuid[0] == ATT_DECL_CHARACTERISTIC)
            break; // found the next Characteristic declaration without findig a CC CFG, 
                    
        handle++;
    }
    
    return 0;  //Should not reach this point. something is wrong with the database
}
#endif // (BLE_CUSTOMS_SERVER)
