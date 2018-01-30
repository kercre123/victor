/**
 ****************************************************************************************
 *
 * @file bmss.c
 *
 * @brief C file - Bond Management Service Server Implementation.
 *
 * Copyright (C) 2015. Dialog Semiconductor Ltd, unpublished work. This computer
 * program includes Confidential, Proprietary Information and is a Trade Secret of
 * Dialog Semiconductor Ltd. All use, disclosure, and/or reproduction is prohibited
 * unless authorized in writing. All Rights Reserved.
 *
 * <bluetooth.support@diasemi.com> and contributors.
 *
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @addtogroup BMSS
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"

#if (BLE_BM_SERVER)
#include "bmss.h"
#include "bmss_task.h"
#include "attm_db.h"

/*
 * BMS ATTRIBUTES
 ****************************************************************************************
 */

/// Bond Management Service
const att_svc_desc_t bms_svc = ATT_SVC_BOND_MANAGEMENT;

/// Bond Management Control Point
const struct att_char_desc bms_ctrl_pt_char = ATT_CHAR(ATT_CHAR_PROP_WR | ATT_CHAR_PROP_EXT_PROP,
                                                       BMS_CTRL_PT_CHAR,
                                                       ATT_CHAR_BM_CTRL_PT);

/// Bond Management Feature
const struct att_char_desc bms_feat_char    = ATT_CHAR(ATT_CHAR_PROP_RD,
                                                       BMS_FEAT_CHAR,
                                                       ATT_CHAR_BM_FEAT);

/// Full BMS Database Description - Used to add attributes into the database
const struct attm_desc bms_att_db[BMS_IDX_NB] =
{
    // Bond Management Service Declaration
    [BMS_IDX_SVC]                 = {ATT_DECL_PRIMARY_SERVICE, PERM(RD, ENABLE), sizeof(bms_svc),
                                     sizeof(bms_svc), (uint8_t *)&bms_svc},

    // Bond Managment Control Point Characteristic Declaration
    [BMS_IDX_BM_CTRL_PT_CHAR]     = {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), sizeof(bms_ctrl_pt_char),
                                     sizeof(bms_ctrl_pt_char), (uint8_t *)&bms_ctrl_pt_char},

    // Bond Managment Control Point Characteristic Value
    [BMS_IDX_BM_CTRL_PT_VAL]      = {ATT_CHAR_BM_CTRL_PT, PERM(WR, ENABLE), BMS_CTRL_PT_MAX_LEN, 0, NULL},

    // Bond Management Control Point Characteristic Extended Properties
    [BMS_IDX_BM_CTRL_PT_EXT_PROP] = {ATT_DESC_CHAR_EXT_PROPERTIES, PERM(RD, ENABLE), sizeof(uint16_t),
                                     0, NULL},

    // Bond Management Feature Characteristic Declaration
    [BMS_IDX_BM_FEAT_CHAR]        = {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), sizeof(bms_feat_char),
                                     sizeof(bms_feat_char), (uint8_t *)&bms_feat_char},

    // Bond Management Feature Characteristic Value
    [BMS_IDX_BM_FEAT_VAL]         = {ATT_CHAR_BM_FEAT, PERM(RD, ENABLE), BMS_FEAT_MAX_LEN, 0, NULL},
};

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

struct bmss_env_tag bmss_env __attribute__((section("retention_mem_area0"),zero_init));

/// BMSS task descriptor
static const struct ke_task_desc TASK_DESC_BMSS = {bmss_state_handler, &bmss_default_handler, bmss_state, BMSS_STATE_MAX, BMSS_IDX_MAX};

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

void bmss_init(void)
{
    // Reset Environment
    memset(&bmss_env, 0, sizeof(bmss_env));

    // Create bms task
    ke_task_create(TASK_BMSS, &TASK_DESC_BMSS);

    ke_state_set(TASK_BMSS, BMSS_DISABLED);
}

void bmss_disable(uint16_t conhdl)
{
    //Disable BMS in database
    attmdb_svc_set_permission(bmss_env.shdl, PERM_RIGHT_DISABLE); // WARNING : is it required ?

    // Send current configuration to the application
    struct bmss_disable_ind *ind = KE_MSG_ALLOC(BMSS_DISABLE_IND,
                                                 bmss_env.con_info.appid, TASK_BMSS,
                                                 bmss_disable_ind);

    ind->conhdl = conhdl;

    ke_msg_send(ind);

    // Go to idle state
    ke_state_set(TASK_BMSS, BMSS_IDLE);
}

#endif // BLE_BM_SERVER

/// @} BMSS
