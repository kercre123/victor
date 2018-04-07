/**
 ****************************************************************************************
 *
 * @file anpc.h
 *
 * @brief Header file - Alert Notification Status Profile - Client Role.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 * $ Rev $
 *
 ****************************************************************************************
 */

#ifndef _ANPC_H_
#define _ANPC_H_

/**
 ****************************************************************************************
 * @addtogroup ANPC Alert Notification Profile Client
 * @ingroup ANP
 * @brief Phone Alert Notification Profile Client
 * @{
 ****************************************************************************************
 */

/// ANPC Alert Notification Profile Client
#define BLE_AN_CLIENT               1
#if !defined (BLE_CLIENT_PRF)
    #define BLE_CLIENT_PRF          1
#endif 

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "anp_common.h"

#if (BLE_AN_CLIENT)

#include "ke_task.h"
#include "prf_types.h"

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// Alert Notification Service Characteristics
enum anpc_ans_chars
{
    /// Supported New Alert Category
    ANPC_CHAR_SUP_NEW_ALERT_CAT,
    /// New Alert
    ANPC_CHAR_NEW_ALERT,
    /// Supported Unread Alert Category
    ANPC_CHAR_SUP_UNREAD_ALERT_CAT,
    /// Unread Alert Status
    ANPC_CHAR_UNREAD_ALERT_STATUS,
    /// Alert Notification Control Point
    ANPC_CHAR_ALERT_NTF_CTNL_PT,

    ANPC_CHAR_MAX,
};

/// Alert Notification Service Characteristic Descriptors
enum anpc_ans_descs
{
    /// New Alert Char. - Client Characteristic Configuration
    ANPC_DESC_NEW_ALERT_CL_CFG,
    /// Unread Alert Status Char. - Client Characteristic Configuration
    ANPC_DESC_UNREAD_ALERT_STATUS_CL_CFG,

    ANPC_DESC_MAX,

    ANPC_DESC_MASK = 0x10,
};

/// Codes for reading/writing a ANS characteristic with one single request
enum anpc_rd_wr_ntf_codes
{
    /// Read ANS Supported New Alert Category
    ANPC_RD_SUP_NEW_ALERT_CAT           = ANPC_CHAR_SUP_NEW_ALERT_CAT,
    /// NTF New Alert
    ANPC_NTF_NEW_ALERT                  = ANPC_CHAR_NEW_ALERT,
    /// Read ANS Supported Unread Alert Category
    ANPC_RD_SUP_UNREAD_ALERT_CAT        = ANPC_CHAR_SUP_UNREAD_ALERT_CAT,
    /// NTF Unread Alert Categories
    ANPC_NTF_UNREAD_ALERT               = ANPC_CHAR_UNREAD_ALERT_STATUS,
    /// Write ANS Alert Notification Control Point
    ANPC_WR_ALERT_NTF_CTNL_PT           = ANPC_CHAR_ALERT_NTF_CTNL_PT,

    /// Read/Write ANS New Alert Client Characteristic Configuration Descriptor
    ANPC_RD_WR_NEW_ALERT_CFG            = (ANPC_DESC_NEW_ALERT_CL_CFG | ANPC_DESC_MASK),
    /// Read ANS Unread Alert Status Client Characteristic Configuration Descriptor
    ANPC_RD_WR_UNREAD_ALERT_STATUS_CFG  = (ANPC_DESC_UNREAD_ALERT_STATUS_CL_CFG | ANPC_DESC_MASK),
};

/// Pointer to the connection clean-up function
#define ANPC_CLEANUP_FNCT        (anpc_cleanup)
/*
 * STRUCTURES
 ****************************************************************************************
 */

/**
 * Structure containing the characteristics handles, value handles and descriptors for
 * the Alert Notification Service
 */
struct anpc_ans_content
{
    /// Service info
    struct prf_svc svc;

    /// Characteristic info:
    ///  - Supported New Alert Category
    ///  - New Alert
    ///  - Supported Unread Alert Category
    ///  - Unread Alert Status
    ///  - Alert Notification Control Point
    struct prf_char_inf chars[ANPC_CHAR_MAX];

    /// Descriptor handles:
    ///  - New Alert Client Cfg
    ///  - Unread Alert Status Client Cfg
    struct prf_char_desc_inf descs[ANPC_DESC_MAX];
};


/// Alert Notification Profile Client environment variable
struct anpc_env_tag
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

    ///Alert Notification Service Characteristics
    struct anpc_ans_content ans;
};

/// Command Message Basic Structure
struct anpc_cmd
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

/// Pool of Alert Notification Profile Client task environments.
extern struct anpc_env_tag **anpc_envs;

/*
 * GLOBAL FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Initialization of the ANPC module.
 * This function performs all the initializations of the ANPC module.
 ****************************************************************************************
 */
void anpc_init(void);


/**
 ****************************************************************************************
 * @brief Clean-up connection dedicated environment parameters
 * This function performs cleanup of ongoing operations
 * @param[in] idx_env Current connection environment variable
 ****************************************************************************************
 */
void anpc_cleanup(prf_env_struct *idx_env);

/**
 ****************************************************************************************
 * @brief Initialization of the ANPC module.
 * This function performs all the initializations of the ANPC module.
 ****************************************************************************************
 */
bool anpc_found_next_alert_cat(struct anpc_env_tag *idx_env,
                               struct anp_cat_id_bit_mask cat_id);

/**
 ****************************************************************************************
 * @brief Initialization of the ANPC module.
 * This function performs all the initializations of the ANPC module.
 ****************************************************************************************
 */
void anpc_write_alert_ntf_ctnl_pt(struct anpc_env_tag *idx_env, uint8_t cmd_id, uint8_t cat_id);

/**
 ****************************************************************************************
 * @brief Send a ANPC_CMP_EVT message to the task which enabled the profile
 ****************************************************************************************
 */
void anpc_send_no_conn_cmp_evt(uint8_t src_id, uint8_t dest_id, uint16_t conhdl, uint8_t operation);

/**
 ****************************************************************************************
 * @brief Send a ANPC_CMP_EVT message to the task which enabled the profile
 ****************************************************************************************
 */
void anpc_send_cmp_evt(struct anpc_env_tag *anpc_env, uint8_t operation, uint8_t status);

#endif //(BLE_AN_CLIENT)

/// @} ANPC

#endif //(_ANPC_H_)
