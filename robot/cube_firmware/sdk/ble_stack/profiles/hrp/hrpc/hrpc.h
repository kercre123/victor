/**
 ****************************************************************************************
 *
 * @file hrpc.h
 *
 * @brief Header file - Heart Rate Profile - Collector Role.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 * $Rev$
 *
 ****************************************************************************************
 */

#ifndef _HRPC_H_
#define _HRPC_H_

/**
 ****************************************************************************************
 * @addtogroup HRPC Heart Rate Profile Collector
 * @ingroup HRP
 * @brief Heart Rate Profile Collector
 *
 * The HRPC is responsible for providing Heart Rate Profile Collector functionalities
 * to upper layer module or application. The device using this profile takes the role
 * of Heart Rate Profile Collector.
 *
 * Heart Rate Profile Collector. (HRPC): A HRPC (e.g. PC, phone, etc)
 * is the term used by this profile to describe a device that can interpret Heart Rate
 * measurement in a way suitable to the user application.
 *
 * @{
 ****************************************************************************************
 */


/// HRPC Heart Rate Profile Collector role
#define BLE_HR_COLLECTOR            1
#if !defined (BLE_CLIENT_PRF)
    #define BLE_CLIENT_PRF          1
#endif 

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#if (BLE_HR_COLLECTOR)
#include "hrp_common.h"
#include "ke_task.h"
#include "prf_types.h"

/*
 * DEFINES
 ****************************************************************************************
 */

/// Characteristics
enum
{
    /// Heart Rate Measurement
    HRPC_CHAR_HR_MEAS,
    /// Body Sensor Location
    HRPC_CHAR_BODY_SENSOR_LOCATION,
    /// Heart Rate Control Point
    HRPC_CHAR_HR_CNTL_POINT,

    HRPC_CHAR_MAX,
};


/// Characteristic descriptors
enum
{
    /// Heart Rate Measurement client config
    HRPC_DESC_HR_MEAS_CLI_CFG,
    HRPC_DESC_MAX,
    HRPC_DESC_MASK = 0x10,
};

/// Internal codes for reading a HRS or DIS characteristic with one single request
enum
{
    ///Read HRS Heart Rate Measurement
    HRPC_RD_HRS_HR_MEAS          = HRPC_CHAR_HR_MEAS,
    ///Body Sensor Location
    HRPC_RD_HRS_BODY_SENSOR_LOC  = HRPC_CHAR_BODY_SENSOR_LOCATION,
    ///Heart Rate Control Point
    HRPC_RD_HRS_CNTL_POINT       = HRPC_CHAR_HR_CNTL_POINT,

    ///Read HRS Heart Rate Measurement Client Cfg. Desc
    HRPC_RD_HRS_HR_MEAS_CFG      = (HRPC_DESC_MASK | HRPC_DESC_HR_MEAS_CLI_CFG),
};

/// Pointer to the connection clean-up function
#define HRPC_CLEANUP_FNCT        (NULL)


/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

///Structure containing the characteristics handles, value handles and descriptors
struct hrs_content
{
    /// service info
    struct prf_svc svc;

    /// characteristic info:
    ///  - Heart Rate Measurement
    ///  - Body Sensor Location
    ///  - Heart Rate Control Point
    struct prf_char_inf chars[HRPC_CHAR_MAX];

    /// Descriptor handles:
    ///  - Heart Rate Measurement client cfg
    struct prf_char_desc_inf descs[HRPC_DESC_MAX];
};

/// Heart Rate Profile Collector environment variable
struct hrpc_env_tag
{
    /// Profile Connection Info
    struct prf_con_info con_info;
    ///Last requested UUID(to keep track of the two services and char)
    uint16_t last_uuid_req;

    ///HTS characteristics
    struct hrs_content hrs;

    /// Last char. code requested to read.
    uint8_t last_char_code;

    /// counter used to check service uniqueness
    uint8_t nb_svc;
};

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */
extern struct hrpc_env_tag **hrpc_envs;

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Initialization of the HRPC module.
 * This function performs all the initializations of the HRPC module.
 ****************************************************************************************
 */
void hrpc_init(void);

/**
 ****************************************************************************************
 * @brief Send Heart Rate ATT DB discovery results to HRPC host.
 ****************************************************************************************
 */
void hrpc_enable_cfm_send(struct hrpc_env_tag *hrpc_env, struct prf_con_info *con_info, uint8_t status);

/**
 ****************************************************************************************
 * @brief Unpack Heart Rate measurement data into a comprehensive structure.
 *
 * @param[out] pmeas_val  Pointer to Heart Rate measurement structure destination
 * @param[in]  packed_hr  Pointer of the packed data of Heart Rate Measurement
 *                        information
 * @param[in]  size       Packet data size
 ****************************************************************************************
 */
void hrpc_unpack_meas_value(struct hrs_hr_meas* pmeas_val, uint8_t* packed_hr, uint8_t size);

/**
 ****************************************************************************************
 * @brief Send error indication from profile to Host, with proprietary status codes.
 * @param status Status code of error.
 ****************************************************************************************
 */
void hrpc_error_ind_send(struct hrpc_env_tag *hrpc_env, uint8_t status);

#endif /* (BLE_HR_COLLECTOR) */

/// @} HRPC

#endif /* _HRPC_H_ */
