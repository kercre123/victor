/**
 ****************************************************************************************
 *
 * @file cscpc.h
 *
 * @brief Header file - Cycling Speed and Cadence Status Profile - Collector Role.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 * $ Rev $
 *
 ****************************************************************************************
 */

#ifndef _CSCPC_H_
#define _CSCPC_H_

/**
 ****************************************************************************************
 * @addtogroup CSCPC Cycling Speed and Cadence Profile Collector
 * @ingroup CSCP
 * @brief Cycling Speed and Cadence Profile Collector
 * @{
 ****************************************************************************************
 */


/// CSCPC Cycling Speed and Cadence Profile Collector role
#define BLE_CSC_COLLECTOR           1
#if !defined (BLE_CLIENT_PRF)
    #define BLE_CLIENT_PRF          1
#endif

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "cscp_common.h"

#if (BLE_CSC_COLLECTOR)

#include "ke_task.h"
#include "prf_types.h"

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// Cycling Speed and Cadence Service Characteristic Descriptors
enum cscpc_cscs_descs
{
    /// CSC Measurement Char. - Client Characteristic Configuration
    CSCPC_DESC_CSC_MEAS_CL_CFG,
    /// SC Control Point Char. - Client Characteristic Configuration
    CSCPC_DESC_SC_CTNL_PT_CL_CFG,

    CSCPC_DESC_MAX,

    CSCPC_DESC_MASK = 0x10,
};

/// Internal codes for reading/writing a CSCS characteristic with one single request
enum cscpc_codes
{
    /// Notified CSC Measurement
    CSCPC_NTF_CSC_MEAS          = CSCP_CSCS_CSC_MEAS_CHAR,
    /// Read CSC Feature
    CSCPC_RD_CSC_FEAT           = CSCP_CSCS_CSC_FEAT_CHAR,
    /// Read Sensor Location
    CSCPC_RD_SENSOR_LOC         = CSCP_CSCS_SENSOR_LOC_CHAR,
    /// Indicated SC Control Point
    CSCPC_IND_SC_CTNL_PT        = CSCP_CSCS_SC_CTNL_PT_CHAR,

    /// Read/Write CSC Measurement Client Char. Configuration Descriptor
    CSCPC_RD_WR_CSC_MEAS_CFG    = (CSCPC_DESC_CSC_MEAS_CL_CFG   | CSCPC_DESC_MASK),
    /// Read SC Control Point Client Char. Configuration Descriptor
    CSCPC_RD_WR_SC_CTNL_PT_CFG  = (CSCPC_DESC_SC_CTNL_PT_CL_CFG | CSCPC_DESC_MASK),
};

/// Pointer to the connection clean-up function
#define CSCPC_CLEANUP_FNCT        (cscpc_cleanup)
/*
 * STRUCTURES
 ****************************************************************************************
 */

/**
 * Structure containing the characteristics handles, value handles and descriptors for
 * the Cycling Speed and Cadence Service
 */
struct cscpc_cscs_content
{
    /// Service info
    struct prf_svc svc;

    /// Characteristic info:
    ///  - CSC Measurement
    ///  - CSC Feature
    ///  - Sensor Location
    ///  - SC Control Point
    struct prf_char_inf chars[CSCP_CSCS_CHAR_MAX];

    /// Descriptor handles:
    ///  - CSC Measurement Client Cfg
    ///  - SC Control Point Client Cfg
    struct prf_char_desc_inf descs[CSCPC_DESC_MAX];
};


/// Cycling Speed and Cadence Profile Collector environment variable
struct cscpc_env_tag
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

    /// Cycling Speed and Cadence Service Characteristics
    struct cscpc_cscs_content cscs;
};

/// Command Message Basic Structure
struct cscpc_cmd
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

/// Pool of Cycling Speed and Cadence Profile Collector task environments.
extern struct cscpc_env_tag **cscpc_envs;

/*
 * GLOBAL FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Initialization of the CSCPC module.
 * This function performs all the initializations of the CSCPC module.
 ****************************************************************************************
 */
void cscpc_init(void);

/**
 ****************************************************************************************
 * @brief Clean-up connection dedicated environment parameters
 * This function performs cleanup of ongoing operations
 * @param[in] idx_env Current connection environment variable
 ****************************************************************************************
 */
void cscpc_cleanup(prf_env_struct *idx_env);

/**
 ****************************************************************************************
 * @brief Send a CSCPC_CMP_EVT message when no connection exists (no environment)
 ****************************************************************************************
 */
void cscpc_send_no_conn_cmp_evt(uint8_t src_id, uint8_t dest_id, uint16_t conhdl, uint8_t operation);

/**
 ****************************************************************************************
 * @brief Send a CSCPC_CMP_EVT message to the task which enabled the profile
 ****************************************************************************************
 */
void cscpc_send_cmp_evt(struct cscpc_env_tag *cscpc_env, uint8_t operation, uint8_t status);

#endif //(BLE_CSC_COLLECTOR)

/// @} CSCPC

#endif //(_CSCPC_H_)
