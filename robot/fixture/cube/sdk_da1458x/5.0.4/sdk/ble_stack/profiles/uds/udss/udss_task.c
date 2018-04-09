/**
 ****************************************************************************************
 *
 * @file udss_task.c
 *
 * @brief User Data Service Task implementation.
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
 * @addtogroup UDSSTASK
 * @{
 ****************************************************************************************
 */


/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "rwip_config.h"

#if (BLE_UDS_SERVER)

#include "gap.h"
#include "gattc_task.h"
#include "uds_common.h"
#include "udss.h"
#include "udss_task.h"
#include "prf_utils.h"
#include "attm_util.h"
#include "atts_util.h"
#include "arch_patch.h"

/*
 * DEFINES
 ****************************************************************************************
 */

#define UDSS_NO_USER            0xFF

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

struct uds_char_def
{
    uint16_t uuid; // the UDS characteristic UUID
    uint16_t max_val_length;
    uint16_t charcode;
};

/*
 * UDS ATTRIBUTES
 ****************************************************************************************
 */

const struct uds_char_def uds_char_def_table[] =
{
    // Optional Characteristics - At least one of them is mandatory
    {ATT_CHAR_UDS_USER_FIRST_NAME,              UDS_VAL_MAX_LEN,    UDS_USER_FIRST_NAME_CHAR},
    {ATT_CHAR_UDS_USER_LAST_NAME,               UDS_VAL_MAX_LEN,    UDS_USER_LAST_NAME_CHAR},
    {ATT_CHAR_UDS_USER_EMAIL_ADDR,              UDS_VAL_MAX_LEN,    UDS_USER_EMAIL_ADDRESS_CHAR},
    {ATT_CHAR_UDS_USER_AGE,                     sizeof(uint8_t),    UDS_USER_AGE_CHAR},
    {ATT_CHAR_UDS_USER_DATE_OF_BIRTH,           sizeof(struct uds_date),   UDS_USER_DATE_OF_BIRTH_CHAR},
    {ATT_CHAR_UDS_USER_GENDER,                  sizeof(uint8_t),    UDS_USER_GENDER_CHAR},
    {ATT_CHAR_UDS_USER_WEIGHT,                  sizeof(uint16_t),   UDS_USER_WEIGHT_CHAR},
    {ATT_CHAR_UDS_USER_HEIGHT,                  sizeof(uint16_t),   UDS_USER_HEIGHT_CHAR},
    {ATT_CHAR_UDS_USER_VO2_MAX,                 sizeof(uint8_t),    UDS_USER_VO2_MAX_CHAR},
    {ATT_CHAR_UDS_USER_HEART_RATE_MAX,          sizeof(uint8_t),    UDS_USER_HEART_RATE_MAX_CHAR},
    {ATT_CHAR_UDS_USER_RESTING_HEART_RATE,      sizeof(uint8_t),    UDS_USER_RESTING_HEART_RATE_CHAR},
    {ATT_CHAR_UDS_USER_MAX_REC_HEART_RATE,      sizeof(uint8_t),    UDS_USER_MAX_RECOMMENDED_HEART_RATE_CHAR},
    {ATT_CHAR_UDS_USER_AEROBIC_THRESHOLD,       sizeof(uint8_t),    UDS_USER_AEROBIC_THRESHOLD_CHAR},
    {ATT_CHAR_UDS_USER_ANAEROBIC_THRESHOLD,     sizeof(uint8_t),    UDS_USER_ANAEROBIC_THRESHOLD_CHAR},
    {ATT_CHAR_UDS_USER_THRESHOLDS_SPORT_TYPE,   sizeof(uint8_t),    UDS_USER_SPORT_TYPE_CHAR},
    {ATT_CHAR_UDS_USER_DATE_OF_THRESHOLD_ASS,   sizeof(uint32_t),   UDS_USER_DATE_OF_THRESHOLD_ASSESSMENT_CHAR},
    {ATT_CHAR_UDS_USER_WAIST_CIRCUMFERENCE,     sizeof(uint16_t),   UDS_USER_WAIST_CIRCUMFERENCE_CHAR},
    {ATT_CHAR_UDS_USER_HIP_CIRCUMFERENCE,       sizeof(uint16_t),   UDS_USER_HIP_CIRCUMFERENCE_CHAR},
    {ATT_CHAR_UDS_USER_FAT_BURN_HR_LOW_LIM,     sizeof(uint8_t),    UDS_USER_FAT_BURN_HEART_RATE_LOW_LIMIT_CHAR},
    {ATT_CHAR_UDS_USER_FAT_BURN_HR_UP_LIM,      sizeof(uint8_t),    UDS_USER_FAT_BURN_HEART_RATE_UP_LIMIT_CHAR},
    {ATT_CHAR_UDS_USER_AEROBIC_HR_LOW_LIM,      sizeof(uint8_t),    UDS_USER_AEROBIC_HEART_RATE_LOW_LIMIT_CHAR},
    {ATT_CHAR_UDS_USER_AEROBIC_HR_UP_LIM,       sizeof(uint8_t),    UDS_USER_AEROBIC_HEART_RATE_UP_LIMIT_CHAR},
    {ATT_CHAR_UDS_USER_ANAEROBIC_HR_LOW_LIM,    sizeof(uint8_t),    UDS_USER_ANEROBIC_HEART_RATE_LOW_LIMIT_CHAR},
    {ATT_CHAR_UDS_USER_ANAEROBIC_HR_UP_LIM,     sizeof(uint8_t),    UDS_USER_ANEROBIC_HEART_RATE_UP_LIMIT_CHAR},
    {ATT_CHAR_UDS_USER_5ZONE_HR_LIM,            sizeof(uint32_t),   UDS_USER_FIVE_ZONE_HEART_RATE_LIMITS_CHAR},
    {ATT_CHAR_UDS_USER_3ZONE_HR_LIM,            sizeof(uint16_t),   UDS_USER_THREE_ZONE_HEART_RATE_LIMITS_CHAR},
    {ATT_CHAR_UDS_USER_2ZONE_HR_LIM,            sizeof(uint8_t),    UDS_USER_TWO_ZONE_HEART_RATE_LIMIT_CHAR},
    {ATT_CHAR_UDS_USER_LANGUAGE,                sizeof(uint16_t),   UDS_USER_LANGUAGE_CHAR},

    // Mandatory Characteristics
    {ATT_CHAR_UDS_USER_DB_CHANGE_INCR,          sizeof(uint32_t),   UDS_USER_DB_CHANGE_INCR_CHAR},
    {ATT_DESC_CLIENT_CHAR_CFG,                  sizeof(uint16_t),   UDS_USER_DB_CHANGE_INCR_CFG},
    {ATT_CHAR_UDS_USER_INDEX,                   sizeof(uint8_t),    UDS_USER_INDEX_CHAR},
    {ATT_CHAR_UDS_USER_CTRL_PT,                 19,                 UDS_USER_CTRL_POINT_CHAR},
    {ATT_DESC_CLIENT_CHAR_CFG,                  sizeof(uint16_t),   UDS_USER_CTRL_POINT_CFG},
};

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Helper function to calculates the attribute count and database size.
 * @param[in] cfg_flag Configuration flag indicating optional characteristics.
 * @param[out] total_size Value pointer to store database size.
 * @return Number of attributes that should be allocated in attribute database.
 ****************************************************************************************
 */
