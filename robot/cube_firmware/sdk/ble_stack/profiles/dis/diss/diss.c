/**
 ****************************************************************************************
 *
 * @file diss.c
 *
 * @brief Device Information Service Server Implementation.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 *
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @addtogroup DISS
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "rwip_config.h"

#if (BLE_DIS_SERVER)
#include "attm_util.h"
#include "atts_util.h"
#include "diss.h"
#include "diss_task.h"
#include "prf_utils.h"

/*
 * MACROS
 ****************************************************************************************
 */

/*
 * DIS ATTRIBUTES
 ****************************************************************************************
 */

/// Full DIS Database Description - Used to add attributes into the database
const struct attm_desc diss_att_db[DIS_IDX_NB] =
{
    // Device Information Service Declaration
    [DIS_IDX_SVC]                           =   {ATT_DECL_PRIMARY_SERVICE, PERM(RD, ENABLE), sizeof(diss_dis_svc),
                                                 sizeof(diss_dis_svc), (uint8_t *)&diss_dis_svc},

    // Manufacturer Name Characteristic Declaration
    [DIS_IDX_MANUFACTURER_NAME_CHAR]        =   {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), sizeof(diss_manufacturer_name_char),
                                                 sizeof(diss_manufacturer_name_char), (uint8_t *)&diss_manufacturer_name_char},
    // Manufacturer Name Characteristic Value
    [DIS_IDX_MANUFACTURER_NAME_VAL]         =   {ATT_CHAR_MANUF_NAME, PERM(RD, ENABLE), DIS_VAL_MAX_LEN, 0, NULL},

    // Model Number String Characteristic Declaration
    [DIS_IDX_MODEL_NB_STR_CHAR]             =   {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), sizeof(diss_model_nb_str_char),
                                                 sizeof(diss_model_nb_str_char), (uint8_t *)&diss_model_nb_str_char},
    // Model Number String Characteristic Value
    [DIS_IDX_MODEL_NB_STR_VAL]              =   {ATT_CHAR_MODEL_NB, PERM(RD, ENABLE), DIS_VAL_MAX_LEN, 0, NULL},

    // Serial Number String Characteristic Declaration
    [DIS_IDX_SERIAL_NB_STR_CHAR]            =   {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), sizeof(diss_serial_nb_str_char),
                                                 sizeof(diss_serial_nb_str_char), (uint8_t *)&diss_serial_nb_str_char},
    // Serial Number String Characteristic Value
    [DIS_IDX_SERIAL_NB_STR_VAL]             =   {ATT_CHAR_SERIAL_NB, PERM(RD, ENABLE), DIS_VAL_MAX_LEN, 0, NULL},

    // Hardware Revision String Characteristic Declaration
    [DIS_IDX_HARD_REV_STR_CHAR]             =   {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), sizeof(diss_hard_rev_str_char),
                                                 sizeof(diss_hard_rev_str_char), (uint8_t *)&diss_hard_rev_str_char},
    // Hardware Revision String Characteristic Value
    [DIS_IDX_HARD_REV_STR_VAL]              =   {ATT_CHAR_HW_REV, PERM(RD, ENABLE), DIS_VAL_MAX_LEN, 0, NULL},

    // Firmware Revision String Characteristic Declaration
    [DIS_IDX_FIRM_REV_STR_CHAR]             =   {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), sizeof(diss_firm_rev_str_char),
                                                 sizeof(diss_firm_rev_str_char), (uint8_t *)&diss_firm_rev_str_char},
    // Firmware Revision String Characteristic Value
    [DIS_IDX_FIRM_REV_STR_VAL]              =   {ATT_CHAR_FW_REV, PERM(RD, ENABLE), DIS_VAL_MAX_LEN, 0, NULL},

    // Software Revision String Characteristic Declaration
    [DIS_IDX_SW_REV_STR_CHAR]               =   {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), sizeof(diss_sw_rev_str_char),
                                                 sizeof(diss_sw_rev_str_char), (uint8_t *)&diss_sw_rev_str_char},
    // Software Revision String Characteristic Value
    [DIS_IDX_SW_REV_STR_VAL]                =   {ATT_CHAR_SW_REV, PERM(RD, ENABLE), DIS_VAL_MAX_LEN, 0, NULL},

    // System ID Characteristic Declaration
    [DIS_IDX_SYSTEM_ID_CHAR]                =   {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), sizeof(diss_system_id_char),
                                                 sizeof(diss_system_id_char), (uint8_t *)&diss_system_id_char},
    // System ID Characteristic Value
    [DIS_IDX_SYSTEM_ID_VAL]                 =   {ATT_CHAR_SYS_ID, PERM(RD, ENABLE), DIS_SYS_ID_LEN, 0, NULL},

    // IEEE 11073-20601 Regulatory Certification Data List Characteristic Declaration
    [DIS_IDX_IEEE_CHAR]                     =   {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), sizeof(diss_ieee_char),
                                                 sizeof(diss_ieee_char), (uint8_t *)&diss_ieee_char},
    // IEEE 11073-20601 Regulatory Certification Data List Characteristic Value
    [DIS_IDX_IEEE_VAL]                      =   {ATT_CHAR_IEEE_CERTIF, PERM(RD, ENABLE), DIS_VAL_MAX_LEN, 0, NULL},

    // PnP ID Characteristic Declaration
    [DIS_IDX_PNP_ID_CHAR]                   =   {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), sizeof(diss_pnp_id_char),
                                                 sizeof(diss_pnp_id_char), (uint8_t *)&diss_pnp_id_char},
    // PnP ID Characteristic Value
    [DIS_IDX_PNP_ID_VAL]                    =   {ATT_CHAR_PNP_ID, PERM(RD, ENABLE), DIS_PNP_ID_LEN, 0, NULL},
};

