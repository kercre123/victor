/**
 ****************************************************************************************
 *
 * @file anps.h
 *
 * @brief Header file - Alert Notification Profile Server.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 * $Rev: $
 *
 ****************************************************************************************
 */

#ifndef ANPS_H_
#define ANPS_H_

/**
 ****************************************************************************************
 * @addtogroup ANPS Alert Notification Profile Server
 * @ingroup ANP
 * @brief Alert Notification Profile Server
 * @{
 ****************************************************************************************
 */


/// ANPS Alert Notification Profile Server Role
#define BLE_AN_SERVER           1
#if !defined (BLE_SERVER_PRF)
    #define BLE_SERVER_PRF      1
#endif

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "anp_common.h"

#if (BLE_AN_SERVER)

#include "prf_types.h"

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// Alert Notification Service Attributes
enum anps_ans_att_list
{
    ANS_IDX_SVC,

    ANS_IDX_SUPP_NEW_ALERT_CAT_CHAR,
    ANS_IDX_SUPP_NEW_ALERT_CAT_VAL,

    ANS_IDX_NEW_ALERT_CHAR,
    ANS_IDX_NEW_ALERT_VAL,
    ANS_IDX_NEW_ALERT_CFG,

    ANS_IDX_SUPP_UNREAD_ALERT_CAT_CHAR,
    ANS_IDX_SUPP_UNREAD_ALERT_CAT_VAL,

    ANS_IDX_UNREAD_ALERT_STATUS_CHAR,
    ANS_IDX_UNREAD_ALERT_STATUS_VAL,
    ANS_IDX_UNREAD_ALERT_STATUS_CFG,

    ANS_IDX_ALERT_NTF_CTNL_PT_CHAR,
    ANS_IDX_ALERT_NTF_CTNL_PT_VAL,

    ANS_IDX_NB,
};

/// Pointer to the connection clean-up function
#define ANPS_CLEANUP_FNCT        (NULL)
/*
 * STRUCTURES
 ****************************************************************************************
 */

/// Alert Notification Profile Server. Environment variable
struct anps_env_tag
{
    /// ANS Start Handle
    uint16_t ans_shdl;

    /// Supported New Alert Category Characteristic Value
    uint16_t supp_new_alert_cat;
    /// Supported Unread Alert Category Characteristic Value
    uint16_t supp_unread_alert_cat;
};

/// Alert Notification Profile Server Instance Environment variable
struct anps_idx_env_tag
{
    /// Connection Info
    struct prf_con_info con_info;

    /// Operation
    uint8_t operation;

    /**
     * Client Characteristic Configuration Status
     *   Bit 0 : New Alert Characteristic
     *   Bit 1 : Unread Alert Status Characteristic
     */
    uint8_t ntf_cfg;

    /**
     * Category Notification Configuration
     *   Bit 0 : Simple Alert
     *   Bit 1 : Email
     *   Bit 2 : News
     *   Bit 3 : Call
     *   Bit 4 : Missed Call
     *   Bit 5 : SMS/MMS
     *   Bit 6 : Voice Mail
     *   Bit 7 : Schedule
     *   Bit 8 : High Prioritized Alert
     *   Bit 9 : Instance Message
     */
    uint16_t ntf_new_alert_cfg;
    uint16_t ntf_unread_alert_cfg;
};

/*
 * GLOBAL VARIABLE DECLARATIONS
 ****************************************************************************************
 */

extern const struct attm_desc ans_att_db[ANS_IDX_NB];

/// Environment common to each task instance
extern struct anps_env_tag anps_env;
/// Pool of tips environments
extern struct anps_idx_env_tag **anps_idx_envs;

/*
 * MACROS
 * **************************************************************************************
 */

#define ANPS_IS_NEW_ALERT_CATEGORY_SUPPORTED(category_id) \
        (((anps_env.supp_new_alert_cat >> category_id) & 1) == 1)