static uint8_t udss_count_attributes(uint32_t cfg_flag, uint16_t *total_size)
{
    uint8_t offset = 0;
    uint8_t nb_att = 0;
    uint32_t mask = 1;

    // Count Service declaration handle and size
    nb_att += 1;
    *total_size = sizeof(att_svc_desc_t);

    // Count optional characteristics
    for (offset = 0; offset < UDS_USER_DB_CHANGE_INCR_CHAR; offset++)
    {
        if ((cfg_flag & mask) == mask)
        {
            // Count declaration handle and value handle
            nb_att += 2;
            // Count declaration and value size
            *total_size += sizeof(struct att_char_desc);
            *total_size += uds_char_def_table[offset].max_val_length;
        }
        mask = mask << 1;
    }

    // At least one is expected to be enabled
    if (nb_att == 1)
        return 0;

    // Count mandatory Database Change Increment Characteristic
    *total_size += sizeof(struct att_char_desc);
    *total_size += sizeof(uint32_t);
    // Count also the CCC descriptor
    *total_size += sizeof(uint16_t);
    nb_att += 3;

    // Count mandatory User Index Characteristic
    *total_size += sizeof(struct att_char_desc);
    *total_size += sizeof(uint8_t);
    nb_att += 2;

    // Count mandatory User Control Point Characteristic
    *total_size += sizeof(struct att_char_desc);
    *total_size += 19;
    // Count also the CCC descriptor
    *total_size += sizeof(uint16_t);
    nb_att += 3;

    return nb_att;
}

