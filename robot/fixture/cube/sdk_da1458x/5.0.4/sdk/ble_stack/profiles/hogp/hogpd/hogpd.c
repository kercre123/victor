/**
 ****************************************************************************************
 *
 * @file hogpd.c
 *
 * @brief HID Over GATT Profile HID Device Role Implementation.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 * $Rev$
 *
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @addtogroup HOGPD
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "rwip_config.h"

#if (BLE_HID_DEVICE)

#include "gap.h"
#include "gattc_task.h"
#include "attm_util.h"
#include "hogpd.h"
#include "hogpd_task.h"
#include "atts.h"
#include "attm_db.h"
#include "prf_utils.h"

#if (BLE_APP_KEYBOARD)
#include "app_kbd.h"
#endif

/*
 * HIDS ATTRIBUTES DEFINITION
 ****************************************************************************************
 */

/// HID Service
const att_svc_desc_t hid_svc = ATT_SVC_HID;

/// HID Information Characteristic
const struct att_char_desc hids_hid_info_char      = ATT_CHAR(ATT_CHAR_PROP_RD,
                                                                HOGPD_HID_INFO_CHAR,
                                                                ATT_CHAR_HID_INFO);

/// HID Control Point Characteristic
const struct att_char_desc hids_hid_ctnl_pt_char   = ATT_CHAR(ATT_CHAR_PROP_WR_NO_RESP,
                                                                HOGPD_HID_CTNL_PT_CHAR,
                                                                ATT_CHAR_HID_CTNL_PT);

/// Report Map Characteristic
const struct att_char_desc hids_report_map_char    = ATT_CHAR(ATT_CHAR_PROP_RD,
                                                                HOGPD_REPORT_MAP_CHAR,
                                                                ATT_CHAR_REPORT_MAP);

/// Protocol Mode Characteristic
const struct att_char_desc hids_proto_mode_char    = ATT_CHAR((ATT_CHAR_PROP_RD | ATT_CHAR_PROP_WR_NO_RESP),
                                                                HOGPD_PROTO_MODE_CHAR,
                                                                ATT_CHAR_PROTOCOL_MODE);

/// Boot Keyboard Input Report Characteristic
const struct att_char_desc hids_boot_kb_in_report_char
                                                    = ATT_CHAR((ATT_CHAR_PROP_RD | ATT_CHAR_PROP_NTF),
                                                                 HOGPD_BOOT_KB_IN_REPORT_CHAR,
                                                                 ATT_CHAR_BOOT_KB_IN_REPORT);

/// Boot Keyboard Output Report Characteristic
const struct att_char_desc hids_boot_kb_out_report_char
                                                    = ATT_CHAR((ATT_CHAR_PROP_RD | ATT_CHAR_PROP_WR | ATT_CHAR_PROP_WR_NO_RESP),
                                                                 HOGPD_BOOT_KB_OUT_REPORT_CHAR,
                                                                 ATT_CHAR_BOOT_KB_OUT_REPORT);

/// Boot Mouse Input Report Characteristic
const struct att_char_desc hids_boot_mouse_in_report_char
                                                    = ATT_CHAR((ATT_CHAR_PROP_RD | ATT_CHAR_PROP_NTF),
                                                                HOGPD_BOOT_MOUSE_IN_REPORT_CHAR,
                                                                ATT_CHAR_BOOT_MOUSE_IN_REPORT);

/// Report Characteristic
const struct att_char_desc hids_report_char        = ATT_CHAR(ATT_CHAR_PROP_RD,
                                                                HOGPD_REPORT_CHAR,
                                                                ATT_CHAR_REPORT);

