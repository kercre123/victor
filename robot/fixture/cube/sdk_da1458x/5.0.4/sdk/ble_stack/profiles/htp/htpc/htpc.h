/**
 ****************************************************************************************
 *
 * @file htpc.h
 *
 * @brief Header file - Health Thermometer Profile - Collector Role.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 *
 ****************************************************************************************
 */

#ifndef HTPC_H_
#define HTPC_H_

/**
 ****************************************************************************************
 * @addtogroup HTP Health Thermometer Profile
 * @ingroup PROFILE
 * @brief Health Thermometer Profile
 *
 * The HTP module is the responsible block for implementing the Health Thermometer Profile
 * functionalities in the BLE Host.
 *
 * The Health Thermometer Profile defines the functionality required in a device that allows
 * the user (Collector device) to configure and recover temperature measurements from a Thermometer device.
 *****************************************************************************************
 */

/**
 ****************************************************************************************
 * @addtogroup HTPC Health Thermometer Profile Collector
 * @ingroup HTP
 * @brief Health Thermometer Profile Collector
 *
 * The HTPC is responsible for providing Health Thermometer Profile Collector functionalities to
 * upper layer module or application. The device using this profile takes the role
 * of Health Thermometer Profile Collector.
 *
 * Health Thermometer Profile Collector (HTPC): A HTPC (e.g. PC, phone, etc)
 * is the term used by this profile to describe a device that can set temperature
 * indication/notification intervals in a HTP Thermometer, interpreting them in a way
 * suitable to the user application.
 *
 * @{
 ****************************************************************************************
 */


/// HRPC Heart Rate Profile Collector role
#define BLE_HT_COLLECTOR            1
#if !defined (BLE_CLIENT_PRF)
    #define BLE_CLIENT_PRF          1
#endif 

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#if (BLE_HT_COLLECTOR || BLE_HT_THERMOM)
#include "ke_task.h"
#include "prf_types.h"

/*
 * DEFINES
 ****************************************************************************************
 */

#define HTPC_PACKED_TEMP_MIN_LEN        (5)
#define HTPC_PACKED_TEMP_MAX_LEN        (13)

/*
 * MACROS
 ****************************************************************************************
 */


/*
 * ENUMERATIONS
 ****************************************************************************************
 */

enum
{
    HTPC_TEMP_MEAS_INTM,
    HTPC_TEMP_MEAS_STABLE,
};

/// Health Thermometer Service Characteristics - Char. Code
enum
{
    /// Temperature Measurement
    HTPC_CHAR_HTS_TEMP_MEAS,
    /// Temperature Type
    HTPC_CHAR_HTS_TEMP_TYPE,
    /// Intermediate Temperature
    HTPC_CHAR_HTS_INTM_TEMP,
    /// Measurement Interval
    HTPC_CHAR_HTS_MEAS_INTV,

    HTPC_CHAR_HTS_MAX,
};

/// Health Thermometer Service Characteristic Descriptors
enum
{
    /// Temp. Meas. Client Config
    HTPC_DESC_HTS_TEMP_MEAS_CLI_CFG,
    /// Intm. Meas. Client Config
    HTPC_DESC_HTS_INTM_MEAS_CLI_CFG,
    /// Meas. Intv. Client Config
    HTPC_DESC_HTS_MEAS_INTV_CLI_CFG,
    /// Meas. Intv. Valid Range,
    HTPC_DESC_HTS_MEAS_INTV_VAL_RGE,

    HTPC_DESC_HTS_MAX,

    HTPC_DESC_HTS_MASK = 0x10,
};

/// Internal codes for reading a HTS characteristic with one single request
enum
{
    ///Read HTS Temp. Type
    HTPC_RD_HTS_TEMP_TYPE           = HTPC_CHAR_HTS_TEMP_TYPE,
    ///Read HTS Measurement Interval
    HTPC_RD_HTS_MEAS_INTV           = HTPC_CHAR_HTS_MEAS_INTV,

    ///Read HTS Temperature Measurement Client Cfg. Desc
    HTPC_RD_HTS_TEMP_MEAS_CLI_CFG   = (HTPC_DESC_HTS_MASK | HTPC_DESC_HTS_TEMP_MEAS_CLI_CFG),
    ///Read HTS Intermediate Temperature Client Cfg. Desc
    HTPC_RD_HTS_INTM_TEMP_CLI_CFG   = (HTPC_DESC_HTS_MASK | HTPC_DESC_HTS_INTM_MEAS_CLI_CFG),
    ///Read HTS Measurement Interval Client Cfg. Desc
    HTPC_RD_HTS_MEAS_INTV_CLI_CFG   = (HTPC_DESC_HTS_MASK | HTPC_DESC_HTS_MEAS_INTV_CLI_CFG),
    ///Read HTS Measurement Interval Client Cfg. Desc
    HTPC_RD_HTS_MEAS_INTV_VAL_RGE   = (HTPC_DESC_HTS_MASK | HTPC_DESC_HTS_MEAS_INTV_VAL_RGE),
};

/// Pointer to the connection clean-up function
#define HTPC_CLEANUP_FNCT        (NULL)


/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

/**
 * Structure containing the characteristics handles, value handles and descriptors for
 * the Health Thermometer Service
 */
struct htpc_hts_content
{
    /// service info
    struct prf_svc svc;

    /// Characteristic info:
    struct prf_char_inf chars[HTPC_CHAR_HTS_MAX];

    /// Descriptor handles:
    struct prf_char_desc_inf descs[HTPC_DESC_HTS_MAX];
};

/// Health Thermometer Profile Client environment variable
struct htpc_env_tag
{
    /// Profile Connection Info
    struct prf_con_info con_info;

    ///Last requested UUID(to keep track of the two services and char)
    uint16_t last_uuid_req;
    /// Last char. code requested to read.
    uint8_t last_char_code;

    /// counter used to check service uniqueness
    uint8_t nb_svc;

    ///Current Time Service Characteristics
    struct htpc_hts_content hts;
};

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

extern struct htpc_env_tag **htpc_envs;

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Initialization of the HTPC module.
 * This function performs all the initializations of the HTPC module.
 ****************************************************************************************
 */
void htpc_init(void);

/**
 ****************************************************************************************
 * @brief Send error indication from profile to Host, with proprietary status codes.
 * @param status Status code of error.
 ****************************************************************************************
 */
void htpc_error_ind_send(struct htpc_env_tag *htpc_env, uint8_t status);

/**
 ****************************************************************************************
 * @brief Send Thermometer ATT DB discovery results to HTPC host.
 ****************************************************************************************
 */
void htpc_enable_cfm_send(struct htpc_env_tag *htpc_env, struct prf_con_info *con_info, uint8_t status);

/**
 ****************************************************************************************
 * @brief Unpack the received temperature measurement value
 ****************************************************************************************
 */
void htpc_unpack_temp(struct htpc_env_tag *htpc_env, uint8_t *packed_temp,
                      uint8_t length, uint8_t flag_stable_meas);

/**
 ****************************************************************************************
 * @brief Look for the next characteristic for which descriptors have to be discovered.
 * @return Char code of this characteristic
 ****************************************************************************************
 */
uint8_t htpc_get_next_desc_char_code(struct htpc_env_tag *htpc_env,
                                     const struct prf_char_desc_def *desc_def);

#endif //BLE_HT_COLLECTOR
/// @} HTPC
#endif // HTPC_H_
