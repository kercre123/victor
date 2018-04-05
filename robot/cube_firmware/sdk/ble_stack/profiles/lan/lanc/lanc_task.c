/**
 ****************************************************************************************
 *
 * @file lanc_task.c
 *
 * @brief Location and Navigation Profile Collector Task implementation.
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 * $ Rev $
 *
 ****************************************************************************************
 */


/**
 ****************************************************************************************
 * @addtogroup LANCTASK
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "rwble_config.h" 

#if (BLE_LN_COLLECTOR)
#include "lan_common.h"
#include "gap.h"
#include "attm.h"
#include "lanc.h"
#include "lanc_task.h"
#include "gattc_task.h"
#include "attm_cfg.h"

/*
 * STRUCTURES
 ****************************************************************************************
 */

/// State machine used to retrieve Location and Navigation service characteristics information
const struct prf_char_def lanc_lans_char[LANP_LANS_CHAR_MAX] =
{
    /// LN Feature
    [LANP_LANS_LN_FEAT_CHAR]    = {ATT_CHAR_LN_FEAT,
                                    ATT_MANDATORY,
                                    ATT_CHAR_PROP_RD},
    /// LN Measurement
    [LANP_LANS_LOC_SPEED_CHAR]  = {ATT_CHAR_LOC_SPEED,
                                    ATT_MANDATORY,
                                    ATT_CHAR_PROP_NTF},
    /// Position Quality
    [LANP_LANS_POS_Q_CHAR]      = {ATT_CHAR_POS_QUALITY,
                                    ATT_OPTIONAL,
                                    ATT_CHAR_PROP_RD},
    /// SC Control Point
    [LANP_LANS_LN_CTNL_PT_CHAR] = {ATT_CHAR_LN_CNTL_PT,
                                    ATT_OPTIONAL,
                                    ATT_CHAR_PROP_WR | ATT_CHAR_PROP_IND},
    /// Navigation
    [LANP_LANS_NAVIG_CHAR]  = {ATT_CHAR_NAVIGATION,
                                    ATT_OPTIONAL,
                                    ATT_CHAR_PROP_NTF},
};

/// State machine used to retrieve Location and Navigation service characteristic descriptor information
const struct prf_char_desc_def lanc_lns_char_desc[LANC_DESC_MAX] =
{
    /// Location and Speed Char. - Client Characteristic Configuration
    [LANC_DESC_LOC_SPEED_CL_CFG]  = {ATT_DESC_CLIENT_CHAR_CFG,
                                        ATT_MANDATORY,
                                        LANP_LANS_LOC_SPEED_CHAR},

    /// Control Point Char. - Client Characteristic Configuration
    [LANC_DESC_LN_CTNL_PT_CL_CFG] = {ATT_DESC_CLIENT_CHAR_CFG,
                                        ATT_OPTIONAL,
                                        LANP_LANS_LN_CTNL_PT_CHAR},

    /// Navigation Char. - Client Characteristic Configuration
    [LANC_DESC_NAVIGATION_CL_CFG] = {ATT_DESC_CLIENT_CHAR_CFG,
                                        ATT_OPTIONAL,
                                        LANP_LANS_NAVIG_CHAR},
};

/*
 * LOCAL FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Performs discovery procedure for CP service
 * @param[in] cppc_env environment variable
 * @return status of the operation
 ****************************************************************************************
 */

static uint8_t lanc_discovery_procedure (struct lanc_env_tag *lanc_env)
{
    // Status
    uint8_t status = PRF_ERR_STOP_DISC_CHAR_MISSING;

    /* -------------------------------------------------
     * SERVICE DISCOVERY -------------------------------
     * ------------------------------------------------- */
    if (lanc_env->last_req == ATT_SVC_LOCATION_AND_NAVIGATION)
    {
        if (lanc_env->nb_svc > 0)
        {
            // Check if service handles are OK
            if ((lanc_env->lans.svc.shdl != ATT_INVALID_HANDLE) &&
                (lanc_env->lans.svc.ehdl != ATT_INVALID_HANDLE) &&
                (lanc_env->lans.svc.shdl < lanc_env->lans.svc.ehdl))
            {
                // Status is OK
                status = PRF_ERR_OK;

                // Discover all LNS characteristics
                lanc_env->last_req = ATT_DECL_CHARACTERISTIC;
                prf_disc_char_all_send(&(lanc_env->con_info), &(lanc_env->lans.svc));
            } // Handles are not corrects, the Location and Navigation Service has not been found, stop
        }// The Location and Navigation Service has not been found, stop discovery
    }

    /* -------------------------------------------------
     * CHARACTERISTICS DISCOVERY -----------------------
     * ------------------------------------------------- */
    else if (lanc_env->last_req == ATT_DECL_CHARACTERISTIC)
    {
        // Check if mandatory properties have been found and if properties are correct
        status = prf_check_svc_char_validity(LANP_LANS_CHAR_MAX, lanc_env->lans.chars, lanc_lans_char);

        // Check for characteristic properties.
        if (status == PRF_ERR_OK)
        {
            lanc_env->last_req = LANP_LANS_LOC_SPEED_CHAR;

            // Find the Client Characteristic Configuration Descriptor for the location and speed characteristic
            prf_disc_char_desc_send(&(lanc_env->con_info),
                                    &(lanc_env->lans.chars[LANP_LANS_LOC_SPEED_CHAR]));
        }
    }

    /* -------------------------------------------------
     * DESCRIPTORS DISCOVERY ---------------------------
     * ------------------------------------------------- */
    else
    {
        // Discovery over ?
        bool disc_over = true;

        if (lanc_env->last_req == LANP_LANS_LOC_SPEED_CHAR)
        {
            // Check if the Control Point Characteristic has been found
            if (lanc_env->lans.chars[LANP_LANS_LN_CTNL_PT_CHAR].char_hdl != ATT_INVALID_SEARCH_HANDLE)
            {
                // Status is OK
                status = PRF_ERR_OK;

                // Find the Client Characteristic Configuration Descriptor for the Control Point characteristic
                lanc_env->last_req = LANP_LANS_LN_CTNL_PT_CHAR;
                prf_disc_char_desc_send(&(lanc_env->con_info),
                                        &(lanc_env->lans.chars[LANP_LANS_LN_CTNL_PT_CHAR]));

                // Discovery continues
                disc_over = false;
            }

            else //case there is no Control Point but Navigation
            {
                // Check if the Control Point Characteristic has been found
                if (lanc_env->lans.chars[LANP_LANS_NAVIG_CHAR].char_hdl != ATT_INVALID_SEARCH_HANDLE)
                {
                    // Status is OK
                    status = PRF_ERR_OK;

                    // Find the Client Characteristic Configuration Descriptor for the SC Control Point characteristic
                    lanc_env->last_req = LANP_LANS_NAVIG_CHAR;
                    prf_disc_char_desc_send(&(lanc_env->con_info),
                                            &(lanc_env->lans.chars[LANP_LANS_NAVIG_CHAR]));

                    // Discovery continues
                    disc_over = false;
                }
            }
        }

        else if (lanc_env->last_req == LANP_LANS_LN_CTNL_PT_CHAR)
        {
            // Check if the Control Point Characteristic has been found
            if (lanc_env->lans.chars[LANP_LANS_NAVIG_CHAR].char_hdl != ATT_INVALID_SEARCH_HANDLE)
            {
                // Status is OK
                status = PRF_ERR_OK;
                // Find the Client Characteristic Configuration Descriptor for the SC Control Point characteristic
                lanc_env->last_req = LANP_LANS_NAVIG_CHAR;
                prf_disc_char_desc_send(&(lanc_env->con_info),
                                        &(lanc_env->lans.chars[LANP_LANS_NAVIG_CHAR]));
                // Discovery continues
                disc_over = false;
            }
        }

        else
        {
            // Discovery is over
            ASSERT_ERR(lanc_env->last_req == LANP_LANS_NAVIG_CHAR);
        }

        if (disc_over)
        {
            status = prf_check_svc_char_desc_validity(LANC_DESC_MAX,
                                                      lanc_env->lans.descs,
                                                      lanc_lns_char_desc,
                                                      lanc_env->lans.chars);

            if (status == PRF_ERR_OK)
            {
                // Reset number of services
                lanc_env->nb_svc = 0;

                // Register in GATT for notifications/indications
                prf_register_atthdl2gatt(&lanc_env->con_info, &lanc_env->lans.svc);

                // Send the content of the service to the HL
                struct lanc_lans_content_ind *ind = KE_MSG_ALLOC(LANC_LNS_CONTENT_IND,
                                                                  lanc_env->con_info.appid,
                                                                  lanc_env->con_info.prf_id,
                                                                  lanc_lans_content_ind);

                ind->conhdl = gapc_get_conhdl(lanc_env->con_info.conidx);
                memcpy(&ind->lns, &lanc_env->lans, sizeof(struct lanc_lns_content));

                // Send the message
                ke_msg_send(ind);

                // Stop discovery procedure.
                lanc_send_cmp_evt(lanc_env, LANC_ENABLE_OP_CODE, status);
            }
        }
    }
    return status;
}