/// Full HIDS Database Description - Used to add attributes into the database
const struct attm_desc hids_att_db[HOGPD_IDX_NB] =
{
    // HID Service Declaration
    [HOGPD_IDX_SVC]                             = {ATT_DECL_PRIMARY_SERVICE,
                                                   PERM(RD, ENABLE),
                                                   sizeof(hid_svc), sizeof(hid_svc),
                                                   (uint8_t *)&hid_svc},

    // HID Service Declaration
    [HOGPD_IDX_INCL_SVC]                        = {ATT_DECL_INCLUDE,
                                                   PERM(RD, ENABLE),
                                                   sizeof(struct att_incl_desc), 0,
                                                   NULL},

    // HID Information Characteristic Declaration
    [HOGPD_IDX_HID_INFO_CHAR]                   = {ATT_DECL_CHARACTERISTIC,
                                                   PERM(RD, ENABLE),
                                                   sizeof(hids_hid_info_char), sizeof(hids_hid_info_char),
                                                   (uint8_t *)&hids_hid_info_char},
    // HID Information Characteristic Value
    [HOGPD_IDX_HID_INFO_VAL]                    = {ATT_CHAR_HID_INFO,
                                                   PERM(RD, ENABLE),
                                                   sizeof(struct hids_hid_info), 0,
                                                   NULL},

    // HID Control Point Characteristic Declaration
    [HOGPD_IDX_HID_CTNL_PT_CHAR]                = {ATT_DECL_CHARACTERISTIC,
                                                   PERM(RD, ENABLE),
                                                   sizeof(hids_hid_ctnl_pt_char), sizeof(hids_hid_ctnl_pt_char),
                                                   (uint8_t *)&hids_hid_ctnl_pt_char},
    // HID Control Point Characteristic Value
    [HOGPD_IDX_HID_CTNL_PT_VAL]                 = {ATT_CHAR_HID_CTNL_PT,
                                                   PERM(WR, ENABLE),
                                                   sizeof(uint8_t), 0,
                                                   NULL},

    // Report Map Characteristic Declaration
    [HOGPD_IDX_REPORT_MAP_CHAR]                 = {ATT_DECL_CHARACTERISTIC,
                                                   PERM(RD, ENABLE),
                                                   sizeof(hids_report_map_char), sizeof(hids_report_map_char),
                                                   (uint8_t *)&hids_report_map_char},
    // Report Map Characteristic Value
    [HOGPD_IDX_REPORT_MAP_VAL]                  = {ATT_CHAR_REPORT_MAP,
                                                   PERM(RD, ENABLE),
                                                   HOGPD_REPORT_MAP_MAX_LEN*sizeof(uint8_t), 0,
                                                   NULL},

    // Report Map Characteristic - External Report Reference Descriptor
    [HOGPD_IDX_REPORT_MAP_EXT_REP_REF]          = {ATT_DESC_EXT_REPORT_REF,
                                                   PERM(RD, ENABLE),
                                                   sizeof(uint16_t), 0,
                                                   NULL},

    // Protocol Mode Characteristic Declaration
    [HOGPD_IDX_PROTO_MODE_CHAR]                 = {ATT_DECL_CHARACTERISTIC,
                                                   PERM(RD, ENABLE),
                                                   sizeof(hids_proto_mode_char), sizeof(hids_proto_mode_char),
                                                   (uint8_t *)&hids_proto_mode_char},
    // Protocol Mode Characteristic Value
    [HOGPD_IDX_PROTO_MODE_VAL]                  = {ATT_CHAR_PROTOCOL_MODE,
                                                   (PERM(RD, ENABLE) | PERM(WR, ENABLE)),
                                                   sizeof(uint8_t), 0,
                                                   NULL},

    // Boot Keyboard Input Report Characteristic Declaration
    [HOGPD_IDX_BOOT_KB_IN_REPORT_CHAR]          = {ATT_DECL_CHARACTERISTIC,
                                                   PERM(RD, ENABLE),
                                                   sizeof(hids_boot_kb_in_report_char), sizeof(hids_boot_kb_in_report_char),
                                                   (uint8_t *)&hids_boot_kb_in_report_char},
    // Boot Keyboard Input Report Characteristic Value
    [HOGPD_IDX_BOOT_KB_IN_REPORT_VAL]           = {ATT_CHAR_BOOT_KB_IN_REPORT,
                                                   (PERM(RD, ENABLE) | PERM(NTF, ENABLE)),
                                                   HOGPD_BOOT_REPORT_MAX_LEN*sizeof(uint8_t), 0,
                                                   NULL},
    // Boot Keyboard Input Report Characteristic - Client Characteristic Configuration Descriptor
    [HOGPD_IDX_BOOT_KB_IN_REPORT_NTF_CFG]       = {ATT_DESC_CLIENT_CHAR_CFG,
                                                   (PERM(RD, ENABLE) | PERM(WR, ENABLE)),
                                                   sizeof(uint16_t), 0,
                                                   NULL},

    // Boot Keyboard Output Report Characteristic Declaration
    [HOGPD_IDX_BOOT_KB_OUT_REPORT_CHAR]         = {ATT_DECL_CHARACTERISTIC,
                                                   PERM(RD, ENABLE),
                                                   sizeof(hids_boot_kb_out_report_char), sizeof(hids_boot_kb_out_report_char),
                                                   (uint8_t *)&hids_boot_kb_out_report_char},
    // Boot Keyboard Output Report Characteristic Value
    [HOGPD_IDX_BOOT_KB_OUT_REPORT_VAL]          = {ATT_CHAR_BOOT_KB_OUT_REPORT,
                                                   (PERM(RD, ENABLE) | PERM(WR, ENABLE)),
                                                   HOGPD_BOOT_REPORT_MAX_LEN*sizeof(uint8_t), 0,
                                                   NULL},

    // Boot Mouse Input Report Characteristic Declaration
    [HOGPD_IDX_BOOT_MOUSE_IN_REPORT_CHAR]       = {ATT_DECL_CHARACTERISTIC,
                                                   PERM(RD, ENABLE),
                                                   sizeof(hids_boot_mouse_in_report_char), sizeof(hids_boot_mouse_in_report_char),
                                                   (uint8_t *)&hids_boot_mouse_in_report_char},
    // Boot Mouse Input Report Characteristic Value
    [HOGPD_IDX_BOOT_MOUSE_IN_REPORT_VAL]        = {ATT_CHAR_BOOT_MOUSE_IN_REPORT,
                                                   (PERM(RD, ENABLE) | PERM(NTF, ENABLE)),
                                                   HOGPD_BOOT_REPORT_MAX_LEN*sizeof(uint8_t), 0,
                                                   NULL},
    // Boot Mouse Input Report Characteristic - Client Characteristic Configuration Descriptor
    [HOGPD_IDX_BOOT_MOUSE_IN_REPORT_NTF_CFG]    = {ATT_DESC_CLIENT_CHAR_CFG,
                                                   (PERM(RD, ENABLE) | PERM(WR, ENABLE)),
                                                   sizeof(uint16_t), 0,
                                                   NULL},

    // Report Characteristic Declaration
    [HOGPD_IDX_REPORT_CHAR]                     = {ATT_DECL_CHARACTERISTIC,
                                                   PERM(RD, ENABLE),
                                                   sizeof(hids_report_char), sizeof(hids_report_char),
                                                   (uint8_t *)&hids_report_char},
    // Report Characteristic Value
    [HOGPD_IDX_REPORT_VAL]                      = {ATT_CHAR_REPORT,
                                                   PERM(RD, ENABLE),
                                                   HOGPD_REPORT_MAX_LEN*sizeof(uint8_t), 0,
                                                   NULL},
    // Report Characteristic - Report Reference Descriptor
    [HOGPD_IDX_REPORT_REP_REF]                  = {ATT_DESC_REPORT_REF,
                                                   PERM(RD, ENABLE),
                                                   sizeof(struct hids_report_ref), 0,
                                                   NULL},
    // Report Characteristic - Client Characteristic Configuration Descriptor
    [HOGPD_IDX_REPORT_NTF_CFG]                  = {ATT_DESC_CLIENT_CHAR_CFG,
                                                   (PERM(RD, ENABLE) | PERM(WR, ENABLE)),
                                                   sizeof(uint16_t), 0,
                                                   NULL},
};

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

