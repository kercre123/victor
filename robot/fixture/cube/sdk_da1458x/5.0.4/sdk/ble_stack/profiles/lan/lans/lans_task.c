/**
 ****************************************************************************************
 *
 * @file lans_task.c
 *
 * @brief Location and Navigation Profile Sensor Task Implementation.
 *
 * Copyright (C) RivieraWaves 2009-2014
 *
 * $ Rev $
 *
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @addtogroup LANSTASK
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "rwble_config.h" 

#if (BLE_LN_SENSOR)
#include "lan_common.h"
#include "gapc.h"
#include "gattc.h"
#include "gattc_task.h"
#include "atts_util.h"
#include "attm_util.h"
#include "lans.h"
#include "lans_task.h"
#include "prf_utils.h"
#include "ke_mem.h"
#include <math.h>

/*
 *  CYCLING POWER SERVICE ATTRIBUTES
 ****************************************************************************************
 */

/// Location and Navigation Sensor Service
static const att_svc_desc_t lans_lns_svc               = ATT_SVC_LOCATION_AND_NAVIGATION;

/// LN Feature characteristic value
static const struct att_char_desc lans_ln_feat_char    = ATT_CHAR(ATT_CHAR_PROP_RD,
                                                                LANP_LANS_LN_FEAT_CHAR,
                                                                ATT_CHAR_LN_FEAT);
/// Location and Speed characteristic value
static const struct att_char_desc lans_loc_speed_char  = ATT_CHAR(ATT_CHAR_PROP_NTF,
                                                                LANP_LANS_LOC_SPEED_CHAR,
                                                                ATT_CHAR_LOC_SPEED);
/// Position quality characteristic value
static const struct att_char_desc lans_pos_q_char      = ATT_CHAR(ATT_CHAR_PROP_RD,
                                                                LANP_LANS_POS_Q_CHAR,
                                                                ATT_CHAR_POS_QUALITY);
/// LN Control Point characteristic value
static const struct att_char_desc lans_ln_ctnl_pt_char = ATT_CHAR(ATT_CHAR_PROP_WR | ATT_CHAR_PROP_IND,
                                                                LANP_LANS_LN_CTNL_PT_CHAR,
                                                                ATT_CHAR_LN_CNTL_PT);
/// Navigation
static const struct att_char_desc lans_navigation_char = ATT_CHAR(ATT_CHAR_PROP_NTF,
                                                                LANP_LANS_NAVIG_CHAR,
                                                                ATT_CHAR_NAVIGATION);

/// Full LANS Database Description - Used to add attributes into the database
static const struct attm_desc lans_att_db[LNS_IDX_NB] =
{
    /// Location and Navigation Service Declaration
    [LNS_IDX_SVC]                  =   {ATT_DECL_PRIMARY_SERVICE, PERM(RD, ENABLE), sizeof(lans_lns_svc),
                                            sizeof(lans_lns_svc), (uint8_t *)&lans_lns_svc},

    /// LN Feature Characteristic Declaration
    [LNS_IDX_LN_FEAT_CHAR]         =   {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), sizeof(lans_ln_feat_char),
                                            sizeof(lans_ln_feat_char), (uint8_t *)&lans_ln_feat_char},
    /// LN Feature Characteristic Value
    [LNS_IDX_LN_FEAT_VAL]          =   {ATT_CHAR_LN_FEAT, PERM(RD, ENABLE), sizeof(uint32_t),
                                            0, NULL},

    /// Location and Speed Characteristic Declaration
    [LNS_IDX_LOC_SPEED_CHAR]       =   {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), sizeof(lans_loc_speed_char),
                                            sizeof(lans_loc_speed_char), (uint8_t *)&lans_loc_speed_char},
    /// Location and Speed Characteristic Value
    [LNS_IDX_LOC_SPEED_VAL]        =   {ATT_CHAR_LOC_SPEED, PERM(NTF, ENABLE), LANP_LAN_LOC_SPEED_MAX_LEN,
                                            0, NULL},
    /// Location and Speed Characteristic - Client Characteristic Configuration Descriptor
    [LNS_IDX_LOC_SPEED_NTF_CFG]    =   {ATT_DESC_CLIENT_CHAR_CFG, PERM(RD, ENABLE) | PERM(WR, ENABLE), sizeof(uint16_t),
                                            0, NULL},

    /// Position Quality Characteristic Declaration
    [LNS_IDX_POS_Q_CHAR]           =   {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), sizeof(lans_pos_q_char),
                                            sizeof(lans_pos_q_char), (uint8_t *)&lans_pos_q_char},
    /// Position Quality Characteristic Value
    [LNS_IDX_POS_Q_VAL]            =   {ATT_CHAR_POS_QUALITY, PERM(RD, ENABLE), LANP_LAN_POSQ_MAX_LEN,
                                            0, NULL},

    /// LN Control Point Characteristic Declaration
    [LNS_IDX_LN_CTNL_PT_CHAR]      =   {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), sizeof(lans_ln_ctnl_pt_char),
                                            sizeof(lans_ln_ctnl_pt_char), (uint8_t *)&lans_ln_ctnl_pt_char},
    /// LN Control Point Characteristic Value - The response has the maximal length
    [LNS_IDX_LN_CTNL_PT_VAL]       =   {ATT_CHAR_LN_CNTL_PT, PERM(WR, ENABLE) | PERM(IND, ENABLE), LANP_LAN_LN_CNTL_PT_RSP_MAX_LEN,
                                            0, NULL},
    /// LN Control Point Characteristic - Client Characteristic Configuration Descriptor
    [LNS_IDX_LN_CTNL_PT_IND_CFG]   =   {ATT_DESC_CLIENT_CHAR_CFG, PERM(RD, ENABLE) | PERM(WR, ENABLE), sizeof(uint16_t),
                                            0, NULL},

    /// Navigation Characteristic Declaration
    [LNS_IDX_NAVIGATION_CHAR]       =   {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), sizeof(lans_navigation_char),
                                            sizeof(lans_navigation_char), (uint8_t *)&lans_navigation_char},
    /// Navigation Characteristic Value
    [LNS_IDX_NAVIGATION_VAL]        =   {ATT_CHAR_NAVIGATION, PERM(NTF, ENABLE), LANP_LAN_NAVIGATION_MAX_LEN,
                                            0, NULL},
    /// Navigation Characteristic - Client Characteristic Configuration Descriptor
    [LNS_IDX_NAVIGATION_NTF_CFG]    =   {ATT_DESC_CLIENT_CHAR_CFG, PERM(RD, ENABLE) | PERM(WR, ENABLE), sizeof(uint16_t),
                                            0, NULL},
};

/*
 * LOCAL FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Packs location and speed notifications
 * @param[in] param Pointer to the parameters of the message.
 * @param[out] pckd_loc_speed pointer to packed message
 * @return status of the operation
 ****************************************************************************************
 */
