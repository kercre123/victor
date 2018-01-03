/**
 ****************************************************************************************
 *
 * @file htpt.c
 *
 * @brief Health Thermometer Profile Thermometer implementation.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 *
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @addtogroup HTPT
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "rwip_config.h"

#if (BLE_HT_THERMOM)
#include "attm_util.h"
#include "htpt.h"
#include "htpt_task.h"
#include "co_utils.h"
#include "prf_utils.h"

/*
 * MACROS
 ****************************************************************************************
 */
#define HTPT_IS_FEATURE_SUPPORTED(bit_mask) \
    (((htpt_env.features & bit_mask) == bit_mask))

#define HTPT_IS_CHAR_SUPPORTED(char_code) \
    (htpt_env.att_tbl[(char_code)] != 0x00)

#define HTPT_IS_INDNTF_ENABLED(bit_mask) \
    (((htpt_env.features & bit_mask) == bit_mask))

/*
 * HTPT PROFILE ATTRIBUTES
 ****************************************************************************************
 */
/// Full HTS Database Description - Used to add attributes into the database
const struct attm_desc htpt_att_db[HTS_IDX_NB] =
{
    // Health Thermometer Service Declaration
    [HTS_IDX_SVC]                      =   {ATT_DECL_PRIMARY_SERVICE, PERM(RD, ENABLE), sizeof(htpt_hlth_therm_svc),
                                         sizeof(htpt_hlth_therm_svc), (uint8_t *)&htpt_hlth_therm_svc},

    // Temperature Measurement Characteristic Declaration
    [HTS_IDX_TEMP_MEAS_CHAR]        =   {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), sizeof(htpt_temp_meas_char),
                                         sizeof(htpt_temp_meas_char), (uint8_t *)&htpt_temp_meas_char},
    // Temperature Measurement Characteristic Value
    [HTS_IDX_TEMP_MEAS_VAL]         =   {ATT_CHAR_TEMPERATURE_MEAS, PERM(IND, ENABLE), HTPT_TEMP_MEAS_MAX_LEN,
                                         0, NULL},
    // Temperature Measurement Characteristic - Client Characteristic Configuration Descriptor
    [HTS_IDX_TEMP_MEAS_IND_CFG]     =   {ATT_DESC_CLIENT_CHAR_CFG, PERM(RD, ENABLE)|PERM(WR, ENABLE), sizeof(uint16_t),
                                         0, NULL},

    // Temperature Type Characteristic Declaration
    [HTS_IDX_TEMP_TYPE_CHAR]        =   {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), sizeof(htpt_temp_type_char),
                                         sizeof(htpt_temp_type_char), (uint8_t *)&htpt_temp_type_char},
    // Temperature Type Characteristic Value
    [HTS_IDX_TEMP_TYPE_VAL]         =   {ATT_CHAR_TEMPERATURE_TYPE, PERM(RD, ENABLE), HTPT_TEMP_TYPE_MAX_LEN,
                                         0, NULL},

    // Intermediate Measurement Characteristic Declaration
    [HTS_IDX_INTERM_TEMP_CHAR]        =   {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), sizeof(htpt_interm_temp_char),
                                         sizeof(htpt_interm_temp_char), (uint8_t *)&htpt_interm_temp_char},
    // Intermediate Measurement Characteristic Value
    [HTS_IDX_INTERM_TEMP_VAL]         =   {ATT_CHAR_INTERMED_TEMPERATURE, PERM(NTF, ENABLE), HTPT_TEMP_MEAS_MAX_LEN,
                                         0, NULL},
    // Intermediate Measurement Characteristic - Client Characteristic Configuration Descriptor
    [HTS_IDX_INTERM_TEMP_CFG]         =   {ATT_DESC_CLIENT_CHAR_CFG, PERM(RD, ENABLE)|PERM(WR, ENABLE), sizeof(uint16_t),
                                         0, NULL},

    // Measurement Interval Characteristic Declaration
    [HTS_IDX_MEAS_INTV_CHAR]        =   {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), sizeof(htpt_meas_intv_char),
                                         sizeof(htpt_meas_intv_char), (uint8_t *)&htpt_meas_intv_char},
    // Measurement Interval Characteristic Value
    [HTS_IDX_MEAS_INTV_VAL]         =   {ATT_CHAR_MEAS_INTERVAL, PERM(RD, ENABLE), HTPT_MEAS_INTV_MAX_LEN,
                                         0, NULL},
    // Measurement Interval Characteristic - Client Characteristic Configuration Descriptor
    [HTS_IDX_MEAS_INTV_CFG]         =   {ATT_DESC_CLIENT_CHAR_CFG, PERM(RD, ENABLE)|PERM(WR, ENABLE), sizeof(uint16_t),
                                         0, NULL},
    // Measurement Interval Characteristic - Valid Range Descriptor
    [HTS_IDX_MEAS_INTV_VAL_RANGE]   =   {ATT_DESC_VALID_RANGE, PERM(RD, ENABLE), sizeof(uint32_t),
                                         0, NULL},
};

