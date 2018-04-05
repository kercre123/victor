/**
 ****************************************************************************************
 *
 * @file bcsc.c
 *
 * @brief Body Composition Service Client implementation.
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
 * @addtogroup BCSC
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
 
#include "rwip_config.h"

#if (BLE_BCS_CLIENT)
#include "bcsc.h"
#include "bcsc_task.h"
#include "gap.h"
#include "gapc.h"
#include "prf_utils.h"

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// Body Composition Service Client Pool of Environments
struct bcsc_env_tag **bcsc_envs __attribute__((section("retention_mem_area0"), zero_init)); //@RETENTION MEMORY

/// Body Composition task descriptor
static const struct ke_task_desc TASK_DESC_BCSC = {bcsc_state_handler, &bcsc_default_handler, bcsc_state, BCSC_STATE_MAX, BCSC_IDX_MAX};

/*
 * EXPORTED FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */

void bcsc_init(void)
{
    // Reset all the profile role tasks
    PRF_CLIENT_RESET(bcsc_envs, BCSC);
}


void bcsc_enable_cfm_send(struct bcsc_env_tag *bcsc_env, struct prf_con_info *con_info, uint8_t status)
{
    // Send to APP the details of the discovered attributes on BLPS
    struct bcsc_enable_cfm * rsp = KE_MSG_ALLOC(BCSC_ENABLE_CFM,
                                                con_info->appid, con_info->prf_id,
                                                bcsc_enable_cfm);

    rsp->conhdl = gapc_get_conhdl(con_info->conidx);
    rsp->status = status;

    if (status == PRF_ERR_OK)
    {
        rsp->bcs = bcsc_env->bcs;

        /* Register for indications */
        prf_register_atthdl2gatt(&bcsc_env->con_info, &bcsc_env->bcs.svc);

        // Go to connected state
        ke_state_set(bcsc_env->con_info.prf_id, BCSC_CONNECTED);
    }
    else
    {
        PRF_CLIENT_ENABLE_ERROR(bcsc_envs, con_info->prf_id, BCSC);
    }

    ke_msg_send(rsp);
}

void bcsc_unpack_meas_value(bcs_meas_t *pmeas_val, uint16_t *flags, uint8_t *packed_bp)
{
    uint8_t *ptr = packed_bp;

    // Get the flags
    *flags = co_read16p(ptr);
    ptr += sizeof(*flags);

    // Set Measurment Unit
    pmeas_val->measurement_unit = (*flags & BCM_FLAG_UNIT_IMPERIAL) ? BCS_UNIT_IMPERIAL : BCS_UNIT_SI;

    // Get the body fat percentage
    pmeas_val->body_fat_percentage = co_read16p(ptr);
    ptr += sizeof(pmeas_val->body_fat_percentage);

    if ((*flags & BCM_FLAG_TIME_STAMP) == BCM_FLAG_TIME_STAMP)
    {
        ptr += prf_unpack_date_time(ptr, &pmeas_val->time_stamp);
    }

    if ((*flags & BCM_FLAG_USER_ID) == BCM_FLAG_USER_ID)
    {
        pmeas_val->user_id = *ptr;
        ptr += sizeof(pmeas_val->user_id);
    }

    if ((*flags & BCM_FLAG_BASAL_METABOLISM) == BCM_FLAG_BASAL_METABOLISM)
    {
        pmeas_val->basal_metabolism = co_read16p(ptr);
        ptr += sizeof(pmeas_val->basal_metabolism);
    }

    if ((*flags & BCM_FLAG_MUSCLE_PERCENTAGE) == BCM_FLAG_MUSCLE_PERCENTAGE)
    {
        pmeas_val->muscle_percentage = co_read16p(ptr);
        ptr += sizeof(pmeas_val->muscle_percentage);
    }

    if ((*flags & BCM_FLAG_MUSCLE_MASS) == BCM_FLAG_MUSCLE_MASS)
    {
        pmeas_val->muscle_mass = co_read16p(ptr);
        ptr += sizeof(pmeas_val->muscle_mass);
    }

    if ((*flags & BCM_FLAG_FAT_FREE_MASS) == BCM_FLAG_FAT_FREE_MASS)
    {
        pmeas_val->fat_free_mass = co_read16p(ptr);
        ptr += sizeof(pmeas_val->fat_free_mass);
    }

    if ((*flags & BCM_FLAG_SOFT_LEAN_MASS) == BCM_FLAG_SOFT_LEAN_MASS)
    {
        pmeas_val->soft_lean_mass = co_read16p(ptr);
        ptr += sizeof(pmeas_val->soft_lean_mass);
    }

    if ((*flags & BCM_FLAG_BODY_WATER_MASS) == BCM_FLAG_BODY_WATER_MASS)
    {
        pmeas_val->body_water_mass = co_read16p(ptr);
        ptr += sizeof(pmeas_val->body_water_mass);
    }

    if ((*flags & BCM_FLAG_IMPEDANCE) == BCM_FLAG_IMPEDANCE)
    {
        pmeas_val->impedance = co_read16p(ptr);
        ptr += sizeof(pmeas_val->impedance);
    }

    if ((*flags & BCM_FLAG_WEIGHT) == BCM_FLAG_WEIGHT)
    {
        pmeas_val->weight = co_read16p(ptr);
        ptr += sizeof(pmeas_val->weight);
    }

    if ((*flags & BCM_FLAG_HEIGHT) == BCM_FLAG_HEIGHT)
    {
        pmeas_val->height = co_read16p(ptr);
        ptr += sizeof(pmeas_val->height);
    }
}

void bcsc_error_ind_send(struct bcsc_env_tag *bcsc_env, uint8_t status)
{
    struct bcsc_error_ind *ind = KE_MSG_ALLOC(BCSC_ERROR_IND,
                                              bcsc_env->con_info.appid,
                                              bcsc_env->con_info.prf_id,
                                              bcsc_error_ind);

    ind->conhdl    = gapc_get_conhdl(bcsc_env->con_info.conidx);
    // It will be an PRF status code
    ind->status    = status;

    // Send the message
    ke_msg_send(ind);
}

#endif /* (BLE_BCS_CLIENT) */

/// @} BCSC
