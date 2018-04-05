/**
 ****************************************************************************************
 *
 * @file cscps.h
 *
 * @brief Header file - Cycling Speed and Cadence Profile Sensor.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 * $ Rev $
 *
 ****************************************************************************************
 */

#ifndef _CSCPS_H_
#define _CSCPS_H_

/**
 ****************************************************************************************
 * @addtogroup CSCPS Cycling Speed and Cadence Profile Sensor
 * @ingroup CSCP
 * @brief Cycling Speed and Cadence Profile Sensor
 * @{
 ****************************************************************************************
 */


/// CSCPS Cycling Speed and Cadence Profile Sensor Role
#define BLE_CSC_SENSOR          1
#if !defined (BLE_SERVER_PRF)
    #define BLE_SERVER_PRF      1
#endif


/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "cscp_common.h"

#if (BLE_CSC_SENSOR)

#include "prf_types.h"

/*
 * DEFINES
 ****************************************************************************************
 */

/********************************************
 ******* CSCPS Configuration Flag Masks ******
 ********************************************/

/// Mandatory Attributes (CSC Measurement + CSC Feature)
#define CSCPS_MANDATORY_MASK               (0x003F)
/// Sensor Location Attributes
#define CSCPS_SENSOR_LOC_MASK              (0x00C0)
/// SC Control Point Attributes
#define CSCPS_SC_CTNL_PT_MASK              (0x0700)

/*
 * MACROS
 ****************************************************************************************
 */

#define CSCPS_IS_FEATURE_SUPPORTED(features, flag) ((features & flag) == flag)

#define CSCPS_IS_PRESENT(features, flag)           ((features & flag) == flag)

#define CSCPS_ENABLE_NTFIND(ccc_flag)              (cscps_env.prf_cfg |= ccc_flag)

#define CSCPS_DISABLE_NTFIND(ccc_flag)             (cscps_env.prf_cfg &= ~ccc_flag)

#define CSCPS_IS_NTFIND_ENABLED(ccc_flag)          ((cscps_env.prf_cfg & ccc_flag) == ccc_flag)

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// Cycling Speed and Cadence Service - Attribute List
enum cscps_cscs_att_list
{
    /// Cycling Speed and Cadence Service
    CSCS_IDX_SVC,
    /// CSC Measurement
    CSCS_IDX_CSC_MEAS_CHAR,
    CSCS_IDX_CSC_MEAS_VAL,
    CSCS_IDX_CSC_MEAS_NTF_CFG,
    /// CSC Feature
    CSCS_IDX_CSC_FEAT_CHAR,
    CSCS_IDX_CSC_FEAT_VAL,
    /// Sensor Location
    CSCS_IDX_SENSOR_LOC_CHAR,
    CSCS_IDX_SENSOR_LOC_VAL,
    /// SC Control Point
    CSCS_IDX_SC_CTNL_PT_CHAR,
    CSCS_IDX_SC_CTNL_PT_VAL,
    CSCS_IDX_SC_CTNL_PT_NTF_CFG,

    /// Number of attributes
    CSCS_IDX_NB,
};

/// Profile Configuration Additional Flags ()
enum cscps_prf_cfg_flag
{
    /// CSC Measurement - Client Char. Cfg
    CSCP_PRF_CFG_FLAG_CSC_MEAS_NTF    = (CSCP_FEAT_MULT_SENSOR_LOC_SUPP << 1),
    /// SC Control Point - Client Char. Cfg
    CSCP_PRF_CFG_FLAG_SC_CTNL_PT_IND  = (CSCP_PRF_CFG_FLAG_CSC_MEAS_NTF << 1),
};

/// Sensor Location Supported Flag
enum cscps_sensor_loc_supp
{
    /// Sensor Location Char. is not supported
    CSCPS_SENSOR_LOC_NOT_SUPP,
    /// Sensor Location Char. is supported
    CSCPS_SENSOR_LOC_SUPP,
};

/// Pointer to the connection clean-up function
#define CSCPS_CLEANUP_FNCT        (NULL)
/*
 * STRUCTURES
 ****************************************************************************************
 */

/// Cycling Speed and Cadence Profile Sensor environment variable
struct cscps_env_tag
{
    /// Connection Information
    struct prf_con_info con_info;

    /// Cycling Speed and Cadence Service Start Handle
    uint16_t shdl;
    /// Characteristic Handle offsets
    uint8_t hdl_offset[CSCP_CSCS_CHAR_MAX];

    /// Operation
    uint8_t operation;

    /**
     * Profile Configuration
     *      Bit 0: Wheel Revolution Data Supported
     *      Bit 1: Crank Revolution Data Supported
     *      Bit 2: Multiple Sensor Location Supported
     *
     *      Bit 5: CSC Measurement - Client Char. Cfg
     *      Bit 6: SC Control Point - Client Char. Cfg
     */
    uint16_t prf_cfg;

    /// Wheel revolution
    uint32_t wheel_revol;
};

/*
 * GLOBAL VARIABLE DECLARATIONS
 ****************************************************************************************
 */

/// Cycling Speed and Cadence Sensor Environment Variable
extern struct cscps_env_tag cscps_env;

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Initialization of the GLPS module.
 * This function performs all the initializations of the GLPS module.
 ****************************************************************************************
 */
void cscps_init(void);

/**
 ****************************************************************************************
 * @brief Send a response indication without parameters to the peer device.
 ****************************************************************************************
 */
void cscps_send_rsp_ind(uint16_t handle, uint8_t req_op_code, uint8_t status);

/**
 ****************************************************************************************
 * @brief Send a CSCPS_CMP_EVT message to the application.
 ****************************************************************************************
 */
void cscps_send_cmp_evt(uint8_t src_id, uint8_t dest_id, uint16_t conhdl,
                        uint8_t operation, uint8_t status);

/**
 ****************************************************************************************
 * @brief Disable actions grouped in getting back to IDLE and sending configuration to
 * requester task
 ****************************************************************************************
 */
void cscps_disable(uint16_t conhdl);

#endif //(BLE_CSC_SENSOR)

/// @} CSCPS

#endif //(_CSCPS_H_)