const att_svc_desc_t diss_dis_svc = ATT_SVC_DEVICE_INFO;

const struct att_char_desc diss_manufacturer_name_char = ATT_CHAR(ATT_CHAR_PROP_RD, DIS_MANUFACTURER_NAME_CHAR,
                                                                                      ATT_CHAR_MANUF_NAME);
const struct att_char_desc diss_model_nb_str_char      = ATT_CHAR(ATT_CHAR_PROP_RD, DIS_MODEL_NB_STR_CHAR,
                                                                                      ATT_CHAR_MODEL_NB);
const struct att_char_desc diss_serial_nb_str_char     = ATT_CHAR(ATT_CHAR_PROP_RD, DIS_SERIAL_NB_STR_CHAR,
                                                                                      ATT_CHAR_SERIAL_NB);
const struct att_char_desc diss_hard_rev_str_char      = ATT_CHAR(ATT_CHAR_PROP_RD, DIS_HARD_REV_STR_CHAR,
                                                                                      ATT_CHAR_HW_REV);
const struct att_char_desc diss_firm_rev_str_char      = ATT_CHAR(ATT_CHAR_PROP_RD, DIS_FIRM_REV_STR_CHAR,
                                                                                      ATT_CHAR_FW_REV);
const struct att_char_desc diss_sw_rev_str_char        = ATT_CHAR(ATT_CHAR_PROP_RD, DIS_SW_REV_STR_CHAR,
                                                                                      ATT_CHAR_SW_REV);
const struct att_char_desc diss_system_id_char         = ATT_CHAR(ATT_CHAR_PROP_RD, DIS_SYSTEM_ID_CHAR,
                                                                                      ATT_CHAR_SYS_ID);
const struct att_char_desc diss_ieee_char              = ATT_CHAR(ATT_CHAR_PROP_RD, DIS_IEEE_CHAR,
                                                                                      ATT_CHAR_IEEE_CERTIF);
const struct att_char_desc diss_pnp_id_char            = ATT_CHAR(ATT_CHAR_PROP_RD, DIS_PNP_ID_CHAR,
                                                                                      ATT_CHAR_PNP_ID);

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

struct diss_env_tag diss_env __attribute__((section("retention_mem_area0"),zero_init)); //@RETENTION MEMORY

/// Device Information Service task descriptor
static const struct ke_task_desc TASK_DESC_DISS = {diss_state_handler, &diss_default_handler, diss_state, DISS_STATE_MAX, DISS_IDX_MAX};


/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

void diss_init(void)
{
    // Reset environment
    memset(&diss_env, 0, sizeof(diss_env));

    // Create DISS task
    ke_task_create(TASK_DISS, &TASK_DESC_DISS);

    // Set task in disabled state
    ke_state_set(TASK_DISS, DISS_DISABLED);
}

uint32_t diss_compute_cfg_flag(uint16_t features)
{
    //Service Declaration
    uint32_t cfg_flag = 1;

    for (uint8_t i = 0; i<DIS_CHAR_MAX; i++)
    {
        if (((features >> i) & 1) == 1)
        {
            cfg_flag |= (3 << (i*2 + 1));
        }
    }

    return cfg_flag;
}

uint8_t diss_check_val_len(uint8_t char_code, uint8_t val_len)
{
    uint8_t status = PRF_ERR_OK;

    // Check if length is upper than the general maximal length
    if (val_len > DIS_VAL_MAX_LEN)
    {
        status = PRF_ERR_UNEXPECTED_LEN;
    }
    else
    {
        // Check if length matches particular requirements
        switch (char_code)
        {
            case DIS_SYSTEM_ID_CHAR:
                if (val_len != DIS_SYS_ID_LEN)
                {
                    status = PRF_ERR_UNEXPECTED_LEN;
                }
                break;
            case DIS_IEEE_CHAR:
                if (val_len < DIS_IEEE_CERTIF_MIN_LEN)
                {
                    status = PRF_ERR_UNEXPECTED_LEN;
                }
                break;
            case DIS_PNP_ID_CHAR:
                if (val_len != DIS_PNP_ID_LEN)
                {
                    status = PRF_ERR_UNEXPECTED_LEN;
                }
                break;
            default:
                break;
        }
    }

    return status;
}

void diss_disable(uint16_t conhdl) 
{
    // Inform the application about the disconnection
    struct diss_disable_ind *ind = KE_MSG_ALLOC(DISS_DISABLE_IND,
                                                diss_env.con_info.appid, TASK_DISS,
                                                diss_disable_ind);

    ind->conhdl = conhdl;

    ke_msg_send(ind);

    //Disable DIS in database
    attmdb_svc_set_permission(diss_env.shdl, PERM(SVC, DISABLE));

    //Go to idle state
    ke_state_set(TASK_DISS, DISS_IDLE);
}

#endif //BLE_DIS_SERVER

/// @} DISS
