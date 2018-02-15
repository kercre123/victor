/**
 ****************************************************************************************
 *
 * @file app_easy_security.c
 *
 * @brief Application Security Entry Point
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
 * @addtogroup APP_SECURITY
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "co_bt.h"

#include "gapc_task.h"          // GAP Controller Task API Definition
#include <stdlib.h>

#include "app_api.h"            // Application task Definition
#include "app_easy_security.h"

/*
 * DEFINES
 ****************************************************************************************
 */

#define APP_EASY_SECURITY_MAX_CONNECTION APP_EASY_MAX_ACTIVE_CONNECTION

/*
 * LOCAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

static struct gapc_bond_cfm *gapc_bond_cfm_pairing_rsp[APP_EASY_SECURITY_MAX_CONNECTION] __attribute__((section("retention_mem_area0"),zero_init));
static struct gapc_bond_cfm *gapc_bond_cfm_tk_exch[APP_EASY_SECURITY_MAX_CONNECTION]     __attribute__((section("retention_mem_area0"),zero_init));
static struct gapc_bond_cfm *gapc_bond_cfm_csrk_exch[APP_EASY_SECURITY_MAX_CONNECTION]   __attribute__((section("retention_mem_area0"),zero_init));
static struct gapc_bond_cfm *gapc_bond_cfm_ltk_exch[APP_EASY_SECURITY_MAX_CONNECTION]    __attribute__((section("retention_mem_area0"),zero_init));
static struct gapc_encrypt_cfm *gapc_encrypt_cfm[APP_EASY_SECURITY_MAX_CONNECTION]       __attribute__((section("retention_mem_area0"),zero_init));
static struct gapc_security_cmd *gapc_security_req[APP_EASY_SECURITY_MAX_CONNECTION]     __attribute__((section("retention_mem_area0"),zero_init));

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

static inline struct gapc_bond_cfm* app_easy_security_pairing_rsp_create_msg(uint8_t connection_idx)
{
    // Allocate a message for GAP
    if (gapc_bond_cfm_pairing_rsp[connection_idx] == NULL)
    {
        struct gapc_bond_cfm* cfm = app_gapc_bond_cfm_pairing_rsp_msg_create(connection_idx);
        gapc_bond_cfm_pairing_rsp[connection_idx] = cfm;

        cfm->data.pairing_feat.iocap = user_security_conf.iocap;
        cfm->data.pairing_feat.oob = user_security_conf.oob;
        cfm->data.pairing_feat.auth = user_security_conf.auth;
        cfm->data.pairing_feat.sec_req = user_security_conf.sec_req;
        cfm->data.pairing_feat.key_size = user_security_conf.key_size;
        cfm->data.pairing_feat.ikey_dist = user_security_conf.ikey_dist;
        cfm->data.pairing_feat.rkey_dist = user_security_conf.rkey_dist;
    }
    return gapc_bond_cfm_pairing_rsp[connection_idx];
}

struct gapc_bond_cfm* app_easy_security_pairing_rsp_get_active(uint8_t connection_idx)
{
    ASSERT_WARNING(connection_idx < APP_EASY_SECURITY_MAX_CONNECTION);
    return app_easy_security_pairing_rsp_create_msg(connection_idx);
}

void app_easy_security_send_pairing_rsp(uint8_t connection_idx)
{
    struct gapc_bond_cfm* cmd;
    ASSERT_WARNING(connection_idx < APP_EASY_SECURITY_MAX_CONNECTION);
    cmd = app_easy_security_pairing_rsp_create_msg(connection_idx);
    ke_msg_send(cmd);
    gapc_bond_cfm_pairing_rsp[connection_idx] = NULL;
}

static inline struct gapc_bond_cfm* app_easy_security_tk_exch_create_msg(uint8_t connection_idx)
{
    // Allocate a message for GAP
    if (gapc_bond_cfm_tk_exch[connection_idx] == NULL)
    {
        gapc_bond_cfm_tk_exch[connection_idx] = app_gapc_bond_cfm_tk_exch_msg_create(connection_idx);
    }
     return gapc_bond_cfm_tk_exch[connection_idx];
}

struct gapc_bond_cfm* app_easy_security_tk_get_active(uint8_t connection_idx)
{
    ASSERT_WARNING(connection_idx < APP_EASY_SECURITY_MAX_CONNECTION);
    return app_easy_security_tk_exch_create_msg(connection_idx);
}

void app_easy_security_tk_exch(uint8_t connection_idx, uint8_t *key, uint8_t length)
{
    struct gapc_bond_cfm* cmd;
    ASSERT_WARNING(connection_idx < APP_EASY_SECURITY_MAX_CONNECTION);
    cmd = app_easy_security_tk_exch_create_msg(connection_idx);

    // Load the pass key or the OOB provided key to the TK member of the created GAPC_BOND_CFM message
    memset((void*)gapc_bond_cfm_tk_exch[connection_idx]->data.tk.key, 0, KEY_LEN);
    memcpy(gapc_bond_cfm_tk_exch[connection_idx]->data.tk.key, key, length * sizeof(uint8_t));

    ke_msg_send(cmd);
    gapc_bond_cfm_tk_exch[connection_idx] = NULL;
}

static inline struct gapc_bond_cfm* app_easy_security_csrk_exch_create_msg(uint8_t connection_idx)
{
    // Allocate a message for GAP
    if (gapc_bond_cfm_csrk_exch[connection_idx] == NULL)
    {
        gapc_bond_cfm_csrk_exch[connection_idx] = app_gapc_bond_cfm_csrk_exch_msg_create(connection_idx);
        app_sec_gen_csrk(connection_idx);
        memcpy((void*)gapc_bond_cfm_csrk_exch[connection_idx]->data.csrk.key, app_sec_env[connection_idx].csrk.key, KEY_LEN);
    }
     return gapc_bond_cfm_csrk_exch[connection_idx];
}

struct gapc_bond_cfm* app_easy_security_csrk_get_active(uint8_t connection_idx)
{
    ASSERT_WARNING(connection_idx < APP_EASY_SECURITY_MAX_CONNECTION);
    return app_easy_security_csrk_exch_create_msg(connection_idx);
}

void app_easy_security_csrk_exch(uint8_t connection_idx)
{
    struct gapc_bond_cfm* cfm;
    ASSERT_WARNING(connection_idx < APP_EASY_SECURITY_MAX_CONNECTION);
    cfm = app_easy_security_csrk_exch_create_msg(connection_idx);
    ke_msg_send(cfm);
    gapc_bond_cfm_csrk_exch[connection_idx] = NULL;
}

static inline struct gapc_bond_cfm* app_easy_security_bond_cfm_ltk_exch_create_msg(uint8_t connection_idx)
{
    // Allocate a message for GAP
    if (gapc_bond_cfm_ltk_exch[connection_idx] == NULL)
    {
        gapc_bond_cfm_ltk_exch[connection_idx] = app_gapc_bond_cfm_ltk_exch_msg_create(connection_idx);
    }
    return gapc_bond_cfm_ltk_exch[connection_idx];
}

struct gapc_bond_cfm* app_easy_security_ltk_exch_get_active(uint8_t connection_idx)
{
    ASSERT_WARNING(connection_idx < APP_EASY_SECURITY_MAX_CONNECTION);
    return app_easy_security_bond_cfm_ltk_exch_create_msg(connection_idx);
}

void app_easy_security_ltk_exch(uint8_t connection_idx)
{
    struct gapc_bond_cfm* cfm;
    ASSERT_WARNING(connection_idx < APP_EASY_SECURITY_MAX_CONNECTION);
    cfm = app_easy_security_bond_cfm_ltk_exch_create_msg(connection_idx);
    ke_msg_send(cfm);
    gapc_bond_cfm_ltk_exch[connection_idx] = NULL;
}

void app_easy_security_set_ltk_exch_from_sec_env(uint8_t connection_idx)
{
    struct gapc_bond_cfm* cfm;
    ASSERT_WARNING(connection_idx < APP_EASY_SECURITY_MAX_CONNECTION);
    cfm = app_easy_security_bond_cfm_ltk_exch_create_msg(connection_idx);
    cfm->data.ltk.key_size = app_sec_env[connection_idx].key_size;
    cfm->data.ltk.ediv = app_sec_env[connection_idx].ediv;

    memcpy(&(cfm->data.ltk.randnb), &(app_sec_env[connection_idx].rand_nb) , RAND_NB_LEN);
    memcpy(&(cfm->data.ltk.ltk), &(app_sec_env[connection_idx].ltk) , KEY_LEN);
}

void app_easy_security_set_ltk_exch(uint8_t connection_idx, uint8_t* long_term_key, uint8_t encryption_key_size,
                                            uint8_t* random_number, uint16_t encryption_diversifier)
{
    struct gapc_bond_cfm* cfm;
    ASSERT_WARNING(connection_idx < APP_EASY_SECURITY_MAX_CONNECTION);
    cfm = app_easy_security_bond_cfm_ltk_exch_create_msg(connection_idx);
    cfm->data.ltk.key_size = encryption_key_size;
    cfm->data.ltk.ediv = encryption_diversifier;

    memcpy(&(cfm->data.ltk.randnb), random_number , RAND_NB_LEN);
    memcpy(&(cfm->data.ltk.ltk), long_term_key , KEY_LEN);
}

static inline struct gapc_encrypt_cfm* app_easy_security_encrypt_cfm_create_msg(uint8_t connection_idx)
{
    // Allocate a message for GAP
    if (gapc_encrypt_cfm[connection_idx] == NULL)
    {
        gapc_encrypt_cfm[connection_idx] = app_gapc_encrypt_cfm_msg_create(connection_idx);
    }
    return gapc_encrypt_cfm[connection_idx];
}

void app_easy_security_encrypt_cfm(uint8_t connection_idx)
{
    struct gapc_encrypt_cfm* cfm;
    ASSERT_WARNING(connection_idx < APP_EASY_SECURITY_MAX_CONNECTION);
    cfm = app_easy_security_encrypt_cfm_create_msg(connection_idx);
    ke_msg_send(cfm);
    gapc_encrypt_cfm[connection_idx] = NULL;
}

struct gapc_encrypt_cfm* app_easy_security_encrypt_cfm_get_active( uint8_t connection_idx )
{
    ASSERT_WARNING(connection_idx < APP_EASY_SECURITY_MAX_CONNECTION);
    return app_easy_security_encrypt_cfm_create_msg(connection_idx);
}

bool app_easy_security_validate_encrypt_req_against_env(uint8_t connection_idx, struct gapc_encrypt_req_ind const *param)
{
    if(((app_sec_env[connection_idx].auth & GAP_AUTH_BOND) != 0) &&
        (memcmp(&(app_sec_env[connection_idx].rand_nb), &(param->rand_nb), RAND_NB_LEN) == 0) &&
        (app_sec_env[connection_idx].ediv == param->ediv))
    {
        return true;
    }
    return false;
}

void app_easy_security_set_encrypt_req_valid(uint8_t connection_idx)
{
    ASSERT_WARNING(connection_idx < APP_EASY_SECURITY_MAX_CONNECTION);
    struct gapc_encrypt_cfm* cfm = app_easy_security_encrypt_cfm_create_msg(connection_idx);
    cfm->found = true;
    cfm->key_size = app_sec_env[connection_idx].key_size;
    memcpy(&(cfm->ltk), &(app_sec_env[connection_idx].ltk), KEY_LEN);
}

void app_easy_security_set_encrypt_req_invalid(uint8_t connection_idx)
{
    ASSERT_WARNING(connection_idx < APP_EASY_SECURITY_MAX_CONNECTION);
    struct gapc_encrypt_cfm* cfm = app_easy_security_encrypt_cfm_create_msg(connection_idx);
    cfm->found = false;
}

static inline struct gapc_security_cmd* app_easy_security_request_create_msg(uint8_t connection_idx)
{
    // Allocate a message for GAP
    if (gapc_security_req[connection_idx] == NULL)
    {
        gapc_security_req[connection_idx] = app_gapc_security_request_msg_create(connection_idx, user_security_conf.auth);
    }
    return gapc_security_req[connection_idx];
}

struct gapc_security_cmd* app_easy_security_request_get_active(uint8_t connection_idx)
{
     ASSERT_WARNING(connection_idx < APP_EASY_SECURITY_MAX_CONNECTION);
     return app_easy_security_request_create_msg(connection_idx);
}

void app_easy_security_request(uint8_t connection_idx)
{
    struct gapc_security_cmd* req;
    ASSERT_WARNING(connection_idx < APP_EASY_SECURITY_MAX_CONNECTION);
    req = app_easy_security_request_get_active(connection_idx);
    ke_msg_send(req);
    gapc_security_req[connection_idx] = NULL;
}

/// @} APP_SECURITY