struct hogpd_env_tag hogpd_env __attribute__((section("retention_mem_area0"),zero_init)); //@RETENTION MEMORY

/// HOGPD task descriptor
static const struct ke_task_desc TASK_DESC_HOGPD = {hogpd_state_handler, &hogpd_default_handler, hogpd_state, HOGPD_STATE_MAX, HOGPD_IDX_MAX};


/*
 * EXPORTED FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */

void hogpd_init(void)
{
    // Reset the find me target environment
    memset(&hogpd_env, 0, sizeof(hogpd_env));

    // Create HOGPD task
    ke_task_create(TASK_HOGPD, &TASK_DESC_HOGPD);

    // Go to IDLE state
    ke_state_set(TASK_HOGPD, HOGPD_DISABLED);
}

void hogpd_init_ntf_cfg(uint16_t ntf_cfg, uint16_t handle, uint8_t con_type,
                        uint8_t *flag, uint8_t mask)
{
    // Default Ntf Cfg value
    uint16_t value = PRF_CLI_STOP_NTFIND;

    //Written value is 0 is discovery connection, given value if normal
    if(con_type == PRF_CON_NORMAL)
    {
        memcpy(&value, &ntf_cfg, sizeof(uint16_t));

        // Save new status in the environment
        if (value == PRF_CLI_START_NTF)
        {
            *flag |= HOGPD_REPORT_NTF_CFG_MASK;
        }
    }

    attmdb_att_set_value(handle, sizeof(uint16_t), (uint8_t *)&value);
}

