/**
 ****************************************************************************************
 *
 * @file blps.h
 *
 * @brief Header file - Blood Pressure Profile Sensor.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 * $Rev$
 *
 ****************************************************************************************
 */

#ifndef _BLPS_H_
#define _BLPS_H_

/**
 ****************************************************************************************
 * @addtogroup BLPS Blood Pressure Profile Sensor
 * @ingroup BLP
 * @brief Blood Pressure Profile Sensor
 *
 * Blood pressure sensor (BPS) profile provides functionalities to upper layer module
 * application. The device using this profile takes the role of Blood pressure sensor.
 *
 * The interface of this role to the Application is:
 *  - Enable the profile role (from APP)
 *  - Disable the profile role (from APP)
 *  - Notify peer device during Blood Pressure measurement (from APP)
 *  - Indicate measurements performed to peer device (from APP)
 *
 * Profile didn't manages multiple users configuration and storage of offline measurements.
 * This must be handled by application.
 *
 * Blood Pressure Profile Sensor. (BLPS): A BLPS (e.g. PC, phone, etc)
 * is the term used by this profile to describe a device that can perform blood pressure
 * measurement and notify about on-going measurement and indicate final result to a peer
 * BLE device.
 *
 * @{
 ****************************************************************************************
 */

/// Blood Pressure Profile Sensor Role
#define BLE_BP_SENSOR       1
#if !defined (BLE_SERVER_PRF)
    #define BLE_SERVER_PRF      1
#endif

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#if (BLE_BP_SENSOR)

#include "blp_common.h"
#include "prf_types.h"


/*
 * DEFINES
 ****************************************************************************************
 */

#define BLPS_BP_MEAS_MAX_LEN            (19)

///BPS Configuration Flag Masks
#define BLPS_MANDATORY_MASK                (0x003F)
#define BLPS_INTM_CUFF_PRESS_MASK        (0x01C0)

/*
 * MACROS
 ****************************************************************************************
 */

#define BLPS_IS_SUPPORTED(mask) \
    (((blps_env.features & mask) == mask))

#define BLPS_IS_ENABLED(mask) \
    (((blps_env.evt_cfg & mask) == mask))

///Attributes State Machine
enum
{
    BPS_IDX_SVC,

    BPS_IDX_BP_MEAS_CHAR,
    BPS_IDX_BP_MEAS_VAL,
    BPS_IDX_BP_MEAS_IND_CFG,

    BPS_IDX_BP_FEATURE_CHAR,
    BPS_IDX_BP_FEATURE_VAL,

    BPS_IDX_INTM_CUFF_PRESS_CHAR,
    BPS_IDX_INTM_CUFF_PRESS_VAL,
    BPS_IDX_INTM_CUFF_PRESS_NTF_CFG,

    BPS_IDX_NB,
};

///Characteristic Codes
enum
{
    BPS_BP_MEAS_CHAR,
    BPS_BP_FEATURE_CHAR,
    BPS_INTM_CUFF_MEAS_CHAR,
};

///Database Configuration Bit Field Flags
enum
{
    /// support of Intermediate Cuff Pressure
    BLPS_INTM_CUFF_PRESS_SUP        = 0x01,
};

/// Indication/notification configuration (put in feature flag to optimize memory usage)
enum
{
    /// Bit used to know if blood pressure measurement indication is enabled
    BLPS_BP_MEAS_IND_CFG            = 0x02,
    /// Bit used to know if cuff pressure measurement notification is enabled
    BLPS_INTM_CUFF_PRESS_NTF_CFG    = 0x04,
    /// indication/notification config mask
    BLPS_NTFIND_MASK                = 0x06,
};

/// Pointer to the connection clean-up function
#define BLPS_CLEANUP_FNCT        (NULL)
/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

/// Blood Pressure Profile Sensor environment variable
struct blps_env_tag
{
    ///Connection Information
    struct prf_con_info con_info;

    ///Blood Pressure Service Start Handle
    uint16_t shdl;

    ///Database Configuration
    uint8_t features;

    ///Event (notification/indication) config
    uint8_t evt_cfg;
};



/*
 * GLOBAL VARIABLE DECLARATIONS
 ****************************************************************************************
 */

extern const struct attm_desc blps_att_db[BPS_IDX_NB];

/// Blood Pressure Sensor Service - only one instance for now
extern const att_svc_desc_t blps_bps_svc;

extern const struct att_char_desc blps_bp_meas_char;
extern const struct att_char_desc blps_interm_cuff_press_char;
extern const struct att_char_desc blps_bp_feature_char;

extern struct blps_env_tag blps_env;
/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Initialization of the BLPS module.
 * This function performs all the initializations of the BLPS module.
 ****************************************************************************************
 */
void blps_init(void);

/**
 ****************************************************************************************
 * @brief Pack Blood Pressure measurement value
 *
 * @param p_meas_val Blood Pressure measurement value
 *
 * @return size of packed value
 ****************************************************************************************
 */
uint8_t blps_pack_meas_value(uint8_t *packed_bp, const struct bps_bp_meas* pmeas_val);

/**
 ****************************************************************************************
 * @brief Send a BLPS_MEAS_SEND_CFM to the application
 ****************************************************************************************
 */
void blps_meas_send_cfm_send(uint8_t status);

/**
 ****************************************************************************************
 * @brief Disable actions grouped in getting back to IDLE and sending configuration to requester task
 ****************************************************************************************
 */
void blps_disable(uint16_t conhdl); 

#endif /* #if (BLE_BP_SENSOR) */

/// @} BLPS

#endif /* _BLPS_H_ */
