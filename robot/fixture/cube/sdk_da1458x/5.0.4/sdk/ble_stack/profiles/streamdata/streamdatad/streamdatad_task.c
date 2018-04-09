/**
 ****************************************************************************************
 *
 * @file streamdatad_task.c
 *
 * @brief StreamData Device profile task
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
 *
 * @file streamdatad_task.c
 *
 * @brief StreamData Device profile task.
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
 * @addtogroup STREAMDATADTASK
 * @{
 ****************************************************************************************
 */


/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "rwble_config.h"

#if (BLE_STREAMDATA_DEVICE)

#include "gap.h"
#include "gattc_task.h"
#include "attm_util.h"
#include "atts_util.h"
#include "attm_cfg.h"
#include "prf_utils.h"

#include "streamdatad_task.h"
#include "streamdatad.h"
#include "llc_task.h" // llc_nb_of_pkt_evt_complete

/// Pong Service
static const att_svc_desc_t streamdatad_svc = STREAMDATAD_SERVICE_UUID;
/// Enable Characteristic

static const struct att_char_desc streamdatad_enable_char = ATT_CHAR(ATT_CHAR_PROP_RD | ATT_CHAR_PROP_WR_NO_RESP, 0, STREAMDATAD_ENABLE_UUID);

/// Data Characteristic
static const struct att_char_desc streamdatad_d0_char = ATT_CHAR(ATT_CHAR_PROP_RD | ATT_CHAR_PROP_WR_NO_RESP | ATT_CHAR_PROP_NTF, 0, STREAMDATAD_D0_UUID);
static const struct att_char_desc streamdatad_d1_char = ATT_CHAR(ATT_CHAR_PROP_RD | ATT_CHAR_PROP_WR_NO_RESP | ATT_CHAR_PROP_NTF, 0, STREAMDATAD_D1_UUID);
static const struct att_char_desc streamdatad_d2_char = ATT_CHAR(ATT_CHAR_PROP_RD | ATT_CHAR_PROP_WR_NO_RESP | ATT_CHAR_PROP_NTF, 0, STREAMDATAD_D2_UUID);
static const struct att_char_desc streamdatad_d3_char = ATT_CHAR(ATT_CHAR_PROP_RD | ATT_CHAR_PROP_WR_NO_RESP | ATT_CHAR_PROP_NTF, 0, STREAMDATAD_D3_UUID);
static const struct att_char_desc streamdatad_d4_char = ATT_CHAR(ATT_CHAR_PROP_RD | ATT_CHAR_PROP_WR_NO_RESP | ATT_CHAR_PROP_NTF, 0, STREAMDATAD_D4_UUID);
static const struct att_char_desc streamdatad_d5_char = ATT_CHAR(ATT_CHAR_PROP_RD | ATT_CHAR_PROP_WR_NO_RESP | ATT_CHAR_PROP_NTF, 0, STREAMDATAD_D5_UUID);
static const struct att_char_desc streamdatad_d6_char = ATT_CHAR(ATT_CHAR_PROP_RD | ATT_CHAR_PROP_WR_NO_RESP | ATT_CHAR_PROP_NTF, 0, STREAMDATAD_D6_UUID);
static const struct att_char_desc streamdatad_d7_char = ATT_CHAR(ATT_CHAR_PROP_RD | ATT_CHAR_PROP_WR_NO_RESP | ATT_CHAR_PROP_NTF, 0, STREAMDATAD_D7_UUID);
static const struct att_char_desc streamdatad_d8_char = ATT_CHAR(ATT_CHAR_PROP_RD | ATT_CHAR_PROP_WR_NO_RESP | ATT_CHAR_PROP_NTF, 0, STREAMDATAD_D8_UUID);
static const struct att_char_desc streamdatad_d9_char = ATT_CHAR(ATT_CHAR_PROP_RD | ATT_CHAR_PROP_WR_NO_RESP | ATT_CHAR_PROP_NTF, 0, STREAMDATAD_D9_UUID);
/// Enable description
static const uint8_t streamdatad_enable_desc[] = STREAMDATAD_ENABLE_DESC;