uint8_t hogpd_ntf_send(uint8_t hids_nb, uint8_t char_code, uint8_t report_nb,
                       uint16_t report_len, uint8_t *p_report)
{
    // Status
    uint8_t status = PRF_ERR_OK;
    // Handle
    uint16_t handle;
    // Mask
    uint8_t mask;
    // Flag
    uint8_t flag;
    
#ifndef USE_ONE_HIDS_INSTANCE
#define HIDS_NB hids_nb
#else
#define HIDS_NB 0
#endif
    

    // Check if the required Report Char. is supported
    if (hogpd_env.att_tbl[HIDS_NB][char_code + report_nb] != 0x00)
    {
        handle = hogpd_env.shdl[HIDS_NB] + hogpd_env.att_tbl[HIDS_NB][char_code + report_nb] + 1;

        // Set value in the database
        attmdb_att_set_value(handle, report_len, p_report);

        // Check if notifications have been enabled for the required characteristic
        switch(char_code)
        {
            case HOGPD_REPORT_CHAR:
                flag = hogpd_env.features[HIDS_NB].report_char_cfg[report_nb];
                mask = HOGPD_REPORT_NTF_CFG_MASK;
                break;
            case HOGPD_BOOT_KB_IN_REPORT_CHAR:
                flag = hogpd_env.features[HIDS_NB].svc_features;
                mask = HOGPD_BOOT_KB_IN_NTF_CFG_MASK;
                break;
            case HOGPD_BOOT_MOUSE_IN_REPORT_CHAR:
                flag = hogpd_env.features[HIDS_NB].svc_features;
                mask = HOGPD_BOOT_MOUSE_IN_NTF_CFG_MASK;
                break;
            default:
                return PRF_ERR_INVALID_PARAM;
        }

        if ((flag & mask) != mask)
        {
            status = PRF_ERR_NTF_DISABLED;
        }

        // Check if notifications are enabled
        if (status == PRF_ERR_OK)
        {
            // keep notified handle
            hogpd_env.ntf_handle = handle;

            // Send notification through GATT
            prf_server_send_event((prf_env_struct *)&hogpd_env, false, handle);
        }
    }
    else
    {
        status = PRF_ERR_FEATURE_NOT_SUPPORTED;
    }

    return status;
}

void hogpd_ntf_cfm_send(uint8_t status, uint8_t char_code, uint8_t hids_nb, uint8_t report_nb)
{
    struct hogpd_ntf_sent_cfm *cfm = KE_MSG_ALLOC(HOGPD_NTF_SENT_CFM, hogpd_env.con_info.appid,
                                                  TASK_HOGPD, hogpd_ntf_sent_cfm);

    cfm->conhdl    = gapc_get_conhdl(hogpd_env.con_info.conidx);
#ifndef USE_ONE_HIDS_INSTANCE
    cfm->hids_nb   = hids_nb;
#else
    cfm->hids_nb   = 0;
#endif    
    cfm->report_nb = report_nb;
    cfm->char_code = char_code;
    cfm->status    = status;

    ke_msg_send(cfm);
}

