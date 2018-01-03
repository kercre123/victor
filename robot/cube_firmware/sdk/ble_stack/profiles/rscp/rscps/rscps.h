/**
 ****************************************************************************************
 *
 * @file rscps.h
 *
 * @brief Header file - Running Speed and Cadence Profile Sensor.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 * $ Rev $
 *
 ****************************************************************************************
 */

#ifndef _RSCPS_H_
#define _RSCPS_H_

/**
 ****************************************************************************************
 * @addtogroup RSCPS Running Speed and Cadence Profile Sensor
 * @ingroup RSCP
 * @brief Running Speed and Cadence Profile Sensor
 * @{
 ****************************************************************************************
 */

/// RSCPS Running Speed and Cadence Profile Sensor Role
#define BLE_RSC_SENSOR          1
#if !defined (BLE_SERVER_PRF)
    #define BLE_SERVER_PRF      1
#endif

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rscp_common.h"

#if (BLE_RSC_SENSOR)

#include "prf_types.h"

/*
 * DEFINES
 ****************************************************************************************
 */

/********************************************
 ******* RSCS Configuration Flag Masks ******
 ********************************************/

/// Mandatory Attributes (RSC Measurement + RSC Feature)
#define RSCPS_MANDATORY_MASK               (0x003F)
/// Sensor Location Attributes
#define RSCPS_SENSOR_LOC_MASK              (0x00C0)
/// SC Control Point Attributes
#define RSCPS_SC_CTNL_PT_MASK              (0x0700)

/*
 * MACROS
 ****************************************************************************************
 */

#define RSCPS_IS_FEATURE_SUPPORTED(features, flag) ((features & flag) == flag)

#define RSCPS_IS_PRESENT(features, flag)           ((features & flag) == flag)

#define RSCPS_ENABLE_NTFIND(ccc_flag)              (rscps_env.prf_cfg |= ccc_flag)

#define RSCPS_DISABLE_NTFIND(ccc_flag)             (rscps_env.prf_cfg &= ~ccc_flag)

#define RSCPS_IS_NTFIND_ENABLED(ccc_flag)          ((rscps_env.prf_cfg & ccc_flag) == ccc_flag)

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// Running Speed and Cadence Service - Attribute List
enum rscps_rscs_att_list
{
    /// Running Speed and Cadence Service
    RSCS_IDX_SVC,
    /// RSC Measurement
    RSCS_IDX_RSC_MEAS_CHAR,
    RSCS_IDX_RSC_MEAS_VAL,
    RSCS_IDX_RSC_MEAS_NTF_CFG,
    /// RSC Feature
    RSCS_IDX_RSC_FEAT_CHAR,
    RSCS_IDX_RSC_FEAT_VAL,
    /// Sensor Location
    RSCS_IDX_SENSOR_LOC_CHAR,
    RSCS_IDX_SENSOR_LOC_VAL,
    /// SC Control Point
    RSCS_IDX_SC_CTNL_PT_CHAR,
    RSCS_IDX_SC_CTNL_PT_VAL,
    RSCS_IDX_SC_CTNL_PT_NTF_CFG,

    /// Number of attributes
    RSCS_IDX_NB,
};

/// Profile Configuration Additional Flags ()
enum rscps_prf_cfg_flag
{
    /// RSC Measurement - Client Char. Cfg
    RSCP_PRF_CFG_FLAG_RSC_MEAS_NTF    = (RSCP_FEAT_MULT_SENSOR_LOC_SUPP << 1),
    /// SC Control Point - Client Char. Cfg
    RSCP_PRF_CFG_FLAG_SC_CTNL_PT_IND  = (RSCP_PRF_CFG_FLAG_RSC_MEAS_NTF << 1),
};

/// Sensor Location Supported Flag
enum rscps_sensor_loc_supp
{
    /// Sensor Location Char. is not supported
    RSCPS_SENSOR_LOC_NOT_SUPP,
    /// Sensor Location Char. is supported
    RSCPS_SENSOR_LOC_SUPP,
};


/// Pointer to the connection clean-up function
#define RSCPS_CLEANUP_FNCT        (NULL)

/*
 * STRUCTURES
 ****************************************************************************************
 */

/// Running Speed and Cadence Profile Sensor environment variable
struct rscps_env_tag
{
    /// Connection Information
    struct prf_con_info con_info;

    /// Running Speed and Cadence Service Start Handle
    uint16_t shdl;
    /// Characteristic Handle offsets
    uint8_t hdl_offset[RSCP_RSCS_CHAR_MAX];

    /// Operation
    uint8_t operation;

    /**
     * Profile Configuration
     *      Bit 0: Instantaneous Stride Length Measurement Supported
     *      Bit 1: Total Distance Measurement Supported
     *      Bit 2: Walking or Running Status Supported
     *      Bit 3: Calibration Procedure Supported
     *      Bit 4: Multiple Sensor Locations Supported
     *
     *      Bit 5: RSC Measurement - Client Char. Cfg
     *      Bit 6: SC Control Point - Client Char. Cfg
     */
    uint16_t prf_cfg;
};

/*
 * GLOBAL VARIABLE DECLARATIONS
 ****************************************************************************************
 */

/// Running Speed and Cadence Sensor Environment Variable
extern struct rscps_env_tag rscps_env;

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
void rscps_init(void);

/**
 ****************************************************************************************
 * @brief Send a response indication without parameters to the peer device.
 ****************************************************************************************
 */
void rscps_send_rsp_ind(uint16_t handle, uint8_t req_op_code, uint8_t status);

/**
 ****************************************************************************************
 * @brief Send a RSCPS_CMP_EVT message to the application.
 ****************************************************************************************
 */
void rscps_send_cmp_evt(uint8_t src_id, uint8_t dest_id, uint16_t conhdl,
                        uint8_t operation, uint8_t status);

/**
 ****************************************************************************************
 * @brief Disable actions grouped in getting back to IDLE and sending configuration to
 * requester task
 ****************************************************************************************
 */
void rscps_disable(uint16_t conhdl);

#endif //(BLE_RSC_SENSOR)

/// @} RSCPS

#endif //(_RSCPS_H_)
