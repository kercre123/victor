/**
 ****************************************************************************************
 *
 * @file tipc.h
 *
 * @brief Header file - Time Profile - Client Role.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 * $Rev$
 *
 ****************************************************************************************
 */

#ifndef _TIPC_H_
#define _TIPC_H_

/**
 ****************************************************************************************
 * @addtogroup TIPC Time Profile Client
 * @ingroup TIP
 * @brief Time Profile Client
 *
 *
 * @{
 ****************************************************************************************
 */

/// TIPC Time Profile Client role
#define BLE_TIP_CLIENT              1
#if !defined (BLE_CLIENT_PRF)
    #define BLE_CLIENT_PRF          1
#endif

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#if (BLE_TIP_CLIENT)
#include "tip_common.h"
#include "ke_task.h"
#include "prf_types.h"

/*
 * DEFINES
 ****************************************************************************************
 */

/// Current Time Service Characteristics
enum
{
    /// Current Time
    TIPC_CHAR_CTS_CURR_TIME,
    /// Local Time Info
    TIPC_CHAR_CTS_LOCAL_TIME_INFO,
    /// Reference Time Info
    TIPC_CHAR_CTS_REF_TIME_INFO,

    TIPC_CHAR_CTS_MAX,
};

/// Next DST Change Service Characteristics
enum
{
    /// Time With DST
    TIPC_CHAR_NDCS_TIME_WITH_DST,

    TIPC_CHAR_NDCS_MAX,
    TIPC_CHAR_NDCS_MASK = 0x10,
};

/// Reference Time Update Service Characteristics
enum
{
    /// Time Update Control Point
    TIPC_CHAR_RTUS_TIME_UPD_CTNL_PT,
    /// Time Update State
    TIPC_CHAR_RTUS_TIME_UPD_STATE,

    TIPC_CHAR_RTUS_MAX,
    TIPC_CHAR_RTUS_MASK = 0x20,
};

/// Current Time Service Characteristic Descriptors
enum
{
    /// Current Time client config
    TIPC_DESC_CTS_CURR_TIME_CLI_CFG,

    TIPC_DESC_CTS_MAX,
    TIPC_DESC_CTS_MASK = 0x30,
};

/// Internal codes for reading a CTS or NDCS or RTUS characteristic with one single request
enum
{
    ///Read CTS Current Time
    TIPC_RD_CTS_CURR_TIME           = TIPC_CHAR_CTS_CURR_TIME,
    ///Read CTS Local Time Info
    TIPC_RD_CTS_LOCAL_TIME_INFO     = TIPC_CHAR_CTS_LOCAL_TIME_INFO,
    ///Read CTS Reference Time Info
    TIPC_RD_CTS_REF_TIME_INFO       = TIPC_CHAR_CTS_REF_TIME_INFO,

    ///Read CTS Current Time Client Cfg. Desc
    TIPC_RD_CTS_CURR_TIME_CLI_CFG   = (TIPC_DESC_CTS_MASK | TIPC_DESC_CTS_CURR_TIME_CLI_CFG),

    ///Read NDCS Time With DST
    TIPC_RD_NDCS_TIME_WITH_DST      = (TIPC_CHAR_NDCS_MASK | TIPC_CHAR_NDCS_TIME_WITH_DST),

    ///Read RTUS Time Update State
    TIPC_RD_RTUS_TIME_UPD_STATE     = (TIPC_CHAR_RTUS_MASK | TIPC_CHAR_RTUS_TIME_UPD_STATE),
};

/// Pointer to the connection clean-up function
#define TIPC_CLEANUP_FNCT        (NULL)


/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

/**
 * Structure containing the characteristics handles, value handles and descriptors for
 * the Current Time Service
 */
struct tipc_cts_content
{
    /// service info
    struct prf_svc svc;

    /// Characteristic info:
    ///  - Current Time
    ///  - Local Time Info
    ///  - Reference Time Info
    struct prf_char_inf chars[TIPC_CHAR_CTS_MAX];

    /// Descriptor handles:
    ///  - Current Time client cfg
    struct prf_char_desc_inf descs[TIPC_DESC_CTS_MAX];
};

/**
 * Structure containing the characteristics handles, value handles and descriptors for
 * the Next DST Change Service
 */
struct tipc_ndcs_content
{
    /// service info
    struct prf_svc svc;

    /// characteristic info:
    ///  - Time With DST
    struct prf_char_inf chars[TIPC_CHAR_NDCS_MAX];
};

/**
 * Structure containing the characteristics handles, value handles and descriptors for
 * the Reference Time Update Service
 */
struct tipc_rtus_content
{
    /// service info
    struct prf_svc svc;

    /// characteristic info:
    ///  - Time Update Control Point
    ///  - Time Update State
    struct prf_char_inf chars[TIPC_CHAR_RTUS_MAX];
};

/// Time Profile Client environment variable
struct tipc_env_tag
{
    /// Profile Connection Info
    struct prf_con_info con_info;

    ///Last requested UUID(to keep track of the two services and char)
    uint16_t last_uuid_req;
    ///Last service for which something was discovered
    uint16_t last_svc_req;
    /// Last char. code requested to read.
    uint8_t last_char_code;

    /// counter used to check service uniqueness
    uint8_t nb_svc;

    ///Current Time Service Characteristics
    struct tipc_cts_content cts;
    ///Next DST Change Characteristics
    struct tipc_ndcs_content ndcs;
    ///Reference Time Update Characteristics
    struct tipc_rtus_content rtus;
};

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

extern struct tipc_env_tag **tipc_envs;

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Initialization of the TIPC module.
 * This function performs all the initializations of the TIPC module.
 ****************************************************************************************
 */
void tipc_init(void);

/**
 ****************************************************************************************
 * @brief Send Time ATT DB discovery results to TIPC host.
 ****************************************************************************************
 */
void tipc_enable_cfm_send(struct tipc_env_tag *tipc_env, struct prf_con_info *con_info, uint8_t status);

/**
 ****************************************************************************************
 * @brief Send error indication from profile to Host, with proprietary status codes.
 * @param status Status code of error.
 ****************************************************************************************
 */
void tipc_error_ind_send(struct tipc_env_tag *tipc_env, uint8_t status);

void tipc_unpack_curr_time_value(struct tip_curr_time* p_curr_time_val, uint8_t* packed_ct);

void tipc_unpack_time_dst_value(struct tip_time_with_dst* p_time_dst_val, uint8_t* packed_tdst);

/**
 ****************************************************************************************
 * @brief Disable Time Profile Client Role Task
 ****************************************************************************************
 */
void tipc_disable(void);

#endif /* (BLE_TIP_CLIENT) */

/// @} TIPC

#endif /* _TIPC_H_ */
