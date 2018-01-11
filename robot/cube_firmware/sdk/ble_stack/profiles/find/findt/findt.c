/**
 ****************************************************************************************
 *
 * @file findt.c
 *
 * @brief Find Me Target implementation.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 *
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @addtogroup FINDT
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "rwip_config.h"

#if (BLE_FINDME_TARGET)
#include "findt.h"
#include "findt_task.h"
#include "prf_utils.h"

/*
 * FINDT PROFILE ATTRIBUTES
 ****************************************************************************************
 */
/// Full IAS Database Description - Used to add attributes into the database
const struct attm_desc findt_att_db[FINDT_IAS_IDX_NB] =
{
    // Immediate Alert Service Declaration
    [FINDT_IAS_IDX_SVC]              =   {ATT_DECL_PRIMARY_SERVICE, PERM(RD, ENABLE), sizeof(findt_ias_svc),
                                         sizeof(findt_ias_svc), (uint8_t *)&findt_ias_svc},

    // Alert Level Characteristic Declaration
    [FINDT_IAS_IDX_ALERT_LVL_CHAR]  =   {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), sizeof(findt_alert_lvl_char),
                                         sizeof(findt_alert_lvl_char), (uint8_t *)&findt_alert_lvl_char},
    // Alert Level Characteristic Value
    [FINDT_IAS_IDX_ALERT_LVL_VAL]   =   {ATT_CHAR_ALERT_LEVEL, PERM(WR, ENABLE), sizeof(uint8_t),
                                         0, NULL},
};

/*
 *  FIND ME PROFILE TARGET ATTRIBUTES
 ****************************************************************************************
 */
/// Immediate Alert Service
const att_svc_desc_t findt_ias_svc = ATT_SVC_IMMEDIATE_ALERT;

/// Alert Level Service - Heart Rate Measurement Characteristic
const struct att_char_desc findt_alert_lvl_char = ATT_CHAR(ATT_CHAR_PROP_WR_NO_RESP,
                                                               0,
                                                             ATT_CHAR_ALERT_LEVEL);

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */
struct findt_env_tag findt_env __attribute__((section("retention_mem_area0"),zero_init)); //@RETENTION MEMORY

/// FIND ME task descriptor
static const struct ke_task_desc TASK_DESC_FINDT = {findt_state_handler, &findt_default_handler, findt_state, FINDT_STATE_MAX, FINDT_IDX_MAX};


/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */
void findt_init(void)
{
    // Reset environment
    memset(&findt_env, 0, sizeof(findt_env));

    // Create FINDT task
    ke_task_create(TASK_FINDT, &TASK_DESC_FINDT);

    // Go to IDLE state
    ke_state_set(TASK_FINDT, FINDT_DISABLED);
}

void findt_disable(uint16_t conhdl) 
{
    // Inform the application about the disconnection
    struct findt_disable_ind *ind = KE_MSG_ALLOC(FINDT_DISABLE_IND,
                                                 findt_env.con_info.appid, TASK_FINDT,
                                                 findt_disable_ind);

    ind->conhdl = conhdl;

    ke_msg_send(ind);

    //Disable IAS in database
    attmdb_svc_set_permission(findt_env.shdl, PERM_RIGHT_DISABLE);

    //Go to idle state
    ke_state_set(TASK_FINDT, FINDT_IDLE);
}


#endif //BLE_FINDME_TARGET

/// @} FINDT
