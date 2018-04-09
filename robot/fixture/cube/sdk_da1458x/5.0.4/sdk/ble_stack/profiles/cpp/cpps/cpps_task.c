/**
 ****************************************************************************************
 *
 * @file cpps_task.c
 *
 * @brief Cycling Power Profile Sensor Task Implementation.
 *
 * Copyright (C) RivieraWaves 2009-2014
 *
 * $ Rev $
 *
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @addtogroup CPPSTASK
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "rwble_config.h" 

#if (BLE_CP_SENSOR)
#include "cpp_common.h"
#include "gapc.h"
#include "gattc.h"
#include "gattc_task.h"
#include "atts_util.h"
#include "attm_util.h"
#include "cpps.h"
#include "cpps_task.h"
#include "prf_utils.h"
#include <math.h>

/*
 *  CYCLING POWER SERVICE ATTRIBUTES
 ****************************************************************************************
 */

/// Cycling Power Sensor Service
static const att_svc_desc_t cpps_cps_svc               = ATT_SVC_CYCLING_POWER;

/// CP Measurement characteristic value
static const struct att_char_desc cpps_cp_meas_char    = ATT_CHAR(ATT_CHAR_PROP_NTF,
                                                                CPP_CPS_MEAS_CHAR,
                                                                ATT_CHAR_CP_MEAS);
/// CP Feature characteristic value
static const struct att_char_desc cpps_cp_feat_char    = ATT_CHAR(ATT_CHAR_PROP_RD,
                                                                CPP_CPS_FEAT_CHAR,
                                                                ATT_CHAR_CP_FEAT);
/// Sensor Location characteristic value
static const struct att_char_desc cpps_sensor_loc_char = ATT_CHAR(ATT_CHAR_PROP_RD,
                                                                CPP_CPS_SENSOR_LOC_CHAR,
                                                                ATT_CHAR_SENSOR_LOC);
/// CP Vector
static const struct att_char_desc cpps_vector_char     = ATT_CHAR(ATT_CHAR_PROP_NTF,
                                                                CPP_CPS_VECTOR_CHAR,
                                                                ATT_CHAR_CP_VECTOR);
/// Control Point characteristic value
static const struct att_char_desc cpps_ctnl_pt_char    = ATT_CHAR(ATT_CHAR_PROP_WR | ATT_CHAR_PROP_IND,
                                                                CPP_CPS_CTNL_PT_CHAR,
                                                                ATT_CHAR_CP_CNTL_PT);

/// Full CPPS Database Description - Used to add attributes into the database
static const struct attm_desc cpps_att_db[CPS_IDX_NB] =
{
    /// Cycling Power Service Declaration
    [CPS_IDX_SVC]                    =   {ATT_DECL_PRIMARY_SERVICE, PERM(RD, ENABLE), sizeof(cpps_cps_svc),
                                            sizeof(cpps_cps_svc), (uint8_t *)&cpps_cps_svc},

    /// CP Measurement Characteristic Declaration
    [CPS_IDX_CP_MEAS_CHAR]           =  {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), sizeof(cpps_cp_meas_char),
                                            sizeof(cpps_cp_meas_char), (uint8_t *)&cpps_cp_meas_char},
    /// CP Measurement Characteristic Value
    [CPS_IDX_CP_MEAS_VAL]            =   {ATT_CHAR_CP_MEAS, PERM(NTF, ENABLE), CPP_CP_MEAS_MAX_LEN,
                                            0, NULL},
    /// CP Measurement Characteristic - Client Characteristic Configuration Descriptor
    [CPS_IDX_CP_MEAS_NTF_CFG]        =   {ATT_DESC_CLIENT_CHAR_CFG, PERM(RD, ENABLE) | PERM(WR, ENABLE), sizeof(uint16_t),
                                            0, NULL},
    /// CP Measurement Characteristic - Server Characteristic Configuration Descriptor
    [CPS_IDX_CP_MEAS_BCST_CFG]       =   {ATT_DESC_SERVER_CHAR_CFG, PERM(RD, ENABLE) | PERM(WR, ENABLE), sizeof(uint16_t),
                                            0, NULL},

    /// CP Feature Characteristic Declaration
    [CPS_IDX_CP_FEAT_CHAR]           =   {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), sizeof(cpps_cp_feat_char),
                                            sizeof(cpps_cp_feat_char), (uint8_t *)&cpps_cp_feat_char},
    /// CP Feature Characteristic Value
    [CPS_IDX_CP_FEAT_VAL]            =   {ATT_CHAR_CP_FEAT, PERM(RD, ENABLE), sizeof(uint32_t),
                                            0, NULL},

    /// Sensor Location Characteristic Declaration
    [CPS_IDX_SENSOR_LOC_CHAR]        =   {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), sizeof(cpps_sensor_loc_char),
                                            sizeof(cpps_sensor_loc_char), (uint8_t *)&cpps_sensor_loc_char},
    /// Sensor Location Characteristic Value
    [CPS_IDX_SENSOR_LOC_VAL]         =   {ATT_CHAR_SENSOR_LOC, PERM(RD, ENABLE), sizeof(uint8_t),
                                            0, NULL},

    /// CP Vector Characteristic Declaration
    [CPS_IDX_VECTOR_CHAR]            =   {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), sizeof(cpps_vector_char),
                                            sizeof(cpps_vector_char), (uint8_t *)&cpps_vector_char},
    /// CP Vector Characteristic Value
    [CPS_IDX_VECTOR_VAL]             =   {ATT_CHAR_CP_VECTOR, PERM(NTF, ENABLE), CPP_CP_VECTOR_MAX_LEN,
                                            0, NULL},
    /// CP Vector Characteristic - Client Characteristic Configuration Descriptor
    [CPS_IDX_VECTOR_NTF_CFG]         =   {ATT_DESC_CLIENT_CHAR_CFG, PERM(RD, ENABLE) | PERM(WR, ENABLE), sizeof(uint16_t),
                                            0, NULL},

    /// CP Control Point Characteristic Declaration
    [CPS_IDX_CTNL_PT_CHAR]           =   {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), sizeof(cpps_ctnl_pt_char),
                                            sizeof(cpps_ctnl_pt_char), (uint8_t *)&cpps_ctnl_pt_char},
    /// CP Control Point Characteristic Value - The response has the maximal length
    [CPS_IDX_CTNL_PT_VAL]            =   {ATT_CHAR_CP_CNTL_PT, PERM(WR, ENABLE) | PERM(IND, ENABLE), CPP_CP_CNTL_PT_RSP_MAX_LEN,
                                            0, NULL},
    /// CP Control Point Characteristic - Client Characteristic Configuration Descriptor
    [CPS_IDX_CTNL_PT_IND_CFG]        =   {ATT_DESC_CLIENT_CHAR_CFG, PERM(RD, ENABLE) | PERM(WR, ENABLE), sizeof(uint16_t),
                                            0, NULL},
};

/*
 * LOCAL FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */
/**
 ****************************************************************************************
 * @brief Packs measurement notifications
 * @param[in] param Pointer to the parameters of the message.
 * @param[out] pckd_meas pointer to packed message
 * @return status of the operation
 ****************************************************************************************
 */