/// Health Thermometer Service
const att_svc_desc_t htpt_hlth_therm_svc = ATT_SVC_HEALTH_THERMOM;

/// Health Thermometer Service Temperature Measurement Characteristic
const struct att_char_desc htpt_temp_meas_char = ATT_CHAR(ATT_CHAR_PROP_IND, HTPT_TEMP_MEAS_CHAR,
                                                            ATT_CHAR_TEMPERATURE_MEAS);

/// Health Thermometer Service Temperature Type Characteristic
const struct att_char_desc htpt_temp_type_char = ATT_CHAR(ATT_CHAR_PROP_RD, HTPT_TEMP_TYPE_CHAR,
                                                            ATT_CHAR_TEMPERATURE_TYPE);

/// Health Thermometer Service Intermediate Temperature Characteristic
const struct att_char_desc htpt_interm_temp_char = ATT_CHAR(ATT_CHAR_PROP_NTF, HTPT_INTERM_TEMP_CHAR,
                                                              ATT_CHAR_INTERMED_TEMPERATURE);

/// Health Thermometer Service Measurement Interval Characteristic
const struct att_char_desc htpt_meas_intv_char = ATT_CHAR(ATT_CHAR_PROP_RD, HTPT_MEAS_INTV_CHAR,
                                                            ATT_CHAR_MEAS_INTERVAL);

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */
struct htpt_env_tag htpt_env __attribute__((section("retention_mem_area0"),zero_init)); //@RETENTION MEMORY

/// Health Thermometer task descriptor
static const struct ke_task_desc TASK_DESC_HTPT = {htpt_state_handler, &htpt_default_handler, htpt_state, HTPT_STATE_MAX, HTPT_IDX_MAX};



/*
 * FUNCTION DEFINITIONSHTPT_IS_CHAR_SUPPORTED
 ****************************************************************************************
 */

void htpt_init(void)
{
    // Reset Environment
    memset(&htpt_env, 0, sizeof(htpt_env));

    // Create HTPT task
    ke_task_create(TASK_HTPT, &TASK_DESC_HTPT);

    // Set task in disabled state
    ke_state_set(TASK_HTPT, HTPT_DISABLED);
}

uint8_t htpt_get_valid_rge_offset(void)
{
    uint8_t offset = 0;

    if (HTPT_IS_FEATURE_SUPPORTED(HTPT_MEAS_INTV_WR_SUP))
    {
        offset += 1;

        if (HTPT_IS_FEATURE_SUPPORTED(HTPT_MEAS_INTV_IND_SUP))
        {
            offset += 1;
        }
    }

    return offset;
}

uint8_t htpt_pack_temp_value(uint8_t *packed_temp, struct htp_temp_meas temp_meas)
{
    uint8_t cursor = 0;

    *(packed_temp + cursor) = temp_meas.flags;
    cursor += 1;

    co_write32p(packed_temp + cursor, temp_meas.temp);
    cursor += 4;

    //Time Flag Set
    if ((temp_meas.flags & HTPT_FLAG_TIME) == HTPT_FLAG_TIME)
    {
        cursor += prf_pack_date_time(packed_temp + cursor, &(temp_meas.time_stamp));
    }

    //Type flag set
    if ((temp_meas.flags & HTPT_FLAG_TYPE) == HTPT_FLAG_TYPE)
    {
        *(packed_temp + cursor) = temp_meas.type;
        cursor += 1;
    }

    //Clear unused packet data
    if(cursor < HTPT_TEMP_MEAS_MAX_LEN)
    {
        memset(packed_temp + cursor, 0, (HTPT_TEMP_MEAS_MAX_LEN - cursor));
    }

    return cursor;
}

