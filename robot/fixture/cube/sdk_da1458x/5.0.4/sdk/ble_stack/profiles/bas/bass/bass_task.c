/**
 ****************************************************************************************
 *
 * @file bass_task.c
 *
 * @brief Battery Service Server Role Task Implementation.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 * $Rev$
 *
 ****************************************************************************************
 */


/**
 ****************************************************************************************
 * @addtogroup BASSTASK
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
#include "atts_util.h"

#include "bass.h"
#include "bass_task.h"

#include "prf_utils.h"

/*
 * GLOBAL FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref BASS_CREATE_DB_REQ message.
 * The handler adds BAS into the database using value of the features param.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int bass_create_db_req_handler(ke_msg_id_t const msgid,
                                      struct bass_create_db_req const *param,
                                      ke_task_id_t const dest_id,
                                      ke_task_id_t const src_id)
{
    // Service content flag
    uint8_t cfg_flag = BAS_CFG_FLAG_MANDATORY_MASK;
    // Status
    uint8_t status = PRF_ERR_OK;
#ifndef USE_ONE_BAS_INSTANCE
    // Counter
    uint8_t i;
#endif    
    // Battery Level characteristic value permissions
    uint16_t perm;
    // Battery Level characteristic value properties
    uint8_t prop;

    // Save profile id
    bass_env.con_info.prf_id = TASK_BASS;

    // Check number of BAS instances
    if (param->bas_nb <= BASS_NB_BAS_INSTANCES_MAX)
    {
        // Save number of BAS
        bass_env.bas_nb = param->bas_nb;

#ifndef USE_ONE_BAS_INSTANCE
        for (i = 0; ((i < param->bas_nb) && (status == PRF_ERR_OK)); i++)
#else
        const int i = 0;
#endif        
        {
            // Save database configuration
            bass_env.features[i] = param->features[i];

            // Check if notifications are supported
            if (bass_env.features[i] == BAS_BATT_LVL_NTF_SUP)
            {
                cfg_flag |= BAS_CFG_FLAG_NTF_SUP_MASK;
            }

            // Check if multiple instances
            if (bass_env.bas_nb > 1)
            {
                cfg_flag |= BAS_CFG_FLAG_MTP_BAS_MASK;
            }

            //Create BAS in the DB
            status = attm_svc_create_db(&bass_env.shdl[i], (uint8_t *)&cfg_flag, BAS_IDX_NB, NULL,
                                        dest_id, &bas_att_db[0]);

            //Disable the service and set optional features
            if (status == PRF_ERR_OK)
            {
                //Disable service
                status = attmdb_svc_set_permission(bass_env.shdl[i], PERM(SVC, DISABLE));

                //Set optional properties and permissions
                if (bass_env.features[i] == BAS_BATT_LVL_NTF_SUP)
                {
                    prop = ATT_CHAR_PROP_RD | ATT_CHAR_PROP_NTF;
                    perm = PERM(RD, ENABLE) | PERM(NTF, ENABLE);

                    attmdb_att_partial_value_update(bass_env.shdl[i] + BAS_IDX_BATT_LVL_CHAR, 0, 1, &prop);
                    attmdb_att_set_permission(bass_env.shdl[i] + BAS_IDX_BATT_LVL_VAL, perm);
                }
            }

            // Reset configuration flag
            cfg_flag = BAS_CFG_FLAG_MANDATORY_MASK;
        }

        if (status == PRF_ERR_OK)
        {
            //If we are here, database has been fulfilled with success, go to idle state
            ke_state_set(TASK_BASS, BASS_IDLE);
        }
    }
    else
    {
        status = PRF_ERR_INVALID_PARAM;
    }

    // Send confirmation to application
    struct bass_create_db_cfm * cfm = KE_MSG_ALLOC(BASS_CREATE_DB_CFM, src_id, TASK_BASS,
                                                   bass_create_db_cfm);
    cfm->status = status;
    ke_msg_send(cfm);

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref BAPS_ENABLE_REQ message.
 * The handler enables the Battery 'Profile' Server Role.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int bass_enable_req_handler(ke_msg_id_t const msgid,
                                   struct bass_enable_req const *param,
                                   ke_task_id_t const dest_id,
                                   ke_task_id_t const src_id)
{
    // Attribute offset - Used to retrieve Char. Pres. Format Descriptor handle
    uint8_t offset;
    // Packed Char. Presentation Format value
    uint8_t packed_char_pres[PRF_CHAR_PRES_FMT_SIZE];
#ifndef USE_ONE_BAS_INSTANCE
    // Counter
    uint8_t i;
#endif    
    // Notification Configuration
    uint16_t ntf_cfg = 0;

    // Save the application task id
    bass_env.con_info.appid = src_id;
    // Save the connection handle associated to the profile
    bass_env.con_info.conidx = gapc_get_conidx(param->conhdl);

    // Check if the provided connection exist
    if (bass_env.con_info.conidx == GAP_INVALID_CONIDX)
    {
        // The connection doesn't exist, request disallowed
        prf_server_error_ind_send((prf_env_struct *)&bass_env, PRF_ERR_REQ_DISALLOWED,
                                  BASS_ERROR_IND, BASS_ENABLE_REQ);
    }
    else
    {
#ifndef USE_ONE_BAS_INSTANCE
        // For each BAS instance
        for (i = 0; i < bass_env.bas_nb; i++)
#else
        const int i = 0;
#endif        
        {
            // Reset Offset value
            offset = BAS_IDX_BATT_LVL_NTF_CFG;

            // Update Battery Level value in DB
            attmdb_att_set_value(bass_env.shdl[i] + BAS_IDX_BATT_LVL_VAL, sizeof(uint8_t),
                                 (uint8_t *)&param->current_batt_lvl[i]);

            // Check if BAS supports notifications
            if (bass_env.features[i] == BAS_BATT_LVL_NTF_SUP)
            {
                // Increment offset (Client Char. Cfg is placed before Char. Pres Fmt. in DB)
                offset++;

                // If client is a bonded client
                if (param->con_type == PRF_CON_NORMAL)
                {
                    ntf_cfg = param->batt_level_ntf_cfg[i];

                    /*
                     * The server shall send a notification if value of the Battery Level characteristic
                     * has changed while the service has been disconnected from a bonded client.
                     */
                    if (param->batt_level_ntf_cfg[i] == PRF_CLI_START_NTF)
                    {
                        // Conserve information in environment
                        bass_env.features[i] |= BASS_FLAG_NTF_CFG_BIT;

                        // Check if old and previous battery level values are different
                        if (param->old_batt_lvl[i] != param->current_batt_lvl[i])
                        {
                            //Notify current battery level value
                            prf_server_send_event((prf_env_struct *)&bass_env, false,
                                    bass_env.shdl[i] + BAS_IDX_BATT_LVL_VAL);
                        }
                    }
                }

                // Set NTF Cfg value in the DB
                attmdb_att_set_value(bass_env.shdl[i] + BAS_IDX_BATT_LVL_NTF_CFG, sizeof(uint16_t),
                                     (uint8_t *)&ntf_cfg);
            }

            // Check if Characteristic Presentation Format descriptor has been added
            if (bass_env.bas_nb > 1)
            {
                prf_pack_char_pres_fmt(&packed_char_pres[0], &param->batt_level_pres_format[i]);

                /*
                 * Set Characteristic Presentation Format descriptor value
                 * Not supposed to change during connection
                 */
                attmdb_att_set_value(bass_env.shdl[i] + offset,
                                     PRF_CHAR_PRES_FMT_SIZE,
                                     &packed_char_pres[0]);
            }

            // Enable Service + Set Security Level
            attmdb_svc_set_permission(bass_env.shdl[i], param->sec_lvl);
        }

        // Go to connected state
        ke_state_set(TASK_BASS, BASS_CONNECTED);
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref BAPS_BATT_LEVEL_SEND_REQ message.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int bass_batt_level_upd_req_handler(ke_msg_id_t const msgid,
                                            struct bass_batt_level_upd_req const *param,
                                            ke_task_id_t const dest_id,
                                            ke_task_id_t const src_id)
{
    uint8_t status = PRF_ERR_OK;

    // Check provided values
    if((param->conhdl == gapc_get_conhdl(bass_env.con_info.conidx)) &&
       (param->bas_instance < bass_env.bas_nb)     &&
       (param->batt_level <= BAS_BATTERY_LVL_MAX))
    {
#ifndef USE_ONE_BAS_INSTANCE
        // Update Battery Level value in DB
        attmdb_att_set_value(bass_env.shdl[param->bas_instance] + BAS_IDX_BATT_LVL_VAL,
                             sizeof(uint8_t), (uint8_t *)&param->batt_level);

        // Check if notifications are enabled
        if((bass_env.features[param->bas_instance] & BASS_FLAG_NTF_CFG_BIT)
                                                  == BASS_FLAG_NTF_CFG_BIT)
        {
            // Send notification through GATT
            prf_server_send_event((prf_env_struct *)&bass_env, false,
                    bass_env.shdl[param->bas_instance] + BAS_IDX_BATT_LVL_VAL);
        }
#else
        // Update Battery Level value in DB
        attmdb_att_set_value(bass_env.shdl[0] + BAS_IDX_BATT_LVL_VAL,
                             sizeof(uint8_t), (uint8_t *)&param->batt_level);

        // Check if notifications are enabled
        if((bass_env.features[0] & BASS_FLAG_NTF_CFG_BIT)
                                                  == BASS_FLAG_NTF_CFG_BIT)
        {
            // Send notification through GATT
            prf_server_send_event((prf_env_struct *)&bass_env, false,
                    bass_env.shdl[0] + BAS_IDX_BATT_LVL_VAL);
        }
#endif        
        else
        {
            status = PRF_ERR_NTF_DISABLED;
        }
    }
    else
    {
        status = PRF_ERR_INVALID_PARAM;
    }

    if (status != PRF_ERR_OK)
    {
        bass_batt_level_upd_cfm_send(status);
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref GL2C_CODE_ATT_WR_CMD_IND message.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gattc_write_cmd_ind_handler(ke_msg_id_t const msgid,
                                      struct gattc_write_cmd_ind const *param,
                                      ke_task_id_t const dest_id,
                                      ke_task_id_t const src_id)
{
#ifndef USE_ONE_BAS_INSTANCE
    // Counter
    uint8_t i;
#endif    
    // Status
    uint8_t status = PRF_APP_ERROR;
    // Written value
    uint16_t ntf_cfg;

#ifndef USE_ONE_BAS_INSTANCE
    // Look for the BAS instance
    for (i = 0; ((i < bass_env.bas_nb) && (status == PRF_APP_ERROR)); i++)
    {
        if (param->handle == bass_env.shdl[i] + BAS_IDX_BATT_LVL_NTF_CFG)
        {
            // Go out of the loop
            status = PRF_ERR_OK;
        }
    }

    //Revert Last incrementation
    i--;
#else
    const int i = 0;
    
    if (param->handle == bass_env.shdl[i] + BAS_IDX_BATT_LVL_NTF_CFG)
    {
        // Go out of the loop
        status = PRF_ERR_OK;
    }
#endif    

    // If the attribute has been found, status is PRF_ERR_OK
    if (status == PRF_ERR_OK)
    {
        // Extract value before check
        ntf_cfg = co_read16p(&param->value[0]);

        // Only update configuration if value for stop or notification enable
        if ((ntf_cfg == PRF_CLI_STOP_NTFIND) || (ntf_cfg == PRF_CLI_START_NTF))
        {
            // Set NTF Cfg value in the DB
            attmdb_att_set_value(bass_env.shdl[i] + BAS_IDX_BATT_LVL_NTF_CFG, sizeof(uint16_t),
                                 (uint8_t *)&ntf_cfg);

            // Conserve information in environment
            if (ntf_cfg == PRF_CLI_START_NTF)
            {
                // Ntf cfg bit set to 1
                bass_env.features[i] |= BASS_FLAG_NTF_CFG_BIT;
            }
            else
            {
                // Ntf cfg bit set to 0
                bass_env.features[i] &= ~BASS_FLAG_NTF_CFG_BIT;
            }
            if(param->last)
            {
                // Inform APP of configuration change
                struct bass_batt_level_ntf_cfg_ind * ind = KE_MSG_ALLOC(BASS_BATT_LEVEL_NTF_CFG_IND,
                                                                        bass_env.con_info.appid, TASK_BASS,
                                                                        bass_batt_level_ntf_cfg_ind);
                ind->conhdl = gapc_get_conhdl(bass_env.con_info.conidx);
                co_write16p(&ind->ntf_cfg, ntf_cfg);
                ind->bas_instance = i;

                ke_msg_send(ind);
            }
        }
        else
        {
            status = PRF_APP_ERROR;
        }

        // Send write response
        atts_write_rsp_send(bass_env.con_info.conidx, param->handle, status);
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles @ref GATTC_CMP_EVT for GATTC_NOTIFY message meaning that Measurement
 * notification has been correctly sent to peer device (but not confirmed by peer device).
 * *
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
    if(param->req_type == GATTC_NOTIFY)
    {
        bass_batt_level_upd_cfm_send(param->status);
    }
    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Disconnection indication to BAPS.
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
    //Check Connection Handle
    if (KE_IDX_GET(src_id) == bass_env.con_info.conidx)
    {
        bass_disable(param->conhdl);
    }

    return (KE_MSG_CONSUMED);
}

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// Disabled State handler definition.
const struct ke_msg_handler bass_disabled[] =
{
    {BASS_CREATE_DB_REQ,            (ke_msg_func_t) bass_create_db_req_handler}
};

/// Idle State handler definition.
const struct ke_msg_handler bass_idle[] =
{
    {BASS_ENABLE_REQ,               (ke_msg_func_t) bass_enable_req_handler}
};

/// Connected State handler definition.
const struct ke_msg_handler bass_connected[] =
{
    {BASS_BATT_LEVEL_UPD_REQ,       (ke_msg_func_t) bass_batt_level_upd_req_handler},
    {GATTC_WRITE_CMD_IND,           (ke_msg_func_t) gattc_write_cmd_ind_handler},
    {GATTC_CMP_EVT,                 (ke_msg_func_t) gattc_cmp_evt_handler},
};

/// Default State handlers definition
const struct ke_msg_handler bass_default_state[] =
{
    {GAPC_DISCONNECT_IND,        (ke_msg_func_t)gapc_disconnect_ind_handler},
};

/// Specifies the message handler structure for every input state.
const struct ke_state_handler bass_state_handler[BASS_STATE_MAX] =
{
    [BASS_DISABLED]       = KE_STATE_HANDLER(bass_disabled),
    [BASS_IDLE]           = KE_STATE_HANDLER(bass_idle),
    [BASS_CONNECTED]      = KE_STATE_HANDLER(bass_connected),
};

/// Specifies the message handlers that are common to all states.
const struct ke_state_handler bass_default_handler = KE_STATE_HANDLER(bass_default_state);

/// Defines the place holder for the states of all the task instances.
ke_state_t bass_state[BASS_IDX_MAX] __attribute__((section("retention_mem_area0"),zero_init)); //@RETENTION MEMORY

#endif /* #if (BLE_BAS_SERVER) */

/// @} BASSTASK
