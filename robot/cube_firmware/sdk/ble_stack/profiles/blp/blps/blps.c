/**
 ****************************************************************************************
 *
 * @file blps.c
 *
 * @brief Blood Pressure Profile Sensor implementation.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 * $Rev$
 *
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @addtogroup BLPS
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"

#if (BLE_BP_SENSOR)
#include "gap.h"
#include "gattc_task.h"
#include "attm_util.h"
#include "blps.h"
#include "blps_task.h"
#include "prf_utils.h"

/*
 * BLPS PROFILE ATTRIBUTES
 ****************************************************************************************
 */

/// Full BLPS Database Description - Used to add attributes into the database
const struct attm_desc blps_att_db[BPS_IDX_NB] =
{
    // Blood Pressure Service Declaration
    [BPS_IDX_SVC]                          =   {ATT_DECL_PRIMARY_SERVICE, PERM(RD, ENABLE), sizeof(blps_bps_svc),
                                              sizeof(blps_bps_svc), (uint8_t *)&blps_bps_svc},

    // Blood Pressure Measurement Characteristic Declaration
    [BPS_IDX_BP_MEAS_CHAR]                =   {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), sizeof(blps_bp_meas_char),
                                              sizeof(blps_bp_meas_char), (uint8_t *)&blps_bp_meas_char},
    // Blood Pressure Measurement Characteristic Value
    [BPS_IDX_BP_MEAS_VAL]                 =   {ATT_CHAR_BLOOD_PRESSURE_MEAS, PERM(IND, ENABLE), BLPS_BP_MEAS_MAX_LEN,
                                              0, NULL},
    // Blood Pressure Measurement Characteristic - Client Characteristic Configuration Descriptor
    [BPS_IDX_BP_MEAS_IND_CFG]             =   {ATT_DESC_CLIENT_CHAR_CFG, PERM(RD, ENABLE)|PERM(WR, ENABLE), sizeof(uint16_t),
                                              0, NULL},

    // Blood Pressure Feature Characteristic Declaration
    [BPS_IDX_BP_FEATURE_CHAR]            =   {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), sizeof(blps_bp_feature_char),
                                              sizeof(blps_bp_feature_char), (uint8_t *)&blps_bp_feature_char},
    // Blood Pressure Feature Characteristic Value
    [BPS_IDX_BP_FEATURE_VAL]             =   {ATT_CHAR_BLOOD_PRESSURE_FEATURE, PERM(RD, ENABLE), sizeof(uint16_t),
                                              0, NULL},

    // Intermediate Cuff Pressure Characteristic Declaration
    [BPS_IDX_INTM_CUFF_PRESS_CHAR]        =   {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), sizeof(blps_interm_cuff_press_char),
                                              sizeof(blps_interm_cuff_press_char), (uint8_t *)&blps_interm_cuff_press_char},
    // Intermediate Cuff Pressure Characteristic Value
    [BPS_IDX_INTM_CUFF_PRESS_VAL]         =   {ATT_CHAR_INTERMEDIATE_CUFF_PRESSURE, PERM(NTF, ENABLE), BLPS_BP_MEAS_MAX_LEN,
                                              0, NULL},
    // Intermediate Cuff Pressure Characteristic - Client Characteristic Configuration Descriptor
    [BPS_IDX_INTM_CUFF_PRESS_NTF_CFG]   =   {ATT_DESC_CLIENT_CHAR_CFG, PERM(RD, ENABLE)|PERM(WR, ENABLE), sizeof(uint16_t),
                                              0, NULL},
};

/*
 *  BLOOD PRESSURE PROFILE ATTRIBUTES
 ****************************************************************************************
 */
/// Blood Pressure Sensor Service
const att_svc_desc_t blps_bps_svc = ATT_SVC_BLOOD_PRESSURE;

/// Blood Pressure Sensor Service - Blood Pressure Measurement Characteristic
const struct att_char_desc blps_bp_meas_char = ATT_CHAR(ATT_CHAR_PROP_IND,
                                                          0, //No attribute table for BPS
                                                          ATT_CHAR_BLOOD_PRESSURE_MEAS);

/// Blood Pressure Sensor Service - Intermediate Cuff Pressure Characteristic
const struct att_char_desc blps_interm_cuff_press_char = ATT_CHAR(ATT_CHAR_PROP_NTF,
                                                                    0, //No attribute table for BPS
                                                                    ATT_CHAR_INTERMEDIATE_CUFF_PRESSURE);

/// Blood Pressure Sensor Service - Blood Pressure Feature Characteristic
const struct att_char_desc blps_bp_feature_char = ATT_CHAR(ATT_CHAR_PROP_RD,
                                                             0, //No attribute table for BPS
                                                             ATT_CHAR_BLOOD_PRESSURE_FEATURE);

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

