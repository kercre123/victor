/**
 ****************************************************************************************
 *
 * @file hogpd_task.c
 *
 * @brief HID Over GATT Profile Device Role Task Implementation.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 * $Rev$
 *
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @addtogroup HOGPDTASK
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
#include "atts_util.h"

//HID Over GATT Profile Device Role Functions
#include "hogpd.h"
#include "hogpd_task.h"

#include "prf_utils.h"

/*
 * DEFINES
 ****************************************************************************************
 */

#define HIDS_CFG_FLAG_MANDATORY_MASK    ((uint32_t)0x000FD)
#define HIDS_CFG_FLAG_MAP_EXT_MASK      ((uint32_t)0x00102)
#define HIDS_CFG_FLAG_PROTO_MODE_MASK   ((uint32_t)0x00600)
#define HIDS_CFG_FLAG_KEYBOARD_MASK     ((uint32_t)0x0F800)
#define HIDS_CFG_FLAG_MOUSE_MASK        ((uint32_t)0x70000)

#define HIDS_CFG_REPORT_MANDATORY_MASK  ((uint32_t)0x7)
#define HIDS_CFG_REPORT_IN_MASK         ((uint32_t)0x8)

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref HOGPD_CREATE_DB_REQ message.
 * The handler adds BAS into the database using value of the features param.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int hogpd_create_db_req_handler(ke_msg_id_t const msgid,
                                       struct hogpd_create_db_req const *param,
                                       ke_task_id_t const dest_id,
                                       ke_task_id_t const src_id)
{
    // Service content flag - Without Report Characteristics
    uint32_t cfg_flag     = HIDS_CFG_FLAG_MANDATORY_MASK;
    // Content flag for Report Characteristics
    uint64_t cfg_flag_rep = 0;
    // Status
    uint8_t status = PRF_ERR_OK;
#ifndef USE_ONE_HIDS_INSTANCE
    // Service Instance Counter, Counter
    uint8_t i, j;
#else    
    // Counter
    int j;
#endif    

    // Number of attributes
    uint8_t nb_att;
    // Total Size
    uint16_t total_size;

    // Position in hids_att_db array
    uint8_t att_db_pos;
    // Current Report CHar. instance added in the DB
    uint8_t report_inst = 0;
    // Characteristic
    struct att_char_desc char_desc;
    // Handle
    uint16_t handle;
    // Optional Permissions
    uint16_t perm = 0;
    // Optional Properties
    uint8_t prop;
    // Report Char. Report Ref value
    struct hids_report_ref report_ref;

    // Save Profile ID
    hogpd_env.con_info.prf_id = TASK_HOGPD;

    // Check number of HIDS instances
    if ((param->hids_nb > 0) && (param->hids_nb <= HOGPD_NB_HIDS_INST_MAX))
    {
        // Save number of HIDS
        hogpd_env.hids_nb = param->hids_nb;

        // Initialize the Report ID
        report_ref.report_id = 0;

        // For each required HIDS instance
#ifndef USE_ONE_HIDS_INSTANCE
        for (i = 0; ((i < param->hids_nb) && (status == PRF_ERR_OK)); i++)
#else
        const int i = 0;
#endif        
        {
            // Check number of Report Char. instances
            if (param->cfg[i].features.report_nb <= HOGPD_NB_REPORT_INST_MAX)
            {
                // Save service features
                memcpy(&hogpd_env.features[i], &param->cfg[i].features, sizeof(struct hogpd_features));

                //--------------------------------------------------------------------
                // Compute cfg_flag without Report Characteristics
                //--------------------------------------------------------------------
                if ((param->cfg[i].features.svc_features & HOGPD_CFG_MAP_EXT_REF) == HOGPD_CFG_MAP_EXT_REF)
                {
                    cfg_flag |= HIDS_CFG_FLAG_MAP_EXT_MASK;
                }

                if ((param->cfg[i].features.svc_features & HOGPD_CFG_PROTO_MODE) == HOGPD_CFG_PROTO_MODE)
                {
                    cfg_flag |= HIDS_CFG_FLAG_PROTO_MODE_MASK;
                }

                if ((param->cfg[i].features.svc_features & HOGPD_CFG_KEYBOARD) == HOGPD_CFG_KEYBOARD)
                {
                    cfg_flag |= HIDS_CFG_FLAG_KEYBOARD_MASK;
                }

                if ((param->cfg[i].features.svc_features & HOGPD_CFG_MOUSE) == HOGPD_CFG_MOUSE)
                {
                    cfg_flag |= HIDS_CFG_FLAG_MOUSE_MASK;
                }

                //--------------------------------------------------------------------
                // Update cfg_flag_rep with Report Characteristics
                //--------------------------------------------------------------------
                for (j = 0; j < param->cfg[i].features.report_nb; j++)
                {
                    cfg_flag_rep |= ((uint64_t)HIDS_CFG_REPORT_MANDATORY_MASK << (4*j));

                    if ((param->cfg[i].features.report_char_cfg[j] & HOGPD_CFG_REPORT_FEAT) == HOGPD_CFG_REPORT_IN)
                    {
                        // Report Char. supports NTF => Client Characteristic Configuration Descriptor
                        cfg_flag_rep |= ((uint64_t)HIDS_CFG_REPORT_IN_MASK << (4*j));
                    }
                }

                // Reset nb_att and total_size
                nb_att = 0;
                total_size = 0;

                //--------------------------------------------------------------------
                // Compute nb_att and total_size without Report Characteristics
                //--------------------------------------------------------------------
                for (j = HOGPD_IDX_SVC; j <= HOGPD_IDX_BOOT_MOUSE_IN_REPORT_NTF_CFG; j++)
                {
                    if (((cfg_flag >> j) & 1) == 1)
                    {
                        nb_att++;
                        total_size += hids_att_db[j].max_length;
                    }
                }

                //--------------------------------------------------------------------
                // Update nb_att and total_size with Report Characteristics
                //--------------------------------------------------------------------
                for (j = 0; j < param->cfg[i].features.report_nb; j++)
                {
                    nb_att += 3;
                    total_size += hids_att_db[HOGPD_IDX_REPORT_CHAR].max_length;
                    total_size += hids_att_db[HOGPD_IDX_REPORT_VAL].max_length;
                    total_size += hids_att_db[HOGPD_IDX_REPORT_REP_REF].max_length;

                    if ((param->cfg[i].features.report_char_cfg[j] & HOGPD_CFG_REPORT_FEAT) == HOGPD_CFG_REPORT_IN)
                    {
                        nb_att++;
                        total_size += hids_att_db[HOGPD_IDX_REPORT_NTF_CFG].max_length;
                    }
                }

                //---------------------------------------------------------------------
                // Add Service in the Database
                //---------------------------------------------------------------------
                status = attmdb_add_service(&hogpd_env.shdl[i], dest_id, nb_att, 0, 0,
                                            total_size);

                //---------------------------------------------------------------------
                // Add Attributes
                //---------------------------------------------------------------------
                if (status == PRF_ERR_OK)
                {
                    bool add_att = false;

                    for (j = HOGPD_IDX_SVC; ((j <= (HOGPD_IDX_BOOT_MOUSE_IN_REPORT_NTF_CFG + 4*param->cfg[i].features.report_nb)) && (status == PRF_ERR_OK)); j++)
                    {
                        if (j <= HOGPD_IDX_BOOT_MOUSE_IN_REPORT_NTF_CFG)
                        {
                            if (((cfg_flag >> j) & 1) == 1)
                            {
                                att_db_pos = j;
                                add_att    = true;
                            }
                        }
                        else
                        {
                            if (((cfg_flag_rep >> (j - HOGPD_IDX_BOOT_MOUSE_IN_REPORT_NTF_CFG - 1)) & 1) == 1)
                            {
                                // Four attributes can be added for the Report Char.
                                att_db_pos = ((j - HOGPD_IDX_BOOT_MOUSE_IN_REPORT_NTF_CFG) % 4) + HOGPD_IDX_BOOT_MOUSE_IN_REPORT_NTF_CFG;
                                add_att    = true;
                            }
                        }

                        if (add_att)
                        {
                            status = attmdb_add_attribute(hogpd_env.shdl[i], hids_att_db[att_db_pos].max_length, ATT_UUID_16_LEN,
                                                          (uint8_t*)&(hids_att_db[att_db_pos].uuid), hids_att_db[att_db_pos].perm,
                                                          &handle);

                            //Set attribute value
                            if (status == PRF_ERR_OK)
                            {
                                if (hids_att_db[att_db_pos].length > 0)
                                {
                                    if (hids_att_db[att_db_pos].uuid == ATT_DECL_CHARACTERISTIC)
                                    {
                                        //Copy attribute characteristic
                                        memcpy(&char_desc,  hids_att_db[att_db_pos].value, sizeof(struct att_char_desc));

                                        //Save handle offset in attribute table
                                        hogpd_env.att_tbl[i][char_desc.attr_hdl[0] + report_inst] = handle - hogpd_env.shdl[i];

                                        if (char_desc.attr_hdl[0] == HOGPD_REPORT_CHAR)
                                        {
                                            report_inst++;
                                        }

                                        //Handle of Char. Value att
                                        handle++;
                                        //Update handle
                                        memcpy(&(char_desc.attr_hdl[0]), &handle, sizeof(uint16_t));

                                        //Set Char. Declaration att value
                                        handle--;
                                        status = attmdb_att_set_value(handle, ATT_CHAR_ELMT_SIZE, (uint8_t*)&char_desc);
                                    }
                                    else
                                    {
                                        status = attmdb_att_set_value(handle, hids_att_db[att_db_pos].length, hids_att_db[att_db_pos].value);
                                    }
                                }
                            }
                        }

                        add_att = false;
                    }
                }

                // Reset configuration flag and Report instance nb
                cfg_flag = HIDS_CFG_FLAG_MANDATORY_MASK;
                cfg_flag_rep = 0;
                report_inst = 0;

                // Attributes configuration
                if (status == PRF_ERR_OK)
                {
                    //Disable service
                    status = attmdb_svc_set_permission(hogpd_env.shdl[i], PERM(SVC, DISABLE));

                    //---------------------------------------------------------------------
                    // Set permanent values
                    //---------------------------------------------------------------------

                    // Set Report Map Char. and Included Svc values
                    if ((param->cfg[i].features.svc_features & HOGPD_CFG_MAP_EXT_REF) == HOGPD_CFG_MAP_EXT_REF)
                    {
                        // Set Included Service Value
                        attmdb_att_set_value(hogpd_env.shdl[i] + HOGPD_IDX_INCL_SVC, sizeof(struct att_incl_desc),
                                             (uint8_t *)&param->cfg[i].ext_rep_ref);

                        // Set Report Map Char. External Report Ref value
                        attmdb_att_set_value(hogpd_env.shdl[i] + HOGPD_IDX_REPORT_MAP_EXT_REP_REF, sizeof(uint16_t),
                                             (uint8_t *)&param->cfg[i].ext_rep_ref_uuid);
                    }

                    // Set HID Information Char. Value
                    attmdb_att_set_value(hogpd_env.shdl[i] + hogpd_env.att_tbl[i][HOGPD_HID_INFO_CHAR] + 1,
                                         sizeof(struct hids_hid_info),
                                         (uint8_t *)&param->cfg[i].hid_info);

                    // Set Report Char. Report Reference Value
                    for (j = 0; j < param->cfg[i].features.report_nb; j++)
                    {
                        // Save the Report ID
                        report_ref.report_id = param->cfg[i].features.report_id[j];
                        // Set Report Type
                        report_ref.report_type = (param->cfg[i].features.report_char_cfg[j] & HOGPD_CFG_REPORT_FEAT);

                        // Set value in the database
                        attmdb_att_set_value(hogpd_env.shdl[i] + hogpd_env.att_tbl[i][HOGPD_REPORT_CHAR + j] + 2,
                                             sizeof(struct hids_report_ref),
                                             (uint8_t *)&report_ref);
                    }

                    //---------------------------------------------------------------------
                    // Set optional properties and permissions
                    //---------------------------------------------------------------------

                    // Set Boot Keyboard Input Report Char properties and permissions
                    if ((param->cfg[i].features.svc_features & HOGPD_CFG_KEYBOARD) == HOGPD_CFG_KEYBOARD)
                    {
                        if ((param->cfg[i].features.svc_features & HOGPD_CFG_BOOT_KB_WR) == HOGPD_CFG_BOOT_KB_WR)
                        {
                            // Compute handle
                            handle = hogpd_env.shdl[i] + hogpd_env.att_tbl[i][HOGPD_BOOT_KB_IN_REPORT_CHAR];

                            prop = ATT_CHAR_PROP_RD | ATT_CHAR_PROP_WR | ATT_CHAR_PROP_NTF;
                            perm = PERM(RD, ENABLE) | PERM(WR, ENABLE) | PERM(NTF, ENABLE);

                            attmdb_att_partial_value_update(handle, 0, 1, &prop);
                            // Select characteristic value att
                            handle++;
                            attmdb_att_set_permission(handle, perm);
                        }
                    }

                    // Set Boot Mouse Input Report Char properties and permissions
                    if ((param->cfg[i].features.svc_features & HOGPD_CFG_MOUSE) == HOGPD_CFG_MOUSE)
                    {
                        if ((param->cfg[i].features.svc_features & HOGPD_CFG_BOOT_MOUSE_WR) == HOGPD_CFG_BOOT_MOUSE_WR)
                        {
                            // Compute handle
                            handle = hogpd_env.shdl[i] + hogpd_env.att_tbl[i][HOGPD_BOOT_MOUSE_IN_REPORT_CHAR];

                            prop = ATT_CHAR_PROP_RD | ATT_CHAR_PROP_WR | ATT_CHAR_PROP_NTF;
                            perm = PERM(RD, ENABLE) | PERM(WR, ENABLE) | PERM(NTF, ENABLE);

                            attmdb_att_partial_value_update(handle, 0, 1, &prop);
                            // Select characteristic value att
                            handle++;
                            attmdb_att_set_permission(handle, perm);
                        }
                    }

                    // Set Report Char. properties and permissions
                    for (j = 0; ((j < param->cfg[i].features.report_nb) && (status == PRF_ERR_OK)); j++)
                    {
                        // Compute handle
                        handle = hogpd_env.shdl[i] + hogpd_env.att_tbl[i][HOGPD_REPORT_CHAR + j];

                        switch (param->cfg[i].features.report_char_cfg[j] & HOGPD_CFG_REPORT_FEAT)
                        {
                            // Input Report
                            case (HOGPD_CFG_REPORT_IN):
                                prop = ATT_CHAR_PROP_RD | ATT_CHAR_PROP_NTF;
                                perm = PERM(RD, ENABLE) | PERM(NTF, ENABLE);

                                if ((param->cfg[i].features.report_char_cfg[j] & HOGPD_CFG_REPORT_WR) == HOGPD_CFG_REPORT_WR)
                                {
                                    prop |= ATT_CHAR_PROP_WR;
                                    perm |= PERM(WR, ENABLE);
                                }
                                break;

                            // Output Report
                            case (HOGPD_CFG_REPORT_OUT):
                                prop = ATT_CHAR_PROP_RD | ATT_CHAR_PROP_WR | ATT_CHAR_PROP_WR_NO_RESP;
                                perm = PERM(RD, ENABLE) | PERM(WR, ENABLE);
                                break;

                            // Feature Report
                            case (HOGPD_CFG_REPORT_FEAT):
                                prop = ATT_CHAR_PROP_RD | ATT_CHAR_PROP_WR;
                                perm = PERM(RD, ENABLE) | PERM(WR, ENABLE);
                                break;

                            default:
                                status = PRF_ERR_INVALID_PARAM;
                                break;
                        }

                            if (status == PRF_ERR_OK)
                            {
                                attmdb_att_partial_value_update(handle, 0, 1, &prop);
                                handle++;
                                attmdb_att_set_permission(handle, perm);
                            }
                    }
                }

                if (status == PRF_ERR_OK)
                {
                    //If we are here, database has been fulfilled with success, go to idle state
                    ke_state_set(TASK_HOGPD, HOGPD_IDLE);
                }
            }
            else
            {
                // Too many Report Char. Instances
                status = PRF_ERR_INVALID_PARAM;
            }
        }
    }
    else
    {
        // Too many HIDS instances
        status = PRF_ERR_INVALID_PARAM;
    }

    // Send confirmation to application
    struct hogpd_create_db_cfm * cfm = KE_MSG_ALLOC(HOGPD_CREATE_DB_CFM, src_id, TASK_HOGPD,
                                                    hogpd_create_db_cfm);

    cfm->status = status;

    ke_msg_send(cfm);

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref HOGPD_DISABLE_REQ message.
 * The handler disables the HID Over GATT Profile Device Role.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int hogpd_set_report_map_req_handler(ke_msg_id_t const msgid,
                                            struct hogpd_set_report_map_req const *param,
                                            ke_task_id_t const dest_id,
                                            ke_task_id_t const src_id)
{
    // Status
    uint8_t status = PRF_ERR_INVALID_PARAM;

    // Check Report map value length
    if (param->report_map_len <= HOGPD_REPORT_MAP_MAX_LEN)
    {
        // Check HIDS instance - The Report Map Characteristic is mandatory and has been added.
        if (param->hids_nb < hogpd_env.hids_nb)
        {
            // Set Report Map Char. Value
#ifndef USE_ONE_HIDS_INSTANCE
            status = attmdb_att_set_value(hogpd_env.shdl[param->hids_nb] + hogpd_env.att_tbl[param->hids_nb][HOGPD_REPORT_MAP_CHAR] + 1,
                                          param->report_map_len,
                                          (uint8_t *)&param->report_map[0]);
#else
            status = attmdb_att_set_value(hogpd_env.shdl[0] + hogpd_env.att_tbl[0][HOGPD_REPORT_MAP_CHAR] + 1,
                                          param->report_map_len,
                                          (uint8_t *)&param->report_map[0]);
#endif            
        }
        else
        {
            // Report Map Char. is not in the DB - Request disallowed
            status = PRF_ERR_REQ_DISALLOWED;
        }
    }

    if (status != PRF_ERR_OK)
    {
        // The connection doesn't exist, request disallowed
        prf_server_error_ind_send((prf_env_struct *)&hogpd_env, status,
                                  HOGPD_ERROR_IND, HOGPD_SET_REPORT_MAP_REQ);
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref HOGPD_ENABLE_REQ message.
 * The handler enables the HID Over GATT Profile Device Role.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int hogpd_enable_req_handler(ke_msg_id_t const msgid,
                                    struct hogpd_enable_req const *param,
                                    ke_task_id_t const dest_id,
                                    ke_task_id_t const src_id)
{
#ifndef USE_ONE_HIDS_INSTANCE
    // Counter for HIDS instance
    uint8_t hids_nb;
#endif    
    // Counter for Report Char. instance
    uint8_t report_nb;

    //Save the application task id
    hogpd_env.con_info.appid = src_id;
    //Save the connection handle associated to the profile
    hogpd_env.con_info.conidx = gapc_get_conidx(param->conhdl);

    // Check if the provided connection exist
    if (hogpd_env.con_info.conidx == GAP_INVALID_CONIDX)
    {
        // The connection doesn't exist, request disallowed
        prf_server_error_ind_send((prf_env_struct *)&hogpd_env, PRF_ERR_REQ_DISALLOWED,
                                  HOGPD_ERROR_IND, HOGPD_ENABLE_REQ);
    }
    else
    {
#ifndef USE_ONE_HIDS_INSTANCE
        for (hids_nb = 0; hids_nb < hogpd_env.hids_nb; hids_nb++)
#else
        const int hids_nb = 0;
#endif        
        {
            // Set default value for Protocol Mode Char. - Report Protocol Mode
            hogpd_env.proto_mode[hids_nb] = HOGP_REPORT_PROTOCOL_MODE;

            // If the Protocol Mode Characteristic is supported, update the value in the database
            if (hogpd_env.att_tbl[hids_nb][HOGPD_PROTO_MODE_CHAR] != 0x00)
            {
                attmdb_att_set_value(hogpd_env.shdl[hids_nb] + hogpd_env.att_tbl[hids_nb][HOGPD_PROTO_MODE_CHAR] + 1,
                                     sizeof(uint8_t),
                                     &hogpd_env.proto_mode[hids_nb]);
            }

            // Set Boot Keyboard Input Report Char. Ntf Cfg
            if (hogpd_env.att_tbl[hids_nb][HOGPD_BOOT_KB_IN_REPORT_CHAR] != 0x00)
            {
                hogpd_init_ntf_cfg(param->ntf_cfg[hids_nb].boot_kb_in_report_ntf_en,
                                   hogpd_env.shdl[hids_nb] + hogpd_env.att_tbl[hids_nb][HOGPD_BOOT_KB_IN_REPORT_CHAR] + 2,
                                   param->con_type,
                                   &hogpd_env.features[hids_nb].svc_features,
                                   HOGPD_BOOT_KB_IN_NTF_CFG_MASK);
            }

            // Set Boot Mouse Input Report Char. Ntf Cfg
            if (hogpd_env.att_tbl[hids_nb][HOGPD_BOOT_MOUSE_IN_REPORT_CHAR] != 0x00)
            {
                hogpd_init_ntf_cfg(param->ntf_cfg[hids_nb].boot_mouse_in_report_ntf_en,
                                   hogpd_env.shdl[hids_nb] + hogpd_env.att_tbl[hids_nb][HOGPD_BOOT_MOUSE_IN_REPORT_CHAR] + 2,
                                   param->con_type,
                                   &hogpd_env.features[hids_nb].svc_features,
                                   HOGPD_BOOT_MOUSE_IN_NTF_CFG_MASK);
            }

            // Set Report Char. Ntf Cfg
            for (report_nb = 0; report_nb < hogpd_env.features[hids_nb].report_nb; report_nb++)
            {
                // If Input Report
                if ((hogpd_env.features[hids_nb].report_char_cfg[report_nb] & HOGPD_CFG_REPORT_FEAT) == HOGPD_CFG_REPORT_IN)
                {
                    hogpd_init_ntf_cfg(param->ntf_cfg[hids_nb].report_ntf_en[report_nb],
                                       hogpd_env.shdl[hids_nb] + hogpd_env.att_tbl[hids_nb][HOGPD_REPORT_CHAR + report_nb] + 3,
                                       param->con_type,
                                       &hogpd_env.features[hids_nb].report_char_cfg[report_nb],
                                       HOGPD_REPORT_NTF_CFG_MASK);
                }
            }

            //Enable Attributes + Set Security Level
            attmdb_svc_set_permission(hogpd_env.shdl[hids_nb], param->sec_lvl);
        }

        // Go to connected state
        ke_state_set(TASK_HOGPD, HOGPD_CONNECTED);
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref HOGPD_REPORT_UPD_REQ message.
 *
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int hogpd_report_upd_req_handler(ke_msg_id_t const msgid,
                                        struct hogpd_report_info const *param,
                                        ke_task_id_t const dest_id,
                                        ke_task_id_t const src_id)
{
    // Status
    uint8_t status = PRF_ERR_INVALID_PARAM;

    // Check Connection Handle, HIDS instance, Report Instance and Report Length
    if ((param->conhdl == gapc_get_conhdl(hogpd_env.con_info.conidx)) &&
        (param->hids_nb <= hogpd_env.hids_nb) &&
        (param->report_nb <= hogpd_env.features[param->hids_nb].report_nb) &&
        (param->report_length <= HOGPD_REPORT_MAX_LEN))
        {
            status = hogpd_ntf_send(param->hids_nb, HOGPD_REPORT_CHAR, param->report_nb,
                                    param->report_length, (uint8_t *)&param->report[0]);
        }

    if (status != PRF_ERR_OK)
    {
        hogpd_ntf_cfm_send(status, HOGPD_REPORT_CFG, param->hids_nb, param->report_nb);
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref HOGPD_BOOT_REPORT_UPD_REQ message.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int hogpd_boot_report_upd_req_handler(ke_msg_id_t const msgid,
                                             struct hogpd_boot_report_info const *param,
                                            ke_task_id_t const dest_id,
                                            ke_task_id_t const src_id)
{
    // Status
    uint8_t status = PRF_ERR_INVALID_PARAM;

    // Check Connection Handle and HIDS instance
    if ((param->conhdl == gapc_get_conhdl(hogpd_env.con_info.conidx)) &&
        (param->hids_nb <= hogpd_env.hids_nb) &&
        (param->report_length <= HOGPD_BOOT_REPORT_MAX_LEN))
    {
         // Check provided characteristic code
        if ((param->char_code == HOGPD_BOOT_KB_IN_REPORT_CHAR) ||
            (param->char_code == HOGPD_BOOT_MOUSE_IN_REPORT_CHAR))
        {
#ifndef USE_ONE_HIDS_INSTANCE
            status = hogpd_ntf_send(param->hids_nb, param->char_code, 0,
                                    param->report_length, (uint8_t *)&param->boot_report[0]);
#else
            status = hogpd_ntf_send(0, param->char_code, 0,
                                    param->report_length, (uint8_t *)&param->boot_report[0]);
#endif            
        }
        else if ((param->char_code == HOGPD_BOOT_KB_OUT_REPORT_CHAR))
        {
#ifndef USE_ONE_HIDS_INSTANCE
            // Check if the Boot Keyboard Output Report Characteristic is supported
            if (hogpd_env.att_tbl[param->hids_nb][HOGPD_BOOT_KB_OUT_REPORT_CHAR] != 0x00)
            {
                // Set value in the database
                status = attmdb_att_set_value(hogpd_env.shdl[param->hids_nb] + hogpd_env.att_tbl[param->hids_nb][HOGPD_BOOT_KB_OUT_REPORT_CHAR] + 1,
                                              param->report_length, (uint8_t *)&param->boot_report[0]);
            }
#else
            // Check if the Boot Keyboard Output Report Characteristic is supported
            if (hogpd_env.att_tbl[0][HOGPD_BOOT_KB_OUT_REPORT_CHAR] != 0x00)
            {
                // Set value in the database
                status = attmdb_att_set_value(hogpd_env.shdl[0] + hogpd_env.att_tbl[0][HOGPD_BOOT_KB_OUT_REPORT_CHAR] + 1,
                                              param->report_length, (uint8_t *)&param->boot_report[0]);
            }
#endif            
            else
            {
                status = PRF_ERR_FEATURE_NOT_SUPPORTED;
            }
        }
    }

    if (status != PRF_ERR_OK)
    {
        hogpd_ntf_cfm_send(status, param->char_code, param->hids_nb, 0);
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
    // Status
    uint8_t status = PRF_APP_ERROR;
    // Ntf Cfg value
    uint16_t ntf_cfg;
    // Pointer for Report value
    uint8_t *report;
    // Length of the report value
    att_size_t report_length;
    // Attribute info
    uint8_t char_code, hids_nb, report_nb;

    // Check the Connection Handle
    if (KE_IDX_GET(src_id) == hogpd_env.con_info.conidx)
    {
        // Get the attribute that has been written
        status = hogpd_get_att(param->handle, &char_code, &hids_nb, &report_nb);

        if (status == PRF_ERR_OK)
        {
            switch(char_code)
            {
                // Client Characteristic Configuration Descriptors
                case (HOGPD_BOOT_KB_IN_REPORT_CFG):
                case (HOGPD_BOOT_MOUSE_IN_REPORT_CFG):
                case (HOGPD_REPORT_CFG):
                    // Extract the value
                    ntf_cfg = co_read16p(&param->value[0]);
                    status = hogpd_ntf_cfg_ind_send(ntf_cfg, param->handle, char_code, hids_nb, report_nb);
                    break;

                // Protocol Mode
                case (HOGPD_PROTO_MODE_CHAR):
                    // Check if provided value is correct
                    if ((param->value[0] == HOGP_BOOT_PROTOCOL_MODE) || (param->value[0] == HOGP_REPORT_PROTOCOL_MODE))
                    {
                        // Set value in the database
                        attmdb_att_set_value(param->handle, sizeof(uint8_t), (uint8_t *)&param->value[0]);

                        // Update value in the environment
                        hogpd_env.proto_mode[hids_nb] = param->value[0];

                        // Inform APP of configuration change
                        struct hogpd_proto_mode_ind * ind = KE_MSG_ALLOC(HOGPD_PROTO_MODE_IND,
                                                                         hogpd_env.con_info.appid, TASK_HOGPD,
                                                                         hogpd_proto_mode_ind);

                        ind->conhdl = gapc_get_conhdl(hogpd_env.con_info.conidx);
                        ind->proto_mode     = param->value[0];
                        ind->hids_nb        = hids_nb;

                        ke_msg_send(ind);
                    }
                    // Write without Response => no need to send an error
                    break;

                // HID Control Point Characteristic
                case (HOGPD_HID_CTNL_PT_CHAR):
                    // Check if provided value is correct
                    if ((param->value[0] == HOGP_CTNL_PT_SUSPEND) || (param->value[0] == HOGP_CTNL_PT_EXIT_SUSPEND))
                    {
                        // Set value in the database
                        attmdb_att_set_value(param->handle, sizeof(uint8_t), (uint8_t *)&param->value[0]);

                        // Inform APP of configuration change
                        struct hogpd_ctnl_pt_ind * ind = KE_MSG_ALLOC(HOGPD_CTNL_PT_IND,
                                                                      hogpd_env.con_info.appid, TASK_HOGPD,
                                                                      hogpd_ctnl_pt_ind);

                        ind->conhdl = gapc_get_conhdl(hogpd_env.con_info.conidx);
                        ind->hid_ctnl_pt    = param->value[0];
                        ind->hids_nb        = hids_nb;

                        ke_msg_send(ind);
                    }
                    break;

                // Boot Keyboard Input Report
                case (HOGPD_BOOT_KB_IN_REPORT_CHAR):
                // Boot Keyboard Output Report
                case (HOGPD_BOOT_KB_OUT_REPORT_CHAR):
                // Boot Mouse Input Report
                case (HOGPD_BOOT_MOUSE_IN_REPORT_CHAR):

                    if (param->length <= HOGPD_BOOT_REPORT_MAX_LEN)
                    {
                        // Set value in the database
                        attmdb_att_set_value(param->handle, param->length, (uint8_t *)&param->value[0]);

                        // Inform APP of configuration change
                        struct hogpd_boot_report_info *report_ind = KE_MSG_ALLOC_DYN(HOGPD_BOOT_REPORT_IND,
                                                                                  hogpd_env.con_info.appid, TASK_HOGPD,
                                                                                     hogpd_boot_report_info,
                                                                                     param->length);

                        report_ind->conhdl = gapc_get_conhdl(hogpd_env.con_info.conidx);
                        memcpy(&report_ind->boot_report[0], &param->value[0], param->length);
                        report_ind->hids_nb = hids_nb;
                        report_ind->report_length = param->length;
                        report_ind->char_code = char_code;

                        ke_msg_send(report_ind);
                    }
                    else
                    {
                        status = PRF_APP_ERROR;
                    }

                    break;

                // Report Characteristic
                case (HOGPD_REPORT_CHAR):
                    if ((param->length + param->offset) <= HOGPD_REPORT_MAX_LEN)
                    {
                        // First part of the Report value
                        if (param->offset == 0)
                        {
                            // Set value in the database
                            attmdb_att_set_value(param->handle, param->length, (uint8_t *)&param->value[0]);
                        }
                        else
                        {
                            // Complete the value stored in the database
                            attmdb_att_update_value(param->handle, param->length, param->offset,
                                                    (uint8_t *)&param->value[0]);
                        }

                        if(param->last)
                        {
                            // Get complete report value and length
                            attmdb_att_get_value(param->handle, &report_length, &report);

                            // Inform APP about the written report value
                            struct hogpd_report_info * ind = KE_MSG_ALLOC_DYN(HOGPD_REPORT_IND,
                                                                             hogpd_env.con_info.appid, TASK_HOGPD,
                                                                              hogpd_report_info,
                                                                             report_length);

                            ind->conhdl = gapc_get_conhdl(hogpd_env.con_info.conidx);
                            ind->hids_nb        = hids_nb;
                            ind->report_nb      = report_nb;

                            ind->report_length = report_length;
                            memcpy(&ind->report, report, report_length);

                            ke_msg_send(ind);
                        }
                    }
                    else
                    {
                        status = PRF_APP_ERROR;
                    }
                    break;
                default:
                	break;
            }
        }

        if (param->response != 0x00)
        {
            // Send write response
            atts_write_rsp_send(hogpd_env.con_info.conidx, param->handle, status);
        }
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles @ref GATT_NOTIFY_CMP_EVT message meaning that Report notification
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
    if(param->req_type == GATTC_NOTIFY)
    {
        // Attribute information
        uint8_t char_code, hids_nb, report_nb;

        // Retrieve attribute information using the handle
        hogpd_get_att(hogpd_env.ntf_handle, &char_code, &hids_nb, &report_nb);

        // Send a HOGPD_NTF_SEND_CFM message to the application
        hogpd_ntf_cfm_send(param->status, char_code, hids_nb, report_nb);
    }

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Disconnection indication to HOGPD.
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
    // Check if this is the link that got disconnected
    if (KE_IDX_GET(src_id) == hogpd_env.con_info.conidx)
    {
        hogpd_disable(param->conhdl);
    }
    // Message is consumed
    return (KE_MSG_CONSUMED);
}

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// Disabled State handler definition.
const struct ke_msg_handler hogpd_disabled[] =
{
    {HOGPD_CREATE_DB_REQ,               (ke_msg_func_t) hogpd_create_db_req_handler}
};

/// Idle State handler definition.
const struct ke_msg_handler hogpd_idle[] =
{
    {HOGPD_ENABLE_REQ,                  (ke_msg_func_t) hogpd_enable_req_handler},
    {HOGPD_SET_REPORT_MAP_REQ,          (ke_msg_func_t) hogpd_set_report_map_req_handler}
};

/// Idle State handler definition.
const struct ke_msg_handler hogpd_connected[] =
{
    {HOGPD_REPORT_UPD_REQ,          (ke_msg_func_t) hogpd_report_upd_req_handler},
    {HOGPD_BOOT_REPORT_UPD_REQ,     (ke_msg_func_t) hogpd_boot_report_upd_req_handler},
    {GATTC_WRITE_CMD_IND,           (ke_msg_func_t) gattc_write_cmd_ind_handler},
    {GATTC_CMP_EVT,                 (ke_msg_func_t) gattc_cmp_evt_handler},
};

/// Default State handlers definition
const struct ke_msg_handler hogpd_default_state[] =
{
    {GAPC_DISCONNECT_IND,        (ke_msg_func_t)gapc_disconnect_ind_handler},
};

/// Specifies the message handler structure for every input state.
const struct ke_state_handler hogpd_state_handler[HOGPD_STATE_MAX] =
{
    [HOGPD_DISABLED]       = KE_STATE_HANDLER(hogpd_disabled),
    [HOGPD_IDLE]           = KE_STATE_HANDLER(hogpd_idle),
    [HOGPD_CONNECTED]      = KE_STATE_HANDLER(hogpd_connected),
};

/// Specifies the message handlers that are common to all states.
const struct ke_state_handler hogpd_default_handler = KE_STATE_HANDLER(hogpd_default_state);

/// Defines the place holder for the states of all the task instances.
ke_state_t hogpd_state[HOGPD_IDX_MAX] __attribute__((section("retention_mem_area0"),zero_init)); //@RETENTION MEMORY

#endif /* #if (BLE_HID_DEVICE) */

/// @} HOGPDTASK
