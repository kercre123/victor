/**
 ****************************************************************************************
 *
 * @file bcss.c
 *
 * @brief Body Composition Service Server Implementation.
 *
 * Copyright (C) 2015 Dialog Semiconductor Ltd, unpublished work. This computer
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
 * @addtogroup BCSS
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"

#if (BLE_BCS_SERVER)

#include "ke_mem.h"
#include "gap.h"
#include "gattc_task.h"
#include "attm_util.h"
#include "prf_utils.h"
#include "bcss.h"
#include "bcss_task.h"
#include "gapc.h"

/*
 * BCS ATTRIBUTES DEFINITION
 ****************************************************************************************
 */

/// Body Composition Service
const att_svc_desc_t bcs_svc = ATT_SVC_BODY_COMPOSITION;

/// Body Composition Feature Characteristic
const struct att_char_desc bcss_feat_char = ATT_CHAR(ATT_CHAR_PROP_RD, 0,
                                                        ATT_CHAR_BCS_FEATURE);

/// Body Composition Measurements Characteristic
const struct att_char_desc bcss_meas_char = ATT_CHAR(ATT_CHAR_PROP_IND, 0,
                                                            ATT_CHAR_BCS_MEAS);

/// Full BCS Database Description - Used to add attributes into the database
struct attm_desc bcss_att_db[BCSS_IDX_NB] =
{
    // Body Composition Service Declaration
    [BCSS_IDX_SVC]                 =   {ATT_DECL_PRIMARY_SERVICE, PERM(RD, ENABLE), sizeof(bcs_svc),
                                        sizeof(bcs_svc), (uint8_t *)&bcs_svc},

    [BCSS_IDX_FEAT_CHAR]           =   {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), sizeof(bcss_feat_char),
                                        sizeof(bcss_feat_char), (uint8_t *)&bcss_feat_char},
    [BCSS_IDX_FEAT_VAL]            =   {ATT_CHAR_BCS_FEATURE, PERM(RD, ENABLE), sizeof(uint32_t),
                                        0, NULL},

    [BCSS_IDX_MEAS_CHAR]           =   {ATT_DECL_CHARACTERISTIC, PERM(RD, ENABLE), sizeof(bcss_meas_char),
                                        sizeof(bcss_meas_char), (uint8_t *)&bcss_meas_char},
    [BCSS_IDX_MEAS_VAL]            =   {ATT_CHAR_BCS_MEAS, PERM(IND, ENABLE), sizeof(bcs_meas_t),
                                        0, NULL},
    [BCSS_IDX_MEAS_IND_CFG]        =   {ATT_DESC_CLIENT_CHAR_CFG, PERM(RD, ENABLE) | PERM(WR, ENABLE), sizeof(uint16_t),
                                        0, NULL},
};

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// Body Composition Service Server environment variable
struct bcss_env_tag bcss_env __attribute__((section("retention_mem_area0"),zero_init)); //@RETENTION MEMORY

/// BCSS task descriptor
static const struct ke_task_desc TASK_DESC_BCSS = {bcss_state_handler, &bcss_default_handler, bcss_state, BCSS_STATE_MAX, BCSS_IDX_MAX};

/*
 * STATIC FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Helper to get additional MTU size requirements for the specific features
 * @param[in] flag Feature flag.
 * @return Feature's additional space requirements.
 ****************************************************************************************
 */
