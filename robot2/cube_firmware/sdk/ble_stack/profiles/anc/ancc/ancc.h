/**
 ****************************************************************************************
 *
 * @file ancc.h
 *
 * @brief Header file - ANCS - Client Role.
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

#ifndef _ANCC_H_
#define _ANCC_H_

/**
 ****************************************************************************************
 * @addtogroup ANCC.
 * @ingroup ANC
 * @brief ANCS - Client Role.
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "anc_common.h"

#if (BLE_ANC_CLIENT)

#include "ke_task.h"
#include "prf_types.h"
#include "prf_utils.h"
#include "prf_utils_128.h"

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// ANCS Characteristics
enum ancc_anc_chars
{
    /// Notification Source
    ANCC_CHAR_NTF_SRC,
    /// Control Point
    ANCC_CHAR_CTRL_PT,
    /// Data Source
    ANCC_CHAR_DATA_SRC,

    ANCC_CHAR_MAX,
};

/// ANCS Characteristic Descriptors
enum ancc_anc_descs
{
    /// Notification Source - Client Characteristic Configuration
    ANCC_DESC_NTF_SRC_CL_CFG,
    /// Data Source - Client Characteristic Configuration
    ANCC_DESC_DATA_SRC_CL_CFG,

    ANCC_DESC_MAX,

    ANCC_DESC_MASK = 0x10,
};

/// Codes for reading/writing a ANC characteristic with one single request
enum ancc_rd_wr_ntf_codes
{
    /// Notification Source 
    ANCC_NTF_NTF_SRC         = ANCC_CHAR_NTF_SRC,

    /// Data Source
    ANCC_NTF_DATA_SRC        = ANCC_CHAR_DATA_SRC,

    /// Read/Write ANC Notification Source Client Characteristic Configuration Descriptor
    ANCC_RD_WR_NTF_SRC_CFG   = (ANCC_DESC_NTF_SRC_CL_CFG | ANCC_DESC_MASK),
    
    /// Read/Write ANC Data Source Client Characteristic Configuration Descriptor
    ANCC_RD_WR_DATA_SRC_CFG  = (ANCC_DESC_DATA_SRC_CL_CFG | ANCC_DESC_MASK),
};

/// Pointer to the connection clean-up function
#define ANCC_CLEANUP_FNCT        (ancc_cleanup)
/*
 * STRUCTURES
 ****************************************************************************************
 */

/**
 * Structure containing the characteristics handles, value handles and descriptors for
 * the Alert Notification Service
 */
struct ancc_anc_content
{
    /// Service info
    struct prf_svc svc;

    /// Characteristic info:
    ///  - Notification Source
    ///  - Control Point
    ///  - Data Source
    struct prf_char_inf chars[ANCC_CHAR_MAX];

    /// Descriptor handles:
    ///  - Notification Source Client Char Cfg
    ///  - Data Source Client Char Cfg
    struct prf_char_desc_inf descs[ANCC_DESC_MAX];
};

/// Structure for ANCS Notification reception variables
struct ancc_data_src_ntf_tag
{
    /// Notification Reception state
    uint8_t state;

    /// Number of requested attributes
    uint8_t nb_req_atts;

    /// Current attribute id
    uint8_t curr_att_id;

    /// Current attribute id
    uint8_t curr_app_id_length;
    
    /// Current attribute id
    uint8_t curr_att_length;

    /// Remaining bytes for current value
    uint16_t curr_value_idx;

    /// Last allocated notification
    void *alloc_rsp_ind;
};

/// ANCS Client environment variable
struct ancc_env_tag
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

    /// ANCS Characteristics
    struct ancc_anc_content anc;
    
    /// Data Source Notification reception variables structure
    struct ancc_data_src_ntf_tag data_src;
};

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// Pool of ANCS Client task environments.
extern struct ancc_env_tag **ancc_envs;

/*
 * GLOBAL FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Initialization of the ANCC module.
 * This function performs all the initializations of the ANCC module.
 ****************************************************************************************
 */
void ancc_init(void);


/**
 ****************************************************************************************
 * @brief Clean-up connection dedicated environment parameters
 * This function performs cleanup of ongoing operations
 * @param[in] idx_env Current connection environment variable
 ****************************************************************************************
 */
void ancc_cleanup(prf_env_struct *idx_env);

/**
 ****************************************************************************************
 * @brief Send a ANCC_CMP_EVT message to the task which enabled the profile
 ****************************************************************************************
 */
void ancc_send_no_conn_cmp_evt(uint8_t src_id, uint8_t dest_id, uint16_t conhdl, uint8_t operation);

/**
 ****************************************************************************************
 * @brief Send a ANCC_CMP_EVT message to the task which enabled the profile
 ****************************************************************************************
 */
void ancc_send_cmp_evt(struct ancc_env_tag *ancc_env, uint8_t operation, uint8_t status);

#endif //(BLE_ANC_CLIENT)

/// @} ANCC

#endif //(_ANCC_H_)
