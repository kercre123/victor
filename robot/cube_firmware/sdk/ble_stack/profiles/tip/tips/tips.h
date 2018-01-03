/**
 ****************************************************************************************
 *
 * @file tips.h
 *
 * @brief Header file - Time Profile Server.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 *
 ****************************************************************************************
 */

#ifndef TIPS_H_
#define TIPS_H_

/**
 ****************************************************************************************
 * @addtogroup TIPS Time Profile Server
 * @ingroup TIP
 * @brief Time Profile Server
 * @{
 ****************************************************************************************
 */
 
/// TIPS Time Profile Server Role
#define BLE_TIP_SERVER          1
#if !defined (BLE_SERVER_PRF)
    #define BLE_SERVER_PRF      1
#endif


/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#if (BLE_TIP_SERVER)
#include <stdint.h>
#include <stdbool.h>
#include "tip_common.h"
#include "prf_types.h"
#include "gap.h"

/*
 * DEFINES
 ****************************************************************************************
 */

//CTS Configuration Flag Masks
#define TIPS_CTS_CURRENT_TIME_MASK        (0x0F)
#define TIPS_CTS_LOC_TIME_INFO_MASK        (0x30)
#define TIPS_CTS_REF_TIME_INFO_MASK        (0xC0)

//Packed Values Length
#define CTS_CURRENT_TIME_VAL_LEN        (10)
#define NDCS_TIME_DST_VAL_LEN            (8)

/*
 * MACROS
 ****************************************************************************************
 */

#define TIPS_IS_SUPPORTED(mask) (((tips_env.features & mask) == mask))

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

///Database Configuration - Bit Field Flags
enum
{
    TIPS_CTS_LOC_TIME_INFO_SUP   = 0x01,
    TIPS_CTS_REF_TIME_INFO_SUP   = 0x02,
    TIPS_NDCS_SUP                = 0x04,
    TIPS_RTUS_SUP                = 0x08,

    TIPS_CTS_CURRENT_TIME_CFG    = 0x10,
};

enum
{
    CTS_IDX_SVC,

    CTS_IDX_CURRENT_TIME_CHAR,
    CTS_IDX_CURRENT_TIME_VAL,
    CTS_IDX_CURRENT_TIME_CFG,

    CTS_IDX_LOCAL_TIME_INFO_CHAR,
    CTS_IDX_LOCAL_TIME_INFO_VAL,

    CTS_IDX_REF_TIME_INFO_CHAR,
    CTS_IDX_REF_TIME_INFO_VAL,

    CTS_IDX_NB,
};

enum
{
    NDCS_IDX_SVC,
    NDCS_IDX_TIME_DST_CHAR,
    NDCS_IDX_TIME_DST_VAL,

    NDCS_IDX_NB,
};

enum
{
    RTUS_IDX_SVC,
    RTUS_IDX_TIME_UPD_CTNL_PT_CHAR,
    RTUS_IDX_TIME_UPD_CTNL_PT_VAL,
    RTUS_IDX_TIME_UPD_STATE_CHAR,
    RTUS_IDX_TIME_UPD_STATE_VAL,

    RTUS_IDX_NB,
};

enum
{
    CTS_CURRENT_TIME_CHAR,
    CTS_LOCAL_TIME_INFO_CHAR,
    CTS_REF_TIME_INFO_CHAR,

    CTS_CHAR_MAX,
};

enum
{
    NDCS_TIME_DST_CHAR,

    NDCS_CHAR_MAX,
};

enum
{
    RTUS_TIME_UPD_CTNL_PT_CHAR,
    RTUS_TIME_UPD_STATE_CHAR,

    RTUS_CHAR_MAX,
};

/// Pointer to the connection clean-up function
#define TIPS_CLEANUP_FNCT        (NULL)


/*
 * STRUCTURES
 ****************************************************************************************
 */

/// Time Profile Server. Environment variable
struct tips_env_tag
{
    /// CTS Start Handle
    uint16_t cts_shdl;
    /// NDCS Start Handle
    uint16_t ndcs_shdl;
    /// RTUS Start Handle
    uint16_t rtus_shdl;

    /// CTS Attribute Table
    uint8_t cts_att_tbl[CTS_CHAR_MAX];
    /// NDCS Attribute Table
    uint8_t ndcs_att_tbl[NDCS_CHAR_MAX];
    /// NDCS Attribute Table
    uint8_t rtus_att_tbl[RTUS_CHAR_MAX];

    /// Database configuration
    uint8_t features;
};

/// Time Profile Server Instance Environment variable
struct tips_idx_env_tag
{
    /// Connection Info
    struct prf_con_info con_info;

    /// NTF State
    uint8_t ntf_state;
};

/*
 * GLOBAL VARIABLE DECLARATIONS
 ****************************************************************************************
 */

extern const struct attm_desc cts_att_db[CTS_IDX_NB];
extern const struct attm_desc ndcs_att_db[NDCS_IDX_NB];
extern const struct attm_desc rtus_att_db[RTUS_IDX_NB];

/*
 * Services
 */
/// Current Time Service
extern const att_svc_desc_t tips_cts_svc;
/// Next DST Change Service
extern const att_svc_desc_t tips_ndcs_svc;
/// Reference Time Update Service
extern const att_svc_desc_t tips_rtus_svc;

/*
 * Current Time Service Characteristics
 */
///Current Time Characteristic
extern const struct att_char_desc tips_curr_time_char;
///Local Time Info Characteristic
extern const struct att_char_desc tips_loc_time_info_char;
///Reference Time Info Characteristic
extern const struct att_char_desc tips_ref_time_info_char;

/*
 * Next DST Change Service Characteristics
 */
///Time with DST Characteristic
extern const struct att_char_desc tips_time_with_dst_char;

/*
 * Reference Time Update Service Characteristics
 */
///Time Update Control Point Characteristic
extern const struct att_char_desc tips_time_upd_contr_pt_char;
///Time Update State
extern const struct att_char_desc tips_time_upd_state_char;

extern struct tips_env_tag tips_env;
extern struct tips_idx_env_tag **tips_idx_envs;

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Initialization of the TIPS module.
 * This function performs all the initializations of the TIPS module.
 ****************************************************************************************
 */
void tips_init(void);

/**
 ****************************************************************************************
 * @brief Send a TIPS_ENABLE_CFM message to the application
 ****************************************************************************************
 */
void tips_enable_cfm_send(struct prf_con_info *con_info, uint8_t status);

/**
 ****************************************************************************************
 * @brief Send a TIPS_ERROR_IND message to the application
 ****************************************************************************************
 */
void tips_error_ind_send(struct prf_con_info *con_info, uint8_t status, ke_msg_id_t msg_id);

/**
 ****************************************************************************************
 * @brief Pack Current Time value
 *
 * @param p_curr_time_val Current Time value
 ****************************************************************************************
 */
void tips_pack_curr_time_value(uint8_t *p_pckd_time,
                               const struct tip_curr_time* p_curr_time_val);
/**
 ****************************************************************************************
 * @brief Pack Time With DST value
 *
 * @param p_time_dst_val Time With DST value
 ****************************************************************************************
 */
void tips_pack_time_dst_value(uint8_t *p_pckd_time_dst,
                              const struct tip_time_with_dst* p_time_dst_val);

/**
 ****************************************************************************************
 * @brief Disable actions grouped in getting back to IDLE and sending configuration to requester task
 ****************************************************************************************
 */
void tips_disable(struct tips_idx_env_tag *tips_idx_env, uint16_t conhdl);

#endif //BLE_TIP_SERVER

/// @} TIPS

#endif // TIPS_H_