static uint8_t get_feature_size(uint16_t flag)
{
    uint8_t size = 0;

    switch (flag)
    {
        case BCM_FLAG_UNIT_IMPERIAL:
            size = 0;
            break;
        case BCM_FLAG_TIME_STAMP:
            size = 7;
            break;
        case BCM_FLAG_USER_ID:
            size = 1;
            break;
        case BCM_FLAG_BASAL_METABOLISM:
            size = 2;
            break;
        case BCM_FLAG_MUSCLE_PERCENTAGE:
            size = 2;
            break;
        case BCM_FLAG_MUSCLE_MASS:
            size = 2;
            break;
        case BCM_FLAG_FAT_FREE_MASS:
            size = 2;
            break;
        case BCM_FLAG_SOFT_LEAN_MASS:
            size = 2;
            break;
        case BCM_FLAG_BODY_WATER_MASS:
            size = 2;
            break;
        case BCM_FLAG_IMPEDANCE:
            size = 2;
            break;
        case BCM_FLAG_WEIGHT:
            size = 2;
            break;
        case BCM_FLAG_HEIGHT:
            size = 2;
            break;
        case BCM_FLAG_MULTIPLE_PACKET:
            size = 0;
            break;
        default:
            break;
    }

    return size;
}

/**
 ****************************************************************************************
 * @brief Helper function to translate measurement flags to BCS features.
 * @param[in] flags Measurement flags.
 * @return BCS Features flag.
 ****************************************************************************************
 */
static uint16_t flags_to_features(uint16_t flags)
{
    uint16_t feat = 0x00;

    if (flags & BCM_FLAG_BASAL_METABOLISM)
        feat |= BCS_FEAT_BASAL_METABOLISM;

    if (flags & BCM_FLAG_MUSCLE_PERCENTAGE)
        feat |= BCS_FEAT_MUSCLE_PERCENTAGE;

    if (flags & BCM_FLAG_MUSCLE_MASS)
        feat |= BCS_FEAT_MUSCLE_MASS;

    if (flags & BCM_FLAG_FAT_FREE_MASS)
        feat |= BCS_FEAT_FAT_FREE_MASS;

    if (flags & BCM_FLAG_SOFT_LEAN_MASS)
        feat |= BCS_FEAT_SOFT_LEAN_MASS;

    if (flags & BCM_FLAG_BODY_WATER_MASS)
        feat |= BCS_FEAT_BODY_WATER_MASS;

    if (flags & BCM_FLAG_IMPEDANCE)
        feat |= BCS_FEAT_IMPEDANCE;

    if (flags & BCM_FLAG_WEIGHT)
        feat |= BCS_FEAT_WEIGHT;

    if (flags & BCM_FLAG_HEIGHT)
        feat |= BCS_FEAT_HEIGHT;

    return feat;
}

/**
 ****************************************************************************************
 * @brief Helper function to check if value fits the available free space in PDU.
 * @param[in] buf Pointer to the whole buffer for packed measurement.
 * @param[in] length Whole packed measurement buffer size.
 * @param[in] val Pointer to empty space in buffer for the packed measurement field.
 * @param[in] flags Measurement flag indicating currently packed.
 * @param[in] meas Measurement to be packed.
 * @return BCS Features flag.
 ****************************************************************************************
 */
static uint16_t verify_value_pack(const uint8_t *buf, uint16_t length, uint8_t **val,
                                                    uint16_t flag, const bcs_meas_t *meas)
{
    // Check if packed value exceeds the available free space
    if (*val + get_feature_size(flag) > buf + length)
    {
        // Set bit for the feature to be sent in next packet
        bcss_env.ind_cont_feat |= flags_to_features(flag);
        return 0;
    }

    // Turn the bit off as feature fits the mtu
    bcss_env.ind_cont_feat &= ~flags_to_features(flag);
    return flag;
}

/**
 ****************************************************************************************
 * @brief Helper function that splits and pack measurements into multiple packets.
 * @param[in] flags Measurement flags.
 * @param[in] meas Measurement that should be packed before sending.
 * @param[out] value Buffer for storing the packed measurement value.
 * @param[in] length Packed Measurement buffer size.
 * @return Length of the packed measurement value.
 ****************************************************************************************
 */