#define ANPS_IS_UNREAD_ALERT_CATEGORY_SUPPORTED(category_id) \
        (((anps_env.supp_unread_alert_cat >> category_id) & 1) == 1)

#define ANPS_IS_ALERT_ENABLED(idx_env, alert_type) \
        (((idx_env->ntf_cfg >> alert_type) & 1) == 1)

#define ANPS_IS_NEW_ALERT_CATEGORY_ENABLED(category_id, idx_env) \
        (((idx_env->ntf_new_alert_cfg >> category_id) & 1) == 1)

#define ANPS_IS_UNREAD_ALERT_CATEGORY_ENABLED(category_id, idx_env) \
        (((idx_env->ntf_unread_alert_cfg >> category_id) & 1) == 1)

#define ANPS_ENABLE_ALERT(idx_env, alert_type) \
        (idx_env->ntf_cfg |= (1 << alert_type))

#define ANPS_DISABLE_ALERT(idx_env, alert_type) \
        (idx_env->ntf_cfg &= ~(1 << alert_type))

#define ANPS_ENABLE_NEW_ALERT_CATEGORY(category_id, idx_env) \
        (idx_env->ntf_new_alert_cfg |= (1 << category_id))

#define ANPS_ENABLE_UNREAD_ALERT_CATEGORY(category_id, idx_env) \
        (idx_env->ntf_unread_alert_cfg |= (1 << category_id))

#define ANPS_DISABLE_NEW_ALERT_CATEGORY(category_id, idx_env) \
        (idx_env->ntf_new_alert_cfg &= ~(1 << category_id))

#define ANPS_DISABLE_UNREAD_ALERT_CATEGORY(category_id, idx_env) \
		(idx_env->ntf_unread_alert_cfg &= ~(1 << category_id))

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Initialization of the ANPS module.
 * This function performs all the initializations of the ANPS module.
 ****************************************************************************************
 */
void anps_init(void);

/**
 ****************************************************************************************
 * @brief Send an ANPS_NTF_STATUS_UPDATE_IND message to a requester.
 *
 * @param[in] src_id        Source ID of the message (instance of TASK_ANPS)
 * @param[in] dest_id       Destination ID of the message
 * @param[in] operation     Code of the completed operation
 * @param[in] status        Status of the request
 ****************************************************************************************
 */
void anps_send_ntf_status_update_ind(struct anps_idx_env_tag *idx_env, uint8_t alert_type);

/**
 ****************************************************************************************
 * @brief Send an ANPS_NTF_IMMEDIATE_REQ_IND message to a requester.
 *
 * @param[in] src_id        Source ID of the message (instance of TASK_ANPS)
 * @param[in] dest_id       Destination ID of the message
 * @param[in] operation     Code of the completed operation
 * @param[in] status        Status of the request
 ****************************************************************************************
 */
void anps_send_ntf_immediate_req_ind(struct anps_idx_env_tag *idx_env, uint8_t alert_type,
                                     uint8_t category_id);

/**
 ****************************************************************************************
 * @brief Send an ANPS_CMP_EVT message to a requester.
 *
 * @param[in] src_id        Source ID of the message (instance of TASK_ANPS)
 * @param[in] dest_id       Destination ID of the message
 * @param[in] operation     Code of the completed operation
 * @param[in] status        Status of the request
 ****************************************************************************************
 */
void anps_send_cmp_evt(ke_task_id_t src_id, ke_task_id_t dest_id, uint16_t conhdl,
                       uint8_t operation, uint8_t status);

/**
 ****************************************************************************************
 * @brief Disable actions grouped in getting back to IDLE and sending configuration to requester task
 ****************************************************************************************
 */
void anps_disable(struct anps_idx_env_tag *anps_idx_env, uint16_t conhdl);

#endif //(BLE_AN_SERVER)

/// @} ANPS

#endif //(ANPS_H_)
