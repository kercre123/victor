/**
 ****************************************************************************************
 *
 * @file user_security.c
 *
 * @brief Security project source code.
 *
 * Copyright (C) 2015. Dialog Semiconductor Ltd, unpublished work. This computer
 * program includes Confidential, Proprietary Information and is a Trade Secret of
 * Dialog Semiconductor Ltd.  All use, disclosure, and/or reproduction is prohibited
 * unless authorized in writing. All Rights Reserved.
 *
 * <bluetooth.support@diasemi.com> and contributors.
 *
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @addtogroup APP
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"             // SW configuration
#include "user_security.h"
#include "arch_api.h"
#include "gap.h"
#include "app_prf_perm_types.h"
#include "app_bond_db.h"            // Security DB
#include "app_easy_security.h"
#include "app_task.h"

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

// Manufacturer Specific Data ADV structure type
struct mnf_specific_data_ad_structure
{
    uint8_t ad_structure_size;
    uint8_t ad_structure_type;
    uint8_t company_id[APP_AD_MSD_COMPANY_ID_LEN];
    uint8_t proprietary_data[APP_AD_MSD_DATA_LEN];
};

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

uint8_t app_connection_idx;
timer_hnd app_adv_data_update_timer_used;
timer_hnd app_param_update_request_timer_used;

// Manufacturer Specific Data
struct mnf_specific_data_ad_structure mnf_data __attribute__((section("retention_mem_area0"),zero_init)); //@RETENTION MEMORY

// Stored static random address
struct gap_bdaddr stored_addr                  __attribute__((section("retention_mem_area0"),zero_init)); //@RETENTION MEMORY

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
*/

/**
 ****************************************************************************************
 * @brief Initialize Manufacturer Specific Data
 * @return void
 ****************************************************************************************
 */
static void mnf_data_init()
{
    mnf_data.ad_structure_size = sizeof(struct mnf_specific_data_ad_structure ) - sizeof(uint8_t); // minus the size of the ad_structure_size field
    mnf_data.ad_structure_type = GAP_AD_TYPE_MANU_SPECIFIC_DATA;
    mnf_data.company_id[0] = APP_AD_MSD_COMPANY_ID & 0xFF; // LSB
    mnf_data.company_id[1] = (APP_AD_MSD_COMPANY_ID >> 8 )& 0xFF; // MSB
    mnf_data.proprietary_data[0] = 0;
    mnf_data.proprietary_data[1] = 0;
}

/**
 ****************************************************************************************
 * @brief Update Manufacturer Specific Data
 * @return void
 ****************************************************************************************
 */
static void mnf_data_update()
{
    uint16_t data;

    data = mnf_data.proprietary_data[0] | (mnf_data.proprietary_data[1] << 8);
    data += 1;
    mnf_data.proprietary_data[0] = data & 0xFF;
    mnf_data.proprietary_data[1] = (data >> 8) & 0xFF;

    if (data == 0xFFFF) {
         mnf_data.proprietary_data[0] = 0;
         mnf_data.proprietary_data[1] = 0;
    }
}

/**
 ****************************************************************************************
 * @brief Advertisement data timer update callback function.
 * @return void
 ****************************************************************************************
*/
static void adv_data_update_timer_cb()
{
    app_easy_gap_advertise_stop();
}

/**
 ****************************************************************************************
 * @brief Parameter update request timer callback function.
 * @return void
 ****************************************************************************************
*/
static void param_update_request_timer_cb()
{
    app_easy_gap_param_update_start(app_connection_idx);
    app_param_update_request_timer_used = EASY_TIMER_INVALID_TIMER;
}

void user_app_init(void)
{
    app_prf_srv_perm_t svc_perm = SRV_PERM_ENABLE;

    app_param_update_request_timer_used = EASY_TIMER_INVALID_TIMER;

    // Initialize Manufacturer Specific Data
    mnf_data_init();

    default_app_on_init();

    // Set custom service permission depending on the currently selected preset
    #if defined(USER_CFG_PAIR_METHOD_JUST_WORKS)
    svc_perm = SRV_PERM_UNAUTH;

    #elif defined (USER_CFG_PAIR_METHOD_PASSKEY)
    svc_perm = SRV_PERM_AUTH;

    #elif defined (USER_CFG_PAIR_METHOD_OOB)
    svc_perm = SRV_PERM_AUTH;

    #else
    svc_perm = SRV_PERM_ENABLE;
    #endif

    app_set_prf_srv_perm(TASK_CUSTS1, svc_perm);

    // Fetch bond data from the external memory
    bond_db_init();
}

/**
 * @brief Add an AD structure in the Advertising or Scan Response Data of the GAPM_START_ADVERTISE_CMD parameter struct.
 * @param[in] cmd               GAPM_START_ADVERTISE_CMD parameter struct
 * @param[in] ad_struct_data    AD structure buffer
 * @param[in] ad_struct_len     AD structure length
 * @return void
 */