/**
 ****************************************************************************************
 * @brief Gets correct read handle according to the request
 * @param[in] cppc_env environment variable
 * @param[in] param Pointer to the parameters of the message.
 * @return handle
 ****************************************************************************************
 */
static uint16_t lanc_get_read_handle_req (struct lanc_env_tag *lanc_env, struct lanc_read_cmd *param)
{
    // Attribute Handle
    uint16_t handle = ATT_INVALID_SEARCH_HANDLE;

    switch (param->read_code)
    {
        // Read LN Feature
        case (LANC_RD_LN_FEAT):
        {
            handle = lanc_env->lans.chars[LANP_LANS_LN_FEAT_CHAR].val_hdl;
        } break;

        // Read Position Quality
        case (LANC_RD_POS_Q):
        {
            handle = lanc_env->lans.chars[LANP_LANS_POS_Q_CHAR].val_hdl;
        } break;

        // Read Location and Speed Characteristic Client Char. Cfg. Descriptor Value
        case (LANC_RD_WR_LOC_SPEED_CL_CFG):
        {
            handle = lanc_env->lans.descs[LANC_DESC_LOC_SPEED_CL_CFG].desc_hdl;
        } break;

        // Read Unread Alert Characteristic Client Char. Cfg. Descriptor Value
        case (LANC_RD_WR_LN_CTNL_PT_CFG):
        {
            handle = lanc_env->lans.descs[LANC_DESC_LN_CTNL_PT_CL_CFG].desc_hdl;
        } break;

        // Read Navigation Characteristic Server Char. Cfg. Descriptor Value
        case (LANC_RD_WR_NAVIGATION_CFG):
        {
            handle = lanc_env->lans.descs[LANC_DESC_NAVIGATION_CL_CFG].desc_hdl;
        } break;


        default:
        {
            handle = ATT_INVALID_SEARCH_HANDLE;
        } break;
    }

    return handle;
}

/**
 ****************************************************************************************
 * @brief Gets correct write handle according to the request
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] cppc_env environment variable
 * @param[out] handle handle
 * @return status of the operation
 ****************************************************************************************
 */
static uint8_t lanc_get_write_desc_handle_req (struct lanc_cfg_ntfind_cmd *param, struct lanc_env_tag *lanc_env, uint16_t *handle)
{
    // Status
    uint8_t status    = PRF_ERR_OK;

    switch(param->desc_code)
    {
        // Write Location and speed Characteristic Client Char. Cfg. Descriptor Value
        case (LANC_RD_WR_LOC_SPEED_CL_CFG):
        {
            if (param->ntfind_cfg <= PRF_CLI_START_NTF)
            {
                *handle = lanc_env->lans.descs[LANC_DESC_LOC_SPEED_CL_CFG].desc_hdl;

                // The descriptor is mandatory
                ASSERT_ERR(*handle != ATT_INVALID_SEARCH_HANDLE);
            }
            else
            {
                status = PRF_ERR_INVALID_PARAM;
            }
        } break;

        // Write LN Control Point Characteristic Client Char. Cfg. Descriptor Value
        case (LANC_RD_WR_LN_CTNL_PT_CFG):
        {
            if ((param->ntfind_cfg == PRF_CLI_STOP_NTFIND) ||
                (param->ntfind_cfg == PRF_CLI_START_IND))
            {
                *handle = lanc_env->lans.descs[LANC_DESC_LN_CTNL_PT_CL_CFG].desc_hdl;

                if (*handle == ATT_INVALID_SEARCH_HANDLE)
                {
                    // The descriptor has not been found.
                    status = PRF_ERR_INEXISTENT_HDL;
                }
            }
            else
            {
                status = PRF_ERR_INVALID_PARAM;
            }
        } break;

        // Write Navigation Characteristic Client Char. Cfg. Descriptor Value
        case (LANC_RD_WR_NAVIGATION_CFG):
        {
            if (param->ntfind_cfg <= PRF_CLI_START_NTF)
            {
                *handle = lanc_env->lans.descs[LANC_DESC_NAVIGATION_CL_CFG].desc_hdl;

                if (*handle == ATT_INVALID_SEARCH_HANDLE)
                {
                    // The descriptor has not been found.
                    status = PRF_ERR_INEXISTENT_HDL;
                }
            }
            else
            {
                status = PRF_ERR_INVALID_PARAM;
            }
        }
        break;

        default:
        {
            status = PRF_ERR_INVALID_PARAM;
        } break;
    }

    return status;
}

