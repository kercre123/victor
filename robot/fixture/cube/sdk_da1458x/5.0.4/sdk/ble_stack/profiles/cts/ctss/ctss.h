/**
 ****************************************************************************************
 *
 * @file ctss.h
 *
 * @brief Header file - Current Time Service Server.
 *
 * Copyright (C) 2015. Dialog Semiconductor Ltd, unpublished work. This computer
 * program includes Confidential, Proprietary Information and is a Trade Secret of
 * Dialog Semiconductor Ltd.  All use, disclosure, and/or reproduction is prohibited
 * unless authorized in writing. All Rights Reserved.
 *
 * <bluetooth.support@diasemi.com> and contributors.
 *
 ****************************************************************************************
 */

#ifndef _CTSS_H_
#define _CTSS_H_


/**
 ****************************************************************************************
 * @addtogroup CTSS Current Time Service Server
 * @ingroup CTS
 * @brief Current Time Service Server
 * @{
 ****************************************************************************************
 */

/// Current Time Server Role
#define BLE_CTS_SERVER              1
#if !defined (BLE_SERVER_PRF)
    #define BLE_SERVER_PRF          1
#endif

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "prf_types.h"
#include "cts_common.h"

#if (BLE_CTS_SERVER)

/*
 * DEFINES
 ****************************************************************************************
 */

#define CTSS_CURRENT_TIME_VAL_LEN        (10)

/*
 * MACROS
 ****************************************************************************************
 */

#define CTSS_IS_SUPPORTED(mask) (((ctss_env.features & mask) == mask))

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// Database Configuration - Bit Field Flags
enum
{
    // Optional Characteristics
    CTSS_LOC_TIME_INFO_SUP   = 0x01,
    CTSS_REF_TIME_INFO_SUP   = 0x02,

    // Current Time Characteristic's notification config
    CTSS_CURRENT_TIME_CFG    = 0x10,
};

/// Current Time Service Attribute indices
enum
{
    CTSS_IDX_SVC,

    CTSS_IDX_CURRENT_TIME_CHAR,
    CTSS_IDX_CURRENT_TIME_VAL,
    CTSS_IDX_CURRENT_TIME_CFG,

    CTSS_IDX_LOCAL_TIME_INFO_CHAR,
    CTSS_IDX_LOCAL_TIME_INFO_VAL,

    CTSS_IDX_REF_TIME_INFO_CHAR,
    CTSS_IDX_REF_TIME_INFO_VAL,

    CTSS_IDX_NB,
};

/// Current Time Service Characteristics
enum
{
    CTSS_CURRENT_TIME_CHAR,
    CTSS_LOCAL_TIME_INFO_CHAR,
    CTSS_REF_TIME_INFO_CHAR,

    CTSS_CHAR_MAX,
};

/*
 * STRUCTURES
 ****************************************************************************************
 */

struct ctss_env_tag
{
    /// Connection Info
    struct prf_con_info con_info;

    /// CTS Start Handle
    uint16_t shdl;

    /// CTS Attribute Table
    uint8_t att_tbl[CTSS_CHAR_MAX];

    /// Database configuration
    uint8_t features;
};

extern struct ctss_env_tag ctss_env;
extern const struct attm_desc att_db[CTSS_IDX_NB];

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Initialization of the CTSS module.
 * This function performs all the initializations of the CTSS module.
 ****************************************************************************************
 */
void ctss_init(void);

/**
 ****************************************************************************************
 * @brief Send a CTS_ENABLE_CFM message to the application
 * @param[in] con_info Connection information
 * @param[in] status Status code of error
 ****************************************************************************************
 */
void ctss_enable_cfm_send(struct prf_con_info *con_info, uint8_t status);

/**
 ****************************************************************************************
 * @brief Send a CTS_ERROR_IND message to the application
 * @param[in] con_info Connection information
 * @param[in] status Status code of error
 * @param[in] msg_id Failing message ID
 ****************************************************************************************
 */
void ctss_error_ind_send(struct prf_con_info *con_info, uint8_t status, ke_msg_id_t msg_id);

/**
 ****************************************************************************************
 * @brief Pack Current Time value
 * @param[out] p_pckd_time Packed Current Time value
 * @param[in] p_curr_time_val Current Time value to be packed
 ****************************************************************************************
 */
void ctss_pack_curr_time_value(uint8_t *p_pckd_time,
                                const struct cts_curr_time *p_curr_time_val);

/**
 ****************************************************************************************
 * @brief Disable actions grouped in getting back to IDLE and sending configuration to requester task
 * @param[in] conhdl Handle of connection for which this service is disabled
 ****************************************************************************************
 */
void ctss_disable(uint16_t conhdl);

#endif /* #if (BLE_CTS_SERVER) */

/// @} CTSS

#endif /* _CTSS_H_ */
