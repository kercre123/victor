/**
 ****************************************************************************************
 *
 * @file htpt_task.h
 *
 * @brief Header file - HTPTTASK.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 *
 ****************************************************************************************
 */

#ifndef HTPT_TASK_H_
#define HTPT_TASK_H_

/**
 ****************************************************************************************
 * @addtogroup HTPTTASK Task
 * @ingroup HTPT
 * @brief Health Thermometer Profile Thermometer Task
 *
 * The HTPTTASK is responsible for handling the messages coming in and out of the
 * @ref HTPT reporter block of the BLE Host.
 *
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#if (BLE_HT_THERMOM)
#include <stdint.h>
#include "ke_task.h"
#include "htpt.h"
#include "prf_types.h"

/*
 * DEFINES
 ****************************************************************************************
 */

#define HTPT_IDX_MAX        (1)

/// Possible states of the HTPT task
enum
{
    ///Disabled state
    HTPT_DISABLED,
    ///Idle state
    HTPT_IDLE,
    ///Connected state
    HTPT_CONNECTED,

    ///Number of defined states.
    HTPT_STATE_MAX
};

/// Messages for Health Thermometer Profile Thermometer
enum
{
    ///Add a HTS instance into the database
    HTPT_CREATE_DB_REQ = KE_FIRST_MSG(TASK_HTPT),
    ///inform APP of database creation status
    HTPT_CREATE_DB_CFM,

    ///Start the Health Thermometer Profile Thermometer profile - at connection
    HTPT_ENABLE_REQ,

    /// Disable confirmation with configuration to save after profile disabled
    HTPT_DISABLE_IND,

    /// Error indication to Host
    HTPT_ERROR_IND,

    ///Send temperature value from APP
    HTPT_TEMP_SEND_REQ,
    ///Send temperature value confirm to APP so stable values can be erased if correctly sent.
    HTPT_TEMP_SEND_CFM,

    ///Indicate Measurement Interval
    HTPT_MEAS_INTV_UPD_REQ,
    ///Inform APP of new measurement interval value
    HTPT_MEAS_INTV_CHG_IND,

    ///Inform APP of new configuration value
    HTPT_CFG_INDNTF_IND,

    ///Update Temperature Type Value
    HTPT_TEMP_TYPE_UPD_REQ,
};

enum
{
    HTPT_THERM_TEMP_SEND,
    HTPT_CENTRAL_IND_CFM
};

/*
 * API MESSAGES STRUCTURES
 ****************************************************************************************
 */

/// Parameters of the @ref HTPT_CREATE_DB_REQ message
struct htpt_create_db_req
{
    ///Measurement Interval Valid Range - Minimal Value
    uint16_t valid_range_min;
    ///Measurement Interval Valid Range - Maximal Value
    uint16_t valid_range_max;

    ///Database configuration
    uint8_t features;
};

/// Parameters of the @ref HTPT_CREATE_DB_CFM message
struct htpt_create_db_cfm
{
    ///Status
    uint8_t status;
};

/// Parameters of the @ref HTPT_ENABLE_REQ message
struct htpt_enable_req
{
    ///Connection handle
    uint16_t conhdl;
    /// security level: b0= nothing, b1=unauthenticated, b2=authenticated, b3=authorized; b1 or b2 and b3 can go together
    uint8_t sec_lvl;
    ///Type of connection - will someday depend on button press length; can be CFG or NORMAL
    uint8_t con_type;

    ///Temperature measurement indication configuration
    uint16_t temp_meas_ind_en;
    ///Intermediate temperature notification configuration
    uint16_t interm_temp_ntf_en;
    ///Measurement interval indication configuration
    uint16_t meas_intv_ind_en;
    ///Measurement interval
    uint16_t meas_intv;
};

///Parameters of the @ref HTPT_DISABLE_IND message
struct htpt_disable_ind
{
    ///Connection handle
    uint16_t conhdl;

    ///Temperature measurement indication configuration
    uint16_t temp_meas_ind_en;
    ///Intermediate temperature notification configuration
    uint16_t interm_temp_ntf_en;
    ///Measurement interval indication configuration
    uint16_t meas_intv_ind_en;
    ///Measurement interval
    uint16_t meas_intv;
};

///Parameters of the @ref HTPT_TEMP_UPD_REQ message
struct htpt_temp_send_req
{
    ///Temperature Measurement
    struct htp_temp_meas temp_meas;
    ///Connection handle
    uint16_t conhdl;
    ///Stable or intermediary type of temperature
    uint8_t flag_stable_meas;
};

///Parameters of the @ref HTPT_TEMP_UPD_CFM message
struct htpt_temp_send_cfm
{
    ///Connection handle
    uint16_t conhdl;
    ///Status
    uint8_t status;
    ///Confirmation Type
    uint8_t cfm_type;
};

///Parameters of the @ref HTPT_MEAS_INTV_UPD_REQ message
struct htpt_meas_intv_upd_req
{
    ///Connection handle
    uint16_t conhdl;
    ///Measurement Interval value
    uint16_t meas_intv;
};

///Parameters of the @ref HTPT_MEAS_INTV_CHG_IND message
struct htpt_meas_intv_chg_ind
{
    uint16_t intv;
};

///Parameters of the @ref HTPT_CFG_INDNTF_IND message
struct htpt_cfg_indntf_ind
{
    ///Connection handle
    uint16_t conhdl;
    ///Stop/notify/indicate value to configure into the peer characteristic
    uint16_t cfg_val;
    ///Own code for differentiating between Temperature Measurement, Intermediate Temperature and Measurement Interval chars
    uint8_t char_code;
};

///Parameters of the @ref HTPT_TEMP_TYPE_UPD_REQ message
struct htpt_temp_type_upd_req
{
    ///Temperature Type Value
    uint8_t value;
};

/*
 * TASK DESCRIPTOR DECLARATIONS
 ****************************************************************************************
 */
extern const struct ke_state_handler htpt_state_handler[HTPT_STATE_MAX];
extern const struct ke_state_handler htpt_default_handler;
extern ke_state_t htpt_state[HTPT_IDX_MAX];

#endif //BLE_HT_THERMOM

/// @} HTPTTASK
#endif // HTPT_TASK_H_