uint8_t hogpd_ntf_cfg_ind_send(uint16_t ntf_cfg, uint16_t handle, uint8_t cfg_code,
                               uint8_t hids_nb, uint8_t report_nb)
{
#ifndef USE_ONE_HIDS_INSTANCE
#define HIDS_NB hids_nb
#else
#define HIDS_NB 0
#endif
    
    // Status
    uint8_t status = PRF_APP_ERROR;
    // Mask
    uint8_t mask = 0;
    // Pointer to the flag saving the notification configuration
    uint8_t *flag = &hogpd_env.features[HIDS_NB].svc_features;

    // Check if provided value is correct
    if ((ntf_cfg == PRF_CLI_STOP_NTFIND) || (ntf_cfg == PRF_CLI_START_NTF))
    {
        // Set value in the database
        attmdb_att_set_value(handle, sizeof(uint16_t), (uint8_t *)&ntf_cfg);

        if (cfg_code == HOGPD_BOOT_KB_IN_REPORT_CFG)
        {
            mask = HOGPD_BOOT_KB_IN_NTF_CFG_MASK;
        }
        else if (cfg_code == HOGPD_BOOT_MOUSE_IN_REPORT_CFG)
        {
            mask = HOGPD_BOOT_MOUSE_IN_NTF_CFG_MASK;
        }
        else if (cfg_code == HOGPD_REPORT_CFG)
        {
            mask = HOGPD_REPORT_NTF_CFG_MASK;
            flag = &hogpd_env.features[HIDS_NB].report_char_cfg[report_nb];
        }

        // Save new status in the environment
        if (ntf_cfg == PRF_CLI_START_NTF)
        {
            *flag |= mask;
        }
        else
        {
            *flag &= ~mask;
        }

        // Inform APP of configuration change
        struct hogpd_ntf_cfg_ind * ind = KE_MSG_ALLOC(HOGPD_NTF_CFG_IND,
                                                      hogpd_env.con_info.appid, TASK_HOGPD,
                                                      hogpd_ntf_cfg_ind);


        ind->conhdl = gapc_get_conhdl(hogpd_env.con_info.conidx);
        ind->ntf_en = ntf_cfg;
        ind->hids_nb   = HIDS_NB;
        ind->report_nb = report_nb;
        ind->cfg_code  = cfg_code;

        ke_msg_send(ind);

        status = PRF_ERR_OK;
    }

    return status;
}

uint8_t hogpd_get_att(uint16_t handle, uint8_t *char_code, uint8_t *hids_nb, uint8_t *report_nb)
{
    // Status, attribute found or not
    uint8_t found = PRF_APP_ERROR;
    // Offset
    uint8_t offset;

#ifndef USE_ONE_HIDS_INSTANCE
    // Counters
    uint8_t svc, att;
    
    for (svc = 0; ((svc < hogpd_env.hids_nb) && (found == PRF_APP_ERROR)); svc++)
#else
    // Counter
    int att;
    
    const int svc = 0;
#endif    
    {
        *hids_nb = svc;
        offset = handle - hogpd_env.shdl[svc];

        for (att = HOGPD_HID_INFO_CHAR; ((att < HOGPD_CHAR_MAX) && (found == PRF_APP_ERROR)); att++)
        {
            // Characteristic Value Attribute
            if (offset == (hogpd_env.att_tbl[svc][att] + 1))
            {
                *char_code = att;

                if (att >= HOGPD_REPORT_CHAR)
                {
                    *report_nb = att - HOGPD_REPORT_CHAR;
                    *char_code = HOGPD_REPORT_CHAR;
                }

                found = PRF_ERR_OK;
            }

            if (found == PRF_APP_ERROR)
            {
                if ((att == HOGPD_BOOT_KB_IN_REPORT_CHAR) || (att == HOGPD_BOOT_MOUSE_IN_REPORT_CHAR))
                {
                    if (offset == hogpd_env.att_tbl[svc][att] + 2)
                    {
                        *report_nb = 0;
                        *char_code = att | HOGPD_DESC_MASK;

                        found = PRF_ERR_OK;
                    }
                }
            }

            if (found == PRF_APP_ERROR)
            {
                if (att >= HOGPD_REPORT_CHAR)
                {
                    if (offset == hogpd_env.att_tbl[svc][att] + 3)
                    {
                        *report_nb = att - HOGPD_REPORT_CHAR;
                        *char_code = HOGPD_REPORT_CHAR | HOGPD_DESC_MASK;

                        found = PRF_ERR_OK;
                    }
                }
            }
        }
    }

    return found;
}

