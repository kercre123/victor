/**
 ****************************************************************************************
 *
 * @file app_diss.c
 *
 * @brief Device Information Service Application entry point
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 *
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @addtogroup APP
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"     // SW configuration

#if (BLE_DIS_SERVER)
#include "app_diss.h"                 // Device Information Service Application Definitions
#include "app_diss_task.h"            // Device Information Service Application Task API
#include "app.h"                     // Application Definitions
#include "app_task.h"                // Application Task Definitions
#include "diss_task.h"               // Health Thermometer Functions
#include "app_prf_perm_types.h"

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

void app_dis_init(void)
{
}

void app_diss_create_db(void)
{
    // Add DIS in the database
    struct diss_create_db_req *req = KE_MSG_ALLOC(DISS_CREATE_DB_REQ,
                                                  TASK_DISS, TASK_APP,
                                                  diss_create_db_req);

    req->features = APP_DIS_FEATURES;

    // Send the message
    ke_msg_send(req);
}

void app_diss_enable(uint16_t conhdl)
{
    // Allocate the message
    struct diss_enable_req *req = KE_MSG_ALLOC(DISS_ENABLE_REQ,
                                               TASK_DISS, TASK_APP,
                                               diss_enable_req);

    // Fill in the parameter structure
    req->conhdl             = conhdl;
    req->sec_lvl            = get_user_prf_srv_perm(TASK_DISS);
    req->con_type           = PRF_CON_DISCOVERY;

    // Send the message
    ke_msg_send(req);
}

#endif // (BLE_DIS_SERVER)

/// @} APP
