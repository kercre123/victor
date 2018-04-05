/**
 ****************************************************************************************
 *
 * @file proxr.c
 *
 * @brief Proximity Reporter Implementation.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 *
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @addtogroup PROXR
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"

#if (BLE_PROX_REPORTER)

#include "proxr.h"
#include "proxr_task.h"
#include "attm_db.h"
#include "gap.h"

/*
 * PROXIMITY PROFILE ATTRIBUTES DEFINITION
 ****************************************************************************************
 */

/// Full LLS Database Description - Used to add attributes into the database
const struct attm_desc proxr_lls_att_db[LLS_IDX_NB] =
{
    // Link Loss Service Declaration
    [LLS_IDX_SVC]                      =   {ATT_DECL_PRIMARY_SERVICE, PERM(RD, ENABLE), sizeof(proxr_lls_svc),
                                         sizeof(proxr_lls_svc), (uint8_t *)&proxr_lls_svc},

    // Alert Level Characteristic Declaration
    [LLS_IDX_ALERT_LVL_CHAR]        =   {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), sizeof(proxr_lls_alert_lvl_char),
                                         sizeof(proxr_lls_alert_lvl_char), (uint8_t *)&proxr_lls_alert_lvl_char},
    // Alert Level Characteristic Value
    [LLS_IDX_ALERT_LVL_VAL]         =   {ATT_CHAR_ALERT_LEVEL, PERM(RD, ENABLE) | PERM(WR, ENABLE), sizeof(uint8_t),
                                         0, NULL},
};

/// Full IAS Database Description - Used to add attributes into the database
const struct attm_desc proxr_ias_att_db[IAS_IDX_NB] =
{
    // Immediate Alert Service Declaration
    [IAS_IDX_SVC]                      =   {ATT_DECL_PRIMARY_SERVICE, PERM(RD, ENABLE), sizeof(proxr_ias_svc),
                                         sizeof(proxr_ias_svc), (uint8_t *)&proxr_ias_svc},

    // Alert Level Characteristic Declaration
    [IAS_IDX_ALERT_LVL_CHAR]        =   {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), sizeof(proxr_ias_alert_lvl_char),
                                         sizeof(proxr_ias_alert_lvl_char), (uint8_t *)&proxr_ias_alert_lvl_char},
    // Alert Level Characteristic Value
    [IAS_IDX_ALERT_LVL_VAL]         =   {ATT_CHAR_ALERT_LEVEL, PERM(WR, ENABLE), sizeof(uint8_t),
                                         0, NULL},
};

/// Full TXPS Database Description - Used to add attributes into the database
const struct attm_desc proxr_txps_att_db[TXPS_IDX_NB] =
{
    // TX Power Service Declaration
    [TXPS_IDX_SVC]                  =   {ATT_DECL_PRIMARY_SERVICE, PERM(RD, ENABLE), sizeof(proxr_txps_svc),
                                         sizeof(proxr_txps_svc), (uint8_t *)&proxr_txps_svc},

    // TX Power Level Characteristic Declaration
    [TXPS_IDX_TX_POWER_LVL_CHAR]       =   {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), sizeof(proxr_txps_tx_power_lvl_char),
                                         sizeof(proxr_txps_tx_power_lvl_char), (uint8_t *)&proxr_txps_tx_power_lvl_char},
    // TX Power Level Characteristic Value
    [TXPS_IDX_TX_POWER_LVL_VAL]     =   {ATT_CHAR_TX_POWER_LEVEL, PERM(RD, ENABLE), sizeof(int8_t),
                                         0, NULL},
};

/*
 *  PROXIMITY PROFILE ATTRIBUTES VALUES DEFINTION
 ****************************************************************************************
 */

/// Link Loss Service
const att_svc_desc_t proxr_lls_svc     = ATT_SVC_LINK_LOSS;

/// Link Loss Service - Alert Level Characteristic
const struct att_char_desc proxr_lls_alert_lvl_char = ATT_CHAR(ATT_CHAR_PROP_RD | ATT_CHAR_PROP_WR,
                                                                    0,
                                                                    ATT_CHAR_ALERT_LEVEL);

/// Immediate Alert Service
const att_svc_desc_t proxr_ias_svc         = ATT_SVC_IMMEDIATE_ALERT;

/// Alert Level Service - Heart Rate Measurement Characteristic
const struct att_char_desc proxr_ias_alert_lvl_char = ATT_CHAR(ATT_CHAR_PROP_WR_NO_RESP,
                                                                    0,
                                                                    ATT_CHAR_ALERT_LEVEL);

/// TX Power Service
const att_svc_desc_t proxr_txps_svc     = ATT_SVC_TX_POWER;

/// TX Power Service -TX Power Level Characteristic
const struct att_char_desc proxr_txps_tx_power_lvl_char = ATT_CHAR(ATT_CHAR_PROP_RD,
                                                                         0,
                                                                         ATT_CHAR_TX_POWER_LEVEL);

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

struct proxr_env_tag proxr_env __attribute__((section("retention_mem_area0"),zero_init)); //@RETENTION MEMORY

/// PROXR task descriptor
static const struct ke_task_desc TASK_DESC_PROXR = {proxr_state_handler, &proxr_default_handler, proxr_state, PROXR_STATE_MAX, PROXR_IDX_MAX};




/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

void proxr_init(void)
{
    // Reset Environment
    memset(&proxr_env, 0, sizeof(proxr_env));

    // Create PROXR task
    ke_task_create(TASK_PROXR, &TASK_DESC_PROXR);

    ke_state_set(TASK_PROXR, PROXR_DISABLED);
}

void proxr_disable(uint16_t conhdl) 
{
    att_size_t length;
    uint8_t *alert_lvl;

    // Disable LLS in database
    attmdb_svc_set_permission(proxr_env.lls_shdl, PERM_RIGHT_DISABLE);

    // If IAS is supported
    if (proxr_env.ias_shdl != ATT_INVALID_HANDLE)
    {
        // Disable IAS in database
        attmdb_svc_set_permission(proxr_env.ias_shdl, PERM_RIGHT_DISABLE);
    }

    // If TXPS is supported
    if (proxr_env.txps_shdl != ATT_INVALID_HANDLE)
    {
        // Disable TXPS in database
        attmdb_svc_set_permission(proxr_env.txps_shdl, PERM_RIGHT_DISABLE);
    }

    struct proxr_disable_ind *ind = KE_MSG_ALLOC(PROXR_DISABLE_IND,
                                                 proxr_env.con_info.appid, TASK_PROXR,
                                                 proxr_disable_ind);

    //Get Alert Level value stored in DB
    attmdb_att_get_value(proxr_env.lls_shdl + LLS_IDX_ALERT_LVL_VAL,
                         &length, &alert_lvl);

    // Fill in the parameter structure
    ind->conhdl            = conhdl;
    ind->lls_alert_lvl    = *alert_lvl;

    // Send the message
    ke_msg_send(ind);

    // Go to idle state
    ke_state_set(TASK_PROXR, PROXR_IDLE);
}

#endif //BLE_PROX_REPORTER

/// @} PROXR
