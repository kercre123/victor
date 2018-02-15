/**
 ****************************************************************************************
 *
 * @file wpts.h
 *
* @brief WPTS service. 
 *
 * Copyright (C) 2013 Dialog Semiconductor GmbH and its Affiliates, unpublished work
 * This computer program includes Confidential, Proprietary Information and is a Trade Secret 
 * of Dialog Semiconductor GmbH and its Affiliates. All use, disclosure, and/or 
 * reproduction is prohibited unless authorized in writing. All Rights Reserved.
 *
 ****************************************************************************************
 */

#ifndef WPTS_H_
#define WPTS_H_

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#if (BLE_WPTS)

#include "wpt_common.h"
#include "prf_types.h"
#include "ke_task.h"
#include "atts.h"


/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// Handles offsets
enum
{
    WPTS_IDX_SVC,

    WPTS_CHARGING_PRU_CTRL_IDX_CHAR,
    WPTS_CHARGING_PRU_CTRL_IDX_VAL,

    WPTS_CHARGING_PTU_STATIC_IDX_CHAR,
    WPTS_CHARGING_PTU_STATIC_IDX_VAL,
    
    WPTS_CHARGING_PRU_ALERT_IDX_CHAR,
    WPTS_CHARGING_PRU_ALERT_IDX_VAL,
    WPTS_CHARGING_PRU_ALERT_IDX_CFG,
    
    WPTS_CHARGING_PRU_STATIC_IDX_CHAR,
    WPTS_CHARGING_PRU_STATIC_IDX_VAL,
    
    WPTS_CHARGING_PRU_DYNAMIC_IDX_CHAR,
    WPTS_CHARGING_PRU_DYNAMIC_IDX_VAL,
    
    WPTS_IDX_NB,
};

/// Characteristics Code for Write Indications
enum
{
    WPTS_ERR_CHAR,
    WPTS_CHAR,
};

/// WPTS features
enum
{
    WPTS_FEATURE_ALERT_NTF_ACTIVE = 0x01,
    WPTS_FEATURE_ALERT_IND_ACTIVE = 0x02,
};
/// WPTS Errors cached on profile layer
enum
{
    WPTS_ERROR_WR_PTU_CONTROL_CHANGE_CMD,
    WPTS_ERROR_WR_PTU_STATIC_VAL_CMD,
};

#define ATT_SVC_WPT     0xFFFE
/*
 * STRUCTURES
 ****************************************************************************************
 */

/// wpts environment variable
struct wpts_env_tag
{
    /// Connection Information
    struct prf_con_info con_info;
    /// LLS Start Handle
    uint16_t wpt_shdl;
    /// Database configuration
    uint8_t features;
};

#define WPTS_IS_SUPPORTED(mask)     (((wpts_env.features & mask) == mask))

/*
 * WPTS PROFILE ATTRIBUTES DECLARATION
 ****************************************************************************************
 */

/// WPTS service Description
//extern const struct atts_desc wpts_1_att_db[wpts_1_IDX_NB];

/*
 *  WPTS PROFILE ATTRIBUTES VALUES DECLARATION
 ****************************************************************************************
 */

extern const att_svc_desc_t wpt_svc;
 
extern struct att_char128_desc wpt_charging_ptu_ctrl_char;

extern struct att_char128_desc wpt_ptu_static_char;

extern struct att_char128_desc wpt_charging_pru_alert_char;
                                                                                                                                        
extern struct att_char128_desc wpt_charging_pru_static_char;

extern struct att_char128_desc wpt_charging_pru_dynamic_char;

/*
 * GLOBAL VARIABLE DECLARATIONS
 ****************************************************************************************
 */

extern struct wpts_env_tag wpts_env;

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Initialization of the wpts module.
 * This function performs all the initializations of the wpts module.
 ****************************************************************************************
 */
void wpts_init(void);

/**
 ****************************************************************************************
 * @brief Disable role.
 ****************************************************************************************
 */
void wpts_disable(void);

void wpts_alert_send_cfm_send(ke_task_id_t const dest_id, uint8_t status);

void wpts_static_val_cfm_send(ke_task_id_t const dest_id, uint8_t status);

void wpts_dynamic_val_cfm_send(ke_task_id_t const dest_id, uint8_t status);

void wpts_error_ind_send(ke_task_id_t const dest_id, uint8_t const error_id, uint8_t reason);

#endif //BLE_WPTS

#endif // WPTS_H_