static uint8_t cpps_pack_meas_ntf(struct cpps_ntf_cp_meas_cmd *param, uint8_t *pckd_meas)
{
    // Packed Measurement length
    uint8_t pckd_meas_len = CPP_CP_MEAS_MIN_LEN;

    // Check provided flags
    if (CPPS_IS_PRESENT(param->parameters.flags, CPP_MEAS_PEDAL_POWER_BALANCE_PRESENT))
    {
        if (CPPS_IS_FEATURE_SUPPORTED(cpps_env.feat_cfg, CPP_FEAT_PEDAL_POWER_BALANCE_SUPP))
        {
            //Pack Pedal Power Balance info
            pckd_meas[pckd_meas_len] = param->parameters.pedal_power_balance;
            pckd_meas_len++;
        }
        else //Not supported by the profile
        {
            //Force to not supported
            param->parameters.flags &= ~CPP_MEAS_PEDAL_POWER_BALANCE_PRESENT;
        }
    }

    if (CPPS_IS_PRESENT(param->parameters.flags, CPP_MEAS_ACCUM_TORQUE_PRESENT))
    {
        if (CPPS_IS_FEATURE_SUPPORTED(cpps_env.feat_cfg, CPP_FEAT_ACCUM_TORQUE_SUPP))
        {
            //Pack Accumulated Torque info
            co_write16p(&pckd_meas[pckd_meas_len], param->parameters.accum_torque);
            pckd_meas_len += 2;
        }
        else //Not supported by the profile
        {
            //Force to not supported
            param->parameters.flags &= ~CPP_MEAS_ACCUM_TORQUE_PRESENT;
        }
    }

    if (CPPS_IS_PRESENT(param->parameters.flags, CPP_MEAS_WHEEL_REV_DATA_PRESENT))
    {
        if (CPPS_IS_FEATURE_SUPPORTED(cpps_env.feat_cfg, CPP_FEAT_WHEEL_REV_DATA_SUPP))
        {
            //Pack Wheel Revolution Data (Cumulative Wheel & Last Wheel Event Time)
            co_write32p(&pckd_meas[pckd_meas_len], param->parameters.cumul_wheel_rev);
            pckd_meas_len += 4;
            co_write16p(&pckd_meas[pckd_meas_len], param->parameters.last_wheel_evt_time);
            pckd_meas_len += 2;
        }
        else //Not supported by the profile
        {
            //Force to not supported
            param->parameters.flags &= ~CPP_MEAS_WHEEL_REV_DATA_PRESENT;
        }
    }

    if (CPPS_IS_PRESENT(param->parameters.flags, CPP_MEAS_CRANK_REV_DATA_PRESENT))
    {
        if (CPPS_IS_FEATURE_SUPPORTED(cpps_env.feat_cfg, CPP_FEAT_CRANK_REV_DATA_SUPP))
        {
            //Pack Crank Revolution Data (Cumulative Crank & Last Crank Event Time)
            co_write16p(&pckd_meas[pckd_meas_len], param->parameters.cumul_crank_rev);
            pckd_meas_len += 2;
            co_write16p(&pckd_meas[pckd_meas_len], param->parameters.last_crank_evt_time);
            pckd_meas_len += 2;
        }
        else //Not supported by the profile
        {
            //Force to not supported
            param->parameters.flags &= ~CPP_MEAS_CRANK_REV_DATA_PRESENT;
        }
    }

    if (CPPS_IS_PRESENT(param->parameters.flags, CPP_MEAS_EXTREME_FORCE_MAGNITUDES_PRESENT))
    {
        if (CPPS_IS_FEATURE_SUPPORTED(cpps_env.feat_cfg, CPP_FEAT_EXTREME_MAGNITUDES_SUPP)
                && CPPS_IS_CLEAR(cpps_env.feat_cfg, CPP_FEAT_SENSOR_MEAS_CONTEXT))
        {
            //Pack Extreme Force Magnitudes (Maximum Force Magnitude & Minimum Force Magnitude)
            co_write16p(&pckd_meas[pckd_meas_len], param->parameters.max_force_magnitude);
            pckd_meas_len += 2;
            co_write16p(&pckd_meas[pckd_meas_len], param->parameters.min_force_magnitude);
            pckd_meas_len += 2;
        }
        else //Not supported by the profile
        {
            //Force to not supported
            param->parameters.flags &= ~CPP_MEAS_EXTREME_FORCE_MAGNITUDES_PRESENT;
        }
    }

    if (CPPS_IS_PRESENT(param->parameters.flags, CPP_MEAS_EXTREME_TORQUE_MAGNITUDES_PRESENT))
    {
        if (CPPS_IS_FEATURE_SUPPORTED(cpps_env.feat_cfg, CPP_FEAT_EXTREME_MAGNITUDES_SUPP)
                && CPPS_IS_SET(cpps_env.feat_cfg, CPP_FEAT_SENSOR_MEAS_CONTEXT))
        {
            //Pack Extreme Force Magnitudes (Maximum Force Magnitude & Minimum Force Magnitude)
            co_write16p(&pckd_meas[pckd_meas_len], param->parameters.max_torque_magnitude);
            pckd_meas_len += 2;
            co_write16p(&pckd_meas[pckd_meas_len], param->parameters.min_torque_magnitude);
            pckd_meas_len += 2;
        }
        else //Not supported by the profile
        {
            //Force to not supported
            param->parameters.flags &= ~CPP_MEAS_EXTREME_TORQUE_MAGNITUDES_PRESENT;
        }
    }

    if (CPPS_IS_PRESENT(param->parameters.flags, CPP_MEAS_EXTREME_ANGLES_PRESENT))
    {
        if (CPPS_IS_FEATURE_SUPPORTED(cpps_env.feat_cfg, CPP_FEAT_EXTREME_ANGLES_SUPP))
        {
            //Pack Extreme Angles (Maximum Angle & Minimum Angle)
            //Force to 12 bits
            param->parameters.max_angle &= 0x0FFF;
            param->parameters.min_angle &= 0x0FFF;
            uint32_t angle = (uint32_t) (param->parameters.max_angle | (param->parameters.min_angle<<12));
            co_write24p(&pckd_meas[pckd_meas_len], angle);
            pckd_meas_len += 3;

        }
        else //Not supported by the profile
        {
            //Force to not supported
            param->parameters.flags &= ~CPP_MEAS_EXTREME_ANGLES_PRESENT;
        }
    }

    if (CPPS_IS_PRESENT(param->parameters.flags, CPP_MEAS_TOP_DEAD_SPOT_ANGLE_PRESENT))
    {
        if (CPPS_IS_FEATURE_SUPPORTED(cpps_env.feat_cfg, CPP_FEAT_TOPBOT_DEAD_SPOT_ANGLES_SUPP))
        {
            //Pack Top Dead Spot Angle
            co_write16p(&pckd_meas[pckd_meas_len], param->parameters.top_dead_spot_angle);
            pckd_meas_len += 2;
        }
        else
        {
            //Force to not supported
            param->parameters.flags &= ~CPP_MEAS_TOP_DEAD_SPOT_ANGLE_PRESENT;
        }
    }

    if (CPPS_IS_PRESENT(param->parameters.flags, CPP_MEAS_BOTTOM_DEAD_SPOT_ANGLE_PRESENT))
    {
        if (CPPS_IS_FEATURE_SUPPORTED(cpps_env.feat_cfg, CPP_FEAT_TOPBOT_DEAD_SPOT_ANGLES_SUPP))
        {
            //Pack Bottom Dead Spot Angle
            co_write16p(&pckd_meas[pckd_meas_len], param->parameters.bot_dead_spot_angle);
            pckd_meas_len += 2;
        }
        else
        {
            //Force to not supported
            param->parameters.flags &= ~CPP_MEAS_BOTTOM_DEAD_SPOT_ANGLE_PRESENT;
        }
    }

    if (CPPS_IS_PRESENT(param->parameters.flags, CPP_MEAS_ACCUM_ENERGY_PRESENT))
    {
        if (CPPS_IS_FEATURE_SUPPORTED(cpps_env.feat_cfg, CPP_FEAT_ACCUM_ENERGY_SUPP))
        {
            //Pack Accumulated Energy
            co_write16p(&pckd_meas[pckd_meas_len], param->parameters.accum_energy);
            pckd_meas_len += 2;
        }
        else
        {
            //Force to not supported
            param->parameters.flags &= ~CPP_MEAS_ACCUM_ENERGY_PRESENT;
        }
    }

    // Force the unused bits of the flag value to 0
    param->parameters.flags &= CPP_MEAS_ALL_SUPP;
    // Flags value
    co_write16p(&pckd_meas[0], param->parameters.flags);
    //Instant Power (Mandatory)
    co_write16p(&pckd_meas[2], param->parameters.inst_power);

    return pckd_meas_len;
}

/**
 ****************************************************************************************
 * @brief Splits notifications in order to be sent with default MTU
 * @param[in] pckd_meas packed message
 * @param[out] ntf1 first packed notification
 * @param[out] len1 length of the first notification
 * @param[out] ntf2 second packed notification
 * @param[out] len2 length of the second notification
 * @return status of the operation
 ****************************************************************************************
 */

static uint8_t cpps_split_meas_ntf(uint8_t *pckd_meas, uint8_t *ntf1, uint8_t *len1, uint8_t *ntf2, uint8_t *len2)
{
    uint16_t flags = co_read16p(&pckd_meas[0]), flags_cp;
    uint8_t pckd_meas_idx = CPP_CP_MEAS_MIN_LEN, len = 0;

    // Initialize parameters
    uint8_t *buff_copy_pt = ntf1;
    uint8_t *len_copy_pt = len1;
    cpps_env.ntf_len = CPP_CP_MEAS_MIN_LEN;

    // Copy status flags
    co_write16p(&ntf1[0], (flags & (CPP_MEAS_PEDAL_POWER_BALANCE_REFERENCE |
                                        CPP_MEAS_ACCUM_TORQUE_SOURCE |
                                        CPP_MEAS_OFFSET_COMPENSATION_INDICATOR)));
    co_write16p(&ntf2[0], (flags & (CPP_MEAS_PEDAL_POWER_BALANCE_REFERENCE |
                                        CPP_MEAS_ACCUM_TORQUE_SOURCE |
                                        CPP_MEAS_OFFSET_COMPENSATION_INDICATOR)));
    // Copy Instantaneous power
    memcpy(&ntf1[2], &pckd_meas[2], 2);
    memcpy(&ntf2[2], &pckd_meas[2], 2);

    for (uint16_t feat = CPP_MEAS_PEDAL_POWER_BALANCE_PRESENT;
            feat <= CPP_MEAS_OFFSET_COMPENSATION_INDICATOR;
            feat <<= 1)
    {
        if (CPPS_IS_PRESENT(flags, feat))
        {
            // Field does not fit, change pointers
            if (!(((ATT_DEFAULT_MTU - 3) - (*len_copy_pt)) >= CPP_CP_MEAS_MAX_PAIR_SIZE))
            {
                cpps_env.ntf_pending = true;
                buff_copy_pt = ntf2;
                len_copy_pt = len2;
            }

            switch (feat)
            {
                case CPP_MEAS_PEDAL_POWER_BALANCE_PRESENT:
                    // Copy uint8
                    buff_copy_pt[(*len_copy_pt)] = pckd_meas[pckd_meas_idx];
                    len = 1;
                    break;

                case CPP_MEAS_ACCUM_TORQUE_PRESENT:
                case CPP_MEAS_TOP_DEAD_SPOT_ANGLE_PRESENT:
                case CPP_MEAS_ACCUM_ENERGY_PRESENT:
                case CPP_MEAS_BOTTOM_DEAD_SPOT_ANGLE_PRESENT:
                    // Copy uint16
                    memcpy(&buff_copy_pt[(*len_copy_pt)], &pckd_meas[pckd_meas_idx], 2);
                    len = 2;
                    break;

                case CPP_MEAS_EXTREME_ANGLES_PRESENT:
                    // Copy uint24
                    memcpy(&buff_copy_pt[(*len_copy_pt)], &pckd_meas[pckd_meas_idx], 3);
                    len = 3;
                    break;

                case CPP_MEAS_CRANK_REV_DATA_PRESENT:
                case CPP_MEAS_EXTREME_FORCE_MAGNITUDES_PRESENT:
                case CPP_MEAS_EXTREME_TORQUE_MAGNITUDES_PRESENT:
                    // Copy uint16 + uint16
                    memcpy(&buff_copy_pt[(*len_copy_pt)], &pckd_meas[pckd_meas_idx], 4);
                    len = 4;
                    break;

                case CPP_MEAS_WHEEL_REV_DATA_PRESENT:
                    // Copy uint32 + uint16
                    memcpy(&buff_copy_pt[(*len_copy_pt)], &pckd_meas[pckd_meas_idx], 6);
                    len = 6;
                    break;

                default:
                    len = 0;
                    break;
            }

            if (len)
            {
                // Update indexes
                pckd_meas_idx += len;
                (*len_copy_pt) += len;

                // Copy flag to current message
                flags_cp = co_read16p(&buff_copy_pt[0]) | feat;
                co_write16p(&buff_copy_pt[0], flags_cp);
            }
        }
    }
    return pckd_meas_idx;
}