static uint8_t lans_pack_loc_speed_ntf(struct lans_ntf_loc_speed_cmd *param, uint8_t *pckd_loc_speed)
{
    // Packed Measurement length
    uint8_t pckd_loc_speed_len = LANP_LAN_LOC_SPEED_MIN_LEN;

    // Check provided flags
    if (LANS_IS_PRESENT(param->parameters.flags, LANP_LSPEED_INST_SPEED_PRESENT))
    {
        if (LANS_IS_FEATURE_SUPPORTED(lans_env.feat_cfg, LANP_FEAT_INSTANTANEOUS_SPEED_SUPP))
        {
            //Pack instantaneous speed
            co_write16p(&pckd_loc_speed[pckd_loc_speed_len], param->parameters.inst_speed);
            pckd_loc_speed_len += 2;
        }
        else //Not supported by the profile
        {
            //Force to not supported
            param->parameters.flags &= ~LANP_LSPEED_INST_SPEED_PRESENT;
        }
    }

    if (LANS_IS_PRESENT(param->parameters.flags, LANP_LSPEED_TOTAL_DISTANCE_PRESENT))
    {
        if (LANS_IS_FEATURE_SUPPORTED(lans_env.feat_cfg, LANP_FEAT_TOTAL_DISTANCE_SUPP))
        {
            //Pack total distance (24bits)
            co_write24p(&pckd_loc_speed[pckd_loc_speed_len], param->parameters.total_dist);
            pckd_loc_speed_len += 3;
        }
        else //Not supported by the profile
        {
            //Force to not supported
            param->parameters.flags &= ~LANP_LSPEED_TOTAL_DISTANCE_PRESENT;
        }
    }

    if (LANS_IS_PRESENT(param->parameters.flags, LANP_LSPEED_LOCATION_PRESENT))
    {
        if (LANS_IS_FEATURE_SUPPORTED(lans_env.feat_cfg, LANP_FEAT_LOCATION_SUPP))
        {
            //Pack Location
            co_write32p(&pckd_loc_speed[pckd_loc_speed_len], param->parameters.latitude);
            pckd_loc_speed_len += 4;
            co_write32p(&pckd_loc_speed[pckd_loc_speed_len], param->parameters.longitude);
            pckd_loc_speed_len += 4;
        }
        else //Not supported by the profile
        {
            //Force to not supported
            param->parameters.flags &= ~LANP_LSPEED_LOCATION_PRESENT;
        }
    }

    if (LANS_IS_PRESENT(param->parameters.flags, LANP_LSPEED_ELEVATION_PRESENT))
    {
        if (LANS_IS_FEATURE_SUPPORTED(lans_env.feat_cfg, LANP_FEAT_ELEVATION_SUPP))
        {
            //Pack elevation (24 bits)
            co_write24p(&pckd_loc_speed[pckd_loc_speed_len], param->parameters.elevation);
            pckd_loc_speed_len += 3;
        }
        else //Not supported by the profile
        {
            //Force to not supported
            param->parameters.flags &= ~LANP_LSPEED_ELEVATION_PRESENT;
        }
    }

    if (LANS_IS_PRESENT(param->parameters.flags, LANP_LSPEED_HEADING_PRESENT))
    {
        if (LANS_IS_FEATURE_SUPPORTED(lans_env.feat_cfg, LANP_FEAT_HEADING_SUPP))
        {
            //Pack Extreme Force Magnitudes (Maximum Force Magnitude & Minimum Force Magnitude)
            co_write16p(&pckd_loc_speed[pckd_loc_speed_len], param->parameters.heading);
            pckd_loc_speed_len += 2;
        }
        else //Not supported by the profile
        {
            //Force to not supported
            param->parameters.flags &= ~LANP_LSPEED_HEADING_PRESENT;
        }
    }

    if (LANS_IS_PRESENT(param->parameters.flags, LANP_LSPEED_ROLLING_TIME_PRESENT))
    {
        if (LANS_IS_FEATURE_SUPPORTED(lans_env.feat_cfg, LANP_FEAT_ROLLING_TIME_SUPP))
        {
            //Pack rolling time
            pckd_loc_speed[pckd_loc_speed_len] = param->parameters.rolling_time;
            pckd_loc_speed_len++;
        }
        else //Not supported by the profile
        {
            //Force to not supported
            param->parameters.flags &= ~LANP_LSPEED_ROLLING_TIME_PRESENT;
        }
    }

    if (LANS_IS_PRESENT(param->parameters.flags, LANP_LSPEED_UTC_TIME_PRESENT))
    {
        if (LANS_IS_FEATURE_SUPPORTED(lans_env.feat_cfg, LANP_FEAT_UTC_TIME_SUPP))
        {
            // Pack UTC time
            pckd_loc_speed_len += prf_pack_date_time(&pckd_loc_speed[pckd_loc_speed_len], &(param->parameters.date_time));

        }
        else //Not supported by the profile
        {
            //Force to not supported
            param->parameters.flags &= ~LANP_LSPEED_UTC_TIME_PRESENT;
        }
    }

    // Force the unused bits of the flag value to 0
//    param->parameters.flags &= LANP_LSPEED_ALL_PRESENT;
    // Flags value
    co_write16p(&pckd_loc_speed[0], param->parameters.flags);

    return pckd_loc_speed_len;
}

/**
 ****************************************************************************************
 * @brief Splits notifications in order to be sent with default MTU
 * @param[in] pckd_locspeed packed message
 * @param[out] ntf1 first packed notification
 * @param[out] len1 length of the first notification
 * @param[out] ntf2 second packed notification
 * @param[out] len2 length of the second notification
 * @return status of the operation
 ****************************************************************************************
 */