static uint8_t pack_attribute_value(uint16_t bcf, const bcs_meas_t *meas,
                                                uint8_t *value, uint16_t length)
{
    uint8_t *ptr = &value[2];
    uint16_t flags = 0;

    if (meas->measurement_unit == BCS_UNIT_IMPERIAL) {
            flags |= BCM_FLAG_UNIT_IMPERIAL;
    }

    // Set flag and get features to be sent if its indication continuation
    if (bcss_env.ind_cont_feat)
    {
        flags |= BCM_FLAG_MULTIPLE_PACKET;
    }

    co_write16p(ptr, meas->body_fat_percentage);
    ptr += sizeof(uint16_t);

    if (!bcss_env.ind_cont_feat && (bcf & BCS_FEAT_TIME_STAMP)) {
            flags |= verify_value_pack(value, length, &ptr, BCM_FLAG_TIME_STAMP, meas);
            ptr += prf_pack_date_time(ptr, &meas->time_stamp);
    }

    if (!bcss_env.ind_cont_feat && (bcf & BCS_FEAT_MULTIPLE_USERS)) {
            flags |= verify_value_pack(value, length, &ptr, BCM_FLAG_USER_ID, meas);
            co_write8(ptr, meas->user_id);
            ptr += sizeof(uint8_t);
    }

    if ((bcf & BCS_FEAT_BASAL_METABOLISM) && meas->basal_metabolism > 0) {
            flags |= verify_value_pack(value, length, &ptr, BCM_FLAG_BASAL_METABOLISM, meas);
            co_write16p(ptr, meas->basal_metabolism);
            ptr += sizeof(uint16_t);
    }

    if ((bcf & BCS_FEAT_MUSCLE_PERCENTAGE) && meas->muscle_percentage > 0) {
            flags |= verify_value_pack(value, length, &ptr, BCM_FLAG_MUSCLE_PERCENTAGE, meas);
            co_write16p(ptr, meas->muscle_percentage);
            ptr += sizeof(uint16_t);
    }

    if ((bcf & BCS_FEAT_MUSCLE_MASS) && meas->muscle_mass > 0) {
            flags |= verify_value_pack(value, length, &ptr, BCM_FLAG_MUSCLE_MASS, meas);
            co_write16p(ptr, meas->muscle_mass);
            ptr += sizeof(uint16_t);
    }

    if ((bcf & BCS_FEAT_FAT_FREE_MASS) && meas->fat_free_mass > 0) {
            flags |= verify_value_pack(value, length, &ptr, BCM_FLAG_FAT_FREE_MASS, meas);
            co_write16p(ptr, meas->fat_free_mass);
            ptr += sizeof(uint16_t);
    }

    if ((bcf & BCS_FEAT_SOFT_LEAN_MASS) && meas->soft_lean_mass > 0) {
            flags |= verify_value_pack(value, length, &ptr, BCM_FLAG_SOFT_LEAN_MASS, meas);
            co_write16p(ptr, meas->soft_lean_mass);
            ptr += sizeof(uint16_t);
    }

    if ((bcf & BCS_FEAT_BODY_WATER_MASS) && meas->body_water_mass > 0) {
            flags |= verify_value_pack(value, length, &ptr, BCM_FLAG_BODY_WATER_MASS, meas);
            co_write16p(ptr, meas->body_water_mass);
            ptr += sizeof(uint16_t);
    }

    if ((bcf & BCS_FEAT_IMPEDANCE) && meas->impedance > 0) {
            flags |= verify_value_pack(value, length, &ptr, BCM_FLAG_IMPEDANCE, meas);
            co_write16p(ptr, meas->impedance);
            ptr += sizeof(uint16_t);
    }

    if ((bcf & BCS_FEAT_WEIGHT) && meas->weight > 0) {
            flags |= verify_value_pack(value, length, &ptr, BCM_FLAG_WEIGHT, meas);
            co_write16p(ptr, meas->weight);
            ptr += sizeof(uint16_t);
    }

    if ((bcf & BCS_FEAT_HEIGHT) && meas->height > 0) {
            flags |= verify_value_pack(value, length, &ptr, BCM_FLAG_HEIGHT, meas);
            co_write16p(ptr, meas->height);
            ptr += sizeof(uint16_t);
    }

    // Update flag for indication continuation packets
    if (bcss_env.ind_cont_feat)
    {
        flags |= BCM_FLAG_MULTIPLE_PACKET;
    }

    co_write16p(value, flags);

    return ptr - value;
}

