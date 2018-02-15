/**
 ****************************************************************************************
 *
 * @file wssc.h
 *
 * @brief Header file - Weight Scale Service Collector.
 *
 * Copyright (C) 2014. Dialog Semiconductor Ltd, unpublished work. This computer 
 * program includes Confidential, Proprietary Information and is a Trade Secret of 
 * Dialog Semiconductor Ltd.  All use, disclosure, and/or reproduction is prohibited 
 * unless authorized in writing. All Rights Reserved.
 *
 * <bluetooth.support@diasemi.com> and contributors.
 *
 ****************************************************************************************
 */

#ifndef WSSC_H_
#define WSSC_H_
/**
 ************************************************************************************
 * @addtogroup WSSS Weight Scale Service Collector
 * @ingroup WSS
 * @brief Weight Scale Service Collector
 * @{
 ************************************************************************************
 */

/// Weight Scale Service Collector Role
#define BLE_WSS_COLLECTOR              1
#if !defined (BLE_CLIENT_PRF)
    #define BLE_CLIENT_PRF             1
#endif
/*
 * INCLUDE FILES
 ************************************************************************************
 */

#if (BLE_WSS_COLLECTOR)
#include "ke_task.h"


/*
 * DEFINES
 ****************************************************************************************
 */

#define WSSC_PACKED_MEAS_MIN_LEN        (3)

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// Weight Scale Service Characteristics
enum
{
    WSSC_CHAR_WSS_WS_FEAT,
    WSSC_CHAR_WSS_WEIGHT_MEAS,

    WSSC_CHAR_MAX
};

/// Weight Scale Service Characteristic Descriptor
enum
{
    // Weight Meas Client Config
    WSSC_DESC_WSS_WM_CLI_CFG,

    WSSC_DESC_MAX,

    WSSC_DESC_MASK = 0x10
};

/// Internal codes for reading WSS characteristics 
enum
{
    // Read Wight Scale Feature
    WSSC_RD_WSS_WS_FEAT         = WSSC_CHAR_WSS_WS_FEAT,

    // Read Weight Measurment Client Cfg Descriptor
    WSSC_RD_WSS_WM_CLI_CFG      = (WSSC_DESC_MASK | WSSC_DESC_WSS_WM_CLI_CFG)
};

/// Pointer to the connection clean-up function
#define WSSC_CLEANUP_FNCT        (NULL)


/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

/**
 * Structure containing the characteristics handles, value handles and descriptor for
 * the Weight Scale Service
 */
struct wssc_wss_content
{
    /// Service info
    struct prf_svc svc;
  
    /// Characteristic info:
    struct prf_char_inf chars[WSSC_CHAR_MAX];
  
    /// Descriptor handles:
    struct prf_char_desc_inf desc[WSSC_DESC_MAX];
};


/// Weight Scale Service Client environment variable
struct wssc_env_tag
{
    /// Profile Connection Info
    struct prf_con_info con_info;

    /// Last requested UUID(to keep track of the two services and char)
    uint16_t last_uuid_req;
    /// Last char. code requested to read.
    uint8_t last_char_code;

    /// Counter used to check service uniqueness
    uint8_t nb_svc;

    struct wssc_wss_content wss;
};


/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

extern struct wssc_env_tag **wssc_envs;

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Initialization of the WSSC module.
 * This function performs all the initializations of the WSSC module.
 ****************************************************************************************
 */
void wssc_init(void);

/**
 ****************************************************************************************
 * @brief Send Weight Scale ATT DB discovery results to WSSC host.
 ****************************************************************************************
 */
void wssc_enable_cfm_send(struct wssc_env_tag *wssc_env, struct prf_con_info *con_info, uint8_t status);

/**
 ****************************************************************************************
 * @brief Unpack the received weight measurement value
 ****************************************************************************************
 */
void wssc_unpack_meas_value(struct wssc_env_tag *wssc_env, uint8_t *packet_data, uint8_t lenght);

/**
 ****************************************************************************************
 * @brief Send error indication from service to Host, with proprietary status codes.
 * @param status Status code of error.
 ****************************************************************************************
 */
void wssc_error_ind_send(struct wssc_env_tag *wssc_env, uint8_t status);


#endif //BLE_WSS_COLLECTOR
/// @} WSSS
#endif // WSSC_H_
