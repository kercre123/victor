/**
 ****************************************************************************************
 *
 * @file bass.c
 *
 * @brief Battery Server Implementation.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 * $Rev$
 *
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @addtogroup BASS
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"

#if (BLE_BAS_SERVER)

#include "gap.h"
#include "gattc_task.h"
#include "attm_util.h"
#include "bass.h"
#include "bass_task.h"
#include "gapc.h"
#include "attm_db.h"
/*
 * BAS ATTRIBUTES DEFINITION
 ****************************************************************************************
 */

/// Full BAS Database Description - Used to add attributes into the database
const struct attm_desc bas_att_db[BAS_IDX_NB] =
{
    // Battery Service Declaration
    [BAS_IDX_SVC]                  =   {ATT_DECL_PRIMARY_SERVICE, PERM(RD, ENABLE), sizeof(bas_svc),
                                        sizeof(bas_svc), (uint8_t *)&bas_svc},

    // Battery Level Characteristic Declaration
    [BAS_IDX_BATT_LVL_CHAR]        =   {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), sizeof(bas_batt_level_char),
                                        sizeof(bas_batt_level_char), (uint8_t *)&bas_batt_level_char},
    // Battery Level Characteristic Value
    [BAS_IDX_BATT_LVL_VAL]         =   {ATT_CHAR_BATTERY_LEVEL, PERM(RD, ENABLE), sizeof(uint8_t),
                                        0, NULL},
    // Battery Level Characteristic - Client Characteristic Configuration Descriptor
    [BAS_IDX_BATT_LVL_NTF_CFG]     =   {ATT_DESC_CLIENT_CHAR_CFG, PERM(RD, ENABLE)|PERM(WR, ENABLE), sizeof(uint16_t),
                                        0, NULL},
    // Battery Level Characteristic - Characteristic Presentation Format Descriptor
    [BAS_IDX_BATT_LVL_PRES_FMT]    =   {ATT_DESC_CHAR_PRES_FORMAT, PERM(RD, ENABLE), sizeof(struct prf_char_pres_fmt),
                                        0, NULL},
};

/// Battery Service
const att_svc_desc_t bas_svc = ATT_SVC_BATTERY_SERVICE;

/// Battery Level characteristic
const struct att_char_desc bas_batt_level_char = ATT_CHAR(ATT_CHAR_PROP_RD,
                                                            0,
                                                            ATT_CHAR_BATTERY_LEVEL);

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

struct bass_env_tag bass_env __attribute__((section("retention_mem_area0"),zero_init)); //@RETENTION MEMORY

/// BASS task descriptor
static const struct ke_task_desc TASK_DESC_BASS = {bass_state_handler, &bass_default_handler, bass_state, BASS_STATE_MAX, BASS_IDX_MAX};


/*
 * GLOBAL FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */

void bass_init(void)
{
    // Reset the find me target environment
    memset(&bass_env, 0, sizeof(bass_env));

    // Create BASS task
    ke_task_create(TASK_BASS, &TASK_DESC_BASS);

    // Go to IDLE state
    ke_state_set(TASK_BASS, BASS_DISABLED);
}

void bass_batt_level_upd_cfm_send(uint8_t status)
{
    struct bass_batt_level_upd_cfm * cfm = KE_MSG_ALLOC(BASS_BATT_LEVEL_UPD_CFM, bass_env.con_info.appid,
                                                        TASK_BASS, bass_batt_level_upd_cfm);

    cfm->conhdl = gapc_get_conhdl(bass_env.con_info.conidx);
    cfm->status = status;

    ke_msg_send(cfm);
}

void bass_disable(uint16_t conhdl) 
{
#ifndef USE_ONE_BAS_INSTANCE
    // Counter
    uint8_t i;
#endif    
    // Information get in the DB
    att_size_t att_length;
    uint8_t *att_value;

    // Send current configuration to the application
    struct bass_disable_ind *ind = KE_MSG_ALLOC(BASS_DISABLE_IND,
                                                bass_env.con_info.appid, TASK_BASS,
                                                bass_disable_ind);

    ind->conhdl = conhdl;

#ifndef USE_ONE_BAS_INSTANCE
    for (i = 0; i < bass_env.bas_nb; i++)
#else
    const int i = 0;
#endif    
    {
        if((bass_env.features[i] & BASS_FLAG_NTF_CFG_BIT)
                                 == BASS_FLAG_NTF_CFG_BIT)
        {
            ind->batt_level_ntf_cfg[i] = PRF_CLI_START_NTF;

            // Reset ntf cfg bit in features
            bass_env.features[i] &= ~BASS_FLAG_NTF_CFG_BIT;
        }
        else
        {
            ind->batt_level_ntf_cfg[i] = PRF_CLI_STOP_NTFIND;
        }

        // Get Battery Level value
        attmdb_att_get_value(bass_env.shdl[i] + BAS_IDX_BATT_LVL_VAL,
                             &att_length, &att_value);
        ind->batt_lvl[i] = *att_value;
    }

    ke_msg_send(ind);

    // Go to idle state
    ke_state_set(TASK_BASS, BASS_IDLE);
}

#endif // (BLE_BAS_SERVER)

/// @} BASS