/**
 ****************************************************************************************
 * @brief Unpacks location and speed data and sends the indication
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] cppc_env environment variable
 * @return offset
 ****************************************************************************************
 */
static uint8_t lanc_unpack_loc_speed_ind (struct gattc_event_ind const *param, struct lanc_env_tag *lanc_env)
{
    // Offset
    uint8_t offset = LANP_LAN_LOC_SPEED_MIN_LEN;

    // LN Measurement value has been received
    struct lanc_value_ind *ind = KE_MSG_ALLOC(LANC_VALUE_IND,
                                                lanc_env->con_info.appid,
                                                lanc_env->con_info.prf_id,
                                                lanc_value_ind);

    // Connection Handle
    ind->conhdl   = gapc_get_conhdl(lanc_env->con_info.conidx);
    // Attribute code
    ind->att_code = LANC_NTF_LOC_SPEED;

    if ((param->length >= LANP_LAN_LOC_SPEED_MIN_LEN) &&
            (param->length <= LANP_LAN_LOC_SPEED_MAX_LEN))
    {
        // Flags
        ind->value.loc_speed.flags = co_read16p(&param->value[0]);

        if (ind->value.loc_speed.flags & LANP_LSPEED_INST_SPEED_PRESENT)
        {
            //Unpack instantaneous speed
            ind->value.loc_speed.inst_speed = co_read16p(&param->value[offset]);
            offset += 2;
        }

        if (ind->value.loc_speed.flags & LANP_LSPEED_TOTAL_DISTANCE_PRESENT)
        {
            //Unpack Total distance (24 bits)
            ind->value.loc_speed.total_dist = co_read24p(&param->value[offset]);
            offset += 3;
        }

        if (ind->value.loc_speed.flags & LANP_LSPEED_LOCATION_PRESENT)
        {
            //Unpack Location
            ind->value.loc_speed.latitude = co_read32p(&param->value[offset]);
            offset += 4;
            ind->value.loc_speed.longitude = co_read32p(&param->value[offset]);
            offset += 4;
        }

        if (ind->value.loc_speed.flags & LANP_LSPEED_ELEVATION_PRESENT)
        {
            // Unpack Elevation (24 bits)
            ind->value.loc_speed.elevation = co_read24p(&param->value[offset]);
            offset += 3;
        }

        if (ind->value.loc_speed.flags & LANP_LSPEED_HEADING_PRESENT)
        {
            // Unpack heading
            ind->value.loc_speed.heading = co_read16p(&param->value[offset]);
            offset += 2;
        }

        if (ind->value.loc_speed.flags & LANP_LSPEED_ROLLING_TIME_PRESENT)
        {
            // Unpack rolling time
            ind->value.loc_speed.rolling_time = param->value[offset];
            offset++;
        }

        if (ind->value.loc_speed.flags & LANP_LSPEED_UTC_TIME_PRESENT)
        {
            //Unpack UTC time
            offset += prf_unpack_date_time(
                    (uint8_t *) &(param->value[offset]), &(ind->value.loc_speed.date_time));
        }
    }

    // Send the message
    ke_msg_send(ind);

    return offset;
}

/**
 ****************************************************************************************
 * @brief Unpacks Navigation and sends the indication
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] cppc_env environment variable
 * @return offset
 ****************************************************************************************
 */

static uint8_t lanc_unpack_navigation_ind (struct gattc_event_ind const *param, struct lanc_env_tag *lanc_env)
{
    // Offset
    uint8_t offset = LANP_LAN_NAVIGATION_MIN_LEN;

    // LN Measurement value has been received
    struct lanc_value_ind *ind = KE_MSG_ALLOC(LANC_VALUE_IND,
            lanc_env->con_info.appid,
            lanc_env->con_info.prf_id,
            lanc_value_ind);

    // Connection Handle
    ind->conhdl   = gapc_get_conhdl(lanc_env->con_info.conidx);
    // Attribute code
    ind->att_code = LANC_NTF_NAVIGATION;

    if ((param->length >= LANP_LAN_NAVIGATION_MIN_LEN) &&
            (param->length <= LANP_LAN_NAVIGATION_MAX_LEN))
    {
        // Flags
        ind->value.navigation.flags = co_read16p(&param->value[0]);
        // Bearing
        ind->value.navigation.bearing = co_read16p(&param->value[2]);
        // Heading
        ind->value.navigation.heading = co_read16p(&param->value[4]);

        if (ind->value.navigation.flags & LANP_NAVI_REMAINING_DIS_PRESENT)
        {
            //Unpack remaining distance (24 bits)
            ind->value.navigation.remaining_distance = co_read24p(&param->value[offset]);
            offset += 3;
        }

        if (ind->value.navigation.flags & LANP_NAVI_REMAINING_VER_DIS_PRESENT)
        {
            //Unpack remaining vertical distance (24 bits)
            ind->value.navigation.remaining_ver_distance = co_read24p(&param->value[offset]);
            offset += 3;
        }

        if (ind->value.navigation.flags & LANP_NAVI_ESTIMATED_TIME_OF_ARRIVAL_PRESENT)
        {
            //Unpack time
            offset += prf_unpack_date_time(
                    (uint8_t *) &(param->value[offset]), &(ind->value.navigation.estimated_arrival_time));
        }
    }

    // Send the message
    ke_msg_send(ind);

    return offset;
}


/**
 ****************************************************************************************
 * @brief Unpacks position quality
 * @param[in] param Pointer to the parameters of the message
 * @param[out] ind Pointer to the value indication
 * @return offset
 ****************************************************************************************
 */
