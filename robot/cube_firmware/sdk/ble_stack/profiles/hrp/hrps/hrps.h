/**
 ****************************************************************************************
 *
 * @file hrps.h
 *
 * @brief Header file - Heart Rate Profile Sensor.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 * $Rev$
 *
 ****************************************************************************************
 */

#ifndef _HRPS_H_
#define _HRPS_H_

/**
 ****************************************************************************************
 * @addtogroup HRPS Heart Rate Profile Sensor
 * @ingroup HRP
 * @brief Heart Rate Profile Sensor
 *
 * Heart Rate sensor (HRS) profile provides functionalities to upper layer module
 * application. The device using this profile takes the role of Heart Rate sensor.
 *
 * The interface of this role to the Application is:
 *  - Enable the profile role (from APP)
 *  - Disable the profile role (from APP)
 *  - Notify peer device during Heart Rate measurement (from APP)
 *  - Indicate measurements performed to peer device (from APP)
 *
 * Profile didn't manages multiple users configuration and storage of offline measurements.
 * This must be handled by application.
 *
 * Heart Rate Profile Sensor. (HRPS): A HRPS (e.g. PC, phone, etc)
 * is the term used by this profile to describe a device that can perform Heart Rate
 * measurement and notify about on-going measurement and indicate final result to a peer
 * BLE device.
 *
 * @{
 ****************************************************************************************
 */

/// Heart Rate Profile Sensor Role
#define BLE_HR_SENSOR           1
#if !defined (BLE_SERVER_PRF)
    #define BLE_SERVER_PRF      1
#endif


/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#if (BLE_HR_SENSOR)
#include "hrp_common.h"
#include "prf_types.h"

/*
 * DEFINES
 ****************************************************************************************
 */

#define HRPS_HT_MEAS_MAX_LEN            (5 + (2 * HRS_MAX_RR_INTERVAL))

#define HRPS_MANDATORY_MASK             (0x0F)
#define HRPS_BODY_SENSOR_LOC_MASK       (0x30)
#define HRPS_HR_CTNL_PT_MASK            (0xC0)

/*
 * MACROS
 ****************************************************************************************
 */

#define HRPS_IS_SUPPORTED(mask) \
    (((hrps_env.features & mask) == mask))

///Attributes State Machine
enum
{
    HRS_IDX_SVC,

    HRS_IDX_HR_MEAS_CHAR,
    HRS_IDX_HR_MEAS_VAL,
    HRS_IDX_HR_MEAS_NTF_CFG,

    HRS_IDX_BOBY_SENSOR_LOC_CHAR,
    HRS_IDX_BOBY_SENSOR_LOC_VAL,

    HRS_IDX_HR_CTNL_PT_CHAR,
    HRS_IDX_HR_CTNL_PT_VAL,

    HRS_IDX_NB,
};

enum
{
    HRPS_HR_MEAS_CHAR,
    HRPS_BODY_SENSOR_LOC_CHAR,
    HRPS_HR_CTNL_PT_CHAR,

    HRPS_CHAR_MAX,
};

enum
{
    /// Body Sensor Location Feature Supported
    HRPS_BODY_SENSOR_LOC_CHAR_SUP       = 0x01,
    /// Energy Expanded Feature Supported
    HRPS_ENGY_EXP_FEAT_SUP              = 0x02,

    HRPS_HR_MEAS_NTF_CFG                = 0x04,
};


/// Pointer to the connection clean-up function
#define HRPS_CLEANUP_FNCT        (NULL)

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

/// Heart Rate Profile Sensor environment variable
struct hrps_env_tag
{
    /// Connection Info
    struct prf_con_info con_info;

    ///Service Start Handle
    uint16_t shdl;
    ///Database configuration
    uint8_t features;
};


/*
 * GLOBAL VARIABLE DECLARATIONS
 ****************************************************************************************
 */

extern const struct attm_desc hrps_att_db[HRS_IDX_NB];

/// Heart Rate Sensor Service - only one instance for now
extern const att_svc_desc_t hrps_hrs_svc;

extern const struct att_char_desc hrps_hr_meas_char;
extern const struct att_char_desc hrps_body_sensor_loc_char;
extern const struct att_char_desc hrps_hr_cntl_point_char;

extern struct hrps_env_tag hrps_env;

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Initialization of the HRPS module.
 * This function performs all the initializations of the HRPS module.
 ****************************************************************************************
 */
void hrps_init(void);

/**
 ****************************************************************************************
 * @brief Send a HRPS_MEAS_SEND_CFM message to the application.
 *
 * @param[in] status Confirmation Status
 ****************************************************************************************
 */
void hrps_meas_send_cfm_send(uint8_t status);

/**
 ****************************************************************************************
 * @brief Pack Heart Rate measurement value
 *
 * @param[in] p_meas_val Heart Rate measurement value
 ****************************************************************************************
 */
uint8_t hrps_pack_meas_value(uint8_t *packed_hr, const struct hrs_hr_meas* pmeas_val);

/**
 ****************************************************************************************
 * @brief Disable actions grouped in getting back to IDLE and sending configuration to requester task
 ****************************************************************************************
 */
void hrps_disable(uint16_t conhdl);

#endif /* #if (BLE_HR_SENSOR) */

/// @} HRPS

#endif /* _HRPS_H_ */