static void app_add_ad_struct(struct gapm_start_advertise_cmd *cmd, void *ad_struct_data, uint8_t ad_struct_len)
{
    if ( (APP_ADV_DATA_MAX_SIZE - cmd->info.host.adv_data_len) >= ad_struct_len)
    {
        // Copy data
        memcpy(&cmd->info.host.adv_data[cmd->info.host.adv_data_len], ad_struct_data, ad_struct_len);

        // Update Advertising Data Length
        cmd->info.host.adv_data_len += ad_struct_len;
    }
    else if ( (APP_SCAN_RESP_DATA_MAX_SIZE - cmd->info.host.scan_rsp_data_len) >= ad_struct_len)
    {
        // Copy data
        memcpy(&cmd->info.host.scan_rsp_data[cmd->info.host.scan_rsp_data_len], ad_struct_data, ad_struct_len);

        // Update Scan Responce Data Length
        cmd->info.host.scan_rsp_data_len += ad_struct_len;
    }
    else
    {
        // Manufacturer Specific Data do not fit in either Advertising Data or Scan Response Data
        ASSERT_ERROR(0);
    }
}

void user_app_adv_start(void)
{
    // Schedule the next advertising data update
    app_adv_data_update_timer_used = app_easy_timer(APP_ADV_DATA_UPDATE_TO, adv_data_update_timer_cb);

    struct gapm_start_advertise_cmd* cmd;
    cmd = app_easy_gap_undirected_advertise_get_active();

    // add manufacturer specific data dynamically
    mnf_data_update();
    app_add_ad_struct(cmd, &mnf_data, sizeof(struct mnf_specific_data_ad_structure));

#if defined (USER_CFG_PRIV_GEN_STATIC_RND)
    // Provide previously generated random address if saved
    if (stored_addr.addr_type == 0x01)
    {
        cmd->op.addr_src = GAPM_PROVIDED_RND_ADDR;
        cmd->op.addr = stored_addr.addr;
        cmd->op.renew_dur = 0;
    }
#endif

    app_easy_gap_undirected_advertise_start();
}

void user_app_connection(uint8_t connection_idx, struct gapc_connection_req_ind const *param)
{
    if (app_env[connection_idx].conidx != GAP_INVALID_CONIDX)
    {
        app_connection_idx = connection_idx;

        // Stop the advertising data update timer
        app_easy_timer_cancel(app_adv_data_update_timer_used);

        // Check if the parameters of the established connection are the preferred ones.
        // If not then schedule a connection parameter update request.
        if ((param->con_interval < user_connection_param_conf.intv_min) ||
            (param->con_interval > user_connection_param_conf.intv_max) ||
            (param->con_latency != user_connection_param_conf.latency) ||
            (param->sup_to != user_connection_param_conf.time_out))
        {
            // Connection params are not these that we expect
            app_param_update_request_timer_used = app_easy_timer(APP_PARAM_UPDATE_REQUEST_TO, param_update_request_timer_cb);
        }
    }
    else
    {
        // No connection has been established, restart advertising
        user_app_adv_start();
    }

    default_app_on_connection(connection_idx, param);
}

void user_app_adv_undirect_complete(uint8_t status)
{
    // If advertising was canceled then update advertising data and start advertising again
    if (status == GAP_ERR_CANCELED)
    {
        user_app_adv_start();
    }
}

void user_app_disconnect(struct gapc_disconnect_ind const *param)
{
    // Cancel the parameter update request timer
    if (app_param_update_request_timer_used != EASY_TIMER_INVALID_TIMER)
    {
        app_easy_timer_cancel(app_param_update_request_timer_used);
        app_param_update_request_timer_used = EASY_TIMER_INVALID_TIMER;
    }

    uint8_t state = ke_state_get(TASK_APP);

    if ((state == APP_SECURITY) ||
        (state == APP_CONNECTED) ||
        (state == APP_PARAM_UPD))
    {
        // Restart Advertising
        user_app_adv_start();
    }
    else
    {
        // We are not in a Connected State
        ASSERT_ERR(0);
    }
}

#if (BLE_APP_SEC)

void user_app_on_tk_exch_nomitm(uint8_t connection_idx,
                                struct gapc_bond_req_ind const * param)
{
#if defined (USER_CFG_PAIR_METHOD_JUST_WORKS) || defined (USER_CFG_PAIR_METHOD_PASSKEY) || defined (USER_CFG_PAIR_METHOD_OOB)
    if (param->data.tk_type == GAP_TK_DISPLAY)
    {
        // By default we send hardcoded passkey
        uint32_t passkey = APP_SECURITY_MITM_PASSKEY_VAL;

        app_easy_security_tk_exch(connection_idx, (uint8_t*) &passkey, 4);
    }
    else if (param->data.tk_type == GAP_TK_OOB)
    {
        // By default we send hardcoded oob data
        uint8_t oob_tk[KEY_LEN] = APP_SECURITY_OOB_TK_VAL;

        app_easy_security_tk_exch(connection_idx, (uint8_t*) oob_tk, KEY_LEN);
    }
#else
    default_app_on_tk_exch_nomitm(connection_idx, param);
#endif
}