static uint8_t lanc_unpack_pos_q_ind (struct gattc_read_ind const *param, struct lanc_value_ind *ind)
{
    /*----------------------------------------------------
     * Unpack Position Quality ---------------------------
     *----------------------------------------------------*/
    // Offset
    uint8_t offset = LANP_LAN_POSQ_MIN_LEN;

    // Flags
    ind->value.pos_q.flags = co_read16p(&param->value[0]);

    if (ind->value.pos_q.flags & LANP_POSQ_NUMBER_OF_BEACONS_IN_SOLUTION_PRESENT)
    {
        //Unpack beacons in solution
        ind->value.pos_q.n_beacons_solution = param->value[offset];
        offset++;
    }

    if (ind->value.pos_q.flags & LANP_POSQ_NUMBER_OF_BEACONS_IN_VIEW_PRESENT)
    {
        //Unpack beacons in view
        ind->value.pos_q.n_beacons_view = param->value[offset];
        offset++;
    }

    if (ind->value.pos_q.flags & LANP_POSQ_TIME_TO_FIRST_FIX_PRESENT)
    {
        //Unpack time first fix
        ind->value.pos_q.time_first_fix = co_read16p(&param->value[offset]);
        offset += 2;
    }

    if (ind->value.pos_q.flags & LANP_POSQ_EHPE_PRESENT)
    {
        //Unpack ehpe
        ind->value.pos_q.ehpe = co_read32p(&param->value[offset]);
        offset += 4;
    }

    if (ind->value.pos_q.flags & LANP_POSQ_EVPE_PRESENT)
    {
        //Unpack evpe
        ind->value.pos_q.evpe = co_read32p(&param->value[offset]);
        offset += 4;
    }

    if (ind->value.pos_q.flags & LANP_POSQ_HDOP_PRESENT)
    {
        //Unpack hdop
        ind->value.pos_q.hdop = param->value[offset];
        offset++;
    }

    if (ind->value.pos_q.flags & LANP_POSQ_VDOP_PRESENT)
    {
        //Unpack vdop
        ind->value.pos_q.vdop = param->value[offset];
        offset++;
    }

    return offset;
}

/**
 ****************************************************************************************
 * @brief Packs Control Point data
 * @param[in] param Pointer to the parameters of the message.
 * @param[out] req packed message
 * @param[out] status status of the operation
 * @return length
 ****************************************************************************************
 */
static uint8_t lanc_pack_ln_ctnl_pt_req (struct lanc_ln_ctnl_pt_cfg_cmd *param, uint8_t *req, uint8_t *status)
{
    // Request Length
    uint8_t req_len = LANP_LAN_LN_CNTL_PT_REQ_MIN_LEN;

    // Set the operation code
    req[0] = param->ln_ctnl_pt.op_code;

    // Fulfill the message according to the operation code
    switch (param->ln_ctnl_pt.op_code)
    {
        case (LANP_LN_CTNL_PT_SET_CUMUL_VALUE):
        {
            // Set the cumulative value (24 bits)
            co_write24p(&req[req_len], param->ln_ctnl_pt.value.cumul_val);
            // Update length
            req_len += 3;
        } break;

        case (LANP_LN_CTNL_PT_MASK_LSPEED_CHAR_CT):
        {
            // Set mask content
            co_write16p(&req[req_len], param->ln_ctnl_pt.value.mask_content);
            // Update length
            req_len += 2;
        } break;

        case (LANP_LN_CTNL_PT_NAVIGATION_CONTROL):
        {
            // Set control value
            req[req_len] = param->ln_ctnl_pt.value.control_value;
            // Update length
            req_len++;
        } break;

        case (LANP_LN_CTNL_PT_REQ_NAME_OF_ROUTE):
        case (LANP_LN_CTNL_PT_SELECT_ROUTE):
        {
            // Set route number
            co_write16p(&req[req_len], param->ln_ctnl_pt.value.route_number);
            // Update length
            req_len += 2;
        } break;

        case (LANP_LN_CTNL_PT_SET_FIX_RATE):
        {
            // Set the fix rate
            req[req_len] = param->ln_ctnl_pt.value.fix_rate;
            // Update length
            req_len++;
        } break;

        case (LANP_LN_CTNL_PT_SET_ELEVATION):
        {
            // Set elevation (24 bits)
            co_write24p(&req[req_len], param->ln_ctnl_pt.value.elevation);
            // Update length
            req_len += 3;
        } break;

        case (LANP_LN_CTNL_PT_REQ_NUMBER_OF_ROUTES):
        {
            // Nothing more to do
        } break;

        default:
        {
            *status = PRF_ERR_INVALID_PARAM;
        } break;
    }

    return req_len;
}

/**
 ****************************************************************************************
 * @brief Unpacks Control Point data and sends the indication
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] cppc_env environment variable
 * @return offset
 ****************************************************************************************
 */
