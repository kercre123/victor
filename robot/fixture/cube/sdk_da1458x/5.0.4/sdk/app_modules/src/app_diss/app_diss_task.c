/**
 ****************************************************************************************
 *
 * @file app_diss_task.c
 *
 * @brief Device Information Service Application Task
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

#include "rwip_config.h"        // SW Configuration

#if (BLE_DIS_SERVER)

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "diss_task.h"          // Device Information Service Server Task API
#include "diss.h"               // Device Information Service Definitions
#include "app_diss.h"           // Device Information Service Application Definitions
#include "app_diss_task.h"      // Device Information Service Application Task API
#include "app_task.h"           // Application Task API
#include "app_entry_point.h"
#include "app.h"

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Handles DIS Server profile database creation confirmation.
 * @param[in] msgid     Id of the message received
 * @param[in] param     Pointer to the parameters of the message
 * @param[in] dest_id   ID of the receiving task instance (TASK_GAP)
 * @param[in] src_id    ID of the sending task instance
 * @return If the message was consumed or not
 ****************************************************************************************
 */
static int diss_create_db_cfm_handler(ke_msg_id_t const msgid,
                                      struct diss_create_db_cfm const *param,
                                      ke_task_id_t const dest_id,
                                      ke_task_id_t const src_id)
{
    uint8 len;

    if (ke_state_get(dest_id) == APP_DB_INIT)
    {
        if (param->status == CO_ERROR_NO_ERROR)
        {
            // Go to the idle state

            {
                // Set Manufacturer Name value in the DB
                struct diss_set_char_val_req *req_name = KE_MSG_ALLOC_DYN(DISS_SET_CHAR_VAL_REQ,
                                                                          TASK_DISS, TASK_APP,
                                                                          diss_set_char_val_req,
                                                                          APP_DIS_MANUFACTURER_NAME_LEN);

                // Fill in the parameter structure
                req_name->char_code     = DIS_MANUFACTURER_NAME_CHAR;
                req_name->val_len       = APP_DIS_MANUFACTURER_NAME_LEN;
                memcpy(&req_name->val[0], APP_DIS_MANUFACTURER_NAME, APP_DIS_MANUFACTURER_NAME_LEN);

                // Send the message
                ke_msg_send(req_name);
            }

            // Set Model Number String value in the DB
            {
                struct diss_set_char_val_req *req_mod = KE_MSG_ALLOC_DYN(DISS_SET_CHAR_VAL_REQ,
                                                                         TASK_DISS, TASK_APP,
                                                                         diss_set_char_val_req,
                                                                         APP_DIS_MODEL_NB_STR_LEN);

                // Fill in the parameter structure
                req_mod->char_code     = DIS_MODEL_NB_STR_CHAR;
                req_mod->val_len       = APP_DIS_MODEL_NB_STR_LEN;
                memcpy(&req_mod->val[0], APP_DIS_MODEL_NB_STR, APP_DIS_MODEL_NB_STR_LEN);

                // Send the message
                ke_msg_send(req_mod);
            }

            // Set System ID value in the DB
            {
                struct diss_set_char_val_req *req_id = KE_MSG_ALLOC_DYN(DISS_SET_CHAR_VAL_REQ,
                                                                        TASK_DISS, TASK_APP,
                                                                        diss_set_char_val_req,
                                                                        APP_DIS_SYSTEM_ID_LEN);

                // Fill in the parameter structure
                req_id->char_code     = DIS_SYSTEM_ID_CHAR;
                req_id->val_len       = APP_DIS_SYSTEM_ID_LEN;
                memcpy(&req_id->val[0], APP_DIS_SYSTEM_ID, APP_DIS_SYSTEM_ID_LEN);

                // Send the message
                ke_msg_send(req_id);
            }

            // Set the software version in the DB. This is the reference design sw version
            {
                len = strlen(APP_DIS_SW_REV);
                struct diss_set_char_val_req *req_id = KE_MSG_ALLOC_DYN(DISS_SET_CHAR_VAL_REQ,
                                                                        TASK_DISS, TASK_APP,
                                                                        diss_set_char_val_req,
                                                                        len);

                // Fill in the parameter structure
                req_id->char_code     = DIS_SW_REV_STR_CHAR;
                req_id->val_len       = len;
                memcpy(&req_id->val[0], APP_DIS_SW_REV, len);

                // Send the message
                ke_msg_send(req_id);
            }

            // Set the firmware version in the DB. This is the common code sw version
            {
                len = strlen(APP_DIS_FIRM_REV);
                struct diss_set_char_val_req *req_id = KE_MSG_ALLOC_DYN(DISS_SET_CHAR_VAL_REQ,
                                                                        TASK_DISS, TASK_APP,
                                                                        diss_set_char_val_req,
                                                                        len);

                // Fill in the parameter structure
                req_id->char_code     = DIS_FIRM_REV_STR_CHAR;
                req_id->val_len       = len;
                memcpy(&req_id->val[0], APP_DIS_FIRM_REV, len);

                // Send the message
                ke_msg_send(req_id);
            }

            // Set the PNP ID in the DB
            {
                uint16 ids[3];
                struct diss_set_char_val_req *req_id = KE_MSG_ALLOC_DYN(DISS_SET_CHAR_VAL_REQ,
                                                                        TASK_DISS, TASK_APP,
                                                                        diss_set_char_val_req,
                                                                        DIS_PNP_ID_LEN);

                // Fill in the parameter structure
                req_id->char_code     = DIS_PNP_ID_CHAR;
                req_id->val_len       = DIS_PNP_ID_LEN;
                req_id->val[0]        = APP_DISS_PNP_COMPANY_ID_TYPE;
                ids[0]                = APP_DISS_PNP_VENDOR_ID;
                ids[1]                = APP_DISS_PNP_PRODUCT_ID;
                ids[2]                = APP_DISS_PNP_PRODUCT_VERSION;
                memcpy(&req_id->val[1], ids, 6);

                // Send the message
                ke_msg_send(req_id);
            }
        }

        // Inform the Application Manager
        struct app_module_init_cmp_evt *cfm = KE_MSG_ALLOC(APP_MODULE_INIT_CMP_EVT,
                                                           TASK_APP, TASK_APP,
                                                           app_module_init_cmp_evt);

        cfm->status = param->status;

        ke_msg_send(cfm);
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles disable indication from the DIS Server profile.
 * @return If the message was consumed or not
 ****************************************************************************************
 */
static int diss_disable_ind_handler(ke_msg_id_t const msgid,
                                    struct diss_disable_ind const *param,
                                    ke_task_id_t const dest_id,
                                    ke_task_id_t const src_id)
{
    return (KE_MSG_CONSUMED);
}

/*
 * GLOBAL VARIABLES DEFINITION
 ****************************************************************************************
 */

static const struct ke_msg_handler app_diss_process_handlers[]=
{
    {DISS_CREATE_DB_CFM,                    (ke_msg_func_t)diss_create_db_cfm_handler},
    {DISS_DISABLE_IND,                      (ke_msg_func_t)diss_disable_ind_handler},
};

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

enum process_event_response app_diss_process_handler(ke_msg_id_t const msgid,
                                                     void const *param,
                                                     ke_task_id_t const dest_id,
                                                     ke_task_id_t const src_id,
                                                     enum ke_msg_status_tag *msg_ret)
{
    return (app_std_process_event(msgid, param, src_id, dest_id, msg_ret, app_diss_process_handlers,
                                         sizeof(app_diss_process_handlers) / sizeof(struct ke_msg_handler)));
}

#endif //(BLE_DIS_SERVER)

/// @} APP
