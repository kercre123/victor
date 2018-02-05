/**
 ****************************************************************************************
 *
 * @file lans.h
 *
 * @brief Header file - Location and Navigation Profile Sensor.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 * $ Rev $
 *
 ****************************************************************************************
 */

#ifndef _LANS_H_
#define _LANS_H_

/**
 ****************************************************************************************
 * @addtogroup LANS Location and Navigation Profile Sensor
 * @ingroup LAN
 * @brief Location and Navigation Profile Sensor
 * @{
 ****************************************************************************************
 */
 
/// LANS Location and Navigation Profile Sensor Role
#define BLE_LN_SENSOR           1
#if !defined (BLE_SERVER_PRF)
    #define BLE_SERVER_PRF      1
#endif


/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "lan_common.h"

#if (BLE_LN_SENSOR)

#include "prf_types.h"

/*
 * DEFINES
 ****************************************************************************************
 */

/********************************************
 ******* LANS Configuration Flag Masks ******
 ********************************************/

/// Mandatory Attributes (LN Feature + Location and Speed)
#define LANS_MANDATORY_MASK           (0x003F)
/// Position quality Attribute
#define LANS_POS_Q_MASK               (0x00C0)
/// Control Point Attributes
#define LANS_LN_CTNL_PT_MASK          (0x0700)
/// Navigation Attributes
#define LANS_NAVI_MASK                (0x3800)


/*
 * MACROS
 ****************************************************************************************
 */

#define LANS_IS_FEATURE_SUPPORTED(features, flag) ((features & flag) == flag)

#define LANS_IS_PRESENT(features, flag)           ((features & flag) == flag)

#define LANS_IS_SET(features, flag)                (features & flag)

#define LANS_IS_CLEAR(features, flag)              ((features & flag) == 0)

#define LANS_ENABLE_NTF_IND(ccc_flag)              (lans_env.prfl_ntf_ind_cfg |= ccc_flag)

#define LANS_DISABLE_NTF_IND(ccc_flag)             (lans_env.prfl_ntf_ind_cfg &= ~ccc_flag)

#define LANS_IS_NTF_IND_ENABLED(ccc_flag)          ((lans_env.prfl_ntf_ind_cfg & ccc_flag) == ccc_flag)

// MACRO TO CALCULATE HANDLE    shdl + idx - POSQ - CNTL PT
// POSQ is 2 if the Position quality is supported otherwise 0
// CNTL PT is 3 if the CP characteristic is supported otherwise 0
#define LANS_HANDLE(idx) \
    (lans_env.shdl + (idx) - \
        ((!(LANS_IS_FEATURE_SUPPORTED(lans_env.prfl_cfg, LANS_POS_Q_MASK)) && \
                ((idx) > LNS_IDX_POS_Q_CHAR))? (2) : (0)) - \
        ((!(LANS_IS_FEATURE_SUPPORTED(lans_env.prfl_cfg, LANS_LN_CTNL_PT_MASK)) && \
                ((idx) > LNS_IDX_LN_CTNL_PT_CHAR))? (3) : (0)))
/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// Location and Navigation Service - Attribute List
enum lans_cps_att_list
{
    /// Location and Navigation Service
    LNS_IDX_SVC,
    /// LN Feature
    LNS_IDX_LN_FEAT_CHAR,
    LNS_IDX_LN_FEAT_VAL,
    /// Location and Speed
    LNS_IDX_LOC_SPEED_CHAR,
    LNS_IDX_LOC_SPEED_VAL,
    LNS_IDX_LOC_SPEED_NTF_CFG,
    /// Position Quality
    LNS_IDX_POS_Q_CHAR,
    LNS_IDX_POS_Q_VAL,
    /// LN Control Point
    LNS_IDX_LN_CTNL_PT_CHAR,
    LNS_IDX_LN_CTNL_PT_VAL,
    LNS_IDX_LN_CTNL_PT_IND_CFG,
    /// Navigation
    LNS_IDX_NAVIGATION_CHAR,
    LNS_IDX_NAVIGATION_VAL,
    LNS_IDX_NAVIGATION_NTF_CFG,

    /// Number of attributes
    LNS_IDX_NB,
};

/// Profile Configuration
enum lans_prf_cfg_flag
{
    /// Nothing supported
    LANS_NOTHING_SUPP_FLAG  = (0x00),
    /// Control Point supported flag
    LANS_CTNL_PT_CHAR_SUPP_FLAG  = (0x01),
    /// Broadcast supported flag
    LANS_NAVIGATION_SUPP_FLAG    = (0x02),
};


/// Profile Configuration NTF/IND Flags ()
enum lans_prf_cfg_ntfind_flag
{
    /// Location and Speed - Client Char. Cfg
    LANS_PRF_CFG_FLAG_LOC_SPEED_NTF   = 0x01,

    /// Control Point - Client Characteristic configuration
    LANS_PRF_CFG_FLAG_LN_CTNL_PT_IND  = 0x02,

    /// Navigation - Client Characteristic configuration
    LANS_PRF_CFG_FLAG_NAVIGATION_NTF  = 0x04,
};


/// Pointer to the connection clean-up function
#define LANS_CLEANUP_FNCT        (NULL)


/*
 * STRUCTURES
 ****************************************************************************************
 */

/// Location and Navigation Profile Sensor environment variable
struct lans_env_tag
{
    /// Connection Information
    struct prf_con_info con_info;

    /// Cycling Speed and Cadence Service Start Handle
    uint16_t shdl;

    /// Characteristic Handle offsets
    uint8_t hdl_offset[LANP_LANS_CHAR_MAX];

    /// Operation
    uint8_t operation;

    /// Profile Configuration Flags
    uint16_t prfl_cfg;

    /// Feature Configuration Flags
    uint32_t feat_cfg;

    /// Profile Notify/Indication Flags
    uint8_t prfl_ntf_ind_cfg;

    /// notification pending
    bool ntf_pending;

    /// LN Navigation Control parameter
    uint8_t navi_control;

    /// LN Navigation enabled
    bool navi_enabled;

    /// Buffer for subsequent notification
    uint8_t ntf[ATT_DEFAULT_MTU - 3];

    /// Length for subsequent notification
    uint8_t ntf_len;

};

/*
 * GLOBAL VARIABLE DECLARATIONS
 ****************************************************************************************
 */

/// Cycling Speed and Cadence Sensor Environment Variable
extern struct lans_env_tag lans_env;

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Initialization of the GLPS module.
 * This function performs all the initializations of the GLPS module.
 ****************************************************************************************
 */
void lans_init(void);

/**
 ****************************************************************************************
 * @brief Send a response indication without parameters to the peer device.
 ****************************************************************************************
 */
void lans_send_rsp_ind(uint16_t handle, uint8_t req_op_code, uint8_t status);

/**
 ****************************************************************************************
 * @brief Send a LANS_CMP_EVT message to the application.
 ****************************************************************************************
 */
void lans_send_cmp_evt(uint8_t src_id, uint8_t dest_id, uint16_t conhdl,
                        uint8_t operation, uint8_t status);

/**
 ****************************************************************************************
 * @brief Disable actions grouped in getting back to IDLE and sending configuration to
 * requester task
 ****************************************************************************************
 */
void lans_disable(uint16_t conhdl);

#endif //(BLE_LN_SENSOR)

/// @} LANS

#endif //(_LANS_H_)