/// Data description
static const uint8_t streamdatad_d_desc[] = STREAMDATAD_D_DESC;
                                       
                                       
/// Full STREAMDATAD Database Description - Used to add attributes into the database
static const struct attm_desc streamdatad_att_db[STREAMDATAD_IDX_NB] =
{
    [STREAMDATAD_IDX_PRIM_SVC]   =   /* StreamData Device service */
                               {ATT_DECL_PRIMARY_SERVICE, PERM(RD, ENABLE),
                                       sizeof(streamdatad_svc), sizeof(streamdatad_svc),
                                       (uint8_t*) &streamdatad_svc},
									   
    [STREAMDATAD_IDX_ENABLE_CHAR]  =        {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), sizeof(streamdatad_enable_char), sizeof(streamdatad_enable_char), (uint8_t*) &streamdatad_enable_char},
    [STREAMDATAD_IDX_ENABLE_VAL] =          {STREAMDATAD_ENABLE_UUID,  (PERM(RD, ENABLE) | PERM(WR, ENABLE) ), sizeof(uint16_t), 0, (uint8_t*) NULL},
    [STREAMDATAD_IDX_ENABLE_EN] =           {ATT_DESC_CLIENT_CHAR_CFG,(PERM(RD, ENABLE) | PERM(WR, ENABLE) | PERM(NTF, ENABLE)), sizeof(uint16_t), 0, (uint8_t*) NULL},
    [STREAMDATAD_IDX_ENABLE_DESC] =         {ATT_DESC_CHAR_USER_DESCRIPTION,  PERM(RD, ENABLE), STREAMDATAD_ENABLE_DESC_LEN, STREAMDATAD_ENABLE_DESC_LEN, (uint8_t*) streamdatad_enable_desc},
									   
    [STREAMDATAD_IDX_STREAMDATAD_D0_CHAR] = {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), sizeof(streamdatad_d0_char), sizeof(streamdatad_d0_char), (uint8_t*) &streamdatad_d0_char},
    [STREAMDATAD_IDX_STREAMDATAD_D0_VAL] =  {STREAMDATAD_D0_UUID, PERM(RD, ENABLE) | PERM(WR, ENABLE) | PERM(NTF, ENABLE), (sizeof(uint8_t) * STREAMDATAD_PACKET_SIZE), 0, (uint8_t*) NULL},
    [STREAMDATAD_IDX_STREAMDATAD_D0_EN] =   {ATT_DESC_CLIENT_CHAR_CFG,(PERM(RD, ENABLE) | PERM(WR, ENABLE) | PERM(NTF, ENABLE)), sizeof(uint16_t), 0, (uint8_t*) NULL},
    [STREAMDATAD_IDX_STREAMDATAD_D0_DESC] = {ATT_DESC_CHAR_USER_DESCRIPTION, PERM(RD, ENABLE), STREAMDATAD_D_DESC_LEN, STREAMDATAD_D_DESC_LEN, (uint8_t*) streamdatad_d_desc},

    [STREAMDATAD_IDX_STREAMDATAD_D1_CHAR] = {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), sizeof(streamdatad_d1_char), sizeof(streamdatad_d1_char), (uint8_t*) &streamdatad_d1_char},
    [STREAMDATAD_IDX_STREAMDATAD_D1_VAL] =  {STREAMDATAD_D1_UUID, PERM(RD, ENABLE) | PERM(WR, ENABLE) | PERM(NTF, ENABLE), (sizeof(uint8_t) * STREAMDATAD_PACKET_SIZE), 0, (uint8_t*) NULL},
    [STREAMDATAD_IDX_STREAMDATAD_D1_EN] =   {ATT_DESC_CLIENT_CHAR_CFG,(PERM(RD, ENABLE) | PERM(WR, ENABLE) | PERM(NTF, ENABLE)), sizeof(uint16_t), 0, (uint8_t*) NULL},
    [STREAMDATAD_IDX_STREAMDATAD_D1_DESC] = {ATT_DESC_CHAR_USER_DESCRIPTION, PERM(RD, ENABLE), STREAMDATAD_D_DESC_LEN, STREAMDATAD_D_DESC_LEN, (uint8_t*) streamdatad_d_desc},

    [STREAMDATAD_IDX_STREAMDATAD_D2_CHAR] = {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), sizeof(streamdatad_d2_char), sizeof(streamdatad_d2_char), (uint8_t*) &streamdatad_d2_char},
    [STREAMDATAD_IDX_STREAMDATAD_D2_VAL] =  {STREAMDATAD_D2_UUID, PERM(RD, ENABLE) | PERM(WR, ENABLE) | PERM(NTF, ENABLE), (sizeof(uint8_t) * STREAMDATAD_PACKET_SIZE), 0, (uint8_t*) NULL},
    [STREAMDATAD_IDX_STREAMDATAD_D2_EN] =   {ATT_DESC_CLIENT_CHAR_CFG,(PERM(RD, ENABLE) | PERM(WR, ENABLE) | PERM(NTF, ENABLE)), sizeof(uint16_t), 0, (uint8_t*) NULL},
    [STREAMDATAD_IDX_STREAMDATAD_D2_DESC] = {ATT_DESC_CHAR_USER_DESCRIPTION, PERM(RD, ENABLE), STREAMDATAD_D_DESC_LEN, STREAMDATAD_D_DESC_LEN, (uint8_t*) streamdatad_d_desc},
    
    [STREAMDATAD_IDX_STREAMDATAD_D3_CHAR] = {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), sizeof(streamdatad_d3_char), sizeof(streamdatad_d3_char), (uint8_t*) &streamdatad_d3_char},
    [STREAMDATAD_IDX_STREAMDATAD_D3_VAL] =  {STREAMDATAD_D3_UUID, PERM(RD, ENABLE) | PERM(WR, ENABLE) | PERM(NTF, ENABLE), (sizeof(uint8_t) * STREAMDATAD_PACKET_SIZE), 0, (uint8_t*) NULL},
    [STREAMDATAD_IDX_STREAMDATAD_D3_EN] =   {ATT_DESC_CLIENT_CHAR_CFG,(PERM(RD, ENABLE) | PERM(WR, ENABLE) | PERM(NTF, ENABLE)), sizeof(uint16_t), 0, (uint8_t*) NULL},
    [STREAMDATAD_IDX_STREAMDATAD_D3_DESC] = {ATT_DESC_CHAR_USER_DESCRIPTION, PERM(RD, ENABLE), STREAMDATAD_D_DESC_LEN, STREAMDATAD_D_DESC_LEN, (uint8_t*) streamdatad_d_desc},

    [STREAMDATAD_IDX_STREAMDATAD_D4_CHAR] = {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), sizeof(streamdatad_d4_char), sizeof(streamdatad_d4_char), (uint8_t*) &streamdatad_d4_char},
    [STREAMDATAD_IDX_STREAMDATAD_D4_VAL] =  {STREAMDATAD_D4_UUID, PERM(RD, ENABLE) | PERM(WR, ENABLE) | PERM(NTF, ENABLE), (sizeof(uint8_t) * STREAMDATAD_PACKET_SIZE), 0, (uint8_t*) NULL},
    [STREAMDATAD_IDX_STREAMDATAD_D4_EN] =   {ATT_DESC_CLIENT_CHAR_CFG,(PERM(RD, ENABLE) | PERM(WR, ENABLE) | PERM(NTF, ENABLE)), sizeof(uint16_t), 0, (uint8_t*) NULL},
    [STREAMDATAD_IDX_STREAMDATAD_D4_DESC] = {ATT_DESC_CHAR_USER_DESCRIPTION, PERM(RD, ENABLE), STREAMDATAD_D_DESC_LEN, STREAMDATAD_D_DESC_LEN, (uint8_t*) streamdatad_d_desc},

    [STREAMDATAD_IDX_STREAMDATAD_D5_CHAR] = {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), sizeof(streamdatad_d5_char), sizeof(streamdatad_d5_char), (uint8_t*) &streamdatad_d5_char},
    [STREAMDATAD_IDX_STREAMDATAD_D5_VAL] =  {STREAMDATAD_D5_UUID, PERM(RD, ENABLE) | PERM(WR, ENABLE) | PERM(NTF, ENABLE), (sizeof(uint8_t) * STREAMDATAD_PACKET_SIZE), 0, (uint8_t*) NULL},
    [STREAMDATAD_IDX_STREAMDATAD_D5_EN] =   {ATT_DESC_CLIENT_CHAR_CFG,(PERM(RD, ENABLE) | PERM(WR, ENABLE) | PERM(NTF, ENABLE)), sizeof(uint16_t), 0, (uint8_t*) NULL},
    [STREAMDATAD_IDX_STREAMDATAD_D5_DESC] = {ATT_DESC_CHAR_USER_DESCRIPTION, PERM(RD, ENABLE), STREAMDATAD_D_DESC_LEN, STREAMDATAD_D_DESC_LEN, (uint8_t*) streamdatad_d_desc},
    
    [STREAMDATAD_IDX_STREAMDATAD_D6_CHAR] = {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), sizeof(streamdatad_d6_char), sizeof(streamdatad_d6_char), (uint8_t*) &streamdatad_d6_char},
    [STREAMDATAD_IDX_STREAMDATAD_D6_VAL] =  {STREAMDATAD_D6_UUID, PERM(RD, ENABLE) | PERM(WR, ENABLE) | PERM(NTF, ENABLE), (sizeof(uint8_t) * STREAMDATAD_PACKET_SIZE), 0, (uint8_t*) NULL},
    [STREAMDATAD_IDX_STREAMDATAD_D6_EN] =   {ATT_DESC_CLIENT_CHAR_CFG,(PERM(RD, ENABLE) | PERM(WR, ENABLE) | PERM(NTF, ENABLE)), sizeof(uint16_t), 0, (uint8_t*) NULL},
    [STREAMDATAD_IDX_STREAMDATAD_D6_DESC] = {ATT_DESC_CHAR_USER_DESCRIPTION, PERM(RD, ENABLE), STREAMDATAD_D_DESC_LEN, STREAMDATAD_D_DESC_LEN, (uint8_t*) streamdatad_d_desc},
    
    [STREAMDATAD_IDX_STREAMDATAD_D7_CHAR] = {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), sizeof(streamdatad_d7_char), sizeof(streamdatad_d7_char), (uint8_t*) &streamdatad_d7_char},
    [STREAMDATAD_IDX_STREAMDATAD_D7_VAL] =  {STREAMDATAD_D7_UUID, PERM(RD, ENABLE) | PERM(WR, ENABLE) | PERM(NTF, ENABLE), (sizeof(uint8_t) * STREAMDATAD_PACKET_SIZE), 0, (uint8_t*) NULL},
    [STREAMDATAD_IDX_STREAMDATAD_D7_EN] =   {ATT_DESC_CLIENT_CHAR_CFG,(PERM(RD, ENABLE) | PERM(WR, ENABLE) | PERM(NTF, ENABLE)), sizeof(uint16_t), 0, (uint8_t*) NULL},
    [STREAMDATAD_IDX_STREAMDATAD_D7_DESC] = {ATT_DESC_CHAR_USER_DESCRIPTION, PERM(RD, ENABLE), STREAMDATAD_D_DESC_LEN, STREAMDATAD_D_DESC_LEN, (uint8_t*) streamdatad_d_desc},
  
    [STREAMDATAD_IDX_STREAMDATAD_D8_CHAR] = {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), sizeof(streamdatad_d8_char), sizeof(streamdatad_d8_char), (uint8_t*) &streamdatad_d8_char},
    [STREAMDATAD_IDX_STREAMDATAD_D8_VAL] =  {STREAMDATAD_D8_UUID, PERM(RD, ENABLE) | PERM(WR, ENABLE) | PERM(NTF, ENABLE), (sizeof(uint8_t) * STREAMDATAD_PACKET_SIZE), 0, (uint8_t*) NULL},
    [STREAMDATAD_IDX_STREAMDATAD_D8_EN] =   {ATT_DESC_CLIENT_CHAR_CFG,(PERM(RD, ENABLE) | PERM(WR, ENABLE) | PERM(NTF, ENABLE)), sizeof(uint16_t), 0, (uint8_t*) NULL},
    [STREAMDATAD_IDX_STREAMDATAD_D8_DESC] = {ATT_DESC_CHAR_USER_DESCRIPTION, PERM(RD, ENABLE), STREAMDATAD_D_DESC_LEN, STREAMDATAD_D_DESC_LEN, (uint8_t*) streamdatad_d_desc},    
    
    [STREAMDATAD_IDX_STREAMDATAD_D9_CHAR] = {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), sizeof(streamdatad_d9_char), sizeof(streamdatad_d9_char), (uint8_t*) &streamdatad_d9_char},
    [STREAMDATAD_IDX_STREAMDATAD_D9_VAL] =  {STREAMDATAD_D9_UUID, PERM(RD, ENABLE) | PERM(WR, ENABLE) | PERM(NTF, ENABLE), (sizeof(uint8_t) * STREAMDATAD_PACKET_SIZE), 0, (uint8_t*) NULL},
    [STREAMDATAD_IDX_STREAMDATAD_D9_EN] =   {ATT_DESC_CLIENT_CHAR_CFG,(PERM(RD, ENABLE) | PERM(WR, ENABLE) | PERM(NTF, ENABLE)), sizeof(uint16_t), 0, (uint8_t*) NULL},
    [STREAMDATAD_IDX_STREAMDATAD_D9_DESC] = {ATT_DESC_CHAR_USER_DESCRIPTION, PERM(RD, ENABLE), STREAMDATAD_D_DESC_LEN, STREAMDATAD_D_DESC_LEN, (uint8_t*) streamdatad_d_desc},
};


