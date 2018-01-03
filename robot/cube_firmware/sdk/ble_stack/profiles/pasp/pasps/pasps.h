/**
 ****************************************************************************************
 *
 * @file pasps.h
 *
 * @brief Header file - Phone Alert Status Profile Server.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 * $Rev: $
 *
 ****************************************************************************************
 */

#ifndef PASPS_H_
#define PASPS_H_

/**
 ****************************************************************************************
 * @addtogroup PASPS Phone Alert Status Profile Server
 * @ingroup PASP
 * @brief Phone Alert Status Profile Server
 * @{
 ****************************************************************************************
 */


/// PASPS Phone Alert Status Profile Server Role
#define BLE_PAS_SERVER          1
#if !defined (BLE_SERVER_PRF)
    #define BLE_SERVER_PRF      1
#endif

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "pasp_common.h"

#if (BLE_PAS_SERVER)

#include <stdint.h>
#include <stdbool.h>
#include "prf_types.h"

/*
 * DEFINES
 ****************************************************************************************
 */

/// Number of parallel instances of the task
#define PASPS_IDX_MAX           (BLE_CONNECTION_MAX)
/// Database Configuration Flag
#define PASPS_DB_CFG_FLAG       (0x01FF)

/// Notification States Flags
#define PASPS_FLAG_ALERT_STATUS_CFG         (0x01)
#define PASPS_FLAG_RINGER_SETTING_CFG       (0x02)

/*
 * MACROS
 ****************************************************************************************
 */

#define PASPS_IS_NTF_ENABLED(idx_env, flag) ((idx_env->ntf_state & flag) == flag)

#define PASPS_ENABLE_NTF(idx_env, flag)     (idx_env->ntf_state |= flag)

#define PASPS_DISABLE_NTF(idx_env, flag)    (idx_env->ntf_state &= ~flag)

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// Attributes State Machine
enum pasps_pass_att_list
{
    PASS_IDX_SVC,

    PASS_IDX_ALERT_STATUS_CHAR,
    PASS_IDX_ALERT_STATUS_VAL,
    PASS_IDX_ALERT_STATUS_CFG,

    PASS_IDX_RINGER_SETTING_CHAR,
    PASS_IDX_RINGER_SETTING_VAL,
    PASS_IDX_RINGER_SETTING_CFG,

    PASS_IDX_RINGER_CTNL_PT_CHAR,
    PASS_IDX_RINGER_CTNL_PT_VAL,

    PASS_IDX_NB,
};

/// Attribute Codes
enum pasps_att_code
{
    /// Alert Status Characteristic
    PASPS_ALERT_STATUS_CHAR_VAL,
    /// Ringer Setting Characteristic
    PASPS_RINGER_SETTING_CHAR_VAL,
    /// Ringer Control Point Characteristic
    PASPS_RINGER_CTNL_PT_CHAR_VAL,

    /// Alert Status Characteristic - Notification Configuration
    PASPS_ALERT_STATUS_NTF_CFG,
    /// Ringer Setting Characteristic - Notification Configuration
    PASPS_RINGER_SETTING_NTF_CFG,
};


/// Pointer to the connection clean-up function
#define PASPS_CLEANUP_FNCT        (NULL)

/*
 * STRUCTURES
 ****************************************************************************************
 */

/// Phone Alert Status Profile Server Environment Variable
struct pasps_env_tag
{
    /// Phone Alert Status Service Start Handle
    uint16_t pass_shdl;
    /// Ringer State
    uint8_t ringer_state;
};

/// Phone Alert Status Profile Server Connection Dependent Environment Variable
struct pasps_idx_env_tag
{
    /// Connection Info
    struct prf_con_info con_info;

    /// Current Operation Code
    uint8_t operation;

    /**
     * Ringer State + Notification State
     *     Bit 0: Alert Status notification configuration
     *     Bit 1: Ringer setting notification configuration
     */
    uint8_t ntf_state;
};

/*
 * GLOBAL VARIABLES DECLARATION
 ****************************************************************************************
 */

/// Array containing attribute to add into the database
extern const struct attm_desc pasps_att_db[PASS_IDX_NB];

/// Phone Alert Status Service
extern const att_svc_desc_t pasps_pass_svc;

/// Alert Status Char.
extern const struct att_char_desc pasps_alert_status_char;
/// Ringer Status Char.
extern const struct att_char_desc pasps_ringer_setting_char;
/// Ringer Control Point Char.
extern const struct att_char_desc pasps_ringer_ctnl_pt_char;

/// Phone Alert Status Profile Server Environment Variable
extern struct pasps_env_tag pasps_env;
/// Phone Alert Status Profile Server Pool of Environments
extern struct pasps_idx_env_tag **pasps_idx_envs;

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Initialization of the PASPS module.
 *        This function performs all the initializations of the PASPS module.
 ****************************************************************************************
 */
void pasps_init(void);

/**
 ****************************************************************************************
 * @brief Send a PASPS_CMP_EVT message to a requester.
 *
 * @param[in] src_id        Source ID of the message (instance of TASK_PASPS)
 * @param[in] dest_id       Destination ID of the message
 * @param[in] operation     Code of the completed operation
 * @param[in] status        Status of the request
 ****************************************************************************************
 */
void pasps_send_cmp_evt(ke_task_id_t src_id, ke_task_id_t dest_id, uint16_t conhdl,
                        uint8_t operation, uint8_t status);

/**
 ****************************************************************************************
 * @brief Disable actions grouped in getting back to IDLE and sending configuration to
 *        requester task.
 ****************************************************************************************
 */
void pasps_disable(struct pasps_idx_env_tag *idx_env, uint16_t conhdl);

#endif //(BLE_PAS_SERVER)

/// @} PASPS

#endif // PASPS_H_
