/**
 ****************************************************************************************
 *
 * @file hrps.c
 *
 * @brief Heart Rate Profile Sensor implementation.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 * $Rev$
 *
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @addtogroup HRPS
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "rwip_config.h"

#if (BLE_HR_SENSOR)
#include "gap.h"
#include "gattc_task.h"
#include "attm_util.h"
#include "smpc_task.h"
#include "hrps.h"
#include "hrps_task.h"
#include "prf_utils.h"

/*
 * HTPT PROFILE ATTRIBUTES
 ****************************************************************************************
 */

/// Full HRS Database Description - Used to add attributes into the database
const struct attm_desc hrps_att_db[HRS_IDX_NB] =
{
    // Heart Rate Service Declaration
    [HRS_IDX_SVC]                      =   {ATT_DECL_PRIMARY_SERVICE, PERM(RD, ENABLE), sizeof(hrps_hrs_svc),
                                         sizeof(hrps_hrs_svc), (uint8_t *)&hrps_hrs_svc},

    // Heart Rate Measurement Characteristic Declaration
    [HRS_IDX_HR_MEAS_CHAR]            =   {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), sizeof(hrps_hr_meas_char),
                                         sizeof(hrps_hr_meas_char), (uint8_t *)&hrps_hr_meas_char},
    // Heart Rate Measurement Characteristic Value
    [HRS_IDX_HR_MEAS_VAL]             =   {ATT_CHAR_HEART_RATE_MEAS, PERM(NTF, ENABLE), HRPS_HT_MEAS_MAX_LEN,
                                         0, NULL},
    // Heart Rate Measurement Characteristic - Client Characteristic Configuration Descriptor
    [HRS_IDX_HR_MEAS_NTF_CFG]         =   {ATT_DESC_CLIENT_CHAR_CFG, PERM(RD, ENABLE)|PERM(WR, ENABLE), sizeof(uint16_t),
                                         0, NULL},

    // Body Sensor Location Characteristic Declaration
    [HRS_IDX_BOBY_SENSOR_LOC_CHAR]  =   {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), sizeof(hrps_body_sensor_loc_char),
                                         sizeof(hrps_body_sensor_loc_char), (uint8_t *)&hrps_body_sensor_loc_char},
    // Body Sensor Location Characteristic Value
    [HRS_IDX_BOBY_SENSOR_LOC_VAL]   =   {ATT_CHAR_BODY_SENSOR_LOCATION, PERM(RD, ENABLE), sizeof(uint8_t),
                                         0, NULL},

    // Heart Rate Control Point Characteristic Declaration
    [HRS_IDX_HR_CTNL_PT_CHAR]        =   {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), sizeof(hrps_hr_cntl_point_char),
                                         sizeof(hrps_hr_cntl_point_char), (uint8_t *)&hrps_hr_cntl_point_char},
    // Heart Rate Control Point Characteristic Value
    [HRS_IDX_HR_CTNL_PT_VAL]         =   {ATT_CHAR_HEART_RATE_CNTL_POINT, PERM(WR, ENABLE), sizeof(uint8_t),
                                         0, NULL},
};

/*
 *  Heart Rate PROFILE ATTRIBUTES
 ****************************************************************************************
 */

/// Heart Rate Sensor Service
const att_svc_desc_t hrps_hrs_svc = ATT_SVC_HEART_RATE;

/// Heart Rate Sensor Service - Heart Rate Measurement Characteristic
const struct att_char_desc hrps_hr_meas_char = ATT_CHAR(ATT_CHAR_PROP_NTF,
                                                          0,
                                                          ATT_CHAR_HEART_RATE_MEAS);

/// Heart Rate Sensor Service -Body Sensor Location characteristic
const struct att_char_desc hrps_body_sensor_loc_char = ATT_CHAR(ATT_CHAR_PROP_RD,
                                                                  0,
                                                                  ATT_CHAR_BODY_SENSOR_LOCATION);

/// Heart Rate Sensor Service - Heart Rate Control Point characteristic
const struct att_char_desc hrps_hr_cntl_point_char = ATT_CHAR(ATT_CHAR_PROP_WR,
                                                                0,
                                                                ATT_CHAR_HEART_RATE_CNTL_POINT);

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