/**
 ****************************************************************************************
 * @brief Handles reception of the @ref STREAMDATAD_CREATE_DB_REQ message.
 * The handler adds STREAMDATAD Service into the database using the database
 * configuration value given in param.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int streamdatad_create_db_req_handler(ke_msg_id_t const msgid,
                                      struct streamdatad_create_db_req const *param,
                                      ke_task_id_t const dest_id,
                                      ke_task_id_t const src_id)
{
    //Service Configuration Flag
    uint32_t cfg_flag = 0xFFFFFFFF;
    //Database Creation Status
    uint8_t status;

    //Save Application ID
    streamdatad_env.appid = src_id;


    // set start handle or automatically set it when creating database (start_hdl = 0)
    streamdatad_env.shdl=param->start_hdl;

    //Add Service Into Database
    status = attm_svc_create_db(&streamdatad_env.shdl, (uint8_t *)&cfg_flag,  STREAMDATAD_IDX_NB, NULL,
                               dest_id, &streamdatad_att_db[0]);

    //Disable GLS
    attmdb_svc_set_permission(streamdatad_env.shdl, PERM(SVC, DISABLE));

    //Go to Idle State
    if (status == ATT_ERR_NO_ERROR)
    {
        //If we are here, database has been fulfilled with success, go to idle test
        ke_state_set(TASK_STREAMDATAD, STREAMDATAD_IDLE);
    }

    //Send response to application
    struct streamdatad_create_db_cfm * cfm = KE_MSG_ALLOC(STREAMDATAD_CREATE_DB_CFM, streamdatad_env.appid,
                                                    TASK_STREAMDATAD, streamdatad_create_db_cfm);
    cfm->status = status;
    ke_msg_send(cfm);

    return (KE_MSG_CONSUMED);
}


/*
 * MESSAGE HANDLERS
 ****************************************************************************************
 */
