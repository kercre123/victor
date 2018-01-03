/**
 ****************************************************************************************
 *
 * @file adc_notify.h
 *
* @brief 128 UUID service. sample code
 *
 * Copyright (C) 2013 Dialog Semiconductor GmbH and its Affiliates, unpublished work
 * This computer program includes Confidential, Proprietary Information and is a Trade Secret 
 * of Dialog Semiconductor GmbH and its Affiliates. All use, disclosure, and/or 
 * reproduction is prohibited unless authorized in writing. All Rights Reserved.
 *
 ****************************************************************************************
 */

#ifndef ADC_NOTIFY_H_
#define ADC_NOTIFY_H_


/// ADC Notify Server Role
#define BLE_ADC_NOTIFY              1
#if !defined (BLE_SERVER_PRF)
    #define BLE_SERVER_PRF          1
#endif

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#if (BLE_ADC_NOTIFY)

#include "ke_task.h"
#include "atts.h"
#include "prf_types.h"

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// Handles offsets
enum
{
    ADC_NOTIFY_1_IDX_SVC,

    ADC_NOTIFY_IDX_CHAR,
    ADC_NOTIFY_IDX_VAL,
    ADC_NOTIFY_IDX_CFG,
    
    ADC_NOTIFY_1_IDX_NB,
};

///Characteristics Code for Write Indications
enum
{
    ADC_NOTIFY_ERR_CHAR,
    ADC_NOTIFY_CFG,
};

/*
 * STRUCTURES
 ****************************************************************************************
 */

/// adc_notify environment variable
struct adc_notify_env_tag
{
    /// Connection Information
    struct prf_con_info con_info;

    /// ADC Notify  svc Start Handle
    uint16_t adc_notify_shdl;
    
    //Notification property status
    uint8_t feature;

};

/*
 *  ADC_NOTIFY PROFILE ATTRIBUTES VALUES DECLARATION
 ****************************************************************************************
 */

/// adc_notify Service
extern const struct att_uuid_128 adc_notify_svc;
/// adc_notify - Characteristic
extern struct att_char128_desc adc_notify_char;
/// adc_notify - Value
extern const struct att_uuid_128 adc_notify_val;

/*
 * GLOBAL VARIABLE DECLARATIONS
 ****************************************************************************************
 */

extern struct adc_notify_env_tag adc_notify_env;
/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Initialization of the ADC_NOTIFY module.
 * This function performs all the initializations of the ADC_NOTIFY module.
 ****************************************************************************************
 */
void adc_notify_init(void);

/**
 ****************************************************************************************
 * @brief Send configuration's value change to application.
 * @param val Value.
 ****************************************************************************************
 */
 
void adc_notify_send_cfg(uint16_t val);

/**
 ****************************************************************************************
 * @brief Disable role.
 ****************************************************************************************
 */
void adc_notify_disable(void);

/**
 ****************************************************************************************
 * @brief Send update char confirmation message
 ****************************************************************************************
 */
void adc_notify_upd_char_cfm_send(uint8_t status);
#endif //BLE_ADC_NOTIFY

#endif // ADC_NOTIFY_H_