/**
 ****************************************************************************************
 * @brief Helper function to add 16-bit UUID Characteristics
 * @param[in] shdl Starting handle of the service
 * @param[in] offset used to store the attribute handle in attribute table
 * @param[in] uuid16 16-bit UUID of added characteristic
 * @param[in] max_val_length Length of characteristic's value
 * @param[in] perm Characteristic permissions
 * @param[in] props Characteristic properties
 * @return Status with error code in case of failure.
 ****************************************************************************************
 */
static uint8_t udss_add_char16(uint16_t *shdl, uint8_t offset, uint8_t *uuid16,
                                    att_size_t max_val_length, uint16_t perm, uint8_t props)
{
    struct att_char_desc char_desc;
    uint8_t buf[max_val_length];
    uint8_t status = PRF_ERR_OK;
    uint16_t val_handle;
    uint16_t handle;
    uint16_t uuid;

    memset(buf, 0, max_val_length);

    // UUID +3 for value handle (2 octets) and properties (1 octet)
    uuid = ATT_DECL_CHARACTERISTIC;
    status = attmdb_add_attribute(*shdl, ATT_UUID_16_LEN + 3,
                                    ATT_UUID_16_LEN, (uint8_t *) &uuid,
                                    PERM(RD, ENABLE), &handle);
    if (status != PRF_ERR_OK)
        return status;

    // Store the characteristic handle
    udss_env.att_tbl[offset] = handle;

    // Characteristic value
    status = attmdb_add_attribute(*shdl, max_val_length, ATT_UUID_16_LEN, uuid16,
                                                        perm, &val_handle);
    if (status != PRF_ERR_OK)
        return status;

    status = attmdb_att_set_value(val_handle, max_val_length, buf);
    if (status != PRF_ERR_OK)
        return status;

    // Set proper value to the characteristic declaration
    // and connect it to the value attribute
    char_desc.prop = props;
    memcpy(char_desc.attr_hdl, &val_handle, sizeof(uint16_t));
    memcpy(char_desc.attr_type, uuid16, ATT_UUID_16_LEN);

    status = attmdb_att_set_value(handle, sizeof(char_desc), (uint8_t *) &char_desc);

    return status;
}

/**
 ****************************************************************************************
 * @brief Helper function used to prepare the attribute database
 * @param[in] shdl Starting handle of the service
 * @param[in] cfg_flag Database configuration flag
 * @param[out] att_tbl Table to store added attribute handles
 * @param[in] dest_id ID of the current task
 * @return Status with error code in case of failure.
 ****************************************************************************************
 */
