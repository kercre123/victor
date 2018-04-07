/**
 ****************************************************************************************
 *
 * @file blpc.h
 *
 * @brief Header file - Blood Pressure Profile - Collector Role.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 * $Rev$
 *
 ****************************************************************************************
 */


#ifndef _BLPC_H_
#define _BLPC_H_


/**
 ****************************************************************************************
 * @addtogroup BLPC Blood Pressure Profile Collector
 * @ingroup BLP
 * @brief Blood Pressure Profile Collector
 *
 * The BLPC is responsible for providing Blood Pressure Profile Collector functionalities
 * to upper layer module or application. The device using this profile takes the role
 * of Blood Pressure Profile Collector.
 *
 * Blood Pressure Profile Collector. (BLPC): A BLPC (e.g. PC, phone, etc)
 * is the term used by this profile to describe a device that can interpret blood pressure
 * measurement in a way suitable to the user application.
 *
 * @{
 ****************************************************************************************
 */


/// Blood Pressure Profile Collector Role
#define BLE_BP_COLLECTOR            1
#if !defined (BLE_CLIENT_PRF)
    #define BLE_CLIENT_PRF          1
#endif 
/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#if (BLE_BP_COLLECTOR)
#include "blp_common.h"
#include "ke_task.h"
#include "prf_types.h"
#include "blps.h"

/*
 * DEFINES
 ****************************************************************************************
 */

/// Characteristics
enum
{
    /// Blood Pressure Measurement
    BLPC_CHAR_BP_MEAS,
    /// Intermediate Cuff pressure
    BLPC_CHAR_CP_MEAS,
    /// Blood Pressure Feature
    BLPC_CHAR_BP_FEATURE,

    BLPC_CHAR_MAX,
};

/// Characteristic descriptors
enum
{
    /// Blood Pressure Measurement client config
    BLPC_DESC_BP_MEAS_CLI_CFG,
    /// Intermediate Cuff pressure client config
    BLPC_DESC_IC_MEAS_CLI_CFG,
    BLPC_DESC_MAX,
    BLPC_DESC_MASK = 0x10,
};

/// Internal codes for reading a BPS or DIS characteristic with one single request
enum
{
    ///Read BPS Blood pressure Measurement
    BLPC_RD_BPS_BP_MEAS          = BLPC_CHAR_BP_MEAS,
    ///Read BPS Intermdiate Cuff Pressure
    BLPC_RD_BPS_CP_MEAS          = BLPC_CHAR_CP_MEAS,
    ///Read BPS Blood pressure Features
    BLPC_RD_BPS_FEATURE          = BLPC_CHAR_BP_FEATURE,

    ///Read BPS Blood pressure Measurement Client Cfg. Desc
    BLPC_RD_BPS_BP_MEAS_CFG      = (BLPC_DESC_MASK | BLPC_DESC_BP_MEAS_CLI_CFG),
    ///Read BPS Intermdiate Cuff Pressure Client Cfg. Desc
    BLPC_RD_BPS_CP_MEAS_CFG      = (BLPC_DESC_MASK | BLPC_DESC_IC_MEAS_CLI_CFG),
};

/// Pointer to the connection clean-up function
#define BLPC_CLEANUP_FNCT        (NULL)
/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

///Structure containing the characteristics handles, value handles and descriptors
struct bps_content
{
    /// service info
    struct prf_svc svc;

    /// characteristic info:
    ///  - Blood Pressure Measurement
    ///  - Intermediate Cuff pressure
    ///  - Blood Pressure Feature
    struct prf_char_inf chars[BLPC_CHAR_MAX];

    /// Descriptor handles:
    ///  - Blood Pressure Measurement client cfg
    ///  - Intermediate Cuff pressure client cfg
    struct prf_char_desc_inf descs[BLPC_DESC_MAX];
};

/// Blood Pressure Profile Collector environment variable
struct blpc_env_tag
{
    /// Profile Connection Info
    struct prf_con_info con_info;
    ///Last requested UUID(to keep track of the two services and char)
    uint16_t last_uuid_req;
    ///Last Char. for which a read request has been sent
    uint8_t last_char_code;

    /// counter used to check service uniqueness
    uint8_t nb_svc;

    ///HTS characteristics
    struct bps_content bps;
};

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */
extern struct blpc_env_tag **blpc_envs;

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Initialization of the BLPC module.
 * This function performs all the initializations of the BLPC module.
 ****************************************************************************************
 */
void blpc_init(void);

/**
 ****************************************************************************************
 * @brief Send Blood Pressure ATT DB discovery results to BLPC host.
 ****************************************************************************************
 */
void blpc_enable_cfm_send(struct blpc_env_tag *blpc_env, struct prf_con_info *con_info, uint8_t status);

/**
 ****************************************************************************************
 * @brief Unpack Blood pressure measurement data into a comprehensive structure.
 *
 * @param[out] pmeas_val  Pointer to Blood pressure measurement structure destination
 * @param[in] packed_bp   Pointer of the packed data of Blood Pressure Measurement
 *                        information
 ****************************************************************************************
 */
void blpc_unpack_meas_value(struct bps_bp_meas* pmeas_val, uint8_t* packed_bp);

/**
 ****************************************************************************************
 * @brief Send error indication from profile to Host, with proprietary status codes.
 * @param status Status code of error.
 ****************************************************************************************
 */
void blpc_error_ind_send(struct blpc_env_tag *blpc_env, uint8_t status);

#endif /* (BLE_BP_COLLECTOR) */

/// @} BLPC

#endif /* _BLPC_H_ */