/**
 ****************************************************************************************
 * @brief updates the environment with the descriptor configuration and sends indication
 * @param[in] prfl_config profile descriptor code
 * @param[in] param pointer to the message parameters
 * @return status of the operation
 ****************************************************************************************
 */
static uint8_t cpps_update_characteristic_config(uint8_t prfl_config, struct gattc_write_cmd_ind const *param)
{
    // New configuration
    struct cpps_ntf_ind_cfg_ind *ind = KE_MSG_ALLOC(CPPS_NTF_IND_CFG_IND,
                                                     cpps_env.con_info.appid, cpps_env.con_info.prf_id,
                                                     cpps_ntf_ind_cfg_ind);

    ind->conhdl    = gapc_get_conhdl(cpps_env.con_info.conidx);

    // Status
    uint8_t status = PRF_ERR_OK;

    uint16_t ntf_cfg;

    // Get the value
    co_write16p(&ntf_cfg, param->value[0]);

    // Check if the value is correct
    if (prfl_config == CPP_PRF_CFG_FLAG_CTNL_PT_IND)
    {
        if ((ntf_cfg == PRF_CLI_STOP_NTFIND) || (ntf_cfg == PRF_CLI_START_IND))
        {
            // Save the new configuration in the environment
            (ntf_cfg == PRF_CLI_STOP_NTFIND) ?
                    CPPS_DISABLE_NTF_IND_BCST(prfl_config) : CPPS_ENABLE_NTF_IND_BCST(prfl_config);
        }
        else
        {
            status = CPP_ERROR_INAPP_CONNECT_PARAMS;
        }
    }
    else if (prfl_config == CPP_PRF_CFG_FLAG_SP_MEAS_NTF)
    {
        if ((ntf_cfg == PRF_SRV_STOP_BCST) || (ntf_cfg == PRF_SRV_START_BCST))
        {
            // Save the new configuration in the environment
            (ntf_cfg == PRF_SRV_STOP_BCST) ?
                    CPPS_DISABLE_NTF_IND_BCST(prfl_config) : CPPS_ENABLE_NTF_IND_BCST(prfl_config);
        }
        else
        {
            status = CPP_ERROR_INAPP_CONNECT_PARAMS;
        }
    }
    else
    {
        if ((ntf_cfg == PRF_CLI_STOP_NTFIND) || (ntf_cfg == PRF_CLI_START_NTF))
        {
            if (prfl_config != CPP_PRF_CFG_FLAG_VECTOR_NTF)
            {
                // Save the new configuration in the environment
                (ntf_cfg == PRF_CLI_STOP_NTFIND) ?
                        CPPS_DISABLE_NTF_IND_BCST(prfl_config) : CPPS_ENABLE_NTF_IND_BCST(prfl_config);
            }
        }
        else
        {
            status = CPP_ERROR_INAPP_CONNECT_PARAMS;
        }
    }

    if (status == PRF_ERR_OK)
    {
        if (prfl_config != CPP_PRF_CFG_FLAG_VECTOR_NTF)
        {
            // Set the value in the database
            attmdb_att_set_value(param->handle, sizeof(uint16_t), (uint8_t *)&ntf_cfg);
        }
        ind->char_code = prfl_config;
        ind->ntf_cfg   = ntf_cfg;

        ke_msg_send(ind);
    }
    else
    {
        ke_msg_free(ke_param2msg(ind));
    }

    return status;
}

/**
 ****************************************************************************************
 * @brief Packs vector notifications
 * @param[in] param Pointer to the parameters of the message.
 * @param[out] pckd_vector pointer to packed message
 * @return length
 ****************************************************************************************
 */
static uint8_t cpps_pack_vector_ntf(struct cpps_ntf_cp_vector_cmd *param, uint8_t *pckd_vector)
{
    // Packed Measurement length
    uint8_t pckd_vector_len = CPP_CP_VECTOR_MIN_LEN;

    // Check provided flags
    if (CPPS_IS_PRESENT(param->parameters.flags, CPP_VECTOR_CRANK_REV_DATA_PRESENT))
    {
        if (CPPS_IS_FEATURE_SUPPORTED(cpps_env.feat_cfg, CPP_FEAT_CRANK_REV_DATA_SUPP))
        {
            //Pack Crank Revolution Data (Cumulative Crank & Last Crank Event Time)
            co_write16p(&pckd_vector[pckd_vector_len], param->parameters.cumul_crank_rev);
            pckd_vector_len += 2;
            co_write16p(&pckd_vector[pckd_vector_len], param->parameters.last_crank_evt_time);
            pckd_vector_len += 2;
        }
        else //Not supported by the profile
        {
            //Force to not supported
            param->parameters.flags &= ~CPP_VECTOR_CRANK_REV_DATA_PRESENT;
        }
    }

    if (CPPS_IS_PRESENT(param->parameters.flags, CPP_VECTOR_FIRST_CRANK_MEAS_ANGLE_PRESENT))
    {
        if (CPPS_IS_FEATURE_SUPPORTED(cpps_env.feat_cfg, CPP_FEAT_EXTREME_ANGLES_SUPP))
        {
            //Pack First Crank Measurement Angle
            co_write16p(&pckd_vector[pckd_vector_len], param->parameters.first_crank_meas_angle);
            pckd_vector_len += 2;
        }
        else //Not supported by the profile
        {
            //Force to not supported
            param->parameters.flags &= ~CPP_VECTOR_FIRST_CRANK_MEAS_ANGLE_PRESENT;
        }
    }

    if (CPPS_IS_PRESENT(param->parameters.flags, CPP_VECTOR_INST_FORCE_MAGNITUDE_ARRAY_PRESENT))
    {
        if (CPPS_IS_CLEAR(cpps_env.feat_cfg, CPP_FEAT_SENSOR_MEAS_CONTEXT))
        {
            //Pack Instantaneous Force Magnitude Array
            if ((param->parameters.nb > CPP_CP_VECTOR_MIN_LEN) &&
                    (param->parameters.nb <= CPP_CP_VECTOR_MAX_LEN - pckd_vector_len))
            {
                for(int j=0; j<param->parameters.nb; j++)
                {
                    co_write16p(&pckd_vector[pckd_vector_len], param->parameters.force_torque_magnitude[j]);
                    pckd_vector_len += 2;
                }
            }
            else
            {
                return 0;
            }
        }
        else //Not supported by the profile
        {
            //Force to not supported
            param->parameters.flags &= ~CPP_VECTOR_INST_FORCE_MAGNITUDE_ARRAY_PRESENT;
        }
    }

    if (CPPS_IS_PRESENT(param->parameters.flags, CPP_VECTOR_INST_TORQUE_MAGNITUDE_ARRAY_PRESENT))
    {
        if (CPPS_IS_SET(cpps_env.feat_cfg, CPP_FEAT_SENSOR_MEAS_CONTEXT))
        {
            //Pack Instantaneous Torque Magnitude Array
            if ((param->parameters.nb > CPP_CP_VECTOR_MIN_LEN) &&
                    (param->parameters.nb <= CPP_CP_VECTOR_MAX_LEN - pckd_vector_len))
            {
                for(int j=0; j<param->parameters.nb; j++)
                {
                    co_write16p(&pckd_vector[pckd_vector_len], param->parameters.force_torque_magnitude[j]);
                    pckd_vector_len += 2;
                }
            }
            else
            {
                return 0;
            }
        }
        else //Not supported by the profile
        {
            //Force to not supported
            param->parameters.flags &= ~CPP_VECTOR_INST_TORQUE_MAGNITUDE_ARRAY_PRESENT;
        }
    }

//            if (CPPS_IS_PRESENT(param->parameters.flags, (CPP_VECTOR_INST_MEAS_DIRECTION_LSB |
//                                                            CPP_VECTOR_INST_MEAS_DIRECTION_MSB)))
//            {
//                if (CPPS_IS_FEATURE_SUPPORTED(cpps_env.feat_cfg, CPP_FEAT_INSTANT_MEAS_DIRECTION_SUPP))
//                {
//                }
//
//            }
    // Force the unused bits of the flag value to 0
    param->parameters.flags &= CPP_VECTOR_ALL_SUPP;
    // Flags value
    pckd_vector[0] = param->parameters.flags;

    return pckd_vector_len;
}

/**
 ****************************************************************************************
 * @brief Unpack control point and sends indication
 * @param[in] param pointer to message
 * @return status of the operation
 ****************************************************************************************
 */