static uint8_t udss_svc_create_db(uint16_t *shdl, uint32_t cfg_flag,
                                    uint8_t *att_tbl, ke_task_id_t const dest_id)
{
    uint8_t status = PRF_ERR_OK;
    uint16_t total_size;
    uint8_t nb_att = 0;
    uint8_t offset = 0;
    uint32_t mask = 1;
    uint16_t uuid;
    uint16_t handle;

    // Count all handles and and verify if at least one optional char. is enabled
    nb_att = udss_count_attributes(cfg_flag, &total_size);
    if (nb_att == 0)
        return PRF_ERR_INVALID_PARAM;

    // Add Service into the Database
    status = attmdb_add_service(shdl, dest_id, nb_att, 0, 0, total_size);
    if (status != PRF_ERR_OK)
        return status;

    // Register task for read request indications
    dg_register_task_for_read_request(dest_id);

    // Store end handle
    udss_env.ehdl = *shdl + nb_att - 1;

    // Add service definition
    uuid = ATT_DECL_PRIMARY_SERVICE,
    status = attmdb_add_attribute(*shdl, ATT_UUID_16_LEN,
                                            ATT_UUID_16_LEN, (uint8_t *) &uuid,
                                            PERM(RD, ENABLE), &handle);
    if (status != PRF_ERR_OK)
        return status;

    // Set Service definition value
    uuid = ATT_SVC_USER_DATA;
    status = attmdb_att_set_value(*shdl, ATT_UUID_16_LEN, (uint8_t *) &uuid);
    if (status != PRF_ERR_OK)
        return status;

    // Add optional characteristics and values
    for (offset = 0; offset < UDS_USER_DB_CHANGE_INCR_CHAR; offset++)
    {
        if ((cfg_flag & mask) == mask)
        {
            status = udss_add_char16(shdl, offset, (uint8_t *) &uds_char_def_table[offset].uuid,
                                            uds_char_def_table[offset].max_val_length,
                                            PERM(RD, ENABLE) | PERM(WR, ENABLE),
                                            ATT_CHAR_PROP_RD | ATT_CHAR_PROP_WR);
            if (status != PRF_ERR_OK)
                return status;
        }
        mask = mask << 1;
    }

    // Add Mandatory Characteristics

    // Add UDS_USER_DB_CHANGE_INCR_CHAR
    offset = UDS_USER_DB_CHANGE_INCR_CHAR;
    status = udss_add_char16(shdl, offset, (uint8_t *) &uds_char_def_table[offset].uuid,
                                uds_char_def_table[offset].max_val_length,
                                PERM(RD, ENABLE) | PERM(WR, ENABLE) | PERM(NTF, ENABLE),
                                ATT_CHAR_PROP_RD | ATT_CHAR_PROP_WR | ATT_CHAR_PROP_NTF);
    if (status != PRF_ERR_OK)
        return status;

    // Add UDS_USER_DB_CHANGE_INCR_CHAR CCC descriptor
    offset = UDS_USER_DB_CHANGE_INCR_CFG;
    status = attmdb_add_attribute(*shdl, uds_char_def_table[offset].max_val_length,
                                    ATT_UUID_16_LEN, (uint8_t *) &uds_char_def_table[offset].uuid,
                                    PERM(RD, ENABLE) | PERM(WR, ENABLE),
                                    &handle);
    if (status != PRF_ERR_OK)
        return status;

    // Store the descriptor handle
    att_tbl[UDS_USER_DB_CHANGE_INCR_CFG] = handle;

    // UDS_USER_INDEX_CHAR
    offset = UDS_USER_INDEX_CHAR;
    status = udss_add_char16(shdl, offset, (uint8_t *) &uds_char_def_table[offset].uuid,
                                    uds_char_def_table[offset].max_val_length,
                                    PERM(RD, ENABLE),
                                    ATT_CHAR_PROP_RD);
    if (status != PRF_ERR_OK)
        return status;

    // Set initial value for user index to UDSS_NO_USER
    uint8_t val = UDSS_NO_USER;
    status = attmdb_att_set_value(att_tbl[UDS_USER_INDEX_CHAR] + 1,
                                                        sizeof(uint8_t), &val);
    if (status != PRF_ERR_OK)
        return status;

    // UDS_USER_CTRL_POINT_CHAR
    offset = UDS_USER_CTRL_POINT_CHAR;
    status = udss_add_char16(shdl, offset, (uint8_t *) &uds_char_def_table[offset].uuid,
                                    uds_char_def_table[offset].max_val_length,
                                    PERM(WR, ENABLE) | PERM(IND, ENABLE),
                                    ATT_CHAR_PROP_WR | ATT_CHAR_PROP_IND);
    if (status != PRF_ERR_OK)
        return status;

    // Add UDS_USER_CTRL_POINT_CHAR CCC descriptor
    offset = UDS_USER_CTRL_POINT_CFG;
    status = attmdb_add_attribute(*shdl, uds_char_def_table[offset].max_val_length,
                                    ATT_UUID_16_LEN, (uint8_t *) &uds_char_def_table[offset].uuid,
                                    (PERM(RD, ENABLE) | PERM(WR, ENABLE)),
                                    &handle);
    if (status != PRF_ERR_OK)
        return status;

    // Store the descriptor handle
    att_tbl[UDS_USER_CTRL_POINT_CFG] = handle;

    return status;
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref UDSS_CREATE_DB_REQ message.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int udss_create_db_req_handler(ke_msg_id_t const msgid,
                                      struct udss_create_db_req const *param,
                                      ke_task_id_t const dest_id,
                                      ke_task_id_t const src_id)
{
    // DB Creation Status
    uint8_t status;

    status = udss_svc_create_db(&udss_env.shdl, param->features,
                                                &udss_env.att_tbl[0], dest_id);

    //Save profile id
    udss_env.con_info.prf_id = TASK_UDSS;

    //Send response to application
    struct udss_create_db_cfm * cfm = KE_MSG_ALLOC(UDSS_CREATE_DB_CFM, src_id, TASK_UDSS,
                                                   udss_create_db_cfm);
    // Configure notifications for the Database Change Increment
    if (status == PRF_ERR_OK)
    {
        uint16_t ccc_val = PRF_CLI_START_NTF;

        attmdb_att_set_value(udss_env.att_tbl[UDS_USER_DB_CHANGE_INCR_CFG],
                                        sizeof(uint16_t), (uint8_t *) &ccc_val);

        // Disable UDSS
        attmdb_svc_set_permission(udss_env.shdl, PERM(SVC, DISABLE));

        ke_state_set(TASK_UDSS, UDSS_IDLE);
    }

    cfm->status = status;
    ke_msg_send(cfm);

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Helper function used to read and compare CCC descriptor values.
 * @param[in] charcode Code/Offset of the characteristic having CCC descriptor
 * @param[in] ccc_val Value of the descriptor that is verified
 * @return Status with error code in case of failure.
 ****************************************************************************************
 */
static uint8_t uds_verify_ccc(uint16_t charcode, uint16_t ccc_val)
{
    uint8_t *value;
    uint16_t length;

    // Check if CCC descriptor is properly configured
    attmdb_att_get_value(udss_env.att_tbl[charcode] + 2, &length, &value);

    if ((length != sizeof(uint16_t)) || (*((uint16_t *) value) != ccc_val))
        return UDS_ERROR_IMPROPERLY_CONFIGURED;

    return PRF_ERR_OK;
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref UDSS_SET_CHAR_VAL_REQ message.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int udss_set_char_val_req_handler(ke_msg_id_t const msgid,
                                         struct udss_set_char_val_req const *param,
                                         ke_task_id_t const dest_id,
                                         ke_task_id_t const src_id)
{
    // Request status
    uint8_t status;

    //Save the application task id
    udss_env.con_info.appid = src_id;

    // Check Characteristic Code
    if (param->char_code >= UDS_CHAR_MAX)//UDS_PNP_ID_CHAR)
    {
        status = PRF_ERR_INVALID_PARAM;
        goto done;
    }

    // Set value in db (value handle is right after the char definition)
    status = attmdb_att_set_value(udss_env.att_tbl[param->char_code] + 1,
                                param->val_len, (uint8_t *)&param->val[0]);

    if ((status != PRF_ERR_OK) ||
                        (param->char_code != UDS_USER_DB_CHANGE_INCR_CHAR))
        goto done;

    // Notify the remote if DB_CHANGE_INCR_CHAR is configured to notify
    status = uds_verify_ccc(param->char_code, PRF_CLI_START_NTF);
            if (status == PRF_ERR_OK)
        prf_server_send_event((prf_env_struct *)&udss_env, false,
                                    udss_env.att_tbl[param->char_code] + 1);

done:
    if (status != PRF_ERR_OK)
    {
        // Report immediately in case of error
        prf_server_error_ind_send((prf_env_struct *)&udss_env, status,
                                  UDSS_ERROR_IND, UDSS_SET_CHAR_VAL_REQ);
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref UDSS_ENABLE_REQ message.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int udss_enable_req_handler(ke_msg_id_t const msgid,
                                   struct udss_enable_req const *param,
                                   ke_task_id_t const dest_id,
                                   ke_task_id_t const src_id)
{
    uint16_t db_change_ntf_en = PRF_CLI_STOP_NTFIND;
    uint16_t cpt_ind_en = PRF_CLI_STOP_NTFIND;

    //Save the connection handle associated to the profile
    udss_env.con_info.conidx = gapc_get_conidx(param->conhdl);
    //Save the application id
    udss_env.con_info.appid = src_id;

    // Check if the provided connection exist
    if (udss_env.con_info.conidx == GAP_INVALID_CONIDX)
    {
        // The connection doesn't exist, request disallowed
        prf_server_error_ind_send((prf_env_struct *)&udss_env, PRF_ERR_REQ_DISALLOWED,
                                  UDSS_ERROR_IND, UDSS_ENABLE_REQ);
    }
    else
    {
        //Enable Attributes + Set Security Level
        attmdb_svc_set_permission(udss_env.shdl, param->sec_lvl);

        // Go to connected state
        ke_state_set(TASK_UDSS, UDSS_CONNECTED);
    }

    // Restore DB Change notification configuration
    if ((param->con_type == PRF_CON_NORMAL) &&
                                (param->db_change_ntf_en == PRF_CLI_START_NTF))
        db_change_ntf_en = param->db_change_ntf_en;

    attmdb_att_set_value(udss_env.att_tbl[UDS_USER_DB_CHANGE_INCR_CHAR] + 2,
                                                sizeof(uint16_t),
                                                (uint8_t *) &db_change_ntf_en);

    // Restore Control Point indication configuration
    if ((param->con_type == PRF_CON_NORMAL) &&
                                    (param->cpt_ind_en == PRF_CLI_START_IND))
        cpt_ind_en = param->cpt_ind_en;

    attmdb_att_set_value(udss_env.att_tbl[UDS_USER_CTRL_POINT_CHAR] + 2,
                                                    sizeof(uint16_t),
                                                    (uint8_t *) &cpt_ind_en);

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref UDSS_UCP_RSP_REQ message,
 * sent by the app when a UCP request is finished
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int udss_ucp_rsp_req_handler(ke_msg_id_t const msgid,
                                     struct udss_ucp_rsp_req const *param,
                                     ke_task_id_t const dest_id,
                                     ke_task_id_t const src_id)
{
    struct uds_ucp_rsp ucp_rsp;
    // Status
    volatile uint8_t status = PRF_ERR_OK;
		volatile int x = sizeof(struct udss_ucp_rsp_req);
		volatile int xx = param->ucp_rsp.op_code;
    // Check connection handle
    if(param->conhdl == gapc_get_conhdl(udss_env.con_info.conidx))
    {
        // Check if op code valid
        if(param->ucp_rsp.op_code != UDS_REQ_RSP_CODE)
        {
            //Wrong op code
            status = PRF_ERR_INVALID_PARAM;
        }
        // Check if UCP on going
        else if(!(UDS_IS(UCP_ON_GOING)))
        {
            // Cannot send response since no RACP on going
            status = PRF_ERR_REQ_DISALLOWED;
        }
        else
        {
            /// Fill-in response code op code
            ucp_rsp.op_code = param->ucp_rsp.op_code;
            /// Fill-in request op code
            ucp_rsp.req_op_code = param->ucp_rsp.req_op_code;
						
            // Check if request op code is supported
            if((param->ucp_rsp.req_op_code < UDS_REQ_REG_NEW_USER)
               && ( param->ucp_rsp.req_op_code > UDS_REQ_DEL_USER_DATA))
            {
                /// Fill-in response value for not supported op code
                ucp_rsp.rsp_val = UDS_RSP_OP_CODE_NOT_SUP;
            }
            else
            {
                // Fill-in response value from application
                ucp_rsp.rsp_val = param->ucp_rsp.rsp_val;
                if((ucp_rsp.req_op_code == UDS_REQ_REG_NEW_USER)
                    &&(ucp_rsp.rsp_val == UDS_RSP_SUCCESS))
                {
                    ucp_rsp.parameter.reg_new_user.user_idx = param->ucp_rsp.parameter.reg_new_user.user_idx;
                    udss_env.db_valid = true;
                }
                else if ((ucp_rsp.req_op_code == UDS_REQ_CONSENT)
                            &&(ucp_rsp.rsp_val == UDS_RSP_SUCCESS))
                {
                    udss_env.db_valid = true;
                }
                else if ((ucp_rsp.req_op_code == UDS_REQ_DEL_USER_DATA)
                            &&(ucp_rsp.rsp_val == UDS_RSP_SUCCESS))
                {
                    uint8_t val = UDSS_NO_USER;

                    attmdb_att_set_value(udss_env.att_tbl[UDS_USER_INDEX_CHAR] + 1,
                                                            sizeof(val), &val);
                    udss_env.db_valid = false;
                }

                // Send UCP indication
                udss_send_ucp_rsp(&ucp_rsp, udss_env.att_tbl[UDS_USER_CTRL_POINT_CHAR] + 1,
                                                    udss_env.con_info.appid);
            }
        }
    }
    else
    {
        //Wrong Connection Handle
        status = PRF_ERR_INVALID_PARAM;
    }

    if (status != PRF_ERR_OK)
    {
        prf_server_error_ind_send((prf_env_struct *)&udss_env, status,
                                  UDSS_ERROR_IND, UDSS_UCP_RSP_REQ);
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref GATTC_WRITE_CMD_IND message.
 * The handler will notify the app about the action to perform
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gattc_write_cmd_ind_handler(ke_msg_id_t const msgid,
                                      struct gattc_write_cmd_ind const *param,
                                      ke_task_id_t const dest_id,
                                      ke_task_id_t const src_id)
{
    uint8_t status = PRF_APP_ERROR;

    if (KE_IDX_GET(src_id) != udss_env.con_info.conidx)
        goto done;

    if (param->handle == udss_env.att_tbl[UDS_USER_CTRL_POINT_CHAR] + 1)
    {
        // status = PRF_ERR_OK;
        uint8_t reqstatus;
        uint8_t* value;
        uint16_t length;
        struct uds_ucp_req ucp_req;

        // Check if there is an ongoing UCP request
        if (UDS_IS(UCP_ON_GOING))
        {
            // Reschedule once to get indication confirmation
            if (!UDS_IS(UCP_PENDING_REQ))
        {
                UDS_SET(UCP_PENDING_REQ);

                ke_msg_forward(param, dest_id, dest_id);
                return (KE_MSG_NO_FREE);
        }
            else
        {
                UDS_CLEAR(UCP_PENDING_REQ);

                status = UDS_ERROR_PROC_IN_PROGRESS;
                goto done;
            }
        }

        // If message was rescheduled, it's handled now - clear the pending flag
        UDS_CLEAR(UCP_PENDING_REQ);

        // Check if CCC descriptor is properly configured
        status = uds_verify_ccc(UDS_USER_CTRL_POINT_CHAR, PRF_CLI_START_IND);
        if (status != PRF_ERR_OK)
            goto done;

            // Update the attribute value (note several write could be required since
            // attribute length > (ATT_MTU-3)
            attmdb_att_update_value(param->handle, param->length, param->offset,
                    (uint8_t*)&(param->value[0]));

        // Retrieve full data.
            attmdb_att_get_value(param->handle, &length, &value);

        // Unpack user control point value
            reqstatus = udss_unpack_ucp_req(value, length, &ucp_req);

        // Check unpacked status
            switch(reqstatus)
            {
                case PRF_APP_ERROR:
                {
                    /* Do nothing, ignore request since it's not complete and maybe
                     * requires several peer write to be performed.
                     */
                }
                break;
                case PRF_ERR_OK:
                {
                // Check wich request shall be send to api task
                    switch(ucp_req.op_code)
                    {
                        case UDS_REQ_REG_NEW_USER:
                        case UDS_REQ_CONSENT:
                        case UDS_REQ_DEL_USER_DATA:
                        {
                        // Forward request operation to application
                            struct udss_ucp_req_ind * req = KE_MSG_ALLOC(UDSS_UCP_REQ_IND,
                                    udss_env.con_info.appid, TASK_UDSS,
                                    udss_ucp_req_ind);

                            // UCP on going.
                        UDS_SET(UCP_ON_GOING);

                            req->conhdl = gapc_get_conhdl(udss_env.con_info.conidx);
                            req->ucp_req = ucp_req;

                            ke_msg_send(req);
                        }
                        break;
                        default:
                        {
                        // Nothing to do since it's handled during unpack
                        }
                        break;
                    }
                }
                break;
                default:
                {
                    /* There is an error in user control request, inform peer
                     * device that request is incorrect. */
                    struct uds_ucp_rsp ucp_rsp;

                    ucp_rsp.op_code = UDS_REQ_RSP_CODE;
                    ucp_rsp.req_op_code = ucp_req.op_code;
                    ucp_rsp.rsp_val = reqstatus;

                atts_write_rsp_send(udss_env.con_info.conidx, param->handle, status);

                // Send user control response indication
                    udss_send_ucp_rsp(&(ucp_rsp),
                                udss_env.att_tbl[UDS_USER_CTRL_POINT_CHAR] + 1,
                            TASK_UDSS);

                return (KE_MSG_CONSUMED);
            }
        }
    }
    else if (param->handle <= udss_env.att_tbl[UDS_CHAR_MAX - 1])
    {
        if (!udss_env.db_valid &&
            (param->handle != udss_env.att_tbl[UDS_USER_DB_CHANGE_INCR_CFG]) &&
            (param->handle != udss_env.att_tbl[UDS_USER_CTRL_POINT_CFG]))
        {
            status = UDS_ERROR_DATA_ACCESS_NOT_PERMITTED;
            goto done;
                }

        // This should handle also the CCC descriptors
        status = attmdb_att_update_value(param->handle, param->length,
                                                param->offset,
                                                (uint8_t*)&(param->value[0]));

        // Notify if its the last write so that the App would know the latest
        // value for the current user.
        if ((param->handle < udss_env.att_tbl[UDS_USER_DB_CHANGE_INCR_CHAR]) &&
                                        (status == PRF_ERR_OK) && (param->last))
        {
            uint16_t charcode = UDS_CHAR_MAX;
            struct udss_char_val_ind *ind;
            uint16_t length;
            uint8_t *value;

            // Get the full value from the db
            status = attmdb_att_get_value(param->handle, &length, &value);
            if (status != PRF_ERR_OK)
                goto done;

            ind = ke_msg_alloc(UDSS_CHAR_VAL_IND, udss_env.con_info.appid,
                                TASK_UDSS,
                                sizeof(struct udss_char_val_ind) + length);

            // Find charcode by the handle
            for (int i = 0; i < UDS_CHAR_MAX; i++)
            {
                if (udss_env.att_tbl[i] + 1 == param->handle)
                {
                    charcode = uds_char_def_table[i].charcode;
                break;
            }
            }

            ind->val_len = length;
            ind->char_code = charcode;
            memcpy(ind->val, value, length);
            
            ke_msg_send(ind);
        }
    }
    // Not expected request
        else
        {
            status = ATT_ERR_WRITE_NOT_PERMITTED;
        }
    
done:
    if(param->response)
    {
        // Send Write Response
        atts_write_rsp_send(udss_env.con_info.conidx, param->handle, status);
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Checks if a handle is the value descriptor of a UDS characteristic.
 * @param[in] handle    The handle to be checked.
 * @return True if the given handle is the value descriptor of a UDS characteristic.
 ****************************************************************************************
 */
static inline bool is_uds_char_value_desc(uint16_t handle)
{
    // The handles of UDS characteristic value descriptors are located at even offsets from
    // the service declaration handle
    if (handle > udss_env.shdl && handle < udss_env.att_tbl[UDS_USER_DB_CHANGE_INCR_CHAR]
        && !((handle - udss_env.shdl) & 1) )
    {
        return true;
    }

    return false;
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref ATTS_READ_REQ_IND message.
 * The handler will notify the app about the action to perform
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int atts_read_req_ind_handler(ke_msg_id_t const msgid,
                                      struct atts_read_req_ind const *param,
                                      ke_task_id_t const dest_id,
                                      ke_task_id_t const src_id)
{
    uint16_t handle = param->handle;
    uint8_t read_error_code = ATT_ERR_NO_ERROR;

    // UDS characteristics can be read only after the consent has been given
    if (is_uds_char_value_desc(handle) && !udss_env.db_valid)
    {
        read_error_code = UDS_ERROR_DATA_ACCESS_NOT_PERMITTED;
    }

    // Confirm read request
    dg_atts_read_cfm(udss_env.con_info.conidx, handle, read_error_code);

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Disconnection indication from GAP.
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
    if (KE_IDX_GET(src_id) == udss_env.con_info.conidx)
    {
        udss_disable(param->conhdl);
        udss_env.db_valid = false;

        // Clear the ongoing UCP request on disconnection
        UDS_CLEAR(UCP_ON_GOING);

        UDS_CLEAR(UCP_PENDING_REQ);
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Completion indication from GATT
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
    if(param->req_type == GATTC_INDICATE)
    {
        // There is no more UCP on going as it was confirmed
        UDS_CLEAR(UCP_ON_GOING);
    }

    return (KE_MSG_CONSUMED);
}

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

///Disabled State handler definition.
const struct ke_msg_handler udss_disabled[] =
{
    {UDSS_CREATE_DB_REQ,         (ke_msg_func_t)udss_create_db_req_handler},
};

///Idle State handler definition.
const struct ke_msg_handler udss_idle[] =
{
    {UDSS_SET_CHAR_VAL_REQ,      (ke_msg_func_t)udss_set_char_val_req_handler},
    {UDSS_ENABLE_REQ,            (ke_msg_func_t)udss_enable_req_handler},
};

///Connected State handler definition.
const struct ke_msg_handler udss_connected[] =
{
    {UDSS_UCP_RSP_REQ,           (ke_msg_func_t) udss_ucp_rsp_req_handler},
    {GATTC_WRITE_CMD_IND,        (ke_msg_func_t) gattc_write_cmd_ind_handler},
    {UDSS_SET_CHAR_VAL_REQ,      (ke_msg_func_t) udss_set_char_val_req_handler},
    {GATTC_CMP_EVT,              (ke_msg_func_t) gattc_cmp_evt_handler},
    {ATTS_READ_REQ_IND,          (ke_msg_func_t) atts_read_req_ind_handler},
};

/// Default State handlers definition
const struct ke_msg_handler udss_default_state[] =
{
    {GAPC_DISCONNECT_IND,        (ke_msg_func_t) gapc_disconnect_ind_handler},
};

///Specifies the message handler structure for every input state.
const struct ke_state_handler udss_state_handler[UDSS_STATE_MAX] =
{
    [UDSS_DISABLED]    = KE_STATE_HANDLER(udss_disabled),
    [UDSS_IDLE]        = KE_STATE_HANDLER(udss_idle),
    [UDSS_CONNECTED]   = KE_STATE_HANDLER(udss_connected),
};

///Specifies the message handlers that are common to all states.
const struct ke_state_handler udss_default_handler = KE_STATE_HANDLER(udss_default_state);

///Defines the place holder for the states of all the task instances.
ke_state_t udss_state[UDSS_IDX_MAX] __attribute__((section("retention_mem_area0"),zero_init)); //@RETENTION MEMORY

#endif //BLE_UDS_SERVER

/// @} UDSSTASK