/**
 ****************************************************************************************
 * @brief Handles reception of the @ref STREAMDATAD_ENABLE_REQ message.
 * The handler enables the StreamData Device profile.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int streamdatad_enable_req_handler(ke_msg_id_t const msgid,
                                    struct streamdatad_enable_req const *param,
                                    ke_task_id_t const dest_id,
                                    ke_task_id_t const src_id)
{
    uint16_t disable_val = 0x00;

    //Save Application ID
    //streamdatad_env.con_info.appid = src_id;
    streamdatad_env.con_info.prf_id = dest_id;
    streamdatad_env.con_info.appid = src_id;
    
    streamdatad_env.appid = src_id;
    //streamdatad_env.con_info.prf_id = dest_id;
    
    // Save the connection handle associated to the profile
    streamdatad_env.con_info.conidx = gapc_get_conidx(param->conhdl);
    // Save the connection handle associated to the profile
    streamdatad_env.conhdl = param->conhdl;

	streamdatad_env.next_attribute_idx = 0;
	streamdatad_env.nr_enabled_attributes = 0;
	streamdatad_env.stream_enabled = 0;

    // get tx buffers available 
    nb_buf_av = l2cm_get_nb_buffer_available() - 6;
    
    attmdb_att_set_value(STREAMDATAD_HANDLE(STREAMDATAD_IDX_ENABLE_VAL), sizeof(uint16_t), (uint8_t*) &(disable_val));

    attmdb_att_set_value(STREAMDATAD_HANDLE(STREAMDATAD_IDX_STREAMDATAD_D0_EN), sizeof(uint16_t),(uint8_t*) &(disable_val));
    attmdb_att_set_value(STREAMDATAD_HANDLE(STREAMDATAD_IDX_STREAMDATAD_D1_EN), sizeof(uint16_t),(uint8_t*) &(disable_val));
    attmdb_att_set_value(STREAMDATAD_HANDLE(STREAMDATAD_IDX_STREAMDATAD_D2_EN), sizeof(uint16_t),(uint8_t*) &(disable_val));
    attmdb_att_set_value(STREAMDATAD_HANDLE(STREAMDATAD_IDX_STREAMDATAD_D3_EN), sizeof(uint16_t),(uint8_t*) &(disable_val));
    attmdb_att_set_value(STREAMDATAD_HANDLE(STREAMDATAD_IDX_STREAMDATAD_D4_EN), sizeof(uint16_t),(uint8_t*) &(disable_val));
    attmdb_att_set_value(STREAMDATAD_HANDLE(STREAMDATAD_IDX_STREAMDATAD_D5_EN), sizeof(uint16_t),(uint8_t*) &(disable_val));
    attmdb_att_set_value(STREAMDATAD_HANDLE(STREAMDATAD_IDX_STREAMDATAD_D6_EN), sizeof(uint16_t),(uint8_t*) &(disable_val));
    attmdb_att_set_value(STREAMDATAD_HANDLE(STREAMDATAD_IDX_STREAMDATAD_D7_EN), sizeof(uint16_t),(uint8_t*) &(disable_val));
    attmdb_att_set_value(STREAMDATAD_HANDLE(STREAMDATAD_IDX_STREAMDATAD_D8_EN), sizeof(uint16_t),(uint8_t*) &(disable_val));
    attmdb_att_set_value(STREAMDATAD_HANDLE(STREAMDATAD_IDX_STREAMDATAD_D9_EN), sizeof(uint16_t),(uint8_t*) &(disable_val));

	//Enable Service
	attmdb_svc_set_permission(streamdatad_env.shdl, PERM(SVC, ENABLE));
    // Go to active state
    ke_state_set(TASK_STREAMDATAD, STREAMDATAD_ACTIVE);

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref STREAMDATAD_DISABLE_REQ message.
 * The handler disables the streamdataderometer profile.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int streamdatad_disable_req_handler(ke_msg_id_t const msgid,
                                     void const *param,
                                     ke_task_id_t const dest_id,
                                     ke_task_id_t const src_id)
{
    // Go to idle state
    ke_state_set(TASK_STREAMDATAD, STREAMDATAD_IDLE);

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref STREAMDATAD_SEND_DATA_PACKETS_REQ message.
 * The handler sends up to l2cm_get_nb_buffer_available() packets of data from param.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
extern    void set_pxact_gpio(void);
int streamdatad_send_data_packets_req_handler(ke_msg_id_t const msgid,
                                   struct streamdatad_send_data_packets_req const *param,
                                   ke_task_id_t const dest_id,
                                   ke_task_id_t const src_id)
{
	uint16_t next_packet;
	uint16_t nr_packets;
    uint16_t* packet_buffer_enabled;
    uint16_t len = 0;

	if (!streamdatad_env.stream_enabled) return KE_MSG_CONSUMED; 

	next_packet = 0;
	nr_packets = param->nr_packets;

    nb_buf_av = l2cm_get_nb_buffer_available();
    
    for (int li = 0; (li < STREAMDATAD_MAX) && (nr_packets > 0) && (nb_buf_av > 0); li++)
	{		
        packet_buffer_enabled = NULL;
        attmdb_att_get_value(STREAMDATAD_DIR_EN_HANDLE(streamdatad_env.next_attribute_idx), &(len), (uint8_t**)&(packet_buffer_enabled));
        
        if ((packet_buffer_enabled && (*packet_buffer_enabled)))
        {        
            // Update the value in the attribute database
            attmdb_att_set_value(STREAMDATAD_DIR_VAL_HANDLE(streamdatad_env.next_attribute_idx), sizeof(uint8_t) * STREAMDATAD_PACKET_SIZE, (uint8_t*) &(param->packets[next_packet][0]));

            // Send notification
            prf_server_send_event((prf_env_struct *)&(streamdatad_env.con_info), false, STREAMDATAD_DIR_VAL_HANDLE(streamdatad_env.next_attribute_idx));
        }
        else
        {
            len = 2;
        }
        
        //set_pxact_gpio();
        next_packet++; nr_packets--;
        nb_buf_av--;
        
        streamdatad_env.next_attribute_idx++; 
        if (streamdatad_env.next_attribute_idx >= STREAMDATAD_MAX) {
            
            streamdatad_env.next_attribute_idx = 0;
            break; // the for loop
        }
        
		// else notification at this index was not enabled;
    }
    //set_pxact_gpio();
    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref GATT_WRITE_CMD_IND message.
 * The handler checks if the stream needs to be turned on.
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
    // Update the attribute value
    attmdb_att_update_value(param->handle, param->length, param->offset,
            (uint8_t*)&(param->value[0]));

    switch (STREAMDATAD_IDX(param->handle))
    {
        case STREAMDATAD_IDX_ENABLE_VAL:
			streamdatad_streamonoff();
			atts_write_rsp_send(streamdatad_env.conhdl, param->handle, PRF_ERR_OK);
			break;

        case STREAMDATAD_IDX_STREAMDATAD_D0_EN:
        case STREAMDATAD_IDX_STREAMDATAD_D1_EN:
        case STREAMDATAD_IDX_STREAMDATAD_D2_EN:
        case STREAMDATAD_IDX_STREAMDATAD_D3_EN:
        case STREAMDATAD_IDX_STREAMDATAD_D4_EN:
        case STREAMDATAD_IDX_STREAMDATAD_D5_EN:
        case STREAMDATAD_IDX_STREAMDATAD_D6_EN:
        case STREAMDATAD_IDX_STREAMDATAD_D7_EN:
        case STREAMDATAD_IDX_STREAMDATAD_D8_EN:
        case STREAMDATAD_IDX_STREAMDATAD_D9_EN:
			atts_write_rsp_send(streamdatad_env.conhdl, param->handle, PRF_ERR_OK);
            break;
        
        case STREAMDATAD_IDX_STREAMDATAD_D0_VAL:
        case STREAMDATAD_IDX_STREAMDATAD_D1_VAL:
        case STREAMDATAD_IDX_STREAMDATAD_D2_VAL:
        case STREAMDATAD_IDX_STREAMDATAD_D3_VAL:
        case STREAMDATAD_IDX_STREAMDATAD_D4_VAL:
        case STREAMDATAD_IDX_STREAMDATAD_D5_VAL:
        case STREAMDATAD_IDX_STREAMDATAD_D6_VAL:
        case STREAMDATAD_IDX_STREAMDATAD_D7_VAL:
        case STREAMDATAD_IDX_STREAMDATAD_D8_VAL:
        case STREAMDATAD_IDX_STREAMDATAD_D9_VAL:
        {
            struct streamdatad_rcv_data_packet_ind * ind = KE_MSG_ALLOC(STREAMDATAD_RCV_DATA_PACKET_IND,
														streamdatad_env.con_info.appid, dest_id,
														streamdatad_rcv_data_packet_ind);

            ind->conhdl     = gapc_get_conhdl(streamdatad_env.con_info.conidx);
            ind->handle     = param->handle;
            ind->seq        = param->value[0]; // is already incrementing (with the dummy data of the test)
            ind->size       = param->length;
            memcpy(&(ind->value[0]), &(param->value[0]), param->length);

            // Forward Received packet data to APP with the sequence number to indicate lost packets
            ke_msg_send(ind);
        }
        break;
        
    }

    return (KE_MSG_CONSUMED);
}

uint8_t nb_buf_av;

#if 0
static int gattc_cmp_evt_handler(ke_msg_id_t const msgid,  struct gattc_cmp_evt const *param,
                                 ke_task_id_t const dest_id, ke_task_id_t const src_id)
{

    if(param->req_type == GATTC_NOTIFY)
    {
        nb_buf_av++;
        //set_pxact_gpio();
    }
    return (KE_MSG_CONSUMED);
}
#endif


/*
 * TASK DESCRIPTOR DEFINITIONS
 ****************************************************************************************
 */