static uint8_t cpps_unpack_ctnl_point_ind (struct gattc_write_cmd_ind const *param)
{
    // Indication Status
    uint8_t ind_status = CPP_CTNL_PT_RESP_NOT_SUPP;

    // Allocate a request indication message for the application
    struct cpps_ctnl_pt_req_ind *req_ind = KE_MSG_ALLOC(CPPS_CTNL_PT_REQ_IND,
                                                            cpps_env.con_info.appid, cpps_env.con_info.prf_id,
                                                            cpps_ctnl_pt_req_ind);
    // Connection Handle
    req_ind->conhdl    = gapc_get_conhdl(cpps_env.con_info.conidx);
    // Operation Code
    req_ind->op_code   = param->value[0];

    // Operation Code
    switch(req_ind->op_code)
    {
        case (CPP_CTNL_PT_SET_CUMUL_VAL):
        {
            // Check if the Wheel Revolution Data feature is supported
            if (CPPS_IS_FEATURE_SUPPORTED(cpps_env.feat_cfg, CPP_FEAT_WHEEL_REV_DATA_SUPP))
            {
                // Provided parameter in not within the defined range
                ind_status = CPP_CTNL_PT_RESP_INV_PARAM;

                if (param->length == 5)
                {
                    // The request can be handled
                    ind_status = CPP_CTNL_PT_RESP_SUCCESS;
                    // Update the environment
                    cpps_env.operation = CPPS_CTNL_PT_SET_CUMUL_VAL_OP_CODE;
                    // Cumulative value
                    req_ind->value.cumul_val = co_read32p(&param->value[1]);
                }
            }
        } break;

        case (CPP_CTNL_PT_UPD_SENSOR_LOC):
        {
            // Check if the Multiple Sensor Location feature is supported
            if (CPPS_IS_FEATURE_SUPPORTED(cpps_env.feat_cfg, CPP_FEAT_MULT_SENSOR_LOC_SUPP))
            {
                // Provided parameter in not within the defined range
                ind_status = CPP_CTNL_PT_RESP_INV_PARAM;

                if (param->length == 2)
                {
                    // Check the sensor location value
                    if (param->value[1] < CPP_LOC_MAX)
                    {
                        // The request can be handled
                        ind_status = CPP_CTNL_PT_RESP_SUCCESS;
                        // Update the environment
                        cpps_env.operation = CPPS_CTNL_PT_UPD_SENSOR_LOC_OP_CODE;
                        // Sensor Location
                        req_ind->value.sensor_loc = param->value[1];
                    }
                }
            }
        } break;

        case (CPP_CTNL_PT_REQ_SUPP_SENSOR_LOC):
        {
            // Check if the Multiple Sensor Location feature is supported
//            if (CPPS_IS_FEATURE_SUPPORTED(cpps_env.feat_cfg, CPP_FEAT_MULT_SENSOR_LOC_SUPP))
            {
                // The request can be handled
                ind_status = CPP_CTNL_PT_RESP_SUCCESS;
                // Update the environment
                cpps_env.operation = CPPS_CTNL_PT_REQ_SUPP_SENSOR_LOC_OP_CODE;
            }
        } break;

        case (CPP_CTNL_PT_SET_CRANK_LENGTH):
        {
            // Check if the Crank Length Adjustment feature is supported
            if (CPPS_IS_FEATURE_SUPPORTED(cpps_env.feat_cfg, CPP_FEAT_CRANK_LENGTH_ADJ_SUPP))
            {
                // Provided parameter in not within the defined range
                ind_status = CPP_CTNL_PT_RESP_INV_PARAM;

                if (param->length == 3)
                {
                    // The request can be handled
                    ind_status = CPP_CTNL_PT_RESP_SUCCESS;
                    // Update the environment
                    cpps_env.operation = CPPS_CTNL_PT_SET_CRANK_LENGTH_OP_CODE;
                    // Crank Length
                    req_ind->value.crank_length = co_read16p(&param->value[1]);
                }
            }

        } break;

        case (CPP_CTNL_PT_REQ_CRANK_LENGTH):
        {
            // Check if the Crank Length Adjustment feature is supported
//            if (CPPS_IS_FEATURE_SUPPORTED(cpps_env.feat_cfg, CPP_FEAT_CRANK_LENGTH_ADJ_SUPP))
            {
                // The request can be handled
                ind_status = CPP_CTNL_PT_RESP_SUCCESS;
                // Update the environment
                cpps_env.operation = CPPS_CTNL_PT_REQ_CRANK_LENGTH_OP_CODE;
            }
        } break;

        case (CPP_CTNL_PT_SET_CHAIN_LENGTH):
        {
            // Check if the Chain Length Adjustment feature is supported
            if (CPPS_IS_FEATURE_SUPPORTED(cpps_env.feat_cfg, CPP_FEAT_CHAIN_LENGTH_ADJ_SUPP))
            {
                // Provided parameter in not within the defined range
                ind_status = CPP_CTNL_PT_RESP_INV_PARAM;

                if (param->length == 3)
                {
                    // The request can be handled
                    ind_status = CPP_CTNL_PT_RESP_SUCCESS;
                    // Update the environment
                    cpps_env.operation = CPPS_CTNL_PT_SET_CHAIN_LENGTH_OP_CODE;
                    // Chain Length
                    req_ind->value.crank_length = co_read16p(&param->value[1]);
                }
            }

        } break;

        case (CPP_CTNL_PT_REQ_CHAIN_LENGTH):
        {
            // Check if the Chain Length Adjustment feature is supported
//            if (CPPS_IS_FEATURE_SUPPORTED(cpps_env.feat_cfg, CPP_FEAT_CHAIN_LENGTH_ADJ_SUPP))
            {
                // The request can be handled
                ind_status = CPP_CTNL_PT_RESP_SUCCESS;
                // Update the environment
                cpps_env.operation = CPPS_CTNL_PT_REQ_CHAIN_LENGTH_OP_CODE;
            }
        } break;

        case (CPP_CTNL_PT_SET_CHAIN_WEIGHT):
        {
            // Check if the Chain Weight Adjustment feature is supported
            if (CPPS_IS_FEATURE_SUPPORTED(cpps_env.feat_cfg, CPP_FEAT_CHAIN_WEIGHT_ADJ_SUPP))
            {
                // Provided parameter in not within the defined range
                ind_status = CPP_CTNL_PT_RESP_INV_PARAM;

                if (param->length == 3)
                {
                    // The request can be handled
                    ind_status = CPP_CTNL_PT_RESP_SUCCESS;
                    // Update the environment
                    cpps_env.operation = CPPS_CTNL_PT_SET_CHAIN_WEIGHT_OP_CODE;
                    // Chain Weight
                    req_ind->value.chain_weight = co_read16p(&param->value[1]);
                }
            }

        } break;

        case (CPP_CTNL_PT_REQ_CHAIN_WEIGHT):
        {
            // Check if the Chain Weight Adjustment feature is supported
//            if (CPPS_IS_FEATURE_SUPPORTED(cpps_env.feat_cfg, CPP_FEAT_CHAIN_WEIGHT_ADJ_SUPP))
            {
                // The request can be handled
                ind_status = CPP_CTNL_PT_RESP_SUCCESS;
                // Update the environment
                cpps_env.operation = CPPS_CTNL_PT_REQ_CHAIN_WEIGHT_OP_CODE;
            }
        } break;

        case (CPP_CTNL_PT_SET_SPAN_LENGTH):
        {
            // Check if the Span Length Adjustment feature is supported
            if (CPPS_IS_FEATURE_SUPPORTED(cpps_env.feat_cfg, CPP_FEAT_SPAN_LENGTH_ADJ_SUPP))
            {
                // Provided parameter in not within the defined range
                ind_status = CPP_CTNL_PT_RESP_INV_PARAM;

                if (param->length == 3)
                {
                    // The request can be handled
                    ind_status = CPP_CTNL_PT_RESP_SUCCESS;
                    // Update the environment
                    cpps_env.operation = CPPS_CTNL_PT_SET_SPAN_LENGTH_OP_CODE;
                    // Span Length
                    req_ind->value.span_length = co_read16p(&param->value[1]);
                }
            }

        } break;

        case (CPP_CTNL_PT_REQ_SPAN_LENGTH):
        {
            // Check if the Span Length Adjustment feature is supported
//            if (CPPS_IS_FEATURE_SUPPORTED(cpps_env.feat_cfg, CPP_FEAT_SPAN_LENGTH_ADJ_SUPP))
            {
                // The request can be handled
                ind_status = CPP_CTNL_PT_RESP_SUCCESS;
                // Update the environment
                cpps_env.operation = CPPS_CTNL_PT_REQ_SPAN_LENGTH_OP_CODE;
            }
        } break;

        case (CPP_CTNL_PT_START_OFFSET_COMP):
        {
            // Check if the Offset Compensation feature is supported
            if (CPPS_IS_FEATURE_SUPPORTED(cpps_env.feat_cfg, CPP_FEAT_OFFSET_COMP_SUPP))
            {
                // The request can be handled
                ind_status = CPP_CTNL_PT_RESP_SUCCESS;
                // Update the environment
                cpps_env.operation = CPPS_CTNL_PT_START_OFFSET_COMP_OP_CODE;
            }
        } break;

        case (CPP_CTNL_MASK_CP_MEAS_CH_CONTENT):
        {
            // Check if the CP Masking feature is supported
            if (CPPS_IS_FEATURE_SUPPORTED(cpps_env.feat_cfg, CPP_FEAT_CP_MEAS_CH_CONTENT_MASKING_SUPP))
            {
                // Provided parameter in not within the defined range
                ind_status = CPP_CTNL_PT_RESP_INV_PARAM;

                if (param->length == 3)
                {
                    // The request can be handled
                    ind_status = CPP_CTNL_PT_RESP_SUCCESS;
                    // Update the environment
                    cpps_env.operation = CPPS_CTNL_MASK_CP_MEAS_CH_CONTENT_OP_CODE;
                    // Mask content
                    req_ind->value.mask_content = co_read16p(&param->value[1]);
                }
            }
        } break;

        case (CPP_CTNL_REQ_SAMPLING_RATE):
        {
            // Check if the Vector Characteristic is supported
            if (cpps_env.hdl_offset[CPP_CPS_VECTOR_CHAR] != 0x00)
            {
                // The request can be handled
                ind_status = CPP_CTNL_PT_RESP_SUCCESS;
                // Update the environment
                cpps_env.operation = CPPS_CTNL_REQ_SAMPLING_RATE_OP_CODE;
            }
        } break;

        case (CPP_CTNL_REQ_FACTORY_CALIBRATION_DATE):
        {
            // Check if the Factory Calibration Date feature is supported
            if (CPPS_IS_FEATURE_SUPPORTED(cpps_env.feat_cfg, CPP_FEAT_FACTORY_CALIBRATION_DATE_SUPP))
            {
                // The request can be handled
                ind_status = CPP_CTNL_PT_RESP_SUCCESS;
                // Update the environment
                cpps_env.operation = CPPS_CTNL_REQ_FACTORY_CALIBRATION_DATE_OP_CODE;
            }
        } break;

        default:
        {
            // Operation Code is invalid, status is already CPP_CTNL_PT_RESP_NOT_SUPP
        } break;
    }

    // Go to the Busy status
    ke_state_set(TASK_CPPS, CPPS_BUSY);

    // If no error raised, inform the application about the request
    if (ind_status == CPP_CTNL_PT_RESP_SUCCESS)
    {
        // Send the request indication to the application
        ke_msg_send(req_ind);
    }
    else
    {
        // Free the allocated message
        ke_msg_free(ke_param2msg(req_ind));

        // Send an error indication
        cpps_send_rsp_ind(param->handle, param->value[0], ind_status);

        cpps_env.operation = CPPS_CTNL_ERR_IND_OP_CODE;
    }

    return ind_status;
}