void user_app_on_encrypt_req_ind(uint8_t connection_idx,
                                    struct gapc_encrypt_req_ind const *param)
{
#if defined (USER_CFG_PAIR_METHOD_JUST_WORKS) || defined (USER_CFG_PAIR_METHOD_PASSKEY) || defined (USER_CFG_PAIR_METHOD_OOB)
    const struct bond_db_data *pbd;

    // Retrieve bond data and put it inside the app_sec_env
    pbd = bond_db_lookup_by_ediv(&param->rand_nb, param->ediv);
    if (pbd)
    {
        app_sec_env[connection_idx].auth = pbd->auth;                                               /// Authentication
        memcpy(&app_sec_env[connection_idx].ltk, &pbd->ltk.ltk, sizeof(struct gap_sec_key));        /// Long Term Key
        app_sec_env[connection_idx].ediv = pbd->ltk.ediv;                                           /// Encryption Diversifier
        memcpy(&app_sec_env[connection_idx].rand_nb, &pbd->ltk.randnb, RAND_NB_LEN);                /// Random Number
        app_sec_env[connection_idx].key_size = pbd->ltk.key_size;                                   /// Encryption key size (7 to 16)
        memcpy(&app_sec_env[connection_idx].irk, &pbd->irk, sizeof(struct gapc_irk));               /// IRK
    }
#endif
    // Validate bond data in app_sec_env
    default_app_on_encrypt_req_ind(connection_idx, param);
}

void user_app_on_pairing_succeeded(void)
{
    struct bond_db_data bd;

    if (app_sec_env[app_connection_idx].auth & GAP_AUTH_BOND)
    {
        memset(&bd, 0, sizeof(bd));

        // store bond data
        bd.valid = 0xAA;

        memcpy(&bd.ltk.ltk, &app_sec_env[app_connection_idx].ltk, sizeof(struct gap_sec_key));  /// Long Term Key
        bd.ltk.ediv = app_sec_env[app_connection_idx].ediv;                                     /// Encryption Diversifier
        memcpy(&bd.ltk.randnb, &app_sec_env[app_connection_idx].rand_nb, RAND_NB_LEN);          /// Random Number
        bd.ltk.key_size = app_sec_env[app_connection_idx].key_size;                             /// Encryption key size (7 to 16)

        // Check bits for resolvable address type
        if ((app_sec_env[app_connection_idx].peer_addr_type == ADDR_RAND) &&
            (app_sec_env[app_connection_idx].peer_addr.addr[0] && GAP_RSLV_ADDR))
        {
            memcpy(&bd.irk, &app_sec_env[app_connection_idx].irk, sizeof(struct gapc_irk));     /// IRK
            bd.flags |= BOND_DB_ENTRY_IRK_PRESENT;
        }

        // Remote address
        bd.bdaddr.addr_type = app_sec_env[app_connection_idx].peer_addr_type;
        memcpy(&bd.bdaddr.addr, &app_sec_env[app_connection_idx].peer_addr, sizeof(struct bd_addr));

        bd.auth = app_sec_env[app_connection_idx].auth;                                         // authentication level

        bond_db_store(&bd);
    }
}

#endif // BLE_APP_SEC

void user_catch_rest_hndl(ke_msg_id_t const msgid,
                          void const *param,
                          ke_task_id_t const dest_id,
                          ke_task_id_t const src_id)
{
    switch(msgid)
    {
        case GAPC_PARAM_UPDATED_IND:
        {
            // Cast the "param" pointer to the appropriate message structure
            struct gapc_param_updated_ind const *msg_param = (struct gapc_param_updated_ind const *)(param);

            // Check if updated Conn Params filled to preffered ones
            if ((msg_param->con_interval >= user_connection_param_conf.intv_min) &&
                (msg_param->con_interval <= user_connection_param_conf.intv_max) &&
                (msg_param->con_latency == user_connection_param_conf.latency) &&
                (msg_param->sup_to == user_connection_param_conf.time_out))
            {
            }
        } break;
        case GAPM_DEV_BDADDR_IND:
        {
            struct gapm_dev_bdaddr_ind const *msg_param = (struct gapm_dev_bdaddr_ind const *)(param);

            stored_addr = msg_param->addr;
        }

        default:
            break;
    }
}

/// @} APP