static uint8_t lans_split_loc_speed_ntf(uint8_t *pckd_locspeed, uint8_t *ntf1, uint8_t *len1, uint8_t *ntf2, uint8_t *len2)
{
    uint16_t flags = co_read16p(&pckd_locspeed[0]), flags_cp;
    uint8_t pckd_meas_idx = LANP_LAN_LOC_SPEED_MIN_LEN, len = 0;

    // Initialize parameters
    uint8_t *buff_copy_pt = ntf1;
    uint8_t *len_copy_pt = len1;
    lans_env.ntf_len = LANP_LAN_LOC_SPEED_MIN_LEN;

    // Copy status flags
    co_write16p(&ntf1[0], (flags & (LANP_LSPEED_POSITION_STATUS_LSB |
                                    LANP_LSPEED_POSITION_STATUS_MSB |
                                    LANP_LSPEED_SPEED_AND_DISTANCE_FORMAT |
                                    LANP_LSPEED_ELEVATION_SOURCE_LSB |
                                    LANP_LSPEED_ELEVATION_SOURCE_MSB |
                                    LANP_LSPEED_HEADING_SOURCE)));

    co_write16p(&ntf2[0], (flags & (LANP_LSPEED_POSITION_STATUS_LSB |
                                    LANP_LSPEED_POSITION_STATUS_MSB |
                                    LANP_LSPEED_SPEED_AND_DISTANCE_FORMAT |
                                    LANP_LSPEED_ELEVATION_SOURCE_LSB |
                                    LANP_LSPEED_ELEVATION_SOURCE_MSB |
                                    LANP_LSPEED_HEADING_SOURCE)));

    for (uint16_t feat = LANP_LSPEED_INST_SPEED_PRESENT;
            feat <= LANP_LSPEED_POSITION_STATUS_LSB;
            feat <<= 1)
    {
        if (LANS_IS_PRESENT(flags, feat))
        {
            // Field does not fit, change pointers
            // ATT_DEFAULT_MTU is 23 equal to buffer size
            if (!(((ATT_DEFAULT_MTU - 3) - (*len_copy_pt)) >= LANP_LAN_LSPEED_MAX_PAIR_SIZE))
            {
                lans_env.ntf_pending = true;
                buff_copy_pt = ntf2;
                len_copy_pt = len2;
            }

            switch (feat)
            {
                case LANP_LSPEED_ROLLING_TIME_PRESENT:
                    // Copy uint8
                    buff_copy_pt[(*len_copy_pt)] = pckd_locspeed[pckd_meas_idx];
                    len = 1;
                    break;

                case LANP_LSPEED_INST_SPEED_PRESENT:
                case LANP_LSPEED_HEADING_PRESENT:
                    // Copy uint16
                    memcpy(&buff_copy_pt[(*len_copy_pt)], &pckd_locspeed[pckd_meas_idx], 2);
                    len = 2;
                    break;

                case LANP_LSPEED_TOTAL_DISTANCE_PRESENT:
                case LANP_LSPEED_ELEVATION_PRESENT:
                    // Copy uint24
                    memcpy(&buff_copy_pt[(*len_copy_pt)], &pckd_locspeed[pckd_meas_idx], 3);
                    len = 3;
                    break;

                case LANP_LSPEED_LOCATION_PRESENT:
                    // Copy latitude and longitude
                    memcpy(&buff_copy_pt[(*len_copy_pt)], &pckd_locspeed[pckd_meas_idx], 8);
                    len = 8;
                    break;

                case LANP_LSPEED_UTC_TIME_PRESENT:
                    // Copy time
                    memcpy(&buff_copy_pt[(*len_copy_pt)], &pckd_locspeed[pckd_meas_idx], 7);
                    len = 7;
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
 * @param[in] prfl_config requested value
 * @param[in] char_code characteristic code
 * @param[in] param pointer to the message parameters
 * @return status of the operation
 ****************************************************************************************
 */
static uint8_t lans_update_characteristic_config(uint8_t prfl_config, uint8_t char_code, struct gattc_write_cmd_ind const *param)
{
    // New configuration
    struct lans_ntf_ind_cfg_ind *ind = KE_MSG_ALLOC(LANS_NTF_IND_CFG_IND,
                                                     lans_env.con_info.appid, lans_env.con_info.prf_id,
                                                     lans_ntf_ind_cfg_ind);

    ind->conhdl    = gapc_get_conhdl(lans_env.con_info.conidx);

    // Status
    uint8_t status = PRF_ERR_OK;
    // Get the value
    uint16_t ntf_cfg = co_read16p(&param->value[0]);

    // Check if the value is correct
    if (prfl_config == LANS_PRF_CFG_FLAG_LN_CTNL_PT_IND)
    {
        if ((ntf_cfg == PRF_CLI_STOP_NTFIND) || (ntf_cfg == PRF_CLI_START_IND))
        {
            // Save the new configuration in the environment
            (ntf_cfg == PRF_CLI_STOP_NTFIND) ?
                    LANS_DISABLE_NTF_IND(prfl_config) : LANS_ENABLE_NTF_IND(prfl_config);
        }
        else
        {
            status = PRF_ERR_INVALID_PARAM;
        }
    }
    else
    {
        if ((ntf_cfg == PRF_CLI_STOP_NTFIND) || (ntf_cfg == PRF_CLI_START_NTF))
        {
            // Save the new configuration in the environment
            (ntf_cfg == PRF_CLI_STOP_NTFIND) ?
                    LANS_DISABLE_NTF_IND(prfl_config) : LANS_ENABLE_NTF_IND(prfl_config);
        }
        else
        {
            status = PRF_ERR_INVALID_PARAM;
        }
    }

    if (status == PRF_ERR_OK)
    {
        // Set the value in the database
        attmdb_att_set_value(param->handle, sizeof(uint16_t), (uint8_t *)&ntf_cfg);

        ind->char_code = char_code;
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
 * @brief Update database in non discovery modes
 * @param[in] con_type Connection type
 * @param[in] handle handle
 * @param[in] ntf_cfg provided flag
 * @param[in] cfg_flag requested flag
 * @return status of the operation
 ****************************************************************************************
 */
static int lans_update_cfg_char_value (uint8_t con_type, uint8_t handle, uint16_t ntf_cfg, uint16_t cfg_flag)
{
    uint8_t status = ATT_ERR_NO_ERROR;
    uint16_t desc_cfg = PRF_CLI_STOP_NTFIND;

    if (con_type != PRF_CON_DISCOVERY)
    {
        // Check the provided value
        if (ntf_cfg & cfg_flag)
        {
            // Store the status
            LANS_ENABLE_NTF_IND(cfg_flag);
            // Start
            desc_cfg = PRF_CLI_START_NTF;
        }
    }
    // Set the Characteristic Configuration value in the database
    status = attmdb_att_set_value(LANS_HANDLE(handle),
            sizeof(uint16_t), (uint8_t *)&desc_cfg);

    return status;
}

/**
 ****************************************************************************************
 * @brief Packs navigation notifications
 * @param[in] param Pointer to the parameters of the message.
 * @param[out] pckd_navigation pointer to packed message
 * @return length
 ****************************************************************************************
 */
static uint8_t lans_pack_navigation_ntf(struct lans_ntf_navigation_cmd *param, uint8_t *pckd_navigation)
{
    // Packed Measurement length
    uint8_t pckd_navigation_len = LANP_LAN_NAVIGATION_MIN_LEN;

    // Check provided flags
    if (LANS_IS_PRESENT(param->parameters.flags, LANP_NAVI_REMAINING_DIS_PRESENT))
    {
        if (LANS_IS_FEATURE_SUPPORTED(lans_env.feat_cfg, LANP_FEAT_REMAINING_DISTANCE_SUPP))
        {
            //Pack distance (24bits)
            co_write24p(&pckd_navigation[pckd_navigation_len], param->parameters.remaining_distance);
            pckd_navigation_len += 3;
        }
        else //Not supported by the profile
        {
            //Force to not supported
            param->parameters.flags &= ~LANP_NAVI_REMAINING_DIS_PRESENT;
        }
    }

    if (LANS_IS_PRESENT(param->parameters.flags, LANP_NAVI_REMAINING_VER_DIS_PRESENT))
    {
        if (LANS_IS_FEATURE_SUPPORTED(lans_env.feat_cfg, LANP_FEAT_REMAINING_VERTICAL_DISTANCE_SUPP))
        {
            //Pack vertical distance (24bits)
            co_write24p(&pckd_navigation[pckd_navigation_len], param->parameters.remaining_ver_distance);
            pckd_navigation_len += 3;
        }
        else //Not supported by the profile
        {
            //Force to not supported
            param->parameters.flags &= ~LANP_NAVI_REMAINING_VER_DIS_PRESENT;
        }
    }

    if (LANS_IS_PRESENT(param->parameters.flags, LANP_NAVI_ESTIMATED_TIME_OF_ARRIVAL_PRESENT))
    {
        if (LANS_IS_FEATURE_SUPPORTED(lans_env.feat_cfg, LANP_FEAT_ESTIMATED_TIME_OF_ARRIVAL_SUPP))
        {
            //Pack time
            pckd_navigation_len += prf_pack_date_time(
                    &pckd_navigation[pckd_navigation_len], &(param->parameters.estimated_arrival_time));

        }
        else //Not supported by the profile
        {
            //Force to not supported
            param->parameters.flags &= ~LANP_NAVI_ESTIMATED_TIME_OF_ARRIVAL_PRESENT;
        }
    }

    // Force the unused bits of the flag value to 0
//    param->parameters.flags &= LANP_NAVI_ALL_PRESENT;
    // Flags value
    co_write16p(&pckd_navigation[0], param->parameters.flags);
    // Bearing value
    co_write16p(&pckd_navigation[2], param->parameters.bearing);
    // heading value
    co_write16p(&pckd_navigation[4], param->parameters.heading);

    return pckd_navigation_len;
}

/**
 ****************************************************************************************
 * @brief Packs position quality
 * @param[in] param Pointer to the parameters of the message.
 * @param[out] pckd_posq pointer to packed message
 * @return length
 ****************************************************************************************
 */
static uint8_t lans_pack_posq(struct lans_upd_pos_q_cmd *param, uint8_t *pckd_posq)
{
    // Packed length
    uint8_t pckd_posq_len = LANP_LAN_POSQ_MIN_LEN;

    // Check provided flags
    if (LANS_IS_PRESENT(param->parameters.flags, LANP_POSQ_NUMBER_OF_BEACONS_IN_SOLUTION_PRESENT))
    {
        if (LANS_IS_FEATURE_SUPPORTED(lans_env.feat_cfg, LANP_FEAT_NUMBER_OF_BEACONS_IN_SOLUTION_SUPP))
        {
            //Pack beacons in solution
            pckd_posq[pckd_posq_len] = param->parameters.n_beacons_solution;
            pckd_posq_len++;
        }
        else //Not supported by the profile
        {
            //Force to not supported
            param->parameters.flags &= ~LANP_POSQ_NUMBER_OF_BEACONS_IN_SOLUTION_PRESENT;
        }
    }

    if (LANS_IS_PRESENT(param->parameters.flags, LANP_POSQ_NUMBER_OF_BEACONS_IN_VIEW_PRESENT))
    {
        if (LANS_IS_FEATURE_SUPPORTED(lans_env.feat_cfg, LANP_FEAT_NUMBER_OF_BEACONS_IN_VIEW_SUPP))
        {
            //Pack beacons in view
            pckd_posq[pckd_posq_len] = param->parameters.n_beacons_view;
            pckd_posq_len++;
        }
        else //Not supported by the profile
        {
            //Force to not supported
            param->parameters.flags &= ~LANP_POSQ_NUMBER_OF_BEACONS_IN_VIEW_PRESENT;
        }
    }

    if (LANS_IS_PRESENT(param->parameters.flags, LANP_POSQ_TIME_TO_FIRST_FIX_PRESENT))
    {
        if (LANS_IS_FEATURE_SUPPORTED(lans_env.feat_cfg, LANP_FEAT_TIME_TO_FIRST_FIX_SUPP))
        {
            //Pack time to fix
            co_write16p(&pckd_posq[pckd_posq_len], param->parameters.time_first_fix);
            pckd_posq_len += 2;
        }
        else //Not supported by the profile
        {
            //Force to not supported
            param->parameters.flags &= ~LANP_POSQ_TIME_TO_FIRST_FIX_PRESENT;
        }
    }

    if (LANS_IS_PRESENT(param->parameters.flags, LANP_POSQ_EHPE_PRESENT))
    {
        if (LANS_IS_FEATURE_SUPPORTED(lans_env.feat_cfg, LANP_FEAT_ESTIMATED_HOR_POSITION_ERROR_SUPP))
        {
            //Pack ehpe
            co_write32p(&pckd_posq[pckd_posq_len], param->parameters.ehpe);
            pckd_posq_len += 4;
        }
        else //Not supported by the profile
        {
            //Force to not supported
            param->parameters.flags &= ~LANP_POSQ_EHPE_PRESENT;
        }
    }

    if (LANS_IS_PRESENT(param->parameters.flags, LANP_POSQ_EVPE_PRESENT))
    {
        if (LANS_IS_FEATURE_SUPPORTED(lans_env.feat_cfg, LANP_FEAT_ESTIMATED_VER_POSITION_ERROR_SUPP))
        {
            //Pack ehpe
            co_write32p(&pckd_posq[pckd_posq_len], param->parameters.evpe);
            pckd_posq_len += 4;
        }
        else //Not supported by the profile
        {
            //Force to not supported
            param->parameters.flags &= ~LANP_POSQ_EVPE_PRESENT;
        }
    }

    if (LANS_IS_PRESENT(param->parameters.flags, LANP_POSQ_HDOP_PRESENT))
    {
        if (LANS_IS_FEATURE_SUPPORTED(lans_env.feat_cfg, LANP_FEAT_HOR_DILUTION_OF_PRECISION_SUPP))
        {
            //Pack ehpe
            pckd_posq[pckd_posq_len] = param->parameters.hdop;
            pckd_posq_len++;
        }
        else //Not supported by the profile
        {
            //Force to not supported
            param->parameters.flags &= ~LANP_POSQ_HDOP_PRESENT;
        }
    }

    if (LANS_IS_PRESENT(param->parameters.flags, LANP_POSQ_VDOP_PRESENT))
    {
        if (LANS_IS_FEATURE_SUPPORTED(lans_env.feat_cfg, LANP_FEAT_VER_DILUTION_OF_PRECISION_SUPP))
        {
            //Pack ehpe
            pckd_posq[pckd_posq_len] = param->parameters.vdop;
            pckd_posq_len++;
        }
        else //Not supported by the profile
        {
            //Force to not supported
            param->parameters.flags &= ~LANP_POSQ_VDOP_PRESENT;
        }
    }

    // Force the unused bits of the flag value to 0
//    param->parameters.flags &= LANP_POSQ_ALL_PRESENT;
    // Flags value
    co_write16p(&pckd_posq[0], param->parameters.flags);

    return pckd_posq_len;
}

/**
 ****************************************************************************************
 * @brief Packs control point
 * @param[in] param Pointer to the parameters of the message.
 * @param[out] rsp pointer to message
 * @return status of the operation
 ****************************************************************************************
 */
static uint8_t lans_pack_ln_ctnl_point_cfm (struct lans_ln_ctnl_pt_cfm *param, uint8_t *rsp)
{
    // Response Length (At least 3)
    uint8_t rsp_len = LANP_LAN_LN_CNTL_PT_RSP_MIN_LEN;

    // Set the Response Code
    rsp[0] = LANP_LN_CTNL_PT_RESPONSE_CODE;
    // Set the Response Value
    rsp[2] = (param->status > LANP_LN_CTNL_PT_RESP_FAILED) ? LANP_LN_CTNL_PT_RESP_FAILED : param->status;

    switch (lans_env.operation)
    {
        case (LANS_SET_CUMUL_VALUE_OP_CODE):
        {
            // Set the request operation code
            rsp[1] = LANP_LN_CTNL_PT_SET_CUMUL_VALUE;
        } break;

        case (LANS_MASK_LSPEED_CHAR_CT_OP_CODE):
        {
            // Set the request operation code
            rsp[1] = LANP_LN_CTNL_PT_MASK_LSPEED_CHAR_CT;
        } break;

        case (LANS_NAVIGATION_CONTROL_OP_CODE):
        {
            // Set the request operation code
            rsp[1] = LANP_LN_CTNL_PT_NAVIGATION_CONTROL;
            if (param->status == LANP_LN_CTNL_PT_RESP_SUCCESS)
            {
                switch(lans_env.navi_control)
                {
                    // Disable notifications
                    case LANP_LN_CTNL_STOP_NAVI:
                    case LANP_LN_CTNL_PAUSE_NAVI:
                        lans_env.navi_enabled = false;
                        break;

                    // Enable notifications
                    case LANP_LN_CTNL_START_NAVI:
                    case LANP_LN_CTNL_RESUME_NAVI:
                    case LANP_LN_CTNL_START_NST_WPT:
                        lans_env.navi_enabled = true;
                        break;

                    // Do nothing
                    case LANP_LN_CTNL_SKIP_WPT:
                    default:
                        break;
                }
            }
        } break;

        case (LANS_REQ_NUMBER_OF_ROUTES_OP_CODE):
        {
            // Set the request operation code
            rsp[1] = LANP_LN_CTNL_PT_REQ_NUMBER_OF_ROUTES;
            if (param->status == LANP_LN_CTNL_PT_RESP_SUCCESS)
            {
                co_write16p(&rsp[rsp_len], param->value.number_of_routes);
                rsp_len += 2;
            }
        } break;

        case (LANS_REQ_NAME_OF_ROUTE_OP_CODE):
        {
            // Set the request operation code
            rsp[1] = LANP_LN_CTNL_PT_REQ_NAME_OF_ROUTE;
            if (param->status == LANP_LN_CTNL_PT_RESP_SUCCESS)
            {
                // pack name of route
                for (int i=0; i<param->value.route.length; i++)
                {
                    rsp[i + 3] = param->value.route.name[i];
                    rsp_len++;
                }
            }
        } break;

        case (LANS_SELECT_ROUTE_OP_CODE):
        {
            // Set the request operation code
            rsp[1] = LANP_LN_CTNL_PT_SELECT_ROUTE;
        } break;

        case (LANS_SET_FIX_RATE_OP_CODE):
        {
            // Set the request operation code
            rsp[1] = LANP_LN_CTNL_PT_SET_FIX_RATE;
        } break;

        case (LANS_SET_ELEVATION_OP_CODE):
        {
            // Set the request operation code
            rsp[1] = LANP_LN_CTNL_PT_SET_ELEVATION;
        } break;

        default:
        {
            param->status = LANP_LN_CTNL_PT_RESP_NOT_SUPP;
        } break;
    }

    return rsp_len;
}


/**
 ****************************************************************************************
 * @brief Unpack control point and sends indication
 * @param[in] param pointer to message
 * @return status of the operation
 ****************************************************************************************
 */
static uint8_t lans_unpack_ln_ctnl_point_ind (struct gattc_write_cmd_ind const *param)
{
    // Indication Status
    uint8_t ind_status = LANP_LN_CTNL_PT_RESP_NOT_SUPP;

    // Allocate a request indication message for the application
    struct lans_ln_ctnl_pt_req_ind *req_ind = KE_MSG_ALLOC(LANS_LN_CTNL_PT_REQ_IND,
                                                            lans_env.con_info.appid, lans_env.con_info.prf_id,
                                                            lans_ln_ctnl_pt_req_ind);
    // Connection Handle
    req_ind->conhdl    = gapc_get_conhdl(lans_env.con_info.conidx);
    // Operation Code
    req_ind->op_code   = param->value[0];

    // Operation Code
    switch(req_ind->op_code)
    {
        case (LANP_LN_CTNL_PT_SET_CUMUL_VALUE):
        {
            // Check if total distance feature is supported
            if (LANS_IS_FEATURE_SUPPORTED(lans_env.feat_cfg, LANP_FEAT_TOTAL_DISTANCE_SUPP))
            {
                // Provided parameter in not within the defined range
                ind_status = LANP_LN_CTNL_PT_RESP_INV_PARAM;

                if (param->length == 4)
                {
                    // The request can be handled
                    ind_status = LANP_LN_CTNL_PT_RESP_SUCCESS;
                    // Update the environment
                    lans_env.operation = LANS_SET_CUMUL_VALUE_OP_CODE;
                    // Cumulative value
                    req_ind->value.cumul_val = co_read24p(&param->value[1]);
                }
            }
        } break;

        case (LANP_LN_CTNL_PT_MASK_LSPEED_CHAR_CT):
        {
            // Check if the LN Masking feature is supported
            if (LANS_IS_FEATURE_SUPPORTED(lans_env.feat_cfg, LANP_FEAT_LSPEED_CHAR_CT_MASKING_SUPP))
            {
                // Provided parameter in not within the defined range
                ind_status = LANP_LN_CTNL_PT_RESP_INV_PARAM;

                if (param->length == 3)
                {
                    // The request can be handled
                    ind_status = LANP_LN_CTNL_PT_RESP_SUCCESS;
                    // Update the environment
                    lans_env.operation = LANS_MASK_LSPEED_CHAR_CT_OP_CODE;
                    // Mask content
                    req_ind->value.mask_content = co_read16p(&param->value[1]);
                }
            }
        } break;

        case (LANP_LN_CTNL_PT_NAVIGATION_CONTROL):
        {
            // Check if navigation feature is supported
            if ((LANS_IS_FEATURE_SUPPORTED(lans_env.prfl_cfg, LANS_NAVI_MASK)) &&
                 (LANS_IS_NTF_IND_ENABLED(LANS_PRF_CFG_FLAG_NAVIGATION_NTF)))
            {
                // Provided parameter in not within the defined range
                ind_status = LANP_LN_CTNL_PT_RESP_INV_PARAM;

                if (param->length == 2)
                {
                    // The request can be handled
                    ind_status = LANP_LN_CTNL_PT_RESP_SUCCESS;
                    // Update the environment
                    lans_env.operation = LANS_NAVIGATION_CONTROL_OP_CODE;
                    // Control value
                    req_ind->value.control_value = param->value[1];
                    // Store value in the environment
                    lans_env.navi_control = req_ind->value.control_value;
                }
            }
        } break;

        case (LANP_LN_CTNL_PT_REQ_NUMBER_OF_ROUTES):
        {
            // Check if navigation feature is supported
            if (LANS_IS_FEATURE_SUPPORTED(lans_env.prfl_cfg, LANS_NAVI_MASK))
            {
                // The request can be handled
                ind_status = LANP_LN_CTNL_PT_RESP_SUCCESS;
                // Update the environment
                lans_env.operation = LANS_REQ_NUMBER_OF_ROUTES_OP_CODE;
            }
        } break;

        case (LANP_LN_CTNL_PT_REQ_NAME_OF_ROUTE):
        {
            // Check if navigation feature is supported
            if (LANS_IS_FEATURE_SUPPORTED(lans_env.prfl_cfg, LANS_NAVI_MASK))
            {
                // Provided parameter in not within the defined range
                ind_status = LANP_LN_CTNL_PT_RESP_INV_PARAM;

                if (param->length == 3)
                {
                    // The request can be handled
                    ind_status = LANP_LN_CTNL_PT_RESP_SUCCESS;
                    // Update the environment
                    lans_env.operation = LANS_REQ_NAME_OF_ROUTE_OP_CODE;
                    // Route number
                    req_ind->value.route_number = co_read16p(&param->value[1]);
                }
            }
        } break;

        case (LANP_LN_CTNL_PT_SELECT_ROUTE):
        {
            // Check if navigation feature is supported
            if (LANS_IS_FEATURE_SUPPORTED(lans_env.prfl_cfg, LANS_NAVI_MASK))
            {
                // Provided parameter in not within the defined range
                ind_status = LANP_LN_CTNL_PT_RESP_INV_PARAM;

                if (param->length == 3)
                {
                    // The request can be handled
                    ind_status = LANP_LN_CTNL_PT_RESP_SUCCESS;
                    // Update the environment
                    lans_env.operation = LANS_SELECT_ROUTE_OP_CODE;
                    // route number
                    req_ind->value.route_number = co_read16p(&param->value[1]);
                }
            }
        } break;

        case (LANP_LN_CTNL_PT_SET_FIX_RATE):
        {
            // Check if the LN Masking feature is supported
            if (LANS_IS_FEATURE_SUPPORTED(lans_env.feat_cfg, LANP_FEAT_FIX_RATE_SETTING_SUPP))
            {
                // Provided parameter in not within the defined range
                ind_status = LANP_LN_CTNL_PT_RESP_INV_PARAM;

                if (param->length == 2)
                {
                    // The request can be handled
                    ind_status = LANP_LN_CTNL_PT_RESP_SUCCESS;
                    // Update the environment
                    lans_env.operation = LANS_SET_FIX_RATE_OP_CODE;
                    // fix rate
                    req_ind->value.fix_rate = param->value[1];
                }
            }
        } break;

        case (LANP_LN_CTNL_PT_SET_ELEVATION):
        {
            // Check if the Chain Weight Adjustment feature is supported
            if (LANS_IS_FEATURE_SUPPORTED(lans_env.feat_cfg, LANP_FEAT_ELEVATION_SETTING_SUPP))
            {
                // Provided parameter in not within the defined range
                ind_status = LANP_LN_CTNL_PT_RESP_INV_PARAM;

                if (param->length == 4)
                {
                    // The request can be handled
                    ind_status = LANP_LN_CTNL_PT_RESP_SUCCESS;
                    // Update the environment
                    lans_env.operation = LANS_SET_ELEVATION_OP_CODE;
                    // elevation
                    req_ind->value.elevation = co_read24p(&param->value[1]);
                }
            }
        } break;

        default:
        {
            // Operation Code is invalid, status is already LAN_CTNL_PT_RESP_NOT_SUPP
        } break;
    }

    // Go to the Busy status
    ke_state_set(TASK_LANS, LANS_BUSY);

    // If no error raised, inform the application about the request
    if (ind_status == LANP_LN_CTNL_PT_RESP_SUCCESS)
    {
        // Send the request indication to the application
        ke_msg_send(req_ind);
    }
    else
    {
        // Free the allocated message
        ke_msg_free(ke_param2msg(req_ind));

        // Send an error indication
        lans_send_rsp_ind(param->handle, param->value[0], ind_status);

        lans_env.operation = LANS_LN_CTNL_ERR_IND_OP_CODE;
    }

    return ind_status;
}


/**
 ****************************************************************************************
 * @brief Handles reception of the @ref LANS_CREATE_DB_CMD message.
 * @param[in] msgid Id of the message received.
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int lans_create_db_cmd_handler(ke_msg_id_t const msgid,
                                       struct lans_create_db_req *param,
                                       ke_task_id_t const dest_id,
                                       ke_task_id_t const src_id)
{
    // Service Configuration Flag
    uint16_t cfg_flag = LANS_MANDATORY_MASK;
    // Database Creation Status
    uint8_t status;

    // Check if a Location and Navigation service has already been added in the database
    if (ke_state_get(dest_id) == LANS_DISABLED)
    {
        /*
         * Check if Position Quality shall be added.
         */
        if((LANS_IS_FEATURE_SUPPORTED(param->ln_feature, LANP_FEAT_NUMBER_OF_BEACONS_IN_SOLUTION_SUPP)) ||
           (LANS_IS_FEATURE_SUPPORTED(param->ln_feature, LANP_FEAT_NUMBER_OF_BEACONS_IN_VIEW_SUPP)) ||
           (LANS_IS_FEATURE_SUPPORTED(param->ln_feature, LANP_FEAT_TIME_TO_FIRST_FIX_SUPP)) ||
           (LANS_IS_FEATURE_SUPPORTED(param->ln_feature, LANP_FEAT_ESTIMATED_HOR_POSITION_ERROR_SUPP)) ||
           (LANS_IS_FEATURE_SUPPORTED(param->ln_feature, LANP_FEAT_ESTIMATED_VER_POSITION_ERROR_SUPP)) ||
           (LANS_IS_FEATURE_SUPPORTED(param->ln_feature, LANP_FEAT_HOR_DILUTION_OF_PRECISION_SUPP)) ||
           (LANS_IS_FEATURE_SUPPORTED(param->ln_feature, LANP_FEAT_VER_DILUTION_OF_PRECISION_SUPP)) ||
           (LANS_IS_FEATURE_SUPPORTED(param->ln_feature, LANP_FEAT_POSITION_STATUS_SUPP)))
        {
            //Add configuration to the database
            cfg_flag |= LANS_POS_Q_MASK;
        }

        /*
         * Check if the Navigation characteristic shall be added.
         */
        if ((LANS_IS_FEATURE_SUPPORTED(param->prfl_config, LANS_NAVIGATION_SUPP_FLAG)) ||
            (LANS_IS_FEATURE_SUPPORTED(param->ln_feature, LANP_FEAT_NUMBER_OF_BEACONS_IN_SOLUTION_SUPP)) ||
            (LANS_IS_FEATURE_SUPPORTED(param->ln_feature, LANP_FEAT_REMAINING_DISTANCE_SUPP)) ||
            (LANS_IS_FEATURE_SUPPORTED(param->ln_feature, LANP_FEAT_REMAINING_VERTICAL_DISTANCE_SUPP)) ||
            (LANS_IS_FEATURE_SUPPORTED(param->ln_feature, LANP_FEAT_ESTIMATED_TIME_OF_ARRIVAL_SUPP)) ||
            (LANS_IS_FEATURE_SUPPORTED(param->ln_feature, LANP_FEAT_POSITION_STATUS_SUPP)))
        {
            cfg_flag |= LANS_NAVI_MASK;
        }
        /*
         * Check if the LN Control Point characteristic shall be added
         */
        if ((LANS_IS_FEATURE_SUPPORTED(cfg_flag, LANS_NAVI_MASK)) ||
            (LANS_IS_FEATURE_SUPPORTED(param->prfl_config, LANS_CTNL_PT_CHAR_SUPP_FLAG)) ||
            (LANS_IS_FEATURE_SUPPORTED(param->ln_feature, LANP_FEAT_TOTAL_DISTANCE_SUPP)) ||
            (LANS_IS_FEATURE_SUPPORTED(param->ln_feature, LANP_FEAT_LSPEED_CHAR_CT_MASKING_SUPP)) ||
            (LANS_IS_FEATURE_SUPPORTED(param->ln_feature, LANP_FEAT_FIX_RATE_SETTING_SUPP)) ||
            ((LANS_IS_FEATURE_SUPPORTED(param->ln_feature, LANP_FEAT_ELEVATION_SUPP)) &&
            (LANS_IS_FEATURE_SUPPORTED(param->ln_feature, LANP_FEAT_ELEVATION_SETTING_SUPP))))
        {
            cfg_flag |= LANS_LN_CTNL_PT_MASK;
        }

        //Store Config flag
        lans_env.prfl_cfg = cfg_flag;
        // Add service in the database
        status = attm_svc_create_db(&lans_env.shdl, (uint8_t *)&lans_env.prfl_cfg, LNS_IDX_NB,
                                    &lans_env.hdl_offset[0], dest_id, &lans_att_db[0]);

        // Check if an error has occured
        if (status == ATT_ERR_NO_ERROR)
        {
            // Force the unused bits of the LN Feature Char value to 0
//            param->ln_feature &= LANP_FEAT_ALL_SUPP;
            // Store the value in the environment
            lans_env.feat_cfg = param->ln_feature;

            // Set the value of the LN Feature characteristic
            attmdb_att_set_value(LANS_HANDLE(LNS_IDX_LN_FEAT_VAL), sizeof(uint32_t), (uint8_t *)&param->ln_feature);

            // Disable LNS
            attmdb_svc_set_permission(lans_env.shdl, PERM(SVC, DISABLE));

            //Database has been fulfilled with success, go to idle state
            ke_state_set(TASK_LANS, LANS_IDLE);
        }
    }
    else
    {
        // Request is disallowed, a LNS has already been added
        status = PRF_ERR_REQ_DISALLOWED;
    }

    // Send complete event message to the application
    lans_send_cmp_evt(TASK_LANS, src_id, GAP_INVALID_CONHDL, LANS_CREATE_DB_REQ_OP_CODE, status);

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref LANS_ENABLE_CMD message.
 * @param[in] msgid Id of the message received
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */

static int lans_enable_cmd_handler(ke_msg_id_t const msgid,
                                    struct lans_enable_req *param,
                                    ke_task_id_t const dest_id,
                                    ke_task_id_t const src_id)
{
    // Status
    uint8_t status = PRF_ERR_OK;

    // Check the current state of the task and if the provided connection handle is valid
    if ((ke_state_get(TASK_LANS) == LANS_IDLE) &&
        (gapc_get_conidx(param->conhdl) != GAP_INVALID_CONIDX))
    {
        // Fill the Connection Information structure
        lans_env.con_info.conidx = gapc_get_conidx(param->conhdl);
        lans_env.con_info.prf_id = dest_id;
        lans_env.con_info.appid  = src_id;

        // Reset the store Client Characteristic Configuration status
        lans_env.prfl_ntf_ind_cfg = 0;

        // Set the Location and speed Char. Client Characteristic Configuration value
        if (lans_update_cfg_char_value(param->con_type, LNS_IDX_LOC_SPEED_NTF_CFG,
                param->prfl_ntf_ind_cfg, LANS_PRF_CFG_FLAG_LOC_SPEED_NTF) != ATT_ERR_NO_ERROR)
        {
            status = PRF_ERR_NOT_WRITABLE;
        }

        // If supported, set the LN Control Point Char. Client Characteristic Configuration value
        if (LANS_IS_FEATURE_SUPPORTED(lans_env.prfl_cfg, LANS_LN_CTNL_PT_MASK))
        {
            if (lans_update_cfg_char_value(param->con_type, LNS_IDX_LN_CTNL_PT_IND_CFG,
                    param->prfl_ntf_ind_cfg, LANS_PRF_CFG_FLAG_LN_CTNL_PT_IND) != ATT_ERR_NO_ERROR)
            {
                status = PRF_ERR_NOT_WRITABLE;
            }
        }

        // If supported, set the Navigation Char. Client Characteristic Configuration value
        if (LANS_IS_FEATURE_SUPPORTED(lans_env.prfl_cfg, LANS_NAVI_MASK))
        {
            if (lans_update_cfg_char_value(param->con_type, LNS_IDX_NAVIGATION_NTF_CFG,
                    param->prfl_ntf_ind_cfg, LANS_PRF_CFG_FLAG_NAVIGATION_NTF) != ATT_ERR_NO_ERROR)
            {
                status = PRF_ERR_NOT_WRITABLE;
            }
        }

        if (status == PRF_ERR_OK)
        {
            //Store the Notify/Indication config
            lans_env.prfl_ntf_ind_cfg = param->prfl_ntf_ind_cfg;

            // Enable LNS
            attmdb_svc_set_permission(lans_env.shdl, param->sec_lvl);

            // Go to Connected State
            ke_state_set(dest_id, LANS_CONNECTED);
        }
    }
    else
    {
        // The request is disallowed (profile already enabled for this connection, or not enough memory, ...)
        status = PRF_ERR_REQ_DISALLOWED;
    }

    // Send response to application
    lans_send_cmp_evt(dest_id, src_id, param->conhdl, LANS_ENABLE_REQ_OP_CODE, status);

    return (KE_MSG_CONSUMED);
}


/**
 ****************************************************************************************
 * @brief Handles reception of the @ref LANS_NTF_LN_MEAS_CMD message.
 * @param[in] msgid Id of the message received.
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance.
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int lans_ntf_loc_speed_cmd_handler(ke_msg_id_t const msgid,
                                          struct lans_ntf_loc_speed_cmd *param,
                                          ke_task_id_t const dest_id,
                                          ke_task_id_t const src_id)
{
    // Message status
    uint8_t msg_status = KE_MSG_CONSUMED;
    // State
    ke_state_t state   = ke_state_get(TASK_LANS);

    // State shall be Connected or Busy
    if (state >= LANS_CONNECTED)
    {
        // Status
        uint8_t status = PRF_ERR_OK;

        do
        {
            // Check the connection handle
            if (gapc_get_conidx(param->conhdl) != lans_env.con_info.conidx)
            {
                status = PRF_ERR_INVALID_PARAM;
                break;
            }

            // Check if sending of notifications has been enabled
            if (!LANS_IS_NTF_IND_ENABLED(LANS_PRF_CFG_FLAG_LOC_SPEED_NTF))
            {
                status = LAN_ERROR_IMPROPERLY_CONFIGURED;
                break;
            }

            // Check the current state of the task
            if (state == LANS_BUSY)
            {
                // Keep the message for later, status will be PRF_ERR_OK
                msg_status = KE_MSG_SAVED;
                break;
            }

            ASSERT_ERR(lans_env.operation == LANS_RESERVED_OP_CODE);

            // Location and speed array declaration
            uint8_t pckd_loc_speed[LANP_LAN_LOC_SPEED_MAX_LEN];
            // Pack Location and speed
            uint8_t nb =  lans_pack_loc_speed_ntf(param, pckd_loc_speed);
            // Get current MTU
            uint8_t mtu = gattc_get_mtu(lans_env.con_info.conidx);

            // Split notification if necessary
            if (nb > (mtu - 3))
            {
                // Maximize first notification with MTU value
                uint8_t *ntf1 = (uint8_t *) ke_malloc(mtu-3, KE_MEM_NON_RETENTION);
                uint8_t len1 = LANP_LAN_LOC_SPEED_MIN_LEN;
            
                // Split notification
                lans_split_loc_speed_ntf(pckd_loc_speed, ntf1, &len1, lans_env.ntf, &lans_env.ntf_len);

                // Set the value in the database
                attmdb_att_set_value(LANS_HANDLE(LNS_IDX_LOC_SPEED_VAL), len1, (uint8_t *)&ntf1[0]);
                // Send the notification
                prf_server_send_event((prf_env_struct *)&lans_env, false, LANS_HANDLE(LNS_IDX_LOC_SPEED_VAL));
                // Free memory
                ke_free((uint8_t *)ntf1);
            }
            else
            {
                // Set the value in the database
                attmdb_att_set_value(LANS_HANDLE(LNS_IDX_LOC_SPEED_VAL), nb, (uint8_t *)&pckd_loc_speed[0]);
                // Send the notification
                prf_server_send_event((prf_env_struct *)&lans_env, false, LANS_HANDLE(LNS_IDX_LOC_SPEED_VAL));
            }

            // Configure the environment
            lans_env.operation = LANS_NTF_LOC_SPEED_OP_CODE;
            // Go to busy state
            ke_state_set(TASK_LANS, LANS_BUSY);
        } while (0);

        if (status != PRF_ERR_OK)
        {
            // Send response to application
            lans_send_cmp_evt(lans_env.con_info.prf_id, lans_env.con_info.appid, gapc_get_conhdl(lans_env.con_info.conidx),
                    LANS_NTF_LOC_SPEED_OP_CODE, status);
        }
    }
    else
    {
        // The profile has not been enabled for a connection or the database has not been created
        lans_send_cmp_evt(dest_id, src_id, param->conhdl, LANS_NTF_LOC_SPEED_OP_CODE, PRF_ERR_REQ_DISALLOWED);
    }

    return (int)msg_status;
}



/**
 ****************************************************************************************
 * @brief Handles reception of the @ref LANS_UPD_POS_Q_CMD message.
 * @param[in] msgid Id of the message received.
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance.
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int lans_upd_pos_q_cmd_handler(ke_msg_id_t const msgid,
                                          struct lans_upd_pos_q_cmd *param,
                                          ke_task_id_t const dest_id,
                                          ke_task_id_t const src_id)
{
    // Status
    uint8_t status = PRF_ERR_INVALID_PARAM;

    // Check if Position Quality Char. has been added in the database
    if (LANS_IS_FEATURE_SUPPORTED(lans_env.prfl_cfg, LANS_POS_Q_MASK))
    {
        // position quality array declaration
        uint8_t pckd_posq[LANP_LAN_POSQ_MAX_LEN];
        // Pack Position quality
        uint8_t nb =  lans_pack_posq(param, pckd_posq);
        // Set the value in the database
        status = attmdb_att_set_value(LANS_HANDLE(LNS_IDX_POS_Q_VAL), nb, (uint8_t *)&pckd_posq[0]);
    }
    else
    {
        status = PRF_ERR_FEATURE_NOT_SUPPORTED;
    }

    // Send response to application
    lans_send_cmp_evt(lans_env.con_info.prf_id, lans_env.con_info.appid, gapc_get_conhdl(lans_env.con_info.conidx),
            LANS_UPD_POS_Q_OP_CODE, status);

    return (KE_MSG_CONSUMED);
}





/**
 ****************************************************************************************
 * @brief Handles reception of the @ref LANS_NTF_NAVIGATION_CMD message.
 * @param[in] msgid Id of the message received.
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance.
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int lans_ntf_navigation_cmd_handler(ke_msg_id_t const msgid,
                                          struct lans_ntf_navigation_cmd *param,
                                          ke_task_id_t const dest_id,
                                          ke_task_id_t const src_id)
{
    // Message status
    uint8_t msg_status = KE_MSG_CONSUMED;
    // State
    ke_state_t state   = ke_state_get(TASK_LANS);

    // State shall be Connected or Busy
    if (state >= LANS_CONNECTED)
    {
        // Status
        uint8_t status = PRF_ERR_OK;

        do
        {
            // Check the connection handle
            if (gapc_get_conidx(param->conhdl) != lans_env.con_info.conidx)
            {
                status = PRF_ERR_INVALID_PARAM;
                break;
            }

            // Check if sending of notifications has been enabled
            if ((!LANS_IS_NTF_IND_ENABLED(LANS_PRF_CFG_FLAG_NAVIGATION_NTF)) ||
                    (lans_env.navi_enabled != true))
            {
                status = LAN_ERROR_IMPROPERLY_CONFIGURED;
                break;
            }

            // Check the current state of the task
            if (state == LANS_BUSY)
            {
                // Keep the message for later, status will be PRF_ERR_OK
                msg_status = KE_MSG_SAVED;
                break;
            }

            ASSERT_ERR(lans_env.operation == LANS_RESERVED_OP_CODE);

            // Navigation array declaration
            uint8_t pckd_navigation[LANP_LAN_NAVIGATION_MAX_LEN];
            // Pack Navigation
            uint8_t nb = lans_pack_navigation_ntf(param, pckd_navigation);
            // Set the value in the database
            attmdb_att_set_value(LANS_HANDLE(LNS_IDX_NAVIGATION_VAL), nb, (uint8_t *)&pckd_navigation[0]);

            // Send the notification
            prf_server_send_event((prf_env_struct *)&lans_env, false, LANS_HANDLE(LNS_IDX_NAVIGATION_VAL));

            // Configure the environment
            lans_env.operation = LANS_NTF_NAVIGATION_OP_CODE;
            // Go to busy state
            ke_state_set(TASK_LANS, LANS_BUSY);
        } while (0);

        if (status != PRF_ERR_OK)
        {
            // Send response to application
            lans_send_cmp_evt(lans_env.con_info.prf_id, lans_env.con_info.appid, gapc_get_conhdl(lans_env.con_info.conidx),
                    LANS_NTF_NAVIGATION_OP_CODE, status);
        }
    }
    else
    {
        // The profile has not been enabled for a connection or the database has not been created
        lans_send_cmp_evt(dest_id, src_id, param->conhdl, LANS_NTF_NAVIGATION_OP_CODE, PRF_ERR_REQ_DISALLOWED);
    }

    return (int)msg_status;
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref LANS_LN_CTNL_PT_CFM message.
 * @param[in] msgid Id of the message received.
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance.
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int lans_ln_ctnl_pt_cfm_handler(ke_msg_id_t const msgid,
                                          struct lans_ln_ctnl_pt_cfm *param,
                                          ke_task_id_t const dest_id,
                                          ke_task_id_t const src_id)
{
    // Control Point Response
    uint8_t rsp[LANP_LAN_LN_CNTL_PT_RSP_MAX_LEN];

    do
    {
        // Check the current operation
        if (lans_env.operation < LANS_SET_CUMUL_VALUE_OP_CODE)
        {
            // The confirmation has been sent without request indication, ignore
            break;
        }

        // The SC Control Point Characteristic must be supported if we are here
        ASSERT_ERR(lans_env.hdl_offset[LANP_LANS_LN_CTNL_PT_CHAR] != 0x00);

        // Pack Control Point confirmation
        uint8_t nb = lans_pack_ln_ctnl_point_cfm (param, rsp);

        // Set the value in the database - If we are here the handle is valid
        attmdb_att_set_value(LANS_HANDLE(LNS_IDX_LN_CTNL_PT_VAL),
                             nb, (uint8_t *)&rsp);

        // Send the response indication to the peer device
        prf_server_send_event((prf_env_struct *)&lans_env, true,
                              LANS_HANDLE(LNS_IDX_LN_CTNL_PT_VAL));

        // Wait for Confirm
        ke_state_set(TASK_LANS, LANS_WAIT_FOR_CFM);

    } while (0);

    return (KE_MSG_CONSUMED);
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
    if((ke_state_get(TASK_LANS) == LANS_WAIT_FOR_CFM)
        && (KE_TYPE_GET(src_id) == TASK_GATTC))
    {
        // reschedule message once to ensure that indication reception is not on-going.
        ke_msg_forward(param, dest_id, dest_id);

        // Message forwarded do not free it
        msg_status = KE_MSG_NO_FREE;
    }

    // Check the connection handle
    else if (ke_state_get(TASK_LANS) >= LANS_CONNECTED)
    {
        // Location and speed Characteristic, Client Characteristic Configuration Descriptor
        if (param->handle == (LANS_HANDLE(LNS_IDX_LOC_SPEED_NTF_CFG)))
        {
            // Update value
            status = lans_update_characteristic_config(LANS_PRF_CFG_FLAG_LOC_SPEED_NTF,
                    LANP_LANS_LOC_SPEED_CHAR, param);
        }

        // Navigation Characteristic, Client Characteristic Configuration Descriptor
        else if (param->handle == (LANS_HANDLE(LNS_IDX_NAVIGATION_NTF_CFG)))
        {
            ASSERT_ERR(lans_env.hdl_offset[LANP_LANS_NAVIG_CHAR] != 0x00);

            // Update value
            status = lans_update_characteristic_config(LANS_PRF_CFG_FLAG_NAVIGATION_NTF,
                    LANP_LANS_NAVIG_CHAR, param);
        }
        // LN Control Point Characteristic
        else
        {
            ASSERT_ERR(lans_env.hdl_offset[LANP_LANS_LN_CTNL_PT_CHAR] != 0x00);

            // LN Control Point, Client Characteristic Configuration Descriptor
            if (param->handle == (LANS_HANDLE(LNS_IDX_LN_CTNL_PT_IND_CFG)))
            {
                // Update value
                status = lans_update_characteristic_config(LANS_PRF_CFG_FLAG_LN_CTNL_PT_IND,
                        LANP_LANS_LN_CTNL_PT_CHAR, param);
            }
            // LN Control Point Characteristic
            else if (param->handle == (LANS_HANDLE(LNS_IDX_LN_CTNL_PT_VAL)))
            {
                do
                {
                    // Check if sending of indications has been enabled
                    if (!LANS_IS_NTF_IND_ENABLED(LANS_PRF_CFG_FLAG_LN_CTNL_PT_IND))
                    {
                        // CCC improperly configured
                        status = LAN_ERROR_IMPROPERLY_CONFIGURED;
                        break;
                    }

                    if (lans_env.operation >= LANS_SET_CUMUL_VALUE_OP_CODE)
                    {
                        // A procedure is already in progress
                        status = LAN_ERROR_PROC_IN_PROGRESS;
                        break;
                    }

                    if (lans_env.operation == LANS_NTF_LOC_SPEED_OP_CODE)
                    {
                        // Keep the message until the end of the current procedure
                        msg_status = KE_MSG_SAVED;
                        break;
                    }

                    // Unpack Control Point parameters
                    lans_unpack_ln_ctnl_point_ind (param);

                } while (0);
            }
            else
            {
                ASSERT_ERR(0);
            }
        }
        // Send the write response to the peer device
        atts_write_rsp_send(lans_env.con_info.conidx, param->handle, status);
    }
    // else drop the message

    return (int)msg_status;
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
    if (ke_state_get(TASK_LANS) >= LANS_CONNECTED)
    {
        switch (param->req_type)
        {
            case (GATTC_NOTIFY):
            {
                if (lans_env.operation == LANS_NTF_LOC_SPEED_OP_CODE)
                {
                    // Notification pending?
                    if (lans_env.ntf_pending)
                    {
                        lans_env.ntf_pending = false;
                        // Set the value in the database
                        attmdb_att_set_value(LANS_HANDLE(LNS_IDX_LOC_SPEED_VAL), lans_env.ntf_len, (uint8_t *)&lans_env.ntf[0]);
                        // Send the notification
                        prf_server_send_event((prf_env_struct *)&lans_env, false, LANS_HANDLE(LNS_IDX_LOC_SPEED_VAL));
                    }
                    else
                    {
                        // Inform the application that a procedure has been completed
                        lans_send_cmp_evt(lans_env.con_info.prf_id, lans_env.con_info.appid, gapc_get_conhdl(lans_env.con_info.conidx),
                                LANS_NTF_LOC_SPEED_OP_CODE, param->status);
                    }
                }
                else if (lans_env.operation == LANS_NTF_NAVIGATION_OP_CODE)
                {
                    // Inform the application that a procedure has been completed
                    lans_send_cmp_evt(lans_env.con_info.prf_id, lans_env.con_info.appid, gapc_get_conhdl(lans_env.con_info.conidx),
                            LANS_NTF_NAVIGATION_OP_CODE, param->status);
                }
                else
                {
                    ASSERT_ERR(0);
                }
                // else ignore the message
            } break;

            case (GATTC_INDICATE):
            {
                    ASSERT_ERR(lans_env.operation >= LANS_SET_CUMUL_VALUE_OP_CODE);

                    // Inform the application that a procedure has been completed
                    lans_send_cmp_evt(lans_env.con_info.prf_id, lans_env.con_info.appid, gapc_get_conhdl(lans_env.con_info.conidx),
                                       lans_env.operation, param->status);
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
 * @brief Disconnection indication to LANS.
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
    if (KE_IDX_GET(src_id) == lans_env.con_info.conidx)
    {
        lans_disable(param->conhdl);
    }

    return (KE_MSG_CONSUMED);
}

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// Specifies the default message handlers
const struct ke_msg_handler lans_default_state[] =
{
    {LANS_CREATE_DB_CMD,            (ke_msg_func_t)lans_create_db_cmd_handler},
    {LANS_ENABLE_CMD,               (ke_msg_func_t)lans_enable_cmd_handler},

    {LANS_NTF_LOC_SPEED_CMD,        (ke_msg_func_t)lans_ntf_loc_speed_cmd_handler},
    {LANS_UPD_POS_Q_CMD,            (ke_msg_func_t)lans_upd_pos_q_cmd_handler},
    {LANS_NTF_NAVIGATION_CMD,       (ke_msg_func_t)lans_ntf_navigation_cmd_handler},

    {LANS_LN_CTNL_PT_CFM,           (ke_msg_func_t)lans_ln_ctnl_pt_cfm_handler},

    {GATTC_WRITE_CMD_IND,           (ke_msg_func_t) gattc_write_cmd_ind_handler},
    {GATTC_CMP_EVT,                 (ke_msg_func_t) gattc_cmp_evt_handler},

    {GAPC_DISCONNECT_IND,           (ke_msg_func_t)gapc_disconnect_ind_handler},
};

/// Specifies the message handler structure for every input state.
const struct ke_state_handler lans_state_handler[LANS_STATE_MAX] =
{
    [LANS_DISABLED]     = KE_STATE_HANDLER_NONE,
    [LANS_IDLE]         = KE_STATE_HANDLER_NONE,
    [LANS_CONNECTED]    = KE_STATE_HANDLER_NONE,
    [LANS_BUSY]         = KE_STATE_HANDLER_NONE,
    [LANS_WAIT_FOR_CFM] = KE_STATE_HANDLER_NONE,
};

/// Specifies the message handlers that are common to all states.
const struct ke_state_handler lans_default_handler = KE_STATE_HANDLER(lans_default_state);

/// Defines the place holder for the states of all the task instances.
ke_state_t lans_state[LANS_IDX_MAX] __attribute__((section("retention_mem_area0"),zero_init)); //@RETENTION MEMORY

#endif //(BLE_LN_SENSOR)

/// @} LANSTASK