static uint8_t lanc_unpack_ln_ctln_pt_ind (struct gattc_event_ind const *param, struct lanc_env_tag *lanc_env)
{
    // Offset
    uint8_t offset = LANP_LAN_LN_CNTL_PT_RSP_MIN_LEN;

    // Control Point value has been received
    struct lanc_ln_ctnl_pt_rsp_ind *ind = KE_MSG_ALLOC_DYN(LANC_LN_CTNL_PT_RSP_IND,
                                            lanc_env->con_info.appid,
                                            lanc_env->con_info.prf_id,
                                            lanc_ln_ctnl_pt_rsp_ind,
                                            param->length);

    // Connection Handle
    ind->conhdl   = gapc_get_conhdl(lanc_env->con_info.conidx);

    // Requested operation code
    ind->rsp.req_op_code = param->value[1];
    // Response value
    ind->rsp.resp_value  = param->value[2];


    if ((ind->rsp.resp_value == LANP_LN_CTNL_PT_RESP_SUCCESS) && (param->length >= 3))
    {
        switch (ind->rsp.req_op_code)
        {
            case (LANP_LN_CTNL_PT_REQ_NUMBER_OF_ROUTES):
            {
                ind->rsp.value.number_of_routes = co_read16p(&param->value[offset]);
                offset += 2;

            } break;

            case (LANP_LN_CTNL_PT_REQ_NAME_OF_ROUTE):
            {
                // Get the length of the route
                ind->rsp.value.route.length = (param->length - 3);
                for (int i = 0; i<ind->rsp.value.route.length; i++)
                {
                    ind->rsp.value.route.name[i] = param->value[i + 3];
                    offset++;
                }
            } break;

            case (LANP_LN_CTNL_PT_SET_CUMUL_VALUE):
            case (LANP_LN_CTNL_PT_MASK_LSPEED_CHAR_CT):
            case (LANP_LN_CTNL_PT_NAVIGATION_CONTROL):
            case (LANP_LN_CTNL_PT_SELECT_ROUTE):
            case (LANP_LN_CTNL_PT_SET_FIX_RATE):
            case (LANP_LN_CTNL_PT_SET_ELEVATION):
            {
                // No parameters
            } break;

            default:
            {

            } break;
        }
    }

    // Send the message
    ke_msg_send(ind);

    return offset;
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref LANC_ENABLE_CMD message.
 * @param[in] msgid Id of the message received.
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance.
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int lanc_enable_cmd_handler(ke_msg_id_t const msgid,
                                    struct lanc_enable_cmd *param,
                                    ke_task_id_t const dest_id,
                                    ke_task_id_t const src_id)
{
    // Status
    uint8_t status     = PRF_ERR_OK;
    // Message status
    uint8_t msg_status = KE_MSG_CONSUMED;
    // Location and Navigation Profile Collector Role Task Environment
    struct lanc_env_tag *lanc_env;
    // Connection Information
    struct prf_con_info con_info;

    // Check if the provided conhdl is valid
    if (gapc_get_conidx(param->conhdl) != GAP_INVALID_CONIDX)
    {
        // Fill the Connection Information structure
        con_info.conidx = gapc_get_conidx(param->conhdl);
        con_info.prf_id = dest_id;
        con_info.appid  = src_id;

        // Add an environment for the provided connection
        status = PRF_CLIENT_ENABLE(con_info, param, lanc);
    }
    else
    {
        status = PRF_ERR_REQ_DISALLOWED;
    }

    if (status == PRF_ERR_OK)
    {
        // Get the newly created environment
        lanc_env = PRF_CLIENT_GET_ENV(dest_id, lanc);

        // Keep the connection info
        memcpy(&lanc_env->con_info, &con_info, sizeof(struct prf_con_info));

        // Start discovering
        if (param->con_type == PRF_CON_DISCOVERY)
        {
            // Configure the environment for a discovery procedure
            lanc_env->last_req  = ATT_SVC_LOCATION_AND_NAVIGATION;
            // Force the operation value
            param->operation     = LANC_ENABLE_OP_CODE;

            prf_disc_svc_send(&(lanc_env->con_info), ATT_SVC_LOCATION_AND_NAVIGATION);

            // Keep the operation
            lanc_env->operation = (void *)param;
            // Do not free the message
            msg_status = KE_MSG_NO_FREE;

            // Go to BUSY state
            ke_state_set(dest_id, LANC_BUSY);
        }
        // Bond information are provided
        else
        {
            // Keep the provided database content
            memcpy(&lanc_env->lans, &param->lans, sizeof(struct lanc_lns_content));

            // Register in GATT for notifications/indications
            prf_register_atthdl2gatt(&lanc_env->con_info, &lanc_env->lans.svc);

            // Send a complete event status to the application
            lanc_send_cmp_evt(lanc_env, LANC_ENABLE_OP_CODE, PRF_ERR_OK);
        }
    }
    else if (status == PRF_ERR_FEATURE_NOT_SUPPORTED)
    {
        // The message will be forwarded towards the good task instance
        msg_status = KE_MSG_NO_FREE;
    }
    else
    {
        // The request is disallowed (profile already enabled for this connection, or not enough memory, ...)
        lanc_send_no_conn_cmp_evt(dest_id, src_id, param->conhdl, LANC_ENABLE_OP_CODE);
    }

    return (int)msg_status;
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref LANC_READ_CMD message.
 * @param[in] msgid Id of the message received.
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance.
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int lanc_read_cmd_handler(ke_msg_id_t const msgid,
                                  struct lanc_read_cmd *param,
                                  ke_task_id_t const dest_id,
                                  ke_task_id_t const src_id)
{
    // Message status
    uint8_t msg_status = KE_MSG_CONSUMED;

    // Get the address of the environment
    struct lanc_env_tag *lanc_env = PRF_CLIENT_GET_ENV(dest_id, lanc);

    if (lanc_env != NULL)
    {
        // Attribute Handle
        uint16_t hdl;
        // Status
        uint8_t status     = PRF_ERR_OK;

        ASSERT_ERR(ke_state_get(dest_id) != LANC_IDLE);

        // Check the provided connection handle
        if (gapc_get_conidx(param->conhdl) == lanc_env->con_info.conidx)
        {
            // Check the current state
            if (ke_state_get(dest_id) == LANC_BUSY)
            {
                // Keep the request for later
                msg_status = KE_MSG_SAVED;
            }
            else    // State is LANC_CONNECTED
            {
                ASSERT_ERR(lanc_env->operation == NULL);

                // Get the handler
                hdl = lanc_get_read_handle_req (lanc_env, param);

                // Check if handle is viable
                if (hdl != ATT_INVALID_SEARCH_HANDLE)
                {
                    // Force the operation value
                    param->operation = LANC_READ_OP_CODE;

                    // Store the command structure
                    lanc_env->operation = param;
                    msg_status = KE_MSG_NO_FREE;

                    // Send the read request
                    prf_read_char_send(&(lanc_env->con_info), lanc_env->lans.svc.shdl,
                                       lanc_env->lans.svc.ehdl, hdl);

                    // Go to the Busy state
                    ke_state_set(dest_id, LANC_BUSY);
                }
                else
                {
                    status = PRF_ERR_INEXISTENT_HDL;
                }
            }
        }
        else
        {
            status = PRF_ERR_INVALID_PARAM;
        }

        if (status != PRF_ERR_OK)
        {
            // Send the complete event message to the task id stored in the environment
            lanc_send_cmp_evt(lanc_env, LANC_READ_OP_CODE, status);
        }
    }
    else
    {
        // No connection exists, send the status to the requester
        lanc_send_no_conn_cmp_evt(dest_id, src_id, param->conhdl, LANC_READ_OP_CODE);
    }

    return (int)msg_status;
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref LANC_CFG_NTFIND_CMD message.
 * @param[in] msgid Id of the message received.
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance.
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int lanc_cfg_ntfind_cmd_handler(ke_msg_id_t const msgid,
                                        struct lanc_cfg_ntfind_cmd *param,
                                        ke_task_id_t const dest_id,
                                        ke_task_id_t const src_id)
{
    // Message status
    uint8_t msg_status = KE_MSG_CONSUMED;
    // Get the address of the environment
    struct lanc_env_tag *lanc_env = PRF_CLIENT_GET_ENV(dest_id, lanc);

    if (lanc_env != NULL)
    {
        // Status
        uint8_t status = PRF_ERR_OK;
        // Handle
        uint16_t handle = ATT_INVALID_SEARCH_HANDLE;

        do
        {
            ASSERT_ERR(ke_state_get(dest_id) != LANC_IDLE);

            // Check the provided connection handle
            if (gapc_get_conidx(param->conhdl) != lanc_env->con_info.conidx)
            {
                status = PRF_ERR_INVALID_PARAM;
                break;
            }

            if (ke_state_get(dest_id) == LANC_BUSY)
            {
                // Another procedure is pending, keep the command for later
                msg_status = KE_MSG_SAVED;
                // Status is PRF_ERR_OK, no message will be sent to the application
                break;
            }

            // state = LANC_CONNECTED
            ASSERT_ERR(lanc_env->operation == NULL);

            // Get handle
            status = lanc_get_write_desc_handle_req (param, lanc_env, &handle);
        } while (0);

        if (status == PRF_ERR_OK)
        {
            ASSERT_ERR(handle != ATT_INVALID_SEARCH_HANDLE);

            // Set the operation code
            param->operation = LANC_CFG_NTF_IND_OP_CODE;

            // Store the command structure
            lanc_env->operation = param;
            msg_status = KE_MSG_NO_FREE;

            // Go to the Busy state
            ke_state_set(dest_id, LANC_BUSY);

            // Send GATT Write Request
            prf_gatt_write_ntf_ind(&lanc_env->con_info, handle, param->ntfind_cfg);
        }
        else
        {
            lanc_send_cmp_evt(lanc_env, LANC_CFG_NTF_IND_OP_CODE, status);
        }
    }
    else
    {
        lanc_send_no_conn_cmp_evt(dest_id, src_id, param->conhdl, LANC_CFG_NTF_IND_OP_CODE);
    }

    return (int)msg_status;
}



/**
 ****************************************************************************************
 * @brief Handles reception of the @ref LANC_LN_CTNL_PT_CFG_CMD message.
 * @param[in] msgid Id of the message received.
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance.
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int lanc_ln_ctnl_pt_cfg_cmd_handler(ke_msg_id_t const msgid,
                                         struct lanc_ln_ctnl_pt_cfg_cmd *param,
                                         ke_task_id_t const dest_id,
                                         ke_task_id_t const src_id)
{
    // Message status
    uint8_t msg_status = KE_MSG_CONSUMED;
    // Get the address of the environment
    struct lanc_env_tag *lanc_env = PRF_CLIENT_GET_ENV(dest_id, lanc);

    if (lanc_env != NULL)
    {
        // Status
        uint8_t status = PRF_ERR_OK;

        do
        {
            // State is Connected or Busy
            ASSERT_ERR(ke_state_get(dest_id) > LANC_IDLE);

            // Check the provided connection handle
            if (gapc_get_conidx(param->conhdl) != lanc_env->con_info.conidx)
            {
                status = PRF_ERR_INVALID_PARAM;
                break;
            }

            if (ke_state_get(dest_id) == LANC_BUSY)
            {
                // Another procedure is pending, keep the command for later
                msg_status = KE_MSG_SAVED;
                // Status is PRF_ERR_OK, no message will be sent to the application
                break;
            }

            // Check if the characteristic has been found
            if (lanc_env->lans.descs[LANC_DESC_LN_CTNL_PT_CL_CFG].desc_hdl != ATT_INVALID_SEARCH_HANDLE)
            {
                // Request array declaration
                uint8_t req[LANP_LAN_LN_CNTL_PT_REQ_MAX_LEN];
                // Pack request
                uint8_t nb = lanc_pack_ln_ctnl_pt_req (param, req, &status);

                if (status == PRF_ERR_OK)
                {
                    // Set the operation code
                    param->operation = LANC_LN_CTNL_PT_CFG_WR_OP_CODE;

                    // Store the command structure
                    lanc_env->operation   = param;
                    // Store the command information
                    msg_status = KE_MSG_NO_FREE;

                    // Go to the Busy state
                    ke_state_set(dest_id, LANC_BUSY);

                    // Send the write request
                    prf_gatt_write(&(lanc_env->con_info), lanc_env->lans.chars[LANP_LANS_LN_CTNL_PT_CHAR].val_hdl,
                                   (uint8_t *)&req[0], nb, GATTC_WRITE);
                }
            }
            else
            {
                status = PRF_ERR_INEXISTENT_HDL;
            }
        } while (0);

        if (status != PRF_ERR_OK)
        {
            // Send a complete event status to the application
            lanc_send_cmp_evt(lanc_env, LANC_LN_CTNL_PT_CFG_WR_OP_CODE, status);
        }
    }
    else
    {
        // No connection
        lanc_send_no_conn_cmp_evt(dest_id, src_id, param->conhdl, LANC_LN_CTNL_PT_CFG_WR_OP_CODE);
    }

    return (int)msg_status;
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref LANC_TIMEOUT_TIMER_IND message. This message is
 * received when the peer device doesn't send a SC Control Point indication within 30s
 * after reception of the write response.
 * @param[in] msgid Id of the message received.
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance.
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int lanc_timeout_timer_ind_handler(ke_msg_id_t const msgid,
                                           void const *param,
                                           ke_task_id_t const dest_id,
                                           ke_task_id_t const src_id)
{
    // Get the address of the environment
    struct lanc_env_tag *lanc_env = PRF_CLIENT_GET_ENV(dest_id, lanc);

    if (lanc_env != NULL)
    {
        ASSERT_ERR(lanc_env->operation != NULL);
        ASSERT_ERR(((struct lanc_cmd *)lanc_env->operation)->operation == LANC_LN_CTNL_PT_CFG_IND_OP_CODE);

        // Send the complete event message
        lanc_send_cmp_evt(lanc_env, LANC_LN_CTNL_PT_CFG_WR_OP_CODE, PRF_ERR_PROC_TIMEOUT);
    }
    // else drop the message

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref GATTC_DISC_SVC_IND message.
 * The handler stores the found service details for service discovery.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gattc_disc_svc_ind_handler(ke_msg_id_t const msgid,
                                      struct gattc_disc_svc_ind const *param,
                                      ke_task_id_t const dest_id,
                                      ke_task_id_t const src_id)
{
    // Get the address of the environment
    struct lanc_env_tag *lanc_env = PRF_CLIENT_GET_ENV(dest_id, lanc);

    // Check if the environment exists
    if (lanc_env != NULL)
    {
        ASSERT_ERR(lanc_env->operation != NULL);
        ASSERT_ERR(((struct lanc_cmd *)lanc_env->operation)->operation == LANC_ENABLE_OP_CODE);

        // Keep only one instance of the service
        if (lanc_env->nb_svc == 0)
        {
            // Keep the start handle and the end handle of the service
            lanc_env->lans.svc.shdl = param->start_hdl;
            lanc_env->lans.svc.ehdl = param->end_hdl;
            lanc_env->nb_svc++;
        }
    }
    // else drop the message

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref GATTC_DISC_CHAR_IND message.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gattc_disc_char_ind_handler(ke_msg_id_t const msgid,
                                       struct gattc_disc_char_ind const *param,
                                       ke_task_id_t const dest_id,
                                       ke_task_id_t const src_id)
{
    // Get the address of the environment
    struct lanc_env_tag *lanc_env = PRF_CLIENT_GET_ENV(dest_id, lanc);

    // Check if the environment exists
    if (lanc_env != NULL)
    {
        ASSERT_ERR(lanc_env->operation != NULL);
        ASSERT_ERR(((struct lanc_cmd *)lanc_env->operation)->operation == LANC_ENABLE_OP_CODE);

        // Retrieve LNS characteristics
        prf_search_chars(lanc_env->lans.svc.ehdl, LANP_LANS_CHAR_MAX,
                         &lanc_env->lans.chars[0], &lanc_lans_char[0],
                         param, (uint8_t *)&lanc_env->last_char_code);
    }
    // else drop the message

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref GATTC_DISC_CHAR_DESC_IND message.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gattc_disc_char_desc_ind_handler(ke_msg_id_t const msgid,
                                           struct gattc_disc_char_desc_ind const *param,
                                           ke_task_id_t const dest_id,
                                           ke_task_id_t const src_id)
{
    // Get the address of the environment
    struct lanc_env_tag *lanc_env = PRF_CLIENT_GET_ENV(dest_id, lanc);

    // Check if the environment exists
    if (lanc_env != NULL)
    {
        ASSERT_ERR(lanc_env->operation != NULL);
        ASSERT_ERR(((struct lanc_cmd *)lanc_env->operation)->operation == LANC_ENABLE_OP_CODE);

        // Retrieve LNS descriptors
        prf_search_descs(LANC_DESC_MAX, &lanc_env->lans.descs[0], &lanc_lns_char_desc[0],
                         param, lanc_env->last_req);
    }
    // else drop the message

    return (KE_MSG_CONSUMED);
}

/**
 ****************************************************************************************
 * @brief Handles reception of the @ref GATTC_CMP_EVT message.
 * @param[in] msgid Id of the message received.
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance.
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gattc_cmp_evt_handler(ke_msg_id_t const msgid,
                                 struct gattc_cmp_evt const *param,
                                 ke_task_id_t const dest_id,
                                 ke_task_id_t const src_id)
{
    // Get the address of the environment
    struct lanc_env_tag *lanc_env = PRF_CLIENT_GET_ENV(dest_id, lanc);

    if (lanc_env != NULL)
    {
        switch (param->req_type)
        {
            case (GATTC_DISC_BY_UUID_SVC):
            case (GATTC_DISC_ALL_CHAR):
            case (GATTC_DISC_DESC_CHAR):
            {
                // Status
                uint8_t status = PRF_ERR_STOP_DISC_CHAR_MISSING;

                ASSERT_ERR(lanc_env->operation != NULL);
                ASSERT_ERR(((struct lanc_cmd *)lanc_env->operation)->operation == LANC_ENABLE_OP_CODE);

                if ((param->status == ATT_ERR_ATTRIBUTE_NOT_FOUND) ||
                    (param->status == ATT_ERR_NO_ERROR))
                {
                    // Discovery
                    status = lanc_discovery_procedure (lanc_env);
                }
                else
                {
                    status = param->status;
                }

                if (status != PRF_ERR_OK)
                {
                    // Stop discovery procedure.
                    lanc_send_cmp_evt(lanc_env, LANC_ENABLE_OP_CODE, status);
                }
            } break;

            case (GATTC_READ):
            {
                // Send the complete event status
                lanc_send_cmp_evt(lanc_env, LANC_READ_OP_CODE, param->status);
            } break;

            case (GATTC_WRITE):
            {
                uint8_t operation = ((struct lanc_cmd *)lanc_env->operation)->operation;

                if (operation == LANC_CFG_NTF_IND_OP_CODE)
                {
                    // Send the complete event status
                    lanc_send_cmp_evt(lanc_env, operation, param->status);
                }

                else if (operation == LANC_LN_CTNL_PT_CFG_WR_OP_CODE)
                {
                    if (param->status == PRF_ERR_OK)
                    {
                        // Start Timeout Procedure
                        ke_timer_set(LANC_TIMEOUT_TIMER_IND, dest_id, ATT_TRANS_RTX);

                        // Wait for the response indication
                        ((struct lanc_cmd *)lanc_env->operation)->operation = LANC_LN_CTNL_PT_CFG_IND_OP_CODE;
                    }
                    else
                    {
                        // Send the complete event status
                        lanc_send_cmp_evt(lanc_env, operation, param->status);
                    }
                }
            } break;

            case (GATTC_REGISTER):
            case (GATTC_UNREGISTER):
            {
                // Do nothing
            } break;

            default:
            {
                ASSERT_ERR(0);
            } break;
        }
    }
    // else ignore the message

    return (KE_MSG_CONSUMED);
}


/**
 ****************************************************************************************
 * @brief Handles reception of the @ref GATTC_READ_IND message.
 * Generic event received after every simple read command sent to peer server.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gattc_read_ind_handler(ke_msg_id_t const msgid,
                                    struct gattc_read_ind const *param,
                                    ke_task_id_t const dest_id,
                                    ke_task_id_t const src_id)
{
    // Get the address of the environment
    struct lanc_env_tag *lanc_env = PRF_CLIENT_GET_ENV(dest_id, lanc);

    if (lanc_env != NULL)
    {
        ASSERT_ERR(lanc_env->operation != NULL);
        ASSERT_ERR(((struct lanc_cmd *)lanc_env->operation)->operation == LANC_READ_OP_CODE);

        // Send the read value to the HL
        struct lanc_value_ind *ind = KE_MSG_ALLOC(LANC_VALUE_IND,
                                                   lanc_env->con_info.appid,
                                                   lanc_env->con_info.prf_id,
                                                   lanc_value_ind);

        ind->conhdl   = gapc_get_conhdl(lanc_env->con_info.conidx);

        switch (((struct lanc_read_cmd *)lanc_env->operation)->read_code)
        {
            // Read LN Feature Characteristic value
            case (LANC_RD_LN_FEAT):
            {
                ind->value.LN_feat = co_read32p(&param->value[0]);

                // Mask the reserved bits
//                ind->value.LN_feat &= LANP_FEAT_ALL_SUPP;
            } break;

            // Read Position Quality
            case (LANC_RD_POS_Q):
            {
                // Read position quality
                lanc_unpack_pos_q_ind (param, ind);

            } break;

            // Read Client Characteristic Configuration Descriptor value
            case (LANC_RD_WR_LOC_SPEED_CL_CFG):
            case (LANC_RD_WR_LN_CTNL_PT_CFG):
            case (LANC_RD_WR_NAVIGATION_CFG):
            {
                ind->value.ntf_cfg = co_read16p(&param->value[0]);
            } break;

            default:
            {
                ASSERT_ERR(0);
            } break;
        }

        ind->att_code = ((struct lanc_read_cmd *)lanc_env->operation)->read_code;

        // Send the message to the application
        ke_msg_send(ind);
    }
    // else drop the message

    return (KE_MSG_CONSUMED);
}





/**
 ****************************************************************************************
 * @brief Handles reception of the @ref GATTC_EVENT_IND message.
 * @param[in] msgid Id of the message received (probably unused).
 * @param[in] param Pointer to the parameters of the message.
 * @param[in] dest_id ID of the receiving task instance (probably unused).
 * @param[in] src_id ID of the sending task instance.
 * @return If the message was consumed or not.
 ****************************************************************************************
 */
static int gattc_event_ind_handler(ke_msg_id_t const msgid,
                                   struct gattc_event_ind const *param,
                                   ke_task_id_t const dest_id,
                                   ke_task_id_t const src_id)
{
    // Get the address of the environment
    struct lanc_env_tag *lanc_env = PRF_CLIENT_GET_ENV(dest_id, lanc);

    if (lanc_env != NULL)
    {
        switch (param->type)
        {
            case (GATTC_NOTIFY):
            {
                if (param->handle == lanc_env->lans.chars[LANP_LANS_LOC_SPEED_CHAR].val_hdl)
                {
                    //Unpack location and speed
                    lanc_unpack_loc_speed_ind (param, lanc_env);
                }
                else if (param->handle == lanc_env->lans.chars[LANP_LANS_NAVIG_CHAR].val_hdl)
                {
                    // Unpack navigation
                    lanc_unpack_navigation_ind (param, lanc_env);
                }
                else
                {
                    ASSERT_ERR(0);
                }
            } break;

            case (GATTC_INDICATE):
            {
                // Check if we were waiting for the indication
                if (lanc_env->operation != NULL)
                {
                    if (((struct lanc_cmd *)lanc_env->operation)->operation == LANC_LN_CTNL_PT_CFG_IND_OP_CODE)
                    {
                        // Stop the procedure timeout timer
                        ke_timer_clear(LANC_TIMEOUT_TIMER_IND, dest_id);

                        //Unpack control point
                        lanc_unpack_ln_ctln_pt_ind (param, lanc_env);

                        // Send the complete event message
                        lanc_send_cmp_evt(lanc_env, LANC_LN_CTNL_PT_CFG_WR_OP_CODE, PRF_ERR_OK);
                    }
                    // else drop the message
                }
                // else drop the message
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
 * @brief Disconnection indication to LANC.
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
    // Get the address of the environment
    struct lanc_env_tag *lanc_env = PRF_CLIENT_GET_ENV(dest_id, lanc);

    ASSERT_ERR(lanc_env != NULL);

    // Free the stored operation if needed
    if (lanc_env->operation != NULL)
    {
        // Check if we were waiting for a SC Control Point indication
        if (((struct lanc_cmd *)lanc_env->operation)->operation == LANC_LN_CTNL_PT_CFG_IND_OP_CODE)
        {
            // Stop the procedure timeout timer
            ke_timer_clear(LANC_TIMEOUT_TIMER_IND, dest_id);
        }

        ke_msg_free(ke_param2msg(lanc_env->operation));
        lanc_env->operation = NULL;
    }

    PRF_CLIENT_DISABLE_IND_SEND(lanc_envs, dest_id, LANC, param->conhdl);

    return (KE_MSG_CONSUMED);
}

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// Specifies the default message handlers
const struct ke_msg_handler lanc_default_state[] =
{
    {LANC_ENABLE_CMD,               (ke_msg_func_t)lanc_enable_cmd_handler},
    {LANC_READ_CMD,                 (ke_msg_func_t)lanc_read_cmd_handler},
    {LANC_CFG_NTFIND_CMD,           (ke_msg_func_t)lanc_cfg_ntfind_cmd_handler},
    {LANC_LN_CTNL_PT_CFG_CMD,       (ke_msg_func_t)lanc_ln_ctnl_pt_cfg_cmd_handler},

    {LANC_TIMEOUT_TIMER_IND,        (ke_msg_func_t)lanc_timeout_timer_ind_handler},

    {GATTC_DISC_SVC_IND,             (ke_msg_func_t)gattc_disc_svc_ind_handler},
    {GATTC_DISC_CHAR_IND,            (ke_msg_func_t)gattc_disc_char_ind_handler},
    {GATTC_DISC_CHAR_DESC_IND,       (ke_msg_func_t)gattc_disc_char_desc_ind_handler},

    {GATTC_READ_IND,                 (ke_msg_func_t)gattc_read_ind_handler},
    {GATTC_EVENT_IND,                (ke_msg_func_t)gattc_event_ind_handler},
    {GATTC_CMP_EVT,                  (ke_msg_func_t)gattc_cmp_evt_handler},

    {GAPC_DISCONNECT_IND,           (ke_msg_func_t)gapc_disconnect_ind_handler},
};

// Specifies the message handler structure for every input state.
const struct ke_state_handler lanc_state_handler[LANC_STATE_MAX] =
{
    [LANC_IDLE]        = KE_STATE_HANDLER_NONE,
    [LANC_CONNECTED]   = KE_STATE_HANDLER_NONE,
    [LANC_BUSY]        = KE_STATE_HANDLER_NONE,
};

// Specifies the message handlers that are common to all states.
const struct ke_state_handler lanc_default_handler = KE_STATE_HANDLER(lanc_default_state);

// Defines the place holder for the states of all the task instances.
ke_state_t lanc_state[LANC_IDX_MAX] __attribute__((section("retention_mem_area0"),zero_init)); //@RETENTION MEMORY

#endif //(BLE_LN_COLLECTOR)

/// @} LANCTASK
