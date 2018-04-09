/**
 ****************************************************************************************
 *
 * @file adc_notify.c
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

#if (BLE_ADC_NOTIFY)

#include "adc_notify.h"
#include "adc_notify_task.h"
#include "attm_db.h"
#include "gapc.h"
/*
 *  ADC_NOTIFY PROFILE ATTRIBUTES VALUES DEFINTION
 ****************************************************************************************
 */

/// adc_notify_1 Service
const struct att_uuid_128 adc_notify_svc     = {{0x4D, 0x65, 0x08, 0x10, 0x64, 0x22, 0x11, 0xE4, 0x98, 0x03, 0x08, 0x00, 0x20, 0x0C, 0x9A, 0x66}};

const struct att_uuid_128 adc_notify_val     = {{0x4E, 0x65, 0x08, 0x10, 0x64, 0x22, 0x11, 0xE4, 0x98, 0x03, 0x08, 0x00, 0x20, 0x0C, 0x9A, 0x66}};

struct att_char128_desc adc_notify_char = {ATT_CHAR_PROP_RD | ATT_CHAR_PROP_NTF,
                                                                    {0,0},
                                                                    {0x4E, 0x65, 0x08, 0x10, 0x64, 0x22, 0x11, 0xE4, 0x98, 0x03, 0x08, 0x00, 0x20, 0x0C, 0x9A, 0x66}}; 


/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

struct adc_notify_env_tag adc_notify_env __attribute__((section("retention_mem_area0"),zero_init));

static const struct ke_task_desc TASK_DESC_ADC_NOTIFY = {adc_notify_state_handler, &adc_notify_default_handler, adc_notify_state, ADC_NOTIFY_STATE_MAX, ADC_NOTIFY_IDX_MAX};

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief ADC notify task creation
 * @return void
 ****************************************************************************************
 */
void adc_notify_init(void)
{
    // Reset Environment
    memset(&adc_notify_env, 0, sizeof(adc_notify_env));
    
    // Create ADC_NOTIFY task
    ke_task_create(TASK_ADC_NOTIFY, &TASK_DESC_ADC_NOTIFY);

    ke_state_set(TASK_ADC_NOTIFY, ADC_NOTIFY_DISABLED);
}

/**
 ****************************************************************************************
 * @brief Send indication to application task that characteristic's client 
 * configuration has been written
 * @return void
 ****************************************************************************************
 */
void adc_notify_send_cfg(uint16_t val)
{
    // Allocate the alert value change indication
   struct adc_notify_cfg_ind *ind = KE_MSG_ALLOC(ADC_NOTIFY_CFG_IND,
                                              adc_notify_env.con_info.appid, TASK_ADC_NOTIFY,
                                              adc_notify_cfg_ind);
   // Fill in the parameter structure
   ind->conhdl = gapc_get_conhdl(adc_notify_env.con_info.conidx);
   ind->val = val;
   
   // Send the message
   ke_msg_send(ind);
}

/**
 ****************************************************************************************
 * @brief ADC notify service diabled. Send indication to application task.
 * @return void
 ****************************************************************************************
 */
void adc_notify_disable(void)
{
        
    // Disable service in database
    attmdb_svc_set_permission(adc_notify_env.adc_notify_shdl, PERM_RIGHT_DISABLE);

    struct adc_notify_disable_ind *ind = KE_MSG_ALLOC(ADC_NOTIFY_DISABLE_IND,
                                                 adc_notify_env.con_info.appid, TASK_ADC_NOTIFY,
                                                 adc_notify_disable_ind);

   // Fill in the parameter structure
    ind->conhdl     = gapc_get_conhdl(adc_notify_env.con_info.conidx);

    // Send the message
    ke_msg_send(ind);

    // Go to idle state
    ke_state_set(TASK_ADC_NOTIFY, ADC_NOTIFY_IDLE);
}

/**
 ****************************************************************************************
 * @brief Send confirmation to application task that update characteristic data has been written.
 * @return void
 ****************************************************************************************
 */
void adc_notify_upd_char_cfm_send(uint8_t status)
{
    struct adc_notify_upd_char_cfm *cfm = KE_MSG_ALLOC(ADC_NOTIFY_UPD_CHAR_CFM,
                                                 adc_notify_env.con_info.appid, TASK_ADC_NOTIFY,
                                                 adc_notify_upd_char_cfm);

    cfm->status = status;
    
    // Send the message
    ke_msg_send(cfm);

}
#endif //BLE_ADC_NOTIFY
