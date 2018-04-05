/**
 ****************************************************************************************
 *
 * @file sample128.c
 *
 * @brief 128 UUID service. Sample code
 *
 * Copyright (C) 2013 Dialog Semiconductor GmbH and its Affiliates, unpublished work
 * This computer program includes Confidential, Proprietary Information and is a Trade Secret 
 * of Dialog Semiconductor GmbH and its Affiliates. All use, disclosure, and/or 
 * reproduction is prohibited unless authorized in writing. All Rights Reserved.
 *
 ****************************************************************************************
 */


/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwble_config.h"

#if (BLE_SAMPLE128)

#include "sample128.h"
#include "sample128_task.h"
#include "attm_db.h"
#include "gapc.h"
/*
 *  SAMPLE128 PROFILE ATTRIBUTES VALUES DEFINTION
 ****************************************************************************************
 */

/// sample128_1 Service
const struct att_uuid_128 sample128_svc     = {{0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F}};

 
/// sample128_1 value attribute UUID
const struct att_uuid_128 sample128_1_val     = {{0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F}};

struct att_char128_desc sample128_1_char = {ATT_CHAR_PROP_RD | ATT_CHAR_PROP_WR,
                                                                    {0,0},
                                                                    {0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F}}; 

const struct att_uuid_128 sample128_2_val     = {{0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F}};

struct att_char128_desc sample128_2_char = {ATT_CHAR_PROP_RD | ATT_CHAR_PROP_NTF,
                                                                    {0,0},
                                                                    {0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F}}; 


/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

struct sample128_env_tag sample128_env __attribute__((section("retention_mem_area0"),zero_init));

static const struct ke_task_desc TASK_DESC_SAMPLE128 = {sample128_state_handler, &sample128_default_handler, sample128_state, SAMPLE128_STATE_MAX, SAMPLE128_IDX_MAX};

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

void sample128_init(void)
{
    // Reset Environment
    memset(&sample128_env, 0, sizeof(sample128_env));
    
    // Create SAMPLE128 task
    ke_task_create(TASK_SAMPLE128, &TASK_DESC_SAMPLE128);

    ke_state_set(TASK_SAMPLE128, SAMPLE128_DISABLED);
}

void sample128_send_val(uint8_t val)
{
    // Allocate the alert value change indication
   struct sample128_val_ind *ind = KE_MSG_ALLOC(SAMPLE128_VAL_IND,
                                              sample128_env.con_info.appid, TASK_SAMPLE128,
                                              sample128_val_ind);
   // Fill in the parameter structure
   ind->conhdl = gapc_get_conhdl(sample128_env.con_info.conidx);
   ind->val = val;
   
   // Send the message
   ke_msg_send(ind);
}

void sample128_disable(void)
{
    att_size_t length;
    uint8_t *alert_lvl;

    // Disable service in database
    attmdb_svc_set_permission(sample128_env.sample128_shdl, PERM_RIGHT_DISABLE);

    struct sample128_disable_ind *ind = KE_MSG_ALLOC(SAMPLE128_DISABLE_IND,
                                                 sample128_env.con_info.appid, TASK_SAMPLE128,
                                                 sample128_disable_ind);

    //Get value stored in DB
    attmdb_att_get_value(sample128_env.sample128_shdl + SAMPLE128_1_IDX_VAL,
                         &length, &alert_lvl);

    // Fill in the parameter structure
    ind->conhdl     = gapc_get_conhdl(sample128_env.con_info.conidx);

    // Send the message
    ke_msg_send(ind);

    // Go to idle state
    ke_state_set(TASK_SAMPLE128, SAMPLE128_IDLE);
}


void sample128_upd_char2_cfm_send(uint8_t status)
{
    struct sample128_upd_char2_cfm *cfm = KE_MSG_ALLOC(SAMPLE128_UPD_CHAR2_CFM,
                                                 sample128_env.con_info.appid, TASK_SAMPLE128,
                                                 sample128_upd_char2_cfm);

    cfm->status = status;
    
    // Send the message
    ke_msg_send(cfm);

}
#endif //BLE_SAMPLE128
