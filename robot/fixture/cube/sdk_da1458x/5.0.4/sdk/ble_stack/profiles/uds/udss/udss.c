/**
 ****************************************************************************************
 *
 * @file udss.c
 *
 * @brief User Data Service Server Implementation.
 *
 * Copyright (C) 2014. Dialog Semiconductor Ltd, unpublished work. This computer 
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
 * @addtogroup UDSS
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "rwip_config.h"

#if (BLE_UDS_SERVER)
#include "attm_util.h"
#include "atts_util.h"
#include "udss.h"
#include "udss_task.h"
#include "prf_utils.h"

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// User Data Service Server environment variable
struct udss_env_tag udss_env __attribute__((section("retention_mem_area0"),zero_init)); //@RETENTION MEMORY

/*
 * LOCAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// User Data Service task descriptor
static const struct ke_task_desc TASK_DESC_UDSS = {udss_state_handler, &udss_default_handler, udss_state, UDSS_STATE_MAX, UDSS_IDX_MAX};


/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

void udss_init(void)
{
    // Reset environment
    memset(&udss_env, 0, sizeof(udss_env));

    // Create UDSS task
    ke_task_create(TASK_UDSS, &TASK_DESC_UDSS);

    // Set task in disabled state
    ke_state_set(TASK_UDSS, UDSS_DISABLED);
}

void udss_disable(uint16_t conhdl) 
{
    // Inform the application about the disconnection
    struct udss_disable_ind *ind = KE_MSG_ALLOC(UDSS_DISABLE_IND,
                                                udss_env.con_info.appid, TASK_UDSS,
                                                udss_disable_ind);

    ind->conhdl = conhdl;

    ke_msg_send(ind);

    //Disable UDS in database
    attmdb_svc_set_permission(udss_env.shdl, PERM(SVC, DISABLE));

    //Go to idle state
    ke_state_set(TASK_UDSS, UDSS_IDLE);
}

uint8_t udss_unpack_ucp_req(uint8_t *packed_val, uint16_t length,
                                        struct uds_ucp_req* ucp_req)
{
    uint8_t cursor = 0;

    // Verify that enough data present
    if(length < 1)
    {
        return PRF_APP_ERROR;
    }

    // Retrieve function op code
    ucp_req->op_code = packed_val[cursor];
    cursor++;

    // Clear user control point parameter structure
    memset(&(ucp_req->parameter), 0, sizeof(ucp_req->parameter));

    // Check if op code is supported
    if((ucp_req->op_code < UDS_REQ_REG_NEW_USER)
            || (ucp_req->op_code > UDS_REQ_DEL_USER_DATA))
    {
        return UDS_RSP_OP_CODE_NOT_SUP;
    }

    // Delete user data operation doesn't require any other parameter
    if(ucp_req->op_code == UDS_REQ_DEL_USER_DATA)
    {
        return PRF_ERR_OK;
    }

    // Register new user function
    if(ucp_req->op_code == UDS_REQ_REG_NEW_USER)
    {
        // Check sufficient data available
        if((length - cursor) < 2)
        {        
            return UDS_RSP_INVALID_PARAMETER;
        }
        
        // Retrieve consent code
        ucp_req->parameter.reg_new_user.consent_code = co_read16p(packed_val + cursor);
        cursor +=2;
        
        if(ucp_req->parameter.reg_new_user.consent_code > UDS_CONSENT_CODE_MAX_VAL)
        {
            return UDS_RSP_INVALID_PARAMETER;
        }
    }

    // Consent function
    if(ucp_req->op_code == UDS_REQ_CONSENT)
    {
        // Check sufficient data available
        if((length - cursor) < 3)
        {        
            return UDS_RSP_INVALID_PARAMETER;
        }
        // Retrieve user index
        ucp_req->parameter.consent.user_idx = *(packed_val + cursor);
        cursor++;
        // Retrieve consent code
        ucp_req->parameter.consent.consent_code = co_read16p(packed_val + cursor);
        cursor +=2;

        if(ucp_req->parameter.consent.consent_code > UDS_CONSENT_CODE_MAX_VAL)
        {
            return UDS_RSP_INVALID_PARAMETER;
        }
    }

    // No errors
    return PRF_ERR_OK;
}

uint8_t udss_pack_ucp_rsp(uint8_t *packed_val,
                          struct uds_ucp_rsp* ucp_rsp)
{
    uint8_t cursor = 0;

    // Set response op code
    packed_val[cursor] = ucp_rsp->op_code;
    cursor++;

    // Set request op code
    packed_val[cursor] = ucp_rsp->req_op_code;
    cursor++;

    // Set response value
    packed_val[cursor] = ucp_rsp->rsp_val;
    cursor++;

    // Fill in parameter with user index for register new user function
    if((ucp_rsp->req_op_code == UDS_REQ_REG_NEW_USER)
       &&(ucp_rsp->rsp_val == UDS_RSP_SUCCESS))
    {
        *(packed_val+cursor) = ucp_rsp->parameter.reg_new_user.user_idx;
        cursor ++;
    }

    return cursor;
}

uint8_t udss_send_ucp_rsp(struct uds_ucp_rsp *ucp_rsp, uint16_t handle,
                                      ke_task_id_t ucp_ind_src)
{
    uint8_t status = PRF_ERR_IND_DISABLED;
    struct attm_elmt * attm_elmt;

    // Retrieve attdb data value pointers
    attm_elmt = attmdb_get_attribute(handle);
    
    // Pack data (updates database)
    attm_elmt->length = udss_pack_ucp_rsp(attm_elmt->value, ucp_rsp);
    
    // Send indication through GATT
    prf_server_send_event((prf_env_struct *)&udss_env, true, handle);
    
    status = PRF_ERR_OK;

    return status;
}

#endif //BLE_UDS_SERVER

/// @} UDSS