/*
 * GLOBAL FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */

void bcss_init(void)
{
    // Reset environment
    memset(&bcss_env, 0, sizeof(bcss_env));

    // Create bcss task
    ke_task_create(TASK_BCSS, &TASK_DESC_BCSS);

    // Set task in disabled state
    ke_state_set(TASK_BCSS, BCSS_DISABLED);
}

void bcss_ind_cfm_send(uint8_t status)
{
    struct bcss_meas_val_ind_cfm *cfm = KE_MSG_ALLOC(BCSS_MEAS_VAL_IND_CFM, bcss_env.con_info.appid,
                                                        TASK_BCSS, bcss_meas_val_ind_cfm);

    cfm->conhdl = gapc_get_conhdl(bcss_env.con_info.conidx);
    cfm->status = status;

    ke_msg_send(cfm);
}

void bcss_indicate(const bcs_meas_t *meas, uint16_t features, uint16_t mtu)
{
    att_size_t att_length, ccc_length;
    uint8_t att_value[mtu - 3];
    uint16_t *ccc_value;
    uint8_t status = PRF_ERR_OK;

    if(meas == NULL)
    {
        status = ATT_ERR_APP_ERROR;
        goto failed;
    }

    // Copy the measurement before slicing it for MTU-3 sized chunks
    if (bcss_env.meas == NULL)
    {
        bcss_env.meas = ke_malloc(sizeof(bcs_meas_t), KE_MEM_NON_RETENTION);
        *bcss_env.meas = *meas;
    }

    // Pack value before writing
    att_length = pack_attribute_value(features, meas, att_value, mtu - 3);

    // Update Measurement value in DB
    status = attmdb_att_set_value(bcss_env.shdl + BCSS_IDX_MEAS_VAL,
                                                        att_length, att_value);

    if (status != ATT_ERR_NO_ERROR)
    {
        goto failed;
    }

    // Check if the provided connection exist
    if (bcss_env.con_info.conidx == GAP_INVALID_CONIDX)
    {
        status = PRF_ERR_DISCONNECTED;
        goto failed;
    }

    attmdb_att_get_value(bcss_env.shdl + BCSS_IDX_MEAS_IND_CFG,
                                        &ccc_length, (uint8_t **)&ccc_value);

    // Check if device has been subscribed for indication
    if (*ccc_value != ATT_CCC_VAL_INDICATE)
    {
        status = PRF_ERR_IND_DISABLED;
        goto failed;
    }

    // Notify current value, task will receive the confirmation
    prf_server_send_event((prf_env_struct *)&bcss_env, true,
                                            bcss_env.shdl + BCSS_IDX_MEAS_VAL);
    return;

failed:
    // If error notify the app right away
    bcss_ind_cfm_send(status);
}

void bcss_disable(uint16_t conhdl)
{
    att_size_t att_length;
    uint16_t *att_value;
    struct bcss_disable_ind *ind = KE_MSG_ALLOC(BCSS_DISABLE_IND,
                                                bcss_env.con_info.appid,
                                                TASK_BCSS, bcss_disable_ind);

    attmdb_att_get_value(bcss_env.shdl + BCSS_IDX_MEAS_IND_CFG,
                                        &att_length, (uint8_t **)&att_value);

    // Send last CCC descriptor value for this client to the app for storing.
    ind->meas_ind_en = *att_value;
    ind->conhdl = bcss_env.con_info.conidx;

    // Disable BCSS in database
    attmdb_svc_set_permission(bcss_env.shdl, PERM(SVC, DISABLE));

    // Notify the app about the disconnection
    ke_msg_send(ind);

    // Go to idle state
    ke_state_set(TASK_BCSS, BCSS_IDLE);
}

#endif // (BLE_BCS_SERVER)

/// @} BCSS
