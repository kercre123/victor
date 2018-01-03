/**
 ****************************************************************************************
 *
 * @file cpps.h
 *
 * @brief Header file - Cycling Power Profile Sensor.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 * $ Rev $
 *
 ****************************************************************************************
 */

#ifndef _CPPS_H_
#define _CPPS_H_

/**
 ****************************************************************************************
 * @addtogroup CPPS Cycling Power Profile Sensor
 * @ingroup CPP
 * @brief Cycling Power Profile Sensor
 * @{
 ****************************************************************************************
 */
 
/// CPPS Cycling Power Profile Sensor Role
#define BLE_CP_SENSOR           1
#if !defined (BLE_SERVER_PRF)
    #define BLE_SERVER_PRF      1
#endif


/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "cpp_common.h"

#if (BLE_CP_SENSOR)

#include "prf_types.h"

/*
 * DEFINES
 ****************************************************************************************
 */

/********************************************
 ******* CPPS Configuration Flag Masks ******
 ********************************************/

/// Mandatory Attributes (CP Measurement + CP Feature + CP Sensor Location)
#define CPPS_MANDATORY_MASK           (0x01EF)
/// Broadcast Attribute
#define CPPS_MEAS_BCST_MASK           (0x0010)
/// Vector Attributes
#define CPPS_VECTOR_MASK              (0x0E00)
/// Control Point Attributes
#define CPPS_CTNL_PT_MASK             (0x7000)

/// Broadcast supported flag
#define CPPS_BROADCASTER_SUPP_FLAG    (0x01)
/// Control Point supported flag
#define CPPS_CTNL_PT_CHAR_SUPP_FLAG    (0x02)


/*
 * MACROS
 ****************************************************************************************
 */

#define CPPS_IS_FEATURE_SUPPORTED(features, flag) ((features & flag) == flag)

#define CPPS_IS_PRESENT(features, flag)           ((features & flag) == flag)

#define CPPS_IS_SET(features, flag)                  (features & flag)

#define CPPS_IS_CLEAR(features, flag)              ((features & flag) == 0)

#define CPPS_ENABLE_NTF_IND_BCST(ccc_flag)              (cpps_env.prfl_ntf_ind_cfg |= ccc_flag)

#define CPPS_DISABLE_NTF_IND_BCST(ccc_flag)             (cpps_env.prfl_ntf_ind_cfg &= ~ccc_flag)

#define CPPS_IS_NTF_IND_BCST_ENABLED(ccc_flag)          ((cpps_env.prfl_ntf_ind_cfg & ccc_flag) == ccc_flag)

// MACRO TO CALCULATE HANDLE    shdl + idx - BCST - VECTOR
// BCST is 1 if the broadcast mode is supported otherwise 0
// VECTOR is 3 if the Vector characteristic is supported otherwise 0
#define CPPS_HANDLE(idx) \
    (cpps_env.shdl + (idx) - \
        ((!(CPPS_IS_FEATURE_SUPPORTED(cpps_env.prfl_cfg, CPPS_MEAS_BCST_MASK)) && \
                ((idx) > CPS_IDX_CP_MEAS_BCST_CFG))? (1) : (0)) - \
        ((!(CPPS_IS_FEATURE_SUPPORTED(cpps_env.prfl_cfg, CPPS_VECTOR_MASK)) && \
                ((idx) > CPS_IDX_VECTOR_CHAR))? (3) : (0)))
/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// Cycling Power Service - Attribute List
enum cpps_cps_att_list
{
    /// Cycling Power Service
    CPS_IDX_SVC,
    /// CP Measurement
    CPS_IDX_CP_MEAS_CHAR,
    CPS_IDX_CP_MEAS_VAL,
    CPS_IDX_CP_MEAS_NTF_CFG,
    CPS_IDX_CP_MEAS_BCST_CFG,
    /// CP Feature
    CPS_IDX_CP_FEAT_CHAR,
    CPS_IDX_CP_FEAT_VAL,
    /// Sensor Location
    CPS_IDX_SENSOR_LOC_CHAR,
    CPS_IDX_SENSOR_LOC_VAL,
    /// CP Vector
    CPS_IDX_VECTOR_CHAR,
    CPS_IDX_VECTOR_VAL,
    CPS_IDX_VECTOR_NTF_CFG,
    /// CP Control Point
    CPS_IDX_CTNL_PT_CHAR,
    CPS_IDX_CTNL_PT_VAL,
    CPS_IDX_CTNL_PT_IND_CFG,

    /// Number of attributes
    CPS_IDX_NB,
};


/// Profile Configuration Additional Flags ()
enum cpps_prf_cfg_flag
{
    /// CP Measurement - Client Char. Cfg
    CPP_PRF_CFG_FLAG_CP_MEAS_NTF  = 0x01,

    /// CP Measurement - Server Char. Cfg
    CPP_PRF_CFG_FLAG_SP_MEAS_NTF  = 0x02,

    /// CP Vector - Client Characteristic configuration
    CPP_PRF_CFG_FLAG_VECTOR_NTF   = 0x04,

    /// Control Point - Client Characteristic configuration
    CPP_PRF_CFG_FLAG_CTNL_PT_IND  = 0x08,
};


/// Pointer to the connection clean-up function
#define CPPS_CLEANUP_FNCT        (NULL)

/*
 * STRUCTURES
 ****************************************************************************************
 */

/// Cycling Power Profile Sensor environment variable
struct cpps_env_tag
{
    /// Connection Information
    struct prf_con_info con_info;

    /// Cycling Speed and Cadence Service Start Handle
    uint16_t shdl;

    /// Characteristic Handle offsets
    uint8_t hdl_offset[CPP_CPS_CHAR_MAX];

    /// Operation
    uint8_t operation;

    /// bool notification pending
    bool ntf_pending;

    /// Buffer for subsequent notification
    uint8_t ntf[ATT_DEFAULT_MTU - 3];

    /// Length for subsequent notification
    uint8_t ntf_len;

    /// Profile Configuration Flags
    uint16_t prfl_cfg;

    /// Feature Configuration Flags
    uint32_t feat_cfg;

    /// Profile Notify/Indication Flags
    uint8_t prfl_ntf_ind_cfg;

    /// Instantaneous Power
    uint32_t inst_power;

    /// Cumulative Value
    uint32_t cumul_wheel_rev;

    /// Sensor Location
    uint8_t sensor_loc;

    /// Crank Length
    uint16_t crank_length;

    /// Chain Length
    uint16_t chain_length;

    /// Chain Weight
    uint16_t chain_weight;

    /// Span Length
    uint16_t span_length;
};

/*
 * GLOBAL VARIABLE DECLARATIONS
 ****************************************************************************************
 */

/// Cycling Speed and Cadence Sensor Environment Variable
extern struct cpps_env_tag cpps_env;

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
void cpps_init(void);

/**
 ****************************************************************************************
 * @brief Send a response indication without parameters to the peer device.
 ****************************************************************************************
 */
void cpps_send_rsp_ind(uint16_t handle, uint8_t req_op_code, uint8_t status);

/**
 ****************************************************************************************
 * @brief Send a CPPS_CMP_EVT message to the application.
 ****************************************************************************************
 */
void cpps_send_cmp_evt(uint8_t src_id, uint8_t dest_id, uint16_t conhdl,
                        uint8_t operation, uint8_t status);

/**
 ****************************************************************************************
 * @brief Disable actions grouped in getting back to IDLE and sending configuration to
 * requester task
 ****************************************************************************************
 */
void cpps_disable(uint16_t conhdl);

#endif //(BLE_CP_SENSOR)

/// @} CPPS

#endif //(_CPPS_H_)