/**
 ****************************************************************************************
 * @brief Packs control point
 * @param[in] param Pointer to the parameters of the message.
 * @param[out] rsp pointer to message
 * @return status of the operation
 ****************************************************************************************
 */
static uint8_t cpps_pack_ctnl_point_cfm (struct cpps_ctnl_pt_cfm *param, uint8_t *rsp)
{
    // Response Length (At least 3)
    uint8_t rsp_len = CPP_CP_CNTL_PT_RSP_MIN_LEN;

    // Set the Response Code
    rsp[0] = CPP_CTNL_PT_RSP_CODE;
    // Set the Response Value
    rsp[2] = (param->status > CPP_CTNL_PT_RESP_FAILED) ? CPP_CTNL_PT_RESP_FAILED : param->status;

    switch (cpps_env.operation)
    {
        case (CPPS_CTNL_PT_SET_CUMUL_VAL_OP_CODE):
        {
            // Set the request operation code
            rsp[1] = CPP_CTNL_PT_SET_CUMUL_VAL;
        } break;

        case (CPPS_CTNL_PT_UPD_SENSOR_LOC_OP_CODE):
        {
            // Set the request operation code
            rsp[1] = CPP_CTNL_PT_UPD_SENSOR_LOC;

            if (param->status == CPP_CTNL_PT_RESP_SUCCESS)
            {
                // If operation is successful, update the value in the database
                if (attmdb_att_set_value(CPPS_HANDLE(CPS_IDX_SENSOR_LOC_VAL),
                                     sizeof(uint8_t), (uint8_t *)&param->value.sensor_loc) != ATT_ERR_NO_ERROR)
                {
                    rsp[2] = CPP_CTNL_PT_RESP_FAILED;
                }
            }
            // Store the new value in the environment
            cpps_env.sensor_loc = param->value.sensor_loc;
        } break;

        case (CPPS_CTNL_PT_REQ_SUPP_SENSOR_LOC_OP_CODE):
        {
            // Set the request operation code
            rsp[1] = CPP_CTNL_PT_REQ_SUPP_SENSOR_LOC;

            if (param->status == CPP_CTNL_PT_RESP_SUCCESS)
            {
                // Set the list of supported location
                for (uint8_t counter = 0; counter < CPP_LOC_MAX; counter++)
                {
                    if ((param->value.supp_sensor_loc >> counter) & 0x0001)
                    {
                        rsp[rsp_len] = counter;
                        rsp_len++;
                    }
                }
            }
        } break;

        case (CPPS_CTNL_PT_SET_CRANK_LENGTH_OP_CODE):
        {
            // Set the request operation code
            rsp[1] = CPP_CTNL_PT_SET_CRANK_LENGTH;
        } break;

        case (CPPS_CTNL_PT_REQ_CRANK_LENGTH_OP_CODE):
        {
            // Set the request operation code
            rsp[1] = CPP_CTNL_PT_REQ_CRANK_LENGTH;
            if (param->status == CPP_CTNL_PT_RESP_SUCCESS)
            {
                // Set the response parameter
                co_write16p(&rsp[rsp_len], param->value.crank_length);
                rsp_len += 2;
            }
        } break;

        case (CPPS_CTNL_PT_SET_CHAIN_LENGTH_OP_CODE):
        {
            // Set the request operation code
            rsp[1] = CPP_CTNL_PT_SET_CHAIN_LENGTH;
        } break;

        case (CPPS_CTNL_PT_REQ_CHAIN_LENGTH_OP_CODE):
        {
            // Set the request operation code
            rsp[1] = CPP_CTNL_PT_REQ_CHAIN_LENGTH;
            if (param->status == CPP_CTNL_PT_RESP_SUCCESS)
            {
                // Set the response parameter
                co_write16p(&rsp[rsp_len], param->value.chain_length);
                rsp_len += 2;
            }
        } break;

        case (CPPS_CTNL_PT_SET_CHAIN_WEIGHT_OP_CODE):
        {
            // Set the request operation code
            rsp[1] = CPP_CTNL_PT_SET_CHAIN_WEIGHT;
        } break;


        case (CPPS_CTNL_PT_REQ_CHAIN_WEIGHT_OP_CODE):
        {
            // Set the request operation code
            rsp[1] = CPP_CTNL_PT_REQ_CHAIN_WEIGHT;
            if (param->status == CPP_CTNL_PT_RESP_SUCCESS)
            {
                // Set the response parameter
                co_write16p(&rsp[rsp_len], param->value.chain_weight);
                rsp_len += 2;
            }
        } break;

        case (CPPS_CTNL_PT_SET_SPAN_LENGTH_OP_CODE):
        {
            // Set the request operation code
            rsp[1] = CPP_CTNL_PT_SET_SPAN_LENGTH;
        } break;

        case (CPPS_CTNL_PT_REQ_SPAN_LENGTH_OP_CODE):
        {
            // Set the request operation code
            rsp[1] = CPP_CTNL_PT_REQ_SPAN_LENGTH;
            if (param->status == CPP_CTNL_PT_RESP_SUCCESS)
            {
                // Set the response parameter
                co_write16p(&rsp[rsp_len], param->value.span_length);
                rsp_len += 2;
            }
        } break;

        case (CPPS_CTNL_PT_START_OFFSET_COMP_OP_CODE):
        {
            // Set the request operation code
            rsp[1] = CPP_CTNL_PT_START_OFFSET_COMP;
            if (param->status == CPP_CTNL_PT_RESP_SUCCESS)
            {
                // Set the response parameter
                co_write16p(&rsp[rsp_len], param->value.offset_comp);
                rsp_len += 2;
            }
        } break;

        case (CPPS_CTNL_MASK_CP_MEAS_CH_CONTENT_OP_CODE):
        {
            // Set the request operation code
            rsp[1] = CPP_CTNL_MASK_CP_MEAS_CH_CONTENT;
            if (param->status == CPP_CTNL_PT_RESP_SUCCESS)
            {
                uint16_t cpp_mask_cp_meas_flags [] =
                {
                    CPP_MEAS_PEDAL_POWER_BALANCE_PRESENT,
                    CPP_MEAS_ACCUM_TORQUE_PRESENT,
                    CPP_MEAS_WHEEL_REV_DATA_PRESENT,
                    CPP_MEAS_CRANK_REV_DATA_PRESENT,
                    CPP_MEAS_EXTREME_FORCE_MAGNITUDES_PRESENT |
                        CPP_MEAS_EXTREME_TORQUE_MAGNITUDES_PRESENT,
                    CPP_MEAS_EXTREME_ANGLES_PRESENT,
                    CPP_MEAS_TOP_DEAD_SPOT_ANGLE_PRESENT,
                    CPP_MEAS_BOTTOM_DEAD_SPOT_ANGLE_PRESENT,
                    CPP_MEAS_ACCUM_ENERGY_PRESENT,
                };

                uint16_t mask = 0;
                for (uint8_t count = 0; count < 9; count++)
                {
                    if ((param->value.mask_meas_content >> count) & 0x0001)
                    {
                        mask |= cpp_mask_cp_meas_flags[count];
                    }
                }
                // Revert bits
                mask = ~mask;
                // update the value in the database
                if (attmdb_att_set_value(CPPS_HANDLE(CPS_IDX_CP_MEAS_VAL),
                                     sizeof(uint16_t), (uint8_t *)&mask) != ATT_ERR_NO_ERROR)
                {
                    rsp[2] = CPP_CTNL_PT_RESP_FAILED;
                }
            }
        } break;

        case (CPPS_CTNL_REQ_SAMPLING_RATE_OP_CODE):
        {
            // Set the request operation code
            rsp[1] = CPP_CTNL_REQ_SAMPLING_RATE;
            if (param->status == CPP_CTNL_PT_RESP_SUCCESS)
            {
                // Set the response parameter
                rsp[rsp_len] = param->value.sampling_rate;
                rsp_len ++;
            }
        } break;

        case (CPPS_CTNL_REQ_FACTORY_CALIBRATION_DATE_OP_CODE):
        {
            // Set the request operation code
            rsp[1] = CPP_CTNL_REQ_FACTORY_CALIBRATION_DATE;
            if (param->status == CPP_CTNL_PT_RESP_SUCCESS)
            {
                // Set the response parameter
                rsp_len += prf_pack_date_time(&rsp[rsp_len], &(param->value.factory_calibration));
            }
        } break;

        default:
        {
            rsp[2] = CPP_CTNL_PT_RESP_NOT_SUPP;
        } break;
    }

    return rsp_len;
}


/**
 ****************************************************************************************
 * @brief Update database in non discovery modes
 * @param[in] con_type Connection type
 * @param[in] handle handle
 * @param[in] ntf_cfg provided flag
 * @param[in] cfg_flag requested flag
 * @return status of the operation
 ****************************************************************************************
 */