struct blps_env_tag blps_env __attribute__((section("retention_mem_area0"),zero_init)); //@RETENTION MEMORY

/// Blood Pressure task descriptor
static const struct ke_task_desc TASK_DESC_BLPS = {blps_state_handler, &blps_default_handler, blps_state, BLPS_STATE_MAX, BLPS_IDX_MAX};


/*
 * EXPORTED FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */

void blps_init(void)
{
    // Reset environment
    memset(&blps_env, 0, sizeof(blps_env));

    // Create BLPS task
    ke_task_create(TASK_BLPS, &TASK_DESC_BLPS);

    // Go to IDLE state
    ke_state_set(TASK_BLPS, BLPS_DISABLED);
}

uint8_t blps_pack_meas_value(uint8_t *packed_bp, const struct bps_bp_meas* pmeas_val)
{
    uint8_t cursor;

    *(packed_bp) = pmeas_val->flags;

    // Blood Pressure Measurement Compound Value - Systolic
    co_write16p(packed_bp + 1, pmeas_val->systolic);

    // Blood Pressure Measurement Compound Value - Diastolic (mmHg)
    co_write16p(packed_bp + 3, pmeas_val->diastolic);

    //  Blood Pressure Measurement Compound Value - Mean Arterial Pressure (mmHg)
    co_write16p(packed_bp + 5, pmeas_val->mean_arterial_pressure);

    cursor = 7;

    // time flag set
    if ((pmeas_val->flags & BPS_FLAG_TIME_STAMP_PRESENT) == BPS_FLAG_TIME_STAMP_PRESENT)
    {
        cursor += prf_pack_date_time(packed_bp + cursor, &(pmeas_val->time_stamp));
    }

    // Pulse rate flag set
    if ((pmeas_val->flags & BPS_FLAG_PULSE_RATE_PRESENT) == BPS_FLAG_PULSE_RATE_PRESENT)
    {
        co_write16p(packed_bp + cursor, pmeas_val->pulse_rate);
        cursor += 2;
    }

    // User ID flag set
    if ((pmeas_val->flags & BPS_FLAG_USER_ID_PRESENT) == BPS_FLAG_USER_ID_PRESENT)
    {
        *(packed_bp + cursor) = pmeas_val->user_id;
        cursor += 1;
    }

    // Measurement status flag set
    if ((pmeas_val->flags & BPS_FLAG_MEAS_STATUS_PRESENT) == BPS_FLAG_MEAS_STATUS_PRESENT)
    {
        co_write16p(packed_bp + cursor, pmeas_val->meas_status);
        cursor += 2;
    }

    // clear unused packet data
    if(cursor < BLPS_BP_MEAS_MAX_LEN)
    {
        memset(packed_bp + cursor, 0, BLPS_BP_MEAS_MAX_LEN - cursor);
    }

    return cursor;
}

void blps_meas_send_cfm_send(uint8_t status)
{
    struct blps_meas_send_cfm * cfm = KE_MSG_ALLOC(BLPS_MEAS_SEND_CFM, blps_env.con_info.appid,
                                                   TASK_BLPS, blps_meas_send_cfm);

    cfm->conhdl     = gapc_get_conhdl(blps_env.con_info.conidx);
    cfm->status     = status;

    ke_msg_send(cfm);
}

void blps_disable(uint16_t conhdl) 
{
    //Disable BPS in database
    attmdb_svc_set_permission(blps_env.shdl, PERM_RIGHT_DISABLE);

    //Send current configuration to APP
    struct blps_disable_ind *ind = KE_MSG_ALLOC(BLPS_DISABLE_IND,
                                                blps_env.con_info.appid, TASK_BLPS,
                                                blps_disable_ind);

    ind->conhdl = conhdl;

    //Blood Pressure Measurement Char. - Indications Configuration
    if (BLPS_IS_ENABLED(BLPS_BP_MEAS_IND_CFG))
    {
        ind->bp_meas_ind_en = PRF_CLI_START_IND;
    }

    //Intermediate Cuff Pressure Char. - Notifications Configuration
    if (BLPS_IS_SUPPORTED(BLPS_INTM_CUFF_PRESS_SUP))
    {
        if (BLPS_IS_ENABLED(BLPS_INTM_CUFF_PRESS_NTF_CFG))
        {
            ind->interm_cp_ntf_en = PRF_CLI_START_NTF;
        }
    }

    ke_msg_send(ind);

    //Reset indications/notifications bit field
    blps_env.evt_cfg &= ~BLPS_NTFIND_MASK;

    //Go to idle state
    ke_state_set(TASK_BLPS, BLPS_IDLE);
}

#endif /* BLE_BP_SENSOR */

/// @} BLPS