struct hrps_env_tag hrps_env __attribute__((section("retention_mem_area0"),zero_init)); //@RETENTION MEMORY

/// Heart Rate Task
static const struct ke_task_desc TASK_DESC_HRPS = {hrps_state_handler, &hrps_default_handler, hrps_state, HRPS_STATE_MAX, HRPS_IDX_MAX};


/*
 * EXPORTED FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */

void hrps_init(void)
{
    // Reset environment
    memset(&hrps_env, 0, sizeof(hrps_env));

    // Create HRPS task
    ke_task_create(TASK_HRPS, &TASK_DESC_HRPS);

    // Go to IDLE state
    ke_state_set(TASK_HRPS, HRPS_DISABLED);
}

void hrps_meas_send_cfm_send(uint8_t status)
{
    // Send CFM to APP that value has been sent or not
    struct hrps_meas_send_cfm * cfm = KE_MSG_ALLOC(HRPS_MEAS_SEND_CFM, hrps_env.con_info.appid,
                                                   TASK_HRPS, hrps_meas_send_cfm);

    cfm->conhdl = gapc_get_conhdl(hrps_env.con_info.conidx);
    cfm->status = status;

    ke_msg_send(cfm);
}

uint8_t hrps_pack_meas_value(uint8_t *packed_hr, const struct hrs_hr_meas* pmeas_val)
{
    uint8_t cursor = 0;
    uint8_t i = 0;

    // Heart Rate measurement flags
    *(packed_hr) = pmeas_val->flags;

    if ((pmeas_val->flags & HRS_FLAG_HR_16BITS_VALUE) == HRS_FLAG_HR_16BITS_VALUE)
    {
        // Heart Rate Measurement Value 16 bits
        co_write16p(packed_hr + 1, pmeas_val->heart_rate);
        cursor += 3;
    }
    else
    {
        // Heart Rate Measurement Value 8 bits
        *(packed_hr + 1) = pmeas_val->heart_rate;
        cursor += 2;
    }

    if ((pmeas_val->flags & HRS_FLAG_ENERGY_EXPENDED_PRESENT) == HRS_FLAG_ENERGY_EXPENDED_PRESENT)
    {
        // Energy Expended present
        co_write16p(packed_hr + cursor, pmeas_val->energy_expended);
        cursor += 2;
    }

    if ((pmeas_val->flags & HRS_FLAG_RR_INTERVAL_PRESENT) == HRS_FLAG_RR_INTERVAL_PRESENT)
    {
        for(i = 0 ; (i < (pmeas_val->nb_rr_interval)) && (i < (HRS_MAX_RR_INTERVAL)) ; i++)
        {
            // RR-Intervals
            co_write16p(packed_hr + cursor, pmeas_val->rr_intervals[i]);
            cursor += 2;
        }
    }

    // Clear unused packet data
    if(cursor < HRPS_HT_MEAS_MAX_LEN)
    {
        memset(packed_hr + cursor, 0, HRPS_HT_MEAS_MAX_LEN - cursor);
    }

    return cursor;
}

void hrps_disable(uint16_t conhdl) 
{
    //Disable HRS in database
    attmdb_svc_set_permission(hrps_env.shdl, PERM_RIGHT_DISABLE);

    //Send current configuration to APP
    struct hrps_disable_ind *ind = KE_MSG_ALLOC(HRPS_DISABLE_IND,
                                                hrps_env.con_info.appid, TASK_HRPS,
                                                hrps_disable_ind);

    ind->conhdl = conhdl;

    //Heart Rate Measurement Char. - Notifications Configuration
    if (HRPS_IS_SUPPORTED(HRPS_HR_MEAS_NTF_CFG))
    {
        ind->hr_meas_ntf_en = PRF_CLI_START_NTF;

        //Reset notifications bit field
        hrps_env.features &= ~HRPS_HR_MEAS_NTF_CFG;
    }

    ke_msg_send(ind);

    //Go to idle state
    ke_state_set(TASK_HRPS, HRPS_IDLE);
}

#endif /* BLE_HR_SENSOR */

/// @} HRPS