static int cpps_update_cfg_char_value (uint8_t con_type, uint8_t handle, uint16_t ntf_cfg, uint16_t cfg_flag)
{
    uint8_t status = ATT_ERR_NO_ERROR;
    uint16_t desc_cfg = PRF_CLI_STOP_NTFIND;

    if (con_type != PRF_CON_DISCOVERY)
    {
        // Check the provided value
        if (ntf_cfg & cfg_flag)
        {
            // Store the status
            CPPS_ENABLE_NTF_IND_BCST(cfg_flag);
            // Start
            desc_cfg = PRF_CLI_START_NTF;
        }
    }
    // Set the Characteristic Configuration value in the database
    status = attmdb_att_set_value(CPPS_HANDLE(handle),
            sizeof(uint16_t), (uint8_t *)&desc_cfg);

    return status;
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref CPPS_CREATE_DB_CMD message.
 * @param[in] msgid Id of the message received.
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int cpps_create_db_cmd_handler(ke_msg_id_t const msgid,
                                       struct cpps_create_db_req *param,
                                       ke_task_id_t const dest_id,
                                       ke_task_id_t const src_id)
{
    // Service Configuration Flag
    uint16_t cfg_flag = CPPS_MANDATORY_MASK;
    // Database Creation Status
    uint8_t status;

    // Check if a Cycling Power service has already been added in the database
    if (ke_state_get(dest_id) == CPPS_DISABLED)
    {
        /*
         * Check if Broadcaster role shall be added.
         */
        if(CPPS_IS_FEATURE_SUPPORTED(param->prfl_config, CPPS_BROADCASTER_SUPP_FLAG))
        {
        	//Add configuration to the database
            cfg_flag |= CPPS_MEAS_BCST_MASK;
        }
        /*
         * Check if the CP Vector characteristic shall be added.
         * Mandatory if at least one Vector procedure is supported, otherwise excluded.
         */
        if ((CPPS_IS_FEATURE_SUPPORTED(param->cp_feature, CPP_FEAT_CRANK_REV_DATA_SUPP)) ||
            (CPPS_IS_FEATURE_SUPPORTED(param->cp_feature, CPP_FEAT_EXTREME_ANGLES_SUPP)) ||
            (CPPS_IS_FEATURE_SUPPORTED(param->cp_feature, CPP_FEAT_INSTANT_MEAS_DIRECTION_SUPP)))
        {
            cfg_flag |= CPPS_VECTOR_MASK;
        }
        /*
         * Check if the Control Point characteristic shall be added
         * Mandatory if server supports:
         *     - Wheel Revolution Data
         *     - Multiple Sensor Locations
         *     - Configurable Settings (CPP_CTNL_PT_SET codes)
         *     - Offset Compensation
         *     - Server allows to be requested for parameters (CPP_CTNL_PT_REQ codes)
         */
        if ((CPPS_IS_FEATURE_SUPPORTED(param->prfl_config, CPPS_CTNL_PT_CHAR_SUPP_FLAG)) ||
            (CPPS_IS_FEATURE_SUPPORTED(param->cp_feature, CPP_FEAT_WHEEL_REV_DATA_SUPP)) ||
            (CPPS_IS_FEATURE_SUPPORTED(param->cp_feature, CPP_FEAT_MULT_SENSOR_LOC_SUPP)) ||
            (CPPS_IS_FEATURE_SUPPORTED(param->cp_feature, CPP_FEAT_CRANK_LENGTH_ADJ_SUPP)) ||
            (CPPS_IS_FEATURE_SUPPORTED(param->cp_feature, CPP_FEAT_CHAIN_LENGTH_ADJ_SUPP)) ||
            (CPPS_IS_FEATURE_SUPPORTED(param->cp_feature, CPP_FEAT_CHAIN_WEIGHT_ADJ_SUPP)) ||
            (CPPS_IS_FEATURE_SUPPORTED(param->cp_feature, CPP_FEAT_SPAN_LENGTH_ADJ_SUPP)) ||
            (CPPS_IS_FEATURE_SUPPORTED(param->cp_feature, CPP_FEAT_OFFSET_COMP_SUPP)))
        {
            cfg_flag |= CPPS_CTNL_PT_MASK;
        }

        //Store Config flag
        cpps_env.prfl_cfg = cfg_flag;

        // Add service in the database
        status = attm_svc_create_db(&cpps_env.shdl, (uint8_t *)&cpps_env.prfl_cfg, CPS_IDX_NB,
                                    &cpps_env.hdl_offset[0], dest_id, &cpps_att_db[0]);

        // Check if an error has occured
        if (status == ATT_ERR_NO_ERROR)
        {
            // Force the unused bits of the CP Feature Char value to 0
//            param->cp_feature &= CPP_FEAT_ALL_SUPP;

            // Store the value in the environment
            cpps_env.feat_cfg = param->cp_feature;

            // Set the value of the CP Feature characteristic
            attmdb_att_set_value(CPPS_HANDLE(CPS_IDX_CP_FEAT_VAL), sizeof(uint32_t), (uint8_t *)&param->cp_feature);

            // Store the value in the environment
            cpps_env.sensor_loc = param->sensor_loc;

            // Set the value of the Sensor Location characteristic
            attmdb_att_set_value(CPPS_HANDLE(CPS_IDX_SENSOR_LOC_VAL), sizeof(uint8_t), (uint8_t *)&param->sensor_loc);

            /*
             * Check if the Broadcaster role shall be added.
             */
            if(CPPS_IS_FEATURE_SUPPORTED(cpps_env.prfl_cfg, CPPS_MEAS_BCST_MASK))
            {
                // Optional Permissions
                uint16_t perm;
                // Optional Properties
                uint8_t prop;

                prop = ATT_CHAR_PROP_NTF | ATT_CHAR_PROP_BCAST;
                perm = PERM(RD, ENABLE) | PERM(WR, ENABLE);

                //Add configuration to the database
                attmdb_att_partial_value_update(CPPS_HANDLE(CPS_IDX_CP_MEAS_CHAR), 0, 1, &prop);
                attmdb_att_set_permission(CPPS_HANDLE(CPS_IDX_CP_MEAS_CHAR), perm);
            }

            // Disable CPS
            attmdb_svc_set_permission(cpps_env.shdl, PERM(SVC, DISABLE));

            // If we are here, database has been fulfilled with success, go to idle state
            ke_state_set(TASK_CPPS, CPPS_IDLE);
        }
    }
    else
    {
        // Request is disallowed, a CPS has already been added
        status = PRF_ERR_REQ_DISALLOWED;
    }

    // Send complete event message to the application
    cpps_send_cmp_evt(TASK_CPPS, src_id, GAP_INVALID_CONHDL, CPPS_CREATE_DB_REQ_OP_CODE, status);

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref CPPS_ENABLE_CMD message.
 * @param[in] msgid Id of the message received
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */

static int cpps_enable_cmd_handler(ke_msg_id_t const msgid,
                                    struct cpps_enable_req *param,
                                    ke_task_id_t const dest_id,
                                    ke_task_id_t const src_id)
{
    // Status
    uint8_t status = PRF_ERR_OK;

    // Check the current state of the task and if the provided connection handle is valid
    if ((ke_state_get(TASK_CPPS) == CPPS_IDLE) &&
        (gapc_get_conidx(param->conhdl) != GAP_INVALID_CONIDX))
    {
        // Fill the Connection Information structure
        cpps_env.con_info.conidx = gapc_get_conidx(param->conhdl);
        cpps_env.con_info.prf_id = dest_id;
        cpps_env.con_info.appid  = src_id;

        // Reset the store Client Characteristic Configuration status
        cpps_env.prfl_ntf_ind_cfg = 0;

        // Set the CP Measurement Char. Client Characteristic Configuration value
        if (cpps_update_cfg_char_value(param->con_type, CPS_IDX_CP_MEAS_NTF_CFG,
                param->prfl_ntf_ind_cfg, CPP_PRF_CFG_FLAG_CP_MEAS_NTF) != ATT_ERR_NO_ERROR)
        {
            status = PRF_ERR_NOT_WRITABLE;
        }

        // If supported, set the Meas Char. Server Characteristic Configuration value
        if (CPPS_IS_FEATURE_SUPPORTED(cpps_env.prfl_cfg, CPPS_MEAS_BCST_MASK))
        {
            if (cpps_update_cfg_char_value(param->con_type, CPS_IDX_CP_MEAS_BCST_CFG,
                    param->prfl_ntf_ind_cfg, CPP_PRF_CFG_FLAG_SP_MEAS_NTF) != ATT_ERR_NO_ERROR)
            {
                status = PRF_ERR_NOT_WRITABLE;
            }
        }

        // If supported, set the Vector Char. Client Characteristic Configuration value
        if (CPPS_IS_FEATURE_SUPPORTED(cpps_env.prfl_cfg, CPPS_VECTOR_MASK))
        {
            if (cpps_update_cfg_char_value(param->con_type, CPS_IDX_VECTOR_NTF_CFG,
                    param->prfl_ntf_ind_cfg, CPP_PRF_CFG_FLAG_VECTOR_NTF) != ATT_ERR_NO_ERROR)
            {
                status = PRF_ERR_NOT_WRITABLE;
            }
        }

        // If supported, set the Control Point Char. Client Characteristic Configuration value
        if (CPPS_IS_FEATURE_SUPPORTED(cpps_env.prfl_cfg, CPPS_CTNL_PT_MASK))
        {
            if (cpps_update_cfg_char_value(param->con_type, CPS_IDX_CTNL_PT_IND_CFG,
                    param->prfl_ntf_ind_cfg, CPP_PRF_CFG_FLAG_CTNL_PT_IND) != ATT_ERR_NO_ERROR)
            {
                status = PRF_ERR_NOT_WRITABLE;
            }
        }

        if (status == PRF_ERR_OK)
        {
            // Store the provided Instantaneous Power
            cpps_env.inst_power = param->instantaneous_power;

            //Store the Notify/Indication config
            cpps_env.prfl_ntf_ind_cfg = param->prfl_ntf_ind_cfg;

            // Enable CPS
            attmdb_svc_set_permission(cpps_env.shdl, param->sec_lvl);

            // Go to Connected State
            ke_state_set(dest_id, CPPS_CONNECTED);
        }
    }
    else
    {
        // The request is disallowed (profile already enabled for this connection, or not enough memory, ...)
        status = PRF_ERR_REQ_DISALLOWED;
    }

    // Send response to application
    cpps_send_cmp_evt(dest_id, src_id, param->conhdl, CPPS_ENABLE_REQ_OP_CODE, status);

    return (KE_MSG_CONSUMED);
}


/**
 ****************************************************************************************
 * @brief Handles reception of the @ref CPPS_NTF_CP_MEAS_CMD message.
 * @param[in] msgid Id of the message received.
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance.
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int cpps_ntf_cp_meas_cmd_handler(ke_msg_id_t const msgid,
                                          struct cpps_ntf_cp_meas_cmd *param,
                                          ke_task_id_t const dest_id,
                                          ke_task_id_t const src_id)
{
    // Message status
    uint8_t msg_status = KE_MSG_CONSUMED;
    // State
    ke_state_t state   = ke_state_get(TASK_CPPS);

    // State shall be Connected or Busy
    if (state >= CPPS_CONNECTED)
    {
        // Status
        uint8_t status = PRF_ERR_OK;

        do
        {
            // Check the connection handle
            if (gapc_get_conidx(param->conhdl) != cpps_env.con_info.conidx)
            {
                status = PRF_ERR_INVALID_PARAM;
                break;
            }

            // Check if sending of notifications has been enabled
            if (!CPPS_IS_NTF_IND_BCST_ENABLED(CPP_PRF_CFG_FLAG_CP_MEAS_NTF))
            {
                status = CPP_ERROR_IMPROPERLY_CONFIGURED;
                break;
            }

            // Check the current state of the task
            if (state == CPPS_BUSY)
            {
                // Keep the message for later, status will be PRF_ERR_OK
                msg_status = KE_MSG_SAVED;
                break;
            }

            ASSERT_ERR(cpps_env.operation == CPPS_RESERVED_OP_CODE);

            // Measurement array declaration
            uint8_t pckd_meas[CPP_CP_MEAS_MAX_LEN];
            // Pack Measurement
            uint8_t nb =  cpps_pack_meas_ntf(param, pckd_meas);
            // Split notification if necessary
            if (nb > (gattc_get_mtu(cpps_env.con_info.conidx) - 3))
            {
                uint8_t ntf1[ATT_DEFAULT_MTU - 3], len1 = CPP_CP_MEAS_MIN_LEN;
                cpps_split_meas_ntf(pckd_meas, ntf1, &len1, cpps_env.ntf, &cpps_env.ntf_len);

                // Set the value in the database
                attmdb_att_set_value(CPPS_HANDLE(CPS_IDX_CP_MEAS_VAL), len1, (uint8_t *)&ntf1[0]);
                // Send the notification
                prf_server_send_event((prf_env_struct *)&cpps_env, false, CPPS_HANDLE(CPS_IDX_CP_MEAS_VAL));
            }
            else
            {
                // Set the value in the database
                attmdb_att_set_value(CPPS_HANDLE(CPS_IDX_CP_MEAS_VAL), nb, (uint8_t *)&pckd_meas[0]);
                // Send the notification
                prf_server_send_event((prf_env_struct *)&cpps_env, false, CPPS_HANDLE(CPS_IDX_CP_MEAS_VAL));
            }

            // Configure the environment
            cpps_env.operation = CPPS_NTF_MEAS_OP_CODE;
            // Go to busy state
            ke_state_set(TASK_CPPS, CPPS_BUSY);
        } while (0);

        if (status != PRF_ERR_OK)
        {
            // Send response to application
            cpps_send_cmp_evt(cpps_env.con_info.prf_id, cpps_env.con_info.appid, gapc_get_conhdl(cpps_env.con_info.conidx),
                    CPPS_NTF_MEAS_OP_CODE, status);
        }
    }
    else
    {
        // The profile has not been enabled for a connection or the database has not been created
        cpps_send_cmp_evt(dest_id, src_id, param->conhdl, CPPS_NTF_MEAS_OP_CODE, PRF_ERR_REQ_DISALLOWED);
    }

    return (int)msg_status;
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref CPPS_NTF_CP_VECTOR_CMD message.
 * @param[in] msgid Id of the message received.
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance.
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int cpps_ntf_cp_vector_cmd_handler(ke_msg_id_t const msgid,
                                          struct cpps_ntf_cp_vector_cmd *param,
                                          ke_task_id_t const dest_id,
                                          ke_task_id_t const src_id)
{
    // Message status
    uint8_t msg_status = KE_MSG_CONSUMED;
    // State
    ke_state_t state   = ke_state_get(TASK_CPPS);

    // State shall be Connected or Busy
    if (state >= CPPS_CONNECTED)
    {
        // Status
        uint8_t status = PRF_ERR_OK;

        do
        {
            // Check the connection handle
            if (gapc_get_conidx(param->conhdl) != cpps_env.con_info.conidx)
            {
                status = PRF_ERR_INVALID_PARAM;
                break;
            }

            // Check if sending of notifications has been enabled
            if (!CPPS_IS_NTF_IND_BCST_ENABLED(CPP_PRF_CFG_FLAG_VECTOR_NTF))
            {
                status = CPP_ERROR_IMPROPERLY_CONFIGURED;
                break;
            }

            // Check the current state of the task
            if (state == CPPS_BUSY)
            {
                // Keep the message for later, status will be PRF_ERR_OK
                msg_status = KE_MSG_SAVED;
                break;
            }

            ASSERT_ERR(cpps_env.operation == CPPS_RESERVED_OP_CODE);

            // Vector array declaration
            uint8_t pckd_vector[CPP_CP_VECTOR_MAX_LEN];
            // Pack Vector
            uint8_t nb = cpps_pack_vector_ntf(param, pckd_vector);

            if (!nb)
            {
                status = PRF_ERR_INVALID_PARAM;
                break;
            }

            // Set the value in the database
            attmdb_att_set_value(CPPS_HANDLE(CPS_IDX_VECTOR_VAL), nb, (uint8_t *)&pckd_vector[0]);

            // Send the notification
            prf_server_send_event((prf_env_struct *)&cpps_env, false, CPPS_HANDLE(CPS_IDX_VECTOR_VAL));

            // Configure the environment
            cpps_env.operation = CPPS_NTF_VECTOR_OP_CODE;
            // Go to busy state
            ke_state_set(TASK_CPPS, CPPS_BUSY);
        } while (0);

        if (status != PRF_ERR_OK)
        {
            // Send response to application
            cpps_send_cmp_evt(cpps_env.con_info.prf_id, cpps_env.con_info.appid, gapc_get_conhdl(cpps_env.con_info.conidx),
                    CPPS_NTF_VECTOR_OP_CODE, status);
        }
    }
    else
    {
        // The profile has not been enabled for a connection or the database has not been created
        cpps_send_cmp_evt(dest_id, src_id, param->conhdl, CPPS_NTF_VECTOR_OP_CODE, PRF_ERR_REQ_DISALLOWED);
    }

    return (int)msg_status;
}


/**
 ****************************************************************************************
 * @brief Handles reception of the @ref GATTC_WRITE_CMD_IND message.
 * @param[in] msgid Id of the message received.
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance.
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gattc_write_cmd_ind_handler(ke_msg_id_t const msgid,
                                       struct gattc_write_cmd_ind const *param,
                                       ke_task_id_t const dest_id,
                                       ke_task_id_t const src_id)
{
    // Message status
    uint8_t msg_status = KE_MSG_CONSUMED;
    // Status
    uint8_t status = PRF_ERR_OK;

    // check if waiting for indication confirmation
    if((ke_state_get(TASK_CPPS) == CPPS_WAIT_FOR_CFM)
        && (KE_TYPE_GET(src_id) == TASK_GATTC))
    {
        // reschedule message once to ensure that indication reception is not on-going.
        ke_msg_forward(param, dest_id, dest_id);

        // Message forwarded do not free it
        msg_status = KE_MSG_NO_FREE;
    }

    // Check the connection handle
    else if (ke_state_get(TASK_CPPS) >= CPPS_CONNECTED)
    {
        // CP Measurement Characteristic, Client Characteristic Configuration Descriptor
        if (param->handle == (CPPS_HANDLE(CPS_IDX_CP_MEAS_NTF_CFG)))
        {
            // Update value
            status = cpps_update_characteristic_config(CPP_PRF_CFG_FLAG_CP_MEAS_NTF, param);
            // Send the write response to the peer device
            atts_write_rsp_send(cpps_env.con_info.conidx, param->handle, status);
        }
        // CP Measurement Characteristic, Server Characteristic Configuration Descriptor
        else if (param->handle == (CPPS_HANDLE(CPS_IDX_CP_MEAS_BCST_CFG)))
        {
            // Update value
            status = cpps_update_characteristic_config(CPP_PRF_CFG_FLAG_SP_MEAS_NTF, param);
            // Send the write response to the peer device
            atts_write_rsp_send(cpps_env.con_info.conidx, param->handle, status);
        }
        // CP Vector Characteristic, Client Characteristic Configuration Descriptor
        else if (param->handle == (CPPS_HANDLE(CPS_IDX_VECTOR_NTF_CFG)))
        {
            ASSERT_ERR(cpps_env.hdl_offset[CPP_CPS_VECTOR_CHAR] != 0x00);

            // Update value
            status = cpps_update_characteristic_config(CPP_PRF_CFG_FLAG_VECTOR_NTF, param);
        }
        // CP Control Point Characteristic
        else
        {
            ASSERT_ERR(cpps_env.hdl_offset[CPP_CPS_CTNL_PT_CHAR] != 0x00);

            // CP Control Point, Client Characteristic Configuration Descriptor
            if (param->handle == (CPPS_HANDLE(CPS_IDX_CTNL_PT_IND_CFG)))
            {
                // Update value
                status = cpps_update_characteristic_config(CPP_PRF_CFG_FLAG_CTNL_PT_IND, param);
                // Send the write response to the peer device
                atts_write_rsp_send(cpps_env.con_info.conidx, param->handle, status);
            }
            // CP Control Point Characteristic
            else if (param->handle == (CPPS_HANDLE(CPS_IDX_CTNL_PT_VAL)))
            {
                do
                {
                    // Check if sending of indications has been enabled
                    if (!CPPS_IS_NTF_IND_BCST_ENABLED(CPP_PRF_CFG_FLAG_CTNL_PT_IND))
                    {
                        // CPP improperly configured
                        status = CPP_ERROR_IMPROPERLY_CONFIGURED;
                        break;
                    }

                    if (cpps_env.operation >= CPPS_CTNL_PT_SET_CUMUL_VAL_OP_CODE)
                    {
                        // A procedure is already in progress
                        status = CPP_ERROR_PROC_IN_PROGRESS;
                        break;
                    }

                    if (cpps_env.operation == CPPS_NTF_MEAS_OP_CODE)
                    {
                        // Keep the message until the end of the current procedure
                        msg_status = KE_MSG_NO_FREE;
                        break;
                    }
                } while (0);

                // Unpack Control Point parameters
                cpps_unpack_ctnl_point_ind (param);

                // Send the write response to the peer device
                atts_write_rsp_send(cpps_env.con_info.conidx, param->handle, status);
            }
            else
            {
                ASSERT_ERR(0);
            }
        }
    }
    // else drop the message

    return (int)msg_status;
}


/**
 ****************************************************************************************
 * @brief Handles reception of the @ref CPPS_VECTOR_CFG_CFM message.
 * @param[in] msgid Id of the message received.
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance.
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int cpps_vector_cfg_cfm_handler(ke_msg_id_t const msgid,
                                          struct cpps_vector_cfg_cfm *param,
                                          ke_task_id_t const dest_id,
                                          ke_task_id_t const src_id)
{
    // Status
    uint8_t status = CPP_ERROR_INAPP_CONNECT_PARAMS;

    if (param->status == PRF_ERR_OK)
    {
        if ((param->ntf_cfg == PRF_CLI_STOP_NTFIND) || (param->ntf_cfg == PRF_CLI_START_NTF))
        {
            // Save the new configuration in the environment
            (param->ntf_cfg == PRF_CLI_STOP_NTFIND) ?
                    CPPS_DISABLE_NTF_IND_BCST(CPP_PRF_CFG_FLAG_VECTOR_NTF) :
                    CPPS_ENABLE_NTF_IND_BCST(CPP_PRF_CFG_FLAG_VECTOR_NTF);

            // Set the value in the database
            attmdb_att_set_value(CPPS_HANDLE(CPS_IDX_VECTOR_NTF_CFG), sizeof(uint16_t), (uint8_t *)&param->ntf_cfg);

            status = PRF_ERR_OK;
        }
    }

    // Send the write response to the peer device
    atts_write_rsp_send(cpps_env.con_info.conidx, CPPS_HANDLE(CPS_IDX_VECTOR_NTF_CFG), status);


    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref CPPS_CTNL_PT_CFM message.
 * @param[in] msgid Id of the message received.
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance.
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int cpps_ctnl_pt_cfm_handler(ke_msg_id_t const msgid,
                                          struct cpps_ctnl_pt_cfm *param,
                                          ke_task_id_t const dest_id,
                                          ke_task_id_t const src_id)
{
    // Control Point Response
    uint8_t rsp[CPP_CP_CNTL_PT_RSP_MAX_LEN];

    do
    {
        // Check the current operation
        if (cpps_env.operation < CPPS_CTNL_PT_SET_CUMUL_VAL_OP_CODE)
        {
            // The confirmation has been sent without request indication, ignore
            break;
        }

        // The SC Control Point Characteristic must be supported if we are here
        ASSERT_ERR(cpps_env.hdl_offset[CPP_CPS_CTNL_PT_CHAR] != 0x00);

        // Pack Control Point confirmation
        uint8_t nb = cpps_pack_ctnl_point_cfm (param, rsp);

        // Set the value in the database - If we are here the handle is valid
        attmdb_att_set_value(CPPS_HANDLE(CPS_IDX_CTNL_PT_VAL),
                             nb, (uint8_t *)&rsp);

        // Send the response indication to the peer device
        prf_server_send_event((prf_env_struct *)&cpps_env, true,
                              CPPS_HANDLE(CPS_IDX_CTNL_PT_VAL));

        // Wait for Confirm
        ke_state_set(TASK_CPPS, CPPS_WAIT_FOR_CFM);
    } while (0);

    return (KE_MSG_CONSUMED);
}


/**
 ****************************************************************************************
 * @brief Handles @ref GATT_NOTIFY_CMP_EVT message meaning that a notification or an indication
 * has been correctly sent to peer device (but not confirmed by peer device).
 *
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance
 * @param[in] src_id    ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gattc_cmp_evt_handler(ke_msg_id_t const msgid,  struct gattc_cmp_evt const *param,
                                 ke_task_id_t const dest_id, ke_task_id_t const src_id)
{
    // Check if a connection exists
    if (ke_state_get(TASK_CPPS) >= CPPS_CONNECTED)
    {
        switch (param->req_type)
        {
            case (GATTC_NOTIFY):
            {
                if (cpps_env.operation == CPPS_NTF_MEAS_OP_CODE)
                {
                    // Notification pending?
                    if (cpps_env.ntf_pending)
                    {
                        cpps_env.ntf_pending = false;

                        // Set the value in the database
                        attmdb_att_set_value(CPPS_HANDLE(CPS_IDX_CP_MEAS_VAL), cpps_env.ntf_len, (uint8_t *)&cpps_env.ntf[0]);
                        // Send the notification
                        prf_server_send_event((prf_env_struct *)&cpps_env, false, CPPS_HANDLE(CPS_IDX_CP_MEAS_VAL));
                    }
                    else
                    {
                        // Inform the application that a procedure has been completed
                        cpps_send_cmp_evt(cpps_env.con_info.prf_id, cpps_env.con_info.appid, gapc_get_conhdl(cpps_env.con_info.conidx),
                                CPPS_NTF_MEAS_OP_CODE, param->status);
                    }
                }
                else if (cpps_env.operation == CPPS_NTF_VECTOR_OP_CODE)
                {
                    // Inform the application that a procedure has been completed
                    cpps_send_cmp_evt(cpps_env.con_info.prf_id, cpps_env.con_info.appid, gapc_get_conhdl(cpps_env.con_info.conidx),
                            CPPS_NTF_VECTOR_OP_CODE, param->status);
                }
                else
                {
                    ASSERT_ERR(0);
                }

                // else ignore the message
            } break;

            case (GATTC_INDICATE):
            {
                    ASSERT_INFO(cpps_env.operation >= CPPS_CTNL_PT_SET_CUMUL_VAL_OP_CODE, 0, 0);

                    // Inform the application that a procedure has been completed
                    cpps_send_cmp_evt(cpps_env.con_info.prf_id, cpps_env.con_info.appid, gapc_get_conhdl(cpps_env.con_info.conidx),
                                       cpps_env.operation, param->status);
                // else ignore the message
            } break;

            default:
            {
                ASSERT_ERR(0);
            } break;
        }
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Disconnection indication to CPPS.
 * @param[in] msgid     Id of the message received.
 * @param[in] param     Pointer to the parameters of the message.
 * @param[in] dest_id   ID of the receiving task instance
 * @param[in] src_id    ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gapc_disconnect_ind_handler(ke_msg_id_t const msgid,
                                        struct gapc_disconnect_ind const *param,
                                        ke_task_id_t const dest_id,
                                        ke_task_id_t const src_id)
{
    // Check if a connection exists
    if (KE_IDX_GET(src_id) == cpps_env.con_info.conidx)
    {
        cpps_disable(param->conhdl);
    }

    return (KE_MSG_CONSUMED);
}

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// Specifies the default message handlers
const struct ke_msg_handler cpps_default_state[] =
{
    {CPPS_CREATE_DB_CMD,            (ke_msg_func_t)cpps_create_db_cmd_handler},
    {CPPS_ENABLE_CMD,               (ke_msg_func_t)cpps_enable_cmd_handler},

    {CPPS_NTF_CP_MEAS_CMD,          (ke_msg_func_t)cpps_ntf_cp_meas_cmd_handler},
    {CPPS_NTF_CP_VECTOR_CMD,        (ke_msg_func_t)cpps_ntf_cp_vector_cmd_handler},

    {CPPS_CTNL_PT_CFM,              (ke_msg_func_t)cpps_ctnl_pt_cfm_handler},

    {GATTC_WRITE_CMD_IND,           (ke_msg_func_t) gattc_write_cmd_ind_handler},
    {CPPS_VECTOR_CFG_CFM,           (ke_msg_func_t) cpps_vector_cfg_cfm_handler},
    {GATTC_CMP_EVT,                 (ke_msg_func_t) gattc_cmp_evt_handler},

    {GAPC_DISCONNECT_IND,           (ke_msg_func_t)gapc_disconnect_ind_handler},
};

/// Specifies the message handler structure for every input state.
const struct ke_state_handler cpps_state_handler[CPPS_STATE_MAX] =
{
    [CPPS_DISABLED]    = KE_STATE_HANDLER_NONE,
    [CPPS_IDLE]        = KE_STATE_HANDLER_NONE,
    [CPPS_CONNECTED]   = KE_STATE_HANDLER_NONE,
    [CPPS_BUSY]        = KE_STATE_HANDLER_NONE,
    [CPPS_WAIT_FOR_CFM] = KE_STATE_HANDLER_NONE,
};

/// Specifies the message handlers that are common to all states.
const struct ke_state_handler cpps_default_handler = KE_STATE_HANDLER(cpps_default_state);

/// Defines the place holder for the states of all the task instances.
ke_state_t cpps_state[CPPS_IDX_MAX] __attribute__((section("retention_mem_area0"),zero_init)); //@RETENTION MEMORY

#endif //(BLE_CSC_SENSOR)

/// @} CPPSTASK