void hogpd_disable(uint16_t conhdl) 
{
    // Report Char. instance counter
    uint8_t report_nb;

    struct hogpd_disable_ind *ind = KE_MSG_ALLOC(HOGPD_DISABLE_IND,
                                                 hogpd_env.con_info.appid, TASK_HOGPD,
                                                 hogpd_disable_ind);

    ind->conhdl = conhdl;

#ifndef USE_ONE_HIDS_INSTANCE
    // HIDS instance counter
    uint8_t hids_nb;
    
    for (hids_nb = 0; hids_nb < hogpd_env.hids_nb; hids_nb++)

#else
    const int hids_nb = 0;
#endif    
    {
        // Disable HIDS in database
        attmdb_svc_set_permission(hogpd_env.shdl[hids_nb], PERM_RIGHT_DISABLE);

        // Save Boot Keyboard Input Report Char. Ntf Cfg
        if (hogpd_env.att_tbl[hids_nb][HOGPD_BOOT_KB_IN_REPORT_CHAR] != 0x00)
        {
            if ((hogpd_env.features[hids_nb].svc_features & HOGPD_BOOT_KB_IN_NTF_CFG_MASK)
                                                         == HOGPD_BOOT_KB_IN_NTF_CFG_MASK)
            {
                ind->ntf_cfg[hids_nb].boot_kb_in_report_ntf_en = PRF_CLI_START_NTF;
            }
        }

        // Save Boot Mouse Input Report Char. Ntf Cfg
        if (hogpd_env.att_tbl[hids_nb][HOGPD_BOOT_MOUSE_IN_REPORT_CHAR] != 0x00)
        {
            if ((hogpd_env.features[hids_nb].svc_features & HOGPD_BOOT_MOUSE_IN_NTF_CFG_MASK)
                                                         == HOGPD_BOOT_MOUSE_IN_NTF_CFG_MASK)
            {
                ind->ntf_cfg[hids_nb].boot_mouse_in_report_ntf_en = PRF_CLI_START_NTF;
            }
        }

        //Reset notifications bit field
        hogpd_env.features[hids_nb].svc_features &= ~(HOGPD_BOOT_KB_IN_NTF_CFG_MASK | HOGPD_BOOT_MOUSE_IN_NTF_CFG_MASK);

        // Save Report Char. Ntf Cfg
        for (report_nb = 0; report_nb < hogpd_env.features[hids_nb].report_nb; report_nb++)
        {
            if ((hogpd_env.features[hids_nb].report_char_cfg[report_nb] & HOGPD_REPORT_NTF_CFG_MASK)
                                                                       == HOGPD_REPORT_NTF_CFG_MASK)
            {
                ind->ntf_cfg[hids_nb].report_ntf_en[report_nb] = PRF_CLI_START_NTF;
            }

            //Reset notifications bit field
            hogpd_env.features[hids_nb].report_char_cfg[report_nb] &= ~HOGPD_REPORT_NTF_CFG_MASK;
        }
    }

    ke_msg_send(ind);

    //Go to idle state
    ke_state_set(TASK_HOGPD, HOGPD_IDLE);
}

#endif /* BLE_HID_DEVICE */

/// @} HOGPD
