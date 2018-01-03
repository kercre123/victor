/**
 ****************************************************************************************
 *
 * @file app_customs.c
 *
 * @brief Custom profiles application file.
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
#include "app_customs.h"
#include "custs1_task.h"
#include "custs2_task.h"
#include "attm_db.h"
#include "attm_db_128.h"
#include "gapc.h"
#include "prf_types.h"
#include "app_prf_types.h"
#include "app_prf_perm_types.h"
#include "user_custs_config.h"

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */
 
 /**
 ****************************************************************************************
 * @brief Initialize custom1 application.
 * @return void
 ****************************************************************************************
 */
#if (BLE_CUSTOM1_SERVER)
void app_custs1_init(void)
{
    return;
}

/**
 ****************************************************************************************
 * @brief Create custom1 profile database.
 * @return void
 ****************************************************************************************
 */
void app_custs1_create_db(void)
{
    // Add Custom Service in the database
    struct custs1_create_db_req *req = KE_MSG_ALLOC(CUSTS1_CREATE_DB_REQ,
                                                    TASK_CUSTS1,
                                                    TASK_APP,
                                                    custs1_create_db_req);
    
    uint8_t i = 0;
    
    // max number of service characteristics
    while(cust_prf_funcs[i].task_id != TASK_NONE)
    {
        if(cust_prf_funcs[i].task_id == TASK_CUSTS1)
        {
            req->max_nb_att = cust_prf_funcs[i].max_nb_att;
            break;
        } else i++;
    }
    
    // Attribute table. In case the handle offset needs to be saved
    req->att_tbl = NULL;
    req->cfg_flag = 0;
    req->features = 0;

    // Send the message
    ke_msg_send(req);
}

/**
 ****************************************************************************************
 * @brief  Enables custom1 profile.
 * @param[in] conhdl    Connection handle
 * @return void
 ****************************************************************************************
 */
void app_custs1_enable(uint16_t conhdl)
{
    // Allocate the message
    struct custs1_enable_req *req = KE_MSG_ALLOC(CUSTS1_ENABLE_REQ,
                                                 TASK_CUSTS1,
                                                 TASK_APP,
                                                 custs1_enable_req);

    // Fill in the parameter structure
    req->conhdl             = conhdl;
    req->sec_lvl            = get_user_prf_srv_perm(TASK_CUSTS1);

    // Send the message
    ke_msg_send(req);

}
#endif //BLE_CUSTOM1_SERVER

/**
 ****************************************************************************************
 * @brief Initialize custom2 application.
 * @return void
 ****************************************************************************************
 */
#if (BLE_CUSTOM2_SERVER)
void app_custs2_init(void)
{
    return;
}

/**
 ****************************************************************************************
 * @brief Create custom2 profile database.
 * @return void
 ****************************************************************************************
 */
void app_custs2_create_db(void)
{
    // Add Custom Service in the database
    struct custs2_create_db_req *req = KE_MSG_ALLOC(CUSTS2_CREATE_DB_REQ,
                                                    TASK_CUSTS2,
                                                    TASK_APP,
                                                    custs2_create_db_req);
    
    uint8_t i = 0;
    
    // max number of service characteristics
    while(cust_prf_funcs[i].task_id != TASK_NONE)
    {
        if(cust_prf_funcs[i].task_id == TASK_CUSTS2)
        {
            req->max_nb_att = cust_prf_funcs[i].max_nb_att;
            break;
        } else i++;
    }
    
    // Attribute table. In case the handle offset needs to be saved
    req->att_tbl = NULL;
    req->cfg_flag = 0;
    req->features = 0;

    // Send the message
    ke_msg_send(req);
}

/**
 ****************************************************************************************
 * @brief  Enables custom2 profile.
 * @param[in] conhdl    Connection handle
 * @return void
 ****************************************************************************************
 */
void app_custs2_enable(uint16_t conhdl)
{
    // Allocate the message
    struct custs2_enable_req *req = KE_MSG_ALLOC(CUSTS2_ENABLE_REQ,
                                                 TASK_CUSTS2,
                                                 TASK_APP,
                                                 custs2_enable_req);

    // Fill in the parameter structure
    req->conhdl             = conhdl;
    req->sec_lvl            = get_user_prf_srv_perm(TASK_CUSTS2);

    // Send the message
    ke_msg_send(req);

}
#endif // (BLE_CUSTOM2_SERVER)
#endif // (BLE_CUSTOM_SERVER)