/// Disabled State handler definition.
const struct ke_msg_handler streamdatad_disabled[] =
{
    {STREAMDATAD_CREATE_DB_REQ,   (ke_msg_func_t) streamdatad_create_db_req_handler}
};


/// IDLE State handlers definition
const struct ke_msg_handler streamdatad_idle[] =
{
    {STREAMDATAD_ENABLE_REQ,     (ke_msg_func_t)streamdatad_enable_req_handler}
};

/// ACTIVE State handlers definition
const struct ke_msg_handler streamdatad_active[] =
{
    {STREAMDATAD_DISABLE_REQ, (ke_msg_func_t)streamdatad_disable_req_handler},
    {STREAMDATAD_SEND_DATA_PACKETS_REQ, (ke_msg_func_t)streamdatad_send_data_packets_req_handler},
    {GATTC_WRITE_CMD_IND, (ke_msg_func_t)gattc_write_cmd_ind_handler},
//    {GATTC_CMP_EVT,       (ke_msg_func_t)gattc_cmp_evt_handler},
}; 

/// Specifies the message handler structure for every input state
const struct ke_state_handler streamdatad_state_handler[STREAMDATAD_STATE_MAX] =
{
    /// DISABLE State message handlers.
    [STREAMDATAD_DISABLED]  = KE_STATE_HANDLER(streamdatad_disabled),
    /// IDLE State message handlers.
    [STREAMDATAD_IDLE]      = KE_STATE_HANDLER(streamdatad_idle),
    /// ACTIVE State message handlers.
    [STREAMDATAD_ACTIVE]    = KE_STATE_HANDLER(streamdatad_active),

};

/// Specifies the message handlers that are common to all states.
const struct ke_state_handler streamdatad_default_handler = KE_STATE_HANDLER_NONE;

/// Defines the placeholder for the states of all the task instances.
ke_state_t streamdatad_state[STREAMDATAD_IDX_MAX]; // __attribute__((section("retention_mem_area0"),zero_init)); //@RETENTION MEMORY

#endif /* BLE_STREAMDATA_DEVICE */
/// @} STREAMDATADTASK
