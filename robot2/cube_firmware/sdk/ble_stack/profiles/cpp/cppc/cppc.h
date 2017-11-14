/**
 ****************************************************************************************
 *
 * @file cppc.h
 *
 * @brief Header file - Cycling Power Status Profile - Collector Role.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 * $ Rev $
 *
 ****************************************************************************************
 */

#ifndef _CPPC_H_
#define _CPPC_H_

/**
 ****************************************************************************************
 * @addtogroup CPPC Cycling Power Profile Collector
 * @ingroup CPP
 * @brief Cycling Power Profile Collector
 * @{
 ****************************************************************************************
 */
 
/// CPPC Cycling Power Profile Collector role
#define BLE_CP_COLLECTOR            1
#if !defined (BLE_CLIENT_PRF)
    #define BLE_CLIENT_PRF          1
#endif


/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "cpp_common.h"

#if (BLE_CP_COLLECTOR)

#include "ke_task.h"
#include "prf_types.h"

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// Cycling Power Service Characteristic Descriptors
enum cppc_cps_descs
{
    /// CP Measurement Char. - Client Characteristic Configuration
    CPPC_DESC_CP_MEAS_CL_CFG,
    /// CP Measurement Char. - Server Characteristic Configuration
    CPPC_DESC_CP_MEAS_SV_CFG,

    /// CP Vector Char. - Client Characteristic Configuration
    CPPC_DESC_VECTOR_CL_CFG,
    /// Control Point Char. - Client Characteristic Configuration
    CPPC_DESC_CTNL_PT_CL_CFG,

    CPPC_DESC_MAX,

    CPPC_DESC_MASK = 0x10,
};

/// Internal codes for reading/writing a CPS characteristic with one single request
enum cppc_code
{
    /// Notified CP Measurement
    CPPC_NTF_CP_MEAS          = CPP_CPS_MEAS_CHAR,
    /// Read CP Feature
    CPPC_RD_CP_FEAT           = CPP_CPS_FEAT_CHAR,
    /// Read Sensor Location
    CPPC_RD_SENSOR_LOC        = CPP_CPS_SENSOR_LOC_CHAR,
    /// Notified Vector
    CPPC_NTF_CP_VECTOR           = CPP_CPS_VECTOR_CHAR,
    /// Indicated SC Control Point
    CPPC_IND_CTNL_PT       = CPP_CPS_CTNL_PT_CHAR,

    /// Read/Write CP Measurement Client Char. Configuration Descriptor
    CPPC_RD_WR_CP_MEAS_CL_CFG    = (CPPC_DESC_CP_MEAS_CL_CFG   | CPPC_DESC_MASK),
    /// Read/Write CP Measurement Server Char. Configuration Descriptor
    CPPC_RD_WR_CP_MEAS_SV_CFG    = (CPPC_DESC_CP_MEAS_SV_CFG   | CPPC_DESC_MASK),

    /// Read/Write Vector Client Char. Configuration Descriptor
    CPPC_RD_WR_VECTOR_CFG    = (CPPC_DESC_VECTOR_CL_CFG   | CPPC_DESC_MASK),
    /// Read SC Control Point Client Char. Configuration Descriptor
    CPPC_RD_WR_CTNL_PT_CFG  = (CPPC_DESC_CTNL_PT_CL_CFG | CPPC_DESC_MASK),
};

/// Pointer to the connection clean-up function
#define CPPC_CLEANUP_FNCT        (cppc_cleanup)
/*
 * STRUCTURES
 ****************************************************************************************
 */

/**
 * Structure containing the characteristics handles, value handles and descriptors for
 * the Cycling Power Service
 */
struct cppc_cps_content
{
    /// Service info
    struct prf_svc svc;

    /// Characteristic info:
    ///  - CP Measurement
    ///  - CP Feature
    ///  - Sensor Location
    ///  - Vector
    ///  - SC Control Point
    struct prf_char_inf chars[CPP_CPS_CHAR_MAX];

    /// Descriptor handles:
    ///  - CP Measurement Client Cfg
    ///  - Vector Client Cfg
    ///  - Control Point Client Cfg
    struct prf_char_desc_inf descs[CPPC_DESC_MAX];
};


/// Cycling Power Profile Collector environment variable
struct cppc_env_tag
{
    /// Profile Connection Info
    struct prf_con_info con_info;

    /// Current Operation
    void *operation;

    /// Provide an indication about the last operation
    uint16_t last_req;
    /// Last characteristic code discovered
    uint8_t last_char_code;
    /// Counter used to check service uniqueness
    uint8_t nb_svc;

    /// Cycling Power Service Characteristics
    struct cppc_cps_content cps;
};

/// Command Message Basic Structure
struct cppc_cmd
{
    /// Connection Handle
    uint16_t conhdl;
    /// Operation Code
    uint8_t operation;

    /// MORE DATA
};

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// Pool of Cycling Power Profile Collector task environments.
extern struct cppc_env_tag **cppc_envs;

/*
 * GLOBAL FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Initialization of the CPPC module.
 * This function performs all the initializations of the CPPC module.
 ****************************************************************************************
 */
void cppc_init(void);


/**
 ****************************************************************************************
 * @brief Clean-up connection dedicated environment parameters
 * This function performs cleanup of ongoing operations
 * @param[in] idx_env Current connection environment variable
 ****************************************************************************************
 */
void cppc_cleanup(prf_env_struct *idx_env);

/**
 ****************************************************************************************
 * @brief Send a CPPC_CMP_EVT message when no connection exists (no environment)
 ****************************************************************************************
 */
void cppc_send_no_conn_cmp_evt(uint8_t src_id, uint8_t dest_id, uint16_t conhdl, uint8_t operation);

/**
 ****************************************************************************************
 * @brief Send a CPPC_CMP_EVT message to the task which enabled the profile
 ****************************************************************************************
 */
void cppc_send_cmp_evt(struct cppc_env_tag *cppc_env, uint8_t operation, uint8_t status);


#endif //(BLE_CP_COLLECTOR)

/// @} CPPC

#endif //(_CPPC_H_)