void htpt_temp_send_cfm_send(uint8_t status, uint8_t cfm_type)
{
    struct htpt_temp_send_cfm *cfm = KE_MSG_ALLOC(HTPT_TEMP_SEND_CFM, htpt_env.con_info.appid,
                                                  TASK_HTPT, htpt_temp_send_cfm);

    cfm->conhdl     = gapc_get_conhdl(htpt_env.con_info.conidx);
    cfm->status     = status;
    cfm->cfm_type   = HTPT_CENTRAL_IND_CFM;

    ke_msg_send(cfm);
}

void htpt_disable(uint16_t conhdl) 
{
    att_size_t att_length;
    uint8_t *att_value;

    //Disable HTS in database
    attmdb_svc_set_permission(htpt_env.shdl, PERM_RIGHT_DISABLE);

    //Send current configuration to APP
    struct htpt_disable_ind* ind = KE_MSG_ALLOC(HTPT_DISABLE_IND,
                                                htpt_env.con_info.appid, TASK_HTPT,
                                                htpt_disable_ind);

    ind->conhdl = conhdl;

    //Temperature Measurement Char. - Indications Configuration
    if (HTPT_IS_INDNTF_ENABLED(HTPT_MASK_TEMP_MEAS_CFG))
    {
        ind->temp_meas_ind_en = PRF_CLI_START_IND;
    }

    //Intermediate Measurement Char. - Indications Configuration
    if (HTPT_IS_CHAR_SUPPORTED(HTPT_INTERM_TEMP_CHAR))
    {
        if (HTPT_IS_INDNTF_ENABLED(HTPT_MASK_INTM_MEAS_CFG))
        {
            ind->interm_temp_ntf_en = PRF_CLI_START_NTF;
        }
    }

    //Measurement Interval Char. - Value
    if (HTPT_IS_CHAR_SUPPORTED(HTPT_MEAS_INTV_CHAR))
    {
        //Measurement Interval Char. - Indications Configuration
        if (HTPT_IS_FEATURE_SUPPORTED(HTPT_MEAS_INTV_IND_SUP))
        {
            if (HTPT_IS_INDNTF_ENABLED(HTPT_MASK_MEAS_INTV_CFG))
            {
                ind->meas_intv_ind_en = PRF_CLI_START_IND;
            }
        }

        attmdb_att_get_value(htpt_env.shdl + htpt_env.att_tbl[HTPT_MEAS_INTV_CHAR] + 1,
                             &att_length, &att_value);
        memcpy(&(ind->meas_intv), att_value, sizeof(uint16_t));
    }

    ke_msg_send(ind);

    //Reset indications/notifications bit field
    htpt_env.features &= ~HTPT_CFG_NTFIND_MASK;

    //Go to idle state
    ke_state_set(TASK_HTPT, HTPT_IDLE);
}

uint16_t htpt_compute_att_table(uint8_t features)
{
    //Temperature Measurement Characteristic is mandatory
    uint16_t att_table = HTPT_TEMP_MEAS_MASK;

    //Check if Temperature Type Char. is supported
    if (HTPT_IS_FEATURE_SUPPORTED(HTPT_TEMP_TYPE_CHAR_SUP))
    {
        att_table |= HTPT_TEMP_TYPE_MASK;
    }

    //Check if Intermediate Temperature Char. is supported
    if (HTPT_IS_FEATURE_SUPPORTED(HTPT_INTERM_TEMP_CHAR_SUP))
    {
        att_table |= HTPT_INTM_TEMP_MASK;
    }

    //Check if Measurement Interval Char. is supported
    if (HTPT_IS_FEATURE_SUPPORTED(HTPT_MEAS_INTV_CHAR_SUP))
    {
        att_table |= HTPT_MEAS_INTV_MASK;

        //Check if Measurement Interval Char. supports indications
        if (HTPT_IS_FEATURE_SUPPORTED(HTPT_MEAS_INTV_IND_SUP))
        {
            att_table |= HTPT_MEAS_INTV_CCC_MASK;
        }

        //Check if Measurement Interval Char. is writable
        if (HTPT_IS_FEATURE_SUPPORTED(HTPT_MEAS_INTV_WR_SUP))
        {
            att_table |= HTPT_MEAS_INTV_VALID_RGE_MASK;
        }
    }

    return att_table;
}

#endif //BLE_HT_THERMOM

/// @} HTPT
